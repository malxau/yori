/**
 * @file lib/string.c
 *
 * Yori string manipulation routines
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
 Initialize a Yori string with no contents.

 @param String Pointer to the string to initialize.
 */
VOID
YoriLibInitEmptyString(
    __out PYORI_STRING String
   )
{
    String->MemoryToFree = NULL;
    String->StartOfString = NULL;
    String->LengthAllocated = 0;
    String->LengthInChars = 0;
}

/**
 Free any memory being used by a Yori string.  This frees the internal
 string buffer, but the structure itself is caller allocated.

 @param String Pointer to the string to free.
 */
VOID
YoriLibFreeStringContents(
    __inout PYORI_STRING String
    )
{
    if (String->MemoryToFree != NULL) {
        YoriLibDereference(String->MemoryToFree);
    }

    YoriLibInitEmptyString(String);
}

/**
 Allocates memory in a Yori string to hold a specified number of characters.
 This routine will not free any previous allocation or copy any previous
 string contents.

 @param String Pointer to the string to allocate.

 @param CharsToAllocate The number of characters to allocate in the string.

 @return TRUE to indicate the allocate was successful, FALSE if it was not.
 */
BOOL
YoriLibAllocateString(
    __out PYORI_STRING String,
    __in DWORD CharsToAllocate
    )
{
    YoriLibInitEmptyString(String);
    String->MemoryToFree = YoriLibReferencedMalloc(CharsToAllocate * sizeof(TCHAR));
    if (String->MemoryToFree == NULL) {
        return FALSE;
    }
    String->LengthAllocated = CharsToAllocate;
    String->StartOfString = String->MemoryToFree;
    return TRUE;
}

/**
 Reallocates memory in a Yori string to hold a specified number of characters,
 preserving any previous contents and deallocating any previous buffer.

 @param String Pointer to the string to reallocate.

 @param CharsToAllocate The number of characters to allocate in the string.

 @return TRUE to indicate the allocate was successful, FALSE if it was not.
 */
BOOL
YoriLibReallocateString(
    __inout PYORI_STRING String,
    __in DWORD CharsToAllocate
    )
{
    LPTSTR NewMemoryToFree;

    if (CharsToAllocate <= String->LengthInChars) {
        return FALSE;
    }

    NewMemoryToFree = YoriLibReferencedMalloc(CharsToAllocate * sizeof(TCHAR));
    if (NewMemoryToFree == NULL) {
        return FALSE;
    }

    if (String->LengthInChars > 0) {
        memcpy(NewMemoryToFree, String->StartOfString, String->LengthInChars * sizeof(TCHAR));
    }

    if (String->MemoryToFree) {
        YoriLibDereference(String->MemoryToFree);
    }

    String->MemoryToFree = NewMemoryToFree;
    String->LengthAllocated = CharsToAllocate;
    String->StartOfString = String->MemoryToFree;
    return TRUE;
}

/**
 Allocate a new buffer to hold a NULL terminated form of the contents of a
 Yori string.  The caller should free this buffer when it is no longer needed
 by calling @ref YoriLibDereference .

 @param String Pointer to the Yori string to convert into a NULL terminated
        string.

 @return Pointer to a NULL terminated string, or NULL on failure.
 */
LPTSTR
YoriLibCStringFromYoriString(
    __in PYORI_STRING String
    )
{
    LPTSTR Return;

    Return = YoriLibReferencedMalloc((String->LengthInChars + 1) * sizeof(TCHAR));
    if (Return == NULL) {
        return NULL;
    }

    memcpy(Return, String->StartOfString, String->LengthInChars * sizeof(TCHAR));
    Return[String->LengthInChars] = '\0';
    return Return;
}

/**
 Create a Yori string that points to a previously existing NULL terminated
 string constant.  The lifetime of the buffer containing the string is
 managed by the caller.

 @param String Pointer to the Yori string to initialize with a preexisting
        NULL terminated string.

 @param Value Pointer to the NULL terminated string to initialize String with.
 */
VOID
YoriLibConstantString(
    __out PYORI_STRING String,
    __in LPCTSTR Value
    )
{
    String->MemoryToFree = NULL;
    String->StartOfString = (LPTSTR)Value;
    String->LengthInChars = _tcslen(Value);
    String->LengthAllocated = String->LengthInChars + 1;
}

/**
 Copy the contents of one Yori string to another by referencing any existing
 allocation.

 @param Dest The Yori string to be populated with new contents.  This string
        will be reinitialized, and this function makes no attempt to free or
        preserve previous contents.

 @param Src The string that contains contents to propagate into the new
        string.
 */
VOID
YoriLibCloneString(
    __out PYORI_STRING Dest,
    __in PYORI_STRING Src
    )
{
    if (Src->MemoryToFree != NULL) {
        YoriLibReference(Src->MemoryToFree);
    }

    memcpy(Dest, Src, sizeof(YORI_STRING));
}

/**
 Return TRUE if the Yori string is NULL terminated.  If it is not NULL
 terminated, return FALSE.

 @param String Pointer to the string to check.

 @return TRUE if the string is NULL terminated, FALSE if it is not.
 */
BOOL
YoriLibIsStringNullTerminated(
    __in PYORI_STRING String
    )
{
    //
    //  Check that the string is of a sane size.  This is really to check
    //  whether the string has been initialized and populated correctly.
    //

    ASSERT(String->LengthAllocated <= 0x1000000);

    if (String->LengthAllocated > String->LengthInChars &&
        String->StartOfString[String->LengthInChars] == '\0') {

        return TRUE;
    }
    return FALSE;
}

/**
 This routine attempts to convert a string to a number using only positive
 decimal integers.

 @param String Pointer to the string to convert into integer form.

 @return The number from the string.  Zero if the string does not contain
         a valid number.
 */
DWORD
YoriLibDecimalStringToInt(
    __in PYORI_STRING String
    )
{
    DWORD Ret = 0;
    DWORD Index;

    for (Index = 0; Index < String->LengthInChars; Index++) {
        if (String->StartOfString[Index] < '0' || String->StartOfString[Index] > '9') {
            break;
        }
        Ret *= 10;
        Ret += String->StartOfString[Index] - '0';
    }
    return Ret;
}

/**
 This routine attempts to convert a string to a number using all available
 parsing.  As of this writing, it understands 0x and 0n prefixes as well
 as negative numbers.

 @param String Pointer to the string to convert into integer form.

 @param IgnoreSeperators If TRUE, continue to generate a number across comma
        delimiters.  If FALSE, terminate on a comma.

 @param Number On successful completion, this is updated to contain the
        resulting number.

 @param CharsConsumed On successful completion, this is updated to indicate
        the number of characters from the string that were used to generate
        the number.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibStringToNumber(
    __in PYORI_STRING String,
    __in BOOL IgnoreSeperators,
    __out PLONGLONG Number,
    __out PDWORD CharsConsumed
    )
{
    LONGLONG Result;
    DWORD Index;
    DWORD Base = 10;
    BOOL Negative = FALSE;

    Result = 0;
    Index = 0;

    while (String->LengthInChars > Index) {
        if (String->StartOfString[Index] == '0' &&
            String->LengthInChars > Index + 1 &&
            String->StartOfString[Index + 1] == 'x') {
            Base = 16;
            Index += 2;
        } else if (String->StartOfString[Index] == '0' &&
                   String->LengthInChars > Index + 1 &&
                   String->StartOfString[Index + 1] == 'n') {
            Base = 10;
            Index += 2;
        } else if (String->StartOfString[Index] == '-') {
            if (Negative) {
                Negative = FALSE;
            } else {
                Negative = TRUE;
            }
            Index++;
        } else {
            break;
        }
    }

    for (; String->LengthInChars > Index; Index++) {
        if (!IgnoreSeperators || String->StartOfString[Index] != ',') {
            if (Base == 10) {
                if (String->StartOfString[Index] < '0' || String->StartOfString[Index] > '9') {
                    break;
                }
                Result *= Base;
                Result += String->StartOfString[Index] - '0';
            } else if (Base == 16) {
                TCHAR Char;
                Char = YoriLibUpcaseChar(String->StartOfString[Index]);
                if (Char >= '0' && Char <= '9') {
                    Result *= Base;
                    Result += Char - '0';
                } else if (Char >= 'A' && Char <= 'F') {
                    Result *= Base;
                    Result += Char - 'A' + 10;
                } else {
                    break;
                }
            }
        }
    }

    if (Negative) {
        Result = 0 - Result;
    }

    *CharsConsumed = Index;
    *Number = Result;
    return TRUE;
}

/**
 Generate a string from a signed 64 bit integer.  If the string is not
 large enough to contain the result, it is reallocated by this routine.

 @param String Pointer to the Yori string to populate with the string
        form of the number

 @param Number The integer to render into a string form.

 @param Base The number base to use.  Supported values are from 2 through
        36, but typically only 10 and 16 are useful.

 @param DigitsPerGroup The number of digits to output between seperators.
        If this value is zero, no seperators are inserted.

 @param GroupSeperator The character to insert between groups.

 @return TRUE for success, and FALSE for failure.  This routine will not
         fail if passed a string with sufficient allocation to contain
         the result.
 */
BOOL
YoriLibNumberToString(
    __inout PYORI_STRING String,
    __in LONGLONG Number,
    __in DWORD Base,
    __in DWORD DigitsPerGroup,
    __in TCHAR GroupSeperator
    )
{
    DWORD Index;
    LONGLONG Num;
    LONGLONG IndexValue;
    DWORD Digits;
    DWORD DigitIndex;

    Index = 0;
    Num = Number;
    if (Num < 0) {
        Index++;
        Num = 0 - Num;
    }

    //
    //  Count the number of Digits we have in the user's
    //  input.  Stop if we hit the format specifier.
    //  Code below will preserve low order values.
    //

    Digits = 1;
    IndexValue = Num;
    while (IndexValue > Base - 1) {
        IndexValue = IndexValue / Base;
        Digits++;
    }

    if (DigitsPerGroup != 0) {
        Digits += (Digits - 1) / DigitsPerGroup;
    }

    Index += Digits;

    if (String->LengthAllocated < Index + 1) {
        YoriLibFreeStringContents(String);
        if (!YoriLibAllocateString(String, Index + 1)) {
            return FALSE;
        }
    }
    String->StartOfString[Index] = '\0';
    String->LengthInChars = Index;
    Index--;
    DigitIndex = 0;

    do {
        if (DigitsPerGroup != 0 && (DigitIndex % (DigitsPerGroup + 1)) == DigitsPerGroup) {
            String->StartOfString[Index] = GroupSeperator;
        } else {
            IndexValue = Num % Base;

            if (IndexValue > 9) {
                String->StartOfString[Index] = (UCHAR)(IndexValue + 'a' - 10);
            } else {
                String->StartOfString[Index] = (UCHAR)(IndexValue + '0');
            }

            Num /= Base;
        }
        DigitIndex++;
        Index--;
    } while(Num > 0);

    if (Number < 0) {
        String->StartOfString[Index] = '-';
    }

    return TRUE;
}

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
YoriLibCompareStringWithLiteralCount(
    __in PYORI_STRING Str1,
    __in LPCTSTR str2,
    __in DWORD count
    )
{
    DWORD Index = 0;

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
YoriLibCompareStringWithLiteral(
    __in PYORI_STRING Str1,
    __in LPCTSTR str2
    )
{
    return YoriLibCompareStringWithLiteralCount(Str1, str2, (DWORD)-1);
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
YoriLibCompareStringWithLiteralInsensitiveCount(
    __in PYORI_STRING Str1,
    __in LPCTSTR str2,
    __in DWORD count
    )
{
    DWORD Index = 0;

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
YoriLibCompareStringWithLiteralInsensitive(
    __in PYORI_STRING Str1,
    __in LPCTSTR str2
    )
{
    return YoriLibCompareStringWithLiteralInsensitiveCount(Str1, str2, (DWORD)-1);
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
YoriLibCompareStringCount(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2,
    __in DWORD count
    )
{
    DWORD Index = 0;

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
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    )
{
    return YoriLibCompareStringCount(Str1, Str2, (DWORD)-1);
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
YoriLibCompareStringInsensitiveCount(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2,
    __in DWORD count
    )
{
    DWORD Index = 0;

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
YoriLibCompareStringInsensitive(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    )
{
    return YoriLibCompareStringInsensitiveCount(Str1, Str2, (DWORD)-1);
}

/**
 Return the count of consecutive characters in String that listed in the
 characters of the NULL terminated chars array.

 @param String The string to check for consecutive characters.

 @param chars A null terminated list of characters to look for.

 @return The number of characters in String that occur in chars.
 */
DWORD
YoriLibCountStringContainingChars(
    __in PYORI_STRING String,
    __in LPCTSTR chars
    )
{
    DWORD len = 0;
    DWORD i;

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
DWORD
YoriLibCountStringNotContainingChars(
    __in PYORI_STRING String,
    __in LPCTSTR match
    )
{
    DWORD len = 0;
    DWORD i;

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
 Search through a string looking to see if any substrings can be located.
 Returns the first match in offet from the beginning of the string order.
 This routine looks for matches case sensitively.

 @param String The string to search through.

 @param NumberMatches The number of substrings to look for.

 @param MatchArray An array of strings corresponding to the matches to
        look for.

 @param StringOffsetOfMatch On successful completion, returns the offset
        within the string of the match.

 @return If a match is found, returns a pointer to the entry in MatchArray
         corresponding to the substring that was matched.  If no match is
         found, returns NULL.
 */
PYORI_STRING
YoriLibFindFirstMatchingSubstring(
    __in PYORI_STRING String,
    __in DWORD NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PDWORD StringOffsetOfMatch
    )
{
    YORI_STRING RemainingString;
    DWORD CheckCount;

    YoriLibInitEmptyString(&RemainingString);
    RemainingString.StartOfString = String->StartOfString;
    RemainingString.LengthInChars = String->LengthInChars;

    while (RemainingString.LengthInChars > 0) {
        for (CheckCount = 0; CheckCount < NumberMatches; CheckCount++) {
            if (YoriLibCompareStringCount(&RemainingString, &MatchArray[CheckCount], MatchArray[CheckCount].LengthInChars) == 0) {
                if (StringOffsetOfMatch != NULL) {
                    *StringOffsetOfMatch = String->LengthInChars - RemainingString.LengthInChars;
                }
                return &MatchArray[CheckCount];
            }
        }

        RemainingString.LengthInChars--;
        RemainingString.StartOfString++;
    }

    if (StringOffsetOfMatch != NULL) {
        *StringOffsetOfMatch = 0;
    }
    return NULL;
}

/**
 Search through a string looking to see if any substrings can be located.
 Returns the first match in offet from the beginning of the string order.
 This routine looks for matches insensitively.

 @param String The string to search through.

 @param NumberMatches The number of substrings to look for.

 @param MatchArray An array of strings corresponding to the matches to
        look for.

 @param StringOffsetOfMatch On successful completion, returns the offset
        within the string of the match.

 @return If a match is found, returns a pointer to the entry in MatchArray
         corresponding to the substring that was matched.  If no match is
         found, returns NULL.
 */
PYORI_STRING
YoriLibFindFirstMatchingSubstringInsensitive(
    __in PYORI_STRING String,
    __in DWORD NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PDWORD StringOffsetOfMatch
    )
{
    YORI_STRING RemainingString;
    DWORD CheckCount;

    YoriLibInitEmptyString(&RemainingString);
    RemainingString.StartOfString = String->StartOfString;
    RemainingString.LengthInChars = String->LengthInChars;

    while (RemainingString.LengthInChars > 0) {
        for (CheckCount = 0; CheckCount < NumberMatches; CheckCount++) {
            if (YoriLibCompareStringInsensitiveCount(&RemainingString, &MatchArray[CheckCount], MatchArray[CheckCount].LengthInChars) == 0) {
                if (StringOffsetOfMatch != NULL) {
                    *StringOffsetOfMatch = String->LengthInChars - RemainingString.LengthInChars;
                }
                return &MatchArray[CheckCount];
            }
        }

        RemainingString.LengthInChars--;
        RemainingString.StartOfString++;
    }

    if (StringOffsetOfMatch != NULL) {
        *StringOffsetOfMatch = 0;
    }
    return NULL;
}

/**
 Search through a string finding the leftmost instance of a character.  If
 no match is found, return NULL.

 @param String The string to search.

 @param CharToFind The character to find.

 @return Pointer to the leftmost matching character, or NULL if no match was
         found.
 */
LPTSTR
YoriLibFindLeftMostCharacter(
    __in PYORI_STRING String,
    __in TCHAR CharToFind
    )
{
    DWORD Index;
    for (Index = 0; Index < String->LengthInChars; Index++) {
        if (String->StartOfString[Index] == CharToFind) {
            return &String->StartOfString[Index];
        }
    }
    return NULL;
}


/**
 Search through a string finding the rightmost instance of a character.  If
 no match is found, return NULL.

 @param String The string to search.

 @param CharToFind The character to find.

 @return Pointer to the rightmost matching character, or NULL if no match was
         found.
 */
LPTSTR
YoriLibFindRightMostCharacter(
    __in PYORI_STRING String,
    __in TCHAR CharToFind
    )
{
    DWORD Index;
    for (Index = String->LengthInChars; Index > 0; Index--) {
        if (String->StartOfString[Index - 1] == CharToFind) {
            return &String->StartOfString[Index - 1];
        }
    }
    return NULL;
}

// vim:sw=4:ts=4:et:
