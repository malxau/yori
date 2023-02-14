/**
 * @file libdlg/input.c
 *
 * Yori shell generic input string dialog
 *
 * Copyright (c) 2019 Malcolm J. Smith
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
 A callback invoked when the ok button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgInputOkButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, TRUE);
}

/**
 A callback invoked when the cancel button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgInputCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 Display a dialog box for the user to enter a generic string.

 @param WinMgrHandle Pointer to the window manager.

 @param Title The string to display in the title of the dialog.

 @param RequireNumeric If TRUE, the string must be a valid number.  If FALSE,
        the string can be anything.

 @param Text On input, specifies an initialized string.  On output, populated
        with the contents of the text that the user entered.

 @return TRUE to indicate that the user successfully entered a string.  FALSE
         to indicate a failure or the user cancelling the operation.
 */
__success(return)
BOOLEAN
YoriDlgInput(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in BOOLEAN RequireNumeric,
    __inout PYORI_STRING Text
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT EditArea;
    SMALL_RECT ButtonArea;
    YORI_STRING Caption;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE Edit;
    DWORD Style;
    DWORD_PTR Result;

    if (!YoriWinCreateWindow(WinMgrHandle, 50, 10, 70, 10, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, Title, &Parent)) {
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    EditArea.Left = 1;
    EditArea.Top = 1;
    EditArea.Right = (WORD)(WindowSize.X - 2);
    EditArea.Bottom = 3;

    YoriLibConstantString(&Caption, _T(""));
    Style = 0;
    if (RequireNumeric) {
        Style = Style | YORI_WIN_EDIT_STYLE_NUMERIC;
    }

    Edit = YoriWinEditCreate(Parent, &EditArea, &Caption, Style);
    if (Edit == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    ButtonWidth = (WORD)(8);

    ButtonArea.Top = (SHORT)(5);
    ButtonArea.Bottom = (SHORT)(7);

    YoriLibConstantString(&Caption, _T("&Ok"));

    ButtonArea.Left = (SHORT)(1);
    ButtonArea.Right = (WORD)(ButtonArea.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &ButtonArea, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, YoriDlgInputOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    ButtonArea.Left = (WORD)(ButtonArea.Left + ButtonWidth + 3);
    ButtonArea.Right = (WORD)(ButtonArea.Right + ButtonWidth + 3);

    YoriLibConstantString(&Caption, _T("&Cancel"));
    Ctrl = YoriWinButtonCreate(Parent, &ButtonArea, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, YoriDlgInputCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    if (Result) {
        if (!YoriWinEditGetText(Edit, Text)) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }
        YoriWinDestroyWindow(Parent);
        return TRUE;
    }

    YoriWinDestroyWindow(Parent);
    return FALSE;
}

// vim:sw=4:ts=4:et:
