/**
 * @file regedit/stredit.c
 *
 * Yori shell registry editor create or edit string values
 *
 * Copyright (c) 2019-2022 Malcolm J. Smith
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
#include <yoridlg.h>
#include "regedit.h"

/**
 Callback invoked when the ok button is clicked.  This closes the dialog
 while indicating that changes should be applied.

 @param Ctrl Pointer to the cancel button control.
 */
VOID
RegeditStrEditOkButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, TRUE);
}

/**
 Callback invoked when the cancel button is clicked.  This closes the dialog
 while indicating that changes should not be applied.

 @param Ctrl Pointer to the cancel button control.
 */
VOID
RegeditStrEditCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 Display a registry editor string editor window.

 @param RegeditContext Pointer to the registry editor global context.

 @param WinMgr Pointer to the window manager.

 @param ValueName String containing the name of the value.  The key is
        contained within RegeditContext.  If ValueNameReadOnly is FALSE,
        this is updated and returned to the caller on output to contain
        the new value name.

 @param ValueNameReadOnly If TRUE, the value name cannot be changed.  This
        occurs when editing an existing value.  If FALSE, the value can be
        changed, which happens when a new value is created.

 @param Value On input, contains the current string value.  On output,
        updated to contain the new value.

 @param ValueReadOnly If TRUE, the value cannot be changed.  This occurs when
        the caller does not have permission to modify the registry key but
        can still view contents.  If FALSE, the value can be changed.

 @return TRUE to indicate that the window was successfully created and the
         user updated the value.  FALSE to indicate that a failure occurred
         or the value should not be changed.
 */
__success(return)
BOOLEAN
RegeditEditStringValue(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr,
    __inout PYORI_STRING ValueName,
    __in BOOLEAN ValueNameReadOnly,
    __inout PYORI_STRING Value,
    __in BOOLEAN ValueReadOnly
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT Area;
    YORI_STRING Caption;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE ValueNameEdit;
    PYORI_WIN_CTRL_HANDLE ValueEdit;
    DWORD_PTR Result;
    COORD WinMgrSize;
    DWORD ButtonWidth;

    UNREFERENCED_PARAMETER(RegeditContext);

    if (!YoriWinGetWinMgrDimensions(WinMgr, &WinMgrSize)) {
        return FALSE;
    }

    if (WinMgrSize.X < 60 || WinMgrSize.Y < 20) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("regedit: window size too small\n"));
        return FALSE;
    }

    WindowSize.X = (WORD)(WinMgrSize.X - 10);
    WindowSize.Y = 11;

    if (ValueReadOnly) {
        YoriLibConstantString(&Caption, _T("View String Value"));
        WindowSize.Y = (WORD)(WindowSize.Y + 3);
    } else {
        YoriLibConstantString(&Caption, _T("Edit String Value"));
    }

    if (!YoriWinCreateWindow(WinMgr, WindowSize.X, WindowSize.Y, WindowSize.X, WindowSize.Y, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_TRANSPARENT, &Caption, &Parent)) {
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    Area.Top = 0;
    Area.Left = 1;

    if (ValueReadOnly) {
        Area.Left = 1;
        Area.Top = 1;
        Area.Right = (WORD)(WindowSize.X - 2);
        Area.Bottom = Area.Top;

        YoriLibConstantString(&Caption, _T("You do not have access to change this value."));

        Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
        if (Ctrl == NULL) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        //
        //  The label below is one row below the top of the edit control
        //  that follows, so it needs to be two rows below this label.
        //

        Area.Top = (WORD)(Area.Top + 2);
    }

    YoriLibConstantString(&Caption, _T("&Name:"));

    Area.Left = 1;
    Area.Top = (WORD)(Area.Top + 1);
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars + 1);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = (WORD)(Area.Right + 1);
    Area.Right = (WORD)(WindowSize.X - 2);
    Area.Top = (WORD)(Area.Top - 1);
    Area.Bottom = (WORD)(Area.Top + 2);

    ValueNameEdit = YoriWinEditCreate(Parent, &Area, ValueName, ValueNameReadOnly?YORI_WIN_EDIT_STYLE_READ_ONLY:0);
    if (ValueNameEdit == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("&Value:"));

    Area.Left = 1;
    Area.Top = (WORD)(Area.Bottom + 2);
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T(""));

    Area.Left = (WORD)(Area.Right + 1);
    Area.Right = (WORD)(WindowSize.X - 2);
    Area.Top = (WORD)(Area.Bottom - 1);
    Area.Bottom = (WORD)(Area.Top + 2);

    ValueEdit = YoriWinEditCreate(Parent, &Area, Value, ValueReadOnly?YORI_WIN_EDIT_STYLE_READ_ONLY:0);
    if (ValueEdit == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }
    if (ValueNameReadOnly) {
        YoriWinSetFocus(Parent, ValueEdit);
    }
    YoriWinEditSetSelectionRange(ValueEdit, 0, Value->LengthInChars);

    ButtonWidth = 8;

    YoriLibConstantString(&Caption, _T("&Ok"));

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Left = 1;
    Area.Bottom = (WORD)(Area.Top + 2);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT | YORI_WIN_BUTTON_STYLE_CANCEL, RegeditStrEditOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("&Cancel"));

    Area.Left = (WORD)(Area.Right + 2);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, RegeditStrEditCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    if (Result) {
        YORI_STRING NewValue;
        YORI_STRING NewValueName;

        YoriLibInitEmptyString(&NewValueName);
        YoriLibInitEmptyString(&NewValue);

        if (!ValueNameReadOnly) {
            if (!YoriWinEditGetText(ValueNameEdit, &NewValueName)) {
                Result = FALSE;
            }
        }

        if (Result && YoriWinEditGetText(ValueEdit, &NewValue)) {
            YoriLibFreeStringContents(Value);
            YoriLibCloneString(Value, &NewValue);
            YoriLibFreeStringContents(&NewValue);
            if (!ValueNameReadOnly) {
                YoriLibFreeStringContents(ValueName);
                YoriLibCloneString(ValueName, &NewValueName);
                YoriLibFreeStringContents(&NewValueName);
            }
        } else {
            Result = FALSE;
        }
    }

    YoriWinDestroyWindow(Parent);
    return (BOOLEAN)Result;
}

// vim:sw=4:ts=4:et:
