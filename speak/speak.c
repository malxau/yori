/**
 * @file speak/speak.c
 *
 * Yori shell speak text
 *
 * Copyright (c) 2024 Malcolm J. Smith
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
CHAR strSpeakHelpText[] =
        "\n"
        "Outputs text.\n"
        "\n"
        "SPEAK [-license] [--] String\n"
        "\n"
        "   --             Treat all further arguments as display parameters\n";

/**
 Display usage text to the user.
 */
BOOL
SpeakHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Speak %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSpeakHelpText);
    return TRUE;
}

/**
 A declaration for a GUID defining the speech API interface.
 */
const GUID CLSID_SpVoice = { 0x96749377L, 0x3391, 0x11D2, { 0x9E, 0xE3, 0x00, 0xC0, 0x4F, 0x79, 0x73, 0x96 } };

/**
 The speech API voice interface.
 */
const GUID IID_ISpVoice = { 0x6C44DF74L, 0x72B9, 0x4992, { 0xA1, 0xEC, 0xEF, 0x99, 0x6E, 0x04, 0x22, 0xD4 } };

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the speak builtin command.
 */
#define ENTRYPOINT YoriCmd_YSPEAK
#else
/**
 The main entrypoint for the speak standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the speak cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    ISpVoice *pVoice = NULL;
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 1;
    YORI_STRING Arg;
    YORI_STRING Text;
    HRESULT hRes;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SpeakHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2024"));
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

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], FALSE, FALSE, &Text)) {
        return EXIT_FAILURE;
    }

    ASSERT(YoriLibIsStringNullTerminated(&Text));

    YoriLibLoadOle32Functions();
    if (DllOle32.pCoCreateInstance == NULL || DllOle32.pCoInitialize == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("speak: OS support not present\n"));
        YoriLibFreeStringContents(&Text);
        return EXIT_FAILURE;
    }

    hRes = DllOle32.pCoInitialize(NULL);
    if (!SUCCEEDED(hRes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CoInitialize failure: %x\n"), hRes);
        YoriLibFreeStringContents(&Text);
        return EXIT_FAILURE;
    }

    hRes = DllOle32.pCoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER, &IID_ISpVoice, (void **)&pVoice);
    if (!SUCCEEDED(hRes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("speak: OS support not present\n"));
        YoriLibFreeStringContents(&Text);
        return EXIT_FAILURE;
    }

    hRes = pVoice->Vtbl->Speak(pVoice, Text.StartOfString, 0, NULL);
    pVoice->Vtbl->Release(pVoice);
    YoriLibFreeStringContents(&Text);
    if (!SUCCEEDED(hRes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("speak: OS support not present\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
