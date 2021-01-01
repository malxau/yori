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
    YORI_STRING FoundInPath;
    BOOL ExecutableFound;
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

        ExecutableFound = FALSE;
        YoriLibInitEmptyString(&FoundInPath);
        if (YoriLibLocateExecutableInPath(&ExecContext->CmdToExec.ArgV[0], NULL, NULL, &FoundInPath) && FoundInPath.LengthInChars > 0) {
            YoriLibFreeStringContents(&ExecContext->CmdToExec.ArgV[0]);
            memcpy(&ExecContext->CmdToExec.ArgV[0], &FoundInPath, sizeof(YORI_STRING));
            ExecutableFound = TRUE;
        }

        //
        //  MSFIX: If !ExecutableFound, need to consult table of builtins.
        //  This seems like it belongs in YoriSh, needs a bit of porting.
        //

        if (ExecutableFound) {
            ExitCode = MakeShExecuteSingleProgram(ExecContext);
        } else {
            ExitCode = 255;
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
