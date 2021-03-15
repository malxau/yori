/**
 * @file sh/history.c
 *
 * Yori shell command history
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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

#include "yori.h"

/**
 Number of elements in command history.
 */
DWORD YoriShCommandHistoryCount;

/**
 The maximum number of history items to record concurrently.
 */
DWORD YoriShCommandHistoryMax;

/**
 A lock around history updates.  History should really only be accessed
 by a single thread, but when the app is being closed it can be
 accessed by a background thread to write to disk.
 */
HANDLE YoriShHistoryLock;

/**
 Set to TRUE once the history module has been initialized.
 */
BOOL YoriShHistoryInitialized;

/**
 Add an entered command into the command history buffer.

 @param NewCmd Pointer to a Yori string corresponding to the new
        entry to add to history.

 @param IgnoreIfRepeat If TRUE, don't add a new line if the immediate
        previous line is identical.  Note it must be exactly identical,
        including case.  If FALSE, add the new entry regardless.
 
 @return TRUE to indicate an entry was successfully added, FALSE if it was
         not.
 */
__success(return)
BOOL
YoriShAddToHistory(
    __in PYORI_STRING NewCmd,
    __in BOOLEAN IgnoreIfRepeat
    )
{
    DWORD LengthToAllocate;
    PYORI_SH_HISTORY_ENTRY NewHistoryEntry;

    if (NewCmd->LengthInChars == 0) {
        return TRUE;
    }

    LengthToAllocate = sizeof(YORI_SH_HISTORY_ENTRY);

    if (WaitForSingleObject(YoriShHistoryLock, 0) == WAIT_OBJECT_0) {

        if (YoriShGlobal.CommandHistory.Next == NULL) {
            YoriLibInitializeListHead(&YoriShGlobal.CommandHistory);
        }

        if (IgnoreIfRepeat) {
            PYORI_LIST_ENTRY ExistingEntry;
            ExistingEntry = YoriLibGetPreviousListEntry(&YoriShGlobal.CommandHistory, NULL);
            if (ExistingEntry != NULL) {
                NewHistoryEntry = CONTAINING_RECORD(ExistingEntry, YORI_SH_HISTORY_ENTRY, ListEntry);
                if (YoriLibCompareString(&NewHistoryEntry->CmdLine, NewCmd) == 0) {
                    ReleaseMutex(YoriShHistoryLock);
                    return FALSE;
                }
            }
        }

        NewHistoryEntry = YoriLibMalloc(LengthToAllocate);
        if (NewHistoryEntry == NULL) {
            ReleaseMutex(YoriShHistoryLock);
            return FALSE;
        }

        YoriLibCloneString(&NewHistoryEntry->CmdLine, NewCmd);

        YoriLibAppendList(&YoriShGlobal.CommandHistory, &NewHistoryEntry->ListEntry);
        YoriShCommandHistoryCount++;
        while (YoriShCommandHistoryCount > YoriShCommandHistoryMax) {
            PYORI_LIST_ENTRY ListEntry;
            PYORI_SH_HISTORY_ENTRY OldHistoryEntry;

            ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.CommandHistory, NULL);
            OldHistoryEntry = CONTAINING_RECORD(ListEntry, YORI_SH_HISTORY_ENTRY, ListEntry);
            YoriLibRemoveListItem(ListEntry);
            YoriLibFreeStringContents(&OldHistoryEntry->CmdLine);
            YoriLibFree(OldHistoryEntry);
            YoriShCommandHistoryCount--;
        }
        ReleaseMutex(YoriShHistoryLock);
    }

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 26165) // Analyze thinks a lock might be leaked
                                 // if WaitForSingleObject acquired it but
                                 // returned a different result.  That can't
                                 // happen.
#endif
    return TRUE;
}

/**
 Remove a single command from the history buffer.

 @param HistoryEntry Pointer to the entry to remove.
 */
VOID
YoriShRemoveOneHistoryEntry(
    __in PYORI_SH_HISTORY_ENTRY HistoryEntry
    )
{
    if (WaitForSingleObject(YoriShHistoryLock, 0) == WAIT_OBJECT_0) {
        YoriLibRemoveListItem(&HistoryEntry->ListEntry);
        YoriLibFreeStringContents(&HistoryEntry->CmdLine);
        YoriLibFree(HistoryEntry);
        YoriShCommandHistoryCount--;
        ReleaseMutex(YoriShHistoryLock);
    }
}

/**
 Free all command history.
 */
VOID
YoriShClearAllHistory(VOID)
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORI_SH_HISTORY_ENTRY HistoryEntry;

    if (WaitForSingleObject(YoriShHistoryLock, 0) == WAIT_OBJECT_0) {
        ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.CommandHistory, NULL);
        while (ListEntry != NULL) {
            HistoryEntry = CONTAINING_RECORD(ListEntry, YORI_SH_HISTORY_ENTRY, ListEntry);
            ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.CommandHistory, ListEntry);
            YoriLibRemoveListItem(&HistoryEntry->ListEntry);
            YoriLibFreeStringContents(&HistoryEntry->CmdLine);
            YoriLibFree(HistoryEntry);
            YoriShCommandHistoryCount--;
        }
        ReleaseMutex(YoriShHistoryLock);
    }
}

/**
 Configure the maximum amount of history to retain if the user has requested
 this behavior by setting YORIHISTSIZE.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShInitHistory(VOID)
{
    DWORD EnvVarLength;

    if (YoriShHistoryInitialized) {
        return FALSE;
    }

    if (YoriShHistoryLock == NULL) {
        YoriShHistoryLock = CreateMutex(NULL, FALSE, NULL);
    }

    if (YoriShHistoryLock == NULL) {
        return FALSE;
    }

    //
    //  Default the history buffer size to something sane.
    //

    YoriShCommandHistoryMax = 250;

    if (YoriShGlobal.CommandHistory.Next == NULL) {
        YoriLibInitializeListHead(&YoriShGlobal.CommandHistory);
    }

    //
    //  See if the user has other ideas.
    //

    EnvVarLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTSIZE"), NULL, 0, NULL);
    if (EnvVarLength != 0) {
        YORI_STRING HistSizeString;
        DWORD CharsConsumed;
        LONGLONG HistSize;

        if (!YoriLibAllocateString(&HistSizeString, EnvVarLength)) {
            return FALSE;
        }

        HistSizeString.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTSIZE"), HistSizeString.StartOfString, HistSizeString.LengthAllocated, NULL);

        if (HistSizeString.LengthInChars == 0 || HistSizeString.LengthInChars >= HistSizeString.LengthAllocated) {
            return FALSE;
        }

        if (YoriLibStringToNumber(&HistSizeString, TRUE, &HistSize, &CharsConsumed) && CharsConsumed > 0) {
            YoriShCommandHistoryMax = (DWORD)HistSize;
        }

        YoriLibFreeStringContents(&HistSizeString);
    }
    YoriShHistoryInitialized = TRUE;
    return TRUE;
}

/**
 Load history from a file if the user has requested this behavior by
 setting YORIHISTFILE.  Configure the maximum amount of history to retain
 if the user has requested this behavior by setting YORIHISTSIZE.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShLoadHistoryFromFile(VOID)
{
    DWORD EnvVarLength;
    YORI_STRING UserHistFileName;
    YORI_STRING FilePath;
    HANDLE FileHandle;
    PVOID LineContext = NULL;
    YORI_STRING LineString;

    if (YoriShHistoryInitialized) {
        return TRUE;
    }

    YoriShInitHistory();

    //
    //  Check if there's a file to load saved history from.
    //

    EnvVarLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTFILE"), NULL, 0, NULL);
    if (EnvVarLength == 0) {
        return TRUE;
    }

    if (!YoriLibAllocateString(&UserHistFileName, EnvVarLength)) {
        return FALSE;
    }

    UserHistFileName.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTFILE"), UserHistFileName.StartOfString, UserHistFileName.LengthAllocated, NULL);

    if (UserHistFileName.LengthInChars == 0 || UserHistFileName.LengthInChars >= UserHistFileName.LengthAllocated) {
        YoriLibFreeStringContents(&UserHistFileName);
        return FALSE;
    }

    if (!YoriLibUserStringToSingleFilePath(&UserHistFileName, TRUE, &FilePath)) {
        YoriLibFreeStringContents(&UserHistFileName);
        return FALSE;
    }

    YoriLibFreeStringContents(&UserHistFileName);

    FileHandle = CreateFile(FilePath.StartOfString,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        if (LastError != ERROR_FILE_NOT_FOUND) {
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("yori: open of %y failed: %s"), &FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            YoriLibFreeStringContents(&FilePath);
        }
        return FALSE;
    }

    YoriLibFreeStringContents(&FilePath);

    YoriLibInitEmptyString(&LineString);

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, FileHandle)) {
            break;
        }

        //
        //  If we fail to add to history, stop.  If it is added to history,
        //  that string is now owned by the history buffer, so reinitialize
        //  between lines.  The free below is really just a dereference.
        //

        if (!YoriShAddToHistory(&LineString, FALSE)) {
            break;
        }

        YoriLibFreeStringContents(&LineString);
        YoriLibInitEmptyString(&LineString);
    }

    YoriLibLineReadCloseOrCache(LineContext);
    YoriLibFreeStringContents(&LineString);
    CloseHandle(FileHandle);
    return TRUE;
}

/**
 Write the current command history buffer to a file, if the user has requested
 this behavior by configuring the YORIHISTFILE environment variable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShSaveHistoryToFile(VOID)
{
    DWORD FileNameLength;
    YORI_STRING UserHistFileName;
    YORI_STRING FilePath;
    HANDLE FileHandle;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_SH_HISTORY_ENTRY HistoryEntry;

    FileNameLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTFILE"), NULL, 0, NULL);
    if (FileNameLength == 0) {
        return TRUE;
    }

    if (!YoriLibAllocateString(&UserHistFileName, FileNameLength)) {
        return FALSE;
    }

    UserHistFileName.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTFILE"), UserHistFileName.StartOfString, UserHistFileName.LengthAllocated, NULL);

    if (UserHistFileName.LengthInChars == 0 || UserHistFileName.LengthInChars >= UserHistFileName.LengthAllocated) {
        YoriLibFreeStringContents(&UserHistFileName);
        return FALSE;
    }

    if (!YoriLibUserStringToSingleFilePath(&UserHistFileName, TRUE, &FilePath)) {
        YoriLibFreeStringContents(&UserHistFileName);
        return FALSE;
    }

    YoriLibFreeStringContents(&UserHistFileName);

    FileHandle = CreateFile(FilePath.StartOfString,
                            GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("yori: open of %y failed: %s"), &FilePath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FilePath);
        return FALSE;
    }

    YoriLibFreeStringContents(&FilePath);

    //
    //  Search the list of history.
    //

    if (WaitForSingleObject(YoriShHistoryLock, 0) == WAIT_OBJECT_0) {
        ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.CommandHistory, NULL);
        while (ListEntry != NULL) {
            HistoryEntry = CONTAINING_RECORD(ListEntry, YORI_SH_HISTORY_ENTRY, ListEntry);

            YoriLibOutputToDevice(FileHandle, 0, _T("%y\n"), &HistoryEntry->CmdLine);

            ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.CommandHistory, ListEntry);
        }
        ReleaseMutex(YoriShHistoryLock);
    }

    CloseHandle(FileHandle);
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 26165) // Analyze thinks a lock might be leaked
                                 // if WaitForSingleObject acquired it but
                                 // returned a different result.  That can't
                                 // happen.
#endif
    return TRUE;
}

/**
 Build history into an array of NULL terminated strings terminated by an
 additional NULL terminator.  The result must be freed with a subsequent
 call to @ref YoriLibFreeStringContents .

 @param MaximumNumber Specifies the maximum number of lines of history to
        return.  This number refers to the most recent history entries.
        If this value is zero, all are returned.

 @param HistoryStrings On successful completion, populated with the set of
        history strings.

 @return Return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShGetHistoryStrings(
    __in DWORD MaximumNumber,
    __inout PYORI_STRING HistoryStrings
    )
{
    DWORD CharsNeeded = 0;
    DWORD StringOffset;
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORI_SH_HISTORY_ENTRY HistoryEntry;
    PYORI_LIST_ENTRY StartReturningFrom = NULL;

    if (YoriShGlobal.CommandHistory.Next != NULL) {
        DWORD EntriesToSkip = 0;
        if (YoriShCommandHistoryCount > MaximumNumber && MaximumNumber > 0) {
            EntriesToSkip = YoriShCommandHistoryCount - MaximumNumber;
            while (EntriesToSkip > 0) {
                ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.CommandHistory, ListEntry);
                EntriesToSkip--;
            }

            StartReturningFrom = ListEntry;
        }

        ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.CommandHistory, StartReturningFrom);
        while (ListEntry != NULL) {
            HistoryEntry = CONTAINING_RECORD(ListEntry, YORI_SH_HISTORY_ENTRY, ListEntry);
            CharsNeeded += HistoryEntry->CmdLine.LengthInChars + 1;
            ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.CommandHistory, ListEntry);
        }
    }

    CharsNeeded += 1;

    if (HistoryStrings->LengthAllocated < CharsNeeded) {
        YoriLibFreeStringContents(HistoryStrings);
        if (!YoriLibAllocateString(HistoryStrings, CharsNeeded)) {
            return FALSE;
        }
    }

    StringOffset = 0;

    if (YoriShGlobal.CommandHistory.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.CommandHistory, StartReturningFrom);
        while (ListEntry != NULL) {
            HistoryEntry = CONTAINING_RECORD(ListEntry, YORI_SH_HISTORY_ENTRY, ListEntry);
            memcpy(&HistoryStrings->StartOfString[StringOffset], HistoryEntry->CmdLine.StartOfString, HistoryEntry->CmdLine.LengthInChars * sizeof(TCHAR));
            StringOffset += HistoryEntry->CmdLine.LengthInChars;
            HistoryStrings->StartOfString[StringOffset] = '\0';
            StringOffset++;
            ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.CommandHistory, ListEntry);
        }
    }
    HistoryStrings->StartOfString[StringOffset] = '\0';
    HistoryStrings->LengthInChars = StringOffset;

    return TRUE;
}

/**
 Add an entered command into the command history buffer and reallocate the
 string such that the caller's buffer is subsequently unreferenced.

 @param NewCmd Pointer to a Yori string corresponding to the new
        entry to add to history.
 
 @return TRUE to indicate an entry was successfully added, FALSE if it was
         not.
 */
__success(return)
BOOL
YoriShAddToHistoryAndReallocate(
    __in PYORI_STRING NewCmd
    )
{
    YORI_STRING NewString;

    if (NewCmd->LengthInChars == 0) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&NewString, NewCmd->LengthInChars)) {
        return FALSE;
    }

    memcpy(NewString.StartOfString, NewCmd->StartOfString, NewCmd->LengthInChars * sizeof(TCHAR));
    NewString.LengthInChars = NewCmd->LengthInChars;

    //
    //  This function will reference the string if it uses it, so this
    //  function can unconditionally dereference the string
    //

    if (!YoriShAddToHistory(&NewString, FALSE)) {
        YoriLibFreeStringContents(&NewString);
        return FALSE;
    }

    YoriLibFreeStringContents(&NewString);
    return TRUE;
}


// vim:sw=4:ts=4:et:

