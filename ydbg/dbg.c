/**
 * @file ydbg/dbg.c
 *
 * Yori shell debug processes
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
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
CHAR strYDbgHelpText[] =
        "\n"
        "Debugs processes and system components.\n"
        "\n"
        "YDBG -c <file>\n"
        "YDBG -d <pid> <file>\n"
        "YDBG [-l] [-w] -e <executable> <args>\n"
        "YDBG -license\n"
        "YDBG -k <file>\n"
        "YDBG -ks <pid> <file>\n"
        "\n"
        "   -c             Dump memory from kernel and user processes to a file\n"
        "   -d             Dump memory from a process to a file\n"
        "   -e             Execute a child process and capture debug output\n"
        "   -k             Dump memory from kernel to a file\n"
        "   -ks            Dump memory from kernel stacks associated with a process to a file\n"
        "   -l             Enable loader snaps for a child process\n"
        "   -w             Create child process in a new window\n";

/**
 Display usage text to the user.
 */
BOOL
YDbgHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("YDbg %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYDbgHelpText);
    return TRUE;
}

/**
 Write the memory from a process to a dump file.

 @param ProcessPid Specifies the process whose memory should be written.

 @param FileName Specifies the file name to write the memory to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YDbgDumpProcess(
    __in DWORD ProcessPid,
    __in PYORI_STRING FileName
    )
{
    HANDLE ProcessHandle;
    HANDLE FileHandle;
    DWORD LastError;
    LPTSTR ErrText;
    YORI_STRING FullPath;

    YoriLibLoadDbgHelpFunctions();
    if (DllDbgHelp.pMiniDumpWriteDump == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: OS support not present\n"));
        return FALSE;
    }

    ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessPid);
    if (ProcessHandle == NULL) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: OpenProcess of %i failed: %s"), ProcessPid, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);

    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: getfullpathname of %y failed: %s"), FileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(ProcessHandle);
        return FALSE;
    }

    FileHandle = CreateFile(FullPath.StartOfString, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: CreateFile of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        CloseHandle(ProcessHandle);
        return FALSE;
    }

    if (!DllDbgHelp.pMiniDumpWriteDump(ProcessHandle, ProcessPid, FileHandle, 2, NULL, NULL, NULL)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: MiniDumpWriteDump failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        CloseHandle(ProcessHandle);
        CloseHandle(FileHandle);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    CloseHandle(ProcessHandle);
    CloseHandle(FileHandle);
    return TRUE;
}

/**
 Scan through the set of processes in the system to find the requested
 process, and scan through each of its threads, opening them to have
 thread handles as expected by the debug API.  The thread handles need
 to be opened with a lot of access, potentially more than the compilation
 environment supports, so this open is specifying an explicit permissions
 mask.  If the running operating system does not support these permissions,
 it doesn't support the debug API either.  Note this function will display
 errors so the caller doesn't have to.

 @param ProcessPid The process ID to find.

 @param HandleArray On successful completion, populated with an array of
        open handles.  The caller should free this with YoriLibFree.

 @param HandleCount On successful completion, populated with the number of
        handles in the array.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YDbgBuildThreadArrayForProcessId(
    __in DWORD ProcessPid,
    __out PHANDLE *HandleArray,
    __out DWORD *HandleCount
    )
{
    PYORI_SYSTEM_PROCESS_INFORMATION ProcessInfo;
    PYORI_SYSTEM_PROCESS_INFORMATION CurrentEntry;
    PYORI_SYSTEM_THREAD_INFORMATION CurrentThread;
    DWORD Index;
    PHANDLE LocalHandleArray;

    if (!YoriLibGetSystemProcessList(&ProcessInfo)) {
        return FALSE;
    }

    CurrentEntry = ProcessInfo;
    do {
        if (CurrentEntry->ProcessId == ProcessPid) {
            break;
        }
        if (CurrentEntry->NextEntryOffset == 0) {
            CurrentEntry = NULL;
            break;
        }
        CurrentEntry = YoriLibAddToPointer(CurrentEntry, CurrentEntry->NextEntryOffset);
    } while(TRUE);

    if (CurrentEntry == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: process %i not found\n"), ProcessPid);
        YoriLibFree(ProcessInfo);
        return FALSE;
    }

    LocalHandleArray = YoriLibMalloc(CurrentEntry->NumberOfThreads * sizeof(HANDLE));
    if (LocalHandleArray == NULL) {
        YoriLibFree(ProcessInfo);
        return FALSE;
    }

    CurrentThread = (PYORI_SYSTEM_THREAD_INFORMATION)(CurrentEntry + 1);
    for (Index = 0; Index < CurrentEntry->NumberOfThreads; Index++) {

        //
        // Ask for all access to the thread including access rights that may
        // not be included by the compilation environment.
        //

        LocalHandleArray[Index] = DllKernel32.pOpenThread(STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF, FALSE, (DWORD)(DWORD_PTR)CurrentThread->ThreadId);
        if (LocalHandleArray[Index] == NULL) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: OpenThread of %i failed: %s"), CurrentThread->ThreadId, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            for (; Index > 0; Index--) {
                CloseHandle(LocalHandleArray[Index - 1]);
            }
            YoriLibFree(LocalHandleArray);
            YoriLibFree(ProcessInfo);
            return FALSE;
        }
    }
    *HandleArray = LocalHandleArray;
    *HandleCount = CurrentEntry->NumberOfThreads;

    YoriLibFree(ProcessInfo);
    return TRUE;
}


/**
 Write the kernel stacks owned by a process to a dump file.

 @param ProcessPid Specifies the process whose kernel stacks should be written.

 @param FileName Specifies the file name to write the memory to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YDbgDumpProcessKernelStacks(
    __in DWORD ProcessPid,
    __in PYORI_STRING FileName
    )
{
    YORI_SYSDBG_TRIAGE_DUMP_CONTROL Ctrl;
    PVOID Buffer;
    DWORD BufferLength;
    LONG NtStatus;
    HANDLE FileHandle;
    DWORD LastError;
    DWORD Index;
    LPTSTR ErrText;
    YORI_STRING FullPath;
    DWORD BytesWritten;
    DWORD BytesWrittenToFile;

    UNREFERENCED_PARAMETER(ProcessPid);

    ZeroMemory(&Ctrl, sizeof(Ctrl));
    if (DllNtDll.pNtSystemDebugControl == NULL ||
        DllKernel32.pOpenThread == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: OS support not present\n"));
        return FALSE;
    }

    if (!YoriLibEnableDebugPrivilege()) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: could not enable debug privilege (access denied)\n"));
        return FALSE;
    }

    //
    //  Allocate space in memory to store the dump contents.
    //

    BufferLength = 4 * 1024 * 1024;
    Buffer = YoriLibMalloc(BufferLength);
    if (Buffer == NULL) {
        return FALSE;
    }

    //
    //  Find the requested process and open all of its threads.
    //

    if (!YDbgBuildThreadArrayForProcessId(ProcessPid, &Ctrl.HandleArray, &Ctrl.ThreadHandleCount))
    {
        YoriLibFree(Buffer);
        return FALSE;
    }

    //
    //  Capture a dump of all of the specified threads.
    //

    NtStatus = DllNtDll.pNtSystemDebugControl(29, &Ctrl, sizeof(Ctrl), Buffer, BufferLength, &BytesWritten);

    //
    //  Believe me or not, but keeping thread handles open to other processes
    //  is dangerous and I've observed it hard hang the system, so get rid of
    //  this liability as soon as possible.
    //

    for (Index = 0; Index < Ctrl.ThreadHandleCount; Index++) {
        CloseHandle(Ctrl.HandleArray[Index]);
    }

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6001) // Analyze doesn't understand this was set by
                                // an annotated out above.  For some reason it
                                // doesn't complain about the array access
                                // above, which seems far more convoluted.
#endif
    YoriLibFree(Ctrl.HandleArray);
    Ctrl.HandleArray = NULL;

    if (NtStatus != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: NtSystemDebugControl failed: %08x\n"), NtStatus);
        YoriLibFree(Buffer);
        return FALSE;
    }

    //
    //  Write the dump contents to a file.
    //

    YoriLibInitEmptyString(&FullPath);

    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: getfullpathname of %y failed: %s"), FileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFree(Buffer);
        return FALSE;
    }

    FileHandle = CreateFile(FullPath.StartOfString, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: CreateFile of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        YoriLibFree(Buffer);
        return FALSE;
    }

    if (!WriteFile(FileHandle, Buffer, BytesWritten, &BytesWrittenToFile, NULL)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: WriteFile to %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        YoriLibFree(Buffer);
        CloseHandle(FileHandle);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    CloseHandle(FileHandle);
    YoriLibFree(Buffer);
    return TRUE;
}

/**
 Write the memory from the kernel to a dump file.

 @param FileName Specifies the file name to write the memory to.

 @param IncludeAll If TRUE, capture user and hypervisor pages in addition
        to kernel pages.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YDbgDumpKernel(
    __in PYORI_STRING FileName,
    __in BOOLEAN IncludeAll
    )
{
    YORI_SYSDBG_LIVEDUMP_CONTROL Ctrl;
    HANDLE FileHandle;
    DWORD LastError;
    DWORD BytesWritten;
    LONG NtStatus;
    BOOL Result;
    LPTSTR ErrText;
    YORI_STRING FullPath;

    ZeroMemory(&Ctrl, sizeof(Ctrl));
    if (DllNtDll.pNtSystemDebugControl == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: OS support not present\n"));
        return FALSE;
    }

    if (!YoriLibEnableDebugPrivilege()) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: could not enable debug privilege (access denied)\n"));
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);

    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: getfullpathname of %y failed: %s"), FileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    FileHandle = CreateFile(FullPath.StartOfString, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: CreateFile of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    YoriLibCancelEnable();

    Ctrl.Version = 1;
    Ctrl.File = FileHandle;
    Ctrl.CancelEvent = YoriLibCancelGetEvent();

    if (IncludeAll) {
        Ctrl.Flags = SYSDBG_LIVEDUMP_FLAG_USER_PAGES;
        Ctrl.AddPagesFlags = SYSDBG_LIVEDUMP_ADD_PAGES_FLAG_HYPERVISOR;
    }

    Result = TRUE;
    NtStatus = DllNtDll.pNtSystemDebugControl(37, &Ctrl, sizeof(Ctrl), NULL, 0, &BytesWritten);
    if (NtStatus == 0xc0000120) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: operation cancelled\n"));
        Result = FALSE;
    } else if (NtStatus == 0xc0000354) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: this operation requires debugging enabled with 'bcdedit /debug on' followed by a reboot\n"));
        Result = FALSE;
    } else if (NtStatus == 0xc0000003) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: OS support not present\n"));
        Result = FALSE;
    } else if (NtStatus == 0xc0000002) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: 64 bit kernel dumps can only be generated from a 64 bit process\n"));
        Result = FALSE;
    } else if (NtStatus != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: NtSystemDebugControl failed: %08x\n"), NtStatus);
        Result = FALSE;
    }

    CloseHandle(FileHandle);
    if (Result == FALSE) {
        DeleteFile(FullPath.StartOfString);
    }
    YoriLibFreeStringContents(&FullPath);
    return Result;
}

/**
 Information about a process where the mini-debugger has observed it be
 launched and has not yet observed termination.
 */
typedef struct _YDBG_OUTSTANDING_PROCESS {

    /**
     The linkage of this process within the list of known processes.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     A handle to the process.  This has been duplicated within this program
     and must be closed when freeing this structure.
     */
    HANDLE hProcess;

    /**
     A handle to the initial thread within the process.  This has been
     duplicated within this program and must be closed when freeing this
     structure.
     */
    HANDLE hInitialThread;

    /**
     The process identifier for this process.
     */
    DWORD dwProcessId;

    /**
     The identifier for the initial thread within the process.
     */
    DWORD dwInitialThreadId;

} YDBG_OUTSTANDING_PROCESS, *PYDBG_OUTSTANDING_PROCESS;

/**
 Find a process in the list of known processes by its process ID.

 @param ListHead Pointer to the list of known processes.

 @param dwProcessId The process identifier of the process whose information is
        requested.

 @return Pointer to the information block about the child process.
 */
PYDBG_OUTSTANDING_PROCESS
YDbgFindProcess(
    __in PYORI_LIST_ENTRY ListHead,
    __in DWORD dwProcessId
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYDBG_OUTSTANDING_PROCESS Process;

    ListEntry = NULL;
    while (TRUE) {
        ListEntry = YoriLibGetNextListEntry(ListHead, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        Process = CONTAINING_RECORD(ListEntry, YDBG_OUTSTANDING_PROCESS, ListEntry);
        if (Process->dwProcessId == dwProcessId) {
            return Process;
        }
    }

    return NULL;
}

/**
 Deallocate information about a single known child process.

 @param Process Pointer to information about a single child process.
 */
VOID
YDbgFreeProcess(
    __in PYDBG_OUTSTANDING_PROCESS Process
    )
{
    YoriLibRemoveListItem(&Process->ListEntry);
    CloseHandle(Process->hProcess);
    CloseHandle(Process->hInitialThread);
    YoriLibFree(Process);
}

/**
 Deallocate information about all known child processes.

 @param ListHead Pointer to the list of known child processes.
 */
VOID
YDbgFreeAllProcesses(
    __in PYORI_LIST_ENTRY ListHead
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYDBG_OUTSTANDING_PROCESS Process;

    ListEntry = NULL;
    while (TRUE) {
        ListEntry = YoriLibGetNextListEntry(ListHead, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        Process = CONTAINING_RECORD(ListEntry, YDBG_OUTSTANDING_PROCESS, ListEntry);
        YDbgFreeProcess(Process);
        ListEntry = NULL;
    }
}


/**
 Pump debug events for child processes and complete when the initial process
 has terminated.

 @param ProcessId The process identifier of the master process to monitor.
        When a process with this ID terminates, pumping events completes and
        this function returns.

 @param DisplayLoaderSnaps TRUE if the program is asking to display loader
        snaps.  When configured, this function will read many output strings
        that originated in the loader, and will filter to those that seem
        most valuable.  When FALSE, this function will assume all debugger
        output came from the child process and will display it all.

 @return TRUE to indicate successful termination of the initial child process,
         FALSE to indicate failure.
 */
BOOL
YDbgPumpDebugEvents(
    __in DWORD ProcessId,
    __in BOOLEAN DisplayLoaderSnaps
    )
{
    YORI_LIST_ENTRY Processes;
    PYDBG_OUTSTANDING_PROCESS Process;

    YoriLibInitializeListHead(&Processes);

    while(TRUE) {
        DEBUG_EVENT DbgEvent;
        DWORD dwContinueStatus;

        ZeroMemory(&DbgEvent, sizeof(DbgEvent));
        if (!WaitForDebugEvent(&DbgEvent, INFINITE)) {
            break;
        }

#if YDBG_DEBUG
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("DbgEvent Pid %x Tid %x Event %x\n"), DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.dwDebugEventCode);
#endif

        dwContinueStatus = DBG_CONTINUE;

        switch(DbgEvent.dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT:
                Process = YoriLibMalloc(sizeof(YDBG_OUTSTANDING_PROCESS));
                if (Process == NULL) {
                    break;
                }

                ZeroMemory(Process, sizeof(YDBG_OUTSTANDING_PROCESS));
                DuplicateHandle(GetCurrentProcess(), DbgEvent.u.CreateProcessInfo.hProcess, GetCurrentProcess(), &Process->hProcess, 0, FALSE, DUPLICATE_SAME_ACCESS);
                DuplicateHandle(GetCurrentProcess(), DbgEvent.u.CreateProcessInfo.hThread, GetCurrentProcess(), &Process->hInitialThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
                Process->dwProcessId = DbgEvent.dwProcessId;
                Process->dwInitialThreadId = DbgEvent.dwThreadId;
                YoriLibAppendList(&Processes, &Process->ListEntry);
                CloseHandle(DbgEvent.u.CreateProcessInfo.hFile);
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                Process = YDbgFindProcess(&Processes, DbgEvent.dwProcessId);
                ASSERT(Process != NULL);
                if (Process != NULL) {
                    YDbgFreeProcess(Process);
                }
                break;
            case LOAD_DLL_DEBUG_EVENT:
                CloseHandle(DbgEvent.u.LoadDll.hFile);
                break;
            case EXCEPTION_DEBUG_EVENT:

                //
                //  Wow64 processes throw a breakpoint once 32 bit code starts
                //  running, and the debugger is expected to handle it.  The
                //  two codes are for breakpoint and x86 breakpoint
                //

                {
                    LPTSTR PrefixString;
                    dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                    if (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT ||
                        DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x4000001F) {
                        dwContinueStatus = DBG_CONTINUE;
                    }

                    if (DbgEvent.u.Exception.dwFirstChance) {
                        PrefixString = _T("first chance");
                    } else {
                        PrefixString = _T("second chance");
                    }

                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: %s exception %08x\n"), PrefixString, DbgEvent.u.Exception.ExceptionRecord.ExceptionCode);

                }
                break;
            case OUTPUT_DEBUG_STRING_EVENT:
                Process = YDbgFindProcess(&Processes, DbgEvent.dwProcessId);
                if (Process != NULL) {
                    PUCHAR Buffer;
                    DWORD StringLengthInBytes;
                    DWORD BufferLengthInBytes;
                    SIZE_T BytesRead;
                    YORI_STRING OutputString;
                    BOOLEAN DisplayString;

                    StringLengthInBytes = DbgEvent.u.DebugString.nDebugStringLength;
                    BufferLengthInBytes = StringLengthInBytes + 1;
                    if (DbgEvent.u.DebugString.fUnicode) {
                        StringLengthInBytes = StringLengthInBytes * sizeof(WCHAR);
                        BufferLengthInBytes = BufferLengthInBytes * sizeof(WCHAR);
                    }

                    Buffer = YoriLibMalloc(BufferLengthInBytes);
                    if (Buffer == NULL) {
                        break;
                    }

                    if (ReadProcessMemory(Process->hProcess, DbgEvent.u.DebugString.lpDebugStringData, Buffer, StringLengthInBytes, &BytesRead) == 0) {
                        YoriLibFree(Buffer);
                        break;
                    }

                    //
                    //  If the string is already Unicode, point to it
                    //  directly.  Otherwise, upconvert to Unicode. To the
                    //  best of my knowledge, the Unicode path here is
                    //  unreachable, because OutputDebugString internally is
                    //  limited to ANSI.
                    //

                    YoriLibInitEmptyString(&OutputString);
                    if (DbgEvent.u.DebugString.fUnicode) {
                        OutputString.StartOfString = (LPTSTR)Buffer;
                        OutputString.LengthInChars = BufferLengthInBytes / sizeof(WCHAR);
                    } else {
                        Buffer[StringLengthInBytes] = '\0';
                        YoriLibYPrintf(&OutputString, _T("%hs"), Buffer);
                        if (OutputString.StartOfString == NULL) {
                            YoriLibFree(Buffer);
                            break;
                        }
                    }

                    //
                    //  If the user wants to see loader snaps, show only
                    //  errors and warnings.
                    //

                    DisplayString = TRUE;
                    if (DisplayLoaderSnaps) {
                        YORI_STRING SearchText;
                        YoriLibConstantString(&SearchText, _T("- Ldrp"));
                        if (YoriLibFindFirstMatchingSubstring(&OutputString, 1, &SearchText, NULL)) {
                            DisplayString = FALSE;
                        }
                        if (!DisplayString) {
                            YoriLibConstantString(&SearchText, _T("ERROR"));
                            if (YoriLibFindFirstMatchingSubstring(&OutputString, 1, &SearchText, NULL)) {
                                DisplayString = TRUE;
                            }
                            YoriLibConstantString(&SearchText, _T("WARNING"));
                            if (YoriLibFindFirstMatchingSubstring(&OutputString, 1, &SearchText, NULL)) {
                                DisplayString = TRUE;
                            }
                        }
                    }

                    //
                    //  Trim any trailing newlines.  We'll insert a newline
                    //  unconditionally below.
                    //

                    while (OutputString.LengthInChars > 0 &&
                           (OutputString.StartOfString[OutputString.LengthInChars - 1] == '\n' ||
                            OutputString.StartOfString[OutputString.LengthInChars - 1] == '\r')) {
                        OutputString.LengthInChars--;
                    }

                    if (DisplayString) {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: %y\n"), &OutputString);
                    }
                    YoriLibFreeStringContents(&OutputString);
                    YoriLibFree(Buffer);
                }
                break;
        }

        ContinueDebugEvent(DbgEvent.dwProcessId, DbgEvent.dwThreadId, dwContinueStatus);
        if (DbgEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT &&
            DbgEvent.dwProcessId == ProcessId) {
            break;
        }
    }

    YDbgFreeAllProcesses(&Processes);

    return TRUE;
}

/**
 The name of the registry value that controls loader snaps.
 */
#define REG_LOADER_SNAP_VALUE _T("GlobalFlag")

/**
 The flag within the registry value that controls loader snaps.
 */
#define REG_LOADER_SNAP_FLAG  (2)

/**
 Return the path in the registry for a specific executable's image loading
 configuration.  The executable specified here can contain a full path.

 @param Executable Pointer to the path of an executable file.

 @param RegPath On successful completion, updated to contain the path to the
        registry key controlling this executable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YDbgBuildIFEOPathFromExecutable(
    __in PYORI_STRING Executable,
    __out PYORI_STRING RegPath
    )
{
    DWORD Index;
    YORI_STRING FileName;

    YoriLibInitEmptyString(&FileName);

    for (Index = Executable->LengthInChars; Index > 0; Index--) {
        if (YoriLibIsSep(Executable->StartOfString[Index - 1])) {
            FileName.StartOfString = &Executable->StartOfString[Index];
            FileName.LengthInChars = Executable->LengthInChars - Index;
            break;
        }
    }

    if (FileName.StartOfString == NULL) {
        FileName.StartOfString = Executable->StartOfString;
        FileName.LengthInChars = Executable->LengthInChars;
    }

    YoriLibInitEmptyString(RegPath);
    YoriLibYPrintf(RegPath, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%y"), &FileName);

    if (RegPath->StartOfString == NULL) {
        return FALSE;
    }

    return TRUE;
}

/**
 Attempt to enable loader snaps for a specific executable.

 @param Executable Pointer to the path to an executable.

 @param DisableRequired On successful completion, set to TRUE to indicate that
        this function enabled loader snaps.  Set to FALSE to indicate loader
        snaps were already enabled and should not be modified.

 @return Win32 error code, indicating ERROR_SUCCESS or reason for failure.
 */
DWORD
YDbgEnableLoaderSnaps(
    __in PYORI_STRING Executable,
    __out PBOOLEAN DisableRequired
    )
{
    LONG RegErr;
    DWORD Disposition;
    DWORD GlobalFlag;
    DWORD ValueSize;
    HKEY Key;
    YORI_STRING RegPath;

    *DisableRequired = FALSE;

    YoriLibLoadAdvApi32Functions();
    if (DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllAdvApi32.pRegSetValueExW == NULL ||
        DllAdvApi32.pRegCloseKey == NULL) {

        return ERROR_NOT_SUPPORTED;
    }

    if (!YDbgBuildIFEOPathFromExecutable(Executable, &RegPath)) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RegErr = DllAdvApi32.pRegCreateKeyExW(HKEY_LOCAL_MACHINE, RegPath.StartOfString, 0, NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &Key, &Disposition);
    YoriLibFreeStringContents(&RegPath);
    if (RegErr != ERROR_SUCCESS) {
        return RegErr;
    }

    ValueSize = sizeof(GlobalFlag);
    GlobalFlag = 0;
    RegErr = DllAdvApi32.pRegQueryValueExW(Key, REG_LOADER_SNAP_VALUE, NULL, NULL, (LPBYTE)&GlobalFlag, &ValueSize);
    if (RegErr != ERROR_SUCCESS && RegErr != ERROR_FILE_NOT_FOUND) {
        DllAdvApi32.pRegCloseKey(Key);
        return RegErr;
    }

    if ((GlobalFlag & REG_LOADER_SNAP_FLAG) == 0) {
        GlobalFlag = GlobalFlag | REG_LOADER_SNAP_FLAG;
        *DisableRequired = TRUE;
    }

    RegErr = DllAdvApi32.pRegSetValueExW(Key, REG_LOADER_SNAP_VALUE, 0, REG_DWORD, (LPBYTE)&GlobalFlag, sizeof(GlobalFlag));
    if (RegErr != ERROR_SUCCESS) {
        DllAdvApi32.pRegCloseKey(Key);
        return RegErr;
    }

    RegErr = DllAdvApi32.pRegCloseKey(Key);
    return RegErr;
}

/**
 Attempt to disable loader snaps for a specific executable.

 @param Executable Pointer to the path to an executable.

 @return Win32 error code, indicating ERROR_SUCCESS or reason for failure.
 */
DWORD
YDbgDisableLoaderSnaps(
    __in PYORI_STRING Executable
    )
{
    LONG RegErr;
    DWORD GlobalFlag;
    DWORD ValueSize;
    HKEY Key;
    YORI_STRING RegPath;

    YoriLibLoadAdvApi32Functions();
    if (DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegDeleteKeyW == NULL ||
        DllAdvApi32.pRegDeleteValueW == NULL ||
        DllAdvApi32.pRegEnumKeyExW == NULL ||
        DllAdvApi32.pRegEnumValueW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllAdvApi32.pRegSetValueExW == NULL ||
        DllAdvApi32.pRegCloseKey == NULL) {

        return ERROR_NOT_SUPPORTED;
    }

    if (!YDbgBuildIFEOPathFromExecutable(Executable, &RegPath)) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RegErr = DllAdvApi32.pRegOpenKeyExW(HKEY_LOCAL_MACHINE, RegPath.StartOfString, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &Key);
    if (RegErr != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&RegPath);
        return RegErr;
    }

    ValueSize = sizeof(GlobalFlag);
    GlobalFlag = 0;
    RegErr = DllAdvApi32.pRegQueryValueExW(Key, REG_LOADER_SNAP_VALUE, NULL, NULL, (LPBYTE)&GlobalFlag, &ValueSize);
    if (RegErr != ERROR_SUCCESS && RegErr != ERROR_FILE_NOT_FOUND) {
        YoriLibFreeStringContents(&RegPath);
        DllAdvApi32.pRegCloseKey(Key);
        return RegErr;
    }

    if ((GlobalFlag & REG_LOADER_SNAP_FLAG) != 0) {
        GlobalFlag = GlobalFlag & ~(REG_LOADER_SNAP_FLAG);
    }

    if (GlobalFlag != 0) {
        YoriLibFreeStringContents(&RegPath);
        RegErr = DllAdvApi32.pRegSetValueExW(Key, REG_LOADER_SNAP_VALUE, 0, REG_DWORD, (LPBYTE)&GlobalFlag, sizeof(GlobalFlag));
        if (RegErr != ERROR_SUCCESS) {
            DllAdvApi32.pRegCloseKey(Key);
            return RegErr;
        }
    } else {
        TCHAR ValueName[1];
        DWORD ValueNameLength;

        RegErr = DllAdvApi32.pRegDeleteValueW(Key, REG_LOADER_SNAP_VALUE);
        if (RegErr != ERROR_SUCCESS) {
            YoriLibFreeStringContents(&RegPath);
            DllAdvApi32.pRegCloseKey(Key);
            return RegErr;
        }

        //
        //  Check if the last value was removed, and if so, delete the key.
        //  The contents of the values are not relevant, only the existence,
        //  so a single char buffer is acceptable to distinguish cases.
        //

        ValueNameLength = sizeof(ValueName);
        RegErr = DllAdvApi32.pRegEnumValueW(Key, 0, ValueName, &ValueNameLength, NULL, NULL, NULL, NULL);
 
        //
        //  This delete will fail if subkeys are present, so we don't need to
        //  check for them explicitly.
        //

        if (RegErr == ERROR_NO_MORE_ITEMS) {
            RegErr = DllAdvApi32.pRegDeleteKeyW(HKEY_LOCAL_MACHINE, RegPath.StartOfString);
        }


        YoriLibFreeStringContents(&RegPath);
    }

    RegErr = DllAdvApi32.pRegCloseKey(Key);
    return RegErr;
}

/**
 Launch a child process and commence pumping debug messages for it.

 @param EnableLoaderSnaps TRUE to indicate that loader snaps should be enabled
        when launching this process, or FALSE to retain existing system
        configuration.

 @param CreateNewWindow If TRUE, a new console window should be created for
        the child process.  The debugger will remain running on the existing
        console.  If FALSE, both processes share a console.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD 
YDbgDebugChildProcess(
    __in BOOLEAN EnableLoaderSnaps,
    __in BOOLEAN CreateNewWindow,
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    DWORD ExitCode;
    DWORD ProcessFlags;
    YORI_STRING CmdLine;
    YORI_STRING Executable;
    PYORI_STRING ChildArgs;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    BOOLEAN DisableLoaderSnaps;


    DisableLoaderSnaps = FALSE;
    YoriLibInitEmptyString(&Executable);

    if (!YoriLibLocateExecutableInPath(&ArgV[0], NULL, NULL, &Executable)) {
        YoriLibInitEmptyString(&Executable);
    }

    if (Executable.LengthInChars == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: unable to find executable\n"));
        YoriLibFreeStringContents(&Executable);
        return EXIT_FAILURE;
    }

    ChildArgs = YoriLibMalloc(ArgC * sizeof(YORI_STRING));
    if (ChildArgs == NULL) {
        YoriLibFreeStringContents(&Executable);
        return EXIT_FAILURE;
    }

    memcpy(&ChildArgs[0], &Executable, sizeof(YORI_STRING));
    if (ArgC > 1) {
        memcpy(&ChildArgs[1], &ArgV[1], (ArgC - 1) * sizeof(YORI_STRING));
    }

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgC, ChildArgs, TRUE, TRUE, &CmdLine)) {
        YoriLibFreeStringContents(&Executable);
        YoriLibFree(ChildArgs);
        return EXIT_FAILURE;
    }

    YoriLibFree(ChildArgs);

    ASSERT(YoriLibIsStringNullTerminated(&CmdLine));

    if (EnableLoaderSnaps) {
        ExitCode = YDbgEnableLoaderSnaps(&Executable, &DisableLoaderSnaps);
        if (ExitCode == ERROR_ACCESS_DENIED) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: access denied enabling loader snap.  Note this typically requires running as Administrator.\n"));
        } else if (ExitCode != ERROR_SUCCESS) {
            LPTSTR ErrText;
            ErrText = YoriLibGetWinErrorText(ExitCode);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
        }

        if (ExitCode != ERROR_SUCCESS) {
            YoriLibFreeStringContents(&Executable);
            YoriLibFreeStringContents(&CmdLine);
            return ExitCode;
        }
    }

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    ProcessFlags = DEBUG_PROCESS | CREATE_DEFAULT_ERROR_MODE;
    if (CreateNewWindow) {
        ProcessFlags = ProcessFlags | CREATE_NEW_CONSOLE;
    }

    if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, ProcessFlags, NULL, NULL, &StartupInfo, &ProcessInfo)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: execution failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&CmdLine);
        YoriLibFreeStringContents(&Executable);
        return EXIT_FAILURE;
    }

    YoriLibFreeStringContents(&CmdLine);

    YDbgPumpDebugEvents(ProcessInfo.dwProcessId, EnableLoaderSnaps);

    if (DisableLoaderSnaps) {
        YDbgDisableLoaderSnaps(&Executable);
    }

    YoriLibFreeStringContents(&Executable);

    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);

    return ExitCode;
}

/**
 The set of operations supported by this program.
 */
typedef enum _YDBG_OP {
    YDbgOperationNone = 0,
    YDbgOperationProcessDump = 1,
    YDbgOperationKernelDump = 2,
    YDbgOperationCompleteDump = 3,
    YDbgOperationProcessKernelStacks = 4,
    YDbgOperationDebugChildProcess = 5,
} YDBG_OP;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the ydbg builtin command.
 */
#define ENTRYPOINT YoriCmd_YDBG
#else
/**
 The main entrypoint for the ydbg standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the ydbg cmdlet.

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
    DWORD StartArg = 1;
    YORI_STRING Arg;
    YDBG_OP Op;
    DWORD ProcessPid = 0;
    PYORI_STRING FileName = NULL;
    LONGLONG llTemp;
    DWORD CharsConsumed;
    DWORD ExitResult;
    BOOLEAN EnableLoaderSnaps;
    BOOLEAN CreateNewWindow;

    EnableLoaderSnaps = FALSE;
    CreateNewWindow = FALSE;
    Op = YDbgOperationNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YDbgHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    Op = YDbgOperationCompleteDump;
                    FileName = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i += 1;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                if (ArgC > i + 2) {
                    Op = YDbgOperationProcessDump;
                    if (!YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed)) {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid pid.\n"), &ArgV[i + 1]);
                        return EXIT_FAILURE;
                    }
                    ProcessPid = (DWORD)llTemp;
                    FileName = &ArgV[i + 2];
                    ArgumentUnderstood = TRUE;
                    i += 2;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                if (ArgC > i + 1) {
                    Op = YDbgOperationDebugChildProcess;
                    StartArg = i + 1;
                    ArgumentUnderstood = TRUE;
                    break;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("k")) == 0) {
                if (ArgC > i + 1) {
                    Op = YDbgOperationKernelDump;
                    FileName = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i += 1;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("ks")) == 0) {
                if (ArgC > i + 2) {
                    Op = YDbgOperationProcessKernelStacks;
                    if (!YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed)) {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid pid.\n"), &ArgV[i + 1]);
                        return EXIT_FAILURE;
                    }
                    ProcessPid = (DWORD)llTemp;
                    FileName = &ArgV[i + 2];
                    ArgumentUnderstood = TRUE;
                    i += 2;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                EnableLoaderSnaps = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("w")) == 0) {
                CreateNewWindow = TRUE;
                ArgumentUnderstood = TRUE;
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

    if (Op == YDbgOperationNone) {
        YDbgHelp();
        return EXIT_SUCCESS;
    }

    i = StartArg;

    ExitResult = EXIT_SUCCESS;
    if (Op == YDbgOperationProcessDump) {
        if (!YDbgDumpProcess(ProcessPid, FileName)) {
            ExitResult = EXIT_FAILURE;
        }
    } else if (Op == YDbgOperationProcessKernelStacks) {
        if (!YDbgDumpProcessKernelStacks(ProcessPid, FileName)) {
            ExitResult = EXIT_FAILURE;
        }
    } else if (Op == YDbgOperationKernelDump) {
        if (!YDbgDumpKernel(FileName, FALSE)) {
            ExitResult = EXIT_FAILURE;
        }
    } else if (Op == YDbgOperationCompleteDump) {
        if (!YDbgDumpKernel(FileName, TRUE)) {
            ExitResult = EXIT_FAILURE;
        }
    } else if (Op == YDbgOperationDebugChildProcess) {
        ExitResult = YDbgDebugChildProcess(EnableLoaderSnaps, CreateNewWindow, ArgC - StartArg, &ArgV[StartArg]);
    }

    return ExitResult;
}

// vim:sw=4:ts=4:et:
