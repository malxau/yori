/**
 * @file libwin/winpriv.h
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

#ifndef YORI_WIN_WINDOW_DEFINED
/**
 An opaque pointer to a popup window.
 */
typedef PVOID PYORI_WIN_WINDOW;
#endif

/**
 An event that may be of interest to a window or control.  This is typically
 keyboard or mouse input but may ultimately describe higher level concepts.
 */
typedef struct _YORI_WIN_EVENT {

    /**
     The type of the event.
     */
    enum {
        YoriWinEventKeyDown                     = 1,
        YoriWinEventKeyUp                       = 2,
        YoriWinEventParentDestroyed             = 3,
        YoriWinEventMouseDownInClient           = 4,
        YoriWinEventMouseUpInClient             = 5,
        YoriWinEventMouseDownInNonClient        = 6,
        YoriWinEventMouseUpInNonClient          = 7,
        YoriWinEventMouseDownOutsideWindow      = 8,
        YoriWinEventMouseUpOutsideWindow        = 9,
        YoriWinEventGetFocus                    = 10,
        YoriWinEventLoseFocus                   = 11,
        YoriWinEventGetEffectiveDefault         = 12,
        YoriWinEventLoseEffectiveDefault        = 13,
        YoriWinEventGetEffectiveCancel          = 14,
        YoriWinEventLoseEffectiveCancel         = 15,
        YoriWinEventMouseDoubleClickInClient    = 16,
        YoriWinEventMouseDoubleClickInNonClient = 17,
        YoriWinEventDisplayAccelerators         = 18,
        YoriWinEventHideAccelerators            = 19,
        YoriWinEventExecute                     = 20,
        YoriWinEventMouseMoveInClient           = 21,
        YoriWinEventMouseMoveInNonClient        = 23,
        YoriWinEventMouseWheelUpInClient        = 24,
        YoriWinEventMouseWheelUpInNonClient     = 25,
        YoriWinEventMouseWheelDownInClient      = 26,
        YoriWinEventMouseWheelDownInNonClient   = 27,
        YoriWinEventBeyondMax                   = 28
    } EventType;

    /**
     Information specific to the type of the event.
     */
    union {
        struct {
            DWORD CtrlMask;
            DWORD VirtualKeyCode;
            DWORD VirtualScanCode;
            TCHAR Char;
        } KeyDown;

        struct {
            DWORD CtrlMask;
            DWORD VirtualKeyCode;
            DWORD VirtualScanCode;
            TCHAR Char;
        } KeyUp;

        struct {
            DWORD ButtonsPressed;
            COORD Location;
            DWORD ControlKeyState;
        } MouseDown;

        struct {
            DWORD ButtonsReleased;
            COORD Location;
            DWORD ControlKeyState;
        } MouseUp;

        struct {
            COORD Location;
            DWORD ControlKeyState;
        } MouseMove;

        struct {
            DWORD LinesToMove;
            COORD Location;
            DWORD ControlKeyState;
        } MouseWheel;
    };
} YORI_WIN_EVENT, *PYORI_WIN_EVENT;

/**
 A function prototype that can be invoked to deliver notification events
 for a specific control.
 */
typedef BOOLEAN YORI_WIN_NOTIFY_EVENT(PVOID, PYORI_WIN_EVENT);

/**
 A pointer to a function that can be invoked to deliver notification events
 for a specific control.
 */
typedef YORI_WIN_NOTIFY_EVENT *PYORI_WIN_NOTIFY_EVENT;

/**
 A common header which is embedded within each control.
 */
typedef struct _YORI_WIN_CTRL {

    /**
     A pointer to the control containing the control.  This is typically a
     toplevel window, which can be determined because Parent->Parent will be
     NULL.
     */
    struct _YORI_WIN_CTRL *Parent;

    /**
     A list of controls on the parent window.  This control is one element
     among its peers.
     */
    YORI_LIST_ENTRY ParentControlList;

    /**
     A list of child controls.  These controls render onto this control.
     Paired with ParentControlList above.
     */
    YORI_LIST_ENTRY ChildControlList;

    /**
     The dimensions occupied by the control within the parent window,
     relative to the parent window's client area.
     */
    SMALL_RECT FullRect;

    /**
     The dimensions within the control's dimensions of its client area.
     Note these are not the dimensions within the parent window.
     */
    SMALL_RECT ClientRect;

    /**
     If non-NULL, a function to receive notification about keyboard and mouse
     events that should be processed by this control.
     */
    PYORI_WIN_NOTIFY_EVENT NotifyEventFn;

    /**
     An identifer for the control.  This is used purely for a window to
     identify controls for its own higher level use; it is not used by the
     window system itself, which doesn't know or care what control has what
     identifier.
     */
    DWORD_PTR CtrlId;

    /**
     A character that when combined with the Alt key indicates the user wants
     to execute this control.
     */
    TCHAR AcceleratorChar;

    /**
     The attributes of the console cells to default to within the control.
     */
    WORD DefaultAttributes;

    /**
     If TRUE, the coordinates in FullRect are relative to the parent's client
     area.  If FALSE, the coordinates in FullRect are relative to the
     parent's FullRect.  Typically controls on top level windows are relative
     to the parent's client area but controls that are part of other controls
     (eg. scrollbars) are relative to the parent's full area.
     */
    BOOLEAN RelativeToParentClient;

    /**
     If TRUE, the control is capable of receiving focus.  If FALSE, focus
     should be sent to the next control capable of receiving it.
     */
    BOOLEAN CanReceiveFocus;

    /**
     A bitmask of the mouse button down notifications that have been
     received by this control.  If a control has observed a mouse down event,
     it will also be sent a mouse up event, even if the event occurs outside
     of the control dimensions.  MouseUpOutsideWindow is used to distinguish
     between releasing a button within a control and releasing a button
     outside of the window.
     */
    UCHAR MouseButtonsPressed;

    // TODO: Probably should have a control type if only for validation and
    // debugging

} YORI_WIN_CTRL, *PYORI_WIN_CTRL;

// ITEMARAY.C

/**
 On a multiselect list, this flag is set to indicate that the item is selected.
 */
#define YORI_WIN_ITEM_SELECTED (0x0001)

/**
 A structure defining a single item within the list.
 */
typedef struct _YORI_WIN_ITEM_ENTRY {

    /**
     The string corresponding to the item.
     */
    YORI_STRING String;

    /**
     State about the item.
     */
    DWORD Flags;

} YORI_WIN_ITEM_ENTRY, *PYORI_WIN_ITEM_ENTRY;

/**
 An array of items, suitable for use in a list control or similar.
 */
typedef struct _YORI_WIN_ITEM_ARRAY {

    /**
     Count of items in the array.
     */
    DWORD Count;

    /**
     An array of items in memory.  This allocation is referenced because it
     is used here, and may be referenced by each string that is contained
     within the allocation.
     */
    PYORI_WIN_ITEM_ENTRY Items;
} YORI_WIN_ITEM_ARRAY, *PYORI_WIN_ITEM_ARRAY;

VOID
YoriWinItemArrayInitialize(
    __out PYORI_WIN_ITEM_ARRAY ItemArray
    );

VOID
YoriWinItemArrayCleanup(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray
    );

__success(return)
BOOL
YoriWinItemArrayAddItems(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray,
    __in PYORI_STRING NewItems,
    __in DWORD NumNewItems
    );

__success(return)
BOOL
YoriWinItemArrayAddItemArray(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray,
    __in PYORI_WIN_ITEM_ARRAY NewItems
    );

/**
 Draw a border with a single color all around
 */
#define YORI_WIN_BORDER_TYPE_FLAT   0x0000

/**
 Draw a border with a bright top and left, and dark right and bottom
 */
#define YORI_WIN_BORDER_TYPE_RAISED 0x0001

/**
 Draw a border with a dark top and left, and a bright right and bottom
 */
#define YORI_WIN_BORDER_TYPE_SUNKEN 0x0002

/**
 The set of bits which define the color scheme to use for the border
 */
#define YORI_WIN_BORDER_THREED_MASK 0x0003


/**
 Draw a border with a single line
 */
#define YORI_WIN_BORDER_TYPE_SINGLE     0x0000

/**
 Draw a border with a double line
 */
#define YORI_WIN_BORDER_TYPE_DOUBLE     0x0010

/**
 Draw a border with an entire full height character
 */
#define YORI_WIN_BORDER_TYPE_SOLID_FULL 0x0020

/**
 Draw a border with a full character left and right and a half character top
 and bottom
 */
#define YORI_WIN_BORDER_TYPE_SOLID_HALF 0x0040

/**
 The set of bits which define the border characters to use
 */
#define YORI_WIN_BORDER_STYLE_MASK      0x0070


BOOL
YoriWinDrawBorderOnWindow(
    __inout PYORI_WIN_WINDOW Window,
    __in PSMALL_RECT Dimensions,
    __in WORD Attributes,
    __in WORD BorderType
    );

BOOL
YoriWinDrawBorderOnControl(
    __inout PYORI_WIN_CTRL Ctrl,
    __in PSMALL_RECT Dimensions,
    __in WORD Attributes,
    __in WORD BorderType
    );


// CTRL.C

__success(return)
BOOL
YoriWinCreateControl(
    __in_opt PYORI_WIN_CTRL Parent,
    __in PSMALL_RECT Rect,
    __in BOOLEAN CanReceiveFocus,
    __out PYORI_WIN_CTRL Ctrl
    );

VOID
YoriWinDestroyControl(
    __in PYORI_WIN_CTRL Ctrl
    );

PYORI_WIN_WINDOW
YoriWinGetTopLevelWindow(
    __in PYORI_WIN_CTRL Ctrl
    );

VOID
YoriWinGetControlClientSize(
    __in PYORI_WIN_CTRL Ctrl,
    __out PCOORD Size
    );

BOOL
YoriWinSetControlCursorState(
    __in PYORI_WIN_CTRL Ctrl,
    __in BOOL Visible,
    __in DWORD SizePercentage
    );

VOID
YoriWinSetControlClientCursorLocation(
    __in PYORI_WIN_CTRL Ctrl,
    __in WORD X,
    __in WORD Y
    );

BOOL
YoriWinCoordInSmallRect(
    __in PCOORD Location,
    __in PSMALL_RECT Area
    );

__success(return != NULL)
PYORI_WIN_CTRL
YoriWinFindControlAtCoordinates(
    __in PYORI_WIN_CTRL Parent,
    __in COORD Location,
    __in BOOLEAN ParentLocationRelativeToClient,
    __out PCOORD LocationInChild,
    __out PBOOLEAN ChildLocationRelativeToClient
    );

BOOLEAN
YoriWinTranslateMouseEventForChild(
    __in PYORI_WIN_EVENT Event,
    __in PYORI_WIN_CTRL Ctrl,
    __in COORD ChildLocation,
    __in BOOLEAN InChildClientArea
    );

VOID
YoriWinNotifyAllControls(
    __in PYORI_WIN_CTRL Parent,
    __in PYORI_WIN_EVENT Event
    );

VOID
YoriWinSetControlClientCell(
    __in PYORI_WIN_CTRL Ctrl,
    __in WORD X,
    __in WORD Y,
    __in TCHAR Char,
    __in WORD Attr
    );

VOID
YoriWinSetControlNonClientCell(
    __in PYORI_WIN_CTRL Ctrl,
    __in WORD X,
    __in WORD Y,
    __in TCHAR Char,
    __in WORD Attr
    );

VOID
YoriWinTranslateCtrlCoordinatesToWindowCoordinates(
    __in PYORI_WIN_CTRL Ctrl,
    __in BOOLEAN CtrlCoordInClient,
    __in COORD CtrlCoord,
    __in BOOLEAN WinCoordInClient,
    __out PYORI_WIN_WINDOW* Window,
    __out PCOORD WinCoord
    );

VOID
YoriWinTranslateCtrlCoordinatesToScreenCoordinates(
    __in PYORI_WIN_CTRL Ctrl,
    __in BOOLEAN CtrlCoordInClient,
    __in COORD CtrlCoord,
    __out PCOORD ScreenCoord
    );

// LIST.C

__success(return)
BOOL
YoriWinListAddItemArray(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_ITEM_ARRAY NewItems
    );

// SCROLBAR.C

PYORI_WIN_CTRL
YoriWinCreateScrollBar(
    __in PYORI_WIN_CTRL Parent,
    __in PSMALL_RECT Size,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY_BUTTON_CLICK ChangeCallback
    );

DWORDLONG
YoriWinGetScrollBarPosition(
    __in PYORI_WIN_CTRL Ctrl
    );

VOID
YoriWinSetScrollBarPosition(
    __in PYORI_WIN_CTRL Ctrl,
    __in DWORDLONG CurrentValue,
    __in DWORDLONG NumberVisible,
    __in DWORDLONG MaximumValue
    );

// WINDOW.C

PYORI_WIN_WINDOW_MANAGER_HANDLE
YoriWinGetWindowManagerHandle(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

PYORI_WIN_WINDOW
YoriWinGetWindowFromWindowCtrl(
    __in PYORI_WIN_CTRL Ctrl
    );

PYORI_WIN_CTRL
YoriWinGetCtrlFromWindow(
    __in PYORI_WIN_WINDOW Window
    );

VOID
YoriWinSetWindowCell(
    __inout PYORI_WIN_WINDOW Window,
    __in WORD X,
    __in WORD Y,
    __in TCHAR Char,
    __in WORD Attr
    );

VOID
YoriWinSetWindowClientCell(
    __inout PYORI_WIN_WINDOW Window,
    __in WORD X,
    __in WORD Y,
    __in TCHAR Char,
    __in WORD Attr
    );

BOOL
YoriWinDisplayWindowContents(
    __in PYORI_WIN_WINDOW Window
    );

VOID
YoriWinGetWindowSize(
    __in PYORI_WIN_WINDOW Window,
    __out PCOORD WindowSize
    );

BOOL
YoriWinSetCursorState(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in BOOL Visible,
    __in DWORD SizePercentage
    );

VOID
YoriWinSetCursorPosition(
    __in PYORI_WIN_WINDOW Window,
    __in WORD NewX,
    __in WORD NewY
    );

VOID
YoriWinSetFocus(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_CTRL Ctrl
    );

VOID
YoriWinSetFocusToNextCtrl(
    __in PYORI_WIN_WINDOW Window
    );

VOID
YoriWinSetDefaultCtrl(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_CTRL Ctrl
    );

VOID
YoriWinSuppressDefaultControl(
    __in PYORI_WIN_WINDOW Window
    );

VOID
YoriWinRestoreDefaultControl(
    __in PYORI_WIN_WINDOW Window
    );

VOID
YoriWinSetCancelCtrl(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_CTRL Ctrl
    );

VOID
YoriWinSuppressCancelControl(
    __in PYORI_WIN_WINDOW Window
    );

VOID
YoriWinRestoreCancelControl(
    __in PYORI_WIN_WINDOW Window
    );

VOID
YoriWinAddControlToWindow(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_CTRL Ctrl
    );

VOID
YoriWinRemoveControlFromWindow(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_CTRL Ctrl
    );

BOOLEAN
YoriWinSetCustomNotification(
    __in PYORI_WIN_WINDOW Window,
    __in DWORD EventType,
    __in PYORI_WIN_NOTIFY_EVENT Handler
    );

// WINMGR.C

HANDLE
YoriWinGetConsoleInputHandle(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

HANDLE
YoriWinGetConsoleOutputHandle(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

BOOLEAN
YoriWinIsConhostv2(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

DWORD
YoriWinGetPreviousMouseButtonState(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

VOID
YoriWinSetPreviousMouseButtonState(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in DWORD PreviousMouseButtonState
    );

// vim:sw=4:ts=4:et:
