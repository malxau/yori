/**
 * @file libwin/border.c
 *
 * Yori display rectangle that could constitute a border on a window or
 * control
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
 Returns the bright shade for a highlight

 @param OriginalAttributes The original color to get the bright shade for

 @return The bright shade of the specified color
 */
WORD
YoriWinBorderGetDarkAttributes(
    __in WORD OriginalAttributes
    )
{
    WORD Forecolor;
    Forecolor = (WORD)(OriginalAttributes & 0x0F);
    if (Forecolor == 0) {
        return (WORD)((OriginalAttributes & 0xf0) | FOREGROUND_INTENSITY);
    }
    return (WORD)((OriginalAttributes & 0xf0) | FOREGROUND_INTENSITY);
}

/**
 Returns the dark shade for a shadow

 @param OriginalAttributes The original color to get the dark shade for

 @return The dark shade of the specified color
 */
WORD
YoriWinBorderGetLightAttributes(
    __in WORD OriginalAttributes
    )
{
    WORD Forecolor;
    Forecolor = (WORD)(OriginalAttributes & 0x0F);
    if (Forecolor == 0) {
        return (WORD)(OriginalAttributes | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }
    return (WORD)(OriginalAttributes | FOREGROUND_INTENSITY);
}

/**
 Given the requested border style and attributes, calculate the bright and
 dark attributes as well as characters to use for the border.

 @param WinMgrHandle Pointer to the window manager.

 @param Attributes The base attributes of the region.

 @param BorderType The border style to apply.

 @param TopAttributes Pointer to a variable to receive the highlight color.

 @param BottomAttributes Pointer to a variable to receive the dark color.

 @param BorderChars Pointer to a pointer which will be updated to point to
        the set of characters to use to render the border.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinTranslateAttributesAndBorderStyle(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in WORD Attributes,
    __in WORD BorderType,
    __out PWORD TopAttributes,
    __out PWORD BottomAttributes,
    __out CONST TCHAR** BorderChars
    )
{ 
    WORD ThreeDMask;
    WORD BorderStyleMask;
    YORI_WIN_CHARACTERS CharSet;

    ThreeDMask = (WORD)(BorderType & YORI_WIN_BORDER_THREED_MASK);
    *TopAttributes = *BottomAttributes = Attributes;
    switch(ThreeDMask) {
        case YORI_WIN_BORDER_TYPE_RAISED:
            *TopAttributes = YoriWinBorderGetLightAttributes(Attributes);
            *BottomAttributes = YoriWinBorderGetDarkAttributes(Attributes);
            break;
        case YORI_WIN_BORDER_TYPE_SUNKEN:
            *TopAttributes = YoriWinBorderGetDarkAttributes(Attributes);
            *BottomAttributes = YoriWinBorderGetLightAttributes(Attributes);
            break;
    }

    CharSet = YoriWinCharsSingleLineBorder;
    BorderStyleMask = (WORD)(BorderType & YORI_WIN_BORDER_STYLE_MASK);
    switch(BorderStyleMask) {
        case YORI_WIN_BORDER_TYPE_DOUBLE:
            CharSet = YoriWinCharsDoubleLineBorder;
            break;
        case YORI_WIN_BORDER_TYPE_SOLID_FULL:
            CharSet = YoriWinCharsFullSolidBorder;
            break;
        case YORI_WIN_BORDER_TYPE_SOLID_HALF:
            CharSet = YoriWinCharsHalfSolidBorder;
            break;
    }

    *BorderChars = YoriWinGetDrawingCharacters(WinMgrHandle, CharSet);

    return TRUE;
}

/**
 Draw a rectangle on the control with the specified coordinates.

 @param Ctrl Pointer to the control to draw the rectangle on.

 @param Dimensions The dimensions of the rectangle to draw.

 @param Attributes The color to use for the border.

 @param BorderType Specifies if the border should be single or double line,
        raised, lowered, or flat

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinDrawBorderOnControl(
    __inout PYORI_WIN_CTRL Ctrl,
    __in PSMALL_RECT Dimensions,
    __in WORD Attributes,
    __in WORD BorderType
    )
{
    WORD RowIndex;
    WORD CellIndex;
    WORD TopAttributes;
    WORD BottomAttributes;
    CONST TCHAR* BorderChars;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

    WinMgrHandle = YoriWinGetWindowManagerHandle(YoriWinGetTopLevelWindow(Ctrl));

    YoriWinTranslateAttributesAndBorderStyle(WinMgrHandle, Attributes, BorderType, &TopAttributes, &BottomAttributes, &BorderChars);

    YoriWinSetControlNonClientCell(Ctrl, Dimensions->Left, Dimensions->Top, BorderChars[0], TopAttributes);
    for (CellIndex = (WORD)(Dimensions->Left + 1); CellIndex < Dimensions->Right; CellIndex++) {
        YoriWinSetControlNonClientCell(Ctrl, CellIndex, Dimensions->Top, BorderChars[1], TopAttributes);
    }
    YoriWinSetControlNonClientCell(Ctrl, Dimensions->Right, Dimensions->Top, BorderChars[2], BottomAttributes);

    for (RowIndex = (WORD)(Dimensions->Top + 1); RowIndex <= Dimensions->Bottom - 1; RowIndex++) {
        YoriWinSetControlNonClientCell(Ctrl, Dimensions->Left, RowIndex, BorderChars[3], TopAttributes);
        YoriWinSetControlNonClientCell(Ctrl, Dimensions->Right, RowIndex, BorderChars[4], BottomAttributes);
    }
    YoriWinSetControlNonClientCell(Ctrl, Dimensions->Left, Dimensions->Bottom, BorderChars[5], TopAttributes);
    for (CellIndex = (WORD)(Dimensions->Left + 1); CellIndex < Dimensions->Right; CellIndex++) {
        YoriWinSetControlNonClientCell(Ctrl, CellIndex, Dimensions->Bottom, BorderChars[6], BottomAttributes);
    }
    YoriWinSetControlNonClientCell(Ctrl, Dimensions->Right, Dimensions->Bottom, BorderChars[7], BottomAttributes);

    return TRUE;
}

/**
 Draw characters on the edge of a single line control to represent a limited
 border.

 @param Ctrl Pointer to the control to draw the rectangle on.

 @param Dimensions The dimensions of the rectangle to draw.

 @param Attributes The color to use for the border.

 @param BorderType Specifies if the border should be single or double line,
        raised, lowered, or flat.  This is used within this function to change
        color without changing characters or 3D appearence.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinDrawSingleLineBorderOnControl(
    __inout PYORI_WIN_CTRL Ctrl,
    __in PSMALL_RECT Dimensions,
    __in WORD Attributes,
    __in WORD BorderType
    )
{
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;
    WORD AttributesToUse;
    WORD BorderStyleMask;
    CONST TCHAR* BorderChars;

    if (Dimensions->Top != Dimensions->Bottom) {
        return FALSE;
    }

    WinMgrHandle = YoriWinGetWindowManagerHandle(YoriWinGetTopLevelWindow(Ctrl));

    AttributesToUse = Attributes;
    BorderChars = YoriWinGetDrawingCharacters(WinMgrHandle, YoriWinCharsOneLineSingleBorder);
    BorderStyleMask = (WORD)(BorderType & YORI_WIN_BORDER_STYLE_MASK);
    if (BorderStyleMask == YORI_WIN_BORDER_TYPE_DOUBLE) {
        BorderChars = YoriWinGetDrawingCharacters(WinMgrHandle, YoriWinCharsOneLineDoubleBorder);
    }

    if (BorderType & YORI_WIN_BORDER_BRIGHT) {
        AttributesToUse = YoriWinBorderGetLightAttributes(AttributesToUse);
    }

    YoriWinSetControlNonClientCell(Ctrl, Dimensions->Left, Dimensions->Top, BorderChars[0], AttributesToUse);
    YoriWinSetControlNonClientCell(Ctrl, Dimensions->Right, Dimensions->Top, BorderChars[1], AttributesToUse);

    return TRUE;
}

// vim:sw=4:ts=4:et:
