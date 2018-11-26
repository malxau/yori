/**
 * @file hilite/hilite.c
 *
 * Yori shell highlight lines in an input stream
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
CHAR strHiliteHelpText[] =
        "\n"
        "Output the contents of one or more files with highlight on lines\n"
        "matching specified criteria.\n"
        "\n"
        "HILITE [-license] [-b] [-c <string> <color>] [-h <string> <color>]\n"
        "       [-i] [-s] [-t <string> <color>] [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Highlight lines containing <string> with <color>\n"
        "   -h             Highlight lines starting with <string> with <color>\n"
        "   -i             Match insensitively\n"
        "   -s             Process files from all subdirectories\n"
        "   -t             Highlight lines ending with <string> with <color>\n";

/**
 Display usage text to the user.
 */
BOOL
HiliteHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Hilite %i.%i\n"), HILITE_VER_MAJOR, HILITE_VER_MINOR);
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
    BOOL Insensitive;

    /**
     The color to apply if none of the matches match.
     */
    YORILIB_COLOR_ATTRIBUTES DefaultColor;

    /**
     A list of matches to apply against the stream.
     */
    YORI_LIST_ENTRY Matches;

} HILITE_CONTEXT, *PHILITE_CONTEXT;

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
    PHILITE_MATCH_CRITERIA MatchCriteria;
    YORILIB_COLOR_ATTRIBUTES ColorToUse;
    PYORI_LIST_ENTRY ListEntry;

    YoriLibInitEmptyString(&LineString);

    HiliteContext->FilesFound++;

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, TRUE, hSource)) {
            break;
        }

        ColorToUse = HiliteContext->DefaultColor;

        //
        //  Enumerate through the matches and see if there is anything to
        //  apply.
        //

        ListEntry = YoriLibGetNextListEntry(&HiliteContext->Matches, NULL);
        while (ListEntry != NULL) {
            MatchCriteria = CONTAINING_RECORD(ListEntry, HILITE_MATCH_CRITERIA, ListEntry);
            if (MatchCriteria->MatchType == HiliteMatchTypeBeginsWith) {
                if (HiliteContext->Insensitive) {
                    if (YoriLibCompareStringInsensitiveCount(&LineString,
                                                             &MatchCriteria->MatchString,
                                                             MatchCriteria->MatchString.LengthInChars) == 0) {
                        ColorToUse = MatchCriteria->Color;
                        break;
                    }
                } else {
                    if (YoriLibCompareStringCount(&LineString,
                                                  &MatchCriteria->MatchString,
                                                  MatchCriteria->MatchString.LengthInChars) == 0) {
                        ColorToUse = MatchCriteria->Color;
                        break;
                    }
                }
            } else if (MatchCriteria->MatchType == HiliteMatchTypeEndsWith) {
                YORI_STRING TailOfLine;
    
                if (LineString.LengthInChars >= MatchCriteria->MatchString.LengthInChars) {
                    YoriLibInitEmptyString(&TailOfLine);
                    TailOfLine.LengthInChars = MatchCriteria->MatchString.LengthInChars;
                    TailOfLine.StartOfString = &LineString.StartOfString[LineString.LengthInChars - MatchCriteria->MatchString.LengthInChars];
    
                    if (HiliteContext->Insensitive) {
                        if (YoriLibCompareStringInsensitive(&TailOfLine, &MatchCriteria->MatchString) == 0) {
                            ColorToUse = MatchCriteria->Color;
                            break;
                        }
                    } else {
                        if (YoriLibCompareString(&TailOfLine, &MatchCriteria->MatchString) == 0) {
                            ColorToUse = MatchCriteria->Color;
                            break;
                        }
                    }
                }
            } else if (MatchCriteria->MatchType == HiliteMatchTypeContains) {
                if (HiliteContext->Insensitive) {
                    if (YoriLibFindFirstMatchingSubstringInsensitive(&LineString, 1, &MatchCriteria->MatchString, NULL)) {
                        ColorToUse = MatchCriteria->Color;
                        break;
                    }
                } else {
                    if (YoriLibFindFirstMatchingSubstring(&LineString, 1, &MatchCriteria->MatchString, NULL)) {
                        ColorToUse = MatchCriteria->Color;
                        break;
                    }
                }
            }
            ListEntry = YoriLibGetNextListEntry(&HiliteContext->Matches, ListEntry);
        }

        //
        //  Apply the color and output the line.
        //

        YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, ColorToUse.Win32Attr);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &LineString);
        if (LineString.LengthInChars == 0 || !GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo) || ScreenInfo.dwCursorPosition.X != 0) {
            YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, HiliteContext->DefaultColor.Win32Attr);
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

 @param HiliteContext Pointer to the hilite context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
HiliteFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PHILITE_CONTEXT HiliteContext
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
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hilite: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        HiliteProcessStream(FileHandle, HiliteContext);

        CloseHandle(FileHandle);
    }

    return TRUE;
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
    BOOL Recursive = FALSE;
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

    YoriLibInitializeListHead(&HiliteContext.Matches);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                HiliteHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (i + 2 < ArgC) {
                    NewCriteria = YoriLibMalloc(sizeof(HILITE_MATCH_CRITERIA));
                    if (NewCriteria == NULL) {
                        return EXIT_FAILURE;
                    }
                    NewCriteria->MatchType = HiliteMatchTypeContains;
                    YoriLibInitEmptyString(&NewCriteria->MatchString);
                    NewCriteria->MatchString.StartOfString = ArgV[i + 1].StartOfString;
                    NewCriteria->MatchString.LengthInChars = ArgV[i + 1].LengthInChars;
                    NewCriteria->Color = YoriLibAttributeFromString(ArgV[i + 2].StartOfString);
                    NewCriteria->Color = YoriLibResolveWindowColorComponents(NewCriteria->Color, HiliteContext.DefaultColor, FALSE);
                    YoriLibAppendList(&HiliteContext.Matches, &NewCriteria->ListEntry);
                    ArgumentUnderstood = TRUE;
                    i += 2;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                if (i + 2 < ArgC) {
                    NewCriteria = YoriLibMalloc(sizeof(HILITE_MATCH_CRITERIA));
                    if (NewCriteria == NULL) {
                        return EXIT_FAILURE;
                    }
                    NewCriteria->MatchType = HiliteMatchTypeBeginsWith;
                    YoriLibInitEmptyString(&NewCriteria->MatchString);
                    NewCriteria->MatchString.StartOfString = ArgV[i + 1].StartOfString;
                    NewCriteria->MatchString.LengthInChars = ArgV[i + 1].LengthInChars;
                    NewCriteria->Color = YoriLibAttributeFromString(ArgV[i + 2].StartOfString);
                    NewCriteria->Color = YoriLibResolveWindowColorComponents(NewCriteria->Color, HiliteContext.DefaultColor, FALSE);
                    YoriLibAppendList(&HiliteContext.Matches, &NewCriteria->ListEntry);
                    ArgumentUnderstood = TRUE;
                    i += 2;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                HiliteContext.Insensitive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                if (i + 2 < ArgC) {
                    NewCriteria = YoriLibMalloc(sizeof(HILITE_MATCH_CRITERIA));
                    if (NewCriteria == NULL) {
                        return EXIT_FAILURE;
                    }
                    NewCriteria->MatchType = HiliteMatchTypeEndsWith;
                    YoriLibInitEmptyString(&NewCriteria->MatchString);
                    NewCriteria->MatchString.StartOfString = ArgV[i + 1].StartOfString;
                    NewCriteria->MatchString.LengthInChars = ArgV[i + 1].LengthInChars;
                    NewCriteria->Color = YoriLibAttributeFromString(ArgV[i + 2].StartOfString);
                    NewCriteria->Color = YoriLibResolveWindowColorComponents(NewCriteria->Color, HiliteContext.DefaultColor, FALSE);
                    YoriLibAppendList(&HiliteContext.Matches, &NewCriteria->ListEntry);
                    ArgumentUnderstood = TRUE;
                    i += 2;
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

        HiliteProcessStream(GetStdHandle(STD_INPUT_HANDLE), &HiliteContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }
    
        for (i = StartArg; i < ArgC; i++) {
    
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, HiliteFileFoundCallback, &HiliteContext);
        }
    }

    if (HiliteContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hilite: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
