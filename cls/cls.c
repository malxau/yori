/**
 * @file cls/cls.c
 *
 * Yori shell display command line output
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
CHAR strClsHelpText[] =
        "\n"
        "Clears the console.\n"
        "\n"
        "CLS [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
ClsHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Cls %i.%i\n"), CLS_VER_MAJOR, CLS_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strClsHelpText);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the cls builtin command.
 */
#define ENTRYPOINT YoriCmd_CLS
#else
/**
 The main entrypoint for the cls standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the cls cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
    DWORD CharsWritten;
    BOOL ArgumentUnderstood;
    DWORD i;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ClsHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            }
        } else {
            ArgumentUnderstood = TRUE;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), ArgV[i]);
        }
    }

    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &BufferInfo)) {
        return EXIT_FAILURE;
    }

    BufferInfo.dwCursorPosition.X = 0;
    BufferInfo.dwCursorPosition.Y = 0;


    FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', (DWORD)BufferInfo.dwSize.X * BufferInfo.dwSize.Y, BufferInfo.dwCursorPosition, &CharsWritten);

    FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BufferInfo.wAttributes, (DWORD)BufferInfo.dwSize.X * BufferInfo.dwSize.Y, BufferInfo.dwCursorPosition, &CharsWritten);

    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), BufferInfo.dwCursorPosition);


    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
