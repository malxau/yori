/**
 * @file lib/fileinfo.c
 *
 * Collect information about files
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Specifies a pointer to a function which can enumerate named streams on a
 file.
 */
typedef HANDLE (WINAPI * PFIND_FIRST_STREAM_FN)(LPCTSTR, DWORD, PWIN32_FIND_STREAM_DATA, DWORD);

/**
 Specifies a pointer to a function which can enumerate named streams on a
 file.
 */
typedef BOOL (WINAPI * PFIND_NEXT_STREAM_FN)(HANDLE, PWIN32_FIND_STREAM_DATA);

/**
 Specifies a pointer to a function which can return the compressed file size
 for a file.
 */
typedef DWORD (WINAPI * PGET_COMPRESSED_FILE_SIZE_FN)(LPCTSTR, LPDWORD);

/**
 Specifies a pointer to a function which can return extra information about
 an opened file.
 */
typedef BOOL (WINAPI * PGET_FILE_INFORMATION_BY_HANDLE_EX_FN)(HANDLE, ULONG, PVOID, DWORD);

/**
 Prototype for the GetFileVersionInfoSize function.
 */
typedef DWORD WINAPI GET_FILE_VERSION_INFO_SIZEW_FN(LPWSTR, LPDWORD);

/**
 Pointer to the GetFileVersionInfoSize function.
 */
typedef GET_FILE_VERSION_INFO_SIZEW_FN *PGET_FILE_VERSION_INFO_SIZEW_FN;

/**
 Prototype for the GetFileVersionInfo function.
 */
typedef BOOL WINAPI GET_FILE_VERSION_INFOW_FN(LPWSTR, DWORD, DWORD, LPVOID);

/**
 Pointer to the GetFileVersionInfo function.
 */
typedef GET_FILE_VERSION_INFOW_FN *PGET_FILE_VERSION_INFOW_FN;

/**
 Prototype for the VerQueryValue function.
 */
typedef BOOL WINAPI VER_QUERY_VALUEW_FN(CONST LPVOID, LPWSTR, LPVOID *, PUINT);

/**
 Pointer to the VerQueryValue function.
 */
typedef VER_QUERY_VALUEW_FN *PVER_QUERY_VALUEW_FN;

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
YoriLibCopyFileName(
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


/**
 Collect information from a directory enumerate and full file name relating
 to the file's access time.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCollectAccessTime (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
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
YoriLibCollectAllocatedRangeCount (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    HANDLE hFile;

    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->AllocatedRangeCount.HighPart = 0;
    Entry->AllocatedRangeCount.LowPart = 0;

    hFile = CreateFile(FullPath->StartOfString,
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
 Pointer to the kernel32!GetFileInformationByHandleEx export.
 */
PGET_FILE_INFORMATION_BY_HANDLE_EX_FN pGetFileInformationByHandleEx;

/**
 Collect information from a directory enumerate and full file name relating
 to the allocation size.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCollectAllocationSize (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    BOOL RealAllocSize = FALSE;

    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    if (pGetFileInformationByHandleEx == NULL) {
        HANDLE hKernel;
        hKernel = GetModuleHandle(_T("KERNEL32"));
        pGetFileInformationByHandleEx = (PGET_FILE_INFORMATION_BY_HANDLE_EX_FN)GetProcAddress(hKernel, "GetFileInformationByHandleEx");
    }

    if (pGetFileInformationByHandleEx) {

        HANDLE hFile;

        hFile = CreateFile(FullPath->StartOfString,
                           FILE_READ_ATTRIBUTES,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                           NULL);
    
        if (hFile != INVALID_HANDLE_VALUE) {
            FILE_STANDARD_INFO StandardInfo;
    
            if (pGetFileInformationByHandleEx(hFile, FileStandardInfo, &StandardInfo, sizeof(StandardInfo))) {
                Entry->AllocationSize = StandardInfo.AllocationSize;
                RealAllocSize = TRUE;
            }

            CloseHandle(hFile);
        }
    }

    if (!RealAllocSize) {
        LPTSTR FinalSeperator;
        YORI_STRING ParentPath;
        DWORD ClusterSize;

        ClusterSize = 4 * 1024;
        YoriLibInitEmptyString(&ParentPath);
        FinalSeperator = YoriLibFindRightMostCharacter(FullPath, '\\');
        if (FinalSeperator != NULL) {
            DWORD StringLength = (DWORD)(FinalSeperator - FullPath->StartOfString);
            if (YoriLibAllocateString(&ParentPath, StringLength + 1)) {
                memcpy(ParentPath.StartOfString, FullPath->StartOfString, StringLength * sizeof(TCHAR));
                ParentPath.StartOfString[StringLength] = '\0';
                ParentPath.LengthInChars = StringLength;
            }
        }

        if (ParentPath.StartOfString != NULL)  {
            DWORD BytesPerSector;
            DWORD SectorsPerCluster;
            DWORD FreeClusters;
            DWORD TotalClusters;
            GetDiskFreeSpace(ParentPath.StartOfString, &SectorsPerCluster, &BytesPerSector, &FreeClusters, &TotalClusters);

            ClusterSize = SectorsPerCluster * BytesPerSector;
            YoriLibFreeStringContents(&ParentPath);
        }

        Entry->AllocationSize.LowPart = FindData->nFileSizeLow;
        Entry->AllocationSize.HighPart = FindData->nFileSizeHigh;

        Entry->AllocationSize.QuadPart = (Entry->AllocationSize.QuadPart + ClusterSize - 1) & (~(ClusterSize - 1));
    }

    return TRUE;
}

/**
 A structure containing the core fields of a PE header.
 */
typedef struct _YORILIB_PE_HEADERS {
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
} YORILIB_PE_HEADERS, *PYORILIB_PE_HEADERS;

/**
 Helper function to load an executable's PE header for parsing.  This is used
 by multiple collection functions whose data comes from a PE header.

 @param FullPath Pointer to a string to the full file name.

 @param PeHeaders On successful completion, updated to point to the contents
        of the executable's PE headers.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCapturePeHeaders (
    __in PYORI_STRING FullPath,
    __out PYORILIB_PE_HEADERS PeHeaders
    )
{
    HANDLE hFileRead;
    IMAGE_DOS_HEADER DosHeader;
    DWORD BytesReturned;

    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    //
    //  We want the earlier handle to be attribute only so we can
    //  operate on directories, but we need data for this, so we
    //  end up with two handles.
    //

    hFileRead = CreateFile(FullPath->StartOfString,
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

            if (ReadFile(hFileRead, PeHeaders, sizeof(YORILIB_PE_HEADERS), &BytesReturned, NULL) &&
                BytesReturned == sizeof(YORILIB_PE_HEADERS) &&
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
YoriLibCollectArch (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    YORILIB_PE_HEADERS PeHeaders;

    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    UNREFERENCED_PARAMETER(FindData);

    Entry->OsVersionHigh = 0;
    Entry->OsVersionLow = 0;

    if (YoriLibCapturePeHeaders(FullPath, &PeHeaders)) {

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
YoriLibCollectCompressionAlgorithm (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    HANDLE hFile;

    UNREFERENCED_PARAMETER(FindData);

    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->CompressionAlgorithm = YoriLibCompressionNone;

    hFile = CreateFile(FullPath->StartOfString,
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
                Entry->CompressionAlgorithm = YoriLibCompressionLznt;
            } else if (NtfsCompressionAlgorithm != COMPRESSION_FORMAT_NONE) {
                Entry->CompressionAlgorithm = YoriLibCompressionNtfsUnknown;
            }
        }

        if (Entry->CompressionAlgorithm == YoriLibCompressionNone) {
            if (DeviceIoControl(hFile, FSCTL_GET_EXTERNAL_BACKING, NULL, 0, &WofInfo, sizeof(WofInfo), &BytesReturned, NULL)) {
    
                if (WofInfo.WofHeader.Provider == WOF_PROVIDER_WIM) {
                    Entry->CompressionAlgorithm = YoriLibCompressionWim;
                } else if (WofInfo.WofHeader.Provider == WOF_PROVIDER_FILE) {
                    if (WofInfo.u.FileInfo.Algorithm == FILE_PROVIDER_COMPRESSION_XPRESS4K) {
                        Entry->CompressionAlgorithm = YoriLibCompressionXpress4k;
                    } else if (WofInfo.u.FileInfo.Algorithm == FILE_PROVIDER_COMPRESSION_XPRESS8K) {
                        Entry->CompressionAlgorithm = YoriLibCompressionXpress8k;
                    } else if (WofInfo.u.FileInfo.Algorithm == FILE_PROVIDER_COMPRESSION_XPRESS16K) {
                        Entry->CompressionAlgorithm = YoriLibCompressionXpress16k;
                    } else if (WofInfo.u.FileInfo.Algorithm == FILE_PROVIDER_COMPRESSION_LZX) {
                        Entry->CompressionAlgorithm = YoriLibCompressionLzx;
                    } else {
                        Entry->CompressionAlgorithm = YoriLibCompressionWofFileUnknown;
                    }
                } else {
                    Entry->CompressionAlgorithm = YoriLibCompressionWofUnknown;
                }
            }
        }

        CloseHandle(hFile);
    }
    return TRUE;
}

/**
 Pointer to the kernel32!GetCompressedFileSizeW export.
 */
PGET_COMPRESSED_FILE_SIZE_FN pGetCompressedFileSizeW;

/**
 Collect information from a directory enumerate and full file name relating
 to the file's compression size.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCollectCompressedFileSize (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    ASSERT(YoriLibIsStringNullTerminated(FullPath));
    Entry->CompressedFileSize.LowPart = FindData->nFileSizeLow;
    Entry->CompressedFileSize.HighPart = FindData->nFileSizeHigh;

    if (pGetCompressedFileSizeW == NULL) {
        HANDLE hKernel;
        hKernel = GetModuleHandle(_T("KERNEL32"));
        pGetCompressedFileSizeW = (PGET_COMPRESSED_FILE_SIZE_FN)GetProcAddress(hKernel, "GetCompressedFileSizeW");
    }

    if (pGetCompressedFileSizeW) {
        Entry->CompressedFileSize.LowPart = pGetCompressedFileSizeW(FullPath->StartOfString, (PDWORD)&Entry->CompressedFileSize.HighPart);

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
YoriLibCollectCreateTime (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
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
YoriLibCollectEffectivePermissions (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
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
    BOOL AccessGranted;
    GENERIC_MAPPING Mapping = {0};
    PRIVILEGE_SET Privilege;
    DWORD PrivilegeLength = sizeof(Privilege);

    UNREFERENCED_PARAMETER(FindData);

    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->EffectivePermissions = 0;

    SecurityDescriptor = LocalSecurityDescriptor;

    if (!GetFileSecurity(FullPath->StartOfString, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)SecurityDescriptor, sizeof(LocalSecurityDescriptor), &dwSdRequired)) {
        if (dwSdRequired != 0) {
            SecurityDescriptor = YoriLibMalloc(dwSdRequired);
    
            if (!GetFileSecurity(FullPath->StartOfString, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)SecurityDescriptor, dwSdRequired, &dwSdRequired)) {
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
YoriLibCollectFileAttributes (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    UNREFERENCED_PARAMETER(FullPath);

    Entry->FileAttributes = FindData->dwFileAttributes;
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
YoriLibCollectFileExtension (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
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
YoriLibCollectFileId (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    HANDLE hFile;

    UNREFERENCED_PARAMETER(FindData);
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->FileId.QuadPart = 0;

    hFile = CreateFile(FullPath->StartOfString,
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
YoriLibCollectFileName (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    UNREFERENCED_PARAMETER(FullPath);

    YoriLibCopyFileName(Entry->FileName, FindData->cFileName, MAX_PATH - 1, &Entry->FileNameLengthInChars);

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
YoriLibCollectFileSize (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
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
YoriLibCollectFragmentCount (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    HANDLE hFile;

    UNREFERENCED_PARAMETER(FindData);

    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->FragmentCount.HighPart = 0;
    Entry->FragmentCount.LowPart = 0;

    hFile = CreateFile(FullPath->StartOfString,
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
YoriLibCollectLinkCount (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    HANDLE hFile;

    UNREFERENCED_PARAMETER(FindData);
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->LinkCount = 0;

    hFile = CreateFile(FullPath->StartOfString,
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
YoriLibCollectObjectId (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    HANDLE hFile;
    FILE_OBJECTID_BUFFER Buffer;
    DWORD BytesReturned;

    UNREFERENCED_PARAMETER(FindData);
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    ZeroMemory(&Entry->ObjectId, sizeof(Entry->ObjectId));

    hFile = CreateFile(FullPath->StartOfString,
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
YoriLibCollectOsVersion (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    YORILIB_PE_HEADERS PeHeaders;

    UNREFERENCED_PARAMETER(FindData);
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->OsVersionHigh = 0;
    Entry->OsVersionLow = 0;

    if (YoriLibCapturePeHeaders(FullPath, &PeHeaders)) {

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
YoriLibCollectOwner (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
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
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    UserName[0] = '\0';
    Entry->Owner[0] = '\0';

    if (GetFileSecurity(FullPath->StartOfString, OWNER_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)SecurityDescriptor, sizeof(SecurityDescriptor), &dwSdRequired)) {
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
YoriLibCollectReparseTag (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
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
YoriLibCollectShortName (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    UNREFERENCED_PARAMETER(FullPath);

    if (FindData->cAlternateFileName[0] == '\0') {
        DWORD FileNameLength = (DWORD)_tcslen(FindData->cFileName);

        if (FileNameLength <= 12) {
            YoriLibCopyFileName(Entry->ShortFileName, FindData->cFileName, sizeof(Entry->ShortFileName), NULL);
        } else {
            Entry->ShortFileName[0] = '\0';
        }
    } else {
        YoriLibCopyFileName(Entry->ShortFileName, FindData->cAlternateFileName, sizeof(Entry->ShortFileName), NULL);
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
YoriLibCollectSubsystem (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    YORILIB_PE_HEADERS PeHeaders;

    UNREFERENCED_PARAMETER(FindData);
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->Subsystem = 0;

    if (YoriLibCapturePeHeaders(FullPath, &PeHeaders)) {

        Entry->Subsystem = PeHeaders.OptionalHeader.Subsystem;
    }

    return TRUE;
}

/**
 Pointer to the kernel32!FindFirstStreamW export.
 */
PFIND_FIRST_STREAM_FN pFindFirstStreamW;

/**
 Pointer to the kernel32!FindNextStreamW export.
 */
PFIND_NEXT_STREAM_FN pFindNextStreamW;

/**
 Collect information from a directory enumerate and full file name relating
 to the file's stream count.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCollectStreamCount (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    HANDLE hFind;
    WIN32_FIND_STREAM_DATA FindStreamData;

    UNREFERENCED_PARAMETER(FindData);
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->StreamCount = 0;
    
    if (pFindFirstStreamW == NULL || pFindNextStreamW == NULL) {
        HANDLE hKernel;
        hKernel = GetModuleHandle(_T("KERNEL32"));
        pFindFirstStreamW = (PFIND_FIRST_STREAM_FN)GetProcAddress(hKernel, "FindFirstStreamW");
        pFindNextStreamW = (PFIND_NEXT_STREAM_FN)GetProcAddress(hKernel, "FindNextStreamW");
    }

    //
    //  These APIs are Unicode only.  We could do an ANSI to Unicode thunk here, but
    //  since Unicode is the default build and ANSI is only useful for older systems
    //  (where this API won't exist) there doesn't seem much point.
    //

    if (pFindFirstStreamW && pFindNextStreamW) {
        hFind = pFindFirstStreamW(FullPath->StartOfString, 0, &FindStreamData, 0);
        if (hFind != INVALID_HANDLE_VALUE) {

            do {
                Entry->StreamCount++;
            } while (pFindNextStreamW(hFind, &FindStreamData));
        }

        FindClose(hFind);
    }

    return TRUE;
}

/**
 Collect information from a directory enumerate and full file name relating
 to the file's USN.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCollectUsn (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    HANDLE hFile;

    UNREFERENCED_PARAMETER(FindData);
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->Usn.QuadPart = 0;
    hFile = CreateFile(FullPath->StartOfString,
                       FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {

        struct {
            USN_RECORD UsnRecord;
            WCHAR FileName[YORI_LIB_MAX_FILE_NAME];
        } s1;
        DWORD BytesReturned;

        if (DeviceIoControl(hFile, FSCTL_READ_FILE_USN_DATA, NULL, 0, &s1, sizeof(s1), &BytesReturned, NULL)) {
            Entry->Usn.QuadPart = s1.UsnRecord.Usn;
        }

        CloseHandle(hFile);
    }
    return TRUE;
}

/**
 Pointer to the version!GetFileVersionInfoSizeW export.
 */
PGET_FILE_VERSION_INFO_SIZEW_FN pGetFileVersionInfoSize;

/**
 Pointer to the version!GetFileVersionInfoW export.
 */
PGET_FILE_VERSION_INFOW_FN pGetFileVersionInfo;

/**
 Pointer to the version!VerQueryValueW export.
 */
PVER_QUERY_VALUEW_FN pVerQueryValue;

/**
 Collect information from a directory enumerate and full file name relating
 to the executable's version resource.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCollectVersion (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    DWORD Junk;
    PVOID Buffer;
    DWORD VerSize;
    VS_FIXEDFILEINFO * RootBlock;

    UNREFERENCED_PARAMETER(FindData);
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->FileVersion.QuadPart = 0;
    Entry->FileVersionFlags = 0;

    if (pGetFileVersionInfoSize == NULL) {
        HINSTANCE hVerDll;

        hVerDll = LoadLibrary(_T("VERSION.DLL"));
        pGetFileVersionInfoSize = (PGET_FILE_VERSION_INFO_SIZEW_FN)GetProcAddress(hVerDll, "GetFileVersionInfoSizeW");
        pGetFileVersionInfo = (PGET_FILE_VERSION_INFOW_FN)GetProcAddress(hVerDll, "GetFileVersionInfoW");
        pVerQueryValue = (PVER_QUERY_VALUEW_FN)GetProcAddress(hVerDll, "VerQueryValueW");

        if (pGetFileVersionInfoSize == NULL ||
            pGetFileVersionInfo == NULL ||
            pVerQueryValue == NULL) {

            FreeLibrary(hVerDll);
            return TRUE;
        }
    }

    VerSize = pGetFileVersionInfoSize(FullPath->StartOfString, &Junk);

    Buffer = YoriLibMalloc(VerSize);
    if (Buffer != NULL) {
        if (pGetFileVersionInfo(FullPath->StartOfString, 0, VerSize, Buffer)) {
            if (pVerQueryValue(Buffer, _T("\\"), (PVOID*)&RootBlock, (PUINT)&Junk)) {
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
YoriLibCollectWriteTime(
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    FILETIME tmp;

    UNREFERENCED_PARAMETER(FullPath);

    FileTimeToLocalFileTime(&FindData->ftLastWriteTime, &tmp);
    FileTimeToSystemTime(&tmp, &Entry->WriteTime);
    return TRUE;
}

// vim:sw=4:ts=4:et:
