/**
 * @file sh/job.c
 *
 * Facilities for managing background jobs
 *
 * Copyright (c) 2017 Malcolm J. Smith
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
 A structure to record a currently executing job.
 */
typedef struct _YORI_JOB {

    /**
     The link into the global list of active jobs.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The current state of the job, including whether it was last known to be
     executing, completed awaiting garbage collection, or completed and to be
     retained indefinitely.  Note that this state is updated before the user
     enters a command (ie., the state might not reflect the operating system's
     opinion on the state) but this allows for deterministic interaction with
     jobs.
     */
    enum {
        JobStateExecuting = 1,
        JobStateCompletedAwaitingDelete = 2,
        JobStateRetained = 3
    } JobState;

    /**
     The job ID of this job.
     */
    DWORD JobId;

    /**
     The ID of the child process.
     */
    DWORD dwProcessId;

    /**
     The count of times that scan has encountered the job and chosen
     to not delete it.
     */
    DWORD ScanEncounteredAfterCompleteCount;

    /**
     The exit code of the process.  Only valid once JobState is either
     JobStateCompletedAwaitingDelete or JobStateRetained.
     */
    DWORD ExitCode;

    /**
     A handle to the child process.
     */
    HANDLE hProcess;


    /**
     The full command line that was used to execute the child process.
     */
    YORI_STRING CmdLine;

    /**
     Pointer to the process buffers, if buffers exist for this job.
     */
    PVOID ProcessBuffers;
} YORI_JOB, *PYORI_JOB;

/**
 The global list of active jobs.
 */
YORI_LIST_ENTRY JobList;

/**
 Allocate a new job for background processing.

 @param ExecContext The process that is currently executing in the background.

 @return TRUE to indicate a job was successfully allocated, FALSE if it was
         not.
 */
__success(return)
BOOL
YoriShCreateNewJob(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    PYORI_JOB ThisJob;

    if (YoriShGlobal.PreviousJobId == 0) {
        YoriLibInitializeListHead(&JobList);
    }

    ThisJob = YoriLibMalloc(sizeof(YORI_JOB));
    if (ThisJob == NULL) {
        return FALSE;
    }

    ZeroMemory(ThisJob, sizeof(YORI_JOB));

    if (!YoriLibShBuildCmdlineFromCmdContext(&ExecContext->CmdToExec, &ThisJob->CmdLine, TRUE, NULL, NULL)) {
        YoriLibFree(ThisJob);
        return FALSE;
    }

    ThisJob->JobId = ++YoriShGlobal.PreviousJobId;
    ThisJob->hProcess = ExecContext->hProcess;
    ThisJob->dwProcessId = ExecContext->dwProcessId;

    if (ExecContext->StdOutType == StdOutTypeBuffer &&
        ExecContext->StdOut.Buffer.ProcessBuffers != NULL) {

        YoriLibShReferenceProcessBuffer(ExecContext->StdOut.Buffer.ProcessBuffers);
        ThisJob->ProcessBuffers = ExecContext->StdOut.Buffer.ProcessBuffers;
    } else if (ExecContext->StdErrType == StdErrTypeBuffer &&
               ExecContext->StdErr.Buffer.ProcessBuffers != NULL) {

        YoriLibShReferenceProcessBuffer(ExecContext->StdErr.Buffer.ProcessBuffers);
        ThisJob->ProcessBuffers = ExecContext->StdErr.Buffer.ProcessBuffers;
    }

    ThisJob->JobState = JobStateExecuting;

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Job %i: %y\n"), ThisJob->JobId, &ThisJob->CmdLine);
    YoriLibAppendList(&JobList, &ThisJob->ListEntry);

    return TRUE;
}

/**
 Free a job structure.

 @param ThisJob The job to free.
 */
VOID
YoriShFreeJob(
    __in PYORI_JOB ThisJob
    )
{
    YoriLibRemoveListItem(&ThisJob->ListEntry);

    if (ThisJob->ProcessBuffers != NULL) {
        YoriLibShDereferenceProcessBuffer(ThisJob->ProcessBuffers);
    }

    YoriLibFreeStringContents(&ThisJob->CmdLine);
    YoriLibFree(ThisJob);
}

/**
 Scan the set of outstanding jobs and report to the user if any have
 completed.

 @param TeardownAll TRUE if the shell is exiting and wants to tear down all
        state.  FALSE if this is a periodic check to tear down things that
        have been around a while.

 @return Always TRUE currently.
 */
__success(return)
BOOL
YoriShScanJobsReportCompletion(
    __in BOOL TeardownAll
    )
{
    PYORI_JOB ThisJob;
    PYORI_LIST_ENTRY ListEntry;

    if (YoriShGlobal.PreviousJobId == 0) {
        return TRUE;
    }

    ListEntry = YoriLibGetNextListEntry(&JobList, NULL);
    while (ListEntry != NULL) {
        ThisJob = CONTAINING_RECORD(ListEntry, YORI_JOB, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&JobList, ListEntry);
        if (ThisJob->JobState == JobStateExecuting) {
            if (WaitForSingleObject(ThisJob->hProcess, 0) == WAIT_OBJECT_0) {
                GetExitCodeProcess(ThisJob->hProcess, &ThisJob->ExitCode);
                ThisJob->JobState = JobStateCompletedAwaitingDelete;
                if (!TeardownAll) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Job %i completed, result %i: %y\n"), ThisJob->JobId, ThisJob->ExitCode, &ThisJob->CmdLine);
                }
            }
        }

        if (TeardownAll && ThisJob->JobState == JobStateExecuting) {
            ThisJob->JobState = JobStateCompletedAwaitingDelete;
        }

        if (ThisJob->JobState == JobStateCompletedAwaitingDelete) {
            ThisJob->ScanEncounteredAfterCompleteCount++;
            if (ThisJob->ScanEncounteredAfterCompleteCount >= 16 || TeardownAll) {
                if (!TeardownAll) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Job %i deleted, result %i: %y\n"), ThisJob->JobId, ThisJob->ExitCode, &ThisJob->CmdLine);
                }
                YoriShFreeJob(ThisJob);
            }
        }
    }

    return TRUE;
}


/**
 Terminates a specified job.

 @param JobId The job to terminate.

 @return TRUE to indicate that the job was requested to terminate, FALSE if it
         was not.
 */
__success(return)
BOOL
YoriShTerminateJob(
    __in DWORD JobId
    )
{
    PYORI_JOB ThisJob;
    PYORI_LIST_ENTRY ListEntry;

    if (YoriShGlobal.PreviousJobId == 0) {
        return TRUE;
    }

    ListEntry = YoriLibGetNextListEntry(&JobList, NULL);
    while (ListEntry != NULL) {
        ThisJob = CONTAINING_RECORD(ListEntry, YORI_JOB, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&JobList, ListEntry);
        if (ThisJob->JobId == JobId) {
            TerminateProcess(ThisJob->hProcess, 1);
            return TRUE;
        }
    }
    return FALSE;
}

/**
 Given a previous job ID, return the next ID that is currently executing.
 To commence from the beginning, specify a PreviousJobId of zero.

 @param PreviousJobId The ID that was previously returned, or zero to
        commence from the beginning.

 @return The next job ID, or zero if all have been enumerated.
 */
DWORD
YoriShGetNextJobId(
    __in DWORD PreviousJobId
    )
{
    PYORI_JOB ThisJob;
    PYORI_LIST_ENTRY ListEntry;

    if (YoriShGlobal.PreviousJobId == 0) {
        return 0;
    }

    ListEntry = YoriLibGetNextListEntry(&JobList, NULL);
    while (ListEntry != NULL) {
        ThisJob = CONTAINING_RECORD(ListEntry, YORI_JOB, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&JobList, ListEntry);
        if (ThisJob->JobId > PreviousJobId) {
            return ThisJob->JobId;
        }
    }
    return 0;
}

/**
 Waits until the specified job ID is no longer active, or until the user
 cancels or breaks out of the wait.

 @param JobId The ID to wait for.
 */
VOID
YoriShJobWait(
    __in DWORD JobId
    )
{
    YORI_SH_WAIT_INPUT_CONTEXT WaitContext;
    YORI_SH_WAIT_OUTCOME Outcome;
    PYORI_JOB ThisJob;
    PYORI_LIST_ENTRY ListEntry;

    if (YoriShGlobal.PreviousJobId == 0) {
        return;
    }

    ListEntry = YoriLibGetNextListEntry(&JobList, NULL);
    ThisJob = NULL;
    while (ListEntry != NULL) {
        ThisJob = CONTAINING_RECORD(ListEntry, YORI_JOB, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&JobList, ListEntry);
        if (ThisJob->JobId == JobId) {
            break;
        } else {
            ThisJob = NULL;
        }
    }

    if (ThisJob == NULL || ThisJob->hProcess == NULL) {
        return;
    }

    YoriShInitializeWaitContext(&WaitContext, ThisJob->hProcess);

    while (TRUE) {
        Outcome = YoriShWaitForProcessOrInput(&WaitContext);

        if (Outcome == YoriShWaitOutcomeProcessExit) {

            //
            //  Once the process has completed, if it's outputting to
            //  buffers, wait for the buffers to contain final data.
            //
            //  MSFIX Bring this back
            //

            /*
            if (ExecContext->StdOutType == StdOutTypeBuffer &&
                ExecContext->StdOut.Buffer.ProcessBuffers != NULL)  {

                YoriLibShWaitForProcessBufferToFinalize(ExecContext->StdOut.Buffer.ProcessBuffers);
            }

            if (ExecContext->StdErrType == StdErrTypeBuffer &&
                ExecContext->StdErr.Buffer.ProcessBuffers != NULL) {

                YoriLibShWaitForProcessBufferToFinalize(ExecContext->StdErr.Buffer.ProcessBuffers);
            }
            */
            break;
        }

        //
        //  If the user has hit Ctrl+C or Ctrl+Break, request the process
        //  to clean up gracefully and unwind.  Later on we'll try to kill
        //  all processes in the exec plan, so we don't need to try too hard
        //  at this point.
        //

        if (Outcome == YoriShWaitOutcomeCancel) {

            //
            //  MSFIX Bring back TerminateGracefully
            //

            /*
            if (ExecContext->TerminateGracefully && ExecContext->dwProcessId != 0) {
                GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, ExecContext->dwProcessId);
                break;
            } else if (ExecContext->hProcess != NULL) {
                TerminateProcess(ExecContext->hProcess, 1);
                break;
            } else {
                Sleep(50);
            }
            */
            TerminateProcess(ThisJob->hProcess, 1);
            break;
        }

        if (Outcome == YoriShWaitOutcomeBackground) {

            break;
        }

    }

    YoriShCleanupWaitContext(&WaitContext);
}

/**
 Sets the priority associated with a job.

 @param JobId The ID to set priority for.

 @param PriorityClass The new priority class.

 @return TRUE to indicate that the priority class was changed, FALSE if it
         was not.
 */
__success(return)
BOOL
YoriShJobSetPriority(
    __in DWORD JobId,
    __in DWORD PriorityClass
    )
{
    PYORI_JOB ThisJob;
    PYORI_LIST_ENTRY ListEntry;

    if (YoriShGlobal.PreviousJobId == 0) {
        return FALSE;
    }

    ListEntry = YoriLibGetNextListEntry(&JobList, NULL);
    while (ListEntry != NULL) {
        ThisJob = CONTAINING_RECORD(ListEntry, YORI_JOB, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&JobList, ListEntry);
        if (ThisJob->JobId == JobId) {
            return SetPriorityClass(ThisJob->hProcess, PriorityClass);
        }
    }

    return FALSE;
}

/**
 Get any output buffers from a completed job, including stdout and stderr
 buffers.

 @param JobId Specifies the job ID to fetch buffers for.

 @param Output On successful completion, populated with any output in the job
        output buffer.

 @param Errors On successful completion, populated with any output in the job
        error buffer.

 @return TRUE to indicate success, FALSE to indicate error.
 */
__success(return)
BOOL
YoriShGetJobOutput(
    __in DWORD JobId,
    __inout PYORI_STRING Output,
    __inout PYORI_STRING Errors
    )
{
    PYORI_JOB ThisJob;
    PYORI_LIST_ENTRY ListEntry;
    BOOL Result;

    if (YoriShGlobal.PreviousJobId == 0) {
        return FALSE;
    }

    ListEntry = YoriLibGetNextListEntry(&JobList, NULL);
    while (ListEntry != NULL) {
        ThisJob = CONTAINING_RECORD(ListEntry, YORI_JOB, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&JobList, ListEntry);
        if (ThisJob->JobId == JobId &&
            ThisJob->ProcessBuffers != NULL) {

            Result = YoriLibShGetProcessOutputBuffer(ThisJob->ProcessBuffers, Output);
            if (Result) {
                Result = YoriLibShGetProcessErrorBuffer(ThisJob->ProcessBuffers, Errors);
                if (!Result) {
                    YoriLibFreeStringContents(Output);
                }
            }
            return Result;
        }
    }

    return FALSE;
}

/**
 Take any existing output from a job and send it to a pipe handle, and continue
 sending further output into the pipe handle.

 @param JobId Specifies the job ID to fetch buffers for.

 @param hPipeOutput Specifies a pipe to forward job standard output into.

 @param hPipeErrors Specifies a pipe to forward job standard error into.

 @return TRUE to indicate success, FALSE to indicate error.
 */
__success(return)
BOOL
YoriShPipeJobOutput(
    __in DWORD JobId,
    __in_opt HANDLE hPipeOutput,
    __in_opt HANDLE hPipeErrors
    )
{
    PYORI_JOB ThisJob;
    PYORI_LIST_ENTRY ListEntry;
    BOOL Result;

    if (YoriShGlobal.PreviousJobId == 0) {
        return FALSE;
    }

    ListEntry = YoriLibGetNextListEntry(&JobList, NULL);
    while (ListEntry != NULL) {
        ThisJob = CONTAINING_RECORD(ListEntry, YORI_JOB, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&JobList, ListEntry);
        if (ThisJob->JobId == JobId &&
            ThisJob->ProcessBuffers != NULL) {

            Result = YoriLibShPipeProcessBuffers(ThisJob->ProcessBuffers, hPipeOutput, hPipeErrors);
            return Result;
        }
    }

    return FALSE;
}

/**
 Returns information associated with an executing or completed job ID.

 @param JobId The ID to query information for.

 @param HasCompleted On successful completion, set to TRUE if the job has
        finished executing; FALSE if the job is still executing.

 @param HasOutput On successful completion, set to TRUE if the job has
        output buffers retained that can be displayed as needed.
        This is only meaningful if the job has completed.

 @param ExitCode On successful completion, once the job has completed,
        contains the exit code from the job.

 @param Command On successful completion, updated to contain the command
        line used to initiate the job.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShGetJobInformation(
    __in DWORD JobId,
    __out PBOOL HasCompleted,
    __out PBOOL HasOutput,
    __out PDWORD ExitCode,
    __inout PYORI_STRING Command
    )
{
    PYORI_JOB ThisJob;
    PYORI_LIST_ENTRY ListEntry;
    DWORD CmdLength;

    if (YoriShGlobal.PreviousJobId == 0) {
        return FALSE;
    }

    ListEntry = YoriLibGetNextListEntry(&JobList, NULL);
    while (ListEntry != NULL) {
        ThisJob = CONTAINING_RECORD(ListEntry, YORI_JOB, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&JobList, ListEntry);
        if (ThisJob->JobId == JobId) {

            if (ThisJob->JobState == JobStateCompletedAwaitingDelete ||
                ThisJob->JobState == JobStateRetained) {

                *HasCompleted = TRUE;
                if (ThisJob->ProcessBuffers != NULL) {
                    *HasOutput = TRUE;
                } else {
                    *HasOutput = FALSE;
                }
                *ExitCode = ThisJob->ExitCode;
            } else {
                *HasCompleted = FALSE;
                *HasOutput = FALSE;
                *ExitCode = 0;
            }

            //
            //  Allocate space for the command line, and include a trailing
            //  NULL just to be polite.
            //

            CmdLength = ThisJob->CmdLine.LengthInChars + 1;
            if (Command->LengthAllocated < CmdLength) {
                YoriLibFreeStringContents(Command);
                if (!YoriLibAllocateString(Command, CmdLength)) {
                    return FALSE;
                }
            }

            memcpy(Command->StartOfString, ThisJob->CmdLine.StartOfString, CmdLength * sizeof(TCHAR));
            Command->LengthInChars = CmdLength - 1;

            return TRUE;
        }
    }

    return FALSE;
}


// vim:sw=4:ts=4:et:
