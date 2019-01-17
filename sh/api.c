/**
 * @file sh/api.c
 *
 * Yori exported API for modules to call
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

#include "yori.h"

/**
 Add a new, or replace an existing, shell alias.

 @param Alias The alias to add or update.

 @param Value The value to assign to the alias.

 @return TRUE if the alias was successfully updated, FALSE if it was not.
 */
BOOL
YoriApiAddAlias(
    __in PYORI_STRING Alias,
    __in PYORI_STRING Value
    )
{
    return YoriShAddAlias(Alias, Value, FALSE);
}

/**
 Add a new string to command history.

 @return TRUE if the string was successfully added, FALSE if not.
 */
BOOL
YoriApiAddHistoryString(
    __in PYORI_STRING NewCmd
    )
{
    return YoriShAddToHistoryAndReallocate(NewCmd);
}

/**
 Associate a new builtin command with a function pointer to be invoked when
 the command is specified.

 @param BuiltinCmd The command to register.

 @param CallbackFn The function to invoke in response to the command.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriApiBuiltinRegister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    )
{
    return YoriShBuiltinRegister(BuiltinCmd, CallbackFn);
}

/**
 Dissociate a previously associated builtin command such that the function is
 no longer invoked in response to the command.

 @param BuiltinCmd The command to unregister.

 @param CallbackFn The previously registered function to stop invoking in
        response to the command.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriApiBuiltinUnregister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    )
{
    return YoriShBuiltinUnregister(BuiltinCmd, CallbackFn);
}

/**
 Clear existing history strings.

 @return TRUE if the history strings were successfully deleted, FALSE if not.
 */
BOOL
YoriApiClearHistoryStrings(
    )
{
    YoriShClearAllHistory();
    return TRUE;
}

/**
 Delete an existing shell alias.

 @param Alias The alias to delete.

 @return TRUE if the alias was successfully deleted, FALSE if it was not
         found.
 */
BOOL
YoriApiDeleteAlias(
    __in PYORI_STRING Alias
    )
{
    return YoriShDeleteAlias(Alias);
}

/**
 Execute a builtin command.  Do not invoke path processing or look for any
 ways to execute the command that is not builtin to the shell.

 @param Expression Pointer to the command to invoke.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriApiExecuteBuiltin(
    __in PYORI_STRING Expression
    )
{
    return YoriShExecuteBuiltinString(Expression);
}

/**
 Parse and execute a command string.  This will internally perform parsing
 and redirection, as well as execute multiple subprocesses as needed.

 @param Expression The string to execute.

 @return TRUE to indicate it was successfully executed, FALSE otherwise.
 */
BOOL
YoriApiExecuteExpression(
    __in PYORI_STRING Expression
    )
{
    return YoriShExecuteExpression(Expression);
}

/**
 Terminates the currently running instance of Yori.

 @param ExitCode The exit code to return.
 */
VOID
YoriApiExitProcess(
    __in DWORD ExitCode
    )
{
    YoriShGlobal.ExitProcess = TRUE;
    YoriShGlobal.ExitProcessExitCode = ExitCode;
}

/**
 Take a user specified command and expand any alias contained within it.

 @param CommandString A string containing the command to expand.

 @param ExpandedString On successful completion, populated with a string
        with any aliases expanded.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriApiExpandAlias(
    __in PYORI_STRING CommandString,
    __out PYORI_STRING ExpandedString
    )
{
    return YoriShExpandAliasFromString(CommandString, ExpandedString);
}

/**
 Free a previously returned Yori string.

 @param String Pointer to the string to free.
 */
VOID
YoriApiFreeYoriString(
    __in PYORI_STRING String
    )
{
    YoriLibFreeStringContents(String);
}

/**
 Build the complete set of aliases into a an array of key value pairs
 and return a pointer to the result.  This must be freed with a
 subsequent call to @ref YoriApiFreeYoriString .

 @param AliasStrings Pointer to a string structure to populate with a newly
        allocated string containing a set of NULL terminated alias strings.

 @return TRUE to indicate success, or FALSE to indicate failure.
 */
BOOL
YoriApiGetAliasStrings(
    __out PYORI_STRING AliasStrings
    )
{
    YoriLibInitEmptyString(AliasStrings);
    return YoriShGetAliasStrings(FALSE, AliasStrings);
}

/**
 Return the most recently set exit code after a previous command completion.
 */
DWORD
YoriApiGetErrorLevel()
{
    return YoriShGlobal.ErrorLevel;
}

/**
 Build history into an array of NULL terminated strings terminated by an
 additional NULL terminator.  The result must be freed with a subsequent
 call to @ref YoriApiFreeYoriString .

 @param MaximumNumber Specifies the maximum number of lines of history to
        return.  This number refers to the most recent history entries.
        If this value is zero, all are returned.

 @param HistoryStrings On successful completion, populated with the set of
        history strings.

 @return Return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriApiGetHistoryStrings(
    __in DWORD MaximumNumber,
    __out PYORI_STRING HistoryStrings
    )
{
    YoriLibInitEmptyString(HistoryStrings);
    return YoriShGetHistoryStrings(MaximumNumber, HistoryStrings);
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
BOOL  
YoriApiGetJobInformation(
    __in DWORD JobId,
    __out PBOOL HasCompleted,
    __out PBOOL HasOutput,
    __out PDWORD ExitCode,
    __inout PYORI_STRING Command
    )
{
    return YoriShGetJobInformation(JobId, HasCompleted, HasOutput, ExitCode, Command);
}

/**
 Get any output buffers from a completed job, including stdout and stderr
 buffers.

 @param JobId Specifies the job ID to fetch buffers for.

 @param Output On successful completion, populated with any output in the job
        output buffer.  Free this with @ref YoriApiFreeYoriString .

 @param Errors On successful completion, populated with any output in the job
        error buffer.  Free this with @ref YoriApiFreeYoriString .

 @return TRUE to indicate success, FALSE to indicate error.
 */
BOOL
YoriApiGetJobOutput(
    __in DWORD JobId,
    __inout PYORI_STRING Output,
    __inout PYORI_STRING Errors
    )
{
    return YoriShGetJobOutput(JobId, Output, Errors);
}

/**
 Given a previous job ID, return the next ID that is currently executing.
 To commence from the beginning, specify a PreviousJobId of zero.

 @param PreviousJobId The ID that was previously returned, or zero to
        commence from the beginning.

 @return The next job ID, or zero if all have been enumerated.
 */
DWORD
YoriApiGetNextJobId(
    __in DWORD PreviousJobId
    )
{
    return YoriShGetNextJobId(PreviousJobId);
}

/**
 Returns the version number associated with this build of the Yori shell.

 @param MajorVersion On successful completion, updated to contain the major
        version number.

 @param MinorVersion On successful completion, updated to contain the minor
        version number.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriApiGetYoriVersion(
    __out PDWORD MajorVersion,
    __out PDWORD MinorVersion
    )
{
    *MajorVersion = YORI_VER_MAJOR;
    *MinorVersion = YORI_VER_MINOR;
    return TRUE;
}

/**
 Take any existing output from a job and send it to a pipe handle, and continue
 sending further output into the pipe handle.

 @param JobId Specifies the job ID to fetch buffers for.

 @param hPipeOutput Specifies a pipe to forward job standard output into.

 @param hPipeErrors Specifies a pipe to forward job standard error into.

 @return TRUE to indicate success, FALSE to indicate error.
 */
BOOL
YoriApiPipeJobOutput(
    __in DWORD JobId,
    __in_opt HANDLE hPipeOutput,
    __in_opt HANDLE hPipeErrors
    )
{
    return YoriShPipeJobOutput(JobId, hPipeOutput, hPipeErrors);
}

/**
 Sets the default color associated with the process.

 @param NewDefaultColor The new default color in Win32 format.
 */
VOID
YoriApiSetDefaultColor(
    __in WORD NewDefaultColor
    )
{
    YoriLibVtSetDefaultColor(NewDefaultColor);
}

/**
 Sets the priority associated with a job.

 @param JobId The ID to set priority for.

 @param PriorityClass The new priority class.

 @return TRUE to indicate that the priority class was changed, FALSE if it
         was not.
 */
BOOL
YoriApiSetJobPriority(
    __in DWORD JobId,
    __in DWORD PriorityClass
    )
{
    return YoriShJobSetPriority(JobId, PriorityClass);
}

/**
 Add a new function to invoke on shell exit or module unload.

 @param UnloadNotify Pointer to the function to invoke.

 @return TRUE if the callback was successfully added, FALSE if it was not.
 */
BOOL
YoriApiSetUnloadRoutine(
    __in PYORI_BUILTIN_UNLOAD_NOTIFY UnloadNotify
    )
{
    return YoriShSetUnloadRoutine(UnloadNotify);
}

/**
 Terminates a specified job.

 @param JobId The job to terminate.

 @return TRUE to indicate that the job was requested to terminate, FALSE if it
         was not.
 */
BOOL
YoriApiTerminateJob(
    __in DWORD JobId
    )
{
    return YoriShTerminateJob(JobId);
}

/**
 Waits until the specified job ID is no longer active.

 @param JobId The ID to wait for.
 */
VOID
YoriApiWaitForJob(
    __in DWORD JobId
    )
{
    YoriShJobWait(JobId);
}

// vim:sw=4:ts=4:et:
