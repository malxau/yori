/**
 * @file extents/extents.c
 *
 * Yori shell display file extents
 *
 * Copyright (c) 2018-2024 Malcolm J. Smith
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
        "Output location of files within a volume or disk.\n"
        "\n"
        "EXTENTS [-license] <file>...\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -d             Return directories rather than directory contents\n"
        "   -h             Display output in hexadecimal\n"
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
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     Records the total number of files processed for each argument processed.
     */
    LONGLONG FilesFoundThisArg;

    /**
     Output offsets in hex rather than decimal.
     */
    BOOLEAN DisplayHex;

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
    STARTING_VCN_INPUT_BUFFER StartingVcn;
    PRETRIEVAL_POINTERS_BUFFER RetrievalPointers;
    YORI_ALLOC_SIZE_T RetrievalPointerSize;
    DWORD BytesReturned;
    DWORD Error;
    LPTSTR ErrText;
    BOOLEAN MoreToGo;
    DWORD Index;
    DWORD SectorsPerCluster;
    DWORD SectorSize;
    DWORD FreeClusters;
    DWORD TotalClusters;
    DWORD BytesPerCluster;
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
        return TRUE;
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
        return TRUE;
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

    VolumeHandle = CreateFile(VolRootName.StartOfString,
                              FILE_READ_ATTRIBUTES | FILE_TRAVERSE,
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
        return TRUE;
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

        VolumeDiskExtents.NumberOfDiskExtents = 0;
    }

    CloseHandle(VolumeHandle);

    RetrievalPointerSize = 4096;
    RetrievalPointers = YoriLibMalloc(RetrievalPointerSize);
    if (RetrievalPointers == NULL) {
        YoriLibFreeStringContents(&VolRootName);
        return TRUE;
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
        YoriLibFree(RetrievalPointers);
        return TRUE;
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
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  File Offset    |    Cluster    |   Partition Offset"));
            
                if (VolumeDiskExtents.NumberOfDiskExtents == 1) {
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

                    if (VolumeDiskExtents.NumberOfDiskExtents == 1) {
                        DiskOffset = PartitionOffset + VolumeDiskExtents.Extents[0].StartingOffset.QuadPart;
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


    CloseHandle(FileHandle);
    YoriLibFreeStringContents(&VolRootName);
    YoriLibFree(RetrievalPointers);

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

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ExtentsHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2024"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                ReturnDirectories = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                ExtentsContext.DisplayHex = TRUE;
                ArgumentUnderstood = TRUE;
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("extents: missing argument\n"));
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

            ExtentsContext.FilesFoundThisArg = 0;
            YoriLibForEachStream(&ArgV[i], MatchFlags, 0, ExtentsFileFoundCallback, NULL, &ExtentsContext);
            if (ExtentsContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
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
