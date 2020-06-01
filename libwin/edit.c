/**
 * @file libwin/edit.c
 *
 * Yori window edit control
 *
 * Copyright (c) 2019-2020 Malcolm J. Smith
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
 Specifies legitimate values for horizontal text alignment within the
 edit.
 */
typedef enum _YORI_WIN_TEXT_ALIGNMENT {
    YoriWinTextAlignLeft = 0,
    YoriWinTextAlignCenter = 1,
    YoriWinTextAlignRight = 2
} YORI_WIN_TEXT_ALIGNMENT;

/**
 Information about the selection region within a edit control.
 */
typedef struct _YORI_WIN_EDIT_SELECT {

    /**
     Indicates if a selection is currently active, and if so, what caused the
     activation.
     */
    enum {
        YoriWinEditSelectNotActive = 0,
        YoriWinEditSelectKeyboardFromTopDown = 1,
        YoriWinEditSelectKeyboardFromBottomUp = 2,
        YoriWinEditSelectMouseFromTopDown = 3,
        YoriWinEditSelectMouseFromBottomUp = 4,
        YoriWinEditSelectMouseComplete = 5
    } Active;

    /**
     Specifies the character offset containing the beginning of the selection.
     */
    DWORD FirstCharOffset;

    /**
     Specifies the index after the final character selected on the final line.
     This value can therefore be zero through to the length of string
     inclusive.
     */
    DWORD LastCharOffset;

} YORI_WIN_EDIT_SELECT, *PYORI_WIN_EDIT_SELECT;

/**
 A structure describing the contents of a edit control.
 */
typedef struct _YORI_WIN_CTRL_EDIT {

    /**
     A common header for all controls
     */
    YORI_WIN_CTRL Ctrl;

    /**
     The text contents of the edit control
     */
    YORI_STRING Text;

    /**
     Specifies the selection state of text within the edit control.
     This is encapsulated into a structure purely for readability.
     */
    YORI_WIN_EDIT_SELECT Selection;

    /**
     Specifies if the text should be rendered to the left, center, or right of
     each line horizontally.
     */
    YORI_WIN_TEXT_ALIGNMENT TextAlign;

    /**
     Specifies the offset within the text string to display.
     */
    DWORD DisplayOffset;

    /**
     Specifies the offset within the text string to insert text.
     */
    DWORD CursorOffset;

    /**
     The attributes to display text in.
     */
    WORD TextAttributes;

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
     If TRUE, the control has focus, indicating the cursor should be
     displayed.
     */
    BOOLEAN HasFocus;

    /**
     TRUE if events indicate that the left mouse button is currently held
     down.  FALSE if the mouse button is released.
     */
    BOOLEAN MouseButtonDown;

} YORI_WIN_CTRL_EDIT, *PYORI_WIN_CTRL_EDIT;

/**
 Return TRUE if a selection region is active, or FALSE if no selection is
 currently active.

 @param CtrlHandle Pointer to the edit control.

 @return TRUE if a selection region is active, or FALSE if no selection is
         currently active.
 */
BOOLEAN
YoriWinEditSelectionActive(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_EDIT Edit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);

    if (Edit->Selection.Active == YoriWinEditSelectNotActive) {
        return FALSE;
    }
    return TRUE;
}

/**
 Adjust the first character to display in the control to ensure the current
 user cursor is visible somewhere within the control.

 @param Edit Pointer to the edit control.
 */
VOID
YoriWinEditEnsureCursorVisible(
    __in PYORI_WIN_CTRL_EDIT Edit
    )
{
    COORD ClientSize;
    YoriWinGetControlClientSize(&Edit->Ctrl, &ClientSize);

    //
    //  We can't guarantee that the entire selection is on the screen,
    //  but if it's a single line selection that would fit, try to
    //  take that into account.  Do this first so if the cursor would
    //  move the viewport, that takes precedence.
    //

    if (YoriWinEditSelectionActive(Edit)) {
        DWORD StartSelection;
        DWORD EndSelection;

        StartSelection = Edit->Selection.FirstCharOffset;
        EndSelection = Edit->Selection.LastCharOffset;

        if (StartSelection < Edit->DisplayOffset) {
            Edit->DisplayOffset = StartSelection;
        } else if (EndSelection >= Edit->DisplayOffset + ClientSize.X) {
            Edit->DisplayOffset = EndSelection - ClientSize.X + 1;
        }
    }
    if (Edit->CursorOffset < Edit->DisplayOffset) {
        Edit->DisplayOffset = Edit->CursorOffset;
    } else if (Edit->CursorOffset >= Edit->DisplayOffset + ClientSize.X) {
        Edit->DisplayOffset = Edit->CursorOffset - ClientSize.X + 1;
    }
}

/**
 Draw the border around the edit control.

 @param Edit Pointer to the edit to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditPaintNonClient(
    __in PYORI_WIN_CTRL_EDIT Edit
    )
{
    SMALL_RECT NonClientRect;
    WORD Height;

    NonClientRect.Left = 0;
    NonClientRect.Top = 0;
    NonClientRect.Right = (SHORT)(Edit->Ctrl.FullRect.Right - Edit->Ctrl.FullRect.Left);
    NonClientRect.Bottom = (SHORT)(Edit->Ctrl.FullRect.Bottom - Edit->Ctrl.FullRect.Top);

    Height = (WORD)(NonClientRect.Bottom + 1);

    if (Height == 1) {
        YoriWinSetControlNonClientCell(&Edit->Ctrl, 0, 0, '[', Edit->TextAttributes);
        YoriWinSetControlNonClientCell(&Edit->Ctrl, NonClientRect.Right, 0, ']', Edit->TextAttributes);
    } else {
        YoriWinDrawBorderOnControl(&Edit->Ctrl, &NonClientRect, Edit->TextAttributes, YORI_WIN_BORDER_TYPE_SUNKEN);
    }

    return TRUE;
}


/**
 Draw the edit with its current state applied.

 @param Edit Pointer to the edit to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditPaint(
    __in PYORI_WIN_CTRL_EDIT Edit
    )
{
    WORD WinAttributes;
    WORD TextAttributes;
    WORD StartColumn;
    WORD CellIndex;
    WORD CharIndex;
    WORD DisplayCursorOffset;
    COORD ClientSize;
    YORI_STRING DisplayLine;
    BOOLEAN SelectionActive;

    YoriWinGetControlClientSize(&Edit->Ctrl, &ClientSize);
    WinAttributes = Edit->Ctrl.DefaultAttributes;
    TextAttributes = Edit->TextAttributes;

    YoriLibInitEmptyString(&DisplayLine);

    //
    //  This should be ensured by YoriWinEditEnsureCursorVisible.
    //

    ASSERT(Edit->CursorOffset >= Edit->DisplayOffset && Edit->CursorOffset < Edit->DisplayOffset + ClientSize.X);

    DisplayCursorOffset = (WORD)(Edit->CursorOffset - Edit->DisplayOffset);

    if (Edit->DisplayOffset < Edit->Text.LengthInChars) {
        DisplayLine.StartOfString = Edit->Text.StartOfString + Edit->DisplayOffset;
        DisplayLine.LengthInChars = Edit->Text.LengthInChars - Edit->DisplayOffset;
        if (DisplayLine.LengthInChars > (WORD)ClientSize.X) {
            DisplayLine.LengthInChars = (WORD)ClientSize.X;
        }
    }

    //
    //  Calculate the starting cell for the text from the left based
    //  on alignment specification
    //
    StartColumn = 0;
    if (Edit->TextAlign == YoriWinTextAlignRight) {
        StartColumn = (WORD)(ClientSize.X - DisplayLine.LengthInChars);
    } else if (Edit->TextAlign == YoriWinTextAlignCenter) {
        StartColumn = (WORD)((ClientSize.X - DisplayLine.LengthInChars) / 2);
    }

    //
    //  Pad area before the text
    //

    for (CellIndex = 0; CellIndex < StartColumn; CellIndex++) {
        YoriWinSetControlClientCell(&Edit->Ctrl, CellIndex, 0, ' ', WinAttributes);
    }

    //
    //  Render the text
    //

    SelectionActive = YoriWinEditSelectionActive(&Edit->Ctrl);
    for (CharIndex = 0; CharIndex < DisplayLine.LengthInChars; CharIndex++) {
        if (SelectionActive) {
            TextAttributes = Edit->TextAttributes;
            if (CharIndex + Edit->DisplayOffset >= Edit->Selection.FirstCharOffset &&
                CharIndex + Edit->DisplayOffset < Edit->Selection.LastCharOffset) {

                TextAttributes = (WORD)(((TextAttributes & 0xF) << 4) | ((TextAttributes & 0xF0) >> 4));
            }
        }
        YoriWinSetControlClientCell(&Edit->Ctrl, (WORD)(StartColumn + CharIndex), 0, DisplayLine.StartOfString[CharIndex], TextAttributes);
    }

    //
    //  Pad the area after the text
    //

    for (CellIndex = (WORD)(StartColumn + DisplayLine.LengthInChars); CellIndex < (DWORD)(ClientSize.X); CellIndex++) {
        YoriWinSetControlClientCell(&Edit->Ctrl, CellIndex, 0, ' ', WinAttributes);
    }

    if (Edit->HasFocus) {
        YoriWinSetControlClientCursorLocation(&Edit->Ctrl, (WORD)(StartColumn + DisplayCursorOffset), 0);
    }

    return TRUE;
}

/**
 Perform debug only checks to see that the selection state follows whatever
 rules are currently defined for it.

 @param Edit Pointer to the edit control specifying the selection state.
 */
VOID
YoriWinEditCheckSelectionState(
    __in PYORI_WIN_CTRL_EDIT Edit
    )
{
    PYORI_WIN_EDIT_SELECT Selection;
    Selection = &Edit->Selection;

    if (Selection->Active  == YoriWinEditSelectNotActive) {
        return;
    }
    if (Selection->Active == YoriWinEditSelectMouseFromTopDown ||
        Selection->Active == YoriWinEditSelectMouseFromBottomUp) {
        ASSERT(Selection->FirstCharOffset <= Selection->LastCharOffset);
    } else {
        ASSERT(Selection->FirstCharOffset < Selection->LastCharOffset);
    }
    ASSERT(Selection->FirstCharOffset <= Edit->Text.LengthInChars);
    ASSERT(Selection->LastCharOffset <= Edit->Text.LengthInChars);
}

/**
 If a selection is currently active, delete all text in the selection.

 @param CtrlHandle Pointer to the edit control containing the selection and
        contents of the buffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditDeleteSelection(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_EDIT Edit;
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_EDIT_SELECT Selection;
    DWORD CharsToCopy;
    DWORD CharsToDelete;
    PYORI_STRING Line;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);

    if (!YoriWinEditSelectionActive(&Edit->Ctrl)) {
        return TRUE;
    }

    Selection = &Edit->Selection;
    Line = &Edit->Text;

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

    Edit->CursorOffset = Selection->FirstCharOffset;
    Selection->Active = YoriWinEditSelectNotActive;
    return TRUE;
}


/**
 Build a single continuous string covering the selected range in an edit
 control.

 @param CtrlHandle Pointer to the edit control containing the selection and
        contents of the buffer.

 @param SelectedText On successful completion, populated with a newly
        allocated buffer containing the selected text.  The caller is
        expected to free this buffer with @ref YoriLibFreeStringContents .

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditGetSelectedText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __out PYORI_STRING SelectedText
    )
{
    PYORI_WIN_CTRL_EDIT Edit;
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_EDIT_SELECT Selection;
    DWORD CharsInRange;
    PYORI_STRING Line;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);

    if (!YoriWinEditSelectionActive(CtrlHandle)) {
        YoriLibInitEmptyString(SelectedText);
        return TRUE;
    }

    Selection = &Edit->Selection;
    Line = &Edit->Text;

    if (Selection->FirstCharOffset >= Selection->LastCharOffset) {
        YoriLibInitEmptyString(SelectedText);
        return TRUE;
    }

    CharsInRange = Selection->LastCharOffset - Selection->FirstCharOffset;

    if (!YoriLibAllocateString(SelectedText, CharsInRange + 1)) {
        return FALSE;
    }

    memcpy(SelectedText->StartOfString, &Line->StartOfString[Selection->FirstCharOffset], CharsInRange * sizeof(TCHAR));
    SelectedText->LengthInChars = CharsInRange;
    SelectedText->StartOfString[CharsInRange] = '\0';
    return TRUE;
}

/**
 Start a new selection from the current cursor location if no selection is
 currently active.  If one is active, this call is ignored.

 @param Edit Pointer to the edit control that describes the selection and
        cursor location.

 @param Mouse If TRUE, the selection is being initiated by mouse operations.
        If FALSE, the selection is being initiated by keyboard operations.
 */
VOID
YoriWinEditStartSelectionAtCursor(
    __in PYORI_WIN_CTRL_EDIT Edit,
    __in BOOLEAN Mouse
    )
{
    PYORI_WIN_EDIT_SELECT Selection;

    Selection = &Edit->Selection;

    //
    //  If a mouse selection is active and keyboard selection is requested
    //  or vice versa, clear the existing selection.
    //

    if (Mouse) {
        if (Selection->Active == YoriWinEditSelectKeyboardFromTopDown ||
            Selection->Active == YoriWinEditSelectKeyboardFromBottomUp ||
            Selection->Active == YoriWinEditSelectMouseComplete) {

            Selection->Active = YoriWinEditSelectNotActive;
        }
    } else {
        if (Selection->Active == YoriWinEditSelectMouseFromTopDown ||
            Selection->Active == YoriWinEditSelectMouseFromBottomUp ||
            Selection->Active == YoriWinEditSelectMouseComplete) {

            Selection->Active = YoriWinEditSelectNotActive;
        }
    }

    //
    //  If no selection is active, activate it.
    //

    if (Selection->Active == YoriWinEditSelectNotActive) {
        DWORD EffectiveCursorOffset;
        EffectiveCursorOffset = Edit->CursorOffset;
        if (EffectiveCursorOffset > Edit->Text.LengthInChars) {
            EffectiveCursorOffset = Edit->Text.LengthInChars;
        }

        if (Mouse) {
            Selection->Active = YoriWinEditSelectMouseFromTopDown;
        } else {
            Selection->Active = YoriWinEditSelectKeyboardFromTopDown;
        }

        Selection->FirstCharOffset = EffectiveCursorOffset;
        Selection->LastCharOffset = EffectiveCursorOffset;
    }
}

/**
 Extend the current selection to the location of the cursor.

 @param Edit Pointer to the edit control that describes the current selection
        and cursor location.
 */
VOID
YoriWinEditExtendSelectionToCursor(
    __in PYORI_WIN_CTRL_EDIT Edit
    )
{
    DWORD AnchorOffset;
    DWORD EffectiveCursorOffset;
    BOOLEAN MouseSelection = FALSE;

    PYORI_WIN_EDIT_SELECT Selection;

    AnchorOffset = 0;

    Selection = &Edit->Selection;

    //
    //  Find the place where the selection started from the user's point of
    //  view.  This might be the beginning or end of the selection in terms
    //  of its location in the buffer.
    //

    ASSERT(YoriWinEditSelectionActive(&Edit->Ctrl));
    if (Selection->Active == YoriWinEditSelectKeyboardFromTopDown ||
        Selection->Active == YoriWinEditSelectMouseFromTopDown) {

        AnchorOffset = Selection->FirstCharOffset;

    } else if (Selection->Active == YoriWinEditSelectKeyboardFromBottomUp ||
               Selection->Active == YoriWinEditSelectMouseFromBottomUp) {

        AnchorOffset = Selection->LastCharOffset;

    } else {
        return;
    }

    if (Selection->Active == YoriWinEditSelectMouseFromTopDown ||
        Selection->Active == YoriWinEditSelectMouseFromBottomUp) {

        MouseSelection = TRUE;
    }

    EffectiveCursorOffset = Edit->CursorOffset;

    if (EffectiveCursorOffset > Edit->Text.LengthInChars) {
        EffectiveCursorOffset = Edit->Text.LengthInChars;
    }

    if (EffectiveCursorOffset < AnchorOffset) {
        if (MouseSelection) {
            Selection->Active = YoriWinEditSelectMouseFromBottomUp;
        } else {
            Selection->Active = YoriWinEditSelectKeyboardFromBottomUp;
        }
        Selection->LastCharOffset = AnchorOffset;
        Selection->FirstCharOffset = EffectiveCursorOffset;
    } else if (EffectiveCursorOffset > AnchorOffset) {
        if (MouseSelection) {
            Selection->Active = YoriWinEditSelectMouseFromTopDown;
        } else {
            Selection->Active = YoriWinEditSelectKeyboardFromTopDown;
        }
        Selection->FirstCharOffset = AnchorOffset;
        Selection->LastCharOffset = EffectiveCursorOffset;
    } else if (!MouseSelection) {
        Selection->Active = YoriWinEditSelectNotActive;
    } else {
        Selection->LastCharOffset = AnchorOffset;
        Selection->FirstCharOffset = AnchorOffset;
    }

    YoriWinEditCheckSelectionState(Edit);
}

/**
 Set the selection range within an edit control to an explicitly provided
 range.

 @param CtrlHandle Pointer to the edit control.

 @param StartOffset Specifies the character offset within the line to the
        beginning of the selection.

 @param EndOffset Specifies the character offset within the line to the
        end of the selection.
 */
VOID
YoriWinEditSetSelectionRange(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in DWORD StartOffset,
    __in DWORD EndOffset
    )
{
    PYORI_WIN_CTRL_EDIT Edit;

    Edit = (PYORI_WIN_CTRL_EDIT)CtrlHandle;

    Edit->Selection.Active = YoriWinEditSelectNotActive;
    Edit->CursorOffset = StartOffset;
    YoriWinEditStartSelectionAtCursor(Edit, FALSE);
    Edit->CursorOffset = EndOffset;
    YoriWinEditExtendSelectionToCursor(Edit);
    YoriWinEditEnsureCursorVisible(Edit);
    YoriWinEditPaint(Edit);
}

/**
 Set the text attributes within the edit to a value and repaint the control.
 Note this refers to the attributes of the text within the edit, not the
 entire edit area.

 @param CtrlHandle Pointer to a edit control.

 @param TextAttributes The new attributes to use.
 */
VOID
YoriWinEditSetTextAttributes(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in WORD TextAttributes
    )
{
    PYORI_WIN_CTRL_EDIT Edit;
    PYORI_WIN_CTRL Ctrl;
    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);
    Edit->TextAttributes = TextAttributes;
    YoriWinEditPaint(Edit);
}

/**
 Make the cursor visible or hidden as necessary and display it with the
 size implied by the current insert mode.

 @param Edit Pointer to the edit control.

 @param Visible TRUE to display the cursor, FALSE to hide it.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditSetCursorVisible(
    __in PYORI_WIN_CTRL_EDIT Edit,
    __in BOOLEAN Visible
    )
{
    WORD Percent;
    if (Edit->InsertMode) {
        Percent = 20;
    } else {
        Percent = 50;
    }

    Edit->HasFocus = Visible;
    YoriWinSetControlCursorState(&Edit->Ctrl, Visible, Percent);
    return TRUE;
}

/**
 Toggle the insert state of the control.  If new keystrokes would previously
 insert new characters, future characters overwrite existing characters, and
 vice versa.  The cursor shape will be updated to reflect the new state.

 @param Edit Pointer to the edit control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditToggleInsert(
    __in PYORI_WIN_CTRL_EDIT Edit
    )
{
    if (Edit->InsertMode) {
        Edit->InsertMode = FALSE;
    } else {
        Edit->InsertMode = TRUE;
    }
    YoriWinEditSetCursorVisible(Edit, TRUE);

    return TRUE;
}


/**
 Add new text and honor the current insert or overwrite state.

 @param Edit Pointer to the edit control, indicating the current cursor
        location and insert state.

 @param Text The string to include.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditInsertTextAtCursor(
    __in PYORI_WIN_CTRL_EDIT Edit,
    __in PYORI_STRING Text
    )
{
    DWORD LengthNeeded;

    if (YoriWinEditSelectionActive(Edit)) {
        YoriWinEditDeleteSelection(Edit);
    }

    if (Text->LengthInChars == 0) {
        return TRUE;
    }

    if (Edit->InsertMode) {
        LengthNeeded = Edit->Text.LengthInChars + Text->LengthInChars;
    } else {
        LengthNeeded = Edit->CursorOffset + Text->LengthInChars - 1;
    }

    if (LengthNeeded + 1 >= Edit->Text.LengthAllocated) {
        DWORD LengthToAllocate;
        LengthToAllocate = Edit->Text.LengthAllocated * 2 + 80;
        if (LengthNeeded >= LengthToAllocate) {
            LengthToAllocate = LengthNeeded + 1;
        }
        if (!YoriLibReallocateString(&Edit->Text, LengthToAllocate)) {
            return FALSE;
        }
    }

    if (Edit->InsertMode &&
        Edit->CursorOffset < Edit->Text.LengthInChars) {

        DWORD CharsToCopy;
        CharsToCopy = Edit->Text.LengthInChars - Edit->CursorOffset;
        memmove(&Edit->Text.StartOfString[Edit->CursorOffset + Text->LengthInChars],
                &Edit->Text.StartOfString[Edit->CursorOffset],
                CharsToCopy * sizeof(TCHAR));
        Edit->Text.LengthInChars = Edit->Text.LengthInChars + Text->LengthInChars;
    }

    memcpy(&Edit->Text.StartOfString[Edit->CursorOffset], Text->StartOfString, Text->LengthInChars * sizeof(TCHAR));
    Edit->CursorOffset = Edit->CursorOffset + Text->LengthInChars;
    if (Edit->CursorOffset > Edit->Text.LengthInChars) {
        Edit->Text.LengthInChars = Edit->CursorOffset;
    }

    return TRUE;
}

/**
 Add a single character and honor the current insert or overwrite state.

 @param Edit Pointer to the edit control, indicating the current cursor
        location and insert state.

 @param Char The character to include.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditAddChar(
    __in PYORI_WIN_CTRL_EDIT Edit,
    __in TCHAR Char
    )
{
    YORI_STRING String;

    YoriLibInitEmptyString(&String);
    String.StartOfString = &Char;
    String.LengthInChars = 1;

    return YoriWinEditInsertTextAtCursor(Edit, &String);
}

/**
 Add the currently selected text to the clipboard and delete it from the
 buffer.

 @param CtrlHandle Pointer to the edit control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditCutSelectedText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_EDIT Edit;
    PYORI_WIN_CTRL Ctrl;
    YORI_STRING Text;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);
    YoriLibInitEmptyString(&Text);

    if (!YoriWinEditGetSelectedText(CtrlHandle, &Text)) {
        return FALSE;
    }

    if (!YoriLibCopyText(&Text)) {
        YoriLibFreeStringContents(&Text);
        return FALSE;
    }

    YoriLibFreeStringContents(&Text);
    YoriWinEditDeleteSelection(CtrlHandle);
    return TRUE;
}

/**
 Add the currently selected text to the clipboard and clear the selection.

 @param CtrlHandle Pointer to the edit control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditCopySelectedText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL_EDIT Edit;
    PYORI_WIN_CTRL Ctrl;
    YORI_STRING Text;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);
    YoriLibInitEmptyString(&Text);

    if (!YoriWinEditGetSelectedText(CtrlHandle, &Text)) {
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
YoriWinEditPasteText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    YORI_STRING Text;
    PYORI_WIN_CTRL_EDIT Edit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);
    YoriLibInitEmptyString(&Text);

    if (YoriWinEditSelectionActive(CtrlHandle)) {
        YoriWinEditDeleteSelection(CtrlHandle);
    }

    if (!YoriLibPasteText(&Text)) {
        return FALSE;
    }
    if (!YoriWinEditInsertTextAtCursor(CtrlHandle, &Text)) {
        YoriLibFreeStringContents(&Text);
        return FALSE;
    }

    YoriLibFreeStringContents(&Text);
    return TRUE;
}


/**
 Delete the character before the cursor and move later characters into
 position.

 @param Edit Pointer to the edit control, indicating the current cursor
        location.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditBackspace(
    __in PYORI_WIN_CTRL_EDIT Edit
    )
{
    DWORD CharsToCopy;

    if (YoriWinEditSelectionActive(&Edit->Ctrl)) {
        return YoriWinEditDeleteSelection(&Edit->Ctrl);
    }

    if (Edit->CursorOffset == 0) {
        return FALSE;
    }

    CharsToCopy = Edit->Text.LengthInChars - Edit->CursorOffset;
    memmove(&Edit->Text.StartOfString[Edit->CursorOffset - 1],
            &Edit->Text.StartOfString[Edit->CursorOffset],
            CharsToCopy * sizeof(TCHAR));
    Edit->Text.LengthInChars--;
    Edit->CursorOffset--;
    return TRUE;
}

/**
 Delete the character at the cursor and move later characters into position.

 @param Edit Pointer to the edit control, indicating the current cursor
        location.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditDelete(
    __in PYORI_WIN_CTRL_EDIT Edit
    )
{
    DWORD CharsToCopy;

    if (YoriWinEditSelectionActive(&Edit->Ctrl)) {
        return YoriWinEditDeleteSelection(&Edit->Ctrl);
    }

    if (Edit->CursorOffset == Edit->Text.LengthInChars) {
        return FALSE;
    }

    CharsToCopy = Edit->Text.LengthInChars - Edit->CursorOffset - 1;
    if (CharsToCopy > 0) {
        memmove(&Edit->Text.StartOfString[Edit->CursorOffset],
                &Edit->Text.StartOfString[Edit->CursorOffset + 1],
                CharsToCopy * sizeof(TCHAR));
    }
    Edit->Text.LengthInChars--;
    return TRUE;
}

/**
 Process a key that may be an enhanced key.  Some of these keys can be either
 enhanced or non-enhanced.

 @param Edit Pointer to the edit control, indicating the current cursor
        location.

 @param Event Pointer to the event describing the state of the key being
        pressed.

 @return TRUE to indicate the key has been processed, FALSE if it is an
         unknown key.
 */
BOOLEAN
YoriWinEditProcessPossiblyEnhancedKey(
    __in PYORI_WIN_CTRL_EDIT Edit,
    __in PYORI_WIN_EVENT Event
    )
{
    BOOLEAN Recognized;
    Recognized = FALSE;

    if (Event->KeyDown.VirtualKeyCode == VK_LEFT) {
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinEditStartSelectionAtCursor(Edit, FALSE);
        } else if (YoriWinEditSelectionActive(&Edit->Ctrl)) {
            Edit->Selection.Active = YoriWinEditSelectNotActive;
        }
        if (Edit->CursorOffset > 0) {
            Edit->CursorOffset--;
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinEditExtendSelectionToCursor(Edit);
            }
            YoriWinEditEnsureCursorVisible(Edit);
        }
        YoriWinEditPaint(Edit);
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_RIGHT) {
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinEditStartSelectionAtCursor(Edit, FALSE);
        } else if (YoriWinEditSelectionActive(&Edit->Ctrl)) {
            Edit->Selection.Active = YoriWinEditSelectNotActive;
        }
        if (Edit->CursorOffset < Edit->Text.LengthInChars) {
            Edit->CursorOffset++;
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinEditExtendSelectionToCursor(Edit);
            }
            YoriWinEditEnsureCursorVisible(Edit);
        }
        YoriWinEditPaint(Edit);
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_HOME) {
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinEditStartSelectionAtCursor(Edit, FALSE);
        } else if (YoriWinEditSelectionActive(&Edit->Ctrl)) {
            Edit->Selection.Active = YoriWinEditSelectNotActive;
        }
        if (Edit->CursorOffset != 0) {
            Edit->CursorOffset = 0;
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinEditExtendSelectionToCursor(Edit);
            }
            YoriWinEditEnsureCursorVisible(Edit);
        }
        YoriWinEditPaint(Edit);
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_END) {
        if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
            YoriWinEditStartSelectionAtCursor(Edit, FALSE);
        } else if (YoriWinEditSelectionActive(&Edit->Ctrl)) {
            Edit->Selection.Active = YoriWinEditSelectNotActive;
        }
        if (Edit->CursorOffset != Edit->Text.LengthInChars) {
            Edit->CursorOffset = Edit->Text.LengthInChars;
            if (Event->KeyDown.CtrlMask & SHIFT_PRESSED) {
                YoriWinEditExtendSelectionToCursor(Edit);
            }
            YoriWinEditEnsureCursorVisible(Edit);
        }
        YoriWinEditPaint(Edit);
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_INSERT) {
        if (!Edit->ReadOnly) {
            YoriWinEditToggleInsert(Edit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_BACK) {

        if (!Edit->ReadOnly && YoriWinEditBackspace(Edit)) {
            YoriWinEditEnsureCursorVisible(Edit);
            YoriWinEditPaint(Edit);
        }
        Recognized = TRUE;

    } else if (Event->KeyDown.VirtualKeyCode == VK_DELETE) {

        if (!Edit->ReadOnly && YoriWinEditDelete(Edit)) {
            YoriWinEditEnsureCursorVisible(Edit);
            YoriWinEditPaint(Edit);
        }
        Recognized = TRUE;
    }

    return Recognized;
}

/**
 Process input events for a edit control.

 @param Ctrl Pointer to the edit control.

 @param Event Pointer to the input event.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate that regular processing
         should continue (although this does not imply that no processing
         has already occurred.)
 */
BOOLEAN
YoriWinEditEventHandler(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_CTRL_EDIT Edit;

    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);
    switch(Event->EventType) {
        case YoriWinEventGetFocus:
            YoriWinEditSetCursorVisible(Edit, TRUE);
            YoriWinEditPaint(Edit);
            break;
        case YoriWinEventLoseFocus:
            YoriWinEditSetCursorVisible(Edit, FALSE);
            YoriWinEditPaint(Edit);
            break;
        case YoriWinEventParentDestroyed:
            YoriWinEditSetCursorVisible(Edit, FALSE);
            YoriLibFreeStringContents(&Edit->Text);
            YoriWinDestroyControl(Ctrl);
            YoriLibDereference(Edit);
            break;
        case YoriWinEventKeyDown:

            //
            // This code is trying to handle the AltGr cases while not
            // handling pure right Alt which would normally be an accelerator.
            //
            // MSFIX: Copy/paste
            //

            if (Event->KeyDown.CtrlMask == 0 ||
                Event->KeyDown.CtrlMask == SHIFT_PRESSED ||
                Event->KeyDown.CtrlMask == (LEFT_CTRL_PRESSED | LEFT_ALT_PRESSED) ||
                Event->KeyDown.CtrlMask == (LEFT_CTRL_PRESSED | LEFT_ALT_PRESSED | SHIFT_PRESSED) ||
                Event->KeyDown.CtrlMask == (LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED) ||
                Event->KeyDown.CtrlMask == (LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | SHIFT_PRESSED)) {

                ASSERT(Edit->CursorOffset <= Edit->Text.LengthInChars);

                if (!YoriWinEditProcessPossiblyEnhancedKey(Edit, Event)) {
                    if (Event->KeyDown.Char != '\0' &&
                        Event->KeyDown.Char != '\r' &&
                        Event->KeyDown.Char != '\n' &&
                        Event->KeyDown.Char != '\t') {

                        if (!Edit->ReadOnly) {
                            YoriWinEditAddChar(Edit, Event->KeyDown.Char);
                            YoriWinEditEnsureCursorVisible(Edit);
                            YoriWinEditPaint(Edit);
                        }
                    }
                }
            } else if (Event->KeyDown.CtrlMask == LEFT_CTRL_PRESSED ||
                       Event->KeyDown.CtrlMask == RIGHT_CTRL_PRESSED) {

                if (Event->KeyDown.VirtualKeyCode == 'C') {
                    if (YoriWinEditCopySelectedText(Ctrl)) {
                        Edit->Selection.Active = YoriWinEditSelectNotActive;
                        YoriWinEditEnsureCursorVisible(Edit);
                        YoriWinEditPaint(Edit);
                    }
                    return TRUE;
                } else if (Event->KeyDown.VirtualKeyCode == 'V') {
                    if (YoriWinEditPasteText(Ctrl)) {
                        YoriWinEditEnsureCursorVisible(Edit);
                        YoriWinEditPaint(Edit);
                    }
                    return TRUE;
                } else if (Event->KeyDown.VirtualKeyCode == 'X') {
                    if (YoriWinEditCutSelectedText(Ctrl)) {
                        YoriWinEditEnsureCursorVisible(Edit);
                        YoriWinEditPaint(Edit);
                    }
                    return TRUE;
                }
            } else if (Event->KeyDown.CtrlMask == ENHANCED_KEY ||
                       Event->KeyDown.CtrlMask == (ENHANCED_KEY | SHIFT_PRESSED)) {
                YoriWinEditProcessPossiblyEnhancedKey(Edit, Event);
            }
            break;
        case YoriWinEventMouseDownInClient:
            {
                DWORD ClickOffset;
                ClickOffset = Edit->DisplayOffset + Event->MouseDown.Location.X;
                if (ClickOffset > Edit->Text.LengthInChars) {
                    ClickOffset = Edit->Text.LengthInChars;
                }

                Edit->CursorOffset = ClickOffset;
                Edit->Selection.Active = YoriWinEditSelectNotActive;
                YoriWinEditStartSelectionAtCursor(Edit, TRUE);
                Edit->MouseButtonDown = TRUE;

                YoriWinEditEnsureCursorVisible(Edit);
                YoriWinEditPaint(Edit);
            }
            break;

        case YoriWinEventMouseMoveInClient:
            if (Edit->MouseButtonDown) {
                DWORD ClickOffset;
                ClickOffset = Edit->DisplayOffset + Event->MouseMove.Location.X;
                if (ClickOffset > Edit->Text.LengthInChars) {
                    ClickOffset = Edit->Text.LengthInChars;
                }
                Edit->CursorOffset = ClickOffset;
                if (Edit->Selection.Active == YoriWinEditSelectMouseFromTopDown ||
                    Edit->Selection.Active == YoriWinEditSelectMouseFromBottomUp) {

                    YoriWinEditExtendSelectionToCursor(Edit);
                } else {
                    YoriWinEditStartSelectionAtCursor(Edit, TRUE);
                }

                YoriWinEditEnsureCursorVisible(Edit);
                YoriWinEditPaint(Edit);
            }
            break;

        case YoriWinEventMouseUpInClient:
        case YoriWinEventMouseUpInNonClient:
        case YoriWinEventMouseUpOutsideWindow:

            if (Edit->Selection.Active == YoriWinEditSelectMouseFromTopDown ||
                Edit->Selection.Active == YoriWinEditSelectMouseFromBottomUp) {

                Edit->Selection.Active = YoriWinEditSelectMouseComplete;
                Edit->MouseButtonDown = FALSE;
            }
            break;
    }

    return FALSE;
}

/**
 Return the text that has been entered into an edit control.

 @param CtrlHandle Pointer to the edit control.

 @param Text On input, an initialized Yori string.  On output, populated with
        the string contents of the edit control.  Note this string can be
        reallocated within this routine.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditGetText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __inout PYORI_STRING Text
    )
{
    PYORI_WIN_CTRL_EDIT Edit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;

    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);
    if (Text->LengthAllocated < Edit->Text.LengthInChars + 1) {
        YORI_STRING NewString;
        if (!YoriLibAllocateString(&NewString, Edit->Text.LengthInChars + 1)) {
            return FALSE;
        }

        YoriLibFreeStringContents(Text);
        memcpy(Text, &NewString, sizeof(YORI_STRING));
    }

    memcpy(Text->StartOfString, Edit->Text.StartOfString, Edit->Text.LengthInChars * sizeof(TCHAR));
    Text->LengthInChars = Edit->Text.LengthInChars;
    Text->StartOfString[Text->LengthInChars] = '\0';
    return TRUE;
}

/**
 Set the text that should be entered within an edit control.

 @param CtrlHandle Pointer to the edit control.

 @param Text Points to the text to populate the edit control with.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditSetText(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_STRING Text
    )
{
    PYORI_WIN_CTRL_EDIT Edit;
    PYORI_WIN_CTRL Ctrl;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;

    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);
    if (Text->LengthInChars + 1 > Edit->Text.LengthAllocated) {
        YORI_STRING NewString;
        if (!YoriLibAllocateString(&NewString, Text->LengthInChars + 1)) {
            return FALSE;
        }

        YoriLibFreeStringContents(&Edit->Text);
        memcpy(&Edit->Text, &NewString, sizeof(YORI_STRING));
    }

    memcpy(Edit->Text.StartOfString, Text->StartOfString, Text->LengthInChars * sizeof(TCHAR));
    Edit->Text.LengthInChars = Text->LengthInChars;
    Edit->Text.StartOfString[Edit->Text.LengthInChars] = '\0';
    Edit->CursorOffset = Edit->Text.LengthInChars;
    YoriWinEditEnsureCursorVisible(Edit);
    YoriWinEditPaint(Edit);
    return TRUE;
}

/**
 Set the size and location of an edit control, and redraw the contents.

 @param CtrlHandle Pointer to the edit to resize or reposition.

 @param CtrlRect Specifies the new size and position of the edit.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinEditReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    PYORI_WIN_CTRL_EDIT Edit;
    WORD Height;

    Height = (WORD)(CtrlRect->Bottom - CtrlRect->Top + 1);
    if (Height != 3 && Height != 1) {
        return FALSE;
    }

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Edit = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_EDIT, Ctrl);

    if (!YoriWinControlReposition(Ctrl, CtrlRect)) {
        return FALSE;
    }

    YoriWinEditPaintNonClient(Edit);
    YoriWinEditPaint(Edit);
    return TRUE;
}


/**
 Create a edit control and add it to a window.  This is destroyed when the
 window is destroyed.

 @param ParentHandle Pointer to the parent control.

 @param Size Specifies the location and size of the edit control.

 @param InitialText Specifies the initial text in the edit control.

 @param Style Specifies style flags for the edit control.

 @return Pointer to the newly created control or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinEditCreate(
    __in PYORI_WIN_CTRL_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in PYORI_STRING InitialText,
    __in DWORD Style
    )
{
    PYORI_WIN_CTRL_EDIT Edit;
    PYORI_WIN_CTRL Parent;
    WORD Height;

    Height = (WORD)(Size->Bottom - Size->Top + 1);
    if (Height != 3 && Height != 1) {
        return NULL;
    }

    Parent = (PYORI_WIN_CTRL)ParentHandle;

    Edit = YoriLibReferencedMalloc(sizeof(YORI_WIN_CTRL_EDIT));
    if (Edit == NULL) {
        return NULL;
    }

    ZeroMemory(Edit, sizeof(YORI_WIN_CTRL_EDIT));

    if (Style & YORI_WIN_EDIT_STYLE_RIGHT_ALIGN) {
        Edit->TextAlign = YoriWinTextAlignRight;
    } else if (Style & YORI_WIN_EDIT_STYLE_CENTER) {
        Edit->TextAlign = YoriWinTextAlignCenter;
    }

    if (Style & YORI_WIN_EDIT_STYLE_READ_ONLY) {
        Edit->ReadOnly = TRUE;
    }

    if (!YoriLibAllocateString(&Edit->Text, InitialText->LengthInChars + 1)) {
        YoriLibDereference(Edit);
        return NULL;
    }

    memcpy(Edit->Text.StartOfString, InitialText->StartOfString, InitialText->LengthInChars * sizeof(TCHAR));
    Edit->Text.LengthInChars = InitialText->LengthInChars;
    Edit->Text.StartOfString[Edit->Text.LengthInChars] = '\0';

    Edit->Ctrl.NotifyEventFn = YoriWinEditEventHandler;
    if (!YoriWinCreateControl(Parent, Size, TRUE, &Edit->Ctrl)) {
        YoriLibFreeStringContents(&Edit->Text);
        YoriLibDereference(Edit);
        return NULL;
    }

    Edit->TextAttributes = Edit->Ctrl.DefaultAttributes;
    Edit->InsertMode = TRUE;

    if (Parent->Parent != NULL) {
        Edit->Ctrl.RelativeToParentClient = FALSE;
    }

    if (Height == 1) {
        Edit->Ctrl.ClientRect.Left++;
        Edit->Ctrl.ClientRect.Right--;
    } else {
        Edit->Ctrl.ClientRect.Top++;
        Edit->Ctrl.ClientRect.Left++;
        Edit->Ctrl.ClientRect.Bottom--;
        Edit->Ctrl.ClientRect.Right--;
    }

    YoriWinEditPaintNonClient(Edit);
    YoriWinEditPaint(Edit);

    return &Edit->Ctrl;
}


// vim:sw=4:ts=4:et:
