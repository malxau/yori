/**
 * @file lib/color.c
 *
 * Parse strings into color codes.
 *
 * This module implements string parsing and rule application to select a
 * given set of color attributes to render any particular file with.
 *
 * Copyright (c) 2014-2018 Malcolm J. Smith
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

/**
 A table of color strings to CGA colors.
 */
YORILIB_ATTRIBUTE_COLOR_STRING ColorString[] = {

    {_T("black"),        {0, 0x00}},
    {_T("blue"),         {0, 0x01}},
    {_T("green"),        {0, 0x02}},
    {_T("cyan"),         {0, 0x03}},
    {_T("red"),          {0, 0x04}},
    {_T("magenta"),      {0, 0x05}},
    {_T("brown"),        {0, 0x06}},
    {_T("gray"),         {0, 0x07}},

    {_T("darkgray"),     {0, 0x08}},
    {_T("lightblue"),    {0, 0x09}},
    {_T("lightgreen"),   {0, 0x0A}},
    {_T("lightcyan"),    {0, 0x0B}},
    {_T("lightred"),     {0, 0x0C}},
    {_T("lightmagenta"), {0, 0x0D}},
    {_T("yellow"),       {0, 0x0E}},
    {_T("white"),        {0, 0x0F}},

    //
    //  Helper modifier
    //

    {_T("bright"),       {0, 0x08}},

    //
    //  Some non-color things
    //

    {_T("invert"),       {YORILIB_ATTRCTRL_INVERT, 0}},
    {_T("hide"),         {YORILIB_ATTRCTRL_HIDE, 0}},
    {_T("continue"),     {YORILIB_ATTRCTRL_CONTINUE, 0}},
    {_T("file"),         {YORILIB_ATTRCTRL_FILE, 0}},
    {_T("window_bg"),    {YORILIB_ATTRCTRL_WINDOW_BG, 0}},
    {_T("window_fg"),    {YORILIB_ATTRCTRL_WINDOW_FG, 0}}
};


/**
 Lookup a color from a string.  This function can combine different foreground
 and background settings.  If it cannot resolve the color, it returns the
 current window background and foreground.

 @param String The string to resolve.

 @return The corresponding color.
 */
YORILIB_COLOR_ATTRIBUTES
YoriLibAttributeFromString(
    __in LPCTSTR String
    )
{
    YORILIB_COLOR_ATTRIBUTES Attribute = {0};
    DWORD Element;
    BOOL Background = FALSE;
    BOOL ExplicitBackground = FALSE;
    BOOL ExplicitForeground = FALSE;
    LPTSTR Next;

    //
    //  Loop through the list of color keywords
    //

    while (String && String[0] != '\0') {

        //
        //  Check if we're setting a background.
        //
    
        Background = FALSE;
        if ((String[0] == 'b' || String[0] == 'B') &&
            (String[1] == 'g' || String[1] == 'G') &&
            String[2] == '_') {
    
            String += 3;
            Background = TRUE;
        }

        //
        //  Look for the next element
        //

        Next = _tcschr(String, '+');
        if (Next) {
            *Next = '\0';
        }

        //
        //  Walk through the string table for a match.
        //
    
        for (Element = 0; Element < sizeof(ColorString)/sizeof(ColorString[0]); Element++) {
            if (_tcsicmp(String, ColorString[Element].String) == 0) {
    
                if (Background) {
                    if (ColorString[Element].Attr.Ctrl != 0) {
                        Attribute.Ctrl |= ColorString[Element].Attr.Ctrl;
                    } else {
                        ExplicitBackground = TRUE;
                    }
                    Attribute.Win32Attr |= (ColorString[Element].Attr.Win32Attr & YORILIB_ATTRIBUTE_ONECOLOR_MASK) << 4;
                } else {
                    if (ColorString[Element].Attr.Ctrl != 0) {
                        Attribute.Ctrl |= ColorString[Element].Attr.Ctrl;
                    } else {
                        ExplicitForeground = TRUE;
                    }
                    Attribute.Win32Attr |= ColorString[Element].Attr.Win32Attr;
                }
    
                break;
            }
        }
    
        //
        //  Advance to the next element
        //

        if (Next) {
            String = Next + 1;
        } else {
            String = NULL;
        }
    }

    //
    //  If an explicit background or foreground was specified,
    //  use it.  If not, assume window defaults.  When combining,
    //  if a previous color was specified explicitly, it will take
    //  precedence.
    //

    if (!ExplicitBackground) {
        Attribute.Ctrl |= YORILIB_ATTRCTRL_WINDOW_BG;
    }

    if (!ExplicitForeground) {
        Attribute.Ctrl |= YORILIB_ATTRCTRL_WINDOW_FG;
    }

    return Attribute;
}

/**
 Set a color structure to an explicit Win32 attribute.

 @param Attributes Pointer to the color structure to set.

 @param Win32Attribute The Win32 color to set the structure to.

 @return TRUE to indicate success or FALSE to indicate failure.
 */
BOOL
YoriLibSetColorToWin32(
    __out PYORILIB_COLOR_ATTRIBUTES Attributes,
    __in UCHAR Win32Attribute
    )
{
    Attributes->Ctrl = 0;
    Attributes->Win32Attr = Win32Attribute;
    return TRUE;
}

/**
 Logically combine two colors.  This means if either color specifies a non
 default window color, that takes precedence.  If both values contain colors,
 the values are xor'd.  Backgrounds and foregrounds are compiled independently
 and recombined.

 @param Color1 The first color to combine.

 @param Color2 The second color to combine.

 @return The resulting combined color.
 */
YORILIB_COLOR_ATTRIBUTES
YoriLibCombineColors(
    __in YORILIB_COLOR_ATTRIBUTES Color1,
    __in YORILIB_COLOR_ATTRIBUTES Color2
    )
{
    YORILIB_COLOR_ATTRIBUTES Result;

    Result.Ctrl = 0;
    Result.Win32Attr = 0;

    Result.Ctrl = (UCHAR)((Color1.Ctrl | Color2.Ctrl) & ~(YORILIB_ATTRCTRL_WINDOW_BG|YORILIB_ATTRCTRL_WINDOW_FG));

    //
    //  Treat the default window color as recessive.  If anything explicitly
    //  includes a color, that takes precedence.
    //

    if ((Color1.Ctrl & YORILIB_ATTRCTRL_WINDOW_BG) != 0 &&
        (Color2.Ctrl & YORILIB_ATTRCTRL_WINDOW_BG) != 0) {

        Result.Ctrl |= YORILIB_ATTRCTRL_WINDOW_BG;
    } else {
        Result.Win32Attr |= (Color1.Win32Attr ^ Color2.Win32Attr) & 0xF0;
    }

    if ((Color1.Ctrl & YORILIB_ATTRCTRL_WINDOW_FG) != 0 &&
        (Color2.Ctrl & YORILIB_ATTRCTRL_WINDOW_FG) != 0) {

        Result.Ctrl |= YORILIB_ATTRCTRL_WINDOW_FG;
    } else {
        Result.Win32Attr |= (Color1.Win32Attr ^ Color2.Win32Attr) & 0x0F;
    }

    return Result;
}

/**
 Substitute default window background and foreground colors for a specified
 window color.

 @param Color The color to resolve window components on.

 @param WindowColor The active color in the window.

 @param RetainWindowCtrlFlags TRUE if the resulting color should still
        indicate that it is using the window background or foreground;
        FALSE if the result should be an explicit color without any indication
        of default components.

 @return The resolved color information.
 */
YORILIB_COLOR_ATTRIBUTES
YoriLibResolveWindowColorComponents(
    __in YORILIB_COLOR_ATTRIBUTES Color,
    __in YORILIB_COLOR_ATTRIBUTES WindowColor,
    __in BOOL RetainWindowCtrlFlags
    )
{
    YORILIB_COLOR_ATTRIBUTES NewColor = Color;

    if (NewColor.Ctrl & YORILIB_ATTRCTRL_WINDOW_BG) {
        NewColor.Win32Attr = (UCHAR)((WindowColor.Win32Attr & 0xF0) + (NewColor.Win32Attr & 0xF));
    }

    if (NewColor.Ctrl & YORILIB_ATTRCTRL_WINDOW_FG) {
        NewColor.Win32Attr = (UCHAR)((NewColor.Win32Attr & 0xF0) + (WindowColor.Win32Attr & 0xF));
    }

    if (!RetainWindowCtrlFlags) {
        NewColor.Ctrl &= ~(YORILIB_ATTRCTRL_WINDOW_BG|YORILIB_ATTRCTRL_WINDOW_FG);
    }

    return NewColor;
}

/**
 Indicates if two colors are the same value.

 @param Color1 The first color to evaluate.

 @param Color2 The second color to evaluate.

 @return TRUE to indicate a match, FALSE to indicate no match.
 */
BOOL
YoriLibAreColorsIdentical(
    __in YORILIB_COLOR_ATTRIBUTES Color1,
    __in YORILIB_COLOR_ATTRIBUTES Color2
    )
{
    if (Color1.Win32Attr == Color2.Win32Attr) {
        return TRUE;
    }
    return FALSE;
}

// vim:sw=4:ts=4:et:
