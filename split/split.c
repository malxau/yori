/**
 * @file split/split.c
 *
 * Yori shell split a file into pieces
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
CHAR strHelpText[] =
        "\n"
        "Split a file into pieces.\n"
        "\n"
        "SPLIT [-license] [-l n | -b n] [-p <prefix>] [<file>]\n"
        "\n"
        "   -b             Use <n> bytes per part\n"
        "   -l             Use <n> number of lines per part\n"
        "   -p             Specify the prefix of part files\n";

/**
 Display usage text to the user.
 */
BOOL
SplitHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Split %i.%i\n"), SPLIT_VER_MAJOR, SPLIT_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
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

    hDestFile = CreateFile(NewFileName, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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
 The main entrypoint for the split cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, typically zero for success and nonzero
         for failure.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    SPLIT_CONTEXT SplitContext;
    YORI_STRING Arg;

    ZeroMemory(&SplitContext, sizeof(SplitContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SplitHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                if (ArgC > i + 1) {
                    DWORD CharsConsumed;
                    ArgumentUnderstood = TRUE;
                    SplitContext.LinesMode = FALSE;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &SplitContext.BytesPerPart, &CharsConsumed);
                    i++;
                }
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

    if (SplitContext.Prefix.StartOfString == NULL) {
        if (StartArg != 0) {
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

    if (SplitContext.LinesMode) {
        if (SplitContext.LinesPerPart == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: invalid lines per part\n"));
            return EXIT_FAILURE;
        }
    } else {
        if (SplitContext.BytesPerPart == 0 || SplitContext.BytesPerPart >= (DWORD)-1) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: invalid bytes per part\n"));
            return EXIT_FAILURE;
        }
    }

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0) {
        DWORD FileType = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
        FileType = FileType & ~(FILE_TYPE_REMOTE);
        if (FileType == FILE_TYPE_CHAR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("split: no file or pipe for input\n"));
            return EXIT_FAILURE;
        }

        if (!SplitProcessStream(GetStdHandle(STD_INPUT_HANDLE), &SplitContext)) {
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
                                FILE_ATTRIBUTE_NORMAL,
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
            return EXIT_FAILURE;
        }
        CloseHandle(FileHandle);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
