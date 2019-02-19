/**
 * @file echo/echo.c
 *
 * Yori shell display command line output
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
CHAR strEchoHelpText[] =
        "\n"
        "Outputs text.\n"
        "\n"
        "ECHO [-license] [-e] [-n] [--] String\n"
        "\n"
        "   --             Treat all further arguments as disaplay parameters\n"
        "   -e             Display to standard error stream\n"
        "   -n             Do not display a newline after text\n";

/**
 Display usage text to the user.
 */
BOOL
EchoHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Echo %i.%02i\n"), ECHO_VER_MAJOR, ECHO_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strEchoHelpText);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the echo builtin command.
 */
#define ENTRYPOINT YoriCmd_YECHO
#else
/**
 The main entrypoint for the echo standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the echo cmdlet.

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
    BOOL NewLine = TRUE;
    BOOL StdErr = FALSE;
    BOOL Result;
    DWORD OutputFlags;
    DWORD i;
    DWORD StartArg = 1;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                EchoHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                StdErr = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                NewLine = FALSE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartArg = i + 1;
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

    i = StartArg;

    if (StdErr) {
        OutputFlags = YORI_LIB_OUTPUT_STDERR;
    } else {
        OutputFlags = YORI_LIB_OUTPUT_STDOUT;
    }

    Result = TRUE;

    do {
        if (i < ArgC) {
            if (!YoriLibOutput(OutputFlags, _T("%y"), &ArgV[i])) {
                Result = FALSE;
            }
            if (i + 1 < ArgC) {
                if (!YoriLibOutput(OutputFlags, _T(" "))) {
                    Result = FALSE;
                }
            }
        }
        i++;
    } while (i < ArgC);

    if (NewLine) {
        if (!YoriLibOutput(OutputFlags, _T("\n"))) {
            Result = FALSE;
        }
    }

    if (Result) {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

// vim:sw=4:ts=4:et:
