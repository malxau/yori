/**
 * @file regedit/numedit.c
 *
 * Yori shell registry editor create or edit numeric values
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
 A set of well known control IDs so the dialog can manipulate its elements.
 */
typedef enum _REGEDIT_NUMEDIT_CONTROLS {
    RegeditNumeditControlValue = 1,
    RegeditNumeditControlHexadecimal = 2,
    RegeditNumeditControlDecimal = 3
} REGEDIT_NUMEDIT_CONTROLS;

/**
 Context structure attached to the numeric edit dialog.
 */
typedef struct _REGEDIT_NUMEDIT_CONTEXT {

    /**
     Records the current number base used to display the number value.
     This is stored because when the user selects a radio button, the new
     number base is determined, but the old number base cannot be determined
     from the radio button state, so is saved here.
     */
    WORD CurrentBase;
} REGEDIT_NUMEDIT_CONTEXT, *PREGEDIT_NUMEDIT_CONTEXT;

/**
 Query the numeric value from the dialog.

 @param Parent Pointer to the control describing the dialog.

 @param ForceBase If zero, the number base for the current edit control is
        determined from the current state of the radio buttons.  If nonzero,
        this parameter indicates how to interpret the edit control.

 @param NumberValue On successful completion, updated to indicate the current
        numeric value in the edit control.

 @param Base If specified, on successful completion, updated to contain the
        number base of the edit control.

 @return TRUE to indicate success, FALSE to indicate failure.  On failure,
         this function displays the message box indicating the value is not
         numeric.
 */
__success(return)
BOOLEAN
RegeditNumEditGetNumberFromDialog(
    __in PYORI_WIN_CTRL_HANDLE Parent,
    __in WORD ForceBase,
    __out PYORI_MAX_SIGNED_T NumberValue,
    __out_opt PWORD Base
    )
{
    PYORI_WIN_CTRL_HANDLE ValueEdit;
    PYORI_WIN_CTRL_HANDLE Radio;
    YORI_STRING ValueText;
    YORI_MAX_SIGNED_T NewNumberValue;
    YORI_ALLOC_SIZE_T CharsConsumed;
    WORD NewBase;

    ValueEdit = YoriWinFindControlById(Parent, RegeditNumeditControlValue);
    ASSERT(ValueEdit != NULL);
    __analysis_assume(ValueEdit != NULL);

    YoriLibInitEmptyString(&ValueText);

    if (ForceBase != 0) {
        NewBase = ForceBase;
    } else {
        Radio = YoriWinFindControlById(Parent, RegeditNumeditControlHexadecimal);
        ASSERT(Radio != NULL);
        __analysis_assume(Radio != NULL);

        NewBase = 10;
        if (YoriWinRadioIsSelected(Radio)) {
            NewBase = 16;
        }
    }

    if (!YoriWinEditGetText(ValueEdit, &ValueText) ||
        !YoriLibStringToNumberBase(&ValueText, NewBase, TRUE, &NewNumberValue, &CharsConsumed) ||
        CharsConsumed == 0) {

        PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;
        YORI_STRING ButtonText[1];
        YORI_STRING Text;
        YORI_STRING Caption;

        YoriLibFreeStringContents(&ValueText);

        WinMgr = YoriWinGetWindowManagerHandle(YoriWinGetWindowFromWindowCtrl(Parent));

        YoriLibConstantString(&Caption, _T("Error"));
        YoriLibConstantString(&ButtonText[0], _T("&Ok"));
        YoriLibConstantString(&Text, _T("Value is not numeric."));
        YoriDlgMessageBox(WinMgr, &Caption, &Text, 1, ButtonText, 0, 0);

        return FALSE;
    }

    YoriLibFreeStringContents(&ValueText);
    *NumberValue = NewNumberValue;
    if (Base != NULL) {
        *Base = NewBase;
    }
    return TRUE;
}


/**
 Callback invoked when the ok button is clicked.  This closes the dialog
 while indicating that changes should be applied.

 @param Ctrl Pointer to the cancel button control.
 */
VOID
RegeditNumEditOkButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    YORI_MAX_SIGNED_T NewNumberValue;

    Parent = YoriWinGetControlParent(Ctrl);

    //
    //  Query the value and throw it away.  This is just to generate a dialog
    //  if it's not numeric
    //

    if (RegeditNumEditGetNumberFromDialog(Parent, 0, &NewNumberValue, NULL)) {
        YoriWinCloseWindow(Parent, TRUE);
    }
}

/**
 Callback invoked when the cancel button is clicked.  This closes the dialog
 while indicating that changes should not be applied.

 @param Ctrl Pointer to the cancel button control.
 */
VOID
RegeditNumEditCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 Update the edit control to be expressed in a new number base.

 @param Ctrl Pointer to the control describing the numeric edit dialog.

 @param NewBase Specifies the new number base to use to populate the edit
        control.
 */
VOID
RegeditNumEditChangeNumberBase(
    __in PYORI_WIN_CTRL_HANDLE Ctrl,
    __in WORD NewBase
    )
{
    PREGEDIT_NUMEDIT_CONTEXT RegeditNumeditContext;
    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_CTRL_HANDLE ValueEdit;
    YORI_STRING Value;
    YORI_MAX_SIGNED_T NumberValue;
    WORD Base;

    Parent = YoriWinGetControlParent(Ctrl);
    RegeditNumeditContext = YoriWinGetControlContext(Parent);

    if (RegeditNumeditContext->CurrentBase == NewBase) {
        return;
    }

    ValueEdit = YoriWinFindControlById(Parent, RegeditNumeditControlValue);
    ASSERT(ValueEdit != NULL);
    __analysis_assume(ValueEdit != NULL);

    if (RegeditNumEditGetNumberFromDialog(Parent, RegeditNumeditContext->CurrentBase, &NumberValue, &Base)) {
        YoriLibInitEmptyString(&Value);
        if (YoriLibNumberToString(&Value, NumberValue, NewBase, 0, ' ')) {
            if (YoriWinEditSetText(ValueEdit, &Value)) {
                RegeditNumeditContext->CurrentBase = NewBase;
            }
        }
        YoriLibFreeStringContents(&Value);
    }
}

/**
 Callback invoked when the decimal radio button is clicked.

 @param Ctrl Pointer to the radio control.
 */
VOID
RegeditNumEditDecimalRadioClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    RegeditNumEditChangeNumberBase(Ctrl, 10);
}

/**
 Callback invoked when the hexadecimal radio button is clicked.

 @param Ctrl Pointer to the radio control.
 */
VOID
RegeditNumEditHexadecimalRadioClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    RegeditNumEditChangeNumberBase(Ctrl, 16);
}

/**
 Display a registry editor numeric editor window.

 @param RegeditContext Pointer to the registry editor global context.

 @param WinMgr Pointer to the window manager.

 @param ValueName String containing the name of the value.  The key is
        contained within RegeditContext.

 @param ValueNameReadOnly If TRUE, the value name cannot be changed.  This
        occurs when editing an existing value.  If FALSE, the value can be
        changed, which happens when a new value is created.

 @param Value On input, contains the current numeric value.  On output,
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
RegeditEditNumericValue(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr,
    __inout PYORI_STRING ValueName,
    __in BOOLEAN ValueNameReadOnly,
    __inout PYORI_MAX_UNSIGNED_T Value,
    __in BOOLEAN ValueReadOnly
    )
{
    REGEDIT_NUMEDIT_CONTEXT RegeditNumeditContext;
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT Area;
    YORI_STRING Caption;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE ValueEdit;
    PYORI_WIN_CTRL_HANDLE ValueNameEdit;
    YORI_STRING NewValue;
    YORI_MAX_SIGNED_T NewNumberValue;
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
    WindowSize.Y = 12;

    if (ValueReadOnly) {
        YoriLibConstantString(&Caption, _T("View Numeric Value"));
        WindowSize.Y = (WORD)(WindowSize.Y + 3);
    } else {
        YoriLibConstantString(&Caption, _T("Edit Numeric Value"));
    }

    if (!YoriWinCreateWindow(WinMgr, WindowSize.X, WindowSize.Y, WindowSize.X, WindowSize.Y, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_TRANSPARENT, &Caption, &Parent)) {
        return FALSE;
    }

    RegeditNumeditContext.CurrentBase = 10;
    YoriWinSetControlContext(Parent, &RegeditNumeditContext);
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
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars);
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
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    NewNumberValue = *(PYORI_MAX_SIGNED_T)Value;
    YoriLibInitEmptyString(&NewValue);
    YoriLibNumberToString(&NewValue, NewNumberValue, 10, 0, ' ');

    Area.Left = (WORD)(Area.Right + 1);
    Area.Right = (WORD)(WindowSize.X - 2);
    Area.Top = (WORD)(Area.Bottom - 1);
    Area.Bottom = (WORD)(Area.Top + 2);

    ValueEdit = YoriWinEditCreate(Parent, &Area, &NewValue, ValueReadOnly?YORI_WIN_EDIT_STYLE_READ_ONLY:0);
    if (ValueEdit == NULL) {
        YoriLibFreeStringContents(&NewValue);
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }
    YoriWinSetControlId(ValueEdit, RegeditNumeditControlValue);
    if (ValueNameReadOnly) {
        YoriWinSetFocus(Parent, ValueEdit);
    }
    YoriWinEditSetSelectionRange(ValueEdit, 0, NewValue.LengthInChars);
    YoriLibFreeStringContents(&NewValue);

    ButtonWidth = 4 + sizeof("Hexadecimal") + 4 + sizeof("Decimal");

    YoriLibConstantString(&Caption, _T("&Hexadecimal"));

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Bottom = Area.Top;
    Area.Left = (WORD)((WindowSize.X - ButtonWidth) / 2);
    Area.Right = (WORD)(Area.Left + 4 + Caption.LengthInChars - 1);

    Ctrl = YoriWinRadioCreate(Parent, &Area, &Caption, NULL, 0, RegeditNumEditHexadecimalRadioClicked);
    if (Ctrl == NULL) {
        YoriLibFreeStringContents(&NewValue);
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, RegeditNumeditControlHexadecimal);

    YoriLibConstantString(&Caption, _T("&Decimal"));

    Area.Left = (WORD)(Area.Right + 3);
    Area.Right = (WORD)(Area.Left + 4 + Caption.LengthInChars - 1);

    Ctrl = YoriWinRadioCreate(Parent, &Area, &Caption, Ctrl, 0, RegeditNumEditDecimalRadioClicked);
    if (Ctrl == NULL) {
        YoriLibFreeStringContents(&NewValue);
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, RegeditNumeditControlDecimal);
    YoriWinRadioSelect(Ctrl);

    ButtonWidth = 8;

    YoriLibConstantString(&Caption, _T("&Ok"));

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Left = 1;
    Area.Bottom = (WORD)(Area.Top + 2);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT | YORI_WIN_BUTTON_STYLE_CANCEL, RegeditNumEditOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("&Cancel"));

    Area.Left = (WORD)(Area.Right + 2);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, RegeditNumEditCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    if (Result) {
        YORI_STRING NewValueName;

        YoriLibInitEmptyString(&NewValueName);

        if (!ValueNameReadOnly) {
            if (!YoriWinEditGetText(ValueNameEdit, &NewValueName)) {
                Result = FALSE;
            }
        }

        if (Result) {
            if (RegeditNumEditGetNumberFromDialog(Parent, 0, &NewNumberValue, NULL)) {
                *Value = (YORI_MAX_UNSIGNED_T)NewNumberValue;
                if (!ValueNameReadOnly) {
                    YoriLibFreeStringContents(ValueName);
                    YoriLibCloneString(ValueName, &NewValueName);
                    YoriLibFreeStringContents(&NewValueName);
                }
            }
        }
    }

    YoriWinDestroyWindow(Parent);
    return (BOOLEAN)Result;
}

// vim:sw=4:ts=4:et:
