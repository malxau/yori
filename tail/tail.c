/**
 * @file tail/tail.c
 *
 * Yori shell display the final lines in a file
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

#pragma warning(disable: 4220) // Varargs matches remaining parameters

/**
 Help text to display to the user.
 */
const
CHAR strTailHelpText[] =
        "\n"
        "Output the final lines of one or more files.\n"
        "\n"
        "TAIL [-license] [-b] [-f] [-s] [-n count] [-c line] [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Specify a line to display context around instead of EOF\n"
        "   -f             Wait for new output and continue outputting\n"
        "   -n             Specify the number of lines to display\n"
        "   -s             Process files from all subdirectories\n";

/**
 Display usage text to the user.
 */
BOOL
TailHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Tail %i.%i\n"), TAIL_VER_MAJOR, TAIL_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strTailHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _TAIL_CONTEXT {

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     Specifies the number of lines to display in each matching file.
     */
    ULONG LinesToDisplay;

    /**
     If nonzero, specifies the final line to display from each file.  This
     implies that tail is running in context mode, looking for a region in
     the middle of the file.
     */
    LONGLONG FinalLine;

    /**
     Specifies the number of lines that have been found from the current
     stream.
     */
    LONGLONG LinesFound;

    /**
     An array of LinesToDisplay YORI_STRING structures.
     */
    PYORI_STRING LinesArray;

    /**
     If TRUE, continue outputting results as more arrive.  If FALSE, terminate
     as soon as the requested lines have been output.
     */
    BOOL WaitForMore;

} TAIL_CONTEXT, *PTAIL_CONTEXT;

/**
 Process a single opened stream, enumerating through all lines and displaying
 the set requested by the user.

 @param hSource The opened source stream.

 @param TailContext Pointer to context information specifying which lines to
        display.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
TailProcessStream(
    __in HANDLE hSource,
    __in PTAIL_CONTEXT TailContext
    )
{
    PVOID LineContext = NULL;
    LONGLONG StartLine;
    LONGLONG CurrentLine;
    PYORI_STRING LineString;
    BOOL LineTerminated;
    BOOL TimeoutReached;

    TailContext->FilesFound++;
    TailContext->LinesFound = 0;

    while (TRUE) {

        if (!YoriLibReadLineToStringEx(&TailContext->LinesArray[TailContext->LinesFound % TailContext->LinesToDisplay], &LineContext, !TailContext->WaitForMore, INFINITE, hSource, &LineTerminated, &TimeoutReached)) {
            break;
        }

        TailContext->LinesFound++;

        if (TailContext->FinalLine != 0 && TailContext->LinesFound >= TailContext->FinalLine) {
            break;
        }
    }

    if (TailContext->LinesFound > TailContext->LinesToDisplay) {
        StartLine = TailContext->LinesFound - TailContext->LinesToDisplay;
    } else {
        StartLine = 0;
    }

    for (CurrentLine = StartLine; CurrentLine < TailContext->LinesFound; CurrentLine++) {
        LineString = &TailContext->LinesArray[CurrentLine % TailContext->LinesToDisplay];
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), LineString);
    }

    if (TailContext->WaitForMore) {
        while (TRUE) {

            if (!YoriLibReadLineToStringEx(&TailContext->LinesArray[0], &LineContext, FALSE, INFINITE, hSource, &LineTerminated, &TimeoutReached)) {
                Sleep(200);
                continue;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &TailContext->LinesArray[0]);
        }
    }

    YoriLibLineReadClose(LineContext);
    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param TailContext Pointer to the tail context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
TailFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PTAIL_CONTEXT TailContext
    )
{
    HANDLE FileHandle;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(FilePath->StartOfString[FilePath->LengthInChars] == '\0');

    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        FileHandle = CreateFile(FilePath->StartOfString,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

        if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tail: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        TailProcessStream(FileHandle, TailContext);

        CloseHandle(FileHandle);
    }

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the tail builtin command.
 */
#define ENTRYPOINT YoriCmd_TAIL
#else
/**
 The main entrypoint for the tail standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the tail cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero indicating success or nonzero on
         failure.
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
    DWORD MatchFlags;
    DWORD Count;
    BOOL Recursive = FALSE;
    BOOL BasicEnumeration = FALSE;
    TAIL_CONTEXT TailContext;
    LONGLONG ContextLine;
    YORI_STRING Arg;

    ZeroMemory(&TailContext, sizeof(TailContext));
    TailContext.LinesToDisplay = 10;
    ContextLine = -1;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                TailHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    DWORD CharsConsumed;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &ContextLine, &CharsConsumed);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                TailContext.WaitForMore = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                if (ArgC > i + 1) {
                    LONGLONG LineCount;
                    DWORD CharsConsumed;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &LineCount, &CharsConsumed);
                    if (LineCount != 0 && LineCount < 1 * 1024 * 1024) {
                        TailContext.LinesToDisplay = (DWORD)LineCount;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
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

    if (ContextLine != -1) {
        TailContext.FinalLine = ContextLine + TailContext.LinesToDisplay / 2;
    }

    TailContext.LinesArray = YoriLibMalloc(TailContext.LinesToDisplay * sizeof(YORI_STRING));
    if (TailContext.LinesArray == NULL) {
        return EXIT_FAILURE;
    }

    for (Count = 0; Count < TailContext.LinesToDisplay; Count++) {
        YoriLibInitEmptyString(&TailContext.LinesArray[Count]);
    }

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0) {
        DWORD FileType = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
        FileType = FileType & ~(FILE_TYPE_REMOTE);
        if (FileType == FILE_TYPE_CHAR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
            YoriLibFree(TailContext.LinesArray);
            return EXIT_FAILURE;
        }

        TailProcessStream(GetStdHandle(STD_INPUT_HANDLE), &TailContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }
    
        for (i = StartArg; i < ArgC; i++) {
    
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, TailFileFoundCallback, &TailContext);
        }
    }

    for (Count = 0; Count < TailContext.LinesToDisplay; Count++) {
        YoriLibFreeStringContents(&TailContext.LinesArray[Count]);
    }
    YoriLibFree(TailContext.LinesArray);

    if (TailContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tail: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
