/**
 * @file libwin/color.c
 *
 * Yori window default color schemes
 *
 * Copyright (c) 2021 Malcolm J. Smith
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
 A table of colors to use by default for controls displaying on a traditional
 16-color VGA display with foreground and background support.
 */
CONST UCHAR YoriWinVgaColors[] = {
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE, // YoriWinColorWindowDefault
    BACKGROUND_BLUE | BACKGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // YoriWinTitleBarDefault
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE, // YoriWinColorMenuDefault
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // YoriWinColorMenuSelected
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // YoriWinColorMenuAccelerator
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // YoriWinColorMenuSelectedAccelerator
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE, // YoriWinColorMultilineCaption
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // YoriWinColorEditSelectedText
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // YoriWinColorAcceleratorDefault
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // YoriWinColorListActive
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // YoriWinColorControlSelected
};

/**
 A table of colors to use by default for controls displaying on an Nano
 Server, which has 16 foreground colors but no background colors.
 */
CONST UCHAR YoriWinNanoColors[] = {
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // YoriWinColorWindowDefault
    FOREGROUND_GREEN | FOREGROUND_INTENSITY, // YoriWinTitleBarDefault
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // YoriWinColorMenuDefault
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // YoriWinColorMenuSelected
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // YoriWinColorMenuAccelerator
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // YoriWinColorMenuSelectedAccelerator
    FOREGROUND_GREEN | FOREGROUND_INTENSITY, // YoriWinColorMultilineCaption
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // YoriWinColorEditSelectedText
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // YoriWinColorAcceleratorDefault
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // YoriWinColorListActive
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // YoriWinColorControlSelected
};


/**
 Return the default color table to use on this system.
 */
YORI_WIN_COLOR_TABLE_HANDLE
YoriWinGetDefaultColorTable(VOID)
{
    if (!YoriLibDoesSystemSupportBackgroundColors()) {
        return (YORI_WIN_COLOR_TABLE_HANDLE)YoriWinNanoColors;
    }
    return (YORI_WIN_COLOR_TABLE_HANDLE)YoriWinVgaColors;
}

/**
 Lookup a specified color value within the supplied color table.

 @param ColorTableHandle Handle to a color table.

 @param ColorId An identifier of a system color.

 @return The attribute value to use.
 */
UCHAR
YoriWinDefaultColorLookup(
    __in YORI_WIN_COLOR_TABLE_HANDLE ColorTableHandle,
    __in YORI_WIN_COLOR_ID ColorId
    )
{
    PUCHAR ColorTable;

    ColorTable = (PUCHAR)ColorTableHandle;
    ASSERT(ColorId < YoriWinColorBeyondMax);
    return ColorTable[ColorId];
}


// vim:sw=4:ts=4:et:
