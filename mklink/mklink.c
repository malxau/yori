/**
 * @file mklink/mklink.c
 *
 * Yori shell display command line output
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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
CHAR strMklinkHelpText[] =
        "Creates hardlinks, symbolic links, or junctions.\n"
        "\n"
        "MKLINK [-license] [[-d]|[-f]|[-h]|[-j]] <link> <target>\n"
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
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Mklink %i.%02i\n"), MKLINK_VER_MAJOR, MKLINK_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strMklinkHelpText);
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
    PYORI_REPARSE_DATA_BUFFER ReparseData;
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

    ReparseData = YoriLibMalloc(sizeof(YORI_REPARSE_DATA_BUFFER) + ReparseDataLength);
    if (ReparseData == NULL) {
        return FALSE;
    }

    ReparseData->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    ReparseData->ReparseDataLength = (WORD)(ReparseDataLength + FIELD_OFFSET(YORI_REPARSE_DATA_BUFFER, u.MountPoint.Buffer) - FIELD_OFFSET(YORI_REPARSE_DATA_BUFFER, u.MountPoint));
    ReparseData->ReservedForAlignment = 0;
    ReparseData->u.MountPoint.RealNameOffsetInBytes = 0;
    ReparseData->u.MountPoint.RealNameLengthInBytes = (WORD)(ExistingFileLength * sizeof(TCHAR));
    ReparseData->u.MountPoint.DisplayNameOffsetInBytes = (WORD)(ExistingFileLength * sizeof(TCHAR) + sizeof(TCHAR));
    ReparseData->u.MountPoint.DisplayNameLengthInBytes = (WORD)((ExistingFileLength - 4) * sizeof(TCHAR));

    memcpy(YoriLibAddToPointer(ReparseData->u.MountPoint.Buffer, ReparseData->u.MountPoint.RealNameOffsetInBytes),
           ExistingFile,
           ExistingFileLength * sizeof(TCHAR) + sizeof(TCHAR));

    *(TCHAR*)YoriLibAddToPointer(ReparseData->u.MountPoint.Buffer, ReparseData->u.MountPoint.RealNameOffsetInBytes + sizeof(TCHAR)) = '?';

    memcpy(YoriLibAddToPointer(ReparseData->u.MountPoint.Buffer, ReparseData->u.MountPoint.DisplayNameOffsetInBytes),
           ExistingFile + 4,
           (ExistingFileLength - 4) * sizeof(TCHAR) + sizeof(TCHAR));

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
    NewFileHandle = CreateFile(NewLink,
                               FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA | SYNCHRONIZE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                               NULL);
    if (NewFileHandle == NULL || NewFileHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: open junction directory failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFree(ReparseData);
        return FALSE;
    }

    if (!DeviceIoControl(NewFileHandle, FSCTL_SET_REPARSE_POINT, ReparseData, FIELD_OFFSET(YORI_REPARSE_DATA_BUFFER, u.MountPoint.Buffer) + ReparseDataLength, NULL, 0, &BytesReturned, NULL)) {
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

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the mklink builtin command.
 */
#define ENTRYPOINT YoriCmd_YMKLINK
#else
/**
 The main entrypoint for the mklink standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the mklink cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code indicating success or failure for the application.
 */
DWORD
ENTRYPOINT(
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
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
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
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i + 1;
                ArgumentUnderstood = TRUE;
                break;
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

    if (StartArg == 0 || ArgC - StartArg < 2) {
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

    //
    //  Attempt to enable backup privilege so an administrator can access more
    //  objects successfully.
    //

    YoriLibEnableBackupPrivilege();

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
