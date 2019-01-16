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
YORILIB_ATTRIBUTE_COLOR_STRING YoriLibColorStringTable[] = {

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
    __in PYORI_STRING String
    )
{
    YORILIB_COLOR_ATTRIBUTES Attribute = {0};
    YORI_STRING SingleElement;
    DWORD Element;
    BOOL Background = FALSE;
    BOOL ExplicitBackground = FALSE;
    BOOL ExplicitForeground = FALSE;
    DWORD Index;

    //
    //  Loop through the list of color keywords
    //

    Index = 0;
    YoriLibInitEmptyString(&SingleElement);
    while (Index < String->LengthInChars) {

        //
        //  Check if we're setting a background.
        //

        Background = FALSE;
        SingleElement.StartOfString = &String->StartOfString[Index];
        SingleElement.LengthInChars = String->LengthInChars - Index;
        if (SingleElement.LengthInChars >= 3 &&
            (SingleElement.StartOfString[0] == 'b' || SingleElement.StartOfString[0] == 'B') &&
            (SingleElement.StartOfString[1] == 'g' || SingleElement.StartOfString[1] == 'G') &&
            SingleElement.StartOfString[2] == '_') {

            SingleElement.StartOfString += 3;
            SingleElement.LengthInChars -= 3;
            Background = TRUE;
        }

        //
        //  Look for the next element
        //

        SingleElement.LengthInChars = YoriLibCountStringNotContainingChars(&SingleElement, _T("+"));

        //
        //  Walk through the string table for a match.
        //

        for (Element = 0; Element < sizeof(YoriLibColorStringTable)/sizeof(YoriLibColorStringTable[0]); Element++) {
            if (YoriLibCompareStringWithLiteralInsensitive(&SingleElement, YoriLibColorStringTable[Element].String) == 0) {

                if (Background) {
                    if (YoriLibColorStringTable[Element].Attr.Ctrl != 0) {
                        Attribute.Ctrl |= YoriLibColorStringTable[Element].Attr.Ctrl;
                    } else {
                        ExplicitBackground = TRUE;
                    }
                    Attribute.Win32Attr |= (YoriLibColorStringTable[Element].Attr.Win32Attr & YORILIB_ATTRIBUTE_ONECOLOR_MASK) << 4;
                } else {
                    if (YoriLibColorStringTable[Element].Attr.Ctrl != 0) {
                        Attribute.Ctrl |= YoriLibColorStringTable[Element].Attr.Ctrl;
                    } else {
                        ExplicitForeground = TRUE;
                    }
                    Attribute.Win32Attr |= YoriLibColorStringTable[Element].Attr.Win32Attr;
                }

                break;
            }
        }

        //
        //  Advance to the next element
        //

        if (Background) {
            Index += 3;
        }
        Index += SingleElement.LengthInChars;
        while (Index < String->LengthInChars) {
            if (String->StartOfString[Index] == '+') {
                Index++;
            } else {
                break;
            }
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
 Lookup a color from a NULL terminated string.  This function can combine
 different foreground and background settings.  If it cannot resolve the
 color, it returns the current window background and foreground.

 @param String The string to resolve.

 @return The corresponding color.
 */
YORILIB_COLOR_ATTRIBUTES
YoriLibAttributeFromLiteralString(
    __in LPCTSTR String
    )
{
    YORI_STRING Ys;
    YoriLibConstantString(&Ys, String);
    return YoriLibAttributeFromString(&Ys);
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

/**
 The default color attributes to apply for file criteria if the user has not
 specified anything else in the environment.
 */
const
CHAR YoriLibDefaultFileColorString[] = 
    "fa&r,magenta;"
    "fa&D,lightmagenta;"
    "fa&R,green;"
    "fa&H,green;"
    "fa&S,green;"
    "fe=bat,lightred;"
    "fe=cmd,lightred;"
    "fe=com,lightcyan;"
    "fe=dll,cyan;"
    "fe=doc,white;"
    "fe=docx,white;"
    "fe=exe,lightcyan;"
    "fe=htm,white;"
    "fe=html,white;"
    "fe=pdf,white;"
    "fe=pl,red;"
    "fe=ppt,white;"
    "fe=pptx,white;"
    "fe=ps1,lightred;"
    "fe=psd1,red;"
    "fe=psm1,red;"
    "fe=sys,cyan;"
    "fe=xls,white;"
    "fe=xlsx,white;"
    "fe=ys1,lightred";

/**
 Return the default file color string for display in help texts.
 
 @return Pointer to a const NULL terminated ANSI string containing the default
         file color string.
 */
LPCSTR
YoriLibGetDefaultFileColorString()
{
    return YoriLibDefaultFileColorString;
}


/**
 Generate an allocated string containing the user's environment contents
 combined with any default.

 @param Custom Optionally points to a string of colors to include ahead of any
        defaults.

 @param Combined On successful completion, populated with a newly allocated
        string representing the entire set of file color criteria to apply
        in order.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadCombinedFileColorString(
    __in_opt PYORI_STRING Custom,
    __out PYORI_STRING Combined
    )
{
    DWORD ColorPrependLength = 0;
    DWORD ColorAppendLength  = 0;
    DWORD ColorReplaceLength = 0;
    DWORD ColorCustomLength = 0;
    DWORD i;

    //
    //  Load any user specified colors from the environment.  Prepend values go before
    //  any default; replace values supersede any default; and append values go last.
    //  We need to insert semicolons between these values if they're specified.
    //
    //  First, count how big the allocation needs to be and allocate it.
    //

    ColorPrependLength = GetEnvironmentVariable(_T("SDIR_COLOR_PREPEND"), NULL, 0);
    ColorReplaceLength = GetEnvironmentVariable(_T("SDIR_COLOR_REPLACE"), NULL, 0);
    ColorAppendLength  = GetEnvironmentVariable(_T("SDIR_COLOR_APPEND"), NULL, 0);
    if (Custom != NULL) {
        ColorCustomLength = Custom->LengthInChars;
    }

    if (!YoriLibAllocateString(Combined, ColorPrependLength + 1 + ColorAppendLength + 1 + ColorReplaceLength + 1 + ColorCustomLength + sizeof(YoriLibDefaultFileColorString))) {
        return FALSE;
    }

    //
    //  Now, load any environment variables into the buffer.  If replace isn't
    //  specified, we use the hardcoded default.
    //

    i = 0;
    if (ColorPrependLength) {
        GetEnvironmentVariable(_T("SDIR_COLOR_PREPEND"),
                               Combined->StartOfString,
                               ColorPrependLength);
        i += ColorPrependLength;
        Combined->StartOfString[i - 1] = ';';
    }

    if (ColorCustomLength) {
        YoriLibSPrintfS(&Combined->StartOfString[i], ColorCustomLength + 1, _T("%y"), Custom);
        i += ColorCustomLength;
        Combined->StartOfString[i] = ';';
        i++;
    }

    if (ColorReplaceLength) {
        GetEnvironmentVariable(_T("SDIR_COLOR_REPLACE"),
                               &Combined->StartOfString[i],
                               ColorReplaceLength);
        i += ColorReplaceLength - 1;
    } else {
        YoriLibSPrintfS(&Combined->StartOfString[i],
                        sizeof(YoriLibDefaultFileColorString) / sizeof(YoriLibDefaultFileColorString[0]),
                        _T("%hs"),
                        YoriLibDefaultFileColorString);
        i += sizeof(YoriLibDefaultFileColorString) / sizeof(YoriLibDefaultFileColorString[0]) - 1;
    }

    if (ColorAppendLength) {
        Combined->StartOfString[i] = ';';
        i += 1;
        GetEnvironmentVariable(_T("SDIR_COLOR_APPEND"),
                               &Combined->StartOfString[i],
                               ColorAppendLength);
        i += ColorAppendLength - 1;
    }

    Combined->LengthInChars = i;

    return TRUE;
}

// vim:sw=4:ts=4:et:
