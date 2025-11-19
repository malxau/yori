/**
 * @file lib/ylvolpth.c
 *
 * Volume enumeration and information routines.
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
 Return the volume name of the volume that is hosting a particular file.  This
 is normally done via the Win32 GetVolumePathName API, which was added in
 Windows 2000 to support mount points; on older versions this behavior is
 emulated by returning the drive letter or UNC share name.

 @param FileName Pointer to the file name to obtain the volume for.

 @param VolumeName On successful completion, populated with a path to the
        volume name.  This string is expected to be initialized on entry and
        may be reallocated within this routine.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibGetVolumePathName(
    __in PYORI_STRING FileName,
    __inout PYORI_STRING VolumeName
    )
{
    BOOL FreeOnFailure = FALSE;
    ASSERT(YoriLibIsStringNullTerminated(FileName));

    //
    //  This function expects a full/escaped path, because Win32 has no way
    //  to determine the buffer length if it's anything else.
    //

    if (FileName->LengthInChars < 4 ||
        !YoriLibIsSep(FileName->StartOfString[0]) ||
        !YoriLibIsSep(FileName->StartOfString[1]) ||
        (FileName->StartOfString[2] != '?' && FileName->StartOfString[2] != '.') ||
        !YoriLibIsSep(FileName->StartOfString[3])) {

        return FALSE;
    }

    //
    //  The volume name can be as long as the file name, plus a NULL
    //  terminator.
    //

    if (VolumeName->LengthAllocated <= FileName->LengthInChars) {
        YoriLibFreeStringContents(VolumeName);
        if (!YoriLibAllocateString(VolumeName, FileName->LengthInChars + 1)) {
            return FALSE;
        }
        FreeOnFailure = TRUE;
    }

    //
    //  If Win32 support exists, use it.
    //

    if (DllKernel32.pGetVolumePathNameW != NULL) {
        if (!DllKernel32.pGetVolumePathNameW(FileName->StartOfString, VolumeName->StartOfString, VolumeName->LengthAllocated)) {
            if (FreeOnFailure) {
                YoriLibFreeStringContents(VolumeName);
            }
            return FALSE;
        }
        VolumeName->LengthInChars = (YORI_ALLOC_SIZE_T)_tcslen(VolumeName->StartOfString);

        //
        //  For some reason Windows doesn't add the prefix to this string,
        //  which is really broken - a volume name of "C:" is not a volume
        //  name, it's a reference to a current directory.
        //

        if (!YoriLibIsPathPrefixed(VolumeName)) {
            YORI_STRING EscapedVolumeName;
            YoriLibInitEmptyString(&EscapedVolumeName);
            if (!YoriLibGetFullPathNameAlloc(VolumeName, TRUE, &EscapedVolumeName, NULL)) {
                if (FreeOnFailure) {
                    YoriLibFreeStringContents(VolumeName);
                }
                return FALSE;
            }

            //
            //  If it fits in the existing allocation, use that.  This allows
            //  the caller to supply an appropriately sized buffer and
            //  expect the result in that buffer.
            //

            if (EscapedVolumeName.LengthInChars < VolumeName->LengthAllocated) {
                memcpy(VolumeName->StartOfString, EscapedVolumeName.StartOfString, EscapedVolumeName.LengthInChars * sizeof(TCHAR));
                VolumeName->LengthInChars = EscapedVolumeName.LengthInChars;
                VolumeName->StartOfString[VolumeName->LengthInChars] = '\0';
            } else {
                YoriLibFreeStringContents(VolumeName);
                YoriLibCloneString(VolumeName, &EscapedVolumeName);
                FreeOnFailure = TRUE;
            }

            YoriLibFreeStringContents(&EscapedVolumeName);
        }

        //
        //  If it ends in a backslash, truncate it
        //

        if (VolumeName->LengthInChars > 0 &&
            YoriLibIsSep(VolumeName->StartOfString[VolumeName->LengthInChars - 1])) {

            VolumeName->LengthInChars--;
            VolumeName->StartOfString[VolumeName->LengthInChars] = '\0';
        }
        return TRUE;
    }

    //
    //  If Win32 support doesn't exist, we know that mount points can't
    //  exist so we can return only the drive letter path, or the UNC
    //  path with server and share.
    //

    if (!YoriLibIsFullPathUnc(FileName)) {
        if (FileName->LengthInChars >= 6) {
            memcpy(VolumeName->StartOfString, FileName->StartOfString, 6 * sizeof(TCHAR));
            VolumeName->StartOfString[6] = '\0';
            VolumeName->LengthInChars = 6;
            return TRUE;
        }
    } else {
        if (FileName->LengthInChars >= sizeof("\\\\?\\UNC\\")) {
            YORI_STRING Subset;
            LPTSTR Slash;
            YORI_ALLOC_SIZE_T CharsToCopy;

            YoriLibInitEmptyString(&Subset);
            Subset.StartOfString = FileName->StartOfString + sizeof("\\\\?\\UNC\\");
            Subset.LengthInChars = FileName->LengthInChars - sizeof("\\\\?\\UNC\\");

            Slash = YoriLibFindLeftMostCharacter(&Subset, '\\');
            if (Slash != NULL) {
                Subset.LengthInChars = Subset.LengthInChars - (YORI_ALLOC_SIZE_T)(Slash - Subset.StartOfString);
                Subset.StartOfString = Slash;

                if (Subset.LengthInChars > 0) {
                    Subset.LengthInChars--;
                    Subset.StartOfString++;
                    Slash = YoriLibFindLeftMostCharacter(&Subset, '\\');
                    if (Slash != NULL) {
                        CharsToCopy = (YORI_ALLOC_SIZE_T)(Slash - FileName->StartOfString);
                    } else {
                        CharsToCopy = (YORI_ALLOC_SIZE_T)(Subset.StartOfString - FileName->StartOfString) + Subset.LengthInChars;
                    }

                    memcpy(VolumeName->StartOfString, FileName->StartOfString, CharsToCopy * sizeof(TCHAR));
                    VolumeName->StartOfString[CharsToCopy] = '\0';
                    VolumeName->LengthInChars = CharsToCopy;
                    return TRUE;
                }
            }
        }
    }

    if (FreeOnFailure) {
        YoriLibFreeStringContents(VolumeName);
    }
    VolumeName->LengthInChars = 0;
    return FALSE;
}

/**
 Determine if the specified directory supports long file names.

 @param PathName Pointer to the directory to check.

 @param LongNameSupport On successful completion, updated to TRUE to indicate
        long name support, FALSE to indicate no long name support.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriLibPathSupportsLongNames(
    __in PCYORI_STRING PathName,
    __out PBOOLEAN LongNameSupport
    )
{
    YORI_STRING VolumeLabel;
    YORI_STRING FsName;
    YORI_STRING FullPathName;
    YORI_STRING VolRootName;
    DWORD ShortSerialNumber;
    DWORD Capabilities;
    DWORD MaxComponentLength;
    BOOLEAN Result;

    YoriLibInitEmptyString(&VolumeLabel);
    YoriLibInitEmptyString(&FsName);
    YoriLibInitEmptyString(&FullPathName);
    YoriLibInitEmptyString(&VolRootName);
    Result = FALSE;

    if (!YoriLibAllocateString(&VolumeLabel, 256)) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&FsName, 256)) {
        goto Exit;
    }

    if (!YoriLibUserToSingleFilePath(PathName, TRUE, &FullPathName)) {
        goto Exit;
    }

    //
    //  We want to translate the user specified path into a volume root.
    //  Windows 2000 and above have a nice API for this, which says it's
    //  guaranteed to return less than or equal to the size of the input
    //  string, so we allocate the input string, plus space for a trailing
    //  backslash and a NULL terminator.
    //

    if (!YoriLibAllocateString(&VolRootName, FullPathName.LengthInChars + 2)) {
        goto Exit;
    }

    if (!YoriLibGetVolumePathName(&FullPathName, &VolRootName)) {
        goto Exit;
    }

    //
    //  GetVolumeInformation wants a name with a trailing backslash.  Add one
    //  if needed.
    //

    if (VolRootName.LengthInChars > 0 &&
        VolRootName.LengthInChars + 1 < VolRootName.LengthAllocated &&
        VolRootName.StartOfString[VolRootName.LengthInChars - 1] != '\\') {

        VolRootName.StartOfString[VolRootName.LengthInChars] = '\\';
        VolRootName.StartOfString[VolRootName.LengthInChars + 1] = '\0';
        VolRootName.LengthInChars++;
    }

    if (GetVolumeInformation(VolRootName.StartOfString,
                             VolumeLabel.StartOfString,
                             VolumeLabel.LengthAllocated,
                             &ShortSerialNumber,
                             &MaxComponentLength,
                             &Capabilities,
                             FsName.StartOfString,
                             FsName.LengthAllocated)) {

        Result = TRUE;
        if (MaxComponentLength >= 255) {
            *LongNameSupport = TRUE;
        } else {
            *LongNameSupport = FALSE;
        }
    }

Exit:
    YoriLibFreeStringContents(&FullPathName);
    YoriLibFreeStringContents(&VolRootName);
    YoriLibFreeStringContents(&FsName);
    YoriLibFreeStringContents(&VolumeLabel);

    return Result;
}


/**
 Context structure used to preserve state about the next volume to return
 when a native platform implementation of FindFirstVolume et al are not
 available.
 */
typedef struct _YORI_LIB_FIND_VOLUME_CONTEXT {

    /**
     Indicates the number of the drive to probe on the next call to
     @ref YoriLibFindNextVolume .
     */
    DWORD NextDriveLetter;
} YORI_LIB_FIND_VOLUME_CONTEXT, *PYORI_LIB_FIND_VOLUME_CONTEXT;

/**
 Returns the next volume on the system following a previous call to
 @ref YoriLibFindFirstVolume.  When no more volumes are available, this
 function will return FALSE and set last error to ERROR_NO_MORE_FILES.
 When this occurs, the handle must be closed with
 @ref YoriLibFindVolumeClose.

 @param FindHandle The handle previously returned from
        @ref YoriLibFindFirstVolume .

 @param VolumeName On successful completion, populated with the path to the
        next volume found.

 @param BufferLength Specifies the length, in characters, of the VolumeName
        buffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFindNextVolume(
    __in HANDLE FindHandle,
    __out_ecount(BufferLength) LPWSTR VolumeName,
    __in DWORD BufferLength
    )
{
    if (DllKernel32.pFindFirstVolumeW &&
        DllKernel32.pFindNextVolumeW &&
        DllKernel32.pFindVolumeClose &&
        DllKernel32.pGetVolumePathNamesForVolumeNameW) {

        return DllKernel32.pFindNextVolumeW(FindHandle, VolumeName, BufferLength);
    } else {
        PYORI_LIB_FIND_VOLUME_CONTEXT FindContext = (PYORI_LIB_FIND_VOLUME_CONTEXT)FindHandle;
        TCHAR ProbeString[sizeof("A:\\")];
        DWORD DriveType;

        while(TRUE) {
            if (FindContext->NextDriveLetter + 'A' > 'Z') {
                SetLastError(ERROR_NO_MORE_FILES);
                return FALSE;
            }

            ProbeString[0] = (TCHAR)(FindContext->NextDriveLetter + 'A');
            ProbeString[1] = ':';
            ProbeString[2] = '\\';
            ProbeString[3] = '\0';

            DriveType = GetDriveType(ProbeString);
            if (DriveType != DRIVE_UNKNOWN &&
                DriveType != DRIVE_NO_ROOT_DIR) {

                if (BufferLength >= sizeof(ProbeString)/sizeof(ProbeString[0])) {
                    memcpy(VolumeName, ProbeString, (sizeof(ProbeString)/sizeof(ProbeString[0]) - 1) * sizeof(TCHAR));
                    VolumeName[sizeof(ProbeString)/sizeof(ProbeString[0]) - 1] = '\0';
                    FindContext->NextDriveLetter++;
                    return TRUE;
                } else {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                }
            }

            FindContext->NextDriveLetter++;
        }
    }

    return FALSE;
}

/**
 Close a handle returned from @ref YoriLibFindFirstVolume .

 @param FindHandle The handle to close.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFindVolumeClose(
    __in HANDLE FindHandle
    )
{
    if (DllKernel32.pFindFirstVolumeW &&
        DllKernel32.pFindNextVolumeW &&
        DllKernel32.pFindVolumeClose &&
        DllKernel32.pGetVolumePathNamesForVolumeNameW) {

        return DllKernel32.pFindVolumeClose(FindHandle);
    } else {
        YoriLibFree(FindHandle);
    }
    return TRUE;
}

/**
 Returns the first volume on the system and a handle to use for subsequent
 volumes with @ref YoriLibFindNextVolume .  This handle must be closed with
 @ref YoriLibFindVolumeClose.

 @param VolumeName On successful completion, populated with the path to the
        first volume found.

 @param BufferLength Specifies the length, in characters, of the VolumeName
        buffer.

 @return On successful completion, an opaque handle to use for subsequent
         matches by calling @ref YoriLibFindNextVolume , and terminated by
         calling @ref YoriLibFindVolumeClose .  On failure,
         INVALID_HANDLE_VALUE.
 */
__success(return != INVALID_HANDLE_VALUE)
HANDLE
YoriLibFindFirstVolume(
    __out LPWSTR VolumeName,
    __in DWORD BufferLength
    )
{

    //
    //  Windows 2000 supports mount points but doesn't provide the API
    //  needed to find a human name for them, so we treat it like NT4
    //  and only look for drive letter paths.  Not including mount points
    //  seems like a lesser evil than giving the user volume GUIDs.
    //

    if (DllKernel32.pFindFirstVolumeW &&
        DllKernel32.pFindNextVolumeW &&
        DllKernel32.pFindVolumeClose &&
        DllKernel32.pGetVolumePathNamesForVolumeNameW) {

        return DllKernel32.pFindFirstVolumeW(VolumeName, BufferLength);
    } else {
        PYORI_LIB_FIND_VOLUME_CONTEXT FindContext;

        FindContext = YoriLibMalloc(sizeof(YORI_LIB_FIND_VOLUME_CONTEXT));
        if (FindContext == NULL) {
            return INVALID_HANDLE_VALUE;
        }

        FindContext->NextDriveLetter = 0;

        if (YoriLibFindNextVolume((HANDLE)FindContext, VolumeName, BufferLength)) {
            return (HANDLE)FindContext;
        } else {
            YoriLibFindVolumeClose((HANDLE)FindContext);
        }
    }
    return INVALID_HANDLE_VALUE;
}

/**
 Wrapper that calls GetDiskFreeSpaceEx if present, and if not uses 64 bit
 math to calculate total and free disk space up to the limit (which should
 be around 8Tb with a 4Kb cluster size.)

 @param DirectoryName Specifies the drive or directory to calculate free
        space for.

 @param BytesAvailable Optionally points to a location to receive the amount
        of allocatable space on successful completion.

 @param TotalBytes Optionally points to a location to receive the amount
        of total space on successful completion.

 @param FreeBytes Optionally points to a location to receive the amount
        of unused space on successful completion.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibGetDiskFreeSpace(
    __in LPCTSTR DirectoryName,
    __out_opt PLARGE_INTEGER BytesAvailable,
    __out_opt PLARGE_INTEGER TotalBytes,
    __out_opt PLARGE_INTEGER FreeBytes
    )
{
    LARGE_INTEGER LocalBytesAvailable;
    LARGE_INTEGER LocalTotalBytes;
    LARGE_INTEGER LocalFreeBytes;
    DWORD LocalSectorsPerCluster;
    DWORD LocalBytesPerSector;
    LARGE_INTEGER LocalAllocationSize;
    LARGE_INTEGER LocalNumberOfFreeClusters;
    LARGE_INTEGER LocalTotalNumberOfClusters;
    BOOL Result;

    if (DllKernel32.pGetDiskFreeSpaceExW != NULL) {
        Result = DllKernel32.pGetDiskFreeSpaceExW(DirectoryName,
                                                  &LocalBytesAvailable,
                                                  &LocalTotalBytes,
                                                  &LocalFreeBytes);
        if (!Result) {
            return FALSE;
        }
    } else {

        LocalNumberOfFreeClusters.HighPart = 0;
        LocalTotalNumberOfClusters.HighPart = 0;

        Result = GetDiskFreeSpace(DirectoryName,
                                  &LocalSectorsPerCluster,
                                  &LocalBytesPerSector,
                                  &LocalNumberOfFreeClusters.LowPart,
                                  &LocalTotalNumberOfClusters.LowPart);

        if (!Result) {
            return FALSE;
        }

        LocalAllocationSize.QuadPart = LocalSectorsPerCluster * LocalBytesPerSector;

        LocalBytesAvailable.QuadPart = LocalAllocationSize.QuadPart * LocalNumberOfFreeClusters.QuadPart;
        LocalFreeBytes.QuadPart = LocalBytesAvailable.QuadPart;
        LocalTotalBytes.QuadPart = LocalAllocationSize.QuadPart * LocalTotalNumberOfClusters.QuadPart;
    }

    if (BytesAvailable != NULL) {
        BytesAvailable->QuadPart = LocalBytesAvailable.QuadPart;
    }
    if (TotalBytes != NULL) {
        TotalBytes->QuadPart = LocalTotalBytes.QuadPart;
    }
    if (FreeBytes != NULL) {
        FreeBytes->QuadPart = LocalFreeBytes.QuadPart;
    }

    return TRUE;
}


// vim:sw=4:ts=4:et:
