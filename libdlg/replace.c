/**
 * @file libdlg/find.c
 *
 * Yori shell replace text dialog
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
YoriDlgReplaceChangeOneButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, 1);
}

/**
 A callback invoked when the ok button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgReplaceChangeAllButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, 2);
}

/**
 A callback invoked when the cancel button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgReplaceCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 The smallest possible replace dialog box in characters.
 */
#define YORI_DLG_REPLACE_SMALL_HEIGHT (10)

/**
 The medium replace dialog box in characters.
 */
#define YORI_DLG_REPLACE_MEDIUM_HEIGHT (YORI_DLG_REPLACE_SMALL_HEIGHT + 4)

/**
 The largest possible replace dialog box in characters.
 */
#define YORI_DLG_REPLACE_LARGE_HEIGHT (YORI_DLG_REPLACE_MEDIUM_HEIGHT + 3)

/**
 Return the size of the replace dialog box in characters when displayed on
 the specified window manager.

 @param WinMgrHandle The window manager specifying its size.

 @return The height of the dialog, in characters.
 */
WORD
YoriDlgReplaceGetDialogHeight(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    COORD WindowSize;
    WORD WindowHeight;

    if (!YoriWinGetWinMgrDimensions(WinMgrHandle, &WindowSize)) {
        return FALSE;
    }

    //
    //  Minimum window height:
    //   - One line for title bar
    //   - One line for label above first edit
    //   - One line for edit
    //   - One line for label above second edit
    //   - One line for edit
    //   - One line for checkbox
    //   - Three lines for push buttons
    //   - One line for bottom border
    //

    WindowHeight = YORI_DLG_REPLACE_SMALL_HEIGHT;

    if (WindowSize.Y >= 25 && WindowSize.Y < 30) {

        //
        //  On medium displays, add:
        //   - Two lines for each of two edits, four lines total
        //

        WindowHeight = YORI_DLG_REPLACE_MEDIUM_HEIGHT;

    } else if (WindowSize.Y >= 30) {

        //
        //  On larger displays, add:
        //   - One line of padding between title bar and first label
        //   - One line of padding above checkbox
        //   - One line of padding below checkbox
        //

        WindowHeight = YORI_DLG_REPLACE_LARGE_HEIGHT;
    }

    return WindowHeight;
}

/**
 Display a dialog box for the user to search for text.

 @param WinMgrHandle Pointer to the window manager.

 @param DesiredLeft The desired left offset of the dialog, or -1 to center
        the dialog.

 @param DesiredTop The desired top offset of the dialog, or -1 to center
        the dialog.

 @param Title The string to display in the title of the dialog.

 @param InitialBeforeText The initial text to populate into the before field
        of the dialog.

 @param InitialAfterText The initial text to populate into the after field
        of the dialog.

 @param MatchCase On successful completion, populated with TRUE to indicate
        that the search should be case sensitive, or FALSE to indicate the
        search should be case insensitive.

 @param ReplaceAll On successful completion, populated with TRUE to indicate
        that all instances should be replaced, FALSE if the selected text
        should be replaced if it matches, and if it doesn't, the next match
        should be selected.

 @param BeforeText On input, specifies an initialized string.  On output,
        populated with the contents of the text that the user entered that
        should be replaced.  Note this must not be an empty string for
        successful completion.

 @param AfterText On input, specifies an initialized string.  On output,
        populated with the contents of the text that the user entered that
        should be replace BeforeText.

 @return TRUE to indicate that the user successfully entered strings to
         substitute.  FALSE to indicate a failure or the user cancelling the
         operation.
 */
__success(return)
BOOLEAN
YoriDlgReplaceText(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in WORD DesiredLeft,
    __in WORD DesiredTop,
    __in PYORI_STRING Title,
    __in PYORI_STRING InitialBeforeText,
    __in PYORI_STRING InitialAfterText,
    __out PBOOLEAN MatchCase,
    __out PBOOLEAN ReplaceAll,
    __inout PYORI_STRING BeforeText,
    __inout PYORI_STRING AfterText
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT Area;
    YORI_STRING Caption;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE MatchCaseCheckbox;
    PYORI_WIN_CTRL_HANDLE BeforeEdit;
    PYORI_WIN_CTRL_HANDLE AfterEdit;
    DWORD_PTR Result;
    SMALL_RECT WindowRect;
    DWORD Style;
    WORD DialogHeight;

    Style = YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID;

    DialogHeight = YoriDlgReplaceGetDialogHeight(WinMgrHandle);

    if (!YoriWinDetermineWindowRect(WinMgrHandle, 50, DialogHeight, 70, DialogHeight, DesiredLeft, DesiredTop, Style, &WindowRect)) {
        return FALSE;
    }

    if (!YoriWinCreateWindowEx(WinMgrHandle, &WindowRect, Style, Title, &Parent)) {
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    YoriLibConstantString(&Caption, _T("&Find text:"));

    if (DialogHeight < YORI_DLG_REPLACE_LARGE_HEIGHT) {
        Area.Top = 0;
    } else {
        Area.Top = 1;
    }
    Area.Left = 2;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = 1;
    Area.Top = (WORD)(Area.Top + 1);
    Area.Right = (WORD)(WindowSize.X - 2);
    if (DialogHeight < YORI_DLG_REPLACE_MEDIUM_HEIGHT) {
        Area.Bottom = (WORD)(Area.Top);
    } else {
        Area.Bottom = (WORD)(Area.Top + 2);
    }

    YoriLibConstantString(&Caption, _T(""));

    BeforeEdit = YoriWinEditCreate(Parent, &Area, &Caption, 0);
    if (BeforeEdit == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    if (InitialBeforeText->LengthInChars > 0) {
        YoriWinEditSetText(BeforeEdit, InitialBeforeText);
    }

    YoriLibConstantString(&Caption, _T("Change &To:"));

    Area.Left = 2;
    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = 1;
    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Right = (WORD)(WindowSize.X - 2);
    if (DialogHeight < YORI_DLG_REPLACE_MEDIUM_HEIGHT) {
        Area.Bottom = (WORD)(Area.Top);
    } else {
        Area.Bottom = (WORD)(Area.Top + 2);
    }

    YoriLibConstantString(&Caption, _T(""));

    AfterEdit = YoriWinEditCreate(Parent, &Area, &Caption, 0);
    if (AfterEdit == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    if (InitialAfterText->LengthInChars > 0) {
        YoriWinEditSetText(AfterEdit, InitialAfterText);
    }

    Area.Top = (SHORT)(Area.Bottom + 1);
    if (DialogHeight >= YORI_DLG_REPLACE_LARGE_HEIGHT) {
        Area.Top = (SHORT)(Area.Top + 1);
    }
    Area.Bottom = (SHORT)(Area.Top);
    Area.Left = (SHORT)(1);
    Area.Right = (WORD)(WindowSize.X - 2);

    YoriLibConstantString(&Caption, _T("&Match Case"));

    MatchCaseCheckbox = YoriWinCheckboxCreate(Parent, &Area, &Caption, 0, NULL);
    if (MatchCaseCheckbox == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    ButtonWidth = (WORD)(12);

    Area.Top = (SHORT)(Area.Bottom + 1);
    if (DialogHeight >= YORI_DLG_REPLACE_LARGE_HEIGHT) {
        Area.Top = (SHORT)(Area.Top + 1);
    }
    Area.Bottom = (SHORT)(Area.Top + 2);

    YoriLibConstantString(&Caption, _T("Change &One"));

    Area.Left = (SHORT)(1);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, YoriDlgReplaceChangeOneButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = (WORD)(Area.Left + ButtonWidth + 3);
    Area.Right = (WORD)(Area.Right + ButtonWidth + 3);

    YoriLibConstantString(&Caption, _T("Change &All"));

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, 0, YoriDlgReplaceChangeAllButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("&Cancel"));

    Area.Left = (WORD)(Area.Left + ButtonWidth + 3);
    Area.Right = (WORD)(Area.Right + ButtonWidth + 3);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, YoriDlgReplaceCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    if (Result) {
        if (!YoriWinEditGetText(BeforeEdit, BeforeText)) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        if (!YoriWinEditGetText(AfterEdit, AfterText)) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        if (Result >= 2) {
            *ReplaceAll = TRUE;
        } else {
            *ReplaceAll = FALSE;
        }

        *MatchCase = YoriWinCheckboxIsChecked(MatchCaseCheckbox);

        YoriWinDestroyWindow(Parent);
        return TRUE;
    }

    YoriWinDestroyWindow(Parent);
    return FALSE;
}

// vim:sw=4:ts=4:et:
