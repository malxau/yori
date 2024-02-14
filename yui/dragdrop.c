/**
 * @file yui/dragdrop.c
 *
 * Yori shell minimal drag drop handler
 *
 * Copyright (c) 2023 Malcolm J. Smith
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
#include "yui.h"


/**
 A structure describing a Yui drag and drop context.
 */
typedef struct _YUI_DROPTARGET {

    /**
     Pointer to the functions that implement the IDropTarget interface.
     */
    struct _YUI_DROPTARGET_VTBL CONST *Vtbl;

    /**
     The reference count for this object.
     */
    DWORD ReferenceCount;

    /**
     Remembers the last taskbar control ID that has been activated by a
     drag and drop operation, or zero if the drag and drop operation has
     not activated a control ID.  This is used if a drag moves across
     multiple buttons, so a drag move operation needs to change the active
     window.
     */
    WORD PreviouslyActivatedCtrl;

    /**
     Pointer to the application context.
     */
    PYUI_CONTEXT YuiContext;

    /**
     The taskbar window handle associated with this drag and drop context.
     */
    HWND hWnd;
} YUI_DROPTARGET, *PYUI_DROPTARGET;

/**
 Function prototype for the IDropTarget QueryInterface method.
 */
typedef HRESULT WINAPI YUI_DROPTARGET_QUERY_INTERFACE(PYUI_DROPTARGET, CONST GUID *, PVOID *);

/**
 Type for a pointer to an IDropTarget QueryInterface method.
 */
typedef YUI_DROPTARGET_QUERY_INTERFACE *PYUI_DROPTARGET_QUERY_INTERFACE;

/**
 Function prototype for the IDropTarget AddRef method.
 */
typedef DWORD WINAPI YUI_DROPTARGET_ADDREF(PYUI_DROPTARGET);

/**
 Type for a pointer to an IDropTarget AddRef method.
 */
typedef YUI_DROPTARGET_ADDREF *PYUI_DROPTARGET_ADDREF;

/**
 Function prototype for the IDropTarget Release method.
 */
typedef DWORD WINAPI YUI_DROPTARGET_RELEASE(PYUI_DROPTARGET);

/**
 Type for a pointer to an IDropTarget Release method.
 */
typedef YUI_DROPTARGET_RELEASE *PYUI_DROPTARGET_RELEASE;

/**
 Function prototype for the IDropTarget DragEnter method.
 */
typedef HRESULT WINAPI YUI_DROPTARGET_DRAGENTER(PYUI_DROPTARGET, PVOID, DWORD, POINTL, PDWORD);

/**
 Type for a pointer to an IDropTarget DragEnter method.
 */
typedef YUI_DROPTARGET_DRAGENTER *PYUI_DROPTARGET_DRAGENTER;

/**
 Function prototype for the IDropTarget DragOver method.
 */
typedef HRESULT WINAPI YUI_DROPTARGET_DRAGOVER(PYUI_DROPTARGET, DWORD, POINTL, PDWORD);

/**
 Type for a pointer to an IDropTarget DragOver method.
 */
typedef YUI_DROPTARGET_DRAGOVER *PYUI_DROPTARGET_DRAGOVER;

/**
 Function prototype for the IDropTarget DragLeave method.
 */
typedef HRESULT WINAPI YUI_DROPTARGET_DRAGLEAVE(PYUI_DROPTARGET);

/**
 Type for a pointer to an IDropTarget DragLeave method.
 */
typedef YUI_DROPTARGET_DRAGLEAVE *PYUI_DROPTARGET_DRAGLEAVE;

/**
 Function prototype for the IDropTarget Drop method.
 */
typedef HRESULT WINAPI YUI_DROPTARGET_DROP(PYUI_DROPTARGET, PVOID, DWORD, POINTL, PDWORD);

/**
 Type for a pointer to an IDropTarget Drop method.
 */
typedef YUI_DROPTARGET_DRAGENTER *PYUI_DROPTARGET_DROP;


/**
 A structure describing the function pointers implemented by the IDropTarget
 interface.
 */
typedef struct _YUI_DROPTARGET_VTBL {

    /**
     Pointer to the QueryInterface method.
     */
    PYUI_DROPTARGET_QUERY_INTERFACE QueryInterface;

    /**
     Pointer to the AddRef method.
     */
    PYUI_DROPTARGET_ADDREF AddRef;

    /**
     Pointer to the Release method.
     */
    PYUI_DROPTARGET_RELEASE Release;

    /**
     Pointer to the DragEnter method.
     */
    PYUI_DROPTARGET_DRAGENTER DragEnter;

    /**
     Pointer to the DragOver method.
     */
    PYUI_DROPTARGET_DRAGOVER DragOver;

    /**
     Pointer to the DragLeave method.
     */
    PYUI_DROPTARGET_DRAGLEAVE DragLeave;

    /**
     Pointer to the Drop method.
     */
    PYUI_DROPTARGET_DROP Drop;
} YUI_DROPTARGET_VTBL, *PYUI_DROPTARGET_VTBL;

/**
 The GUID to identify an IDropTarget interface.
 */
CONST GUID IID_YuiIDropTarget = { 0x122, 0x00, 0x00, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }};

/**
 The GUID to identify an IUnknown interface.
 */
CONST GUID IID_YuiIUnknown = { 0x00, 0x00, 0x00, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }};

/**
 Lookup an interface from a drag and drop context.

 @param This Pointer to the drag and drop context.
 
 @param Iid The interface being queried.

 @param pObject On successful completion, updated to point to a reference
        counted object that implements the specified interface.  In this
        application, only one interface is supported, so it always points
        to the original object.

 @return S_OK to indicate success, or E_NOINTERFACE if the interface is not
         supported by this program.
 */
HRESULT WINAPI
YuiDropTargetQueryInterface(
    __in PYUI_DROPTARGET This,
    __in CONST GUID * Iid,
    __out PVOID *pObject
    )
{
    *pObject = NULL;

    if (memcmp(Iid, &IID_YuiIUnknown, sizeof(GUID)) == 0 ||
        memcmp(Iid, &IID_YuiIDropTarget, sizeof(GUID)) == 0) {

        This->Vtbl->AddRef(This);
        *pObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

/**
 Add a reference to a drag and drop context.

 @param This Pointer to the context to add the reference to.

 @return The new reference count.
 */
DWORD WINAPI
YuiDropTargetAddRef(
    __in PYUI_DROPTARGET This
    )
{
    return InterlockedIncrement(&This->ReferenceCount);
}

/**
 Release a reference from a drag and drop context.

 @param This Pointer to the context to release the reference from.

 @return The remaining reference count.
 */
DWORD WINAPI
YuiDropTargetRelease(
    __in PYUI_DROPTARGET This
    )
{
    DWORD NewCount;

    NewCount = InterlockedDecrement(&This->ReferenceCount);
    if (NewCount == 0) {
        YoriLibFree(This);
    }

    return NewCount;
}

/**
 A callback invoked when an object is dragged into the taskbar.

 @param This Pointer to the window's drag and drop context.

 @param pData Pointer to the data being dropped, which is defined by an
        outside application.

 @param dwKeyState The key modifiers held at the time of the drop.

 @param pt Position within the window where the drop occurred.

 @param pdwEffect The drop effect to use for the operation.

 @return S_OK to indicate success, or appropriate error.
 */
HRESULT WINAPI
YuiDropTargetDragEnter(
    __in PYUI_DROPTARGET This,
    __in PVOID pData,
    __in DWORD dwKeyState,
    __in POINTL pt,
    __inout PDWORD pdwEffect
    )
{
    WORD CtrlId;

    UNREFERENCED_PARAMETER(pData);
    UNREFERENCED_PARAMETER(dwKeyState);
    UNREFERENCED_PARAMETER(pdwEffect);

    CtrlId = YuiTaskbarFindByOffset(This->YuiContext, (SHORT)pt.x);
    if (CtrlId != 0) {
        YuiTaskbarSwitchToTask(This->YuiContext, CtrlId);
        This->PreviouslyActivatedCtrl = CtrlId;
    }

    return S_OK;
}

/**
 A callback invoked when an object is dragged across the taskbar.

 @param This Pointer to the window's drag and drop context.

 @param dwKeyState The key modifiers held at the time of the drop.

 @param pt Position within the window where the drop occurred.

 @param pdwEffect The drop effect to use for the operation.

 @return S_OK to indicate success, or appropriate error.
 */
HRESULT WINAPI
YuiDropTargetDragOver(
    __in PYUI_DROPTARGET This,
    __in DWORD dwKeyState,
    __in POINTL pt,
    __inout PDWORD pdwEffect
    )
{
    WORD CtrlId;

    UNREFERENCED_PARAMETER(dwKeyState);
    UNREFERENCED_PARAMETER(pdwEffect);

    CtrlId = YuiTaskbarFindByOffset(This->YuiContext, (SHORT)pt.x);
    if (CtrlId != 0 &&
        CtrlId != This->PreviouslyActivatedCtrl) {

        YuiTaskbarSwitchToTask(This->YuiContext, CtrlId);
        This->PreviouslyActivatedCtrl = CtrlId;
    }

    return S_OK;
}

/**
 A callback invoked when an object is dragged outside the taskbar.

 @param This Pointer to the window's drag and drop context.

 @return S_OK to indicate success, or appropriate error.
 */
HRESULT WINAPI
YuiDropTargetDragLeave(
    __in PYUI_DROPTARGET This
    )
{
    This->PreviouslyActivatedCtrl = 0;
    return S_OK;
}

/**
 A callback invoked when an object is dropped on the taskbar.

 @param This Pointer to the window's drag and drop context.

 @param pData Pointer to the data being dropped, which is defined by an
        outside application.

 @param dwKeyState The key modifiers held at the time of the drop.

 @param pt Position within the window where the drop occurred.

 @param pdwEffect The drop effect to use for the operation.

 @return S_OK to indicate success, or appropriate error.
 */
HRESULT WINAPI
YuiDropTargetDrop(
    __in PYUI_DROPTARGET This,
    __in PVOID pData,
    __in DWORD dwKeyState,
    __in POINTL pt,
    __inout PDWORD pdwEffect
    )
{
    UNREFERENCED_PARAMETER(pData);
    UNREFERENCED_PARAMETER(dwKeyState);
    UNREFERENCED_PARAMETER(pt);
    UNREFERENCED_PARAMETER(pdwEffect);
    This->PreviouslyActivatedCtrl = 0;
    return S_OK;
}

/**
 An application global function table.  This is implementing the IDropTarget
 interface and functions here must match those signatures.
 */
CONST YUI_DROPTARGET_VTBL
YuiDropTargetVtbl = {
    YuiDropTargetQueryInterface,
    YuiDropTargetAddRef,
    YuiDropTargetRelease,
    YuiDropTargetDragEnter,
    YuiDropTargetDragOver,
    YuiDropTargetDragLeave,
    YuiDropTargetDrop
};

/**
 Register a window's drop support so objects can be dragged to the taskbar.

 @param YuiContext Pointer to the application context.

 @param hWnd The window handle.

 @return On successful completion, returns an opaque pointer to an allocated
         context.  The caller should free this with
         @ref YuiUnregisterDropWindow .
 */
PVOID
YuiRegisterDropWindow(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    PYUI_DROPTARGET DropTarget;
    HRESULT hr;

    YoriLibLoadOle32Functions();
    if (DllOle32.pOleInitialize == NULL ||
        DllOle32.pOleUninitialize == NULL ||
        DllOle32.pCoLockObjectExternal == NULL ||
        DllOle32.pRegisterDragDrop == NULL ||
        DllOle32.pRevokeDragDrop == NULL) {

        return NULL;
    }

    DropTarget = YoriLibMalloc(sizeof(YUI_DROPTARGET));
    if (DropTarget == NULL) {
        return NULL;
    }

    DropTarget->Vtbl = &YuiDropTargetVtbl;
    DropTarget->ReferenceCount = 1;
    DropTarget->YuiContext = YuiContext;
    DropTarget->hWnd = hWnd;

    hr = DllOle32.pOleInitialize(NULL);
    if (hr != S_OK) {
        return NULL;
    }

    hr = DllOle32.pCoLockObjectExternal(DropTarget, TRUE, FALSE);
    if (hr != S_OK) {
        DllOle32.pOleUninitialize();
        YuiDropTargetRelease(DropTarget);
        return NULL;
    }

    hr = DllOle32.pRegisterDragDrop(hWnd, DropTarget);
    if (hr != S_OK) {
        DllOle32.pCoLockObjectExternal(DropTarget, FALSE, TRUE);
        DllOle32.pOleUninitialize();
        YuiDropTargetRelease(DropTarget);
        return NULL;
    }

#if DBG
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("RegisterDragDrop success\n"));
#endif

    return DropTarget;
}

/**
 Unregister a previously registered drop handler.

 @param hWnd The window handle.

 @param DropHandle The allocated state from when the drop handler was created.
 */
VOID
YuiUnregisterDropWindow(
    __in HWND hWnd,
    __in PVOID DropHandle
    )
{
    PYUI_DROPTARGET DropTarget;
    HRESULT hr;

    hr = DllOle32.pRevokeDragDrop(hWnd);
    if (hr != S_OK) {
        return;
    }

    DropTarget = (PYUI_DROPTARGET)DropHandle;
    DllOle32.pCoLockObjectExternal(DropTarget, FALSE, TRUE);
    YuiDropTargetRelease(DropHandle);
    DllOle32.pOleUninitialize();
}


// vim:sw=4:ts=4:et:
