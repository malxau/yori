/**
 * @file ysetup/ysetup.c
 *
 * Yori shell bootstrap installer
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
#include "resource.h"
#include "ysetup.h"

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
SetupHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ysetup %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}


/**
 Install the default set of packages to a specified directory.

 @param InstallDirectory Specifies the directory to install to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetupInstallToDirectory(
    __inout PYORI_STRING InstallDirectory
    )
{
    YORI_STRING LocalPath;
    PYORI_STRING CustomSource;
    YORI_STRING PkgNames[4];

    YoriLibInitEmptyString(&LocalPath);
    CustomSource = NULL;

    if (SetupFindLocalPkgPath(&LocalPath)) {
        CustomSource = &LocalPath;
    } else {
        YoriLibInitEmptyString(&LocalPath);
    }

    if (!YoriLibCreateDirectoryAndParents(InstallDirectory)) {
        DWORD Err = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ysetup: Could not create installation directory %y: %s\n"), InstallDirectory, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&LocalPath);
        return FALSE;
    }

    YoriLibConstantString(&PkgNames[0], _T("yori-ypm"));
    YoriLibConstantString(&PkgNames[1], _T("yori-core"));
    YoriLibConstantString(&PkgNames[2], _T("yori-typical"));
    YoriLibConstantString(&PkgNames[3], _T("yori-completion"));

    if (YoriPkgInstallRemotePackages(PkgNames, sizeof(PkgNames)/sizeof(PkgNames[0]), CustomSource, InstallDirectory, NULL, NULL)) {
        YoriLibFreeStringContents(&LocalPath);
        return TRUE;
    }

    YoriLibFreeStringContents(&LocalPath);
    return FALSE;
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
    YORI_STRING Arg;
    YORI_STRING NewDirectory;

    YoriLibInitEmptyString(&NewDirectory);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SetupHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2021"));
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
        if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &NewDirectory)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ysetup: install failed\n"));
            return EXIT_FAILURE;
        }

        if (!SetupInstallToDirectory(&NewDirectory)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ysetup: install failed\n"));
            YoriLibFreeStringContents(&NewDirectory);
            return EXIT_FAILURE;
        }
        YoriLibFreeStringContents(&NewDirectory);
    } else {
        SetupGuiDisplayUi();
    }


    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
