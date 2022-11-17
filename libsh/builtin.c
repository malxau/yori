/**
 * @file /libsh/builtin.c
 *
 * Yori shell built in function registration
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

#include <yoripch.h>
#include <yorilib.h>
#include <yorish.h>

/**
 Information related to builtins registered against this process.
 */
typedef struct _YORI_LIBSH_BUILTIN_GLOBAL {

    /**
     A list of currently loaded modules.
     */
    YORI_LIST_ENTRY LoadedModules;

    /**
     Hash table of builtin callbacks currently registered with Yori.
     */
    PYORI_HASH_TABLE Hash;

    /**
     A list of unload functions to invoke.  These are only for code statically
     linked into the shell executable, not loadable modules.  Once added, a
     callback is never removed - the code is guaranteed to still exist, so the
     worst case is calling something that has no work to do.
     */
    YORI_LIST_ENTRY UnloadCallbacks;

    /**
     Pointer to the active module, being the DLL that was most recently invoked
     by the shell.
     */
    PYORI_LIBSH_LOADED_MODULE ActiveModule;

    /**
     List of builtin callbacks currently registered with Yori.
     */
    YORI_LIST_ENTRY BuiltinCallbacks;

} YORI_LIBSH_BUILTIN_GLOBAL, *PYORI_LIBSH_BUILTIN_GLOBAL;

/**
 Information related to builtins registered against this process.
 */
YORI_LIBSH_BUILTIN_GLOBAL YoriLibShBuiltinGlobal;

/**
 A single callback function to invoke on shell exit that is part of the
 shell executable.
 */
typedef struct _YORI_LIBSH_BUILTIN_UNLOAD_CALLBACK {

    /**
     A list of unload notifications to make within the shell executable.
     This is paired with YoriLibShBuiltinUnloadCallbacks.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     Pointer to a function to call on shell exit.
     */
    PYORI_BUILTIN_UNLOAD_NOTIFY UnloadNotify;
} YORI_LIBSH_BUILTIN_UNLOAD_CALLBACK, *PYORI_LIBSH_BUILTIN_UNLOAD_CALLBACK;

/**
 Load a DLL file into a loaded module object that can be referenced.

 @param DllName Pointer to a NULL terminated string of a DLL to load.

 @return Pointer a referenced loaded module.  This module is linked into
         the loaded module list.  NULL on failure.
 */
PYORI_LIBSH_LOADED_MODULE
YoriLibShLoadDll(
    __in LPTSTR DllName
    )
{
    PYORI_LIBSH_LOADED_MODULE FoundEntry = NULL;
    PYORI_LIST_ENTRY ListEntry;
    DWORD DllNameLength;
    DWORD OldErrorMode;

    if (YoriLibShBuiltinGlobal.LoadedModules.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriLibShBuiltinGlobal.LoadedModules, NULL);
        while (ListEntry != NULL) {
            FoundEntry = CONTAINING_RECORD(ListEntry, YORI_LIBSH_LOADED_MODULE, ListEntry);
            if (YoriLibCompareStringWithLiteralInsensitive(&FoundEntry->DllName, DllName) == 0) {
                FoundEntry->ReferenceCount++;
                return FoundEntry;
            }

            ListEntry = YoriLibGetNextListEntry(&YoriLibShBuiltinGlobal.LoadedModules, ListEntry);
        }
    }

    DllNameLength = _tcslen(DllName);
    FoundEntry = YoriLibMalloc(sizeof(YORI_LIBSH_LOADED_MODULE) + (DllNameLength + 1) * sizeof(TCHAR));
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

    FoundEntry->ModuleHandle = LoadLibraryEx(DllName, NULL, 0);
    if (FoundEntry->ModuleHandle == NULL) {
        SetErrorMode(OldErrorMode);
        YoriLibFree(FoundEntry);
        return NULL;
    }
    SetErrorMode(OldErrorMode);

    if (YoriLibShBuiltinGlobal.LoadedModules.Next == NULL) {
        YoriLibInitializeListHead(&YoriLibShBuiltinGlobal.LoadedModules);
    }
    YoriLibAppendList(&YoriLibShBuiltinGlobal.LoadedModules, &FoundEntry->ListEntry);
    return FoundEntry;
}

/**
 Dereference a loaded DLL module and free it if the reference count reaches
 zero.

 @param LoadedModule The module to dereference.
 */
VOID
YoriLibShReleaseDll(
    __in PYORI_LIBSH_LOADED_MODULE LoadedModule
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
YoriLibShReferenceDll(
    __in PYORI_LIBSH_LOADED_MODULE LoadedModule
    )
{
    ASSERT(LoadedModule->ReferenceCount > 0);
    LoadedModule->ReferenceCount++;
}

/**
 Add a new function to invoke on shell exit or module unload.

 @param UnloadNotify Pointer to the function to invoke.

 @return TRUE if the callback was successfully added, FALSE if it was not.
 */
__success(return)
BOOL
YoriLibShSetUnloadRoutine(
    __in PYORI_BUILTIN_UNLOAD_NOTIFY UnloadNotify
    )
{
    if (YoriLibShBuiltinGlobal.ActiveModule != NULL) {

        if (YoriLibShBuiltinGlobal.ActiveModule->UnloadNotify == NULL) {
            YoriLibShBuiltinGlobal.ActiveModule->UnloadNotify = UnloadNotify;
        }

        if (YoriLibShBuiltinGlobal.ActiveModule->UnloadNotify == UnloadNotify) {
            return TRUE;
        }
    } else {

        PYORI_LIST_ENTRY ListEntry = NULL;
        PYORI_LIBSH_BUILTIN_UNLOAD_CALLBACK Callback;

        if (YoriLibShBuiltinGlobal.UnloadCallbacks.Next == NULL) {
            YoriLibInitializeListHead(&YoriLibShBuiltinGlobal.UnloadCallbacks);
        }

        ListEntry = YoriLibGetNextListEntry(&YoriLibShBuiltinGlobal.UnloadCallbacks, NULL);
        while (ListEntry != NULL) {
            Callback = CONTAINING_RECORD(ListEntry, YORI_LIBSH_BUILTIN_UNLOAD_CALLBACK, ListEntry);
            if (Callback->UnloadNotify == UnloadNotify) {
                return TRUE;
            }
            ListEntry = YoriLibGetNextListEntry(&YoriLibShBuiltinGlobal.UnloadCallbacks, ListEntry);
        }

        Callback = YoriLibMalloc(sizeof(YORI_LIBSH_BUILTIN_UNLOAD_CALLBACK));
        if (Callback == NULL) {
            return FALSE;
        }

        Callback->UnloadNotify = UnloadNotify;
        YoriLibAppendList(&YoriLibShBuiltinGlobal.UnloadCallbacks, &Callback->ListEntry);
        return TRUE;
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
YoriLibShBuiltinRegister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    )
{
    PYORI_LIBSH_BUILTIN_CALLBACK NewCallback;
    if (YoriLibShBuiltinGlobal.BuiltinCallbacks.Next == NULL) {
        YoriLibInitializeListHead(&YoriLibShBuiltinGlobal.BuiltinCallbacks);
    }
    if (YoriLibShBuiltinGlobal.Hash == NULL) {
        YoriLibShBuiltinGlobal.Hash = YoriLibAllocateHashTable(50);
        if (YoriLibShBuiltinGlobal.Hash == NULL) {
            return FALSE;
        }
    }

    NewCallback = YoriLibReferencedMalloc(sizeof(YORI_LIBSH_BUILTIN_CALLBACK) + (BuiltinCmd->LengthInChars + 1) * sizeof(TCHAR));
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
    if (YoriLibShBuiltinGlobal.ActiveModule != NULL) {
        YoriLibShBuiltinGlobal.ActiveModule->ReferenceCount++;
    }
    NewCallback->ReferencedModule = YoriLibShBuiltinGlobal.ActiveModule;

    //
    //  Insert at the front of the list so the most recently added entry
    //  will be found first, and the most recently added entry will be
    //  the first to be removed.
    //

    YoriLibInsertList(&YoriLibShBuiltinGlobal.BuiltinCallbacks, &NewCallback->ListEntry);
    YoriLibHashInsertByKey(YoriLibShBuiltinGlobal.Hash, &NewCallback->BuiltinName, NewCallback, &NewCallback->HashEntry);
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
YoriLibShBuiltinUnregister(
    __in PYORI_STRING BuiltinCmd,
    __in PYORI_CMD_BUILTIN CallbackFn
    )
{
    PYORI_LIBSH_BUILTIN_CALLBACK Callback;
    PYORI_HASH_ENTRY HashEntry;

    UNREFERENCED_PARAMETER(CallbackFn);

    if (YoriLibShBuiltinGlobal.Hash == NULL) {
        return FALSE;
    }

    if (YoriLibShBuiltinGlobal.Hash != NULL) {
        HashEntry = YoriLibHashRemoveByKey(YoriLibShBuiltinGlobal.Hash, BuiltinCmd);
        if (HashEntry != NULL) {
            Callback = (PYORI_LIBSH_BUILTIN_CALLBACK)HashEntry->Context;
            ASSERT(CallbackFn == Callback->BuiltInFn);
            YoriLibRemoveListItem(&Callback->ListEntry);
            if (Callback->ReferencedModule != NULL) {
                YoriLibShReleaseDll(Callback->ReferencedModule);
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
YoriLibShBuiltinUnregisterAll(VOID)
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIBSH_BUILTIN_CALLBACK Callback;
    PYORI_LIBSH_BUILTIN_UNLOAD_CALLBACK UnloadCallback;

    if (YoriLibShBuiltinGlobal.BuiltinCallbacks.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriLibShBuiltinGlobal.BuiltinCallbacks, NULL);
        while (ListEntry != NULL) {
            Callback = CONTAINING_RECORD(ListEntry, YORI_LIBSH_BUILTIN_CALLBACK, ListEntry);
            YoriLibRemoveListItem(&Callback->ListEntry);
            YoriLibHashRemoveByEntry(&Callback->HashEntry);
            if (Callback->ReferencedModule != NULL) {
                YoriLibShReleaseDll(Callback->ReferencedModule);
                Callback->ReferencedModule = NULL;
            }
            YoriLibFreeStringContents(&Callback->BuiltinName);
            YoriLibDereference(Callback);
            ListEntry = YoriLibGetNextListEntry(&YoriLibShBuiltinGlobal.BuiltinCallbacks, NULL);
        }
    }

    if (YoriLibShBuiltinGlobal.Hash != NULL) {
        YoriLibFreeEmptyHashTable(YoriLibShBuiltinGlobal.Hash);
    }

    if (YoriLibShBuiltinGlobal.UnloadCallbacks.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriLibShBuiltinGlobal.UnloadCallbacks, NULL);
        while (ListEntry != NULL) {
            UnloadCallback = CONTAINING_RECORD(ListEntry, YORI_LIBSH_BUILTIN_UNLOAD_CALLBACK, ListEntry);
            ListEntry = YoriLibGetNextListEntry(&YoriLibShBuiltinGlobal.UnloadCallbacks, ListEntry);
            YoriLibRemoveListItem(&UnloadCallback->ListEntry);
            UnloadCallback->UnloadNotify();
            YoriLibFree(UnloadCallback);
        }
    }

    return;
}

/**
 Return the currently executing DLL module.  Code executing now is not part
 of the DLL, but it might be above this code in the stack.

 @return Pointer to the currently executing DLL module, or NULL if none is
         active.
 */
PYORI_LIBSH_LOADED_MODULE
YoriLibShGetActiveModule(VOID)
{
    return YoriLibShBuiltinGlobal.ActiveModule;
}

/**
 Set the currently executing DLL module.  Code executing now is not part of
 the DLL, but the DLL is about to be entered.  Alternatively, one DLL may
 have unwound and the previous DLL is being restored to be active.

 @param NewModule Pointer to the DLL module that is about to execute, or NULL
        if none is active.
 */
VOID
YoriLibShSetActiveModule(
    __in_opt PYORI_LIBSH_LOADED_MODULE NewModule
    )
{
    YoriLibShBuiltinGlobal.ActiveModule = NewModule;
}

/**
 Lookup a registered builtin function by case insensitive name.

 @param Name Pointer to the name of the function to look up.

 @return Pointer to the context describing this builtin function, or NULL if
         it is not found.
 */
PYORI_LIBSH_BUILTIN_CALLBACK
YoriLibShLookupBuiltinByName(
    __in PYORI_STRING Name
    )
{
    PYORI_HASH_ENTRY HashEntry;

    if (YoriLibShBuiltinGlobal.Hash == NULL) {
        return NULL;
    }

    HashEntry = YoriLibHashLookupByKey(YoriLibShBuiltinGlobal.Hash, Name);
    if (HashEntry == NULL) {
        return NULL;
    }

    return (PYORI_LIBSH_BUILTIN_CALLBACK)HashEntry->Context;
}

/**
 Return the previous (newer in time) builtin relative to a specified builtin.

 @param Existing Optionally points to a currently known builtin context.  If
        not specified, the oldest builtin is returned.

 @return Pointer to the previous callback context.
 */
PYORI_LIBSH_BUILTIN_CALLBACK
YoriLibShGetPreviousBuiltinCallback(
    __in_opt PYORI_LIBSH_BUILTIN_CALLBACK Existing
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIBSH_BUILTIN_CALLBACK Previous;

    if (YoriLibShBuiltinGlobal.BuiltinCallbacks.Next == NULL) {
        return NULL;
    }

    if (Existing == NULL) {
        ListEntry = YoriLibGetPreviousListEntry(&YoriLibShBuiltinGlobal.BuiltinCallbacks, NULL);
    } else {
        ListEntry = YoriLibGetPreviousListEntry(&YoriLibShBuiltinGlobal.BuiltinCallbacks, &Existing->ListEntry);
    }

    if (ListEntry == NULL) {
        return NULL;
    }

    Previous = CONTAINING_RECORD(ListEntry, YORI_LIBSH_BUILTIN_CALLBACK, ListEntry);
    return Previous;
}


// vim:sw=4:ts=4:et:
