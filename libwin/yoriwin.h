/**
 * @file libwin/yoriwin.h
 *
 * Header for control and window toolkit routines that may be of value from
 * the shell as well as external tools.
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

/**
 Opaque pointer to a window manager.
 */
typedef PVOID PYORI_WIN_WINDOW_MANAGER_HANDLE;

/**
 Opaque pointer to a window.
 */
typedef PVOID PYORI_WIN_WINDOW_HANDLE;

/**
 Opaque pointer to a control.
 */
typedef PVOID PYORI_WIN_CTRL_HANDLE;

/**
 A function prototype that can be invoked to deliver notification events
 for a specific control.
 */
typedef VOID YORI_WIN_NOTIFY(PYORI_WIN_CTRL_HANDLE);

/**
 A pointer to a function that can be invoked to deliver notification events
 for a specific control.
 */
typedef YORI_WIN_NOTIFY *PYORI_WIN_NOTIFY;

/**
 A list of possible color tables to use.
 */
typedef enum _YORI_WIN_COLOR_TABLE_ID {
    YoriWinColorTableDefault = 0,
    YoriWinColorTableVga = 1,
    YoriWinColorTableNano = 2,
    YoriWinColorTableMono = 3
} YORI_WIN_COLOR_TABLE_ID;

// BUTTON.C

BOOLEAN
YoriWinButtonReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

/**
 The button is the default button on the window.
 */
#define YORI_WIN_BUTTON_STYLE_DEFAULT (0x0001)

/**
 The button is the cancel button on the window.
 */
#define YORI_WIN_BUTTON_STYLE_CANCEL  (0x0002)

/**
 The button can never receive keyboard focus, but is still functional.
 */
#define YORI_WIN_BUTTON_STYLE_DISABLE_FOCUS  (0x0004)

PYORI_WIN_CTRL_HANDLE
YoriWinButtonCreate(
    __in PYORI_WIN_WINDOW_HANDLE Parent,
    __in PSMALL_RECT Size,
    __in PYORI_STRING Caption,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY ClickCallback
    );

// CHECKBOX.C

BOOLEAN
YoriWinCheckboxIsChecked(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinCheckboxReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

PYORI_WIN_CTRL_HANDLE
YoriWinCheckboxCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in PYORI_STRING Caption,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY ToggleCallback
    );

// COMBO.C

__success(return)
BOOLEAN
YoriWinComboGetActiveOption(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PYORI_ALLOC_SIZE_T CurrentlyActiveIndex
    );

__success(return)
BOOLEAN
YoriWinComboSetActiveOption(
    __inout PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T ActiveOption
    );

__success(return)
BOOLEAN
YoriWinComboAddItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PCYORI_STRING ListOptions,
    __in YORI_ALLOC_SIZE_T NumberOptions
    );

BOOLEAN
YoriWinComboReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

PYORI_WIN_CTRL_HANDLE
YoriWinComboCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in WORD LinesInList,
    __in PYORI_STRING Caption,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY ClickCallback
    );

// CTRL.C

PYORI_WIN_CTRL_HANDLE
YoriWinGetControlParent(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

DWORD_PTR
YoriWinGetControlId(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

VOID
YoriWinSetControlId(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD_PTR CtrlId
    );

PYORI_WIN_CTRL_HANDLE
YoriWinFindControlById(
    __in PYORI_WIN_CTRL_HANDLE ParentCtrl,
    __in DWORD_PTR CtrlId
    );

VOID
YoriWinGetControlClientSize(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PCOORD Size
    );

PVOID
YoriWinGetControlContext(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

VOID
YoriWinSetControlContext(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PVOID Context
    );

BOOLEAN
YoriWinControlSetFocusOnMouseClick(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN ReceiveFocusOnMouseClick
    );

// EDIT.C

/**
 The edit should left align text.
 */
#define YORI_WIN_EDIT_STYLE_LEFT_ALIGN      (0x0000)

/**
 The edit should right align text.
 */
#define YORI_WIN_EDIT_STYLE_RIGHT_ALIGN     (0x0001)

/**
 The edit should center text.
 */
#define YORI_WIN_EDIT_STYLE_CENTER          (0x0002)

/**
 The edit should not allow, uhh, edits.  This allows it to operate like a
 label, but it can still do navigation, and get focus, etc.
 */
#define YORI_WIN_EDIT_STYLE_READ_ONLY       (0x0004)

/**
 The edit should only accept numeric input.
 */
#define YORI_WIN_EDIT_STYLE_NUMERIC         (0x0008)

BOOLEAN
YoriWinEditSelectionActive(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinEditDeleteSelection(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinEditGetSelectedText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PYORI_STRING SelectedText
    );

VOID
YoriWinEditSetSelectionRange(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T StartOffset,
    __in YORI_ALLOC_SIZE_T EndOffset
    );

BOOLEAN
YoriWinEditGetText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __inout PYORI_STRING Text
    );

BOOLEAN
YoriWinEditSetText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING Text
    );

BOOLEAN
YoriWinEditReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

PYORI_WIN_CTRL_HANDLE
YoriWinEditCreate(
    __in PYORI_WIN_CTRL_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in PYORI_STRING InitialText,
    __in DWORD Style
    );

// HEXEDIT.C

/**
 A function prototype that can be invoked to deliver notification events
 when the cursor is moved.
 */
typedef VOID YORI_WIN_NOTIFY_HEX_EDIT_CURSOR_MOVE(PYORI_WIN_CTRL_HANDLE, DWORDLONG, DWORD);

/**
 A pointer to a function that can be invoked to deliver notification events
 when the cursor is moved.
 */
typedef YORI_WIN_NOTIFY_HEX_EDIT_CURSOR_MOVE *PYORI_WIN_NOTIFY_HEX_EDIT_CURSOR_MOVE;

/**
 The hex edit should display a vertical scroll bar.
 */
#define YORI_WIN_HEX_EDIT_STYLE_VSCROLLBAR    (0x0001)

/**
 The hex edit should be read only.
 */
#define YORI_WIN_HEX_EDIT_STYLE_READ_ONLY     (0x0002)

/**
 The hex edit should contain 32 bit offset values.
 */
#define YORI_WIN_HEX_EDIT_STYLE_OFFSET        (0x0004)

/**
 The hex edit should contain 64 bit offset values.
 */
#define YORI_WIN_HEX_EDIT_STYLE_LARGE_OFFSET  (0x0008)

BOOLEAN
YoriWinHexEditClear(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

DWORD
YoriWinHexEditGetBytesPerWord(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinHexEditGetDataNoCopy(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PUCHAR *Buffer,
    __out PYORI_ALLOC_SIZE_T BufferLength
    );

__success(return)
BOOLEAN
YoriWinHexEditGetSelectedData(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PVOID * Data,
    __out PYORI_ALLOC_SIZE_T DataLength
    );

BOOLEAN
YoriWinHexEditDeleteSelection(
    __in PYORI_WIN_CTRL_HANDLE HexEdit
    );

BOOLEAN
YoriWinHexEditGetModifyState(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinHexEditReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

BOOLEAN
YoriWinHexEditSetBytesPerWord(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in UCHAR BytesPerWord
    );

BOOLEAN
YoriWinHexEditSetStyle(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD NewStyle
    );

BOOLEAN
YoriWinHexEditSetCaption(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING Caption
    );

VOID
YoriWinHexEditSetColor(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in WORD Attributes,
    __in WORD SelectedAttributes
    );

BOOLEAN
YoriWinHexEditSetCursorMoveNotifyCallback(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_NOTIFY_HEX_EDIT_CURSOR_MOVE NotifyCallback
    );

BOOLEAN
YoriWinHexEditSetDataNoCopy(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PUCHAR NewBuffer,
    __in YORI_ALLOC_SIZE_T NewBufferAllocated,
    __in YORI_ALLOC_SIZE_T NewBufferValid
    );

BOOLEAN
YoriWinHexEditSetModifyState(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN ModifyState
    );

BOOLEAN
YoriWinHexEditSetReadOnly(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN NewReadOnlyState
    );

__success(return)
BOOLEAN
YoriWinHexEditSetCursorLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN AsChar,
    __in YORI_ALLOC_SIZE_T BufferOffset,
    __in UCHAR BitShift
    );

__success(return)
BOOLEAN
YoriWinHexEditGetCursorLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PBOOLEAN AsChar,
    __out PYORI_ALLOC_SIZE_T BufferOffset,
    __out PUCHAR BitShift
    );

__success(return)
BOOLEAN
YoriWinHexEditGetVisualCursorLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PYORI_ALLOC_SIZE_T CursorOffset,
    __out PYORI_ALLOC_SIZE_T CursorLine
    );

VOID
YoriWinHexEditGetViewportLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PYORI_ALLOC_SIZE_T ViewportLeft,
    __out PYORI_ALLOC_SIZE_T ViewportTop
    );

VOID
YoriWinHexEditSetViewportLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T NewViewportLeft,
    __in YORI_ALLOC_SIZE_T NewViewportTop
    );

VOID
YoriWinHexEditClearSelection(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinHexEditSelectionActive(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

__success(return)
BOOLEAN
YoriWinHexEditSetSelectionRange(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T FirstByteOffset,
    __in YORI_ALLOC_SIZE_T LastByteOffset
    );

__success(return)
BOOLEAN
YoriWinHexEditDeleteData(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T DataOffset,
    __in YORI_ALLOC_SIZE_T Length
    );

__success(return)
BOOLEAN
YoriWinHexEditInsertData(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T DataOffset,
    __in PVOID Data,
    __in YORI_ALLOC_SIZE_T Length
    );

__success(return)
BOOLEAN
YoriWinHexEditReplaceData(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T DataOffset,
    __in PVOID Data,
    __in YORI_ALLOC_SIZE_T Length
    );

BOOLEAN
YoriWinHexEditCutSelectedData(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinHexEditCopySelectedData(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinHexEditPasteData(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

PYORI_WIN_CTRL_HANDLE
YoriWinHexEditCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in_opt PYORI_STRING Caption,
    __in PSMALL_RECT Size,
    __in UCHAR BytesPerWord,
    __in DWORD Style
    );

// LABEL.C

/**
 The label should left align text.
 */
#define YORI_WIN_LABEL_STYLE_LEFT_ALIGN      (0x0000)

/**
 The label should right align text.
 */
#define YORI_WIN_LABEL_STYLE_RIGHT_ALIGN     (0x0001)

/**
 The label should center text.
 */
#define YORI_WIN_LABEL_STYLE_CENTER          (0x0002)

/**
 The label should top align text.
 */
#define YORI_WIN_LABEL_STYLE_TOP_ALIGN       (0x0000)

/**
 The label should bottom align text.
 */
#define YORI_WIN_LABEL_STYLE_BOTTOM_ALIGN    (0x0004)

/**
 The label should vertically center text.
 */
#define YORI_WIN_LABEL_STYLE_VERTICAL_CENTER (0x0008)

/**
 The label should not parse accelerators.
 */
#define YORI_WIN_LABEL_NO_ACCELERATOR        (0x0010)

PYORI_WIN_CTRL_HANDLE
YoriWinLabelCreate(
    __in PYORI_WIN_CTRL_HANDLE Parent,
    __in PSMALL_RECT Size,
    __in PYORI_STRING Caption,
    __in DWORD Style
    );

VOID
YoriWinLabelSetTextAttributes(
    __in PYORI_WIN_CTRL_HANDLE Ctrl,
    __in WORD TextAttributes
    );

VOID
YoriWinLabelParseAccelerator(
    __in PCYORI_STRING RawString,
    __inout_opt PYORI_STRING ParsedString,
    __out_opt TCHAR* AcceleratorChar,
    __out_opt PYORI_ALLOC_SIZE_T HighlightOffset,
    __out_opt PYORI_ALLOC_SIZE_T DisplayLength
    );

YORI_ALLOC_SIZE_T
YoriWinLabelCountLinesRequiredForText(
    __in PYORI_STRING Text,
    __in YORI_ALLOC_SIZE_T CtrlWidth,
    __out_opt PYORI_ALLOC_SIZE_T MaximumWidth
    );

BOOLEAN
YoriWinLabelSetCaption(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PCYORI_STRING Caption
    );

BOOLEAN
YoriWinLabelReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );


// LIST.C

/**
 The list should display a vertical scroll bar.
 */
#define YORI_WIN_LIST_STYLE_VSCROLLBAR  (0x0001)

/**
 The list should support selection per row, not one per list
 */
#define YORI_WIN_LIST_STYLE_MULTISELECT (0x0002)

/**
 The list should clear selection when losing focus
 */
#define YORI_WIN_LIST_STYLE_DESELECT_ON_LOSE_FOCUS (0x0004)

/**
 The list should display multiple items on one line
 */
#define YORI_WIN_LIST_STYLE_HORIZONTAL  (0x0008)

/**
 The list should not have a border around the control
 */
#define YORI_WIN_LIST_STYLE_NO_BORDER   (0x0010)

PYORI_WIN_CTRL_HANDLE
YoriWinListCreate(
    __in PYORI_WIN_WINDOW_HANDLE Parent,
    __in PSMALL_RECT Size,
    __in DWORD Style
    );

DWORD
YoriWinListGetItemCount(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

__success(return)
BOOLEAN
YoriWinListGetActiveOption(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PYORI_ALLOC_SIZE_T CurrentlyActiveIndex
    );

__success(return)
BOOLEAN
YoriWinListSetActiveOption(
    __inout PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T ActiveOption
    );

BOOLEAN
YoriWinListIsOptionSelected(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T Index
    );

BOOLEAN
YoriWinListClearAllItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

__success(return)
BOOLEAN
YoriWinListAddItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PCYORI_STRING ListOptions,
    __in YORI_ALLOC_SIZE_T NumberOptions
    );

BOOLEAN
YoriWinListGetItemText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T Index,
    __inout PYORI_STRING Text
    );

BOOLEAN
YoriWinListReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

BOOLEAN
YoriWinListSetHorizontalItemWidth(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in WORD ItemWidth
    );

BOOLEAN
YoriWinListSetSelectionNotifyCallback(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_NOTIFY NotifyCallback
    );


// MENUBAR.C


/**
 Specifies an API representation of a menu.
 */
typedef struct _YORI_WIN_MENU {

    /**
     An array of menu items contained within the menu.
     */
    struct _YORI_WIN_MENU_ENTRY *Items;

    /**
     The number of menu items contained within the menu.
     */
    YORI_ALLOC_SIZE_T ItemCount;
} YORI_WIN_MENU, *PYORI_WIN_MENU;

/**
 Indicates that the menu entry should be a horizontal seperator bar.
 */
#define YORI_WIN_MENU_ENTRY_SEPERATOR (0x00000001)

/**
 Indicates that the menu entry should be disabled.
 */
#define YORI_WIN_MENU_ENTRY_DISABLED (0x00000002)

/**
 Indicates that the menu entry should be checked.
 */
#define YORI_WIN_MENU_ENTRY_CHECKED (0x00000004)

/**
 Specifies an API representation for a menu item within a menu bar control.
 */
typedef struct _YORI_WIN_MENU_ENTRY {

    /**
     Specifies the string for this menu item.  Note this string may contain an
     ampersand to indicate which item is the accelerator character.
     */
    YORI_STRING Caption;

    /**
     A human readable form of the hotkey.
     */
    YORI_STRING Hotkey;

    /**
     Specifies a callback function to invoke when this item is activated.
     */
    PYORI_WIN_NOTIFY NotifyCallback;

    /**
     Specifies any child menu associated with the menu item.  This structure
     indicates the number of child items and has an array of those items.
     */
    YORI_WIN_MENU ChildMenu;

    /**
     Specifies flags associated with the menu item.
     */
    DWORD Flags;
} YORI_WIN_MENU_ENTRY, *PYORI_WIN_MENU_ENTRY;

PYORI_WIN_CTRL_HANDLE
YoriWinMenuBarCreate(
    __in PYORI_WIN_CTRL_HANDLE ParentHandle,
    __in DWORD Style
    );

BOOLEAN
YoriWinMenuBarAppendItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_MENU Items
    );

VOID
YoriWinMenuBarDisableMenuItem(
    __in PYORI_WIN_CTRL_HANDLE ItemHandle
    );

VOID
YoriWinMenuBarEnableMenuItem(
    __in PYORI_WIN_CTRL_HANDLE ItemHandle
    );

VOID
YoriWinMenuBarCheckMenuItem(
    __in PYORI_WIN_CTRL_HANDLE ItemHandle
    );

VOID
YoriWinMenuBarUncheckMenuItem(
    __in PYORI_WIN_CTRL_HANDLE ItemHandle
    );

PYORI_WIN_CTRL_HANDLE
YoriWinMenuBarGetSubmenuHandle(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in_opt PYORI_WIN_CTRL_HANDLE ParentItemHandle,
    __in DWORD SubIndex
    );

BOOLEAN
YoriWinMenuBarReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

// MLEDIT.C

/**
 A function prototype that can be invoked to deliver notification events
 when the cursor is moved.
 */
typedef VOID YORI_WIN_NOTIFY_MULTILINE_EDIT_CURSOR_MOVE(PYORI_WIN_CTRL_HANDLE, DWORD, DWORD);

/**
 A pointer to a function that can be invoked to deliver notification events
 when the cursor is moved.
 */
typedef YORI_WIN_NOTIFY_MULTILINE_EDIT_CURSOR_MOVE *PYORI_WIN_NOTIFY_MULTILINE_EDIT_CURSOR_MOVE;

/**
 The multiline edit should display a vertical scroll bar.
 */
#define YORI_WIN_MULTILINE_EDIT_STYLE_VSCROLLBAR  (0x0001)

/**
 The multiline edit should be read only.
 */
#define YORI_WIN_MULTILINE_EDIT_STYLE_READ_ONLY   (0x0002)

BOOLEAN
YoriWinMultilineEditSelectionActive(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditClear(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditDeleteSelection(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

__success(return)
BOOLEAN
YoriWinMultilineEditGetSelectedText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING NewlineString,
    __out PYORI_STRING SelectedText
    );

BOOLEAN
YoriWinMultilineEditInsertTextAtCursor(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING Text
    );

BOOLEAN
YoriWinMultilineEditAppendLinesNoDataCopy(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING NewLines,
    __in YORI_ALLOC_SIZE_T NewLineCount
    );

BOOLEAN
YoriWinMultilineEditCopySelectedText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditCutSelectedText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditPasteText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

VOID
YoriWinMultilineEditSetSelectionRange(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T StartLine,
    __in YORI_ALLOC_SIZE_T StartOffset,
    __in YORI_ALLOC_SIZE_T EndLine,
    __in YORI_ALLOC_SIZE_T EndOffset
    );

__success(return)
BOOLEAN
YoriWinMultilineEditGetSelectionRange(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PYORI_ALLOC_SIZE_T StartLine,
    __out PYORI_ALLOC_SIZE_T StartOffset,
    __out PYORI_ALLOC_SIZE_T EndLine,
    __out PYORI_ALLOC_SIZE_T EndOffset
    );

VOID
YoriWinMultilineEditSetColor(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in WORD Attributes,
    __in WORD SelectedAttributes
    );

YORI_ALLOC_SIZE_T
YoriWinMultilineEditGetLineCount(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

PYORI_STRING
YoriWinMultilineEditGetLineByIndex(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T Index
    );

VOID
YoriWinMultilineEditGetCursorLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PYORI_ALLOC_SIZE_T CursorOffset,
    __out PYORI_ALLOC_SIZE_T CursorLine
    );

VOID
YoriWinMultilineEditSetCursorLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T NewCursorOffset,
    __in YORI_ALLOC_SIZE_T NewCursorLine
    );

VOID
YoriWinMultilineEditGetViewportLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PYORI_ALLOC_SIZE_T ViewportLeft,
    __out PYORI_ALLOC_SIZE_T ViewportTop
    );

VOID
YoriWinMultilineEditSetViewportLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T NewViewportLeft,
    __in YORI_ALLOC_SIZE_T NewViewportTop
    );

BOOLEAN
YoriWinMultilineEditSetCaption(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING Caption
    );

BOOLEAN
YoriWinMultilineEditSetModifyState(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN ModifyState
    );

__success(return)
BOOLEAN
YoriWinMultilineEditGetTabWidth(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PYORI_ALLOC_SIZE_T TabWidth
    );

__success(return)
BOOLEAN
YoriWinMultilineEditSetTabWidth(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in YORI_ALLOC_SIZE_T TabWidth
    );

VOID
YoriWinMultilineEditSetAutoIndent(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN AutoIndentEnabled
    );

VOID
YoriWinMultilineEditSetExpandTab(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN ExpandTabEnabled
    );

VOID
YoriWinMultilineEditSetTrimTrailingWhitespace(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN TrimTrailingWhitespaceEnabled
    );

VOID
YoriWinMultilineEditSetTraditionalNavigation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN TraditionalNavigationEnabled
    );

BOOLEAN
YoriWinMultilineEditGetModifyState(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditSetCursorMoveNotifyCallback(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_NOTIFY_MULTILINE_EDIT_CURSOR_MOVE NotifyCallback
    );

BOOLEAN
YoriWinMultilineEditIsUndoAvailable(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditIsRedoAvailable(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditUndo(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditRedo(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

BOOLEAN
YoriWinMultilineEditSetReadOnly(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN NewReadOnlyState
    );

PYORI_WIN_CTRL_HANDLE
YoriWinMultilineEditCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in_opt PYORI_STRING Caption,
    __in PSMALL_RECT Size,
    __in DWORD Style
    );

// RADIO.C

BOOLEAN
YoriWinRadioIsSelected(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

VOID
YoriWinRadioSelect(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinRadioReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

PYORI_WIN_CTRL_HANDLE
YoriWinRadioCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in PYORI_STRING Caption,
    __in_opt PYORI_WIN_CTRL_HANDLE FirstRadioControl,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY ToggleCallback
    );

// WINDOW.C

/**
 A function prototype that can be invoked to deliver notification events
 when the window manager size changes.
 */
typedef VOID YORI_WIN_NOTIFY_WINDOW_MANAGER_RESIZE(PYORI_WIN_WINDOW_HANDLE, PSMALL_RECT, PSMALL_RECT);

/**
 A pointer to a function that can be invoked to deliver notification events
 when the window manager size changes.
 */
typedef YORI_WIN_NOTIFY_WINDOW_MANAGER_RESIZE *PYORI_WIN_NOTIFY_WINDOW_MANAGER_RESIZE;

VOID
YoriWinCloseWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in DWORD_PTR Result
    );

VOID
YoriWinDestroyWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

BOOLEAN
YoriWinDisplayWindowContents(
    __in PYORI_WIN_WINDOW_HANDLE Window
    );

VOID
YoriWinSetFocus(
    __in PYORI_WIN_WINDOW_HANDLE Window,
    __in_opt PYORI_WIN_CTRL_HANDLE Ctrl
    );

BOOLEAN
YoriWinSetWindowManagerResizeNotifyCallback(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in PYORI_WIN_NOTIFY_WINDOW_MANAGER_RESIZE NotifyCallback
    );

PYORI_WIN_WINDOW_MANAGER_HANDLE
YoriWinGetWindowManagerHandle(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

PYORI_WIN_CTRL_HANDLE
YoriWinGetCtrlFromWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

PYORI_WIN_WINDOW_HANDLE
YoriWinGetWindowFromWindowCtrl(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

/**
 Display a single line border around the window.
 */
#define YORI_WIN_WINDOW_STYLE_BORDER_SINGLE      (0x0001)

/**
 Display a double line border around the window.
 */
#define YORI_WIN_WINDOW_STYLE_BORDER_DOUBLE      (0x0002)

/**
 Display a solid shadow under the window.
 */
#define YORI_WIN_WINDOW_STYLE_SHADOW_SOLID       (0x0004)

/**
 Display a transparent shadow under the window.
 */
#define YORI_WIN_WINDOW_STYLE_SHADOW_TRANSPARENT (0x0008)

__success(return)
BOOLEAN
YoriWinCreateWindowEx(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PSMALL_RECT WindowRect,
    __in DWORD Style,
    __in_opt PCYORI_STRING Title,
    __out PYORI_WIN_WINDOW_HANDLE *OutWindow
    );

__success(return)
BOOLEAN
YoriWinCreateWindow(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in WORD MinimumWidth,
    __in WORD MinimumHeight,
    __in WORD DesiredWidth,
    __in WORD DesiredHeight,
    __in DWORD Style,
    __in_opt PCYORI_STRING Title,
    __out PYORI_WIN_WINDOW_HANDLE *OutWindow
    );

__success(return)
BOOLEAN
YoriWinDetermineWindowRect(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in WORD MinimumWidth,
    __in WORD MinimumHeight,
    __in WORD DesiredWidth,
    __in WORD DesiredHeight,
    __in WORD DesiredLeft,
    __in WORD DesiredTop,
    __in DWORD Style,
    __out PSMALL_RECT WindowRect
    );

BOOLEAN
YoriWinWindowReposition(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in PSMALL_RECT WindowRect
    );

VOID
YoriWinGetClientSize(
    __in PYORI_WIN_WINDOW_HANDLE Window,
    __out PCOORD Size
    );

VOID
YoriWinEnableNonAltAccelerators(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in BOOLEAN EnableNonAltAccelerators
    );

__success(return)
BOOLEAN
YoriWinProcessInputForWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __out_opt PDWORD_PTR Result
    );

// WINMGR.C

__success(return)
BOOLEAN
YoriWinOpenWindowManager(
    __in BOOLEAN UseAlternateBuffer,
    __in YORI_WIN_COLOR_TABLE_ID ColorTableId,
    __out PYORI_WIN_WINDOW_MANAGER_HANDLE *WinMgrHandle
    );

VOID
YoriWinMgrSetAsciiDrawing(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in BOOLEAN UseAsciiDrawing
    );

__success(return)
BOOLEAN
YoriWinGetWinMgrDimensions(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __out PCOORD Size
    );

__success(return)
BOOLEAN
YoriWinGetWinMgrLocation(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __out PSMALL_RECT Rect
    );

__success(return)
BOOLEAN
YoriWinGetWinMgrInitialCursorLocation(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __out PCOORD CursorLocation
    );

VOID
YoriWinCloseWindowManager(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

__success(return)
BOOLEAN
YoriWinMgrProcessAllEvents(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

// vim:sw=4:ts=4:et:
