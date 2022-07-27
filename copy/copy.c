/**
 * @file copy/copy.c
 *
 * Yori shell copy files
 *
 * Copyright (c) 2017-2022 Malcolm J. Smith
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, COPYESS OR
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
CHAR strCopyHelpText[] =
        "\n"
        "Copies one or more files.\n"
        "\n"
        "COPY [-license] [-b] [-c:algorithm] [-ds size] [-l] [-n|-nt|-p] [-s] [-t] [-v]\n"
        "      [-x exclude] <src>\n"
        "COPY [-license] [-b] [-c:algorithm] [-ds size] [-l] [-n|-nt|-p] [-s] [-t] [-v]\n"
        "      [-x exclude] <src> [<src> ...] <dest>\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Compress targets with specified algorithm.  Options are:\n"
        "                    lzx, ntfs, xp4k, xp8k, xp16k\n"
        "   -ds            The size of the device, ignored for files\n"
        "   -l             Copy links as links rather than contents\n"
        "   -n             Copy new or files whose size have changed only\n"
        "   -nt            Copy new or files whose size or timestamps have changed only\n"
        "   -p             Preserve existing files, no overwriting\n"
        "   -s             Copy subdirectories as well as files\n"
        "   -t             Copy timestamps only, no data\n"
        "   -v             Verbose output\n"
        "   -x             Exclude files matching specified pattern\n";

/**
 Display usage text to the user.
 */
BOOL
CopyHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Copy %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strCopyHelpText);
    return TRUE;
}

/**
 A single item to exclude.  Note this can refer to multiple files.
 */
typedef struct _COPY_EXCLUDE_ITEM {

    /**
     List of items to exclude.
     */
    YORI_LIST_ENTRY ExcludeList;

    /**
     A string describing the object to exclude, which may include
     wildcards.
     */
    YORI_STRING ExcludeCriteria;
} COPY_EXCLUDE_ITEM, *PCOPY_EXCLUDE_ITEM;

/**
 A context passed between each source file match when copying multiple
 files.
 */
typedef struct _COPY_CONTEXT {
    /**
     Path to the destination for the copy operation.
     */
    YORI_STRING Dest;

    /**
     Files matching any of the exclude rules will not be copied.
     */
    YORI_LIST_ENTRY ExcludeList;

    /**
     State related to background compression of files after copy.
     */
    YORILIB_COMPRESS_CONTEXT CompressContext;

    /**
     The number of bytes to copy when copying to or from a device.  Zero
     means copy until the end of the device.
     */
    LARGE_INTEGER DeviceSize;

    /**
     The file system attributes of the destination.  Used to determine if
     the destination exists and is a directory.
     */
    DWORD DestAttributes;

    /**
     The number of files that have been previously copied to this
     destination.  This can be used to determine if we're about to copy
     a second object over the top of an earlier copied file.
     */
    DWORD FilesCopied;

    /**
     The number of files that have been enumerated while expanding a
     particular command argument.  If this is zero, the argument hasn't
     resolved to any existing files.
     */
    DWORD FilesFoundThisArg;

    /**
     If TRUE, targets should be compressed.
     */
    BOOLEAN CompressDest;

    /**
     If TRUE, links are copied as links rather than having their contents
     copied.
     */
    BOOLEAN CopyAsLinks;

    /**
     If TRUE, files are copied if they are not on the target, or if the size
     on the source is different to the target.  Depending on the value of
     CopyChangedTimestamps, a file may be copied if its size is identical
     on the source and target but the timestamp has changed.
     */
    BOOLEAN CopyNewOnly;

    /**
     If TRUE, files are copied if the timestamp has changed despite other
     attributes being the same.  If FALSE, the timestamp is ignored, and
     files will not be copied unless other attributes have changed.  This
     field is only meaningful if CopyNewOnly is TRUE.
     */
    BOOLEAN CopyChangedTimestamps;

    /**
     If TRUE, files are copied if they do not already exists.  Any existing
     file will be skipped.
     */
    BOOLEAN PreserveExisting;

    /**
     If TRUE, times from the source are explicitly copied to the target. If
     FALSE, this task is left to CopyFile's defaults.
     */
    BOOLEAN CopyTimestamps;

    /**
     If TRUE, data copies are skipped.  This is done when timestamps are
     being copied on existing files without moving any data.
     */
    BOOLEAN SkipDataCopy;

    /**
     If TRUE, the destination is a device rather than a file, and CopyFile
     should not be used since setting file metadata on the device is
     expected to fail.
     */
    BOOLEAN DestinationIsDevice;

    /**
     If TRUE, output is generated for each object copied.
     */
    BOOLEAN Verbose;
} COPY_CONTEXT, *PCOPY_CONTEXT;

/**
 Add a new exclude criteria to the list.

 @param CopyContext Pointer to the copy context to populate with a new
        exclude criteria.

 @param NewCriteria Pointer to the new criteria to add, which may include
        wildcards.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CopyAddExclude(
    __in PCOPY_CONTEXT CopyContext,
    __in PYORI_STRING NewCriteria
    )
{
    PCOPY_EXCLUDE_ITEM ExcludeItem;
    ExcludeItem = YoriLibReferencedMalloc(sizeof(COPY_EXCLUDE_ITEM) + (NewCriteria->LengthInChars + 1) * sizeof(TCHAR));

    if (ExcludeItem == NULL) {
        return FALSE;
    }

    ZeroMemory(ExcludeItem, sizeof(COPY_EXCLUDE_ITEM));
    ExcludeItem->ExcludeCriteria.StartOfString = (LPTSTR)(ExcludeItem + 1);
    ExcludeItem->ExcludeCriteria.LengthInChars = NewCriteria->LengthInChars;
    ExcludeItem->ExcludeCriteria.LengthAllocated = NewCriteria->LengthInChars + 1;
    memcpy(ExcludeItem->ExcludeCriteria.StartOfString, NewCriteria->StartOfString, ExcludeItem->ExcludeCriteria.LengthInChars * sizeof(TCHAR));
    ExcludeItem->ExcludeCriteria.StartOfString[ExcludeItem->ExcludeCriteria.LengthInChars] = '\0';
    YoriLibAppendList(&CopyContext->ExcludeList, &ExcludeItem->ExcludeList);
    return TRUE;
}

/**
 Free all previously added exclude criteria.

 @param CopyContext Pointer to the copy context to free all exclude criteria
        from.
 */
VOID
CopyFreeExcludes(
    __in PCOPY_CONTEXT CopyContext
    )
{
    PCOPY_EXCLUDE_ITEM ExcludeItem;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&CopyContext->ExcludeList, NULL);
    while (ListEntry != NULL) {
        ExcludeItem = CONTAINING_RECORD(ListEntry, COPY_EXCLUDE_ITEM, ExcludeList);
        YoriLibRemoveListItem(&ExcludeItem->ExcludeList);
        YoriLibDereference(ExcludeItem);
        ListEntry = YoriLibGetNextListEntry(&CopyContext->ExcludeList, NULL);
    }
}


/**
 Construct a full path to the destination from a CopyContext which specifies
 the destination location, and the relative path from the source.

 @param CopyContext Pointer to a copy context specifying the destination.

 @param RelativePathFromSource Pointer to the file name relative to the source
        root.

 @param FullDest On successful completion, updated to point to a fully
        qualified name to the destination.

 @return TRUE to indicate success, FALSE to indicate failure.  Note this
         function can display errors to the console.
 */
__success(return)
BOOL
CopyBuildDestinationPath(
    __in PCOPY_CONTEXT CopyContext,
    __in PYORI_STRING RelativePathFromSource,
    __inout PYORI_STRING FullDest
    )
{
    //
    //  If the target is a directory, construct a full path to the object
    //  within the target's directory tree.  Otherwise, the target is just
    //  a regular file with no path.
    //

    if (CopyContext->DestAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        YORI_STRING DestWithFile;

        if (!YoriLibAllocateString(&DestWithFile, CopyContext->Dest.LengthInChars + 1 + RelativePathFromSource->LengthInChars + 1)) {
            return FALSE;
        }
        DestWithFile.LengthInChars = YoriLibSPrintf(DestWithFile.StartOfString, _T("%y\\%y"), &CopyContext->Dest, RelativePathFromSource);
        if (!YoriLibGetFullPathNameReturnAllocation(&DestWithFile, TRUE, FullDest, NULL)) {
            return FALSE;
        }
        YoriLibFreeStringContents(&DestWithFile);
    } else {
        if (CopyContext->FilesCopied > 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Attempting to copy multiple files over a single file (%y)\n"), &CopyContext->Dest);
            return FALSE;
        }
        YoriLibCloneString(FullDest, &CopyContext->Dest);
    }
    return TRUE;
}

/**
 Returns TRUE to indicate that an object should be excluded based on the
 exclude criteria, or FALSE if it should be included.

 @param CopyContext Pointer to the copy context to check the new object
        against.

 @param RelativeSourcePath Pointer to a string describing the file relative
        to the root of the source of the copy operation.

 @param SourceFindData Pointer to information about the source as returned
        from directory enumeration.  This can be NULL if the source was not
        found from directory enumeration.

 @return TRUE to exclude the file, FALSE to include it.
 */
BOOL
CopyShouldExclude(
    __in PCOPY_CONTEXT CopyContext,
    __in PYORI_STRING RelativeSourcePath,
    __in_opt PWIN32_FIND_DATA SourceFindData
    )
{
    PCOPY_EXCLUDE_ITEM ExcludeItem;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&CopyContext->ExcludeList, NULL);
    while (ListEntry != NULL) {
        ExcludeItem = CONTAINING_RECORD(ListEntry, COPY_EXCLUDE_ITEM, ExcludeList);
        if (YoriLibDoesFileMatchExpression(RelativeSourcePath, &ExcludeItem->ExcludeCriteria)) {
            return TRUE;
        }
        ListEntry = YoriLibGetNextListEntry(&CopyContext->ExcludeList, ListEntry);
    }

    if (CopyContext->CopyNewOnly || CopyContext->PreserveExisting) {
        YORI_STRING FullDest;
        BY_HANDLE_FILE_INFORMATION DestFileInfo;
        LARGE_INTEGER DestWriteTime;
        LARGE_INTEGER SourceWriteTime;
        HANDLE DestFileHandle;

        YoriLibInitEmptyString(&FullDest);

        if (!CopyBuildDestinationPath(CopyContext, RelativeSourcePath, &FullDest)) {
            return FALSE;
        }

        DestFileHandle = CreateFile(FullDest.StartOfString,
                                    FILE_READ_ATTRIBUTES,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL|FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL);

        YoriLibFreeStringContents(&FullDest);

        if (DestFileHandle == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        if (CopyContext->PreserveExisting) {
            CloseHandle(DestFileHandle);
            return TRUE;
        }

        if (!GetFileInformationByHandle(DestFileHandle, &DestFileInfo)) {
            CloseHandle(DestFileHandle);
            return FALSE;
        }

        if (SourceFindData == NULL) {
            CloseHandle(DestFileHandle);
            return TRUE;
        }

        if (DestFileInfo.nFileSizeHigh != SourceFindData->nFileSizeHigh ||
            DestFileInfo.nFileSizeLow != SourceFindData->nFileSizeLow) {

            CloseHandle(DestFileHandle);
            return FALSE;
        }

        if (CopyContext->CopyChangedTimestamps) {

            DestWriteTime.HighPart = DestFileInfo.ftLastWriteTime.dwHighDateTime;
            DestWriteTime.LowPart = DestFileInfo.ftLastWriteTime.dwLowDateTime;
            SourceWriteTime.HighPart = SourceFindData->ftLastWriteTime.dwHighDateTime;
            SourceWriteTime.LowPart = SourceFindData->ftLastWriteTime.dwLowDateTime;

            //
            //  Due to file system timing granularity, if the source was written
            //  to more than 5 seconds before or after the target, consider it
            //  a timestamp change.
            //

            if (SourceWriteTime.QuadPart < DestWriteTime.QuadPart - 10 * 1000 * 1000 * 5 ||
                SourceWriteTime.QuadPart > DestWriteTime.QuadPart + 10 * 1000 * 1000 * 5) {

                CloseHandle(DestFileHandle);
                return FALSE;
            }
        }

        CloseHandle(DestFileHandle);
        return TRUE;
    }
    return FALSE;
}

/**
 Copy a single file from the source to the target by preserving its link
 contents.

 @param SourceFileName A NULL terminated string specifying the source file.

 @param DestFileName A NULL terminated string specifying the destination file.

 @param IsDirectory TRUE if the object being copied is a directory, FALSE if
        it is not.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CopyAsLink(
    __in LPTSTR SourceFileName,
    __in LPTSTR DestFileName,
    __in BOOL IsDirectory
    )
{
    PVOID ReparseData;
    HANDLE SourceFileHandle;
    HANDLE DestFileHandle;
    DWORD LastError;
    DWORD BytesReturned;
    LPTSTR ErrText;

    SourceFileHandle = CreateFile(SourceFileName,
                                  GENERIC_READ,
                                  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL|FILE_FLAG_BACKUP_SEMANTICS,
                                  NULL);

    if (SourceFileHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %s: %s"), SourceFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    if (IsDirectory) {
        if (!CreateDirectory(DestFileName, NULL)) {
            LastError = GetLastError();
            if (LastError != ERROR_ALREADY_EXISTS) {
                ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Create of destination failed: %s: %s"), DestFileName, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                CloseHandle(SourceFileHandle);
                return FALSE;
            }
        }

        DestFileHandle = CreateFile(DestFileName,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL|FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL);

        if (DestFileHandle == INVALID_HANDLE_VALUE) {
            LastError = GetLastError();
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of destination failed: %s: %s"), DestFileName, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            CloseHandle(SourceFileHandle);
            RemoveDirectory(DestFileName);
            return FALSE;
        }
    } else {
        DestFileHandle = CreateFile(DestFileName,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL|FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL);

        if (DestFileHandle == INVALID_HANDLE_VALUE) {
            LastError = GetLastError();
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of destination failed: %s: %s"), DestFileName, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            CloseHandle(SourceFileHandle);
            return FALSE;
        }
    }

    ReparseData = YoriLibMalloc(64 * 1024);
    if (ReparseData == NULL) {
        CloseHandle(DestFileHandle);
        CloseHandle(SourceFileHandle);
        if (IsDirectory) {
            RemoveDirectory(DestFileName);
        } else {
            DeleteFile(DestFileName);
        }
        return FALSE;
    }

    if (!DeviceIoControl(SourceFileHandle, FSCTL_GET_REPARSE_POINT, NULL, 0, ReparseData, 64 * 1024, &BytesReturned, NULL)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Querying reparse data of source failed: %s: %s"), SourceFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(SourceFileHandle);
        CloseHandle(DestFileHandle);
        YoriLibFree(ReparseData);
        if (IsDirectory) {
            RemoveDirectory(DestFileName);
        } else {
            DeleteFile(DestFileName);
        }
        return FALSE;
    }

    if (!DeviceIoControl(DestFileHandle, FSCTL_SET_REPARSE_POINT, ReparseData, BytesReturned, NULL, 0, &BytesReturned, NULL)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Setting reparse data on dest failed: %s: %s"), DestFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(SourceFileHandle);
        CloseHandle(DestFileHandle);
        YoriLibFree(ReparseData);
        if (IsDirectory) {
            RemoveDirectory(DestFileName);
        } else {
            DeleteFile(DestFileName);
        }
        return FALSE;
    }

    CloseHandle(SourceFileHandle);
    CloseHandle(DestFileHandle);
    YoriLibFree(ReparseData);
    return TRUE;
}

/**
 For objects that are not really files, copy can't use CopyFile, and instead
 falls back to this stupid thing of reading and writing.  Note this path
 should not be used for files since it makes no attempt to preserve any kind
 of file metadata, but for devices file metadata is meaningless anyway.

 @param CopyContext Pointer to the copy context, specifying device size.

 @param SourceFile Pointer to the source file/device name.

 @param DestFile Pointer to the destination file/device name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CopyAsDumbDataMove(
    __in PCOPY_CONTEXT CopyContext,
    __in PYORI_STRING SourceFile,
    __in PYORI_STRING DestFile
    )
{
    PVOID Buffer;
    DWORD BytesCopied;
    DWORD BufferSize;
    DWORD SectorSize;
    HANDLE SourceHandle;
    HANDLE DestHandle;
    DWORD LastError;
    LPTSTR ErrText;
    LONGLONG TotalBytesCopied;

    SourceHandle = CreateFile(SourceFile->StartOfString,
                              GENERIC_READ,
                              FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_OPEN_NO_RECALL|FILE_FLAG_BACKUP_SEMANTICS,
                              NULL);

    if (SourceHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %y: %s"), SourceFile, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    DestHandle = CreateFile(DestFile->StartOfString,
                            GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    LastError = GetLastError();

    if (LastError == ERROR_INVALID_PARAMETER) {
        DestHandle = CreateFile(DestFile->StartOfString,
                                GENERIC_WRITE,
                                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);
    }

    if (DestHandle == INVALID_HANDLE_VALUE) {
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of destination failed: %y: %s"), DestFile, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(SourceHandle);
        return FALSE;
    }

    SectorSize = YoriLibGetHandleSectorSize(DestHandle);

    BufferSize = 64 * 1024;
    Buffer = YoriLibMalloc(BufferSize);
    if (Buffer == NULL) {
        CloseHandle(SourceHandle);
        CloseHandle(DestHandle);
        return FALSE;
    }

    if (SectorSize > BufferSize) {
        SectorSize = BufferSize;
    }

    TotalBytesCopied = 0;

    while (ReadFile(SourceHandle, Buffer, BufferSize, &BytesCopied, NULL)) {
        if (BytesCopied == 0) {
            break;
        }

        if (CopyContext->DeviceSize.QuadPart != 0 &&
            (TotalBytesCopied + BytesCopied) > CopyContext->DeviceSize.QuadPart) {

            BytesCopied = (DWORD)(CopyContext->DeviceSize.QuadPart - TotalBytesCopied);

        }

        //
        //  If the destination has a sector size requirement, round up to the
        //  next whole sector
        //

        if (SectorSize != 0 &&
            (BytesCopied % SectorSize) != 0) {

            DWORD SectorOffset;
            DWORD SectorRemaining;
            DWORD BufferOffset;

            SectorOffset = BytesCopied % SectorSize;
            SectorRemaining = SectorSize - SectorOffset;

            BufferOffset = (BytesCopied / SectorSize) * SectorSize + SectorOffset;

            ZeroMemory(YoriLibAddToPointer(Buffer, BufferOffset), SectorRemaining);
            BytesCopied = BytesCopied + SectorRemaining;
        }

        if (!WriteFile(DestHandle, Buffer, BytesCopied, &BytesCopied, NULL)) {
            LastError = GetLastError();
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Write to destination failed: %y: %s"), DestFile, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            YoriLibFree(Buffer);
            CloseHandle(SourceHandle);
            CloseHandle(DestHandle);
            return FALSE;
        }

        TotalBytesCopied = TotalBytesCopied + BytesCopied;
        if (CopyContext->DeviceSize.QuadPart != 0 &&
            TotalBytesCopied >= CopyContext->DeviceSize.QuadPart) {

            break;
        }
    }

    YoriLibFree(Buffer);
    CloseHandle(SourceHandle);
    CloseHandle(DestHandle);
    return TRUE;
}

/**
 Apply the timestamps from the source enumeration to the target file.  This
 can be done as a standalone operation or as part of updating files to
 newer contents, where it is important that the timestamps of the target are
 updated.

 @param SourceFindData Pointer to the enumeration from the source specifying
        file times to apply.

 @param DestFile Points to the fully qualified pathname to the target to
        apply timestamps to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CopyTimestamps(
    __in PWIN32_FIND_DATA SourceFindData,
    __in PYORI_STRING DestFile
    )
{
    HANDLE DestFileHandle;

    DestFileHandle = CreateFile(DestFile->StartOfString,
                                FILE_WRITE_ATTRIBUTES,
                                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL|FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);

    if (DestFileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (!SetFileTime(DestFileHandle, &SourceFindData->ftCreationTime, &SourceFindData->ftLastAccessTime, &SourceFindData->ftLastWriteTime)) {
        CloseHandle(DestFileHandle);
        return FALSE;
    }

    CloseHandle(DestFileHandle);
    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.  This can be NULL if the file was
        not found from enumeration, since the file may not be a file system
        object (ie., it may be a device.)

 @param Depth Indicates the recursion depth.  Used by copy to check if it
        needs to create new directories in the destination path.

 @param Context Pointer to a context block specifying the destination of the
        copy, indicating parameters to the copy operation, and tracking how
        many objects have been copied.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
CopyFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PCOPY_CONTEXT CopyContext = (PCOPY_CONTEXT)Context;
    YORI_STRING RelativePathFromSource;
    YORI_STRING FullDest;
    YORI_STRING HumanSourcePath;
    YORI_STRING HumanDestPath;
    PYORI_STRING SourceNameToDisplay;
    PYORI_STRING DestNameToDisplay;
    DWORD SlashesFound;
    DWORD Index;
    DWORD LastError;

    CopyContext->FilesFoundThisArg++;

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    YoriLibInitEmptyString(&FullDest);
    YoriLibInitEmptyString(&RelativePathFromSource);
    YoriLibInitEmptyString(&HumanSourcePath);
    YoriLibInitEmptyString(&HumanDestPath);
    SourceNameToDisplay = FilePath;

    SlashesFound = 0;
    for (Index = FilePath->LengthInChars; Index > 0; Index--) {
        if (FilePath->StartOfString[Index - 1] == '\\') {
            SlashesFound++;
            if (SlashesFound == Depth + 1) {
                break;
            }
        }
    }

    ASSERT(Index > 0);
    ASSERT(SlashesFound == Depth + 1);

    RelativePathFromSource.StartOfString = &FilePath->StartOfString[Index];
    RelativePathFromSource.LengthInChars = FilePath->LengthInChars - Index;

    //
    //  Check if the user wanted to exclude this file
    //

    if (CopyShouldExclude(CopyContext, &RelativePathFromSource, FileInfo)) {

        if (CopyContext->Verbose) {
            if (YoriLibUnescapePath(FilePath, &HumanSourcePath)) {
                SourceNameToDisplay = &HumanSourcePath;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Skipping %y\n"), SourceNameToDisplay);
            YoriLibFreeStringContents(&HumanSourcePath);
        }

        return TRUE;
    }

    if (!CopyBuildDestinationPath(CopyContext, &RelativePathFromSource, &FullDest)) {
        return FALSE;
    }

    //
    //  This cannot detect all cases where two paths might refer to the same
    //  file, but it can improve the experience if a user fails to specify
    //  a destination (implying a relative path to a file in the current
    //  directory should be copied to the current directory.)  It can't even
    //  check for case insensitivity given NTFS can support case sensitive
    //  paths.
    //

    if (YoriLibCompareString(&FullDest, FilePath) == 0) {
        YoriLibFreeStringContents(&FullDest);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Cannot copy file over itself: %y\n"), FilePath);
        return TRUE;
    }

    DestNameToDisplay = &FullDest;

    if (CopyContext->Verbose) {
        if (YoriLibUnescapePath(FilePath, &HumanSourcePath)) {
            SourceNameToDisplay = &HumanSourcePath;
        }
        if (YoriLibUnescapePath(&FullDest, &HumanDestPath)) {
            DestNameToDisplay = &HumanDestPath;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Copying %y to %y\n"), SourceNameToDisplay, DestNameToDisplay);
    }


    if (!CopyContext->SkipDataCopy) {
        if (FileInfo != NULL &&
            FileInfo->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT &&
            CopyContext->CopyAsLinks &&
            (FileInfo->dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT || FileInfo->dwReserved0 == IO_REPARSE_TAG_SYMLINK)) {

            CopyAsLink(FilePath->StartOfString, FullDest.StartOfString, (FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));

        } else if (FileInfo != NULL &&
                   FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!CreateDirectory(FullDest.StartOfString, NULL)) {
                LastError = GetLastError();
                if (LastError != ERROR_ALREADY_EXISTS) {
                    LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CreateDirectory failed: %s: %s"), FullDest.StartOfString, ErrText);
                    YoriLibFreeWinErrorText(ErrText);
                }
            }
        } else if (CopyContext->DestinationIsDevice || YoriLibIsFileNameDeviceName(FilePath)) {
            CopyAsDumbDataMove(CopyContext, FilePath, &FullDest);
        } else {
            LastError = YoriLibCopyFile(FilePath, &FullDest);
            if (LastError != ERROR_SUCCESS) {

                //
                //  If it failed with an error indicating CopyFile couldn't
                //  handle it, fall back to dumb data copy.  Note that this
                //  function will output its own errors, so from this point,
                //  error handling is over.
                //

                if (LastError == ERROR_INVALID_PARAMETER) {
                    CopyAsDumbDataMove(CopyContext, FilePath, &FullDest);
                } else {
                    LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
                    if (SourceNameToDisplay != &HumanSourcePath) {
                        if (YoriLibUnescapePath(FilePath, &HumanSourcePath)) {
                            SourceNameToDisplay = &HumanSourcePath;
                        }
                    }
                    if (DestNameToDisplay != &HumanDestPath) {
                        if (YoriLibUnescapePath(&FullDest, &HumanDestPath)) {
                            DestNameToDisplay = &HumanDestPath;
                        }
                    }
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CopyFile failed: %y to %y: %s"), SourceNameToDisplay, DestNameToDisplay, ErrText);
                    YoriLibFreeWinErrorText(ErrText);
                }
            }

            if (CopyContext->CompressDest) {

                YoriLibCompressFileInBackground(&CopyContext->CompressContext, &FullDest);
            }
        }
    }

    if (CopyContext->CopyTimestamps && FileInfo != NULL) {
        CopyTimestamps(FileInfo, &FullDest);
    }

    CopyContext->FilesCopied++;
    YoriLibFreeStringContents(&FullDest);
    YoriLibFreeStringContents(&HumanSourcePath);
    YoriLibFreeStringContents(&HumanDestPath);
    return TRUE;
}

/**
 Free the structures allocated within a copy context.  The structure itself
 is on the stack and is not freed.  This will wait for any outstanding
 compression work to complete.

 @param CopyContext Pointer to the context to free.
 */
VOID
CopyFreeCopyContext(
    __in PCOPY_CONTEXT CopyContext
    )
{
    YoriLibFreeCompressContext(&CopyContext->CompressContext);
    YoriLibFreeStringContents(&CopyContext->Dest);
    CopyFreeExcludes(CopyContext);
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the copy builtin command.
 */
#define ENTRYPOINT YoriCmd_YCOPY
#else
/**
 The main entrypoint for the copy standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the copy cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD FilesProcessed;
    DWORD FileCount;
    DWORD LastFileArg = 0;
    DWORD FirstFileArg = 0;
    DWORD MatchFlags;
    BOOL BasicEnumeration;
    BOOL Recursive;
    DWORD i;
    DWORD Result;
    COPY_CONTEXT CopyContext;
    YORILIB_COMPRESS_ALGORITHM CompressionAlgorithm;
    YORI_STRING Arg;

    FileCount = 0;
    Recursive = FALSE;
    BasicEnumeration = FALSE;
    ZeroMemory(&CopyContext, sizeof(CopyContext));
    CompressionAlgorithm.EntireAlgorithm = 0;

    YoriLibInitializeListHead(&CopyContext.ExcludeList);

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));
        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                CopyHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2022"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS16K;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("ds")) == 0) {
                if (i + 1 < ArgC) {
                    CopyContext.DeviceSize = YoriLibStringToFileSize(&ArgV[i + 1]);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:lzx")) == 0) {

                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_LZX;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:ntfs")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.NtfsAlgorithm = COMPRESSION_FORMAT_DEFAULT;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xpress")) == 0 ||
                       YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xp4k")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS4K;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xp8k")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS8K;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xp16k")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS16K;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                CopyContext.CopyAsLinks = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                CopyContext.PreserveExisting = FALSE;
                CopyContext.SkipDataCopy = FALSE;
                CopyContext.CopyNewOnly = TRUE;
                CopyContext.CopyChangedTimestamps = FALSE;
                CopyContext.CopyTimestamps = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("nt")) == 0) {
                CopyContext.PreserveExisting = FALSE;
                CopyContext.SkipDataCopy = FALSE;
                CopyContext.CopyNewOnly = TRUE;
                CopyContext.CopyChangedTimestamps = TRUE;
                CopyContext.CopyTimestamps = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                CopyContext.CopyNewOnly = FALSE;
                CopyContext.SkipDataCopy = FALSE;
                CopyContext.PreserveExisting = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                CopyContext.CopyTimestamps = TRUE;
                CopyContext.SkipDataCopy = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("v")) == 0) {
                CopyContext.Verbose = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("x")) == 0) {
                if (i + 1 < ArgC) {
                    CopyAddExclude(&CopyContext, &ArgV[i + 1]);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            }
        } else {
            ArgumentUnderstood = TRUE;
            FileCount++;
            LastFileArg = i;
            if (FirstFileArg == 0) {
                FirstFileArg = i;
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (FileCount == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("copy: argument missing\n"));
        CopyFreeCopyContext(&CopyContext);
        return EXIT_FAILURE;
    } else if (FileCount == 1) {
        YORI_STRING RelativeCurrentDirectory;
        YoriLibConstantString(&RelativeCurrentDirectory, _T("."));
        if (!YoriLibUserStringToSingleFilePathOrDevice(&RelativeCurrentDirectory, TRUE, &CopyContext.Dest)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("copy: could not resolve %y\n"), &RelativeCurrentDirectory);
            CopyFreeCopyContext(&CopyContext);
            return EXIT_FAILURE;
        }
    } else {
        if (!YoriLibUserStringToSingleFilePathOrDevice(&ArgV[LastFileArg], TRUE, &CopyContext.Dest)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("copy: could not resolve %y\n"), &ArgV[LastFileArg]);
            CopyFreeCopyContext(&CopyContext);
            return EXIT_FAILURE;
        }
        if (YoriLibIsFileNameDeviceName(&ArgV[LastFileArg])) {
            CopyContext.DestinationIsDevice = TRUE;
        }
        FileCount--;
    }

    ASSERT(YoriLibIsStringNullTerminated(&CopyContext.Dest));

    if (CopyContext.CopyAsLinks) {
        if (!YoriLibEnableSymbolicLinkPrivilege()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("copy: warning: could not enable symlink privilege\n"));
        }
    }

    CopyContext.DestAttributes = GetFileAttributes(CopyContext.Dest.StartOfString);
    if (CopyContext.DestAttributes == 0xFFFFFFFF) {
        if (Recursive) {
            if (!CreateDirectory(CopyContext.Dest.StartOfString, NULL)) {
                DWORD LastError = GetLastError();
                LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CreateDirectory failed: %y: %s"), &CopyContext.Dest, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                CopyFreeCopyContext(&CopyContext);
                return EXIT_FAILURE;
            }
            CopyContext.DestAttributes = GetFileAttributes(CopyContext.Dest.StartOfString);
        }

        if (CopyContext.DestAttributes == 0xFFFFFFFF) {
            CopyContext.DestAttributes = 0;
        }
    }

    if (CopyContext.CompressDest) {
        if (!YoriLibInitializeCompressContext(&CopyContext.CompressContext, CompressionAlgorithm)) {
            CopyFreeCopyContext(&CopyContext);
            return EXIT_FAILURE;
        }
        if (CopyContext.Verbose) {
            CopyContext.CompressContext.Verbose = TRUE;
        }
    }

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    CopyContext.FilesCopied = 0;
    FilesProcessed = 0;

    for (i = FirstFileArg; i <= LastFileArg; i++) {
        if (!YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            //
            //  Cmd's copy and xcopy both treat any specified directory as
            //  referring to the contents of the directory.  So if you use:
            //
            //  xcopy /s dir_a dir_b
            //
            //  The files in dir_a are copied to dir_b, and there is no dir_a
            //  in the result.
            //
            //  Here we deliberately deviate from that behavior, and copy
            //  the object known as dir_a under dir_b.  Referring to files
            //  requires specifying dir_a\* .  This seems more logical and
            //  flexible than cmd/xcopy, even though it is different.
            //
            //  The "copy directory contents" case is retained in
            //  non-recursive copies, because in that case presumably the
            //  intent was not to copy an empty directory.
            //

            MatchFlags = YORILIB_FILEENUM_RETURN_FILES;
            if (BasicEnumeration) {
                MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
            }
            if (Recursive) {
                MatchFlags |= YORILIB_FILEENUM_RECURSE_AFTER_RETURN | YORILIB_FILEENUM_RETURN_DIRECTORIES;
                if (CopyContext.CopyAsLinks) {
                    MatchFlags |= YORILIB_FILEENUM_NO_LINK_TRAVERSE;
                }
            } else {
                MatchFlags |= YORILIB_FILEENUM_DIRECTORY_CONTENTS;
            }

            CopyContext.FilesFoundThisArg = 0;
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, CopyFileFoundCallback, NULL, &CopyContext);
            if (CopyContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                if (YoriLibUserStringToSingleFilePathOrDevice(&ArgV[i], TRUE, &FullPath)) {
                    CopyFileFoundCallback(&FullPath, NULL, 0, &CopyContext);
                    YoriLibFreeStringContents(&FullPath);
                }
            }
            FilesProcessed++;
            if (FilesProcessed == FileCount) {
                break;
            }
        }
    }

    Result = EXIT_SUCCESS;

    if (CopyContext.FilesCopied == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("copy: no matching files found\n"));
        Result = EXIT_FAILURE;
    }

    CopyFreeCopyContext(&CopyContext);

    return Result;
}

// vim:sw=4:ts=4:et:
