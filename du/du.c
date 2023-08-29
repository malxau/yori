/**
 * @file du/du.c
 *
 * Yori shell display space used by files in directories
 *
 * Copyright (c) 2019 Malcolm J. Smith
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
CHAR strDuHelpText[] =
        "\n"
        "Display disk space used within directories.\n"
        "\n"
        "DU [-license] [-a] [-b] [-c] [-color] [-d] [-h] [-r <num>] [-s <size>]\n"
        "   [-w] [<spec>...]\n"
        "\n"
        "   -a             Enable all features for maximum accuracy\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Display compressed file size\n"
        "   -color         Use file color highlighting\n"
        "   -d             Include space used by alternate data streams\n"
        "   -h             Average space used across multiple hard links\n"
        "   -r <num>       The maximum recursion depth to display\n"
        "   -s <size>      Only display directories containing at least size bytes\n"
        "   -u             Round space up to file allocation unit or cluster size\n"
        "   -w             Count files backed by a WIM archive as zero size\n";

/**
 Display usage text to the user.
 */
BOOL
DuHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Du %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strDuHelpText);
    return TRUE;
}

/**
 The maximum size of the stream name in WIN32_FIND_STREAM_DATA.
 */
#define DU_MAX_STREAM_NAME          (MAX_PATH + 36)

/**
 A private definition of WIN32_FIND_STREAM_DATA in case the compilation
 environment doesn't provide it.
 */
typedef struct _DU_WIN32_FIND_STREAM_DATA {

    /**
     The length of the stream, in bytes.
     */
    LARGE_INTEGER StreamSize;

    /**
     The name of the stream.
     */
    WCHAR cStreamName[DU_MAX_STREAM_NAME];
} DU_WIN32_FIND_STREAM_DATA, *PDU_WIN32_FIND_STREAM_DATA;

/**
 A structure describing a particular directory.  When traversing through
 files to calculate space, there will be one of these structures for each
 parent component of the file.
 */
typedef struct _DU_DIRECTORY_STACK {

    /**
     The name of this directory, in escaped form.
     */
    YORI_STRING DirectoryName;

    /**
     The number of files or directories encountered within this directory.
     */
    LONGLONG ObjectsFoundThisDirectory;

    /**
     The amount of bytes consumed by files within this directory.
     */
    LONGLONG SpaceConsumedThisDirectory;

    /**
     The amount of bytes consumed by subdirectories within this directory.
     Note this is populated only when the subdirectories have completed
     their enumerations.
     */
    LONGLONG SpaceConsumedInChildren;

    /**
     The number of bytes in each file system allocation unit for this
     directory.  This is only meaningful if AllocationSize reporting is
     enabled.
     */
    LONGLONG AllocationSize;
} DU_DIRECTORY_STACK, *PDU_DIRECTORY_STACK;

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _DU_CONTEXT {

    /**
     An array of directory components corresponding to the components within
     the path currently being parsed.  File sizes are added to the leafmost
     element (ie., StackIndex) and when a change to a parent component is
     detected, size from the child component is propagated to the parent and
     the child component is prepared for reuse by the next child directory.
     */
    PDU_DIRECTORY_STACK DirStack;

    /**
     Index of the last element in the directory stack that has been operated
     on.
     */
    DWORD StackIndex;

    /**
     The number of allocated elements in the DirStack array.  Will be zero
     if the array has not been allocated yet.
     */
    DWORD StackAllocated;

    /**
     The maximum depth to display.  This is a user specified value allowing
     recursion to terminate at a particular level to display a summary view
     only at a particular depth.
     */
    DWORD MaximumDepthToDisplay;

    /**
     Round file size up to allocation unit
     */
    BOOL AllocationSize;

    /**
     Display compressed file size, as opposed to logical file size.
     */
    BOOL CompressedFileSize;

    /**
     Average size across multiple hard links.
     */
    BOOL AverageHardLinkSize;

    /**
     Count space used by alternate data streams on the file.
     */
    BOOL IncludeNamedStreams;

    /**
     Count WIM backed files as zero size, because the space is accounted for
     as the WIM file itself.
     */
    BOOL WimBackedFilesAsZero;

    /**
     The color to display file sizes in.
     */
    YORILIB_COLOR_ATTRIBUTES FileSizeColor;

    /**
     The minimum directory size to display.
     */
    LARGE_INTEGER MinimumDirectorySizeToDisplay;

    /**
     A string form of the VT sequence for the file color above.
     */
    YORI_STRING FileSizeColorString;

    /**
     The buffer for the above string.
     */
    TCHAR FileSizeColorStringBuffer[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];

    /**
     Color information to display against matching files.
     */
    YORI_LIB_FILE_FILTER ColorRules;

} DU_CONTEXT, *PDU_CONTEXT;

/**
 Deallocate all child allocations within a DU_CONTEXT structure.  The
 structure itself is typically stack allocated and will not be freed.

 @param DuContext Pointer to the DuContext to clean up.
 */
VOID
DuCleanupContext(
    __in PDU_CONTEXT DuContext
    )
{
    DWORD Index;

    for (Index = 0; Index < DuContext->StackAllocated; Index++) {
        YoriLibFreeStringContents(&DuContext->DirStack[Index].DirectoryName);
    }

    if (DuContext->DirStack != NULL) {
        YoriLibFree(DuContext->DirStack);
        DuContext->DirStack = NULL;
    }

    DuContext->StackAllocated = 0;
    DuContext->StackIndex = 0;
    YoriLibFileFiltFreeFilter(&DuContext->ColorRules);
}

/**
 Clear the contents of a directory frame so it can be reused.

 @param DirStack Pointer to the directory frame to clear.
 */
VOID
DuCloseStack(
    __in PDU_DIRECTORY_STACK DirStack
    )
{
    //
    //  Note the DirectoryName string remains allocated in the hope that the
    //  next directory can use it.
    //

    DirStack->DirectoryName.LengthInChars = 0;
    DirStack->ObjectsFoundThisDirectory = 0;
    DirStack->SpaceConsumedThisDirectory = 0;
    DirStack->SpaceConsumedInChildren = 0;
}

/**
 Print the space consumed by a particular directory, and close out the
 directory's stack frame so it can be reused by the next directory.

 @param DuContext Pointer to the DuContext which contains the directory to
        display and close.

 @param Depth Specifies the array index of the directory to display and close.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
DuReportAndCloseStack(
    __in PDU_CONTEXT DuContext,
    __in DWORD Depth
    )
{
    YORI_STRING UnescapedPath;
    PYORI_STRING StringToDisplay;
    YORI_STRING FileSizeString;
    TCHAR FileSizeStringBuffer[8];
    LARGE_INTEGER SizeToDisplay;
    YORI_STRING VtAttribute;
    TCHAR VtAttributeBuffer[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];
    YORILIB_COLOR_ATTRIBUTES Attribute;

    PDU_DIRECTORY_STACK DirStack;

    DirStack = &DuContext->DirStack[Depth];

    if (DuContext->MaximumDepthToDisplay == 0 ||
        Depth <= DuContext->MaximumDepthToDisplay) {

        SizeToDisplay.QuadPart = DirStack->SpaceConsumedInChildren + DirStack->SpaceConsumedThisDirectory;

        if (DuContext->MinimumDirectorySizeToDisplay.QuadPart == 0 ||
            SizeToDisplay.QuadPart >= DuContext->MinimumDirectorySizeToDisplay.QuadPart) {

            //
            //  Convert the escaped path into a path for humans.
            //

            YoriLibInitEmptyString(&UnescapedPath);
            if (YoriLibUnescapePath(&DirStack->DirectoryName, &UnescapedPath)) {
                StringToDisplay = &UnescapedPath;
            } else {
                StringToDisplay = &DirStack->DirectoryName;
            }

            //
            //  Convert the file size from a number of bytes to a short string
            //  with a suffix
            //

            YoriLibInitEmptyString(&FileSizeString);
            FileSizeString.StartOfString = FileSizeStringBuffer;
            FileSizeString.LengthAllocated = sizeof(FileSizeStringBuffer)/sizeof(FileSizeStringBuffer[0]);
            YoriLibFileSizeToString(&FileSizeString, &SizeToDisplay);

            //
            //  If the user requested it, determine the color to display with
            //

            YoriLibInitEmptyString(&VtAttribute);
            if (DuContext->ColorRules.NumberCriteria) {
                WIN32_FIND_DATA FileInfo;

                VtAttribute.StartOfString = VtAttributeBuffer;
                VtAttribute.LengthAllocated = sizeof(VtAttributeBuffer)/sizeof(VtAttributeBuffer[0]);

                if (!YoriLibUpdateFindDataFromFileInformation(&FileInfo, DirStack->DirectoryName.StartOfString, TRUE) || 
                    !YoriLibFileFiltCheckColorMatch(&DuContext->ColorRules, &DirStack->DirectoryName, &FileInfo, &Attribute)) {
                    Attribute.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG;
                    Attribute.Win32Attr = (UCHAR)YoriLibVtGetDefaultColor();
                }

                YoriLibVtStringForTextAttribute(&VtAttribute, Attribute.Ctrl, Attribute.Win32Attr);
            }

            if (VtAttribute.LengthInChars > 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                              _T("%y%y%c[0m %y%y%c[0m\n"),
                              &DuContext->FileSizeColorString,
                              &FileSizeString,
                              27,
                              &VtAttribute,
                              StringToDisplay,
                              27);
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y %y\n"), &FileSizeString, StringToDisplay);
            }

            YoriLibFreeStringContents(&UnescapedPath);
        }
    }

    DuCloseStack(DirStack);
    return TRUE;
}

/**
 Display all directory frames which have not been otherwise displayed because
 no object has been found in a subsequent directory.

 @param DuContext Pointer to the DuContext which may contain active directory
        frames.

 @param MinDepthToDisplay Indicates the minimum depth number that should be
        displayed to the user.  Frames below this are cleared for reuse but
        not displayed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
DuReportAndCloseAllActiveStacks(
    __in PDU_CONTEXT DuContext,
    __in DWORD MinDepthToDisplay
    )
{
    DWORD Index;
    Index = DuContext->StackIndex;
    if (Index >= DuContext->StackAllocated) {
        ASSERT(Index == 0);
        ASSERT(DuContext->DirStack == NULL);
        return TRUE;
    }
    while (TRUE) {
        if (Index > 0) {
            DuContext->DirStack[Index - 1].SpaceConsumedInChildren +=
                DuContext->DirStack[Index].SpaceConsumedInChildren +
                DuContext->DirStack[Index].SpaceConsumedThisDirectory;
        }
        if (Index >= MinDepthToDisplay) {
            DuReportAndCloseStack(DuContext, Index);
        } else {
            DuCloseStack(&DuContext->DirStack[Index]);
        }
        if (Index == 0) {
            break;
        }
        Index--;
        DuContext->StackIndex--;
    }
    return TRUE;
}

/**
 Initialize a single directory stack location.

 @param DuContext Pointer to the DU context specifying the options to apply.

 @param DirStack Pointer to the directory stack location to initialize.

 @param DirName Pointer to the directory name to initialize in the stack.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
DuInitializeDirectoryStack(
    __in PDU_CONTEXT DuContext,
    __in PDU_DIRECTORY_STACK DirStack,
    __in PYORI_STRING DirName
    )
{
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD NumberOfFreeClusters;
    DWORD TotalNumberOfClusters;

    if (DirStack->DirectoryName.LengthAllocated <= DirName->LengthInChars) {
        YoriLibFreeStringContents(&DirStack->DirectoryName);
        if (!YoriLibAllocateString(&DirStack->DirectoryName, DirName->LengthInChars + 80)) {
            return FALSE;
        }
    }

    memcpy(DirStack->DirectoryName.StartOfString, DirName->StartOfString, DirName->LengthInChars * sizeof(TCHAR));
    DirStack->DirectoryName.StartOfString[DirName->LengthInChars] = '\0';
    DirStack->DirectoryName.LengthInChars = DirName->LengthInChars;

    //
    //  If GetDiskFreeSpace fails, see if it works on the effective root.
    //  This is to support systems without mount points where this call can
    //  fail when called on a directory.
    //

    if (DuContext->AllocationSize) {
        if (!GetDiskFreeSpace(DirStack->DirectoryName.StartOfString, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters)) {
            YORI_STRING EffectiveRoot;

            DirStack->AllocationSize = 4096;

            if (YoriLibFindEffectiveRoot(&DirStack->DirectoryName, &EffectiveRoot) &&
                EffectiveRoot.LengthInChars < DirStack->DirectoryName.LengthInChars) {

                TCHAR SavedChar;
                SavedChar = EffectiveRoot.StartOfString[EffectiveRoot.LengthInChars];
                EffectiveRoot.StartOfString[EffectiveRoot.LengthInChars] = '\0';

                if (GetDiskFreeSpace(EffectiveRoot.StartOfString, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters)) {
                    DirStack->AllocationSize = SectorsPerCluster * BytesPerSector;
                }

                EffectiveRoot.StartOfString[EffectiveRoot.LengthInChars] = SavedChar;
            }

        } else {
            DirStack->AllocationSize = SectorsPerCluster * BytesPerSector;
        }
    }

    return TRUE;
}

/**
 Count the amount of disk space to attribute to a file given the user selected
 options.

 @param DuContext Context specifying the accounting options to apply.

 @param DirStack Pointer to the directory stack indicating the allocation size
        used for the directory.

 @param FilePath Pointer to a fully specified path to the file.

 @param FileInfo Pointer to the block of data returned from directory
        enumerate.

 @return The number of bytes attributable to the file.
 */
LARGE_INTEGER
DuCalculateSpaceUsedByFile(
    __in PDU_CONTEXT DuContext,
    __in PDU_DIRECTORY_STACK DirStack,
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo
    )
{
    LARGE_INTEGER FileSize;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    BOOL ForceSizeZero = FALSE;
    BOOL ReportedOpenError = FALSE;

    FileSize.QuadPart = 0;

    if (DuContext->AverageHardLinkSize || DuContext->WimBackedFilesAsZero) {

        FileHandle = CreateFile(FilePath->StartOfString,
                                FILE_READ_ATTRIBUTES|SYNCHRONIZE,
                                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_OPEN_NO_RECALL | FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);
        if (FileHandle == INVALID_HANDLE_VALUE) {
            DWORD ErrorCode = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(ErrorCode);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of %y failed, results inaccurate: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            ReportedOpenError = TRUE;
        }
    }

    //
    //  If the file is WIM backed and the user requested it, count the default
    //  stream size as zero.
    //

    if (DuContext->WimBackedFilesAsZero) {
        struct {
            WOF_EXTERNAL_INFO WofHeader;
            union {
                WIM_PROVIDER_EXTERNAL_INFO WimInfo;
                FILE_PROVIDER_EXTERNAL_INFO FileInfo;
            } u;
        } WofInfo;
        DWORD BytesReturned;

        if (DeviceIoControl(FileHandle, FSCTL_GET_EXTERNAL_BACKING, NULL, 0, &WofInfo, sizeof(WofInfo), &BytesReturned, NULL)) {
            if (WofInfo.WofHeader.Provider == WOF_PROVIDER_WIM) {
                FileSize.QuadPart = 0;
                ForceSizeZero = TRUE;
            }
        }
    }

    //
    //  If the default stream size wasn't forced to zero above, calculate it
    //  now as either the compressed or uncompressed file size.
    //

    if (!ForceSizeZero) {
        if (DuContext->CompressedFileSize &&
            DllKernel32.pGetCompressedFileSizeW) {

            FileSize.LowPart = DllKernel32.pGetCompressedFileSizeW(FilePath->StartOfString, (PDWORD)&FileSize.HighPart);

            if (FileSize.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
                FileSize.LowPart = FileInfo->nFileSizeLow;
                FileSize.HighPart = FileInfo->nFileSizeHigh;
            }
        } else {
            FileSize.LowPart = FileInfo->nFileSizeLow;
            FileSize.HighPart = FileInfo->nFileSizeHigh;
        }
    }

    //
    //  Round up to allocation size if requested.
    //

    if (DuContext->AllocationSize) {
        FileSize.QuadPart = (FileSize.QuadPart + DirStack->AllocationSize - 1) & (~(DirStack->AllocationSize - 1));
    }

    //
    //  Add in any space used by alternate streams.  Note in particular that
    //  WIM backed files will only have their default stream WIM backed, so
    //  these are added in even in the ForceSizeZero case.
    //

    if (DuContext->IncludeNamedStreams &&
        DllKernel32.pFindFirstStreamW &&
        DllKernel32.pFindNextStreamW) {

        HANDLE hFind;
        WIN32_FIND_STREAM_DATA FindStreamData;

        hFind = DllKernel32.pFindFirstStreamW(FilePath->StartOfString, 0, &FindStreamData, 0);
        if (hFind == INVALID_HANDLE_VALUE) {
            if (!ReportedOpenError) {
                DWORD ErrorCode = GetLastError();
                LPTSTR ErrText = YoriLibGetWinErrorText(ErrorCode);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of %y failed, results inaccurate: %s"), FilePath, ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
        } else {
            do {
                if (_tcscmp(FindStreamData.cStreamName, L"::$DATA") != 0) {
                    FileSize.QuadPart += FindStreamData.StreamSize.QuadPart;
                    if (DuContext->AllocationSize) {
                        FileSize.QuadPart = (FileSize.QuadPart + DirStack->AllocationSize - 1) & (~(DirStack->AllocationSize - 1));
                    }
                }
            } while (DllKernel32.pFindNextStreamW(hFind, &FindStreamData));
            FindClose(hFind);
        }
    }

    //
    //  If the file has a size and hardlink averaging is reuqested, divide the
    //  size found by the number of hard links.
    //

    if (DuContext->AverageHardLinkSize && FileHandle != INVALID_HANDLE_VALUE && FileSize.QuadPart != 0) {
        BY_HANDLE_FILE_INFORMATION HandleFileInfo;

        if (GetFileInformationByHandle(FileHandle, &HandleFileInfo)) {
            if (HandleFileInfo.nNumberOfLinks > 1) {
                FileSize.QuadPart = FileSize.QuadPart / HandleFileInfo.nNumberOfLinks;
            }
        }
    }

    if (FileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(FileHandle);
    }


    return FileSize;
}



/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the du context structure indicating the
        action to perform and populated with the number of objects found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
DuFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PDU_CONTEXT DuContext = (PDU_CONTEXT)Context;
    LPTSTR FilePart;
    DWORD Index;

    if (Depth >= DuContext->StackAllocated) {
        PDU_DIRECTORY_STACK NewStack;
        NewStack = YoriLibMalloc((Depth + 8) * sizeof(DU_DIRECTORY_STACK));
        if (NewStack == NULL) {
            return FALSE;
        }

        if (DuContext->StackAllocated > 0) {
            memcpy(NewStack, DuContext->DirStack, DuContext->StackAllocated * sizeof(DU_DIRECTORY_STACK));
            YoriLibFree(DuContext->DirStack);
        }

        for (Index = DuContext->StackAllocated; Index < Depth + 8; Index++) {
            YoriLibInitEmptyString(&NewStack[Index].DirectoryName);
            NewStack[Index].ObjectsFoundThisDirectory = 0;
            NewStack[Index].SpaceConsumedThisDirectory = 0;
            NewStack[Index].SpaceConsumedInChildren = 0;
        }

        DuContext->DirStack = NewStack;
        DuContext->StackAllocated = Depth + 8;
    }

    //
    //  StackIndex would normally be populated except for the first item at
    //  Depth == 0
    //

    if (DuContext->StackIndex > 0 || DuContext->DirStack[0].DirectoryName.LengthInChars > 0) {
        Index = DuContext->StackIndex;
        while (TRUE) {
            DWORD StackDirLength;
            StackDirLength = DuContext->DirStack[Index].DirectoryName.LengthInChars;
            if (Depth >= Index &&
                YoriLibCompareStringCount(FilePath, &DuContext->DirStack[Index].DirectoryName, StackDirLength) == 0 &&
                (FilePath->LengthInChars == StackDirLength ||
                 YoriLibIsSep(FilePath->StartOfString[StackDirLength]) ||
                 YoriLibIsSep(DuContext->DirStack[Index].DirectoryName.StartOfString[StackDirLength - 1]))) {

                break;
            }

            if (Index > 0) {
                DuContext->DirStack[Index - 1].SpaceConsumedInChildren +=
                    DuContext->DirStack[Index].SpaceConsumedInChildren +
                    DuContext->DirStack[Index].SpaceConsumedThisDirectory;
            }
            DuReportAndCloseStack(DuContext, Index);
            if (Index == 0) {
                break;
            }
            Index--;
            DuContext->StackIndex--;
        }
    }

    FilePart = YoriLibFindRightMostCharacter(FilePath, '\\');
    ASSERT(FilePart != NULL);
    if (FilePart != NULL) {
        YORI_STRING ThisDirName;
        YoriLibInitEmptyString(&ThisDirName);
        ThisDirName.StartOfString = FilePath->StartOfString;
        ThisDirName.LengthInChars = (DWORD)(FilePart - FilePath->StartOfString);
        if (ThisDirName.LengthInChars == 6) {
            ThisDirName.LengthInChars++;
            if (!YoriLibIsPrefixedDriveLetterWithColonAndSlash(&ThisDirName)) {
                ThisDirName.LengthInChars--;
            }
        }

        Index = Depth;
        while (TRUE) {
            if (DuContext->DirStack[Index].DirectoryName.LengthInChars > 0) {

                ASSERT(Index == DuContext->StackIndex);
                ASSERT(YoriLibCompareString(&DuContext->DirStack[Index].DirectoryName, &ThisDirName) == 0);
                break;
            }
            if (!DuInitializeDirectoryStack(DuContext, &DuContext->DirStack[Index], &ThisDirName)) {
                return FALSE;
            }
            ASSERT(DuContext->DirStack[Index].ObjectsFoundThisDirectory == 0);
            ASSERT(DuContext->DirStack[Index].SpaceConsumedThisDirectory == 0);
            ASSERT(DuContext->DirStack[Index].SpaceConsumedInChildren == 0);
            if (Index == 0) {
                break;
            }
            Index--;
            FilePart = YoriLibFindRightMostCharacter(&ThisDirName, '\\');
            ASSERT(FilePart != NULL);
            ThisDirName.LengthInChars = (DWORD)(FilePart - ThisDirName.StartOfString);
            if (ThisDirName.LengthInChars == 6) {
                ThisDirName.LengthInChars++;
                if (!YoriLibIsPrefixedDriveLetterWithColonAndSlash(&ThisDirName)) {
                    ThisDirName.LengthInChars--;
                }
            }
        }
    }

    DuContext->StackIndex = Depth;
    DuContext->DirStack[Depth].ObjectsFoundThisDirectory++;

    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        LARGE_INTEGER FileSize;
        FileSize = DuCalculateSpaceUsedByFile(DuContext, &DuContext->DirStack[Depth], FilePath, FileInfo);
        DuContext->DirStack[Depth].SpaceConsumedThisDirectory += FileSize.QuadPart;
    }

    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the du context structure indicating the
        action to perform and populated with the number of objects found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
DuFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    LPTSTR ErrText = YoriLibGetWinErrorText(ErrorCode);
    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(Context);
    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Enumerate of %y failed, results incomplete: %s"), FilePath, ErrText);
    YoriLibFreeWinErrorText(ErrText);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the du builtin command.
 */
#define ENTRYPOINT YoriCmd_YDU
#else
/**
 The main entrypoint for the du standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the du cmdlet.

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
    DWORD i;
    DWORD StartArg = 0;
    DWORD MatchFlags;
    BOOL BasicEnumeration = FALSE;
    DU_CONTEXT DuContext;
    YORI_STRING Combined;
    YORI_STRING Arg;

    ZeroMemory(&DuContext, sizeof(DuContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                DuHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                DuContext.CompressedFileSize = TRUE;
                DuContext.IncludeNamedStreams = TRUE;
                DuContext.AverageHardLinkSize = TRUE;
                DuContext.AllocationSize = TRUE;
                DuContext.WimBackedFilesAsZero = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                DuContext.CompressedFileSize = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                DuContext.IncludeNamedStreams = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                DuContext.AverageHardLinkSize = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                if (i + 1 < ArgC) {
                    LONGLONG Depth;
                    DWORD CharsConsumed;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &Depth, &CharsConsumed);
                    if (CharsConsumed > 0) {
                        DuContext.MaximumDepthToDisplay = (DWORD)Depth;
                    }
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                if (i + 1 < ArgC) {
                    DuContext.MinimumDirectorySizeToDisplay = YoriLibStringToFileSize(&ArgV[i + 1]);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("u")) == 0) {
                DuContext.AllocationSize = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("w")) == 0) {
                DuContext.WimBackedFilesAsZero = TRUE;
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

    if (YoriLibLoadCombinedFileColorString(NULL, &Combined)) {
        YORI_STRING ErrorSubstring;
        if (!YoriLibFileFiltParseColorString(&DuContext.ColorRules, &Combined, &ErrorSubstring)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("du: parse error at %y\n"), &ErrorSubstring);
        }
        YoriLibFreeStringContents(&Combined);
    }

    YoriLibConstantString(&Combined, _T("fs"));
    if (!YoriLibGetMetadataColor(&Combined, &DuContext.FileSizeColor)) {
        DuContext.FileSizeColor.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG;
        DuContext.FileSizeColor.Win32Attr = (UCHAR)YoriLibVtGetDefaultColor();
    }

    DuContext.FileSizeColorString.StartOfString = DuContext.FileSizeColorStringBuffer;
    DuContext.FileSizeColorString.LengthAllocated = YORI_MAX_INTERNAL_VT_ESCAPE_CHARS;

    YoriLibVtStringForTextAttribute(&DuContext.FileSizeColorString, DuContext.FileSizeColor.Ctrl, DuContext.FileSizeColor.Win32Attr);

    YoriLibEnableBackupPrivilege();

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    MatchFlags = YORILIB_FILEENUM_RETURN_FILES |
                 YORILIB_FILEENUM_RETURN_DIRECTORIES |
                 YORILIB_FILEENUM_RECURSE_BEFORE_RETURN |
                 YORILIB_FILEENUM_NO_LINK_TRAVERSE;
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    //
    //  If no file name is specified, use .
    //

    if (StartArg == 0 || StartArg == ArgC) {
        YORI_STRING FilesInDirectorySpec;
        YoriLibConstantString(&FilesInDirectorySpec, _T("."));
        YoriLibForEachFile(&FilesInDirectorySpec, MatchFlags, 0, DuFileFoundCallback, NULL, &DuContext);
        DuReportAndCloseAllActiveStacks(&DuContext, 1);
    } else {
        for (i = StartArg; i < ArgC; i++) {
            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, DuFileFoundCallback, DuFileEnumerateErrorCallback, &DuContext);
            DuReportAndCloseAllActiveStacks(&DuContext, 1);
        }
    }

    DuCleanupContext(&DuContext);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
