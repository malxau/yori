/**
 * @file which/which.c
 *
 * Small minicrt wrapper app that searches the path or other environment
 * variable looking for the first matching file.
 *
 * Copyright (c) 2014-2018 Malcolm J. Smith
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
 The string to search for.
 */
PYORI_STRING SearchFor = NULL;

/**
 The variable to search in.
 */
LPTSTR SearchVar = _T("PATH");

/**
 Usage text for this application.
 */
const
CHAR strUsageText[] =
     "\n"
     "Searches a semicolon delimited environment variable for a file.  When\n"
     "searching PATH, also applies PATHEXT executable extension matching.\n"
     "\n"
     "WHICH [-license] [-p <variable>] <file>\n"
     "\n"
     "   -p var Indicates the environment variable to search.  If not specified, use PATH\n"
     "\n"
     "   -get-update    Update to latest version of current type\n"
     "   -get-daily     Get the latest daily version\n"
     "   -get-source    Get source for this version\n"
     "   -get-stable    Get the latest stable version\n"
     "   -get-symbol    Get debugging symbol for this version\n"
     "   -get-ver-x.y   Get a specific version of the program\n"
     "\n"
     " If PATHEXT not defined, defaults to .COM, .EXE, .BAT and .CMD\n"
     " If file extension not specified and var not specified, searches for files\n"
     "  ending in extensions in PATHEXT\n";

/**
 Display usage text to the user.
 */
VOID
WhichUsage()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Which %i.%i\n"), WHICH_VER_MAJOR, WHICH_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strUsageText);
}

/**
 Parse user specified arguments.

 @param ArgC The count of arguments.

 @param ArgV Array of arguments.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
WhichParseArgs(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    DWORD i;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {
        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {
            BOOL Parsed = FALSE;

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2014-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0 &&
                       ArgC > i + 1) {

                i++;
                SearchVar = ArgV[i].StartOfString;
                Parsed = TRUE;
            }

            if (!Parsed) {
                WhichUsage();
                return FALSE;
            }
        } else {
            SearchFor = &ArgV[i];
        }
    }

    if (SearchFor == NULL) {
        WhichUsage();
        return FALSE;
    }

    return TRUE;
}

/**
 The main entrypoint for the which application.

 @param ArgC Count of arguments.

 @param ArgV Array of arguments.

 @return ExitCode, zero indicating success, nonzero indicating failure.
 */
DWORD
ymain (
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_STRING FoundPath;
    BOOL Result = FALSE;

    if (!WhichParseArgs(ArgC, ArgV)) {
        return EXIT_FAILURE;
    }

    //
    //  When running on WOW64, we don't want file system redirection,
    //  for path evaluation, because 32 bit processes will execute
    //  from 64 bit paths.  For other variables, leave redirection
    //  in place.
    //

    if (DllKernel32.pWow64DisableWow64FsRedirection &&
        _tcsicmp(SearchVar, _T("PATH")) == 0) {

        PVOID DontCare;
        DllKernel32.pWow64DisableWow64FsRedirection(&DontCare);
    }

    YoriLibInitEmptyString(&FoundPath);

    if (_tcsicmp(SearchVar, _T("PATH")) == 0) {
        Result = YoriLibLocateExecutableInPath(SearchFor, NULL, NULL, &FoundPath);
    } else {
        DWORD VarLength;
        YORI_STRING SearchVarData;

        YoriLibInitEmptyString(&SearchVarData);
        VarLength = GetEnvironmentVariable(SearchVar, NULL, 0);

        if (VarLength > 0) {
            if (YoriLibAllocateString(&SearchVarData, VarLength)) {
                SearchVarData.LengthInChars = GetEnvironmentVariable(SearchVar, SearchVarData.StartOfString, SearchVarData.LengthAllocated);
                if (SearchVarData.LengthInChars == 0 || SearchVarData.LengthInChars >= SearchVarData.LengthAllocated) {
                    YoriLibFreeStringContents(&SearchVarData);
                }
            }
        }

        if (!YoriLibAllocateString(&FoundPath, VarLength + MAX_PATH)) {
            YoriLibFreeStringContents(&SearchVarData);
        }

        Result = FALSE;
        if (SearchVarData.StartOfString != NULL) {
            if (FoundPath.StartOfString != NULL) {
                Result = YoriLibPathLocateKnownExtensionUnknownLocation(SearchFor, &SearchVarData, NULL, NULL, &FoundPath);
            }
            YoriLibFreeStringContents(&SearchVarData);
        }
    }

    //
    //  Tell the user what we found, if anything
    //

    if (!Result) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Error performing search\n"));
        return EXIT_FAILURE;
    } else if (FoundPath.LengthInChars > 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &FoundPath);
        YoriLibFreeStringContents(&FoundPath);
        return EXIT_SUCCESS;
    } else {
        YoriLibFreeStringContents(&FoundPath);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Not found\n"));
        return EXIT_FAILURE;
    }
}

// vim:sw=4:ts=4:et:
