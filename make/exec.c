/**
 * @file make/exec.c
 *
 * Yori shell make execute child process support
 *
 * Copyright (c) 2020 Malcolm J. Smith
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

/**
 Information about a currently executing child process.
 */
typedef struct _MAKE_CHILD_PROCESS {

    /**
     Set to TRUE to indicate that the command for the child has been parsed
     and on completion this parsed state needs to be torn down.  If FALSE,
     a process has not launched successfully so this structure can be used
     without tearing anything down.
     */
    BOOLEAN CmdContextPresent;

    /**
     Indicates the job identifier.
     */
    DWORD JobId;

    /**
     The target that has requested this child process to be performed.
     */
    PMAKE_TARGET Target;

    /**
     The current command within the target being executed.
     */
    PMAKE_CMD_TO_EXEC Cmd;

    /**
     A handle to a child process to wait for completion.  This is the same
     value as embedded in the ExecPlan and is replicated here only to make
     code referring to it shorter.
     */
    HANDLE ProcessHandle;

    /**
     A command context.  Should be deallocated if CmdContextPresent is TRUE.
     */
    YORI_LIBSH_CMD_CONTEXT CmdContext;

    /**
     An execution plan.  Should be deallocated if CmdContextPresent is TRUE.
     */
    YORI_LIBSH_EXEC_PLAN ExecPlan;
} MAKE_CHILD_PROCESS, *PMAKE_CHILD_PROCESS;

/**
 Attempt to set the temporary directory for this process to match the
 specified JobId, creating the directory if it does not exist.

 @param MakeContext Pointer to the make context, indicating which directories
        have been created and what the parent temporary path is.

 @param JobId Specifies the job identifier.

 @return TRUE to indicate the directory exists and has been set to be the
         active temporary directory, or FALSE if the directory could not be
         changed.
 */
BOOL
MakeSetTemporaryDirectory(
    __in PMAKE_CONTEXT MakeContext,
    __in DWORD JobId
    )
{
    DWORDLONG TestMask;
    YORI_STRING JobTempPath;

    ASSERT(JobId < MakeContext->NumberProcesses);
    TestMask = 1;
    TestMask = TestMask << JobId;

    if (!YoriLibAllocateString(&JobTempPath, MakeContext->TempPath.LengthInChars + sizeof("\\YMAKE12"))) {
        return FALSE;
    }

    JobTempPath.LengthInChars = YoriLibSPrintf(JobTempPath.StartOfString, _T("%y\\YMAKE%i"), &MakeContext->TempPath, JobId);

    if ((MakeContext->TempDirectoriesCreated & TestMask) == 0) {
        if (!YoriLibCreateDirectoryAndParents(&JobTempPath)) {
            YoriLibFreeStringContents(&JobTempPath);
            return FALSE;
        }

        MakeContext->TempDirectoriesCreated = MakeContext->TempDirectoriesCreated | TestMask;
    }

    if (!SetEnvironmentVariable(_T("TEMP"), JobTempPath.StartOfString)) {
        YoriLibFreeStringContents(&JobTempPath);
        return FALSE;
    }

    if (!SetEnvironmentVariable(_T("TMP"), JobTempPath.StartOfString)) {
        YoriLibFreeStringContents(&JobTempPath);
        return FALSE;
    }

    YoriLibFreeStringContents(&JobTempPath);
    return TRUE;
}

/**
 Attempt to remove any temporary directories created by this program.
 Currently directories that contain files are not removed.  This occurs if a
 child process did not clean up temporary files.

 @param MakeContext Pointer to the make context.
 */
VOID
MakeCleanupTemporaryDirectories(
    __in PMAKE_CONTEXT MakeContext
    )
{
    DWORD Probe;
    DWORDLONG TestMask;
    YORI_STRING TempPath;

    if (!YoriLibAllocateString(&TempPath, MakeContext->TempPath.LengthInChars + sizeof("\\YMAKE12"))) {
        return;
    }

    for (Probe = 0; Probe < MakeContext->NumberProcesses; Probe++) {
        TestMask = 1;
        TestMask = TestMask << Probe;
        if ((MakeContext->TempDirectoriesCreated & TestMask) != 0) {
            TempPath.LengthInChars = YoriLibSPrintf(TempPath.StartOfString, _T("%y\\YMAKE%i"), &MakeContext->TempPath, Probe);
            RemoveDirectory(TempPath.StartOfString);
        }
    }

    YoriLibFreeStringContents(&TempPath);
}

/**
 Allocate a job identifier for the child process.  This is a number between
 zero to the maximum number of child processes minus one.  Because of the
 limit in the number of child processes, one of these values should always
 be available.

 @param MakeContext Pointer to the make context.

 @return The job identifier.
 */
DWORD
MakeAllocateJobId(
    __in PMAKE_CONTEXT MakeContext
    )
{
    DWORD Probe;
    DWORDLONG TestMask;

    for (Probe = 0; Probe < MakeContext->NumberProcesses; Probe++) {
        TestMask = 1;
        TestMask = TestMask << Probe;
        if ((MakeContext->JobIdsAllocated & TestMask) == 0) {
            MakeContext->JobIdsAllocated = MakeContext->JobIdsAllocated | TestMask;
            MakeSetTemporaryDirectory(MakeContext, Probe);
            return Probe;
        }
    }

    ASSERT(Probe < MakeContext->NumberProcesses);
    return Probe;
}

/**
 Free a job identifier that was previously associated with a child process.

 @param MakeContext Pointer to the make context.

 @param JobId The job identifier.
 */
VOID
MakeFreeJobId(
    __in PMAKE_CONTEXT MakeContext,
    __in DWORD JobId
    )
{
    DWORDLONG TestMask;

    ASSERT(JobId < MakeContext->NumberProcesses);

    TestMask = 1;
    TestMask = TestMask << JobId;
    ASSERT(MakeContext->JobIdsAllocated & TestMask);
    MakeContext->JobIdsAllocated = MakeContext->JobIdsAllocated & ~(TestMask);
}

/**
 The list of commands to invoke via cmd /c without trying to spawn an
 external process.
 */
CONST LPTSTR
MakePuntToCmd[] = {
    _T("COPY"),
    _T("ERASE"),
    _T("FOR"),
    _T("IF"),
    _T("MOVE"),
    _T("REN")
};

/**
 Return TRUE if there are more commands to execute as part of constructing
 this target, or FALSE if this target is complete.

 @param ChildProcess Pointer to the child process structure specifying the
        target.

 @return TRUE if there are more commands to execute as part of constructing
         this target, or FALSE if this target is complete.
 */
BOOLEAN
MakeDoesTargetHaveMoreCommands(
    __in PMAKE_CHILD_PROCESS ChildProcess
    )
{
    PYORI_LIST_ENTRY ListEntry;
    ListEntry = YoriLibGetNextListEntry(&ChildProcess->Target->ExecCmds, &ChildProcess->Cmd->ListEntry);
    if (ListEntry == NULL) {
        return FALSE;
    }

    return TRUE;
}

/**
 Inspect an if statement and indicate whether it can be handled in process or
 needs to be handled by CMD.  Currently in process support only exists for
 "if exist" or "if not exist".  If support exists in process, the command to
 execute is returned in CmdToExec.  Note that the command to execute may be
 an empty string, indicating the condition is not satisfied, and no command
 needs to be executed.

 @param ChildProcess Pointer to information about the child process to
        execute, including the working directory.

 @param ArgC Specifies the number of arguments.

 @param ArgV Specifies an array of arguments.

 @param CmdToExec On input, specifies the full command string of the if
        expression.  On output, if the command can be handled in proc, this
        is updated to contain the command to execute in CMD, which may be
        empty.

 @return TRUE to indicate this if expression was handled in proc, and the
         CmdToExec string is updated to indicate the external command to
         invoke.  FALSE to indicate the expression should be resolved by CMD.
 */
__success(return)
BOOLEAN
MakeProcessIf(
    __in PMAKE_CHILD_PROCESS ChildProcess,
    __in DWORD ArgC,
    __in YORI_STRING ArgV[],
    __inout PYORI_STRING CmdToExec
    )
{
    DWORD Index;
    BOOLEAN Not;
    YORI_STRING FullPath;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    BOOLEAN Found;
    BOOLEAN ConditionTrue;

    Not = FALSE;

    for (Index = 1; Index < ArgC; Index++) {
        if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[Index], _T("NOT")) == 0) {
            if (Not) {
                Not = FALSE;
            } else {
                Not = TRUE;
            }
        } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[Index], _T("EXIST")) == 0 &&
                   Index + 1 < ArgC) {
            break;
        } else {
            return FALSE;
        }
    }

    if (Index + 1 > ArgC) {
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibGetFullPathNameRelativeTo(&ChildProcess->Target->ScopeContext->HashEntry.Key, &ArgV[Index + 1], TRUE, &FullPath, NULL)) {
        return FALSE;
    }

    Found = FALSE;
    FindHandle = FindFirstFile(FullPath.StartOfString, &FindData);
    if (FindHandle != INVALID_HANDLE_VALUE) {
        Found = TRUE;
        FindClose(FindHandle);
    }

    YoriLibFreeStringContents(&FullPath);

    ConditionTrue = FALSE;
    if ((Found && !Not) || (!Found && Not)) {

        ConditionTrue = TRUE;
    }

    if (ArgC > Index + 2 && ConditionTrue) {
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - Index - 2, &ArgV[Index + 2], TRUE, TRUE, &FullPath)) {
            return FALSE;
        }
    }

    YoriLibFreeStringContents(CmdToExec);
    memcpy(CmdToExec, &FullPath, sizeof(YORI_STRING));
    return TRUE;
}


/**
 If there is a CmdContext or ExecPlan attached to the child process structure,
 free it now in preparation for reuse.

 @param ChildProcess Pointer to the child process structure.
 */
VOID
MakeFreeCmdContextIfNecessary(
    __in PMAKE_CHILD_PROCESS ChildProcess
    )
{
    if (ChildProcess->CmdContextPresent) {
        YoriLibShFreeExecPlan(&ChildProcess->ExecPlan);
        YoriLibShFreeCmdContext(&ChildProcess->CmdContext);
        ChildProcess->CmdContextPresent = FALSE;
    }
}

/**
 Start executing the next command within a target.

 @param MakeContext Pointer to the make context.

 @param ChildProcess Pointer to the child process structure specifying the
        target.

 @return TRUE to indicate that a child process was successfully created, or
         FALSE if it was not.  Note that if the command indicates that failure
         is tolerable, this function can return TRUE without creating a 
         child process, which is indicated by the ChildProcess->ProcessHandle
         member being NULL.
 */
BOOLEAN
MakeLaunchNextCmd(
    __in PMAKE_CONTEXT MakeContext,
    __inout PMAKE_CHILD_PROCESS ChildProcess
    )
{
    STARTUPINFO si;
    PYORI_LIST_ENTRY ListEntry;
    PMAKE_TARGET Target;
    PMAKE_CMD_TO_EXEC CmdToExec;
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext;
    DWORD Index;
    DWORD Error;
    BOOL FailedInRedirection;
    YORI_STRING CmdToParse;
    YORI_STRING CurrentDirectory;
    BOOLEAN ExecutedBuiltin;
    BOOLEAN PuntToCmd;
    BOOLEAN Reparse;

    ASSERT(!ChildProcess->CmdContextPresent);

    Target = ChildProcess->Target;

    if (ChildProcess->Cmd == NULL) {
        ListEntry = YoriLibGetNextListEntry(&Target->ExecCmds, NULL);
    } else {
        ListEntry = YoriLibGetNextListEntry(&Target->ExecCmds, &ChildProcess->Cmd->ListEntry);
    }

    //
    //  Each target being executed should have one command, and when
    //  it finishes, we need to check whether to call here or update
    //  dependencies and move to the next target
    //

    ASSERT(ListEntry != NULL);
    ChildProcess->Cmd = CONTAINING_RECORD(ListEntry, MAKE_CMD_TO_EXEC, ListEntry);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    CmdToExec = CONTAINING_RECORD(ListEntry, MAKE_CMD_TO_EXEC, ListEntry);
    if (CmdToExec->DisplayCmd && !MakeContext->SilentCommandLaunching) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &CmdToExec->Cmd);
    }

    YoriLibInitEmptyString(&CurrentDirectory);
    CurrentDirectory.StartOfString = Target->ScopeContext->HashEntry.Key.StartOfString;
    CurrentDirectory.LengthInChars = Target->ScopeContext->HashEntry.Key.LengthInChars;
    CurrentDirectory.LengthAllocated = Target->ScopeContext->HashEntry.Key.LengthAllocated;
    if (YoriLibIsPathPrefixed(&CurrentDirectory)) {
        CurrentDirectory.StartOfString += sizeof("\\\\.\\") - 1;
        CurrentDirectory.LengthInChars -= sizeof("\\\\.\\") - 1;
        CurrentDirectory.LengthAllocated -= sizeof("\\\\.\\") - 1;
    }

    ASSERT(YoriLibIsStringNullTerminated(&CurrentDirectory));

    YoriLibInitEmptyString(&CmdToParse);
    CmdToParse.StartOfString = CmdToExec->Cmd.StartOfString;
    CmdToParse.LengthInChars = CmdToExec->Cmd.LengthInChars;

    //
    //  Check if this command is a builtin, and if so, execute it inline
    //

    PuntToCmd = FALSE;
    while(TRUE) {
        ExecutedBuiltin = FALSE;
        PuntToCmd = FALSE;
        Reparse = FALSE;

        ASSERT(!ChildProcess->CmdContextPresent);

        if (!YoriLibShParseCmdlineToCmdContext(&CmdToParse, 0, &ChildProcess->CmdContext)) {
            YoriLibFreeStringContents(&CmdToParse);
            return FALSE;
        }

        if (!YoriLibShParseCmdContextToExecPlan(&ChildProcess->CmdContext, &ChildProcess->ExecPlan, NULL, NULL, NULL, NULL)) {
            YoriLibShFreeCmdContext(&ChildProcess->CmdContext);
            YoriLibFreeStringContents(&CmdToParse);
            return FALSE;
        }

        ChildProcess->CmdContextPresent = TRUE;
        ExecContext = ChildProcess->ExecPlan.FirstCmd;

        if (ExecContext->CmdToExec.ArgV == NULL ||
            ExecContext->CmdToExec.ArgC == 0) {

            ExecutedBuiltin = TRUE;
            ChildProcess->ProcessHandle = NULL;

        } else {

            DWORD Result = EXIT_SUCCESS;
            if (YoriLibCompareStringWithLiteralInsensitive(&ExecContext->CmdToExec.ArgV[0], _T("IF")) == 0) {
                if (MakeProcessIf(ChildProcess, ExecContext->CmdToExec.ArgC, ExecContext->CmdToExec.ArgV, &CmdToParse)) {
                    Reparse = TRUE;
                }
            } else {
                PYORI_LIBSH_BUILTIN_CALLBACK Callback;

                Callback = YoriLibShLookupBuiltinByName(&ExecContext->CmdToExec.ArgV[0]);
                if (Callback) {

                    SetCurrentDirectory(CurrentDirectory.StartOfString);
                    Result = MakeShExecuteInProc(Callback->BuiltInFn, ExecContext);
                    SetCurrentDirectory(MakeContext->ProcessCurrentDirectory.StartOfString);

                    ExecutedBuiltin = TRUE;
                    ChildProcess->ProcessHandle = NULL;
                }
            }

            if (!ExecutedBuiltin && !Reparse) {
                for (Index = 0; Index < sizeof(MakePuntToCmd)/sizeof(MakePuntToCmd[0]); Index++) {
                    if (YoriLibCompareStringWithLiteralInsensitive(&ExecContext->CmdToExec.ArgV[0], MakePuntToCmd[Index]) == 0) {
                        PuntToCmd = TRUE;
                        break;
                    }
                }
            }

            if (Result != EXIT_SUCCESS && !CmdToExec->IgnoreErrors) {
                MakeFreeCmdContextIfNecessary(ChildProcess);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Failure to launch:\n%y\n"), &CmdToExec->Cmd);
                return FALSE;
            }
        }

        if (ExecutedBuiltin) {
            YoriLibFreeStringContents(&CmdToParse);
            return TRUE;
        }

        if (!Reparse) {
            break;
        }

        MakeFreeCmdContextIfNecessary(ChildProcess);

        if (CmdToParse.LengthInChars == 0) {
            YoriLibFreeStringContents(&CmdToParse);
            ChildProcess->ProcessHandle = NULL;
            return TRUE;
        }
    }

    //
    //  Check for unsupported operators.  Currently redirection can be
    //  handled within make, but it does not understand how to pipe commands
    //  or otherwise execute multi-command plans (|, ||, &&, &.)
    //

    if (!PuntToCmd) {
        ASSERT(ChildProcess->CmdContextPresent);
        if (ChildProcess->ExecPlan.NumberCommands > 1) {
            PuntToCmd = TRUE;
        }
    }

    //
    //  If it's not a known builtin, execute the command.  When punting to cmd
    //  this involves creating a string for it (which performs path lookup
    //  internally), but for anything else we perform path lookup below.
    //

    if (PuntToCmd) {

        MakeFreeCmdContextIfNecessary(ChildProcess);

        if (!YoriLibShBuildCmdContextForCmdBuckPass(&ChildProcess->CmdContext, &CmdToParse)) {
            YoriLibFreeStringContents(&CmdToParse);
            return FALSE;
        }

        if (!YoriLibShParseCmdContextToExecPlan(&ChildProcess->CmdContext, &ChildProcess->ExecPlan, NULL, NULL, NULL, NULL)) {
            YoriLibShFreeCmdContext(&ChildProcess->CmdContext);
            return FALSE;
        }

        ChildProcess->CmdContextPresent = TRUE;
    } else {
        YORI_STRING FoundInPath;

        ExecContext = ChildProcess->ExecPlan.FirstCmd;

        YoriLibInitEmptyString(&FoundInPath);
        if (YoriLibLocateExecutableInPath(&ExecContext->CmdToExec.ArgV[0], NULL, NULL, &FoundInPath)) {
            if (FoundInPath.LengthInChars > 0) {
                YoriLibFreeStringContents(&ExecContext->CmdToExec.ArgV[0]);
                memcpy(&ExecContext->CmdToExec.ArgV[0], &FoundInPath, sizeof(YORI_STRING));
            } else {
                YoriLibFreeStringContents(&FoundInPath);
            }
        }
    }

    ExecContext = ChildProcess->ExecPlan.FirstCmd;

    ASSERT(ChildProcess->ExecPlan.NumberCommands == 1);
    if (ExecContext->StdOutType == StdOutTypeDefault) {
        ExecContext->StdOutType = StdOutTypeBuffer;
        if (ExecContext->StdErrType == StdErrTypeDefault) {
            ExecContext->StdErrType = StdErrTypeStdOut;
        }
    }

    ChildProcess->JobId = MakeAllocateJobId(MakeContext);

    Error = YoriLibShCreateProcess(ExecContext,
                                   CurrentDirectory.StartOfString,
                                   &FailedInRedirection);

    if (Error != ERROR_SUCCESS) {
        MakeFreeJobId(MakeContext, ChildProcess->JobId);
        ChildProcess->ProcessHandle = NULL;
        if (!CmdToExec->IgnoreErrors) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Failure to launch:\n%y\n"), &CmdToParse);
            YoriLibFreeStringContents(&CmdToParse);
            MakeFreeCmdContextIfNecessary(ChildProcess);
            return FALSE;
        }
    } else {
        CloseHandle(ExecContext->hPrimaryThread);
        ExecContext->hPrimaryThread = NULL;
        ChildProcess->ProcessHandle = ExecContext->hProcess;
        YoriLibShCommenceProcessBuffersIfNeeded(ExecContext);
    }
    YoriLibFreeStringContents(&CmdToParse);

    return TRUE;
}

/**
 Launch the recipe for the next ready target.

 @param MakeContext Pointer to the context.

 @param ChildProcess Pointer to the child process structure to populate with
        information about the child process that has been initiated.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
MakeLaunchNextTarget(
    __in PMAKE_CONTEXT MakeContext,
    __inout PMAKE_CHILD_PROCESS ChildProcess
    )
{
    PMAKE_TARGET Target;
    PYORI_LIST_ENTRY ListEntry;

    ASSERT(!ChildProcess->CmdContextPresent);

    ListEntry = YoriLibGetNextListEntry(&MakeContext->TargetsReady, NULL);

    //
    //  The caller should have checked for this
    //

    ASSERT(ListEntry != NULL);
    if (ListEntry == NULL) {
        return FALSE;
    }

    YoriLibRemoveListItem(ListEntry);
    YoriLibAppendList(&MakeContext->TargetsRunning, ListEntry);
    Target = CONTAINING_RECORD(ListEntry, MAKE_TARGET, RebuildList);

    ChildProcess->Target = Target;
    ChildProcess->Cmd = NULL;

    return MakeLaunchNextCmd(MakeContext, ChildProcess);
}

/**
 Update the dependency graph to ensure that any targets waiting for the
 specified target can now be executed.

 @param MakeContext Pointer to the context.

 @param Target Pointer to the target that has completed.
 */
VOID
MakeUpdateDependenciesForTarget(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET Target
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PMAKE_TARGET_DEPENDENCY Dependency;

    YoriLibRemoveListItem(&Target->RebuildList);
    YoriLibAppendList(&MakeContext->TargetsFinished, &Target->RebuildList);

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&Target->ChildDependents, ListEntry);
    while (ListEntry != NULL) {
        Dependency = CONTAINING_RECORD(ListEntry, MAKE_TARGET_DEPENDENCY, ParentDependents);
        if (Dependency->Child->RebuildRequired) {
            Dependency->Child->NumberParentsToBuild--;
            if (Dependency->Child->NumberParentsToBuild == 0) {
                YoriLibRemoveListItem(&Dependency->Child->RebuildList);
                YoriLibAppendList(&MakeContext->TargetsReady, &Dependency->Child->RebuildList);
            }
        }
        ListEntry = YoriLibGetNextListEntry(&Target->ChildDependents, ListEntry);
    }
}

/**
 After a child process has completed, indicate whether it succeeded or
 failed.  A failed process that the makefile indicates is allowed to fail
 is considered success.

 @param MakeContext Pointer to the make context.

 @param ChildProcess Pointer to the child that has completed.

 @return TRUE to indicate the process succeeded, FALSE to indicate the process
         failed.
 */
BOOLEAN
MakeProcessCompletion(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_CHILD_PROCESS ChildProcess
    )
{
    DWORD ExitCode;
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext;
    YORI_STRING ProcessOutput;
    BOOLEAN Result;
    BOOLEAN RestoreColor;
    WORD DefaultColor;

    ExitCode = EXIT_SUCCESS;

    Result = TRUE;

    //
    //  Ideally this would wait for the process buffer threads rather than
    //  wait for process termination, then get here and wait for the process
    //  buffer threads.  Unfortunately since there are two threads and we're
    //  hitting the 64 object wait limit, it seems like the lesser evil.
    //

    if (ChildProcess->ProcessHandle != NULL) {

        ExitCode = 255;
        GetExitCodeProcess(ChildProcess->ProcessHandle, &ExitCode);
        ASSERT(ChildProcess->CmdContextPresent);

        DefaultColor = YoriLibVtGetDefaultColor();
        RestoreColor = FALSE;

        if (ExitCode != 0) {

            if (!ChildProcess->Cmd->IgnoreErrors) {
                YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, (WORD)((DefaultColor & 0xF0) | FOREGROUND_RED | FOREGROUND_INTENSITY));
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Error in target: %y\n"), &ChildProcess->Target->HashEntry.Key);
                Result = FALSE;
            } else {
                YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, (WORD)((DefaultColor & 0xF0) | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY));
            }

            RestoreColor = TRUE;
        }

        //
        //  If the output was being sent to a buffer owned by make, display
        //  the result now, in a different color if the process failed.
        //  Note at this point execution is serialized - this might prevent
        //  child processes from being launched, but output will be ordered.
        //

        ExecContext = ChildProcess->ExecPlan.FirstCmd;

        if (ExecContext->StdOutType == StdOutTypeBuffer) {
            YoriLibShWaitForProcessBufferToFinalize(ExecContext->StdOut.Buffer.ProcessBuffers);
            if (YoriLibShGetProcessOutputBuffer(ExecContext->StdOut.Buffer.ProcessBuffers, &ProcessOutput)) {
                if (ProcessOutput.LengthInChars > 0) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &ProcessOutput);
                }
                YoriLibFreeStringContents(&ProcessOutput);
            }

            YoriLibShTeardownProcessBuffersIfCompleted(ExecContext->StdOut.Buffer.ProcessBuffers);
        } else {
            ASSERT(ExecContext->StdErrType != StdErrTypeBuffer);
        }

        if (!ChildProcess->Cmd->IgnoreErrors && ExitCode != 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Command:\n%y\n"), &ChildProcess->Cmd->Cmd);
        }

        if (RestoreColor) {
            YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, YoriLibVtGetDefaultColor());
        }

        //
        //  This is closed implicitly as part of FreeExecPlan below
        //

        ASSERT(ChildProcess->CmdContextPresent);
        ChildProcess->ProcessHandle = NULL;
        MakeFreeJobId(MakeContext, ChildProcess->JobId);
    }

    MakeFreeCmdContextIfNecessary(ChildProcess);

    return Result;
}


/**
 Remove all targets that are in the front of the ready queue but really have
 no actions to perform.

 MSFIX This process should probably occur earlier, when a target moves from
 waiting it can move directly to completed if there is nothing to do.  This
 allows targets that depend on it to immediately become ready rather than
 waiting for that target to reach the front of the queue before realizing
 that there are other actions that can be performed.

 @param MakeContext Pointer to the context.

 @return TRUE to indicate one or more targets were processed, FALSE to
         indicate that the next target has an action to perform.
 */
BOOLEAN
MakeCompleteReadyWithNoRecipe(
    __in PMAKE_CONTEXT MakeContext
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PMAKE_TARGET Target;
    BOOLEAN RemovedItem;

    RemovedItem = FALSE;
    ListEntry = YoriLibGetNextListEntry(&MakeContext->TargetsReady, NULL);
    while (ListEntry != NULL) {
        Target = CONTAINING_RECORD(ListEntry, MAKE_TARGET, RebuildList);
        if (YoriLibIsListEmpty(&Target->ExecCmds)) {
            RemovedItem = TRUE;
            MakeUpdateDependenciesForTarget(MakeContext, Target);
        } else {
            break;
        }
        ListEntry = YoriLibGetNextListEntry(&MakeContext->TargetsReady, NULL);
    }

    return RemovedItem;
}

/**
 Execute commands required to build the requested target.

 @param MakeContext Pointer to the context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeExecuteRequiredTargets(
    __in PMAKE_CONTEXT MakeContext
    )
{

    DWORD NumberActiveProcesses;
    DWORD Index;
    HANDLE *ProcessHandleArray;
    PMAKE_CHILD_PROCESS ChildProcessArray;
    BOOLEAN Result;
    BOOLEAN MoveToNextTarget;
    BOOLEAN TargetFailureObserved;

    NumberActiveProcesses = 0;
    TargetFailureObserved = FALSE;

    ProcessHandleArray = YoriLibMalloc(MakeContext->NumberProcesses * sizeof(HANDLE));
    if (ProcessHandleArray == NULL) {
        return FALSE;
    }

    ZeroMemory(ProcessHandleArray, MakeContext->NumberProcesses * sizeof(HANDLE));

    ChildProcessArray = YoriLibMalloc(MakeContext->NumberProcesses * sizeof(MAKE_CHILD_PROCESS));
    if (ChildProcessArray == NULL) {
        YoriLibFree(ProcessHandleArray);
        return FALSE;
    }

    ZeroMemory(ChildProcessArray, MakeContext->NumberProcesses * sizeof(MAKE_CHILD_PROCESS));
    Result = TRUE;

    while (TRUE) {

        while (NumberActiveProcesses < MakeContext->NumberProcesses && !YoriLibIsListEmpty(&MakeContext->TargetsReady)) {
            if (!MakeCompleteReadyWithNoRecipe(MakeContext)) {
                if (!MakeLaunchNextTarget(MakeContext, &ChildProcessArray[NumberActiveProcesses])) {
                    Result = FALSE;
                    goto Drain;
                }
                NumberActiveProcesses++;
            }
        }

        while (NumberActiveProcesses == MakeContext->NumberProcesses || YoriLibIsListEmpty(&MakeContext->TargetsReady)) {

            if (NumberActiveProcesses == 0) {
                break;
            }

            //
            //  A process handle can be NULL if either a command failed to
            //  launch but was prefixed with - indicating failures should be
            //  ignored; or if it's a builtin command that completed
            //  synchronously.  In either case rather than wait, just process
            //  as if this command completed and move to the next command or
            //  target.
            //

            for (Index = 0; Index < NumberActiveProcesses; Index++) {
                ProcessHandleArray[Index] = ChildProcessArray[Index].ProcessHandle;
                if (ProcessHandleArray[Index] == NULL) {
                    break;
                }
            }

            if (Index == NumberActiveProcesses) {
                Index = WaitForMultipleObjects(NumberActiveProcesses, ProcessHandleArray, FALSE, INFINITE);
                Index = Index - WAIT_OBJECT_0;
            }

            //
            //  Check if the process succeeded.  If so, and there are more
            //  commands for the target, try to launch those.
            //

            MoveToNextTarget = TRUE;
            Result = MakeProcessCompletion(MakeContext, &ChildProcessArray[Index]);
            if (Result) {
                if (MakeDoesTargetHaveMoreCommands(&ChildProcessArray[Index])) {
                    if (MakeLaunchNextCmd(MakeContext, &ChildProcessArray[Index])) {
                        MoveToNextTarget = FALSE;
                    } else {
                        Result = FALSE;
                    }
                }
            }

            //
            //  If we are moving to the next target, overwrite this child
            //  process with the executing ones, and launch a new target.
            //  If we failed, we still need to move up all of the executing
            //  ones to facilitate drain.
            //

            if (MoveToNextTarget) {
                if (Result) {
                    MakeUpdateDependenciesForTarget(MakeContext, ChildProcessArray[Index].Target);
                }

                if (NumberActiveProcesses > Index + 1) {
                    memmove(&ChildProcessArray[Index],
                            &ChildProcessArray[Index + 1],
                            (NumberActiveProcesses - Index - 1) * sizeof(MAKE_CHILD_PROCESS));
                }

                NumberActiveProcesses--;

                //
                //  Everything in the final entry has been moved to earlier
                //  entries, so the final entry is uninitialized.
                //

                ZeroMemory(&ChildProcessArray[NumberActiveProcesses], sizeof(MAKE_CHILD_PROCESS));
            }

            if (Result == FALSE) {
                if (MakeContext->KeepGoing) {
                    TargetFailureObserved = TRUE;
                } else {
                    goto Drain;
                }
            }
        }

        //
        //  Nothing failed to launch, nothing returned a failure code, all
        //  ready targets have been launched and completed.  There shouldn't
        //  be anything left to do or something is horribly wrong.
        //

        if (NumberActiveProcesses == 0 && YoriLibIsListEmpty(&MakeContext->TargetsReady)) {
            ASSERT(MakeContext->KeepGoing || YoriLibIsListEmpty(&MakeContext->TargetsWaiting));
            break;
        }
    }

    if (TargetFailureObserved) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Failures encountered, build incomplete\n"));
    }

Drain:

    while (NumberActiveProcesses > 0) {
        for (Index = 0; Index < NumberActiveProcesses; Index++) {
            if (ChildProcessArray[Index].ProcessHandle == NULL) {
                break;
            }
            ProcessHandleArray[Index] = ChildProcessArray[Index].ProcessHandle;
        }

        if (Index == NumberActiveProcesses) {
            Index = WaitForMultipleObjects(NumberActiveProcesses, ProcessHandleArray, FALSE, INFINITE);
            Index = Index - WAIT_OBJECT_0;

            MakeProcessCompletion(MakeContext, &ChildProcessArray[Index]);
        }

        if (NumberActiveProcesses > Index + 1) {
            memmove(&ChildProcessArray[Index],
                    &ChildProcessArray[Index + 1],
                    (NumberActiveProcesses - Index - 1) * sizeof(MAKE_CHILD_PROCESS));
        }

        NumberActiveProcesses--;
    }

    YoriLibFree(ChildProcessArray);
    YoriLibFree(ProcessHandleArray);

    return Result;
}

// vim:sw=4:ts=4:et:
