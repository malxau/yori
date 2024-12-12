/**
 * @file lib/ylstrcmp.c
 *
 * Yori string comparison routines
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
 Compare a Yori string against a NULL terminated string up to a specified
 maximum number of characters.  If the strings are equal, return zero; if
 the first string is lexicographically earlier than the second, return
 negative; if the second string is lexicographically earlier than the first,
 return positive.

 @param Str1 The first (Yori) string to compare.

 @param str2 The second (NULL terminated) string to compare.

 @param count The number of characters to compare.

 @return Zero for equality; -1 if the first is less than the second; 1 if
         the first is greater than the second.
 */
int
YoriLibCompareStringLitCnt(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2,
    __in YORI_ALLOC_SIZE_T count
    )
{
    YORI_ALLOC_SIZE_T Index = 0;

    if (count == 0) {
        return 0;
    }

    while(TRUE) {

        if (Index == Str1->LengthInChars) {
            if (str2[Index] == '\0') {
                return 0;
            } else {
                return -1;
            }
        } else if (str2[Index] == '\0') {
            return 1;
        }

        if (Str1->StartOfString[Index] < str2[Index]) {
            return -1;
        } else if (Str1->StartOfString[Index] > str2[Index]) {
            return 1;
        }

        Index++;

        if (Index == count) {
            return 0;
        }
    }
    return 0;
}

/**
 Compare a Yori string against a NULL terminated string.  If the strings are
 equal, return zero; if the first string is lexicographically earlier than
 the second, return negative; if the second string is lexicographically
 earlier than the first, return positive.

 @param Str1 The first (Yori) string to compare.

 @param str2 The second (NULL terminated) string to compare.

 @return Zero for equality; -1 if the first is less than the second; 1 if
         the first is greater than the second.
 */
int
YoriLibCompareStringLit(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2
    )
{
    return YoriLibCompareStringLitCnt(Str1, str2, (YORI_ALLOC_SIZE_T)-1);
}

/**
 Convert a single english character to its uppercase form.

 @param c The character to convert.

 @return The uppercase form of the character.
 */
TCHAR
YoriLibUpcaseChar(
    __in TCHAR c
    )
{
    if (c >= 'a' && c <= 'z') {
        return (TCHAR)(c - 'a' + 'A');
    }
    return c;
}

/**
 Compare a Yori string against a NULL terminated string up to a specified
 maximum number of characters without regard to case.  If the strings are
 equal, return zero; if the first string is lexicographically earlier than
 the second, return negative; if the second string is lexicographically
 earlier than the first, return positive.

 @param Str1 The first (Yori) string to compare.

 @param str2 The second (NULL terminated) string to compare.

 @param count The number of characters to compare.

 @return Zero for equality; -1 if the first is less than the second; 1 if
         the first is greater than the second.
 */
int
YoriLibCompareStringLitInsCnt(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2,
    __in YORI_ALLOC_SIZE_T count
    )
{
    YORI_ALLOC_SIZE_T Index = 0;

    if (count == 0) {
        return 0;
    }

    while(TRUE) {

        if (Index == Str1->LengthInChars) {
            if (str2[Index] == '\0') {
                return 0;
            } else {
                return -1;
            }
        } else if (str2[Index] == '\0') {
            return 1;
        }

        if (YoriLibUpcaseChar(Str1->StartOfString[Index]) < YoriLibUpcaseChar(str2[Index])) {
            return -1;
        } else if (YoriLibUpcaseChar(Str1->StartOfString[Index]) > YoriLibUpcaseChar(str2[Index])) {
            return 1;
        }

        Index++;

        if (Index == count) {
            return 0;
        }
    }
    return 0;
}

/**
 Compare a Yori string against a NULL terminated string without regard to
 case.  If the strings are equal, return zero; if the first string is
 lexicographically earlier than the second, return negative; if the second
 string is lexicographically earlier than the first, return positive.

 @param Str1 The first (Yori) string to compare.

 @param str2 The second (NULL terminated) string to compare.

 @return Zero for equality; -1 if the first is less than the second; 1 if
         the first is greater than the second.
 */
int
YoriLibCompareStringLitIns(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2
    )
{
    return YoriLibCompareStringLitInsCnt(Str1, str2, (YORI_ALLOC_SIZE_T)-1);
}

/**
 Compare two Yori strings up to a specified maximum number of characters.
 If the strings are equal, return zero; if the first string is
 lexicographically earlier than the second, return negative; if the second
 string is lexicographically earlier than the first, return positive.

 @param Str1 The first (Yori) string to compare.

 @param Str2 The second (NULL terminated) string to compare.

 @param count The number of characters to compare.

 @return Zero for equality; -1 if the first is less than the second; 1 if
         the first is greater than the second.
 */
int
YoriLibCompareStringCnt(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2,
    __in YORI_ALLOC_SIZE_T count
    )
{
    YORI_ALLOC_SIZE_T Index = 0;

    if (count == 0) {
        return 0;
    }

    while(TRUE) {

        if (Index == Str1->LengthInChars) {
            if (Index == Str2->LengthInChars) {
                return 0;
            } else {
                return -1;
            }
        } else if (Index == Str2->LengthInChars) {
            return 1;
        }

        if (Str1->StartOfString[Index] < Str2->StartOfString[Index]) {
            return -1;
        } else if (Str1->StartOfString[Index] > Str2->StartOfString[Index]) {
            return 1;
        }

        Index++;

        if (Index == count) {
            return 0;
        }
    }
    return 0;
}

/**
 Compare two Yori strings.  If the strings are equal, return zero; if the
 first string is lexicographically earlier than the second, return negative;
 if the second string is lexicographically earlier than the first, return
 positive.

 @param Str1 The first string to compare.

 @param Str2 The second string to compare.

 @return Zero for equality; -1 if the first is less than the second; 1 if
         the first is greater than the second.
 */
int
YoriLibCompareString(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2
    )
{
    return YoriLibCompareStringCnt(Str1, Str2, (YORI_ALLOC_SIZE_T)-1);
}

/**
 Compare two Yori strings up to a specified maximum number of characters
 without regard to case.  If the strings are equal, return zero; if the
 first string is lexicographically earlier than the second, return negative;
 if the second string is lexicographically earlier than the first, return
 positive.

 @param Str1 The first (Yori) string to compare.

 @param Str2 The second (NULL terminated) string to compare.

 @param count The number of characters to compare.

 @return Zero for equality; -1 if the first is less than the second; 1 if
         the first is greater than the second.
 */
int
YoriLibCompareStringInsCnt(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2,
    __in YORI_ALLOC_SIZE_T count
    )
{
    YORI_ALLOC_SIZE_T Index = 0;

    if (count == 0) {
        return 0;
    }

    while(TRUE) {

        if (Index == Str1->LengthInChars) {
            if (Index == Str2->LengthInChars) {
                return 0;
            } else {
                return -1;
            }
        } else if (Index == Str2->LengthInChars) {
            return 1;
        }

        if (YoriLibUpcaseChar(Str1->StartOfString[Index]) < YoriLibUpcaseChar(Str2->StartOfString[Index])) {
            return -1;
        } else if (YoriLibUpcaseChar(Str1->StartOfString[Index]) > YoriLibUpcaseChar(Str2->StartOfString[Index])) {
            return 1;
        }

        Index++;

        if (Index == count) {
            return 0;
        }
    }
    return 0;
}

/**
 Compare two Yori strings without regard to case.  If the strings are equal,
 return zero; if the first string is lexicographically earlier than the
 second, return negative; if the second string is lexicographically earlier
 than the first, return positive.

 @param Str1 The first string to compare.

 @param Str2 The second string to compare.

 @return Zero for equality; -1 if the first is less than the second; 1 if
         the first is greater than the second.
 */
int
YoriLibCompareStringIns(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2
    )
{
    return YoriLibCompareStringInsCnt(Str1, Str2, (YORI_ALLOC_SIZE_T)-1);
}

// vim:sw=4:ts=4:et:
