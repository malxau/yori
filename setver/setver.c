/**
 * @file setver/setver.c
 *
 * Yori shell invoke a child process with an explicit Windows version number.
 *
 * Copyright (c) 2018-2019 Malcolm J. Smith
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
CHAR strSetVerHelpText[] =
        "\n"
        "Runs a child program with an explicit Windows version.\n"
        "\n"
        "SETVER [-license] <version> <command>\n";

/**
 Display usage text to the user.
 */
BOOL
SetVerHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("SetVer %i.%i\n"), SETVER_VER_MAJOR, SETVER_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSetVerHelpText);
    return TRUE;
}

/**
 Context to pass around between functions describing the operation to perform.
 */
typedef struct _SETVER_CONTEXT {

    /**
     The major version to report to applications.
     */
    DWORD AppVerMajor;

    /**
     The minor version to report to applications.
     */
    DWORD AppVerMinor;

    /**
     The build number to report to applications.
     */
    DWORD AppBuildNumber;

    /**
     Pointer to process information state about the initial process launched.
     */
    PPROCESS_INFORMATION ProcessInfo;
} SETVER_CONTEXT, *PSETVER_CONTEXT;

/**
 If TRUE, output information related to messages being processed by the
 debugger.
 */
#define SETVER_DEBUG 0

/**
 Apply the requested OS version into the PEB of an opened process.

 @param Context Pointer to the context describing the OS version to apply.

 @param hProcess Handle to the process to apply the OS version into.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetVerApplyVersionToProcess(
    __in PSETVER_CONTEXT Context,
    __in HANDLE hProcess
    )
{
    BOOL TargetProcess32BitPeb;
    LONG Status;
    PROCESS_BASIC_INFORMATION BasicInfo;
    SIZE_T BytesReturned;
    DWORD dwBytesReturned;

    if (DllNtDll.pNtQueryInformationProcess == NULL) {
        return FALSE;
    }

    TargetProcess32BitPeb = YoriLibDoesProcessHave32BitPeb(hProcess);

    Status = DllNtDll.pNtQueryInformationProcess(hProcess, 0, &BasicInfo, sizeof(BasicInfo), &dwBytesReturned);
    if (Status != 0) {
        return FALSE;
    }

#if SETVER_DEBUG
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Peb at %p, Target %i bit PEB\n"), BasicInfo.PebBaseAddress, TargetProcess32BitPeb?32:64);
#endif

    if (TargetProcess32BitPeb) {
        YORI_LIB_PEB32_NATIVE ProcessPeb;

        if (!ReadProcessMemory(hProcess, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            return FALSE;
        }

        ProcessPeb.OSMajorVersion = Context->AppVerMajor;
        ProcessPeb.OSMinorVersion = Context->AppVerMinor;
        ProcessPeb.OSBuildNumber = (WORD)Context->AppBuildNumber;

        if (!WriteProcessMemory(hProcess, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            return FALSE;
        }
    } else {
        YORI_LIB_PEB64 ProcessPeb;

        if (!ReadProcessMemory(hProcess, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            return FALSE;
        }

        ProcessPeb.OSMajorVersion = Context->AppVerMajor;
        ProcessPeb.OSMinorVersion = Context->AppVerMinor;
        ProcessPeb.OSBuildNumber = (WORD)Context->AppBuildNumber;

        if (!WriteProcessMemory(hProcess, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 Structure to query from NtQueryInformationThread since it is not defined
 by the compilation environment.
 */
typedef struct _YORI_THREAD_BASIC_INFORMATION {

    /**
     If the thread has terminated, the exit code for the thread.
     */
    LONG ExitStatus;

    /**
     Pointer to the native TEB for the thread.
     */
    PVOID TebAddress;

    /**
     A unique process handle.
     */
    HANDLE ProcessHandle;

    /**
     A unique thread handle.
     */
    HANDLE ThreadHandle;

    /**
     The processors the thread should be scheduled on.
     */
    ULONG_PTR AffinityMask;

    /**
     The priority of the thread.
     */
    LONG Priority;

    /**
     The base priority of the thread.
     */
    LONG BasePriority;
} YORI_THREAD_BASIC_INFORMATION, *PYORI_THREAD_BASIC_INFORMATION;

#if defined(_WIN64)

/**
 Apply the requested OS version into the 32 bit PEB of a WOW process.

 @param Context Pointer to the context describing the OS version to apply.

 @param hProcess Handle to the process to apply the OS version into.

 @param hThread Handle to a thread in the process.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetVerApplyVersionToProcessWow(
    __in PSETVER_CONTEXT Context,
    __in HANDLE hProcess,
    __in HANDLE hThread
    )
{
    LONG Status;
    YORI_THREAD_BASIC_INFORMATION BasicInfo;
    SIZE_T BytesReturned;
    DWORD dwBytesReturned;
    PVOID Teb64Address;
    PVOID Teb32Address;
    PVOID Peb32Address;
    YORI_LIB_TEB32 Teb32;
    YORI_LIB_PEB32_WOW ProcessPeb;

    if (DllNtDll.pNtQueryInformationThread == NULL) {
        return FALSE;
    }

    Status = DllNtDll.pNtQueryInformationThread(hThread, 0, &BasicInfo, sizeof(BasicInfo), &dwBytesReturned);
    if (Status != 0) {
        return FALSE;
    }

    Teb64Address = BasicInfo.TebAddress;
    Teb32Address = (PUCHAR)Teb64Address + 0x2000;

#if SETVER_DEBUG
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("64 bit Teb at %p, 32 bit Teb at %p\n"), Teb64Address, Teb32Address);
#endif

    if (!ReadProcessMemory(hProcess, Teb32Address, &Teb32, sizeof(Teb32), &BytesReturned)) {
        return FALSE;
    }

    Peb32Address = (PVOID)(DWORD_PTR)Teb32.Peb32Address;

#if SETVER_DEBUG
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("32 bit Peb at %p\n"), Peb32Address);
#endif

    if (!ReadProcessMemory(hProcess, Peb32Address, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
        return FALSE;
    }

    ProcessPeb.OSMajorVersion = Context->AppVerMajor;
    ProcessPeb.OSMinorVersion = Context->AppVerMinor;
    ProcessPeb.OSBuildNumber = (WORD)Context->AppBuildNumber;

    if (!WriteProcessMemory(hProcess, Peb32Address, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
        return FALSE;
    }

    return TRUE;
}

#endif

#pragma warning(disable: 4152)
#pragma warning(disable: 4054)

/**
 Information about a process where the mini-debugger has observed it be
 launched and has not yet observed termination.  This is used so that the
 version information can be fixed up in all child processes once they
 reach their first instruction to execute.
 */
typedef struct _SETVER_OUTSTANDING_PROCESS {

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
     Pointer within the target process' VA for where execution will commence.
     */
    LPTHREAD_START_ROUTINE StartRoutine;

    /**
     The process identifier for this process.
     */
    DWORD dwProcessId;

    /**
     A saved copy of the first byte in the StartRoutine.  This may be an
     entire instruction or may be part of one, but since the breakpoint is
     a single byte, this byte must be restored before execution can
     resume.
     */
    UCHAR FirstInstruction;

    /**
     TRUE once the StartRoutine function has executed, indicating that the
     breakpoint has fired and been cleared, and no handling should be
     performed for exceptions from the StartRoutine again.
     */
    BOOLEAN ProcessStarted;
} SETVER_OUTSTANDING_PROCESS, *PSETVER_OUTSTANDING_PROCESS;

/**
 Find a process in the list of known processes by its process ID.

 @param ListHead Pointer to the list of known processes.

 @param dwProcessId The process identifier of the process whose information is
        requested.

 @return Pointer to the information block about the child process.
 */
PSETVER_OUTSTANDING_PROCESS
SetVerFindProcess(
    __in PYORI_LIST_ENTRY ListHead,
    __in DWORD dwProcessId
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PSETVER_OUTSTANDING_PROCESS Process;

    ListEntry = NULL;
    while (TRUE) {
        ListEntry = YoriLibGetNextListEntry(ListHead, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        Process = CONTAINING_RECORD(ListEntry, SETVER_OUTSTANDING_PROCESS, ListEntry);
        if (Process->dwProcessId == dwProcessId) {
            return Process;
        }
    }

    return NULL;
}

/**
 Deallocate information about a single known child processes.

 @param Process Pointer to information about a single child process.
 */
VOID
SetVerFreeProcess(
    __in PSETVER_OUTSTANDING_PROCESS Process
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
SetVerFreeAllProcesses(
    __in PYORI_LIST_ENTRY ListHead
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PSETVER_OUTSTANDING_PROCESS Process;

    ListEntry = NULL;
    while (TRUE) {
        ListEntry = YoriLibGetNextListEntry(ListHead, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        Process = CONTAINING_RECORD(ListEntry, SETVER_OUTSTANDING_PROCESS, ListEntry);
        SetVerFreeProcess(Process);
        ListEntry = NULL;
    }
}


/**
 Pump debug events for child processes and complete when the initial process
 has terminated.

 @param Context Pointer to context describing the operations to perform.

 @return TRUE to indicate successful termination of the initial child process,
         FALSE to indicate failure.
 */
BOOL
SetVerPumpDebugEvents(
    __in PSETVER_CONTEXT Context
    )
{
    UCHAR BreakpointInstruction = 0xcc;
    SIZE_T BytesWritten;
    YORI_LIST_ENTRY Processes;
    PSETVER_OUTSTANDING_PROCESS Process;
    BOOL UseProcessBreakpoint;
    BOOL ProcessIsWow;

    YoriLibInitializeListHead(&Processes);

    while(TRUE) {
        DEBUG_EVENT DbgEvent;
        DWORD dwContinueStatus;

        ZeroMemory(&DbgEvent, sizeof(DbgEvent));
        if (!WaitForDebugEvent(&DbgEvent, INFINITE)) {
            break;
        }

#if SETVER_DEBUG
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("DbgEvent Pid %x Tid %x Event %x\n"), DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.dwDebugEventCode);
#endif

        dwContinueStatus = DBG_CONTINUE;

        switch(DbgEvent.dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT:

                Process = YoriLibMalloc(sizeof(SETVER_OUTSTANDING_PROCESS));
                if (Process == NULL) {
                    break;
                }
                
                ZeroMemory(Process, sizeof(SETVER_OUTSTANDING_PROCESS));
                DuplicateHandle(GetCurrentProcess(), DbgEvent.u.CreateProcessInfo.hProcess, GetCurrentProcess(), &Process->hProcess, 0, FALSE, DUPLICATE_SAME_ACCESS);
                DuplicateHandle(GetCurrentProcess(), DbgEvent.u.CreateProcessInfo.hThread, GetCurrentProcess(), &Process->hInitialThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
                Process->StartRoutine = DbgEvent.u.CreateProcessInfo.lpStartAddress;
                Process->dwProcessId = DbgEvent.dwProcessId;

#if SETVER_DEBUG
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("StartRoutine of Pid %x at %p\n"), DbgEvent.dwProcessId, Process->StartRoutine);
#endif

                //
                //  On x86/amd64, set a breakpoint at the entrypoint code
                //  and resume execution.  We'll fix up the versions over
                //  there.  On any other architecture, fix up the versions
                //  now and hope things go well.  We can't do this if the
                //  OS doesn't have Wow64GetThreadContext and the child
                //  process is 32 bit, since we have no way to process the
                //  breakpoints in the child.
                //

#if defined(_M_AMD64) || defined(_M_IX86)
                UseProcessBreakpoint = TRUE;
#else
                UseProcessBreakpoint = FALSE;
#endif
                ProcessIsWow = FALSE;
                if (DllKernel32.pIsWow64Process != NULL) {
                    if (DllKernel32.pIsWow64Process(Process->hProcess, &ProcessIsWow) && ProcessIsWow) {
                        if (DllKernel32.pWow64GetThreadContext == NULL ||
                            DllKernel32.pWow64SetThreadContext == NULL) {
                            UseProcessBreakpoint = FALSE;
                        }
                    }

                }

#if defined(_M_AMD64) || defined(_M_IX86)
                if (UseProcessBreakpoint) {
                    ReadProcessMemory(Process->hProcess, (LPVOID)Process->StartRoutine, &Process->FirstInstruction, sizeof(Process->FirstInstruction), &BytesWritten);
                    WriteProcessMemory(Process->hProcess, (LPVOID)Process->StartRoutine, &BreakpointInstruction, sizeof(BreakpointInstruction), &BytesWritten);
                    FlushInstructionCache(Process->hProcess, Process->StartRoutine, 1);
                }
#endif

                if (!UseProcessBreakpoint) {
                    if (!SetVerApplyVersionToProcess(Context, Process->hProcess)) {
                        break;
                    }
#if defined(_WIN64)
                    if (ProcessIsWow) {
                        if (!SetVerApplyVersionToProcessWow(Context, Process->hProcess, Process->hInitialThread)) {
                            break;
                        }
                    }
#endif
                    Process->ProcessStarted = TRUE;
                }

                YoriLibAppendList(&Processes, &Process->ListEntry);

                CloseHandle(DbgEvent.u.CreateProcessInfo.hFile);
                break;
            case EXIT_PROCESS_DEBUG_EVENT:
                Process = SetVerFindProcess(&Processes, DbgEvent.dwProcessId);
                ASSERT(Process != NULL);
                if (Process != NULL) {
                    SetVerFreeProcess(Process);
                }
                break;
            case LOAD_DLL_DEBUG_EVENT:
#if SETVER_DEBUG
                if (DbgEvent.u.LoadDll.lpImageName != NULL && DbgEvent.dwProcessId == Context->ProcessInfo->dwProcessId) {
                    SIZE_T BytesReturned;
                    PVOID DllNamePtr;
                    TCHAR DllName[128];

                    if (ReadProcessMemory(Context->ProcessInfo->hProcess, DbgEvent.u.LoadDll.lpImageName, &DllNamePtr, sizeof(DllNamePtr), &BytesReturned)) {
                        if (ReadProcessMemory(Context->ProcessInfo->hProcess, DllNamePtr, &DllName, sizeof(DllName), &BytesReturned)) {
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

                dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                if (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) {

                    dwContinueStatus = DBG_CONTINUE;

#if defined(_M_AMD64) || defined(_M_IX86)
                    Process = SetVerFindProcess(&Processes, DbgEvent.dwProcessId);
                    ASSERT(Process != NULL);
                    if (Process != NULL &&
                        DbgEvent.u.Exception.ExceptionRecord.ExceptionAddress == Process->StartRoutine &&
                        !Process->ProcessStarted) {


                        CONTEXT ThreadContext;
                        ZeroMemory(&ThreadContext, sizeof(ThreadContext));

                        ThreadContext.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
                        GetThreadContext(Process->hInitialThread, &ThreadContext);
#if defined(_M_AMD64)
#if SETVER_DEBUG
                        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("rip %p\n"), ThreadContext.Rip);
#endif
                        ThreadContext.Rip = (DWORD_PTR)(LPVOID)Process->StartRoutine;
#else
#if SETVER_DEBUG
                        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("eip %p\n"), ThreadContext.Eip);
#endif
                        ThreadContext.Eip = (DWORD_PTR)(LPVOID)Process->StartRoutine;
#endif
                        
                        WriteProcessMemory(Process->hProcess, (LPVOID)Process->StartRoutine, &Process->FirstInstruction, sizeof(Process->FirstInstruction), &BytesWritten);
                        FlushInstructionCache(Process->hProcess, Process->StartRoutine, 1);
                        SetThreadContext(Process->hInitialThread, &ThreadContext);
                        Process->ProcessStarted = TRUE;

                        if (!SetVerApplyVersionToProcess(Context, Process->hProcess)) {
                            break;
                        }
                    }
#endif
                }

                if (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x4000001F) {

                    dwContinueStatus = DBG_CONTINUE;
#if defined(_M_AMD64)
                    Process = SetVerFindProcess(&Processes, DbgEvent.dwProcessId);
                    ASSERT(Process != NULL);
                    if (Process != NULL &&
                        DllKernel32.pWow64GetThreadContext != NULL &&
                        DllKernel32.pWow64SetThreadContext != NULL &&
                        DbgEvent.u.Exception.ExceptionRecord.ExceptionAddress == Process->StartRoutine &&
                        !Process->ProcessStarted) {

                        if (DbgEvent.u.Exception.ExceptionRecord.ExceptionAddress == Process->StartRoutine) {
                            YORI_LIB_WOW64_CONTEXT ThreadContext;
                            ZeroMemory(&ThreadContext, sizeof(ThreadContext));
    
                            ThreadContext.ContextFlags = YORI_WOW64_CONTEXT_CONTROL | YORI_WOW64_CONTEXT_INTEGER;
                            DllKernel32.pWow64GetThreadContext(Process->hInitialThread, &ThreadContext);
#if SETVER_DEBUG
                            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("eip %p\n"), ThreadContext.Eip);
#endif
                            ThreadContext.Eip = (DWORD)(DWORD_PTR)(LPVOID)Process->StartRoutine;
                            DllKernel32.pWow64SetThreadContext(Process->hInitialThread, &ThreadContext);
                            WriteProcessMemory(Process->hProcess, (LPVOID)Process->StartRoutine, &Process->FirstInstruction, sizeof(Process->FirstInstruction), &BytesWritten);
                            FlushInstructionCache(Process->hProcess, Process->StartRoutine, 1);
                            Process->ProcessStarted = TRUE;
                        }
                        
                        if (!SetVerApplyVersionToProcess(Context, Process->hProcess)) {
                            break;
                        }
                        if (!SetVerApplyVersionToProcessWow(Context, Process->hProcess, Process->hInitialThread)) {
                            break;
                        }
                    }
#endif
                }


#if SETVER_DEBUG
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ExceptionCode %x Address %p ContinueStatus %x\n"), DbgEvent.u.Exception.ExceptionRecord.ExceptionCode, DbgEvent.u.Exception.ExceptionRecord.ExceptionAddress, dwContinueStatus);
#endif

                break;
        }

        ContinueDebugEvent(DbgEvent.dwProcessId, DbgEvent.dwThreadId, dwContinueStatus);
        if (DbgEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT &&
            DbgEvent.dwProcessId == Context->ProcessInfo->dwProcessId) {
            break;
        }
    }

    SetVerFreeAllProcesses(&Processes);

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the setver builtin command.
 */
#define ENTRYPOINT YoriCmd_SETVER
#else
/**
 The main entrypoint for the setver standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the setver cmdlet.

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
    YORI_STRING CmdLine;
    YORI_STRING Executable;
    PYORI_STRING ChildArgs;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    DWORD ExitCode;
    BOOL ArgumentUnderstood;
    DWORD StartArg = 0;
    DWORD AppArg;
    DWORD i;
    YORI_STRING Arg;
    YORI_STRING WinVer;
    LONGLONG llTemp;
    DWORD CharsConsumed;
    SETVER_CONTEXT Context;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SetVerHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2019"));
                return EXIT_SUCCESS;
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("setver: missing argument\n"));
        return EXIT_FAILURE;
    }

    memcpy(&WinVer, &ArgV[StartArg], sizeof(YORI_STRING));

    AppArg = StartArg + 1;
    if (AppArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("setver: missing argument\n"));
        return EXIT_FAILURE;
    }

    Context.AppVerMajor = 6;
    Context.AppVerMinor = 1;
    Context.AppBuildNumber = 7600;

    if (YoriLibStringToNumber(&WinVer, FALSE, &llTemp, &CharsConsumed)) {
        Context.AppVerMajor = (DWORD)llTemp;
        WinVer.LengthInChars -= CharsConsumed;
        WinVer.StartOfString += CharsConsumed;
        if (WinVer.LengthInChars > 0) {
            WinVer.LengthInChars -= 1;
            WinVer.StartOfString += 1;
            if (YoriLibStringToNumber(&WinVer, FALSE, &llTemp, &CharsConsumed)) {
                Context.AppVerMinor = (DWORD)llTemp;
                WinVer.LengthInChars -= CharsConsumed;
                WinVer.StartOfString += CharsConsumed;
                if (WinVer.LengthInChars > 0) {
                    WinVer.LengthInChars -= 1;
                    WinVer.StartOfString += 1;
                    if (YoriLibStringToNumber(&WinVer, FALSE, &llTemp, &CharsConsumed)) {
                        Context.AppBuildNumber = (DWORD)llTemp;
                    }
                }
            }
        }
    }

    YoriLibInitEmptyString(&Executable);
    if (!YoriLibLocateExecutableInPath(&ArgV[AppArg], NULL, NULL, &Executable) ||
        Executable.LengthInChars == 0) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("setver: unable to find executable\n"));
        return EXIT_FAILURE;
    }

    ChildArgs = YoriLibMalloc((ArgC - AppArg) * sizeof(YORI_STRING));
    if (ChildArgs == NULL) {
        YoriLibFreeStringContents(&Executable);
        return EXIT_FAILURE;
    }

    memcpy(&ChildArgs[0], &Executable, sizeof(YORI_STRING));
    if (AppArg + 1 < ArgC) {
        memcpy(&ChildArgs[1], &ArgV[AppArg + 1], (ArgC - AppArg - 1) * sizeof(YORI_STRING));
    }

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - AppArg, ChildArgs, TRUE, &CmdLine)) {
        YoriLibFreeStringContents(&Executable);
        YoriLibFree(ChildArgs);
        return EXIT_FAILURE;
    }

    ASSERT(YoriLibIsStringNullTerminated(&CmdLine));

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, DEBUG_PROCESS, NULL, NULL, &StartupInfo, &ProcessInfo)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("setver: execution failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&Executable);
        YoriLibFreeStringContents(&CmdLine);
        YoriLibFree(ChildArgs);
        return EXIT_FAILURE;
    }

    YoriLibFreeStringContents(&Executable);
    YoriLibFreeStringContents(&CmdLine);
    YoriLibFree(ChildArgs);

    Context.ProcessInfo = &ProcessInfo;

    if (!SetVerPumpDebugEvents(&Context)) {
        return EXIT_FAILURE;
    }

    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);

    return ExitCode;
}

// vim:sw=4:ts=4:et:
