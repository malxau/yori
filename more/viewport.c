/**
 * @file more/viewport.c
 *
 * Yori shell more console display
 *
 * Copyright (c) 2017-2022 Malcolm J. Smith
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
 Clear the screen.  This program generally does not do this; normally text
 flows downwards, causing the display to scroll, so history of the file is
 preserved within the terminal.  This function is useful when displaying
 text that can't possibly be sequentially meaningful; as of this writing,
 it's used when changing filter criteria to display a subset of lines.

 @param MoreContext Pointer to the more context.
 */
VOID
MoreClearScreen(
    __in PMORE_CONTEXT MoreContext
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE StdOutHandle;
    COORD ClearPosition;
    DWORD NumberWritten;
    DWORD CellsToWrite;

    UNREFERENCED_PARAMETER(MoreContext);

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

    ClearPosition.X = 0;
    ClearPosition.Y = ScreenInfo.srWindow.Top;

    CellsToWrite = ScreenInfo.dwSize.X;
    CellsToWrite = CellsToWrite * (ScreenInfo.srWindow.Bottom - ScreenInfo.srWindow.Top + 1);

    FillConsoleOutputCharacter(StdOutHandle, ' ', CellsToWrite, ClearPosition, &NumberWritten);
    FillConsoleOutputAttribute(StdOutHandle, YoriLibVtGetDefaultColor(), CellsToWrite, ClearPosition, &NumberWritten);

    SetConsoleCursorPosition(StdOutHandle, ClearPosition);
}

/**
 Clear any previously drawn status line.

 @param MoreContext Pointer to the current data, including physical lines and
        populated viewport lines.
 */
VOID
MoreClearStatusLine(
    __in PMORE_CONTEXT MoreContext
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE StdOutHandle;
    COORD ClearPosition;
    DWORD NumberWritten;

    UNREFERENCED_PARAMETER(MoreContext);

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

    //
    //  Clear the region we want to overwrite
    //

    ClearPosition.X = 0;
    ClearPosition.Y = ScreenInfo.dwCursorPosition.Y;

    FillConsoleOutputCharacter(StdOutHandle, ' ', ScreenInfo.dwSize.X, ClearPosition, &NumberWritten);
    FillConsoleOutputAttribute(StdOutHandle, YoriLibVtGetDefaultColor(), ScreenInfo.dwSize.X, ClearPosition, &NumberWritten);

    SetConsoleCursorPosition(StdOutHandle, ClearPosition);
}

/**
 Draw the status line indicating the lines currently displayed and percentage
 complete.

 @param MoreContext Pointer to the current data, including physical lines and
        populated viewport lines.
 */
VOID
MoreDrawStatusLine(
    __in PMORE_CONTEXT MoreContext
    )
{
    DWORDLONG FirstViewportLine;
    DWORDLONG LastViewportLine;
    DWORDLONG TotalLines;
    DWORDLONG TotalFilteredLines;
    BOOL PageFull;
    BOOL ThreadActive;
    LPTSTR StringToDisplay;
    YORI_STRING LineToDisplay;
    PYORI_STRING SearchString;
    UCHAR SearchIndex;
    DWORD InvisibleChars;
    DWORD Percent;

    //
    //  If the screen isn't full, there's no point displaying status
    //

    if (!MoreContext->FilterToSearch &&
        (MoreContext->LinesInViewport < MoreContext->ViewportHeight ||
         MoreContext->SuspendPagination)) {

        MoreContext->SearchDirty = FALSE;

        return;
    }

    YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    //
    //  Capture statistics
    //

    Percent = 0;
    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    TotalLines = MoreContext->LineCount;
    TotalFilteredLines = MoreContext->FilteredLineCount;
    MoreContext->TotalLinesInViewportStatus = TotalFilteredLines;

    if (MoreContext->FilterToSearch) {
        if (MoreContext->LinesInViewport > 0) {
            FirstViewportLine = MoreContext->DisplayViewportLines[0].PhysicalLine->FilteredLineNumber;
            LastViewportLine = MoreContext->DisplayViewportLines[MoreContext->LinesInViewport - 1].PhysicalLine->FilteredLineNumber;
            ASSERT(TotalFilteredLines > 0);
            Percent = (DWORD)(LastViewportLine * 100 / TotalFilteredLines);
        } else {
            FirstViewportLine = 0;
            LastViewportLine = 0;
            Percent = 0;
        }
    } else {
        ASSERT(MoreContext->LinesInViewport > 0);
        FirstViewportLine = MoreContext->DisplayViewportLines[0].PhysicalLine->LineNumber;
        LastViewportLine = MoreContext->DisplayViewportLines[MoreContext->LinesInViewport - 1].PhysicalLine->LineNumber;
        Percent = (DWORD)(LastViewportLine * 100 / TotalLines);
        ASSERT(TotalFilteredLines == TotalLines);
        TotalFilteredLines = TotalLines;
    }

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    ASSERT(MoreContext->LinesInPage <= MoreContext->LinesInViewport);
    if (MoreContext->LinesInViewport == MoreContext->LinesInPage) {
        PageFull = TRUE;
    } else {
        PageFull = FALSE;
    }

    //
    //  Check ingest status
    //

    if (WaitForSingleObject(MoreContext->IngestThread, 0) == WAIT_OBJECT_0) {
        ThreadActive = FALSE;
    } else {
        ThreadActive = TRUE;
    }

    if (!ThreadActive && TotalFilteredLines == LastViewportLine) {
        StringToDisplay = _T("End");
    } else if (!PageFull) {
        StringToDisplay = _T("Awaiting data");
    } else {
        StringToDisplay = _T("More");
    }

    YoriLibInitEmptyString(&LineToDisplay);
    InvisibleChars = 0;

    //
    //  If the user is actively entering a search term, display only that one.
    //  If not, display as much as we can fit.
    //

    if (MoreContext->SearchUiActive) {
        YORI_STRING SearchColorString;
        TCHAR SearchColorBuffer[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];

        SearchIndex = MoreSearchIndexForColorIndex(MoreContext, MoreContext->SearchColorIndex);
        SearchString = &MoreContext->SearchStrings[SearchIndex];

        YoriLibInitEmptyString(&SearchColorString);
        SearchColorString.StartOfString = SearchColorBuffer;
        SearchColorString.LengthAllocated = sizeof(SearchColorBuffer)/sizeof(SearchColorBuffer[0]);

        YoriLibVtStringForTextAttribute(&SearchColorString, 0, MoreContext->SearchColors[MoreContext->SearchColorIndex]);

        InvisibleChars = SearchColorString.LengthInChars;

        YoriLibYPrintf(&LineToDisplay,
                      _T(" --- %s --- (%lli-%lli of %lli, %i%%)%s %ySearch: %y"),
                      StringToDisplay,
                      FirstViewportLine,
                      LastViewportLine,
                      TotalFilteredLines,
                      Percent,
                      (MoreContext->FilterToSearch?_T(" (filtered)"):_T("")),
                      &SearchColorString,
                      SearchString);
    } else {
        UCHAR SearchCount;
        YORI_ALLOC_SIZE_T CharsNeeded;
        YORI_ALLOC_SIZE_T TotalSearchChars;
        YORI_ALLOC_SIZE_T SearchChars;
        YORI_ALLOC_SIZE_T Index;
        YORI_STRING Remaining;
        YORI_STRING SearchSubstring;

        SearchCount = MoreSearchCountActive(MoreContext);

        //
        //  We expect to fill the viewport width, but each search needs to
        //  change color and revert.  Note this isn't really a worst case
        //  allocation - it's possible that the viewport is too small to
        //  contain even the fixed portion and it will be reallocated anyway.
        //

        CharsNeeded = MoreContext->ViewportWidth + SearchCount * (YORI_MAX_INTERNAL_VT_ESCAPE_CHARS + 4);

        if (YoriLibAllocateString(&LineToDisplay, CharsNeeded)) {
            YoriLibYPrintf(&LineToDisplay,
                          _T(" --- %s --- (%lli-%lli of %lli, %i%%)%s"),
                          StringToDisplay,
                          FirstViewportLine,
                          LastViewportLine,
                          TotalFilteredLines,
                          Percent,
                          (MoreContext->FilterToSearch?_T(" (filtered)"):_T("")));


            //
            //  If searches are active, see how much space we can devote to
            //  each term.
            //

            TotalSearchChars = 0;
            SearchChars = 0;
            if (SearchCount > 0 &&
                LineToDisplay.LengthAllocated >= CharsNeeded &&
                LineToDisplay.LengthInChars < MoreContext->ViewportWidth) {

                TotalSearchChars = MoreContext->ViewportWidth - LineToDisplay.LengthInChars;
                SearchChars = TotalSearchChars / SearchCount;
            }

            //
            //  If we can display more than three per term, which really means
            //  one space plus three actual chars per term, loop through the
            //  terms to add them.
            //

            if (SearchChars > 3) {
                YoriLibInitEmptyString(&Remaining);
                YoriLibInitEmptyString(&SearchSubstring);
                Remaining.LengthAllocated = LineToDisplay.LengthAllocated - LineToDisplay.LengthInChars;
                Remaining.StartOfString = &LineToDisplay.StartOfString[LineToDisplay.LengthInChars];
                for (Index = 0; Index < SearchCount; Index++) {

                    //
                    //  Add a space to seperate components.
                    //

                    Remaining.StartOfString[0] = ' ';
                    Remaining.StartOfString = Remaining.StartOfString + 1;
                    Remaining.LengthAllocated = Remaining.LengthAllocated - 1;
                    LineToDisplay.LengthInChars = LineToDisplay.LengthInChars + 1;

                    //
                    //  Switch to the color used for the search term.
                    //

                    YoriLibVtStringForTextAttribute(&Remaining, 0, MoreContext->SearchColors[MoreContext->SearchContext[Index].ColorIndex]);
                    Remaining.StartOfString = Remaining.StartOfString + Remaining.LengthInChars;
                    Remaining.LengthAllocated = Remaining.LengthAllocated - Remaining.LengthInChars;
                    InvisibleChars = InvisibleChars + Remaining.LengthInChars;
                    LineToDisplay.LengthInChars = LineToDisplay.LengthInChars + Remaining.LengthInChars;
                    Remaining.LengthInChars = 0;

                    //
                    //  Take the search term, and truncate it as needed to
                    //  fit.
                    //

                    SearchSubstring.StartOfString = MoreContext->SearchStrings[Index].StartOfString;
                    SearchSubstring.LengthInChars = MoreContext->SearchStrings[Index].LengthInChars;
                    if (SearchSubstring.LengthInChars >= SearchChars) {
                        SearchSubstring.LengthInChars = SearchChars - 1;
                    }

                    //
                    //  Add the search term and reset the color.
                    //

                    Remaining.LengthInChars = YoriLibSPrintfS(Remaining.StartOfString, Remaining.LengthAllocated, _T("%y\x1b[0m"), &SearchSubstring);

                    Remaining.StartOfString = Remaining.StartOfString + Remaining.LengthInChars;
                    Remaining.LengthAllocated = Remaining.LengthAllocated - Remaining.LengthInChars;
                    LineToDisplay.LengthInChars = LineToDisplay.LengthInChars + Remaining.LengthInChars;
                    Remaining.LengthInChars = 0;
                    InvisibleChars = InvisibleChars + 4;

                }
            }
        }
    }

    //
    //  If the status line would be more than a line, truncate it.  Add three
    //  dots to the end of it if the console is a sane width to indicate that
    //  it has been truncated
    //

    if (MoreContext->ViewportWidth > 5) {
        if (LineToDisplay.LengthInChars > MoreContext->ViewportWidth + InvisibleChars - 1) {
            MoreTruncateStringToVisibleChars(&LineToDisplay, MoreContext->ViewportWidth - 3);
            LineToDisplay.StartOfString[LineToDisplay.LengthInChars] = '.';
            LineToDisplay.StartOfString[LineToDisplay.LengthInChars + 1] = '.';
            LineToDisplay.StartOfString[LineToDisplay.LengthInChars + 2] = '.';
            LineToDisplay.LengthInChars = LineToDisplay.LengthInChars + 3;
        }
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &LineToDisplay);
    YoriLibFreeStringContents(&LineToDisplay);
    MoreContext->SearchDirty = FALSE;

    YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, YoriLibVtGetDefaultColor());
}

/**
 Clear the screen and write out the display buffer.  This is slow because it
 doesn't take advantage of console scrolling, but it allows verification of
 the memory buffer.

 @param MoreContext Pointer to the more context specifying the data to
        display.
 */
VOID
MoreDegenerateDisplay(
    __in PMORE_CONTEXT MoreContext
    )
{
    DWORD Index;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE StdOutHandle;
    COORD NewPosition;
    DWORD NumberWritten;

    MoreClearStatusLine(MoreContext);

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

    //
    //  Clear the region we want to overwrite
    //

    NewPosition.X = 0;
    NewPosition.Y = 0;

    FillConsoleOutputCharacter(StdOutHandle, ' ', ScreenInfo.dwSize.X * MoreContext->LinesInViewport, NewPosition, &NumberWritten);
    FillConsoleOutputAttribute(StdOutHandle, YoriLibVtGetDefaultColor(), ScreenInfo.dwSize.X * MoreContext->LinesInViewport, NewPosition, &NumberWritten);

    //
    //  Set the cursor to the top of the viewport
    //

    SetConsoleCursorPosition(StdOutHandle, NewPosition);

    for (Index = 0; Index < MoreContext->LinesInViewport; Index++) {

        if (MoreContext->DisplayViewportLines[Index].InitialDisplayColor == 7) {
            if (Index % 2 != 0) {
                YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, 0x17);
            } else {
                YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, 0x7);
            }
        } else {
            YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, MoreContext->DisplayViewportLines[Index].InitialDisplayColor);
        }

        if (MoreContext->DisplayViewportLines[Index].ExplicitNewlineRequired) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\x1b[0m\n"), &MoreContext->DisplayViewportLines[Index].Line);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\x1b[0m"), &MoreContext->DisplayViewportLines[Index].Line);
        }
    }

    MoreDrawStatusLine(MoreContext);
}

/**
 Output a series of lines.  This function will attempt to group the series of
 lines into a single operation so the console only needs to scroll once.  If
 that fails, it falls back to line by line display.

 @param FirstLine Pointer to the first line in an array of lines to display.

 @param LineCount The number of lines to display.
 */
VOID
MoreOutputSeriesOfLines(
    __in PMORE_LOGICAL_LINE FirstLine,
    __in YORI_ALLOC_SIZE_T LineCount
    )
{
    YORI_ALLOC_SIZE_T Index;
    YORI_ALLOC_SIZE_T CharsRequired;
    YORI_STRING CombinedBuffer;
    YORI_STRING VtAttribute;
    TCHAR VtAttributeBuffer[32];

    YoriLibInitEmptyString(&VtAttribute);
    VtAttribute.StartOfString = VtAttributeBuffer;
    VtAttribute.LengthAllocated = sizeof(VtAttributeBuffer)/sizeof(TCHAR);

    CharsRequired = 0;

    for (Index = 0; Index < LineCount; Index++) {
        YoriLibVtStringForTextAttribute(&VtAttribute, 0, FirstLine[Index].InitialDisplayColor);
        CharsRequired = CharsRequired + VtAttribute.LengthInChars;
        CharsRequired = CharsRequired + FirstLine[Index].Line.LengthInChars;

        //
        //  When scrolling to a new line, the console can initialize the
        //  attributes of the new line as the active color.  Make sure we
        //  reset the color before displaying the newline.
        //

        CharsRequired += sizeof("e[0m\n");
    }

    CharsRequired++;

    if (YoriLibAllocateString(&CombinedBuffer, CharsRequired)) {
        CharsRequired = 0;
        for (Index = 0; Index < LineCount; Index++) {
            YoriLibVtStringForTextAttribute(&VtAttribute, 0, FirstLine[Index].InitialDisplayColor);
            memcpy(&CombinedBuffer.StartOfString[CharsRequired], VtAttribute.StartOfString, VtAttribute.LengthInChars * sizeof(TCHAR));
            CharsRequired = CharsRequired + VtAttribute.LengthInChars;
            memcpy(&CombinedBuffer.StartOfString[CharsRequired], FirstLine[Index].Line.StartOfString, FirstLine[Index].Line.LengthInChars * sizeof(TCHAR));
            CharsRequired = CharsRequired + FirstLine[Index].Line.LengthInChars;
            CombinedBuffer.StartOfString[CharsRequired] = 0x1b;
            CharsRequired++;
            CombinedBuffer.StartOfString[CharsRequired] = '[';
            CharsRequired++;
            CombinedBuffer.StartOfString[CharsRequired] = '0';
            CharsRequired++;
            CombinedBuffer.StartOfString[CharsRequired] = 'm';
            CharsRequired++;
            if (FirstLine[Index].ExplicitNewlineRequired) {
                CombinedBuffer.StartOfString[CharsRequired] = '\n';
                CharsRequired++;
            }
        }
        CombinedBuffer.StartOfString[CharsRequired] = '\0';
        CombinedBuffer.LengthInChars = CharsRequired;
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &CombinedBuffer);
        YoriLibFreeStringContents(&CombinedBuffer);
    } else {
        for (Index = 0; Index < LineCount; Index++) {
            YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, FirstLine[Index].InitialDisplayColor);

            //
            //  When scrolling to a new line, the console can initialize the
            //  attributes of the new line as the active color.  Make sure we
            //  reset the color before displaying the newline.
            //

            if (FirstLine[Index].ExplicitNewlineRequired) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\x1b[0m\n"), &FirstLine[Index].Line);
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\x1b[0m"), &FirstLine[Index].Line);
            }
        }
    }
}

/**
 After a search term has changed, redisplay all of the contents currently
 in the viewport with contents including highlighted search terms.  In the
 current implementation of this routine, only lines containing search
 match changes are redisplayed.

 @param MoreContext Pointer to the more context, which in this function
        indicates the currently displayed logical lines.

 @return The number of lines whose display has been altered.  This can be
         zero if no lines in the viewport contain a search match.
 */
DWORD
MoreDisplayChangedLinesInViewport(
    __inout PMORE_CONTEXT MoreContext
    )
{
    YORI_ALLOC_SIZE_T Index;
    YORI_ALLOC_SIZE_T ChangedLineCount = 0;
    YORI_ALLOC_SIZE_T NumberNewLines;
    DWORD NumberWritten;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE StdOutHandle;
    COORD NewPosition;

    if (MoreContext->LinesInViewport == 0) {
        return ChangedLineCount;
    }

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

    if (!MoreGetNextLogicalLines(MoreContext,
                                 &MoreContext->DisplayViewportLines[0],
                                 FALSE,
                                 MoreContext->LinesInViewport,
                                 MoreContext->StagingViewportLines,
                                 &NumberNewLines)) {

        return 0;
    }

    //
    //  Normally, the data shouldn't already be in the viewport if it's
    //  unavailable.  Previously available data can become unavailable when
    //  filtering to a search selection, since we might be viewing less data
    //  than before.
    //

    ASSERT(NumberNewLines == MoreContext->LinesInViewport || 
           (MoreContext->FilterToSearch && NumberNewLines <= MoreContext->LinesInViewport));

    for (Index = 0; Index < NumberNewLines; Index++) {

        if (YoriLibCompareString(&MoreContext->DisplayViewportLines[Index].Line, &MoreContext->StagingViewportLines[Index].Line) != 0) {

            ChangedLineCount++;

            //
            //  Clear the region we want to overwrite
            //

            NewPosition.X = 0;
            NewPosition.Y = (USHORT)(ScreenInfo.dwCursorPosition.Y - MoreContext->LinesInViewport + Index);
            FillConsoleOutputCharacter(StdOutHandle, ' ', ScreenInfo.dwSize.X, NewPosition, &NumberWritten);
            FillConsoleOutputAttribute(StdOutHandle, YoriLibVtGetDefaultColor(), ScreenInfo.dwSize.X, NewPosition, &NumberWritten);

            //
            //  Set the cursor to the top of the viewport
            //

            SetConsoleCursorPosition(StdOutHandle, NewPosition);

            MoreCloneLogicalLine(&MoreContext->DisplayViewportLines[Index], &MoreContext->StagingViewportLines[Index]);

            MoreOutputSeriesOfLines(&MoreContext->DisplayViewportLines[Index], 1);
        }
    }

    for (Index = 0; Index < MoreContext->LinesInViewport; Index++) {
        YoriLibFreeStringContents(&MoreContext->StagingViewportLines[Index].Line);
    }

    //
    //  MSFIX Do we need to free DisplayViewportLines after this? Do we need
    //  to clear these screen regions?
    //

    MoreContext->LinesInViewport = NumberNewLines;

    //
    //  Restore the cursor to the bottom of the viewport
    //

    NewPosition.X = 0;
    NewPosition.Y = ScreenInfo.dwCursorPosition.Y;
    SetConsoleCursorPosition(StdOutHandle, NewPosition);

    if (ChangedLineCount == 0) {
        return ChangedLineCount;
    }

    return ChangedLineCount;
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
    __in YORI_ALLOC_SIZE_T NewLineCount
    )
{
    YORI_ALLOC_SIZE_T Index;
    YORI_ALLOC_SIZE_T FirstLineToDisplay;
    YORI_ALLOC_SIZE_T LinesToPreserve;
    YORI_ALLOC_SIZE_T LineIndexToPreserve;

    //
    //  The math to calculate lines to preserve will get confused if we
    //  try to output more than a viewport
    //

    ASSERT(NewLineCount <= MoreContext->ViewportHeight);

    if (MoreContext->LinesInViewport + NewLineCount > MoreContext->ViewportHeight) {

        LinesToPreserve = MoreContext->ViewportHeight - NewLineCount;
        LineIndexToPreserve = MoreContext->LinesInViewport + NewLineCount - MoreContext->ViewportHeight;

        for (Index = 0; Index < LinesToPreserve; Index++) {
            MoreMoveLogicalLine(&MoreContext->DisplayViewportLines[Index],
                                &MoreContext->DisplayViewportLines[LineIndexToPreserve + Index]);
        }

        MoreContext->LinesInViewport = MoreContext->ViewportHeight - NewLineCount;
    }

    ASSERT(MoreContext->LinesInViewport + NewLineCount <= MoreContext->ViewportHeight);

    for (Index = 0; Index < NewLineCount; Index++) {
        MoreMoveLogicalLine(&MoreContext->DisplayViewportLines[MoreContext->LinesInViewport + Index], &NewLines[Index]);
    }

    FirstLineToDisplay = MoreContext->LinesInViewport;
    MoreContext->LinesInViewport = MoreContext->LinesInViewport + NewLineCount;
    MoreContext->LinesInPage = MoreContext->LinesInPage + NewLineCount;
    if (MoreContext->LinesInPage > MoreContext->LinesInViewport) {
        MoreContext->LinesInPage = MoreContext->LinesInViewport;
    }

    if (MoreContext->DebugDisplay) {
        MoreDegenerateDisplay(MoreContext);
    } else {
        MoreClearStatusLine(MoreContext);
        MoreOutputSeriesOfLines(&MoreContext->DisplayViewportLines[FirstLineToDisplay], NewLineCount);
        MoreDrawStatusLine(MoreContext);
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
    __in YORI_ALLOC_SIZE_T NewLineCount
    )
{
    YORI_ALLOC_SIZE_T Index;
    YORI_ALLOC_SIZE_T LinesToPreserve;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE StdOutHandle;
    YORI_ALLOC_SIZE_T OldLinesInViewport;
    COORD NewPosition;
    DWORD NumberWritten;
    SMALL_RECT RectToMove;
    CHAR_INFO Fill;

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

    //
    //  Note this logic is assuming that if the lines in viewport are less
    //  than the viewport height, this routine is not making new lines
    //  available, or else they would already have been visible.  That is,
    //  this program does not allow a pagedown or similar to leave a half
    //  populated screen, then allow the user to press up to add a line at
    //  the top of the viewport.  Pagedown will fill the viewport, even if
    //  that means moving by less than a page.
    //

    OldLinesInViewport = MoreContext->LinesInViewport;

    //
    //  If there are lines to retain, move them down in the buffer.
    //

    if (MoreContext->LinesInViewport > NewLineCount) {

        LinesToPreserve = MoreContext->LinesInViewport - NewLineCount;

        for (Index = LinesToPreserve; Index > 0; Index--) {
            MoreMoveLogicalLine(&MoreContext->DisplayViewportLines[Index + NewLineCount - 1],
                                &MoreContext->DisplayViewportLines[Index - 1]);
        }
    }

    //
    //  Scrolling up means no new data should be displayed as it arrives.
    //

    MoreContext->LinesInPage = MoreContext->LinesInViewport;

    //
    //  Add new lines to the top of the buffer.
    //

    for (Index = 0; Index < NewLineCount; Index++) {
        MoreMoveLogicalLine(&MoreContext->DisplayViewportLines[Index],
                            &NewLines[Index]);
    }

    if (MoreContext->DebugDisplay) {
        MoreDegenerateDisplay(MoreContext);
    } else {

        MoreClearStatusLine(MoreContext);

        if (OldLinesInViewport > NewLineCount) {

            LinesToPreserve = OldLinesInViewport - NewLineCount;

            //
            //  Move the text we want to preserve in the display
            //

            RectToMove.Top = (USHORT)(ScreenInfo.dwCursorPosition.Y - OldLinesInViewport);
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
        NewPosition.Y = (USHORT)(ScreenInfo.dwCursorPosition.Y - OldLinesInViewport);
        FillConsoleOutputCharacter(StdOutHandle, ' ', ScreenInfo.dwSize.X * NewLineCount, NewPosition, &NumberWritten);
        FillConsoleOutputAttribute(StdOutHandle, YoriLibVtGetDefaultColor(), ScreenInfo.dwSize.X * NewLineCount, NewPosition, &NumberWritten);

        //
        //  Set the cursor to the top of the viewport
        //

        SetConsoleCursorPosition(StdOutHandle, NewPosition);

        MoreOutputSeriesOfLines(MoreContext->DisplayViewportLines, NewLineCount);

        //
        //  Restore the cursor to the bottom of the viewport
        //

        NewPosition.X = 0;
        NewPosition.Y = ScreenInfo.dwCursorPosition.Y;
        SetConsoleCursorPosition(StdOutHandle, NewPosition);
        MoreDrawStatusLine(MoreContext);
    }
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
    YORI_ALLOC_SIZE_T LinesDesired;
    YORI_ALLOC_SIZE_T LinesReturned;
    BOOLEAN Success;

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    //
    //  Resume after the previous line, or the first physical line if there
    //  is no previous line
    //

    if (MoreContext->LinesInViewport == 0) {
        CurrentLine = NULL;
    } else {
        CurrentLine = &MoreContext->DisplayViewportLines[MoreContext->LinesInViewport - 1];
    }

    if (MoreContext->SuspendPagination) {
        LinesDesired = MoreContext->ViewportHeight;
    } else {
        LinesDesired = MoreContext->ViewportHeight - MoreContext->LinesInPage;
    }

    Success = MoreGetNextLogicalLines(MoreContext, CurrentLine, TRUE, LinesDesired, MoreContext->StagingViewportLines, &LinesReturned);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (!Success || LinesReturned == 0) {
        return;
    }

    MoreDisplayNewLinesInViewport(MoreContext, MoreContext->StagingViewportLines, LinesReturned);

}

/**
 Move the viewport to the top of the buffer of text.

 @param MoreContext Pointer to the context describing the data to display.

 @return The number of lines actually moved.
 */
YORI_ALLOC_SIZE_T
MoreMoveViewportToTop(
    __inout PMORE_CONTEXT MoreContext
    )
{
    BOOLEAN Success;
    YORI_ALLOC_SIZE_T LinesReturned;

    if (MoreContext->LinesInViewport == 0) {
        return 0;
    }

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    Success = MoreGetNextLogicalLines(MoreContext, NULL, TRUE, MoreContext->ViewportHeight, MoreContext->StagingViewportLines, &LinesReturned);

    ASSERT(!Success || LinesReturned <= MoreContext->ViewportHeight);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (!Success || LinesReturned == 0) {
        return 0;
    }

    MoreDisplayPreviousLinesInViewport(MoreContext, MoreContext->StagingViewportLines, LinesReturned);
    return LinesReturned;
}

/**
 Move the viewport to the bottom of the buffer of text.

 @param MoreContext Pointer to the context describing the data to display.

 @return The number of lines actually moved.
 */
YORI_ALLOC_SIZE_T
MoreMoveViewportToBottom(
    __inout PMORE_CONTEXT MoreContext
    )
{
    BOOLEAN Success;
    YORI_ALLOC_SIZE_T LinesReturned;

    if (MoreContext->LinesInViewport == 0) {
        return 0;
    }

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    Success = MoreGetPreviousLogicalLines(MoreContext, NULL, MoreContext->ViewportHeight, MoreContext->StagingViewportLines, &LinesReturned);

    ASSERT(!Success || LinesReturned <= MoreContext->ViewportHeight);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (!Success || LinesReturned == 0) {
        return 0;
    }

    MoreDisplayPreviousLinesInViewport(MoreContext, &MoreContext->StagingViewportLines[MoreContext->ViewportHeight - LinesReturned], LinesReturned);
    return LinesReturned;
}

/**
 Move the viewport up within the buffer of text, so that the previous line
 of data is rendered at the top of the screen.

 @param MoreContext Pointer to the context describing the data to display.

 @param LinesToMove Specifies the number of lines to move up.

 @return The number of lines actually moved.
 */
YORI_ALLOC_SIZE_T
MoreMoveViewportUp(
    __inout PMORE_CONTEXT MoreContext,
    __in YORI_ALLOC_SIZE_T LinesToMove
    )
{
    PMORE_LOGICAL_LINE CurrentLine;
    BOOLEAN Success;
    YORI_ALLOC_SIZE_T LinesReturned;
    YORI_ALLOC_SIZE_T CappedLinesToMove;

    CappedLinesToMove = LinesToMove;

    if (MoreContext->LinesInViewport == 0) {
        return 0;
    }

    if (CappedLinesToMove > MoreContext->LinesInViewport) {
        CappedLinesToMove = MoreContext->LinesInViewport - 1;
    }

    if (CappedLinesToMove == 0) {
        return 0;
    }

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    CurrentLine = MoreContext->DisplayViewportLines;

    Success = MoreGetPreviousLogicalLines(MoreContext, CurrentLine, CappedLinesToMove, MoreContext->StagingViewportLines, &LinesReturned);

    ASSERT(!Success || LinesReturned <= CappedLinesToMove);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (!Success || LinesReturned == 0) {
        return 0;
    }

    MoreDisplayPreviousLinesInViewport(MoreContext, &MoreContext->StagingViewportLines[CappedLinesToMove - LinesReturned], LinesReturned);
    return LinesReturned;
}


/**
 Move the viewport down within the buffer of text, so that the next line
 of data is rendered at the bottom of the screen.

 @param MoreContext Pointer to the context describing the data to display.

 @param LinesToMove Specifies the number of lines to move down.

 @return The number of lines actually moved.
 */
YORI_ALLOC_SIZE_T
MoreMoveViewportDown(
    __inout PMORE_CONTEXT MoreContext,
    __in YORI_ALLOC_SIZE_T LinesToMove
    )
{
    PMORE_LOGICAL_LINE CurrentLine;
    BOOLEAN Success;
    YORI_ALLOC_SIZE_T LinesReturned;
    YORI_ALLOC_SIZE_T CappedLinesToMove;

    CappedLinesToMove = LinesToMove;

    if (MoreContext->LinesInViewport == 0) {
        return 0;
    }

    if (CappedLinesToMove > MoreContext->LinesInViewport) {
        CappedLinesToMove = MoreContext->LinesInViewport;
    }

    if (CappedLinesToMove == 0) {
        return 0;
    }

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    if (MoreContext->LinesInViewport == 0) {
        CurrentLine = NULL;
    } else {
        CurrentLine = &MoreContext->DisplayViewportLines[MoreContext->LinesInViewport - 1];
    }

    Success = MoreGetNextLogicalLines(MoreContext, CurrentLine, TRUE, CappedLinesToMove, MoreContext->StagingViewportLines, &LinesReturned);

    ASSERT(!Success || LinesReturned <= CappedLinesToMove);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (!Success || LinesReturned == 0) {
        return 0;
    }

    MoreDisplayNewLinesInViewport(MoreContext, MoreContext->StagingViewportLines, LinesReturned);

    return LinesReturned;
}

/**
 Regenerate new logical lines into the viewport based on the next line
 of data is rendered at the bottom of the screen.

 @param MoreContext Pointer to the context describing the data to display.

 @param FirstPhysicalLine Pointer to the first physical line to display on
        the first line of the regenerated display.
 */
VOID
MoreRegenerateViewport(
    __inout PMORE_CONTEXT MoreContext,
    __in_opt PMORE_PHYSICAL_LINE FirstPhysicalLine
    )
{
    YORI_ALLOC_SIZE_T LinesReturned;
    YORI_ALLOC_SIZE_T CappedLinesToMove;
    BOOLEAN Success;
    MORE_LOGICAL_LINE CurrentLogicalLine;
    MORE_LOGICAL_LINE PreviousLogicalLine;
    PMORE_LOGICAL_LINE LineToFollow;

    ZeroMemory(&CurrentLogicalLine, sizeof(CurrentLogicalLine));
    ZeroMemory(&PreviousLogicalLine, sizeof(PreviousLogicalLine));
    CurrentLogicalLine.PhysicalLine = FirstPhysicalLine;
    CappedLinesToMove = MoreContext->ViewportHeight;

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    LineToFollow = NULL;
    if (FirstPhysicalLine != NULL) {
        if (MoreGetPreviousLogicalLines(MoreContext, &CurrentLogicalLine, 1, &PreviousLogicalLine, &LinesReturned) && LinesReturned > 0) {
            LineToFollow = &PreviousLogicalLine;
        }
    }

    Success = MoreGetNextLogicalLines(MoreContext, LineToFollow, TRUE, CappedLinesToMove, MoreContext->StagingViewportLines, &LinesReturned);

    if (LineToFollow != NULL) {
        YoriLibFreeStringContents(&LineToFollow->Line);
    }

    ASSERT(!Success || LinesReturned <= CappedLinesToMove);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (!Success || LinesReturned == 0) {
        return;
    }

    MoreDisplayNewLinesInViewport(MoreContext, MoreContext->StagingViewportLines, LinesReturned);
}

/**
 Display the buffer including and following a specified line.  If there's
 not enough buffer to fill the viewport, use earlier lines.  If there's still
 not enough lines, don't do anything, since everything is already displayed.

 This is used to display search matches - they must be on the screen, but the
 screen should still be full.

 @param MoreContext Pointer to the context describing the data to display.

 @param FirstPhysicalLine Pointer to the first physical line to ensure is
        visible on the regenerated display.
 */
VOID
MoreGenerateEntireViewportWithStartingLine(
    __inout PMORE_CONTEXT MoreContext,
    __in_opt PMORE_PHYSICAL_LINE FirstPhysicalLine
    )
{
    YORI_ALLOC_SIZE_T LinesReturned;
    YORI_ALLOC_SIZE_T LinesRemaining;
    YORI_ALLOC_SIZE_T CappedLinesToMove;
    BOOLEAN Success;
    MORE_LOGICAL_LINE CurrentLogicalLine;
    MORE_LOGICAL_LINE PreviousLogicalLine;
    PMORE_LOGICAL_LINE LineToFollow;
    YORI_ALLOC_SIZE_T Index;

    ZeroMemory(&CurrentLogicalLine, sizeof(CurrentLogicalLine));
    ZeroMemory(&PreviousLogicalLine, sizeof(PreviousLogicalLine));
    CurrentLogicalLine.PhysicalLine = FirstPhysicalLine;
    CappedLinesToMove = MoreContext->ViewportHeight;

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    LineToFollow = NULL;
    if (FirstPhysicalLine != NULL) {
        if (MoreGetPreviousLogicalLines(MoreContext, &CurrentLogicalLine, 1, &PreviousLogicalLine, &LinesReturned) && LinesReturned > 0) {
            LineToFollow = &PreviousLogicalLine;
        }
    }

    Success = MoreGetNextLogicalLines(MoreContext, LineToFollow, TRUE, CappedLinesToMove, MoreContext->StagingViewportLines, &LinesReturned);

    if (LineToFollow != NULL) {
        YoriLibFreeStringContents(&LineToFollow->Line);
    }

    if (Success) {

        //
        //  If reading forward didn't provide enough lines, start reading
        //  backwards
        //
        //  MSFIX Since this routine just grabbed the mutex, it's always
        //  possible that it found more lines than any previous render did -
        //  it's not valid to make this assumption anywhere.  How can these
        //  bugs be found?
        //

        if (MoreContext->LinesInViewport > LinesReturned) {
            LinesRemaining = MoreContext->LinesInViewport - LinesReturned;

            for (Index = LinesReturned; Index > 0; Index--) {
                MoreMoveLogicalLine(&MoreContext->StagingViewportLines[LinesRemaining + Index - 1],
                                    &MoreContext->StagingViewportLines[Index - 1]);
            }
            if (LinesReturned > 0) {
                LineToFollow = &MoreContext->StagingViewportLines[LinesRemaining];
            } else {
                LineToFollow = NULL;
            }
            Success = MoreGetPreviousLogicalLines(MoreContext, LineToFollow, LinesRemaining, MoreContext->StagingViewportLines, &LinesReturned);

            //
            //  If there's still not enough lines, don't display anything
            //

            if (!Success || LinesReturned < LinesRemaining) {
                for (Index = 0; Index < MoreContext->LinesInViewport; Index++) {
                    YoriLibFreeStringContents(&MoreContext->StagingViewportLines[Index].Line);
                }
                LinesReturned = 0;
                Success = FALSE;
            } else {
                LinesReturned = MoreContext->LinesInViewport;
            }
        }
    }

    //
    //  If the contents of the viewport aren't changing (the top line is the
    //  same), throw it away without displaying anything.
    //

    if (Success && MoreContext->LinesInViewport > 0 && LinesReturned > 0) {
        if (MoreContext->DisplayViewportLines[0].PhysicalLine ==
            MoreContext->StagingViewportLines[0].PhysicalLine &&

            MoreContext->DisplayViewportLines[0].LogicalLineIndex ==
            MoreContext->StagingViewportLines[0].LogicalLineIndex) {

            for (Index = 0; Index < MoreContext->LinesInViewport; Index++) {
                YoriLibFreeStringContents(&MoreContext->StagingViewportLines[Index].Line);
            }
            LinesReturned = 0;
            Success = FALSE;
        }
    }

    ASSERT(!Success || LinesReturned <= CappedLinesToMove);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (!Success || LinesReturned == 0) {
        return;
    }

    MoreDisplayNewLinesInViewport(MoreContext, MoreContext->StagingViewportLines, LinesReturned);
}

/**
 Find the next search match, meaning any match after the top logical line,
 and advance the viewport to it.  If no further match is found, no update is
 made.

 @param MoreContext Pointer to the more context to search for a match and use
        for the source of any display refresh.
 */
VOID
MoreMoveViewportToNextSearchMatch(
    __inout PMORE_CONTEXT MoreContext
    )
{
    PMORE_PHYSICAL_LINE NextMatch;
    PMORE_LOGICAL_LINE LineToFollow;
    YORI_ALLOC_SIZE_T LinesMoved;

    LineToFollow = NULL;
    if (MoreContext->LinesInViewport > 0) {
        LineToFollow = &MoreContext->DisplayViewportLines[0];
    }

    NextMatch = MoreFindNextLineWithSearchMatch(MoreContext, LineToFollow, TRUE, MoreContext->ViewportHeight, &LinesMoved);
    if (NextMatch == NULL) {
        return;
    }

    if (YoriLibIsSelectionActive(&MoreContext->Selection)) {
        YoriLibClearSelection(&MoreContext->Selection);
        YoriLibRedrawSelection(&MoreContext->Selection);
    }

    if (LinesMoved >= MoreContext->ViewportHeight) {
        MoreContext->LinesInPage = 0;
        MoreGenerateEntireViewportWithStartingLine(MoreContext, NextMatch);
    } else {
        MoreMoveViewportDown(MoreContext, LinesMoved);
    }
}

/**
 Find the previous search match, meaning any match before the top logical line,
 and move the viewport to it.  If no further match is found, no update is
 made.

 @param MoreContext Pointer to the more context to search for a match and use
        for the source of any display refresh.
 */
VOID
MoreMoveViewportToPreviousSearchMatch(
    __inout PMORE_CONTEXT MoreContext
    )
{
    PMORE_PHYSICAL_LINE NextMatch;
    PMORE_LOGICAL_LINE LineToFollow;
    YORI_ALLOC_SIZE_T LinesMoved;

    LineToFollow = NULL;
    if (MoreContext->LinesInViewport > 0) {
        LineToFollow = &MoreContext->DisplayViewportLines[0];
    }

    NextMatch = MoreFindPreviousLineWithSearchMatch(MoreContext, LineToFollow, TRUE, MoreContext->ViewportHeight, &LinesMoved);
    if (NextMatch == NULL) {
        return;
    }

    if (YoriLibIsSelectionActive(&MoreContext->Selection)) {
        YoriLibClearSelection(&MoreContext->Selection);
        YoriLibRedrawSelection(&MoreContext->Selection);
    }

    if (LinesMoved >= MoreContext->ViewportHeight) {
        MoreContext->LinesInPage = 0;
        MoreGenerateEntireViewportWithStartingLine(MoreContext, NextMatch);
    } else {
        MoreMoveViewportUp(MoreContext, LinesMoved);
    }
}

/**
 Move the viewport left, if the buffer is wider than the window.

 @param MoreContext Pointer to the context describing the data to display.

 @param LinesToMove Specifies the number of lines to move down.
 */
VOID
MoreMoveViewportLeft(
    __inout PMORE_CONTEXT MoreContext,
    __in DWORD LinesToMove
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE StdOutHandle;

    UNREFERENCED_PARAMETER(MoreContext);

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

    if (ScreenInfo.srWindow.Left == 0) {
        return;
    }

    if ((DWORD)ScreenInfo.srWindow.Left > LinesToMove) {
        ScreenInfo.srWindow.Left = (SHORT)(ScreenInfo.srWindow.Left - LinesToMove);
        ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.srWindow.Right - LinesToMove);
    } else {
        ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.srWindow.Right - ScreenInfo.srWindow.Left);
        ScreenInfo.srWindow.Left = 0;
    }

    SetConsoleWindowInfo(StdOutHandle, TRUE, &ScreenInfo.srWindow);
}

/**
 Move the viewport right, if the buffer is wider than the window.

 @param MoreContext Pointer to the context describing the data to display.

 @param LinesToMove Specifies the number of lines to move down.
 */
VOID
MoreMoveViewportRight(
    __inout PMORE_CONTEXT MoreContext,
    __in DWORD LinesToMove
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    DWORD LinesLeft;
    HANDLE StdOutHandle;

    UNREFERENCED_PARAMETER(MoreContext);

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

    if (ScreenInfo.srWindow.Right == ScreenInfo.dwSize.X - 1) {
        return;
    }

    LinesLeft = ScreenInfo.dwSize.X - ScreenInfo.srWindow.Right - 1;

    if (LinesLeft > LinesToMove) {
        ScreenInfo.srWindow.Left = (SHORT)(ScreenInfo.srWindow.Left + LinesToMove);
        ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.srWindow.Right + LinesToMove);
    } else {
        ScreenInfo.srWindow.Left = (SHORT)(ScreenInfo.srWindow.Left + LinesLeft);
        ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.srWindow.Right + LinesLeft);
    }

    SetConsoleWindowInfo(StdOutHandle, TRUE, &ScreenInfo.srWindow);
}

/**
 If a selection region is active, copy the region as text to the clipboard.

 @param MoreContext Pointer to the data being displayed by the program and
        information about the region of the data that is currently selected.

 @return TRUE if the region was successfully copied, FALSE if it was not
         copied including if no selection was present.
 */
BOOL
MoreCopySelectionIfPresent(
    __in PMORE_CONTEXT MoreContext
    )
{
    YORI_STRING TextToCopy;
    YORI_STRING VtText;
    YORI_STRING HtmlText;
    YORI_STRING RtfText;
    HANDLE ConsoleHandle;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    PMORE_LOGICAL_LINE EntireLogicalLines;
    PMORE_LOGICAL_LINE CopyLogicalLines;
    PMORE_LOGICAL_LINE StartLine;
    YORI_STRING Subset;
    YORI_ALLOC_SIZE_T LineCount;
    YORI_ALLOC_SIZE_T StartingLineIndex;
    YORI_ALLOC_SIZE_T SingleLineBufferSize;
    YORI_ALLOC_SIZE_T VtTextBufferSize;
    YORI_ALLOC_SIZE_T LineIndex;
    YORI_ALLOC_SIZE_T CharactersRemainingInMatch = 0;
    BOOL Result = FALSE;
    YORI_CONSOLE_SCREEN_BUFFER_INFOEX ScreenInfoEx;
    PDWORD ColorTableToUse = NULL;

    PYORILIB_SELECTION Selection = &MoreContext->Selection;

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

    if (!Selection->SelectionPreviouslyActive &&
        (Selection->CurrentlyDisplayed.Left != Selection->PreviouslyDisplayed.Left ||
         Selection->CurrentlyDisplayed.Right != Selection->PreviouslyDisplayed.Right ||
         Selection->CurrentlyDisplayed.Top != Selection->PreviouslyDisplayed.Top ||
         Selection->CurrentlyDisplayed.Bottom != Selection->PreviouslyDisplayed.Bottom)) {

        YoriLibRedrawSelection(Selection);
    }

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo);

    ASSERT((Selection->CurrentlySelected.Bottom >= ScreenInfo.srWindow.Top && Selection->CurrentlySelected.Bottom <= ScreenInfo.srWindow.Bottom) ||
           (Selection->CurrentlySelected.Top >= ScreenInfo.srWindow.Top && Selection->CurrentlySelected.Top <= ScreenInfo.srWindow.Bottom));


    //
    //  Allocate an array of logical lines covering the number of lines in the
    //  selection.  These will be populated using regular logical line parsing
    //  for the viewport width.  The same allocation also contains an array of
    //  logical lines for the copy region, which is a subset of the entire
    //  logical lines.
    //

    SingleLineBufferSize = (Selection->CurrentlySelected.Bottom - Selection->CurrentlySelected.Top + 1) * sizeof(MORE_LOGICAL_LINE);
    EntireLogicalLines = YoriLibMalloc(SingleLineBufferSize * 2);
    if (EntireLogicalLines == NULL) {
        MoreContext->OutOfMemory = TRUE;
        return FALSE;
    }
    CopyLogicalLines = (PMORE_LOGICAL_LINE)YoriLibAddToPointer(EntireLogicalLines, SingleLineBufferSize);
    ZeroMemory(EntireLogicalLines, SingleLineBufferSize * 2);

    if (Selection->CurrentlySelected.Top >= ScreenInfo.srWindow.Top) {

        //
        //  Take a logical line from the array (can be greater than zero), and
        //  continue parsing forward from there
        //

        StartLine = &MoreContext->DisplayViewportLines[Selection->CurrentlySelected.Top - ScreenInfo.srWindow.Top];
        MoreCloneLogicalLine(EntireLogicalLines, StartLine);

        //
        //  Note that the allocation is for Bottom - Top + 1, and here we only
        //  populate Bottom - Top, because the first line is the starting
        //  line.
        //

        if (!MoreGetNextLogicalLines(MoreContext, StartLine, TRUE, Selection->CurrentlySelected.Bottom - Selection->CurrentlySelected.Top, &EntireLogicalLines[1], &LineCount)) {
            YoriLibFreeStringContents(&EntireLogicalLines[0].Line);
            YoriLibFree(EntireLogicalLines);
            return FALSE;
        }
        LineCount++;
        StartingLineIndex = 0;

    } else if (Selection->CurrentlySelected.Bottom >= ScreenInfo.srWindow.Top) {

        //
        //  Take a logical line from the array (can be greater than zero) and
        //  parse backward to find the start
        //

        StartLine = &MoreContext->DisplayViewportLines[Selection->CurrentlySelected.Bottom - ScreenInfo.srWindow.Top];
        MoreCloneLogicalLine(&EntireLogicalLines[Selection->CurrentlySelected.Bottom - Selection->CurrentlySelected.Top], StartLine);

        //
        //  Note that the allocation is for Bottom - Top + 1, and here we only
        //  populate Bottom - Top, because the final line is the starting
        //  line.
        //

        if (!MoreGetPreviousLogicalLines(MoreContext, StartLine, Selection->CurrentlySelected.Bottom - Selection->CurrentlySelected.Top, EntireLogicalLines, &LineCount)) {
            YoriLibFreeStringContents(&EntireLogicalLines[Selection->CurrentlySelected.Bottom - Selection->CurrentlySelected.Top].Line);
            YoriLibFree(EntireLogicalLines);
            return FALSE;
        }
        StartingLineIndex = Selection->CurrentlySelected.Bottom - Selection->CurrentlySelected.Top - LineCount;
        LineCount++;
    } else {
        YoriLibFree(EntireLogicalLines);
        return FALSE;
    }

    //
    //  For each logical line, need to strip off CurrentlySelected.Left
    //  (saving initial color), then create a new logical line for the
    //  selected text.
    //

    VtTextBufferSize = 0;
    {
        WORD InitialDisplayColor;
        WORD InitialUserColor;
        YORI_ALLOC_SIZE_T LogicalLineLength;
        MORE_LINE_END_CONTEXT LineEndContext;

        for (LineIndex = StartingLineIndex; LineIndex < StartingLineIndex + LineCount; LineIndex++) {
            YoriLibInitEmptyString(&Subset);
            Subset.StartOfString = EntireLogicalLines[LineIndex].Line.StartOfString;
            Subset.LengthInChars = EntireLogicalLines[LineIndex].Line.LengthInChars;
            InitialDisplayColor = EntireLogicalLines[LineIndex].InitialDisplayColor;
            InitialUserColor = EntireLogicalLines[LineIndex].InitialUserColor;
            CharactersRemainingInMatch = EntireLogicalLines[LineIndex].CharactersRemainingInMatch;
            if (Selection->CurrentlySelected.Left > 0) {
                LogicalLineLength = MoreGetLogicalLineLength(MoreContext,
                                                             &Subset,
                                                             Selection->CurrentlySelected.Left,
                                                             InitialDisplayColor,
                                                             InitialUserColor,
                                                             CharactersRemainingInMatch,
                                                             &LineEndContext);
                InitialDisplayColor = LineEndContext.FinalDisplayColor;
                InitialUserColor = LineEndContext.FinalUserColor;
                Subset.LengthInChars = Subset.LengthInChars - LogicalLineLength;
                Subset.StartOfString += LogicalLineLength;
                CharactersRemainingInMatch = LineEndContext.CharactersRemainingInMatch;
            }
            LogicalLineLength = MoreGetLogicalLineLength(MoreContext,
                                                         &Subset,
                                                         Selection->CurrentlySelected.Right - Selection->CurrentlySelected.Left + 1,
                                                         InitialDisplayColor,
                                                         InitialUserColor,
                                                         CharactersRemainingInMatch,
                                                         &LineEndContext);
            CopyLogicalLines[LineIndex].InitialDisplayColor = InitialDisplayColor;
            CopyLogicalLines[LineIndex].InitialUserColor = InitialUserColor;
            CopyLogicalLines[LineIndex].CharactersRemainingInMatch = CharactersRemainingInMatch;
            CopyLogicalLines[LineIndex].Line.StartOfString = Subset.StartOfString;
            CopyLogicalLines[LineIndex].Line.LengthInChars = LogicalLineLength;

            //
            //  We need enough space for the line text, plus CRLF (two chars),
            //  plus the initial color.  We're pessimistic about the initial
            //  color length.  Note that one character is added for CRLF
            //  because the other is the NULL char from sizeof.
            //

            VtTextBufferSize += LogicalLineLength + 1 + YORI_MAX_INTERNAL_VT_ESCAPE_CHARS;
        }
    }

    VtTextBufferSize++;
    YoriLibInitEmptyString(&HtmlText);
    YoriLibInitEmptyString(&RtfText);
    YoriLibInitEmptyString(&VtText);
    YoriLibInitEmptyString(&TextToCopy);
    if (!YoriLibAllocateString(&VtText, VtTextBufferSize)) {
        MoreContext->OutOfMemory = TRUE;
        goto Exit;
    }

    //
    //  For the rich text version, these logical lines need to be concatenated
    //  along with the initial color for each line.
    //

    Subset.StartOfString = VtText.StartOfString;
    Subset.LengthAllocated = VtText.LengthAllocated;
    for (LineIndex = StartingLineIndex; LineIndex < StartingLineIndex + LineCount; LineIndex++) {
        YoriLibVtStringForTextAttribute(&Subset, 0, CopyLogicalLines[LineIndex].InitialDisplayColor);
        VtText.LengthInChars = VtText.LengthInChars + Subset.LengthInChars;
        Subset.StartOfString += Subset.LengthInChars;
        Subset.LengthAllocated = Subset.LengthAllocated - Subset.LengthInChars;
        Subset.LengthInChars = 0;

        YoriLibYPrintf(&Subset, _T("%y\r\n"), &CopyLogicalLines[LineIndex].Line);

        VtText.LengthInChars = VtText.LengthInChars + Subset.LengthInChars;
        Subset.StartOfString += Subset.LengthInChars;
        Subset.LengthAllocated = Subset.LengthAllocated - Subset.LengthInChars;
        Subset.LengthInChars = 0;
    }

    //
    //  For the plain text version, these logical lines need to be copied while
    //  removing any escapes.  Also note this form should not have a trailing
    //  CRLF.
    //

    YoriLibStripVtEscapes(&VtText, &TextToCopy);
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

    if (!YoriLibHtmlConvertToHtmlFromVt(&VtText, &HtmlText, ColorTableToUse, 4)) {
        goto Exit;
    }

    if (!YoriLibRtfConvertToRtfFromVt(&VtText, &RtfText, ColorTableToUse)) {
        goto Exit;
    }

    YoriLibFreeStringContents(&VtText);

    //
    //  Copy HTML, RTF and plain text forms to the clipboard
    //

    if (YoriLibCopyTextRtfAndHtml(&TextToCopy, &RtfText, &HtmlText)) {
        Result = TRUE;
    }

Exit:
    for (LineIndex = StartingLineIndex; LineIndex < StartingLineIndex + LineCount; LineIndex++) {
        YoriLibFreeStringContents(&EntireLogicalLines[LineIndex].Line);
    }
    YoriLibFree(EntireLogicalLines);
    YoriLibFreeStringContents(&HtmlText);
    YoriLibFreeStringContents(&RtfText);
    YoriLibFreeStringContents(&TextToCopy);
    YoriLibFreeStringContents(&VtText);
    return Result;
}

/**
 Return TRUE if there are lines which have already been populated into the
 physical line list that have not yet been displayed on the viewport.

 @param MoreContext Pointer to the context describing the physical lines
        available and the lines displayed in the viewport.

 @return TRUE if there are more lines available to display.
 */
BOOL
MoreAreMoreLinesAvailable(
    __in PMORE_CONTEXT MoreContext
    )
{
    DWORDLONG LastViewportLineNumber;
    DWORDLONG LastPhysicalLineNumber;
    PYORI_LIST_ENTRY ListEntry;
    PMORE_PHYSICAL_LINE LastPhysicalLine;
    PMORE_LOGICAL_LINE LastViewportLine;

    //
    //  If nothing is currently on the display, the assumption is that there
    //  was previously nothing available and the code should wait for more
    //  physical lines.
    //

    if (MoreContext->LinesInViewport == 0) {
        return FALSE;
    }

    LastViewportLine = &MoreContext->DisplayViewportLines[MoreContext->LinesInViewport - 1];

    //
    //  If the last logical line in the viewport indicates there are more
    //  logical lines from the same physical line, then there must be more
    //  contents and the physical line number doesn't matter.
    //

    if (LastViewportLine->MoreLogicalLines) {
        return TRUE;
    }

    // 
    //  If the end of the physical line has been reached, check for the
    //  existence of more physical lines.
    //

    LastViewportLineNumber = LastViewportLine->PhysicalLine->LineNumber;

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);
    ListEntry = YoriLibGetPreviousListEntry(&MoreContext->PhysicalLineList, NULL);
    LastPhysicalLine = CONTAINING_RECORD(ListEntry, MORE_PHYSICAL_LINE, LineList);
    LastPhysicalLineNumber = LastPhysicalLine->LineNumber;
    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (LastPhysicalLineNumber > LastViewportLineNumber) {
        return TRUE;
    }

    return FALSE;
}

/**
 Apply search changes that could affect the set of lines being filtered.
 This applies the new filter, then displays the new set of lines in the
 viewport, and indicates that the status line needs to be redrawn.

 @param MoreContext Pointer to the more context.
 */
VOID
MoreRefreshFilteredLinesDisplay(
    __in PMORE_CONTEXT MoreContext
    )
{
    PMORE_PHYSICAL_LINE NewStart;
    if (MoreContext->LinesInViewport > 0) {
        NewStart = MoreUpdateFilteredLines(MoreContext, MoreContext->DisplayViewportLines[0].PhysicalLine);
    } else {
        NewStart = MoreUpdateFilteredLines(MoreContext, NULL);
    }

    //
    //  A lot of code here isn't robust to LinesInViewport going backwards,
    //  which is easy to do when filtering.  Clearing the screen here solves
    //  this problem, is visually sane, but relies on the assumption that the
    //  user can't be manipulating which lines get displayed and still expect
    //  a complete history of lines in their Terminal scrollback buffer.
    //

    MoreClearScreen(MoreContext);
    MoreContext->LinesInViewport = 0;
    MoreContext->LinesInPage = 0;
    MoreContext->SearchDirty = TRUE;
    MoreGenerateEntireViewportWithStartingLine(MoreContext, NewStart);
}

/**
 Append a string to the current search string.  This is often just appending
 a single character in response to a key press, but may append a buffer from
 the clipboard.

 @param MoreContext Pointer to the more context.

 @param String The string to append.

 @param RepeatCount The number of times to append the string.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MoreAppendToSearchString(
    __in PMORE_CONTEXT MoreContext,
    __in PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T RepeatCount
    )
{
    YORI_ALLOC_SIZE_T SearchIndex;
    DWORD LengthRequired;
    PYORI_STRING SearchString;
    YORI_ALLOC_SIZE_T Count;

    SearchIndex = MoreSearchIndexForColorIndex(MoreContext, MoreContext->SearchColorIndex);
    SearchString = &MoreContext->SearchStrings[SearchIndex];

    LengthRequired = SearchString->LengthInChars;
    LengthRequired = LengthRequired + String->LengthInChars * RepeatCount + 1;

    if (SearchString->LengthAllocated < LengthRequired) {
        DWORD BytesDesired;
        YORI_ALLOC_SIZE_T NewAllocSize;
        BytesDesired = SearchString->LengthAllocated;
        BytesDesired = BytesDesired + 4096;
        if (BytesDesired < LengthRequired) {
            BytesDesired = LengthRequired;
        }
        NewAllocSize = YoriLibMaximumAllocationInRange(LengthRequired, BytesDesired);
        if (NewAllocSize == 0) {
            return FALSE;
        }
        if (!YoriLibReallocateString(SearchString, NewAllocSize)) {
            return FALSE;
        }
    }

    for (Count = 0; Count < RepeatCount; Count++) {
        memcpy(&SearchString->StartOfString[SearchString->LengthInChars],
               String->StartOfString,
               String->LengthInChars * sizeof(TCHAR));

        SearchString->LengthInChars = SearchString->LengthInChars + String->LengthInChars;
    }
    MoreContext->SearchContext[SearchIndex].ColorIndex = MoreContext->SearchColorIndex;
    MoreContext->SearchDirty = TRUE;
    return TRUE;
}

/**
 Process a key that is typically an enhanced key, including arrows, insert,
 delete, home, end, etc.  The "normal" placement of these keys is as enhanced,
 but the original XT keys are the ones on the number pad when num lock is off,
 which have the same key codes but don't have the enhanced bit set.  This
 function is invoked to handle either case.

 @param MoreContext Pointer to the context describing the data to display.

 @param InputRecord Pointer to the structure describing the key that was
        pressed.
 */
VOID
MoreProcessEnhancedKeyDown(
    __inout PMORE_CONTEXT MoreContext,
    __in PINPUT_RECORD InputRecord
    )
{
    WORD KeyCode;
    KeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;

    if (KeyCode == VK_DOWN) {
        MoreMoveViewportDown(MoreContext, 1);
    } else if (KeyCode == VK_UP) {
        MoreMoveViewportUp(MoreContext, 1);
    } else if (KeyCode == VK_LEFT) {
        MoreMoveViewportLeft(MoreContext, 1);
    } else if (KeyCode == VK_RIGHT) {
        MoreMoveViewportRight(MoreContext, 1);
    } else if (KeyCode == VK_NEXT) {
        MoreContext->LinesInPage = 0;
        MoreMoveViewportDown(MoreContext, MoreContext->ViewportHeight);
    } else if (KeyCode == VK_PRIOR) {
        MoreMoveViewportUp(MoreContext, MoreContext->ViewportHeight);
    } else if (KeyCode == VK_HOME) {
        MoreMoveViewportToTop(MoreContext);
    } else if (KeyCode == VK_END) {
        MoreMoveViewportToBottom(MoreContext);
    } else if (KeyCode == VK_F3) {
        MoreMoveViewportToNextSearchMatch(MoreContext);
    } else if (KeyCode == VK_F4) {
        MoreMoveViewportToPreviousSearchMatch(MoreContext);
    }
}

/**
 Process a key that is typically an enhanced key with Ctrl held, including
 home/end.  The "normal" placement of these keys is as enhanced, but the
 original XT keys are the ones on the number pad when num lock is off,
 which have the same key codes but don't have the enhanced bit set.  This
 function is invoked to handle either case.

 @param MoreContext Pointer to the context describing the data to display.

 @param InputRecord Pointer to the structure describing the key that was
        pressed.
 */
VOID
MoreProcessEnhancedCtrlKeyDown(
    __inout PMORE_CONTEXT MoreContext,
    __in PINPUT_RECORD InputRecord
    )
{
    WORD KeyCode;
    KeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;

    if (KeyCode == VK_HOME) {
        MoreMoveViewportToTop(MoreContext);
    } else if (KeyCode == VK_END) {
        MoreMoveViewportToBottom(MoreContext);
    }
}

/**
 Process a key that is typically an enhanced key with Shift held, including
 function keys.  These may be reported as enhanced or not. This function is
 invoked to handle either case.

 @param MoreContext Pointer to the context describing the data to display.

 @param InputRecord Pointer to the structure describing the key that was
        pressed.
 */
VOID
MoreProcessEnhancedShiftKeyDown(
    __inout PMORE_CONTEXT MoreContext,
    __in PINPUT_RECORD InputRecord
    )
{
    WORD KeyCode;
    KeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;

    if (KeyCode == VK_F3) {
        MoreMoveViewportToPreviousSearchMatch(MoreContext);
    }
}

/**
 Perform the requested action when the user presses a key.

 @param MoreContext Pointer to the context describing the data to display.

 @param InputRecord Pointer to the structure describing the key that was
        pressed.

 @param Terminate On successful completion, set to TRUE to indicate the
        program should terminate.

 @param RedrawStatus On successful completion, set to TRUE to indicate that
        caller needs to update the status line because the window area has
        moved.
 */
VOID
MoreProcessKeyDown(
    __inout PMORE_CONTEXT MoreContext,
    __in PINPUT_RECORD InputRecord,
    __inout PBOOL Terminate,
    __inout PBOOL RedrawStatus
    )
{
    DWORD CtrlMask;
    TCHAR Char;
    WORD KeyCode;
    WORD ScanCode;
    BOOL ClearSelection = FALSE;
    PYORI_STRING SearchString;
    YORI_STRING NewString;
    UCHAR SearchIndex;

    *Terminate = FALSE;

    Char = InputRecord->Event.KeyEvent.uChar.UnicodeChar;
    CtrlMask = InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | ENHANCED_KEY | SHIFT_PRESSED);
    KeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;
    ScanCode = InputRecord->Event.KeyEvent.wVirtualScanCode;

    SearchIndex = MoreSearchIndexForColorIndex(MoreContext, MoreContext->SearchColorIndex);

    SearchString = &MoreContext->SearchStrings[SearchIndex];

    if (CtrlMask == 0 || CtrlMask == SHIFT_PRESSED) {
        ClearSelection = TRUE;
        if (MoreContext->SearchUiActive) {
            if (Char == 27) {
                MoreContext->SearchUiActive = FALSE;
                YoriLibFreeStringContents(SearchString);
                MoreSearchIndexFree(MoreContext, SearchIndex);
                if (MoreContext->FilterToSearch) {
                    MoreRefreshFilteredLinesDisplay(MoreContext);
                }
                MoreContext->SearchDirty = TRUE;
            } else if (Char == '\b') {
                if (InputRecord->Event.KeyEvent.wRepeatCount >= SearchString->LengthInChars) {
                    SearchString->LengthInChars = 0;
                    MoreSearchIndexFree(MoreContext, SearchIndex);
                    if (MoreContext->FilterToSearch) {
                        MoreRefreshFilteredLinesDisplay(MoreContext);
                    }
                } else {
                    SearchString->LengthInChars = SearchString->LengthInChars - InputRecord->Event.KeyEvent.wRepeatCount;
                }
                MoreContext->SearchDirty = TRUE;
            } else if (Char == '\r') {
                if (YoriLibIsSelectionActive(&MoreContext->Selection)) {
                    MoreCopySelectionIfPresent(MoreContext);
                } else if (MoreContext->SearchUiActive) {
                    if (MoreContext->FilterToSearch) {
                        MoreRefreshFilteredLinesDisplay(MoreContext);
                    }
                    MoreContext->SearchUiActive = FALSE;
                    MoreContext->SearchDirty = TRUE;
                }
            } else if (Char != '\0' && Char != '\n') {
                YoriLibInitEmptyString(&NewString);
                NewString.StartOfString = &Char;
                NewString.LengthInChars = 1;

                MoreAppendToSearchString(MoreContext, &NewString, InputRecord->Event.KeyEvent.wRepeatCount);
            } else if (Char == '\0') {
                MoreProcessEnhancedKeyDown(MoreContext, InputRecord);
            }
        } else {
            if (Char == 'q' || Char == 'Q' || Char == 27) {
                *Terminate = TRUE;
            } else if (Char == ' ') {
                MoreContext->LinesInPage = 0;
                if (YoriLibIsSelectionActive(&MoreContext->Selection)) {
                    YoriLibClearSelection(&MoreContext->Selection);
                    YoriLibRedrawSelection(&MoreContext->Selection);
                }
                MoreMoveViewportDown(MoreContext, MoreContext->ViewportHeight);
            } else if (Char == 'F' || Char == 'f') {
                if (MoreContext->FilterToSearch) {
                    MoreContext->FilterToSearch = FALSE;
                } else {
                    MoreContext->FilterToSearch = TRUE;
                }
                MoreRefreshFilteredLinesDisplay(MoreContext);
            } else if (Char == '\r') {
                MoreCopySelectionIfPresent(MoreContext);
            } else if (Char == '/') {
                MoreContext->SearchUiActive = TRUE;
                MoreContext->SearchDirty = TRUE;
                *RedrawStatus = TRUE;
            } else if (KeyCode == VK_SCROLL) {
                if (InputRecord->Event.KeyEvent.dwControlKeyState & SCROLLLOCK_ON) {
                    MoreContext->SuspendPagination = TRUE;
                } else {
                    MoreContext->SuspendPagination = FALSE;
                    *RedrawStatus = TRUE;
                }
            } else if (KeyCode == VK_PAUSE) {
                MoreContext->SuspendPagination = FALSE;
                *RedrawStatus = TRUE;
            } else if (Char == '\0') {
                MoreProcessEnhancedKeyDown(MoreContext, InputRecord);
            }
        }
    } else if (CtrlMask == ENHANCED_KEY) {
        ClearSelection = TRUE;
        MoreProcessEnhancedKeyDown(MoreContext, InputRecord);
    } else if (CtrlMask == (ENHANCED_KEY | SHIFT_PRESSED)) {
        ClearSelection = TRUE;
        MoreProcessEnhancedShiftKeyDown(MoreContext, InputRecord);
    } else if (CtrlMask == (ENHANCED_KEY | LEFT_CTRL_PRESSED) ||
               CtrlMask == (ENHANCED_KEY | RIGHT_CTRL_PRESSED)) {
        MoreProcessEnhancedCtrlKeyDown(MoreContext, InputRecord);
    } else if (CtrlMask == RIGHT_CTRL_PRESSED ||
               CtrlMask == LEFT_CTRL_PRESSED) {

        if (KeyCode == 'C') {
            if (YoriLibIsSelectionActive(&MoreContext->Selection)) {
                MoreCopySelectionIfPresent(MoreContext);
            } else {
                *Terminate = TRUE;
            }
        } else if (KeyCode == 'F') {
            if (MoreContext->FilterToSearch) {
                MoreContext->FilterToSearch = FALSE;
            } else {
                MoreContext->FilterToSearch = TRUE;
            }
            MoreRefreshFilteredLinesDisplay(MoreContext);
        } else if (KeyCode == 'Q') {
            MoreContext->SuspendPagination = TRUE;
        } else if (KeyCode == 'S') {
            MoreContext->SuspendPagination = FALSE;
            *RedrawStatus = TRUE;
        } else if (KeyCode == 'V') {
            if (MoreContext->SearchUiActive) {
                YoriLibInitEmptyString(&NewString);
                if (YoriLibPasteText(&NewString)) {
                    MoreAppendToSearchString(MoreContext, &NewString, InputRecord->Event.KeyEvent.wRepeatCount);
                    YoriLibFreeStringContents(&NewString);
                }
            }
        } else if (KeyCode >= '1' && KeyCode <= '9') {
            MoreContext->SearchUiActive = TRUE;
            MoreContext->SearchColorIndex = (UCHAR)(KeyCode - '1');
            MoreContext->SearchDirty = TRUE;
        } else if (Char == '\0') {
            MoreProcessEnhancedCtrlKeyDown(MoreContext, InputRecord);
        }
    }

    if (ClearSelection &&
        KeyCode != VK_SHIFT &&
        KeyCode != VK_CONTROL) {

        if (YoriLibIsSelectionActive(&MoreContext->Selection)) {
            YoriLibClearSelection(&MoreContext->Selection);
            YoriLibRedrawSelection(&MoreContext->Selection);
        }
    }

    return;
}

/**
 Reallocate display buffers and regenerate display if the window has changed
 in size.

 @param MoreContext Pointer to the current state of the data and display.
 */
VOID
MoreProcessResizeViewport(
    __inout PMORE_CONTEXT MoreContext
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE StdOutHandle;
    PMORE_LOGICAL_LINE NewDisplayViewportLines;
    PMORE_LOGICAL_LINE NewStagingViewportLines;
    PMORE_PHYSICAL_LINE FirstPhysicalLine;
    YORI_ALLOC_SIZE_T NewViewportHeight;
    YORI_ALLOC_SIZE_T NewViewportWidth;
    YORI_ALLOC_SIZE_T OldLinesInViewport;
    YORI_ALLOC_SIZE_T Index;
    PMORE_LOGICAL_LINE OldDisplayViewportLines;
    PMORE_LOGICAL_LINE OldStagingViewportLines;

    if (YoriLibIsSelectionActive(&MoreContext->Selection)) {
        YoriLibClearSelection(&MoreContext->Selection);
        YoriLibRedrawSelection(&MoreContext->Selection);
    }

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

    NewViewportHeight = ScreenInfo.srWindow.Bottom - ScreenInfo.srWindow.Top;
    MoreGetViewportDimensions(&ScreenInfo, &NewViewportWidth, &NewViewportHeight);

    if (!MoreAllocateViewportStructures(NewViewportWidth, NewViewportHeight, &NewDisplayViewportLines, &NewStagingViewportLines)) {
        return;
    }

    OldLinesInViewport = MoreContext->LinesInViewport;
    OldStagingViewportLines = MoreContext->StagingViewportLines;
    OldDisplayViewportLines = MoreContext->DisplayViewportLines;

    if (NewViewportWidth == MoreContext->ViewportWidth) {
        if (NewViewportHeight > MoreContext->ViewportHeight) {
            memcpy(NewDisplayViewportLines, MoreContext->DisplayViewportLines, MoreContext->ViewportHeight * sizeof(MORE_LOGICAL_LINE));
            MoreContext->ViewportHeight = NewViewportHeight;
            MoreContext->DisplayViewportLines = NewDisplayViewportLines;
            MoreContext->StagingViewportLines = NewStagingViewportLines;
            MoreClearStatusLine(MoreContext);
            MoreAddNewLinesToViewport(MoreContext);
        } else {
            COORD NewCursorPosition;
            SMALL_RECT NewWindow;
            DWORD NumberWritten;

            MoreClearStatusLine(MoreContext);
            NewCursorPosition.X = 0;
            NewCursorPosition.Y = (USHORT)((DWORD)ScreenInfo.dwCursorPosition.Y + NewViewportHeight - MoreContext->ViewportHeight);

            NewWindow.Left = 0;
            NewWindow.Right = (USHORT)(ScreenInfo.srWindow.Right - ScreenInfo.srWindow.Left + 1);
            NewWindow.Top = (USHORT)(NewCursorPosition.Y - NewViewportHeight);
            NewWindow.Bottom = NewCursorPosition.Y;

            memcpy(NewDisplayViewportLines, MoreContext->DisplayViewportLines, NewViewportHeight * sizeof(MORE_LOGICAL_LINE));

            if (OldLinesInViewport > NewViewportHeight) {
                for (Index = NewViewportHeight; Index < OldLinesInViewport; Index++) {
                    YoriLibFreeStringContents(&OldDisplayViewportLines[Index].Line);
                }
                FillConsoleOutputCharacter(StdOutHandle, ' ', ScreenInfo.dwSize.X * (OldLinesInViewport - NewViewportHeight + 1), NewCursorPosition, &NumberWritten);
                FillConsoleOutputAttribute(StdOutHandle,
                                           YoriLibVtGetDefaultColor(),
                                           ScreenInfo.dwSize.X * (OldLinesInViewport - NewViewportHeight + 1),
                                           NewCursorPosition,
                                           &NumberWritten);
            }

            MoreContext->ViewportHeight = NewViewportHeight;
            MoreContext->DisplayViewportLines = NewDisplayViewportLines;
            MoreContext->StagingViewportLines = NewStagingViewportLines;

            if (MoreContext->LinesInViewport > MoreContext->ViewportHeight) {
                MoreContext->LinesInViewport = MoreContext->ViewportHeight;
            }

            if (MoreContext->LinesInPage > MoreContext->ViewportHeight) {
                MoreContext->LinesInPage = MoreContext->ViewportHeight;
            }
            SetConsoleCursorPosition(StdOutHandle, NewCursorPosition);
            MoreClearStatusLine(MoreContext);
            MoreDrawStatusLine(MoreContext);
            SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &NewWindow);
        }
    } else {
        FirstPhysicalLine = NULL;
        if (MoreContext->LinesInViewport > 0) {
            FirstPhysicalLine = MoreContext->DisplayViewportLines[0].PhysicalLine;
        }

        MoreContext->LinesInPage = 0;
        MoreContext->LinesInViewport = 0;
        MoreContext->DisplayViewportLines = NewDisplayViewportLines;
        MoreContext->StagingViewportLines = NewStagingViewportLines;
        MoreContext->ViewportHeight = NewViewportHeight;
        MoreContext->ViewportWidth = ScreenInfo.srWindow.Right - ScreenInfo.srWindow.Left + 1;

        MoreRegenerateViewport(MoreContext, FirstPhysicalLine);

        for (Index = 0; Index < OldLinesInViewport; Index++) {
            YoriLibFreeStringContents(&OldDisplayViewportLines[Index].Line);
        }
    }

    YoriLibFree(OldStagingViewportLines);
    YoriLibFree(OldDisplayViewportLines);
}

/**
 Check if the number of lines in the window has changed.  The console has
 buffer size but not window size notifications, so this is effectively polled.
 If the size has changed, recalculate the viewport for the new display.

 @param MoreContext Pointer to the more context specifying the data and number
        of lines available.
 */
VOID
MoreCheckForWindowSizeChange(
    __inout PMORE_CONTEXT MoreContext
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE StdOutHandle;
    YORI_ALLOC_SIZE_T NewViewportWidth;
    YORI_ALLOC_SIZE_T NewViewportHeight;

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

    MoreGetViewportDimensions(&ScreenInfo, &NewViewportWidth, &NewViewportHeight);

    if (NewViewportHeight != MoreContext->ViewportHeight ||
        NewViewportWidth != MoreContext->ViewportWidth) {

        MoreProcessResizeViewport(MoreContext);
    }
}

/**
 Check if the number of lines available has changed since the status line was
 last drawn.  If it has, clear the status line and redraw it.

 @param MoreContext Pointer to the more context specifying the data and number
        of lines available.
 */
VOID
MoreCheckForStatusLineChange(
    __inout PMORE_CONTEXT MoreContext
    )
{
    if (MoreContext->TotalLinesInViewportStatus != MoreContext->FilteredLineCount || MoreContext->SearchDirty) {
        MoreClearStatusLine(MoreContext);
        MoreDrawStatusLine(MoreContext);
    }
}

/**
 Periodically update the selection by scrolling.  This occurs when the mouse
 button is held down and the mouse pointer is outside the console window,
 indicating that the console window should be updated to contain new contents
 in the direction of the mouse cursor.

 @param MoreContext Pointer to the more context including the selection to
        update.

 @return TRUE if the window was scrolled, FALSE if it was not.
 */
BOOL
MorePeriodicScrollForSelection(
    __inout PMORE_CONTEXT MoreContext
    )
{
    HANDLE ConsoleHandle;
    SHORT CellsToScroll;
    DWORD LinesMoved;
    CONSOLE_SCREEN_BUFFER_INFO StartScreenInfo;
    CONSOLE_SCREEN_BUFFER_INFO EndScreenInfo;
    PYORILIB_SELECTION Selection = &MoreContext->Selection;

    if (Selection->PeriodicScrollAmount.Y == 0 &&
        Selection->PeriodicScrollAmount.X == 0) {

        return FALSE;
    }

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(ConsoleHandle, &StartScreenInfo)) {
        return FALSE;
    }

    memcpy(&EndScreenInfo, &StartScreenInfo, sizeof(CONSOLE_SCREEN_BUFFER_INFO));

    //
    //  MSFIX Need to truncate the selection if it's scrolling off the
    //  buffer.  This is what happens if we scroll down until the top of
    //  the actual (not displayed) selection would be less than zero, or
    //  if we scroll up until the actual (not displayed) selection would
    //  be greater than buffer height.
    //

    if (Selection->PeriodicScrollAmount.Y < 0) {
        CellsToScroll = (SHORT)(0 - Selection->PeriodicScrollAmount.Y);
        if (MoreContext->DebugDisplay) {
            YoriLibClearPreviousSelectionDisplay(Selection);
        }
        LinesMoved = MoreMoveViewportUp(MoreContext, CellsToScroll);
        if (LinesMoved > 0) {
            YoriLibNotifyScrollBufferMoved(Selection, (SHORT)LinesMoved);
        }
        if (MoreContext->DebugDisplay) {
            YoriLibDrawCurrentSelectionDisplay(Selection);
        }
        ASSERT(MoreContext->Selection.CurrentlySelected.Top >= StartScreenInfo.srWindow.Top);
    } else if (Selection->PeriodicScrollAmount.Y > 0) {
        CellsToScroll = Selection->PeriodicScrollAmount.Y;
        if (MoreContext->DebugDisplay) {
            YoriLibClearPreviousSelectionDisplay(Selection);
        }
        LinesMoved = MoreMoveViewportDown(MoreContext, CellsToScroll);
        if (MoreContext->DebugDisplay) {
            if (LinesMoved > 0) {
                SHORT SignedLinesMoved = (SHORT)(0 - LinesMoved);
                YoriLibNotifyScrollBufferMoved(Selection, SignedLinesMoved);
            }
            YoriLibDrawCurrentSelectionDisplay(Selection);
        } else {
            if (!GetConsoleScreenBufferInfo(ConsoleHandle, &EndScreenInfo)) {
                return FALSE;
            }

            //
            //  If moving the viewport down moved the window within a screen
            //  buffer, then all coordinates known to the selection remain
            //  correct.  If it scrolled data within the buffer, then all offsets
            //  have changed and the selection needs to be updated to reflect
            //  the new coordinates.  Calculate this by taking the lines we output
            //  and subtracting any window movement from it; anything remaining is
            //  the data in the buffer moving.
            //

            if (LinesMoved > 0) {
                SHORT SignedLinesMoved = (SHORT)(LinesMoved - (EndScreenInfo.srWindow.Bottom - StartScreenInfo.srWindow.Bottom));
                if (SignedLinesMoved > 0) {
                    SignedLinesMoved = (SHORT)(0 - SignedLinesMoved);
                    YoriLibNotifyScrollBufferMoved(Selection, SignedLinesMoved);
                }
            }
        }
    }

    if (Selection->PeriodicScrollAmount.X < 0) {
        CellsToScroll = (SHORT)(0 - Selection->PeriodicScrollAmount.X);
        if (EndScreenInfo.srWindow.Left > 0) {
            if (EndScreenInfo.srWindow.Left > CellsToScroll) {
                EndScreenInfo.srWindow.Left = (SHORT)(EndScreenInfo.srWindow.Left - CellsToScroll);
                EndScreenInfo.srWindow.Right = (SHORT)(EndScreenInfo.srWindow.Right - CellsToScroll);
            } else {
                EndScreenInfo.srWindow.Right = (SHORT)(EndScreenInfo.srWindow.Right - EndScreenInfo.srWindow.Left);
                EndScreenInfo.srWindow.Left = 0;
            }
        }
    } else if (Selection->PeriodicScrollAmount.X > 0) {
        CellsToScroll = Selection->PeriodicScrollAmount.X;
        if (EndScreenInfo.srWindow.Right < EndScreenInfo.dwSize.X - 1) {
            if (EndScreenInfo.srWindow.Right < EndScreenInfo.dwSize.X - CellsToScroll - 1) {
                EndScreenInfo.srWindow.Left = (SHORT)(EndScreenInfo.srWindow.Left + CellsToScroll);
                EndScreenInfo.srWindow.Right = (SHORT)(EndScreenInfo.srWindow.Right + CellsToScroll);
            } else {
                EndScreenInfo.srWindow.Left = (SHORT)(EndScreenInfo.srWindow.Left + (EndScreenInfo.dwSize.X - EndScreenInfo.srWindow.Right - 1));
                EndScreenInfo.srWindow.Right = (SHORT)(EndScreenInfo.dwSize.X - 1);
            }
        }
    }

    SetConsoleWindowInfo(ConsoleHandle, TRUE, &EndScreenInfo.srWindow);

    return TRUE;
}


/**
 Perform processing related to when a mouse button is pressed.

 @param MoreContext Pointer to the more context which describes the current
        selection.

 @param InputRecord Pointer to the console input event.

 @param ButtonsPressed A bit mask of buttons that were just pressed.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
MoreProcessMouseButtonDown(
    __inout PMORE_CONTEXT MoreContext,
    __in PINPUT_RECORD InputRecord,
    __in DWORD ButtonsPressed,
    __inout PBOOL TerminateInput
    )
{
    BOOL BufferChanged = FALSE;

    UNREFERENCED_PARAMETER(TerminateInput);

    if (ButtonsPressed & FROM_LEFT_1ST_BUTTON_PRESSED) {
        BufferChanged = YoriLibCreateSelectionFromPoint(&MoreContext->Selection,
                                                        InputRecord->Event.MouseEvent.dwMousePosition.X,
                                                        InputRecord->Event.MouseEvent.dwMousePosition.Y);

    } else if (ButtonsPressed & RIGHTMOST_BUTTON_PRESSED) {
        if (YoriLibIsSelectionActive(&MoreContext->Selection)) {
            BufferChanged = MoreCopySelectionIfPresent(MoreContext);
            if (BufferChanged) {
                YoriLibClearSelection(&MoreContext->Selection);
            }
        } else if (MoreContext->SearchUiActive) {
            YORI_STRING NewString;
            YoriLibInitEmptyString(&NewString);
            if (YoriLibPasteText(&NewString)) {
                MoreAppendToSearchString(MoreContext, &NewString, 1);
                YoriLibFreeStringContents(&NewString);
                BufferChanged = TRUE;
            }
        }
    }

    return BufferChanged;
}

/**
 Perform processing related to when a mouse button is released.

 @param MoreContext Pointer to the more context which describes the current
        selection.

 @param InputRecord Pointer to the console input event.

 @param ButtonsReleased A bit mask of buttons that were just released.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
MoreProcessMouseButtonUp(
    __inout PMORE_CONTEXT MoreContext,
    __in PINPUT_RECORD InputRecord,
    __in DWORD ButtonsReleased,
    __inout PBOOL TerminateInput
    )
{
    UNREFERENCED_PARAMETER(InputRecord);
    UNREFERENCED_PARAMETER(TerminateInput);

    //
    //  If the left mouse button was released and periodic scrolling was in
    //  effect, stop it now.
    //

    if (ButtonsReleased & FROM_LEFT_1ST_BUTTON_PRESSED) {
        YoriLibClearPeriodicScroll(&MoreContext->Selection);
    }

    return FALSE;
}


/**
 Perform processing related to when a mouse is double clicked.

 @param MoreContext Pointer to the more context which describes the current
        selection.

 @param InputRecord Pointer to the console input event.

 @param ButtonsPressed A bit mask of buttons that were just pressed.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
MoreProcessMouseDoubleClick(
    __inout PMORE_CONTEXT MoreContext,
    __in PINPUT_RECORD InputRecord,
    __in DWORD ButtonsPressed,
    __inout PBOOL TerminateInput
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
        YORI_STRING BreakChars;

        if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
            return FALSE;
        }
        YoriLibGetSelectionDoubleClickBreakChars(&BreakChars);

        BufferChanged = YoriLibClearSelection(&MoreContext->Selection);

        ReadChar = ' ';
        ReadPoint.Y = InputRecord->Event.MouseEvent.dwMousePosition.Y;
        ReadPoint.X = InputRecord->Event.MouseEvent.dwMousePosition.X;

        //
        //  If the user double clicked on a break char, do nothing.
        //

        ReadConsoleOutputCharacter(ConsoleHandle, &ReadChar, 1, ReadPoint, &CharsRead);
        if (YoriLibFindLeftMostCharacter(&BreakChars, ReadChar) != NULL) {
            YoriLibFreeStringContents(&BreakChars);
            return FALSE;
        }

        //
        //  Nagivate left to find beginning of line or next break char.
        //

        for (StartOffset = InputRecord->Event.MouseEvent.dwMousePosition.X; StartOffset > 0; StartOffset--) {
            ReadPoint.X = (SHORT)(StartOffset - 1);
            ReadConsoleOutputCharacter(ConsoleHandle, &ReadChar, 1, ReadPoint, &CharsRead);
            if (YoriLibFindLeftMostCharacter(&BreakChars, ReadChar) != NULL) {
                break;
            }
        }

        //
        //  Navigate right to find end of line or next break char.
        //

        for (EndOffset = InputRecord->Event.MouseEvent.dwMousePosition.X; EndOffset < ScreenInfo.dwSize.X - 1; EndOffset++) {
            ReadPoint.X = (SHORT)(EndOffset + 1);
            ReadConsoleOutputCharacter(ConsoleHandle, &ReadChar, 1, ReadPoint, &CharsRead);
            if (YoriLibFindLeftMostCharacter(&BreakChars, ReadChar) != NULL) {
                break;
            }
        }

        YoriLibCreateSelectionFromRange(&MoreContext->Selection, StartOffset, ReadPoint.Y, EndOffset, ReadPoint.Y);

        BufferChanged = TRUE;
        YoriLibFreeStringContents(&BreakChars);
    }

    return BufferChanged;
}

/**
 Perform processing related to a mouse move event.

 @param MoreContext Pointer to the more context which describes the current
        selection.

 @param InputRecord Pointer to the console input event.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @param RedrawStatus On successful completion, set to TRUE to indicate that
        caller needs to update the status line because the window area has
        moved.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
MoreProcessMouseMove(
    __inout PMORE_CONTEXT MoreContext,
    __in PINPUT_RECORD InputRecord,
    __inout PBOOL TerminateInput,
    __inout PBOOL RedrawStatus
    )
{
    HANDLE ConsoleHandle;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    UNREFERENCED_PARAMETER(TerminateInput);

    if (InputRecord->Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {

        ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
            return FALSE;
        }

        YoriLibUpdateSelectionToPoint(&MoreContext->Selection,
                                      InputRecord->Event.MouseEvent.dwMousePosition.X,
                                      InputRecord->Event.MouseEvent.dwMousePosition.Y);

        if (MoreContext->Selection.CurrentlyDisplayed.Bottom >= ScreenInfo.srWindow.Bottom) {
            MoreContext->Selection.CurrentlyDisplayed.Bottom = (SHORT)(ScreenInfo.srWindow.Bottom - 1);
        }

        if (MoreContext->Selection.PreviouslyDisplayed.Bottom >= ScreenInfo.srWindow.Bottom) {
            *RedrawStatus = TRUE;
        }

        if (MorePeriodicScrollForSelection(MoreContext)) {
            *RedrawStatus = TRUE;
        }

        return TRUE;
    }

    return FALSE;
}

/**
 Perform processing related to when a mouse wheel is scrolled.

 @param MoreContext Pointer to the more context which describes the current
        selection.

 @param InputRecord Pointer to the console input event.

 @param ButtonsPressed A bit mask of buttons that were just pressed.

 @param TerminateInput On successful completion, set to TRUE to indicate that
        the input sequence is complete and should be returned to the caller.

 @return TRUE to indicate the input buffer has changed and needs to be
         redisplayed.
 */
BOOL
MoreProcessMouseScroll(
    __inout PMORE_CONTEXT MoreContext,
    __in PINPUT_RECORD InputRecord,
    __in DWORD ButtonsPressed,
    __inout PBOOL TerminateInput
    )
{
    HANDLE ConsoleHandle;
    SHORT Direction;
    SHORT LinesToScroll;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    UNREFERENCED_PARAMETER(MoreContext);
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
 Manage the console display of the more application.

 @param MoreContext Pointer to the context describing the data to display.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
BOOL
MoreViewportDisplay(
    __inout PMORE_CONTEXT MoreContext
    )
{
    HANDLE ObjectsToWaitFor[4];
    HANDLE InHandle;
    DWORD WaitObject;
    DWORD HandleCountToWait;
    DWORD PreviousMouseButtonState = 0;
    DWORD Timeout;
    DWORD InputFlags;
    BOOL WaitForIngestThread = TRUE;
    BOOL WaitForNewLines = TRUE;

    InHandle = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (InHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    //  If YoriQuickEdit is enabled, set the extended flags, which indicates
    //  an intention to clear the console's quickedit functionality.  Note
    //  that we have no way to know about the previous value and cannot
    //  accurately restore it or other extended flags.
    //

    if (YoriLibIsYoriQuickEditEnabled()) {
        InputFlags = ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
        YoriLibSetInputConsoleMode(InHandle, InputFlags);
    } else {
        InputFlags = ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT;
        YoriLibSetInputConsoleModeWithoutExtended(InHandle, InputFlags);
    }

    if (YoriLibIsNanoServer()) {

        //
        //  The nano console implicitly wraps at line end whether we like it
        //  or not.  On regular consoles, this is bad because on auto wrap the
        //  attributes of the next line are set to match the previous cell,
        //  but on Nano this is benign since no background colors are
        //  supported anyway.  On regular consoles, resize causes text to
        //  move lines without an explicit newline, but the Nano console is
        //  incapable of resize anyway.
        //

        MoreContext->AutoWrapAtLineEnd = TRUE;
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);

    } else {

        //
        //  A better way to read this is "disable ENABLE_WRAP_AT_EOL_OUTPUT"
        //  which is the default.  This program must emit explicit newlines
        //  after each viewport line.
        //

        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT);
    }

    while(TRUE) {

        //
        //  If the viewport is full, we don't care about new lines being
        //  ingested.
        //

        if (MoreContext->LinesInPage == MoreContext->ViewportHeight &&
            !MoreContext->SuspendPagination) {
            WaitForNewLines = FALSE;
        } else {
            WaitForNewLines = TRUE;
        }

        HandleCountToWait = 0;
        ObjectsToWaitFor[HandleCountToWait++] = InHandle;
        ObjectsToWaitFor[HandleCountToWait++] = YoriLibCancelGetEvent();
        if (WaitForNewLines) {
            ObjectsToWaitFor[HandleCountToWait++] = MoreContext->PhysicalLineAvailableEvent;
        }
        if (WaitForIngestThread) {
            ObjectsToWaitFor[HandleCountToWait++] = MoreContext->IngestThread;
        }

        if (YoriLibIsPeriodicScrollActive(&MoreContext->Selection)) {
            Timeout = 100;
        } else {
            Timeout = 250;
        }

        //
        //  If pagination is suspended, and lines are available to display,
        //  perform the wait with a zero timeout.  This means if input
        //  events are available they will be processed, but if not, more
        //  lines will be displayed.  If the ingest thread has terminated
        //  and pagination is suspended and there are no more contents,
        //  the process is complete.
        //

        if (MoreContext->SuspendPagination) {
            if (MoreAreMoreLinesAvailable(MoreContext)) {
                Timeout = 0;
            } else if (!WaitForIngestThread) {
                break;
            }
        }

        WaitObject = WaitForMultipleObjectsEx(HandleCountToWait, ObjectsToWaitFor, FALSE, Timeout, FALSE);

        //
        //  If the ingest thread has died due to failure, we have incomplete
        //  results, and likely will hit more errors or bad behavior if we
        //  try to continue.  Try to die as gracefully as possible.
        //

        if (MoreContext->OutOfMemory) {
            MoreClearStatusLine(MoreContext);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Out of memory ingesting or displaying data\n"));
            break;
        }

        if (WaitObject == WAIT_TIMEOUT) {
            if (YoriLibIsPeriodicScrollActive(&MoreContext->Selection)) {
                MorePeriodicScrollForSelection(MoreContext);
                MoreCheckForWindowSizeChange(MoreContext);
                MoreCheckForStatusLineChange(MoreContext);
            } else if (MoreContext->SuspendPagination && MoreAreMoreLinesAvailable(MoreContext)) {
                MoreAddNewLinesToViewport(MoreContext);
            } else {
                MoreCheckForWindowSizeChange(MoreContext);
                MoreCheckForStatusLineChange(MoreContext);
            }
        } else {
            if (WaitObject >= WAIT_OBJECT_0 + HandleCountToWait) {
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
                    if (MoreContext->LinesInPage < MoreContext->ViewportHeight) {
                        break;
                    }
                }
            } else if (ObjectsToWaitFor[WaitObject - WAIT_OBJECT_0] == YoriLibCancelGetEvent()) {
                MoreClearStatusLine(MoreContext);
                break;
            } else if (ObjectsToWaitFor[WaitObject - WAIT_OBJECT_0] == InHandle) {
                INPUT_RECORD InputRecords[20];
                PINPUT_RECORD InputRecord;
                DWORD ActuallyRead;
                DWORD CurrentIndex;
                BOOL Terminate = FALSE;
                BOOL RedrawStatus = FALSE;

                if (!ReadConsoleInput(InHandle, InputRecords, sizeof(InputRecords)/sizeof(InputRecords[0]), &ActuallyRead)) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Failed to read input, terminating\n"));
                    break;
                }

                MoreCheckForWindowSizeChange(MoreContext);

                for (CurrentIndex = 0; CurrentIndex < ActuallyRead; CurrentIndex++) {
                    InputRecord = &InputRecords[CurrentIndex];
                    if (InputRecord->EventType == KEY_EVENT &&
                        InputRecord->Event.KeyEvent.bKeyDown) {

                        MoreProcessKeyDown(MoreContext, InputRecord, &Terminate, &RedrawStatus);
                        if (MoreContext->SearchDirty) {
                            MoreDisplayChangedLinesInViewport(MoreContext);
                            RedrawStatus = TRUE;
                        }
                        if (RedrawStatus) {
                            MoreClearStatusLine(MoreContext);
                            YoriLibRedrawSelection(&MoreContext->Selection);
                            MoreDrawStatusLine(MoreContext);
                        }
                    } else if (InputRecord->EventType == MOUSE_EVENT) {

                        DWORD ButtonsPressed = InputRecord->Event.MouseEvent.dwButtonState - (PreviousMouseButtonState & InputRecord->Event.MouseEvent.dwButtonState);
                        DWORD ButtonsReleased = PreviousMouseButtonState - (PreviousMouseButtonState & InputRecord->Event.MouseEvent.dwButtonState);
                        BOOL ReDisplayRequired = FALSE;

                        if (ButtonsReleased > 0) {
                            ReDisplayRequired |= MoreProcessMouseButtonUp(MoreContext, InputRecord, ButtonsReleased, &Terminate);
                        }

                        if (ButtonsPressed > 0) {
                            ReDisplayRequired |= MoreProcessMouseButtonDown(MoreContext, InputRecord, ButtonsPressed, &Terminate);
                        }

                        PreviousMouseButtonState = InputRecord->Event.MouseEvent.dwButtonState;
                        if (InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_MOVED) {
                            ReDisplayRequired |= MoreProcessMouseMove(MoreContext, InputRecord, &Terminate, &RedrawStatus);
                        }

                        if (InputRecord->Event.MouseEvent.dwEventFlags & DOUBLE_CLICK) {
                            ReDisplayRequired |= MoreProcessMouseDoubleClick(MoreContext, InputRecord, ButtonsPressed, &Terminate);
                        }

                        if (InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_WHEELED) {
                            ReDisplayRequired |= MoreProcessMouseScroll(MoreContext, InputRecord, ButtonsPressed, &Terminate);
                        }

                        if (ReDisplayRequired) {
                            if (MoreContext->SearchDirty) {
                                MoreDisplayChangedLinesInViewport(MoreContext);
                                RedrawStatus = TRUE;
                            }
                            if (RedrawStatus) {
                                MoreClearStatusLine(MoreContext);
                                YoriLibRedrawSelection(&MoreContext->Selection);
                                MoreDrawStatusLine(MoreContext);
                            } else {
                                YoriLibRedrawSelection(&MoreContext->Selection);
                            }
                        }

                    } else if (InputRecord->EventType == WINDOW_BUFFER_SIZE_EVENT) {
                        MoreProcessResizeViewport(MoreContext);
                    }
                }

                if (Terminate) {
                    MoreClearStatusLine(MoreContext);
                    break;
                }
            }
        }
    }

    YoriLibCleanupSelection(&MoreContext->Selection);
    CloseHandle(InHandle);
    return TRUE;
}

// vim:sw=4:ts=4:et:
