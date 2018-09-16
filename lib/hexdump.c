/**
 * @file lib/hexdump.c
 *
 * Yori display a large hex buffer
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
 The number of bytes of data to display in hex form per line.
 */
#define YORI_LIB_HEXDUMP_BYTES_PER_LINE 16

/**
 Display a line of up to YORI_LIB_HEXDUMP_BYTES_PER_LINE in units of one
 UCHAR.

 @param Buffer Pointer to the start of the buffer.

 @param BytesToDisplay Number of bytes to display, can be equal to or less
        than YORI_LIB_HEXDUMP_BYTES_PER_LINE.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexByteLine(
    __in LPCSTR Buffer,
    __in DWORD BytesToDisplay
    )
{
    UCHAR WordToDisplay = 0;
    DWORD WordIndex;
    BOOL DisplayWord;
    DWORD ByteIndex;

    if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
        return FALSE;
    }

    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(WordToDisplay); WordIndex++) {

        if (WordIndex == YORI_LIB_HEXDUMP_BYTES_PER_LINE / (sizeof(WordToDisplay) * 2)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("| "));
        }

        WordToDisplay = 0;
        DisplayWord = FALSE;

        for (ByteIndex = 0; ByteIndex < sizeof(WordToDisplay); ByteIndex++) {
            if (WordIndex * sizeof(WordToDisplay) + ByteIndex < BytesToDisplay) {
                DisplayWord = TRUE;
                WordToDisplay = (UCHAR)(WordToDisplay << 8);
                WordToDisplay = (UCHAR)(WordToDisplay + Buffer[WordIndex * sizeof(WordToDisplay) + ByteIndex]);
            }
        }
        
        if (DisplayWord) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%02x "), WordToDisplay);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("   "));
        }
    }

    return TRUE;
}

/**
 Display a line of up to YORI_LIB_HEXDUMP_BYTES_PER_LINE in units of one
 WORD.

 @param Buffer Pointer to the start of the buffer.

 @param BytesToDisplay Number of bytes to display, can be equal to or less
        than YORI_LIB_HEXDUMP_BYTES_PER_LINE.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexWordLine(
    __in LPCSTR Buffer,
    __in DWORD BytesToDisplay
    )
{
    WORD WordToDisplay = 0;
    DWORD WordIndex;
    BOOL DisplayWord;
    DWORD ByteIndex;

    if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
        return FALSE;
    }

    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(WordToDisplay); WordIndex++) {

        if (WordIndex == YORI_LIB_HEXDUMP_BYTES_PER_LINE / (sizeof(WordToDisplay) * 2)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("| "));
        }

        WordToDisplay = 0;
        DisplayWord = FALSE;

        for (ByteIndex = 0; ByteIndex < sizeof(WordToDisplay); ByteIndex++) {
            if (WordIndex * sizeof(WordToDisplay) + ByteIndex < BytesToDisplay) {
                DisplayWord = TRUE;
                WordToDisplay = (WORD)(WordToDisplay << 8);
                WordToDisplay = (WORD)(WordToDisplay + Buffer[WordIndex * sizeof(WordToDisplay) + ByteIndex]);
            }
        }
        
        if (DisplayWord) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%04x "), WordToDisplay);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("     "));
        }
    }

    return TRUE;
}


/**
 Display a line of up to YORI_LIB_HEXDUMP_BYTES_PER_LINE in units of one
 DWORD.

 @param Buffer Pointer to the start of the buffer.

 @param BytesToDisplay Number of bytes to display, can be equal to or less
        than YORI_LIB_HEXDUMP_BYTES_PER_LINE.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexDwordLine(
    __in LPCSTR Buffer,
    __in DWORD BytesToDisplay
    )
{
    DWORD WordToDisplay = 0;
    DWORD WordIndex;
    BOOL DisplayWord;
    DWORD ByteIndex;

    if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
        return FALSE;
    }

    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(WordToDisplay); WordIndex++) {

        if (WordIndex == YORI_LIB_HEXDUMP_BYTES_PER_LINE / (sizeof(WordToDisplay) * 2)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("| "));
        }

        WordToDisplay = 0;
        DisplayWord = FALSE;

        for (ByteIndex = 0; ByteIndex < sizeof(WordToDisplay); ByteIndex++) {
            if (WordIndex * sizeof(WordToDisplay) + ByteIndex < BytesToDisplay) {
                DisplayWord = TRUE;
                WordToDisplay = WordToDisplay << 8;
                WordToDisplay = WordToDisplay + Buffer[WordIndex * sizeof(WordToDisplay) + ByteIndex];
            }
        }
        
        if (DisplayWord) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%08x "), WordToDisplay);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("         "));
        }
    }

    return TRUE;
}

/**
 Display a line of up to YORI_LIB_HEXDUMP_BYTES_PER_LINE in units of one
 DWORDLONG.

 @param Buffer Pointer to the start of the buffer.

 @param BytesToDisplay Number of bytes to display, can be equal to or less
        than YORI_LIB_HEXDUMP_BYTES_PER_LINE.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexDwordLongLine(
    __in LPCSTR Buffer,
    __in DWORD BytesToDisplay
    )
{
    DWORDLONG WordToDisplay = 0;
    DWORD WordIndex;
    BOOL DisplayWord;
    DWORD ByteIndex;

    if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
        return FALSE;
    }

    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(WordToDisplay); WordIndex++) {

        if (WordIndex == YORI_LIB_HEXDUMP_BYTES_PER_LINE / (sizeof(WordToDisplay) * 2)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("| "));
        }

        WordToDisplay = 0;
        DisplayWord = FALSE;

        for (ByteIndex = 0; ByteIndex < sizeof(WordToDisplay); ByteIndex++) {
            if (WordIndex * sizeof(WordToDisplay) + ByteIndex < BytesToDisplay) {
                DisplayWord = TRUE;
                WordToDisplay = WordToDisplay << 8;
                WordToDisplay = WordToDisplay + Buffer[WordIndex * sizeof(WordToDisplay) + ByteIndex];
            }
        }
        
        if (DisplayWord) {
            LARGE_INTEGER DisplayWord;
            DisplayWord.QuadPart = WordToDisplay;
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%08x`%08x "), DisplayWord.HighPart, DisplayWord.LowPart);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("                  "));
        }
    }

    return TRUE;
}

/**
 Display a buffer in hex format.  This routine is currently
 very limited and only supports one byte printing with chars
 and four byte printing without chars, and four byte printing
 assumes alignment.
 
 @param Buffer Pointer to the buffer to display.

 @param StartOfBufferOffset If the buffer displayed to this call is part of
        a larger logical stream of data, this value indicates the offset of
        this buffer within the larger logical stream.  This is used for
        display only.

 @param BufferLength The length of the buffer, in bytes.

 @param BytesPerWord The number of bytes to display at a time.

 @param DumpFlags Flags for the operation.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexDump(
    __in LPCSTR Buffer,
    __in LONGLONG StartOfBufferOffset,
    __in DWORD BufferLength,
    __in DWORD BytesPerWord,
    __in DWORD DumpFlags
    )
{
    DWORD LineCount = (BufferLength + YORI_LIB_HEXDUMP_BYTES_PER_LINE - 1) / YORI_LIB_HEXDUMP_BYTES_PER_LINE;
    DWORD LineIndex;
    DWORD WordIndex;
    DWORD BytesToDisplay;
    DWORD BytesRemaining;
    CHAR CharToDisplay;
    LARGE_INTEGER DisplayBufferOffset;

    BytesRemaining = BufferLength;

    if (BytesPerWord != 1 && BytesPerWord != 2 && BytesPerWord != 4 && BytesPerWord != 8) {
        return FALSE;
    }

    DisplayBufferOffset.QuadPart = StartOfBufferOffset;

    for (LineIndex = 0; LineIndex < LineCount; LineIndex++) {

        //
        //  If the caller requested to display the buffer offset for each
        //  line, display it
        //

        if (DumpFlags & YORI_LIB_HEX_FLAG_DISPLAY_LARGE_OFFSET) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%08x`%08x: "), DisplayBufferOffset.HighPart, DisplayBufferOffset.LowPart);
            DisplayBufferOffset.QuadPart += YORI_LIB_HEXDUMP_BYTES_PER_LINE;
        } else if (DumpFlags & YORI_LIB_HEX_FLAG_DISPLAY_OFFSET) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%08x: "), DisplayBufferOffset.LowPart);
            DisplayBufferOffset.QuadPart += YORI_LIB_HEXDUMP_BYTES_PER_LINE;
        }

        //
        //  Figure out how many hex bytes can be displayed on this line
        //

        BytesToDisplay = BufferLength - LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE;
        if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
            BytesToDisplay = YORI_LIB_HEXDUMP_BYTES_PER_LINE;
        }

        //
        //  Depending on the requested display format, display the data.
        //

        if (BytesPerWord == 1) {
            YoriLibHexByteLine(&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE], BytesToDisplay);
        } else if (BytesPerWord == 2) {
            YoriLibHexWordLine(&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE], BytesToDisplay);
        } else if (BytesPerWord == 4) {
            YoriLibHexDwordLine(&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE], BytesToDisplay);
        } else if (BytesPerWord == 8) {
            YoriLibHexDwordLongLine(&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE], BytesToDisplay);
        }

        //
        //  If the caller requested characters after the hex output, display
        //  it.
        //

        if (DumpFlags & YORI_LIB_HEX_FLAG_DISPLAY_CHARS) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T(" "));
            for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE; WordIndex++) {
                if (WordIndex < BytesToDisplay) {
                    CharToDisplay = Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE + WordIndex];
                    if (CharToDisplay < 32) {
                        CharToDisplay = '.';
                    }
                } else {
                    CharToDisplay = ' ';
                }
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%c"), CharToDisplay);
            }
        }

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
    }

    return TRUE;
}


// vim:sw=4:ts=4:et:
