/**
 * @file pause/pause.c
 *
 * Yori shell wait for the user to press a key
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, PAUSEESS OR
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
CHAR strPauseHelpText[] =
        "\n"
        "Prompt the user to press any key before continuing.\n"
        "\n"
        "PAUSE [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
PauseHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Pause %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strPauseHelpText);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the pause builtin command.
 */
#define ENTRYPOINT YoriCmd_YPAUSE
#else
/**
 The main entrypoint for the pause standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the pause cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process indicating success or failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    TCHAR Char;
    DWORD BytesRead;
    YORI_ALLOC_SIZE_T i;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                PauseHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Press any key to continue...\n"));
    if (!YoriLibSetInputConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0)) {
        if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), &Char, 1, &BytesRead, NULL)) {
            return EXIT_FAILURE;
        }
    } else {
        INPUT_RECORD InputRecord;

        while(TRUE) {
            if (!ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &InputRecord, 1, &BytesRead)) {
                break;
            }

            if (InputRecord.EventType == KEY_EVENT &&
                InputRecord.Event.KeyEvent.bKeyDown) {
                break;
            }
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
