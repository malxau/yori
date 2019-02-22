/**
 * @file repl/repl.c
 *
 * Yori shell replace text with other text on an input stream
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
CHAR strReplHelpText[] =
        "\n"
        "Output the contents of one or more files with specified text replaced\n"
        "with alternate text.\n"
        "\n"
        "REPL [-license] [-b] [-i] [-s] <old text> <new text> [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -i             Match insensitively\n"
        "   -s             Process files from all subdirectories\n";

/**
 Display usage text to the user.
 */
BOOL
ReplHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Repl %i.%02i\n"), REPL_VER_MAJOR, REPL_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strReplHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _REPL_CONTEXT {

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     TRUE if matches should be applied case insensitively, FALSE if they
     should be applied case sensitively.
     */
    BOOL Insensitive;

    /**
     TRUE to indicate that files are being enumerated recursively.
     */
    BOOL Recursive;

    /**
     A string to compare with to determine a match.
     */
    PYORI_STRING MatchString;

    /**
     A string to replace the match with.
     */
    PYORI_STRING NewString;

} REPL_CONTEXT, *PREPL_CONTEXT;

/**
 Process a stream and apply the repl criteria before outputting to standard
 output.

 @param hSource The incoming stream to highlight.

 @param ReplContext Pointer to a set of criteria to apply to the stream
        before outputting.

 @return TRUE for success, FALSE on failure.
 */
BOOL
ReplProcessStream(
    __in HANDLE hSource,
    __in PREPL_CONTEXT ReplContext
    )
{
    PVOID LineContext = NULL;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    YORI_STRING LineString;
    YORI_STRING AlternateStrings[2];
    YORI_STRING InitialPortion;
    YORI_STRING TrailingPortion;
    PYORI_STRING SourceString;
    DWORD SearchOffset;
    YORI_STRING SearchSubset;
    DWORD MatchOffset;
    DWORD NextAlternate;
    DWORD LengthRequired;

    YoriLibInitEmptyString(&LineString);
    YoriLibInitEmptyString(&AlternateStrings[0]);
    YoriLibInitEmptyString(&AlternateStrings[1]);

    ReplContext->FilesFound++;

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
            break;
        }

        //
        //  Loop through the line, finding all occurrences of the MatchString
        //  and replacing them with NewString
        //

        SourceString = &LineString;
        SearchOffset = 0;
        NextAlternate = 0;
        while(TRUE) {

            //
            //  Continue searching after any previous replacements
            //

            YoriLibInitEmptyString(&SearchSubset);
            SearchSubset.StartOfString = &SourceString->StartOfString[SearchOffset];
            SearchSubset.LengthInChars = SourceString->LengthInChars - SearchOffset;

            //
            //  If no match is found, the line processing is complete
            //

            if (ReplContext->Insensitive) {
                if (YoriLibFindFirstMatchingSubstringInsensitive(&SearchSubset, 1, ReplContext->MatchString, &MatchOffset) == NULL) {
                    break;
                }
            } else {
                if (YoriLibFindFirstMatchingSubstring(&SearchSubset, 1, ReplContext->MatchString, &MatchOffset) == NULL) {
                    break;
                }
            }

            //
            //  If a match is found, a new line needs to be assembled
            //  consisting of characters already processed and not searched
            //  for, characters before any match was found, the new string,
            //  and any characters following the match.
            //

            LengthRequired = SearchOffset + SearchSubset.LengthInChars + ReplContext->NewString->LengthInChars - ReplContext->MatchString->LengthInChars + 1;

            if (LengthRequired > AlternateStrings[NextAlternate].LengthAllocated) {
                YoriLibFreeStringContents(&AlternateStrings[NextAlternate]);
                if (!YoriLibAllocateString(&AlternateStrings[NextAlternate], LengthRequired + 256)) {
                    break;
                }
            }

            YoriLibInitEmptyString(&InitialPortion);
            InitialPortion.StartOfString = SourceString->StartOfString;
            InitialPortion.LengthInChars = SearchOffset + MatchOffset;

            YoriLibInitEmptyString(&TrailingPortion);
            TrailingPortion.StartOfString = &SourceString->StartOfString[SearchOffset + MatchOffset + ReplContext->MatchString->LengthInChars];
            TrailingPortion.LengthInChars = SourceString->LengthInChars - SearchOffset - MatchOffset - ReplContext->MatchString->LengthInChars;

            AlternateStrings[NextAlternate].LengthInChars = YoriLibSPrintf(AlternateStrings[NextAlternate].StartOfString, _T("%y%y%y"), &InitialPortion, ReplContext->NewString, &TrailingPortion);

            //
            //  Continue searching from the newly assembled string after
            //  the point of any substitutions.
            //

            SourceString = &AlternateStrings[NextAlternate];
            SearchOffset += MatchOffset + ReplContext->NewString->LengthInChars;
            NextAlternate = (NextAlternate + 1) % 2;
        }

        //
        //  Output the line.
        //

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), SourceString);
        if (SourceString->LengthInChars == 0 || !GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo) || ScreenInfo.dwCursorPosition.X != 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }
    }

    YoriLibLineReadClose(LineContext);
    YoriLibFreeStringContents(&LineString);
    YoriLibFreeStringContents(&AlternateStrings[0]);
    YoriLibFreeStringContents(&AlternateStrings[1]);

    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the repl context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
ReplFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    PREPL_CONTEXT ReplContext = (PREPL_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

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
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("repl: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        ReplProcessStream(FileHandle, ReplContext);

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
ReplFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;
    PREPL_CONTEXT ReplContext = (PREPL_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        if (!ReplContext->Recursive) {
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
 The main entrypoint for the repl builtin command.
 */
#define ENTRYPOINT YoriCmd_REPL
#else
/**
 The main entrypoint for the repl standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the repl cmdlet.

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
    REPL_CONTEXT ReplContext;
    YORI_STRING Arg;

    ZeroMemory(&ReplContext, sizeof(ReplContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ReplHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                ReplContext.Insensitive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                ReplContext.Recursive = TRUE;
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

    if (StartArg == 0 || StartArg + 2 >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("repl: missing argument\n"));
        return EXIT_FAILURE;
    }

    ReplContext.MatchString = &ArgV[StartArg];
    ReplContext.NewString = &ArgV[StartArg + 1];
    StartArg += 2;

#if YORI_BUILTIN
    YoriLibCancelEnable();
#endif

    if (StartArg == 0) {
        DWORD FileType = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
        FileType = FileType & ~(FILE_TYPE_REMOTE);
        if (FileType == FILE_TYPE_CHAR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
            return EXIT_FAILURE;
        }

        ReplProcessStream(GetStdHandle(STD_INPUT_HANDLE), &ReplContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (ReplContext.Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }
    
        for (i = StartArg; i < ArgC; i++) {
    
            YoriLibForEachFile(&ArgV[i],
                               MatchFlags,
                               0,
                               ReplFileFoundCallback,
                               ReplFileEnumerateErrorCallback,
                               &ReplContext);
        }
    }

    if (ReplContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("repl: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
