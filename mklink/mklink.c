/**
 * @file mklink/mklink.c
 *
 * Yori shell display command line output
 *
 * Copyright (c) 2017 Malcolm J. Smith
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>
#include <winioctl.h>

/**
 Help text to display to the user.
 */
const
CHAR strHelpText[] =
        "Creates hardlinks, symbolic links, or junctions.\n"
        "\n"
        "MKLINK [[-d]|[-f]|[-h]|[-j]] <link> <target>\n"
        "\n"
        "   -d             Create a directory symbolic link\n"
        "   -f             Create a file symbolic link\n"
        "   -h             Create a hard link (files only)\n"
        "   -j             Create a junction (directories only)\n" ;

/**
 Display usage text to the user.
 */
BOOL
MklinkHelp()
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2017"), &License);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Mklink %i.%i\n"), MKLINK_VER_MAJOR, MKLINK_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s"), &License);
    YoriLibFreeStringContents(&License);
    return TRUE;
}


/**
 Create a hard link.

 @param NewLink Pointer to a NULL terminated string containing the name of the
        new link object to create.

 @param ExistingFile Pointer to a NULL terminated string containing the name
        of the existing file that the hard link should refer to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MklinkCreateHardLink(
    __in LPTSTR NewLink,
    __in LPTSTR ExistingFile
    )
{
    DWORD LastError;
    LPTSTR ErrText;

    if (DllKernel32.pCreateHardLinkW == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: create hard link failed: CreateHardLinkW export not found\n"));
        return FALSE;
    }

    if (!DllKernel32.pCreateHardLinkW(NewLink, ExistingFile, NULL)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: create hard link failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }
    return TRUE;
}

/**
 Create a symbolic link.

 @param NewLink Pointer to a NULL terminated string containing the name of the
        new link object to create.

 @param ExistingFile Pointer to a NULL terminated string containing the name
        of the existing file that the symbolic link should refer to.

 @param Flags Flags for the symbolic link.  Should be zero for a file
        symbolic link, or 1 for a directory symbolic link.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MklinkCreateSymbolicLink(
    __in LPTSTR NewLink,
    __in LPTSTR ExistingFile,
    __in DWORD Flags
    )
{
    DWORD LastError;
    LPTSTR ErrText;

    if (DllKernel32.pCreateSymbolicLinkW == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: create symbolic link failed: CreateSymbolicLinkW export not found"));
        return FALSE;
    }

    if (!DllKernel32.pCreateSymbolicLinkW(NewLink, ExistingFile, Flags)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: create symbolic link failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    return TRUE;
}

/**
 The basic reparse data to attach to a junction.
 */
typedef struct _MKLINK_JUNCTION_DATA {

    /**
     The reparse tag.  For a junction, this is 0xa0000003 .
     */
    ULONG ReparseTag;

    /**
     The length of the reparse data.  This means the length in bytes
     from the RealNameOffsetInBytes field.
     */
    USHORT ReparseDataLength;

    /**
     Currently unused.
     */
    USHORT Reserved;

    /**
     For a junction, the offset in bytes following this structure to the
     beginning of the name to substitute when the link is traversed.
     */
    USHORT RealNameOffsetInBytes;

    /**
     For a junction, the length in bytes of the name to substitute when the
     link is traversed.
     */
    USHORT RealNameLengthInBytes;

    /**
     For a junction, the offset in bytes following this structure to the
     beginning of the name to display when the user queries the link.
     */
    USHORT PrintNameOffsetInBytes;

    /**
     For a junction, the length in bytes of the name to display when the user
     queries the link.
     */
    USHORT PrintNameLengthInBytes;
} MKLINK_JUNCTION_DATA, *PMKLINK_JUNCTION_DATA;

/**
 Create a junction.

 @param NewLink Pointer to a NULL terminated string containing the name of the
        new link object to create.

 @param ExistingFile Pointer to a NULL terminated string containing the name
        of the existing file that the junction should refer to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MklinkCreateJunction(
    __in LPTSTR NewLink,
    __in LPTSTR ExistingFile
    )
{
    PMKLINK_JUNCTION_DATA ReparseData;
    USHORT ReparseDataLength;
    DWORD ExistingFileLength;
    HANDLE NewFileHandle;
    DWORD BytesReturned;
    DWORD LastError;
    LPTSTR ErrText;

    ExistingFileLength = _tcslen(ExistingFile);
    if (ExistingFileLength >= 0x3fff) {
        return FALSE;
    }
    ReparseDataLength = (WORD)((ExistingFileLength + 1) * 2 * sizeof(TCHAR) - 4 * sizeof(TCHAR));

    ReparseData = YoriLibMalloc(sizeof(MKLINK_JUNCTION_DATA) + ReparseDataLength);
    if (ReparseData == NULL) {
        return FALSE;
    }

    ReparseData->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    ReparseData->ReparseDataLength = (WORD)(ReparseDataLength + sizeof(MKLINK_JUNCTION_DATA) - FIELD_OFFSET(MKLINK_JUNCTION_DATA, RealNameOffsetInBytes));
    ReparseData->Reserved = 0;
    ReparseData->RealNameOffsetInBytes = 0;
    ReparseData->RealNameLengthInBytes = (WORD)(ExistingFileLength * sizeof(TCHAR));
    ReparseData->PrintNameOffsetInBytes = (WORD)(ExistingFileLength * sizeof(TCHAR) + sizeof(TCHAR));
    ReparseData->PrintNameLengthInBytes = (WORD)((ExistingFileLength - 4) * sizeof(TCHAR));

    memcpy(YoriLibAddToPointer(ReparseData, sizeof(MKLINK_JUNCTION_DATA) + ReparseData->RealNameOffsetInBytes),
           ExistingFile,
           ExistingFileLength * sizeof(TCHAR) + sizeof(TCHAR));

    *(TCHAR*)YoriLibAddToPointer(ReparseData, sizeof(MKLINK_JUNCTION_DATA) + ReparseData->RealNameOffsetInBytes + sizeof(TCHAR)) = '?';

    memcpy(YoriLibAddToPointer(ReparseData, sizeof(MKLINK_JUNCTION_DATA) + ReparseData->PrintNameOffsetInBytes),
           ExistingFile + 4,
           (ExistingFileLength - 4)* sizeof(TCHAR) + sizeof(TCHAR));

    if (!CreateDirectory(NewLink, NULL)) {
        LastError = GetLastError();
        if (LastError != ERROR_ALREADY_EXISTS) {
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: create junction directory failed: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            YoriLibFree(ReparseData);
            return FALSE;
        }
    }
    NewFileHandle = CreateFile(NewLink, FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (NewFileHandle == NULL || NewFileHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: open junction directory failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFree(ReparseData);
        return FALSE;
    }

    if (!DeviceIoControl(NewFileHandle, FSCTL_SET_REPARSE_POINT, ReparseData, sizeof(MKLINK_JUNCTION_DATA) + ReparseDataLength, NULL, 0, &BytesReturned, NULL)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: setting junction reparse data failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(NewFileHandle);
        YoriLibFree(ReparseData);
        return FALSE;
    }

    CloseHandle(NewFileHandle);
    YoriLibFree(ReparseData);
    return TRUE;
}


/**
 Specifies the type of link to create.
 */
typedef enum _MKLINK_LINK_TYPE {
    MklinkLinkTypeHard = 1,
    MklinkLinkTypeJunction = 2,
    MklinkLinkTypeFileSym = 3,
    MklinkLinkTypeDirSym = 4
} MKLINK_LINK_TYPE;

/**
 Pointer to the type of link to create.
 */
typedef MKLINK_LINK_TYPE *PMKLINK_LINK_TYPE;


/**
 The main entrypoint for the mklink cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code indicating success or failure for the application.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 1;
    MKLINK_LINK_TYPE LinkType = MklinkLinkTypeHard;
    YORI_STRING NewLinkName;
    YORI_STRING ExistingFileName;
    YORI_STRING Arg;
    DWORD ExitCode;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                MklinkHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                LinkType = MklinkLinkTypeDirSym;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                LinkType = MklinkLinkTypeFileSym;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                LinkType = MklinkLinkTypeHard;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("j")) == 0) {
                LinkType = MklinkLinkTypeJunction;
                ArgumentUnderstood = TRUE;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    i = StartArg;
    if (ArgC - StartArg < 2) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &NewLinkName)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: failed to resolve %y\n"), &ArgV[StartArg]);
        return EXIT_FAILURE;
    }

    if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg + 1], TRUE, &ExistingFileName)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: failed to resolve %y\n"), &ArgV[StartArg + 1]);
        YoriLibFreeStringContents(&NewLinkName);
        return EXIT_FAILURE;
    }

    ExitCode = EXIT_SUCCESS;

    switch(LinkType) {
        case MklinkLinkTypeHard:
            if (!MklinkCreateHardLink(NewLinkName.StartOfString, ExistingFileName.StartOfString)) {
                ExitCode = EXIT_FAILURE;
            }
            break;
        case MklinkLinkTypeDirSym:
            if (!MklinkCreateSymbolicLink(NewLinkName.StartOfString, ExistingFileName.StartOfString, 1)) {
                ExitCode = EXIT_FAILURE;
            }
            break;
        case MklinkLinkTypeFileSym:
            if (!MklinkCreateSymbolicLink(NewLinkName.StartOfString, ExistingFileName.StartOfString, 0)) {
                ExitCode = EXIT_FAILURE;
            }
            break;
        case MklinkLinkTypeJunction:
            if (!MklinkCreateJunction(NewLinkName.StartOfString, ExistingFileName.StartOfString)) {
                ExitCode = EXIT_FAILURE;
            }
            break;
        default:
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: link type not implemented\n"));
            ExitCode = EXIT_FAILURE;
            break;
    }

    YoriLibFreeStringContents(&NewLinkName);
    YoriLibFreeStringContents(&ExistingFileName);

    return ExitCode;
}

// vim:sw=4:ts=4:et:
