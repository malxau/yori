/**
 * @file builtins/color.c
 *
 * Yori shell change console colors
 *
 * Copyright (c) 2017-2022 Malcolm J. Smith
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
        "COLOR -l <file>\n"
        "COLOR -s <file>\n"
        "\n"
        "   -d             Change the default color for the shell\n"
        "   -f             Change all characters on the console\n"
        "   -l             Load colors from a color scheme file\n"
        "   -s             Save colors to a color scheme file\n";

/**
 Display usage text to the user.
 */
BOOL
ColorHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Color %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strColorHelpText);
    return TRUE;
}

/**
 Load colors from a color scheme file.

 @param FileName Pointer to the scheme file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ColorLoadScheme(
    __in PCYORI_STRING FileName
    )
{
    YORI_STRING FullFileName;
    YORI_CONSOLE_SCREEN_BUFFER_INFOEX BufferInfoEx;
    UCHAR Color;

    YoriLibInitEmptyString(&FullFileName);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullFileName)) {
        return FALSE;
    }

    if (DllKernel32.pGetConsoleScreenBufferInfoEx == NULL ||
        DllKernel32.pSetConsoleScreenBufferInfoEx == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: OS support not present\n"));
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    BufferInfoEx.cbSize = sizeof(BufferInfoEx);
    if (!DllKernel32.pGetConsoleScreenBufferInfoEx(GetStdHandle(STD_OUTPUT_HANDLE), &BufferInfoEx)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: GetConsoleScreenBufferEx failed\n"));
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    if (!YoriLibLoadColorTableFromScheme(&FullFileName, BufferInfoEx.ColorTable)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: cannot load scheme\n"));
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    if (!YoriLibLoadWindowColorFromScheme(&FullFileName, &Color)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: cannot load scheme\n"));
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    BufferInfoEx.wAttributes = Color;

    if (!YoriLibLoadPopupColorFromScheme(&FullFileName, &Color)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: cannot load scheme\n"));
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullFileName);

    BufferInfoEx.wPopupAttributes = Color;

    BufferInfoEx.srWindow.Bottom++;
    BufferInfoEx.srWindow.Right++;

    if (!DllKernel32.pSetConsoleScreenBufferInfoEx(GetStdHandle(STD_OUTPUT_HANDLE), &BufferInfoEx)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: SetConsoleScreenBufferEx failed\n"));
        return FALSE;
    }

    YoriCallSetDefaultColor(BufferInfoEx.wAttributes);

    return TRUE;
}

/**
 Save colors to a color scheme file.

 @param FileName Pointer to the scheme file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ColorSaveScheme(
    __in PCYORI_STRING FileName
    )
{
    YORI_STRING FullFileName;
    YORI_CONSOLE_SCREEN_BUFFER_INFOEX BufferInfoEx;

    YoriLibInitEmptyString(&FullFileName);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullFileName)) {
        return FALSE;
    }

    if (DllKernel32.pGetConsoleScreenBufferInfoEx == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: OS support not present\n"));
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    BufferInfoEx.cbSize = sizeof(BufferInfoEx);
    if (!DllKernel32.pGetConsoleScreenBufferInfoEx(GetStdHandle(STD_OUTPUT_HANDLE), &BufferInfoEx)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: GetConsoleScreenBufferEx failed\n"));
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    if (!YoriLibSaveColorTableToScheme(&FullFileName, BufferInfoEx.ColorTable)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: cannot save scheme\n"));
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    if (!YoriLibSavePopupColorToScheme(&FullFileName, (UCHAR)BufferInfoEx.wPopupAttributes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: cannot save scheme\n"));
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    if (!YoriLibSaveWindowColorToScheme(&FullFileName, (UCHAR)BufferInfoEx.wAttributes)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: cannot save scheme\n"));
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullFileName);

    return TRUE;
}

/**
 A set of operations supported by this program.
 */
typedef enum _COLOR_OP {
    ColorOpSetColor = 0,
    ColorOpLoadScheme = 1,
    ColorOpSaveScheme = 2
} COLOR_OP;

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
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
    DWORD CharsWritten;
    WORD OriginalAttributes;
    BOOLEAN ArgumentUnderstood;
    BOOLEAN Fullscreen;
    BOOLEAN Default;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg;
    YORI_STRING Arg;
    PYORI_STRING SchemeFile;
    YORILIB_COLOR_ATTRIBUTES Attributes;
    COLOR_OP Op;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    SchemeFile = NULL;
    Default = FALSE;
    Fullscreen = FALSE;
    StartArg = 0;
    Op = ColorOpSetColor;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ColorHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2022"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                Default = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                Fullscreen = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                if (i + 1 < ArgC) {
                    SchemeFile = &ArgV[i + 1];
                    Op = ColorOpLoadScheme;
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                if (i + 1 < ArgC) {
                    SchemeFile = &ArgV[i + 1];
                    Op = ColorOpSaveScheme;
                    i++;
                    ArgumentUnderstood = TRUE;
                }
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

    if (Op == ColorOpLoadScheme) {
        if (!ColorLoadScheme(SchemeFile)) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    } else if (Op == ColorOpSaveScheme) {
        if (!ColorSaveScheme(SchemeFile)) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &BufferInfo)) {
        HANDLE hConsole;
        ZeroMemory(&BufferInfo, sizeof(BufferInfo));
        hConsole = CreateFile(_T("CONOUT$"),
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);
        if (hConsole != INVALID_HANDLE_VALUE) {
            GetConsoleScreenBufferInfo(hConsole, &BufferInfo);
            CloseHandle(hConsole);
        }
    }

    OriginalAttributes = BufferInfo.wAttributes;
    BufferInfo.wAttributes = 0;
    Attributes.Win32Attr = 0;
    Attributes.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG;
    for (i = 0; i < 2; i++) {
        if (ArgV[StartArg].LengthInChars <= i) {
            break;
        }
        if (i == 0) {
            Attributes.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG;
        } else {
            Attributes.Ctrl = 0;
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

            if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[StartArg], _T("reset")) == 0) {
                Attributes.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG;
            } else {
                YoriLibAttributeFromString(&ArgV[StartArg], &Attributes);
                if (Attributes.Ctrl == (YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG)) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("color: invalid character '%c'\n"), ArgV[StartArg].StartOfString[i]);
                    return EXIT_FAILURE;
                }
            }

            WindowAttributes.Ctrl = 0;
            WindowAttributes.Win32Attr = (UCHAR)OriginalAttributes;
            YoriLibResolveWindowColorComponents(Attributes, WindowAttributes, TRUE, &Attributes);
            BufferInfo.wAttributes = Attributes.Win32Attr;
            break;
        }
    }

    if (Attributes.Ctrl == 0 &&
        (BufferInfo.wAttributes >> 4) == (BufferInfo.wAttributes & 0xf)) {

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

    YoriLibVtSetConsoleTextAttributeOnDevice(GetStdHandle(STD_OUTPUT_HANDLE), 0, Attributes.Ctrl, BufferInfo.wAttributes);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
