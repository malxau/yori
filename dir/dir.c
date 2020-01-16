/**
 * @file dir/dir.c
 *
 * Yori shell enumerate files in directories
 *
 * Copyright (c) 2017-2019 Malcolm J. Smith
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
CHAR strDirHelpText[] =
        "\n"
        "Enumerate the contents of directories.\n"
        "\n"
        "DIR [-license] [-b] [-color] [-g] [-h] [-m] [-r] [-s] [-x] [<spec>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -color         Use file color highlighting\n"
        "   -g             Use global time and date format, ignoring locale\n"
        "   -h             Hide hidden and system files\n"
        "   -m             Minimal display, file names only\n"
        "   -r             Display named streams\n"
        "   -s             Process files from all subdirectories\n"
        "   -x             Display short file names\n"
        "\n"
        "For a more powerful enumerator, consider using sdir instead.\n";

/**
 Display usage text to the user.
 */
BOOL
DirHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Dir %i.%02i\n"), DIR_VER_MAJOR, DIR_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strDirHelpText);
    return TRUE;
}

/**
 The maximum size of the stream name in WIN32_FIND_STREAM_DATA.
 */
#define DIR_MAX_STREAM_NAME          (MAX_PATH + 36)

/**
 The date order code for Month-Day-Year.  Zero because Windows came from the
 US.
 */
#define DIR_DATE_ORDER_MDY (0)

/**
 The date order code for Day-Month-Year.
 */
#define DIR_DATE_ORDER_DMY (1)

/**
 The date order code for Year-Month-Day.
 */
#define DIR_DATE_ORDER_YMD (2)

/**
 A private definition of WIN32_FIND_STREAM_DATA in case the compilation
 environment doesn't provide it.
 */
typedef struct _DIR_WIN32_FIND_STREAM_DATA {

    /**
     The length of the stream, in bytes.
     */
    LARGE_INTEGER StreamSize;

    /**
     The name of the stream.
     */
    WCHAR cStreamName[DIR_MAX_STREAM_NAME];
} DIR_WIN32_FIND_STREAM_DATA, *PDIR_WIN32_FIND_STREAM_DATA;

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _DIR_CONTEXT {

    /**
     TRUE if the directory enumeration should display short file names as well
     as long file names.  FALSE if only long names should be displayed.
     */
    BOOLEAN DisplayShortNames;

    /**
     TRUE if the directory enumeration should display named streams on files.
     */
    BOOLEAN DisplayStreams;

    /**
     TRUE if the display should be minimal and only include file names.
     */
    BOOLEAN MinimalDisplay;

    /**
     TRUE if the directory enumerate is recursively scanning through
     directories, FALSE if it is one directory only.
     */
    BOOLEAN Recursive;

    /**
     TRUE if hidden and system files should be hidden.  FALSE if they should
     be displayed.
     */
    BOOLEAN HideHidden;

    /**
     TRUE if locale should not be used to configure date and time display.
     FALSE if locale information should be used when available.
     */
    BOOLEAN IgnoreLocale;

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     Records the total number of directories processed.
     */
    LONGLONG DirsFound;

    /**
     The total amount of bytes consumed by all files processed.
     */
    LONGLONG TotalFileSize;

    /**
     A string containing the directory currently being enumerated.
     */
    YORI_STRING CurrentDirectoryName;

    /**
     The total number of directory entries enumerated from this directory.
     */
    LONGLONG ObjectsFoundInThisDir;

    /**
     The total number of directory entries enumerated from this directory
     that were themselves directories.
     */
    LONGLONG DirsFoundInThisDir;

    /**
     The total number of directory entries enumerated from this directory
     that were themselves files.
     */
    LONGLONG FilesFoundInThisDir;

    /**
     The total amount of bytes consumed by files within this directory.
     */
    LONGLONG FileSizeInThisDir;

    /**
     Color information to display against matching files.
     */
    YORI_LIB_FILE_FILTER ColorRules;

    /**
     A buffer allocated to fetch reparse data.  This is here because we
     probably won't allocate it, but if we do, it makes sense to reuse it
     until enumeration is complete.
     */
    PYORI_REPARSE_DATA_BUFFER ReparseDataBuffer;

    /**
     The number of bytes in ReparseDataBuffer allocation.
     */
    DWORD ReparseDataBufferLength;

    /**
     The character to use to seperate date components.
     */
    TCHAR DateSeperator;

    /**
     The order of date components.
     */
    TCHAR DateOrder;

    /**
     Nonzero if the year should have four characters.  Zero if it should
     have two characters.
     */
    TCHAR FourCharYear;

    /**
     The character to use to seperate time components.
     */
    TCHAR TimeSeperator;

    /**
     Nonzero if the time should be in 24 hour format.  Zero if it should
     have an AM/PM suffix.
     */
    TCHAR Time24Hour;

} DIR_CONTEXT, *PDIR_CONTEXT;

/**
 This function right aligns a Yori string by moving characters in place
 to ensure the total length of the string equals the specified alignment.

 @param String Pointer to the string to align.

 @param Align The number of characters that the string should contain.  If
        it currently has less than this number, spaces are inserted at the
        beginning of the string.
 */
VOID
DirRightAlignString(
    __in PYORI_STRING String,
    __in DWORD Align
    )
{
    DWORD Index;
    DWORD Delta;
    if (String->LengthInChars >= Align) {
        return;
    }
    ASSERT(String->LengthAllocated >= Align);
    if (String->LengthAllocated < Align) {
        return;
    }
    Delta = Align - String->LengthInChars;
    for (Index = Align - 1; Index >= Delta; Index--) {
        String->StartOfString[Index] = String->StartOfString[Index - Delta];
    }
    for (Index = 0; Index < Delta; Index++) {
        String->StartOfString[Index] = ' ';
    }
    String->LengthInChars = Align;
}

/**
 The number of characters to use to display the date of objects in the
 directory.
 */
#define DIR_DATE_FIELD_SIZE  11

/**
 The number of characters to use to display the time of objects in the
 directory.
 */
#define DIR_TIME_FIELD_SIZE  9

/**
 The number of characters to use to display the size of objects in the
 directory.
 */
#define DIR_SIZE_FIELD_SIZE  18

/**
 The number of characters to use to display the count of objects in the
 directory.
 */
#define DIR_COUNT_FIELD_SIZE 12

/**
 Before displaying the contents of a directory, this function displays any
 directory level header information.

 @param DirContext Context describing the state of the enumeration.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
DirOutputBeginningOfDirectorySummary(
    __in PDIR_CONTEXT DirContext
    )
{
    YORI_STRING UnescapedPath;
    PYORI_STRING PathToDisplay;
    YORI_STRING VtAttribute;
    TCHAR VtAttributeBuffer[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];
    YORILIB_COLOR_ATTRIBUTES Attribute;

    YoriLibInitEmptyString(&VtAttribute);

    YoriLibInitEmptyString(&UnescapedPath);
    YoriLibUnescapePath(&DirContext->CurrentDirectoryName, &UnescapedPath);
    if (UnescapedPath.LengthInChars > 0) {
        PathToDisplay = &UnescapedPath;
    } else {
        PathToDisplay = &DirContext->CurrentDirectoryName;
    }

    if (DirContext->ColorRules.NumberCriteria) {
        WIN32_FIND_DATA FileInfo;

        VtAttribute.StartOfString = VtAttributeBuffer;
        VtAttribute.LengthAllocated = sizeof(VtAttributeBuffer)/sizeof(VtAttributeBuffer[0]);

        YoriLibUpdateFindDataFromFileInformation(&FileInfo, DirContext->CurrentDirectoryName.StartOfString, TRUE);

        if (!YoriLibFileFiltCheckColorMatch(&DirContext->ColorRules, &DirContext->CurrentDirectoryName, &FileInfo, &Attribute)) {
            Attribute.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG;
            Attribute.Win32Attr = (UCHAR)YoriLibVtGetDefaultColor();
        }

        YoriLibVtStringForTextAttribute(&VtAttribute, Attribute.Ctrl, Attribute.Win32Attr);
    }

    if (VtAttribute.LengthInChars > 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n Directory of %y%y%c[0m\n\n"), &VtAttribute, PathToDisplay, 27);
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n Directory of %y\n\n"), PathToDisplay);
    }
    YoriLibFreeStringContents(&UnescapedPath);
    return TRUE;
}

/**
 After displaying the contents of a directory, this function displays any
 directory level footer information.

 @param DirContext Context describing the state of the enumeration.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
DirOutputEndOfDirectorySummary(
    __in PDIR_CONTEXT DirContext
    )
{
    YORI_STRING CountString;
    YORI_STRING SizeString;
    TCHAR CountStringBuffer[DIR_COUNT_FIELD_SIZE];
    TCHAR SizeStringBuffer[DIR_SIZE_FIELD_SIZE];
    LARGE_INTEGER FreeSpace;

    YoriLibInitEmptyString(&SizeString);
    SizeString.StartOfString = SizeStringBuffer;
    SizeString.LengthAllocated = sizeof(SizeStringBuffer)/sizeof(SizeStringBuffer[0]);

    YoriLibInitEmptyString(&CountString);
    CountString.StartOfString = CountStringBuffer;
    CountString.LengthAllocated = sizeof(CountStringBuffer)/sizeof(CountStringBuffer[0]);

    YoriLibNumberToString(&CountString, DirContext->FilesFoundInThisDir, 10, 3, ',');
    YoriLibNumberToString(&SizeString, DirContext->FileSizeInThisDir, 10, 3, ',');

    DirRightAlignString(&CountString, DIR_COUNT_FIELD_SIZE);
    DirRightAlignString(&SizeString, DIR_SIZE_FIELD_SIZE);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y File(s) %y bytes\n"), &CountString, &SizeString);


    FreeSpace.QuadPart = 0;
    YoriLibGetDiskFreeSpace(DirContext->CurrentDirectoryName.StartOfString, NULL, NULL, &FreeSpace);

    YoriLibNumberToString(&CountString, DirContext->DirsFoundInThisDir, 10, 3, ',');
    YoriLibNumberToString(&SizeString, FreeSpace.QuadPart, 10, 3, ',');

    DirRightAlignString(&CountString, DIR_COUNT_FIELD_SIZE);
    DirRightAlignString(&SizeString, DIR_SIZE_FIELD_SIZE);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y Dir(s)  %y bytes free\n"), &CountString, &SizeString);

    DirContext->ObjectsFoundInThisDir = 0;
    DirContext->FilesFoundInThisDir = 0;
    DirContext->FileSizeInThisDir = 0;
    DirContext->DirsFoundInThisDir = 0;

    YoriLibFreeStringContents(&CountString);
    YoriLibFreeStringContents(&SizeString);
    return TRUE;
}

/**
 After recursively displaying the contents of a directory, this function
 the total numbers found from all child directories.

 @param DirContext Context describing the state of the enumeration.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
DirOutputEndOfRecursiveSummary(
    __in PDIR_CONTEXT DirContext
    )
{
    YORI_STRING CountString;
    YORI_STRING SizeString;
    TCHAR CountStringBuffer[DIR_COUNT_FIELD_SIZE];
    TCHAR SizeStringBuffer[DIR_SIZE_FIELD_SIZE];

    YoriLibInitEmptyString(&SizeString);
    SizeString.StartOfString = SizeStringBuffer;
    SizeString.LengthAllocated = sizeof(SizeStringBuffer)/sizeof(SizeStringBuffer[0]);

    YoriLibInitEmptyString(&CountString);
    CountString.StartOfString = CountStringBuffer;
    CountString.LengthAllocated = sizeof(CountStringBuffer)/sizeof(CountStringBuffer[0]);

    YoriLibNumberToString(&CountString, DirContext->FilesFound, 10, 3, ',');
    YoriLibNumberToString(&SizeString, DirContext->TotalFileSize, 10, 3, ',');

    DirRightAlignString(&CountString, DIR_COUNT_FIELD_SIZE);
    DirRightAlignString(&SizeString, DIR_SIZE_FIELD_SIZE);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n     Total Files Listed:\n"));
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y File(s) %y bytes\n"), &CountString, &SizeString);

    YoriLibNumberToString(&CountString, DirContext->DirsFound, 10, 3, ',');

    DirRightAlignString(&CountString, DIR_COUNT_FIELD_SIZE);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y Dir(s)\n"), &CountString);

    YoriLibFreeStringContents(&CountString);
    YoriLibFreeStringContents(&SizeString);
    return TRUE;
}

/**
 Load the reparse buffer from a file.

 @param FilePath Pointer to the full path to the file.

 @param DirContext Pointer to the context for the application.  This contains
        the buffer to populate with reparse data.

 @return TRUE to indicate the reparse data was successfully obtained, FALSE to
         indicate it was not.
 */
BOOL
DirLoadReparseData(
    __in PYORI_STRING FilePath,
    __inout PDIR_CONTEXT DirContext
    )
{
    HANDLE FileHandle;
    DWORD BytesReturned;

    FileHandle = CreateFile(FilePath->StartOfString,
                            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL,
                            NULL);

    if (FileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    //  Get the reparse data
    //

    if (!DeviceIoControl(FileHandle, FSCTL_GET_REPARSE_POINT, NULL, 0, DirContext->ReparseDataBuffer, DirContext->ReparseDataBufferLength, &BytesReturned, NULL)) {
        CloseHandle(FileHandle);
        return FALSE;
    }

    //
    //  Check if kernel lied and overflowed the buffer
    //

    if (BytesReturned > DirContext->ReparseDataBufferLength) {
        CloseHandle(FileHandle);
        return FALSE;
    }

    //
    //  Check that the length in the buffer is consistent with the length
    //  returned
    //

    if (BytesReturned < sizeof(DWORDLONG)) {
        CloseHandle(FileHandle);
        return FALSE;
    }

    if (DirContext->ReparseDataBuffer->ReparseDataLength > BytesReturned - sizeof(DWORDLONG)) {
        CloseHandle(FileHandle);
        return FALSE;
    }

    CloseHandle(FileHandle);
    return TRUE;
}


/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the dir context structure indicating the
        action to perform and populated with the number of objects found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
DirFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    SYSTEMTIME FileWriteTime;
    FILETIME LocalFileTime;
    YORI_STRING SizeString;
    TCHAR SizeStringBuffer[DIR_SIZE_FIELD_SIZE];
    TCHAR DateStringBuffer[DIR_DATE_FIELD_SIZE];
    TCHAR TimeStringBuffer[DIR_TIME_FIELD_SIZE];
    TCHAR YearStringBuffer[5];
    LARGE_INTEGER FileSize;
    LPTSTR FilePart;
    PDIR_CONTEXT DirContext = (PDIR_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);
    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    FilePart = YoriLibFindRightMostCharacter(FilePath, '\\');
    ASSERT(FilePart != NULL);
    if (FilePart != NULL) {
        YORI_STRING ThisDirName;
        YoriLibInitEmptyString(&ThisDirName);
        ThisDirName.StartOfString = FilePath->StartOfString;
        ThisDirName.LengthInChars = (DWORD)(FilePart - FilePath->StartOfString);
        if (ThisDirName.LengthInChars == (sizeof("\\\\?\\c:") - 1) &&
            YoriLibIsPrefixedDriveLetterWithColon(&ThisDirName)) {

            ThisDirName.LengthInChars++;
        }
        if (YoriLibCompareString(&ThisDirName, &DirContext->CurrentDirectoryName) != 0) {
            if (DirContext->ObjectsFoundInThisDir != 0 && !DirContext->MinimalDisplay) {
                DirOutputEndOfDirectorySummary(DirContext);
            }
            if (DirContext->CurrentDirectoryName.LengthAllocated <= ThisDirName.LengthInChars) {
                YoriLibFreeStringContents(&DirContext->CurrentDirectoryName);
                if (!YoriLibAllocateString(&DirContext->CurrentDirectoryName, ThisDirName.LengthInChars + 80)) {
                    return FALSE;
                }
            }
            memcpy(DirContext->CurrentDirectoryName.StartOfString,
                   ThisDirName.StartOfString,
                   ThisDirName.LengthInChars * sizeof(TCHAR));
            DirContext->CurrentDirectoryName.StartOfString[ThisDirName.LengthInChars] = '\0';
            DirContext->CurrentDirectoryName.LengthInChars = ThisDirName.LengthInChars;
            if (!DirContext->MinimalDisplay) {
                DirOutputBeginningOfDirectorySummary(DirContext);
            }
        }
        FilePart++;
    }

    if (DirContext->HideHidden &&
        (FileInfo->dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0) {

        return TRUE;
    }

    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        DirContext->DirsFound++;
    } else {
        DirContext->FilesFound++;
    }
    DirContext->ObjectsFoundInThisDir++;

    YoriLibInitEmptyString(&SizeString);

    if (DirContext->MinimalDisplay) {

        //
        //  In bare mode, CMD never displays short file names.  It displays
        //  full paths when operating recursively and names only when not.
        //  Sound buggy?  Maybe, but it simplifies things, so...
        //

        if (DirContext->Recursive) {
            YORI_STRING UnescapedPath;
            YoriLibInitEmptyString(&UnescapedPath);
            if (YoriLibUnescapePath(FilePath, &UnescapedPath)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &UnescapedPath);
                YoriLibFreeStringContents(&UnescapedPath);
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), FilePath);
            }
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s\n"), FilePart);
        }

    } else {

        YORI_STRING VtAttribute;
        YORI_STRING LocalFileName;
        TCHAR VtAttributeBuffer[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];
        YORILIB_COLOR_ATTRIBUTES Attribute;
        BOOLEAN DisplayReparseBuffer;

        YoriLibInitEmptyString(&VtAttribute);
        YoriLibInitEmptyString(&LocalFileName);

        FileTimeToLocalFileTime(&FileInfo->ftLastWriteTime, &LocalFileTime);
        FileTimeToSystemTime(&LocalFileTime, &FileWriteTime);
        SizeString.StartOfString = SizeStringBuffer;
        SizeString.LengthAllocated = sizeof(SizeStringBuffer)/sizeof(SizeStringBuffer[0]);
        DisplayReparseBuffer = FALSE;

        if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
            if (FileInfo->dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT) {
                YoriLibYPrintf(&SizeString, _T(" <JUNCTION>"));
                DisplayReparseBuffer = TRUE;
            } else if (FileInfo->dwReserved0 == IO_REPARSE_TAG_SYMLINK) {
                if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                    YoriLibYPrintf(&SizeString, _T(" <SYMLINKD>"));
                } else {
                    YoriLibYPrintf(&SizeString, _T(" <SYMLINK>"));
                }
                DisplayReparseBuffer = TRUE;
            } else if (FileInfo->dwReserved0 == IO_REPARSE_TAG_APPEXECLINK) {
                YoriLibYPrintf(&SizeString, _T(" <APP>"));
                DisplayReparseBuffer = TRUE;
            }

            //
            //  If the entry should display the reparse buffer contents after
            //  the file name, allocate memory for the buffer if necessary and
            //  load the buffer
            //

            if (DisplayReparseBuffer) {
                if (DirContext->ReparseDataBuffer == NULL) {
                    DirContext->ReparseDataBuffer = YoriLibMalloc(64 * 1024);
                    if (DirContext->ReparseDataBuffer == NULL) {
                        DisplayReparseBuffer = FALSE;
                    } else {
                        DirContext->ReparseDataBufferLength = 64 * 1024;
                    }
                }
            }

            if (DisplayReparseBuffer) {
                if (!DirLoadReparseData(FilePath, DirContext)) {
                    DisplayReparseBuffer = FALSE;
                }
            }

            //
            //  Find the string within the reparse buffer to display
            //

            if (DisplayReparseBuffer) {
                YORI_STRING ReparseString;
                YoriLibInitEmptyString(&ReparseString);

                if (FileInfo->dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT) {
                    if ((DWORD)DirContext->ReparseDataBuffer->u.MountPoint.DisplayNameOffsetInBytes + DirContext->ReparseDataBuffer->u.MountPoint.DisplayNameLengthInBytes <= DirContext->ReparseDataBufferLength) {
                        ReparseString.StartOfString = YoriLibAddToPointer(&DirContext->ReparseDataBuffer->u.MountPoint.Buffer, DirContext->ReparseDataBuffer->u.MountPoint.DisplayNameOffsetInBytes);
                        ReparseString.LengthInChars = DirContext->ReparseDataBuffer->u.MountPoint.DisplayNameLengthInBytes / sizeof(WCHAR);
                    }
                } else if (FileInfo->dwReserved0 == IO_REPARSE_TAG_SYMLINK) {
                    if ((DWORD)DirContext->ReparseDataBuffer->u.SymLink.DisplayNameOffsetInBytes + DirContext->ReparseDataBuffer->u.SymLink.DisplayNameLengthInBytes <= DirContext->ReparseDataBufferLength) {
                        ReparseString.StartOfString = YoriLibAddToPointer(&DirContext->ReparseDataBuffer->u.SymLink.Buffer, DirContext->ReparseDataBuffer->u.SymLink.DisplayNameOffsetInBytes);
                        ReparseString.LengthInChars = DirContext->ReparseDataBuffer->u.SymLink.DisplayNameLengthInBytes / sizeof(WCHAR);
                    }
                } else if (FileInfo->dwReserved0 == IO_REPARSE_TAG_APPEXECLINK) {
                    DWORD CharCount;
                    DWORD CharIndex;
                    DWORD StringIndex;
                    LPTSTR StartOfString;
                    if (DirContext->ReparseDataBuffer->ReparseDataLength > sizeof(DWORD)) {
                        CharCount = (DirContext->ReparseDataBuffer->ReparseDataLength - sizeof(DWORD)) / sizeof(WCHAR);
                        StartOfString = DirContext->ReparseDataBuffer->u.AppxLink.Buffer;
                        StringIndex = 0;

                        for (CharIndex = 0; CharIndex < CharCount; CharIndex++) {
                            if (StartOfString[CharIndex] == '\0') {
                                StringIndex++;
                                if (StringIndex == 2) {
                                    ReparseString.StartOfString = &StartOfString[CharIndex + 1];
                                } else if (StringIndex == 3) {
                                    ReparseString.LengthInChars = (DWORD)(&StartOfString[CharIndex] - ReparseString.StartOfString);
                                }
                            }
                        }
                    }
                }

                //
                //  Allocate a new buffer to contain both the file name and
                //  reparse string, and alter the display name to point to
                //  it
                //

                if (ReparseString.LengthInChars > 0) {
                    YoriLibYPrintf(&LocalFileName, _T("%s [%y]"), FilePart, &ReparseString);
                    if (LocalFileName.StartOfString != NULL) {
                        FilePart = LocalFileName.StartOfString;
                    }
                }

            }
        }

        if (SizeString.LengthInChars == 0) {
            if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                YoriLibYPrintf(&SizeString, _T(" <DIR>"));
                DirContext->DirsFoundInThisDir++;
            } else {
                FileSize.HighPart = FileInfo->nFileSizeHigh;
                FileSize.LowPart = FileInfo->nFileSizeLow;
                DirContext->FileSizeInThisDir += FileSize.QuadPart;
                DirContext->TotalFileSize += FileSize.QuadPart;
                YoriLibNumberToString(&SizeString, FileSize.QuadPart, 10, 3, ',');
                if (SizeString.LengthInChars < DIR_SIZE_FIELD_SIZE) {
                    DirRightAlignString(&SizeString, DIR_SIZE_FIELD_SIZE);
                }
                DirContext->FilesFoundInThisDir++;
            }
        }

        if (SizeString.LengthInChars < DIR_SIZE_FIELD_SIZE) {
            DWORD Index;
            for (Index = SizeString.LengthInChars; Index < DIR_SIZE_FIELD_SIZE; Index++) {
                SizeString.StartOfString[Index] = ' ';
            }
            SizeString.LengthInChars = DIR_SIZE_FIELD_SIZE;
        }

        if (DirContext->ColorRules.NumberCriteria) {
            VtAttribute.StartOfString = VtAttributeBuffer;
            VtAttribute.LengthAllocated = sizeof(VtAttributeBuffer)/sizeof(VtAttributeBuffer[0]);

            if (!YoriLibFileFiltCheckColorMatch(&DirContext->ColorRules, FilePath, FileInfo, &Attribute)) {
                Attribute.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG;
                Attribute.Win32Attr = (UCHAR)YoriLibVtGetDefaultColor();
            }

            YoriLibVtStringForTextAttribute(&VtAttribute, Attribute.Ctrl, Attribute.Win32Attr);
        }

        if (DirContext->FourCharYear) {
            YoriLibSPrintf(YearStringBuffer, _T("%04i"), FileWriteTime.wYear);
        } else {
            YoriLibSPrintf(YearStringBuffer, _T("%02i"), FileWriteTime.wYear % 100);
        }

        ASSERT(DirContext->DateOrder == DIR_DATE_ORDER_MDY ||
               DirContext->DateOrder == DIR_DATE_ORDER_DMY ||
               DirContext->DateOrder == DIR_DATE_ORDER_YMD);

        if (DirContext->DateOrder == DIR_DATE_ORDER_MDY) {
            YoriLibSPrintf(DateStringBuffer, _T("%02i%c%02i%c%s"), FileWriteTime.wMonth, DirContext->DateSeperator, FileWriteTime.wDay, DirContext->DateSeperator, YearStringBuffer);
        } else if (DirContext->DateOrder == DIR_DATE_ORDER_DMY) {
            YoriLibSPrintf(DateStringBuffer, _T("%02i%c%02i%c%s"), FileWriteTime.wDay, DirContext->DateSeperator, FileWriteTime.wMonth, DirContext->DateSeperator, YearStringBuffer);
        } else {
            YoriLibSPrintf(DateStringBuffer, _T("%s%c%02i%c%02i"), YearStringBuffer, DirContext->DateSeperator, FileWriteTime.wMonth, DirContext->DateSeperator, FileWriteTime.wDay);
        }

        if (DirContext->Time24Hour) {
            YoriLibSPrintf(TimeStringBuffer, _T("%02i%c%02i"), FileWriteTime.wHour, DirContext->TimeSeperator, FileWriteTime.wMinute);
        } else {
            WORD HourToDisplay;
            BOOLEAN IsPm;

            HourToDisplay = (WORD)(FileWriteTime.wHour % 12);
            if (HourToDisplay == 0) {
                HourToDisplay = 12;
            }

            IsPm = FALSE;
            if (FileWriteTime.wHour >= 12) {
                IsPm = TRUE;
            }

            YoriLibSPrintf(TimeStringBuffer, _T("%02i%c%02i %s"), HourToDisplay, DirContext->TimeSeperator, FileWriteTime.wMinute, IsPm?_T("PM"):_T("AM"));
        }

        if (VtAttribute.LengthInChars > 0) {
            if (DirContext->DisplayShortNames) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s  %s %y %y%12s %s%c[0m\n"), DateStringBuffer, TimeStringBuffer, &SizeString, &VtAttribute, FileInfo->cAlternateFileName, FilePart, 27);
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s  %s %y %y%s%c[0m\n"), DateStringBuffer, TimeStringBuffer, &SizeString, &VtAttribute, FilePart, 27);
            }
        } else {
            if (DirContext->DisplayShortNames) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s  %s %y %12s %s\n"), DateStringBuffer, TimeStringBuffer, &SizeString, FileInfo->cAlternateFileName, FilePart);
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s  %s %y %s\n"), DateStringBuffer, TimeStringBuffer, &SizeString, FilePart);
            }
        }

        if (DirContext->DisplayStreams) {
            HANDLE hFind;
            WIN32_FIND_STREAM_DATA FindStreamData;
            WIN32_FIND_DATA BogusFileInfo;
            YORI_STRING StreamFullPath;

            hFind = DllKernel32.pFindFirstStreamW(FilePath->StartOfString, 0, &FindStreamData, 0);
            if (hFind != INVALID_HANDLE_VALUE) {
                YoriLibInitEmptyString(&StreamFullPath);
                if (DirContext->ColorRules.NumberCriteria) {
                    if (!YoriLibAllocateString(&StreamFullPath, FilePath->LengthInChars + YORI_LIB_MAX_STREAM_NAME)) {
                        YoriLibFreeStringContents(&SizeString);
                        return FALSE;
                    }
                }

                do {
                    if (_tcscmp(FindStreamData.cStreamName, L"::$DATA") != 0) {
                        DWORD StreamLength = (DWORD)_tcslen(FindStreamData.cStreamName);

                        //
                        //  Truncate any trailing :$DATA attribute name
                        //

                        if (StreamLength > 6 && _tcscmp(FindStreamData.cStreamName + StreamLength - 6, L":$DATA") == 0) {
                            FindStreamData.cStreamName[StreamLength - 6] = '\0';
                        }

                        if (DirContext->ColorRules.NumberCriteria) {

                            //
                            //  Generate a full path to the stream
                            //

                            StreamFullPath.LengthInChars = YoriLibSPrintfS(StreamFullPath.StartOfString, StreamFullPath.LengthAllocated, _T("%y%s"), FilePath, FindStreamData.cStreamName);

                            //
                            //  Assume file state is stream state
                            //

                            memcpy(&BogusFileInfo, FileInfo, FIELD_OFFSET(WIN32_FIND_DATA, cFileName));
                            BogusFileInfo.cAlternateFileName[0] = '\0';

                            //
                            //  Populate stream name
                            //

                            YoriLibSPrintfS(BogusFileInfo.cFileName, sizeof(BogusFileInfo.cFileName)/sizeof(BogusFileInfo.cFileName[0]), _T("%s%s"), FileInfo->cFileName, FindStreamData.cStreamName);

                            //
                            //  Update stream information
                            //

                            YoriLibUpdateFindDataFromFileInformation(&BogusFileInfo, StreamFullPath.StartOfString, FALSE);
                            if (!YoriLibFileFiltCheckColorMatch(&DirContext->ColorRules, &StreamFullPath, &BogusFileInfo, &Attribute)) {
                                Attribute.Ctrl = YORILIB_ATTRCTRL_WINDOW_BG | YORILIB_ATTRCTRL_WINDOW_FG;
                                Attribute.Win32Attr = (UCHAR)YoriLibVtGetDefaultColor();
                            }

                            YoriLibVtStringForTextAttribute(&VtAttribute, Attribute.Ctrl, Attribute.Win32Attr);
                        }
                        YoriLibNumberToString(&SizeString, FindStreamData.StreamSize.QuadPart, 10, 3, ',');
                        if (SizeString.LengthInChars < DIR_SIZE_FIELD_SIZE) {
                            DirRightAlignString(&SizeString, DIR_SIZE_FIELD_SIZE);
                        }
                        if (VtAttribute.LengthInChars > 0) {
                            if (DirContext->DisplayShortNames) {
                                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%18s%y %13s%y%s%s%c[0m\n"), _T(""), &SizeString, _T(""), &VtAttribute, FileInfo->cFileName, FindStreamData.cStreamName, 27);
                            } else {
                                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%18s%y %y%s%s%c[0m\n"), _T(""), &SizeString, &VtAttribute, FileInfo->cFileName, FindStreamData.cStreamName, 27);
                            }
                        } else {
                            if (DirContext->DisplayShortNames) {
                                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%18s%y %13s%s%s\n"), _T(""), &SizeString, _T(""), FileInfo->cFileName, FindStreamData.cStreamName);
                            } else {
                                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%18s%y %s%s\n"), _T(""), &SizeString, FileInfo->cFileName, FindStreamData.cStreamName);
                            }
                        }
                    }
                } while (DllKernel32.pFindNextStreamW(hFind, &FindStreamData));
                FindClose(hFind);
                YoriLibFreeStringContents(&StreamFullPath);
            }
        }

        YoriLibFreeStringContents(&SizeString);
        YoriLibFreeStringContents(&LocalFileName);
    }

    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the DirContext, indicating whether the request is
        recursive or not.  Recursive enumerates are more willing to continue
        on error than single directory enumerates.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
DirFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;
    PDIR_CONTEXT DirContext = (PDIR_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        Result = TRUE;
    } else {
        LPTSTR ErrText = YoriLibGetWinErrorText(ErrorCode);
        YORI_STRING DirName;
        LPTSTR FilePart;
        YoriLibInitEmptyString(&DirName);
        DirName.StartOfString = UnescapedFilePath.StartOfString;
        FilePart = YoriLibFindRightMostCharacter(&UnescapedFilePath, '\\');
        if (FilePart != NULL) {
            DirName.LengthInChars = (DWORD)(FilePart - DirName.StartOfString);
        } else {
            DirName.LengthInChars = UnescapedFilePath.LengthInChars;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Enumerate of %y failed: %s"), &DirName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        if (DirContext->Recursive) {
            Result = TRUE;
        }
    }
    YoriLibFreeStringContents(&UnescapedFilePath);
    return Result;
}

/**
 Determine the seperators, order of display and general formatting for times
 and dates based on the current user's locale.  Fallback to sensible defaults
 if the information is not available.

 @param DirContext On successful completion, fields within the DirContext
        indicating time and date formatting are updated with the current
        user's locale settings.
 */
VOID
DirLoadLocaleSettings(
    __inout PDIR_CONTEXT DirContext
    )
{
    WCHAR Buffer[4];
    DWORD CharsReturned;

    DirContext->DateSeperator = '/';
    DirContext->DateOrder = DIR_DATE_ORDER_YMD;
    DirContext->FourCharYear = TRUE;
    DirContext->TimeSeperator = ':';
    DirContext->Time24Hour = TRUE;

    if (!DirContext->IgnoreLocale) {

        CharsReturned = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDATE, Buffer, sizeof(Buffer)/sizeof(Buffer[0]));
        if (CharsReturned > 0 && CharsReturned <= sizeof(Buffer)/sizeof(Buffer[0])) {
            DirContext->DateSeperator = Buffer[0];
        }

        CharsReturned = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDATE, Buffer, sizeof(Buffer)/sizeof(Buffer[0]));
        if (CharsReturned > 0 && CharsReturned <= sizeof(Buffer)/sizeof(Buffer[0]) && Buffer[0] >= '0' && Buffer[0] <= '2') {
            DirContext->DateOrder = (TCHAR)(Buffer[0] - '0');
        }

        CharsReturned = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ICENTURY, Buffer, sizeof(Buffer)/sizeof(Buffer[0]));
        if (CharsReturned > 0 && CharsReturned <= sizeof(Buffer)/sizeof(Buffer[0]) && Buffer[0] == '0') {
            DirContext->FourCharYear = FALSE;
        }

        CharsReturned = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STIME, Buffer, sizeof(Buffer)/sizeof(Buffer[0]));
        if (CharsReturned > 0 && CharsReturned <= sizeof(Buffer)/sizeof(Buffer[0])) {
            DirContext->TimeSeperator = Buffer[0];
        }

        CharsReturned = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ITIME, Buffer, sizeof(Buffer)/sizeof(Buffer[0]));
        if (CharsReturned > 0 && CharsReturned <= sizeof(Buffer)/sizeof(Buffer[0]) && Buffer[0] == '0') {
            DirContext->Time24Hour = FALSE;
        }
    }
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the dir builtin command.
 */
#define ENTRYPOINT YoriCmd_YDIR
#else
/**
 The main entrypoint for the dir standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the dir cmdlet.

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
    BOOL DisplayColor = FALSE;
    DIR_CONTEXT DirContext;
    YORI_STRING Arg;

    ZeroMemory(&DirContext, sizeof(DirContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                DirHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("color")) == 0) {
                DisplayColor = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("g")) == 0) {
                DirContext.IgnoreLocale = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                DirContext.HideHidden = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("m")) == 0) {
                DirContext.MinimalDisplay = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                DirContext.DisplayStreams = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                DirContext.Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("x")) == 0) {
                DirContext.DisplayShortNames = TRUE;
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

    if (DirContext.DisplayStreams) {
        if (DllKernel32.pFindFirstStreamW == NULL || DllKernel32.pFindNextStreamW == NULL) {
            DirContext.DisplayStreams = FALSE;
        }
    }

    if (DisplayColor) {
        YORI_STRING ErrorSubstring;
        YORI_STRING Combined;
        if (YoriLibLoadCombinedFileColorString(NULL, &Combined)) {
            if (!YoriLibFileFiltParseColorString(&DirContext.ColorRules, &Combined, &ErrorSubstring)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("dir: parse error at %y\n"), &ErrorSubstring);
            }
            YoriLibFreeStringContents(&Combined);
        }
    }

#if YORI_BUILTIN
    YoriLibCancelEnable();
#endif

    DirLoadLocaleSettings(&DirContext);

    //
    //  Attempt to enable backup privilege so an administrator can access more
    //  objects successfully.
    //

    YoriLibEnableBackupPrivilege();

    //
    //  If no file name is specified, use *
    //

    MatchFlags = YORILIB_FILEENUM_RETURN_FILES |
                 YORILIB_FILEENUM_RETURN_DIRECTORIES |
                 YORILIB_FILEENUM_DIRECTORY_CONTENTS;
    if (!DirContext.MinimalDisplay) {
        MatchFlags |= YORILIB_FILEENUM_INCLUDE_DOTFILES;
    }
    if (DirContext.Recursive) {
        MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
    }
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    if (StartArg == 0 || StartArg == ArgC) {
        YORI_STRING FilesInDirectorySpec;
        YoriLibConstantString(&FilesInDirectorySpec, _T("*"));
        YoriLibForEachStream(&FilesInDirectorySpec,
                             MatchFlags,
                             0,
                             DirFileFoundCallback,
                             DirFileEnumerateErrorCallback,
                             &DirContext);
    } else {
        for (i = StartArg; i < ArgC; i++) {
            YoriLibForEachStream(&ArgV[i],
                                 MatchFlags,
                                 0,
                                 DirFileFoundCallback,
                                 DirFileEnumerateErrorCallback,
                                 &DirContext);
        }
    }

    if (DirContext.ObjectsFoundInThisDir > 0 && !DirContext.MinimalDisplay) {
        DirOutputEndOfDirectorySummary(&DirContext);
    } 

    YoriLibFreeStringContents(&DirContext.CurrentDirectoryName);
    YoriLibFileFiltFreeFilter(&DirContext.ColorRules);
    if (DirContext.ReparseDataBuffer != NULL) {
        YoriLibFree(DirContext.ReparseDataBuffer);
    }

    if (DirContext.FilesFound == 0 && DirContext.DirsFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("dir: no matching files found\n"));
        return EXIT_FAILURE;
    } else if (DirContext.Recursive) {
        DirOutputEndOfRecursiveSummary(&DirContext);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
