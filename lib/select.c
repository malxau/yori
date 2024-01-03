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
    return Selection->SelectionCurrentlyActive;
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
    return Selection->SelectionPreviouslyActive;
}

/**
 Returns TRUE if a selection has commenced from a point, and FALSE if it has
 not commenced or is a fully specified region.

 @param Selection The structure describing the current selection state.

 @return TRUE if a current selection has originated from a point, FALSE if it
         has not.
 */
BOOL
YoriLibSelectionInitialSpecified(
    __in PYORILIB_SELECTION Selection
    )
{
    return Selection->InitialSpecified;
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

    if (!YoriLibIsSizeAllocatable(RequiredLength * sizeof(WORD))) {
        return FALSE;
    }
    AttributeBuffer->AttributeArray = YoriLibMalloc((YORI_ALLOC_SIZE_T)(RequiredLength * sizeof(WORD)));
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

    //
    //  If there was no previous selection, clearing it is easy
    //

    if (!YoriLibIsPreviousSelectionActive(Selection)) {
        return;
    }

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    //
    //  Grab a pointer to the previous selection attributes.  Note the
    //  actual buffer can be NULL if there was an allocation failure
    //  when selecting.  In that case attributes are restored to a
    //  default color.
    //

    ActiveAttributes = &Selection->PreviousBuffer[Selection->CurrentPreviousIndex];
    AttributeReadPoint = ActiveAttributes->AttributeArray;

    LineLength = (SHORT)(Selection->PreviouslyDisplayed.Right - Selection->PreviouslyDisplayed.Left + 1);

    for (LineIndex = Selection->PreviouslyDisplayed.Top; LineIndex <= Selection->PreviouslyDisplayed.Bottom; LineIndex++) {
        StartPoint.X = Selection->PreviouslyDisplayed.Left;
        StartPoint.Y = LineIndex;

        YoriLibDisplayAttributes(ConsoleHandle, AttributeReadPoint, 0x07, LineLength, StartPoint, &CharsWritten);

        if (AttributeReadPoint) {
            AttributeReadPoint += LineLength;
        }
    }

    Selection->CurrentlyDisplayed.Left = 0;
    Selection->CurrentlyDisplayed.Right = 0;
    Selection->CurrentlyDisplayed.Top = 0;
    Selection->CurrentlyDisplayed.Bottom = 0;

    Selection->PreviouslyDisplayed.Left = Selection->CurrentlyDisplayed.Left;
    Selection->PreviouslyDisplayed.Top = Selection->CurrentlyDisplayed.Top;
    Selection->PreviouslyDisplayed.Right = Selection->CurrentlyDisplayed.Right;
    Selection->PreviouslyDisplayed.Bottom = Selection->CurrentlyDisplayed.Bottom;

    Selection->SelectionPreviouslyActive = FALSE;
}

/**
 Windows 10 consoles have a nasty bug where ReadConsoleOutputAttribute
 doesn't return correct colors when the console has previously displayed
 colors that are not describable in 16 color form.  ReadConsoleOutput does
 return the correct value though, so this routine uses that instead and
 reformats the result to provide the same interface as
 ReadConsoleOutputAttribute.  This needs a larger allocation though, so any
 allocation is kept on the Selection in the hope it can be used here for
 a subsequent operation.

 @param Selection Pointer to the selection which may contain a previous
        allocation for this routine to use, and may be populated with a new
        allocation for a later call to this routine.

 @param hConsole Handle to the console output.

 @param lpAttribute Pointer to an array of WORDs that is populated with
        attributes inside this routine.

 @param nLength The number of console cells and attributes to populate.

 @param dwReadCoord The console buffer coordinates to start reading
        attributes from.

 @param lpNumberOfAttrsRead On successful completion, updated to contain the
        number of attributes successfully read.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibReadConsoleOutputAttributeForSelection(
    __in PYORILIB_SELECTION Selection,
    __in HANDLE hConsole,
    __out_ecount(nLength) LPWORD lpAttribute,
    __in DWORD nLength,
    __in COORD dwReadCoord,
    __out LPDWORD lpNumberOfAttrsRead
    )
{

    PCHAR_INFO CharInfo;
    SMALL_RECT ReadRegion;
    COORD dwBufferSize;
    COORD dwBufferCoord;
    DWORD dwIndex;
    DWORD dwAllocSize;

    if (nLength > Selection->TempCharInfoBufferSize) {
        dwAllocSize = nLength * 2;
        if (dwAllocSize < 0x100) {
            dwAllocSize = 0x100;
        }

        if (!YoriLibIsSizeAllocatable(dwAllocSize * sizeof(CHAR_INFO))) {
            return FALSE;
        }

        CharInfo = YoriLibMalloc((YORI_ALLOC_SIZE_T)(dwAllocSize * sizeof(CHAR_INFO)));
        if (CharInfo == NULL) {
            return FALSE;
        }

        if (Selection->TempCharInfoBuffer != NULL) {
            YoriLibFree(Selection->TempCharInfoBuffer);
        }

        Selection->TempCharInfoBuffer = CharInfo;
        Selection->TempCharInfoBufferSize = dwAllocSize;
    }

    CharInfo = Selection->TempCharInfoBuffer;

    dwBufferSize.X = (SHORT)nLength;
    dwBufferSize.Y = 1;
    dwBufferCoord.X = 0;
    dwBufferCoord.Y = 0;
    ReadRegion.Left = dwReadCoord.X;
    ReadRegion.Top = dwReadCoord.Y;
    ReadRegion.Right = (SHORT)(dwReadCoord.X + nLength - 1);
    ReadRegion.Bottom = dwReadCoord.Y;

    if (!ReadConsoleOutput(hConsole, CharInfo, dwBufferSize, dwBufferCoord, &ReadRegion)) {
        return FALSE;
    }

    for (dwIndex = 0; dwIndex < nLength; dwIndex++) {
        lpAttribute[dwIndex] = CharInfo[dwIndex].Attributes;
    }

    if (lpNumberOfAttrsRead != NULL) {
        *lpNumberOfAttrsRead = ReadRegion.Right - ReadRegion.Left + 1;
    }

    return TRUE;
}

/**
 Nano doesn't implement ReadConsoleOutputCharacter, so fall back to
 ReadConsoleOutput.

 @param Selection Pointer to the selection which may contain a previous
        allocation for this routine to use, and may be populated with a new
        allocation for a later call to this routine.

 @param hConsole Handle to the console output.

 @param lpCharacter Pointer to an array of TCHARs that is populated with
        characters inside this routine.

 @param nLength The number of console cells and characters to populate.

 @param dwReadCoord The console buffer coordinates to start reading
        characters from.

 @param lpNumberOfCharsRead On successful completion, updated to contain the
        number of characters successfully read.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibReadConsoleOutputCharacterForSelection(
    __in PYORILIB_SELECTION Selection,
    __in HANDLE hConsole,
    __out_ecount(nLength) LPTSTR lpCharacter,
    __in DWORD nLength,
    __in COORD dwReadCoord,
    __out LPDWORD lpNumberOfCharsRead
    )
{
    PCHAR_INFO CharInfo;
    SMALL_RECT ReadRegion;
    COORD dwBufferSize;
    COORD dwBufferCoord;
    DWORD dwIndex;
    DWORD dwAllocSize;

    if (nLength > Selection->TempCharInfoBufferSize) {
        dwAllocSize = nLength * 2;
        if (dwAllocSize < 0x100) {
            dwAllocSize = 0x100;
        }

        if (!YoriLibIsSizeAllocatable(dwAllocSize * sizeof(CHAR_INFO))) {
            return FALSE;
        }

        CharInfo = YoriLibMalloc((YORI_ALLOC_SIZE_T)(dwAllocSize * sizeof(CHAR_INFO)));
        if (CharInfo == NULL) {
            return FALSE;
        }

        if (Selection->TempCharInfoBuffer != NULL) {
            YoriLibFree(Selection->TempCharInfoBuffer);
        }

        Selection->TempCharInfoBuffer = CharInfo;
        Selection->TempCharInfoBufferSize = dwAllocSize;
    }

    CharInfo = Selection->TempCharInfoBuffer;

    dwBufferSize.X = (SHORT)nLength;
    dwBufferSize.Y = 1;
    dwBufferCoord.X = 0;
    dwBufferCoord.Y = 0;
    ReadRegion.Left = dwReadCoord.X;
    ReadRegion.Top = dwReadCoord.Y;
    ReadRegion.Right = (SHORT)(dwReadCoord.X + nLength - 1);
    ReadRegion.Bottom = dwReadCoord.Y;

    if (!ReadConsoleOutput(hConsole, CharInfo, dwBufferSize, dwBufferCoord, &ReadRegion)) {
        return FALSE;
    }

    for (dwIndex = 0; dwIndex < nLength; dwIndex++) {
        lpCharacter[dwIndex] = CharInfo[dwIndex].Char.UnicodeChar;
    }

    if (lpNumberOfCharsRead != NULL) {
        *lpNumberOfCharsRead = ReadRegion.Right - ReadRegion.Left + 1;
    }

    return TRUE;
}



/**
 Return the selection color to use.  On Vista and newer systems this is the
 console popup color, which is what quickedit would do.  On Nano server,
 which doesn't support background colors, it's bright Yellow.  On older
 systems it is currently hardcoded to Yellow on Blue.

 @param ConsoleHandle Handle to the console output device.

 @return The color to use for selection.
 */
WORD
YoriLibGetSelectionColor(
    __in HANDLE ConsoleHandle
    )
{
    WORD SelectionColor;

    if (!YoriLibDoesSystemSupportBackgroundColors()) {
        SelectionColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    } else {
        SelectionColor = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    }

    if (DllKernel32.pGetConsoleScreenBufferInfoEx) {
        YORI_CONSOLE_SCREEN_BUFFER_INFOEX ScreenInfo;
        ZeroMemory(&ScreenInfo, sizeof(ScreenInfo));
        ScreenInfo.cbSize = sizeof(ScreenInfo);
        if (DllKernel32.pGetConsoleScreenBufferInfoEx(ConsoleHandle, &ScreenInfo)) {
            UCHAR Background;
            UCHAR Foreground;

            Background = (UCHAR)((ScreenInfo.wPopupAttributes & 0xF0) >> 4);
            Foreground = (UCHAR)(ScreenInfo.wPopupAttributes & 0xF);

            //
            //  If background == foreground, the selection would be invisible,
            //  so fall back to defaults.  This is mainly to handle Nano which
            //  succeeds the above call with a popup color of zero (black on
            //  black.)
            //

            if (Background != Foreground) {
                SelectionColor = ScreenInfo.wPopupAttributes;
            }
        }
    }

    return SelectionColor;
}

/**
 Set the color to use for a selection.  If this function is not called, the
 popup color from the console is used.

 @param Selection Pointer to the selection to set a custom selection color on.

 @param SelectionColor The Win32 selection color to use.
 */
VOID
YoriLibSetSelectionColor(
    __in PYORILIB_SELECTION Selection,
    __in WORD SelectionColor
    )
{
    Selection->SelectionColor = SelectionColor;
    Selection->SelectionColorSet = TRUE;
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
    PYORILIB_PREVIOUS_SELECTION_BUFFER ActiveAttributes;

    //
    //  If there is no current selection, drawing it is easy
    //

    if (!YoriLibIsSelectionActive(Selection)) {

        return;
    }

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    RequiredLength = (Selection->CurrentlyDisplayed.Right - Selection->CurrentlyDisplayed.Left + 1) *
                     (Selection->CurrentlyDisplayed.Bottom - Selection->CurrentlyDisplayed.Top + 1);

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
    LineLength = (SHORT)(Selection->CurrentlyDisplayed.Right - Selection->CurrentlyDisplayed.Left + 1);

    if (!Selection->SelectionColorSet) {
        Selection->SelectionColor = YoriLibGetSelectionColor(ConsoleHandle);
        Selection->SelectionColorSet = TRUE;
    }

    for (LineIndex = Selection->CurrentlyDisplayed.Top; LineIndex <= Selection->CurrentlyDisplayed.Bottom; LineIndex++) {
        StartPoint.X = Selection->CurrentlyDisplayed.Left;
        StartPoint.Y = LineIndex;

        if (AttributeWritePoint != NULL) {
            YoriLibReadConsoleOutputAttributeForSelection(Selection, ConsoleHandle, AttributeWritePoint, LineLength, StartPoint, &CharsWritten);
            AttributeWritePoint += LineLength;
        }

        FillConsoleOutputAttribute(ConsoleHandle, Selection->SelectionColor, LineLength, StartPoint, &CharsWritten);
    }

    Selection->PreviouslyDisplayed.Left = Selection->CurrentlyDisplayed.Left;
    Selection->PreviouslyDisplayed.Top = Selection->CurrentlyDisplayed.Top;
    Selection->PreviouslyDisplayed.Right = Selection->CurrentlyDisplayed.Right;
    Selection->PreviouslyDisplayed.Bottom = Selection->CurrentlyDisplayed.Bottom;
    Selection->SelectionPreviouslyActive = Selection->SelectionCurrentlyActive;
}

/**
 Given attributes describing a region of the console buffer and a new region
 of the console buffer, determine the attributes for the new region.  These
 are partly extracted from the console and partly copied from the previous
 region, if the two overlap.  Optionally, mark all of the cells within the
 new region as selected.  This is used when generating a selection rectangle;
 when copying cells from an off screen region, we need to calculate the
 attributes without updating the display.

 @param Selection Pointer to the selection, which is used purely as an
        allocation cache if temporary scratch space is needed.

 @param OldAttributes Pointer to a buffer describing the state of a previously
        selected range of cells.

 @param OldRegion Specifies the coordinates of the previously selected range.

 @param NewAttributes Pointer to a buffer to populate with the state of the
        new region.

 @param NewRegion Specifies the coordinates of the range whose attributes are
        requested.

 @param UpdateNewRegionDisplay If TRUE, the NewRegion area should be marked
        as selected within the console.  If FALSE, the range is left
        unmodified in the console.

 @param SelectionColor Specifies the color of the selection to apply within
        the console.  Only meaningful if UpdateNewRegionDisplay is TRUE.
 */
VOID
YoriLibCreateNewAttributeBufferFromPreviousBuffer(
    __inout PYORILIB_SELECTION Selection,
    __in PYORILIB_PREVIOUS_SELECTION_BUFFER OldAttributes,
    __in PSMALL_RECT OldRegion,
    __inout PYORILIB_PREVIOUS_SELECTION_BUFFER NewAttributes,
    __in PSMALL_RECT NewRegion,
    __in BOOL UpdateNewRegionDisplay,
    __in WORD SelectionColor
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

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    RequiredLength = (NewRegion->Right - NewRegion->Left + 1) * (NewRegion->Bottom - NewRegion->Top + 1);

    if (RequiredLength > NewAttributes->BufferSize) {
        YoriLibReallocateAttributeArray(NewAttributes, RequiredLength * 2);
    }

    LineLength = (SHORT)(NewRegion->Right - NewRegion->Left + 1);

    //
    //  Walk through all of the new selection to save off attributes for it,
    //  and update the console to have selection color
    //

    AttributeWritePoint = NewAttributes->AttributeArray;
    for (LineIndex = NewRegion->Top; LineIndex <= NewRegion->Bottom; LineIndex++) {

        //
        //  An entire line wasn't previously selected
        //

        if (LineIndex < OldRegion->Top ||
            LineIndex > OldRegion->Bottom) {

            StartPoint.X = NewRegion->Left;
            StartPoint.Y = LineIndex;

            if (AttributeWritePoint != NULL) {
                YoriLibReadConsoleOutputAttributeForSelection(Selection, ConsoleHandle, AttributeWritePoint, LineLength, StartPoint, &CharsWritten);
                AttributeWritePoint += LineLength;
            }

            if (UpdateNewRegionDisplay) {
                FillConsoleOutputAttribute(ConsoleHandle, SelectionColor, LineLength, StartPoint, &CharsWritten);
            }

        } else {

            //
            //  A set of characters to the left of the previous selection
            //  that are now selected
            //

            if (NewRegion->Left < OldRegion->Left) {

                StartPoint.X = NewRegion->Left;
                StartPoint.Y = LineIndex;

                RunLength = (SHORT)(OldRegion->Left - NewRegion->Left);
                if (LineLength < RunLength) {
                    RunLength = LineLength;
                }

                if (AttributeWritePoint != NULL) {
                    YoriLibReadConsoleOutputAttributeForSelection(Selection, ConsoleHandle, AttributeWritePoint, RunLength, StartPoint, &CharsWritten);
                    AttributeWritePoint += RunLength;
                }

                if (UpdateNewRegionDisplay) {
                    FillConsoleOutputAttribute(ConsoleHandle, SelectionColor, RunLength, StartPoint, &CharsWritten);
                }

            }

            //
            //  A set of characters were previously selected.  These
            //  attributes need to be migrated to the new buffer, but
            //  the console state is already correct
            //

            if (NewRegion->Right >= OldRegion->Left &&
                NewRegion->Left <= OldRegion->Right) {

                StartPoint.X = (SHORT)(NewRegion->Left - OldRegion->Left);
                RunLength = LineLength;
                if (StartPoint.X < 0) {
                    RunLength = (SHORT)(RunLength + StartPoint.X);
                    StartPoint.X = 0;
                }
                if (StartPoint.X + RunLength > OldRegion->Right - OldRegion->Left + 1) {
                    RunLength = (SHORT)(OldRegion->Right - OldRegion->Left + 1 - StartPoint.X);
                }

                StartPoint.Y = (SHORT)(LineIndex - OldRegion->Top);

                BufferOffset = (OldRegion->Right - OldRegion->Left + 1) * StartPoint.Y + StartPoint.X;

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

            if (NewRegion->Right > OldRegion->Right) {

                StartPoint.X = (SHORT)(OldRegion->Right + 1);
                if (NewRegion->Left > StartPoint.X) {
                    StartPoint.X = NewRegion->Left;
                    RunLength = LineLength;
                } else {
                    RunLength = (SHORT)(NewRegion->Right - StartPoint.X + 1);
                }
                StartPoint.Y = LineIndex;

                if (AttributeWritePoint != NULL) {
                    YoriLibReadConsoleOutputAttributeForSelection(Selection, ConsoleHandle, AttributeWritePoint, RunLength, StartPoint, &CharsWritten);
                    AttributeWritePoint += RunLength;
                }

                if (UpdateNewRegionDisplay) {
                    FillConsoleOutputAttribute(ConsoleHandle, SelectionColor, RunLength, StartPoint, &CharsWritten);
                }
            }
        }
    }

    //
    //  Go through the old selection looking for regions that are no longer
    //  selected, and restore their attributes into the console
    //

    if (UpdateNewRegionDisplay) {
        LineLength = (SHORT)(OldRegion->Right - OldRegion->Left + 1);

        for (LineIndex = OldRegion->Top; LineIndex <= OldRegion->Bottom; LineIndex++) {

            //
            //  A line was previously selected and no longer is.  Restore the
            //  saved attributes.
            //

            if (LineIndex < NewRegion->Top ||
                LineIndex > NewRegion->Bottom) {

                StartPoint.X = OldRegion->Left;
                StartPoint.Y = LineIndex;
                RunLength = LineLength;

                if (OldAttributes->AttributeArray != NULL) {
                    BufferOffset = LineLength * (LineIndex - OldRegion->Top);
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

                if (OldRegion->Left < NewRegion->Left) {

                    StartPoint.X = OldRegion->Left;
                    StartPoint.Y = LineIndex;
                    RunLength = LineLength;
                    if (NewRegion->Left - OldRegion->Left < RunLength) {
                        RunLength = (SHORT)(NewRegion->Left - OldRegion->Left);
                    }

                    if (OldAttributes->AttributeArray != NULL) {
                        BufferOffset = LineLength * (LineIndex - OldRegion->Top);
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

                if (OldRegion->Right > NewRegion->Right) {

                    BufferOffset = LineLength * (LineIndex - OldRegion->Top);
                    StartPoint.Y = LineIndex;

                    if (OldRegion->Left > NewRegion->Right) {
                        StartPoint.X = OldRegion->Left;
                        RunLength = LineLength;
                    } else {
                        RunLength = (SHORT)(OldRegion->Right - NewRegion->Right);
                        StartPoint.X = (SHORT)(NewRegion->Right + 1);
                        BufferOffset += NewRegion->Right - OldRegion->Left + 1;
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
    PYORILIB_PREVIOUS_SELECTION_BUFFER NewAttributes;
    PYORILIB_PREVIOUS_SELECTION_BUFFER OldAttributes;
    DWORD NewAttributeIndex;

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

    if (!Selection->SelectionColorSet) {
        HANDLE ConsoleHandle;
        ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        Selection->SelectionColor = YoriLibGetSelectionColor(ConsoleHandle);
        Selection->SelectionColorSet = TRUE;
    }

    YoriLibCreateNewAttributeBufferFromPreviousBuffer(Selection,
                                                      OldAttributes,
                                                      &Selection->PreviouslyDisplayed,
                                                      NewAttributes,
                                                      &Selection->CurrentlyDisplayed,
                                                      TRUE,
                                                      Selection->SelectionColor);

    ASSERT(Selection->CurrentPreviousIndex != NewAttributeIndex);
    Selection->CurrentPreviousIndex = NewAttributeIndex;

    Selection->PreviouslyDisplayed.Left = Selection->CurrentlyDisplayed.Left;
    Selection->PreviouslyDisplayed.Top = Selection->CurrentlyDisplayed.Top;
    Selection->PreviouslyDisplayed.Right = Selection->CurrentlyDisplayed.Right;
    Selection->PreviouslyDisplayed.Bottom = Selection->CurrentlyDisplayed.Bottom;
    Selection->SelectionPreviouslyActive = Selection->SelectionCurrentlyActive;
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

    if (Selection->TempCharInfoBuffer != NULL) {
        YoriLibFree(Selection->TempCharInfoBuffer);
        Selection->TempCharInfoBuffer = NULL;
        Selection->TempCharInfoBufferSize = 0;
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
    Selection->CurrentlyDisplayed.Left = 0;
    Selection->CurrentlyDisplayed.Right = 0;
    Selection->CurrentlyDisplayed.Top = 0;
    Selection->CurrentlyDisplayed.Bottom = 0;

    Selection->CurrentlySelected.Left = 0;
    Selection->CurrentlySelected.Right = 0;
    Selection->CurrentlySelected.Top = 0;
    Selection->CurrentlySelected.Bottom = 0;

    Selection->SelectionCurrentlyActive = FALSE;
    Selection->InitialSpecified = FALSE;

    Selection->PeriodicScrollAmount.X = 0;
    Selection->PeriodicScrollAmount.Y = 0;

    if (Selection->CurrentlyDisplayed.Left != Selection->PreviouslyDisplayed.Left ||
        Selection->CurrentlyDisplayed.Right != Selection->PreviouslyDisplayed.Right ||
        Selection->CurrentlyDisplayed.Top != Selection->PreviouslyDisplayed.Top ||
        Selection->CurrentlyDisplayed.Bottom != Selection->PreviouslyDisplayed.Bottom) {

        return TRUE;
    }

    return FALSE;
}

/**
 Given a signed 16 bit value and signed 16 bit change to it, perform the
 addition and return the result if it is within the specified Min and Max.  If
 the result is greater than Max, Max is returned; if the result is less than
 Min, Min is returned.

 @param BaseValue The value to operate on.

 @param Adjustment The signed value to add to BaseValue.

 @param Min The value which is the minimum result.

 @param Max The value which is the maximum result.

 @return The result of the signed addition.
 */
SHORT
YoriLibNewLineValueWithMinMax(
    __in SHORT BaseValue,
    __in SHORT Adjustment,
    __in SHORT Min,
    __in SHORT Max
    )
{
    LONG NewValue;

    NewValue = BaseValue + Adjustment;
    if (NewValue < Min) {
        return Min;
    }

    if (NewValue > Max) {
        return Max;
    }

    return (SHORT)NewValue;
}


/**
 Update the coordinates of a scroll region to reflect that the characters have
 been externally moved.

 @param Selection Pointer to the selection to update.

 @param LinesToMove Specifies the number of lines to move.  If this number is
        a positive number, it indicates that the coordinates are now at a
        greater cell location (eg. line 5 is now line 6) which occurs when the
        display scrolls upwards.  If this number is a negative number, it
        indicates that the coordinates are now at a lesser cell location (eg.
        line 6 is now line 5) which occurs when the display scrolls downwards.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibNotifyScrollBufferMoved(
    __inout PYORILIB_SELECTION Selection,
    __in SHORT LinesToMove
    )
{
    HANDLE ConsoleHandle;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
        return FALSE;
    }

    Selection->InitialPoint.Y = YoriLibNewLineValueWithMinMax(Selection->InitialPoint.Y, LinesToMove, 0, (SHORT)(ScreenInfo.dwSize.Y - 1));
    Selection->PreviouslyDisplayed.Top = YoriLibNewLineValueWithMinMax(Selection->PreviouslyDisplayed.Top, LinesToMove, 0, (SHORT)(ScreenInfo.dwSize.Y - 1));
    Selection->PreviouslyDisplayed.Bottom = YoriLibNewLineValueWithMinMax(Selection->PreviouslyDisplayed.Bottom, LinesToMove, 0, (SHORT)(ScreenInfo.dwSize.Y - 1));
    Selection->CurrentlyDisplayed.Top = YoriLibNewLineValueWithMinMax(Selection->CurrentlyDisplayed.Top, LinesToMove, 0, (SHORT)(ScreenInfo.dwSize.Y - 1));
    Selection->CurrentlyDisplayed.Bottom = YoriLibNewLineValueWithMinMax(Selection->CurrentlyDisplayed.Bottom, LinesToMove, 0, (SHORT)(ScreenInfo.dwSize.Y - 1));
    Selection->CurrentlySelected.Top = YoriLibNewLineValueWithMinMax(Selection->CurrentlySelected.Top, LinesToMove, 0, (SHORT)(ScreenInfo.dwSize.Y - 1));
    Selection->CurrentlySelected.Bottom = YoriLibNewLineValueWithMinMax(Selection->CurrentlySelected.Bottom, LinesToMove, 0, (SHORT)(ScreenInfo.dwSize.Y - 1));

    ASSERT(Selection->PreviouslyDisplayed.Top >= 0);
    ASSERT(Selection->CurrentlyDisplayed.Top >= 0);

    return TRUE;
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
    SMALL_RECT NewWindow;

    if (Selection->PeriodicScrollAmount.Y == 0 &&
        Selection->PeriodicScrollAmount.X == 0) {

        return FALSE;
    }

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
        return FALSE;
    }

    NewWindow.Top = ScreenInfo.srWindow.Top;
    NewWindow.Bottom = ScreenInfo.srWindow.Bottom;
    NewWindow.Left = ScreenInfo.srWindow.Left;
    NewWindow.Right = ScreenInfo.srWindow.Right;

    if (Selection->PeriodicScrollAmount.Y < 0) {
        CellsToScroll = (SHORT)(0 - Selection->PeriodicScrollAmount.Y);
        if (NewWindow.Top > 0) {
            if (NewWindow.Top > CellsToScroll) {
                NewWindow.Top = (SHORT)(NewWindow.Top - CellsToScroll);
                NewWindow.Bottom = (SHORT)(NewWindow.Bottom - CellsToScroll);
            } else {
                NewWindow.Bottom = (SHORT)(NewWindow.Bottom - NewWindow.Top);
                NewWindow.Top = 0;
            }
        }
    } else if (Selection->PeriodicScrollAmount.Y > 0) {
        CellsToScroll = Selection->PeriodicScrollAmount.Y;
        if (NewWindow.Bottom < ScreenInfo.dwSize.Y - 1) {
            if (NewWindow.Bottom < ScreenInfo.dwSize.Y - CellsToScroll - 1) {
                NewWindow.Top = (SHORT)(NewWindow.Top + CellsToScroll);
                NewWindow.Bottom = (SHORT)(NewWindow.Bottom + CellsToScroll);
            } else {
                NewWindow.Top = (SHORT)(NewWindow.Top + (ScreenInfo.dwSize.Y - NewWindow.Bottom - 1));
                NewWindow.Bottom = (SHORT)(ScreenInfo.dwSize.Y - 1);
            }
        }
    }

    if (Selection->PeriodicScrollAmount.X < 0) {
        CellsToScroll = (SHORT)(0 - Selection->PeriodicScrollAmount.X);
        if (NewWindow.Left > 0) {
            if (NewWindow.Left > CellsToScroll) {
                NewWindow.Left = (SHORT)(NewWindow.Left - CellsToScroll);
                NewWindow.Right = (SHORT)(NewWindow.Right - CellsToScroll);
            } else {
                NewWindow.Right = (SHORT)(NewWindow.Right - NewWindow.Left);
                NewWindow.Left = 0;
            }
        }
    } else if (Selection->PeriodicScrollAmount.X > 0) {
        CellsToScroll = Selection->PeriodicScrollAmount.X;
        if (NewWindow.Right < ScreenInfo.dwSize.X - 1) {
            if (NewWindow.Right < ScreenInfo.dwSize.X - CellsToScroll - 1) {
                NewWindow.Left = (SHORT)(NewWindow.Left + CellsToScroll);
                NewWindow.Right = (SHORT)(NewWindow.Right + CellsToScroll);
            } else {
                NewWindow.Left = (SHORT)(NewWindow.Left + (ScreenInfo.dwSize.X - NewWindow.Right - 1));
                NewWindow.Right = (SHORT)(ScreenInfo.dwSize.X - 1);
            }
        }
    }

    if (NewWindow.Left == ScreenInfo.srWindow.Left &&
        NewWindow.Right == ScreenInfo.srWindow.Right &&
        NewWindow.Top == ScreenInfo.srWindow.Top &&
        NewWindow.Bottom == ScreenInfo.srWindow.Bottom) {

        return FALSE;
    }

    SetConsoleWindowInfo(ConsoleHandle, TRUE, &NewWindow);
    return TRUE;
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

    Selection->InitialSpecified = TRUE;

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

    Selection->CurrentlyDisplayed.Top = StartY;
    Selection->CurrentlyDisplayed.Bottom = EndY;
    Selection->CurrentlyDisplayed.Left = StartX;
    Selection->CurrentlyDisplayed.Right = EndX;

    Selection->CurrentlySelected.Top = StartY;
    Selection->CurrentlySelected.Bottom = EndY;
    Selection->CurrentlySelected.Left = StartX;
    Selection->CurrentlySelected.Right = EndX;

    Selection->SelectionCurrentlyActive = TRUE;

    ASSERT(Selection->CurrentlyDisplayed.Bottom >= 0);
    ASSERT(Selection->CurrentlyDisplayed.Top >= 0);

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
    __in SHORT X,
    __in SHORT Y
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE ConsoleHandle;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
        return FALSE;
    }

    ASSERT(Selection->InitialSpecified);

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

    if (X <= ScreenInfo.srWindow.Left) {
        Selection->PeriodicScrollAmount.X = (SHORT)(X - ScreenInfo.srWindow.Left);
        if (Selection->PeriodicScrollAmount.X == 0) {
            Selection->PeriodicScrollAmount.X = -1;
        }
    } else if (X >= ScreenInfo.srWindow.Right) {
        Selection->PeriodicScrollAmount.X = (SHORT)(X - ScreenInfo.srWindow.Right);
        if (Selection->PeriodicScrollAmount.X == 0) {
            Selection->PeriodicScrollAmount.X = 1;
        }
    }

    if (Y <= ScreenInfo.srWindow.Top) {
        Selection->PeriodicScrollAmount.Y = (SHORT)(Y - ScreenInfo.srWindow.Top);
        if (Selection->PeriodicScrollAmount.Y == 0) {
            Selection->PeriodicScrollAmount.Y = -1;
        }
    } else if (Y >= ScreenInfo.srWindow.Bottom) {
        Selection->PeriodicScrollAmount.Y = (SHORT)(Y - ScreenInfo.srWindow.Bottom);
        if (Selection->PeriodicScrollAmount.Y == 0) {
            Selection->PeriodicScrollAmount.Y = 1;
        }
    }

    //
    //  Don't update the selection location outside of the window.  The caller
    //  can scroll the window when desired to select outside of it.
    //

    if (X < ScreenInfo.srWindow.Left) {
        X = ScreenInfo.srWindow.Left;
    } else if (X > ScreenInfo.srWindow.Right) {
        X = ScreenInfo.srWindow.Right;
    }

    if (Y < ScreenInfo.srWindow.Top) {
        Y = ScreenInfo.srWindow.Top;
    } else if (Y > ScreenInfo.srWindow.Bottom) {
        Y = ScreenInfo.srWindow.Bottom;
    }

    if (Selection->InitialPoint.X < X) {
        Selection->CurrentlyDisplayed.Left = Selection->InitialPoint.X;
        Selection->CurrentlyDisplayed.Right = X;
        Selection->CurrentlySelected.Left = Selection->InitialPoint.X;
        Selection->CurrentlySelected.Right = X;
    } else {
        Selection->CurrentlyDisplayed.Left = X;
        Selection->CurrentlyDisplayed.Right = Selection->InitialPoint.X;
        Selection->CurrentlySelected.Left = X;
        Selection->CurrentlySelected.Right = Selection->InitialPoint.X;
    }

    if (Selection->InitialPoint.Y < Y) {
        Selection->CurrentlyDisplayed.Top = Selection->InitialPoint.Y;
        Selection->CurrentlyDisplayed.Bottom = Y;
        Selection->CurrentlySelected.Top = Selection->InitialPoint.Y;
        Selection->CurrentlySelected.Bottom = Y;
    } else {
        Selection->CurrentlyDisplayed.Top = Y;
        Selection->CurrentlyDisplayed.Bottom = Selection->InitialPoint.Y;
        Selection->CurrentlySelected.Top = Y;
        Selection->CurrentlySelected.Bottom = Selection->InitialPoint.Y;
    }

    Selection->SelectionCurrentlyActive = TRUE;

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
    YORI_STRING RtfText;
    SHORT LineLength;
    SHORT LineCount;
    SHORT LineIndex;
    LPTSTR TextWritePoint;
    COORD StartPoint;
    DWORD CharsWritten;
    HANDLE ConsoleHandle;
    YORILIB_PREVIOUS_SELECTION_BUFFER Attributes;
    YORI_CONSOLE_SCREEN_BUFFER_INFOEX ScreenInfoEx;
    PDWORD ColorTableToUse = NULL;

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

    if (!YoriLibIsPreviousSelectionActive(Selection) &&
        (Selection->CurrentlyDisplayed.Left != Selection->PreviouslyDisplayed.Left ||
         Selection->CurrentlyDisplayed.Right != Selection->PreviouslyDisplayed.Right ||
         Selection->CurrentlyDisplayed.Top != Selection->PreviouslyDisplayed.Top ||
         Selection->CurrentlyDisplayed.Bottom != Selection->PreviouslyDisplayed.Bottom)) {

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

    LineLength = (SHORT)(Selection->CurrentlySelected.Right - Selection->CurrentlySelected.Left + 1);
    LineCount = (SHORT)(Selection->CurrentlySelected.Bottom - Selection->CurrentlySelected.Top + 1);

    if (!YoriLibAllocateString(&TextToCopy, (LineLength + 2) * LineCount)) {
        return FALSE;
    }

    //
    //  In the first pass, copy all of the text, including trailing spaces.
    //  This version will be used to construct the rich text form.
    //

    TextWritePoint = TextToCopy.StartOfString;
    for (LineIndex = Selection->CurrentlySelected.Top; LineIndex <= Selection->CurrentlySelected.Bottom; LineIndex++) {
        StartPoint.X = Selection->CurrentlySelected.Left;
        StartPoint.Y = LineIndex;

        YoriLibReadConsoleOutputCharacterForSelection(Selection, ConsoleHandle, TextWritePoint, LineLength, StartPoint, &CharsWritten);

        TextWritePoint += LineLength;
    }

    TextToCopy.LengthInChars = (YORI_ALLOC_SIZE_T)(TextWritePoint - TextToCopy.StartOfString);

    StartPoint.X = (SHORT)(Selection->CurrentlySelected.Right - Selection->CurrentlySelected.Left + 1);
    StartPoint.Y = (SHORT)(Selection->CurrentlySelected.Bottom - Selection->CurrentlySelected.Top + 1);

    //
    //  If the system clipboard is not available and clipboard support is
    //  emulated within the process, only plain text is supported.  If the
    //  system clipboard is available, build RTF and HTML forms.
    //

    if (!YoriLibIsSystemClipboardAvailable()) {
        if (YoriLibCopyTextWithProcessFallback(&TextToCopy)) {
            YoriLibFreeStringContents(&TextToCopy);
            return TRUE;
        }
    } else {

        //
        //  Combine the captured text with previously saved attributes into a
        //  VT100 stream.  This will turn into HTML and RTF.
        //

        YoriLibInitEmptyString(&VtText);
        ZeroMemory(&Attributes, sizeof(Attributes));

        YoriLibCreateNewAttributeBufferFromPreviousBuffer(Selection,
                                                          &Selection->PreviousBuffer[Selection->CurrentPreviousIndex],
                                                          &Selection->CurrentlyDisplayed,
                                                          &Attributes,
                                                          &Selection->CurrentlySelected,
                                                          FALSE,
                                                          0);

        if (Attributes.AttributeArray == NULL) {
            YoriLibFreeStringContents(&TextToCopy);
            return FALSE;
        }

        if (!YoriLibGenerateVtStringFromConsoleBuffers(&VtText, StartPoint, TextToCopy.StartOfString, Attributes.AttributeArray)) {
            YoriLibFree(Attributes.AttributeArray);
            YoriLibFreeStringContents(&TextToCopy);
            return FALSE;
        }

        YoriLibFree(Attributes.AttributeArray);

        //
        //  In the second pass, copy all of the text, truncating trailing spaces.
        //  This version will be used to construct the plain text form.
        //

        TextWritePoint = TextToCopy.StartOfString;
        for (LineIndex = Selection->CurrentlySelected.Top; LineIndex <= Selection->CurrentlySelected.Bottom; LineIndex++) {
            StartPoint.X = Selection->CurrentlySelected.Left;
            StartPoint.Y = LineIndex;

            YoriLibReadConsoleOutputCharacterForSelection(Selection, ConsoleHandle, TextWritePoint, LineLength, StartPoint, &CharsWritten);
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

        TextToCopy.LengthInChars = (YORI_ALLOC_SIZE_T)(TextWritePoint - TextToCopy.StartOfString);

        //
        //  Remove the final CRLF
        //

        if (TextToCopy.LengthInChars >= 2) {
            TextToCopy.LengthInChars -= 2;
        }

        //
        //  Convert the VT100 form into HTML and RTF, and free it
        //

        if (DllKernel32.pGetConsoleScreenBufferInfoEx) {
            ScreenInfoEx.cbSize = sizeof(ScreenInfoEx);
            if (DllKernel32.pGetConsoleScreenBufferInfoEx(ConsoleHandle, &ScreenInfoEx)) {
                ColorTableToUse = (PDWORD)&ScreenInfoEx.ColorTable;
            }
        }

        YoriLibInitEmptyString(&HtmlText);
        if (!YoriLibHtmlConvertToHtmlFromVt(&VtText, &HtmlText, ColorTableToUse, 4)) {
            YoriLibFreeStringContents(&VtText);
            YoriLibFreeStringContents(&TextToCopy);
            YoriLibFreeStringContents(&HtmlText);
            return FALSE;
        }

        YoriLibInitEmptyString(&RtfText);
        if (!YoriLibRtfConvertToRtfFromVt(&VtText, &RtfText, ColorTableToUse)) {
            YoriLibFreeStringContents(&VtText);
            YoriLibFreeStringContents(&TextToCopy);
            YoriLibFreeStringContents(&HtmlText);
            YoriLibFreeStringContents(&RtfText);
            return FALSE;
        }

        YoriLibFreeStringContents(&VtText);

        //
        //  Copy HTML, RTF and plain text forms to the clipboard
        //

        if (YoriLibCopyTextRtfAndHtml(&TextToCopy, &RtfText, &HtmlText)) {
            YoriLibFreeStringContents(&TextToCopy);
            YoriLibFreeStringContents(&HtmlText);
            YoriLibFreeStringContents(&RtfText);

            return TRUE;
        }

        YoriLibFreeStringContents(&HtmlText);
        YoriLibFreeStringContents(&RtfText);
    }
    YoriLibFreeStringContents(&TextToCopy);
    return FALSE;
}

/**
 Return the set of characters that should be considered break characters when
 the user double clicks to select.  Break characters are never themselves
 selected.

 @param BreakChars On successful completion, populated with a set of
        characters that should be considered break characters.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGetSelectionDoubleClickBreakChars(
    __out PYORI_STRING BreakChars
    )
{
    YORI_ALLOC_SIZE_T WriteIndex;
    YORI_ALLOC_SIZE_T ReadIndex;
    YORI_STRING Substring;

    YoriLibInitEmptyString(BreakChars);
    if (!YoriLibAllocateAndGetEnvironmentVariable(_T("YORIQUICKEDITBREAKCHARS"), BreakChars) || BreakChars->LengthInChars == 0) {

        //
        //  0x2500 is Unicode full horizontal line (used by sdir)
        //  0x2502 is Unicode full vertical line (used by sdir)
        //  0x00BB is double angle quotation mark, used in elevated prompts
        //

        YoriLibConstantString(BreakChars, _T(" \t'[]<>|\x2500\x2502\x252c\x2534\x00BB"));
        return TRUE;
    }

    ReadIndex = 0;
    WriteIndex = 0;
    YoriLibInitEmptyString(&Substring);

    for (ReadIndex = 0; ReadIndex < BreakChars->LengthInChars; ReadIndex++) {
        if (ReadIndex + 1 < BreakChars->LengthInChars &&
            BreakChars->StartOfString[ReadIndex] == '0' &&
            BreakChars->StartOfString[ReadIndex + 1] == 'x') {

            YORI_ALLOC_SIZE_T CharsConsumed;
            LONGLONG Number;

            Substring.StartOfString = &BreakChars->StartOfString[ReadIndex];
            Substring.LengthInChars = BreakChars->LengthInChars - ReadIndex;

            if (YoriLibStringToNumber(&Substring, FALSE, &Number, &CharsConsumed) &&
                CharsConsumed > 0 &&
                Number <= 0xFFFF) {

                BreakChars->StartOfString[WriteIndex] = (TCHAR)Number;
                WriteIndex++;
                ReadIndex = ReadIndex + CharsConsumed;
            }
        } else {
            if (ReadIndex != WriteIndex) {
                BreakChars->StartOfString[WriteIndex] = BreakChars->StartOfString[ReadIndex];
            }
            WriteIndex++;
        }
    }

    BreakChars->LengthInChars = WriteIndex;

    return TRUE;
}

/**
 Indicates if Yori QuickEdit should be enabled based on the state of the
 environment.  In this mode, the shell will disable QuickEdit support from
 the console and implement its own selection logic, but reenable QuickEdit
 for the benefit of applications.

 @return TRUE to indicate YoriQuickEdit should be enabled, and FALSE to
         leave the user's selection for QuickEdit behavior in effect.
 */
BOOL
YoriLibIsYoriQuickEditEnabled(VOID)
{
    YORI_STRING EnvVar;
    LONGLONG llTemp;
    YORI_ALLOC_SIZE_T CharsConsumed;

    YoriLibInitEmptyString(&EnvVar);
    if (!YoriLibAllocateAndGetEnvironmentVariable(_T("YORIQUICKEDIT"), &EnvVar)) {
        return FALSE;
    }

    if (YoriLibStringToNumber(&EnvVar, TRUE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
        YoriLibFreeStringContents(&EnvVar);
        if (llTemp == 1) {
            return TRUE;
        }
    }
    YoriLibFreeStringContents(&EnvVar);
    return FALSE;
}


// vim:sw=4:ts=4:et:
