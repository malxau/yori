/**
 * @file lib/bargraph.c
 *
 * Yori bar graph for disk space, battery, memory, etc.
 *
 * Copyright (c) 2017-2023 Malcolm J. Smith
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
 Display a bar graph indicating resource utilization.

 @param hOutput Handle to the device to output the graph to.

 @param TenthsOfPercent The fraction of the graph to fill, where the entire
        graph would be described by 1000 units.

 @param GreenThreshold If TenthsOfPercent is beyond this value, the graph
        should be displayed as green, indicating ample resources.  Whether
        the value is above or below this threshold to display green is
        determined by whether this value is above or below the red threshold.

 @param RedThreshold If TenthsOfPercent is beyond this value, the graph
        should be displayed as red, indicating resource scarcity.  Whether
        the value is above or below this threshold to display red is
        determined by whether this value is above or below the green
        threshold.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibDisplayBarGraph(
    __in HANDLE hOutput,
    __in DWORD TenthsOfPercent,
    __in DWORD GreenThreshold,
    __in DWORD RedThreshold
    )
{
    YORI_STRING Subset;
    YORI_STRING Line;
    WORD Background;
    YORI_ALLOC_SIZE_T TotalBarSize;
    YORI_ALLOC_SIZE_T CharsRequired;
    DWORD WholeBarsSet;
    DWORD PartialBarsSet;
    DWORD Index;
    UCHAR Ctrl;
    USHORT Color;
    BOOLEAN LowerIsBetter;
    BOOL SupportsColor;
    BOOL SupportsExtendedChars;
    WORD Width;

    Width = 0;
    YoriLibGetWindowDimensions(hOutput, &Width, NULL);

    //
    //  This view contains four cells of overhead (space plus bracket on each
    //  end.)  Having fewer than around 10 cells would make a meaningless
    //  display.
    //

    if (Width < 10) {
        return FALSE;
    }

    SupportsColor = FALSE;
    SupportsExtendedChars = FALSE;

    if (!YoriLibQueryConsoleCapabilities(hOutput, &SupportsColor, &SupportsExtendedChars, NULL)) {
        SupportsColor = FALSE;
        SupportsExtendedChars = FALSE;
    }

    CharsRequired = (YORI_ALLOC_SIZE_T)(Width + YORI_MAX_VT_ESCAPE_CHARS * 2);
    if (!YoriLibAllocateString(&Line, CharsRequired)) {
        return FALSE;
    }

    Line.StartOfString[0] = ' ';
    Line.StartOfString[1] = '[';

    YoriLibInitEmptyString(&Subset);
    Subset.StartOfString = &Line.StartOfString[2];
    Subset.LengthAllocated = (YORI_ALLOC_SIZE_T)(Line.LengthAllocated - 2);

    Ctrl = YORILIB_ATTRCTRL_WINDOW_BG;
    Background = (USHORT)(YoriLibVtGetDefaultColor() & 0xF0);

    LowerIsBetter = FALSE;
    if (GreenThreshold < RedThreshold) {
        LowerIsBetter = TRUE;
    }

    if (SupportsColor) {
        if (LowerIsBetter) {
            if (TenthsOfPercent <= GreenThreshold) {
                Color = (USHORT)(Background | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            } else if (TenthsOfPercent >= RedThreshold) {
                Color = (USHORT)(Background | FOREGROUND_RED | FOREGROUND_INTENSITY);
            } else {
                Color = (USHORT)(Background | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            }
        } else {
            if (TenthsOfPercent >= GreenThreshold) {
                Color = (USHORT)(Background | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            } else if (TenthsOfPercent <= RedThreshold) {
                Color = (USHORT)(Background | FOREGROUND_RED | FOREGROUND_INTENSITY);
            } else {
                Color = (USHORT)(Background | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            }
        }

        YoriLibVtStringForTextAttribute(&Subset, Ctrl, Color);
    }

    Line.LengthInChars = (YORI_ALLOC_SIZE_T)(2 + Subset.LengthInChars);

    TotalBarSize = (YORI_ALLOC_SIZE_T)(Width - 4);
    PartialBarsSet = TotalBarSize * TenthsOfPercent / 500;
    WholeBarsSet = PartialBarsSet / 2;
    PartialBarsSet = PartialBarsSet % 2;

    if (SupportsExtendedChars) {
        for (Index = 0; Index < WholeBarsSet; Index++) {
            Line.StartOfString[Index + Line.LengthInChars] = 0x2588;
        }
        if (PartialBarsSet) {
            Line.StartOfString[Index + Line.LengthInChars] = 0x258c;
            Index++;
        }
    } else {
        for (Index = 0; Index < WholeBarsSet; Index++) {
            Line.StartOfString[Index + Line.LengthInChars] = '#';
        }
    }
    for (; Index < TotalBarSize; Index++) {
        Line.StartOfString[Index + Line.LengthInChars] = ' ';
    }
    Line.LengthInChars = (YORI_ALLOC_SIZE_T)(Line.LengthInChars + TotalBarSize);

    Subset.StartOfString = &Line.StartOfString[Line.LengthInChars];
    Subset.LengthAllocated = (YORI_ALLOC_SIZE_T)(Line.LengthAllocated - Line.LengthInChars);

    if (SupportsColor) {
        Subset.LengthInChars = (YORI_ALLOC_SIZE_T)YoriLibSPrintf(Subset.StartOfString, _T("%c[0m]\n"), 27);
    } else {
        Subset.LengthInChars = (YORI_ALLOC_SIZE_T)YoriLibSPrintf(Subset.StartOfString, _T("]\n"));
    }
    Line.LengthInChars = (YORI_ALLOC_SIZE_T)(Line.LengthInChars + Subset.LengthInChars);

    YoriLibOutputToDevice(hOutput, 0, _T("%y"), &Line);
    YoriLibFreeStringContents(&Line);

    return TRUE;
}

// vim:sw=4:ts=4:et:
