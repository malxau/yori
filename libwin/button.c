/**
 * @file libwin/button.c
 *
 * Yori window button control
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
 A structure describing the contents of a button control.
 */
typedef struct _YORI_WIN_CTRL_BUTTON {

    /**
     A common header for all controls
     */
    YORI_WIN_CTRL Ctrl;

    /**
     Pointer to the child label control that renders the text within the
     button.
     */
    PYORI_WIN_CTRL Label;

    /**
     A function to invoke when the button is clicked via any mechanism.
     */
    PYORI_WIN_NOTIFY ClickCallback;

    /**
     TRUE if the button is "pressed" as in the mouse is pressed on the
     button.  FALSE if the display is regular.
     */
    BOOLEAN PressedAppearance;

    /**
     TRUE if the button is currently implicitly invoked if the user presses
     enter on the window.
     */
    BOOLEAN EffectiveDefault;

    /**
     TRUE if the button is currently implicitly invoked if the user presses
     escape on the window.
     */
    BOOLEAN EffectiveCancel;

    /**
     TRUE if the button currently has focus, FALSE if another control has
     focus.
     */
    BOOLEAN HasFocus;

    /**
     TRUE if the button should not receive focus.  FALSE for a regular button
     that can receive focus via the tab key.
     */
    BOOLEAN DisableFocus;

} YORI_WIN_CTRL_BUTTON, *PYORI_WIN_CTRL_BUTTON;

/**
 Draw the button with its current state applied.

 @param Button Pointer to the button to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinButtonPaint(
    __in PYORI_WIN_CTRL_BUTTON Button
    )
{
    SMALL_RECT BorderLocation;
    WORD WindowAttributes;
    WORD TextAttributes;
    WORD BorderFlags;

    BorderLocation.Left = 0;
    BorderLocation.Top = 0;
    BorderLocation.Right = (SHORT)(Button->Ctrl.FullRect.Right - Button->Ctrl.FullRect.Left);
    BorderLocation.Bottom = (SHORT)(Button->Ctrl.FullRect.Bottom - Button->Ctrl.FullRect.Top);

    BorderFlags = YORI_WIN_BORDER_TYPE_RAISED;

    if (Button->PressedAppearance) {
        BorderFlags = YORI_WIN_BORDER_TYPE_SUNKEN;
    }

    if (Button->EffectiveDefault || Button->HasFocus) {
        BorderFlags = (WORD)(BorderFlags | YORI_WIN_BORDER_TYPE_DOUBLE);
    }

    WindowAttributes = Button->Ctrl.DefaultAttributes;
    YoriWinDrawBorderOnControl(&Button->Ctrl, &BorderLocation, WindowAttributes, BorderFlags);

    TextAttributes = WindowAttributes;
    if (Button->HasFocus || Button->PressedAppearance) {
        TextAttributes = (WORD)((TextAttributes & 0xF0) >> 4 | (TextAttributes & 0x0F) << 4);
    }

    YoriWinLabelSetTextAttributes(Button->Label, TextAttributes);

    return TRUE;
}


/**
 Process input events for a button control.

 @param Ctrl Pointer to the button control.

 @param Event Pointer to the input event.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate that regular processing
         should continue (although this does not imply that no processing
         has already occurred.)
 */
BOOLEAN
YoriWinButtonEventHandler(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_CTRL_BUTTON Button;
    Button = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_BUTTON, Ctrl);
    switch(Event->EventType) {
        case YoriWinEventKeyDown:
            if (Event->KeyDown.CtrlMask == 0) {
                if ((Event->KeyDown.VirtualKeyCode == VK_RETURN) ||
                    (Event->KeyDown.VirtualKeyCode == VK_SPACE)) {
                    if (Button->ClickCallback != NULL) {
                        Button->ClickCallback(&Button->Ctrl);
                    }
                } else if (Button->EffectiveCancel &&
                           (Event->KeyDown.VirtualKeyCode == VK_ESCAPE)) {
                    if (Button->ClickCallback != NULL) {
                        Button->ClickCallback(&Button->Ctrl);
                    }
                }
            }
            break;
        case YoriWinEventExecute:
            if (Button->ClickCallback != NULL) {
                Button->ClickCallback(&Button->Ctrl);
            }
            break;
        case YoriWinEventParentDestroyed:
            if (Button->Label->NotifyEventFn != NULL) {
                Button->Label->NotifyEventFn(Button->Label, Event);
            }
            YoriWinDestroyControl(Ctrl);
            YoriLibDereference(Button);
            break;
        case YoriWinEventMouseDownInClient:
        case YoriWinEventMouseDownInNonClient:
            Button->PressedAppearance = TRUE;
            YoriWinButtonPaint(Button);
            break;
        case YoriWinEventMouseUpInClient:
        case YoriWinEventMouseUpInNonClient:
            Button->PressedAppearance = FALSE;
            YoriWinButtonPaint(Button);
            if (Button->ClickCallback != NULL) {
                Button->ClickCallback(&Button->Ctrl);
            }
            break;
        case YoriWinEventMouseUpOutsideWindow:
            Button->PressedAppearance = FALSE;
            YoriWinButtonPaint(Button);
            break;
        case YoriWinEventGetEffectiveDefault:
            Button->EffectiveDefault = TRUE;
            YoriWinButtonPaint(Button);
            break;
        case YoriWinEventLoseEffectiveDefault:
            Button->EffectiveDefault = FALSE;
            YoriWinButtonPaint(Button);
            break;
        case YoriWinEventGetEffectiveCancel:
            Button->EffectiveCancel = TRUE;
            break;
        case YoriWinEventLoseEffectiveCancel:
            Button->EffectiveCancel = FALSE;
            break;
        case YoriWinEventLoseFocus:
            ASSERT(!Button->DisableFocus);
            ASSERT(Button->HasFocus);
            Button->HasFocus = FALSE;
            YoriWinRestoreDefaultControl(Button->Ctrl.Parent);
            YoriWinButtonPaint(Button);
            break;
        case YoriWinEventGetFocus:
            ASSERT(!Button->DisableFocus);
            ASSERT(!Button->HasFocus);
            Button->HasFocus = TRUE;
            YoriWinSuppressDefaultControl(Button->Ctrl.Parent);
            YoriWinButtonPaint(Button);
            break;
        case YoriWinEventDisplayAccelerators:
            if (Button->Label->NotifyEventFn != NULL) {
                Button->Label->NotifyEventFn(Button->Label, Event);
            }
            break;
        case YoriWinEventHideAccelerators:
            if (Button->Label->NotifyEventFn != NULL) {
                Button->Label->NotifyEventFn(Button->Label, Event);
            }
            break;
    }

    return FALSE;
}

/**
 Set the size and location of a button control, and redraw the contents.

 @param CtrlHandle Pointer to the button to resize or reposition.

 @param CtrlRect Specifies the new size and position of the button.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinButtonReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    PYORI_WIN_CTRL_BUTTON Button;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    Button = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_BUTTON, Ctrl);

    if (!YoriWinControlReposition(Ctrl, CtrlRect)) {
        return FALSE;
    }

    YoriWinLabelReposition(Button->Label, &Ctrl->ClientRect);

    YoriWinButtonPaint(Button);
    return TRUE;
}

/**
 Create a button control and add it to a window.  This is destroyed when the
 window is destroyed.

 @param ParentHandle Pointer to the parent window.

 @param Size Specifies the location and size of the button.

 @param Caption Specifies the text to display on the button.

 @param Style Specifies style flags for the button including whether it is the
        default or cancel button on a window.

 @param ClickCallback A function to invoke when the button is pressed.

 @return Pointer to the newly created control or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinButtonCreate(
    __in PYORI_WIN_WINDOW_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in PYORI_STRING Caption,
    __in DWORD Style,
    __in_opt PYORI_WIN_NOTIFY ClickCallback
    )
{
    PYORI_WIN_CTRL_BUTTON Button;
    PYORI_WIN_WINDOW Parent;

    Parent = (PYORI_WIN_WINDOW)ParentHandle;

    Button = YoriLibReferencedMalloc(sizeof(YORI_WIN_CTRL_BUTTON));
    if (Button == NULL) {
        return NULL;
    }

    ZeroMemory(Button, sizeof(YORI_WIN_CTRL_BUTTON));
    if (Style & YORI_WIN_BUTTON_STYLE_DISABLE_FOCUS) {
        Button->DisableFocus = TRUE;
    }

    Button->Ctrl.NotifyEventFn = YoriWinButtonEventHandler;
    if (!YoriWinCreateControl(Parent, Size, !Button->DisableFocus, &Button->Ctrl)) {
        YoriLibDereference(Button);
        return NULL;
    }

    Button->Ctrl.ClientRect.Top++;
    Button->Ctrl.ClientRect.Left++;
    Button->Ctrl.ClientRect.Bottom--;
    Button->Ctrl.ClientRect.Right--;

    Button->ClickCallback = ClickCallback;

    Button->Label = YoriWinLabelCreate(&Button->Ctrl, &Button->Ctrl.ClientRect, Caption, YORI_WIN_LABEL_STYLE_VERTICAL_CENTER | YORI_WIN_LABEL_STYLE_CENTER);
    if (Button->Label == NULL) {
        YoriWinDestroyControl(&Button->Ctrl);
        YoriLibDereference(Button);
        return NULL;
    }

    //
    //  Once the label has parsed what the accelerator char is, steal it so
    //  the parent window will invoke the button control when it is used.
    //

    Button->Ctrl.AcceleratorChar = Button->Label->AcceleratorChar;

    YoriWinButtonPaint(Button);

    if (Style & YORI_WIN_BUTTON_STYLE_DEFAULT) {
        YoriWinSetDefaultCtrl(Parent, &Button->Ctrl);
    }

    if (Style & YORI_WIN_BUTTON_STYLE_CANCEL) {
        YoriWinSetCancelCtrl(Parent, &Button->Ctrl);
    }

    return &Button->Ctrl;
}


// vim:sw=4:ts=4:et:
