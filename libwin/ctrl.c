/**
 * @file libwin/ctrl.c
 *
 * Yori window control generic routines
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
#include "winpriv.h"


/**
 Initialize the generic control header that is embedded in each control.

 @param Parent Pointer to the parent window.  This is NULL when initializing
        the control header for a top level window.

 @param Rect Specifies the region of the parent window occupied by the
        control.

 @param CanReceiveFocus If TRUE, the control is capable of being selected with
        the tab key or similar.  If FALSE, focus should skip this control.

 @param ReceiveFocusOnMouseClick If TRUE, clicking on the control should
        switch input focus to it.  For some controls, a click can perform an
        action without needing to retain focus, but the control still needs to
        receive focus on keyboard navigation.

 @param Ctrl On successful completion, this control structure is updated
        to its initialized state.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinCreateControl(
    __in_opt PYORI_WIN_CTRL Parent,
    __in PSMALL_RECT Rect,
    __in BOOLEAN CanReceiveFocus,
    __in BOOLEAN ReceiveFocusOnMouseClick,
    __out PYORI_WIN_CTRL Ctrl
    )
{
    PYORI_WIN_WINDOW Window;

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
    if (CanReceiveFocus) {
        Ctrl->ReceiveFocusOnMouseClick = ReceiveFocusOnMouseClick;
    }

    YoriLibInitializeListHead(&Ctrl->ParentControlList);
    YoriLibInitializeListHead(&Ctrl->ChildControlList);
    YoriLibInitializeListHead(&Ctrl->PostEventList);

    //
    //  Currently there's no notification mechanism to a non-window parent
    //  when a child control is added.  Not clear if this is needed.
    //

    if (Parent != NULL) {
        Ctrl->DefaultAttributes = Ctrl->Parent->DefaultAttributes;
        YoriLibAppendList(&Parent->ChildControlList, &Ctrl->ParentControlList);
        if (Parent->Parent == NULL) {
            Window = YoriWinGetWindowFromWindowCtrl(Parent);
            YoriWinAddControlToWindow(Parent, Ctrl);
        }
    } else {
        PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;
        Window = YoriWinGetWindowFromWindowCtrl(Ctrl);
        WinMgrHandle = YoriWinGetWindowManagerHandle(Window);
        Ctrl->DefaultAttributes = YoriWinMgrDefaultColorLookup(WinMgrHandle, YoriWinColorWindowDefault);
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
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle = NULL;
    PYORI_WIN_EVENT PostedEvent;

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
            WinMgrHandle = YoriWinGetWindowManagerHandle(Window);
            YoriWinRemoveControlFromWindow(Window, Ctrl);
        }
        YoriLibRemoveListItem(&Ctrl->ParentControlList);
    }

    while ((PostedEvent = YoriWinGetNextPostedEvent(Ctrl)) != NULL) {
        YoriWinFreePostedEvent(PostedEvent);
    }

    if (WinMgrHandle != NULL) {
        YoriWinMgrRemoveTimersForControl(WinMgrHandle, Ctrl);
    }
}

/**
 Reposition a control to a new location.  This function will fix up the
 client area of the control to conform to its new size.  It will fail if
 the resulting control would have no addressable client area.

 @param Ctrl Pointer to the control to resize or reposition.

 @param NewRect Specifies the new location and size of the control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinControlReposition(
    __in PYORI_WIN_CTRL Ctrl,
    __in PSMALL_RECT NewRect
    )
{
    SMALL_RECT ClientGap;
    COORD MaxCell;

    MaxCell.X = (SHORT)(Ctrl->FullRect.Right - Ctrl->FullRect.Left);
    MaxCell.Y = (SHORT)(Ctrl->FullRect.Bottom - Ctrl->FullRect.Top);

    ClientGap.Left = Ctrl->ClientRect.Left;
    ClientGap.Top = Ctrl->ClientRect.Top;
    ClientGap.Right = (SHORT)(MaxCell.X - Ctrl->ClientRect.Right);
    ClientGap.Bottom = (SHORT)(MaxCell.Y - Ctrl->ClientRect.Bottom);

    ASSERT(ClientGap.Right >= 0);
    ASSERT(ClientGap.Bottom >= 0);

    //
    //  If the resulting client area doesn't contain at least one cell, then
    //  the new size is invalid.
    //

    if (ClientGap.Left + ClientGap.Right >= NewRect->Right - NewRect->Left + 1) {
        return FALSE;
    }

    if (ClientGap.Top + ClientGap.Bottom >= NewRect->Bottom - NewRect->Top + 1) {
        return FALSE;
    }

    Ctrl->FullRect.Left = NewRect->Left;
    Ctrl->FullRect.Top = NewRect->Top;
    Ctrl->FullRect.Right = NewRect->Right;
    Ctrl->FullRect.Bottom = NewRect->Bottom;

    MaxCell.X = (SHORT)(Ctrl->FullRect.Right - Ctrl->FullRect.Left);
    MaxCell.Y = (SHORT)(Ctrl->FullRect.Bottom - Ctrl->FullRect.Top);

    Ctrl->ClientRect.Right = (SHORT)(MaxCell.X - ClientGap.Right);
    Ctrl->ClientRect.Bottom = (SHORT)(MaxCell.Y - ClientGap.Bottom);

    ASSERT(Ctrl->ClientRect.Right <= MaxCell.X);
    ASSERT(Ctrl->ClientRect.Top <= MaxCell.Y);

    return TRUE;
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

    Parent = Ctrl;
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

 @param CtrlHandle Pointer to the control.

 @param Size On successful completion, updated to contain the size of the
        control's client area.
 */
VOID
YoriWinGetControlClientSize(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PCOORD Size
    )
{
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;

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
BOOLEAN
YoriWinSetControlCursorState(
    __in PYORI_WIN_CTRL Ctrl,
    __in BOOLEAN Visible,
    __in UCHAR SizePercentage
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
BOOLEAN
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
 Given a position which may be within or outside of a region, calculate the
 location in terms of a subregion, indicating wither the position is outside
 of the subregion and/or coordinates within the subregion.  This can be used
 to determine a screen coordinate relative to a window location,  or client
 coordinates relative to a nonclient region.

 @param Pos Pointer to a position which may contain coordinates or an
        indication of being out of bounds.  The coordinates here could refer
        to any space.

 @param SubRegion Pointer to a rectangle within the coordinate space.

 @param SubPos On completion, updated to indicate the coordinates relative to
        the SubRegion, including indicating whether the initial position is
        out of bounds of SubRegion.
 */
VOID
YoriWinBoundCoordInSubRegion(
    __in PYORI_WIN_BOUNDED_COORD Pos,
    __in PSMALL_RECT SubRegion,
    __out PYORI_WIN_BOUNDED_COORD SubPos
    )
{
    if (Pos->Left ||
        (!Pos->Right && Pos->Pos.X < SubRegion->Left)) {
        SubPos->Left = TRUE;
        SubPos->Right = FALSE;
        SubPos->Pos.X = 0;
    } else if (Pos->Right ||
               Pos->Pos.X > SubRegion->Right) {
        SubPos->Left = FALSE;
        SubPos->Right = TRUE;
        SubPos->Pos.X = 0;
    } else {
        SubPos->Left = FALSE;
        SubPos->Right = FALSE;
        SubPos->Pos.X = (SHORT)(Pos->Pos.X - SubRegion->Left);
    }

    if (Pos->Above ||
        (!Pos->Below && Pos->Pos.Y < SubRegion->Top)) {
        SubPos->Above = TRUE;
        SubPos->Below = FALSE;
        SubPos->Pos.Y = 0;
    } else if (Pos->Below ||
               Pos->Pos.Y > SubRegion->Bottom) {
        SubPos->Above = FALSE;
        SubPos->Below = TRUE;
        SubPos->Pos.Y = 0;
    } else {
        SubPos->Above = FALSE;
        SubPos->Below = FALSE;
        SubPos->Pos.Y = (SHORT)(Pos->Pos.Y - SubRegion->Top);
    }
}

/**
 Obtain the nonclient region of a control in terms of its parent's
 nonclient coordinates.

 @param Ctrl Pointer to the control.

 @param CtrlRect On completion, updated to indicate the location of the
        control relative to the nonclient area of its parent.
 */
VOID
YoriWinGetControlNonClientRegion(
    __in PYORI_WIN_CTRL Ctrl,
    __out PSMALL_RECT CtrlRect
    )
{
    CtrlRect->Left = Ctrl->FullRect.Left;
    CtrlRect->Right = Ctrl->FullRect.Right;
    CtrlRect->Top = Ctrl->FullRect.Top;
    CtrlRect->Bottom = Ctrl->FullRect.Bottom;

    if (Ctrl->RelativeToParentClient) {
        PYORI_WIN_CTRL Parent;
        Parent = Ctrl->Parent;

        CtrlRect->Left = (SHORT)(CtrlRect->Left + Parent->ClientRect.Left);
        CtrlRect->Right = (SHORT)(CtrlRect->Right + Parent->ClientRect.Left);
        CtrlRect->Top = (SHORT)(CtrlRect->Top + Parent->ClientRect.Top);
        CtrlRect->Bottom = (SHORT)(CtrlRect->Bottom + Parent->ClientRect.Top);
    }
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
 Given a control which is a toplevel window, return the screen buffer
 coordinates relative to that window.  This function can indicate that the
 returned coordinates are relative to the window's client area, if the
 coordinates are in the client area; or relative to the nonclient area,
 if the coordinates are in the nonclient area; or can return no coordinates
 and indicate that the coordinates are not within the window.

 @param WinMgrHandle Pointer to the window manager.

 @param Ctrl Pointer to a control which is a toplevel window.

 @param ScreenCoord Specifies screen buffer coordinates.

 @param InWindowRange On sucessful completion, set to TRUE to indicate that
        ScreenCoord is within the window area.

 @param InWindowClientRange On successful completion, set to TRUE to indicate
        that ScreenCoord is within the window's client area.

 @param CtrlCoord On successful completion, returns coordinates relative to
        the control's client area if InWindowClientRange is TRUE, or relative
        to the nonclient area if InWindowRange is TRUE, or zeroed if the
        coordinates are outside the window range.
 */
VOID
YoriWinTranslateScreenCoordinatesToWindow(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_CTRL Ctrl,
    __in COORD ScreenCoord,
    __out PBOOLEAN InWindowRange,
    __out PBOOLEAN InWindowClientRange,
    __out PCOORD CtrlCoord
    )
{
    SMALL_RECT WinMgrLocation;
    COORD WinMgrCoord;

    YoriWinGetWinMgrLocation(WinMgrHandle, &WinMgrLocation);

    *InWindowRange = FALSE;
    *InWindowClientRange = FALSE;

    if (!YoriWinCoordInSmallRect(&ScreenCoord, &WinMgrLocation)) {
        CtrlCoord->X = 0;
        CtrlCoord->Y = 0;
        return;
    }

    WinMgrCoord.X = (SHORT)(ScreenCoord.X - WinMgrLocation.Left);
    WinMgrCoord.Y = (SHORT)(ScreenCoord.Y - WinMgrLocation.Top);

    if (YoriWinCoordInSmallRect(&WinMgrCoord, &Ctrl->FullRect)) {
        SMALL_RECT ClientArea;
        ClientArea.Left = (SHORT)(Ctrl->FullRect.Left + Ctrl->ClientRect.Left);
        ClientArea.Top = (SHORT)(Ctrl->FullRect.Top + Ctrl->ClientRect.Top);
        ClientArea.Right = (SHORT)(Ctrl->FullRect.Left + Ctrl->ClientRect.Right);
        ClientArea.Bottom = (SHORT)(Ctrl->FullRect.Top + Ctrl->ClientRect.Bottom);
        *InWindowRange = TRUE;
        if (YoriWinCoordInSmallRect(&WinMgrCoord, &ClientArea)) {
            *InWindowClientRange = TRUE;
            CtrlCoord->X = (SHORT)(WinMgrCoord.X - ClientArea.Left);
            CtrlCoord->Y = (SHORT)(WinMgrCoord.Y - ClientArea.Top);
        } else {
            CtrlCoord->X = (SHORT)(WinMgrCoord.X - Ctrl->FullRect.Left);
            CtrlCoord->Y = (SHORT)(WinMgrCoord.Y - Ctrl->FullRect.Top);
        }
    } else {
        CtrlCoord->X = 0;
        CtrlCoord->Y = 0;
    }
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

/**
 Get an opaque pointer on a control that was previously set with
 @ref YoriWinSetControlContext .

 @param CtrlHandle Pointer to the control.
 */
PVOID
YoriWinGetControlContext(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    return Ctrl->UserContext;
}

/**
 Set an opaque pointer on a control that can be used for any user defined
 purpose.  Note this is opaque, so it will not be freed when the control is
 destroyed.

 @param CtrlHandle Pointer to the control.

 @param Context A pointer-sized blob of data to associate with the control.
 */
VOID
YoriWinSetControlContext(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PVOID Context
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Ctrl->UserContext = Context;
}

/**
 Change whether the control should receive focus in response to a mouse click.
 Note controls can receive focus via the keyboard, even if focus is not
 changed via a click.

 @param CtrlHandle Pointer to the control.

 @param ReceiveFocusOnMouseClick TRUE if the control should gain focus on a
        mouse click, FALSE if it should not.

 @return TRUE if the state was successfully changed, FALSE if it was not.
 */
BOOLEAN
YoriWinControlSetFocusOnMouseClick(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN ReceiveFocusOnMouseClick
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    if (Ctrl->CanReceiveFocus) {
        Ctrl->ReceiveFocusOnMouseClick = ReceiveFocusOnMouseClick;
        return TRUE;
    }
    return FALSE;
}

/**
 Add an event to a control's queue of events to process next time the
 control is processing events.  Note this routine will allocate space for
 the event from heap, so the caller is free to reclaim the memory pointed
 to in Event after this call.

 @param Ctrl Pointer to the control.

 @param Event Pointer to the event to process in a deferred fashion.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinPostEvent(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_EVENT EventCopy;

    EventCopy = YoriLibReferencedMalloc(sizeof(YORI_WIN_EVENT));
    if (EventCopy == NULL) {
        return FALSE;
    }

    memcpy(EventCopy, Event, sizeof(YORI_WIN_EVENT));
    YoriLibAppendList(&Ctrl->PostEventList, &EventCopy->PostEventListEntry);
    return TRUE;
}

/**
 Return the next event that has been posted to a control's queue, or NULL if
 there are no more posted events outstanding.  Any event returned from this
 function is removed from the control's queue.  The caller is expected to free
 it with a call to @ref YoriWinFreePostedEvent .

 @param Ctrl Pointer to the control.

 @return Pointer to the first event outstanding, or NULL if no more events
         are outstanding.
 */
PYORI_WIN_EVENT
YoriWinGetNextPostedEvent(
    __in PYORI_WIN_CTRL Ctrl
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_EVENT Event;

    ListEntry = YoriLibGetNextListEntry(&Ctrl->PostEventList, NULL);
    if (ListEntry == NULL) {
        return NULL;
    }

    YoriLibRemoveListItem(ListEntry);
    Event = CONTAINING_RECORD(ListEntry, YORI_WIN_EVENT, PostEventListEntry);
    return Event;
}

/**
 Free an event that was posted to a control's queue.  These require heap
 allocations.

 @param Event Pointer to the event to deallocate.
 */
VOID
YoriWinFreePostedEvent(
    __in PYORI_WIN_EVENT Event
    )
{
    YoriLibDereference(Event);
}

// vim:sw=4:ts=4:et:
