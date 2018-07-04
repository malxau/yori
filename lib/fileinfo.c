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

    if (DllKernel32.pGetFileInformationByHandleEx) {

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
    
            if (DllKernel32.pGetFileInformationByHandleEx(hFile, FileStandardInfo, &StandardInfo, sizeof(StandardInfo))) {
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
 Returns TRUE if the executable is a GUI executable.  If it's not a PE, or
 any error occurs, or it's any other subsystem, it's assumed to not be 
 a GUI executable.

 @param FullPath Path to the executable file to check for whether it's
        GUI.

 @return TRUE if the executable is GUI, FALSE if it is not or indeterminate.
 */
BOOL
YoriLibIsExecutableGui(
    __in PYORI_STRING FullPath
    )
{
    YORILIB_PE_HEADERS PeHeaders;

    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    if (YoriLibCapturePeHeaders(FullPath, &PeHeaders)) {
        if (PeHeaders.OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) {
            return TRUE;
        }
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

    if (DllKernel32.pGetCompressedFileSizeW) {
        Entry->CompressedFileSize.LowPart = DllKernel32.pGetCompressedFileSizeW(FullPath->StartOfString, (PDWORD)&Entry->CompressedFileSize.HighPart);

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
 to the executable's version resource's file description.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCollectDescription (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    DWORD Junk;
    PVOID Buffer;
    DWORD VerSize;
    PWORD TranslationBlock;

    UNREFERENCED_PARAMETER(FindData);
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->Description[0] = '\0';

    YoriLibLoadVersionFunctions();

    if (DllVersion.pGetFileVersionInfoSizeW == NULL ||
        DllVersion.pGetFileVersionInfoW == NULL ||
        DllVersion.pVerQueryValueW == NULL) {

        return TRUE;
    }

    VerSize = DllVersion.pGetFileVersionInfoSizeW(FullPath->StartOfString, &Junk);

    Buffer = YoriLibMalloc(VerSize);
    if (Buffer != NULL) {
        if (DllVersion.pGetFileVersionInfoW(FullPath->StartOfString, 0, VerSize, Buffer)) {
            if (DllVersion.pVerQueryValueW(Buffer, _T("\\VarFileInfo\\Translation"), (PVOID*)&TranslationBlock, (PUINT)&Junk) && sizeof(Junk) >= 2 * sizeof(WORD)) {

                TCHAR LanguageBlockToFind[sizeof("\\StringFileInfo\\01234567\\FileDescription")];
                LPTSTR Description;

                YoriLibSPrintf(LanguageBlockToFind, _T("\\StringFileInfo\\%04x%04x\\FileDescription"), TranslationBlock[0], TranslationBlock[1]);
                if (DllVersion.pVerQueryValueW(Buffer, LanguageBlockToFind, (PVOID*)&Description, (PUINT)&Junk)) {
                    DWORD BytesToCopy = Junk * sizeof(TCHAR);
                    if (BytesToCopy > sizeof(Entry->Description) - sizeof(TCHAR)) {
                        BytesToCopy = sizeof(Entry->Description) - sizeof(TCHAR);
                    }
                    memcpy(Entry->Description, Description, BytesToCopy);
                    Entry->Description[BytesToCopy / sizeof(TCHAR)] = '\0';
                }
            }
        }
        YoriLibFree(Buffer);
    }
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

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pGetFileSecurityW == NULL ||
        DllAdvApi32.pImpersonateSelf == NULL ||
        DllAdvApi32.pOpenThreadToken == NULL ||
        DllAdvApi32.pAccessCheck == NULL ||
        DllAdvApi32.pRevertToSelf == NULL) {

        return FALSE;
    }

    Entry->EffectivePermissions = 0;

    SecurityDescriptor = LocalSecurityDescriptor;

    if (!DllAdvApi32.pGetFileSecurityW(FullPath->StartOfString, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)SecurityDescriptor, sizeof(LocalSecurityDescriptor), &dwSdRequired)) {
        if (dwSdRequired != 0) {
            SecurityDescriptor = YoriLibMalloc(dwSdRequired);
    
            if (!DllAdvApi32.pGetFileSecurityW(FullPath->StartOfString, OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)SecurityDescriptor, dwSdRequired, &dwSdRequired)) {
                goto Exit;
            }
        } else {
            goto Exit;
        }
    }

    if (!DllAdvApi32.pImpersonateSelf(SecurityIdentification)) {
        goto Exit;
    }
    if (!DllAdvApi32.pOpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &TokenHandle)) {
        DllAdvApi32.pRevertToSelf();
        goto Exit;
    }

    DllAdvApi32.pAccessCheck((PSECURITY_DESCRIPTOR)SecurityDescriptor, TokenHandle, MAXIMUM_ALLOWED, &Mapping, &Privilege, &PrivilegeLength, &Entry->EffectivePermissions, &AccessGranted);

Exit:
    if (TokenHandle != NULL) {
        CloseHandle(TokenHandle);
        DllAdvApi32.pRevertToSelf();
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
 to the executable's version resource's file version string.

 @param Entry The directory entry to populate.

 @param FindData The directory enumeration information.

 @param FullPath Pointer to a string to the full file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCollectFileVersionString (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    )
{
    DWORD Junk;
    PVOID Buffer;
    DWORD VerSize;
    PWORD TranslationBlock;

    UNREFERENCED_PARAMETER(FindData);
    ASSERT(YoriLibIsStringNullTerminated(FullPath));

    Entry->FileVersionString[0] = '\0';

    YoriLibLoadVersionFunctions();

    if (DllVersion.pGetFileVersionInfoSizeW == NULL ||
        DllVersion.pGetFileVersionInfoW == NULL ||
        DllVersion.pVerQueryValueW == NULL) {

        return TRUE;
    }

    VerSize = DllVersion.pGetFileVersionInfoSizeW(FullPath->StartOfString, &Junk);

    Buffer = YoriLibMalloc(VerSize);
    if (Buffer != NULL) {
        if (DllVersion.pGetFileVersionInfoW(FullPath->StartOfString, 0, VerSize, Buffer)) {
            if (DllVersion.pVerQueryValueW(Buffer, _T("\\VarFileInfo\\Translation"), (PVOID*)&TranslationBlock, (PUINT)&Junk) && sizeof(Junk) >= 2 * sizeof(WORD)) {

                TCHAR LanguageBlockToFind[sizeof("\\StringFileInfo\\01234567\\FileVersion")];
                LPTSTR FileVersionString;

                YoriLibSPrintf(LanguageBlockToFind, _T("\\StringFileInfo\\%04x%04x\\FileVersion"), TranslationBlock[0], TranslationBlock[1]);
                if (DllVersion.pVerQueryValueW(Buffer, LanguageBlockToFind, (PVOID*)&FileVersionString, (PUINT)&Junk)) {
                    DWORD BytesToCopy = Junk * sizeof(TCHAR);
                    if (BytesToCopy > sizeof(Entry->FileVersionString) - sizeof(TCHAR)) {
                        BytesToCopy = sizeof(Entry->FileVersionString) - sizeof(TCHAR);
                    }
                    memcpy(Entry->FileVersionString, FileVersionString, BytesToCopy);
                    Entry->FileVersionString[BytesToCopy / sizeof(TCHAR)] = '\0';
                }
            }
        }
        YoriLibFree(Buffer);
    }
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

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pGetFileSecurityW == NULL ||
        DllAdvApi32.pGetSecurityDescriptorOwner == NULL ||
        DllAdvApi32.pLookupAccountSidW == NULL) {

        return FALSE;
    }

    UserName[0] = '\0';
    Entry->Owner[0] = '\0';

    if (DllAdvApi32.pGetFileSecurityW(FullPath->StartOfString, OWNER_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)SecurityDescriptor, sizeof(SecurityDescriptor), &dwSdRequired)) {
        if (DllAdvApi32.pGetSecurityDescriptorOwner((PSECURITY_DESCRIPTOR)SecurityDescriptor, &pOwnerSid, &OwnerDefaulted)) {
            if (DllAdvApi32.pLookupAccountSidW(NULL, pOwnerSid, UserName, &NameLength, DomainName, &DomainLength, &eUse)) {
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
    
    //
    //  These APIs are Unicode only.  We could do an ANSI to Unicode thunk here, but
    //  since Unicode is the default build and ANSI is only useful for older systems
    //  (where this API won't exist) there doesn't seem much point.
    //

    if (DllKernel32.pFindFirstStreamW != NULL && DllKernel32.pFindNextStreamW != NULL) {
        hFind = DllKernel32.pFindFirstStreamW(FullPath->StartOfString, 0, &FindStreamData, 0);
        if (hFind != INVALID_HANDLE_VALUE) {

            do {
                Entry->StreamCount++;
            } while (DllKernel32.pFindNextStreamW(hFind, &FindStreamData));
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

    YoriLibLoadVersionFunctions();

    if (DllVersion.pGetFileVersionInfoSizeW == NULL ||
        DllVersion.pGetFileVersionInfoW == NULL ||
        DllVersion.pVerQueryValueW == NULL) {

        return TRUE;
    }

    VerSize = DllVersion.pGetFileVersionInfoSizeW(FullPath->StartOfString, &Junk);

    Buffer = YoriLibMalloc(VerSize);
    if (Buffer != NULL) {
        if (DllVersion.pGetFileVersionInfoW(FullPath->StartOfString, 0, VerSize, Buffer)) {
            if (DllVersion.pVerQueryValueW(Buffer, _T("\\"), (PVOID*)&RootBlock, (PUINT)&Junk)) {
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


//
//  When criteria are specified to apply attributes, we need to load the
//  specification into a dummy dirent to perform comparisons against.  The
//  below functions implement these.
//

/**
 Parse a string and populate a directory entry to facilitate comparisons for
 last access date.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateAccessDate(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    return YoriLibStringToDate(String, &Entry->AccessTime, NULL);
}

/**
 Parse a string and populate a directory entry to facilitate comparisons for
 last access time.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateAccessTime(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    return YoriLibStringToTime(String, &Entry->AccessTime);
}

/**
 Parse a string and populate a directory entry to facilitate comparisons for
 the number of allocated ranges.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateAllocatedRangeCount(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    DWORD CharsConsumed;
    YoriLibStringToNumber(String, TRUE, &Entry->AllocatedRangeCount.QuadPart, &CharsConsumed);
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's allocation size.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateAllocationSize(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    Entry->AllocationSize = YoriLibStringToFileSize(String);
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for an executable's CPU architecture.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateArch(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    if (YoriLibCompareStringWithLiteralInsensitive(String, _T("None")) == 0) {
        Entry->Architecture = 0;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("i386")) == 0) {
        Entry->Architecture = IMAGE_FILE_MACHINE_I386;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("amd64")) == 0) {
        Entry->Architecture = IMAGE_FILE_MACHINE_AMD64;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("arm")) == 0) {
        Entry->Architecture = IMAGE_FILE_MACHINE_ARMNT;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("arm64")) == 0) {
        Entry->Architecture = IMAGE_FILE_MACHINE_ARM64;
    } else {
        return FALSE;
    }
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's compression algorithm.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateCompressionAlgorithm(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    if (YoriLibCompareStringWithLiteralInsensitive(String, _T("None")) == 0) {
        Entry->CompressionAlgorithm = YoriLibCompressionNone;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("LZNT")) == 0) {
        Entry->CompressionAlgorithm = YoriLibCompressionLznt;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("NTFS")) == 0) {
        Entry->CompressionAlgorithm = YoriLibCompressionNtfsUnknown;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("WIM")) == 0) {
        Entry->CompressionAlgorithm = YoriLibCompressionWim;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("LZX")) == 0) {
        Entry->CompressionAlgorithm = YoriLibCompressionLzx;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("Xp4")) == 0) {
        Entry->CompressionAlgorithm = YoriLibCompressionXpress4k;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("Xp8")) == 0) {
        Entry->CompressionAlgorithm = YoriLibCompressionXpress8k;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("Xp16")) == 0) {
        Entry->CompressionAlgorithm = YoriLibCompressionXpress16k;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("File")) == 0) {
        Entry->CompressionAlgorithm = YoriLibCompressionWofFileUnknown;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("Wof")) == 0) {
        Entry->CompressionAlgorithm = YoriLibCompressionWofUnknown;
    } else {
        return FALSE;
    }
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's compressed file size.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateCompressedFileSize(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    Entry->CompressedFileSize = YoriLibStringToFileSize(String);
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's creation date.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateCreateDate(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    return YoriLibStringToDate(String, &Entry->CreateTime, NULL);
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's creation time.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateCreateTime(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    return YoriLibStringToTime(String, &Entry->CreateTime);
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's version description.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateDescription(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    YoriLibSPrintfS(Entry->Description, sizeof(Entry->Description)/sizeof(Entry->Description[0]), _T("%y"), String);
    Entry->Description[sizeof(Entry->Description)/sizeof(Entry->Description[0]) - 1] = '\0';

    return TRUE;
}


/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's extension.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateFileExtension(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    //
    //  Since we have one dirent per comparison, just shove the extension in
    //  the file name buffer and point the extension to it.  This buffer can't
    //  be used for anything else anyway.
    //

    YoriLibSPrintfS(Entry->FileName, sizeof(Entry->FileName)/sizeof(Entry->FileName[0]), _T("%y"), String);
    Entry->FileName[sizeof(Entry->FileName)/sizeof(Entry->FileName[0]) - 1] = '\0';
    Entry->Extension = Entry->FileName;

    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's name.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateFileName(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    Entry->FileNameLengthInChars = YoriLibSPrintfS(Entry->FileName, sizeof(Entry->FileName)/sizeof(Entry->FileName[0]), _T("%y"), String);
    Entry->FileName[sizeof(Entry->FileName)/sizeof(Entry->FileName[0]) - 1] = '\0';

    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's size.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateFileSize(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    Entry->FileSize = YoriLibStringToFileSize(String);
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's version string.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateFileVersionString(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    YoriLibSPrintfS(Entry->FileVersionString, sizeof(Entry->FileVersionString)/sizeof(Entry->FileVersionString[0]), _T("%y"), String);
    Entry->FileVersionString[sizeof(Entry->FileVersionString)/sizeof(Entry->FileVersionString[0]) - 1] = '\0';

    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's fragment count.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateFragmentCount(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    DWORD CharsConsumed;
    YoriLibStringToNumber(String, TRUE, &Entry->FragmentCount.QuadPart, &CharsConsumed);
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's link count.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateLinkCount(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    DWORD CharsConsumed;
    LONGLONG llTemp;
    YoriLibStringToNumber(String, TRUE, &llTemp, &CharsConsumed);
    Entry->LinkCount = (DWORD)llTemp;
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's object ID.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateObjectId(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    UCHAR Buffer[16];
    if (YoriLibStringToHexBuffer(String, (PUCHAR)&Buffer, sizeof(Buffer))) {
        memcpy(Entry->ObjectId, Buffer, sizeof(Buffer));
    }
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for an executable's minimum OS version.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateOsVersion(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
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

    Entry->OsVersionHigh = (WORD)llTemp;

    if (CharsConsumed < Substring.LengthInChars && Substring.StartOfString[CharsConsumed] == '.') {
        Substring.LengthInChars -= CharsConsumed + 1;
        Substring.StartOfString += CharsConsumed + 1;

        if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed)) {
            return FALSE;
        }

        Entry->OsVersionLow = (WORD)llTemp;
    }

    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's owner.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateOwner(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    YoriLibSPrintfS(Entry->Owner, sizeof(Entry->Owner)/sizeof(Entry->Owner[0]), _T("%y"), String);
    Entry->Owner[sizeof(Entry->Owner)/sizeof(Entry->Owner[0]) - 1] = '\0';

    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's reparse tag.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateReparseTag(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    DWORD CharsConsumed;
    LONGLONG llTemp;
    YoriLibStringToNumber(String, TRUE, &llTemp, &CharsConsumed);
    Entry->ReparseTag = (DWORD)llTemp;
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's short file name.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateShortName(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    YoriLibSPrintfS(Entry->ShortFileName, sizeof(Entry->ShortFileName)/sizeof(Entry->ShortFileName[0]), _T("%y"), String);
    Entry->ShortFileName[sizeof(Entry->ShortFileName)/sizeof(Entry->ShortFileName[0]) - 1] = '\0';

    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for an executable's target subsystem.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateSubsystem (
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    if (YoriLibCompareStringWithLiteralInsensitive(String, _T("None")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_UNKNOWN;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("NT")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_NATIVE;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("GUI")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("Cons")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("OS/2")) == 0 || YoriLibCompareStringWithLiteralInsensitive(String, _T("OS2")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_OS2_CUI;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("Posx")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_POSIX_CUI;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("w9x")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_NATIVE_WINDOWS;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("CE")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CE_GUI;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("EFIa")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_EFI_APPLICATION;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("EFIb")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("EFId")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("EFIr")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_EFI_ROM;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("Xbox")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_XBOX;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("Xbcc")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("Boot")) == 0) {
        Entry->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION;
    } else {
        return FALSE;
    }

    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's stream count.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateStreamCount(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    DWORD CharsConsumed;
    LONGLONG llTemp;
    YoriLibStringToNumber(String, TRUE, &llTemp, &CharsConsumed);
    Entry->StreamCount = (DWORD)llTemp;
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's USN.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateUsn(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    DWORD CharsConsumed;
    YoriLibStringToNumber(String, TRUE, &Entry->Usn.QuadPart, &CharsConsumed);
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for an executable's version.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateVersion(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    LARGE_INTEGER FileVersion = {0};
    YORI_STRING Substring;
    DWORD CharsConsumed;
    LONGLONG llTemp;

    YoriLibInitEmptyString(&Substring);
    Substring.StartOfString = String->StartOfString;
    Substring.LengthInChars = String->LengthInChars;

    if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed)) {
        return FALSE;
    }

    FileVersion.HighPart = (DWORD)llTemp << 16;

    if (CharsConsumed < Substring.LengthInChars && Substring.StartOfString[CharsConsumed] == '.') {
        Substring.LengthInChars -= CharsConsumed + 1;
        Substring.StartOfString += CharsConsumed + 1;

        if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed)) {
            return FALSE;
        }

        FileVersion.HighPart = FileVersion.HighPart + (WORD)llTemp;

        if (CharsConsumed < Substring.LengthInChars && Substring.StartOfString[CharsConsumed] == '.') {
            Substring.LengthInChars -= CharsConsumed + 1;
            Substring.StartOfString += CharsConsumed + 1;
    
            if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed)) {
                return FALSE;
            }

            FileVersion.LowPart = (DWORD)llTemp << 16;

            if (CharsConsumed < Substring.LengthInChars && Substring.StartOfString[CharsConsumed] == '.') {
                Substring.LengthInChars -= CharsConsumed + 1;
                Substring.StartOfString += CharsConsumed + 1;
        
                if (!YoriLibStringToNumber(&Substring, TRUE, &llTemp, &CharsConsumed)) {
                    return FALSE;
                }

                FileVersion.LowPart = FileVersion.LowPart + (WORD)llTemp;
            }

        }
    }

    Entry->FileVersion = FileVersion;
    return TRUE;
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's write date.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateWriteDate(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    return YoriLibStringToDate(String, &Entry->WriteTime, NULL);
}

/**
 Parse a string and populate a directory entry to facilitate
 comparisons for a file's write time.

 @param Entry The directory entry to populate from the string.

 @param String Pointer to a string to use to populate the
        directory entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateWriteTime(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    )
{
    return YoriLibStringToTime(String, &Entry->WriteTime);
}


// vim:sw=4:ts=4:et:
