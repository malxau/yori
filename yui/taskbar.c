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
    YORI_ALLOC_SIZE_T WindowTitleLength;
    YORI_STRING ClassName;
    YORI_STRING ExcludePrefix;
    TCHAR ClassNameBuffer[64];

    if (!IsWindowVisible(hWnd)) {
        return FALSE;
    }

    //
    //  This used to filter based on IsWindowEnabled.  That still seems to be
    //  an approximately correct thing to do; unfortunately VirtualBox (based
    //  on QT5) likes to create disabled windows then enable them, and we
    //  don't get notification about the enable.  Sigh.
    //

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

    WindowTitleLength = (YORI_ALLOC_SIZE_T)GetWindowTextLength(hWnd);
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

    ClassName.LengthInChars = (YORI_ALLOC_SIZE_T)GetClassName(hWnd, ClassName.StartOfString, ClassName.LengthAllocated);

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

    return TRUE;
}

/**
 Check if a window is full screen.

 @param YuiMonitor Pointer to the monitor context.

 @param hWnd Handle to the window to check.

 @return TRUE if the window appears full screen, FALSE if not.
 */
BOOLEAN
YuiTaskbarFullscreenWindow(
    __in PYUI_MONITOR YuiMonitor,
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

        if (WindowRect.left > (INT)(YuiMonitor->ScreenLeft + 2) ||
            WindowRect.top > (INT)(YuiMonitor->ScreenTop + 2) ||
            WindowRect.bottom < (INT)(YuiMonitor->ScreenTop + YuiMonitor->ScreenHeight - 2) ||
            WindowRect.right < (INT)(YuiMonitor->ScreenLeft + YuiMonitor->ScreenWidth - 2)) {

            return FALSE;
        }

        //
        //  If the window is maximized, it implies that the work area is no
        //  longer excluding the task bar.  Trigger a work area update to 
        //  recalculate the work area, and then re-query the current window
        //  location to see if it's still full screen.
        //

        if (RetryCount == 1 && IsZoomed(hWnd)) {
#if DBG
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Fullscreen window is maximized, resetting work area\n"));
#endif
            YuiResetWorkArea(YuiMonitor, TRUE);
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

 @param YuiMonitor Pointer to the monitorcontext.

 @param hWnd Handle to the currently active window.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YuiTaskbarUpdateFullscreenStatus(
    __in PYUI_MONITOR YuiMonitor,
    __in HWND hWnd
    )
{
    BOOLEAN FullscreenWindowActive;

    FullscreenWindowActive = YuiTaskbarFullscreenWindow(YuiMonitor, hWnd);

    if (YuiMonitor->FullscreenModeActive) {
        if (!FullscreenWindowActive) {
#if DBG
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Window %08x not fullscreen, displaying taskbar\n"), hWnd);
#endif
            DllUser32.pShowWindow(YuiMonitor->hWndTaskbar, SW_SHOW);
            YuiMonitor->FullscreenModeActive = FullscreenWindowActive;
        }
    } else if (FullscreenWindowActive) {
#if DBG
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Window %08x fullscreen, hiding taskbar\n"), hWnd);
#endif
        DllUser32.pShowWindow(YuiMonitor->hWndTaskbar, SW_HIDE);
        YuiMonitor->FullscreenModeActive = FullscreenWindowActive;
    }

    return FullscreenWindowActive;
}

/**
 Temporarily display the taskbar, even if a full screen window is active.
 This is to allow hotkeys to render the taskbar when the start menu is being
 displayed.

 @param YuiMonitor Pointer to the monitor context.

 @return TRUE if a full screen mode was disabled; FALSE if no action was
         taken.
 */
BOOLEAN
YuiTaskbarSuppressFullscreenHiding(
    __in PYUI_MONITOR YuiMonitor
    )
{
    if (YuiMonitor->FullscreenModeActive) {
        DllUser32.pShowWindow(YuiMonitor->hWndTaskbar, SW_SHOW);
        YuiMonitor->FullscreenModeActive = FALSE;
        return TRUE;
    }

    return FALSE;
}

/**
 Allocate a unique identifier for a button control that will be displayed
 on the taskbar.

 @param YuiMonitor Pointer to the monitor context.

 @return The identifier for the button control.
 */
WORD
YuiTaskbarGetNewCtrlId(
    __in PYUI_MONITOR YuiMonitor
    )
{
    YuiMonitor->NextTaskbarId++;
    return YuiMonitor->NextTaskbarId;
}

/**
 Update the button text to contain characters that can be displayed by common
 system fonts.

 @param Button Pointer to the button containing populated button text.
 */
VOID
YuiTaskbarMungeButtonText(
    __in PYUI_TASKBAR_BUTTON Button
    )
{
    YORI_ALLOC_SIZE_T Index;

    for (Index = 0; Index < Button->ButtonText.LengthInChars; Index++) {

        //
        //  Replace EM-dash with a dash that all fonts have
        //

        if (Button->ButtonText.StartOfString[Index] == 0x2014) {
            Button->ButtonText.StartOfString[Index] = '-';
        }
    }
}

/**
 Allocate memory for the structure that describes a taskbar button.  Note
 this does not create the button control itself.

 @param YuiMonitor Pointer to the monitor context.

 @param hWnd The window handle to the taskbar window.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiTaskbarAllocateAndAddButton(
    __in PYUI_MONITOR YuiMonitor,
    __in HWND hWnd
    )
{
    PYUI_TASKBAR_BUTTON NewButton;
    YORI_ALLOC_SIZE_T WindowTitleLength;
    PYORI_LIST_ENTRY ListEntry;
    PYUI_RECENT_CHILD_PROCESS ChildProcess;
    LONGLONG CurrentTime;
    LONGLONG ExpireTime;
    PYUI_CONTEXT YuiContext;

    YuiContext = YuiMonitor->YuiContext;

    WindowTitleLength = (YORI_ALLOC_SIZE_T)GetWindowTextLength(hWnd);

    NewButton = YoriLibReferencedMalloc(sizeof(YUI_TASKBAR_BUTTON) + (WindowTitleLength + 1) * sizeof(TCHAR));
    if (NewButton == NULL) {
        return FALSE;
    }

    YoriLibInitEmptyString(&NewButton->ShortcutPath);

    YoriLibInitEmptyString(&NewButton->ButtonText);
    NewButton->ButtonText.StartOfString = (LPTSTR)(NewButton + 1);
    NewButton->ButtonText.LengthAllocated = WindowTitleLength + 1;
    YoriLibReference(NewButton);
    NewButton->ButtonText.MemoryToFree = NewButton;
    NewButton->ButtonText.LengthInChars = (YORI_ALLOC_SIZE_T)
        GetWindowText(hWnd, NewButton->ButtonText.StartOfString, NewButton->ButtonText.LengthAllocated);
    YuiTaskbarMungeButtonText(NewButton);

    NewButton->ChildProcess = NULL;
    NewButton->hWndToActivate = hWnd;
    NewButton->hWndButton = NULL;
    NewButton->WindowActive = FALSE;
    NewButton->AssociatedWindowFound = TRUE;
    NewButton->Flashing = FALSE;
    NewButton->ProcessId = 0;

    CurrentTime = YoriLibGetSystemTimeAsInteger();
    ExpireTime = CurrentTime - (10 * 1000 * 1000 * 30);
    GetWindowThreadProcessId(hWnd, &NewButton->ProcessId);
    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiContext->RecentProcessList, ListEntry);
    while (ListEntry != NULL) {
        ChildProcess = CONTAINING_RECORD(ListEntry, YUI_RECENT_CHILD_PROCESS, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&YuiContext->RecentProcessList, ListEntry);

        //
        //  Note that new processes are inserted into the front of the list,
        //  so there's a chance of a duplicate process ID in the list, where
        //  the first is current and the second is stale.
        //

        if (ChildProcess->ProcessId == NewButton->ProcessId &&
            NewButton->ChildProcess == NULL) {

            ChildProcess->TaskbarButtonCount++;
            YoriLibReference(ChildProcess);
            NewButton->ChildProcess = ChildProcess;
            YoriLibCloneString(&NewButton->ShortcutPath, &ChildProcess->FilePath);
#if DBG
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Associating shortcut with button %x %y\n"), NewButton->ProcessId, &ChildProcess->FilePath);
#endif

        //
        //  If the process has had no windows for the last minute, remove it
        //  from this list, since there's a good chance any newly created
        //  window for the same process ID refers to a different process.
        //

        } else if (ChildProcess->TaskbarButtonCount == 0 &&
                   ChildProcess->LastModifiedTime < ExpireTime) {

#if DBG
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Removing expired process %x %y\n"), ChildProcess->ProcessId, &ChildProcess->FilePath);
#endif
            YoriLibRemoveListItem(&ChildProcess->ListEntry);
            YoriLibFreeStringContents(&ChildProcess->FilePath);
            YoriLibDereference(ChildProcess);
            YuiContext->RecentProcessCount--;
        }
    }

    YoriLibAppendList(&YuiMonitor->TaskbarButtons, &NewButton->ListEntry);
    YuiMonitor->TaskbarButtonCount++;

    return TRUE;
}

/**
 Free a taskbar button structure.

 @param Button Pointer to the button structure to free.
 */
VOID
YuiTaskbarFreeButton(
    __in PYUI_TASKBAR_BUTTON Button
    )
{
    ASSERT(Button->hWndButton == NULL);
    YoriLibRemoveListItem(&Button->ListEntry);
    YoriLibFreeStringContents(&Button->ShortcutPath);
    if (Button->ChildProcess != NULL) {
        PYUI_RECENT_CHILD_PROCESS ChildProcess;
        ChildProcess = Button->ChildProcess;
#if DBG
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Removing association with button %x %y\n"), ChildProcess->ProcessId, &ChildProcess->FilePath);
#endif
        ChildProcess->TaskbarButtonCount--;
        if (ChildProcess->TaskbarButtonCount == 0) {
            ChildProcess->LastModifiedTime = YoriLibGetSystemTimeAsInteger();
        }
        YoriLibDereference(ChildProcess);
        Button->ChildProcess = NULL;
    }
    YoriLibFreeStringContents(&Button->ButtonText);
    YoriLibDereference(Button);
}

/**
 Create a button window for the specified button.

 @param YuiMonitor Pointer to the monitor context.

 @param ThisButton Pointer to the button structure indicating its text,
        location, and control ID.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiTaskbarCreateButtonControl(
    __in PYUI_MONITOR YuiMonitor,
    __in PYUI_TASKBAR_BUTTON ThisButton
    )
{
    ThisButton->hWndButton = CreateWindow(_T("BUTTON"),
                                          _T(""),
                                          BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                                          ThisButton->LeftOffset,
                                          ThisButton->TopOffset,
                                          (WORD)(ThisButton->RightOffset - ThisButton->LeftOffset),
                                          (WORD)(ThisButton->BottomOffset - ThisButton->TopOffset),
                                          YuiMonitor->hWndTaskbar,
                                          (HMENU)(DWORD_PTR)ThisButton->ControlId,
                                          NULL,
                                          NULL);

    if (ThisButton->hWndButton == NULL) {
        return FALSE;
    }

    SetWindowLongPtr(ThisButton->hWndButton, GWLP_WNDPROC, (LONG_PTR)YuiTaskbarButtonWndProc);
    SendMessage(ThisButton->hWndButton, WM_SETFONT, (WPARAM)YuiMonitor->hFont, MAKELPARAM(TRUE, 0));

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
    PYUI_MONITOR YuiMonitor;

    YuiContext = (PYUI_CONTEXT)lParam;
    if (!YuiTaskbarIncludeWindow(hWnd)) {
        return TRUE;
    }
    YuiMonitor = YuiMonitorFromApplicationHwnd(YuiContext, hWnd);

#if DBG
    {
        YORI_ALLOC_SIZE_T CharsNeeded;
        YORI_STRING WindowTitle;

        CharsNeeded = (YORI_ALLOC_SIZE_T)GetWindowTextLength(hWnd);
        if (YoriLibAllocateString(&WindowTitle, CharsNeeded + 1)) {
            WindowTitle.LengthInChars = (YORI_ALLOC_SIZE_T)GetWindowText(hWnd, WindowTitle.StartOfString, WindowTitle.LengthAllocated);
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                      _T("Enumerated window %08x on %08x %y\n"),
                      hWnd,
                      YuiMonitor->MonitorHandle,
                      &WindowTitle);
        YoriLibFreeStringContents(&WindowTitle);
    }
#endif

    YuiTaskbarAllocateAndAddButton(YuiMonitor, hWnd);

    return TRUE;
}

/**
 Mark the button as hosting the active window.  This updates internal state,
 sets the font, and updates button to the pressed appearance.

 @param YuiMonitor Pointer to the monitor context.

 @param ThisButton Pointer to the button to mark as active.
 */
VOID
YuiTaskbarMarkButtonActive(
    __in PYUI_MONITOR YuiMonitor,
    __in PYUI_TASKBAR_BUTTON ThisButton
    )
{
    ThisButton->Flashing = FALSE;
    ThisButton->WindowActive = TRUE;
    SendMessage(ThisButton->hWndButton, WM_SETFONT, (WPARAM)YuiMonitor->hBoldFont, MAKELPARAM(FALSE, 0));
    SendMessage(ThisButton->hWndButton, BM_SETSTATE, TRUE, 0);
}

/**
 Mark the button as not hosting the active window.  This updates internal
 state, sets the font, and updates button to the raised appearance.

 @param YuiMonitor Pointer to the monitor context.

 @param ThisButton Pointer to the button to mark as inactive.
 */
VOID
YuiTaskbarMarkButtonInactive(
    __in PYUI_MONITOR YuiMonitor,
    __in PYUI_TASKBAR_BUTTON ThisButton
    )
{
    ThisButton->WindowActive = FALSE;
    SendMessage(ThisButton->hWndButton, WM_SETFONT, (WPARAM)YuiMonitor->hFont, MAKELPARAM(FALSE, 0));
    SendMessage(ThisButton->hWndButton, BM_SETSTATE, FALSE, 0);
}

/**
 Calculate the width for every taskbar button.  Each button has the same
 width.  The width is going to be the size of the taskbar divided by the
 number of buttons, with a maximum size per button to prevent a single
 window occupying the entire bar, etc.

 @param YuiMonitor Pointer to the monitor context.

 @return The number of pixels in each taskbar button.
 */
DWORD
YuiTaskbarCalculateButtonWidth(
    __in PYUI_MONITOR YuiMonitor
    )
{
    RECT TaskbarWindowClient;
    DWORD TotalWidthForButtons;
    DWORD WidthPerButton;

    DllUser32.pGetClientRect(YuiMonitor->hWndTaskbar, &TaskbarWindowClient);
    TotalWidthForButtons = TaskbarWindowClient.right -
                           YuiMonitor->LeftmostTaskbarOffset -
                           YuiMonitor->RightmostTaskbarOffset - 1;

    if (YuiMonitor->TaskbarButtonCount == 0) {
        WidthPerButton = YuiMonitor->MaximumTaskbarButtonWidth;
    } else {
        WidthPerButton = TotalWidthForButtons / YuiMonitor->TaskbarButtonCount;
        if (WidthPerButton > YuiMonitor->MaximumTaskbarButtonWidth) {
            WidthPerButton = YuiMonitor->MaximumTaskbarButtonWidth;
        }
    }

    return WidthPerButton;
}

/**
 Move existing buttons on the taskbar.  This attempts to use DeferWindowPos
 in the hope that Windows can do this efficiently.

 @param YuiMonitor Pointer to the monitor context.
 */
VOID
YuiTaskbarRepositionExistingButtons(
    __in PYUI_MONITOR YuiMonitor
    )
{
    DWORD WidthPerButton;
    RECT TaskbarWindowClient;
    DWORD Index;
    PYORI_LIST_ENTRY ListEntry;
    PYUI_TASKBAR_BUTTON ThisButton;
    HDWP WindowPos;
    HDWP Result;
    WORD NewLeftOffset;
    WORD NewRightOffset;
    WORD NewTopOffset;
    WORD NewBottomOffset;

    Index = 0;
    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        Index++;
    }

    WindowPos = BeginDeferWindowPos(Index);
    if (WindowPos == NULL) {
        return;
    }

    WidthPerButton = YuiTaskbarCalculateButtonWidth(YuiMonitor);

    DllUser32.pGetClientRect(YuiMonitor->hWndTaskbar, &TaskbarWindowClient);

    Index = 0;
    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);

        //
        //  It's possible no button window exists if it's still in the
        //  process of being created.  When this happens, this function should
        //  move all of the other windows, allowing the new one to be created
        //  in available space.
        //

        if (ThisButton->hWndButton != NULL) {

            //
            //  Check if the button is actually moving.  If the taskbar is not
            //  full, new buttons arriving will not move buttons; existing
            //  buttons being removed only move buttons that follow, etc.
            //

            NewLeftOffset = (WORD)(YuiMonitor->LeftmostTaskbarOffset + Index * WidthPerButton + YuiMonitor->ControlBorderWidth);
            NewRightOffset = (WORD)(NewLeftOffset + WidthPerButton - 2 * YuiMonitor->ControlBorderWidth);
            NewTopOffset = YuiMonitor->TaskbarPaddingVertical;
            NewBottomOffset = (WORD)(TaskbarWindowClient.bottom - YuiMonitor->TaskbarPaddingVertical);

            if (NewLeftOffset != ThisButton->LeftOffset ||
                NewRightOffset != ThisButton->RightOffset ||
                NewTopOffset != ThisButton->TopOffset ||
                NewBottomOffset != ThisButton->BottomOffset) {

                ThisButton->LeftOffset = NewLeftOffset;
                ThisButton->RightOffset = NewRightOffset;
                ThisButton->TopOffset = NewTopOffset;
                ThisButton->BottomOffset = NewBottomOffset;
                Result = DeferWindowPos(WindowPos,
                                        ThisButton->hWndButton,
                                        NULL,
                                        ThisButton->LeftOffset,
                                        ThisButton->TopOffset,
                                        WidthPerButton - 2 * YuiMonitor->ControlBorderWidth,
                                        ThisButton->BottomOffset - ThisButton->TopOffset,
                                        SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);

                if (Result == NULL) {

                    //
                    //  Per MSDN, we should "abandon" this and not call
                    //  EndDeferWindowPos.  I hope that means it was already freed
                    //  as part of the above failure...?
                    //

                    return;
                }
            }
        }

        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        Index++;
    }

    EndDeferWindowPos(WindowPos);
}

/**
 Populate the taskbar with the set of windows that exist at the time the
 taskbar was created.

 @param YuiContext Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiTaskbarPopulateWindows(
    __in PYUI_CONTEXT YuiContext
    )
{
    DWORD WidthPerButton;
    DWORD Index;
    PYORI_LIST_ENTRY ListEntry;
    PYUI_TASKBAR_BUTTON ThisButton;
    HWND ActiveWindow;
    RECT TaskbarWindowClient;
    PYUI_MONITOR YuiMonitor;

    EnumWindows(YuiTaskbarWindowCallback, (LPARAM)YuiContext);

    ActiveWindow = GetForegroundWindow();

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {

        WidthPerButton = YuiTaskbarCalculateButtonWidth(YuiMonitor);
    
        DllUser32.pGetClientRect(YuiMonitor->hWndTaskbar, &TaskbarWindowClient);
    
        ListEntry = NULL;
        Index = 0;
        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        while (ListEntry != NULL) {
            ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
    
            ThisButton->ControlId = YuiTaskbarGetNewCtrlId(YuiMonitor);
            ThisButton->LeftOffset = (WORD)(YuiMonitor->LeftmostTaskbarOffset + Index * WidthPerButton + 1),
            ThisButton->RightOffset = (WORD)(ThisButton->LeftOffset + WidthPerButton - 2),
            ThisButton->TopOffset = YuiMonitor->TaskbarPaddingVertical;
            ThisButton->BottomOffset = (WORD)(TaskbarWindowClient.bottom - YuiMonitor->TaskbarPaddingVertical);
            YuiTaskbarCreateButtonControl(YuiMonitor, ThisButton);
            if (ThisButton->hWndToActivate == ActiveWindow) {
                YuiTaskbarMarkButtonActive(YuiMonitor, ThisButton);
            }
            ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
            Index++;
        }
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    }

    return TRUE;
}

/**
 Find a button structure from a specified control ID.

 @param YuiMonitor Pointer to the monitor context.

 @param CtrlId The control ID to find.
 */
PYUI_TASKBAR_BUTTON
YuiTaskbarFindButtonFromCtrlId(
    __in PYUI_MONITOR YuiMonitor,
    __in DWORD CtrlId
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_TASKBAR_BUTTON ThisButton;

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (ThisButton->ControlId == CtrlId) {
            return ThisButton;
        }
        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
    }

    return NULL;
}

/**
 Find a button structure from a specified application window.

 @param YuiMonitor Pointer to the monitor context.

 @param hWndToActivate The application window to find the button for.
 */
PYUI_TASKBAR_BUTTON
YuiTaskbarFindButtonFromHwndToActivate(
    __in PYUI_MONITOR YuiMonitor,
    __in HWND hWndToActivate
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_TASKBAR_BUTTON ThisButton;

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (ThisButton->hWndToActivate == hWndToActivate) {
            return ThisButton;
        }
        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
    }

    return NULL;
}

/**
 Processes a notification that the resolution of the screen has changed,
 which implies the taskbar is not the same size as previously and buttons may
 need to be moved around.

 @param YuiMonitor Pointer to the context containing the taskbar buttons to
        move.
 */
VOID
YuiTaskbarNotifyResolutionChange(
    __in PYUI_MONITOR YuiMonitor
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    DWORD Index;
    DWORD WidthPerButton;
    PYORI_LIST_ENTRY ListEntry;
    RECT TaskbarWindowClient;

    WidthPerButton = YuiTaskbarCalculateButtonWidth(YuiMonitor);
    DllUser32.pGetClientRect(YuiMonitor->hWndTaskbar, &TaskbarWindowClient);

    ListEntry = NULL;
    Index = 0;
    ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);

        if (ThisButton->hWndButton != NULL) {
            if (ThisButton->WindowActive) {
                SendMessage(ThisButton->hWndButton, WM_SETFONT, (WPARAM)YuiMonitor->hBoldFont, MAKELPARAM(TRUE, 0));
            } else {
                SendMessage(ThisButton->hWndButton, WM_SETFONT, (WPARAM)YuiMonitor->hFont, MAKELPARAM(TRUE, 0));
            }
        }
        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        Index++;
    }

    YuiTaskbarRepositionExistingButtons(YuiMonitor);
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
    PYUI_MONITOR YuiMonitor;

    if (!YuiTaskbarIncludeWindow(hWnd)) {
        return;
    }

    YuiMonitor = YuiMonitorFromApplicationHwnd(YuiContext, hWnd);
    YuiTaskbarUpdateFullscreenStatus(YuiMonitor, hWnd);

    ThisButton = YuiTaskbarFindButtonFromHwndToActivate(YuiMonitor, hWnd);
    if (ThisButton == NULL) {
        if (!YuiTaskbarAllocateAndAddButton(YuiMonitor, hWnd)) {
            return;
        }
    }

    YuiTaskbarRepositionExistingButtons(YuiMonitor);
    WidthPerButton = YuiTaskbarCalculateButtonWidth(YuiMonitor);
    DllUser32.pGetClientRect(YuiMonitor->hWndTaskbar, &TaskbarWindowClient);

    ListEntry = NULL;
    Index = 0;
    ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (ThisButton->hWndButton == NULL) {
            ThisButton->LeftOffset = (WORD)(YuiMonitor->LeftmostTaskbarOffset + Index * WidthPerButton + 1);
            ThisButton->RightOffset = (WORD)(ThisButton->LeftOffset + WidthPerButton - 2);
            ThisButton->TopOffset = YuiMonitor->TaskbarPaddingVertical;
            ThisButton->BottomOffset = (WORD)(TaskbarWindowClient.bottom - YuiMonitor->TaskbarPaddingVertical);

            ThisButton->ControlId = YuiTaskbarGetNewCtrlId(YuiMonitor);
            YuiTaskbarCreateButtonControl(YuiMonitor, ThisButton);
            if (ThisButton->hWndToActivate == GetForegroundWindow()) {
                YuiTaskbarMarkButtonActive(YuiMonitor, ThisButton);
            }
        }

        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
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
    PYUI_MONITOR YuiMonitor;

    if (hWnd == NULL) {
        return;
    }

    //
    //  Rather than ask where the window is (was?), clean up all monitors
    //  that might have heard of it.
    //

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
        ThisButton = YuiTaskbarFindButtonFromHwndToActivate(YuiMonitor, hWnd);
        if (ThisButton != NULL) {

            ASSERT(ThisButton->hWndButton != NULL);
            __analysis_assume(ThisButton->hWndButton != NULL);
            DestroyWindow(ThisButton->hWndButton);
            ThisButton->hWndButton = NULL;
            YuiTaskbarFreeButton(ThisButton);
        
            ASSERT(YuiMonitor->TaskbarButtonCount > 0);
            YuiMonitor->TaskbarButtonCount--;
        
            YuiTaskbarRepositionExistingButtons(YuiMonitor);
        }
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
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
    PYUI_MONITOR YuiMonitor;

    if (hWnd == NULL) {
        return;
    }

    //
    //  MSFIX Really want to ensure we've called UpdateFullscreenStatus on
    //  the old window before the new one.  We don't currently know the old
    //  one without scanning first though.
    //

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
    
        ListEntry = NULL;
        ThisButton = NULL;
        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        while (ListEntry != NULL) {
            ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
            if (ThisButton->hWndToActivate == hWnd) {
                if (!ThisButton->WindowActive) {
                    YuiTaskbarMarkButtonActive(YuiMonitor, ThisButton);
                    YuiTaskbarUpdateFullscreenStatus(YuiMonitor, hWnd);
                }
            } else {
                if (ThisButton->WindowActive) {
                    YuiTaskbarMarkButtonInactive(YuiMonitor, ThisButton);
                    YuiTaskbarUpdateFullscreenStatus(YuiMonitor, hWnd);
                }
            }
            ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
            ThisButton = NULL;
        }
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
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
    PYUI_MONITOR YuiMonitor;

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

    YuiMonitor = YuiMonitorFromApplicationHwnd(YuiContext, hWnd);
    YuiTaskbarUpdateFullscreenStatus(YuiMonitor, hWnd);

    ThisButton = YuiTaskbarFindButtonFromHwndToActivate(YuiMonitor, hWnd);
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
        if (YoriLibAllocateString(&NewTitle, (YORI_ALLOC_SIZE_T)GetWindowTextLength(hWnd) + 1)) {
            NewTitle.LengthInChars = (YORI_ALLOC_SIZE_T)GetWindowText(hWnd, NewTitle.StartOfString, NewTitle.LengthAllocated);
            YoriLibFreeStringContents(&ThisButton->ButtonText);
            memcpy(&ThisButton->ButtonText, &NewTitle, sizeof(YORI_STRING));
            YuiTaskbarMungeButtonText(ThisButton);
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
    PYUI_MONITOR YuiMonitor;

    if (hWnd == NULL) {
        return;
    }

    YuiMonitor = YuiMonitorFromApplicationHwnd(YuiContext, hWnd);

    ThisButton = YuiTaskbarFindButtonFromHwndToActivate(YuiMonitor, hWnd);
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
 A function invoked to indicate that a window has changed monitors.

 @param YuiContext Pointer to the application context.

 @param hWnd The window which has moved to a new monitor.
 */
VOID
YuiTaskbarNotifyMonitorChanged(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    YuiTaskbarNotifyDestroyWindow(YuiContext, hWnd);
    YuiTaskbarNotifyNewWindow(YuiContext, hWnd);
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
    PYUI_MONITOR YuiMonitor;
    PYUI_TASKBAR_BUTTON ThisButton;

    YuiContext = (PYUI_CONTEXT)lParam;

    if (!YuiTaskbarIncludeWindow(hWnd)) {
        return TRUE;
    }

    //
    //  MSFIX Might want lower level helper functions since we've already
    //  determined the monitor and the child functions here will do it
    //  again
    //

    YuiMonitor = YuiMonitorFromApplicationHwnd(YuiContext, hWnd);
    ThisButton = YuiTaskbarFindButtonFromHwndToActivate(YuiMonitor, hWnd);
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
    PYUI_MONITOR YuiMonitor;

    //
    //  Enumerate the set of buttons and indicate that none of them have been
    //  confirmed to exist with any currently open window.
    //

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
        ListEntry = NULL;
        ThisButton = NULL;
        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        while (ListEntry != NULL) {
            ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
            ThisButton->AssociatedWindowFound = FALSE;
            ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
            ThisButton = NULL;
        }
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
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
    //  MSFIX Do buttons need to identify their owning taskbar?
    //

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
        ListEntry = NULL;
        ThisButton = NULL;
        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        while (ListEntry != NULL) {
            ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
            ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
            if (!ThisButton->AssociatedWindowFound) {
                YuiTaskbarNotifyDestroyWindow(YuiContext, ThisButton->hWndToActivate);
            }
            ThisButton = NULL;
        }
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    }

    //
    //  Indicate the currently active window has become active.
    //

    YuiTaskbarNotifyActivateWindow(YuiContext, GetForegroundWindow());
}

/**
 Free all taskbar buttons attached to a single monitor.  This is used when
 cleaning up all monitors, or where a specific monitor is being removed.

 @param YuiMonitor Pointer to the monitor to clean up.
 */
VOID
YuiTaskbarFreeButtonsOneMonitor(
    __in PYUI_MONITOR YuiMonitor
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIST_ENTRY NextEntry;
    PYUI_TASKBAR_BUTTON ThisButton;

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        NextEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (ThisButton->hWndButton != NULL) {
            DestroyWindow(ThisButton->hWndButton);
            ThisButton->hWndButton = NULL;
        }
        YuiTaskbarFreeButton(ThisButton);
        ListEntry = NextEntry;
    }
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
    PYUI_MONITOR YuiMonitor;

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
        YuiTaskbarFreeButtonsOneMonitor(YuiMonitor);
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    }
}

/**
 Switch to the application described by a taskbar button.

 @param YuiMonitor Pointer to the monitor context.

 @param ThisButton Pointer to the taskbar button description.
 */
VOID
YuiTaskbarSwitchToButton(
    __in PYUI_MONITOR YuiMonitor,
    __in PYUI_TASKBAR_BUTTON ThisButton
    )
{

    if (DllUser32.pSwitchToThisWindow != NULL) {
        DllUser32.pSwitchToThisWindow(ThisButton->hWndToActivate, TRUE);
    } else {
        if (IsIconic(ThisButton->hWndToActivate)) {
            if (DllUser32.pShowWindowAsync != NULL) {
                DllUser32.pShowWindowAsync(ThisButton->hWndToActivate, SW_RESTORE);
            } else {
                DllUser32.pShowWindow(ThisButton->hWndToActivate, SW_RESTORE);
            }
        }
        DllUser32.pSetForegroundWindow(ThisButton->hWndToActivate);
        SetFocus(ThisButton->hWndToActivate);
    }

    //
    //  If the taskbar is polling, force an update now without
    //  waiting for the poll.  If it's driven by events, don't
    //  update now, and handle it as part of the window activation
    //  notification (so it's only repainted once.)
    //

    if (YuiMonitor->YuiContext->TaskbarRefreshFrequency != 0) {
        YuiTaskbarNotifyActivateWindow(YuiMonitor->YuiContext, ThisButton->hWndToActivate);
    }
}

/**
 Switch to the window associated with the specified control identifier.

 @param YuiMonitor Pointer to the monitor context.

 @param CtrlId The identifier of the button that was pressed indicating a
        desire to change to the associated window.
 */
VOID
YuiTaskbarSwitchToTask(
    __in PYUI_MONITOR YuiMonitor,
    __in DWORD CtrlId
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;

    ThisButton = YuiTaskbarFindButtonFromCtrlId(YuiMonitor, CtrlId);
    if (ThisButton != NULL) {
        YuiTaskbarSwitchToButton(YuiMonitor, ThisButton);
    }
}

/**
 Attempt to launch a new instance of a running program.  This is invoked if
 the user shift-clicks a taskbar button.

 @param YuiMonitor Pointer to the monitor context.

 @param CtrlId The identifier of the button that was pressed indicating a
        desire to relaunch the associated window.
 */
VOID
YuiTaskbarLaunchNewTask(
    __in PYUI_MONITOR YuiMonitor,
    __in DWORD CtrlId
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    YORI_STRING ModuleName;
    HANDLE ProcessHandle;
    HINSTANCE hInst;
    BOOLEAN Elevated;

    ThisButton = YuiTaskbarFindButtonFromCtrlId(YuiMonitor, CtrlId);
    if (ThisButton != NULL) {

        //
        //  Simple case: this window was found to be associated with a process
        //  launched by this program.  In that case, we know which shortcut
        //  was used to launch it, and can accurately launch a new instance
        //  from the same shortcut.
        //

        Elevated = FALSE;
        if (ThisButton->ChildProcess != NULL) {
            Elevated = ThisButton->ChildProcess->Elevated;
        }
        if (ThisButton->ShortcutPath.LengthInChars > 0) {
#if DBG
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Launching shortcut associated with button %y\n"), &ThisButton->ShortcutPath);
#endif
            YuiExecuteShortcut(YuiMonitor, &ThisButton->ShortcutPath, Elevated);
            return;
        }

        //
        //  If that didn't happen, look up the process name associated with
        //  the window and launch it.  We don't know anything about startup
        //  parameters to use.
        //

        YoriLibLoadPsapiFunctions();
        if (DllPsapi.pGetModuleFileNameExW == NULL ||
            DllShell32.pShellExecuteW == NULL) {
            return;
        }

        //
        //  Attempt to find the process executable.  If we don't have access,
        //  there's not much we can do.
        //

        ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, ThisButton->ProcessId);
        if (ProcessHandle == NULL) {
            return;
        }

        if (!YoriLibAllocateString(&ModuleName, 32 * 1024)) {
            CloseHandle(ProcessHandle);
            return;
        }

        ModuleName.LengthInChars = (YORI_ALLOC_SIZE_T)DllPsapi.pGetModuleFileNameExW(ProcessHandle, NULL, ModuleName.StartOfString, ModuleName.LengthAllocated);

        //
        //  Launch what we've got, no arguments, working directory, or other
        //  state.
        //

        hInst = DllShell32.pShellExecuteW(NULL, NULL, ModuleName.StartOfString, NULL, NULL, SW_SHOWNORMAL);

        YoriLibFreeStringContents(&ModuleName);
        CloseHandle(ProcessHandle);
    }
}

/**
 Display the context menu associated with a specific taskbar button.

 @param YuiMonitor Pointer to the monitor context.

 @param CtrlId A control ID for the taskbar button.

 @param CursorX The horizontal location of the mouse, used to decide where to
        draw the menu.

 @param CursorY The vertical location of the mouse, used to decide where to
        draw the menu.

 @return TRUE if any action was performed, FALSE if it was not.
 */
BOOL
YuiTaskbarDisplayContextMenuForTask(
    __in PYUI_MONITOR YuiMonitor,
    __in DWORD CtrlId,
    __in DWORD CursorX,
    __in DWORD CursorY
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;

    ThisButton = YuiTaskbarFindButtonFromCtrlId(YuiMonitor, CtrlId);
    if (ThisButton != NULL) {
        return YuiMenuDisplayWindowContext(YuiMonitor,
                                           YuiMonitor->hWndTaskbar,
                                           ThisButton->hWndToActivate,
                                           ThisButton->ProcessId,
                                           CursorX,
                                           CursorY);
    }

    return FALSE;
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
    PYUI_MONITOR YuiMonitor;

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
        ListEntry = NULL;
        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        while (ListEntry != NULL) {
            ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
            if (ThisButton->WindowActive) {
                YuiTaskbarSwitchToButton(YuiMonitor, ThisButton);
                break;
            }
            ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        }
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    }
}

/**
 Draw a taskbar button.

 @param YuiMonitor Pointer to the monitor context.

 @param CtrlId The identifier of the button that should be redrawn.

 @param DrawItemStruct Pointer to a structure describing the button to
        redraw, including its bounds and device context.
 */
VOID
YuiTaskbarDrawButton(
    __in PYUI_MONITOR YuiMonitor,
    __in DWORD CtrlId,
    __in PDRAWITEMSTRUCT DrawItemStruct
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    HICON Icon;
    LRESULT Result;
    DWORD_PTR MsgResult;

    ThisButton = YuiTaskbarFindButtonFromCtrlId(YuiMonitor, CtrlId);
    if (ThisButton == NULL) {
        return;
    }

    //
    //  Try to get the icon for the window.  This can invoke a different
    //  process window procedure, and we're currently in the rendering path
    //  of the taskbar.  Set a fairly small timeout - if the remote window
    //  procedure doesn't respond really fast, just fall back to the class
    //  icon.
    //

    Icon = NULL;
    Result = SendMessageTimeout(ThisButton->hWndToActivate, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 10, &MsgResult);
    if (Result) {
        Icon = (HICON)MsgResult;
    }

    if (Icon == NULL) {
        Icon = (HICON)GetClassLongPtr(ThisButton->hWndToActivate, GCLP_HICONSM);
    }

    YuiDrawButton(YuiMonitor,
                  DrawItemStruct,
                  ThisButton->WindowActive,
                  ThisButton->Flashing,
                  FALSE,
                  Icon,
                  &ThisButton->ButtonText,
                  FALSE);
}

/**
 Check if the specified position overlaps with the start button.

 @param YuiMonitor Pointer to the monitor context.

 @param XPos The horizontal coordinate, relative to the client area.

 @return TRUE if the coordinate overlaps with the start button, FALSE if it
         does not.
 */
BOOLEAN
YuiTaskbarIsPositionOverStart(
    __in PYUI_MONITOR YuiMonitor,
    __in SHORT XPos
    )
{
    if (XPos <= YuiMonitor->StartRightOffset) {
        return TRUE;
    }

    return FALSE;
}

/**
 Find a taskbar button based on the horizontal coordinate relative to the
 client area.  This is used to activate buttons if the user clicks outside
 of the button area.

 @param YuiMonitor Pointer to the monitor context.

 @param XPos The horizontal coordinate, relative to the client area.

 */
WORD
YuiTaskbarFindByOffset(
    __in PYUI_MONITOR YuiMonitor,
    __in SHORT XPos
    )
{
    PYUI_TASKBAR_BUTTON ThisButton;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = NULL;
    ThisButton = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
    while (ListEntry != NULL) {
        ThisButton = CONTAINING_RECORD(ListEntry, YUI_TASKBAR_BUTTON, ListEntry);
        if (XPos >= ThisButton->LeftOffset && XPos <= ThisButton->RightOffset) {
            return ThisButton->ControlId;
        }
        ListEntry = YoriLibGetNextListEntry(&YuiMonitor->TaskbarButtons, ListEntry);
        ThisButton = NULL;
    }
    return 0;
}

// vim:sw=4:ts=4:et:
