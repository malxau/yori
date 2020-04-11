/**
 * @file iconv/iconv.c
 *
 * Yori character encoding conversions
 *
 * Copyright (c) 2017-2020 Malcolm J. Smith
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
CHAR strIconvHelpText[] =
        "\n"
        "Convert the character encoding of one or more files.\n"
        "\n"
        "ICONV [-license] [-b] [-s] [-e <encoding>] [-i <encoding>] [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -e <encoding>  Specifies the new encoding to use\n"
        "   -i <encoding>  Specifies the input (current) encoding\n"
        "   -m             Use traditional Mac line endings (CR)\n"
        "   -s             Process files from all subdirectories\n"
        "   -u             Use Unix line endings (LF)\n"
        "   -w             Use Windows line endings (CRLF)\n";

/**
 Display usage text to the user.
 */
BOOL
IconvHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Iconv %i.%02i\n"), ICONV_VER_MAJOR, ICONV_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strIconvHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _ICONV_CONTEXT {

    /**
     TRUE if file enumeration is being performed recursively; FALSE if it is
     in one directory only.
     */
    BOOL Recursive;

    /**
     The encoding to use when reading data.
     */
    DWORD SourceEncoding;

    /**
     The encoding to use when outputting data.
     */
    DWORD TargetEncoding;

    /**
     The line ending to use when outputting data.
     */
    LPTSTR LineEnding;

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

} ICONV_CONTEXT, *PICONV_CONTEXT;

/**
 Convert the encoding of an opened stream by reading the source with the
 requested encoding, then writing to the destination with the requested
 encoding.

 @param hSource Handle to the source.

 @param IconvContext Specifies the encodings to apply.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
IconvProcessStream(
    __in HANDLE hSource,
    __in PICONV_CONTEXT IconvContext
    )
{
    PVOID LineContext = NULL;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    YORI_STRING LineString;
    DWORD OriginalInputEncoding;
    DWORD OriginalOutputEncoding;
    LPTSTR OriginalLineEnding;

    IconvContext->FilesFound++;

    OriginalInputEncoding = YoriLibGetMultibyteInputEncoding();
    OriginalOutputEncoding = YoriLibGetMultibyteOutputEncoding();
    OriginalLineEnding = YoriLibVtGetLineEnding();

    YoriLibSetMultibyteInputEncoding(IconvContext->SourceEncoding);
    YoriLibSetMultibyteOutputEncoding(IconvContext->TargetEncoding);
    YoriLibVtSetLineEnding(IconvContext->LineEnding);

    YoriLibInitEmptyString(&LineString);

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
            break;
        }

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &LineString);
        if (LineString.LengthInChars == 0 || !GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo) || ScreenInfo.dwCursorPosition.X != 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }
    }

    YoriLibSetMultibyteInputEncoding(OriginalInputEncoding);
    YoriLibSetMultibyteOutputEncoding(OriginalOutputEncoding);
    YoriLibVtSetLineEnding(OriginalLineEnding);

    YoriLibLineReadClose(LineContext);
    YoriLibFreeStringContents(&LineString);

    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth The depth level.  Ignored in this application.

 @param Context Pointer to the type context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
IconvFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    PICONV_CONTEXT IconvContext = (PICONV_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        FileHandle = CreateFile(FilePath->StartOfString,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);

        if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("iconv: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        IconvProcessStream(FileHandle, IconvContext);

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
IconvFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;
    PICONV_CONTEXT IconvContext = (PICONV_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        if (!IconvContext->Recursive) {
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


/**
 Parse a user specified argument into an encoding identifier.

 @param String The string to parse.

 @return The encoding identifier.  -1 is used to indicate failure.
 */
DWORD
IconvEncodingFromString(
    __in PYORI_STRING String
    )
{
    if (YoriLibCompareStringWithLiteralInsensitive(String, _T("utf8")) == 0) {
        return CP_UTF8;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("ascii")) == 0) {
        return CP_OEMCP;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("ansi")) == 0) {
        return CP_ACP;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("utf16")) == 0) {
        return CP_UTF16;
    }
    return (DWORD)-1;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the iconv builtin command.
 */
#define ENTRYPOINT YoriCmd_ICONV
#else
/**
 The main entrypoint for the iconv standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the type cmdlet.

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
    ICONV_CONTEXT IconvContext;
    YORI_STRING Arg;

    ZeroMemory(&IconvContext, sizeof(IconvContext));
    IconvContext.SourceEncoding = CP_UTF8;
    IconvContext.TargetEncoding = CP_UTF8;
    IconvContext.LineEnding = _T("\r\n");

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                IconvHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2020"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                if (ArgC > i + 1) {
                    DWORD NewEncoding;
                    NewEncoding = IconvEncodingFromString(&ArgV[i + 1]);
                    if (NewEncoding != (DWORD)-1) {
                        IconvContext.TargetEncoding = NewEncoding;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                if (ArgC > i + 1) {
                    DWORD NewEncoding;
                    NewEncoding = IconvEncodingFromString(&ArgV[i + 1]);
                    if (NewEncoding != (DWORD)-1) {
                        IconvContext.SourceEncoding = NewEncoding;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("m")) == 0) {
                IconvContext.LineEnding = _T("\r");
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("u")) == 0) {
                IconvContext.LineEnding = _T("\n");
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                IconvContext.Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("w")) == 0) {
                IconvContext.LineEnding = _T("\r\n");
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

        IconvProcessStream(GetStdHandle(STD_INPUT_HANDLE), &IconvContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (IconvContext.Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }
    
        for (i = StartArg; i < ArgC; i++) {
    
            YoriLibForEachStream(&ArgV[i],
                                 MatchFlags,
                                 0,
                                 IconvFileFoundCallback,
                                 IconvFileEnumerateErrorCallback,
                                 &IconvContext);
        }
    }

    if (IconvContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("iconv: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
