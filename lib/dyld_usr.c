/**
 * @file lib/dyld_usr.c
 *
 * Yori dynamically loaded OS function support
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Map a function name to a location in memory to store a function pointer.
 */
typedef struct _YORI_DLL_NAME_MAP {

    /**
     Pointer to a memory location to be updated with a function pointer when
     the specified function is resolved.
     */
    FARPROC * FnPtr;

    /**
     The name of the function to resolve.
     */
    LPCSTR FnName;
} YORI_DLL_NAME_MAP, *PYORI_DLL_NAME_MAP;

/**
 A structure containing pointers to user32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_USER32_FUNCTIONS DllUser32;

/**
 The set of functions to resolve from user32.dll.
 */
CONST YORI_DLL_NAME_MAP DllUser32Symbols[] = {
    {(FARPROC *)&DllUser32.pCascadeWindows, "CascadeWindows"},
    {(FARPROC *)&DllUser32.pCloseClipboard, "CloseClipboard"},
    {(FARPROC *)&DllUser32.pDdeClientTransaction, "DdeClientTransaction"},
    {(FARPROC *)&DllUser32.pDdeConnect, "DdeConnect"},
    {(FARPROC *)&DllUser32.pDdeCreateDataHandle, "DdeCreateDataHandle"},
    {(FARPROC *)&DllUser32.pDdeCreateStringHandleW, "DdeCreateStringHandleW"},
    {(FARPROC *)&DllUser32.pDdeDisconnect, "DdeDisconnect"},
    {(FARPROC *)&DllUser32.pDdeFreeStringHandle, "DdeFreeStringHandle"},
    {(FARPROC *)&DllUser32.pDdeInitializeW, "DdeInitializeW"},
    {(FARPROC *)&DllUser32.pDdeUninitialize, "DdeUninitialize"},
    {(FARPROC *)&DllUser32.pDrawFrameControl, "DrawFrameControl"},
    {(FARPROC *)&DllUser32.pDrawIconEx, "DrawIconEx"},
    {(FARPROC *)&DllUser32.pEmptyClipboard, "EmptyClipboard"},
    {(FARPROC *)&DllUser32.pEnumClipboardFormats, "EnumClipboardFormats"},
    {(FARPROC *)&DllUser32.pExitWindowsEx, "ExitWindowsEx"},
    {(FARPROC *)&DllUser32.pFindWindowW, "FindWindowW"},
    {(FARPROC *)&DllUser32.pGetClipboardData, "GetClipboardData"},
    {(FARPROC *)&DllUser32.pGetClipboardFormatNameW, "GetClipboardFormatNameW"},
    {(FARPROC *)&DllUser32.pGetClientRect, "GetClientRect"},
    {(FARPROC *)&DllUser32.pGetDesktopWindow, "GetDesktopWindow"},
    {(FARPROC *)&DllUser32.pGetKeyboardLayout, "GetKeyboardLayout"},
    {(FARPROC *)&DllUser32.pGetTaskmanWindow, "GetTaskmanWindow"},
    {(FARPROC *)&DllUser32.pGetWindowRect, "GetWindowRect"},
    {(FARPROC *)&DllUser32.pLockWorkStation, "LockWorkStation"},
    {(FARPROC *)&DllUser32.pMoveWindow, "MoveWindow"},
    {(FARPROC *)&DllUser32.pOpenClipboard, "OpenClipboard"},
    {(FARPROC *)&DllUser32.pRegisterClipboardFormatW, "RegisterClipboardFormatW"},
    {(FARPROC *)&DllUser32.pRegisterShellHookWindow, "RegisterShellHookWindow"},
    {(FARPROC *)&DllUser32.pSendMessageTimeoutW, "SendMessageTimeoutW"},
    {(FARPROC *)&DllUser32.pSetClipboardData, "SetClipboardData"},
    {(FARPROC *)&DllUser32.pSetForegroundWindow, "SetForegroundWindow"},
    {(FARPROC *)&DllUser32.pSetTaskmanWindow, "SetTaskmanWindow"},
    {(FARPROC *)&DllUser32.pSetWindowPos, "SetWindowPos"},
    {(FARPROC *)&DllUser32.pSetWindowTextW, "SetWindowTextW"},
    {(FARPROC *)&DllUser32.pShowWindow, "ShowWindow"},
    {(FARPROC *)&DllUser32.pShowWindowAsync, "ShowWindowAsync"},
    {(FARPROC *)&DllUser32.pTileWindows, "TileWindows"}
};

/**
 Load pointers to all optional user32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadUser32Functions(VOID)
{
    DWORD Count;

    if (DllUser32.hDll != NULL) {
        return TRUE;
    }

    DllUser32.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("USER32.DLL"));
    if (DllUser32.hDll == NULL) {
        return FALSE;
    }


    for (Count = 0; Count < sizeof(DllUser32Symbols)/sizeof(DllUser32Symbols[0]); Count++) {
        if (*(DllUser32Symbols[Count].FnPtr) == NULL) {
            *(DllUser32Symbols[Count].FnPtr) = GetProcAddress(DllUser32.hDll, DllUser32Symbols[Count].FnName);
        }
    }


    return TRUE;
}

// vim:sw=4:ts=4:et:
