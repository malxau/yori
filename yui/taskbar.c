/**
 * @file yui/taskbar.c
 *
 * Yori shell populate taskbar with windows and allow selection
 *
 * Copyright (c) 2019-2023 Malcolm J. Smith
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
 A custom window procedure used by buttons on the taskbar.  This is a form of
 subclass that enables us to filter the messages processed by the regular
 button implementation.

 @param hWnd Window handle to the button.

 @param uMsg The message identifier.

 @param wParam The first parameter associated with the window message.

 @param lParam The second parameter associated with the window message.

 @return A value which depends on the type of message being processed.
 */
LRESULT CALLBACK
YuiTaskbarButtonWndProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch(uMsg) {
        case WM_NCHITTEST:

            //
            //  Indicate the entire button is not a hit target.  The taskbar
            //  has code to detect presses beneath the button area so will
            //  catch this and handle it.  The reason for not having the
            //  button handle it is this will cause the button to "click",
            //  and when the window activates the buttons state is changed
            //  explicitly, resulting in "flashing" behavior as it's rendered
            //  twice.  By doing this, the button state changes when the
            //  window is activated.
            //

            return HTTRANSPARENT;

        case WM_SETFOCUS:
        case WM_KILLFOCUS:

            //
            //  Focus changes normally repaint controls because they do
            //  dotted lines around text.  This control won't do that, so
            //  swallow the message to avoid a flash.
            //

            return 0;
    }

    return CallWindowProc(YuiGetDefaultButtonWndProc(), hWnd, uMsg, wParam, lParam);
}


/**
 Return TRUE if this window should be included in the taskbar window list.

 @param hWnd The Window to check for taskbar inclusion.

 @return TRUE if the window should be displayed on the taskbar, FALSE if it
         should not.
 */
BOOL
YuiTaskbarIncludeWindow(
    __in HWND hWnd
    )
{
    DWORD WinStyle;
    DWORD ExStyle;
    DWORD WindowTitleLength;
    YORI_STRING ClassName;
    YORI_STRING ExcludePrefix;
    TCHAR ClassNameBuffer[64];

    if (!IsWindowVisible(hWnd)) {
        return FALSE;
    }

    if (!IsWindowEnabled(hWnd)) {
        return FALSE;
    }

    if (GetWindow(hWnd, GW_OWNER) != NULL) {
        return FALSE;
    }

    WinStyle = GetWindowLong(hWnd, GWL_STYLE);
    ExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

    if ((ExStyle & WS_EX_TOOLWINDOW) != 0) {
        return FALSE;
    }

    //
    //  If there's no border and no system menu, it doesn't seem like an
    //  application window.
    //

    if ((WinStyle & (WS_CAPTION | WS_SYSMENU)) == 0) {
        return FALSE;
    }

    WindowTitleLength = GetWindowTextLength(hWnd);
    if (WindowTitleLength == 0) {
        return FALSE;
    }

    //
    //  Explorer creates Windows.Internal.Shell.TabProxyWindow instances.
    //  They're owned by explorer but seem related to what Edge is doing.
    //  Drop them.
    //

    YoriLibInitEmptyString(&ClassName);
    ClassName.StartOfString = ClassNameBuffer;
    ClassName.LengthAllocated = sizeof(ClassNameBuffer)/sizeof(ClassNameBuffer[0]);

    ClassName.LengthInChars = GetClassName(hWnd, ClassName.StartOfString, ClassName.LengthAllocated);

    YoriLibConstantString(&ExcludePrefix, _T("Windows.Internal.Shell."));
    if (YoriLibCompareStringCount(&ExcludePrefix, &ClassName, ExcludePrefix.LengthInChars) == 0) {
        return FALSE;
    }

    //
    //  Office splash screens seem to generate a notification when created but
    //  not when destroyed.  This seems like it has to be an underlying bug in
    //  the platform (allowing window styles to change that affect
    //  notifications without indicating the change) but in the absence of
    //  notifications, there's not much we can do.
    //
    //  The generic fix for this would be a periodic refresh, defeating the
    //  purpose of notifications.
    //

    YoriLibConstantString(&ExcludePrefix, _T("MsoSplash"));
    if (YoriLibCompareString(&ExcludePrefix, &ClassName) == 0) {
        return FALSE;
    }

#if DBG
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("    including window due to style %08x ExStyle %08x\n"), WinStyle, ExStyle);
#endif

    return TRUE;
}

/**
 Check if a window is full screen.

 @param YuiContext Pointer to the application context.

 @param hWnd Handle to the window to check.

 @return TRUE if the window appears full screen, FALSE if not.
 */
BOOLEAN
YuiTaskbarFullscreenWindow(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    RECT WindowRect;
    DWORD RetryCount;

    for (RetryCount = 0; RetryCount < 2; RetryCount++) {

        if (IsIconic(hWnd)) {
            return FALSE;
        }

        if (!GetWindowRect(hWnd, &WindowRect)) {
            return FALSE;
        }

        //
        //  This is a bit rubbery.  A window is a full screen window if it
        //  approximates the screen size.
        //

        if (WindowRect.left > 2 ||
            WindowRect.top > 2 ||
            WindowRect.bottom < (INT)(YuiContext->ScreenHeight - 2) ||
            WindowRect.right < (INT)(YuiContext->ScreenWidth - 2)) {

            return FALSE;
        }

        //
        //  If the window is maximized, it implies that the work area is no
        //  longer excluding the task bar.  Trigger a display refresh to
        //  recalculate the work area, and then re-query the current window
        //  location to see if it's still full screen.
        //

        if (RetryCount == 1 && IsZoomed(hWnd)) {
            YuiNotifyResolutionChange(YuiContext->hWnd);
        } else {
            break;
        }
    }

    return TRUE;
}

/**
 Check if the current window is a full screen window.  If so, hide the
 taskbar.  If the taskbar is currently hidden and the new window is not
 full screen, un-hide the taskbar.

 @param YuiContext Pointer to the application context.

 @param hWnd Handle to the currently active window.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YuiTaskbarUpdateFullscreenStatus(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    BOOLEAN FullscreenWindowActive;

    FullscreenWindowActive = YuiTaskbarFullscreenWindow(YuiContext, hWnd);

    if (YuiContext->FullscreenModeActive) {
        if (!FullscreenWindowActive) {
#if DBG
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Window %08x not fullscreen, displaying taskbar\n"), hWnd);
#endif
            DllUser32.pShowWindow(YuiContext->hWnd, SW_SHOW);
            YuiContext->FullscreenModeActive = FullscreenWindowActive;
        }
    } else if (FullscreenWindowActive) {
#if DBG
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Window %08x fullscreen, hiding taskbar\n"), hWnd);
#endif
        DllUser32.pShowWindow(YuiContext->hWnd, SW_HIDE);
        YuiContext->FullscreenModeActive = FullscreenWindowActive;
    }

    return FullscreenWindowActive;
}


/**
 Allocate a unique identifier for a button control that will be displayed
 on the taskbar.

 @param YuiContext Pointer to the application context.

 @return The identifier for the button control.
 */
WORD
YuiTaskbarGetNewCtrlId(
    __in PYUI_CONTEXT YuiContext
    )
{
    YuiContext->NextTaskbarId++;
    return YuiContext->NextTaskbarId;
}

/**
 Allocate memory for the structure that describes a taskbar button.  Note
 this does not create the button control itself.

 @param YuiContext Pointer to the application context.

 @param hWnd The window handle to the taskbar window.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiTaskbarAllocateAndAddButton(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    PYUI_TASKBAR_BUTTON NewButton;
    DWORD WindowTitleLength;

    WindowTitleLength = GetWindowTextLength(hWnd);

    NewButton = YoriLibReferencedMalloc(sizeof(YUI_TASKBAR_BUTTON) + (WindowTitleLength + 1) * sizeof(TCHAR));
    if (NewButton == NULL) {
        return FALSE;
    }

    YoriLibInitEmptyString(&NewButton->ButtonText);
    NewButton->ButtonText.StartOfString = (LPTSTR)(NewButton + 1);
    NewButton->ButtonText.LengthAllocated = WindowTitleLength + 1;
    YoriLibReference(NewButton);
    NewButton->ButtonText.MemoryToFree = NewButton;
    NewButton->ButtonText.LengthInChars = GetWindowText(hWnd, NewButton->ButtonText.StartOfString, NewButton->ButtonText.LengthAllocated);

    NewButton->hWndToActivate = hWnd;
    NewButton->hWndButton = NULL;
    NewButton->WindowActive = FALSE;
    NewButton->AssociatedWindowFound = TRUE;
    NewButton->Flashing = FALSE;

    YoriLibAppendList(&YuiContext->TaskbarButtons, &NewButton->ListEntry);
    YuiContext->TaskbarButtonCount++;

    return TRUE;
}

/**
 Create a button window for the specified button.

 @param YuiContext Pointer to the Yui context.

 @param ThisButton Pointer to the button structure indicating its text,
        location, and control ID.

 @param TaskbarHwnd The taskbar window (parent.)

 @param Height Height of the new button, in pixels.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiTaskbarCreateButtonControl(
    __in PYUI_CONTEXT YuiContext,
    __in PYUI_TASKBAR_BUTTON ThisButton,
    __in HWND TaskbarHwnd,
    __in WORD Height
    )
{
    ThisButton->hWndButton = CreateWindow(_T("BUTTON"),
                                          _T(""),
                                          BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                                          ThisButton->LeftOffset,
                                          1,
                                          (WORD)(ThisButton->RightOffset - ThisButton->LeftOffset),
                                          Height,
                                          TaskbarHwnd,
                                          (HMENU)(DWORD_PTR)ThisButton->ControlId,
                                          NULL,
                                          NULL);

    if (ThisButton->hWndButton == NULL) {
        return FALSE;
    }

    SetWindowLongPtr(ThisButton->hWndButton, GWLP_WNDPROC, (LONG_PTR)YuiTaskbarButtonWndProc);
    SendMessage(ThisButton->hWndButton, WM_SETFONT, (WPARAM)YuiContext->hFont, MAKELPARAM(TRUE, 0));

    return TRUE;
}


/**
 A callback function that is invoked when initially populating the taskbar
 for every window found currently in existence.

 @param hWnd The window that currently exists.

 @param lParam Pointer to the application context.

 @return TRUE to continue enumerating windows.
 */
BOOL WINAPI
YuiTaskbarWindowCallback(
    __in HWND hWnd,
    __in LPARAM lParam
    )
{
    PYUI_CONTEXT YuiContext;

    YuiContext = (PYUI_CONTEXT)lParam;

#if DBG
    {
        DWORD CharsNeeded;
        YORI_STRING WindowTitle;

        CharsNeeded = GetWindowTextLength(hWnd);
        if (YoriLibAllocateString(&WindowTitle, CharsNeeded + 1)) {
            WindowTitle.LengthInChars = GetWindowText(hWnd, WindowTitle.StartOfString, WindowTitle.LengthAllocated);
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Enumerated window %08x %y\n"), hWnd, &WindowTitle);
        YoriLibFreeStringContents(&WindowTitle);
    }
#endif

    if (!YuiTaskbarIncludeWindow(hWnd)) {
        return TRUE;
    }

    YuiTaskbarAllocateAndAddButton(YuiContext, hWnd);

    return TRUE;
}

/**
 Mark the button as hosting the active window.  This updates internal state,
 sets the font, and updates button to the pressed appearance.

 @param YuiContext Pointer to the application context.

 @param ThisButton Pointer to the button to mark as active.
 */
VOID
YuiTaskbarMarkButtonActive(
    __in PYUI_CONTEXT YuiContext,
    __in PYUI_TASKBAR_BUTTON ThisButton
    )
{
    ThisButton->Flashing = FALSE;
    ThisButton->WindowActive = TRUE;
    SendMessage(ThisButton->hWndButton, WM_SETFONT, (WPARAM)YuiContext->hBoldFont, MAKELPARAM(FALSE, 0));
    SendMessage(ThisButton->hWndButton, BM_SETSTATE, TRUE, 0);
}

/**
 Mark the button as not hosting the active window.  This updates internal
 state, sets the font, and updates button to the raised appearance.

 @param YuiContext Pointer to the application context.

 @param ThisButton Pointer to the button to mark as inactive.
 */
VOID
YuiTaskbarMarkButtonInactive(
    __in PYUI_CONTEXT YuiContext,
    __in PYUI_TASKBAR_BUTTON ThisButton
    )
{
    ThisButton->WindowActive = FALSE;
    SendMessage(ThisButton->hWndButton, WM_SETFONT, (WPARAM)YuiContext->hFont, MAKELPARAM(FALSE, 0));
    SendMessage(ThisButton->hWndButton, BM_SETSTATE, FALSE, 0);
}

/**
 Calculate the width for every taskbar button.  Each button has the same
 width.  The width is going to be the size of the taskbar divided by the
 number of buttons, with a maximum size per button to prevent a single
 window occupying the entire bar, etc.

 @param YuiContext Pointer to the application context.

 @param TaskbarHwnd The taskbar window.

 @return The number of pixels in each taskbar button.
 */
DWORD
YuiTaskbarCalculateButtonWidth(
    __in PYUI_CONTEXT YuiContext,
    __in HWND TaskbarHwnd
    )
{
    RECT TaskbarWindowClient;
    DWORD TotalWidthForButtons;
    DWORD WidthPerButton;

    DllUser32.pGetClientRect(TaskbarHwnd, &TaskbarWindowClient);
    TotalWidthForButtons = TaskbarWindowClient.right - YuiContext->LeftmostTaskbarOffset - YuiContext->RightmostTaskbarOffset - 1;

    if (YuiContext->TaskbarButtonCount == 0) {
        WidthPerButton = YuiContext->MaximumTaskbarButtonWidth;
    } else {
        WidthPerButton = TotalWidthForButtons / YuiContext->TaskbarButtonCount;
        if (WidthPerButton > YuiContext->MaximumTaskbarButtonWidth) {
            WidthPerButton = YuiContext->MaximumTaskbarButtonWidth;
        }
    }

    return WidthPerButton;
}

/**
 Populate the taskbar with the set of windows that exist at the time the
 taskbar was created.

 @param YuiContext Pointer to the application context.

 @param TaskbarHwnd The taskbar window.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiTaskbarPopulateWindows(
    __in PYUI_CONTEXT YuiContext,
    __in HWND TaskbarHwnd
    )
{
    DWORD WidthPerButton;
    DWORD Index;
    PYORI_LIST_ENTRY ListEntry;
    PYUI_TASKBAR_BUTTON ThisButton;
    HWND ActiveWindow;
    RECT TaskbarWindowClient;

    EnumWindows(YuiTaskbarWindowCallback, (LPARAM)YuiContext);

    WidthPerButton = YuiTaskbarCalculateButtonWidth(YuiContext, TaskbarHwnd);

    DllUser32.pGetClientRect(TaskbarHwnd, &TaskbarWindowClient);

    ListEntry = NULL;
    Index = 0;
    ActiveWindow = GetForegroundWindow();
    YuiContext->NextTaskbarId = YUI_FIRST_TASKBAR_BUTTON;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);

        ThisButton->ControlId = YuiTaskbarGetNewCtrlId(YuiContext);
        ThisButton->LeftOffset = (WORD)(YuiContext->LeftmostTaskbarOffset + Index * WidthPerButton + 1),
        ThisButton->RightOffset = (WORD)(ThisButton->LeftOffset + WidthPerButton - 2),
        YuiTaskbarCreateButtonControl(YuiContext, ThisButton, TaskbarHwnd, (WORD)(TaskbarWindowClient.bottom - 2));
        if (ThisButton->hWndToActivate == ActiveWindow) {
            YuiTaskbarMarkButtonActive(YuiContext, ThisButton);
        }
        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
        Index++;
    }

    return TRUE;
}

/**
 Find a button structure from a specified control ID.

 @param YuiContext Pointer to the application context.

 @param CtrlId The control ID to find.
 */
PYUI_TASKBAR_BUTTON
YuiTaskbarFindButtonFromCtrlId(
    __in PYUI_CONTEXT YuiContext,
    __in DWORD CtrlId
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_TASKBAR_BUTTON ThisButton;

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (ThisButton->ControlId == CtrlId) {
            return ThisButton;
        }
        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    }

    return NULL;
}

/**
 Find a button structure from a specified application window.

 @param YuiContext Pointer to the application context.

 @param hWndToActivate The application window to find the button for.
 */
PYUI_TASKBAR_BUTTON
YuiTaskbarFindButtonFromHwndToActivate(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWndToActivate
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_TASKBAR_BUTTON ThisButton;

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (ThisButton->hWndToActivate == hWndToActivate) {
            return ThisButton;
        }
        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    }

    return NULL;
}

/**
 Processes a notification that the resolution of the screen has changed,
 which implies the taskbar is not the same size as previously and buttons may
 need to be moved around.

 @param YuiContext Pointer to the context containing the taskbar buttons to
        move.
 */
VOID
YuiTaskbarNotifyResolutionChange(
    __in PYUI_CONTEXT YuiContext
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    DWORD Index;
    DWORD WidthPerButton;
    PYORI_LIST_ENTRY ListEntry;
    RECT TaskbarWindowClient;
    HWND TaskbarHwnd;

    TaskbarHwnd = YuiContext->hWnd;
    WidthPerButton = YuiTaskbarCalculateButtonWidth(YuiContext, TaskbarHwnd);
    DllUser32.pGetClientRect(TaskbarHwnd, &TaskbarWindowClient);

    ListEntry = NULL;
    Index = 0;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);

        if (ThisButton->hWndButton != NULL) {
            SendMessage(ThisButton->hWndButton, WM_SETFONT, (WPARAM)YuiContext->hFont, MAKELPARAM(TRUE, 0));
            ThisButton->LeftOffset = (WORD)(YuiContext->LeftmostTaskbarOffset + Index * WidthPerButton + 1);
            ThisButton->RightOffset = (WORD)(ThisButton->LeftOffset + WidthPerButton - 2);
            DllUser32.pMoveWindow(ThisButton->hWndButton,
                                  ThisButton->LeftOffset,
                                  1,
                                  WidthPerButton - 2,
                                  TaskbarWindowClient.bottom - 2,
                                  TRUE);
        }
        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
        Index++;
    }
}

/**
 A function invoked to indicate the existence of a new window.

 @param YuiContext Pointer to the application context.

 @param hWnd The new window being created.
 */
VOID
YuiTaskbarNotifyNewWindow(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    DWORD Index;
    DWORD WidthPerButton;
    PYORI_LIST_ENTRY ListEntry;
    RECT TaskbarWindowClient;
    HWND TaskbarHwnd;

    if (!YuiTaskbarIncludeWindow(hWnd)) {
        return;
    }

    YuiTaskbarUpdateFullscreenStatus(YuiContext, hWnd);

    ThisButton = YuiTaskbarFindButtonFromHwndToActivate(YuiContext, hWnd);
    if (ThisButton == NULL) {
        if (!YuiTaskbarAllocateAndAddButton(YuiContext, hWnd)) {
            return;
        }
    }

    TaskbarHwnd = YuiContext->hWnd;
    WidthPerButton = YuiTaskbarCalculateButtonWidth(YuiContext, TaskbarHwnd);
    DllUser32.pGetClientRect(TaskbarHwnd, &TaskbarWindowClient);

    ListEntry = NULL;
    Index = 0;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        ThisButton->LeftOffset = (WORD)(YuiContext->LeftmostTaskbarOffset + Index * WidthPerButton + 1);
        ThisButton->RightOffset = (WORD)(ThisButton->LeftOffset + WidthPerButton - 2);

        if (ThisButton->hWndButton != NULL) {
            DllUser32.pMoveWindow(ThisButton->hWndButton,
                                  ThisButton->LeftOffset,
                                  1,
                                  WidthPerButton - 2,
                                  TaskbarWindowClient.bottom - 2,
                                  TRUE);
        } else {
            ThisButton->ControlId = YuiTaskbarGetNewCtrlId(YuiContext);
            YuiTaskbarCreateButtonControl(YuiContext, ThisButton, TaskbarHwnd, (WORD)(TaskbarWindowClient.bottom - 2));
            if (ThisButton->hWndToActivate == GetForegroundWindow()) {
                YuiTaskbarMarkButtonActive(YuiContext, ThisButton);
            }
        }

        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
        Index++;
    }
}

/**
 A function invoked to indicate that a window is being destroyed.

 @param YuiContext Pointer to the application context.

 @param hWnd The window being destroyed.
 */
VOID
YuiTaskbarNotifyDestroyWindow(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    DWORD Index;
    DWORD WidthPerButton;
    PYORI_LIST_ENTRY ListEntry;
    RECT TaskbarWindowClient;
    HWND TaskbarHwnd;

    if (hWnd == NULL) {
        return;
    }

    ThisButton = YuiTaskbarFindButtonFromHwndToActivate(YuiContext, hWnd);
    if (ThisButton == NULL) {
        return;
    }

    DestroyWindow(ThisButton->hWndButton);
    YoriLibRemoveListItem(&ThisButton->ListEntry);
    YoriLibFreeStringContents(&ThisButton->ButtonText);
    YoriLibDereference(ThisButton);

    ASSERT(YuiContext->TaskbarButtonCount > 0);
    YuiContext->TaskbarButtonCount--;

    TaskbarHwnd = YuiContext->hWnd;
    WidthPerButton = YuiTaskbarCalculateButtonWidth(YuiContext, TaskbarHwnd);
    DllUser32.pGetClientRect(TaskbarHwnd, &TaskbarWindowClient);

    ListEntry = NULL;
    Index = 0;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        ThisButton->LeftOffset = (WORD)(YuiContext->LeftmostTaskbarOffset + Index * WidthPerButton + 1);
        ThisButton->RightOffset = (WORD)(ThisButton->LeftOffset + WidthPerButton - 2);

        if (ThisButton->hWndButton != NULL) {
            DllUser32.pMoveWindow(ThisButton->hWndButton,
                                  ThisButton->LeftOffset,
                                  1,
                                  WidthPerButton - 2,
                                  TaskbarWindowClient.bottom - 2,
                                  TRUE);
        }

        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
        Index++;
    }
}

/**
 A function invoked to indicate that the active window has changed.

 @param YuiContext Pointer to the application context.

 @param hWnd The new window becoming active.
 */
VOID
YuiTaskbarNotifyActivateWindow(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    PYORI_LIST_ENTRY ListEntry;

    if (hWnd == NULL) {
        return;
    }

    YuiTaskbarUpdateFullscreenStatus(YuiContext, hWnd);

    ListEntry = NULL;
    ThisButton = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (ThisButton->hWndToActivate == hWnd) {
            if (!ThisButton->WindowActive) {
                YuiTaskbarMarkButtonActive(YuiContext, ThisButton);
            }
        } else {
            if (ThisButton->WindowActive) {
                YuiTaskbarMarkButtonInactive(YuiContext, ThisButton);
            }
        }
        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
        ThisButton = NULL;
    }
}

/**
 A function invoked to indicate that a window's title is changing.

 @param YuiContext Pointer to the application context.

 @param hWnd The window whose title is being updated.
 */
VOID
YuiTaskbarNotifyTitleChange(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;

    if (hWnd == NULL) {
        return;
    }

    //
    //  If the window has changed in a way that would cause it to be
    //  ineligible for a taskbar button, remove it.
    //

    if (!YuiTaskbarIncludeWindow(hWnd)) {
        YuiTaskbarNotifyDestroyWindow(YuiContext, hWnd);
        return;
    }

    YuiTaskbarUpdateFullscreenStatus(YuiContext, hWnd);

    ThisButton = YuiTaskbarFindButtonFromHwndToActivate(YuiContext, hWnd);
    if (ThisButton == NULL) {

        //
        //  If no button is found, check if one should be created.  This can
        //  happen if the title was initially empty and changes to contain a
        //  string.  Once a window ever contained a string, it is retained
        //  even if the title is removed.
        //

        YuiTaskbarNotifyNewWindow(YuiContext, hWnd);
    } else {
        YORI_STRING NewTitle;
        YoriLibInitEmptyString(&NewTitle);
        if (YoriLibAllocateString(&NewTitle, GetWindowTextLength(hWnd) + 1)) {
            NewTitle.LengthInChars = GetWindowText(hWnd, NewTitle.StartOfString, NewTitle.LengthAllocated);
            YoriLibFreeStringContents(&ThisButton->ButtonText);
            memcpy(&ThisButton->ButtonText, &NewTitle, sizeof(YORI_STRING));
            RedrawWindow(ThisButton->hWndButton, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
        }
    }
}

/**
 A function invoked to indicate that a window is flashing.

 @param YuiContext Pointer to the application context.

 @param hWnd The window which is flashing.
 */
VOID
YuiTaskbarNotifyFlash(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;

    if (hWnd == NULL) {
        return;
    }

    ThisButton = YuiTaskbarFindButtonFromHwndToActivate(YuiContext, hWnd);
    if (ThisButton == NULL) {
        return;
    }

    //
    //  If it's not active and not flashing, mark it as flashing and redraw.
    //

    if (!ThisButton->WindowActive && !ThisButton->Flashing) {
        ThisButton->Flashing = TRUE;
        RedrawWindow(ThisButton->hWndButton, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
    }
}

/**
 A callback function that is invoked when initially syncing the taskbar with
 the current state of taskbar buttons.

 @param hWnd The window that currently exists.

 @param lParam Pointer to the application context.

 @return TRUE to continue enumerating windows.
 */
BOOL WINAPI
YuiTaskbarSyncWindowCallback(
    __in HWND hWnd,
    __in LPARAM lParam
    )
{
    PYUI_CONTEXT YuiContext;
    PYUI_TASKBAR_BUTTON ThisButton;

    YuiContext = (PYUI_CONTEXT)lParam;

    if (!YuiTaskbarIncludeWindow(hWnd)) {
        return TRUE;
    }

    ThisButton = YuiTaskbarFindButtonFromHwndToActivate(YuiContext, hWnd);
    if (ThisButton == NULL) {

        // 
        //  If it doesn't have a button, go ahead and create a new one.
        //

        YuiTaskbarNotifyNewWindow(YuiContext, hWnd);
    } else {
        ThisButton->AssociatedWindowFound = TRUE;
        if ((DWORD)GetWindowTextLength(ThisButton->hWndToActivate) != ThisButton->ButtonText.LengthInChars) {
            YuiTaskbarNotifyTitleChange(YuiContext, hWnd);
        }
    }

    return TRUE;
}

/**
 Enumerate all current windows and update the taskbar with any changes.
 Also activates the taskbar button corresponding to the currently active
 window.  This is degenerate fallback code that executes on systems incapable
 of providing real time window notifications.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiTaskbarSyncWithCurrent(
    __in PYUI_CONTEXT YuiContext
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    PYORI_LIST_ENTRY ListEntry;

    //
    //  Enumerate the set of buttons and indicate that none of them have been
    //  confirmed to exist with any currently open window.
    //

    ListEntry = NULL;
    ThisButton = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        ThisButton->AssociatedWindowFound = FALSE;
        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
        ThisButton = NULL;
    }

    //
    //  Enumerate the set of currently open windows.  If this finds a window
    //  that's currently on the taskbar, mark it as existing.
    //

    EnumWindows(YuiTaskbarSyncWindowCallback, (LPARAM)YuiContext);

    //
    //  Enumerate the set of windows that are on the taskbar, and if any have
    //  not been found in the most recent enumerate, tear them down.
    //

    ListEntry = NULL;
    ThisButton = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
        if (!ThisButton->AssociatedWindowFound) {
            YuiTaskbarNotifyDestroyWindow(YuiContext, ThisButton->hWndToActivate);
        }
        ThisButton = NULL;
    }

    //
    //  Indicate the currently active window has become active.
    //

    YuiTaskbarNotifyActivateWindow(YuiContext, GetForegroundWindow());
}

/**
 Free all button structures and destroy button windows in preparation for
 exiting the application.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiTaskbarFreeButtons(
    __in PYUI_CONTEXT YuiContext
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIST_ENTRY NextEntry;
    PYUI_TASKBAR_BUTTON ThisButton;

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        NextEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (ThisButton->hWndButton != NULL) {
            DestroyWindow(ThisButton->hWndButton);
            ThisButton->hWndButton = NULL;
        }
        YoriLibRemoveListItem(&ThisButton->ListEntry);
        YoriLibFreeStringContents(&ThisButton->ButtonText);
        YoriLibDereference(ThisButton);
        ListEntry = NextEntry;
    }
}

/**
 Switch to the application described by a taskbar button.

 @param YuiContext Pointer to the application context.

 @param ThisButton Pointer to the taskbar button description.
 */
VOID
YuiTaskbarSwitchToButton(
    __in PYUI_CONTEXT YuiContext,
    __in PYUI_TASKBAR_BUTTON ThisButton
    )
{
    if (IsIconic(ThisButton->hWndToActivate)) {
        if (DllUser32.pShowWindowAsync != NULL) {
            DllUser32.pShowWindowAsync(ThisButton->hWndToActivate, SW_RESTORE);
        } else {
            DllUser32.pShowWindow(ThisButton->hWndToActivate, SW_RESTORE);
        }
    }
    DllUser32.pSetForegroundWindow(ThisButton->hWndToActivate);
    SetFocus(ThisButton->hWndToActivate);

    //
    //  If the taskbar is polling, force an update now without
    //  waiting for the poll.  If it's driven by events, don't
    //  update now, and handle it as part of the window activation
    //  notification (so it's only repainted once.)
    //

    if (YuiContext->TaskbarRefreshFrequency != 0) {
        YuiTaskbarNotifyActivateWindow(YuiContext, ThisButton->hWndToActivate);
    }
}

/**
 Switch to the window associated with the specified control identifier.

 @param YuiContext Pointer to the application context.

 @param CtrlId The identifier of the button that was pressed indicating a
        desire to change to the associated window.
 */
VOID
YuiTaskbarSwitchToTask(
    __in PYUI_CONTEXT YuiContext,
    __in DWORD CtrlId
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;

    ThisButton = YuiTaskbarFindButtonFromCtrlId(YuiContext, CtrlId);
    if (ThisButton != NULL) {
        YuiTaskbarSwitchToButton(YuiContext, ThisButton);
    }
}

/**
 If a taskbar button is currently pressed, switch to that window.  This is
 used after an action which would leave the taskbar with input focus.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiTaskbarSwitchToActiveTask(
    __in PYUI_CONTEXT YuiContext
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_TASKBAR_BUTTON ThisButton;

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (ThisButton->WindowActive) {
            YuiTaskbarSwitchToButton(YuiContext, ThisButton);
            break;
        }
        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    }
}

/**
 Draw a taskbar button.

 @param YuiContext Pointer to the application context.

 @param CtrlId The identifier of the button that should be redrawn.

 @param DrawItemStruct Pointer to a structure describing the button to
        redraw, including its bounds and device context.
 */
VOID
YuiTaskbarDrawButton(
    __in PYUI_CONTEXT YuiContext,
    __in DWORD CtrlId,
    __in PDRAWITEMSTRUCT DrawItemStruct
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    HICON Icon;

    ThisButton = YuiTaskbarFindButtonFromCtrlId(YuiContext, CtrlId);
    if (ThisButton == NULL) {
        return;
    }

    Icon = (HICON)GetClassLongPtr(ThisButton->hWndToActivate, GCLP_HICONSM);
    YuiDrawButton(DrawItemStruct, ThisButton->WindowActive, ThisButton->Flashing, Icon, &ThisButton->ButtonText, FALSE);
}

/**
 Find a taskbar button based on the horizontal coordinate relative to the
 client area.  This is used to activate buttons if the user clicks outside
 of the button area.

 @param YuiContext Pointer to the application context.

 @param XPos The horizontal coordinate, relative to the client area.

 */
WORD
YuiTaskbarFindByOffset(
    __in PYUI_CONTEXT YuiContext,
    __in SHORT XPos
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = NULL;
    ThisButton = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (XPos >= ThisButton->LeftOffset && XPos <= ThisButton->RightOffset) {
            return ThisButton->ControlId;
        }
        ListEntry = YoriLibGetNextListEntry(&YuiContext->TaskbarButtons, ListEntry);
        ThisButton = NULL;
    }
    return 0;
}

// vim:sw=4:ts=4:et:
