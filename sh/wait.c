/**
 * @file /sh/wait.c
 *
 * Yori shell debug child processes and wait for completion.  These two
 * seemingly unrelated things are joined because the debugger is launched
 * when waiting.
 *
 * Copyright (c) 2017-2022 Malcolm J. Smith
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

#if DBG
/**
 If TRUE, use verbose output when invoking processes under a debugger.
 */
#define YORI_SH_DEBUG_DEBUGGER 0
#else
/**
 If TRUE, use verbose output when invoking processes under a debugger.
 */
#define YORI_SH_DEBUG_DEBUGGER 0
#endif

/**
 The smallest unit of memory that can have protection applied.  It's not
 super critical that this match the system page size - this is used to
 request smaller memory reads from the target.  So long as the system page
 size is a multiple of this value, the logic will still be correct.
 */
#define YORI_SH_MEMORY_PROTECTION_SIZE (4096)

/**
 Given a process that has finished execution, locate the environment block
 within the process and extract it into a string in the currently executing
 process.

 @param ProcessHandle The handle of the child process whose environment is
        requested.

 @param EnvString On successful completion, a newly allocated string
        containing the child process environment.

 @param CurrentDirectory On successful completion, a newly allocated string
        containing the child current directory.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShSuckEnv(
    __in HANDLE ProcessHandle,
    __out PYORI_STRING EnvString,
    __out PYORI_STRING CurrentDirectory
    )
{
    PROCESS_BASIC_INFORMATION BasicInfo;

    LONG Status;
    DWORD dwBytesReturned;
    SIZE_T BytesReturned;
    YORI_ALLOC_SIZE_T EnvironmentBlockPageOffset;
    YORI_ALLOC_SIZE_T EnvCharsToMask;
    YORI_ALLOC_SIZE_T CurrentDirectoryCharsToRead;
    PVOID ProcessParamsBlockToRead;
    PVOID CurrentDirectoryToRead;
    PVOID EnvironmentBlockToRead;
    BOOL TargetProcess32BitPeb;
    DWORD OsVerMajor;
    DWORD OsVerMinor;
    DWORD OsBuildNumber;

    if (DllNtDll.pNtQueryInformationProcess == NULL) {
        return FALSE;
    }

    TargetProcess32BitPeb = YoriLibDoesProcessHave32BitPeb(ProcessHandle);

    Status = DllNtDll.pNtQueryInformationProcess(ProcessHandle, 0, &BasicInfo, sizeof(BasicInfo), &dwBytesReturned);
    if (Status != 0) {
        return FALSE;
    }

#if YORI_SH_DEBUG_DEBUGGER
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Peb at %p, Target %i bit PEB\n"), BasicInfo.PebBaseAddress, TargetProcess32BitPeb?32:64);
#endif

    if (TargetProcess32BitPeb) {
        YORI_LIB_PEB32_NATIVE ProcessPeb;

        if (!ReadProcessMemory(ProcessHandle, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            return FALSE;
        }

#if YORI_SH_DEBUG_DEBUGGER
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Peb contents:\n"));
        YoriLibHexDump((PUCHAR)&ProcessPeb,
                       0,
                       (DWORD)BytesReturned,
                       sizeof(DWORD), 
                       YORI_LIB_HEX_FLAG_DISPLAY_OFFSET);

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ProcessParameters offset %x\n"), FIELD_OFFSET(YORI_LIB_PEB32_NATIVE, ProcessParameters));
#endif

        ProcessParamsBlockToRead = (PVOID)(ULONG_PTR)ProcessPeb.ProcessParameters;

    } else {
        YORI_LIB_PEB64 ProcessPeb;

        if (!ReadProcessMemory(ProcessHandle, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            return FALSE;
        }

#if YORI_SH_DEBUG_DEBUGGER
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ProcessParameters offset %x\n"), FIELD_OFFSET(YORI_LIB_PEB64, ProcessParameters));
#endif

        ProcessParamsBlockToRead = (PVOID)(ULONG_PTR)ProcessPeb.ProcessParameters;
    }

#if YORI_SH_DEBUG_DEBUGGER
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ProcessParameters at %p\n"), ProcessParamsBlockToRead);
#endif

    if (TargetProcess32BitPeb) {
        YORI_LIB_PROCESS_PARAMETERS32 ProcessParameters;

        if (!ReadProcessMemory(ProcessHandle, ProcessParamsBlockToRead, &ProcessParameters, sizeof(ProcessParameters), &BytesReturned)) {
            return FALSE;
        }

#if YORI_SH_DEBUG_DEBUGGER
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ProcessParameters contents:\n"));
        YoriLibHexDump((PUCHAR)&ProcessParameters,
                       0,
                       (DWORD)BytesReturned,
                       sizeof(DWORD),
                       YORI_LIB_HEX_FLAG_DISPLAY_OFFSET);
#endif

        CurrentDirectoryToRead = (PVOID)(ULONG_PTR)ProcessParameters.CurrentDirectory;
        CurrentDirectoryCharsToRead = (YORI_ALLOC_SIZE_T)ProcessParameters.CurrentDirectoryLengthInBytes / sizeof(TCHAR);
        EnvironmentBlockToRead = (PVOID)(ULONG_PTR)ProcessParameters.EnvironmentBlock;
        EnvironmentBlockPageOffset = (YORI_SH_MEMORY_PROTECTION_SIZE - 1) & (YORI_ALLOC_SIZE_T)ProcessParameters.EnvironmentBlock;
    } else {
        YORI_LIB_PROCESS_PARAMETERS64 ProcessParameters;

        if (!ReadProcessMemory(ProcessHandle, ProcessParamsBlockToRead, &ProcessParameters, sizeof(ProcessParameters), &BytesReturned)) {
            return FALSE;
        }

        CurrentDirectoryToRead = (PVOID)(ULONG_PTR)ProcessParameters.CurrentDirectory;
        CurrentDirectoryCharsToRead = (YORI_ALLOC_SIZE_T)ProcessParameters.CurrentDirectoryLengthInBytes / sizeof(TCHAR);
        EnvironmentBlockToRead = (PVOID)(ULONG_PTR)ProcessParameters.EnvironmentBlock;
        EnvironmentBlockPageOffset = (YORI_SH_MEMORY_PROTECTION_SIZE - 1) & (YORI_ALLOC_SIZE_T)ProcessParameters.EnvironmentBlock;
    }

    EnvCharsToMask = EnvironmentBlockPageOffset / sizeof(TCHAR);
#if YORI_SH_DEBUG_DEBUGGER
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("EnvironmentBlock at %p PageOffset %04x EnvCharsToMask %04x\n"), EnvironmentBlockToRead, EnvironmentBlockPageOffset, EnvCharsToMask);
#endif

    //
    //  Attempt to read 64Kb of environment minus the offset from the
    //  page containing the environment.  This occurs because older
    //  versions of Windows don't record how large the block is.  As
    //  a result, this may be truncated, which is acceptable.
    //

    if (!YoriLibAllocateString(EnvString, 32 * 1024 - EnvCharsToMask)) {
        return FALSE;
    }

    //
    //  Loop issuing reads and decreasing the read size by one page each
    //  time if reads are failing due to invalid memory on the target
    //

    do {

        if (!ReadProcessMemory(ProcessHandle, EnvironmentBlockToRead, EnvString->StartOfString, EnvString->LengthAllocated * sizeof(TCHAR), &BytesReturned)) {
            DWORD Err = GetLastError();
            if (Err != ERROR_PARTIAL_COPY && Err != ERROR_NOACCESS) {
#if YORI_SH_DEBUG_DEBUGGER
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ReadProcessMemory returned error %08x BytesReturned %i\n"), Err, BytesReturned);
#endif
                YoriLibFreeStringContents(EnvString);
                return FALSE;
            }

            if (EnvString->LengthAllocated * sizeof(TCHAR) < YORI_SH_MEMORY_PROTECTION_SIZE) {
#if YORI_SH_DEBUG_DEBUGGER
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ReadProcessMemory failed to read less than a page\n"));
#endif
                YoriLibFreeStringContents(EnvString);
                return FALSE;
            }

            EnvString->LengthAllocated -= YORI_SH_MEMORY_PROTECTION_SIZE / sizeof(TCHAR);
        } else {

#if YORI_SH_DEBUG_DEBUGGER
            if (BytesReturned > 0x2000) {
                BytesReturned = 0x2000;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Environment contents:\n"));
            YoriLibHexDump((PUCHAR)EnvString->StartOfString,
                           0,
                           (DWORD)BytesReturned,
                           sizeof(UCHAR),
                           YORI_LIB_HEX_FLAG_DISPLAY_OFFSET | YORI_LIB_HEX_FLAG_DISPLAY_CHARS);
#endif
            break;
        }

    } while (TRUE);

    //
    //  NT 3.1 describes the environment block in ANSI.  Although people love
    //  to criticize it, this is probably the worst quirk I've found in it
    //  yet.
    //

    YoriLibGetOsVersion(&OsVerMajor, &OsVerMinor, &OsBuildNumber);

    if (OsVerMajor == 3 && OsVerMinor == 10) {
        YORI_STRING UnicodeEnvString;

        if (!YoriLibAreAnsiEnvironmentStringsValid((PUCHAR)EnvString->StartOfString, EnvString->LengthAllocated, &UnicodeEnvString)) {
#if YORI_SH_DEBUG_DEBUGGER
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("EnvString not valid\n"));
#endif
            YoriLibFreeStringContents(EnvString);
            return FALSE;
        }

        YoriLibFreeStringContents(EnvString);
        memcpy(EnvString, &UnicodeEnvString, sizeof(YORI_STRING));
    } else {
        if (!YoriLibAreEnvironmentStringsValid(EnvString)) {
#if YORI_SH_DEBUG_DEBUGGER
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("EnvString not valid\n"));
#endif
            YoriLibFreeStringContents(EnvString);
            return FALSE;
        }
    }

#if YORI_SH_DEBUG_DEBUGGER
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("EnvString has %i chars\n"), EnvString->LengthInChars);
#endif

    if (EnvString->LengthInChars <= 2) {
#if YORI_SH_DEBUG_DEBUGGER
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("EnvString is empty\n"));
#endif
        YoriLibFreeStringContents(EnvString);
        return FALSE;
    }

    if (!YoriLibAllocateString(CurrentDirectory, CurrentDirectoryCharsToRead + 1)) {
        YoriLibFreeStringContents(EnvString);
        return FALSE;
    }

    if (!ReadProcessMemory(ProcessHandle, CurrentDirectoryToRead, CurrentDirectory->StartOfString, CurrentDirectoryCharsToRead * sizeof(TCHAR), &BytesReturned)) {
        YoriLibFreeStringContents(EnvString);
        return FALSE;
    }

    CurrentDirectory->LengthInChars = CurrentDirectoryCharsToRead;
    CurrentDirectory->StartOfString[CurrentDirectoryCharsToRead] = '\0';

    return TRUE;
}

/**
 Find a process in the list of known debugged child processes by its process
 ID.

 @param ListHead Pointer to the list of known processes.

 @param dwProcessId The process identifier of the process whose information is
        requested.

 @return Pointer to the information block about the child process.
 */
PYORI_LIBSH_DEBUGGED_CHILD_PROCESS
YoriShFindDebuggedChildProcess(
    __in PYORI_LIST_ENTRY ListHead,
    __in DWORD dwProcessId
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIBSH_DEBUGGED_CHILD_PROCESS Process;

    ListEntry = NULL;
    while (TRUE) {
        ListEntry = YoriLibGetNextListEntry(ListHead, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        Process = CONTAINING_RECORD(ListEntry, YORI_LIBSH_DEBUGGED_CHILD_PROCESS, ListEntry);
        if (Process->dwProcessId == dwProcessId) {
            return Process;
        }
    }

    return NULL;
}

/**
 A structure passed into a debugger thread to indicate which actions to
 perform.
 */
typedef struct _YORI_SH_DEBUG_THREAD_CONTEXT {

    /**
     A referenced ExecContext indicating the process to launch.
     */
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext;

    /**
     An event to signal once the process has been launched, indicating that
     redirection has been initiated, the process has started, and redirection
     has been reverted.  This indicates the calling thread is free to reason
     about stdin/stdout and console state.
     */
    HANDLE InitializedEvent;

} YORI_SH_DEBUG_THREAD_CONTEXT, *PYORI_SH_DEBUG_THREAD_CONTEXT;

/**
 Pump debug messages from a child process, and when the child process has
 completed execution, extract its environment and apply it to the currently
 executing process.

 @param Context Pointer to the exec context for the child process.

 @return Not meaningful.
 */
DWORD WINAPI
YoriShPumpProcessDebugEventsAndApplyEnvironmentOnExit(
    __in PVOID Context
    )
{
    PYORI_SH_DEBUG_THREAD_CONTEXT ThreadContext = (PYORI_SH_DEBUG_THREAD_CONTEXT)Context;
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext = ThreadContext->ExecContext;
    HANDLE InitializedEvent = ThreadContext->InitializedEvent;
    PYORI_LIBSH_DEBUGGED_CHILD_PROCESS DebuggedChild;
    YORI_STRING OriginalAliases;
    DWORD Err;
    BOOL HaveOriginalAliases;
    BOOL ApplyEnvironment = TRUE;
    BOOL FailedInRedirection = FALSE;

    YoriLibInitEmptyString(&OriginalAliases);
    HaveOriginalAliases = YoriShGetSystemAliasStrings(TRUE, &OriginalAliases);

    Err = YoriLibShCreateProcess(ExecContext, NULL, &FailedInRedirection);
    if (Err != NO_ERROR) {
        LPTSTR ErrText = YoriLibGetWinErrorText(Err);
        if (FailedInRedirection) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Failed to initialize redirection: %s"), ErrText);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CreateProcess failed: %s"), ErrText);
        }
        YoriLibFreeWinErrorText(ErrText);
        YoriLibShCleanupFailedProcessLaunch(ExecContext);
        if (HaveOriginalAliases) {
            YoriLibFreeStringContents(&OriginalAliases);
        }
        YoriLibShDereferenceExecContext(ExecContext, TRUE);
        SetEvent(InitializedEvent);
        return 0;
    }

    YoriLibShCommenceProcessBuffersIfNeeded(ExecContext);
    SetEvent(InitializedEvent);

    while (TRUE) {
        DEBUG_EVENT DbgEvent;
        DWORD dwContinueStatus;

        ZeroMemory(&DbgEvent, sizeof(DbgEvent));
        if (!WaitForDebugEvent(&DbgEvent, INFINITE)) {
            break;
        }

#if YORI_SH_DEBUG_DEBUGGER
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("DbgEvent Pid %x Tid %x Event %x\n"), DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.dwDebugEventCode);
#endif

        dwContinueStatus = DBG_CONTINUE;

        switch(DbgEvent.dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT:
                CloseHandle(DbgEvent.u.CreateProcessInfo.hFile);

                DebuggedChild = YoriLibReferencedMalloc(sizeof(YORI_LIBSH_DEBUGGED_CHILD_PROCESS));
                if (DebuggedChild == NULL) {
                    break;
                }

                ZeroMemory(DebuggedChild, sizeof(YORI_LIBSH_DEBUGGED_CHILD_PROCESS));
                if (!DuplicateHandle(GetCurrentProcess(), DbgEvent.u.CreateProcessInfo.hProcess, GetCurrentProcess(), &DebuggedChild->hProcess, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
                    YoriLibDereference(DebuggedChild);
                    break;
                }
                if (!DuplicateHandle(GetCurrentProcess(), DbgEvent.u.CreateProcessInfo.hThread, GetCurrentProcess(), &DebuggedChild->hInitialThread, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
                    CloseHandle(DebuggedChild->hProcess);
                    YoriLibDereference(DebuggedChild);
                    break;
                }

                DebuggedChild->dwProcessId = DbgEvent.dwProcessId;
                DebuggedChild->dwInitialThreadId = DbgEvent.dwThreadId;
#if YORI_SH_DEBUG_DEBUGGER
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("PROCESS CREATE recorded for Pid %x Tid %x\n"), DbgEvent.dwProcessId, DbgEvent.dwThreadId);
#endif

                YoriLibAppendList(&ExecContext->DebuggedChildren, &DebuggedChild->ListEntry);
                DebuggedChild = NULL;

                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                DebuggedChild = YoriShFindDebuggedChildProcess(&ExecContext->DebuggedChildren, DbgEvent.dwProcessId);
                ASSERT(DebuggedChild != NULL);
                if (DebuggedChild != NULL) {
#if YORI_SH_DEBUG_DEBUGGER
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("PROCESS DELETE recorded for Pid %x\n"), DbgEvent.dwProcessId);
#endif
                    YoriLibRemoveListItem(&DebuggedChild->ListEntry);
                    CloseHandle(DebuggedChild->hProcess);
                    CloseHandle(DebuggedChild->hInitialThread);
                    YoriLibDereference(DebuggedChild);
                    DebuggedChild = NULL;
                }
                break;
            case LOAD_DLL_DEBUG_EVENT:
#if YORI_SH_DEBUG_DEBUGGER
                if (DbgEvent.u.LoadDll.lpImageName != NULL) {
                    SIZE_T BytesReturned;
                    PVOID DllNamePtr;
                    TCHAR DllName[128];

                    if (ReadProcessMemory(ExecContext->hProcess, DbgEvent.u.LoadDll.lpImageName, &DllNamePtr, sizeof(DllNamePtr), &BytesReturned)) {
                        if (ReadProcessMemory(ExecContext->hProcess, DllNamePtr, &DllName, sizeof(DllName), &BytesReturned)) {
                            if (DbgEvent.u.LoadDll.fUnicode) {
                                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Dll loaded: %s\n"), DllName);
                            } else {
                                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Dll loaded: %hs\n"), DllName);
                            }
                        }
                    }
                }
#endif
                CloseHandle(DbgEvent.u.LoadDll.hFile);
                break;
            case EXCEPTION_DEBUG_EVENT:

                //
                //  Wow64 processes throw a breakpoint once 32 bit code starts
                //  running, and the debugger is expected to handle it.  The
                //  two codes are for breakpoint and x86 breakpoint
                //

#if YORI_SH_DEBUG_DEBUGGER
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ExceptionCode %x\n"), DbgEvent.u.Exception.ExceptionRecord.ExceptionCode);
#endif
                dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;

                if (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) {
                    dwContinueStatus = DBG_CONTINUE;
#if _M_MRX000 || _M_ALPHA
                    DebuggedChild = YoriShFindDebuggedChildProcess(&ExecContext->DebuggedChildren, DbgEvent.dwProcessId);
                    ASSERT(DebuggedChild != NULL);

                    //
                    //  MIPS appears to continue from the instruction that
                    //  raised the exception.  We want to skip over it, and
                    //  fortunately we know all instructions are 4 bytes.  We
                    //  only do this for the initial breakpoint, which is on
                    //  the initial thread; other threads would crash the
                    //  process if the debugger wasn't here, so let it die.
                    //

                    if (DbgEvent.dwThreadId == DebuggedChild->dwInitialThreadId) {
                        CONTEXT ThreadContext;
                        ZeroMemory(&ThreadContext, sizeof(ThreadContext));
                        ThreadContext.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
                        GetThreadContext(DebuggedChild->hInitialThread, &ThreadContext);
                        ThreadContext.Fir += 4;
                        SetThreadContext(DebuggedChild->hInitialThread, &ThreadContext);
                    } else {
                        dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                    }
#endif
                }

                if (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x4000001F) {

                    dwContinueStatus = DBG_CONTINUE;
                }

                break;
        }

        if (DbgEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT &&
            DbgEvent.dwProcessId == ExecContext->dwProcessId) {

            YORI_STRING EnvString;
            YORI_STRING CurrentDirectory;

            //
            //  If the user sent this task the background after starting it,
            //  the environment should not be applied anymore.
            //

            if (!ExecContext->CaptureEnvironmentOnExit) {
                ApplyEnvironment = FALSE;
            }

            if (ApplyEnvironment &&
                YoriShSuckEnv(ExecContext->hProcess, &EnvString, &CurrentDirectory)) {
                YoriShSetEnvironmentStrings(&EnvString);

                YoriLibSetCurrentDirectorySaveDriveCurrentDirectory(&CurrentDirectory);
                YoriLibFreeStringContents(&EnvString);
                YoriLibFreeStringContents(&CurrentDirectory);
            }
        }

        ContinueDebugEvent(DbgEvent.dwProcessId, DbgEvent.dwThreadId, dwContinueStatus);
        if (DbgEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT &&
            DbgEvent.dwProcessId == ExecContext->dwProcessId) {
            break;
        }
    }

    WaitForSingleObject(ExecContext->hProcess, INFINITE);
    if (HaveOriginalAliases) {
        YORI_STRING NewAliases;
        if (ApplyEnvironment &&
            YoriShGetSystemAliasStrings(TRUE, &NewAliases)) {
            YoriShMergeChangedAliasStrings(TRUE, &OriginalAliases, &NewAliases);
            YoriLibFreeStringContents(&NewAliases);
        }
        YoriLibFreeStringContents(&OriginalAliases);
    }

    ExecContext->DebugPumpThreadFinished = TRUE;
    YoriLibShDereferenceExecContext(ExecContext, TRUE);
    return 0;
}


/**
 Initialize the wait context with default values.

 @param WaitContext Pointer to the wait context to initialize.

 @param WaitHandle Indicates the process handle or debug thread handle to
        wait for termination on.
 */
VOID
YoriShInitializeWaitContext(
    __out PYORI_SH_WAIT_INPUT_CONTEXT WaitContext,
    __in HANDLE WaitHandle
    )
{
    YoriLibCancelEnable(FALSE);
    WaitContext->WaitOn[0] = WaitHandle;
    WaitContext->WaitOn[1] = YoriLibCancelGetEvent();
    WaitContext->WaitOn[2] = GetStdHandle(STD_INPUT_HANDLE);
    WaitContext->InputRecords = NULL;
    WaitContext->RecordsAllocated = 0;
    WaitContext->CtrlBCount = 0;
    WaitContext->LoseFocusCount = 0;
    WaitContext->Delay = INFINITE;
}

/**
 Clean up any state allocated within the wait context.

 @param WaitContext Pointer to the wait context.
 */
VOID
YoriShCleanupWaitContext(
    __in PYORI_SH_WAIT_INPUT_CONTEXT WaitContext
    )
{
    if (WaitContext->InputRecords != NULL) {
        YoriLibFree(WaitContext->InputRecords);
    }

    YoriLibCancelIgnore();
}


/**
 Perform a wait, indicating the reason for the wait completion.  This routine
 monitors for console input and determines if a process should be sent to the
 background.

 @param WaitContext Pointer to the context describing the state of the wait.

 @return The reason for terminating the wait.
 */
YORI_SH_WAIT_OUTCOME
YoriShWaitForProcessOrInput(
    __inout PYORI_SH_WAIT_INPUT_CONTEXT WaitContext
    )
{
    DWORD Result;
    DWORD Count;
    DWORD RecordsNeeded;
    DWORD RecordsRead;
    BOOLEAN CtrlBFoundThisPass;
    BOOLEAN LoseFocusFoundThisPass;

    while (TRUE) {

        if (YoriShGlobal.ImplicitSynchronousTaskActive) {
            Result = WaitForMultipleObjectsEx(2, WaitContext->WaitOn, FALSE, WaitContext->Delay, FALSE);
        } else if (WaitContext->Delay == INFINITE) {
            Result = WaitForMultipleObjectsEx(3, WaitContext->WaitOn, FALSE, WaitContext->Delay, FALSE);
        } else {
            Result = WaitForMultipleObjectsEx(2, WaitContext->WaitOn, FALSE, WaitContext->Delay, FALSE);
        }
        if (Result == WAIT_OBJECT_0) {
            return YoriShWaitOutcomeProcessExit;
        }

        //
        //  If the user has hit Ctrl+C or Ctrl+Break, request the process
        //  to clean up gracefully and unwind.  Later on we'll try to kill
        //  all processes in the exec plan, so we don't need to try too hard
        //  at this point.
        //

        if (Result == (WAIT_OBJECT_0 + 1)) {
            return YoriShWaitOutcomeCancel;
        }

        //
        //  If there's no input at all from the console, rewait.
        //

        if (YoriShGlobal.ImplicitSynchronousTaskActive ||
            WaitForSingleObject(WaitContext->WaitOn[2], 0) == WAIT_TIMEOUT) {

            WaitContext->CtrlBCount = 0;
            WaitContext->LoseFocusCount = 0;
            WaitContext->Delay = INFINITE;

            continue;
        }

        //
        //  Check if there's pending input.  If there is, go have a look.
        //

        GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &RecordsNeeded);

        if (RecordsNeeded > WaitContext->RecordsAllocated ||
            WaitContext->InputRecords == NULL) {

            if (WaitContext->InputRecords != NULL) {
                YoriLibFree(WaitContext->InputRecords);
            }

            //
            //  Since the user is only ever adding input, overallocate to see
            //  if we can avoid a few allocations later.
            //

            RecordsNeeded += 10;
            
            if (!YoriLibIsSizeAllocatable(RecordsNeeded * sizeof(INPUT_RECORD))) {
                WaitContext->RecordsAllocated = 0;
                Sleep(50);

                continue;
            }

            WaitContext->InputRecords = YoriLibMalloc((YORI_ALLOC_SIZE_T)RecordsNeeded * sizeof(INPUT_RECORD));
            if (WaitContext->InputRecords == NULL) {
                WaitContext->RecordsAllocated = 0;
                Sleep(50);

                continue;
            }

            WaitContext->RecordsAllocated = RecordsNeeded;
        }

        //
        //  Conceptually, the user is interacting with another process, so only
        //  peek at the input and try to leave it alone.  If we see a Ctrl+B,
        //  and the foreground process isn't paying any attention and leaves it
        //  in the input buffer for three passes, we may as well assume it was
        //  for us.
        //
        //  Leave all the input in the buffer so we can catch it later.
        //

        if (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), WaitContext->InputRecords, WaitContext->RecordsAllocated, &RecordsRead) && RecordsRead > 0) {

            CtrlBFoundThisPass = FALSE;
            LoseFocusFoundThisPass = FALSE;

            for (Count = 0; Count < RecordsRead; Count++) {

                if (WaitContext->InputRecords[Count].EventType == KEY_EVENT &&
                    WaitContext->InputRecords[Count].Event.KeyEvent.bKeyDown &&
                    WaitContext->InputRecords[Count].Event.KeyEvent.wVirtualKeyCode == 'B') {

                    DWORD CtrlMask;
                    CtrlMask = WaitContext->InputRecords[Count].Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED);
                    if (CtrlMask == RIGHT_CTRL_PRESSED || CtrlMask == LEFT_CTRL_PRESSED) {
                        CtrlBFoundThisPass = TRUE;
                        break;
                    }
                } else if (WaitContext->InputRecords[Count].EventType == FOCUS_EVENT &&
                           !WaitContext->InputRecords[Count].Event.FocusEvent.bSetFocus) {

                    LoseFocusFoundThisPass = TRUE;
                }
            }

            WaitContext->Delay = 100;

            if (CtrlBFoundThisPass) {
                if (WaitContext->CtrlBCount < 7) {
                    WaitContext->CtrlBCount++;
                    WaitContext->Delay = 20;

                    continue;
                } else {
                    return YoriShWaitOutcomeBackground;
                }
            } else {
                WaitContext->CtrlBCount = 0;
            }

            if (LoseFocusFoundThisPass) {
                if (WaitContext->LoseFocusCount < 7) {
                    WaitContext->LoseFocusCount++;
                    WaitContext->Delay = 20;
                } else {
                    return YoriShWaitOutcomeLoseFocus;
                }
            } else {
                WaitContext->LoseFocusCount = 0;
            }
        }
    }
}


/**
 Wait for a process to terminate.  This is also a good opportunity for Yori
 to monitor for keyboard input that may be better handled by Yori than the
 foreground process.

 @param ExecContext Pointer to the context used to invoke the process, which
        includes information about whether it should be cancelled.
 */
VOID
YoriShWaitForProcessToTerminate(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    HANDLE WaitHandle;
    YORI_SH_WAIT_INPUT_CONTEXT WaitContext;
    YORI_SH_WAIT_OUTCOME Outcome;

    //
    //  If the child isn't running under a debugger, by this point redirection
    //  has been established and then reverted.  This should be dealing with
    //  the original input handle.  If it's running under a debugger, we haven't
    //  started redirecting yet.
    //

    if (ExecContext->CaptureEnvironmentOnExit) {
        YORI_SH_DEBUG_THREAD_CONTEXT ThreadContext;
        DWORD ThreadId;

        //
        //  Because the debugger thread needs to initialize redirection,
        //  start the thread and wait for it to indicate this process is done.
        //
        //  This thread can't reason about the stdin handle and console state
        //  until that is finished.
        //

        ThreadContext.InitializedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (ThreadContext.InitializedEvent == NULL) {
            YoriLibCancelEnable(TRUE);
            return;
        }

        YoriLibShReferenceExecContext(ExecContext);
        ThreadContext.ExecContext = ExecContext;
        ExecContext->hDebuggerThread = CreateThread(NULL, 0, YoriShPumpProcessDebugEventsAndApplyEnvironmentOnExit, &ThreadContext, 0, &ThreadId);
        if (ExecContext->hDebuggerThread == NULL) {
            YoriLibShDereferenceExecContext(ExecContext, TRUE);
            CloseHandle(ThreadContext.InitializedEvent);
            YoriLibCancelEnable(TRUE);
            return;
        }

        WaitForSingleObject(ThreadContext.InitializedEvent, INFINITE);
        CloseHandle(ThreadContext.InitializedEvent);

        WaitHandle = ExecContext->hDebuggerThread;
    } else {
        ASSERT(ExecContext->hProcess != NULL);
        WaitHandle = ExecContext->hProcess;
    }

    YoriShInitializeWaitContext(&WaitContext, WaitHandle);

    while (TRUE) {
        Outcome = YoriShWaitForProcessOrInput(&WaitContext);

        if (Outcome == YoriShWaitOutcomeProcessExit) {

            //
            //  Once the process has completed, if it's outputting to
            //  buffers, wait for the buffers to contain final data.
            //

            if (ExecContext->StdOutType == StdOutTypeBuffer &&
                ExecContext->StdOut.Buffer.ProcessBuffers != NULL)  {

                YoriLibShWaitForProcessBufferToFinalize(ExecContext->StdOut.Buffer.ProcessBuffers);
            }

            if (ExecContext->StdErrType == StdErrTypeBuffer &&
                ExecContext->StdErr.Buffer.ProcessBuffers != NULL) {

                YoriLibShWaitForProcessBufferToFinalize(ExecContext->StdErr.Buffer.ProcessBuffers);
            }
            break;
        }

        //
        //  If the user has hit Ctrl+C or Ctrl+Break, request the process
        //  to clean up gracefully and unwind.  Later on we'll try to kill
        //  all processes in the exec plan, so we don't need to try too hard
        //  at this point.
        //

        if (Outcome == YoriShWaitOutcomeCancel) {
            if (ExecContext->TerminateGracefully && ExecContext->dwProcessId != 0) {
                GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, ExecContext->dwProcessId);
                break;
            } else if (ExecContext->hProcess != NULL) {
                TerminateProcess(ExecContext->hProcess, 1);
                break;
            } else {
                Sleep(50);
            }
        }

        if (Outcome == YoriShWaitOutcomeBackground) {

            //
            //  Attempt to promote the process to a job so it's
            //  possible to wait on later.  Note this only works
            //  because job wait is totally different code so
            //  this doesn't need to consider that the process
            //  might already be in a job.
            //

            if (!ExecContext->CaptureEnvironmentOnExit &&
                ExecContext->hProcess != NULL) {

                if (YoriShCreateNewJob(ExecContext)) {
                    ExecContext->dwProcessId = 0;
                    ExecContext->hProcess = NULL;
                }
            }

            //
            //  If a process is being moved to the background, don't
            //  suck back any environment later when it completes.
            //  Note this is a race condition, since that logic
            //  is occurring on a different thread that is processing
            //  debug messages while this code is running.  For the
            //  same reason though, if process termination is racing
            //  with observing Ctrl+B, either outcome is possible.
            //

            ExecContext->CaptureEnvironmentOnExit = FALSE;

            //
            //  If the taskbar is showing an active task, clear it.
            //  We don't really know if the task failed or succeeded,
            //  but we do know the user is interacting with this
            //  console, so flashing the taskbar a random color is
            //  not helpful or desirable.
            //

            YoriShSetWindowState(YORI_SH_TASK_COMPLETE);

            break;
        }

        if (Outcome == YoriShWaitOutcomeLoseFocus) {

            if (!ExecContext->SuppressTaskCompletion &&
                !ExecContext->TaskCompletionDisplayed &&
                !YoriLibIsExecutableGui(&ExecContext->CmdToExec.ArgV[0])) {

                ExecContext->TaskCompletionDisplayed = TRUE;
                YoriShSetWindowState(YORI_SH_TASK_IN_PROGRESS);
            }
        }
    }

    YoriShCleanupWaitContext(&WaitContext);
}

// vim:sw=4:ts=4:et:
