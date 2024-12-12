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
ClsHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Cls %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
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
#define ENTRYPOINT YoriCmd_YCLS
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
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
    DWORD CharsWritten;
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_STRING Arg;
    HANDLE hConOut;

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                ClsHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
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

    hConOut = GetStdHandle(STD_OUTPUT_HANDLE);

    //
    //  We can't clear the screen of non-console devices
    //

    if (!GetConsoleScreenBufferInfo(hConOut, &BufferInfo)) {
        return EXIT_FAILURE;
    }

    //
    //  If the device supports VT sequences, try to clear the scrollback
    //  buffer
    //

    if (SetConsoleMode(hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT | YORI_LIB_OUTPUT_PASSTHROUGH_VT, _T("\x1b[2J\x1b[3J"));
    }

    //
    //  Now clear all addressable cells
    //

    BufferInfo.dwCursorPosition.X = 0;
    BufferInfo.dwCursorPosition.Y = 0;

    FillConsoleOutputCharacter(hConOut, ' ', (DWORD)BufferInfo.dwSize.X * BufferInfo.dwSize.Y, BufferInfo.dwCursorPosition, &CharsWritten);

    FillConsoleOutputAttribute(hConOut, BufferInfo.wAttributes, (DWORD)BufferInfo.dwSize.X * BufferInfo.dwSize.Y, BufferInfo.dwCursorPosition, &CharsWritten);

    SetConsoleCursorPosition(hConOut, BufferInfo.dwCursorPosition);


    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
