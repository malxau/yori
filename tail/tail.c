/**
 * @file tail/tail.c
 *
 * Yori shell display the final lines in a file
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
TailHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Tail %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
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
    DWORDLONG FilesFound;

    /**
     Records the total number of files processed within a single command line
     argument.
     */
    DWORDLONG FilesFoundThisArg;

    /**
     Specifies the number of lines to display in each matching file.
     */
    DWORD LinesToDisplay;

    /**
     The first error encountered when enumerating objects from a single arg.
     This is used to preserve file not found/path not found errors so that
     when the program falls back to interpreting the argument as a literal,
     if that still doesn't work, this is the error code that is displayed.
     */
    DWORD SavedErrorThisArg;

    /**
     If nonzero, specifies the final line to display from each file.  This
     implies that tail is running in context mode, looking for a region in
     the middle of the file.
     */
    DWORDLONG FinalLine;

    /**
     Specifies the number of lines that have been found from the current
     stream.
     */
    DWORDLONG LinesFound;

    /**
     An array of LinesToDisplay YORI_STRING structures.
     */
    PYORI_STRING LinesArray;

    /**
     If TRUE, continue outputting results as more arrive.  If FALSE, terminate
     as soon as the requested lines have been output.
     */
    BOOLEAN WaitForMore;

    /**
     TRUE to indicate that files are being enumerated recursively.
     */
    BOOLEAN Recursive;

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
    DWORDLONG StartLine = 0;
    DWORDLONG CurrentLine;
    PYORI_STRING LineString;
    YORI_LIB_LINE_ENDING LineEnding;
    BOOL TimeoutReached;
    DWORD SeekToEndOffset = 0;
    DWORD Err;
    DWORD BytesWritten;

    DWORD FileType = GetFileType(hSource);
    FileType = FileType & ~(FILE_TYPE_REMOTE);

    //
    //  If it's a file and we want the final few lines, start searching
    //  from the end, assuming an average line size of 256 bytes.
    //

    if (FileType == FILE_TYPE_DISK && TailContext->FinalLine == 0) {
        SeekToEndOffset = 256 * TailContext->LinesToDisplay;
    }

    TailContext->FilesFound++;
    TailContext->FilesFoundThisArg++;

    while (TRUE) {

        if (SeekToEndOffset != 0) {
            if (!SetFilePointer(hSource, -1 * SeekToEndOffset, NULL, FILE_END)) {
                SeekToEndOffset = 0;
                SetFilePointer(hSource, 0, NULL, FILE_BEGIN);
            }
        }
        TailContext->LinesFound = 0;

        while (TRUE) {

            if (!YoriLibReadLineToStringEx(&TailContext->LinesArray[TailContext->LinesFound % TailContext->LinesToDisplay],
                                           &LineContext,
                                           !TailContext->WaitForMore,
                                           INFINITE,
                                           hSource,
                                           &LineEnding,
                                           &TimeoutReached)) {
                break;
            }

            TailContext->LinesFound++;

            if (TailContext->FinalLine != 0 && TailContext->LinesFound >= TailContext->FinalLine) {
                break;
            }
        }

        if (TailContext->LinesFound > TailContext->LinesToDisplay) {
            StartLine = TailContext->LinesFound - TailContext->LinesToDisplay;
            break;
        } else if (SeekToEndOffset != 0) {

            //
            //  If we didn't get enough lines and we have a file that
            //  supports arbitrary seeks, try to grab more data.  If
            //  we've already hit our arbitrary maximum (a 4Kb average
            //  line size) start scanning from the top.
            //

            if (SeekToEndOffset < 4096 * TailContext->LinesToDisplay) {
                SeekToEndOffset = 4096 * TailContext->LinesToDisplay;
            } else {
                SeekToEndOffset = 0;
                SetFilePointer(hSource, 0, NULL, FILE_BEGIN);
            }
            YoriLibLineReadCloseOrCache(LineContext);
            LineContext = NULL;
            continue;
        } else {
            StartLine = 0;
            break;
        }
    }

    for (CurrentLine = StartLine; CurrentLine < TailContext->LinesFound; CurrentLine++) {
        LineString = &TailContext->LinesArray[CurrentLine % TailContext->LinesToDisplay];
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), LineString);
    }

    if (TailContext->WaitForMore) {
        while (TRUE) {

            if (!YoriLibReadLineToStringEx(&TailContext->LinesArray[0], &LineContext, FALSE, INFINITE, hSource, &LineEnding, &TimeoutReached)) {

                //
                //  Check if the target handle is still around
                //

                if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), NULL, 0, &BytesWritten, NULL)) {
                    Err = GetLastError();
                    if (Err == ERROR_NO_DATA ||
                        Err == ERROR_PIPE_NOT_CONNECTED) {
                        break;
                    }
                }

                if (YoriLibIsOperationCancelled()) {
                    break;
                }

                Sleep(200);
                continue;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &TailContext->LinesArray[0]);
        }
    }

    YoriLibLineReadCloseOrCache(LineContext);
    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.  This can be NULL if the file
        was not found by enumeration.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param Context Pointer to the tail context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
TailFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    PTAIL_CONTEXT TailContext = (PTAIL_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    if (FileInfo == NULL ||
        (FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

        FileHandle = CreateFile(FilePath->StartOfString,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);

        if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
            if (TailContext->SavedErrorThisArg == ERROR_SUCCESS) {
                DWORD LastError = GetLastError();
                LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tail: open of %y failed: %s"), FilePath, ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
            return TRUE;
        }

        TailContext->SavedErrorThisArg = ERROR_SUCCESS;
        TailProcessStream(FileHandle, TailContext);

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
TailFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;
    PTAIL_CONTEXT TailContext = (PTAIL_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        if (!TailContext->Recursive) {
            TailContext->SavedErrorThisArg = ErrorCode;
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
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    DWORD CharsConsumed;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &ContextLine, &CharsConsumed) &&
                        CharsConsumed > 0)  {

                        ArgumentUnderstood = TRUE;
                        i++;
                    } else {
                        ContextLine = -1;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                TailContext.WaitForMore = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                if (ArgC > i + 1) {
                    LONGLONG LineCount;
                    DWORD CharsConsumed;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &LineCount, &CharsConsumed) &&
                        LineCount != 0 && LineCount < 1 * 1024 * 1024) {

                        TailContext.LinesToDisplay = (DWORD)LineCount;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                TailContext.Recursive = TRUE;
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

    if (ContextLine != -1) {
        TailContext.FinalLine = ContextLine + TailContext.LinesToDisplay / 2;
    }

    TailContext.LinesArray = YoriLibMalloc(TailContext.LinesToDisplay * sizeof(YORI_STRING));
    if (TailContext.LinesArray == NULL) {
        return EXIT_FAILURE;
    }

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    //
    //  Attempt to enable backup privilege so an administrator can access more
    //  objects successfully.
    //

    YoriLibEnableBackupPrivilege();

    for (Count = 0; Count < TailContext.LinesToDisplay; Count++) {
        YoriLibInitEmptyString(&TailContext.LinesArray[Count]);
    }

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0 || StartArg == ArgC) {
        if (YoriLibIsStdInConsole()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
            YoriLibFree(TailContext.LinesArray);
            return EXIT_FAILURE;
        }

        TailProcessStream(GetStdHandle(STD_INPUT_HANDLE), &TailContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (TailContext.Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }

        for (i = StartArg; i < ArgC; i++) {

            TailContext.FilesFoundThisArg = 0;
            TailContext.SavedErrorThisArg = ERROR_SUCCESS;

            YoriLibForEachStream(&ArgV[i],
                                 MatchFlags,
                                 0,
                                 TailFileFoundCallback,
                                 TailFileEnumerateErrorCallback,
                                 &TailContext);

            if (TailContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    TailFileFoundCallback(&FullPath, NULL, 0, &TailContext);
                    YoriLibFreeStringContents(&FullPath);
                }

                if (TailContext.SavedErrorThisArg != ERROR_SUCCESS) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File or directory not found: %y\n"), &ArgV[i]);
                }
            }
        }
    }

    for (Count = 0; Count < TailContext.LinesToDisplay; Count++) {
        YoriLibFreeStringContents(&TailContext.LinesArray[Count]);
    }
    YoriLibFree(TailContext.LinesArray);

#if !YORI_BUILTIN
    YoriLibLineReadCleanupCache();
#endif

    if (TailContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tail: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
