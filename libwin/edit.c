/**
 * @file libwin/edit.c
 *
 * Yori window edit control
 *
 * Copyright (c) 2019 Malcolm J. Smith
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

} YORI_WIN_CTRL_EDIT, *PYORI_WIN_CTRL_EDIT;

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
    if (Edit->CursorOffset < Edit->DisplayOffset) {
        Edit->DisplayOffset = Edit->CursorOffset;
    } else if (Edit->CursorOffset >= Edit->DisplayOffset + ClientSize.X) {
        Edit->DisplayOffset = Edit->CursorOffset - ClientSize.X + 1;
    }
}

/**
 Draw the edit with its current state applied.

 @param Edit Pointer to the edit to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriWinPaintEdit(
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
    //  Render the text and highlight the accelerator if it's in
    //  scope and highlighting is enabled
    //

    for (CharIndex = 0; CharIndex < DisplayLine.LengthInChars; CharIndex++) {
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
    YoriWinPaintEdit(Edit);
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
 Add a new character and honor the current insert or overwrite state.

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
    DWORD LengthNeeded;

    if (Edit->InsertMode) {
        LengthNeeded = Edit->Text.LengthInChars + 1;
    } else {
        LengthNeeded = Edit->CursorOffset;
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
        Edit->CursorOffset + 1 < Edit->Text.LengthInChars) {

        DWORD CharsToCopy;
        CharsToCopy = Edit->Text.LengthInChars - Edit->CursorOffset;
        if (Edit->CursorOffset < Edit->Text.LengthInChars) {
            memmove(&Edit->Text.StartOfString[Edit->CursorOffset + 1],
                    &Edit->Text.StartOfString[Edit->CursorOffset],
                    CharsToCopy * sizeof(TCHAR));
            Edit->Text.LengthInChars++;
        }
    }

    Edit->Text.StartOfString[Edit->CursorOffset] = Char;
    Edit->CursorOffset++;
    if (Edit->CursorOffset > Edit->Text.LengthInChars) {
        Edit->Text.LengthInChars = Edit->CursorOffset;
    }

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
        if (Edit->CursorOffset > 0) {
            Edit->CursorOffset--;
            YoriWinEditEnsureCursorVisible(Edit);
            YoriWinPaintEdit(Edit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_RIGHT) {
        if (Edit->CursorOffset < Edit->Text.LengthInChars) {
            Edit->CursorOffset++;
            YoriWinEditEnsureCursorVisible(Edit);
            YoriWinPaintEdit(Edit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_HOME) {
        if (Edit->CursorOffset != 0) {
            Edit->CursorOffset = 0;
            YoriWinEditEnsureCursorVisible(Edit);
            YoriWinPaintEdit(Edit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_END) {
        if (Edit->CursorOffset != Edit->Text.LengthInChars) {
            Edit->CursorOffset = Edit->Text.LengthInChars;
            YoriWinEditEnsureCursorVisible(Edit);
            YoriWinPaintEdit(Edit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_INSERT) {
        if (!Edit->ReadOnly) {
            YoriWinEditToggleInsert(Edit);
        }
        Recognized = TRUE;
    } else if (Event->KeyDown.VirtualKeyCode == VK_BACK) {

        if (!Edit->ReadOnly && YoriWinEditBackspace(Edit)) {
            YoriWinEditEnsureCursorVisible(Edit);
            YoriWinPaintEdit(Edit);
        }
        Recognized = TRUE;

    } else if (Event->KeyDown.VirtualKeyCode == VK_DELETE) {

        if (!Edit->ReadOnly && YoriWinEditDelete(Edit)) {
            YoriWinEditEnsureCursorVisible(Edit);
            YoriWinPaintEdit(Edit);
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
            YoriWinPaintEdit(Edit);
            break;
        case YoriWinEventLoseFocus:
            YoriWinEditSetCursorVisible(Edit, FALSE);
            YoriWinPaintEdit(Edit);
            break;
        case YoriWinEventParentDestroyed:
            YoriWinEditSetCursorVisible(Edit, FALSE);
            YoriLibFreeStringContents(&Edit->Text);
            YoriWinDestroyControl(Ctrl);
            YoriLibDereference(Edit);
            break;
        case YoriWinEventKeyDown:

            // MSFIX: Handle all of the AltGr cases.  Have to be careful to
            // not catch pure Alt cases that should be accelerators
            //
            // TODO: Selection
            // TODO: Copy/paste
            if (Event->KeyDown.CtrlMask == 0 ||
                Event->KeyDown.CtrlMask == SHIFT_PRESSED) {

                ASSERT(Edit->CursorOffset <= Edit->Text.LengthInChars);

                if (!YoriWinEditProcessPossiblyEnhancedKey(Edit, Event)) {
                    if (Event->KeyDown.Char != '\0' &&
                        Event->KeyDown.Char != '\r' &&
                        Event->KeyDown.Char != '\n' &&
                        Event->KeyDown.Char != '\t') {

                        if (!Edit->ReadOnly) {
                            YoriWinEditAddChar(Edit, Event->KeyDown.Char);
                            YoriWinEditEnsureCursorVisible(Edit);
                            YoriWinPaintEdit(Edit);
                        }
                    }
                }
            } else if (Event->KeyDown.CtrlMask == ENHANCED_KEY) {
                YoriWinEditProcessPossiblyEnhancedKey(Edit, Event);
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
    YoriWinPaintEdit(Edit);
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
YoriWinCreateEdit(
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
        YoriWinSetControlNonClientCell(&Edit->Ctrl, 0, 0, '[', Edit->TextAttributes);
        YoriWinSetControlNonClientCell(&Edit->Ctrl, Edit->Ctrl.ClientRect.Right, 0, ']', Edit->TextAttributes);
        Edit->Ctrl.ClientRect.Left++;
        Edit->Ctrl.ClientRect.Right--;
    } else {
        YoriWinDrawBorderOnControl(&Edit->Ctrl, &Edit->Ctrl.ClientRect, Edit->TextAttributes, YORI_WIN_BORDER_TYPE_SUNKEN);
        Edit->Ctrl.ClientRect.Top++;
        Edit->Ctrl.ClientRect.Left++;
        Edit->Ctrl.ClientRect.Bottom--;
        Edit->Ctrl.ClientRect.Right--;
    }

    YoriWinPaintEdit(Edit);

    return &Edit->Ctrl;
}


// vim:sw=4:ts=4:et:
