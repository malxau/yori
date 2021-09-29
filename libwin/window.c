/**
 * @file libwin/window.c
 *
 * Yori display an overlapping window
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

#include "yoripch.h"
#include "yorilib.h"
#include "yoriwin.h"

/**
 A forward declaration of a window pointer.  The actual definition is
 below.
 */
typedef struct _YORI_WIN_WINDOW *PYORI_WIN_WINDOW;

/**
 Set to indicate to winpriv.h that it should not define YORI_WIN_WINDOW as
 an opaque type since it has already been defined here explicitly.
 */
#define YORI_WIN_WINDOW_DEFINED 1

#include "winpriv.h"

/**
 A structure indicating how to process a particular event that occurs on a
 window if no control processes the event first.
 */
typedef struct _YORI_WIN_NOTIFY_HANDLER {
    /**
     A pointer to a function to invoke that should process this event.
     */
    PYORI_WIN_NOTIFY_EVENT Handler;
} YORI_WIN_NOTIFY_HANDLER, *PYORI_WIN_NOTIFY_HANDLER;

/**
 A structure describing a popup menu
 */
typedef struct _YORI_WIN_WINDOW {

    /**
     A common header for all controls
     */
    YORI_WIN_CTRL Ctrl;

    /**
     Pointer to the window manager.
     */
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

    /**
     The entry for this window in the stack of windows processing events in
     the window manager.
     */
    YORI_LIST_ENTRY TopLevelWindowListEntry;

    /**
     When SavedCursorInfoPresent is TRUE, this contains the state of the
     cursor when the window was launched.
     */
    CONSOLE_CURSOR_INFO SavedCursorInfo;

    /**
     When SavedCursorInfoPresent is TRUE, this contains the position of the
     cursor when the window was launched.
     */
    COORD SavedCursorPosition;

    /**
     An array of cells describing the contents of the buffer before the popup
     was drawn, which will be restored when it terminates
     */
    PCHAR_INFO SavedContents;

    /**
     An array of cells describing the current contents of the window
     */
    PCHAR_INFO Contents;

    /**
     The control that currently has keyboard focus.  This can be NULL if no
     control currently has keyboard focus.
     */
    struct _YORI_WIN_CTRL *KeyboardFocusCtrl;

    /**
     The control that would generally be invoked implicitly if the user
     presses enter on the window.  This can be temporarily overrules by
     changes in keyboard focus.
     */
    struct _YORI_WIN_CTRL *GeneralDefaultCtrl;

    /**
     The control that would generally be invoked implicitly if the user
     presses escape on the window.  This can be temporarily overrules by
     changes in keyboard focus.
     */
    struct _YORI_WIN_CTRL *GeneralCancelCtrl;

    /**
     Optionally points to a callback function to invoke if the window
     manager is resized.
     */
    PYORI_WIN_NOTIFY_WINDOW_MANAGER_RESIZE WindowManagerResizeNotifyCallback;

    /**
     An array of callbacks that can be invoked when particular events occur
     in the window, which were not processed by any control on the window.
     */
    PYORI_WIN_NOTIFY_HANDLER CustomNotifications;

    /**
     The dimensions of the window.
     */
    COORD WindowSize;

    /**
     The location of the cursor relative to the window's nonclient area.
     */
    COORD CursorPosition;

    /**
     The dimensions within the window buffer that have changed and need to
     be redrawn on the next call to redraw.  Note these are relative to the
     window's rectangle, not its client area.
     */
    SMALL_RECT DirtyRect;

    /**
     The title to display on the window.
     */
    YORI_STRING Title;

    /**
     Style flags for the window.
     */
    DWORD Style;

    /**
     Set to TRUE to indicate that YoriWinCloseWindow has been called so that
     no new input event processing should occur and the input pumping
     operation should unwind.
     */
    BOOLEAN Closing;

    /**
     Set to TRUE to indicate the window contents have changed and need to be
     redrawn.  The area to be redrawn is specified in DirtyRect below.
     */
    BOOLEAN Dirty;

    /**
     Set to TRUE to indicate that the general default control is temporarily
     suppressed from acting as a default control, because the control in
     focus will handle the default action.
     */
    BOOLEAN DefaultControlSuppressed;

    /**
     Set to TRUE to indicate that the general cancel control is temporarily
     suppressed from acting as a cancel control, because the control in
     focus will handle the cancel action.
     */
    BOOLEAN CancelControlSuppressed;

    /**
     Set to TRUE to indicate that the window is in the process of tearing
     itself down, so if controls request actions of it as part of their
     teardown the window might ignore, fail, or short circuit those
     requests.
     */
    BOOLEAN Destroying;

    /**
     If TRUE, accelerators are currently highlighted.  If FALSE, they are
     invisible.
     */
    BOOLEAN AcceleratorsDisplayed;

    /**
     If TRUE, accelerators should be invoked whether the Alt key is pressed
     or not.  This is used on simple dialogs where text entry is not needed.
     */
    BOOLEAN EnableNonAltAccelerators;

    /**
     If TRUE, the window is not visible on the screen and display updates
     should be buffered without being displayed.  If FALSE, the window is
     visible and the display will be updated on request.
     */
    BOOLEAN Hidden;

    /**
     If TRUE, SavedCursorInfo contains state about the cursor when the window
     was launched.  This can be restored when the window is destroyed.
     If FALSE, cursor info could not be populated and should not be restored.
     */
    BOOLEAN SavedCursorInfoPresent;

    /**
     Application specific context that is passed from YoriWinCloseWindow to
     the code which executes once the window has been closed.
     */
    DWORD_PTR Result;

} YORI_WIN_WINDOW;

BOOLEAN
YoriWinNotifyEvent(
    __in PYORI_WIN_CTRL WindowCtrl,
    __in PYORI_WIN_EVENT Event
    );

/**
 Return the window manager that is responsible for handling IO operations for
 this window.

 @param WindowHandle Pointer to the window.

 @return Pointer to the window manager.
 */
PYORI_WIN_WINDOW_MANAGER_HANDLE
YoriWinGetWindowManagerHandle(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_WIN_WINDOW Window;
    Window = (PYORI_WIN_WINDOW)WindowHandle;
    return Window->WinMgrHandle;
}

/**
 If a control is a top level window (it has no parent) convert the control
 pointer into a window pointer.  Note this is really doing nothing - the two
 values are the same.

 @param CtrlHandle Pointer to the control.

 @return Pointer to the window.
 */
PYORI_WIN_WINDOW_HANDLE
YoriWinGetWindowFromWindowCtrl(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl;
    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    ASSERT(Ctrl->Parent == NULL);
    return CONTAINING_RECORD(Ctrl, YORI_WIN_WINDOW, Ctrl);
}

/**
 Return the control corresponding to a top level window.

 @param WindowHandle Pointer to the window.

 @return Pointer to the control.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinGetCtrlFromWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_WIN_WINDOW Window;
    Window = (PYORI_WIN_WINDOW)WindowHandle;
    return &Window->Ctrl;
}

/**
 Return the total size (client and non client area) of the popup window.

 @param Window Pointer to the window.

 @param WindowSize On successful completion, updated to contain the size of
        the window.
 */
VOID
YoriWinGetWindowSize(
    __in PYORI_WIN_WINDOW Window,
    __out PCOORD WindowSize
    )
{
    WindowSize->X = Window->WindowSize.X;
    WindowSize->Y = Window->WindowSize.Y;
}

/**
 Return the size of the window's client area.

 @param WindowHandle Pointer to the window.

 @param Size On successful completion, updated to contain the size of the
        window's client area.
 */
VOID
YoriWinGetClientSize(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __out PCOORD Size
    )
{
    PYORI_WIN_WINDOW Window;
    Window = (PYORI_WIN_WINDOW)WindowHandle;

    Size->X = (SHORT)(Window->Ctrl.ClientRect.Right - Window->Ctrl.ClientRect.Left + 1);
    Size->Y = (SHORT)(Window->Ctrl.ClientRect.Bottom - Window->Ctrl.ClientRect.Top + 1);
}

/**
 Set the cursor visibility and shape for the window.

 @param WindowHandle Pointer to the window.

 @param Visible TRUE if the cursor should be visible, FALSE if it should be
        hidden.

 @param SizePercentage The percentage of the cell to display as the cursor.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinSetCursorState(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in BOOL Visible,
    __in DWORD SizePercentage
    )
{
    PYORI_WIN_WINDOW Window;
    CONSOLE_CURSOR_INFO CursorInfo;
    HANDLE hConOut;

    Window = (PYORI_WIN_WINDOW)WindowHandle;
    CursorInfo.bVisible = Visible;
    CursorInfo.dwSize = SizePercentage;

    hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);
    if (!SetConsoleCursorInfo(hConOut, &CursorInfo)) {
        return FALSE;
    }

    return TRUE;
}

/**
 Set the cursor location relative to the nonclient area of the window.

 @param Window Pointer to the window.

 @param NewX The horizontal coordinate relative to the nonclient area of the
        window.

 @param NewY The vertical coordinate relative to the nonclient area of the
        window.
 */
VOID
YoriWinSetCursorPosition(
    __in PYORI_WIN_WINDOW Window,
    __in WORD NewX,
    __in WORD NewY
    )
{
    COORD NewPos;
    HANDLE hConOut;

    Window->CursorPosition.X = NewX;
    Window->CursorPosition.Y = NewY;

    NewPos.X = (WORD)(NewX + Window->Ctrl.FullRect.Left);
    NewPos.Y = (WORD)(NewY + Window->Ctrl.FullRect.Top);

    hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);
    SetConsoleCursorPosition(hConOut, NewPos);
}

/**
 Determine if the window should require the Alt key to invoke accelerators.
 If EnableNonAltAccelerators is TRUE, accelerators should be invoked whether
 the Alt key is pressed or not.  This is used on simple dialogs where text
 entry is not needed.  By default, EnableNonAltAccelerators is FALSE.

 @param WindowHandle Pointer to the window to configure behavior for.

 @param EnableNonAltAccelerators The new accelerator behavior to use.
 */
VOID
YoriWinEnableNonAltAccelerators(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in BOOLEAN EnableNonAltAccelerators
    )
{
    PYORI_WIN_WINDOW Window = (PYORI_WIN_WINDOW)WindowHandle;
    Window->EnableNonAltAccelerators = EnableNonAltAccelerators;
}

/**
 Update a specified cell within a window.

 @param Window The window to update.

 @param X The horizontal coordinate of the cell to update.

 @param Y The vertical coordinate of the cell to update.

 @param Char The character to place in the cell.

 @param Attr The attributes to place in the cell.
 */
VOID
YoriWinSetWindowCell(
    __inout PYORI_WIN_WINDOW Window,
    __in WORD X,
    __in WORD Y,
    __in TCHAR Char,
    __in WORD Attr
    )
{
    PCHAR_INFO Cell;
    if (Y >= Window->WindowSize.Y || X >= Window->WindowSize.X) {
        return;
    }
    Cell = &Window->Contents[Y * Window->WindowSize.X + X];
    if (Cell->Char.UnicodeChar == Char && Cell->Attributes == Attr) {
        return;
    }
    Cell->Char.UnicodeChar = Char;
    Cell->Attributes = Attr;

    if (!Window->Dirty) {
        Window->Dirty = TRUE;
        Window->DirtyRect.Left = X;
        Window->DirtyRect.Top = Y;
        Window->DirtyRect.Right = X;
        Window->DirtyRect.Bottom = Y;
    } else {
        if (X < Window->DirtyRect.Left) {
            Window->DirtyRect.Left = X;
        } else if (X > Window->DirtyRect.Right) {
            Window->DirtyRect.Right = X;
        }

        if (Y < Window->DirtyRect.Top) {
            Window->DirtyRect.Top = Y;
        } else if (Y > Window->DirtyRect.Bottom) {
            Window->DirtyRect.Bottom = Y;
        }
    }
}

/**
 Update a specified client cell within a window.

 @param Window The window to update.

 @param X The horizontal coordinate of the cell to update.

 @param Y The vertical coordinate of the cell to update.

 @param Char The character to place in the cell.

 @param Attr The attributes to place in the cell.
 */
VOID
YoriWinSetWindowClientCell(
    __inout PYORI_WIN_WINDOW Window,
    __in WORD X,
    __in WORD Y,
    __in TCHAR Char,
    __in WORD Attr
    )
{
    if (X > (Window->Ctrl.ClientRect.Right - Window->Ctrl.ClientRect.Left) ||
        Y > (Window->Ctrl.ClientRect.Bottom - Window->Ctrl.ClientRect.Top)) {

        return;
    }

    YoriWinSetWindowCell(Window,
                         (WORD)(X + Window->Ctrl.ClientRect.Left),
                         (WORD)(Y + Window->Ctrl.ClientRect.Top),
                         Char,
                         Attr);
}


/**
 Display the window buffer into the console.

 @param WindowHandle Pointer to the window to display.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinDisplayWindowContents(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_WIN_WINDOW Window;
    COORD BufferPosition;
    SMALL_RECT RedrawWindow;
    HANDLE hConOut;

    Window = (PYORI_WIN_WINDOW)WindowHandle;

    if (!Window->Dirty || Window->Hidden) {
        return TRUE;
    }

    BufferPosition.X = Window->DirtyRect.Left;
    BufferPosition.Y = Window->DirtyRect.Top;

    RedrawWindow.Left = (SHORT)(Window->Ctrl.FullRect.Left + Window->DirtyRect.Left);
    RedrawWindow.Right = (SHORT)(Window->Ctrl.FullRect.Left + Window->DirtyRect.Right);
    RedrawWindow.Top = (SHORT)(Window->Ctrl.FullRect.Top + Window->DirtyRect.Top);
    RedrawWindow.Bottom = (SHORT)(Window->Ctrl.FullRect.Top + Window->DirtyRect.Bottom);

    hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);
    if (!WriteConsoleOutput(hConOut, Window->Contents, Window->WindowSize, BufferPosition, &RedrawWindow)) {
        return FALSE;
    }

    Window->Dirty = FALSE;

    return TRUE;
}

/**
 Look through the controls on the window and find which control is responsible
 for handling an Alt+N keyboard accelerator, and invoke that control if it is
 found.

 @param Window Pointer to the window.

 @param Char The character for the keyboard accelerator.

 @return TRUE to indicate that the accelerator was handled and no further
         processing should occur.  FALSE does not guarantee that no action
         was performed, but default handling can continue.
 */
BOOLEAN
YoriWinInvokeAccelerator(
    __in PYORI_WIN_WINDOW Window,
    __in TCHAR Char
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_CTRL Ctrl;
    YORI_WIN_EVENT CtrlEvent;
    TCHAR UpcaseChar;

    UpcaseChar = YoriLibUpcaseChar(Char);

    ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, NULL);
    while (ListEntry != NULL) {

        //
        //  Take the current control in the list and find the next list
        //  element in case the current control attempts to kill itself
        //  during notification
        //

        Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
        ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, ListEntry);

        if (Ctrl->NotifyEventFn != NULL) {
            if (Ctrl->AcceleratorChar &&
                YoriLibUpcaseChar(Ctrl->AcceleratorChar) == UpcaseChar) {

                BOOLEAN ActivateNext;
                ActivateNext = FALSE;

                // 
                //  Labels are special in that they support accelerators but
                //  the intention is to activate the next control
                //

                while (!Ctrl->CanReceiveFocus) {
                    if (ListEntry == NULL) {
                        return FALSE;
                    }

                    Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
                    ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, ListEntry);
                    ActivateNext = TRUE;
                }

                if (ActivateNext) {
                    YoriWinSetFocus(Window, Ctrl);
                } else {
                    ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
                    CtrlEvent.EventType = YoriWinEventExecute;

                    return Ctrl->NotifyEventFn(Ctrl, &CtrlEvent);
                }
            }
        }
    }

    //
    //  If the correct control hasn't been found, go through them all
    //  again with a different notification to see if any control wants to
    //  handle it
    //

    ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, NULL);
    while (ListEntry != NULL) {

        //
        //  Take the current control in the list and find the next list
        //  element in case the current control attempts to kill itself
        //  during notification
        //

        Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
        ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, ListEntry);

        if (Ctrl->NotifyEventFn != NULL) {

            ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
            CtrlEvent.EventType = YoriWinEventAccelerator;
            CtrlEvent.Accelerator.Char = UpcaseChar;

            if (Ctrl->NotifyEventFn(Ctrl, &CtrlEvent)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/**
 Save the current state of the cursor.  This is done when the window is
 created so it can be restored when the window is destroyed.

 @param WindowHandle Pointer to the window.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinSaveCursorState(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_WIN_WINDOW Window;
    HANDLE hConOut;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    Window = (PYORI_WIN_WINDOW)WindowHandle;
    hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);

    if (GetConsoleCursorInfo(hConOut, &Window->SavedCursorInfo)) {
        if (GetConsoleScreenBufferInfo(hConOut, &ScreenInfo)) {
            Window->SavedCursorPosition = ScreenInfo.dwCursorPosition;
            Window->SavedCursorInfoPresent = TRUE;
            return TRUE;
        }
    }

    return FALSE;
}


/**
 Redisplay the saved buffer of cells underneath the window, effectively
 making the window invisible.

 @param Window Pointer to the window to display the saved buffer for.
 */
VOID
YoriWinRestoreSavedWindowContents(
    __in PYORI_WIN_WINDOW Window
    )
{
    COORD BufferPosition;
    HANDLE hConOut;
    SMALL_RECT WriteRect;

    //
    //  Restore saved contents
    //

    BufferPosition.X = 0;
    BufferPosition.Y = 0;

    WriteRect.Left = Window->Ctrl.FullRect.Left;
    WriteRect.Top = Window->Ctrl.FullRect.Top;
    WriteRect.Right = Window->Ctrl.FullRect.Right;
    WriteRect.Bottom = Window->Ctrl.FullRect.Bottom;

    hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);
    WriteConsoleOutput(hConOut, Window->SavedContents, Window->WindowSize, BufferPosition, &WriteRect);
}


/**
 Clean up any internal allocations or structures used to display a window.

 @param WindowHandle Pointer to the window to clean up.
 */
VOID
YoriWinDestroyWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_WIN_WINDOW Window = (PYORI_WIN_WINDOW)WindowHandle;

    Window->Destroying = TRUE;
    YoriWinDestroyControl(&Window->Ctrl);

    if (Window->SavedContents != NULL) {
        YoriWinRestoreSavedWindowContents(Window);
        YoriLibFree(Window->SavedContents);
        Window->SavedContents = NULL;
    }

    if (Window->SavedCursorInfoPresent) {
        HANDLE hConOut;

        hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);
        SetConsoleCursorPosition(hConOut, Window->SavedCursorPosition);
        SetConsoleCursorInfo(hConOut, &Window->SavedCursorInfo);
        Window->SavedCursorInfoPresent = FALSE;
    }

    if (Window->CustomNotifications) {
        YoriLibFree(Window->CustomNotifications);
    }

    YoriLibFreeStringContents(&Window->Title);
    YoriLibDereference(Window);
}

/**
 Indicates that a modal window should stop processing input and should
 prepare for termination.

 @param WindowHandle Pointer to the window to close.

 @param Result Application specific context indicating state that can be
        passed from the code initiating a window close and the code that
        executes after the window has closed.
 */
VOID
YoriWinCloseWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in DWORD_PTR Result
    )
{
    PYORI_WIN_WINDOW Window = (PYORI_WIN_WINDOW)WindowHandle;

    Window->Closing = TRUE;
    Window->Result = Result;
}

/**
 Returns TRUE to indicate that a window intends to close itself.  Closing
 a window requires terminating message processing on the window and
 returning the result.

 @param WindowHandle Pointer to the window.

 @return TRUE if the window is intending to close, FALSE if it intends to
         remain operational.
 */
BOOLEAN
YoriWinIsWindowClosing(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_WIN_WINDOW Window = (PYORI_WIN_WINDOW)WindowHandle;
    return Window->Closing;
}

/**
 Derive what the transparent shadow color should be for a specified color.
 This means only the low four bits are meaningful; there is no background
 color specified here.

 @param Color Specifies the color that is present without a shadow.

 @return The corresponding color that should be used if this color is to
         become a transparent shadow.
 */
WORD
YoriWinTransparentColorFromColor(
    __in WORD Color
    )
{
    if (Color & FOREGROUND_INTENSITY) {
        return (WORD)(Color & (FOREGROUND_INTENSITY - 1));
    } else if (Color == (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)) {
        return FOREGROUND_INTENSITY;
    }
    return 0;
}

/**
 Derive what the transparent shadow attributes should be for a specified
 set of attributes.  The attributes here refer to both foreground and
 background.

 @param Attributes Specifies the foreground and background attributes that
        are present without a shadow.

 @return The corresponding attributes that should be used if this color is
         to become a transparent shadow.
 */
WORD
YoriWinTransparentAttributesFromAttributes(
    __in WORD Attributes
    )
{
    return (WORD)(YoriWinTransparentColorFromColor((WORD)(Attributes & 0xF)) |
                  (YoriWinTransparentColorFromColor((WORD)((Attributes >> 4) & 0xF)) << 4));
}

/**
 Clear the background of the window.  This needs to be used before redrawing
 controls, because this module does not track the regions to invalidate
 when a control is moved.

 @param Window Pointer to the window whose background should be erased.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEraseWindowBackground(
    __in PYORI_WIN_WINDOW Window
    )
{
    COORD BufferPosition;
    WORD BorderWidth;
    WORD BorderHeight;
    WORD ShadowWidth;
    WORD ShadowHeight;

    if (Window->Style & (YORI_WIN_WINDOW_STYLE_SHADOW_SOLID | YORI_WIN_WINDOW_STYLE_SHADOW_TRANSPARENT)) {
        ShadowWidth = 2;
        ShadowHeight = 1;
    } else {
        ShadowWidth = 0;
        ShadowHeight = 0;
    }

    if (Window->Style & (YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_BORDER_DOUBLE)) {
        BorderWidth = 1;
        BorderHeight = 1;
    } else {
        BorderWidth = 0;
        BorderHeight = 0;
    }

    for (BufferPosition.Y = BorderHeight; BufferPosition.Y < (SHORT)(Window->WindowSize.Y - ShadowHeight - BorderHeight); BufferPosition.Y++) {
        for (BufferPosition.X = BorderWidth; BufferPosition.X < (SHORT)(Window->WindowSize.X - ShadowWidth - BorderWidth); BufferPosition.X++) {
            Window->Contents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Attributes = Window->Ctrl.DefaultAttributes;
            Window->Contents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Char.UnicodeChar = ' ';
        }
    }

    return TRUE;
}


/**
 Render the non client area of the window.  This is the border, shadow, and
 title bar.

 @param Window Pointer to the window whose nonclient area should be rendered.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinPaintWindowNonClientArea(
    __in PYORI_WIN_WINDOW Window
    )
{
    WORD BorderWidth;
    WORD BorderHeight;
    WORD ShadowColor;
    TCHAR ShadowChar;
    WORD ShadowWidth;
    WORD ShadowHeight;
    COORD BufferPosition;
    CONST TCHAR* ShadowChars;

    ShadowChars = YoriWinGetDrawingCharacters(Window->WinMgrHandle, YoriWinCharsShadow);

    if (Window->Style & (YORI_WIN_WINDOW_STYLE_SHADOW_SOLID | YORI_WIN_WINDOW_STYLE_SHADOW_TRANSPARENT)) {
        ShadowWidth = 2;
        ShadowHeight = 1;
    } else {
        ShadowWidth = 0;
        ShadowHeight = 0;
    }

    if (Window->Style & (YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_BORDER_DOUBLE)) {
        BorderWidth = 1;
        BorderHeight = 1;
    } else {
        BorderWidth = 0;
        BorderHeight = 0;
    }

    Window->Dirty = TRUE;
    Window->DirtyRect.Left = 0;
    Window->DirtyRect.Top = 0;
    Window->DirtyRect.Right = (SHORT)(Window->WindowSize.X - 1);
    Window->DirtyRect.Bottom = (SHORT)(Window->WindowSize.Y - 1);

    //
    //  Initialize the shadow for the window
    //

    if (ShadowWidth > 0 || ShadowHeight > 0) {
        ShadowColor = (WORD)(YoriLibVtGetDefaultColor() & 0xF0 | FOREGROUND_INTENSITY);
        ShadowChar = ShadowChars[0];
        for (BufferPosition.Y = 0; BufferPosition.Y < (SHORT)Window->WindowSize.Y; BufferPosition.Y++) {
            if (BufferPosition.Y < ShadowHeight) {

                //
                //  For lines less than the shadow height, preserve the previous
                //  buffer contents into the "shadow" cells"
                //

                for (BufferPosition.X = (SHORT)(Window->WindowSize.X - ShadowWidth); BufferPosition.X < (SHORT)(Window->WindowSize.X); BufferPosition.X++) {
                    YoriWinSetWindowCell(Window,
                                         BufferPosition.X,
                                         BufferPosition.Y,
                                         Window->SavedContents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Char.UnicodeChar,
                                         Window->SavedContents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Attributes);
                }

            } else if (BufferPosition.Y >= (SHORT)(Window->WindowSize.Y - ShadowHeight)) {

                //
                //  For lines that are beneath the window and constitute its
                //  shadow, fill in the first chars to be the "previous" cells
                //  and the remainder to be a shadow
                //

                for (BufferPosition.X = 0; BufferPosition.X < ShadowWidth; BufferPosition.X++) {
                    YoriWinSetWindowCell(Window,
                                         BufferPosition.X,
                                         BufferPosition.Y,
                                         Window->SavedContents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Char.UnicodeChar,
                                         Window->SavedContents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Attributes);
                }

                for (BufferPosition.X = ShadowWidth; BufferPosition.X < (SHORT)(Window->WindowSize.X); BufferPosition.X++) {
                    if (Window->Style & YORI_WIN_WINDOW_STYLE_SHADOW_TRANSPARENT) {
                        ShadowChar = Window->SavedContents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Char.UnicodeChar;
                        ShadowColor = YoriWinTransparentAttributesFromAttributes(Window->SavedContents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Attributes);
                    }
                    YoriWinSetWindowCell(Window,
                                         BufferPosition.X,
                                         BufferPosition.Y,
                                         ShadowChar,
                                         ShadowColor);
                }
            } else {

                //
                //  For regular lines, fill in the shadow area on the right of
                //  the line
                //

                for (BufferPosition.X = (SHORT)(Window->WindowSize.X - ShadowWidth); BufferPosition.X < (SHORT)(Window->WindowSize.X); BufferPosition.X++) {
                    if (Window->Style & YORI_WIN_WINDOW_STYLE_SHADOW_TRANSPARENT) {
                        ShadowChar = Window->SavedContents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Char.UnicodeChar;
                        ShadowColor = YoriWinTransparentAttributesFromAttributes(Window->SavedContents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Attributes);
                    }
                    YoriWinSetWindowCell(Window,
                                         BufferPosition.X,
                                         BufferPosition.Y,
                                         ShadowChar,
                                         ShadowColor);
                }
            }
        }
    }


    if (Window->Style & (YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_BORDER_DOUBLE)) {

        SMALL_RECT Border;
        WORD BorderFlags;
        Border.Left = 0;
        Border.Top = 0;
        Border.Right = (SHORT)(Window->WindowSize.X - 1 - ShadowWidth);
        Border.Bottom = (SHORT)(Window->WindowSize.Y - 1 - ShadowHeight);

        BorderFlags = YORI_WIN_BORDER_TYPE_RAISED;
        if (Window->Style & YORI_WIN_WINDOW_STYLE_BORDER_SINGLE) {
            BorderFlags |= YORI_WIN_BORDER_TYPE_SINGLE;
        } else if (Window->Style & YORI_WIN_WINDOW_STYLE_BORDER_DOUBLE) {
            BorderFlags |= YORI_WIN_BORDER_TYPE_DOUBLE;
        }

        YoriWinDrawBorderOnControl(&Window->Ctrl, &Border, Window->Ctrl.DefaultAttributes, BorderFlags);

        Window->Ctrl.ClientRect.Left = (SHORT)(Border.Left + 1);
        Window->Ctrl.ClientRect.Top = (SHORT)(Border.Top + 1);
        Window->Ctrl.ClientRect.Right = (SHORT)(Border.Right - 1);
        Window->Ctrl.ClientRect.Bottom = (SHORT)(Border.Bottom - 1);
    } else if (Window->Style & (YORI_WIN_WINDOW_STYLE_SHADOW_SOLID | YORI_WIN_WINDOW_STYLE_SHADOW_TRANSPARENT)) {
        Window->Ctrl.ClientRect.Right = (SHORT)(Window->Ctrl.ClientRect.Right - ShadowWidth);
        Window->Ctrl.ClientRect.Bottom = (SHORT)(Window->Ctrl.ClientRect.Bottom - ShadowHeight);
    }

    if (Window->Title.LengthInChars > 0) {
        DWORD Index;
        DWORD Offset;
        DWORD Length;
        WORD TitleBarColor;

        Length = Window->Title.LengthInChars;
        if (Length > (DWORD)(Window->WindowSize.X - ShadowWidth - 2)) {
            Length = (DWORD)(Window->WindowSize.X - ShadowWidth - 2);
        }

        Offset = ((Window->WindowSize.X - ShadowWidth) - Length) / 2;
        TitleBarColor = YoriWinMgrDefaultColorLookup(Window->WinMgrHandle, YoriWinColorTitleBarDefault);

        for (Index = 1; Index < Offset; Index++) {
            Window->Contents[Index].Attributes = TitleBarColor;
            Window->Contents[Index].Char.UnicodeChar = ' ';
        }

        for (Index = 0; Index < Length; Index++) {
            Window->Contents[Offset + Index].Char.UnicodeChar = Window->Title.StartOfString[Index];
            Window->Contents[Offset + Index].Attributes = TitleBarColor;
        }

        for (Index = Offset + Length; Index < (DWORD)(Window->WindowSize.X - ShadowWidth - 1); Index++) {
            Window->Contents[Index].Attributes = TitleBarColor;
            Window->Contents[Index].Char.UnicodeChar = ' ';
        }
    }

    return TRUE;
}

/**
 Record the contents of the console that are in the range that the window
 will display over.

 @param Window Pointer to the window.  This should already contain an
        allocation to store the saved window contents into.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinSaveWindowContents(
    __in PYORI_WIN_WINDOW Window
    )
{
    COORD BufferPosition;
    HANDLE hConOut;
    SMALL_RECT ReadRect;

    BufferPosition.X = 0;
    BufferPosition.Y = 0;
    hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);
    ReadRect.Left = Window->Ctrl.FullRect.Left;
    ReadRect.Top = Window->Ctrl.FullRect.Top;
    ReadRect.Right = Window->Ctrl.FullRect.Right;
    ReadRect.Bottom = Window->Ctrl.FullRect.Bottom;

    if (!ReadConsoleOutput(hConOut, Window->SavedContents, Window->WindowSize, BufferPosition, &ReadRect)) {
        return FALSE;
    }

    return TRUE;
}

/**
 Display a popup window at a specified location.

 @param WinMgrHandle Pointer to the window manager.

 @param WindowRect Indicates the location of the window within the console
        screen buffer.

 @param Style Specifies style flags for the window including the border type
        to use and whether to display a shadow.

 @param Title Pointer to a string to display in the title bar.

 @param OutWindow On successful completion, updated to point to the newly
        allocated popup window.

 @return TRUE to indicate the popup window was successfully created, FALSE to
         indicate a failure.
 */
__success(return)
BOOLEAN
YoriWinCreateWindowEx(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PSMALL_RECT WindowRect,
    __in DWORD Style,
    __in_opt PYORI_STRING Title,
    __out PYORI_WIN_WINDOW_HANDLE *OutWindow
    )
{
    PYORI_WIN_WINDOW Window;
    DWORD CellCount;

    Window = YoriLibReferencedMalloc(sizeof(YORI_WIN_WINDOW));
    if (Window == NULL) {
        return FALSE;
    }

    ZeroMemory(Window, sizeof(YORI_WIN_WINDOW));
    Window->WinMgrHandle = WinMgrHandle;
    Window->Style = Style;

    Window->WindowSize.X = (SHORT)(WindowRect->Right - WindowRect->Left + 1);
    Window->WindowSize.Y = (SHORT)(WindowRect->Bottom - WindowRect->Top + 1);

    if (!YoriWinCreateControl(NULL, WindowRect, TRUE, &Window->Ctrl)) {
        YoriWinDestroyWindow(Window);
        return FALSE;
    }
    Window->Ctrl.NotifyEventFn = YoriWinNotifyEvent;

    //
    //  If there's a title, save it.
    //

    if (Title != NULL) {
        if (!YoriLibAllocateString(&Window->Title, Title->LengthInChars + 1)) {
            YoriWinDestroyWindow(Window);
            return FALSE;
        }

        Window->Title.LengthInChars = YoriLibSPrintf(Window->Title.StartOfString, _T("%y"), Title);
    }

    //
    //  Save contents at the location of the window
    //

    CellCount = Window->WindowSize.X;
    CellCount *= Window->WindowSize.Y;

    Window->SavedContents = YoriLibMalloc(CellCount * sizeof(CHAR_INFO) * 2);
    if (Window->SavedContents == NULL) {
        YoriWinDestroyWindow(Window);
        return FALSE;
    }

    Window->Contents = Window->SavedContents + CellCount;

    if (!YoriWinSaveWindowContents(Window)) {
        YoriLibFree(Window->SavedContents);
        Window->SavedContents = NULL;
        YoriWinDestroyWindow(Window);
        return FALSE;
    }

    //
    //  Initialize the new contents in the window
    //

    YoriWinEraseWindowBackground(Window);
    YoriWinPaintWindowNonClientArea(Window);

    *OutWindow = Window;
    return TRUE;
}

/**
 Find the location to display a window within the window manager.

 @param WinMgrHandle Pointer to the window manager.

 @param MinimumWidth The minimum width of the window.

 @param MinimumHeight The minimum height of the window.

 @param DesiredWidth The maximum width of the window, if space is available.

 @param DesiredHeight The maximum height of the window, if space is available.

 @param DesiredLeft The desired left offset of the window relative to the
        window manager.  If -1, the window should be horizontally centered.

 @param DesiredTop The desired top offset of the window relative to the
        window manager.  If -1, the window should be vertically centered.

 @param Style Specifies style flags for the window including the border type
        to use and whether to display a shadow.

 @param WindowRect On successful completion, populated with the location of
        the window in screen buffer coordinates.

 @return TRUE to indicate the location was successfully populated, FALSE to
         indicate failure.
 */
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
    )
{
    WORD DisplayWidth;
    WORD DisplayHeight;
    WORD BorderWidth;
    WORD BorderHeight;
    WORD ShadowWidth;
    WORD ShadowHeight;
    COORD WindowSize;
    SMALL_RECT WinMgrRect;

    if (MinimumWidth > MAXSHORT ||
        MinimumHeight > MAXSHORT ||
        DesiredWidth > MAXSHORT ||
        DesiredHeight > MAXSHORT) {

        return FALSE;
    }

    if (Style & (YORI_WIN_WINDOW_STYLE_SHADOW_SOLID | YORI_WIN_WINDOW_STYLE_SHADOW_TRANSPARENT)) {
        ShadowWidth = 2;
        ShadowHeight = 1;
    } else {
        ShadowWidth = 0;
        ShadowHeight = 0;
    }

    if (Style & (YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_BORDER_DOUBLE)) {
        BorderWidth = 1;
        BorderHeight = 1;
    } else {
        BorderWidth = 0;
        BorderHeight = 0;
    }

    //
    //  Currently a window needs a nonzero client area, so check that the
    //  dimensions aren't less than or equal to decoration
    //

    if (MinimumWidth <= ShadowWidth + 2 * BorderWidth || MinimumHeight <= ShadowHeight + 2 * BorderHeight) {
        return FALSE;
    }

    if (!YoriWinGetWinMgrLocation(WinMgrHandle, &WinMgrRect)) {
        return FALSE;
    }

    //
    //  Calculate Window location
    //

    DisplayWidth = (WORD)(WinMgrRect.Right - WinMgrRect.Left + 1);
    DisplayHeight = (WORD)(WinMgrRect.Bottom - WinMgrRect.Top + 1);

    if (DisplayWidth < MinimumWidth || DisplayHeight < MinimumHeight) {
        return FALSE;
    }

    WindowSize.X = (SHORT)DesiredWidth;
    WindowSize.Y = (SHORT)DesiredHeight;

    if (WindowSize.X > DisplayWidth) {
        WindowSize.X = DisplayWidth;
    }

    if (WindowSize.Y > DisplayHeight) {
        WindowSize.Y = DisplayHeight;
    }

    if (DesiredLeft == (WORD)-1) {
        WindowRect->Left = (WORD)((DisplayWidth - WindowSize.X) / 2 + WinMgrRect.Left);
    } else if (WinMgrRect.Left + DesiredLeft + WindowSize.X > WinMgrRect.Right) {
        WindowRect->Left = (WORD)(WinMgrRect.Right - WindowSize.X);
    } else {
        WindowRect->Left = (WORD)(WinMgrRect.Left + DesiredLeft);
    }
    WindowRect->Right = (WORD)(WindowRect->Left + WindowSize.X - 1);

    if (DesiredTop == (WORD)-1) {
        WindowRect->Top = (WORD)((DisplayHeight - WindowSize.Y) / 2 + WinMgrRect.Top);
    } else if (WinMgrRect.Top + DesiredTop + WindowSize.Y > WinMgrRect.Bottom) {
        WindowRect->Top = (WORD)(WinMgrRect.Bottom - WindowSize.Y);
    } else {
        WindowRect->Top = (WORD)(WinMgrRect.Top + DesiredTop);
    }
    WindowRect->Bottom = (WORD)(WindowRect->Top + WindowSize.Y - 1);

    return TRUE;
}

/**
 Display a popup window in the center of the console window.

 @param WinMgrHandle Pointer to the window manager.

 @param MinimumWidth The minimum width of the window.

 @param MinimumHeight The minimum height of the window.

 @param DesiredWidth The maximum width of the window, if space is available.

 @param DesiredHeight The maximum height of the window, if space is available.

 @param Style Specifies style flags for the window including the border type
        to use and whether to display a shadow.

 @param Title Pointer to a string to display in the title bar.

 @param OutWindow On successful completion, updated to point to the newly
        allocated popup window.

 @return TRUE to indicate the popup window was successfully created, FALSE to
         indicate a failure.
 */
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
    )
{
    SMALL_RECT WindowRect;

    if (!YoriWinDetermineWindowRect(WinMgrHandle, MinimumWidth, MinimumHeight, DesiredWidth, DesiredHeight, (WORD)-1, (WORD)-1, Style, &WindowRect)) {
        return FALSE;
    }

    return YoriWinCreateWindowEx(WinMgrHandle, &WindowRect, Style, Title, OutWindow);
}

/**
 Set the size and position of the window.

 @param WindowHandle Pointer to the window.

 @param WindowRect Specifies the new size and location of the window.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinWindowReposition(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in PSMALL_RECT WindowRect
    )
{
    PCHAR_INFO NewSavedContents;
    COORD NewWindowSize;
    DWORD CellCount;
    PYORI_WIN_WINDOW Window;

    Window = (PYORI_WIN_WINDOW)WindowHandle;

    NewWindowSize.X = (SHORT)(WindowRect->Right - WindowRect->Left + 1);
    NewWindowSize.Y = (SHORT)(WindowRect->Bottom - WindowRect->Top + 1);

    CellCount = NewWindowSize.X;
    CellCount *= NewWindowSize.Y;

    NewSavedContents = YoriLibMalloc(CellCount * sizeof(CHAR_INFO) * 2);
    if (NewSavedContents == NULL) {
        return FALSE;
    }

    if (!YoriWinControlReposition(&Window->Ctrl, WindowRect)) {
        YoriLibFree(NewSavedContents);
        return FALSE;
    }

    YoriLibFree(Window->SavedContents);
    Window->SavedContents = NewSavedContents;
    Window->Contents = Window->SavedContents + CellCount;
    Window->WindowSize.X = NewWindowSize.X;
    Window->WindowSize.Y = NewWindowSize.Y;

    YoriWinSaveWindowContents(Window);
    YoriWinEraseWindowBackground(Window);
    YoriWinPaintWindowNonClientArea(Window);

    return TRUE;
}


/**
 Register a control with its parent window.  This links the control in to
 the list of controls on the parent window.

 @param Window Pointer to the parent window.

 @param Ctrl Pointer to the control.
 */
VOID
YoriWinAddControlToWindow(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_CTRL Ctrl
    )
{
    if (Window->KeyboardFocusCtrl == NULL && Ctrl->CanReceiveFocus) {
        Window->KeyboardFocusCtrl = Ctrl;
    }
}

/**
 Remove a control from its parent window.  This unlinks the control from
 the list of controls on the parent window.

 @param Window Pointer to the parent window.

 @param Ctrl Pointer to the control.
 */
VOID
YoriWinRemoveControlFromWindow(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_CTRL Ctrl
    )
{
    ASSERT(Ctrl->Parent == &Window->Ctrl);
    Ctrl->Parent = NULL;

    if (Window->KeyboardFocusCtrl == Ctrl) {
        if (Window->Destroying) {
            Window->KeyboardFocusCtrl = NULL;
        } else {
            YoriWinSetFocusToNextCtrl(Window);
        }
    }

    if (Window->GeneralDefaultCtrl == Ctrl) {
        Window->GeneralDefaultCtrl = NULL;
    }

    if (Window->GeneralCancelCtrl == Ctrl) {
        Window->GeneralCancelCtrl = NULL;
    }
}

/**
 Return the current control with focus, if any.

 @param WindowHandle Pointer to the top level window.

 @return Pointer to the control that currently has focus, or NULL if no
         control has focus.
 */
PYORI_WIN_CTRL
YoriWinGetFocus(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_WIN_WINDOW Window = (PYORI_WIN_WINDOW)WindowHandle;
    return Window->KeyboardFocusCtrl;
}

/**
 Sets a specific control to be the control that currently receives keyboard
 input.

 @param Window Pointer to the window whose control in focus should change.

 @param Ctrl Pointer to the control which should receive keyboard input.
 */
VOID
YoriWinSetFocus(
    __in PYORI_WIN_WINDOW Window,
    __in_opt PYORI_WIN_CTRL Ctrl
    )
{
    YORI_WIN_EVENT Event;
    BOOLEAN Terminate = FALSE;
    PYORI_WIN_CTRL OldCtrl;

    ASSERT(Ctrl == NULL || Ctrl->CanReceiveFocus);

    OldCtrl = Window->KeyboardFocusCtrl;
    Window->KeyboardFocusCtrl = NULL;

    if (OldCtrl != NULL &&
        OldCtrl->NotifyEventFn != NULL) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventLoseFocus;

        Terminate = OldCtrl->NotifyEventFn(OldCtrl, &Event);
        if (Terminate) {
            return;
        }
    }

    if (Window->KeyboardFocusCtrl != NULL) {
        return;
    }

    Window->KeyboardFocusCtrl = Ctrl;

    if (Ctrl != NULL &&
        Ctrl->NotifyEventFn != NULL) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventGetFocus;

        Terminate = Ctrl->NotifyEventFn(Ctrl, &Event);
        if (Terminate) {
            return;
        }
    }
}

/**
 Notifies the control which should have focus when the window is created that
 it does have focus.  Note this means no event is generated to indicate a
 prior control is losing focus.

 @param WindowHandle Pointer to the window whose control in focus should be
        initialized.
 */
VOID
YoriWinSetInitialFocus(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    YORI_WIN_EVENT Event;
    BOOLEAN Terminate = FALSE;
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_WINDOW Window = (PYORI_WIN_WINDOW)WindowHandle;

    Ctrl = Window->KeyboardFocusCtrl;

    if (Ctrl != NULL &&
        Ctrl->NotifyEventFn != NULL) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventGetFocus;

        Terminate = Ctrl->NotifyEventFn(Ctrl, &Event);
        if (Terminate) {
            return;
        }
    }
}


/**
 Advance keyboard focus to the next control capable of processing keyboard
 input.

 @param Window Pointer to the window to advance focus on.
 */
VOID
YoriWinSetFocusToNextCtrl(
    __in PYORI_WIN_WINDOW Window
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORI_WIN_CTRL Ctrl;

    //
    //  Move forward from the current control
    //

    if (Window->KeyboardFocusCtrl != NULL) {
        ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, &Window->KeyboardFocusCtrl->ParentControlList);
        while (ListEntry != NULL) {
            Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
            if (Ctrl->CanReceiveFocus) {
                break;
            }
            ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, ListEntry);
        }
    }

    //
    //  If no current control or at the end of the list, move from the
    //  beginning
    //

    if (ListEntry == NULL) {
        ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, NULL);
        while (ListEntry != NULL) {
            Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
            if (Ctrl->CanReceiveFocus) {
                break;
            }
            ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, ListEntry);
        }
    }

    //
    //  If nothing found, we're done
    //

    if (ListEntry == NULL) {
        return;
    }

    Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
    YoriWinSetFocus(Window, Ctrl);
}

/**
 Move keyboard focus to the previous control capable of processing keyboard
 input.

 @param Window Pointer to the window to move focus on.
 */
VOID
YoriWinSetFocusToPreviousCtrl(
    __in PYORI_WIN_WINDOW Window
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORI_WIN_CTRL Ctrl;

    //
    //  Move backwards from the current control
    //

    if (Window->KeyboardFocusCtrl != NULL) {
        ListEntry = YoriLibGetPreviousListEntry(&Window->Ctrl.ChildControlList, &Window->KeyboardFocusCtrl->ParentControlList);
        while (ListEntry != NULL) {
            Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
            if (Ctrl->CanReceiveFocus) {
                break;
            }
            ListEntry = YoriLibGetPreviousListEntry(&Window->Ctrl.ChildControlList, ListEntry);
        }
    }

    //
    //  If no current control or at the end of the list, move from the end
    //

    if (ListEntry == NULL) {
        ListEntry = YoriLibGetPreviousListEntry(&Window->Ctrl.ChildControlList, NULL);
        while (ListEntry != NULL) {
            Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
            if (Ctrl->CanReceiveFocus) {
                break;
            }
            ListEntry = YoriLibGetPreviousListEntry(&Window->Ctrl.ChildControlList, ListEntry);
        }
    }

    //
    //  If nothing found, we're done
    //

    if (ListEntry == NULL) {
        return;
    }

    Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
    YoriWinSetFocus(Window, Ctrl);
}

/**
 Sets the specified control to be the control that should be invoked if the
 user presses enter and the focus is on a control that does not handle the
 event explicitly.

 @param Window Pointer to the window.

 @param Ctrl Pointer to the control to invoke by default when the user presses
        enter.
 */
VOID
YoriWinSetDefaultCtrl(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_CTRL Ctrl
    )
{
    YORI_WIN_EVENT Event;
    BOOLEAN Terminate = FALSE;
    PYORI_WIN_CTRL OldCtrl;

    OldCtrl = Window->GeneralDefaultCtrl;
    Window->GeneralDefaultCtrl = NULL;

    if (OldCtrl != NULL &&
        OldCtrl->NotifyEventFn != NULL &&
        !Window->DefaultControlSuppressed) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventLoseEffectiveDefault;

        Terminate = OldCtrl->NotifyEventFn(OldCtrl, &Event);
        if (Terminate) {
            return;
        }

        if (Window->GeneralDefaultCtrl != NULL) {
            return;
        }
    }

    Window->GeneralDefaultCtrl = Ctrl;

    if (Ctrl != NULL &&
        Ctrl->NotifyEventFn != NULL &&
        !Window->DefaultControlSuppressed) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventGetEffectiveDefault;

        Terminate = Ctrl->NotifyEventFn(Ctrl, &Event);
        if (Terminate) {
            return;
        }
    }
}

/**
 Indicate that the current control is capable of processing the enter key
 so the default control should not be invoked.  Note that a control can always
 process the enter key and indicate that further processing should stop, but
 this mechanism allows the control that is the default control to be notified
 that it is no longer the default control and should update its display
 accordingly.

 @param Window Pointer to the window whose default control should be
        temporarily disabled.
 */
VOID
YoriWinSuppressDefaultControl(
    __in PYORI_WIN_WINDOW Window
    )
{
    YORI_WIN_EVENT Event;
    BOOLEAN Terminate = FALSE;
    PYORI_WIN_CTRL OldCtrl;

    ASSERT(!Window->DefaultControlSuppressed);

    OldCtrl = Window->GeneralDefaultCtrl;

    if (OldCtrl != NULL &&
        OldCtrl->NotifyEventFn != NULL &&
        !Window->DefaultControlSuppressed) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventLoseEffectiveDefault;

        Terminate = OldCtrl->NotifyEventFn(OldCtrl, &Event);
        if (Terminate) {
            return;
        }
    }

    Window->DefaultControlSuppressed = TRUE;
}

/**
 Indicate that the current control will not process the enter key specially.
 This allows the control that is the default control to be notified that it
 will perform the default action, so it can update its display accordingly.

 @param Window Pointer to the window whose default control should be
        reenabled.
 */
VOID
YoriWinRestoreDefaultControl(
    __in PYORI_WIN_WINDOW Window
    )
{
    YORI_WIN_EVENT Event;
    BOOLEAN Terminate = FALSE;
    PYORI_WIN_CTRL OldCtrl;

    ASSERT(Window->DefaultControlSuppressed);

    OldCtrl = Window->GeneralDefaultCtrl;
    Window->DefaultControlSuppressed = FALSE;

    if (OldCtrl != NULL &&
        OldCtrl->NotifyEventFn != NULL) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventGetEffectiveDefault;

        Terminate = OldCtrl->NotifyEventFn(OldCtrl, &Event);
        if (Terminate) {
            return;
        }
    }
}


/**
 Sets the specified control to be the control that should be invoked if the
 user presses escape and the focus is on a control that does not handle the
 event explicitly.

 @param Window Pointer to the window.

 @param Ctrl Pointer to the control to invoke by default when the user presses
        escape.
 */
VOID
YoriWinSetCancelCtrl(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_CTRL Ctrl
    )
{
    YORI_WIN_EVENT Event;
    BOOLEAN Terminate = FALSE;
    PYORI_WIN_CTRL OldCtrl;

    OldCtrl = Window->GeneralCancelCtrl;
    Window->GeneralCancelCtrl = NULL;

    if (OldCtrl != NULL &&
        OldCtrl->NotifyEventFn != NULL &&
        !Window->CancelControlSuppressed) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventLoseEffectiveCancel;

        Terminate = OldCtrl->NotifyEventFn(OldCtrl, &Event);
        if (Terminate) {
            return;
        }

        if (Window->GeneralCancelCtrl != NULL) {
            return;
        }
    }

    Window->GeneralCancelCtrl = Ctrl;

    if (Ctrl != NULL &&
        Ctrl->NotifyEventFn != NULL &&
        !Window->CancelControlSuppressed) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventGetEffectiveCancel;

        Terminate = Ctrl->NotifyEventFn(Ctrl, &Event);
        if (Terminate) {
            return;
        }
    }
}

/**
 Indicate that the current control is capable of processing the escape key
 so the cancel control should not be invoked.  Note that a control can always
 process the escape key and indicate that further processing should stop, but
 this mechanism allows the control that is the cancel control to be notified
 that it is no longer the cancel control and should update its display
 accordingly.

 @param Window Pointer to the window whose cancel control should be
        temporarily disabled.
 */
VOID
YoriWinSuppressCancelControl(
    __in PYORI_WIN_WINDOW Window
    )
{
    YORI_WIN_EVENT Event;
    BOOLEAN Terminate = FALSE;
    PYORI_WIN_CTRL OldCtrl;

    ASSERT(!Window->CancelControlSuppressed);

    OldCtrl = Window->GeneralCancelCtrl;

    if (OldCtrl != NULL &&
        OldCtrl->NotifyEventFn != NULL &&
        !Window->CancelControlSuppressed) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventLoseEffectiveCancel;

        Terminate = OldCtrl->NotifyEventFn(OldCtrl, &Event);
        if (Terminate) {
            return;
        }
    }

    Window->CancelControlSuppressed = TRUE;
}

/**
 Indicate that the current control will not process the escape key specially.
 This allows the control that is the cancel control to be notified that it
 will perform the cancel action, so it can update its display accordingly.

 @param Window Pointer to the window whose cancel control should be reenabled.
 */
VOID
YoriWinRestoreCancelControl(
    __in PYORI_WIN_WINDOW Window
    )
{
    YORI_WIN_EVENT Event;
    BOOLEAN Terminate = FALSE;
    PYORI_WIN_CTRL OldCtrl;

    ASSERT(Window->CancelControlSuppressed);

    OldCtrl = Window->GeneralCancelCtrl;
    Window->CancelControlSuppressed = FALSE;

    if (OldCtrl != NULL &&
        OldCtrl->NotifyEventFn != NULL) {

        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventGetEffectiveCancel;

        Terminate = OldCtrl->NotifyEventFn(OldCtrl, &Event);
        if (Terminate) {
            return;
        }
    }
}

/**
 Set a callback to invoke if the window manager is resized, typically meaning
 the user resized the window.

 @param WindowHandle Pointer to the window to invoke a callback from when the
        window manager changes size.

 @param NotifyCallback A function to invoke when the window manager changes
        size.

 @return TRUE to indicate success, FALSE to indicate failure.  Note though
         that this program cannot prevent the resize operation.
 */
BOOLEAN
YoriWinSetWindowManagerResizeNotifyCallback(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __in PYORI_WIN_NOTIFY_WINDOW_MANAGER_RESIZE NotifyCallback
    )
{
    PYORI_WIN_WINDOW Window;
    Window = (PYORI_WIN_WINDOW)WindowHandle;

    if (Window->WindowManagerResizeNotifyCallback != NULL) {
        return FALSE;
    }

    Window->WindowManagerResizeNotifyCallback = NotifyCallback;
    return TRUE;
}

/**
 Set a callback to be invoked when an event occurs on the window that is not
 explicitly handled by a control.  As of this writing, only one callback can
 be added per type of event, so this is only useful when a popup window is
 used by a control temporarily, such as a popup menu or pull down list.

 @param Window Pointer to the window to add a notification handler to.

 @param EventType Indicates the type of event that the notification handler
        should be used for.

 @param Handler Pointer to a function to invoke when the event occurs.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinSetCustomNotification(
    __in PYORI_WIN_WINDOW Window,
    __in DWORD EventType,
    __in PYORI_WIN_NOTIFY_EVENT Handler
    )
{
    if (Window->CustomNotifications == NULL) {
        Window->CustomNotifications = YoriLibMalloc(sizeof(YORI_WIN_NOTIFY_HANDLER) * YoriWinEventBeyondMax);
        if (Window->CustomNotifications == NULL) {
            return FALSE;
        }

        ZeroMemory(Window->CustomNotifications, sizeof(YORI_WIN_NOTIFY_HANDLER) * YoriWinEventBeyondMax);
    }

    if (EventType >= YoriWinEventBeyondMax) {
        return FALSE;
    }

    Window->CustomNotifications[EventType].Handler = Handler;
    return TRUE;
}


/**
 A function to invoke when input events occur on the window.  Generally this
 function will identify the control to handle the input event, and send it
 there.

 @param WindowCtrl Pointer to control structure within the window.

 @param Event Pointer to the event.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate that regular processing
         should continue (although this does not imply that no processing
         has already occurred.)
 */
BOOLEAN
YoriWinNotifyEvent(
    __in PYORI_WIN_CTRL WindowCtrl,
    __in PYORI_WIN_EVENT Event
    )
{
    BOOLEAN Terminate;
    PYORI_WIN_WINDOW Window;
    YORI_WIN_EVENT CtrlEvent;

    Window = CONTAINING_RECORD(WindowCtrl, YORI_WIN_WINDOW, Ctrl);

    if (Event->EventType == YoriWinEventKeyDown ||
        Event->EventType == YoriWinEventKeyUp) {

        DWORD EffectiveCtrlMask;

        //
        //  If a control has focus, send the event to it.  Translate
        //  VK_DIVIDE which reports as ENHANCED_KEY, unlike all the similar
        //  keys on the number pad.
        //

        if (Window->KeyboardFocusCtrl != NULL &&
            Window->KeyboardFocusCtrl->NotifyEventFn != NULL) {

            ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
            CtrlEvent.EventType = Event->EventType;
            CtrlEvent.KeyDown.CtrlMask = Event->KeyDown.CtrlMask;
            CtrlEvent.KeyDown.VirtualKeyCode = Event->KeyDown.VirtualKeyCode;
            CtrlEvent.KeyDown.VirtualScanCode = Event->KeyDown.VirtualScanCode;
            CtrlEvent.KeyDown.Char = Event->KeyDown.Char;

            if (CtrlEvent.KeyDown.VirtualKeyCode == VK_DIVIDE &&
                (CtrlEvent.KeyDown.CtrlMask & ENHANCED_KEY) != 0) {
                CtrlEvent.KeyDown.CtrlMask = CtrlEvent.KeyDown.CtrlMask & ~(ENHANCED_KEY);
            }

            Terminate = Window->KeyboardFocusCtrl->NotifyEventFn(Window->KeyboardFocusCtrl, &CtrlEvent);
            if (Terminate) {
                return Terminate;
            }
        }

        EffectiveCtrlMask = 0;
        Terminate = FALSE;
        if (Event->EventType == YoriWinEventKeyDown) {
            EffectiveCtrlMask = (Event->KeyDown.CtrlMask & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED));
        } else {
            EffectiveCtrlMask = (Event->KeyUp.CtrlMask & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED));
        }

        if (Event->EventType == YoriWinEventKeyDown &&
            EffectiveCtrlMask == 0) {

            if ((Event->KeyDown.VirtualKeyCode == VK_RETURN) &&
                Window->GeneralDefaultCtrl != NULL &&
                !Window->DefaultControlSuppressed) {

                ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
                CtrlEvent.EventType = YoriWinEventExecute;

                Terminate = Window->GeneralDefaultCtrl->NotifyEventFn(Window->GeneralDefaultCtrl, &CtrlEvent);
            } else  if ((Event->KeyDown.VirtualKeyCode == VK_ESCAPE) &&
                         Window->GeneralCancelCtrl != NULL) {

                ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
                CtrlEvent.EventType = YoriWinEventExecute;

                Terminate = Window->GeneralCancelCtrl->NotifyEventFn(Window->GeneralCancelCtrl, &CtrlEvent);
            } else if (Event->KeyDown.VirtualKeyCode == VK_TAB) {
                YoriWinSetFocusToNextCtrl(Window);
            } else if (Window->EnableNonAltAccelerators) {
                Terminate = YoriWinInvokeAccelerator(Window, Event->KeyDown.Char);
            }

            if (Terminate) {
                return Terminate;
            }
        } else if (Event->EventType == YoriWinEventKeyDown &&
                   EffectiveCtrlMask == SHIFT_PRESSED) {

            if (Event->KeyDown.VirtualKeyCode == VK_TAB) {
                YoriWinSetFocusToPreviousCtrl(Window);
            }
        } else if (Event->EventType == YoriWinEventKeyDown &&
                   (EffectiveCtrlMask == LEFT_ALT_PRESSED ||
                    EffectiveCtrlMask == RIGHT_ALT_PRESSED)) {

            if (Event->KeyDown.VirtualKeyCode == VK_MENU) {
                if (!Window->AcceleratorsDisplayed) {
                    ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
                    CtrlEvent.EventType = YoriWinEventDisplayAccelerators;
                    YoriWinNotifyAllControls(&Window->Ctrl, &CtrlEvent);
                    Window->AcceleratorsDisplayed = TRUE;
                }
            } else {
                if (Event->KeyDown.Char) {
                    if (Window->AcceleratorsDisplayed) {
                        ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
                        CtrlEvent.EventType = YoriWinEventHideAccelerators;
                        YoriWinNotifyAllControls(&Window->Ctrl, &CtrlEvent);
                        Window->AcceleratorsDisplayed = FALSE;
                        YoriWinDisplayWindowContents(Window);
                    }
                    Terminate = YoriWinInvokeAccelerator(Window, Event->KeyDown.Char);
                    if (Terminate) {
                        return Terminate;
                    }
                }
            }
        } else if (Event->EventType == YoriWinEventKeyUp) {
            if (Event->KeyDown.VirtualKeyCode == VK_MENU &&
                Window->AcceleratorsDisplayed) {

                ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
                CtrlEvent.EventType = YoriWinEventHideAccelerators;
                YoriWinNotifyAllControls(&Window->Ctrl, &CtrlEvent);
                Window->AcceleratorsDisplayed = FALSE;
            }
        }

        //
        //  If it's a Ctrl or Alt keyboard combination or it's a function
        //  key, look through all controls for a hotkey
        //

        if (Event->EventType == YoriWinEventKeyDown &&
            ((EffectiveCtrlMask & ~(SHIFT_PRESSED)) != 0 ||
             (Event->KeyDown.VirtualKeyCode >= VK_F1 &&
              Event->KeyDown.VirtualKeyCode <= VK_F12))) {

            ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
            CtrlEvent.EventType = YoriWinEventHotKeyDown;
            CtrlEvent.KeyDown.CtrlMask = EffectiveCtrlMask;
            CtrlEvent.KeyDown.VirtualKeyCode = Event->KeyDown.VirtualKeyCode;
            CtrlEvent.KeyDown.VirtualScanCode = Event->KeyDown.VirtualScanCode;
            CtrlEvent.KeyDown.Char = Event->KeyDown.Char;
            YoriWinNotifyAllControls(&Window->Ctrl, &CtrlEvent);
        }

    } else if (Event->EventType == YoriWinEventMouseDownInClient ||
               Event->EventType == YoriWinEventMouseDoubleClickInClient) {

        PYORI_WIN_CTRL Ctrl;
        COORD ChildLocation;
        BOOLEAN InChildClientArea;
        Ctrl = YoriWinFindControlAtCoordinates(&Window->Ctrl,
                                               Event->MouseDown.Location,
                                               TRUE,
                                               &ChildLocation,
                                               &InChildClientArea);

        if (Ctrl != NULL &&
            YoriWinTranslateMouseEventForChild(Event, Ctrl, ChildLocation, InChildClientArea)) {

            return TRUE;
        }

    } else if (Event->EventType == YoriWinEventMouseDownInNonClient) {

        PYORI_WIN_CTRL Ctrl;
        COORD ChildLocation;
        BOOLEAN InChildClientArea;
        Ctrl = YoriWinFindControlAtCoordinates(&Window->Ctrl,
                                               Event->MouseDown.Location,
                                               FALSE,
                                               &ChildLocation,
                                               &InChildClientArea);

        if (Ctrl != NULL &&
            YoriWinTranslateMouseEventForChild(Event, Ctrl, ChildLocation, InChildClientArea)) {

            return TRUE;
        }

    } else if (Event->EventType == YoriWinEventMouseUpInClient ||
               Event->EventType == YoriWinEventMouseUpInNonClient ||
               Event->EventType == YoriWinEventMouseUpOutsideWindow) {

        PYORI_WIN_CTRL CtrlUnderMouse = NULL;
        PYORI_LIST_ENTRY ListEntry;
        PYORI_WIN_CTRL ChildCtrl;
        COORD ChildLocation;
        BOOLEAN InChildClientArea = FALSE;

        //
        //  See if the mouse was released above a control.
        //

        ChildLocation.X = 0;
        ChildLocation.Y = 0;
        Terminate = FALSE;

        if (Event->EventType == YoriWinEventMouseUpInClient) {
            CtrlUnderMouse = YoriWinFindControlAtCoordinates(&Window->Ctrl,
                                                             Event->MouseUp.Location,
                                                             TRUE,
                                                             &ChildLocation,
                                                             &InChildClientArea);
        }

        //
        //  Scan through all controls seeing which ones have observed the
        //  mouse button press that is being unpressed.  If it's the control
        //  that the mouse is over, use the regular translation to ensure
        //  the coordinates are translated correctly.  If not, a
        //  YoriWinEventMouseUpOutsideWindow event is generated which cannot
        //  specify control relative coordinates because the mouse is outside
        //  of those boundaries.
        //

        ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, NULL);
        while (ListEntry != NULL) {
            ChildCtrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
            ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, ListEntry);

            if (ChildCtrl->MouseButtonsPressed & Event->MouseUp.ButtonsReleased) {
                if (ChildCtrl == CtrlUnderMouse) {
                    Terminate = YoriWinTranslateMouseEventForChild(Event, ChildCtrl, ChildLocation, InChildClientArea);
                } else if (ChildCtrl->NotifyEventFn != NULL) {
                    ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
                    CtrlEvent.EventType = YoriWinEventMouseUpOutsideWindow;
                    CtrlEvent.MouseUp.ButtonsReleased = ChildCtrl->MouseButtonsPressed & Event->MouseUp.ButtonsReleased;
                    CtrlEvent.MouseUp.ControlKeyState = Event->MouseUp.ControlKeyState;
                    ChildCtrl->MouseButtonsPressed = (UCHAR)(ChildCtrl->MouseButtonsPressed & ~(Event->MouseUp.ButtonsReleased));
                    Terminate = ChildCtrl->NotifyEventFn(ChildCtrl, &CtrlEvent);
                }
                if (Terminate) {
                    return TRUE;
                }
            }
        }

    } else if (Event->EventType == YoriWinEventMouseMoveInClient ||
               Event->EventType == YoriWinEventMouseMoveInNonClient ||
               Event->EventType == YoriWinEventMouseMoveOutsideWindow) {

        //
        //  Supporting MouseMoveOutsideWindow to child controls requires
        //  looping through all of the controls, looking for those that have
        //  observed a button press.  Those controls are notified of the move.
        //  Translating the input into the control relative form requires a
        //  different scheme for each input event.  There are three cases:
        //
        //  1. This event is InClient.  The loop examining controls which have
        //     observed mouse presses can calculate Above/Below/Left/Right by
        //     comparing the client coordinates in the event to the control
        //     position.
        //  2. This event is NonClient.  The loop examining controls needs to
        //     translate the control's location into nonclient coords to
        //     compare and calculate ABLR.
        //  3. This event is OutsideWindow.  The loop examining controls can
        //     infer that if the event has any of ABLR set those apply to the
        //     child.  If neither of AB is set, or neither of LR is set, it
        //     needs to compare the location value against the nonclient
        //     coords of the control.
        //
        //  To simplify these cases, the InClient event has its coordinates
        //  converted to nonclient, so all three cases have the same input
        //  coordinates.  Any control encountered has its coordinates
        //  converted to nonclient, so we can handle controls inside and
        //  outside the client area.
        //

        PYORI_WIN_CTRL CtrlUnderMouse = NULL;
        PYORI_LIST_ENTRY ListEntry;
        PYORI_WIN_CTRL ChildCtrl;
        YORI_WIN_BOUNDED_COORD WinPos;
        DWORD CtrlKeyState;

        WinPos.Above = FALSE;
        WinPos.Below = FALSE;
        WinPos.Left = FALSE;
        WinPos.Right = FALSE;

        //
        //  Check if the mouse is moving across a control.  If so, pass the
        //  event to that control.
        //

        if (Event->EventType == YoriWinEventMouseMoveInClient ||
            Event->EventType == YoriWinEventMouseMoveInNonClient) {

            BOOLEAN InChildClientArea;
            BOOLEAN LocationInParentClient;
            COORD ChildLocation;

            if (Event->EventType == YoriWinEventMouseMoveInClient) {
                LocationInParentClient = TRUE;
            } else {
                LocationInParentClient = FALSE;
            }

            memcpy(&CtrlEvent, Event, sizeof(YORI_WIN_EVENT));
            CtrlUnderMouse = YoriWinFindControlAtCoordinates(&Window->Ctrl,
                                                             CtrlEvent.MouseMove.Location,
                                                             LocationInParentClient,
                                                             &ChildLocation,
                                                             &InChildClientArea);

            if (CtrlUnderMouse != NULL &&
                YoriWinTranslateMouseEventForChild(&CtrlEvent, CtrlUnderMouse, ChildLocation, InChildClientArea)) {

                return TRUE;
            }
            WinPos.Pos.X = Event->MouseMove.Location.X;
            WinPos.Pos.Y = Event->MouseMove.Location.Y;
            if (Event->EventType == YoriWinEventMouseMoveInClient) {
                WinPos.Pos.X = (SHORT)(WinPos.Pos.X + Window->Ctrl.ClientRect.Left);
                WinPos.Pos.Y = (SHORT)(WinPos.Pos.Y + Window->Ctrl.ClientRect.Top);
            }
            CtrlKeyState = Event->MouseMove.ControlKeyState;
        } else {
            WinPos.Left = Event->MouseMoveOutsideWindow.Location.Left;
            WinPos.Right = Event->MouseMoveOutsideWindow.Location.Right;
            WinPos.Above = Event->MouseMoveOutsideWindow.Location.Above;
            WinPos.Below = Event->MouseMoveOutsideWindow.Location.Below;
            WinPos.Pos.X = Event->MouseMoveOutsideWindow.Location.Pos.X;
            WinPos.Pos.Y = Event->MouseMoveOutsideWindow.Location.Pos.Y;
            CtrlKeyState = Event->MouseMoveOutsideWindow.ControlKeyState;
        }

        //
        //  Scan through all controls seeing which ones have observed a mouse
        //  button press.  Notify all such controls of the mouse movement.
        //

        ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, NULL);
        while (ListEntry != NULL) {
            ChildCtrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
            ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, ListEntry);

            if (ChildCtrl != CtrlUnderMouse &&
                ChildCtrl->MouseButtonsPressed &&
                ChildCtrl->NotifyEventFn != NULL) {

                SMALL_RECT CtrlRect;

                YoriWinGetControlNonClientRegion(ChildCtrl, &CtrlRect);

                ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
                CtrlEvent.EventType = YoriWinEventMouseMoveOutsideWindow;

                YoriWinBoundCoordInSubRegion(&WinPos, &CtrlRect, &CtrlEvent.MouseMoveOutsideWindow.Location);
                CtrlEvent.MouseMoveOutsideWindow.ControlKeyState = CtrlKeyState;

                Terminate = ChildCtrl->NotifyEventFn(ChildCtrl, &CtrlEvent);
                if (Terminate) {
                    return TRUE;
                }
            }
        }

    } else if (Event->EventType == YoriWinEventMouseWheelUpInClient ||
               Event->EventType == YoriWinEventMouseWheelDownInClient) {

        PYORI_WIN_CTRL Ctrl;
        COORD ChildLocation;
        BOOLEAN InChildClientArea;

        Ctrl = YoriWinFindControlAtCoordinates(&Window->Ctrl,
                                               Event->MouseWheel.Location,
                                               TRUE,
                                               &ChildLocation,
                                               &InChildClientArea);

        if (Ctrl != NULL &&
            YoriWinTranslateMouseEventForChild(Event, Ctrl, ChildLocation, InChildClientArea)) {

            return TRUE;
        }

    } else if (Event->EventType == YoriWinEventExecute) {
        if (Window->GeneralDefaultCtrl != NULL &&
            !Window->DefaultControlSuppressed) {

            Terminate = Window->GeneralDefaultCtrl->NotifyEventFn(Window->GeneralDefaultCtrl, Event);
            if (Terminate) {
                return TRUE;
            }
        }
    } else if (Event->EventType == YoriWinEventHideWindow) {
        ASSERT(!Window->Hidden);
        Window->Hidden = TRUE;
        YoriWinRestoreSavedWindowContents(Window);
    } else if (Event->EventType == YoriWinEventShowWindow) {

        //
        //  MSFIX This also wants to re-capture the saved window contents,
        //  since the reason for this dance is to allow underlying windows
        //  to change while higher windows are redisplayed on top
        //

        ASSERT(Window->Hidden);
        Window->Hidden = FALSE;
        Window->Dirty = TRUE;
        Window->DirtyRect.Left = 0;
        Window->DirtyRect.Top = 0;
        Window->DirtyRect.Right = Window->WindowSize.X;
        Window->DirtyRect.Bottom = Window->WindowSize.Y;
        {
            COORD NewPos;
            HANDLE hConOut;

            NewPos.X = (WORD)(Window->CursorPosition.X + Window->Ctrl.FullRect.Left);
            NewPos.Y = (WORD)(Window->CursorPosition.Y + Window->Ctrl.FullRect.Top);

            hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);
            SetConsoleCursorPosition(hConOut, NewPos);
        }
    } else if (Event->EventType == YoriWinEventWindowManagerResize) {
        if (Window->WindowManagerResizeNotifyCallback != NULL) {
            Window->WindowManagerResizeNotifyCallback(Window, &Event->WindowManagerResize.OldWinMgrDimensions, &Event->WindowManagerResize.NewWinMgrDimensions);
        }
    }

    if (Window->CustomNotifications != NULL &&
        Event->EventType >= 0 &&
        Event->EventType < YoriWinEventBeyondMax &&
        Window->CustomNotifications[Event->EventType].Handler != NULL) {

        Terminate = Window->CustomNotifications[Event->EventType].Handler(&Window->Ctrl, Event);
        if (Terminate) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 Return a window handle to a window that owns the list element recording
 a window within the list of top level windows that process events.

 @param ListEntry Pointer to the TopLevelWindowListEntry field within a
        window structure.

 @return Pointer to the window.
 */
PYORI_WIN_WINDOW_HANDLE
YoriWinWindowFromTopLevelListEntry(
    __in PYORI_LIST_ENTRY ListEntry
    )
{
    return CONTAINING_RECORD(ListEntry, YORI_WIN_WINDOW, TopLevelWindowListEntry);
}


/**
 Process the input events from the system and send them to the window and
 its controls for processing.

 @param WindowHandle Pointer to the window to process input events for.

 @param Result Optionally points to a memory location which can receive
        context passed from the code which closed the window.  Note this is
        only meaningful when this function returns TRUE.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinProcessInputForWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __out_opt PDWORD_PTR Result
    )
{
    PYORI_WIN_WINDOW Window;

    Window = (PYORI_WIN_WINDOW)WindowHandle;

    YoriWinMgrPushWindow(Window->WinMgrHandle, &Window->TopLevelWindowListEntry);
    if (YoriWinMgrProcessEvents(Window->WinMgrHandle, Window)) {
        YoriWinMgrPopWindow(Window->WinMgrHandle, &Window->TopLevelWindowListEntry);
        if (Result != NULL) {
            *Result = Window->Result;
        }
        return TRUE;
    }
    YoriWinMgrPopWindow(Window->WinMgrHandle, &Window->TopLevelWindowListEntry);

    return FALSE;
}

// vim:sw=4:ts=4:et:
