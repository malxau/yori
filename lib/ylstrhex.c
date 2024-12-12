/**
 * @file lib/ylstrhex.c
 *
 * Yori string to hex routines
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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
 Parse a string containing hex digits and generate a string that encodes each
 two hex digits into a byte.

 @param String Pointer to the string to parse.

 @param Buffer Pointer to a buffer to populate with a string representation of
        the hex characters.  Note this string is always 8 bit, not TCHARs.

 @param BufferSize Specifies the length of Buffer, in bytes.

 @return TRUE to indicate parse success, FALSE to indicate failure.  Failure
         occurs if the input stream is not compliant hex, or is not aligned in
         two character pairs.
 */
__success(return)
BOOL
YoriLibStringToHexBuffer(
    __in PYORI_STRING String,
    __out_ecount(BufferSize) PUCHAR Buffer,
    __in YORI_ALLOC_SIZE_T BufferSize
    )
{
    UCHAR DestChar;
    TCHAR SourceChar;
    YORI_ALLOC_SIZE_T Offset;
    YORI_ALLOC_SIZE_T Digit;
    YORI_ALLOC_SIZE_T StrIndex;

    //
    //  Loop through the output string
    //

    StrIndex = 0;
    for (Offset = 0; Offset < BufferSize; Offset++) {

        DestChar = 0;
        Digit = 0;

        //
        //  So long as we have a valid character, populate thischar.  Do this exactly
        //  twice, so we have one complete byte.
        //

        while (Digit < 2 && StrIndex < String->LengthInChars) {

            SourceChar = String->StartOfString[StrIndex];

            if ((SourceChar >= '0' && SourceChar <= '9') ||
                (SourceChar >= 'a' && SourceChar <= 'f') ||
                (SourceChar >= 'A' && SourceChar <= 'F')) {

                DestChar *= 16;
                if (SourceChar >= '0' && SourceChar <= '9') {
                    DestChar = (UCHAR)(DestChar + SourceChar - '0');
                } else if (SourceChar >= 'a' && SourceChar <= 'f') {
                    DestChar = (UCHAR)(DestChar + SourceChar - 'a' + 10);
                } else if (SourceChar >= 'A' && SourceChar <= 'F') {
                    DestChar = (UCHAR)(DestChar + SourceChar - 'A' + 10);
                }

                StrIndex++;
                Digit++;
            } else {
                break;
            }
        }

        //
        //  If we did actually get two input chars, output the byte and go back for
        //  more.  Otherwise, the input doesn't match the output requirement
        //

        if (Digit == 2) {
            Buffer[Offset] = DestChar;
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 Parse a buffer containing binary data and generate a string that encodes the
 data into hex (implying two chars per byte.)

 @param Buffer Pointer to a buffer to parse.

 @param BufferSize Specifies the length of Buffer, in bytes.

 @param String Pointer to the string to populate.

 @return TRUE to indicate parse success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHexBufferToString(
    __in PUCHAR Buffer,
    __in YORI_ALLOC_SIZE_T BufferSize,
    __in PYORI_STRING String
    )
{
    UCHAR SourceChar;
    UCHAR SourceNibble;
    YORI_ALLOC_SIZE_T Offset;
    YORI_ALLOC_SIZE_T StrIndex;

    //
    //  Currently this routine assumes the caller allocated a large enough
    //  buffer.  The buffer should hold two chars per byte plus a NULL.
    //

    if (String->LengthAllocated <= BufferSize * sizeof(TCHAR)) {
        return FALSE;
    }

    //
    //  Loop through the buffer
    //

    StrIndex = 0;
    for (Offset = 0; Offset < BufferSize; Offset++) {

        SourceChar = Buffer[Offset];
        SourceNibble = (UCHAR)(SourceChar >> 4);
        if (SourceNibble >= 10) {
            String->StartOfString[StrIndex] = (UCHAR)('a' + SourceNibble - 10);
        } else {
            String->StartOfString[StrIndex] = (UCHAR)('0' + SourceNibble);
        }

        StrIndex++;
        SourceNibble = (UCHAR)(SourceChar & 0xF);
        if (SourceNibble >= 10) {
            String->StartOfString[StrIndex] = (UCHAR)('a' + SourceNibble - 10);
        } else {
            String->StartOfString[StrIndex] = (UCHAR)('0' + SourceNibble);
        }

        StrIndex++;
    }

    String->StartOfString[StrIndex] = '\0';
    String->LengthInChars = StrIndex;
    return TRUE;
}

// vim:sw=4:ts=4:et:
