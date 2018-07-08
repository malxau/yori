/**
 * @file builtins/color.c
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
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strColorHelpText[] =
        "\n"
        "Change the active color or all characters on the console.\n"
        "\n"
        "COLOR [-d] [-f] [-license] <color>\n"
        "\n"
        "   -d             Change the default color for the shell\n"
        "   -f             Change all characters on the console\n";

/**
 Display usage text to the user.
 */
BOOL
ColorHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Color %i.%i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strColorHelpText);
    return TRUE;
}

/**
 The main entrypoint for the color cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_COLOR(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
    DWORD CharsWritten;
    WORD OriginalAttributes;
    BOOL ArgumentUnderstood;
    BOOL Fullscreen;
    BOOL Default;
    DWORD i;
    DWORD StartArg;
    YORI_STRING Arg;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    Default = FALSE;
    Fullscreen = FALSE;
    StartArg = 0;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ColorHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                Default = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                Fullscreen = TRUE;
                ArgumentUnderstood = TRUE;
            }
        } else {
            StartArg = i;
            ArgumentUnderstood = TRUE;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &BufferInfo)) {
        ZeroMemory(&BufferInfo, sizeof(BufferInfo));
    }

    OriginalAttributes = BufferInfo.wAttributes;
    BufferInfo.wAttributes = 0;
    for (i = 0; i < 2; i++) {
        if (ArgV[StartArg].LengthInChars <= i) {
            break;
        }
        BufferInfo.wAttributes = (WORD)(BufferInfo.wAttributes << 4);
        if (ArgV[StartArg].StartOfString[i] >= 'a' && ArgV[StartArg].StartOfString[i] <= 'f') {
            BufferInfo.wAttributes = (WORD)(BufferInfo.wAttributes + ArgV[StartArg].StartOfString[i] - 'a' + 10);
        } else if (ArgV[StartArg].StartOfString[i] >= 'A' && ArgV[StartArg].StartOfString[i] <= 'F') {
            BufferInfo.wAttributes = (WORD)(BufferInfo.wAttributes + ArgV[StartArg].StartOfString[i] - 'A' + 10);
        } else if (ArgV[StartArg].StartOfString[i] >= '0' && ArgV[StartArg].StartOfString[i] <= '9') {
            BufferInfo.wAttributes = (WORD)(BufferInfo.wAttributes + ArgV[StartArg].StartOfString[i] - '0');
        } else {
            YORILIB_COLOR_ATTRIBUTES WindowAttributes;
            YORILIB_COLOR_ATTRIBUTES Attributes;

            Attributes = YoriLibAttributeFromString(ArgV[StartArg].StartOfString);
            if (Attributes.Ctrl == (YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: invalid character '%c'\n"), ArgV[StartArg].StartOfString[i]);
                return EXIT_FAILURE;
            }

            WindowAttributes.Ctrl = 0;
            WindowAttributes.Win32Attr = (UCHAR)OriginalAttributes;
            Attributes = YoriLibResolveWindowColorComponents(Attributes, WindowAttributes, TRUE);
            BufferInfo.wAttributes = Attributes.Win32Attr;
            break;
        }
    }

    if ((BufferInfo.wAttributes >> 4) == (BufferInfo.wAttributes & 0xf)) {
        return EXIT_FAILURE;
    }

    if (Fullscreen) {
        BufferInfo.dwCursorPosition.X = 0;
        BufferInfo.dwCursorPosition.Y = 0;
        
        if (BufferInfo.dwSize.X > 0 && BufferInfo.dwSize.Y > 0) {
            FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                                       BufferInfo.wAttributes,
                                       BufferInfo.dwSize.X * BufferInfo.dwSize.Y,
                                       BufferInfo.dwCursorPosition,
                                       &CharsWritten);
        }
    }

    if (Default) {
        YoriCallSetDefaultColor(BufferInfo.wAttributes);
    }
    
    YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, BufferInfo.wAttributes);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
