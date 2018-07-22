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
    PYORI_LIB_CAB_EXPAND_FILE_CALLBACK UserCallback;

    /**
     Context information to pass to the user specified callback.
     */
    PVOID UserContext;
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
YoriLibCabFileOpen(
    __in LPSTR FileName,
    __in INT OFlag,
    __in INT PMode
    )
{
    HANDLE hFile;
    DWORD DesiredAccess;
    DWORD ShareMode;
    DWORD Disposition;
    DWORD OpenType = ((DWORD)OFlag & YORI_LIB_CAB_OPEN_MASK);

    UNREFERENCED_PARAMETER(PMode);

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

 @return Handle to the opened file, or INVALID_HANDLE_VALUE on failure.
 */
DWORD_PTR
YoriLibCabFileOpenForExtract(
    __in PYORI_STRING FullPath
    )
{
    HANDLE hFile;
    DWORD Err;

    while (TRUE) {

        //
        //  Try to open the target file
        //

        hFile = CreateFile(FullPath->StartOfString,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL,
                           CREATE_NEW,
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

            LastSep = YoriLibFindRightMostCharacter(FullPath, '\\');
            if (LastSep == NULL) {
                break;
            }

            *LastSep = '\0';

            if (!CreateDirectory(FullPath->StartOfString, NULL)) {
                break;
            }

            *LastSep = '\\';

            hFile = CreateFile(FullPath->StartOfString,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL,
                               CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

            break;

        //
        //  If the file is already there, try to rename it out of the way.
        //

        } else if (Err == ERROR_FILE_EXISTS) {

            YORI_STRING NewName;

            if (!YoriLibAllocateString(&NewName, FullPath->LengthInChars + sizeof(".old"))) {
                break;
            }

            NewName.LengthInChars = YoriLibSPrintf(NewName.StartOfString, _T("%y.old"), FullPath);
            if (!MoveFileEx(FullPath->StartOfString, NewName.StartOfString, MOVEFILE_REPLACE_EXISTING)) {
                YoriLibFreeStringContents(&NewName);
                break;
            }

            hFile = CreateFile(FullPath->StartOfString,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL,
                               CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

            if (hFile == INVALID_HANDLE_VALUE) {
                MoveFileEx(NewName.StartOfString, FullPath->StartOfString, MOVEFILE_REPLACE_EXISTING);
            } else {
                HANDLE hDeadFile;

                //
                //  Try to delete the old file via DeleteFile and
                //  FILE_FLAG_DELETE_ON_CLOSE, and hope one of them
                //  works
                //

                DeleteFile(NewName.StartOfString);
                hDeadFile = CreateFile(NewName.StartOfString,
                                       DELETE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_FLAG_DELETE_ON_CLOSE,
                                       NULL);

                if (hDeadFile != INVALID_HANDLE_VALUE) {
                    CloseHandle(hDeadFile);
                }
            }
            YoriLibFreeStringContents(&NewName);
            break;
        }
    }

    return (DWORD_PTR)hFile;
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
YoriLibCabFileRead(
    __in DWORD_PTR FileHandle,
    __out PVOID Buffer,
    __in DWORD ByteCount
    )
{
    DWORD BytesRead;
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
 A callback invoked during FDICopy to write to file.  Note that these
 callbacks always refer to the "current file position".
 
 @param FileHandle The file to write to.
 
 @param Buffer Pointer to a block of memory containing data to write.

 @param ByteCount The number of bytes to write.

 @return The number of bytes actually written or -1 on failure.
 */
DWORD DIAMONDAPI
YoriLibCabFileWrite(
    __in DWORD_PTR FileHandle,
    __in PVOID Buffer,
    __in DWORD ByteCount
    )
{
    DWORD BytesWritten;
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
 A callback invoked during FDICopy to close a file.
 
 @param FileHandle The file to close.

 @return Zero for success, nonzero to indicate an error.
 */
INT DIAMONDAPI
YoriLibCabFileClose(
    __in DWORD_PTR FileHandle
    )
{
    CloseHandle((HANDLE)FileHandle);
    return 0;
}


/**
 A callback invoked during FDICopy to change current file position.
 
 @param FileHandle The file to set current position on.

 @param DistanceToMove The number of bytes to move.

 @param SeekType The origin of the move.

 @return The new file position.
 */
DWORD DIAMONDAPI
YoriLibCabFileSeek(
    __in DWORD_PTR FileHandle,
    __in DWORD DistanceToMove,
    __in INT SeekType
    )
{
    DWORD NewPosition;

    NewPosition = SetFilePointer((HANDLE)FileHandle,
                                 DistanceToMove,
                                 NULL,
                                 SeekType);

    return NewPosition;
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
    __in CAB_CB_NOTIFYTYPE NotifyType,
    __in PCAB_CB_NOTIFICATION Notification
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
                return (DWORD_PTR)INVALID_HANDLE_VALUE;
            }
            if (YoriLibCabShouldIncludeFile(&FileName, ExpandContext)) {
                if (ExpandContext->UserCallback == NULL ||
                    ExpandContext->UserCallback(&FullPath, &FileName, ExpandContext->UserContext)) {

                    Handle = YoriLibCabFileOpenForExtract(&FullPath);
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

            //
            //  MSFIX Need to apply DOS attributes
            //

            YoriLibCabFileClose(Notification->FileHandle);
            return 1;
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

 @param UserCallback Optionally points to a a function to invoke for each file
        processed as part of extracting the CAB.

 @param UserContext Optionally points to context to pass to UserCallback.

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
    __in_opt PYORI_LIB_CAB_EXPAND_FILE_CALLBACK UserCallback,
    __in_opt PVOID UserContext
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
    ExpandContext.UserCallback = UserCallback;
    ExpandContext.UserContext = UserContext;

    if (!YoriLibUserStringToSingleFilePath(CabFileName, FALSE, &FullCabFileName)) {
        return FALSE;
    }

    if (!YoriLibUserStringToSingleFilePath(TargetDirectory, FALSE, &FullTargetDirectory)) {
        return FALSE;
    }

    //
    //  A full path should have a backslash somewhere
    //

    FinalBackslash = YoriLibFindRightMostCharacter(&FullCabFileName, '\\');
    if (FinalBackslash == NULL) {
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
        goto Exit;
    }

    AnsiCabFileName = AnsiCabParentDirectory + CabParentDirectory.LengthInChars + 1;

    if (WideCharToMultiByte(CP_ACP, 0, CabParentDirectory.StartOfString, CabParentDirectory.LengthInChars, AnsiCabParentDirectory, CabParentDirectory.LengthInChars + 1, NULL, &DefaultUsed) != (INT)(CabParentDirectory.LengthInChars)) {
        goto Exit;
    }

    if (DefaultUsed) {
        goto Exit;
    }

    AnsiCabParentDirectory[CabParentDirectory.LengthInChars] = '\0';

    if (WideCharToMultiByte(CP_ACP, 0, CabFileNameOnly.StartOfString, CabFileNameOnly.LengthInChars, AnsiCabFileName, CabFileNameOnly.LengthInChars + 1, NULL, &DefaultUsed) != (INT)(CabFileNameOnly.LengthInChars)) {
        goto Exit;
    }

    if (DefaultUsed) {
        goto Exit;
    }

    AnsiCabFileName[CabFileNameOnly.LengthInChars] = '\0';

    hFdi = DllCabinet.pFdiCreate(YoriLibCabAlloc, YoriLibCabFree, YoriLibCabFileOpen, YoriLibCabFileRead, YoriLibCabFileWrite, YoriLibCabFileClose, YoriLibCabFileSeek, -1, &CabErrors);

    if (hFdi == NULL) {
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


// vim:sw=4:ts=4:et:
