/**
 * @file sdir/callbacks.c
 *
 * Colorful, sorted and optionally rich directory enumeration
 * for Windows.
 *
 * This module implements functions to collect, display, sort, and deserialize
 * individual data types associated with files that we can enumerate.
 *
 * Copyright (c) 2014-2021 Malcolm J. Smith
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
//  File enumeration support
//


/**
 Collect information from a directory entry into the active summary count.

 @param Entry The directory entry to count in the summary.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectSummary(
    __in PYORI_FILE_INFO Entry
    )
{

    //
    //  Don't count . and .. at all
    //

    if ((Entry->FileNameLengthInChars == 1 && Entry->FileName[0] == '.') ||
        (Entry->FileNameLengthInChars == 2 && Entry->FileName[0] == '.' && Entry->FileName[1] == '.')) {

        return TRUE;
    }

    //
    //  Otherwise, count either as a file or a dir
    //

    if (Entry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        Summary->NumDirs++;
    } else {
        Summary->NumFiles++;
    }

    Summary->TotalSize += SdirFileSizeFromLargeInt(&Entry->FileSize);

    if (Opts->FtCompressedFileSize.Flags & SDIR_FEATURE_COLLECT) {
        Summary->CompressedSize += SdirFileSizeFromLargeInt(&Entry->CompressedFileSize);
    }

    return TRUE;
}


//
//  Specific formatting and sizing callbacks for each supported piece of file metadata.
//

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's last access date.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFileAccessDate (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    return SdirDisplayFileDate(Buffer, Attributes, &Entry->AccessTime);
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's last access time.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFileAccessTime (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    return SdirDisplayFileTime(Buffer, Attributes, &Entry->AccessTime);
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's allocated range count.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayAllocatedRangeCount (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR Str[8];

    if (Buffer) {

        //
        //  As a special hack to suppress the 'b' suffix, just display as
        //  a number unless it's six digits.  At that point we know we'll
        //  have a 'k' suffix or above.
        //

        if (SdirFileSizeFromLargeInt(&Entry->AllocatedRangeCount) <= 99999) { 
            YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" %05i"), (int)Entry->AllocatedRangeCount.LowPart);
            SdirPasteStr(Buffer, Str, Attributes, 6);
        } else {
            SdirDisplayGenericSize(Buffer, Attributes, &Entry->AllocatedRangeCount);
        }
    }
    return 6;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's allocation size.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayAllocationSize (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    return SdirDisplayGenericSize(Buffer, Attributes, &Entry->AllocationSize);
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for an executable's CPU architecture.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayArch (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR StrBuf[8];
    LPTSTR StrFixed;

    if (Buffer) {
        switch(Entry->Architecture) {
            case IMAGE_FILE_MACHINE_UNKNOWN:
                StrFixed = _T("None ");
                break;
            case IMAGE_FILE_MACHINE_I386:
                StrFixed = _T("i386 ");
                break;
            case IMAGE_FILE_MACHINE_AMD64:
                StrFixed = _T("amd64");
                break;
            case IMAGE_FILE_MACHINE_ARMNT:
                StrFixed = _T("arm  ");
                break;
            case IMAGE_FILE_MACHINE_ARM64:
                StrFixed = _T("arm64");
                break;
            case IMAGE_FILE_MACHINE_R4000:
                StrFixed = _T("mips ");
                break;
            case IMAGE_FILE_MACHINE_POWERPC:
                StrFixed = _T("ppc  ");
                break;
            case IMAGE_FILE_MACHINE_ALPHA:
                StrFixed = _T("axp  ");
                break;
            case IMAGE_FILE_MACHINE_IA64:
                StrFixed = _T("ia64 ");
                break;
            case IMAGE_FILE_MACHINE_AXP64:
                StrFixed = _T("axp64");
                break;
            default:
                StrFixed = _T("New? ");
                break;
        }
        YoriLibSPrintfS(StrBuf, sizeof(StrBuf)/sizeof(StrBuf[0]), _T(" %5s"), StrFixed);
        SdirPasteStr(Buffer, StrBuf, Attributes, 6);
    }
    return 6;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's compression algorithm.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayCompressionAlgorithm (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR StrBuf[8];
    LPTSTR StrFixed;

    if (Buffer) {
        switch(Entry->CompressionAlgorithm) {
            case YoriLibCompressionNone:
                StrFixed = _T("None");
                break;
            case YoriLibCompressionLznt:
                StrFixed = _T("LZNT");
                break;
            case YoriLibCompressionNtfsUnknown:
                StrFixed = _T("NTFS");
                break;
            case YoriLibCompressionWim:
                StrFixed = _T("WIM ");
                break;
            case YoriLibCompressionLzx:
                StrFixed = _T("LZX ");
                break;
            case YoriLibCompressionXpress4k:
                StrFixed = _T("Xp4 ");
                break;
            case YoriLibCompressionXpress8k:
                StrFixed = _T("Xp8 ");
                break;
            case YoriLibCompressionXpress16k:
                StrFixed = _T("Xp16");
                break;
            case YoriLibCompressionWofFileUnknown:
                StrFixed = _T("File");
                break;
            case YoriLibCompressionWofUnknown:
                StrFixed = _T("Wof ");
                break;
            default:
                StrFixed = _T("BUG ");
                break;
        }
        YoriLibSPrintfS(StrBuf, sizeof(StrBuf)/sizeof(StrBuf[0]), _T(" %4s"), StrFixed);
        SdirPasteStr(Buffer, StrBuf, Attributes, 5);
    }
    return 5;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a directory's case sensitivity state.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayCaseSensitivity (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR StrBuf[4];
    LPTSTR StrFixed;
    if (Buffer) {
        if (Entry->CaseSensitive) {
            StrFixed = _T("cs");
        } else {
            StrFixed = _T("ci");
        }

        YoriLibSPrintfS(StrBuf, sizeof(StrBuf)/sizeof(StrBuf[0]), _T(" %2s"), StrFixed);
        SdirPasteStr(Buffer, StrBuf, Attributes, 3);
    }
    return 3;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's compressed file size.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayCompressedFileSize (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    return SdirDisplayGenericSize(Buffer, Attributes, &Entry->CompressedFileSize);
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's description.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayDescription (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    ULONG CurrentChar = 0;
    DWORD ShortLength;

    if (Buffer) {
        SdirPasteStrAndPad(&Buffer[CurrentChar], NULL, Attributes, 0, 1);
    }
    CurrentChar++;

    if (Buffer) {
        ShortLength = (DWORD)_tcslen(Entry->Description);

        SdirPasteStrAndPad(&Buffer[CurrentChar], Entry->Description, Attributes, ShortLength, sizeof(Entry->Description)/sizeof(Entry->Description[0]) - 1);
    }
    CurrentChar += sizeof(Entry->Description)/sizeof(Entry->Description[0]) - 1;
    return CurrentChar;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's effective permissions.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayEffectivePermissions (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR StrAtts[32];
    DWORD i;

    PCYORI_LIB_CHAR_TO_DWORD_FLAG Pairs;
    DWORD PairCount;

    YoriLibGetFilePermissionPairs(&PairCount, &Pairs);

    if (Buffer) {
        StrAtts[0] = ' ';
        for (i = 0; i < PairCount; i++) {
            if (Entry->EffectivePermissions & Pairs[i].Flag) {
                StrAtts[i + 1] = Pairs[i].DisplayLetter;
            } else {
                StrAtts[i + 1] = '-';
            }
        }

        StrAtts[i + 1] = '\0';

        SdirPasteStr(Buffer, StrAtts, Attributes, PairCount + 1);
    }
    return PairCount + 1;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's create date.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFileCreateDate (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    return SdirDisplayFileDate(Buffer, Attributes, &Entry->CreateTime);
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's create time.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFileCreateTime (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    return SdirDisplayFileTime(Buffer, Attributes, &Entry->CreateTime);
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's attributes.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFileAttributes (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR StrAtts[32];
    DWORD i;
    PCYORI_LIB_CHAR_TO_DWORD_FLAG Pairs;
    DWORD PairCount;

    YoriLibGetFileAttrPairs(&PairCount, &Pairs);

    if (Buffer) {
        StrAtts[0] = ' ';
        for (i = 0; i < PairCount; i++) {
            if (Entry->FileAttributes & Pairs[i].Flag) {
                StrAtts[i + 1] = Pairs[i].DisplayLetter;
            } else {
                StrAtts[i + 1] = '-';
            }
        }

        StrAtts[i + 1] = '\0';

        SdirPasteStr(Buffer, StrAtts, Attributes, PairCount + 1);
    }
    return PairCount + 1;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's ID.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFileId (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    return SdirDisplayHex64(Buffer, Attributes, &Entry->FileId);
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's size.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFileSize (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    if (Buffer) {
        if (Entry->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            if (Entry->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
                SdirPasteStr(Buffer, _T(" <LNK>"), Entry->RenderAttributes, 6);
            } else if (Entry->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
                SdirPasteStr(Buffer, _T(" <MNT>"), Entry->RenderAttributes, 6);
            } else if (Entry->ReparseTag == IO_REPARSE_TAG_APPEXECLINK) {
                SdirPasteStr(Buffer, _T(" <APP>"), Entry->RenderAttributes, 6);
            } else {
                return SdirDisplayGenericSize(Buffer, Attributes, &Entry->FileSize);
            }
            return 6;
        } else if (Entry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            SdirPasteStr(Buffer, _T(" <DIR>"), Entry->RenderAttributes, 6);
            return 6;
        } else {
            return SdirDisplayGenericSize(Buffer, Attributes, &Entry->FileSize);
        }
    }
    return 6;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's version string.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFileVersionString (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    ULONG CurrentChar = 0;
    DWORD ShortLength;

    if (Buffer) {
        SdirPasteStrAndPad(&Buffer[CurrentChar], NULL, Attributes, 0, 1);
    }
    CurrentChar++;

    if (Buffer) {
        ShortLength = (DWORD)_tcslen(Entry->FileVersionString);

        SdirPasteStrAndPad(&Buffer[CurrentChar], Entry->FileVersionString, Attributes, ShortLength, sizeof(Entry->FileVersionString)/sizeof(Entry->FileVersionString[0]) - 1);
    }
    CurrentChar += sizeof(Entry->FileVersionString)/sizeof(Entry->FileVersionString[0]) - 1;
    return CurrentChar;
}


/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's fragment count.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFragmentCount (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR Str[8];

    if (Buffer) {

        //
        //  As a special hack to suppress the 'b' suffix, just display as
        //  a number unless it's six digits.  At that point we know we'll
        //  have a 'k' suffix or above.
        //

        if (SdirFileSizeFromLargeInt(&Entry->FragmentCount) <= 99999) { 
            YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" %05i"), (int)Entry->FragmentCount.LowPart);
            SdirPasteStr(Buffer, Str, Attributes, 6);
        } else {
            SdirDisplayGenericSize(Buffer, Attributes, &Entry->FragmentCount);
        }
    }
    return 6;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's link count.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayLinkCount (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR Str[5];

    if (Buffer) {
        if (Entry->LinkCount >= 1000) {
            YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" >1k"));
        } else {
            YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" %03i"), (int)Entry->LinkCount);
        }
        SdirPasteStr(Buffer, Str, Attributes, 4);
    }
    return 4;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's object ID.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayObjectId (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    if (Buffer) {
        SdirDisplayGenericHexBuffer(Buffer, Attributes, (PUCHAR)&Entry->ObjectId, sizeof(Entry->ObjectId));
    }

    return 2 * sizeof(Entry->ObjectId) + 1;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for an executable's minimum OS version.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayOsVersion (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR Str[30];
    BYTE  ThisOsMajor = LOBYTE(LOWORD(Opts->OsVersion));
    BYTE  ThisOsMinor = HIBYTE(LOWORD(Opts->OsVersion));

    if (Buffer) {
        YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" %02i.%02i"), Entry->OsVersionHigh, Entry->OsVersionLow);

        if (Entry->OsVersionHigh > ThisOsMajor ||
            (Entry->OsVersionHigh == ThisOsMajor && Entry->OsVersionLow > ThisOsMinor)) {

            YoriLibSetColorToWin32(&Attributes, SDIR_FUTURE_VERSION_COLOR);
        }

        SdirPasteStr(Buffer, Str, Attributes, 6);
    }
    return 6;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's owner.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayOwner (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    ULONG CurrentChar = 0;
    DWORD ShortLength;

    if (Buffer) {
        SdirPasteStrAndPad(&Buffer[CurrentChar], NULL, Attributes, 0, 1);
    }
    CurrentChar++;

    if (Buffer) {
        ShortLength = (DWORD)_tcslen(Entry->Owner);

        SdirPasteStrAndPad(&Buffer[CurrentChar], Entry->Owner, Attributes, ShortLength, sizeof(Entry->Owner)/sizeof(Entry->Owner[0]) - 1);
    }
    CurrentChar += sizeof(Entry->Owner)/sizeof(Entry->Owner[0]) - 1;
    return CurrentChar;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's reparse tag.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayReparseTag (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    ULONG Tag = 0;
    if (Entry) {
        Tag = Entry->ReparseTag;
    }
    return SdirDisplayHex32(Buffer, Attributes, Tag);
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's short file name.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayShortName (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    ULONG CurrentChar = 0;
    DWORD ShortLength;

    //
    //  Just because we like special cases, add a space only if both
    //  names are being displayed.
    //

    if (Opts->FtFileName.Flags & SDIR_FEATURE_DISPLAY) {
        if (Buffer) {
            SdirPasteStrAndPad(&Buffer[CurrentChar], NULL, Attributes, 0, 1);
        }
        CurrentChar++;
    }

    if (Buffer) {
        ShortLength = (DWORD)_tcslen(Entry->ShortFileName);

        SdirPasteStrAndPad(&Buffer[CurrentChar], Entry->ShortFileName, Attributes, ShortLength, 12);
    }
    CurrentChar += 12;
    return CurrentChar;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for an executable's subsystem.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplaySubsystem (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR StrBuf[8];
    LPTSTR StrFixed;

    if (Buffer) {
        switch(Entry->Subsystem) {
            case IMAGE_SUBSYSTEM_UNKNOWN:
                StrFixed = _T("None");
                break;
            case IMAGE_SUBSYSTEM_NATIVE:
                StrFixed = _T("NT  ");
                break;
            case IMAGE_SUBSYSTEM_WINDOWS_GUI:
                StrFixed = _T("GUI ");
                break;
            case IMAGE_SUBSYSTEM_WINDOWS_CUI:
                StrFixed = _T("Cons");
                break;
            case IMAGE_SUBSYSTEM_OS2_CUI:
                StrFixed = _T("OS/2");
                break;
            case IMAGE_SUBSYSTEM_POSIX_CUI:
                StrFixed = _T("Posx");
                break;
            case IMAGE_SUBSYSTEM_NATIVE_WINDOWS:
                StrFixed = _T("w9x ");
                break;
            case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
                StrFixed = _T("CE  ");
                break;
            case IMAGE_SUBSYSTEM_EFI_APPLICATION:
                StrFixed = _T("EFIa");
                break;
            case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
                StrFixed = _T("EFIb");
                break;
            case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
                StrFixed = _T("EFId");
                break;
            case IMAGE_SUBSYSTEM_EFI_ROM:
                StrFixed = _T("EFIr");
                break;
            case IMAGE_SUBSYSTEM_XBOX:
                StrFixed = _T("Xbox");
                break;
            case IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG:
                StrFixed = _T("Xbcc");
                break;
            case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION:
                StrFixed = _T("Boot");
                break;
            default:
                StrFixed = _T("New?");
                break;
        }
        YoriLibSPrintfS(StrBuf, sizeof(StrBuf)/sizeof(StrBuf[0]), _T(" %4s"), StrFixed);
        SdirPasteStr(Buffer, StrBuf, Attributes, 5);
    }
    return 5;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's stream count.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayStreamCount (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR Str[5];

    if (Buffer) {
        if (Entry->StreamCount >= 1000) {
            YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" >1k"));
        } else {
            YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" %03i"), (int)Entry->StreamCount);
        }
        SdirPasteStr(Buffer, Str, Attributes, 4);
    }
    return 4;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's USN.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayUsn(
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    return SdirDisplayHex64(Buffer, Attributes, &Entry->Usn);
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for an executable's version.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayVersion(
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    TCHAR Str[40];

    if (Buffer) {
        YoriLibSPrintfS(Str,
                    sizeof(Str)/sizeof(Str[0]),
                    _T(" %03i.%03i.%05i.%05i %c%c"),
                    (int)(Entry->FileVersion.HighPart >> 16),
                    (int)(Entry->FileVersion.HighPart & 0xffff),
                    (int)(Entry->FileVersion.LowPart >> 16),
                    (int)(Entry->FileVersion.LowPart & 0xffff),
                    Entry->FileVersionFlags & VS_FF_DEBUG ? 'D' : '-',
                    Entry->FileVersionFlags & VS_FF_PRERELEASE ? 'P' : '-');

        SdirPasteStr(Buffer, Str, Attributes, 23);
    }
    return 23;
}

/**
 Display the summary text to the output device.

 @param DefaultAttributes Specifies the default color to use for the summary
        string.  Note individual fields within the summary string may still
        have their own colors.

 @return The number of characters written to the output device.
 */
ULONG
SdirDisplaySummary(
    __in YORILIB_COLOR_ATTRIBUTES DefaultAttributes
    )
{
    SDIR_FMTCHAR Buffer[200];
    TCHAR Str[100];
    DWORD Len;
    LARGE_INTEGER Size;
    ULONG CurrentChar = 0;

    YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" %i "), (int)Summary->NumFiles);
    Len = (DWORD)_tcslen(Str);
    SdirPasteStr(&Buffer[CurrentChar], Str, Opts->FtNumberFiles.HighlightColor, Len);
    CurrentChar += Len;

    SdirPasteStr(&Buffer[CurrentChar], _T("files,"), DefaultAttributes, 6);
    CurrentChar += 6;

    YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T(" %i "), (int)Summary->NumDirs);
    Len = (DWORD)_tcslen(Str);
    SdirPasteStr(&Buffer[CurrentChar], Str, Opts->FtNumberFiles.HighlightColor, Len);
    CurrentChar += Len;

    SdirPasteStr(&Buffer[CurrentChar], _T("dirs,"), DefaultAttributes, 5);
    CurrentChar += 5;

    Size.HighPart = 0;
    SdirFileSizeFromLargeInt(&Size) = Summary->TotalSize;

    CurrentChar += SdirDisplayGenericSize(&Buffer[CurrentChar], Opts->FtFileSize.HighlightColor, &Size);
    SdirPasteStr(&Buffer[CurrentChar], _T(" used,"), DefaultAttributes, 6);
    CurrentChar += 6;

    if (Opts->FtCompressedFileSize.Flags & SDIR_FEATURE_DISPLAY) {
        Size.HighPart = 0;
        SdirFileSizeFromLargeInt(&Size) = Summary->CompressedSize;

        CurrentChar += SdirDisplayGenericSize(&Buffer[CurrentChar], Opts->FtCompressedFileSize.HighlightColor, &Size);
        SdirPasteStr(&Buffer[CurrentChar], _T(" compressed,"), DefaultAttributes, 12);
        CurrentChar += 12;
    }

    CurrentChar += SdirDisplayGenericSize(&Buffer[CurrentChar], Opts->FtFileSize.HighlightColor, &Summary->VolumeSize);
    SdirPasteStr(&Buffer[CurrentChar], _T(" vol size,"), DefaultAttributes, 10);
    CurrentChar += 10;

    CurrentChar += SdirDisplayGenericSize(&Buffer[CurrentChar], Opts->FtFileSize.HighlightColor, &Summary->FreeSize);
    SdirPasteStr(&Buffer[CurrentChar], _T(" vol free"), DefaultAttributes, 9);
    CurrentChar += 9;

    SdirWrite(Buffer, CurrentChar);

    return CurrentChar;
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's write date.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFileWriteDate (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    return SdirDisplayFileDate(Buffer, Attributes, &Entry->WriteTime);
}

/**
 Take the data inside a directory entry, convert it to a formatted string, and
 output the result to a buffer for a file's write time.

 @param Buffer The formatted string to be updated to contain the information.
        If not specified, the length of characters needed to hold the result
        is returned.

 @param Attributes The color to use when writing the formatted data to the
        string.

 @param Entry The directory entry containing the information to write.

 @return The number of characters written to the buffer, or the number of
         characters required to hold the data if the buffer is not present.
 */
ULONG
SdirDisplayFileWriteTime (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
    )
{
    return SdirDisplayFileTime(Buffer, Attributes, &Entry->WriteTime);
}

/**
 Determine the offset of a feature within the global options structure.
 */
#define OPT_OS(x) FIELD_OFFSET(SDIR_OPTS, x)

/**
 This table corresponds to the supported options in the program.
 Each option has a flags value describing whether to collect or display
 the piece of metadata, a default, a switch to turn display on or off
 or sort by it, a callback function to tell how much space it will need,
 an optional sort compare function, an optional callback to generate the
 binary form of this from a command line string, and some help text.

 File names, extensions and attributes are always collected, because
 that's how we determine colors.

 This table is entirely const, so it exists in read only data in the
 executable file.  Each entry refers to the offset within the options
 structure which is where any dynamic configuration is recorded.
*/
const SDIR_OPT
SdirOptions[] = {

    {OPT_OS(FtAllocatedRangeCount),          _T("ac"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayAllocatedRangeCount,      YoriLibCollectAllocatedRangeCount,
        YoriLibCompareAllocatedRangeCount,   NULL,
        YoriLibGenerateAllocatedRangeCount,  "allocated range count"},

    {OPT_OS(FtAccessDate),                   _T("ad"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN|FOREGROUND_BLUE}},
        SdirDisplayFileAccessDate,           YoriLibCollectAccessTime,
        YoriLibCompareAccessDate,            NULL,
        YoriLibGenerateAccessDate,           "access date"},

    {OPT_OS(FtArch),                         _T("ar"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY}},
        SdirDisplayArch,                     YoriLibCollectArch,
        YoriLibCompareArch,                  NULL,
        YoriLibGenerateArch,                 "CPU architecture"},

    {OPT_OS(FtAllocationSize),               _T("as"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY}},
        SdirDisplayAllocationSize,           YoriLibCollectAllocationSize,
        YoriLibCompareAllocationSize,        NULL,
        YoriLibGenerateAllocationSize,       "allocation size"},

    {OPT_OS(FtAccessTime),                   _T("at"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN|FOREGROUND_BLUE}},
        SdirDisplayFileAccessTime,           YoriLibCollectAccessTime,
        YoriLibCompareAccessTime,            NULL,
        YoriLibGenerateAccessTime,           "access time"},

    {OPT_OS(FtCompressionAlgorithm),         _T("ca"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayCompressionAlgorithm,     YoriLibCollectCompressionAlgorithm,
        YoriLibCompareCompressionAlgorithm,  NULL,
        YoriLibGenerateCompressionAlgorithm, "compression algorithm"},

    {OPT_OS(FtCreateDate),                   _T("cd"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayFileCreateDate,           YoriLibCollectCreateTime,
        YoriLibCompareCreateDate,            NULL,
        YoriLibGenerateCreateDate,           "create date"},

    {OPT_OS(FtCaseSensitivity),              _T("ci"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayCaseSensitivity,          YoriLibCollectCaseSensitivity,
        YoriLibCompareCaseSensitivity,       NULL,
        YoriLibGenerateCaseSensitivity,      "case insensitivity"},

    {OPT_OS(FtCompressedFileSize),           _T("cs"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY}},
        SdirDisplayCompressedFileSize,       YoriLibCollectCompressedFileSize,
        YoriLibCompareCompressedFileSize,    NULL,
        YoriLibGenerateCompressedFileSize,   "compressed size"},

    {OPT_OS(FtCreateTime),                   _T("ct"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayFileCreateTime,           YoriLibCollectCreateTime,
        YoriLibCompareCreateTime,            NULL,
        YoriLibGenerateCreateTime,           "create time"},

    {OPT_OS(FtDescription),                  _T("de"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayDescription,              YoriLibCollectDescription,
        YoriLibCompareDescription,           NULL,
        YoriLibGenerateDescription,          "description"},

    {OPT_OS(FtDirectory),                    _T("dr"),
        {SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_FILE, 0}},
        NULL,                                YoriLibCollectFileAttributes,
        YoriLibCompareDirectory,             NULL,
        YoriLibGenerateDirectory,            "directory"},

    {OPT_OS(FtEffectivePermissions),         _T("ep"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayEffectivePermissions,     YoriLibCollectEffectivePermissions,
        YoriLibCompareEffectivePermissions,  YoriLibBitwiseEffectivePermissions,
        YoriLibGenerateEffectivePermissions, "effective permissions"},

    {OPT_OS(FtError),                        _T("er"),
        {0, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_INTENSITY}},
        NULL,                                NULL,
        NULL,                                NULL,
        NULL,                                "errors"},

    {OPT_OS(FtFileAttributes),               _T("fa"),
        {SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_DISPLAY, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY}},
        SdirDisplayFileAttributes,           YoriLibCollectFileAttributes,
        YoriLibCompareFileAttributes,        YoriLibBitwiseFileAttributes,
        YoriLibGenerateFileAttributes,       "file attributes"},

    {OPT_OS(FtFragmentCount),                _T("fc"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayFragmentCount,            YoriLibCollectFragmentCount,
        YoriLibCompareFragmentCount,         NULL,
        YoriLibGenerateFragmentCount,        "fragment count"},

    {OPT_OS(FtFileExtension),                _T("fe"),
        {SDIR_FEATURE_DISPLAY|SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_SORT|SDIR_FEATURE_FIXED_COLOR, {0, 0}},
        NULL,                                YoriLibCollectFileExtension,
        YoriLibCompareFileExtension,         NULL,
        YoriLibGenerateFileExtension,        "file extension"},

    {OPT_OS(FtFileId),                       _T("fi"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayFileId,                   YoriLibCollectFileId,
        YoriLibCompareFileId,                NULL,
        NULL,                                "file id"},

    {OPT_OS(FtFileName),                     _T("fn"),
        {SDIR_FEATURE_DISPLAY|SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT|SDIR_FEATURE_USE_FILE_COLOR, {0, 0}},
        NULL,                                YoriLibCollectFileName,
        YoriLibCompareFileName,              YoriLibBitwiseFileName,
        YoriLibGenerateFileName,             "file name"},

    {OPT_OS(FtFileSize),                     _T("fs"),
        {SDIR_FEATURE_DISPLAY|SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY}},
        SdirDisplayFileSize,                 YoriLibCollectFileSize,
        YoriLibCompareFileSize,              NULL,
        YoriLibGenerateFileSize,             "file size"},

    {OPT_OS(FtFileVersionString),            _T("fv"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayFileVersionString,        YoriLibCollectFileVersionString,
        YoriLibCompareFileVersionString,     NULL,
        YoriLibGenerateFileVersionString,    "file version string"},

    {OPT_OS(FtGrid),                         _T("gr"),
        {0, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN}},
        NULL,                                NULL,
        NULL,                                NULL,
        NULL,                                "grid"},

    {OPT_OS(FtLinkCount),                    _T("lc"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayLinkCount,                YoriLibCollectLinkCount,
        YoriLibCompareLinkCount,             NULL,
        YoriLibGenerateLinkCount,            "link count"},

    {OPT_OS(FtNumberFiles),                  _T("nf"),
        {0, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN|FOREGROUND_INTENSITY}},
        NULL,                                NULL,
        NULL,                                NULL,
        NULL,                                "number files"},

#ifdef UNICODE
    {OPT_OS(FtNamedStreams),                 _T("ns"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_FIXED_COLOR, {0, 0}},
        NULL,                                NULL,
        NULL,                                NULL,
        NULL,                                "named streams"},
#endif

    {OPT_OS(FtObjectId),                     _T("oi"),
        {SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayObjectId,                 YoriLibCollectObjectId,
        YoriLibCompareObjectId,              NULL,
        YoriLibGenerateObjectId,             "object id"},

    {OPT_OS(FtOsVersion),                    _T("os"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayOsVersion,                YoriLibCollectOsVersion,
        YoriLibCompareOsVersion,             NULL,
        YoriLibGenerateOsVersion,            "minimum OS version"},

    {OPT_OS(FtOwner),                        _T("ow"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayOwner,                    YoriLibCollectOwner,
        YoriLibCompareOwner,                 NULL,
        YoriLibGenerateOwner,                "owner"},

    {OPT_OS(FtReparseTag),                   _T("rt"),
        {SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayReparseTag,               YoriLibCollectReparseTag,
        YoriLibCompareReparseTag,            NULL,
        YoriLibGenerateReparseTag,           "reparse tag"},

#ifdef UNICODE
    {OPT_OS(FtStreamCount),                  _T("sc"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayStreamCount,              YoriLibCollectStreamCount,
        YoriLibCompareStreamCount,           NULL,
        YoriLibGenerateStreamCount,          "stream count"},
#endif

    {OPT_OS(FtSummary),                      _T("sm"),
        {SDIR_FEATURE_DISPLAY|SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_DISPLAY, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE}},
        NULL,                                NULL,
        NULL,                                NULL,
        NULL,                                "summary"},

    {OPT_OS(FtShortName),                    _T("sn"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT|SDIR_FEATURE_USE_FILE_COLOR, {0, 0}},
        SdirDisplayShortName,                YoriLibCollectShortName,
        YoriLibCompareShortName,             NULL,
        YoriLibGenerateShortName,            "short name"},

    {OPT_OS(FtSubsystem),                    _T("ss"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplaySubsystem,                YoriLibCollectSubsystem,
        YoriLibCompareSubsystem,             NULL,
        YoriLibGenerateSubsystem,            "subsystem"},

    {OPT_OS(FtUsn),                          _T("us"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayUsn,                      YoriLibCollectUsn,
        YoriLibCompareUsn,                   NULL,
        YoriLibGenerateUsn,                  "USN"},

    {OPT_OS(FtVersion),                      _T("vr"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN}},
        SdirDisplayVersion,                  YoriLibCollectVersion,
        YoriLibCompareVersion,               NULL,
        YoriLibGenerateVersion,              "version"},

    {OPT_OS(FtWriteDate),                    _T("wd"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN}},
        SdirDisplayFileWriteDate,            YoriLibCollectWriteTime,
        YoriLibCompareWriteDate,             NULL,
        YoriLibGenerateWriteDate,            "write date"},

    {OPT_OS(FtWriteTime),                    _T("wt"),
        {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, {YORILIB_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN}},
        SdirDisplayFileWriteTime,            YoriLibCollectWriteTime,
        YoriLibCompareWriteTime,             NULL,
        YoriLibGenerateWriteTime,            "write time"},
};

/**
 Return the count of features that exist in the table of things supported.
 This exists so other modules can know about how many things there are
 without having access to know what all of the things are.
 */
DWORD
SdirGetNumSdirOptions(VOID)
{
    return sizeof(SdirOptions)/sizeof(SdirOptions[0]);
}

/**
 This table is really quite redundant, but it corresponds to the order
 in which we will render each piece of metadata.  The table above is
 the order we display help.
 */
const SDIR_EXEC
SdirExec[] = {
    {OPT_OS(FtFileId),               SdirDisplayFileId},
    {OPT_OS(FtUsn),                  SdirDisplayUsn},
    {OPT_OS(FtFileSize),             SdirDisplayFileSize},
    {OPT_OS(FtCompressedFileSize),   SdirDisplayCompressedFileSize},
    {OPT_OS(FtAllocationSize),       SdirDisplayAllocationSize},
    {OPT_OS(FtFileAttributes),       SdirDisplayFileAttributes},
    {OPT_OS(FtObjectId),             SdirDisplayObjectId},
    {OPT_OS(FtReparseTag),           SdirDisplayReparseTag},
    {OPT_OS(FtLinkCount),            SdirDisplayLinkCount},
    {OPT_OS(FtStreamCount),          SdirDisplayStreamCount},
    {OPT_OS(FtOwner),                SdirDisplayOwner},
    {OPT_OS(FtEffectivePermissions), SdirDisplayEffectivePermissions},
    {OPT_OS(FtCreateDate),           SdirDisplayFileCreateDate},
    {OPT_OS(FtCreateTime),           SdirDisplayFileCreateTime},
    {OPT_OS(FtWriteDate),            SdirDisplayFileWriteDate},
    {OPT_OS(FtWriteTime),            SdirDisplayFileWriteTime},
    {OPT_OS(FtAccessDate),           SdirDisplayFileAccessDate},
    {OPT_OS(FtAccessTime),           SdirDisplayFileAccessTime},
    {OPT_OS(FtVersion),              SdirDisplayVersion},
    {OPT_OS(FtOsVersion),            SdirDisplayOsVersion},
    {OPT_OS(FtArch),                 SdirDisplayArch},
    {OPT_OS(FtSubsystem),            SdirDisplaySubsystem},
    {OPT_OS(FtDescription),          SdirDisplayDescription},
    {OPT_OS(FtFileVersionString),    SdirDisplayFileVersionString},
    {OPT_OS(FtCompressionAlgorithm), SdirDisplayCompressionAlgorithm},
    {OPT_OS(FtFragmentCount),        SdirDisplayFragmentCount},
    {OPT_OS(FtAllocatedRangeCount),  SdirDisplayAllocatedRangeCount},
    {OPT_OS(FtCaseSensitivity),      SdirDisplayCaseSensitivity},
};

/**
 Return the count of features that exist in the table of things to execute.
 This exists so other modules can know about how many things there are to do
 without having access to know what all of the things are.
 */
DWORD
SdirGetNumSdirExec(VOID)
{
    return sizeof(SdirExec)/sizeof(SdirExec[0]);
}

// vim:sw=4:ts=4:et:
