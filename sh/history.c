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
 List of command history.
 */
YORI_LIST_ENTRY YoriShCommandHistory;

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
 Add an entered command into the command history buffer.  On error it is
 silently discarded and not propagated into history.

 @param NewCmd Pointer to a Yori string corresponding to the new
        entry to add to history.
 */
BOOL
YoriShAddToHistory(
    __in PYORI_STRING NewCmd
    )
{
    DWORD LengthToAllocate;
    PYORI_SH_HISTORY_ENTRY NewHistoryEntry;

    if (NewCmd->LengthInChars == 0) {
        return TRUE;
    }

    LengthToAllocate = sizeof(YORI_SH_HISTORY_ENTRY);

    if (WaitForSingleObject(YoriShHistoryLock, 0) == WAIT_OBJECT_0) {

        NewHistoryEntry = YoriLibMalloc(LengthToAllocate);
        if (NewHistoryEntry == NULL) {
            ReleaseMutex(YoriShHistoryLock);
            return FALSE;
        }

        YoriLibCloneString(&NewHistoryEntry->CmdLine, NewCmd);

        if (YoriShCommandHistory.Next == NULL) {
            YoriLibInitializeListHead(&YoriShCommandHistory);
        }

        YoriLibAppendList(&YoriShCommandHistory, &NewHistoryEntry->ListEntry);
        YoriShCommandHistoryCount++;
        while (YoriShCommandHistoryCount > YoriShCommandHistoryMax) {
            PYORI_LIST_ENTRY ListEntry;
            PYORI_SH_HISTORY_ENTRY OldHistoryEntry;

            ListEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, NULL);
            OldHistoryEntry = CONTAINING_RECORD(ListEntry, YORI_SH_HISTORY_ENTRY, ListEntry);
            YoriLibRemoveListItem(ListEntry);
            YoriLibFreeStringContents(&OldHistoryEntry->CmdLine);
            YoriLibFree(OldHistoryEntry);
            YoriShCommandHistoryCount--;
        }
        ReleaseMutex(YoriShHistoryLock);
    }

    return TRUE;
}

/**
 Free all command history.
 */
VOID
YoriShClearAllHistory()
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORI_SH_HISTORY_ENTRY HistoryEntry;

    if (WaitForSingleObject(YoriShHistoryLock, 0) == WAIT_OBJECT_0) {
        ListEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, NULL);
        while (ListEntry != NULL) {
            HistoryEntry = CONTAINING_RECORD(ListEntry, YORI_SH_HISTORY_ENTRY, ListEntry);
            ListEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, ListEntry);
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
BOOL
YoriShInitHistory()
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

    if (YoriShCommandHistory.Next == NULL) {
        YoriLibInitializeListHead(&YoriShCommandHistory);
    }

    //
    //  See if the user has other ideas.
    //

    EnvVarLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTSIZE"), NULL, 0);
    if (EnvVarLength != 0) {
        YORI_STRING HistSizeString;
        DWORD CharsConsumed;
        LONGLONG HistSize;

        if (!YoriLibAllocateString(&HistSizeString, EnvVarLength)) {
            return FALSE;
        }

        HistSizeString.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTSIZE"), HistSizeString.StartOfString, HistSizeString.LengthAllocated);

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
BOOL
YoriShLoadHistoryFromFile()
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

    EnvVarLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTFILE"), NULL, 0);
    if (EnvVarLength == 0) {
        return TRUE;
    }

    if (!YoriLibAllocateString(&UserHistFileName, EnvVarLength)) {
        return FALSE;
    }

    UserHistFileName.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTFILE"), UserHistFileName.StartOfString, UserHistFileName.LengthAllocated);

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

        if (!YoriShAddToHistory(&LineString)) {
            break;
        }

        YoriLibFreeStringContents(&LineString);
        YoriLibInitEmptyString(&LineString);
    }

    YoriLibLineReadClose(LineContext);
    YoriLibFreeStringContents(&LineString);
    CloseHandle(FileHandle);
    return TRUE;
}

/**
 Write the current command history buffer to a file, if the user has requested
 this behavior by configuring the YORIHISTFILE environment variable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShSaveHistoryToFile()
{
    DWORD FileNameLength;
    YORI_STRING UserHistFileName;
    YORI_STRING FilePath;
    HANDLE FileHandle;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_SH_HISTORY_ENTRY HistoryEntry;

    FileNameLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTFILE"), NULL, 0);
    if (FileNameLength == 0) {
        return TRUE;
    }

    if (!YoriLibAllocateString(&UserHistFileName, FileNameLength)) {
        return FALSE;
    }

    UserHistFileName.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIHISTFILE"), UserHistFileName.StartOfString, UserHistFileName.LengthAllocated);

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
        ListEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, NULL);
        while (ListEntry != NULL) {
            HistoryEntry = CONTAINING_RECORD(ListEntry, YORI_SH_HISTORY_ENTRY, ListEntry);

            YoriLibOutputToDevice(FileHandle, 0, _T("%y\n"), &HistoryEntry->CmdLine);

            ListEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, ListEntry);
        }
        ReleaseMutex(YoriShHistoryLock);
    }

    CloseHandle(FileHandle);
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

    if (YoriShCommandHistory.Next != NULL) {
        DWORD EntriesToSkip = 0;
        if (YoriShCommandHistoryCount > MaximumNumber && MaximumNumber > 0) {
            EntriesToSkip = YoriShCommandHistoryCount - MaximumNumber;
            while (EntriesToSkip > 0) {
                ListEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, ListEntry);
                EntriesToSkip--;
            }

            StartReturningFrom = ListEntry;
        }

        ListEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, StartReturningFrom);
        while (ListEntry != NULL) {
            HistoryEntry = CONTAINING_RECORD(ListEntry, YORI_SH_HISTORY_ENTRY, ListEntry);
            CharsNeeded += HistoryEntry->CmdLine.LengthInChars + 1;
            ListEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, ListEntry);
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

    if (YoriShCommandHistory.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, StartReturningFrom);
        while (ListEntry != NULL) {
            HistoryEntry = CONTAINING_RECORD(ListEntry, YORI_SH_HISTORY_ENTRY, ListEntry);
            memcpy(&HistoryStrings->StartOfString[StringOffset], HistoryEntry->CmdLine.StartOfString, HistoryEntry->CmdLine.LengthInChars * sizeof(TCHAR));
            StringOffset += HistoryEntry->CmdLine.LengthInChars;
            HistoryStrings->StartOfString[StringOffset] = '\0';
            StringOffset++;
            ListEntry = YoriLibGetNextListEntry(&YoriShCommandHistory, ListEntry);
        }
    }
    HistoryStrings->StartOfString[StringOffset] = '\0';
    HistoryStrings->LengthInChars = StringOffset;

    return TRUE;
}

// vim:sw=4:ts=4:et:

