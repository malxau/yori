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
#include "make.h"

/**
 Information about a currently executing child process.
 */
typedef struct _MAKE_CHILD_PROCESS {

    /**
     The target that has requested this child process to be performed.
     */
    PMAKE_TARGET Target;

    /**
     The current command within the target being executed.
     */
    PMAKE_CMD_TO_EXEC Cmd;

    /**
     The process information about the child.  This includes a handle to
     the process, providing something to wait on.
     */
    PROCESS_INFORMATION ProcessInfo;
} MAKE_CHILD_PROCESS, *PMAKE_CHILD_PROCESS;

/**
 A structure defining a mapping between a command name and a function to
 execute.  This is used to populate builtin commands.
 */
typedef struct _MAKE_BUILTIN_NAME_MAPPING {

    /**
     The command name.
     */
    LPTSTR CommandName;

    /**
     Pointer to the function to execute.
     */
    PYORI_CMD_BUILTIN BuiltinFn;
} MAKE_BUILTIN_NAME_MAPPING, *PMAKE_BUILTIN_NAME_MAPPING;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YECHO;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YMKDIR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YRMDIR;

/**
 The list of builtin commands supported by this build of Yori.
 */
CONST MAKE_BUILTIN_NAME_MAPPING
MakeBuiltinCmds[] = {
    {_T("ECHO"),      YoriCmd_YECHO},
    {_T("MKDIR"),     YoriCmd_YMKDIR},
    {_T("RMDIR"),     YoriCmd_YRMDIR}
};

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
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - Index - 2, &ArgV[Index + 2], TRUE, &FullPath)) {
            return FALSE;
        }
    }

    YoriLibFreeStringContents(CmdToExec);
    memcpy(CmdToExec, &FullPath, sizeof(YORI_STRING));
    return TRUE;
}

/**
 Start executing the next command within a target.

 @param ChildProcess Pointer to the child process structure specifying the
        target.

 @return TRUE to indicate that a child process was successfully created, or
         FALSE if it was not.  Note that if the command indicates that failure
         is tolerable, this function can return TRUE without creating a 
         child process, which is indicated by the
         ChildProcess->ProcessInfo.hProcess member being NULL.
 */
BOOLEAN
MakeLaunchNextCmd(
    __inout PMAKE_CHILD_PROCESS ChildProcess
    )
{
    STARTUPINFO si;
    PYORI_LIST_ENTRY ListEntry;
    PMAKE_TARGET Target;
    PMAKE_CMD_TO_EXEC CmdToExec;
    PYORI_STRING ArgV;
    DWORD Index;
    DWORD ArgC;
    BOOL Success;
    YORI_STRING ExecString;
    YORI_STRING CmdToParse;
    BOOLEAN ExecutedBuiltin;
    BOOLEAN PuntToCmd;
    BOOLEAN Reparse;

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
    if (CmdToExec->DisplayCmd) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &CmdToExec->Cmd);
    }

    YoriLibInitEmptyString(&CmdToParse);
    CmdToParse.StartOfString = CmdToExec->Cmd.StartOfString;
    CmdToParse.LengthInChars = CmdToExec->Cmd.LengthInChars;

    //
    //  Check if this command is a builtin, and if so, execute it inline
    //

    while(TRUE) {
        ExecutedBuiltin = FALSE;
        PuntToCmd = FALSE;
        Reparse = FALSE;

        ArgV = YoriLibCmdlineToArgcArgv(CmdToParse.StartOfString, (DWORD)-1, &ArgC);
        if (ArgV != NULL) {
            DWORD Result = EXIT_SUCCESS;
            if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[0], _T("IF")) == 0) {
                if (MakeProcessIf(ChildProcess, ArgC, ArgV, &CmdToParse)) {
                    Reparse = TRUE;
                }
            } else {
                for (Index = 0; Index < sizeof(MakeBuiltinCmds)/sizeof(MakeBuiltinCmds[0]); Index++) {
                    if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[0], MakeBuiltinCmds[Index].CommandName) == 0) {

                        // 
                        //  MSFIX This needs to SetCurrentDirectory or ensure
                        //  that nothing depends on current directory
                        //

                        Result = MakeBuiltinCmds[Index].BuiltinFn(ArgC, ArgV);

                        ExecutedBuiltin = TRUE;
                        ChildProcess->ProcessInfo.hProcess = NULL;
                        ChildProcess->ProcessInfo.hThread = NULL;
                        break;
                    }
                }
            }

            if (!ExecutedBuiltin && !Reparse) {
                for (Index = 0; Index < sizeof(MakePuntToCmd)/sizeof(MakePuntToCmd[0]); Index++) {
                    if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[0], MakePuntToCmd[Index]) == 0) {
                        PuntToCmd = TRUE;
                        break;
                    }
                }
            }

            for (Index = 0; Index < ArgC; Index++) {
                YoriLibFreeStringContents(&ArgV[Index]);
            }
            YoriLibDereference(ArgV);
            if (Result != EXIT_SUCCESS && !CmdToExec->IgnoreErrors) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Failure to launch %y\n"), &CmdToExec->Cmd);
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

        if (CmdToParse.LengthInChars == 0) {
            YoriLibFreeStringContents(&CmdToParse);
            ChildProcess->ProcessInfo.hProcess = NULL;
            ChildProcess->ProcessInfo.hThread = NULL;
            return TRUE;
        }
    }

    //
    //  Check for operators which would only be meaningful to CMD and if
    //  present use CMD to invoke the command.
    //

    for (Index = 0; Index < CmdToParse.LengthInChars; Index++) {
        if (CmdToParse.StartOfString[Index] == '>' ||
            CmdToParse.StartOfString[Index] == '<') {

            PuntToCmd = TRUE;
            break;
        }
    }

    //
    //  If it's not a known builtin, execute the command.
    //

    YoriLibInitEmptyString(&ExecString);
    if (PuntToCmd) {
        if (!YoriLibAllocateString(&ExecString, sizeof("cmd /c ") + CmdToParse.LengthInChars + 1)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Out of memory\n"));
            YoriLibFreeStringContents(&CmdToParse);
            return FALSE;
        }

        ExecString.LengthInChars = YoriLibSPrintf(ExecString.StartOfString, _T("cmd /c %y"), &CmdToParse);

    } else {
        ExecString.StartOfString = CmdToParse.StartOfString;
        ExecString.LengthInChars = CmdToParse.LengthInChars;
    }

    Success = CreateProcess(NULL,
                            ExecString.StartOfString,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            ChildProcess->Target->ScopeContext->HashEntry.Key.StartOfString,
                            &si,
                            &ChildProcess->ProcessInfo);

    if (!Success) {
        if (!CmdToExec->IgnoreErrors) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Failure to launch %y\n"), &ExecString);
            YoriLibFreeStringContents(&CmdToParse);
            YoriLibFreeStringContents(&ExecString);
            return FALSE;
        }

        ChildProcess->ProcessInfo.hProcess = NULL;
    } else {
        CloseHandle(ChildProcess->ProcessInfo.hThread);
        ChildProcess->ProcessInfo.hThread = NULL;
    }
    YoriLibFreeStringContents(&ExecString);
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
    __out PMAKE_CHILD_PROCESS ChildProcess
    )
{
    PMAKE_TARGET Target;
    PYORI_LIST_ENTRY ListEntry;

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

    return MakeLaunchNextCmd(ChildProcess);
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

 @param ChildProcess Pointer to the child that has completed.

 @return TRUE to indicate the process succeeded, FALSE to indicate the process
         failed.
 */
BOOLEAN
MakeProcessCompletion(
    __in PMAKE_CHILD_PROCESS ChildProcess
    )
{
    DWORD ExitCode;

    ExitCode = EXIT_SUCCESS;
    if (ChildProcess->ProcessInfo.hProcess != NULL) {
        ExitCode = 255;
        GetExitCodeProcess(ChildProcess->ProcessInfo.hProcess, &ExitCode);
        CloseHandle(ChildProcess->ProcessInfo.hProcess);
    }

    if (!ChildProcess->Cmd->IgnoreErrors && ExitCode != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Terminating due to error executing %y\n"), &ChildProcess->Cmd->Cmd);
        return FALSE;
    }

    return TRUE;
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

    NumberActiveProcesses = 0;

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
                ProcessHandleArray[Index] = ChildProcessArray[Index].ProcessInfo.hProcess;
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
            Result = MakeProcessCompletion(&ChildProcessArray[Index]);
            if (Result) {
                if (MakeDoesTargetHaveMoreCommands(&ChildProcessArray[Index])) {
                    if (MakeLaunchNextCmd(&ChildProcessArray[Index])) {
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
            }

            if (Result == FALSE) {
                goto Drain;
            }
        }

        //
        //  Nothing failed to launch, nothing returned a failure code, all
        //  ready targets have been launched and completed.  There shouldn't
        //  be anything left to do or something is horribly wrong.
        //

        if (NumberActiveProcesses == 0 && YoriLibIsListEmpty(&MakeContext->TargetsReady)) {
            ASSERT(YoriLibIsListEmpty(&MakeContext->TargetsWaiting));
            break;
        }
    }

Drain:

    while (NumberActiveProcesses > 0) {
        for (Index = 0; Index < NumberActiveProcesses; Index++) {
            if (ChildProcessArray[Index].ProcessInfo.hProcess == NULL) {
                break;
            }
            ProcessHandleArray[Index] = ChildProcessArray[Index].ProcessInfo.hProcess;
        }

        if (Index == NumberActiveProcesses) {
            Index = WaitForMultipleObjects(NumberActiveProcesses, ProcessHandleArray, FALSE, INFINITE);
            Index = Index - WAIT_OBJECT_0;

            CloseHandle(ChildProcessArray[Index].ProcessInfo.hProcess);
            ChildProcessArray[Index].ProcessInfo.hProcess = NULL;
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
