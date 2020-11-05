/**
 * @file edit/about.c
 *
 * Edit about dialog
 *
 * Copyright (c) 2019-2020 Malcolm J. Smith
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
#include <yoriwin.h>

/**
 A callback invoked when a button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
EditAboutDlgMsgButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, 0);
}

/**
 Display a dialog box to display edit about information.

 @param WinMgrHandle Pointer to the window manager.

 @param Title The string to display in the title of the dialog.

 @param CenteredText The string to display at the top of the dialog.

 @param LeftText The string to display after the centered text in the dialog.

 @return Meaningless for this dialog.
 */
__success(return)
DWORD
EditAboutDialog(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in PYORI_STRING CenteredText,
    __in PYORI_STRING LeftText
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT TextArea;
    SMALL_RECT ButtonArea;
    YORI_STRING ButtonText;
    DWORD DisplayLength;
    DWORD TotalButtonWidth;
    DWORD CenteredLabelLinesRequired;
    DWORD CenteredLabelWidthRequired;
    DWORD LeftLabelLinesRequired;
    DWORD LeftLabelWidthRequired;
    DWORD Style;
    WORD WindowWidth;
    WORD WindowHeight;
    PYORI_WIN_CTRL_HANDLE Ctrl;

    UNREFERENCED_PARAMETER(LeftText);

    YoriWinGetWinMgrDimensions(WinMgrHandle, &WindowSize);

    //
    //  The window decoration will take six characters (two border, two
    //  shadow, two padding between label text and window.)  We take a
    //  few extra characters off just for visual reasons.
    //

    DisplayLength = WindowSize.X - 10;
    CenteredLabelLinesRequired = YoriWinLabelCountLinesRequiredForText(CenteredText, DisplayLength, &CenteredLabelWidthRequired);
    LeftLabelLinesRequired = YoriWinLabelCountLinesRequiredForText(LeftText, DisplayLength, &LeftLabelWidthRequired);

    //
    //  Vertically, the window has 9 lines of overhead (title bar, padding
    //  above text, padding between texts, padding below text, three lines of
    //  buttons, border, and shadow.)
    //

    if (CenteredLabelLinesRequired + LeftLabelLinesRequired + 9 > (WORD)WindowSize.Y &&
        WindowSize.Y > 9) {

        LeftLabelLinesRequired = 0;
        LeftLabelWidthRequired = 0;
    }

    if (LeftLabelWidthRequired > CenteredLabelWidthRequired) {
        WindowWidth = (WORD)(LeftLabelWidthRequired + 6);
    } else {
        WindowWidth = (WORD)(CenteredLabelWidthRequired + 6);
    }

    if (LeftLabelLinesRequired == 0) {
        WindowHeight = (WORD)(CenteredLabelLinesRequired + 8);
    } else {
        WindowHeight = (WORD)(CenteredLabelLinesRequired + LeftLabelLinesRequired + 9);
    }

    if (!YoriWinCreateWindow(WinMgrHandle, WindowWidth, WindowHeight, WindowWidth, WindowHeight, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, Title, &Parent)) {
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    TextArea.Left = 1;
    TextArea.Top = 1;
    TextArea.Right = (WORD)(WindowSize.X - 2);
    TextArea.Bottom = (SHORT)(TextArea.Top + CenteredLabelLinesRequired - 1);

    Ctrl = YoriWinLabelCreate(Parent, &TextArea, CenteredText, YORI_WIN_LABEL_STYLE_CENTER | YORI_WIN_LABEL_NO_ACCELERATOR);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    if (LeftLabelLinesRequired > 0) {
        TextArea.Top = (WORD)(TextArea.Bottom + 2);
        TextArea.Bottom = (SHORT)(TextArea.Top + LeftLabelLinesRequired - 1);

        Ctrl = YoriWinLabelCreate(Parent, &TextArea, LeftText, YORI_WIN_LABEL_NO_ACCELERATOR);
        if (Ctrl == NULL) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }
    }

    //
    //  Each button also has a space before and after the text, a border
    //  left and right.
    //

    TotalButtonWidth = 4 + sizeof("Ok") - 1;

    YoriLibConstantString(&ButtonText, _T("&Ok"));

    ButtonArea.Top = (SHORT)(TextArea.Bottom + 2);
    ButtonArea.Bottom = (SHORT)(ButtonArea.Top + 2);
    ButtonArea.Left = (SHORT)((WindowSize.X - TotalButtonWidth) / 2);

    YoriWinLabelParseAccelerator(&ButtonText,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &DisplayLength);

    ButtonArea.Right = (SHORT)(ButtonArea.Left + 3 + DisplayLength);

    Style = YORI_WIN_BUTTON_STYLE_DEFAULT | YORI_WIN_BUTTON_STYLE_CANCEL;

    Ctrl = YoriWinButtonCreate(Parent, &ButtonArea, &ButtonText, Style, EditAboutDlgMsgButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinEnableNonAltAccelerators(Parent, TRUE);
    YoriWinProcessInputForWindow(Parent, NULL);
    YoriWinDestroyWindow(Parent);
    return 0;
}

// vim:sw=4:ts=4:et:
