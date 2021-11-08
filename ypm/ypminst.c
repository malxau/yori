/**
 * @file ypm/ypminst.c
 *
 * Yori shell package manager install packages
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
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
#include <yoripkg.h>
#include "ypm.h"

/**
 Help text to display to the user.
 */
const
CHAR strYpmInstallHelpText[] =
        "\n"
        "Installs one or more packages.\n"
        "\n"
        "YPM [-license]\n"
        "YPM -i <file> ...\n";

/**
 Help text to display to the user.
 */
const
CHAR strYpmInstallRemoteHelpText[] =
        "\n"
        "Install packages from remote servers.\n"
        "\n"
        "YPM [-license]\n"
        "YPM -ri [-a <arch>] [-v <version>] <pkgname>\n"
        "\n"
        "   -a             Specify a CPU architecture to install\n"
        "   -v             Specify a package version to install\n";

/**
 Help text to display to the user.
 */
const
CHAR strYpmInstallSourceHelpText[] =
        "\n"
        "Installs source for one or more packages.\n"
        "\n"
        "YPM [-license]\n"
        "YPM -src [<pkg>]\n";

/**
 Help text to display to the user.
 */
const
CHAR strYpmInstallSymbolsHelpText[] =
        "\n"
        "Installs debugging symbols for one or more packages.\n"
        "\n"
        "YPM [-license]\n"
        "YPM -sym [<pkg>]\n";

/**
 Display usage text to the user.
 */
BOOL
YpmInstallHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ypm %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYpmInstallHelpText);
    return TRUE;
}

/**
 Display usage text to the user.
 */
BOOL
YpmInstallRemoteHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ypm %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYpmInstallRemoteHelpText);
    return TRUE;
}

/**
 Display usage text to the user.
 */
BOOL
YpmInstallSourceHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ypm %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYpmInstallSourceHelpText);
    return TRUE;
}

/**
 Display usage text to the user.
 */
BOOL
YpmInstallSymbolsHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ypm %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYpmInstallSymbolsHelpText);
    return TRUE;
}

/**
 Install one or more packages.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process.
 */
DWORD
YpmInstall(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YpmInstallHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
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

    if (StartArg == 0 || StartArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: missing argument\n"));
        return EXIT_FAILURE;
    }

    for (i = StartArg; i < ArgC; i++) {
        YoriPkgInstallSinglePackage(&ArgV[i], NULL);
    }

    return EXIT_SUCCESS;
}

/**
 Install the source for one or more packages.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process.
 */
DWORD
YpmInstallSource(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YpmInstallSourceHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
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

    if (StartArg == 0 || StartArg >= ArgC) {
        YoriPkgInstallSourceForInstalledPackages();
    } else {
        for (i = StartArg; i < ArgC; i++) {
            YoriPkgInstallSourceForSinglePackage(&ArgV[i]);
        }
    }

    return EXIT_SUCCESS;
}

/**
 Install the debugging symbols for one or more packages.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process.
 */
DWORD
YpmInstallSymbols(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YpmInstallSymbolsHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
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

    if (StartArg == 0 || StartArg >= ArgC) {
        YoriPkgInstallSymbolsForInstalledPackages();
    } else {
        for (i = StartArg; i < ArgC; i++) {
            YoriPkgInstallSymbolForSinglePackage(&ArgV[i]);
        }
    }

    return EXIT_SUCCESS;
}

/**
 Install packages from remote sources.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process.
 */
DWORD
YpmInstallRemote(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;
    PYORI_STRING NewVersion = NULL;
    PYORI_STRING NewArch = NULL;
    DWORD PkgCount;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YpmInstallRemoteHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                if (i + 1 < ArgC) {
                    NewArch = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("v")) == 0) {
                if (i + 1 < ArgC) {
                    NewVersion = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
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

    PkgCount = ArgC - StartArg;
    if (StartArg == 0 || PkgCount == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: missing package name\n"));
        return EXIT_FAILURE;
    }
    if (!YoriPkgInstallRemotePackages(&ArgV[StartArg], PkgCount, NULL, NULL, NewVersion, NewArch)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: one or more packages could not be found or installed\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
