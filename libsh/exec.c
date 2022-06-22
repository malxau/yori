/**
 * @file /libsh/exec.c
 *
 * Yori shell helper routines for executing programs
 *
 * Copyright (c) 2017-2021 Malcolm J. Smith
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
#include <yorish.h>

/**
 Capture the current handles used for stdin/stdout/stderr.

 @param RedirectContext Pointer to the block to capture current state into.
 */
VOID
YoriLibShCaptureRedirectContext(
    __out PYORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT RedirectContext
    )
{
    RedirectContext->ResetInput = FALSE;
    RedirectContext->ResetOutput = FALSE;
    RedirectContext->ResetError = FALSE;
    RedirectContext->StdErrRedirectsToStdOut = FALSE;
    RedirectContext->StdOutRedirectsToStdErr = FALSE;

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
 @ref YoriLibShInitializeRedirection.

 @param PreviousRedirectContext The context to revert to.
 */
VOID
YoriLibShRevertRedirection(
    __in PYORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext
    )
{
    YORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT CurrentRedirectContext;

    YoriLibCancelInheritedIgnore();

    YoriLibShCaptureRedirectContext(&CurrentRedirectContext);

    if (PreviousRedirectContext->ResetInput) {
        SetStdHandle(STD_INPUT_HANDLE, PreviousRedirectContext->StdInput);
        CloseHandle(CurrentRedirectContext.StdInput);
    }

    if (PreviousRedirectContext->ResetOutput) {
        SetStdHandle(STD_OUTPUT_HANDLE, PreviousRedirectContext->StdOutput);
        if (!PreviousRedirectContext->StdOutRedirectsToStdErr) {
            CloseHandle(CurrentRedirectContext.StdOutput);
        }
    }

    if (PreviousRedirectContext->ResetError) {
        SetStdHandle(STD_ERROR_HANDLE, PreviousRedirectContext->StdError);
        if (!PreviousRedirectContext->StdErrRedirectsToStdOut) {
            CloseHandle(CurrentRedirectContext.StdError);
        }
    }
}

/**
 Temporarily set this process to have the same stdin/stdout/stderr as a
 a program that it intends to launch.  Keep information about the current
 stdin/stdout/stderr in PreviousRedirectContext so that it can be restored
 with a later call to @ref YoriLibShRevertRedirection.  If any error occurs,
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
YoriLibShInitializeRedirection(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __in BOOL PrepareForBuiltIn,
    __out PYORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext
    )
{
    SECURITY_ATTRIBUTES InheritHandle;
    HANDLE Handle;
    DWORD Error;

    ZeroMemory(&InheritHandle, sizeof(InheritHandle));
    InheritHandle.nLength = sizeof(InheritHandle);
    InheritHandle.bInheritHandle = TRUE;

    YoriLibShCaptureRedirectContext(PreviousRedirectContext);

    //
    //  MSFIX: What this is doing is allowing child processes to see Ctrl+C,
    //  which is wrong, because we only want the foreground process to see it
    //  which implies handling it in the shell.  Unfortunately
    //  GenerateConsoleCtrlEvent has a nasty bug where it can only safely be
    //  called on console processes remaining in this console, which will
    //  require more processing to determine.
    //

    if (!PrepareForBuiltIn) {
        YoriLibSetInputConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        YoriLibCancelInheritedProcess();
    }

    Error = ERROR_SUCCESS;

    if (ExecContext->StdInType == StdInTypeFile) {
        Handle = CreateFile(ExecContext->StdIn.File.FileName.StartOfString,
                            GENERIC_READ,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
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
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            PreviousRedirectContext->ResetInput = TRUE;
            SetStdHandle(STD_INPUT_HANDLE, Handle);
        } else {
            Error = GetLastError();
        }
    } else if (ExecContext->StdInType == StdInTypePipe) {
        if (ExecContext->StdIn.Pipe.PipeFromPriorProcess != NULL) {

            HANDLE NewHandle;

            if (!YoriLibMakeInheritableHandle(ExecContext->StdIn.Pipe.PipeFromPriorProcess,
                                              &NewHandle)) {

                Error = GetLastError();
            } else {
                PreviousRedirectContext->ResetInput = TRUE;
                SetStdHandle(STD_INPUT_HANDLE, NewHandle);
                ExecContext->StdIn.Pipe.PipeFromPriorProcess = NULL;
            }

        } else {
            HANDLE ReadHandle;
            HANDLE WriteHandle;
            HANDLE NewHandle;

            if (CreatePipe(&ReadHandle, &WriteHandle, NULL, 0)) {
                if (!YoriLibMakeInheritableHandle(ReadHandle, &NewHandle)) {
                    Error = GetLastError();
                    CloseHandle(ReadHandle);
                    CloseHandle(WriteHandle);
                } else {
                    PreviousRedirectContext->ResetInput = TRUE;
                    SetStdHandle(STD_INPUT_HANDLE, NewHandle);
                    CloseHandle(WriteHandle);
                }
            } else {
                Error = GetLastError();
            }
        }
    }

    if (Error != ERROR_SUCCESS) {
        YoriLibShRevertRedirection(PreviousRedirectContext);
        return Error;
    }


    if (ExecContext->StdOutType == StdOutTypeOverwrite) {
        Handle = CreateFile(ExecContext->StdOut.Overwrite.FileName.StartOfString,
                            GENERIC_WRITE,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
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
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
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
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
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
        HANDLE NewHandle;

        if (ExecContext->NextProgram != NULL &&
            ExecContext->NextProgram->StdInType == StdInTypePipe) {

            if (CreatePipe(&ReadHandle, &WriteHandle, NULL, 0)) {

                if (!YoriLibMakeInheritableHandle(WriteHandle, &NewHandle)) {
                    Error = GetLastError();
                    CloseHandle(ReadHandle);
                    CloseHandle(WriteHandle);
                } else {
                    PreviousRedirectContext->ResetOutput = TRUE;
                    SetStdHandle(STD_OUTPUT_HANDLE, NewHandle);
                    ExecContext->NextProgram->StdIn.Pipe.PipeFromPriorProcess = ReadHandle;
                }
            } else {
                Error = GetLastError();
            }
        }
    } else if (ExecContext->StdOutType == StdOutTypeBuffer) {
        HANDLE ReadHandle;
        HANDLE WriteHandle;
        HANDLE NewHandle;
        if (CreatePipe(&ReadHandle, &WriteHandle, NULL, 0)) {

            if (!YoriLibMakeInheritableHandle(WriteHandle, &NewHandle)) {
                Error = GetLastError();
                CloseHandle(ReadHandle);
                CloseHandle(WriteHandle);
            } else {
                PreviousRedirectContext->ResetOutput = TRUE;
                SetStdHandle(STD_OUTPUT_HANDLE, NewHandle);
                ExecContext->StdOut.Buffer.PipeFromProcess = ReadHandle;
            }
        } else {
            Error = GetLastError();
        }
    }

    if (Error != ERROR_SUCCESS) {
        YoriLibShRevertRedirection(PreviousRedirectContext);
        return Error;
    }

    if (ExecContext->StdErrType == StdErrTypeOverwrite) {
        Handle = CreateFile(ExecContext->StdErr.Overwrite.FileName.StartOfString,
                            GENERIC_WRITE,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
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
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            PreviousRedirectContext->ResetError = TRUE;
            SetStdHandle(STD_ERROR_HANDLE, Handle);
        } else {
            Error = GetLastError();
        }
    } else if (ExecContext->StdErrType == StdErrTypeNull) {
        Handle = CreateFile(_T("NUL"),
                            GENERIC_WRITE,
                            FILE_SHARE_DELETE | FILE_SHARE_READ,
                            &InheritHandle,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
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
        HANDLE NewHandle;
        if (CreatePipe(&ReadHandle, &WriteHandle, NULL, 0)) {

            if (!YoriLibMakeInheritableHandle(WriteHandle, &NewHandle)) {
                Error = GetLastError();
                CloseHandle(ReadHandle);
                CloseHandle(WriteHandle);
            } else {
                PreviousRedirectContext->ResetError = TRUE;
                SetStdHandle(STD_ERROR_HANDLE, NewHandle);
                ExecContext->StdErr.Buffer.PipeFromProcess = ReadHandle;
            }
        } else {
            Error = GetLastError();
        }
    }

    if (Error != ERROR_SUCCESS) {
        YoriLibShRevertRedirection(PreviousRedirectContext);
        return Error;
    }

    if (ExecContext->StdErrType == StdErrTypeStdOut) {
        ASSERT(ExecContext->StdOutType != StdOutTypeStdErr);
        PreviousRedirectContext->ResetError = TRUE;
        PreviousRedirectContext->StdErrRedirectsToStdOut = TRUE;
        SetStdHandle(STD_ERROR_HANDLE, GetStdHandle(STD_OUTPUT_HANDLE));
    } else if (ExecContext->StdOutType == StdOutTypeStdErr) {
        PreviousRedirectContext->ResetOutput = TRUE;
        PreviousRedirectContext->StdOutRedirectsToStdErr = TRUE;
        SetStdHandle(STD_OUTPUT_HANDLE, GetStdHandle(STD_ERROR_HANDLE));
    }

    return ERROR_SUCCESS;
}

/**
 Given a process that has been launched and is currently suspended, examine
 its in memory state to determine which subsystem the process will operate in.

 @param ProcessHandle Specifies the executing process.

 @param Subsystem On successful completion, indicates the subsystem that was
        marked in the executable's PE header.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriLibShGetSubsystemFromExecutingImage(
    __in HANDLE ProcessHandle,
    __out PWORD Subsystem
    )
{
    PROCESS_BASIC_INFORMATION BasicInfo;

    LONG Status;
    DWORD dwBytesReturned;
    SIZE_T BytesReturned;
    PVOID ImageBaseAddress;
    BOOL TargetProcess32BitPeb;
    PVOID PeHeaderPtr;

    //
    //  Since only one is read at a time, use the same buffer for all.
    //
    union {
        YORI_LIB_PEB32_NATIVE ProcessPeb32;
        YORI_LIB_PEB64 ProcessPeb64;
        IMAGE_DOS_HEADER DosHeader;
        YORILIB_PE_HEADERS PeHeaders;
    } u;

    if (DllNtDll.pNtQueryInformationProcess == NULL) {
        return FALSE;
    }

    TargetProcess32BitPeb = YoriLibDoesProcessHave32BitPeb(ProcessHandle);

    Status = DllNtDll.pNtQueryInformationProcess(ProcessHandle, 0, &BasicInfo, sizeof(BasicInfo), &dwBytesReturned);
    if (Status != 0) {
        return FALSE;
    }

    if (TargetProcess32BitPeb) {
        if (!ReadProcessMemory(ProcessHandle, BasicInfo.PebBaseAddress, &u.ProcessPeb32, sizeof(u.ProcessPeb32), &BytesReturned)) {
            return FALSE;
        }

        ImageBaseAddress = (PVOID)(ULONG_PTR)u.ProcessPeb32.ImageBaseAddress;
    } else {

        if (!ReadProcessMemory(ProcessHandle, BasicInfo.PebBaseAddress, &u.ProcessPeb64, sizeof(u.ProcessPeb64), &BytesReturned)) {
            return FALSE;
        }

        ImageBaseAddress = (PVOID)(ULONG_PTR)u.ProcessPeb64.ImageBaseAddress;
    }

    if (!ReadProcessMemory(ProcessHandle, ImageBaseAddress, &u.DosHeader, sizeof(u.DosHeader), &BytesReturned)) {
        return FALSE;
    }

    if (BytesReturned < sizeof(u.DosHeader) ||
        u.DosHeader.e_magic != IMAGE_DOS_SIGNATURE ||
        u.DosHeader.e_lfanew == 0) {

        return FALSE;
    }

    PeHeaderPtr = YoriLibAddToPointer(ImageBaseAddress, u.DosHeader.e_lfanew);

    if (!ReadProcessMemory(ProcessHandle, PeHeaderPtr, &u.PeHeaders, sizeof(u.PeHeaders), &BytesReturned)) {
        return FALSE;
    }

    if (BytesReturned >= sizeof(YORILIB_PE_HEADERS) &&
        u.PeHeaders.Signature == IMAGE_NT_SIGNATURE &&
        u.PeHeaders.ImageHeader.SizeOfOptionalHeader >= FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, Subsystem) + sizeof(WORD)) {

        *Subsystem = u.PeHeaders.OptionalHeader.Subsystem;
        return TRUE;
    }

    return FALSE;
}

/**
 A wrapper around CreateProcess that sets up redirection and launches a
 process.  The point of this function is that it can be called from the
 main thread or from a debugging thread.

 @param ExecContext Pointer to the ExecContext to attempt to launch via
        CreateProcess.

 @param CurrentDirectory Optionally points to a NULL terminated string
        indicating the current directory to apply.  If NULL, the process
        current directory is used instead.

 @param FailedInRedirection Optionally points to a boolean value to be set to
        TRUE if any error originated while setting up redirection, and FALSE
        if it came from launching the process.

 @return Win32 error code, meaning zero indicates success.
 */
DWORD
YoriLibShCreateProcess(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __in_opt LPTSTR CurrentDirectory,
    __out_opt PBOOL FailedInRedirection
    )
{
    YORI_STRING CmdLine;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    YORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext;
    DWORD CreationFlags = 0;
    DWORD LastError;

    ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
    if (FailedInRedirection != NULL) {
        *FailedInRedirection = FALSE;
    }

    YoriLibInitEmptyString(&CmdLine);
    if (!YoriLibShBuildCmdlineFromCmdContext(&ExecContext->CmdToExec, &CmdLine, !ExecContext->IncludeEscapesAsLiteral, NULL, NULL)) {
        return ERROR_OUTOFMEMORY;
    }
    ASSERT(YoriLibIsStringNullTerminated(&CmdLine));

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (ExecContext->RunOnSecondConsole) {
        CreationFlags |= CREATE_NEW_CONSOLE;
    }

    if (ExecContext->CaptureEnvironmentOnExit) {
        CreationFlags |= DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;
    }

    CreationFlags |= CREATE_NEW_PROCESS_GROUP | CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

    LastError = YoriLibShInitializeRedirection(ExecContext, FALSE, &PreviousRedirectContext);
    if (LastError != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&CmdLine);
        if (FailedInRedirection != NULL) {
            *FailedInRedirection = TRUE;
        }
        return LastError;
    }

    if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, CreationFlags, NULL, CurrentDirectory, &StartupInfo, &ProcessInfo)) {
        LastError = GetLastError();
        YoriLibShRevertRedirection(&PreviousRedirectContext);
        YoriLibFreeStringContents(&CmdLine);
        return LastError;
    } else {
        YoriLibShRevertRedirection(&PreviousRedirectContext);
    }

    //
    //  The nice way to terminate console processes is via
    //  GenerateConsoleCtrlEvent, which gives the child a chance to exit
    //  gracefully.  Unfortunately the console misbehaves very badly if this
    //  is performed on a process that is not a console process.  Default to
    //  non-graceful termination, and see if we can upgrade to graceful
    //  termination by verifying that the process is a console process.
    //

    ExecContext->TerminateGracefully = FALSE;
    if (!ExecContext->RunOnSecondConsole) {
        WORD Subsystem;
        if (YoriLibShGetSubsystemFromExecutingImage(ProcessInfo.hProcess, &Subsystem) &&
            Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI) {

            ExecContext->TerminateGracefully = TRUE;
        }
    }

    ResumeThread(ProcessInfo.hThread);

    ASSERT(ExecContext->hProcess == NULL);
    ExecContext->hProcess = ProcessInfo.hProcess;
    ExecContext->hPrimaryThread = ProcessInfo.hThread;
    ExecContext->dwProcessId = ProcessInfo.dwProcessId;

    YoriLibFreeStringContents(&CmdLine);

    return ERROR_SUCCESS;
}

/**
 Cleanup the ExecContext if the process failed to launch.

 @param ExecContext Pointer to the ExecContext to clean up.
 */
VOID
YoriLibShCleanupFailedProcessLaunch(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
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
YoriLibShCommenceProcessBuffersIfNeeded(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
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
            if (YoriLibShAppendToExistingProcessBuffer(ExecContext)) {
                ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
            } else {
                ExecContext->StdOut.Buffer.ProcessBuffers = NULL;
            }
        } else {
            if (YoriLibShCreateNewProcessBuffer(ExecContext)) {
                if (ExecContext->StdOutType == StdOutTypeBuffer) {
                    ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
                }
                if (ExecContext->StdErrType == StdErrTypeBuffer) {
                    ExecContext->StdErr.Buffer.PipeFromProcess = NULL;
                }
            }
        }
    }
}

/**
 Construct a CmdContext that contains "cmd", "/c" and an arbitrary string.
 CMD uses this syntax to allow a whole pile of things, including redirects,
 to be encoded into a single argument.  On success, the caller is expected
 to free this with @ref YoriLibShFreeCmdContext .

 @param CmdContext Pointer to a CmdContext to be populated within this
        routine.

 @param CmdLine Pointer to an arbitrary command to include.  This is cloned
        (not copied) within this routine, so the caller is expected to not
        modify it or copy it if necessary.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibShBuildCmdContextForCmdBuckPass (
    __out PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __in PYORI_STRING CmdLine
    )
{
    YORI_STRING Arg;
    YORI_STRING FoundInPath;

    //
    //  Allocate three components, for "cmd", "/c" and "CmdLine"
    //

    CmdContext->ArgC = 3;
    CmdContext->MemoryToFree = YoriLibReferencedMalloc(CmdContext->ArgC * (sizeof(YORI_STRING) + sizeof(YORI_LIBSH_ARG_CONTEXT)));
    if (CmdContext->MemoryToFree == NULL) {
        return FALSE;
    }

    CmdContext->ArgV = CmdContext->MemoryToFree;

    CmdContext->ArgContexts = (PYORI_LIBSH_ARG_CONTEXT)YoriLibAddToPointer(CmdContext->ArgV, sizeof(YORI_STRING) * CmdContext->ArgC);
    ZeroMemory(CmdContext->ArgContexts, sizeof(YORI_LIBSH_ARG_CONTEXT) * CmdContext->ArgC);

    //
    //  Locate "cmd" in PATH
    //

    YoriLibConstantString(&Arg, _T("cmd"));
    YoriLibInitEmptyString(&FoundInPath);
    if (YoriLibLocateExecutableInPath(&Arg, NULL, NULL, &FoundInPath) && FoundInPath.LengthInChars > 0) {
        memcpy(&CmdContext->ArgV[0], &FoundInPath, sizeof(YORI_STRING));
        ASSERT(YoriLibIsStringNullTerminated(&CmdContext->ArgV[0]));
        YoriLibInitEmptyString(&FoundInPath);
    } else {
        YoriLibCloneString(&CmdContext->ArgV[0], &Arg);
    }

    YoriLibFreeStringContents(&FoundInPath);

    YoriLibConstantString(&CmdContext->ArgV[1], _T("/c"));

    //
    //  Add user arg and enforce that it is always quoted.  Note this isn't
    //  a deep copy, just a clone; the caller is expected to copy if needed.
    //

    YoriLibCloneString(&CmdContext->ArgV[2], CmdLine);
    CmdContext->ArgContexts[2].Quoted = TRUE;
    CmdContext->ArgContexts[2].QuoteTerminated = TRUE;

    //
    //  Initialize unused fields
    //

    CmdContext->CurrentArg = 0;
    CmdContext->CurrentArgOffset = 0;
    CmdContext->TrailingChars = FALSE;

    YoriLibShCheckIfArgNeedsQuotes(CmdContext, 0);

    return TRUE;
}

// vim:sw=4:ts=4:et:
