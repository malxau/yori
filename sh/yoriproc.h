/**
 * @file sh/yoriproc.h
 *
 * Yori shell function declaration header file
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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

// *** ALIAS.C ***

BOOL
YoriShAddAlias(
    __in PYORI_STRING Alias,
    __in PYORI_STRING Value,
    __in BOOL Internal
    );

BOOL
YoriShAddAliasLiteral(
    __in LPTSTR Alias,
    __in LPTSTR Value,
    __in BOOL Internal
    );

BOOL
YoriShDeleteAlias(
    __in PYORI_STRING Alias
    );

BOOL
YoriShExpandAlias(
    __inout PYORI_SH_CMD_CONTEXT CmdContext
    );

BOOL
YoriShExpandAliasFromString(
    __in PYORI_STRING CommandString,
    __out PYORI_STRING ExpandedString
    );

VOID
YoriShClearAllAliases();

BOOL
YoriShGetAliasStrings(
    __in BOOL IncludeInternal,
    __inout PYORI_STRING AliasStrings
    );

BOOL
YoriShLoadSystemAliases(
    __in BOOL ImportFromCmd
    );

BOOL
YoriShMergeChangedAliasStrings(
    __in BOOL MergeFromCmd,
    __in PYORI_STRING OldStrings,
    __in PYORI_STRING NewStrings
    );

BOOL
YoriShGetSystemAliasStrings(
    __in BOOL LoadFromCmd,
    __out PYORI_STRING AliasBuffer
    );

// *** BUILTIN.C ***

extern CONST YORI_SH_BUILTIN_NAME_MAPPING YoriShBuiltins[];

DWORD
YoriShBuckPass (
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext,
    __in DWORD ExtraArgCount,
    ...
    );

BOOL
YoriShExecuteNamedModuleInProc(
    __in LPTSTR ModuleFileName,
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext,
    __out PDWORD ExitCode
    );

DWORD
YoriShBuiltIn (
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    );

BOOL
YoriShExecuteBuiltinString(
    __in PYORI_STRING Expression
    );

BOOL
YoriShSetUnloadRoutine(
    __in PYORI_BUILTIN_UNLOAD_NOTIFY UnloadNotify
    );

BOOL
YoriShBuiltinRegister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    );

BOOL
YoriShBuiltinUnregister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    );

VOID
YoriShBuiltinUnregisterAll(
    );

// *** CMDBUF.C ***

BOOL
YoriShCreateNewProcessBuffer(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    );

BOOL
YoriShAppendToExistingProcessBuffer(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    );

BOOL
YoriShForwardProcessBufferToNextProcess(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    );

VOID
YoriShDereferenceProcessBuffer(
    __in PVOID ThisBuffer
    );

VOID
YoriShReferenceProcessBuffer(
    __in PVOID ThisBuffer
    );

BOOL
YoriShGetProcessOutputBuffer(
    __in PVOID ThisBuffer,
    __out PYORI_STRING String
    );

BOOL
YoriShGetProcessErrorBuffer(
    __in PVOID ThisBuffer,
    __out PYORI_STRING String
    );

BOOL
YoriShScanProcessBuffersForTeardown(
    __in BOOL TeardownAll
    );

BOOL
YoriShWaitForProcessBufferToFinalize(
    __in PVOID ThisBuffer
    );

BOOL
YoriShPipeProcessBuffers(
    __in PVOID ThisBuffer,
    __in HANDLE hPipeOutput,
    __in HANDLE hPipeErrors
    );

// *** COMPLETE.C ***

VOID
YoriShClearTabCompletionMatches(
    __inout PYORI_SH_INPUT_BUFFER Buffer
    );

/**
 If this flag is set, completion should match the full path rather than a
 relative file name.
 */
#define YORI_SH_TAB_COMPLETE_FULL_PATH      (0x00000001)

/**
 If this flag is set, completion should match command history rather than
 files or arguments.
 */
#define YORI_SH_TAB_COMPLETE_HISTORY        (0x00000002)

/**
 If this flag is set, completion should navigate backwards through the list
 of matches.
 */
#define YORI_SH_TAB_COMPLETE_BACKWARDS      (0x00000004)

VOID
YoriShTabCompletion(
    __inout PYORI_SH_INPUT_BUFFER Buffer,
    __in DWORD TabFlags
    );

VOID
YoriShTrimSuggestionList(
    __inout PYORI_SH_INPUT_BUFFER Buffer,
    __in PYORI_STRING NewString
    );

VOID
YoriShCompleteSuggestion(
    __inout PYORI_SH_INPUT_BUFFER Buffer
    );

// *** ENV.C ***

BOOL
YoriShIsEnvironmentVariableChar(
    __in TCHAR Char
    );

DWORD
YoriShGetEnvironmentVariableWithoutSubstitution(
    __in LPCTSTR Name,
    __out_opt LPTSTR Variable,
    __in DWORD Size,
    __out_opt PDWORD Generation
    );

BOOL
YoriShGetEnvironmentVariable(
    __in LPCTSTR Name,
    __out_opt LPTSTR Variable,
    __in DWORD Size,
    __out PDWORD ReturnedSize,
    __out_opt PDWORD Generation
    );

BOOL
YoriShAllocateAndGetEnvironmentVariable(
    __in LPCTSTR Name,
    __out PYORI_STRING Value,
    __out_opt PDWORD Generation
    );

BOOL
YoriShExpandEnvironmentVariables(
    __in PYORI_STRING Expression,
    __out PYORI_STRING ResultingExpression
    );

BOOL
YoriShSetEnvironmentVariable(
    __in PYORI_STRING VariableName,
    __in_opt PYORI_STRING Value
    );

BOOL
YoriShSetEnvironmentStrings(
    __in PYORI_STRING NewEnv
    );

// *** EXEC.C ***

DWORD
YoriShInitializeRedirection(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext,
    __in BOOL PrepareForBuiltIn,
    __out PYORI_SH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext
    );

VOID
YoriShRevertRedirection(
    __in PYORI_SH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext
    );

DWORD
YoriShExecuteSingleProgram(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    );

BOOL
YoriShExecuteExpressionAndCaptureOutput(
    __in PYORI_STRING Expression,
    __out PYORI_STRING ProcessOutput
    );

BOOL
YoriShExpandBackquotes(
    __in PYORI_STRING Expression,
    __out PYORI_STRING ResultingExpression
    );

BOOL
YoriShExecuteExpression(
    __in PYORI_STRING Expression
    );

// *** HISTORY.C ***

BOOL
YoriShAddToHistory(
    __in PYORI_STRING NewCmd
    );

BOOL
YoriShAddToHistoryAndReallocate(
    __in PYORI_STRING NewCmd
    );

VOID
YoriShRemoveOneHistoryEntry(
    __in PYORI_SH_HISTORY_ENTRY HistoryEntry
    );

VOID
YoriShClearAllHistory();

BOOL
YoriShInitHistory();

BOOL
YoriShLoadHistoryFromFile();

BOOL
YoriShSaveHistoryToFile();

BOOL
YoriShGetHistoryStrings(
    __in DWORD MaximumNumber,
    __inout PYORI_STRING HistoryStrings
    );

// *** INPUT.C ***

BOOL
YoriShEnsureStringHasEnoughCharacters(
    __inout PYORI_STRING String,
    __in DWORD CharactersNeeded
    );

BOOL
YoriShGetExpression(
    __inout PYORI_STRING Expression
    );

VOID
YoriShCleanupInputContext();

// *** JOB.C ***

BOOL
YoriShCreateNewJob(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext,
    __in HANDLE hProcess,
    __in DWORD dwProcessId
    );

BOOL
YoriShScanJobsReportCompletion(
    __in BOOL TeardownAll
    );

DWORD
YoriShGetNextJobId(
    __in DWORD PreviousJobId
    );

BOOL
YoriShJobSetPriority(
    __in DWORD JobId,
    __in DWORD PriorityClass
    );

BOOL
YoriShTerminateJob(
    __in DWORD JobId
    );

VOID
YoriShJobWait(
    __in DWORD JobId
    );

BOOL
YoriShGetJobOutput(
    __in DWORD JobId,
    __inout PYORI_STRING Output,
    __inout PYORI_STRING Errors
    );

BOOL
YoriShPipeJobOutput(
    __in DWORD JobId,
    __in_opt HANDLE hPipeOutput,
    __in_opt HANDLE hPipeErrors
    );

BOOL
YoriShGetJobInformation(
    __in DWORD JobId,
    __out PBOOL HasCompleted,
    __out PBOOL HasOutput,
    __out PDWORD ExitCode,
    __inout PYORI_STRING Command
    );

// *** MAIN.C ***

VOID
YoriShPreCommand();

// *** PARSE.C ***

BOOL
YoriShParseCmdlineToCmdContext(
    __in PYORI_STRING CmdLine,
    __in DWORD CurrentOffset,
    __out PYORI_SH_CMD_CONTEXT CmdContext
    );

LPTSTR
YoriShBuildCmdlineFromCmdContext(
    __in PYORI_SH_CMD_CONTEXT CmdContext,
    __in BOOL RemoveEscapes,
    __out_opt PDWORD BeginCurrentArg,
    __out_opt PDWORD EndCurrentArg
    );

BOOL
YoriShRemoveEscapesFromCmdContext(
    __in PYORI_SH_CMD_CONTEXT CmdContext
    );

VOID
YoriShCopyArg(
    __in PYORI_SH_CMD_CONTEXT SrcCmdContext,
    __in DWORD SrcArgument,
    __in PYORI_SH_CMD_CONTEXT DestCmdContext,
    __in DWORD DestArgument
    );

BOOL
YoriShCopyCmdContext(
    __out PYORI_SH_CMD_CONTEXT DestCmdContext,
    __in PYORI_SH_CMD_CONTEXT SrcCmdContext
    );

VOID
YoriShCheckIfArgNeedsQuotes(
    __in PYORI_SH_CMD_CONTEXT CmdContext,
    __in DWORD ArgIndex
    );

VOID
YoriShFreeCmdContext(
    __in PYORI_SH_CMD_CONTEXT CmdContext
    );

VOID
YoriShFreeExecContext(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    );

VOID
YoriShFreeExecPlan(
    __in PYORI_SH_EXEC_PLAN ExecPlan
    );

BOOL
YoriShParseCmdContextToExecPlan(
    __in PYORI_SH_CMD_CONTEXT CmdContext,
    __out PYORI_SH_EXEC_PLAN ExecPlan,
    __out_opt PYORI_SH_SINGLE_EXEC_CONTEXT* CurrentExecContext,
    __out_opt PBOOL CurrentArgIsForProgram,
    __out_opt PDWORD CurrentArgIndex
    );

BOOL
YoriShDoesExpressionSpecifyPath(
    __in PYORI_STRING SearchFor
    );

BOOL
YoriShResolveCommandToExecutable(
    __in PYORI_SH_CMD_CONTEXT CmdContext,
    __out PBOOL ExecutableFound
    );

BOOL
YoriShFindNextBackquoteSubstring(
    __in PYORI_STRING String,
    __out PYORI_STRING CurrentSubset,
    __out PDWORD CharsInPrefix
    );

BOOL
YoriShFindBestBackquoteSubstringAtOffset(
    __in PYORI_STRING String,
    __in DWORD StringOffset,
    __out PYORI_STRING CurrentSubset
    );

// *** PROMPT.C ***
BOOL
YoriShDisplayPrompt();

// *** RESTART.C ***

BOOL
YoriShSaveRestartState();

BOOL
YoriShLoadSavedRestartState(
    __in PYORI_STRING ProcessId
    );

VOID
YoriShDiscardSavedRestartState(
    __in_opt PYORI_STRING ProcessId
    );

// *** WINDOW.C ***

/**
 Indicates the task has succeeded and the taskbar button should be green.
 */
#define YORI_SH_TASK_SUCCESS     0x01

/**
 Indicates the task has failed and the taskbar button should be red.
 */
#define YORI_SH_TASK_FAILED      0x02

/**
 Indicates the task is in progress and the taskbar button should be yellow.
 */
#define YORI_SH_TASK_IN_PROGRESS 0x03

/**
 Indicates the task is complete and the taskbar button should be normal.
 */
#define YORI_SH_TASK_COMPLETE    0x04

BOOL
YoriShSetWindowState(
    __in DWORD State
    );

// *** YORISTD.C / YORINONE.C ***

DWORD
YoriShDefaultAliasEntriesCount();

// vim:sw=4:ts=4:et:
