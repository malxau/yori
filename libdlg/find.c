/**
 * @file libdlg/find.c
 *
 * Yori shell find text dialog
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
 A callback invoked when the ok button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgFindOkButtonClicked(
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
YoriDlgFindCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 Display a dialog box for the user to search for text.

 @param WinMgrHandle Pointer to the window manager.

 @param Title The string to display in the title of the dialog.

 @param InitialText The initial text to populate into the dialog.

 @param MatchCase On successful completion, populated with TRUE to indicate
        that the search should be case sensitive, or FALSE to indicate the
        search should be case insensitive.

 @param Text On input, specifies an initialized string.  On output, populated
        with the contents of the text that the user entered.

 @return TRUE to indicate that the user successfully entered a string.  FALSE
         to indicate a failure or the user cancelling the operation.
 */
__success(return)
BOOLEAN
YoriDlgFindText(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in PYORI_STRING InitialText,
    __out PBOOLEAN MatchCase,
    __inout PYORI_STRING Text
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT Area;
    YORI_STRING Caption;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE MatchCaseCheckbox;
    PYORI_WIN_CTRL_HANDLE Edit;
    DWORD_PTR Result;

    if (!YoriWinCreateWindow(WinMgrHandle, 50, 13, 70, 13, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, Title, &Parent)) {
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    YoriLibConstantString(&Caption, _T("&Find text:"));

    Area.Left = 2;
    Area.Top = 1;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);
    Area.Bottom = 1;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = 1;
    Area.Top = 2;
    Area.Right = (WORD)(WindowSize.X - 2);
    Area.Bottom = 4;

    YoriLibConstantString(&Caption, _T(""));

    Edit = YoriWinEditCreate(Parent, &Area, &Caption, 0);
    if (Edit == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    if (InitialText->LengthInChars > 0) {
        YoriWinEditSetText(Edit, InitialText);
    }

    Area.Top = (SHORT)(6);
    Area.Bottom = (SHORT)(6);
    Area.Left = (SHORT)(1);
    Area.Right = (WORD)(WindowSize.X - 2);

    YoriLibConstantString(&Caption, _T("&Match Case"));

    MatchCaseCheckbox = YoriWinCheckboxCreate(Parent, &Area, &Caption, 0, NULL);
    if (MatchCaseCheckbox == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    ButtonWidth = (WORD)(8);

    Area.Top = (SHORT)(8);
    Area.Bottom = (SHORT)(10);

    YoriLibConstantString(&Caption, _T("&Ok"));

    Area.Left = (SHORT)(1);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, YoriDlgFindOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = (WORD)(Area.Left + ButtonWidth + 3);
    Area.Right = (WORD)(Area.Right + ButtonWidth + 3);

    YoriLibConstantString(&Caption, _T("&Cancel"));
    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, YoriDlgFindCancelButtonClicked);
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

        *MatchCase = YoriWinCheckboxIsChecked(MatchCaseCheckbox);

        YoriWinDestroyWindow(Parent);
        return TRUE;
    }

    YoriWinDestroyWindow(Parent);
    return FALSE;
}

// vim:sw=4:ts=4:et:
