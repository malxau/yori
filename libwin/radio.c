/**
 * @file libwin/radio.c
 *
 * Yori window radio buttons
 *
 * Copyright (c) 2020-2021 Malcolm J. Smith
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
 A structure describing the contents of a radio control.
 */
typedef struct _YORI_WIN_CTRL_RADIO {

    /**
     A common header for all controls
     */
    YORI_WIN_CTRL Ctrl;

    /**
     Pointer to the child label control that renders the text within the
     radio control.
     */
    PYORI_WIN_CTRL Label;

    /**
     A list of radio controls forming part of the same group.  When one is
     selected, the others in this group are unselected.
     */
    YORI_LIST_ENTRY RelatedRadioControls;

    /**
     A function to invoke when the radio is toggled via any mechanism.
     */
    PYORI_WIN_NOTIFY ToggleCallback;

    /**
     The color to display text in when the control has focus.
     */
    WORD SelectedTextAttributes;

    /**
     TRUE if the radio is "pressed" as in the mouse is pressed on the
     radio.  FALSE if the display is regular.
     */
    BOOLEAN PressedAppearance;

    /**
     TRUE if the radio currently has focus, FALSE if another control has
     focus.
     */
    BOOLEAN HasFocus;

    /**
     TRUE if the radio is currently selected.  FALSE if it is not.
     */
    BOOLEAN Selected;

} YORI_WIN_CTRL_RADIO, *PYORI_WIN_CTRL_RADIO;

/**
 Draw the radio button with its current state applied.

 @param Radio Pointer to the radio button to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinRadioPaint(
    __in PYORI_WIN_CTRL_RADIO Radio
    )
{
    WORD TextAttributes;

    TextAttributes = Radio->Ctrl.DefaultAttributes;

    if (Radio->HasFocus || Radio->PressedAppearance) {
        TextAttributes = Radio->SelectedTextAttributes;
    }

    YoriWinSetControlClientCell(&Radio->Ctrl, 0, 0, '(', TextAttributes);
    if (Radio->Selected) {
        PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;
        CONST TCHAR *SelectedChars;
        WinMgrHandle = YoriWinGetWindowManagerHandle(YoriWinGetTopLevelWindow(&Radio->Ctrl));
        SelectedChars = YoriWinGetDrawingCharacters(WinMgrHandle, YoriWinCharsRadioSelection);
        YoriWinSetControlClientCell(&Radio->Ctrl, 1, 0, SelectedChars[0], TextAttributes);
    } else {
        YoriWinSetControlClientCell(&Radio->Ctrl, 1, 0, ' ', TextAttributes);
    }
    YoriWinSetControlClientCell(&Radio->Ctrl, 2, 0, ')', TextAttributes);
    YoriWinSetControlClientCell(&Radio->Ctrl, 3, 0, ' ', TextAttributes);

    YoriWinLabelSetTextAttributes(Radio->Label, TextAttributes);

    return TRUE;
}

/**
 Select a specific radio control, thereby unselecting any related radio
 controls, and re-display any control whose state has changed.

 @param Radio Pointer to the radio control to select.
 */
VOID
YoriWinRadioSelectControl(
    __in PYORI_WIN_CTRL_RADIO Radio
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_CTRL_RADIO RelatedRadio;

    ListEntry = YoriLibGetNextListEntry(&Radio->RelatedRadioControls, NULL);
    while (ListEntry != NULL) {
        RelatedRadio = CONTAINING_RECORD(ListEntry, YORI_WIN_CTRL_RADIO, RelatedRadioControls);
        if (RelatedRadio->Selected) {
            RelatedRadio->Selected = FALSE;
            YoriWinRadioPaint(RelatedRadio);
        }
        ListEntry = YoriLibGetNextListEntry(&Radio->RelatedRadioControls, ListEntry);
    }

    if (!Radio->Selected) {
        Radio->Selected = TRUE;
        YoriWinRadioPaint(Radio);
        if (Radio->ToggleCallback != NULL) {
            Radio->ToggleCallback(&Radio->Ctrl);
        }
    }
}

/**
 Process input events for a radio control.

 @param Ctrl Pointer to the radio control.

 @param Event Pointer to the input event.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate that regular processing
         should continue (although this does not imply that no processing
         has already occurred.)
 */
BOOLEAN
YoriWinRadioEventHandler(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_CTRL_RADIO Radio;
    Radio = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_RADIO, Ctrl);
    switch(Event->EventType) {
        case YoriWinEventKeyDown:
            if (Event->KeyDown.CtrlMask == 0) {
                if ((Event->KeyDown.VirtualKeyCode == VK_RETURN) ||
                    (Event->KeyDown.VirtualKeyCode == VK_SPACE)) {
                    YoriWinRadioSelectControl(Radio);
                }
            }
            break;
        case YoriWinEventExecute:
            YoriWinRadioSelectControl(Radio);
            break;
        case YoriWinEventParentDestroyed:
            if (Radio->Label->NotifyEventFn != NULL) {
                Radio->Label->NotifyEventFn(Radio->Label, Event);
            }
            YoriWinDestroyControl(Ctrl);
            YoriLibDereference(Radio);
            break;
        case YoriWinEventMouseDownInClient:
        case YoriWinEventMouseDownInNonClient:
            Radio->PressedAppearance = TRUE;
            YoriWinRadioPaint(Radio);
            break;
        case YoriWinEventMouseUpInClient:
        case YoriWinEventMouseUpInNonClient:
            Radio->PressedAppearance = FALSE;
            YoriWinRadioSelectControl(Radio);
            break;
        case YoriWinEventMouseUpOutsideWindow:
            Radio->PressedAppearance = FALSE;
            YoriWinRadioPaint(Radio);
            break;
        case YoriWinEventLoseFocus:
            ASSERT(Radio->HasFocus);
            Radio->HasFocus = FALSE;
            YoriWinRadioPaint(Radio);
            break;
        case YoriWinEventGetFocus:
            ASSERT(!Radio->HasFocus);
            Radio->HasFocus = TRUE;
            YoriWinRadioPaint(Radio);
            break;
        case YoriWinEventDisplayAccelerators:
            if (Radio->Label->NotifyEventFn != NULL) {
                Radio->Label->NotifyEventFn(Radio->Label, Event);
            }
            break;
        case YoriWinEventHideAccelerators:
            if (Radio->Label->NotifyEventFn != NULL) {
                Radio->Label->NotifyEventFn(Radio->Label, Event);
            }
            break;
    }

    return FALSE;
}

/**
 Return TRUE if the radio is checked, FALSE if it is not checked.

 @param CtrlHandle Pointer to the radio control.

 @return TRUE if the radio is checked, FALSE if it is not checked.
 */
BOOLEAN
YoriWinRadioIsSelected(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_RADIO Radio;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Radio = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_RADIO, Ctrl);

    return Radio->Selected;
}

/**
 Select this radio control, implicitly deselecting others in the group.

 @param CtrlHandle Pointer to the radio control.
 */
VOID
YoriWinRadioSelect(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_RADIO Radio;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Radio = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_RADIO, Ctrl);

    YoriWinRadioSelectControl(Radio);
}

/**
 Set the size and location of a radio control, and redraw the contents.

 @param CtrlHandle Pointer to the radio to resize or reposition.

 @param CtrlRect Specifies the new size and position of the radio.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinRadioReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    SMALL_RECT LabelRect;
    PYORI_WIN_CTRL_RADIO Radio;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Radio = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_RADIO, Ctrl);

    if (!YoriWinControlReposition(Ctrl, CtrlRect)) {
        return FALSE;
    }

    LabelRect.Left = 4;
    LabelRect.Right = Radio->Ctrl.ClientRect.Right;
    LabelRect.Top = 0;
    LabelRect.Bottom = 0;

    YoriWinLabelReposition(Radio->Label, &LabelRect);

    YoriWinRadioPaint(Radio);
    return TRUE;
}

/**
 Create a radio control and add it to a window.  This is destroyed when
 the window is destroyed.

 @param ParentHandle Pointer to the parent window.

 @param Size Specifies the location and size of the radio.

 @param Caption Specifies the text to display on the radio.

 @param FirstRadioControl Specifies the first radio control in a set of
        controls.  Everything within the set will be deselected implicitly
        once any control is selected.

 @param Style Specifies style flags for the radio.

 @param ToggleCallback A function to invoke when the radio is toggled.

 @return Pointer to the newly created control or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinRadioCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in PYORI_STRING Caption,
    __in_opt PYORI_WIN_CTRL_HANDLE FirstRadioControl,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY ToggleCallback
    )
{
    PYORI_WIN_CTRL_RADIO Radio;
    PYORI_WIN_WINDOW Parent;
    SMALL_RECT LabelRect;
    PYORI_WIN_WINDOW TopLevelWindow;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

    UNREFERENCED_PARAMETER(Style);

    Parent = (PYORI_WIN_WINDOW)ParentHandle;

    Radio = YoriLibReferencedMalloc(sizeof(YORI_WIN_CTRL_RADIO));
    if (Radio == NULL) {
        return NULL;
    }

    ZeroMemory(Radio, sizeof(YORI_WIN_CTRL_RADIO));

    Radio->Ctrl.NotifyEventFn = YoriWinRadioEventHandler;
    if (!YoriWinCreateControl(Parent, Size, TRUE, &Radio->Ctrl)) {
        YoriLibDereference(Radio);
        return NULL;
    }

    YoriLibInitializeListHead(&Radio->RelatedRadioControls);
    if (FirstRadioControl != NULL) {
        PYORI_WIN_CTRL_RADIO FirstCtrl;
        FirstCtrl = CONTAINING_RECORD(FirstRadioControl, YORI_WIN_CTRL_RADIO, Ctrl);
        YoriLibAppendList(&FirstCtrl->RelatedRadioControls, &Radio->RelatedRadioControls);
    }
    Radio->ToggleCallback = ToggleCallback;

    LabelRect.Left = 4;
    LabelRect.Right = Radio->Ctrl.ClientRect.Right;
    LabelRect.Top = 0;
    LabelRect.Bottom = 0;

    Radio->Label = YoriWinLabelCreate(&Radio->Ctrl, &LabelRect, Caption, 0);
    if (Radio->Label == NULL) {
        YoriWinDestroyControl(&Radio->Ctrl);
        YoriLibDereference(Radio);
        return NULL;
    }

    TopLevelWindow = YoriWinGetTopLevelWindow(Parent);
    WinMgrHandle = YoriWinGetWindowManagerHandle(TopLevelWindow);

    Radio->SelectedTextAttributes = YoriWinMgrDefaultColorLookup(WinMgrHandle, YoriWinColorControlSelected);

    //
    //  Once the label has parsed what the accelerator char is, steal it so
    //  the parent window will invoke the radio control when it is used.
    //

    Radio->Ctrl.AcceleratorChar = Radio->Label->AcceleratorChar;

    YoriWinRadioPaint(Radio);

    return &Radio->Ctrl;
}


// vim:sw=4:ts=4:et:
