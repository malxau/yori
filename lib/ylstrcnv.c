/**
 * @file lib/ylstrcnv.c
 *
 * Yori string conversion routines
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
 An ordered array of suffixes, where each suffix represents sizes 1024 times
 larger than the previous one.
 */
CONST TCHAR YoriLibSizeSuffixes[] = {'b', 'k', 'm', 'g', 't', '?'};

/**
 Parse a string specifying a file size and return a 64 bit unsigned integer
 from the result.  This function can parse prefixed hex or decimal inputs, and
 understands file size qualifiers (k, m, g et al.)

 @param String Pointer to the string to parse.

 @return The parsed, 64 bit unsigned integer value from the string.
 */
LARGE_INTEGER
YoriLibStringToFileSize(
    __in PCYORI_STRING String
    )
{
    DWORD SuffixLevel = 0;
    LARGE_INTEGER FileSize;
    DWORD i;
    YORI_ALLOC_SIZE_T CharsConsumed;
    YORI_MAX_SIGNED_T llTemp;

    llTemp = 0;
    if (!YoriLibStringToNumber(String, TRUE, &llTemp, &CharsConsumed)) {
        YoriLibLiAssignUnsigned(&FileSize, llTemp);
        return FileSize;
    }
    YoriLibLiAssignUnsigned(&FileSize, llTemp);

    if (CharsConsumed < String->LengthInChars) {
        TCHAR SuffixChar = String->StartOfString[CharsConsumed];

        for (i = 0; i < sizeof(YoriLibSizeSuffixes)/sizeof(YoriLibSizeSuffixes[0]); i++) {
            if (SuffixChar == YoriLibSizeSuffixes[i] || SuffixChar == (YoriLibSizeSuffixes[i] + 'A' - 'a')) {
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
    DWORD SuffixLevel = 0;
    LARGE_INTEGER Size;
    LARGE_INTEGER OldSize;

    Size.HighPart = FileSize->HighPart;
    Size.LowPart = FileSize->LowPart;
    OldSize.HighPart = Size.HighPart;
    OldSize.LowPart = Size.LowPart;

    if (String->LengthAllocated < sizeof("12.3k")) {
        return FALSE;
    }


    while (Size.HighPart != 0 || Size.LowPart > 9999) {
        SuffixLevel++;
        OldSize.HighPart = Size.HighPart;
        OldSize.LowPart = Size.LowPart;

        //
        //  Conceptually we want to divide by 1024.  We do this
        //  in two 32-bit shifts combining back the high bits
        //  so we don't need full compiler support.
        //

        Size.LowPart = (Size.LowPart >> 10) + ((Size.HighPart & 0x3ff) << 22);
        Size.HighPart = (Size.HighPart >> 10) & 0x003fffff;
    }

    if (SuffixLevel >= sizeof(YoriLibSizeSuffixes)/sizeof(TCHAR)) {
        SuffixLevel = sizeof(YoriLibSizeSuffixes)/sizeof(TCHAR) - 1;
    }

    if (Size.LowPart < 100 && SuffixLevel > 0) {
        OldSize.LowPart = (OldSize.LowPart % 1024) * 10 / 1024;
        String->LengthInChars = YoriLibSPrintfS(String->StartOfString,
                                                String->LengthAllocated,
                                                _T("%2i.%1i%c"),
                                                (int)Size.LowPart,
                                                (int)OldSize.LowPart,
                                                YoriLibSizeSuffixes[SuffixLevel]);
    } else {
        String->LengthInChars = YoriLibSPrintfS(String->StartOfString,
                                                String->LengthAllocated,
                                                _T("%4i%c"),
                                                (int)Size.LowPart,
                                                YoriLibSizeSuffixes[SuffixLevel]);
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
    __in PCYORI_STRING String,
    __out LPSYSTEMTIME Date,
    __out_opt PYORI_ALLOC_SIZE_T CharsConsumed
    )
{
    YORI_STRING Substring;
    YORI_ALLOC_SIZE_T CurrentCharsConsumed;
    YORI_ALLOC_SIZE_T TotalCharsConsumed;
    YORI_MAX_SIGNED_T llTemp;

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

    TotalCharsConsumed = TotalCharsConsumed + CurrentCharsConsumed;

    if (CurrentCharsConsumed < Substring.LengthInChars &&
        (Substring.StartOfString[CurrentCharsConsumed] == '/' || Substring.StartOfString[CurrentCharsConsumed] == '-')) {
        Substring.LengthInChars -= CurrentCharsConsumed + 1;
        Substring.StartOfString += CurrentCharsConsumed + 1;

        if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CurrentCharsConsumed)) {
            return FALSE;
        }

        Date->wMonth = (WORD)llTemp;
        TotalCharsConsumed = TotalCharsConsumed + CurrentCharsConsumed + 1;

        if (CurrentCharsConsumed < Substring.LengthInChars &&
            (Substring.StartOfString[CurrentCharsConsumed] == '/' || Substring.StartOfString[CurrentCharsConsumed] == '-')) {
            Substring.LengthInChars -= CurrentCharsConsumed + 1;
            Substring.StartOfString += CurrentCharsConsumed + 1;

            if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CurrentCharsConsumed)) {
                return FALSE;
            }

            Date->wDay = (WORD)llTemp;
            TotalCharsConsumed = TotalCharsConsumed + CurrentCharsConsumed + 1;
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
    __in PCYORI_STRING String,
    __out LPSYSTEMTIME Date
    )
{
    YORI_STRING Substring;
    YORI_ALLOC_SIZE_T CharsConsumed;
    YORI_MAX_SIGNED_T llTemp;

    YoriLibInitEmptyString(&Substring);
    Substring.StartOfString = String->StartOfString;
    Substring.LengthInChars = String->LengthInChars;

    if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed)) {
        return FALSE;
    }

    Date->wHour = (WORD)llTemp;

    if (CharsConsumed < Substring.LengthInChars && Substring.StartOfString[CharsConsumed] == ':') {
        Substring.LengthInChars = Substring.LengthInChars - (CharsConsumed + 1);
        Substring.StartOfString += CharsConsumed + 1;

        if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed)) {
            return FALSE;
        }

        Date->wMinute = (WORD)llTemp;

        if (CharsConsumed < Substring.LengthInChars && Substring.StartOfString[CharsConsumed] == ':') {
            Substring.LengthInChars = Substring.LengthInChars - (CharsConsumed + 1);
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
    __in PCYORI_STRING String,
    __out LPSYSTEMTIME Date
    )
{
    YORI_STRING Substring;
    YORI_ALLOC_SIZE_T CharsConsumed;

    if (!YoriLibStringToDate(String, Date, &CharsConsumed)) {
        return FALSE;
    }

    Substring.StartOfString = String->StartOfString;
    Substring.LengthInChars = String->LengthInChars;

    if (CharsConsumed < Substring.LengthInChars && Substring.StartOfString[CharsConsumed] == ':') {
        Substring.StartOfString += CharsConsumed + 1;
        Substring.LengthInChars = Substring.LengthInChars - (CharsConsumed + 1);

        if (!YoriLibStringToTime(&Substring, Date)) {
            return FALSE;
        }
    }

    return TRUE;
}

// vim:sw=4:ts=4:et:
