/**
 * @file yui/yuishook.c
 *
 * Absurd DLL for generating window messages from shell hook notifications
 * which was broken on server core for no good reason
 *
 * Copyright (c) 2019 Malcolm J. Smith
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
 The window handle to inform via window messages about window state changes.
 */
HWND YuiShookWindowToInform;

/**
 The message identifier to use to inform the target window about window state
 changes.
 */
UINT YuiShookMessage;


/**
 A callback function invoked by the window manager to inform about window
 state changes.  Events of interest are translated into window messages and
 sent to the window monitoring for changes.

 @param nCode The event code from the window hook.

 @param wParam The first event-specific parameter.

 @param lParam The second event-specific parameter.

 @return An event specific return code.
 */
LRESULT WINAPI
YuiShookShellHookFn(
    __in int nCode,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    UNREFERENCED_PARAMETER(lParam);
#if DBG
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("WndToInform %08x Msg %08x ShellHookFn Code %08x wParam %016llx\n"), YuiShookWindowToInform, YuiShookMessage, nCode, wParam);
#endif
    switch(nCode) {
        case HSHELL_WINDOWACTIVATED:
        case HSHELL_WINDOWCREATED:
        case HSHELL_WINDOWDESTROYED:
            PostMessage(YuiShookWindowToInform, YuiShookMessage, nCode, wParam);
            break;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}


/**
 A DLL exported function to record which window to notify about window state
 changes.

 @param hWnd The window to sent messages to about window state changes.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiShookRegisterShellHookWindow(
    __in HWND hWnd
    )
{
    YuiShookWindowToInform = hWnd;
    YuiShookMessage = RegisterWindowMessage(_T("SHELLHOOK"));

    if (!SetWindowsHookEx(WH_SHELL, YuiShookShellHookFn, GetModuleHandle(_T("YUISHOOK")), 0)) {
#if DBG
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("SetWindowsHookEx failed %i\n"), GetLastError());
#endif
        return FALSE;
    }

    return TRUE;
}

// vim:sw=4:ts=4:et:
