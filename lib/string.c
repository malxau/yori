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
 Reallocates memory in a Yori string to hold a specified number of characters,
 without preserving contents, and deallocate the previous buffer on success.

 @param String Pointer to the string to reallocate.

 @param CharsToAllocate The number of characters to allocate in the string.

 @return TRUE to indicate the allocate was successful, FALSE if it was not.
 */
BOOL
YoriLibReallocateStringWithoutPreservingContents(
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

    if (String->MemoryToFree) {
        YoriLibDereference(String->MemoryToFree);
    }

    String->MemoryToFree = NewMemoryToFree;
    String->LengthAllocated = CharsToAllocate;
    String->StartOfString = String->MemoryToFree;
    String->LengthInChars = 0;
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
    __out _Post_satisfies_(String->StartOfString != NULL) PYORI_STRING String,
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
 This routine attempts to convert a string to a number using a specified
 number base (ie., decimal or hexadecimal or octal or binary.)

 @param String Pointer to the string to convert into integer form.

 @param Base The number base.  Must be 10 or 16 or 8 or 2.

 @param IgnoreSeperators If TRUE, continue to generate a number across comma
        delimiters.  If FALSE, terminate on a comma.

 @param Number On successful completion, this is updated to contain the
        resulting number.

 @param CharsConsumed On successful completion, this is updated to indicate
        the number of characters from the string that were used to generate
        the number.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibStringToNumberSpecifyBase(
    __in PYORI_STRING String,
    __in DWORD Base,
    __in BOOL IgnoreSeperators,
    __out PLONGLONG Number,
    __out PDWORD CharsConsumed
    )
{
    LONGLONG Result;
    DWORD Index;
    BOOL Negative = FALSE;

    Result = 0;
    Index = 0;

    while (String->LengthInChars > Index) {
        if (String->StartOfString[Index] == '-') {
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
            } else if (Base == 8) {
                if (String->StartOfString[Index] < '0' || String->StartOfString[Index] >= '8') {
                    break;
                }
                Result *= Base;
                Result += String->StartOfString[Index] - '0';
            } else if (Base == 2) {
                if (String->StartOfString[Index] != '0' && String->StartOfString[Index] != '1') {
                    break;
                }
                Result *= Base;
                Result += String->StartOfString[Index] - '0';
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
 This routine attempts to convert a string to a number using all available
 parsing.  As of this writing, it understands 0x and 0n and 0o and 0x prefixes
 as well as negative numbers.

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
__success(return)
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
        } else if (String->StartOfString[Index] == '0' &&
                   String->LengthInChars > Index + 1 &&
                   String->StartOfString[Index + 1] == 'o') {
            Base = 8;
            Index += 2;
        } else if (String->StartOfString[Index] == '0' &&
                   String->LengthInChars > Index + 1 &&
                   String->StartOfString[Index + 1] == 'b') {
            Base = 2;
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
            } else if (Base == 8) {
                if (String->StartOfString[Index] < '0' || String->StartOfString[Index] >= '8') {
                    break;
                }
                Result *= Base;
                Result += String->StartOfString[Index] - '0';
            } else if (Base == 2) {
                if (String->StartOfString[Index] != '0' && String->StartOfString[Index] != '1') {
                    break;
                }
                Result *= Base;
                Result += String->StartOfString[Index] - '0';
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
__success(return)
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
    DWORDLONG Num;
    DWORDLONG IndexValue;
    DWORD Digits;
    DWORD DigitIndex;

    Index = 0;
    if (Number < 0) {
        Index++;
        Num = 0;
        Num = Num - Number;
    } else {
        Num = Number;
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
    __in PCYORI_STRING Str1,
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
    __in PCYORI_STRING Str1,
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
    __in PCYORI_STRING Str1,
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
    __in PCYORI_STRING Str1,
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
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2,
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
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2
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
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2,
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
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2
    )
{
    return YoriLibCompareStringInsensitiveCount(Str1, Str2, (DWORD)-1);
}

/**
 Count the number of identical characters between two strings.

 @param Str1 The first string to compare.

 @param Str2 The second string to compare.

 @return The number of identical characters.
 */
DWORD
YoriLibCountStringMatchingChars(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    )
{
    DWORD Index;

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
DWORD
YoriLibCountStringMatchingCharsInsensitive(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    )
{
    DWORD Index;

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
DWORD
YoriLibCountStringContainingChars(
    __in PCYORI_STRING String,
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
    __in PCYORI_STRING String,
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
 Return the count of consecutive characters at the end of String that are
 listed in the characters of the NULL terminated chars array.

 @param String The string to check for consecutive characters.

 @param chars A null terminated list of characters to look for.

 @return The number of characters in String that occur in chars.
 */
DWORD
YoriLibCountStringTrailingChars(
    __in PCYORI_STRING String,
    __in LPCTSTR chars
    )
{
    DWORD len = 0;
    DWORD i;

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
 Search through a string looking to see if any substrings can be located.
 Returns the last match in offet from the end of the string order.
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
YoriLibFindLastMatchingSubstring(
    __in PYORI_STRING String,
    __in DWORD NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PDWORD StringOffsetOfMatch
    )
{
    YORI_STRING RemainingString;
    DWORD CheckCount;

    YoriLibInitEmptyString(&RemainingString);

    while (TRUE) {

        RemainingString.LengthInChars++;
        if (RemainingString.LengthInChars > String->LengthInChars) {
            break;
        }
        RemainingString.StartOfString = &String->StartOfString[String->LengthInChars - RemainingString.LengthInChars];

        for (CheckCount = 0; CheckCount < NumberMatches; CheckCount++) {
            if (YoriLibCompareStringCount(&RemainingString, &MatchArray[CheckCount], MatchArray[CheckCount].LengthInChars) == 0) {
                if (StringOffsetOfMatch != NULL) {
                    *StringOffsetOfMatch = String->LengthInChars - RemainingString.LengthInChars;
                }
                return &MatchArray[CheckCount];
            }
        }
    }

    if (StringOffsetOfMatch != NULL) {
        *StringOffsetOfMatch = 0;
    }
    return NULL;
}

/**
 Search through a string looking to see if any substrings can be located.
 Returns the last match in offet from the end of the string order.
 This routine looks for matches case insensitively.

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
YoriLibFindLastMatchingSubstringInsensitive(
    __in PYORI_STRING String,
    __in DWORD NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PDWORD StringOffsetOfMatch
    )
{
    YORI_STRING RemainingString;
    DWORD CheckCount;

    YoriLibInitEmptyString(&RemainingString);

    while (TRUE) {

        RemainingString.LengthInChars++;
        if (RemainingString.LengthInChars > String->LengthInChars) {
            break;
        }
        RemainingString.StartOfString = &String->StartOfString[String->LengthInChars - RemainingString.LengthInChars];

        for (CheckCount = 0; CheckCount < NumberMatches; CheckCount++) {
            if (YoriLibCompareStringInsensitiveCount(&RemainingString, &MatchArray[CheckCount], MatchArray[CheckCount].LengthInChars) == 0) {
                if (StringOffsetOfMatch != NULL) {
                    *StringOffsetOfMatch = String->LengthInChars - RemainingString.LengthInChars;
                }
                return &MatchArray[CheckCount];
            }
        }
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
    __in PCYORI_STRING String,
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
    __in PCYORI_STRING String,
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
    __in DWORD BufferSize
    )
{
    UCHAR DestChar;
    TCHAR SourceChar;
    DWORD Offset;
    DWORD Digit;
    DWORD StrIndex;

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
    __in DWORD BufferSize,
    __in PYORI_STRING String
    )
{
    UCHAR SourceChar;
    UCHAR SourceNibble;
    DWORD Offset;
    DWORD StrIndex;

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

/**
 Parse a string specifying a file size and return a 64 bit unsigned integer
 from the result.  This function can parse prefixed hex or decimal inputs, and
 understands file size qualifiers (k, m, g et al.)

 @param String Pointer to the string to parse.

 @return The parsed, 64 bit unsigned integer value from the string.
 */
LARGE_INTEGER
YoriLibStringToFileSize(
    __in PYORI_STRING String
    )
{
    TCHAR Suffixes[] = {'b', 'k', 'm', 'g', 't'};
    DWORD SuffixLevel = 0;
    LARGE_INTEGER FileSize;
    DWORD i;
    DWORD CharsConsumed;

    if (!YoriLibStringToNumber(String, TRUE, &FileSize.QuadPart, &CharsConsumed)) {
        return FileSize;
    }

    if (CharsConsumed < String->LengthInChars) {
        TCHAR SuffixChar = String->StartOfString[CharsConsumed];

        for (i = 0; i < sizeof(Suffixes)/sizeof(Suffixes[0]); i++) {
            if (SuffixChar == Suffixes[i] || SuffixChar == (Suffixes[i] + 'A' - 'a')) {
                SuffixLevel = i;
                break;
            }
        }

        while(SuffixLevel) {

            FileSize.HighPart = (FileSize.HighPart << 10) + (FileSize.LowPart >> 22);
            FileSize.LowPart = (FileSize.LowPart << 10);

            SuffixLevel--;
        }
    }

    return FileSize;
}

/**
 Convert a 64 bit file size into a string.  This function returns 3 or 4
 significant digits followed by a suffix indicating the magnitude, for a
 total of 5 chars.

 @param String On successful completion, updated to contain the string form
        of the file size.  The caller should ensure this is allocated with
        six chars on input.

 @param FileSize The numeric file size.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFileSizeToString(
    __inout PYORI_STRING String,
    __in PLARGE_INTEGER FileSize
    )
{
    TCHAR Suffixes[] = {'b', 'k', 'm', 'g', 't', '?'};
    DWORD SuffixLevel = 0;
    LARGE_INTEGER Size;
    LARGE_INTEGER OldSize;

    Size.QuadPart = FileSize->QuadPart;
    OldSize.QuadPart = Size.QuadPart;

    if (String->LengthAllocated < sizeof("12.3k")) {
        return FALSE;
    }


    while (Size.HighPart != 0 || Size.LowPart > 9999) {
        SuffixLevel++;
        OldSize.QuadPart = Size.QuadPart;

        //
        //  Conceptually we want to divide by 1024.  We do this
        //  in two 32-bit shifts combining back the high bits
        //  so we don't need full compiler support.
        //

        Size.LowPart = (Size.LowPart >> 10) + ((Size.HighPart & 0x3ff) << 22);
        Size.HighPart = (Size.HighPart >> 10) & 0x003fffff;
    }

    if (SuffixLevel >= sizeof(Suffixes)/sizeof(TCHAR)) {
        SuffixLevel = sizeof(Suffixes)/sizeof(TCHAR) - 1;
    }

    if (Size.LowPart < 100 && SuffixLevel > 0) {
        OldSize.LowPart = (OldSize.LowPart % 1024) * 10 / 1024;
        String->LengthInChars = YoriLibSPrintfS(String->StartOfString,
                                                String->LengthAllocated,
                                                _T("%2i.%1i%c"),
                                                (int)Size.LowPart,
                                                (int)OldSize.LowPart,
                                                Suffixes[SuffixLevel]);
    } else {
        String->LengthInChars = YoriLibSPrintfS(String->StartOfString,
                                                String->LengthAllocated,
                                                _T("%4i%c"),
                                                (int)Size.LowPart,
                                                Suffixes[SuffixLevel]);
    }
    return TRUE;
}

/**
 Parse a string specifying a file date and return a timestamp from the result.

 @param String Pointer to the string to parse.

 @param Date On successful completion, updated to point to a timestamp.

 @param CharsConsumed Optionally points to a value to update with the number of
        characters consumed from String to generate the requested output.

 @return TRUE to indicate success, FALSE to indicate parse failure.
 */
__success(return)
BOOL
YoriLibStringToDate(
    __in PYORI_STRING String,
    __out LPSYSTEMTIME Date,
    __out_opt PDWORD CharsConsumed
    )
{
    YORI_STRING Substring;
    DWORD CurrentCharsConsumed;
    DWORD TotalCharsConsumed;
    LONGLONG llTemp;

    YoriLibInitEmptyString(&Substring);
    Substring.StartOfString = String->StartOfString;
    Substring.LengthInChars = String->LengthInChars;
    TotalCharsConsumed = 0;

    if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CurrentCharsConsumed)) {
        return FALSE;
    }

    Date->wYear = (WORD)llTemp;
    if (Date->wYear < 100) {
        Date->wYear += 2000;
    }

    TotalCharsConsumed += CurrentCharsConsumed;

    if (CurrentCharsConsumed < Substring.LengthInChars && Substring.StartOfString[CurrentCharsConsumed] == '/') {
        Substring.LengthInChars -= CurrentCharsConsumed + 1;
        Substring.StartOfString += CurrentCharsConsumed + 1;

        if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CurrentCharsConsumed)) {
            return FALSE;
        }

        Date->wMonth = (WORD)llTemp;
        TotalCharsConsumed += CurrentCharsConsumed + 1;

        if (CurrentCharsConsumed < Substring.LengthInChars && Substring.StartOfString[CurrentCharsConsumed] == '/') {
            Substring.LengthInChars -= CurrentCharsConsumed + 1;
            Substring.StartOfString += CurrentCharsConsumed + 1;

            if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CurrentCharsConsumed)) {
                return FALSE;
            }

            Date->wDay = (WORD)llTemp;
            TotalCharsConsumed += CurrentCharsConsumed + 1;
        }
    }

    if (CharsConsumed != NULL) {
        *CharsConsumed = TotalCharsConsumed;
    }

    return TRUE;
}

/**
 Parse a string specifying a file time and return a timestamp from the result.

 @param String Pointer to the string to parse.

 @param Date On successful completion, updated to point to a timestamp.

 @return TRUE to indicate success, FALSE to indicate parse failure.
 */
__success(return)
BOOL
YoriLibStringToTime(
    __in PYORI_STRING String,
    __out LPSYSTEMTIME Date
    )
{
    YORI_STRING Substring;
    DWORD CharsConsumed;
    LONGLONG llTemp;

    YoriLibInitEmptyString(&Substring);
    Substring.StartOfString = String->StartOfString;
    Substring.LengthInChars = String->LengthInChars;

    if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed)) {
        return FALSE;
    }

    Date->wHour = (WORD)llTemp;

    if (CharsConsumed < Substring.LengthInChars && Substring.StartOfString[CharsConsumed] == ':') {
        Substring.LengthInChars -= CharsConsumed + 1;
        Substring.StartOfString += CharsConsumed + 1;

        if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed)) {
            return FALSE;
        }

        Date->wMinute = (WORD)llTemp;

        if (CharsConsumed < Substring.LengthInChars && Substring.StartOfString[CharsConsumed] == ':') {
            Substring.LengthInChars -= CharsConsumed + 1;
            Substring.StartOfString += CharsConsumed + 1;

            if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed)) {
                return FALSE;
            }

            Date->wSecond = (WORD)llTemp;
        }
    }

    return TRUE;
}

/**
 Parse a string specifying a file date and time and return a timestamp from
 the result.

 @param String Pointer to the string to parse.

 @param Date On successful completion, updated to point to a timestamp.

 @return TRUE to indicate success, FALSE to indicate parse failure.
 */
__success(return)
BOOL
YoriLibStringToDateTime(
    __in PYORI_STRING String,
    __out LPSYSTEMTIME Date
    )
{
    YORI_STRING Substring;
    DWORD CharsConsumed;

    if (!YoriLibStringToDate(String, Date, &CharsConsumed)) {
        return FALSE;
    }

    Substring.StartOfString = String->StartOfString;
    Substring.LengthInChars = String->LengthInChars;

    if (CharsConsumed < Substring.LengthInChars && Substring.StartOfString[CharsConsumed] == ':') {
        Substring.StartOfString += CharsConsumed + 1;
        Substring.LengthInChars -= CharsConsumed + 1;

        if (!YoriLibStringToTime(&Substring, Date)) {
            return FALSE;
        }
    }

    return TRUE;
}


// vim:sw=4:ts=4:et:
