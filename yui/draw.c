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

 @param YuiMonitor Pointer to the monitor context including font and font
        metrics.

 @param UseBold TRUE to get the width using the bold font; FALSE to get the
        width of the nonbold font.

 @param Text Pointer to the string whose width should be calculated.

 @return The width of the string when displayed, in pixels.
 */
DWORD
YuiDrawGetTextWidth(
    __in PYUI_MONITOR YuiMonitor,
    __in BOOLEAN UseBold,
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

    hDC = GetWindowDC(YuiMonitor->hWndTaskbar);
    if (UseBold) {
        OldObject = SelectObject(hDC, YuiMonitor->hBoldFont);
    } else {
        OldObject = SelectObject(hDC, YuiMonitor->hFont);
    }
    GetTextExtentPoint32(hDC, Text->StartOfString, Text->LengthInChars, &Size);
    SelectObject(hDC, OldObject);
    ReleaseDC(YuiMonitor->hWndTaskbar, hDC);

    return (DWORD)Size.cx;
}

/**
 Draw an owner draw button.

 @param YuiMonitor Pointer to the monitor context.

 @param DrawItemStruct Pointer to a structure describing the button to
        redraw, including its bounds and device context.

 @param Pushed TRUE if the button should have a pushed appearance, FALSE if
        it should have a raised appearance.

 @param Flashing TRUE if the button should have a colored background to
        indicate to the user it is requesting attention.

 @param Disabled TRUE if the button should have a disabled appearence; FALSE
        for an enabled appearance.

 @param Icon Optionally specifies an icon handle to display in the button.

 @param Text Pointer to the button text.

 @param CenterText TRUE if the text should be centered within the button.
        FALSE if it should be left aligned.
 */
VOID
YuiDrawButton(
    __in PYUI_MONITOR YuiMonitor,
    __in PDRAWITEMSTRUCT DrawItemStruct,
    __in BOOLEAN Pushed,
    __in BOOLEAN Flashing,
    __in BOOLEAN Disabled,
    __in_opt HICON Icon,
    __in PYORI_STRING Text,
    __in BOOLEAN CenterText
    )
{
    DWORD Flags;
    RECT TextRect;
    WORD IconWidth;
    WORD IconHeight;
    WORD IconTopOffset;
    WORD IconLeftOffset;
    DWORD ButtonHeight;
    COLORREF ButtonBackground;
    COLORREF TopLeft;
    COLORREF SecondTopLeft;
    COLORREF BottomRight;
    COLORREF SecondBottomRight;
    COLORREF TextColor;
    HBRUSH Brush;
    HGDIOBJ OldObject;
    HPEN TopLeftPen;
    HPEN BottomRightPen;
    HPEN SecondTopLeftPen;
    HPEN SecondBottomRightPen;
    WORD Index;
    WORD LineCount;
    PYUI_CONTEXT YuiContext;

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

    TopLeftPen = CreatePen(PS_SOLID, 0, TopLeft);
    BottomRightPen = CreatePen(PS_SOLID, 0, BottomRight);
    SecondBottomRightPen = CreatePen(PS_SOLID, 0, SecondBottomRight);

    if (Pushed) {
        SecondTopLeftPen = CreatePen(PS_SOLID, 0, SecondTopLeft);

        OldObject = SelectObject(DrawItemStruct->hDC, TopLeftPen);
        MoveToEx(DrawItemStruct->hDC, DrawItemStruct->rcItem.left, DrawItemStruct->rcItem.bottom - 1, NULL);
        LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.left, DrawItemStruct->rcItem.top);
        LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 1, DrawItemStruct->rcItem.top);

        SelectObject(DrawItemStruct->hDC, BottomRightPen);

        for (Index = 0; Index < YuiMonitor->ControlBorderWidth; Index++) {
            MoveToEx(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 1 - Index, DrawItemStruct->rcItem.top + Index, NULL);
            LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 1 - Index, DrawItemStruct->rcItem.bottom - 1 - Index);
            LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.left + Index, DrawItemStruct->rcItem.bottom - 1 - Index);
        }

        //
        //  This draws one more pixel left due to the line count reduction
        //  below.
        //

        SelectObject(DrawItemStruct->hDC, SecondBottomRightPen);
        MoveToEx(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 1 - Index, DrawItemStruct->rcItem.top + Index, NULL);
        LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 1 - Index, DrawItemStruct->rcItem.bottom - 1 - Index);
        LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.left + Index - 1, DrawItemStruct->rcItem.bottom - 1 - Index);

        //
        //  One black plus two dark grey is a really strong look.  Dial this
        //  back by one line for purely cosmetic reasons.
        //

        LineCount = (WORD)(YuiMonitor->ControlBorderWidth - 1);
        if (LineCount == 0) {
            LineCount = 1;
        }

        SelectObject(DrawItemStruct->hDC, SecondTopLeftPen);
        for (Index = 0; Index < LineCount; Index++) {
            MoveToEx(DrawItemStruct->hDC, DrawItemStruct->rcItem.left + 1 + Index, DrawItemStruct->rcItem.bottom - 2 - Index, NULL);
            LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.left + 1 + Index, DrawItemStruct->rcItem.top + 1 + Index);
            LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 2 - Index, DrawItemStruct->rcItem.top + 1 + Index);
        }
        SelectObject(DrawItemStruct->hDC, OldObject);
        DeleteObject(SecondTopLeftPen);

    } else {
        OldObject = SelectObject(DrawItemStruct->hDC, TopLeftPen);
        for (Index = 0; Index < YuiMonitor->ControlBorderWidth; Index++) {
            MoveToEx(DrawItemStruct->hDC, DrawItemStruct->rcItem.left + Index, DrawItemStruct->rcItem.bottom - 1 - Index, NULL);
            LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.left + Index, DrawItemStruct->rcItem.top + Index);
            LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 1 - Index, DrawItemStruct->rcItem.top + Index);
        }

        SelectObject(DrawItemStruct->hDC, BottomRightPen);

        MoveToEx(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 1, DrawItemStruct->rcItem.top, NULL);
        LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 1, DrawItemStruct->rcItem.bottom - 1);
        LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.left, DrawItemStruct->rcItem.bottom - 1);

        SelectObject(DrawItemStruct->hDC, SecondBottomRightPen);
        for (Index = 0; Index < YuiMonitor->ControlBorderWidth; Index++) {
            MoveToEx(DrawItemStruct->hDC, DrawItemStruct->rcItem.left + 1 + Index, DrawItemStruct->rcItem.bottom - 2 - Index, NULL);
            LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 2 - Index, DrawItemStruct->rcItem.bottom - 2 - Index);
            LineTo(DrawItemStruct->hDC, DrawItemStruct->rcItem.right - 2 - Index, DrawItemStruct->rcItem.top - 1 + Index);
        }
        SelectObject(DrawItemStruct->hDC, OldObject);
    }

    DeleteObject(TopLeftPen);
    DeleteObject(BottomRightPen);
    DeleteObject(SecondBottomRightPen);

    //
    //  If the dimensions are too small to draw text, stop now.
    //

    if (DrawItemStruct->rcItem.right - DrawItemStruct->rcItem.left < 12 ||
        DrawItemStruct->rcItem.bottom - DrawItemStruct->rcItem.top < 6) {

        return;
    }

    TextRect.left = DrawItemStruct->rcItem.left + YuiMonitor->ControlBorderWidth + 2;
    TextRect.right = DrawItemStruct->rcItem.right - YuiMonitor->ControlBorderWidth - 4;
    TextRect.top = DrawItemStruct->rcItem.top + YuiMonitor->ControlBorderWidth;
    TextRect.bottom = DrawItemStruct->rcItem.bottom - YuiMonitor->ControlBorderWidth;

    if (Pushed) {
        TextRect.top = TextRect.top + 2;
    } else {
        TextRect.top = TextRect.top + YuiMonitor->ExtraPixelsAboveText;
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

    YuiContext = YuiMonitor->YuiContext;
    IconWidth = 0;
    if (Icon != NULL) {
        IconWidth = YuiContext->SmallTaskbarIconWidth;
        IconHeight = YuiContext->SmallTaskbarIconHeight;

        ButtonHeight = DrawItemStruct->rcItem.bottom - DrawItemStruct->rcItem.top;
        IconTopOffset = 1;

        if (ButtonHeight > IconHeight) {
            IconTopOffset = (WORD)((ButtonHeight - IconHeight) / 2);
        }

        if (Pushed) {
            IconTopOffset = (WORD)(IconTopOffset + 1);
        }
        IconLeftOffset = (WORD)(YuiMonitor->ControlBorderWidth * 2 + 2);
        DllUser32.pDrawIconEx(DrawItemStruct->hDC, IconLeftOffset, IconTopOffset, Icon, IconWidth, IconHeight, 0, NULL, DI_NORMAL);
        IconWidth = (WORD)(IconWidth + IconLeftOffset);
    }

    //
    //  Render text in the button if there's space for it.
    //

    if (TextRect.right - TextRect.left > (INT)(IconWidth) &&
        TextRect.bottom - TextRect.top > 6) {

        TextColor = YuiGetMenuTextColor();

        TextRect.left = TextRect.left + IconWidth;
        Flags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;
        if (CenterText) {
            Flags = Flags | DT_CENTER;
        }
        if (Disabled) {

            DWORD MoveAmt;

            MoveAmt = 1;
            SetBkMode(DrawItemStruct->hDC, TRANSPARENT);

            //
            //  Make the text region 1px smaller.  Both renders will still
            //  fill the specified size, but each render gets a smaller area.
            //

            TextRect.bottom = TextRect.bottom - MoveAmt;
            TextRect.right = TextRect.right - MoveAmt;

            //
            //  For the first pass, move everything down and right by 1px
            //

            TextRect.left = TextRect.left + MoveAmt;
            TextRect.top = TextRect.top + MoveAmt;
            TextRect.right = TextRect.right + MoveAmt;
            TextRect.bottom = TextRect.bottom + MoveAmt;

            TextColor = YuiGetHighlightColor();
            SetTextColor(DrawItemStruct->hDC, TextColor);
            DrawText(DrawItemStruct->hDC, Text->StartOfString, Text->LengthInChars, &TextRect, Flags);
            TextColor = YuiGetShadowColor();

            //
            //  For the second pass, move everything up and left by 1px
            //

            TextRect.left = TextRect.left - MoveAmt;
            TextRect.top = TextRect.top - MoveAmt;
            TextRect.right = TextRect.right - MoveAmt;
            TextRect.bottom = TextRect.bottom - MoveAmt;
        }
        SetTextColor(DrawItemStruct->hDC, TextColor);
        DrawText(DrawItemStruct->hDC, Text->StartOfString, Text->LengthInChars, &TextRect, Flags);
    }
}

/**
 Fill a rectangle with the window background color.

 @param hDC The device context to fill.

 @param Rect The dimensions to fill.
 */
VOID
YuiDrawWindowBackground(
    __in HDC hDC,
    __in PRECT Rect
    )
{
    COLORREF Background;
    HBRUSH Brush;

    Background = YuiGetWindowBackgroundColor();

    Brush = CreateSolidBrush(Background);
    SetBkColor(hDC, Background);
    FillRect(hDC, Rect, Brush);
    DeleteObject(Brush);
}

/**
 Draw a simple, single line 3D box.  This can be raised or sunken.

 @param hDC The device context to render the box into.

 @param Rect The dimensions to render the box.

 @param LineWidth The width of the box to draw, in pixels.

 @param Pressed If TRUE, display a sunken box.  If FALSE, display a raised
        box.
 */
VOID
YuiDrawThreeDBox(
    __in HDC hDC,
    __in PRECT Rect,
    __in WORD LineWidth,
    __in BOOLEAN Pressed
    )
{
    COLORREF TopLeft;
    COLORREF BottomRight;
    HPEN TopLeftPen;
    HPEN BottomRightPen;
    HGDIOBJ OldObject;
    WORD Index;

    if (Pressed) {
        TopLeft = YuiGetShadowColor();
        BottomRight = YuiGetHighlightColor();
    } else {
        TopLeft = YuiGetHighlightColor();
        BottomRight = YuiGetShadowColor();
    }

    //
    //  Render the outline.
    //

    TopLeftPen = CreatePen(PS_SOLID, 0, TopLeft);
    BottomRightPen = CreatePen(PS_SOLID, 0, BottomRight);
    OldObject = SelectObject(hDC, TopLeftPen);

    for (Index = 0; Index < LineWidth; Index++) {
        SelectObject(hDC, TopLeftPen);
        MoveToEx(hDC, Rect->left + Index, Rect->bottom - 1 - Index, NULL);
        LineTo(hDC, Rect->left + Index, Rect->top + Index);
        LineTo(hDC, Rect->right - 1 - Index, Rect->top + Index);

        SelectObject(hDC, BottomRightPen);
        LineTo(hDC, Rect->right - 1 - Index, Rect->bottom - 1 - Index);
        LineTo(hDC, Rect->left + Index, Rect->bottom - 1 - Index);
    }

    SelectObject(hDC, OldObject);

    DeleteObject(TopLeftPen);
    DeleteObject(BottomRightPen);
}

/**
 Draw an ownerdraw static control.  Currently this routine assumes it will
 have a sunken appearence.

 @param YuiMonitor Pointer to the monitor context.

 @param DrawItemStruct Pointer to the struct describing the draw operation.

 @param Text Pointer to text to draw in the control.  Currently this routine
        assumes this text should be vertically and horizontally centered.
 */
VOID
YuiTaskbarDrawStatic(
    __in PYUI_MONITOR YuiMonitor,
    __in PDRAWITEMSTRUCT DrawItemStruct,
    __in PYORI_STRING Text
    )
{
    DWORD Flags;
    RECT TextRect;

    YuiDrawWindowBackground(DrawItemStruct->hDC, &DrawItemStruct->rcItem);
    YuiDrawThreeDBox(DrawItemStruct->hDC, &DrawItemStruct->rcItem, YuiMonitor->ControlBorderWidth, TRUE);

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
 Draw a raised 3D effect within a window.

 @param YuiMonitor Pointer to the monitor context.

 @param hWnd The window to draw the effect within.

 @param hDC Optional device context.  This can be NULL in which case normal
        BeginPaint logic is used.

 @return TRUE if painting occurred, FALSE if it did not.
 */
BOOLEAN
YuiDrawWindowFrame(
    __in PYUI_MONITOR YuiMonitor,
    __in HWND hWnd,
    __in_opt HDC hDC
    )
{
    RECT ClientRect;
    HDC hDCPaint;
    HDC hDCToUse;
    PAINTSTRUCT paintStruct;

    //
    //  If the window has no repainting to do, stop.
    //

    if (!GetUpdateRect(hWnd, &ClientRect, FALSE)) {
        return FALSE;
    }

    //
    //  If it does, redraw everything.
    //

    hDCPaint = BeginPaint(hWnd, &paintStruct);
    if (hDCPaint == NULL) {
        return FALSE;
    }

    if (hDC != NULL) {
        hDCToUse = hDC;
    } else {
        hDCToUse = hDCPaint;
    }
    GetClientRect(hWnd, &ClientRect);
    YuiDrawWindowBackground(hDCToUse, &ClientRect);
    YuiDrawThreeDBox(hDCToUse, &ClientRect, YuiMonitor->ControlBorderWidth, FALSE);

    EndPaint(hWnd, &paintStruct);
    return TRUE;
}

/**
 Draw a popup menu.  Note this is distinct from drawing items within a popup
 menu, and this operation is not well supported by the platform.  This
 function will fill the menu with a chosen color, and draw a raised 3D box
 around it.

 @param YuiMonitor Pointer to the monitor context.

 @param hDC The device context for drawing a single menu item (not necessarily
        the entire menu.)
 */
VOID
YuiDrawEntireMenu(
    __in PYUI_MONITOR YuiMonitor,
    __in HDC hDC
    )
{
    HWND hwndMenu;
    HDC MenuDC;
    RECT WindowRect;
    RECT ClientRect;
    DWORD WindowHeight;
    DWORD WindowWidth;
    WORD Index;
    WORD MarginX;
    WORD MarginY;
    WORD BorderWidth;
    COLORREF Background;
    HPEN Pen;
    HGDIOBJ OldObject;

    hwndMenu = WindowFromDC(hDC);

    //
    //  When menu animation is in effect, the menu is rendered to a non-window
    //  DC.  There's not much we can do with it.
    //

    if (hwndMenu == NULL) {
        return;
    }

    if (!DllUser32.pGetWindowRect(hwndMenu, &WindowRect)) {
        return;
    }
    if (!DllUser32.pGetClientRect(hwndMenu, &ClientRect)) {
        return;
    }

    //
    //  Calculate the area outside of the client rect, which will not be drawn
    //  by menu items.
    //

    WindowHeight = WindowRect.bottom - WindowRect.top;
    WindowWidth = WindowRect.right - WindowRect.left;
    MarginX = (WORD)((WindowWidth - ClientRect.right) / 2);
    MarginY = (WORD)((WindowHeight - ClientRect.bottom) / 2);

    //
    //  Draw a 3D box there, without filling the background.  Since the margin
    //  size is out of our control, limit the width of the 3D box to fit in the
    //  margin.
    //

    ClientRect.left = 0;
    ClientRect.top = 0;
    ClientRect.right = WindowRect.right - WindowRect.left;
    ClientRect.bottom = WindowRect.bottom - WindowRect.top;
    MenuDC = GetWindowDC(hwndMenu);
    BorderWidth = YuiMonitor->ControlBorderWidth;
    if (BorderWidth > MarginX) {
        BorderWidth = MarginX;
    }
    if (BorderWidth > MarginY) {
        BorderWidth = MarginY;
    }
    YuiDrawThreeDBox(MenuDC, &ClientRect, BorderWidth, FALSE);

    //
    //  Draw the window background color in any margin not rendered as 3D.
    //  Note that LineTo does not draw the final pixel in its range, so
    //  these calls draw one pixel too far.  Each LineTo is followed by
    //  a MoveTo so final position is not relevant.
    //

    Background = YuiGetWindowBackgroundColor();
    Pen = CreatePen(PS_SOLID, 0, Background);
    OldObject = SelectObject(MenuDC, Pen);
    for (Index = BorderWidth; Index < MarginX; Index++) {
        MoveToEx(MenuDC, Index, Index, NULL);
        LineTo(MenuDC, Index, WindowHeight - Index);
        MoveToEx(MenuDC, WindowWidth - Index - 1, Index, NULL);
        LineTo(MenuDC, WindowWidth - Index - 1, WindowHeight - Index);
    }
    for (Index = BorderWidth; Index < MarginY; Index++) {
        MoveToEx(MenuDC, Index, Index, NULL);
        LineTo(MenuDC, WindowWidth - Index, Index);
        MoveToEx(MenuDC, Index, WindowHeight - Index - 1, NULL);
        LineTo(MenuDC, WindowWidth - Index, WindowHeight - Index - 1);
    }
    SelectObject(MenuDC, OldObject);
    ReleaseDC(hwndMenu, MenuDC);
}

/**
 Determint the size of a an owner draw menu item.

 @param YuiMonitor Pointer to the monitor context.

 @param Item Pointer to the item to draw.

 @return TRUE to indicate the item was measured.
 */
BOOL
YuiDrawMeasureMenuItem(
    __in PYUI_MONITOR YuiMonitor,
    __in LPMEASUREITEMSTRUCT Item
    )
{
    PYUI_MENU_OWNERDRAW_ITEM ItemContext;
    PYUI_CONTEXT YuiContext;

    YuiContext = YuiMonitor->YuiContext;
    ItemContext = (PYUI_MENU_OWNERDRAW_ITEM)Item->itemData;

    if (ItemContext->WidthByStringLength) {
        Item->itemWidth = YuiDrawGetTextWidth(YuiMonitor, FALSE, &ItemContext->Text) + 
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
        if (YuiMonitor->ScreenWidth > 800) {
            Item->itemWidth = Item->itemWidth + (YuiMonitor->ScreenWidth - 800) / 20;
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

    if (YuiMonitor->ScreenHeight > 800) {
        Item->itemHeight = Item->itemHeight + (YuiMonitor->ScreenHeight - 800) / 100;
    }

    return TRUE;
}

/**
 Draw an owner draw menu item.

 @param YuiMonitor Pointer to the monitor context.

 @param Item Pointer to the item to draw.

 @return TRUE to indicate the item was drawn.  On failure there's not much we
         can do.
 */
BOOL
YuiDrawMenuItem(
    __in PYUI_MONITOR YuiMonitor,
    __in LPDRAWITEMSTRUCT Item
    )
{
    COLORREF BackColor;
    COLORREF ForeColor;
    HBRUSH Brush;
    RECT TextRect;
    PYUI_MENU_OWNERDRAW_ITEM ItemContext;
    DWORD IconPadding;
    PYUI_CONTEXT YuiContext;

    if (Item->rcItem.top == 0 &&
        Item->rcItem.left == 0 &&
        Item->itemAction == ODA_DRAWENTIRE) {

        YuiDrawEntireMenu(YuiMonitor, Item->hDC);
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

    YuiContext = YuiMonitor->YuiContext;

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
        WORD SeperatorWidth;

        SeperatorWidth = (WORD)(YuiMonitor->ControlBorderWidth / 2);
        if (SeperatorWidth == 0) {
            SeperatorWidth = 1;
        }

        DrawRect.left = Item->rcItem.left;
        DrawRect.right = Item->rcItem.right;
        DrawRect.top = (Item->rcItem.bottom - Item->rcItem.top) / 2 + Item->rcItem.top - SeperatorWidth;
        DrawRect.bottom = DrawRect.top + 2 * SeperatorWidth;
        YuiDrawThreeDBox(Item->hDC, &DrawRect, SeperatorWidth, TRUE);

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
        OldObject = SelectObject(Item->hDC, YuiMonitor->hFont);
        DrawText(Item->hDC, ItemContext->Text.StartOfString, ItemContext->Text.LengthInChars, &TextRect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        SelectObject(Item->hDC, OldObject);
    }
    return TRUE;
}

// vim:sw=4:ts=4:et:
