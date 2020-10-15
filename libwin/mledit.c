/**
 * @file libwin/mledit.c
 *
 * Yori window multiline edit control
 *
 * Copyright (c) 2020 Malcolm J. Smith
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
#include "yoriwin.h"
#include "winpriv.h"


/**
 When reallocating a line, add this many extra characters on the assumption
 that the user is actively working on the line and another modification
 that needs space is likely.  This value is arbitrary.
 */
#define YORI_WIN_MULTILINE_EDIT_LINE_PADDING (0x40)

/**
 Information about the selection region within a multiline edit control.
 */
typedef struct _YORI_WIN_MULTILINE_EDIT_SELECT {

    /**
     Indicates if a selection is currently active, and if so, what caused the
     activation.
     */
    enum {
        YoriWinMultilineEditSelectNotActive = 0,
        YoriWinMultilineEditSelectKeyboardFromTopDown = 1,
        YoriWinMultilineEditSelectKeyboardFromBottomUp = 2,
        YoriWinMultilineEditSelectMouseFromTopDown = 3,
        YoriWinMultilineEditSelectMouseFromBottomUp = 4,
        YoriWinMultilineEditSelectMouseComplete = 5
    } Active;

    /**
     Specifies the line index containing the beginning of the selection.
     */
    DWORD FirstLine;

    /**
     Specifies the character offset containing the beginning of the selection.
     */
    DWORD FirstCharOffset;

    /**
     Specifies the line index containing the end of the selection.
     */
    DWORD LastLine;

    /**
     Specifies the index after the final character selected on the final line.
     This value can therefore be zero through to the length of string
     inclusive.
     */
    DWORD LastCharOffset;

} YORI_WIN_MULTILINE_EDIT_SELECT, *PYORI_WIN_MULTILINE_EDIT_SELECT;

/**
 A structure describing the contents of a multiline edit control.
 */
typedef struct _YORI_WIN_CTRL_MULTILINE_EDIT {

    /**
     A common header for all controls
     */
    YORI_WIN_CTRL Ctrl;

    /**
     Pointer to the vertical scroll bar associated with the multiline edit.
     */
    PYORI_WIN_CTRL VScrollCtrl;

    /**
     Optional pointer to a callback to invoke when the cursor moves.
     */
    PYORI_WIN_NOTIFY_CURSOR_MOVE CursorMoveCallback;

    /**
     The caption to display above the edit control.
     */
    YORI_STRING Caption;

    /**
     An array of lines corresponding to lines within a file.
     */
    PYORI_STRING LineArray;

    /**
     The number of lines allocated within LineArray.
     */
    DWORD LinesAllocated;

    /**
     The number of lines populated with text within LineArray.
     */
    DWORD LinesPopulated;

    /**
     The index within LineArray that is displayed at the top of the control.
     */
    DWORD ViewportTop;

    /**
     The horizontal offset within each line to display.
     */
    DWORD ViewportLeft;

    /**
     The index within LineArray that the cursor is located at.
     */
    DWORD CursorLine;

    /**
     The horizontal offset of the cursor in terms of the offset within the
     line buffer.
     */
    DWORD CursorOffset;

    /**
     The horizontal offset of the cursor in terms of the cell where it should
     be displayed.  This is typically the same as CursorOffset but can differ
     due to things like tab expansion.
     */
    DWORD DisplayCursorOffset;

    /**
     The current number of spaces to display for each tab.
     */
    DWORD TabWidth;

    /**
     The first line, in cursor coordinates, that requires redrawing.  Lines
     between this and the last line below (inclusive) will be redrawn on
     paint.  If this value is greater than the last line, no redrawing
     occurs.  This is a fairly common scenario when the cursor is moved,
     where a repaint is needed but no data changes are occurring.
     */
    DWORD FirstDirtyLine;

    /**
     The last line, in cursor coordinates, that requires redrawing.  Lines
     between the first line above and this line (inclusive) will be redrawn
     on paint.
     */
    DWORD LastDirtyLine;

    /**
     Specifies the selection state of text within the multiline edit control.
     This is encapsulated into a structure purely for readability.
     */
    YORI_WIN_MULTILINE_EDIT_SELECT Selection;

    /**
     The attributes to display text in.
     */
    WORD TextAttributes;

    /**
     0 if the cursor is currently not visible.  20 for insert mode, 50 for
     overwrite mode.  Paint calculates the desired value and based on
     comparing the new value with the current value decides on the action
     to take.
     */
    UCHAR PercentCursorVisibleLastPaint;

    /**
     If TRUE, new characters are inserted at the cursor position.  If FALSE,
     new characters overwrite existing characters.
     */
    BOOLEAN InsertMode;

    /**
     If TRUE, the edit control should not support editing.  If FALSE, it is
     a regular, editable edit control.
     */
    BOOLEAN ReadOnly;

    /**
     TRUE if the control currently has focus, FALSE if another control has
     focus.
     */
    BOOLEAN HasFocus;

    /**
     TRUE if the contents of the control have been modified by user input.
     FALSE if the contents have not changed since this value was last reset.
     */
    BOOLEAN UserModified;

    /**
     TRUE if events indicate that the left mouse button is currently held
     down.  FALSE if the mouse button is released.
     */
    BOOLEAN MouseButtonDown;

} YORI_WIN_CTRL_MULTILINE_EDIT, *PYORI_WIN_CTRL_MULTILINE_EDIT;

/**
 Calculate the line of text to display.  This is typically the exact same
 string as the line from the file's contents, but can diverge due to 
 display requirements such as tab expansion.

 @param MultilineEdit Pointer to the multiline edit control.

 @param LineIndex Specifies the line number to obtain a display line for.

 @param DisplayLine On successful completion, populated with a string to
        display.  This may point back into the same data as the original
        line, or may be a fresh allocation.  The caller should free it with
        @ref YoriLibFreeStringContents .  If the result points back to the
        original string, the MemoryToFree member will be NULL to indicate
        that the caller has nothing to deallocate.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditGenerateDisplayLine(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD LineIndex,
    __out PYORI_STRING DisplayLine
    )
{
    DWORD CharIndex;
    DWORD DestIndex;
    PYORI_STRING SourceLine;
    DWORD TabCount;
    BOOLEAN NeedDoubleBuffer;

    ASSERT(LineIndex < MultilineEdit->LinesPopulated);

    SourceLine = &MultilineEdit->LineArray[LineIndex];

    NeedDoubleBuffer = FALSE;
    TabCount = 0;
    for (CharIndex = 0; CharIndex < SourceLine->LengthInChars; CharIndex++) {
        if (SourceLine->StartOfString[CharIndex] == '\t') {
            NeedDoubleBuffer = TRUE;
            TabCount++;
        }
    }

    if (!NeedDoubleBuffer) {
        YoriLibInitEmptyString(DisplayLine);
        DisplayLine->StartOfString = SourceLine->StartOfString;
        DisplayLine->LengthInChars = SourceLine->LengthInChars;
        return TRUE;
    }

    if (!YoriLibAllocateString(DisplayLine, SourceLine->LengthInChars + TabCount * MultilineEdit->TabWidth)) {
        return FALSE;
    }

    DestIndex = 0;
    for (CharIndex = 0; CharIndex < SourceLine->LengthInChars; CharIndex++) {
        if (SourceLine->StartOfString[CharIndex] == '\t') {
            DWORD TabIndex;

            for (TabIndex = 0; TabIndex < MultilineEdit->TabWidth; TabIndex++) {
                DisplayLine->StartOfString[DestIndex + TabIndex] = ' ';
            }
            DestIndex = DestIndex + MultilineEdit->TabWidth;

        } else {
            DisplayLine->StartOfString[DestIndex] = SourceLine->StartOfString[CharIndex];
            DestIndex++;
        }
    }

    DisplayLine->LengthInChars = DestIndex;
    return TRUE;
}

/**
 Given a cursor offset expressed in terms of the display location of the
 cursor, find the offset within the string buffer.  These are typically the
 same but tab expansion means they are not guaranteed to be identical.

 @param MultilineEdit Pointer to the multiline edit control.

 @param LineIndex Specifies the line to evaluate against.

 @param DisplayChar Specifies the location in terms of the number of cells
        from the left of the line.

 @param CursorChar On completion, populated with the offset in the line buffer
        corresponding to the display offset.
 */
VOID
YoriWinMultilineEditFindCursorCharFromDisplayChar(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD LineIndex,
    __in DWORD DisplayChar,
    __out PDWORD CursorChar
    )
{
    DWORD CharIndex;
    DWORD CurrentDisplayIndex;
    PYORI_STRING Line;

    if (LineIndex >= MultilineEdit->LinesPopulated) {
        *CursorChar = DisplayChar;
        return;
    }

    Line = &MultilineEdit->LineArray[LineIndex];

    CurrentDisplayIndex = 0;
    for (CharIndex = 0; CharIndex < Line->LengthInChars; CharIndex++) {
        if (CurrentDisplayIndex >= DisplayChar) {
            *CursorChar = CharIndex;
            return;
        }

        if (Line->StartOfString[CharIndex] == '\t') {
            CurrentDisplayIndex += MultilineEdit->TabWidth;
        } else {
            CurrentDisplayIndex++;
        }
    }

    *CursorChar = DisplayChar - (CurrentDisplayIndex - CharIndex);
}

/**
 Given a cursor offset expressed in terms of the buffer offset of the cursor,
 find the offset within the display.  These are typically the same but tab
 expansion means they are not guaranteed to be identical.

 @param MultilineEdit Pointer to the multiline edit control.

 @param LineIndex Specifies the line to evaluate against.

 @param CursorChar Specifies the location in terms of the line buffer offset.

 @param DisplayChar On completion, populated with the offset from the left of
        the display.
 */
VOID
YoriWinMultilineEditFindDisplayCharFromCursorChar(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD LineIndex,
    __in DWORD CursorChar,
    __out PDWORD DisplayChar
    )
{
    DWORD CharIndex;
    DWORD CurrentDisplayIndex;
    PYORI_STRING Line;

    if (LineIndex >= MultilineEdit->LinesPopulated) {
        *DisplayChar = CursorChar;
        return;
    }

    Line = &MultilineEdit->LineArray[LineIndex];

    CurrentDisplayIndex = 0;
    for (CharIndex = 0; CharIndex < Line->LengthInChars; CharIndex++) {
        if (CharIndex >= CursorChar) {
            *DisplayChar = CurrentDisplayIndex;
            return;
        }

        if (Line->StartOfString[CharIndex] == '\t') {
            CurrentDisplayIndex += MultilineEdit->TabWidth;
        } else {
            CurrentDisplayIndex++;
        }
    }

    *DisplayChar = CursorChar + (CurrentDisplayIndex - CharIndex);
}

/**
 Translate coordinates relative to the control's client area into
 cursor coordinates, being offsets to the line and character within the
 buffers being edited.

 @param MultilineEdit Pointer to the multiline edit control.

 @param ViewportLeftOffset Offset from the left of the client area.

 @param ViewportTopOffset Offset from the top of the client area.

 @param LineIndex Populated with the cursor index to the line.

 @param CursorChar Populated with the offset within the line of the cursor.

 @return TRUE to indicate the region is within the current buffer.  FALSE
         to indicate it's beyond the current buffer.
 */
BOOLEAN
YoriWinMultilineEditTranslateViewportCoordinatesToCursorCoordinates(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD ViewportLeftOffset,
    __in DWORD ViewportTopOffset,
    __out PDWORD LineIndex,
    __out PDWORD CursorChar
    )
{
    DWORD LineOffset;
    DWORD DisplayOffset;
    BOOLEAN Result = TRUE;

    LineOffset = ViewportTopOffset + MultilineEdit->ViewportTop;
    if (LineOffset >= MultilineEdit->LinesPopulated) {
        if (MultilineEdit->LinesPopulated == 0) {
            LineOffset = 0;
        } else {
            LineOffset = MultilineEdit->LinesPopulated - 1;
        }
        Result = FALSE;
    }

    DisplayOffset = ViewportLeftOffset + MultilineEdit->ViewportLeft;

    YoriWinMultilineEditFindCursorCharFromDisplayChar(MultilineEdit, LineOffset, DisplayOffset, CursorChar);
    *LineIndex = LineOffset;
    return Result;
}



/**
 Return TRUE if a selection region is active, or FALSE if no selection is
 currently active.

 @param CtrlHandle Pointer to the multiline edit control.

 @return TRUE if a selection region is active, or FALSE if no selection is
         currently active.
 */
BOOLEAN
YoriWinMultilineEditSelectionActive(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (MultilineEdit->Selection.Active == YoriWinMultilineEditSelectNotActive) {
        return FALSE;
    }
    return TRUE;
}

/**
 Draw the scroll bar with current information about the location and contents
 of the viewport.

 @param MultilineEdit Pointer to the multiline edit to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditRepaintScrollBar(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    if (MultilineEdit->VScrollCtrl) {
        DWORD MaximumTopValue;
        COORD ClientSize;

        YoriWinGetControlClientSize(&MultilineEdit->Ctrl, &ClientSize);

        if (MultilineEdit->LinesPopulated > (DWORD)ClientSize.Y) {
            MaximumTopValue = MultilineEdit->LinesPopulated - ClientSize.Y;
        } else {
            MaximumTopValue = 0;
        }

        YoriWinScrollBarSetPosition(MultilineEdit->VScrollCtrl, MultilineEdit->ViewportTop, ClientSize.Y, MaximumTopValue);
    }

    return TRUE;
}

/**
 Draw the border, caption and scroll bars on the control.

 @param MultilineEdit Pointer to the multiline edit to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditPaintNonClient(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    SMALL_RECT BorderLocation;
    WORD BorderFlags;
    WORD WindowAttributes;
    WORD TextAttributes;
    WORD ColumnIndex;

    BorderLocation.Left = 0;
    BorderLocation.Top = 0;
    BorderLocation.Right = (SHORT)(MultilineEdit->Ctrl.FullRect.Right - MultilineEdit->Ctrl.FullRect.Left);
    BorderLocation.Bottom = (SHORT)(MultilineEdit->Ctrl.FullRect.Bottom - MultilineEdit->Ctrl.FullRect.Top);

    BorderFlags = YORI_WIN_BORDER_TYPE_SUNKEN | YORI_WIN_BORDER_TYPE_SINGLE;

    WindowAttributes = MultilineEdit->TextAttributes;
    YoriWinDrawBorderOnControl(&MultilineEdit->Ctrl, &BorderLocation, WindowAttributes, BorderFlags);

    if (MultilineEdit->Caption.LengthInChars > 0) {
        DWORD CaptionCharsToDisplay;
        DWORD StartOffset;
        COORD ClientSize;

        YoriWinGetControlClientSize(&MultilineEdit->Ctrl, &ClientSize);

        CaptionCharsToDisplay = MultilineEdit->Caption.LengthInChars;
        if (CaptionCharsToDisplay > (WORD)ClientSize.X) {
            CaptionCharsToDisplay = ClientSize.X;
        }

        StartOffset = (ClientSize.X - CaptionCharsToDisplay) / 2;
        TextAttributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
        for (ColumnIndex = 0; ColumnIndex < CaptionCharsToDisplay; ColumnIndex++) {
            YoriWinSetControlNonClientCell(&MultilineEdit->Ctrl, (WORD)(ColumnIndex + StartOffset), 0, MultilineEdit->Caption.StartOfString[ColumnIndex], TextAttributes);
        }
    }

    //
    //  Repaint the scroll bar after the border is drawn
    //

    YoriWinMultilineEditRepaintScrollBar(MultilineEdit);
    return TRUE;
}

/**
 Draw a single line of text within the client area of a multiline edit
 control.

 @param MultilineEdit Pointer to the multiline edit control.

 @param ClientSize Pointer to the dimensions of the client area of the
        control.

 @param LineIndex Specifies the index of the line to draw, in cursor
        coordinates.
 */
VOID
YoriWinMultilineEditPaintSingleLine(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in PCOORD ClientSize,
    __in DWORD LineIndex
    )
{
    WORD ColumnIndex;
    WORD WindowAttributes;
    WORD TextAttributes;
    WORD RowIndex;
    BOOLEAN SelectionActive;
    YORI_STRING Line;

    ColumnIndex = 0;
    RowIndex = (WORD)(LineIndex - MultilineEdit->ViewportTop);
    WindowAttributes = MultilineEdit->TextAttributes;
    SelectionActive = YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl);

    if (LineIndex < MultilineEdit->LinesPopulated) {
        TextAttributes = WindowAttributes;

        //
        //  If the entire line is selected, indicate that.
        //

        if (SelectionActive &&
            LineIndex > MultilineEdit->Selection.FirstLine &&
            LineIndex < MultilineEdit->Selection.LastLine) {
            TextAttributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
        }

        if (!YoriWinMultilineEditGenerateDisplayLine(MultilineEdit, LineIndex, &Line)) {
            YoriLibInitEmptyString(&Line);
        }
        for (; ColumnIndex < ClientSize->X && ColumnIndex + MultilineEdit->ViewportLeft < Line.LengthInChars; ColumnIndex++) {
            if (SelectionActive) {
                DWORD DisplayFirstCharOffset;
                DWORD DisplayLastCharOffset;
                YoriWinMultilineEditFindDisplayCharFromCursorChar(MultilineEdit, MultilineEdit->Selection.FirstLine, MultilineEdit->Selection.FirstCharOffset, &DisplayFirstCharOffset);
                YoriWinMultilineEditFindDisplayCharFromCursorChar(MultilineEdit, MultilineEdit->Selection.LastLine, MultilineEdit->Selection.LastCharOffset, &DisplayLastCharOffset);


                if (LineIndex == MultilineEdit->Selection.FirstLine &&
                    LineIndex == MultilineEdit->Selection.LastLine) {
                    TextAttributes = WindowAttributes;
                    if (ColumnIndex + MultilineEdit->ViewportLeft >= DisplayFirstCharOffset &&
                        ColumnIndex + MultilineEdit->ViewportLeft < DisplayLastCharOffset) {
                        TextAttributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
                    }
                } else if (LineIndex == MultilineEdit->Selection.FirstLine) {
                    TextAttributes = WindowAttributes;
                    if (ColumnIndex + MultilineEdit->ViewportLeft >= DisplayFirstCharOffset) {
                        TextAttributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
                    }
                } else if (LineIndex == MultilineEdit->Selection.LastLine) {
                    TextAttributes = WindowAttributes;
                    if (ColumnIndex + MultilineEdit->ViewportLeft < DisplayLastCharOffset) {
                        TextAttributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
                    }
                }
            }

            YoriWinSetControlClientCell(&MultilineEdit->Ctrl, ColumnIndex, RowIndex, Line.StartOfString[ColumnIndex + MultilineEdit->ViewportLeft], TextAttributes);
        }

        //
        //  Unless a tab is present, this is a no-op
        //

        YoriLibFreeStringContents(&Line);
    }
    for (; ColumnIndex < ClientSize->X; ColumnIndex++) {
        YoriWinSetControlClientCell(&MultilineEdit->Ctrl, ColumnIndex, RowIndex, ' ', WindowAttributes);
    }
}

/**
 Draw the edit with its current state applied.

 @param MultilineEdit Pointer to the multiline edit to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditPaint(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    WORD RowIndex;
    DWORD LineIndex;
    COORD ClientSize;

    YoriWinGetControlClientSize(&MultilineEdit->Ctrl, &ClientSize);

    if (MultilineEdit->FirstDirtyLine <= MultilineEdit->LastDirtyLine) {

        for (RowIndex = 0; RowIndex < ClientSize.Y; RowIndex++) {
            LineIndex = MultilineEdit->ViewportTop + RowIndex;

            //
            //  If the line in the viewport actually has a line in the buffer.
            //  Lines after the end of the buffer still need to be rendered in
            //  the viewport, even if it's trivial.
            //

            if (LineIndex >= MultilineEdit->FirstDirtyLine &&
                LineIndex <= MultilineEdit->LastDirtyLine) {

                YoriWinMultilineEditPaintSingleLine(MultilineEdit, &ClientSize, LineIndex);
            }
        }

        MultilineEdit->FirstDirtyLine = (DWORD)-1;
        MultilineEdit->LastDirtyLine = 0;
    }

    YoriWinMultilineEditFindDisplayCharFromCursorChar(MultilineEdit, MultilineEdit->CursorLine, MultilineEdit->CursorOffset, &MultilineEdit->DisplayCursorOffset);

    {
        WORD CursorLineWithinDisplay = 0;
        WORD CursorColumnWithinDisplay = 0;
        UCHAR NewPercentCursorVisible = 0;

        //
        //  If the control has focus, check based on insert state which
        //  type of cursor to display.
        //

        if (MultilineEdit->HasFocus) {
            if (MultilineEdit->InsertMode) {
                NewPercentCursorVisible = 20;
            } else {
                NewPercentCursorVisible = 50;
            }
        }

        //
        //  If the cursor is off the display, make it invisible.  If not,
        //  find the offset relative to the display.
        //

        if (MultilineEdit->CursorLine < MultilineEdit->ViewportTop ||
            MultilineEdit->CursorLine >= MultilineEdit->ViewportTop + ClientSize.Y) {

            NewPercentCursorVisible = 0;
        } else {
            CursorLineWithinDisplay = (WORD)(MultilineEdit->CursorLine - MultilineEdit->ViewportTop);
        }

        if (MultilineEdit->DisplayCursorOffset < MultilineEdit->ViewportLeft ||
            MultilineEdit->DisplayCursorOffset >= MultilineEdit->ViewportLeft + ClientSize.X) {

            NewPercentCursorVisible = 0;
        } else {
            CursorColumnWithinDisplay = (WORD)(MultilineEdit->DisplayCursorOffset - MultilineEdit->ViewportLeft);
        }

        //
        //  If the cursor is now invisible and previously wasn't, hide the
        //  cursor.  If it should be visible and previously was some other
        //  state, make it visible in the correct percentage.  If it should
        //  be visible now, position it regardless of state.  Note that the
        //  Windows API expects a nonzero percentage even when hiding the
        //  cursor, so we give it a fairly meaningless value.
        //

        if (NewPercentCursorVisible == 0)  {
            if (MultilineEdit->PercentCursorVisibleLastPaint != 0) {
                YoriWinSetControlCursorState(&MultilineEdit->Ctrl, FALSE, 25);
            }
        } else {
            if (MultilineEdit->PercentCursorVisibleLastPaint != NewPercentCursorVisible) {
                YoriWinSetControlCursorState(&MultilineEdit->Ctrl, TRUE, NewPercentCursorVisible);
            }

            YoriWinSetControlClientCursorLocation(&MultilineEdit->Ctrl, CursorColumnWithinDisplay, CursorLineWithinDisplay);
        }

        MultilineEdit->PercentCursorVisibleLastPaint = NewPercentCursorVisible;
    }

    return TRUE;
}

/**
 Set the range of the multiline edit control that requires redrawing.  This
 range can only be shrunk by actual drawing, so use any new lines to extend
 but not contract the range.

 @param MultilineEdit Pointer to the multiline edit control.

 @param NewFirstDirtyLine Specifies the first line that needs to be redrawn.

 @param NewLastDirtyLine Specifies the last line that needs to be redrawn.
 */
VOID
YoriWinMultilineEditExpandDirtyRange(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD NewFirstDirtyLine,
    __in DWORD NewLastDirtyLine
    )
{
    if (NewFirstDirtyLine < MultilineEdit->FirstDirtyLine) {
        MultilineEdit->FirstDirtyLine = NewFirstDirtyLine;
    }

    if (NewLastDirtyLine > MultilineEdit->LastDirtyLine) {
        MultilineEdit->LastDirtyLine = NewLastDirtyLine;
    }
}

/**
 Clear any selection if it is active and indicate that the region it covered
 needs to be redrawn.

 @param MultilineEdit Pointer to the multiline edit control.
 */
VOID
YoriWinMultilineEditClearSelection(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    if (MultilineEdit->Selection.Active == YoriWinMultilineEditSelectNotActive) {
        return;
    }
    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, MultilineEdit->Selection.FirstLine, MultilineEdit->Selection.LastLine);
    MultilineEdit->Selection.Active = YoriWinMultilineEditSelectNotActive;
}


/**
 Modify the cursor location within the multiline edit control.

 @param MultilineEdit Pointer to the multiline edit control.

 @param NewCursorOffset The offset of the cursor from the beginning of the
        line, in buffer coordinates.

 @param NewCursorLine The buffer line that the cursor is located on.
 */
VOID
YoriWinMultilineEditSetCursorLocationInternal(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD NewCursorOffset,
    __in DWORD NewCursorLine
    )
{
    if (NewCursorOffset == MultilineEdit->CursorOffset &&
        NewCursorLine == MultilineEdit->CursorLine) {

        return;
    }

    ASSERT(NewCursorLine == 0 || NewCursorLine < MultilineEdit->LinesPopulated);

    if (MultilineEdit->CursorMoveCallback != NULL) {
        MultilineEdit->CursorMoveCallback(&MultilineEdit->Ctrl, NewCursorOffset, NewCursorLine);
    }

    MultilineEdit->CursorOffset = NewCursorOffset;
    MultilineEdit->CursorLine = NewCursorLine;
}

/**
 Adjust the first character to display in the control to ensure the current
 user cursor is visible somewhere within the control.

 @param MultilineEdit Pointer to the multiline edit control.
 */
VOID
YoriWinMultilineEditEnsureCursorVisible(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    COORD ClientSize;
    DWORD NewViewportLeft;
    DWORD NewViewportTop;

    NewViewportLeft = MultilineEdit->ViewportLeft;
    NewViewportTop = MultilineEdit->ViewportTop;

    YoriWinGetControlClientSize(&MultilineEdit->Ctrl, &ClientSize);

    //
    //  We can't guarantee that the entire selection is on the screen,
    //  but if it's a single line selection that would fit, try to
    //  take that into account.  Do this first so if the cursor would
    //  move the viewport, that takes precedence.
    //

    if (YoriWinMultilineEditSelectionActive(MultilineEdit)) {
        DWORD StartSelection;
        DWORD EndSelection;

        YoriWinMultilineEditFindDisplayCharFromCursorChar(MultilineEdit, MultilineEdit->Selection.FirstLine, MultilineEdit->Selection.FirstCharOffset, &StartSelection);
        YoriWinMultilineEditFindDisplayCharFromCursorChar(MultilineEdit, MultilineEdit->Selection.LastLine, MultilineEdit->Selection.LastCharOffset, &EndSelection);

        if (StartSelection < NewViewportLeft) {
            NewViewportLeft = StartSelection;
        } else if (EndSelection >= NewViewportLeft + ClientSize.X) {
            NewViewportLeft = EndSelection - ClientSize.X + 1;
        }
    }

    YoriWinMultilineEditFindDisplayCharFromCursorChar(MultilineEdit, MultilineEdit->CursorLine, MultilineEdit->CursorOffset, &MultilineEdit->DisplayCursorOffset);

    if (MultilineEdit->DisplayCursorOffset < NewViewportLeft) {
        NewViewportLeft = MultilineEdit->DisplayCursorOffset;
    } else if (MultilineEdit->DisplayCursorOffset >= NewViewportLeft + ClientSize.X) {
        NewViewportLeft = MultilineEdit->DisplayCursorOffset - ClientSize.X + 1;
    }

    if (MultilineEdit->CursorLine < NewViewportTop) {
        NewViewportTop = MultilineEdit->CursorLine;
    } else if (MultilineEdit->CursorLine >= NewViewportTop + ClientSize.Y) {
        NewViewportTop = MultilineEdit->CursorLine - ClientSize.Y + 1;
    }

    if (NewViewportTop != MultilineEdit->ViewportTop) {
        MultilineEdit->ViewportTop = NewViewportTop;
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, NewViewportTop, (DWORD)-1);
        YoriWinMultilineEditRepaintScrollBar(MultilineEdit);
    }

    if (NewViewportLeft != MultilineEdit->ViewportLeft) {
        MultilineEdit->ViewportLeft = NewViewportLeft;
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, NewViewportTop, (DWORD)-1);
    }
}


/**
 Set the color attributes of the multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @param Attributes Specifies the foreground and background color for the
        multiline edit control to use.
 */
VOID
YoriWinMultilineEditSetColor(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in WORD Attributes
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    MultilineEdit->TextAttributes = Attributes;
    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, 0, (DWORD)-1);
    YoriWinMultilineEditPaintNonClient(MultilineEdit);
    YoriWinMultilineEditPaint(MultilineEdit);
}

/**
 Toggle the insert state of the control.  If new keystrokes would previously
 insert new characters, future characters overwrite existing characters, and
 vice versa.  The cursor shape will be updated to reflect the new state.

 @param MultilineEdit Pointer to the multiline edit control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditToggleInsert(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    if (MultilineEdit->InsertMode) {
        MultilineEdit->InsertMode = FALSE;
    } else {
        MultilineEdit->InsertMode = TRUE;
    }
    return TRUE;
}

/**
 Merge two lines into one.  This occurs when the user deletes a line break.

 @param MultilineEdit Pointer to the multiline edit control.

 @param FirstLineIndex Specifies the first of two lines to merge.  The line
        immediately following this line is the other line to merge.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditMergeLines(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD FirstLineIndex
    )
{
    PYORI_STRING Line[2];
    DWORD LinesToCopy;

    if (FirstLineIndex + 1 > MultilineEdit->LinesPopulated) {
        return FALSE;
    }

    Line[0] = &MultilineEdit->LineArray[FirstLineIndex];
    Line[1] = &MultilineEdit->LineArray[FirstLineIndex + 1];

    if (Line[0]->LengthInChars + Line[1]->LengthInChars > Line[0]->LengthAllocated) {
        YORI_STRING TargetLine;

        if (!YoriLibAllocateString(&TargetLine, Line[0]->LengthInChars + Line[1]->LengthInChars + YORI_WIN_MULTILINE_EDIT_LINE_PADDING)) {
            return FALSE;
        }

        memcpy(TargetLine.StartOfString, Line[0]->StartOfString, Line[0]->LengthInChars * sizeof(TCHAR));
        TargetLine.LengthInChars = Line[0]->LengthInChars;
        YoriLibFreeStringContents(Line[0]);
        memcpy(Line[0], &TargetLine, sizeof(YORI_STRING));
    }

    memcpy(&Line[0]->StartOfString[Line[0]->LengthInChars], Line[1]->StartOfString, Line[1]->LengthInChars * sizeof(TCHAR));
    Line[0]->LengthInChars += Line[1]->LengthInChars;

    YoriLibFreeStringContents(Line[1]);

    LinesToCopy = MultilineEdit->LinesPopulated - FirstLineIndex - 2;
    if (LinesToCopy > 0) {
        memmove(&MultilineEdit->LineArray[FirstLineIndex + 1],
                &MultilineEdit->LineArray[FirstLineIndex + 2], 
                LinesToCopy * sizeof(YORI_STRING));
    }
    MultilineEdit->LinesPopulated--;
    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, FirstLineIndex, MultilineEdit->LinesPopulated);

    return TRUE;
}

/**
 Perform debug only checks to see that the selection state follows whatever
 rules are currently defined for it.

 @param MultilineEdit Pointer to the multiline edit control specifying the
        selection state.
 */
VOID
YoriWinMultilineEditCheckSelectionState(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    PYORI_WIN_MULTILINE_EDIT_SELECT Selection;
    Selection = &MultilineEdit->Selection;

    if (Selection->Active  == YoriWinMultilineEditSelectNotActive) {
        return;
    }
    ASSERT(Selection->LastLine < MultilineEdit->LinesPopulated);
    ASSERT(Selection->FirstLine <= Selection->LastLine);
    if (Selection->Active == YoriWinMultilineEditSelectMouseFromTopDown ||
        Selection->Active == YoriWinMultilineEditSelectMouseFromBottomUp) {
        ASSERT(Selection->LastLine != Selection->FirstLine || Selection->FirstCharOffset <= Selection->LastCharOffset);
    } else {
        ASSERT(Selection->LastLine != Selection->FirstLine || Selection->FirstCharOffset < Selection->LastCharOffset);
    }
    ASSERT(Selection->FirstCharOffset <= MultilineEdit->LineArray[Selection->FirstLine].LengthInChars);
    ASSERT(Selection->LastCharOffset <= MultilineEdit->LineArray[Selection->LastLine].LengthInChars);
}

/**
 Split one line into two.  This occurs when the user presses enter.

 @param MultilineEdit Pointer to the multiline edit control.

 @param LineIndex Specifies the line to split.

 @param CharOffset Specifies the number of characters to include in the first
        line.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditSplitLines(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD LineIndex,
    __in DWORD CharOffset
    )
{
    PYORI_STRING Line[2];
    YORI_STRING TargetLine;
    DWORD LinesToCopy;
    DWORD CharsNeededOnNewLine;

    if (LineIndex >= MultilineEdit->LinesPopulated) {
        return FALSE;
    }

    //
    //  The caller should have reallocated the line array as needed before
    //  calling this function.
    //

    ASSERT(LineIndex + 1 < MultilineEdit->LinesAllocated);
    if (LineIndex + 1 >= MultilineEdit->LinesAllocated) {
        return FALSE;
    }

    Line[0] = &MultilineEdit->LineArray[LineIndex];
    Line[1] = &MultilineEdit->LineArray[LineIndex + 1];

    //
    //  If there is text to preserve from the end of the first line, allocate
    //  a new line and copy the text into it.
    //

    if (CharOffset < Line[0]->LengthInChars) {
        CharsNeededOnNewLine = Line[0]->LengthInChars - CharOffset;
        if (!YoriLibAllocateString(&TargetLine, CharsNeededOnNewLine + YORI_WIN_MULTILINE_EDIT_LINE_PADDING)) {
            return FALSE;
        }

        memcpy(TargetLine.StartOfString, &Line[0]->StartOfString[CharOffset], CharsNeededOnNewLine * sizeof(TCHAR));
        TargetLine.LengthInChars = CharsNeededOnNewLine;
        Line[0]->LengthInChars = CharOffset;
    } else {
        YoriLibInitEmptyString(&TargetLine);
    }

    //
    //  If there are lines following this point, move them down.
    //

    LinesToCopy = MultilineEdit->LinesPopulated - LineIndex - 1;
    if (LinesToCopy > 0) {
        memmove(&MultilineEdit->LineArray[LineIndex + 2],
                &MultilineEdit->LineArray[LineIndex + 1], 
                LinesToCopy * sizeof(YORI_STRING));
    }

    //
    //  Copy the new line into position.
    //

    memcpy(Line[1], &TargetLine, sizeof(YORI_STRING));
    MultilineEdit->LinesPopulated++;
    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, LineIndex, MultilineEdit->LinesPopulated);

    return TRUE;
}

/**
 If a selection is currently active, delete all text in the selection.
 This implies deleting multiple lines, and/or merging the end of one line
 with the beginning of another.

 @param CtrlHandle Pointer to the multiline edit control containing the
        selection and contents of the buffer.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditDeleteSelection(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_MULTILINE_EDIT_SELECT Selection;
    DWORD CharsToCopy;
    DWORD CharsToDelete;
    DWORD LinesToDelete;
    DWORD LinesToCopy;
    DWORD FirstLineIndexToKeep;
    DWORD LineIndexToDelete;
    PYORI_STRING Line;
    PYORI_STRING FinalLine;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (!YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
        return TRUE;
    }

    Selection = &MultilineEdit->Selection;
    Line = &MultilineEdit->LineArray[Selection->FirstLine];

    //
    //  If the selection is one line, this is a simple case, because no
    //  line combining is required.
    //

    if (Selection->FirstLine == Selection->LastLine) {

        if (Selection->FirstCharOffset >= Selection->LastCharOffset) {
            return TRUE;
        }

        CharsToDelete = Selection->LastCharOffset - Selection->FirstCharOffset;
        CharsToCopy = Line->LengthInChars - Selection->LastCharOffset;

        if (CharsToCopy > 0) {
            memmove(&Line->StartOfString[Selection->FirstCharOffset],
                    &Line->StartOfString[Selection->LastCharOffset],
                    CharsToCopy * sizeof(TCHAR));
        }

        Line->LengthInChars -= CharsToDelete;

        YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, Selection->FirstCharOffset, Selection->FirstLine);

        YoriWinMultilineEditClearSelection(MultilineEdit);
        MultilineEdit->UserModified = TRUE;
        return TRUE;
    }

    LinesToDelete = 0;
    ASSERT(Selection->LastLine < MultilineEdit->LinesPopulated);
    FinalLine = &MultilineEdit->LineArray[Selection->LastLine];
    CharsToCopy = FinalLine->LengthInChars - Selection->LastCharOffset;

    //
    //  If the first part of the first line and the last part of the last
    //  line (the unselected regions of each) don't fit in the final line's
    //  allocation, reallocate it.
    //

    if (Selection->FirstCharOffset + CharsToCopy > FinalLine->LengthAllocated) {
        YORI_STRING NewLine;
        if (!YoriLibAllocateString(&NewLine, Selection->FirstCharOffset + CharsToCopy + YORI_WIN_MULTILINE_EDIT_LINE_PADDING)) {
            return FALSE;
        }

        memcpy(NewLine.StartOfString, Line->StartOfString, Selection->FirstCharOffset * sizeof(TCHAR));
        NewLine.LengthInChars = Selection->FirstCharOffset;
        YoriLibFreeStringContents(Line);
        memcpy(Line, &NewLine, sizeof(YORI_STRING));
    }

    //
    //  Move the combined regions into one line.
    //

    memcpy(&Line->StartOfString[Selection->FirstCharOffset],
           &FinalLine->StartOfString[Selection->LastCharOffset],
           CharsToCopy * sizeof(TCHAR));

    //
    //  Delete any completely selected lines.
    //

    Line->LengthInChars = Selection->FirstCharOffset + CharsToCopy;
    LinesToDelete = Selection->LastLine - Selection->FirstLine;

    for (LineIndexToDelete = 0; LineIndexToDelete < LinesToDelete; LineIndexToDelete++) {
        YoriLibFreeStringContents(&MultilineEdit->LineArray[Selection->FirstLine + 1 + LineIndexToDelete]);
    }

    FirstLineIndexToKeep = Selection->FirstLine + 1 + LinesToDelete;
    LinesToCopy = MultilineEdit->LinesPopulated - FirstLineIndexToKeep;
    memmove(&MultilineEdit->LineArray[Selection->FirstLine + 1],
            &MultilineEdit->LineArray[Selection->FirstLine + 1 + LinesToDelete],
            LinesToCopy * sizeof(YORI_STRING));

    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, Selection->FirstLine, MultilineEdit->LinesPopulated);

    MultilineEdit->LinesPopulated = MultilineEdit->LinesPopulated - LinesToDelete;

    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, Selection->FirstCharOffset, Selection->FirstLine);

    YoriWinMultilineEditClearSelection(MultilineEdit);
    MultilineEdit->UserModified = TRUE;

    return TRUE;
}


/**
 Build a single continuous string covering the selected range in a multiline
 edit control.

 @param CtrlHandle Pointer to the multiline edit control containing the
        selection and contents of the buffer.

 @param NewlineString Specifies the string to use to delimit lines.  This
        allows this routine to return text with any arbitrary line ending.

 @param SelectedText On successful completion, populated with a newly
        allocated buffer containing the selected text.  The caller is
        expected to free this buffer with @ref YoriLibFreeStringContents .
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditGetSelectedText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING NewlineString,
    __out PYORI_STRING SelectedText
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_MULTILINE_EDIT_SELECT Selection;
    DWORD CharsInRange;
    DWORD LinesInRange;
    DWORD LineIndex;
    PYORI_STRING Line;
    LPTSTR Ptr;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (!YoriWinMultilineEditSelectionActive(CtrlHandle)) {
        YoriLibInitEmptyString(SelectedText);
        return TRUE;
    }

    Selection = &MultilineEdit->Selection;
    Line = &MultilineEdit->LineArray[Selection->FirstLine];

    if (Selection->FirstLine == Selection->LastLine) {

        CharsInRange = Selection->LastCharOffset - Selection->FirstCharOffset;

        if (!YoriLibAllocateString(SelectedText, CharsInRange + 1)) {
            return FALSE;
        }

        memcpy(SelectedText->StartOfString, &Line->StartOfString[Selection->FirstCharOffset], CharsInRange * sizeof(TCHAR));
        SelectedText->LengthInChars = CharsInRange;
        SelectedText->StartOfString[CharsInRange] = '\0';
        return TRUE;
    }

    LinesInRange = Selection->LastLine - Selection->FirstLine;
    CharsInRange = Line->LengthInChars - Selection->FirstCharOffset;
    for (LineIndex = Selection->FirstLine + 1; LineIndex < Selection->LastLine; LineIndex++) {
        CharsInRange += MultilineEdit->LineArray[LineIndex].LengthInChars;
    }
    CharsInRange += Selection->LastCharOffset;

    if (!YoriLibAllocateString(SelectedText, CharsInRange + LinesInRange * NewlineString->LengthInChars + 1)) {
        return FALSE;
    }

    Ptr = SelectedText->StartOfString;
    memcpy(Ptr, &Line->StartOfString[Selection->FirstCharOffset], (Line->LengthInChars - Selection->FirstCharOffset) * sizeof(TCHAR));
    Ptr += (Line->LengthInChars - Selection->FirstCharOffset);
    for (LineIndex = Selection->FirstLine + 1; LineIndex < Selection->LastLine; LineIndex++) {
        memcpy(Ptr, NewlineString->StartOfString, NewlineString->LengthInChars * sizeof(TCHAR));
        Ptr += NewlineString->LengthInChars;
        memcpy(Ptr,
               MultilineEdit->LineArray[LineIndex].StartOfString,
               MultilineEdit->LineArray[LineIndex].LengthInChars * sizeof(TCHAR));
        Ptr += MultilineEdit->LineArray[LineIndex].LengthInChars;
    }
    memcpy(Ptr, NewlineString->StartOfString, NewlineString->LengthInChars * sizeof(TCHAR));
    Ptr += NewlineString->LengthInChars;
    memcpy(Ptr, MultilineEdit->LineArray[Selection->LastLine].StartOfString, Selection->LastCharOffset * sizeof(TCHAR));
    Ptr += Selection->LastCharOffset;

    SelectedText->LengthInChars = (DWORD)(Ptr - SelectedText->StartOfString);

    return TRUE;
}

/**
 Allocate new lines for the line array.  This is used when the number of lines
 in the file grows.  Note the allocations for the contents in each line are
 not performed here.

 @param MultilineEdit Pointer to the multiline edit control.

 @param NewLineCount Specifies the new total number of lines that the control
        should have allocated.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditReallocateLineArray(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD NewLineCount
    )
{
    PYORI_STRING NewLineArray;
    ASSERT(NewLineCount > MultilineEdit->LinesPopulated);

    NewLineArray = YoriLibReferencedMalloc(NewLineCount * sizeof(YORI_STRING));
    if (NewLineArray == NULL) {
        return FALSE;
    }

    if (MultilineEdit->LinesPopulated > 0) {
        memcpy(NewLineArray, MultilineEdit->LineArray, MultilineEdit->LinesPopulated * sizeof(YORI_STRING));
        YoriLibDereference(MultilineEdit->LineArray);
    }

    MultilineEdit->LineArray = NewLineArray;
    MultilineEdit->LinesAllocated = NewLineCount;
    return TRUE;
}

/**
 Insert a block of text, which may contain newlines, into the control at the
 current cursor position.

 @param CtrlHandle Pointer to the multiline edit control.

 @param Text Pointer to the text to insert.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditInsertTextAtCursor(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING Text
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;
    DWORD LineCount;
    DWORD LineIndex;
    DWORD Index;
    DWORD CharsThisLine;
    DWORD CharsFirstLine;
    DWORD CharsLastLine;
    DWORD CharsNeeded;
    YORI_STRING TrailingPortionOfCursorLine;
    PYORI_STRING Line;
    BOOLEAN TerminateLine;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    //
    //  Count the number of lines in the input text.  This may be zero.
    //

    LineCount = 0;
    for (Index = 0; Index < Text->LengthInChars; Index++) {
        if (Text->StartOfString[Index] == '\r') {
            LineCount++;
            if (Index + 1 < Text->LengthInChars &&
                Text->StartOfString[Index + 1] == '\n') {
                Index++;
            }
        } else if (Text->StartOfString[Index] == '\n') {
            LineCount++;
        }
    }

    //
    //  If new lines are being added, check if the line array is large
    //  enough and reallocate as needed.  Even if lines are already
    //  allocated, the current lines need to be moved downwards to make room
    //  for the lines that are about to be inserted.
    //

    if (LineCount > 0 || MultilineEdit->LinesPopulated == 0) {
        DWORD SourceLine;
        DWORD TargetLine;
        DWORD LinesNeeded;

        LinesNeeded = MultilineEdit->LinesPopulated + LineCount;
        if (MultilineEdit->LinesPopulated == 0) {
            LinesNeeded++;
        }
        if (LinesNeeded > MultilineEdit->LinesAllocated) {
            DWORD NewLineCount;
            NewLineCount = MultilineEdit->LinesAllocated * 2;

            if (NewLineCount < LinesNeeded) {
                NewLineCount = LinesNeeded;
                NewLineCount += 0x1000;
                NewLineCount = NewLineCount & ~(0xfff);
            } else if (NewLineCount < 0x1000) {
                NewLineCount = 0x1000;
            }

            if (!YoriWinMultilineEditReallocateLineArray(MultilineEdit, NewLineCount)) {
                return FALSE;
            }
        }

        if (MultilineEdit->LinesPopulated > 0) {
            SourceLine = MultilineEdit->CursorLine + 1;
            TargetLine = SourceLine + LineCount;
        } else {
            SourceLine = MultilineEdit->CursorLine;
            TargetLine = SourceLine + LineCount + 1;
        }
        if (SourceLine < MultilineEdit->LinesPopulated) {
            DWORD LinesToMove;
            LinesToMove = MultilineEdit->LinesPopulated - SourceLine;
            memmove(&MultilineEdit->LineArray[TargetLine],
                    &MultilineEdit->LineArray[SourceLine],
                    LinesToMove * sizeof(YORI_STRING));
        }

        for (Index = SourceLine; Index < TargetLine; Index++) {
            YoriLibInitEmptyString(&MultilineEdit->LineArray[Index]);
        }

        MultilineEdit->LinesPopulated = LinesNeeded;
    }

    //
    //  Record pointers to the string following the cursor on the current
    //  cursor line.  This text needs to be logically moved to the end of
    //  the newly inserted text, which is on a new line.  To achieve this
    //  the current text is pointed to, and the first line is processed
    //  last, after the last line has been constructed.
    //

    YoriLibInitEmptyString(&TrailingPortionOfCursorLine);
    if (MultilineEdit->CursorLine < MultilineEdit->LinesPopulated) {
        Line = &MultilineEdit->LineArray[MultilineEdit->CursorLine];
        if (MultilineEdit->CursorOffset < Line->LengthInChars) {
            TrailingPortionOfCursorLine.StartOfString = &Line->StartOfString[MultilineEdit->CursorOffset];
            TrailingPortionOfCursorLine.LengthInChars = Line->LengthInChars - MultilineEdit->CursorOffset;
        }
    }

    //
    //  Go through each line.  For all lines except the first, construct the
    //  new line.  Note that these lines should be empty lines due to the
    //  line rearrangement above.
    //

    LineIndex = 0;
    CharsThisLine = 0;
    CharsFirstLine = 0;
    CharsLastLine = 0;
    TerminateLine = FALSE;
    for (Index = 0; Index <= Text->LengthInChars; Index++) {

        //
        //  Look for end of line, and treat end of string as end of line
        //

        if (Index == Text->LengthInChars) {
            TerminateLine = TRUE;
        } else if (Text->StartOfString[Index] == '\r' ||
                   Text->StartOfString[Index] == '\n') {

            TerminateLine = TRUE;
        }

        if (TerminateLine) {

            //
            //  On the end of the first line, make a note of where the string
            //  is.  This is done so the trailing portion of the current text
            //  in the first line can be moved to the end of the last line
            //  without needing to reallocate it
            //

            if (LineIndex == 0) {
                CharsFirstLine = CharsThisLine;
                if (LineCount == 0) {
                    CharsLastLine = CharsThisLine;
                }
            } else {
                Line = &MultilineEdit->LineArray[MultilineEdit->CursorLine + LineIndex];
                ASSERT(Line->LengthInChars == 0);
                CharsNeeded = CharsThisLine;
                if (LineIndex == LineCount) {
                    CharsNeeded += TrailingPortionOfCursorLine.LengthInChars;
                }
                if (Line->LengthAllocated < CharsNeeded) {
                    YoriLibFreeStringContents(Line);
                    if (!YoriLibAllocateString(Line, CharsNeeded + YORI_WIN_MULTILINE_EDIT_LINE_PADDING)) {
                        return FALSE;
                    }
                }

                if (CharsThisLine > 0) {
                    memcpy(Line->StartOfString,
                           &Text->StartOfString[Index - CharsThisLine],
                           CharsThisLine * sizeof(TCHAR));
                }
                Line->LengthInChars = CharsThisLine;

                //
                //  On the final line, copy the final portion currently in the
                //  first line after the newly inserted text.  Save away the
                //  number of characters on this line so that the cursor can
                //  be positioned at that point.
                //

                if (LineIndex == LineCount) {
                    CharsLastLine = CharsThisLine;
                    if (TrailingPortionOfCursorLine.LengthInChars > 0) {
                        memcpy(&Line->StartOfString[CharsThisLine],
                               TrailingPortionOfCursorLine.StartOfString,
                               TrailingPortionOfCursorLine.LengthInChars * sizeof(TCHAR));
                        Line->LengthInChars += TrailingPortionOfCursorLine.LengthInChars;
                    }
                }
            }
            LineIndex++;
            CharsThisLine = 0;
            TerminateLine = FALSE;

            //
            //  Skip one extra char if this is a \r\n line
            //

            if (Text->StartOfString[Index] == '\r' && 
                Index + 1 < Text->LengthInChars &&
                Text->StartOfString[Index + 1] == '\n') {

                Index++;
            }
            continue;
        }

        CharsThisLine++;
    }

    //
    //  Because the first line was left unaltered in the regular loop to
    //  enable its text to be moved to the end of the last line, fix up
    //  the first line now.  If the first line is the same as the last
    //  line (LineCount == 0), we have to move the trailing portion after
    //  the newly inserted text.  Otherwise, that text is on a different
    //  line so we can completely ignore it.
    //

    if (LineCount != 0) {
        YoriLibInitEmptyString(&TrailingPortionOfCursorLine);
    }

    Line = &MultilineEdit->LineArray[MultilineEdit->CursorLine];
    if (MultilineEdit->CursorOffset + CharsFirstLine + TrailingPortionOfCursorLine.LengthInChars > Line->LengthAllocated) {
        if (!YoriLibReallocateString(Line, MultilineEdit->CursorOffset + CharsFirstLine + TrailingPortionOfCursorLine.LengthInChars + YORI_WIN_MULTILINE_EDIT_LINE_PADDING)) {
            return FALSE;
        }
    }

    if (TrailingPortionOfCursorLine.LengthInChars > 0) {
        memmove(&Line->StartOfString[MultilineEdit->CursorOffset + CharsFirstLine],
                TrailingPortionOfCursorLine.StartOfString,
                TrailingPortionOfCursorLine.LengthInChars * sizeof(TCHAR));
    }

    while (MultilineEdit->CursorOffset > Line->LengthInChars) {
        Line->StartOfString[Line->LengthInChars++] = ' ';
    }

    if (CharsFirstLine > 0) {
        memcpy(&Line->StartOfString[MultilineEdit->CursorOffset], Text->StartOfString, CharsFirstLine * sizeof(TCHAR));
        Line->LengthInChars = MultilineEdit->CursorOffset + CharsFirstLine + TrailingPortionOfCursorLine.LengthInChars;
    }

    //
    //  Set the cursor to be after the newly inserted range.
    //

    if (LineCount > 0) {
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, MultilineEdit->CursorLine, (DWORD)-1);
        YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, CharsLastLine, MultilineEdit->CursorLine + LineCount);
    } else {
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, MultilineEdit->CursorLine, MultilineEdit->CursorLine);
        YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, MultilineEdit->CursorOffset + CharsFirstLine, MultilineEdit->CursorLine);
    }

    return TRUE;
}

/**
 Add an array of lines to the end of a multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @param NewLines Pointer to an array of lines.

 @param NewLineCount Specifies the number of lines in the array.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditAppendLinesNoDataCopy(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING NewLines,
    __in DWORD NewLineCount
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (NewLineCount + MultilineEdit->LinesPopulated > MultilineEdit->LinesAllocated) {
        DWORD NewLinesToAllocate;
        NewLinesToAllocate = MultilineEdit->LinesAllocated * 2;

        if (NewLinesToAllocate < NewLineCount) {
            NewLinesToAllocate = NewLineCount;
            NewLinesToAllocate += 0x1000;
            NewLinesToAllocate = NewLinesToAllocate & ~(0xfff);
        } else if (NewLinesToAllocate < 0x1000) {
            NewLinesToAllocate = 0x1000;
        }

        if (!YoriWinMultilineEditReallocateLineArray(MultilineEdit, NewLinesToAllocate)) {
            return FALSE;
        }
    }

    memcpy(&MultilineEdit->LineArray[MultilineEdit->LinesPopulated], NewLines, NewLineCount * sizeof(YORI_STRING));
    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, MultilineEdit->LinesPopulated, MultilineEdit->LinesPopulated + NewLineCount);
    MultilineEdit->LinesPopulated += NewLineCount;

    YoriWinMultilineEditPaint(MultilineEdit);
    return TRUE;
}

/**
 Add the currently selected text to the clipboard and delete it from the
 buffer.

 @param CtrlHandle Pointer to the multiline edit control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditCutSelectedText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;
    YORI_STRING Newline;
    YORI_STRING Text;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);
    YoriLibConstantString(&Newline, _T("\r\n"));
    YoriLibInitEmptyString(&Text);

    if (!YoriWinMultilineEditGetSelectedText(CtrlHandle, &Newline, &Text)) {
        return FALSE;
    }

    if (!YoriLibCopyText(&Text)) {
        YoriLibFreeStringContents(&Text);
        return FALSE;
    }

    YoriLibFreeStringContents(&Text);
    YoriWinMultilineEditDeleteSelection(CtrlHandle);
    return TRUE;
}

/**
 Add the currently selected text to the clipboard and clear the selection.

 @param CtrlHandle Pointer to the multiline edit control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditCopySelectedText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;
    YORI_STRING Newline;
    YORI_STRING Text;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);
    YoriLibConstantString(&Newline, _T("\r\n"));
    YoriLibInitEmptyString(&Text);

    if (!YoriWinMultilineEditGetSelectedText(CtrlHandle, &Newline, &Text)) {
        return FALSE;
    }

    if (!YoriLibCopyText(&Text)) {
        YoriLibFreeStringContents(&Text);
        return FALSE;
    }

    YoriLibFreeStringContents(&Text);
    return TRUE;
}

/**
 Paste the text that is currently in the clipboard at the current cursor
 location.  Note this can update the cursor location.

 @param CtrlHandle Pointer to the multiline edit control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditPasteText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    YORI_STRING Text;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);
    YoriLibInitEmptyString(&Text);

    if (YoriWinMultilineEditSelectionActive(CtrlHandle)) {
        YoriWinMultilineEditDeleteSelection(CtrlHandle);
    }

    if (!YoriLibPasteText(&Text)) {
        return FALSE;
    }
    if (!YoriWinMultilineEditInsertTextAtCursor(CtrlHandle, &Text)) {
        YoriLibFreeStringContents(&Text);
        return FALSE;
    }

    YoriLibFreeStringContents(&Text);
    return TRUE;
}

/**
 Delete the character before the cursor and move later characters into
 position.

 @param MultilineEdit Pointer to the multiline edit control, indicating the
        current cursor location.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditBackspace(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    DWORD CharsToCopy;
    PYORI_STRING Line;
    if (MultilineEdit->CursorLine >= MultilineEdit->LinesPopulated) {
        return FALSE;
    }

    if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
        return YoriWinMultilineEditDeleteSelection(&MultilineEdit->Ctrl);
    }

    Line = &MultilineEdit->LineArray[MultilineEdit->CursorLine];

    //
    //  If we're at the beginning of the line, we may need to merge lines.
    //  If it's the first line, we're finished.
    //

    if (MultilineEdit->CursorOffset == 0) {

        DWORD CursorOffset;

        if (MultilineEdit->CursorLine == 0) {
            return FALSE;
        }

        MultilineEdit->UserModified = TRUE;

        CursorOffset = MultilineEdit->LineArray[MultilineEdit->CursorLine - 1].LengthInChars;

        if (!YoriWinMultilineEditMergeLines(MultilineEdit, MultilineEdit->CursorLine - 1)) {
            return FALSE;
        }

        YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, CursorOffset, MultilineEdit->CursorLine - 1);
        return TRUE;
    }

    MultilineEdit->UserModified = TRUE;

    if (MultilineEdit->CursorOffset < Line->LengthInChars) {
        CharsToCopy = Line->LengthInChars - MultilineEdit->CursorOffset;
        memmove(&Line->StartOfString[MultilineEdit->CursorOffset - 1],
                &Line->StartOfString[MultilineEdit->CursorOffset],
                CharsToCopy * sizeof(TCHAR));
    }

    if (MultilineEdit->CursorOffset <= Line->LengthInChars) {
        Line->LengthInChars--;
    }

    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, MultilineEdit->CursorLine, MultilineEdit->CursorLine);
    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, MultilineEdit->CursorOffset - 1, MultilineEdit->CursorLine);
    return TRUE;
}


/**
 Delete the character at the cursor and move later characters into position.

 @param MultilineEdit Pointer to the multiline edit control, indicating the
        current cursor location.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditDelete(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    DWORD CharsToCopy;
    PYORI_STRING Line;
    DWORD LineLengthNeeded;
    DWORD Index;

    if (MultilineEdit->CursorLine >= MultilineEdit->LinesPopulated) {
        return FALSE;
    }

    if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
        return YoriWinMultilineEditDeleteSelection(&MultilineEdit->Ctrl);
    }

    Line = &MultilineEdit->LineArray[MultilineEdit->CursorLine];

    if (MultilineEdit->CursorOffset >= Line->LengthInChars) {
        LineLengthNeeded = MultilineEdit->CursorOffset;

        if (LineLengthNeeded > Line->LengthAllocated) {
            DWORD LengthToAllocate;
            LengthToAllocate = Line->LengthAllocated * 2 + 80;
            if (LineLengthNeeded >= LengthToAllocate) {
                LengthToAllocate = LineLengthNeeded + 80;
            }

            if (!YoriLibReallocateString(Line, LengthToAllocate)) {
                return FALSE;
            }
        }

        if (LineLengthNeeded > Line->LengthInChars) {
            for (Index = Line->LengthInChars; Index < LineLengthNeeded; Index++) {
                Line->StartOfString[Index] = ' ';
            }

            Line->LengthInChars = LineLengthNeeded;
        }

        if (!YoriWinMultilineEditMergeLines(MultilineEdit, MultilineEdit->CursorLine)) {
            return FALSE;
        }

        MultilineEdit->UserModified = TRUE;
        return TRUE;
    }

    MultilineEdit->UserModified = TRUE;

    CharsToCopy = Line->LengthInChars - MultilineEdit->CursorOffset - 1;
    if (CharsToCopy > 0) {
        memmove(&Line->StartOfString[MultilineEdit->CursorOffset],
                &Line->StartOfString[MultilineEdit->CursorOffset + 1],
                CharsToCopy * sizeof(TCHAR));
    }
    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, MultilineEdit->CursorLine, MultilineEdit->CursorLine);
    Line->LengthInChars--;
    return TRUE;
}

/**
 Start a new selection from the current cursor location if no selection is
 currently active.  If one is active, this call is ignored.

 @param MultilineEdit Pointer to the multiline edit control that describes
        the selection and cursor location.
 
 @param Mouse If TRUE, the selection is being initiated by mouse operations.
        If FALSE, the selection is being initiated by keyboard operations.
 */
VOID
YoriWinMultilineEditStartSelectionAtCursor(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in BOOLEAN Mouse
    )
{
    PYORI_WIN_MULTILINE_EDIT_SELECT Selection;

    Selection = &MultilineEdit->Selection;

    //
    //  If a mouse selection is active and keyboard selection is requested
    //  or vice versa, clear the existing selection.
    //

    if (Mouse) {
        if (Selection->Active == YoriWinMultilineEditSelectKeyboardFromTopDown ||
            Selection->Active == YoriWinMultilineEditSelectKeyboardFromBottomUp ||
            Selection->Active == YoriWinMultilineEditSelectMouseComplete) {

            YoriWinMultilineEditClearSelection(MultilineEdit);
        }
    } else {
        if (Selection->Active == YoriWinMultilineEditSelectMouseFromTopDown ||
            Selection->Active == YoriWinMultilineEditSelectMouseFromBottomUp ||
            Selection->Active == YoriWinMultilineEditSelectMouseComplete) {

            YoriWinMultilineEditClearSelection(MultilineEdit);
        }
    }

    //
    //  If no selection is active, activate it.
    //

    if (Selection->Active == YoriWinMultilineEditSelectNotActive) {
        DWORD EffectiveCursorLine;
        DWORD EffectiveCursorOffset;
        EffectiveCursorLine = MultilineEdit->CursorLine;
        EffectiveCursorOffset = MultilineEdit->CursorOffset;
        if (MultilineEdit->LinesPopulated == 0) {
            EffectiveCursorLine = 0;
            EffectiveCursorOffset = 0;
        } else if (EffectiveCursorLine >= MultilineEdit->LinesPopulated) {

            EffectiveCursorLine = MultilineEdit->LinesPopulated - 1;
            EffectiveCursorOffset = MultilineEdit->LineArray[EffectiveCursorLine].LengthInChars;

        }

        if (EffectiveCursorLine < MultilineEdit->LinesPopulated) {
            if (EffectiveCursorOffset > MultilineEdit->LineArray[EffectiveCursorLine].LengthInChars) {
                EffectiveCursorOffset = MultilineEdit->LineArray[EffectiveCursorLine].LengthInChars;
            }
        }

        if (Mouse) {
            Selection->Active = YoriWinMultilineEditSelectMouseFromTopDown;
        } else {
            Selection->Active = YoriWinMultilineEditSelectKeyboardFromTopDown;
        }

        Selection->FirstLine = EffectiveCursorLine;
        Selection->FirstCharOffset = EffectiveCursorOffset;
        Selection->LastLine = EffectiveCursorLine;
        Selection->LastCharOffset = EffectiveCursorOffset;

        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, Selection->FirstLine, Selection->LastLine);
    }
}

/**
 Modify a selection line.  The selection line could move forward or backward,
 and any gap needs to be redrawn.

 @param MultilineEdit Pointer to the multiline edit control.

 @param SelectionLine Pointer to a selection line value to update.

 @param NewValue Specifies the new value of the selection value.
 */
VOID
YoriWinMultilineEditSetSelectionLine(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in PDWORD SelectionLine,
    __in DWORD NewValue
    )
{
    if (NewValue < *SelectionLine) {
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, NewValue, *SelectionLine);
    } else if (NewValue > *SelectionLine) {
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, *SelectionLine, NewValue);
    }

    *SelectionLine = NewValue;
}

/**
 Extend the current selection to the location of the cursor.

 @param MultilineEdit Pointer to the multiline edit control that describes
        the current selection and cursor location.
 */
VOID
YoriWinMultilineEditExtendSelectionToCursor(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    DWORD AnchorLine;
    DWORD AnchorOffset;
    DWORD EffectiveCursorLine;
    DWORD EffectiveCursorOffset;
    BOOLEAN MouseSelection = FALSE;

    PYORI_WIN_MULTILINE_EDIT_SELECT Selection;

    AnchorLine = 0;
    AnchorOffset = 0;

    Selection = &MultilineEdit->Selection;

    //
    //  Find the place where the selection started from the user's point of
    //  view.  This might be the beginning or end of the selection in terms
    //  of its location in the buffer.
    //

    ASSERT(YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl));
    if (Selection->Active == YoriWinMultilineEditSelectKeyboardFromTopDown ||
        Selection->Active == YoriWinMultilineEditSelectMouseFromTopDown) {

        AnchorLine = Selection->FirstLine;
        AnchorOffset = Selection->FirstCharOffset;

    } else if (Selection->Active == YoriWinMultilineEditSelectKeyboardFromBottomUp ||
               Selection->Active == YoriWinMultilineEditSelectMouseFromBottomUp) {

        AnchorLine = Selection->LastLine;
        AnchorOffset = Selection->LastCharOffset;

    } else {
        return;
    }

    if (Selection->Active == YoriWinMultilineEditSelectMouseFromTopDown ||
        Selection->Active == YoriWinMultilineEditSelectMouseFromBottomUp) {

        MouseSelection = TRUE;
    }

    //
    //  If there's no data, there's nothing to select
    //

    if (MultilineEdit->LinesPopulated == 0) {
        YoriWinMultilineEditClearSelection(MultilineEdit);
        return;
    }

    EffectiveCursorLine = MultilineEdit->CursorLine;
    EffectiveCursorOffset = MultilineEdit->CursorOffset;
    if (EffectiveCursorLine >= MultilineEdit->LinesPopulated) {
        EffectiveCursorLine = MultilineEdit->LinesPopulated - 1;
        EffectiveCursorOffset = MultilineEdit->LineArray[EffectiveCursorLine].LengthInChars;
    }

    if (EffectiveCursorOffset > MultilineEdit->LineArray[EffectiveCursorLine].LengthInChars) {
        EffectiveCursorOffset = MultilineEdit->LineArray[EffectiveCursorLine].LengthInChars;
    }

    if (EffectiveCursorLine < AnchorLine) {

        if (MouseSelection) {
            Selection->Active = YoriWinMultilineEditSelectMouseFromBottomUp;
        } else {
            Selection->Active = YoriWinMultilineEditSelectKeyboardFromBottomUp;
        }

        YoriWinMultilineEditSetSelectionLine(MultilineEdit, &Selection->LastLine, AnchorLine);
        Selection->LastCharOffset = AnchorOffset;
        YoriWinMultilineEditSetSelectionLine(MultilineEdit, &Selection->FirstLine, EffectiveCursorLine);
        Selection->FirstCharOffset = EffectiveCursorOffset;
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, EffectiveCursorLine, EffectiveCursorLine);

    } else if (EffectiveCursorLine > AnchorLine) {

        if (MouseSelection) {
            Selection->Active = YoriWinMultilineEditSelectMouseFromTopDown;
        } else {
            Selection->Active = YoriWinMultilineEditSelectKeyboardFromTopDown;
        }

        YoriWinMultilineEditSetSelectionLine(MultilineEdit, &Selection->FirstLine, AnchorLine);
        Selection->FirstCharOffset = AnchorOffset;
        YoriWinMultilineEditSetSelectionLine(MultilineEdit, &Selection->LastLine, EffectiveCursorLine);
        Selection->LastCharOffset = EffectiveCursorOffset;
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, EffectiveCursorLine, EffectiveCursorLine);

    } else {
        YoriWinMultilineEditSetSelectionLine(MultilineEdit, &Selection->FirstLine, AnchorLine);
        YoriWinMultilineEditSetSelectionLine(MultilineEdit, &Selection->LastLine, AnchorLine);
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, AnchorLine, AnchorLine);
        if (EffectiveCursorOffset < AnchorOffset) {

            if (MouseSelection) {
                Selection->Active = YoriWinMultilineEditSelectMouseFromBottomUp;
            } else {
                Selection->Active = YoriWinMultilineEditSelectKeyboardFromBottomUp;
            }

            Selection->LastCharOffset = AnchorOffset;
            Selection->FirstCharOffset = EffectiveCursorOffset;

        } else if (EffectiveCursorOffset > AnchorOffset) {

            if (MouseSelection) {
                Selection->Active = YoriWinMultilineEditSelectMouseFromTopDown;
            } else {
                Selection->Active = YoriWinMultilineEditSelectKeyboardFromTopDown;
            }

            Selection->FirstCharOffset = AnchorOffset;
            Selection->LastCharOffset = EffectiveCursorOffset;
        } else if (!MouseSelection) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
        } else {
            Selection->LastCharOffset = AnchorOffset;
            Selection->FirstCharOffset = AnchorOffset;
        }
    }

    YoriWinMultilineEditCheckSelectionState(MultilineEdit);
}

/**
 End selection extension.  This is invoked when the mouse button is released.
 At this point, the user may have selected text (click, hold, drag) or have
 just moved the cursor (click and release.)  We don't know which case happened
 until the mouse button is released (ie., now.)

 @param MultilineEdit Pointer to the multiline edit control.
 */
VOID
YoriWinMultilineEditFinishMouseSelection(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    PYORI_WIN_MULTILINE_EDIT_SELECT Selection;

    MultilineEdit->MouseButtonDown = FALSE;

    Selection = &MultilineEdit->Selection;
    Selection->Active = YoriWinMultilineEditSelectMouseComplete;

    //
    //  If no characters were selected, disable the selection
    //

    if (Selection->FirstLine == Selection->LastLine) {
        if (Selection->FirstCharOffset >= Selection->LastCharOffset) {
            Selection->Active = YoriWinMultilineEditSelectNotActive;
        }
    }
}


/**
 Set the selection range within a multiline edit control to an explicitly
 provided range.

 @param CtrlHandle Pointer to the multiline edit control.

 @param StartLine Specifies the line index of the beginning of the selection.

 @param StartOffset Specifies the character offset within the line to the
        beginning of the selection.

 @param EndLine Specifies the line index of the end of the selection.

 @param EndOffset Specifies the character offset within the line to the
        end of the selection.
 */
VOID
YoriWinMultilineEditSetSelectionRange(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD StartLine,
    __in DWORD StartOffset,
    __in DWORD EndLine,
    __in DWORD EndOffset
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    MultilineEdit = (PYORI_WIN_CTRL_MULTILINE_EDIT)CtrlHandle;

    YoriWinMultilineEditClearSelection(MultilineEdit);
    MultilineEdit->CursorLine = StartLine;
    MultilineEdit->CursorOffset = StartOffset;
    YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, EndOffset, EndLine);
    YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
    YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
    YoriWinMultilineEditPaint(MultilineEdit);
}

/**
 Return the current cursor location within a multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @param CursorOffset On successful completion, populated with the character
        offset within the line that the cursor is currently on.

 @param CursorLine On successful completion, populated with the line that the
        cursor is currently on.
 */
VOID
YoriWinMultilineEditGetCursorLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PDWORD CursorOffset,
    __out PDWORD CursorLine
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    MultilineEdit = (PYORI_WIN_CTRL_MULTILINE_EDIT)CtrlHandle;

    *CursorOffset = MultilineEdit->CursorOffset;
    *CursorLine = MultilineEdit->CursorLine;
}

/**
 Modify the cursor location within the multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @param NewCursorOffset The offset of the cursor from the beginning of the
        line, in buffer coordinates.

 @param NewCursorLine The buffer line that the cursor is located on.
 */
VOID
YoriWinMultilineEditSetCursorLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD NewCursorOffset,
    __in DWORD NewCursorLine
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    DWORD EffectiveNewCursorLine;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    EffectiveNewCursorLine = NewCursorLine;

    if (EffectiveNewCursorLine > MultilineEdit->LinesPopulated) {
        if (MultilineEdit->LinesPopulated > 0) {
            EffectiveNewCursorLine = MultilineEdit->LinesPopulated - 1;
        } else {
            EffectiveNewCursorLine = 0;
        }
    }
    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, EffectiveNewCursorLine);
    YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
    YoriWinMultilineEditPaint(MultilineEdit);
}

/**
 Return the current viewport location within a multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @param ViewportLeft On successful completion, populated with the first
        character displayed in the control.

 @param ViewportTop On successful completion, populated with the first line
        displayed in the control.
 */
VOID
YoriWinMultilineEditGetViewportLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PDWORD ViewportLeft,
    __out PDWORD ViewportTop
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    MultilineEdit = (PYORI_WIN_CTRL_MULTILINE_EDIT)CtrlHandle;

    *ViewportLeft = MultilineEdit->ViewportLeft;
    *ViewportTop = MultilineEdit->ViewportTop;
}

/**
 Modify the viewport location within the multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @param NewViewportLeft The display offset of the first character to display
        on the left of the control.

 @param NewViewportTop The display offset of the first character to display
        on the top of the control.
 */
VOID
YoriWinMultilineEditSetViewportLocation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD NewViewportLeft,
    __in DWORD NewViewportTop
    )
{
    COORD ClientSize;
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    DWORD EffectiveNewViewportTop;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    YoriWinGetControlClientSize(&MultilineEdit->Ctrl, &ClientSize);

    EffectiveNewViewportTop = NewViewportTop;

    if (EffectiveNewViewportTop > MultilineEdit->LinesPopulated) {
        if (MultilineEdit->LinesPopulated > 0) {
            EffectiveNewViewportTop = MultilineEdit->LinesPopulated - 1;
        } else {
            EffectiveNewViewportTop = 0;
        }
    }

    //
    //  Normally we'd call YoriWinMultilineEditEnsureCursorVisible,
    //  but this series of routines allow the viewport to move where the
    //  cursor isn't.
    //

    if (EffectiveNewViewportTop != MultilineEdit->ViewportTop) {
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, EffectiveNewViewportTop, (DWORD)-1);
        MultilineEdit->ViewportTop = EffectiveNewViewportTop;
        YoriWinMultilineEditRepaintScrollBar(MultilineEdit);
    }

    if (NewViewportLeft != MultilineEdit->ViewportLeft) {
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, EffectiveNewViewportTop, (DWORD)-1);
        MultilineEdit->ViewportLeft = NewViewportLeft;
    }
    YoriWinMultilineEditPaint(MultilineEdit);
}

/**
 Move the viewport up by one screenful and move the cursor to match.
 If we're at the top of the range, do nothing.  The somewhat strange
 logic here is patterned after the original edit.

 @param MultilineEdit Pointer to the multiline edit control specifying the
        viewport location and cursor location.  On completion these may be
        adjusted.

 @return TRUE to indicate the display was moved, FALSE if it was not.
 */
BOOLEAN
YoriWinMultilineEditPageUp(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    COORD ClientSize;
    DWORD ViewportHeight;
    DWORD NewCursorLine;
    DWORD NewCursorOffset;

    YoriWinGetControlClientSize(&MultilineEdit->Ctrl, &ClientSize);
    ViewportHeight = ClientSize.Y;

    if (MultilineEdit->CursorLine > 0) {
        if (MultilineEdit->CursorLine >= ViewportHeight) {
            NewCursorLine = MultilineEdit->CursorLine - ViewportHeight;
        } else {
            NewCursorLine = 0;
        }

        if (MultilineEdit->ViewportTop >= ViewportHeight) {
            MultilineEdit->ViewportTop = MultilineEdit->ViewportTop - ViewportHeight;
        } else {
            MultilineEdit->ViewportTop = 0;
        }

        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, MultilineEdit->ViewportTop, (DWORD)-1);

        YoriWinMultilineEditFindCursorCharFromDisplayChar(MultilineEdit, NewCursorLine, MultilineEdit->DisplayCursorOffset, &NewCursorOffset);
        YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, NewCursorLine);
        YoriWinMultilineEditRepaintScrollBar(MultilineEdit);
        return TRUE;
    }

    return FALSE;
}

/**
 Move the viewport down by one screenful and move the cursor to match.
 If we're at the bottom of the range, do nothing.  The somewhat strange
 logic here is patterned after the original edit.

 @param MultilineEdit Pointer to the multiline edit control specifying the
        viewport location and cursor location.  On completion these may be
        adjusted.

 @return TRUE to indicate the display was moved, FALSE if it was not.
 */
BOOLEAN
YoriWinMultilineEditPageDown(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    COORD ClientSize;
    DWORD ViewportHeight;
    DWORD NewCursorLine;
    DWORD NewCursorOffset;

    YoriWinGetControlClientSize(&MultilineEdit->Ctrl, &ClientSize);
    ViewportHeight = ClientSize.Y;

    if (MultilineEdit->ViewportTop + ViewportHeight < MultilineEdit->LinesPopulated) {
        MultilineEdit->ViewportTop = MultilineEdit->ViewportTop + ViewportHeight;
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, MultilineEdit->ViewportTop, (DWORD)-1);
        NewCursorLine = MultilineEdit->CursorLine;
        if (MultilineEdit->CursorLine + ViewportHeight < MultilineEdit->LinesPopulated) {
            NewCursorLine = MultilineEdit->CursorLine + ViewportHeight;
        } else if (MultilineEdit->CursorLine + 1 < MultilineEdit->LinesPopulated) {
            NewCursorLine = MultilineEdit->LinesPopulated - 1;
        }
        YoriWinMultilineEditFindCursorCharFromDisplayChar(MultilineEdit, NewCursorLine, MultilineEdit->DisplayCursorOffset, &NewCursorOffset);
        YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, NewCursorLine);
        YoriWinMultilineEditRepaintScrollBar(MultilineEdit);
        return TRUE;
    }

    return FALSE;
}

/**
 Scroll the multiline edit based on a mouse wheel notification.

 @param MultilineEdit Pointer to the multiline edit to scroll.

 @param LinesToMove The number of lines to scroll.

 @param MoveUp If TRUE, scroll backwards through the text.  If FALSE,
        scroll forwards through the text.
 */
VOID
YoriWinMultilineEditNotifyMouseWheel(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD LinesToMove,
    __in BOOLEAN MoveUp
    )
{
    COORD ClientSize;
    DWORD LineCountToDisplay;
    DWORD NewViewportTop;

    YoriWinGetControlClientSize(&MultilineEdit->Ctrl, &ClientSize);
    LineCountToDisplay = ClientSize.Y;

    if (MoveUp) {
        if (MultilineEdit->ViewportTop < LinesToMove) {
            NewViewportTop = 0;
        } else {
            NewViewportTop = MultilineEdit->ViewportTop - LinesToMove;
        }
    } else {
        if (MultilineEdit->ViewportTop + LinesToMove + LineCountToDisplay > MultilineEdit->LinesPopulated) {
            if (MultilineEdit->LinesPopulated >= LineCountToDisplay) {
                NewViewportTop = MultilineEdit->LinesPopulated - LineCountToDisplay;
            } else {
                NewViewportTop = 0;
            }
        } else {
            NewViewportTop = MultilineEdit->ViewportTop + LinesToMove;
        }
    }

    YoriWinMultilineEditSetViewportLocation(&MultilineEdit->Ctrl, MultilineEdit->ViewportLeft, NewViewportTop);
}


/**
 Process a key that may be an enhanced key.  Some of these keys can be either
 enhanced or non-enhanced.

 @param MultilineEdit Pointer to the multiline edit control, indicating the
        current cursor location.

 @param Event Pointer to the event describing the state of the key being
        pressed.

 @return TRUE to indicate the key has been processed, FALSE if it is an
         unknown key.
 */
BOOLEAN
YoriWinMultilineEditProcessPossiblyEnhancedKey(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in PYORI_WIN_EVENT Event
    )
{
    BOOLEAN Recognized;
    DWORD NewCursorLine;
    DWORD NewCursorOffset;
    Recognized = FALSE;

    if (Event->KeyDown.VirtualKeyCode == VK_LEFT) {
        if (MultilineEdit->CursorOffset > 0) {
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
            } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
                YoriWinMultilineEditClearSelection(MultilineEdit);
            }
            NewCursorOffset = MultilineEdit->CursorOffset - 1;
            YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, MultilineEdit->CursorLine);
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
            }
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_RIGHT) {
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
        } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
        }
        NewCursorOffset = MultilineEdit->CursorOffset + 1;
        YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, MultilineEdit->CursorLine);
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
        }
        YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
        YoriWinMultilineEditPaint(MultilineEdit);
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_HOME) {
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
        } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
        }
        if (MultilineEdit->CursorOffset != 0) {
            NewCursorOffset = 0;
            YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, MultilineEdit->CursorLine);
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
            }
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_END) {
        DWORD FinalChar = 0;
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
        } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
        }
        if (MultilineEdit->CursorLine < MultilineEdit->LinesPopulated) {
            FinalChar = MultilineEdit->LineArray[MultilineEdit->CursorLine].LengthInChars;
        }
        if (MultilineEdit->CursorOffset != FinalChar) {
            YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, FinalChar, MultilineEdit->CursorLine);
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
            }
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_INSERT) {
        if (!MultilineEdit->ReadOnly) {
            YoriWinMultilineEditToggleInsert(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_UP) {
        if (MultilineEdit->CursorLine != 0) {
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
            } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
                YoriWinMultilineEditClearSelection(MultilineEdit);
            }
            NewCursorLine = MultilineEdit->CursorLine - 1;
            YoriWinMultilineEditFindCursorCharFromDisplayChar(MultilineEdit, NewCursorLine, MultilineEdit->DisplayCursorOffset, &NewCursorOffset);
            YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, NewCursorLine);
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
            }
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_DOWN) {
        if (MultilineEdit->CursorLine + 1 < MultilineEdit->LinesPopulated) {
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
            } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
                YoriWinMultilineEditClearSelection(MultilineEdit);
            }
            NewCursorLine = MultilineEdit->CursorLine + 1;
            YoriWinMultilineEditFindCursorCharFromDisplayChar(MultilineEdit, NewCursorLine, MultilineEdit->DisplayCursorOffset, &NewCursorOffset);
            YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, NewCursorLine);
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
            }
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_PRIOR) {
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
        } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
        }

        if (YoriWinMultilineEditPageUp(MultilineEdit)) {
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
            }
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_NEXT) {
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
        } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
        }
        if (YoriWinMultilineEditPageDown(MultilineEdit)) {
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
            }
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_BACK) {

        if (!MultilineEdit->ReadOnly && YoriWinMultilineEditBackspace(MultilineEdit)) {
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;

    } else if (Event->KeyDown.VirtualKeyCode == VK_DELETE) {

        if (!MultilineEdit->ReadOnly && YoriWinMultilineEditDelete(MultilineEdit)) {
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_ESCAPE) {
        if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    }

    return Recognized;
}

/**
 Process a key that may be an enhanced key with ctrl held.  Some of these
 keys can be either enhanced or non-enhanced.

 @param MultilineEdit Pointer to the multiline edit control, indicating the
        current cursor location.

 @param Event Pointer to the event describing the state of the key being
        pressed.

 @return TRUE to indicate the key has been processed, FALSE if it is an
         unknown key.
 */
BOOLEAN
YoriWinMultilineEditProcessPossiblyEnhancedCtrlKey(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in PYORI_WIN_EVENT Event
    )
{
    BOOLEAN Recognized;
    Recognized = FALSE;

    if (Event->KeyDown.VirtualKeyCode == VK_HOME) {
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
        } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
        }
        if (MultilineEdit->CursorOffset != 0 || MultilineEdit->CursorLine != 0) {
            YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, 0, 0);
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
            }
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_END) {
        DWORD FinalChar = 0;
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
        } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
        }
        if (MultilineEdit->LinesPopulated > 0) {
            FinalChar = MultilineEdit->LineArray[MultilineEdit->LinesPopulated - 1].LengthInChars;
            if (MultilineEdit->CursorLine != MultilineEdit->LinesPopulated - 1 || MultilineEdit->CursorOffset != FinalChar) {
                YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, FinalChar, MultilineEdit->LinesPopulated - 1);
                if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                    YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
                }
                YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                YoriWinMultilineEditPaint(MultilineEdit);
            }
        }
        Recognized = TRUE;
    }

    return Recognized;
}


/**
 When the user presses a regular key, insert that key into the control.

 @param MultilineEdit Pointer to the multiline edit control, specifying the
        location of the cursor and contents of the control.

 @param Char The character to insert.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditAddChar(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in TCHAR Char
    )
{
    DWORD LineLengthNeeded;
    DWORD NewCursorOffset;
    PYORI_STRING Line;

    if (YoriWinMultilineEditSelectionActive(MultilineEdit)) {
        YoriWinMultilineEditDeleteSelection(MultilineEdit);
    }

    //
    //  If the line array doesn't have enough lines, allocate more.
    //

    if (MultilineEdit->CursorLine >= MultilineEdit->LinesAllocated ||
        (Char == '\r' && MultilineEdit->LinesPopulated == MultilineEdit->LinesAllocated)) {
        DWORD NewLineCount;

        NewLineCount = MultilineEdit->LinesAllocated * 2;
        if (NewLineCount < 0x1000) {
            NewLineCount = 0x1000;
        }

        if (!YoriWinMultilineEditReallocateLineArray(MultilineEdit, NewLineCount)) {
            return FALSE;
        }
    }

    //
    //  If the user wants to split a line, split it.
    //

    if (Char == '\r') {
        BOOLEAN Result;
        Result = YoriWinMultilineEditSplitLines(MultilineEdit, MultilineEdit->CursorLine, MultilineEdit->CursorOffset);
        if (Result) {
            YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, 0, MultilineEdit->CursorLine + 1);
            MultilineEdit->DisplayCursorOffset = 0;
        }
        return Result;
    }

    //
    //  If the line array isn't populated to the current point, populate it.
    //

    for (;
         MultilineEdit->LinesPopulated <= MultilineEdit->CursorLine;
         MultilineEdit->LinesPopulated++) {

        YoriLibInitEmptyString(&MultilineEdit->LineArray[MultilineEdit->LinesPopulated]);
    }

    MultilineEdit->UserModified = TRUE;

    //
    //  Calculate how much space is needed in the line.  If there's not
    //  enough, reallocate it.
    //

    Line = &MultilineEdit->LineArray[MultilineEdit->CursorLine];

    LineLengthNeeded = 0;
    if (MultilineEdit->InsertMode) {
        LineLengthNeeded = Line->LengthInChars + 1;
    }

    if (MultilineEdit->CursorOffset >= LineLengthNeeded) {
        LineLengthNeeded = MultilineEdit->CursorOffset + 1;
    }

    if (LineLengthNeeded > Line->LengthAllocated) {
        DWORD LengthToAllocate;
        LengthToAllocate = Line->LengthAllocated * 2 + 80;
        if (LineLengthNeeded >= LengthToAllocate) {
            LengthToAllocate = LineLengthNeeded + 80;
        }

        if (!YoriLibReallocateString(Line, LengthToAllocate)) {
            return FALSE;
        }
    }

    //
    //  If inserting, move following characters forward.
    //

    if (MultilineEdit->InsertMode &&
        MultilineEdit->CursorOffset + 1 < Line->LengthInChars) {

        DWORD CharsToCopy;
        CharsToCopy = Line->LengthInChars - MultilineEdit->CursorOffset;
        if (MultilineEdit->CursorOffset < Line->LengthInChars) {
            memmove(&Line->StartOfString[MultilineEdit->CursorOffset + 1],
                    &Line->StartOfString[MultilineEdit->CursorOffset],
                    CharsToCopy * sizeof(TCHAR));

            Line->LengthInChars++;
        }
    }

    //
    //  If the new character is beyond the current end of the line, pad the
    //  gap with spaces.
    //

    for (;
         Line->LengthInChars < MultilineEdit->CursorOffset;
         Line->LengthInChars++) {

        Line->StartOfString[Line->LengthInChars] = ' ';
    }

    //
    //  Now actually add the specified character.
    //

    Line->StartOfString[MultilineEdit->CursorOffset] = Char;
    NewCursorOffset = MultilineEdit->CursorOffset + 1;
    if (NewCursorOffset > Line->LengthInChars) {
        Line->LengthInChars = NewCursorOffset;
    }

    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, MultilineEdit->CursorLine, MultilineEdit->CursorLine);
    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, MultilineEdit->CursorLine);

    return TRUE;

}

/**
 Process input events for a multiline edit control.

 @param Ctrl Pointer to the multiline edit control.

 @param Event Pointer to the input event.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate that regular processing
         should continue (although this does not imply that no processing
         has already occurred.)
 */
BOOLEAN
YoriWinMultilineEditEventHandler(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    DWORD Index;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);
    switch(Event->EventType) {
        case YoriWinEventParentDestroyed:
            for (Index = 0; Index < MultilineEdit->LinesPopulated; Index++) {
                YoriLibFreeStringContents(&MultilineEdit->LineArray[Index]);
            }
            if (MultilineEdit->LineArray != NULL) {
                YoriLibDereference(MultilineEdit->LineArray);
                MultilineEdit->LineArray = NULL;
            }
            YoriLibFreeStringContents(&MultilineEdit->Caption);
            YoriWinDestroyControl(Ctrl);
            YoriLibDereference(MultilineEdit);
            break;
        case YoriWinEventLoseFocus:
            ASSERT(MultilineEdit->HasFocus);
            MultilineEdit->HasFocus = FALSE;
            YoriWinMultilineEditPaint(MultilineEdit);
            break;
        case YoriWinEventGetFocus:
            ASSERT(!MultilineEdit->HasFocus);
            MultilineEdit->HasFocus = TRUE;
            YoriWinMultilineEditPaint(MultilineEdit);
            break;
        case YoriWinEventKeyDown:

            //
            // This code is trying to handle the AltGr cases while not
            // handling pure right Alt which would normally be an accelerator.
            //

            if (Event->KeyDown.CtrlMask == 0 ||
                Event->KeyDown.CtrlMask == SHIFT_PRESSED ||
                Event->KeyDown.CtrlMask == (LEFT_CTRL_PRESSED | LEFT_ALT_PRESSED) ||
                Event->KeyDown.CtrlMask == (LEFT_CTRL_PRESSED | LEFT_ALT_PRESSED | SHIFT_PRESSED) ||
                Event->KeyDown.CtrlMask == (LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED) ||
                Event->KeyDown.CtrlMask == (LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | SHIFT_PRESSED)) {


                if (!YoriWinMultilineEditProcessPossiblyEnhancedKey(MultilineEdit, Event)) {
                    if (Event->KeyDown.Char != '\0' &&
                        Event->KeyDown.Char != '\n') {

                        if (!MultilineEdit->ReadOnly) {
                            YoriWinMultilineEditAddChar(MultilineEdit, Event->KeyDown.Char);
                            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                            YoriWinMultilineEditPaint(MultilineEdit);
                            return TRUE;
                        }
                    }
                }
            } else if (Event->KeyDown.CtrlMask == LEFT_CTRL_PRESSED ||
                       Event->KeyDown.CtrlMask == RIGHT_CTRL_PRESSED) {

                if (!YoriWinMultilineEditProcessPossiblyEnhancedCtrlKey(MultilineEdit, Event)) {
                    if (Event->KeyDown.VirtualKeyCode == 'A') {
                        if (MultilineEdit->LinesPopulated > 0) {
                            YoriWinMultilineEditSetSelectionRange(Ctrl, 0, 0, MultilineEdit->LinesPopulated - 1, MultilineEdit->LineArray[MultilineEdit->LinesPopulated - 1].LengthInChars);
                        }
                        return TRUE;
                    } else if (Event->KeyDown.VirtualKeyCode == 'C') {
                        if (YoriWinMultilineEditCopySelectedText(Ctrl)) {
                            YoriWinMultilineEditClearSelection(MultilineEdit);
                            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                            YoriWinMultilineEditPaint(MultilineEdit);
                        }
                        return TRUE;
                    } else if (Event->KeyDown.VirtualKeyCode == 'V') {
                        if (YoriWinMultilineEditPasteText(Ctrl)) {
                            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                            YoriWinMultilineEditPaint(MultilineEdit);
                        }
                        return TRUE;
                    } else if (Event->KeyDown.VirtualKeyCode == 'X') {
                        if (YoriWinMultilineEditCutSelectedText(Ctrl)) {
                            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                            YoriWinMultilineEditPaint(MultilineEdit);
                        }
                        return TRUE;
                    }
                }
            } else if (Event->KeyDown.CtrlMask == ENHANCED_KEY ||
                       Event->KeyDown.CtrlMask == (ENHANCED_KEY | SHIFT_PRESSED)) {
                YoriWinMultilineEditProcessPossiblyEnhancedKey(MultilineEdit, Event);
            } else if (Event->KeyDown.CtrlMask == (ENHANCED_KEY | LEFT_CTRL_PRESSED) ||
                       Event->KeyDown.CtrlMask == (ENHANCED_KEY | RIGHT_CTRL_PRESSED) ||
                       Event->KeyDown.CtrlMask == (SHIFT_PRESSED | LEFT_CTRL_PRESSED) ||
                       Event->KeyDown.CtrlMask == (SHIFT_PRESSED | RIGHT_CTRL_PRESSED) ||
                       Event->KeyDown.CtrlMask == (ENHANCED_KEY | SHIFT_PRESSED | LEFT_CTRL_PRESSED) ||
                       Event->KeyDown.CtrlMask == (ENHANCED_KEY | SHIFT_PRESSED | RIGHT_CTRL_PRESSED)
                       ) {
                YoriWinMultilineEditProcessPossiblyEnhancedCtrlKey(MultilineEdit, Event);
            }
            break;

        case YoriWinEventMouseWheelDownInClient:
        case YoriWinEventMouseWheelDownInNonClient:
            YoriWinMultilineEditNotifyMouseWheel(MultilineEdit, Event->MouseWheel.LinesToMove, FALSE);
            break;

        case YoriWinEventMouseWheelUpInClient:
        case YoriWinEventMouseWheelUpInNonClient:
            YoriWinMultilineEditNotifyMouseWheel(MultilineEdit, Event->MouseWheel.LinesToMove, TRUE);
            break;

        case YoriWinEventMouseUpInNonClient:
            if (MultilineEdit->Selection.Active == YoriWinMultilineEditSelectMouseFromTopDown ||
                MultilineEdit->Selection.Active == YoriWinMultilineEditSelectMouseFromBottomUp) {

                YoriWinMultilineEditFinishMouseSelection(MultilineEdit);
            }
            // Intentional fallthrough
        case YoriWinEventMouseDownInNonClient:
        case YoriWinEventMouseDoubleClickInNonClient:
            {
                PYORI_WIN_CTRL Child;
                COORD ChildLocation;
                BOOLEAN InChildClientArea;
                Child = YoriWinFindControlAtCoordinates(Ctrl,
                                                        Event->MouseDown.Location,
                                                        FALSE,
                                                        &ChildLocation,
                                                        &InChildClientArea);

                if (Child != NULL) {
                    if (YoriWinTranslateMouseEventForChild(Event, Child, ChildLocation, InChildClientArea)) {
                        return TRUE;
                    }
                    return FALSE;
                }
            }
            break;
        case YoriWinEventMouseDownInClient:
            {
                DWORD NewCursorLine;
                DWORD NewCursorChar;

                if (YoriWinMultilineEditTranslateViewportCoordinatesToCursorCoordinates(MultilineEdit, Event->MouseDown.Location.X, Event->MouseDown.Location.Y, &NewCursorLine, &NewCursorChar)) {

                    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorChar, NewCursorLine);
                    YoriWinMultilineEditClearSelection(MultilineEdit);
                    YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, TRUE);
                    MultilineEdit->MouseButtonDown = TRUE;

                    YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                    YoriWinMultilineEditPaint(MultilineEdit);
                }
            }
            break;
        case YoriWinEventMouseMoveInClient:
            if (MultilineEdit->MouseButtonDown) {
                DWORD NewCursorLine;
                DWORD NewCursorChar;

                YoriWinMultilineEditTranslateViewportCoordinatesToCursorCoordinates(MultilineEdit, Event->MouseMove.Location.X, Event->MouseMove.Location.Y, &NewCursorLine, &NewCursorChar);

                YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorChar, NewCursorLine);
                if (MultilineEdit->Selection.Active == YoriWinMultilineEditSelectMouseFromTopDown ||
                    MultilineEdit->Selection.Active == YoriWinMultilineEditSelectMouseFromBottomUp) {
                    YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
                } else {
                    YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, TRUE);
                }

                YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                YoriWinMultilineEditPaint(MultilineEdit);
            }
            break;
        case YoriWinEventMouseUpInClient:
        case YoriWinEventMouseUpOutsideWindow:
            if (MultilineEdit->Selection.Active == YoriWinMultilineEditSelectMouseFromTopDown ||
                MultilineEdit->Selection.Active == YoriWinMultilineEditSelectMouseFromBottomUp) {

                YoriWinMultilineEditFinishMouseSelection(MultilineEdit);
            }
            break;
    }

    return FALSE;
}

/**
 Clear all of the contents of a multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditClear(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    DWORD Index;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    for (Index = 0; Index < MultilineEdit->LinesPopulated; Index++) {
        YoriLibFreeStringContents(&MultilineEdit->LineArray[Index]);
    }

    MultilineEdit->LinesPopulated = 0;
    MultilineEdit->ViewportTop = 0;
    MultilineEdit->ViewportLeft = 0;

    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, MultilineEdit->ViewportTop, (DWORD)-1);
    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, 0, 0);

    YoriWinMultilineEditPaint(MultilineEdit);
    return TRUE;
}

/**
 Return the number of lines with data in a multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @return The number of lines with data.
 */
DWORD
YoriWinMultilineEditGetLineCount(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    return MultilineEdit->LinesPopulated;
}

/**
 Return the string that describes a single line within a multiline edit
 control.  As of this writing, this is a pointer to the string used by the
 control itself, and as such is only meaningful if the text cannot be
 altered by any mechanism.

 @param CtrlHandle Pointer to the multiline edit control.

 @param Index The line number to return.

 @return Pointer to the line string, or NULL if the line index is out of
         bounds.
 */
PYORI_STRING
YoriWinMultilineEditGetLineByIndex(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD Index
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (Index >= MultilineEdit->LinesPopulated) {
        return NULL;
    }

    return &MultilineEdit->LineArray[Index];
}

/**
 Set the title to display on the top of a multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @param Caption Pointer to the caption to display on the top of the multiline
        edit control.  This can point to an empty string to indicate no
        caption should be displayed.

 @return TRUE to indicate the caption was successfully updated, or FALSE on
         failure.
 */
BOOLEAN
YoriWinMultilineEditSetCaption(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING Caption
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    YORI_STRING NewCaption;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (MultilineEdit->Caption.LengthAllocated < Caption->LengthInChars) {
        if (!YoriLibAllocateString(&NewCaption, Caption->LengthInChars)) {
            return FALSE;
        }

        YoriLibFreeStringContents(&MultilineEdit->Caption);
        memcpy(&MultilineEdit->Caption, &NewCaption, sizeof(YORI_STRING));
    }

    if (Caption->LengthInChars > 0) {
        memcpy(MultilineEdit->Caption.StartOfString, Caption->StartOfString, Caption->LengthInChars * sizeof(TCHAR));
    }
    MultilineEdit->Caption.LengthInChars = Caption->LengthInChars;
    YoriWinMultilineEditPaintNonClient(MultilineEdit);
    return TRUE;
}

/**
 Indicates whether the multiline edit control has been modified by the user.
 This is typically used after some external event indicates that the buffer
 should be considered unchanged, eg., a file is successfully saved.

 @param CtrlHandle Pointer to the multiline edit contorl.

 @param ModifyState TRUE if the control should consider itself modified by
        the user, FALSE if it should not.

 @return TRUE if the control was previously modified by the user, FALSE if it
         was not.
 */
BOOLEAN
YoriWinMultilineEditSetModifyState(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN ModifyState
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    BOOLEAN PreviousValue;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    PreviousValue = MultilineEdit->UserModified;
    MultilineEdit->UserModified = ModifyState;
    return PreviousValue;
}

/**
 Query the number of spaces to display for each tab character in the buffer.

 @param CtrlHandle Pointer to the multiline edit control.

 @param TabWidth On successful completion, set to the current tab width.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMultilineEditGetTabWidth(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PDWORD TabWidth
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    *TabWidth = MultilineEdit->TabWidth;
    return TRUE;
}

/**
 Set the number of spaces to display for each tab character in the buffer.

 @param CtrlHandle Pointer to the multiline edit control.

 @param TabWidth Specifies the new tab width.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMultilineEditSetTabWidth(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD TabWidth
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    MultilineEdit->TabWidth = TabWidth;
    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, 0, MultilineEdit->LinesPopulated);
    return TRUE;
}

/**
 Returns TRUE if the multiline edit control has been modified by the user
 since the last time @ref YoriWinMultilineEditSetModifyState indicated that
 no user modification has occurred.

 @param CtrlHandle Pointer to the multiline edit contorl.

 @return TRUE if the control has been modified by the user, FALSE if it has
         not.
 */
BOOLEAN
YoriWinMultilineEditGetModifyState(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    return MultilineEdit->UserModified;
}

/**
 Set a function to call when the cursor location changes.

 @param CtrlHandle Pointer to the multiline edit control.

 @param NotifyCallback Pointer to a function to invoke when the cursor
        moves.

 @return TRUE to indicate the callback function was successfully updated,
         FALSE to indicate another callback function was already present.
 */
BOOLEAN
YoriWinMultilineEditSetCursorMoveNotifyCallback(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_NOTIFY_CURSOR_MOVE NotifyCallback
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (MultilineEdit->CursorMoveCallback != NULL) {
        return FALSE;
    }

    MultilineEdit->CursorMoveCallback = NotifyCallback;

    return TRUE;
}

/**
 Invoked when the user manipulates the scroll bar to indicate that the
 position within the multiline edit should be updated.

 @param ScrollCtrlHandle Pointer to the scroll bar control.
 */
VOID
YoriWinMultilineEditNotifyScrollChange(
    __in PYORI_WIN_CTRL_HANDLE ScrollCtrlHandle
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    DWORDLONG ScrollValue;
    COORD ClientSize;
    WORD ElementCountToDisplay;
    PYORI_WIN_CTRL ScrollCtrl;
    DWORD NewViewportTop;

    ScrollCtrl = (PYORI_WIN_CTRL)ScrollCtrlHandle;
    MultilineEdit = CONTAINING_RECORD(ScrollCtrl->Parent, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);
    ASSERT(MultilineEdit->VScrollCtrl == ScrollCtrl);

    YoriWinGetControlClientSize(&MultilineEdit->Ctrl, &ClientSize);
    ElementCountToDisplay = ClientSize.Y;
    NewViewportTop = MultilineEdit->ViewportTop;

    ScrollValue = YoriWinScrollBarGetPosition(ScrollCtrl);
    ASSERT(ScrollValue <= MultilineEdit->LinesPopulated);
    if (ScrollValue + ElementCountToDisplay > MultilineEdit->LinesPopulated) {
        if (MultilineEdit->LinesPopulated >= ElementCountToDisplay) {
            NewViewportTop = MultilineEdit->LinesPopulated - ElementCountToDisplay;
        } else {
            NewViewportTop = 0;
        }
    } else {
        if (ScrollValue < MultilineEdit->LinesPopulated) {
            NewViewportTop = (DWORD)ScrollValue;
        }
    }

    if (NewViewportTop != MultilineEdit->ViewportTop) {
        MultilineEdit->ViewportTop = NewViewportTop;
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, NewViewportTop, (DWORD)-1);
    } else {
        return;
    }

    if (MultilineEdit->CursorLine < MultilineEdit->ViewportTop) {
        YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, MultilineEdit->CursorOffset, MultilineEdit->ViewportTop);
    } else if (MultilineEdit->CursorLine >= MultilineEdit->ViewportTop + ClientSize.Y) {
        YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, MultilineEdit->CursorOffset, MultilineEdit->ViewportTop + ClientSize.Y - 1);
    }

    YoriWinMultilineEditPaint(MultilineEdit);
}

/**
 Set the size and location of a multiline edit control, and redraw the
 contents.

 @param CtrlHandle Pointer to the multiline edit to resize or reposition.

 @param CtrlRect Specifies the new size and position of the multiline edit.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (!YoriWinControlReposition(Ctrl, CtrlRect)) {
        return FALSE;
    }

    if (MultilineEdit->VScrollCtrl != NULL) {
        SMALL_RECT ScrollBarRect;

        ScrollBarRect.Left = (SHORT)(MultilineEdit->Ctrl.FullRect.Right - MultilineEdit->Ctrl.FullRect.Left);
        ScrollBarRect.Right = ScrollBarRect.Left;
        ScrollBarRect.Top = 1;
        ScrollBarRect.Bottom = (SHORT)(MultilineEdit->Ctrl.FullRect.Bottom - MultilineEdit->Ctrl.FullRect.Top - 1);

        YoriWinScrollBarReposition(MultilineEdit->VScrollCtrl, &ScrollBarRect);
    }

    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, 0, (DWORD)-1);
    YoriWinMultilineEditPaintNonClient(MultilineEdit);
    YoriWinMultilineEditPaint(MultilineEdit);

    return TRUE;
}


/**
 Create a multiline edit control and add it to a window.  This is destroyed
 when the window is destroyed.

 @param ParentHandle Pointer to the parent window.

 @param Caption Optionally points to the initial caption to display on the top
        of the multiline edit control.  If not supplied, no caption is
        displayed.

 @param Size Specifies the location and size of the multiline edit.

 @param Style Specifies style flags for the multiline edit.

 @return Pointer to the newly created control or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinMultilineEditCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in_opt PYORI_STRING Caption,
    __in PSMALL_RECT Size,
    __in DWORD Style
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_WINDOW Parent;
    SMALL_RECT ScrollBarRect;

    Parent = (PYORI_WIN_WINDOW)ParentHandle;

    MultilineEdit = YoriLibReferencedMalloc(sizeof(YORI_WIN_CTRL_MULTILINE_EDIT));
    if (MultilineEdit == NULL) {
        return NULL;
    }

    ZeroMemory(MultilineEdit, sizeof(YORI_WIN_CTRL_MULTILINE_EDIT));

    MultilineEdit->Ctrl.NotifyEventFn = YoriWinMultilineEditEventHandler;
    if (!YoriWinCreateControl(Parent, Size, TRUE, &MultilineEdit->Ctrl)) {
        YoriLibDereference(MultilineEdit);
        return NULL;
    }

    if (Caption != NULL && Caption->LengthInChars > 0) {
        if (!YoriLibAllocateString(&MultilineEdit->Caption, Caption->LengthInChars)) {
            YoriWinDestroyControl(&MultilineEdit->Ctrl);
            YoriLibDereference(MultilineEdit);
            return NULL;
        }

        memcpy(MultilineEdit->Caption.StartOfString, Caption->StartOfString, Caption->LengthInChars * sizeof(TCHAR));
        MultilineEdit->Caption.LengthInChars = Caption->LengthInChars;
    }

    if (Style & YORI_WIN_MULTILINE_EDIT_STYLE_VSCROLLBAR) {

        ScrollBarRect.Left = (SHORT)(MultilineEdit->Ctrl.FullRect.Right - MultilineEdit->Ctrl.FullRect.Left);
        ScrollBarRect.Right = ScrollBarRect.Left;
        ScrollBarRect.Top = 1;
        ScrollBarRect.Bottom = (SHORT)(MultilineEdit->Ctrl.FullRect.Bottom - MultilineEdit->Ctrl.FullRect.Top - 1);
        MultilineEdit->VScrollCtrl = YoriWinScrollBarCreate(&MultilineEdit->Ctrl, &ScrollBarRect, 0, YoriWinMultilineEditNotifyScrollChange);
    }

    // MSFIX: ReadOnly style
    // MSFIX: Text alignment
    // MSFIX: Optional border styles

    MultilineEdit->Ctrl.ClientRect.Top++;
    MultilineEdit->Ctrl.ClientRect.Left++;
    MultilineEdit->Ctrl.ClientRect.Bottom--;
    MultilineEdit->Ctrl.ClientRect.Right--;

    MultilineEdit->InsertMode = TRUE;
    MultilineEdit->TextAttributes = MultilineEdit->Ctrl.DefaultAttributes;

    MultilineEdit->TabWidth = 4;

    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, 0, (DWORD)-1);
    YoriWinMultilineEditPaintNonClient(MultilineEdit);
    YoriWinMultilineEditPaint(MultilineEdit);

    return &MultilineEdit->Ctrl;
}


// vim:sw=4:ts=4:et:
