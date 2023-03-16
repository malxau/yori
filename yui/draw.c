/**
 * @file yui/draw.c
 *
 * Yori shell owner draw button routines
 *
 * Copyright (c) 2023 Malcolm J. Smith
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
#include "yui.h"

/**
 Draw an owner draw button.

 @param DrawItemStruct Pointer to a structure describing the button to
        redraw, including its bounds and device context.

 @param Pushed TRUE if the button should have a pushed appearance, FALSE if
        it should have a raised appearance.

 @param Icon Optionally specifies an icon handle to display in the button.

 @param Text Pointer to the button text.
 */
VOID
YuiDrawButton(
    __in PDRAWITEMSTRUCT DrawItemStruct,
    __in BOOLEAN Pushed,
    __in_opt HICON Icon,
    __in PYORI_STRING Text
    )
{
    DWORD Flags;
    RECT TextRect;
    DWORD IconWidth;
    DWORD IconHeight;
    DWORD IconOffset;
    DWORD ButtonHeight;

    //
    //  Check if the button should be pressed.
    //

    Flags = DFCS_BUTTONPUSH;
    if (Pushed) {
        Flags = Flags | DFCS_PUSHED;
    }

    //
    //  Render the basic button outline.
    //

    DllUser32.pDrawFrameControl(DrawItemStruct->hDC, &DrawItemStruct->rcItem, DFC_BUTTON, Flags);

    //
    //  If the dimensions are too small to draw text, stop now.
    //

    if (DrawItemStruct->rcItem.right - DrawItemStruct->rcItem.left < 12 ||
        DrawItemStruct->rcItem.bottom - DrawItemStruct->rcItem.top < 6) {

        return;
    }

    TextRect.left = DrawItemStruct->rcItem.left + 5;
    TextRect.right = DrawItemStruct->rcItem.right - 5;
    TextRect.top = DrawItemStruct->rcItem.top + 1;
    TextRect.bottom = DrawItemStruct->rcItem.bottom - 1;

    if (Pushed) {
        TextRect.top = TextRect.top + 2;
    } else {
        TextRect.bottom = TextRect.bottom - 2;
    }

    //
    //  If an icon is associated with the window, render it.  If the button
    //  is pushed, move the icon down a pixel.
    //

    IconWidth = 0;
    if (Icon != NULL) {
        IconWidth = GetSystemMetrics(SM_CXSMICON);
        IconHeight = GetSystemMetrics(SM_CYSMICON);

        ButtonHeight = DrawItemStruct->rcItem.bottom - DrawItemStruct->rcItem.top;
        IconOffset = 1;

        if (ButtonHeight > IconHeight) {
            IconOffset = (ButtonHeight - IconHeight) / 2;
        }

        if (Flags & DFCS_PUSHED) {
            IconOffset = IconOffset + 1;
        }
        DllUser32.pDrawIconEx(DrawItemStruct->hDC, 4, IconOffset, Icon, IconWidth, IconHeight, 0, NULL, DI_NORMAL);
        IconWidth = IconWidth + 4;
    }

    //
    //  Render text in the button if there's space for it.
    //

    if (TextRect.right - TextRect.left > (INT)(IconWidth) &&
        TextRect.bottom - TextRect.top > 6) {

        TextRect.left = TextRect.left + IconWidth;
        DrawText(DrawItemStruct->hDC, Text->StartOfString, Text->LengthInChars, &TextRect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    }
}

// vim:sw=4:ts=4:et:
