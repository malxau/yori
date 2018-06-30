/**
 * @file input.c
 *
 * Yori shell command entry from a console
 *
 * Copyright (c) 2017 Malcolm J. Smith
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

#include "yori.h"

/**
 Returns the coordinates in the console if the cursor is moved by a given
 number of cells.  Note the input value is signed, as this routine can move
 forwards (positive values) or backwards (negative values.)

 @param PlacesToMove The number of cells to move relative to the cursor.

 @return The X/Y coordinates of the cell if the cursor was moved the
         specified number of places.
 */
COORD
YoriShDetermineCellLocationIfMoved(
    __in INT PlacesToMove
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    INT PlacesToMoveDown;
    INT PlacesToMoveRight;
    COORD NewPosition;
    HANDLE ConsoleHandle;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // MSFIX Error handling
    GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo);

    PlacesToMoveDown = PlacesToMove / ScreenInfo.dwSize.X;
    PlacesToMoveRight = PlacesToMove % ScreenInfo.dwSize.X;
    if (PlacesToMoveRight > 0) {
        if (PlacesToMoveRight + ScreenInfo.dwCursorPosition.X >= ScreenInfo.dwSize.X) {
            PlacesToMoveRight -= ScreenInfo.dwSize.X;
            PlacesToMoveDown++;
        }
    } else {

        if (PlacesToMoveRight + ScreenInfo.dwCursorPosition.X < 0) {
            PlacesToMoveRight += ScreenInfo.dwSize.X;
            PlacesToMoveDown--;
        }
    }

    NewPosition.Y = (WORD)(ScreenInfo.dwCursorPosition.Y + PlacesToMoveDown);
    NewPosition.X = (WORD)(ScreenInfo.dwCursorPosition.X + PlacesToMoveRight);

    if (NewPosition.Y >= ScreenInfo.dwSize.Y) {
        SMALL_RECT ContentsToPreserve;
        SMALL_RECT ContentsToErase;
        COORD Origin;
        CHAR_INFO NewChar;
        WORD LinesToMove;

        LinesToMove = (WORD)(NewPosition.Y - ScreenInfo.dwSize.Y + 1);

        ContentsToPreserve.Left = 0;
        ContentsToPreserve.Right = (WORD)(ScreenInfo.dwSize.X - 1);
        ContentsToPreserve.Top = LinesToMove;
        ContentsToPreserve.Bottom = (WORD)(ScreenInfo.dwSize.Y - 1);

        ContentsToErase.Left = 0;
        ContentsToErase.Right = (WORD)(ScreenInfo.dwSize.X - 1);
        ContentsToErase.Top = (WORD)(ScreenInfo.dwSize.Y - LinesToMove);
        ContentsToErase.Bottom = (WORD)(ScreenInfo.dwSize.Y - 1);

        Origin.X = 0;
        Origin.Y = 0;

        NewChar.Char.UnicodeChar = ' ';
        NewChar.Attributes = ScreenInfo.wAttributes;
        ScrollConsoleScreenBuffer(ConsoleHandle, &ContentsToPreserve, NULL, Origin, &NewChar);

        ScreenInfo.dwCursorPosition.Y = (WORD)(ScreenInfo.dwCursorPosition.Y - LinesToMove);
        SetConsoleCursorPosition(ConsoleHandle, ScreenInfo.dwCursorPosition);

        NewPosition.Y = (WORD)(NewPosition.Y - LinesToMove);
    }

    return NewPosition;
}

/**
 Determine the offset within the input buffer of specified X,Y coordinates
 relative to the console screen buffer.

 @param Buffer The current input buffer.

 @param TargetCoordinates The coordinates to check against the input buffer.

 @param StringOffset On successful completion, updated to point to the
        location within the string of the coordinates.  Note this can point
        to one past the length of the string.

 @return TRUE to indicate the specified coordinates are within the string
         range, FALSE to indicate the coordinates are not within the string.
 */
BOOL
YoriShStringOffsetFromCoordinates(
    __in PYORI_INPUT_BUFFER Buffer,
    __in COORD TargetCoordinates,
    __out PDWORD StringOffset
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE ConsoleHandle;
    DWORD StartOfString;
    DWORD CursorPosition;
    DWORD TargetPosition;

    UNREFERENCED_PARAMETER(TargetCoordinates);

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
        return FALSE;
    }

    TargetPosition = TargetCoordinates.Y * ScreenInfo.dwSize.X + TargetCoordinates.X;
    CursorPosition = ScreenInfo.dwCursorPosition.Y * ScreenInfo.dwSize.X + ScreenInfo.dwCursorPosition.X;

    if (Buffer->PreviousCurrentOffset > CursorPosition) {
        return FALSE;
    }
    StartOfString = CursorPosition - Buffer->PreviousCurrentOffset;

    if (TargetPosition < StartOfString) {
        return FALSE;
    }

    if (TargetPosition > StartOfString + Buffer->String.LengthInChars) {
        return FALSE;
    }

    *StringOffset = TargetPosition - StartOfString;
    return TRUE;
}

/**
 Move the cursor from its current position.  Note the input value is signed,
 as this routine can move forwards (positive values) or backwards (negative
 values.)

 @param PlacesToMove The number of cells to move the cursor.
 */
VOID
YoriShMoveCursor(
    __in INT PlacesToMove
    )
{
    COORD NewPosition = YoriShDetermineCellLocationIfMoved(PlacesToMove);
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), NewPosition);
}

/**
 After a key has been pressed, capture the current state of the buffer so
 that it is ready to accept transformations as a result of the key
 being pressed.

 @param Buffer Pointer to the input buffer whose state should be captured.
 */
VOID
YoriShPrepareForNextKey(
    __in PYORI_INPUT_BUFFER Buffer
    )
{
    Buffer->PriorTabCount = Buffer->TabContext.TabCount;
}

/**
 Cleanup after processing a key press.

 @param Buffer The current state of the input buffer.
 */
VOID
YoriShPostKeyPress(
    __in PYORI_INPUT_BUFFER Buffer
    )
{
    //
    //  If the number of Tabs hasn't changed, the tab context can be torn
    //  down since the user is not repeatedly pressing Tab.
    //

    if (Buffer->PriorTabCount == Buffer->TabContext.TabCount &&
        Buffer->SuggestionString.LengthInChars == 0) {

        YoriShClearTabCompletionMatches(Buffer);
    }
}

/**
 Returns TRUE if the current selection region is active, and FALSE if it
 is not.

 @param Buffer The input buffer describing the current selection state.

 @return TRUE if a current selection is active, FALSE if it is not.
 */
BOOL
YoriShIsSelectionActive(
    __inout PYORI_INPUT_BUFFER Buffer
    )
{
    if (Buffer->CurrentSelection.Left == Buffer->CurrentSelection.Right &&
        Buffer->CurrentSelection.Top == Buffer->CurrentSelection.Bottom) {

        return FALSE;
    }

    return TRUE;
}

/**
 Returns TRUE if a previous selection region is active, and FALSE if it
 is not.

 @param Buffer The input buffer describing the current selection state.

 @return TRUE if a current selection is active, FALSE if it is not.
 */
BOOL
YoriShIsPreviousSelectionActive(
    __inout PYORI_INPUT_BUFFER Buffer
    )
{
    if (Buffer->PreviousSelection.Left == Buffer->PreviousSelection.Right &&
        Buffer->PreviousSelection.Top == Buffer->PreviousSelection.Bottom) {

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
YoriShDisplayAttributes(
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
YoriShReallocateAttributeArray(
    __in PYORI_SH_PREVIOUS_SELECTION_BUFFER AttributeBuffer,
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

 @param Buffer The input buffer to clear the displayed selection for.
 */
VOID
YoriShClearPreviousSelectionDisplay(
    __in PYORI_INPUT_BUFFER Buffer
    )
{
    SHORT LineIndex;
    SHORT LineLength;
    HANDLE ConsoleHandle;
    COORD StartPoint;
    DWORD CharsWritten;
    PWORD AttributeReadPoint;
    PYORI_SH_PREVIOUS_SELECTION_BUFFER ActiveAttributes;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    //
    //  If there was no previous selection, clearing it is easy
    //

    if (!YoriShIsPreviousSelectionActive(Buffer)) {
        return;
    }

    //
    //  Grab a pointer to the previous selection attributes.  Note the
    //  actual buffer can be NULL if there was an allocation failure
    //  when selecting.  In that case attributes are restored to a
    //  default color.
    //

    ActiveAttributes = &Buffer->PreviousSelectionBuffer[Buffer->CurrentPreviousSelectionIndex];
    AttributeReadPoint = ActiveAttributes->AttributeArray;

    LineLength = (SHORT)(Buffer->PreviousSelection.Right - Buffer->PreviousSelection.Left + 1);

    for (LineIndex = Buffer->PreviousSelection.Top; LineIndex <= Buffer->PreviousSelection.Bottom; LineIndex++) {
        StartPoint.X = Buffer->PreviousSelection.Left;
        StartPoint.Y = LineIndex;

        YoriShDisplayAttributes(ConsoleHandle, AttributeReadPoint, 0x07, LineLength, StartPoint, &CharsWritten);

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
YoriShGetSelectionColor(
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

 @param Buffer The input buffer to display the selection for.
 */
VOID
YoriShDrawCurrentSelectionDisplay(
    __in PYORI_INPUT_BUFFER Buffer
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
    PYORI_SH_PREVIOUS_SELECTION_BUFFER ActiveAttributes;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    //
    //  If there is no current selection, drawing it is easy
    //

    if (!YoriShIsSelectionActive(Buffer)) {

        return;
    }

    RequiredLength = (Buffer->CurrentSelection.Right - Buffer->CurrentSelection.Left + 1) * (Buffer->CurrentSelection.Bottom - Buffer->CurrentSelection.Top + 1);

    ActiveAttributes = &Buffer->PreviousSelectionBuffer[Buffer->CurrentPreviousSelectionIndex];

    if (ActiveAttributes->BufferSize < RequiredLength) {

        //
        //  Allocate more than we strictly need so as to reduce the number of
        //  reallocations
        //

        RequiredLength = RequiredLength * 2;
        YoriShReallocateAttributeArray(ActiveAttributes, RequiredLength);
    }

    //
    //  Note this can be NULL on allocation failure
    //

    AttributeWritePoint = ActiveAttributes->AttributeArray;
    LineLength = (SHORT)(Buffer->CurrentSelection.Right - Buffer->CurrentSelection.Left + 1);

    SelectionColor = YoriShGetSelectionColor(ConsoleHandle);

    for (LineIndex = Buffer->CurrentSelection.Top; LineIndex <= Buffer->CurrentSelection.Bottom; LineIndex++) {
        StartPoint.X = Buffer->CurrentSelection.Left;
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

 @param Buffer The input buffer to display the selection for.
 */
VOID
YoriShDrawCurrentSelectionOverPreviousSelection(
    __in PYORI_INPUT_BUFFER Buffer
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
    PYORI_SH_PREVIOUS_SELECTION_BUFFER NewAttributes;
    PYORI_SH_PREVIOUS_SELECTION_BUFFER OldAttributes;
    DWORD NewAttributeIndex;
    WORD SelectionColor;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    ASSERT(YoriShIsPreviousSelectionActive(Buffer) && YoriShIsSelectionActive(Buffer));

    //
    //  Find the buffers that are not the ones that currently contain the
    //  attributes of selected cells.  We'll fill that buffer with updated
    //  information, typically drawn from the currently active buffer.
    //

    NewAttributeIndex = (Buffer->CurrentPreviousSelectionIndex + 1) % 2;
    NewAttributes = &Buffer->PreviousSelectionBuffer[NewAttributeIndex];
    OldAttributes = &Buffer->PreviousSelectionBuffer[Buffer->CurrentPreviousSelectionIndex];

    RequiredLength = (Buffer->CurrentSelection.Right - Buffer->CurrentSelection.Left + 1) * (Buffer->CurrentSelection.Bottom - Buffer->CurrentSelection.Top + 1);

    if (RequiredLength > NewAttributes->BufferSize) {
        RequiredLength = RequiredLength * 2;
        YoriShReallocateAttributeArray(NewAttributes, RequiredLength);
    }

    LineLength = (SHORT)(Buffer->CurrentSelection.Right - Buffer->CurrentSelection.Left + 1);

    //
    //  Walk through all of the new selection to save off attributes for it,
    //  and update the console to have selection color
    //

    SelectionColor = YoriShGetSelectionColor(ConsoleHandle);
    AttributeWritePoint = NewAttributes->AttributeArray;
    for (LineIndex = Buffer->CurrentSelection.Top; LineIndex <= Buffer->CurrentSelection.Bottom; LineIndex++) {

        //
        //  An entire line wasn't previously selected
        //

        if (LineIndex < Buffer->PreviousSelection.Top ||
            LineIndex > Buffer->PreviousSelection.Bottom) {

            StartPoint.X = Buffer->CurrentSelection.Left;
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

            if (Buffer->CurrentSelection.Left < Buffer->PreviousSelection.Left) {

                StartPoint.X = Buffer->CurrentSelection.Left;
                StartPoint.Y = LineIndex;

                RunLength = (SHORT)(Buffer->PreviousSelection.Left - Buffer->CurrentSelection.Left);
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

            if (Buffer->CurrentSelection.Right >= Buffer->PreviousSelection.Left &&
                Buffer->CurrentSelection.Left <= Buffer->PreviousSelection.Right) {

                StartPoint.X = (SHORT)(Buffer->CurrentSelection.Left - Buffer->PreviousSelection.Left);
                RunLength = LineLength;
                if (StartPoint.X < 0) {
                    RunLength = (SHORT)(RunLength + StartPoint.X);
                    StartPoint.X = 0;
                }
                if (StartPoint.X + RunLength > Buffer->PreviousSelection.Right - Buffer->PreviousSelection.Left + 1) {
                    RunLength = (SHORT)(Buffer->PreviousSelection.Right - Buffer->PreviousSelection.Left + 1 - StartPoint.X);
                }

                StartPoint.Y = (SHORT)(LineIndex - Buffer->PreviousSelection.Top);

                BufferOffset = (Buffer->PreviousSelection.Right - Buffer->PreviousSelection.Left + 1) * StartPoint.Y + StartPoint.X;

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

            if (Buffer->CurrentSelection.Right > Buffer->PreviousSelection.Right) {

                StartPoint.X = (SHORT)(Buffer->PreviousSelection.Right + 1);
                if (Buffer->CurrentSelection.Left > StartPoint.X) {
                    StartPoint.X = Buffer->CurrentSelection.Left;
                    RunLength = LineLength;
                } else {
                    RunLength = (SHORT)(Buffer->CurrentSelection.Right - StartPoint.X + 1);
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

    LineLength = (SHORT)(Buffer->PreviousSelection.Right - Buffer->PreviousSelection.Left + 1);

    for (LineIndex = Buffer->PreviousSelection.Top; LineIndex <= Buffer->PreviousSelection.Bottom; LineIndex++) {

        //
        //  A line was previously selected and no longer is.  Restore the
        //  saved attributes.
        //

        if (LineIndex < Buffer->CurrentSelection.Top ||
            LineIndex > Buffer->CurrentSelection.Bottom) {

            StartPoint.X = Buffer->PreviousSelection.Left;
            StartPoint.Y = LineIndex;
            RunLength = LineLength;

            if (OldAttributes->AttributeArray != NULL) {
                BufferOffset = LineLength * (LineIndex - Buffer->PreviousSelection.Top);
                BufferPointer = &OldAttributes->AttributeArray[BufferOffset];
            } else {
                BufferPointer = NULL;
            }

            YoriShDisplayAttributes(ConsoleHandle, BufferPointer, 0x07, RunLength, StartPoint, &CharsWritten);

        } else {

            //
            //  A region to the left of the currently selected region was
            //  previously selected.  Restore the saved attributes.
            //

            if (Buffer->PreviousSelection.Left < Buffer->CurrentSelection.Left) {

                StartPoint.X = Buffer->PreviousSelection.Left;
                StartPoint.Y = LineIndex;
                RunLength = LineLength;
                if (Buffer->CurrentSelection.Left - Buffer->PreviousSelection.Left < RunLength) {
                    RunLength = (SHORT)(Buffer->CurrentSelection.Left - Buffer->PreviousSelection.Left);
                }

                if (OldAttributes->AttributeArray != NULL) {
                    BufferOffset = LineLength * (LineIndex - Buffer->PreviousSelection.Top);
                    BufferPointer = &OldAttributes->AttributeArray[BufferOffset];
                } else {
                    BufferPointer = NULL;
                }

                YoriShDisplayAttributes(ConsoleHandle, BufferPointer, 0x07, RunLength, StartPoint, &CharsWritten);

            }

            //
            //  A region to the right of the current selection was previously
            //  selected.  Restore the saved attributes.
            //

            if (Buffer->PreviousSelection.Right > Buffer->CurrentSelection.Right) {

                BufferOffset = LineLength * (LineIndex - Buffer->PreviousSelection.Top);
                StartPoint.Y = LineIndex;

                if (Buffer->PreviousSelection.Left > Buffer->CurrentSelection.Right) {
                    StartPoint.X = Buffer->PreviousSelection.Left;
                    RunLength = LineLength;
                } else {
                    RunLength = (SHORT)(Buffer->PreviousSelection.Right - Buffer->CurrentSelection.Right);
                    StartPoint.X = (SHORT)(Buffer->CurrentSelection.Right + 1);
                    BufferOffset += Buffer->CurrentSelection.Right - Buffer->PreviousSelection.Left + 1;
                }

                if (OldAttributes->AttributeArray != NULL) {
                    BufferPointer = &OldAttributes->AttributeArray[BufferOffset];
                } else {
                    BufferPointer = NULL;
                }

                YoriShDisplayAttributes(ConsoleHandle, BufferPointer, 0x07, RunLength, StartPoint, &CharsWritten);
            }
        }
    }

    ASSERT(Buffer->CurrentPreviousSelectionIndex != NewAttributeIndex);
    Buffer->CurrentPreviousSelectionIndex = NewAttributeIndex;
}

/**
 After a key has been pressed and processed, display the resulting buffer.

 @param Buffer Pointer to the input buffer to display.
 */
VOID
YoriShDisplayAfterKeyPress(
    __in PYORI_INPUT_BUFFER Buffer
    )
{
    DWORD NumberWritten;
    DWORD NumberToWrite = 0;
    DWORD NumberToFill = 0;
    COORD WritePosition;
    COORD FillPosition;
    COORD SuggestionPosition;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE hConsole;

    WritePosition.X = 0;
    WritePosition.Y = 0;
    SuggestionPosition.X = 0;
    SuggestionPosition.Y = 0;
    FillPosition.X = 0;
    FillPosition.Y = 0;

    if (YoriShIsPreviousSelectionActive(Buffer) && YoriShIsSelectionActive(Buffer)) {
        YoriShDrawCurrentSelectionOverPreviousSelection(Buffer);
    } else {
        YoriShClearPreviousSelectionDisplay(Buffer);
        YoriShDrawCurrentSelectionDisplay(Buffer);
    }
    Buffer->PreviousSelection.Left = Buffer->CurrentSelection.Left;
    Buffer->PreviousSelection.Top = Buffer->CurrentSelection.Top;
    Buffer->PreviousSelection.Right = Buffer->CurrentSelection.Right;
    Buffer->PreviousSelection.Bottom = Buffer->CurrentSelection.Bottom;

    //
    //  Re-render the text if part of the input string has changed,
    //  or if the location of the cursor in the input string has changed
    //

    if (Buffer->DirtyBeginOffset != 0 || Buffer->DirtyLength != 0 ||
        Buffer->SuggestionDirty ||
        Buffer->PreviousCurrentOffset != Buffer->CurrentOffset) {

        // MSFIX Error handling
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(hConsole, &ScreenInfo);

        //
        //  Calculate the number of characters truncated from the currently
        //  displayed buffer.
        //

        if (Buffer->PreviousCharsDisplayed > Buffer->String.LengthInChars + Buffer->SuggestionString.LengthInChars) {
            NumberToFill = Buffer->PreviousCharsDisplayed - Buffer->String.LengthInChars - Buffer->SuggestionString.LengthInChars;
        }

        //
        //  Calculate the locations to write both the new text as well as where
        //  to erase any previous test.
        //
        //  Calculate where the buffer will end and discard the result; this is
        //  done to ensure the screen buffer is scrolled so the whole output
        //  has somewhere to go.
        //

        if (Buffer->DirtyBeginOffset < Buffer->String.LengthInChars && Buffer->DirtyLength > 0) {
            if (Buffer->DirtyBeginOffset + Buffer->DirtyLength > Buffer->String.LengthInChars) {
                NumberToWrite = Buffer->String.LengthInChars - Buffer->DirtyBeginOffset;
            } else {
                NumberToWrite = Buffer->DirtyLength;
            }
            YoriShDetermineCellLocationIfMoved((-1 * Buffer->PreviousCurrentOffset) + Buffer->DirtyBeginOffset + NumberToWrite);
            WritePosition = YoriShDetermineCellLocationIfMoved((-1 * Buffer->PreviousCurrentOffset) + Buffer->DirtyBeginOffset);
        }

        if (Buffer->SuggestionString.LengthInChars > 0) {
            YoriShDetermineCellLocationIfMoved(-1 * Buffer->PreviousCurrentOffset + Buffer->String.LengthInChars + Buffer->SuggestionString.LengthInChars);
            SuggestionPosition = YoriShDetermineCellLocationIfMoved(-1 * Buffer->PreviousCurrentOffset + Buffer->String.LengthInChars);
        }

        if (NumberToFill) {
            YoriShDetermineCellLocationIfMoved(-1 * Buffer->PreviousCurrentOffset + Buffer->String.LengthInChars + Buffer->SuggestionString.LengthInChars + NumberToFill);
            FillPosition = YoriShDetermineCellLocationIfMoved(-1 * Buffer->PreviousCurrentOffset + Buffer->String.LengthInChars + Buffer->SuggestionString.LengthInChars);
        }

        //
        //  Now that we know where the text should go, advance the cursor
        //  and render the text.
        //

        YoriShMoveCursor(Buffer->CurrentOffset - Buffer->PreviousCurrentOffset);

        if (NumberToWrite) {
            WriteConsoleOutputCharacter(hConsole, &Buffer->String.StartOfString[Buffer->DirtyBeginOffset], NumberToWrite, WritePosition, &NumberWritten);
            FillConsoleOutputAttribute(hConsole, ScreenInfo.wAttributes, NumberToWrite, WritePosition, &NumberWritten);
        }

        if (Buffer->SuggestionString.LengthInChars > 0) {
            WriteConsoleOutputCharacter(hConsole, Buffer->SuggestionString.StartOfString, Buffer->SuggestionString.LengthInChars, SuggestionPosition, &NumberWritten);
            FillConsoleOutputAttribute(hConsole, (USHORT)((ScreenInfo.wAttributes & 0xF0) | FOREGROUND_INTENSITY), Buffer->SuggestionString.LengthInChars, SuggestionPosition, &NumberWritten);
        }

        //
        //  If there are additional cells to empty due to truncation, display
        //  those now.
        //

        if (NumberToFill) {
            FillConsoleOutputCharacter(hConsole, ' ', NumberToFill, FillPosition, &NumberWritten);
            FillConsoleOutputAttribute(hConsole, ScreenInfo.wAttributes, NumberToFill, FillPosition, &NumberWritten);
        }

        Buffer->PreviousCurrentOffset = Buffer->CurrentOffset;
        Buffer->PreviousCharsDisplayed = Buffer->String.LengthInChars + Buffer->SuggestionString.LengthInChars;
        Buffer->DirtyBeginOffset = 0;
        Buffer->DirtyLength = 0;
        Buffer->SuggestionDirty = FALSE;
    }
}

/**
 Check that the string has enough characters to hold the new number of
 characters including a NULL terminator.  If it doesn't, reallocate a
 new buffer that is large enough to hold the new number of characters.
 Note that since this is an allocation it can fail.

 @param String Pointer to the current string.

 @param CharactersNeeded The number of characters that are needed in
        the buffer.

 @return TRUE to indicate the current buffer is large enough or it was
         successfully reallocated, FALSE to indicate allocation failure.
 */
BOOL
YoriShEnsureStringHasEnoughCharacters(
    __inout PYORI_STRING String,
    __in DWORD CharactersNeeded
    )
{
    while (CharactersNeeded + 1 >= String->LengthAllocated) {
        if (!YoriLibReallocateString(String, String->LengthAllocated * 4)) {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 NULL terminate the input buffer, and display a carriage return, in preparation
 for parsing and executing the input.

 @param Buffer Pointer to the input buffer.
 */
VOID
YoriShTerminateInput(
    __inout PYORI_INPUT_BUFFER Buffer
    )
{
    DWORD Index;
    YoriShDisplayAfterKeyPress(Buffer);
    YoriShPostKeyPress(Buffer);
    YoriLibFreeStringContents(&Buffer->SuggestionString);
    YoriShClearTabCompletionMatches(Buffer);
    for (Index = 0; Index < 2; Index++) {
        if (Buffer->PreviousSelectionBuffer[Index].AttributeArray) {
            YoriLibFree(Buffer->PreviousSelectionBuffer[Index].AttributeArray);
            Buffer->PreviousSelectionBuffer[Index].AttributeArray = NULL;
            Buffer->PreviousSelectionBuffer[Index].BufferSize = 0;
        }
    }
    Buffer->String.StartOfString[Buffer->String.LengthInChars] = '\0';
    YoriShMoveCursor(Buffer->String.LengthInChars - Buffer->CurrentOffset);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
}


/**
 Clear any current selection.  Note this is clearing in memory state and it
 will not be re-rendered on the screen until that action is requested.

 @param Buffer Pointer to the input buffer describing the selection.

 @return TRUE to indicate a selection was cleared and the buffer requires
         redrawing; FALSE if no redrawing is required.
 */
BOOL
YoriShClearSelection(
    __inout PYORI_INPUT_BUFFER Buffer
    )
{
    Buffer->CurrentSelection.Left = 0;
    Buffer->CurrentSelection.Right = 0;
    Buffer->CurrentSelection.Top = 0;
    Buffer->CurrentSelection.Bottom = 0;

    Buffer->PeriodicScrollAmount.X = 0;
    Buffer->PeriodicScrollAmount.Y = 0;

    if (Buffer->CurrentSelection.Left != Buffer->PreviousSelection.Left ||
        Buffer->CurrentSelection.Right != Buffer->PreviousSelection.Right ||
        Buffer->CurrentSelection.Top != Buffer->PreviousSelection.Top ||
        Buffer->CurrentSelection.Bottom != Buffer->PreviousSelection.Bottom) {

        return TRUE;
    }

    return FALSE;
}

/**
 Empty the current input buffer.

 @param Buffer Pointer to the buffer to empty.
 */
VOID
YoriShClearInput(
    __inout PYORI_INPUT_BUFFER Buffer
    )
{
    YoriLibFreeStringContents(&Buffer->SuggestionString);
    YoriShClearTabCompletionMatches(Buffer);
    Buffer->String.LengthInChars = 0;
    Buffer->CurrentOffset = 0;
    YoriShClearSelection(Buffer);
}

/**
 Perform the necessary buffer transformations to implement backspace.

 @param Buffer Pointer to the buffer to apply backspace to.

 @param Count The number of backspace operations to apply.
 */
VOID
YoriShBackspace(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in DWORD Count
    )
{
    DWORD CountToUse;

    CountToUse = Count;

    if (Buffer->CurrentOffset < CountToUse) {

        CountToUse = Buffer->CurrentOffset;
    }

    if (Buffer->CurrentOffset != Buffer->String.LengthInChars) {
        memmove(&Buffer->String.StartOfString[Buffer->CurrentOffset - CountToUse],
                &Buffer->String.StartOfString[Buffer->CurrentOffset],
                (Buffer->String.LengthInChars - Buffer->CurrentOffset) * sizeof(TCHAR));
    }

    if (Buffer->DirtyLength == 0) {
        Buffer->DirtyBeginOffset = Buffer->CurrentOffset - CountToUse;
        Buffer->DirtyLength = Buffer->String.LengthInChars - Buffer->DirtyBeginOffset;
    } else {
        if (Buffer->CurrentOffset - CountToUse < Buffer->DirtyBeginOffset) {
            Buffer->DirtyLength += Buffer->DirtyBeginOffset - (Buffer->CurrentOffset - CountToUse);
            Buffer->DirtyBeginOffset = Buffer->CurrentOffset - CountToUse;
        }
        if (Buffer->DirtyBeginOffset + Buffer->DirtyLength < Buffer->String.LengthInChars) {
            Buffer->DirtyLength = Buffer->String.LengthInChars - Buffer->DirtyBeginOffset;
        }
    }

    Buffer->CurrentOffset -= CountToUse;
    Buffer->String.LengthInChars -= CountToUse;

    Buffer->SuggestionDirty = TRUE;
    YoriLibFreeStringContents(&Buffer->SuggestionString);
}

/**
 If a selection region is active and covers the input string, delete the
 selected range of the input string and leave the cursor at the point where
 the selection was to allow for a subsequent insert.

 @param Buffer Pointer to the input buffer describing the selection region.

 @return TRUE if the region was successfully removed; FALSE if no selection
         is active or the selection did not apply to the command input string.
 */
BOOL
YoriShOverwriteSelectionIfInInput(
    __inout PYORI_INPUT_BUFFER Buffer
    )
{

    DWORD StartStringOffset;
    DWORD EndStringOffset;
    DWORD Length;
    COORD StartOfSelection;

    //
    //  No selection, nothing to overwrite
    //

    if (!YoriShIsSelectionActive(Buffer)) {
        return FALSE;
    }

    //
    //  Currently only support operating on one line at a time, to avoid
    //  trying to define the screwy behavior of multiple discontiguous
    //  ranges.
    //

    if (Buffer->CurrentSelection.Bottom != Buffer->CurrentSelection.Top) {
        return FALSE;
    }

    StartOfSelection.X = Buffer->CurrentSelection.Left;
    StartOfSelection.Y = Buffer->CurrentSelection.Top;

    if (!YoriShStringOffsetFromCoordinates(Buffer, StartOfSelection, &StartStringOffset)) {
        return FALSE;
    }

    StartOfSelection.X = Buffer->CurrentSelection.Right;

    if (!YoriShStringOffsetFromCoordinates(Buffer, StartOfSelection, &EndStringOffset)) {
        return FALSE;
    }

    Length = EndStringOffset - StartStringOffset + 1;

    if (StartStringOffset + Length > Buffer->String.LengthInChars) {
        if (StartStringOffset > Buffer->String.LengthInChars) {
            return FALSE;
        }
        Length = Buffer->String.LengthInChars - StartStringOffset;
    }

    Buffer->CurrentOffset = StartStringOffset + Length;
    YoriShBackspace(Buffer, Length);
    return TRUE;
}

/**
 If a selection region is active, copy the region as text to the clipboard.

 @param Buffer Pointer to the input buffer describing the selection region.

 @return TRUE if the region was successfully copied, FALSE if it was not
         copied including if no selection was present.
 */
BOOL
YoriShCopySelectionIfPresent(
    __in PYORI_INPUT_BUFFER Buffer
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

    if (!YoriShIsSelectionActive(Buffer)) {
        return FALSE;
    }

    //
    //  We want to get the attributes for rich text copy.  Rather than reinvent
    //  that wheel, force the console to re-render if it's stale and use the
    //  saved attribute buffer.
    //
    
    if (Buffer->CurrentSelection.Left != Buffer->PreviousSelection.Left ||
        Buffer->CurrentSelection.Right != Buffer->PreviousSelection.Right ||
        Buffer->CurrentSelection.Top != Buffer->PreviousSelection.Top ||
        Buffer->CurrentSelection.Bottom != Buffer->PreviousSelection.Bottom) {

        YoriShDisplayAfterKeyPress(Buffer);
    }

    //
    //  If there was an allocation failure collecting attributes, stop.
    //

    if (Buffer->PreviousSelectionBuffer[Buffer->CurrentPreviousSelectionIndex].AttributeArray == NULL) {
        return FALSE;
    }

    //
    //  Allocate a buffer to hold the text.  Add two chars per line for newlines.
    //

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    LineLength = (SHORT)(Buffer->CurrentSelection.Right - Buffer->CurrentSelection.Left + 1);
    LineCount = (SHORT)(Buffer->CurrentSelection.Bottom - Buffer->CurrentSelection.Top + 1);

    if (!YoriLibAllocateString(&TextToCopy, (LineLength + 2) * LineCount)) {
        return FALSE;
    }

    //
    //  In the first pass, copy all of the text, including trailing spaces.
    //  This version will be used to construct the rich text form.
    //

    TextWritePoint = TextToCopy.StartOfString;
    for (LineIndex = Buffer->CurrentSelection.Top; LineIndex <= Buffer->CurrentSelection.Bottom; LineIndex++) {
        StartPoint.X = Buffer->CurrentSelection.Left;
        StartPoint.Y = LineIndex;

        ReadConsoleOutputCharacter(ConsoleHandle, TextWritePoint, LineLength, StartPoint, &CharsWritten);

        TextWritePoint += LineLength;
    }

    TextToCopy.LengthInChars = (DWORD)(TextWritePoint - TextToCopy.StartOfString);

    StartPoint.X = (SHORT)(Buffer->CurrentSelection.Right - Buffer->CurrentSelection.Left + 1);
    StartPoint.Y = (SHORT)(Buffer->CurrentSelection.Bottom - Buffer->CurrentSelection.Top + 1);

    //
    //  Combine the captured text with previously saved attributes into a
    //  VT100 stream.  This will turn into HTML.
    //

    YoriLibInitEmptyString(&VtText);
    if (!YoriLibGenerateVtStringFromConsoleBuffers(&VtText, StartPoint, TextToCopy.StartOfString, Buffer->PreviousSelectionBuffer[Buffer->CurrentPreviousSelectionIndex].AttributeArray)) {
        YoriLibFreeStringContents(&TextToCopy);
        return FALSE;
    }

    //
    //  In the second pass, copy all of the text, truncating trailing spaces.
    //  This version will be used to construct the plain text form.
    //

    TextWritePoint = TextToCopy.StartOfString;
    for (LineIndex = Buffer->CurrentSelection.Top; LineIndex <= Buffer->CurrentSelection.Bottom; LineIndex++) {
        StartPoint.X = Buffer->CurrentSelection.Left;
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

    if (YoriShCopyTextAndHtml(&TextToCopy, &HtmlText)) {
        YoriLibFreeStringContents(&TextToCopy);
        YoriLibFreeStringContents(&HtmlText);
        return TRUE;
    }

    YoriLibFreeStringContents(&HtmlText);
    YoriLibFreeStringContents(&TextToCopy);
    return FALSE;
}

/**
 Apply incoming characters to an input buffer.

 @param Buffer The input buffer to apply new characters to.

 @param String A yori string to enter into the buffer.
 */
VOID
YoriShAddYoriStringToInput(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in PYORI_STRING String
    )
{
    BOOL KeepSuggestions;

    //
    //  Need more allocated than populated due to NULL termination
    //

    KeepSuggestions = FALSE;
    YoriShOverwriteSelectionIfInInput(Buffer);
    ASSERT(Buffer->String.LengthAllocated > Buffer->String.LengthInChars);
    ASSERT(Buffer->String.LengthInChars >= Buffer->CurrentOffset);

    //
    //  If the characters are at the end of the string, see if a
    //  current suggestion can be retained.
    //

    if (Buffer->String.LengthInChars == Buffer->CurrentOffset) {
        KeepSuggestions = TRUE;
    }

    if (!KeepSuggestions) {
        YoriLibFreeStringContents(&Buffer->SuggestionString);
        YoriShClearTabCompletionMatches(Buffer);
    } else {
        YoriShTrimSuggestionList(Buffer, String);
    }
    Buffer->SuggestionDirty = TRUE;

    //
    //  If we're inserting, shuffle the data; if we're overwriting, clobber
    //  the data.
    //

    if (Buffer->InsertMode) {
        if (!YoriShEnsureStringHasEnoughCharacters(&Buffer->String, Buffer->String.LengthInChars + String->LengthInChars)) {
            return;
        }

        if (Buffer->String.LengthInChars != Buffer->CurrentOffset) {
            memmove(&Buffer->String.StartOfString[Buffer->CurrentOffset + String->LengthInChars],
                    &Buffer->String.StartOfString[Buffer->CurrentOffset],
                    (Buffer->String.LengthInChars - Buffer->CurrentOffset) * sizeof(TCHAR));
        }
        Buffer->String.LengthInChars += String->LengthInChars;
        memcpy(&Buffer->String.StartOfString[Buffer->CurrentOffset], String->StartOfString, String->LengthInChars * sizeof(TCHAR));

        if (Buffer->DirtyLength == 0) {
            Buffer->DirtyBeginOffset = Buffer->CurrentOffset;
            Buffer->DirtyLength = Buffer->String.LengthInChars - Buffer->CurrentOffset;
        } else {
            if (Buffer->CurrentOffset < Buffer->DirtyBeginOffset) {
                Buffer->DirtyLength += Buffer->DirtyBeginOffset - Buffer->CurrentOffset;
                Buffer->DirtyBeginOffset = Buffer->CurrentOffset;
            }
            if (Buffer->DirtyBeginOffset + Buffer->DirtyLength < Buffer->String.LengthInChars) {
                Buffer->DirtyLength = Buffer->String.LengthInChars - Buffer->DirtyBeginOffset;
            }
        }
        Buffer->CurrentOffset += String->LengthInChars;
    } else {
        if (!YoriShEnsureStringHasEnoughCharacters(&Buffer->String, Buffer->CurrentOffset + String->LengthInChars)) {
            return;
        }
        memcpy(&Buffer->String.StartOfString[Buffer->CurrentOffset], String->StartOfString, String->LengthInChars * sizeof(TCHAR));
        Buffer->CurrentOffset += String->LengthInChars;
        if (Buffer->CurrentOffset > Buffer->String.LengthInChars) {
            Buffer->String.LengthInChars = Buffer->CurrentOffset;
        }
        if (Buffer->DirtyLength == 0) {
            Buffer->DirtyBeginOffset = Buffer->CurrentOffset - String->LengthInChars;
            Buffer->DirtyLength = String->LengthInChars;
        } else {
            if (Buffer->CurrentOffset - String->LengthInChars < Buffer->DirtyBeginOffset) {
                Buffer->DirtyLength += Buffer->DirtyBeginOffset - (Buffer->CurrentOffset - String->LengthInChars);
                Buffer->DirtyBeginOffset = Buffer->CurrentOffset - String->LengthInChars;
            }
            if (Buffer->DirtyBeginOffset + Buffer->DirtyLength < Buffer->CurrentOffset) {
                Buffer->DirtyLength = Buffer->CurrentOffset - Buffer->DirtyBeginOffset;
            }
        }
    }

    ASSERT(Buffer->String.LengthAllocated > Buffer->String.LengthInChars);
    ASSERT(Buffer->String.LengthInChars >= Buffer->CurrentOffset);
}

/**
 Add a NULL terminated string to the input buffer.  This could be an
 append, an insert in the middle, or an overwrite.

 @param Buffer The input buffer context.

 @param String the NULL terminated string to append.
 */
VOID
YoriShAddCStringToInput(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in LPTSTR String
    )
{
    YORI_STRING YoriString;

    YoriLibConstantString(&YoriString, String);
    YoriShAddYoriStringToInput(Buffer, &YoriString);
}



/**
 Move the current cursor offset within the buffer to the argument before the
 one that is selected.  This requires parsing the arguments and moving the
 current offset into the last one.  This is used to implement Ctrl+Left
 functionality.  On error, the offset is not updated.

 @param Buffer Pointer to the current input buffer context.
 */
VOID
YoriShMoveCursorToPriorArgument(
    __in PYORI_INPUT_BUFFER Buffer
    )
{
    YORI_CMD_CONTEXT CmdContext;
    LPTSTR NewString = NULL;
    DWORD BeginCurrentArg = 0;
    DWORD EndCurrentArg = 0;
    DWORD NewStringLen;

    if (!YoriShParseCmdlineToCmdContext(&Buffer->String, Buffer->CurrentOffset, &CmdContext)) {
        return;
    }

    if (CmdContext.ArgC == 0) {
        YoriShFreeCmdContext(&CmdContext);
        return;
    }

    if (CmdContext.CurrentArg > 0) {

        //
        //  If we're on the final argument but not at the first letter, go to
        //  the beginning of the current argument.
        //
        //  MSFIX This is relying on the current string offset being compared
        //  against the new string offset, which is not guaranteed to be
        //  correct.  Doing this properly requires parser support to indicate
        //  "current position within argument."
        //

        if (CmdContext.CurrentArg < CmdContext.ArgC) {
            NewString = YoriShBuildCmdlineFromCmdContext(&CmdContext, FALSE, &BeginCurrentArg, &EndCurrentArg);
            if (Buffer->CurrentOffset <= BeginCurrentArg) {
                YoriLibDereference(NewString);
                NewString = NULL;
                CmdContext.CurrentArg--;
            }
        } else {
            CmdContext.CurrentArg--;
        }
    }

    if (NewString == NULL) {
        NewString = YoriShBuildCmdlineFromCmdContext(&CmdContext, FALSE, &BeginCurrentArg, &EndCurrentArg);
    }

    if (NewString != NULL) {
        NewStringLen = _tcslen(NewString);
        if (!YoriShEnsureStringHasEnoughCharacters(&Buffer->String, NewStringLen)) {
            return;
        }
        YoriLibYPrintf(&Buffer->String, _T("%s"), NewString);
        Buffer->CurrentOffset = BeginCurrentArg;
        if (Buffer->CurrentOffset > Buffer->String.LengthInChars) {
            Buffer->CurrentOffset = Buffer->String.LengthInChars;
        }
        YoriLibDereference(NewString);
    }

    YoriShFreeCmdContext(&CmdContext);
}

/**
 Move the current cursor offset within the buffer to the argument following the
 one that is selected.  This requires parsing the arguments and moving the
 current offset into the next one.  This is used to implement Ctrl+Right
 functionality.  On error, the offset is not updated.

 @param Buffer Pointer to the current input buffer context.
 */
VOID
YoriShMoveCursorToNextArgument(
    __in PYORI_INPUT_BUFFER Buffer
    )
{
    YORI_CMD_CONTEXT CmdContext;
    LPTSTR NewString;
    DWORD BeginCurrentArg;
    DWORD EndCurrentArg;
    BOOL MoveToEnd = FALSE;

    if (!YoriShParseCmdlineToCmdContext(&Buffer->String, Buffer->CurrentOffset, &CmdContext)) {
        return;
    }

    if (CmdContext.ArgC == 0) {
        YoriShFreeCmdContext(&CmdContext);
        return;
    }

    if (CmdContext.CurrentArg + 1 < (DWORD)CmdContext.ArgC) {
        CmdContext.CurrentArg++;
    } else {
        MoveToEnd = TRUE;
    }

    NewString = YoriShBuildCmdlineFromCmdContext(&CmdContext, FALSE, &BeginCurrentArg, &EndCurrentArg);

    if (NewString != NULL) {
        DWORD NewStringLen;
        NewStringLen = _tcslen(NewString);
        if (!YoriShEnsureStringHasEnoughCharacters(&Buffer->String, NewStringLen)) {
            return;
        }
        YoriLibYPrintf(&Buffer->String, _T("%s"), NewString);
        if (MoveToEnd) {
            Buffer->CurrentOffset = Buffer->String.LengthInChars;
        } else {
            Buffer->CurrentOffset = BeginCurrentArg;
        }
        if (Buffer->CurrentOffset > Buffer->String.LengthInChars) {
            Buffer->CurrentOffset = Buffer->String.LengthInChars;
        }
        YoriLibDereference(NewString);
    }

    YoriShFreeCmdContext(&CmdContext);
}

/**
 Check if an alias mapping exists for a given hotkey, and if so populate the
 input buffer with the result of that alias and return TRUE indicating that
 it should be executed.

 @param Buffer The input buffer which may be populated with an expression to
        execute.

 @param KeyCode The virtual key code corresponding to the hotkey.

 @param CtrlMask The key modifiers that were held down when the key was
        pressed.

 @return TRUE to indicate the input buffer has been populated with a command
         to execute, FALSE if it was not.
 */
BOOL
YoriShHotkey(
    __in PYORI_INPUT_BUFFER Buffer,
    __in WORD KeyCode,
    __in DWORD CtrlMask
    )
{
    BOOL CtrlPressed = FALSE;
    DWORD FunctionIndex;
    TCHAR NewStringBuffer[32];
    YORI_STRING NewString;
    YORI_CMD_CONTEXT CmdContext;
    LPTSTR CmdLine;

    if (CtrlMask & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
        return FALSE;
    }

    if (CtrlMask & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
        CtrlPressed = TRUE;
    }

    FunctionIndex = KeyCode - VK_F1 + 1;

    YoriLibInitEmptyString(&NewString);
    NewString.StartOfString = NewStringBuffer;
    NewString.LengthAllocated = sizeof(NewStringBuffer)/sizeof(NewStringBuffer[0]);

    NewString.LengthInChars = YoriLibSPrintf(NewString.StartOfString, _T("%sF%i"), CtrlPressed?_T("Ctrl"):_T(""), FunctionIndex);

    if (!YoriShParseCmdlineToCmdContext(&NewString, 0, &CmdContext)) {
        return FALSE;
    }

    if (CmdContext.ArgC == 0) {
        YoriShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (!YoriShExpandAlias(&CmdContext)) {
        YoriShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    CmdLine = YoriShBuildCmdlineFromCmdContext(&CmdContext, FALSE, NULL, NULL);
    YoriShFreeCmdContext(&CmdContext);
    if (CmdLine == NULL) {
        return FALSE;
    }

    YoriShClearInput(Buffer);
    YoriShAddCStringToInput(Buffer, CmdLine);
    YoriLibDereference(CmdLine);
    return TRUE;
}

/**
 Check the environment to see if the user wants to customize suggestion
 settings.

 @param Buffer Pointer to the input buffer structure to update in response
        to values found in the environment.
 */
VOID
YoriShConfigureSuggestionSettings(
    __in PYORI_INPUT_BUFFER Buffer
    )
{
    DWORD EnvVarLength;
    YORI_STRING EnvVar;
    LONGLONG llTemp;
    DWORD CharsConsumed;

    //
    //  Default to suggesting in 400ms after seeing 2 chars in an arg.
    //

    Buffer->DelayBeforeSuggesting = 400;
    Buffer->MinimumCharsInArgBeforeSuggesting = 2;

    //
    //  Check the environment to see if the user wants to override the
    //  suggestion delay.  Note a value of zero disables the feature.
    //

    EnvVarLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORISUGGESTIONDELAY"), NULL, 0);
    if (EnvVarLength > 0) {
        if (YoriLibAllocateString(&EnvVar, EnvVarLength)) {
            EnvVar.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORISUGGESTIONDELAY"), EnvVar.StartOfString, EnvVar.LengthAllocated);
            if (YoriLibStringToNumber(&EnvVar, TRUE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
                Buffer->DelayBeforeSuggesting = (ULONG)llTemp;
            }
            YoriLibFreeStringContents(&EnvVar);
        }
    }

    //
    //  Check the environment to see if the user wants to override the
    //  minimum number of characters needed in an arg before suggesting.
    //

    EnvVarLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORISUGGESTIONMINCHARS"), NULL, 0);
    if (EnvVarLength > 0) {
        if (YoriLibAllocateString(&EnvVar, EnvVarLength)) {
            EnvVar.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORISUGGESTIONMINCHARS"), EnvVar.StartOfString, EnvVar.LengthAllocated);
            if (YoriLibStringToNumber(&EnvVar, TRUE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
                Buffer->MinimumCharsInArgBeforeSuggesting = (ULONG)llTemp;
            }
            YoriLibFreeStringContents(&EnvVar);
        }
    }
}

/**
 Perform processing related to when a key is pressed.

 @param Buffer Pointer to the input buffer to update.

 @param InputRecord Pointer to the console input event.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriShProcessKeyDown(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in PINPUT_RECORD InputRecord,
    __out PBOOL TerminateInput
    )
{
    DWORD CtrlMask;
    TCHAR Char;
    DWORD Count;
    WORD KeyCode;
    WORD ScanCode;

    *TerminateInput = FALSE;
    YoriShPrepareForNextKey(Buffer);

    Char = InputRecord->Event.KeyEvent.uChar.UnicodeChar;
    CtrlMask = InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | ENHANCED_KEY | SHIFT_PRESSED);
    KeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;
    ScanCode = InputRecord->Event.KeyEvent.wVirtualScanCode;

    if (KeyCode >= VK_F1 && KeyCode <= VK_F12) {
        if (YoriShHotkey(Buffer, KeyCode, CtrlMask)) {
            *TerminateInput = TRUE;
            return TRUE;
        }
    }

    if (CtrlMask == 0 || CtrlMask == SHIFT_PRESSED) {

        if (Char == '\r') {
            if (!YoriShCopySelectionIfPresent(Buffer)) {
                *TerminateInput = TRUE;
            }
            return TRUE;
        } else if (Char == 27) {
            YoriShClearInput(Buffer);
        } else if (Char == '\t') {
            if ((CtrlMask & SHIFT_PRESSED) == 0) {
                YoriShTabCompletion(Buffer, 0);
            } else {
                YoriShTabCompletion(Buffer, YORI_SH_TAB_COMPLETE_BACKWARDS);
            }
        } else if (Char == '\b') {
            if (!YoriShOverwriteSelectionIfInInput(Buffer)) {
                YoriShBackspace(Buffer, InputRecord->Event.KeyEvent.wRepeatCount);
            }
        } else if (Char == '\0') {
        } else {
            for (Count = 0; Count < InputRecord->Event.KeyEvent.wRepeatCount; Count++) {
                TCHAR StringChar[2];
                StringChar[0] = Char;
                StringChar[1] = '\0';
                YoriShAddCStringToInput(Buffer, StringChar);
            }
        }
    } else if (CtrlMask == RIGHT_CTRL_PRESSED ||
               CtrlMask == LEFT_CTRL_PRESSED ||
               CtrlMask == (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) {
        if (KeyCode == 'C') {
            YoriShClearInput(Buffer);
            *TerminateInput = TRUE;
            return TRUE;
        } else if (KeyCode == 'E') {
            TCHAR StringChar[2];
            StringChar[0] = 27;
            StringChar[1] = '\0';
            YoriShAddCStringToInput(Buffer, StringChar);
        } else if (KeyCode == 'V') {
            YORI_STRING ClipboardData;
            YoriLibInitEmptyString(&ClipboardData);
            if (YoriShPasteText(&ClipboardData)) {
                YoriShAddYoriStringToInput(Buffer, &ClipboardData);
                YoriLibFreeStringContents(&ClipboardData);
            }
        } else if (KeyCode == VK_TAB) {
            YoriShTabCompletion(Buffer, YORI_SH_TAB_COMPLETE_FULL_PATH);
        }
    } else if (CtrlMask == (RIGHT_CTRL_PRESSED | SHIFT_PRESSED) ||
               CtrlMask == (LEFT_CTRL_PRESSED | SHIFT_PRESSED) ||
               CtrlMask == (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED)) {
        if (KeyCode == VK_TAB) {
            YoriShTabCompletion(Buffer, YORI_SH_TAB_COMPLETE_FULL_PATH | YORI_SH_TAB_COMPLETE_BACKWARDS);
        }
    } else if (CtrlMask == ENHANCED_KEY) {
        PYORI_LIST_ENTRY NewEntry = NULL;
        PYORI_HISTORY_ENTRY HistoryEntry;
        if (KeyCode == VK_UP) {
            NewEntry = YoriLibGetPreviousListEntry(&YoriShCommandHistory, Buffer->HistoryEntryToUse);
            if (NewEntry != NULL) {
                Buffer->HistoryEntryToUse = NewEntry;
                HistoryEntry = CONTAINING_RECORD(NewEntry, YORI_HISTORY_ENTRY, ListEntry);
                YoriShClearInput(Buffer);
                YoriShAddYoriStringToInput(Buffer, &HistoryEntry->CmdLine);
            }
        } else if (KeyCode == VK_DOWN) {
            if (Buffer->HistoryEntryToUse != NULL) {
                NewEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, Buffer->HistoryEntryToUse);
            }
            if (NewEntry != NULL) {
                Buffer->HistoryEntryToUse = NewEntry;
                HistoryEntry = CONTAINING_RECORD(NewEntry, YORI_HISTORY_ENTRY, ListEntry);
                YoriShClearInput(Buffer);
                YoriShAddYoriStringToInput(Buffer, &HistoryEntry->CmdLine);
            }
        } else if (KeyCode == VK_LEFT) {
            if (Buffer->CurrentOffset > 0) {
                Buffer->CurrentOffset--;
            }
        } else if (KeyCode == VK_RIGHT) {
            if (Buffer->CurrentOffset < Buffer->String.LengthInChars) {
                Buffer->CurrentOffset++;
            }
        } else if (KeyCode == VK_INSERT) {
            Buffer->CursorInfo.bVisible = TRUE;
            if (Buffer->InsertMode) {
                Buffer->InsertMode = FALSE;
                Buffer->CursorInfo.dwSize = 100;
            } else {
                Buffer->InsertMode = TRUE;
                Buffer->CursorInfo.dwSize = 20;
            }
            SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &Buffer->CursorInfo);
        } else if (KeyCode == VK_HOME) {
            Buffer->CurrentOffset = 0;
        } else if (KeyCode == VK_END) {
            Buffer->CurrentOffset = Buffer->String.LengthInChars;
        } else if (KeyCode == VK_DELETE) {

            if (!YoriShOverwriteSelectionIfInInput(Buffer)) {
                Count = InputRecord->Event.KeyEvent.wRepeatCount;
                if (Count + Buffer->CurrentOffset > Buffer->String.LengthInChars) {
                    Count = Buffer->String.LengthInChars - Buffer->CurrentOffset;
                }

                Buffer->CurrentOffset += Count;

                YoriShBackspace(Buffer, Count);
            }

        } else if (KeyCode == VK_RETURN) {
            if (!YoriShCopySelectionIfPresent(Buffer)) {
                *TerminateInput = TRUE;
            }
            return TRUE;
        }
    } else if (CtrlMask == (RIGHT_CTRL_PRESSED | ENHANCED_KEY) ||
               CtrlMask == (LEFT_CTRL_PRESSED | ENHANCED_KEY) ||
               CtrlMask == (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | ENHANCED_KEY)) {

        if (KeyCode == VK_LEFT) {
            YoriShMoveCursorToPriorArgument(Buffer);
        } else if (KeyCode == VK_RIGHT) {
            YoriShMoveCursorToNextArgument(Buffer);
        } else if (KeyCode == VK_UP) {
            YoriShTabCompletion(Buffer, YORI_SH_TAB_COMPLETE_HISTORY);
        } else if (KeyCode == VK_DOWN) {
            YoriShTabCompletion(Buffer, YORI_SH_TAB_COMPLETE_HISTORY | YORI_SH_TAB_COMPLETE_BACKWARDS);
        }
    } else if (CtrlMask == LEFT_ALT_PRESSED || CtrlMask == RIGHT_ALT_PRESSED ||
               CtrlMask == (LEFT_ALT_PRESSED | ENHANCED_KEY) ||
               CtrlMask == (RIGHT_ALT_PRESSED | ENHANCED_KEY)) {
        if (KeyCode >= '0' && KeyCode <= '9') {
            if (KeyCode == '0' && Buffer->NumericKeyValue == 0 && !Buffer->NumericKeyAnsiMode) {
                Buffer->NumericKeyAnsiMode = TRUE;
            } else {
                Buffer->NumericKeyValue = Buffer->NumericKeyValue * 10 + KeyCode - '0';
            }
        } else if (KeyCode >= VK_NUMPAD0 && KeyCode <= VK_NUMPAD9) {
            if (KeyCode == VK_NUMPAD0 && Buffer->NumericKeyValue == 0 && !Buffer->NumericKeyAnsiMode) {
                Buffer->NumericKeyAnsiMode = TRUE;
            } else {
                Buffer->NumericKeyValue = Buffer->NumericKeyValue * 10 + KeyCode - VK_NUMPAD0;
            }
        } else if (ScanCode >= 0x47 && ScanCode <= 0x49) {
            Buffer->NumericKeyValue = Buffer->NumericKeyValue * 10 + ScanCode - 0x47 + 7;
        } else if (ScanCode >= 0x4b && ScanCode <= 0x4d) {
            Buffer->NumericKeyValue = Buffer->NumericKeyValue * 10 + ScanCode - 0x4b + 4;
        } else if (ScanCode >= 0x4f && ScanCode <= 0x51) {
            Buffer->NumericKeyValue = Buffer->NumericKeyValue * 10 + ScanCode - 0x4f + 1;
        } else if (ScanCode == 0x52) {
            if (Buffer->NumericKeyValue == 0 && !Buffer->NumericKeyAnsiMode) {
                Buffer->NumericKeyAnsiMode = TRUE;
            } else {
                Buffer->NumericKeyValue = Buffer->NumericKeyValue * 10;
            }
        }
    } else if (CtrlMask == (SHIFT_PRESSED | ENHANCED_KEY)) {
        if (KeyCode == VK_INSERT) {
            YORI_STRING ClipboardData;
            YoriLibInitEmptyString(&ClipboardData);
            if (YoriShPasteText(&ClipboardData)) {
                YoriShAddYoriStringToInput(Buffer, &ClipboardData);
                YoriLibFreeStringContents(&ClipboardData);
            }
        }
    }

    if (KeyCode != VK_SHIFT &&
        KeyCode != VK_CONTROL) {

        YoriShPostKeyPress(Buffer);
        return TRUE;
    }

    return FALSE;
}


/**
 Perform processing related to when a key is released.  This is only used for
 Alt+Number numerical key sequences.

 @param Buffer Pointer to the input buffer to update.

 @param InputRecord Pointer to the console input event.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriShProcessKeyUp(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in PINPUT_RECORD InputRecord,
    __out PBOOL TerminateInput
    )
{
    BOOL KeyPressGenerated = FALSE;

    UNREFERENCED_PARAMETER(TerminateInput);

    if ((InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) == 0 &&
        Buffer->NumericKeyValue != 0) {

        CHAR SmallKeyValue = (CHAR)Buffer->NumericKeyValue;
        TCHAR HostKeyValue[2];

        HostKeyValue[0] = HostKeyValue[1] = 0;

        MultiByteToWideChar(Buffer->NumericKeyAnsiMode?CP_ACP:CP_OEMCP,
                            0,
                            &SmallKeyValue,
                            1,
                            HostKeyValue,
                            1);

        if (HostKeyValue != 0) {
            YoriShPrepareForNextKey(Buffer);
            YoriShAddCStringToInput(Buffer, HostKeyValue);
            YoriShPostKeyPress(Buffer);
            KeyPressGenerated = TRUE;
        }

        Buffer->NumericKeyValue = 0;
        Buffer->NumericKeyAnsiMode = FALSE;
    }

    return KeyPressGenerated;
}

/**
 Perform processing related to when a mouse button is pressed.

 @param Buffer Pointer to the input buffer to update.

 @param InputRecord Pointer to the console input event.

 @param ButtonsPressed A bit mask of buttons that were just pressed.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriShProcessMouseButtonDown(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in PINPUT_RECORD InputRecord,
    __in DWORD ButtonsPressed,
    __out PBOOL TerminateInput
    )
{
    BOOL BufferChanged = FALSE;

    UNREFERENCED_PARAMETER(TerminateInput);

    if (ButtonsPressed & FROM_LEFT_1ST_BUTTON_PRESSED) {
        DWORD StringOffset;

        BufferChanged = YoriShClearSelection(Buffer);

        Buffer->InitialSelectionPoint.X = InputRecord->Event.MouseEvent.dwMousePosition.X;
        Buffer->InitialSelectionPoint.Y = InputRecord->Event.MouseEvent.dwMousePosition.Y;

        if (YoriShStringOffsetFromCoordinates(Buffer, InputRecord->Event.MouseEvent.dwMousePosition, &StringOffset)) {
            Buffer->CurrentOffset = StringOffset;
            BufferChanged = TRUE;
        }
    } else if (ButtonsPressed & RIGHTMOST_BUTTON_PRESSED) {
        if (YoriShIsSelectionActive(Buffer)) {
            BufferChanged = YoriShCopySelectionIfPresent(Buffer);
            if (BufferChanged) {
                YoriShClearSelection(Buffer);
            }
        } else {
            YORI_STRING ClipboardData;
            YoriLibInitEmptyString(&ClipboardData);
            if (YoriShPasteText(&ClipboardData)) {
                YoriShAddYoriStringToInput(Buffer, &ClipboardData);
                YoriLibFreeStringContents(&ClipboardData);
                BufferChanged = TRUE;
            }
        }
    }

    return BufferChanged;
}

/**
 Perform processing related to when a mouse button is released.

 @param Buffer Pointer to the input buffer to update.

 @param InputRecord Pointer to the console input event.

 @param ButtonsReleased A bit mask of buttons that were just released.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriShProcessMouseButtonUp(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in PINPUT_RECORD InputRecord,
    __in DWORD ButtonsReleased,
    __out PBOOL TerminateInput
    )
{
    UNREFERENCED_PARAMETER(InputRecord);
    UNREFERENCED_PARAMETER(TerminateInput);

    //
    //  If the left mouse button was released and periodic scrolling was in
    //  effect, stop it now.
    //

    if (ButtonsReleased & FROM_LEFT_1ST_BUTTON_PRESSED) {
        Buffer->PeriodicScrollAmount.X = 0;
        Buffer->PeriodicScrollAmount.Y = 0;
    }

    return FALSE;
}

/**
 Return TRUE if the character should be considered a break character when the
 user double clicks to select.  Break characters are never themselves selected.

 @param Char The character to test for whether it is a break character.

 @return TRUE if the character is a break character, FALSE if it should be
         selected.
 */
BOOL
YoriShIsSelectionDoubleClickBreakChar(
    __in TCHAR Char
    )
{
    if (Char == ' ' ||
        Char == '>' ||
        Char == '<' ||
        Char == '|' ||
        Char == 0x2502) {  // Aka Unicode full vertical line (used by sdir)

        return TRUE;
    }

    return FALSE;
}


/**
 Perform processing related to when a mouse is double clicked.

 @param Buffer Pointer to the input buffer to update.

 @param InputRecord Pointer to the console input event.

 @param ButtonsPressed A bit mask of buttons that were just pressed.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriShProcessMouseDoubleClick(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in PINPUT_RECORD InputRecord,
    __in DWORD ButtonsPressed,
    __out PBOOL TerminateInput
    )
{
    BOOL BufferChanged = FALSE;
    HANDLE ConsoleHandle;
    COORD ReadPoint;
    TCHAR ReadChar;
    DWORD CharsRead;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    UNREFERENCED_PARAMETER(TerminateInput);

    if (ButtonsPressed & FROM_LEFT_1ST_BUTTON_PRESSED) {
        SHORT StartOffset;
        SHORT EndOffset;
        CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

        if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
            return FALSE;
        }

        BufferChanged = YoriShClearSelection(Buffer);

        ReadChar = ' ';
        ReadPoint.Y = InputRecord->Event.MouseEvent.dwMousePosition.Y;
        ReadPoint.X = InputRecord->Event.MouseEvent.dwMousePosition.X;

        //
        //  If the user double clicked on a break char, do nothing.
        //

        ReadConsoleOutputCharacter(ConsoleHandle, &ReadChar, 1, ReadPoint, &CharsRead);
        if (YoriShIsSelectionDoubleClickBreakChar(ReadChar)) {
            return FALSE;
        }

        //
        //  Nagivate left to find beginning of line or next break char.
        //

        for (StartOffset = InputRecord->Event.MouseEvent.dwMousePosition.X; StartOffset > 0; StartOffset--) {
            ReadPoint.X = (SHORT)(StartOffset - 1);
            ReadConsoleOutputCharacter(ConsoleHandle, &ReadChar, 1, ReadPoint, &CharsRead);
            if (YoriShIsSelectionDoubleClickBreakChar(ReadChar)) {
                break;
            }
        }

        //
        //  Navigate right to find end of line or next break char.
        //

        for (EndOffset = InputRecord->Event.MouseEvent.dwMousePosition.X; EndOffset < ScreenInfo.dwSize.X - 1; EndOffset++) {
            ReadPoint.X = (SHORT)(EndOffset + 1);
            ReadConsoleOutputCharacter(ConsoleHandle, &ReadChar, 1, ReadPoint, &CharsRead);
            if (YoriShIsSelectionDoubleClickBreakChar(ReadChar)) {
                break;
            }
        }

        Buffer->CurrentSelection.Top = ReadPoint.Y;
        Buffer->CurrentSelection.Bottom = ReadPoint.Y;
        Buffer->CurrentSelection.Left = StartOffset;
        Buffer->CurrentSelection.Right = EndOffset;

        BufferChanged = TRUE;

    }

    return BufferChanged;
}

/**
 If the user is holding down the mouse button and trying to select a region
 that is off the screen, this routine is called periodically to move the
 window within the buffer to allow the selection to take place.

 @param Buffer Pointer to the input buffer to update.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriShPeriodicScroll(
    __inout PYORI_INPUT_BUFFER Buffer
    )
{
    HANDLE ConsoleHandle;
    SHORT CellsToScroll;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
        return FALSE;
    }

    if (Buffer->PeriodicScrollAmount.Y < 0) {
        CellsToScroll = (SHORT)(0 - Buffer->PeriodicScrollAmount.Y);
        if (ScreenInfo.srWindow.Top > 0) {
            if (ScreenInfo.srWindow.Top > CellsToScroll) {
                ScreenInfo.srWindow.Top = (SHORT)(ScreenInfo.srWindow.Top - CellsToScroll);
                ScreenInfo.srWindow.Bottom = (SHORT)(ScreenInfo.srWindow.Bottom - CellsToScroll);
            } else {
                ScreenInfo.srWindow.Bottom = (SHORT)(ScreenInfo.srWindow.Bottom - ScreenInfo.srWindow.Top);
                ScreenInfo.srWindow.Top = 0;
            }
        }
    } else if (Buffer->PeriodicScrollAmount.Y > 0) {
        CellsToScroll = Buffer->PeriodicScrollAmount.Y;
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

    if (Buffer->PeriodicScrollAmount.X < 0) {
        CellsToScroll = (SHORT)(0 - Buffer->PeriodicScrollAmount.X);
        if (ScreenInfo.srWindow.Left > 0) {
            if (ScreenInfo.srWindow.Left > CellsToScroll) {
                ScreenInfo.srWindow.Left = (SHORT)(ScreenInfo.srWindow.Left - CellsToScroll);
                ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.srWindow.Right - CellsToScroll);
            } else {
                ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.srWindow.Right - ScreenInfo.srWindow.Left);
                ScreenInfo.srWindow.Left = 0;
            }
        }
    } else if (Buffer->PeriodicScrollAmount.X > 0) {
        CellsToScroll = Buffer->PeriodicScrollAmount.X;
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
 Perform processing related to a mouse move event.

 @param Buffer Pointer to the input buffer to update.

 @param InputRecord Pointer to the console input event.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriShProcessMouseMove(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in PINPUT_RECORD InputRecord,
    __out PBOOL TerminateInput
    )
{
    UNREFERENCED_PARAMETER(TerminateInput);

    if (InputRecord->Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
        CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
        HANDLE ConsoleHandle;

        ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

        if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
            return FALSE;
        }

        if (Buffer->InitialSelectionPoint.X < InputRecord->Event.MouseEvent.dwMousePosition.X) {
            Buffer->CurrentSelection.Left = Buffer->InitialSelectionPoint.X;
            Buffer->CurrentSelection.Right = InputRecord->Event.MouseEvent.dwMousePosition.X;
        } else {
            Buffer->CurrentSelection.Left = InputRecord->Event.MouseEvent.dwMousePosition.X;
            Buffer->CurrentSelection.Right = Buffer->InitialSelectionPoint.X;
        }

        if (Buffer->InitialSelectionPoint.Y < InputRecord->Event.MouseEvent.dwMousePosition.Y) {
            Buffer->CurrentSelection.Top = Buffer->InitialSelectionPoint.Y;
            Buffer->CurrentSelection.Bottom = InputRecord->Event.MouseEvent.dwMousePosition.Y;
        } else {
            Buffer->CurrentSelection.Top = InputRecord->Event.MouseEvent.dwMousePosition.Y;
            Buffer->CurrentSelection.Bottom = Buffer->InitialSelectionPoint.Y;
        }

        //
        //  Assume that the mouse move is inside the window, so periodic
        //  scrolling is off.
        //

        Buffer->PeriodicScrollAmount.X = 0;
        Buffer->PeriodicScrollAmount.Y = 0;

        //
        //  Check if it's outside the window and the extent of that
        //  distance to see which periodic scrolling may be enabled.
        //

        if (InputRecord->Event.MouseEvent.dwMousePosition.X < ScreenInfo.srWindow.Left) {
            Buffer->PeriodicScrollAmount.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - ScreenInfo.srWindow.Left);
        } else if (InputRecord->Event.MouseEvent.dwMousePosition.X > ScreenInfo.srWindow.Right) {
            Buffer->PeriodicScrollAmount.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - ScreenInfo.srWindow.Right);
        }

        if (InputRecord->Event.MouseEvent.dwMousePosition.Y < ScreenInfo.srWindow.Top) {
            Buffer->PeriodicScrollAmount.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - ScreenInfo.srWindow.Top);
        } else if (InputRecord->Event.MouseEvent.dwMousePosition.Y > ScreenInfo.srWindow.Bottom) {
            Buffer->PeriodicScrollAmount.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - ScreenInfo.srWindow.Bottom);
        }

        //
        //  Do one scroll immediately.  This allows the user to force scrolling
        //  by moving the mouse outside the window.
        //

        if (Buffer->PeriodicScrollAmount.X != 0 ||
            Buffer->PeriodicScrollAmount.Y != 0) {

            YoriShPeriodicScroll(Buffer);
        }

        return TRUE;
    }

    return FALSE;
}

/**
 Perform processing related to when a mouse wheel is scrolled.

 @param Buffer Pointer to the input buffer to update.

 @param InputRecord Pointer to the console input event.

 @param ButtonsPressed A bit mask of buttons that were just pressed.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
YoriShProcessMouseScroll(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in PINPUT_RECORD InputRecord,
    __in DWORD ButtonsPressed,
    __out PBOOL TerminateInput
    )
{
    HANDLE ConsoleHandle;
    SHORT Direction;
    SHORT LinesToScroll;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(ButtonsPressed);
    UNREFERENCED_PARAMETER(TerminateInput);

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    Direction = HIWORD(InputRecord->Event.MouseEvent.dwButtonState);
    if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
        return FALSE;
    }

    if (Direction > 0) {
        LinesToScroll = (SHORT)(Direction / 0x20);
        if (ScreenInfo.srWindow.Top > 0) {
            if (ScreenInfo.srWindow.Top > LinesToScroll) {
                ScreenInfo.srWindow.Top = (SHORT)(ScreenInfo.srWindow.Top - LinesToScroll);
                ScreenInfo.srWindow.Bottom = (SHORT)(ScreenInfo.srWindow.Bottom - LinesToScroll);
            } else {
                ScreenInfo.srWindow.Bottom = (SHORT)(ScreenInfo.srWindow.Bottom - ScreenInfo.srWindow.Top);
                ScreenInfo.srWindow.Top = 0;
            }
        }
    } else if (Direction < 0) {
        LinesToScroll = (SHORT)(0 - (Direction / 0x20));
        if (ScreenInfo.srWindow.Bottom < ScreenInfo.dwSize.Y - 1) {
            if (ScreenInfo.srWindow.Bottom < ScreenInfo.dwSize.Y - LinesToScroll - 1) {
                ScreenInfo.srWindow.Top = (SHORT)(ScreenInfo.srWindow.Top + LinesToScroll);
                ScreenInfo.srWindow.Bottom = (SHORT)(ScreenInfo.srWindow.Bottom + LinesToScroll);
            } else {
                ScreenInfo.srWindow.Top = (SHORT)(ScreenInfo.srWindow.Top + (ScreenInfo.dwSize.Y - ScreenInfo.srWindow.Bottom - 1));
                ScreenInfo.srWindow.Bottom = (SHORT)(ScreenInfo.dwSize.Y - 1);
            }
        }
    }

    SetConsoleWindowInfo(ConsoleHandle, TRUE, &ScreenInfo.srWindow);

    return FALSE;
}



/**
 Get a new expression from the user through the console.

 @param Expression On successful completion, updated to point to the
        entered expression.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShGetExpression(
    __inout PYORI_STRING Expression
    )
{
    YORI_INPUT_BUFFER Buffer;
    DWORD ActuallyRead = 0;
    DWORD CurrentRecordIndex = 0;
    DWORD err;
    INPUT_RECORD InputRecords[20];
    PINPUT_RECORD InputRecord;
    BOOL ReDisplayRequired;
    BOOL TerminateInput;
    BOOL RestartStateSaved = FALSE;
    BOOL SuggestionPopulated = FALSE;


    ZeroMemory(&Buffer, sizeof(Buffer));
    Buffer.InsertMode = TRUE;
    Buffer.CursorInfo.bVisible = TRUE;
    Buffer.CursorInfo.dwSize = 20;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &Buffer.CursorInfo);

    if (!YoriLibAllocateString(&Buffer.String, 256)) {
        return FALSE;
    }

    YoriShConfigureSuggestionSettings(&Buffer);

    while (TRUE) {

        if (!PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), InputRecords, sizeof(InputRecords)/sizeof(InputRecords[0]), &ActuallyRead)) {
            break;
        }

        ReDisplayRequired = FALSE;

        for (CurrentRecordIndex = 0; CurrentRecordIndex < ActuallyRead; CurrentRecordIndex++) {

            InputRecord = &InputRecords[CurrentRecordIndex];
            TerminateInput = FALSE;

            if (InputRecord->EventType == KEY_EVENT) {

                if (InputRecord->Event.KeyEvent.bKeyDown) {
                    ReDisplayRequired |= YoriShProcessKeyDown(&Buffer, InputRecord, &TerminateInput);
                } else {
                    ReDisplayRequired |= YoriShProcessKeyUp(&Buffer, InputRecord, &TerminateInput);
                }

                if (ReDisplayRequired) {
                    YoriShClearSelection(&Buffer);
                }

            } else if (InputRecord->EventType == MOUSE_EVENT) {

                DWORD ButtonsPressed = InputRecord->Event.MouseEvent.dwButtonState - (Buffer.PreviousMouseButtonState & InputRecord->Event.MouseEvent.dwButtonState);
                DWORD ButtonsReleased = Buffer.PreviousMouseButtonState - (Buffer.PreviousMouseButtonState & InputRecord->Event.MouseEvent.dwButtonState);

                if (ButtonsReleased > 0) {
                    ReDisplayRequired |= YoriShProcessMouseButtonUp(&Buffer, InputRecord, ButtonsReleased, &TerminateInput);
                }

                if (ButtonsPressed > 0) {
                    ReDisplayRequired |= YoriShProcessMouseButtonDown(&Buffer, InputRecord, ButtonsPressed, &TerminateInput);
                }

                Buffer.PreviousMouseButtonState = InputRecord->Event.MouseEvent.dwButtonState;
                if (InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_MOVED) {
                    ReDisplayRequired |= YoriShProcessMouseMove(&Buffer, InputRecord, &TerminateInput);
                }

                if (InputRecord->Event.MouseEvent.dwEventFlags & DOUBLE_CLICK) {
                    ReDisplayRequired |= YoriShProcessMouseDoubleClick(&Buffer, InputRecord, ButtonsPressed, &TerminateInput);
                }

                if (InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_WHEELED) {
                    ReDisplayRequired |= YoriShProcessMouseScroll(&Buffer, InputRecord, ButtonsPressed, &TerminateInput);
                }

            } else if (InputRecord->EventType == WINDOW_BUFFER_SIZE_EVENT) {

                ReDisplayRequired |= YoriShClearSelection(&Buffer);
            }

            if (TerminateInput) {
                YoriShTerminateInput(&Buffer);
                ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), InputRecords, CurrentRecordIndex + 1, &ActuallyRead);
                if (Buffer.String.LengthInChars > 0) {
                    YoriShAddToHistory(&Buffer.String);
                }
                memcpy(Expression, &Buffer.String, sizeof(YORI_STRING));
                return TRUE;
            }
        }

        if (ReDisplayRequired) {
            YoriShDisplayAfterKeyPress(&Buffer);
        }

        //
        //  If we processed any events, remove them from the queue.
        //

        if (ActuallyRead > 0) {
            if (!ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), InputRecords, ActuallyRead, &ActuallyRead)) {
                break;
            }
        }

        //
        //  Wait to see if any further events arrive.  If we haven't saved
        //  state and the user hasn't done anything for 30 seconds, save
        //  state.  Initialize the variable for compilers that are smart
        //  enough to expect variables to be initialized but too stupid to
        //  understand that they actually are.
        //

        SuggestionPopulated = FALSE;
        if (Buffer.SuggestionString.LengthInChars > 0 ||
            Buffer.DelayBeforeSuggesting == 0 ||
            Buffer.TabContext.TabCount != 0) {

            SuggestionPopulated = TRUE;
        }
        err = WAIT_OBJECT_0;
        while (TRUE) {
            if (Buffer.PeriodicScrollAmount.X != 0 ||
                Buffer.PeriodicScrollAmount.Y != 0) {

                err = WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), 250);
                if (err == WAIT_OBJECT_0) {
                    break;
                }
                if (err == WAIT_TIMEOUT) {
                    YoriShPeriodicScroll(&Buffer);
                }
            } else if (!SuggestionPopulated) {
                err = WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), Buffer.DelayBeforeSuggesting);
                if (err == WAIT_OBJECT_0) {
                    break;
                }
                if (err == WAIT_TIMEOUT) {
                    ASSERT(!SuggestionPopulated);
                    YoriShCompleteSuggestion(&Buffer);
                    SuggestionPopulated = TRUE;
                    Buffer.SuggestionDirty = TRUE;
                    if (Buffer.SuggestionString.LengthInChars > 0) {
                        YoriShDisplayAfterKeyPress(&Buffer);
                    }
                }
            } else if (!RestartStateSaved) {
                err = WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), 30 * 1000);
                if (err == WAIT_OBJECT_0) {
                    break;
                }
                if (err == WAIT_TIMEOUT) {
                    ASSERT(!RestartStateSaved);
                    YoriShSaveRestartState();
                    RestartStateSaved = TRUE;
                }
            } else {
                err = WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), INFINITE);
                if (err == WAIT_OBJECT_0) {
                    break;
                }
            }

            if (err != WAIT_TIMEOUT) {
                break;
            }
        }

        if (err != WAIT_OBJECT_0) {
            break;
        }
    }

    err = GetLastError();

    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Error reading from console %i handle %08x\n"), err, GetStdHandle(STD_INPUT_HANDLE));

    YoriLibFreeStringContents(&Buffer.String);
    return FALSE;
}


// vim:sw=4:ts=4:et:
