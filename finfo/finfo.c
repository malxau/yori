/**
 * @file finfo/finfo.c
 *
 * Yori shell display file metadata
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
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

#include <yoripch.h>
#include <yorilib.h>

/**
 Help text to display to the user.
 */
const
CHAR strFInfoHelpText[] =
        "\n"
        "Output information about file metadata.\n"
        "\n"
        "FINFO [-license] [-b] [-d] [-f fmt] [-s] <file>...\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -d             Return directories rather than directory contents\n"
        "   -f             Specify a custom format string\n"
        "   -s             Process files from all subdirectories\n";

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _FINFO_CONTEXT {

    /**
     The format string to apply when deciding which variables associated
     with the file to output and the form with which to output them.
     */
    YORI_STRING FormatString;

    /**
     Pointer to the full file path for a matching file.
     */
    PYORI_STRING FilePath;

    /**
     Pointer to the directory information from the matching file.
     */
    PWIN32_FIND_DATA FileInfo;

    /**
     Extracted information about the file.
     */
    YORI_FILE_INFO Entry;

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     Records the total number of files processed for each argument processed.
     */
    LONGLONG FilesFoundThisArg;

} FINFO_CONTEXT, *PFINFO_CONTEXT;

/**
 Specifies a pointer to a function which can collect file information from
 the disk or file system for some particular piece of data.
 */
typedef BOOL (* PFINFO_COLLECT_FN)(PYORI_FILE_INFO, PWIN32_FIND_DATA, PYORI_STRING);

/**
 Specifies a pointer to a function which can output a particular piece of file
 information.
 */
typedef YORI_ALLOC_SIZE_T (* PFINFO_OUTPUT_FN)(PFINFO_CONTEXT, PYORI_STRING);

/**
 Output a 64 bit integer.

 @param LargeInt A large integer to output.

 @param NumberBase Specifies the numeric base to use.  Should be 10 for
        decimal or 16 for hex.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputLargeInteger(
    __in LARGE_INTEGER LargeInt,
    __in WORD NumberBase,
    __inout PYORI_STRING OutputString
    )
{
    YORI_STRING String;
    TCHAR StringBuffer[32];
    YORI_MAX_SIGNED_T Value;

    YoriLibInitEmptyString(&String);
    String.StartOfString = StringBuffer;
    String.LengthAllocated = sizeof(StringBuffer)/sizeof(StringBuffer[0]);

    Value = (YORI_MAX_SIGNED_T)LargeInt.QuadPart;
    YoriLibNumberToString(&String, Value, NumberBase, 0, ' ');

    if (OutputString->LengthAllocated >= String.LengthInChars) {
        memcpy(OutputString->StartOfString, String.StartOfString, String.LengthInChars * sizeof(TCHAR));
    }

    return String.LengthInChars;
}

/**
 Output the access date year.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputAccessDateYear(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 4) {
        YoriLibYPrintf(OutputString, _T("%04i"), Context->Entry.AccessTime.wYear);
    }
    return 4;
}

/**
 Output the access date month.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputAccessDateMon(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.AccessTime.wMonth);
    }
    return 2;
}

/**
 Output the access date day.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputAccessDateDay(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.AccessTime.wDay);
    }
    return 2;
}

/**
 Output the access time hour.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputAccessTimeHour(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.AccessTime.wHour);
    }
    return 2;
}

/**
 Output the access time minute.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputAccessTimeMin(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.AccessTime.wMinute);
    }
    return 2;
}

/**
 Output the access time second.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputAccessTimeSec(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.AccessTime.wSecond);
    }
    return 2;
}


/**
 Output the allocated range count.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputAllocatedRangeCount(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.AllocatedRangeCount, 10, OutputString);
}

/**
 Output the allocation size.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputAllocationSize(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.AllocationSize, 10, OutputString);
}

/**
 Output the allocation size in hex.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputAllocationSizeHex(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.AllocationSize, 16, OutputString);
}

/**
 Output the case sensitivity state.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputCaseSensitivity(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 1) {
        YoriLibYPrintf(OutputString, _T("%01i"), Context->Entry.CaseSensitive);
    }
    return 1;
}

/**
 Output the compressed size.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputCompressedSize(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.CompressedFileSize, 10, OutputString);
}

/**
 Output the compressed size in hex.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputCompressedSizeHex(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.CompressedFileSize, 16, OutputString);
}

/**
 Output the create date year.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputCreateDateYear(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 4) {
        YoriLibYPrintf(OutputString, _T("%04i"), Context->Entry.CreateTime.wYear);
    }
    return 4;
}

/**
 Output the create date month.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputCreateDateMon(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.CreateTime.wMonth);
    }
    return 2;
}

/**
 Output the create date day.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputCreateDateDay(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.CreateTime.wDay);
    }
    return 2;
}

/**
 Output the create time hour.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputCreateTimeHour(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.CreateTime.wHour);
    }
    return 2;
}

/**
 Output the create time minute.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputCreateTimeMin(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.CreateTime.wMinute);
    }
    return 2;
}

/**
 Output the create time second.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputCreateTimeSec(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.CreateTime.wSecond);
    }
    return 2;
}

/**
 Output the file description from the version resource.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputDescription(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    YORI_ALLOC_SIZE_T DescriptionLength = (YORI_ALLOC_SIZE_T)_tcslen(Context->Entry.Description);
    if (OutputString->LengthAllocated >= DescriptionLength) {
        memcpy(OutputString->StartOfString, Context->Entry.Description, DescriptionLength * sizeof(TCHAR));
        OutputString->LengthInChars = DescriptionLength;
    }
    return DescriptionLength;
}

/**
 Output the file extension.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputExt(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    YORI_ALLOC_SIZE_T ExtLen = 0;

    if (Context->Entry.Extension != NULL) {
        ExtLen = (YORI_ALLOC_SIZE_T)_tcslen(Context->Entry.Extension);

        if (OutputString->LengthAllocated >= ExtLen) {
            memcpy(OutputString->StartOfString, Context->Entry.Extension, ExtLen * sizeof(TCHAR));
        }
    }

    return ExtLen;
}

/**
 Output the file ID.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputFileId(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.FileId, 10, OutputString);
}

/**
 Output the file ID in hex.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputFileIdHex(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.FileId, 16, OutputString);
}

/**
 Output the file version string.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputFileVersionString(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    YORI_ALLOC_SIZE_T FileVersionLength = (YORI_ALLOC_SIZE_T)_tcslen(Context->Entry.FileVersionString);
    if (OutputString->LengthAllocated >= FileVersionLength) {
        memcpy(OutputString->StartOfString, Context->Entry.FileVersionString, FileVersionLength * sizeof(TCHAR));
        OutputString->LengthInChars = FileVersionLength;
    }
    return FileVersionLength;
}

/**
 Output the fragment count.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputFragmentCount(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.FragmentCount, 10, OutputString);
}

/**
 Output the link count.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputLinkCount(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    LARGE_INTEGER liTemp;
    liTemp.HighPart = 0;
    liTemp.LowPart = Context->Entry.LinkCount;
    return FInfoOutputLargeInteger(liTemp, 10, OutputString);
}

/**
 Output the file name.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputName(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= Context->Entry.FileNameLengthInChars) {
        memcpy(OutputString->StartOfString, Context->Entry.FileName, Context->Entry.FileNameLengthInChars * sizeof(TCHAR));
        OutputString->LengthInChars = Context->Entry.FileNameLengthInChars;
    }
    return Context->Entry.FileNameLengthInChars;
}

/**
 Output the major required OS version.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputOsVerMajor(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    LARGE_INTEGER liTemp;
    liTemp.HighPart = 0;
    liTemp.LowPart = Context->Entry.OsVersionHigh;
    return FInfoOutputLargeInteger(liTemp, 10, OutputString);
}

/**
 Output the minor required OS version.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputOsVerMinor(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    LARGE_INTEGER liTemp;
    liTemp.HighPart = 0;
    liTemp.LowPart = Context->Entry.OsVersionLow;
    return FInfoOutputLargeInteger(liTemp, 10, OutputString);
}

/**
 Output the owner.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputOwner(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    YORI_ALLOC_SIZE_T OwnerLength = (YORI_ALLOC_SIZE_T)_tcslen(Context->Entry.Owner);
    if (OutputString->LengthAllocated >= OwnerLength) {
        memcpy(OutputString->StartOfString, Context->Entry.Owner, OwnerLength * sizeof(TCHAR));
        OutputString->LengthInChars = OwnerLength;
    }
    return OwnerLength;
}

/**
 Output the reparse tag.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputReparseTag(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    LARGE_INTEGER liTemp;
    liTemp.HighPart = 0;
    liTemp.LowPart = Context->Entry.ReparseTag;
    return FInfoOutputLargeInteger(liTemp, 10, OutputString);
}

/**
 Output the reparse tag in hex.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputReparseTagHex(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    LARGE_INTEGER liTemp;
    liTemp.HighPart = 0;
    liTemp.LowPart = Context->Entry.ReparseTag;
    return FInfoOutputLargeInteger(liTemp, 16, OutputString);
}

/**
 Output the short file name.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputShortName(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    YORI_ALLOC_SIZE_T NameLength = (YORI_ALLOC_SIZE_T)_tcslen(Context->Entry.ShortFileName);
    if (OutputString->LengthAllocated >= NameLength) {
        memcpy(OutputString->StartOfString, Context->Entry.FileName, NameLength * sizeof(TCHAR));
        OutputString->LengthInChars = NameLength;
    }
    return NameLength;
}

/**
 Output the file size variable.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputSize(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.FileSize, 10, OutputString);
}

/**
 Output the file size variable in hex.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputSizeHex(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.FileSize, 16, OutputString);
}

/**
 Output the stream count.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputStreamCount(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    LARGE_INTEGER liTemp;
    liTemp.HighPart = 0;
    liTemp.LowPart = Context->Entry.StreamCount;
    return FInfoOutputLargeInteger(liTemp, 10, OutputString);
}

/**
 Output the USN.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputUsn(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.Usn, 10, OutputString);
}

/**
 Output the USN in hex.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputUsnHex(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.Usn, 16, OutputString);
}

/**
 Output the write date year.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputWriteDateYear(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 4) {
        YoriLibYPrintf(OutputString, _T("%04i"), Context->Entry.WriteTime.wYear);
    }
    return 4;
}

/**
 Output the write date month.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputWriteDateMon(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.WriteTime.wMonth);
    }
    return 2;
}

/**
 Output the write date day.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputWriteDateDay(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.WriteTime.wDay);
    }
    return 2;
}

/**
 Output the write time hour.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputWriteTimeHour(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.WriteTime.wHour);
    }
    return 2;
}

/**
 Output the write time minute.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputWriteTimeMin(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.WriteTime.wMinute);
    }
    return 2;
}

/**
 Output the write time second.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
FInfoOutputWriteTimeSec(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    if (OutputString->LengthAllocated >= 2) {
        YoriLibYPrintf(OutputString, _T("%02i"), Context->Entry.WriteTime.wSecond);
    }
    return 2;
}


/**
 Information about a single variable that this program can substitute with
 values about a file.
 */
typedef struct _FINFO_KNOWN_VARIABLE {

    /**
     A NULL terminated string corresponding to the name of the variable.
     */
    TCHAR VariableName[24];

    /**
     Pointer to a function which can obtain the variable contents from
     the system.
     */
    PFINFO_COLLECT_FN CollectFn;

    /**
     Pointer to a function which can convert the variable contents into
     an output string.
     */
    PFINFO_OUTPUT_FN OutputFn;

    /**
     Help text string.
     */
    TCHAR Help[64];
} FINFO_KNOWN_VARIABLE, *PFINFO_KNOWN_VARIABLE;

/**
 An array of known file variables and the functions needed to obtain and
 output information for those variables.
 */
FINFO_KNOWN_VARIABLE FInfoKnownVariables[] = {
    {_T("ACCESSDATE_YEAR"),    YoriLibCollectAccessTime,            FInfoOutputAccessDateYear,
     _T("The year when the file was last read.")},

    {_T("ACCESSDATE_MON"),     YoriLibCollectAccessTime,            FInfoOutputAccessDateMon,
     _T("The month when the file was last read.")},

    {_T("ACCESSDATE_DAY"),     YoriLibCollectAccessTime,            FInfoOutputAccessDateDay,
     _T("The day when the file was last read.")},

    {_T("ACCESSTIME_HOUR"),    YoriLibCollectAccessTime,            FInfoOutputAccessTimeHour,
     _T("The hour when the file was last read.")},

    {_T("ACCESSTIME_MIN"),     YoriLibCollectAccessTime,            FInfoOutputAccessTimeMin,
     _T("The minute when the file was last read.")},

    {_T("ACCESSTIME_SEC"),     YoriLibCollectAccessTime,            FInfoOutputAccessTimeSec,
     _T("The second when the file was last read.")},

    {_T("ALLOCRANGECOUNT"),    YoriLibCollectAllocatedRangeCount,   FInfoOutputAllocatedRangeCount,
     _T("The number of allocated ranges in the file.")},

    {_T("ALLOCSIZE"),          YoriLibCollectAllocationSize,        FInfoOutputAllocationSize,
     _T("The number of allocated bytes in the file in decimal.")},

    {_T("ALLOCSIZE_HEX"),      YoriLibCollectAllocationSize,        FInfoOutputAllocationSizeHex,
     _T("The number of allocated bytes in the file in hex.")},

    {_T("CASESENSITIVE"),      YoriLibCollectCaseSensitivity,       FInfoOutputCaseSensitivity,
     _T("The case sensitivity status of the directory.")},

    {_T("COMPRESSEDSIZE"),     YoriLibCollectCompressedFileSize,    FInfoOutputCompressedSize,
     _T("The number of compressed bytes in the file in decimal.")},

    {_T("COMPRESSEDSIZE_HEX"), YoriLibCollectCompressedFileSize,    FInfoOutputCompressedSizeHex,
     _T("The number of compressed bytes in the file in hex.")},

    {_T("CREATEDATE_YEAR"),    YoriLibCollectCreateTime,            FInfoOutputCreateDateYear,
     _T("The year when the file was created.")},

    {_T("CREATEDATE_MON"),     YoriLibCollectCreateTime,            FInfoOutputCreateDateMon,
     _T("The month when the file was created.")},

    {_T("CREATEDATE_DAY"),     YoriLibCollectCreateTime,            FInfoOutputCreateDateDay,
     _T("The day when the file was created.")},

    {_T("CREATETIME_HOUR"),    YoriLibCollectCreateTime,            FInfoOutputCreateTimeHour,
     _T("The hour when the file was created.")},

    {_T("CREATETIME_MIN"),     YoriLibCollectCreateTime,            FInfoOutputCreateTimeMin,
     _T("The minute when the file was created.")},

    {_T("CREATETIME_SEC"),     YoriLibCollectCreateTime,            FInfoOutputCreateTimeSec,
     _T("The second when the file was created.")},

    {_T("DESCRIPTION"),        YoriLibCollectDescription,           FInfoOutputDescription,
     _T("The executable's description.")},

    {_T("EXT"),                YoriLibCollectFileName,              FInfoOutputExt,
     _T("The file extension.")},

    {_T("FILEID"),             YoriLibCollectFileId,                FInfoOutputFileId,
     _T("The file ID in decimal.")},

    {_T("FILEID_HEX"),         YoriLibCollectFileId,                FInfoOutputFileIdHex,
     _T("The file ID in hex.")},

    {_T("FILEVERSTRING"),      YoriLibCollectFileVersionString,     FInfoOutputFileVersionString,
     _T("The file version string.")},

    {_T("FRAGMENTCOUNT"),      YoriLibCollectFragmentCount,         FInfoOutputFragmentCount,
     _T("The number of extents in the file.")},

    {_T("LINKCOUNT"),          YoriLibCollectLinkCount,             FInfoOutputLinkCount,
     _T("The number of hardlinks on the file.")},

    {_T("NAME"),               YoriLibCollectFileName,              FInfoOutputName,
     _T("The file name.")},

    {_T("OSVERMAJOR"),         YoriLibCollectOsVersion,             FInfoOutputOsVerMajor,
     _T("The minimum major OS version required by the program.")},

    {_T("OSVERMINOR"),         YoriLibCollectOsVersion,             FInfoOutputOsVerMinor,
     _T("The minimum minor OS version required by the program.")},

    {_T("OWNER"),              YoriLibCollectOwner,                 FInfoOutputOwner,
     _T("The owner of the file.")},

    {_T("REPARSETAG"),         YoriLibCollectReparseTag,            FInfoOutputReparseTag,
     _T("The reparse tag in decimal.")},

    {_T("REPARSETAG_HEX"),     YoriLibCollectReparseTag,            FInfoOutputReparseTagHex,
     _T("The reparse tag in hex.")},

    {_T("SIZE"),               YoriLibCollectFileSize,              FInfoOutputSize,
     _T("The file size in bytes.")},

    {_T("SHORTNAME"),          YoriLibCollectShortName,             FInfoOutputShortName,
     _T("The short file name.")},

    {_T("STREAMCOUNT"),        YoriLibCollectStreamCount,           FInfoOutputStreamCount,
     _T("The number of named streams on the file.")},

    {_T("USN"),                YoriLibCollectUsn,                   FInfoOutputUsn,
     _T("The USN on the file in decimal.")},

    {_T("USN_HEX"),            YoriLibCollectUsn,                   FInfoOutputUsnHex,
     _T("The USN on the file in hex.")},

    {_T("WRITEDATE_YEAR"),     YoriLibCollectWriteTime,             FInfoOutputWriteDateYear,
     _T("The year when the file was last written to.")},

    {_T("WRITEDATE_MON"),      YoriLibCollectWriteTime,             FInfoOutputWriteDateMon,
     _T("The month when the file was last written to.")},

    {_T("WRITEDATE_DAY"),      YoriLibCollectWriteTime,             FInfoOutputWriteDateDay,
     _T("The day when the file was last written to.")},

    {_T("WRITETIME_HOUR"),     YoriLibCollectWriteTime,             FInfoOutputWriteTimeHour,
     _T("The hour when the file was last written to.")},

    {_T("WRITETIME_MIN"),      YoriLibCollectWriteTime,             FInfoOutputWriteTimeMin,
     _T("The minute when the file was last written to.")},

    {_T("WRITETIME_SEC"),      YoriLibCollectWriteTime,             FInfoOutputWriteTimeSec,
     _T("The second when the file was last written to.")}
};

/**
 Display usage text to the user.
 */
BOOL
FInfoHelp(VOID)
{
    YORI_ALLOC_SIZE_T Count;
    TCHAR NameWithQualifiers[32];

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("FInfo %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strFInfoHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Format specifiers are:\n\n"));
    for (Count = 0; Count < sizeof(FInfoKnownVariables)/sizeof(FInfoKnownVariables[0]); Count++) {
        YoriLibSPrintfS(NameWithQualifiers, sizeof(NameWithQualifiers)/sizeof(NameWithQualifiers[0]), _T("$%s$"), FInfoKnownVariables[Count].VariableName);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-20s %s\n"), NameWithQualifiers, FInfoKnownVariables[Count].Help);
    }
    return TRUE;
}

/**
 Expand any variables in the format string of information to display for each
 file.

 @param OutputString The buffer to populate with the result of variable
        expansion.

 @param VariableName The name of the variable to populate data from.

 @param Context Pointer to a FINFO_CONTEXT structure containing state about
        the file.

 @return The number of characters populated or number of characters required
         to successfully populate the variable contents.
 */
YORI_ALLOC_SIZE_T
FInfoExpandVariables(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    YORI_ALLOC_SIZE_T Index;
    YORI_ALLOC_SIZE_T CharsNeeded = 0;
    PFINFO_CONTEXT FInfoContext = (PFINFO_CONTEXT)Context;

    for (Index = 0; Index < sizeof(FInfoKnownVariables)/sizeof(FInfoKnownVariables[0]); Index++) {
        if (YoriLibCompareStringWithLiteral(VariableName, FInfoKnownVariables[Index].VariableName) == 0) {
            FInfoKnownVariables[Index].CollectFn(&FInfoContext->Entry, FInfoContext->FileInfo, FInfoContext->FilePath);
            CharsNeeded = FInfoKnownVariables[Index].OutputFn(FInfoContext, OutputString);
        }
    }

    return CharsNeeded;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param Context Pointer to the finfo context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
FInfoFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING DisplayString;
    WIN32_FIND_DATA LocalFileInfo;
    PWIN32_FIND_DATA FileInfoToUse;
    PFINFO_CONTEXT FInfoContext;

    UNREFERENCED_PARAMETER(Depth);
    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    FInfoContext = (PFINFO_CONTEXT)Context;

    if (FileInfo != NULL) {
        FileInfoToUse = FileInfo;
    } else {
        ZeroMemory(&LocalFileInfo, sizeof(LocalFileInfo));
        YoriLibUpdateFindDataFromFileInformation(&LocalFileInfo, FilePath->StartOfString, TRUE);
        FileInfoToUse = &LocalFileInfo;
    }

    FInfoContext->FilePath = FilePath;
    FInfoContext->FileInfo = FileInfoToUse;
    FInfoContext->FilesFound++;
    FInfoContext->FilesFoundThisArg++;

    YoriLibInitEmptyString(&DisplayString);
    YoriLibExpandCommandVariables(&FInfoContext->FormatString, '$', TRUE, FInfoExpandVariables, FInfoContext, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        if (FInfoContext->FilesFound > 1) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n%y"), &DisplayString);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        }
        YoriLibFreeStringContents(&DisplayString);
    }

    return TRUE;
}

/**
 A default format string to use if the user didn't specify one.
 */
TCHAR FInfoDefaultFormatString[] = 
    _T("NAME:             $NAME$\n")
    _T("ACCESSDATE:       $ACCESSDATE_YEAR$/$ACCESSDATE_MON$/$ACCESSDATE_DAY$\n")
    _T("ACCESSTIME:       $ACCESSTIME_HOUR$:$ACCESSTIME_MIN$:$ACCESSTIME_SEC$\n")
    _T("CREATEDATE:       $CREATEDATE_YEAR$/$CREATEDATE_MON$/$CREATEDATE_DAY$\n")
    _T("CREATETIME:       $CREATETIME_HOUR$:$CREATETIME_MIN$:$CREATETIME_SEC$\n")
    _T("ALLOCRANGECOUNT:  $ALLOCRANGECOUNT$\n")
    _T("ALLOCSIZE:        $ALLOCSIZE$\n")
    _T("CASESENSITIVE:    $CASESENSITIVE$\n")
    _T("COMPRESSEDSIZE:   $COMPRESSEDSIZE$\n")
    _T("EXT:              $EXT$\n")
    _T("FILEID_HEX:       $FILEID_HEX$\n")
    _T("FRAGMENTCOUNT:    $FRAGMENTCOUNT$\n")
    _T("LINKCOUNT:        $LINKCOUNT$\n")
    _T("OSVERMAJOR:       $OSVERMAJOR$\n")
    _T("OSVERMINOR:       $OSVERMINOR$\n")
    _T("OWNER:            $OWNER$\n")
    _T("REPARSETAG_HEX:   $REPARSETAG_HEX$\n")
    _T("SIZE:             $SIZE$\n")
    _T("SHORTNAME:        $SHORTNAME$\n")
    _T("STREAMCOUNT:      $STREAMCOUNT$\n")
    _T("WRITEDATE:        $WRITEDATE_YEAR$/$WRITEDATE_MON$/$WRITEDATE_DAY$\n")
    _T("WRITETIME:        $WRITETIME_HOUR$:$WRITETIME_MIN$:$WRITETIME_SEC$\n");

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the finfo builtin command.
 */
#define ENTRYPOINT YoriCmd_FINFO
#else
/**
 The main entrypoint for the finfo standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the finfo cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero on success, nonzero on failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    WORD MatchFlags;
    BOOLEAN Recursive = FALSE;
    BOOLEAN BasicEnumeration = FALSE;
    BOOLEAN ReturnDirectories = FALSE;
    FINFO_CONTEXT FInfoContext;
    YORI_STRING Arg;

    ZeroMemory(&FInfoContext, sizeof(FInfoContext));
    YoriLibConstantString(&FInfoContext.FormatString, FInfoDefaultFormatString);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                FInfoHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                ReturnDirectories = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (i + 1 < ArgC) {
                    ArgumentUnderstood = TRUE;
                    i++;
                    memcpy(&FInfoContext.FormatString, &ArgV[i], sizeof(YORI_STRING));
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartArg = i + 1;
                break;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0 || StartArg == ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("finfo: missing argument\n"));
        return EXIT_FAILURE;
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES;

        if (ReturnDirectories) {
            MatchFlags |= YORILIB_FILEENUM_RETURN_DIRECTORIES;
        } else {
            MatchFlags |= YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        }

        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }

        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }

        for (i = StartArg; i < ArgC; i++) {

            FInfoContext.FilesFoundThisArg = 0;
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, FInfoFileFoundCallback, NULL, &FInfoContext);
            if (FInfoContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    FInfoFileFoundCallback(&FullPath, NULL, 0, &FInfoContext);
                    YoriLibFreeStringContents(&FullPath);
                }
            }
        }
    }

    if (FInfoContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("finfo: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
