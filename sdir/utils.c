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
//  Sorting support
//

/**
 Compare two 64 bit unsigned large integers and return which is greater than
 the other, or if the two are equal.

 @param Left Pointer to the first integer to compare.

 @param Right Pointer to the second integer to compare.

 @return SDIR_LESS_THAN if the first integer is less than the second;
         SDIR_GREATER_THAN if the first integer is greater than the second;
         SDIR_EQUAL if the two are identical.
 */
DWORD
SdirCompareLargeInt (
    __in PULARGE_INTEGER Left,
    __in PULARGE_INTEGER Right
    )
{
    if (Left->HighPart < Right->HighPart) {
        return SDIR_LESS_THAN;
    } else if (Left->HighPart > Right->HighPart) {
        return SDIR_GREATER_THAN;
    }

    if (Left->LowPart < Right->LowPart) {
        return SDIR_LESS_THAN;
    } else if (Left->LowPart > Right->LowPart) {
        return SDIR_GREATER_THAN;
    }

    return SDIR_EQUAL;
}

/**
 Compare two NULL terminated strings and return which is greater than the
 other, or if the two are equal.

 @param Left Pointer to the first string to compare.

 @param Right Pointer to the second string to compare.

 @return SDIR_LESS_THAN if the first string is less than the second;
         SDIR_GREATER_THAN if the first string is greater than the second;
         SDIR_EQUAL if the two are identical.
 */
DWORD
SdirCompareString (
    __in LPCTSTR Left,
    __in LPCTSTR Right
    )
{
    int ret = _tcsicmp(Left, Right);

    if (ret < 0) {
        return SDIR_LESS_THAN;
    } else if (ret == 0) {
        return SDIR_EQUAL;
    } else {
        return SDIR_GREATER_THAN;
    }
}

/**
 Compare two NT formatted timestamps and return which date is greater than the
 other, or if the two are equal.  Note specifically that this routine does not
 care about time components in the timestamp, only date components.

 @param Left Pointer to the date to compare.

 @param Right Pointer to the date to compare.

 @return SDIR_LESS_THAN if the first date is less than the second;
         SDIR_GREATER_THAN if the first date is greater than the second;
         SDIR_EQUAL if the two are identical.
 */
DWORD
SdirCompareDate (
    __in LPSYSTEMTIME Left,
    __in LPSYSTEMTIME Right
    )
{
    if (Left->wYear < Right->wYear) {
        return SDIR_LESS_THAN;
    } else if (Left->wYear > Right->wYear) {
        return SDIR_GREATER_THAN;
    }

    if (Left->wMonth < Right->wMonth) {
        return SDIR_LESS_THAN;
    } else if (Left->wMonth > Right->wMonth) {
        return SDIR_GREATER_THAN;
    }

    if (Left->wDay < Right->wDay) {
        return SDIR_LESS_THAN;
    } else if (Left->wDay > Right->wDay) {
        return SDIR_GREATER_THAN;
    }

    return SDIR_EQUAL;
}

/**
 Compare two NT formatted timestamps and return which time is greater than the
 other, or if the two are equal.  Note specifically that this routine does not
 care about date components in the timestamp, only time components.

 @param Left Pointer to the time to compare.

 @param Right Pointer to the time to compare.

 @return SDIR_LESS_THAN if the first time is less than the second;
         SDIR_GREATER_THAN if the first time is greater than the second;
         SDIR_EQUAL if the two are identical.
 */
DWORD
SdirCompareTime (
    __in LPSYSTEMTIME Left,
    __in LPSYSTEMTIME Right
    )
{
    if (Left->wHour < Right->wHour) {
        return SDIR_LESS_THAN;
    } else if (Left->wHour > Right->wHour) {
        return SDIR_GREATER_THAN;
    }

    if (Left->wMinute < Right->wMinute) {
        return SDIR_LESS_THAN;
    } else if (Left->wMinute > Right->wMinute) {
        return SDIR_GREATER_THAN;
    }

    if (Left->wSecond < Right->wSecond) {
        return SDIR_LESS_THAN;
    } else if (Left->wSecond > Right->wSecond) {
        return SDIR_GREATER_THAN;
    }

    if (Left->wMilliseconds < Right->wMilliseconds) {
        return SDIR_LESS_THAN;
    } else if (Left->wMilliseconds > Right->wMilliseconds) {
        return SDIR_GREATER_THAN;
    }

    return SDIR_EQUAL;
}

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

/**
 Parse a NULL terminated string and return a 64 bit unsigned integer from
 the result.  This function will only parse hex inputs but will tolerate any
 prefix.  It can optionally indicate the end of the string that it processed
 (meaning, the first non numeric character in the string.)

 @param str Pointer to the string to parse.

 @param endptr Optionally points to a value to receive a pointer to the first
        non-numeric character in the string.

 @return The parsed, 64 bit unsigned integer value from the string.
 */
LARGE_INTEGER
SdirStringToNum64(
    __in LPCTSTR str,
    __out_opt LPCTSTR* endptr
    )
{
    LARGE_INTEGER ret = {0};
    if (str[0] == '0' && str[1] == 'x') {
        str += 2;
    }
    while ((*str >= '0' && *str <= '9') || (*str >= 'a' && *str <= 'f') || (*str >= 'A' && *str <= 'F')) {
        ret.HighPart = (ret.HighPart<<4) + ((ret.LowPart & 0xf0000000)>>28);
        ret.LowPart *= 16;
        if (*str >= '0' && *str <= '9') {
            ret.LowPart += *str - '0';
        } else if (*str >= 'a' && *str <= 'f') {
            ret.LowPart += *str - 'a' + 10;
        } else if (*str >= 'A' && *str <= 'F') {
            ret.LowPart += *str - 'A' + 10;
        }
        str++;
    }
    if (endptr) {
        *endptr = str;
    }
    return ret;
}

/**
 Parse a NULL terminated string containing hex digits and generate a string
 that encodes each two hex digits into a byte.

 @param str Pointer to the string to parse.

 @param buffer Pointer to a buffer to populate with a string representation of
        the hex characters.  Note this string is always 8 bit, not TCHARs.

 @param size Specifies the length of buffer, in bytes.

 @return TRUE to indicate parse success, FALSE to indicate failure.  Failure
         occurs if the input stream is not compliant hex, or is not aligned in
         two character pairs.
 */
BOOL
SdirStringToHexBuffer(
    __in LPCTSTR str,
    __out PUCHAR buffer,
    __in DWORD size
    )
{
    UCHAR thischar;
    DWORD offset;
    DWORD digit;

    //
    //  Loop through the output string
    //

    for (offset = 0; offset < size; offset++) {

        thischar = 0;
        digit = 0;

        //
        //  So long as we have a valid character, populate thischar.  Do this exactly
        //  twice, so we have one complete byte.
        //

        while ((digit < 2) &&
               ((*str >= '0' && *str <= '9') || (*str >= 'a' && *str <= 'f') || (*str >= 'A' && *str <= 'F'))) {
            thischar *= 16;
            if (*str >= '0' && *str <= '9') {
                thischar = (UCHAR)(thischar + *str - '0');
            } else if (*str >= 'a' && *str <= 'f') {
                thischar = (UCHAR)(thischar + *str - 'a' + 10);
            } else if (*str >= 'A' && *str <= 'F') {
                thischar = (UCHAR)(thischar + *str - 'A' + 10);
            }
            str++;
            digit++;
        }

        //
        //  If we did actually get two input chars, output the byte and go back for
        //  more.  Otherwise, the input doesn't match the output requirement
        //

        if (digit == 2) {
            buffer[offset] = thischar;
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 Parse a NULL terminated string specifying a file size and return a 64 bit
 unsigned integer from the result.  This function can parse prefixed hex or
 decimal inputs, and understands file size qualifiers (k, m, g et al.)

 @param str Pointer to the string to parse.

 @return The parsed, 64 bit unsigned integer value from the string.
 */
LARGE_INTEGER
SdirStringToFileSize(
    __in LPCTSTR str
    )
{
    TCHAR Suffixes[] = {'b', 'k', 'm', 'g', 't'};
    DWORD SuffixLevel = 0;
    LARGE_INTEGER FileSize;
    DWORD i;
    LPCTSTR endptr;

    FileSize.HighPart = 0;
    FileSize.LowPart = SdirStringToNum32(str, &endptr);

    for (i = 0; i < sizeof(Suffixes)/sizeof(Suffixes[0]); i++) {
        if (*endptr == Suffixes[i] || *endptr == (Suffixes[i] + 'A' - 'a')) {
            SuffixLevel = i;
            break;
        }
    }

    while(SuffixLevel) {

        FileSize.HighPart = (FileSize.HighPart << 10) + (FileSize.LowPart >> 22);
        FileSize.LowPart = (FileSize.LowPart << 10);

        SuffixLevel--;
    }

    return FileSize;
}

/**
 Parse a NULL terminated string specifying a file date and return a timestamp
 from the result.

 @param str Pointer to the string to parse.

 @param date On successful completion, updated to point to a timestamp.

 @return TRUE to indicate success, FALSE to indicate parse failure.
 */
BOOL
SdirStringToDate(
    __in LPCTSTR str,
    __out LPSYSTEMTIME date
    )
{
    LPCTSTR endptr = NULL;
    date->wYear = (WORD)SdirStringToNum32(str, &endptr);
    if (date->wYear < 100) {
        date->wYear += 2000;
    }
    if (*endptr == '/') {
        date->wMonth = (WORD)SdirStringToNum32(endptr + 1, &endptr);
        if (*endptr == '/') {
            date->wDay = (WORD)SdirStringToNum32(endptr + 1, &endptr);
        }
    }

    return TRUE;
}

/**
 Parse a NULL terminated string specifying a file time and return a timestamp
 from the result.

 @param str Pointer to the string to parse.

 @param date On successful completion, updated to point to a timestamp.

 @return TRUE to indicate success, FALSE to indicate parse failure.
 */
BOOL
SdirStringToTime(
    __in LPCTSTR str,
    __out LPSYSTEMTIME date
    )
{
    LPCTSTR endptr = NULL;
    date->wHour = (WORD)SdirStringToNum32(str, &endptr);
    if (*endptr == ':') {
        date->wMinute = (WORD)SdirStringToNum32(endptr + 1, &endptr);
        if (*endptr == ':') {
            date->wSecond = (WORD)SdirStringToNum32(endptr + 1, &endptr);
        }
    }

    return TRUE;
}

/**
 Copy a file name from one buffer to another, sanitizing unprintable
 characters into ?'s.

 @param Dest Pointer to the string to copy the file name to.

 @param Src Pointer to the string to copy the file name from.

 @param MaxLength Specifies the size of dest, in bytes.  No characters will be
        written beyond this value (ie., this value includes space for NULL.)

 @param ValidCharCount Optionally points to an integer to populate with the
        number of characters read from the source.  This can be less than the
        length of the source is MaxLength is reached.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCopyFileName(
    __out LPTSTR Dest,
    __in LPCTSTR Src,
    __in DWORD MaxLength,
    __out_opt PDWORD ValidCharCount
    )
{
    DWORD Index;
    DWORD Length = MaxLength - 1;

    for (Index = 0; Index < Length; Index++) {
        if (Src[Index] == 0) {
            break;
        } else if (Src[Index] < 32) {
            Dest[Index] = '?';
        } else {
            Dest[Index] = Src[Index];
        }
    }
    Dest[Index] = '\0';

    if (ValidCharCount != NULL) {
        *ValidCharCount = Index;
    }

    return TRUE;
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
    TCHAR Suffixes[] = {'b', 'k', 'm', 'g', 't', '?'};
    DWORD SuffixLevel = 0;
    TCHAR StrSize[10];
        
    if (Buffer) {
        LARGE_INTEGER Size = *GenericSize;
        LARGE_INTEGER OldSize = Size;

        while (Size.HighPart != 0 || Size.LowPart > 9999) {
            SuffixLevel++;
            OldSize = Size;

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
            YoriLibSPrintfS(StrSize,
                        sizeof(StrSize)/sizeof(StrSize[0]),
                        _T(" %2i.%1i%c"),
                        (int)Size.LowPart,
                        (int)OldSize.LowPart,
                        Suffixes[SuffixLevel]);
        } else {
            YoriLibSPrintfS(StrSize,
                        sizeof(StrSize)/sizeof(StrSize[0]),
                        _T(" %4i%c"),
                        (int)Size.LowPart,
                        Suffixes[SuffixLevel]);
        }
            
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
