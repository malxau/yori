/**
 * @file lines/lines.c
 *
 * Yori shell display count of lines in files
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

/**
 Help text to display to the user.
 */
const
CHAR strHelpText[] =
        "\n"
        "Count the number of lines in one or more files.\n"
        "\n"
        "LINES [-license] [-b] [-s] [-t] [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -s             Process files from all subdirectories\n"
        "   -t             Display total line count of all files\n";

/**
 Display usage text to the user.
 */
BOOL
LinesHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Lines %i.%i\n"), LINES_VER_MAJOR, LINES_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _LINES_CONTEXT {

    /**
     TRUE to indicate that the only output should be the total number of
     lines, after all files have been processed.
     */
    BOOL SummaryOnly;

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     Records the number of lines found in a single file.
     */
    LONGLONG FileLinesFound;

    /**
     Records the total number of lines processed for all files.
     */
    LONGLONG TotalLinesFound;
} LINES_CONTEXT, *PLINES_CONTEXT;

/**
 Count the lines in an opened stream.

 @param hSource Handle to the source.

 @param LinesContext Specifies the context to record line count information.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
LinesProcessStream(
    __in HANDLE hSource,
    __in PLINES_CONTEXT LinesContext
    )
{
    PVOID LineContext = NULL;
    YORI_STRING LineString;

    YoriLibInitEmptyString(&LineString);

    LinesContext->FilesFound++;
    LinesContext->FileLinesFound = 0;

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
            break;
        }

        LinesContext->FileLinesFound++;
    }

    YoriLibLineReadClose(LineContext);
    YoriLibFreeStringContents(&LineString);

    LinesContext->TotalLinesFound += LinesContext->FileLinesFound;
    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies the recursion depth.  Ignored in this application.

 @param LinesContext Pointer to the lines context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
LinesFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PLINES_CONTEXT LinesContext
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
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("lines: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        LinesProcessStream(FileHandle, LinesContext);

        if (!LinesContext->SummaryOnly) {
            YORI_STRING StringFormOfLineCount;
            YORI_STRING UnescapedFilePath;
            TCHAR StackBuffer[16];

            YoriLibInitEmptyString(&StringFormOfLineCount);
            StringFormOfLineCount.StartOfString = StackBuffer;
            StringFormOfLineCount.LengthAllocated = sizeof(StackBuffer)/sizeof(StackBuffer[0]);
            YoriLibNumberToString(&StringFormOfLineCount, LinesContext->FileLinesFound, 10, 3, ',');
            YoriLibInitEmptyString(&UnescapedFilePath);
            YoriLibUnescapePath(FilePath, &UnescapedFilePath);
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%16y %y\n"), &StringFormOfLineCount, &UnescapedFilePath);
            YoriLibFreeStringContents(&StringFormOfLineCount);
            YoriLibFreeStringContents(&UnescapedFilePath);
        }

        CloseHandle(FileHandle);
    }

    return TRUE;
}

/**
 The main entrypoint for the lines cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
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
    DWORD MatchFlags;
    BOOL Recursive = FALSE;
    BOOL BasicEnumeration = FALSE;
    LINES_CONTEXT LinesContext;
    YORI_STRING Arg;

    ZeroMemory(&LinesContext, sizeof(LinesContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                LinesHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                LinesContext.SummaryOnly = TRUE;
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

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0) {
        DWORD FileType = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
        FileType = FileType & ~(FILE_TYPE_REMOTE);
        if (FileType == FILE_TYPE_CHAR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
            return EXIT_FAILURE;
        }

        LinesContext.SummaryOnly = TRUE;

        LinesProcessStream(GetStdHandle(STD_INPUT_HANDLE), &LinesContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }
    
        for (i = StartArg; i < ArgC; i++) {
    
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, LinesFileFoundCallback, &LinesContext);
        }
    }

    if (LinesContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("lines: no matching files found\n"));
        return EXIT_FAILURE;
    } else if (LinesContext.FilesFound > 1 || LinesContext.SummaryOnly) {
        YORI_STRING StringFormOfLineCount;

        YoriLibInitEmptyString(&StringFormOfLineCount);
        YoriLibNumberToString(&StringFormOfLineCount, LinesContext.TotalLinesFound, 10, 3, ',');
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &StringFormOfLineCount);
        YoriLibFreeStringContents(&StringFormOfLineCount);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
