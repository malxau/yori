/**
 * @file make/minish.c
 *
 * Yori make stripped down shell logic
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

#include <yoripch.h>
#include <yorilib.h>
#include <yorish.h>
#include "make.h"

//
//  The functions here are cut down variants of the ones in sh/exec.c.
//  The regular shell needs to support scenarios that make no sense for
//  make - things like executing programs through ShellExecute to
//  prompt for elevation, job control, or even in-proc modules don't make
//  much sense here.  There is a fuzzy line between what's in and out
//  though, so potentially more code could migrate here or to libsh to
//  enable it to be shared.
//


/**
 Execute a single program.  If the execution is synchronous, this routine will
 wait for the program to complete and return its exit code.  If the execution
 is not synchronous, this routine will return without waiting and provide zero
 as a (not meaningful) exit code.

 @param ExecContext The context of a single program to execute.

 @return The exit code of the program, when executed synchronously.
 */
DWORD
MakeShExecuteSingleProgram(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    DWORD ExitCode = 0;
    BOOL FailedInRedirection = FALSE;
    DWORD Err;

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
        return 1;
    }

    YoriLibShCommenceProcessBuffersIfNeeded(ExecContext);

    ASSERT(ExecContext->hProcess != NULL);
    if (ExecContext->WaitForCompletion) {
        WaitForSingleObject(ExecContext->hProcess, INFINITE);
        GetExitCodeProcess(ExecContext->hProcess, &ExitCode);
    }
    return ExitCode;
}

/**
 Call a builtin function.  This is part of the main excecutable and is
 executed synchronously via a call rather than a CreateProcess.

 @param Fn The function associated with the builtin operation to call.

 @param ExecContext The context surrounding this particular function.

 @return ExitCode, typically zero for success, nonzero for failure.
 */
DWORD
MakeShExecuteInProc(
    __in PYORI_CMD_BUILTIN Fn,
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    YORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext;
    BOOLEAN WasPipe = FALSE;
    PYORI_LIBSH_CMD_CONTEXT OriginalCmdContext = &ExecContext->CmdToExec;
    YORI_STRING CmdLine;
    PYORI_STRING EscapedArgV;
    PYORI_STRING NoEscapedArgV;
    YORI_ALLOC_SIZE_T ArgC;
    YORI_ALLOC_SIZE_T Count;
    DWORD ExitCode = 0;

    EscapedArgV = NULL;
    NoEscapedArgV = NULL;
    ArgC = 0;

    //
    //  Build a command line, leaving all escapes in the command line.
    //

    YoriLibInitEmptyString(&CmdLine);
    if (!YoriLibShBuildCmdlineFromCmdContext(OriginalCmdContext, &CmdLine, FALSE, NULL, NULL)) {
        ExitCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    //  Parse the command line in the same way that a child process would.
    //

    ASSERT(YoriLibIsStringNullTerminated(&CmdLine));
    EscapedArgV = YoriLibCmdlineToArgcArgv(CmdLine.StartOfString, (YORI_ALLOC_SIZE_T)-1, TRUE, &ArgC);
    YoriLibFreeStringContents(&CmdLine);

    if (EscapedArgV == NULL) {
        ExitCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    //  Remove the escapes from the command line.  This allows the builtin to
    //  have access to the escaped form if required.
    //

    NoEscapedArgV = YoriLibReferencedMalloc(ArgC * sizeof(YORI_STRING));
    if (NoEscapedArgV == NULL) {
        ExitCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    for (Count = 0; Count < ArgC; Count++) {
        YoriLibCloneString(&NoEscapedArgV[Count], &EscapedArgV[Count]);
    }

    if (!YoriLibShRemoveEscapesFromArgCArgV(ArgC, NoEscapedArgV)) {
        ExitCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    //  We execute builtins on a single thread due to the amount of
    //  process wide state that could get messed up if we don't (eg.
    //  stdout.)  Unfortunately this means we can't natively implement
    //  things like pipe from builtins, because the builtin has to
    //  finish before the next process can start.  So if a pipe is
    //  requested, convert it into a buffer, and let the process
    //  finish.
    //

    if (ExecContext->StdOutType == StdOutTypePipe) {
        WasPipe = TRUE;
        ExecContext->StdOutType = StdOutTypeBuffer;
    }

    ExitCode = YoriLibShInitializeRedirection(ExecContext, TRUE, &PreviousRedirectContext);
    if (ExitCode != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    //  Unlike external processes, builtins need to start buffering
    //  before they start to ensure that output during execution has
    //  somewhere to go.
    //

    if (ExecContext->StdOutType == StdOutTypeBuffer) {
        if (ExecContext->StdOut.Buffer.ProcessBuffers != NULL) {
            if (YoriLibShAppendToExistingProcessBuffer(ExecContext)) {
                ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
            } else {
                ExecContext->StdOut.Buffer.ProcessBuffers = NULL;
            }
        } else {
            if (YoriLibShCreateNewProcessBuffer(ExecContext)) {
                ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
            }
        }
    }

    ExitCode = Fn(ArgC, NoEscapedArgV);
    YoriLibShRevertRedirection(&PreviousRedirectContext);

    if (WasPipe) {
        YoriLibShForwardProcessBufferToNextProcess(ExecContext);
    } else {

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
    }

Cleanup:

    if (NoEscapedArgV != NULL) {
        for (Count = 0; Count < ArgC; Count++) {
            YoriLibFreeStringContents(&NoEscapedArgV[Count]);
        }
        YoriLibDereference(NoEscapedArgV);
    }

    if (EscapedArgV != NULL) {
        for (Count = 0; Count < ArgC; Count++) {
            YoriLibFreeStringContents(&EscapedArgV[Count]);
        }
        YoriLibDereference(EscapedArgV);
    }

    return ExitCode;
}

/**
 Cancel an exec plan.  This is invoked after the user hits Ctrl+C and attempts
 to terminate all outstanding processes associated with the request.

 @param ExecPlan The plan to terminate all outstanding processes in.
 */
VOID
MakeShCancelExecPlan(
    __in PYORI_LIBSH_EXEC_PLAN ExecPlan
    )
{
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext;

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
 Execute an exec plan.  An exec plan has multiple processes, including
 different pipe and redirection operators.  Optionally return the result
 of any output buffered processes in the plan, to facilitate back quotes.

 @param ExecPlan Pointer to the exec plan to execute.

 @param OutputBuffer On successful completion, updated to point to the
        resulting output buffer.
 */
DWORD
MakeShExecExecPlan(
    __in PYORI_LIBSH_EXEC_PLAN ExecPlan,
    __out_opt PVOID * OutputBuffer
    )
{
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext;
    PVOID PreviouslyObservedOutputBuffer = NULL;
    PYORI_LIBSH_BUILTIN_CALLBACK Callback;
    YORI_STRING FoundInPath;
    DWORD ExitCode = 0;

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

        ExitCode = 255;
        Callback = YoriLibShLookupBuiltinByName(&ExecContext->CmdToExec.ArgV[0]);
        if (Callback != NULL) {
            ExitCode = MakeShExecuteInProc(Callback->BuiltInFn, ExecContext);
        } else {
            YoriLibInitEmptyString(&FoundInPath);
            if (YoriLibLocateExecutableInPath(&ExecContext->CmdToExec.ArgV[0], NULL, NULL, &FoundInPath)) {
                if (FoundInPath.LengthInChars > 0) {
                    YoriLibFreeStringContents(&ExecContext->CmdToExec.ArgV[0]);
                    memcpy(&ExecContext->CmdToExec.ArgV[0], &FoundInPath, sizeof(YORI_STRING));

                    ExitCode = MakeShExecuteSingleProgram(ExecContext);
                } else {
                    YoriLibFreeStringContents(&FoundInPath);
                }
            }
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
                    if (ExitCode != 0) {
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
                    if (ExitCode == 0) {
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
                    ASSERT(!"YoriLibShParseCmdContextToExecPlan created a plan that MakeShExecExecPlan doesn't know how to execute");
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
        MakeShCancelExecPlan(ExecPlan);
    }

    return ExitCode;
}

// vim:sw=4:ts=4:et:
