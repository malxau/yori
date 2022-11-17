/**
 * @file lib/scheme.c
 *
 * Load and process console color schemes.
 *
 * This module implements string parsing and rule application to select a
 * given set of color attributes to render any particular file with.
 *
 * Copyright (c) 2022 Malcolm J. Smith
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
 An array of color names, as used by the scheme format.  These are in order
 to allow for mapping of Win32 colors (by index) to string.
 */
LPCTSTR YoriLibSchemeColorNames[] = {
    _T("BLACK"),
    _T("BLUE"),
    _T("GREEN"),
    _T("CYAN"),
    _T("RED"),
    _T("MAGENTA"),
    _T("YELLOW"),
    _T("WHITE")
};

/**
 An array of intensity values, as used by the scheme format.  These are in
 order to allow for mapping of Win32 intensity (by index) to string.
 */
LPCTSTR YoriLibSchemeColorPrefixes[] = {
    _T("DARK"),
    _T("BRIGHT")
};

/**
 Parse a single comma delimited RGB value into a COLORREF.
 
 @param ColorName Pointer to the string form of the RGB value.
 
 @param ColorValue On successful completion, updated to contain the RGB
        color.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibParseSchemeColorString(
    __in LPTSTR ColorName,
    __out COLORREF *ColorValue
    )
{
    LONGLONG Temp;
    YORI_STRING SubString;
    DWORD CharsConsumed;
    UCHAR Red;
    UCHAR Green;
    UCHAR Blue;

    YoriLibConstantString(&SubString, ColorName);
    YoriLibTrimSpaces(&SubString);

    *ColorValue = 0;

    if (!YoriLibStringToNumber(&SubString, FALSE, &Temp, &CharsConsumed) ||
        CharsConsumed == 0) {

        return FALSE;
    }

    Red = (UCHAR)Temp;

    SubString.StartOfString = SubString.StartOfString + CharsConsumed;
    SubString.LengthInChars = SubString.LengthInChars - CharsConsumed;

    while (SubString.LengthInChars > 0 &&
           (SubString.StartOfString[0] == ',' || SubString.StartOfString[0] == ' ')) {

        SubString.StartOfString++;
        SubString.LengthInChars--;
    }

    if (!YoriLibStringToNumber(&SubString, FALSE, &Temp, &CharsConsumed) ||
        CharsConsumed == 0) {

        return FALSE;
    }

    Green = (UCHAR)Temp;

    SubString.StartOfString = SubString.StartOfString + CharsConsumed;
    SubString.LengthInChars = SubString.LengthInChars - CharsConsumed;

    while (SubString.LengthInChars > 0 &&
           (SubString.StartOfString[0] == ',' || SubString.StartOfString[0] == ' ')) {

        SubString.StartOfString++;
        SubString.LengthInChars--;
    }

    if (!YoriLibStringToNumber(&SubString, FALSE, &Temp, &CharsConsumed) ||
        CharsConsumed == 0) {

        return FALSE;
    }

    Blue = (UCHAR)Temp;
    *ColorValue = RGB(Red, Green, Blue);
    return TRUE;
}

/**
 Update an array of 16 RGB color values by loading the values from a scheme
 file.

 @param IniFileName Pointer to the INI file name.

 @param ColorTable On successful completion, updated with 16 RGB color values
        in Win32 order.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibLoadColorTableFromScheme(
    __in PCYORI_STRING IniFileName,
    __out_ecount(16) COLORREF *ColorTable
    )
{
    DWORD Index;
    DWORD ColorIndex;
    DWORD PrefixIndex;
    TCHAR ValueName[16];
    TCHAR Value[64];
    COLORREF Color;

    if (DllKernel32.pGetPrivateProfileStringW == NULL) {
        return FALSE;
    }

    for (PrefixIndex = 0; PrefixIndex < sizeof(YoriLibSchemeColorPrefixes)/sizeof(YoriLibSchemeColorPrefixes[0]); PrefixIndex++) {
        for (ColorIndex = 0; ColorIndex < sizeof(YoriLibSchemeColorNames)/sizeof(YoriLibSchemeColorNames[0]); ColorIndex++) {
            YoriLibSPrintf(ValueName, _T("%s_%s"), YoriLibSchemeColorPrefixes[PrefixIndex], YoriLibSchemeColorNames[ColorIndex]);
            DllKernel32.pGetPrivateProfileStringW(_T("Table"), ValueName, _T(""), Value, sizeof(Value)/sizeof(Value[0]), IniFileName->StartOfString);

            if (!YoriLibParseSchemeColorString(Value, &Color)) {
                return FALSE;
            }

            Index = PrefixIndex * sizeof(YoriLibSchemeColorNames)/sizeof(YoriLibSchemeColorNames[0]) + ColorIndex;

            ColorTable[Index] = Color;
        }
    }

    return TRUE;
}

/**
 Load a single color in INTENSITY_COLOR form into a 4 bit Win32
 representation.

 @param String Pointer to the INTENSITY_COLOR string form.

 @param OutColor On successful completion, updated to contain the Win32 color
        value.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibLoadColorFromSchemeString(
    __in PCYORI_STRING String,
    __out PUCHAR OutColor
    )
{
    YORI_STRING Dark;
    YORI_STRING Bright;
    UCHAR Index;
    UCHAR Color;
    YORI_STRING StringValue;

    StringValue.StartOfString = String->StartOfString;
    StringValue.LengthInChars = String->LengthInChars;

    Color = 0;
    YoriLibConstantString(&Dark, YoriLibSchemeColorPrefixes[0]);
    YoriLibConstantString(&Bright, YoriLibSchemeColorPrefixes[1]);
    if (YoriLibCompareStringInsensitiveCount(&StringValue, &Dark, Dark.LengthInChars) == 0) {
        StringValue.StartOfString = StringValue.StartOfString + Dark.LengthInChars;
        StringValue.LengthInChars = StringValue.LengthInChars - Dark.LengthInChars;
    } else if (YoriLibCompareStringInsensitiveCount(&StringValue, &Bright, Bright.LengthInChars) == 0) {
        Color = (UCHAR)(Color | FOREGROUND_INTENSITY);
        StringValue.StartOfString = StringValue.StartOfString + Bright.LengthInChars;
        StringValue.LengthInChars = StringValue.LengthInChars - Bright.LengthInChars;
    } else {
        return FALSE;
    }

    if (StringValue.LengthInChars < 1 || StringValue.StartOfString[0] != '_') {
        return FALSE;
    }

    StringValue.StartOfString = StringValue.StartOfString + 1;
    StringValue.LengthInChars = StringValue.LengthInChars - 1;

    for (Index = 0; Index < sizeof(YoriLibSchemeColorNames)/sizeof(YoriLibSchemeColorNames[0]); Index++) {
        if (YoriLibCompareStringWithLiteralInsensitive(&StringValue, YoriLibSchemeColorNames[Index]) == 0) {
            Color = (UCHAR)(Color + Index);
            break;
        }
    }

    if (Index == sizeof(YoriLibSchemeColorNames)/sizeof(YoriLibSchemeColorNames[0])) {
        return FALSE;
    }

    *OutColor = Color;
    return TRUE;
}

/**
 Load a Foreground and Background color in INTENSITY_COLOR form from an INI
 section.

 @param IniFileName Pointer to the INI file name.

 @param SectionName Pointer to the section within the INI file.

 @param WindowColor On successful completion, updated to contain the Win32
        color including background and foreground components.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibLoadSectionColorFromScheme(
    __in PCYORI_STRING IniFileName,
    __in LPCTSTR SectionName,
    __out PUCHAR WindowColor
    )
{
    TCHAR Value[64];
    YORI_STRING StringValue;
    UCHAR Foreground;
    UCHAR Background;

    if (DllKernel32.pGetPrivateProfileStringW == NULL) {
        return FALSE;
    }

    StringValue.StartOfString = Value;
    StringValue.LengthAllocated = sizeof(Value)/sizeof(Value[0]);

    StringValue.LengthInChars = DllKernel32.pGetPrivateProfileStringW(SectionName, _T("Foreground"), _T(""), Value, sizeof(Value)/sizeof(Value[0]), IniFileName->StartOfString);

    if (!YoriLibLoadColorFromSchemeString(&StringValue, &Foreground)) {
        return FALSE;
    }

    StringValue.LengthInChars = DllKernel32.pGetPrivateProfileStringW(SectionName, _T("Background"), _T(""), Value, sizeof(Value)/sizeof(Value[0]), IniFileName->StartOfString);

    if (!YoriLibLoadColorFromSchemeString(&StringValue, &Background)) {
        return FALSE;
    }

    *WindowColor = (UCHAR)((Background << 4) | Foreground);
    return TRUE;
}

/**
 Load a Foreground and Background color in INTENSITY_COLOR form from an INI
 file describing the default window color.

 @param IniFileName Pointer to the INI file name.

 @param WindowColor On successful completion, updated to contain the Win32
        color including background and foreground components.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibLoadWindowColorFromScheme(
    __in PCYORI_STRING IniFileName,
    __out PUCHAR WindowColor
    )
{
    return YoriLibLoadSectionColorFromScheme(IniFileName, _T("Screen"), WindowColor);
}

/**
 Load a Foreground and Background color in INTENSITY_COLOR form from an INI
 file describing the popup color.

 @param IniFileName Pointer to the INI file name.

 @param WindowColor On successful completion, updated to contain the Win32
        color including background and foreground components.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibLoadPopupColorFromScheme(
    __in PCYORI_STRING IniFileName,
    __out PUCHAR WindowColor
    )
{
    return YoriLibLoadSectionColorFromScheme(IniFileName, _T("Popup"), WindowColor);
}

/**
 Save an array of 16 RGB values corresponding to Win32 colors to a scheme
 INI file.

 @param IniFileName Pointer to the INI file name.

 @param ColorTable Pointer to an array of 16 colors to save.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibSaveColorTableToScheme(
    __in PCYORI_STRING IniFileName,
    __in COLORREF *ColorTable
    )
{
    DWORD Index;
    DWORD ColorIndex;
    DWORD PrefixIndex;
    TCHAR ValueName[16];
    TCHAR Value[64];
    COLORREF Color;

    if (DllKernel32.pWritePrivateProfileStringW == NULL) {
        return FALSE;
    }

    for (PrefixIndex = 0; PrefixIndex < sizeof(YoriLibSchemeColorPrefixes)/sizeof(YoriLibSchemeColorPrefixes[0]); PrefixIndex++) {
        for (ColorIndex = 0; ColorIndex < sizeof(YoriLibSchemeColorNames)/sizeof(YoriLibSchemeColorNames[0]); ColorIndex++) {
            YoriLibSPrintf(ValueName, _T("%s_%s"), YoriLibSchemeColorPrefixes[PrefixIndex], YoriLibSchemeColorNames[ColorIndex]);
            Index = PrefixIndex * sizeof(YoriLibSchemeColorNames)/sizeof(YoriLibSchemeColorNames[0]) + ColorIndex;
            Color = ColorTable[Index];
            YoriLibSPrintf(Value, _T("%i, %i, %i"), GetRValue(Color), GetGValue(Color), GetBValue(Color));
            DllKernel32.pWritePrivateProfileStringW(_T("Table"), ValueName, Value, IniFileName->StartOfString);
        }
    }

    return TRUE;
}

/**
 Save a Foreground and Background color in INTENSITY_COLOR form to the
 specified section of an INI file.

 @param IniFileName Pointer to the INI file name.

 @param SectionName Pointer to the section within the INI file.

 @param WindowColor The color to save.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibSaveSectionColorToScheme(
    __in PCYORI_STRING IniFileName,
    __in LPCTSTR SectionName,
    __in UCHAR WindowColor
    )

{
    TCHAR Value[64];
    UCHAR Intensity;
    UCHAR Color;
    UCHAR Component;

    if (DllKernel32.pWritePrivateProfileStringW == NULL) {
        return FALSE;
    }

    Component = (UCHAR)(WindowColor & 0xF);

    Intensity = (UCHAR)(Component >> 3);
    Color = (UCHAR)(Component & (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE));

    YoriLibSPrintf(Value, _T("%s_%s"), YoriLibSchemeColorPrefixes[Intensity], YoriLibSchemeColorNames[Component & (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)]);
    DllKernel32.pWritePrivateProfileStringW(SectionName, _T("Foreground"), Value, IniFileName->StartOfString);

    Component = (UCHAR)((WindowColor & 0xF0) >> 4);

    Intensity = (UCHAR)(Component >> 3);
    Color = (UCHAR)(Component & (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE));

    YoriLibSPrintf(Value, _T("%s_%s"), YoriLibSchemeColorPrefixes[Intensity], YoriLibSchemeColorNames[Component & (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)]);
    DllKernel32.pWritePrivateProfileStringW(SectionName, _T("Background"), Value, IniFileName->StartOfString);

    return TRUE;
}

/**
 Save a Foreground and Background color in INTENSITY_COLOR form describing the
 default window color to the specified section of an INI file.

 @param IniFileName Pointer to the INI file name.

 @param WindowColor The color to save.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibSaveWindowColorToScheme(
    __in PCYORI_STRING IniFileName,
    __in UCHAR WindowColor
    )
{
    return YoriLibSaveSectionColorToScheme(IniFileName, _T("Screen"), WindowColor);
}

/**
 Save a Foreground and Background color in INTENSITY_COLOR form describing the
 popup color to the specified section of an INI file.

 @param IniFileName Pointer to the INI file name.

 @param WindowColor The color to save.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibSavePopupColorToScheme(
    __in PCYORI_STRING IniFileName,
    __in UCHAR WindowColor
    )
{
    return YoriLibSaveSectionColorToScheme(IniFileName, _T("Popup"), WindowColor);
}


// vim:sw=4:ts=4:et:
