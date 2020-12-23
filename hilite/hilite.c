/**
 * @file hilite/hilite.c
 *
 * Yori shell highlight lines or text in an input stream
 *
 * Copyright (c) 2018-2020 Malcolm J. Smith
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
CHAR strHiliteHelpText[] =
        "\n"
        "Output the contents of one or more files with highlight on lines\n"
        "or text matching specified criteria.\n"
        "\n"
        "HILITE [-license] [-b] [-c <string> <color>] [-h <string> <color>]\n"
        "       [-i] [-m] [-s] [-t <string> <color>] [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Highlight lines containing <string> with <color>\n"
        "   -h             Highlight lines starting with <string> with <color>\n"
        "   -i             Match insensitively\n"
        "   -m             Highlight matching text (as opposed to matching lines)\n"
        "   -s             Process files from all subdirectories\n"
        "   -t             Highlight lines ending with <string> with <color>\n";

/**
 Display usage text to the user.
 */
BOOL
HiliteHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Hilite %i.%02i\n"), HILITE_VER_MAJOR, HILITE_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHiliteHelpText);
    return TRUE;
}

/**
 The types of matches this program supports.
 */
typedef enum _HILITE_MATCH_TYPE {
    HiliteMatchTypeBeginsWith = 1,
    HiliteMatchTypeEndsWith = 2,
    HiliteMatchTypeContains = 3
} HILITE_MATCH_TYPE;

/**
 Context information for a specific match, including the criteria, the string
 to match against, and the color to apply if the match matches.
 */
typedef struct _HILITE_MATCH_CRITERIA {

    /**
     The list of matches to apply.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The type of comparison to apply to determine a match.
     */
    HILITE_MATCH_TYPE MatchType;

    /**
     A string to compare with to determine a match.
     */
    YORI_STRING MatchString;

    /**
     The color to apply to the line, in event of a match.
     */
    YORILIB_COLOR_ATTRIBUTES Color;
} HILITE_MATCH_CRITERIA, *PHILITE_MATCH_CRITERIA;

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _HILITE_CONTEXT {

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     TRUE if matches should be applied case insensitively, FALSE if they
     should be applied case sensitively.
     */
    BOOLEAN Insensitive;

    /**
     TRUE if any text matching the criteria should be highlighted.  FALSE if
     the entire line should be highlighted.
     */
    BOOLEAN HighlightMatchText;

    /**
     TRUE if file enumeration is being performed recursively; FALSE if it is
     in one directory only.
     */
    BOOLEAN Recursive;

    /**
     The color to apply if none of the matches match.
     */
    YORILIB_COLOR_ATTRIBUTES DefaultColor;

    /**
     A list of matches to apply against the beginning of lines.
     */
    YORI_LIST_ENTRY StartMatches;

    /**
     A list of matches to apply against the middle of lines.
     */
    YORI_LIST_ENTRY MiddleMatches;

    /**
     A list of matches to apply against the end of lines.
     */
    YORI_LIST_ENTRY EndMatches;

} HILITE_CONTEXT, *PHILITE_CONTEXT;

/**
 Return the next match, in order.  All matches that must be at the start
 of the line are returned first, then all matches in the middle of the line,
 then all matches at the end of the line.

 @param HiliteContext Pointer to the context.

 @param ListContext Pointer to a variable that on input contains the list to
        start navigating from, and on completion contains the list to navigate
        next.

 @param PreviousMatch Pointer to the previously returned match, or NULL to
        start matching from the start.

 @return Pointer to the next match.
 */
PHILITE_MATCH_CRITERIA
HiliteGetNextMatch(
    __in PHILITE_CONTEXT HiliteContext,
    __in PYORI_LIST_ENTRY *ListContext,
    __in_opt PHILITE_MATCH_CRITERIA PreviousMatch
    )
{
    PYORI_LIST_ENTRY ListHead;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIST_ENTRY PreviousEntry;
    PHILITE_MATCH_CRITERIA Match;

    ListHead = *ListContext;
    if (ListHead == NULL) {
        ListHead = &HiliteContext->StartMatches;
    }

    if (PreviousMatch != NULL) {
        PreviousEntry = &PreviousMatch->ListEntry;
    } else {
        PreviousEntry = NULL;
    }

    ListEntry = NULL;
    while(TRUE) {
        ListEntry = YoriLibGetNextListEntry(ListHead, PreviousEntry);
        if (ListEntry != NULL) {
            break;
        }

        if (ListHead == &HiliteContext->StartMatches) {
            ListHead = &HiliteContext->MiddleMatches;
        } else if (ListHead == &HiliteContext->MiddleMatches) {
            ListHead = &HiliteContext->EndMatches;
        } else {
            ListEntry = NULL;
            break;
        }

        PreviousEntry = NULL;
        *ListContext = ListHead;
    }

    if (ListEntry != NULL) {
        Match = CONTAINING_RECORD(ListEntry, HILITE_MATCH_CRITERIA, ListEntry);
        return Match;
    }

    return NULL;
}

/**
 Process a stream and apply the hilite criteria before outputting to standard
 output.

 @param hSource The incoming stream to highlight.

 @param HiliteContext Pointer to a set of criteria to apply to the stream
        before outputting.

 @return TRUE for success, FALSE on failure.
 */
BOOL
HiliteProcessStream(
    __in HANDLE hSource,
    __in PHILITE_CONTEXT HiliteContext
    )
{
    PVOID LineContext = NULL;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    YORI_STRING LineString;
    YORI_STRING Substring;
    YORI_STRING DisplayString;
    PHILITE_MATCH_CRITERIA MatchCriteria;
    YORILIB_COLOR_ATTRIBUTES ColorToUse;
    PYORI_LIST_ENTRY ListHead;
    BOOLEAN MatchFound;
    BOOLEAN AnyMatchFound;
    DWORD MatchOffset;

    YoriLibInitEmptyString(&LineString);
    YoriLibInitEmptyString(&Substring);
    YoriLibInitEmptyString(&DisplayString);
    MatchOffset = 0;

    HiliteContext->FilesFound++;

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
            break;
        }

        Substring.StartOfString = LineString.StartOfString;
        Substring.LengthInChars = LineString.LengthInChars;
        ColorToUse.Ctrl = HiliteContext->DefaultColor.Ctrl;
        ColorToUse.Win32Attr = HiliteContext->DefaultColor.Win32Attr;

        while (Substring.LengthInChars > 0) {

            //
            //  Enumerate through the matches and see if there is anything to
            //  apply.  At the start of the line, enumerate the matches to
            //  the beginning of lines; after that enumerate the matches that
            //  could be in the middle of a line.
            //

            AnyMatchFound = FALSE;
            if (Substring.StartOfString == LineString.StartOfString) {
                ListHead = &HiliteContext->StartMatches;
            } else {
                ListHead = &HiliteContext->MiddleMatches;
            }
            MatchCriteria = NULL;
            MatchCriteria = HiliteGetNextMatch(HiliteContext, &ListHead, MatchCriteria);
            while (MatchCriteria != NULL) {
                MatchFound = FALSE;
                if (MatchCriteria->MatchType == HiliteMatchTypeBeginsWith) {
                    if (HiliteContext->Insensitive) {
                        if (YoriLibCompareStringInsensitiveCount(&Substring,
                                                                 &MatchCriteria->MatchString,
                                                                 MatchCriteria->MatchString.LengthInChars) == 0) {
                            MatchFound = TRUE;
                            MatchOffset = 0;
                        }
                    } else {
                        if (YoriLibCompareStringCount(&Substring,
                                                      &MatchCriteria->MatchString,
                                                      MatchCriteria->MatchString.LengthInChars) == 0) {
                            MatchFound = TRUE;
                            MatchOffset = 0;
                        }
                    }
                } else if (MatchCriteria->MatchType == HiliteMatchTypeEndsWith) {
                    YORI_STRING TailOfLine;

                    if (Substring.LengthInChars >= MatchCriteria->MatchString.LengthInChars) {
                        YoriLibInitEmptyString(&TailOfLine);
                        TailOfLine.LengthInChars = MatchCriteria->MatchString.LengthInChars;
                        TailOfLine.StartOfString = &Substring.StartOfString[Substring.LengthInChars - MatchCriteria->MatchString.LengthInChars];

                        if (HiliteContext->Insensitive) {
                            if (YoriLibCompareStringInsensitive(&TailOfLine, &MatchCriteria->MatchString) == 0) {
                                MatchFound = TRUE;
                                MatchOffset = Substring.LengthInChars - MatchCriteria->MatchString.LengthInChars;
                            }
                        } else {
                            if (YoriLibCompareString(&TailOfLine, &MatchCriteria->MatchString) == 0) {
                                MatchFound = TRUE;
                                MatchOffset = Substring.LengthInChars - MatchCriteria->MatchString.LengthInChars;
                            }
                        }
                    }
                } else if (MatchCriteria->MatchType == HiliteMatchTypeContains) {
                    if (HiliteContext->Insensitive) {
                        if (YoriLibFindFirstMatchingSubstringInsensitive(&Substring, 1, &MatchCriteria->MatchString, &MatchOffset)) {
                            MatchFound = TRUE;
                        }
                    } else {
                        if (YoriLibFindFirstMatchingSubstring(&Substring, 1, &MatchCriteria->MatchString, &MatchOffset)) {
                            MatchFound = TRUE;
                        }
                    }
                }

                //
                //  If this is highlighting a search term only, display any
                //  text before the match in regular color, then display the
                //  match in the requested color.  If highlighting the whole
                //  line, display all the text.  Then start searching again,
                //  from all entries that can be in the middle of lines.
                //

                DisplayString.StartOfString = Substring.StartOfString;
                DisplayString.LengthInChars = Substring.LengthInChars;
                if (MatchFound) {
                    AnyMatchFound = TRUE;
                    if (HiliteContext->HighlightMatchText) {
                        if (MatchOffset > 0) {
                            DisplayString.LengthInChars = MatchOffset;
                            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                            DisplayString.StartOfString = &Substring.StartOfString[MatchOffset];
                            Substring.LengthInChars = Substring.LengthInChars - MatchOffset;
                            Substring.StartOfString = &Substring.StartOfString[MatchOffset];
                        }
                        DisplayString.LengthInChars = MatchCriteria->MatchString.LengthInChars;
                        ColorToUse.Ctrl = MatchCriteria->Color.Ctrl;
                        ColorToUse.Win32Attr = MatchCriteria->Color.Win32Attr;
                    } else {
                        ColorToUse.Ctrl = MatchCriteria->Color.Ctrl;
                        ColorToUse.Win32Attr = MatchCriteria->Color.Win32Attr;
                    }

                    YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, ColorToUse.Win32Attr);
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                    YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, HiliteContext->DefaultColor.Win32Attr);
                    Substring.StartOfString = &Substring.StartOfString[DisplayString.LengthInChars];
                    Substring.LengthInChars = Substring.LengthInChars - DisplayString.LengthInChars;
                    break;
                }
                MatchCriteria = HiliteGetNextMatch(HiliteContext, &ListHead, MatchCriteria);
            }

            //
            //  If all matches have been navigated and the string hasn't
            //  changed, no more matches were found, so move to the next line.
            //

            if (!AnyMatchFound) {
                break;
            }
        }

        //
        //  If no matches are found, display the line.
        //

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Substring);

        //
        //  Apply a newline if needed.
        //

        if (LineString.LengthInChars == 0 || !GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo) || ScreenInfo.dwCursorPosition.X != 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }
    }

    YoriLibLineReadClose(LineContext);
    YoriLibFreeStringContents(&LineString);

    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the hilite context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
HiliteFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    PHILITE_CONTEXT HiliteContext = (PHILITE_CONTEXT)Context;

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
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hilite: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        HiliteProcessStream(FileHandle, HiliteContext);

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
HiliteFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    PHILITE_CONTEXT HiliteContext = (PHILITE_CONTEXT)Context;
    BOOL Result = FALSE;

    UNREFERENCED_PARAMETER(Depth);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        if (!HiliteContext->Recursive) {
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
 Deallocate any user specified hilite criteria.

 @param HiliteContext The context from which all user specified criteria
        should be deallocated.
 */
VOID
HiliteCleanupContext(
    __in PHILITE_CONTEXT HiliteContext
    )
{
    PHILITE_MATCH_CRITERIA MatchCriteria;
    PHILITE_MATCH_CRITERIA NextMatchCriteria;
    PYORI_LIST_ENTRY ListHead;

    ListHead = &HiliteContext->StartMatches;

    MatchCriteria = HiliteGetNextMatch(HiliteContext, &ListHead, NULL);
    while (MatchCriteria != NULL) {
        NextMatchCriteria = HiliteGetNextMatch(HiliteContext, &ListHead, MatchCriteria);
        YoriLibRemoveListItem(&MatchCriteria->ListEntry);
        YoriLibFree(MatchCriteria);
        MatchCriteria = NextMatchCriteria;
    }
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the hilite builtin command.
 */
#define ENTRYPOINT YoriCmd_HILITE
#else
/**
 The main entrypoint for the hilite standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the hilite cmdlet.

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
    HILITE_CONTEXT HiliteContext;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    PHILITE_MATCH_CRITERIA NewCriteria;
    YORI_STRING Arg;

    ZeroMemory(&HiliteContext, sizeof(HiliteContext));

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo)) {
        HiliteContext.DefaultColor.Ctrl = 0;
        HiliteContext.DefaultColor.Win32Attr = (UCHAR)ScreenInfo.wAttributes;
    } else {
        HiliteContext.DefaultColor.Ctrl = 0;
        HiliteContext.DefaultColor.Win32Attr = 0x7;
    }

    YoriLibInitializeListHead(&HiliteContext.StartMatches);
    YoriLibInitializeListHead(&HiliteContext.MiddleMatches);
    YoriLibInitializeListHead(&HiliteContext.EndMatches);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                HiliteHelp();
                HiliteCleanupContext(&HiliteContext);
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2020"));
                HiliteCleanupContext(&HiliteContext);
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (i + 2 < ArgC) {
                    NewCriteria = YoriLibMalloc(sizeof(HILITE_MATCH_CRITERIA));
                    if (NewCriteria == NULL) {
                        HiliteCleanupContext(&HiliteContext);
                        return EXIT_FAILURE;
                    }
                    NewCriteria->MatchType = HiliteMatchTypeContains;
                    YoriLibInitEmptyString(&NewCriteria->MatchString);
                    NewCriteria->MatchString.StartOfString = ArgV[i + 1].StartOfString;
                    NewCriteria->MatchString.LengthInChars = ArgV[i + 1].LengthInChars;
                    YoriLibAttributeFromLiteralString(ArgV[i + 2].StartOfString, &NewCriteria->Color);
                    YoriLibResolveWindowColorComponents(NewCriteria->Color, HiliteContext.DefaultColor, FALSE, &NewCriteria->Color);
                    YoriLibAppendList(&HiliteContext.MiddleMatches, &NewCriteria->ListEntry);
                    ArgumentUnderstood = TRUE;
                    i += 2;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                if (i + 2 < ArgC) {
                    NewCriteria = YoriLibMalloc(sizeof(HILITE_MATCH_CRITERIA));
                    if (NewCriteria == NULL) {
                        HiliteCleanupContext(&HiliteContext);
                        return EXIT_FAILURE;
                    }
                    NewCriteria->MatchType = HiliteMatchTypeBeginsWith;
                    YoriLibInitEmptyString(&NewCriteria->MatchString);
                    NewCriteria->MatchString.StartOfString = ArgV[i + 1].StartOfString;
                    NewCriteria->MatchString.LengthInChars = ArgV[i + 1].LengthInChars;
                    YoriLibAttributeFromLiteralString(ArgV[i + 2].StartOfString, &NewCriteria->Color);
                    YoriLibResolveWindowColorComponents(NewCriteria->Color, HiliteContext.DefaultColor, FALSE, &NewCriteria->Color);
                    YoriLibAppendList(&HiliteContext.StartMatches, &NewCriteria->ListEntry);
                    ArgumentUnderstood = TRUE;
                    i += 2;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                HiliteContext.Insensitive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("m")) == 0) {
                HiliteContext.HighlightMatchText = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                HiliteContext.Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                if (i + 2 < ArgC) {
                    NewCriteria = YoriLibMalloc(sizeof(HILITE_MATCH_CRITERIA));
                    if (NewCriteria == NULL) {
                        HiliteCleanupContext(&HiliteContext);
                        return EXIT_FAILURE;
                    }
                    NewCriteria->MatchType = HiliteMatchTypeEndsWith;
                    YoriLibInitEmptyString(&NewCriteria->MatchString);
                    NewCriteria->MatchString.StartOfString = ArgV[i + 1].StartOfString;
                    NewCriteria->MatchString.LengthInChars = ArgV[i + 1].LengthInChars;
                    YoriLibAttributeFromLiteralString(ArgV[i + 2].StartOfString, &NewCriteria->Color);
                    YoriLibResolveWindowColorComponents(NewCriteria->Color, HiliteContext.DefaultColor, FALSE, &NewCriteria->Color);
                    YoriLibAppendList(&HiliteContext.EndMatches, &NewCriteria->ListEntry);
                    ArgumentUnderstood = TRUE;
                    i += 2;
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
            HiliteCleanupContext(&HiliteContext);
            return EXIT_FAILURE;
        }

        HiliteProcessStream(GetStdHandle(STD_INPUT_HANDLE), &HiliteContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (HiliteContext.Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }

        for (i = StartArg; i < ArgC; i++) {

            YoriLibForEachStream(&ArgV[i],
                                 MatchFlags,
                                 0,
                                 HiliteFileFoundCallback,
                                 HiliteFileEnumerateErrorCallback,
                                 &HiliteContext);
        }
    }

    HiliteCleanupContext(&HiliteContext);

    if (HiliteContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hilite: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
