/**
 * @file lib/hexdump.c
 *
 * Yori display a large hex buffer
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
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
 Generate a line of up to YORI_LIB_HEXDUMP_BYTES_PER_LINE in of bytes to
 include into a C file.

 @param Output Pointer to a string to populate with the result.

 @param Buffer Pointer to the start of the buffer.

 @param BytesToDisplay Number of bytes to display, can be equal to or less
        than YORI_LIB_HEXDUMP_BYTES_PER_LINE.

 @param MoreFollowing If TRUE, the line should be terminated with a comma
        because more data remains.  If FALSE, this is the final line and
        it should be terminated with a newline.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexByteCStyle(
    __inout PYORI_STRING Output,
    __in_ecount(BytesToDisplay) UCHAR CONST * Buffer,
    __in DWORD BytesToDisplay,
    __in BOOLEAN MoreFollowing
    )
{
    UCHAR WordToDisplay = 0;
    DWORD WordIndex;
    BOOL DisplayWord;
    DWORD OutputIndex = 0;
    YORI_STRING Subset;

    if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
        return FALSE;
    }

    Subset.StartOfString = &Output->StartOfString[OutputIndex];
    Subset.LengthAllocated = Output->LengthAllocated - OutputIndex;
    Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                           Subset.LengthAllocated,
                                           _T("        "));
    OutputIndex += Subset.LengthInChars;

    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(WordToDisplay); WordIndex++) {

        WordToDisplay = 0;
        DisplayWord = FALSE;

        if (WordIndex * sizeof(WordToDisplay) < BytesToDisplay) {
            DisplayWord = TRUE;
            WordToDisplay = (UCHAR)Buffer[WordIndex];
        }

        if (DisplayWord) {
            Subset.StartOfString = &Output->StartOfString[OutputIndex];
            Subset.LengthAllocated = Output->LengthAllocated - OutputIndex;

            if (WordIndex + 1 == BytesToDisplay && !MoreFollowing) {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T("%02x"),
                                                       WordToDisplay);
            } else {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T("%02x, "),
                                                       WordToDisplay);
            }
            OutputIndex += Subset.LengthInChars;
        }
    }
    Output->LengthInChars = OutputIndex;

    return TRUE;
}

/**
 Generate a line of up to YORI_LIB_HEXDUMP_BYTES_PER_LINE in units of one
 UCHAR.

 @param Output Pointer to a string to populate with the result.

 @param Buffer Pointer to the start of the buffer.

 @param BytesToDisplay Number of bytes to display, can be equal to or less
        than YORI_LIB_HEXDUMP_BYTES_PER_LINE.

 @param HilightBits The set of bytes that should be hilighted.  This is
        a bitmask with YORI_LIB_HEXDUMP_BYTES_PER_LINE bits where the high
        order bit corresponds to the first byte.

 @param DisplaySeperator If TRUE, display a character at the midpoint of
        each line.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexByteLine(
    __inout PYORI_STRING Output,
    __in_ecount(BytesToDisplay) UCHAR CONST * Buffer,
    __in DWORD BytesToDisplay,
    __in DWORD HilightBits,
    __in BOOLEAN DisplaySeperator
    )
{
    UCHAR WordToDisplay = 0;
    DWORD WordIndex;
    BOOL DisplayWord;
    DWORD ByteIndex;
    DWORD OutputIndex = 0;
    DWORD CurrentBit = (0x1 << (YORI_LIB_HEXDUMP_BYTES_PER_LINE - sizeof(WordToDisplay)));

    if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
        return FALSE;
    }

    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(WordToDisplay); WordIndex++) {

        if (DisplaySeperator && WordIndex == YORI_LIB_HEXDUMP_BYTES_PER_LINE / (sizeof(WordToDisplay) * 2)) {
            if (OutputIndex + 1 < Output->LengthAllocated) {
                Output->StartOfString[OutputIndex] = ':';
                Output->StartOfString[OutputIndex + 1] = ' ';
                OutputIndex += 2;
            }
        }

        WordToDisplay = 0;
        DisplayWord = FALSE;

        for (ByteIndex = 0; ByteIndex < sizeof(WordToDisplay); ByteIndex++) {
            if (WordIndex * sizeof(WordToDisplay) + ByteIndex < BytesToDisplay) {
                DisplayWord = TRUE;
                WordToDisplay = (UCHAR)(WordToDisplay + (Buffer[WordIndex * sizeof(WordToDisplay) + ByteIndex] << (ByteIndex * 8)));
            }
        }

        if (DisplayWord) {
            YORI_STRING Subset;
            Subset.StartOfString = &Output->StartOfString[OutputIndex];
            Subset.LengthAllocated = Output->LengthAllocated - OutputIndex;
            Subset.LengthInChars = 0;
            if (HilightBits) {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T("\x1b[0%sm%02x\x1b[0m "),
                                                       (HilightBits & CurrentBit)?_T(";1"):_T(""),
                                                       WordToDisplay);
            } else {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T("%02x "),
                                                       WordToDisplay);
            }
            OutputIndex += Subset.LengthInChars;
        } else {
            for (ByteIndex = 0;
                 OutputIndex < Output->LengthAllocated && ByteIndex < (sizeof(WordToDisplay) * 2 + 1);
                 ByteIndex++) {

                Output->StartOfString[OutputIndex] = ' ';
                OutputIndex++;
            }
        }

        CurrentBit = CurrentBit >> sizeof(WordToDisplay);
    }
    Output->LengthInChars = OutputIndex;

    return TRUE;
}

/**
 Generate a line of up to YORI_LIB_HEXDUMP_BYTES_PER_LINE in units of one
 WORD.

 @param Output Pointer to a string to populate with the result.

 @param Buffer Pointer to the start of the buffer.

 @param BytesToDisplay Number of bytes to display, can be equal to or less
        than YORI_LIB_HEXDUMP_BYTES_PER_LINE.

 @param HilightBits The set of bytes that should be hilighted.  This is
        a bitmask with YORI_LIB_HEXDUMP_BYTES_PER_LINE bits where the high
        order bit corresponds to the first byte.

 @param DisplaySeperator If TRUE, display a character at the midpoint of
        each line.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexWordLine(
    __inout PYORI_STRING Output,
    __in UCHAR CONST * Buffer,
    __in DWORD BytesToDisplay,
    __in DWORD HilightBits,
    __in BOOLEAN DisplaySeperator
    )
{
    WORD WordToDisplay = 0;
    DWORD WordIndex;
    BOOL DisplayWord;
    DWORD ByteIndex;
    DWORD OutputIndex = 0;
    DWORD CurrentBit = (0x3 << (YORI_LIB_HEXDUMP_BYTES_PER_LINE - sizeof(WordToDisplay)));

    if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
        return FALSE;
    }

    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(WordToDisplay); WordIndex++) {

        if (DisplaySeperator && WordIndex == YORI_LIB_HEXDUMP_BYTES_PER_LINE / (sizeof(WordToDisplay) * 2)) {
            if (OutputIndex + 1 < Output->LengthAllocated) {
                Output->StartOfString[OutputIndex] = ':';
                Output->StartOfString[OutputIndex + 1] = ' ';
                OutputIndex += 2;
            }
        }

        WordToDisplay = 0;
        DisplayWord = FALSE;

        for (ByteIndex = 0; ByteIndex < sizeof(WordToDisplay); ByteIndex++) {
            if (WordIndex * sizeof(WordToDisplay) + ByteIndex < BytesToDisplay) {
                DisplayWord = TRUE;
                WordToDisplay = (WORD)(WordToDisplay + (Buffer[WordIndex * sizeof(WordToDisplay) + ByteIndex] << (ByteIndex * 8)));
            }
        }

        if (DisplayWord) {
            YORI_STRING Subset;
            Subset.StartOfString = &Output->StartOfString[OutputIndex];
            Subset.LengthAllocated = Output->LengthAllocated - OutputIndex;
            Subset.LengthInChars = 0;
            if (HilightBits) {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T("\x1b[0%sm%04x\x1b[0m "),
                                                       (HilightBits & CurrentBit)?_T(";1"):_T(""),
                                                       WordToDisplay);
            } else {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T("%04x "),
                                                       WordToDisplay);
            }
            OutputIndex += Subset.LengthInChars;
        } else {
            for (ByteIndex = 0;
                 OutputIndex < Output->LengthAllocated && ByteIndex < (sizeof(WordToDisplay) * 2 + 1);
                 ByteIndex++) {

                Output->StartOfString[OutputIndex] = ' ';
                OutputIndex++;
            }
        }

        CurrentBit = CurrentBit >> sizeof(WordToDisplay);
    }
    Output->LengthInChars = OutputIndex;

    return TRUE;
}


/**
 Generate a line of up to YORI_LIB_HEXDUMP_BYTES_PER_LINE in units of one
 DWORD.

 @param Output Pointer to a string to populate with the result.

 @param Buffer Pointer to the start of the buffer.

 @param BytesToDisplay Number of bytes to display, can be equal to or less
        than YORI_LIB_HEXDUMP_BYTES_PER_LINE.

 @param HilightBits The set of bytes that should be hilighted.  This is
        a bitmask with YORI_LIB_HEXDUMP_BYTES_PER_LINE bits where the high
        order bit corresponds to the first byte.

 @param DisplaySeperator If TRUE, display a character at the midpoint of
        each line.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexDwordLine(
    __inout PYORI_STRING Output,
    __in UCHAR CONST * Buffer,
    __in DWORD BytesToDisplay,
    __in DWORD HilightBits,
    __in BOOLEAN DisplaySeperator
    )
{
    DWORD WordToDisplay = 0;
    DWORD WordIndex;
    BOOL DisplayWord;
    DWORD ByteIndex;
    DWORD OutputIndex = 0;
    DWORD CurrentBit = (0xf << (YORI_LIB_HEXDUMP_BYTES_PER_LINE - sizeof(WordToDisplay)));

    if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
        return FALSE;
    }

    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(WordToDisplay); WordIndex++) {

        if (DisplaySeperator && WordIndex == YORI_LIB_HEXDUMP_BYTES_PER_LINE / (sizeof(WordToDisplay) * 2)) {
            if (OutputIndex + 1 < Output->LengthAllocated) {
                Output->StartOfString[OutputIndex] = ':';
                Output->StartOfString[OutputIndex + 1] = ' ';
                OutputIndex += 2;
            }
        }

        WordToDisplay = 0;
        DisplayWord = FALSE;

        for (ByteIndex = 0; ByteIndex < sizeof(WordToDisplay); ByteIndex++) {
            if (WordIndex * sizeof(WordToDisplay) + ByteIndex < BytesToDisplay) {
                DisplayWord = TRUE;
                WordToDisplay = WordToDisplay + ((DWORD)Buffer[WordIndex * sizeof(WordToDisplay) + ByteIndex] << (ByteIndex * 8));
            }
        }

        if (DisplayWord) {
            YORI_STRING Subset;
            Subset.StartOfString = &Output->StartOfString[OutputIndex];
            Subset.LengthAllocated = Output->LengthAllocated - OutputIndex;
            Subset.LengthInChars = 0;
            if (HilightBits) {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T("\x1b[0%sm%08x\x1b[0m "),
                                                       (HilightBits & CurrentBit)?_T(";1"):_T(""),
                                                       WordToDisplay);
            } else {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T("%08x "),
                                                       WordToDisplay);
            }
            OutputIndex += Subset.LengthInChars;
        } else {
            for (ByteIndex = 0;
                 OutputIndex < Output->LengthAllocated && ByteIndex < (sizeof(WordToDisplay) * 2 + 1);
                 ByteIndex++) {

                Output->StartOfString[OutputIndex] = ' ';
                OutputIndex++;
            }
        }

        CurrentBit = CurrentBit >> sizeof(WordToDisplay);
    }
    Output->LengthInChars = OutputIndex;

    return TRUE;
}

/**
 Generate a line of up to YORI_LIB_HEXDUMP_BYTES_PER_LINE in units of one
 DWORDLONG.

 @param Output Pointer to a string to populate with the result.

 @param Buffer Pointer to the start of the buffer.

 @param BytesToDisplay Number of bytes to display, can be equal to or less
        than YORI_LIB_HEXDUMP_BYTES_PER_LINE.

 @param HilightBits The set of bytes that should be hilighted.  This is
        a bitmask with YORI_LIB_HEXDUMP_BYTES_PER_LINE bits where the high
        order bit corresponds to the first byte.

 @param DisplaySeperator If TRUE, display a character at the midpoint of
        each line.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexDwordLongLine(
    __inout PYORI_STRING Output,
    __in UCHAR CONST * Buffer,
    __in DWORD BytesToDisplay,
    __in DWORD HilightBits,
    __in BOOLEAN DisplaySeperator
    )
{
    DWORDLONG WordToDisplay = 0;
    DWORD WordIndex;
    BOOL DisplayWord;
    DWORD ByteIndex;
    DWORD OutputIndex = 0;
    DWORD CurrentBit = (0xff << (YORI_LIB_HEXDUMP_BYTES_PER_LINE - sizeof(WordToDisplay)));

    if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
        return FALSE;
    }

    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(WordToDisplay); WordIndex++) {

        if (DisplaySeperator && WordIndex == YORI_LIB_HEXDUMP_BYTES_PER_LINE / (sizeof(WordToDisplay) * 2)) {
            if (OutputIndex + 1 < Output->LengthAllocated) {
                Output->StartOfString[OutputIndex] = ':';
                Output->StartOfString[OutputIndex + 1] = ' ';
                OutputIndex += 2;
            }
        }

        WordToDisplay = 0;
        DisplayWord = FALSE;

        for (ByteIndex = 0; ByteIndex < sizeof(WordToDisplay); ByteIndex++) {
            if (WordIndex * sizeof(WordToDisplay) + ByteIndex < BytesToDisplay) {
                DisplayWord = TRUE;
                WordToDisplay = WordToDisplay + ((DWORDLONG)Buffer[WordIndex * sizeof(WordToDisplay) + ByteIndex] << (ByteIndex * 8));
            }
        }

        if (DisplayWord) {
            LARGE_INTEGER DisplayValue;
            YORI_STRING Subset;
            Subset.StartOfString = &Output->StartOfString[OutputIndex];
            Subset.LengthAllocated = Output->LengthAllocated - OutputIndex;
            Subset.LengthInChars = 0;
            DisplayValue.QuadPart = WordToDisplay;
            if (HilightBits) {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T("\x1b[0%sm%08x`%08x\x1b[0m "),
                                                       (HilightBits & CurrentBit)?_T(";1"):_T(""),
                                                       DisplayValue.HighPart,
                                                       DisplayValue.LowPart);
            } else {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T("%08x`%08x "),
                                                       DisplayValue.HighPart,
                                                       DisplayValue.LowPart);
            }
            OutputIndex += Subset.LengthInChars;
        } else {
            for (ByteIndex = 0;
                 OutputIndex < Output->LengthAllocated && ByteIndex < (sizeof(WordToDisplay) * 2 + 1);
                 ByteIndex++) {

                Output->StartOfString[OutputIndex] = ' ';
                OutputIndex++;
            }
        }
        CurrentBit = CurrentBit >> sizeof(WordToDisplay);
    }
    Output->LengthInChars = OutputIndex;

    return TRUE;
}

/**
 Display a buffer in hex format.

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
    UCHAR CharToDisplay;
    TCHAR TCharToDisplay;
    LARGE_INTEGER DisplayBufferOffset;
    YORI_STRING LineBuffer;
    YORI_STRING Subset;

    if (BytesPerWord != 1 && BytesPerWord != 2 && BytesPerWord != 4 && BytesPerWord != 8) {
        return FALSE;
    }

    //
    //  16 chars per byte: 6 chars to initiate a highlight; 4 to end it; and 4
    //  is the worst case for the data itself, being two hex digits, a space,
    //  and a character.
    //

    if (!YoriLibAllocateString(&LineBuffer, 16 * YORI_LIB_HEXDUMP_BYTES_PER_LINE + 32)) {
        return FALSE;
    }

    Subset.StartOfString = LineBuffer.StartOfString;
    Subset.LengthInChars = LineBuffer.LengthInChars;
    Subset.LengthAllocated = LineBuffer.LengthAllocated;

    DisplayBufferOffset.QuadPart = StartOfBufferOffset;

    for (LineIndex = 0; LineIndex < LineCount; LineIndex++) {

        //
        //  If the caller requested to display the buffer offset for each
        //  line, display it
        //

        if (DumpFlags & YORI_LIB_HEX_FLAG_DISPLAY_LARGE_OFFSET) {
            Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString, Subset.LengthAllocated, _T("%08x`%08x: "), DisplayBufferOffset.HighPart, DisplayBufferOffset.LowPart);
            DisplayBufferOffset.QuadPart += YORI_LIB_HEXDUMP_BYTES_PER_LINE;
        } else if (DumpFlags & YORI_LIB_HEX_FLAG_DISPLAY_OFFSET) {
            Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString, Subset.LengthAllocated, _T("%08x: "), DisplayBufferOffset.LowPart);
            DisplayBufferOffset.QuadPart += YORI_LIB_HEXDUMP_BYTES_PER_LINE;
        }

        //
        //  Advance the buffer
        //

        LineBuffer.LengthInChars += Subset.LengthInChars;
        Subset.StartOfString += Subset.LengthInChars;
        Subset.LengthAllocated -= Subset.LengthInChars;
        Subset.LengthInChars = 0;

        //
        //  Figure out how many hex bytes can be displayed on this line
        //

        BytesToDisplay = BufferLength - LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE;
        if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
            BytesToDisplay = YORI_LIB_HEXDUMP_BYTES_PER_LINE;
        }

        //
        //  Depending on the requested display format, generate the data.
        //

        if (DumpFlags & YORI_LIB_HEX_FLAG_C_STYLE) {
            if (LineIndex + 1 == LineCount) {
                YoriLibHexByteCStyle(&Subset, (CONST UCHAR *)&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE], BytesToDisplay, FALSE);
            } else {
                YoriLibHexByteCStyle(&Subset, (CONST UCHAR *)&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE], BytesToDisplay, TRUE);
            }
        } else if (BytesPerWord == 1) {
            YoriLibHexByteLine(&Subset, (CONST UCHAR *)&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE], BytesToDisplay, 0, FALSE);
        } else if (BytesPerWord == 2) {
            YoriLibHexWordLine(&Subset, (CONST UCHAR *)&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE], BytesToDisplay, 0, FALSE);
        } else if (BytesPerWord == 4) {
            YoriLibHexDwordLine(&Subset, (CONST UCHAR *)&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE], BytesToDisplay, 0, FALSE);
        } else if (BytesPerWord == 8) {
            YoriLibHexDwordLongLine(&Subset, (CONST UCHAR *)&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE], BytesToDisplay, 0, FALSE);
        }

        //
        //  Advance the buffer
        //

        LineBuffer.LengthInChars += Subset.LengthInChars;
        Subset.StartOfString += Subset.LengthInChars;
        Subset.LengthAllocated -= Subset.LengthInChars;
        Subset.LengthInChars = 0;

        //
        //  If the caller requested characters after the hex output, generate
        //  those.
        //

        if (DumpFlags & (YORI_LIB_HEX_FLAG_DISPLAY_CHARS | YORI_LIB_HEX_FLAG_DISPLAY_WCHARS)) {
            if (LineBuffer.LengthInChars < LineBuffer.LengthAllocated) {
                Subset.StartOfString[0] = ' ';
                Subset.StartOfString++;
                LineBuffer.LengthInChars++;
            }
            if (DumpFlags & YORI_LIB_HEX_FLAG_DISPLAY_CHARS) {
                for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE; WordIndex++) {
                    if (WordIndex < BytesToDisplay) {
                        CharToDisplay = Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE + WordIndex];
                        if (!YoriLibIsCharPrintable(CharToDisplay)) {
                            CharToDisplay = '.';
                        }
                    } else {
                        CharToDisplay = ' ';
                    }
                    Subset.StartOfString[0] = CharToDisplay;
                    Subset.StartOfString++;
                    LineBuffer.LengthInChars++;
                    if (LineBuffer.LengthInChars == LineBuffer.LengthAllocated) {
                        break;
                    }
                }
            } else {
                UCHAR Low;
                UCHAR High;
                for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(TCHAR); WordIndex++) {
                    if (WordIndex * sizeof(TCHAR) < BytesToDisplay) {
                        Low = Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE + WordIndex * sizeof(TCHAR)];
                        High = Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE + WordIndex * sizeof(TCHAR) + 1];
                        TCharToDisplay = (TCHAR)((High << 8) + Low);
                        if (!YoriLibIsCharPrintable(TCharToDisplay)) {
                            TCharToDisplay = '.';
                        }
                    } else {
                        TCharToDisplay = ' ';
                    }
                    Subset.StartOfString[0] = TCharToDisplay;
                    Subset.StartOfString++;
                    LineBuffer.LengthInChars++;
                    if (LineBuffer.LengthInChars == LineBuffer.LengthAllocated) {
                        break;
                    }
                }
            }
        }

        if (LineBuffer.LengthInChars < LineBuffer.LengthAllocated) {
            Subset.StartOfString[0] = '\n';
            Subset.StartOfString++;
            LineBuffer.LengthInChars++;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &LineBuffer);
        LineBuffer.LengthInChars = 0;
        Subset.StartOfString = LineBuffer.StartOfString;
        Subset.LengthInChars = LineBuffer.LengthInChars;
        Subset.LengthAllocated = LineBuffer.LengthAllocated;
    }

    YoriLibFreeStringContents(&LineBuffer);
    return TRUE;
}

/**
 Display two buffers side by side in hex format.

 @param StartOfBufferOffset If the buffer displayed to this call is part of
        a larger logical stream of data, this value indicates the offset of
        this buffer within the larger logical stream.  This is used for
        display only.

 @param Buffer1 Pointer to the first buffer to display.

 @param Buffer1Length The length of the first buffer, in bytes.

 @param Buffer2 Pointer to the second buffer to display.

 @param Buffer2Length The length of the second buffer, in bytes.

 @param BytesPerWord The number of bytes to display at a time.

 @param DumpFlags Flags for the operation.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexDiff(
    __in LONGLONG StartOfBufferOffset,
    __in LPCSTR Buffer1,
    __in DWORD Buffer1Length,
    __in LPCSTR Buffer2,
    __in DWORD Buffer2Length,
    __in DWORD BytesPerWord,
    __in DWORD DumpFlags
    )
{
    DWORD LineCount;
    DWORD LineIndex;
    DWORD WordIndex;
    DWORD BufferIndex;
    DWORD BytesToDisplay;
    DWORD HilightBits;
    DWORD CurrentBit;
    UCHAR CharToDisplay;
    TCHAR TCharToDisplay;
    LARGE_INTEGER DisplayBufferOffset;
    LPCSTR BufferToDisplay;
    LPCSTR Buffers[2];
    DWORD BufferLengths[2];
    YORI_STRING LineBuffer;
    YORI_STRING Subset;

    if (BytesPerWord != 1 && BytesPerWord != 2 && BytesPerWord != 4 && BytesPerWord != 8) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&LineBuffer, 32 * YORI_LIB_HEXDUMP_BYTES_PER_LINE + 64)) {
        return FALSE;
    }

    Subset.StartOfString = LineBuffer.StartOfString;
    Subset.LengthInChars = LineBuffer.LengthInChars;
    Subset.LengthAllocated = LineBuffer.LengthAllocated;

    DisplayBufferOffset.QuadPart = StartOfBufferOffset;

    if (Buffer1Length > Buffer2Length) {
        LineCount = (Buffer1Length + YORI_LIB_HEXDUMP_BYTES_PER_LINE - 1) / YORI_LIB_HEXDUMP_BYTES_PER_LINE;
    } else {
        LineCount = (Buffer2Length + YORI_LIB_HEXDUMP_BYTES_PER_LINE - 1) / YORI_LIB_HEXDUMP_BYTES_PER_LINE;
    }

    Buffers[0] = Buffer1;
    Buffers[1] = Buffer2;
    BufferLengths[0] = Buffer1Length;
    BufferLengths[1] = Buffer2Length;

    for (LineIndex = 0; LineIndex < LineCount; LineIndex++) {

        //
        //  If the caller requested to display the buffer offset for each
        //  line, display it
        //

        if (DumpFlags & YORI_LIB_HEX_FLAG_DISPLAY_LARGE_OFFSET) {
            Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString, Subset.LengthAllocated, _T("%08x`%08x: "), DisplayBufferOffset.HighPart, DisplayBufferOffset.LowPart);
            DisplayBufferOffset.QuadPart += YORI_LIB_HEXDUMP_BYTES_PER_LINE;
        } else if (DumpFlags & YORI_LIB_HEX_FLAG_DISPLAY_OFFSET) {
            Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString, Subset.LengthAllocated, _T("%08x: "), DisplayBufferOffset.LowPart);
            DisplayBufferOffset.QuadPart += YORI_LIB_HEXDUMP_BYTES_PER_LINE;
        }

        //
        //  Advance the buffer
        //

        LineBuffer.LengthInChars += Subset.LengthInChars;
        Subset.StartOfString += Subset.LengthInChars;
        Subset.LengthAllocated -= Subset.LengthInChars;
        Subset.LengthInChars = 0;

        //
        //  For this line, calculate a set of bits corresponding to bytes
        //  that are different
        //

        HilightBits = 0;
        for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE; WordIndex++) {
            HilightBits = HilightBits << 1;
            BytesToDisplay = LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE + WordIndex;
            if (BufferLengths[0] > BytesToDisplay && BufferLengths[1] > BytesToDisplay) {
                if (Buffers[0][BytesToDisplay] != Buffers[1][BytesToDisplay]) {
                    HilightBits = HilightBits | 1;
                }
            } else {
                HilightBits = HilightBits | 1;
            }
        }

        for (BufferIndex = 0; BufferIndex < 2; BufferIndex++) {

            //
            //  Figure out how many hex bytes can be displayed on this line
            //

            if (LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE >= BufferLengths[BufferIndex]) {
                BytesToDisplay = 0;
                BufferToDisplay = NULL;
            } else {
                BytesToDisplay = BufferLengths[BufferIndex] - LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE;
                BufferToDisplay = &Buffers[BufferIndex][LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE];
                if (BytesToDisplay > YORI_LIB_HEXDUMP_BYTES_PER_LINE) {
                    BytesToDisplay = YORI_LIB_HEXDUMP_BYTES_PER_LINE;
                }
            }


            //
            //  Depending on the requested display format, display the data.
            //

            if (BytesPerWord == 1) {
                YoriLibHexByteLine(&Subset, (CONST UCHAR *)BufferToDisplay, BytesToDisplay, HilightBits, TRUE);
            } else if (BytesPerWord == 2) {
                YoriLibHexWordLine(&Subset, (CONST UCHAR *)BufferToDisplay, BytesToDisplay, HilightBits, TRUE);
            } else if (BytesPerWord == 4) {
                YoriLibHexDwordLine(&Subset, (CONST UCHAR *)BufferToDisplay, BytesToDisplay, HilightBits, TRUE);
            } else if (BytesPerWord == 8) {
                YoriLibHexDwordLongLine(&Subset, (CONST UCHAR *)BufferToDisplay, BytesToDisplay, HilightBits, TRUE);
            }

            //
            //  Advance the buffer
            //

            LineBuffer.LengthInChars += Subset.LengthInChars;
            Subset.StartOfString += Subset.LengthInChars;
            Subset.LengthAllocated -= Subset.LengthInChars;
            Subset.LengthInChars = 0;

            //
            //  If the caller requested characters after the hex output,
            //  generate them.
            //

            if (DumpFlags & (YORI_LIB_HEX_FLAG_DISPLAY_CHARS | YORI_LIB_HEX_FLAG_DISPLAY_WCHARS)) {
                if (LineBuffer.LengthInChars < LineBuffer.LengthAllocated) {
                    Subset.StartOfString[0] = ' ';
                    Subset.StartOfString++;
                    LineBuffer.LengthInChars++;
                }
                if (DumpFlags & YORI_LIB_HEX_FLAG_DISPLAY_CHARS) {
                    CurrentBit = (0x1 << (YORI_LIB_HEXDUMP_BYTES_PER_LINE - sizeof(CharToDisplay)));
                    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE; WordIndex++) {
                        if (WordIndex < BytesToDisplay) {
                            CharToDisplay = BufferToDisplay[WordIndex];
                            if (CharToDisplay < 32) {
                                CharToDisplay = '.';
                            }
                        } else {
                            CharToDisplay = ' ';
                        }

                        Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                               Subset.LengthAllocated,
                                                               _T("\x1b[0%sm%c"),
                                                               (HilightBits & CurrentBit)?_T(";1"):_T(""),
                                                               CharToDisplay);

                        LineBuffer.LengthInChars += Subset.LengthInChars;
                        Subset.StartOfString += Subset.LengthInChars;
                        Subset.LengthAllocated -= Subset.LengthInChars;
                        Subset.LengthInChars = 0;

                        CurrentBit = CurrentBit >> sizeof(CharToDisplay);
                    }
                } else {
                    UCHAR Low;
                    UCHAR High;
                    CurrentBit = (0x3 << (YORI_LIB_HEXDUMP_BYTES_PER_LINE - sizeof(TCharToDisplay)));
                    for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / sizeof(TCHAR); WordIndex++) {
                        if (WordIndex < BytesToDisplay) {
                            Low = BufferToDisplay[WordIndex * sizeof(TCHAR)];
                            High = BufferToDisplay[WordIndex * sizeof(TCHAR) + 1];
                            TCharToDisplay = (TCHAR)((High << 8) + Low);
                            if (TCharToDisplay < 32) {
                                TCharToDisplay = '.';
                            }
                        } else {
                            TCharToDisplay = ' ';
                        }

                        Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                               Subset.LengthAllocated,
                                                               _T("\x1b[0%sm%c"),
                                                               (HilightBits & CurrentBit)?_T(";1"):_T(""),
                                                               TCharToDisplay);

                        LineBuffer.LengthInChars += Subset.LengthInChars;
                        Subset.StartOfString += Subset.LengthInChars;
                        Subset.LengthAllocated -= Subset.LengthInChars;
                        Subset.LengthInChars = 0;

                        CurrentBit = CurrentBit >> sizeof(TCharToDisplay);
                    }
                }
            }

            if (BufferIndex == 0) {
                Subset.LengthInChars = YoriLibSPrintfS(Subset.StartOfString,
                                                       Subset.LengthAllocated,
                                                       _T(" | "));
                LineBuffer.LengthInChars += Subset.LengthInChars;
                Subset.StartOfString += Subset.LengthInChars;
                Subset.LengthAllocated -= Subset.LengthInChars;
                Subset.LengthInChars = 0;
            }
        }

        if (LineBuffer.LengthInChars < LineBuffer.LengthAllocated) {
            Subset.StartOfString[0] = '\n';
            Subset.StartOfString++;
            LineBuffer.LengthInChars++;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &LineBuffer);
        LineBuffer.LengthInChars = 0;
        Subset.StartOfString = LineBuffer.StartOfString;
        Subset.LengthInChars = LineBuffer.LengthInChars;
        Subset.LengthAllocated = LineBuffer.LengthAllocated;
    }

    YoriLibFreeStringContents(&LineBuffer);
    return TRUE;
}


// vim:sw=4:ts=4:et:
