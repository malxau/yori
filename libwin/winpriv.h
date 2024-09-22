/**
 * @file libwin/winpriv.h
 *
 * Header for library routines that may be of value from the shell as well
 * as external tools.
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

#ifndef YORI_WIN_WINDOW_DEFINED
/**
 An opaque pointer to a popup window.
 */
typedef PVOID PYORI_WIN_WINDOW;
#endif

/**
 Pointer to a character and attribute array that should not be changed.
 */
typedef CHAR_INFO CONST * PCCHAR_INFO;

/**
 Pointer to a rectangle that should not be changed.
 */
typedef SMALL_RECT CONST * PCSMALL_RECT;

/**
 Some events might want to refer to a position which is outside of a control.
 Expressing this in control relative terms implies a position (if that
 coordinate is within the control) which can be superseded by values
 indicating the orientations where the position is outside of the control.
 */
typedef struct _YORI_WIN_BOUNDED_COORD {

    /**
     The position within the control.  The X component or Y component may be
     valid independently based on the below values.
     */
    COORD Pos;

    /**
     If TRUE, the position is above the control.  Mutually exclusive with
     Below, and implies that Pos.Y is not meaningful.
     */
    BOOLEAN Above;

    /**
     If TRUE, the position is below the control.  Mutually exclusive with
     Above, and implies that Pos.Y is not meaningful.
     */
    BOOLEAN Below;

    /**
     If TRUE, the position is to the left of the control.  Mutually
     exclusive with Right, and implies that Pos.X is not meaningful.
     */
    BOOLEAN Left;

    /**
     If TRUE, the position is to the left of the control.  Mutually
     exclusive with Left, and implies that Pos.X is not meaningful.
     */
    BOOLEAN Right;

} YORI_WIN_BOUNDED_COORD, *PYORI_WIN_BOUNDED_COORD;

/**
 An event that may be of interest to a window or control.  This is typically
 keyboard or mouse input but may ultimately describe higher level concepts.
 */
typedef struct _YORI_WIN_EVENT {

    /**
     Linkage to allow events to be added to a list prior to being dispatched.
     */
    YORI_LIST_ENTRY PostEventListEntry;

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
        YoriWinEventAccelerator                 = 28,
        YoriWinEventHotKeyDown                  = 29,
        YoriWinEventHideWindow                  = 30,
        YoriWinEventShowWindow                  = 31,
        YoriWinEventWindowManagerResize         = 32,
        YoriWinEventParentResize                = 33,
        YoriWinEventMouseMoveOutsideWindow      = 34,
        YoriWinEventTimer                       = 35,
        YoriWinEventBeyondMax                   = 36
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
            YORI_WIN_BOUNDED_COORD Location;
            DWORD ControlKeyState;
        } MouseMoveOutsideWindow;

        struct {
            DWORD LinesToMove;
            COORD Location;
            DWORD ControlKeyState;
        } MouseWheel;

        struct {
            TCHAR Char;
        } Accelerator;

        struct {
            SMALL_RECT OldWinMgrDimensions;
            SMALL_RECT NewWinMgrDimensions;
        } WindowManagerResize;

        struct {
            PYORI_WIN_CTRL_HANDLE Timer;
        } Timer;
    };
} YORI_WIN_EVENT, *PYORI_WIN_EVENT;

/**
 A pointer to a common header which is embedded within each control.
 */
typedef struct _YORI_WIN_CTRL *PYORI_WIN_CTRL;

/**
 A function prototype that can be invoked to deliver notification events
 for a specific control.
 */
typedef BOOLEAN YORI_WIN_NOTIFY_EVENT(PYORI_WIN_CTRL, PYORI_WIN_EVENT);

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
     A list of events that have not yet been processed.
     */
    YORI_LIST_ENTRY PostEventList;

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
     Pointer to memory that is associated with the control but not owned by
     this module.
     */
    PVOID UserContext;

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
     If TRUE, the control should receive focus in response to a mouse click.
     Individual controls or dialogs can suppress this behavior based on UX
     considerations (eg. should a check box click take focus from an input
     field), etc.
     */
    BOOLEAN ReceiveFocusOnMouseClick;

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

} YORI_WIN_CTRL;

/**
 A description of how the cursor should be displayed.
 */
typedef struct _YORI_WIN_CURSOR_STATE {

    /**
     TRUE if the cursor should be visible, FALSE if it should be hidden.
     */
    BOOLEAN Visible;

    /**
     The size of the cursor, in percent.
     */
    UCHAR SizePercentage;

    /**
     The X and Y coordinates for where the cursor should be displayed.
     */
    COORD Pos;
} YORI_WIN_CURSOR_STATE, *PYORI_WIN_CURSOR_STATE;

/**
 A list of possible shadow types that can be associated with a window.
 */
typedef enum _YORI_WIN_SHADOW_TYPE {
    YoriWinShadowNone,
    YoriWinShadowSolid,
    YoriWinShadowTransparent
} YORI_WIN_SHADOW_TYPE;


/**
 A pointer to a shadow type that can be associated with a window.
 */
typedef YORI_WIN_SHADOW_TYPE *PYORI_WIN_SHADOW_TYPE;

/**
 A set of character arrays which describe various drawing operations.  Note
 the order here corresponds to the order these are defined in winmgr.c.
 */
typedef enum _YORI_WIN_CHARACTERS {
    YoriWinCharsSingleLineBorder,
    YoriWinCharsDoubleLineBorder,
    YoriWinCharsFullSolidBorder,
    YoriWinCharsHalfSolidBorder,
    YoriWinCharsSingleLineAsciiBorder,
    YoriWinCharsDoubleLineAsciiBorder,
    YoriWinCharsMenu,
    YoriWinCharsAsciiMenu,
    YoriWinCharsScrollBar,
    YoriWinCharsAsciiScrollBar,
    YoriWinCharsShadow,
    YoriWinCharsAsciiShadow,
    YoriWinCharsComboDown,
    YoriWinCharsAsciiComboDown,
    YoriWinCharsRadioSelection,
    YoriWinCharsAsciiRadioSelection,
    YoriWinCharsOneLineSingleBorder,
    YoriWinCharsOneLineDoubleBorder,
} YORI_WIN_CHARACTERS;

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
     Count of items with meaningful data in the array.
     */
    YORI_ALLOC_SIZE_T Count;

    /**
     Number of elements allocated in the Items array.
     */
    YORI_ALLOC_SIZE_T CountAllocated;

    /**
     The base of the string allocation (to reference when consuming.)
     */
    LPTSTR StringAllocationBase;

    /**
     The current end of the string allocation (to write to when consuming.)
     */
    LPTSTR StringAllocationCurrent;

    /**
     The number of characters remaining in the string allocation.
     */
    YORI_ALLOC_SIZE_T StringAllocationRemaining;

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
BOOLEAN
YoriWinItemArrayAddItems(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray,
    __in PCYORI_STRING NewItems,
    __in YORI_ALLOC_SIZE_T NumNewItems
    );

__success(return)
BOOLEAN
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

/**
 A flag to indicate that the border should be displayed with a bright color.
 */
#define YORI_WIN_BORDER_BRIGHT          0x1000

BOOLEAN
YoriWinDrawBorderOnControl(
    __inout PYORI_WIN_CTRL Ctrl,
    __in PSMALL_RECT Dimensions,
    __in WORD Attributes,
    __in WORD BorderType
    );

BOOLEAN
YoriWinDrawSingleLineBorderOnControl(
    __inout PYORI_WIN_CTRL Ctrl,
    __in PSMALL_RECT Dimensions,
    __in WORD Attributes,
    __in WORD BorderType
    );

// COLOR.C

/**
 An opaque handle to a color table.
 */
typedef PVOID YORI_WIN_COLOR_TABLE_HANDLE;

/**
 The set of well known system colors.
 */
typedef enum _YORI_WIN_COLOR_ID {
    YoriWinColorWindowDefault = 0,
    YoriWinColorTitleBarActive,
    YoriWinColorMenuDefault,
    YoriWinColorMenuSelected,
    YoriWinColorMenuAccelerator,
    YoriWinColorMenuSelectedAccelerator,
    YoriWinColorMultilineCaption,
    YoriWinColorEditSelectedText,
    YoriWinColorAcceleratorDefault,
    YoriWinColorListActive,
    YoriWinColorControlSelected,
    YoriWinColorTitleBarInactive,
    YoriWinColorBeyondMax
} YORI_WIN_COLOR_ID;

YORI_WIN_COLOR_TABLE_HANDLE
YoriWinGetColorTable(
    __in YORI_WIN_COLOR_TABLE_ID ColorTableId
    );

UCHAR
YoriWinDefaultColorLookup(
    __in YORI_WIN_COLOR_TABLE_HANDLE ColorTableHandle,
    __in YORI_WIN_COLOR_ID ColorId
    );

// CTRL.C

__success(return)
BOOLEAN
YoriWinCreateControl(
    __in_opt PYORI_WIN_CTRL Parent,
    __in PSMALL_RECT Rect,
    __in BOOLEAN CanReceiveFocus,
    __in BOOLEAN ReceiveFocusOnMouseClick,
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

BOOLEAN
YoriWinSetControlCursorState(
    __in PYORI_WIN_CTRL Ctrl,
    __in BOOLEAN Visible,
    __in UCHAR SizePercentage
    );

VOID
YoriWinSetControlClientCursorLocation(
    __in PYORI_WIN_CTRL Ctrl,
    __in WORD X,
    __in WORD Y
    );

VOID
YoriWinGetCursorState(
    __in PYORI_WIN_WINDOW Window,
    __out PYORI_WIN_CURSOR_STATE CursorState
    );

BOOLEAN
YoriWinCoordInSmallRect(
    __in PCOORD Location,
    __in PSMALL_RECT Area
    );

VOID
YoriWinBoundCoordInSubRegion(
    __in PYORI_WIN_BOUNDED_COORD Pos,
    __in PSMALL_RECT SubRegion,
    __out PYORI_WIN_BOUNDED_COORD SubPos
    );

VOID
YoriWinGetControlNonClientRegion(
    __in PYORI_WIN_CTRL Ctrl,
    __out PSMALL_RECT CtrlRect
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

VOID
YoriWinTranslateScreenCoordinatesToWindow(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_CTRL Ctrl,
    __in COORD ScreenCoord,
    __out PBOOLEAN InWindowRange,
    __out PBOOLEAN InWindowClientRange,
    __out PCOORD CtrlCoord
    );

__success(return)
BOOLEAN
YoriWinControlReposition(
    __in PYORI_WIN_CTRL Ctrl,
    __in PSMALL_RECT NewRect
    );

BOOLEAN
YoriWinPostEvent(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    );

PYORI_WIN_EVENT
YoriWinGetNextPostedEvent(
    __in PYORI_WIN_CTRL Ctrl
    );

VOID
YoriWinFreePostedEvent(
    __in PYORI_WIN_EVENT Event
    );

// LIST.C

__success(return)
BOOLEAN
YoriWinListAddItemArray(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_ITEM_ARRAY NewItems
    );

// SCROLBAR.C

PYORI_WIN_CTRL
YoriWinScrollBarCreate(
    __in PYORI_WIN_CTRL Parent,
    __in PSMALL_RECT Size,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY ChangeCallback
    );

YORI_MAX_UNSIGNED_T
YoriWinScrollBarGetPosition(
    __in PYORI_WIN_CTRL Ctrl
    );

VOID
YoriWinScrollBarSetPosition(
    __in PYORI_WIN_CTRL Ctrl,
    __in YORI_MAX_UNSIGNED_T CurrentValue,
    __in YORI_MAX_UNSIGNED_T NumberVisible,
    __in YORI_MAX_UNSIGNED_T MaximumValue
    );

BOOLEAN
YoriWinScrollBarReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    );

// WINDOW.C

BOOLEAN
YoriWinFlushWindowContents(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

PYORI_WIN_CTRL
YoriWinGetFocus(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

PYORI_WIN_WINDOW_MANAGER_HANDLE
YoriWinGetWindowManagerHandle(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
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

VOID
YoriWinGetWindowSize(
    __in PYORI_WIN_WINDOW Window,
    __out PCOORD WindowSize
    );

BOOLEAN
YoriWinSetCursorState(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in BOOLEAN Visible,
    __in UCHAR SizePercentage
    );

VOID
YoriWinSetCursorPosition(
    __in PYORI_WIN_WINDOW Window,
    __in WORD NewX,
    __in WORD NewY
    );

VOID
YoriWinSetWindowFocus(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

VOID
YoriWinLoseWindowFocus(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
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

BOOLEAN
YoriWinIsWindowClosing(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

BOOLEAN
YoriWinIsWindowHidden(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

BOOLEAN
YoriWinIsWindowEnabled(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

VOID
YoriWinEnableWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

VOID
YoriWinDisableWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

PCCHAR_INFO
YoriWinGetWindowContentsBuffer(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __out PCSMALL_RECT *WindowRect,
    __out PYORI_WIN_SHADOW_TYPE ShadowType
    );

PYORI_WIN_WINDOW_HANDLE
YoriWinWindowFromZOrderListEntry(
    __in PYORI_LIST_ENTRY ListEntry
    );

PYORI_LIST_ENTRY
YoriWinZOrderListEntryFromWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

// WINMGR.C

BOOLEAN
YoriWinMgrAlwaysDisplayAccelerators(VOID);

LPCTSTR
YoriWinGetDrawingCharacters(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in YORI_WIN_CHARACTERS CharacterSet
    );

UCHAR
YoriWinMgrDefaultColorLookup(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in YORI_WIN_COLOR_ID ColorId
    );

BOOLEAN
YoriWinIsConhostv2(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

BOOLEAN
YoriWinIsDoubleWideCharSupported(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

VOID
YoriWinMgrRegenerateRegion(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PSMALL_RECT Rect
    );

VOID
YoriWinMgrRefreshWindowRegion(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

BOOLEAN
YoriWinMgrDisplayContents(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

BOOLEAN
YoriWinMgrIsWindowTopmostAndActive(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

VOID
YoriWinMgrNotifyWindowDestroy(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

VOID
YoriWinMgrPushWindowZOrder(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

VOID
YoriWinMgrPopWindowZOrder(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

BOOLEAN
YoriWinMgrLockMouseExclusively(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE ExclusiveWindow
    );

BOOLEAN
YoriWinMgrUnlockMouseExclusively(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE ExclusiveWindow
    );

PYORI_WIN_CTRL_HANDLE
YoriWinMgrAllocateRecurringTimer(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_CTRL Ctrl,
    __in DWORD PeriodicInterval
    );

VOID
YoriWinMgrFreeTimer(
    __in PYORI_WIN_CTRL_HANDLE TimerHandle
    );

VOID
YoriWinMgrRemoveTimersForControl(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_CTRL Ctrl
    );

__success(return)
BOOLEAN
YoriWinMgrProcessEvents(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    );

// vim:sw=4:ts=4:et:
