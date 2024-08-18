/**
 * @file libwin/color.c
 *
 * Yori window default color schemes
 *
 * Copyright (c) 2021-2024 Malcolm J. Smith
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
 For readability, a constant including all foreground color components.
 */
#define FOREGROUND_GREY (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

/**
 For readability, a constant including all background color components.
 */
#define BACKGROUND_GREY (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)


/**
 A table of colors to use by default for controls displaying on a traditional
 16-color VGA display with foreground and background support.
 */
CONST UCHAR YoriWinVgaColors[] = {
    BACKGROUND_GREY,                                          // YoriWinColorWindowDefault
    BACKGROUND_BLUE | BACKGROUND_INTENSITY | FOREGROUND_GREY | FOREGROUND_INTENSITY, // YoriWinTitleBarActive
    BACKGROUND_GREY,                                          // YoriWinColorMenuDefault
    FOREGROUND_GREY,                                          // YoriWinColorMenuSelected
    BACKGROUND_GREY | FOREGROUND_GREY | FOREGROUND_INTENSITY, // YoriWinColorMenuAccelerator
    FOREGROUND_GREY | FOREGROUND_INTENSITY,                   // YoriWinColorMenuSelectedAccelerator
    BACKGROUND_GREY,                                          // YoriWinColorMultilineCaption
    FOREGROUND_GREY,                                          // YoriWinColorEditSelectedText
    BACKGROUND_GREY | FOREGROUND_GREY | FOREGROUND_INTENSITY, // YoriWinColorAcceleratorDefault
    FOREGROUND_GREY,                                          // YoriWinColorListActive
    FOREGROUND_GREY,                                          // YoriWinColorControlSelected
    BACKGROUND_INTENSITY | FOREGROUND_GREY | FOREGROUND_INTENSITY, // YoriWinTitleBarInactive
};

/**
 A table of colors to use by default for controls displaying on an Nano
 Server, which has 16 foreground colors but no background colors.
 */
CONST UCHAR YoriWinNanoColors[] = {
    FOREGROUND_GREY,                                          // YoriWinColorWindowDefault
    FOREGROUND_GREEN | FOREGROUND_INTENSITY,                  // YoriWinTitleBarActive
    FOREGROUND_GREY,                                          // YoriWinColorMenuDefault
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // YoriWinColorMenuSelected
    FOREGROUND_GREY | FOREGROUND_INTENSITY,                   // YoriWinColorMenuAccelerator
    FOREGROUND_GREY | FOREGROUND_INTENSITY,                   // YoriWinColorMenuSelectedAccelerator
    FOREGROUND_GREEN | FOREGROUND_INTENSITY,                  // YoriWinColorMultilineCaption
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // YoriWinColorEditSelectedText
    FOREGROUND_GREY | FOREGROUND_INTENSITY,                   // YoriWinColorAcceleratorDefault
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // YoriWinColorListActive
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // YoriWinColorControlSelected
    FOREGROUND_INTENSITY,                                     // YoriWinTitleBarInactive
};

/**
 A table of colors to use when displaying on a 4 color monochrome display.
 */
CONST UCHAR YoriWinMonoColors[] = {
    BACKGROUND_GREY,                                          // YoriWinColorWindowDefault
    BACKGROUND_GREY | BACKGROUND_INTENSITY,                   // YoriWinTitleBarActive
    BACKGROUND_GREY,                                          // YoriWinColorMenuDefault
    FOREGROUND_GREY,                                          // YoriWinColorMenuSelected
    BACKGROUND_GREY | FOREGROUND_GREY | FOREGROUND_INTENSITY, // YoriWinColorMenuAccelerator
    FOREGROUND_GREY | FOREGROUND_INTENSITY,                   // YoriWinColorMenuSelectedAccelerator
    BACKGROUND_GREY,                                          // YoriWinColorMultilineCaption
    FOREGROUND_GREY,                                          // YoriWinColorEditSelectedText
    BACKGROUND_GREY | FOREGROUND_GREY | FOREGROUND_INTENSITY, // YoriWinColorAcceleratorDefault
    FOREGROUND_GREY,                                          // YoriWinColorListActive
    FOREGROUND_GREY,                                          // YoriWinColorControlSelected
    BACKGROUND_INTENSITY | FOREGROUND_GREY | FOREGROUND_INTENSITY, // YoriWinTitleBarInactive
};

/**
 Return the specified color table.

 @param ColorTableId Indicates the color table to return.
 */
YORI_WIN_COLOR_TABLE_HANDLE
YoriWinGetColorTable(
    __in YORI_WIN_COLOR_TABLE_ID ColorTableId
    )
{
    YORI_WIN_COLOR_TABLE_HANDLE Result;
    Result = NULL;
    switch(ColorTableId) {
        case YoriWinColorTableDefault:
            if (!YoriLibDoesSystemSupportBackgroundColors()) {
                Result = (YORI_WIN_COLOR_TABLE_HANDLE)YoriWinNanoColors;
            } else {
                Result = (YORI_WIN_COLOR_TABLE_HANDLE)YoriWinVgaColors;
            }
            break;
        case YoriWinColorTableVga:
            Result = (YORI_WIN_COLOR_TABLE_HANDLE)YoriWinVgaColors;
            break;
        case YoriWinColorTableNano:
            Result = (YORI_WIN_COLOR_TABLE_HANDLE)YoriWinNanoColors;
            break;
        case YoriWinColorTableMono:
            Result = (YORI_WIN_COLOR_TABLE_HANDLE)YoriWinMonoColors;
            break;
    }

    return Result;
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
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 33010) // Unchecked lower bound for enum
#endif
    return ColorTable[ColorId];
}


// vim:sw=4:ts=4:et:
