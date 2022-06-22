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

#if DBG

/**
 Set to nonzero to enable cancel debugging
 */
#define YORI_CANCEL_DBG 1

/**
 The number of stack frames to capture for debugging.
 */
#define YORI_CANCEL_DBG_STACK_FRAMES (10)

#endif

/**
 A structure containing process global state for cancel support.
 */
typedef struct _YORI_LIB_CANCEL_STATE {

    /**
     Event to be signalled when Ctrl+C is pressed.
     */
    HANDLE Event;

    /**
     TRUE if the system is currently configured to handle Ctrl+C input.
     */
    BOOLEAN HandlerSet;

    /**
     TRUE if the system should perform no processing in response to
     Ctrl+C input.
     */
    BOOLEAN Ignore;

    /**
     TRUE to indicate SetConsoleCtrlHandler has been invoked with NULL/TRUE
     to ignore handling by this and child processes.  FALSE to indicate it
     has been called with NULL/FALSE to resume handling by this and child
     processes.
     */
    BOOLEAN InheritedIgnore;

    /**
     TRUE to indicate that input has PROCESSED_INPUT set so that Ctrl+C will
     be processed as a signal.
     */
    BOOLEAN ProcessedInput;

#if YORI_CANCEL_DBG
    /**
     A stack of when the Ignore value was changed.
     */
    PVOID DbgIgnoreStack[YORI_CANCEL_DBG_STACK_FRAMES];

    /**
     A stack of when the handler was last invoked.
     */
    PVOID DbgHandlerInvokedStack[YORI_CANCEL_DBG_STACK_FRAMES];

    /**
     A stack of when the InheritedIgnore value was changed.
     */
    PVOID DbgInheritedStateStack[YORI_CANCEL_DBG_STACK_FRAMES];

    /**
     A stack of when the console input mode value was changed, including
     setting ProcessedInput.
     */
    PVOID DbgConsoleInputModeStack[YORI_CANCEL_DBG_STACK_FRAMES];
#endif

} YORI_LIB_CANCEL_STATE;

/**
 Process global state for cancel support.
 */
YORI_LIB_CANCEL_STATE g_YoriLibCancel;

/**
 Set the console attribute indicating that Ctrl+C signals should be ignored by
 this and child processes.
 */
VOID
YoriLibCancelInheritedIgnore(VOID)
{
    g_YoriLibCancel.InheritedIgnore = TRUE;
#if YORI_CANCEL_DBG
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        DllKernel32.pRtlCaptureStackBackTrace(1, YORI_CANCEL_DBG_STACK_FRAMES, g_YoriLibCancel.DbgInheritedStateStack, NULL);
    }
#endif
    SetConsoleCtrlHandler(NULL, TRUE);
}

/**
 Set the console attribute indicating that Ctrl+C signals should be processed
 by this and child processes.
 */
VOID
YoriLibCancelInheritedProcess(VOID)
{
    g_YoriLibCancel.InheritedIgnore = FALSE;
#if YORI_CANCEL_DBG
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        DllKernel32.pRtlCaptureStackBackTrace(1, YORI_CANCEL_DBG_STACK_FRAMES, g_YoriLibCancel.DbgInheritedStateStack, NULL);
    }
#endif
    SetConsoleCtrlHandler(NULL, FALSE);
}

/**
 Set the input console mode, while capturing the intention to facilitate
 debugging.

 @param Handle Handle to a console input device.

 @param ConsoleMode The mode to set on a console input handle.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibSetInputConsoleMode(
    __in HANDLE Handle,
    __in DWORD ConsoleMode
    )
{
    if (ConsoleMode & ENABLE_PROCESSED_INPUT) {
        g_YoriLibCancel.ProcessedInput = TRUE;
    } else {
        g_YoriLibCancel.ProcessedInput = FALSE;
    }

#if YORI_CANCEL_DBG
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        DllKernel32.pRtlCaptureStackBackTrace(1, YORI_CANCEL_DBG_STACK_FRAMES, g_YoriLibCancel.DbgConsoleInputModeStack, NULL);
    }
#endif

    return SetConsoleMode(Handle, ConsoleMode);
}

/**
 Indicate that the operation should be cancelled.
 */
VOID
YoriLibCancelSet(VOID)
{
    if (!g_YoriLibCancel.Ignore && g_YoriLibCancel.Event != NULL) {
        SetEvent(g_YoriLibCancel.Event);
    }
}

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
    ASSERT(g_YoriLibCancel.Event != NULL);
    ASSERT(g_YoriLibCancel.HandlerSet);
    YoriLibCancelSet();

#if YORI_CANCEL_DBG
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        DllKernel32.pRtlCaptureStackBackTrace(1, YORI_CANCEL_DBG_STACK_FRAMES, g_YoriLibCancel.DbgHandlerInvokedStack, NULL);
    }
#endif

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
YoriLibCancelEnable(
    __in BOOLEAN Ignore
    )
{
    if (g_YoriLibCancel.Event == NULL) {
        g_YoriLibCancel.Event = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (g_YoriLibCancel.Event == NULL) {
            return FALSE;
        }
    }

#if YORI_CANCEL_DBG
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        DllKernel32.pRtlCaptureStackBackTrace(1, YORI_CANCEL_DBG_STACK_FRAMES, g_YoriLibCancel.DbgIgnoreStack, NULL);
    }
#endif

    g_YoriLibCancel.Ignore = Ignore;
    YoriLibCancelInheritedProcess();
    if (!g_YoriLibCancel.HandlerSet) {
        g_YoriLibCancel.HandlerSet = TRUE;
        SetConsoleCtrlHandler(YoriLibCtrlCHandler, TRUE);
    }
    YoriLibSetInputConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
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
    ASSERT(g_YoriLibCancel.HandlerSet);
    SetConsoleCtrlHandler(YoriLibCtrlCHandler, FALSE);
    YoriLibCancelInheritedIgnore();
    g_YoriLibCancel.HandlerSet = FALSE;
    return TRUE;
}

/**
 Continue handling Ctrl+C keystrokes, but perform no action when they arrive.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCancelIgnore(VOID)
{
    ASSERT(g_YoriLibCancel.HandlerSet);
#if YORI_CANCEL_DBG
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        DllKernel32.pRtlCaptureStackBackTrace(1, YORI_CANCEL_DBG_STACK_FRAMES, g_YoriLibCancel.DbgIgnoreStack, NULL);
    }
#endif
    g_YoriLibCancel.Ignore = TRUE;
    return TRUE;
}

/**
 Return TRUE to indicate that the operation has been cancelled, FALSE if it
 has not.
 */
BOOL
YoriLibIsOperationCancelled(VOID)
{
    if (g_YoriLibCancel.Event == NULL) {
        return FALSE;
    }

    if (WaitForSingleObject(g_YoriLibCancel.Event, 0) == WAIT_OBJECT_0) {
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
    if (g_YoriLibCancel.Event) {
        ResetEvent(g_YoriLibCancel.Event);
    }
}

/**
 Return the cancel event.
 */
HANDLE
YoriLibCancelGetEvent(VOID)
{
    return g_YoriLibCancel.Event;
}

// vim:sw=4:ts=4:et:
