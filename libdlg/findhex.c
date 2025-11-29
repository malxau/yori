/**
 * @file libdlg/findhex.c
 *
 * Yori shell find binary dialog
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
typedef enum _YORI_DLG_FIND_HEX_CONTROLS {
    YoriDlgFindHexControlBuffer = 1,
    YoriDlgFindHexControlBytesPerWord = 2
} YORI_DLG_FIND_HEX_CONTROLS;

/**
 A callback invoked when the ok button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgFindHexOkButtonClicked(
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
YoriDlgFindHexCancelButtonClicked(
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
YoriDlgFindHexBytesPerWordToIndex(
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
YoriDlgFindHexIndexToBytesPerWord(
    __in UCHAR Index
    )
{
    return (UCHAR)(1 << Index);
}

/**
 A callback that is invoked when the bytes per word combo box is changed.

 @param Ctrl Pointer to the combo box control.
 */
VOID
YoriDlgFindHexBytesPerWordChanged(
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

    BytesPerWord = YoriDlgFindHexIndexToBytesPerWord((UCHAR)Active);
    HexEdit = YoriWinFindControlById(Parent, YoriDlgFindHexControlBuffer);
    ASSERT(HexEdit != NULL);

    YoriWinHexEditSetBytesPerWord(HexEdit, BytesPerWord);
}

/**
 Display a dialog box for the user to search for text.

 @param WinMgrHandle Pointer to the window manager.

 @param Title The string to display in the title of the dialog.

 @param InitialData The initial data to populate into the dialog.

 @param InitialDataLength The size of the initial data, in bytes.

 @param InitialBytesPerWord The number of bytes to display per word.

 @param FindData On successful completion, updated to point to a value to
        find.  Note this value is allocated within this routine and should
        be freed by the caller with @ref YoriLibDereference .

 @param FindDataLength On successful completion, updated to contain the
        length of FindData.

 @return TRUE to indicate that the user successfully entered a search buffer.
         FALSE to indicate a failure or the user cancelling the operation.
 */
__success(return)
BOOLEAN
YoriDlgFindHex(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in_opt PUCHAR InitialData,
    __in YORI_ALLOC_SIZE_T InitialDataLength,
    __in UCHAR InitialBytesPerWord,
    __out PUCHAR * FindData,
    __out PYORI_ALLOC_SIZE_T FindDataLength
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT Area;
    YORI_STRING Caption;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE HexEdit;
    DWORD_PTR Result;
    DWORD Style;
    WORD DialogWidth;
    YORI_STRING BytesPerWordOptions[4];

    if (!YoriWinGetWinMgrDimensions(WinMgrHandle, &WindowSize)) {
        return FALSE;
    }

    Style = YORI_WIN_HEX_EDIT_STYLE_VSCROLLBAR | YORI_WIN_HEX_EDIT_STYLE_VERTICAL_SEPERATOR;

    //
    //  The dialog wants space for three borders on the left, 16 3 cell
    //  hex values, a seperator, a space, 16 cell values, and 3 cells of
    //  borders on the right.
    //

    DialogWidth = 3 + 4 * YORI_LIB_HEXDUMP_BYTES_PER_LINE + 1 + 1 + 3;

    //
    //  If the window manager can also fit an extra 10 cells for 8 chars of
    //  offset, a colon, and a space, include that too.  Otherwise just omit
    //  it so the dialog can fit in an 80 cell terminal.
    //

    if (WindowSize.X >= (WORD)(DialogWidth + 10)) {
        Style = Style | YORI_WIN_HEX_EDIT_STYLE_OFFSET;
        DialogWidth = (WORD)(DialogWidth + 10);
    }

    if (!YoriWinCreateWindow(WinMgrHandle, DialogWidth, 15, DialogWidth, 15, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, Title, &Parent)) {
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
    Area.Bottom = (WORD)(WindowSize.Y - 5);

    YoriLibConstantString(&Caption, _T(""));

    HexEdit = YoriWinHexEditCreate(Parent, NULL, &Area, InitialBytesPerWord, Style);
    if (HexEdit == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinSetControlId(HexEdit, YoriDlgFindHexControlBuffer);

    if (InitialData != NULL) {
        PUCHAR InitialDataCopy;
        YORI_ALLOC_SIZE_T InitialDataCopyLength;

        InitialDataCopyLength = YoriLibIsAllocationExtendable(InitialDataLength, 0, 0x100);
        InitialDataCopy = YoriLibReferencedMalloc(InitialDataCopyLength);
        if (InitialDataCopy == NULL) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        memcpy(InitialDataCopy, InitialData, InitialDataLength);
        if (!YoriWinHexEditSetDataNoCopy(HexEdit, InitialDataCopy, InitialDataCopyLength, InitialDataLength)) {
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

    Ctrl = YoriWinComboCreate(Parent, &Area, 4, BytesPerWordOptions, 0, YoriDlgFindHexBytesPerWordChanged);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, YoriDlgFindHexControlBytesPerWord);

    if (!YoriWinComboAddItems(Ctrl, BytesPerWordOptions, sizeof(BytesPerWordOptions)/sizeof(BytesPerWordOptions[0]))) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }
    YoriWinComboSetActiveOption(Ctrl, YoriDlgFindHexBytesPerWordToIndex(InitialBytesPerWord));

    ButtonWidth = (WORD)(8);

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Bottom = (SHORT)(Area.Top + 2);

    YoriLibConstantString(&Caption, _T("&Ok"));

    Area.Left = (SHORT)(1);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, YoriDlgFindHexOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = (WORD)(Area.Left + ButtonWidth + 3);
    Area.Right = (WORD)(Area.Right + ButtonWidth + 3);

    YoriLibConstantString(&Caption, _T("&Cancel"));
    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, YoriDlgFindHexCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Result = FALSE;

    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    if (Result) {
        PUCHAR Buffer;
        YORI_ALLOC_SIZE_T BufferLength;

        if (!YoriWinHexEditGetDataNoCopy(HexEdit, &Buffer, &BufferLength)) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        *FindData = Buffer;
        *FindDataLength = BufferLength;

        YoriWinDestroyWindow(Parent);
        return TRUE;
    }

    YoriWinDestroyWindow(Parent);
    return FALSE;
}

// vim:sw=4:ts=4:et:
