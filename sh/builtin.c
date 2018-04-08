/**
 * @file sh/builtin.c
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
 A structure containing information about a currently loaded DLL.
 */
typedef struct _YORI_LOADED_MODULE {

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
     The reference count of this module.
     */
    ULONG ReferenceCount;

    /**
     A handle to the DLL.
     */
    HANDLE ModuleHandle;
} YORI_LOADED_MODULE, *PYORI_LOADED_MODULE;

/**
 A structure containing an individual builtin callback.
 */
typedef struct _YORI_BUILTIN_CALLBACK {

    /**
     Links between the registered builtin callbacks.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The name of the callback.
     */
    YORI_STRING BuiltinName;

    /**
     A function pointer to the builtin.
     */
    PYORI_CMD_BUILTIN BuiltInFn;

    /**
     Pointer to a referenced module that implements this builtin function.
     This may be NULL if it's a function statically linked into the main
     executable.
     */
    PYORI_LOADED_MODULE ReferencedModule;

} YORI_BUILTIN_CALLBACK, *PYORI_BUILTIN_CALLBACK;

/**
 A list of currently loaded modules.
 */
YORI_LIST_ENTRY YoriShLoadedModules;

/**
 Load a DLL file into a loaded module object that can be referenced.

 @param DllName Pointer to a NULL terminated string of a DLL to load.

 @return Pointer a referenced loaded module.  This module is linked into
         the loaded module list.  NULL on failure.
 */
PYORI_LOADED_MODULE
YoriShLoadDll(
    __in LPTSTR DllName
    )
{
    PYORI_LOADED_MODULE FoundEntry = NULL;
    PYORI_LIST_ENTRY ListEntry;
    DWORD DllNameLength;

    if (YoriShLoadedModules.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShLoadedModules, NULL);
        while (ListEntry != NULL) {
            FoundEntry = CONTAINING_RECORD(ListEntry, YORI_LOADED_MODULE, ListEntry);
            if (YoriLibCompareStringWithLiteralInsensitive(&FoundEntry->DllName, DllName) == 0) {
                FoundEntry->ReferenceCount++;
                return FoundEntry;
            }

            ListEntry = YoriLibGetNextListEntry(&YoriShLoadedModules, ListEntry);
        }
    }

    DllNameLength = _tcslen(DllName);
    FoundEntry = YoriLibMalloc(sizeof(YORI_LOADED_MODULE) + (DllNameLength + 1) * sizeof(TCHAR));
    if (FoundEntry == NULL) {
        return NULL;
    }

    YoriLibInitEmptyString(&FoundEntry->DllName);
    FoundEntry->DllName.StartOfString = (LPTSTR)(FoundEntry + 1);
    FoundEntry->DllName.LengthInChars = DllNameLength;
    memcpy(FoundEntry->DllName.StartOfString, DllName, DllNameLength * sizeof(TCHAR));
    FoundEntry->ReferenceCount = 1;

    FoundEntry->ModuleHandle = LoadLibrary(DllName);
    if (FoundEntry->ModuleHandle == NULL) {
        YoriLibFree(FoundEntry);
        return NULL;
    }

    if (YoriShLoadedModules.Next == NULL) {
        YoriLibInitializeListHead(&YoriShLoadedModules);
    }
    YoriLibAppendList(&YoriShLoadedModules, &FoundEntry->ListEntry);
    return FoundEntry;
}

/**
 Dereference a loaded DLL module and free it if the reference count reaches
 zero.

 @param LoadedModule The module to dereference.
 */
VOID
YoriShReleaseDll(
    __in PYORI_LOADED_MODULE LoadedModule
    )
{
    if (LoadedModule->ReferenceCount > 1) {
        LoadedModule->ReferenceCount--;
        return;
    }

    YoriLibRemoveListItem(&LoadedModule->ListEntry);
    if (LoadedModule->ModuleHandle != NULL) {
        FreeLibrary(LoadedModule->ModuleHandle);
    }
    YoriLibFree(LoadedModule);
}

/**
 Add a reference to a previously loaded DLL module.

 @param LoadedModule The module to reference.
 */
VOID
YoriShReferenceDll(
    __in PYORI_LOADED_MODULE LoadedModule
    )
{
    ASSERT(LoadedModule->ReferenceCount > 0);
    LoadedModule->ReferenceCount++;
}

/**
 Pointer to the active module, being the DLL that was most recently invoked
 by the shell.
 */
PYORI_LOADED_MODULE YoriShActiveModule = NULL;

/**
 List of builtin callbacks currently registered with Yori.
 */
YORI_LIST_ENTRY YoriShBuiltinCallbacks;

/**
 Invoke a different program to complete executing the command.  This may be
 CMD for its scripts, Powershell for its scripts, YS for its scripts, etc.

 @param ExecContext Pointer to the exec context for a command that is known
        to be implemented by an external program.

 @param ExtraArgCount The number of extra arguments that should be inserted
        into the beginning of the ExecContext, followed by a set of NULL
        terminated strings to insert into the ExecContext.

 @return ExitCode from CMD, or nonzero on failure.
 */
DWORD
YoriShBuckPass (
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext,
    __in DWORD ExtraArgCount,
    ...
    )
{
    YORI_CMD_CONTEXT OldCmdContext;
    DWORD ExitCode = 1;
    DWORD count;
    BOOL ExecAsBuiltin = FALSE;
    va_list marker;

    memcpy(&OldCmdContext, &ExecContext->CmdToExec, sizeof(YORI_CMD_CONTEXT));

    ExecContext->CmdToExec.argc += ExtraArgCount;
    ExecContext->CmdToExec.MemoryToFree = YoriLibReferencedMalloc(ExecContext->CmdToExec.argc * (sizeof(YORI_STRING) + sizeof(YORI_ARG_CONTEXT)));
    if (ExecContext->CmdToExec.MemoryToFree == NULL) {
        memcpy(&ExecContext->CmdToExec, &OldCmdContext, sizeof(YORI_CMD_CONTEXT));
        return ExitCode;
    }

    ExecContext->CmdToExec.ysargv = ExecContext->CmdToExec.MemoryToFree;

    ExecContext->CmdToExec.ArgContexts = (PYORI_ARG_CONTEXT)YoriLibAddToPointer(ExecContext->CmdToExec.ysargv, sizeof(YORI_STRING) * ExecContext->CmdToExec.argc);
    ZeroMemory(ExecContext->CmdToExec.ArgContexts, sizeof(YORI_ARG_CONTEXT) * ExecContext->CmdToExec.argc);

    va_start(marker, ExtraArgCount);
    for (count = 0; count < ExtraArgCount; count++) {
        LPTSTR NewArg = (LPTSTR)va_arg(marker, LPTSTR);
        YORI_STRING YsNewArg;
        YORI_STRING FoundInPath;

        YoriLibInitEmptyString(&ExecContext->CmdToExec.ysargv[count]);
        YoriLibConstantString(&YsNewArg, NewArg);

        //
        //  Search for the first extra argument in the path.  If we find it,
        //  execute as a program; if not, execute as a builtin.
        //

        if (count == 0) {
            YoriLibInitEmptyString(&FoundInPath);
            if (YoriLibLocateExecutableInPath(&YsNewArg, NULL, NULL, &FoundInPath) && FoundInPath.LengthInChars > 0) {
                memcpy(&ExecContext->CmdToExec.ysargv[0], &FoundInPath, sizeof(YORI_STRING));
                ASSERT(YoriLibIsStringNullTerminated(&ExecContext->CmdToExec.ysargv[0]));
                YoriLibInitEmptyString(&FoundInPath);
            } else {
                ExecAsBuiltin = TRUE;
                YoriLibConstantString(&ExecContext->CmdToExec.ysargv[count], NewArg);
            }

            YoriLibFreeStringContents(&FoundInPath);
        } else {
            YoriLibConstantString(&ExecContext->CmdToExec.ysargv[count], NewArg);
        }
    }
    va_end(marker);

    for (count = 0; count < OldCmdContext.argc; count++) {
        YoriShCopyArg(&OldCmdContext, count, &ExecContext->CmdToExec, count + ExtraArgCount);
    }

    YoriShCheckIfArgNeedsQuotes(&ExecContext->CmdToExec, 0);

    if (ExecAsBuiltin) {
        ExitCode = YoriShBuiltIn(ExecContext);

    } else {
        ExitCode = YoriShExecuteSingleProgram(ExecContext);
    }
    YoriShFreeCmdContext(&ExecContext->CmdToExec);
    memcpy(&ExecContext->CmdToExec, &OldCmdContext, sizeof(YORI_CMD_CONTEXT));
    
    return ExitCode;
}

/**
 Call a builtin function.  This may be in a DLL or part of the main executable,
 but it is executed synchronously via a call rather than a CreateProcess.

 @param Fn The function associated with the builtin operation to call.

 @param ExecContext The context surrounding this particular function.

 @return ExitCode, typically zero for success, nonzero for failure.
 */
int
YoriShExecuteInProc(
    __in PYORI_CMD_BUILTIN Fn,
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    YORI_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext;
    BOOLEAN WasPipe = FALSE;
    PYORI_CMD_CONTEXT CmdContext = &ExecContext->CmdToExec;
    DWORD Count;
    DWORD ExitCode = 0;

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

    YoriShRemoveEscapesFromCmdContext(CmdContext);
    YoriShInitializeRedirection(ExecContext, TRUE, &PreviousRedirectContext);

    //
    //  Unlike external processes, builtins need to start buffering
    //  before they start to ensure that output during execution has
    //  somewhere to go.
    //
    
    if (ExecContext->StdOutType == StdOutTypeBuffer) {
        if (ExecContext->StdOut.Buffer.ProcessBuffers != NULL) {
            if (YoriShAppendToExistingProcessBuffer(ExecContext)) {
                ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
            } else {
                ExecContext->StdOut.Buffer.ProcessBuffers = NULL;
            }
        } else {
            if (YoriShCreateNewProcessBuffer(ExecContext)) {
                ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
            }
        }
    }

    for (Count = 0; Count < CmdContext->argc; Count++) {
        ASSERT(YoriLibIsStringNullTerminated(&CmdContext->ysargv[Count]));
    }

    ExitCode = Fn(CmdContext->argc, CmdContext->ysargv);
    YoriShRevertRedirection(&PreviousRedirectContext);

    if (WasPipe) {
        YoriShForwardProcessBufferToNextProcess(ExecContext);
    } else {

        //
        //  Once the process has completed, if it's outputting to
        //  buffers, wait for the buffers to contain final data.
        //

        if (ExecContext->StdOutType == StdOutTypeBuffer &&
            ExecContext->StdOut.Buffer.ProcessBuffers != NULL)  {

            YoriShWaitForProcessBufferToFinalize(ExecContext->StdOut.Buffer.ProcessBuffers);
        }

        if (ExecContext->StdErrType == StdErrTypeBuffer &&
            ExecContext->StdErr.Buffer.ProcessBuffers != NULL) {

            YoriShWaitForProcessBufferToFinalize(ExecContext->StdErr.Buffer.ProcessBuffers);
        }
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
BOOL
YoriShExecuteNamedModuleInProc(
    __in LPTSTR ModuleFileName,
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext,
    __out PDWORD ExitCode
    )
{
    PYORI_CMD_BUILTIN Main;
    PYORI_LOADED_MODULE Module;

    Module = YoriShLoadDll(ModuleFileName);
    if (Module == NULL) {
        return FALSE;
    }

    Main = (PYORI_CMD_BUILTIN)GetProcAddress(Module->ModuleHandle, "YoriMain");
    if (Main == NULL) {
        YoriShReleaseDll(Module);
        return FALSE;
    }
    
    if (Main) {
        PYORI_LOADED_MODULE PreviousModule;

        PreviousModule = YoriShActiveModule;
        YoriShActiveModule = Module;
        *ExitCode = YoriShExecuteInProc(Main, ExecContext);
        YoriShActiveModule = PreviousModule;
    }
    
    YoriShReleaseDll(Module);
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
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    DWORD ExitCode = 1;
    PYORI_CMD_CONTEXT CmdContext = &ExecContext->CmdToExec;
    HANDLE hThisExecutable = GetModuleHandle(NULL);
    PYORI_CMD_BUILTIN BuiltInCmd = NULL;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_BUILTIN_CALLBACK CallbackEntry = NULL;

    if (YoriShBuiltinCallbacks.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShBuiltinCallbacks, NULL);
        while (ListEntry != NULL) {
            CallbackEntry = CONTAINING_RECORD(ListEntry, YORI_BUILTIN_CALLBACK, ListEntry);
            if (YoriLibCompareStringInsensitive(&CallbackEntry->BuiltinName, &CmdContext->ysargv[0]) == 0) {
                BuiltInCmd = CallbackEntry->BuiltInFn;
                break;
            }
            ListEntry = YoriLibGetNextListEntry(&YoriShBuiltinCallbacks, ListEntry);
            CallbackEntry = NULL;
        }
    }

    if (BuiltInCmd == NULL) {
        LPSTR ExportName;
        DWORD LengthNeeded;
        ASSERT(CallbackEntry == NULL);
        LengthNeeded = CmdContext->ysargv[0].LengthInChars + sizeof("YoriCmd_");
        ExportName = YoriLibMalloc(LengthNeeded);
        if (ExportName == NULL) {
            return ExitCode;
        }
        YoriLibSPrintfA(ExportName, "YoriCmd_%ls", CmdContext->ysargv[0].StartOfString);
        _strupr(ExportName + sizeof("YoriCmd_") - 1);
        BuiltInCmd = (PYORI_CMD_BUILTIN)GetProcAddress(hThisExecutable, ExportName);
        YoriLibFree(ExportName);
    }

    if (BuiltInCmd) {
        PYORI_LOADED_MODULE PreviousModule;
        PYORI_LOADED_MODULE HostingModule = NULL;

        //
        //  If the function is in a module, reference the DLL to keep it alive
        //  until it returns.
        //

        if (CallbackEntry != NULL && CallbackEntry->ReferencedModule != NULL) {
            HostingModule = CallbackEntry->ReferencedModule;
            YoriShReferenceDll(HostingModule);
        }

        //
        //  Indicate which module is currently executing, and execute from it.
        //

        PreviousModule = YoriShActiveModule;
        YoriShActiveModule = HostingModule;
        ExitCode = YoriShExecuteInProc(BuiltInCmd, ExecContext);
        ASSERT(YoriShActiveModule == HostingModule);
        YoriShActiveModule = PreviousModule;

        if (HostingModule != NULL) {
            YoriShReleaseDll(HostingModule);
        }
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Unrecognized command: %y\n"), &CmdContext->ysargv[0]);
        if (ExecContext->StdOutType == StdOutTypePipe &&
            ExecContext->NextProgram != NULL &&
            ExecContext->NextProgram->StdInType == StdInTypePipe) {

            ExecContext->NextProgramType = NextProgramExecNever;
        }
    }

    return ExitCode;
}

/**
 Associate a new builtin command with a function pointer to be invoked when
 the command is specified.

 @param BuiltinCmd The command to register.

 @param CallbackFn The function to invoke in response to the command.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShBuiltinRegister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    )
{
    PYORI_BUILTIN_CALLBACK NewCallback;
    if (YoriShBuiltinCallbacks.Next == NULL) {
        YoriLibInitializeListHead(&YoriShBuiltinCallbacks);
    }

    NewCallback = YoriLibReferencedMalloc(sizeof(YORI_BUILTIN_CALLBACK) + (BuiltinCmd->LengthInChars + 1) * sizeof(TCHAR));
    if (NewCallback == NULL) {
        return FALSE;
    }

    YoriLibInitEmptyString(&NewCallback->BuiltinName);
    NewCallback->BuiltinName.StartOfString = (LPTSTR)(NewCallback + 1);
    NewCallback->BuiltinName.LengthInChars = BuiltinCmd->LengthInChars;
    memcpy(NewCallback->BuiltinName.StartOfString, BuiltinCmd->StartOfString, BuiltinCmd->LengthInChars * sizeof(TCHAR));
    NewCallback->BuiltinName.StartOfString[BuiltinCmd->LengthInChars] = '\0';

    NewCallback->BuiltInFn = CallbackFn;
    if (YoriShActiveModule != NULL) {
        YoriShActiveModule->ReferenceCount++;
    }
    NewCallback->ReferencedModule = YoriShActiveModule;

    //
    //  Insert at the front of the list so the most recently added entry
    //  will be found first, and the most recently added entry will be
    //  the first to be removed.
    //

    if (YoriShBuiltinCallbacks.Next == NULL) {
        YoriLibInitializeListHead(&YoriShBuiltinCallbacks);
    }
    YoriLibInsertList(&YoriShBuiltinCallbacks, &NewCallback->ListEntry);
    return TRUE;
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
YoriShBuiltinUnregister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_BUILTIN_CALLBACK Callback;

    UNREFERENCED_PARAMETER(CallbackFn);

    if (YoriShBuiltinCallbacks.Next == NULL) {
        return FALSE;
    }

    ListEntry = YoriLibGetNextListEntry(&YoriShBuiltinCallbacks, NULL);
    while (ListEntry != NULL) {
        Callback = CONTAINING_RECORD(ListEntry, YORI_BUILTIN_CALLBACK, ListEntry);
        if (YoriLibCompareStringInsensitive(BuiltinCmd, &Callback->BuiltinName) == 0) {
            ASSERT(CallbackFn == Callback->BuiltInFn);
            YoriLibRemoveListItem(&Callback->ListEntry);
            if (Callback->ReferencedModule != NULL) {
                YoriShReleaseDll(Callback->ReferencedModule);
                Callback->ReferencedModule = NULL;
            }
            YoriLibDereference(Callback);
            return TRUE;
        }

        ListEntry = YoriLibGetNextListEntry(&YoriShBuiltinCallbacks, ListEntry);
    }

    return FALSE;
}


/**
 Execute a command that is built in to the shell.  This can be used by in
 process extension modules.

 @param Expression The expression to execute.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShExecuteBuiltinString(
    __in PYORI_STRING Expression
    )
{
    YORI_EXEC_PLAN ExecPlan;
    YORI_CMD_CONTEXT CmdContext;
    PYORI_SINGLE_EXEC_CONTEXT ExecContext;

    //
    //  Parse the expression we're trying to execute.
    //

    if (!YoriShParseCmdlineToCmdContext(Expression, 0, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        return FALSE;
    }

    if (CmdContext.argc == 0) {
        YoriShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (!YoriShParseCmdContextToExecPlan(&CmdContext, &ExecPlan, NULL, NULL, NULL)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        YoriShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    ExecContext = ExecPlan.FirstCmd;

    if (ExecContext->NextProgram != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Attempt to invoke multi component expression as a builtin\n"));
        YoriShFreeExecPlan(&ExecPlan);
        YoriShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    g_ErrorLevel = YoriShBuiltIn(ExecContext);

    YoriShFreeExecPlan(&ExecPlan);
    YoriShFreeCmdContext(&CmdContext);

    return TRUE;
}

// vim:sw=4:ts=4:et:
