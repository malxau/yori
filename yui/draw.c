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
#include "resource.h"

/**
 Return the color used to display a flashing window in the taskbar.
 */
COLORREF
YuiGetWindowFlashColor(VOID)
{
    return RGB(255, 192, 96);
}

/**
 Draw an owner draw button.

 @param DrawItemStruct Pointer to a structure describing the button to
        redraw, including its bounds and device context.

 @param Pushed TRUE if the button should have a pushed appearance, FALSE if
        it should have a raised appearance.

 @param Flashing TRUE if the button should have a colored background to
        indicate to the user it is requesting attention.

 @param Icon Optionally specifies an icon handle to display in the button.

 @param Text Pointer to the button text.

 @param CenterText TRUE if the text should be centered within the button.
        FALSE if it should be left aligned.
 */
VOID
YuiDrawButton(
    __in PDRAWITEMSTRUCT DrawItemStruct,
    __in BOOLEAN Pushed,
    __in BOOLEAN Flashing,
    __in_opt HICON Icon,
    __in PYORI_STRING Text,
    __in BOOLEAN CenterText
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

    TextRect.left = DrawItemStruct->rcItem.left + 3;
    TextRect.right = DrawItemStruct->rcItem.right - 5;
    TextRect.top = DrawItemStruct->rcItem.top + 1;
    TextRect.bottom = DrawItemStruct->rcItem.bottom - 1;

    if (Pushed) {
        TextRect.top = TextRect.top + 2;
    } else {
        TextRect.bottom = TextRect.bottom - 2;
    }

    if (Flashing) {
        HBRUSH Brush;
        Brush = CreateSolidBrush(YuiGetWindowFlashColor());
        FillRect(DrawItemStruct->hDC, &TextRect, Brush);
        DeleteObject(Brush);
    }

    TextRect.left = TextRect.left + 2;

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
        Flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;
        if (CenterText) {
            Flags = Flags | DT_CENTER;
        }
        if (Flashing) {
            SetBkColor(DrawItemStruct->hDC, YuiGetWindowFlashColor());
        }
        DrawText(DrawItemStruct->hDC, Text->StartOfString, Text->LengthInChars, &TextRect, Flags);
    }
}

/**
 Determint the size of a an owner draw menu item.

 @param YuiContext Pointer to the application context.

 @param Item Pointer to the item to draw.

 @return TRUE to indicate the item was measured.
 */
BOOL
YuiDrawMeasureMenuItem(
    __in PYUI_CONTEXT YuiContext,
    __in LPMEASUREITEMSTRUCT Item
    )
{
    PYUI_MENU_OWNERDRAW_ITEM ItemContext;

    ItemContext = (PYUI_MENU_OWNERDRAW_ITEM)Item->itemData;

    //
    //  The base width is chosen to fit three menus side by side on an 800x600
    //  display.  Add 5% of any horizontal pixels above 1200 to allow wider
    //  menu items to display.
    //

    Item->itemWidth = 245;
    if (YuiContext->ScreenWidth > 800) {
        Item->itemWidth = Item->itemWidth + (YuiContext->ScreenWidth - 800) / 20;
    }

    if (ItemContext->TallItem) {
        Item->itemHeight = YuiContext->TallMenuHeight;
    } else {
        Item->itemHeight = YuiContext->ShortMenuHeight;
    }

    //
    //  The base height is chosen to fit icons cleanly.  Add 1% of space above
    //  800 pixels just so there's extra room in case the font gets larger.
    //

    if (YuiContext->ScreenHeight > 800) {
        Item->itemHeight = Item->itemHeight + (YuiContext->ScreenHeight - 800) / 100;
    }

    return TRUE;
}

/**
 Draw an owner draw menu item.

 @param YuiContext Pointer to the application context.

 @param Item Pointer to the item to draw.

 @return TRUE to indicate the item was drawn.  On failure there's not much we
         can do.
 */
BOOL
YuiDrawMenuItem(
    __in PYUI_CONTEXT YuiContext,
    __in LPDRAWITEMSTRUCT Item
    )
{
    COLORREF BackColor;
    COLORREF ForeColor;
    HBRUSH Brush;
    RECT TextRect;
    PYUI_MENU_OWNERDRAW_ITEM ItemContext;
    DWORD IconPadding;

    ItemContext = (PYUI_MENU_OWNERDRAW_ITEM)Item->itemData;

    if (Item->itemState & ODS_SELECTED) {
        BackColor = GetSysColor(COLOR_HIGHLIGHT);
        ForeColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
    } else {
        BackColor = GetSysColor(COLOR_MENU);
        ForeColor = GetSysColor(COLOR_MENUTEXT);
    }

    Brush = CreateSolidBrush(BackColor);
    FillRect(Item->hDC, &Item->rcItem, Brush);
    DeleteObject(Brush);

    if (ItemContext->Icon != NULL || (Item->itemState & ODS_CHECKED)) {
        DWORD IconLeft;
        DWORD IconTop;
        DWORD IconWidth;
        DWORD IconHeight;

        if (ItemContext->TallItem) {
            IconPadding = YuiContext->TallIconPadding;
            IconWidth = YuiContext->TallIconWidth;
            IconHeight = YuiContext->TallIconHeight;
        } else {
            IconPadding = YuiContext->ShortIconPadding;
            IconWidth = YuiContext->SmallIconWidth;
            IconHeight = YuiContext->SmallIconHeight;
        }

        IconTop = Item->rcItem.top + (Item->rcItem.bottom - Item->rcItem.top - IconHeight) / 2;
        IconLeft = Item->rcItem.left + IconPadding;

        if (ItemContext->Icon != NULL) {
            DllUser32.pDrawIconEx(Item->hDC, IconLeft, IconTop, ItemContext->Icon->Icon, IconWidth, IconHeight, 0, NULL, DI_NORMAL);
        } else {
            HICON CheckIcon;

            //
            //  Normally it would make sense to keep an icon like this
            //  always loaded.  As of this writing, the check is only
            //  used in the debug menu and off by default, so being
            //  inefficient for this seems acceptable.
            //

            if (DllUser32.pLoadImageW != NULL) {
                CheckIcon = DllUser32.pLoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCE(CHECKEDICON), IMAGE_ICON, IconWidth, IconHeight, 0);
            } else {
                CheckIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(CHECKEDICON));
            }
            if (CheckIcon != NULL) {
                DllUser32.pDrawIconEx(Item->hDC, IconLeft, IconTop, CheckIcon, IconWidth, IconHeight, 0, NULL, DI_NORMAL);
                DestroyIcon(CheckIcon);
            }
        }
    }

    if (ItemContext->TallItem) {
        IconPadding = YuiContext->TallIconPadding * 2 + YuiContext->TallIconWidth;
    } else {
        IconPadding = YuiContext->ShortIconPadding * 2 + YuiContext->SmallIconWidth;
    }

    //
    //  Create a rectangle for text after the icon.  Leave some space on the
    //  right for a menu flyout.  This is done unconditionally just to ensure
    //  all items have the same text region, flyout or not.
    //

    TextRect.left = Item->rcItem.left + IconPadding;
    TextRect.top = Item->rcItem.top;
    TextRect.right = Item->rcItem.right - 20;
    TextRect.bottom = Item->rcItem.bottom;

    SetBkColor(Item->hDC, BackColor);
    SetTextColor(Item->hDC, ForeColor);
    SelectObject(Item->hDC, YuiContext->hFont);
    DrawText(Item->hDC, ItemContext->Text.StartOfString, ItemContext->Text.LengthInChars, &TextRect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    return TRUE;
}

// vim:sw=4:ts=4:et:
