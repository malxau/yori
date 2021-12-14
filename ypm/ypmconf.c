/**
 * @file ypm/ypmconf.c
 *
 * Yori shell package manager configuration module
 *
 * Copyright (c) 2021 Malcolm J. Smith
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
CHAR strYpmConfigHelpText[] =
        "\n"
        "Update system configuration.\n"
        "\n"
        "YPM [-license]\n"
        "YPM -config [-desktop] [-loginshell] [-start] [-systempath] [-terminal]\n"
        "            [-userpath] [-yui]\n"
        "\n"
        "   -desktop       Create a Desktop shortcut\n"
        "   -loginshell    Make Yori the program to run on login\n"
        "   -start         Create a Start Menu shortcut\n"
        "   -systempath    Add to system path\n"
        "   -terminal      Create a Windows Terminal fragment\n"
        "   -userpath      Add to user path\n"
        "   -yui           Make Yui the program to run on login\n";

/**
 Display usage text to the user.
 */
BOOL
YpmConfigHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ypm %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYpmConfigHelpText);
    return TRUE;
}

/**
 Yori update system configuration.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process.
 */
DWORD
YpmConfig(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;
    BOOLEAN CreateTerminalProfile;
    BOOLEAN CreateDesktopShortcut;
    BOOLEAN CreateStartMenuShortcut;
    BOOLEAN AppendToUserPath;
    BOOLEAN AppendToSystemPath;
    BOOLEAN LoginShell;
    BOOLEAN YuiShell;
    BOOLEAN Failure;

    CreateTerminalProfile = FALSE;
    CreateDesktopShortcut = FALSE;
    CreateStartMenuShortcut = FALSE;
    AppendToUserPath = FALSE;
    AppendToSystemPath = FALSE;
    LoginShell = FALSE;
    YuiShell = FALSE;

    Failure = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YpmConfigHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("desktop")) == 0) {
                CreateDesktopShortcut = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("loginshell")) == 0) {
                LoginShell = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("start")) == 0) {
                CreateStartMenuShortcut = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("systempath")) == 0) {
                AppendToSystemPath = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("terminal")) == 0) {
                CreateTerminalProfile = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("userpath")) == 0) {
                AppendToUserPath = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("yui")) == 0) {
                YuiShell = TRUE;
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

    if (!CreateTerminalProfile &&
        !CreateDesktopShortcut &&
        !CreateStartMenuShortcut &&
        !AppendToUserPath &&
        !AppendToSystemPath &&
        !LoginShell &&
        !YuiShell) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm config: missing operation\n"));
        return EXIT_FAILURE;
    }

    if (LoginShell && YuiShell) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm config: cannot set login shell to yui and yori simultaneously\n"));
        return EXIT_FAILURE;
    }

    if (CreateTerminalProfile) {
        if (!YoriPkgWriteTerminalProfile(NULL)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm config: could not create terminal profile\n"));
            Failure = TRUE;
        }
    }

    if (CreateDesktopShortcut) {
        if (!YoriPkgCreateDesktopShortcut(NULL)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm config: could not create desktop shortcut\n"));
            Failure = TRUE;
        }
    }

    if (CreateStartMenuShortcut) {
        if (!YoriPkgCreateStartMenuShortcut(NULL)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm config: could not create start menu shortcut\n"));
            Failure = TRUE;
        }
    }

    if (AppendToUserPath || AppendToSystemPath) {
        if (!YoriPkgAppendInstallDirToPath(NULL, AppendToUserPath, AppendToSystemPath)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm config: could not update path\n"));
            Failure = TRUE;
        }
    }

    if (LoginShell) {
        if (!YoriPkgInstallYoriAsLoginShell(NULL)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm config: could not update login shell. Are you running as an elevated Administrator?\n"));
            Failure = TRUE;
        }
    }

    if (YuiShell) {
        if (YoriLibIsNanoServer()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm config: cannot install a graphical shell on a text mode operating system\n"));
            Failure = TRUE;
        } else if (!YoriPkgInstallYuiAsLoginShell(NULL)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm config: could not update login shell. Are you running as an elevated Administrator?\n  Is yui installed?\n"));
            Failure = TRUE;
        }
    }

    if (Failure) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
