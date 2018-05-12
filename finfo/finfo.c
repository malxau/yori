/**
 * @file finfo/finfo.c
 *
 * Yori shell display file metadata
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
CHAR strHelpText[] =
        "\n"
        "Output information about file metadata.\n"
        "\n"
        "FINFO [-b] [-f fmt] [-s] <file>...\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -f             Specify a custom format string\n"
        "   -s             Process files from all subdirectories\n";

/**
 Display usage text to the user.
 */
BOOL
FInfoHelp()
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2018"), &License);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("FInfo %i.%i\n"), FINFO_VER_MAJOR, FINFO_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &License);
    YoriLibFreeStringContents(&License);
    return TRUE;
}


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
typedef DWORD (* PFINFO_OUTPUT_FN)(PFINFO_CONTEXT, PYORI_STRING);

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
DWORD
FInfoOutputLargeInteger(
    __in LARGE_INTEGER LargeInt,
    __in DWORD NumberBase,
    __inout PYORI_STRING OutputString
    )
{
    YORI_STRING String;
    TCHAR StringBuffer[32];

    YoriLibInitEmptyString(&String);
    String.StartOfString = StringBuffer;
    String.LengthAllocated = sizeof(StringBuffer)/sizeof(StringBuffer[0]);

    YoriLibNumberToString(&String, LargeInt.QuadPart, NumberBase, 0, ' ');

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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
FInfoOutputAllocationSizeHex(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.AllocationSize, 16, OutputString);
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
 Output the file extension.

 @param Context Pointer to context about the file, including previously
        obtained information to satisfy the output request.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
DWORD
FInfoOutputExt(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    DWORD ExtLen = 0;
    
    if (Context->Entry.Extension != NULL) {
        ExtLen = _tcslen(Context->Entry.Extension);

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
DWORD
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
DWORD
FInfoOutputFileIdHex(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    return FInfoOutputLargeInteger(Context->Entry.FileId, 16, OutputString);
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
FInfoOutputOwner(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    DWORD OwnerLength = _tcslen(Context->Entry.Owner);
    if (OutputString->LengthAllocated >= OwnerLength) {
        memcpy(OutputString->StartOfString, Context->Entry.FileName, OwnerLength * sizeof(TCHAR));
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
DWORD
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
DWORD
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
DWORD
FInfoOutputShortName(
    __in PFINFO_CONTEXT Context,
    __inout PYORI_STRING OutputString
    )
{
    DWORD NameLength = _tcslen(Context->Entry.ShortFileName);
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
DWORD
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
    LPTSTR VariableName;

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
} FINFO_KNOWN_VARIABLE, *PFINFO_KNOWN_VARIABLE;

/**
 An array of known file variables and the functions needed to obtain and
 output information for those variables.
 */
FINFO_KNOWN_VARIABLE FInfoKnownVariables[] = {
    {_T("ACCESSDATE_YEAR"),    YoriLibCollectAccessTime,            FInfoOutputAccessDateYear},
    {_T("ACCESSDATE_MON"),     YoriLibCollectAccessTime,            FInfoOutputAccessDateMon},
    {_T("ACCESSDATE_DAY"),     YoriLibCollectAccessTime,            FInfoOutputAccessDateDay},
    {_T("ACCESSTIME_HOUR"),    YoriLibCollectAccessTime,            FInfoOutputAccessTimeHour},
    {_T("ACCESSTIME_MIN"),     YoriLibCollectAccessTime,            FInfoOutputAccessTimeMin},
    {_T("ACCESSTIME_SEC"),     YoriLibCollectAccessTime,            FInfoOutputAccessTimeSec},
    {_T("ALLOCRANGECOUNT"),    YoriLibCollectAllocatedRangeCount,   FInfoOutputAllocatedRangeCount},
    {_T("ALLOCSIZE"),          YoriLibCollectAllocationSize,        FInfoOutputAllocationSize},
    {_T("ALLOCSIZE_HEX"),      YoriLibCollectAllocationSize,        FInfoOutputAllocationSizeHex},
    {_T("COMPRESSEDSIZE"),     YoriLibCollectCompressedFileSize,    FInfoOutputCompressedSize},
    {_T("COMPRESSEDSIZE_HEX"), YoriLibCollectCompressedFileSize,    FInfoOutputCompressedSizeHex},
    {_T("CREATEDATE_YEAR"),    YoriLibCollectCreateTime,            FInfoOutputCreateDateYear},
    {_T("CREATEDATE_MON"),     YoriLibCollectCreateTime,            FInfoOutputCreateDateMon},
    {_T("CREATEDATE_DAY"),     YoriLibCollectCreateTime,            FInfoOutputCreateDateDay},
    {_T("CREATETIME_HOUR"),    YoriLibCollectCreateTime,            FInfoOutputCreateTimeHour},
    {_T("CREATETIME_MIN"),     YoriLibCollectCreateTime,            FInfoOutputCreateTimeMin},
    {_T("CREATETIME_SEC"),     YoriLibCollectCreateTime,            FInfoOutputCreateTimeSec},
    {_T("EXT"),                YoriLibCollectFileName,              FInfoOutputExt},
    {_T("FILEID"),             YoriLibCollectFileId,                FInfoOutputFileId},
    {_T("FILEID_HEX"),         YoriLibCollectFileId,                FInfoOutputFileIdHex},
    {_T("FRAGMENTCOUNT"),      YoriLibCollectFragmentCount,         FInfoOutputFragmentCount},
    {_T("LINKCOUNT"),          YoriLibCollectLinkCount,             FInfoOutputLinkCount},
    {_T("NAME"),               YoriLibCollectFileName,              FInfoOutputName},
    {_T("OSVERMAJOR"),         YoriLibCollectOsVersion,             FInfoOutputOsVerMajor},
    {_T("OSVERMINOR"),         YoriLibCollectOsVersion,             FInfoOutputOsVerMinor},
    {_T("OWNER"),              YoriLibCollectOwner,                 FInfoOutputOwner},
    {_T("REPARSETAG"),         YoriLibCollectReparseTag,            FInfoOutputReparseTag},
    {_T("REPARSETAG_HEX"),     YoriLibCollectReparseTag,            FInfoOutputReparseTagHex},
    {_T("SIZE"),               YoriLibCollectFileSize,              FInfoOutputSize},
    {_T("SHORTNAME"),          YoriLibCollectShortName,             FInfoOutputShortName},
    {_T("STREAMCOUNT"),        YoriLibCollectStreamCount,           FInfoOutputStreamCount},
    {_T("USN"),                YoriLibCollectUsn,                   FInfoOutputUsn},
    {_T("USN_HEX"),            YoriLibCollectUsn,                   FInfoOutputUsnHex},
    {_T("WRITEDATE_YEAR"),     YoriLibCollectWriteTime,             FInfoOutputWriteDateYear},
    {_T("WRITEDATE_MON"),      YoriLibCollectWriteTime,             FInfoOutputWriteDateMon},
    {_T("WRITEDATE_DAY"),      YoriLibCollectWriteTime,             FInfoOutputWriteDateDay},
    {_T("WRITETIME_HOUR"),     YoriLibCollectWriteTime,             FInfoOutputWriteTimeHour},
    {_T("WRITETIME_MIN"),      YoriLibCollectWriteTime,             FInfoOutputWriteTimeMin},
    {_T("WRITETIME_SEC"),      YoriLibCollectWriteTime,             FInfoOutputWriteTimeSec}
};

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
DWORD
FInfoExpandVariables(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PFINFO_CONTEXT Context
    )
{
    DWORD Index;
    DWORD CharsNeeded = 0;

    for (Index = 0; Index < sizeof(FInfoKnownVariables)/sizeof(FInfoKnownVariables[0]); Index++) {
        if (YoriLibCompareStringWithLiteral(VariableName, FInfoKnownVariables[Index].VariableName) == 0) {
            FInfoKnownVariables[Index].CollectFn(&Context->Entry, Context->FileInfo, Context->FilePath);
            CharsNeeded = FInfoKnownVariables[Index].OutputFn(Context, OutputString);
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

 @param FInfoContext Pointer to the finfo context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
FInfoFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PFINFO_CONTEXT FInfoContext
    )
{
    YORI_STRING DisplayString;

    UNREFERENCED_PARAMETER(Depth);
    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    FInfoContext->FilePath = FilePath;
    FInfoContext->FileInfo = FileInfo;
    FInfoContext->FilesFound++;

    YoriLibInitEmptyString(&DisplayString);
    YoriLibExpandCommandVariables(&FInfoContext->FormatString, '$', TRUE, FInfoExpandVariables, FInfoContext, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        YoriLibFreeStringContents(&DisplayString);
    }

    return TRUE;
}

/**
 A default format string to use if the user didn't specify one.
 */
TCHAR DefaultFormatString[] = 
    _T("NAME:             $NAME$\n")
    _T("ACCESSDATE:       $ACCESSDATE_YEAR$/$ACCESSDATE_MON$/$ACCESSDATE_DAY$\n")
    _T("ACCESSTIME:       $ACCESSTIME_HOUR$:$ACCESSTIME_MIN$:$ACCESSTIME_SEC$\n")
    _T("CREATEDATE:       $CREATEDATE_YEAR$/$CREATEDATE_MON$/$CREATEDATE_DAY$\n")
    _T("CREATETIME:       $CREATETIME_HOUR$:$CREATETIME_MIN$:$CREATETIME_SEC$\n")
    _T("ALLOCRANGECOUNT:  $ALLOCRANGECOUNT$\n")
    _T("ALLOCSIZE:        $ALLOCSIZE$\n")
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


/**
 The main entrypoint for the finfo cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero on success, nonzero on failure.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    DWORD MatchFlags;
    BOOL Recursive = FALSE;
    BOOL BasicEnumeration = FALSE;
    FINFO_CONTEXT FInfoContext;
    YORI_STRING Arg;

    ZeroMemory(&FInfoContext, sizeof(FInfoContext));
    YoriLibConstantString(&FInfoContext.FormatString, DefaultFormatString);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                FInfoHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
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

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("finfo: missing argument\n"));
        return EXIT_FAILURE;
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }
    
        for (i = StartArg; i < ArgC; i++) {
    
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, FInfoFileFoundCallback, &FInfoContext);
        }
    }

    if (FInfoContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("finfo: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
