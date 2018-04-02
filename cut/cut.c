/**
 * @file cut/cut.c
 *
 * Yori shell filter within a line of output
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
        "Outputs a portion of an input buffer of text.\n"
        "\n"
        "CUT [-b] [-s] [-f n] [-d <delimiter chars>] [-o n] [-l n] [file]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -o             The offset in bytes to cut from the line or field\n"
        "   -l             The length in bytes to cut from the line or field\n"
        "   -f n           The field number to cut\n"
        "   -d             The set of characters which delimit fields, default comma\n"
        "   -s             Match files from all subdirectories\n"
        ;

/**
 Display usage text to the user.
 */
BOOL
CutHelp()
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2017-2018"), &License);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Cut %i.%i\n"), CUT_VER_MAJOR, CUT_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &License);
    YoriLibFreeStringContents(&License);
    return TRUE;
}

/**
 Context describing the operations to perform on each file found.
 */
typedef struct _CUT_CONTEXT {

    /**
     TRUE if the input should be delimited by fields.  If FALSE, the input
     is delimited via bytes.
     */
    BOOL FieldDelimited;

    /**
     For a field delimited stream, contains the NULL terminated string
     indicating one or more characters to interpret as delimiters.
     */
    LPTSTR FieldSeperator;

    /**
     For a field delimited stream, indicates the field number that should
     be output.
     */
    DWORD FieldOfInterest;

    /**
     Indicates the offset of the line or field, in bytes, that is of interest.
     */
    DWORD DesiredOffset;

    /**
     Indicates the length of the range that is of interest.
     */
    DWORD DesiredLength;

    /**
     Counts the number of files encountered as files are processed.
     */
    DWORD FilesFound;
} CUT_CONTEXT, *PCUT_CONTEXT;

/**
 Process an incoming stream from a single handle, applying the user requested
 actions.

 @param hSource The source handle containing data to process.

 @param CutContext The context that describes the actions to perform.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CutFilterHandle(
    __in HANDLE hSource,
    __in PCUT_CONTEXT CutContext
    )
{
    YORI_STRING MatchingSubset;
    PVOID LineContext = NULL;
    YORI_STRING LineString;

    YoriLibInitEmptyString(&LineString);

    while (TRUE) {
        if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
            break;
        }

        YoriLibInitEmptyString(&MatchingSubset);

        MatchingSubset.StartOfString = LineString.StartOfString;
        MatchingSubset.LengthInChars = LineString.LengthInChars;

        if (CutContext->FieldDelimited) {
            DWORD CurrentField = 0;

            for (CurrentField = 0; CurrentField <= CutContext->FieldOfInterest; CurrentField++) {
                DWORD CharsBeforeSeperator;

                CharsBeforeSeperator = YoriLibCountStringNotContainingChars(&MatchingSubset, CutContext->FieldSeperator);
                if (CurrentField == CutContext->FieldOfInterest) {
                    MatchingSubset.LengthInChars = CharsBeforeSeperator;
                } else {
                    MatchingSubset.LengthInChars -= CharsBeforeSeperator;
                    if (MatchingSubset.LengthInChars == 0) {
                        break;
                    }
                    MatchingSubset.LengthInChars -= 1;
                    MatchingSubset.StartOfString = &MatchingSubset.StartOfString[CharsBeforeSeperator + 1];
                }
            }
        }
        if (MatchingSubset.LengthInChars > CutContext->DesiredOffset) {
            MatchingSubset.StartOfString = &MatchingSubset.StartOfString[CutContext->DesiredOffset];
            MatchingSubset.LengthInChars = MatchingSubset.LengthInChars - CutContext->DesiredOffset;

            if (CutContext->DesiredLength != 0 &&
                MatchingSubset.LengthInChars > CutContext->DesiredLength) {

                MatchingSubset.LengthInChars = CutContext->DesiredLength;
            }
        } else {
            MatchingSubset.LengthInChars = 0;
        }

        if (MatchingSubset.LengthInChars > 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &MatchingSubset);
        }
    }

    YoriLibLineReadClose(LineContext);
    YoriLibFreeStringContents(&LineString);

    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param Filename Pointer to the file path that was found.

 @param FindData Information about the file.

 @param Depth Recursion depth, unused in this application.

 @param CutContext Pointer to a context block the cut operation to perform and
        tracking the number of files that have been processed.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
CutFileFoundCallback(
    __in PYORI_STRING Filename,
    __in PWIN32_FIND_DATA FindData,
    __in DWORD Depth,
    __in PCUT_CONTEXT CutContext
    )
{
    HANDLE hSource;

    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FindData);

    ASSERT(Filename->StartOfString[Filename->LengthInChars] == '\0');

    hSource = CreateFile(Filename->StartOfString, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSource == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("cut: open file failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return EXIT_FAILURE;
    }

    CutContext->FilesFound++;
    CutFilterHandle(hSource, CutContext);

    CloseHandle(hSource);
    return TRUE;
}

/**
 The main entrypoint for the cut cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of zero to indicate success, nonzero to indicate failure.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    HANDLE hSource = NULL;
    BOOL ArgumentUnderstood;
    DWORD i;
    CUT_CONTEXT CutContext;
    BOOL Recursive = FALSE;
    BOOL BasicEnumeration = FALSE;
    DWORD StartArg = 0;
    YORI_STRING Arg;
    LONGLONG Temp;
    DWORD CharsConsumed;

    ZeroMemory(&CutContext, sizeof(CutContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                CutHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("o")) == 0) {
                if (ArgC > i + 1) {
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &Temp, &CharsConsumed)) {
                        CutContext.DesiredOffset = (DWORD)Temp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                if (ArgC > i + 1) {
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &Temp, &CharsConsumed)) {
                        CutContext.DesiredLength = (DWORD)Temp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (ArgC > i + 1) {
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &Temp, &CharsConsumed)) {
                        CutContext.FieldDelimited = TRUE;
                        CutContext.FieldOfInterest = (DWORD)Temp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                if (ArgC > i + 1) {
                    CutContext.FieldDelimited = TRUE;
                    CutContext.FieldSeperator = ArgV[i + 1].StartOfString;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            }
        } else {
            StartArg = i;
            ArgumentUnderstood = TRUE;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (CutContext.FieldSeperator == NULL) {
        CutContext.FieldSeperator = _T(",");
    }

    if (StartArg == 0) {
        DWORD FileType = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
        FileType = FileType & ~(FILE_TYPE_REMOTE);
        if (FileType == FILE_TYPE_CHAR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
            return EXIT_FAILURE;
        }
        hSource = GetStdHandle(STD_INPUT_HANDLE);
        CutFilterHandle(hSource, &CutContext);
    } else {
        DWORD MatchFlags = YORILIB_FILEENUM_RETURN_FILES;
        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }

        for (i = StartArg; i < ArgC; i++) {
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, CutFileFoundCallback, &CutContext);
        }

        if (CutContext.FilesFound == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("cut: no matching files found\n"));
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
