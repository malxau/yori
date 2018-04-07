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
