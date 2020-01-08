/**
 * @file libwin/window.c
 *
 * Yori display an overlapping window
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
     An array of callbacks that can be invoked when particular events occur
     in the window, which were not processed by any control on the window.
     */
    PYORI_WIN_NOTIFY_HANDLER CustomNotifications;

    /**
     The dimensions of the window.
     */
    COORD WindowSize;

    /**
     The dimensions within the window buffer that have changed and need to
     be redrawn on the next call to redraw.  Note these are relative to the
     window's rectangle, not its client area.
     */
    SMALL_RECT DirtyRect;

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
     Application specific context that is passed from YoriWinCloseWindow to
     the code which executes once the window has been closed.
     */
    DWORD_PTR Result;

} YORI_WIN_WINDOW;

BOOLEAN
YoriWinNotifyEvent(
    __in PYORI_WIN_WINDOW Window,
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

 @param Ctrl Pointer to the control.

 @return Pointer to the window.
 */
PYORI_WIN_WINDOW
YoriWinGetWindowFromWindowCtrl(
    __in PYORI_WIN_CTRL Ctrl
    )
{
    ASSERT(Ctrl->Parent == NULL);
    return CONTAINING_RECORD(Ctrl, YORI_WIN_WINDOW, Ctrl);
}

/**
 Return the control corresponding to a top level window.

 @param Window Pointer to the window.

 @return Pointer to the control.
 */
PYORI_WIN_CTRL
YoriWinGetCtrlFromWindow(
    __in PYORI_WIN_WINDOW Window
    )
{
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
BOOL
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
    return SetConsoleCursorInfo(hConOut, &CursorInfo);
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

    NewPos.X = (WORD)(NewX + Window->Ctrl.FullRect.Left);
    NewPos.Y = (WORD)(NewY + Window->Ctrl.FullRect.Top);

    hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);
    SetConsoleCursorPosition(hConOut, NewPos);
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
    Cell = &Window->Contents[Y * Window->WindowSize.X + X];
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

 @param Window Pointer to the window to display.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriWinDisplayWindowContents(
    __in PYORI_WIN_WINDOW Window
    )
{
    COORD BufferPosition;
    SMALL_RECT RedrawWindow;
    HANDLE hConOut;

    if (!Window->Dirty) {
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


    ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, NULL);
    while (ListEntry != NULL) {

        //
        //  Take the current control in the list and find the next list
        //  element in case the current control attempts to kill itself
        //  during notification
        //

        Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
        ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, ListEntry);

        if (Ctrl->NotifyEventFn != NULL &&
            Ctrl->AcceleratorChar &&
            YoriLibUpcaseChar(Ctrl->AcceleratorChar) == YoriLibUpcaseChar(Char)) {

            while (!Ctrl->CanReceiveFocus) {
                if (ListEntry == NULL) {
                    return FALSE;
                }

                Ctrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
                ListEntry = YoriLibGetNextListEntry(&Window->Ctrl.ChildControlList, ListEntry);
            }

            ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
            CtrlEvent.EventType = YoriWinEventExecute;

            return Ctrl->NotifyEventFn(Ctrl, &CtrlEvent);
        }
    }

    return FALSE;
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
    HANDLE hConOut;

    Window->Destroying = TRUE;
    YoriWinDestroyControl(&Window->Ctrl);

    if (Window->SavedContents != NULL) {

        COORD BufferPosition;

        //
        //  Restore saved contents
        //

        BufferPosition.X = 0;
        BufferPosition.Y = 0;

        hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);
        WriteConsoleOutput(hConOut, Window->SavedContents, Window->WindowSize, BufferPosition, &Window->Ctrl.FullRect);

        YoriLibFree(Window->SavedContents);
        Window->SavedContents = NULL;
    }

    if (Window->CustomNotifications) {
        YoriLibFree(Window->CustomNotifications);
    }

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
BOOL
YoriWinCreateWindowEx(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PSMALL_RECT WindowRect,
    __in DWORD Style,
    __in_opt PYORI_STRING Title,
    __out PYORI_WIN_WINDOW_HANDLE *OutWindow
    )
{
    PYORI_WIN_WINDOW Window;
    HANDLE hConOut;

    WORD BorderWidth;
    WORD BorderHeight;
    WORD ShadowColor;
    WORD ShadowWidth;
    WORD ShadowHeight;
    DWORD CellCount;
    COORD BufferPosition;

    if (Style & YORI_WIN_WINDOW_STYLE_SHADOW) {
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

    Window = YoriLibReferencedMalloc(sizeof(YORI_WIN_WINDOW));
    if (Window == NULL) {
        return FALSE;
    }

    ZeroMemory(Window, sizeof(YORI_WIN_WINDOW));
    Window->WinMgrHandle = WinMgrHandle;

    Window->WindowSize.X = (SHORT)(WindowRect->Right - WindowRect->Left + 1);
    Window->WindowSize.Y = (SHORT)(WindowRect->Bottom - WindowRect->Top + 1);

    if (!YoriWinCreateControl(NULL, WindowRect, TRUE, &Window->Ctrl)) {
        YoriWinDestroyWindow(Window);
        return FALSE;
    }
    Window->Ctrl.NotifyEventFn = YoriWinNotifyEvent;

    //
    //  Save contents at the location of the window
    //

    BufferPosition.X = 0;
    BufferPosition.Y = 0;

    CellCount = Window->WindowSize.X;
    CellCount *= Window->WindowSize.Y;

    Window->SavedContents = YoriLibMalloc(CellCount * sizeof(CHAR_INFO) * 2);
    if (Window->SavedContents == NULL) {
        YoriWinDestroyWindow(Window);
        return FALSE;
    }

    hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);
    Window->Contents = Window->SavedContents + CellCount;

    if (!ReadConsoleOutput(hConOut, Window->SavedContents, Window->WindowSize, BufferPosition, &Window->Ctrl.FullRect)) {
        YoriLibFree(Window->SavedContents);
        Window->SavedContents = NULL;
        YoriWinDestroyWindow(Window);
        return FALSE;
    }

    //
    //  Initialize the new contents in the window
    //

    for (BufferPosition.Y = 0; BufferPosition.Y < (SHORT)(Window->WindowSize.Y - ShadowHeight); BufferPosition.Y++) {
        for (BufferPosition.X = 0; BufferPosition.X < (SHORT)(Window->WindowSize.X - ShadowWidth); BufferPosition.X++) {
            Window->Contents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Attributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
            Window->Contents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Char.UnicodeChar = ' ';
        }
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
                                         Window->SavedContents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Attributes );
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
                                         Window->SavedContents[BufferPosition.Y * Window->WindowSize.X + BufferPosition.X].Attributes );
                }

                for (BufferPosition.X = ShadowWidth; BufferPosition.X < (SHORT)(Window->WindowSize.X); BufferPosition.X++) {
                    YoriWinSetWindowCell(Window,
                                         BufferPosition.X,
                                         BufferPosition.Y,
                                         0x2591,
                                         ShadowColor);
                }
            } else {

                //
                //  For regular lines, fill in the shadow area on the right of
                //  the line
                //

                for (BufferPosition.X = (SHORT)(Window->WindowSize.X - ShadowWidth); BufferPosition.X < (SHORT)(Window->WindowSize.X); BufferPosition.X++) {
                    YoriWinSetWindowCell(Window,
                                         BufferPosition.X,
                                         BufferPosition.Y,
                                         0x2591,
                                         ShadowColor);
                }
            }
        }
    }


    if (Style & (YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_BORDER_DOUBLE)) {

        SMALL_RECT Border;
        WORD BorderFlags;
        Border.Left = 0;
        Border.Top = 0;
        Border.Right = (SHORT)(Window->WindowSize.X - 1 - ShadowWidth);
        Border.Bottom = (SHORT)(Window->WindowSize.Y - 1 - ShadowHeight);

        BorderFlags = YORI_WIN_BORDER_TYPE_RAISED;
        if (Style & YORI_WIN_WINDOW_STYLE_BORDER_SINGLE) {
            BorderFlags |= YORI_WIN_BORDER_TYPE_SINGLE;
        } else if (Style & YORI_WIN_WINDOW_STYLE_BORDER_DOUBLE) {
            BorderFlags |= YORI_WIN_BORDER_TYPE_DOUBLE;
        }

        YoriWinDrawBorderOnWindow(Window, &Border, Window->Ctrl.DefaultAttributes, BorderFlags);

        Window->Ctrl.ClientRect.Left = (SHORT)(Border.Left + 1);
        Window->Ctrl.ClientRect.Top = (SHORT)(Border.Top + 1);
        Window->Ctrl.ClientRect.Right = (SHORT)(Border.Right - 1);
        Window->Ctrl.ClientRect.Bottom = (SHORT)(Border.Bottom - 1);
    }

    if (Title != NULL && Title->LengthInChars > 0) {
        DWORD Index;
        DWORD Offset;
        DWORD Length;
        WORD TitleBarColor;


        Length = Title->LengthInChars;
        if (Length > (DWORD)(Window->WindowSize.X - ShadowWidth - 2)) {
            Length = (DWORD)(Window->WindowSize.X - ShadowWidth - 2);
        }

        Offset = ((Window->WindowSize.X - ShadowWidth) - Length) / 2;
        TitleBarColor = BACKGROUND_BLUE | BACKGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

        for (Index = 1; Index < Offset; Index++) {
            Window->Contents[Index].Attributes = TitleBarColor;
            Window->Contents[Index].Char.UnicodeChar = ' ';
        }

        for (Index = 0; Index < Length; Index++) {
            Window->Contents[Offset + Index].Char.UnicodeChar = Title->StartOfString[Index];
            Window->Contents[Offset + Index].Attributes = TitleBarColor;
        }

        for (Index = Offset + Length; Index < (DWORD)(Window->WindowSize.X - ShadowWidth - 1); Index++) {
            Window->Contents[Index].Attributes = TitleBarColor;
            Window->Contents[Index].Char.UnicodeChar = ' ';
        }
    }

    *OutWindow = Window;
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
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE hConOut;

    WORD DisplayWidth;
    WORD DisplayHeight;
    WORD BorderWidth;
    WORD BorderHeight;
    WORD ShadowWidth;
    WORD ShadowHeight;
    COORD WindowSize;
    SMALL_RECT WindowRect;

    if (MinimumWidth > MAXSHORT ||
        MinimumHeight > MAXSHORT ||
        DesiredWidth > MAXSHORT ||
        DesiredHeight > MAXSHORT) {

        return FALSE;
    }

    if (Style & YORI_WIN_WINDOW_STYLE_SHADOW) {
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

    hConOut = YoriWinGetConsoleOutputHandle(WinMgrHandle);
    if (!GetConsoleScreenBufferInfo(hConOut, &ScreenInfo)) {
        return FALSE;
    }

    //
    //  Calculate Window location
    //

    DisplayWidth = (WORD)(ScreenInfo.srWindow.Right - ScreenInfo.srWindow.Left + 1);
    DisplayHeight = (WORD)(ScreenInfo.srWindow.Bottom - ScreenInfo.srWindow.Top + 1);

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

    WindowRect.Left = (USHORT)((DisplayWidth - WindowSize.X) / 2 + ScreenInfo.srWindow.Left);
    WindowRect.Top = (USHORT)((DisplayHeight - WindowSize.Y) / 2 + ScreenInfo.srWindow.Top);
    WindowRect.Right = (USHORT)(WindowRect.Left + WindowSize.X - 1);
    WindowRect.Bottom = (USHORT)(WindowRect.Top + WindowSize.Y - 1);

    return YoriWinCreateWindowEx(WinMgrHandle, &WindowRect, Style, Title, OutWindow);

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
 Sets a specific control to be the control that currently receives keyboard
 input.

 @param Window Pointer to the window whose control in focus should change.

 @param Ctrl Pointer to the control which should receive keyboard input.
 */
VOID
YoriWinSetFocus(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_CTRL Ctrl
    )
{
    YORI_WIN_EVENT Event;
    BOOLEAN Terminate = FALSE;
    PYORI_WIN_CTRL OldCtrl;

    ASSERT(Ctrl->CanReceiveFocus);

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

 @param Window Pointer to the window.

 @param Event Pointer to the event.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate that regular processing
         should continue (although this does not imply that no processing
         has already occurred.)
 */
BOOLEAN
YoriWinNotifyEvent(
    __in PYORI_WIN_WINDOW Window,
    __in PYORI_WIN_EVENT Event
    )
{
    BOOLEAN Terminate;
    YORI_WIN_EVENT CtrlEvent;

    if (Event->EventType == YoriWinEventKeyDown ||
        Event->EventType == YoriWinEventKeyUp) {

        if (Window->KeyboardFocusCtrl != NULL &&
            Window->KeyboardFocusCtrl->NotifyEventFn != NULL) {

            Terminate = Window->KeyboardFocusCtrl->NotifyEventFn(Window->KeyboardFocusCtrl, Event);
            if (Terminate) {
                return Terminate;
            }
        }

        if (Event->EventType == YoriWinEventKeyDown &&
            Event->KeyDown.CtrlMask == 0) {

            Terminate = FALSE;

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
            }

            if (Terminate) {
                return Terminate;
            }
        } else if (Event->EventType == YoriWinEventKeyDown &&
                   Event->KeyDown.CtrlMask == SHIFT_PRESSED) {

            if (Event->KeyDown.VirtualKeyCode == VK_TAB) {
                YoriWinSetFocusToPreviousCtrl(Window);
            }
        } else if (Event->EventType == YoriWinEventKeyDown &&
                   Event->KeyDown.CtrlMask == LEFT_ALT_PRESSED) {

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

    } else if (Event->EventType == YoriWinEventMouseMoveInClient) {

        PYORI_WIN_CTRL Ctrl;
        COORD ChildLocation;
        BOOLEAN InChildClientArea;
        Ctrl = YoriWinFindControlAtCoordinates(&Window->Ctrl,
                                               Event->MouseMove.Location,
                                               TRUE,
                                               &ChildLocation,
                                               &InChildClientArea);

        if (Ctrl != NULL &&
            YoriWinTranslateMouseEventForChild(Event, Ctrl, ChildLocation, InChildClientArea)) {

            return TRUE;
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
    }

    if (Window->CustomNotifications != NULL &&
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
 Process the input events from the system and send them to the window and
 its controls for processing.

 @param WindowHandle Pointer to the window to process input events for.

 @param Result Optionally points to a memory location which can receive
        context passed from the code which closed the window.  Note this is
        only meaningful when this function returns TRUE.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriWinProcessInputForWindow(
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle,
    __out_opt PDWORD_PTR Result
    )
{
    HANDLE hConIn;
    HANDLE hConOut;
    INPUT_RECORD InputRecords[10];
    CONSOLE_CURSOR_INFO NewCursorInfo;
    PINPUT_RECORD InputRecord;
    DWORD ActuallyRead;
    DWORD Index;
    DWORD RepeatIndex;
    YORI_WIN_EVENT Event;
    PYORI_WIN_WINDOW Window;

    Window = (PYORI_WIN_WINDOW)WindowHandle;

    hConIn = YoriWinGetConsoleInputHandle(Window->WinMgrHandle);
    hConOut = YoriWinGetConsoleOutputHandle(Window->WinMgrHandle);

    //
    //  Make the cursor invisible while the window display is active.
    //

    NewCursorInfo.dwSize = 20;
    NewCursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConOut, &NewCursorInfo);

    //
    //  If the window is supposed to have a control in focus, set it now
    //  to generate the focus event.  Clear the focus control so we don't
    //  also generate a lose focus event.
    //

    if (Window->KeyboardFocusCtrl != NULL) {
        PYORI_WIN_CTRL FocusCtrl = Window->KeyboardFocusCtrl;
        Window->KeyboardFocusCtrl = NULL;
        YoriWinSetFocus(Window, FocusCtrl);
    }

    while(TRUE) {

        //
        //  Display window contents if they have changed.
        //

        if (!YoriWinDisplayWindowContents(Window)) {
            break;
        }

        if (!ReadConsoleInput(hConIn, InputRecords, sizeof(InputRecords)/sizeof(InputRecords[0]), &ActuallyRead)) {
            break;
        }

        for (Index = 0; Index < ActuallyRead; Index++) {
            InputRecord = &InputRecords[Index];
            if (InputRecord->EventType == KEY_EVENT) {
                for (RepeatIndex = 0; RepeatIndex < InputRecord->Event.KeyEvent.wRepeatCount; RepeatIndex++) {
                    if (InputRecord->Event.KeyEvent.bKeyDown) {
                        Event.EventType = YoriWinEventKeyDown;

                        Event.KeyDown.CtrlMask = InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED | ENHANCED_KEY);
                        Event.KeyDown.VirtualKeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;
                        Event.KeyDown.VirtualScanCode = InputRecord->Event.KeyEvent.wVirtualScanCode;
                        Event.KeyDown.Char = InputRecord->Event.KeyEvent.uChar.UnicodeChar;
                    } else {
                        Event.EventType = YoriWinEventKeyUp;
                        Event.KeyUp.CtrlMask = InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED | ENHANCED_KEY);
                        Event.KeyUp.VirtualKeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;
                        Event.KeyUp.VirtualScanCode = InputRecord->Event.KeyEvent.wVirtualScanCode;
                        Event.KeyUp.Char = InputRecord->Event.KeyEvent.uChar.UnicodeChar;
                    }

                    YoriWinNotifyEvent(Window, &Event);
                }
            } else if (InputRecord->EventType == MOUSE_EVENT) {
                DWORD ButtonsPressed;
                DWORD ButtonsReleased;
                DWORD PreviousMouseButtonState;
                BOOLEAN InWindowRange;
                BOOLEAN InWindowClientRange;
                PreviousMouseButtonState = YoriWinGetPreviousMouseButtonState(Window->WinMgrHandle);

                ButtonsPressed = InputRecord->Event.MouseEvent.dwButtonState - (PreviousMouseButtonState & InputRecord->Event.MouseEvent.dwButtonState);
                ButtonsReleased = PreviousMouseButtonState - (PreviousMouseButtonState & InputRecord->Event.MouseEvent.dwButtonState);
                YoriWinSetPreviousMouseButtonState(Window->WinMgrHandle, InputRecord->Event.MouseEvent.dwButtonState);

                InWindowRange = FALSE;
                InWindowClientRange = FALSE;

                if (YoriWinCoordInSmallRect(&InputRecord->Event.MouseEvent.dwMousePosition, &Window->Ctrl.FullRect)) {
                    SMALL_RECT ClientArea;
                    ClientArea.Left = (SHORT)(Window->Ctrl.FullRect.Left + Window->Ctrl.ClientRect.Left);
                    ClientArea.Top = (SHORT)(Window->Ctrl.FullRect.Top + Window->Ctrl.ClientRect.Top);
                    ClientArea.Right = (SHORT)(Window->Ctrl.FullRect.Left + Window->Ctrl.ClientRect.Right);
                    ClientArea.Bottom = (SHORT)(Window->Ctrl.FullRect.Top + Window->Ctrl.ClientRect.Bottom);
                    InWindowRange = TRUE;
                    if (YoriWinCoordInSmallRect(&InputRecord->Event.MouseEvent.dwMousePosition, &ClientArea)) {
                        InWindowClientRange = TRUE;
                    }
                }

                if (InputRecord->Event.MouseEvent.dwEventFlags == 0) {

                    if (InWindowClientRange) {
                        if (ButtonsReleased > 0) {
                            Event.EventType = YoriWinEventMouseUpInClient;
                            Event.MouseUp.ButtonsReleased = ButtonsReleased;
                            Event.MouseUp.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseUp.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - Window->Ctrl.FullRect.Left - Window->Ctrl.ClientRect.Left);
                            Event.MouseUp.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - Window->Ctrl.FullRect.Top - Window->Ctrl.ClientRect.Top);
                            YoriWinNotifyEvent(Window, &Event);
                        }
                        if (ButtonsPressed > 0) {
                            Event.EventType = YoriWinEventMouseDownInClient;
                            Event.MouseDown.ButtonsPressed = ButtonsPressed;
                            Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseDown.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - Window->Ctrl.FullRect.Left - Window->Ctrl.ClientRect.Left);
                            Event.MouseDown.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - Window->Ctrl.FullRect.Top - Window->Ctrl.ClientRect.Top);
                            YoriWinNotifyEvent(Window, &Event);
                        }
                    } else if (InWindowRange) {
                        if (ButtonsReleased > 0) {
                            Event.EventType = YoriWinEventMouseUpInNonClient;
                            Event.MouseUp.ButtonsReleased = ButtonsReleased;
                            Event.MouseUp.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseUp.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - Window->Ctrl.FullRect.Left);
                            Event.MouseUp.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - Window->Ctrl.FullRect.Top);
                            YoriWinNotifyEvent(Window, &Event);
                        }
                        if (ButtonsPressed > 0) {
                            Event.EventType = YoriWinEventMouseDownInNonClient;
                            Event.MouseDown.ButtonsPressed = ButtonsPressed;
                            Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseDown.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - Window->Ctrl.FullRect.Left);
                            Event.MouseDown.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - Window->Ctrl.FullRect.Top);
                            YoriWinNotifyEvent(Window, &Event);
                        }
                    } else {
                        if (ButtonsReleased > 0) {
                            Event.EventType = YoriWinEventMouseUpOutsideWindow;
                            Event.MouseUp.ButtonsReleased = ButtonsReleased;
                            Event.MouseUp.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseUp.Location.X = 0;
                            Event.MouseUp.Location.Y = 0;
                            YoriWinNotifyEvent(Window, &Event);
                        }
                        if (ButtonsPressed > 0) {
                            Event.EventType = YoriWinEventMouseDownOutsideWindow;
                            Event.MouseUp.ButtonsReleased = ButtonsPressed;
                            Event.MouseUp.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseUp.Location.X = 0;
                            Event.MouseUp.Location.Y = 0;
                            YoriWinNotifyEvent(Window, &Event);
                        }
                    }
                }

                if (InputRecord->Event.MouseEvent.dwEventFlags & DOUBLE_CLICK) {
                    if (InWindowClientRange) {
                        Event.EventType = YoriWinEventMouseDoubleClickInClient;
                        Event.MouseDown.ButtonsPressed = InputRecord->Event.MouseEvent.dwButtonState;
                        Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseDown.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - Window->Ctrl.FullRect.Left - Window->Ctrl.ClientRect.Left);
                        Event.MouseDown.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - Window->Ctrl.FullRect.Top - Window->Ctrl.ClientRect.Top);
                        YoriWinNotifyEvent(Window, &Event);
                    } else if (InWindowRange) {
                        Event.EventType = YoriWinEventMouseDoubleClickInNonClient;
                        Event.MouseDown.ButtonsPressed = InputRecord->Event.MouseEvent.dwButtonState;
                        Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseDown.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - Window->Ctrl.FullRect.Left);
                        Event.MouseDown.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - Window->Ctrl.FullRect.Top);
                        YoriWinNotifyEvent(Window, &Event);
                    }
                }

                if (InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_MOVED) {
                    if (InWindowClientRange) {
                        Event.EventType = YoriWinEventMouseMoveInClient;
                        Event.MouseMove.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseMove.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - Window->Ctrl.FullRect.Left - Window->Ctrl.ClientRect.Left);
                        Event.MouseMove.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - Window->Ctrl.FullRect.Top - Window->Ctrl.ClientRect.Top);
                        YoriWinNotifyEvent(Window, &Event);
                    } else if (InWindowRange) {
                        Event.EventType = YoriWinEventMouseMoveInNonClient;
                        Event.MouseMove.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseMove.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - Window->Ctrl.FullRect.Left);
                        Event.MouseMove.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - Window->Ctrl.FullRect.Top);
                        YoriWinNotifyEvent(Window, &Event);
                    }
                }

                //
                //  Conhost v1 sends these events with the mouse location
                //  based on screen coordinates, then caps the returned values
                //  to console buffer size.  This capping means that
                //  information has been lost which prevents this code from
                //  re-translating the supplied coordinates back to screen
                //  coordinates and calculating the buffer location correctly.
                //  In addition, there appears to no way to get the "current"
                //  mouse position, which is obviously generally a bad idea.
                //  For these reasons, currently mouse wheel support is
                //  limited to Conhostv2.
                //
                if (InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_WHEELED &&
                    YoriWinIsConhostv2(Window->WinMgrHandle)) {

                    BOOLEAN MoveUp;
                    SHORT MoveAmount;
                    COORD Location;

                    Location.X = InputRecord->Event.MouseEvent.dwMousePosition.X;
                    Location.Y = InputRecord->Event.MouseEvent.dwMousePosition.Y;

                    MoveAmount = HIWORD(InputRecord->Event.MouseEvent.dwButtonState);
                    MoveUp = TRUE;
                    if (MoveAmount < 0) {
                        MoveUp = FALSE;
                        MoveAmount = (WORD)(-1 * MoveAmount);
                    }

                    MoveAmount = (WORD)(MoveAmount / 60);
                    if (MoveAmount == 0) {
                        MoveAmount = 1;
                    }

                    if (InWindowClientRange) {
                        if (MoveUp) {
                            Event.EventType = YoriWinEventMouseWheelUpInClient;
                        } else {
                            Event.EventType = YoriWinEventMouseWheelDownInClient;
                        }
                        Event.MouseWheel.LinesToMove = MoveAmount;
                        Event.MouseWheel.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseWheel.Location.X = (SHORT)(Location.X - Window->Ctrl.FullRect.Left - Window->Ctrl.ClientRect.Left);
                        Event.MouseWheel.Location.Y = (SHORT)(Location.Y - Window->Ctrl.FullRect.Top - Window->Ctrl.ClientRect.Top);
                        YoriWinNotifyEvent(Window, &Event);
                    } else if (InWindowRange) {
                        if (MoveUp) {
                            Event.EventType = YoriWinEventMouseWheelUpInNonClient;
                        } else {
                            Event.EventType = YoriWinEventMouseWheelDownInNonClient;
                        }
                        Event.MouseWheel.LinesToMove = MoveAmount;
                        Event.MouseWheel.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseWheel.Location.X = (SHORT)(Location.X - Window->Ctrl.FullRect.Left);
                        Event.MouseWheel.Location.Y = (SHORT)(Location.Y - Window->Ctrl.FullRect.Top);
                        YoriWinNotifyEvent(Window, &Event);
                    }
                }
            }
        }

        if (Window->Closing) {
            YoriWinDisplayWindowContents(Window);

            // MSFIX Should this restore the display rather than destroy?

            if (Result != NULL) {
                *Result = Window->Result;
            }
            return TRUE;
        }
    }

    return FALSE;
}

// vim:sw=4:ts=4:et:
