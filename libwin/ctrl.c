/**
 * @file libwin/ctrl.c
 *
 * Yori window control generic routines
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
#include "winpriv.h"


/**
 Initialize the generic control header that is embedded in each control.

 @param Parent Pointer to the parent window.  This is NULL when initializing
        the control header for a top level window.

 @param Rect Specifies the region of the parent window occupied by the
        control.

 @param CanReceiveFocus If TRUE, the control is capable of being selected with
        the tab key or similar.  If FALSE, focus should skip this control.

 @param Ctrl On successful completion, this control structure is updated
        to its initialized state.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriWinCreateControl(
    __in_opt PYORI_WIN_CTRL Parent,
    __in PSMALL_RECT Rect,
    __in BOOLEAN CanReceiveFocus,
    __out PYORI_WIN_CTRL Ctrl
    )
{
    if (Rect->Right < Rect->Left || Rect->Bottom < Rect->Top) {
        return FALSE;
    }

    Ctrl->Parent = Parent;

    Ctrl->FullRect.Left = Rect->Left;
    Ctrl->FullRect.Top = Rect->Top;
    Ctrl->FullRect.Right = Rect->Right;
    Ctrl->FullRect.Bottom = Rect->Bottom;

    Ctrl->ClientRect.Left = 0;
    Ctrl->ClientRect.Top = 0;
    Ctrl->ClientRect.Right = (SHORT)(Rect->Right - Rect->Left);
    Ctrl->ClientRect.Bottom = (SHORT)(Rect->Bottom - Rect->Top);

    Ctrl->RelativeToParentClient = TRUE;
    Ctrl->CanReceiveFocus = CanReceiveFocus;

    YoriLibInitializeListHead(&Ctrl->ParentControlList);
    YoriLibInitializeListHead(&Ctrl->ChildControlList);

    //
    //  Currently there's no notification mechanism to a non-window parent
    //  when a child control is added.  Not clear if this is needed.
    //

    if (Parent != NULL) {
        Ctrl->DefaultAttributes = Ctrl->Parent->DefaultAttributes;
        YoriLibAppendList(&Parent->ChildControlList, &Ctrl->ParentControlList);
        if (Parent->Parent == NULL) {
            PYORI_WIN_WINDOW Window;
            Window = YoriWinGetWindowFromWindowCtrl(Parent);
            YoriWinAddControlToWindow(Parent, Ctrl);
        }
    } else {
        Ctrl->DefaultAttributes = BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN;
    }

    return TRUE;
}

/**
 Close any initialized state within the common control header.

 @param Ctrl Pointer to the control to clean up.
 */
VOID
YoriWinDestroyControl(
    __in PYORI_WIN_CTRL Ctrl
    )
{

    //
    //  Notify all controls that the parent is going away in case they
    //  have their own cleanup to perform
    //

    if (!YoriLibIsListEmpty(&Ctrl->ChildControlList)) {
        YORI_WIN_EVENT Event;
        ZeroMemory(&Event, sizeof(Event));
        Event.EventType = YoriWinEventParentDestroyed;
        YoriWinNotifyAllControls(Ctrl, &Event);
    }

    if (Ctrl->Parent != NULL) {
        if (Ctrl->Parent->Parent == NULL) {
            PYORI_WIN_WINDOW Window;
            Window = YoriWinGetWindowFromWindowCtrl(Ctrl->Parent);
            YoriWinRemoveControlFromWindow(Window, Ctrl);
        }
        YoriLibRemoveListItem(&Ctrl->ParentControlList);
    }
}

/**
 Return the top level window that hosts this control.  This is either a
 direct parent, or some form of ancestor.

 @param Ctrl Pointer to the control.

 @return Pointer to the top level window.
 */
PYORI_WIN_WINDOW
YoriWinGetTopLevelWindow(
    __in PYORI_WIN_CTRL Ctrl
    )
{
    PYORI_WIN_CTRL Parent;
    PYORI_WIN_WINDOW Window;

    Parent = Ctrl->Parent;
    while (Parent->Parent != NULL) {
        Parent = Parent->Parent;
    }

    Window = YoriWinGetWindowFromWindowCtrl(Parent);
    return Window;
}

/**
 Notify all child controls of the control about an event that will affect
 affect each of them.

 @param Parent Pointer to the parent control.

 @param Event Pointer to the event to broadcast to all controls.
 */
VOID
YoriWinNotifyAllControls(
    __in PYORI_WIN_CTRL Parent,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_CTRL ChildCtrl;

    ListEntry = YoriLibGetNextListEntry(&Parent->ChildControlList, NULL);
    while (ListEntry != NULL) {

        //
        //  Take the current control in the list and find the next list
        //  element in case the current control attempts to kill itself
        //  during notification
        //

        ChildCtrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
        ListEntry = YoriLibGetNextListEntry(&Parent->ChildControlList, ListEntry);

        if (ChildCtrl->NotifyEventFn != NULL) {
            ChildCtrl->NotifyEventFn(ChildCtrl, Event);
        }
    }
}


/**
 Returns the size of the control's client area.

 @param Ctrl Pointer to the control.

 @param Size On successful completion, updated to contain the size of the
        control's client area.
 */
VOID
YoriWinGetControlClientSize(
    __in PYORI_WIN_CTRL Ctrl,
    __out PCOORD Size
    )
{
    Size->X = (SHORT)(Ctrl->ClientRect.Right - Ctrl->ClientRect.Left + 1);
    Size->Y = (SHORT)(Ctrl->ClientRect.Bottom - Ctrl->ClientRect.Top + 1);
}

/**
 Set the cursor visibility and shape for the control.

 @param Ctrl Pointer to the control.

 @param Visible TRUE if the cursor should be visible, FALSE if it should be
        hidden.

 @param SizePercentage The percentage of the cell to display as the cursor.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriWinSetControlCursorState(
    __in PYORI_WIN_CTRL Ctrl,
    __in BOOL Visible,
    __in DWORD SizePercentage
    )
{
    if (Ctrl->Parent != NULL) {
        return YoriWinSetControlCursorState(Ctrl->Parent, Visible, SizePercentage);
    } else {
        return YoriWinSetCursorState(Ctrl, Visible, SizePercentage);
    }
}

/**
 Check if the specified coordinates fall inside the specified bounding
 rectangle.

 @param Location The coordinates.

 @param Area The bounding rectangle.

 @return TRUE if the location falls within the bounding rectange, FALSE if it
         does not.
 */
BOOL
YoriWinCoordInSmallRect(
    __in PCOORD Location,
    __in PSMALL_RECT Area
    )
{
    if (Location->X >= Area->Left &&
        Location->X <= Area->Right &&
        Location->Y >= Area->Top &&
        Location->Y <= Area->Bottom) {

        return TRUE;
    }
    return FALSE;
}


/**
 Given coordinates relative to a parent window or control, locate a child
 control and return what the coordinates are relative to the child.

 @param Parent Pointer to the parent window or control.

 @param Location The coordinates within the parent control.

 @param ParentLocationRelativeToClient If TRUE, the Location parameter
        indicates coordinates based on the parent's client area.  If FALSE,
        the Location parameter indicates coordinates based on the parent's
        total window area.

 @param LocationInChild On successful completion, populated with the
        coordinates relative to the child.

 @param ChildLocationRelativeToClient On successful completion, set to TRUE to
        indicate the coordinates are within the child's client area, and if
        so, the returned coordinates in LocationInChild are relative to the
        client area.  Set to FALSE to indicate that Location resolved to a
        nonclient area within the control, and LocationInChild is relative to
        the total control area.

 @return Pointer to the child control if the coordinates overlap a child
         control.  NULL if no child control exists at the specified location.
 */
__success(return != NULL)
PYORI_WIN_CTRL
YoriWinFindControlAtCoordinates(
    __in PYORI_WIN_CTRL Parent,
    __in COORD Location,
    __in BOOLEAN ParentLocationRelativeToClient,
    __out PCOORD LocationInChild,
    __out PBOOLEAN ChildLocationRelativeToClient
    )
{
    COORD ClientRelativeLocation;
    COORD WindowRelativeLocation;
    BOOLEAN LocationInClient;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_CTRL Child;
    SMALL_RECT ChildClientArea;

    LocationInClient = FALSE;

    //
    //  If the input value is client relative, ensure we have both client and
    //  window relative locations.  If the input value is window relative,
    //  check if it falls in the client area, and if so, generate both
    //  locations.  If it doesn't, record that in a boolean to indicate that
    //  this location cannot match any client relative control.
    //

    if (ParentLocationRelativeToClient) {
        ClientRelativeLocation.X = Location.X;
        ClientRelativeLocation.Y = Location.Y;
        LocationInClient = TRUE;

        WindowRelativeLocation.X = (SHORT)(ClientRelativeLocation.X + Parent->ClientRect.Left);
        WindowRelativeLocation.Y = (SHORT)(ClientRelativeLocation.Y + Parent->ClientRect.Top);
    } else {
        WindowRelativeLocation.X = Location.X;
        WindowRelativeLocation.Y = Location.Y;

        if (Location.X >= Parent->ClientRect.Left && Location.X <= Parent->ClientRect.Right &&
            Location.Y >= Parent->ClientRect.Top && Location.Y <= Parent->ClientRect.Bottom) {

            ClientRelativeLocation.X = (SHORT)(Location.X - Parent->ClientRect.Left);
            ClientRelativeLocation.Y = (SHORT)(Location.Y - Parent->ClientRect.Top);
            LocationInClient = TRUE;
        }
    }

    Child = NULL;
    ListEntry = YoriLibGetNextListEntry(&Parent->ChildControlList, NULL);
    while (ListEntry != NULL) {

        //
        //  Take the current control in the list and find the next list
        //  element in case the current control attempts to kill itself
        //  during notification
        //

        Child = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
        ListEntry = YoriLibGetNextListEntry(&Parent->ChildControlList, ListEntry);

        if (Child->RelativeToParentClient) {
            if (LocationInClient) {
                if (YoriWinCoordInSmallRect(&ClientRelativeLocation, &Child->FullRect)) {
                    break;
                }
            }
        } else {
            if (YoriWinCoordInSmallRect(&WindowRelativeLocation, &Child->FullRect)) {
                break;
            }
        }
        Child = NULL;
    }

    if (Child == NULL) {
        return NULL;
    }

    ChildClientArea.Left = (SHORT)(Child->FullRect.Left + Child->ClientRect.Left);
    ChildClientArea.Top = (SHORT)(Child->FullRect.Top + Child->ClientRect.Top);
    ChildClientArea.Right = (SHORT)(Child->FullRect.Left + Child->ClientRect.Right);
    ChildClientArea.Bottom = (SHORT)(Child->FullRect.Top + Child->ClientRect.Bottom);

    if (Child->RelativeToParentClient) {
        if (YoriWinCoordInSmallRect(&ClientRelativeLocation, &ChildClientArea)) {
            LocationInChild->X = (SHORT)(ClientRelativeLocation.X - ChildClientArea.Left);
            LocationInChild->Y = (SHORT)(ClientRelativeLocation.Y - ChildClientArea.Top);
            *ChildLocationRelativeToClient = TRUE;
            return Child;
        }

        LocationInChild->X = (SHORT)(ClientRelativeLocation.X - Child->FullRect.Left);
        LocationInChild->Y = (SHORT)(ClientRelativeLocation.Y - Child->FullRect.Top);
        *ChildLocationRelativeToClient = FALSE;
        return Child;
    }

    if (YoriWinCoordInSmallRect(&WindowRelativeLocation, &ChildClientArea)) {
        LocationInChild->X = (SHORT)(WindowRelativeLocation.X - ChildClientArea.Left);
        LocationInChild->Y = (SHORT)(WindowRelativeLocation.Y - ChildClientArea.Top);
        *ChildLocationRelativeToClient = TRUE;
        return Child;
    }

    LocationInChild->X = (SHORT)(WindowRelativeLocation.X - Child->FullRect.Left);
    LocationInChild->Y = (SHORT)(WindowRelativeLocation.Y - Child->FullRect.Top);
    *ChildLocationRelativeToClient = FALSE;
    return Child;
}

/**
 Take an event that was sent to a parent control, and modify the event as
 needed to send to a child control.  This function is essentially assuming
 that @ref YoriWinFindControlAtCoordinates was already used to locate the
 child control which means the caller already has in hand what the new
 location properties are for the event.

 @param Event Pointer to the event as sent to the parent window or control.

 @param Ctrl Pointer to the child control.

 @param ChildLocation The location of the mouse event within the child
        control.

 @param InChildClientArea If TRUE, indicates that the event occurred within
        the child's client area, and that ChildLocation refers to client
        coordinates.  If FALSE the event occurred within the child's non-
        client area, and ChildLocation refers to its window coordinates.
        This is used to distinguish the type of event to generate.

 @return TRUE to indicate that a child control processed the event and wants
         to suppress further event processing.  Note that a return value of
         FALSE does not imply no processing occurred.
 */
BOOLEAN
YoriWinTranslateMouseEventForChild(
    __in PYORI_WIN_EVENT Event,
    __in PYORI_WIN_CTRL Ctrl,
    __in COORD ChildLocation,
    __in BOOLEAN InChildClientArea
    )
{
    if (Ctrl->NotifyEventFn != NULL) {

        YORI_WIN_EVENT CtrlEvent;
        BOOLEAN Terminate;

        ZeroMemory(&CtrlEvent, sizeof(CtrlEvent));
        CtrlEvent.EventType = Event->EventType;
        if (InChildClientArea) {
            if (Event->EventType == YoriWinEventMouseDownInNonClient) {
                CtrlEvent.EventType = YoriWinEventMouseDownInClient;
            } else if (Event->EventType == YoriWinEventMouseUpInNonClient) {
                CtrlEvent.EventType = YoriWinEventMouseUpInClient;
            } else if (Event->EventType == YoriWinEventMouseDoubleClickInNonClient) {
                CtrlEvent.EventType = YoriWinEventMouseDoubleClickInClient;
            } else if (Event->EventType == YoriWinEventMouseMoveInNonClient) {
                CtrlEvent.EventType = YoriWinEventMouseMoveInClient;
            } else if (Event->EventType == YoriWinEventMouseWheelUpInNonClient) {
                CtrlEvent.EventType = YoriWinEventMouseWheelUpInClient;
            } else if (Event->EventType == YoriWinEventMouseWheelDownInNonClient) {
                CtrlEvent.EventType = YoriWinEventMouseWheelDownInClient;
            }
        } else {
            if (Event->EventType == YoriWinEventMouseDownInClient) {
                CtrlEvent.EventType = YoriWinEventMouseDownInNonClient;
            } else if (Event->EventType == YoriWinEventMouseUpInClient) {
                CtrlEvent.EventType = YoriWinEventMouseUpInNonClient;
            } else if (Event->EventType == YoriWinEventMouseDoubleClickInClient) {
                CtrlEvent.EventType = YoriWinEventMouseDoubleClickInNonClient;
            } else if (Event->EventType == YoriWinEventMouseMoveInClient) {
                CtrlEvent.EventType = YoriWinEventMouseMoveInNonClient;
            } else if (Event->EventType == YoriWinEventMouseWheelUpInClient) {
                CtrlEvent.EventType = YoriWinEventMouseWheelUpInNonClient;
            } else if (Event->EventType == YoriWinEventMouseWheelDownInClient) {
                CtrlEvent.EventType = YoriWinEventMouseWheelDownInNonClient;
            }
        }

        if (Event->EventType == YoriWinEventMouseDownInClient ||
            Event->EventType == YoriWinEventMouseDownInNonClient) {
            Ctrl->MouseButtonsPressed |= (UCHAR)Event->MouseDown.ButtonsPressed;
            CtrlEvent.MouseDown.ButtonsPressed = Event->MouseDown.ButtonsPressed;
            CtrlEvent.MouseDown.ControlKeyState = Event->MouseDown.ControlKeyState;
            CtrlEvent.MouseDown.Location.X = ChildLocation.X;
            CtrlEvent.MouseDown.Location.Y = ChildLocation.Y;
        } else if (Event->EventType == YoriWinEventMouseDoubleClickInClient ||
                   Event->EventType == YoriWinEventMouseDoubleClickInNonClient) {
            CtrlEvent.MouseDown.ButtonsPressed = Event->MouseDown.ButtonsPressed;
            CtrlEvent.MouseDown.ControlKeyState = Event->MouseDown.ControlKeyState;
            CtrlEvent.MouseDown.Location.X = ChildLocation.X;
            CtrlEvent.MouseDown.Location.Y = ChildLocation.Y;
        } else if (Event->EventType == YoriWinEventMouseUpInClient ||
                   Event->EventType == YoriWinEventMouseUpInNonClient) {
            Ctrl->MouseButtonsPressed = (UCHAR)(Ctrl->MouseButtonsPressed & ~(Event->MouseUp.ButtonsReleased));
            CtrlEvent.MouseUp.ButtonsReleased = Event->MouseUp.ButtonsReleased;
            CtrlEvent.MouseUp.ControlKeyState = Event->MouseUp.ControlKeyState;
            CtrlEvent.MouseUp.Location.X = ChildLocation.X;
            CtrlEvent.MouseUp.Location.Y = ChildLocation.Y;
        } else if (Event->EventType == YoriWinEventMouseMoveInClient ||
                   Event->EventType == YoriWinEventMouseMoveInNonClient) {
            CtrlEvent.MouseMove.ControlKeyState = Event->MouseMove.ControlKeyState;
            CtrlEvent.MouseMove.Location.X = ChildLocation.X;
            CtrlEvent.MouseMove.Location.Y = ChildLocation.Y;
        } else if (Event->EventType == YoriWinEventMouseWheelUpInClient ||
                   Event->EventType == YoriWinEventMouseWheelUpInNonClient ||
                   Event->EventType == YoriWinEventMouseWheelDownInClient ||
                   Event->EventType == YoriWinEventMouseWheelDownInNonClient) {
            CtrlEvent.MouseWheel.LinesToMove = Event->MouseWheel.LinesToMove;
            CtrlEvent.MouseWheel.ControlKeyState = Event->MouseWheel.ControlKeyState;
            CtrlEvent.MouseWheel.Location.X = ChildLocation.X;
            CtrlEvent.MouseWheel.Location.Y = ChildLocation.Y;
        }

        Terminate = Ctrl->NotifyEventFn(Ctrl, &CtrlEvent);
        if (Terminate) {
            return Terminate;
        }
    }
    return FALSE;
}

/**
 Translate control coordinates to toplevel window coordinates.

 @param Ctrl Pointer to the control.

 @param CtrlCoordInClient If TRUE, CtrlCoord refers to the control client
        area.  If FALSE, it refers to the nonclient area.

 @param CtrlCoord The coordinates within the control.

 @param WinCoordInClient If TRUE, the output coordinates refer to the
        window's client area.  If FALSE, the output coordinates refer to
        the window's nonclient area.

 @param Window On successful completion, populated with the toplevel window
        that the window coordinates are relative to.
 
 @param WinCoord On successful completion, populated with the window
        coordinates.
 */
VOID
YoriWinTranslateCtrlCoordinatesToWindowCoordinates(
    __in PYORI_WIN_CTRL Ctrl,
    __in BOOLEAN CtrlCoordInClient,
    __in COORD CtrlCoord,
    __in BOOLEAN WinCoordInClient,
    __out PYORI_WIN_WINDOW* Window,
    __out PCOORD WinCoord
    )
{
    WORD NewX;
    WORD NewY;
    BOOLEAN AddClientOffset;
    PYORI_WIN_CTRL ThisCtrl;

    ThisCtrl = Ctrl;

    NewX = CtrlCoord.X;
    NewY = CtrlCoord.Y;
    AddClientOffset = CtrlCoordInClient;

    while(TRUE) {
        if (AddClientOffset) {
            NewX = (WORD)(NewX + ThisCtrl->ClientRect.Left);
            NewY = (WORD)(NewY + ThisCtrl->ClientRect.Top);
        }

        if (ThisCtrl->Parent == NULL) {
            *Window = YoriWinGetWindowFromWindowCtrl(ThisCtrl);
            break;
        }

        NewX = (WORD)(NewX + ThisCtrl->FullRect.Left);
        NewY = (WORD)(NewY + ThisCtrl->FullRect.Top);

        if (ThisCtrl->RelativeToParentClient) {
            AddClientOffset = TRUE;
        } else {
            AddClientOffset = FALSE;
        }

        ThisCtrl = ThisCtrl->Parent;
    }

    //
    //  At this point the coordinates are nonclient.  If the caller wanted
    //  client coordinates, translate them back.
    //

    if (WinCoordInClient) {
        NewX = (WORD)(NewX - ThisCtrl->ClientRect.Left);
        NewY = (WORD)(NewY - ThisCtrl->ClientRect.Top);
    }

    WinCoord->X = NewX;
    WinCoord->Y = NewY;
}

/**
 Translate control coordinates to screen buffer coordinates.

 @param Ctrl Pointer to the control.

 @param CtrlCoordInClient If TRUE, CtrlCoord refers to the control client
        area.  If FALSE, it refers to the nonclient area.

 @param CtrlCoord The coordinates within the control.

 @param ScreenCoord On successful completion, populated with the screen
        coordinates.
 */
VOID
YoriWinTranslateCtrlCoordinatesToScreenCoordinates(
    __in PYORI_WIN_CTRL Ctrl,
    __in BOOLEAN CtrlCoordInClient,
    __in COORD CtrlCoord,
    __out PCOORD ScreenCoord
    )
{
    PYORI_WIN_WINDOW ParentWindow;
    PYORI_WIN_CTRL WindowCtrl;
    COORD WinCoord;

    YoriWinTranslateCtrlCoordinatesToWindowCoordinates(Ctrl,
                                                       CtrlCoordInClient,
                                                       CtrlCoord,
                                                       FALSE,
                                                       &ParentWindow,
                                                       &WinCoord);

    WindowCtrl = YoriWinGetCtrlFromWindow(ParentWindow);

    ScreenCoord->X = (WORD)(WinCoord.X + WindowCtrl->FullRect.Left);
    ScreenCoord->Y = (WORD)(WinCoord.Y + WindowCtrl->FullRect.Top);
}


/**
 Updates the specified cell within the control's client area.

 @param Ctrl Pointer to the control to update.

 @param X Horizontal coordinate relative to the left of the control's client
        area.

 @param Y Vertical coordinate relative to the top of the control's client
        area.

 @param Char The character to place in the cell.

 @param Attr The color attributes to place in the cell.
 */
VOID
YoriWinSetControlClientCell(
    __in PYORI_WIN_CTRL Ctrl,
    __in WORD X,
    __in WORD Y,
    __in TCHAR Char,
    __in WORD Attr
    )
{
    COORD CtrlCoord;
    COORD WinCoord;
    PYORI_WIN_WINDOW Window;

    if (X > (Ctrl->ClientRect.Right - Ctrl->ClientRect.Left) ||
        Y > (Ctrl->ClientRect.Bottom - Ctrl->ClientRect.Top)) {

        return;
    }

    CtrlCoord.X = X;
    CtrlCoord.Y = Y;

    YoriWinTranslateCtrlCoordinatesToWindowCoordinates(Ctrl,
                                                       TRUE,
                                                       CtrlCoord,
                                                       FALSE,
                                                       &Window,
                                                       &WinCoord);

    YoriWinSetWindowCell(Window,
                         WinCoord.X,
                         WinCoord.Y,
                         Char,
                         Attr);
}

/**
 Updates the specified cell within the control's raw area, including its
 area outside of the client area.

 @param Ctrl Pointer to the control to update.

 @param X Horizontal coordinate relative to the left of the control's raw
        area.

 @param Y Vertical coordinate relative to the top of the control's raw
        area.

 @param Char The character to place in the cell.

 @param Attr The color attributes to place in the cell.
 */
VOID
YoriWinSetControlNonClientCell(
    __in PYORI_WIN_CTRL Ctrl,
    __in WORD X,
    __in WORD Y,
    __in TCHAR Char,
    __in WORD Attr
    )
{
    COORD CtrlCoord;
    COORD WinCoord;
    PYORI_WIN_WINDOW Window;

    if (X > (Ctrl->FullRect.Right - Ctrl->FullRect.Left) ||
        Y > (Ctrl->FullRect.Bottom - Ctrl->FullRect.Top)) {

        return;
    }

    CtrlCoord.X = X;
    CtrlCoord.Y = Y;

    YoriWinTranslateCtrlCoordinatesToWindowCoordinates(Ctrl,
                                                       FALSE,
                                                       CtrlCoord,
                                                       FALSE,
                                                       &Window,
                                                       &WinCoord);

    YoriWinSetWindowCell(Window,
                         WinCoord.X,
                         WinCoord.Y,
                         Char,
                         Attr);
}

/**
 Set the cursor location relative to the control.

 @param Ctrl Pointer to the control.

 @param X The horizontal coordinate relative to the client area of the
          control.

 @param Y The vertical coordinate relative to the client area of the
          control.
 */
VOID
YoriWinSetControlClientCursorLocation(
    __in PYORI_WIN_CTRL Ctrl,
    __in WORD X,
    __in WORD Y
    )
{
    COORD CtrlCoord;
    COORD WinCoord;
    PYORI_WIN_WINDOW Window;

    CtrlCoord.X = X;
    CtrlCoord.Y = Y;

    YoriWinTranslateCtrlCoordinatesToWindowCoordinates(Ctrl,
                                                       TRUE,
                                                       CtrlCoord,
                                                       FALSE,
                                                       &Window,
                                                       &WinCoord);

    YoriWinSetCursorPosition(Window, WinCoord.X, WinCoord.Y);
}

/**
 Return a pointer to the parent associated with a control.

 @param CtrlHandle The control to obtain the parent for.

 @return Pointer to the parent of the control, or NULL if no parent exists,
         implying the control is a top level window.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinGetControlParent(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    return Ctrl->Parent;
}

/**
 Get the control ID previously associated with the control.

 @param CtrlHandle Pointer to the control.

 @return A control ID previously associated with the control from an earlier
         call to @ref YoriWinSetControlId .
 */
DWORD_PTR
YoriWinGetControlId(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    return Ctrl->CtrlId;
}

/**
 Associate a control ID with a control.

 @param CtrlHandle Pointer to the control.

 @param CtrlId The control ID to associate with the control.
 */
VOID
YoriWinSetControlId(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD_PTR CtrlId
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Ctrl->CtrlId = CtrlId;
}

/**
 Find any child control with a matching control ID.  If no matching control
 is found, returns NULL.

 @param ParentCtrl Pointer to the parent control.  Note that this can be a
        window control.

 @param CtrlId Specifies the control identifier to find.

 @return Pointer to the child control, or NULL if no matching control is
         found.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinFindControlById(
    __in PYORI_WIN_CTRL_HANDLE ParentCtrl,
    __in DWORD_PTR CtrlId
    )
{
    PYORI_WIN_CTRL Parent = (PYORI_WIN_CTRL)ParentCtrl;
    PYORI_WIN_CTRL ChildCtrl;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&Parent->ChildControlList, NULL);
    while (ListEntry != NULL) {

        //
        //  Take the current control in the list and find the next list
        //  element in case the current control attempts to kill itself
        //  during notification
        //

        ChildCtrl = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL, ParentControlList);
        if (ChildCtrl->CtrlId == CtrlId) {
            return ChildCtrl;
        }
        ListEntry = YoriLibGetNextListEntry(&Parent->ChildControlList, ListEntry);
    }

    return NULL;
}

// vim:sw=4:ts=4:et:
