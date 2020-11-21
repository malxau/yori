/**
 * @file sh/window.c
 *
 * Yori shell window status
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
 A structure definition for a TaskbarList object.  This is presumably followed
 by opaque data, but we don't care about that.
 */
typedef struct _YORI_SH_TASKBARLIST {

    /**
     The table of function pointers for the ITaskbarList3 interface.
     */
    struct _YORI_SH_ITASKBARLIST_VTBL * Vtbl;
} YORI_SH_TASKBARLIST, *PYORI_SH_TASKBARLIST;

/**
 Function declaration for the ITaskbarList3::SetProgressValue API.
 */
typedef HRESULT STDMETHODCALLTYPE ITaskbarList_SetProgressValue(PYORI_SH_TASKBARLIST, HWND, DWORDLONG, DWORDLONG);

/**
 Function declaration for the ITaskbarList3::SetProgressState API.
 */
typedef HRESULT STDMETHODCALLTYPE ITaskbarList_SetProgressState(PYORI_SH_TASKBARLIST, HWND, DWORD);

/**
 The set of functions used by Yori on the ITaskbarList3 interface.
 Because this object is created and never destroyed, Yori only
 needs the control functions.
 */
typedef struct _YORI_SH_ITASKBARLIST_VTBL {

    /**
     Functions Yori doesn't call and doesn't need declarations for
     */
    PVOID Ignored[9];

    /**
     Pointer to the SetProgressValue API.
     */
    ITaskbarList_SetProgressValue * SetProgressValue;

    /**
     Pointer to the SetProgressState API.
     */
    ITaskbarList_SetProgressState * SetProgressState;

} YORI_SH_ITASKBARLIST_VTBL, *PYORI_SH_ITASKBARLIST_VTBL;

/**
 Pointer to the object implementing ITaskbarList3.
 */
PYORI_SH_TASKBARLIST YoriShTaskbarList; 

/**
 A GUID describing the TaskbarList object.
 */
CONST GUID CLSID_TaskbarList = { 0x56fdf344, 0xfd6d, 0x11d0, { 0x95, 0x8a, 0x00, 0x60, 0x97, 0xc9, 0xa0, 0x90}};

/**
 A GUID describing the ITaskbarList3 interface.
 */
CONST GUID IID_ITaskbarList3 = { 0xea1afb91, 0x9e28, 0x4b86, { 0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf}};

/**
 Set the current taskbar button state.  This is used to indicate a process
 in flight or how it was completed.

 @param State The new state of the window.  Can be one of
        YORI_SH_TASK_SUCCESS, YORI_SH_TASK_FAILED,
        YORI_SH_TASK_IN_PROGRESS, or YORI_SH_TASK_COMPLETE.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShSetWindowState(
    __in DWORD State
    )
{
    HRESULT hr;

    //
    //  Subshells shouldn't do anything.  Let the parent shell display
    //  progress and track completion.
    //

    if (YoriShGlobal.SubShell || YoriShGlobal.ImplicitSynchronousTaskActive) {
        return TRUE;
    }

    if (DllKernel32.pGetConsoleWindow == NULL) {
        return FALSE;
    }

    //
    //  If no UI has been displayed and we're being asked to clear it, do
    //  nothing.  This is to avoid loading COM etc when the user isn't
    //  using the feature.  If we're being invoked from within a builtin
    //  command (ie., script), it's okay to start the progress indicator but
    //  don't display success or fail until the builtin is done.
    //

    if (State != YORI_SH_TASK_IN_PROGRESS) {

        if (!YoriShGlobal.TaskUiActive) {
            return TRUE;
        }

        if (YoriShGlobal.RecursionDepth > 0) {
            return TRUE;
        }
    }

    if (!YoriShGlobal.InitializedCom) {

        YoriLibLoadOle32Functions();
        if (DllOle32.pCoInitialize == NULL || DllOle32.pCoCreateInstance == NULL) {
            return FALSE;
        }

        hr = DllOle32.pCoInitialize(NULL);
        if (!SUCCEEDED(hr)) {
            return FALSE;
        }

        YoriShGlobal.InitializedCom = TRUE;
    }

    if (YoriShTaskbarList == NULL) {
        hr = DllOle32.pCoCreateInstance(&CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskbarList3, &YoriShTaskbarList);
        if (!SUCCEEDED(hr)) {
            return FALSE;
        }
    }

    if (State == YORI_SH_TASK_SUCCESS) {
        YoriShTaskbarList->Vtbl->SetProgressState(YoriShTaskbarList, DllKernel32.pGetConsoleWindow(), 0x2);
        YoriShTaskbarList->Vtbl->SetProgressValue(YoriShTaskbarList, DllKernel32.pGetConsoleWindow(), 100, 100);
        YoriShGlobal.TaskUiActive = TRUE;
    } else if (State == YORI_SH_TASK_FAILED) {
        YoriShTaskbarList->Vtbl->SetProgressState(YoriShTaskbarList, DllKernel32.pGetConsoleWindow(), 0x4);
        YoriShTaskbarList->Vtbl->SetProgressValue(YoriShTaskbarList, DllKernel32.pGetConsoleWindow(), 100, 100);
        YoriShGlobal.TaskUiActive = TRUE;
    } else if (State == YORI_SH_TASK_IN_PROGRESS) {
        YoriShTaskbarList->Vtbl->SetProgressState(YoriShTaskbarList, DllKernel32.pGetConsoleWindow(), 0x1);
        YoriShGlobal.TaskUiActive = TRUE;
    } else if (State == YORI_SH_TASK_COMPLETE) {
        YoriShTaskbarList->Vtbl->SetProgressState(YoriShTaskbarList, DllKernel32.pGetConsoleWindow(), 0);
        YoriShGlobal.TaskUiActive = FALSE;
    }

    return TRUE;
}

/**
 Kill a single process specified by process identifier.

 @return TRUE to indicate that the process was terminated, FALSE to indicate
         that it could not be terminated.
 */
BOOLEAN
YoriShKillProcessById(
    __in DWORD ProcessId
    )
{
    HANDLE ProcessHandle;

    ProcessHandle = OpenProcess(PROCESS_TERMINATE, FALSE, ProcessId);
    if (ProcessHandle == NULL) {
        return FALSE;
    }

    if (!TerminateProcess(ProcessHandle, EXIT_FAILURE)) {
        CloseHandle(ProcessHandle);
        return FALSE;
    }

    CloseHandle(ProcessHandle);
    return TRUE;
}

/**
 Kill all processes associated with the current console except for the
 currently executing process.  This process will also exit momentarily.

 @return TRUE to indicate all other processes associated with the console
         have been killed, and FALSE to indicate that one or more of them
         could not be terminated.
 */
BOOLEAN
YoriShCloseWindow()
{
    LPDWORD PidList = NULL;
    DWORD PidListSize = 0;
    DWORD PidCountReturned;
    DWORD Index;
    DWORD CurrentPid;


    if (DllKernel32.pGetConsoleProcessList == NULL) {
        return FALSE;
    }

    CurrentPid = GetCurrentProcessId();

    do {
        PidCountReturned = DllKernel32.pGetConsoleProcessList(PidList, PidListSize);
        if (PidCountReturned == 0 && PidList != NULL) {
            if (PidList) {
                YoriLibFree(PidList);
            }
            return FALSE;
        }

        if (PidCountReturned > 0 && PidCountReturned <= PidListSize) {
            break;
        }

        if (PidCountReturned == 0) {
            PidCountReturned = 4;
        }

        if (PidList) {
            YoriLibFree(PidList);
        }

        PidListSize = PidCountReturned + 4;
        PidList = YoriLibMalloc(PidListSize * sizeof(DWORD));
        if (PidList == NULL) {
            return FALSE;
        }

    } while(TRUE);

    for (Index = 0; Index < PidCountReturned; Index++) {
        if (PidList[Index] != CurrentPid) {
            if (!YoriShKillProcessById(PidList[Index])) {
                YoriLibFree(PidList);
                return FALSE;
            }
        }
    }

    YoriLibFree(PidList);
    return TRUE;
}

// vim:sw=4:ts=4:et:
