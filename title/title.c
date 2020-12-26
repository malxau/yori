/**
 * @file title/title.c
 *
 * Yori shell set window title
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
CHAR strTitleHelpText[] =
        "\n"
        "Get or set the console window title.\n"
        "\n"
        "TITLE [-license] [-g|<title>]\n";

/**
 Display usage text to the user.
 */
BOOL
TitleHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Title %i.%02i\n"), TITLE_VER_MAJOR, TITLE_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strTitleHelpText);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the title builtin command.
 */
#define ENTRYPOINT YoriCmd_YTITLE
#else
/**
 The main entrypoint for the title standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the title cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero on success, nonzero on failure.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_STRING CmdLine;
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 1;
    YORI_STRING Arg;
    BOOL GetTitleMode = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                TitleHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("g")) == 0) {
                GetTitleMode = TRUE;
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

    if (GetTitleMode) {
        YORI_STRING PreviousTitle;

        //
        //  MSDN implies there's no way to know how big the buffer needs to be,
        //  but it needs to be less than 64Kb, so we use that.
        //

        if (!YoriLibAllocateString(&PreviousTitle, 64 * 1024)) {
            return EXIT_FAILURE;
        }

        PreviousTitle.LengthInChars = GetConsoleTitle(PreviousTitle.StartOfString, PreviousTitle.LengthAllocated);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &PreviousTitle);
        YoriLibFreeStringContents(&PreviousTitle);

    } else {
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, &CmdLine)) {
            return EXIT_FAILURE;
        }
        ASSERT(YoriLibIsStringNullTerminated(&CmdLine));
    
        SetConsoleTitle(CmdLine.StartOfString);
        YoriLibFreeStringContents(&CmdLine);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
