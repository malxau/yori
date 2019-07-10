/**
 * @file lines/lines.c
 *
 * Yori shell display count of lines in files
 *
 * Copyright (c) 2017-2019 Malcolm J. Smith
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
CHAR strLinesHelpText[] =
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
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Lines %i.%02i\n"), LINES_VER_MAJOR, LINES_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strLinesHelpText);
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
     TRUE to indicate that files are being enumerated recursively.
     */
    BOOL Recursive;

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     Records the total number of files processed within a single command line
     argument.
     */
    LONGLONG FilesFoundThisArg;

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
    LinesContext->FilesFoundThisArg++;
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

 @param FileInfo Information about the file.  This can be NULL if the file
        was not found by enumeration.

 @param Depth Specifies the recursion depth.  Ignored in this application.

 @param Context Pointer to the lines context structure indicating the action
        to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
LinesFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    PLINES_CONTEXT LinesContext = (PLINES_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    if (FileInfo == NULL ||
        (FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

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
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the context block indicating whether the
        enumeration was recursive.  Recursive enumerates do not complain
        if a matching file is not in every single directory, because
        common usage expects files to be in a subset of directories only.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
LinesFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;
    PLINES_CONTEXT LinesContext = (PLINES_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        if (!LinesContext->Recursive) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File or directory not found: %y\n"), &UnescapedFilePath);
        }
        Result = TRUE;
    } else {
        LPTSTR ErrText = YoriLibGetWinErrorText(ErrorCode);
        YORI_STRING DirName;
        LPTSTR FilePart;
        YoriLibInitEmptyString(&DirName);
        DirName.StartOfString = UnescapedFilePath.StartOfString;
        FilePart = YoriLibFindRightMostCharacter(&UnescapedFilePath, '\\');
        if (FilePart != NULL) {
            DirName.LengthInChars = (DWORD)(FilePart - DirName.StartOfString);
        } else {
            DirName.LengthInChars = UnescapedFilePath.LengthInChars;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Enumerate of %y failed: %s"), &DirName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
    }
    YoriLibFreeStringContents(&UnescapedFilePath);
    return Result;
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the lines builtin command.
 */
#define ENTRYPOINT YoriCmd_LINES
#else
/**
 The main entrypoint for the lines standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the lines cmdlet.

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
    DWORD StartArg = 0;
    DWORD MatchFlags;
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
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                LinesContext.Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                LinesContext.SummaryOnly = TRUE;
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

#if YORI_BUILTIN
    YoriLibCancelEnable();
#endif

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0 || StartArg == ArgC) {
        if (YoriLibIsStdInConsole()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
            return EXIT_FAILURE;
        }

        LinesContext.SummaryOnly = TRUE;

        LinesProcessStream(GetStdHandle(STD_INPUT_HANDLE), &LinesContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (LinesContext.Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }
    
        for (i = StartArg; i < ArgC; i++) {

            LinesContext.FilesFoundThisArg = 0;
    
            YoriLibForEachStream(&ArgV[i],
                                 MatchFlags,
                                 0,
                                 LinesFileFoundCallback,
                                 LinesFileEnumerateErrorCallback,
                                 &LinesContext);

            if (LinesContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    LinesFileFoundCallback(&FullPath, NULL, 0, &LinesContext);
                    YoriLibFreeStringContents(&FullPath);
                }
            }
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
