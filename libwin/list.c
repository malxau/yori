/**
 * @file libwin/list.c
 *
 * Yori display a list box control
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
 A structure describing the contents of a list control.
 */
typedef struct _YORI_WIN_CTRL_LIST {

    /**
     A common header for all controls
     */
    YORI_WIN_CTRL Ctrl;

    /**
     Pointer to the vertical scroll bar associated with the list.
     */
    PYORI_WIN_CTRL VScrollCtrl;

    /**
     Callback function to notify after a selection has changed.
     */
    PYORI_WIN_NOTIFY SelectionChangeCallback;

    /**
     The set of options to display in the list
     */
    YORI_WIN_ITEM_ARRAY ItemArray;

    /**
     The index within ItemArray of the first array element to display in the
     list
     */
    DWORD FirstDisplayedOption;

    /**
     The index within ItemArray of the array element that is currently
     highlighted
     */
    DWORD ActiveOption;

    /**
     The number of character cells for each item when the list is displayed
     horizontally.
     */
    WORD HorizontalItemWidth;

    /**
     Set to TRUE if any option is activated.  FALSE if no item has been
     activated.
     */
    BOOLEAN ItemActive;

    /**
     Set to TRUE if the list control supports multiple selection.
     */
    BOOLEAN MultiSelect;

    /**
     If TRUE, the control has focus, indicating the cursor should be
     displayed.
     */
    BOOLEAN HasFocus;

    /**
     If TRUE, the active selection should be cleared when losing focus.
     */
    BOOLEAN DeselectOnLoseFocus;

    /**
     If TRUE, items should be displayed horizontally, with multiple items per
     line.  If FALSE, items are displayed vertically.
     */
    BOOLEAN HorizontalDisplay;

    /**
     If TRUE, the control should display a border.
     */
    BOOLEAN DisplayBorder;

} YORI_WIN_CTRL_LIST, *PYORI_WIN_CTRL_LIST;

/**
 Move the first displayed option in the list to ensure that the currently
 selected item is within the display.

 @param List Pointer to the list control.
 */
VOID
YoriWinListEnsureActiveItemVisible(
    __inout PYORI_WIN_CTRL_LIST List
    )
{
    WORD ElementCountToDisplay;
    COORD ClientSize;

    if (!List->ItemActive) {
        return;
    }

    YoriWinGetControlClientSize(&List->Ctrl, &ClientSize);
    if (List->HorizontalDisplay) {
        ElementCountToDisplay = (WORD)(ClientSize.X / List->HorizontalItemWidth);
    } else {
        ElementCountToDisplay = ClientSize.Y;
    }

    if (List->ItemArray.Count < ElementCountToDisplay) {
        ElementCountToDisplay = (WORD)List->ItemArray.Count;
    }

    if (List->ActiveOption < List->FirstDisplayedOption) {
        List->FirstDisplayedOption = List->ActiveOption;
    }

    if (List->ActiveOption >= List->FirstDisplayedOption + ElementCountToDisplay) {
        List->FirstDisplayedOption = List->ActiveOption - ElementCountToDisplay + 1;
    }
}

/**
 Render the current set of visible options into the window buffer when the
 list is configured to display each option on a seperate line.  This function
 will scroll options as necessary and display the currently selected option
 as inverted.

 @param List Pointer to the control to update the window buffer on.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinListPaintVerticalList(
    __inout PYORI_WIN_CTRL_LIST List
    )
{
    WORD RowIndex;
    WORD CellIndex;
    WORD CharsToDisplay;
    WORD ElementCountToDisplay;
    WORD Attributes;
    WORD WindowAttributes;
    PYORI_WIN_ITEM_ENTRY Element;
    COORD ClientSize;

    WindowAttributes = List->Ctrl.DefaultAttributes;
    YoriWinGetControlClientSize(&List->Ctrl, &ClientSize);
    ElementCountToDisplay = ClientSize.Y;

    if (List->ItemArray.Count < ElementCountToDisplay) {
        ElementCountToDisplay = (WORD)List->ItemArray.Count;
    }

    for (RowIndex = 0; RowIndex < ElementCountToDisplay; RowIndex++) {
        Element = &List->ItemArray.Items[List->FirstDisplayedOption + RowIndex];
        Attributes = WindowAttributes;
        if (List->ItemActive &&
            RowIndex + List->FirstDisplayedOption == List->ActiveOption) {

            Attributes = (WORD)(((Attributes & 0xf0) >> 4) | ((Attributes & 0x0f) << 4));
        }
        if (List->MultiSelect) {
            CharsToDisplay = (WORD)(ClientSize.X - 2);
            if (CharsToDisplay > Element->String.LengthInChars) {
                CharsToDisplay = (WORD)Element->String.LengthInChars;
            }
            if (Element->Flags & YORI_WIN_ITEM_SELECTED) {
                YoriWinSetControlClientCell(&List->Ctrl, 0, RowIndex, '*', Attributes);
            } else {
                YoriWinSetControlClientCell(&List->Ctrl, 0, RowIndex, ' ', Attributes);
            }
            YoriWinSetControlClientCell(&List->Ctrl, 1, RowIndex, ' ', Attributes);
            for (CellIndex = 0; CellIndex < CharsToDisplay; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellIndex + 2), RowIndex, Element->String.StartOfString[CellIndex], Attributes);
            }
            for (;CellIndex < ClientSize.X - 2; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellIndex + 2), RowIndex, ' ', Attributes);
            }

        } else {
            CharsToDisplay = ClientSize.X;
            if (CharsToDisplay > Element->String.LengthInChars) {
                CharsToDisplay = (WORD)Element->String.LengthInChars;
            }
            for (CellIndex = 0; CellIndex < CharsToDisplay; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, CellIndex, RowIndex, Element->String.StartOfString[CellIndex], Attributes);
            }
            for (;CellIndex < ClientSize.X; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, CellIndex, RowIndex, ' ', Attributes);
            }
        }
    }

    //
    //  Clear any rows following rows with contents
    //

    for (;RowIndex < ClientSize.Y; RowIndex++) {
        for (CellIndex = 0; CellIndex < ClientSize.X; CellIndex++) {
            YoriWinSetControlClientCell(&List->Ctrl, CellIndex, RowIndex, ' ', WindowAttributes);
        }
    }

    if (List->VScrollCtrl) {
        DWORD MaximumTopValue;
        if (List->ItemArray.Count > (DWORD)ClientSize.Y) {
            MaximumTopValue = List->ItemArray.Count - ClientSize.Y;
        } else {
            MaximumTopValue = 0;
        }

        YoriWinScrollBarSetPosition(List->VScrollCtrl, List->FirstDisplayedOption, ElementCountToDisplay, MaximumTopValue);
    }

    if (List->HasFocus) {
        DWORD SelectedRowOffset;
        SelectedRowOffset = 0;
        if (List->ItemActive) {
            SelectedRowOffset = List->ActiveOption - List->FirstDisplayedOption;
            if (SelectedRowOffset >= (DWORD)ClientSize.Y) {
                SelectedRowOffset = 0;
            }
        }
        YoriWinSetControlClientCursorLocation(&List->Ctrl, 0, (WORD)SelectedRowOffset);
    }

    return TRUE;
}

/**
 Render the current set of visible options into the window buffer when the
 list is configured to display all options on a single line.  This function
 will scroll options as necessary and display the currently selected option
 as inverted.

 @param List Pointer to the control to update the window buffer on.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinListPaintHorizontalList(
    __inout PYORI_WIN_CTRL_LIST List
    )
{
    WORD RowIndex;
    WORD CellIndex;
    WORD CellOffset;

    WORD CharsToDisplay;
    WORD ElementCountToDisplay;
    WORD Attributes;
    WORD WindowAttributes;
    PYORI_WIN_ITEM_ENTRY Element;
    COORD ClientSize;

    WindowAttributes = List->Ctrl.DefaultAttributes;
    YoriWinGetControlClientSize(&List->Ctrl, &ClientSize);
    ElementCountToDisplay = (WORD)(ClientSize.X / List->HorizontalItemWidth);

    if (List->ItemArray.Count < ElementCountToDisplay) {
        ElementCountToDisplay = (WORD)List->ItemArray.Count;
    }

    for (RowIndex = 0; RowIndex < ElementCountToDisplay; RowIndex++) {
        Element = &List->ItemArray.Items[List->FirstDisplayedOption + RowIndex];
        CellOffset = (WORD)(List->HorizontalItemWidth * RowIndex);
        Attributes = WindowAttributes;
        if (List->ItemActive &&
            RowIndex + List->FirstDisplayedOption == List->ActiveOption) {

            Attributes = (WORD)(((Attributes & 0xf0) >> 4) | ((Attributes & 0x0f) << 4));
        }
        if (List->MultiSelect) {
            CharsToDisplay = (WORD)(List->HorizontalItemWidth - 4);
            if (CharsToDisplay > Element->String.LengthInChars) {
                CharsToDisplay = (WORD)Element->String.LengthInChars;
            }
            YoriWinSetControlClientCell(&List->Ctrl, CellOffset, 0, ' ', Attributes);
            if (Element->Flags & YORI_WIN_ITEM_SELECTED) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + 1), 0, '*', Attributes);
            } else {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + 1), 0, ' ', Attributes);
            }
            YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + 2), 0, ' ', Attributes);
            for (CellIndex = 0; CellIndex < CharsToDisplay; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + CellIndex + 3), 0, Element->String.StartOfString[CellIndex], Attributes);
            }
            for (;CellIndex < List->HorizontalItemWidth - 3; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + CellIndex + 3), 0, ' ', Attributes);
            }

        } else {
            CharsToDisplay = (WORD)(List->HorizontalItemWidth - 2);
            if (CharsToDisplay > Element->String.LengthInChars) {
                CharsToDisplay = (WORD)Element->String.LengthInChars;
            }
            YoriWinSetControlClientCell(&List->Ctrl, CellOffset, 0, ' ', Attributes);
            for (CellIndex = 0; CellIndex < CharsToDisplay; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + CellIndex + 1), 0, Element->String.StartOfString[CellIndex], Attributes);
            }
            for (;CellIndex < List->HorizontalItemWidth - 1; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + CellIndex + 1), 0, ' ', Attributes);
            }
        }
    }

    //
    //  Clear any rows following rows with contents
    //

    CellOffset = (WORD)(List->HorizontalItemWidth * RowIndex);
    for (;CellOffset < ClientSize.X; CellOffset++) {
        YoriWinSetControlClientCell(&List->Ctrl, CellOffset, 0, ' ', WindowAttributes);
    }

    if (List->HasFocus) {
        DWORD SelectedRowOffset;
        SelectedRowOffset = 0;
        if (List->ItemActive) {
            SelectedRowOffset = List->ActiveOption - List->FirstDisplayedOption;
            if (SelectedRowOffset >= (DWORD)(List->HorizontalItemWidth * RowIndex)) {
                SelectedRowOffset = 0;
            }
        }
        YoriWinSetControlClientCursorLocation(&List->Ctrl, (WORD)(SelectedRowOffset * List->HorizontalItemWidth), 0);
    }

    return TRUE;
}



/**
 Render the current set of visible options into the window buffer.  This
 function will scroll options as necessary and display the currently selected
 option as inverted.

 @param List Pointer to the control to update the window buffer on.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinListPaint(
    __inout PYORI_WIN_CTRL_LIST List
    )
{
    if (List->HorizontalDisplay) {
        return YoriWinListPaintHorizontalList(List);
    } else {
        return YoriWinListPaintVerticalList(List);
    }
}

/**
 Clear all items in the list control and reset selection to nothing.

 @param CtrlHandle Pointer to the list control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinListClearAllItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_LIST List;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    YoriWinItemArrayCleanup(&List->ItemArray);
    List->FirstDisplayedOption = 0;
    List->ActiveOption = 0;
    if (List->ItemActive) {
        List->ItemActive = FALSE;
        if (List->SelectionChangeCallback) {
            List->SelectionChangeCallback(&List->Ctrl);
        }
    }
    YoriWinListPaint(List);
    return TRUE;
}

/**
 Invoked when the user manipulates the scroll bar to indicate that the
 position within the list should be updated.

 MSFIX This logic is quite confused because the scroll bar is trying to
 describe the visible region and this function is trying to manipulate the
 selected item because it can't manipulate the visible region.  This leads to
 buggy behavior.  Once the list can describe a visible region outside of the
 selection, this function will need to be redone, hopefully be simpler, and
 more correct.

 @param ScrollCtrlHandle Pointer to the scroll bar control.
 */
VOID
YoriWinListNotifyScrollChange(
    __in PYORI_WIN_CTRL_HANDLE ScrollCtrlHandle
    )
{
    PYORI_WIN_CTRL_LIST List;
    DWORDLONG ScrollValue;
    COORD ClientSize;
    WORD ElementCountToDisplay;
    PYORI_WIN_CTRL ScrollCtrl;

    ScrollCtrl = (PYORI_WIN_CTRL)ScrollCtrlHandle;
    List = CONTAINING_RECORD(ScrollCtrl->Parent, YORI_WIN_CTRL_LIST, Ctrl);
    ASSERT(List->VScrollCtrl == ScrollCtrl);

    YoriWinGetControlClientSize(&List->Ctrl, &ClientSize);
    ElementCountToDisplay = ClientSize.Y;

    ScrollValue = YoriWinScrollBarGetPosition(ScrollCtrl);
    ASSERT(ScrollValue <= List->ItemArray.Count);
    if (ScrollValue + ElementCountToDisplay > List->ItemArray.Count) {
        if (List->ItemArray.Count >= ElementCountToDisplay) {
            List->FirstDisplayedOption = List->ItemArray.Count - ElementCountToDisplay;
        } else {
            List->FirstDisplayedOption = 0;
        }
    } else {

        if (ScrollValue < List->ItemArray.Count) {
            List->FirstDisplayedOption = (DWORD)ScrollValue;
        }
    }

    YoriWinListPaint(List);
}

/**
 Scroll the list based on a mouse wheel notification.

 @param List Pointer to the list to scroll.

 @param LinesToMove The number of lines to scroll.

 @param MoveUp If TRUE, scroll backwards through the list.  If FALSE,
        scroll forwards through the list.
 */
VOID
YoriWinListNotifyMouseWheel(
    __in PYORI_WIN_CTRL_LIST List,
    __in DWORD LinesToMove,
    __in BOOLEAN MoveUp
    )
{
    COORD ClientSize;
    WORD ElementCountToDisplay;

    YoriWinGetControlClientSize(&List->Ctrl, &ClientSize);
    ElementCountToDisplay = ClientSize.Y;

    if (MoveUp) {
        if (List->FirstDisplayedOption < LinesToMove) {
            List->FirstDisplayedOption = 0;
        } else {
            List->FirstDisplayedOption = List->FirstDisplayedOption - LinesToMove;
        }
    } else {
        if (List->FirstDisplayedOption + LinesToMove + ElementCountToDisplay > List->ItemArray.Count) {
            if (List->ItemArray.Count >= ElementCountToDisplay) {
                List->FirstDisplayedOption = List->ItemArray.Count - ElementCountToDisplay;
            } else {
                List->FirstDisplayedOption = 0;
            }
        } else {
            List->FirstDisplayedOption = List->FirstDisplayedOption + LinesToMove;
        }
    }
    YoriWinListPaint(List);
}

/**
 Given a mouse click at a control relative location, find the item in the list
 that would be at that location.  It's possible that no item is at the
 location, in which case this function returns FALSE.

 @param List Pointer to the list control.

 @param MousePos Specifies the location of the mouse relative to the control's
        client area.

 @param SelectedItem On successful completion, updated to contain the item
        that the mouse location refers to.

 @return TRUE to indicate there is a list item corresponding to this mouse
         location, FALSE if there is not.
 */
__success(return)
BOOLEAN
YoriWinListGetItemSelectedByMouseLocation(
    __in PYORI_WIN_CTRL_LIST List,
    __in COORD MousePos,
    __out PDWORD SelectedItem
    )
{
    DWORD ItemRelativeToFirstDisplayed;

    if (List->HorizontalDisplay) {
        ItemRelativeToFirstDisplayed = MousePos.X / List->HorizontalItemWidth;
    } else {
        ItemRelativeToFirstDisplayed = MousePos.Y;
    }

    if (ItemRelativeToFirstDisplayed + List->FirstDisplayedOption < List->ItemArray.Count) {
        *SelectedItem = ItemRelativeToFirstDisplayed + List->FirstDisplayedOption;
        return TRUE;
    }

    return FALSE;
}


/**
 Process input events for a list control.

 @param Ctrl Pointer to the list control.

 @param Event Pointer to the input event.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate that regular processing
         should continue (although this does not imply that no processing
         has already occurred.)
 */
BOOLEAN
YoriWinListEventHandler(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_CTRL_LIST List;
    DWORD NewOption;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    switch(Event->EventType) {
        case YoriWinEventKeyDown:
            if (Event->KeyDown.CtrlMask == ENHANCED_KEY ||
                Event->KeyDown.CtrlMask == 0) {

                if (Event->KeyDown.VirtualKeyCode == VK_UP ||
                    (List->HorizontalDisplay && Event->KeyDown.VirtualKeyCode == VK_LEFT)) {
                    if (List->ItemActive) {
                        if (List->ActiveOption > 0) {
                            List->ActiveOption--;
                            YoriWinListEnsureActiveItemVisible(List);
                            if (List->SelectionChangeCallback) {
                                List->SelectionChangeCallback(&List->Ctrl);
                            }
                            YoriWinListPaint(List);
                        }
                    } else if (List->ItemArray.Count > 0) {
                        List->ItemActive = TRUE;
                        List->ActiveOption = 0;
                        YoriWinListEnsureActiveItemVisible(List);
                        if (List->SelectionChangeCallback) {
                            List->SelectionChangeCallback(&List->Ctrl);
                        }
                        YoriWinListPaint(List);
                    }
                } else if (Event->KeyDown.VirtualKeyCode == VK_DOWN ||
                    (List->HorizontalDisplay && Event->KeyDown.VirtualKeyCode == VK_RIGHT)) {
                    if (List->ItemActive) {
                        if (List->ActiveOption + 1 < List->ItemArray.Count) {
                            List->ActiveOption++;
                            YoriWinListEnsureActiveItemVisible(List);
                            if (List->SelectionChangeCallback) {
                                List->SelectionChangeCallback(&List->Ctrl);
                            }
                            YoriWinListPaint(List);
                        }
                    } else if (List->ItemArray.Count > 0) {
                        List->ItemActive = TRUE;
                        List->ActiveOption = 0;
                        YoriWinListEnsureActiveItemVisible(List);
                        if (List->SelectionChangeCallback) {
                            List->SelectionChangeCallback(&List->Ctrl);
                        }
                        YoriWinListPaint(List);
                    }
                } else if (Event->KeyDown.VirtualKeyCode == VK_PRIOR) {
                    COORD ClientSize;
                    DWORD ElementCountToDisplay;
                    if (List->ItemActive) {
                        YoriWinGetControlClientSize(&List->Ctrl, &ClientSize);
                        ElementCountToDisplay = ClientSize.Y;
                        if (List->ActiveOption > List->FirstDisplayedOption) {
                            List->ActiveOption = List->FirstDisplayedOption;
                        } else if (ElementCountToDisplay < List->ActiveOption) {
                            List->ActiveOption = List->ActiveOption - ElementCountToDisplay;
                        } else {
                            List->ActiveOption = 0;
                        }
                    } else if (List->ItemArray.Count > 0) {
                        List->ItemActive = TRUE;
                        List->ActiveOption = 0;
                    }
                    YoriWinListEnsureActiveItemVisible(List);
                    if (List->SelectionChangeCallback) {
                        List->SelectionChangeCallback(&List->Ctrl);
                    }
                    YoriWinListPaint(List);
                } else if (Event->KeyDown.VirtualKeyCode == VK_NEXT) {
                    COORD ClientSize;
                    DWORD ElementCountToDisplay;
                    if (List->ItemActive) {
                        YoriWinGetControlClientSize(&List->Ctrl, &ClientSize);
                        ElementCountToDisplay = ClientSize.Y;
                        if (List->ActiveOption < List->FirstDisplayedOption + ElementCountToDisplay - 1 &&
                            List->FirstDisplayedOption + ElementCountToDisplay - 1 < List->ItemArray.Count) {
                            List->ActiveOption = List->FirstDisplayedOption + ElementCountToDisplay - 1;
                        } else if (List->ActiveOption + ElementCountToDisplay < List->ItemArray.Count) {
                            List->ActiveOption = List->ActiveOption + ElementCountToDisplay;
                        } else {
                            List->ActiveOption = List->ItemArray.Count - 1;
                        }
                    } else if (List->ItemArray.Count > 0) {
                        List->ItemActive = TRUE;
                        List->ActiveOption = 0;
                    }
                    YoriWinListEnsureActiveItemVisible(List);
                    if (List->SelectionChangeCallback) {
                        List->SelectionChangeCallback(&List->Ctrl);
                    }
                    YoriWinListPaint(List);
                } else if (Event->KeyDown.Char == ' ' &&
                           List->ItemActive &&
                           List->MultiSelect) {
                    PYORI_WIN_ITEM_ENTRY Element;

                    ASSERT(List->ActiveOption < List->ItemArray.Count);
                    Element = &List->ItemArray.Items[List->ActiveOption];
                    Element->Flags = Element->Flags ^ YORI_WIN_ITEM_SELECTED;
                    if (List->SelectionChangeCallback) {
                        List->SelectionChangeCallback(&List->Ctrl);
                    }
                    YoriWinListPaint(List);
                }
            }
            break;
        case YoriWinEventMouseDownInClient:

            if (YoriWinListGetItemSelectedByMouseLocation(List, Event->MouseDown.Location, &NewOption)) {
                PYORI_WIN_ITEM_ENTRY Element;

                List->ItemActive = TRUE;
                if (List->ActiveOption == NewOption && List->MultiSelect) {

                    Element = &List->ItemArray.Items[List->ActiveOption];
                    Element->Flags = Element->Flags ^ YORI_WIN_ITEM_SELECTED;
                } 
                List->ActiveOption = NewOption;
                if (List->SelectionChangeCallback) {
                    List->SelectionChangeCallback(&List->Ctrl);
                }
                YoriWinListPaint(List);
            }

            break;
        case YoriWinEventMouseDoubleClickInClient:
            if (YoriWinListGetItemSelectedByMouseLocation(List, Event->MouseDown.Location, &NewOption)) {
                YORI_WIN_EVENT DefaultEvent;
                PYORI_WIN_ITEM_ENTRY Element;

                List->ItemActive = TRUE;
                List->ActiveOption = NewOption;
                if (List->MultiSelect) {
                    Element = &List->ItemArray.Items[List->ActiveOption];
                    Element->Flags = Element->Flags ^ YORI_WIN_ITEM_SELECTED;
                }

                if (List->SelectionChangeCallback) {
                    List->SelectionChangeCallback(&List->Ctrl);
                }

                YoriWinListPaint(List);
                if (!List->MultiSelect && List->Ctrl.Parent->NotifyEventFn != NULL) {
                    ZeroMemory(&DefaultEvent, sizeof(DefaultEvent));
                    DefaultEvent.EventType = YoriWinEventExecute;
                    List->Ctrl.Parent->NotifyEventFn(List->Ctrl.Parent, &DefaultEvent);
                }
            }
            break;
        case YoriWinEventMouseDownInNonClient:
        case YoriWinEventMouseUpInNonClient:
        case YoriWinEventMouseDoubleClickInNonClient:
            {
                PYORI_WIN_CTRL Child;
                COORD ChildLocation;
                BOOLEAN InChildClientArea;
                Child = YoriWinFindControlAtCoordinates(Ctrl,
                                                        Event->MouseDown.Location,
                                                        FALSE,
                                                        &ChildLocation,
                                                        &InChildClientArea);

                if (Child != NULL &&
                    YoriWinTranslateMouseEventForChild(Event, Child, ChildLocation, InChildClientArea)) {

                    return TRUE;
                }
            }
            break;

        case YoriWinEventMouseWheelDownInClient:
        case YoriWinEventMouseWheelDownInNonClient:
            YoriWinListNotifyMouseWheel(List, Event->MouseWheel.LinesToMove, FALSE);
            break;

        case YoriWinEventMouseWheelUpInClient:
        case YoriWinEventMouseWheelUpInNonClient:
            YoriWinListNotifyMouseWheel(List, Event->MouseWheel.LinesToMove, TRUE);
            break;

        case YoriWinEventGetFocus:
            List->HasFocus = TRUE;
            YoriWinSetControlCursorState(&List->Ctrl, TRUE, 20);
            YoriWinListPaint(List);
            break;

        case YoriWinEventLoseFocus:
            List->HasFocus = FALSE;
            YoriWinSetControlCursorState(&List->Ctrl, FALSE, 20);
            if (List->DeselectOnLoseFocus) {
                if (List->ItemActive) {
                    List->ItemActive = FALSE;
                    if (List->SelectionChangeCallback) {
                        List->SelectionChangeCallback(&List->Ctrl);
                    }
                }
            }
            YoriWinListPaint(List);
            break;

        case YoriWinEventParentDestroyed:
            YoriWinItemArrayCleanup(&List->ItemArray);
            YoriWinDestroyControl(Ctrl);
            YoriLibDereference(List);
            break;
    }

    return FALSE;
}

/**
 Returns the number of items populated into the list control.

 @param CtrlHandle Pointer to the list control.
 */
DWORD
YoriWinListGetItemCount(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_LIST List;
    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);
    return List->ItemArray.Count;
}

/**
 Returns the currently active option within the list control.

 @param CtrlHandle Pointer to the list control.

 @param CurrentlyActiveIndex On successful completion, populated with the
        currently selected list item.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinListGetActiveOption(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PDWORD CurrentlyActiveIndex
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_LIST List;
    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);
    if (!List->ItemActive) {
        return FALSE;
    }
    *CurrentlyActiveIndex = List->ActiveOption;
    return TRUE;
}

/**
 Set the currently selected option within the list control.

 @param CtrlHandle Pointer to the list control.

 @param ActiveOption Specifies the index of the item to select.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinListSetActiveOption(
    __inout PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD ActiveOption
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_LIST List;
    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    if (ActiveOption < List->ItemArray.Count) {
        List->ItemActive = TRUE;
        List->ActiveOption = ActiveOption;
        YoriWinListEnsureActiveItemVisible(List);
        if (List->SelectionChangeCallback) {
            List->SelectionChangeCallback(&List->Ctrl);
        }
        YoriWinListPaint(List);
        return TRUE;
    }

    return FALSE;
}

/**
 Indicates if the specified list index item is selected.  This is used on
 multiselect lists where there can be many selected items.  On a single
 select list, this will return TRUE for the active item.

 @param CtrlHandle Pointer to the list control.

 @param Index Specifies the index of the item to test for selection.

 @return TRUE to indicate the item is selected, FALSE to indicate it is not.
 */
BOOLEAN
YoriWinListIsOptionSelected(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD Index
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_LIST List;
    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    if (Index < List->ItemArray.Count) {
        if (List->MultiSelect) {
            if (List->ItemArray.Items[Index].Flags & YORI_WIN_ITEM_SELECTED) {
                return TRUE;
            }
        } else {
            if (List->ItemActive && List->ActiveOption == Index) {
                return TRUE;
            }
        }
    }
    return FALSE;
}


/**
 Adds new items to the list control.

 @param CtrlHandle Pointer to the list control to add items to.

 @param ListOptions Pointer to an array of options to include in the list
        control.

 @param NumberOptions Specifies the number of options in the above array.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinListAddItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING ListOptions,
    __in DWORD NumberOptions
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_LIST List;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    if (!YoriWinItemArrayAddItems(&List->ItemArray, ListOptions, NumberOptions)) {
        return FALSE;
    }

    YoriWinListEnsureActiveItemVisible(List);
    YoriWinListPaint(List);
    return TRUE;
}

/**
 Adds new items to the list control.

 @param CtrlHandle Pointer to the list control to add items to.

 @param NewItems Pointer to an array of items to add to the list control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinListAddItemArray(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_ITEM_ARRAY NewItems
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_LIST List;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    if (!YoriWinItemArrayAddItemArray(&List->ItemArray, NewItems)) {
        return FALSE;
    }

    YoriWinListEnsureActiveItemVisible(List);
    YoriWinListPaint(List);
    return TRUE;
}

/**
 Return the text within a specified element of a list control.

 @param CtrlHandle Pointer to the list control.

 @param Index Specifies the element to retrieve from the list control.

 @param Text On input, an initialized Yori string.  On output, populated with
        the string contents of the item within a list control.  Note this
        string can be reallocated within this routine.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinListGetItemText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD Index,
    __inout PYORI_STRING Text
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_LIST List;
    PYORI_STRING Source;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    if (Index >= List->ItemArray.Count) {
        return FALSE;
    }

    Source = &List->ItemArray.Items[Index].String;

    if (Text->LengthAllocated < Source->LengthInChars + 1) {
        YORI_STRING NewString;
        if (!YoriLibAllocateString(&NewString, Source->LengthInChars + 1)) {
            return FALSE;
        }

        YoriLibFreeStringContents(Text);
        memcpy(Text, &NewString, sizeof(YORI_STRING));
    }

    memcpy(Text->StartOfString, Source->StartOfString, Source->LengthInChars * sizeof(TCHAR));
    Text->LengthInChars = Source->LengthInChars;
    Text->StartOfString[Source->LengthInChars] = '\0';
    return TRUE;
}

/**
 Set the size and location of a list control, and redraw the contents.

 @param CtrlHandle Pointer to the list to resize or reposition.

 @param CtrlRect Specifies the new size and position of the list.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinListReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    PYORI_WIN_CTRL_LIST List;
    WORD WindowAttributes;
    SMALL_RECT BorderRect;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    if (!YoriWinControlReposition(Ctrl, CtrlRect)) {
        return FALSE;
    }

    WindowAttributes = List->Ctrl.DefaultAttributes;

    if (List->DisplayBorder) {
        BorderRect.Left = 0;
        BorderRect.Top = 0;
        BorderRect.Right = (SHORT)(List->Ctrl.FullRect.Right - List->Ctrl.FullRect.Left);
        BorderRect.Bottom = (SHORT)(List->Ctrl.FullRect.Bottom - List->Ctrl.FullRect.Top - 1);

        YoriWinDrawBorderOnControl(&List->Ctrl, &BorderRect, WindowAttributes, YORI_WIN_BORDER_TYPE_SUNKEN);
    }

    if (List->VScrollCtrl != NULL) {
        SMALL_RECT ScrollBarRect;

        ScrollBarRect.Left = (SHORT)(List->Ctrl.FullRect.Right - List->Ctrl.FullRect.Left);
        ScrollBarRect.Right = ScrollBarRect.Left;
        ScrollBarRect.Top = 1;
        ScrollBarRect.Bottom = (SHORT)(List->Ctrl.FullRect.Bottom - List->Ctrl.FullRect.Top - 1);

        YoriWinScrollBarReposition(List->VScrollCtrl, &ScrollBarRect);
    }

    YoriWinListPaint(List);

    return TRUE;
}

/**
 Register a callback to receive notifications when the selected row in the
 list changes.

 @param CtrlHandle Pointer to the list control.

 @param NotifyCallback Pointer to the callback function to invoke on selection
        change.

 @return TRUE if the callback function is successfully registered, FALSE if
         another callback is already registered.
 */
BOOLEAN
YoriWinListSetSelectionNotifyCallback(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_NOTIFY NotifyCallback
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_LIST List;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    if (List->SelectionChangeCallback != NULL) {
        return FALSE;
    }

    List->SelectionChangeCallback = NotifyCallback;
    return TRUE;
}

/**
 Set the number of characters to use for each element when the list is
 displaying items horizontally.

 @param CtrlHandle Pointer to the list control.

 @param ItemWidth Specifies the width in characters of each item.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinListSetHorizontalItemWidth(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in WORD ItemWidth
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_LIST List;
    COORD ClientSize;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    if (!List->HorizontalDisplay) {
        return FALSE;
    }

    if (ItemWidth < 5) {
        return FALSE;
    }

    YoriWinGetControlClientSize(&List->Ctrl, &ClientSize);

    if (ItemWidth > ClientSize.X) {
        List->HorizontalItemWidth = ClientSize.X;
    } else {
        List->HorizontalItemWidth = ItemWidth;
    }
    YoriWinListPaint(List);
    return TRUE;
}


/**
 Create a list control and add it to a window.  This is destroyed when the
 window is destroyed.

 @param ParentHandle Pointer to the parent window.

 @param Size Specifies the location and size of the button.

 @param Style Specifies style flags for the list including whether it should
        display a vertical scroll bar.

 @return Pointer to the newly created control or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinListCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in DWORD Style
    )
{
    PYORI_WIN_CTRL_LIST List;
    PYORI_WIN_WINDOW Parent;
    SMALL_RECT ScrollBarRect;
    WORD WindowAttributes;

    Parent = (PYORI_WIN_WINDOW)ParentHandle;

    List = YoriLibReferencedMalloc(sizeof(YORI_WIN_CTRL_LIST));
    if (List == NULL) {
        return NULL;
    }

    ZeroMemory(List, sizeof(YORI_WIN_CTRL_LIST));

    YoriWinItemArrayInitialize(&List->ItemArray);

    List->Ctrl.NotifyEventFn = YoriWinListEventHandler;
    if (!YoriWinCreateControl(Parent, Size, TRUE, &List->Ctrl)) {
        YoriLibDereference(List);
        return FALSE;
    }

    WindowAttributes = List->Ctrl.DefaultAttributes;

    if ((Style & YORI_WIN_LIST_STYLE_NO_BORDER) == 0) {
        List->DisplayBorder = TRUE;

        YoriWinDrawBorderOnControl(&List->Ctrl, &List->Ctrl.ClientRect, WindowAttributes, YORI_WIN_BORDER_TYPE_SUNKEN);
        List->Ctrl.ClientRect.Top++;
        List->Ctrl.ClientRect.Left++;
        List->Ctrl.ClientRect.Bottom--;
        List->Ctrl.ClientRect.Right--;
    }

    if (Style & YORI_WIN_LIST_STYLE_HORIZONTAL) {
        List->HorizontalDisplay = TRUE;
        YoriWinListSetHorizontalItemWidth(&List->Ctrl, 20);
    }

    if (Style & YORI_WIN_LIST_STYLE_VSCROLLBAR) {
        ScrollBarRect.Left = (SHORT)(List->Ctrl.FullRect.Right - List->Ctrl.FullRect.Left);
        ScrollBarRect.Right = ScrollBarRect.Left;
        ScrollBarRect.Top = 1;
        ScrollBarRect.Bottom = (SHORT)(List->Ctrl.FullRect.Bottom - List->Ctrl.FullRect.Top - 1);
        List->VScrollCtrl = YoriWinScrollBarCreate(&List->Ctrl, &ScrollBarRect, 0, YoriWinListNotifyScrollChange);
    }

    if (Style & YORI_WIN_LIST_STYLE_MULTISELECT) {
        List->MultiSelect = TRUE;
    }

    if (Style & YORI_WIN_LIST_STYLE_DESELECT_ON_LOSE_FOCUS) {
        List->DeselectOnLoseFocus = TRUE;
    }

    List->ItemActive = FALSE;

    YoriWinListEnsureActiveItemVisible(List);
    YoriWinListPaint(List);

    return &List->Ctrl;
}


// vim:sw=4:ts=4:et:
