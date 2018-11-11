/**
 * @file lib/yoricall.h
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

BOOL
YoriCallAddAlias(
    __in PYORI_STRING Alias,
    __in PYORI_STRING Value
    );

BOOL
YoriCallAddHistoryString(
    __in PYORI_STRING NewCmd
    );

BOOL
YoriCallBuiltinRegister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    );

BOOL
YoriCallBuiltinUnregister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    );

BOOL
YoriCallClearHistoryStrings(
    );

BOOL
YoriCallDeleteAlias(
    __in PYORI_STRING Alias
    );

BOOL
YoriCallExecuteBuiltin(
    __in PYORI_STRING Expression
    );

BOOL
YoriCallExecuteExpression(
    __in PYORI_STRING Expression
    );

VOID
YoriCallExitProcess(
    __in DWORD ExitCode
    );

BOOL
YoriCallExpandAlias(
    __in PYORI_STRING CommandString,
    __in PYORI_STRING ExpandedString
    );

VOID
YoriCallFreeYoriString(
    __in PYORI_STRING String
    );

BOOL
YoriCallGetAliasStrings(
    __out PYORI_STRING AliasStrings
    );

DWORD
YoriCallGetErrorLevel(
    );

BOOL
YoriCallGetHistoryStrings(
    __in DWORD MaximumNumber,
    __out PYORI_STRING HistoryStrings
    );

BOOL
YoriCallGetJobInformation(
    __in DWORD JobId,
    __out PBOOL HasCompleted,
    __out PBOOL HasOutput,
    __out PDWORD ExitCode,
    __inout PYORI_STRING Command
    );

BOOL
YoriCallGetJobOutput(
    __in DWORD JobId,
    __out PYORI_STRING Output,
    __out PYORI_STRING Errors
    );

DWORD
YoriCallGetNextJobId(
    __in DWORD PreviousJobId
    );

BOOL
YoriCallGetYoriVersion(
    __out PDWORD MajorVersion,
    __out PDWORD MinorVersion
    );

BOOL
YoriCallPipeJobOutput(
    __in DWORD JobId,
    __in_opt HANDLE hPipeOutput,
    __in_opt HANDLE hPipeErrors
    );

BOOL
YoriCallSetDefaultColor(
    __in WORD NewDefaultColor
    );

BOOL
YoriCallSetJobPriority(
    __in DWORD JobId,
    __in DWORD PriorityClass
    );

BOOL
YoriCallSetUnloadRoutine(
    __in PYORI_BUILTIN_UNLOAD_NOTIFY UnloadNotify
    );

BOOL
YoriCallTerminateJob(
    __in DWORD JobId
    );

VOID
YoriCallWaitForJob(
    __in DWORD JobId
    );
