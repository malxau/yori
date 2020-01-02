/**
 * @file libwin/yoriwin.h
 *
 * Header for library routines that may be of value from the shell as well
 * as external tools.
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

// BUTTON.C

/**
 A function prototype that can be invoked to deliver notification events
 for a specific control.
 */
typedef VOID YORI_WIN_NOTIFY_BUTTON_CLICK(PYORI_WIN_CTRL_HANDLE);

/**
 A pointer to a function that can be invoked to deliver notification events
 for a specific control.
 */
typedef YORI_WIN_NOTIFY_BUTTON_CLICK *PYORI_WIN_NOTIFY_BUTTON_CLICK;

/**
 The button is the default button on the window.
 */
#define YORI_WIN_BUTTON_STYLE_DEFAULT (0x0001)

/**
 The button is the cancel button on the window.
 */
#define YORI_WIN_BUTTON_STYLE_CANCEL  (0x0002)

PYORI_WIN_CTRL_HANDLE
YoriWinCreateButton(
    __in PYORI_WIN_WINDOW_HANDLE Parent,
    __in PSMALL_RECT Size,
    __in PYORI_STRING Caption,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY_BUTTON_CLICK ClickCallback
    );

// COMBO.C

__success(return)
BOOL
YoriWinComboGetActiveOption(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PDWORD CurrentlyActiveIndex
    );

__success(return)
BOOL
YoriWinComboSetActiveOption(
    __inout PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD ActiveOption
    );

__success(return)
BOOL
YoriWinComboAddItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING ListOptions,
    __in DWORD NumberOptions
    );

PYORI_WIN_CTRL_HANDLE
YoriWinCreateCombo(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in WORD LinesInList,
    __in PYORI_STRING Caption,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY_BUTTON_CLICK ClickCallback
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
YoriWinEditGetText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __inout PYORI_STRING Text
    );

BOOLEAN
YoriWinEditSetText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING Text
    );

PYORI_WIN_CTRL_HANDLE
YoriWinCreateEdit(
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
YoriWinCreateLabel(
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


// LIST.C

/**
 The list should display a vertical scroll bar.
 */
#define YORI_WIN_LIST_STYLE_VSCROLLBAR  (0x0001)

/**
 The list should support selection per row, not one per list
 */
#define YORI_WIN_LIST_STYLE_MULTISELECT (0x0002)

PYORI_WIN_CTRL_HANDLE
YoriWinCreateList(
    __in PYORI_WIN_WINDOW_HANDLE Parent,
    __in PSMALL_RECT Size,
    __in DWORD Style
    );

__success(return)
BOOL
YoriWinListGetActiveOption(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PDWORD CurrentlyActiveIndex
    );

__success(return)
BOOL
YoriWinListSetActiveOption(
    __inout PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD ActiveOption
    );

BOOL
YoriWinListIsOptionSelected(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD Index
    );

BOOL
YoriWinListClearAllItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    );

__success(return)
BOOL
YoriWinListAddItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING ListOptions,
    __in DWORD NumberOptions
    );

// *** WINDOW.C ***

VOID
YoriWinCloseWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in DWORD_PTR Result
    );

VOID
YoriWinDestroyWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

/**
 Display a single line border around the window.
 */
#define YORI_WIN_WINDOW_STYLE_BORDER_SINGLE (0x0001)

/**
 Display a double line border around the window.
 */
#define YORI_WIN_WINDOW_STYLE_BORDER_DOUBLE (0x0002)

/**
 Display a shadow under the window.
 */
#define YORI_WIN_WINDOW_STYLE_SHADOW        (0x0004)

__success(return)
BOOL
YoriWinCreateWindowEx(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PSMALL_RECT WindowRect,
    __in DWORD Style,
    __in_opt PYORI_STRING Title,
    __out PYORI_WIN_WINDOW_HANDLE *OutWindow
    );

__success(return)
BOOL
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

VOID
YoriWinGetClientSize(
    __in PYORI_WIN_WINDOW_HANDLE Window,
    __out PCOORD Size
    );

__success(return)
BOOL
YoriWinProcessInputForWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __out_opt PDWORD_PTR Result
    );

// *** WINMGR.C ***

__success(return)
BOOL
YoriWinOpenWindowManager(
    __out PYORI_WIN_WINDOW_MANAGER_HANDLE *WinMgrHandle
    );

VOID
YoriWinCloseWindowManager(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

// vim:sw=4:ts=4:et:
