/**
 * @file yui/multimon.c
 *
 * Yori shell lightweight graphical UI monitor routines
 *
 * Copyright (c) 2019-2024 Malcolm J. Smith
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
#include "resource.h"

/**
 Return the next monitor in a list of monitors, so the list can be iterated.

 @param YuiContext Pointer to the application context.

 @param PreviousMonitor Pointer to the monitor before the one to return.

 @return Pointer to the next monitor, or NULL if no further monitors exist.
 */
PYUI_MONITOR
YuiGetNextMonitor(
    __in PYUI_CONTEXT YuiContext,
    __in_opt PYUI_MONITOR PreviousMonitor
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_MONITOR YuiMonitor;

    if (PreviousMonitor == NULL) {
        ListEntry = NULL;
    } else {
        ListEntry = &PreviousMonitor->ListEntry;
    }

    ListEntry = YoriLibGetNextListEntry(&YuiContext->MonitorList, ListEntry);
    if (ListEntry == NULL) {
        return NULL;
    }

    YuiMonitor = CONTAINING_RECORD(ListEntry, YUI_MONITOR, ListEntry);
    return YuiMonitor;
}

/**
 Return the next monitor that is known to exist.  This occurs after the set
 of monitors has been refreshed, where some existing monitors may be removed,
 new monitors may be added, etc.  This program assumes the existence of at
 least one monitor.

 @param YuiContext Pointer to the application context.

 @param PreviousMonitor Pointer to the monitor before the one to return.

 @return Pointer to the next confirmed monitor, or NULL if no further
         confirmed monitors exist.
 */
PYUI_MONITOR
YuiGetNextConfirmedMonitor(
    __in PYUI_CONTEXT YuiContext,
    __in_opt PYUI_MONITOR PreviousMonitor
    )
{
    PYUI_MONITOR YuiMonitor;

    YuiMonitor = PreviousMonitor;
    while(TRUE) {
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
        if (YuiMonitor == NULL) {
            break;
        }

        if (YuiMonitor->AssociatedMonitorFound) {
            break;
        }
    }

    return YuiMonitor;
}

/**
 Look for a monitor containing the specified taskbar window.

 @param YuiContext Pointer to the application context.

 @param hWnd The taskbar window to find.

 @return Pointer to the monitor context, or NULL if no match is found.
 */
PYUI_MONITOR
YuiMonitorFromTaskbarHwnd(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    PYUI_MONITOR YuiMonitor;
    YuiMonitor = NULL;

    YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
        if (YuiMonitor->hWndTaskbar == hWnd) {
            return YuiMonitor;
        }
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    }

    return NULL;
}

/**
 Look for a YUI monitor on the specified HMONITOR.

 @param YuiContext Pointer to the application context.

 @param hMonitor The monitor to find.

 @return Pointer to the monitor context, or NULL if no match is found.
 */
PYUI_MONITOR
YuiMonitorFromHMonitor(
    __in PYUI_CONTEXT YuiContext,
    __in_opt HANDLE hMonitor
    )
{
    PYUI_MONITOR YuiMonitor;
    YuiMonitor = NULL;

    YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
        if (YuiMonitor->MonitorHandle == hMonitor) {
            return YuiMonitor;
        }
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    }

    return NULL;
}


/**
 Look for a monitor containing the specified application window.

 @param YuiContext Pointer to the application context.

 @param hWnd The window to find.

 @return Pointer to the monitor context.
 */
PYUI_MONITOR
YuiMonitorFromApplicationHwnd(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    HANDLE hMonitor;
    PYUI_MONITOR YuiMonitor;

    if (DllUser32.pMonitorFromWindow != NULL) {
        hMonitor = DllUser32.pMonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        YuiMonitor = YuiMonitorFromHMonitor(YuiContext, hMonitor);
#if DBG
        if (YuiContext->DebugLogEnabled) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                          _T("MonitorFromWindow for hwnd %08x returned hMonitor %p YuiMonitor %p\n"),
                          hWnd,
                          hMonitor,
                          YuiMonitor);
        }
#endif
        if (YuiMonitor != NULL) {
            return YuiMonitor;
        }
    }
    return YuiContext->PrimaryMon;
}

/**
 Return the next explorer taskbar in a list of taskbars, so the list can be
 iterated.

 @param YuiContext Pointer to the application context.

 @param PreviousTaskbar Pointer to the explorer taskbar before the one to
        return.

 @return Pointer to the next explorer taskbar, or NULL if no further taskbars
         exist.
 */
PYUI_EXPLORER_TASKBAR
YuiGetNextExplorerTaskbar(
    __in PYUI_CONTEXT YuiContext,
    __in_opt PYUI_EXPLORER_TASKBAR PreviousTaskbar
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_EXPLORER_TASKBAR ExplorerTaskbar;

    if (PreviousTaskbar == NULL) {
        ListEntry = NULL;
    } else {
        ListEntry = &PreviousTaskbar->ListEntry;
    }

    ListEntry = YoriLibGetNextListEntry(&YuiContext->ExplorerTaskbarList, ListEntry);
    if (ListEntry == NULL) {
        return NULL;
    }

    ExplorerTaskbar = CONTAINING_RECORD(ListEntry, YUI_EXPLORER_TASKBAR, ListEntry);
    return ExplorerTaskbar;
}

/**
 Create a YUI_MONITOR structure for an hMonitor.  If an existing structure
 exists for this hMonitor, it is returned, and no new structure is created.

 @param YuiContext Pointer to the application context.

 @param hMonitor Handle to the monitor.

 @param MonitorRect Pointer to the region of the virtual screen occupied by
        this monitor.

 @param IsPrimary TRUE to indicate this is the primary monitor; FALSE if it
        is a regular monitor.

 @return Pointer to the YUI_MONITOR structure for this monitor, or NULL on
         failure.
 */
PYUI_MONITOR
YuiCreateOrInitializeMonitor(
    __in PYUI_CONTEXT YuiContext,
    __in_opt HANDLE hMonitor,
    __in LPRECT MonitorRect,
    __in BOOLEAN IsPrimary
    )
{
    PYUI_MONITOR YuiMonitor;

    YuiMonitor = YuiMonitorFromHMonitor(YuiContext, hMonitor);
    if (YuiMonitor != NULL) {

        YuiMonitor->AssociatedMonitorFound = TRUE;

        if (YuiMonitor->ScreenLeft != MonitorRect->left ||
            YuiMonitor->ScreenTop != MonitorRect->top ||
            YuiMonitor->ScreenWidth != (DWORD)(MonitorRect->right - MonitorRect->left) ||
            YuiMonitor->ScreenHeight != (DWORD)(MonitorRect->bottom - MonitorRect->top)) {

            YuiMonitor->DimensionsChanged = TRUE;
            YuiMonitor->ScreenLeft = MonitorRect->left;
            YuiMonitor->ScreenTop = MonitorRect->top;
            YuiMonitor->ScreenWidth = MonitorRect->right - MonitorRect->left;
            YuiMonitor->ScreenHeight = MonitorRect->bottom - MonitorRect->top;
        }

#if DBG
        if (YuiContext->DebugLogEnabled) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                          _T("Found existing monitor %p, DimensionsChanged %i Origin %i,%i Width %i Height %i\n"),
                          YuiMonitor->MonitorHandle,
                          YuiMonitor->DimensionsChanged,
                          YuiMonitor->ScreenLeft,
                          YuiMonitor->ScreenTop,
                          YuiMonitor->ScreenWidth,
                          YuiMonitor->ScreenHeight);
        }
#endif
        return YuiMonitor;
    }

    YuiMonitor = YoriLibMalloc(sizeof(YUI_MONITOR));
    if (YuiMonitor == NULL) {
        return NULL;
    }

    ZeroMemory(YuiMonitor, sizeof(YUI_MONITOR));
    YuiMonitor->YuiContext = YuiContext;
    YuiMonitor->MonitorHandle = hMonitor;

    YoriLibInitializeListHead(&YuiMonitor->TaskbarButtons);
    YuiMonitor->NextTaskbarId = YUI_FIRST_TASKBAR_BUTTON;

    YuiMonitor->ScreenLeft = MonitorRect->left;
    YuiMonitor->ScreenTop = MonitorRect->top;
    YuiMonitor->ScreenWidth = MonitorRect->right - MonitorRect->left;
    YuiMonitor->ScreenHeight = MonitorRect->bottom - MonitorRect->top;

#if DBG
    if (YuiContext->DebugLogEnabled) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Created monitor %p, Origin %i,%i Width %i Height %i\n"),
                      YuiMonitor->MonitorHandle,
                      YuiMonitor->ScreenLeft,
                      YuiMonitor->ScreenTop,
                      YuiMonitor->ScreenWidth,
                      YuiMonitor->ScreenHeight);
    }
#endif

    YuiMonitor->AssociatedMonitorFound = TRUE;
    YuiMonitor->DimensionsChanged = TRUE;

    //
    //  Yui's primary monitor is a bit silly because all monitors are
    //  symmetrical.  The "primary" is the one that is used for global
    //  window messages, and must never change.
    //

    if (IsPrimary && YuiContext->PrimaryMon == NULL) {
        YuiContext->PrimaryMon = YuiMonitor;
    }

    YoriLibAppendList(&YuiContext->MonitorList, &YuiMonitor->ListEntry);

    return YuiMonitor;
}

/**
 A callback invoked when enumerating monitors.

 @param hMonitor Handle to the monitor.

 @param hDC Device context, ignored in this program.

 @param lprcMonitor Pointer to the region of the virtual screen occupied by
        this monitor.

 @param dwData Context information; in this program, a pointer to the global
        application context.

 @return TRUE to continue enumerating, FALSE to terminate.
 */
BOOL WINAPI
YuiMonitorCallback(
    __in HANDLE hMonitor,
    __in HDC hDC,
    __in LPRECT lprcMonitor,
    __in LPARAM dwData
    )
{
    PYUI_CONTEXT YuiContext;
    YORI_MONITORINFO MonitorInfo;
    BOOLEAN IsPrimary;

    UNREFERENCED_PARAMETER(hDC);

    YuiContext = (PYUI_CONTEXT)dwData;

    if (DllUser32.pGetMonitorInfoW == NULL) {
        return FALSE;
    }

    MonitorInfo.cbSize = sizeof(MonitorInfo);
    if (!DllUser32.pGetMonitorInfoW(hMonitor, &MonitorInfo)) {
        return FALSE;
    }

    IsPrimary = FALSE;
    if (MonitorInfo.dwFlags & MONITORINFOF_PRIMARY) {
        IsPrimary = TRUE;
    }

    if (YuiCreateOrInitializeMonitor(YuiContext, hMonitor, lprcMonitor, IsPrimary) == NULL) {
        return FALSE;
    }

    return TRUE;
}

/**
 Enumerate monitors in the system and create corresponding YUI_MONITOR
 structures.  At this point each monitor is allocated with populated
 dimensions, but child windows for each monitor are not created.  Note this
 logic is used when monitors change, and it does not deallocate monitors,
 only create new monitors.

 @param YuiContext Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YuiInitializeMonitors(
    __in PYUI_CONTEXT YuiContext
    )
{
    RECT MonitorRect;
    DWORD ScreenWidth;
    DWORD ScreenHeight;

    YuiContext->VirtualScreenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    YuiContext->VirtualScreenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    YuiContext->VirtualScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    YuiContext->VirtualScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    if (YuiContext->VirtualScreenWidth == 0 ||
        YuiContext->VirtualScreenHeight == 0) {

        YuiContext->VirtualScreenLeft = 0;
        YuiContext->VirtualScreenTop = 0;
        YuiContext->VirtualScreenWidth = ScreenWidth;
        YuiContext->VirtualScreenHeight = ScreenHeight;
    }


    if (DllUser32.pEnumDisplayMonitors != NULL) {
        if (DllUser32.pEnumDisplayMonitors(NULL, NULL, YuiMonitorCallback, (LPARAM)YuiContext)) {
            ASSERT(YuiContext->PrimaryMon != NULL);
            return TRUE;
        }
    }

    MonitorRect.left = 0;
    MonitorRect.top = 0;
    MonitorRect.right = ScreenWidth;
    MonitorRect.bottom = ScreenHeight;

    if (YuiCreateOrInitializeMonitor(YuiContext, NULL, &MonitorRect, TRUE) == NULL) {
        return FALSE;
    }
    ASSERT(YuiContext->PrimaryMon != NULL);

    return TRUE;
}

/**
 Set the work area to its desired size.  This function assumes that the task
 bar must be at the bottom of the primary display, and the explorer task bar
 is not present.

 @param YuiMonitor Pointer to the monitor context.

 @param Notify If TRUE, notify running applications of the work area update.
        If FALSE, suppress notifications. Note that notifying applications
        implies notifying Explorer, which may mean it will redisplay itself.

 @return TRUE to indicate the work area was updated, FALSE if it was not.
 */
BOOLEAN
YuiResetWorkArea(
    __in PYUI_MONITOR YuiMonitor,
    __in BOOLEAN Notify
    )
{
    RECT NewWorkArea;
    PRECT OldWorkArea;
    YORI_MONITORINFO MonitorInfo;

    if (DllUser32.pGetMonitorInfoW != NULL) {
        MonitorInfo.cbSize = sizeof(MonitorInfo);
        if (!DllUser32.pGetMonitorInfoW(YuiMonitor->MonitorHandle, &MonitorInfo)) {
            return FALSE;
        }
    } else {
        if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &MonitorInfo.rcWork, 0)) {
            return FALSE;
        }
    }

    OldWorkArea = &MonitorInfo.rcWork;

    NewWorkArea.left = OldWorkArea->left;
    NewWorkArea.right = OldWorkArea->right;
    NewWorkArea.top = OldWorkArea->top;
    NewWorkArea.bottom = YuiMonitor->ScreenTop + 
                         YuiMonitor->ScreenHeight - YuiMonitor->TaskbarHeight;

    if (NewWorkArea.left != OldWorkArea->left ||
        NewWorkArea.right != OldWorkArea->right ||
        NewWorkArea.top != OldWorkArea->top ||
        NewWorkArea.bottom != OldWorkArea->bottom) {
        DWORD Flags;

        Flags = 0;
        if (Notify) {
            Flags = SPIF_SENDWININICHANGE;
        }
#if DBG
        if (YuiMonitor->YuiContext->DebugLogEnabled) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                          _T("Updating work area, OldWorkArea %i,%i-%i,%i, NewWorkArea %i,%i-%i,%i Notify %i\n"),
                          OldWorkArea->left,
                          OldWorkArea->top,
                          OldWorkArea->right,
                          OldWorkArea->bottom,
                          NewWorkArea.left,
                          NewWorkArea.top,
                          NewWorkArea.right,
                          NewWorkArea.bottom,
                          Notify);
        }
#endif

        SystemParametersInfo(SPI_SETWORKAREA, 0, &NewWorkArea, Flags);
        return TRUE;
    }

    return FALSE;
}

/**
 If the explorer taskbar is visible, hide it.

 @param YuiContext Pointer to the application context.
 
 @param ExplorerTaskbar Pointer to the explorer taskbar to hide.

 @return TRUE to indicate an explorer taskbar window was hidden or FALSE if
         no state change occurred.
 */
BOOLEAN
YuiHideExplorerTaskbar(
    __in PYUI_CONTEXT YuiContext,
    __in PYUI_EXPLORER_TASKBAR ExplorerTaskbar
    )
{
    UNREFERENCED_PARAMETER(YuiContext);

    //
    //  If the taskbar isn't visible, claim we hid it.  This isn't really
    //  true but it's useful to recover if Yui previously terminated
    //  abnormally, so a new instance can learn the effects of a previous
    //  instance.
    //

    if (!IsWindowVisible(ExplorerTaskbar->hWnd)) {
        ExplorerTaskbar->Hidden = TRUE;
        return FALSE;
    }

#if DBG
    if (YuiContext->DebugLogEnabled) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                      _T("Attempting to hide taskbar %p\n"),
                      ExplorerTaskbar->hWnd);
    }
#endif

    DllUser32.pShowWindow(ExplorerTaskbar->hWnd, SW_HIDE);
    ExplorerTaskbar->Hidden = TRUE;
    return TRUE;
}

/**
 Cleanup a single monitor context.

 @param YuiMonitor Pointer to a monitor context to clean up.
 */
VOID
YuiCleanupMonitor(
    __in PYUI_MONITOR YuiMonitor
    )
{
    if (YuiMonitor->DropHandle != NULL) {
        YuiUnregisterDropWindow(YuiMonitor->hWndTaskbar, YuiMonitor->DropHandle);
        YuiMonitor->DropHandle = NULL;
    }

    if (YuiMonitor->hWndClock != NULL) {
        DestroyWindow(YuiMonitor->hWndClock);
        YuiMonitor->hWndClock = NULL;
    }

    if (YuiMonitor->hWndBattery != NULL) {
        DestroyWindow(YuiMonitor->hWndBattery);
        YuiMonitor->hWndBattery = NULL;
    }

    if (YuiMonitor->hWndStart != NULL) {
        DestroyWindow(YuiMonitor->hWndStart);
        YuiMonitor->hWndStart = NULL;
    }

    if (YuiMonitor->hWndTaskbar != NULL) {
        DestroyWindow(YuiMonitor->hWndTaskbar);
        YuiMonitor->hWndTaskbar = NULL;
    }

    if (YuiMonitor->hFont) {
        DeleteObject(YuiMonitor->hFont);
        YuiMonitor->hFont = NULL;
    }

    if (YuiMonitor->hBoldFont) {
        DeleteObject(YuiMonitor->hBoldFont);
        YuiMonitor->hBoldFont = NULL;
    }

    YuiTaskbarFreeButtonsOneMonitor(YuiMonitor);
}

/**
 Take window state from one monitor and assign it to a different monitor.
 This happens when the "primary" monitor is removed, but this program needs
 to keep those window handles since they are used for global notifications.
 To solve this problem, a random existing monitor is cleaned, and window
 handles are assigned to it.  These will be moved to the correct location on
 the monitor as part of resolution change handling.

 @param TargetMonitor Pointer to an already cleaned YUI_MONITOR that can have
        window handles assigned to it.

 @param SourceMonitor Pointer to an existing YUI_MONITOR which should have
        window handles taken from it, and is effectively clean on completion.
 */
VOID
YuiAssignMonitorState(
    __in PYUI_MONITOR TargetMonitor,
    __in PYUI_MONITOR SourceMonitor
    )
{
    PYORI_LIST_ENTRY ListEntry;

    ASSERT(TargetMonitor->DropHandle == NULL);
    TargetMonitor->DropHandle = SourceMonitor->DropHandle;
    SourceMonitor->DropHandle = NULL;

    ASSERT(TargetMonitor->hWndClock == NULL);
    TargetMonitor->hWndClock = SourceMonitor->hWndClock;
    SourceMonitor->hWndClock = NULL;

    ASSERT(TargetMonitor->hWndBattery == NULL);
    TargetMonitor->hWndBattery = SourceMonitor->hWndBattery;
    SourceMonitor->hWndBattery = NULL;

    ASSERT(TargetMonitor->hWndStart == NULL);
    TargetMonitor->hWndStart = SourceMonitor->hWndStart;
    SourceMonitor->hWndStart = NULL;

    ASSERT(TargetMonitor->hWndTaskbar == NULL);
    TargetMonitor->hWndTaskbar = SourceMonitor->hWndTaskbar;
    SourceMonitor->hWndTaskbar = NULL;

    ASSERT(TargetMonitor->hFont == NULL);
    TargetMonitor->hFont = SourceMonitor->hFont;
    SourceMonitor->hFont = NULL;

    ASSERT(YoriLibIsListEmpty(&TargetMonitor->TaskbarButtons));

    while (TRUE) {
        ListEntry = YoriLibGetNextListEntry(&SourceMonitor->TaskbarButtons, NULL);
        if (ListEntry == NULL) {
            break;
        }
        ASSERT(SourceMonitor->TaskbarButtonCount > 0);
        SourceMonitor->TaskbarButtonCount--;
        YoriLibRemoveListItem(ListEntry);
        YoriLibAppendList(&TargetMonitor->TaskbarButtons, ListEntry);
        TargetMonitor->TaskbarButtonCount++;
    }
    ASSERT(SourceMonitor->TaskbarButtonCount == 0);
    YoriLibInitializeListHead(&SourceMonitor->TaskbarButtons);
}


/**
 Indicate that no Explorer taskbars have been found in preparation for a
 rescan.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiResetExplorerTaskbarsFound(
    __in PYUI_CONTEXT YuiContext
    )
{
    PYUI_EXPLORER_TASKBAR ExplorerTaskbar;

    ExplorerTaskbar = YuiGetNextExplorerTaskbar(YuiContext, NULL);
    while (ExplorerTaskbar != NULL) {
        ExplorerTaskbar->Found = FALSE;
        ExplorerTaskbar = YuiGetNextExplorerTaskbar(YuiContext, ExplorerTaskbar);
    }
}

/**
 Deallocate any Explorer taskbars which were not found in a rescan.  Since
 these have already been destroyed by Explorer, there's no point trying to
 un-hide them.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiRemoveDestroyedTaskbars(
    __in PYUI_CONTEXT YuiContext
    )
{
    PYUI_EXPLORER_TASKBAR ExplorerTaskbar;
    PYUI_EXPLORER_TASKBAR NextTaskbar;

    ExplorerTaskbar = YuiGetNextExplorerTaskbar(YuiContext, NULL);
    while (ExplorerTaskbar != NULL) {
        NextTaskbar = YuiGetNextExplorerTaskbar(YuiContext, ExplorerTaskbar);
        if (!ExplorerTaskbar->Found) {
            YoriLibRemoveListItem(&ExplorerTaskbar->ListEntry);
#if DBG
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Removed Explorer taskbar %p, Primary %i\n"), ExplorerTaskbar->hWnd, ExplorerTaskbar->Primary);
#endif
            YoriLibFree(ExplorerTaskbar);
        }
        ExplorerTaskbar = NextTaskbar;
    }
}

/**
 Indicate that a new Explorer taskbar has been found as part of a rescan.
 This may find an existing entry and mark it as found, or may allocate a new
 entry.

 @param YuiContext Pointer to the application context.

 @param hWnd The window handle to the Explorer taskbar.

 @param Primary TRUE if this is the primary Explorer taskbar, FALSE if it is
        a secondary taskbar.
 */
VOID
YuiAddOrUpdateExplorerTaskbar(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd,
    __in BOOLEAN Primary
    )
{
    PYUI_EXPLORER_TASKBAR ExplorerTaskbar;

    ExplorerTaskbar = YuiGetNextExplorerTaskbar(YuiContext, NULL);
    while (ExplorerTaskbar != NULL) {
        if (ExplorerTaskbar->hWnd == hWnd) {
            ExplorerTaskbar->Found = TRUE;
            ExplorerTaskbar->Primary = Primary;
#if DBG
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Found existing Explorer taskbar %p, Primary %i\n"), hWnd, Primary);
#endif
            break;
        }
        ExplorerTaskbar = YuiGetNextExplorerTaskbar(YuiContext, ExplorerTaskbar);
    }

    if (ExplorerTaskbar == NULL) {
        ExplorerTaskbar = YoriLibMalloc(sizeof(YUI_EXPLORER_TASKBAR));
        if (ExplorerTaskbar != NULL) {
            ExplorerTaskbar->hWnd = hWnd;
            ExplorerTaskbar->Hidden = FALSE;
            ExplorerTaskbar->Found = TRUE;
            ExplorerTaskbar->Primary = Primary;
#if DBG
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Found new Explorer taskbar %p, Primary %i\n"), hWnd, Primary);
#endif
            YoriLibAppendList(&YuiContext->ExplorerTaskbarList, &ExplorerTaskbar->ListEntry);
        }
    }
}

/**
 A callback invoked for every window on the desktop.  Each window is checked
 to see if it is an Explorer taskbar window that should be associated with
 the monitor it is displayed on.

 @param hWnd Pointer to a window on the desktop.

 @param lParam Application defined context, in this case a pointer to the
        application context.

 @return TRUE to continue enumerating, or FALSE to stop enumerating.
 */
BOOL CALLBACK
YuiFindExplorerWindowFound(
    __in HWND hWnd,
    __in LPARAM lParam
    )
{
    PYUI_CONTEXT YuiContext;
    YORI_STRING ClassName;
    TCHAR ClassNameBuffer[64];

    YuiContext = (PYUI_CONTEXT)lParam;

    YoriLibInitEmptyString(&ClassName);
    ClassName.StartOfString = ClassNameBuffer;
    ClassName.LengthAllocated = sizeof(ClassNameBuffer)/sizeof(ClassNameBuffer[0]);

    ClassName.LengthInChars = (YORI_ALLOC_SIZE_T)GetClassName(hWnd, ClassName.StartOfString, ClassName.LengthAllocated);

    if (YoriLibCompareStringLit(&ClassName, _T("Shell_TrayWnd")) == 0) {
        YuiAddOrUpdateExplorerTaskbar(YuiContext, hWnd, TRUE);
    }

    if (YoriLibCompareStringLit(&ClassName, _T("Shell_SecondaryTrayWnd")) == 0) {
        YuiAddOrUpdateExplorerTaskbar(YuiContext, hWnd, FALSE);
    }

    return TRUE;
}

/**
 Scan for Explorer taskbars.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiFindExplorerTaskbars(
    __in PYUI_CONTEXT YuiContext
    )
{
    //
    //  Indicate that any known taskbars have not been found.
    //

    YuiResetExplorerTaskbarsFound(YuiContext);

    //
    //  Rescan the session looking for taskbars.
    //

    EnumWindows(YuiFindExplorerWindowFound, (LPARAM)YuiContext);

    //
    //  If any taskbar was previously known that was not found, destroy it.
    //  There's no point trying to unhide these; they're already gone.
    //

    YuiRemoveDestroyedTaskbars(YuiContext);
}

/**
 Update the work areas on all monitors to reflect the existence of Yui
 taskbars, and hide any found Explorer taskbars.

 @param YuiContext Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YuiAdjustAllWorkAreasAndHideExplorer(
    __in PYUI_CONTEXT YuiContext
    )
{
    PYUI_MONITOR YuiMonitor;
    PYUI_EXPLORER_TASKBAR ExplorerTaskbar;

    //
    //  Reset work areas, notifying applications.  Explorer can observe this
    //  and may attempt to display itself again, so only do this for a login
    //  shell.  If explorer is running, it will respond to this
    //  asynchronously, so this program has no idea when it's safe to
    //  proceed.
    //

    if (YuiContext->LoginShell) {
        YuiMonitor = YuiGetNextMonitor(YuiContext, NULL);
        while (YuiMonitor != NULL) {
            YuiResetWorkArea(YuiMonitor, TRUE);
            YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
        }
    }

    //
    //  Explorer is getting the same notification we do, and is busy
    //  updating itself too.  We can't hide taskbars until they're
    //  created, so give it a chance to "win" this race.
    //

    if (!YuiContext->LoginShell) {
        Sleep(500);
        YuiFindExplorerTaskbars(YuiContext);
    }

    //
    //  Hide explorer if it displayed itself after the above notification.
    //  Primary taskbars must be hidden first, since those will redisplay
    //  secondary taskbars when they are hidden.
    //

    ExplorerTaskbar = YuiGetNextExplorerTaskbar(YuiContext, NULL);
    while (ExplorerTaskbar != NULL) {
        if (ExplorerTaskbar->Primary) {
            YuiHideExplorerTaskbar(YuiContext, ExplorerTaskbar);
        }
        ExplorerTaskbar = YuiGetNextExplorerTaskbar(YuiContext, ExplorerTaskbar);
    }

    ExplorerTaskbar = YuiGetNextExplorerTaskbar(YuiContext, NULL);
    while (ExplorerTaskbar != NULL) {
        if (!ExplorerTaskbar->Primary) {
            YuiHideExplorerTaskbar(YuiContext, ExplorerTaskbar);
        }
        ExplorerTaskbar = YuiGetNextExplorerTaskbar(YuiContext, ExplorerTaskbar);
    }

    //
    //  Reset work areas again, without notifying applications.  This will
    //  only do anything if Explorer messed with things earlier, and
    //  suppressing notifications prevents that happening again.  It may
    //  leave applications in the wrong spot, but there's no way to notify
    //  everything _except_ Explorer.
    //

    YuiMonitor = YuiGetNextMonitor(YuiContext, NULL);
    while (YuiMonitor != NULL) {
        YuiResetWorkArea(YuiMonitor, FALSE);
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    }

    return TRUE;
}

/**
 Enumerate monitors to allocate new monitor structures and populate them
 with new window handles, as well as detect removed monitors to clean up.
 Any monitor whose resolution has changed will have its windows repositioned.
 Taskbars will be fully refreshed to correspond to the windows found on each
 monitor, since both taskbars and applications are moving across monitors.

 @param YuiContext Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YuiRefreshMonitors(
    __in PYUI_CONTEXT YuiContext
    )
{
    PYUI_MONITOR YuiMonitor;
    PYUI_MONITOR NextMonitor;
    BOOLEAN ChangeFound;

    //
    //  Indicate that no monitors have been found.
    //

    YuiMonitor = YuiGetNextMonitor(YuiContext, NULL);
    while (YuiMonitor != NULL) {
        YuiMonitor->AssociatedMonitorFound = FALSE;
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    }

    //
    //  Rescan monitors and find their current positions.
    //

    YuiInitializeMonitors(YuiContext);

    //
    //  If the monitor is not found and not a primary, clean it up.  If it
    //  is a primary, these window handles need to be preserved, so swap
    //  state with a monitor that does exist, and clean up the "non"
    //  primary monitor.
    //

    ChangeFound = FALSE;
    YuiMonitor = YuiGetNextMonitor(YuiContext, NULL);
    while (YuiMonitor != NULL) {
        NextMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
        if (!YuiMonitor->AssociatedMonitorFound) {
            ChangeFound = TRUE;
            if (YuiMonitor == YuiContext->PrimaryMon) {
                PYUI_MONITOR OtherMonitor;
                OtherMonitor = YuiGetNextConfirmedMonitor(YuiContext, NULL);
                ASSERT(OtherMonitor != NULL);
                ASSERT(OtherMonitor != YuiMonitor);
#if DBG
                if (YuiContext->DebugLogEnabled) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                                  _T("Moving window handles from %p to %p to destroy %p\n"),
                                  YuiMonitor->MonitorHandle,
                                  OtherMonitor->MonitorHandle,
                                  YuiMonitor->MonitorHandle);
                }
#endif

                //
                //  This just cleaned up a monitor that exists and has taskbar
                //  buttons.  Switching the structures means the taskbar and
                //  its buttons from the dead monitor are preserved, but not
                //  the buttons from the live monitor.  Later a full refresh
                //  is forced to handle this.
                //

                YuiCleanupMonitor(OtherMonitor);
                YuiAssignMonitorState(OtherMonitor, YuiMonitor);
                YoriLibRemoveListItem(&YuiMonitor->ListEntry);
                YoriLibFree(YuiMonitor);
                OtherMonitor->DimensionsChanged = TRUE;
                YuiContext->PrimaryMon = OtherMonitor;
            } else {
#if DBG
                if (YuiContext->DebugLogEnabled) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                                  _T("Destroying monitor %p\n"),
                                  YuiMonitor->MonitorHandle);
                }
#endif
                YuiCleanupMonitor(YuiMonitor);
                YoriLibRemoveListItem(&YuiMonitor->ListEntry);
                YoriLibFree(YuiMonitor);
            }
        } else if (YuiMonitor->hWndTaskbar == NULL) {
            YuiInitializeMonitor(YuiMonitor);
            //
            //  MSFIX This is getting cleared above and it really shouldn't
            //
            YuiMonitor->DimensionsChanged = TRUE;
        }
        YuiMonitor = NextMonitor;
    }

    //
    //  Refresh the display and rearrange windows according to the new
    //  monitor locations.
    //

    YuiMonitor = YuiGetNextMonitor(YuiContext, NULL);
    while (YuiMonitor != NULL) {
        YuiMonitor->AssociatedMonitorFound = FALSE;

        if (YuiMonitor->DimensionsChanged) {
            ChangeFound = TRUE;
            YuiNotifyResolutionChange(YuiMonitor);
            if (!IsWindowVisible(YuiMonitor->hWndTaskbar)) {
                DllUser32.pShowWindow(YuiMonitor->hWndTaskbar, SW_SHOW);
            }
        }
        YuiMonitor = YuiGetNextMonitor(YuiContext, YuiMonitor);
    }

    YuiAdjustAllWorkAreasAndHideExplorer(YuiContext);

    YuiTaskbarSyncWithCurrent(YuiContext);
    return TRUE;
}

/**
 Create a window on the taskbar for the battery.  This can be added after the
 taskbar is created if a battery starts reporting itself as existing.

 @param YuiMonitor Pointer to the monitor.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiInitializeBatteryWindow(
    __in PYUI_MONITOR YuiMonitor
    )
{
    RECT ClientRect;
    PYUI_CONTEXT YuiContext;

    YuiContext = YuiMonitor->YuiContext;

    ASSERT(YuiMonitor->hWndBattery == NULL);
    ASSERT(YuiMonitor->BatteryWidth != 0);

    //
    //  Create the battery window
    //

    DllUser32.pGetClientRect(YuiMonitor->hWndTaskbar, &ClientRect);
    YuiMonitor->hWndBattery = CreateWindowEx(0,
                                             _T("STATIC"),
                                             _T(""),
                                             SS_OWNERDRAW | SS_NOTIFY | WS_VISIBLE | WS_CHILD,
                                             ClientRect.right - YuiMonitor->ClockWidth - 3 - YuiMonitor->BatteryWidth - YuiMonitor->TaskbarPaddingHorizontal,
                                             YuiMonitor->TaskbarPaddingVertical,
                                             YuiMonitor->BatteryWidth,
                                             ClientRect.bottom - 2 * YuiMonitor->TaskbarPaddingVertical,
                                             YuiMonitor->hWndTaskbar,
                                             (HMENU)(DWORD_PTR)YUI_BATTERY_DISPLAY,
                                             NULL,
                                             NULL);

    if (YuiMonitor->hWndBattery == NULL) {
        return FALSE;
    }

    SendMessage(YuiMonitor->hWndBattery,
                WM_SETFONT,
                (WPARAM)YuiMonitor->hFont,
                MAKELPARAM(TRUE, 0));

    //
    //  Shrink the space for taskbar buttons and notify the taskbar to
    //  recalculate
    //

    YuiMonitor->RightmostTaskbarOffset = (WORD)(YuiMonitor->ControlBorderWidth +
                                                YuiMonitor->BatteryWidth +
                                                3 +
                                                YuiMonitor->ClockWidth +
                                                YuiMonitor->TaskbarPaddingHorizontal);
    if (!YuiContext->DisplayResolutionChangeInProgress) {
        YuiTaskbarNotifyResolutionChange(YuiMonitor);
        YuiContext->DisplayResolutionChangeInProgress = FALSE;
    }
    return TRUE;
}


// vim:sw=4:ts=4:et:
