/**
 * @file cshot/cshot.c
 *
 * Yori shell cshot
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
 * FITNESS CSHOT A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE CSHOT ANY CLAIM, DAMAGES OR OTHER
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
        "Captures previous output on the console and outputs to standard output.\n"
        "\n"
        "CSHOT [-license] [-s num] [-c num]\n"
        "\n"
        "   -c             The number of lines to capture\n"
        "   -s             The number of lines to skip\n";

/**
 Display usage text to the user.
 */
BOOL
CshotHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Cshot %i.%i\n"), CSHOT_VER_MAJOR, CSHOT_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 The main entrypoint for the cshot cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD SkipCount = 0;
    DWORD LineCount = 0;
    DWORD i;
    YORI_STRING Arg;
    LONGLONG Temp;
    DWORD CharsConsumed;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                CshotHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &Temp, &CharsConsumed)) {


                        LineCount = (DWORD)Temp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                if (ArgC > i + 1) {
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &Temp, &CharsConsumed)) {
                        SkipCount = (DWORD)Temp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (!YoriLibRewriteConsoleContents(GetStdHandle(STD_OUTPUT_HANDLE), LineCount, SkipCount)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
