/**
 * @file libwin/window.c
 *
 * Yori display an overlapping window
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

#include "yoripch.h"
#include "yorilib.h"
#include "yoriwin.h"

#include "winpriv.h"

/**
 A structure describing a window manager
 */
typedef struct _YORI_WIN_WINDOW_MANAGER {

    /**
     A handle to the output for the console
     */
    HANDLE hConOut;

    /**
     A handle to the input from the console
     */
    HANDLE hConIn;

    /**
     Information about the cursor from when the window manager was started.
     */
    CONSOLE_CURSOR_INFO SavedCursorInfo;

    /**
     Information about the screen buffer from when the window manager was
     started.
     */
    CONSOLE_SCREEN_BUFFER_INFO SavedScreenBufferInfo;

    /**
     The mouse buttons that were pressed last time a mouse event was
     processed.  This is used to detect which buttons were pressed or
     released.
     */
    DWORD PreviousMouseButtonState;

    /**
     Set to TRUE if the console is a v2 console.  These appear to have fixed
     bugs relating to the coordinates of mouse wheel events.  As of this
     writing, mouse wheel is only supported here on v2 - supporting on v1
     would require some manual fudging of coordinates.
     */
    BOOLEAN IsConhostv2;

    /**
     Set to TRUE to indicate that SavedCursorInfo is valid and should be
     restored on exit.
     */
    BOOLEAN HaveSavedCursorInfo;

    /**
     Set to TRUE to indicate that SavedScreenBufferInfo is valid and should
     be restored on exit.
     */
    BOOLEAN HaveSavedScreenBufferInfo;

} YORI_WIN_WINDOW_MANAGER, *PYORI_WIN_WINDOW_MANAGER;

/**
 Close and free the window manager.

 @param WinMgrHandle Pointer to the window manager.
 */
VOID
YoriWinCloseWindowManager(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{

    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    if (WinMgr->HaveSavedScreenBufferInfo) {
        SetConsoleCursorPosition(WinMgr->hConOut, WinMgr->SavedScreenBufferInfo.dwCursorPosition);
    }

    if (WinMgr->HaveSavedCursorInfo) {
        SetConsoleCursorInfo(WinMgr->hConOut, &WinMgr->SavedCursorInfo);
    }

    if (WinMgr->hConOut != NULL) {
        CloseHandle(WinMgr->hConOut);
    }

    if (WinMgr->hConIn != NULL) {
        CloseHandle(WinMgr->hConIn);
    }

    YoriLibFree(WinMgr);
}


/**
 Initialize and open a window manager.  This should be called once per process
 that is interacting with the display via UI.

 @param WinMgrHandle On successful completion, populated with a pointer to the
        window manager.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriWinOpenWindowManager(
    __out PYORI_WIN_WINDOW_MANAGER_HANDLE *WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr;

    WinMgr = YoriLibMalloc(sizeof(YORI_WIN_WINDOW_MANAGER));
    if (WinMgr == NULL) {
        return FALSE;
    }

    WinMgr->hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (WinMgr->hConOut == INVALID_HANDLE_VALUE) {
        WinMgr->hConOut = NULL;
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    WinMgr->hConIn = CreateFile(_T("CONIN$"), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (WinMgr->hConIn == INVALID_HANDLE_VALUE) {
        WinMgr->hConIn = NULL;
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (GetConsoleCursorInfo(WinMgr->hConOut, &WinMgr->SavedCursorInfo)) {
        WinMgr->HaveSavedCursorInfo = TRUE;
    }
    if (GetConsoleScreenBufferInfo(WinMgr->hConOut, &WinMgr->SavedScreenBufferInfo)) {
        WinMgr->HaveSavedScreenBufferInfo = TRUE;
    }

    //
    //  Probe for Conhostv2 by asking for a flag that only it supports.
    //  Conhostv2 reports coordinates differently (correctly) for mouse
    //  wheel events, whereas v1's coordinates are a mess.
    //

    if (SetConsoleMode(WinMgr->hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        WinMgr->IsConhostv2 = TRUE;
    }

    SetConsoleMode(WinMgr->hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
    SetConsoleMode(WinMgr->hConIn, ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);

    *WinMgrHandle = WinMgr;
    return TRUE;
}

/**
 Returns the console input handle associated with the window manager.

 @param WinMgrHandle Pointer to the window manager.

 @return A handle to the console input.
 */
HANDLE
YoriWinGetConsoleInputHandle(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    return WinMgr->hConIn;
}

/**
 Returns the console output handle associated with the window manager.

 @param WinMgrHandle Pointer to the window manager.

 @return A handle to the console output.
 */
HANDLE
YoriWinGetConsoleOutputHandle(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    return WinMgr->hConOut;
}

/**
 Return TRUE if the console being used to manage the windows is a v2 console,
 which happens to handle mouse wheel events in a correct fashion.

 @param WinMgrHandle Pointer to the window manager.

 @return TRUE to indicate the console is a v2 console, FALSE if it is a v1
         console.
 */
BOOLEAN
YoriWinIsConhostv2(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    return WinMgr->IsConhostv2;
}

/**
 Return the mask of mouse buttons which were pressed last time mouse input was
 processed.

 @param WinMgrHandle Pointer to the window manager.

 @return A mask of which mouse buttons were pressed last time mouse input was
         processed.
 */
DWORD
YoriWinGetPreviousMouseButtonState(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    return WinMgr->PreviousMouseButtonState;
}

/**
 Set the mask of mouse buttons which were pressed after performing mouse
 processing which will be used to calculate which buttons were pressed and
 released on the next processing pass.

 @param WinMgrHandle Pointer to the window manager.

 @param PreviousMouseButtonState Specifies the mask of mouse buttons which
        are pressed after processing an input event.
 */
VOID
YoriWinSetPreviousMouseButtonState(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in DWORD PreviousMouseButtonState
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    WinMgr->PreviousMouseButtonState = PreviousMouseButtonState;
}


// vim:sw=4:ts=4:et:
