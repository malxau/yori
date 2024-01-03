/**
 * @file libdlg/replhex.c
 *
 * Yori shell replace binary dialog
 *
 * Copyright (c) 2019-2023 Malcolm J. Smith
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
 A set of well known control IDs so the dialog can manipulate its elements.
 */
typedef enum _YORI_DLG_REPLACE_HEX_CONTROLS {
    YoriDlgReplaceHexControlFindBuffer = 1,
    YoriDlgReplaceHexControlChangeToBuffer = 2,
    YoriDlgReplaceHexControlBytesPerWord = 3
} YORI_DLG_REPLACE_HEX_CONTROLS;

/**
 A callback invoked when the change one button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgReplaceHexChangeOneButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, 1);
}

/**
 A callback invoked when the change all button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgReplaceHexChangeAllButtonClicked(
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
YoriDlgReplaceHexCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 Convert the specified number of bytes per word into the corresponding combo
 pull down index.

 @param BytesPerWord The bytes per word.

 @return The corresponding combo box index.  On any invalid value, the first
         index (bytes) is used.
 */
UCHAR
YoriDlgReplaceHexBytesPerWordToIndex(
    __in UCHAR BytesPerWord
    )
{
    UCHAR Index;
    UCHAR Value;

    Value = BytesPerWord;
    Index = 0;

    while(Value != 0) {
        if (Value & 1) {
            return Index;
        }

        Value = (UCHAR)(Value>>1);
        Index++;
    }

    return 0;
}

/**
 Convert the specified combo box index into the corresponding bytes per word
 value.

 @param Index The combo box index.

 @return The corresponding bytes per word.
 */
UCHAR
YoriDlgReplaceHexIndexToBytesPerWord(
    __in UCHAR Index
    )
{
    return (UCHAR)(1 << Index);
}

/**
 Return the size of the replace dialog box in characters when displayed on
 the specified window manager.

 @param WinMgrHandle The window manager specifying its size.

 @return The height of the dialog, in characters.
 */
WORD
YoriDlgReplaceHexGetDialogHeight(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    COORD WindowSize;

    if (!YoriWinGetWinMgrDimensions(WinMgrHandle, &WindowSize)) {
        return FALSE;
    }

    //
    //  Minimum window height:
    //   - One line for title bar
    //   - One line for label above first hex edit
    //   - Five lines for hex edit
    //   - One line for label above second hex edit
    //   - Five lines for hex edit
    //   - One line for display as pull down
    //   - Three lines for push buttons
    //   - One line for bottom border
    //

    return 18;
}


/**
 A callback that is invoked when the bytes per word combo box is changed.

 @param Ctrl Pointer to the combo box control.
 */
VOID
YoriDlgReplaceHexBytesPerWordChanged(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_CTRL_HANDLE HexEdit;
    PYORI_WIN_WINDOW_HANDLE Window;
    YORI_ALLOC_SIZE_T Active;
    UCHAR BytesPerWord;

    Parent = YoriWinGetControlParent(Ctrl);
    Window = YoriWinGetWindowFromWindowCtrl(Parent);
    if (!YoriWinComboGetActiveOption(Ctrl, &Active)) {
        return;
    }

    BytesPerWord = YoriDlgReplaceHexIndexToBytesPerWord((UCHAR)Active);
    HexEdit = YoriWinFindControlById(Parent, YoriDlgReplaceHexControlFindBuffer);
    ASSERT(HexEdit != NULL);

    YoriWinHexEditSetBytesPerWord(HexEdit, BytesPerWord);

    HexEdit = YoriWinFindControlById(Parent, YoriDlgReplaceHexControlChangeToBuffer);
    ASSERT(HexEdit != NULL);

    YoriWinHexEditSetBytesPerWord(HexEdit, BytesPerWord);
}

/**
 Display a dialog box for the user to replace a hex buffer with a new hex
 buffer.

 @param WinMgrHandle Pointer to the window manager.

 @param DesiredLeft The desired left offset of the dialog, or -1 to center
        the dialog.

 @param DesiredTop The desired top offset of the dialog, or -1 to center
        the dialog.

 @param Title The string to display in the title of the dialog.

 @param InitialBeforeData Pointer to the initial data to search for.

 @param InitialBeforeDataLength The number of bytes in InitialBeforeData.

 @param InitialAfterData Pointer to the initial data to change any found data
        with.

 @param InitialAfterDataLength The number of bytes in InitialAfterData.

 @param InitialBytesPerWord The number of bytes to display per word.

 @param ReplaceAll On successful completion, populated with TRUE to indicate
        that all instances should be replaced, FALSE if the selected text
        should be replaced if it matches, and if it doesn't, the next match
        should be selected.

 @param OldData On successful completion, updated to point to a buffer
        containing data to find.  Note this value is allocated within this
        routine and should be freed by the caller with
        @ref YoriLibDereference .

 @param OldDataLength On successful completion, updated to contain the
        length of OldData.

 @param NewData On successful completion, updated to point to a buffer
        containing data to change to.  Note this value is allocated within
        this routine and should be freed by the caller with
        @ref YoriLibDereference .

 @param NewDataLength On successful completion, updated to contain the
        length of NewData.

 @return TRUE to indicate that the user successfully entered a search buffer.
         FALSE to indicate a failure or the user cancelling the operation.
 */
__success(return)
BOOLEAN
YoriDlgReplaceHex(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in WORD DesiredLeft,
    __in WORD DesiredTop,
    __in PYORI_STRING Title,
    __in_opt PUCHAR InitialBeforeData,
    __in YORI_ALLOC_SIZE_T InitialBeforeDataLength,
    __in_opt PUCHAR InitialAfterData,
    __in YORI_ALLOC_SIZE_T InitialAfterDataLength,
    __in UCHAR InitialBytesPerWord,
    __out PBOOLEAN ReplaceAll,
    __out PUCHAR * OldData,
    __out PYORI_ALLOC_SIZE_T OldDataLength,
    __out PUCHAR * NewData,
    __out PYORI_ALLOC_SIZE_T NewDataLength
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT Area;
    YORI_STRING Caption;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE HexEditFind;
    PYORI_WIN_CTRL_HANDLE HexEditChangeTo;
    DWORD_PTR Result;
    DWORD HexStyle;
    DWORD WindowStyle;
    DWORD HexEditHeight;
    WORD DialogWidth;
    WORD DialogHeight;
    YORI_STRING BytesPerWordOptions[4];
    PUCHAR InitialDataCopy;
    YORI_ALLOC_SIZE_T InitialDataCopyLength;
    SMALL_RECT WindowRect;

    if (!YoriWinGetWinMgrDimensions(WinMgrHandle, &WindowSize)) {
        return FALSE;
    }

    HexStyle = YORI_WIN_HEX_EDIT_STYLE_VSCROLLBAR;

    //
    //  The dialog wants space for three borders on the left, 16 3 cell
    //  hex values, a space, 16 cell values, and 3 cells of borders on
    //  the right.
    //

    DialogWidth = 3 + 3 * 16 + 1 + 16 + 3;
    HexEditHeight = 5;

    //
    //  If the window manager can also fit an extra 10 cells for 8 chars of
    //  offset, a colon, and a space, include that too.  Otherwise just omit
    //  it so the dialog can fit in an 80 cell terminal.
    //

    if (WindowSize.X >= (WORD)(DialogWidth + 10)) {
        HexStyle = HexStyle | YORI_WIN_HEX_EDIT_STYLE_OFFSET;
        DialogWidth = (WORD)(DialogWidth + 10);
    }

    WindowStyle = YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID;

    DialogHeight = YoriDlgReplaceHexGetDialogHeight(WinMgrHandle);

    if (!YoriWinDetermineWindowRect(WinMgrHandle, DialogWidth, DialogHeight, DialogWidth, DialogHeight, DesiredLeft, DesiredTop, WindowStyle, &WindowRect)) {
        return FALSE;
    }

    if (!YoriWinCreateWindowEx(WinMgrHandle, &WindowRect, WindowStyle, Title, &Parent)) {
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    YoriLibConstantString(&Caption, _T("&Find:"));

    Area.Left = 2;
    Area.Top = 0;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);
    Area.Bottom = 0;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = 1;
    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Right = (WORD)(WindowSize.X - 2);
    Area.Bottom = (WORD)(Area.Top + HexEditHeight - 1);

    YoriLibConstantString(&Caption, _T(""));

    HexEditFind = YoriWinHexEditCreate(Parent, NULL, &Area, InitialBytesPerWord, HexStyle);
    if (HexEditFind == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinSetControlId(HexEditFind, YoriDlgReplaceHexControlFindBuffer);

    YoriLibConstantString(&Caption, _T("&Change To:"));

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
    Area.Bottom = (WORD)(Area.Top + HexEditHeight - 1);

    YoriLibConstantString(&Caption, _T(""));

    HexEditChangeTo = YoriWinHexEditCreate(Parent, NULL, &Area, InitialBytesPerWord, HexStyle);
    if (HexEditFind == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinSetControlId(HexEditChangeTo, YoriDlgReplaceHexControlChangeToBuffer);

    if (InitialBeforeData != NULL) {

        InitialDataCopyLength = YoriLibIsAllocationExtendable(InitialBeforeDataLength, 0, 0x100);

        InitialDataCopy = YoriLibReferencedMalloc(InitialDataCopyLength);
        if (InitialDataCopy == NULL) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        memcpy(InitialDataCopy, InitialBeforeData, InitialBeforeDataLength);
        if (!YoriWinHexEditSetDataNoCopy(HexEditFind, InitialDataCopy, InitialDataCopyLength, InitialBeforeDataLength)) {
            YoriLibDereference(InitialDataCopy);
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        YoriLibDereference(InitialDataCopy);
    }

    if (InitialAfterData != NULL) {
        InitialDataCopyLength = YoriLibIsAllocationExtendable(InitialAfterDataLength, 0, 0x100);

        InitialDataCopy = YoriLibReferencedMalloc(InitialDataCopyLength);
        if (InitialDataCopy == NULL) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        memcpy(InitialDataCopy, InitialAfterData, InitialAfterDataLength);
        if (!YoriWinHexEditSetDataNoCopy(HexEditChangeTo, InitialDataCopy, InitialDataCopyLength, InitialAfterDataLength)) {
            YoriLibDereference(InitialDataCopy);
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        YoriLibDereference(InitialDataCopy);
    }

    YoriLibConstantString(&Caption, _T("&Display As:"));

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Bottom = Area.Top;
    Area.Left = 2;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = (WORD)(Area.Right + 1);
    Area.Right = (WORD)(WindowSize.X - 2);

    YoriLibConstantString(&BytesPerWordOptions[0], _T("Bytes"));
    YoriLibConstantString(&BytesPerWordOptions[1], _T("Words"));
    YoriLibConstantString(&BytesPerWordOptions[2], _T("DWords"));
    YoriLibConstantString(&BytesPerWordOptions[3], _T("QWords"));

    Ctrl = YoriWinComboCreate(Parent, &Area, 4, BytesPerWordOptions, 0, YoriDlgReplaceHexBytesPerWordChanged);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, YoriDlgReplaceHexControlBytesPerWord);

    if (!YoriWinComboAddItems(Ctrl, BytesPerWordOptions, sizeof(BytesPerWordOptions)/sizeof(BytesPerWordOptions[0]))) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }
    YoriWinComboSetActiveOption(Ctrl, YoriDlgReplaceHexBytesPerWordToIndex(InitialBytesPerWord));

    ButtonWidth = (WORD)(12);

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Bottom = (SHORT)(Area.Top + 2);

    YoriLibConstantString(&Caption, _T("Change &One"));

    Area.Left = (SHORT)(1);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, YoriDlgReplaceHexChangeOneButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = (WORD)(Area.Left + ButtonWidth + 3);
    Area.Right = (WORD)(Area.Right + ButtonWidth + 3);

    YoriLibConstantString(&Caption, _T("Change &All"));
    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, 0, YoriDlgReplaceHexChangeAllButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = (WORD)(Area.Left + ButtonWidth + 3);
    Area.Right = (WORD)(Area.Right + ButtonWidth + 3);

    YoriLibConstantString(&Caption, _T("&Cancel"));
    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, YoriDlgReplaceHexCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Result = FALSE;

    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    if (Result) {
        PUCHAR OldBuffer;
        PUCHAR NewBuffer;
        YORI_ALLOC_SIZE_T BufferLength;

        if (!YoriWinHexEditGetDataNoCopy(HexEditFind, &OldBuffer, &BufferLength)) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        *OldData = OldBuffer;
        *OldDataLength = BufferLength;

        if (!YoriWinHexEditGetDataNoCopy(HexEditChangeTo, &NewBuffer, &BufferLength)) {
            YoriLibDereference(OldBuffer);
            *OldData = NULL;
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        *NewData = NewBuffer;
        *NewDataLength = BufferLength;

        YoriWinDestroyWindow(Parent);

        if (Result == 2) {
            *ReplaceAll = TRUE;
        } else {
            *ReplaceAll = FALSE;
        }
        return TRUE;
    }

    YoriWinDestroyWindow(Parent);
    return FALSE;
}

// vim:sw=4:ts=4:et:
