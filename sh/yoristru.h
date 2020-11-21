/**
 * @file sh/yoristru.h
 *
 * Yori shell structures header file
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

/**
 Information about each argument in an enumerated list of arguments.
 */
typedef struct _YORI_SH_ARG_CONTEXT {

    /**
     TRUE if the argument is enclosed in quotes.
     */
    BOOL Quoted;

} YORI_SH_ARG_CONTEXT, *PYORI_SH_ARG_CONTEXT;

/**
 A command line that has been broken up into a series of arguments.
 */
typedef struct _YORI_SH_CMD_CONTEXT {

    /**
     The number of arguments.
     */
    DWORD ArgC;

    /**
     An array of pointers to each argument.  Each of these arguments has been
     referenced and should be dereferenced when no longer needed.
     */
    PYORI_STRING ArgV;

    /**
     An array of information about each argument, including the object that
     was referenced for each.
     */
    PYORI_SH_ARG_CONTEXT ArgContexts;

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
} YORI_SH_CMD_CONTEXT, *PYORI_SH_CMD_CONTEXT;

/**
 For every process that the shell launches which is subject to debugging, it
 may receive debug events from child processes launched by that process.
 These are otherwise unknown to the shell.  For the most part, it has no
 reason to care about these processes, except the minimum to allow the
 debugging logic to operate correctly.
 */
typedef struct _YORI_SH_DEBUGGED_CHILD_PROCESS {

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

} YORI_SH_DEBUGGED_CHILD_PROCESS, *PYORI_SH_DEBUGGED_CHILD_PROCESS;

/**
 Information about how to execute a single program.  The program may be
 internal or external.
 */
typedef struct _YORI_SH_SINGLE_EXEC_CONTEXT {

    /**
     The set of arguments to invoke the program with.
     */
    YORI_SH_CMD_CONTEXT CmdToExec;

    /**
     Pointer to the next program in an execution chain or NULL if there is
     no next program.
     */
    struct _YORI_SH_SINGLE_EXEC_CONTEXT * NextProgram;

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

} YORI_SH_SINGLE_EXEC_CONTEXT, *PYORI_SH_SINGLE_EXEC_CONTEXT;

/**
 When programs are executed, they temporarily modify the stdin/stdout/stderr
 of the shell process.  This structure contains information needed to revert
 back to the previous behavior.
 */
typedef struct _YORI_SH_PREVIOUS_REDIRECT_CONTEXT {

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
} YORI_SH_PREVIOUS_REDIRECT_CONTEXT, *PYORI_SH_PREVIOUS_REDIRECT_CONTEXT;

/**
 A plan to execute multiple programs.
 */
typedef struct _YORI_SH_EXEC_PLAN {

    /**
     Pointer to the first program to execute.  It will link to subsequent
     programs to execute.  This is used when each program is executed by this
     instance of the shell; if the shell decides to create a subshell, the
     EntireCmd below is used instead.
     */
    PYORI_SH_SINGLE_EXEC_CONTEXT FirstCmd;

    /**
     The total number of programs in the FirstCmd program list.
     */
    ULONG NumberCommands;

    /**
     The entire command to execute.  This is used if execution decides to hand
     this plan to a subshell process.  If the current shell is executing the
     plan, the FirstCmd list above is used instead.
     */
    YORI_SH_SINGLE_EXEC_CONTEXT EntireCmd;

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

} YORI_SH_EXEC_PLAN, *PYORI_SH_EXEC_PLAN;

/**
 Information about a previous command executed by the user.
 */
typedef struct _YORI_SH_HISTORY_ENTRY {

    /**
     The links for this history entry.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The command that was executed by the user.
     */
    YORI_STRING CmdLine;
} YORI_SH_HISTORY_ENTRY, *PYORI_SH_HISTORY_ENTRY;

/**
 Information about a single tab complete match.
 */
typedef struct _YORI_SH_TAB_COMPLETE_MATCH {

    /**
     The list entry for this match.  Paired with @ref
     YORI_SH_TAB_COMPLETE_CONTEXT::MatchList .
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The hash entry for this match.  Paired with @ref
     YORI_SH_TAB_COMPLETE_CONTEXT::MatchHashTable .
     */
    YORI_HASH_ENTRY HashEntry;

    /**
     The string corresponding to this match.
     */
    YORI_STRING Value;

    /**
     The offset to place the cursor within the match.  If zero, the cursor
     is placed at the end of the string.  Note that it generally wouldn't
     make sense for tab complete (which is generating characters) to then
     want the cursor to be at the true beginning of the string.
     */
    DWORD CursorOffset;

} YORI_SH_TAB_COMPLETE_MATCH, *PYORI_SH_TAB_COMPLETE_MATCH;

/**
 A set of tab completion match types that can be performed.
 */
typedef enum _YORI_SH_TAB_COMPLETE_SEARCH_TYPE {
    YoriTabCompleteSearchExecutables = 1,
    YoriTabCompleteSearchFiles = 2,
    YoriTabCompleteSearchHistory = 3,
    YoriTabCompleteSearchArguments = 4
} YORI_SH_TAB_COMPLETE_SEARCH_TYPE;

/**
 Information about the state of tab completion.
 */
typedef struct _YORI_SH_TAB_COMPLETE_CONTEXT {

    /**
     Indicates the number of times tab has been repeatedly pressed.  This
     is reset if any other key is pressed instead of tab.  It is used to
     determine if the tab context requires initialization for the first
     tab, and where to resume from for later tabs.
     */
    DWORD TabCount;

    /**
     Indicates the number of characters of valid data exists in an argument
     when making suggestions.  Not used outside of suggestions.
     */
    DWORD CurrentArgLength;

    /**
     Indicates the TabFlags passed when building any match list.  This is
     used to detect a later, incompatible set of flags that implies the
     list should be reconstructed.
     */
    DWORD TabFlagsUsedCreatingList;

    /**
     Indicates which data source to search through.
     */
    YORI_SH_TAB_COMPLETE_SEARCH_TYPE SearchType;

    /**
     TRUE if later compares should be case sensitive.  This is used when
     refining suggestions.
     */
    BOOLEAN CaseSensitive;

    /**
     TRUE if when populating suggestions entries were skipped due to not
     being a full prefix match.  This implies a subsequent tab completion
     needs to recompute.
     */
    BOOLEAN PotentialNonPrefixMatch;

    /**
     A list of matches that apply to the criteria that was searched.
     */
    YORI_LIST_ENTRY MatchList;

    /**
     A hash table of matches that apply to the criteria that was searched.
     This is used to efficiently strip duplicates.
     */
    PYORI_HASH_TABLE MatchHashTable;

    /**
     Pointer to the previously returned match.  If the user repeatedly hits
     tab, we advance to the next match.
     */
    PYORI_SH_TAB_COMPLETE_MATCH PreviousMatch;

    /**
     The matching criteria that is being searched for.  This is typically
     the string that was present when the user first hit tab followed by
     a "*".
     */
    YORI_STRING SearchString;

    /**
     The offset in characters from the beginning of SearchString to where
     the cursor currently is.
     */
    DWORD SearchStringOffset;

} YORI_SH_TAB_COMPLETE_CONTEXT, *PYORI_SH_TAB_COMPLETE_CONTEXT;

/**
 The context of a line that is currently being entered by the user.
 */
typedef struct _YORI_SH_INPUT_BUFFER {

    /**
     Handle to standard input when it's a console.
     */
    HANDLE ConsoleInputHandle;

    /**
     Handle to standard output when it's a console.
     */
    HANDLE ConsoleOutputHandle;

    /**
     Pointer to a string containing the text as being entered by the user.
     */
    YORI_STRING String;

    /**
     The current offset within @ref String that the user is modifying.
     */
    DWORD CurrentOffset;

    /**
     The number of characters that were filled in prior to a key press
     being evaluated.
     */
    DWORD PreviousCharsDisplayed;

    /**
     The current position that was selected prior to a key press being
     evaluated.
     */
    DWORD PreviousCurrentOffset;

    /**
     The number of times the tab key had been pressed prior to a key being
     evaluated.
     */
    DWORD PriorTabCount;

    /**
     The first character in the buffer that may have changed since the last
     draw.
     */
    DWORD DirtyBeginOffset;

    /**
     The last character in the buffer that may have changed since the last
     draw.
     */
    DWORD DirtyLength;

    /**
     TRUE if the input should be in insert mode, FALSE if it should be
     overwrite mode.
     */
    BOOL InsertMode;

    /**
     Information about how to display the cursor.
     */
    CONSOLE_CURSOR_INFO CursorInfo;

    /**
     The size of the console buffer.  Used to detect and avoid processing
     bogus buffer resize notifications.
     */
    COORD ConsoleBufferDimensions;

    /**
     Pointer to the currently selected history entry when navigating through
     history.
     */
    PYORI_LIST_ENTRY HistoryEntryToUse;

    /**
     When inputting a character by value, the current value that has been
     accumulated (since this requires multiple key events.)
     */
    DWORD NumericKeyValue;

    /**
     Indicates how to interpret the NumericKeyValue.  Ascii uses CP_OEMCP,
     Ansi uses CP_ACP, Unicode is direct.  Also note that Unicode takes
     input in hexadecimal to match the normal U+xxxx specification.
     */
    YORI_LIB_NUMERIC_KEY_TYPE NumericKeyType;

    /**
     The last known bitmask of mouse button state.
     */
    DWORD PreviousMouseButtonState;

    /**
     The tick count when the window was last made active, or zero if a
     window activation has not been observed since this input line began.
     */
    DWORD WindowActivatedTick;

    /**
     The tick count when the selection was started, or zero if no selection
     has been started.
     */
    DWORD SelectionStartedTick;

    /**
     Description of the current selected region.
     */
    YORILIB_SELECTION Selection;

    /**
     Description of the current mouseover region.
     */
    YORILIB_SELECTION Mouseover;

    /**
     Extra information specific to tab completion processing.
     */
    YORI_SH_TAB_COMPLETE_CONTEXT TabContext;

    /**
     Set to TRUE if the suggestion string has changed and requires
     redisplay.
     */
    BOOLEAN SuggestionDirty;

    /**
     Set to TRUE if the suggestion candidates have been calculated and do not
     need to be recalculated.  Set to FALSE if a subsequent change to the
     buffer invalidates the previously calculated set.
     */
    BOOLEAN SuggestionPopulated;

    /**
     The currently active suggestion string.
     */
    YORI_STRING SuggestionString;

    /**
     If TRUE, the search buffer is the active buffer where keystrokes and
     backspace keys should be delivered to.  If FALSE, keystrokes are
     added to the input buffer.
     */
    BOOL SearchMode;

    /**
     The offset as it was when the search operation started.  This is used
     if no match is found or a search is cancelled.
     */
    DWORD PreSearchOffset;

    /**
     The current search string, when searching within the buffer itself.
     */
    YORI_STRING SearchString;

} YORI_SH_INPUT_BUFFER, *PYORI_SH_INPUT_BUFFER;

/**
 A structure defining a mapping between a command name and a function to
 execute.  This is used to populate builtin commands.
 */
typedef struct _YORI_SH_BUILTIN_NAME_MAPPING {

    /**
     The command name.
     */
    LPTSTR CommandName;

    /**
     Pointer to the function to execute.
     */
    PYORI_CMD_BUILTIN BuiltinFn;
} YORI_SH_BUILTIN_NAME_MAPPING, *PYORI_SH_BUILTIN_NAME_MAPPING;

/**
 A structure defining an initial mapping of alias to value.
 */
typedef struct _YORI_SH_DEFAULT_ALIAS_ENTRY {

    /**
     The initial alias name.
     */
    LPTSTR Alias;

    /**
     The initial value.
     */
    LPTSTR Value;
} YORI_SH_DEFAULT_ALIAS_ENTRY, *PYORI_SH_DEFAULT_ALIAS_ENTRY;

extern CONST YORI_SH_DEFAULT_ALIAS_ENTRY YoriShDefaultAliasEntries[];

/**
 A structure containing information about a currently loaded DLL.
 */
typedef struct _YORI_SH_LOADED_MODULE {

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
} YORI_SH_LOADED_MODULE, *PYORI_SH_LOADED_MODULE;

/**
 A structure containing an individual builtin callback.
 */
typedef struct _YORI_SH_BUILTIN_CALLBACK {

    /**
     Links between the registered builtin callbacks.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The name of the callback.
     */
    YORI_STRING BuiltinName;

    /**
     The hash entry for this match.  Paired with @ref YoriShBuiltinHash .
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
    PYORI_SH_LOADED_MODULE ReferencedModule;

} YORI_SH_BUILTIN_CALLBACK, *PYORI_SH_BUILTIN_CALLBACK;


/**
 A structure containing state that is global across the Yori shell process.
 */
typedef struct _YORI_SH_GLOBALS {

    /**
     The exit code ("error level") of the previous process to complete.
     */
    DWORD ErrorLevel;

    /**
     When ExitProcess is set to TRUE, this is set to the code that the shell
     should return as its exit code.
     */
    DWORD ExitProcessExitCode;

    /**
     The most recent Job ID that was assigned.
    */
    DWORD PreviousJobId;

    /**
     A pointer to the command context of the currently active builtin
     command before escapes have been removed.  This can be given to a
     builtin if it needs to know the original escaped string.
     */
    PYORI_SH_CMD_CONTEXT EscapedCmdContext;

    /**
     Count of recursion depth.  This is incremented when calling a builtin
     or when the shell is invoked from a subshell, and decremented when
     these return.  A recursion depth of zero implies a shell ready for user
     interaction.
     */
    DWORD RecursionDepth;

    /**
     Count of prompt recursion depth.  This is the number of characters to
     display when $+$ is used.
     */
    DWORD PromptRecursionDepth;

    /**
     The current revision number of the environment variables in the process.
     This is incremented whenever a change occurs to the environment which
     may imply that cached state about shell behavior needs to be reloaded.
     */
    DWORD EnvironmentGeneration;

    /**
     The number of ms to wait before suggesting the completion to a command.
     */
    DWORD DelayBeforeSuggesting;

    /**
     The minimum number of characters that the user must enter before
     suggestions occur.
     */
    DWORD MinimumCharsInArgBeforeSuggesting;

    /**
     The generation of the environment last time input parameters were
     refreshed.
     */
    DWORD InputParamsGeneration;

    /**
     A handle to a thread which is saving restart state.  Note that this may
     be NULL if no thread has been created or if it has completed.
     */
    HANDLE RestartSaveThread;

    /**
     List of command history.
     */
    YORI_LIST_ENTRY CommandHistory;

    /**
     List of builtin callbacks currently registered with Yori.
     */
    YORI_LIST_ENTRY BuiltinCallbacks;

    /**
     The contents of the YORIPRECMD environment variable.
     */
    YORI_STRING PreCmdVariable;

    /**
     The generation of the environment at the time the variable was queried.
     */
    DWORD PreCmdGeneration;

    /**
     The contents of the YORIPOSTCMD environment variable.
     */
    YORI_STRING PostCmdVariable;

    /**
     The generation of the environment at the time the variable was queried.
     */
    DWORD PostCmdGeneration;

    /**
     The contents of the YORIPROMPT environment variable.
     */
    YORI_STRING PromptVariable;

    /**
     The generation of the environment at the time the variable was queried.
     */
    DWORD PromptGeneration;

    /**
     The contents of the YORITITLE environment variable.
     */
    YORI_STRING TitleVariable;

    /**
     The generation of the environment at the time the variable was queried.
     */
    DWORD TitleGeneration;

    /**
     The offset within NextCommand to initialize the cursor to.
     */
    DWORD NextCommandOffset;

    /**
     The text to use as a prepopulated string for the next user command.
     */
    YORI_STRING NextCommand;

    /**
     When set to TRUE, the process should end rather than seek another
     command.
     */
    BOOLEAN ExitProcess;

    /**
     When set to TRUE, indicates this process has been spawned as a subshell
     to execute builtin commands from a monolithic shell.
     */
    BOOLEAN SubShell;

    /**
     Set to TRUE once the process has initialized COM.
     */
    BOOLEAN InitializedCom;

    /**
     Set to TRUE if the process has set the taskbar button to any non-default
     state.
     */
    BOOLEAN TaskUiActive;

    /**
     Set to TRUE to indicate child processes launched without explicit user
     request, including to generate prompts and titles.  This implies ignoring
     any task UI state updates, and not processing input when waiting for
     process completion.
     */
    BOOLEAN ImplicitSynchronousTaskActive;

    /**
     Set to TRUE to disable the console's quickedit before inputting
     commands and enable it before executing them.
     */
    BOOLEAN YoriQuickEdit;

    /**
     Set to TRUE to include a trailing backslash when completing a directory
     as part of tab completion or suggestions.
     */
    BOOLEAN CompletionTrailingSlash;

    /**
     Set to TRUE to tell tab completion to list out all options when multiple
     are available instead of selecting the first.
     */
    BOOLEAN CompletionListAll;

    /**
     TRUE if mouseover support is enabled, FALSE if it is disabled.  Note this
     is currently enabled by default.
     */
    BOOL MouseoverEnabled;

    /**
     The Win32 color to use when changing text color due to a mouse over.
     */
    WORD MouseoverColor;

    /**
     The current yanked string, similar to the system clipboard but used only
     for the kill and yank commands (Ctrl+K and Ctrl+Y).
     */
    YORI_STRING YankBuffer;

} YORI_SH_GLOBALS, *PYORI_SH_GLOBALS;

extern YORI_SH_GLOBALS YoriShGlobal;

// vim:sw=4:ts=4:et:
