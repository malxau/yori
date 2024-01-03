/**
 * @file more/lines.c
 *
 * Yori shell more search and split lines
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
 Check if any search string exists.  Because these need to be stored in a
 compacted form, if the first search string is populated, one exists; if not,
 none exist.

 @param MoreContext Pointer to the more context, indicating the search
        strings.

 @return TRUE if any search is active, FALSE if it is not.
 */
BOOLEAN
MoreIsAnySearchActive(
    __in PMORE_CONTEXT MoreContext
    )
{
    //
    //  Because search strings need to be kept packed so there's a single
    //  array of search strings to apply to lines, if the first one has
    //  a search string, a search is active.
    //

    if (MoreContext->SearchStrings[0].LengthInChars > 0) {
        ASSERT(MoreContext->SearchContext[0].ColorIndex != (UCHAR)-1);
        return TRUE;
    }

    ASSERT(MoreContext->SearchContext[0].ColorIndex == (UCHAR)-1);
    return FALSE;
}

/**
 Return the number of active search strings.

 @param MoreContext Pointer to the more context containing the search strings.

 @return The number of active search strings.
 */
UCHAR
MoreSearchCountActive(
    __in PMORE_CONTEXT MoreContext
    )
{
    UCHAR CountFound;

    for (CountFound = 0; CountFound < MORE_MAX_SEARCHES; CountFound++) {
        if (MoreContext->SearchContext[CountFound].ColorIndex == (UCHAR)-1) {
            break;
        }
    }

    return CountFound;
}

/**
 Find the next search match within a physical line.

 @param MoreContext Pointer to the context indicating the strings to search
        for.

 @param StringToSearch Pointer to the string to search within, which is
        typically a physical line or subset of one.

 @param MatchOffset Optionally points to a value to update on successful
        completion indicating the offset within StringToSearch where a match
        was found.

 @param MatchIndex Optionally points to a value to update on successful
        completion indicating which matching string was located.

 @return TRUE to indicate a search match was found, FALSE if it was not.
 */
__success(return)
BOOLEAN
MoreFindNextSearchMatch(
    __in PMORE_CONTEXT MoreContext,
    __in PCYORI_STRING StringToSearch,
    __out_opt PYORI_ALLOC_SIZE_T MatchOffset,
    __out_opt PUCHAR MatchIndex
    )
{
    PYORI_STRING Found;
    UCHAR Index;
    UCHAR CountFound;

    CountFound = MoreSearchCountActive(MoreContext);

    Found = YoriLibFindFirstMatchingSubstringInsensitive(StringToSearch, CountFound, MoreContext->SearchStrings, MatchOffset);
    if (Found != NULL) {
        if (MatchIndex != NULL) {

            for (Index = 0; Index < CountFound; Index++) {
                if (Found == &MoreContext->SearchStrings[Index]) {
                    *MatchIndex = Index;
                    break;
                }
            }

            //
            //  Found has to be one of the input strings, but the analyzer
            //  doesn't know that, so give it a nudge
            //


            __analysis_assume(Index < CountFound);
        }

        return TRUE;
    }

    return FALSE;
}

/**
 Find a search index for a specified color index.  The user indicates the
 color index to apply via Ctrl+n key combinations.  That means when the user
 specifies a new color index, we have to find a compacted slot to use for the
 string for that color.  This function looks up an existing entry for the
 specified color, or can allocate a new slot if none are in use.  The caller
 is expected to set the color in the SearchContext if it updates the
 allocated string, which records its color assignment; this is done to ensure
 that a string doesn't need to be re-freed if the string is not updated.

 @param MoreContext Pointer to the more context specifying the search strings
        and colors.

 @param ColorIndex Specifies the color to look up.

 @return An index within the compacted array of search strings to use for
         this color.  One is guaranteed to exist since we have as many slots
         as colors.
 */
UCHAR
MoreSearchIndexForColorIndex(
    __in PMORE_CONTEXT MoreContext,
    __in UCHAR ColorIndex
    )
{
    UCHAR Index;

    for (Index = 0; Index < MORE_MAX_SEARCHES; Index++) {
        if (MoreContext->SearchContext[Index].ColorIndex == ColorIndex ||
            MoreContext->SearchContext[Index].ColorIndex == (UCHAR)-1) {

            return Index;
        }
    }

    //
    //  There are as many slots for concurrent searches as colors.  There
    //  should either be an entry for each color or an empty slot to use
    //  for one.
    //

    ASSERT(FALSE);
    return 0;
}

/**
 Free a search entry corresponding to a specific index.  All of the active
 search terms need to be next to each other to be efficient, so this means
 moving any later entries that are in use into the position that was
 occupied by the entry being freed.

 @param MoreContext Pointer to the more context including search strings and
        settings.

 @param SearchIndex The index of the entry to free.
 */
VOID
MoreSearchIndexFree(
    __in PMORE_CONTEXT MoreContext,
    __in UCHAR SearchIndex
    )
{
    UCHAR Index;

    //
    //  If it's already free, we're done
    //

    if (MoreContext->SearchContext[SearchIndex].ColorIndex == (UCHAR)-1) {
        return;
    }

    //
    //  Free the string allocation
    //

    YoriLibFreeStringContents(&MoreContext->SearchStrings[SearchIndex]);

    //
    //  Compact any later search strings by moving them down into the newly
    //  emptied slot
    //

    Index = SearchIndex;
    while (Index + 1 < MORE_MAX_SEARCHES &&
           MoreContext->SearchContext[Index + 1].ColorIndex != (UCHAR)-1) {

        memcpy(&MoreContext->SearchStrings[Index],
               &MoreContext->SearchStrings[Index + 1],
               sizeof(YORI_STRING));
        MoreContext->SearchContext[Index].ColorIndex = MoreContext->SearchContext[Index + 1].ColorIndex;
        Index++;
    }

    //
    //  Empty the new final slot (which may be the one we just freed above.)
    //

    YoriLibInitEmptyString(&MoreContext->SearchStrings[Index]);
    MoreContext->SearchContext[Index].ColorIndex = (UCHAR)-1;
}

/**
 Truncate a string such that it contains the specified number of visible
 characters, ie., so that it fits in a defined space.  This is frustratingly
 forked from the main section of code which also has to consider highlighting
 search terms.

 @param String Pointer to the string.

 @param VisibleChars Specifies the number of characters that should be
        displayed.
 */
VOID
MoreTruncateStringToVisibleChars(
    __in PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T VisibleChars
    )
{
    YORI_STRING EscapeSubset;
    YORI_ALLOC_SIZE_T EndOfEscape;
    YORI_ALLOC_SIZE_T VisibleCharsFound;
    YORI_ALLOC_SIZE_T Index;

    VisibleCharsFound = 0;

    for (Index = 0; Index < String->LengthInChars; Index++) {

        //
        //  If the string is <ESC>[, then treat it as an escape sequence.
        //  Look for the final letter after any numbers or semicolon.
        //

        if (String->LengthInChars > Index + 2 &&
            String->StartOfString[Index] == 27 &&
            String->StartOfString[Index + 1] == '[') {

            YoriLibInitEmptyString(&EscapeSubset);
            EscapeSubset.StartOfString = &String->StartOfString[Index + 2];
            EscapeSubset.LengthInChars = String->LengthInChars - Index - 2;
            EndOfEscape = YoriLibCountStringContainingChars(&EscapeSubset, _T("0123456789;"));
            //
            //  Note the trailing char is a letter.  This points index to that
            //  char.
            //

            Index = Index + 2 + EndOfEscape;
        } else if (VisibleCharsFound < VisibleChars) {
            VisibleCharsFound++;
        } else {
            String->LengthInChars = Index;
            break;
        }
    }
}

/**
 Return the next filtered physical line.  This refers to a physical line that
 matches the search criteria when filtering is enabled.  If filtering is not
 in effect, this is the same as getting the next physical line.

 @param MoreContext Pointer to the more context.

 @param PreviousLine Optionally points to a previous physical line, where this
        function should return the next line.  If not specified, the first
        filtered physical line is returned.

 @return Pointer to the next physical line, or NULL if no more physical lines
         are present.
 */
PMORE_PHYSICAL_LINE
MoreGetNextFilteredPhysicalLine(
    __in PMORE_CONTEXT MoreContext,
    __in_opt PMORE_PHYSICAL_LINE PreviousLine
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PMORE_PHYSICAL_LINE ThisLine;

    if (PreviousLine != NULL) {
        ListEntry = YoriLibGetNextListEntry(&MoreContext->FilteredPhysicalLineList, &PreviousLine->FilteredLineList);
    } else {
        ListEntry = YoriLibGetNextListEntry(&MoreContext->FilteredPhysicalLineList, NULL);
    }

    if (ListEntry == NULL) {
        return NULL;
    }

    ThisLine = CONTAINING_RECORD(ListEntry, MORE_PHYSICAL_LINE, FilteredLineList);

    //
    //  Check that the list is sorted
    //

    ASSERT(PreviousLine == NULL || ThisLine->FilteredLineNumber == PreviousLine->FilteredLineNumber + 1);
    ASSERT(PreviousLine == NULL || ThisLine->LineNumber > PreviousLine->LineNumber);

    return ThisLine;
}

/**
 Return the previous filtered physical line.  This refers to a physical line
 that matches the search criteria when filtering is enabled.  If filtering is
 not in effect, this is the same as getting the previous physical line.

 @param MoreContext Pointer to the more context.

 @param NextLine Optionally points to a next physical line, where this
        function should return the previous line.  If not specified, the
        final filtered physical line is returned.

 @return Pointer to the previous physical line, or NULL if no more physical
         lines are present.
 */
PMORE_PHYSICAL_LINE
MoreGetPreviousFilteredPhysicalLine(
    __in PMORE_CONTEXT MoreContext,
    __in_opt PMORE_PHYSICAL_LINE NextLine
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PMORE_PHYSICAL_LINE ThisLine;

    if (NextLine != NULL) {
        ListEntry = YoriLibGetPreviousListEntry(&MoreContext->FilteredPhysicalLineList, &NextLine->FilteredLineList);
    } else {
        ListEntry = YoriLibGetPreviousListEntry(&MoreContext->FilteredPhysicalLineList, NULL);
    }

    if (ListEntry == NULL) {
        return NULL;
    }

    ThisLine = CONTAINING_RECORD(ListEntry, MORE_PHYSICAL_LINE, FilteredLineList);

    //
    //  Check that the list is sorted
    //

    ASSERT(NextLine == NULL || ThisLine->FilteredLineNumber + 1 == NextLine->FilteredLineNumber);
    ASSERT(NextLine == NULL || ThisLine->LineNumber < NextLine->LineNumber);
    return ThisLine;
}

/**
 Apply a new search criteria to update the set of filtered lines.

 MSFIX This routine wants to be much smarter.  Ideally it would initiate an
 asynchronous process that gets synchronized when next/previous lines are
 needed.  This would allow the display to rapidly update while the number of
 filtered lines is updated in the background (refreshing the status line.)

 @param MoreContext Pointer to the more context, indicating the current search
        terms.

 @param PreviousStartPoint Optionally points to a physical line which is
        currently displayed.  If specified, this function attempts to return
        a "good" physical line to display once the new filter has been
        applied.

 @return Pointer to a physical line which should be used to display after the
         filter has been updated.
 */
PMORE_PHYSICAL_LINE
MoreUpdateFilteredLines(
    __in PMORE_CONTEXT MoreContext,
    __in_opt PMORE_PHYSICAL_LINE PreviousStartPoint
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PMORE_PHYSICAL_LINE PreviousFilteredLine;
    PMORE_PHYSICAL_LINE ThisLine;
    BOOLEAN MatchFound;
    DWORDLONG FilteredLineNumber;
    DWORDLONG PreviousStartLineNumber;
    PMORE_PHYSICAL_LINE NewStartPoint;

    PreviousStartLineNumber = 0;
    NewStartPoint = NULL;
    if (PreviousStartPoint != NULL) {
        PreviousStartLineNumber = PreviousStartPoint->LineNumber;
    }

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    PreviousFilteredLine = NULL;
    FilteredLineNumber = 0;
    ListEntry = YoriLibGetNextListEntry(&MoreContext->PhysicalLineList, NULL);

    while (ListEntry != NULL) {

        ThisLine = CONTAINING_RECORD(ListEntry, MORE_PHYSICAL_LINE, LineList);
        if (MoreContext->FilterToSearch) {
            MatchFound = MoreFindNextSearchMatch(MoreContext, &ThisLine->LineContents, NULL, NULL);
        } else {
            MatchFound = TRUE;
        }

        if (!MatchFound) {
            if (ThisLine->FilteredLineList.Next != NULL) {
                ASSERT(MoreContext->FilteredLineCount > 0);
                MoreContext->FilteredLineCount--;
                YoriLibRemoveListItem(&ThisLine->FilteredLineList);
                ThisLine->FilteredLineList.Next = NULL;
            }
        } else {
            if (ThisLine->FilteredLineList.Next == NULL) {
                if (PreviousFilteredLine != NULL) {
                    ASSERT(ThisLine->LineNumber > PreviousFilteredLine->LineNumber);
                    YoriLibInsertList(&PreviousFilteredLine->FilteredLineList, &ThisLine->FilteredLineList);
                } else {
                    YoriLibInsertList(&MoreContext->FilteredPhysicalLineList, &ThisLine->FilteredLineList);
                }
                MoreContext->FilteredLineCount++;
                ASSERT(MoreContext->FilteredLineCount <= MoreContext->LineCount);
            }
            FilteredLineNumber++;
            ThisLine->FilteredLineNumber = FilteredLineNumber;
            PreviousFilteredLine = ThisLine;
            if (NewStartPoint == NULL && ThisLine->LineNumber >= PreviousStartLineNumber) {
                NewStartPoint = ThisLine;
            }
        }


        ListEntry = YoriLibGetNextListEntry(&MoreContext->PhysicalLineList, &ThisLine->LineList);
    }

    ASSERT(MoreContext->FilteredLineCount == FilteredLineNumber);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    return NewStartPoint;
}


/**
 Return the number of characters within a subset of a physical line which
 will form a logical line.  Conceptually this represents either the minimum
 of the length of the string or the viewport width.  In practice it can be
 a little more convoluted due to nonprinting characters.

 @param MoreContext Pointer to the more context.

 @param PhysicalLineSubset Pointer to a string which represents either a
        complete physical line, or a trailing portion of a physical line after
        other logical lines have already been constructed from the previous
        characters.

 @param MaximumVisibleCharacters The number of visible characters to preserve
        in the logical line.  Note the logical line may contain fewer
        characters due to insufficient data or more characters, so long as
        they are escapes that are not visible.

 @param InitialDisplayColor The starting color to display, in Win32 form.

 @param InitialUserColor The starting color according to the previous input
        stream.  This may be different to the display color if the display
        is being altered by this program to do things like highlight search
        matches.

 @param CharactersRemainingInMatch Specifies the number of characters at the
        beginning of the logical line which consist of a match against a
        search that commenced on a previous logical line.  If zero, no match
        is spanning the two lines.

 @param LineEndContext Optionally points to a context block to be populated
        with information describing parsing state at the end of a logical
        line.  This is used when generating multiple logical lines.  It is
        not needed to count logical lines without generating them.

 @return The number of characters to consume from the physical line buffer
         that will be part of the next logical line.  Note this is not
         necessarily the same as the number of characters allocated into the
         logical line, which may contain extra information if
         RequiresGeneration is set.
 */
YORI_ALLOC_SIZE_T
MoreGetLogicalLineLength(
    __in PMORE_CONTEXT MoreContext,
    __in PYORI_STRING PhysicalLineSubset,
    __in YORI_ALLOC_SIZE_T MaximumVisibleCharacters,
    __in WORD InitialDisplayColor,
    __in WORD InitialUserColor,
    __in YORI_ALLOC_SIZE_T CharactersRemainingInMatch,
    __out_opt PMORE_LINE_END_CONTEXT LineEndContext
    )
{
    YORI_ALLOC_SIZE_T SourceIndex;
    YORI_ALLOC_SIZE_T CharsInOutputBuffer;
    YORI_ALLOC_SIZE_T CellsDisplayed;
    YORI_STRING EscapeSubset;
    YORI_ALLOC_SIZE_T EndOfEscape;
    WORD CurrentColor = InitialDisplayColor;
    WORD CurrentUserColor = InitialUserColor;
    YORI_STRING MatchEscapeChars;
    TCHAR MatchEscapeCharsBuf[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];
    YORI_ALLOC_SIZE_T MatchOffset;
    YORI_ALLOC_SIZE_T MatchLength;
    BOOLEAN MatchFound;
    WORD SearchColor;
    UCHAR MatchIndex;

    CharsInOutputBuffer = 0;
    CellsDisplayed = 0;

    if (LineEndContext != NULL) {
        LineEndContext->ExplicitNewlineRequired = TRUE;
        LineEndContext->RequiresGeneration = FALSE;
    }

    SearchColor = 0;

    MatchOffset = 0;
    MatchLength = 0;
    YoriLibInitEmptyString(&MatchEscapeChars);
    MatchEscapeChars.StartOfString = MatchEscapeCharsBuf;
    MatchEscapeChars.LengthAllocated = sizeof(MatchEscapeCharsBuf)/sizeof(MatchEscapeCharsBuf[0]);
    if (CharactersRemainingInMatch > 0) {
        MatchFound = TRUE;
        MatchLength = CharactersRemainingInMatch;
    } else {
        MatchFound = MoreIsAnySearchActive(MoreContext);
    }

    for (SourceIndex = 0; SourceIndex < PhysicalLineSubset->LengthInChars; ) {

        //
        //  Look to see if the string contains another search match from this
        //  offset.  On the first character we're doing this check if the
        //  search string is not empty.
        //

        if (MatchFound &&
            SourceIndex >= MatchOffset + MatchLength) {

            YORI_STRING StringForNextMatch;

            YoriLibInitEmptyString(&StringForNextMatch);
            StringForNextMatch.StartOfString = &PhysicalLineSubset->StartOfString[SourceIndex];
            StringForNextMatch.LengthInChars = PhysicalLineSubset->LengthInChars - SourceIndex;
            MatchFound = MoreFindNextSearchMatch(MoreContext, &StringForNextMatch, &MatchOffset, &MatchIndex);
            if (MatchFound) {
                MatchLength = MoreContext->SearchStrings[MatchIndex].LengthInChars;
                SearchColor = MoreContext->SearchColors[MoreContext->SearchContext[MatchIndex].ColorIndex];
                MatchOffset = MatchOffset + SourceIndex;
            }
        }

        //
        //  If the string is <ESC>[, then treat it as an escape sequence.
        //  Look for the final letter after any numbers or semicolon.
        //

        if (PhysicalLineSubset->LengthInChars > SourceIndex + 2 &&
            PhysicalLineSubset->StartOfString[SourceIndex] == 27 &&
            PhysicalLineSubset->StartOfString[SourceIndex + 1] == '[') {

            YoriLibInitEmptyString(&EscapeSubset);
            EscapeSubset.StartOfString = &PhysicalLineSubset->StartOfString[SourceIndex + 2];
            EscapeSubset.LengthInChars = PhysicalLineSubset->LengthInChars - SourceIndex - 2;
            EndOfEscape = YoriLibCountStringContainingChars(&EscapeSubset, _T("0123456789;"));

            //
            //  Count everything as consuming the source and needing buffer
            //  space in the destination but consuming no display cells.  This
            //  may include the final letter, if we found one.
            //

            if (PhysicalLineSubset->LengthInChars > SourceIndex + 2 + EndOfEscape) {
                CharsInOutputBuffer += 3 + EndOfEscape;
                SourceIndex += 3 + EndOfEscape;

                EscapeSubset.StartOfString -= 2;
                EscapeSubset.LengthInChars = EndOfEscape + 3;
                YoriLibVtFinalColorFromSequence(CurrentUserColor, &EscapeSubset, &CurrentUserColor);
                if (!MatchFound || SourceIndex < MatchOffset) {
                    CurrentColor = CurrentUserColor;
                }
            } else {
                CharsInOutputBuffer += 2 + EndOfEscape;
                SourceIndex += 2 + EndOfEscape;
            }
        } else {
            if (MatchFound) {
                if (MatchOffset == SourceIndex) {
                    if (LineEndContext != NULL) {
                        LineEndContext->RequiresGeneration = TRUE;
                    }
                    YoriLibVtStringForTextAttribute(&MatchEscapeChars, 0, SearchColor);
                    CharsInOutputBuffer = CharsInOutputBuffer + MatchEscapeChars.LengthInChars;
                    CurrentColor = SearchColor;
                }
            }
            CharsInOutputBuffer++;
            CellsDisplayed++;
            SourceIndex++;
        }

        if (MatchFound) {
            if (MatchOffset + MatchLength <= SourceIndex) {
                if (LineEndContext != NULL) {
                    LineEndContext->RequiresGeneration = TRUE;
                }
                YoriLibVtStringForTextAttribute(&MatchEscapeChars, 0, CurrentUserColor);
                CharsInOutputBuffer = CharsInOutputBuffer + MatchEscapeChars.LengthInChars;
                CurrentColor = CurrentUserColor;
            }
        }

        ASSERT(CellsDisplayed <= MaximumVisibleCharacters);
        if (CellsDisplayed == MaximumVisibleCharacters) {

            //
            //  If the line is full of text but the next text is an escape
            //  sequence, keep processing despite the line being full.
            //

            if (PhysicalLineSubset->LengthInChars <= SourceIndex + 2 ||
                PhysicalLineSubset->StartOfString[SourceIndex] != 27 ||
                PhysicalLineSubset->StartOfString[SourceIndex + 1] != '[') {

                if (LineEndContext != NULL && MoreContext->AutoWrapAtLineEnd) {
                    LineEndContext->ExplicitNewlineRequired = FALSE;
                }
                break;
            }
        }
    }

    //
    //  When the search criteria spans logical lines, we need to indicate how
    //  many visible characters are remaining until the search ends, not just
    //  that the display color is different
    //

    if (LineEndContext != NULL) {
        LineEndContext->FinalDisplayColor = CurrentColor;
        LineEndContext->FinalUserColor = CurrentUserColor;
        LineEndContext->CharactersNeededInAllocation = CharsInOutputBuffer;
        if (MatchFound) {
            if (MatchOffset < SourceIndex) {
                LineEndContext->CharactersRemainingInMatch = (MatchOffset + MatchLength - SourceIndex);
            } else {
                LineEndContext->CharactersRemainingInMatch = 0;
            }
        } else {
            LineEndContext->CharactersRemainingInMatch = 0;
        }
    }

    return SourceIndex;
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
YORI_ALLOC_SIZE_T
MoreCountLogicalLinesOnPhysicalLine(
    __in PMORE_CONTEXT MoreContext,
    __in PMORE_PHYSICAL_LINE PhysicalLine
    )
{
    YORI_ALLOC_SIZE_T Count = 0;
    YORI_ALLOC_SIZE_T LogicalLineLength;
    YORI_STRING Subset;

    YoriLibInitEmptyString(&Subset);
    Subset.StartOfString = PhysicalLine->LineContents.StartOfString;
    Subset.LengthInChars = PhysicalLine->LineContents.LengthInChars;
    while(TRUE) {
        LogicalLineLength = MoreGetLogicalLineLength(MoreContext, &Subset, MoreContext->ViewportWidth, 0, 0, 0, NULL);
        Subset.StartOfString += LogicalLineLength;
        Subset.LengthInChars = Subset.LengthInChars - LogicalLineLength;
        Count++;
        if (Subset.LengthInChars == 0) {
            break;
        }
    }

    return Count;
}

/**
 Move a logical line from one memory location to another.  Logical lines
 are referenced, so the move implies dereferencing anything being overwritten,
 and transferring the data with its existing reference, zeroing out the
 source as it should no longer be dereferenced.

 @param Dest Pointer to the destination of the move.

 @param Src Pointer to the source of the move.
 */
VOID
MoreMoveLogicalLine(
    __inout PMORE_LOGICAL_LINE Dest,
    __in PMORE_LOGICAL_LINE Src
    )
{
    ASSERT(Dest != Src);
    if (Dest->Line.MemoryToFree != NULL) {
        YoriLibFreeStringContents(&Dest->Line);
    }
    memcpy(Dest, Src, sizeof(MORE_LOGICAL_LINE));
    ZeroMemory(Src, sizeof(MORE_LOGICAL_LINE));
}

/**
 Copy a logical line to a new logical line by referencing the memory.  This
 assumes that both logical lines are immutable.

 @param Dest On successful completion, updated to contain a new logical line
        referencing the same memory as the source.

 @param Src Pointer to the source logical line to copy from.
 */
VOID
MoreCloneLogicalLine(
    __inout PMORE_LOGICAL_LINE Dest,
    __in PMORE_LOGICAL_LINE Src
    )
{
    ASSERT(Dest != Src);
    if (Dest->Line.MemoryToFree != NULL) {
        YoriLibFreeStringContents(&Dest->Line);
    }
    memcpy(Dest, Src, sizeof(MORE_LOGICAL_LINE));
    if (Dest->Line.MemoryToFree != NULL) {
        YoriLibReference(Dest->Line.MemoryToFree);
    }
}

/**
 Copy a string from a physical line into a logical line.  Typically this
 just references the same memory and creates a substring pointing within it.
 If RegenerationRequired is set, the logical line is explicitly allocated
 seperately to the physical line and its contents are manually constructed.

 @param MoreContext Pointer to the more context.

 @param LogicalLine Pointer to the logical line, describing its state but
        not yet containing the string representation of the logical line.

 @param RegenerationRequired TRUE if the logical line string needs to be
        allocated and generated manually.  FALSE if the logical line will
        just be a referenced substring from the physical line.

 @param SourceCharsToConsume The number of characters to consume from the
        physical line, which may not match the number of characters in the
        logical line if RegenerationRequired is TRUE.

 @param AllocationLengthRequired The number of characters needed in the
        logical line, which is not the same as SourceCharsToConsume if
        RegenerationRequired is true.
 */
BOOLEAN
MoreCopyRangeIntoLogicalLine(
    __in PMORE_CONTEXT MoreContext,
    __in PMORE_LOGICAL_LINE LogicalLine,
    __in BOOLEAN RegenerationRequired,
    __in YORI_ALLOC_SIZE_T SourceCharsToConsume,
    __in YORI_ALLOC_SIZE_T AllocationLengthRequired
    )
{
    ASSERT(LogicalLine->Line.LengthAllocated == 0 && LogicalLine->Line.MemoryToFree == NULL);

    if (RegenerationRequired) {
        YORI_STRING PhysicalLineSubset;
        YORI_ALLOC_SIZE_T SourceIndex;
        YORI_ALLOC_SIZE_T CharsInOutputBuffer;
        YORI_STRING MatchEscapeChars;
        YORI_STRING EscapeSubset;
        YORI_ALLOC_SIZE_T EndOfEscape;
        TCHAR MatchEscapeCharsBuf[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];
        YORI_ALLOC_SIZE_T MatchOffset;
        YORI_ALLOC_SIZE_T MatchLength;
        BOOLEAN MatchFound;
        WORD CurrentUserColor = LogicalLine->InitialUserColor;
        WORD SearchColor;
        UCHAR MatchIndex;

        YoriLibInitEmptyString(&PhysicalLineSubset);
        PhysicalLineSubset.StartOfString = &LogicalLine->PhysicalLine->LineContents.StartOfString[LogicalLine->PhysicalLineCharacterOffset]; 
        PhysicalLineSubset.LengthInChars = SourceCharsToConsume;

        if (!YoriLibAllocateString(&LogicalLine->Line, AllocationLengthRequired)) {
            MoreContext->OutOfMemory = TRUE;
            return FALSE;
        }

        CharsInOutputBuffer = 0;

        SearchColor = 0;
        MatchOffset = 0;
        MatchLength = 0;
        YoriLibInitEmptyString(&MatchEscapeChars);
        MatchEscapeChars.StartOfString = MatchEscapeCharsBuf;
        MatchEscapeChars.LengthAllocated = sizeof(MatchEscapeCharsBuf)/sizeof(MatchEscapeCharsBuf[0]);
        if (LogicalLine->CharactersRemainingInMatch > 0) {
            MatchFound = TRUE;
            MatchLength = LogicalLine->CharactersRemainingInMatch;
        } else {
            MatchFound = MoreIsAnySearchActive(MoreContext);
        }

        for (SourceIndex = 0; SourceIndex < PhysicalLineSubset.LengthInChars; ) {

            //
            //  Look to see if the string contains another search match from this
            //  offset.  On the first character we're doing this check if the
            //  search string is not empty.
            //

            if (MatchFound &&
                SourceIndex == MatchOffset + MatchLength) {

                YORI_STRING StringForNextMatch;

                YoriLibInitEmptyString(&StringForNextMatch);
                StringForNextMatch.StartOfString = &PhysicalLineSubset.StartOfString[SourceIndex];
                StringForNextMatch.LengthInChars = LogicalLine->PhysicalLine->LineContents.LengthInChars - LogicalLine->PhysicalLineCharacterOffset - SourceIndex;
                MatchFound = MoreFindNextSearchMatch(MoreContext, &StringForNextMatch, &MatchOffset, &MatchIndex);
                if (MatchFound) {
                    MatchLength = MoreContext->SearchStrings[MatchIndex].LengthInChars;
                    SearchColor = MoreContext->SearchColors[MoreContext->SearchContext[MatchIndex].ColorIndex];
                    MatchOffset = MatchOffset + SourceIndex;
                }
            }

            //
            //  If the string is <ESC>[, then treat it as an escape sequence.
            //  Look for the final letter after any numbers or semicolon.
            //

            if (PhysicalLineSubset.LengthInChars > SourceIndex + 2 &&
                PhysicalLineSubset.StartOfString[SourceIndex] == 27 &&
                PhysicalLineSubset.StartOfString[SourceIndex + 1] == '[') {

                YoriLibInitEmptyString(&EscapeSubset);
                EscapeSubset.StartOfString = &PhysicalLineSubset.StartOfString[SourceIndex + 2];
                EscapeSubset.LengthInChars = PhysicalLineSubset.LengthInChars - SourceIndex - 2;
                EndOfEscape = YoriLibCountStringContainingChars(&EscapeSubset, _T("0123456789;"));

                //
                //  Count everything as consuming the source and needing buffer
                //  space in the destination but consuming no display cells.  This
                //  may include the final letter, if we found one.
                //

                if (PhysicalLineSubset.LengthInChars > SourceIndex + 2 + EndOfEscape) {
                    memcpy(&LogicalLine->Line.StartOfString[CharsInOutputBuffer], &PhysicalLineSubset.StartOfString[SourceIndex], (3 + EndOfEscape) * sizeof(TCHAR));
                    CharsInOutputBuffer += 3 + EndOfEscape;
                    SourceIndex += 3 + EndOfEscape;

                    EscapeSubset.StartOfString -= 2;
                    EscapeSubset.LengthInChars = EndOfEscape + 3;
                    YoriLibVtFinalColorFromSequence(CurrentUserColor, &EscapeSubset, &CurrentUserColor);
                } else {
                    memcpy(&LogicalLine->Line.StartOfString[CharsInOutputBuffer], &PhysicalLineSubset.StartOfString[SourceIndex], (2 + EndOfEscape) * sizeof(TCHAR));
                    CharsInOutputBuffer += 2 + EndOfEscape;
                    SourceIndex += 2 + EndOfEscape;
                }
            } else {

                if (MatchFound) {
                    if (MatchOffset == SourceIndex) {
                        YoriLibVtStringForTextAttribute(&MatchEscapeChars, 0, SearchColor);
                        memcpy(&LogicalLine->Line.StartOfString[CharsInOutputBuffer], MatchEscapeChars.StartOfString, MatchEscapeChars.LengthInChars * sizeof(TCHAR));
                        CharsInOutputBuffer = CharsInOutputBuffer + MatchEscapeChars.LengthInChars;
                    }
                }

                LogicalLine->Line.StartOfString[CharsInOutputBuffer] = PhysicalLineSubset.StartOfString[SourceIndex];
                CharsInOutputBuffer++;
                SourceIndex++;
            }

            ASSERT(CharsInOutputBuffer <= LogicalLine->Line.LengthAllocated);

            if (MatchFound) {

                if (MatchOffset + MatchLength <= SourceIndex) {
                    YoriLibVtStringForTextAttribute(&MatchEscapeChars, 0, CurrentUserColor);
                    memcpy(&LogicalLine->Line.StartOfString[CharsInOutputBuffer], MatchEscapeChars.StartOfString, MatchEscapeChars.LengthInChars * sizeof(TCHAR));
                    CharsInOutputBuffer = CharsInOutputBuffer + MatchEscapeChars.LengthInChars;
                }
            }
        }

        LogicalLine->Line.LengthInChars = CharsInOutputBuffer;
    } else {
        ASSERT(SourceCharsToConsume == AllocationLengthRequired);
        YoriLibInitEmptyString(&LogicalLine->Line);
        LogicalLine->Line.StartOfString = &LogicalLine->PhysicalLine->LineContents.StartOfString[LogicalLine->PhysicalLineCharacterOffset];
        LogicalLine->Line.LengthInChars = SourceCharsToConsume;

        YoriLibReference(LogicalLine->PhysicalLine->MemoryToFree);
        LogicalLine->Line.MemoryToFree = LogicalLine->PhysicalLine->MemoryToFree;
    }

    return TRUE;
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
BOOLEAN
MoreGenerateLogicalLinesFromPhysicalLine(
    __in PMORE_CONTEXT MoreContext,
    __in PMORE_PHYSICAL_LINE PhysicalLine,
    __in YORI_ALLOC_SIZE_T FirstLogicalLineIndex,
    __in YORI_ALLOC_SIZE_T NumberLogicalLines,
    __out_ecount(NumberLogicalLines) PMORE_LOGICAL_LINE OutputLines
    )
{
    YORI_ALLOC_SIZE_T Count = 0;
    YORI_ALLOC_SIZE_T CharIndex = 0;
    YORI_ALLOC_SIZE_T LogicalLineLength;
    YORI_ALLOC_SIZE_T CharactersRemainingInMatch = 0;
    YORI_STRING Subset;
    PMORE_LOGICAL_LINE ThisLine;
    WORD InitialUserColor = PhysicalLine->InitialColor;
    WORD InitialDisplayColor = PhysicalLine->InitialColor;
    MORE_LINE_END_CONTEXT LineEndContext;

    YoriLibInitEmptyString(&Subset);
    Subset.StartOfString = PhysicalLine->LineContents.StartOfString;
    Subset.LengthInChars = PhysicalLine->LineContents.LengthInChars;
    while(TRUE) {
        if (Count >= FirstLogicalLineIndex + NumberLogicalLines) {
            break;
        }
        LogicalLineLength = MoreGetLogicalLineLength(MoreContext, &Subset, MoreContext->ViewportWidth, InitialDisplayColor, InitialUserColor, CharactersRemainingInMatch, &LineEndContext);
        if (Count >= FirstLogicalLineIndex) {
            ThisLine = &OutputLines[Count - FirstLogicalLineIndex];
            ThisLine->PhysicalLine = PhysicalLine;
            ThisLine->InitialUserColor = InitialUserColor;
            ThisLine->InitialDisplayColor = InitialDisplayColor;
            ThisLine->CharactersRemainingInMatch = CharactersRemainingInMatch;
            ThisLine->LogicalLineIndex = Count;
            ThisLine->PhysicalLineCharacterOffset = CharIndex;
            if (LogicalLineLength >= Subset.LengthInChars) {
                ThisLine->MoreLogicalLines = FALSE;
            } else {
                ThisLine->MoreLogicalLines = TRUE;
            }
            ThisLine->ExplicitNewlineRequired = LineEndContext.ExplicitNewlineRequired;

            ASSERT(ThisLine->CharactersRemainingInMatch == 0 || ThisLine->InitialUserColor != ThisLine->InitialDisplayColor);

            if (!MoreCopyRangeIntoLogicalLine(MoreContext, ThisLine, LineEndContext.RequiresGeneration, LogicalLineLength, LineEndContext.CharactersNeededInAllocation)) {
                return FALSE;
            }

            CharactersRemainingInMatch = LineEndContext.CharactersRemainingInMatch;
        }

        InitialUserColor = LineEndContext.FinalUserColor;
        InitialDisplayColor = LineEndContext.FinalDisplayColor;
        Subset.StartOfString += LogicalLineLength;
        Subset.LengthInChars = Subset.LengthInChars - LogicalLineLength;
        Count++;
        CharIndex = CharIndex + LogicalLineLength;
        if (Subset.LengthInChars == 0) {
            break;
        }
    }

    return TRUE;
}


/**
 Return the previous set of logical lines preceeding a previous logical line.

 @param MoreContext Pointer to the more context specifying the data to
        display.

 @param CurrentLine Optionally points to the current line that the previous
        lines should be returned for. If NULL, start from the last line in
        the buffer.

 @param LinesToOutput Specifies the number of lines to output.

 @param OutputLines An array of logical lines with LinesToOutput elements.  On
        successful completion, populated with the data for those logical lines.

 @param NumberLinesGenerated On successful completion, populated with the
        number of logical lines generated.  This can be less than LinesToOutput
        if there is no more buffer.  When this occurs, the buffer is populated
        from last-to-first, so the first entries are the ones missing.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
MoreGetPreviousLogicalLines(
    __inout PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE CurrentLine,
    __in YORI_ALLOC_SIZE_T LinesToOutput,
    __out_ecount(*NumberLinesGenerated) PMORE_LOGICAL_LINE OutputLines,
    __out PYORI_ALLOC_SIZE_T NumberLinesGenerated
    )
{
    YORI_ALLOC_SIZE_T LinesRemaining = LinesToOutput;
    YORI_ALLOC_SIZE_T LinesToCopy;
    YORI_ALLOC_SIZE_T LineIndexToCopy;
    PMORE_LOGICAL_LINE CurrentOutputLine;
    PMORE_LOGICAL_LINE CurrentInputLine;
    BOOLEAN Result = TRUE;

    CurrentInputLine = CurrentLine;

    //
    //  This routine wants to find earlier logical lines.  If the current
    //  logical line is partway through a physical line, go find the logical
    //  lines before this one on that physical line.
    //

    if (CurrentInputLine != NULL && CurrentInputLine->LogicalLineIndex > 0) {
        if (CurrentInputLine->LogicalLineIndex > LinesRemaining) {
            LinesToCopy = LinesRemaining;
            LineIndexToCopy = CurrentInputLine->LogicalLineIndex - LinesToCopy;
        } else {
            LinesToCopy = CurrentInputLine->LogicalLineIndex;
            LineIndexToCopy = 0;
        }
        CurrentOutputLine = &OutputLines[LinesRemaining - LinesToCopy];
        if (!MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                      CurrentInputLine->PhysicalLine,
                                                      LineIndexToCopy,
                                                      LinesToCopy,
                                                      CurrentOutputLine)) {

            Result = FALSE;
        }
        LinesRemaining = LinesRemaining - LinesToCopy;
    }

    //
    //  If there are still more logical lines to get, walk backwards one
    //  physical line at a time and see how many logical lines it contains,
    //  filling the output buffer from each until it's full.  If a physical
    //  line has more logical lines than the number needed, get the
    //  final logical lines from it.
    //

    while(Result && LinesRemaining > 0) {
        PMORE_PHYSICAL_LINE PreviousPhysicalLine;
        YORI_ALLOC_SIZE_T LogicalLineCount;

        if (CurrentInputLine != NULL) {
            PreviousPhysicalLine = MoreGetPreviousFilteredPhysicalLine(MoreContext, CurrentInputLine->PhysicalLine);
        } else {
            PreviousPhysicalLine = MoreGetPreviousFilteredPhysicalLine(MoreContext, NULL);
        }
        if (PreviousPhysicalLine == NULL) {
            break;
        }

        LogicalLineCount = MoreCountLogicalLinesOnPhysicalLine(MoreContext, PreviousPhysicalLine);

        if (LogicalLineCount > LinesRemaining) {
            LinesToCopy = LinesRemaining;
            LineIndexToCopy = LogicalLineCount - LinesToCopy;
        } else {
            LinesToCopy = LogicalLineCount;
            LineIndexToCopy = 0;
        }
        CurrentOutputLine = &OutputLines[LinesRemaining - LinesToCopy];
        if (!MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                      PreviousPhysicalLine,
                                                      LineIndexToCopy,
                                                      LinesToCopy,
                                                      CurrentOutputLine)) {

            Result = FALSE;
            break;
        }

        LinesRemaining = LinesRemaining - LinesToCopy;
        CurrentInputLine = CurrentOutputLine;
    }

    if (Result) {
        *NumberLinesGenerated = LinesToOutput - LinesRemaining;
    } else {
        for (LinesRemaining = 0; LinesRemaining < LinesToOutput; LinesRemaining++) {
            YoriLibFreeStringContents(&OutputLines[LinesRemaining].Line);
        }
    }
    return Result;
}

/**
 Return the next set of logical lines following a previous logical line.  If
 no previous logical line is specified, returns the set from the first
 physical line.

 @param MoreContext Pointer to the more context specifying the data to
        display.

 @param CurrentLine Optionally points to the current line that the next lines
        should follow. If NULL, start from the first line in the buffer.

 @param StartFromNextLine If TRUE, logical lines should be generated following
        CurrentLine.  If FALSE, logical lines should be generated including
        and following CurrentLine.

 @param LinesToOutput Specifies the number of lines to output.

 @param OutputLines An array of logical lines with LinesToOutput elements.  On
        successful completion, populated with the data for those logical lines.

 @param NumberLinesGenerated On successful completion, populated with the
        number of logical lines generated.  This can be less than LinesToOutput
        if there is no more buffer.  When this occurs, the buffer is populated
        from first-to-last, so the last entries are the ones missing.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
MoreGetNextLogicalLines(
    __inout PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE CurrentLine,
    __in BOOLEAN StartFromNextLine,
    __in YORI_ALLOC_SIZE_T LinesToOutput,
    __out_ecount(*NumberLinesGenerated) PMORE_LOGICAL_LINE OutputLines,
    __out PYORI_ALLOC_SIZE_T NumberLinesGenerated
    )
{
    YORI_ALLOC_SIZE_T LinesRemaining = LinesToOutput;
    YORI_ALLOC_SIZE_T LinesToCopy;
    YORI_ALLOC_SIZE_T LineIndexToCopy;
    PMORE_LOGICAL_LINE CurrentOutputLine;
    PMORE_LOGICAL_LINE CurrentInputLine;
    YORI_ALLOC_SIZE_T LogicalLineCount;
    BOOLEAN Result = TRUE;

    CurrentInputLine = CurrentLine;

    if (CurrentInputLine != NULL) {
        if (StartFromNextLine) {
            LogicalLineCount = MoreCountLogicalLinesOnPhysicalLine(MoreContext, CurrentInputLine->PhysicalLine);

            if (CurrentInputLine->LogicalLineIndex < LogicalLineCount - 1) {
                LineIndexToCopy = CurrentInputLine->LogicalLineIndex + 1;
                if (LogicalLineCount - CurrentInputLine->LogicalLineIndex - 1 > LinesRemaining) {
                    LinesToCopy = LinesRemaining;
                } else {
                    LinesToCopy = LogicalLineCount - CurrentInputLine->LogicalLineIndex - 1;
                }
                CurrentOutputLine = &OutputLines[LinesToOutput - LinesRemaining];
                Result = MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                                  CurrentInputLine->PhysicalLine,
                                                                  LineIndexToCopy,
                                                                  LinesToCopy,
                                                                  CurrentOutputLine);
                LinesRemaining = LinesRemaining - LinesToCopy;
            }
        } else {
            LogicalLineCount = MoreCountLogicalLinesOnPhysicalLine(MoreContext, CurrentInputLine->PhysicalLine);
            LineIndexToCopy = CurrentInputLine->LogicalLineIndex;
            if (LogicalLineCount - CurrentInputLine->LogicalLineIndex > LinesRemaining) {
                LinesToCopy = LinesRemaining;
            } else {
                LinesToCopy = LogicalLineCount - CurrentInputLine->LogicalLineIndex;
            }
            CurrentOutputLine = &OutputLines[LinesToOutput - LinesRemaining];
            Result = MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                              CurrentInputLine->PhysicalLine,
                                                              LineIndexToCopy,
                                                              LinesToCopy,
                                                              CurrentOutputLine);
            LinesRemaining = LinesRemaining - LinesToCopy;
        }
    }

    while(Result && LinesRemaining > 0) {
        PMORE_PHYSICAL_LINE NextPhysicalLine;

        if (CurrentInputLine != NULL) {
            ASSERT(CurrentInputLine->PhysicalLine != NULL);
            __analysis_assume(CurrentInputLine->PhysicalLine != NULL);
            NextPhysicalLine = MoreGetNextFilteredPhysicalLine(MoreContext, CurrentInputLine->PhysicalLine);
        } else {
            NextPhysicalLine = MoreGetNextFilteredPhysicalLine(MoreContext, NULL);
        }

        if (NextPhysicalLine == NULL) {
            break;
        }

        LogicalLineCount = MoreCountLogicalLinesOnPhysicalLine(MoreContext, NextPhysicalLine);

        LineIndexToCopy = 0;
        if (LogicalLineCount > LinesRemaining) {
            LinesToCopy = LinesRemaining;
        } else {
            LinesToCopy = LogicalLineCount;
        }
        CurrentOutputLine = &OutputLines[LinesToOutput - LinesRemaining];
        Result = MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                          NextPhysicalLine,
                                                          LineIndexToCopy,
                                                          LinesToCopy,
                                                          CurrentOutputLine);

        LinesRemaining = LinesRemaining - LinesToCopy;
        CurrentInputLine = &CurrentOutputLine[LinesToCopy - 1];
    }

    if (Result) {
        *NumberLinesGenerated = LinesToOutput - LinesRemaining;
    } else {
        for (LinesRemaining = 0; LinesRemaining < LinesToOutput; LinesRemaining++) {
            YoriLibFreeStringContents(&OutputLines[LinesRemaining].Line);
        }
    }

    return Result;
}

/**
 Find the next physical line that contains a match for the current search
 string, or for any search string.

 @param MoreContext Pointer to the more context, containing all physical
        lines.

 @param PreviousMatchLine Pointer to the logical line which is the most
        recent line to not look for matches within.

 @param MatchAny If TRUE, match against any search string.  If FALSE, match
        against the current search string only.

 @param MaxLogicalLinesMoved Specifies the maximum number of logical lines
        to count.  This value is a performance optimization - searching
        physical lines is much cheaper than parsing them into logical lines,
        so there's no point doing this unless the caller wants to know the
        result.  Currently the caller only cares if the logical lines are
        less than the viewport (so advance a specified number of lines); if
        it's larger than the viewport, render everything from scratch.

 @param LogicalLinesMoved On successful completion, updated to indicate
        the number of logical lines that were processed before finding a
        match.  This value is limited to MaxLogicalLinesMoved above.

 @return Pointer to the next physical line containing a match, or NULL
         if no further physical lines contain a match.
 */
__success(return != NULL)
PMORE_PHYSICAL_LINE
MoreFindNextLineWithSearchMatch(
    __in PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE PreviousMatchLine,
    __in BOOLEAN MatchAny,
    __in YORI_ALLOC_SIZE_T MaxLogicalLinesMoved,
    __out_opt PYORI_ALLOC_SIZE_T LogicalLinesMoved
    )
{
    PMORE_PHYSICAL_LINE SearchLine;
    PYORI_STRING SearchString;
    YORI_ALLOC_SIZE_T MatchOffset;
    YORI_ALLOC_SIZE_T Count;
    YORI_ALLOC_SIZE_T LogicalLinesThisPhysicalLine;

    Count = 0;

    //
    //  MSFIX Although the function signature takes a logical line, this
    //  function looks for a match on the next physical line.  This means
    //  that a match on a later logical line on the same physical line
    //  will not be found.  With the current function signature, there
    //  would be no way to return a new logical line on the same physical
    //  line; perhaps this should return a logical line?
    //

    if (PreviousMatchLine == NULL) {
        SearchLine = NULL;
    } else {
        SearchLine = PreviousMatchLine->PhysicalLine;

        if (LogicalLinesMoved != NULL) {
            LogicalLinesThisPhysicalLine = MoreCountLogicalLinesOnPhysicalLine(MoreContext, SearchLine);
            ASSERT(LogicalLinesThisPhysicalLine >= PreviousMatchLine->LogicalLineIndex + 1);
            Count = LogicalLinesThisPhysicalLine - PreviousMatchLine->LogicalLineIndex;
        }
    }


    SearchString = &MoreContext->SearchStrings[MoreContext->SearchColorIndex];
    ASSERT(SearchString->LengthInChars > 0);

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    while (TRUE) {
        SearchLine = MoreGetNextFilteredPhysicalLine(MoreContext, SearchLine);
        if (SearchLine == NULL) {
            break;
        }

        if (MatchAny) {
            if (MoreFindNextSearchMatch(MoreContext, &SearchLine->LineContents, NULL, NULL)) {
                break;
            }
        } else {
            if (YoriLibFindFirstMatchingSubstringInsensitive(&SearchLine->LineContents, 1, SearchString, &MatchOffset)) {
                break;
            }
        }

        if (LogicalLinesMoved != NULL && Count < MaxLogicalLinesMoved) {
            LogicalLinesThisPhysicalLine = MoreCountLogicalLinesOnPhysicalLine(MoreContext, SearchLine);
            Count = Count + LogicalLinesThisPhysicalLine;
        }
    }

    if (LogicalLinesMoved != NULL) {
        if (Count > MaxLogicalLinesMoved) {
            *LogicalLinesMoved = MaxLogicalLinesMoved;
        } else {
            *LogicalLinesMoved = Count;
        }
    }

    ReleaseMutex(MoreContext->PhysicalLineMutex);
    return SearchLine;
}

/**
 Find the previous physical line that contains a match for the current search
 string, or for any search string.

 @param MoreContext Pointer to the more context, containing all physical
        lines.

 @param PreviousMatchLine Pointer to the logical line which is the most
        recent line to not look for matches within.

 @param MatchAny If TRUE, match against any search string.  If FALSE, match
        against the current search string only.

 @param MaxLogicalLinesMoved Specifies the maximum number of logical lines
        to count.  This value is a performance optimization - searching
        physical lines is much cheaper than parsing them into logical lines,
        so there's no point doing this unless the caller wants to know the
        result.  Currently the caller only cares if the logical lines are
        less than the viewport (so advance a specified number of lines); if
        it's larger than the viewport, render everything from scratch.

 @param LogicalLinesMoved On successful completion, updated to indicate
        the number of logical lines that were processed before finding a
        match.  This value is limited to MaxLogicalLinesMoved above.

 @return Pointer to the next physical line containing a match, or NULL
         if no further physical lines contain a match.
 */
__success(return != NULL)
PMORE_PHYSICAL_LINE
MoreFindPreviousLineWithSearchMatch(
    __in PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE PreviousMatchLine,
    __in BOOLEAN MatchAny,
    __in YORI_ALLOC_SIZE_T MaxLogicalLinesMoved,
    __out_opt PYORI_ALLOC_SIZE_T LogicalLinesMoved
    )
{
    PMORE_PHYSICAL_LINE SearchLine;
    PYORI_STRING SearchString;
    YORI_ALLOC_SIZE_T MatchOffset;
    YORI_ALLOC_SIZE_T Count;
    YORI_ALLOC_SIZE_T LogicalLinesThisPhysicalLine;

    Count = 0;

    if (PreviousMatchLine == NULL) {
        SearchLine = NULL;
    } else {
        SearchLine = PreviousMatchLine->PhysicalLine;

        if (LogicalLinesMoved != NULL) {
            Count = PreviousMatchLine->LogicalLineIndex + 1;
        }
    }

    SearchString = &MoreContext->SearchStrings[MoreContext->SearchColorIndex];
    ASSERT(SearchString->LengthInChars > 0);

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    while (TRUE) {
        SearchLine = MoreGetPreviousFilteredPhysicalLine(MoreContext, SearchLine);
        if (SearchLine == NULL) {
            break;
        }

        if (MatchAny) {
            if (MoreFindNextSearchMatch(MoreContext, &SearchLine->LineContents, NULL, NULL)) {
                break;
            }
        } else {
            if (YoriLibFindFirstMatchingSubstringInsensitive(&SearchLine->LineContents, 1, SearchString, &MatchOffset)) {
                break;
            }
        }

        if (LogicalLinesMoved != NULL && Count < MaxLogicalLinesMoved) {
            LogicalLinesThisPhysicalLine = MoreCountLogicalLinesOnPhysicalLine(MoreContext, SearchLine);
            Count = Count + LogicalLinesThisPhysicalLine;
        }
    }

    if (LogicalLinesMoved != NULL) {
        if (Count > MaxLogicalLinesMoved) {
            *LogicalLinesMoved = MaxLogicalLinesMoved;
        } else {
            *LogicalLinesMoved = Count;
        }
    }

    ReleaseMutex(MoreContext->PhysicalLineMutex);
    return SearchLine;
}


// vim:sw=4:ts=4:et:
