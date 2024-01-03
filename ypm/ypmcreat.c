/**
 * @file ypm/ypmcreat.c
 *
 * Yori shell package manager create packages
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
CHAR strYpmCreateBinaryHelpText[] =
        "\n"
        "Create a binary package.\n"
        "\n"
        "YPM [-license]\n"
        "YPM -c <file> <pkgname> <version> <arch> -filelist <file>\n"
        "       [-minimumosbuild <number>] [-packagepathforolderbuilds <path>]\n"
        "       [-upgradedaily <path>] [-upgradepath <path>] [-upgradestable <path>]\n"
        "       [-sourcepath <path>] [-symbolpath <path>] [-replaces <packages>]\n"
        "\n"
        "   -filelist       Specifies a file containing a list of files to include in\n"
        "                   the package, one per line\n"
        "   -minimumosbuild Specifies the minimum build of NT that can run the package\n"
        "   -packagepathforolderbuilds\n"
        "                   Specifies a URL for a package that can install on builds\n"
        "                   older than minimumosbuild\n"
        "   -replaces       A list of package names that this package should replace\n"
        "   -sourcepath     Specifies a URL containing the source package for this\n"
        "                   binary package\n"
        "   -symbolpath     Specifies a URL containing the symbol package for this\n"
        "                   binary package\n"
        "   -upgradedaily   Specifies a URL containing the latest daily version of\n"
        "                   the package\n"
        "   -upgradepath    Specifies a URL containing the latest version of the\n" 
        "                   package\n"
        "   -upgradestable  Specifies a URL containing the latest stable version of\n"
        "                   the package\n";
/**
 Help text to display to the user.
 */
const
CHAR strYpmCreateSourceHelpText[] =
        "\n"
        "Create a source package.\n"
        "\n"
        "YPM [-license]\n"
        "YPM -cs <file> <pkgname> <version> -filepath <directory>\n"
        "\n"
        "   -filepath       Specifies a directory containing source code\n";

/**
 Display usage text to the user.
 */
BOOL
YpmCreateBinaryHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ypm %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYpmCreateBinaryHelpText);
    return TRUE;
}

/**
 Display usage text to the user.
 */
BOOL
YpmCreateSourceHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ypm %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYpmCreateSourceHelpText);
    return TRUE;
}

/**
 Create a new package containing binary files that can be installed on a
 user's system.  These packages consist of files provided in a list of files.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process.
 */
DWORD
YpmCreateBinaryPackage(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    PYORI_STRING NewFileName = NULL;
    PYORI_STRING NewVersion = NULL;
    PYORI_STRING NewArch = NULL;
    PYORI_STRING NewName = NULL;
    PYORI_STRING SourcePath = NULL;
    PYORI_STRING UpgradePath = NULL;
    PYORI_STRING UpgradeToStablePath = NULL;
    PYORI_STRING UpgradeToDailyPath = NULL;
    PYORI_STRING SymbolPath = NULL;
    PYORI_STRING FileList = NULL;
    PYORI_STRING FilePath = NULL;
    PYORI_STRING Replaces = NULL;
    PYORI_STRING MinimumOSBuild = NULL;
    PYORI_STRING PackagePathForOlderBuilds = NULL;
    DWORD ReplaceCount = 0;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YpmCreateBinaryHelp();
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
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("filelist")) == 0) {
                if (i + 1 < ArgC) {
                    FileList = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("filepath")) == 0) {
                if (i + 1 < ArgC) {
                    FilePath = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("minimumosbuild")) == 0) {
                if (i + 1 < ArgC) {
                    MinimumOSBuild = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }

            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("packagepathforolderbuilds")) == 0) {
                if (i + 1 < ArgC) {
                    PackagePathForOlderBuilds = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("replaces")) == 0) {
                if (i + 1 < ArgC) {
                    ArgumentUnderstood = TRUE;
                    Replaces = &ArgV[i + 1];

                    while (i + 1 < ArgC &&
                           !YoriLibIsCommandLineOption(&ArgV[i + 1], &Arg)) {

                        ReplaceCount++;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("sourcepath")) == 0) {
                if (i + 1 < ArgC) {
                    SourcePath = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("symbolpath")) == 0) {
                if (i + 1 < ArgC) {
                    SymbolPath = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("upgradedaily")) == 0) {
                if (i + 1 < ArgC) {
                    UpgradeToDailyPath = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("upgradepath")) == 0) {
                if (i + 1 < ArgC) {
                    UpgradePath = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("upgradestable")) == 0) {
                if (i + 1 < ArgC) {
                    UpgradeToStablePath = &ArgV[i + 1];
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

            //
            //  Historically these commands used un-named arguments at the
            //  beginning, which is not how StartArg would normally work.
            //  Here this is special cased to allow these to be anywhere.
            //  Note the lack of 'break' in the below.
            //

            if (StartArg == 0 && i + 3 < ArgC) {
                ArgumentUnderstood = TRUE;
                StartArg = i;
                i = i + 3;
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0 || StartArg + 3 >= ArgC) {
        YpmCreateBinaryHelp();
        return EXIT_FAILURE;
    }

    NewFileName = &ArgV[StartArg];
    NewName = &ArgV[StartArg + 1];
    NewVersion = &ArgV[StartArg + 2];
    NewArch = &ArgV[StartArg + 3];

    ASSERT(NewFileName != NULL && NewName != NULL && NewVersion != NULL && NewArch != NULL);
    if (FileList == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: missing file list\n"));
        return EXIT_FAILURE;
    }

    YoriPkgCreateBinaryPackage(NewFileName,
                               NewName,
                               NewVersion,
                               NewArch,
                               FileList,
                               MinimumOSBuild,
                               PackagePathForOlderBuilds,
                               UpgradePath,
                               SourcePath,
                               SymbolPath,
                               UpgradeToStablePath,
                               UpgradeToDailyPath,
                               Replaces,
                               ReplaceCount);

    return EXIT_SUCCESS;
}

/**
 Create a new package containing source files that can be installed on a
 user's system.  These packages consist of files found in a directory tree,
 while excluding any files specified by .gitignore.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process.
 */
DWORD
YpmCreateSourcePackage(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    PYORI_STRING NewFileName = NULL;
    PYORI_STRING NewVersion = NULL;
    PYORI_STRING NewName = NULL;
    PYORI_STRING FilePath = NULL;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YpmCreateSourceHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("filepath")) == 0) {
                if (i + 1 < ArgC) {
                    FilePath = &ArgV[i + 1];
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartArg = i + 1;
                break;
            }
        } else {
            //
            //  Historically these commands used un-named arguments at the
            //  beginning, which is not how StartArg would normally work.
            //  Here this is special cased to allow these to be anywhere.
            //  Note the lack of 'break' in the below.
            //

            if (StartArg == 0 && i + 2 < ArgC) {
                ArgumentUnderstood = TRUE;
                StartArg = i;
                i = i + 2;
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0 || StartArg + 2 >= ArgC) {
        YpmCreateSourceHelp();
        return EXIT_FAILURE;
    }

    NewFileName = &ArgV[StartArg];
    NewName = &ArgV[StartArg + 1];
    NewVersion = &ArgV[StartArg + 2];

    ASSERT(NewFileName != NULL && NewName != NULL && NewVersion != NULL);
    if (FilePath == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: missing file tree root\n"));
        return EXIT_FAILURE;
    }
    YoriPkgCreateSourcePackage(NewFileName, NewName, NewVersion, FilePath);

    return EXIT_SUCCESS;
}


// vim:sw=4:ts=4:et:
