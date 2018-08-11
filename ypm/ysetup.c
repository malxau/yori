/**
 * @file ypm/ysetup.c
 *
 * Yori shell bootstrap installer
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
#include "ypm.h"

/**
 Help text to display to the user.
 */
const
CHAR strHelpText[] =
        "\n"
        "Installs a basic Yori system.\n"
        "\n"
        "YSETUP [-license] [directory]\n";

/**
 Display usage text to the user.
 */
BOOL
YsetupHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ysetup %i.%i\n"), YPM_VER_MAJOR, YPM_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 The main entrypoint for the setup cmdlet.

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
    DWORD StartArg = 0;
    DWORD SuccessCount = 0;
    YORI_STRING Arg;
    YORI_STRING PkgNames[3];
    YORI_STRING NewDirectory;
    PYORI_STRING DirectoryToUse = NULL;

    YoriLibInitEmptyString(&NewDirectory);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YsetupHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
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

    if (StartArg > 0) {
        YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &NewDirectory);
        DirectoryToUse = &NewDirectory;
    }

    YoriLibConstantString(&PkgNames[0], _T("yori-ypm"));
    YoriLibConstantString(&PkgNames[1], _T("yori-core"));
    YoriLibConstantString(&PkgNames[2], _T("yori-typical"));

    SuccessCount = YpmInstallRemotePackages(PkgNames, sizeof(PkgNames)/sizeof(PkgNames[0]), DirectoryToUse, NULL, NULL);

    YoriLibFreeStringContents(&NewDirectory);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
