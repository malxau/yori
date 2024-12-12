/**
 * @file lib/ylstrtrm.c
 *
 * Yori string trim routines
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
 Remove spaces from the beginning and end of a Yori string.
 Note this implies advancing the StartOfString pointer, so a caller
 cannot assume this pointer is unchanged across the call.

 @param String Pointer to the Yori string to remove spaces from.
 */
VOID
YoriLibTrimSpaces(
    __in PYORI_STRING String
    )
{
    while (String->LengthInChars > 0) {
        if (String->StartOfString[0] == ' ') {
            String->StartOfString++;
            String->LengthInChars--;
        } else {
            break;
        }
    }

    while (String->LengthInChars > 0) {
        if (String->StartOfString[String->LengthInChars - 1] == ' ') {
            String->LengthInChars--;
        } else {
            break;
        }
    }
}

/**
 Remove newlines from the end of a Yori string.

 @param String Pointer to the Yori string to remove newlines from.
 */
VOID
YoriLibTrimTrailingNewlines(
    __in PYORI_STRING String
    )
{
    while (String->LengthInChars > 0 &&
           (String->StartOfString[String->LengthInChars - 1] == '\n' ||
            String->StartOfString[String->LengthInChars - 1] == '\r')) {

        String->LengthInChars--;
    }
}

/**
 Remove NULL terminated from the end of a Yori string.

 @param String Pointer to the Yori string to remove NULLs from.
 */
VOID
YoriLibTrimNullTerminators(
    __in PYORI_STRING String
    )
{
    while (String->LengthInChars > 0) {
        if (String->StartOfString[String->LengthInChars - 1] == '\0') {
            String->LengthInChars--;
        } else {
            break;
        }
    }
}

/**
 This function right aligns a Yori string by moving characters in place
 to ensure the total length of the string equals the specified alignment.

 @param String Pointer to the string to align.

 @param Align The number of characters that the string should contain.  If
        it currently has less than this number, spaces are inserted at the
        beginning of the string.
 */
VOID
YoriLibRightAlignString(
    __in PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T Align
    )
{
    YORI_ALLOC_SIZE_T Index;
    YORI_ALLOC_SIZE_T Delta;
    if (String->LengthInChars >= Align) {
        return;
    }
    ASSERT(String->LengthAllocated >= Align);
    if (String->LengthAllocated < Align) {
        return;
    }
    Delta = Align - String->LengthInChars;
    for (Index = Align - 1; Index >= Delta; Index--) {
        String->StartOfString[Index] = String->StartOfString[Index - Delta];
    }
    for (Index = 0; Index < Delta; Index++) {
        String->StartOfString[Index] = ' ';
    }
    String->LengthInChars = Align;
}

// vim:sw=4:ts=4:et:
