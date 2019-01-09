/**
 * @file lib/cabinet.c
 *
 * Yori shell extract .cab files
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
 Context information to pass around as files are being expanded.
 */
typedef struct _YORI_LIB_CAB_EXPAND_CONTEXT {

    /**
     The directory to expand files into.
     */
    PYORI_STRING TargetDirectory;

    /**
     If TRUE, all files not matching any list below are expanded.  If not,
     only files explicitly listed are expanded.
     */
    BOOL DefaultInclude;

    /**
     The number of files in the FilesToInclude array.
     */
    DWORD NumberFilesToInclude;

    /**
     An array of strings corresponding to files that should be expanded.
     */
    PYORI_STRING FilesToInclude;

    /**
     The number of files in the FilesToExclude array.
     */
    DWORD NumberFilesToExclude;

    /**
     An array of strings corresponding to files that should not be expanded.
     */
    PYORI_STRING FilesToExclude;

    /**
     A user specified callback to provide notification for a given file.
     */
    PYORI_LIB_CAB_EXPAND_FILE_CALLBACK CommenceExtractCallback;

    /**
     A user specified callback to provide notification for a given file.
     */
    PYORI_LIB_CAB_EXPAND_FILE_CALLBACK CompleteExtractCallback;

    /**
     Context information to pass to the user specified callback.
     */
    PVOID UserContext;

    /**
     Optionally points to a string to populate with error information to
     display to a user.
     */
    PYORI_STRING ErrorString;

} YORI_LIB_CAB_EXPAND_CONTEXT, *PYORI_LIB_CAB_EXPAND_CONTEXT;

/**
 A callback invoked during FDICopy to allocate memory.

 @param Bytes The number of bytes to allocate.
 
 @return Pointer to allocated memory or NULL on failure.
 */
LPVOID DIAMONDAPI
YoriLibCabAlloc(
    __in DWORD Bytes
    )
{
    LPVOID Alloc;
    Alloc = YoriLibMalloc(Bytes);
    return Alloc;
}

/**
 A callback invoked during FDICopy to free memory.

 @param Alloc The allocation to free.
 */
VOID DIAMONDAPI
YoriLibCabFree(
    __in PVOID Alloc
    )
{
    YoriLibFree(Alloc);
}

/**
 Notification that a file has been placed into a CAB when compressing new
 files.

 @param CabContext Pointer to the cabinet structure which can be populated
        with new information.

 @param FileNameInCab Pointer to an ANSI NULL terminated string indicating
        the file that was placed in the CAB.

 @param FileLength Indicates the length of the file placed into the CAB,
        in bytes.

 @param Continuation TRUE if the file has been split into segments and this
        is a subsequent segment.

 @param Context Pointer to user context, currently ignored.

 @return Zero to indicate success.
 */
DWORD_PTR DIAMONDAPI
YoriLibCabFciFilePlaced(
    __in PCAB_FCI_CONTEXT CabContext,
    __in LPSTR FileNameInCab,
    __in DWORD FileLength,
    __in BOOL Continuation,
    __in PVOID Context
    )
{
    UNREFERENCED_PARAMETER(CabContext);
    UNREFERENCED_PARAMETER(FileNameInCab);
    UNREFERENCED_PARAMETER(FileLength);
    UNREFERENCED_PARAMETER(Continuation);
    UNREFERENCED_PARAMETER(Context);

    return 0;
}

/**
 Bits indicating a file should be opened read only.
 */
#define YORI_LIB_CAB_OPEN_READONLY   (0x0000)

/**
 Bits indicating a file should be opened write only.
 */
#define YORI_LIB_CAB_OPEN_WRITEONLY  (0x0001)

/**
 Bits indicating a file should be opened read and write.
 */
#define YORI_LIB_CAB_OPEN_READWRITE  (0x0002)

/**
 The set of bits to compare when trying to discover the open mode.
 */
#define YORI_LIB_CAB_OPEN_MASK       (0x0003)

/**
 A callback invoked during FCI to open a file.  Note that this is used
 on the same file multiple times and thus requires sharing with previous
 opens.
 
 @param FileName A NULL terminated ANSI (sigh) string indicating the file
        name.

 @param OFlag The open flags, apparently modelled on the CRT but don't
        really match those values.

 @param PMode No idea (per MSDN.)

 @param Err Optionally points to an integer which could be populated with
        extra error information.

 @param Context Optionally points to user context, which is currently unused.

 @return File handle.
 */
DWORD_PTR DIAMONDAPI
YoriLibCabFciFileOpen(
    __in LPSTR FileName,
    __in INT OFlag,
    __in INT PMode,
    __inout_opt PINT Err,
    __inout_opt PVOID Context
    )
{
    HANDLE hFile;
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD Disposition;
    DWORD OpenType = ((DWORD)OFlag & YORI_LIB_CAB_OPEN_MASK);

    UNREFERENCED_PARAMETER(PMode);
    UNREFERENCED_PARAMETER(Err);
    UNREFERENCED_PARAMETER(Context);

    switch(OpenType) {
        case YORI_LIB_CAB_OPEN_READONLY:
            DesiredAccess = GENERIC_READ;
            Disposition = OPEN_EXISTING;
            ShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
            break;
        case YORI_LIB_CAB_OPEN_WRITEONLY:
            DesiredAccess = GENERIC_WRITE;
            Disposition = CREATE_ALWAYS;
            ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            break;
        case YORI_LIB_CAB_OPEN_READWRITE:
            DesiredAccess = GENERIC_READ | GENERIC_WRITE;
            Disposition = OPEN_ALWAYS;
            ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            break;
        default:
            return (DWORD_PTR)INVALID_HANDLE_VALUE;
    }

    hFile = CreateFileA(FileName,
                        DesiredAccess,
                        ShareMode,
                        NULL,
                        Disposition,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    return (DWORD_PTR)hFile;
}

/**
 A callback invoked during FDICopy to open a file.  Note that this is used
 on the same file multiple times and thus requires sharing with previous
 opens.
 
 @param FileName A NULL terminated ANSI (sigh) string indicating the file
        name.

 @param OFlag The open flags, apparently modelled on the CRT but don't
        really match those values.

 @param PMode No idea (per MSDN.)

 @return File handle.
 */
DWORD_PTR DIAMONDAPI
YoriLibCabFdiFileOpen(
    __in LPSTR FileName,
    __in INT OFlag,
    __in INT PMode
    )
{
    return YoriLibCabFciFileOpen(FileName, OFlag, PMode, NULL, NULL);
}


/**
 Combine a parent directory with the CAB's ANSI relative file name (sigh) and
 output the combined path and Unicode form of the relative file name.

 @param ParentDirectory Pointer to the parent directory to place files.

 @param FileName Pointer to a NULL terminated ANSI string for the name of the
        object within the CAB.

 @param FullPathName If specified, points to a Yori string to receive the
        full path name.  The memory is allocated in this routine and returned
        with a reference.

 @param FileNameOnly If specified, points to a Yori string to receive the
        file name without parent path.  The memory is allocated in this
        routine and is returned with a reference.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCabBuildFileNames(
    __in PYORI_STRING ParentDirectory,
    __in LPSTR FileName,
    __out_opt PYORI_STRING FullPathName,
    __out_opt PYORI_STRING FileNameOnly
    )
{
    DWORD NameLengthInChars = strlen(FileName);
    YORI_STRING FullPath;
    DWORD ExtraChars;

    if (!YoriLibAllocateString(&FullPath, ParentDirectory->LengthInChars + 1 + NameLengthInChars + 1)) {
        return FALSE;
    }

    if (ParentDirectory->LengthInChars >= 1 &&
        ParentDirectory->StartOfString[ParentDirectory->LengthInChars - 1] == '\\') {

        FullPath.LengthInChars = YoriLibSPrintf(FullPath.StartOfString, _T("%y%hs"), ParentDirectory, FileName);
        ExtraChars = 0;
    } else {
        FullPath.LengthInChars = YoriLibSPrintf(FullPath.StartOfString, _T("%y\\%hs"), ParentDirectory, FileName);
        ExtraChars = 1;
    }

    if (FullPathName != NULL) {
        YoriLibCloneString(FullPathName, &FullPath);
    }

    if (FileNameOnly != NULL) {
        YoriLibCloneString(FileNameOnly, &FullPath);
        FileNameOnly->StartOfString += ParentDirectory->LengthInChars + ExtraChars;
        FileNameOnly->LengthInChars -= ParentDirectory->LengthInChars + ExtraChars;
    }

    YoriLibFreeStringContents(&FullPath);

    return TRUE;
}

/**
 Return TRUE to indicate a specified file should be expanded, or FALSE if
 it should not be.

 @param FileName Pointer to a string containing the relative file name within
        the CAB (ie., no destination path.)

 @param ExpandContext Pointer to the expand context containing the set of
        files to include or exclude.

 @return TRUE to indicate a file should be expanded, or FALSE if it should
         not be.
 */
BOOL
YoriLibCabShouldIncludeFile(
    __in PYORI_STRING FileName,
    __in PYORI_LIB_CAB_EXPAND_CONTEXT ExpandContext
    )
{
    BOOL IncludeFile = ExpandContext->DefaultInclude;
    DWORD Count;

    if (IncludeFile) {
        for (Count = 0; Count < ExpandContext->NumberFilesToExclude; Count++) {
            if (YoriLibCompareStringInsensitive(FileName, &ExpandContext->FilesToExclude[Count]) == 0) {
                IncludeFile = FALSE;
                break;
            }
        }
    }

    if (!IncludeFile) {
        for (Count = 0; Count < ExpandContext->NumberFilesToInclude; Count++) {
            if (YoriLibCompareStringInsensitive(FileName, &ExpandContext->FilesToInclude[Count]) == 0) {
                IncludeFile = TRUE;
                break;
            }
        }
    }

    return IncludeFile;
}

/**
 Open a new file being extracted from a Cabinet.  This implies the file is
 being opened for write.  Create a single parent directory if it doesn't
 exist yet, and if the file exists already, try to move it out of the way
 and attempt to delete it.  Note that for running executables this case is
 very common.

 @param FullPath Pointer to a string describing the file to open.

 @param ErrorString Optionally points to a string to populate with information
        about any error encountered in the extraction process.

 @return Handle to the opened file, or INVALID_HANDLE_VALUE on failure.
 */
DWORD_PTR
YoriLibCabFileOpenForExtract(
    __in PYORI_STRING FullPath,
    __out_opt PYORI_STRING ErrorString
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD Err = 0;

    while (TRUE) {

        //
        //  Try to open the target file
        //

        hFile = CreateFile(FullPath->StartOfString,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            break;
        }

        Err = GetLastError();

        //
        //  If the path isn't found, try to create one parent component and
        //  retry the create.
        //

        if (Err == ERROR_PATH_NOT_FOUND) {
            LPTSTR LastSep;
            DWORD OldLength = FullPath->LengthInChars;

            LastSep = YoriLibFindRightMostCharacter(FullPath, '\\');
            if (LastSep == NULL) {
                break;
            }

            *LastSep = '\0';
            FullPath->LengthInChars = (DWORD)(LastSep - FullPath->StartOfString);

            if (!YoriLibCreateDirectoryAndParents(FullPath)) {
                Err = GetLastError();
                hFile = INVALID_HANDLE_VALUE;
            } else {

                FullPath->LengthInChars = OldLength;
                *LastSep = '\\';

                hFile = CreateFile(FullPath->StartOfString,
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL,
                                   CREATE_NEW,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

                if (hFile == INVALID_HANDLE_VALUE) {
                    Err = GetLastError();
                }
            }

            break;
        } else {
            break;
        }
    }

    if (hFile == INVALID_HANDLE_VALUE && ErrorString != NULL) {
        LPTSTR ErrText;
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibYPrintf(ErrorString, _T("Error opening %y: %s"), FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
    }

    return (DWORD_PTR)hFile;
}

/**
 A callback invoked during FDICopy to read from a file.  Note that these
 callbacks always refer to the "current file position".
 
 @param FileHandle The file to read from.
 
 @param Buffer Pointer to a block of memory to place read data.

 @param ByteCount The number of bytes to read.

 @param Err Optionally points to an integer which could be populated with
        extra error information.

 @param Context Optionally points to user context, which is currently unused.

 @return The number of bytes actually read or -1 on failure.
 */
DWORD DIAMONDAPI
YoriLibCabFciFileRead(
    __in DWORD_PTR FileHandle,
    __out PVOID Buffer,
    __in DWORD ByteCount,
    __inout_opt PINT Err,
    __inout_opt PVOID Context
    )
{
    DWORD BytesRead;
    UNREFERENCED_PARAMETER(Err);
    UNREFERENCED_PARAMETER(Context);
    if (!ReadFile((HANDLE)FileHandle,
                  Buffer,
                  ByteCount,
                  &BytesRead,
                  NULL)) {
        return (DWORD)-1;
    }

    return BytesRead;
}

/**
 A callback invoked during FDICopy to read from a file.  Note that these
 callbacks always refer to the "current file position".
 
 @param FileHandle The file to read from.
 
 @param Buffer Pointer to a block of memory to place read data.

 @param ByteCount The number of bytes to read.

 @return The number of bytes actually read or -1 on failure.
 */
DWORD DIAMONDAPI
YoriLibCabFdiFileRead(
    __in DWORD_PTR FileHandle,
    __out PVOID Buffer,
    __in DWORD ByteCount
    )
{
    return YoriLibCabFciFileRead(FileHandle, Buffer, ByteCount, NULL, NULL);
}

/**
 A callback invoked during FDICopy to write to file.  Note that these
 callbacks always refer to the "current file position".
 
 @param FileHandle The file to write to.
 
 @param Buffer Pointer to a block of memory containing data to write.

 @param ByteCount The number of bytes to write.

 @param Err Optionally points to an integer which could be populated with
        extra error information.

 @param Context Optionally points to user context, which is currently unused.

 @return The number of bytes actually written or -1 on failure.
 */
DWORD DIAMONDAPI
YoriLibCabFciFileWrite(
    __in DWORD_PTR FileHandle,
    __in PVOID Buffer,
    __in DWORD ByteCount,
    __inout_opt PINT Err,
    __inout_opt PVOID Context
    )
{
    DWORD BytesWritten;
    UNREFERENCED_PARAMETER(Err);
    UNREFERENCED_PARAMETER(Context);
    if (!WriteFile((HANDLE)FileHandle,
                   Buffer,
                   ByteCount,
                   &BytesWritten,
                   NULL)) {
        return (DWORD)-1;
    }

    return BytesWritten;
}

/**
 A callback invoked during FDICopy to write to file.  Note that these
 callbacks always refer to the "current file position".
 
 @param FileHandle The file to write to.
 
 @param Buffer Pointer to a block of memory containing data to write.

 @param ByteCount The number of bytes to write.

 @return The number of bytes actually written or -1 on failure.
 */
DWORD DIAMONDAPI
YoriLibCabFdiFileWrite(
    __in DWORD_PTR FileHandle,
    __in PVOID Buffer,
    __in DWORD ByteCount
    )
{
    return YoriLibCabFciFileWrite(FileHandle, Buffer, ByteCount, NULL, NULL);
}

/**
 A callback invoked during FDICopy to close a file.
 
 @param FileHandle The file to close.

 @param Err Optionally points to an integer which could be populated with
        extra error information.

 @param Context Optionally points to user context, which is currently unused.

 @return Zero for success, nonzero to indicate an error.
 */
INT DIAMONDAPI
YoriLibCabFciFileClose(
    __in DWORD_PTR FileHandle,
    __inout_opt PINT Err,
    __inout_opt PVOID Context
    )
{
    UNREFERENCED_PARAMETER(Err);
    UNREFERENCED_PARAMETER(Context);

    CloseHandle((HANDLE)FileHandle);
    return 0;
}

/**
 A callback invoked during FDICopy to close a file.
 
 @param FileHandle The file to close.

 @return Zero for success, nonzero to indicate an error.
 */
INT DIAMONDAPI
YoriLibCabFdiFileClose(
    __in DWORD_PTR FileHandle
    )
{
    return YoriLibCabFciFileClose(FileHandle, NULL, NULL);
}

/**
 A callback invoked during FDICopy to change current file position.
 
 @param FileHandle The file to set current position on.

 @param DistanceToMove The number of bytes to move.

 @param SeekType The origin of the move.

 @param Err Optionally points to an integer which could be populated with
        extra error information.

 @param Context Optionally points to user context, which is currently unused.

 @return The new file position.
 */
DWORD DIAMONDAPI
YoriLibCabFciFileSeek(
    __in DWORD_PTR FileHandle,
    __in DWORD DistanceToMove,
    __in INT SeekType,
    __inout_opt PINT Err,
    __inout_opt PVOID Context
    )
{
    DWORD NewPosition;

    UNREFERENCED_PARAMETER(Err);
    UNREFERENCED_PARAMETER(Context);

    NewPosition = SetFilePointer((HANDLE)FileHandle,
                                 DistanceToMove,
                                 NULL,
                                 SeekType);

    return NewPosition;
}

/**
 A callback invoked during FDICopy to change current file position.
 
 @param FileHandle The file to set current position on.

 @param DistanceToMove The number of bytes to move.

 @param SeekType The origin of the move.

 @return The new file position.
 */
DWORD DIAMONDAPI
YoriLibCabFdiFileSeek(
    __in DWORD_PTR FileHandle,
    __in DWORD DistanceToMove,
    __in INT SeekType
    )
{
    return YoriLibCabFciFileSeek(FileHandle, DistanceToMove, SeekType, NULL, NULL);
}

/**
 A callback to delete a file when creating a CAB.  This is used for temporary
 files.

 @param FileName An ANSI NULL terminated string indicating the file to delete.

 @param Err Optionally points to an integer which could be populated with
        extra error information.

 @param Context Optionally points to user context, which is currently unused.

 @return Zero to indicate success.  This is speculation since I haven't found
         this documented, but it appears to work.
 */
INT DIAMONDAPI
YoriLibCabFciFileDelete(
    __in LPSTR FileName,
    __inout_opt PINT Err,
    __inout_opt PVOID Context
    )
{
    UNREFERENCED_PARAMETER(Err);
    UNREFERENCED_PARAMETER(Context);
    DeleteFileA(FileName);
    return 0;
}

/**
 Return a temporary file name.  Cabinet invokes this with a 256 byte buffer
 (ie., less than MAX_PATH) so this routine constructs the temporary name on
 the stack and copies back the result.

 @param FileName On successful completion, updated to contain a new temporary
        file name.  Note this is a NULL terminated ANSI string.

 @param FileNameLength Indicates the size of the FileName buffer.

 @param Context Optionally points to caller supplied context.  This is
        currently unused.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL DIAMONDAPI
YoriLibCabFciGetTempFile(
    __in LPSTR FileName,
    __in INT FileNameLength,
    __inout_opt PVOID Context
    )
{
    CHAR TempPath[MAX_PATH];
    CHAR LocalTempFile[MAX_PATH];
    DWORD BytesToCopy = MAX_PATH;

    UNREFERENCED_PARAMETER(Context);

    GetTempPathA(sizeof(TempPath), TempPath);
    GetTempFileNameA(TempPath, "YRI", 0, LocalTempFile);
    if (FileNameLength < MAX_PATH) {
        BytesToCopy = FileNameLength;
    }
    memcpy(FileName, LocalTempFile, BytesToCopy);
    FileName[FileNameLength - 1] = '\0';
    return TRUE;
}

/**
 Called when creating a CAB to request a new CAB file and information about it.
 Because this package is trying to only generate single CAB files (not chains)
 this routine should never be invoked and isn't really handled.

 @param CabContext Pointer to the previous cab information.  This could be
        updated as needed to describe the next one.

 @param SizeOfLastCab The size of the last CAB file, in bytes.

 @param Context Optionally points to caller supplied context.  This is
        currently unused.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL DIAMONDAPI
YoriLibCabFciGetNextCabinet(
    __in PCAB_FCI_CONTEXT CabContext,
    __in DWORD SizeOfLastCab,
    __inout_opt PVOID Context
    )
{
    UNREFERENCED_PARAMETER(CabContext);
    UNREFERENCED_PARAMETER(SizeOfLastCab);
    UNREFERENCED_PARAMETER(Context);

    ASSERT(FALSE);

    return TRUE;
}

/**
 A callback function that is called to indicate progress through compressing
 files into a CAB.

 @param StatusType The type of notification.  When this is 2, it's requesting
        the final size of the CAB file to create.

 @param Value1 Type-specific data.

 @param Value2 Type-specific data.  When StatusType is 2, this contains the
        current size of the CAB file.

 @param Context Pointer to user supplied context, currently unused.

 @return Typically zero for success, or the size of the CAB file to create
         when StatusType is 2.
 */
DWORD DIAMONDAPI
YoriLibCabFciStatus(
    __in DWORD StatusType,
    __in DWORD Value1,
    __in DWORD Value2,
    __inout_opt PVOID Context
    )
{
    UNREFERENCED_PARAMETER(Value1);
    UNREFERENCED_PARAMETER(Context);

    if (StatusType == 2) {
        return Value2;
    }

    return 0;
}

/**
 Open a file for read for the purpose of adding it to a CAB, and return
 metadata about the file being opened.

 @param FileName An ANSI NULL-terminated string describing the file to open.

 @param Date On successful completion, updated to point to the MS-DOS 16 bit
        date stamp for the file.

 @param Time On successful completion, updated to point to the MS-DOS 16 bit
        time stamp for the file.

 @param Attributes On successful completion, updated to point to the MS-DOS
        16 bit file attributes for the file.

 @param Err Updated to contain extra error information on failure.

 @param Context Pointer to user context, currently unused.

 @return A file handle.
 */
DWORD_PTR DIAMONDAPI
YoriLibCabFciGetOpenInfo(
    __in LPSTR FileName,
    __out PWORD Date,
    __out PWORD Time,
    __out PWORD Attributes,
    __inout_opt PINT Err,
    __inout_opt PVOID Context
    )
{
    DWORD_PTR Handle;
    BY_HANDLE_FILE_INFORMATION FileInfo;

    UNREFERENCED_PARAMETER(Err);
    UNREFERENCED_PARAMETER(Context);

    Handle = YoriLibCabFdiFileOpen(FileName, YORI_LIB_CAB_OPEN_READONLY, 0);
    if (Handle == (DWORD_PTR)INVALID_HANDLE_VALUE) {
        return Handle;
    }

    GetFileInformationByHandle((HANDLE)Handle, &FileInfo);
    *Attributes = (WORD)(FileInfo.dwFileAttributes & 0xFFFF);
    FileTimeToDosDateTime(&FileInfo.ftLastWriteTime, Date, Time);

    return Handle;
}


/**
 A callback invoked during FDICopy to indicate events and state encountered
 while processing the CAB file.

 @param NotifyType The type of the notification.

 @param Notification Pointer to a block of memory defining additional state
        associated with the notification.  The meaning of the fields in
        this structure are NotificationType specific.  The original FDI.H
        header is the best documentation for the notifications and expected
        transformations that can occur in this routine (don't bother with
        MSDN.)
 
 @return Type specific result.  Typically zero means success, and -1 means
         error, although this is not always true.
 */
DWORD_PTR DIAMONDAPI
YoriLibCabNotify(
    __in CAB_CB_FDI_NOTIFYTYPE NotifyType,
    __in PCAB_CB_FDI_NOTIFICATION Notification
    )
{
    FILETIME TimeToSet;
    LARGE_INTEGER liTemp;
    TIME_ZONE_INFORMATION Tzi;
    PYORI_LIB_CAB_EXPAND_CONTEXT ExpandContext;
    YORI_STRING FullPath;
    YORI_STRING FileName;
    DWORD_PTR Handle;

    switch(NotifyType) {
        case YoriLibCabNotifyCopyFile:
            ExpandContext = (PYORI_LIB_CAB_EXPAND_CONTEXT)Notification->Context;
            if (!YoriLibCabBuildFileNames(ExpandContext->TargetDirectory, Notification->String1, &FullPath, &FileName)) {
                if (ExpandContext->ErrorString != NULL) {
                    YoriLibYPrintf(ExpandContext->ErrorString, _T("Could not build file name for directory %y CAB name %hs"), ExpandContext->TargetDirectory, Notification->String1);
                }
                return (DWORD_PTR)INVALID_HANDLE_VALUE;
            }
            if (YoriLibCabShouldIncludeFile(&FileName, ExpandContext)) {
                if (ExpandContext->CommenceExtractCallback == NULL ||
                    ExpandContext->CommenceExtractCallback(&FullPath, &FileName, ExpandContext->UserContext)) {

                    Handle = YoriLibCabFileOpenForExtract(&FullPath, ExpandContext->ErrorString);
                } else {
                    Handle = 0;
                }
            } else {
                Handle = 0;
            }
            YoriLibFreeStringContents(&FullPath);
            YoriLibFreeStringContents(&FileName);
            return Handle;
        case YoriLibCabNotifyCloseFile:
            GetTimeZoneInformation(&Tzi);

            //
            //  Convert the DOS time into a local time zone relative NT time
            //

            DosDateTimeToFileTime(Notification->TinyDate, Notification->TinyTime, &TimeToSet);

            //
            //  Apply the time zone bias adjustment to the NT time
            //

            liTemp.LowPart = TimeToSet.dwLowDateTime;
            liTemp.HighPart = TimeToSet.dwHighDateTime;
            liTemp.QuadPart = liTemp.QuadPart + ((DWORDLONG)Tzi.Bias) * 10 * 1000 * 1000 * 60;
            TimeToSet.dwLowDateTime = liTemp.LowPart;
            TimeToSet.dwHighDateTime = liTemp.HighPart;

            //
            //  Set the time on the file
            //

            SetFileTime((HANDLE)Notification->FileHandle, &TimeToSet, &TimeToSet, &TimeToSet);
            YoriLibCabFdiFileClose(Notification->FileHandle);

            ExpandContext = (PYORI_LIB_CAB_EXPAND_CONTEXT)Notification->Context;
            if (YoriLibCabBuildFileNames(ExpandContext->TargetDirectory, Notification->String1, &FullPath, &FileName)) {
                SetFileAttributes(FullPath.StartOfString, Notification->HalfAttributes);

                if (ExpandContext->CompleteExtractCallback != NULL) {
                    ExpandContext->CompleteExtractCallback(&FullPath, &FileName, ExpandContext->UserContext);
                }
                YoriLibFreeStringContents(&FullPath);
                YoriLibFreeStringContents(&FileName);
            }
            return 1;
        case YoriLibCabNotifyNextCabinet:
            if (Notification->FdiError != 0) {
                return (DWORD_PTR)-1;
            }
    }
    return 0;
}

/**
 Extract a cabinet file into a specified directory.

 @param CabFileName Pointer to the file name of the Cabinet to extract.

 @param TargetDirectory Pointer to the name of the directory to extract
        into.

 @param IncludeAllByDefault If TRUE, files not listed in the below arrays
        are expanded.  If FALSE, only files explicitly listed are expanded.

 @param NumberFilesToInclude The number of files in the FilesToInclude array.

 @param FilesToInclude An array of strings corresponding to files that should
        be expanded.

 @param NumberFilesToExclude The number of files in the FilesToExclude array.

 @param FilesToExclude An array of strings corresponding to files that should
        not be expanded.

 @param CommenceExtractCallback Optionally points to a a function to invoke
        for each file processed as part of extracting the CAB.  This function
        is invoked before extract and gives the user a chance to skip
        particular files.

 @param CompleteExtractCallback Optionally points to a a function to invoke
        for each file processed as part of extracting the CAB.  This function
        is invoked after extract and gives the user a chance to make extra
        changes to files.

 @param UserContext Optionally points to context to pass to
        CommenceExtractCallback and CompleteExtractCallback.

 @param ErrorString Optionally points to a string to populate with information
        about any error encountered in the extraction process.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibExtractCab(
    __in PYORI_STRING CabFileName,
    __in PYORI_STRING TargetDirectory,
    __in BOOL IncludeAllByDefault,
    __in DWORD NumberFilesToExclude,
    __in_opt PYORI_STRING FilesToExclude,
    __in DWORD NumberFilesToInclude,
    __in_opt PYORI_STRING FilesToInclude,
    __in_opt PYORI_LIB_CAB_EXPAND_FILE_CALLBACK CommenceExtractCallback,
    __in_opt PYORI_LIB_CAB_EXPAND_FILE_CALLBACK CompleteExtractCallback,
    __in_opt PVOID UserContext,
    __out_opt PYORI_STRING ErrorString
    )
{
    YORI_STRING FullCabFileName;
    YORI_STRING CabParentDirectory;
    YORI_STRING CabFileNameOnly;
    YORI_STRING FullTargetDirectory;
    LPTSTR FinalBackslash;
    LPVOID hFdi;
    CAB_CB_ERROR CabErrors;
    LPSTR AnsiCabFileName;
    LPSTR AnsiCabParentDirectory;
    BOOL DefaultUsed = FALSE;
    BOOL Result = FALSE;
    YORI_LIB_CAB_EXPAND_CONTEXT ExpandContext;

    YoriLibLoadCabinetFunctions();
    if (DllCabinet.pFdiCreate == NULL ||
        DllCabinet.pFdiCopy == NULL) {

        if (ErrorString != NULL) {
            YoriLibYPrintf(ErrorString, _T("Cabinet.dll not loaded or expected functions not found"));
        }
        return FALSE;
    }

    YoriLibInitEmptyString(&FullCabFileName);
    YoriLibInitEmptyString(&FullTargetDirectory);
    AnsiCabParentDirectory = NULL;
    hFdi = NULL;
    ZeroMemory(&ExpandContext, sizeof(ExpandContext));
    ExpandContext.DefaultInclude = IncludeAllByDefault;
    ExpandContext.NumberFilesToInclude = NumberFilesToInclude;
    ExpandContext.NumberFilesToExclude = NumberFilesToExclude;
    ExpandContext.FilesToInclude = FilesToInclude;
    ExpandContext.FilesToExclude = FilesToExclude;
    ExpandContext.CommenceExtractCallback = CommenceExtractCallback;
    ExpandContext.CompleteExtractCallback = CompleteExtractCallback;
    ExpandContext.UserContext = UserContext;
    ExpandContext.ErrorString = ErrorString;

    if (!YoriLibUserStringToSingleFilePath(CabFileName, FALSE, &FullCabFileName)) {
        if (ErrorString != NULL) {
            YoriLibYPrintf(ErrorString, _T("Cannot convert %y to full path"), CabFileName);
        }
        return FALSE;
    }

    if (!YoriLibUserStringToSingleFilePath(TargetDirectory, FALSE, &FullTargetDirectory)) {
        if (ErrorString != NULL) {
            YoriLibYPrintf(ErrorString, _T("Cannot convert %y to full path"), TargetDirectory);
        }
        return FALSE;
    }

    //
    //  A full path should have a backslash somewhere
    //

    FinalBackslash = YoriLibFindRightMostCharacter(&FullCabFileName, '\\');
    if (FinalBackslash == NULL) {
        if (ErrorString != NULL) {
            YoriLibYPrintf(ErrorString, _T("Cannot find final seperator in %y"), &FullCabFileName);
        }
        goto Exit;
    }

    YoriLibInitEmptyString(&CabParentDirectory);
    YoriLibInitEmptyString(&CabFileNameOnly);

    CabParentDirectory.StartOfString = FullCabFileName.StartOfString;
    CabParentDirectory.LengthInChars = (DWORD)(FinalBackslash - FullCabFileName.StartOfString + 1);

    CabFileNameOnly.StartOfString = &FullCabFileName.StartOfString[CabParentDirectory.LengthInChars];
    CabFileNameOnly.LengthInChars = FullCabFileName.LengthInChars - CabParentDirectory.LengthInChars;

    AnsiCabParentDirectory = YoriLibMalloc(CabParentDirectory.LengthInChars + 1 + CabFileNameOnly.LengthInChars + 1);
    if (AnsiCabParentDirectory == NULL) {
        if (ErrorString != NULL) {
            YoriLibYPrintf(ErrorString, _T("Allocation failure"));
        }
        goto Exit;
    }

    AnsiCabFileName = AnsiCabParentDirectory + CabParentDirectory.LengthInChars + 1;

    if (WideCharToMultiByte(CP_ACP, 0, CabParentDirectory.StartOfString, CabParentDirectory.LengthInChars, AnsiCabParentDirectory, CabParentDirectory.LengthInChars + 1, NULL, &DefaultUsed) != (INT)(CabParentDirectory.LengthInChars)) {
        if (ErrorString != NULL) {
            YoriLibYPrintf(ErrorString, _T("Error converting %y to ANSI"), &CabParentDirectory);
        }
        goto Exit;
    }

    if (DefaultUsed) {
        if (ErrorString != NULL) {
            YoriLibYPrintf(ErrorString, _T("Error converting %y to ANSI"), &CabParentDirectory);
        }
        goto Exit;
    }

    AnsiCabParentDirectory[CabParentDirectory.LengthInChars] = '\0';

    if (WideCharToMultiByte(CP_ACP, 0, CabFileNameOnly.StartOfString, CabFileNameOnly.LengthInChars, AnsiCabFileName, CabFileNameOnly.LengthInChars + 1, NULL, &DefaultUsed) != (INT)(CabFileNameOnly.LengthInChars)) {
        if (ErrorString != NULL) {
            YoriLibYPrintf(ErrorString, _T("Error converting %y to ANSI"), &CabFileNameOnly);
        }
        goto Exit;
    }

    if (DefaultUsed) {
        if (ErrorString != NULL) {
            YoriLibYPrintf(ErrorString, _T("Error converting %y to ANSI"), &CabFileNameOnly);
        }
        goto Exit;
    }

    AnsiCabFileName[CabFileNameOnly.LengthInChars] = '\0';

    hFdi = DllCabinet.pFdiCreate(YoriLibCabAlloc, YoriLibCabFree, YoriLibCabFdiFileOpen, YoriLibCabFdiFileRead, YoriLibCabFdiFileWrite, YoriLibCabFdiFileClose, YoriLibCabFdiFileSeek, -1, &CabErrors);

    if (hFdi == NULL) {
        if (ErrorString != NULL && ErrorString->LengthInChars == 0) {
            YoriLibYPrintf(ErrorString, _T("Error %i in pFdiCreate"), GetLastError());
        }
        goto Exit;
    }

    ExpandContext.TargetDirectory = &FullTargetDirectory;

    if (!DllCabinet.pFdiCopy(hFdi,
                             AnsiCabFileName,
                             AnsiCabParentDirectory,
                             0,
                             YoriLibCabNotify,
                             NULL,
                             &ExpandContext)) {
        if (ErrorString != NULL && ErrorString->LengthInChars == 0) {
            YoriLibYPrintf(ErrorString, _T("Error %i in pFdiCopy"), GetLastError());
        }
        goto Exit;
    }

    Result = TRUE;
Exit:

    if (DllCabinet.pFdiDestroy != NULL && hFdi != NULL) {
        DllCabinet.pFdiDestroy(hFdi);
    }

    YoriLibFreeStringContents(&FullCabFileName);
    YoriLibFreeStringContents(&FullTargetDirectory);
    if (AnsiCabParentDirectory != NULL) {
        YoriLibFree(AnsiCabParentDirectory);
    }
    return Result;
}

/**
 A structure owned by this module for each CAB file being created.  This is
 the nonopaque form of a handle returned from @ref YoriLibCreateCab .
 */
typedef struct _YORI_CAB_HANDLE {

    /**
     A description of the CAB file being constructed.
     */
    CAB_FCI_CONTEXT CompressContext;

    /**
     Memory allocated to retrieve errors that occur during compression.
     */
    CAB_CB_ERROR Err;

    /**
     A handle returned from FCICreate that is used by the cabinet API for
     future operations on the CAB.
     */
    PVOID FciHandle;
} YORI_CAB_HANDLE, *PYORI_CAB_HANDLE;

/**
 Create a new CAB file.  Files can be added to it with
 @ref YoriLibAddFileToCab .

 @param CabFileName The file name of the CAB to create on disk.  Note that
        due to limitations of the Cabinet API, this must be capable of being
        converted to ANSI losslessly.

 @param Handle On successful completion, this is updated to contain an opaque
        handle that can be used in @ref YoriLibAddFileToCab or
        @ref YoriLibCloseCab .

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCreateCab(
    __in PYORI_STRING CabFileName,
    __out PVOID * Handle
    )
{
    PYORI_CAB_HANDLE CabHandle;
    BOOL DefaultUsed = FALSE;

    YoriLibLoadCabinetFunctions();
    if (DllCabinet.pFciCreate == NULL ||
        DllCabinet.pFciAddFile == NULL) {

        return FALSE;
    }

    CabHandle = YoriLibReferencedMalloc(sizeof(YORI_CAB_HANDLE));
    if (CabHandle == NULL) {
        return FALSE;
    }

    ZeroMemory(CabHandle, sizeof(YORI_CAB_HANDLE));

    //
    //  We don't want to split data across multiple CABs.  This feature
    //  was for floppy disks.  Today, set the maximum size to as large
    //  as is possible.
    //

    CabHandle->CompressContext.SizeAvailable = 0x7FFFF000;
    CabHandle->CompressContext.ThresholdForNextFolder = 0x7FFFF000;

    if (WideCharToMultiByte(CP_ACP, 0, CabFileName->StartOfString, CabFileName->LengthInChars, CabHandle->CompressContext.CabPath, sizeof(CabHandle->CompressContext.CabPath), NULL, &DefaultUsed) != (INT)(CabFileName->LengthInChars)) {
        YoriLibDereference(CabHandle);
        return FALSE;
    }

    if (DefaultUsed) {
        YoriLibDereference(CabHandle);
        return FALSE;
    }

    CabHandle->FciHandle = DllCabinet.pFciCreate(&CabHandle->Err, YoriLibCabFciFilePlaced, YoriLibCabAlloc, YoriLibCabFree, YoriLibCabFciFileOpen, YoriLibCabFciFileRead, YoriLibCabFciFileWrite, YoriLibCabFciFileClose, YoriLibCabFciFileSeek, YoriLibCabFciFileDelete, YoriLibCabFciGetTempFile, &CabHandle->CompressContext, NULL);

    if (CabHandle->FciHandle == NULL) {
        YoriLibDereference(CabHandle);
        return FALSE;
    }

    *Handle = CabHandle;
    return TRUE;
}

/**
 Add a file to a CAB file.

 @param Handle The opaque handle returned from @ref YoriLibCreateCab.

 @param FileNameOnDisk The path to the file on disk.  Note that due to
        limitations of the Cabinet API, this must be capable of being
        converted to ANSI losslessly.

 @param FileNameInCab The path to record the file as within the CAB.
        Note that due to limitations of the Cabinet API, this must be
        capable of being converted to ANSI losslessly.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibAddFileToCab(
    __in PVOID Handle,
    __in PYORI_STRING FileNameOnDisk,
    __in PYORI_STRING FileNameInCab
    )
{
    PYORI_CAB_HANDLE CabHandle = (PYORI_CAB_HANDLE)Handle;

    LPSTR FileNameOnDiskAnsi;
    LPSTR FileNameInCabAnsi;
    BOOL DefaultUsed;
    BOOL Result;

    FileNameOnDiskAnsi = YoriLibMalloc(FileNameOnDisk->LengthInChars + 1);
    if (FileNameOnDiskAnsi == NULL) {
        return FALSE;
    }

    if (WideCharToMultiByte(CP_ACP, 0, FileNameOnDisk->StartOfString, FileNameOnDisk->LengthInChars, FileNameOnDiskAnsi, FileNameOnDisk->LengthInChars, NULL, &DefaultUsed) != (INT)(FileNameOnDisk->LengthInChars)) {
        YoriLibFree(FileNameOnDiskAnsi);
        return FALSE;
    }

    if (DefaultUsed) {
        YoriLibFree(FileNameOnDiskAnsi);
        return FALSE;
    }

    FileNameOnDiskAnsi[FileNameOnDisk->LengthInChars] = '\0';

    FileNameInCabAnsi = YoriLibMalloc(FileNameInCab->LengthInChars + 1);
    if (FileNameInCabAnsi == NULL) {
        YoriLibFree(FileNameOnDiskAnsi);
        return FALSE;
    }

    if (WideCharToMultiByte(CP_ACP, 0, FileNameInCab->StartOfString, FileNameInCab->LengthInChars, FileNameInCabAnsi, FileNameInCab->LengthInChars, NULL, &DefaultUsed) != (INT)(FileNameInCab->LengthInChars)) {
        YoriLibFree(FileNameOnDiskAnsi);
        YoriLibFree(FileNameInCabAnsi);
        return FALSE;
    }

    if (DefaultUsed) {
        YoriLibFree(FileNameOnDiskAnsi);
        YoriLibFree(FileNameInCabAnsi);
        return FALSE;
    }

    FileNameInCabAnsi[FileNameInCab->LengthInChars] = '\0';

    Result = DllCabinet.pFciAddFile(CabHandle->FciHandle, FileNameOnDiskAnsi, FileNameInCabAnsi, FALSE, YoriLibCabFciGetNextCabinet, YoriLibCabFciStatus, YoriLibCabFciGetOpenInfo, CAB_FCI_ALGORITHM_MSZIP);

    YoriLibFree(FileNameOnDiskAnsi);
    YoriLibFree(FileNameInCabAnsi);
    return Result;
}

/**
 Complete the creation of a new CAB file.

 @param Handle The opaque handle returned from @ref YoriLibCreateCab.
 */
VOID
YoriLibCloseCab(
    __in PVOID Handle
    )
{
    PYORI_CAB_HANDLE CabHandle = (PYORI_CAB_HANDLE)Handle;

    if (DllCabinet.pFciFlushFolder) {
        DllCabinet.pFciFlushFolder(CabHandle->FciHandle, YoriLibCabFciGetNextCabinet, YoriLibCabFciStatus);
    }
    if (DllCabinet.pFciFlushCabinet) {
        DllCabinet.pFciFlushCabinet(CabHandle->FciHandle, FALSE, YoriLibCabFciGetNextCabinet, YoriLibCabFciStatus);
    }
    if (DllCabinet.pFciDestroy) {
        DllCabinet.pFciDestroy(CabHandle->FciHandle);
    }

    YoriLibDereference(CabHandle);
}


// vim:sw=4:ts=4:et:
