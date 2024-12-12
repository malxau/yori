/**
 * @file lib/ylstrnum.c
 *
 * Yori string to number routines
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
    YORI_ALLOC_SIZE_T Index;
    TCHAR Char;

    for (Index = 0; Index < String->LengthInChars; Index++) {
        Char = String->StartOfString[Index];
        if (Char < '0' || Char > '9') {
            break;
        }
        Ret = Ret * 10;
        Ret += Char - '0';
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
YoriLibStringToNumberBase(
    __in PCYORI_STRING String,
    __in WORD Base,
    __in BOOL IgnoreSeperators,
    __out PYORI_MAX_SIGNED_T Number,
    __out PYORI_ALLOC_SIZE_T CharsConsumed
    )
{
    YORI_MAX_SIGNED_T SignedResult;
    YORI_MAX_UNSIGNED_T Result;
    YORI_ALLOC_SIZE_T Index;
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
        SignedResult = (YORI_MAX_SIGNED_T)0 - (YORI_MAX_SIGNED_T)Result;
    } else {
        SignedResult = (YORI_MAX_SIGNED_T)Result;
    }

    *CharsConsumed = Index;
    *Number = SignedResult;
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
    __in PCYORI_STRING String,
    __in BOOL IgnoreSeperators,
    __out PYORI_MAX_SIGNED_T Number,
    __out PYORI_ALLOC_SIZE_T CharsConsumed
    )
{
    YORI_MAX_UNSIGNED_T Result;
    YORI_MAX_SIGNED_T SignedResult;
    YORI_ALLOC_SIZE_T Index;
    YORI_ALLOC_SIZE_T Base = 10;
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
        SignedResult = (YORI_MAX_SIGNED_T)0 - (YORI_MAX_SIGNED_T)Result;
    } else {
        SignedResult = (YORI_MAX_SIGNED_T)Result;
    }

    *CharsConsumed = Index;
    *Number = SignedResult;
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
    __in YORI_MAX_SIGNED_T Number,
    __in WORD Base,
    __in WORD DigitsPerGroup,
    __in TCHAR GroupSeperator
    )
{
    YORI_ALLOC_SIZE_T Index;
    YORI_MAX_UNSIGNED_T Num;
    YORI_MAX_UNSIGNED_T IndexValue;
    YORI_ALLOC_SIZE_T Digits;
    YORI_ALLOC_SIZE_T DigitIndex;

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
    while (IndexValue > (WORD)(Base - 1)) {
        IndexValue = IndexValue / Base;
        Digits++;
    }

    if (DigitsPerGroup != 0) {
        Digits = Digits + ((Digits - 1) / DigitsPerGroup);
    }

    Index = Index + Digits;

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

// vim:sw=4:ts=4:et:
