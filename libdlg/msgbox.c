/**
 * @file libdlg/msgbox.c
 *
 * Yori shell message dialog
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
YoriDlgMsgButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    DWORD_PTR CtrlId;
    Parent = YoriWinGetControlParent(Ctrl);
    CtrlId = YoriWinGetControlId(Ctrl);
    YoriWinCloseWindow(Parent, CtrlId);
}

/**
 Display a dialog box to display a message and allow the user to click
 a button

 @param WinMgrHandle Pointer to the window manager.

 @param Title The string to display in the title of the dialog.

 @param Text The string to display in the body of the dialog.

 @param NumButtons The number of buttons to display.

 @param ButtonTexts An array of NumButtons length of Yori strings to display
        in the buttons.

 @param DefaultIndex The zero-based index of the button which should be
        invoked when the user presses enter.

 @param CancelIndex The zero-based index of the button which should be
        invoked when the user presses escape.

 @return Zero to indicate failure.  A nonzero value indicates the element of
         the ButtonTexts array that was clicked, where the first button has
         a value of one.
 */
__success(return)
DWORD
YoriDlgMessageBox(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in PYORI_STRING Text,
    __in DWORD NumButtons,
    __in PYORI_STRING ButtonTexts,
    __in DWORD DefaultIndex,
    __in DWORD CancelIndex
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT TextArea;
    SMALL_RECT ButtonArea;
    DWORD Index;
    DWORD DisplayLength;
    DWORD TotalButtonWidth;
    DWORD LabelLinesRequired;
    DWORD LabelWidthRequired;
    DWORD Style;
    WORD WindowWidth;
    WORD WindowHeight;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    DWORD_PTR Result;

    YoriWinGetWinMgrDimensions(WinMgrHandle, &WindowSize);

    //
    //  The window decoration will take six characters (two border, two
    //  shadow, two padding between label text and window.)  We take a
    //  few extra characters off just for visual reasons.
    //

    DisplayLength = WindowSize.X - 10;
    LabelLinesRequired = YoriWinLabelCountLinesRequiredForText(Text, DisplayLength, &LabelWidthRequired);

    //
    //  Vertically, the window has 8 lines of overhead (title bar, padding
    //  above text, padding below text, three lines of buttons, border, and
    //  shadow.)
    //

    if (LabelLinesRequired + 8 > (WORD)WindowSize.Y &&
        WindowSize.Y > 8) {

        LabelLinesRequired = LabelLinesRequired - WindowSize.Y;
    }

    WindowWidth = (WORD)(LabelWidthRequired + 6);
    WindowHeight = (WORD)(LabelLinesRequired + 8);

    if (!YoriWinCreateWindow(WinMgrHandle, WindowWidth, WindowHeight, WindowWidth, WindowHeight, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, Title, &Parent)) {
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    TextArea.Left = 1;
    TextArea.Top = 1;
    TextArea.Right = (WORD)(WindowSize.X - 2);
    TextArea.Bottom = (SHORT)(TextArea.Top + LabelLinesRequired - 1);

    Ctrl = YoriWinLabelCreate(Parent, &TextArea, Text, YORI_WIN_LABEL_STYLE_CENTER | YORI_WIN_LABEL_NO_ACCELERATOR);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    //
    //  Count the number of characters in the buttons.
    //

    TotalButtonWidth = 0;
    for (Index = 0; Index < NumButtons; Index++) {

        YoriWinLabelParseAccelerator(&ButtonTexts[Index],
                                     NULL,
                                     NULL,
                                     NULL,
                                     &DisplayLength);

        TotalButtonWidth += DisplayLength;
    }

    //
    //  Each button also has a space before and after the text, a border
    //  left and right, and one char between the buttons.
    //

    TotalButtonWidth += 5 * NumButtons - 1;

    ButtonArea.Top = (SHORT)(TextArea.Bottom + 2);
    ButtonArea.Bottom = (SHORT)(ButtonArea.Top + 2);
    ButtonArea.Left = (SHORT)((WindowSize.X - TotalButtonWidth) / 2);

    for (Index = 0; Index < NumButtons; Index++) {

        YoriWinLabelParseAccelerator(&ButtonTexts[Index],
                                     NULL,
                                     NULL,
                                     NULL,
                                     &DisplayLength);

        ButtonArea.Right = (SHORT)(ButtonArea.Left + 3 + DisplayLength);

        Style = 0;

        if (Index == DefaultIndex) {
            Style |= YORI_WIN_BUTTON_STYLE_DEFAULT;
        }

        if (Index == CancelIndex) {
            Style |= YORI_WIN_BUTTON_STYLE_CANCEL;
        }

        Ctrl = YoriWinButtonCreate(Parent, &ButtonArea, &ButtonTexts[Index], Style, YoriDlgMsgButtonClicked);
        if (Ctrl == NULL) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        YoriWinSetControlId(Ctrl, Index + 1);
        ButtonArea.Left = (SHORT)(ButtonArea.Right + 2);
    }

    Result = 0;
    YoriWinEnableNonAltAccelerators(Parent, TRUE);
    YoriWinProcessInputForWindow(Parent, &Result);
    YoriWinDestroyWindow(Parent);
    return (DWORD)Result;
}

// vim:sw=4:ts=4:et:
