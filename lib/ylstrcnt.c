/**
 * @file lib/ylstrcnt.c
 *
 * Yori string couting routines
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
 Count the number of identical characters between two strings.

 @param Str1 The first string to compare.

 @param Str2 The second string to compare.

 @return The number of identical characters.
 */
YORI_ALLOC_SIZE_T
YoriLibCntStringMatchChars(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    )
{
    YORI_ALLOC_SIZE_T Index;

    Index = 0;

    while (Index < Str1->LengthInChars &&
           Index < Str2->LengthInChars &&
           Str1->StartOfString[Index] == Str2->StartOfString[Index]) {

        Index++;
    }

    return Index;
}

/**
 Count the number of identical characters between two strings without regard
 to case.

 @param Str1 The first string to compare.

 @param Str2 The second string to compare.

 @return The number of identical characters.
 */
YORI_ALLOC_SIZE_T
YoriLibCntStringMatchCharsIns(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    )
{
    YORI_ALLOC_SIZE_T Index;

    Index = 0;

    while (Index < Str1->LengthInChars &&
           Index < Str2->LengthInChars &&
           YoriLibUpcaseChar(Str1->StartOfString[Index]) == YoriLibUpcaseChar(Str2->StartOfString[Index])) {

        Index++;
    }

    return Index;
}

/**
 Return the count of consecutive characters in String that are listed in the
 characters of the NULL terminated chars array.

 @param String The string to check for consecutive characters.

 @param chars A null terminated list of characters to look for.

 @return The number of characters in String that occur in chars.
 */
YORI_ALLOC_SIZE_T
YoriLibCntStringWithChars(
    __in PCYORI_STRING String,
    __in LPCTSTR chars
    )
{
    YORI_ALLOC_SIZE_T len = 0;
    YORI_ALLOC_SIZE_T i;

    for (len = 0; len < String->LengthInChars; len++) {
        for (i = 0; chars[i] != '\0'; i++) {
            if (String->StartOfString[len] == chars[i]) {
                break;
            }
        }
        if (chars[i] == '\0') {
            return len;
        }
    }

    return len;
}

/**
 Return the count of characters in String that are none of the characters
 in the NULL terminated match array.

 @param String The string to check for consecutive characters.

 @param match A null terminated list of characters to look for.

 @return The number of characters in String that do not occur in match.
 */
YORI_ALLOC_SIZE_T
YoriLibCntStringNotWithChars(
    __in PCYORI_STRING String,
    __in LPCTSTR match
    )
{
    YORI_ALLOC_SIZE_T len = 0;
    YORI_ALLOC_SIZE_T i;

    for (len = 0; len < String->LengthInChars; len++) {
        for (i = 0; match[i] != '\0'; i++) {
            if (String->StartOfString[len] == match[i]) {
                return len;
            }
        }
    }

    return len;
}

/**
 Return the count of consecutive characters at the end of String that are
 listed in the characters of the NULL terminated chars array.

 @param String The string to check for consecutive characters.

 @param chars A null terminated list of characters to look for.

 @return The number of characters in String that occur in chars.
 */
YORI_ALLOC_SIZE_T
YoriLibCntStringTrailingChars(
    __in PCYORI_STRING String,
    __in LPCTSTR chars
    )
{
    YORI_ALLOC_SIZE_T len = 0;
    YORI_ALLOC_SIZE_T i;

    for (len = String->LengthInChars; len > 0; len--) {
        for (i = 0; chars[i] != '\0'; i++) {
            if (String->StartOfString[len - 1] == chars[i]) {
                break;
            }
        }
        if (chars[i] == '\0') {
            return String->LengthInChars - len;
        }
    }

    return String->LengthInChars - len;
}

// vim:sw=4:ts=4:et:
