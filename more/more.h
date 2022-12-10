/**
 * @file more/more.h
 *
 * Yori shell display master header
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

#include <yoripch.h>
#include <yorilib.h>

/**
 The maximum number of concurrent searches.  It's convenient to define this
 because there is an array of this size indicating the number of user
 configurable colors, with different arrays for strings to search for and
 settings for each.  These all need to correspond to each other.
 */
#define MORE_MAX_SEARCHES 10

/**
 Data describing a physical line.  A physical line is a line of text from the
 data source, which may take more characters than fit on a viewport line.
 */
typedef struct _MORE_PHYSICAL_LINE {

    /**
     A list of physical lines.  Paired with MORE_CONTEXT::PhysicalLineList
     and synchronized with MORE_CONTEXT::PhysicalLineMutex .
     */
    YORI_LIST_ENTRY LineList;

    /**
     Pointer to the referenced allocation that contains this physical line.
     */
    PVOID MemoryToFree;

    /**
     The color attribute to display at the beginning of the line.
     */
    WORD InitialColor;

    /**
     The number of this physical line within the input stream.  The first
     line is zero.
     */
    DWORDLONG LineNumber;

    /**
     The contents of the physical line.
     */
    YORI_STRING LineContents;
} MORE_PHYSICAL_LINE, *PMORE_PHYSICAL_LINE;

/**
 A logical line, meaning a line rendered for display on the console.
 */
typedef struct _MORE_LOGICAL_LINE {

    /**
     Pointer to the physical line whose data is being decomposed into this
     logical line.
     */
    PMORE_PHYSICAL_LINE PhysicalLine;

    /**
     The zero based index of this logical line from within the corresponding
     physical line.
     */
    DWORD LogicalLineIndex;

    /**
     The offset in characters from the PhysicalLine to the beginning of the
     string represented by this logical line.
     */
    DWORD PhysicalLineCharacterOffset;

    /**
     Characters remaining in any search match, if a search match commenced
     on a previous logical line from the same physical line.
     */
    DWORD CharactersRemainingInMatch;

    /**
     The color attribute to display at the beginning of the line.
     */
    WORD InitialDisplayColor;

    /**
     The color attribute at the beginning of the line as indicated by the
     input stream.  This can be different to display color where things like
     search are used which changes the color of the input stream.
     */
    WORD InitialUserColor;

    /**
     If TRUE, an explicit newline should be added after this line.  If FALSE,
     the console auto wraps and no newline should be issued.
     */
    BOOLEAN ExplicitNewlineRequired;

    /**
     If TRUE, there are more logical lines to follow this one that are derived
     from the same physical line.  If FALSE, this logical line is the end of
     the physical line.
     */
    BOOLEAN MoreLogicalLines;

    /**
     The string representation of the logical line.
     */
    YORI_STRING Line;
} MORE_LOGICAL_LINE, *PMORE_LOGICAL_LINE;

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
     Set to TRUE to indicate that once this line is displayed an explicit
     newline character should be written.  This allows a logical line to be
     generated referring only to characters within a physical line (no
     reallocation or double buffering) but still terminate the line before
     starting the next line.  This is typically FALSE because a logical line
     has written to the edge of the console window so that processing is
     resuming on the next console line without a newline present.
     */
    BOOLEAN ExplicitNewlineRequired;

    /**
     Set to TRUE to indicate the logical line needs to be parsed character
     by character because the contents of the logical line are not merely
     a subset of characters from a physical line.  FALSE if the line is
     just part of a physical line allocation.  Currently this is only used
     when a search is present so that the logical line contains extra
     escape sequences that the physical line does not.
     */
    BOOLEAN RequiresGeneration;

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
 Additional information about each search string.  This is seperated from the
 search strings to allow the search strings to be passed as-is when searching
 on each line.  Information about the string can be looked up after matches
 are located.
 */
typedef struct _MORE_SEARCH_CONTEXT {

    /**
     The color index for the search match.
     */
    UCHAR ColorIndex;

} MORE_SEARCH_CONTEXT, *PMORE_SEARCH_CONTEXT;

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _MORE_CONTEXT {

    /**
     A linked list of physical lines.
     */
    YORI_LIST_ENTRY PhysicalLineList;

    /**
     Synchronization around PhysicalLineList.
     */
    HANDLE PhysicalLineMutex;

    /**
     An event that is signalled when new lines are added to the
     PhysicalLineList in case the viewport thread wants to update display
     when lines are added.
     */
    HANDLE PhysicalLineAvailableEvent;

    /**
     An event that is signalled when the ingest process should be terminated
     quickly and the application should exit.
     */
    HANDLE ShutdownEvent;


    /**
     The current width of the window, in characters.
     */
    DWORD ViewportWidth;

    /**
     The current height of the window, in lines.  Note this corresponds to the
     number of elements in ViewportLines so updating it implies a
     reallocation.
     */
    DWORD ViewportHeight;

    /**
     Description of the current selected region.
     */
    YORILIB_SELECTION Selection;

    /**
     An array of size ViewportHeight of lines currently displayed.  Note these
     refer to the strings in the PhysicalLineList.
     */
    PMORE_LOGICAL_LINE DisplayViewportLines;

    /**
     An array of size ViewportHeight of lines that are being constructed to
     display in future.  Note these refer to the strings in the
     PhysicalLineList.
     */
    PMORE_LOGICAL_LINE StagingViewportLines;

    /**
     Specifies the total number of ingested lines when the status line was
     last calculated.
     */
    DWORDLONG TotalLinesInViewportStatus;

    /**
     Specifies the number of lines within DisplayViewportLines that are
     currently populated with data.  Since population is a process, this
     starts at zero and counts up to ViewportHeight.
     */
    DWORD LinesInViewport;

    /**
     The number of lines that have been displayed as part of a single page.
     If the user hits space or similar, this value is reset such that
     another ViewportHeight number of lines is processed.
     */
    DWORD LinesInPage;

    /**
     The number of command line arguments to use as input.  This can be zero
     if input is coming from a pipe.
     */
    DWORD InputSourceCount;

    /**
     The number of spaces to display for every tab.
     */
    DWORD TabWidth;

    /**
     The color attribute to display at the beginning of the application.
     */
    WORD InitialColor;

    /**
     Pointer to an array of length InputSourceCount for file specifications
     to process.
     */
    PYORI_STRING InputSources;

    /**
     An array of strings to search for.  These are compacted so the existence
     of one empty string implies no further strings following.  This means
     there is no correlation between an entry in this array and the color for
     that entry.
     */
    YORI_STRING SearchStrings[MORE_MAX_SEARCHES];

    /**
     An array of additional information about each search string above.  This
     array currently contains information about the color to use for the
     search match.
     */
    MORE_SEARCH_CONTEXT SearchContext[MORE_MAX_SEARCHES];

    /**
     An array of colors to use to display search matches.
     */
    UCHAR SearchColors[MORE_MAX_SEARCHES];

    /**
     Indicates the current color that the user is manipulating.  This can
     be 0-8, where the user switches them with Ctrl+1 - Ctrl+9.
     */
    UCHAR SearchColorIndex;

    /**
     Handle to the thread that is adding to the physical line array.
     */
    HANDLE IngestThread;

    /**
     TRUE if we are in search mode, meaning that keystrokes will be applied
     to the active SearchString.  The active SearchString is identified by
     SearchColorIndex.  FALSE if keystrokes imply navigation.
     */
    BOOLEAN SearchUiActive;

    /**
     TRUE if the status line needs to be redrawn as a result of a search
     string change.
     */
    BOOLEAN SearchDirty;

    /**
     TRUE if the display implies that text at the last cell in a line auto
     wraps to the next line.  This behavior is generally undesirable on NT,
     because it updates the attributes of the new line to match the final
     cell on the previous line, and resizing the window will display the
     text in a different location.  On Nano, we have no choice and need to
     operate in this degraded way.
     */
    BOOLEAN AutoWrapAtLineEnd;

    /**
     TRUE if the set of files should be enumerated recursively.  FALSE if they
     should be interpreted as files and not recursed.
     */
    BOOLEAN Recursive;

    /**
     TRUE if enumeration should not expand {}, [], or similar operators.
     FALSE if these should be expanded.
     */
    BOOLEAN BasicEnumeration;

    /**
     TRUE if the display should be the debug version which clears the screen
     and dumps the internal buffer on any display change.  This helps to
     clarify the state of the system, but is much slower than just telling
     the console about changes and moving things in the console buffer.
     */
    BOOLEAN DebugDisplay;

    /**
     TRUE if out of memory occurred and viewport can't intelligently keep
     displaying results.  This can happen because there's no memory to
     ingest more data, or because the data we have cannot be rendered.
     */
    BOOLEAN OutOfMemory;

    /**
     TRUE if the user has pressed Ctrl+Q in order to suspend pagination.
     */
    BOOLEAN SuspendPagination;

    /**
     TRUE if when reading files, this program should continually wait for
     more data to be added.  This is useful where a file is being extended
     continually by another program, but it implies that this program cannot
     move to the next file.  FALSE if this program should read until the end
     of each file and move to the next.
     */
    BOOLEAN WaitForMore;

    /**
     Records the total number of files processed.
     */
    DWORDLONG FilesFound;

    /**
     Records the total number of lines processed.
     */
    DWORDLONG LineCount;

} MORE_CONTEXT, *PMORE_CONTEXT;

VOID
MoreGetViewportDimensions(
    __in PCONSOLE_SCREEN_BUFFER_INFO ScreenInfo,
    __out PDWORD ViewportWidth,
    __out PDWORD ViewportHeight
    );

__success(return)
BOOL
MoreAllocateViewportStructures(
    __in DWORD ViewportWidth,
    __in DWORD ViewportHeight,
    __out PMORE_LOGICAL_LINE * DisplayViewportLines,
    __out PMORE_LOGICAL_LINE * StagingViewportLines
    );

BOOL
MoreInitContext(
    __inout PMORE_CONTEXT MoreContext,
    __in DWORD ArgCount,
    __in_opt PYORI_STRING ArgStrings,
    __in BOOLEAN Recursive,
    __in BOOLEAN BasicEnumeration,
    __in BOOLEAN DebugDisplay,
    __in BOOLEAN SuspendPagination,
    __in BOOLEAN WaitForMore
    );

VOID
MoreCleanupContext(
    __inout PMORE_CONTEXT MoreContext
    );

VOID
MoreGracefulExit(
    __inout PMORE_CONTEXT MoreContext
    );

DWORD WINAPI
MoreIngestThread(
    __in LPVOID Context
    );

BOOL
MoreViewportDisplay(
    __inout PMORE_CONTEXT MoreContext
    );

UCHAR
MoreSearchIndexForColorIndex(
    __in PMORE_CONTEXT MoreContext,
    __in UCHAR ColorIndex
    );

UCHAR
MoreSearchCountActive(
    __in PMORE_CONTEXT MoreContext
    );

VOID
MoreSearchIndexFree(
    __in PMORE_CONTEXT MoreContext,
    __in UCHAR SearchIndex
    );

VOID
MoreTruncateStringToVisibleChars(
    __in PYORI_STRING String,
    __in DWORD VisibleChars
    );

VOID
MoreMoveLogicalLine(
    __inout PMORE_LOGICAL_LINE Dest,
    __in PMORE_LOGICAL_LINE Src
    );

VOID
MoreCloneLogicalLine(
    __inout PMORE_LOGICAL_LINE Dest,
    __in PMORE_LOGICAL_LINE Src
    );

DWORD
MoreGetLogicalLineLength(
    __in PMORE_CONTEXT MoreContext,
    __in PYORI_STRING PhysicalLineSubset,
    __in DWORD MaximumVisibleCharacters,
    __in WORD InitialDisplayColor,
    __in WORD InitialUserColor,
    __in DWORD CharactersRemainingInMatch,
    __out_opt PMORE_LINE_END_CONTEXT LineEndContext
    );

__success(return)
DWORD
MoreGetNextLogicalLines(
    __inout PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE CurrentLine,
    __in BOOL StartFromNextLine,
    __in DWORD LinesToOutput,
    __out_ecount(*NumberLinesGenerated) PMORE_LOGICAL_LINE OutputLines,
    __out PDWORD NumberLinesGenerated
    );

__success(return)
BOOL
MoreGetPreviousLogicalLines(
    __inout PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE CurrentLine,
    __in DWORD LinesToOutput,
    __out_ecount(*NumberLinesGenerated) PMORE_LOGICAL_LINE OutputLines,
    __out PDWORD NumberLinesGenerated
    );

__success(return != NULL)
PMORE_PHYSICAL_LINE
MoreFindNextLineWithSearchMatch(
    __in PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE PreviousMatchLine,
    __in BOOLEAN MatchAny,
    __in DWORD MaxLogicalLinesMoved,
    __out_opt PDWORD LogicalLinesMoved
    );

__success(return != NULL)
PMORE_PHYSICAL_LINE
MoreFindPreviousLineWithSearchMatch(
    __in PMORE_CONTEXT MoreContext,
    __in_opt PMORE_LOGICAL_LINE PreviousMatchLine,
    __in BOOLEAN MatchAny,
    __in DWORD MaxLogicalLinesMoved,
    __out_opt PDWORD LogicalLinesMoved
    );

// vim:sw=4:ts=4:et:
