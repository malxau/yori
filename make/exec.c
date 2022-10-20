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
typedef struct _MAKE_CHILD_RECIPE {

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
     The current directory for this recipe.
     */
    YORI_STRING CurrentDirectory;

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
} MAKE_CHILD_RECIPE, *PMAKE_CHILD_RECIPE;

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
    _T("REN"),
    _T("TYPE")
};

/**
 Return TRUE if there are more commands to execute as part of constructing
 this target, or FALSE if this target is complete.

 @param ChildRecipe Pointer to the child recipe structure specifying the
        target.

 @return TRUE if there are more commands to execute as part of constructing
         this target, or FALSE if this target is complete.
 */
BOOLEAN
MakeDoesTargetHaveMoreCommands(
    __in PMAKE_CHILD_RECIPE ChildRecipe
    )
{
    PYORI_LIST_ENTRY ListEntry;
    ListEntry = YoriLibGetNextListEntry(&ChildRecipe->Target->ExecCmds, &ChildRecipe->Cmd->ListEntry);
    if (ListEntry == NULL) {
        return FALSE;
    }

    return TRUE;
}

/**
 Change the current directory for the recipe.

 Unlike a "real" change current directory, this is not modifying global
 process state.  It can evaluate relative paths to the current recipe path,
 which is common, but it cannot evaluate things relative to other drive
 letters without recording a full per-drive set of current directories for
 each recipe.  This seems unnecessary because build systems want to traverse
 directories within their build tree, and changing drives would be outside
 that.  If changing drives were supported, we'd also need to implement the
 x: alias notation.

 @param ChildRecipe Pointer to the child recipe specifying the current
        directory for the recipe.

 @param ArgC Specifies the number of arguments.

 @param ArgV Pointer to an array of arguments.

 @return EXIT_FAILURE or EXIT_SUCCESS as appropriate.
 */
DWORD
MakeProcessCd(
    __in PMAKE_CHILD_RECIPE ChildRecipe,
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_STRING NewCurrentDirectory;

    if (ArgC < 2) {
        return EXIT_FAILURE;
    }

    YoriLibInitEmptyString(&NewCurrentDirectory);

    if (!YoriLibGetFullPathNameRelativeTo(&ChildRecipe->CurrentDirectory, &ArgV[1], FALSE, &NewCurrentDirectory, NULL)) {
        return EXIT_FAILURE;
    }

    YoriLibFreeStringContents(&ChildRecipe->CurrentDirectory);
    memcpy(&ChildRecipe->CurrentDirectory, &NewCurrentDirectory, sizeof(YORI_STRING));
    return EXIT_SUCCESS;
}

/**
 Inspect an if statement and indicate whether it can be handled in process or
 needs to be handled by CMD.  Currently in process support only exists for
 "if exist" or "if not exist".  If support exists in process, the command to
 execute is returned in CmdToExec.  Note that the command to execute may be
 an empty string, indicating the condition is not satisfied, and no command
 needs to be executed.

 @param ChildRecipe Pointer to information about the child recipe to
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
    __in PMAKE_CHILD_RECIPE ChildRecipe,
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
    BOOLEAN Insensitive;

    Not = FALSE;
    Insensitive = FALSE;

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
        } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[Index], _T("/I")) == 0) {
            Insensitive = TRUE;
        } else {
            YORI_STRING EqualsOperator;
            YORI_STRING FirstPart;
            YORI_STRING SecondPart;
            PYORI_STRING MatchingOperator;
            DWORD OperatorIndex;

            //
            //  Perform simple string comparison.  If the strings indicate the
            //  rest of the expression shouldn't be invoked, we don't need to
            //  spawn CMD.
            //

            YoriLibConstantString(&EqualsOperator, _T("=="));
            MatchingOperator = YoriLibFindFirstMatchingSubstring(&ArgV[Index], 1, &EqualsOperator, &OperatorIndex);

            ConditionTrue = FALSE;

            if (MatchingOperator != NULL) {
                YoriLibInitEmptyString(&FirstPart);
                FirstPart.StartOfString = ArgV[Index].StartOfString;
                FirstPart.LengthInChars = OperatorIndex;

                YoriLibInitEmptyString(&SecondPart);
                SecondPart.StartOfString = ArgV[Index].StartOfString + OperatorIndex + MatchingOperator->LengthInChars;
                SecondPart.LengthInChars = ArgV[Index].LengthInChars - OperatorIndex - MatchingOperator->LengthInChars;

                if (Insensitive) {
                    if (YoriLibCompareStringInsensitive(&FirstPart, &SecondPart) == 0) {
                        ConditionTrue = TRUE;
                    }
                } else {
                    if (YoriLibCompareString(&FirstPart, &SecondPart) == 0) {
                        ConditionTrue = TRUE;
                    }
                }

                if (Not) {
                    ConditionTrue = (BOOLEAN)(!ConditionTrue);
                }

                if (!ConditionTrue) {
                    YoriLibFreeStringContents(CmdToExec);
                    return TRUE;
                }
            }

            return FALSE;
        }
    }

    if (Index + 1 > ArgC) {
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibGetFullPathNameRelativeTo(&ChildRecipe->CurrentDirectory, &ArgV[Index + 1], TRUE, &FullPath, NULL)) {
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

 @param ChildRecipe Pointer to the child recipe structure.
 */
VOID
MakeFreeCmdContextIfNecessary(
    __in PMAKE_CHILD_RECIPE ChildRecipe
    )
{
    if (ChildRecipe->CmdContextPresent) {
        YoriLibShFreeExecPlan(&ChildRecipe->ExecPlan);
        YoriLibShFreeCmdContext(&ChildRecipe->CmdContext);
        ChildRecipe->CmdContextPresent = FALSE;
    }
}

/**
 Start executing the next command within a target.

 @param MakeContext Pointer to the make context.

 @param ChildRecipe Pointer to the child recipe structure specifying the
        target.

 @return TRUE to indicate that a child process was successfully created, or
         FALSE if it was not.  Note that if the command indicates that failure
         is tolerable, this function can return TRUE without creating a 
         child process, which is indicated by the ChildRecipe->ProcessHandle
         member being NULL.
 */
BOOLEAN
MakeLaunchNextCmd(
    __in PMAKE_CONTEXT MakeContext,
    __inout PMAKE_CHILD_RECIPE ChildRecipe
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
    BOOLEAN ExecutedBuiltin;
    BOOLEAN PuntToCmd;
    BOOLEAN Reparse;

    ASSERT(!ChildRecipe->CmdContextPresent);

    Target = ChildRecipe->Target;

    //
    //  Get the next command within the recipe.
    //

    if (ChildRecipe->Cmd == NULL) {
        ListEntry = YoriLibGetNextListEntry(&Target->ExecCmds, NULL);
    } else {
        ListEntry = YoriLibGetNextListEntry(&Target->ExecCmds, &ChildRecipe->Cmd->ListEntry);
    }

    //
    //  Each target being executed should have at least one command, and when
    //  it finishes, we need to check whether to call here or update
    //  dependencies and move to the next target
    //

    ASSERT(ListEntry != NULL);
    ChildRecipe->Cmd = CONTAINING_RECORD(ListEntry, MAKE_CMD_TO_EXEC, ListEntry);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    CmdToExec = CONTAINING_RECORD(ListEntry, MAKE_CMD_TO_EXEC, ListEntry);
    if (CmdToExec->DisplayCmd && !MakeContext->SilentCommandLaunching) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &CmdToExec->Cmd);
    }


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

        ASSERT(!ChildRecipe->CmdContextPresent);

        if (!YoriLibShParseCmdlineToCmdContext(&CmdToParse, 0, &ChildRecipe->CmdContext)) {
            YoriLibFreeStringContents(&CmdToParse);
            return FALSE;
        }

        if (!YoriLibShParseCmdContextToExecPlan(&ChildRecipe->CmdContext, &ChildRecipe->ExecPlan, NULL, NULL, NULL, NULL)) {
            YoriLibShFreeCmdContext(&ChildRecipe->CmdContext);
            YoriLibFreeStringContents(&CmdToParse);
            return FALSE;
        }

        ChildRecipe->CmdContextPresent = TRUE;
        ExecContext = ChildRecipe->ExecPlan.FirstCmd;

        if (ExecContext->CmdToExec.ArgV == NULL ||
            ExecContext->CmdToExec.ArgC == 0) {

            ExecutedBuiltin = TRUE;
            ChildRecipe->ProcessHandle = NULL;

        } else {

            DWORD Result = EXIT_SUCCESS;
            if (YoriLibCompareStringWithLiteralInsensitive(&ExecContext->CmdToExec.ArgV[0], _T("IF")) == 0) {
                if (MakeProcessIf(ChildRecipe, ExecContext->CmdToExec.ArgC, ExecContext->CmdToExec.ArgV, &CmdToParse)) {
                    Reparse = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&ExecContext->CmdToExec.ArgV[0], _T("CD")) == 0) {
                Result = MakeProcessCd(ChildRecipe, ExecContext->CmdToExec.ArgC, ExecContext->CmdToExec.ArgV);
                ExecutedBuiltin = TRUE;
                ChildRecipe->ProcessHandle = NULL;
            } else {
                PYORI_LIBSH_BUILTIN_CALLBACK Callback;

                Callback = YoriLibShLookupBuiltinByName(&ExecContext->CmdToExec.ArgV[0]);
                if (Callback) {

                    SetCurrentDirectory(ChildRecipe->CurrentDirectory.StartOfString);
                    Result = MakeShExecuteInProc(Callback->BuiltInFn, ExecContext);
                    SetCurrentDirectory(MakeContext->ProcessCurrentDirectory.StartOfString);

                    ExecutedBuiltin = TRUE;
                    ChildRecipe->ProcessHandle = NULL;
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
                MakeFreeCmdContextIfNecessary(ChildRecipe);
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

        MakeFreeCmdContextIfNecessary(ChildRecipe);

        if (CmdToParse.LengthInChars == 0) {
            YoriLibFreeStringContents(&CmdToParse);
            ChildRecipe->ProcessHandle = NULL;
            return TRUE;
        }
    }

    //
    //  Check for unsupported operators.  Currently redirection can be
    //  handled within make, but it does not understand how to pipe commands
    //  or otherwise execute multi-command plans (|, ||, &&, &.)
    //

    if (!PuntToCmd) {
        ASSERT(ChildRecipe->CmdContextPresent);
        if (ChildRecipe->ExecPlan.NumberCommands > 1) {
            PuntToCmd = TRUE;
        }
    }

    //
    //  If it's not a known builtin, execute the command.  When punting to cmd
    //  this involves creating a string for it (which performs path lookup
    //  internally), but for anything else we perform path lookup below.
    //

    if (PuntToCmd) {

        MakeFreeCmdContextIfNecessary(ChildRecipe);

        if (!YoriLibShBuildCmdContextForCmdBuckPass(&ChildRecipe->CmdContext, &CmdToParse)) {
            YoriLibFreeStringContents(&CmdToParse);
            return FALSE;
        }

        if (!YoriLibShParseCmdContextToExecPlan(&ChildRecipe->CmdContext, &ChildRecipe->ExecPlan, NULL, NULL, NULL, NULL)) {
            YoriLibShFreeCmdContext(&ChildRecipe->CmdContext);
            return FALSE;
        }

        ChildRecipe->CmdContextPresent = TRUE;
    } else {
        YORI_STRING FoundInPath;

        ExecContext = ChildRecipe->ExecPlan.FirstCmd;

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

    ExecContext = ChildRecipe->ExecPlan.FirstCmd;

    ASSERT(ChildRecipe->ExecPlan.NumberCommands == 1);
    if (ExecContext->StdOutType == StdOutTypeDefault) {
        ExecContext->StdOutType = StdOutTypeBuffer;
        if (ExecContext->StdErrType == StdErrTypeDefault) {
            ExecContext->StdErrType = StdErrTypeStdOut;
        }
    }

    ChildRecipe->JobId = MakeAllocateJobId(MakeContext);

    Error = YoriLibShCreateProcess(ExecContext,
                                   ChildRecipe->CurrentDirectory.StartOfString,
                                   &FailedInRedirection);

    if (Error != ERROR_SUCCESS) {
        MakeFreeJobId(MakeContext, ChildRecipe->JobId);
        ChildRecipe->ProcessHandle = NULL;
        if (!CmdToExec->IgnoreErrors) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Failure to launch:\n%y\n"), &CmdToParse);
            YoriLibFreeStringContents(&CmdToParse);
            MakeFreeCmdContextIfNecessary(ChildRecipe);
            return FALSE;
        }
    } else {
        CloseHandle(ExecContext->hPrimaryThread);
        ExecContext->hPrimaryThread = NULL;
        ChildRecipe->ProcessHandle = ExecContext->hProcess;
        YoriLibShCommenceProcessBuffersIfNeeded(ExecContext);
    }
    YoriLibFreeStringContents(&CmdToParse);

    return TRUE;
}

/**
 Launch the recipe for the next ready target.

 @param MakeContext Pointer to the context.

 @param ChildRecipe Pointer to the child process structure to populate with
        information about the child process that has been initiated.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
MakeLaunchNextTarget(
    __in PMAKE_CONTEXT MakeContext,
    __inout PMAKE_CHILD_RECIPE ChildRecipe
    )
{
    PMAKE_TARGET Target;
    PYORI_LIST_ENTRY ListEntry;

    //
    //  This recipe structure should be available for use.
    //

    ASSERT(!ChildRecipe->CmdContextPresent);

    //
    //  Get the next target.
    //

    ListEntry = YoriLibGetNextListEntry(&MakeContext->TargetsReady, NULL);

    //
    //  The caller should have checked that there is a target.
    //

    ASSERT(ListEntry != NULL);
    if (ListEntry == NULL) {
        return FALSE;
    }

    YoriLibRemoveListItem(ListEntry);
    YoriLibAppendList(&MakeContext->TargetsRunning, ListEntry);
    Target = CONTAINING_RECORD(ListEntry, MAKE_TARGET, RebuildList);

    ChildRecipe->Target = Target;
    ChildRecipe->Cmd = NULL;

    //
    //  The previous recipe should have been cleaned up.
    //

    ASSERT(ChildRecipe->CurrentDirectory.LengthInChars == 0);
    ASSERT(ChildRecipe->CurrentDirectory.MemoryToFree == NULL);

    //
    //  Initialize the recipe current directory as the scope context
    //  directory.  The recipe is able to change it.
    //

    YoriLibCloneString(&ChildRecipe->CurrentDirectory, &Target->ScopeContext->HashEntry.Key);
    if (YoriLibIsPathPrefixed(&ChildRecipe->CurrentDirectory)) {
        ChildRecipe->CurrentDirectory.StartOfString += sizeof("\\\\.\\") - 1;
        ChildRecipe->CurrentDirectory.LengthInChars -= sizeof("\\\\.\\") - 1;
        ChildRecipe->CurrentDirectory.LengthAllocated -= sizeof("\\\\.\\") - 1;
    }

    ASSERT(YoriLibIsStringNullTerminated(&ChildRecipe->CurrentDirectory));

    return MakeLaunchNextCmd(MakeContext, ChildRecipe);
}

/**
 Indicate that an entire recipe has completed.  This may have succeeded, or
 may have failed partway.  This function needs to clean up regardless.

 @param MakeContext Pointer to the make context.

 @param ChildRecipe Pointer to the recipe to clean up for reuse.
 */
VOID
MakeRecipeCompletion(
    __in PMAKE_CONTEXT MakeContext,
    __inout PMAKE_CHILD_RECIPE ChildRecipe
    )
{
    UNREFERENCED_PARAMETER(MakeContext);

    //
    //  This recipe structure should not have child processes executing.
    //

    ASSERT(!ChildRecipe->CmdContextPresent);
    YoriLibFreeStringContents(&ChildRecipe->CurrentDirectory);
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

 @param ChildRecipe Pointer to the child that has completed.

 @return TRUE to indicate the process succeeded, FALSE to indicate the process
         failed.
 */
BOOLEAN
MakeProcessCompletion(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_CHILD_RECIPE ChildRecipe
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

    if (ChildRecipe->ProcessHandle != NULL) {

        ExitCode = 255;
        GetExitCodeProcess(ChildRecipe->ProcessHandle, &ExitCode);
        ASSERT(ChildRecipe->CmdContextPresent);

        DefaultColor = YoriLibVtGetDefaultColor();
        RestoreColor = FALSE;

        if (ExitCode != 0) {

            if (!ChildRecipe->Cmd->IgnoreErrors) {
                YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, (WORD)((DefaultColor & 0xF0) | FOREGROUND_RED | FOREGROUND_INTENSITY));
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Error in target: %y\n"), &ChildRecipe->Target->HashEntry.Key);
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

        ExecContext = ChildRecipe->ExecPlan.FirstCmd;

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

        if (!ChildRecipe->Cmd->IgnoreErrors && ExitCode != 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Command:\n%y\n"), &ChildRecipe->Cmd->Cmd);
        }

        if (RestoreColor) {
            YoriLibVtSetConsoleTextAttribute(YORI_LIB_OUTPUT_STDOUT, YoriLibVtGetDefaultColor());
        }

        //
        //  This is closed implicitly as part of FreeExecPlan below
        //

        ASSERT(ChildRecipe->CmdContextPresent);
        ChildRecipe->ProcessHandle = NULL;
        MakeFreeJobId(MakeContext, ChildRecipe->JobId);
    }

    MakeFreeCmdContextIfNecessary(ChildRecipe);

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
    PMAKE_CHILD_RECIPE ChildRecipeArray;
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

    ChildRecipeArray = YoriLibMalloc(MakeContext->NumberProcesses * sizeof(MAKE_CHILD_RECIPE));
    if (ChildRecipeArray == NULL) {
        YoriLibFree(ProcessHandleArray);
        return FALSE;
    }

    ZeroMemory(ChildRecipeArray, MakeContext->NumberProcesses * sizeof(MAKE_CHILD_RECIPE));
    Result = TRUE;

    while (TRUE) {

        while (NumberActiveProcesses < MakeContext->NumberProcesses && !YoriLibIsListEmpty(&MakeContext->TargetsReady)) {
            if (!MakeCompleteReadyWithNoRecipe(MakeContext)) {
                if (!MakeLaunchNextTarget(MakeContext, &ChildRecipeArray[NumberActiveProcesses])) {
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
                ProcessHandleArray[Index] = ChildRecipeArray[Index].ProcessHandle;
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
            Result = MakeProcessCompletion(MakeContext, &ChildRecipeArray[Index]);
            if (Result) {
                if (MakeDoesTargetHaveMoreCommands(&ChildRecipeArray[Index])) {
                    if (MakeLaunchNextCmd(MakeContext, &ChildRecipeArray[Index])) {
                        MoveToNextTarget = FALSE;
                    } else {
                        Result = FALSE;
                    }
                } else {
                    MakeRecipeCompletion(MakeContext, &ChildRecipeArray[Index]);
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
                    MakeUpdateDependenciesForTarget(MakeContext, ChildRecipeArray[Index].Target);
                } else {
                    MakeRecipeCompletion(MakeContext, &ChildRecipeArray[Index]);
                }

                if (NumberActiveProcesses > Index + 1) {
                    memmove(&ChildRecipeArray[Index],
                            &ChildRecipeArray[Index + 1],
                            (NumberActiveProcesses - Index - 1) * sizeof(MAKE_CHILD_RECIPE));
                }

                NumberActiveProcesses--;

                //
                //  Everything in the final entry has been moved to earlier
                //  entries, so the final entry is uninitialized.
                //

                ZeroMemory(&ChildRecipeArray[NumberActiveProcesses], sizeof(MAKE_CHILD_RECIPE));
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
            if (ChildRecipeArray[Index].ProcessHandle == NULL) {
                break;
            }
            ProcessHandleArray[Index] = ChildRecipeArray[Index].ProcessHandle;
        }

        if (Index == NumberActiveProcesses) {
            Index = WaitForMultipleObjects(NumberActiveProcesses, ProcessHandleArray, FALSE, INFINITE);
            Index = Index - WAIT_OBJECT_0;

            MakeProcessCompletion(MakeContext, &ChildRecipeArray[Index]);
            MakeRecipeCompletion(MakeContext, &ChildRecipeArray[Index]);
        }

        if (NumberActiveProcesses > Index + 1) {
            memmove(&ChildRecipeArray[Index],
                    &ChildRecipeArray[Index + 1],
                    (NumberActiveProcesses - Index - 1) * sizeof(MAKE_CHILD_RECIPE));
        }

        NumberActiveProcesses--;
    }

    YoriLibFree(ChildRecipeArray);
    YoriLibFree(ProcessHandleArray);

    return Result;
}

// vim:sw=4:ts=4:et:
