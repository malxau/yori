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
 Information about a single tab complete match.
 */
typedef struct _YORI_TAB_COMPLETE_MATCH {

    /**
     The list entry for this match.  Paired with @ref
     YORI_TAB_COMPLETE_CONTEXT::MatchList .
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The string corresponding to this match.
     */
    YORI_STRING YsValue;

} YORI_TAB_COMPLETE_MATCH, *PYORI_TAB_COMPLETE_MATCH;

/**
 Information about the state of tab completion.
 */
typedef struct _YORI_TAB_COMPLETE_CONTEXT {

    /**
     Indicates the number of times tab has been repeatedly pressed.  This
     is reset if any other key is pressed instead of tab.  It is used to
     determine if the tab context requires initialization for the first
     tab, and where to resume from for later tabs.
     */
    DWORD TabCount;

    /**
     Indicates which data source to search through.
     */
    enum {
        YoriTabCompleteSearchExecutables = 1,
        YoriTabCompleteSearchFiles = 2,
        YoriTabCompleteSearchHistory = 3
    } SearchType;

    /**
     A list of matches that apply to the criteria that was searched.
     */
    YORI_LIST_ENTRY MatchList;

    /**
     Pointer to the previously returned match.  If the user repeatedly hits
     tab, we advance to the next match.
     */
    PYORI_TAB_COMPLETE_MATCH PreviousMatch;

    /**
     The matching criteria that is being searched for.  This is typically
     the string that was present when the user first hit tab followed by
     a "*".
     */
    YORI_STRING SearchString;

} YORI_TAB_COMPLETE_CONTEXT, *PYORI_TAB_COMPLETE_CONTEXT;

/**
 The context of a line that is currently being entered by the user.
 */
typedef struct _YORI_INPUT_BUFFER {

    /**
     Pointer to a string containing the text as being entered by the user.
     */
    YORI_STRING String;

    /**
     The current offset within @ref String that the user is modifying.
     */
    DWORD CurrentOffset;

    /**
     The number of characters that were filled in prior to a key press
     being evaluated.
     */
    DWORD PreviousMaxPopulated;

    /**
     The current position that was selected prior to a key press being
     evaluated.
     */
    DWORD PreviousCurrentOffset;

    /**
     The number of times the tab key had been pressed prior to a key being
     evaluated.
     */
    DWORD PriorTabCount;

    /**
     The first character in the buffer that may have changed since the last
     draw.
     */
    DWORD DirtyBeginOffset;

    /**
     The last character in the buffer that may have changed since the last
     draw.
     */
    DWORD DirtyLength;

    /**
     Extra information specific to tab completion processing.
     */
    YORI_TAB_COMPLETE_CONTEXT TabContext;
} YORI_INPUT_BUFFER, *PYORI_INPUT_BUFFER;

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

    if (Buffer->PriorTabCount == Buffer->TabContext.TabCount) {
        PYORI_LIST_ENTRY ListEntry = NULL;
        PYORI_TAB_COMPLETE_MATCH Match;

        YoriLibFreeStringContents(&Buffer->TabContext.SearchString);

        ListEntry = YoriLibGetNextListEntry(&Buffer->TabContext.MatchList, NULL);
        while (ListEntry != NULL) {
            Match = CONTAINING_RECORD(ListEntry, YORI_TAB_COMPLETE_MATCH, ListEntry);
            ListEntry = YoriLibGetNextListEntry(&Buffer->TabContext.MatchList, ListEntry);

            YoriLibFreeStringContents(&Match->YsValue);
            YoriLibDereference(Match);
        }

        ZeroMemory(&Buffer->TabContext, sizeof(Buffer->TabContext));
        Buffer->PriorTabCount = 0;
    }
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
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    WritePosition.X = 0;
    WritePosition.Y = 0;
    FillPosition.X = 0;
    FillPosition.Y = 0;

    // MSFIX Error handling
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo);

    //
    //  Calculate the number of characters truncated from the currently
    //  displayed buffer.
    //

    if (Buffer->PreviousMaxPopulated > Buffer->String.LengthInChars) {
        NumberToFill = Buffer->PreviousMaxPopulated - Buffer->String.LengthInChars;
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

    if (NumberToFill) {
        YoriShDetermineCellLocationIfMoved(-1 * Buffer->PreviousCurrentOffset + Buffer->String.LengthInChars + NumberToFill);
        FillPosition = YoriShDetermineCellLocationIfMoved(-1 * Buffer->PreviousCurrentOffset + Buffer->String.LengthInChars);
    }

    //
    //  Now that we know where the text should go, advance the cursor
    //  and render the text.
    //

    YoriShMoveCursor(Buffer->CurrentOffset - Buffer->PreviousCurrentOffset);

    if (NumberToWrite) {
        WriteConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), &Buffer->String.StartOfString[Buffer->DirtyBeginOffset], NumberToWrite, WritePosition, &NumberWritten);
        FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ScreenInfo.wAttributes, NumberToWrite, WritePosition, &NumberWritten);
    }

    //
    //  If there are additional cells to empty due to truncation, display
    //  those now.
    //

    if (NumberToFill) {
        FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', NumberToFill, FillPosition, &NumberWritten);
        FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ScreenInfo.wAttributes, NumberToFill, FillPosition, &NumberWritten);
    }

    Buffer->PreviousCurrentOffset = Buffer->CurrentOffset;
    Buffer->PreviousMaxPopulated = Buffer->String.LengthInChars;
    Buffer->DirtyBeginOffset = 0;
    Buffer->DirtyLength = 0;
}

/**
 Check that the buffer has enough characters to hold the new number of
 characters.  If it doesn't, reallocate a new buffer that is large
 enough to hold the new number of characters.  Note that since this is
 an allocation it can fail.

 @param Buffer Pointer to the current buffer.

 @param CharactersNeeded The number of characters that are needed in
        the buffer.

 @return TRUE to indicate the current buffer is large enough or it was
         successfully reallocated, FALSE to indicate allocation failure.
 */
BOOL
YoriShEnsureStringHasEnoughCharacters(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in DWORD CharactersNeeded
    )
{
    //
    //  If the current buffer can't hold the requested size plus a NULL,
    //  extend the buffer.
    //

    while (CharactersNeeded + 1 > Buffer->String.LengthAllocated) {
        YORI_STRING NewString;

        if (!YoriLibAllocateString(&NewString, Buffer->String.LengthAllocated * 4)) {
            return FALSE;
        }

        CopyMemory(NewString.StartOfString, Buffer->String.StartOfString, Buffer->String.LengthInChars * sizeof(TCHAR));
        NewString.LengthInChars = Buffer->String.LengthInChars;

        YoriLibFreeStringContents(&Buffer->String);
        CopyMemory(&Buffer->String, &NewString, sizeof(YORI_STRING));
    }

    return TRUE;
}

/**
 Apply incoming characters to an input buffer.

 @param Buffer The input buffer to apply new characters to.

 @param String A yori string to enter into the buffer.

 @param InsertMode If TRUE, the string should be inserted into the buffer;
        if FALSE, the string should be overwritten into the buffer.
 */
VOID
YoriShAddYoriStringToInput(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in PYORI_STRING String,
    __in BOOLEAN InsertMode
    )
{
    //
    //  Need more allocated than populated due to NULL termination
    //

    ASSERT(Buffer->String.LengthAllocated > Buffer->String.LengthInChars);
    ASSERT(Buffer->String.LengthInChars >= Buffer->CurrentOffset);

    //
    //  If we're inserting, shuffle the data; if we're overwriting, clobber
    //  the data.
    //

    if (InsertMode) {
        if (!YoriShEnsureStringHasEnoughCharacters(Buffer, Buffer->String.LengthInChars + String->LengthInChars)) {
            return;
        }

        if (Buffer->String.LengthInChars != Buffer->CurrentOffset) {
            MoveMemory(&Buffer->String.StartOfString[Buffer->CurrentOffset + String->LengthInChars], &Buffer->String.StartOfString[Buffer->CurrentOffset], (Buffer->String.LengthInChars - Buffer->CurrentOffset) * sizeof(TCHAR));
        }
        Buffer->String.LengthInChars += String->LengthInChars;
        CopyMemory(&Buffer->String.StartOfString[Buffer->CurrentOffset], String->StartOfString, String->LengthInChars * sizeof(TCHAR));

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
        if (!YoriShEnsureStringHasEnoughCharacters(Buffer, Buffer->CurrentOffset + String->LengthInChars)) {
            return;
        }
        CopyMemory(&Buffer->String.StartOfString[Buffer->CurrentOffset], String->StartOfString, String->LengthInChars * sizeof(TCHAR));
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

 @param InsertMode TRUE if the text should be inserted into the buffer,
        FALSE if it should be overwritten into the buffer.
 */
VOID
YoriShAddCStringToInput(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in LPTSTR String,
    __in BOOLEAN InsertMode
    )
{
    YORI_STRING YoriString;

    YoriLibConstantString(&YoriString, String);
    YoriShAddYoriStringToInput(Buffer, &YoriString, InsertMode);
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
    YoriShPostKeyPress(Buffer);
    Buffer->String.StartOfString[Buffer->String.LengthInChars] = '\0';
    YoriShMoveCursor(Buffer->String.LengthInChars - Buffer->CurrentOffset);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
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
    Buffer->String.LengthInChars = 0;
    Buffer->CurrentOffset = 0;
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
        MoveMemory(&Buffer->String.StartOfString[Buffer->CurrentOffset - CountToUse], &Buffer->String.StartOfString[Buffer->CurrentOffset], (Buffer->String.LengthInChars - Buffer->CurrentOffset) * sizeof(TCHAR));
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
}

/**
 Populates the list of matches for a command history tab completion.  This
 function searches the history for matching commands in MRU order and
 populates the list with the result.

 @param TabContext Pointer to the tab completion context.  This provides
        the search criteria and has its match list populated with results
        on success.

 @param ExpandFullPath Specifies if full path expansion should be performed.
        For executable matches, full path expansion is always performed.
 */
VOID
YoriShPerformHistoryTabCompletion(
    __inout PYORI_TAB_COMPLETE_CONTEXT TabContext,
    __in BOOL ExpandFullPath
    )
{
    LPTSTR FoundPath;
    DWORD CompareLength;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_HISTORY_ENTRY HistoryEntry;
    PYORI_TAB_COMPLETE_MATCH Match;

    UNREFERENCED_PARAMETER(ExpandFullPath);

    //
    //  Set up state necessary for different types of searching.
    //

    FoundPath = _tcschr(TabContext->SearchString.StartOfString, '*');
    if (FoundPath != NULL) {
        CompareLength = (DWORD)(FoundPath - TabContext->SearchString.StartOfString);
    } else {
        CompareLength = TabContext->SearchString.LengthInChars;
    }
    FoundPath = NULL;

    //
    //  Search the list of history.
    //

    ListEntry = YoriLibGetPreviousListEntry(&YoriShCommandHistory, NULL);
    while (ListEntry != NULL) {
        HistoryEntry = CONTAINING_RECORD(ListEntry, YORI_HISTORY_ENTRY, ListEntry);

        if (YoriLibCompareStringInsensitiveCount(&HistoryEntry->CmdLine, &TabContext->SearchString, CompareLength) == 0) {

            //
            //  Allocate a match entry for this file.
            //
    
            Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (HistoryEntry->CmdLine.LengthInChars + 1) * sizeof(TCHAR));
            if (Match == NULL) {
                return;
            }
    
            //
            //  Populate the file into the entry.
            //
    
            YoriLibInitEmptyString(&Match->YsValue);
            Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
            YoriLibReference(Match);
            Match->YsValue.MemoryToFree = Match;
            YoriLibSPrintf(Match->YsValue.StartOfString, _T("%y"), &HistoryEntry->CmdLine);
            Match->YsValue.LengthInChars = HistoryEntry->CmdLine.LengthInChars;
    
            //
            //  Append to the list.
            //
    
            YoriLibAppendList(&TabContext->MatchList, &Match->ListEntry);
        }
        ListEntry = YoriLibGetPreviousListEntry(&YoriShCommandHistory, ListEntry);
    }
}

/**
 A callback function that is invoked by the path resolver to add any
 candidate programs to the tab completion list.

 @param FoundPath A match that was found when enumerating through the path.

 @param Context Pointer to the tab complete context to populate with the new
        match.
 */
BOOL
YoriShAddToTabList(
    __in PYORI_STRING FoundPath,
    __in PVOID Context
    )
{
    PYORI_TAB_COMPLETE_MATCH Match;
    PYORI_TAB_COMPLETE_MATCH Existing;
    PYORI_TAB_COMPLETE_CONTEXT TabContext = (PYORI_TAB_COMPLETE_CONTEXT)Context;
    PYORI_LIST_ENTRY ListEntry;


    //
    //  Allocate a match entry for this file.
    //

    Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (FoundPath->LengthInChars + 1) * sizeof(TCHAR));
    if (Match == NULL) {
        return FALSE;
    }

    //
    //  Populate the file into the entry.
    //

    YoriLibInitEmptyString(&Match->YsValue);
    Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
    YoriLibReference(Match);
    Match->YsValue.MemoryToFree = Match;
    memcpy(Match->YsValue.StartOfString, FoundPath->StartOfString, FoundPath->LengthInChars * sizeof(TCHAR));
    Match->YsValue.LengthInChars = FoundPath->LengthInChars;
    Match->YsValue.LengthAllocated = FoundPath->LengthInChars + 1;
    Match->YsValue.StartOfString[Match->YsValue.LengthInChars] = '\0';

    //
    //  Insert into the list if no duplicate is found.
    //

    ListEntry = YoriLibGetNextListEntry(&TabContext->MatchList, NULL);
    do {
        if (ListEntry == NULL) {
            YoriLibAppendList(&TabContext->MatchList, &Match->ListEntry);
            Match = NULL;
            break;
        }
        Existing = CONTAINING_RECORD(ListEntry, YORI_TAB_COMPLETE_MATCH, ListEntry);
        if (YoriLibCompareStringInsensitive(&Match->YsValue, &Existing->YsValue) == 0) {
            break;
        }
        ListEntry = YoriLibGetNextListEntry(&TabContext->MatchList, ListEntry);

    } while(TRUE);

    if (Match != NULL) {
        YoriLibFreeStringContents(&Match->YsValue);
        YoriLibDereference(Match);
    }

    return TRUE;
}


/**
 Populates the list of matches for an executable tab completion.  This
 function searches the path for matching binaries in execution order
 and populates the list with the result.

 @param TabContext Pointer to the tab completion context.  This provides
        the search criteria and has its match list populated with results
        on success.

 @param ExpandFullPath Specifies if full path expansion should be performed.
        For executable matches, full path expansion is always performed.
 */
VOID
YoriShPerformExecutableTabCompletion(
    __inout PYORI_TAB_COMPLETE_CONTEXT TabContext,
    __in BOOL ExpandFullPath
    )
{
    LPTSTR FoundPath;
    YORI_STRING FoundExecutable;
    BOOL Result;
    PYORI_TAB_COMPLETE_MATCH Match;
    YORI_STRING AliasStrings;
    DWORD CompareLength;

    UNREFERENCED_PARAMETER(ExpandFullPath);

    //
    //  Set up state necessary for different types of searching.
    //

    FoundPath = _tcschr(TabContext->SearchString.StartOfString, '*');
    if (FoundPath != NULL) {
        CompareLength = (DWORD)(FoundPath - TabContext->SearchString.StartOfString);
    } else {
        CompareLength = TabContext->SearchString.LengthInChars;
    }

    //
    //  Firstly, search the table of aliases.
    //

    YoriLibInitEmptyString(&AliasStrings);
    if (YoriShGetAliasStrings(TRUE, &AliasStrings)) {
        LPTSTR ThisAlias;
        LPTSTR AliasValue;
        DWORD AliasLength;
        DWORD AliasNameLength;


        ThisAlias = AliasStrings.StartOfString;
        while (*ThisAlias != '\0') {
            AliasNameLength = 
            AliasLength = _tcslen(ThisAlias);

            //
            //  Look at the alias name only, not what it maps to.
            //

            AliasValue = _tcschr(ThisAlias, '=');
            ASSERT(AliasValue != NULL);
            if (AliasValue != NULL) {
                *AliasValue = '\0';
                AliasNameLength = (DWORD)((PUCHAR)AliasValue - (PUCHAR)ThisAlias);
            }

            if (YoriLibCompareStringWithLiteralInsensitiveCount(&TabContext->SearchString, ThisAlias, CompareLength) == 0) {

                //
                //  Allocate a match entry for this file.
                //
        
                Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (AliasNameLength + 1) * sizeof(TCHAR));
                if (Match == NULL) {
                    YoriLibFreeStringContents(&AliasStrings);
                    return;
                }
        
                //
                //  Populate the file into the entry.
                //
        
                YoriLibInitEmptyString(&Match->YsValue);
                Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
                YoriLibReference(Match);
                Match->YsValue.MemoryToFree = Match;
                _tcscpy(Match->YsValue.StartOfString, ThisAlias);
                Match->YsValue.LengthInChars = AliasNameLength;
        
                //
                //  Append to the list.
                //
        
                YoriLibAppendList(&TabContext->MatchList, &Match->ListEntry);
            }

            //
            //  Move to the next alias
            //

            ThisAlias += AliasLength;
            ThisAlias++;
        }
        YoriLibFreeStringContents(&AliasStrings);
    }

    //
    //  Secondly, search for the object in the PATH, resuming after the
    //  previous search
    //

    YoriLibInitEmptyString(&FoundExecutable);
    Result = YoriLibLocateExecutableInPath(&TabContext->SearchString, YoriShAddToTabList, TabContext, &FoundExecutable);
    ASSERT(FoundExecutable.StartOfString == NULL);

    //
    //  Thirdly, search the table of builtins.
    //

    if (YoriShBuiltins != NULL) {
        LPTSTR ThisBuiltin;
        DWORD BuiltinLength;

        ThisBuiltin = YoriShBuiltins;
        while (*ThisBuiltin != '\0') {
            BuiltinLength = _tcslen(ThisBuiltin);

            if (YoriLibCompareStringWithLiteralInsensitiveCount(&TabContext->SearchString, ThisBuiltin, CompareLength) == 0) {

                //
                //  Allocate a match entry for this file.
                //
        
                Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (BuiltinLength + 1) * sizeof(TCHAR));
                if (Match == NULL) {
                    return;
                }
        
                //
                //  Populate the file into the entry.
                //
        
                YoriLibInitEmptyString(&Match->YsValue);
                Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
                YoriLibReference(Match);
                Match->YsValue.MemoryToFree = Match;
                _tcscpy(Match->YsValue.StartOfString, ThisBuiltin);
                Match->YsValue.LengthInChars = BuiltinLength;
        
                //
                //  Append to the list.
                //
        
                YoriLibAppendList(&TabContext->MatchList, &Match->ListEntry);
            }

            //
            //  Move to the next alias
            //

            ThisBuiltin += BuiltinLength;
            ThisBuiltin++;
        }
    }
}

/**
 Context information for a file based tab completion.
 */
typedef struct _YORI_FILE_COMPLETE_CONTEXT {

    /**
     The tab completion context to populate with any matches.
     */
    PYORI_TAB_COMPLETE_CONTEXT TabContext;

    /**
     Extra characters to include at the beginning of any found match.
     */
    YORI_STRING Prefix;

    /**
     The string to search for.
     */
    LPTSTR SearchString;

    /**
     The number of characters in the SearchString until the final slash.
     This is used to distinguish where to search from what to search for.
     */
    DWORD CharsToFinalSlash;

    /**
     The number of files that have been found.
     */
    DWORD FilesFound;

    /**
     If TRUE, the resulting tab completion should expand the entire path,
     if FALSE it should only expand the file name (inside the specified
     directory, if present.)
     */
    BOOL ExpandFullPath;
} YORI_FILE_COMPLETE_CONTEXT, *PYORI_FILE_COMPLETE_CONTEXT;

/**
 Invoked for each file matching a file based tab completion pattern.

 @param Filename Pointer to a string containing the full file name.

 @param FileInfo Pointer to the block of information returned by directory
        enumeration describing the entry.

 @param Depth Specifies the recursion depth.  Ignored in this function.

 @param Context Pointer to a context block describing the tab completion
        context to populate with the entry and properties for the population.

 @return TRUE to continue enumerating, FALSE to stop.
 */
BOOL
YoriShFileTabCompletionCallback(
    __in PYORI_STRING Filename,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PYORI_FILE_COMPLETE_CONTEXT Context
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_TAB_COMPLETE_MATCH Match;
    PYORI_TAB_COMPLETE_MATCH Existing;

    UNREFERENCED_PARAMETER(Depth);

    if (Context->ExpandFullPath) {

        //
        //  Allocate a match entry for this file.
        //

        Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (Context->Prefix.LengthInChars + Filename->LengthInChars + 1) * sizeof(TCHAR));
        if (Match == NULL) {
            return FALSE;
        }

        //
        //  Populate the file into the entry.
        //

        YoriLibInitEmptyString(&Match->YsValue);
        Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
        YoriLibReference(Match);
        Match->YsValue.MemoryToFree = Match;

        if (Context->Prefix.LengthInChars > 0) {
            CopyMemory(Match->YsValue.StartOfString, Context->Prefix.StartOfString, Context->Prefix.LengthInChars * sizeof(TCHAR));
        }
        CopyMemory(&Match->YsValue.StartOfString[Context->Prefix.LengthInChars], Filename->StartOfString, Filename->LengthInChars * sizeof(TCHAR));
        Match->YsValue.StartOfString[Context->Prefix.LengthInChars + Filename->LengthInChars] = '\0';
        Match->YsValue.LengthInChars = Context->Prefix.LengthInChars + Filename->LengthInChars;
    } else {
        DWORD CharsInFileName;
        CharsInFileName = _tcslen(FileInfo->cFileName);

        //
        //  Allocate a match entry for this file.
        //
    
        Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (Context->Prefix.LengthInChars + Context->CharsToFinalSlash + CharsInFileName + 1) * sizeof(TCHAR));
        if (Match == NULL) {
            return FALSE;
        }
    
        //
        //  Populate the file into the entry.
        //
    
        YoriLibInitEmptyString(&Match->YsValue);
        Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
        YoriLibReference(Match);
        Match->YsValue.MemoryToFree = Match;
    
        if (Context->Prefix.LengthInChars > 0) {
            CopyMemory(Match->YsValue.StartOfString, Context->Prefix.StartOfString, Context->Prefix.LengthInChars * sizeof(TCHAR));
        }
        if (Context->CharsToFinalSlash > 0) {
            CopyMemory(&Match->YsValue.StartOfString[Context->Prefix.LengthInChars], Context->SearchString, Context->CharsToFinalSlash * sizeof(TCHAR));
        }
        CopyMemory(&Match->YsValue.StartOfString[Context->Prefix.LengthInChars + Context->CharsToFinalSlash], FileInfo->cFileName, CharsInFileName * sizeof(TCHAR));
        Match->YsValue.StartOfString[Context->Prefix.LengthInChars + Context->CharsToFinalSlash + CharsInFileName] = '\0';
        Match->YsValue.LengthInChars = Context->Prefix.LengthInChars + Context->CharsToFinalSlash + CharsInFileName;
    }

    //
    //  Insert into the list in lexicographical order.
    //

    ListEntry = YoriLibGetNextListEntry(&Context->TabContext->MatchList, NULL);
    do {
        if (ListEntry == NULL) {
            YoriLibAppendList(&Context->TabContext->MatchList, &Match->ListEntry);
            break;
        }
        Existing = CONTAINING_RECORD(ListEntry, YORI_TAB_COMPLETE_MATCH, ListEntry);
        if (YoriLibCompareStringInsensitive(&Match->YsValue, &Existing->YsValue) < 0) {
            YoriLibAppendList(ListEntry, &Match->ListEntry);
            break;
        }
        ListEntry = YoriLibGetNextListEntry(&Context->TabContext->MatchList, ListEntry);
    } while(TRUE);

    Context->FilesFound++;

    return TRUE;
}

/**
 A structure describing a string which when encountered in a string used for
 file tab completion may indicate the existence of a file.
 */
typedef struct _YORI_TAB_FILE_HEURISTIC_MATCH {

    /**
     The string to match against.
     */
    LPCTSTR MatchString;

    /**
     The offset, from the beginning of the matched string, to where the file
     name would be.  Note this value can be negative, indicating a match of a
     string within a file name.
     */
    INT CharsToSkip;
} YORI_TAB_FILE_HEURISTIC_MATCH, *PYORI_TAB_FILE_HEURISTIC_MATCH;

/**
 A list of strings which may, heuristically, indicate a good place to look for
 file names.
 */
YORI_TAB_FILE_HEURISTIC_MATCH YoriShTabHeuristicMatches[] = {
    {_T(":\\"), -1},
    {_T("\\\\"), 0},
    {_T(">>"), 2},
    {_T(">"), 1},
    {_T(":"), 1},
    {_T("="), 1}
};

/**
 Find the final seperator or colon in event of a drive letter colon
 prefix string, such that the criteria being searched for can be
 seperated from the location of the search.

 @param String Pointer to the string to locate the final seperator
        in.

 @return The index of the seperator, which may be zero to indicate
         one was not found.
 */
DWORD
YoriShFindFinalSlashIfSpecified(
    __in PYORI_STRING String
    )
{
    DWORD CharsInFileName;

    CharsInFileName = String->LengthInChars;

    while (CharsInFileName > 0) {
        if (YoriLibIsSep(String->StartOfString[CharsInFileName - 1])) {

            break;
        }

        if (CharsInFileName == 2 &&
            YoriLibIsDriveLetterWithColon(String)) {

            break;
        }

        CharsInFileName--;
    }

    return CharsInFileName;
}

/**
 Populates the list of matches for a file based tab completion.  This
 function searches the path for matching files in lexicographic order
 and populates the list with the result.

 @param TabContext Pointer to the tab completion context.  This provides
        the search criteria and has its match list populated with results
        on success.

 @param ExpandFullPath Specifies if full path expansion should be performed.
 */
VOID
YoriShPerformFileTabCompletion(
    __inout PYORI_TAB_COMPLETE_CONTEXT TabContext,
    __in BOOL ExpandFullPath
    )
{
    YORI_FILE_COMPLETE_CONTEXT EnumContext;
    YORI_STRING YsSearchString;
    DWORD PrefixLen;

    YoriLibInitEmptyString(&YsSearchString);
    YsSearchString.StartOfString = TabContext->SearchString.StartOfString;
    YsSearchString.LengthInChars = TabContext->SearchString.LengthInChars;
    YsSearchString.LengthAllocated = TabContext->SearchString.LengthAllocated;

    PrefixLen = sizeof("file:///") - 1;

    if (YoriLibCompareStringWithLiteralInsensitiveCount(&YsSearchString, _T("file:///"), PrefixLen) == 0) {
        YsSearchString.StartOfString += PrefixLen;
        YsSearchString.LengthInChars -= PrefixLen;
        YsSearchString.LengthAllocated -= PrefixLen;
    }

    EnumContext.ExpandFullPath = ExpandFullPath;
    EnumContext.CharsToFinalSlash = YoriShFindFinalSlashIfSpecified(&YsSearchString);
    EnumContext.SearchString = YsSearchString.StartOfString;
    YoriLibInitEmptyString(&EnumContext.Prefix);
    EnumContext.TabContext = TabContext;
    EnumContext.FilesFound = 0;
    YoriLibForEachFile(&YsSearchString, YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES, 0, YoriShFileTabCompletionCallback, &EnumContext);

    if (EnumContext.FilesFound == 0) {
        DWORD Count = sizeof(YoriShTabHeuristicMatches)/sizeof(YoriShTabHeuristicMatches[0]);
        DWORD Index;
        DWORD StringOffsetOfMatch;
        PYORI_STRING MatchArray;
        PYORI_STRING FoundMatch;

        MatchArray = YoriLibMalloc(sizeof(YORI_STRING) * Count);
        if (MatchArray == NULL) {
            return;
        }

        for (Index = 0; Index < Count; Index++) {
            YoriLibConstantString(&MatchArray[Index], YoriShTabHeuristicMatches[Index].MatchString);
        }

        FoundMatch = YoriLibFindFirstMatchingSubstring(&YsSearchString, Count, MatchArray, &StringOffsetOfMatch);
        if (FoundMatch == NULL) {
            YoriLibFree(MatchArray);
            return;
        }

        for (Index = 0; Index < Count; Index++) {
            if (FoundMatch->StartOfString == MatchArray[Index].StartOfString) {
                break;
            }
        }

        if (Index == Count) {
            ASSERT(Index != Count);
            YoriLibFree(MatchArray);
            return;
        }

        //
        //  If the file would begin before the beginning of the string, stop.
        //

        if (YoriShTabHeuristicMatches[Index].CharsToSkip < 0 &&
            (DWORD)(-1 * YoriShTabHeuristicMatches[Index].CharsToSkip) > YsSearchString.LengthInChars) {

            YoriLibFree(MatchArray);
            return;
        }

        //
        //  If the file would begin beyond the end of the string, stop.
        //

        if (YoriShTabHeuristicMatches[Index].CharsToSkip > 0 &&
            StringOffsetOfMatch + (DWORD)YoriShTabHeuristicMatches[Index].CharsToSkip >= YsSearchString.LengthInChars) {

            YoriLibFree(MatchArray);
            return;
        }

        //
        //  Seperate the string between the file portion (that we're looking
        //  for) and a prefix to append to any match.
        //

        EnumContext.Prefix.StartOfString = YsSearchString.StartOfString;
        EnumContext.Prefix.LengthInChars = StringOffsetOfMatch + YoriShTabHeuristicMatches[Index].CharsToSkip;

        YsSearchString.StartOfString += EnumContext.Prefix.LengthInChars;
        YsSearchString.LengthInChars -= EnumContext.Prefix.LengthInChars;

        EnumContext.CharsToFinalSlash = YoriShFindFinalSlashIfSpecified(&YsSearchString);
        EnumContext.SearchString = YsSearchString.StartOfString;

        YoriLibForEachFile(&YsSearchString, YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES, 0, YoriShFileTabCompletionCallback, &EnumContext);

        YoriLibFree(MatchArray);
    }
    return;
}

/**
 Perform tab completion processing.  On error the buffer is left unchanged.

 @param Buffer Pointer to the current input context.

 @param ExpandFullPath If TRUE, the path should be expanded to contain a fully
        specified path.  If FALSE, a minimal or relative path should be used.

 @param SearchHistory Specifies that tab completion should search through
        command history for matches rather than executable matches.
 */
VOID
YoriShTabCompletion(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in BOOL ExpandFullPath,
    __in BOOL SearchHistory
    )
{
    DWORD SearchLength;
    YORI_CMD_CONTEXT CmdContext;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_TAB_COMPLETE_MATCH Match;

    Buffer->TabContext.TabCount++;
    if (Buffer->String.LengthInChars == 0) {
        return;
    }
    if (!YoriShParseCmdlineToCmdContext(&Buffer->String, Buffer->CurrentOffset, &CmdContext)) {
        return;
    }

    if (CmdContext.argc == 0) {
        YoriShFreeCmdContext(&CmdContext);
        return;
    }

    //
    //  If we're searching for the first time, set up the search
    //  criteria and populate the list of matches.
    //

    if (Buffer->TabContext.TabCount == 1) {

        YORI_STRING CurrentArgString;

        YoriLibInitEmptyString(&CurrentArgString);

        YoriLibInitializeListHead(&Buffer->TabContext.MatchList);
        Buffer->TabContext.PreviousMatch = NULL;

        if (CmdContext.CurrentArg < CmdContext.argc) {
            CopyMemory(&CurrentArgString, &CmdContext.ysargv[CmdContext.CurrentArg], sizeof(YORI_STRING));
        }
        SearchLength = CurrentArgString.LengthInChars + 1;
        if (!YoriLibAllocateString(&Buffer->TabContext.SearchString, SearchLength + 1)) {
            YoriShFreeCmdContext(&CmdContext);
            return;
        }
    
        Buffer->TabContext.SearchString.LengthInChars = YoriLibSPrintfS(Buffer->TabContext.SearchString.StartOfString, SearchLength + 1, _T("%y*"), &CurrentArgString);

        if (CmdContext.CurrentArg == 0) {

            if (SearchHistory) {
                Buffer->TabContext.SearchType = YoriTabCompleteSearchHistory;
            } else if (!YoriShDoesExpressionSpecifyPath(&CmdContext.ysargv[0])) {
                Buffer->TabContext.SearchType = YoriTabCompleteSearchExecutables;
            } else {
                Buffer->TabContext.SearchType = YoriTabCompleteSearchFiles;
            }

        } else {
            Buffer->TabContext.SearchType = YoriTabCompleteSearchFiles;
        }

        if (Buffer->TabContext.SearchType == YoriTabCompleteSearchExecutables) {
            YoriShPerformExecutableTabCompletion(&Buffer->TabContext, ExpandFullPath);
        } else if (Buffer->TabContext.SearchType == YoriTabCompleteSearchHistory) {
            YoriShPerformHistoryTabCompletion(&Buffer->TabContext, ExpandFullPath);
        } else {
            YoriShPerformFileTabCompletion(&Buffer->TabContext, ExpandFullPath);
        }
    }

    //
    //  Check if we have any match.  If we do, try to use it.  If not, leave
    //  the buffer unchanged.
    //

    ListEntry = YoriLibGetNextListEntry(&Buffer->TabContext.MatchList, &Buffer->TabContext.PreviousMatch->ListEntry);
    if (ListEntry == NULL) {
        if (Buffer->TabContext.TabCount != 1) {
            ListEntry = YoriLibGetNextListEntry(&Buffer->TabContext.MatchList, NULL);
        }
    }
    if (ListEntry == NULL) {
        YoriShFreeCmdContext(&CmdContext);
        return;
    }

    Match = CONTAINING_RECORD(ListEntry, YORI_TAB_COMPLETE_MATCH, ListEntry);
    Buffer->TabContext.PreviousMatch = Match;

    {
        DWORD BeginCurrentArg;
        DWORD EndCurrentArg;
        LPTSTR NewString;
        BOOL FreeNewString = FALSE;
        DWORD NewStringLen;

        //
        //  MSFIX This isn't updating the referenced memory.  This works because
        //  we'll free the "correct" one and not the one we just put here, but
        //  it seems dodgy.
        //

        if (Buffer->TabContext.SearchType != YoriTabCompleteSearchHistory) {
            PYORI_STRING OldArgv = NULL;
            PYORI_ARG_CONTEXT OldArgContext = NULL;
            DWORD OldArgCount = 0;

            if (CmdContext.CurrentArg >= CmdContext.argc) {
                DWORD Count;

                OldArgCount = CmdContext.argc;
                OldArgv = CmdContext.ysargv;
                OldArgContext = CmdContext.ArgContexts;

                CmdContext.ysargv = YoriLibMalloc((CmdContext.CurrentArg + 1) * (sizeof(YORI_STRING) + sizeof(YORI_ARG_CONTEXT)));
                if (CmdContext.ysargv == NULL) {
                    YoriShFreeCmdContext(&CmdContext);
                    return;
                }

                CmdContext.argc = CmdContext.CurrentArg + 1;
                ZeroMemory(CmdContext.ysargv, CmdContext.argc * (sizeof(YORI_STRING) + sizeof(YORI_ARG_CONTEXT)));
                CmdContext.ArgContexts = (PYORI_ARG_CONTEXT)YoriLibAddToPointer(CmdContext.ysargv, CmdContext.argc * sizeof(YORI_STRING));

                CopyMemory(CmdContext.ysargv, OldArgv, OldArgCount * sizeof(YORI_STRING));
                for (Count = 0; Count < OldArgCount; Count++) {
                    CmdContext.ArgContexts[Count] = OldArgContext[Count];
                }

                YoriLibInitEmptyString(&CmdContext.ysargv[CmdContext.CurrentArg]);
            }

            YoriLibFreeStringContents(&CmdContext.ysargv[CmdContext.CurrentArg]);
            YoriLibCloneString(&CmdContext.ysargv[CmdContext.CurrentArg], &Match->YsValue);
            CmdContext.ArgContexts[CmdContext.CurrentArg].Quoted = FALSE;
            YoriShCheckIfArgNeedsQuotes(&CmdContext, CmdContext.CurrentArg);
            NewString = YoriShBuildCmdlineFromCmdContext(&CmdContext, FALSE, &BeginCurrentArg, &EndCurrentArg);

            if (OldArgv != NULL) {
                YoriLibFree(CmdContext.ysargv);
                CmdContext.argc = OldArgCount;
                CmdContext.ysargv = OldArgv;
                CmdContext.ArgContexts = OldArgContext;
            }

            if (NewString == NULL) {
                YoriShFreeCmdContext(&CmdContext);
                return;
            }

            FreeNewString = TRUE;
            Buffer->CurrentOffset = EndCurrentArg + 1;
            NewStringLen = _tcslen(NewString);

        } else {
            NewString = Match->YsValue.StartOfString;
            NewStringLen = Match->YsValue.LengthInChars;
            Buffer->CurrentOffset = NewStringLen;
        }

        if (NewString != NULL) {
            if (!YoriShEnsureStringHasEnoughCharacters(Buffer, NewStringLen)) {
                YoriShFreeCmdContext(&CmdContext);
                return;
            }
            YoriLibYPrintf(&Buffer->String, _T("%s"), NewString);
            if (Buffer->CurrentOffset > Buffer->String.LengthInChars) {
                Buffer->CurrentOffset = Buffer->String.LengthInChars;
            }

            if (FreeNewString) {
                YoriLibDereference(NewString);
            }

            //
            //  For successful tab completion, redraw everything.  It's rare
            //  and plenty of changes are possible.
            //

            Buffer->DirtyBeginOffset = 0;
            Buffer->DirtyLength = Buffer->String.LengthInChars;
        }
    }
    
    YoriShFreeCmdContext(&CmdContext);
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
    LPTSTR NewString;
    DWORD BeginCurrentArg;
    DWORD EndCurrentArg;
    DWORD NewStringLen;

    if (!YoriShParseCmdlineToCmdContext(&Buffer->String, Buffer->CurrentOffset, &CmdContext)) {
        return;
    }

    if (CmdContext.argc == 0) {
        YoriShFreeCmdContext(&CmdContext);
        return;
    }

    if (CmdContext.CurrentArg > 0) {
        CmdContext.CurrentArg--;
    }

    NewString = YoriShBuildCmdlineFromCmdContext(&CmdContext, FALSE, &BeginCurrentArg, &EndCurrentArg);

    if (NewString != NULL) {
        NewStringLen = _tcslen(NewString);
        if (!YoriShEnsureStringHasEnoughCharacters(Buffer, NewStringLen)) {
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

    if (CmdContext.argc == 0) {
        YoriShFreeCmdContext(&CmdContext);
        return;
    }

    if (CmdContext.CurrentArg + 1 < (DWORD)CmdContext.argc) {
        CmdContext.CurrentArg++;
    } else {
        MoveToEnd = TRUE;
    }

    NewString = YoriShBuildCmdlineFromCmdContext(&CmdContext, FALSE, &BeginCurrentArg, &EndCurrentArg);

    if (NewString != NULL) {
        DWORD NewStringLen;
        NewStringLen = _tcslen(NewString);
        if (!YoriShEnsureStringHasEnoughCharacters(Buffer, NewStringLen)) {
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

    if (CmdContext.argc == 0) {
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
    YoriShAddCStringToInput(Buffer, CmdLine, TRUE);
    YoriLibDereference(CmdLine);
    return TRUE;
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
    PYORI_LIST_ENTRY HistoryEntryToUse = NULL;
    BOOLEAN InsertMode = TRUE;
    CONSOLE_CURSOR_INFO CursorInfo;
    INPUT_RECORD InputRecords[20];
    PINPUT_RECORD InputRecord;
    BOOL KeyPressFound;
    DWORD NumericKeyValue = 0;
    BOOL NumericKeyAnsiMode = FALSE;

    CursorInfo.bVisible = TRUE;
    CursorInfo.dwSize = 20;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &CursorInfo);

    ZeroMemory(&Buffer, sizeof(Buffer));

    if (!YoriLibAllocateString(&Buffer.String, 256)) {
        return FALSE;
    }

    while (TRUE) {

        if (!PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), InputRecords, sizeof(InputRecords)/sizeof(InputRecords[0]), &ActuallyRead)) {
            break;
        }

        KeyPressFound = FALSE;

        for (CurrentRecordIndex = 0; CurrentRecordIndex < ActuallyRead; CurrentRecordIndex++) {

            InputRecord = &InputRecords[CurrentRecordIndex];

            if (InputRecord->EventType == KEY_EVENT &&
                InputRecord->Event.KeyEvent.bKeyDown) {
    
                DWORD CtrlMask;
                TCHAR Char;
                DWORD Count;
                WORD KeyCode;
                WORD ScanCode;
    
                KeyPressFound = TRUE;
                YoriShPrepareForNextKey(&Buffer);
        
                Char = InputRecord->Event.KeyEvent.uChar.UnicodeChar;
                CtrlMask = InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | ENHANCED_KEY | SHIFT_PRESSED);
                KeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;
                ScanCode = InputRecord->Event.KeyEvent.wVirtualScanCode;
    
                if (KeyCode >= VK_F1 && KeyCode <= VK_F12) {
                    if (YoriShHotkey(&Buffer, KeyCode, CtrlMask)) {
                        YoriShDisplayAfterKeyPress(&Buffer);
                        YoriShTerminateInput(&Buffer);
                        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), InputRecords, CurrentRecordIndex + 1, &ActuallyRead);
                        YoriShAddToHistory(&Buffer.String);
                        memcpy(Expression, &Buffer.String, sizeof(YORI_STRING));
                        return TRUE;
                    }
                }
    
                if (CtrlMask == 0 || CtrlMask == SHIFT_PRESSED) {
        
                    if (Char == '\r') {
                        YoriShDisplayAfterKeyPress(&Buffer);
                        YoriShTerminateInput(&Buffer);
                        YoriShAddToHistory(&Buffer.String);
                        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), InputRecords, CurrentRecordIndex + 1, &ActuallyRead);
                        memcpy(Expression, &Buffer.String, sizeof(YORI_STRING));
                        return TRUE;
                    } else if (Char == 27) {
                        YoriShClearInput(&Buffer);
                    } else if (Char == '\t') {
                        YoriShTabCompletion(&Buffer, FALSE, FALSE);
                    } else if (Char == '\b') {
                        YoriShBackspace(&Buffer, InputRecord->Event.KeyEvent.wRepeatCount);
                    } else if (Char == '\0') {
                    } else {
                        for (Count = 0; Count < InputRecord->Event.KeyEvent.wRepeatCount; Count++) {
                            TCHAR StringChar[2];
                            StringChar[0] = Char;
                            StringChar[1] = '\0';
                            YoriShAddCStringToInput(&Buffer, StringChar, InsertMode);
                        }
                    }
                } else if (CtrlMask == RIGHT_CTRL_PRESSED ||
                           CtrlMask == LEFT_CTRL_PRESSED ||
                           CtrlMask == (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) {
                    if (KeyCode == 'C') {
                        YoriShClearInput(&Buffer);
                        YoriShTerminateInput(&Buffer);
                        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), InputRecords, CurrentRecordIndex + 1, &ActuallyRead);
                        memcpy(Expression, &Buffer.String, sizeof(YORI_STRING));
                        return TRUE;
                    } else if (KeyCode == 'E') {
                        TCHAR StringChar[2];
                        StringChar[0] = 27;
                        StringChar[1] = '\0';
                        YoriShAddCStringToInput(&Buffer, StringChar, InsertMode);
                    } else if (KeyCode == 'V') {
                        YORI_STRING ClipboardData;
                        YoriLibInitEmptyString(&ClipboardData);
                        if (YoriShPasteText(&ClipboardData)) {
                            YoriShAddYoriStringToInput(&Buffer, &ClipboardData, InsertMode);
                            YoriLibFreeStringContents(&ClipboardData);
                        }
                    } else if (KeyCode == VK_TAB) {
                        YoriShTabCompletion(&Buffer, TRUE, FALSE);
                    }
                } else if (CtrlMask == ENHANCED_KEY) {
                    PYORI_LIST_ENTRY NewEntry = NULL;
                    PYORI_HISTORY_ENTRY HistoryEntry;
                    if (KeyCode == VK_UP) {
                        NewEntry = YoriLibGetPreviousListEntry(&YoriShCommandHistory, HistoryEntryToUse);
                        if (NewEntry != NULL) {
                            HistoryEntryToUse = NewEntry;
                            HistoryEntry = CONTAINING_RECORD(NewEntry, YORI_HISTORY_ENTRY, ListEntry);
                            YoriShClearInput(&Buffer);
                            YoriShAddYoriStringToInput(&Buffer, &HistoryEntry->CmdLine, InsertMode);
                        }
                    } else if (KeyCode == VK_DOWN) {
                        if (HistoryEntryToUse != NULL) {
                            NewEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, HistoryEntryToUse);
                        }
                        if (NewEntry != NULL) {
                            HistoryEntryToUse = NewEntry;
                            HistoryEntry = CONTAINING_RECORD(NewEntry, YORI_HISTORY_ENTRY, ListEntry);
                            YoriShClearInput(&Buffer);
                            YoriShAddYoriStringToInput(&Buffer, &HistoryEntry->CmdLine, InsertMode);
                        }
                    } else if (KeyCode == VK_LEFT) {
                        if (Buffer.CurrentOffset > 0) {
                            Buffer.CurrentOffset--;
                        }
                    } else if (KeyCode == VK_RIGHT) {
                        if (Buffer.CurrentOffset < Buffer.String.LengthInChars) {
                            Buffer.CurrentOffset++;
                        }
                    } else if (KeyCode == VK_INSERT) {
                        CursorInfo.bVisible = TRUE;
                        if (InsertMode) {
                            InsertMode = FALSE;
                            CursorInfo.dwSize = 100;
                        } else {
                            InsertMode = TRUE;
                            CursorInfo.dwSize = 20;
                        }
                        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &CursorInfo);
                    } else if (KeyCode == VK_HOME) {
                        Buffer.CurrentOffset = 0;
                    } else if (KeyCode == VK_END) {
                        Buffer.CurrentOffset = Buffer.String.LengthInChars;
                    } else if (KeyCode == VK_DELETE) {
    
                        Count = InputRecord->Event.KeyEvent.wRepeatCount;
                        if (Count + Buffer.CurrentOffset > Buffer.String.LengthInChars) {
                            Count = Buffer.String.LengthInChars - Buffer.CurrentOffset;
                        }
    
                        Buffer.CurrentOffset += Count;
                        
                        YoriShBackspace(&Buffer, Count);
                        
                    } else if (KeyCode == VK_RETURN) {
                        YoriShDisplayAfterKeyPress(&Buffer);
                        YoriShTerminateInput(&Buffer);
                        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), InputRecords, CurrentRecordIndex + 1, &ActuallyRead);
                        YoriShAddToHistory(&Buffer.String);
                        memcpy(Expression, &Buffer.String, sizeof(YORI_STRING));
                        return TRUE;
                    }
                } else if (CtrlMask == (RIGHT_CTRL_PRESSED | ENHANCED_KEY) ||
                           CtrlMask == (LEFT_CTRL_PRESSED | ENHANCED_KEY) ||
                           CtrlMask == (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | ENHANCED_KEY)) {
    
                    if (KeyCode == VK_LEFT) {
                        YoriShMoveCursorToPriorArgument(&Buffer);
                    } else if (KeyCode == VK_RIGHT) {
                        YoriShMoveCursorToNextArgument(&Buffer);
                    } else if (KeyCode == VK_UP) {
                        YoriShTabCompletion(&Buffer, FALSE, TRUE);
                    }
                } else if (CtrlMask == LEFT_ALT_PRESSED || CtrlMask == RIGHT_ALT_PRESSED ||
                           CtrlMask == (LEFT_ALT_PRESSED | ENHANCED_KEY) ||
                           CtrlMask == (RIGHT_ALT_PRESSED | ENHANCED_KEY)) {
                    if (KeyCode >= '0' && KeyCode <= '9') {
                        if (KeyCode == '0' && NumericKeyValue == 0 && !NumericKeyAnsiMode) {
                            NumericKeyAnsiMode = TRUE;
                        } else {
                            NumericKeyValue = NumericKeyValue * 10 + KeyCode - '0';
                        }
                    } else if (KeyCode >= VK_NUMPAD0 && KeyCode <= VK_NUMPAD9) {
                        if (KeyCode == VK_NUMPAD0 && NumericKeyValue == 0 && !NumericKeyAnsiMode) {
                            NumericKeyAnsiMode = TRUE;
                        } else {
                            NumericKeyValue = NumericKeyValue * 10 + KeyCode - VK_NUMPAD0;
                        }
                    } else if (ScanCode >= 0x47 && ScanCode <= 0x49) {
                        NumericKeyValue = NumericKeyValue * 10 + ScanCode - 0x47 + 7;
                    } else if (ScanCode >= 0x4b && ScanCode <= 0x4d) {
                        NumericKeyValue = NumericKeyValue * 10 + ScanCode - 0x4b + 4;
                    } else if (ScanCode >= 0x4f && ScanCode <= 0x51) {
                        NumericKeyValue = NumericKeyValue * 10 + ScanCode - 0x4f + 1;
                    } else if (ScanCode == 0x52) {
                        if (NumericKeyValue == 0 && !NumericKeyAnsiMode) {
                            NumericKeyAnsiMode = TRUE;
                        } else {
                            NumericKeyValue = NumericKeyValue * 10;
                        }
                    }
                } else if (CtrlMask == (SHIFT_PRESSED | ENHANCED_KEY)) {
                    if (KeyCode == VK_INSERT) {
                        YORI_STRING ClipboardData;
                        YoriLibInitEmptyString(&ClipboardData);
                        if (YoriShPasteText(&ClipboardData)) {
                            YoriShAddYoriStringToInput(&Buffer, &ClipboardData, InsertMode);
                            YoriLibFreeStringContents(&ClipboardData);
                        }
                    }
                }

                YoriShPostKeyPress(&Buffer);
            } else if (InputRecord->EventType == KEY_EVENT) {
                ASSERT(!InputRecord->Event.KeyEvent.bKeyDown);

                if ((InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) == 0 &&
                    NumericKeyValue != 0) {

                    CHAR SmallKeyValue = (CHAR)NumericKeyValue;
                    TCHAR HostKeyValue[2];

                    HostKeyValue[0] = HostKeyValue[1] = 0;

                    MultiByteToWideChar(NumericKeyAnsiMode?CP_ACP:CP_OEMCP,
                                        0,
                                        &SmallKeyValue,
                                        1,
                                        HostKeyValue,
                                        1);

                    if (HostKeyValue != 0) {
                        YoriShPrepareForNextKey(&Buffer);
                        YoriShAddCStringToInput(&Buffer, HostKeyValue, InsertMode);
                        YoriShPostKeyPress(&Buffer);
                        KeyPressFound = TRUE;
                    }

                    NumericKeyValue = 0;
                    NumericKeyAnsiMode = FALSE;
                }
            }
        }
        if (KeyPressFound) {
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
        //  Wait to see if any further events arrive.
        //

        if (WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), INFINITE) != WAIT_OBJECT_0) {
            break;
        }
    }

    err = GetLastError();

    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Error reading from console %i handle %08x\n"), err, GetStdHandle(STD_INPUT_HANDLE));

    YoriLibFreeStringContents(&Buffer.String);
    return FALSE;
}


// vim:sw=4:ts=4:et:
