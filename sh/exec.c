/**
 * @file /sh/exec.c
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

/**
 Try to launch a single program via ShellExecuteEx rather than CreateProcess.
 This is used to open URLs, documents and scripts, as well as when
 CreateProcess said elevation is needed.

 @param ExecContext Pointer to the program to execute.

 @param ProcessInfo On successful completion, the process handle might be
        populated here.  It might not, because shell could decide to
        communicate with an existing process via DDE, and it doesn't tell us
        about the process it was talking to in that case.  In fairness, we
        probably shouldn't wait on a process that we didn't launch.
 
 @return TRUE to indicate success, FALSE on failure.
 */
__success(return)
BOOL
YoriShExecViaShellExecute(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __out PPROCESS_INFORMATION ProcessInfo
    )
{
    YORI_STRING Args;
    YORI_LIBSH_CMD_CONTEXT ArgContext;
    YORI_SHELLEXECUTEINFO sei;
    YORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext;
    SYSERR LastError;

    YoriLibLoadShell32Functions();

    //
    //  This function is called for two reasons.  It might be because a
    //  process launch requires elevation, in which case ShellExecuteEx
    //  should exist because any OS with UAC will have it.  For NT 3.51,
    //  ShellExecuteEx exists but fails, and before that, it's not even
    //  there.  This code has to handle each case.
    //

    if (DllShell32.pShellExecuteExW == NULL && DllShell32.pShellExecuteW == NULL) {
        return FALSE;
    }

    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS |
                SEE_MASK_FLAG_NO_UI |
                SEE_MASK_NOZONECHECKS |
                SEE_MASK_UNICODE |
                SEE_MASK_NO_CONSOLE;

    sei.lpFile = ExecContext->CmdToExec.ArgV[0].StartOfString;
    YoriLibInitEmptyString(&Args);
    if (ExecContext->CmdToExec.ArgC > 1) {
        memcpy(&ArgContext, &ExecContext->CmdToExec, sizeof(YORI_LIBSH_CMD_CONTEXT));
        ArgContext.ArgC--;
        ArgContext.ArgV = &ArgContext.ArgV[1];
        ArgContext.ArgContexts = &ArgContext.ArgContexts[1];
        YoriLibShBuildCmdlineFromCmdContext(&ArgContext, &Args, !ExecContext->IncludeEscapesAsLiteral, NULL, NULL);
    }

    sei.lpParameters = Args.StartOfString;
    sei.nShow = SW_SHOWNORMAL;

    ZeroMemory(ProcessInfo, sizeof(PROCESS_INFORMATION));

    LastError = YoriLibShInitializeRedirection(ExecContext, FALSE, &PreviousRedirectContext);
    if (LastError != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&Args);
        return FALSE;
    }

    LastError = ERROR_SUCCESS;
    if (DllShell32.pShellExecuteExW != NULL) {
        if (!DllShell32.pShellExecuteExW(&sei)) {
            LastError = GetLastError();
        }
    }

    if (DllShell32.pShellExecuteExW == NULL ||
        LastError == ERROR_CALL_NOT_IMPLEMENTED) {

        HINSTANCE hInst;
        hInst = DllShell32.pShellExecuteW(NULL, NULL, sei.lpFile, sei.lpParameters, _T("."), sei.nShow);
        LastError = YoriLibShellExecuteInstanceToError(hInst);
    }

    YoriLibShRevertRedirection(&PreviousRedirectContext);
    YoriLibFreeStringContents(&Args);

    if (LastError != ERROR_SUCCESS) {
        LPTSTR ErrText;
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ShellExecuteEx failed (%i): %s"), LastError, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
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
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
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
            YsExt.LengthInChars = ExecContext->CmdToExec.ArgV[0].LengthInChars - (YORI_ALLOC_SIZE_T)(szExt - ExecContext->CmdToExec.ArgV[0].StartOfString);

            if (YoriLibCompareStringLitIns(&YsExt, _T(".com")) == 0) {
                if (YoriShExecuteNamedModuleInProc(ExecContext->CmdToExec.ArgV[0].StartOfString, ExecContext, &ExitCode)) {
                    ExecProcess = FALSE;
                } else {
                    ExitCode = 0;
                }
            } else if (YoriLibCompareStringLitIns(&YsExt, _T(".ys1")) == 0) {
                ExecProcess = FALSE;
                YoriLibShCheckIfArgNeedsQuotes(&ExecContext->CmdToExec, 0);
                ExitCode = YoriShBuckPass(ExecContext, 1, _T("ys"));
            } else if (YoriLibCompareStringLitIns(&YsExt, _T(".cmd")) == 0 ||
                       YoriLibCompareStringLitIns(&YsExt, _T(".bat")) == 0) {
                ExecProcess = FALSE;
                YoriLibShCheckIfArgNeedsQuotes(&ExecContext->CmdToExec, 0);
                if (ExecContext->WaitForCompletion) {
                    ExecContext->CaptureEnvironmentOnExit = TRUE;
                }
                ExitCode = YoriShBuckPassToCmd(ExecContext);
            } else if (YoriLibCompareStringLitIns(&YsExt, _T(".exe")) != 0) {
                LaunchViaShellExecute = TRUE;
                ExecContext->SuppressTaskCompletion = TRUE;
            }
        }
    }

    if (ExecProcess) {

        BOOL FailedInRedirection = FALSE;

        if (!LaunchViaShellExecute && !ExecContext->CaptureEnvironmentOnExit) {
            DWORD Err = YoriLibShCreateProcess(ExecContext, NULL, &FailedInRedirection);

            if (Err != NO_ERROR) {
                if (Err == ERROR_ELEVATION_REQUIRED) {
                    LaunchViaShellExecute = TRUE;
                } else {
                    LPTSTR ErrText = YoriLibGetWinErrorText(Err);
                    if (FailedInRedirection) {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Failed to initialize redirection: %s"), ErrText);
                    } else {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CreateProcess failed: %s"), ErrText);
                    }
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
            YoriLibShCleanupFailedProcessLaunch(ExecContext);
            return 1;
        }

        if (!ExecContext->CaptureEnvironmentOnExit) {
            YoriLibShCommenceProcessBuffersIfNeeded(ExecContext);
        }

        //
        //  We may not have a process handle but still be successful if
        //  ShellExecute decided to interact with an existing process
        //  rather than launch a new one.  This isn't going to be very
        //  common in any interactive shell, and it's clearly going to break
        //  things, but there's not much we can do about it from here.
        //
        //  When launching under a debugger, the launch occurs from the
        //  debugging thread, so a process handle may not be present
        //  until the call to wait on it.
        //

        if (ExecContext->hProcess != NULL || ExecContext->CaptureEnvironmentOnExit) {
            if (ExecContext->CaptureEnvironmentOnExit) {
                ASSERT(ExecContext->WaitForCompletion);
                ExecContext->WaitForCompletion = TRUE;
            }
            if (ExecContext->WaitForCompletion) {
                YoriShWaitForProcessToTerminate(ExecContext);
                if (ExecContext->hProcess != NULL) {
                    GetExitCodeProcess(ExecContext->hProcess, &ExitCode);
                } else {
                    ExitCode = EXIT_FAILURE;
                }
            } else if (ExecContext->StdOutType != StdOutTypePipe) {
                ASSERT(!ExecContext->CaptureEnvironmentOnExit);
                if (YoriShCreateNewJob(ExecContext)) {
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
                if (ExecContext->TerminateGracefully &&
                    ExecContext->dwProcessId != 0) {

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
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
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
    __in PYORI_LIBSH_EXEC_PLAN ExecPlan,
    __out_opt PVOID * OutputBuffer
    )
{
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext;
    PVOID PreviouslyObservedOutputBuffer = NULL;
    BOOLEAN ExecutableFound;

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

        YoriShExpandAlias(&ExecContext->CmdToExec);

        if (YoriLibIsPathUrl(&ExecContext->CmdToExec.ArgV[0])) {
            YoriShGlobal.ErrorLevel = YoriShExecuteSingleProgram(ExecContext);
        } else if (ExecContext->CmdToExec.ArgC >= 2 &&
                   YoriLibCompareStringLitIns(&ExecContext->CmdToExec.ArgV[0], _T("BUILTIN")) == 0) {

            PYORI_STRING ArgV;
            ArgV = ExecContext->CmdToExec.ArgV;
            ExecContext->CmdToExec.ArgV = &ExecContext->CmdToExec.ArgV[1];
            ExecContext->CmdToExec.ArgContexts = &ExecContext->CmdToExec.ArgContexts[1];
            ExecContext->CmdToExec.ArgC--;
            YoriLibFreeStringContents(ArgV);

            YoriShGlobal.ErrorLevel = YoriShBuiltIn(ExecContext);
        } else {
            if (!YoriShResolveCommandToExecutable(&ExecContext->CmdToExec, &ExecutableFound)) {
                break;
            }

            if (ExecutableFound) {
                YoriShGlobal.ErrorLevel = YoriShExecuteSingleProgram(ExecContext);
            } else if (ExecPlan->NumberCommands == 1 && !ExecPlan->WaitForCompletion) {
                YoriShExecViaSubshell(ExecContext);
                if (OutputBuffer != NULL) {
                    *OutputBuffer = NULL;
                }
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
__success(return)
BOOL
YoriShExecuteExpressionAndCaptureOutput(
    __in PYORI_STRING Expression,
    __out PYORI_STRING ProcessOutput
    )
{
    YORI_LIBSH_EXEC_PLAN ExecPlan;
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext;
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    PVOID OutputBuffer;
    YORI_ALLOC_SIZE_T Index;

    //
    //  Parse the expression we're trying to execute.
    //

    if (!YoriLibShParseCmdlineToCmdContext(Expression, 0, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        return FALSE;
    }

    if (CmdContext.ArgC == 0) {
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (!YoriShExpandEnvironmentInCmdContext(&CmdContext)) {
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (!YoriLibShParseCmdContextToExecPlan(&CmdContext, &ExecPlan, NULL, NULL, NULL, NULL)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        YoriLibShFreeCmdContext(&CmdContext);
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

        if (!YoriLibShGetProcessOutputBuffer(OutputBuffer, ProcessOutput)) {
            YoriLibInitEmptyString(ProcessOutput);
        }

        //
        //  Truncate any newlines from the output, which tools
        //  frequently emit but are of no value here
        //

        YoriLibTrimTrailingNewlines(ProcessOutput);

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

    YoriLibShFreeExecPlan(&ExecPlan);
    YoriLibShFreeCmdContext(&CmdContext);

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
__success(return)
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

    YORI_ALLOC_SIZE_T CharsInBackquotePrefix;

    YoriLibInitEmptyString(&CurrentFullExpression);
    CurrentFullExpression.StartOfString = Expression->StartOfString;
    CurrentFullExpression.LengthInChars = Expression->LengthInChars;

    while(TRUE) {

        //
        //  MSFIX This logic currently rescans from the beginning.  Should
        //  we only rescan from the end of the previous scan so we don't
        //  create commands that can nest further backticks?
        //

        if (!YoriLibShFindNextBackquoteSubstring(&CurrentFullExpression, &CurrentExpressionSubset, &CharsInBackquotePrefix)) {
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
        InitialPortion.LengthInChars = (YORI_ALLOC_SIZE_T)(CurrentExpressionSubset.StartOfString - CurrentFullExpression.StartOfString - CharsInBackquotePrefix);

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
__success(return)
BOOL
YoriShExecuteExpression(
    __in PYORI_STRING Expression
    )
{
    YORI_LIBSH_EXEC_PLAN ExecPlan;
    YORI_LIBSH_CMD_CONTEXT CmdContext;
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

    if (!YoriLibShParseCmdlineToCmdContext(&CurrentFullExpression, 0, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        YoriLibFreeStringContents(&CurrentFullExpression);
        return FALSE;
    }

    if (CmdContext.ArgC == 0) {
        YoriLibShFreeCmdContext(&CmdContext);
        YoriLibFreeStringContents(&CurrentFullExpression);
        return FALSE;
    }

    if (!YoriShExpandEnvironmentInCmdContext(&CmdContext)) {
        YoriLibShFreeCmdContext(&CmdContext);
        YoriLibFreeStringContents(&CurrentFullExpression);
        return FALSE;
    }

    if (!YoriLibShParseCmdContextToExecPlan(&CmdContext, &ExecPlan, NULL, NULL, NULL, NULL)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        YoriLibFreeStringContents(&CurrentFullExpression);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriShExecExecPlan(&ExecPlan, NULL);

    YoriLibShFreeExecPlan(&ExecPlan);
    YoriLibShFreeCmdContext(&CmdContext);

    YoriLibFreeStringContents(&CurrentFullExpression);

    return TRUE;
}

// vim:sw=4:ts=4:et:
