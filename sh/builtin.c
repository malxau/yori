/**
 * @file /sh/builtin.c
 *
 * Yori shell built in function handler
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
 Invoke a different program to complete executing the command.  This may be
 Powershell for its scripts, YS for its scripts, etc.

 @param ExecContext Pointer to the exec context for a command that is known
        to be implemented by an external program.

 @param ExtraArgCount The number of extra arguments that should be inserted
        into the beginning of the ExecContext, followed by a set of NULL
        terminated strings to insert into the ExecContext.

 @return ExitCode from process, or nonzero on failure.
 */
DWORD
YoriShBuckPass (
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __in YORI_ALLOC_SIZE_T ExtraArgCount,
    ...
    )
{
    YORI_LIBSH_CMD_CONTEXT OldCmdContext;
    DWORD ExitCode = 1;
    YORI_ALLOC_SIZE_T count;
    BOOL ExecAsBuiltin = FALSE;
    PVOID MemoryToFree;
    va_list marker;

    memcpy(&OldCmdContext, &ExecContext->CmdToExec, sizeof(YORI_LIBSH_CMD_CONTEXT));

    ExecContext->CmdToExec.ArgC = ExecContext->CmdToExec.ArgC + ExtraArgCount;
    MemoryToFree = YoriLibReferencedMalloc((YORI_ALLOC_SIZE_T)ExecContext->CmdToExec.ArgC * (sizeof(YORI_STRING) + sizeof(YORI_LIBSH_ARG_CONTEXT)));
    if (MemoryToFree == NULL) {
        memcpy(&ExecContext->CmdToExec, &OldCmdContext, sizeof(YORI_LIBSH_CMD_CONTEXT));
        return ExitCode;
    }

    ExecContext->CmdToExec.ArgV = MemoryToFree;
    ExecContext->CmdToExec.MemoryToFreeArgV = MemoryToFree;

    YoriLibReference(MemoryToFree);
    ExecContext->CmdToExec.ArgContexts = (PYORI_LIBSH_ARG_CONTEXT)YoriLibAddToPointer(ExecContext->CmdToExec.ArgV, sizeof(YORI_STRING) * ExecContext->CmdToExec.ArgC);
    ExecContext->CmdToExec.MemoryToFreeArgContexts = MemoryToFree;
    ZeroMemory(ExecContext->CmdToExec.ArgContexts, sizeof(YORI_LIBSH_ARG_CONTEXT) * ExecContext->CmdToExec.ArgC);


    va_start(marker, ExtraArgCount);
    for (count = 0; count < ExtraArgCount; count++) {
        LPTSTR NewArg = (LPTSTR)va_arg(marker, LPTSTR);
        YORI_STRING YsNewArg;
        YORI_STRING FoundInPath;

        YoriLibInitEmptyString(&ExecContext->CmdToExec.ArgV[count]);
        YoriLibConstantString(&YsNewArg, NewArg);

        //
        //  Search for the first extra argument in the path.  If we find it,
        //  execute as a program; if not, execute as a builtin.
        //

        if (count == 0) {
            YoriLibInitEmptyString(&FoundInPath);
            if (YoriLibLocateExecutableInPath(&YsNewArg, NULL, NULL, &FoundInPath) && FoundInPath.LengthInChars > 0) {
                memcpy(&ExecContext->CmdToExec.ArgV[0], &FoundInPath, sizeof(YORI_STRING));
                ASSERT(YoriLibIsStringNullTerminated(&ExecContext->CmdToExec.ArgV[0]));
                YoriLibInitEmptyString(&FoundInPath);
            } else {
                ExecAsBuiltin = TRUE;
                YoriLibConstantString(&ExecContext->CmdToExec.ArgV[count], NewArg);
            }

            YoriLibFreeStringContents(&FoundInPath);
        } else {
            YoriLibConstantString(&ExecContext->CmdToExec.ArgV[count], NewArg);
        }
    }
    va_end(marker);

    for (count = 0; count < OldCmdContext.ArgC; count++) {
        YoriLibShCopyArg(&OldCmdContext, count, &ExecContext->CmdToExec, count + ExtraArgCount);
    }

    YoriLibShCheckIfArgNeedsQuotes(&ExecContext->CmdToExec, 0);

    if (ExecAsBuiltin) {
        ExitCode = YoriShBuiltIn(ExecContext);
    } else {
        ExecContext->IncludeEscapesAsLiteral = TRUE;
        ExitCode = YoriShExecuteSingleProgram(ExecContext);
    }

    YoriLibShFreeCmdContext(&ExecContext->CmdToExec);
    memcpy(&ExecContext->CmdToExec, &OldCmdContext, sizeof(YORI_LIBSH_CMD_CONTEXT));

    return ExitCode;
}

/**
 Invoke CMD to execute a script.  This is different to regular child
 processes because CMD wants all arguments to be enclosed in quotes, in
 addition to quotes around individual arguments, as in:
 C:\\Windows\\System32\\Cmd.exe /c ""C:\\Path To\\Foo.cmd" "Arg To Foo""

 @param ExecContext Pointer to the exec context for a command that is known
        to be implemented by CMD.

 @return ExitCode from CMD, or nonzero on failure.
 */
DWORD
YoriShBuckPassToCmd (
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    YORI_LIBSH_CMD_CONTEXT OldCmdContext;
    YORI_STRING CmdLine;
    DWORD ExitCode = EXIT_SUCCESS;

    memcpy(&OldCmdContext, &ExecContext->CmdToExec, sizeof(YORI_LIBSH_CMD_CONTEXT));
    YoriLibInitEmptyString(&CmdLine);
    if (!YoriLibShBuildCmdlineFromCmdContext(&OldCmdContext, &CmdLine, FALSE, NULL, NULL)) {
        return EXIT_FAILURE;
    }

    if (!YoriLibShBuildCmdContextForCmdBuckPass(&ExecContext->CmdToExec, &CmdLine)) {
        YoriLibFreeStringContents(&CmdLine);
        return EXIT_FAILURE;
    }

    YoriLibFreeStringContents(&CmdLine);
    ExecContext->IncludeEscapesAsLiteral = TRUE;
    ExitCode = YoriShExecuteSingleProgram(ExecContext);

    YoriLibShFreeCmdContext(&ExecContext->CmdToExec);
    memcpy(&ExecContext->CmdToExec, &OldCmdContext, sizeof(YORI_LIBSH_CMD_CONTEXT));

    return ExitCode;
}

/**
 Call a builtin function.  This may be in a DLL or part of the main executable,
 but it is executed synchronously via a call rather than a CreateProcess.  The
 caller is required to reparse the ExecContext to expand environment variables,
 ensuring arguments are in their correct position after expansion.

 @param Fn The function associated with the builtin operation to call.

 @param ExecContext The context surrounding this particular function.  This
        is used to establish redirection context, but the command is expected
        to have already been reparsed on function entry to perform environment
        variable expansion.

 @param ArgC The number of arguments to execute.

 @param EscapedArgV An array of arguments to pass to the builtin.

 @return ExitCode, typically zero for success, nonzero for failure.
 */
int
YoriShExecuteInProc(
    __in PYORI_CMD_BUILTIN Fn,
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __in YORI_ALLOC_SIZE_T ArgC,
    __in PYORI_STRING EscapedArgV
    )
{
    YORI_LIBSH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext;
    BOOLEAN WasPipe = FALSE;
    PYORI_STRING NoEscapedArgV;
    PYORI_STRING SavedEscapedArgV;
    YORI_ALLOC_SIZE_T SavedEscapedArgC;
    YORI_ALLOC_SIZE_T Count;
    DWORD ExitCode = 0;

    NoEscapedArgV = NULL;

    //
    //  Remove the escapes from the command line.  This allows the builtin to
    //  have access to the escaped form if required.
    //

    NoEscapedArgV = YoriLibReferencedMalloc((YORI_ALLOC_SIZE_T)ArgC * sizeof(YORI_STRING));
    if (NoEscapedArgV == NULL) {
        ExitCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    for (Count = 0; Count < ArgC; Count++) {
        YoriLibCloneString(&NoEscapedArgV[Count], &EscapedArgV[Count]);
    }

    if (!YoriLibShRemoveEscapesFromArgCArgV(ArgC, NoEscapedArgV)) {
        ExitCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    //  We execute builtins on a single thread due to the amount of
    //  process wide state that could get messed up if we don't (eg.
    //  stdout.)  Unfortunately this means we can't natively implement
    //  things like pipe from builtins, because the builtin has to
    //  finish before the next process can start.  So if a pipe is
    //  requested, convert it into a buffer, and let the process
    //  finish.
    //

    if (ExecContext->StdOutType == StdOutTypePipe) {
        WasPipe = TRUE;
        ExecContext->StdOutType = StdOutTypeBuffer;
    }

    ExitCode = YoriLibShInitializeRedirection(ExecContext, TRUE, &PreviousRedirectContext);
    if (ExitCode != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    //  Unlike external processes, builtins need to start buffering
    //  before they start to ensure that output during execution has
    //  somewhere to go.
    //

    if (ExecContext->StdOutType == StdOutTypeBuffer) {
        if (ExecContext->StdOut.Buffer.ProcessBuffers != NULL) {
            if (YoriLibShAppendToExistingProcessBuffer(ExecContext)) {
                ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
            } else {
                ExecContext->StdOut.Buffer.ProcessBuffers = NULL;
            }
        } else {
            if (YoriLibShCreateNewProcessBuffer(ExecContext)) {
                ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
            }
        }
    }

    SavedEscapedArgC = YoriShGlobal.EscapedArgC;
    SavedEscapedArgV = YoriShGlobal.EscapedArgV;
    YoriShGlobal.EscapedArgC = ArgC;
    YoriShGlobal.EscapedArgV = EscapedArgV;
    YoriShGlobal.RecursionDepth++;
    ExitCode = Fn(ArgC, NoEscapedArgV);
    YoriShGlobal.RecursionDepth--;
    YoriShGlobal.EscapedArgC = SavedEscapedArgC;
    YoriShGlobal.EscapedArgV = SavedEscapedArgV;
    YoriLibShRevertRedirection(&PreviousRedirectContext);

    if (WasPipe) {
        YoriLibShForwardProcessBufferToNextProcess(ExecContext);
    } else {

        //
        //  Once the process has completed, if it's outputting to
        //  buffers, wait for the buffers to contain final data.
        //

        if (ExecContext->StdOutType == StdOutTypeBuffer &&
            ExecContext->StdOut.Buffer.ProcessBuffers != NULL)  {

            YoriLibShWaitForProcessBufferToFinalize(ExecContext->StdOut.Buffer.ProcessBuffers);
        }

        if (ExecContext->StdErrType == StdErrTypeBuffer &&
            ExecContext->StdErr.Buffer.ProcessBuffers != NULL) {

            YoriLibShWaitForProcessBufferToFinalize(ExecContext->StdErr.Buffer.ProcessBuffers);
        }
    }

Cleanup:

    if (NoEscapedArgV != NULL) {
        for (Count = 0; Count < ArgC; Count++) {
            YoriLibFreeStringContents(&NoEscapedArgV[Count]);
        }
        YoriLibDereference(NoEscapedArgV);
    }

    return ExitCode;
}

/**
 Execute a command contained in a DLL file.

 @param ModuleFileName Pointer to the full path to the DLL file.

 @param ExecContext Specifies all of the arguments that should be passed to
        the module.

 @param ExitCode On successful completion, updated to point to the exit code
        from the module.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShExecuteNamedModuleInProc(
    __in LPTSTR ModuleFileName,
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __out PDWORD ExitCode
    )
{
    PYORI_CMD_BUILTIN Main;
    PYORI_LIBSH_LOADED_MODULE Module;

    Module = YoriLibShLoadDll(ModuleFileName);
    if (Module == NULL) {
        return FALSE;
    }

    Main = (PYORI_CMD_BUILTIN)GetProcAddress(Module->ModuleHandle, "YoriMain");
    if (Main == NULL) {
        YoriLibShReleaseDll(Module);
        return FALSE;
    }

    if (Main) {
        PYORI_LIBSH_LOADED_MODULE PreviousModule;
        YORI_STRING CmdLine;
        YORI_ALLOC_SIZE_T ArgC;
        YORI_ALLOC_SIZE_T Count;
        PYORI_STRING EscapedArgV;

        //
        //  Build a command line, leaving all escapes in the command line.
        //

        YoriLibInitEmptyString(&CmdLine);
        if (!YoriLibShBuildCmdlineFromCmdContext(&ExecContext->CmdToExec, &CmdLine, FALSE, NULL, NULL)) {
            YoriLibShReleaseDll(Module);
            return FALSE;
        }

        //
        //  Parse the command line in the same way that a child process would.
        //

        ASSERT(YoriLibIsStringNullTerminated(&CmdLine));
        EscapedArgV = YoriLibCmdlineToArgcArgv(CmdLine.StartOfString, (DWORD)-1, TRUE, &ArgC);
        YoriLibFreeStringContents(&CmdLine);

        if (EscapedArgV == NULL) {
            YoriLibShReleaseDll(Module);
            return FALSE;
        }

        PreviousModule = YoriLibShGetActiveModule();
        YoriLibShSetActiveModule(Module);
        *ExitCode = YoriShExecuteInProc(Main, ExecContext, ArgC, EscapedArgV);
        YoriLibShSetActiveModule(PreviousModule);

        for (Count = 0; Count < ArgC; Count++) {
            YoriLibFreeStringContents(&EscapedArgV[Count]);
        }
        YoriLibDereference(EscapedArgV);
    }

    YoriLibShReleaseDll(Module);
    return TRUE;
}


/**
 Execute a function if we can't find it in the PATH.  Because Yori looks
 for programs in the path first, this function acts as a "last chance" to
 see if there's an internal implementation before we give up and fail.

 @param ExecContext Pointer to the exec context for this program.

 @return ExitCode, typically zero for success or nonzero on failure.
 */
DWORD
YoriShBuiltIn (
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    DWORD ExitCode = 1;
    PYORI_CMD_BUILTIN BuiltInCmd = NULL;
    PYORI_LIBSH_BUILTIN_CALLBACK CallbackEntry = NULL;
    PYORI_STRING CmdString;
    YORI_STRING CmdLine;
    YORI_ALLOC_SIZE_T ArgC;
    YORI_ALLOC_SIZE_T Count;
    PYORI_STRING EscapedArgV;

    //
    //  Lookup the builtin command.
    //

    CmdString = &ExecContext->CmdToExec.ArgV[0];
    CallbackEntry = YoriLibShLookupBuiltinByName(CmdString);

    //
    //  If the command is not found, split the command looking for the first
    //  period, slash or backslash.  Note by this point we know that no file
    //  was found from the string, and are only trying to find any builtin.
    //

    if (CallbackEntry == NULL) {
        PYORI_STRING NewString;
        YORI_ALLOC_SIZE_T LenBeforeSep;

        LenBeforeSep = YoriLibCountStringNotContainingChars(CmdString, _T("./\\"));
        if (LenBeforeSep < CmdString->LengthInChars) {

            if (!YoriLibShExpandCmdContext(&ExecContext->CmdToExec, 1, 1)) {
                return ExitCode;
            }

            CmdString = &ExecContext->CmdToExec.ArgV[0];
            NewString = &ExecContext->CmdToExec.ArgV[1];
            if (CmdString->MemoryToFree != NULL) {
                YoriLibReference(CmdString->MemoryToFree);
            }
            NewString->MemoryToFree = CmdString->MemoryToFree;
            NewString->StartOfString = &CmdString->StartOfString[LenBeforeSep];
            NewString->LengthInChars = CmdString->LengthInChars - LenBeforeSep;
            NewString->LengthAllocated = CmdString->LengthAllocated - LenBeforeSep;

            YoriLibShCheckIfArgNeedsQuotes(&ExecContext->CmdToExec, 1);

            CmdString->LengthInChars = LenBeforeSep;
            CmdString->LengthAllocated = LenBeforeSep;

            YoriShExpandAlias(&ExecContext->CmdToExec);

            CmdString = &ExecContext->CmdToExec.ArgV[0];
            CallbackEntry = YoriLibShLookupBuiltinByName(CmdString);
        }
    }

    //
    //  Build a command line, leaving all escapes in the command line.
    //

    YoriLibInitEmptyString(&CmdLine);
    if (!YoriLibShBuildCmdlineFromCmdContext(&ExecContext->CmdToExec, &CmdLine, FALSE, NULL, NULL)) {
        ExitCode = ERROR_OUTOFMEMORY;
        return ExitCode;
    }

    //
    //  Parse the command line in the same way that a child process would.
    //

    ASSERT(YoriLibIsStringNullTerminated(&CmdLine));
    EscapedArgV = YoriLibCmdlineToArgcArgv(CmdLine.StartOfString, (DWORD)-1, TRUE, &ArgC);
    YoriLibFreeStringContents(&CmdLine);

    if (EscapedArgV == NULL) {
        ExitCode = ERROR_OUTOFMEMORY;
        return ExitCode;
    }

    if (CallbackEntry != NULL) {
        BuiltInCmd = CallbackEntry->BuiltInFn;
    }

    if (BuiltInCmd) {
        PYORI_LIBSH_LOADED_MODULE PreviousModule;
        PYORI_LIBSH_LOADED_MODULE HostingModule = NULL;

        ASSERT(CallbackEntry != NULL);

        //
        //  If the function is in a module, reference the DLL to keep it alive
        //  until it returns.
        //

        if (CallbackEntry != NULL && CallbackEntry->ReferencedModule != NULL) {
            HostingModule = CallbackEntry->ReferencedModule;
            YoriLibShReferenceDll(HostingModule);
        }

        //
        //  Indicate which module is currently executing, and execute from it.
        //

        PreviousModule = YoriLibShGetActiveModule();
        YoriLibShSetActiveModule(HostingModule);
        ExitCode = YoriShExecuteInProc(BuiltInCmd, ExecContext, ArgC, EscapedArgV);
        ASSERT(YoriLibShGetActiveModule() == HostingModule);
        YoriLibShSetActiveModule(PreviousModule);

        if (HostingModule != NULL) {
            YoriLibShReleaseDll(HostingModule);
        }
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Unrecognized command: %y\n"), &EscapedArgV[0]);
        if (ExecContext->StdOutType == StdOutTypePipe &&
            ExecContext->NextProgram != NULL &&
            ExecContext->NextProgram->StdInType == StdInTypePipe) {

            ExecContext->NextProgramType = NextProgramExecNever;
        }
    }

    for (Count = 0; Count < ArgC; Count++) {
        YoriLibFreeStringContents(&EscapedArgV[Count]);
    }
    YoriLibDereference(EscapedArgV);

    return ExitCode;
}

/**
 Execute a command that is built in to the shell.  This can be used by in
 process extension modules.

 @param Expression The expression to execute.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShExecuteBuiltinString(
    __in PYORI_STRING Expression
    )
{
    YORI_LIBSH_EXEC_PLAN ExecPlan;
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext;

    //
    //  Parse the expression we're trying to execute.
    //

    if (!YoriLibShParseCmdlineToCmdContext(Expression, 0, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        return FALSE;
    }

    if (CmdContext.ArgC == 0) {
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (!YoriShExpandEnvironmentInCmdContext(&CmdContext)) {
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (!YoriLibShParseCmdContextToExecPlan(&CmdContext, &ExecPlan, NULL, NULL, NULL, NULL)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    ExecContext = ExecPlan.FirstCmd;

    if (ExecContext->NextProgram != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Attempt to invoke multi component expression as a builtin\n"));
        YoriLibShFreeExecPlan(&ExecPlan);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriShGlobal.ErrorLevel = YoriShBuiltIn(ExecContext);

    YoriLibShFreeExecPlan(&ExecPlan);
    YoriLibShFreeCmdContext(&CmdContext);

    return TRUE;
}

// vim:sw=4:ts=4:et:
