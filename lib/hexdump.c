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
 Display a buffer in hex format.  This routine is currently
 very limited and only supports one byte printing with chars
 and four byte printing without chars, and four byte printing
 assumes alignment.
 
 @param Buffer Pointer to the buffer to display.

 @param BufferLength The length of the buffer, in bytes.

 @param BytesPerWord The number of bytes to display at a time.

 @param DumpFlags Flags for the operation.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibHexDump(
    __in LPCSTR Buffer,
    __in DWORD BufferLength,
    __in DWORD BytesPerWord,
    __in DWORD DumpFlags
    )
{
    DWORD LineCount = (BufferLength + YORI_LIB_HEXDUMP_BYTES_PER_LINE - 1) / YORI_LIB_HEXDUMP_BYTES_PER_LINE;
    DWORD LineIndex;
    DWORD WordIndex;
    DWORD BytesRemaining;

    BytesRemaining = BufferLength;
    UNREFERENCED_PARAMETER(DumpFlags);

    if (BytesPerWord != 1 && BytesPerWord != 4) {
        return FALSE;
    }

    for (LineIndex = 0; LineIndex < LineCount; LineIndex++) {

        if (BytesPerWord == 1) {
            for (WordIndex = 0; WordIndex < BytesRemaining && WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE; WordIndex++) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%02x "), Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE + WordIndex]);
            }

            for (; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE; WordIndex++) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("   "));
            }
            for (WordIndex = 0; WordIndex < BytesRemaining && WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE; WordIndex++) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%c"), Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE + WordIndex]);
            }
        } else if (BytesPerWord == 4) {
            DWORD WordToDisplay = 0;
            for (WordIndex = 0; WordIndex < YORI_LIB_HEXDUMP_BYTES_PER_LINE / BytesPerWord; WordIndex++) {
                if (LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE + WordIndex * BytesPerWord > BufferLength) {
                    break;
                }

                //
                // MSFIX Want to be smarter on unaligned buffers
                //

                WordToDisplay = *(PDWORD)&Buffer[LineIndex * YORI_LIB_HEXDUMP_BYTES_PER_LINE + WordIndex * sizeof(DWORD)];

                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%08x "), WordToDisplay);
            }
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
    }

    return TRUE;
}


// vim:sw=4:ts=4:et:
