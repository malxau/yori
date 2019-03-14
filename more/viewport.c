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
 A structure to describe the state of parsing after a single logical line
 has been processed.
 */
typedef struct _MORE_LINE_END_CONTEXT {

    /**
     Indicates the color that is being displayed at the end of one logical
     line.  Conceptually the next logical line starts with this color.
     */
    WORD FinalDisplayColor;

    /**
     Indicates the color that is specified by the inbound text stream at the
     end of a logical line.  Typically this is the same as the display color
     above, but can be different if the display color is being overwridden
     by this program via a search.
     */
    WORD FinalUserColor;

    /**
     Set to TRUE to indicate the logical line needs to be parsed character
     by character because the contents of the logical line are not merely
     a subset of characters from a physical line.  FALSE if the line is
     just part of a physical line allocation.  Currently this is only used
     when a search is present so that the logical line contains extra
     escape sequences that the physical line does not.
     */
    BOOL RequiresGeneration;

    /**
     Specifies the number of characters needed to describe the logical line
     contents.  When RequiresGeneration is TRUE this number may not match
     the number of characters needing to be consumed from the physical line.
     */
    DWORD CharactersNeededInAllocation;

    /**
     Indicates the number of characters remaining in a search match.  This is
     nonzero if a match is found that is partially on one logical line and
     partially on a following logical line.  In that case, the second logical
     line contains a highlighted region that is for fewer characters than the
     match string, does not match the match string, etc.
     */
    DWORD CharactersRemainingInMatch;
} MORE_LINE_END_CONTEXT, *PMORE_LINE_END_CONTEXT;

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
DWORD
MoreGetLogicalLineLength(
    __in PMORE_CONTEXT MoreContext,
    __in PYORI_STRING PhysicalLineSubset,
    __in DWORD MaximumVisibleCharacters,
    __in WORD InitialDisplayColor,
    __in WORD InitialUserColor,
    __in DWORD CharactersRemainingInMatch,
    __out_opt PMORE_LINE_END_CONTEXT LineEndContext
    )
{
    DWORD SourceIndex;
    DWORD CharsInOutputBuffer;
    DWORD CellsDisplayed;
    YORI_STRING EscapeSubset;
    DWORD EndOfEscape;
    WORD CurrentColor = InitialDisplayColor;
    WORD CurrentUserColor = InitialUserColor;
    YORI_STRING MatchEscapeChars;
    TCHAR MatchEscapeCharsBuf[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];
    DWORD MatchOffset;
    DWORD MatchLength;
    BOOL MatchFound;

    CharsInOutputBuffer = 0;
    CellsDisplayed = 0;

    if (LineEndContext != NULL) {
        LineEndContext->RequiresGeneration = FALSE;
    }

    MatchOffset = 0;
    MatchLength = 0;
    YoriLibInitEmptyString(&MatchEscapeChars);
    MatchEscapeChars.StartOfString = MatchEscapeCharsBuf;
    MatchEscapeChars.LengthAllocated = sizeof(MatchEscapeCharsBuf)/sizeof(MatchEscapeCharsBuf[0]);
    if (CharactersRemainingInMatch > 0) {
        MatchFound = TRUE;
        MatchLength = CharactersRemainingInMatch;
    } else if (MoreContext->SearchString.LengthInChars > 0) {
        MatchFound = TRUE;
    } else {
        MatchFound = FALSE;
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
            if (YoriLibFindFirstMatchingSubstringInsensitive(&StringForNextMatch, 1, &MoreContext->SearchString, &MatchOffset)) {
                MatchFound = TRUE;
                MatchLength = MoreContext->SearchString.LengthInChars;
                MatchOffset += SourceIndex;
            } else {
                MatchFound = FALSE;
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
                    YoriLibVtStringForTextAttribute(&MatchEscapeChars, 0, MoreContext->SearchColor);
                    CharsInOutputBuffer += MatchEscapeChars.LengthInChars;
                    CurrentColor = MoreContext->SearchColor;
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
                CharsInOutputBuffer += MatchEscapeChars.LengthInChars;
                CurrentColor = CurrentUserColor;
            }
        }

        ASSERT(CellsDisplayed <= MaximumVisibleCharacters);
        if (CellsDisplayed == MaximumVisibleCharacters) {
            break;
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
        LogicalLineLength = MoreGetLogicalLineLength(MoreContext, &Subset, MoreContext->ViewportWidth, 0, 0, 0, NULL);
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
 Move a logical line from one memory location to another.  Logical lines
 are referenced, so the move implies dereferencing anything being overwritten,
 and transferring the data with its existing reference, zeroing out the
 source as it should no longer be dereferenced.

 @param Dest Pointer to the destination of the move.

 @param Src Pointer to the source of the move.
 */
VOID
MoreMoveLogicalLine(
    __out PMORE_LOGICAL_LINE Dest,
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
    __out PMORE_LOGICAL_LINE Dest,
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
BOOL
MoreCopyRangeIntoLogicalLine(
    __in PMORE_CONTEXT MoreContext,
    __in PMORE_LOGICAL_LINE LogicalLine,
    __in BOOL RegenerationRequired,
    __in DWORD SourceCharsToConsume,
    __in DWORD AllocationLengthRequired
    )
{
    ASSERT(LogicalLine->Line.LengthAllocated == 0 && LogicalLine->Line.MemoryToFree == NULL);

    if (RegenerationRequired) {
        YORI_STRING PhysicalLineSubset;
        DWORD SourceIndex;
        DWORD CharsInOutputBuffer;
        YORI_STRING MatchEscapeChars;
        YORI_STRING EscapeSubset;
        DWORD EndOfEscape;
        TCHAR MatchEscapeCharsBuf[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];
        DWORD MatchOffset;
        DWORD MatchLength;
        BOOL MatchFound;
        WORD CurrentUserColor = LogicalLine->InitialUserColor;

        YoriLibInitEmptyString(&PhysicalLineSubset);
        PhysicalLineSubset.StartOfString = &LogicalLine->PhysicalLine->LineContents.StartOfString[LogicalLine->PhysicalLineCharacterOffset]; 
        PhysicalLineSubset.LengthInChars = SourceCharsToConsume;

        if (!YoriLibAllocateString(&LogicalLine->Line, AllocationLengthRequired)) {
            MoreContext->OutOfMemory = TRUE;
            return FALSE;
        }

        CharsInOutputBuffer = 0;
    
        MatchOffset = 0;
        MatchLength = 0;
        YoriLibInitEmptyString(&MatchEscapeChars);
        MatchEscapeChars.StartOfString = MatchEscapeCharsBuf;
        MatchEscapeChars.LengthAllocated = sizeof(MatchEscapeCharsBuf)/sizeof(MatchEscapeCharsBuf[0]);
        if (LogicalLine->CharactersRemainingInMatch > 0) {
            MatchFound = TRUE;
            MatchLength = LogicalLine->CharactersRemainingInMatch;
        } else if (MoreContext->SearchString.LengthInChars > 0) {
            MatchFound = TRUE;
        } else {
            MatchFound = FALSE;
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
                if (YoriLibFindFirstMatchingSubstringInsensitive(&StringForNextMatch, 1, &MoreContext->SearchString, &MatchOffset)) {
                    MatchFound = TRUE;
                    MatchLength = MoreContext->SearchString.LengthInChars;
                    MatchOffset += SourceIndex;
                } else {
                    MatchFound = FALSE;
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
                        YoriLibVtStringForTextAttribute(&MatchEscapeChars, 0, MoreContext->SearchColor);
                        memcpy(&LogicalLine->Line.StartOfString[CharsInOutputBuffer], MatchEscapeChars.StartOfString, MatchEscapeChars.LengthInChars * sizeof(TCHAR));
                        CharsInOutputBuffer += MatchEscapeChars.LengthInChars;
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
                    CharsInOutputBuffer += MatchEscapeChars.LengthInChars;
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
BOOL
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
    DWORD CharactersRemainingInMatch = 0;
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

            ASSERT(ThisLine->CharactersRemainingInMatch == 0 || ThisLine->InitialUserColor != ThisLine->InitialDisplayColor);

            if (!MoreCopyRangeIntoLogicalLine(MoreContext, ThisLine, LineEndContext.RequiresGeneration, LogicalLineLength, LineEndContext.CharactersNeededInAllocation)) {
                return FALSE;
            }

            CharactersRemainingInMatch = LineEndContext.CharactersRemainingInMatch;
        }

        InitialUserColor = LineEndContext.FinalUserColor;
        InitialDisplayColor = LineEndContext.FinalDisplayColor;
        Subset.StartOfString += LogicalLineLength;
        Subset.LengthInChars -= LogicalLineLength;
        Count++;
        CharIndex += LogicalLineLength;
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

 @param CurrentLine Points to the current line that the previous lines should
        be returned for.

 @param LinesToOutput Specifies the number of lines to output.

 @param OutputLines An array of logical lines with LinesToOutput elements.  On
        successful completion, populated with the data for those logical lines.

 @param NumberLinesGenerated On successful completion, populated with the
        number of logical lines generated.  This can be less than LinesToOutput
        if there is no more buffer.  When this occurs, the buffer is populated
        from last-to-first, so the first entries are the ones missing.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MoreGetPreviousLogicalLines(
    __inout PMORE_CONTEXT MoreContext,
    __in PMORE_LOGICAL_LINE CurrentLine,
    __in DWORD LinesToOutput,
    __out PMORE_LOGICAL_LINE OutputLines,
    __out PDWORD NumberLinesGenerated
    )
{
    DWORD LinesRemaining = LinesToOutput;
    DWORD LinesToCopy;
    DWORD LineIndexToCopy;
    PMORE_LOGICAL_LINE CurrentOutputLine;
    PMORE_LOGICAL_LINE CurrentInputLine;
    BOOL Result = TRUE;

    CurrentInputLine = CurrentLine;

    //
    //  This routine wants to find earlier logical lines.  If the current
    //  logical line is partway through a physical line, go find the logical
    //  lines before this one on that physical line.
    //

    if (CurrentInputLine->LogicalLineIndex > 0) {
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
        LinesRemaining -= LinesToCopy;
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
        if (!MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                      PreviousPhysicalLine,
                                                      LineIndexToCopy,
                                                      LinesToCopy,
                                                      CurrentOutputLine)) {

            Result = FALSE;
            break;
        }

        LinesRemaining -= LinesToCopy;
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
        should follow.

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
DWORD
MoreGetNextLogicalLines(
    __inout PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE CurrentLine,
    __in BOOL StartFromNextLine,
    __in DWORD LinesToOutput,
    __out PMORE_LOGICAL_LINE OutputLines,
    __out PDWORD NumberLinesGenerated
    )
{
    DWORD LinesRemaining = LinesToOutput;
    DWORD LinesToCopy;
    DWORD LineIndexToCopy;
    PMORE_LOGICAL_LINE CurrentOutputLine;
    PMORE_LOGICAL_LINE CurrentInputLine;
    DWORD LogicalLineCount;
    BOOL Result = TRUE;

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
                LinesRemaining -= LinesToCopy;
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
            LinesRemaining -= LinesToCopy;
        }
    }

    while(Result && LinesRemaining > 0) {
        PMORE_PHYSICAL_LINE NextPhysicalLine;
        PYORI_LIST_ENTRY ListEntry;

        if (CurrentInputLine != NULL) {
            ASSERT(CurrentInputLine->PhysicalLine != NULL);
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
        Result = MoreGenerateLogicalLinesFromPhysicalLine(MoreContext,
                                                          NextPhysicalLine,
                                                          LineIndexToCopy,
                                                          LinesToCopy,
                                                          CurrentOutputLine);

        LinesRemaining -= LinesToCopy;
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
 string.

 @param MoreContext Pointer to the more context, containing all physical
        lines.

 @param PreviousMatchLine Pointer to the logical line which is the most
        recent line to not look for matches within.

 @return Pointer to the next physical line containing a match, or NULL
         if no further physical lines contain a match.
 */
PMORE_PHYSICAL_LINE
MoreFindNextLineWithSearchMatch(
    __in PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE PreviousMatchLine
    )
{
    PMORE_PHYSICAL_LINE SearchLine;
    PYORI_LIST_ENTRY ListEntry;
    DWORD MatchOffset;

    if (PreviousMatchLine == NULL) {
        SearchLine = NULL;
        ListEntry = NULL;
    } else {
        SearchLine = PreviousMatchLine->PhysicalLine;
        ListEntry = &SearchLine->LineList;
    }

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);

    while (TRUE) {
        ListEntry = YoriLibGetNextListEntry(&MoreContext->PhysicalLineList, ListEntry);
        if (ListEntry == NULL) {
            ReleaseMutex(MoreContext->PhysicalLineMutex);
            return NULL;
        }

        SearchLine = CONTAINING_RECORD(ListEntry, MORE_PHYSICAL_LINE, LineList);
        if (YoriLibFindFirstMatchingSubstringInsensitive(&SearchLine->LineContents, 1, &MoreContext->SearchString, &MatchOffset)) {
            ReleaseMutex(MoreContext->PhysicalLineMutex);
            return SearchLine;
        }
    }
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
    PYORI_LIST_ENTRY ListEntry;
    PMORE_PHYSICAL_LINE LastPhysicalLine;
    BOOL PageFull;
    BOOL ThreadActive;
    LPTSTR StringToDisplay;
    YORI_STRING LineToDisplay;

    //
    //  If the screen isn't full, there's no point displaying status
    //

    if (MoreContext->LinesInViewport < MoreContext->ViewportHeight) {
        return;
    }

    YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    FirstViewportLine = MoreContext->DisplayViewportLines[0].PhysicalLine->LineNumber;
    LastViewportLine = MoreContext->DisplayViewportLines[MoreContext->LinesInViewport - 1].PhysicalLine->LineNumber;

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);
    ListEntry = YoriLibGetPreviousListEntry(&MoreContext->PhysicalLineList, NULL);
    LastPhysicalLine = CONTAINING_RECORD(ListEntry, MORE_PHYSICAL_LINE, LineList);
    TotalLines = LastPhysicalLine->LineNumber;
    MoreContext->TotalLinesInViewportStatus = TotalLines;
    ReleaseMutex(MoreContext->PhysicalLineMutex);

    ASSERT(MoreContext->LinesInPage <= MoreContext->LinesInViewport);
    if (MoreContext->LinesInViewport == MoreContext->LinesInPage) {
        PageFull = TRUE;
    } else {
        PageFull = FALSE;
    }

    if (WaitForSingleObject(MoreContext->IngestThread, 0) == WAIT_OBJECT_0) {
        ThreadActive = FALSE;
    } else {
        ThreadActive = TRUE;
    }

    if (!ThreadActive && TotalLines == LastViewportLine) {
        StringToDisplay = _T("End");
    } else if (!PageFull) {
        StringToDisplay = _T("Awaiting data");
    } else {
        StringToDisplay = _T("More");
    }

    YoriLibInitEmptyString(&LineToDisplay);
    if (MoreContext->SearchString.LengthInChars > 0 || MoreContext->SearchMode) {
        YoriLibYPrintf(&LineToDisplay,
                      _T(" --- %s --- (%lli-%lli of %lli, %i%%) Search: %y"),
                      StringToDisplay,
                      FirstViewportLine,
                      LastViewportLine,
                      TotalLines,
                      LastViewportLine * 100 / TotalLines,
                      &MoreContext->SearchString);
    } else {
        YoriLibYPrintf(&LineToDisplay,
                      _T(" --- %s --- (%lli-%lli of %lli, %i%%)"),
                      StringToDisplay,
                      FirstViewportLine,
                      LastViewportLine,
                      TotalLines,
                      LastViewportLine * 100 / TotalLines);
    }

    //
    //  If the status line would be more than a line, truncate it.  Add three
    //  dots to the end of it if the console is a sane width to indicate that
    //  it has been truncated
    //

    if (MoreContext->ViewportWidth > 0) {
        if (LineToDisplay.LengthInChars > MoreContext->ViewportWidth - 1) {
            LineToDisplay.LengthInChars = MoreContext->ViewportWidth - 1;
            if (LineToDisplay.LengthInChars > 5) {
                LineToDisplay.StartOfString[LineToDisplay.LengthInChars - 1] = '.';
                LineToDisplay.StartOfString[LineToDisplay.LengthInChars - 2] = '.';
                LineToDisplay.StartOfString[LineToDisplay.LengthInChars - 3] = '.';
            }
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

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y%c[0m\n"), &MoreContext->DisplayViewportLines[Index].Line, 27);
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
    __in DWORD LineCount
    )
{
    DWORD Index;
    DWORD CharsRequired;
    YORI_STRING CombinedBuffer;
    YORI_STRING VtAttribute;
    TCHAR VtAttributeBuffer[32];

    YoriLibInitEmptyString(&VtAttribute);
    VtAttribute.StartOfString = VtAttributeBuffer;
    VtAttribute.LengthAllocated = sizeof(VtAttributeBuffer)/sizeof(TCHAR);

    CharsRequired = 0;

    for (Index = 0; Index < LineCount; Index++) {
        YoriLibVtStringForTextAttribute(&VtAttribute, 0, FirstLine[Index].InitialDisplayColor);
        CharsRequired += VtAttribute.LengthInChars;
        CharsRequired += FirstLine[Index].Line.LengthInChars;

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
            CharsRequired += VtAttribute.LengthInChars;
            memcpy(&CombinedBuffer.StartOfString[CharsRequired], FirstLine[Index].Line.StartOfString, FirstLine[Index].Line.LengthInChars * sizeof(TCHAR));
            CharsRequired += FirstLine[Index].Line.LengthInChars;
            CharsRequired += YoriLibSPrintf(&CombinedBuffer.StartOfString[CharsRequired], _T("%c[0m\n"), 27);
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

            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y%c[0m\n"), &FirstLine[Index].Line, 27);
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
    DWORD Index;
    DWORD ChangedLineCount = 0;
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
                                 &NumberWritten)) {

        return 0;
    }

    //
    //  The data shouldn't already be in the viewport if it's unavailable.
    //

    ASSERT(NumberWritten == MoreContext->LinesInViewport);
                            
    for (Index = 0; Index < MoreContext->LinesInViewport; Index++) {

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
    __in DWORD NewLineCount
    )
{
    DWORD Index;
    DWORD FirstLineToDisplay;
    DWORD LinesToPreserve;
    DWORD LineIndexToPreserve;

    ASSERT(NewLineCount <= MoreContext->ViewportHeight);

    if (MoreContext->LinesInViewport + NewLineCount > MoreContext->ViewportHeight) {

        LinesToPreserve = MoreContext->LinesInViewport - NewLineCount;
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
    MoreContext->LinesInViewport += NewLineCount;
    MoreContext->LinesInPage += NewLineCount;
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
    __in DWORD NewLineCount
    )
{
    DWORD Index;
    DWORD LinesToPreserve;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE StdOutHandle;
    DWORD OldLinesInViewport;
    COORD NewPosition;
    DWORD NumberWritten;
    SMALL_RECT RectToMove;
    CHAR_INFO Fill;

    StdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(StdOutHandle, &ScreenInfo);

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
    //  Add new lines to the top of the buffer.
    //

    for (Index = 0; Index < NewLineCount; Index++) {
        MoreMoveLogicalLine(&MoreContext->DisplayViewportLines[Index],
                            &NewLines[Index]);
    }

    //
    //  If the buffer has more lines as a result, update the number of
    //  lines.
    //

    MoreContext->LinesInViewport += NewLineCount;
    if (MoreContext->LinesInViewport > MoreContext->ViewportHeight) {
        MoreContext->LinesInViewport = MoreContext->ViewportHeight;
    }
    MoreContext->LinesInPage = MoreContext->LinesInViewport;

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
    DWORD LinesDesired;
    DWORD LinesReturned;
    BOOL Success;
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

    LinesDesired = MoreContext->ViewportHeight - MoreContext->LinesInPage;

    Success = MoreGetNextLogicalLines(MoreContext, CurrentLine, TRUE, LinesDesired, MoreContext->StagingViewportLines, &LinesReturned);

    ReleaseMutex(MoreContext->PhysicalLineMutex);

    if (!Success || LinesReturned == 0) {
        return;
    }

    MoreDisplayNewLinesInViewport(MoreContext, MoreContext->StagingViewportLines, LinesReturned);
}

/**
 Move the viewport up within the buffer of text, so that the previous line
 of data is rendered at the top of the screen.

 @param MoreContext Pointer to the context describing the data to display.

 @param LinesToMove Specifies the number of lines to move up.

 @return The number of lines actually moved.
 */
DWORD
MoreMoveViewportUp(
    __inout PMORE_CONTEXT MoreContext,
    __in DWORD LinesToMove
    )
{
    PMORE_LOGICAL_LINE CurrentLine;
    BOOL Success;
    DWORD LinesReturned;
    DWORD CappedLinesToMove;

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
DWORD
MoreMoveViewportDown(
    __inout PMORE_CONTEXT MoreContext,
    __in DWORD LinesToMove
    )
{
    PMORE_LOGICAL_LINE CurrentLine;
    BOOL Success;
    DWORD LinesReturned;
    DWORD CappedLinesToMove;

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

    ASSERT(LinesReturned <= CappedLinesToMove);

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
    DWORD LinesReturned;
    DWORD CappedLinesToMove;
    BOOL Success;
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

    ASSERT(LinesReturned <= CappedLinesToMove);

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

    LineToFollow = NULL;
    if (MoreContext->LinesInViewport > 0) {
        LineToFollow = &MoreContext->DisplayViewportLines[0];
    }

    NextMatch = MoreFindNextLineWithSearchMatch(MoreContext, LineToFollow);
    if (NextMatch == NULL) {
        return;
    }

    MoreContext->LinesInPage = 0;
    if (YoriLibIsSelectionActive(&MoreContext->Selection)) {
        YoriLibClearSelection(&MoreContext->Selection);
        YoriLibRedrawSelection(&MoreContext->Selection);
    }

    MoreRegenerateViewport(MoreContext, NextMatch);
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
    DWORD LineCount;
    DWORD StartingLineIndex;
    DWORD SingleLineBufferSize;
    DWORD VtTextBufferSize;
    DWORD LineIndex;
    DWORD CharactersRemainingInMatch = 0;
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
        Selection->CurrentlyDisplayed.Left != Selection->PreviouslyDisplayed.Left ||
        Selection->CurrentlyDisplayed.Right != Selection->PreviouslyDisplayed.Right ||
        Selection->CurrentlyDisplayed.Top != Selection->PreviouslyDisplayed.Top ||
        Selection->CurrentlyDisplayed.Bottom != Selection->PreviouslyDisplayed.Bottom) {

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
        DWORD LogicalLineLength;
        MORE_LINE_END_CONTEXT LineEndContext;

        for (LineIndex = StartingLineIndex; LineIndex < StartingLineIndex + LineCount; LineIndex++) {
            YoriLibInitEmptyString(&Subset);
            Subset.StartOfString = EntireLogicalLines[LineIndex].Line.StartOfString;
            Subset.LengthInChars = EntireLogicalLines[LineIndex].Line.LengthInChars;
            InitialDisplayColor = EntireLogicalLines[LineIndex].InitialDisplayColor;
            InitialUserColor = EntireLogicalLines[LineIndex].InitialUserColor;
            CharactersRemainingInMatch = EntireLogicalLines[LineIndex].CharactersRemainingInMatch;
            if (Selection->CurrentlySelected.Left > 0) {
                LogicalLineLength = MoreGetLogicalLineLength(MoreContext, &Subset, Selection->CurrentlySelected.Left, InitialDisplayColor, InitialUserColor, CharactersRemainingInMatch, &LineEndContext);
                InitialDisplayColor = LineEndContext.FinalDisplayColor;
                InitialUserColor = LineEndContext.FinalUserColor;
                Subset.LengthInChars -= LogicalLineLength;
                Subset.StartOfString += LogicalLineLength;
                CharactersRemainingInMatch = LineEndContext.CharactersRemainingInMatch;
            }
            LogicalLineLength = MoreGetLogicalLineLength(MoreContext, &Subset, Selection->CurrentlySelected.Right - Selection->CurrentlySelected.Left + 1, InitialDisplayColor, InitialUserColor, CharactersRemainingInMatch, &LineEndContext);
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
        VtText.LengthInChars += Subset.LengthInChars;
        Subset.StartOfString += Subset.LengthInChars;
        Subset.LengthAllocated -= Subset.LengthInChars;
        Subset.LengthInChars = 0;

        YoriLibYPrintf(&Subset, _T("%y\r\n"), &CopyLogicalLines[LineIndex].Line);

        VtText.LengthInChars += Subset.LengthInChars;
        Subset.StartOfString += Subset.LengthInChars;
        Subset.LengthAllocated -= Subset.LengthInChars;
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
    __out PBOOL Terminate,
    __out PBOOL RedrawStatus
    )
{
    DWORD CtrlMask;
    TCHAR Char;
    WORD KeyCode;
    WORD ScanCode;
    BOOL ClearSelection = FALSE;

    *Terminate = FALSE;

    Char = InputRecord->Event.KeyEvent.uChar.UnicodeChar;
    CtrlMask = InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | ENHANCED_KEY | SHIFT_PRESSED);
    KeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;
    ScanCode = InputRecord->Event.KeyEvent.wVirtualScanCode;

    if (CtrlMask == 0 || CtrlMask == SHIFT_PRESSED) {
        ClearSelection = TRUE;
        if (MoreContext->SearchMode) {
            if (Char == 27) {
                MoreContext->SearchMode = FALSE;
                YoriLibFreeStringContents(&MoreContext->SearchString);
                MoreContext->SearchDirty = TRUE;
            } else if (Char == '\b') {
                if (InputRecord->Event.KeyEvent.wRepeatCount > MoreContext->SearchString.LengthInChars) {
                    MoreContext->SearchString.LengthInChars = 0;
                } else {
                    MoreContext->SearchString.LengthInChars = MoreContext->SearchString.LengthInChars - InputRecord->Event.KeyEvent.wRepeatCount;
                }
                MoreContext->SearchDirty = TRUE;
            } else if (Char == '\r') {
                if (YoriLibIsSelectionActive(&MoreContext->Selection)) {
                    MoreCopySelectionIfPresent(MoreContext);
                } else {
                    MoreMoveViewportToNextSearchMatch(MoreContext);
                }
            } else if (Char != '\0' && Char != '\n') {
                if (MoreContext->SearchString.LengthAllocated < MoreContext->SearchString.LengthInChars + InputRecord->Event.KeyEvent.wRepeatCount + 1) {
                    DWORD NewAllocSize;
                    NewAllocSize = MoreContext->SearchString.LengthAllocated + 4096;
                    if (NewAllocSize < MoreContext->SearchString.LengthInChars + InputRecord->Event.KeyEvent.wRepeatCount + 1) {
                        NewAllocSize = MoreContext->SearchString.LengthInChars + InputRecord->Event.KeyEvent.wRepeatCount + 1;
                    }
                    YoriLibReallocateString(&MoreContext->SearchString, NewAllocSize);
                }
                if (MoreContext->SearchString.LengthAllocated >= MoreContext->SearchString.LengthInChars + InputRecord->Event.KeyEvent.wRepeatCount + 1) {
                    DWORD Count;
                    for (Count = 0; Count < InputRecord->Event.KeyEvent.wRepeatCount; Count++) {
                        MoreContext->SearchString.StartOfString[MoreContext->SearchString.LengthInChars + Count] = Char;
                    }
                    MoreContext->SearchString.LengthInChars = MoreContext->SearchString.LengthInChars + InputRecord->Event.KeyEvent.wRepeatCount;
                    MoreContext->SearchDirty = TRUE;
                }
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
            } else if (Char == '\r') {
                MoreCopySelectionIfPresent(MoreContext);
            } else if (Char == '/') {
                MoreContext->SearchMode = TRUE;
                MoreContext->SearchDirty = TRUE;
                *RedrawStatus = TRUE;
            }
        }
    } else if (CtrlMask == ENHANCED_KEY) {
        ClearSelection = TRUE;
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
    DWORD NewViewportHeight;
    DWORD NewViewportWidth;
    DWORD OldLinesInViewport;
    DWORD Index;
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
                FillConsoleOutputAttribute(StdOutHandle, YoriLibVtGetDefaultColor(), ScreenInfo.dwSize.X * (OldLinesInViewport - NewViewportHeight + 1), NewCursorPosition, &NumberWritten);
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
    DWORD NewViewportWidth;
    DWORD NewViewportHeight;

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
    if (MoreContext->TotalLinesInViewportStatus != MoreContext->LineCount || MoreContext->SearchDirty) {
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
    __out PBOOL TerminateInput
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
    __out PBOOL TerminateInput,
    __out PBOOL RedrawStatus
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
    __out PBOOL TerminateInput
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
    HANDLE ObjectsToWaitFor[3];
    HANDLE InHandle;
    DWORD WaitObject;
    DWORD HandleCountToWait;
    DWORD PreviousMouseButtonState = 0;
    DWORD Timeout;
    BOOL WaitForIngestThread = TRUE;
    BOOL WaitForNewLines = TRUE;

    InHandle = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (InHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    SetConsoleMode(InHandle, ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);

    //
    //  A better way to read this is "disable ENABLE_WRAP_AT_EOL_OUTPUT"
    //  which is the default.  This program must emit explicit newlines
    //  after each viewport line.
    //

    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT);

    while(TRUE) {

        //
        //  If the viewport is full, we don't care about new lines being
        //  ingested.
        //

        if (MoreContext->LinesInPage == MoreContext->ViewportHeight) {
            WaitForNewLines = FALSE;
        } else {
            WaitForNewLines = TRUE;
        }

        HandleCountToWait = 0;
        ObjectsToWaitFor[HandleCountToWait++] = InHandle;
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

        WaitObject = WaitForMultipleObjects(HandleCountToWait, ObjectsToWaitFor, FALSE, Timeout);

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
            }
            MoreCheckForWindowSizeChange(MoreContext);
            MoreCheckForStatusLineChange(MoreContext);
        } else {
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
                    if (MoreContext->LinesInPage < MoreContext->ViewportHeight) {
                        break;
                    }
                }
            } else if (ObjectsToWaitFor[WaitObject - WAIT_OBJECT_0] == InHandle) {
                INPUT_RECORD InputRecords[20];
                PINPUT_RECORD InputRecord;
                DWORD ActuallyRead;
                DWORD CurrentIndex;
                BOOL Terminate = FALSE;
                BOOL RedrawStatus = FALSE;

                if (!ReadConsoleInput(InHandle, InputRecords, sizeof(InputRecords)/sizeof(InputRecords[0]), &ActuallyRead)) {
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

                        /*
                        if (InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_WHEELED) {
                            ReDisplayRequired |= MoreProcessMouseScroll(MoreContext, InputRecord, ButtonsPressed, &Terminate);
                        }
                        */

                        if (ReDisplayRequired) {
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
