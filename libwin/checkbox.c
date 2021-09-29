/**
 * @file libwin/checkbox.c
 *
 * Yori window checkbox control
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
 A structure describing the contents of a checkbox control.
 */
typedef struct _YORI_WIN_CTRL_CHECKBOX {

    /**
     A common header for all controls
     */
    YORI_WIN_CTRL Ctrl;

    /**
     Pointer to the child label control that renders the text within the
     checkbox.
     */
    PYORI_WIN_CTRL Label;

    /**
     A function to invoke when the checkbox is toggled via any mechanism.
     */
    PYORI_WIN_NOTIFY ToggleCallback;

    /**
     TRUE if the checkbox is "pressed" as in the mouse is pressed on the
     checkbox.  FALSE if the display is regular.
     */
    BOOLEAN PressedAppearance;

    /**
     TRUE if the checkbox currently has focus, FALSE if another control has
     focus.
     */
    BOOLEAN HasFocus;

    /**
     TRUE if the checkbox is currently checked.  FALSE if it is not.
     */
    BOOLEAN Checked;

} YORI_WIN_CTRL_CHECKBOX, *PYORI_WIN_CTRL_CHECKBOX;

/**
 Draw the checkbox with its current state applied.

 @param Checkbox Pointer to the checkbox to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinCheckboxPaint(
    __in PYORI_WIN_CTRL_CHECKBOX Checkbox
    )
{
    WORD TextAttributes;

    TextAttributes = Checkbox->Ctrl.DefaultAttributes;

    if (Checkbox->HasFocus || Checkbox->PressedAppearance) {
        PYORI_WIN_WINDOW TopLevelWindow;
        PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

        TopLevelWindow = YoriWinGetTopLevelWindow(&Checkbox->Ctrl);
        WinMgrHandle = YoriWinGetWindowManagerHandle(TopLevelWindow);

        TextAttributes = YoriWinMgrDefaultColorLookup(WinMgrHandle, YoriWinColorControlSelected);
    }

    YoriWinSetControlClientCell(&Checkbox->Ctrl, 0, 0, '[', TextAttributes);
    if (Checkbox->Checked) {
        YoriWinSetControlClientCell(&Checkbox->Ctrl, 1, 0, 'X', TextAttributes);
    } else {
        YoriWinSetControlClientCell(&Checkbox->Ctrl, 1, 0, ' ', TextAttributes);
    }
    YoriWinSetControlClientCell(&Checkbox->Ctrl, 2, 0, ']', TextAttributes);
    YoriWinSetControlClientCell(&Checkbox->Ctrl, 3, 0, ' ', TextAttributes);

    YoriWinLabelSetTextAttributes(Checkbox->Label, TextAttributes);

    return TRUE;
}


/**
 Process input events for a checkbox control.

 @param Ctrl Pointer to the checkbox control.

 @param Event Pointer to the input event.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate that regular processing
         should continue (although this does not imply that no processing
         has already occurred.)
 */
BOOLEAN
YoriWinCheckboxEventHandler(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_CTRL_CHECKBOX Checkbox;
    Checkbox = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_CHECKBOX, Ctrl);
    switch(Event->EventType) {
        case YoriWinEventKeyDown:
            if (Event->KeyDown.CtrlMask == 0) {
                if ((Event->KeyDown.VirtualKeyCode == VK_RETURN) ||
                    (Event->KeyDown.VirtualKeyCode == VK_SPACE)) {

                    if (Checkbox->Checked) {
                        Checkbox->Checked = FALSE;
                    } else {
                        Checkbox->Checked = TRUE;
                    }
                    YoriWinCheckboxPaint(Checkbox);
                    if (Checkbox->ToggleCallback != NULL) {
                        Checkbox->ToggleCallback(&Checkbox->Ctrl);
                    }
                }
            }
            break;
        case YoriWinEventExecute:
            if (Checkbox->Checked) {
                Checkbox->Checked = FALSE;
            } else {
                Checkbox->Checked = TRUE;
            }
            YoriWinCheckboxPaint(Checkbox);
            if (Checkbox->ToggleCallback != NULL) {
                Checkbox->ToggleCallback(&Checkbox->Ctrl);
            }
            break;
        case YoriWinEventParentDestroyed:
            if (Checkbox->Label->NotifyEventFn != NULL) {
                Checkbox->Label->NotifyEventFn(Checkbox->Label, Event);
            }
            YoriWinDestroyControl(Ctrl);
            YoriLibDereference(Checkbox);
            break;
        case YoriWinEventMouseDownInClient:
        case YoriWinEventMouseDownInNonClient:
            Checkbox->PressedAppearance = TRUE;
            YoriWinCheckboxPaint(Checkbox);
            break;
        case YoriWinEventMouseUpInClient:
        case YoriWinEventMouseUpInNonClient:
            Checkbox->PressedAppearance = FALSE;
            if (Checkbox->Checked) {
                Checkbox->Checked = FALSE;
            } else {
                Checkbox->Checked = TRUE;
            }
            YoriWinCheckboxPaint(Checkbox);
            if (Checkbox->ToggleCallback != NULL) {
                Checkbox->ToggleCallback(&Checkbox->Ctrl);
            }
            break;
        case YoriWinEventMouseUpOutsideWindow:
            Checkbox->PressedAppearance = FALSE;
            YoriWinCheckboxPaint(Checkbox);
            break;
        case YoriWinEventLoseFocus:
            ASSERT(Checkbox->HasFocus);
            Checkbox->HasFocus = FALSE;
            YoriWinCheckboxPaint(Checkbox);
            break;
        case YoriWinEventGetFocus:
            ASSERT(!Checkbox->HasFocus);
            Checkbox->HasFocus = TRUE;
            YoriWinCheckboxPaint(Checkbox);
            break;
        case YoriWinEventDisplayAccelerators:
            if (Checkbox->Label->NotifyEventFn != NULL) {
                Checkbox->Label->NotifyEventFn(Checkbox->Label, Event);
            }
            break;
        case YoriWinEventHideAccelerators:
            if (Checkbox->Label->NotifyEventFn != NULL) {
                Checkbox->Label->NotifyEventFn(Checkbox->Label, Event);
            }
            break;
    }

    return FALSE;
}

/**
 Return TRUE if the checkbox is checked, FALSE if it is not checked.

 @param CtrlHandle Pointer to the checkbox control.

 @return TRUE if the checkbox is checked, FALSE if it is not checked.
 */
BOOLEAN
YoriWinCheckboxIsChecked(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_CHECKBOX Checkbox;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Checkbox = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_CHECKBOX, Ctrl);

    return Checkbox->Checked;
}

/**
 Set the size and location of a checkbox control, and redraw the contents.

 @param CtrlHandle Pointer to the checkbox to resize or reposition.

 @param CtrlRect Specifies the new size and position of the checkbox.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinCheckboxReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    SMALL_RECT LabelRect;
    PYORI_WIN_CTRL_CHECKBOX Checkbox;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Checkbox = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_CHECKBOX, Ctrl);

    if (!YoriWinControlReposition(Ctrl, CtrlRect)) {
        return FALSE;
    }

    LabelRect.Left = 4;
    LabelRect.Right = Checkbox->Ctrl.ClientRect.Right;
    LabelRect.Top = 0;
    LabelRect.Bottom = 0;

    YoriWinLabelReposition(Checkbox->Label, &LabelRect);

    YoriWinCheckboxPaint(Checkbox);
    return TRUE;
}

/**
 Create a checkbox control and add it to a window.  This is destroyed when
 the window is destroyed.

 @param ParentHandle Pointer to the parent window.

 @param Size Specifies the location and size of the checkbox.

 @param Caption Specifies the text to display on the checkbox.

 @param Style Specifies style flags for the checkbox.

 @param ToggleCallback A function to invoke when the checkbox is toggled.

 @return Pointer to the newly created control or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinCheckboxCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in PYORI_STRING Caption,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY ToggleCallback
    )
{
    PYORI_WIN_CTRL_CHECKBOX Checkbox;
    PYORI_WIN_WINDOW Parent;
    SMALL_RECT LabelRect;

    UNREFERENCED_PARAMETER(Style);

    Parent = (PYORI_WIN_WINDOW)ParentHandle;

    Checkbox = YoriLibReferencedMalloc(sizeof(YORI_WIN_CTRL_CHECKBOX));
    if (Checkbox == NULL) {
        return NULL;
    }

    ZeroMemory(Checkbox, sizeof(YORI_WIN_CTRL_CHECKBOX));

    Checkbox->Ctrl.NotifyEventFn = YoriWinCheckboxEventHandler;
    if (!YoriWinCreateControl(Parent, Size, TRUE, &Checkbox->Ctrl)) {
        YoriLibDereference(Checkbox);
        return NULL;
    }

    Checkbox->ToggleCallback = ToggleCallback;

    LabelRect.Left = 4;
    LabelRect.Right = Checkbox->Ctrl.ClientRect.Right;
    LabelRect.Top = 0;
    LabelRect.Bottom = 0;

    Checkbox->Label = YoriWinLabelCreate(&Checkbox->Ctrl, &LabelRect, Caption, 0);
    if (Checkbox->Label == NULL) {
        YoriWinDestroyControl(&Checkbox->Ctrl);
        YoriLibDereference(Checkbox);
        return NULL;
    }

    //
    //  Once the label has parsed what the accelerator char is, steal it so
    //  the parent window will invoke the checkbox control when it is used.
    //

    Checkbox->Ctrl.AcceleratorChar = Checkbox->Label->AcceleratorChar;

    YoriWinCheckboxPaint(Checkbox);

    return &Checkbox->Ctrl;
}


// vim:sw=4:ts=4:et:
