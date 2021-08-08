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
    __in PYORI_STRING PackagePath
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

// vim:sw=4:ts=4:et:
