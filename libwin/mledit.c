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
 A set of modification operations that can be performed on the buffer that
 can be undone.
 */
typedef enum _YORI_WIN_CTRL_MULTILINE_EDIT_UNDO_OPERATION {
    YoriWinMultilineEditUndoInsertText = 0,
    YoriWinMultilineEditUndoOverwriteText = 1,
    YoriWinMultilineEditUndoDeleteText = 2
} YORI_WIN_CTRL_MULTILINE_EDIT_UNDO_OPERATION;

/**
 Information about a single operation to undo.
 */
typedef struct _YORI_WIN_CTRL_MULTILINE_EDIT_UNDO {

    /**
     The list of operations that can be undone on the multiline edit control.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The type of this operation.
     */
    YORI_WIN_CTRL_MULTILINE_EDIT_UNDO_OPERATION Op;

    /**
     Information specific to each type of operation.
     */
    union {
        struct {

            /**
             The first line of the range that was inserted and should be
             deleted on undo.
             */
            DWORD FirstLineToDelete;

            /**
             The first offset of the range that was inserted and should be
             deleted on undo.
             */
            DWORD FirstCharOffsetToDelete;

            /**
             The last line of the range that was inserted and should be
             deleted on undo.
             */
            DWORD LastLineToDelete;

            /**
             The last offset of the range that was inserted and should be
             deleted on undo.
             */
            DWORD LastCharOffsetToDelete;
        } InsertText;

        struct {

            /**
             The first line of the range that was deleted and needs to be
             reinserted.
             */
            DWORD FirstLine;

            /**
             The first character of the range that was deleted and needs to be
             reinserted.
             */
            DWORD FirstCharOffset;

            /**
             The text to reinsert on undo.
             */
            YORI_STRING Text;
        } DeleteText;

        struct {
            /**
             The first line of the range that was overwritten and should be
             deleted on undo.
             */
            DWORD FirstLineToDelete;

            /**
             The first offset of the range that was overwritten and should be
             deleted on undo.
             */
            DWORD FirstCharOffsetToDelete;

            /**
             The last line of the range that was overwritten and should be
             deleted on undo.
             */
            DWORD LastLineToDelete;

            /**
             The last offset of the range that was overwritten and should be
             deleted on undo.
             */
            DWORD LastCharOffsetToDelete;

            /**
             The first line of the range that should be inserted to replace
             the overwritten text.
             */
            DWORD FirstLine;

            /**
             The first character of the range that should be inserted to replace
             the overwritten text.
             */
            DWORD FirstCharOffset;

            /**
             The offset of the first character that the user changed.  This
             must be on the same line as FirstLine but may be after
             FirstCharOffset because the saved range may be larger than the
             range that the user modified.  This value is used to determine
             the cursor location on undo.
             */
            DWORD FirstCharOffsetModified;

            /**
             The offset of the last character that the user changed.  This
             must be on the same line as LastLineToDelete but may be before
             LastCharOffsetToDelete because the saved range may be larger than
             the range that the user modified.  This value is used to
             determine if a later modification should be part of an earlier
             undo record.
             */
            DWORD LastCharOffsetModified;

            /**
             The text to reinsert on undo.
             */
            YORI_STRING Text;
        } OverwriteText;
    } u;
} YORI_WIN_CTRL_MULTILINE_EDIT_UNDO, *PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO;

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
    PYORI_WIN_NOTIFY_MULTILINE_EDIT_CURSOR_MOVE CursorMoveCallback;

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
     A stack of changes which can be undone.
     */
    YORI_LIST_ENTRY Undo;

    /**
     A stack of changes which can be redone.
     */
    YORI_LIST_ENTRY Redo;

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
     The desired horizontal offset from the beginning of the display.  This
     can be greater than the actual DisplayCursorOffset above if the user is
     navigating up or down, and the current line is shorter than the offset
     of the cursor when the user started navigating.  A special value of -1
     is used to indicate that this value is not populated, because navigation
     is not currently occurring.
     */
    DWORD DesiredDisplayCursorOffset;

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
     If TRUE, the previous edit has started a new line which has auto indent
     applied.  When this occurs, backspace should remove an entire indent,
     not just a character.  Any modification or cursor movement should set
     this to FALSE, with the frustrating exception of backspace itself,
     which leaves this mode in effect.
     */
    BOOLEAN AutoIndentApplied;

    /**
     When AutoIndentApplied is TRUE, specifies the line used to supply the
     auto indent.  When backspace is pressed, earlier lines are examined to
     find the previous indent.
     */
    DWORD AutoIndentSourceLine;

    /**
     When AutoIndentApplied is TRUE, specifies the number of characters to
     obtain from the AutoIndentSourceLine.  This can be less than the total
     number of indentation characters if the lines contain different
     white space characters (eg. if the first line contains a space and tab,
     and a later line contains two spaces, the first space is considered a
     match but the tab is not.
     */
    DWORD AutoIndentSourceLength;

    /**
     When AutoIndentApplied is TRUE, specifies the line that has an auto
     indent applied.  This is used to detect cursor movement away from the
     line and reset auto indent state.
     */
    DWORD AutoIndentAppliedLine;

    /**
     Records the last observed mouse location when a mouse selection is
     active.  This is repeatedly used via a timer when the mouse moves off
     the control area.  Once the mouse returns to the control area or the
     button is released (completing the selection) this value is undefined.
     */
    YORI_WIN_BOUNDED_COORD LastMousePos;

    /**
     A timer that is used to indicate the previous mouse position should be
     repeated to facilitate scroll.  This can be NULL if auto scroll is not
     in effect.
     */
    PYORI_WIN_CTRL_HANDLE Timer;

    /**
     When inputting a character by value, the current value that has been
     accumulated (since this requires multiple key events.)
     */
    DWORD NumericKeyValue;

    /**
     Indicates how to interpret the NumericKeyValue.  Ascii uses CP_OEMCP,
     Ansi uses CP_ACP, Unicode is direct.  Also note that Unicode takes
     input in hexadecimal to match the normal U+xxxx specification.
     */
    YORI_LIB_NUMERIC_KEY_TYPE NumericKeyType;

    /**
     The attributes to display text in.
     */
    WORD TextAttributes;

    /**
     The attributes to display selected text in.
     */
    WORD SelectedAttributes;

    /**
     The attributes to display the caption in.
     */
    WORD CaptionAttributes;

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

    /**
     TRUE if the multiline edit control is following traditional MS-DOS edit
     navigation rules, FALSE if following more modern multiline edit
     navigation rules.  In the traditional model, the cursor can move
     infinitely right of the text in any line, so the cursor's line does not
     change in response to left and right keys.
     */
    BOOLEAN TraditionalEditNavigation;

    /**
     TRUE if new lines should start with leading whitespace characters from
     previous lines.  FALSE if new lines should start at offset zero.
     */
    BOOLEAN AutoIndent;

} YORI_WIN_CTRL_MULTILINE_EDIT, *PYORI_WIN_CTRL_MULTILINE_EDIT;

//
//  =========================================
//  DISPLAY FUNCTIONS
//  =========================================
//

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
    DWORD DesiredCursorChar;
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

    //
    //  In a modern control, the cursor can't be beyond the end of the text,
    //  so cap it here.
    //

    DesiredCursorChar = DisplayChar - (CurrentDisplayIndex - CharIndex);
    if (!MultilineEdit->TraditionalEditNavigation) {
        if (DesiredCursorChar > Line->LengthInChars) {
            DesiredCursorChar = Line->LengthInChars;
        }
    }

    *CursorChar = DesiredCursorChar;
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
 If one is not already defined, define the desired display offset, which is
 the display column that would ideally be returned to as the cursor moves up
 or down lines.  This may already be defined, if the user is navigating up or
 down multiple times.

 @param MultilineEdit Pointer to the multiline edit control.
 */
VOID
YoriWinMultilineEditPopulateDesiredDisplayOffset(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    if (MultilineEdit->DesiredDisplayCursorOffset == (DWORD)-1) {
        MultilineEdit->DesiredDisplayCursorOffset = MultilineEdit->DisplayCursorOffset;
    }
}

/**
 Indicate that the user has performed an operation that is not navigating up
 or down, meaning that any desired offset should be cleared.

 @param MultilineEdit Pointer to the multiline edit control.
 */
VOID
YoriWinMultilineEditClearDesiredDisplayOffset(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    MultilineEdit->DesiredDisplayCursorOffset = (DWORD)-1;
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
        for (ColumnIndex = 0; ColumnIndex < CaptionCharsToDisplay; ColumnIndex++) {
            YoriWinSetControlNonClientCell(&MultilineEdit->Ctrl, (WORD)(ColumnIndex + StartOffset), 0, MultilineEdit->Caption.StartOfString[ColumnIndex], MultilineEdit->CaptionAttributes);
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
    TCHAR Char;

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
            TextAttributes = MultilineEdit->SelectedAttributes;
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
                        TextAttributes = MultilineEdit->SelectedAttributes;
                    }
                } else if (LineIndex == MultilineEdit->Selection.FirstLine) {
                    TextAttributes = WindowAttributes;
                    if (ColumnIndex + MultilineEdit->ViewportLeft >= DisplayFirstCharOffset) {
                        TextAttributes = MultilineEdit->SelectedAttributes;
                    }
                } else if (LineIndex == MultilineEdit->Selection.LastLine) {
                    TextAttributes = WindowAttributes;
                    if (ColumnIndex + MultilineEdit->ViewportLeft < DisplayLastCharOffset) {
                        TextAttributes = MultilineEdit->SelectedAttributes;
                    }
                }
            }

            Char = Line.StartOfString[ColumnIndex + MultilineEdit->ViewportLeft];

            //
            //  Nano server interprets NULL as "leave previous contents alone"
            //  which is hazardous for an editor.
            //

            if (Char == 0 && YoriLibIsNanoServer()) {
                Char = ' ';
            }

            YoriWinSetControlClientCell(&MultilineEdit->Ctrl, ColumnIndex, RowIndex, Char, TextAttributes);
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

    if (MultilineEdit->AutoIndentApplied) {
        if (NewCursorLine != MultilineEdit->AutoIndentAppliedLine ||
            NewCursorOffset != MultilineEdit->AutoIndentSourceLength) {

            MultilineEdit->AutoIndentApplied = FALSE;
        }
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

//
//  =========================================
//  UNDO FUNCTIONS
//  =========================================
//

/**
 Free a single undo entry.  This entry is expected to be unlinked from the
 chain.

 @param Undo Pointer to the undo entry to free.
 */
VOID
YoriWinMultilineEditFreeSingleUndo(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo
    )
{
    switch(Undo->Op) {
        case YoriWinMultilineEditUndoOverwriteText:
            YoriLibFreeStringContents(&Undo->u.OverwriteText.Text);
            break;
        case YoriWinMultilineEditUndoDeleteText:
            YoriLibFreeStringContents(&Undo->u.DeleteText.Text);
            break;
    }

    YoriLibFree(Undo);
}

/**
 Free all undo entries that are linked into the multiline edit control.

 @param MultilineEdit Pointer to the multiline edit control.
 */
VOID
YoriWinMultilineEditClearUndo(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    PYORI_LIST_ENTRY ListHead;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo;

    ListHead = &MultilineEdit->Undo;
    while (ListHead != NULL) {

        ListEntry = YoriLibGetNextListEntry(ListHead, NULL);
        while (ListEntry != NULL) {
            YoriLibRemoveListItem(ListEntry);
            Undo = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL_MULTILINE_EDIT_UNDO, ListEntry);
            YoriWinMultilineEditFreeSingleUndo(Undo);
            ListEntry = YoriLibGetNextListEntry(ListHead, NULL);
        }

        if (ListHead == &MultilineEdit->Undo) {
            ListHead = &MultilineEdit->Redo;
        } else {
            ListHead = NULL;
        }
    }
}

/**
 Free all redo entries that are linked into the multiline edit control.

 @param MultilineEdit Pointer to the multiline edit control.
 */
VOID
YoriWinMultilineEditClearRedo(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo;

    ListEntry = YoriLibGetNextListEntry(&MultilineEdit->Redo, NULL);
    while (ListEntry != NULL) {
        YoriLibRemoveListItem(ListEntry);
        Undo = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL_MULTILINE_EDIT_UNDO, ListEntry);
        YoriWinMultilineEditFreeSingleUndo(Undo);
        ListEntry = YoriLibGetNextListEntry(&MultilineEdit->Redo, NULL);
    }
}

/**
 Check if a new modification should be included in a previous undo entry
 because the new modification is immediately before the range in the previous
 entry.

 @param MultilineEdit Pointer to the multiline edit control.

 @param ExistingFirstLine Specifies the beginning line of the range currently
        covered by an undo record.

 @param ExistingFirstCharOffset Specifies the beginning offset of the range
        currently covered by an undo record.

 @param ProposedLastLine Specifies the ending line of a newly modified range.

 @param ProposedLastCharOffset Specifies the ending offset of a newly modified
        range.

 @return TRUE to indicate that the new change is immediately before the
         previous undo record.  FALSE to indicate it requires a new entry.
 */
BOOLEAN
YoriWinMultilineEditRangeImmediatelyPreceeds(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD ExistingFirstLine,
    __in DWORD ExistingFirstCharOffset,
    __in DWORD ProposedLastLine,
    __in DWORD ProposedLastCharOffset
    )
{
    UNREFERENCED_PARAMETER(MultilineEdit);

    if (ExistingFirstLine == ProposedLastLine &&
        ExistingFirstCharOffset == ProposedLastCharOffset) {

        return TRUE;
    }

    return FALSE;
}

/**
 Check if a new modification should be included in a previous undo entry
 because the new modification is immediately after the range in the previous
 entry.

 @param MultilineEdit Pointer to the multiline edit control.

 @param ExistingLastLine Specifies the ending line of the range currently
        covered by an undo record.

 @param ExistingLastCharOffset Specifies the ending offset of the range
        currently covered by an undo record.

 @param ProposedFirstLine Specifies the beginning line of a newly modified
        range.

 @param ProposedFirstCharOffset Specifies the beginning offset of a newly
        modified range.

 @return TRUE to indicate that the new change is immediately after the
         previous undo record.  FALSE to indicate it requires a new entry.
 */
BOOLEAN
YoriWinMultilineEditRangeImmediatelyFollows(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD ExistingLastLine,
    __in DWORD ExistingLastCharOffset,
    __in DWORD ProposedFirstLine,
    __in DWORD ProposedFirstCharOffset
    )
{
    UNREFERENCED_PARAMETER(MultilineEdit);

    if (ExistingLastLine == ProposedFirstLine &&
        ExistingLastCharOffset == ProposedFirstCharOffset) {

        return TRUE;
    }

    return FALSE;
}

/**
 Return an undo record for the incoming operation.  This may be a newly
 allocated undo record, or if the operation is adjacent to the previous
 operation it may return an existing record.

 @param MultilineEdit Pointer to the multiline edit control.

 @param Op Specifies the type of the operation.  Only the same type of
        operations can reuse previous records.

 @param FirstLine Specifies the beginning line of the range that is being
        modified.

 @param FirstCharOffset Specifies the beginning offset of the range that is
        being modified.

 @param LastLine Specifies the last line of the range that is being modified.
        This may not always be known until after the operation is performed,
        but in that case the operation cannot be prepended to a previous
        operation of the same type, so it is not required.

 @param LastCharOffset Specifies the last offset of the range that is being
        modified.  This may not always be known until after the operation is
        performed, but in that case the operation cannot be prepended to a
        previous operation of the same type, so it is not required.

 @param NewRangeBeforeExistingRange On successful completion, set to TRUE
        to indicate the new change is being applied to an existing record
        before the existing record's current range.  FALSE implies the
        change is either after the end of an existing record or is going to
        a new record.

 @return Pointer to the undo record, or NULL to indicate failure.
 */
PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO
YoriWinMultilineEditGetUndoRecordForOperation(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in YORI_WIN_CTRL_MULTILINE_EDIT_UNDO_OPERATION Op,
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in DWORD LastLine,
    __in DWORD LastCharOffset,
    __out PBOOLEAN NewRangeBeforeExistingRange
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo = NULL;

    YoriWinMultilineEditClearRedo(MultilineEdit);

    *NewRangeBeforeExistingRange = FALSE;

    ListEntry = YoriLibGetNextListEntry(&MultilineEdit->Undo, NULL);
    if (ListEntry != NULL) {
        Undo = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL_MULTILINE_EDIT_UNDO, ListEntry);
        if (Undo->Op != Op) {
            Undo = NULL;
        } else {
            switch (Op) {
                case YoriWinMultilineEditUndoInsertText:
                    if (!YoriWinMultilineEditRangeImmediatelyFollows(MultilineEdit, Undo->u.InsertText.LastLineToDelete, Undo->u.InsertText.LastCharOffsetToDelete, FirstLine, FirstCharOffset)) {
                        Undo = NULL;
                    }
                    break;
                case YoriWinMultilineEditUndoDeleteText:
                    if (YoriWinMultilineEditRangeImmediatelyPreceeds(MultilineEdit, Undo->u.DeleteText.FirstLine, Undo->u.DeleteText.FirstCharOffset, LastLine, LastCharOffset)) {
                        *NewRangeBeforeExistingRange = TRUE;
                    } else if (!YoriWinMultilineEditRangeImmediatelyFollows(MultilineEdit, Undo->u.DeleteText.FirstLine, Undo->u.DeleteText.FirstCharOffset, FirstLine, FirstCharOffset)) {
                        Undo = NULL;
                    }
                    break;
                case YoriWinMultilineEditUndoOverwriteText:
                    if (!YoriWinMultilineEditRangeImmediatelyFollows(MultilineEdit, Undo->u.OverwriteText.LastLineToDelete, Undo->u.OverwriteText.LastCharOffsetModified, FirstLine, FirstCharOffset)) {
                        Undo = NULL;
                    }
                    break;
                default:
                    Undo = NULL;
                    break;
            }
        }
    }

    if (Undo == NULL) {
        Undo = YoriLibMalloc(sizeof(YORI_WIN_CTRL_MULTILINE_EDIT_UNDO));
        if (Undo == NULL) {
            YoriWinMultilineEditClearUndo(MultilineEdit);
            return NULL;
        }

        ZeroMemory(Undo, sizeof(YORI_WIN_CTRL_MULTILINE_EDIT_UNDO));
        YoriLibInsertList(&MultilineEdit->Undo, &Undo->ListEntry);

        Undo->Op = Op;

        switch(Op) {
            case YoriWinMultilineEditUndoInsertText:
                Undo->u.InsertText.FirstLineToDelete = FirstLine;
                Undo->u.InsertText.FirstCharOffsetToDelete = FirstCharOffset;
                Undo->u.InsertText.LastLineToDelete = LastLine;
                Undo->u.InsertText.LastCharOffsetToDelete = LastCharOffset;
                break;
            case YoriWinMultilineEditUndoDeleteText:
                Undo->u.DeleteText.FirstLine = FirstLine;
                Undo->u.DeleteText.FirstCharOffset = FirstCharOffset;
                YoriLibInitEmptyString(&Undo->u.DeleteText.Text);
                break;
            case YoriWinMultilineEditUndoOverwriteText:
                Undo->u.OverwriteText.FirstLineToDelete = FirstLine;
                Undo->u.OverwriteText.FirstCharOffsetToDelete = FirstCharOffset;
                Undo->u.OverwriteText.LastLineToDelete = LastLine;
                Undo->u.OverwriteText.LastCharOffsetToDelete = LastCharOffset;
                Undo->u.OverwriteText.FirstLine = FirstLine;
                Undo->u.OverwriteText.FirstCharOffset = FirstCharOffset;
                Undo->u.OverwriteText.FirstCharOffsetModified = FirstCharOffset;
                Undo->u.OverwriteText.LastCharOffsetModified = LastCharOffset;
                YoriLibInitEmptyString(&Undo->u.OverwriteText.Text);
                break;
        }
    }
    return Undo;
}

/**
 If a change needs to be saved so that it can be undone, the change may be
 before or after a previous change that should be undone in the same
 operation (consider when the user hits backspace or del.)  In order to do
 this, new text may need to be saved before or after previously saved text.
 Here a string is allocated where the range used is in the middle of the
 allocation, allowing characters to be inserted before or after it by
 adjusting the start pointer and length of the string.  Clearly if it is
 continually modified, it may also need to be reallocated periodically, but
 not for each key press.

 @param CombinedString Pointer to a string which contains the current saved
        text.  The StartOfString and LengthInChars members specify the current
        saved text, and the gap before can be found from the difference
        between MemoryToFree and StartOfString, and the gap afterwards from
        LengthAllocated and LengthInChars.

 @param CharsNeeded Specifies the number of new characters that should be
        added.

 @param CharsBefore TRUE if the new characters should be added before existing
        text, FALSE if the new characters should be added after the existing
        text.

 @param Substring On successful completion, populated with a string for the
        caller to write their new changes in the correct place.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMultilineEditEnsureSpaceBeforeOrAfterString(
    __in PYORI_STRING CombinedString,
    __in DWORD CharsNeeded,
    __in BOOLEAN CharsBefore,
    __out PYORI_STRING Substring
    )
{
    DWORD CurrentCharsBefore;
    DWORD CurrentCharsAfter;
    DWORD LengthNeeded;
    YORI_STRING Temp;

    CurrentCharsBefore = (DWORD)(CombinedString->StartOfString - (LPTSTR)CombinedString->MemoryToFree);
    CurrentCharsAfter = CombinedString->LengthAllocated - CurrentCharsBefore - CombinedString->LengthInChars;

    while(TRUE) {

        if (CharsBefore) {
            if (CharsNeeded <= CurrentCharsBefore) {
                CombinedString->StartOfString = CombinedString->StartOfString - CharsNeeded;
                CombinedString->LengthInChars = CombinedString->LengthInChars + CharsNeeded;
                YoriLibInitEmptyString(Substring);
                Substring->StartOfString = CombinedString->StartOfString;
                Substring->LengthInChars = CharsNeeded;
                return TRUE;
            }
        } else {
            if (CharsNeeded <= CurrentCharsAfter) {
                YoriLibInitEmptyString(Substring);
                Substring->StartOfString = CombinedString->StartOfString + CombinedString->LengthInChars;
                Substring->LengthInChars = CharsNeeded;
                CombinedString->LengthInChars = CombinedString->LengthInChars + CharsNeeded;
                return TRUE;
            }
        }

        //
        //  Allocate an extra 1Kb before and after in the hope that repeated
        //  keystrokes won't cause new allocations and copies.
        //

        CurrentCharsBefore = CurrentCharsAfter = 0x400;
        if (CharsBefore) {
            CurrentCharsBefore = CurrentCharsBefore + CharsNeeded;
        } else {
            CurrentCharsAfter = CurrentCharsAfter + CharsNeeded;
        }

        LengthNeeded = CurrentCharsBefore + CombinedString->LengthInChars + CurrentCharsAfter;
        if (!YoriLibAllocateString(&Temp, LengthNeeded)) {
            return FALSE;
        }

        Temp.StartOfString = Temp.StartOfString + CurrentCharsBefore;

        memcpy(Temp.StartOfString,
               CombinedString->StartOfString,
               CombinedString->LengthInChars * sizeof(TCHAR));

        Temp.LengthInChars = CombinedString->LengthInChars;
        YoriLibFreeStringContents(CombinedString);
        memcpy(CombinedString, &Temp, sizeof(YORI_STRING));
    }
}

/**
 Return TRUE to indicate that there are records specifying how to undo
 previous operations.

 @param CtrlHandle Pointer to the multiline edit control.

 @return TRUE if there are operations available to undo, FALSE if there
         are not.
 */
BOOLEAN
YoriWinMultilineEditIsUndoAvailable(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (!YoriLibIsListEmpty(&MultilineEdit->Undo)) {
        return TRUE;
    }

    return FALSE;
}

/**
 Return TRUE to indicate that there are records specifying how to redo
 previous operations.

 @param CtrlHandle Pointer to the multiline edit control.

 @return TRUE if there are operations available to redo, FALSE if there
         are not.
 */
BOOLEAN
YoriWinMultilineEditIsRedoAvailable(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (!YoriLibIsListEmpty(&MultilineEdit->Redo)) {
        return TRUE;
    }

    return FALSE;
}

__success(return)
BOOLEAN
YoriWinMultilineEditGetTextRange(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in DWORD LastLine,
    __in DWORD LastCharOffset,
    __in PYORI_STRING NewlineString,
    __out PYORI_STRING SelectedText
    );

__success(return)
BOOLEAN
YoriWinMultilineEditDeleteTextRange(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in BOOLEAN ProcessingBackspace,
    __in BOOLEAN ProcessingUndo,
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in DWORD LastLine,
    __in DWORD LastCharOffset
    );

__success(return)
BOOLEAN
YoriWinMultilineEditInsertTextRange(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in BOOLEAN ProcessingUndo,
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in PYORI_STRING Text,
    __out PDWORD LastLine,
    __out PDWORD LastCharOffset
    );

VOID
YoriWinMultilineEditCalculateEndingPointOfText(
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in PYORI_STRING Text,
    __out PDWORD LastLine,
    __out PDWORD LastCharOffset
    );

/**
 Given an undo record, generate a record that would undo the undo.

 @param MultilineEdit Pointer to the multiline edit control containing the
        state of the buffer before the undo record has been applied.

 @param Undo Pointer to an undo record indicating changes to perform.

 @param AddToUndoList If FALSE, the resulting undo of the undo should be added
        to the Redo list.  If TRUE, the undo here is already a redo, so the
        undo of the undo (of the undo) goes onto the undo list.

 @return Pointer to a newly allocated undo of the undo record.
 */
PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO
YoriWinMultilineEditGenerateRedoRecordForUndo(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo,
    __in BOOLEAN AddToUndoList
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Redo;
    YORI_STRING Newline;

    //
    //  Note that clearing all undo may remove the undo record that is
    //  causing this redo.  This implies that when this call fails, the
    //  caller cannot continue using the undo record.
    //

    Redo = YoriLibMalloc(sizeof(YORI_WIN_CTRL_MULTILINE_EDIT_UNDO));
    if (Redo == NULL) {
        YoriWinMultilineEditClearUndo(MultilineEdit);
        return NULL;
    }

    ZeroMemory(Redo, sizeof(YORI_WIN_CTRL_MULTILINE_EDIT_UNDO));
    if (AddToUndoList) {
        YoriLibInsertList(&MultilineEdit->Undo, &Redo->ListEntry);
    } else {
        YoriLibInsertList(&MultilineEdit->Redo, &Redo->ListEntry);
    }

    switch(Undo->Op) {
        case YoriWinMultilineEditUndoInsertText:
            Redo->Op = YoriWinMultilineEditUndoDeleteText;
            Redo->u.DeleteText.FirstLine = Undo->u.InsertText.FirstLineToDelete;
            Redo->u.DeleteText.FirstCharOffset = Undo->u.InsertText.FirstCharOffsetToDelete;
            YoriLibConstantString(&Newline, _T("\n"));
            if (!YoriWinMultilineEditGetTextRange(MultilineEdit,
                                                  Undo->u.InsertText.FirstLineToDelete,
                                                  Undo->u.InsertText.FirstCharOffsetToDelete,
                                                  Undo->u.InsertText.LastLineToDelete,
                                                  Undo->u.InsertText.LastCharOffsetToDelete,
                                                  &Newline,
                                                  &Redo->u.DeleteText.Text)) {
                YoriWinMultilineEditClearUndo(MultilineEdit);
                return NULL;
            }

            break;
        case YoriWinMultilineEditUndoDeleteText:
            Redo->Op = YoriWinMultilineEditUndoInsertText;
            Redo->u.InsertText.FirstLineToDelete = Undo->u.DeleteText.FirstLine;
            Redo->u.InsertText.FirstCharOffsetToDelete = Undo->u.DeleteText.FirstCharOffset;
            YoriWinMultilineEditCalculateEndingPointOfText(Undo->u.DeleteText.FirstLine,
                                                           Undo->u.DeleteText.FirstCharOffset,
                                                           &Undo->u.DeleteText.Text,
                                                           &Redo->u.InsertText.LastLineToDelete,
                                                           &Redo->u.InsertText.LastCharOffsetToDelete);
            break;
        case YoriWinMultilineEditUndoOverwriteText:

            Redo->Op = Undo->Op;
            Redo->u.OverwriteText.FirstLineToDelete = Undo->u.OverwriteText.FirstLine;
            Redo->u.OverwriteText.FirstCharOffsetToDelete = Undo->u.OverwriteText.FirstCharOffset;
            YoriWinMultilineEditCalculateEndingPointOfText(Undo->u.OverwriteText.FirstLine,
                                                           Undo->u.OverwriteText.FirstCharOffset,
                                                           &Undo->u.OverwriteText.Text,
                                                           &Redo->u.OverwriteText.LastLineToDelete,
                                                           &Redo->u.OverwriteText.LastCharOffsetToDelete);
            Redo->u.OverwriteText.FirstLine = Undo->u.OverwriteText.FirstLineToDelete;
            Redo->u.OverwriteText.FirstCharOffset = Undo->u.OverwriteText.FirstCharOffsetToDelete;

            YoriLibConstantString(&Newline, _T("\n"));
            if (!YoriWinMultilineEditGetTextRange(MultilineEdit,
                                                  Undo->u.OverwriteText.FirstLineToDelete,
                                                  Undo->u.OverwriteText.FirstCharOffsetToDelete,
                                                  Undo->u.OverwriteText.LastLineToDelete,
                                                  Undo->u.OverwriteText.LastCharOffsetToDelete,
                                                  &Newline,
                                                  &Redo->u.OverwriteText.Text)) {
                YoriWinMultilineEditClearUndo(MultilineEdit);
                return NULL;
            }

            Redo->u.OverwriteText.FirstCharOffsetModified = Undo->u.OverwriteText.FirstCharOffsetModified;
            Redo->u.OverwriteText.LastCharOffsetModified = Undo->u.OverwriteText.FirstCharOffsetModified;

            break;
    }

    return Redo;
}

/**
 Modify the buffer of the control per the direction of an undo record.

 @param MultilineEdit Pointer to the multiline edit control indicating the
        buffer and cursor position.

 @param Undo Pointer to the undo record indicating the changes to perform.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditApplyUndoRecord(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo
    )
{
    BOOLEAN Success;
    DWORD NewLastLine;
    DWORD NewLastCharOffset;

    Success = FALSE;
    switch(Undo->Op) {
        case YoriWinMultilineEditUndoInsertText:
            Success = YoriWinMultilineEditDeleteTextRange(MultilineEdit, FALSE, TRUE, Undo->u.InsertText.FirstLineToDelete, Undo->u.InsertText.FirstCharOffsetToDelete, Undo->u.InsertText.LastLineToDelete, Undo->u.InsertText.LastCharOffsetToDelete);
            if (Success) {
                YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, Undo->u.InsertText.FirstCharOffsetToDelete, Undo->u.InsertText.FirstLineToDelete);
            }
            break;
        case YoriWinMultilineEditUndoDeleteText:
            Success = YoriWinMultilineEditInsertTextRange(MultilineEdit, TRUE, Undo->u.DeleteText.FirstLine, Undo->u.DeleteText.FirstCharOffset, &Undo->u.DeleteText.Text, &NewLastLine, &NewLastCharOffset);
            if (Success) {
                YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewLastCharOffset, NewLastLine);
            }
            break;
        case YoriWinMultilineEditUndoOverwriteText:
            NewLastLine = 0;
            NewLastCharOffset = 0;
            Success = YoriWinMultilineEditDeleteTextRange(MultilineEdit, FALSE, TRUE, Undo->u.OverwriteText.FirstLineToDelete, Undo->u.OverwriteText.FirstCharOffsetToDelete, Undo->u.OverwriteText.LastLineToDelete, Undo->u.OverwriteText.LastCharOffsetToDelete);
            if (Success) {
                Success = YoriWinMultilineEditInsertTextRange(MultilineEdit, TRUE, Undo->u.OverwriteText.FirstLine, Undo->u.OverwriteText.FirstCharOffset, &Undo->u.OverwriteText.Text, &NewLastLine, &NewLastCharOffset);
            }
            if (Success) {
                YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, Undo->u.OverwriteText.FirstCharOffsetModified, Undo->u.OverwriteText.FirstLine);
            }
            break;
    }

    return Success;
}

/**
 Undo the most recent change to a multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditUndo(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo = NULL;
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Redo;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;
    BOOLEAN Success;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (YoriLibIsListEmpty(&MultilineEdit->Undo)) {
        return FALSE;
    }

    Undo = CONTAINING_RECORD(MultilineEdit->Undo.Next, YORI_WIN_CTRL_MULTILINE_EDIT_UNDO, ListEntry);

    Redo = YoriWinMultilineEditGenerateRedoRecordForUndo(MultilineEdit, Undo, FALSE);
    if (Redo == NULL) {
        return FALSE;
    }

    Success = YoriWinMultilineEditApplyUndoRecord(MultilineEdit, Undo);

    if (Success) {
        YoriLibRemoveListItem(&Undo->ListEntry);
        YoriWinMultilineEditFreeSingleUndo(Undo);
    } else {
        YoriLibRemoveListItem(&Redo->ListEntry);
        YoriWinMultilineEditFreeSingleUndo(Redo);
    }

    return Success;
}

/**
 Redo the most recently undone change to a multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditRedo(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo = NULL;
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Redo;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (YoriLibIsListEmpty(&MultilineEdit->Redo)) {
        return FALSE;
    }

    Undo = CONTAINING_RECORD(MultilineEdit->Redo.Next, YORI_WIN_CTRL_MULTILINE_EDIT_UNDO, ListEntry);

    Redo = YoriWinMultilineEditGenerateRedoRecordForUndo(MultilineEdit, Undo, TRUE);
    if (Redo == NULL) {
        return FALSE;
    }

    if (!YoriWinMultilineEditApplyUndoRecord(MultilineEdit, Undo)) {
        YoriLibRemoveListItem(&Redo->ListEntry);
        YoriWinMultilineEditFreeSingleUndo(Redo);
        return FALSE;
    }

    YoriLibRemoveListItem(&Undo->ListEntry);
    YoriWinMultilineEditFreeSingleUndo(Undo);

    return TRUE;
}

//
//  =========================================
//  BUFFER MANIPULATION FUNCTIONS
//  =========================================
//

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
 Find the length in characters needed to store a single continuous string
 covering the specified range in a multiline edit control.

 @param MultilineEdit Pointer to the multiline edit control containing the
        contents of the buffer.

 @param FirstLine Specifies the line that contains the first character to
        return.

 @param FirstCharOffset Specifies the offset within FirstLine of the first
        character to return.

 @param LastLine Specifies the line that contains the last character to
        return.

 @param LastCharOffset Specifies the offset beyond the last character to
        return.

 @param NewlineLength Specifies the number of characters in each newline.

 @return The number of characters needed to contain the requested range.
 */
DWORD
YoriWinMultilineEditGetTextRangeLength(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in DWORD LastLine,
    __in DWORD LastCharOffset,
    __in DWORD NewlineLength
    )
{
    DWORD CharsInRange;
    DWORD LinesInRange;
    DWORD LineIndex;

    if (FirstLine == LastLine) {
        CharsInRange = LastCharOffset - FirstCharOffset;
    } else {
        LinesInRange = LastLine - FirstLine;
        CharsInRange = MultilineEdit->LineArray[FirstLine].LengthInChars - FirstCharOffset;
        for (LineIndex = FirstLine + 1; LineIndex < LastLine; LineIndex++) {
            CharsInRange += MultilineEdit->LineArray[LineIndex].LengthInChars;
        }
        CharsInRange += LastCharOffset;
        CharsInRange += LinesInRange * NewlineLength;
    }

    return CharsInRange;
}

/**
 Count the leading whitespace characters in a line and return a substring
 that can be used to apply an indentation to a later line.

 @param MultilineEdit Pointer to the multiline edit control containing the
        contents of the buffer.

 @param LineIndex Specifies the line that contains the indentation string
        to return.

 @param Indent On successful completion, updated to point to the substring
        within the line that consists of initial white space characters.
 */
VOID
YoriWinMultilineEditGetIndentationOnLine(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD LineIndex,
    __out PYORI_STRING Indent
    )
{
    PYORI_STRING Line;
    DWORD Index;

    YoriLibInitEmptyString(Indent);
    Line = &MultilineEdit->LineArray[LineIndex];
    Indent->StartOfString = Line->StartOfString;
    for (Index = 0; Index < Line->LengthInChars; Index++) {
        if (Line->StartOfString[Index] != ' ' &&
            Line->StartOfString[Index] != '\t') {

            break;
        }
    }
    Indent->LengthInChars = Index;
}

/**
 When an auto indent has been applied to a line and the backspace key is
 pressed, search backwards through previous lines to find one that contains
 less indentation than the current match, and return that line index along
 with the new indentation to apply.

 @param MultilineEdit Pointer to the multiline edit control containing the
        contents of the buffer.

 @param NewLine On successful completion, updated to indicate the line
        index containing the new indentation.

 @param NewIndent On successful completion, updated to point to a substring
        within the NewLine buffer containing the new indentation to apply.
        This should be a subset of the current indentation, up to zero
        characters.
 */
VOID
YoriWinMultilineEditFindPreviousIndentLine(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __out PDWORD NewLine,
    __out PYORI_STRING NewIndent
    )
{
    DWORD ProbeLine;
    YORI_STRING ProbeIndent;
    YORI_STRING CurrentIndent;
    DWORD MatchingLength;

    ASSERT(MultilineEdit->AutoIndentApplied);
    YoriWinMultilineEditGetIndentationOnLine(MultilineEdit, MultilineEdit->AutoIndentSourceLine, &CurrentIndent);
    ASSERT(MultilineEdit->AutoIndentSourceLength <= CurrentIndent.LengthInChars);
    CurrentIndent.LengthInChars = MultilineEdit->AutoIndentSourceLength;

    //
    //  Count backwards from one prior to the current auto indent line up
    //  to the first
    //

    for (ProbeLine = MultilineEdit->AutoIndentSourceLine; ProbeLine > 0; ProbeLine--) {
        YoriWinMultilineEditGetIndentationOnLine(MultilineEdit, ProbeLine - 1, &ProbeIndent);
        MatchingLength = YoriLibCountStringMatchingChars(&CurrentIndent, &ProbeIndent);
        if (MatchingLength < CurrentIndent.LengthInChars) {
            *NewLine = ProbeLine - 1;
            ProbeIndent.LengthInChars = MatchingLength;
            memcpy(NewIndent, &ProbeIndent, sizeof(YORI_STRING));
            return;
        }
    }

    *NewLine = 0;
    YoriLibInitEmptyString(NewIndent);
}

/**
 Build a single continuous string covering the specified range in a multiline
 edit control and store it in a preallocated allocation.

 @param MultilineEdit Pointer to the multiline edit control containing the
        contents of the buffer.

 @param FirstLine Specifies the line that contains the first character to
        return.

 @param FirstCharOffset Specifies the offset within FirstLine of the first
        character to return.

 @param LastLine Specifies the line that contains the last character to
        return.

 @param LastCharOffset Specifies the offset beyond the last character to
        return.

 @param NewlineString Specifies the string to use to delimit lines.  This
        allows this routine to return text with any arbitrary line ending.

 @param SelectedText On input, contains an allocated string that's large
        enough to contain the requested text.  On successful completion,
        populated with the selected text.  The caller is expected to free
        this buffer with @ref YoriLibFreeStringContents .
 */
VOID
YoriWinMultilineEditPopulateTextRange(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in DWORD LastLine,
    __in DWORD LastCharOffset,
    __in PYORI_STRING NewlineString,
    __inout PYORI_STRING SelectedText
    )
{
    DWORD CharsInRange;
    DWORD LineIndex;
    PYORI_STRING Line;
    LPTSTR Ptr;

    Line = &MultilineEdit->LineArray[FirstLine];

    if (FirstLine == LastLine) {
        CharsInRange = LastCharOffset - FirstCharOffset;
        memcpy(SelectedText->StartOfString, &Line->StartOfString[FirstCharOffset], CharsInRange * sizeof(TCHAR));
        SelectedText->LengthInChars = CharsInRange;
    } else {
        Ptr = SelectedText->StartOfString;
        memcpy(Ptr, &Line->StartOfString[FirstCharOffset], (Line->LengthInChars - FirstCharOffset) * sizeof(TCHAR));
        Ptr += (Line->LengthInChars - FirstCharOffset);
        for (LineIndex = FirstLine + 1; LineIndex < LastLine; LineIndex++) {
            memcpy(Ptr, NewlineString->StartOfString, NewlineString->LengthInChars * sizeof(TCHAR));
            Ptr += NewlineString->LengthInChars;
            memcpy(Ptr,
                   MultilineEdit->LineArray[LineIndex].StartOfString,
                   MultilineEdit->LineArray[LineIndex].LengthInChars * sizeof(TCHAR));
            Ptr += MultilineEdit->LineArray[LineIndex].LengthInChars;
        }
        memcpy(Ptr, NewlineString->StartOfString, NewlineString->LengthInChars * sizeof(TCHAR));
        Ptr += NewlineString->LengthInChars;
        memcpy(Ptr, MultilineEdit->LineArray[LastLine].StartOfString, LastCharOffset * sizeof(TCHAR));
        Ptr += LastCharOffset;

        SelectedText->LengthInChars = (DWORD)(Ptr - SelectedText->StartOfString);
    }
}

/**
 Build a single continuous string covering the specified range in a multiline
 edit control and return it in a new allocation.

 @param MultilineEdit Pointer to the multiline edit control containing the
        contents of the buffer.

 @param FirstLine Specifies the line that contains the first character to
        return.

 @param FirstCharOffset Specifies the offset within FirstLine of the first
        character to return.

 @param LastLine Specifies the line that contains the last character to
        return.

 @param LastCharOffset Specifies the offset beyond the last character to
        return.

 @param NewlineString Specifies the string to use to delimit lines.  This
        allows this routine to return text with any arbitrary line ending.

 @param SelectedText On successful completion, populated with a newly
        allocated buffer containing the selected text.  The caller is
        expected to free this buffer with @ref YoriLibFreeStringContents .
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMultilineEditGetTextRange(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in DWORD LastLine,
    __in DWORD LastCharOffset,
    __in PYORI_STRING NewlineString,
    __out PYORI_STRING SelectedText
    )
{
    DWORD CharsInRange;

    CharsInRange = YoriWinMultilineEditGetTextRangeLength(MultilineEdit, FirstLine, FirstCharOffset, LastLine, LastCharOffset, NewlineString->LengthInChars);

    if (!YoriLibAllocateString(SelectedText, CharsInRange + 1)) {
        return FALSE;
    }

    YoriWinMultilineEditPopulateTextRange(MultilineEdit, FirstLine, FirstCharOffset, LastLine, LastCharOffset, NewlineString, SelectedText);
    SelectedText->StartOfString[CharsInRange] = '\0';

    return TRUE;
}


/**
 Delete a range of characters, which may span lines.  This is used when
 deleting a selection.  When deleting ranges that are not entire lines,
 this implies merging the end of one line with the beginning of another.

 @param MultilineEdit Pointer to the multiline edit control containing the
        contents of the buffer.

 @param ProcessingBackspace If TRUE, this delete is a response to the
        backspace key, which means any auto indent that has previously
        been applied should remain in effect.  Backspace processing should
        clear this explicitly if the indentation has been reduced to zero.
        If FALSE, this routine is considered a buffer modification that
        invalidates auto indent.

 @param ProcessingUndo If TRUE, this delete is being invoked by undo and
        should not try to create or maintain an undo entry.

 @param FirstLine Specifies the line that contains the first character to
        remove.

 @param FirstCharOffset Specifies the offset within FirstLine of the first
        character to remove.

 @param LastLine Specifies the line that contains the last character to
        remove.

 @param LastCharOffset Specifies the offset beyond the last character to
        remove.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMultilineEditDeleteTextRange(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in BOOLEAN ProcessingBackspace,
    __in BOOLEAN ProcessingUndo,
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in DWORD LastLine,
    __in DWORD LastCharOffset
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo = NULL;
    DWORD CharsToCopy;
    DWORD CharsToDelete;
    DWORD LinesToDelete;
    DWORD LinesToCopy;
    DWORD FirstLineIndexToKeep;
    DWORD LineIndexToDelete;
    PYORI_STRING Line;
    PYORI_STRING FinalLine;

    if (!ProcessingBackspace) {
        MultilineEdit->AutoIndentApplied = FALSE;
    }

    if (!ProcessingUndo) {
        BOOLEAN RangeBeforeExistingRange;
        Undo = YoriWinMultilineEditGetUndoRecordForOperation(MultilineEdit, YoriWinMultilineEditUndoDeleteText, FirstLine, FirstCharOffset, LastLine, LastCharOffset, &RangeBeforeExistingRange);
        if (Undo != NULL) {
            YORI_STRING Newline;
            YORI_STRING Text;
            DWORD CharsNeeded;
            YoriLibConstantString(&Newline, _T("\n"));

            CharsNeeded = YoriWinMultilineEditGetTextRangeLength(MultilineEdit, FirstLine, FirstCharOffset, LastLine, LastCharOffset, Newline.LengthInChars);

            if (!YoriWinMultilineEditEnsureSpaceBeforeOrAfterString(&Undo->u.DeleteText.Text,
                                                                    CharsNeeded,
                                                                    RangeBeforeExistingRange,
                                                                    &Text)) {
                return FALSE;
            }

            YoriWinMultilineEditPopulateTextRange(MultilineEdit, FirstLine, FirstCharOffset, LastLine, LastCharOffset, &Newline, &Text);
            if (RangeBeforeExistingRange) {
                Undo->u.DeleteText.FirstLine = FirstLine;
                Undo->u.DeleteText.FirstCharOffset = FirstCharOffset;
            }
        }
    }

    Line = &MultilineEdit->LineArray[FirstLine];

    //
    //  If the selection is one line, this is a simple case, because no
    //  line combining is required.
    //

    if (FirstLine == LastLine) {

        if (FirstCharOffset >= LastCharOffset || FirstCharOffset > Line->LengthInChars) {
            return TRUE;
        }

        if (LastCharOffset > Line->LengthInChars) {
            CharsToDelete = Line->LengthInChars - FirstCharOffset;
            CharsToCopy = 0;
        } else {
            CharsToDelete = LastCharOffset - FirstCharOffset;
            CharsToCopy = Line->LengthInChars - LastCharOffset;
        }

        if (CharsToCopy > 0) {
            memmove(&Line->StartOfString[FirstCharOffset],
                    &Line->StartOfString[LastCharOffset],
                    CharsToCopy * sizeof(TCHAR));
        }

        Line->LengthInChars -= CharsToDelete;
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, FirstLine, FirstLine);
        MultilineEdit->UserModified = TRUE;
        return TRUE;
    }

    LinesToDelete = 0;
    ASSERT(LastLine < MultilineEdit->LinesPopulated ||
           (LastLine == MultilineEdit->LinesPopulated && LastCharOffset == 0));
    if (LastLine < MultilineEdit->LinesPopulated) {
        FinalLine = &MultilineEdit->LineArray[LastLine];
    } else {
        FinalLine = NULL;
    }

    if (FinalLine != NULL && LastCharOffset < FinalLine->LengthInChars) {
        CharsToCopy = FinalLine->LengthInChars - LastCharOffset;
    } else {
        CharsToCopy = 0;
    }

    //
    //  If the first part of the first line and the last part of the last
    //  line (the unselected regions of each) don't fit in the first line's
    //  allocation, reallocate it.
    //

    if (FinalLine != NULL &&
        FirstCharOffset + CharsToCopy > Line->LengthAllocated) {

        YORI_STRING NewLine;
        DWORD CharsFromFirstLine;
        if (!YoriLibAllocateString(&NewLine, FirstCharOffset + CharsToCopy + YORI_WIN_MULTILINE_EDIT_LINE_PADDING)) {
            return FALSE;
        }

        if (FirstCharOffset < Line->LengthInChars) {
            CharsFromFirstLine = FirstCharOffset;
        } else {
            CharsFromFirstLine = Line->LengthInChars;
        }

        memcpy(NewLine.StartOfString, Line->StartOfString, CharsFromFirstLine * sizeof(TCHAR));
        while (CharsFromFirstLine < FirstCharOffset) {
            NewLine.StartOfString[CharsFromFirstLine] = ' ';
            CharsFromFirstLine++;
        }

        NewLine.LengthInChars = FirstCharOffset;
        YoriLibFreeStringContents(Line);
        memcpy(Line, &NewLine, sizeof(YORI_STRING));
    }

    //
    //  Move the combined regions into one line.
    //

    if (CharsToCopy > 0) {
        memcpy(&Line->StartOfString[FirstCharOffset],
               &FinalLine->StartOfString[LastCharOffset],
               CharsToCopy * sizeof(TCHAR));
    }

    //
    //  Delete any completely selected lines.
    //

    Line->LengthInChars = FirstCharOffset + CharsToCopy;
    if (LastLine < MultilineEdit->LinesPopulated) {
        LinesToDelete = LastLine - FirstLine;
    } else {
        if (FirstLine + 1 < MultilineEdit->LinesPopulated) {
            LinesToDelete = MultilineEdit->LinesPopulated - 1 - FirstLine;
        } else {
            LinesToDelete = 0;
        }
    }

    for (LineIndexToDelete = 0; LineIndexToDelete < LinesToDelete; LineIndexToDelete++) {
        YoriLibFreeStringContents(&MultilineEdit->LineArray[FirstLine + 1 + LineIndexToDelete]);
    }

    FirstLineIndexToKeep = FirstLine + 1 + LinesToDelete;
    if (FirstLineIndexToKeep < MultilineEdit->LinesPopulated) {
        LinesToCopy = MultilineEdit->LinesPopulated - FirstLineIndexToKeep;
    } else {
        LinesToCopy = 0;
    }

    if (LinesToCopy > 0) {
        memmove(&MultilineEdit->LineArray[FirstLine + 1],
                &MultilineEdit->LineArray[FirstLine + 1 + LinesToDelete],
                LinesToCopy * sizeof(YORI_STRING));
    }

    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, FirstLine, MultilineEdit->LinesPopulated);
    MultilineEdit->UserModified = TRUE;

    MultilineEdit->LinesPopulated = MultilineEdit->LinesPopulated - LinesToDelete;

    return TRUE;
}

/**
 Given a starting location and a pile of text, determine the ending point of
 the pile of text.

 @param FirstLine Specifies the first line of the text.

 @param FirstCharOffset Specifies the offset within the line to start
        inserting text.

 @param Text Pointer to the text to insert.

 @param LastLine On completion, updated to indicate the final line containing
        the text.

 @param LastCharOffset On completion, updated to indicate the character
        following the final character in the text.
 */
VOID
YoriWinMultilineEditCalculateEndingPointOfText(
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in PYORI_STRING Text,
    __out PDWORD LastLine,
    __out PDWORD LastCharOffset
    )
{
    DWORD LineCount;
    DWORD Index;
    DWORD LineCharCount;

    //
    //  Count the number of lines in the input text.  This may be zero.
    //

    LineCount = 0;
    LineCharCount = FirstCharOffset;
    for (Index = 0; Index < Text->LengthInChars; Index++) {
        if (Text->StartOfString[Index] == '\r') {
            LineCount++;
            if (Index + 1 < Text->LengthInChars &&
                Text->StartOfString[Index + 1] == '\n') {
                Index++;
            }
            LineCharCount = 0;
        } else if (Text->StartOfString[Index] == '\n') {
            LineCount++;
            LineCharCount = 0;
        } else {
            LineCharCount++;
        }
    }

    *LastLine = FirstLine + LineCount;
    *LastCharOffset = LineCharCount;
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
 Create new empty lines after an insertion point, and move all existing lines
 further down.

 @param MultilineEdit Pointer to the multiline edit control.

 @param FirstLine The line that insertion should happen after.

 @param LineCount The number of lines to insert.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditInsertLines(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD FirstLine,
    __in DWORD LineCount
    )
{
    DWORD SourceLine;
    DWORD TargetLine;
    DWORD LinesNeeded;
    DWORD Index;

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
        SourceLine = FirstLine + 1;
        TargetLine = SourceLine + LineCount;
    } else {
        SourceLine = FirstLine;
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
    return TRUE;
}

/**
 Insert a block of text, which may contain newlines, into the control at the
 specified position.  Currently, this happens in three scenarios: user input,
 clipboard paste, or undo.

 @param MultilineEdit Pointer to the multiline edit control.

 @param ProcessingUndo If TRUE, this insert is being invoked by undo and
        should not try to create or maintain an undo entry.

 @param FirstLine Specifies the line in the buffer where text should be
        inserted.

 @param FirstCharOffset Specifies the offset in the line where text should be
        inserted.

 @param Text Pointer to the text to insert.

 @param LastLine On successful completion, populated with the line containing
        the end of the newly inserted text.

 @param LastCharOffset On successful completion, populated with the offset
        beyond the newly inserted text.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMultilineEditInsertTextRange(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in BOOLEAN ProcessingUndo,
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in PYORI_STRING Text,
    __out PDWORD LastLine,
    __out PDWORD LastCharOffset
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo = NULL;
    DWORD LineCount;
    DWORD LineIndex;
    DWORD Index;
    DWORD CharsThisLine;
    DWORD CharsFirstLine;
    DWORD CharsLastLine;
    DWORD CharsNeeded;
    DWORD LocalLastLine;
    DWORD LocalLastCharOffset;
    YORI_STRING TrailingPortionOfFirstLine;
    YORI_STRING AutoIndentLeadingString;
    PYORI_STRING Line;
    BOOLEAN TerminateLine;

    YoriLibInitEmptyString(&AutoIndentLeadingString);

    //
    //  Count the number of lines in the input text.  This may be zero.
    //

    YoriWinMultilineEditCalculateEndingPointOfText(FirstLine, FirstCharOffset, Text, &LocalLastLine, &LocalLastCharOffset);
    LineCount = LocalLastLine - FirstLine;

    //
    //  If auto indent is in effect, and the text ends on the beginning of
    //  a subsequent line, calculate any indentation prefix.
    //

    if (LineCount > 0 &&
        LocalLastCharOffset == 0 &&
        MultilineEdit->AutoIndent &&
        !ProcessingUndo &&
        FirstLine < MultilineEdit->LinesPopulated) {

        YoriWinMultilineEditGetIndentationOnLine(MultilineEdit, FirstLine, &AutoIndentLeadingString);
        LocalLastCharOffset = AutoIndentLeadingString.LengthInChars;
    }

    //
    //  If new lines are being added, check if the line array is large
    //  enough and reallocate as needed.  Even if lines are already
    //  allocated, the current lines need to be moved downwards to make room
    //  for the lines that are about to be inserted.
    //

    if (LineCount > 0 || MultilineEdit->LinesPopulated == 0) {
        if (!YoriWinMultilineEditInsertLines(MultilineEdit, FirstLine, LineCount)) {
            return FALSE;
        }
    }

    //
    //  Record pointers to the string following the cursor on the current
    //  cursor line.  This text needs to be logically moved to the end of
    //  the newly inserted text, which is on a new line.  To achieve this
    //  the current text is pointed to, and the first line is processed
    //  last, after the last line has been constructed.
    //

    YoriLibInitEmptyString(&TrailingPortionOfFirstLine);
    if (FirstLine < MultilineEdit->LinesPopulated) {
        Line = &MultilineEdit->LineArray[FirstLine];
        if (FirstCharOffset < Line->LengthInChars) {
            TrailingPortionOfFirstLine.StartOfString = &Line->StartOfString[FirstCharOffset];
            TrailingPortionOfFirstLine.LengthInChars = Line->LengthInChars - FirstCharOffset;
        }
    }

    MultilineEdit->AutoIndentApplied = FALSE;

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
                Line = &MultilineEdit->LineArray[FirstLine + LineIndex];
                ASSERT(Line->LengthInChars == 0);
                CharsNeeded = CharsThisLine;
                if (LineIndex == LineCount) {
                    CharsNeeded += AutoIndentLeadingString.LengthInChars + TrailingPortionOfFirstLine.LengthInChars;
                }
                if (Line->LengthAllocated < CharsNeeded) {
                    YoriLibFreeStringContents(Line);
                    if (!YoriLibAllocateString(Line, CharsNeeded + YORI_WIN_MULTILINE_EDIT_LINE_PADDING)) {
                        return FALSE;
                    }
                }

                //
                //  On the final line, apply any auto indent if needed.  Auto
                //  indent wouldn't make sense if new data is arriving.
                //

                Line->LengthInChars = 0;
                if (LineIndex == LineCount) {
                    if (AutoIndentLeadingString.LengthInChars > 0) {

                        ASSERT(CharsThisLine == 0);

                        memcpy(Line->StartOfString,
                               AutoIndentLeadingString.StartOfString,
                               AutoIndentLeadingString.LengthInChars * sizeof(TCHAR));
                        Line->LengthInChars = AutoIndentLeadingString.LengthInChars;
                        MultilineEdit->AutoIndentApplied = TRUE;
                        MultilineEdit->AutoIndentSourceLine = FirstLine;
                        MultilineEdit->AutoIndentSourceLength = AutoIndentLeadingString.LengthInChars;
                        MultilineEdit->AutoIndentAppliedLine = LocalLastLine;
                    }
                }

                //
                //  Add the new text to the beginning of the line
                //

                if (CharsThisLine > 0) {
                    memcpy(&Line->StartOfString[Line->LengthInChars],
                           &Text->StartOfString[Index - CharsThisLine],
                           CharsThisLine * sizeof(TCHAR));
                }
                Line->LengthInChars = Line->LengthInChars + CharsThisLine;

                //
                //  On the final line, copy the final portion currently in the
                //  first line after the newly inserted text.  Save away the
                //  number of characters on this line so that the cursor can
                //  be positioned at that point.
                //

                if (LineIndex == LineCount) {
                    CharsLastLine = AutoIndentLeadingString.LengthInChars + CharsThisLine;
                    if (TrailingPortionOfFirstLine.LengthInChars > 0) {
                        memcpy(&Line->StartOfString[CharsThisLine + AutoIndentLeadingString.LengthInChars],
                               TrailingPortionOfFirstLine.StartOfString,
                               TrailingPortionOfFirstLine.LengthInChars * sizeof(TCHAR));
                        Line->LengthInChars += TrailingPortionOfFirstLine.LengthInChars;
                    }
                }
            }
            LineIndex++;
            CharsThisLine = 0;
            TerminateLine = FALSE;

            //
            //  Skip one extra char if this is a \r\n line
            //

            if (Index + 1 < Text->LengthInChars &&
                Text->StartOfString[Index] == '\r' && 
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
        YoriLibInitEmptyString(&TrailingPortionOfFirstLine);
    }

    Line = &MultilineEdit->LineArray[FirstLine];
    if (FirstCharOffset + CharsFirstLine + TrailingPortionOfFirstLine.LengthInChars > Line->LengthAllocated) {
        if (!YoriLibReallocateString(Line, FirstCharOffset + CharsFirstLine + TrailingPortionOfFirstLine.LengthInChars + YORI_WIN_MULTILINE_EDIT_LINE_PADDING)) {
            return FALSE;
        }
    }

    if (TrailingPortionOfFirstLine.LengthInChars > 0) {
        memmove(&Line->StartOfString[FirstCharOffset + CharsFirstLine],
                TrailingPortionOfFirstLine.StartOfString,
                TrailingPortionOfFirstLine.LengthInChars * sizeof(TCHAR));
    }

    while (FirstCharOffset > Line->LengthInChars) {
        Line->StartOfString[Line->LengthInChars++] = ' ';
    }

    if (CharsFirstLine > 0) {
        memcpy(&Line->StartOfString[FirstCharOffset], Text->StartOfString, CharsFirstLine * sizeof(TCHAR));
    }
    Line->LengthInChars = FirstCharOffset + CharsFirstLine + TrailingPortionOfFirstLine.LengthInChars;

    if (LineCount > 0) {
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, FirstLine, (DWORD)-1);
        ASSERT(LocalLastCharOffset == CharsLastLine);
    } else {
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, FirstLine, FirstLine);
        ASSERT(LocalLastCharOffset == FirstCharOffset + CharsFirstLine);
    }

    if (!ProcessingUndo) {
        BOOLEAN RangeBeforeExistingRange;
        Undo = YoriWinMultilineEditGetUndoRecordForOperation(MultilineEdit, YoriWinMultilineEditUndoInsertText, FirstLine, FirstCharOffset, LocalLastLine, LocalLastCharOffset, &RangeBeforeExistingRange);
        if (Undo != NULL) {
            Undo->u.InsertText.LastLineToDelete = LocalLastLine;
            Undo->u.InsertText.LastCharOffsetToDelete = LocalLastCharOffset;
        }
    }

    //
    //  Set the cursor to be after the newly inserted range.
    //

    *LastLine = LocalLastLine;
    *LastCharOffset = LocalLastCharOffset;
    MultilineEdit->UserModified = TRUE;

    return TRUE;
}

/**
 Overwrite a block of text, which may contain newlines, into the control at the
 specified position.  Note that "overwrite" in this context refers to adding
 text with insert mode off.  This is not a true/strict overwrite, because the
 semantics of typing with insert off is that new lines are inserted, but text
 on an existing line are overwritten.

 @param MultilineEdit Pointer to the multiline edit control.

 @param ProcessingUndo If TRUE, this overwrite is being invoked by undo and
        should not try to create or maintain an undo entry.

 @param FirstLine Specifies the line in the buffer where text should be
        added.

 @param FirstCharOffset Specifies the offset in the line where text should be
        added.

 @param Text Pointer to the text to add.

 @param LastLine On successful completion, populated with the line containing
        the end of the newly added text.

 @param LastCharOffset On successful completion, populated with the offset
        beyond the newly added text.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMultilineEditOverwriteTextRange(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in BOOLEAN ProcessingUndo,
    __in DWORD FirstLine,
    __in DWORD FirstCharOffset,
    __in PYORI_STRING Text,
    __out PDWORD LastLine,
    __out PDWORD LastCharOffset
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT_UNDO Undo = NULL;
    DWORD LineCount;
    DWORD LineIndex;
    DWORD Index;
    DWORD CharsThisLine;
    DWORD CharsLastLine;
    DWORD CharsNeeded;
    DWORD StartOffsetThisLine;
    DWORD LocalLastLine;
    DWORD LocalLastCharOffset;
    PYORI_STRING Line;
    BOOLEAN TerminateLine;
    BOOLEAN MoveTrailingTextToNextLine;

    MultilineEdit->AutoIndentApplied = FALSE;

    if (!ProcessingUndo) {
        BOOLEAN RangeBeforeExistingRange;

        //
        //  At this point we don't know the ending range for this text but it
        //  doesn't matter.  An overwrite will only extend a previous one, not
        //  occur before it, so the end range specified here can be bogus.
        //

        Undo = YoriWinMultilineEditGetUndoRecordForOperation(MultilineEdit, YoriWinMultilineEditUndoOverwriteText, FirstLine, FirstCharOffset, FirstLine, FirstCharOffset, &RangeBeforeExistingRange);
        if (Undo != NULL) {

            //
            //  If this is a new record, save off the entire line to be
            //  deleted and restored.  This is done to ensure it doesn't
            //  need to be manipulated on each keypress.  It also means if
            //  the user starts a new line, the delete range can be expanded
            //  while leaving the restore range alone.
            //

            if (Undo->u.OverwriteText.Text.StartOfString == NULL) {
                Line = &MultilineEdit->LineArray[FirstLine];
                if (!YoriLibAllocateString(&Undo->u.OverwriteText.Text, Line->LengthInChars)) {
                    return FALSE;
                }

                memcpy(Undo->u.OverwriteText.Text.StartOfString, Line->StartOfString, Line->LengthInChars * sizeof(TCHAR));
                Undo->u.OverwriteText.Text.LengthInChars = Line->LengthInChars;

                Undo->u.OverwriteText.FirstLineToDelete = FirstLine;
                Undo->u.OverwriteText.FirstCharOffsetToDelete = 0;
                Undo->u.OverwriteText.LastLineToDelete = FirstLine;
                Undo->u.OverwriteText.LastCharOffsetToDelete = Line->LengthInChars;
                Undo->u.OverwriteText.FirstLine = FirstLine;
                Undo->u.OverwriteText.FirstCharOffset = 0;
            }
        }
    }

    //
    //  Count the number of lines in the input text.  This may be zero.
    //

    YoriWinMultilineEditCalculateEndingPointOfText(FirstLine, FirstCharOffset, Text, &LocalLastLine, &LocalLastCharOffset);
    LineCount = LocalLastLine - FirstLine;

    //
    //  If new lines are being added, check if the line array is large
    //  enough and reallocate as needed.  Even if lines are already
    //  allocated, the current lines need to be moved downwards to make room
    //  for the lines that are about to be inserted.
    //

    if (LineCount > 0 || MultilineEdit->LinesPopulated == 0) {
        if (!YoriWinMultilineEditInsertLines(MultilineEdit, FirstLine, LineCount)) {
            return FALSE;
        }
        if (Undo != NULL) {
            Undo->u.OverwriteText.LastLineToDelete = FirstLine + LineCount;
            Undo->u.OverwriteText.LastCharOffsetToDelete = 0;
        }
    }

    //
    //  Go through each line.  Construct the new line.  For all lines except
    //  the first, these lines should be empty lines due to the line
    //  rearrangement above.
    //

    LineIndex = 0;
    CharsThisLine = 0;
    CharsLastLine = 0;
    TerminateLine = FALSE;
    MoveTrailingTextToNextLine = FALSE;
    for (Index = 0; Index <= Text->LengthInChars; Index++) {

        //
        //  Look for end of line, and treat end of string as end of line
        //

        if (Index == Text->LengthInChars) {
            TerminateLine = TRUE;
            MoveTrailingTextToNextLine = FALSE;
        } else if (Text->StartOfString[Index] == '\r' ||
                   Text->StartOfString[Index] == '\n') {

            TerminateLine = TRUE;
            MoveTrailingTextToNextLine = TRUE;
        }

        if (TerminateLine) {

            //
            //  On the end of the first line, make a note of where the string
            //  is.
            //

            StartOffsetThisLine = 0;
            if (LineIndex == 0) {
                StartOffsetThisLine = FirstCharOffset;
            }

            Line = &MultilineEdit->LineArray[FirstLine + LineIndex];
            CharsNeeded = StartOffsetThisLine + CharsThisLine;
            if (Line->LengthAllocated < CharsNeeded) {
                YoriLibFreeStringContents(Line);
                if (!YoriLibAllocateString(Line, CharsNeeded + YORI_WIN_MULTILINE_EDIT_LINE_PADDING)) {
                    return FALSE;
                }
            }

            while (StartOffsetThisLine > Line->LengthInChars) {
                Line->StartOfString[Line->LengthInChars++] = ' ';
            }

            if (CharsThisLine > 0) {
                memcpy(&Line->StartOfString[StartOffsetThisLine],
                       &Text->StartOfString[Index - CharsThisLine],
                       CharsThisLine * sizeof(TCHAR));
            }

            //
            //  If the line is extending, extend it.  If enter was pressed,
            //  move any contents after this point to the next line.
            //

            if (StartOffsetThisLine + CharsThisLine > Line->LengthInChars) {
                Line->LengthInChars = StartOffsetThisLine + CharsThisLine;
            } else if (MoveTrailingTextToNextLine && Line->LengthInChars > StartOffsetThisLine + CharsThisLine) {
                PYORI_STRING NextLine;
                NextLine = &MultilineEdit->LineArray[FirstLine + LineIndex + 1];
                ASSERT(NextLine->LengthInChars == 0);
                CharsNeeded = Line->LengthInChars - (StartOffsetThisLine + CharsThisLine);
                if (NextLine->LengthAllocated < CharsNeeded) {
                    YoriLibFreeStringContents(NextLine);
                    if (!YoriLibAllocateString(NextLine, CharsNeeded + YORI_WIN_MULTILINE_EDIT_LINE_PADDING)) {
                        return FALSE;
                    }
                }

                memcpy(NextLine->StartOfString,
                       &Line->StartOfString[StartOffsetThisLine + CharsThisLine],
                       CharsNeeded * sizeof(TCHAR));
                NextLine->LengthInChars = CharsNeeded;
                Line->LengthInChars = StartOffsetThisLine + CharsThisLine;
            }

            //
            //  Save away the number of characters on this line so that
            //  the cursor can be positioned at that point.  Update the undo
            //  record so that any modification after this point is attributed
            //  to the same undo record, and any changes made up to this point
            //  need to be deleted.
            //

            if (LineIndex == LineCount) {
                CharsLastLine = CharsThisLine;
                if (Undo != NULL) {
                    ASSERT(Undo->u.OverwriteText.LastLineToDelete == FirstLine + LineIndex);
                    Undo->u.OverwriteText.LastCharOffsetModified = StartOffsetThisLine + CharsLastLine;

                    if (Line->LengthInChars > Undo->u.OverwriteText.LastCharOffsetToDelete) {
                        Undo->u.OverwriteText.LastCharOffsetToDelete = Line->LengthInChars;
                    }
                }
            }

            LineIndex++;
            CharsThisLine = 0;
            TerminateLine = FALSE;

            //
            //  Skip one extra char if this is a \r\n line
            //

            if (Index + 1 < Text->LengthInChars &&
                Text->StartOfString[Index] == '\r' && 
                Text->StartOfString[Index + 1] == '\n') {

                Index++;
            }
            continue;
        }

        CharsThisLine++;
    }


    //
    //  Set the cursor to be after the newly inserted range.
    //

    if (LineCount > 0) {
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, FirstLine, (DWORD)-1);
        *LastLine = FirstLine + LineCount;
        *LastCharOffset = CharsLastLine;
        ASSERT(LocalLastLine == FirstLine + LineCount);
        ASSERT(LocalLastCharOffset == CharsLastLine);
    } else {
        YoriWinMultilineEditExpandDirtyRange(MultilineEdit, FirstLine, FirstLine);
        *LastLine = FirstLine;
        *LastCharOffset = FirstCharOffset + CharsLastLine;
        ASSERT(LocalLastLine == FirstLine + LineCount);
        ASSERT(LocalLastCharOffset == FirstCharOffset + CharsLastLine);
    }
    MultilineEdit->UserModified = TRUE;

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

    YoriWinMultilineEditClearUndo(MultilineEdit);

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

//
//  =========================================
//  SELECTION FUNCTIONS
//  =========================================
//

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
    DWORD FirstLine;
    DWORD FirstCharOffset;
    DWORD LastLine;
    DWORD LastCharOffset;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (!YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
        return TRUE;
    }

    Selection = &MultilineEdit->Selection;

    FirstLine = Selection->FirstLine;
    FirstCharOffset = Selection->FirstCharOffset;
    LastLine = Selection->LastLine;
    LastCharOffset = Selection->LastCharOffset;

    if (!YoriWinMultilineEditDeleteTextRange(MultilineEdit, FALSE, FALSE, FirstLine, FirstCharOffset, LastLine, LastCharOffset)) {
        return FALSE;
    }

    YoriWinMultilineEditClearSelection(MultilineEdit);
    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, FirstCharOffset, FirstLine);

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
__success(return)
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

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (!YoriWinMultilineEditSelectionActive(CtrlHandle)) {
        YoriLibInitEmptyString(SelectedText);
        return TRUE;
    }

    Selection = &MultilineEdit->Selection;

    return YoriWinMultilineEditGetTextRange(MultilineEdit, Selection->FirstLine, Selection->FirstCharOffset, Selection->LastLine, Selection->LastCharOffset, NewlineString, SelectedText);
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

    if (MultilineEdit->Timer != NULL) {
        YoriWinMgrFreeTimer(MultilineEdit->Timer);
        MultilineEdit->Timer = NULL;
    }
}


/**
 Get the selection range within a multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @param StartLine Specifies the line index of the beginning of the selection.

 @param StartOffset Specifies the character offset within the line to the
        beginning of the selection.

 @param EndLine Specifies the line index of the end of the selection.

 @param EndOffset Specifies the character offset within the line to the
        end of the selection.

 @return TRUE to indicate that the selection is active and a range has been
         returned.  FALSE to indicate no selection is active.
 */
__success(return)
BOOLEAN
YoriWinMultilineEditGetSelectionRange(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PDWORD StartLine,
    __out PDWORD StartOffset,
    __out PDWORD EndLine,
    __out PDWORD EndOffset
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    if (!YoriWinMultilineEditSelectionActive(CtrlHandle)) {
        return FALSE;
    }

    MultilineEdit = (PYORI_WIN_CTRL_MULTILINE_EDIT)CtrlHandle;

    *StartLine = MultilineEdit->Selection.FirstLine;
    *StartOffset = MultilineEdit->Selection.FirstCharOffset;
    *EndLine = MultilineEdit->Selection.LastLine;
    *EndOffset = MultilineEdit->Selection.LastCharOffset;

    return TRUE;
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

//
//  =========================================
//  CLIPBOARD FUNCTIONS
//  =========================================
//

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

    if (!YoriLibCopyTextWithProcessFallback(&Text)) {
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

    if (!YoriLibCopyTextWithProcessFallback(&Text)) {
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

    if (!YoriLibPasteTextWithProcessFallback(&Text)) {
        return FALSE;
    }
    if (!YoriWinMultilineEditInsertTextAtCursor(CtrlHandle, &Text)) {
        YoriLibFreeStringContents(&Text);
        return FALSE;
    }

    YoriLibFreeStringContents(&Text);
    return TRUE;
}

//
//  =========================================
//  GENERAL EXPORTED API FUNCTIONS
//  =========================================
//

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
    DWORD LastLine;
    DWORD LastCharOffset;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    if (!YoriWinMultilineEditInsertTextRange(MultilineEdit, FALSE, MultilineEdit->CursorLine, MultilineEdit->CursorOffset, Text, &LastLine, &LastCharOffset)) {
        return FALSE;
    }

    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, LastCharOffset, LastLine);
    return TRUE;
}

/**
 Set the color attributes of the multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @param Attributes Specifies the foreground and background color for the
        multiline edit control to use.

 @param SelectedAttributes Specifies the foreground and background color
        to use for selected text within the multiline edit control.
 */
VOID
YoriWinMultilineEditSetColor(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in WORD Attributes,
    __in WORD SelectedAttributes
    )
{
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);

    MultilineEdit->TextAttributes = Attributes;
    MultilineEdit->SelectedAttributes = SelectedAttributes;
    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, 0, (DWORD)-1);
    YoriWinMultilineEditPaintNonClient(MultilineEdit);
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
    YoriWinMultilineEditClearUndo(MultilineEdit);

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
 Enable or disable traditional MS-DOS edit navigation rules.  In the
 traditional model, the cursor can move infinitely right of the text in any
 line, so the cursor's line does not change in response to left and right keys.
 In the more modern model, navigating left beyond the beginning of the line
 moves to the previous line, and navigating right beyond the end of the line
 moves to the next line.

 @param CtrlHandle Pointer to the multiline edit control.

 @param TraditionalNavigationEnabled TRUE to use traditional MS-DOS edit
        navigation, FALSE to use Windows style multiline edit navigation.
 */
VOID
YoriWinMultilineEditSetTraditionalNavigation(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN TraditionalNavigationEnabled
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);
    MultilineEdit->TraditionalEditNavigation = TraditionalNavigationEnabled;
    YoriWinMultilineEditClearDesiredDisplayOffset(MultilineEdit);
    if (!MultilineEdit->TraditionalEditNavigation) {
        if (MultilineEdit->CursorLine < MultilineEdit->LinesPopulated) {
            if (MultilineEdit->CursorOffset > MultilineEdit->LineArray[MultilineEdit->CursorLine].LengthInChars) {
                YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, MultilineEdit->LineArray[MultilineEdit->CursorLine].LengthInChars, MultilineEdit->CursorLine);
            }
        }
    }
}

/**
 Enable or disable auto indent.  If a new line is created when auto indent
 is enabled, the line is initialized with the leading white space from the
 previous line.  If auto indent is disabled, a new line is initialized with
 no leading white space.

 @param CtrlHandle Pointer to the multiline edit control.

 @param AutoIndentEnabled TRUE to enable auto indent behavior; FALSE to
        disable auto indent behavior.
 */
VOID
YoriWinMultilineEditSetAutoIndent(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN AutoIndentEnabled
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);
    MultilineEdit->AutoIndent = AutoIndentEnabled;
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
    __in PYORI_WIN_NOTIFY_MULTILINE_EDIT_CURSOR_MOVE NotifyCallback
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

//
//  =========================================
//  INPUT HANDLING FUNCTIONS
//  =========================================
//

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
    PYORI_STRING Line;
    DWORD FirstLine;
    DWORD FirstCharOffset;
    DWORD LastLine;
    DWORD LastCharOffset;

    if (MultilineEdit->CursorLine >= MultilineEdit->LinesPopulated) {
        return FALSE;
    }

    YoriWinMultilineEditClearDesiredDisplayOffset(MultilineEdit);

    if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
        return YoriWinMultilineEditDeleteSelection(&MultilineEdit->Ctrl);
    }

    Line = &MultilineEdit->LineArray[MultilineEdit->CursorLine];

    LastLine = MultilineEdit->CursorLine;
    LastCharOffset = MultilineEdit->CursorOffset;

    if (MultilineEdit->AutoIndentApplied) {
        YORI_STRING NewIndent;
        DWORD NewIndentSourceLine;

        ASSERT(LastCharOffset > 0);
        YoriWinMultilineEditFindPreviousIndentLine(MultilineEdit, &NewIndentSourceLine, &NewIndent);

        FirstLine = LastLine;
        FirstCharOffset = NewIndent.LengthInChars;

        if (NewIndent.LengthInChars == 0) {
            MultilineEdit->AutoIndentApplied = FALSE;
        } else {
            MultilineEdit->AutoIndentSourceLine = NewIndentSourceLine;
            MultilineEdit->AutoIndentSourceLength = NewIndent.LengthInChars;
        }

    } else if (LastCharOffset == 0) {

        //
        //  If we're at the beginning of the line, we may need to merge lines.
        //  If it's the first line, we're finished.
        //

        if (MultilineEdit->CursorLine == 0) {
            return FALSE;
        }

        FirstLine = MultilineEdit->CursorLine - 1;
        FirstCharOffset = MultilineEdit->LineArray[FirstLine].LengthInChars;
    } else {
        FirstLine = LastLine;
        FirstCharOffset = LastCharOffset - 1;
    }

    if (!YoriWinMultilineEditDeleteTextRange(MultilineEdit, TRUE, FALSE, FirstLine, FirstCharOffset, LastLine, LastCharOffset)) {
        return FALSE;
    }

    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, FirstCharOffset, FirstLine);
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
    PYORI_STRING Line;
    DWORD FirstLine;
    DWORD FirstCharOffset;
    DWORD LastLine;
    DWORD LastCharOffset;

    if (MultilineEdit->CursorLine >= MultilineEdit->LinesPopulated) {
        return FALSE;
    }

    if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
        return YoriWinMultilineEditDeleteSelection(&MultilineEdit->Ctrl);
    }

    Line = &MultilineEdit->LineArray[MultilineEdit->CursorLine];

    FirstLine = MultilineEdit->CursorLine;
    FirstCharOffset = MultilineEdit->CursorOffset;

    if (FirstCharOffset >= Line->LengthInChars) {
        LastLine = FirstLine + 1;
        LastCharOffset = 0;
    } else {
        LastLine = FirstLine;
        LastCharOffset = FirstCharOffset + 1;
    }

    if (!YoriWinMultilineEditDeleteTextRange(MultilineEdit, FALSE, FALSE, FirstLine, FirstCharOffset, LastLine, LastCharOffset)) {
        return FALSE;
    }

    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, FirstCharOffset, FirstLine);

    return TRUE;
}

/**
 Delete the line at the cursor and move later lines into position.

 @param MultilineEdit Pointer to the multiline edit control, indicating the
        current cursor location.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditDeleteLine(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit
    )
{
    if (MultilineEdit->LinesPopulated == 0) {
        return FALSE;
    }

    if (!YoriWinMultilineEditDeleteTextRange(MultilineEdit, FALSE, FALSE, MultilineEdit->CursorLine, 0, MultilineEdit->CursorLine + 1, 0)) {
        return FALSE;
    }

    return TRUE;
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

        YoriWinMultilineEditPopulateDesiredDisplayOffset(MultilineEdit);
        YoriWinMultilineEditFindCursorCharFromDisplayChar(MultilineEdit, NewCursorLine, MultilineEdit->DesiredDisplayCursorOffset, &NewCursorOffset);
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

        YoriWinMultilineEditPopulateDesiredDisplayOffset(MultilineEdit);
        YoriWinMultilineEditFindCursorCharFromDisplayChar(MultilineEdit, NewCursorLine, MultilineEdit->DesiredDisplayCursorOffset, &NewCursorOffset);
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
 Handle a double-click within a multi line edit control.  This is supposed to
 select a "word" which is delimited by a user controllable set of characters.

 @param MultilineEdit Pointer to the multiline edit control.

 @param ViewportX The horizontal position in the control relative to its
        client area.

 @param ViewportY The vertial position in the control relative to its
        client area.
 */
VOID
YoriWinMultilineEditNotifyDoubleClick(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in DWORD ViewportX,
    __in DWORD ViewportY
    )
{
    DWORD NewCursorLine;
    DWORD NewCursorChar;
    PYORI_STRING Line;
    YORI_STRING BreakChars;

    //
    //  Translate the viewport location into a buffer location.
    //

    if (YoriWinMultilineEditTranslateViewportCoordinatesToCursorCoordinates(MultilineEdit, ViewportX, ViewportY, &NewCursorLine, &NewCursorChar)) {
        DWORD BeginRangeOffset;
        DWORD EndRangeOffset;

        //
        //  If it's beyond the number of lines populated, there's nothing to
        //  select.
        //

        if (NewCursorLine >= MultilineEdit->LinesPopulated) {
            return;
        }

        //
        //  If it's beyond the end of the line, there's nothing to select.
        //

        Line = &MultilineEdit->LineArray[NewCursorLine];
        if (NewCursorChar >= Line->LengthInChars) {
            return;
        }

        //
        //  Determine which characters delimit words.
        //

        if (!YoriLibGetSelectionDoubleClickBreakChars(&BreakChars)) {
            return;
        }

        //
        //  Search left looking for a delimiter or the start of the string.
        //

        BeginRangeOffset = NewCursorChar;
        if (YoriLibFindLeftMostCharacter(&BreakChars, Line->StartOfString[BeginRangeOffset]) == NULL) {
            while (BeginRangeOffset > 0 &&
                   YoriLibFindLeftMostCharacter(&BreakChars, Line->StartOfString[BeginRangeOffset - 1]) == NULL) {
                BeginRangeOffset--;
            }
        }

        //
        //  Search right looking for a delimiter or the end of the string.
        //

        EndRangeOffset = NewCursorChar;
        while (EndRangeOffset < Line->LengthInChars &&
               YoriLibFindLeftMostCharacter(&BreakChars, Line->StartOfString[EndRangeOffset]) == NULL) {
            EndRangeOffset++;
        }

        YoriLibFreeStringContents(&BreakChars);

        //
        //  If any range was found (ie., the user didn't click on a word
        //  delimiter) select the range.
        //

        if (EndRangeOffset > BeginRangeOffset) {
            YoriWinMultilineEditSetSelectionRange(&MultilineEdit->Ctrl, NewCursorLine, BeginRangeOffset, NewCursorLine, EndRangeOffset);
        }
    }
}

/**
 Adjust the viewport and selection to reflect the mouse being dragged,
 potentially outside the control's client area while the button is held down,
 thereby extending the selection.

 @param MultilineEdit Pointer to the multiline edit control.

 @param MousePos Specifies the mouse position.
 */
VOID
YoriWinMultilineEditScrollForMouseSelect(
    __in PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit,
    __in PYORI_WIN_BOUNDED_COORD MousePos
    )
{
    COORD ClientSize;
    DWORD LineCountToDisplay;
    DWORD NewCursorLine;
    DWORD NewCursorOffset;
    DWORD NewViewportTop;
    DWORD NewViewportLeft;
    DWORD DisplayOffset;
    BOOLEAN SetTimer;

    SetTimer = FALSE;
    if (MousePos != &MultilineEdit->LastMousePos) {
        MultilineEdit->LastMousePos.Pos.X = MousePos->Pos.X;
        MultilineEdit->LastMousePos.Pos.Y = MousePos->Pos.Y;
        MultilineEdit->LastMousePos.Above = MousePos->Above;
        MultilineEdit->LastMousePos.Below = MousePos->Below;
        MultilineEdit->LastMousePos.Left = MousePos->Left;
        MultilineEdit->LastMousePos.Right = MousePos->Right;
    }

    YoriWinGetControlClientSize(&MultilineEdit->Ctrl, &ClientSize);
    LineCountToDisplay = ClientSize.Y;

    NewViewportTop = MultilineEdit->ViewportTop;
    NewViewportLeft = MultilineEdit->ViewportLeft;
    NewCursorLine = MultilineEdit->CursorLine;

    //
    //  First find the cursor line.  This can be above the viewport, below
    //  the viewport, or any line within the viewport.
    //

    if (MousePos->Above) {
        if (MultilineEdit->ViewportTop < 1) {
            NewCursorLine = 0;
        } else {
            NewCursorLine = NewViewportTop - 1;
        }
        SetTimer = TRUE;
    } else if (MousePos->Below) {
        if (NewViewportTop + 1 + LineCountToDisplay > MultilineEdit->LinesPopulated) {
            if (MultilineEdit->LinesPopulated > 0) {
                NewCursorLine = MultilineEdit->LinesPopulated - 1;
            } else {
                NewCursorLine = 0;
            }
        } else {
            NewCursorLine = NewViewportTop + LineCountToDisplay + 1;
        }
        SetTimer = TRUE;
    } else {
        if (NewViewportTop + MousePos->Pos.Y < MultilineEdit->LinesPopulated) {
            NewCursorLine = NewViewportTop + MousePos->Pos.Y;
        } else if (MultilineEdit->LinesPopulated > 0) {
            NewCursorLine = MultilineEdit->LinesPopulated - 1;
        } else {
            NewCursorLine = 0;
        }
    }

    //
    //  Now find the cursor column.  This can be left of the viewport, right
    //  of the viewport, or any column within the viewport.  When in the
    //  viewport, this needs to be translated from a display location to
    //  a buffer location.
    //

    if (MousePos->Left) {
        if (NewViewportLeft > 0) {
            DisplayOffset = NewViewportLeft - 1;
        } else {
            DisplayOffset = 0;
        }
        SetTimer = TRUE;
    } else if (MousePos->Right) {
        DisplayOffset = NewViewportLeft + ClientSize.X + 1;
        SetTimer = TRUE;
    } else {
        DisplayOffset = NewViewportLeft + MousePos->Pos.X;
    }

    if (SetTimer) {
        if (MultilineEdit->Timer == NULL) {
            PYORI_WIN_WINDOW TopLevelWindow;
            TopLevelWindow = YoriWinGetTopLevelWindow(&MultilineEdit->Ctrl);
            MultilineEdit->Timer = YoriWinMgrAllocateRecurringTimer(YoriWinGetWindowManagerHandle(TopLevelWindow), &MultilineEdit->Ctrl, 100);
        }
    } else {
        if (MultilineEdit->Timer != NULL) {
            YoriWinMgrFreeTimer(MultilineEdit->Timer);
            MultilineEdit->Timer = NULL;
        }
    }

    YoriWinMultilineEditFindCursorCharFromDisplayChar(MultilineEdit, NewCursorLine, DisplayOffset, &NewCursorOffset);

    //
    //  When using modern navigation, the cursor can't move to the right of
    //  the text in the line.  With traditional MS-DOS navigation, it can.
    //

    if (!MultilineEdit->TraditionalEditNavigation) {
        if (MultilineEdit->LinesPopulated > 0) {
            ASSERT(NewCursorLine < MultilineEdit->LinesPopulated);
            if (NewCursorOffset > MultilineEdit->LineArray[NewCursorLine].LengthInChars) {
                NewCursorOffset = MultilineEdit->LineArray[NewCursorLine].LengthInChars;
            }
        }
    }

    YoriWinMultilineEditClearDesiredDisplayOffset(MultilineEdit);
    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, NewCursorLine);
    if (MultilineEdit->Selection.Active == YoriWinMultilineEditSelectMouseFromTopDown ||
        MultilineEdit->Selection.Active == YoriWinMultilineEditSelectMouseFromBottomUp) {
        YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
    } else {
        YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, TRUE);
    }
    YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
    YoriWinMultilineEditPaint(MultilineEdit);
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
    DWORD NewCursorLine;
    DWORD NewCursorOffset;
    YORI_STRING String;

    if (YoriWinMultilineEditSelectionActive(MultilineEdit)) {
        YoriWinMultilineEditDeleteSelection(MultilineEdit);
    }

    YoriWinMultilineEditClearDesiredDisplayOffset(MultilineEdit);

    YoriLibInitEmptyString(&String);
    String.StartOfString = &Char;
    String.LengthInChars = 1;

    if (!MultilineEdit->InsertMode) {
        if (!YoriWinMultilineEditOverwriteTextRange(MultilineEdit, FALSE, MultilineEdit->CursorLine, MultilineEdit->CursorOffset, &String, &NewCursorLine, &NewCursorOffset)) {
            return FALSE;
        }
    } else {
        if (!YoriWinMultilineEditInsertTextRange(MultilineEdit, FALSE, MultilineEdit->CursorLine, MultilineEdit->CursorOffset, &String, &NewCursorLine, &NewCursorOffset)) {
            return FALSE;
        }
    }

    YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, NewCursorLine);

    return TRUE;

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
        if (MultilineEdit->CursorOffset > 0 ||
            (!MultilineEdit->TraditionalEditNavigation && MultilineEdit->CursorLine > 0)) {
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
            } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
                YoriWinMultilineEditClearSelection(MultilineEdit);
            }
            NewCursorLine = MultilineEdit->CursorLine;
            if (MultilineEdit->CursorOffset == 0) {
                ASSERT(!MultilineEdit->TraditionalEditNavigation);
                NewCursorLine = NewCursorLine - 1;
                NewCursorOffset = MultilineEdit->LineArray[NewCursorLine].LengthInChars;
            } else {
                NewCursorOffset = MultilineEdit->CursorOffset - 1;
            }
            YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, NewCursorLine);
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
            }
            YoriWinMultilineEditClearDesiredDisplayOffset(MultilineEdit);
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_RIGHT) {
        if (MultilineEdit->TraditionalEditNavigation ||
            (MultilineEdit->CursorLine < MultilineEdit->LinesPopulated && MultilineEdit->CursorOffset < MultilineEdit->LineArray[MultilineEdit->CursorLine].LengthInChars) ||
            MultilineEdit->CursorLine + 1 < MultilineEdit->LinesPopulated) {

            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
            } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
                YoriWinMultilineEditClearSelection(MultilineEdit);
            }
            NewCursorLine = MultilineEdit->CursorLine;
            NewCursorOffset = MultilineEdit->CursorOffset + 1;
            if (!MultilineEdit->TraditionalEditNavigation) {
                if ((NewCursorLine < MultilineEdit->LinesPopulated && NewCursorOffset > MultilineEdit->LineArray[NewCursorLine].LengthInChars)) {
                    NewCursorLine = NewCursorLine + 1;
                    NewCursorOffset = 0;
                }
            }
            YoriWinMultilineEditSetCursorLocationInternal(MultilineEdit, NewCursorOffset, NewCursorLine);
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
            }
            YoriWinMultilineEditClearDesiredDisplayOffset(MultilineEdit);
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
            YoriWinMultilineEditPaint(MultilineEdit);
        }
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
            YoriWinMultilineEditClearDesiredDisplayOffset(MultilineEdit);
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
            YoriWinMultilineEditClearDesiredDisplayOffset(MultilineEdit);
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
            YoriWinMultilineEditPopulateDesiredDisplayOffset(MultilineEdit);
            YoriWinMultilineEditFindCursorCharFromDisplayChar(MultilineEdit, NewCursorLine, MultilineEdit->DesiredDisplayCursorOffset, &NewCursorOffset);
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
            YoriWinMultilineEditPopulateDesiredDisplayOffset(MultilineEdit);
            YoriWinMultilineEditFindCursorCharFromDisplayChar(MultilineEdit, NewCursorLine, MultilineEdit->DesiredDisplayCursorOffset, &NewCursorOffset);
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
    } else if (Event->KeyDown.VirtualKeyCode == VK_RETURN) {
        if (!MultilineEdit->ReadOnly && YoriWinMultilineEditAddChar(MultilineEdit, '\r')) {
            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
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
    DWORD ProbeLine;
    DWORD ProbeOffset;
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
    } else if (Event->KeyDown.VirtualKeyCode == VK_LEFT) {
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
        } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
        }
        ProbeLine = MultilineEdit->CursorLine;
        ProbeOffset = MultilineEdit->CursorOffset;
        if (ProbeLine < MultilineEdit->LinesPopulated) {
            while (TRUE) {
                DWORD Index;
                YORI_STRING WhitespaceChars = YORILIB_CONSTANT_STRING(_T(" -\t"));
                PYORI_STRING Line;

                Line = &MultilineEdit->LineArray[ProbeLine];
                Index = ProbeOffset;
                if (Index > Line->LengthInChars) {
                    Index = Line->LengthInChars;
                }
                while(Index > 0 && YoriLibFindLeftMostCharacter(&WhitespaceChars, Line->StartOfString[Index - 1])) {
                    Index--;
                }
                if (Index == 0 && ProbeLine > 0) {
                    ProbeLine--;
                    ProbeOffset = MultilineEdit->LineArray[ProbeLine].LengthInChars;
                    continue;
                }
                while(Index > 0 && (YoriLibFindLeftMostCharacter(&WhitespaceChars, Line->StartOfString[Index - 1]) == NULL)) {
                    Index--;
                }
                MultilineEdit->CursorLine = ProbeLine;
                MultilineEdit->CursorOffset = Index;
                break;
            }
        } else {
            MultilineEdit->CursorOffset = 0;
        }
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
        }
        YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
        YoriWinMultilineEditPaint(MultilineEdit);
    } else if (Event->KeyDown.VirtualKeyCode == VK_RIGHT) {
        BOOLEAN SkipCurrentWord;
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditStartSelectionAtCursor(MultilineEdit, FALSE);
        } else if (YoriWinMultilineEditSelectionActive(&MultilineEdit->Ctrl)) {
            YoriWinMultilineEditClearSelection(MultilineEdit);
        }
        ProbeLine = MultilineEdit->CursorLine;
        ProbeOffset = MultilineEdit->CursorOffset;
        SkipCurrentWord = TRUE;
        if (ProbeLine >= MultilineEdit->LinesPopulated) {
            MultilineEdit->CursorOffset = 0;
        } else {
            while (ProbeLine < MultilineEdit->LinesPopulated) {
                DWORD Index;
                YORI_STRING WhitespaceChars = YORILIB_CONSTANT_STRING(_T(" -\t"));
                PYORI_STRING Line;

                Line = &MultilineEdit->LineArray[ProbeLine];
                Index = ProbeOffset;
                if (Index > Line->LengthInChars) {
                    Index = Line->LengthInChars;
                }
                if (SkipCurrentWord) {
                    while(Index < Line->LengthInChars && YoriLibFindLeftMostCharacter(&WhitespaceChars, Line->StartOfString[Index]) == NULL) {
                        Index++;
                    }
                }
                while(Index < Line->LengthInChars && YoriLibFindLeftMostCharacter(&WhitespaceChars, Line->StartOfString[Index])) {
                    Index++;
                }
                if (Index == Line->LengthInChars && ProbeLine + 1 < MultilineEdit->LinesPopulated) {
                    ProbeLine++;
                    ProbeOffset = 0;
                    SkipCurrentWord = FALSE;
                    continue;
                }
                MultilineEdit->CursorLine = ProbeLine;
                MultilineEdit->CursorOffset = Index;
                break;
            }
        }
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinMultilineEditExtendSelectionToCursor(MultilineEdit);
        }
        YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
        YoriWinMultilineEditPaint(MultilineEdit);
    }

    return Recognized;
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
            YoriWinMultilineEditClearUndo(MultilineEdit);
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
                        Event->KeyDown.Char != '\x1b' &&
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
                    } else if (Event->KeyDown.VirtualKeyCode == 'R') {
                        if (!MultilineEdit->ReadOnly && YoriWinMultilineEditRedo(MultilineEdit)) {
                            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                            YoriWinMultilineEditPaint(MultilineEdit);
                        }
                        return TRUE;
                    } else if (Event->KeyDown.VirtualKeyCode == 'V') {
                        if (!MultilineEdit->ReadOnly && YoriWinMultilineEditPasteText(Ctrl)) {
                            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                            YoriWinMultilineEditPaint(MultilineEdit);
                        }
                        return TRUE;
                    } else if (Event->KeyDown.VirtualKeyCode == 'X') {
                        if (!MultilineEdit->ReadOnly && YoriWinMultilineEditCutSelectedText(Ctrl)) {
                            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                            YoriWinMultilineEditPaint(MultilineEdit);
                        }
                        return TRUE;
                    } else if (Event->KeyDown.VirtualKeyCode == 'Y') {
                        if (!MultilineEdit->ReadOnly && YoriWinMultilineEditDeleteLine(MultilineEdit)) {
                            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                            YoriWinMultilineEditPaint(MultilineEdit);
                        }
                        return TRUE;
                    } else if (Event->KeyDown.VirtualKeyCode == 'Z') {
                        if (!MultilineEdit->ReadOnly && YoriWinMultilineEditUndo(MultilineEdit)) {
                            YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                            YoriWinMultilineEditPaint(MultilineEdit);
                        }
                        return TRUE;
                    }
                }
            } else if (Event->KeyDown.CtrlMask == LEFT_ALT_PRESSED ||
                       Event->KeyDown.CtrlMask == (LEFT_ALT_PRESSED | ENHANCED_KEY)) {
                YoriLibBuildNumericKey(&MultilineEdit->NumericKeyValue, &MultilineEdit->NumericKeyType, Event->KeyDown.VirtualKeyCode, Event->KeyDown.VirtualScanCode);

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

        case YoriWinEventKeyUp:
            if ((Event->KeyUp.CtrlMask & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) == 0 &&
                !MultilineEdit->ReadOnly &&
                (MultilineEdit->NumericKeyValue != 0 ||
                 (Event->KeyUp.VirtualKeyCode == VK_MENU && Event->KeyUp.Char != 0))) {

                DWORD NumericKeyValue;
                TCHAR Char;

                NumericKeyValue = MultilineEdit->NumericKeyValue;
                if (NumericKeyValue == 0) {
                    MultilineEdit->NumericKeyType = YoriLibNumericKeyUnicode;
                    NumericKeyValue = Event->KeyUp.Char;
                }

                YoriLibTranslateNumericKeyToChar(NumericKeyValue, MultilineEdit->NumericKeyType, &Char);
                MultilineEdit->NumericKeyValue = 0;
                MultilineEdit->NumericKeyType = YoriLibNumericKeyAscii;

                YoriWinMultilineEditAddChar(MultilineEdit, Event->KeyDown.Char);
                YoriWinMultilineEditEnsureCursorVisible(MultilineEdit);
                YoriWinMultilineEditPaint(MultilineEdit);
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
        case YoriWinEventMouseDoubleClickInClient:
            YoriWinMultilineEditNotifyDoubleClick(MultilineEdit, Event->MouseDown.Location.X, Event->MouseDown.Location.Y);
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
                YORI_WIN_BOUNDED_COORD ClientPos;
                ClientPos.Left = FALSE;
                ClientPos.Right = FALSE;
                ClientPos.Above = FALSE;
                ClientPos.Below = FALSE;
                ClientPos.Pos.X = Event->MouseMove.Location.X;
                ClientPos.Pos.Y = Event->MouseMove.Location.Y;

                YoriWinMultilineEditScrollForMouseSelect(MultilineEdit, &ClientPos);
            }
            break;
        case YoriWinEventMouseMoveInNonClient:
            if (MultilineEdit->MouseButtonDown) {
                YORI_WIN_BOUNDED_COORD Pos;
                YORI_WIN_BOUNDED_COORD ClientPos;
                Pos.Left = FALSE;
                Pos.Right = FALSE;
                Pos.Above = FALSE;
                Pos.Below = FALSE;
                Pos.Pos.X = Event->MouseMove.Location.X;
                Pos.Pos.Y = Event->MouseMove.Location.Y;

                YoriWinBoundCoordInSubRegion(&Pos, &Ctrl->ClientRect, &ClientPos);

                YoriWinMultilineEditScrollForMouseSelect(MultilineEdit, &ClientPos);
            }
            break;
        case YoriWinEventMouseMoveOutsideWindow:
            if (MultilineEdit->MouseButtonDown) {

                //
                //  Translate any coordinates that are present into client
                //  relative form.  Anything that's out of bounds will stay
                //  that way.
                //

                YORI_WIN_BOUNDED_COORD ClientPos;
                YoriWinBoundCoordInSubRegion(&Event->MouseMoveOutsideWindow.Location, &Ctrl->ClientRect, &ClientPos);
                YoriWinMultilineEditScrollForMouseSelect(MultilineEdit, &ClientPos);
            }
            break;
        case YoriWinEventTimer:
            ASSERT(MultilineEdit->MouseButtonDown);
            ASSERT(MultilineEdit->Selection.Active == YoriWinMultilineEditSelectMouseFromTopDown ||
                   MultilineEdit->Selection.Active == YoriWinMultilineEditSelectMouseFromBottomUp);
            ASSERT(Event->Timer.Timer == MultilineEdit->Timer);
            YoriWinMultilineEditScrollForMouseSelect(MultilineEdit, &MultilineEdit->LastMousePos);
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
 Change the read only state of an existing multiline edit control.

 @param CtrlHandle Pointer to the multiline edit control.

 @param NewReadOnlyState TRUE to indicate the multiline edit control should be
        read only, FALSE if it should be writable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMultilineEditSetReadOnly(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in BOOLEAN NewReadOnlyState
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    PYORI_WIN_CTRL_MULTILINE_EDIT MultilineEdit;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MultilineEdit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MULTILINE_EDIT, Ctrl);
    MultilineEdit->ReadOnly = NewReadOnlyState;

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
    PYORI_WIN_WINDOW_HANDLE TopLevelWindow;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

    Parent = (PYORI_WIN_WINDOW)ParentHandle;

    MultilineEdit = YoriLibReferencedMalloc(sizeof(YORI_WIN_CTRL_MULTILINE_EDIT));
    if (MultilineEdit == NULL) {
        return NULL;
    }

    ZeroMemory(MultilineEdit, sizeof(YORI_WIN_CTRL_MULTILINE_EDIT));

    YoriLibInitializeListHead(&MultilineEdit->Undo);
    YoriLibInitializeListHead(&MultilineEdit->Redo);

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

    if (Style & YORI_WIN_MULTILINE_EDIT_STYLE_READ_ONLY) {
        MultilineEdit->ReadOnly = TRUE;
    }

    MultilineEdit->Ctrl.ClientRect.Top++;
    MultilineEdit->Ctrl.ClientRect.Left++;
    MultilineEdit->Ctrl.ClientRect.Bottom--;
    MultilineEdit->Ctrl.ClientRect.Right--;

    MultilineEdit->InsertMode = TRUE;
    MultilineEdit->TextAttributes = MultilineEdit->Ctrl.DefaultAttributes;
    TopLevelWindow = YoriWinGetTopLevelWindow(Parent);
    WinMgrHandle = YoriWinGetWindowManagerHandle(TopLevelWindow);
    MultilineEdit->SelectedAttributes = YoriWinMgrDefaultColorLookup(WinMgrHandle, YoriWinColorEditSelectedText);
    MultilineEdit->CaptionAttributes = YoriWinMgrDefaultColorLookup(WinMgrHandle, YoriWinColorMultilineCaption);

    MultilineEdit->TabWidth = 4;
    YoriWinMultilineEditClearDesiredDisplayOffset(MultilineEdit);

    YoriWinMultilineEditExpandDirtyRange(MultilineEdit, 0, (DWORD)-1);
    YoriWinMultilineEditPaintNonClient(MultilineEdit);
    YoriWinMultilineEditPaint(MultilineEdit);

    return &MultilineEdit->Ctrl;
}


// vim:sw=4:ts=4:et:
