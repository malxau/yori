/**
 * @file libwin/yoriwin.h
 *
 * Header for control and window toolkit routines that may be of value from
 * the shell as well as external tools.
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
    __out PDWORD CurrentlyActiveIndex
    );

__success(return)
BOOLEAN
YoriWinComboSetActiveOption(
    __inout PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD ActiveOption
    );

__success(return)
BOOLEAN
YoriWinComboAddItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING ListOptions,
    __in DWORD NumberOptions
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
    __in DWORD StartOffset,
    __in DWORD EndOffset
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
    __in PYORI_STRING RawString,
    __inout_opt PYORI_STRING ParsedString,
    __out_opt TCHAR* AcceleratorChar,
    __out_opt PDWORD HighlightOffset,
    __out_opt PDWORD DisplayLength
    );

DWORD
YoriWinLabelCountLinesRequiredForText(
    __in PYORI_STRING Text,
    __in DWORD CtrlWidth,
    __out_opt PDWORD MaximumWidth
    );

BOOLEAN
YoriWinLabelSetCaption(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING Caption
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
    __out PDWORD CurrentlyActiveIndex
    );

__success(return)
BOOLEAN
YoriWinListSetActiveOption(
    __inout PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD ActiveOption
    );

BOOLEAN
YoriWinListIsOptionSelected(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD Index
    );

BOOLEAN
YoriWinListClearAllItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

__success(return)
BOOLEAN
YoriWinListAddItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING ListOptions,
    __in DWORD NumberOptions
    );

BOOLEAN
YoriWinListGetItemText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD Index,
    __inout PYORI_STRING Text
    );

BOOLEAN
YoriWinListReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

BOOLEAN
YoriWinListSetSelectionNotifyCallback(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_NOTIFY NotifyCallback
    );


// *** MENUBAR.C ***


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
    DWORD ItemCount;
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

// *** MLEDIT.C ***

/**
 A function prototype that can be invoked to deliver notification events
 when the cursor is moved.
 */
typedef VOID YORI_WIN_NOTIFY_CURSOR_MOVE(PYORI_WIN_CTRL_HANDLE, DWORD, DWORD);

/**
 A pointer to a function that can be invoked to deliver notification events
 when the cursor is moved.
 */
typedef YORI_WIN_NOTIFY_CURSOR_MOVE *PYORI_WIN_NOTIFY_CURSOR_MOVE;

/**
 The multiline edit should display a vertical scroll bar.
 */
#define YORI_WIN_MULTILINE_EDIT_STYLE_VSCROLLBAR  (0x0001)

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
    __in DWORD NewLineCount
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
    __in DWORD StartLine,
    __in DWORD StartOffset,
    __in DWORD EndLine,
    __in DWORD EndOffset
    );

BOOLEAN
YoriWinMultilineEditGetSelectionRange(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PDWORD StartLine,
    __out PDWORD StartOffset,
    __out PDWORD EndLine,
    __out PDWORD EndOffset
    );

VOID
YoriWinMultilineEditSetColor(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in WORD Attributes
    );

DWORD
YoriWinMultilineEditGetLineCount(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

PYORI_STRING
YoriWinMultilineEditGetLineByIndex(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD Index
    );

VOID
YoriWinMultilineEditGetCursorLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PDWORD CursorOffset,
    __out PDWORD CursorLine
    );

VOID
YoriWinMultilineEditSetCursorLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD NewCursorOffset,
    __in DWORD NewCursorLine
    );

VOID
YoriWinMultilineEditGetViewportLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PDWORD ViewportLeft,
    __out PDWORD ViewportTop
    );

VOID
YoriWinMultilineEditSetViewportLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD NewViewportLeft,
    __in DWORD NewViewportTop
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
    __out PDWORD TabWidth
    );

__success(return)
BOOLEAN
YoriWinMultilineEditSetTabWidth(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD TabWidth
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
    __in PYORI_WIN_NOTIFY_CURSOR_MOVE NotifyCallback
    );

BOOLEAN
YoriWinMultilineEditIsUndoAvailable(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditUndo(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

BOOLEAN
YoriWinMultilineEditReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

PYORI_WIN_CTRL_HANDLE
YoriWinMultilineEditCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in_opt PYORI_STRING Caption,
    __in PSMALL_RECT Size,
    __in DWORD Style
    );

// *** WINDOW.C ***

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
    __in_opt PYORI_STRING Title,
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
    __in_opt PYORI_STRING Title,
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

// *** WINMGR.C ***

__success(return)
BOOLEAN
YoriWinOpenWindowManager(
    __in BOOLEAN UseAlternateBuffer,
    __out PYORI_WIN_WINDOW_MANAGER_HANDLE *WinMgrHandle
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

VOID
YoriWinCloseWindowManager(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

// vim:sw=4:ts=4:et:
