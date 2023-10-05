/**
 * @file ysetup/ysetup.c
 *
 * Yori shell bootstrap installer
 *
 * Copyright (c) 2018-2023 Malcolm J. Smith
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
        "YSETUP [-license] [-core|-typical|-complete] [-desktop] [-gui] [-source]\n"
        "       [-start] [-symbols] [-systempath] [-terminal] [-text] [-uninstall]\n"
        "       [-userpath] [directory]\n"
        "\n"
        "   -gui           Use graphical installer (default)\n"
#if YSETUP_TUI
        "   -text          Use text installer\n"
#endif
        "\n"
        "   -core          Install minimal components.\n"
        "   -typical       Install typical components.\n"
        "   -complete      Install all components.\n"
        "\n"
        "   -desktop       Install a desktop shortcut.\n"
        "   -source        Install source code.\n"
        "   -start         Install a start menu shortcut.\n"
        "   -symbols       Install debugging symbols.\n"
        "   -systempath    Add path to the system path.\n"
        "   -terminal      Install Windows Terminal profile.\n"
        "   -uninstall     Install an uninstall entry.\n"
        "   -userpath      Add path to the user path.\n";

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
 Display installation progress to the console.

 @param StatusText Pointer to the current status.

 @param Context Opaque context pointer, unused in this routine.
 */
VOID
SetupCliUpdateStatus(
    __in PCYORI_STRING StatusText,
    __in_opt PVOID Context
    )
{
    UNREFERENCED_PARAMETER(Context);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), StatusText);
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
    YSETUP_INSTALL_TYPE InstallType;
    DWORD InstallOptions;
    YORI_STRING NewDirectory;
    BOOL Result;
    enum {
        UiDefault,
        UiCli,
#if YSETUP_TUI
        UiTui,
#endif
        UiGui
    } UiToUse;

    YoriLibInitEmptyString(&NewDirectory);
    InstallOptions = YSETUP_INSTALL_COMPLETION;
    InstallType = InstallTypeDefault;
    UiToUse = UiDefault;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SetupHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2023"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("complete")) == 0) {
                InstallType = InstallTypeComplete;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("core")) == 0) {
                InstallType = InstallTypeCore;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("desktop")) == 0) {
                InstallOptions = InstallOptions | YSETUP_INSTALL_DESKTOP_SHORTCUT;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("gui")) == 0) {
                UiToUse = UiGui;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("source")) == 0) {
                InstallOptions = InstallOptions | YSETUP_INSTALL_SOURCE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("start")) == 0) {
                InstallOptions = InstallOptions | YSETUP_INSTALL_START_SHORTCUT;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("symbols")) == 0) {
                InstallOptions = InstallOptions | YSETUP_INSTALL_SYMBOLS;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("systempath")) == 0) {
                InstallOptions = InstallOptions | YSETUP_INSTALL_SYSTEM_PATH;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("terminal")) == 0) {
                InstallOptions = InstallOptions | YSETUP_INSTALL_TERMINAL_PROFILE;
                ArgumentUnderstood = TRUE;
#if YSETUP_TUI
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("text")) == 0) {
                UiToUse = UiTui;
                ArgumentUnderstood = TRUE;
#endif
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("typical")) == 0) {
                InstallType = InstallTypeTypical;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("uninstall")) == 0) {
                InstallOptions = InstallOptions | YSETUP_INSTALL_UNINSTALL;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("userpath")) == 0) {
                InstallOptions = InstallOptions | YSETUP_INSTALL_USER_PATH;
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

    if (UiToUse == UiDefault) {
        if (StartArg > 0) {
            UiToUse = UiCli;
#if YSETUP_TUI
        } else if (YoriLibIsNanoServer() || YoriLibIsRunningUnderSsh()) {
            UiToUse = UiTui;
#endif
        } else if (SetupGuiInitialize()) {
            UiToUse = UiGui;
        } else {
#if YSETUP_TUI
            UiToUse = UiTui;
#else
            UiToUse = UiCli;
#endif
        }
    } else if (UiToUse == UiGui) {
        if (!SetupGuiInitialize()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ysetup: OS support not present\n"));
        }
    }

    //
    //  Initialize COM for the benefit of shell functions
    //

    YoriLibLoadOle32Functions();
    if (DllOle32.pCoInitialize != NULL) {
        DllOle32.pCoInitialize(NULL);
    }

    //
    //  Shell is needed for a few different things, like the Browse button,
    //  or shortcut creation.  Load it now so we can check if it's there
    //  easily.
    //

    YoriLibLoadShell32Functions();
    YoriLibLoadShfolderFunctions();
    YoriLibLoadCabinetFunctions();
    YoriLibLoadWinInetFunctions();
    if (DllWinInet.hDll == NULL) {
        YoriLibLoadWinHttpFunctions();
    }

    if (UiToUse == UiCli) {
        YORI_STRING ErrorText;
        BOOLEAN LongNameSupport;

        //
        //  Unlike most code, don't use prefix escapes here, since this path
        //  can be passed to create shortcuts etc which can't handle them.
        //

        if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], FALSE, &NewDirectory)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ysetup: install failed\n"));
            return EXIT_FAILURE;
        }

        if (InstallType == InstallTypeDefault) {
            InstallType = InstallTypeTypical;
        }

        LongNameSupport = TRUE;
        if (!YoriLibPathSupportsLongNames(&NewDirectory, &LongNameSupport)) {
            LongNameSupport = TRUE;
        }

        if (!LongNameSupport) {
            LPCSTR LongNameMessage;
            LongNameMessage = SetupGetNoLongFileNamesMessage(InstallOptions);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs\n"), LongNameMessage);
            InstallOptions = InstallOptions & ~(YSETUP_INSTALL_SOURCE | YSETUP_INSTALL_COMPLETION);
        }

        YoriLibInitEmptyString(&ErrorText);

        Result = SetupInstallSelectedWithOptions(&NewDirectory, InstallType, InstallOptions, SetupCliUpdateStatus, NULL, &ErrorText);
        if (Result) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Success: %y\n"), &ErrorText);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Error: %y\n"), &ErrorText);
        }

        YoriLibFreeStringContents(&NewDirectory);
        YoriLibFreeStringContents(&ErrorText);
#if YSETUP_TUI
    } else if (UiToUse == UiTui) {
        SetupTuiDisplayUi();
#endif
    } else {
        SetupGuiDisplayUi();
    }


    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
