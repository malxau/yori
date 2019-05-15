/**
 * @file sh/exec.c
 *
 * Yori shell execute external program
 *
 * Copyright (c) 2017-2019 Malcolm J. Smith
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
 Capture the current handles used for stdin/stdout/stderr.

 @param RedirectContext Pointer to the block to capture current state into.
 */
VOID
YoriShCaptureRedirectContext(
    __out PYORI_SH_PREVIOUS_REDIRECT_CONTEXT RedirectContext
    )
{
    RedirectContext->ResetInput = FALSE;
    RedirectContext->ResetOutput = FALSE;
    RedirectContext->ResetError = FALSE;
    RedirectContext->StdErrAndOutSame = FALSE;

    //
    //  Always duplicate as noninherited.  If we don't override one of the
    //  existing stdin/stdout/stderr values then the original inheritance
    //  is still in effect.  These are only used to restore our process.
    //

    RedirectContext->StdInput = GetStdHandle(STD_INPUT_HANDLE);
    RedirectContext->StdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    RedirectContext->StdError = GetStdHandle(STD_ERROR_HANDLE);
}

/**
 Revert the redirection context previously put in place by a call to
 @ref YoriShInitializeRedirection.

 @param PreviousRedirectContext The context to revert to.
 */
VOID
YoriShRevertRedirection(
    __in PYORI_SH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext
    )
{
    YORI_SH_PREVIOUS_REDIRECT_CONTEXT CurrentRedirectContext;

    SetConsoleCtrlHandler(NULL, TRUE);

    YoriShCaptureRedirectContext(&CurrentRedirectContext);

    if (PreviousRedirectContext->ResetInput) {
        SetStdHandle(STD_INPUT_HANDLE, PreviousRedirectContext->StdInput);
        CloseHandle(CurrentRedirectContext.StdInput);
    }

    if (PreviousRedirectContext->ResetOutput) {
        SetStdHandle(STD_OUTPUT_HANDLE, PreviousRedirectContext->StdOutput);
        CloseHandle(CurrentRedirectContext.StdOutput);
    }

    if (PreviousRedirectContext->ResetError) {
        SetStdHandle(STD_ERROR_HANDLE, PreviousRedirectContext->StdError);
        if (!PreviousRedirectContext->StdErrAndOutSame) {
            CloseHandle(CurrentRedirectContext.StdError);
        }
    }
}

/**
 Temporarily set this process to have the same stdin/stdout/stderr as a
 a program that it intends to launch.  Keep information about the current
 stdin/stdout/stderr in PreviousRedirectContext so that it can be restored
 with a later call to @ref YoriShRevertRedirection.  If any error occurs,
 the Win32 error code is returned and redirection is restored to its
 original state.

 @param ExecContext The context of the program whose stdin/stdout/stderr
        should be initialized.

 @param PrepareForBuiltIn If TRUE, leave Ctrl+C handling suppressed.  The
        builtin program is free to enable it if it has long running
        processing to perform, and it can install its own handler.

 @param PreviousRedirectContext On successful completion, this is populated
        with the current stdin/stdout/stderr information so that it can
        be restored later.

 @return ERROR_SUCCESS to indicate that redirection has been completely
         initialized, or a Win32 error code indicating if an error has
         occurred.
 */
DWORD
YoriShInitializeRedirection(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext,
    __in BOOL PrepareForBuiltIn,
    __out PYORI_SH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext
    )
{
    SECURITY_ATTRIBUTES InheritHandle;
    HANDLE Handle;
    DWORD Error;

    ZeroMemory(&InheritHandle, sizeof(InheritHandle));
    InheritHandle.nLength = sizeof(InheritHandle);
    InheritHandle.bInheritHandle = TRUE;

    YoriShCaptureRedirectContext(PreviousRedirectContext);

    if (!PrepareForBuiltIn) {
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        SetConsoleCtrlHandler(NULL, FALSE);
    }

    Error = ERROR_SUCCESS;

    if (ExecContext->StdInType == StdInTypeFile) {
        Handle = CreateFile(ExecContext->StdIn.File.FileName.StartOfString,
                            GENERIC_READ,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            PreviousRedirectContext->ResetInput = TRUE;
            SetStdHandle(STD_INPUT_HANDLE, Handle);
        } else {
            Error = GetLastError();
        }
    } else if (ExecContext->StdInType == StdInTypeNull) {
        Handle = CreateFile(_T("NUL"),
                            GENERIC_READ,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            PreviousRedirectContext->ResetInput = TRUE;
            SetStdHandle(STD_INPUT_HANDLE, Handle);
        } else {
            Error = GetLastError();
        }
    } else if (ExecContext->StdInType == StdInTypePipe) {
        if (ExecContext->StdIn.Pipe.PipeFromPriorProcess != NULL) {

            YoriLibMakeInheritableHandle(ExecContext->StdIn.Pipe.PipeFromPriorProcess,
                                         &ExecContext->StdIn.Pipe.PipeFromPriorProcess);

            PreviousRedirectContext->ResetInput = TRUE;
            SetStdHandle(STD_INPUT_HANDLE, ExecContext->StdIn.Pipe.PipeFromPriorProcess);
            ExecContext->StdIn.Pipe.PipeFromPriorProcess = NULL;
        }
    }

    if (Error != ERROR_SUCCESS) {
        YoriShRevertRedirection(PreviousRedirectContext);
        return Error;
    }


    if (ExecContext->StdOutType == StdOutTypeOverwrite) {
        Handle = CreateFile(ExecContext->StdOut.Overwrite.FileName.StartOfString,
                            GENERIC_WRITE,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            PreviousRedirectContext->ResetOutput = TRUE;
            SetStdHandle(STD_OUTPUT_HANDLE, Handle);
        } else {
            Error = GetLastError();
        }

    } else if (ExecContext->StdOutType == StdOutTypeAppend) {
        Handle = CreateFile(ExecContext->StdOut.Append.FileName.StartOfString,
                            FILE_APPEND_DATA|SYNCHRONIZE,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            PreviousRedirectContext->ResetOutput = TRUE;
            SetStdHandle(STD_OUTPUT_HANDLE, Handle);
        } else {
            Error = GetLastError();
        }
    } else if (ExecContext->StdOutType == StdOutTypeNull) {
        Handle = CreateFile(_T("NUL"),
                            GENERIC_WRITE,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            PreviousRedirectContext->ResetOutput = TRUE;
            SetStdHandle(STD_OUTPUT_HANDLE, Handle);
        } else {
            Error = GetLastError();
        }
    } else if (ExecContext->StdOutType == StdOutTypePipe) {
        HANDLE ReadHandle;
        HANDLE WriteHandle;
        if (ExecContext->NextProgram != NULL &&
            ExecContext->NextProgram->StdInType == StdInTypePipe) {

            if (CreatePipe(&ReadHandle, &WriteHandle, NULL, 0)) {

                YoriLibMakeInheritableHandle(WriteHandle, &WriteHandle);

                PreviousRedirectContext->ResetOutput = TRUE;
                SetStdHandle(STD_OUTPUT_HANDLE, WriteHandle);
                ExecContext->NextProgram->StdIn.Pipe.PipeFromPriorProcess = ReadHandle;
            } else {
                Error = GetLastError();
            }
        }
    } else if (ExecContext->StdOutType == StdOutTypeBuffer) {
        HANDLE ReadHandle;
        HANDLE WriteHandle;
        if (CreatePipe(&ReadHandle, &WriteHandle, NULL, 0)) {

            YoriLibMakeInheritableHandle(WriteHandle, &WriteHandle);

            PreviousRedirectContext->ResetOutput = TRUE;
            SetStdHandle(STD_OUTPUT_HANDLE, WriteHandle);
            ExecContext->StdOut.Buffer.PipeFromProcess = ReadHandle;
        } else {
            Error = GetLastError();
        }
    }

    if (Error != ERROR_SUCCESS) {
        YoriShRevertRedirection(PreviousRedirectContext);
        return Error;
    }

    if (ExecContext->StdErrType == StdErrTypeOverwrite) {
        Handle = CreateFile(ExecContext->StdErr.Overwrite.FileName.StartOfString,
                            GENERIC_WRITE,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            PreviousRedirectContext->ResetError = TRUE;
            SetStdHandle(STD_ERROR_HANDLE, Handle);
        } else {
            Error = GetLastError();
        }

    } else if (ExecContext->StdErrType == StdErrTypeAppend) {
        Handle = CreateFile(ExecContext->StdErr.Append.FileName.StartOfString,
                            FILE_APPEND_DATA|SYNCHRONIZE,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            PreviousRedirectContext->ResetError = TRUE;
            SetStdHandle(STD_ERROR_HANDLE, Handle);
        } else {
            Error = GetLastError();
        }
    } else if (ExecContext->StdErrType == StdErrTypeStdOut) {
        PreviousRedirectContext->ResetError = TRUE;
        PreviousRedirectContext->StdErrAndOutSame = TRUE;
        SetStdHandle(STD_ERROR_HANDLE, GetStdHandle(STD_OUTPUT_HANDLE));
    } else if (ExecContext->StdErrType == StdErrTypeNull) {
        Handle = CreateFile(_T("NUL"),
                            GENERIC_WRITE,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            PreviousRedirectContext->ResetError = TRUE;
            SetStdHandle(STD_ERROR_HANDLE, Handle);
        } else {
            Error = GetLastError();
        }
    } else if (ExecContext->StdErrType == StdErrTypeBuffer) {
        HANDLE ReadHandle;
        HANDLE WriteHandle;
        if (CreatePipe(&ReadHandle, &WriteHandle, NULL, 0)) {

            YoriLibMakeInheritableHandle(WriteHandle, &WriteHandle);

            PreviousRedirectContext->ResetError = TRUE;
            SetStdHandle(STD_ERROR_HANDLE, WriteHandle);
            ExecContext->StdErr.Buffer.PipeFromProcess = ReadHandle;
        } else {
            Error = GetLastError();
        }
    }

    if (Error != ERROR_SUCCESS) {
        YoriShRevertRedirection(PreviousRedirectContext);
        return Error;
    }

    return ERROR_SUCCESS;
}

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

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShSuckEnv(
    __in HANDLE ProcessHandle,
    __out PYORI_STRING EnvString
    )
{
    PROCESS_BASIC_INFORMATION BasicInfo;

    LONG Status;
    DWORD dwBytesReturned;
    SIZE_T BytesReturned;
    DWORD EnvironmentBlockPageOffset;
    DWORD CharsToMask;
    PVOID ProcessParamsBlockToRead;
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

        EnvironmentBlockToRead = (PVOID)(ULONG_PTR)ProcessParameters.EnvironmentBlock;
        EnvironmentBlockPageOffset = (YORI_SH_MEMORY_PROTECTION_SIZE - 1) & (DWORD)ProcessParameters.EnvironmentBlock;
    } else {
        YORI_LIB_PROCESS_PARAMETERS64 ProcessParameters;

        if (!ReadProcessMemory(ProcessHandle, ProcessParamsBlockToRead, &ProcessParameters, sizeof(ProcessParameters), &BytesReturned)) {
            return FALSE;
        }

        EnvironmentBlockToRead = (PVOID)(ULONG_PTR)ProcessParameters.EnvironmentBlock;
        EnvironmentBlockPageOffset = (YORI_SH_MEMORY_PROTECTION_SIZE - 1) & (DWORD)ProcessParameters.EnvironmentBlock;
    }

    CharsToMask = EnvironmentBlockPageOffset / sizeof(TCHAR);
#if YORI_SH_DEBUG_DEBUGGER
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("EnvironmentBlock at %p PageOffset %04x CharsToMask %04x\n"), EnvironmentBlockToRead, EnvironmentBlockPageOffset, CharsToMask);
#endif

    //
    //  Attempt to read 64Kb of environment minus the offset from the
    //  page containing the environment.  This occurs because older
    //  versions of Windows don't record how large the block is.  As
    //  a result, this may be truncated, which is acceptable.
    //

    if (!YoriLibAllocateString(EnvString, 32 * 1024 - CharsToMask)) {
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
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Environment contents:\n"));
            YoriLibHexDump(EnvString->StartOfString,
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

    return TRUE;
}

/**
 A wrapper around CreateProcess that sets up redirection and launches a
 process.  The point of this function is that it can be called from the
 main thread or from a debugging thread.

 @param ExecContext Pointer to the ExecContext to attempt to launch via
        CreateProcess.

 @return Win32 error code, meaning zero indicates success.
 */
DWORD
YoriShCreateProcess(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    LPTSTR CmdLine;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    YORI_SH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext;
    DWORD CreationFlags = 0;
    DWORD LastError;

    ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

    CmdLine = YoriShBuildCmdlineFromCmdContext(&ExecContext->CmdToExec, !ExecContext->IncludeEscapesAsLiteral, NULL, NULL);
    if (CmdLine == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (ExecContext->RunOnSecondConsole) {
        CreationFlags |= CREATE_NEW_CONSOLE;
    }

    if (ExecContext->CaptureEnvironmentOnExit) {
        CreationFlags |= DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;
    }

    CreationFlags |= CREATE_NEW_PROCESS_GROUP;

    LastError = YoriShInitializeRedirection(ExecContext, FALSE, &PreviousRedirectContext);
    if (LastError != ERROR_SUCCESS) {
        YoriLibDereference(CmdLine);
        return LastError;
    }

    if (!CreateProcess(NULL, CmdLine, NULL, NULL, TRUE, CreationFlags, NULL, NULL, &StartupInfo, &ProcessInfo)) {
        LastError = GetLastError();
        YoriShRevertRedirection(&PreviousRedirectContext);
        YoriLibDereference(CmdLine);
        return LastError;
    } else {
        YoriShRevertRedirection(&PreviousRedirectContext);
    }

    ASSERT(ExecContext->hProcess == NULL);
    ExecContext->hProcess = ProcessInfo.hProcess;
    ExecContext->hPrimaryThread = ProcessInfo.hThread;
    ExecContext->dwProcessId = ProcessInfo.dwProcessId;

    YoriLibDereference(CmdLine);

    return ERROR_SUCCESS;
}

/**
 Cleanup the ExecContext if the process failed to launch.

 @param ExecContext Pointer to the ExecContext to clean up.
 */
VOID
YoriShCleanupFailedProcessLaunch(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    if (ExecContext->StdOutType == StdOutTypePipe &&
        ExecContext->NextProgram != NULL &&
        ExecContext->NextProgram->StdInType == StdInTypePipe) {

        CloseHandle(ExecContext->NextProgram->StdIn.Pipe.PipeFromPriorProcess);
        ExecContext->NextProgram->StdIn.Pipe.PipeFromPriorProcess = NULL;
        ExecContext->NextProgramType = NextProgramExecNever;
    }
}

/**
 Start buffering process output if the process is configured for it.

 @param ExecContext Pointer to the ExecContext to set up buffers for.
 */
VOID
YoriShCommenceProcessBuffersIfNeeded(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    //
    //  If we're buffering output, start that process now.  If it succeeds,
    //  the pipe is owned by the buffer pump and shouldn't be torn down
    //  when the ExecContext is.
    //

    if (ExecContext->StdOutType == StdOutTypeBuffer ||
        ExecContext->StdErrType == StdErrTypeBuffer) {

        if (ExecContext->StdOut.Buffer.ProcessBuffers != NULL) {
            ASSERT(ExecContext->StdErrType != StdErrTypeBuffer);
            if (YoriShAppendToExistingProcessBuffer(ExecContext)) {
                ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
            } else {
                ExecContext->StdOut.Buffer.ProcessBuffers = NULL;
            }
        } else {
            if (YoriShCreateNewProcessBuffer(ExecContext)) {
                ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
                ExecContext->StdErr.Buffer.PipeFromProcess = NULL;
            }
        }
    }
}

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
    PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext = (PYORI_SH_SINGLE_EXEC_CONTEXT)Context;
    YORI_STRING OriginalAliases;
    DWORD Err;
    BOOL HaveOriginalAliases;
    BOOL ApplyEnvironment = TRUE;

    YoriLibInitEmptyString(&OriginalAliases);
    HaveOriginalAliases = YoriShGetSystemAliasStrings(TRUE, &OriginalAliases);

    Err = YoriShCreateProcess(ExecContext);
    if (Err != NO_ERROR) {
        LPTSTR ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CreateProcess failed (%i): %s"), Err, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriShCleanupFailedProcessLaunch(ExecContext);
        YoriLibFreeStringContents(&OriginalAliases);
        YoriShDereferenceExecContext(ExecContext, TRUE);
        return 0;
    }

    YoriShCommenceProcessBuffersIfNeeded(ExecContext);

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
                }

                if (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x4000001F) {

                    dwContinueStatus = DBG_CONTINUE;
                }

                break;
        }

        if (DbgEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT &&
            DbgEvent.dwProcessId == ExecContext->dwProcessId) {

            YORI_STRING EnvString;

            //
            //  If the user sent this task the background after starting it,
            //  the environment should not be applied anymore.
            //

            if (!ExecContext->CaptureEnvironmentOnExit) {
                ApplyEnvironment = FALSE;
            }

            if (ApplyEnvironment &&
                YoriShSuckEnv(ExecContext->hProcess, &EnvString)) {
                YoriShSetEnvironmentStrings(&EnvString);
                YoriLibFreeStringContents(&EnvString);
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
    YoriShDereferenceExecContext(ExecContext, TRUE);
    return 0;
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
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    HANDLE WaitOn[3];
    DWORD Result;
    PINPUT_RECORD InputRecords = NULL;
    DWORD RecordsNeeded;
    DWORD RecordsAllocated = 0;
    DWORD RecordsRead;
    DWORD Count;
    DWORD CtrlBCount = 0;
    DWORD LoseFocusCount = 0;
    DWORD Delay;
    BOOLEAN CtrlBFoundThisPass;
    BOOLEAN LoseFocusFoundThisPass;

    //
    //  By this point redirection has been established and then reverted.
    //  This should be dealing with the original input handle.
    //

    YoriLibCancelEnable();

    if (ExecContext->CaptureEnvironmentOnExit) {
        DWORD ThreadId;
        YoriShReferenceExecContext(ExecContext);
        ExecContext->hDebuggerThread = CreateThread(NULL, 0, YoriShPumpProcessDebugEventsAndApplyEnvironmentOnExit, ExecContext, 0, &ThreadId);
        if (ExecContext->hDebuggerThread == NULL) {
            YoriShDereferenceExecContext(ExecContext, TRUE);
            YoriLibCancelIgnore();
            return;
        }

        WaitOn[0] = ExecContext->hDebuggerThread;
    } else {
        ASSERT(ExecContext->hProcess != NULL);
        WaitOn[0] = ExecContext->hProcess;
    }
    WaitOn[1] = YoriLibCancelGetEvent();
    WaitOn[2] = GetStdHandle(STD_INPUT_HANDLE);

    Delay = INFINITE;

    while (TRUE) {
        if (Delay == INFINITE) {
            Result = WaitForMultipleObjects(3, WaitOn, FALSE, Delay);
        } else {
            Result = WaitForMultipleObjects(2, WaitOn, FALSE, Delay);
        }
        if (Result == WAIT_OBJECT_0) {

            //
            //  Once the process has completed, if it's outputting to
            //  buffers, wait for the buffers to contain final data.
            //

            if (ExecContext->StdOutType == StdOutTypeBuffer &&
                ExecContext->StdOut.Buffer.ProcessBuffers != NULL)  {

                YoriShWaitForProcessBufferToFinalize(ExecContext->StdOut.Buffer.ProcessBuffers);
            }

            if (ExecContext->StdErrType == StdErrTypeBuffer &&
                ExecContext->StdErr.Buffer.ProcessBuffers != NULL) {

                YoriShWaitForProcessBufferToFinalize(ExecContext->StdErr.Buffer.ProcessBuffers);
            }
            break;
        }

        //
        //  If the user has hit Ctrl+C or Ctrl+Break, request the process
        //  to clean up gracefully and unwind.  Later on we'll try to kill
        //  all processes in the exec plan, so we don't need to try too hard
        //  at this point.  If the process doesn't exist, which happens when
        //  we're trying to launch it as a debuggee, wait a bit to see if it
        //  comes into existence.  If launching it totally failed, the debug
        //  thread will terminate and we'll exit; if it succeeds, we'll get
        //  to cancel it again here.
        //

        if (Result == (WAIT_OBJECT_0 + 1)) {
            if (ExecContext->dwProcessId != 0) {
                GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, ExecContext->dwProcessId);
                break;
            } else {
                Sleep(50);
            }
        }

        if (WaitForSingleObject(WaitOn[2], 0) == WAIT_TIMEOUT) {
            CtrlBCount = 0;
            LoseFocusCount = 0;
            Delay = INFINITE;
            continue;
        }

        //
        //  Check if there's pending input.  If there is, go have a look.
        //

        GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &RecordsNeeded);

        if (RecordsNeeded > RecordsAllocated) {
            if (InputRecords != NULL) {
                YoriLibFree(InputRecords);
            }

            //
            //  Since the user is only ever adding input, overallocate to see
            //  if we can avoid a few allocations later.
            //

            RecordsNeeded += 10;

            InputRecords = YoriLibMalloc(RecordsNeeded * sizeof(INPUT_RECORD));
            if (InputRecords == NULL) {
                Sleep(50);
                continue;
            }

            RecordsAllocated = RecordsNeeded;
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

        if (PeekConsoleInput(GetStdHandle(STD_INPUT_HANDLE), InputRecords, RecordsAllocated, &RecordsRead) && RecordsRead > 0) {

            CtrlBFoundThisPass = FALSE;
            LoseFocusFoundThisPass = FALSE;

            for (Count = 0; Count < RecordsRead; Count++) {

                if (InputRecords[Count].EventType == KEY_EVENT &&
                    InputRecords[Count].Event.KeyEvent.bKeyDown &&
                    InputRecords[Count].Event.KeyEvent.wVirtualKeyCode == 'B') {

                    DWORD CtrlMask;
                    CtrlMask = InputRecords[Count].Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED);
                    if (CtrlMask == RIGHT_CTRL_PRESSED || CtrlMask == LEFT_CTRL_PRESSED) {
                        CtrlBFoundThisPass = TRUE;
                        break;
                    }
                } else if (InputRecords[Count].EventType == FOCUS_EVENT &&
                           !InputRecords[Count].Event.FocusEvent.bSetFocus) {

                    LoseFocusFoundThisPass = TRUE;
                }
            }

            Delay = 100;

            if (CtrlBFoundThisPass) {
                if (CtrlBCount < 3) {
                    CtrlBCount++;
                    Delay = 30;
                    continue;
                } else {

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
            } else {
                CtrlBCount = 0;
            }

            if (LoseFocusFoundThisPass) {
                if (LoseFocusCount < 3) {
                    LoseFocusCount++;
                    Delay = 30;
                } else {
                    if (!ExecContext->SuppressTaskCompletion &&
                        !ExecContext->TaskCompletionDisplayed &&
                        !YoriLibIsExecutableGui(&ExecContext->CmdToExec.ArgV[0])) {

                        ExecContext->TaskCompletionDisplayed = TRUE;
                        YoriShSetWindowState(YORI_SH_TASK_IN_PROGRESS);
                    }
                }
            } else {
                LoseFocusCount = 0;
            }
        }
    }

    if (InputRecords != NULL) {
        YoriLibFree(InputRecords);
    }

    YoriLibCancelIgnore();
}

/**
 Try to launch a single program via ShellExecuteEx rather than CreateProcess.
 This is used when CreateProcess said elevation is needed, and that requires
 UI, so we need to call a function that can support UI...except since Win7
 we're unlikely to actually have any UI because the system components are
 capable of auto-elevating.  But, at least command prompts get to instantiate
 COM, because that's sure going to end well.

 @param ExecContext Pointer to the program to execute.

 @param ProcessInfo On successful completion, the process handle might be
        populated here.  It might not, because shell could decide to
        communicate with an existing process via DDE, and it doesn't tell us
        about the process it was talking to in that case.  In fairness, we
        probably shouldn't wait on a process that we didn't launch.
 
 @return TRUE to indicate success, FALSE on failure.
 */
BOOL
YoriShExecViaShellExecute(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext,
    __out PPROCESS_INFORMATION ProcessInfo
    )
{
    LPTSTR Args = NULL;
    YORI_SH_CMD_CONTEXT ArgContext;
    YORI_SHELLEXECUTEINFO sei;
    YORI_SH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext;
    DWORD LastError;

    YoriLibLoadShell32Functions();

    //
    //  This really shouldn't fail.  We're only here because a process
    //  launch requires elevation, and if UAC is there, ShellExecuteEx had
    //  better be there too.
    //

    ASSERT(DllShell32.pShellExecuteExW != NULL);
    if (DllShell32.pShellExecuteExW == NULL) {
        return FALSE;
    }

    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI|SEE_MASK_NOZONECHECKS|SEE_MASK_UNICODE;
    sei.lpFile = ExecContext->CmdToExec.ArgV[0].StartOfString;
    if (ExecContext->CmdToExec.ArgC > 1) {
        memcpy(&ArgContext, &ExecContext->CmdToExec, sizeof(YORI_SH_CMD_CONTEXT));
        ArgContext.ArgC--;
        ArgContext.ArgV = &ArgContext.ArgV[1];
        ArgContext.ArgContexts = &ArgContext.ArgContexts[1];
        Args = YoriShBuildCmdlineFromCmdContext(&ArgContext, !ExecContext->IncludeEscapesAsLiteral, NULL, NULL);
    }

    sei.lpParameters = Args;
    sei.nShow = SW_SHOWNORMAL;

    ZeroMemory(ProcessInfo, sizeof(PROCESS_INFORMATION));

    LastError = YoriShInitializeRedirection(ExecContext, FALSE, &PreviousRedirectContext);
    if (LastError != ERROR_SUCCESS) {
        if (Args != NULL) {
            YoriLibDereference(Args);
        }
        return FALSE;
    }

    if (!DllShell32.pShellExecuteExW(&sei)) {
        LPTSTR ErrText;
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriShRevertRedirection(&PreviousRedirectContext);
        if (Args != NULL) {
            YoriLibDereference(Args);
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ShellExecuteEx failed (%i): %s"), LastError, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }
    YoriShRevertRedirection(&PreviousRedirectContext);
    if (Args != NULL) {
        YoriLibDereference(Args);
    }

    ProcessInfo->hProcess = sei.hProcess;
    return TRUE;
}

/**
 Execute a single program.  If the execution is synchronous, this routine will
 wait for the program to complete and return its exit code.  If the execution
 is not synchronous, this routine will return without waiting and provide zero
 as a (not meaningful) exit code.

 @param ExecContext The context of a single program to execute.

 @return The exit code of the program, when executed synchronously.
 */
DWORD
YoriShExecuteSingleProgram(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    DWORD ExitCode = 0;
    LPTSTR szExt;
    BOOLEAN ExecProcess = TRUE;
    BOOLEAN LaunchFailed = FALSE;
    BOOLEAN LaunchViaShellExecute = FALSE;

    if (YoriLibIsPathUrl(&ExecContext->CmdToExec.ArgV[0])) {
        LaunchViaShellExecute = TRUE;
        ExecContext->SuppressTaskCompletion = TRUE;
    } else {
        szExt = YoriLibFindRightMostCharacter(&ExecContext->CmdToExec.ArgV[0], '.');
        if (szExt != NULL) {
            YORI_STRING YsExt;

            YoriLibInitEmptyString(&YsExt);
            YsExt.StartOfString = szExt;
            YsExt.LengthInChars = ExecContext->CmdToExec.ArgV[0].LengthInChars - (DWORD)(szExt - ExecContext->CmdToExec.ArgV[0].StartOfString);

            if (YoriLibCompareStringWithLiteralInsensitive(&YsExt, _T(".com")) == 0) {
                if (YoriShExecuteNamedModuleInProc(ExecContext->CmdToExec.ArgV[0].StartOfString, ExecContext, &ExitCode)) {
                    ExecProcess = FALSE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&YsExt, _T(".ys1")) == 0) {
                ExecProcess = FALSE;
                ExitCode = YoriShBuckPass(ExecContext, 1, _T("ys"));
            } else if (YoriLibCompareStringWithLiteralInsensitive(&YsExt, _T(".cmd")) == 0 ||
                       YoriLibCompareStringWithLiteralInsensitive(&YsExt, _T(".bat")) == 0) {
                ExecProcess = FALSE;
                YoriShCheckIfArgNeedsQuotes(&ExecContext->CmdToExec, 0);
                if (ExecContext->WaitForCompletion) {
                    ExecContext->CaptureEnvironmentOnExit = TRUE;
                }
                ExitCode = YoriShBuckPass(ExecContext, 2, _T("cmd.exe"), _T("/c"));
            } else if (YoriLibCompareStringWithLiteralInsensitive(&YsExt, _T(".exe")) != 0) {
                LaunchViaShellExecute = TRUE;
                ExecContext->SuppressTaskCompletion = TRUE;
            }
        }
    }

    if (ExecProcess) {

        if (!LaunchViaShellExecute && !ExecContext->CaptureEnvironmentOnExit) {
            DWORD Err = YoriShCreateProcess(ExecContext);

            if (Err != NO_ERROR) {
                if (Err == ERROR_ELEVATION_REQUIRED) {
                    LaunchViaShellExecute = TRUE;
                } else {
                    LPTSTR ErrText = YoriLibGetWinErrorText(Err);
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CreateProcess failed (%i): %s"), Err, ErrText);
                    YoriLibFreeWinErrorText(ErrText);
                    LaunchFailed = TRUE;
                }
            }
        }

        if (LaunchViaShellExecute) {
            PROCESS_INFORMATION ProcessInfo;

            ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

            if (!YoriShExecViaShellExecute(ExecContext, &ProcessInfo)) {
                LaunchFailed = TRUE;
            } else {
                ExecContext->hProcess = ProcessInfo.hProcess;
                ExecContext->hPrimaryThread = ProcessInfo.hThread;
                ExecContext->dwProcessId = ProcessInfo.dwProcessId;
            }
        }

        if (LaunchFailed) {
            YoriShCleanupFailedProcessLaunch(ExecContext);
            return 1;
        }

        if (!ExecContext->CaptureEnvironmentOnExit) {
            YoriShCommenceProcessBuffersIfNeeded(ExecContext);
        }


        //
        //  We may not have a process handle but still be successful if
        //  ShellExecute decided to interact with an existing process
        //  rather than launch a new one.  This isn't going to be very
        //  common in any interactive shell, and it's clearly going to break
        //  things, but there's not much we can do about it from here.
        //

        if (ExecContext->hProcess != NULL || ExecContext->CaptureEnvironmentOnExit) {
            if (ExecContext->WaitForCompletion) {
                YoriShWaitForProcessToTerminate(ExecContext);
                GetExitCodeProcess(ExecContext->hProcess, &ExitCode);
            } else if (ExecContext->StdOutType != StdOutTypePipe) {
                ASSERT(!ExecContext->CaptureEnvironmentOnExit);
                if (YoriShCreateNewJob(ExecContext, ExecContext->hProcess, ExecContext->dwProcessId)) {
                    ExecContext->dwProcessId = 0;
                    ExecContext->hProcess = NULL;
                }
            }
        }
    }
    return ExitCode;
}

/**
 Cancel an exec plan.  This is invoked after the user hits Ctrl+C and attempts
 to terminate all outstanding processes associated with the request.

 @param ExecPlan The plan to terminate all outstanding processes in.
 */
VOID
YoriShCancelExecPlan(
    __in PYORI_SH_EXEC_PLAN ExecPlan
    )
{
    PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext;

    //
    //  Loop and ask the processes nicely to terminate.
    //

    ExecContext = ExecPlan->FirstCmd;
    while (ExecContext != NULL) {
        if (ExecContext->hProcess != NULL) {
            if (WaitForSingleObject(ExecContext->hProcess, 0) != WAIT_OBJECT_0) {
                if (ExecContext->dwProcessId != 0) {
                    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, ExecContext->dwProcessId);
                }
            }
        }
        ExecContext = ExecContext->NextProgram;
    }

    Sleep(50);

    //
    //  Loop again and ask the processes less nicely to terminate.
    //

    ExecContext = ExecPlan->FirstCmd;
    while (ExecContext != NULL) {
        if (ExecContext->hProcess != NULL) {
            if (WaitForSingleObject(ExecContext->hProcess, 0) != WAIT_OBJECT_0) {
                TerminateProcess(ExecContext->hProcess, 1);
            }
        }
        ExecContext = ExecContext->NextProgram;
    }

    //
    //  Loop waiting for any debugger threads to terminate.  These are
    //  referencing the ExecContext so it's important that they're
    //  terminated before we start freeing them.
    //

    ExecContext = ExecPlan->FirstCmd;
    while (ExecContext != NULL) {
        if (ExecContext->hDebuggerThread != NULL) {
            WaitForSingleObject(ExecContext->hDebuggerThread, INFINITE);
        }
        ExecContext = ExecContext->NextProgram;
    }
}

/**
 Execute a single command by invoking the YORISPEC executable and telling it
 to execute the command string.  This is used when an expression is compound
 but cannot wait (eg. a & b &) such that something has to wait for a to
 finish before executing b but input is supposed to continute immediately.  It
 is also used when a builtin is being executed without waiting, so that long
 running tasks can be builtin to the shell executable and still have non-
 waiting semantics on request.

 @param ExecContext Pointer to the single program string to execute.  This is
        prepended with the YORISPEC executable and executed.
 */
VOID
YoriShExecViaSubshell(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    YORI_STRING PathToYori;

    YoriLibInitEmptyString(&PathToYori);
    if (YoriShAllocateAndGetEnvironmentVariable(_T("YORISPEC"), &PathToYori, NULL)) {

        YoriShGlobal.ErrorLevel = YoriShBuckPass(ExecContext, 2, PathToYori.StartOfString, _T("/ss"));
        YoriLibFreeStringContents(&PathToYori);
        return;
    } else {
        YoriShGlobal.ErrorLevel = EXIT_FAILURE;
    }
}


/**
 Execute an exec plan.  An exec plan has multiple processes, including
 different pipe and redirection operators.  Optionally return the result
 of any output buffered processes in the plan, to facilitate back quotes.

 @param ExecPlan Pointer to the exec plan to execute.

 @param OutputBuffer On successful completion, updated to point to the
        resulting output buffer.
 */
VOID
YoriShExecExecPlan(
    __in PYORI_SH_EXEC_PLAN ExecPlan,
    __out_opt PVOID * OutputBuffer
    )
{
    PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext;
    PVOID PreviouslyObservedOutputBuffer = NULL;
    BOOL ExecutableFound;

    //
    //  If a plan requires executing multiple tasks without waiting, hand the
    //  request to a subshell so we can execute a single thing without
    //  waiting and let it schedule the tasks
    //

    if (OutputBuffer == NULL &&
        ExecPlan->NumberCommands > 1 &&
        !ExecPlan->WaitForCompletion) {

        YoriShExecViaSubshell(&ExecPlan->EntireCmd);
        return;
    }

    ExecContext = ExecPlan->FirstCmd;
    while (ExecContext != NULL) {


        //
        //  If some previous program in the plan has output to a buffer, use
        //  the same buffer for any later program which intends to output to
        //  a buffer.
        //

        if (ExecContext->StdOutType == StdOutTypeBuffer &&
            ExecContext->WaitForCompletion) {

            ExecContext->StdOut.Buffer.ProcessBuffers = PreviouslyObservedOutputBuffer;
        }

        if (YoriLibIsOperationCancelled()) {
            break;
        }

        if (YoriLibIsPathUrl(&ExecContext->CmdToExec.ArgV[0])) {
            YoriShGlobal.ErrorLevel = YoriShExecuteSingleProgram(ExecContext);
        } else {
            if (!YoriShResolveCommandToExecutable(&ExecContext->CmdToExec, &ExecutableFound)) {
                break;
            }

            if (ExecutableFound) {
                YoriShGlobal.ErrorLevel = YoriShExecuteSingleProgram(ExecContext);
            } else if (ExecPlan->NumberCommands == 1 && !ExecPlan->WaitForCompletion) {
                YoriShExecViaSubshell(ExecContext);
                return;
            } else {
                YoriShGlobal.ErrorLevel = YoriShBuiltIn(ExecContext);
            }
        }

        if (ExecContext->TaskCompletionDisplayed) {
            ExecPlan->TaskCompletionDisplayed = TRUE;
        }

        //
        //  If the program output back to a shell owned buffer and we waited
        //  for it to complete, we can use the same buffer for later commands
        //  in the set.
        //

        if (ExecContext->StdOutType == StdOutTypeBuffer &&
            ExecContext->StdOut.Buffer.ProcessBuffers != NULL &&
            ExecContext->WaitForCompletion) {

            PreviouslyObservedOutputBuffer = ExecContext->StdOut.Buffer.ProcessBuffers;
        }

        //
        //  Determine which program to execute next, if any.
        //

        if (ExecContext->NextProgram != NULL) {
            switch(ExecContext->NextProgramType) {
                case NextProgramExecUnconditionally:
                    ExecContext = ExecContext->NextProgram;
                    break;
                case NextProgramExecConcurrently:
                    ExecContext = ExecContext->NextProgram;
                    break;
                case NextProgramExecOnFailure:
                    if (YoriShGlobal.ErrorLevel != 0) {
                        ExecContext = ExecContext->NextProgram;
                    } else {
                        do {
                            ExecContext = ExecContext->NextProgram;
                        } while (ExecContext != NULL && (ExecContext->NextProgramType == NextProgramExecOnFailure || ExecContext->NextProgramType == NextProgramExecConcurrently));
                        if (ExecContext != NULL) {
                            ExecContext = ExecContext->NextProgram;
                        }
                    }
                    break;
                case NextProgramExecOnSuccess:
                    if (YoriShGlobal.ErrorLevel == 0) {
                        ExecContext = ExecContext->NextProgram;
                    } else {
                        do {
                            ExecContext = ExecContext->NextProgram;
                        } while (ExecContext != NULL && (ExecContext->NextProgramType == NextProgramExecOnSuccess || ExecContext->NextProgramType == NextProgramExecConcurrently));
                        if (ExecContext != NULL) {
                            ExecContext = ExecContext->NextProgram;
                        }
                    }
                    break;
                case NextProgramExecNever:
                    ExecContext = NULL;
                    break;
                default:
                    ASSERT(!"YoriShParseCmdContextToExecPlan created a plan that YoriShExecExecPlan doesn't know how to execute");
                    ExecContext = NULL;
                    break;
            }
        } else {
            ExecContext = NULL;
        }
    }

    if (OutputBuffer != NULL) {
        *OutputBuffer = PreviouslyObservedOutputBuffer;
    }

    if (YoriLibIsOperationCancelled()) {
        YoriShCancelExecPlan(ExecPlan);
    }
}

/**
 Execute an expression and capture the output of the entire expression into
 a buffer.  This is used when evaluating backquoted expressions.

 @param Expression Pointer to a string describing the expression to execute.

 @param ProcessOutput On successful completion, populated with the result of
        the expression.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShExecuteExpressionAndCaptureOutput(
    __in PYORI_STRING Expression,
    __out PYORI_STRING ProcessOutput
    )
{
    YORI_SH_EXEC_PLAN ExecPlan;
    PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext;
    YORI_SH_CMD_CONTEXT CmdContext;
    PVOID OutputBuffer;
    DWORD Index;

    //
    //  Parse the expression we're trying to execute.
    //

    if (!YoriShParseCmdlineToCmdContext(Expression, 0, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        return FALSE;
    }

    if (CmdContext.ArgC == 0) {
        YoriShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (!YoriShParseCmdContextToExecPlan(&CmdContext, &ExecPlan, NULL, NULL, NULL)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        YoriShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    //
    //  If we're doing backquote evaluation, set the output back to a 
    //  shell owned buffer, and the process must wait.
    //

    ExecContext = ExecPlan.FirstCmd;
    while (ExecContext != NULL) {

        if (ExecContext->StdOutType == StdOutTypeDefault) {
            ExecContext->StdOutType = StdOutTypeBuffer;

            if (!ExecContext->WaitForCompletion &&
                ExecContext->NextProgramType != NextProgramExecUnconditionally) {

                ExecContext->WaitForCompletion = TRUE;
            }
        }

        ExecContext = ExecContext->NextProgram;
    }

    YoriShExecExecPlan(&ExecPlan, &OutputBuffer);

    YoriLibInitEmptyString(ProcessOutput);
    if (OutputBuffer != NULL) {

        YoriShGetProcessOutputBuffer(OutputBuffer, ProcessOutput);

        //
        //  Truncate any newlines from the output, which tools
        //  frequently emit but are of no value here
        //

        while (ProcessOutput->LengthInChars > 0 &&
               (ProcessOutput->StartOfString[ProcessOutput->LengthInChars - 1] == '\n' ||
                ProcessOutput->StartOfString[ProcessOutput->LengthInChars - 1] == '\r')) {

            ProcessOutput->LengthInChars--;
        }

        //
        //  Convert any remaining newlines to spaces
        //

        for (Index = 0; Index < ProcessOutput->LengthInChars; Index++) {
            if ((ProcessOutput->StartOfString[Index] == '\n' ||
                 ProcessOutput->StartOfString[Index] == '\r')) {

                ProcessOutput->StartOfString[Index] = ' ';
            }
        }
    }

    YoriShFreeExecPlan(&ExecPlan);
    YoriShFreeCmdContext(&CmdContext);

    return TRUE;
}


/**
 Parse and execute all backquotes in an expression, potentially resulting
 in a new expression.  This will internally perform parsing and redirection,
 as well as execute multiple subprocesses as needed.

 @param Expression The string to execute.

 @param ResultingExpression On successful completion, updated to contain
        the final expression to evaluate.  This may be the same as Expression
        if no backquote expansion occurred.

 @return TRUE to indicate it was successfully executed, FALSE otherwise.
 */
BOOL
YoriShExpandBackquotes(
    __in PYORI_STRING Expression,
    __out PYORI_STRING ResultingExpression
    )
{
    YORI_STRING CurrentFullExpression;
    YORI_STRING CurrentExpressionSubset;

    YORI_STRING ProcessOutput;

    YORI_STRING InitialPortion;
    YORI_STRING TrailingPortion;
    YORI_STRING NewFullExpression;

    DWORD CharsInBackquotePrefix;

    YoriLibInitEmptyString(&CurrentFullExpression);
    CurrentFullExpression.StartOfString = Expression->StartOfString;
    CurrentFullExpression.LengthInChars = Expression->LengthInChars;

    while(TRUE) {

        //
        //  MSFIX This logic currently rescans from the beginning.  Should
        //  we only rescan from the end of the previous scan so we don't
        //  create commands that can nest further backticks?
        //

        if (!YoriShFindNextBackquoteSubstring(&CurrentFullExpression, &CurrentExpressionSubset, &CharsInBackquotePrefix)) {
            break;
        }

        if (!YoriShExecuteExpressionAndCaptureOutput(&CurrentExpressionSubset, &ProcessOutput)) {
            break;
        }

        //
        //  Calculate the number of characters from before the first
        //  backquote, the number after the last backquote, and the
        //  number just obtained from the buffer.
        //

        YoriLibInitEmptyString(&InitialPortion);
        YoriLibInitEmptyString(&TrailingPortion);

        InitialPortion.StartOfString = CurrentFullExpression.StartOfString;
        InitialPortion.LengthInChars = (DWORD)(CurrentExpressionSubset.StartOfString - CurrentFullExpression.StartOfString - CharsInBackquotePrefix);

        TrailingPortion.StartOfString = &CurrentFullExpression.StartOfString[InitialPortion.LengthInChars + CurrentExpressionSubset.LengthInChars + 1 + CharsInBackquotePrefix];
        TrailingPortion.LengthInChars = CurrentFullExpression.LengthInChars - InitialPortion.LengthInChars - CurrentExpressionSubset.LengthInChars - 1 - CharsInBackquotePrefix;

        if (!YoriLibAllocateString(&NewFullExpression, InitialPortion.LengthInChars + ProcessOutput.LengthInChars + TrailingPortion.LengthInChars + 1)) {
            YoriLibFreeStringContents(&CurrentFullExpression);
            YoriLibFreeStringContents(&ProcessOutput);
            return FALSE;
        }

        NewFullExpression.LengthInChars = YoriLibSPrintf(NewFullExpression.StartOfString,
                                                         _T("%y%y%y"),
                                                         &InitialPortion,
                                                         &ProcessOutput,
                                                         &TrailingPortion);

        YoriLibFreeStringContents(&CurrentFullExpression);

        memcpy(&CurrentFullExpression, &NewFullExpression, sizeof(YORI_STRING));

        YoriLibFreeStringContents(&ProcessOutput);
    }

    memcpy(ResultingExpression, &CurrentFullExpression, sizeof(YORI_STRING));
    return TRUE;
}

/**
 Parse and execute a command string.  This will internally perform parsing
 and redirection, as well as execute multiple subprocesses as needed.  This
 specific function mainly deals with backquote evaluation, carving the
 expression up into multiple multi-program execution plans, and executing
 each.

 @param Expression The string to execute.

 @return TRUE to indicate it was successfully executed, FALSE otherwise.
 */
BOOL
YoriShExecuteExpression(
    __in PYORI_STRING Expression
    )
{
    YORI_SH_EXEC_PLAN ExecPlan;
    YORI_SH_CMD_CONTEXT CmdContext;
    YORI_STRING CurrentFullExpression;

    //
    //  Expand all backquotes.
    //

    if (!YoriShExpandBackquotes(Expression, &CurrentFullExpression)) {
        return FALSE;
    }

    ASSERT(CurrentFullExpression.StartOfString != Expression->StartOfString || CurrentFullExpression.MemoryToFree == NULL);

    //
    //  Parse the expression we're trying to execute.
    //

    if (!YoriShParseCmdlineToCmdContext(&CurrentFullExpression, 0, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        YoriLibFreeStringContents(&CurrentFullExpression);
        return FALSE;
    }

    if (CmdContext.ArgC == 0) {
        YoriShFreeCmdContext(&CmdContext);
        YoriLibFreeStringContents(&CurrentFullExpression);
        return FALSE;
    }

    if (!YoriShParseCmdContextToExecPlan(&CmdContext, &ExecPlan, NULL, NULL, NULL)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        YoriLibFreeStringContents(&CurrentFullExpression);
        YoriShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriShExecExecPlan(&ExecPlan, NULL);

    YoriShFreeExecPlan(&ExecPlan);
    YoriShFreeCmdContext(&CmdContext);

    YoriLibFreeStringContents(&CurrentFullExpression);

    return TRUE;
}

// vim:sw=4:ts=4:et:
