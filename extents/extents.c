/**
 * @file extents/extents.c
 *
 * Yori shell display and manipulate file extents
 *
 * Copyright (c) 2018-2025 Malcolm J. Smith
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
CHAR strExtentsHelpText[] =
        "\n"
        "Output location of files within a volume or disk and relocate files.\n"
        "\n"
        "EXTENTS [-license] [-b] [-d] [-h] [-m vcn lcn cnt] [-s] <file>...\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -d             Return directories rather than directory contents\n"
        "   -dd            Display contents of the file from the disk\n"
        "   -df            Display contents of the NTFS file record\n"
        "   -dv            Display contents of the file from the volume\n"
        "   -h             Display output in hexadecimal\n"
        "   -m vcn lcn cnt Move a range of a file to a new position\n"
        "   -s             Process files from all subdirectories\n";

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _EXTENTS_CONTEXT {

    /**
     Pointer to the full file path for a matching file.
     */
    PYORI_STRING FilePath;

    /**
     When moving a file (ClusterCount != 0), specifies the offset of the file
     to move.
     */
    LONGLONG StartingVcn;

    /**
     When moving a file (ClusterCount != 0), specifies the target cluster on
     the volume to move to.
     */
    LONGLONG StartingLcn;

    /**
     Specifies the number of clusters to move.  If zero, the program is
     outputting the locations of files.
     */
    DWORD ClusterCount;

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     Records the total number of files processed for each argument processed.
     */
    LONGLONG FilesFoundThisArg;

    /**
     If TRUE, display file contents via a disk handle.
     */
    BOOLEAN DisplayDiskContents;

    /**
     If TRUE, display contents of an NTFS file record.
     */
    BOOLEAN DisplayFileRecord;

    /**
     Output offsets in hex rather than decimal.
     */
    BOOLEAN DisplayHex;

    /**
     If TRUE, display file contents via a volume handle.
     */
    BOOLEAN DisplayVolumeContents;

} EXTENTS_CONTEXT, *PEXTENTS_CONTEXT;

/**
 Display usage text to the user.
 */
BOOL
ExtentsHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Extents %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strExtentsHelpText);
    return TRUE;
}

/**
 Display the contents of an NTFS file record.

 @param ExtentsContext Pointer to the application context.

 @param FileHandle Specifies the handle to the file whose file record should
        be displayed.

 @param DeviceHandle Specifies the volume handle to read the file record from.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
ExtentsDisplayFileRecord(
    __in PEXTENTS_CONTEXT ExtentsContext,
    __in HANDLE FileHandle,
    __in HANDLE DeviceHandle
    )
{
    BY_HANDLE_FILE_INFORMATION FileInfo;
    LARGE_INTEGER FileNumber;
    PNTFS_FILE_RECORD_OUTPUT_BUFFER FileRecord;
    YORI_ALLOC_SIZE_T FileRecordSize;
    DWORD BytesReturned;
    SYSERR Error;
    LPTSTR ErrText;

    UNREFERENCED_PARAMETER(ExtentsContext);

    if (!GetFileInformationByHandle(FileHandle, &FileInfo)) {
        Error = GetLastError();
        ErrText = YoriLibGetWinErrorText(Error);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: get file information failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    FileNumber.LowPart = FileInfo.nFileIndexLow;
    FileNumber.HighPart = FileInfo.nFileIndexHigh;

    //
    //  NTFS supports 1Kb and 4Kb file records.  4Kb plus the size of the
    //  structure header should be sufficient for any NTFS volume.
    //

    FileRecordSize = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + 4096;
    FileRecord = YoriLibMalloc(FileRecordSize);
    if (FileRecord == NULL) {
        return FALSE;
    }

    if (!DeviceIoControl(DeviceHandle,
                         FSCTL_GET_NTFS_FILE_RECORD,
                         &FileNumber,
                         sizeof(FileNumber),
                         FileRecord,
                         FileRecordSize,
                         &BytesReturned,
                         NULL)) {

        Error = GetLastError();
        ErrText = YoriLibGetWinErrorText(Error);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: get file record failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFree(FileRecord);
        return FALSE;
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\nNTFS file record:\n"));
    YoriLibHexDump(FileRecord->FileRecordBuffer, 0, FileRecord->FileRecordLength, sizeof(DWORD), YORI_LIB_HEX_FLAG_DISPLAY_CHARS | YORI_LIB_HEX_FLAG_DISPLAY_OFFSET);

    YoriLibFree(FileRecord);
    return TRUE;
}

/**
 Display the contents of a file by reading underlying extents from a device.

 @param ExtentsContext Pointer to the application context.

 @param FileHandle Specifies the handle to the file whose extents should
        be queried.

 @param DeviceHandle Specifies the handle to the device to read from.

 @param BytesPerCluster Specifies the bytes per cluster to use in display
        calculations.

 @param DeviceOffset Specifies the offset within the device where cluster
        allocations start.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
ExtentsDisplayData(
    __in PEXTENTS_CONTEXT ExtentsContext,
    __in HANDLE FileHandle,
    __in HANDLE DeviceHandle,
    __in DWORD BytesPerCluster,
    __in DWORDLONG DeviceOffset
    )
{
    STARTING_VCN_INPUT_BUFFER StartingVcn;
    PRETRIEVAL_POINTERS_BUFFER RetrievalPointers;
    YORI_ALLOC_SIZE_T RetrievalPointerSize;
    PVOID Buffer;
    BOOLEAN MoreToGo;
    DWORD Index;
    DWORD BytesReturned;
    SYSERR Error;
    LPTSTR ErrText;

    UNREFERENCED_PARAMETER(ExtentsContext);

    RetrievalPointerSize = 4096;
    RetrievalPointers = YoriLibMalloc(RetrievalPointerSize);
    if (RetrievalPointers == NULL) {
        return FALSE;
    }
    Buffer = YoriLibMalloc(BytesPerCluster);
    if (Buffer == NULL) {
        YoriLibFree(RetrievalPointers);
        return FALSE;
    }

    YoriLibLiAssignUnsigned(&StartingVcn.StartingVcn, 0);

    MoreToGo = TRUE;
    while (MoreToGo) {
        MoreToGo = FALSE;
        Error = ERROR_SUCCESS;
        if (!DeviceIoControl(FileHandle,
                             FSCTL_GET_RETRIEVAL_POINTERS,
                             &StartingVcn,
                             sizeof(StartingVcn),
                             RetrievalPointers,
                             RetrievalPointerSize,
                             &BytesReturned,
                             NULL)) {

            Error = GetLastError();
            if (Error == ERROR_MORE_DATA) {
                MoreToGo = TRUE;
                Error = ERROR_SUCCESS;
            }
        }

        if (Error == ERROR_SUCCESS) {
            YORI_MAX_UNSIGNED_T FileOffset;
            YORI_MAX_UNSIGNED_T Lcn;
            YORI_MAX_SIGNED_T CurrentVcn;
            YORI_MAX_UNSIGNED_T DeviceExtentOffset;
            YORI_MAX_UNSIGNED_T ExtentLength;
            LARGE_INTEGER liDeviceExtentOffset;
            DWORD ExtentIndex;
            DWORD BytesRead;

            if (StartingVcn.StartingVcn.QuadPart == 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\nFile contents:\n"));
            }

            //
            //  Loop over all extents returned from this call.
            //  Note this is not necessarily all extents for
            //  the file.
            //

            CurrentVcn = RetrievalPointers->StartingVcn.QuadPart;
            for (Index = 0; Index < RetrievalPointers->ExtentCount; Index++) {

                FileOffset = CurrentVcn * BytesPerCluster;
                Lcn = RetrievalPointers->Extents[Index].Lcn.QuadPart;
                ExtentLength = RetrievalPointers->Extents[Index].NextVcn.QuadPart - CurrentVcn;

                for (ExtentIndex = 0; ExtentIndex < ExtentLength; ExtentIndex++) {

                    DeviceExtentOffset = (Lcn + ExtentIndex) * BytesPerCluster + DeviceOffset;
                    YoriLibLiAssignUnsigned(&liDeviceExtentOffset, DeviceExtentOffset);

                    if (!SetFilePointer(DeviceHandle, liDeviceExtentOffset.LowPart, &liDeviceExtentOffset.HighPart, FILE_BEGIN)) {
                        Error = GetLastError();
                        ErrText = YoriLibGetWinErrorText(Error);
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: get retrieval pointers failed: %s"), ErrText);
                        YoriLibFreeWinErrorText(ErrText);
                        MoreToGo = FALSE;
                        break;
                    }

                    if (!ReadFile(DeviceHandle, Buffer, BytesPerCluster, &BytesRead, NULL)) {
                        Error = GetLastError();
                        ErrText = YoriLibGetWinErrorText(Error);
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: reading data at offset 0x%llx failed: %s"), DeviceExtentOffset, ErrText);
                        YoriLibFreeWinErrorText(ErrText);
                        MoreToGo = FALSE;
                        break;
                    }

                    YoriLibHexDump(Buffer, DeviceExtentOffset, BytesPerCluster, sizeof(DWORD), YORI_LIB_HEX_FLAG_DISPLAY_CHARS | YORI_LIB_HEX_FLAG_DISPLAY_LARGE_OFFSET);
                }

                CurrentVcn = RetrievalPointers->Extents[Index].NextVcn.QuadPart;
            }

            //
            //  Find the start point for the next call, or indicate that
            //  the end of file has been reached.
            //

            if (!MoreToGo) {
                FileOffset = CurrentVcn * BytesPerCluster;
                break;
            } else if (CurrentVcn > StartingVcn.StartingVcn.QuadPart) {
                StartingVcn.StartingVcn.QuadPart = CurrentVcn;
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: get retrieval pointers did not advance, previous start vcn %lli, new start vcn %lli\n"), StartingVcn.StartingVcn.QuadPart, CurrentVcn);
                MoreToGo = FALSE;
            }
        } else {
            MoreToGo = FALSE;
        }
    }

    YoriLibFree(RetrievalPointers);
    YoriLibFree(Buffer);

    return TRUE;
}

/**
 Display the contents of a file by opening a disk handle and reading contents
 from it.

 @param ExtentsContext Pointer to the application context.

 @param FileHandle Specifies the handle to the file whose extents should
        be queried.

 @param BytesPerCluster Specifies the bytes per cluster to use in display
        calculations.

 @param PartitionOffset Specifies the offset within the partition where
        cluster allocations start.

 @param VolumeDiskExtents Specifies the location of the volume within the
        disk to use in display calculations.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
ExtentsDisplayDiskData(
    __in PEXTENTS_CONTEXT ExtentsContext,
    __in HANDLE FileHandle,
    __in DWORD BytesPerCluster,
    __in DWORDLONG PartitionOffset,
    __in PYORI_VOLUME_DISK_EXTENTS VolumeDiskExtents
    )
{
    HANDLE DeviceHandle;
    YORI_STRING DevicePath;
    BOOLEAN Result;
    SYSERR Error;
    LPTSTR ErrText;

    if (VolumeDiskExtents->NumberOfDiskExtents != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: cannot display data from disk unless partition maps to single disk region\n"));
        return FALSE;
    }

    YoriLibInitEmptyString(&DevicePath);
    YoriLibYPrintf(&DevicePath, _T("\\\\.\\PhysicalDrive%i"), VolumeDiskExtents->Extents[0].DiskNumber);
    if (DevicePath.StartOfString == NULL) {
        return FALSE;
    }

    DeviceHandle = CreateFile(DevicePath.StartOfString,
                              FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_OPEN_NO_RECALL,
                              NULL);
    if (DeviceHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
        ErrText = YoriLibGetWinErrorText(Error);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: open of %y failed: %s\n"), &DevicePath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&DevicePath);
        return FALSE;
    }

    Result = ExtentsDisplayData(ExtentsContext,
                                FileHandle,
                                DeviceHandle,
                                BytesPerCluster,
                                PartitionOffset + VolumeDiskExtents->Extents[0].StartingOffset.QuadPart);

    YoriLibFreeStringContents(&DevicePath);
    CloseHandle(DeviceHandle);
    return Result;
}

/**
 Display the extents used by a single file.

 @param ExtentsContext Pointer to the application context.

 @param FilePath Specifies the full path to the file.

 @param FileHandle Specifies the handle to the file whose extents should
        be displayed.

 @param VolumePath Specifies the path to the volume for display.

 @param BytesPerCluster Specifies the bytes per cluster to use in display
        calculations.

 @param RetrievalPointerBase Specifies the base offset of the volume to use
        in display calculations.

 @param VolumeDiskExtents Specifies the location of the volume within the
        disk to use in display calculations.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
ExtentsDisplayExtents(
    __in PEXTENTS_CONTEXT ExtentsContext,
    __in PYORI_STRING FilePath,
    __in HANDLE FileHandle,
    __in PYORI_STRING VolumePath,
    __in DWORD BytesPerCluster,
    __in DWORDLONG RetrievalPointerBase,
    __in PYORI_VOLUME_DISK_EXTENTS VolumeDiskExtents
    )
{
    STARTING_VCN_INPUT_BUFFER StartingVcn;
    PRETRIEVAL_POINTERS_BUFFER RetrievalPointers;
    YORI_ALLOC_SIZE_T RetrievalPointerSize;
    BOOLEAN MoreToGo;
    DWORD Index;
    DWORD BytesReturned;
    SYSERR Error;
    LPTSTR ErrText;

    RetrievalPointerSize = 4096;
    RetrievalPointers = YoriLibMalloc(RetrievalPointerSize);
    if (RetrievalPointers == NULL) {
        return FALSE;
    }

    YoriLibLiAssignUnsigned(&StartingVcn.StartingVcn, 0);

    MoreToGo = TRUE;
    while (MoreToGo) {
        MoreToGo = FALSE;
        Error = ERROR_SUCCESS;
        if (!DeviceIoControl(FileHandle,
                             FSCTL_GET_RETRIEVAL_POINTERS,
                             &StartingVcn,
                             sizeof(StartingVcn),
                             RetrievalPointers,
                             RetrievalPointerSize,
                             &BytesReturned,
                             NULL)) {

            Error = GetLastError();
            if (Error == ERROR_MORE_DATA) {
                MoreToGo = TRUE;
                Error = ERROR_SUCCESS;
            }
        }

        if (Error == ERROR_SUCCESS) {
            YORI_MAX_UNSIGNED_T FileOffset;
            YORI_MAX_UNSIGNED_T Lcn;
            YORI_MAX_SIGNED_T CurrentVcn;
            YORI_MAX_UNSIGNED_T PartitionOffset;
            YORI_MAX_UNSIGNED_T DiskOffset;

            //
            //  For the first range, output headers.  This is deferred until
            //  here so we don't output headers followed by an error.
            //

            if (StartingVcn.StartingVcn.QuadPart == 0) {
                if (ExtentsContext->FilesFound > 0) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
                }
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y:\n"), FilePath);
                if (VolumeDiskExtents->NumberOfDiskExtents == 1) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  (on volume %y, \\\\.\\PhysicalDrive%i):\n"), VolumePath, VolumeDiskExtents->Extents[0].DiskNumber);
                } else {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  (on volume %y):\n"), VolumePath);
                }
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  File Offset    |    Cluster    |   Partition Offset"));
            
                if (VolumeDiskExtents->NumberOfDiskExtents == 1) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("   |  Disk Offset\n"));
                } else {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
                }
            }

            //
            //  Loop over all extents returned from this call.  Note this is
            //  not necessarily all extents for the file.
            //

            CurrentVcn = RetrievalPointers->StartingVcn.QuadPart;
            for (Index = 0; Index < RetrievalPointers->ExtentCount; Index++) {

                FileOffset = CurrentVcn * BytesPerCluster;
                Lcn = RetrievalPointers->Extents[Index].Lcn.QuadPart;
                PartitionOffset = Lcn * BytesPerCluster + RetrievalPointerBase;

                //
                //  Display file offset
                //
    
                if (ExtentsContext->DisplayHex) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("0x%-14llx | "), FileOffset);
                } else {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-16lli | "), FileOffset);
                }

                //
                //  Display cluster and partition offset
                //

                if (Lcn == INVALID_LCN) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("*** NOT ALLOCATED ***\n"));

                } else {
                    if (ExtentsContext->DisplayHex) {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("0x%-11llx | 0x%-18llx"), Lcn, PartitionOffset);
                    } else {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-13lli | %-20lli"), Lcn, PartitionOffset);
                    }

                    //
                    //  On a simple disk, display disk offset
                    //

                    if (VolumeDiskExtents->NumberOfDiskExtents == 1) {
                        DiskOffset = PartitionOffset + VolumeDiskExtents->Extents[0].StartingOffset.QuadPart;
                        if (ExtentsContext->DisplayHex) {
                            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T(" | 0x%-18llx\n"), DiskOffset);
                        } else {
                            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T(" | %-20lli\n"), DiskOffset);
                        }
                    } else {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
                    }
                }

                CurrentVcn = RetrievalPointers->Extents[Index].NextVcn.QuadPart;
            }

            //
            //  Find the start point for the next call, or indicate that
            //  the end of file has been reached.
            //

            if (!MoreToGo) {
                FileOffset = CurrentVcn * BytesPerCluster;
                if (ExtentsContext->DisplayHex) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("0x%-14llx | *** END ***\n"), FileOffset);
                } else {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-16lli | *** END ***\n"), FileOffset);
                }
            } else if (CurrentVcn > StartingVcn.StartingVcn.QuadPart) {
                StartingVcn.StartingVcn.QuadPart = CurrentVcn;
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: get retrieval pointers did not advance, previous start vcn %lli, new start vcn %lli\n"), StartingVcn.StartingVcn.QuadPart, CurrentVcn);
                MoreToGo = FALSE;
            }
        } else if (Error == ERROR_HANDLE_EOF) {
            if (ExtentsContext->FilesFound > 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: %y has no extents\n"), FilePath);
            MoreToGo = FALSE;
        } else {
            ErrText = YoriLibGetWinErrorText(Error);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: get retrieval pointers of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            MoreToGo = FALSE;
        }
    }

    YoriLibFree(RetrievalPointers);
    return TRUE;
}

/**
 Move a range of a file to the specified target extents.

 @param ExtentsContext Pointer to the application context.

 @param FilePath Specifies the full path of the file to move.

 @param VolumeHandle Specifies a handle to the volume that hosts the file.

 @param FileHandle Specifies a handle to the file to move.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
ExtentsMoveFile(
    __in PEXTENTS_CONTEXT ExtentsContext,
    __in PYORI_STRING FilePath,
    __in HANDLE VolumeHandle,
    __in HANDLE FileHandle
    )
{
    MOVE_FILE_DATA MoveData;
    DWORD BytesReturned;
    SYSERR Error;
    LPTSTR ErrText;

    MoveData.FileHandle = FileHandle;
    MoveData.StartingVcn.QuadPart = ExtentsContext->StartingVcn;
    MoveData.StartingLcn.QuadPart = ExtentsContext->StartingLcn;
    MoveData.ClusterCount = ExtentsContext->ClusterCount;

    Error = ERROR_SUCCESS;
    if (!DeviceIoControl(VolumeHandle,
                         FSCTL_MOVE_FILE,
                         &MoveData,
                         sizeof(MoveData),
                         NULL,
                         0,
                         &BytesReturned,
                         NULL)) {

        Error = GetLastError();
        ErrText = YoriLibGetWinErrorText(Error);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: move extent 0x%llx of %y to 0x%llx failed: %s"), ExtentsContext->StartingVcn, FilePath, ExtentsContext->StartingLcn, ErrText);
        YoriLibFreeWinErrorText(ErrText);
    }

    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param Context Pointer to the extents context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
ExtentsFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING VolRootName;
    PEXTENTS_CONTEXT ExtentsContext;
    HANDLE VolumeHandle;
    HANDLE FileHandle;
    DWORD BytesReturned;
    SYSERR Error;
    LPTSTR ErrText;
    DWORD SectorsPerCluster;
    DWORD SectorSize;
    DWORD FreeClusters;
    DWORD TotalClusters;
    DWORD BytesPerCluster;
    DWORD DesiredAccess;
    DWORDLONG RetrievalPointerBase;
    YORI_VOLUME_DISK_EXTENTS VolumeDiskExtents;

    UNREFERENCED_PARAMETER(FileInfo);
    UNREFERENCED_PARAMETER(Depth);
    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    ExtentsContext = (PEXTENTS_CONTEXT)Context;

    //
    //  Find the volume hosting this file.
    //

    YoriLibInitEmptyString(&VolRootName);
    if (!YoriLibGetVolumePathName(FilePath, &VolRootName)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: failed to find volume for file %y\n"), FilePath);
        goto End;
    }

    //
    //  GetDiskFreeSpace wants a name with a trailing backslash.  Add one
    //  if needed.
    //

    if (VolRootName.LengthInChars > 0 &&
        VolRootName.LengthInChars + 1 < VolRootName.LengthAllocated &&
        VolRootName.StartOfString[VolRootName.LengthInChars - 1] != '\\') {

        VolRootName.StartOfString[VolRootName.LengthInChars] = '\\';
        VolRootName.StartOfString[VolRootName.LengthInChars + 1] = '\0';
        VolRootName.LengthInChars++;
    }

    //
    //  Determine the cluster size for the volume.
    //

    if (!GetDiskFreeSpace(VolRootName.StartOfString,
                          &SectorsPerCluster,
                          &SectorSize,
                          &FreeClusters,
                          &TotalClusters)) {
        Error = GetLastError();
        ErrText = YoriLibGetWinErrorText(Error);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: GetDiskFreeSpace of %y failed: %s\n"), &VolRootName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&VolRootName);
        goto End;
    }

    BytesPerCluster = SectorsPerCluster * SectorSize;

    //
    //  Truncate the trailing backslash so as to open the volume instead of
    //  root directory
    //

    if (VolRootName.LengthInChars > 0 &&
        VolRootName.StartOfString[VolRootName.LengthInChars - 1] == '\\') {

        VolRootName.LengthInChars--;
        VolRootName.StartOfString[VolRootName.LengthInChars] = '\0';
    }

    //
    //  This needs to be more than FILE_READ_ATTRIBUTES to get a file system
    //  handle, but not require any form of write access or read data access,
    //  or else it needs an administrative caller.
    //
    DesiredAccess = FILE_READ_ATTRIBUTES | FILE_TRAVERSE;
    if (ExtentsContext->ClusterCount != 0) {
        DesiredAccess = FILE_READ_ATTRIBUTES | FILE_READ_DATA | FILE_WRITE_DATA;
    } else if (ExtentsContext->DisplayVolumeContents) {
        DesiredAccess = FILE_READ_ATTRIBUTES | FILE_TRAVERSE | FILE_READ_DATA;
    }

    VolumeHandle = CreateFile(VolRootName.StartOfString,
                              DesiredAccess,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_OPEN_NO_RECALL,
                              NULL);
    if (VolumeHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
        ErrText = YoriLibGetWinErrorText(Error);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: open of %y failed: %s\n"), &VolRootName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&VolRootName);
        goto End;
    }

    //
    //  NTFS will not answer this unless the user is elevated.  exFAT,
    //  which is what this is really for, will answer it regardless.
    //  If this is not implemented or we don't have access, assume zero.
    //

    RetrievalPointerBase = 0;
    if (DeviceIoControl(VolumeHandle,
                        FSCTL_GET_RETRIEVAL_POINTER_BASE,
                        NULL,
                        0,
                        &RetrievalPointerBase,
                        sizeof(RetrievalPointerBase),
                        &BytesReturned,
                        NULL)) {

        RetrievalPointerBase = RetrievalPointerBase * SectorSize;
    } else {
        Error = GetLastError();
        if (Error != ERROR_INVALID_FUNCTION && Error != ERROR_ACCESS_DENIED) {
            ErrText = YoriLibGetWinErrorText(Error);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: warning: could not query cluster offset on %y: %s\n"), &VolRootName, ErrText);
            YoriLibFreeWinErrorText(ErrText);
        }
        RetrievalPointerBase = 0;
    }

    //
    //  Currently this program can only handle a single disk extent, ie.,
    //  a partition on a single disk device.  Use the NumberOfDiskExtents
    //  field to check whether this call succeeded with the one case the
    //  program can handle.
    //

    VolumeDiskExtents.NumberOfDiskExtents = 0;
    if (!DeviceIoControl(VolumeHandle,
                         IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                         NULL,
                         0,
                         &VolumeDiskExtents,
                         sizeof(VolumeDiskExtents),
                         &BytesReturned,
                         NULL)) {

        Error = GetLastError();
        ErrText = YoriLibGetWinErrorText(Error);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: warning: volume disk extents unavailable on %y: %s\n"), &VolRootName, ErrText);

        VolumeDiskExtents.NumberOfDiskExtents = 0;
    }


    //
    //  Open the file and start querying its extents.
    //

    FileHandle = CreateFile(FilePath->StartOfString,
                            FILE_READ_ATTRIBUTES,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
        ErrText = YoriLibGetWinErrorText(Error);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: open of %y failed: %s"), FilePath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&VolRootName);
        CloseHandle(VolumeHandle);
        return TRUE;
    }

    if (ExtentsContext->ClusterCount == 0) {
        ExtentsDisplayExtents(ExtentsContext,
                              FilePath,
                              FileHandle,
                              &VolRootName,
                              BytesPerCluster,
                              RetrievalPointerBase,
                              &VolumeDiskExtents);

        if (ExtentsContext->DisplayFileRecord) {
            ExtentsDisplayFileRecord(ExtentsContext, FileHandle, VolumeHandle);
        }

        if (ExtentsContext->DisplayVolumeContents) {
            ExtentsDisplayData(ExtentsContext, FileHandle, VolumeHandle, BytesPerCluster, RetrievalPointerBase);
        }

        if (ExtentsContext->DisplayDiskContents) {
            ExtentsDisplayDiskData(ExtentsContext, FileHandle, BytesPerCluster, RetrievalPointerBase, &VolumeDiskExtents);
        }

    } else if (ExtentsContext->FilesFound == 0) {
        ExtentsMoveFile(ExtentsContext, FilePath, VolumeHandle, FileHandle);
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: cannot move multiple files to the same LCN"));
    }


    CloseHandle(FileHandle);
    CloseHandle(VolumeHandle);
    YoriLibFreeStringContents(&VolRootName);

End:

    ExtentsContext->FilesFound++;
    ExtentsContext->FilesFoundThisArg++;

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the extents builtin command.
 */
#define ENTRYPOINT YoriCmd_YEXTENTS
#else
/**
 The main entrypoint for the extents standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the extents cmdlet.

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
    EXTENTS_CONTEXT ExtentsContext;
    YORI_STRING Arg;

    ZeroMemory(&ExtentsContext, sizeof(ExtentsContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                ExtentsHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2025"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("d")) == 0) {
                ReturnDirectories = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("dd")) == 0) {
                ExtentsContext.DisplayDiskContents = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("df")) == 0) {
                ExtentsContext.DisplayFileRecord = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("dv")) == 0) {
                ExtentsContext.DisplayVolumeContents = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("h")) == 0) {
                ExtentsContext.DisplayHex = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("m")) == 0) {
                if (ArgC > i + 3) {
                    YORI_ALLOC_SIZE_T CharsConsumed;
                    YORI_MAX_SIGNED_T llTemp = 0;
                    if (!YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) ||
                        CharsConsumed == 0 ||
                        llTemp < 0) {

                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: invalid VCN\n"));
                        return EXIT_FAILURE;
                    }
                    ExtentsContext.StartingVcn = llTemp;

                    if (!YoriLibStringToNumber(&ArgV[i + 2], TRUE, &llTemp, &CharsConsumed) ||
                        CharsConsumed == 0 ||
                        llTemp < 0) {

                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: invalid LCN\n"));
                        return EXIT_FAILURE;
                    }
                    ExtentsContext.StartingLcn = llTemp;

                    if (!YoriLibStringToNumber(&ArgV[i + 3], TRUE, &llTemp, &CharsConsumed) ||
                        CharsConsumed == 0 ||
                        llTemp <= 0) {

                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: invalid cluster count\n"));
                        return EXIT_FAILURE;
                    }
                    ExtentsContext.ClusterCount = (DWORD)llTemp;
                    ArgumentUnderstood = TRUE;
                    i = i + 3;
                }

            } else if (YoriLibCompareStringLitIns(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("-")) == 0) {
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: missing argument\n"));
        return EXIT_FAILURE;
    } else {
        MatchFlags = YORILIB_ENUM_RETURN_FILES;

        if (ReturnDirectories) {
            MatchFlags |= YORILIB_ENUM_RETURN_DIRECTORIES;
        } else {
            MatchFlags |= YORILIB_ENUM_DIRECTORY_CONTENTS;
        }

        if (Recursive) {
            MatchFlags |= YORILIB_ENUM_REC_BEFORE_RETURN | YORILIB_ENUM_REC_PRESERVE_WILD;
        }

        if (BasicEnumeration) {
            MatchFlags |= YORILIB_ENUM_BASIC_EXPANSION;
        }

        for (i = StartArg; i < ArgC; i++) {

            ExtentsContext.FilesFoundThisArg = 0;
            YoriLibForEachStream(&ArgV[i], MatchFlags, 0, ExtentsFileFoundCallback, NULL, &ExtentsContext);
            if (ExtentsContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    ExtentsFileFoundCallback(&FullPath, NULL, 0, &ExtentsContext);
                    YoriLibFreeStringContents(&FullPath);
                }
            }
        }
    }

    if (ExtentsContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
