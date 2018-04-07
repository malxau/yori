/**
 * @file sdir/callbacks.c
 *
 * Colorful, sorted and optionally rich directory enumeration
 * for Windows.
 *
 * This module implements functions to collect, display, sort, and deserialize
 * individual data types associated with files that we can enumerate.
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

/**
 A table that maps file attribute flags as returned by the system to character
 representations used in UI or specified by the user.
 */
const SDIR_ATTRPAIR
AttrPairs[] = {
    {FILE_ATTRIBUTE_ARCHIVE,           'A'},
    {FILE_ATTRIBUTE_READONLY,          'R'},
    {FILE_ATTRIBUTE_HIDDEN,            'H'},
    {FILE_ATTRIBUTE_SYSTEM,            'S'},
    {FILE_ATTRIBUTE_DIRECTORY,         'D'},
    {FILE_ATTRIBUTE_COMPRESSED,        'C'},
    {FILE_ATTRIBUTE_ENCRYPTED,         'E'},
    {FILE_ATTRIBUTE_OFFLINE,           'O'},
    {FILE_ATTRIBUTE_REPARSE_POINT,     'r'},
    {FILE_ATTRIBUTE_SPARSE_FILE,       's'},
    {FILE_ATTRIBUTE_INTEGRITY_STREAM,  'I'},
    };

/**
 Return the count of attribute pairs, meaning characters to flags describing
 attributes.
 This exists so other modules can know about how many attributes there are
 without having access to know what all of the attributes are.
 */
DWORD
SdirGetNumAttrPairs()
{
    return sizeof(AttrPairs)/sizeof(AttrPairs[0]);
}

/**
 A table that maps file permission flags as returned by the system to
 character representations used in UI or specified by the user.
 */
const SDIR_ATTRPAIR PermissionPairs[] = {
    {FILE_READ_DATA,                   'R'},
    {FILE_READ_ATTRIBUTES,             'r'},
    {FILE_WRITE_DATA,                  'W'},
    {FILE_WRITE_ATTRIBUTES,            'w'},
    {FILE_APPEND_DATA,                 'A'},
    {FILE_EXECUTE,                     'X'},
    {DELETE,                           'D'},
    };

/**
 Return the count of permission pairs, meaning characters to flags describing
 permissions.
 This exists so other modules can know about how many permissions there are
 without having access to know what all of the permissions are.
 */
DWORD
SdirGetNumPermissionPairs()
{
    return sizeof(PermissionPairs)/sizeof(PermissionPairs[0]);
}


//
//  Sorting support
//

/**
 Compare two directory entry access dates.

 @param Left The first date to compare.

 @param Right The second date to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareAccessDate (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareDate(&Left->AccessTime, &Right->AccessTime);
}

/**
 Compare two directory entry access times.

 @param Left The first time to compare.

 @param Right The second time to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareAccessTime (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareTime(&Left->AccessTime, &Right->AccessTime);
}

/**
 Compare two directory entry allocated range counts.

 @param Left The first count to compare.

 @param Right The second count to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareAllocatedRangeCount (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareLargeInt((PULARGE_INTEGER)&Left->AllocatedRangeCount, (PULARGE_INTEGER)&Right->AllocatedRangeCount);
}

/**
 Compare two directory entry allocation sizes.

 @param Left The first size to compare.

 @param Right The second size to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareAllocationSize (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareLargeInt((PULARGE_INTEGER)&Left->AllocationSize, (PULARGE_INTEGER)&Right->AllocationSize);
}

/**
 Compare two directory entry OS architectures.

 @param Left The first architecture to compare.

 @param Right The second architecture to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareArch (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if (Left->Architecture < Right->Architecture) {
        return SDIR_LESS_THAN;
    } else if (Left->Architecture > Right->Architecture) {
        return SDIR_GREATER_THAN;
    }
    return SDIR_EQUAL;
}

/**
 Compare two directory entry compression algorithms.

 @param Left The first algorithm to compare.

 @param Right The second algorithm to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareCompressionAlgorithm (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if (Left->CompressionAlgorithm < Right->CompressionAlgorithm) {
        return SDIR_LESS_THAN;
    } else if (Left->CompressionAlgorithm > Right->CompressionAlgorithm) {
        return SDIR_GREATER_THAN;
    }
    return SDIR_EQUAL;
}

/**
 Compare two directory entry compressed file sizes.

 @param Left The first size to compare.

 @param Right The second size to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareCompressedFileSize (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareLargeInt((PULARGE_INTEGER)&Left->CompressedFileSize, (PULARGE_INTEGER)&Right->CompressedFileSize);
}

/**
 Compare two directory entry create dates.

 @param Left The first date to compare.

 @param Right The second date to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareCreateDate (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareDate(&Left->CreateTime, &Right->CreateTime);
}

/**
 Compare two directory entry create times.

 @param Left The first time to compare.

 @param Right The second time to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareCreateTime (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareTime(&Left->CreateTime, &Right->CreateTime);
}

/**
 Compare two directory entry effective permissions.

 @param Left The first set of permissions to compare.

 @param Right The second set of permissions to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareEffectivePermissions (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if (Left->EffectivePermissions < Right->EffectivePermissions) {
        return SDIR_LESS_THAN;
    } else if (Left->EffectivePermissions > Right->EffectivePermissions) {
        return SDIR_GREATER_THAN;
    }
    return SDIR_EQUAL;
}

/**
 Compare two directory entry file attributes.

 @param Left The first set of attributes to compare.

 @param Right The second set of attributes to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareFileAttributes (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if (Left->FileAttributes < Right->FileAttributes) {
        return SDIR_LESS_THAN;
    } else if (Left->FileAttributes > Right->FileAttributes) {
        return SDIR_GREATER_THAN;
    }
    return SDIR_EQUAL;
}

/**
 Compare two directory entry file extensions.

 @param Left The first extension to compare.

 @param Right The second extension to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareFileExtension (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareString(Left->Extension, Right->Extension);
}

/**
 Compare two directory entry file identifiers.

 @param Left The first ID to compare.

 @param Right The second ID to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareFileId (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareLargeInt((PULARGE_INTEGER)&Left->FileId, (PULARGE_INTEGER)&Right->FileId);
}

/**
 Compare two directory entry file names.

 @param Left The first name to compare.

 @param Right The second name to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareFileName (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareString(Left->FileName, Right->FileName);
}

/**
 Compare two directory entry file sizes.

 @param Left The first size to compare.

 @param Right The second size to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareFileSize (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareLargeInt((PULARGE_INTEGER)&Left->FileSize, (PULARGE_INTEGER)&Right->FileSize);
}

/**
 Compare two directory entry fragment counts.

 @param Left The first count to compare.

 @param Right The second count to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareFragmentCount (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareLargeInt((PULARGE_INTEGER)&Left->FragmentCount, (PULARGE_INTEGER)&Right->FragmentCount);
}

/**
 Compare two directory entry link counts.

 @param Left The first count to compare.

 @param Right The second count to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareLinkCount (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if (Left->LinkCount < Right->LinkCount) {
        return SDIR_LESS_THAN;
    } else if (Left->LinkCount > Right->LinkCount) {
        return SDIR_GREATER_THAN;
    }
    return SDIR_EQUAL;
}

/**
 Compare two directory entry object IDs.

 @param Left The first ID to compare.

 @param Right The second ID to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareObjectId (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    int result = memcmp(&Left->ObjectId, &Right->ObjectId, sizeof(Left->ObjectId));
    
    if (result < 0) {
        return SDIR_LESS_THAN;
    } else if (result > 0) {
        return SDIR_GREATER_THAN;
    }
    return SDIR_EQUAL;
}

/**
 Compare two directory entry minimum OS versions.

 @param Left The first version to compare.

 @param Right The second version to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareOsVersion (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if (Left->OsVersionHigh < Right->OsVersionHigh) {
        return SDIR_LESS_THAN;
    } else if (Left->OsVersionHigh > Right->OsVersionHigh) {
        return SDIR_GREATER_THAN;
    }

    if (Left->OsVersionLow < Right->OsVersionLow) {
        return SDIR_LESS_THAN;
    } else if (Left->OsVersionLow > Right->OsVersionLow) {
        return SDIR_GREATER_THAN;
    }

    return SDIR_EQUAL;
}

/**
 Compare two directory owners.

 @param Left The first owner to compare.

 @param Right The second owner to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareOwner (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareString(Left->Owner, Right->Owner);
}

/**
 Compare two directory entry reparse tags.

 @param Left The first tag to compare.

 @param Right The second tag to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareReparseTag (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if (Left->ReparseTag < Right->ReparseTag) {
        return SDIR_LESS_THAN;
    } else if (Left->ReparseTag > Right->ReparseTag) {
        return SDIR_GREATER_THAN;
    }
    return SDIR_EQUAL;
}

/**
 Compare two directory entry short file names.

 @param Left The first name to compare.

 @param Right The second name to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareShortName (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareString(Left->ShortFileName, Right->ShortFileName);
}

/**
 Compare two directory entry OS subsystems.

 @param Left The first subsystem to compare.

 @param Right The second subsystem to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareSubsystem (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if (Left->Subsystem < Right->Subsystem) {
        return SDIR_LESS_THAN;
    } else if (Left->Subsystem > Right->Subsystem) {
        return SDIR_GREATER_THAN;
    }
    return SDIR_EQUAL;
}

/**
 Compare two directory entry stream counts.

 @param Left The first count to compare.

 @param Right The second count to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareStreamCount (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if (Left->StreamCount < Right->StreamCount) {
        return SDIR_LESS_THAN;
    } else if (Left->StreamCount > Right->StreamCount) {
        return SDIR_GREATER_THAN;
    }
    return SDIR_EQUAL;
}

/**
 Compare two directory USN values.

 @param Left The first USN to compare.

 @param Right The second USN to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareUsn (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareLargeInt((PULARGE_INTEGER)&Left->Usn, (PULARGE_INTEGER)&Right->Usn);
}

/**
 Compare two directory entry version resources.

 @param Left The first version to compare.

 @param Right The second version to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareVersion (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareLargeInt((PULARGE_INTEGER)&Left->FileVersion, (PULARGE_INTEGER)&Right->FileVersion);
}

/**
 Compare two directory entry write dates.

 @param Left The first date to compare.

 @param Right The second date to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareWriteDate (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareDate(&Left->WriteTime, &Right->WriteTime);
}

/**
 Compare two directory entry write times.

 @param Left The first time to compare.

 @param Right The second time to compare.

 @return SDIR_LESS_THAN if the first is less than the second,
         SDIR_GREATER_THAN if the first is greater than the second,
         SDIR_EQUAL if the two are the same.
 */
DWORD
SdirCompareWriteTime (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    return SdirCompareTime(&Left->WriteTime, &Right->WriteTime);
}

/**
 Compare two directory entry effective permissions to see if all bits
 in the second are in the first.

 @param Left The first set of permissions to compare.

 @param Right The second set of permissions to compare.

 @return SDIR_NOT_EQUAL if not all bits from the first are set in the second,
         SDIR_EQUAL if all bits are set in the second.
 */
DWORD
SdirBitwiseEffectivePermissions (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if ((Left->EffectivePermissions & Right->EffectivePermissions) == Right->EffectivePermissions) {
        return SDIR_EQUAL;
    }
    return SDIR_NOT_EQUAL;
}

/**
 Compare two directory entry file attributes to see if all bits
 in the second are in the first.

 @param Left The first set of attributes to compare.

 @param Right The second set of attributes to compare.

 @return SDIR_NOT_EQUAL if not all bits from the first are set in the second,
         SDIR_EQUAL if all bits are set in the second.
 */
DWORD
SdirBitwiseFileAttributes (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    if ((Left->FileAttributes & Right->FileAttributes) == Right->FileAttributes) {
        return SDIR_EQUAL;
    }
    return SDIR_NOT_EQUAL;
}

/**
 Upcase a single character within a string, referenced by offset.

 @param Str Pointer to the string.

 @param Index The offset within the string of the character.

 @return The upcased form of the character.
 */
TCHAR
SdirGetUpcasedCharFromString (
    __in LPTSTR Str,
    __in DWORD Index
    )
{
    return YoriLibUpcaseChar(Str[Index]);
}

/**
 Compare two directory entry file names to see if the first matches the
 wildcard criteria in the second.

 @param Left The first file name to compare.

 @param Right The second file name to compare.

 @return SDIR_NOT_EQUAL if not specified characters in the first match the
         second,
         SDIR_EQUAL if the first name matches the wildcard pattern in the
         second.
 */
DWORD
SdirBitwiseFileName (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    )
{
    DWORD LeftIndex, RightIndex;

    TCHAR CompareLeft;
    TCHAR CompareRight;

    LeftIndex = 0;
    RightIndex = 0;

    while (Left->FileName[LeftIndex] != '\0' && Right->FileName[RightIndex] != '\0') {

        CompareLeft = SdirGetUpcasedCharFromString(Left->FileName, LeftIndex);
        CompareRight = SdirGetUpcasedCharFromString(Right->FileName, RightIndex);

        LeftIndex++;
        RightIndex++;

        if (CompareRight == '?') {

            //
            //  '?' matches with everything.  We've already advanced to the next
            //  char, so continue.
            //

        } else if (CompareRight == '*') {

            //
            //  Skip one char so Right is the one after *.  Left should compare
            //  the character it's currently on.  Keep going through Left until
            //  we find the char in Right
            //

            LeftIndex--;
            CompareRight = SdirGetUpcasedCharFromString(Right->FileName, RightIndex);
            CompareLeft = SdirGetUpcasedCharFromString(Left->FileName, LeftIndex);

            while (CompareLeft != CompareRight && CompareLeft != '\0') {
                LeftIndex++;
                CompareLeft = SdirGetUpcasedCharFromString(Left->FileName, LeftIndex);
            }

        } else {
            if (CompareLeft != CompareRight) {
                return SDIR_NOT_EQUAL;
            }
        }

    }

    if (Left->FileName[LeftIndex] == '\0' && Right->FileName[RightIndex] == '\0') {
        return SDIR_EQUAL;
    }

    return SDIR_NOT_EQUAL;
}


//
//  File enumeration support
//

/**
 Collect information from a directory enumerate and full file name relating
 to the file's access time.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectAccessTime (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    FILETIME tmp;

    UNREFERENCED_PARAMETER(FullPath);

    FileTimeToLocalFileTime(&FindData->ftLastAccessTime, &tmp);
    FileTimeToSystemTime(&tmp, &Entry->AccessTime);
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's allocated range count.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectAllocatedRangeCount (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    HANDLE hFile;

    Entry->AllocatedRangeCount.HighPart = 0;
    Entry->AllocatedRangeCount.LowPart = 0;

    hFile = CreateFile(FullPath,
                       FILE_READ_ATTRIBUTES|FILE_READ_DATA,
                       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {

        FILE_ALLOCATED_RANGE_BUFFER StartBuffer;
        union {
            FILE_ALLOCATED_RANGE_BUFFER Extents[1];
            UCHAR Buffer[2048];
        } u;
        DWORD BytesReturned;
        DWORD ElementCount;

        LARGE_INTEGER PriorRunLength = {0};
        LARGE_INTEGER PriorRunOffset = {0};

        StartBuffer.FileOffset.QuadPart = 0;
        StartBuffer.Length.LowPart = FindData->nFileSizeLow;
        StartBuffer.Length.HighPart = FindData->nFileSizeHigh;

        while ((DeviceIoControl(hFile, FSCTL_QUERY_ALLOCATED_RANGES, &StartBuffer, sizeof(StartBuffer), &u.Extents, sizeof(u), &BytesReturned, NULL) || GetLastError() == ERROR_MORE_DATA) &&
               BytesReturned > 0) {

            ElementCount = BytesReturned / sizeof(FILE_ALLOCATED_RANGE_BUFFER);

            // 
            //  Look through the extents.  If it's not a sparse hole, record it as a 
            //  fragment.  If it's also discontiguous with the previous run, count it as a
            //  fragment.
            //

            for (BytesReturned = 0; BytesReturned < ElementCount; BytesReturned++) {

                if (u.Extents[BytesReturned].FileOffset.QuadPart == 0 ||
                    PriorRunOffset.QuadPart + PriorRunLength.QuadPart != u.Extents[BytesReturned].FileOffset.QuadPart) {

                    if (Entry->AllocatedRangeCount.LowPart == (DWORD)-1) {
                        Entry->AllocatedRangeCount.HighPart++;
                    }
                    Entry->AllocatedRangeCount.LowPart++;
                }

                PriorRunLength.QuadPart = u.Extents[BytesReturned].Length.QuadPart;
                PriorRunOffset.QuadPart = u.Extents[BytesReturned].FileOffset.QuadPart;
            }

            StartBuffer.FileOffset.QuadPart = u.Extents[ElementCount - 1].FileOffset.QuadPart + u.Extents[ElementCount - 1].Length.QuadPart;

            if ((ULONG)StartBuffer.FileOffset.HighPart > FindData->nFileSizeHigh ||
                ((ULONG)StartBuffer.FileOffset.HighPart == FindData->nFileSizeHigh &&
                 StartBuffer.FileOffset.LowPart >= FindData->nFileSizeLow)) {

                break;
            }
        }

        CloseHandle(hFile);
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the allocation size.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectAllocationSize (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    BOOL RealAllocSize = FALSE;

    if (Opts->GetFileInformationByHandleEx) {

        HANDLE hFile;

        hFile = CreateFile(FullPath,
                           FILE_READ_ATTRIBUTES,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                           NULL);
    
        if (hFile != INVALID_HANDLE_VALUE) {
            FILE_STANDARD_INFO StandardInfo;
    
            if (Opts->GetFileInformationByHandleEx(hFile, FileStandardInfo, &StandardInfo, sizeof(StandardInfo))) {
                Entry->AllocationSize = StandardInfo.AllocationSize;
                RealAllocSize = TRUE;
            }

            CloseHandle(hFile);
        }
    }

    if (!RealAllocSize) {
        ULONG BytesPerSector;
        ULONG SectorsPerCluster;
        ULONG FreeClusters;
        ULONG TotalClusters;
        ULONG ClusterSize;

        GetDiskFreeSpace(Opts->ParentName.StartOfString, &SectorsPerCluster, &BytesPerSector, &FreeClusters, &TotalClusters);

        ClusterSize = SectorsPerCluster * BytesPerSector;

        Entry->AllocationSize.LowPart = FindData->nFileSizeLow;
        Entry->AllocationSize.HighPart = FindData->nFileSizeHigh;

        SdirFileSizeFromLargeInt(&Entry->AllocationSize) = (SdirFileSizeFromLargeInt(&Entry->AllocationSize) + ClusterSize - 1) & (~(ClusterSize - 1));
    }

    return TRUE;
}

/**
 A structure containing the core fields of a PE header.
 */
typedef struct _SDIR_PE_HEADERS {
    /**
     The signature indicating a PE file.
     */
    DWORD Signature;

    /**
     The base PE header.
     */
    IMAGE_FILE_HEADER ImageHeader;

    /**
     The contents of the PE optional header.  This isn't really optional in
     NT since it contains core fields needed for NT to run things.
     */
    IMAGE_OPTIONAL_HEADER OptionalHeader;
} SDIR_PE_HEADERS, *PSDIR_PE_HEADERS;

/**
 Helper function to load an executable's PE header for parsing.  This is used
 by multiple collection functions whose data comes from a PE header.

 @param FullPath Pointer to a string to the full file name.

 @param PeHeaders On successful completion, updated to point to the contents
        of the executable's PE headers.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCapturePeHeaders (
    __in LPCTSTR FullPath,
    __out PSDIR_PE_HEADERS PeHeaders
    )
{
    HANDLE hFileRead;
    IMAGE_DOS_HEADER DosHeader;
    DWORD BytesReturned;

    //
    //  We want the earlier handle to be attribute only so we can
    //  operate on directories, but we need data for this, so we
    //  end up with two handles.
    //

    hFileRead = CreateFile(FullPath,
                           FILE_READ_ATTRIBUTES|FILE_READ_DATA,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);

    if (hFileRead != INVALID_HANDLE_VALUE) {

        if (ReadFile(hFileRead, &DosHeader, sizeof(DosHeader), &BytesReturned, NULL) &&
            BytesReturned == sizeof(DosHeader) &&
            DosHeader.e_magic == IMAGE_DOS_SIGNATURE &&
            DosHeader.e_lfanew != 0) {

            SetFilePointer(hFileRead, DosHeader.e_lfanew, NULL, FILE_BEGIN);

            if (ReadFile(hFileRead, PeHeaders, sizeof(SDIR_PE_HEADERS), &BytesReturned, NULL) &&
                BytesReturned == sizeof(SDIR_PE_HEADERS) &&
                PeHeaders->Signature == IMAGE_NT_SIGNATURE &&
                PeHeaders->ImageHeader.SizeOfOptionalHeader >= FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, Subsystem)) {

                CloseHandle(hFileRead);
                return TRUE;

            }
        }
        CloseHandle(hFileRead);
    }
    return FALSE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the executable's architecture.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectArch (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    SDIR_PE_HEADERS PeHeaders;

    UNREFERENCED_PARAMETER(FindData);

    Entry->OsVersionHigh = 0;
    Entry->OsVersionLow = 0;

    if (SdirCapturePeHeaders(FullPath, &PeHeaders)) {

        Entry->Architecture = PeHeaders.ImageHeader.Machine;
    }

    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's compression algorithm.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectCompressionAlgorithm (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    HANDLE hFile;

    UNREFERENCED_PARAMETER(FindData);

    Entry->CompressionAlgorithm = SdirCompressionNone;

    hFile = CreateFile(FullPath,
                       FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {

        USHORT NtfsCompressionAlgorithm;
        DWORD BytesReturned;
        struct {
            WOF_EXTERNAL_INFO WofHeader;
            union {
                WIM_PROVIDER_EXTERNAL_INFO WimInfo;
                FILE_PROVIDER_EXTERNAL_INFO FileInfo;
            } u;
        } WofInfo;

        if (DeviceIoControl(hFile, FSCTL_GET_COMPRESSION, NULL, 0, &NtfsCompressionAlgorithm, sizeof(NtfsCompressionAlgorithm), &BytesReturned, NULL)) {

            if (NtfsCompressionAlgorithm == COMPRESSION_FORMAT_LZNT1) {
                Entry->CompressionAlgorithm = SdirCompressionLznt;
            } else if (NtfsCompressionAlgorithm != COMPRESSION_FORMAT_NONE) {
                Entry->CompressionAlgorithm = SdirCompressionNtfsUnknown;
            }
        }

        if (Entry->CompressionAlgorithm == SdirCompressionNone) {
            if (DeviceIoControl(hFile, FSCTL_GET_EXTERNAL_BACKING, NULL, 0, &WofInfo, sizeof(WofInfo), &BytesReturned, NULL)) {
    
                if (WofInfo.WofHeader.Provider == WOF_PROVIDER_WIM) {
                    Entry->CompressionAlgorithm = SdirCompressionWim;
                } else if (WofInfo.WofHeader.Provider == WOF_PROVIDER_FILE) {
                    if (WofInfo.u.FileInfo.Algorithm == FILE_PROVIDER_COMPRESSION_XPRESS4K) {
                        Entry->CompressionAlgorithm = SdirCompressionXpress4k;
                    } else if (WofInfo.u.FileInfo.Algorithm == FILE_PROVIDER_COMPRESSION_XPRESS8K) {
                        Entry->CompressionAlgorithm = SdirCompressionXpress8k;
                    } else if (WofInfo.u.FileInfo.Algorithm == FILE_PROVIDER_COMPRESSION_XPRESS16K) {
                        Entry->CompressionAlgorithm = SdirCompressionXpress16k;
                    } else if (WofInfo.u.FileInfo.Algorithm == FILE_PROVIDER_COMPRESSION_LZX) {
                        Entry->CompressionAlgorithm = SdirCompressionLzx;
                    } else {
                        Entry->CompressionAlgorithm = SdirCompressionWofFileUnknown;
                    }
                } else {
                    Entry->CompressionAlgorithm = SdirCompressionWofUnknown;
                }
            }
        }

        CloseHandle(hFile);
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's compression size.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectCompressedFileSize (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    Entry->CompressedFileSize.LowPart = FindData->nFileSizeLow;
    Entry->CompressedFileSize.HighPart = FindData->nFileSizeHigh;

    if (Opts->GetCompressedFileSize) {
        Entry->CompressedFileSize.LowPart = Opts->GetCompressedFileSize(FullPath, (PDWORD)&Entry->CompressedFileSize.HighPart);

        if (Entry->CompressedFileSize.LowPart == INVALID_FILE_SIZE) {
            Entry->CompressedFileSize.LowPart = FindData->nFileSizeLow;
            Entry->CompressedFileSize.HighPart = FindData->nFileSizeHigh;
        }
    }

    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's creation time.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectCreateTime (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    FILETIME tmp;

    UNREFERENCED_PARAMETER(FullPath);

    FileTimeToLocalFileTime(&FindData->ftCreationTime, &tmp);
    FileTimeToSystemTime(&tmp, &Entry->CreateTime);
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's effective permissions.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectEffectivePermissions (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{

    //
    //  Allocate some buffers on the stack to hold the user SID,
    //  and one for the security descriptor which we can reallocate
    //  as needed.
    //

    UCHAR LocalSecurityDescriptor[512];
    PUCHAR SecurityDescriptor = NULL;
    DWORD dwSdRequired = 0;
    HANDLE TokenHandle = NULL;
    DWORD i;
    BOOL AccessGranted;
    ACCESS_MASK UnderstoodPermissions = 0;
    GENERIC_MAPPING Mapping = {0};
    PRIVILEGE_SET Privilege;
    DWORD PrivilegeLength = sizeof(Privilege);

    UNREFERENCED_PARAMETER(FindData);

    Entry->EffectivePermissions = 0;

    SecurityDescriptor = LocalSecurityDescriptor;

    if (!GetFileSecurity(FullPath, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)SecurityDescriptor, sizeof(LocalSecurityDescriptor), &dwSdRequired)) {
        if (dwSdRequired != 0) {
            SecurityDescriptor = YoriLibMalloc(dwSdRequired);
    
            if (!GetFileSecurity(FullPath, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)SecurityDescriptor, dwSdRequired, &dwSdRequired)) {
                goto Exit;
            }
        } else {
            goto Exit;
        }
    }

    if (!ImpersonateSelf(SecurityIdentification)) {
        goto Exit;
    }
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &TokenHandle)) {
        RevertToSelf();
        goto Exit;
    }

    AccessCheck((PSECURITY_DESCRIPTOR)SecurityDescriptor, TokenHandle, MAXIMUM_ALLOWED, &Mapping, &Privilege, &PrivilegeLength, &Entry->EffectivePermissions, &AccessGranted);

    //
    //  Strip off any permissions we don't understand so that tests for
    //  equality are meaningful
    //

    for (i = 0; i < SdirGetNumPermissionPairs(); i++) {
        UnderstoodPermissions |= PermissionPairs[i].FileAttribute;
    }

    Entry->EffectivePermissions &= UnderstoodPermissions;

Exit:
    if (TokenHandle != NULL) {
        CloseHandle(TokenHandle);
        RevertToSelf();
    }
    if (SecurityDescriptor != NULL && SecurityDescriptor != LocalSecurityDescriptor) {
        YoriLibFree(SecurityDescriptor);
    }

    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's attributes.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectFileAttributes (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    DWORD i;

    UNREFERENCED_PARAMETER(FullPath);

    //
    //  We do this bit by bit to ensure that we don't have file attributes
    //  recorded that we don't understand.  This allows us to perform
    //  equality comparisons where the result is understandable to the user
    //  in that it can be specified and displayed.
    //

    Entry->FileAttributes = 0;
    for (i = 0; i < SdirGetNumAttrPairs(); i++) {
        if (FindData->dwFileAttributes & AttrPairs[i].FileAttribute) {
            Entry->FileAttributes |= AttrPairs[i].FileAttribute;
        }
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's extension.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectFileExtension (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    UNREFERENCED_PARAMETER(FullPath);
    UNREFERENCED_PARAMETER(FindData);
    UNREFERENCED_PARAMETER(Entry);

    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's ID.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectFileId (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    HANDLE hFile;

    UNREFERENCED_PARAMETER(FindData);

    Entry->FileId.QuadPart = 0;

    hFile = CreateFile(FullPath,
                       FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        BY_HANDLE_FILE_INFORMATION FileInfo;

        if (GetFileInformationByHandle(hFile, &FileInfo)) {
            Entry->FileId.LowPart = FileInfo.nFileIndexLow;
            Entry->FileId.HighPart = FileInfo.nFileIndexHigh;
        }

        CloseHandle(hFile);
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's name.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectFileName (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    UNREFERENCED_PARAMETER(FullPath);

    SdirCopyFileName(Entry->FileName, FindData->cFileName, SDIR_MAX_FILE_NAME - 1, &Entry->FileNameLengthInChars);

    Entry->Extension = _tcsrchr(Entry->FileName, '.');

    //
    //  For simplicity's sake, if we have no extension set the field
    //  to the end of string, so we'll see a valid pointer of nothing.
    //

    if (Entry->Extension == NULL) {
        Entry->Extension = Entry->FileName + Entry->FileNameLengthInChars;
    } else {
        Entry->Extension++;
    }

    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's size.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectFileSize (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    UNREFERENCED_PARAMETER(FullPath);

    Entry->FileSize.LowPart = FindData->nFileSizeLow;
    Entry->FileSize.HighPart = FindData->nFileSizeHigh;
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's fragment count.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectFragmentCount (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    HANDLE hFile;

    UNREFERENCED_PARAMETER(FindData);

    Entry->FragmentCount.HighPart = 0;
    Entry->FragmentCount.LowPart = 0;

    hFile = CreateFile(FullPath,
                       FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {

        STARTING_VCN_INPUT_BUFFER StartBuffer;
        union {
            RETRIEVAL_POINTERS_BUFFER Extents;
            UCHAR Buffer[2048];
        } u;
        DWORD BytesReturned;

        LARGE_INTEGER PriorRunLength = {0};
        LARGE_INTEGER PriorNextVcn = {0};
        LARGE_INTEGER PriorLcn = {0};

        StartBuffer.StartingVcn.QuadPart = 0;

        while ((DeviceIoControl(hFile, FSCTL_GET_RETRIEVAL_POINTERS, &StartBuffer, sizeof(StartBuffer), &u.Extents, sizeof(u), &BytesReturned, NULL) || GetLastError() == ERROR_MORE_DATA) &&
               u.Extents.ExtentCount > 0) {

            // 
            //  Look through the extents.  If it's not a sparse hole, record it as a 
            //  fragment.  If it's also discontiguous with the previous run, count it as a
            //  fragment.
            //

            for (BytesReturned = 0; BytesReturned < u.Extents.ExtentCount; BytesReturned++) {
                if (u.Extents.Extents[BytesReturned].Lcn.HighPart != (DWORD)-1 &&
                    u.Extents.Extents[BytesReturned].Lcn.LowPart != (DWORD)-1) {

                    if (PriorLcn.QuadPart + PriorRunLength.QuadPart != u.Extents.Extents[BytesReturned].Lcn.QuadPart) {
                        if (Entry->FragmentCount.LowPart == (DWORD)-1) {
                            Entry->FragmentCount.HighPart++;
                        }
                        Entry->FragmentCount.LowPart++;
                    }
                }

                PriorRunLength.QuadPart = u.Extents.Extents[BytesReturned].NextVcn.QuadPart - PriorNextVcn.QuadPart;
                PriorNextVcn = u.Extents.Extents[BytesReturned].NextVcn;
                PriorLcn = u.Extents.Extents[BytesReturned].Lcn;
            }

            StartBuffer.StartingVcn.QuadPart = u.Extents.Extents[u.Extents.ExtentCount - 1].NextVcn.QuadPart;
        }

        CloseHandle(hFile);
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's link count.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectLinkCount (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    HANDLE hFile;

    UNREFERENCED_PARAMETER(FindData);

    Entry->LinkCount = 0;

    hFile = CreateFile(FullPath,
                       FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        BY_HANDLE_FILE_INFORMATION FileInfo;

        if (GetFileInformationByHandle(hFile, &FileInfo)) {
            Entry->LinkCount = FileInfo.nNumberOfLinks;
        }

        CloseHandle(hFile);
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's object ID.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectObjectId (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    HANDLE hFile;
    FILE_OBJECTID_BUFFER Buffer;
    DWORD BytesReturned;

    UNREFERENCED_PARAMETER(FindData);

    ZeroMemory(&Entry->ObjectId, sizeof(Entry->ObjectId));

    hFile = CreateFile(FullPath,
                       FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        if (DeviceIoControl(hFile, FSCTL_GET_OBJECT_ID, NULL, 0, &Buffer, sizeof(Buffer), &BytesReturned, NULL)) {
            memcpy(&Entry->ObjectId, &Buffer.ObjectId, sizeof(Buffer.ObjectId));
        }
        CloseHandle(hFile);
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the executable's minimum OS version.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectOsVersion (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    SDIR_PE_HEADERS PeHeaders;

    UNREFERENCED_PARAMETER(FindData);

    Entry->OsVersionHigh = 0;
    Entry->OsVersionLow = 0;

    if (SdirCapturePeHeaders(FullPath, &PeHeaders)) {

        Entry->OsVersionHigh = PeHeaders.OptionalHeader.MajorSubsystemVersion;
        Entry->OsVersionLow = PeHeaders.OptionalHeader.MinorSubsystemVersion;
    }

    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's owner.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectOwner (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{

    //
    //  Allocate some buffers on the stack to hold the user name, domain name
    //  and owner portion of the security descriptor.  In the first case, this
    //  is to help ensure we have space to store the whole thing; in the
    //  second case, this function crashes without a buffer even if we discard
    //  the result; and the descriptor here doesn't contain the ACL and only
    //  needs to be big enough to hold one variable sized owner SID.
    //

    TCHAR UserName[128];
    DWORD NameLength = sizeof(UserName)/sizeof(UserName[0]);
    TCHAR DomainName[128];
    DWORD DomainLength = sizeof(DomainName)/sizeof(DomainName[0]);
    UCHAR SecurityDescriptor[256];
    DWORD dwSdRequired = 0;
    BOOL OwnerDefaulted;
    PSID pOwnerSid;
    SID_NAME_USE eUse;

    UNREFERENCED_PARAMETER(FindData);

    UserName[0] = '\0';
    Entry->Owner[0] = '\0';

    if (GetFileSecurity(FullPath, OWNER_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)SecurityDescriptor, sizeof(SecurityDescriptor), &dwSdRequired)) {
        if (GetSecurityDescriptorOwner((PSECURITY_DESCRIPTOR)SecurityDescriptor, &pOwnerSid, &OwnerDefaulted)) {
            if (LookupAccountSid(NULL, pOwnerSid, UserName, &NameLength, DomainName, &DomainLength, &eUse)) {
                UserName[sizeof(Entry->Owner) - 1] = '\0';
                memcpy(Entry->Owner, UserName, sizeof(Entry->Owner));
            }
        }
    }

    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's reparse tag.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectReparseTag (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    UNREFERENCED_PARAMETER(FullPath);

    if (Entry->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        Entry->ReparseTag = FindData->dwReserved0;
    } else {
        Entry->ReparseTag = 0;
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's short file name.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectShortName (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    UNREFERENCED_PARAMETER(FullPath);

    if (FindData->cAlternateFileName[0] == '\0') {
        DWORD FileNameLength = (DWORD)_tcslen(FindData->cFileName);

        if (FileNameLength <= 12) {
            SdirCopyFileName(Entry->ShortFileName, FindData->cFileName, sizeof(Entry->ShortFileName), NULL);
        } else {
            Entry->ShortFileName[0] = '\0';
        }
    } else {
        SdirCopyFileName(Entry->ShortFileName, FindData->cAlternateFileName, sizeof(Entry->ShortFileName), NULL);
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the executable's subsystem type.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectSubsystem (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    SDIR_PE_HEADERS PeHeaders;

    UNREFERENCED_PARAMETER(FindData);

    Entry->Subsystem = 0;

    if (SdirCapturePeHeaders(FullPath, &PeHeaders)) {

        Entry->Subsystem = PeHeaders.OptionalHeader.Subsystem;
    }

    return TRUE;
}

#ifdef UNICODE
/**
 Collect information from a directory enumerate and full file name relating
 to the file's stream count.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectStreamCount (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    HANDLE hFind;
    SDIR_WIN32_FIND_STREAM_DATA FindStreamData;

    UNREFERENCED_PARAMETER(FindData);

    Entry->StreamCount = 0;

    //
    //  These APIs are Unicode only.  We could do an ANSI to Unicode thunk here, but
    //  since Unicode is the default build and ANSI is only useful for older systems
    //  (where this API won't exist) there doesn't seem much point.
    //

    if (Opts->FindFirstStreamW && Opts->FindNextStreamW) {
        hFind = Opts->FindFirstStreamW(FullPath, 0, &FindStreamData, 0);
        if (hFind != INVALID_HANDLE_VALUE) {

            do {
                Entry->StreamCount++;
            } while (Opts->FindNextStreamW(hFind, &FindStreamData));
        }

        FindClose(hFind);
    }

    return TRUE;
}
#endif

/**
 Collect information from a directory enumerate and full file name relating
 to the file's USN.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectUsn (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    HANDLE hFile;

    UNREFERENCED_PARAMETER(FindData);

    Entry->Usn.QuadPart = 0;
    hFile = CreateFile(FullPath,
                       FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {

        struct {
            SDIR_USN_RECORD UsnRecord;
            WCHAR FileName[SDIR_MAX_FILE_NAME];
        } s1;
        DWORD BytesReturned;

        if (DeviceIoControl(hFile, FSCTL_READ_FILE_USN_DATA, NULL, 0, &s1, sizeof(s1), &BytesReturned, NULL)) {
            Entry->Usn.QuadPart = s1.UsnRecord.Usn.QuadPart;
        }

        CloseHandle(hFile);
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the executable's version resource.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectVersion (
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    DWORD Junk;
    PVOID Buffer;
    DWORD VerSize;
    VS_FIXEDFILEINFO * RootBlock;

    if (Opts->GetFileVersionInfoSize == NULL) {
        if (!SdirLoadVerFunctions()) {
            return TRUE;
        }
    }
    VerSize = Opts->GetFileVersionInfoSize((LPTSTR)FullPath, &Junk);

    UNREFERENCED_PARAMETER(FindData);

    Entry->FileVersion.QuadPart = 0;
    Entry->FileVersionFlags = 0;

    Buffer = YoriLibMalloc(VerSize);
    if (Buffer != NULL) {
        if (Opts->GetFileVersionInfo((LPTSTR)FullPath, 0, VerSize, Buffer)) {
            if (Opts->VerQueryValue(Buffer, _T("\\"), (PVOID*)&RootBlock, (PUINT)&Junk)) {
                Entry->FileVersion.HighPart = RootBlock->dwFileVersionMS;
                Entry->FileVersion.LowPart = RootBlock->dwFileVersionLS;
                Entry->FileVersionFlags = RootBlock->dwFileFlags & RootBlock->dwFileFlagsMask;
            }
        }
        YoriLibFree(Buffer);
    }
    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's write time.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectWriteTime(
    __inout PSDIR_DIRENT Entry,
    __in PWIN32_FIND_DATA FindData,
    __in LPCTSTR FullPath
    )
{
    FILETIME tmp;

    UNREFERENCED_PARAMETER(FullPath);

    FileTimeToLocalFileTime(&FindData->ftLastWriteTime, &tmp);
    FileTimeToSystemTime(&tmp, &Entry->WriteTime);
    return TRUE;
}

/**
 Collect information from a directory entry into the active summary count.

 @param Entry The directory entry to count in the summary.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCollectSummary(
    __in PSDIR_DIRENT Entry
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

    if (Opts->EnableAverageLinkSize) {
        ULONG EffectiveLinkCount = Entry->LinkCount;
        if (EffectiveLinkCount == 0) {
            EffectiveLinkCount = 1;
        }
        Summary->TotalSize += SdirFileSizeFromLargeInt(&Entry->FileSize) / EffectiveLinkCount;
    
        if (Opts->FtCompressedFileSize.Flags & SDIR_FEATURE_COLLECT) {
            Summary->CompressedSize += SdirFileSizeFromLargeInt(&Entry->CompressedFileSize) / EffectiveLinkCount;
        }
    } else {
        Summary->TotalSize += SdirFileSizeFromLargeInt(&Entry->FileSize);
    
        if (Opts->FtCompressedFileSize.Flags & SDIR_FEATURE_COLLECT) {
            Summary->CompressedSize += SdirFileSizeFromLargeInt(&Entry->CompressedFileSize);
        }
    }

    return TRUE;
}

//
//  When criteria are specified to apply attributes, we need to load the
//  specification into a dummy dirent to perform comparisons against.  The
//  below functions implement these.
//

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for last access date.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateAccessDate(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    return SdirStringToDate(String, &Entry->AccessTime);
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for last access time.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateAccessTime(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    return SdirStringToTime(String, &Entry->AccessTime);
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for the number of allocated ranges.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateAllocatedRangeCount(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    Entry->AllocatedRangeCount = SdirStringToNum64(String, NULL);
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's allocation size.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateAllocationSize(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    Entry->AllocationSize = SdirStringToFileSize(String);
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for an executable's CPU architecture.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateArch(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    if (_tcsicmp(String, _T("None")) == 0) {
        Entry->Architecture = 0;
    } else if (_tcsicmp(String, _T("i386")) == 0) {
        Entry->Architecture = IMAGE_FILE_MACHINE_I386;
    } else if (_tcsicmp(String, _T("amd64")) == 0) {
        Entry->Architecture = IMAGE_FILE_MACHINE_AMD64;
    } else if (_tcsicmp(String, _T("arm")) == 0) {
        Entry->Architecture = IMAGE_FILE_MACHINE_ARMNT;
    } else if (_tcsicmp(String, _T("arm64")) == 0) {
        Entry->Architecture = IMAGE_FILE_MACHINE_ARM64;
    } else {
        return FALSE;
    }
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's compression algorithm.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateCompressionAlgorithm(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    if (_tcsicmp(String, _T("None")) == 0) {
        Entry->CompressionAlgorithm = SdirCompressionNone;
    } else if (_tcsicmp(String, _T("LZNT")) == 0) {
        Entry->CompressionAlgorithm = SdirCompressionLznt;
    } else if (_tcsicmp(String, _T("NTFS")) == 0) {
        Entry->CompressionAlgorithm = SdirCompressionNtfsUnknown;
    } else if (_tcsicmp(String, _T("WIM")) == 0) {
        Entry->CompressionAlgorithm = SdirCompressionWim;
    } else if (_tcsicmp(String, _T("LZX")) == 0) {
        Entry->CompressionAlgorithm = SdirCompressionLzx;
    } else if (_tcsicmp(String, _T("Xp4")) == 0) {
        Entry->CompressionAlgorithm = SdirCompressionXpress4k;
    } else if (_tcsicmp(String, _T("Xp8")) == 0) {
        Entry->CompressionAlgorithm = SdirCompressionXpress8k;
    } else if (_tcsicmp(String, _T("Xp16")) == 0) {
        Entry->CompressionAlgorithm = SdirCompressionXpress16k;
    } else if (_tcsicmp(String, _T("File")) == 0) {
        Entry->CompressionAlgorithm = SdirCompressionWofFileUnknown;
    } else if (_tcsicmp(String, _T("Wof")) == 0) {
        Entry->CompressionAlgorithm = SdirCompressionWofUnknown;
    } else {
        return FALSE;
    }
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's compressed file size.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateCompressedFileSize(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    Entry->CompressedFileSize = SdirStringToFileSize(String);
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's creation date.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateCreateDate(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    return SdirStringToDate(String, &Entry->CreateTime);
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's creation time.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateCreateTime(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    return SdirStringToTime(String, &Entry->CreateTime);
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's effective permissions.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateEffectivePermissions(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    DWORD i;

    Entry->FileAttributes = 0;

    while (*String) {

        for (i = 0; i < SdirGetNumPermissionPairs(); i++) {
            if (*String == PermissionPairs[i].DisplayLetter) {
                Entry->EffectivePermissions |= PermissionPairs[i].FileAttribute;
            }
        }

        String++;
    }
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's attributes.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateFileAttributes(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    DWORD i;

    Entry->FileAttributes = 0;

    while (*String) {

        for (i = 0; i < SdirGetNumAttrPairs(); i++) {
            if (*String == AttrPairs[i].DisplayLetter) {
                Entry->FileAttributes |= AttrPairs[i].FileAttribute;
            }
        }

        String++;
    }
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's extension.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateFileExtension(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    //
    //  Since we have one dirent per comparison, just shove the extension in
    //  the file name buffer and point the extension to it.  This buffer can't
    //  be used for anything else anyway.
    //

    YoriLibSPrintfS(Entry->FileName, sizeof(Entry->FileName)/sizeof(Entry->FileName[0]), _T("%s"), String);
    Entry->FileName[sizeof(Entry->FileName)/sizeof(Entry->FileName[0]) - 1] = '\0';
    Entry->Extension = Entry->FileName;

    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's name.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateFileName(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    Entry->FileNameLengthInChars = YoriLibSPrintfS(Entry->FileName, sizeof(Entry->FileName)/sizeof(Entry->FileName[0]), _T("%s"), String);
    Entry->FileName[sizeof(Entry->FileName)/sizeof(Entry->FileName[0]) - 1] = '\0';

    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's size.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateFileSize(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    Entry->FileSize = SdirStringToFileSize(String);
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's fragment count.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateFragmentCount(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    Entry->FragmentCount = SdirStringToNum64(String, NULL);
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's link count.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateLinkCount(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    Entry->LinkCount = SdirStringToNum32(String, NULL);
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's object ID.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateObjectId(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    UCHAR Buffer[16];
    if (SdirStringToHexBuffer(String, (PUCHAR)&Buffer, sizeof(Buffer))) {
        memcpy(Entry->ObjectId, Buffer, sizeof(Buffer));
    }
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for an executable's minimum OS version.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateOsVersion(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    LPCTSTR endptr = NULL;

    Entry->OsVersionHigh = (WORD)SdirStringToNum32(String, &endptr);

    if (*endptr == '.') {
        Entry->OsVersionLow = (WORD)SdirStringToNum32(endptr+1, NULL);
    }

    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's owner.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateOwner(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    YoriLibSPrintfS(Entry->Owner, sizeof(Entry->Owner)/sizeof(Entry->Owner[0]), _T("%s"), String);
    Entry->Owner[sizeof(Entry->Owner)/sizeof(Entry->Owner[0]) - 1] = '\0';

    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's reparse tag.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateReparseTag(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    Entry->ReparseTag = SdirStringToNum32(String, NULL);
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's short file name.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateShortName(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    YoriLibSPrintfS(Entry->ShortFileName, sizeof(Entry->ShortFileName)/sizeof(Entry->ShortFileName[0]), _T("%s"), String);
    Entry->ShortFileName[sizeof(Entry->ShortFileName)/sizeof(Entry->ShortFileName[0]) - 1] = '\0';

    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for an executable's target subsystem.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateSubsystem (
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    if (_tcsicmp(String, _T("None")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_UNKNOWN;
    } else if (_tcsicmp(String, _T("NT")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_NATIVE;
    } else if (_tcsicmp(String, _T("GUI")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
    } else if (_tcsicmp(String, _T("Cons")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    } else if (_tcsicmp(String, _T("OS/2")) == 0 || _tcsicmp(String, _T("OS2")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_OS2_CUI;
    } else if (_tcsicmp(String, _T("Posx")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_POSIX_CUI;
    } else if (_tcsicmp(String, _T("w9x")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_NATIVE_WINDOWS;
    } else if (_tcsicmp(String, _T("CE")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CE_GUI;
    } else if (_tcsicmp(String, _T("EFIa")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_EFI_APPLICATION;
    } else if (_tcsicmp(String, _T("EFIb")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    } else if (_tcsicmp(String, _T("EFId")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER;
    } else if (_tcsicmp(String, _T("EFIr")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_EFI_ROM;
    } else if (_tcsicmp(String, _T("Xbox")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_XBOX;
    } else if (_tcsicmp(String, _T("Xbcc")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG;
    } else if (_tcsicmp(String, _T("Boot")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION;
    } else {
        return FALSE;
    }

    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's stream count.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateStreamCount(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    Entry->StreamCount = SdirStringToNum32(String, NULL);
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's USN.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateUsn(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    Entry->Usn = SdirStringToNum64(String, NULL);
    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for an executable's version.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateVersion(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    LARGE_INTEGER Version = {0};
    LPCTSTR endptr = NULL;

    Version.HighPart = SdirStringToNum32(String, &endptr)<<16;

    if (*endptr == '.') {
        Version.HighPart = Version.HighPart + (WORD)SdirStringToNum32(endptr+1, &endptr);

        if (*endptr == '.') {
            Version.LowPart = SdirStringToNum32(endptr+1, &endptr)<<16;
            if (*endptr == '.') {
                Version.LowPart = Version.LowPart + (WORD)SdirStringToNum32(endptr+1, NULL);
            }
        }
    }

    Entry->FileVersion = Version;

    return TRUE;
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's write date.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateWriteDate(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    return SdirStringToDate(String, &Entry->WriteTime);
}

/**
 Parse a NULL terminated string and populate a directory entry to facilitate
 comparisons for a file's write time.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a NULL terminated string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirGenerateWriteTime(
    __inout PSDIR_DIRENT Entry,
    __in LPCTSTR String
    )
{
    return SdirStringToTime(String, &Entry->WriteTime);
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
    )
{
    TCHAR StrBuf[8];
    LPTSTR StrFixed;

    if (Buffer) {
        switch(Entry->CompressionAlgorithm) {
            case SdirCompressionNone:
                StrFixed = _T("None");
                break;
            case SdirCompressionLznt:
                StrFixed = _T("LZNT");
                break;
            case SdirCompressionNtfsUnknown:
                StrFixed = _T("NTFS");
                break;
            case SdirCompressionWim:
                StrFixed = _T("WIM ");
                break;
            case SdirCompressionLzx:
                StrFixed = _T("LZX ");
                break;
            case SdirCompressionXpress4k:
                StrFixed = _T("Xp4 ");
                break;
            case SdirCompressionXpress8k:
                StrFixed = _T("Xp8 ");
                break;
            case SdirCompressionXpress16k:
                StrFixed = _T("Xp16");
                break;
            case SdirCompressionWofFileUnknown:
                StrFixed = _T("File");
                break;
            case SdirCompressionWofUnknown:
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
    __in PSDIR_DIRENT Entry
    )
{
    return SdirDisplayGenericSize(Buffer, Attributes, &Entry->CompressedFileSize);
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
    __in PSDIR_DIRENT Entry
    )
{
    TCHAR StrAtts[32];
    DWORD i;

    if (Buffer) {
        StrAtts[0] = ' ';
        for (i = 0; i < SdirGetNumPermissionPairs(); i++) {
            if (Entry->EffectivePermissions & PermissionPairs[i].FileAttribute) {
                StrAtts[i + 1] = PermissionPairs[i].DisplayLetter;
            } else {
                StrAtts[i + 1] = '-';
            }
        }

        StrAtts[i + 1] = '\0';
    
        SdirPasteStr(Buffer, StrAtts, Attributes, SdirGetNumPermissionPairs() + 1);
    }
    return SdirGetNumPermissionPairs() + 1;
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
    )
{
    TCHAR StrAtts[32];
    DWORD i;

    if (Buffer) {
        StrAtts[0] = ' ';
        for (i = 0; i < SdirGetNumAttrPairs(); i++) {
            if (Entry->FileAttributes & AttrPairs[i].FileAttribute) {
                StrAtts[i + 1] = AttrPairs[i].DisplayLetter;
            } else {
                StrAtts[i + 1] = '-';
            }
        }

        StrAtts[i + 1] = '\0';
    
        SdirPasteStr(Buffer, StrAtts, Attributes, SdirGetNumAttrPairs() + 1);
    }
    return SdirGetNumAttrPairs() + 1;
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
    )
{
    if (Buffer) {
        if (Entry->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            if (Entry->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
                SdirPasteStr(Buffer, _T(" <LNK>"), Entry->RenderAttributes, 6);
            } else if (Entry->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
                SdirPasteStr(Buffer, _T(" <MNT>"), Entry->RenderAttributes, 6);
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
    )
{
    TCHAR Str[40];

    if (Buffer) {
        YoriLibSPrintfS(Str,
                    sizeof(Str)/sizeof(Str[0]),
                    _T(" %02i.%02i.%05i.%05i %c%c"),
                    (int)(Entry->FileVersion.HighPart >> 16),
                    (int)(Entry->FileVersion.HighPart & 0xffff),
                    (int)(Entry->FileVersion.LowPart >> 16),
                    (int)(Entry->FileVersion.LowPart & 0xffff),
                    Entry->FileVersionFlags & VS_FF_DEBUG ? 'D' : '-',
                    Entry->FileVersionFlags & VS_FF_PRERELEASE ? 'P' : '-');

        SdirPasteStr(Buffer, Str, Attributes, 21);
    }
    return 21;
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
    __in PSDIR_DIRENT Entry
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
    __in PSDIR_DIRENT Entry
    )
{
    return SdirDisplayFileTime(Buffer, Attributes, &Entry->WriteTime);
}

//
//

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

    {OPT_OS(FtAllocatedRangeCount),      _T("ac"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayAllocatedRangeCount,  SdirCollectAllocatedRangeCount,
        SdirCompareAllocatedRangeCount,  NULL,                          SdirGenerateAllocatedRangeCount,
        "allocated range count"},

    {OPT_OS(FtAccessDate),               _T("ad"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN|FOREGROUND_BLUE},
        SdirDisplayFileAccessDate,       SdirCollectAccessTime,
        SdirCompareAccessDate,           NULL,                          SdirGenerateAccessDate,
        "access date"},

    {OPT_OS(FtArch),                     _T("ar"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY},
        SdirDisplayArch,                 SdirCollectArch,
        SdirCompareArch,                 NULL,                          SdirGenerateArch,
        "CPU architecture"},

    {OPT_OS(FtAllocationSize),           _T("as"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY},
        SdirDisplayAllocationSize,       SdirCollectAllocationSize,
        SdirCompareAllocationSize,       NULL,                          SdirGenerateAllocationSize,
        "allocation size"},

    {OPT_OS(FtAccessTime),               _T("at"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN|FOREGROUND_BLUE},
        SdirDisplayFileAccessTime,       SdirCollectAccessTime,
        SdirCompareAccessTime,           NULL,                          SdirGenerateAccessTime,
        "access time"},

    {OPT_OS(FtBriefAlternate),           _T("ba"), {0, 0, BACKGROUND_BLUE},
        NULL,                            NULL,                 
        NULL,                            NULL,                          NULL,
        "brief alternate mask"},

    {OPT_OS(FtCompressionAlgorithm),     _T("ca"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayCompressionAlgorithm, SdirCollectCompressionAlgorithm,
        SdirCompareCompressionAlgorithm, NULL,                          SdirGenerateCompressionAlgorithm,
        "compression algorithm"},
    
    {OPT_OS(FtCreateDate),               _T("cd"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayFileCreateDate,       SdirCollectCreateTime,
        SdirCompareCreateDate,           NULL,                          SdirGenerateCreateDate,
        "create date"},

    {OPT_OS(FtCompressedFileSize),       _T("cs"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY},
        SdirDisplayCompressedFileSize,   SdirCollectCompressedFileSize,
        SdirCompareCompressedFileSize,   NULL,                          SdirGenerateCompressedFileSize,
        "compressed size"},

    {OPT_OS(FtCreateTime),               _T("ct"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayFileCreateTime,       SdirCollectCreateTime,
        SdirCompareCreateTime,           NULL,                          SdirGenerateCreateTime,
        "create time"},

    {OPT_OS(FtEffectivePermissions),     _T("ep"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayEffectivePermissions, SdirCollectEffectivePermissions,
        SdirCompareEffectivePermissions, SdirBitwiseEffectivePermissions, SdirGenerateEffectivePermissions,
        "effective permissions"},

    {OPT_OS(FtError),                    _T("er"), {0, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_INTENSITY},
        NULL,                            NULL,                 
        NULL,                            NULL,                          NULL,
        "errors"},

    {OPT_OS(FtFileAttributes),           _T("fa"), {SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_DISPLAY, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY},
        SdirDisplayFileAttributes,       SdirCollectFileAttributes,
        SdirCompareFileAttributes,       SdirBitwiseFileAttributes,     SdirGenerateFileAttributes,
        "file attributes"},

    {OPT_OS(FtFragmentCount),            _T("fc"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayFragmentCount,        SdirCollectFragmentCount,
        SdirCompareFragmentCount,        NULL,                          SdirGenerateFragmentCount,
        "fragment count"},

    {OPT_OS(FtFileExtension),            _T("fe"), {SDIR_FEATURE_DISPLAY|SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_SORT|SDIR_FEATURE_FIXED_COLOR, 0, 0},
        NULL,                            SdirCollectFileExtension,
        SdirCompareFileExtension,        NULL,                          SdirGenerateFileExtension,
        "file extension"},

    {OPT_OS(FtFileId),                   _T("fi"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayFileId,               SdirCollectFileId,
        SdirCompareFileId,               NULL,                          NULL,
        "file id"},

    {OPT_OS(FtFileName),                 _T("fn"), {SDIR_FEATURE_DISPLAY|SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT|SDIR_FEATURE_USE_FILE_COLOR, 0, 0},
        NULL,                            SdirCollectFileName,
        SdirCompareFileName,             SdirBitwiseFileName,           SdirGenerateFileName,
        "file name"},

    {OPT_OS(FtFileSize),                 _T("fs"), {SDIR_FEATURE_DISPLAY|SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY},
        SdirDisplayFileSize,             SdirCollectFileSize,
        SdirCompareFileSize,             NULL,                          SdirGenerateFileSize,
        "file size"},

    {OPT_OS(FtGrid),                     _T("gr"), {0, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN},
        NULL,                            NULL,                 
        NULL,                            NULL,                          NULL,
        "grid"},

    {OPT_OS(FtLinkCount),                _T("lc"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayLinkCount,            SdirCollectLinkCount,
        SdirCompareLinkCount,            NULL,                          SdirGenerateLinkCount,
        "link count"},

    {OPT_OS(FtNumberFiles),              _T("nf"), {0, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN|FOREGROUND_INTENSITY},
        NULL,                            NULL,                 
        NULL,                            NULL,                          NULL,
        "number files"},

#ifdef UNICODE
    {OPT_OS(FtNamedStreams),             _T("ns"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_FIXED_COLOR, 0, 0},
        NULL,                            NULL,                   
        NULL,                            NULL,                          NULL,
        "named streams"},
#endif

    {OPT_OS(FtObjectId),                _T("oi"), {SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayObjectId,             SdirCollectObjectId,
        SdirCompareObjectId,             NULL,                          SdirGenerateObjectId,
        "object id"},

    {OPT_OS(FtOsVersion),                _T("os"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayOsVersion,            SdirCollectOsVersion,
        SdirCompareOsVersion,            NULL,                          SdirGenerateOsVersion,
        "minimum OS version"},

    {OPT_OS(FtOwner),                    _T("ow"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayOwner,                SdirCollectOwner,
        SdirCompareOwner,                NULL,                          SdirGenerateOwner,
        "owner"},

    {OPT_OS(FtReparseTag),               _T("rt"), {SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayReparseTag,           SdirCollectReparseTag,
        SdirCompareReparseTag,           NULL,                          SdirGenerateReparseTag,
        "reparse tag"},

#ifdef UNICODE
    {OPT_OS(FtStreamCount),              _T("sc"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayStreamCount,          SdirCollectStreamCount,
        SdirCompareStreamCount,          NULL,                          SdirGenerateStreamCount,
        "stream count"},
#endif

    {OPT_OS(FtSummary),                  _T("sm"), {SDIR_FEATURE_DISPLAY|SDIR_FEATURE_COLLECT|SDIR_FEATURE_ALLOW_DISPLAY, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE},
        NULL,                            NULL,
        NULL,                            NULL,                          NULL,
        "summary"},

    {OPT_OS(FtShortName),                _T("sn"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT|SDIR_FEATURE_USE_FILE_COLOR, 0, 0},
        SdirDisplayShortName,            SdirCollectShortName,
        SdirCompareShortName,            NULL,                          SdirGenerateShortName,
        "short name"},

    {OPT_OS(FtSubsystem),                _T("ss"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplaySubsystem,            SdirCollectSubsystem,
        SdirCompareSubsystem,            NULL,                          SdirGenerateSubsystem,
        "subsystem"},

    {OPT_OS(FtUsn),                      _T("us"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayUsn,                  SdirCollectUsn,
        SdirCompareUsn,                  NULL,                          SdirGenerateUsn,
        "USN"},

    {OPT_OS(FtVersion),                  _T("vr"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_RED|FOREGROUND_GREEN},
        SdirDisplayVersion,              SdirCollectVersion,
        SdirCompareVersion,              NULL,                          SdirGenerateVersion,
        "version"},

    {OPT_OS(FtWriteDate),                _T("wd"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN},
        SdirDisplayFileWriteDate,        SdirCollectWriteTime,
        SdirCompareWriteDate,            NULL,                          SdirGenerateWriteDate,
        "write date"},

    {OPT_OS(FtWriteTime),                _T("wt"), {SDIR_FEATURE_ALLOW_DISPLAY|SDIR_FEATURE_ALLOW_SORT, SDIR_ATTRCTRL_WINDOW_BG, FOREGROUND_GREEN},
        SdirDisplayFileWriteTime,        SdirCollectWriteTime,
        SdirCompareWriteTime,            NULL,                          SdirGenerateWriteTime,
        "write time"},
};

/**
 Return the count of features that exist in the table of things supported.
 This exists so other modules can know about how many things there are
 without having access to know what all of the things are.
 */
DWORD
SdirGetNumSdirOptions()
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
    {OPT_OS(FtCompressionAlgorithm), SdirDisplayCompressionAlgorithm},
    {OPT_OS(FtFragmentCount),        SdirDisplayFragmentCount},
    {OPT_OS(FtAllocatedRangeCount),  SdirDisplayAllocatedRangeCount},
};

/**
 Return the count of features that exist in the table of things to execute.
 This exists so other modules can know about how many things there are to do
 without having access to know what all of the things are.
 */
DWORD
SdirGetNumSdirExec()
{
    return sizeof(SdirExec)/sizeof(SdirExec[0]);
}

// vim:sw=4:ts=4:et:
