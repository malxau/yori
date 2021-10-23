/**
 * @file petool/petool.c
 *
 * Yori shell PE tool for manipulating PE files
 *
 * Copyright (c) 2021 Malcolm J. Smith
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

/**
 Help text to display to the user.
 */
const
CHAR strPeToolHelpText[] =
        "\n"
        "Manage PE files.\n"
        "\n"
        "PETOOL [-license]\n"
        "PETOOL -c file\n"
        "PETOOL -cu file\n"
        "\n"
        "   -c             Calculate the PE checksum for a binary\n"
        "   -cu            Update the checksum in the PE header from contents\n";

/**
 Display usage text to the user.
 */
BOOL
PeToolHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("PeTool %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strPeToolHelpText);
    return TRUE;
}

/**
 Calculate and display the checksum for a specified file.

 @param FileName Pointer to the file name to calculate a checksum for.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
PeToolCalculateChecksum(
    __in PYORI_STRING FileName
    )
{
    DWORD Err;
    DWORD HeaderChecksum;
    DWORD DataChecksum;
    YORI_STRING FullPath;

    YoriLibLoadImageHlpFunctions();

    if (DllImageHlp.pMapFileAndCheckSumW == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("petool: OS support not present\n"));
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        return FALSE;
    }

    Err = DllImageHlp.pMapFileAndCheckSumW(FullPath.StartOfString, &HeaderChecksum, &DataChecksum);
    if (Err != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %y\n"), FileName);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Checksum in PE header: %08x\n")
                                          _T("Checksum of file contents: %08x\n"),
                                          HeaderChecksum,
                                          DataChecksum);

    YoriLibFreeStringContents(&FullPath);
    return TRUE;
}

/**
 Generate the checksum for a binary and update the header.

 @param FileName Pointer to the file name to generate a checksum for.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
PeToolUpdateChecksum(
    __in PYORI_STRING FileName
    )
{
    DWORD Err;
    DWORD HeaderChecksum;
    DWORD DataChecksum;
    DWORD PeHeaderOffset;
    HANDLE hFileRead;
    union {
        IMAGE_DOS_HEADER DosHeader;
        YORILIB_PE_HEADERS PeHeaders;
    } u;
    DWORD BytesReturned;
    YORI_STRING FullPath;

    YoriLibLoadImageHlpFunctions();

    if (DllImageHlp.pMapFileAndCheckSumW == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("petool: OS support not present\n"));
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        return FALSE;
    }

    Err = DllImageHlp.pMapFileAndCheckSumW(FullPath.StartOfString, &HeaderChecksum, &DataChecksum);
    if (Err != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %y\n"), FileName);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }


    ASSERT(YoriLibIsStringNullTerminated(&FullPath));

    //
    //  We want the earlier handle to be attribute only so we can
    //  operate on directories, but we need data for this, so we
    //  end up with two handles.
    //

    hFileRead = CreateFile(FullPath.StartOfString,
                           FILE_READ_ATTRIBUTES|FILE_READ_DATA|FILE_WRITE_DATA,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);

    if (hFileRead == INVALID_HANDLE_VALUE) {
        LPTSTR ErrText;
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %y: %s"), &FullPath, ErrText);
        YoriLibFreeStringContents(&FullPath);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }


    if (ReadFile(hFileRead, &u.DosHeader, sizeof(u.DosHeader), &BytesReturned, NULL) &&
        BytesReturned == sizeof(u.DosHeader) &&
        u.DosHeader.e_magic == IMAGE_DOS_SIGNATURE &&
        u.DosHeader.e_lfanew != 0) {

        PeHeaderOffset = u.DosHeader.e_lfanew;
        SetFilePointer(hFileRead, PeHeaderOffset, NULL, FILE_BEGIN);

        if (ReadFile(hFileRead, &u.PeHeaders, sizeof(YORILIB_PE_HEADERS), &BytesReturned, NULL) &&
            BytesReturned == sizeof(YORILIB_PE_HEADERS) &&
            u.PeHeaders.Signature == IMAGE_NT_SIGNATURE &&
            u.PeHeaders.ImageHeader.SizeOfOptionalHeader >= FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(u.PeHeaders.OptionalHeader.CheckSum)) {

            u.PeHeaders.OptionalHeader.CheckSum = DataChecksum;
            SetFilePointer(hFileRead, PeHeaderOffset, NULL, FILE_BEGIN);

            if (!WriteFile(hFileRead, &u.PeHeaders, sizeof(YORILIB_PE_HEADERS), &BytesReturned, NULL)) {
                CloseHandle(hFileRead);
                YoriLibFreeStringContents(&FullPath);
                return FALSE;
            }
        }
    }

    CloseHandle(hFileRead);
    YoriLibFreeStringContents(&FullPath);

    return TRUE;
}

/**
 A set of operations supported by this application.
 */
typedef enum _PETOOL_OP {
    PeToolOpNone = 0,
    PeToolOpCalculateChecksum = 1,
    PeToolOpUpdateChecksum = 2
} PETOOL_OP;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the petool builtin command.
 */
#define ENTRYPOINT YoriCmd_PETOOL
#else
/**
 The main entrypoint for the petool standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the petool cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg;
    YORI_STRING Arg;
    PYORI_STRING FileName = NULL;
    PETOOL_OP Op;
    DWORD Result;

    Op = PeToolOpNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                PeToolHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    FileName = &ArgV[i + 1];
                    Op = PeToolOpCalculateChecksum;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("cu")) == 0) {
                if (ArgC > i + 1) {
                    FileName = &ArgV[i + 1];
                    Op = PeToolOpUpdateChecksum;
                    ArgumentUnderstood = TRUE;
                }
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

    if (Op == PeToolOpNone) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("petool: operation not specified\n"));
        return EXIT_FAILURE;
    }

    Result = EXIT_SUCCESS;

    switch(Op) {
        case PeToolOpCalculateChecksum:
            if (!PeToolCalculateChecksum(FileName)) {
                Result = EXIT_FAILURE;
            }
            break;
        case PeToolOpUpdateChecksum:
            if (!PeToolUpdateChecksum(FileName)) {
                Result = EXIT_FAILURE;
            }
            break;
    }

    return Result;
}

// vim:sw=4:ts=4:et:
