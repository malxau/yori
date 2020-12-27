/**
 * @file lib/cancel.c
 *
 * Yori lib Ctrl+C processing support
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Event to be signalled when Ctrl+C is pressed.
 */
HANDLE g_CancelEvent;

/**
 TRUE if the system is currently configured to handle Ctrl+C input.
 */
BOOL g_CancelHandlerSet;

/**
 TRUE if the system should perform no processing in response to
 Ctrl+C input.
 */
BOOL g_CancelIgnore;

/**
 A handler to be invoked when a Ctrl+C or Ctrl+Break event is caught.

 @param CtrlType Indicates the type of the control message.

 @return FALSE to indicate that this handler performed no processing and the
         signal should be sent to the next handler.  TRUE to indicate this
         handler processed the signal and no other handler should be invoked.
 */
BOOL WINAPI
YoriLibCtrlCHandler(
    __in DWORD CtrlType
    )
{
    ASSERT(g_CancelEvent != NULL);
    ASSERT(g_CancelHandlerSet);
    if (!g_CancelIgnore && g_CancelEvent != NULL) {
        SetEvent(g_CancelEvent);
    }

    if (CtrlType == CTRL_CLOSE_EVENT ||
        CtrlType == CTRL_LOGOFF_EVENT ||
        CtrlType == CTRL_SHUTDOWN_EVENT) {

        return FALSE;
    }

    if (CtrlType == CTRL_C_EVENT ||
        CtrlType == CTRL_BREAK_EVENT) {

        return TRUE;
    }

    return TRUE;
}

/**
 Configure the system to be ready to process Ctrl+C keystrokes, including
 initializing the event, registering a handler, and telling the handler
 to process the key.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCancelEnable(VOID)
{
    if (g_CancelEvent == NULL) {
        g_CancelEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (g_CancelEvent == NULL) {
            return FALSE;
        }
    }

    g_CancelIgnore = FALSE;
    SetConsoleCtrlHandler(NULL, FALSE);
    if (!g_CancelHandlerSet) {
        g_CancelHandlerSet = TRUE;
        SetConsoleCtrlHandler(YoriLibCtrlCHandler, TRUE);
    }
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    return TRUE;
}

/**
 Configure the system to stop handling Ctrl+C keystrokes, by unregistering
 the handler.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCancelDisable(VOID)
{
    ASSERT(g_CancelHandlerSet);
    SetConsoleCtrlHandler(YoriLibCtrlCHandler, FALSE);
    SetConsoleCtrlHandler(NULL, TRUE);
    g_CancelHandlerSet = FALSE;
    return TRUE;
}

/**
 Continue handling Ctrl+C keystrokes, but perform no action when they arrive.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCancelIgnore(VOID)
{
    ASSERT(g_CancelHandlerSet);
    g_CancelIgnore = TRUE;
    return TRUE;
}

/**
 Return TRUE to indicate that the operation has been cancelled, FALSE if it
 has not.
 */
BOOL
YoriLibIsOperationCancelled(VOID)
{
    if (g_CancelEvent == NULL) {
        return FALSE;
    }

    if (WaitForSingleObject(g_CancelEvent, 0) == WAIT_OBJECT_0) {
        return TRUE;
    }

    return FALSE;
}

/**
 Reset any cancel state to prepare for the next cancellable operation.
 */
VOID
YoriLibCancelReset(VOID)
{
    if (g_CancelEvent) {
        ResetEvent(g_CancelEvent);
    }
}

/**
 Return the cancel event.
 */
HANDLE
YoriLibCancelGetEvent(VOID)
{
    return g_CancelEvent;
}

// vim:sw=4:ts=4:et:
