/**
 * @file /lib/util.c
 *
 * Yori trivial utility routines
 *
 * Copyright (c) 2017 Malcolm J. Smith
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
 Returns TRUE if the character should be treated as an escape character.

 @param Char The character to check.

 @return TRUE if the character is an escape character, FALSE if it is not.
 */
BOOL
YoriLibIsEscapeChar(
    __in TCHAR Char
    )
{
    if (Char == '^') {
        return TRUE;
    }
    return FALSE;
}

/**
 Convert a noninheritable handle into an inheritable handle.

 @param OriginalHandle A handle to convert, which is presumably not
        inheritable.  On success, this handle is closed.

 @param InheritableHandle On successful completion, populated with a new
        handle which is inheritable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibMakeInheritableHandle(
    __in HANDLE OriginalHandle,
    __out PHANDLE InheritableHandle
    )
{
    HANDLE NewHandle;

    if (DuplicateHandle(GetCurrentProcess(),
                        OriginalHandle,
                        GetCurrentProcess(),
                        &NewHandle,
                        0,
                        TRUE,
                        DUPLICATE_SAME_ACCESS)) {

        CloseHandle(OriginalHandle);
        *InheritableHandle = NewHandle;
        return TRUE;
    }

    return FALSE;
}

/**
 A constant string to return if detailed error text could not be returned.
 */
CONST LPTSTR NoWinErrorText = _T("Could not fetch error text.");

/**
 Lookup a Win32 error code and return a pointer to the error string.
 If the string could not be located, returns NULL.  The returned string
 should be freed with @ref YoriLibFreeWinErrorText.

 @param ErrorCode The error code to fetch a string for.

 @return Pointer to the error string on success, NULL on failure.
 */
LPTSTR
YoriLibGetWinErrorText(
    __in DWORD ErrorCode
    )
{
    LPTSTR OutputBuffer = NULL;
    DWORD CharsReturned;

    CharsReturned = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL,
                                  ErrorCode,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  (LPTSTR)&OutputBuffer,
                                  0,
                                  NULL);

    if (CharsReturned == 0) {
        OutputBuffer = NULL;
    }

    if (OutputBuffer == NULL) {
        OutputBuffer = NoWinErrorText;
    }

    return OutputBuffer;
}

/**
 Lookup an NT status code and return a pointer to the error string.
 If the string could not be located, returns NULL.  The returned string
 should be freed with @ref YoriLibFreeWinErrorText.

 @param ErrorCode The error code to fetch a string for.

 @return Pointer to the error string on success, NULL on failure.
 */
LPTSTR
YoriLibGetNtErrorText(
    __in DWORD ErrorCode
    )
{
    LPTSTR OutputBuffer = NULL;
    HANDLE NtdllHandle;
    DWORD CharsReturned;

    NtdllHandle = GetModuleHandle(_T("NTDLL"));

    CharsReturned = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                                  NtdllHandle,
                                  ErrorCode,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  (LPTSTR)&OutputBuffer,
                                  0,
                                  NULL);

    if (CharsReturned == 0) {
        OutputBuffer = NULL;
    }

    if (OutputBuffer == NULL) {
        OutputBuffer = NoWinErrorText;
    }

    return OutputBuffer;
}

/**
 Free an error string previously allocated with @ref YoriLibGetWinErrorText .

 @param ErrText Pointer to the error text string.
 */
VOID
YoriLibFreeWinErrorText(
    __in LPTSTR ErrText
    )
{
    if (ErrText != NULL && ErrText != NoWinErrorText) {
        LocalFree(ErrText);
    }
}

/**
 Create a directory, and any parent directories that do not yet exist.  Note
 the input buffer is modified within this routine.  On success it will be
 restored to its original contents, on failure it will indicate the name of
 the path which could not be created.

 @param DirName The directory to create.  On failure this will contain the
        name of the directory that could not be created.

 @return TRUE to indicate success, FALSE to indicate failure.  On failure,
         Win32 LastError will indicate the reason.
 */
__success(return)
BOOL
YoriLibCreateDirectoryAndParents(
    __inout PYORI_STRING DirName
    )
{
    DWORD MaxIndex = DirName->LengthInChars - 1;
    DWORD Err;
    DWORD SepIndex = MaxIndex;
    BOOL Result;
    BOOL StartedSucceeding = FALSE;

    while (TRUE) {
        Result = CreateDirectory(DirName->StartOfString, NULL);
        if (!Result) {
            Err = GetLastError();
            if (Err == ERROR_ALREADY_EXISTS) {
                DWORD Attributes = GetFileAttributes(DirName->StartOfString);
                if ((Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                    Err = ERROR_ACCESS_DENIED;
                    SetLastError(Err);
                } else {
                    Err = ERROR_SUCCESS;
                }
            }
        } else {
            Err = ERROR_SUCCESS;
        }

        if (Err == ERROR_PATH_NOT_FOUND && !StartedSucceeding) {

            //
            //  MSFIX Check for truncation beyond \\?\ or \\?\UNC\ ?
            //

            for (;!YoriLibIsSep(DirName->StartOfString[SepIndex]) && SepIndex > 0; SepIndex--) {
            }

            if (!YoriLibIsSep(DirName->StartOfString[SepIndex])) {
                return FALSE;
            }

            DirName->StartOfString[SepIndex] = '\0';
            DirName->LengthInChars = SepIndex;
            continue;

        } else if (Err != ERROR_SUCCESS) {
            return FALSE;
        } else {
            StartedSucceeding = TRUE;
            if (SepIndex < MaxIndex) {
                ASSERT(DirName->StartOfString[SepIndex] == '\0');

                DirName->StartOfString[SepIndex] = '\\';
                for (;DirName->StartOfString[SepIndex] != '\0' && SepIndex <= MaxIndex; SepIndex++);
                DirName->LengthInChars = SepIndex;
                continue;
            } else {
                DirName->LengthInChars = MaxIndex + 1;
                return TRUE;
            }
        }
    }
}

/**
 Rename a file from its current full name to a backup name, and return which
 backup name was used.  This routine cycles through appending .old, then
 .old.1 up to .old.9.  If any existing file exists with these names, it is
 overwritten, unless it is in use, in which case the next name is used.
 The reason for overwriting a previous name is the assumption that it was
 generated by a previous attempt to use this code, when the file was in use,
 and that it is no longer in use and the old file should be discarded.  Note
 in particular that the same operation will not use the same backup name
 multiple times, because doing so implies it is operating on the same source
 file multiple times in the same operation, which is invalid.

 @param FullPath Pointer to the name specifying the original file that should
        be renamed to a backup name.

 @param NewName On successful completion, updated to point to a new allocation
        that describes the new backup file name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibRenameFileToBackupName(
    __in PYORI_STRING FullPath,
    __out PYORI_STRING NewName
    )
{
    DWORD ProbeIndex;
    HANDLE hDeadFile;
    YORI_STRING ShortFullPath;
    BOOLEAN ShortMode;
    DWORD Index;
    DWORD EndIndex;
    DWORD Err;

    ShortMode = FALSE;
    YoriLibInitEmptyString(&ShortFullPath);
    ShortFullPath.StartOfString = FullPath->StartOfString;
    ShortFullPath.LengthInChars = FullPath->LengthInChars;

    //
    //  Find the short name by truncating to any period until a seperator
    //  is reached, and once the seperator is reached, truncate to 8 chars.
    //  This is used if the file system can't handle long file names.

    EndIndex = ShortFullPath.LengthInChars;
    for (Index = ShortFullPath.LengthInChars; Index > 0; Index--) {
        if (ShortFullPath.StartOfString[Index - 1] == '.') {
            EndIndex = Index - 1;
        } else if (YoriLibIsSep(ShortFullPath.StartOfString[Index - 1])) {
            if (Index - 1 + 8 < EndIndex) {
                EndIndex = Index - 1 + 8;
            }
            break;
        }
    }
    ShortFullPath.LengthInChars = EndIndex;

    if (!YoriLibAllocateString(NewName, FullPath->LengthInChars + sizeof(".old.9"))) {
        return FALSE;
    }

    for (ProbeIndex = 0; ProbeIndex < 10; ProbeIndex++) {

        if (ShortMode) {
            if (ProbeIndex == 0) {
                NewName->LengthInChars = YoriLibSPrintf(NewName->StartOfString, _T("%y.old"), &ShortFullPath);
            } else {
                NewName->LengthInChars = YoriLibSPrintf(NewName->StartOfString, _T("%y.ol%i"), &ShortFullPath, ProbeIndex);
            }
        } else {
            if (ProbeIndex == 0) {
                NewName->LengthInChars = YoriLibSPrintf(NewName->StartOfString, _T("%y.old"), FullPath);
            } else {
                NewName->LengthInChars = YoriLibSPrintf(NewName->StartOfString, _T("%y.old.%i"), FullPath, ProbeIndex);
            }
        }

        //
        //  Try to delete the old file via DeleteFile and
        //  FILE_FLAG_DELETE_ON_CLOSE, then do a superseding
        //  rename, and hope one of them works
        //

        DeleteFile(NewName->StartOfString);
        hDeadFile = CreateFile(NewName->StartOfString,
                               DELETE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_BACKUP_SEMANTICS,
                               NULL);

        //
        //  If it failed with ERROR_INVALID_NAME, use 8.3 compliant temporary
        //  file names and see if that works better
        //

        if (hDeadFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hDeadFile);
        } else {
            Err = GetLastError();
            if (Err == ERROR_INVALID_NAME && !ShortMode) {
                ShortMode = TRUE;
                ProbeIndex--;
                continue;
            }
        }

        if (MoveFileEx(FullPath->StartOfString, NewName->StartOfString, MOVEFILE_REPLACE_EXISTING)) {
            break;
        }
    }

    //
    //  If we couldn't find a suitable name in 10 attempts, stop.
    //

    if (ProbeIndex == 10) {
        YoriLibFreeStringContents(NewName);
        return FALSE;
    }

    return TRUE;
}


/**
 Returns TRUE if the specified path is an Internet path that requires wininet.
 Technically this can return FALSE for paths that are still remote (SMB paths),
 but those are functionally the same as local paths.

 @param PackagePath Pointer to the path to check.

 @return TRUE if the path is an Internet path, FALSE if it is a local path.
 */
BOOL
YoriLibIsPathUrl(
    __in PCYORI_STRING PackagePath
    )
{
    if (YoriLibCompareStringWithLiteralInsensitiveCount(PackagePath, _T("http://"), sizeof("http://") - 1) == 0 ||
        YoriLibCompareStringWithLiteralInsensitiveCount(PackagePath, _T("https://"), sizeof("https://") - 1) == 0 ||
        YoriLibCompareStringWithLiteralInsensitiveCount(PackagePath, _T("ftp://"), sizeof("ftp://") - 1) == 0) {

        return TRUE;
    }

    return FALSE;
}

/**
 Returns TRUE if the standard input handle is to a console.  Yori tools
 generally treat this as an error, indicating the user forgot to specify
 a file or establish a pipe, rather than leave the console waiting for
 typed input.

 @return TRUE to indicate standard input is a console; FALSE if it is
         from another source.
 */
BOOL
YoriLibIsStdInConsole(VOID)
{
    DWORD ConsoleMode;
    if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &ConsoleMode)) {
        return TRUE;
    }
    return FALSE;
}

/**
 Report the system time in NT units as an integer.  This is implemented as a
 helper because NT 3.1 doesn't have GetSystemTimeAsFileTime, and that
 function also doesn't return things in a LARGE_INTEGER so it needs to be
 manually manipulated to get a single number anyway.

 @return The system time as an integer.
 */
LONGLONG
YoriLibGetSystemTimeAsInteger(VOID)
{
    SYSTEMTIME CurrentSystemTime;
    FILETIME CurrentSystemTimeAsFileTime;
    LARGE_INTEGER ReturnValue;

    //
    //  This roundabout implementation occurs because NT 3.1 doesn't have
    //  GetSystemTimeAsFileTime.  As a performance optimization, this
    //  logic could use that API where it exists.  It is still frustrating
    //  to have to convert the result of that to an aligned integer, and
    //  bypassing Win32 completely could avoid that.
    //

    GetSystemTime(&CurrentSystemTime);

    // 
    //  This function can fail if it's given input that's not within a valid
    //  range.  That can't really happen here, but static analysis complains
    //  if the condition isn't checked.
    //

    if (!SystemTimeToFileTime(&CurrentSystemTime, &CurrentSystemTimeAsFileTime)) {
        CurrentSystemTimeAsFileTime.dwHighDateTime = 0;
        CurrentSystemTimeAsFileTime.dwLowDateTime = 0;
    }

    ReturnValue.HighPart = CurrentSystemTimeAsFileTime.dwHighDateTime;
    ReturnValue.LowPart = CurrentSystemTimeAsFileTime.dwLowDateTime;
    return ReturnValue.QuadPart;
}

/**
 Attempt to delete a file using POSIX semantics.

 @param FileName Pointer to the file name to delete.

 @return TRUE to indicate the file was successfully marked for delete,
         FALSE if not.
 */
BOOL
YoriLibPosixDeleteFile(
    __in PYORI_STRING FileName
    )
{
    HANDLE hFile;
    FILE_DISPOSITION_INFO_EX DispositionInfo;

    ASSERT(YoriLibIsStringNullTerminated(FileName));

    if (DllKernel32.pSetFileInformationByHandle == NULL) {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return FALSE;
    }

    hFile = CreateFile(FileName->StartOfString,
                       DELETE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DispositionInfo.Flags = FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS;
    if (!DllKernel32.pSetFileInformationByHandle(hFile, FileDispositionInfoEx, &DispositionInfo, sizeof(DispositionInfo))) {
        DWORD Err;
        Err = GetLastError();
        CloseHandle(hFile);
        SetLastError(Err);
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}

/**
 The 32 bit math assembly routines currently can't support a 64 bit
 denominator.  Wrapping them in this function ensures that they will not be
 invoked with a 64 bit denominator.  This function is used as a barrier so
 that multiple 32 bit divisions can be performed without the compiler
 upgrading them into fewer 64 bit divisions.
 
 @param Numerator The numerator of the division operation.
 
 @param Denominator The denominator of the division operation.

 @return The result of the division operation.
 */
DWORDLONG
YoriLibDivide32(
    __in DWORDLONG Numerator,
    __in DWORD Denominator
    )
{
    return Numerator/Denominator;
}

/**
 Return TRUE to indicate that a character can be printed as a character.
 Return FALSE to indicate that a character is a control character that will
 modify terminal behavior.

 @param Char The character to test.

 @return TRUE if the character can be printed as a character, FALSE if it can
         not.
 */
BOOLEAN
YoriLibIsCharPrintable(
    __in WCHAR Char
    )
{
    if (Char == 0x00 ||
        (Char >= 0x07 && Char <= 0x0F) ||
        Char == 0x1B ||
        (Char >= 0x7F && Char <= 0x9F)) {

        return FALSE;
    }

    return TRUE;
}

/**
 Query the sector size that applies to a given handle.  If zero is returned,
 the device does not impose sector alignment requirements.

 @param FileHandle The file handle to query.

 @return The logical sector size, or zero.
 */
DWORD
YoriLibGetHandleSectorSize(
    __in HANDLE FileHandle
    )
{
    DWORD SectorSize;
    DISK_GEOMETRY DiskGeometry;
    DWORD BytesCopied;

    SectorSize = 0;
    if (DeviceIoControl(FileHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &DiskGeometry, sizeof(DiskGeometry), &BytesCopied, NULL)) {
        SectorSize = DiskGeometry.BytesPerSector;
    }

    return SectorSize;
}

/**
 Query a file or device length.  These use different APIs so this function
 tries a few to see if any work.

 @param FileHandle A handle to a file or device.

 @param FileSizePtr On successful completion, updated to point to the file
        or device size in bytes.

 @return ERROR_SUCCESS to indicate success, or alternative Win32 code to
         indicate failure.
 */
__success(return == ERROR_SUCCESS)
DWORD
YoriLibGetFileOrDeviceSize(
    __in HANDLE FileHandle,
    __out PDWORDLONG FileSizePtr
    )
{
    LARGE_INTEGER FileSize;
    DISK_GEOMETRY DiskGeometry;
    DWORD BytesReturned;
    DWORD Err;

    Err = ERROR_SUCCESS;
    FileSize.LowPart = GetFileSize(FileHandle, &FileSize.HighPart);
    if (FileSize.LowPart == INVALID_FILE_SIZE) {
        Err = GetLastError();
    }

    if (Err == ERROR_SUCCESS) {
        *FileSizePtr = FileSize.QuadPart;
        return Err;
    }

    if (Err != ERROR_INVALID_PARAMETER) {
        return Err;
    }

    if (DeviceIoControl(FileHandle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &FileSize, sizeof(FileSize), &BytesReturned, NULL)) {
        *FileSizePtr = FileSize.QuadPart;
        return ERROR_SUCCESS;
    }

    if (DeviceIoControl(FileHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &DiskGeometry, sizeof(DiskGeometry), &BytesReturned, NULL)) {
        FileSize.QuadPart = DiskGeometry.Cylinders.QuadPart;
        FileSize.QuadPart = FileSize.QuadPart * DiskGeometry.TracksPerCylinder * DiskGeometry.SectorsPerTrack * DiskGeometry.BytesPerSector;
        *FileSizePtr = FileSize.QuadPart;
        return ERROR_SUCCESS;
    }

    return GetLastError();
}

/**
 Multiply a 32 bit number by a 32 bit number and divide by a 32 bit number.
 Since we have a 64 bit math library, this can be implemented fairly simply.
 Note that on x86 the assembly routines are limited to 32 bit denominators,
 which works fine for this.

 @param Number One of the numbers to multiply.

 @param Numerator The second number to multiply.

 @param Denominator The number to divide by.

 @return The result.
 */
INT
YoriLibMulDiv(
    __in INT Number,
    __in INT Numerator,
    __in INT Denominator
    )
{
    LONGLONG LongNumber;

    LongNumber = Number;
    LongNumber = LongNumber * Numerator;
    LongNumber = LongNumber / Denominator;

    return (INT)LongNumber;
}

/**
 Calculate the FAT timestamp values from a FILETIME structure.

 @param FileTime Pointer to the FILETIME structure.

 @param FatDate On successful completion, the FAT date value.

 @param FatTime On successful completion, the FAT time value.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibFileTimeToDosDateTime(
    __in CONST FILETIME *FileTime,
    __out LPWORD FatDate,
    __out LPWORD FatTime
    )
{
    SYSTEMTIME SystemTime;
    FILETIME LocalFileTime;
    WORD LocalFatDate;
    WORD LocalFatTime;

    FileTimeToLocalFileTime(FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SystemTime);

    if (SystemTime.wYear < 1980 || SystemTime.wYear > (1980 + 127)) {
        return FALSE;
    }

    LocalFatDate = (WORD)((SystemTime.wDay & 0x1F) | ((SystemTime.wMonth & 0xF) << 5) | (((SystemTime.wYear - 1980) & 0x7F) << 9));
    LocalFatTime = (WORD)(((SystemTime.wSecond >> 1) & 0x1F) | ((SystemTime.wMinute & 0x3F) << 5) | ((SystemTime.wHour & 0x1F) << 11));

    *FatDate = LocalFatDate;
    *FatTime = LocalFatTime;

    return TRUE;
}

/**
 Calculate the FILETIME structure from FAT timestamp values.

 @param FatDate The FAT date value.

 @param FatTime The FAT time value.

 @param FileTime On successful completion, updated to contain the FILETIME
        value.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibDosDateTimeToFileTime(
    __in WORD FatDate,
    __in WORD FatTime,
    __out LPFILETIME FileTime
    )
{
    SYSTEMTIME SystemTime;

    ZeroMemory(&SystemTime, sizeof(SystemTime));
    SystemTime.wSecond = (WORD)((FatTime & 0x1F) << 1);
    SystemTime.wMinute = (WORD)((FatTime >> 5) & 0x3F);
    SystemTime.wHour = (WORD)((FatTime >> 11) & 0x1F);
    SystemTime.wDay = (WORD)((FatDate & 0x1F));
    SystemTime.wMonth = (WORD)((FatDate >> 5) & 0xF);
    SystemTime.wYear = (WORD)(((FatDate >> 9) & 0x7F) + 1980);

    //
    //  For some strange reason, the OS version of this function doesn't do
    //  timezone adjustment, so this doesn't either
    //

    if (!SystemTimeToFileTime(&SystemTime, FileTime)) {
        return FALSE;
    }

    return TRUE;
}


/**
 The original ShellExecute predates NT and returns 32 error values, or a
 greater value to indicate success.  Translate these into their NT
 counterparts.

 @param hInst Instance returned from ShellExecute.

 @return Win32 error code, including ERROR_SUCCESS to indicate success.
 */
DWORD
YoriLibShellExecuteInstanceToError(
    __in HINSTANCE hInst
    )
{
    DWORD Err;

    Err = ERROR_SUCCESS;
    if (hInst < (HINSTANCE)(DWORD_PTR)32) {
        switch((DWORD_PTR)hInst) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_ACCESS_DENIED:
            case ERROR_BAD_FORMAT:
                Err = (DWORD)(DWORD_PTR)hInst;
                break;
            case SE_ERR_SHARE:
                Err = ERROR_SHARING_VIOLATION;
                break;
            default:
                Err = ERROR_TOO_MANY_OPEN_FILES;
                break;
        }
    }

    return Err;
}


// vim:sw=4:ts=4:et:
