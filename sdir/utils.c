/**
 * @file sdir/utils.c
 *
 * Colorful, sorted and optionally rich directory enumeration
 * for Windows.
 *
 * This module contains helper functions that can be used in a variety of
 * situations.
 *
 * Copyright (c) 2014-2018 Malcolm J. Smith
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

#include "sdir.h"

//
//  These are general helper routines to assist in converting strings into
//  something we can use directly.
//

/**
 Parse a NULL terminated string and return a 32 bit unsigned integer from
 the result.  This function can parse prefixed hex or decimal inputs, and
 indicates the end of the string that it processed (meaning, the first non
 numeric character in the string.)

 @param str Pointer to the string to parse.

 @param endptr Optionally points to a value to receive a pointer to the first
        non-numeric character in the string.

 @return The parsed, 32 bit unsigned integer value from the string.
 */
DWORD
SdirStringToNum32(
    __in LPCTSTR str,
    __out_opt LPCTSTR* endptr
    )
{
    DWORD ret = 0;
    if (str[0] == '0' && str[1] == 'x') {
        str += 2;
        while ((*str >= '0' && *str <= '9') || (*str >= 'a' && *str <= 'f') || (*str >= 'A' && *str <= 'F')) {
            ret *= 16;
            if (*str >= '0' && *str <= '9') {
                ret += *str - '0';
            } else if (*str >= 'a' && *str <= 'f') {
                ret += *str - 'a' + 10;
            } else if (*str >= 'A' && *str <= 'F') {
                ret += *str - 'A' + 10;
            }
            str++;
        }
    } else {
        while (*str >= '0' && *str <= '9') {
            ret *= 10;
            ret += *str - '0';
            str++;
        }
    }
    if (endptr) {
        *endptr = str;
    }
    return ret;
}

//
//  Formatting support.  Generic routines used across different types of data.
//

/**
 Populate a file size value into a string of formatted characters.  This
 function will apply any file size suffixes to the string as needed.  It
 will only write 6 chars to Buffer (space, 4 chars of number possibly
 including a decimal point, and suffix.)

 @param Buffer The buffer to populate with the file size.

 @param Attributes The color attributes to apply to the resulting file size.

 @param GenericSize Pointer to the file size as a number.

 @return The number of characters written to the buffer (6.)
 */
ULONG
SdirDisplayGenericSize(
    __out_ecount(6) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PLARGE_INTEGER GenericSize
    )
{
    TCHAR StrSize[10];
    YORI_STRING YsSize;

    if (Buffer) {
        YoriLibInitEmptyString(&YsSize);
        YsSize.StartOfString = &StrSize[1];
        YsSize.LengthAllocated = sizeof(StrSize)/sizeof(StrSize[0]) - 1;

        YoriLibFileSizeToString(&YsSize, GenericSize);
        ASSERT(YsSize.LengthInChars == 5);

        StrSize[0] = ' ';
        SdirPasteStr(Buffer, StrSize, Attributes, 6);
    }
    return 6;
}

/**
 Populate a generic 64 bit number into a string using formatted hex
 characters.  It will only write 18 chars to Buffer (space, 8 chars of hex,
 seperator, 8 chars of hex.)

 @param Buffer The buffer to populate with the hex string.

 @param Attributes The color attributes to apply to the resulting string.

 @param Hex Pointer to the input value as a number.

 @return The number of characters written to the buffer (18.)
 */
ULONG
SdirDisplayHex64 (
    __out_ecount(18) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PLARGE_INTEGER Hex
    )
{
    TCHAR Str[19];

    if (Buffer) {
        YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" %08x`%08x"), (int)Hex->HighPart, (int)Hex->LowPart);
        SdirPasteStr(Buffer, Str, Attributes, 18);
    }

    return 18;
}

/**
 Populate a generic 32 bit number into a string using formatted hex
 characters.  It will only write 9 chars to Buffer (space, 8 chars of hex.)

 @param Buffer The buffer to populate with the hex string.

 @param Attributes The color attributes to apply to the resulting string.

 @param Hex Pointer to the input value as a number.

 @return The number of characters written to the buffer (9.)
 */
ULONG
SdirDisplayHex32 (
    __out_ecount(9) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in DWORD Hex
    )
{
    TCHAR Str[10];

    if (Buffer) {
        YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" %08x"), (int)Hex);
        SdirPasteStr(Buffer, Str, Attributes, 9);
    }
    return 9;
}

/**
 Populate a generic buffer into a string using formatted hex characters.
 It will write two formatted chars per char in the input buffer, plus a
 space.

 @param Buffer The buffer to populate with the hex string.

 @param Attributes The color attributes to apply to the resulting string.

 @param InputBuffer Pointer to the buffer containing chars to encode into
        hex.  Note this is always an 8 bit string.

 @param Size The number of bytes in InputBuffer to encode into Buffer.

 @return The number of characters written to the buffer.
 */
ULONG
SdirDisplayGenericHexBuffer (
    __out_ecount(Size * 2 + 1) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PUCHAR InputBuffer,
    __in DWORD Size
    )
{
    TCHAR Str[3];
    DWORD Offset;

    if (Buffer) {
        SdirPasteStr(Buffer, _T(" "), Attributes, 1);
        for (Offset = 0; Offset < Size; Offset++) {
            YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T("%02x"), InputBuffer[Offset]);
            SdirPasteStr(Buffer + 1 + 2 * Offset, Str, Attributes, 2);
        }
    }

    return Size * 2 + 1;
}

/**
 Populate a date into a formatted string.  It will write 11 formatted chars
 into the buffer (space, 4 chars for year, seperator, 2 chars for month,
 seperator, 2 chars for day.)

 @param Buffer The buffer to populate with the date string.

 @param Attributes The color attributes to apply to the resulting string.

 @param Time Pointer to the timestamp specifying the date.  Note only the
        date components are consumed, time values are ignored.

 @return The number of characters written to the buffer (11.)
 */
ULONG
SdirDisplayFileDate (
    __out_ecount(11) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in SYSTEMTIME * Time
    )
{
    TCHAR StrDate[16];

    if (Buffer) {
        YoriLibSPrintfS(StrDate,
                    sizeof(StrDate)/sizeof(StrDate[0]),
                    _T(" %04i/%02i/%02i"),
                    Time->wYear,
                    Time->wMonth,
                    Time->wDay);

        SdirPasteStr(Buffer, StrDate, Attributes, 11);
    }
    return 11;
}

/**
 Populate a time into a formatted string.  It will write 9 formatted chars
 into the buffer (space, 2 chars for hour, seperator, 2 chars for min,
 seperator, 2 chars for second.)

 @param Buffer The buffer to populate with the time string.

 @param Attributes The color attributes to apply to the resulting string.

 @param Time Pointer to the timestamp specifying the time.  Note only the
        time components are consumed, date values are ignored.

 @return The number of characters written to the buffer (9.)
 */
ULONG
SdirDisplayFileTime (
    __out_ecount(9) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in SYSTEMTIME * Time
    )
{
    TCHAR StrTime[10];

    if (Buffer) {
        YoriLibSPrintfS(StrTime,
                    sizeof(StrTime)/sizeof(StrTime[0]),
                    _T(" %02i:%02i:%02i"),
                    Time->wHour,
                    Time->wMinute,
                    Time->wSecond);

        SdirPasteStr(Buffer, StrTime, Attributes, 9);
    }
    return 9;
}

/**
 Multiply a 64 bit value by a 32 bit value which consists of only a single
 set bit (ie., power of two.)  This is done to allow limited 64 bit math
 in environments that don't natively support 64 bit math by using bit
 shift operations.

 @param OriginalValue The value to multiply.

 @param MultiplyBy A value to multiply by, which must consist of a power of
        two value.

 @return The resulting multiplied value.
 */
LARGE_INTEGER
SdirMultiplyViaShift(
    __in LARGE_INTEGER OriginalValue,
    __in DWORD MultiplyBy
    )
{
    DWORD ShiftCount = 0;
    DWORD ShiftedMultiplyBy = MultiplyBy;
    LARGE_INTEGER ReturnValue;

    if (MultiplyBy == 0) {
        ReturnValue.HighPart = 0;
        ReturnValue.LowPart = 0;
        return ReturnValue;
    }

    while ((ShiftedMultiplyBy & 1) == 0) {
        ShiftedMultiplyBy = ShiftedMultiplyBy >> 1;
        ShiftCount++;
    }

    ReturnValue.HighPart = OriginalValue.HighPart << ShiftCount;
    if (ShiftCount > 0) {
       ReturnValue.HighPart += OriginalValue.LowPart >> (32 - ShiftCount);
    }
    ReturnValue.LowPart = OriginalValue.LowPart << ShiftCount;

    return ReturnValue;
}

/**
 Update summary information for free and total disk space from a path using
 GetDiskFreeSpace.  This is a fallback path when GetDiskFreeSpaceEx is not
 available.  Because GetDiskFreeSpace relies on the caller doing 64 bit math,
 this is emulated using @ref SdirMultiplyViaShift .

 @param Path Pointer to a string specifying the path to query free disk space
        for.

 @param LocalSummary Pointer to the summary information to update with free
        disk space information.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirPopulateSummaryWithGetDiskFreeSpace(
    __in LPTSTR Path,
    __inout PSDIR_SUMMARY LocalSummary
    )
{
    LARGE_INTEGER Temp;
    ULONG BytesPerSector;
    ULONG SectorsPerCluster;
    ULONG FreeClusters;
    ULONG TotalClusters;

    //
    //  This path requires 64 bit multiplication which is emulated using
    //  shifts since we can't rely on CRT support for it.
    //

    if (GetDiskFreeSpace(Path, &SectorsPerCluster, &BytesPerSector, &FreeClusters, &TotalClusters)) {
        Temp.HighPart = 0;
        Temp.LowPart = TotalClusters;

        LocalSummary->VolumeSize = SdirMultiplyViaShift(SdirMultiplyViaShift(Temp, SectorsPerCluster), BytesPerSector);

        Temp.HighPart = 0;
        Temp.LowPart = FreeClusters;
        LocalSummary->FreeSize = SdirMultiplyViaShift(SdirMultiplyViaShift(Temp, SectorsPerCluster), BytesPerSector);
        return TRUE;
    }
    return FALSE;
}

// vim:sw=4:ts=4:et:
