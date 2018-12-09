/**
 * @file sdir/sdir.c
 *
 * Colorful, sorted and optionally rich directory enumeration
 * for Windows.
 *
 * This module implements the core logic of displaying directories.
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
 Specifies the number of directory entries that are currently allocated.
 */
DWORD SdirAllocatedDirents;

/**
 Pointer to an array of directory entries.  This corresponds to files in
 a single directory, populated in response to enumerate.
 */
PYORI_FILE_INFO SdirDirCollection;

/**
 Pointer to an array of pointers to directory entries.  These pointers
 are maintained sorted based on the user's sort criteria so that files
 can be displayed in order from this indirection.
 */
PYORI_FILE_INFO * SdirDirSorted;

/**
 Specifies the number of allocated directory entries that have been
 populated with files returned from directory enumerate.
 */
ULONG SdirDirCollectionCurrent;

/**
 Specifies the longest file name found so far when enumerating through a
 single directory.  This length in in characters.
 */
ULONG SdirDirCollectionLongest;

/**
 Specifies the total length of characters stored in file names during
 enumeration of a single directory.  This is used to truncate file names
 if they start to look too long.
 */
ULONG SdirDirCollectionTotalNameLength;

/**
 Pointer to a dynamically allocated options structure which contains run
 time configuration about the application.
 */
PSDIR_OPTS Opts;

/**
 Pointer to a dynamically allocated summary structure recording the number
 of files and directories found during enumerate and their sizes.
 */
PSDIR_SUMMARY Summary;


BOOL
SdirDisplayCollection();

/**
 Capture all required information from a file found by the system into a
 directory entry.

 @param CurrentEntry Pointer to a directory entry to populate with
        information.

 @param FindData Information returned by the system when enumerating files.

 @param FullPath Pointer to a string referring to the full path to the file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirCaptureFoundItemIntoDirent (
    __out PYORI_FILE_INFO CurrentEntry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    ) 
{
    DWORD i;

    //
    //  Copy over the data from Win32's FindFirstFile into our own structure.
    //

    for (i = 0; i < SdirGetNumSdirOptions(); i++) {

        PSDIR_FEATURE Feature;
        Feature = SdirFeatureByOptionNumber(i);

        //
        //  If we're displaying or sorting, we need the data to use
        //

        if ((Feature->Flags & SDIR_FEATURE_COLLECT) &&
               SdirOptions[i].CollectFn) {

            SdirOptions[i].CollectFn(CurrentEntry, FindData, FullPath);
        }
    }

    //
    //  Determine the color to display each entry from extensions and attributes.
    //  If we're asked to hide, just walk away from the entry we created and
    //  continue.
    //
    
    SdirApplyAttribute(CurrentEntry, &CurrentEntry->RenderAttributes);

    return TRUE;
}

/**
 Generate information typically returned from a directory enumeration by
 opening the file and querying information from it.  This is used for named
 streams which do not go through a regular file enumeration.

 @param FindData On successful completion, populated with information 
        typically returned by the system when enumerating files.

 @param FullPath Pointer to a NULL terminate string referring to the full
        path to the file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUpdateFindDataFromFileInformation (
    __out PWIN32_FIND_DATA FindData,
    __in LPTSTR FullPath
    )
{
    HANDLE hFile;
    BY_HANDLE_FILE_INFORMATION FileInfo;

    hFile = CreateFile(FullPath,
                       FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {

        GetFileInformationByHandle(hFile, &FileInfo);

        FindData->dwFileAttributes = FileInfo.dwFileAttributes;
        FindData->ftCreationTime = FileInfo.ftCreationTime;
        FindData->ftLastAccessTime = FileInfo.ftLastAccessTime;
        FindData->ftLastWriteTime = FileInfo.ftLastWriteTime;
        FindData->nFileSizeHigh = FileInfo.nFileSizeHigh;
        FindData->nFileSizeLow  = FileInfo.nFileSizeLow;

        CloseHandle(hFile);
        return TRUE;
    }
    return FALSE;
}

/**
 Capture enough state about a file from its path to determine the color
 to display it with.  This is used when displaying directory names as part
 of recursive enumerations where the directories aren't rendered as part of
 the regular display but we still want to display them with color.

 @param FullPath A fully specified path to an object (which is currently a
        directory.)

 @return The color attributes to use to display the object.
 */
YORILIB_COLOR_ATTRIBUTES
SdirRenderAttributesFromPath (
    __in PYORI_STRING FullPath
    )
{
    HANDLE hFind;
    WIN32_FIND_DATA FindData;
    YORI_FILE_INFO CurrentEntry;

    memset(&CurrentEntry, 0, sizeof(CurrentEntry));

    hFind = FindFirstFile(FullPath->StartOfString, &FindData);
    if (hFind != INVALID_HANDLE_VALUE) {
        SdirCaptureFoundItemIntoDirent(&CurrentEntry, &FindData, FullPath);
        FindClose(hFind);
        return CurrentEntry.RenderAttributes;
    } else {
        YORI_STRING DummyString;

        if (!YoriLibAllocateString(&DummyString, FullPath->LengthInChars + 2)) {
            return SdirDefaultColor;
        };

        memset(&FindData, 0, sizeof(FindData));
        DummyString.LengthInChars = YoriLibSPrintfS(DummyString.StartOfString, DummyString.LengthAllocated, _T("%s\\"), FullPath);
        SdirUpdateFindDataFromFileInformation(&FindData, DummyString.StartOfString);
        SdirCaptureFoundItemIntoDirent(&CurrentEntry, &FindData, &DummyString);
        YoriLibFreeStringContents(&DummyString);
        return CurrentEntry.RenderAttributes;
    }
}

/**
 Add a single found object to the set of files found so far.

 @param FindData Pointer to the block of data returned from the directory as
        part of the enumeration.

 @param FullPath Pointer to a fully specified file name for the file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirAddToCollection (
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    ) 
{
    PYORI_FILE_INFO CurrentEntry;
    DWORD i, j;
    DWORD CompareResult = 0;

    if (SdirDirCollectionCurrent >= SdirAllocatedDirents) {
        if (SdirDirCollectionCurrent < UINT_MAX) {
            SdirDirCollectionCurrent++;
        }
        return FALSE;
    }

    CurrentEntry = &SdirDirCollection[SdirDirCollectionCurrent];

    SdirDirCollectionCurrent++;

    SdirCaptureFoundItemIntoDirent(CurrentEntry, FindData, FullPath);

    if (Opts->BriefRecurseDepth == 0 &&
        CurrentEntry->RenderAttributes.Ctrl & SDIR_ATTRCTRL_HIDE) {

        SdirDirCollectionCurrent--;
        return TRUE;
    }

    if (CurrentEntry->FileNameLengthInChars > SdirDirCollectionLongest) {
        SdirDirCollectionLongest = CurrentEntry->FileNameLengthInChars;
    }

    SdirDirCollectionTotalNameLength += CurrentEntry->FileNameLengthInChars;

    if (Opts->FtSummary.Flags & SDIR_FEATURE_COLLECT) {
        SdirCollectSummary(CurrentEntry);
    }

    //
    //  Now that our internal entry is fully poulated, insert it into the correct sorted position.
    //  As an optimization, check if we just need to insert at the end (for file name sort on
    //  NTFS, this is the common case.)
    //

    if (SdirDirCollectionCurrent > 1 &&
        Opts->Sort[0].CompareFn(SdirDirSorted[SdirDirCollectionCurrent - 2], CurrentEntry) == Opts->Sort[0].CompareInverseCondition) {

        SdirDirSorted[SdirDirCollectionCurrent - 1] = CurrentEntry;
        return TRUE;
    }

    //
    //  MSFIX This algorithm seems really stupid.  Since we know the sort
    //  list is sorted, shouldn't we binary search?
    //

    for (i = 0; i < SdirDirCollectionCurrent - 1; i++) {

        for (j = 0; j < Opts->CurrentSort; j++) {
            CompareResult = Opts->Sort[j].CompareFn(SdirDirSorted[i], CurrentEntry);

            if (CompareResult == Opts->Sort[j].CompareBreakCondition ||
                CompareResult == Opts->Sort[j].CompareInverseCondition) {
                break;
            }
        }

        if (CompareResult == Opts->Sort[j].CompareBreakCondition) {
            for (j = SdirDirCollectionCurrent - 1; j > i; j--) {
                SdirDirSorted[j] = SdirDirSorted[j - 1];
            }
            SdirDirSorted[i] = CurrentEntry;
            return TRUE;
        }
    }
    SdirDirSorted[SdirDirCollectionCurrent - 1] = CurrentEntry;
    return TRUE;
}

/**
 A context structure passed around through all files found as part of a single
 enumerate request.
 */
typedef struct _SDIR_ITEM_FOUND_CONTEXT {

    /**
     The number of items that were found as part of a single enumerate
     request.
     */
    DWORD ItemsFound;
} SDIR_ITEM_FOUND_CONTEXT, *PSDIR_ITEM_FOUND_CONTEXT;

/**
 A callback invoked by @ref SdirEnumeratePathWithDepth for every file found.

 @param FullPath Pointer to a full, escaped path to the file.

 @param FindData Points to information returned from the directory enumerate.

 @param Depth Specifies the recursion depth.  This should be zero and is
        ignored.

 @param Context Pointer to a an SDIR_ITEM_FOUND_CONTEXT block for all objects
        found via a single enumerate request.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirItemFoundCallback(
    __in PYORI_STRING FullPath,
    __in PWIN32_FIND_DATA FindData,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PSDIR_ITEM_FOUND_CONTEXT ItemContext = (PSDIR_ITEM_FOUND_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

#if defined(UNICODE)
    if (DllKernel32.pFindFirstStreamW != NULL &&
        (Opts->FtNamedStreams.Flags & SDIR_FEATURE_DISPLAY)) {

        HANDLE hStreamFind;
        WIN32_FIND_STREAM_DATA FindStreamData;
        WIN32_FIND_DATA BogusFindData;
        YORI_STRING StreamFullPath;

        //
        //  Display the default stream
        //

        SdirAddToCollection(FindData, FullPath);

        //
        //  Look for any named streams
        //

        hStreamFind = DllKernel32.pFindFirstStreamW(FullPath->StartOfString, 0, &FindStreamData, 0);
        if (hStreamFind != INVALID_HANDLE_VALUE) {

            if (!YoriLibAllocateString(&StreamFullPath, FullPath->LengthInChars + YORI_LIB_MAX_STREAM_NAME)) {
                FindClose(hStreamFind);
                return FALSE;
            }

            do {

                //
                //  For the default stream, just report the information we found
                //  for the file.  For anything else, query all the equivalent
                //  stream information
                //

                if (_tcscmp(FindStreamData.cStreamName, L"::$DATA") != 0) {

                    DWORD StreamLength = (DWORD)_tcslen(FindStreamData.cStreamName);

                    if (StreamLength > 6 && _tcscmp(FindStreamData.cStreamName + StreamLength - 6, L":$DATA") == 0) {
                        FindStreamData.cStreamName[StreamLength - 6] = '\0';
                    }

                    StreamFullPath.LengthInChars = YoriLibSPrintfS(StreamFullPath.StartOfString, StreamFullPath.LengthAllocated, _T("%s%s%s"), Opts->ParentName.StartOfString, FindData->cFileName, FindStreamData.cStreamName);

                    //
                    //  Assume file state is stream state
                    //

                    memcpy(&BogusFindData, &FindData, sizeof(FindData));

                    //
                    //  Populate stream name
                    //

                    YoriLibSPrintfS(BogusFindData.cFileName, sizeof(BogusFindData.cFileName)/sizeof(BogusFindData.cFileName[0]), _T("%s%s"), FindData->cFileName, FindStreamData.cStreamName);

                    //
                    //  Populate stream information
                    //

                    SdirUpdateFindDataFromFileInformation(&BogusFindData, StreamFullPath.StartOfString);
                    SdirAddToCollection(&BogusFindData, &StreamFullPath);
                }
            } while (DllKernel32.pFindNextStreamW(hStreamFind, &FindStreamData));

            //
            //  MSFIX Keep this on the context so we can reuse it
            //

            YoriLibFreeStringContents(&StreamFullPath);
        }

        FindClose(hStreamFind);

    } else {
#endif
        SdirAddToCollection(FindData, FullPath);
#if defined(UNICODE)
    }
#endif
    ItemContext->ItemsFound++;
    return TRUE;
}


/**
 Enumerate all of the files in a given single directory/wildcard pattern,
 and populate the results into the global SdirAllocatedDirents array.

 @param FindStr The compound directory/wildcard pattern to enumerate.

 @param Depth Recursion depth.  If nonzero, this implies basic enumeration.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirEnumeratePathWithDepth (
    __in PYORI_STRING FindStr,
    __in DWORD Depth
    )
{
    LPTSTR FinalPart;
    DWORD SizeCopied;
    ULONG DirEntsToPreserve;
    PYORI_FILE_INFO NewSdirDirCollection;
    PYORI_FILE_INFO * NewSdirDirSorted;
    SDIR_ITEM_FOUND_CONTEXT ItemFoundContext;
    DWORD MatchFlags;

    //
    //  At this point we should have a directory and an enumeration criteria.
    //  Copy the directory name and clobber the enumeration criteria.
    //
    
    if (Opts->ParentName.MemoryToFree != NULL) {
        YoriLibFreeStringContents(&Opts->ParentName);
    }

    if (!YoriLibGetFullPathNameReturnAllocation(FindStr, TRUE, &Opts->ParentName, &FinalPart)) {
        SdirDisplayError(GetLastError(), _T("YoriLibGetFullPathNameReturnAllocation"));
        return FALSE;
    }

    if (FinalPart) {
        Opts->ParentName.LengthInChars = (DWORD)(FinalPart - Opts->ParentName.StartOfString);
        *FinalPart = '\0';
    }

    if (Opts->FtSummary.Flags & SDIR_FEATURE_COLLECT) {

        LARGE_INTEGER Junk;

        //
        //  If GetDiskFreeSpaceEx fails, fall back to GetDiskFreeSpace.  According
        //  to MSDN, this can sometimes be present and fail with not supported.
        //

        if (Summary->VolumeSize.QuadPart == 0 &&
            (DllKernel32.pGetDiskFreeSpaceExW == NULL ||
             !DllKernel32.pGetDiskFreeSpaceExW(Opts->ParentName.StartOfString, &Junk, &Summary->VolumeSize, &Summary->FreeSize))) {

            if (!SdirPopulateSummaryWithGetDiskFreeSpace(Opts->ParentName.StartOfString, Summary)) {

                //
                //  On very old platforms, this API requires a volume root.
                //  Frustratingly, that also means the APIs to detect a volume
                //  root don't exist, so all we can do is guess.  This code
                //  currently guesses for drive letters but not SMB shares;
                //  (my patience for NT 3.x support is not limitless.)
                //

                TCHAR BackupChar;
                DWORD VolumeRootLength = 0;

                if (VolumeRootLength == 0) {

                    if (Opts->ParentName.StartOfString[0] == '\\' &&
                        Opts->ParentName.StartOfString[1] == '\\' &&
                        (Opts->ParentName.StartOfString[2] == '?' || Opts->ParentName.StartOfString[2] == '.') &&
                        Opts->ParentName.StartOfString[3] == '\\' &&
                        Opts->ParentName.StartOfString[5] == ':' &&
                        Opts->ParentName.StartOfString[6] == '\\') {
    
                        VolumeRootLength = 7;
    
                    } else if (Opts->ParentName.StartOfString[1] == ':' &&
                               Opts->ParentName.StartOfString[2] == '\\') {
    
                        VolumeRootLength = 3;
                    }
                }

                if (VolumeRootLength) {
                    BackupChar = Opts->ParentName.StartOfString[VolumeRootLength];
                    Opts->ParentName.StartOfString[VolumeRootLength] = '\0';

                    SdirPopulateSummaryWithGetDiskFreeSpace(Opts->ParentName.StartOfString, Summary);

                    Opts->ParentName.StartOfString[VolumeRootLength] = BackupChar;
                }
            }
        }
    }

    //
    //  We loop enumerating all the files.  Hopefully for common directories
    //  we'll allocate a big enough buffer in the first case and we can then
    //  just populate that buffer and display it.  If the directory is large
    //  enough though, we count the number of entries, loop back, and allocate
    //  a large enough buffer to hold the result, then enumerate it again.
    //  Because we sort the output, we must keep the entire set in memory to
    //  be able to meaningfully process it.
    //

    DirEntsToPreserve = SdirDirCollectionCurrent;

    do {

        //
        //  If this is a subsequent pass and we didn't allocate a big enough
        //  buffer, reallocate our buffers.  Add an extra 100 just in case
        //  we're still adding files in real time.
        //

        if (SdirDirCollectionCurrent >= SdirAllocatedDirents || SdirDirCollection == NULL) {

            if (SdirDirCollectionCurrent > SdirAllocatedDirents) {
                SdirAllocatedDirents = SdirDirCollectionCurrent;
            }
            if (SdirAllocatedDirents < UINT_MAX - 100) {
                SdirAllocatedDirents += 100;
            }

            NewSdirDirCollection = YoriLibMalloc(SdirAllocatedDirents * sizeof(YORI_FILE_INFO));
            if (NewSdirDirCollection == NULL) {
                SdirAllocatedDirents = SdirDirCollectionCurrent;
                SdirDisplayError(GetLastError(), _T("YoriLibMalloc"));
                return FALSE;
            }
    
            NewSdirDirSorted = YoriLibMalloc(SdirAllocatedDirents * sizeof(PYORI_FILE_INFO));
            if (NewSdirDirSorted == NULL) {
                SdirAllocatedDirents = SdirDirCollectionCurrent;
                YoriLibFree(NewSdirDirCollection);
                SdirDisplayError(GetLastError(), _T("YoriLibMalloc"));
                return FALSE;
            }

            //
            // Copy back any previous data.  This occurs when multiple criteria are specified, eg.,
            // "*.a *.b".  Apply fixups to everything in the sorted array which were based on the
            // previous collection and now need to be based on the new one.
            //
            // MSFIX I don't think this is right.  Fixing up offsets is fine, but if we already
            // inserted things from the second criteria before failing out, the sorted array
            // is being updated to point to items that we haven't populated into the new array.
            // This is rare because we need to have multiple criteria, succeed with one, try
            // for the second, then fail out and reallocate.
            //
    
            if (DirEntsToPreserve > 0) {
                memcpy(NewSdirDirCollection, SdirDirCollection, sizeof(YORI_FILE_INFO)*DirEntsToPreserve);
                for (SizeCopied = 0; SizeCopied < DirEntsToPreserve; SizeCopied++) {
                    NewSdirDirSorted[SizeCopied] = (PYORI_FILE_INFO)((PUCHAR)SdirDirSorted[SizeCopied] -
                                                                  (PUCHAR)SdirDirCollection +
                                                                  (PUCHAR)NewSdirDirCollection);
                }
            }
    
            if (SdirDirCollection != NULL) {
                YoriLibFree(SdirDirCollection);
            }
    
            if (SdirDirSorted != NULL) {
                YoriLibFree(SdirDirSorted);
            }
    
            SdirDirCollection = NewSdirDirCollection;
            SdirDirSorted = NewSdirDirSorted;
            SdirDirCollectionCurrent = DirEntsToPreserve;
        }

        //
        //  If we can't find enumerate, display the error except when we're recursive
        //  and the error is we found no files in this particular directory.
        //
        //  MSFIX YoriLibForEachFile isn't trying to set last error.  This means
        //  error reports are garbage and we don't know why items weren't found
        //  (access denied vs. no matches.)
        //

        ItemFoundContext.ItemsFound = 0;
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES | YORILIB_FILEENUM_INCLUDE_DOTFILES;

        //
        //  MSFIX This isn't really correct without a major refactor.  What
        //  we want is to allow full expansion of the search criteria but
        //  basic expansion of the search path, since it was the result of
        //  a prior enumerate.
        //

        if (Depth > 0) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }
        if (!YoriLibForEachFile(FindStr,
                                MatchFlags,
                                0,
                                SdirItemFoundCallback,
                                &ItemFoundContext)) {

            if (!Opts->Recursive) {
                DWORD Err = GetLastError();
                SdirDisplayYsError(Err, FindStr);
                SetLastError(Err);
            }
            return FALSE;
        }

        if (ItemFoundContext.ItemsFound == 0) {
            if (!Opts->Recursive) {
                SdirDisplayYsError(GetLastError(), FindStr);
            }
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }

        //
        //  If we've encountered more than we've allocated, go back
        //  and reallocate.
        //

    } while (SdirDirCollectionCurrent >= SdirAllocatedDirents);

    return TRUE;
}

/**
 Enumerate all of the files in a given single directory/wildcard pattern,
 and populate the results into the global SdirAllocatedDirents array.
 This is a trivial wrapper around SdirEnumeratePathWithDepth to maintain
 a function signature.

 @param FindStr The compound directory/wildcard pattern to enumerate.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirEnumeratePath (
    __in PYORI_STRING FindStr
    )
{
    return SdirEnumeratePathWithDepth(FindStr, 0);
}


/**
 An array element in the character array corresponding to a horizontal line.
 */
#define SDIR_LINE_ELEMENT_HORIZ      0

/**
 An array element in the character array corresponding to a horizontal and
 vertical line intersection where the vertical line is beneath the horizontal
 line.
 */
#define SDIR_LINE_ELEMENT_TOP_T      1

/**
 An array element in the character array corresponding to a horizontal and
 vertical line intersection where the vertical line is above the horizontal
 line.
 */
#define SDIR_LINE_ELEMENT_BOTTOM_T   2

/**
 An array element in the character array corresponding to a vertical line.
 */
#define SDIR_LINE_ELEMENT_VERT       3

#ifdef UNICODE
/**
 A character array for grid characters using Unicode characters.
 */
TCHAR SdirLineElementsRich[] = {0x2500, 0x252c, 0x2534, 0x2502};
#endif
/**
 A character array for grid characters using only 7 bit characters.
 */
TCHAR SdirLineElementsText[] = {'-', '+', '+', '|'};


/**
 Write a newline by generating the active line ending characters and sending
 them to the display module.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirNewlineThroughDisplay()
{
    SDIR_FMTCHAR Line;

    Line.Char = '\n';
    YoriLibSetColorToWin32(&Line.Attr, 0);

    SdirWrite(&Line, 1);
    return TRUE;
}

/**
 Resolves a specific feature color either from a configured value for that
 feature or from the file's color if the feature color was supposed to be
 derived from that.
 */
#define SdirFeatureColor(Feat, FileColor) \
    (((Feat)->Flags & SDIR_FEATURE_USE_FILE_COLOR)?FileColor:(Feat)->HighlightColor)


/**
 Display the loaded set of files.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirDisplayCollection()
{
    PYORI_FILE_INFO CurrentEntry;
    DWORD Index, Ext;
    YORILIB_COLOR_ATTRIBUTES Attributes;
    YORILIB_COLOR_ATTRIBUTES FeatureColor;
    DWORD Columns;
    DWORD ColumnWidth;
    DWORD ActiveColumn = 0;
    SDIR_FMTCHAR Line[SDIR_MAX_WIDTH];
    DWORD CurrentChar = 0;
    DWORD BufferRows;
    DWORD LongestDisplayedFileName = SdirDirCollectionLongest;
    LPTSTR LineElements = SdirLineElementsText;
    PSDIR_FEATURE Feature;

#ifdef UNICODE
    if (Opts->OutputExtendedCharacters) {
        LineElements = SdirLineElementsRich;
    }
#endif

    //
    //  If we're allowed to shorten names to make the display more
    //  legible, we won't allow a longest name greater than twice
    //  average size.  Since we need to insert three characters to
    //  indicate we butchered a name, don't do this unless we have
    //  a meaningful length to start with (currently 10.)
    //

    if (Opts->EnableNameTruncation) {
        ULONG AverageNameLength;

        AverageNameLength = SdirDirCollectionTotalNameLength / SdirDirCollectionCurrent;
        if (LongestDisplayedFileName > 2 * AverageNameLength) {
            LongestDisplayedFileName = 2 * AverageNameLength;
            if (LongestDisplayedFileName < 10) {
                LongestDisplayedFileName = 10;
            }
        }
    }

    ColumnWidth = Opts->ConsoleWidth;
    if (ColumnWidth > SDIR_MAX_WIDTH) {
        ColumnWidth = SDIR_MAX_WIDTH;
    }

    if (Opts->FtFileName.Flags & SDIR_FEATURE_DISPLAY) {
        Columns = ColumnWidth / (LongestDisplayedFileName + Opts->MetadataWidth);
    } else {
        Columns = ColumnWidth / (Opts->MetadataWidth);
    }

    //
    //  If the output is too big to fit the display, force one column
    //  of sufficient size.  The output here will be scrambled, but it
    //  will indicate to the user what happened.
    //

    if (Columns > 0) {
        ColumnWidth = ColumnWidth / Columns;
        LongestDisplayedFileName = ColumnWidth - Opts->MetadataWidth;
    } else {
        Columns = 1;
        ColumnWidth = Opts->MetadataWidth;
        if (Opts->FtFileName.Flags & SDIR_FEATURE_DISPLAY) {
            ColumnWidth += LongestDisplayedFileName;
        }
    }

    //
    //  This really shouldn't happen, even in the worst case of a MAX_PATH name
    //  with all metadata options enabled, but we'll be paranoid.
    //

    if (Columns * ColumnWidth > SDIR_MAX_WIDTH) {
        SdirWriteString(_T("Too much data for a single line!"));
        return FALSE;
    }

    BufferRows = (SdirDirCollectionCurrent + Columns - 1) / Columns;

    //
    //  Draw the top grid line.
    //

    for (Index = 0; Index < ColumnWidth * Columns; Index++) {
        if (Index % ColumnWidth == ColumnWidth - 1 && Index < (ColumnWidth*Columns-1)) {
            Line[Index].Char = LineElements[SDIR_LINE_ELEMENT_TOP_T];
            Line[Index].Attr = Opts->FtGrid.HighlightColor;
        } else {
            Line[Index].Char = LineElements[SDIR_LINE_ELEMENT_HORIZ];
            Line[Index].Attr = Opts->FtGrid.HighlightColor;
        }
    }

    SdirWrite(Line, Index);

    if (ColumnWidth * Columns != Opts->ConsoleBufferWidth || !Opts->OutputHasAutoLineWrap) {
        SdirNewlineThroughDisplay();
    }

    if (!SdirRowDisplayed()) {
        return FALSE;
    }

    //
    //  Enumerate through the entries.
    //

    for (Index = 0; Index < BufferRows * Columns && !Opts->Cancelled; Index++) {

        //
        //  Because we're sorting down columns first, but rendering a row at a time,
        //  we need to do some matrix math to find which elements belong in which cells.
        //  Some cells in the bottom right might be empty.
        //

        Ext = ActiveColumn * BufferRows + Index / Columns;
        if (Ext < SdirDirCollectionCurrent) {
            CurrentEntry = SdirDirSorted[Ext];
        } else {
            CurrentEntry = NULL;
        }

        //
        //  Render the empty cell.  Or, if we have contents, render that too.
        //

        if (CurrentEntry == NULL) {
            SdirPasteStrAndPad(&Line[CurrentChar], NULL, SdirDefaultColor, 0, ColumnWidth - 1);
            CurrentChar += ColumnWidth - 1;
        } else {

            Attributes = CurrentEntry->RenderAttributes;
    
            //
            //  Paste file name into buffer
            //
    
            if (Opts->FtFileName.Flags & SDIR_FEATURE_DISPLAY) {
                FeatureColor = SdirFeatureColor(&Opts->FtFileName, Attributes);
                if (CurrentEntry->FileNameLengthInChars > LongestDisplayedFileName) {
                    ULONG ExtractedLength = (LongestDisplayedFileName - 3) / 2;

                    SdirPasteStr(&Line[CurrentChar], CurrentEntry->FileName, Attributes, ExtractedLength);
                    CurrentChar += ExtractedLength;

                    SdirPasteStr(&Line[CurrentChar], _T("..."), Attributes, ExtractedLength);
                    CurrentChar += 3;

                    SdirPasteStrAndPad(&Line[CurrentChar],
                                       CurrentEntry->FileName + CurrentEntry->FileNameLengthInChars - ExtractedLength,
                                       FeatureColor,
                                       ExtractedLength,
                                       ColumnWidth - Opts->MetadataWidth - ExtractedLength - 3);

                    CurrentChar += ColumnWidth - Opts->MetadataWidth - ExtractedLength - 3;
                } else {
                    SdirPasteStrAndPad(&Line[CurrentChar],
                                       CurrentEntry->FileName,
                                       FeatureColor,
                                       CurrentEntry->FileNameLengthInChars,
                                       ColumnWidth - Opts->MetadataWidth);
                    CurrentChar += ColumnWidth - Opts->MetadataWidth;
                }
            }

            if (Opts->FtShortName.Flags & SDIR_FEATURE_DISPLAY) {
                FeatureColor = SdirFeatureColor(&Opts->FtShortName, Attributes);
                CurrentChar += SdirDisplayShortName(&Line[CurrentChar], FeatureColor, CurrentEntry);

                if (!(Opts->FtFileName.Flags & SDIR_FEATURE_DISPLAY)) {

                    //
                    //  If file name is hidden, we may need to align things, because
                    //  our columns may contain padding (they're essentially justified.)
                    //

                    SdirPasteStrAndPad(&Line[CurrentChar], NULL, FeatureColor, 0, ColumnWidth - Opts->MetadataWidth);
                    CurrentChar += ColumnWidth - Opts->MetadataWidth;
                }
            }

            //
            //  If file names or short names are being displayed, column
            //  justification has already been performed.  If neither
            //  are displayed, force manual justification here.
            //

            if (!(Opts->FtShortName.Flags & SDIR_FEATURE_DISPLAY || Opts->FtFileName.Flags & SDIR_FEATURE_DISPLAY)) {
                SdirPasteStrAndPad(&Line[CurrentChar],
                                   NULL,
                                   Attributes,
                                   0,
                                   ColumnWidth - Opts->MetadataWidth);
                CurrentChar += ColumnWidth - Opts->MetadataWidth;
            }

            //
            //  Paste any metadata options into the buffer.
            //

            for (Ext = 0; Ext < SdirGetNumSdirExec(); Ext++) {
                Feature = (PSDIR_FEATURE)((PUCHAR)Opts + SdirExec[Ext].FtOffset);
                if (Feature->Flags & SDIR_FEATURE_DISPLAY) {
                    FeatureColor = SdirFeatureColor(Feature, Attributes);
                    CurrentChar += SdirExec[Ext].Function(&Line[CurrentChar], FeatureColor, CurrentEntry);
                }
            }
        }
    
        //
        //  We're starting a new column.  If it's the final one we might want a newline,
        //  otherwise we might want a gridline.  Do this manually so we only write the
        //  line once.
        //

        ActiveColumn++;
        if (ActiveColumn % Columns == 0) {

            Line[CurrentChar].Char = '\n';
            Line[CurrentChar].Attr = SdirDefaultColor;
            CurrentChar++;
            SdirWrite(Line, CurrentChar);

            CurrentChar = 0;
            ActiveColumn = 0;
            if (!SdirRowDisplayed()) {
                return FALSE;
            }
        } else {
            Line[CurrentChar].Char = LineElements[SDIR_LINE_ELEMENT_VERT];
            Line[CurrentChar].Attr = Opts->FtGrid.HighlightColor;
            CurrentChar++;
        }
    }

    //
    //  Render the bottom gridline.
    //

    for (Index = 0; Index < ColumnWidth * Columns; Index++) {
        if (Index % ColumnWidth == ColumnWidth - 1 && Index < (ColumnWidth*Columns-1)) {
            Line[Index].Char = LineElements[SDIR_LINE_ELEMENT_BOTTOM_T];
            Line[Index].Attr = Opts->FtGrid.HighlightColor;
        } else {
            Line[Index].Char = LineElements[SDIR_LINE_ELEMENT_HORIZ];
            Line[Index].Attr = Opts->FtGrid.HighlightColor;
        }
    }

    SdirWrite(Line, Index);
    CurrentChar = 0;

    if (ColumnWidth * Columns != Opts->ConsoleBufferWidth || !Opts->OutputHasAutoLineWrap) {
        SdirNewlineThroughDisplay();
        CurrentChar = 0;
    }

    if (!SdirRowDisplayed()) {
        return FALSE;
    }

    return TRUE;
}

/**
 A prototype for a callback function to invoke for each parameter that
 describes a set of files to enumerate.
 */
typedef BOOL (* SDIR_FOR_EACH_PATHSPEC_FN)(PYORI_STRING);

/**
 For every parameter specified on the command line that refers to a set of
 files, invoke a callback to facilitate enumeration.  If the user hasn't
 specified any set of files explicitly, enumerate all files from the current
 directory.

 @param ArgC The number of arguments passed to the application.

 @param ArgV An array of arguments passed to the application.

 @param Callback Specifies a callback function to invoke for each parameter
        that represents a set of files.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirForEachPathSpec (
    __in DWORD ArgC,
    __in YORI_STRING ArgV[],
    __in SDIR_FOR_EACH_PATHSPEC_FN Callback
    )
{
    ULONG  CurrentArg;
    BOOLEAN EnumerateUserSpecified = FALSE;
    YORI_STRING FindStr;
    YORI_STRING Arg;

    YoriLibInitEmptyString(&FindStr);

    for (CurrentArg = 1; CurrentArg < ArgC; CurrentArg++) {

        if (!YoriLibIsCommandLineOption(&ArgV[CurrentArg], &Arg)) {

            HANDLE hDir;

            EnumerateUserSpecified = TRUE;

            YoriLibFreeStringContents(&FindStr);

            if (!YoriLibUserStringToSingleFilePath(&ArgV[CurrentArg], TRUE, &FindStr)) {
                SdirDisplayError(GetLastError(), _T("YoriLibUserStringToSingleFilePath"));
                return FALSE;
            }

            //
            //  FILE_READ_DATA (aka FILE_LIST_DIRECTORY) is needed for some
            //  popular SMB servers to return accurate information.  But if
            //  we don't have access and the info may be available, try
            //  without it.
            //

            hDir = CreateFile(FindStr.StartOfString,
                              FILE_READ_ATTRIBUTES|FILE_READ_DATA,
                              FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                              NULL);

            if (hDir == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED) {
                hDir = CreateFile(FindStr.StartOfString,
                                  FILE_READ_ATTRIBUTES,
                                  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                                  NULL);
            }
    
            if (hDir != INVALID_HANDLE_VALUE) {
                BY_HANDLE_FILE_INFORMATION HandleInfo;
    
                if (!GetFileInformationByHandle(hDir, &HandleInfo)) {
                    HandleInfo.dwFileAttributes = 0;
                }

                if (HandleInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    LPTSTR FindStrWithWild;
                    LPTSTR szFormatStr;
                    DWORD FindStrWithWildLength = (DWORD)FindStr.LengthInChars + 3;

                    FindStrWithWild = YoriLibReferencedMalloc(FindStrWithWildLength * sizeof(TCHAR));
                    if (FindStrWithWild == NULL) {
                        SdirDisplayError(GetLastError(), _T("YoriLibMalloc"));
                        YoriLibFreeStringContents(&FindStr);
                        return FALSE;
                    }
                    if (FindStr.StartOfString[FindStrWithWildLength - 4] == '\\') {
                        szFormatStr = _T("%y*");
                    } else {
                        szFormatStr = _T("%y\\*");
                    }
                    FindStr.LengthInChars = YoriLibSPrintfS(FindStrWithWild, FindStrWithWildLength, szFormatStr, &FindStr);
                    YoriLibDereference(FindStr.MemoryToFree);
                    FindStr.MemoryToFree = FindStrWithWild;
                    FindStr.StartOfString = FindStrWithWild;
                    FindStr.LengthAllocated = FindStr.LengthInChars + 1;
                }
    
                CloseHandle(hDir);
            }

            if (!Callback(&FindStr)) {
                YoriLibFreeStringContents(&FindStr);
                return FALSE;
            }
        }
    }

    YoriLibFreeStringContents(&FindStr);

    if (!EnumerateUserSpecified) {
        YoriLibConstantString(&FindStr, _T("*"));
        if (!Callback(&FindStr)) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 Enumerate and display the contents of a single directory.

 @param ArgC The number of arguments passed to the application.

 @param ArgV An array of arguments passed to the application.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirEnumerateAndDisplay (
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    if (!SdirForEachPathSpec(ArgC, ArgV, SdirEnumeratePath)) {
        return FALSE;
    }

    if (SdirDirCollectionCurrent == 0) {
        SdirDisplayError(ERROR_FILE_NOT_FOUND, NULL);
        return FALSE;
    }

    if (!SdirDisplayCollection()) {
        return FALSE;
    }
    
    return TRUE;
}

/**
 Count of the number of lines displayed in brief recurse mode to allow
 for alternating colors in order to make lines more readable.
 */
ULONG HeirarchyLineNumber = 0;

/**
 Display a single line of output during brief recurse (du style) enumerates.

 @param NodeName Pointer to a NULL terminated string indicating the directory
        that was just enumerated.

 @param Before Pointer to summary information indicating the number of files
        found and their sizes before this directory enumerate.

 @param After Pointer to summary information indicating the number of files
        found and their sizes after this directory enumerate.

 @param DefaultAttributes Specifies the default color information to apply for
        text.  This is overridden by much of the display elements.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirDisplayHeirarchySummary(
    __in LPTSTR NodeName,
    __in PSDIR_SUMMARY Before,
    __in PSDIR_SUMMARY After,
    __in YORILIB_COLOR_ATTRIBUTES DefaultAttributes
    )
{
    PSDIR_FMTCHAR Buffer;
    TCHAR Str[100];
    DWORD Len;
    DWORD PrefixLen;
    LARGE_INTEGER Size;
    ULONG CurrentChar = 0;
    ULONG MetadataSize = 40;
    ULONG FileCountAlignSize = 4;
    ULONG DirectoryNameAlignSize;
    YORILIB_COLOR_ATTRIBUTES Background;
    YORILIB_COLOR_ATTRIBUTES RenderAttributes;
    YORILIB_COLOR_ATTRIBUTES ThisColor;
    YORI_STRING YsNodeName;

    //
    //  If it doesn't meet size criteria, display nothing
    //

    if (Opts->BriefRecurseSize > 0) {
        if ((After->TotalSize - Before->TotalSize) < Opts->BriefRecurseSize) {
            return TRUE;
        }
    }

    YoriLibConstantString(&YsNodeName, NodeName);

    Len = YsNodeName.LengthInChars + SDIR_MAX_WIDTH;
    Buffer = YoriLibMalloc(Len * sizeof(SDIR_FMTCHAR));
    if (Buffer == NULL) {
        SdirDisplayError(GetLastError(), _T("YoriLibMalloc"));
        return FALSE;
    }

    HeirarchyLineNumber++;

    if ((HeirarchyLineNumber % 2) == 0) {
        Background = Opts->FtBriefAlternate.HighlightColor;
    } else {
        YoriLibSetColorToWin32(&Background, 0);
    }

    //
    //  Guess a good amount of padding based on the window size
    //

    if (Opts->FtCompressedFileSize.Flags & SDIR_FEATURE_DISPLAY) {
        MetadataSize += 20;
    }

    if (Opts->ConsoleWidth < 10 + MetadataSize) {
        DirectoryNameAlignSize = 10;
    } else {
        DirectoryNameAlignSize = Opts->ConsoleWidth - MetadataSize;
    }

    if (DirectoryNameAlignSize > (SDIR_MAX_WIDTH - 100)) {
        DirectoryNameAlignSize = SDIR_MAX_WIDTH - 100;
    }

    if (Opts->ConsoleWidth > 70) {
        FileCountAlignSize++;
        DirectoryNameAlignSize -= 2;
    }

    if (Opts->ConsoleWidth > 100) {
        FileCountAlignSize++;
        DirectoryNameAlignSize -= 2;
    }

    if (Opts->ConsoleWidth > 130) {
        FileCountAlignSize++;
        DirectoryNameAlignSize -= 2;
    }

    //
    //  Display directory name
    //

    RenderAttributes = SdirRenderAttributesFromPath(&YsNodeName);
    RenderAttributes = YoriLibCombineColors(RenderAttributes, Background);
    Len = YsNodeName.LengthInChars;
    PrefixLen = 0;

    if (YoriLibIsFullPathUnc(&YsNodeName)) {

        Len -= 8;
        NodeName += 8;

        SdirPasteStr(&Buffer[CurrentChar], _T("\\\\"), RenderAttributes, 2);
        CurrentChar += 2;
        PrefixLen = 2;

    } else {
        Len -= 4;
        NodeName += 4;
    }
    
    SdirPasteStr(&Buffer[CurrentChar], NodeName, RenderAttributes, Len);
    CurrentChar += Len;
    Len = DirectoryNameAlignSize - ((Len + PrefixLen) % DirectoryNameAlignSize);
    SdirPasteStrAndPad(&Buffer[CurrentChar], NULL, RenderAttributes, 0, Len);
    CurrentChar += Len;

    //
    //  Display size of the directory contents
    //

    Size.HighPart = 0;
    SdirFileSizeFromLargeInt(&Size) = After->TotalSize - Before->TotalSize;
    ThisColor = YoriLibCombineColors(Opts->FtFileSize.HighlightColor, Background);
    CurrentChar += SdirDisplayGenericSize(&Buffer[CurrentChar], ThisColor, &Size);
    ThisColor = YoriLibCombineColors(DefaultAttributes, Background);
    SdirPasteStr(&Buffer[CurrentChar], _T(" used "), ThisColor, 6);
    CurrentChar += 6;

    //
    //  Display compressed size if requested
    //

    if (Opts->FtCompressedFileSize.Flags & SDIR_FEATURE_DISPLAY) {
        ThisColor = YoriLibCombineColors(DefaultAttributes, Background);
        SdirPasteStr(&Buffer[CurrentChar], _T("("), ThisColor, 1);
        CurrentChar += 1;

        Size.HighPart = 0;
        SdirFileSizeFromLargeInt(&Size) = After->CompressedSize - Before->CompressedSize;

        ThisColor = YoriLibCombineColors(Opts->FtCompressedFileSize.HighlightColor, Background);
        CurrentChar += SdirDisplayGenericSize(&Buffer[CurrentChar], ThisColor, &Size);
        ThisColor = YoriLibCombineColors(DefaultAttributes, Background);
        SdirPasteStr(&Buffer[CurrentChar], _T(" compressed) "), ThisColor, 13);
        CurrentChar += 13;
    }

    //
    //  Display count of files
    //

    YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T("%i"), (int)(After->NumFiles - Before->NumFiles));
    Len = (DWORD)_tcslen(Str);

    ThisColor = YoriLibCombineColors(DefaultAttributes, Background);
    SdirPasteStrAndPad(&Buffer[CurrentChar], _T("in "), ThisColor, 3, 3+FileCountAlignSize);
    CurrentChar += (Len>FileCountAlignSize)?3:(3+FileCountAlignSize-Len);
    ThisColor = YoriLibCombineColors(Opts->FtNumberFiles.HighlightColor, Background);
    SdirPasteStr(&Buffer[CurrentChar], Str, ThisColor, Len);
    CurrentChar += Len;

    //
    //  And count of dirs
    //

    YoriLibSPrintfS(Str, sizeof(Str)/sizeof(Str[0]), _T("%i"), (int)(After->NumDirs - Before->NumDirs));
    Len = (DWORD)_tcslen(Str);

    ThisColor = YoriLibCombineColors(DefaultAttributes, Background);
    SdirPasteStrAndPad(&Buffer[CurrentChar], _T(" files and "), ThisColor, 11, 11+FileCountAlignSize);
    CurrentChar += (Len>FileCountAlignSize)?11:(11+FileCountAlignSize-Len);

    ThisColor = YoriLibCombineColors(Opts->FtNumberFiles.HighlightColor, Background);
    SdirPasteStr(&Buffer[CurrentChar], Str, ThisColor, Len);
    CurrentChar += Len;

    ThisColor = YoriLibCombineColors(DefaultAttributes, Background);
    SdirPasteStr(&Buffer[CurrentChar], _T(" dirs"), ThisColor, 5);
    CurrentChar += 5;

    //
    //  Display the formatted line
    //

    for (Len = 0; Len < CurrentChar / Opts->ConsoleWidth; Len++) {
        if (!SdirRowDisplayed()) {
            YoriLibFree(Buffer);
            return FALSE;
        }
    }
    SdirWrite(Buffer, CurrentChar);

    //
    //  Newline is written through this function for automatic pause
    //  accounting
    //

    ThisColor = YoriLibCombineColors(DefaultAttributes, Background);
    if (!SdirWriteStringWithAttribute(_T("\n"), ThisColor)) {
        YoriLibFree(Buffer);
        return FALSE;
    }

    YoriLibFree(Buffer);
    return TRUE;
}

/**
 Error codes that should allow enumeration to continue rather than abort.
 */
#define SdirContinuableError(err) \
    (err == ERROR_FILE_NOT_FOUND || err == ERROR_ACCESS_DENIED)

/**
 Error codes that should be reported to the user when performing recursive
 enumerates.
 */
#define SdirIsReportableError(err) \
    (err != ERROR_FILE_NOT_FOUND)

/**
 Perform a recursive enumerate.  This may be a brief enumerate (du-style) or
 may be a regular display of files in each directory.

 @param Depth Indicates the current recursion depth.  The user specified
        directory is zero.

 @param FileSpec A string which contains a directory and search criteria.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirEnumerateAndDisplaySubtree (
    __in DWORD Depth,
    __in PYORI_STRING FileSpec
    )
{
    YORI_STRING ParentDirectory;
    YORI_STRING SearchCriteria;
    LPTSTR FinalBackslash;
    YORI_STRING NextSubDir;
    HANDLE hFind;
    WIN32_FIND_DATA FindData;
    SDIR_SUMMARY SummaryOnEntry;
    LPTSTR szFormatStr;

    YoriLibInitEmptyString(&ParentDirectory);
    YoriLibInitEmptyString(&SearchCriteria);

    FinalBackslash = YoriLibFindRightMostCharacter(FileSpec, '\\');
    if (FinalBackslash != NULL) {
        ParentDirectory.StartOfString = FileSpec->StartOfString;
        ParentDirectory.LengthInChars = (DWORD)(FinalBackslash - FileSpec->StartOfString);
        ParentDirectory.LengthAllocated = ParentDirectory.LengthInChars + 1;

        FinalBackslash[0] = '\0';

        SearchCriteria.StartOfString = FinalBackslash + 1;
        SearchCriteria.LengthInChars = FileSpec->LengthInChars - ParentDirectory.LengthInChars - 1;
        SearchCriteria.LengthAllocated = FileSpec->LengthAllocated - ParentDirectory.LengthAllocated;

    } else {
        SearchCriteria.StartOfString = FileSpec->StartOfString;
        SearchCriteria.LengthInChars = FileSpec->LengthInChars;
        SearchCriteria.LengthAllocated = FileSpec->LengthAllocated;
    }

    ASSERT(ParentDirectory.LengthInChars == 0 || YoriLibIsStringNullTerminated(&ParentDirectory));
    ASSERT(YoriLibIsStringNullTerminated(&SearchCriteria));

    memcpy(&SummaryOnEntry, Summary, sizeof(SDIR_SUMMARY));

    if (!YoriLibAllocateString(&NextSubDir, ParentDirectory.LengthInChars + YORI_LIB_MAX_FILE_NAME + 2 + SearchCriteria.LengthInChars + 1)) {
        SdirDisplayError(GetLastError(), _T("YoriLibAllocateString"));
        return FALSE;
    }

    if (ParentDirectory.LengthInChars == 0 ||
        ParentDirectory.StartOfString[ParentDirectory.LengthInChars - 1] == '\\') {
        szFormatStr = _T("%y%y");
    } else {
        szFormatStr = _T("%y\\%y");
    }

    if (YoriLibYPrintf(&NextSubDir, szFormatStr, &ParentDirectory, &SearchCriteria) < 0 ||
        NextSubDir.LengthInChars == 0) {

        SdirWriteString(_T("Path exceeds allocated length\n"));
        YoriLibFreeStringContents(&NextSubDir);
        return FALSE;
    }

    if (!SdirEnumeratePathWithDepth(&NextSubDir, Depth)) {
        DWORD Err = GetLastError();
        if (SdirIsReportableError(Err)) {

            Opts->ErrorsFound = TRUE;
        }
        if (!SdirContinuableError(Err)) {

            YoriLibFreeStringContents(&NextSubDir);
            return FALSE;
        }
    }

    //
    //  If we're giving a regular view and have something to display,
    //  display it.
    //

    if (Opts->BriefRecurseDepth == 0 && SdirDirCollectionCurrent > 0) {

        YORILIB_COLOR_ATTRIBUTES RenderAttributes;
        RenderAttributes = SdirRenderAttributesFromPath(&ParentDirectory);

        if (YoriLibIsFullPathUnc(&ParentDirectory)) {
            SdirWriteStringWithAttribute(_T("\\\\"), RenderAttributes);
            SdirWriteStringWithAttribute(&ParentDirectory.StartOfString[8], RenderAttributes);
        } else {
            SdirWriteStringWithAttribute(&ParentDirectory.StartOfString[4], RenderAttributes);
        }
    
        SdirNewlineThroughDisplay();

        if (!SdirRowDisplayed()) {
            YoriLibFreeStringContents(&NextSubDir);
            return FALSE;
        }

        if (!SdirDisplayCollection()) {
            YoriLibFreeStringContents(&NextSubDir);
            return FALSE;
        }
    }

    //
    //  Now traverse down the tree through all directories,
    //  optionally following links.
    //

    SdirDirCollectionCurrent = 0;
    SdirDirCollectionLongest = 0;
    SdirDirCollectionTotalNameLength = 0;

    if (ParentDirectory.LengthInChars == 0 ||
        ParentDirectory.StartOfString[ParentDirectory.LengthInChars - 1] == '\\') {

        szFormatStr = _T("%y*");
    } else {
        szFormatStr = _T("%y\\*");
    }

    if (YoriLibYPrintf(&NextSubDir, szFormatStr, &ParentDirectory) < 0 ||
        NextSubDir.LengthInChars == 0) {

        SdirWriteString(_T("Path exceeds allocated length\n"));
        YoriLibFreeStringContents(&NextSubDir);
        return FALSE;
    }

    hFind = FindFirstFile(NextSubDir.StartOfString, &FindData);
    
    if (hFind == NULL || hFind == INVALID_HANDLE_VALUE) {
        DWORD Err = GetLastError();
        Opts->ErrorsFound = TRUE;
        if (SdirIsReportableError(Err)) {
            if (!SdirDisplayYsError(Err, &NextSubDir)) {
                YoriLibFreeStringContents(&NextSubDir);
                return FALSE;
            }
        }
        if (!SdirContinuableError(Err)) {
            YoriLibFreeStringContents(&NextSubDir);
            return FALSE;
        } else {
            YoriLibFreeStringContents(&NextSubDir);
            return TRUE;
        }
    }
    
    do {
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
            _tcscmp(FindData.cFileName, _T(".")) != 0 &&
            _tcscmp(FindData.cFileName, _T("..")) != 0) {

            if (Opts->TraverseLinks ||
                (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0 ||
                (FindData.dwReserved0 != IO_REPARSE_TAG_MOUNT_POINT &&
                 FindData.dwReserved0 != IO_REPARSE_TAG_SYMLINK)) {

                if (ParentDirectory.LengthInChars == 0 ||
                    ParentDirectory.StartOfString[ParentDirectory.LengthInChars - 1] == '\\') {
                    szFormatStr = _T("%y%s\\%y");
                } else {
                    szFormatStr = _T("%y\\%s\\%y");
                }

                if (YoriLibYPrintf(&NextSubDir, szFormatStr, &ParentDirectory, FindData.cFileName, &SearchCriteria) < 0 ||
                    NextSubDir.LengthInChars == 0) {

                    SdirWriteString(_T("Path exceeds allocated length\n"));
                    YoriLibFreeStringContents(&NextSubDir);
                    return FALSE;
                }
    
                if (!SdirEnumerateAndDisplaySubtree(Depth + 1, &NextSubDir)) {
                    FindClose(hFind);
                    YoriLibFreeStringContents(&NextSubDir);
                    return FALSE;
                }
            }
        }
    } while (FindNextFile(hFind, &FindData) && !Opts->Cancelled);

    YoriLibFreeStringContents(&NextSubDir);
    
    FindClose(hFind);

    //
    //  If we're displaying a heirarchy, display the results at
    //  the end, after all children have been accounted for.
    //

    if (Opts->BriefRecurseDepth != 0 && Depth <= Opts->BriefRecurseDepth) {
        if (!SdirDisplayHeirarchySummary(ParentDirectory.StartOfString, &SummaryOnEntry, Summary, Opts->FtSummary.HighlightColor)) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 Walk through each of the arguments to the program and enumerate each
 recursively.

 @param ArgC Number of arguments passed to the program.

 @param ArgV An array of each argument passed to the program.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirEnumerateAndDisplayRecursive (
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN EnumerateUserSpecified = FALSE;
    ULONG CurrentArg;
    YORI_STRING Arg;
    YORI_STRING FindStr;

    for (CurrentArg = 1; CurrentArg < ArgC; CurrentArg++) {
        if (!YoriLibIsCommandLineOption(&ArgV[CurrentArg], &Arg)) {

            YoriLibInitEmptyString(&FindStr);
            if (!YoriLibUserStringToSingleFilePath(&ArgV[CurrentArg], TRUE, &FindStr)) {
                return FALSE;
            }

            if (!SdirEnumerateAndDisplaySubtree(0, &FindStr)) {
                YoriLibFreeStringContents(&FindStr);
                return FALSE;
            }
            YoriLibFreeStringContents(&FindStr);
            EnumerateUserSpecified = TRUE;
        }
    }

    if (!EnumerateUserSpecified) {
        YoriLibConstantString(&Arg, _T("*"));
        YoriLibInitEmptyString(&FindStr);
        if (!YoriLibUserStringToSingleFilePath(&Arg, TRUE, &FindStr)) {
            return FALSE;
        }
        if (!SdirEnumerateAndDisplaySubtree(0, &FindStr)) {
            YoriLibFreeStringContents(&FindStr);
            return FALSE;
        }
        YoriLibFreeStringContents(&FindStr);
    }

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the sdir builtin command.
 */
#define ENTRYPOINT YoriCmd_SDIR
#else
/**
 The main entrypoint for the sdir standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the sdir command.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    SdirAllocatedDirents = 1000;
    SdirDirCollection = NULL;
    SdirDirSorted = NULL;
    SdirDirCollectionCurrent = 0;
    SdirDirCollectionLongest = 0;
    SdirDirCollectionTotalNameLength = 0;
    SdirWriteStringLinesDisplayed = 0;

    if (!SdirInit(ArgC, ArgV)) {
        goto restore_and_exit;
    }

    if (Opts->Recursive) {
        if (!SdirEnumerateAndDisplayRecursive(ArgC, ArgV)) {
            goto restore_and_exit;
        }

        if (Opts->ErrorsFound) {
            SdirWriteStringWithAttribute(_T("Errors found during enumerate; results are incomplete\n"), Opts->FtError.HighlightColor);
        }
    } else {
        if (!SdirEnumerateAndDisplay(ArgC, ArgV)) {
            goto restore_and_exit;
        }
    }

    if (Opts->FtSummary.Flags & SDIR_FEATURE_DISPLAY) {

        SdirDisplaySummary(Opts->FtSummary.HighlightColor);
    }

restore_and_exit:

    if (Opts != NULL) {
        SdirSetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Opts->PreviousAttributes);
    }
    SdirAppCleanup();

    return 0;
}

// vim:sw=4:ts=4:et:
