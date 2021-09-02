/**
 * @file yui/yui.c
 *
 * Yori shell display lightweight graphical UI
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
YUI_ENUM_CONTEXT YuiContext;

/**
 The height of the taskbar, in pixels.
 */
#define YUI_TASKBAR_HEIGHT (28)


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

    if (YuiContext.DisplayResolutionChangeInProgress) {
        return TRUE;
    }

    YuiContext.DisplayResolutionChangeInProgress = TRUE;

    if (DllShell32.pSHAppBarMessage != NULL) {

        AppBar.cbSize = sizeof(AppBar);
        AppBar.hWnd = hWnd;
        AppBar.uCallbackMessage = WM_USER;
        AppBar.uEdge = 3;
        AppBar.rc.left = 0;
        AppBar.rc.top = ScreenHeight - YUI_TASKBAR_HEIGHT;
        AppBar.rc.bottom = ScreenHeight;
        AppBar.rc.right = ScreenWidth;
        AppBar.lParam = TRUE;

        // New
        DllShell32.pSHAppBarMessage(0, &AppBar);

        // Query (request) position
        DllShell32.pSHAppBarMessage(2, &AppBar);

        if (AppBar.rc.bottom - AppBar.rc.top < YUI_TASKBAR_HEIGHT) {
            AppBar.rc.top = AppBar.rc.bottom - YUI_TASKBAR_HEIGHT;
        }

        MoveWindow(hWnd,
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
        MoveWindow(hWnd,
                   0,
                   ScreenHeight - YUI_TASKBAR_HEIGHT,
                   ScreenWidth,
                   YUI_TASKBAR_HEIGHT,
                   TRUE);
    }

    GetClientRect(hWnd, &ClientRect);

    if (YuiContext.hWndClock != NULL) {
        MoveWindow(YuiContext.hWndClock,
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
 Server core systems do not accurately indicate changes including toplevel
 window creation, deletion, or activation.  On these systems Yui is forced
 to inefficiently poll.  Since the registration for notification succeeds,
 the only way to know whether polling is necessary is to inspect the
 product SKU and based on the result, infer whether the notification system
 is buggy or not.

 @return TRUE to indicate that the current system is a server core like
         system, and FALSE if it appears to be a full UI system.
 */
BOOL
YuiIsServerCore(VOID)
{
    DWORD ProductType;

    //
    //  If the API that indicates whether server core is present is not
    //  present, that implies we're not running on server core.
    //
    if (DllKernel32.pGetProductInfo == NULL) {
        return FALSE;
    }

    if (!DllKernel32.pGetProductInfo(6, 1, 0, 0, &ProductType)) {
        return FALSE;
    }

    switch(ProductType) {
        case PRODUCT_DATACENTER_SERVER_CORE:
        case PRODUCT_STANDARD_SERVER_CORE:
        case PRODUCT_ENTERPRISE_SERVER_CORE:
        case PRODUCT_WEB_SERVER_CORE:
        case PRODUCT_DATACENTER_SERVER_CORE_V:
        case PRODUCT_STANDARD_SERVER_CORE_V:
        case PRODUCT_ENTERPRISE_SERVER_CORE_V:
        case PRODUCT_HYPERV:
        case PRODUCT_STORAGE_EXPRESS_SERVER_CORE:
        case PRODUCT_STORAGE_STANDARD_SERVER_CORE:
        case PRODUCT_STORAGE_WORKGROUP_SERVER_CORE:
        case PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE:
        case PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE:
        case PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE:
        case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE:
        case PRODUCT_DATACENTER_A_SERVER_CORE:
        case PRODUCT_STANDARD_A_SERVER_CORE:
        case PRODUCT_DATACENTER_WS_SERVER_CORE:
        case PRODUCT_STANDARD_WS_SERVER_CORE:
        case PRODUCT_DATACENTER_EVALUATION_SERVER_CORE:
        case PRODUCT_STANDARD_EVALUATION_SERVER_CORE:
        case PRODUCT_AZURE_SERVER_CORE:
            return TRUE;
    }

    return FALSE;
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
    SendMessage(YuiContext.hWndStart, BM_SETSTATE, FALSE, 0);
    YuiContext.MenuActive = FALSE;
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

    switch(uMsg) {
        case WM_COMMAND:
            {
                WORD CtrlId;
                CtrlId = LOWORD(wParam);
                if (CtrlId == YUI_START_BUTTON) {
                    YuiDisplayMenu();
                } else {
                    ASSERT(CtrlId >= YUI_FIRST_TASKBAR_BUTTON);
                    YuiTaskbarSwitchToTask(&YuiContext, CtrlId);
                }
            }
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
 A function signature for the YuiShook.dll version of RegisterShellHookWindow,
 which can be used if RegisterShellHookWindow is not present.
 */
typedef
BOOL WINAPI YUISHOOK_REGISTER_SHELL_HOOK_WINDOW(HWND);

/**
 A pointer to a function for the YuiShook.dll version of
 RegisterShellHookWindow, which can be used if RegisterShellHookWindow is not
 present.
 */
typedef YUISHOOK_REGISTER_SHELL_HOOK_WINDOW *PYUISHOOK_REGISTER_SHELL_HOOK_WINDOW;


/**
 Create the taskbar window, start button, and other assorted global elements,
 including populating the start menu and task bar with current state.

 @param Context Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiCreateWindow(
    __in PYUI_ENUM_CONTEXT Context
    )
{
    WNDCLASS wc;
    BOOL Result;
    HDC hDC;
    RECT ClientRect;
    DWORD ScreenHeight;
    DWORD ScreenWidth;

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

    Context->hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                                   _T("YuiWnd"),
                                   _T("Yui"),
                                   WS_POPUP | WS_DLGFRAME | WS_CLIPCHILDREN,
                                   0,
                                   ScreenHeight - YUI_TASKBAR_HEIGHT,
                                   ScreenWidth,
                                   YUI_TASKBAR_HEIGHT,
                                   NULL, NULL, NULL, 0);
    if (Context->hWnd == NULL) {
        return FALSE;
    }

    YuiNotifyResolutionChange(Context->hWnd, ScreenWidth, ScreenHeight);

    GetClientRect(Context->hWnd, &ClientRect);

    hDC = GetWindowDC(Context->hWnd);
    Context->hFont = CreateFont(-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),
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
    ReleaseDC(Context->hWnd, hDC);

    if (Context->hFont == NULL) {
        DestroyWindow(Context->hWnd);
        Context->hWnd = NULL;
        return FALSE;
    }

    Context->hWndStart = CreateWindow(_T("BUTTON"),
                                      _T("Start"),
                                      BS_PUSHBUTTON | BS_CENTER | WS_VISIBLE | WS_CHILD,
                                      1,
                                      1,
                                      YUI_START_BUTTON_WIDTH,
                                      ClientRect.bottom - 2,
                                      Context->hWnd,
                                      (HMENU)(DWORD_PTR)YUI_START_BUTTON,
                                      NULL,
                                      NULL);

    if (Context->hWndStart == NULL) {
        DeleteObject(Context->hFont);
        Context->hFont = NULL;
        DestroyWindow(Context->hWnd);
        Context->hWnd = NULL;
        return FALSE;
    }

    SendMessage(Context->hWndStart, WM_SETFONT, (WPARAM)Context->hFont, MAKELPARAM(TRUE, 0));

    YoriLibInitEmptyString(&Context->ClockDisplayedValue);
    Context->ClockDisplayedValue.StartOfString = Context->ClockDisplayedValueBuffer;
    Context->ClockDisplayedValue.LengthAllocated = sizeof(Context->ClockDisplayedValueBuffer)/sizeof(Context->ClockDisplayedValueBuffer[0]);

    Context->hWndClock = CreateWindowEx(WS_EX_STATICEDGE,
                                        _T("STATIC"),
                                        _T(""),
                                        SS_CENTER | SS_SUNKEN | WS_VISIBLE | WS_CHILD,
                                        ClientRect.right - YUI_CLOCK_WIDTH - 1,
                                        1,
                                        YUI_CLOCK_WIDTH,
                                        ClientRect.bottom - 2,
                                        Context->hWnd,
                                        NULL,
                                        NULL,
                                        NULL);

    if (Context->hWndClock == NULL) {
        DestroyWindow(Context->hWndStart);
        Context->hWndStart = NULL;
        DeleteObject(Context->hFont);
        Context->hFont = NULL;
        DestroyWindow(Context->hWnd);
        Context->hWnd = NULL;
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
        (DllUser32.pRegisterShellHookWindow == NULL || YuiIsServerCore())) {

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

    ShowWindow(Context->hWnd, SW_SHOW);

    /*
    {
        HMODULE Helper;
        Context->ShellHookMsg = RegisterWindowMessage(_T("SHELLHOOK"));
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ShellHookMsg %08x\n"), Context->ShellHookMsg);
        Helper = LoadLibrary(_T("YUISHOOK.DLL"));
        if (Helper != NULL) {
            PYUISHOOK_REGISTER_SHELL_HOOK_WINDOW pYuiShookRegisterShellHookWindow;
            pYuiShookRegisterShellHookWindow = (PYUISHOOK_REGISTER_SHELL_HOOK_WINDOW)GetProcAddress(Helper, "YuiShookRegisterShellHookWindow");
            if (pYuiShookRegisterShellHookWindow != NULL) {
                if (!pYuiShookRegisterShellHookWindow(Context->hWnd)) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("RegisterShellHookWindow failed %i\n"), GetLastError());
                }
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("RegisterShellHookWindow not exported %i\n"), GetLastError());
            }
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YuiShook.dll not loaded %i\n"), GetLastError());
        }
    }
    */

    return TRUE;
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
                YoriLibDisplayMitLicense(_T("2019"));
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

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo)) {
        if (ScreenInfo.dwCursorPosition.X == 0 && ScreenInfo.dwCursorPosition.Y == 0) {
            FreeConsole();
        }
    }

    YuiMenuPopulate(&YuiContext);
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
