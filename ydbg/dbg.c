/**
 * @file ydbg/dbg.c
 *
 * Yori shell debug processes
 *
 * Copyright (c) 2018-2020 Malcolm J. Smith
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
        "YDBG [-license] [-c <file>] [-d <pid> <file>] [-k <file>] [-ks <pid> <file>]\n"
        "\n"
        "   -c             Dump memory from kernel and user processes to a file\n"
        "   -d             Dump memory from a process to a file\n"
        "   -k             Dump memory from kernel to a file\n"
        "   -ks            Dump memory from kernel stacks associated with a process to a file\n";

/**
 Display usage text to the user.
 */
BOOL
YDbgHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("YDbg %i.%02i\n"), YDBG_VER_MAJOR, YDBG_VER_MINOR);
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
 The set of operations supported by this program.
 */
typedef enum _YDBG_OP {
    YDbgOperationNone = 0,
    YDbgOperationProcessDump = 1,
    YDbgOperationKernelDump = 2,
    YDbgOperationCompleteDump = 3,
    YDbgOperationProcessKernelStacks = 4,
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

    Op = YDbgOperationNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YDbgHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2020"));
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
    }

    return ExitResult;
}

// vim:sw=4:ts=4:et:
