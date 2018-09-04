/**
 * @file more/viewport.c
 *
 * Yori shell more console display
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

#include "more.h"

/**
 Return the number of characters within a subset of a physical line which
 will form a logical line.  Conceptually this represents either the minimum
 of the length of the string or the viewport width.  In practice it can be
 a little more convoluted due to nonprinting characters.

 MSFIX This needs to handle tabs, VT100 chars, etc

 @param MoreContext Pointer to the more context containing the data to
        display.

 @param PhysicalLineSubset Pointer to a string which represents either a
        complete physical line, or a trailing portion of a physical line after
        other logical lines have already been constructed from the previous
        characters.

 @return The number of characters within the next logical line.
 */
DWORD
MoreGetLogicalLineLength(
    __in PMORE_CONTEXT MoreContext,
    __in PYORI_STRING PhysicalLineSubset
    )
{
    if (PhysicalLineSubset->LengthInChars > MoreContext->ViewportWidth) {
        return MoreContext->ViewportWidth;
    }
    return PhysicalLineSubset->LengthInChars;
}

/**
 Return the number of logical lines that can be derived from a single
 physical line.  Note that each physical line must have at least one logical
 line, because an empty physical line translates to an empty logical line.

 @param MoreContext Pointer to the more context containing the data to
        display.

 @param PhysicalLine Pointer to the physical line to decompose into one or
        more logical lines.

 @return The number of logical lines within the physical line.
 */
DWORD
MoreCountLogicalLinesOnPhysicalLine(
    __in PMORE_CONTEXT MoreContext,
    __in PMORE_PHYSICAL_LINE PhysicalLine
    )
{
    DWORD Count = 0;
    DWORD LogicalLineLength;
    YORI_STRING Subset;

    YoriLibInitEmptyString(&Subset);
    Subset.StartOfString = PhysicalLine->LineContents.StartOfString;
    Subset.LengthInChars = PhysicalLine->LineContents.LengthInChars;
    while(TRUE) {
        LogicalLineLength = MoreGetLogicalLineLength(MoreContext, &Subset);
        Subset.StartOfString += LogicalLineLength;
        Subset.LengthInChars -= LogicalLineLength;
        Count++;
        if (Subset.LengthInChars == 0) {
            break;
        }
    }

    return Count;
}

/**
 From a specified physical line, generate one or more logical lines to
 display.

 @param MoreContext Pointer to the more context containing the data to
        display.

 @param PhysicalLine Pointer to the physical line to decompose into one or
        more logical lines.

 @param FirstLogicalLineIndex Specifies the number of the logical line within
        the physical line to return.  Zero represents the first local line
        within the physical line.

 @param NumberLogicalLines Specifies the number of logical lines to output.

 @param OutputLines Points to an array of NumberLogicalLines elements.
        On successful completion, these are populated with the logical lines
        derived from the physical line.
 */
VOID
MoreGenerateLogicalLinesFromPhysicalLine(
    __in PMORE_CONTEXT MoreContext,
    __in PMORE_PHYSICAL_LINE PhysicalLine,
    __in DWORD FirstLogicalLineIndex,
    __in DWORD NumberLogicalLines,
    __out PMORE_LOGICAL_LINE OutputLines
    )
{
    DWORD Count = 0;
    DWORD CharIndex = 0;
    DWORD LogicalLineLength;
    YORI_STRING Subset;
    PMORE_LOGICAL_LINE ThisLine;

    YoriLibInitEmptyString(&Subset);
    Subset.StartOfString = PhysicalLine->LineContents.StartOfString;
    Subset.LengthInChars = PhysicalLine->LineContents.LengthInChars;
    while(TRUE) {
        if (Count >= FirstLogicalLineIndex + NumberLogicalLines) {
            break;
        }
        LogicalLineLength = MoreGetLogicalLineLength(MoreContext, &Subset);
        if (Count >= FirstLogicalLineIndex) {
            ThisLine = &OutputLines[Count - FirstLogicalLineIndex];
            ThisLine->PhysicalLine = PhysicalLine;
            ThisLine->LogicalLineIndex = Count;
            ThisLine->PhysicalLineCharacterOffset = CharIndex;

            //
            //  MSFIX This needs to be referenced so that a logical line can
            //  be constructed that's not identical to a subset of the
            //  physical line.  This is desired for tab expansion (so we know
            //  how many spaces a tab represents), and searching where we
            //  might want to insert more formatting
            //

            YoriLibInitEmptyString(&ThisLine->Line);
            ThisLine->Line.StartOfString = Subset.StartOfString;
            ThisLine->Line.LengthInChars = LogicalLineLength;
        }


        Subset.StartOfString += LogicalLineLength;
        Subset.LengthInChars -= LogicalLineLength;
        Count++;
        CharIndex += LogicalLineLength;
        if (Subset.LengthInChars == 0) {
            break;
        }
    }
}


/**
 Return the previous set of logical lines preceeding a previous logical line.

 @param MoreContext Pointer to the more context specifying the data to
        display.

 @param CurrentLine Points to the current line that the previous lines should
        be returned for.

 @param LinesToOutput Specifies the number of lines to output.

 @param OutputLines An array of logical lines with LinesToOutput elements.  On
        successful completion, populated with the data for those logical lines.

 @return The number of logical lines generated.  This can be less than
         LinesToOutput if there is no more buffer.  When this occurs, the
         buffer is populated from last-to-first, so the first entries are the
         ones missing.
 */
DWORD
MoreGetPreviousLogicalLines(
    __inout PMORE_CONTEXT MoreContext,
    __in PMORE_LOGICAL_LINE CurrentLine,
    __in DWORD LinesToOutput,
    __out PMORE_LOGICAL_LINE OutputLines
    )
{
    DWORD LinesRemaining = LinesToOutput;
    DWORD LinesToCopy;
    DWORD LineIndexToCopy;
    PMORE_LOGICAL_LINE CurrentOutputLine;
    PMORE_LOGICAL_LINE CurrentInputLine;

    CurrentInputLine = CurrentLine;

    if (CurrentInputLine->LogicalLineIndex > 0) {
        if (CurrentInputLine->LogicalLineIndex > LinesRemaining) {
            LinesToCopy = LinesRemaining;
            LineIndexToCopy = CurrentInputLine->LogicalLineIndex - LinesToCopy;
        } else {
            LinesToCopy = CurrentInputLine->LogicalLineIndex;
            LineIndexToCopy = 0;
        }
        CurrentOutputLine = &OutputLines[LinesRemaining - LinesToCopy];
        MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                 CurrentInputLine->PhysicalLine,
                                                 LineIndexToCopy,
                                                 LinesToCopy,
                                                 CurrentOutputLine);
        LinesRemaining -= LinesToCopy;
    }

    while(LinesRemaining > 0) {
        PMORE_PHYSICAL_LINE PreviousPhysicalLine;
        PYORI_LIST_ENTRY ListEntry;
        DWORD LogicalLineCount;

        ListEntry = YoriLibGetPreviousListEntry(&MoreContext->PhysicalLineList, &CurrentInputLine->PhysicalLine->LineList);
        if (ListEntry == NULL) {
            break;
        }

        PreviousPhysicalLine = CONTAINING_RECORD(ListEntry, MORE_PHYSICAL_LINE, LineList);
        LogicalLineCount = MoreCountLogicalLinesOnPhysicalLine(MoreContext, PreviousPhysicalLine);

        if (LogicalLineCount > LinesRemaining) {
            LinesToCopy = LinesRemaining;
            LineIndexToCopy = LogicalLineCount - LinesToCopy;
        } else {
            LinesToCopy = LogicalLineCount;
            LineIndexToCopy = 0;
        }
        CurrentOutputLine = &OutputLines[LinesRemaining - LinesToCopy];
        MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                 PreviousPhysicalLine,
                                                 LineIndexToCopy,
                                                 LinesToCopy,
                                                 CurrentOutputLine);

        LinesRemaining -= LinesToCopy;
        CurrentInputLine = CurrentOutputLine;
    }
    return LinesToOutput - LinesRemaining;
}

/**
 Return the next set of logical lines following a previous logical line.  If
 no previous logical line is specified, returns the set from the first
 physical line.

 @param MoreContext Pointer to the more context specifying the data to
        display.

 @param CurrentLine Optionally points to the current line that the next lines
        should follow.

 @param LinesToOutput Specifies the number of lines to output.

 @param OutputLines An array of logical lines with LinesToOutput elements.  On
        successful completion, populated with the data for those logical lines.

 @return The number of logical lines generated.  This can be less than
         LinesToOutput if there is no more buffer.  When this occurs, the
         buffer is populated from first-to-last, so the last entries are the
         ones missing.
 */
DWORD
MoreGetNextLogicalLines(
    __inout PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE CurrentLine,
    __in DWORD LinesToOutput,
    __out PMORE_LOGICAL_LINE OutputLines
    )
{
    DWORD LinesRemaining = LinesToOutput;
    DWORD LinesToCopy;
    DWORD LineIndexToCopy;
    PMORE_LOGICAL_LINE CurrentOutputLine;
    PMORE_LOGICAL_LINE CurrentInputLine;
    DWORD LogicalLineCount;

    CurrentInputLine = CurrentLine;

    if (CurrentInputLine != NULL) {
        LogicalLineCount = MoreCountLogicalLinesOnPhysicalLine(MoreContext, CurrentInputLine->PhysicalLine);

        if (CurrentInputLine->LogicalLineIndex < LogicalLineCount - 1) {
            LineIndexToCopy = CurrentInputLine->LogicalLineIndex + 1;
            if (LogicalLineCount - CurrentInputLine->LogicalLineIndex - 1 > LinesRemaining) {
                LinesToCopy = LinesRemaining;
            } else {
                LinesToCopy = LogicalLineCount - CurrentInputLine->LogicalLineIndex - 1;
            }
            CurrentOutputLine = &OutputLines[LinesToOutput - LinesRemaining];
            MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                     CurrentInputLine->PhysicalLine,
                                                     LineIndexToCopy,
                                                     LinesToCopy,
                                                     CurrentOutputLine);
            LinesRemaining -= LinesToCopy;
        }
    }

    while(LinesRemaining > 0) {
        PMORE_PHYSICAL_LINE NextPhysicalLine;
        PYORI_LIST_ENTRY ListEntry;

        if (CurrentInputLine != NULL) {
            ListEntry = YoriLibGetNextListEntry(&MoreContext->PhysicalLineList, &CurrentInputLine->PhysicalLine->LineList);
        } else {
            ListEntry = YoriLibGetNextListEntry(&MoreContext->PhysicalLineList, NULL);
        }
        if (ListEntry == NULL) {

            break;
        }

        NextPhysicalLine = CONTAINING_RECORD(ListEntry, MORE_PHYSICAL_LINE, LineList);
        LogicalLineCount = MoreCountLogicalLinesOnPhysicalLine(MoreContext, NextPhysicalLine);

        LineIndexToCopy = 0;
        if (LogicalLineCount > LinesRemaining) {
            LinesToCopy = LinesRemaining;
        } else {
            LinesToCopy = LogicalLineCount;
        }
        CurrentOutputLine = &OutputLines[LinesToOutput - LinesRemaining];
        MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                 NextPhysicalLine,
                                                 LineIndexToCopy,
                                                 LinesToCopy,
                                                 CurrentOutputLine);

        LinesRemaining -= LinesToCopy;
        CurrentInputLine = &CurrentOutputLine[LinesToCopy - 1];
    }
    return LinesToOutput - LinesRemaining;
}

/**
 Given the current display buffer and a specified number of new lines to
 display after the current display buffer, update the display buffer and
 the actual display.

 @param MoreContext Pointer to the context describing the data to display.

 @param NewLines Specifies an array of new logical lines to add after
        the current display buffer.

 @param NewLineCount Specifies the number of lines to add, ie., the length
        of the NewLines array.
 */
VOID
MoreDisplayNewLinesInViewport(
    __inout PMORE_CONTEXT MoreContext,
    __in PMORE_LOGICAL_LINE NewLines,
    __in DWORD NewLineCount
    )
{
    DWORD Index;

    //
    //  MSFIX This function should own scrolling.
    //

    ASSERT(MoreContext->LinesInViewport + NewLineCount <= MoreContext->ViewportHeight);

    memcpy(&MoreContext->DisplayViewportLines[MoreContext->LinesInViewport],
           NewLines,
           sizeof(MORE_LOGICAL_LINE) * NewLineCount);

    MoreContext->LinesInViewport += NewLineCount;

    for (Index = 0; Index < NewLineCount; Index++) {
        ASSERT(NewLines[Index].Line.LengthInChars <= MoreContext->ViewportWidth);
        if (NewLines[Index].Line.LengthInChars >= MoreContext->ViewportWidth) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &NewLines[Index].Line);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &NewLines[Index].Line);
        }
    }
}

/**
 Given the current display buffer and a specified number of new lines to
 display before the current display buffer, update the display buffer and
 the actual display.

 @param MoreContext Pointer to the context describing the data to display.

 @param NewLines Specifies an array of new logical lines to insert before
        the current display buffer.

 @param NewLineCount Specifies the number of lines to add, ie., the length
        of the NewLines array.
 */
VOID
MoreDisplayPreviousLinesInViewport(
    __inout PMORE_CONTEXT MoreContext,
    __in PMORE_LOGICAL_LINE NewLines,
    __in DWORD NewLineCount
    )
{
    DWORD Index;
    DWORD LinesToPreserve;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE StdOutHandle;
    COORD NewPosition;
    DWORD NumberWritten;

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

    if (MoreContext->LinesInViewport > NewLineCount) {
        SMALL_RECT RectToMove;
        CHAR_INFO Fill;

        LinesToPreserve = MoreContext->LinesInViewport - NewLineCount;

        //
        //  MSFIX When these are referenced, we need to tear them down here
        //

        memmove(&MoreContext->DisplayViewportLines[NewLineCount],
                MoreContext->DisplayViewportLines,
                (LinesToPreserve * sizeof(MORE_LOGICAL_LINE)));

        //
        //  Move the text we want to preserve
        //

        RectToMove.Top = (USHORT)(ScreenInfo.dwCursorPosition.Y - MoreContext->LinesInViewport);
        RectToMove.Left = 0;
        RectToMove.Right = (USHORT)(ScreenInfo.dwSize.X - 1);
        RectToMove.Bottom = (USHORT)(RectToMove.Top + LinesToPreserve - 1);
        NewPosition.X = 0;
        NewPosition.Y = (USHORT)(RectToMove.Top + NewLineCount);
        Fill.Char.UnicodeChar = ' ';
        Fill.Attributes = YoriLibVtGetDefaultColor();
        ScrollConsoleScreenBuffer(StdOutHandle, &RectToMove, NULL, NewPosition, &Fill);
    }

    //
    //  Clear the region we want to overwrite
    //

    NewPosition.X = 0;
    NewPosition.Y = (USHORT)(ScreenInfo.dwCursorPosition.Y - MoreContext->LinesInViewport);
    FillConsoleOutputCharacter(StdOutHandle, ' ', ScreenInfo.dwSize.X * NewLineCount, NewPosition, &NumberWritten);
    FillConsoleOutputAttribute(StdOutHandle, YoriLibVtGetDefaultColor(), ScreenInfo.dwSize.X * NewLineCount, NewPosition, &NumberWritten);

    //
    //  Set the cursor to the top of the viewport
    //

    SetConsoleCursorPosition(StdOutHandle, NewPosition);

    memcpy(MoreContext->DisplayViewportLines,
           NewLines,
           sizeof(MORE_LOGICAL_LINE) * NewLineCount);

    MoreContext->LinesInViewport += NewLineCount;
    if (MoreContext->LinesInViewport > MoreContext->ViewportHeight) {
        MoreContext->LinesInViewport = MoreContext->ViewportHeight;
    }

    for (Index = 0; Index < NewLineCount; Index++) {

        //
        //  MSFIX Once we have nonprinting characters like VT, this needs
        //  to be smarter to know whether the number of printing characters
        //  reaches the end of the line
        //

        ASSERT(NewLines[Index].Line.LengthInChars <= MoreContext->ViewportWidth);
        if (NewLines[Index].Line.LengthInChars >= MoreContext->ViewportWidth) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &NewLines[Index].Line);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &NewLines[Index].Line);
        }
    }

    //
    //  Restore the cursor to the bottom of the viewport
    //

    NewPosition.X = 0;
    NewPosition.Y = ScreenInfo.dwCursorPosition.Y;
    SetConsoleCursorPosition(StdOutHandle, NewPosition);
}


/**
 Process new incoming data and add it to the bottom of the viewport.

 @param MoreContext Pointer to the context describing the data to display.
 */
VOID
MoreAddNewLinesToViewport(
    __inout PMORE_CONTEXT MoreContext
    )
{
    PMORE_LOGICAL_LINE CurrentLine;
    DWORD LinesDesired;
    DWORD LinesReturned;
    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    //
    //  MSFIX Need to preserve the previous logical line explicitly, outside
    //  of the viewport, so we can move to "next page", have an empty
    //  viewport, yet resume correctly
    //

    if (MoreContext->LinesInViewport == 0) {
        CurrentLine = NULL;
    } else {
        CurrentLine = &MoreContext->DisplayViewportLines[MoreContext->LinesInViewport - 1];
    }

    LinesDesired = MoreContext->ViewportHeight - MoreContext->LinesInViewport;

    LinesReturned = MoreGetNextLogicalLines(MoreContext, CurrentLine, LinesDesired, MoreContext->StagingViewportLines);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (LinesReturned == 0) {
        return;
    }

    MoreDisplayNewLinesInViewport(MoreContext, MoreContext->StagingViewportLines, LinesReturned);
}

/**
 Move the viewport up within the buffer of text, so that the previous line
 of data is rendered at the top of the screen.

 @param MoreContext Pointer to the context describing the data to display.

 @param LinesToMove Specifies the number of lines to move up.
 */
VOID
MoreMoveViewportUp(
    __inout PMORE_CONTEXT MoreContext,
    __in DWORD LinesToMove
    )
{
    PMORE_LOGICAL_LINE CurrentLine;
    DWORD LinesReturned;
    DWORD CappedLinesToMove;

    CappedLinesToMove = LinesToMove;

    if (MoreContext->LinesInViewport == 0) {
        return;
    }

    if (CappedLinesToMove > MoreContext->LinesInViewport) {
        CappedLinesToMove = MoreContext->LinesInViewport - 1;
    }

    if (CappedLinesToMove == 0) {
        return;
    }

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    //
    //  MSFIX Need to preserve the previous logical line explicitly, outside
    //  of the viewport, so we can move to "next page", have an empty
    //  viewport, yet resume correctly
    //

    CurrentLine = MoreContext->DisplayViewportLines;

    LinesReturned = MoreGetPreviousLogicalLines(MoreContext, CurrentLine, CappedLinesToMove, MoreContext->StagingViewportLines);

    ASSERT(LinesReturned <= CappedLinesToMove);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (LinesReturned == 0) {
        return;
    }

    MoreDisplayPreviousLinesInViewport(MoreContext, &MoreContext->StagingViewportLines[CappedLinesToMove - LinesReturned], LinesReturned);
}


/**
 Move the viewport down within the buffer of text, so that the next line
 of data is rendered at the bottom of the screen.

 @param MoreContext Pointer to the context describing the data to display.

 @param LinesToMove Specifies the number of lines to move down.
 */
VOID
MoreMoveViewportDown(
    __inout PMORE_CONTEXT MoreContext,
    __in DWORD LinesToMove
    )
{
    PMORE_LOGICAL_LINE CurrentLine;
    DWORD LinesReturned;
    DWORD CappedLinesToMove;
    DWORD LinesToPreserve;

    CappedLinesToMove = LinesToMove;

    //
    //  Preserve at least one line in the viewport to work around the
    //  MSFIX comment below
    //

    if (MoreContext->LinesInViewport == 0) {
        return;
    }

    if (CappedLinesToMove > MoreContext->LinesInViewport) {
        CappedLinesToMove = MoreContext->LinesInViewport - 1;
    }

    if (CappedLinesToMove == 0) {
        return;
    }

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    //
    //  MSFIX Need to preserve the previous logical line explicitly, outside
    //  of the viewport, so we can move to "next page", have an empty
    //  viewport, yet resume correctly
    //

    if (MoreContext->LinesInViewport == 0) {
        CurrentLine = NULL;
    } else {
        CurrentLine = &MoreContext->DisplayViewportLines[MoreContext->LinesInViewport - 1];
    }

    LinesReturned = MoreGetNextLogicalLines(MoreContext, CurrentLine, CappedLinesToMove, MoreContext->StagingViewportLines);

    ASSERT(LinesReturned <= CappedLinesToMove);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (LinesReturned == 0) {
        return;
    }

    //
    //  MSFIX Scrolling should move to the helper function
    //

    LinesToPreserve = MoreContext->LinesInViewport - LinesReturned;

    //
    //  MSFIX When these are referenced, need to tear down overwritten
    //  entries
    //

    memmove(MoreContext->DisplayViewportLines,
            &MoreContext->DisplayViewportLines[LinesReturned],
            (LinesToPreserve * sizeof(MORE_LOGICAL_LINE)));

    MoreContext->LinesInViewport -= LinesReturned;

    MoreDisplayNewLinesInViewport(MoreContext, MoreContext->StagingViewportLines, LinesReturned);
}


/**
 Perform the requested action when the user presses a key.

 @param MoreContext Pointer to the context describing the data to display.

 @param InputRecord Pointer to the structure describing the key that was
        pressed.

 @param Terminate On successful completion, set to TRUE to indicate the
        program should terminate.
 */
VOID
MoreProcessKeyDown(
    __inout PMORE_CONTEXT MoreContext,
    __in PINPUT_RECORD InputRecord,
    __out PBOOL Terminate
    )
{
    DWORD CtrlMask;
    TCHAR Char;
    WORD KeyCode;
    WORD ScanCode;

    UNREFERENCED_PARAMETER(MoreContext);

    *Terminate = FALSE;

    Char = InputRecord->Event.KeyEvent.uChar.UnicodeChar;
    CtrlMask = InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | ENHANCED_KEY | SHIFT_PRESSED);
    KeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;
    ScanCode = InputRecord->Event.KeyEvent.wVirtualScanCode;

    if (CtrlMask == 0 || CtrlMask == SHIFT_PRESSED) {
        if (Char == 'q' || Char == 'Q') {
            *Terminate = TRUE;
        } else if (Char == ' ') {
            MoreMoveViewportDown(MoreContext, MoreContext->ViewportHeight);
        }
    } else if (CtrlMask == ENHANCED_KEY) {
        if (KeyCode == VK_DOWN) {
            MoreMoveViewportDown(MoreContext, 1);
        } else if (KeyCode == VK_UP) {
            MoreMoveViewportUp(MoreContext, 1);
        } else if (KeyCode == VK_NEXT) {
            MoreMoveViewportDown(MoreContext, MoreContext->ViewportHeight);
        } else if (KeyCode == VK_PRIOR) {
            MoreMoveViewportUp(MoreContext, MoreContext->ViewportHeight);
        }
    }

    return;
}

/**
 Manage the console display of the more application.

 @param MoreContext Pointer to the context describing the data to display.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
BOOL
MoreViewportDisplay(
    __inout PMORE_CONTEXT MoreContext
    )
{
    HANDLE ObjectsToWaitFor[3];
    HANDLE StdInHandle;
    DWORD WaitObject;
    DWORD HandleCountToWait;
    BOOL WaitForIngestThread = TRUE;
    BOOL WaitForNewLines = TRUE;

    StdInHandle = GetStdHandle(STD_INPUT_HANDLE);

    while(TRUE) {

        //
        //  If the viewport is full, we don't care about new lines being
        //  ingested.
        //

        if (MoreContext->LinesInViewport == MoreContext->ViewportHeight) {
            WaitForNewLines = FALSE;
        } else {
            WaitForNewLines = TRUE;
        }

        HandleCountToWait = 0;
        ObjectsToWaitFor[HandleCountToWait++] = StdInHandle;
        if (WaitForNewLines) {
            ObjectsToWaitFor[HandleCountToWait++] = MoreContext->PhysicalLineAvailableEvent;
        }
        if (WaitForIngestThread) {
            ObjectsToWaitFor[HandleCountToWait++] = MoreContext->IngestThread;
        }

        WaitObject = WaitForMultipleObjects(HandleCountToWait, ObjectsToWaitFor, FALSE, INFINITE);
        if (WaitObject < WAIT_OBJECT_0 || WaitObject >= WAIT_OBJECT_0 + HandleCountToWait) {
            break;
        }
        if (ObjectsToWaitFor[WaitObject - WAIT_OBJECT_0] == MoreContext->PhysicalLineAvailableEvent) {

            MoreAddNewLinesToViewport(MoreContext);

        } else if (ObjectsToWaitFor[WaitObject - WAIT_OBJECT_0] == MoreContext->IngestThread) {

            WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

            //
            //  If the ingest thread has found zero lines and terminated, we
            //  don't really need a UI.  The onus is on the ingest thread to
            //  tell the user what went wrong.  Once we've seen it find
            //  anything at all, we'll do UI and wait for the user to
            //  indicate not to.
            //

            if (MoreContext->LineCount == 0) {
                ReleaseMutex(MoreContext->PhysicalLineMutex);
                break;
            } else {
                WaitForIngestThread = FALSE;
                ReleaseMutex(MoreContext->PhysicalLineMutex);
            }
        } else if (ObjectsToWaitFor[WaitObject - WAIT_OBJECT_0] == StdInHandle) {
            INPUT_RECORD InputRecords[20];
            PINPUT_RECORD InputRecord;
            DWORD ActuallyRead;
            DWORD CurrentIndex;
            BOOL Terminate = FALSE;

            if (!ReadConsoleInput(StdInHandle, InputRecords, sizeof(InputRecords)/sizeof(InputRecords[0]), &ActuallyRead)) {
                break;
            }

            for (CurrentIndex = 0; CurrentIndex < ActuallyRead; CurrentIndex++) {
                InputRecord = &InputRecords[CurrentIndex];
                if (InputRecord->EventType == KEY_EVENT &&
                    InputRecord->Event.KeyEvent.bKeyDown) {

                    MoreProcessKeyDown(MoreContext, InputRecord, &Terminate);
                }
            }

            if (Terminate) {
                break;
            }
        }
    }
    return TRUE;
}

// vim:sw=4:ts=4:et:
