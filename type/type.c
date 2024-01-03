/**
 * @file type/type.c
 *
 * Yori shell display file contents
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
CHAR strTypeHelpText[] =
        "\n"
        "Output the contents of one or more files.\n"
        "\n"
        "TYPE [-license] [-b] [-s] [-h <num>] [-n] [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -h <num>       Display <num> lines from the beginning of each file\n"
        "   -n             Display line numbers\n"
        "   -s             Process files from all subdirectories\n";

/**
 Display usage text to the user.
 */
BOOL
TypeHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Type %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strTypeHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _TYPE_CONTEXT {

    /**
     TRUE to indicate that files are being enumerated recursively.
     */
    BOOLEAN Recursive;

    /**
     TRUE to indicate that line numbers should be displayed.
     */
    BOOLEAN DisplayLineNumbers;

    /**
     The first error encountered when enumerating objects from a single arg.
     This is used to preserve file not found/path not found errors so that
     when the program falls back to interpreting the argument as a literal,
     if that still doesn't work, this is the error code that is displayed.
     */
    DWORD SavedErrorThisArg;

    /**
     Specifies the number of lines from the top of each file to display.
     */
    DWORD HeadLines;

    /**
     Records the total number of files processed.
     */
    DWORDLONG FilesFound;

    /**
     Records the total number of files processed for this command line
     argument.
     */
    DWORDLONG FilesFoundThisArg;

    /**
     Records the number of lines found from a specific file.
     */
    DWORDLONG FileLinesFound;

} TYPE_CONTEXT, *PTYPE_CONTEXT;

/**
 Process a single opened stream, enumerating through all lines and displaying
 the set requested by the user.

 @param hSource The opened source stream.

 @param TypeContext Pointer to context information specifying which lines to
        display.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
TypeProcessStream(
    __in HANDLE hSource,
    __in PTYPE_CONTEXT TypeContext
    )
{
    PVOID LineContext = NULL;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    YORI_STRING LineString;
    BOOL OutputIsConsole;
    DWORD dwMode;
    DWORD CharactersDisplayed;
    HANDLE OutputHandle;

    OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    YoriLibInitEmptyString(&LineString);

    TypeContext->FilesFound++;
    TypeContext->FilesFoundThisArg++;
    TypeContext->FileLinesFound = 0;

    OutputIsConsole = FALSE;
    if (GetConsoleMode(OutputHandle, &dwMode)) {
        OutputIsConsole = TRUE;
    }

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
            break;
        }

        TypeContext->FileLinesFound++;

        if ((TypeContext->HeadLines == 0 || TypeContext->FileLinesFound <= TypeContext->HeadLines)) {
            if (TypeContext->DisplayLineNumbers) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%8lli: %y"), TypeContext->FileLinesFound, &LineString);
                CharactersDisplayed = LineString.LengthInChars + 10;
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &LineString);
                CharactersDisplayed = LineString.LengthInChars;
            }
            if (CharactersDisplayed == 0 ||
                !OutputIsConsole ||
                !GetConsoleScreenBufferInfo(OutputHandle, &ScreenInfo) ||
                ScreenInfo.dwCursorPosition.X != 0) {

                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
            }
        } else {
            break;
        }
    }

    YoriLibLineReadCloseOrCache(LineContext);
    YoriLibFreeStringContents(&LineString);

    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.  This can be NULL if the file
        was not found by enumeration.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param Context Pointer to the type context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
TypeFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    PTYPE_CONTEXT TypeContext = (PTYPE_CONTEXT)Context;

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
            if (TypeContext->SavedErrorThisArg == ERROR_SUCCESS) {
                DWORD LastError = GetLastError();
                LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("type: open of %y failed: %s"), FilePath, ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
            return TRUE;
        }

        TypeContext->SavedErrorThisArg = ERROR_SUCCESS;
        TypeProcessStream(FileHandle, TypeContext);

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
TypeFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;
    PTYPE_CONTEXT TypeContext = (PTYPE_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        if (!TypeContext->Recursive) {
            TypeContext->SavedErrorThisArg = ErrorCode;
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
            DirName.LengthInChars = (YORI_ALLOC_SIZE_T)(FilePart - DirName.StartOfString);
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
 The main entrypoint for the type builtin command.
 */
#define ENTRYPOINT YoriCmd_YTYPE
#else
/**
 The main entrypoint for the type standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the type cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero on success, nonzero on failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    WORD MatchFlags;
    BOOLEAN BasicEnumeration = FALSE;
    TYPE_CONTEXT TypeContext;
    YORI_STRING Arg;

    ZeroMemory(&TypeContext, sizeof(TypeContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                TypeHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                if (ArgC > i + 1) {
                    YORI_ALLOC_SIZE_T CharsConsumed;
                    LONGLONG HeadLines = 0;
                    if (!YoriLibStringToNumber(&ArgV[i + 1], TRUE, &HeadLines, &CharsConsumed) ||
                         CharsConsumed == 0) {

                        //
                        //  If it's not numeric, assume it's a file name.
                        //  Default to 10 lines, and don't advance the
                        //  argument.
                        //

                        TypeContext.HeadLines = 10;
                        ArgumentUnderstood = TRUE;

                    } else {

                        //
                        //  Hack to allow "head -50" to mean "positive 50"
                        //

                        if (HeadLines < 0) {
                            HeadLines = HeadLines * -1;
                        }
                        TypeContext.HeadLines = (ULONG)HeadLines;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                TypeContext.DisplayLineNumbers = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                TypeContext.Recursive = TRUE;
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
    YoriLibCancelEnable(FALSE);
#endif

    //
    //  Attempt to enable backup privilege so an administrator can access more
    //  objects successfully.
    //

    YoriLibEnableBackupPrivilege();

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0 || StartArg == ArgC) {
        if (YoriLibIsStdInConsole()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
            return EXIT_FAILURE;
        }

        TypeProcessStream(GetStdHandle(STD_INPUT_HANDLE), &TypeContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (TypeContext.Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }

        for (i = StartArg; i < ArgC; i++) {

            TypeContext.FilesFoundThisArg = 0;
            TypeContext.SavedErrorThisArg = ERROR_SUCCESS;

            YoriLibForEachStream(&ArgV[i],
                                 MatchFlags,
                                 0,
                                 TypeFileFoundCallback,
                                 TypeFileEnumerateErrorCallback,
                                 &TypeContext);

            if (TypeContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    TypeFileFoundCallback(&FullPath, NULL, 0, &TypeContext);
                    YoriLibFreeStringContents(&FullPath);
                }
                if (TypeContext.SavedErrorThisArg != ERROR_SUCCESS) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File or directory not found: %y\n"), &ArgV[i]);
                }
            }
        }
    }

#if !YORI_BUILTIN
    YoriLibLineReadCleanupCache();
#endif

    if (TypeContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("type: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
