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
 A list of currently loaded modules.
 */
YORI_LIST_ENTRY YoriShLoadedModules;

/**
 Hash table of builtin callbacks currently registered with Yori.
 */
PYORI_HASH_TABLE YoriShBuiltinHash;

/**
 A list of unload functions to invoke.  These are only for code statically
 linked into the shell executable, not loadable modules.  Once added, a
 callback is never removed - the code is guaranteed to still exist, so the
 worst case is calling something that has no work to do.
 */
YORI_LIST_ENTRY YoriShBuiltinUnloadCallbacks;

/**
 A single callback function to invoke on shell exit that is part of the
 shell executable.
 */
typedef struct _YORI_SH_BUILTIN_UNLOAD_CALLBACK {

    /**
     A list of unload notifications to make within the shell executable.
     This is paired with YoriShBuiltinUnloadCallbacks.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     Pointer to a function to call on shell exit.
     */
    PYORI_BUILTIN_UNLOAD_NOTIFY UnloadNotify;
} YORI_SH_BUILTIN_UNLOAD_CALLBACK, *PYORI_SH_BUILTIN_UNLOAD_CALLBACK;

/**
 Load a DLL file into a loaded module object that can be referenced.

 @param DllName Pointer to a NULL terminated string of a DLL to load.

 @return Pointer a referenced loaded module.  This module is linked into
         the loaded module list.  NULL on failure.
 */
PYORI_SH_LOADED_MODULE
YoriShLoadDll(
    __in LPTSTR DllName
    )
{
    PYORI_SH_LOADED_MODULE FoundEntry = NULL;
    PYORI_LIST_ENTRY ListEntry;
    DWORD DllNameLength;
    DWORD OldErrorMode;

    if (YoriShLoadedModules.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShLoadedModules, NULL);
        while (ListEntry != NULL) {
            FoundEntry = CONTAINING_RECORD(ListEntry, YORI_SH_LOADED_MODULE, ListEntry);
            if (YoriLibCompareStringWithLiteralInsensitive(&FoundEntry->DllName, DllName) == 0) {
                FoundEntry->ReferenceCount++;
                return FoundEntry;
            }

            ListEntry = YoriLibGetNextListEntry(&YoriShLoadedModules, ListEntry);
        }
    }

    DllNameLength = _tcslen(DllName);
    FoundEntry = YoriLibMalloc(sizeof(YORI_SH_LOADED_MODULE) + (DllNameLength + 1) * sizeof(TCHAR));
    if (FoundEntry == NULL) {
        return NULL;
    }

    YoriLibInitEmptyString(&FoundEntry->DllName);
    FoundEntry->DllName.StartOfString = (LPTSTR)(FoundEntry + 1);
    FoundEntry->DllName.LengthInChars = DllNameLength;
    memcpy(FoundEntry->DllName.StartOfString, DllName, DllNameLength * sizeof(TCHAR));
    FoundEntry->ReferenceCount = 1;
    FoundEntry->UnloadNotify = NULL;

    //
    //  Disable the dialog if the file is not a valid DLL.  This might really
    //  be a DOS executable, not a DLL.
    //

    OldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FoundEntry->ModuleHandle = LoadLibrary(DllName);
    if (FoundEntry->ModuleHandle == NULL) {
        SetErrorMode(OldErrorMode);
        YoriLibFree(FoundEntry);
        return NULL;
    }
    SetErrorMode(OldErrorMode);

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
    __in PYORI_SH_LOADED_MODULE LoadedModule
    )
{
    if (LoadedModule->ReferenceCount > 1) {
        LoadedModule->ReferenceCount--;
        return;
    }

    YoriLibRemoveListItem(&LoadedModule->ListEntry);
    if (LoadedModule->UnloadNotify != NULL) {
        LoadedModule->UnloadNotify();
    }
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
    __in PYORI_SH_LOADED_MODULE LoadedModule
    )
{
    ASSERT(LoadedModule->ReferenceCount > 0);
    LoadedModule->ReferenceCount++;
}

/**
 Pointer to the active module, being the DLL that was most recently invoked
 by the shell.
 */
PYORI_SH_LOADED_MODULE YoriShActiveModule = NULL;

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
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext,
    __in DWORD ExtraArgCount,
    ...
    )
{
    YORI_SH_CMD_CONTEXT OldCmdContext;
    DWORD ExitCode = 1;
    DWORD count;
    BOOL ExecAsBuiltin = FALSE;
    va_list marker;

    memcpy(&OldCmdContext, &ExecContext->CmdToExec, sizeof(YORI_SH_CMD_CONTEXT));

    ExecContext->CmdToExec.ArgC += ExtraArgCount;
    ExecContext->CmdToExec.MemoryToFree = YoriLibReferencedMalloc(ExecContext->CmdToExec.ArgC * (sizeof(YORI_STRING) + sizeof(YORI_SH_ARG_CONTEXT)));
    if (ExecContext->CmdToExec.MemoryToFree == NULL) {
        memcpy(&ExecContext->CmdToExec, &OldCmdContext, sizeof(YORI_SH_CMD_CONTEXT));
        return ExitCode;
    }

    ExecContext->CmdToExec.ArgV = ExecContext->CmdToExec.MemoryToFree;

    ExecContext->CmdToExec.ArgContexts = (PYORI_SH_ARG_CONTEXT)YoriLibAddToPointer(ExecContext->CmdToExec.ArgV, sizeof(YORI_STRING) * ExecContext->CmdToExec.ArgC);
    ZeroMemory(ExecContext->CmdToExec.ArgContexts, sizeof(YORI_SH_ARG_CONTEXT) * ExecContext->CmdToExec.ArgC);

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
        YoriShCopyArg(&OldCmdContext, count, &ExecContext->CmdToExec, count + ExtraArgCount);
    }

    YoriShCheckIfArgNeedsQuotes(&ExecContext->CmdToExec, 0);

    if (ExecAsBuiltin) {
        ExitCode = YoriShBuiltIn(ExecContext);
    } else {
        ExecContext->IncludeEscapesAsLiteral = TRUE;
        ExitCode = YoriShExecuteSingleProgram(ExecContext);
    }

    YoriShFreeCmdContext(&ExecContext->CmdToExec);
    memcpy(&ExecContext->CmdToExec, &OldCmdContext, sizeof(YORI_SH_CMD_CONTEXT));

    return ExitCode;
}

/**
 Invoke CMD to execute a script.  This is different to regular child
 processes because CMD wants all arguments to be enclosed in quotes, in
 addition to quotes around individual arguments, as in:
 C:\\Windows\\System32\\Cmd.exe /c ""C:\\Path To\\Foo.cmd" "Arg To Foo""

 @param ExecContext Pointer to the exec context for a command that is known
        to be implemented by an external program.

 @param ExtraArgCount The number of extra arguments that should be inserted
        into the beginning of the ExecContext, followed by a set of NULL
        terminated strings to insert into the ExecContext.

 @return ExitCode from CMD, or nonzero on failure.
 */
DWORD
YoriShBuckPassToCmd (
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext,
    __in DWORD ExtraArgCount,
    ...
    )
{
    YORI_SH_CMD_CONTEXT OldCmdContext;
    LPTSTR CmdLine;
    DWORD ExitCode = EXIT_SUCCESS;
    DWORD count;
    va_list marker;

    memcpy(&OldCmdContext, &ExecContext->CmdToExec, sizeof(YORI_SH_CMD_CONTEXT));

    ExecContext->CmdToExec.ArgC = ExtraArgCount + 1;
    ExecContext->CmdToExec.MemoryToFree = YoriLibReferencedMalloc(ExecContext->CmdToExec.ArgC * (sizeof(YORI_STRING) + sizeof(YORI_SH_ARG_CONTEXT)));
    if (ExecContext->CmdToExec.MemoryToFree == NULL) {
        memcpy(&ExecContext->CmdToExec, &OldCmdContext, sizeof(YORI_SH_CMD_CONTEXT));
        ExitCode = EXIT_FAILURE;
        return ExitCode;
    }

    ExecContext->CmdToExec.ArgV = ExecContext->CmdToExec.MemoryToFree;

    ExecContext->CmdToExec.ArgContexts = (PYORI_SH_ARG_CONTEXT)YoriLibAddToPointer(ExecContext->CmdToExec.ArgV, sizeof(YORI_STRING) * ExecContext->CmdToExec.ArgC);
    ZeroMemory(ExecContext->CmdToExec.ArgContexts, sizeof(YORI_SH_ARG_CONTEXT) * ExecContext->CmdToExec.ArgC);

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
                ExitCode = EXIT_FAILURE;
                YoriLibConstantString(&ExecContext->CmdToExec.ArgV[count], NewArg);
            }

            YoriLibFreeStringContents(&FoundInPath);
        } else {
            YoriLibConstantString(&ExecContext->CmdToExec.ArgV[count], NewArg);
        }
    }
    va_end(marker);

    CmdLine = YoriShBuildCmdlineFromCmdContext(&OldCmdContext, FALSE, NULL, NULL);
    if (CmdLine == NULL) {
        ExitCode = EXIT_FAILURE;
    } else {
        YoriLibConstantString(&ExecContext->CmdToExec.ArgV[ExtraArgCount], CmdLine);
        ExecContext->CmdToExec.ArgV[ExtraArgCount].MemoryToFree = CmdLine;
        ExecContext->CmdToExec.ArgContexts[ExtraArgCount].Quoted = TRUE;
    }

    YoriShCheckIfArgNeedsQuotes(&ExecContext->CmdToExec, 0);

    if (ExitCode == EXIT_SUCCESS) {
        ExecContext->IncludeEscapesAsLiteral = TRUE;
        ExitCode = YoriShExecuteSingleProgram(ExecContext);
    }

    YoriShFreeCmdContext(&ExecContext->CmdToExec);
    memcpy(&ExecContext->CmdToExec, &OldCmdContext, sizeof(YORI_SH_CMD_CONTEXT));

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
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    YORI_SH_PREVIOUS_REDIRECT_CONTEXT PreviousRedirectContext;
    BOOLEAN WasPipe = FALSE;
    PYORI_SH_CMD_CONTEXT OriginalCmdContext = &ExecContext->CmdToExec;
    PYORI_SH_CMD_CONTEXT SavedEscapedCmdContext;
    YORI_SH_CMD_CONTEXT NoEscapesCmdContext;
    LPTSTR CmdLine;
    PYORI_STRING ArgV;
    DWORD ArgC;
    DWORD Count;
    DWORD ExitCode = 0;

    if (!YoriShRemoveEscapesFromCmdContext(OriginalCmdContext, &NoEscapesCmdContext)) {
        return ERROR_OUTOFMEMORY;
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

    //
    //  Check if an argument isn't quoted but requires quotes.  This implies
    //  something happened outside the user's immediate control, such as
    //  environment variable expansion.  When this occurs, reprocess the
    //  command back to a string form and recompose into ArgC/ArgV using the
    //  same routines as would occur for an external process.
    //

    ArgC = NoEscapesCmdContext.ArgC;
    ArgV = NoEscapesCmdContext.ArgV;

    for (Count = 0; Count < NoEscapesCmdContext.ArgC; Count++) {
        ASSERT(YoriLibIsStringNullTerminated(&NoEscapesCmdContext.ArgV[Count]));
        if (!NoEscapesCmdContext.ArgContexts[Count].Quoted &&
            YoriLibCheckIfArgNeedsQuotes(&NoEscapesCmdContext.ArgV[Count]) &&
            ArgV == NoEscapesCmdContext.ArgV) {

            CmdLine = YoriShBuildCmdlineFromCmdContext(&NoEscapesCmdContext, TRUE, NULL, NULL);
            if (CmdLine == NULL) {
                YoriShFreeCmdContext(&NoEscapesCmdContext);
                return ERROR_OUTOFMEMORY;
            }

            ArgV = YoriLibCmdlineToArgcArgv(CmdLine, (DWORD)-1, &ArgC);
            YoriLibDereference(CmdLine);

            if (ArgV == NULL) {
                YoriShFreeCmdContext(&NoEscapesCmdContext);
                return ERROR_OUTOFMEMORY;
            }
        }
    }

    ExitCode = YoriShInitializeRedirection(ExecContext, TRUE, &PreviousRedirectContext);
    if (ExitCode != ERROR_SUCCESS) {
        if (ArgV != NoEscapesCmdContext.ArgV) {
            for (Count = 0; Count < ArgC; Count++) {
                YoriLibFreeStringContents(&ArgV[Count]);
            }
            YoriLibDereference(ArgV);
        }
        YoriShFreeCmdContext(&NoEscapesCmdContext);

        return ExitCode;
    }

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

    SavedEscapedCmdContext = YoriShGlobal.EscapedCmdContext;
    YoriShGlobal.EscapedCmdContext = OriginalCmdContext;
    YoriShGlobal.RecursionDepth++;
    ExitCode = Fn(ArgC, ArgV);
    YoriShGlobal.RecursionDepth--;
    YoriShGlobal.EscapedCmdContext = SavedEscapedCmdContext;
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

    if (ArgV != NoEscapesCmdContext.ArgV) {
        for (Count = 0; Count < ArgC; Count++) {
            YoriLibFreeStringContents(&ArgV[Count]);
        }
        YoriLibDereference(ArgV);
    }
    YoriShFreeCmdContext(&NoEscapesCmdContext);

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
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext,
    __out PDWORD ExitCode
    )
{
    PYORI_CMD_BUILTIN Main;
    PYORI_SH_LOADED_MODULE Module;

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
        PYORI_SH_LOADED_MODULE PreviousModule;

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
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    DWORD ExitCode = 1;
    PYORI_SH_CMD_CONTEXT CmdContext = &ExecContext->CmdToExec;
    PYORI_CMD_BUILTIN BuiltInCmd = NULL;
    PYORI_SH_BUILTIN_CALLBACK CallbackEntry = NULL;
    PYORI_HASH_ENTRY HashEntry;

    if (YoriShBuiltinHash != NULL) {
        HashEntry = YoriLibHashLookupByKey(YoriShBuiltinHash, &CmdContext->ArgV[0]);
        if (HashEntry != NULL) {
            CallbackEntry = (PYORI_SH_BUILTIN_CALLBACK)HashEntry->Context;
            BuiltInCmd = CallbackEntry->BuiltInFn;
        }
    }

    if (BuiltInCmd) {
        PYORI_SH_LOADED_MODULE PreviousModule;
        PYORI_SH_LOADED_MODULE HostingModule = NULL;

        ASSERT(CallbackEntry != NULL);

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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Unrecognized command: %y\n"), &CmdContext->ArgV[0]);
        if (ExecContext->StdOutType == StdOutTypePipe &&
            ExecContext->NextProgram != NULL &&
            ExecContext->NextProgram->StdInType == StdInTypePipe) {

            ExecContext->NextProgramType = NextProgramExecNever;
        }
    }

    return ExitCode;
}


/**
 Add a new function to invoke on shell exit or module unload.

 @param UnloadNotify Pointer to the function to invoke.

 @return TRUE if the callback was successfully added, FALSE if it was not.
 */
__success(return)
BOOL
YoriShSetUnloadRoutine(
    __in PYORI_BUILTIN_UNLOAD_NOTIFY UnloadNotify
    )
{
    if (YoriShActiveModule != NULL) {

        if (YoriShActiveModule->UnloadNotify == NULL) {
            YoriShActiveModule->UnloadNotify = UnloadNotify;
        }

        if (YoriShActiveModule->UnloadNotify == UnloadNotify) {
            return TRUE;
        }

        return FALSE;
    } else {

        PYORI_LIST_ENTRY ListEntry = NULL;
        PYORI_SH_BUILTIN_UNLOAD_CALLBACK Callback;

        if (YoriShBuiltinUnloadCallbacks.Next == NULL) {
            YoriLibInitializeListHead(&YoriShBuiltinUnloadCallbacks);
        }

        ListEntry = YoriLibGetNextListEntry(&YoriShBuiltinUnloadCallbacks, NULL);
        while (ListEntry != NULL) {
            Callback = CONTAINING_RECORD(ListEntry, YORI_SH_BUILTIN_UNLOAD_CALLBACK, ListEntry);
            if (Callback->UnloadNotify == UnloadNotify) {
                return TRUE;
            }
            ListEntry = YoriLibGetNextListEntry(&YoriShBuiltinUnloadCallbacks, ListEntry);
        }

        Callback = YoriLibMalloc(sizeof(YORI_SH_BUILTIN_UNLOAD_CALLBACK));
        if (Callback == NULL) {
            return FALSE;
        }

        Callback->UnloadNotify = UnloadNotify;
        YoriLibAppendList(&YoriShBuiltinUnloadCallbacks, &Callback->ListEntry);
    }

    return FALSE;
}

/**
 Associate a new builtin command with a function pointer to be invoked when
 the command is specified.

 @param BuiltinCmd The command to register.

 @param CallbackFn The function to invoke in response to the command.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShBuiltinRegister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    )
{
    PYORI_SH_BUILTIN_CALLBACK NewCallback;
    if (YoriShGlobal.BuiltinCallbacks.Next == NULL) {
        YoriLibInitializeListHead(&YoriShGlobal.BuiltinCallbacks);
    }
    if (YoriShBuiltinHash == NULL) {
        YoriShBuiltinHash = YoriLibAllocateHashTable(50);
        if (YoriShBuiltinHash == NULL) {
            return FALSE;
        }
    }

    NewCallback = YoriLibReferencedMalloc(sizeof(YORI_SH_BUILTIN_CALLBACK) + (BuiltinCmd->LengthInChars + 1) * sizeof(TCHAR));
    if (NewCallback == NULL) {
        return FALSE;
    }

    YoriLibInitEmptyString(&NewCallback->BuiltinName);
    NewCallback->BuiltinName.StartOfString = (LPTSTR)(NewCallback + 1);
    NewCallback->BuiltinName.LengthInChars = BuiltinCmd->LengthInChars;
    memcpy(NewCallback->BuiltinName.StartOfString, BuiltinCmd->StartOfString, BuiltinCmd->LengthInChars * sizeof(TCHAR));
    NewCallback->BuiltinName.StartOfString[BuiltinCmd->LengthInChars] = '\0';
    YoriLibReference(NewCallback);
    NewCallback->BuiltinName.MemoryToFree = NewCallback;

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

    YoriLibInsertList(&YoriShGlobal.BuiltinCallbacks, &NewCallback->ListEntry);
    YoriLibHashInsertByKey(YoriShBuiltinHash, &NewCallback->BuiltinName, NewCallback, &NewCallback->HashEntry);
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
__success(return)
BOOL
YoriShBuiltinUnregister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    )
{
    PYORI_SH_BUILTIN_CALLBACK Callback;
    PYORI_HASH_ENTRY HashEntry;

    UNREFERENCED_PARAMETER(CallbackFn);

    if (YoriShBuiltinHash == NULL) {
        return FALSE;
    }


    if (YoriShBuiltinHash != NULL) {
        HashEntry = YoriLibHashRemoveByKey(YoriShBuiltinHash, BuiltinCmd);
        if (HashEntry != NULL) {
            Callback = (PYORI_SH_BUILTIN_CALLBACK)HashEntry->Context;
            ASSERT(CallbackFn == Callback->BuiltInFn);
            YoriLibRemoveListItem(&Callback->ListEntry);
            if (Callback->ReferencedModule != NULL) {
                YoriShReleaseDll(Callback->ReferencedModule);
                Callback->ReferencedModule = NULL;
            }
            YoriLibFreeStringContents(&Callback->BuiltinName);
            YoriLibDereference(Callback);
        }
    }

    return FALSE;
}

/**
 Dissociate all previously associated builtin commands in preparation for
 process exit.
 */
VOID
YoriShBuiltinUnregisterAll(
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_SH_BUILTIN_CALLBACK Callback;
    PYORI_SH_BUILTIN_UNLOAD_CALLBACK UnloadCallback;

    if (YoriShGlobal.BuiltinCallbacks.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.BuiltinCallbacks, NULL);
        while (ListEntry != NULL) {
            Callback = CONTAINING_RECORD(ListEntry, YORI_SH_BUILTIN_CALLBACK, ListEntry);
            YoriLibRemoveListItem(&Callback->ListEntry);
            YoriLibHashRemoveByEntry(&Callback->HashEntry);
            if (Callback->ReferencedModule != NULL) {
                YoriShReleaseDll(Callback->ReferencedModule);
                Callback->ReferencedModule = NULL;
            }
            YoriLibFreeStringContents(&Callback->BuiltinName);
            YoriLibDereference(Callback);
            ListEntry = YoriLibGetNextListEntry(&YoriShGlobal.BuiltinCallbacks, NULL);
        }
    }

    if (YoriShBuiltinHash != NULL) {
        YoriLibFreeEmptyHashTable(YoriShBuiltinHash);
    }

    if (YoriShBuiltinUnloadCallbacks.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShBuiltinUnloadCallbacks, NULL);
        while (ListEntry != NULL) {
            UnloadCallback = CONTAINING_RECORD(ListEntry, YORI_SH_BUILTIN_UNLOAD_CALLBACK, ListEntry);
            ListEntry = YoriLibGetNextListEntry(&YoriShBuiltinUnloadCallbacks, ListEntry);
            YoriLibRemoveListItem(&UnloadCallback->ListEntry);
            UnloadCallback->UnloadNotify();
            YoriLibFree(UnloadCallback);
        }
    }

    return;
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
    YORI_SH_EXEC_PLAN ExecPlan;
    YORI_SH_CMD_CONTEXT CmdContext;
    PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext;

    //
    //  Parse the expression we're trying to execute.
    //

    if (!YoriShParseCmdlineToCmdContext(Expression, 0, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error\n"));
        return FALSE;
    }

    if (CmdContext.ArgC == 0) {
        YoriShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (!YoriShParseCmdContextToExecPlan(&CmdContext, &ExecPlan, NULL, NULL, NULL, NULL)) {
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

    YoriShGlobal.ErrorLevel = YoriShBuiltIn(ExecContext);

    YoriShFreeExecPlan(&ExecPlan);
    YoriShFreeCmdContext(&CmdContext);

    return TRUE;
}

// vim:sw=4:ts=4:et:
