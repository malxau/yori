/**
 * @file lib/select.c
 *
 * Yori shell select region on the console and copy to clipboard
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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

/**
 Returns TRUE if the current selection region is active, and FALSE if it
 is not.

 @param Selection The structure describing the current selection state.

 @return TRUE if a current selection is active, FALSE if it is not.
 */
BOOL
YoriLibIsSelectionActive(
    __in PYORILIB_SELECTION Selection
    )
{
    if (Selection->Current.Left == Selection->Current.Right &&
        Selection->Current.Top == Selection->Current.Bottom) {

        return FALSE;
    }

    return TRUE;
}

/**
 Returns TRUE if a previous selection region is active, and FALSE if it
 is not.

 @param Selection The structure describing the current selection state.

 @return TRUE if a current selection is active, FALSE if it is not.
 */
BOOL
YoriLibIsPreviousSelectionActive(
    __in PYORILIB_SELECTION Selection
    )
{
    if (Selection->Previous.Left == Selection->Previous.Right &&
        Selection->Previous.Top == Selection->Previous.Bottom) {

        return FALSE;
    }

    return TRUE;
}

/**
 Update a range of console cells with specified attributes.  If the attributes
 don't exist due to allocation failure, use a default attribute for the
 entire range.

 @param ConsoleHandle Handle to the console.

 @param AttributeArray If specified, pointer to an array of attributes to
        apply.  If not specified, DefaultAttribute is used instead.

 @param DefaultAttribute The attribute to use for the range if AttributeArray
        is NULL.

 @param Length The number of cells to update.

 @param StartPoint The coordinates in the console buffer to start updating
        from.

 @param CharsWritten On successful completion, updated to contain the number
        of characters updated in the console.
 */
VOID
YoriLibDisplayAttributes(
    __in HANDLE ConsoleHandle,
    __in_opt PWORD AttributeArray,
    __in WORD DefaultAttribute,
    __in DWORD Length,
    __in COORD StartPoint,
    __out LPDWORD CharsWritten
    )
{
    if (AttributeArray) {
        WriteConsoleOutputAttribute(ConsoleHandle, AttributeArray, Length, StartPoint, CharsWritten);
    } else {
        FillConsoleOutputAttribute(ConsoleHandle, DefaultAttribute, Length, StartPoint, CharsWritten);
    }
}


/**
 Allocate an attribute buffer, or reallocate one if it's already allocated,
 to contain the specified number of elements.

 @param AttributeBuffer Pointer to the attribute buffer to allocate or
        reallocate.

 @param RequiredLength Specifies the number of cells to allocate.

 @return TRUE if allocation succeeded, FALSE if it did not.
 */
BOOL
YoriLibReallocateAttributeArray(
    __in PYORILIB_PREVIOUS_SELECTION_BUFFER AttributeBuffer,
    __in DWORD RequiredLength
    )
{
    if (AttributeBuffer->AttributeArray != NULL) {
        YoriLibFree(AttributeBuffer->AttributeArray);
        AttributeBuffer->AttributeArray = NULL;
    }

    //
    //  Allocate more than we strictly need so as to reduce the number of
    //  reallocations
    //

    AttributeBuffer->AttributeArray = YoriLibMalloc(RequiredLength * sizeof(WORD));
    if (AttributeBuffer->AttributeArray != NULL) {
        AttributeBuffer->BufferSize = RequiredLength;
        return TRUE;
    } else {
        AttributeBuffer->BufferSize = 0;
        return FALSE;
    }
}

/**
 Redraw any cells covered by a previous selection, restoring their original
 character attributes.

 @param Selection The selection to clear the displayed selection for.
 */
VOID
YoriLibClearPreviousSelectionDisplay(
    __in PYORILIB_SELECTION Selection
    )
{
    SHORT LineIndex;
    SHORT LineLength;
    HANDLE ConsoleHandle;
    COORD StartPoint;
    DWORD CharsWritten;
    PWORD AttributeReadPoint;
    PYORILIB_PREVIOUS_SELECTION_BUFFER ActiveAttributes;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    //
    //  If there was no previous selection, clearing it is easy
    //

    if (!YoriLibIsPreviousSelectionActive(Selection)) {
        return;
    }

    //
    //  Grab a pointer to the previous selection attributes.  Note the
    //  actual buffer can be NULL if there was an allocation failure
    //  when selecting.  In that case attributes are restored to a
    //  default color.
    //

    ActiveAttributes = &Selection->PreviousBuffer[Selection->CurrentPreviousIndex];
    AttributeReadPoint = ActiveAttributes->AttributeArray;

    LineLength = (SHORT)(Selection->Previous.Right - Selection->Previous.Left + 1);

    for (LineIndex = Selection->Previous.Top; LineIndex <= Selection->Previous.Bottom; LineIndex++) {
        StartPoint.X = Selection->Previous.Left;
        StartPoint.Y = LineIndex;

        YoriLibDisplayAttributes(ConsoleHandle, AttributeReadPoint, 0x07, LineLength, StartPoint, &CharsWritten);

        if (AttributeReadPoint) {
            AttributeReadPoint += LineLength;
        }
    }
}

/**
 Return the selection color to use.  On Vista and newer systems this is the
 console popup color, which is what quickedit would do.  On older systems it
 is currently hardcoded to Yellow on Blue.

 @param ConsoleHandle Handle to the console output device.

 @return The color to use for selection.
 */
WORD
YoriLibGetSelectionColor(
    __in HANDLE ConsoleHandle
    )
{
    WORD SelectionColor;

    SelectionColor = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    if (DllKernel32.pGetConsoleScreenBufferInfoEx) {
        YORI_CONSOLE_SCREEN_BUFFER_INFOEX ScreenInfo;
        ZeroMemory(&ScreenInfo, sizeof(ScreenInfo));
        ScreenInfo.cbSize = sizeof(ScreenInfo);
        if (DllKernel32.pGetConsoleScreenBufferInfoEx(ConsoleHandle, &ScreenInfo)) {
            SelectionColor = ScreenInfo.wPopupAttributes;
        }
    }

    return SelectionColor;
}

/**
 Draw the selection highlight around the current selection, and save off the
 character attributes of the text underneath the selection.

 @param Selection The selection to display the selection for.
 */
VOID
YoriLibDrawCurrentSelectionDisplay(
    __in PYORILIB_SELECTION Selection
    )
{
    SHORT LineIndex;
    SHORT LineLength;
    HANDLE ConsoleHandle;
    COORD StartPoint;
    DWORD CharsWritten;
    DWORD RequiredLength;
    PWORD AttributeWritePoint;
    WORD SelectionColor;
    PYORILIB_PREVIOUS_SELECTION_BUFFER ActiveAttributes;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    //
    //  If there is no current selection, drawing it is easy
    //

    if (!YoriLibIsSelectionActive(Selection)) {

        return;
    }

    RequiredLength = (Selection->Current.Right - Selection->Current.Left + 1) * (Selection->Current.Bottom - Selection->Current.Top + 1);

    ActiveAttributes = &Selection->PreviousBuffer[Selection->CurrentPreviousIndex];

    if (ActiveAttributes->BufferSize < RequiredLength) {

        //
        //  Allocate more than we strictly need so as to reduce the number of
        //  reallocations
        //

        RequiredLength = RequiredLength * 2;
        YoriLibReallocateAttributeArray(ActiveAttributes, RequiredLength);
    }

    //
    //  Note this can be NULL on allocation failure
    //

    AttributeWritePoint = ActiveAttributes->AttributeArray;
    LineLength = (SHORT)(Selection->Current.Right - Selection->Current.Left + 1);

    SelectionColor = YoriLibGetSelectionColor(ConsoleHandle);

    for (LineIndex = Selection->Current.Top; LineIndex <= Selection->Current.Bottom; LineIndex++) {
        StartPoint.X = Selection->Current.Left;
        StartPoint.Y = LineIndex;

        if (AttributeWritePoint != NULL) {
            ReadConsoleOutputAttribute(ConsoleHandle, AttributeWritePoint, LineLength, StartPoint, &CharsWritten);
            AttributeWritePoint += LineLength;
        }

        FillConsoleOutputAttribute(ConsoleHandle, SelectionColor, LineLength, StartPoint, &CharsWritten);
    }
}

/**
 Draw the selection highlight around the current selection, and save off the
 character attributes of the text underneath the selection.

 @param Selection The selection to display the selection for.
 */
VOID
YoriLibDrawCurrentSelectionOverPreviousSelection(
    __in PYORILIB_SELECTION Selection
    )
{
    SHORT LineIndex;
    SHORT LineLength;
    SHORT RunLength;
    HANDLE ConsoleHandle;
    COORD StartPoint;
    DWORD BufferOffset;
    PWORD BufferPointer;
    DWORD CharsWritten;
    DWORD RequiredLength;
    PWORD AttributeWritePoint;
    PYORILIB_PREVIOUS_SELECTION_BUFFER NewAttributes;
    PYORILIB_PREVIOUS_SELECTION_BUFFER OldAttributes;
    DWORD NewAttributeIndex;
    WORD SelectionColor;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    ASSERT(YoriLibIsPreviousSelectionActive(Selection) &&
           YoriLibIsSelectionActive(Selection));

    //
    //  Find the buffers that are not the ones that currently contain the
    //  attributes of selected cells.  We'll fill that buffer with updated
    //  information, typically drawn from the currently active buffer.
    //

    NewAttributeIndex = (Selection->CurrentPreviousIndex + 1) % 2;
    NewAttributes = &Selection->PreviousBuffer[NewAttributeIndex];
    OldAttributes = &Selection->PreviousBuffer[Selection->CurrentPreviousIndex];

    RequiredLength = (Selection->Current.Right - Selection->Current.Left + 1) * (Selection->Current.Bottom - Selection->Current.Top + 1);

    if (RequiredLength > NewAttributes->BufferSize) {
        RequiredLength = RequiredLength * 2;
        YoriLibReallocateAttributeArray(NewAttributes, RequiredLength);
    }

    LineLength = (SHORT)(Selection->Current.Right - Selection->Current.Left + 1);

    //
    //  Walk through all of the new selection to save off attributes for it,
    //  and update the console to have selection color
    //

    SelectionColor = YoriLibGetSelectionColor(ConsoleHandle);
    AttributeWritePoint = NewAttributes->AttributeArray;
    for (LineIndex = Selection->Current.Top; LineIndex <= Selection->Current.Bottom; LineIndex++) {

        //
        //  An entire line wasn't previously selected
        //

        if (LineIndex < Selection->Previous.Top ||
            LineIndex > Selection->Previous.Bottom) {

            StartPoint.X = Selection->Current.Left;
            StartPoint.Y = LineIndex;

            if (AttributeWritePoint != NULL) {
                ReadConsoleOutputAttribute(ConsoleHandle, AttributeWritePoint, LineLength, StartPoint, &CharsWritten);
                AttributeWritePoint += LineLength;
            }

            FillConsoleOutputAttribute(ConsoleHandle, SelectionColor, LineLength, StartPoint, &CharsWritten);

        } else {

            //
            //  A set of characters to the left of the previous selection
            //  that are now selected
            //

            if (Selection->Current.Left < Selection->Previous.Left) {

                StartPoint.X = Selection->Current.Left;
                StartPoint.Y = LineIndex;

                RunLength = (SHORT)(Selection->Previous.Left - Selection->Current.Left);
                if (LineLength < RunLength) {
                    RunLength = LineLength;
                }

                if (AttributeWritePoint != NULL) {
                    ReadConsoleOutputAttribute(ConsoleHandle, AttributeWritePoint, RunLength, StartPoint, &CharsWritten);
                    AttributeWritePoint += RunLength;
                }

                FillConsoleOutputAttribute(ConsoleHandle, SelectionColor, RunLength, StartPoint, &CharsWritten);

            }

            //
            //  A set of characters were previously selected.  These
            //  attributes need to be migrated to the new buffer, but
            //  the console state is already correct
            //

            if (Selection->Current.Right >= Selection->Previous.Left &&
                Selection->Current.Left <= Selection->Previous.Right) {

                StartPoint.X = (SHORT)(Selection->Current.Left - Selection->Previous.Left);
                RunLength = LineLength;
                if (StartPoint.X < 0) {
                    RunLength = (SHORT)(RunLength + StartPoint.X);
                    StartPoint.X = 0;
                }
                if (StartPoint.X + RunLength > Selection->Previous.Right - Selection->Previous.Left + 1) {
                    RunLength = (SHORT)(Selection->Previous.Right - Selection->Previous.Left + 1 - StartPoint.X);
                }

                StartPoint.Y = (SHORT)(LineIndex - Selection->Previous.Top);

                BufferOffset = (Selection->Previous.Right - Selection->Previous.Left + 1) * StartPoint.Y + StartPoint.X;

                if (AttributeWritePoint != NULL) {
                    if (OldAttributes->AttributeArray != NULL) {
                        memcpy(AttributeWritePoint, &OldAttributes->AttributeArray[BufferOffset], RunLength * sizeof(WORD));
                    }

                    AttributeWritePoint += RunLength;
                }
            }

            //
            //  A set of characters to the right of the previous selection
            //  that are now selected
            //

            if (Selection->Current.Right > Selection->Previous.Right) {

                StartPoint.X = (SHORT)(Selection->Previous.Right + 1);
                if (Selection->Current.Left > StartPoint.X) {
                    StartPoint.X = Selection->Current.Left;
                    RunLength = LineLength;
                } else {
                    RunLength = (SHORT)(Selection->Current.Right - StartPoint.X + 1);
                }
                StartPoint.Y = LineIndex;

                if (AttributeWritePoint != NULL) {
                    ReadConsoleOutputAttribute(ConsoleHandle, AttributeWritePoint, RunLength, StartPoint, &CharsWritten);
                    AttributeWritePoint += RunLength;
                }

                FillConsoleOutputAttribute(ConsoleHandle, SelectionColor, RunLength, StartPoint, &CharsWritten);
            }
        }
    }

    //
    //  Go through the old selection looking for regions that are no longer
    //  selected, and restore their attributes into the console
    //

    LineLength = (SHORT)(Selection->Previous.Right - Selection->Previous.Left + 1);

    for (LineIndex = Selection->Previous.Top; LineIndex <= Selection->Previous.Bottom; LineIndex++) {

        //
        //  A line was previously selected and no longer is.  Restore the
        //  saved attributes.
        //

        if (LineIndex < Selection->Current.Top ||
            LineIndex > Selection->Current.Bottom) {

            StartPoint.X = Selection->Previous.Left;
            StartPoint.Y = LineIndex;
            RunLength = LineLength;

            if (OldAttributes->AttributeArray != NULL) {
                BufferOffset = LineLength * (LineIndex - Selection->Previous.Top);
                BufferPointer = &OldAttributes->AttributeArray[BufferOffset];
            } else {
                BufferPointer = NULL;
            }

            YoriLibDisplayAttributes(ConsoleHandle, BufferPointer, 0x07, RunLength, StartPoint, &CharsWritten);

        } else {

            //
            //  A region to the left of the currently selected region was
            //  previously selected.  Restore the saved attributes.
            //

            if (Selection->Previous.Left < Selection->Current.Left) {

                StartPoint.X = Selection->Previous.Left;
                StartPoint.Y = LineIndex;
                RunLength = LineLength;
                if (Selection->Current.Left - Selection->Previous.Left < RunLength) {
                    RunLength = (SHORT)(Selection->Current.Left - Selection->Previous.Left);
                }

                if (OldAttributes->AttributeArray != NULL) {
                    BufferOffset = LineLength * (LineIndex - Selection->Previous.Top);
                    BufferPointer = &OldAttributes->AttributeArray[BufferOffset];
                } else {
                    BufferPointer = NULL;
                }

                YoriLibDisplayAttributes(ConsoleHandle, BufferPointer, 0x07, RunLength, StartPoint, &CharsWritten);

            }

            //
            //  A region to the right of the current selection was previously
            //  selected.  Restore the saved attributes.
            //

            if (Selection->Previous.Right > Selection->Current.Right) {

                BufferOffset = LineLength * (LineIndex - Selection->Previous.Top);
                StartPoint.Y = LineIndex;

                if (Selection->Previous.Left > Selection->Current.Right) {
                    StartPoint.X = Selection->Previous.Left;
                    RunLength = LineLength;
                } else {
                    RunLength = (SHORT)(Selection->Previous.Right - Selection->Current.Right);
                    StartPoint.X = (SHORT)(Selection->Current.Right + 1);
                    BufferOffset += Selection->Current.Right - Selection->Previous.Left + 1;
                }

                if (OldAttributes->AttributeArray != NULL) {
                    BufferPointer = &OldAttributes->AttributeArray[BufferOffset];
                } else {
                    BufferPointer = NULL;
                }

                YoriLibDisplayAttributes(ConsoleHandle, BufferPointer, 0x07, RunLength, StartPoint, &CharsWritten);
            }
        }
    }

    ASSERT(Selection->CurrentPreviousIndex != NewAttributeIndex);
    Selection->CurrentPreviousIndex = NewAttributeIndex;
}

/**
 Redraw a selection.  This will restore any previously selected cells to their
 original values and will display the result of the current selection.

 @param Selection Pointer to the selection to redraw.
 */
VOID
YoriLibRedrawSelection(
    __inout PYORILIB_SELECTION Selection
    )
{
    if (YoriLibIsPreviousSelectionActive(Selection) &&
        YoriLibIsSelectionActive(Selection)) {
        YoriLibDrawCurrentSelectionOverPreviousSelection(Selection);
    } else {
        YoriLibClearPreviousSelectionDisplay(Selection);
        YoriLibDrawCurrentSelectionDisplay(Selection);
    }
    Selection->Previous.Left = Selection->Current.Left;
    Selection->Previous.Top = Selection->Current.Top;
    Selection->Previous.Right = Selection->Current.Right;
    Selection->Previous.Bottom = Selection->Current.Bottom;
}

/**
 Deallocate any internal allocations within a selection.

 @param Selection Pointer to the selection to clean up.
 */
VOID
YoriLibCleanupSelection(
    __inout PYORILIB_SELECTION Selection
    )
{
    DWORD Index;

    for (Index = 0; Index < 2; Index++) {
        if (Selection->PreviousBuffer[Index].AttributeArray) {
            YoriLibFree(Selection->PreviousBuffer[Index].AttributeArray);
            Selection->PreviousBuffer[Index].AttributeArray = NULL;
            Selection->PreviousBuffer[Index].BufferSize = 0;
        }
    }
}

/**
 Clear any current selection.  Note this is clearing in memory state and it
 will not be re-rendered on the screen until that action is requested.

 @param Selection Pointer to the selection to clear.

 @return TRUE to indicate a selection was cleared and the buffer requires
         redrawing; FALSE if no redrawing is required.
 */
BOOL
YoriLibClearSelection(
    __inout PYORILIB_SELECTION Selection
    )
{
    Selection->Current.Left = 0;
    Selection->Current.Right = 0;
    Selection->Current.Top = 0;
    Selection->Current.Bottom = 0;

    Selection->PeriodicScrollAmount.X = 0;
    Selection->PeriodicScrollAmount.Y = 0;

    if (Selection->Current.Left != Selection->Previous.Left ||
        Selection->Current.Right != Selection->Previous.Right ||
        Selection->Current.Top != Selection->Previous.Top ||
        Selection->Current.Bottom != Selection->Previous.Bottom) {

        return TRUE;
    }

    return FALSE;
}

/**
 If the user is holding down the mouse button and trying to select a region
 that is off the screen, this routine is called periodically to move the
 window within the buffer to allow the selection to take place.

 @param Selection Pointer to the selection to update.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriLibPeriodicScrollForSelection(
    __inout PYORILIB_SELECTION Selection
    )
{
    HANDLE ConsoleHandle;
    SHORT CellsToScroll;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
        return FALSE;
    }

    if (Selection->PeriodicScrollAmount.Y < 0) {
        CellsToScroll = (SHORT)(0 - Selection->PeriodicScrollAmount.Y);
        if (ScreenInfo.srWindow.Top > 0) {
            if (ScreenInfo.srWindow.Top > CellsToScroll) {
                ScreenInfo.srWindow.Top = (SHORT)(ScreenInfo.srWindow.Top - CellsToScroll);
                ScreenInfo.srWindow.Bottom = (SHORT)(ScreenInfo.srWindow.Bottom - CellsToScroll);
            } else {
                ScreenInfo.srWindow.Bottom = (SHORT)(ScreenInfo.srWindow.Bottom - ScreenInfo.srWindow.Top);
                ScreenInfo.srWindow.Top = 0;
            }
        }
    } else if (Selection->PeriodicScrollAmount.Y > 0) {
        CellsToScroll = Selection->PeriodicScrollAmount.Y;
        if (ScreenInfo.srWindow.Bottom < ScreenInfo.dwSize.Y - 1) {
            if (ScreenInfo.srWindow.Bottom < ScreenInfo.dwSize.Y - CellsToScroll - 1) {
                ScreenInfo.srWindow.Top = (SHORT)(ScreenInfo.srWindow.Top + CellsToScroll);
                ScreenInfo.srWindow.Bottom = (SHORT)(ScreenInfo.srWindow.Bottom + CellsToScroll);
            } else {
                ScreenInfo.srWindow.Top = (SHORT)(ScreenInfo.srWindow.Top + (ScreenInfo.dwSize.Y - ScreenInfo.srWindow.Bottom - 1));
                ScreenInfo.srWindow.Bottom = (SHORT)(ScreenInfo.dwSize.Y - 1);
            }
        }
    }

    if (Selection->PeriodicScrollAmount.X < 0) {
        CellsToScroll = (SHORT)(0 - Selection->PeriodicScrollAmount.X);
        if (ScreenInfo.srWindow.Left > 0) {
            if (ScreenInfo.srWindow.Left > CellsToScroll) {
                ScreenInfo.srWindow.Left = (SHORT)(ScreenInfo.srWindow.Left - CellsToScroll);
                ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.srWindow.Right - CellsToScroll);
            } else {
                ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.srWindow.Right - ScreenInfo.srWindow.Left);
                ScreenInfo.srWindow.Left = 0;
            }
        }
    } else if (Selection->PeriodicScrollAmount.X > 0) {
        CellsToScroll = Selection->PeriodicScrollAmount.X;
        if (ScreenInfo.srWindow.Right < ScreenInfo.dwSize.X - 1) {
            if (ScreenInfo.srWindow.Right < ScreenInfo.dwSize.X - CellsToScroll - 1) {
                ScreenInfo.srWindow.Left = (SHORT)(ScreenInfo.srWindow.Left + CellsToScroll);
                ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.srWindow.Right + CellsToScroll);
            } else {
                ScreenInfo.srWindow.Left = (SHORT)(ScreenInfo.srWindow.Left + (ScreenInfo.dwSize.X - ScreenInfo.srWindow.Right - 1));
                ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.dwSize.X - 1);
            }
        }
    }

    SetConsoleWindowInfo(ConsoleHandle, TRUE, &ScreenInfo.srWindow);

    return FALSE;
}

/**
 Start a new selection from a given set of coordinates, typically
 corresponding to where the mouse button is pressed.

 @param Selection Pointer to the selection to create.

 @param X The horizontal coordinate specifying the start of the selection.

 @param Y The vertical coordinate specifying the start of the selection.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriLibCreateSelectionFromPoint(
    __inout PYORILIB_SELECTION Selection,
    __in USHORT X,
    __in USHORT Y
    )
{
    BOOL BufferChanged;

    BufferChanged = YoriLibClearSelection(Selection);

    Selection->InitialPoint.X = X;
    Selection->InitialPoint.Y = Y;

    return BufferChanged;
}

/**
 Start a new selection from a given set of coordinates that specify the
 dimensions of the selection.  This is typically done in response to double
 click where the length of the selection is defined when it is created.

 @param Selection Pointer to the selection to create.

 @param StartX The horizontal coordinate specifying the start of the selection.

 @param StartY The vertical coordinate specifying the start of the selection.

 @param EndX The horizontal coordinate specifying the end of the selection.

 @param EndY The vertical coordinate specifying the end of the selection.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriLibCreateSelectionFromRange(
    __inout PYORILIB_SELECTION Selection,
    __in USHORT StartX,
    __in USHORT StartY,
    __in USHORT EndX,
    __in USHORT EndY
    )
{
    YoriLibClearSelection(Selection);

    Selection->Current.Top = StartY;
    Selection->Current.Bottom = EndY;
    Selection->Current.Left = StartX;
    Selection->Current.Right = EndX;

    return TRUE;
}

/**
 Update the selection to include a current point.  This is typically called
 when the mouse button is held down and the mouse is moved to a new location.

 @param Selection Pointer to the selection to update.

 @param X Specifies the horizontal coordinate that should be part of the
        selection.

 @param Y Specifies the vertical coordinate that should be part of the
        selection.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriLibUpdateSelectionToPoint(
    __inout PYORILIB_SELECTION Selection,
    __in USHORT X,
    __in USHORT Y
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE ConsoleHandle;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
        return FALSE;
    }

    if (Selection->InitialPoint.X < X) {
        Selection->Current.Left = Selection->InitialPoint.X;
        Selection->Current.Right = X;
    } else {
        Selection->Current.Left = X;
        Selection->Current.Right = Selection->InitialPoint.X;
    }

    if (Selection->InitialPoint.Y < Y) {
        Selection->Current.Top = Selection->InitialPoint.Y;
        Selection->Current.Bottom = Y;
    } else {
        Selection->Current.Top = Y;
        Selection->Current.Bottom = Selection->InitialPoint.Y;
    }

    //
    //  Assume that the mouse move is inside the window, so periodic
    //  scrolling is off.
    //

    Selection->PeriodicScrollAmount.X = 0;
    Selection->PeriodicScrollAmount.Y = 0;

    //
    //  Check if it's outside the window and the extent of that
    //  distance to see which periodic scrolling may be enabled.
    //

    if (X < ScreenInfo.srWindow.Left) {
        Selection->PeriodicScrollAmount.X = (SHORT)(X - ScreenInfo.srWindow.Left);
    } else if (X > ScreenInfo.srWindow.Right) {
        Selection->PeriodicScrollAmount.X = (SHORT)(X - ScreenInfo.srWindow.Right);
    }

    if (Y < ScreenInfo.srWindow.Top) {
        Selection->PeriodicScrollAmount.Y = (SHORT)(Y - ScreenInfo.srWindow.Top);
    } else if (Y > ScreenInfo.srWindow.Bottom) {
        Selection->PeriodicScrollAmount.Y = (SHORT)(Y - ScreenInfo.srWindow.Bottom);
    }

    //
    //  Do one scroll immediately.  This allows the user to force scrolling
    //  by moving the mouse outside the window.
    //

    if (Selection->PeriodicScrollAmount.X != 0 ||
        Selection->PeriodicScrollAmount.Y != 0) {

        YoriLibPeriodicScrollForSelection(Selection);
    }

    return TRUE;
}

/**
 Return TRUE if the selection should trigger periodic scrolling of the window
 within the console to allow the selection to be extended into an off-screen
 region.

 @param Selection Pointer to the selection to check for periodic scroll.

 @return TRUE if the window should be periodically moved to allow for the
         selection to extend to an off screen region, FALSE if it should not.
 */
BOOL
YoriLibIsPeriodicScrollActive(
    __in PYORILIB_SELECTION Selection
    )
{
    if (Selection->PeriodicScrollAmount.X != 0 ||
        Selection->PeriodicScrollAmount.Y != 0) {

        return TRUE;
    }
    return FALSE;
}

/**
 Indicate that a selection should not trigger periodic scrolling of the
 window within the console to allow the selection to be extended into an
 off-screen region.

 @param Selection Pointer to the selection which should no longer trigger
        periodic scroll.
 */
VOID
YoriLibClearPeriodicScroll(
    __in PYORILIB_SELECTION Selection
    )
{
    Selection->PeriodicScrollAmount.X = 0;
    Selection->PeriodicScrollAmount.Y = 0;
}

/**
 If a selection region is active, copy the region as text to the clipboard.

 @param Selection Pointer to the selection describing the selection region.

 @return TRUE if the region was successfully copied, FALSE if it was not
         copied including if no selection was present.
 */
BOOL
YoriLibCopySelectionIfPresent(
    __in PYORILIB_SELECTION Selection
    )
{
    YORI_STRING TextToCopy;
    YORI_STRING VtText;
    YORI_STRING HtmlText;
    SHORT LineLength;
    SHORT LineCount;
    SHORT LineIndex;
    LPTSTR TextWritePoint;
    COORD StartPoint;
    DWORD CharsWritten;
    HANDLE ConsoleHandle;

    //
    //  No selection, nothing to copy
    //

    if (!YoriLibIsSelectionActive(Selection)) {
        return FALSE;
    }

    //
    //  We want to get the attributes for rich text copy.  Rather than reinvent
    //  that wheel, force the console to re-render if it's stale and use the
    //  saved attribute buffer.
    //

    if (Selection->Current.Left != Selection->Previous.Left ||
        Selection->Current.Right != Selection->Previous.Right ||
        Selection->Current.Top != Selection->Previous.Top ||
        Selection->Current.Bottom != Selection->Previous.Bottom) {

        YoriLibRedrawSelection(Selection);
    }

    //
    //  If there was an allocation failure collecting attributes, stop.
    //

    if (Selection->PreviousBuffer[Selection->CurrentPreviousIndex].AttributeArray == NULL) {
        return FALSE;
    }

    //
    //  Allocate a buffer to hold the text.  Add two chars per line for newlines.
    //

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    LineLength = (SHORT)(Selection->Current.Right - Selection->Current.Left + 1);
    LineCount = (SHORT)(Selection->Current.Bottom - Selection->Current.Top + 1);

    if (!YoriLibAllocateString(&TextToCopy, (LineLength + 2) * LineCount)) {
        return FALSE;
    }

    //
    //  In the first pass, copy all of the text, including trailing spaces.
    //  This version will be used to construct the rich text form.
    //

    TextWritePoint = TextToCopy.StartOfString;
    for (LineIndex = Selection->Current.Top; LineIndex <= Selection->Current.Bottom; LineIndex++) {
        StartPoint.X = Selection->Current.Left;
        StartPoint.Y = LineIndex;

        ReadConsoleOutputCharacter(ConsoleHandle, TextWritePoint, LineLength, StartPoint, &CharsWritten);

        TextWritePoint += LineLength;
    }

    TextToCopy.LengthInChars = (DWORD)(TextWritePoint - TextToCopy.StartOfString);

    StartPoint.X = (SHORT)(Selection->Current.Right - Selection->Current.Left + 1);
    StartPoint.Y = (SHORT)(Selection->Current.Bottom - Selection->Current.Top + 1);

    //
    //  Combine the captured text with previously saved attributes into a
    //  VT100 stream.  This will turn into HTML.
    //

    YoriLibInitEmptyString(&VtText);
    if (!YoriLibGenerateVtStringFromConsoleBuffers(&VtText, StartPoint, TextToCopy.StartOfString, Selection->PreviousBuffer[Selection->CurrentPreviousIndex].AttributeArray)) {
        YoriLibFreeStringContents(&TextToCopy);
        return FALSE;
    }

    //
    //  In the second pass, copy all of the text, truncating trailing spaces.
    //  This version will be used to construct the plain text form.
    //

    TextWritePoint = TextToCopy.StartOfString;
    for (LineIndex = Selection->Current.Top; LineIndex <= Selection->Current.Bottom; LineIndex++) {
        StartPoint.X = Selection->Current.Left;
        StartPoint.Y = LineIndex;

        ReadConsoleOutputCharacter(ConsoleHandle, TextWritePoint, LineLength, StartPoint, &CharsWritten);
        while (CharsWritten > 0) {
            if (TextWritePoint[CharsWritten - 1] != ' ') {
                break;
            }
            CharsWritten--;
        }
        TextWritePoint += CharsWritten;

        TextWritePoint[0] = '\r';
        TextWritePoint++;
        TextWritePoint[0] = '\n';
        TextWritePoint++;
    }

    //
    //  Remove the final CRLF
    //

    if (TextToCopy.LengthInChars >= 2) {
        TextToCopy.LengthInChars -= 2;
    }

    TextToCopy.LengthInChars = (DWORD)(TextWritePoint - TextToCopy.StartOfString);

    //
    //  Convert the VT100 form into HTML, and free it
    //

    YoriLibInitEmptyString(&HtmlText);
    if (!YoriLibHtmlConvertToHtmlFromVt(&VtText, &HtmlText, 4)) {
        YoriLibFreeStringContents(&VtText);
        YoriLibFreeStringContents(&TextToCopy);
        YoriLibFreeStringContents(&HtmlText);
        return FALSE;
    }

    YoriLibFreeStringContents(&VtText);

    //
    //  Copy both HTML form and plain text form to the clipboard
    //

    if (YoriLibCopyTextAndHtml(&TextToCopy, &HtmlText)) {
        YoriLibFreeStringContents(&TextToCopy);
        YoriLibFreeStringContents(&HtmlText);
        return TRUE;
    }

    YoriLibFreeStringContents(&HtmlText);
    YoriLibFreeStringContents(&TextToCopy);
    return FALSE;
}

// vim:sw=4:ts=4:et:
