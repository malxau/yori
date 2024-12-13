/**
 * @file libwin/text.c
 *
 * Yori window text rendering support
 *
 * Copyright (c) 2020-2024 Malcolm J. Smith
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
 Calculate a range of cells on a single line to display.  This is often the
 exact string as the input, but can diverge due to display requirements such
 as tab expansion or wide characters.

 @param WinMgr Pointer to the window manager.
 
 @param String The string of text to display.

 @param LeftPadding The number of cells to pad on the left of the string.

 @param TabWidth The number of cells to display a tab.

 @param MaxCells The maximum number of cells to display.

 @param CellsString On input, contains an initialized string that may have
        a buffer to populate.  On successful completion, this will contain
        the cells to display.  Note the buffer within this string may be
        reallocated within this routine.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinTextStringToDisplayCells(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr,
    __in PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T LeftPadding,
    __in YORI_ALLOC_SIZE_T TabWidth,
    __in YORI_ALLOC_SIZE_T MaxCells,
    __inout PYORI_STRING CellsString
    )
{
    YORI_ALLOC_SIZE_T CharIndex;
    YORI_ALLOC_SIZE_T CellsDisplayed;
    TCHAR Char;
    BOOLEAN NeedDoubleBuffer;
    BOOLEAN DoubleWideCharSupported;
    BOOLEAN IsNanoServer;

    IsNanoServer = YoriLibIsNanoServer();
    DoubleWideCharSupported = YoriWinIsDoubleWideCharSupported(WinMgr);

    CellsDisplayed = 0;
    NeedDoubleBuffer = FALSE;

    //
    //  Count how many chars are required to fill the viewport.  If the
    //  chars are all single width and not tab, the line buffer can be
    //  used directly.  Otherwise, count the size of the buffer needed.
    //  Note this sizing can be pessimistic (assume wide chars fit, tabs
    //  are fully expanded.)
    //

    if (LeftPadding > 0) {
        NeedDoubleBuffer = TRUE;
        CellsDisplayed = LeftPadding;
    }

    for (CharIndex = 0;
         CharIndex < String->LengthInChars && CellsDisplayed < MaxCells;
         CharIndex++) {

        Char = String->StartOfString[CharIndex];

        if (Char == '\t') {
            NeedDoubleBuffer = TRUE;
            CellsDisplayed = CellsDisplayed + TabWidth;
        } else if (DoubleWideCharSupported && YoriLibIsDoubleWideChar(Char)) {
            NeedDoubleBuffer = TRUE;
            CellsDisplayed = CellsDisplayed + 2;
        } else if (IsNanoServer && Char == '\0') {
            NeedDoubleBuffer = TRUE;
            CellsDisplayed++;
        } else {
            CellsDisplayed++;
        }
    }

    //
    //  If the caller did not supply a buffer and no double buffer is needed,
    //  point to the original string.
    //

    if (!NeedDoubleBuffer && CellsString->StartOfString == NULL) {
        CellsString->StartOfString = String->StartOfString;
        CellsString->LengthInChars = CellsDisplayed;
        CellsString->LengthAllocated = CellsDisplayed;
        CellsString->MemoryToFree = String->MemoryToFree;
        if (CellsString->MemoryToFree != NULL) {
            YoriLibReference(CellsString->MemoryToFree);
        }
        return TRUE;
    }

    //
    //  If the caller's string is not large enough, attempt to reallocate.
    //

    if (CellsString->LengthAllocated < CellsDisplayed) {
        if (!YoriLibReallocStringNoContents(CellsString, CellsDisplayed)) {
            return FALSE;
        }
    }

    for (CellsDisplayed = 0; CellsDisplayed < LeftPadding; CellsDisplayed++) {
        CellsString->StartOfString[CellsDisplayed] = ' ';
    }

    for (CharIndex = 0;
         CharIndex < String->LengthInChars && CellsDisplayed < MaxCells;
         CharIndex++) {

        Char = String->StartOfString[CharIndex];

        if (Char == '\t') {
            YORI_ALLOC_SIZE_T TabIndex;
            for (TabIndex = 0; TabIndex < TabWidth && CellsDisplayed < MaxCells; TabIndex++) {
                CellsString->StartOfString[CellsDisplayed] = ' ';
                CellsDisplayed++;
            }
        } else if (DoubleWideCharSupported && YoriLibIsDoubleWideChar(Char)) {
            if (CellsDisplayed + 1 < MaxCells) {
                CellsString->StartOfString[CellsDisplayed] = Char;
                CellsDisplayed++;
                CellsString->StartOfString[CellsDisplayed] = ' ';
                CellsDisplayed++;
            } else {
                CellsString->StartOfString[CellsDisplayed] = ' ';
                CellsDisplayed++;
            }
        } else if (IsNanoServer && Char == '\0') {
            CellsString->StartOfString[CellsDisplayed] = ' ';
            CellsDisplayed++;
        } else {
            CellsString->StartOfString[CellsDisplayed] = Char;
            CellsDisplayed++;
        }
    }

    CellsString->LengthInChars = CellsDisplayed;
    return TRUE;
}

