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
     A pointer to the argument count of the currently active builtin
     command before escapes have been removed.  This can be given to a
     builtin if it needs to know the original escaped string.
     */
    DWORD EscapedArgC;

    /**
     A pointer to the argument array of the currently active builtin
     command before escapes have been removed.  This can be given to a
     builtin if it needs to know the original escaped string.
     */
    PYORI_STRING EscapedArgV;

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
     Two buffers that contain the current directory.  After each command, this
     state is loaded into the next buffer, allowing comparison with the
     previous current directory.  The ActiveCurrentDirectory index below
     indicates which buffer to use.
     */
    YORI_STRING CurrentDirectoryBuffers[2];

    /**
     Which of the two current directory buffers above contains the current
     directory.
     */
    DWORD ActiveCurrentDirectory;

    /**
     The current yanked string, similar to the system clipboard but used only
     for the kill and yank commands (Ctrl+K and Ctrl+Y).
     */
    YORI_STRING YankBuffer;

    /**
     When set to TRUE, the output device supports VT sequences.  When FALSE,
     VT sequences are only supported by YoriLib.
     */
    BOOLEAN OutputSupportsVt;

    /**
     When set to TRUE, the capabilities of the output device for VT sequences
     have been determined.  When FALSE, the state is not yet known.
     */
    BOOLEAN OutputSupportsVtDetermined;

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
    BOOLEAN MouseoverEnabled;

    /**
     TRUE if this is an interactive shell, meaning one spawned without /c or
     /ss.  In particular, note that /k is considered interactive.
     */
    BOOLEAN InteractiveMode;

    /**
     The Win32 color to use when changing text color due to a mouse over.
     */
    WORD MouseoverColor;

} YORI_SH_GLOBALS, *PYORI_SH_GLOBALS;

extern YORI_SH_GLOBALS YoriShGlobal;

// vim:sw=4:ts=4:et:
