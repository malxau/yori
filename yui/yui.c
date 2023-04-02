/**
 * @file yui/yui.c
 *
 * Yori shell display lightweight graphical UI
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
#include "resource.h"

/**
 Help text to display to the user.
 */
const
CHAR strYuiHelpText[] =
        "\n"
        "Display lightweight graphical UI.\n"
        "\n"
        "YUI\n";

/**
 Display usage text to the user.
 */
BOOL
YuiHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Yui %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYuiHelpText);
    return TRUE;
}

/**
 The global application context.
 */
YUI_CONTEXT YuiContext;

/**
 The base height of the taskbar, in pixels.
 */
#define YUI_BASE_TASKBAR_HEIGHT (32)

/**
 The base width of each taskbar button, in pixels.
 */
#define YUI_BASE_TASKBAR_BUTTON_WIDTH (190)

/**
 Query the taskbar height for this system.  The taskbar is generally 32 pixels
 but can scale upwards slightly on larger displays.

 @param ScreenWidth The width of the screen, in pixels.

 @param ScreenHeight The height of the screen, in pixels.

 @return The height of the taskbar, in pixels.
 */
DWORD
YuiGetTaskbarHeight(
    __in DWORD ScreenWidth,
    __in DWORD ScreenHeight
    )
{
    DWORD TaskbarHeight;

    UNREFERENCED_PARAMETER(ScreenWidth);

    TaskbarHeight = YUI_BASE_TASKBAR_HEIGHT;

    //
    //  Give 1% of any vertical pixels above 1200
    //

    if (ScreenHeight > 1200) {
        TaskbarHeight = TaskbarHeight + (ScreenHeight - 1200) / 100;
    }

    return TaskbarHeight;
}

/**
 Query the maximum taskbar button width for this system.  The taskbar
 generally uses 180 pixel buttons but can scale upwards slightly on larger
 displays.

 @param ScreenWidth The width of the screen, in pixels.

 @param ScreenHeight The height of the screen, in pixels.

 @return The maximum width of a taskbar button, in pixels.
 */
WORD
YuiGetTaskbarMaximumButtonWidth(
    __in DWORD ScreenWidth,
    __in DWORD ScreenHeight
    )
{
    DWORD TaskbarButtonWidth;

    UNREFERENCED_PARAMETER(ScreenHeight);

    TaskbarButtonWidth = YUI_BASE_TASKBAR_BUTTON_WIDTH;

    //
    //  Give 5% of any horizontal pixels above 1200
    //

    if (ScreenWidth > 1200) {
        TaskbarButtonWidth = TaskbarButtonWidth + (ScreenWidth - 1200) / 20;
    }

    return (WORD)TaskbarButtonWidth;
}

/**
 Draw the start button, including icon and text.

 @param DrawItemStruct Pointer to a structure describing the button to
        redraw, including its bounds and device context.
 */
VOID
YuiStartDrawButton(
    __in PDRAWITEMSTRUCT DrawItemStruct
    )
{
    YORI_STRING Text;
    YoriLibConstantString(&Text, _T("Start"));
    YuiDrawButton(DrawItemStruct, YuiContext.MenuActive, YuiContext.StartIcon, &Text, TRUE);
}

/**
 Indicates that the screen resolution has changed and the taskbar needs to be
 repositioned accordingly.

 @param hWnd The taskbar window to reposition.

 @param ScreenWidth The new width of the screen.

 @param ScreenHeight The new height of the screen.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiNotifyResolutionChange(
    __in HWND hWnd,
    __in DWORD ScreenWidth,
    __in DWORD ScreenHeight
    )
{
    YORI_APPBARDATA AppBar;
    RECT ClientRect;
    RECT NewWorkArea;
    RECT OldWorkArea;
    DWORD TaskbarHeight;
    DWORD FontSize;
    HDC hDC;
    HFONT hFont;
    HFONT hBoldFont;

    if (YuiContext.DisplayResolutionChangeInProgress) {
        return TRUE;
    }

    YuiContext.DisplayResolutionChangeInProgress = TRUE;

    YuiContext.MaximumTaskbarButtonWidth = YuiGetTaskbarMaximumButtonWidth(ScreenWidth, ScreenHeight);

    TaskbarHeight = YuiGetTaskbarHeight(ScreenWidth, ScreenHeight);

    FontSize = 9;
    FontSize = FontSize + (TaskbarHeight - YUI_BASE_TASKBAR_HEIGHT) / 3;

    hDC = GetWindowDC(hWnd);
    hFont = CreateFont(-YoriLibMulDiv(FontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72),
                       0,
                       0,
                       0,
                       FW_NORMAL,
                       FALSE,
                       FALSE,
                       FALSE,
                       DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY,
                       FF_DONTCARE,
                       _T("Tahoma"));

    hBoldFont = CreateFont(-YoriLibMulDiv(FontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72),
                           0,
                           0,
                           0,
                           FW_BOLD,
                           FALSE,
                           FALSE,
                           FALSE,
                           DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS,
                           CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY,
                           FF_DONTCARE,
                           _T("Tahoma"));
    ReleaseDC(hWnd, hDC);

    if (hFont != NULL) {
        if (YuiContext.hFont != NULL) {
            DeleteObject(YuiContext.hFont);
        }
        YuiContext.hFont = hFont;

        if (YuiContext.hWndClock != NULL) {
            SendMessage(YuiContext.hWndClock, WM_SETFONT, (WPARAM)YuiContext.hFont, MAKELPARAM(TRUE, 0));
        }
    }

    if (hBoldFont != NULL) {
        if (YuiContext.hBoldFont != NULL) {
            DeleteObject(YuiContext.hBoldFont);
        }
        YuiContext.hBoldFont = hBoldFont;

        if (YuiContext.hWndStart != NULL) {
            SendMessage(YuiContext.hWndStart, WM_SETFONT, (WPARAM)YuiContext.hBoldFont, MAKELPARAM(TRUE, 0));
        }
    }

    NewWorkArea.left = 0;
    NewWorkArea.right = ScreenWidth - 1;
    NewWorkArea.top = 0;
    NewWorkArea.bottom = ScreenHeight - TaskbarHeight - 1;

    if (DllShell32.pSHAppBarMessage != NULL) {

        AppBar.cbSize = sizeof(AppBar);
        AppBar.hWnd = hWnd;
        AppBar.uCallbackMessage = WM_USER;
        AppBar.uEdge = 3;
        AppBar.rc.left = 0;
        AppBar.rc.top = ScreenHeight - TaskbarHeight;
        AppBar.rc.bottom = ScreenHeight;
        AppBar.rc.right = ScreenWidth;
        AppBar.lParam = TRUE;

        // New
        DllShell32.pSHAppBarMessage(0, &AppBar);

        // Query (request) position
        DllShell32.pSHAppBarMessage(2, &AppBar);

        if (AppBar.rc.bottom - AppBar.rc.top < (INT)TaskbarHeight) {
            AppBar.rc.top = AppBar.rc.bottom - TaskbarHeight;
        }

        NewWorkArea.left = AppBar.rc.left;
        NewWorkArea.right = AppBar.rc.right;
        NewWorkArea.top = 0;
        NewWorkArea.bottom = AppBar.rc.top;

        DllUser32.pMoveWindow(hWnd,
                              AppBar.rc.left,
                              AppBar.rc.top,
                              AppBar.rc.right - AppBar.rc.left,
                              AppBar.rc.bottom - AppBar.rc.top,
                              TRUE);

        // Set position
        DllShell32.pSHAppBarMessage(3, &AppBar);

        // Activate
        DllShell32.pSHAppBarMessage(6, &AppBar);

    } else {
        DllUser32.pMoveWindow(hWnd,
                              0,
                              ScreenHeight - TaskbarHeight,
                              ScreenWidth,
                              TaskbarHeight,
                              TRUE);
    }


    //
    //  When explorer is running, this seems to happen automatically.
    //  That's desirable because it knows about all the other AppBars.
    //  If it hasn't already happened, set the work area to something
    //  that excludes this taskbar.
    //

    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &OldWorkArea, 0)) {
        if (OldWorkArea.bottom > NewWorkArea.bottom) {
            SystemParametersInfo(SPI_SETWORKAREA, 0, &NewWorkArea, 0);
        }
    }

    DllUser32.pGetClientRect(hWnd, &ClientRect);

    if (YuiContext.hWndClock != NULL) {
        DllUser32.pMoveWindow(YuiContext.hWndClock,
                              ClientRect.right - YUI_CLOCK_WIDTH - 1,
                              1,
                              YUI_CLOCK_WIDTH,
                              ClientRect.bottom - 2,
                              TRUE);
    }

    YuiTaskbarNotifyResolutionChange(&YuiContext);

    YuiContext.DisplayResolutionChangeInProgress = FALSE;

    return TRUE;
}

/**
 Display the start menu and perform any action requested.

 @return TRUE to indicate the start menu was displayed successfully.
 */
BOOL
YuiDisplayMenu(VOID)
{
    if (YuiContext.MenuActive) {
        return TRUE;
    }
    YuiContext.MenuActive = TRUE;
    SendMessage(YuiContext.hWndStart, BM_SETSTATE, TRUE, 0);
    YuiMenuDisplayAndExecute(&YuiContext, YuiContext.hWnd);
    if (YuiContext.MenuActive) {
        YuiContext.MenuActive = FALSE;
        SendMessage(YuiContext.hWndStart, BM_SETSTATE, FALSE, 0);
    }
    return TRUE;
}

/**
 The main window procedure which processes messages sent to the taskbar
 window.

 @param hwnd The window handle.

 @param uMsg The message identifier.

 @param wParam The first parameter associated with the window message.

 @param lParam The second parameter associated with the window message.

 @return A value which depends on the type of message being processed.
 */
LRESULT CALLBACK
YuiWindowProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    WORD CtrlId;

    switch(uMsg) {
        case WM_COMMAND:
            {
                CtrlId = LOWORD(wParam);
                if (CtrlId == YUI_START_BUTTON) {
                    YuiDisplayMenu();
                } else {
                    ASSERT(CtrlId >= YUI_FIRST_TASKBAR_BUTTON);
                    YuiTaskbarSwitchToTask(&YuiContext, CtrlId);
                }
            }
            break;
        case WM_LBUTTONDOWN:
            {
                short XPos;

                //
                //  Get the signed horizontal position.  Note that because
                //  nonclient clicks are translated to client area, these
                //  values can be negative.
                //

                XPos = (short)(LOWORD(lParam));
                if (XPos <= YuiContext.StartRightOffset) {
                    YuiDisplayMenu();
                } else {
                    CtrlId = YuiTaskbarFindByOffset(&YuiContext, XPos);
                    if (CtrlId >= YUI_FIRST_TASKBAR_BUTTON) {
                        YuiTaskbarSwitchToTask(&YuiContext, CtrlId);
                    } else {
                        YuiTaskbarSwitchToActiveTask(&YuiContext);
                    }
                }
            }
            break;
        case WM_NCHITTEST:

            //
            //  Indicate that mouse clicks outside the client area should be
            //  treated as part of the client area.  This window doesn't have
            //  moving or sizing controls, so there's nothing else to do.
            //

            return HTCLIENT;
            break;
        case WM_TIMER:
            switch(wParam) {
                case YUI_WINDOW_POLL_TIMER:
                    YuiTaskbarSyncWithCurrent(&YuiContext);
                    break;
                case YUI_CLOCK_TIMER:
                    YuiTaskbarUpdateClock(&YuiContext);
                    break;
            }
            return 0;
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_DISPLAYCHANGE:
            YuiNotifyResolutionChange(YuiContext.hWnd, LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_DRAWITEM:
            {
                PDRAWITEMSTRUCT DrawItemStruct;

                DrawItemStruct = (PDRAWITEMSTRUCT)lParam;

                CtrlId = LOWORD(wParam);
                if (CtrlId == YUI_START_BUTTON) {
                    YuiStartDrawButton(DrawItemStruct);
                } else {
                    ASSERT(CtrlId >= YUI_FIRST_TASKBAR_BUTTON);
                    YuiTaskbarDrawButton(&YuiContext, CtrlId, DrawItemStruct);
                }
            }
            break;
        default:
            if (uMsg == YuiContext.ShellHookMsg) {
                switch(wParam) {
                    case HSHELL_WINDOWACTIVATED:
                    case HSHELL_RUDEAPPACTIVATED:
                        YuiTaskbarNotifyActivateWindow(&YuiContext, (HWND)lParam);
                        break;
                    case HSHELL_WINDOWCREATED:
                        YuiTaskbarNotifyNewWindow(&YuiContext, (HWND)lParam);
                        break;
                    case HSHELL_WINDOWDESTROYED:
                        YuiTaskbarNotifyDestroyWindow(&YuiContext, (HWND)lParam);
                        break;
                    case HSHELL_REDRAW:
                        YuiTaskbarNotifyTitleChange(&YuiContext, (HWND)lParam);
                        break;
                    default:
                        break;
                }
            }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 Close any windows, timers, and other system resources.  In particular, note
 this tells explorer that the app bar is no longer present, which is something
 explorer really wants to be told because it has no process destruction
 notification.
 */ 
VOID
YuiCleanupGlobalState(VOID)
{
    DWORD Count;
    YuiMenuFreeAll(&YuiContext);
    YuiTaskbarFreeButtons(&YuiContext);

    for (Count = 0; Count < sizeof(YuiContext.StartChangeNotifications)/sizeof(YuiContext.StartChangeNotifications[0]); Count++) {
        if (YuiContext.StartChangeNotifications[Count] != NULL) {
            FindCloseChangeNotification(YuiContext.StartChangeNotifications[Count]);
            YuiContext.StartChangeNotifications[Count] = NULL;
        }
    }

    if (YuiContext.ClockTimerId != 0) {
        KillTimer(YuiContext.hWnd, YUI_CLOCK_TIMER);
        YuiContext.ClockTimerId = 0;
    }

    if (YuiContext.SyncTimerId != 0) {
        KillTimer(YuiContext.hWnd, YUI_WINDOW_POLL_TIMER);
        YuiContext.SyncTimerId = 0;
    }

    if (YuiContext.hWndClock != NULL) {
        DestroyWindow(YuiContext.hWndClock);
        YuiContext.hWndClock = NULL;
    }

    if (YuiContext.hWndStart != NULL) {
        DestroyWindow(YuiContext.hWndStart);
        YuiContext.hWndStart = NULL;
    }

    if (DllShell32.pSHAppBarMessage != NULL && YuiContext.hWnd != NULL) {
        YORI_APPBARDATA AppBar;

        AppBar.cbSize = sizeof(AppBar);
        AppBar.hWnd = YuiContext.hWnd;
        AppBar.uCallbackMessage = WM_USER;
        AppBar.uEdge = 3;
        AppBar.rc.left = 0;
        AppBar.rc.top = 0;
        AppBar.rc.bottom = 0;
        AppBar.rc.right = 0;
        AppBar.lParam = TRUE;

        // Remove
        DllShell32.pSHAppBarMessage(1, &AppBar);
    }

    if (YuiContext.hWnd != NULL) {
        DestroyWindow(YuiContext.hWnd);
        YuiContext.hWnd = NULL;
    }

    if (YuiContext.hFont) {
        DeleteObject(YuiContext.hFont);
        YuiContext.hFont = NULL;
    }

    if (YuiContext.hBoldFont) {
        DeleteObject(YuiContext.hBoldFont);
        YuiContext.hBoldFont = NULL;
    }

    if (YuiContext.StartIcon) {
        DestroyIcon(YuiContext.StartIcon);
        YuiContext.StartIcon = NULL;
    }

    if (YuiContext.SavedMinimizedMetrics.cbSize != 0) {
        SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(YORI_MINIMIZEDMETRICS), &YuiContext.SavedMinimizedMetrics, 0);
    }
}

/**
 Create the taskbar window, start button, and other assorted global elements,
 including populating the start menu and task bar with current state.

 @param Context Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiCreateWindow(
    __in PYUI_CONTEXT Context
    )
{
    WNDCLASS wc;
    BOOL Result;
    RECT ClientRect;
    DWORD ScreenHeight;
    DWORD ScreenWidth;
    DWORD TaskbarHeight;
    YORI_MINIMIZEDMETRICS MinimizedMetrics;

    //
    //  Explorer or other shells will register as a magic "Taskman" window.
    //  If this isn't set for the session, assume we must be the main shell.
    //

    if (DllUser32.pGetTaskmanWindow != NULL &&
        DllUser32.pGetTaskmanWindow() == NULL) {

        Context->LoginShell = TRUE;
    }

    YuiMenuPopulate(Context);

    wc.style = 0;
    wc.lpfnWndProc = YuiWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = NULL;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(DWORD_PTR)COLOR_WINDOW;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T("YuiWnd");

    Result = RegisterClass(&wc);
    ASSERT(Result);

    ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    ScreenWidth = GetSystemMetrics(SM_CXSCREEN);

    TaskbarHeight = YuiGetTaskbarHeight(ScreenWidth, ScreenHeight);
    Context->StartIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(STARTICON));

    Context->hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                                   _T("YuiWnd"),
                                   _T("Yui"),
                                   WS_POPUP | WS_DLGFRAME | WS_CLIPCHILDREN,
                                   0,
                                   ScreenHeight - TaskbarHeight,
                                   ScreenWidth,
                                   TaskbarHeight,
                                   NULL, NULL, NULL, 0);
    if (Context->hWnd == NULL) {
        return FALSE;
    }

    //
    //  Mark minimized windows as invisible.  The taskbar should still have
    //  them.  Save the previous state to restore on exit, and if nothing
    //  changes here, restore nothing on exit.
    //
    //  This is required for notification about window state changes.
    //

    Context->SavedMinimizedMetrics.cbSize = sizeof(MinimizedMetrics);
    if (SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, sizeof(YORI_MINIMIZEDMETRICS), &Context->SavedMinimizedMetrics, 0)) {
        MinimizedMetrics.cbSize = sizeof(MinimizedMetrics);
        MinimizedMetrics.iWidth = Context->SavedMinimizedMetrics.iWidth;
        MinimizedMetrics.iHorizontalGap = 0;
        MinimizedMetrics.iVerticalGap = 0;
        MinimizedMetrics.iArrange = ARW_HIDE;

        if (MinimizedMetrics.iArrange != Context->SavedMinimizedMetrics.iArrange) {
            SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(YORI_MINIMIZEDMETRICS), &MinimizedMetrics, 0);
        } else {
            Context->SavedMinimizedMetrics.cbSize = 0;
        }
    } else {
        Context->SavedMinimizedMetrics.cbSize = 0;
    }

    //
    //  Register as the Task Manager window.  This is required for
    //  notification about window state changes.
    //

    if (DllUser32.pSetTaskmanWindow != NULL) {
        DllUser32.pSetTaskmanWindow(Context->hWnd);
    }

    YuiNotifyResolutionChange(Context->hWnd, ScreenWidth, ScreenHeight);
    if (Context->hFont == NULL || Context->hBoldFont == NULL) {
        YuiCleanupGlobalState();
        return FALSE;
    }

    DllUser32.pGetClientRect(Context->hWnd, &ClientRect);

    Context->hWndStart = CreateWindow(_T("BUTTON"),
                                      _T("Start"),
                                      BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                                      1,
                                      1,
                                      YUI_START_BUTTON_WIDTH,
                                      ClientRect.bottom - 2,
                                      Context->hWnd,
                                      (HMENU)(DWORD_PTR)YUI_START_BUTTON,
                                      NULL,
                                      NULL);

    if (Context->hWndStart == NULL) {
        YuiCleanupGlobalState();
        return FALSE;
    }

    Context->StartLeftOffset = 1;
    Context->StartRightOffset = (WORD)(Context->StartLeftOffset + YUI_START_BUTTON_WIDTH);

    SendMessage(Context->hWndStart, WM_SETFONT, (WPARAM)Context->hBoldFont, MAKELPARAM(TRUE, 0));

    YoriLibInitEmptyString(&Context->ClockDisplayedValue);
    Context->ClockDisplayedValue.StartOfString = Context->ClockDisplayedValueBuffer;
    Context->ClockDisplayedValue.LengthAllocated = sizeof(Context->ClockDisplayedValueBuffer)/sizeof(Context->ClockDisplayedValueBuffer[0]);

    Context->hWndClock = CreateWindowEx(WS_EX_STATICEDGE,
                                        _T("STATIC"),
                                        _T(""),
                                        SS_CENTER | SS_SUNKEN | SS_CENTERIMAGE | WS_VISIBLE | WS_CHILD,
                                        ClientRect.right - YUI_CLOCK_WIDTH - 1,
                                        1,
                                        YUI_CLOCK_WIDTH,
                                        ClientRect.bottom - 2,
                                        Context->hWnd,
                                        NULL,
                                        NULL,
                                        NULL);

    if (Context->hWndClock == NULL) {
        YuiCleanupGlobalState();
        return FALSE;
    }

    SendMessage(Context->hWndClock, WM_SETFONT, (WPARAM)Context->hFont, MAKELPARAM(TRUE, 0));
    YuiTaskbarUpdateClock(Context);
    Context->ClockTimerId = SetTimer(Context->hWnd, YUI_CLOCK_TIMER, 5000, NULL);


    Context->LeftmostTaskbarOffset = 1 + YUI_START_BUTTON_WIDTH + 1;
    Context->RightmostTaskbarOffset = 1 + YUI_CLOCK_WIDTH + 1;

    YuiTaskbarPopulateWindows(Context, Context->hWnd);

    //
    //  Check if we're running on a platform that doesn't support
    //  notifications and we need to poll instead.
    //

    if (Context->TaskbarRefreshFrequency == 0 &&
        (DllUser32.pRegisterShellHookWindow == NULL)) {

        Context->TaskbarRefreshFrequency = 250;
    }

    //
    //  If we support notifications, attempt to set them up.
    //

    if (Context->TaskbarRefreshFrequency == 0) {
        Context->ShellHookMsg = RegisterWindowMessage(_T("SHELLHOOK"));
        if (!DllUser32.pRegisterShellHookWindow(Context->hWnd)) {
            Context->TaskbarRefreshFrequency = 250;
        }
    }

    //
    //  If the refresh frequency is specified, or the OS doesn't support
    //  notifications, or setting up notifications failed, set up polling
    //  now.
    //

    if (Context->TaskbarRefreshFrequency != 0) {
        Context->SyncTimerId = SetTimer(Context->hWnd, YUI_WINDOW_POLL_TIMER, Context->TaskbarRefreshFrequency, NULL);
    }

    DllUser32.pShowWindow(Context->hWnd, SW_SHOW);

    return TRUE;
}


/**
 The main entrypoint for the yui cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero on success, nonzero on failure.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    ZeroMemory(&YuiContext, sizeof(YuiContext));
    YoriLibInitializeListHead(&YuiContext.ProgramsDirectory.ListEntry);
    YoriLibInitializeListHead(&YuiContext.ProgramsDirectory.ChildDirectories);
    YoriLibInitializeListHead(&YuiContext.ProgramsDirectory.ChildFiles);
    YoriLibInitEmptyString(&YuiContext.ProgramsDirectory.DirName);
    YoriLibInitializeListHead(&YuiContext.StartDirectory.ListEntry);
    YoriLibInitializeListHead(&YuiContext.StartDirectory.ChildDirectories);
    YoriLibInitializeListHead(&YuiContext.StartDirectory.ChildFiles);
    YoriLibInitEmptyString(&YuiContext.StartDirectory.DirName);
    YoriLibInitializeListHead(&YuiContext.TaskbarButtons);
    YuiContext.TaskbarButtonCount = 0;

    YoriLibLoadUser32Functions();
    YoriLibLoadShell32Functions();
    YoriLibLoadWtsApi32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YuiHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019-2023"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                if (ArgC > i + 1) {
                    LONGLONG llTemp;
                    DWORD CharsConsumed;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
                        ArgumentUnderstood = TRUE;
                        YuiContext.TaskbarRefreshFrequency = (DWORD)llTemp;
                        i++;
                    }
                }
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (DllUser32.pDrawFrameControl == NULL ||
        DllUser32.pDrawIconEx == NULL ||
        DllUser32.pGetClientRect == NULL ||
        DllUser32.pGetWindowRect == NULL ||
        DllUser32.pMoveWindow == NULL ||
        DllUser32.pSetForegroundWindow == NULL ||
        DllUser32.pSetWindowTextW == NULL ||
        DllUser32.pShowWindow == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("yui: OS support not present\n"));
    }

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo)) {
        if (ScreenInfo.dwCursorPosition.X == 0 && ScreenInfo.dwCursorPosition.Y == 0) {
            FreeConsole();
        }
    }

    if (!YuiCreateWindow(&YuiContext)) {
        return EXIT_FAILURE;
    }

    {
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    YuiCleanupGlobalState();

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
