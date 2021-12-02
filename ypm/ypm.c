/**
 * @file ypm/ypm.c
 *
 * Yori shell package manager tool
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
CHAR strYpmHelpText[] =
        "\n"
        "Installs, upgrades, downloads, creates and removes packages.\n"
        "\n"
        "For more information about an option, add -? with the option.\n";

/**
 A structure that maps a command line argument to a callback function which
 implements it.
 */
typedef struct _YPM_OP_MAP {

    /**
     The command line argument.
     */
    LPTSTR CommandArg;

    /**
     The callback function that implements it.
     */
    PYORI_CMD_BUILTIN Fn;

    /**
     Brief text to display about this command.
     */
    LPCSTR HelpText;
} YPM_OP_MAP, *PYPM_OP_MAP;

/**
 An array of supported commands and functions that implement them.
 */
CONST YPM_OP_MAP YpmCallbackFunctions[] = {
    {_T("c"),               YpmCreateBinaryPackage, "Create a new installable package"},
    {_T("config"),          YpmConfig,              "Update system configuration"},
    {_T("cs"),              YpmCreateSourcePackage, "Create a new source package"},
    {_T("d"),               YpmDelete,              "Delete an installed package"},
    {_T("download"),        YpmDownload,            "Download a group of packages for later installation"},
    {_T("download-daily"),  YpmDownloadDaily,       "Download latest daily packages for later installation"},
    {_T("download-stable"), YpmDownloadStable,      "Download latest stable packages for later installation"},
    {_T("i"),               YpmInstall,             "Install one or more packages"},
    {_T("l"),               YpmList,                "List installed packages"},
    {_T("lv"),              YpmListVerbose,         "List installed packages with verbose information"},
    {_T("md"),              YpmMirrorDelete,        "Delete a registered mirror"},
    {_T("mi"),              YpmMirrorInstall,       "Install a new mirror"},
    {_T("ml"),              YpmMirrorList,          "List registered mirrors"},
    {_T("ri"),              YpmInstallRemote,       "Install package by name from remote servers"},
    {_T("rl"),              YpmRemoteList,          "List remote packages"},
    {_T("rsa"),             YpmRemoteSourceAppend,  "Add a new remote server, resolved last"},
    {_T("rsd"),             YpmRemoteSourceDelete,  "Delete a remote server"},
    {_T("rsi"),             YpmRemoteSourceInsert,  "Add a new remote server, resolved first"},
    {_T("rsl"),             YpmRemoteSourceList,    "List remote servers"},
    {_T("src"),             YpmInstallSource,       "Install source package matching an installed package"},
    {_T("sym"),             YpmInstallSymbols,      "Install symbol package matching an installed package"},
    {_T("u"),               YpmUpgrade,             "Upgrade one or more installed packages"},
    {_T("ud"),              YpmUpgradePreferDaily,  "Upgrade packages to latest daily packages"},
    {_T("us"),              YpmUpgradePreferStable, "Upgrade packages to latest stable packages"},
    {_T("uninstall"),       YpmUninstallAll,        "Uninstall all packages"}
};

/**
 Display usage text to the user.
 */
BOOL
YpmHelp(VOID)
{
    DWORD i;

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ypm %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strYpmHelpText);

    for (i = 0; i < sizeof(YpmCallbackFunctions)/sizeof(YpmCallbackFunctions[0]); i++) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("    -%-16s %hs\n"), YpmCallbackFunctions[i].CommandArg, YpmCallbackFunctions[i].HelpText);
    }
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the ypm builtin command.
 */
#define ENTRYPOINT YoriCmd_YPM
#else
/**
 The main entrypoint for the ypm standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the package manager cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;
    PYORI_CMD_BUILTIN CallbackFn;

    CallbackFn = NULL;

    if (ArgC > 1 &&
        YoriLibIsCommandLineOption(&ArgV[1], &Arg)) {

        for (i = 0; i < sizeof(YpmCallbackFunctions)/sizeof(YpmCallbackFunctions[0]); i++) {
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, YpmCallbackFunctions[i].CommandArg) == 0) {
                CallbackFn = YpmCallbackFunctions[i].Fn;
                break;
            }
        }
    }

    if (CallbackFn != NULL) {
        return CallbackFn(ArgC - 1, &ArgV[1]);
    }

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YpmHelp();
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

    YpmHelp();
    return EXIT_FAILURE;
}

// vim:sw=4:ts=4:et:
