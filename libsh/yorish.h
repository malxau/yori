/**
 * @file libsh/yorish.h
 *
 * Header for library routines that are of value to the shell or other
 * components performing very shell like behavior.
 *
 * Copyright (c) 2017-2020 Malcolm J. Smith
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


/**
 Information about each argument in an enumerated list of arguments.
 */
typedef struct _YORI_LIBSH_ARG_CONTEXT {

    /**
     TRUE if the argument is enclosed in quotes.
     */
    BOOLEAN Quoted;

    /**
     TRUE if the closing quote is present on the argument.  In rare cases it
     is possible to have a terminating quote of a non-quoted argument.
     This happens in cases such as '>"foo bar"'.  This logic still treats
     that argument as having a terminating quote to suppress suggestions
     after the quote; in general this value should only be used in
     conjunction with Quoted (indicating a leading quote) above.
     */
    BOOLEAN QuoteTerminated;

} YORI_LIBSH_ARG_CONTEXT, *PYORI_LIBSH_ARG_CONTEXT;

/**
 A command line that has been broken up into a series of arguments.
 */
typedef struct _YORI_LIBSH_CMD_CONTEXT {

    /**
     The number of arguments.
     */
    DWORD ArgC;

    /**
     An array of pointers to each argument.  Each of these arguments has been
     referenced and should be dereferenced when no longer needed.
     */
    _Field_size_(ArgC)
    PYORI_STRING ArgV;

    /**
     An array of information about each argument, including the object that
     was referenced for each.
     */
    _Field_size_(ArgC)
    PYORI_LIBSH_ARG_CONTEXT ArgContexts;

    /**
     When generating the command context, if a string offset is specified,
     this value contains the argument that the string offset would correspond
     to.
     */
    DWORD CurrentArg;

    /**
     When generating the command context, if a string offset is specified,
     this value contains the offset within the current argument that it would
     correspond to.
     */
    DWORD CurrentArgOffset;

    /**
     Memory to dereference when the context is torn down.  Typically this
     single allocation backs the argv and ArgContexts array, and often backs
     the contents of each of the arguments also.
     */
    PVOID MemoryToFree;

    /**
     TRUE if there are characters in the string following the final argument.
     FALSE if the final character in the string is part of an argument.
     */
    BOOL TrailingChars;
} YORI_LIBSH_CMD_CONTEXT, *PYORI_LIBSH_CMD_CONTEXT;

/**
 For every process that the shell launches which is subject to debugging, it
 may receive debug events from child processes launched by that process.
 These are otherwise unknown to the shell.  For the most part, it has no
 reason to care about these processes, except the minimum to allow the
 debugging logic to operate correctly.
 */
typedef struct _YORI_LIBSH_DEBUGGED_CHILD_PROCESS {

    /**
     The list of this debugged child within the process that was launched
     directly from the shell.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     A handle to the process.  This has been duplicated within this program
     and must be closed when freeing this structure.
     */
    HANDLE hProcess;

    /**
     A handle to the initial thread within the process.  This has been
     duplicated within this program and must be closed when freeing this
     structure.
     */
    HANDLE hInitialThread;

    /**
     The process identifier for this process.
     */
    DWORD dwProcessId;

    /**
     The identifier for the initial thread within the process.
     */
    DWORD dwInitialThreadId;

} YORI_LIBSH_DEBUGGED_CHILD_PROCESS, *PYORI_LIBSH_DEBUGGED_CHILD_PROCESS;

/**
 Information about how to execute a single program.  The program may be
 internal or external.
 */
typedef struct _YORI_LIBSH_SINGLE_EXEC_CONTEXT {

    /**
     The set of arguments to invoke the program with.
     */
    YORI_LIBSH_CMD_CONTEXT CmdToExec;

    /**
     Pointer to the next program in an execution chain or NULL if there is
     no next program.
     */
    struct _YORI_LIBSH_SINGLE_EXEC_CONTEXT * NextProgram;

    /**
     Reference count.  This structure should not be cleaned up or deallocated
     until this reaches zero.  Synchronized via Interlock.  Note that while
     an exec context is part of an exec plan, a reference is held from the
     plan.
     */
    DWORD ReferenceCount;

    /**
     Specifies the type of the next program and the conditions under which
     it should execute.
     */
    enum {
        NextProgramTypeNone = 0,
        NextProgramExecUnconditionally = 1,
        NextProgramExecConcurrently = 2,
        NextProgramExecOnFailure = 3,
        NextProgramExecOnSuccess = 4,
        NextProgramExecNever = 5
    } NextProgramType;

    /**
     Specifies the origin of stdin when invoking the program.
     */
    enum {
        StdInTypeDefault = 1,
        StdInTypeFile = 2,
        StdInTypeNull = 3,
        StdInTypePipe = 4
    } StdInType;

    /**
     Extra information specific to each type of stdin origin.
     */
    union {
        struct {
            YORI_STRING FileName;
        } File;
        struct {
            HANDLE PipeFromPriorProcess;
        } Pipe;
    } StdIn;

    /**
     Specifies the target of stdout when invoking the program.
     */
    enum {
        StdOutTypeDefault = 1,
        StdOutTypeOverwrite = 2,
        StdOutTypeAppend = 3,
        StdOutTypeNull = 4,
        StdOutTypePipe = 5,
        StdOutTypeBuffer = 6,
        StdOutTypeStdErr = 7
    } StdOutType;

    /**
     Extra information specific to each type of stdout target.
     */
    union {
        struct {
            YORI_STRING FileName;
        } Overwrite;
        struct {
            YORI_STRING FileName;
        } Append;
        struct {
            HANDLE PipeFromProcess;
            PVOID ProcessBuffers;
            BOOLEAN RetainBufferData;
        } Buffer;
    } StdOut;

    /**
     Specifies the target of stderr when invoking the program.
     */
    enum {
        StdErrTypeDefault = 1,
        StdErrTypeOverwrite = 2,
        StdErrTypeAppend = 3,
        StdErrTypeStdOut = 4,
        StdErrTypeNull = 5,
        StdErrTypeBuffer = 6
    } StdErrType;

    /**
     Extra information specific to each type of stderr target.
     */
    union {
        struct {
            YORI_STRING FileName;
        } Overwrite;
        struct {
            YORI_STRING FileName;
        } Append;
        struct {
            HANDLE PipeFromProcess;
            PVOID ProcessBuffers;
            BOOLEAN RetainBufferData;
        } Buffer;
    } StdErr;

    /**
     If the process has been launched, contains a handle to the child
     process.
     */
    HANDLE hProcess;

    /**
     Handle to the primary thread of the child process.
     */
    HANDLE hPrimaryThread;

    /**
     If the child process is being debugged, a list of ancestor processes
     which the debugging engine needs to reason about.
     */
    YORI_LIST_ENTRY DebuggedChildren;

    /**
     Handle to a debugger thread, if the child process is being debugged.
     */
    HANDLE hDebuggerThread;

    /**
     The process identifier of the child process if it has been launched.
     For some reason some APIs want this and others want the handle.
     */
    DWORD dwProcessId;

    /**
     TRUE if when the program is executed we should wait for it to complete.
     If FALSE, execution can resume immediately, either executing the next
     program or returning to the user for more input.
     */
    BOOLEAN WaitForCompletion;

    /**
     TRUE if any escape characters should be used literally (not removed)
     in any child process string.  This is used when the child process is
     being generated implicitly, for example an implied subshell.  If the
     user consciously generated a command, the shell would normally remove
     any escapes before the child command gets the string.
     */
    BOOLEAN IncludeEscapesAsLiteral;

    /**
     TRUE if the program should be executed on a different console to the
     one the user is operating on.
     */
    BOOLEAN RunOnSecondConsole;

    /**
     TRUE if the environment should be reloaded from the process on
     termination.  This implies WaitForCompletion is TRUE.
     */
    BOOLEAN CaptureEnvironmentOnExit;

    /**
     TRUE if the process was being waited upon and the user switched
     windows, triggering the shell to indicate completion in the
     taskbar.
     */
    BOOLEAN TaskCompletionDisplayed;

    /**
     Set to TRUE to indicate the process should not attempt to display task
     completion because it is believed to not be an active background
     process.
     */
    BOOLEAN SuppressTaskCompletion;

    /**
     Set to TRUE to indicate that the debug pump thread has completed
     processing.  This is currently only used for debugging, because the
     debug pump thread is still running at the point of the final dereference,
     even though it has completed processing debug messages.
     */
    BOOLEAN DebugPumpThreadFinished;

    /**
     Set to TRUE to indicate that the process should be given a chance to
     terminate gracefully with GenerateConsoleCtrlEvent.  If FALSE, the only
     way to end the process is with TerminateProcess.
     */
    BOOLEAN TerminateGracefully;

} YORI_LIBSH_SINGLE_EXEC_CONTEXT, *PYORI_LIBSH_SINGLE_EXEC_CONTEXT;

/**
 When programs are executed, they temporarily modify the stdin/stdout/stderr
 of the shell process.  This structure contains information needed to revert
 back to the previous behavior.
 */
typedef struct _YORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT {

    /**
     TRUE if stdin needs to be reset.
     */
    BOOLEAN ResetInput;

    /**
     TRUE if stdout needs to be reset.
     */
    BOOLEAN ResetOutput;

    /**
     TRUE if stderr needs to be reset.
     */
    BOOLEAN ResetError;

    /**
     TRUE if stdout and stderr have been modified to refer to the same
     location.
     */
    BOOLEAN StdErrRedirectsToStdOut;

    /**
     TRUE if stdout and stderr have been modified to refer to the same
     location.
     */
    BOOLEAN StdOutRedirectsToStdErr;

    /**
     A handle to the original stdin.
     */
    HANDLE StdInput;

    /**
     A handle to the original stdout.
     */
    HANDLE StdOutput;

    /**
     A handle to the original stderr.
     */
    HANDLE StdError;
} YORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT, *PYORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT;

/**
 A plan to execute multiple programs.
 */
typedef struct _YORI_LIBSH_EXEC_PLAN {

    /**
     Pointer to the first program to execute.  It will link to subsequent
     programs to execute.  This is used when each program is executed by this
     instance of the shell; if the shell decides to create a subshell, the
     EntireCmd below is used instead.
     */
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT FirstCmd;

    /**
     The total number of programs in the FirstCmd program list.
     */
    ULONG NumberCommands;

    /**
     The entire command to execute.  This is used if execution decides to hand
     this plan to a subshell process.  If the current shell is executing the
     plan, the FirstCmd list above is used instead.
     */
    YORI_LIBSH_SINGLE_EXEC_CONTEXT EntireCmd;

    /**
     TRUE if the process was being waited upon and the user switched
     windows, triggering the shell to indicate completion in the
     taskbar.
     */
    BOOLEAN TaskCompletionDisplayed;

    /**
     TRUE if when the plan is executed we should wait for it to complete.
     If FALSE, input can resume immediately.  This may require sending the
     plan to a child process which can execute the plan while the parent
     process continues to prompt for input.
     */
    BOOLEAN WaitForCompletion;

} YORI_LIBSH_EXEC_PLAN, *PYORI_LIBSH_EXEC_PLAN;

/**
 A structure containing information about a currently loaded DLL.
 */
typedef struct _YORI_LIBSH_LOADED_MODULE {

    /**
     The entry for this loaded module on the list of actively loaded
     modules.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     A string describing the DLL file name.
     */
    YORI_STRING DllName;

    /**
     A callback function to invoke on unload to facilitate cleanup.
     */
    PYORI_BUILTIN_UNLOAD_NOTIFY UnloadNotify;

    /**
     The reference count of this module.
     */
    ULONG ReferenceCount;

    /**
     A handle to the DLL.
     */
    HANDLE ModuleHandle;
} YORI_LIBSH_LOADED_MODULE, *PYORI_LIBSH_LOADED_MODULE;

/**
 A structure containing an individual builtin callback.
 */
typedef struct _YORI_LIBSH_BUILTIN_CALLBACK {

    /**
     Links between the registered builtin callbacks.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The name of the callback.
     */
    YORI_STRING BuiltinName;

    /**
     The hash entry for this match.
     */
    YORI_HASH_ENTRY HashEntry;

    /**
     A function pointer to the builtin.
     */
    PYORI_CMD_BUILTIN BuiltInFn;

    /**
     Pointer to a referenced module that implements this builtin function.
     This may be NULL if it's a function statically linked into the main
     executable.
     */
    PYORI_LIBSH_LOADED_MODULE ReferencedModule;

} YORI_LIBSH_BUILTIN_CALLBACK, *PYORI_LIBSH_BUILTIN_CALLBACK;

// *** BUILTIN.C ***

PYORI_LIBSH_LOADED_MODULE
YoriLibShLoadDll(
    __in LPTSTR DllName
    );

VOID
YoriLibShReleaseDll(
    __in PYORI_LIBSH_LOADED_MODULE LoadedModule
    );

VOID
YoriLibShReferenceDll(
    __in PYORI_LIBSH_LOADED_MODULE LoadedModule
    );

PYORI_LIBSH_LOADED_MODULE
YoriLibShGetActiveModule(VOID);

VOID
YoriLibShSetActiveModule(
    __in_opt PYORI_LIBSH_LOADED_MODULE NewModule
    );

PYORI_LIBSH_BUILTIN_CALLBACK
YoriLibShLookupBuiltinByName(
    __in PYORI_STRING Name
    );

PYORI_LIBSH_BUILTIN_CALLBACK
YoriLibShGetPreviousBuiltinCallback(
    __in_opt PYORI_LIBSH_BUILTIN_CALLBACK Existing
    );

__success(return)
BOOL
YoriLibShBuiltinRegister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    );

__success(return)
BOOL
YoriLibShBuiltinUnregister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    );

VOID
YoriLibShBuiltinUnregisterAll(VOID);

__success(return)
BOOL
YoriLibShSetUnloadRoutine(
    __in PYORI_BUILTIN_UNLOAD_NOTIFY UnloadNotify
    );

// *** CMDBUF.C ***

__success(return)
BOOL
YoriLibShCreateNewProcessBuffer(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    );

__success(return)
BOOL
YoriLibShAppendToExistingProcessBuffer(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    );

__success(return)
BOOL
YoriLibShForwardProcessBufferToNextProcess(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    );

VOID
YoriLibShDereferenceProcessBuffer(
    __in PVOID ThisBuffer
    );

VOID
YoriLibShReferenceProcessBuffer(
    __in PVOID ThisBuffer
    );

__success(return)
BOOL
YoriLibShGetProcessOutputBuffer(
    __in PVOID ThisBuffer,
    __out PYORI_STRING String
    );

__success(return)
BOOL
YoriLibShGetProcessErrorBuffer(
    __in PVOID ThisBuffer,
    __out PYORI_STRING String
    );

VOID
YoriLibShTeardownProcessBuffersIfCompleted(
    __in PVOID ThisBuffer
    );

__success(return)
BOOL
YoriLibShScanProcessBuffersForTeardown(
    __in BOOL TeardownAll
    );

__success(return)
BOOL
YoriLibShWaitForProcessBufferToFinalize(
    __in PVOID ThisBuffer
    );

__success(return)
BOOL
YoriLibShPipeProcessBuffers(
    __in PVOID ThisBuffer,
    __in_opt HANDLE hPipeOutput,
    __in_opt HANDLE hPipeErrors
    );

// *** EXEC.C ***

VOID
YoriLibShCleanupFailedProcessLaunch(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    );

VOID
YoriLibShCommenceProcessBuffersIfNeeded(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    );

DWORD
YoriLibShCreateProcess(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __in_opt LPTSTR CurrentDirectory,
    __out_opt PBOOL FailedInRedirection
    );

DWORD
YoriLibShInitializeRedirection(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __in BOOL PrepareForBuiltIn,
    __out PYORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext
    );

VOID
YoriLibShRevertRedirection(
    __in PYORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext
    );

BOOLEAN
YoriLibShBuildCmdContextForCmdBuckPass (
    __out PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __in PYORI_STRING CmdLine
    );

// *** PARSE.C ***

__success(return)
BOOLEAN
YoriLibShAllocateArgCount(
    __out PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __in DWORD ArgCount,
    __in DWORD ExtraByteCount,
    __out_opt PVOID *ExtraData
    );

__success(return)
BOOLEAN
YoriLibShBuildCmdlineFromCmdContext(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __inout PYORI_STRING CmdLine,
    __in BOOL RemoveEscapes,
    __out_opt PDWORD BeginCurrentArg,
    __out_opt PDWORD EndCurrentArg
    );

VOID
YoriLibShCheckIfArgNeedsQuotes(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __in DWORD ArgIndex
    );

__success(return)
BOOL
YoriLibShCopyCmdContext(
    __out PYORI_LIBSH_CMD_CONTEXT DestCmdContext,
    __in PYORI_LIBSH_CMD_CONTEXT SrcCmdContext
    );

VOID
YoriLibShCopyArg(
    __in PYORI_LIBSH_CMD_CONTEXT SrcCmdContext,
    __in DWORD SrcArgument,
    __in PYORI_LIBSH_CMD_CONTEXT DestCmdContext,
    __in DWORD DestArgument
    );

VOID
YoriLibShDereferenceExecContext(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __in BOOLEAN Deallocate
    );

__success(return)
BOOL
YoriLibShFindBestBackquoteSubstringAtOffset(
    __in PYORI_STRING String,
    __in DWORD StringOffset,
    __out PYORI_STRING CurrentSubset
    );

__success(return)
BOOL
YoriLibShFindNextBackquoteSubstring(
    __in PYORI_STRING String,
    __out PYORI_STRING CurrentSubset,
    __out PDWORD CharsInPrefix
    );

VOID
YoriLibShFreeCmdContext(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext
    );

VOID
YoriLibShFreeExecPlan(
    __in PYORI_LIBSH_EXEC_PLAN ExecPlan
    );

__success(return)
BOOLEAN
YoriLibShIsArgumentSeperator(
    __in PYORI_STRING String,
    __out_opt PDWORD CharsToConsumeOut,
    __out_opt PBOOLEAN TerminateArgOut
    );

__success(return)
BOOL
YoriLibShParseCmdContextToExecPlan(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __out PYORI_LIBSH_EXEC_PLAN ExecPlan,
    __out_opt PYORI_LIBSH_SINGLE_EXEC_CONTEXT* CurrentExecContext,
    __out_opt PBOOL CurrentArgIsForProgram,
    __out_opt PDWORD CurrentArgIndex,
    __out_opt PDWORD CurrentArgOffset
    );

__success(return)
BOOLEAN
YoriLibShParseCmdlineToCmdContext(
    __in PYORI_STRING CmdLine,
    __in DWORD CurrentOffset,
    __out PYORI_LIBSH_CMD_CONTEXT CmdContext
    );

VOID
YoriLibShReferenceExecContext(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    );

__success(return)
BOOLEAN
YoriLibShRemoveEscapesFromArgCArgV(
    __in DWORD ArgC,
    __inout PYORI_STRING ArgV
    );

__success(return)
BOOLEAN
YoriLibShRemoveEscapesFromCmdContext(
    __in PYORI_LIBSH_CMD_CONTEXT EscapedCmdContext,
    __out PYORI_LIBSH_CMD_CONTEXT NoEscapedCmdContext
    );

// vim:sw=4:ts=4:et:
