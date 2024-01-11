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
 Return the color used for the background of a normal button.
 */
COLORREF
YuiGetWindowBackgroundColor(VOID)
{
    return RGB(192, 192, 192);
}

/**
 Return the color used for the background of a pressed button.
 */
COLORREF
YuiGetPressedBackgroundColor(VOID)
{
    return RGB(224, 224, 224);
}

/**
 Return the color used to display a flashing window in the taskbar.
 */
COLORREF
YuiGetWindowFlashColor(VOID)
{
    return RGB(255, 192, 96);
}

/**
 Return the color used for the bright area of 3D controls.
 */
COLORREF
YuiGetHighlightColor(VOID)
{
    return RGB(255, 255, 255);
}

/**
 Return the color used for the dark area of 3D controls.
 */
COLORREF
YuiGetDarkShadowColor(VOID)
{
    return RGB(0, 0, 0);
}

/**
 Return the color used for the shaded area of 3D controls.
 */
COLORREF
YuiGetShadowColor(VOID)
{
    return RGB(128, 128, 128);
}

/**
 Return the color used for menu text.
 */
COLORREF
YuiGetMenuTextColor(VOID)
{
    return RGB(0, 0, 0);
}

/**
 Return the color used for the background of a selected menu item.
 */
COLORREF
YuiGetMenuSelectedBackgroundColor(VOID)
{
    return RGB(0, 0, 160);
}

/**
 Return the color used for the background of a selected menu item.
 */
COLORREF
YuiGetMenuSelectedTextColor(VOID)
{
    return RGB(255, 255, 255);
}

/**
 Calculate the width of a string in the current font, in pixels.

 @param YuiContext Pointer to the application context including font and font
        metrics.

 @param Text Pointer to the string whose width should be calculated.

 @return The width of the string when displayed, in pixels.
 */
DWORD
YuiDrawGetTextWidth(
    __in PYUI_CONTEXT YuiContext,
    __in PYORI_STRING Text
    )
{
    HDC hDC;
    SIZE Size;
    HGDIOBJ OldObject;

    //
    //  For some odd reason, when measuring menu items, we don't get a DC.
    //  Grab one for the taskbar window, set the font, and assume it'll do
    //  approximately the same as the menu window.
    //

    hDC = GetWindowDC(YuiContext->hWnd);
    OldObject = SelectObject(hDC, YuiContext->hFont);
    GetTextExtentPoint32(hDC, Text->StartOfString, Text->LengthInChars, &Size);
    SelectObject(hDC, OldObject);
    ReleaseDC(YuiContext->hWnd, hDC);

    return (DWORD)Size.cx;
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
    COLORREF ButtonBackground;
    COLORREF TopLeft;
    COLORREF SecondTopLeft;
    COLORREF BottomRight;
    COLORREF SecondBottomRight;
    HBRUSH Brush;
    HGDIOBJ OldObject;
    HPEN Pen;

    //
    //  Check if the button should be pressed.
    //

    ButtonBackground = YuiGetWindowBackgroundColor();
    if (Flashing) {
        ButtonBackground = YuiGetWindowFlashColor();
    } else if (Pushed) {
        ButtonBackground = YuiGetPressedBackgroundColor();
    } 

    //
    //  Render the basic button outline.
    //

    Brush = CreateSolidBrush(ButtonBackground);
    SetBkColor(DrawItemStruct->hDC, ButtonBackground);
    FillRect(DrawItemStruct->hDC, &DrawItemStruct->rcItem, Brush);
    DeleteObject(Brush);

    if (Pushed) {
        TopLeft = YuiGetDarkShadowColor();
        BottomRight = YuiGetHighlightColor();
        SecondBottomRight = YuiGetWindowBackgroundColor();
        SecondTopLeft = YuiGetShadowColor();
    } else {
        TopLeft = YuiGetHighlightColor();
        BottomRight = YuiGetDarkShadowColor();
        SecondBottomRight = YuiGetShadowColor();
        SecondTopLeft = ButtonBackground;
    }

    Pen = CreatePen(PS_SOLID, 0, TopLeft);
    OldObject = SelectObject(DrawItemStruct->hDC, Pen);
    MoveToEx(DrawItemStruct->hDC, DrawItemStruct->rcItem.left, DrawItemStruct->rcItem.bottom - 1, NULL);
    LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.left, DrawItemStruct->rcItem.top);
    LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 1, DrawItemStruct->rcItem.top);
    SelectObject(DrawItemStruct->hDC, OldObject);
    DeleteObject(Pen);

    Pen = CreatePen(PS_SOLID, 0, BottomRight);
    SelectObject(DrawItemStruct->hDC, Pen);
    LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 1, DrawItemStruct->rcItem.bottom - 1);
    LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.left, DrawItemStruct->rcItem.bottom - 1);
    SelectObject(DrawItemStruct->hDC, OldObject);
    DeleteObject(Pen);

    Pen = CreatePen(PS_SOLID, 0, SecondBottomRight);
    SelectObject(DrawItemStruct->hDC, Pen);
    MoveToEx(DrawItemStruct->hDC, DrawItemStruct->rcItem.left + 1, DrawItemStruct->rcItem.bottom -2, NULL);
    LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 2, DrawItemStruct->rcItem.bottom - 2);
    LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 2, DrawItemStruct->rcItem.top - 1);
    SelectObject(DrawItemStruct->hDC, OldObject);
    DeleteObject(Pen);

    if (SecondTopLeft != ButtonBackground) {
        Pen = CreatePen(PS_SOLID, 0, SecondTopLeft);
        SelectObject(DrawItemStruct->hDC, Pen);
        MoveToEx(DrawItemStruct->hDC, DrawItemStruct->rcItem.left + 1, DrawItemStruct->rcItem.bottom - 2, NULL);
        LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.left + 1, DrawItemStruct->rcItem.top - 1);
        LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 2, DrawItemStruct->rcItem.top - 1);
        SelectObject(DrawItemStruct->hDC, OldObject);
        DeleteObject(Pen);
    }

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

        if (Pushed) {
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
        DrawText(DrawItemStruct->hDC, Text->StartOfString, Text->LengthInChars, &TextRect, Flags);
    }
}

/**
 Draw a simple, single line 3D box.  This can be raised or sunken.

 @param hDC The device context to render the box into.

 @param Rect The dimensions to render the box.

 @param Pressed If TRUE, display a sunken box.  If FALSE, display a raised
        box.
 */
VOID
YuiDrawThreeDBox(
    __in HDC hDC,
    __in PRECT Rect,
    __in BOOLEAN Pressed
    )
{
    COLORREF Background;
    COLORREF TopLeft;
    COLORREF BottomRight;
    HBRUSH Brush;
    HPEN Pen;
    HGDIOBJ OldObject;

    //
    //  Check if the button should be pressed.
    //

    Background = YuiGetWindowBackgroundColor();

    //
    //  Render the basic button outline.
    //

    Brush = CreateSolidBrush(Background);
    SetBkColor(hDC, Background);
    FillRect(hDC, Rect, Brush);
    DeleteObject(Brush);

    if (Pressed) {
        TopLeft = YuiGetShadowColor();
        BottomRight = YuiGetHighlightColor();
    } else {
        TopLeft = YuiGetHighlightColor();
        BottomRight = YuiGetShadowColor();
    }

    Pen = CreatePen(PS_SOLID, 0, TopLeft);
    OldObject = SelectObject(hDC, Pen);
    MoveToEx(hDC, Rect->left, Rect->bottom - 1, NULL);
    LineTo(hDC, Rect->left, Rect->top);
    LineTo(hDC, Rect->right - 1, Rect->top);
    SelectObject(hDC, OldObject);
    DeleteObject(Pen);

    Pen = CreatePen(PS_SOLID, 0, BottomRight);
    SelectObject(hDC, Pen);
    LineTo(hDC, Rect->right - 1, Rect->bottom - 1);
    LineTo(hDC, Rect->left, Rect->bottom - 1);
    SelectObject(hDC, OldObject);
    DeleteObject(Pen);
}

/**
 Draw an ownerdraw static control.  Currently this routine assumes it will
 have a sunken appearence.

 @param DrawItemStruct Pointer to the struct describing the draw operation.

 @param Text Pointer to text to draw in the control.  Currently this routine
        assumes this text should be vertically and horizontally centered.
 */
VOID
YuiTaskbarDrawStatic(
    __in PDRAWITEMSTRUCT DrawItemStruct,
    __in PYORI_STRING Text
    )
{
    DWORD Flags;
    RECT TextRect;

    YuiDrawThreeDBox(DrawItemStruct->hDC, &DrawItemStruct->rcItem, TRUE);

    //
    //  If the dimensions are too small to draw text, stop now.
    //

    if (DrawItemStruct->rcItem.right - DrawItemStruct->rcItem.left < 12 ||
        DrawItemStruct->rcItem.bottom - DrawItemStruct->rcItem.top < 6) {

        return;
    }

    TextRect.left = DrawItemStruct->rcItem.left + 1;
    TextRect.right = DrawItemStruct->rcItem.right - 1;
    TextRect.top = DrawItemStruct->rcItem.top + 1;
    TextRect.bottom = DrawItemStruct->rcItem.bottom - 1;

    //
    //  Render text if there's space for it.
    //

    if (Text != NULL &&
        Text->LengthInChars > 0 &&
        TextRect.right - TextRect.left > 6 &&
        TextRect.bottom - TextRect.top > 6) {

        Flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_CENTER;
        DrawText(DrawItemStruct->hDC, Text->StartOfString, Text->LengthInChars, &TextRect, Flags);
    }
}

/**
 Draw a popup menu.  Note this is distinct from drawing items within a popup
 menu, and this operation is not well supported by the platform.  This
 function will fill the menu with a chosen color, and draw a raised 3D box
 around it.

 @param DrawItemStruct Pointer to the struct describing a draw operation.
        Note this is for an item within a menu, not the menu itself.  This
        function is responsible for navigating from this to the menu window
        and region to draw.
 */
VOID
YuiDrawEntireMenu(
    __in LPDRAWITEMSTRUCT DrawItemStruct
    )
{
    HWND hwndMenu;
    HDC menuDC;
    RECT windowRect;
    RECT clientRect;

    hwndMenu = WindowFromDC(DrawItemStruct->hDC);
    GetWindowRect(hwndMenu, &windowRect);
    clientRect.left = 0;
    clientRect.top = 0;
    clientRect.right = windowRect.right - windowRect.left;
    clientRect.bottom = windowRect.bottom - windowRect.top;
    menuDC = GetWindowDC(hwndMenu);
    YuiDrawThreeDBox(menuDC, &clientRect, FALSE);
    ReleaseDC(hwndMenu, menuDC);
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

    if (ItemContext->WidthByStringLength) {
        Item->itemWidth = YuiDrawGetTextWidth(YuiContext, &ItemContext->Text) + 
                          YuiContext->SmallStartIconWidth +
                          2 * YuiContext->ShortIconPadding;
        if (ItemContext->AddFlyoutIcon) {
            Item->itemWidth = Item->itemWidth + YUI_FLYOUT_ICON_WIDTH;
        }
    } else {

        //
        //  The base width is chosen to fit three menus side by side on an 800x600
        //  display.  Add 5% of any horizontal pixels above 1200 to allow wider
        //  menu items to display.
        //

        Item->itemWidth = 245;
        if (YuiContext->ScreenWidth > 800) {
            Item->itemWidth = Item->itemWidth + (YuiContext->ScreenWidth - 800) / 20;
        }
    }

    if (ItemContext->TallItem) {
        Item->itemHeight = YuiContext->TallMenuHeight;
    } else if (ItemContext->Text.LengthInChars > 0) {
        Item->itemHeight = YuiContext->ShortMenuHeight;
    } else {
        Item->itemHeight = YuiContext->MenuSeperatorHeight;
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

    if (Item->rcItem.top == 0 &&
        Item->rcItem.left == 0 &&
        Item->itemAction == ODA_DRAWENTIRE) {

        YuiDrawEntireMenu(Item);
    }

    ItemContext = (PYUI_MENU_OWNERDRAW_ITEM)Item->itemData;

    if (Item->itemState & ODS_SELECTED) {
        BackColor = YuiGetMenuSelectedBackgroundColor();
        ForeColor = YuiGetMenuSelectedTextColor();
    } else {
        BackColor = YuiGetWindowBackgroundColor();
        ForeColor = YuiGetMenuTextColor();
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
            IconWidth = YuiContext->SmallStartIconWidth;
            IconHeight = YuiContext->SmallStartIconHeight;
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
        IconPadding = YuiContext->ShortIconPadding * 2 + YuiContext->SmallStartIconWidth;
    }

    //
    //  If the item has no text, assume it is a seperator and render a sunken
    //  3D line.  If it has text, render the text.
    //

    if (ItemContext->Text.LengthInChars == 0) {
        RECT DrawRect;

        DrawRect.left = Item->rcItem.left;
        DrawRect.right = Item->rcItem.right;
        DrawRect.top = (Item->rcItem.bottom - Item->rcItem.top) / 2 + Item->rcItem.top - 1;
        DrawRect.bottom = DrawRect.top + 2;
        YuiDrawThreeDBox(Item->hDC, &DrawRect, TRUE);

    } else {

        HGDIOBJ OldObject;

        TextRect.left = Item->rcItem.left + IconPadding;
        TextRect.top = Item->rcItem.top;
        if (ItemContext->AddFlyoutIcon) {
            TextRect.right = Item->rcItem.right - YUI_FLYOUT_ICON_WIDTH;
        } else {
            TextRect.right = Item->rcItem.right;
        }
        TextRect.bottom = Item->rcItem.bottom;

        SetBkColor(Item->hDC, BackColor);
        SetTextColor(Item->hDC, ForeColor);
        OldObject = SelectObject(Item->hDC, YuiContext->hFont);
        DrawText(Item->hDC, ItemContext->Text.StartOfString, ItemContext->Text.LengthInChars, &TextRect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        SelectObject(Item->hDC, OldObject);
    }
    return TRUE;
}

// vim:sw=4:ts=4:et:
