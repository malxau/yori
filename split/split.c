/**
 * @file split/split.c
 *
 * Yori shell split a file into pieces
 *
 * Copyright (c) 2018-2019 Malcolm J. Smith
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
CHAR strSplitHelpText[] =
        "\n"
        "Split a file into pieces.\n"
        "\n"
        "SPLIT [-license] [-j] [-l n | -b n] [-p <prefix>] [<file>]\n"
        "\n"
        "   -b             Use <n> bytes per part\n"
        "   -j             Join files previously split into one\n"
        "   -l             Use <n> number of lines per part\n"
        "   -p             Specify the prefix of part files\n";

/**
 Display usage text to the user.
 */
BOOL
SplitHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Split %i.%02i\n"), SPLIT_VER_MAJOR, SPLIT_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSplitHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _SPLIT_CONTEXT {

    /**
     If TRUE, the contents should be split into portions based on a determined
     number of lines.  If FALSE, the contents should be split into portions
     based on the number of bytes.
     */
    BOOL LinesMode;

    /**
     If LinesMode is FALSE, specifies the number of bytes per part.
     */
    LONGLONG BytesPerPart;

    /**
     If LinesMode is TRUE, specifies the number of lines per part.
     */
    LONGLONG LinesPerPart;

    /**
     Indicates the next part to open.
     */
    LONGLONG CurrentPartNumber;

    /**
     A string containing the prefix of newly created split fragments. The
     fragment number is appended to this prefix.
     */
    YORI_STRING Prefix;

} SPLIT_CONTEXT, *PSPLIT_CONTEXT;

/**
 Open a file in which to output the result of a fragment of the split
 operation.

 @param SplitContext Pointer to a context describing the split operation
        and its current state.

 @return Handle to the opened object, or NULL on failure.
 */
HANDLE
SplitOpenTargetForCurrentPart(
    __in PSPLIT_CONTEXT SplitContext
    )
{
    LPTSTR NewFileName;
    YORI_STRING NumberString;
    HANDLE hDestFile;

    YoriLibInitEmptyString(&NumberString);
    if (!YoriLibNumberToString(&NumberString, SplitContext->CurrentPartNumber, 10, 0, '\0')) {
        return NULL;
    }

    NewFileName = YoriLibMalloc((SplitContext->Prefix.LengthInChars + NumberString.LengthInChars + 1) * sizeof(TCHAR));
    if (NewFileName == NULL) {
        YoriLibFreeStringContents(&NumberString);
        return NULL;
    }

    YoriLibSPrintf(NewFileName, _T("%y%y"), &SplitContext->Prefix, &NumberString);
    YoriLibFreeStringContents(&NumberString);

    hDestFile = CreateFile(NewFileName,
                           GENERIC_WRITE,
                           FILE_SHARE_READ|FILE_SHARE_DELETE,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);
    if (hDestFile == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: open of %s failed: %s"), NewFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFree(NewFileName);
        return NULL;
    }
    YoriLibFree(NewFileName);

    return hDestFile;
}

/**
 Take a single incoming stream and break it into pieces.

 @param hSource A handle to the incoming stream, which may be a file or a
        pipe.
 
 @param SplitContext Pointer to a context describing the actions to perform.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SplitProcessStream(
    __in HANDLE hSource,
    __in PSPLIT_CONTEXT SplitContext
    )
{
    HANDLE hDestFile = NULL;

    if (SplitContext->LinesMode) {
        PVOID LineContext = NULL;
        YORI_STRING LineString;
        LONGLONG LineNumber;

        LineNumber = 0;
        YoriLibInitEmptyString(&LineString);
        while (TRUE) {
            if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
                break;
            }

            if ((LineNumber % SplitContext->LinesPerPart) == 0) {
                if (hDestFile != NULL) {
                    CloseHandle(hDestFile);
                    hDestFile = NULL;
                }
            }

            if (hDestFile == NULL) {
                hDestFile = SplitOpenTargetForCurrentPart(SplitContext);
                if (hDestFile == NULL) {
                    YoriLibLineReadClose(LineContext);
                    YoriLibFreeStringContents(&LineString);
                    return FALSE;
                }
                SplitContext->CurrentPartNumber++;
            }

            YoriLibOutputToDevice(hDestFile, 0, _T("%y\n"), &LineString);
            LineNumber++;
        }

        YoriLibLineReadClose(LineContext);
        YoriLibFreeStringContents(&LineString);
    } else {
        PVOID Buffer;
        ULONG BytesRead;

        Buffer = YoriLibMalloc((DWORD)SplitContext->BytesPerPart);
        if (Buffer == NULL) {
            return FALSE;
        }

        while (TRUE) {
            if (!ReadFile(hSource, Buffer, (DWORD)SplitContext->BytesPerPart, &BytesRead, NULL)) {
                break;
            }

            if (BytesRead == 0) {
                break;
            }

            hDestFile = SplitOpenTargetForCurrentPart(SplitContext);
            if (hDestFile == NULL) {
                YoriLibFree(Buffer);
                return FALSE;
            }
            SplitContext->CurrentPartNumber++;

            if (!WriteFile(hDestFile, Buffer, BytesRead, &BytesRead, NULL)) {
                DWORD LastError = GetLastError();
                LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: write failed: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
                CloseHandle(hDestFile);
                YoriLibFree(Buffer);
                return FALSE;
            }

            CloseHandle(hDestFile);
        }

        YoriLibFree(Buffer);
    }

    return TRUE;
}

/**
 Join a series of files with a given prefix back into a single file.  This is
 the inverse of split.

 @param Prefix Pointer to the string containing the prefix name of the set of
        files.

 @param OutputFile Pointer to the string containing the name of the combined
        file to generate.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SplitJoin(
    __in PYORI_STRING Prefix,
    __in PYORI_STRING OutputFile
    )
{
    HANDLE SourceHandle;
    HANDLE TargetHandle;
    PUCHAR Buffer;
    DWORD BytesRead;
    DWORD BytesAllocated;
    LONGLONG CurrentFragment;
    LPTSTR FragmentFileName;
    YORI_STRING NumberString;
    DWORD LastError;
    LPTSTR ErrText;

    ASSERT(YoriLibIsStringNullTerminated(OutputFile));

    BytesAllocated = 256 * 1024;

    Buffer = YoriLibMalloc(BytesAllocated);
    if (Buffer == NULL) {
        return FALSE;
    }

    TargetHandle = CreateFile(OutputFile->StartOfString,
                              GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_DELETE,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                              NULL);

    if (TargetHandle == NULL || TargetHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: open of %y failed: %s"), OutputFile, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFree(Buffer);
        return FALSE;
    }

    CurrentFragment = 0;

    while(TRUE) {

        YoriLibInitEmptyString(&NumberString);
        if (!YoriLibNumberToString(&NumberString, CurrentFragment, 10, 0, '\0')) {
            CloseHandle(TargetHandle);
            YoriLibFree(Buffer);
            return FALSE;
        }

        FragmentFileName = YoriLibMalloc((Prefix->LengthInChars + NumberString.LengthInChars + 1) * sizeof(TCHAR));
        if (FragmentFileName == NULL) {
            YoriLibFreeStringContents(&NumberString);
            CloseHandle(TargetHandle);
            YoriLibFree(Buffer);
            return FALSE;
        }

        YoriLibSPrintf(FragmentFileName, _T("%y%y"), Prefix, &NumberString);
        YoriLibFreeStringContents(&NumberString);

        SourceHandle = CreateFile(FragmentFileName,
                                  GENERIC_READ,
                                  FILE_SHARE_READ|FILE_SHARE_DELETE,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                                  NULL);
        if (SourceHandle == INVALID_HANDLE_VALUE) {
            LastError = GetLastError();
            if (LastError == ERROR_FILE_NOT_FOUND && CurrentFragment > 0) {
                YoriLibFree(FragmentFileName);
                break;
            }
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: open of %s failed: %s"), FragmentFileName, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            YoriLibFree(FragmentFileName);
            CloseHandle(TargetHandle);
            YoriLibFree(Buffer);
            return FALSE;
        }

        while(TRUE) {

            if (!ReadFile(SourceHandle, Buffer, BytesAllocated, &BytesRead, NULL)) {
                LastError = GetLastError();
                if (LastError == ERROR_HANDLE_EOF) {
                    break;
                }
                ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: read of %s failed: %s"), FragmentFileName, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                YoriLibFree(FragmentFileName);
                CloseHandle(TargetHandle);
                CloseHandle(SourceHandle);
                YoriLibFree(Buffer);
                return FALSE;
            }

            if (BytesRead == 0) {
                break;
            }

            if (!WriteFile(TargetHandle, Buffer, BytesRead, &BytesRead, NULL)) {
                LastError = GetLastError();
                ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: write to %y failed: %s"), OutputFile, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                YoriLibFree(FragmentFileName);
                CloseHandle(TargetHandle);
                CloseHandle(SourceHandle);
                YoriLibFree(Buffer);
                return FALSE;
            }
        }

        CloseHandle(SourceHandle);
        YoriLibFree(FragmentFileName);
        CurrentFragment++;
    }

    YoriLibFree(Buffer);
    CloseHandle(TargetHandle);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the split builtin command.
 */
#define ENTRYPOINT YoriCmd_YSPLIT
#else
/**
 The main entrypoint for the split standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the split cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, typically zero for success and nonzero
         for failure.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    SPLIT_CONTEXT SplitContext;
    YORI_STRING Arg;
    BOOL JoinMode = FALSE;

    ZeroMemory(&SplitContext, sizeof(SplitContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SplitHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                if (ArgC > i + 1) {
                    DWORD CharsConsumed;
                    ArgumentUnderstood = TRUE;
                    SplitContext.LinesMode = FALSE;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &SplitContext.BytesPerPart, &CharsConsumed);
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("j")) == 0) {
                JoinMode = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                if (ArgC > i + 1) {
                    DWORD CharsConsumed;
                    ArgumentUnderstood = TRUE;
                    SplitContext.LinesMode = TRUE;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &SplitContext.LinesPerPart, &CharsConsumed);
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                if (ArgC > i + 1) {
                    ArgumentUnderstood = TRUE;
                    YoriLibFreeStringContents(&SplitContext.Prefix);
                    YoriLibUserStringToSingleFilePath(&ArgV[i + 1], TRUE, &SplitContext.Prefix);
                    i++;
                }
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

    if (StartArg == ArgC) {
        StartArg = 0;
    }

    if (SplitContext.Prefix.StartOfString == NULL) {
        if (StartArg != 0 && !JoinMode) {
            YORI_STRING DefaultPrefix;

            if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &DefaultPrefix)) {
                return EXIT_FAILURE;
            }
            YoriLibYPrintf(&SplitContext.Prefix, _T("%y."), &DefaultPrefix);
            YoriLibFreeStringContents(&DefaultPrefix);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: no prefix specified\n"));
            return EXIT_FAILURE;
        }
    }

    if (JoinMode) {
        if (StartArg == 0) {
            YoriLibFreeStringContents(&SplitContext.Prefix);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: no input specified\n"));
            return EXIT_FAILURE;
        }

        if (!SplitJoin(&SplitContext.Prefix, &ArgV[StartArg])) {
            YoriLibFreeStringContents(&SplitContext.Prefix);
            return EXIT_FAILURE;
        }
        YoriLibFreeStringContents(&SplitContext.Prefix);
    } else {
        if (SplitContext.LinesMode) {
            if (SplitContext.LinesPerPart == 0) {
                YoriLibFreeStringContents(&SplitContext.Prefix);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: invalid lines per part\n"));
                return EXIT_FAILURE;
            }
        } else {
            if (SplitContext.BytesPerPart == 0 || SplitContext.BytesPerPart >= (DWORD)-1) {
                YoriLibFreeStringContents(&SplitContext.Prefix);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: invalid bytes per part\n"));
                return EXIT_FAILURE;
            }
        }

        //
        //  Attempt to enable backup privilege so an administrator can access more
        //  objects successfully.
        //

        YoriLibEnableBackupPrivilege();

        //
        //  If no file name is specified, use stdin; otherwise open
        //  the file and use that
        //

        if (StartArg == 0) {
            if (YoriLibIsStdInConsole()) {
                YoriLibFreeStringContents(&SplitContext.Prefix);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: no file or pipe for input\n"));
                return EXIT_FAILURE;
            }

            if (!SplitProcessStream(GetStdHandle(STD_INPUT_HANDLE), &SplitContext)) {
                YoriLibFreeStringContents(&SplitContext.Prefix);
                return EXIT_FAILURE;
            }
        } else {
            HANDLE FileHandle;
            YORI_STRING FilePath;

            if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &FilePath)) {
                return EXIT_FAILURE;
            }
            FileHandle = CreateFile(FilePath.StartOfString,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL);

            if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
                DWORD LastError = GetLastError();
                LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: open of %y failed: %s"), &FilePath, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                return TRUE;
            }

            YoriLibFreeStringContents(&FilePath);

            if (!SplitProcessStream(FileHandle, &SplitContext)) {
                CloseHandle(FileHandle);
                YoriLibFreeStringContents(&SplitContext.Prefix);
                return EXIT_FAILURE;
            }
            CloseHandle(FileHandle);
        }
        YoriLibFreeStringContents(&SplitContext.Prefix);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
