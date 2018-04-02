/**
 * @file start/start.c
 *
 * Yori shell child process priority tool
 *
 * Copyright (c) 2017 Malcolm J. Smith
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
#include <shellapi.h>

/**
 Help text to display to the user.
 */
const
CHAR strHelpText[] =
        "\n"
        "Ask the shell to open a file.\n"
        "\n"
        "START <file>\n";

/**
 Display usage text to the user.
 */
BOOL
StartHelp()
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2017"), &License);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Start %i.%i\n"), START_VER_MAJOR, START_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &License);
    YoriLibFreeStringContents(&License);
    return TRUE;
}

/**
 Try to launch a single program via ShellExecute rather than CreateProcess.

 @param ArgC Count of arguments.

 @param ArgV Array of arguments.

 @return TRUE to indicate success, FALSE on failure.
 */
BOOL
StartShellExecute(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_STRING Args;
    HINSTANCE hInst;
    LPTSTR ErrText = NULL;
    BOOL AllocatedError = FALSE;

    YoriLibInitEmptyString(&Args);
    if (ArgC > 1) {
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - 1, &ArgV[1], TRUE, &Args)) {
            return FALSE;
        }
        ASSERT(YoriLibIsStringNullTerminated(&Args));
    }

    ASSERT(YoriLibIsStringNullTerminated(&ArgV[0]));
    hInst = ShellExecute(NULL, NULL, ArgV[0].StartOfString, Args.StartOfString, NULL, SW_SHOWNORMAL);

    if ((DWORD_PTR)hInst >= 32) {
        return TRUE;
    }

    switch((DWORD_PTR)hInst) {
        case SE_ERR_ASSOCINCOMPLETE:
            ErrText = _T("The filename association is incomplete or invalid.\n");
            break;
        case SE_ERR_DDEBUSY:
            ErrText = _T("The DDE transaction could not be completed because other DDE transactions were being processed.\n");
            break;
        case SE_ERR_DDEFAIL:
            ErrText = _T("The DDE transaction failed.\n");
            break;
        case SE_ERR_DDETIMEOUT:
            ErrText = _T("The DDE transaction could not be completed because the request timed out.\n");
            break;
        case SE_ERR_NOASSOC:
            ErrText = _T("There is no application associated with the given filename extension.\n");
            break;
        case SE_ERR_SHARE:
            ErrText = _T("A sharing violation occurred.\n");
            break;
         default:
            ErrText = YoriLibGetWinErrorText((DWORD)(DWORD_PTR)hInst);
            AllocatedError = TRUE;
            break;
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("start: execution failed: %s"), ErrText);

    if (AllocatedError) {
        YoriLibFreeWinErrorText(ErrText);
    }

    return FALSE;
}


/**
 The main entrypoint for the start cmdlet.

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
    DWORD StartArg = 0;
    DWORD i;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                StartHelp();
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("start: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (StartShellExecute(ArgC - StartArg, &ArgV[StartArg])) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

// vim:sw=4:ts=4:et:
