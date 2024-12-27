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
     Pointer to the horizontal scroll bar associated with the list.
     */
    PYORI_WIN_CTRL HScrollCtrl;

    /**
     Callback function to notify after a selection has changed.
     */
    PYORI_WIN_NOTIFY SelectionChangeCallback;

    /**
     The set of options to display in the list
     */
    YORI_WIN_ITEM_ARRAY ItemArray;

    /**
     A string of keystrokes that the user has entered indicating the item to
     find.
     */
    YORI_STRING SearchString;

    /**
     The tick of when the last keystroke was entered.  Any key pressed quickly
     is appended to the string; if a delay occurs, the string is reset and the
     key constitutes a new search.
     */
    DWORD LastKeyTick;

    /**
     The index within ItemArray of the first array element to display in the
     list
     */
    YORI_ALLOC_SIZE_T FirstDisplayedOption;

    /**
     The index within ItemArray of the array element that is currently
     highlighted
     */
    YORI_ALLOC_SIZE_T ActiveOption;

    /**
     The offset in display cells to begin on the left most visible portion
     of the control.  Any text before this offset is not visible.
     */
    YORI_ALLOC_SIZE_T DisplayOffset;

    /**
     The length in display cells of the longest item within the list.
     */
    YORI_ALLOC_SIZE_T LongestItemLength;

    /**
     The number of character cells for each item when the list is displayed
     horizontally.
     */
    WORD HorizontalItemWidth;

    /**
     The color attributes to display the active item in.
     */
    WORD ActiveAttributes;

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

    /**
     If TRUE, a horizontal scroll bar should be created or destroyed based on
     the length of items within the list control.  If FALSE, it is still
     possible that a horizontal scrollbar is always present, or never present.
     */
    BOOLEAN AutoHorizontalScroll;

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

    if (List->FirstDisplayedOption > 0 &&
        List->FirstDisplayedOption + ElementCountToDisplay > List->ItemArray.Count) {

        if (List->ItemArray.Count < ElementCountToDisplay) {
            List->FirstDisplayedOption = 0;
        } else {
            List->FirstDisplayedOption = (WORD)(List->ItemArray.Count - ElementCountToDisplay);
        }
    }

    if (List->ActiveOption >= List->FirstDisplayedOption + ElementCountToDisplay) {
        List->FirstDisplayedOption = List->ActiveOption - ElementCountToDisplay + 1;
    }
}


/**
 Repaint the border around the list control, if a border is in use.  This is
 used when the control is repositioned or the horizontal scrollbar is being
 reconfigured.

 @param List Pointer to the list control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinListPaintBorder(
    __inout PYORI_WIN_CTRL_LIST List
    )
{
    SMALL_RECT BorderRect;
    WORD WindowAttributes;

    WindowAttributes = List->Ctrl.DefaultAttributes;
    if (List->DisplayBorder) {
        BorderRect.Left = 0;
        BorderRect.Top = 0;
        BorderRect.Right = (SHORT)(List->Ctrl.FullRect.Right - List->Ctrl.FullRect.Left);
        BorderRect.Bottom = (SHORT)(List->Ctrl.FullRect.Bottom - List->Ctrl.FullRect.Top);

        YoriWinDrawBorderOnControl(&List->Ctrl, &BorderRect, WindowAttributes, YORI_WIN_BORDER_TYPE_SUNKEN);
        return TRUE;
    }
    return FALSE;
}

VOID
YoriWinListNotifyHScrollChange(
    __in PYORI_WIN_CTRL_HANDLE ScrollCtrlHandle
    );

/**
 Create a horizontal scrollbar for the control.  This might happen when the
 list is being created or might happen dynamically if the list is being asked
 to contain longer elements.

 @param List Pointer to the list control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinListCreateHorizontalScrollbar(
    __inout PYORI_WIN_CTRL_LIST List
    )
{
    SMALL_RECT ScrollBarRect;

    ASSERT(List->HScrollCtrl == NULL);
    ScrollBarRect.Left = 1;
    ScrollBarRect.Right = (SHORT)(List->Ctrl.FullRect.Right - List->Ctrl.FullRect.Left - 1);
    ScrollBarRect.Top = (SHORT)(List->Ctrl.FullRect.Bottom - List->Ctrl.FullRect.Top);
    ScrollBarRect.Bottom = ScrollBarRect.Top;
    List->HScrollCtrl = YoriWinScrollBarCreate(&List->Ctrl, &ScrollBarRect, 0, YoriWinListNotifyHScrollChange);
    if (List->HScrollCtrl == NULL) {
        return FALSE;
    }
    return TRUE;
}

/**
 Return the number of cells per item that can be displayed.

 @param List Pointer to the list control.

 @return The number of cells per item that can be displayed.
 */
WORD
YoriWinListGetVisibleCellCountPerItem(
    __in PYORI_WIN_CTRL_LIST List
    )
{
    WORD CharsToDisplay;
    COORD ClientSize;

    YoriWinGetControlClientSize(&List->Ctrl, &ClientSize);

    CharsToDisplay = (WORD)ClientSize.X;
    if (List->MultiSelect) {
        CharsToDisplay = (WORD)(ClientSize.X - 2);
    }
    return CharsToDisplay;
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
    WORD MaxCharsToDisplay;
    WORD ElementCountToDisplay;
    WORD Attributes;
    WORD WindowAttributes;
    PYORI_WIN_ITEM_ENTRY Element;
    COORD ClientSize;
    YORI_STRING DisplayCells;
    YORI_STRING VisibleString;
    PYORI_WIN_WINDOW TopLevelWindow;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;
    YORI_ALLOC_SIZE_T ViewportBufferOffset;
    YORI_ALLOC_SIZE_T Remainder;

    TopLevelWindow = YoriWinGetTopLevelWindow(List->Ctrl.Parent);
    WinMgrHandle = YoriWinGetWindowManagerHandle(TopLevelWindow);

    WindowAttributes = List->Ctrl.DefaultAttributes;
    YoriWinGetControlClientSize(&List->Ctrl, &ClientSize);
    ElementCountToDisplay = ClientSize.Y;

    if (List->ItemArray.Count < ElementCountToDisplay) {
        ElementCountToDisplay = (WORD)List->ItemArray.Count;
    }

    MaxCharsToDisplay = YoriWinListGetVisibleCellCountPerItem(List);

    for (RowIndex = 0; RowIndex < ElementCountToDisplay; RowIndex++) {
        Element = &List->ItemArray.Items[List->FirstDisplayedOption + RowIndex];
        Attributes = WindowAttributes;
        if (List->ItemActive &&
            RowIndex + List->FirstDisplayedOption == List->ActiveOption) {

            Attributes = List->ActiveAttributes;
        }

        YoriWinTextBufferOffsetFromDisplayCellOffset(WinMgrHandle,
                                                     &Element->String,
                                                     1,
                                                     List->DisplayOffset,
                                                     FALSE,
                                                     &ViewportBufferOffset,
                                                     &Remainder);

        YoriLibInitEmptyString(&VisibleString);
        VisibleString.StartOfString = &Element->String.StartOfString[ViewportBufferOffset];
        VisibleString.LengthInChars = Element->String.LengthInChars - ViewportBufferOffset;

        YoriLibInitEmptyString(&DisplayCells);
        if (!YoriWinTextStringToDisplayCells(WinMgrHandle,
                                             &VisibleString,
                                             Remainder,
                                             1,
                                             ClientSize.X,
                                             &DisplayCells)) {
            DisplayCells.StartOfString = Element->String.StartOfString;
            DisplayCells.LengthInChars = Element->String.LengthInChars;
        }

        CharsToDisplay = MaxCharsToDisplay;
        if (CharsToDisplay > DisplayCells.LengthInChars) {
            CharsToDisplay = (WORD)DisplayCells.LengthInChars;
        }
        if (List->MultiSelect) {
            if (Element->Flags & YORI_WIN_ITEM_SELECTED) {
                YoriWinSetControlClientCell(&List->Ctrl, 0, RowIndex, '*', Attributes);
            } else {
                YoriWinSetControlClientCell(&List->Ctrl, 0, RowIndex, ' ', Attributes);
            }
            YoriWinSetControlClientCell(&List->Ctrl, 1, RowIndex, ' ', Attributes);
            for (CellIndex = 0; CellIndex < CharsToDisplay; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellIndex + 2), RowIndex, DisplayCells.StartOfString[CellIndex], Attributes);
            }
            for (;CellIndex < ClientSize.X - 2; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellIndex + 2), RowIndex, ' ', Attributes);
            }

        } else {
            for (CellIndex = 0; CellIndex < CharsToDisplay; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, CellIndex, RowIndex, DisplayCells.StartOfString[CellIndex], Attributes);
            }
            for (;CellIndex < ClientSize.X; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, CellIndex, RowIndex, ' ', Attributes);
            }
        }
        YoriLibFreeStringContents(&DisplayCells);
    }

    //
    //  Clear any rows following rows with contents
    //

    for (;RowIndex < ClientSize.Y; RowIndex++) {
        for (CellIndex = 0; CellIndex < ClientSize.X; CellIndex++) {
            YoriWinSetControlClientCell(&List->Ctrl, CellIndex, RowIndex, ' ', WindowAttributes);
        }
    }

    //
    //  If a horizontal scroll bar exists but no elements are wide enough to
    //  need it, or if it doesn't exist but elements are wide enough to need
    //  it, delete or create it respectively.
    //

    if (List->AutoHorizontalScroll) {
        if (List->LongestItemLength <= MaxCharsToDisplay) {
            if (List->HScrollCtrl != NULL) {
                YoriWinCloseControl(List->HScrollCtrl);
                YoriWinListPaintBorder(List);
                List->HScrollCtrl = NULL;
            }
        } else {
            if (List->HScrollCtrl == NULL) {
                YoriWinListCreateHorizontalScrollbar(List);
            }
        }
    }

    //
    //  Update and possibly redraw scroll bars.
    //

    if (List->VScrollCtrl) {
        DWORD MaximumTopValue;
        if (List->ItemArray.Count > (DWORD)ClientSize.Y) {
            MaximumTopValue = List->ItemArray.Count - ClientSize.Y;
        } else {
            MaximumTopValue = 0;
        }

        YoriWinScrollBarSetPosition(List->VScrollCtrl, List->FirstDisplayedOption, ElementCountToDisplay, MaximumTopValue);
    }

    if (List->HScrollCtrl != NULL) {
        YORI_MAX_UNSIGNED_T MaximumInitialValue;
        if (List->LongestItemLength > MaxCharsToDisplay) {
            MaximumInitialValue = List->LongestItemLength - MaxCharsToDisplay;
        } else {
            ASSERT(List->DisplayOffset == 0);
            MaximumInitialValue = 0;
        }
        YoriWinScrollBarSetPosition(List->HScrollCtrl, List->DisplayOffset, ClientSize.X, MaximumInitialValue);
    }

    //
    //  Display cursor if the control has focus.
    //

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
    YORI_STRING DisplayLine;
    PYORI_WIN_WINDOW TopLevelWindow;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

    TopLevelWindow = YoriWinGetTopLevelWindow(List->Ctrl.Parent);
    WinMgrHandle = YoriWinGetWindowManagerHandle(TopLevelWindow);

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
        YoriLibInitEmptyString(&DisplayLine);
        if (!YoriWinTextStringToDisplayCells(WinMgrHandle, &Element->String, 0, 3, List->HorizontalItemWidth, &DisplayLine)) {
            DisplayLine.StartOfString = Element->String.StartOfString;
            DisplayLine.LengthInChars = Element->String.LengthInChars;
        }
        if (List->MultiSelect) {
            CharsToDisplay = (WORD)(List->HorizontalItemWidth - 4);
            if (CharsToDisplay > DisplayLine.LengthInChars) {
                CharsToDisplay = (WORD)DisplayLine.LengthInChars;
            }
            YoriWinSetControlClientCell(&List->Ctrl, CellOffset, 0, ' ', Attributes);
            if (Element->Flags & YORI_WIN_ITEM_SELECTED) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + 1), 0, '*', Attributes);
            } else {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + 1), 0, ' ', Attributes);
            }
            YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + 2), 0, ' ', Attributes);
            for (CellIndex = 0; CellIndex < CharsToDisplay; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + CellIndex + 3), 0, DisplayLine.StartOfString[CellIndex], Attributes);
            }
            for (;CellIndex < List->HorizontalItemWidth - 3; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + CellIndex + 3), 0, ' ', Attributes);
            }

        } else {
            CharsToDisplay = (WORD)(List->HorizontalItemWidth - 2);
            if (CharsToDisplay > DisplayLine.LengthInChars) {
                CharsToDisplay = (WORD)DisplayLine.LengthInChars;
            }
            YoriWinSetControlClientCell(&List->Ctrl, CellOffset, 0, ' ', Attributes);
            for (CellIndex = 0; CellIndex < CharsToDisplay; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + CellIndex + 1), 0, DisplayLine.StartOfString[CellIndex], Attributes);
            }
            for (;CellIndex < List->HorizontalItemWidth - 1; CellIndex++) {
                YoriWinSetControlClientCell(&List->Ctrl, (WORD)(CellOffset + CellIndex + 1), 0, ' ', Attributes);
            }
        }
        YoriLibFreeStringContents(&DisplayLine);
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
 Scan through all items in the list to determine the length of the longest
 item.

 @param List Pointer to the list control.
 */
VOID
YoriWinListRecalculateLongestItem(
    __in PYORI_WIN_CTRL_LIST List
    )
{
    YORI_ALLOC_SIZE_T Index;
    YORI_ALLOC_SIZE_T LongestItemLength;
    YORI_ALLOC_SIZE_T ThisLength;
    PYORI_STRING Text;
    PYORI_WIN_WINDOW TopLevelWindow;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

    TopLevelWindow = YoriWinGetTopLevelWindow(List->Ctrl.Parent);
    WinMgrHandle = YoriWinGetWindowManagerHandle(TopLevelWindow);

    LongestItemLength = 0;
    for (Index = 0; Index < List->ItemArray.Count; Index++) {
        Text = &List->ItemArray.Items[Index].String;
        YoriWinTextDisplayCellOffsetFromBufferOffset(WinMgrHandle, Text, 1, Text->LengthInChars - 1, &ThisLength);
        ThisLength = ThisLength + 2;
        if (ThisLength > LongestItemLength) {
            LongestItemLength = ThisLength;
        }
    }

    ASSERT(List->DisplayOffset <= LongestItemLength);
    List->LongestItemLength = LongestItemLength;
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
    List->DisplayOffset = 0;
    YoriWinListRecalculateLongestItem(List);
    YoriWinListPaint(List);
    return TRUE;
}

/**
 Invoked when the user manipulates the horizontal scroll bar to indicate that
 the offset within the text should be updated.

 @param ScrollCtrlHandle Pointer to the scroll bar control.
 */
VOID
YoriWinListNotifyHScrollChange(
    __in PYORI_WIN_CTRL_HANDLE ScrollCtrlHandle
    )
{
    PYORI_WIN_CTRL_LIST List;
    PYORI_WIN_CTRL ScrollCtrl;
    DWORDLONG ScrollValue;
    WORD MaxCharsToDisplay;

    ScrollCtrl = (PYORI_WIN_CTRL)ScrollCtrlHandle;
    List = CONTAINING_RECORD(ScrollCtrl->Parent, YORI_WIN_CTRL_LIST, Ctrl);
    ASSERT(List->HScrollCtrl == ScrollCtrl);

    MaxCharsToDisplay = YoriWinListGetVisibleCellCountPerItem(List);

    ScrollValue = YoriWinScrollBarGetPosition(ScrollCtrl);
    if (ScrollValue < MaxCharsToDisplay) {
        List->DisplayOffset = (YORI_ALLOC_SIZE_T)ScrollValue;
        YoriWinListPaint(List);
    }
}

/**
 Invoked when the user manipulates the vertical scroll bar to indicate that
 the position within the list should be updated.

 @param ScrollCtrlHandle Pointer to the scroll bar control.
 */
VOID
YoriWinListNotifyVScrollChange(
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
            List->FirstDisplayedOption = (YORI_ALLOC_SIZE_T)ScrollValue;
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
    __in YORI_ALLOC_SIZE_T LinesToMove,
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
    __out PYORI_ALLOC_SIZE_T SelectedItem
    )
{
    YORI_ALLOC_SIZE_T ItemRelativeToFirstDisplayed;

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
 Given a user pressed character, look for an item in the list that starts with
 the character.  This might get smarter for consecutive characters someday.

 @param List Pointer to the list control.

 @param Char The character that was pressed.

 @return TRUE to indicate the selection has changed and paint is needed;
         FALSE if no match was found.
 */
BOOLEAN
YoriWinListFindItemByChar(
    __in PYORI_WIN_CTRL_LIST List,
    __in TCHAR Char
    )
{
    YORI_ALLOC_SIZE_T Index;
    PYORI_WIN_ITEM_ENTRY Element;
    DWORD CurrentTick;

    //
    //  If a key press took longer than 500ms, treat it as the beginning of
    //  a new search
    //

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 28159) // Deprecated GetTickCount; overflows are
                                 // deterministic
#endif
    CurrentTick = GetTickCount();
    if (List->LastKeyTick + 500 < CurrentTick) {
        List->SearchString.LengthInChars = 0;
    }

    if (List->SearchString.LengthInChars + 1 > List->SearchString.LengthAllocated) {
        if (!YoriLibReallocString(&List->SearchString, List->SearchString.LengthAllocated + 128)) {
            return FALSE;
        }
    }

    List->SearchString.StartOfString[List->SearchString.LengthInChars] = Char;
    List->SearchString.LengthInChars = List->SearchString.LengthInChars++;
    List->LastKeyTick = CurrentTick;

    //
    //  If nothing is selected, search from the top.  If something is
    //  selected, start from that one, and wrap from the top if nothing is
    //  found.
    //

    if (!List->ItemActive) {

        for (Index = 0; Index < List->ItemArray.Count; Index++) {
            Element = &List->ItemArray.Items[Index];
            if (Element->String.LengthInChars > 0 &&
                YoriLibCompareStringInsCnt(&List->SearchString, &Element->String, List->SearchString.LengthInChars) == 0) {

                List->ItemActive = TRUE;
                List->ActiveOption = Index;
                return TRUE;
            }
        }

    } else {

        for (Index = List->ActiveOption; Index < List->ItemArray.Count; Index++) {
            Element = &List->ItemArray.Items[Index];
            if (Element->String.LengthInChars > 0 &&
                YoriLibCompareStringInsCnt(&List->SearchString, &Element->String, List->SearchString.LengthInChars) == 0) {

                List->ActiveOption = Index;
                return TRUE;
            }
        }

        for (Index = 0; Index < List->ActiveOption; Index++) {
            Element = &List->ItemArray.Items[Index];
            if (Element->String.LengthInChars > 0 &&
                YoriLibCompareStringInsCnt(&List->SearchString, &Element->String, List->SearchString.LengthInChars) == 0) {

                List->ActiveOption = Index;
                return TRUE;
            }
        }
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
    YORI_ALLOC_SIZE_T NewOption;
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
                    YORI_ALLOC_SIZE_T ElementCountToDisplay;
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
                    YORI_ALLOC_SIZE_T ElementCountToDisplay;
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
                } else if (Event->KeyDown.VirtualKeyCode == VK_LEFT &&
                           !List->HorizontalDisplay) {
                    if (List->DisplayOffset > 0) {
                        List->DisplayOffset = List->DisplayOffset - 1;
                        YoriWinListPaint(List);
                    }
                } else if (Event->KeyDown.VirtualKeyCode == VK_RIGHT &&
                           !List->HorizontalDisplay) {
                    WORD MaxCharsToDisplay;
                    MaxCharsToDisplay = YoriWinListGetVisibleCellCountPerItem(List);
                    if (List->DisplayOffset + MaxCharsToDisplay < List->LongestItemLength) {
                        List->DisplayOffset = List->DisplayOffset + 1;
                        YoriWinListPaint(List);
                    }
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
                } else if (Event->KeyDown.Char >= ' ') {
                    if (YoriWinListFindItemByChar(List, Event->KeyDown.Char)) {
                        YoriWinListEnsureActiveItemVisible(List);
                        if (List->SelectionChangeCallback) {
                            List->SelectionChangeCallback(&List->Ctrl);
                        }
                        YoriWinListPaint(List);
                    }
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
            YoriWinListNotifyMouseWheel(List, (YORI_ALLOC_SIZE_T)Event->MouseWheel.LinesToMove, FALSE);
            break;

        case YoriWinEventMouseWheelUpInClient:
        case YoriWinEventMouseWheelUpInNonClient:
            YoriWinListNotifyMouseWheel(List, (YORI_ALLOC_SIZE_T)Event->MouseWheel.LinesToMove, TRUE);
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
            YoriLibFreeStringContents(&List->SearchString);
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
    __out PYORI_ALLOC_SIZE_T CurrentlyActiveIndex
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
    __in YORI_ALLOC_SIZE_T ActiveOption
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
    __in YORI_ALLOC_SIZE_T Index
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
    __in PCYORI_STRING ListOptions,
    __in YORI_ALLOC_SIZE_T NumberOptions
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
    List->DisplayOffset = 0;
    YoriWinListRecalculateLongestItem(List);
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
    List->DisplayOffset = 0;
    YoriWinListRecalculateLongestItem(List);
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
    __in YORI_ALLOC_SIZE_T Index,
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
    SMALL_RECT ScrollBarRect;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    List = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_LIST, Ctrl);

    if (!YoriWinControlReposition(Ctrl, CtrlRect)) {
        return FALSE;
    }

    YoriWinListEnsureActiveItemVisible(List);

    WindowAttributes = List->Ctrl.DefaultAttributes;

    YoriWinListPaintBorder(List);

    if (List->HScrollCtrl != NULL) {

        ScrollBarRect.Left = 1;
        ScrollBarRect.Right = (SHORT)(List->Ctrl.FullRect.Right - List->Ctrl.FullRect.Left - 1);
        ScrollBarRect.Top = (SHORT)(List->Ctrl.FullRect.Bottom - List->Ctrl.FullRect.Top);
        ScrollBarRect.Bottom = ScrollBarRect.Top;

        YoriWinScrollBarReposition(List->HScrollCtrl, &ScrollBarRect);
    }

    if (List->VScrollCtrl != NULL) {

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
    PYORI_WIN_WINDOW TopLevelWindow;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

    Parent = (PYORI_WIN_WINDOW)ParentHandle;

    //
    //  Scrollbars need borders, and need a vertical control.
    //

    if ((Style & (YORI_WIN_LIST_STYLE_NO_BORDER | YORI_WIN_LIST_STYLE_HORIZONTAL)) != 0) {
        if ((Style & (YORI_WIN_LIST_STYLE_VSCROLLBAR|YORI_WIN_LIST_STYLE_HSCROLLBAR|YORI_WIN_LIST_STYLE_AUTO_HSCROLLBAR)) != 0) {
            return NULL;
        }
    }

    List = YoriLibReferencedMalloc(sizeof(YORI_WIN_CTRL_LIST));
    if (List == NULL) {
        return NULL;
    }

    ZeroMemory(List, sizeof(YORI_WIN_CTRL_LIST));

    YoriWinItemArrayInitialize(&List->ItemArray);

    List->Ctrl.NotifyEventFn = YoriWinListEventHandler;
    if (!YoriWinCreateControl(Parent, Size, TRUE, TRUE, &List->Ctrl)) {
        YoriLibDereference(List);
        return FALSE;
    }

    TopLevelWindow = YoriWinGetTopLevelWindow(Parent);
    WinMgrHandle = YoriWinGetWindowManagerHandle(TopLevelWindow);

    List->ActiveAttributes = YoriWinMgrDefaultColorLookup(WinMgrHandle, YoriWinColorListActive);

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

    if (Style & YORI_WIN_LIST_STYLE_AUTO_HSCROLLBAR) {
        List->AutoHorizontalScroll = TRUE;
    } else if (Style & YORI_WIN_LIST_STYLE_HSCROLLBAR) {
        YoriWinListCreateHorizontalScrollbar(List);
    }

    if (Style & YORI_WIN_LIST_STYLE_VSCROLLBAR) {
        ScrollBarRect.Left = (SHORT)(List->Ctrl.FullRect.Right - List->Ctrl.FullRect.Left);
        ScrollBarRect.Right = ScrollBarRect.Left;
        ScrollBarRect.Top = 1;
        ScrollBarRect.Bottom = (SHORT)(List->Ctrl.FullRect.Bottom - List->Ctrl.FullRect.Top - 1);
        List->VScrollCtrl = YoriWinScrollBarCreate(&List->Ctrl, &ScrollBarRect, 0, YoriWinListNotifyVScrollChange);
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
