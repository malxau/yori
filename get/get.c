/**
 * @file get/get.c
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
CHAR strHelpText[] =
        "Fetches objects from HTTP and stores them in local files.\n"
        "\n"
        "GET [-license] <url> <file>\n";

/**
 Display usage text to the user.
 */
BOOL
GetHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Get %i.%i\n"), GET_VER_MAJOR, GET_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 The main entrypoint for the get cmdlet.

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
    DWORD i;
    DWORD StartArg = 1;
    YORI_STRING NewFileName;
    LPTSTR ExistingUrlName;
    YoriLibUpdError Error;
    TCHAR szAgent[64];
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                GetHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
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
    if (ArgC - StartArg < 2) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("get: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg + 1], TRUE, &NewFileName)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("get: failed to resolve %y\n"), &ArgV[StartArg + 1]);
        return EXIT_FAILURE;
    }

    ExistingUrlName = ArgV[StartArg].StartOfString;

    YoriLibSPrintf(szAgent, _T("YGet %i.%i\r\n"), GET_VER_MAJOR, GET_VER_MINOR);
    Error = YoriLibUpdateBinaryFromUrl(ExistingUrlName, NewFileName.StartOfString, szAgent);
    YoriLibFreeStringContents(&NewFileName);
    if (Error != YoriLibUpdErrorSuccess) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("get: failed to download: %s\n"), YoriLibUpdateErrorString(Error));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
