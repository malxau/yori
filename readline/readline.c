/**
 * @file readline/readline.c
 *
 * Yori shell input a line of text
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

#if defined(_MSC_VER) && (_MSC_VER >= 1700)

/*
 On newer SDKs, ReadConsole is annotated with TCHAR% which the
 current version of the compiler does not understand.  Previously
 the annotation differed between *A and *W functions, which was
 perfectly fine.
 */
#pragma warning(disable: 28286)
#endif

#include <yoripch.h>
#include <yorilib.h>

/**
 Help text to display to the user.
 */
const
CHAR strReadLineHelpText[] =
        "\n"
        "Inputs a line and sends it to output.\n"
        "\n"
        "READLINE [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
ReadLineHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ReadLine %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strReadLineHelpText);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the readline builtin command.
 */
#define ENTRYPOINT YoriCmd_READLINE
#else
/**
 The main entrypoint for the readline standalone application.
 */
#define ENTRYPOINT ymain
#endif


/**
 The main entrypoint for the readline cmdlet.

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
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 1;
    DWORD CurrentMode;
    DWORD BytesRead;
    YORI_STRING InputString;
    YORI_STRING Arg;
    HANDLE InputHandle;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ReadLineHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
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

    InputHandle = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(InputHandle, &CurrentMode)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("readline: cannot read from input device\n"));
        return EXIT_FAILURE;
    }

    CurrentMode |= ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT;

    YoriLibSetInputConsoleMode(InputHandle, CurrentMode);

    if (!YoriLibAllocateString(&InputString, 4096)) {
        return EXIT_FAILURE;
    }

    if (!ReadConsole(InputHandle, InputString.StartOfString, InputString.LengthAllocated, &BytesRead, NULL)) {
        return EXIT_FAILURE;
    }
    InputString.LengthInChars = (YORI_ALLOC_SIZE_T)BytesRead;

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &InputString);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
