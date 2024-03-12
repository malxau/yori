/**
 * @file yui/yui.c
 *
 * Yori shell display lightweight graphical UI
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
    YORI_STRING Text;

    YoriLibInitEmptyString(&Text);
    YoriLibYPrintf(&Text,
                  _T("Yui %i.%02i\n")
#if YORI_BUILD_ID
                  _T("  Build %i\n")
#endif
                  _T("%hs"),
                  YORI_VER_MAJOR,
                  YORI_VER_MINOR,
#if YORI_BUILD_ID
                  YORI_BUILD_ID,
#endif
                  strYuiHelpText);

    if (Text.StartOfString != NULL) {
        MessageBox(NULL, Text.StartOfString, _T("yui"), MB_ICONINFORMATION);
        YoriLibFreeStringContents(&Text);
    }

    return TRUE;
}

/**
 Display license text to the user.

 @param CopyrightYear Pointer to the current copyright year range as text.
 */
BOOL
YuiDisplayMitLicense(
    __in LPCTSTR CopyrightYear
    )
{
    YORI_STRING Text;
    YoriLibInitEmptyString(&Text);
    if (YoriLibMitLicenseText(CopyrightYear, &Text)) {
        MessageBox(NULL, Text.StartOfString, _T("yui"), MB_ICONINFORMATION);
        YoriLibFreeStringContents(&Text);
    }

    return TRUE;
}

/**
 The global application context.
 */
YUI_CONTEXT YuiContext;

/**
 Return the default window procedure for a push button.
 */
WNDPROC
YuiGetDefaultButtonWndProc(VOID)
{
    return YuiContext.DefaultButtonWndProc;
}

/**
 The font name to use in the taskbar and menus at size 8.
 */
#define YUI_SMALL_FONT_NAME _T("MS Sans Serif")

/**
 The font name to use in the taskbar and menus at size 9 or above.
 */
#define YUI_LARGE_FONT_NAME _T("Tahoma")

/**
 The base height of the taskbar, in pixels.
 */
#define YUI_BASE_TASKBAR_HEIGHT (28)

/**
 The base width of each taskbar button, in pixels.
 */
#define YUI_BASE_TASKBAR_BUTTON_WIDTH (160)

/**
 Query the taskbar height for this system.  The taskbar is generally 30 pixels
 or 34 pixels (based on small icon size) but can scale upwards slightly on
 larger displays.


 @return The height of the taskbar, in pixels.
 */
WORD
YuiGetTaskbarHeight(
    __in PYUI_MONITOR YuiMonitor
    )
{
    WORD TaskbarHeight;
    WORD ButtonPadding;
    WORD ExtraPixels;

    YuiMonitor->TaskbarPaddingVertical = 2;
    YuiMonitor->ControlBorderWidth = 1;
    ButtonPadding = 3;

    //
    //  MSFIX Seems like time for per-monitor DPI scaling
    //

    if (YuiMonitor->YuiContext->SmallTaskbarIconHeight >= 28) {
        YuiMonitor->ControlBorderWidth = (WORD)(YuiMonitor->ControlBorderWidth + 2);
    } else if (YuiMonitor->YuiContext->SmallTaskbarIconHeight >= 20) {
        YuiMonitor->ControlBorderWidth = (WORD)(YuiMonitor->ControlBorderWidth + 1);
    } else if (YuiMonitor->ScreenHeight > 800) {

        //
        //  If no DPI scaling is in effect, see if we should scale a bit
        //  anyway due to the number of vertical pixels.  This adds 1.25% of
        //  pixels above 800.
        //

        ExtraPixels = (WORD)((YuiMonitor->ScreenHeight - 800) / 80);

        //
        //  Increasing border width takes 4px, so only do it if we have a
        //  little extra.  This logic means we'll use a larger font at 4px,
        //  and at 7px we'll use that same larger font with larger borders,
        //  and at 10px font size goes up again.
        //

        if (ExtraPixels >= 7) {
            YuiMonitor->ControlBorderWidth = (WORD)(YuiMonitor->ControlBorderWidth + 1);
            ExtraPixels = (WORD)(ExtraPixels - 4);
        } else if (ExtraPixels >= 4) {
            ExtraPixels = 4;
        }

        ButtonPadding = (WORD)(ButtonPadding + ExtraPixels / 2);
    }

    YuiMonitor->TaskbarPaddingVertical = (WORD)(YuiMonitor->TaskbarPaddingVertical + YuiMonitor->ControlBorderWidth);
    YuiMonitor->TaskbarPaddingHorizontal = YuiMonitor->TaskbarPaddingVertical;

    //
    //  First, set the taskbar to be based on the small icon size, with extra
    //  space for its chrome.  The small icon size is a function of the DPI
    //  setting of the system, and keeping the icon the same size prevents
    //  ugly scaling.
    //

    TaskbarHeight = (WORD)(YuiMonitor->YuiContext->SmallTaskbarIconHeight +
                           2 * ButtonPadding +
                           2 * YuiMonitor->ControlBorderWidth +
                           2 * YuiMonitor->TaskbarPaddingVertical);

    return TaskbarHeight;
}

/**
 Query the maximum taskbar button width for this system.  The taskbar
 generally uses 180 pixel buttons but can scale upwards slightly on larger
 displays.

 @param YuiMonitor Pointer to the monitor context describing the screen
        resolution of the monitor.

 @return The maximum width of a taskbar button, in pixels.
 */
WORD
YuiGetTaskbarMaximumButtonWidth(
    __in PYUI_MONITOR YuiMonitor
    )
{
    DWORD TaskbarButtonWidth;

    TaskbarButtonWidth = YUI_BASE_TASKBAR_BUTTON_WIDTH;

    //
    //  Give an extra 50px for every font point size
    //

    if (YuiMonitor->FontSize > YUI_BASE_FONT_SIZE) {
        TaskbarButtonWidth = TaskbarButtonWidth + 50 * (YuiMonitor->FontSize - YUI_BASE_FONT_SIZE);
    }

    //
    //  Give 3% of any horizontal pixels above 800
    //

    if (YuiMonitor->ScreenWidth > 800) {
        TaskbarButtonWidth = TaskbarButtonWidth + (YuiMonitor->ScreenWidth - 800) / 30;
    }

    return (WORD)TaskbarButtonWidth;
}

/**
 Get the text to include in the start button (aka "Start").

 @param Text On successful completion, populated with the text to display
        in the start button.
 */
VOID
YuiStartGetText(
    __out PYORI_STRING Text
    )
{
    YoriLibConstantString(Text, _T("Start"));
}

/**
 Draw the start button, including icon and text.

 @param YuiMonitor Pointer to the monitor context.

 @param DrawItemStruct Pointer to a structure describing the button to
        redraw, including its bounds and device context.
 */
VOID
YuiStartDrawButton(
    __in PYUI_MONITOR YuiMonitor,
    __in PDRAWITEMSTRUCT DrawItemStruct
    )
{
    YORI_STRING Text;

    YuiStartGetText(&Text);
    YuiDrawButton(YuiMonitor,
                  DrawItemStruct,
                  YuiContext.MenuActive,
                  FALSE,
                  FALSE,
                  YuiContext.StartIcon,
                  &Text,
                  TRUE);
    YoriLibFreeStringContents(&Text);
}

/**
 A custom window procedure used by the start button.  This is a form of
 subclass that enables us to filter the messages processed by the regular
 button implementation.

 @param hWnd Window handle to the button.

 @param uMsg The message identifier.

 @param wParam The first parameter associated with the window message.

 @param lParam The second parameter associated with the window message.

 @return A value which depends on the type of message being processed.
 */
LRESULT CALLBACK
YuiStartButtonWndProc(
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
 Indicates that the screen resolution has changed and the taskbar needs to be
 repositioned accordingly.

 @param YuiMonitor The monitor structure that should be refreshed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiNotifyResolutionChange(
    __in PYUI_MONITOR YuiMonitor
    )
{
    RECT ClientRect;
    HDC hDC;
    HFONT hFont;
    HFONT hBoldFont;
    LPCTSTR FontName;
    YORI_STRING Text;
    WORD ButtonHeight;
    INT ScaledFontSize;
    HWND hWnd;

    if (YuiContext.DisplayResolutionChangeInProgress) {
        return TRUE;
    }

    hWnd = YuiMonitor->hWndTaskbar;

    YuiContext.DisplayResolutionChangeInProgress = TRUE;

    YuiMonitor->TaskbarHeight = YuiGetTaskbarHeight(YuiMonitor);

    FontName = YUI_SMALL_FONT_NAME;

    ButtonHeight = (WORD)(YuiMonitor->TaskbarHeight -
                          2 * YuiMonitor->TaskbarPaddingVertical -
                          2 * YuiMonitor->ControlBorderWidth);

    YuiMonitor->FontSize = YUI_BASE_FONT_SIZE;
    YuiMonitor->ExtraPixelsAboveText = 1;
    if (ButtonHeight > 24) {
        YuiMonitor->FontSize = (WORD)(YuiMonitor->FontSize + (ButtonHeight - 24) / 2);
    }

    if (YuiMonitor->FontSize > YUI_BASE_FONT_SIZE) {
        FontName = YUI_LARGE_FONT_NAME;
        YuiMonitor->ExtraPixelsAboveText = 0;
    }

    //
    //  The font was scaled based on button size above, and should not be
    //  scaled based on DPI again or it'd get scaled up twice.  Calculate
    //  the font size based on a fixed 96 DPI display.
    //

    ScaledFontSize = YuiMonitor->FontSize;
    ScaledFontSize = ScaledFontSize * 96 / 72;

    YuiMonitor->MaximumTaskbarButtonWidth = YuiGetTaskbarMaximumButtonWidth(YuiMonitor);

    hDC = GetWindowDC(hWnd);
    hFont = CreateFont(-ScaledFontSize,
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
                       FontName);

    hBoldFont = CreateFont(-ScaledFontSize,
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
                           FontName);

    if (hFont != NULL) {

        if (YuiMonitor->hFont != NULL) {
            DeleteObject(YuiMonitor->hFont);
        }
        YuiMonitor->hFont = hFont;

        if (YuiMonitor->hWndTaskbar != NULL) {
            SendMessage(YuiMonitor->hWndTaskbar,
                        WM_SETFONT,
                        (WPARAM)YuiMonitor->hFont,
                        MAKELPARAM(TRUE, 0));
        }

        if (YuiMonitor->hWndBattery != NULL) {
            SendMessage(YuiMonitor->hWndBattery,
                        WM_SETFONT,
                        (WPARAM)YuiMonitor->hFont,
                        MAKELPARAM(TRUE, 0));
        }

        if (YuiMonitor->hWndClock != NULL) {
            SendMessage(YuiMonitor->hWndClock,
                        WM_SETFONT,
                        (WPARAM)YuiMonitor->hFont,
                        MAKELPARAM(TRUE, 0));
        }
    }

    if (hBoldFont != NULL) {
        if (YuiMonitor->hBoldFont != NULL) {
            DeleteObject(YuiMonitor->hBoldFont);
        }
        YuiMonitor->hBoldFont = hBoldFont;

        if (YuiMonitor->hWndStart != NULL) {
            SendMessage(YuiMonitor->hWndStart,
                        WM_SETFONT,
                        (WPARAM)YuiMonitor->hBoldFont,
                        MAKELPARAM(TRUE, 0));
        }
    }

    if (YuiMonitor->FontSize > YUI_BASE_FONT_SIZE) {
        YuiContext.TallIconPadding = 6;
        YuiContext.ShortIconPadding = 4;
    } else {
        YuiContext.TallIconPadding = 4;
        YuiContext.ShortIconPadding = 3;
    }

    YuiContext.TallMenuHeight = (WORD)(YuiContext.TallIconHeight + 2 * YuiContext.TallIconPadding);
    YuiContext.ShortMenuHeight = (WORD)(YuiContext.SmallStartIconHeight + 2 * YuiContext.ShortIconPadding);
    YuiContext.MenuSeperatorHeight = 12;

    YuiStartGetText(&Text);
    YuiMonitor->StartButtonWidth = (WORD)(YuiContext.SmallTaskbarIconWidth +
                                          4 * YuiMonitor->ControlBorderWidth +
                                          YuiDrawGetTextWidth(YuiMonitor, TRUE, &Text) +
                                          16);

    YoriLibFreeStringContents(&Text);
    YoriLibConstantString(&Text, _T("88:88 PM"));
    YuiMonitor->ClockWidth = (WORD)(YuiDrawGetTextWidth(YuiMonitor, FALSE, &Text) * 5 / 4 + 2 * YuiMonitor->ControlBorderWidth);
    YoriLibConstantString(&Text, _T("100%"));
    YuiMonitor->BatteryWidth = (WORD)(YuiDrawGetTextWidth(YuiMonitor, FALSE, &Text) * 5 / 4 + 2 * YuiMonitor->ControlBorderWidth);
    YuiMonitor->CalendarCellWidth = (WORD)(YUI_CALENDAR_CELL_WIDTH + (YuiMonitor->FontSize - YUI_BASE_FONT_SIZE) * 5);
    YuiMonitor->CalendarCellHeight = (WORD)(YUI_CALENDAR_CELL_HEIGHT + (YuiMonitor->FontSize - YUI_BASE_FONT_SIZE) * 4);

#if DBG
    if (YuiContext.DebugLogEnabled) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                     _T("Based on screen size %ix%i, font size %i, TaskbarHeight %i, ButtonHeight %i\n")
                     _T("StartButtonWidth %i, TaskbarButtonWidth %i\n"),
                     YuiMonitor->ScreenWidth,
                     YuiMonitor->ScreenHeight,
                     YuiMonitor->FontSize,
                     YuiMonitor->TaskbarHeight,
                     ButtonHeight,
                     YuiMonitor->StartButtonWidth,
                     YuiMonitor->MaximumTaskbarButtonWidth);
    }
#endif

    ReleaseDC(hWnd, hDC);

    DllUser32.pMoveWindow(hWnd,
                          YuiMonitor->ScreenLeft,
                          YuiMonitor->ScreenTop + YuiMonitor->ScreenHeight - YuiMonitor->TaskbarHeight,
                          YuiMonitor->ScreenWidth,
                          YuiMonitor->TaskbarHeight,
                          TRUE);

    if (YuiContext.hWndDesktop != NULL) {
        DllUser32.pMoveWindow(YuiContext.hWndDesktop,
                              YuiContext.VirtualScreenLeft,
                              YuiContext.VirtualScreenTop,
                              YuiContext.VirtualScreenWidth,
                              YuiContext.VirtualScreenHeight,
                              TRUE);
    }

    InvalidateRect(hWnd, NULL, TRUE);

    DllUser32.pGetClientRect(hWnd, &ClientRect);

    if (YuiMonitor->hWndBattery != NULL) {
        DllUser32.pMoveWindow(YuiMonitor->hWndBattery,
                              ClientRect.right - YuiMonitor->ClockWidth - 3 - YuiMonitor->BatteryWidth - YuiMonitor->TaskbarPaddingHorizontal,
                              YuiMonitor->TaskbarPaddingVertical,
                              YuiMonitor->BatteryWidth,
                              ClientRect.bottom - 2 * YuiMonitor->TaskbarPaddingVertical,
                              TRUE);
    }

    if (YuiMonitor->hWndClock != NULL) {
        DllUser32.pMoveWindow(YuiMonitor->hWndClock,
                              ClientRect.right - YuiMonitor->ClockWidth - YuiMonitor->TaskbarPaddingHorizontal,
                              YuiMonitor->TaskbarPaddingVertical,
                              YuiMonitor->ClockWidth,
                              ClientRect.bottom - 2 * YuiMonitor->TaskbarPaddingVertical,
                              TRUE);
    }

    if (YuiMonitor->hWndStart != NULL) {
        DllUser32.pMoveWindow(YuiMonitor->hWndStart,
                              YuiMonitor->TaskbarPaddingHorizontal,
                              YuiMonitor->TaskbarPaddingVertical,
                              YuiMonitor->StartButtonWidth,
                              ClientRect.bottom - 2 * YuiMonitor->TaskbarPaddingVertical,
                              TRUE);
    }

    YuiMonitor->StartLeftOffset = YuiMonitor->TaskbarPaddingHorizontal;
    YuiMonitor->StartRightOffset = (WORD)(YuiMonitor->StartLeftOffset + YuiMonitor->StartButtonWidth);

    YuiMonitor->LeftmostTaskbarOffset = (WORD)(YuiMonitor->TaskbarPaddingHorizontal + YuiMonitor->StartButtonWidth + YuiMonitor->ControlBorderWidth * 2);
    YuiMonitor->RightmostTaskbarOffset = (WORD)(YuiMonitor->ControlBorderWidth + YuiMonitor->ClockWidth + YuiMonitor->TaskbarPaddingHorizontal);

    YuiTaskbarNotifyResolutionChange(YuiMonitor);

    YuiContext.DisplayResolutionChangeInProgress = FALSE;
    YuiMonitor->DimensionsChanged = FALSE;

    return TRUE;
}

/**
 Display the start menu and perform any action requested.

 @return TRUE to indicate the start menu was displayed successfully.
 */
BOOL
YuiDisplayMenu(
    __in PYUI_MONITOR YuiMonitor
    )
{
    if (YuiContext.MenuActive) {
        return TRUE;
    }
    YuiTaskbarSuppressFullscreenHiding(YuiMonitor);
    YuiContext.MenuActive = TRUE;
    SendMessage(YuiMonitor->hWndStart, BM_SETSTATE, TRUE, 0);
    YuiMenuDisplayAndExecute(YuiMonitor, YuiMonitor->hWndTaskbar);
    if (YuiContext.MenuActive) {
        YuiContext.MenuActive = FALSE;
        SendMessage(YuiMonitor->hWndStart, BM_SETSTATE, FALSE, 0);
    }
    return TRUE;
}

/**
 Display the taskbar context menu and perform any action requested.

 @param YuiMonitor Pointer to the monitor context.

 @param CursorX The horizontal position of the mouse cursor.

 @param CursorY The vertical position of the mouse cursor.

 @return TRUE to indicate the context menu was displayed successfully.
 */
BOOL
YuiDisplayContextMenu(
    __in PYUI_MONITOR YuiMonitor,
    __in DWORD CursorX,
    __in DWORD CursorY
    )
{
    if (YuiContext.MenuActive) {
        return FALSE;
    }
    YuiTaskbarSuppressFullscreenHiding(YuiMonitor);
    YuiMenuDisplayContext(YuiMonitor, YuiMonitor->hWndTaskbar, CursorX, CursorY);
    return TRUE;
}

#if DBG
/**
 Display a debug string corresponding to a shell hook message.

 @param szMessage A constant string for the message name.

 @param wParam The wParam for the message.

 @param lParam The lParam for the message.  If this is known to be an hWnd,
        the window title is also included.
 */
VOID
YuiPrintShellHookDebugMessage(
    __in LPCTSTR szMessage,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    HWND hWnd;
    YORI_STRING WindowTitle;

    if (!YuiContext.DebugLogEnabled) {
        return;
    }

    hWnd = NULL;

    switch(wParam) {
        case HSHELL_WINDOWACTIVATED:
        case HSHELL_RUDEAPPACTIVATED:
        case HSHELL_WINDOWCREATED:
        case HSHELL_WINDOWDESTROYED:
        case HSHELL_REDRAW:
        case HSHELL_FLASH:
        case HSHELL_MONITORCHANGED:
            hWnd = (HWND)lParam;
            break;
    }

    YoriLibInitEmptyString(&WindowTitle);

    if (hWnd != NULL) {
        YORI_ALLOC_SIZE_T CharsNeeded;

        CharsNeeded = (YORI_ALLOC_SIZE_T)GetWindowTextLength(hWnd);
        if (YoriLibAllocateString(&WindowTitle, CharsNeeded + 1)) {
            WindowTitle.LengthInChars = (YORI_ALLOC_SIZE_T)GetWindowText(hWnd, WindowTitle.StartOfString, WindowTitle.LengthAllocated);
        }
    }

    if (wParam <= 0xffff && lParam <= 0xffffffff) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-24s %04x %08x %y\n"), szMessage, wParam, lParam, &WindowTitle);
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-24s %016llx %016llx %y\n"), szMessage, wParam, lParam, &WindowTitle);
    }

    YoriLibFreeStringContents(&WindowTitle);
}
#endif

/**
 The main window procedure which processes messages sent to the desktop
 window.

 @param hwnd The window handle.

 @param uMsg The message identifier.

 @param wParam The first parameter associated with the window message.

 @param lParam The second parameter associated with the window message.

 @return A value which depends on the type of message being processed.
 */
LRESULT CALLBACK
YuiDesktopWindowProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
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
YuiTaskbarWindowProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PYUI_MONITOR YuiMonitor;
    WORD CtrlId;

    switch(uMsg) {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {

                YuiMonitor = YuiMonitorFromTaskbarHwnd(&YuiContext, hwnd);
                ASSERT(YuiMonitor != NULL);

                //
                //  Explorer won't allow windows without focus to change the
                //  work area.  Since this user interaction gives us that
                //  power, fix things now if needed.
                //

                YuiResetWorkArea(YuiMonitor, FALSE);

                CtrlId = LOWORD(wParam);
                if (CtrlId == YUI_START_BUTTON) {
                    YuiDisplayMenu(YuiMonitor);
                } else if (CtrlId == YUI_CLOCK_DISPLAY) {
                    YuiCalendar(YuiMonitor);
                } else if (CtrlId == YUI_BATTERY_DISPLAY) {
                    YuiClockDisplayBatteryInfo(YuiMonitor);
                } else {
                    ASSERT(CtrlId >= YUI_FIRST_TASKBAR_BUTTON);
                    if (GetKeyState(VK_SHIFT) < 0) {
                        YuiTaskbarLaunchNewTask(YuiMonitor, CtrlId);
                    } else {
                        YuiTaskbarSwitchToTask(YuiMonitor, CtrlId);
                    }
                }
            }
            break;
        case WM_LBUTTONDOWN:
            {
                SHORT XPos;

                YuiMonitor = YuiMonitorFromTaskbarHwnd(&YuiContext, hwnd);
                ASSERT(YuiMonitor != NULL);

                //
                //  Explorer won't allow windows without focus to change the
                //  work area.  Since this user interaction gives us that
                //  power, fix things now if needed.
                //

                YuiResetWorkArea(YuiMonitor, FALSE);

                //
                //  Get the signed horizontal position.  Note that because
                //  nonclient clicks are translated to client area, these
                //  values can be negative.
                //

                XPos = (SHORT)(LOWORD(lParam));
                if (YuiTaskbarIsPositionOverStart(YuiMonitor, XPos)) {
                    YuiDisplayMenu(YuiMonitor);
                } else {
                    CtrlId = YuiTaskbarFindByOffset(YuiMonitor, XPos);
                    if (CtrlId >= YUI_FIRST_TASKBAR_BUTTON) {
                        if (GetKeyState(VK_SHIFT) < 0) {
                            YuiTaskbarLaunchNewTask(YuiMonitor, CtrlId);
                        } else {
                            YuiTaskbarSwitchToTask(YuiMonitor, CtrlId);
                        }
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
                    YuiClockUpdate(&YuiContext);
                    break;
            }
            return 0;
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_INITMENUPOPUP:
            {
                HWND hwndMenu;
                DWORD Style;

                //
                //  Attempt to clear the window shadow.  This is a property
                //  of the window class, not the window.  Doing it here allows
                //  this to happen before the menu window is displayed.
                //

                hwndMenu = FindWindow(_T("#32768"), NULL);
                if (hwndMenu != NULL) {
                    Style = (DWORD)GetClassLongPtr(hwndMenu, GCL_STYLE);
                    if ((Style & CS_DROPSHADOW) != 0) {
                        Style = Style & ~(CS_DROPSHADOW);
                        SetClassLongPtr(hwndMenu, GCL_STYLE, Style);
                    }
                }
            }
            break;
        case WM_CONTEXTMENU:
            {
                SHORT XPos;
                SHORT YPos;
                SHORT RightmostTaskbarOffset;
                RECT TaskbarWindowRect;

                YuiMonitor = YuiMonitorFromTaskbarHwnd(&YuiContext, hwnd);
                ASSERT(YuiMonitor != NULL);

                XPos = (SHORT)(LOWORD(lParam));
                YPos = (SHORT)(HIWORD(lParam));

                //
                //  Check that the mouse is within the taskbar.  This message
                //  can also be sent if the user right clicks a menu, which is
                //  not currently implemented.
                //

                DllUser32.pGetWindowRect(YuiMonitor->hWndTaskbar, &TaskbarWindowRect);
                if (XPos >= TaskbarWindowRect.left &&
                    XPos <= TaskbarWindowRect.right &&
                    YPos >= TaskbarWindowRect.top &&
                    YPos <= TaskbarWindowRect.bottom) {

                    //
                    //  Convert coordinates into client coordinates
                    //

                    XPos = (SHORT)(XPos - TaskbarWindowRect.left);
                    YPos = (SHORT)(YPos - TaskbarWindowRect.top);

                    TaskbarWindowRect.right = TaskbarWindowRect.right - TaskbarWindowRect.left;
                    TaskbarWindowRect.bottom = TaskbarWindowRect.bottom - TaskbarWindowRect.top;

                    RightmostTaskbarOffset = (SHORT)(TaskbarWindowRect.right - YuiMonitor->RightmostTaskbarOffset);
                    if (XPos >= (SHORT)YuiMonitor->LeftmostTaskbarOffset &&
                        XPos <= RightmostTaskbarOffset) {

                        CtrlId = YuiTaskbarFindByOffset(YuiMonitor, XPos);
                        if (CtrlId < YUI_FIRST_TASKBAR_BUTTON) {
                            YuiDisplayContextMenu(YuiMonitor, LOWORD(lParam), HIWORD(lParam));
                        } else {
                            YuiTaskbarDisplayContextMenuForTask(YuiMonitor, CtrlId, LOWORD(lParam), HIWORD(lParam));
                        }
                    }
                }
            }
            break;
        case WM_HOTKEY:
#if DBG
            if (YuiContext.DebugLogEnabled) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("WM_HOTKEY               %016llx %016llx\n"), wParam, lParam);
            }
#endif
            YuiMonitor = YuiMonitorFromTaskbarHwnd(&YuiContext, hwnd);
            ASSERT(YuiMonitor != NULL);
            DllUser32.pSetForegroundWindow(hwnd);
            if ((DWORD)wParam == YUI_MENU_START) {
                YuiDisplayMenu(YuiMonitor);
            } else {
                YuiMenuExecuteById(YuiMonitor, (DWORD)wParam);
            }
            PostMessage(hwnd, WM_NULL, 0, 0);
            break;
        case WM_DISPLAYCHANGE:
#if DBG
            if (YuiContext.DebugLogEnabled) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Display change          %016llx %016llx\n"), wParam, lParam);
            }
#endif
            YuiRefreshMonitors(&YuiContext);
            break;
        case WM_WTSSESSION_CHANGE:
#if DBG
            if (YuiContext.DebugLogEnabled) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Session change          %016llx %016llx\n"), wParam, lParam);
            }
#endif
            YuiRefreshMonitors(&YuiContext);
            break;
        case WM_MEASUREITEM:
            YuiMonitor = YuiMonitorFromTaskbarHwnd(&YuiContext, hwnd);
            ASSERT(YuiMonitor != NULL);
            return YuiDrawMeasureMenuItem(YuiMonitor, (LPMEASUREITEMSTRUCT)lParam);
        case WM_DRAWITEM:
            {
                PDRAWITEMSTRUCT DrawItemStruct;

                YuiMonitor = YuiMonitorFromTaskbarHwnd(&YuiContext, hwnd);
                ASSERT(YuiMonitor != NULL);

                DrawItemStruct = (PDRAWITEMSTRUCT)lParam;

                CtrlId = LOWORD(wParam);
                if (CtrlId == YUI_START_BUTTON) {
                    YuiStartDrawButton(YuiMonitor, DrawItemStruct);
                } else if (CtrlId == YUI_BATTERY_DISPLAY) {
                    YuiTaskbarDrawStatic(YuiMonitor,
                                         DrawItemStruct,
                                         &YuiContext.BatteryDisplayedValue);
                } else if (CtrlId == YUI_CLOCK_DISPLAY) {
                    YuiTaskbarDrawStatic(YuiMonitor,
                                         DrawItemStruct,
                                         &YuiContext.ClockDisplayedValue);
                } else if (CtrlId != 0) {
                    ASSERT(CtrlId >= YUI_FIRST_TASKBAR_BUTTON);
                    YuiTaskbarDrawButton(YuiMonitor, CtrlId, DrawItemStruct);
                } else {
                    YuiDrawMenuItem(YuiMonitor, DrawItemStruct);
                }
            }
            break;
        case WM_CTLCOLORBTN:
            return (LRESULT)YuiContext.BackgroundBrush;
            break;
        case WM_PAINT:
            YuiMonitor = YuiMonitorFromTaskbarHwnd(&YuiContext, hwnd);
            ASSERT(YuiMonitor != NULL);
            if (hwnd == YuiMonitor->hWndTaskbar) {
                if (YuiDrawWindowFrame(YuiMonitor, YuiMonitor->hWndTaskbar, (HDC)wParam)) {
                    return 0;
                }
            }
            break;
#if DBG
        case WM_WININICHANGE:
            if (YuiContext.DebugLogEnabled) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("WM_SETTINGCHANGE        %016llx %016llx\n"), wParam, lParam);
            }

            //
            //  Ideally we'd detect a work area change and reset here, but
            //  doing that ends up creating a loop with explorer.  It looks
            //  like explorer thinks a window that doesn't have input focus
            //  doesn't get to change this, but a window with input focus
            //  can.
            //

            break;
#endif

        default:
            if (uMsg == YuiContext.ShellHookMsg) {
                switch(wParam) {
                    case HSHELL_WINDOWACTIVATED:
                    case HSHELL_RUDEAPPACTIVATED:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_WINDOWACTIVATED"), wParam, lParam);
#endif
                        YuiTaskbarNotifyActivateWindow(&YuiContext, (HWND)lParam);
                        break;
                    case HSHELL_WINDOWCREATED:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_WINDOWCREATED"), wParam, lParam);
#endif
                        YuiTaskbarNotifyNewWindow(&YuiContext, (HWND)lParam);
                        break;
                    case HSHELL_WINDOWDESTROYED:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_WINDOWDESTROYED"), wParam, lParam);
#endif
                        YuiTaskbarNotifyDestroyWindow(&YuiContext, (HWND)lParam);
                        break;
                    case HSHELL_REDRAW:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_REDRAW"), wParam, lParam);
#endif
                        YuiTaskbarNotifyTitleChange(&YuiContext, (HWND)lParam);
                        break;
                    case HSHELL_FLASH:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_FLASH"), wParam, lParam);
#endif
                        YuiTaskbarNotifyFlash(&YuiContext, (HWND)lParam);
                        break;
                    case HSHELL_GETMINRECT:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_GETMINRECT"), wParam, lParam);
#endif
                        break;
                    case HSHELL_TASKMAN:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_TASKMAN"), wParam, lParam);
#endif
                        break;
                    case HSHELL_LANGUAGE:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_LANGUAGE"), wParam, lParam);
#endif
                        break;
                    case HSHELL_SYSMENU:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_SYSMENU"), wParam, lParam);
#endif
                        break;
                    case HSHELL_ENDTASK:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_ENDTASK"), wParam, lParam);
#endif
                        break;
                    case HSHELL_APPCOMMAND:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_APPCOMMAND"), wParam, lParam);
#endif
                        break;
                    case HSHELL_WINDOWREPLACED:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_WINDOWREPLACED"), wParam, lParam);
#endif
                        YuiTaskbarNotifyDestroyWindow(&YuiContext, (HWND)lParam);
                        break;
                    case HSHELL_WINDOWREPLACING:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_WINDOWREPLACING"), wParam, lParam);
#endif
                        YuiTaskbarNotifyNewWindow(&YuiContext, (HWND)lParam);
                        break;
                    case HSHELL_MONITORCHANGED:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("HSHELL_MONITORCHANGED"), wParam, lParam);
#endif
                        YuiTaskbarNotifyMonitorChanged(&YuiContext, (HWND)lParam);
                        break;
                    default:
#if DBG
                        YuiPrintShellHookDebugMessage(_T("Unhandled HookMsg"), wParam, lParam);
#endif
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
    PYORI_LIST_ENTRY ListEntry;
    PYUI_RECENT_CHILD_PROCESS ChildProcess;
    PYUI_MONITOR YuiMonitor;
    PYUI_MONITOR NextMonitor;
    PYUI_EXPLORER_TASKBAR ExplorerTaskbar;
    HWND hWndPrimaryTaskbar;

    YuiMenuWaitForBackgroundReload(&YuiContext);
    YuiMenuFreeAll(&YuiContext);
    YuiTaskbarFreeButtons(&YuiContext);
    YuiMenuCleanupContext();
    YuiIconCacheCleanupContext();

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiContext.RecentProcessList, ListEntry);
    while (ListEntry != NULL) {
        ChildProcess = CONTAINING_RECORD(ListEntry, YUI_RECENT_CHILD_PROCESS, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&YuiContext.RecentProcessList, ListEntry);
        ASSERT(ChildProcess->TaskbarButtonCount == 0);
        YoriLibRemoveListItem(&ChildProcess->ListEntry);
        YoriLibFreeStringContents(&ChildProcess->FilePath);
        YoriLibDereference(ChildProcess);
        YuiContext.RecentProcessCount--;
    }
    ASSERT(YuiContext.RecentProcessCount == 0);

    for (Count = 0; Count < sizeof(YuiContext.StartChangeNotifications)/sizeof(YuiContext.StartChangeNotifications[0]); Count++) {
        if (YuiContext.StartChangeNotifications[Count] != NULL) {
            FindCloseChangeNotification(YuiContext.StartChangeNotifications[Count]);
            YuiContext.StartChangeNotifications[Count] = NULL;
        }
    }

    hWndPrimaryTaskbar = YuiContext.PrimaryMon->hWndTaskbar;

    if (DllWtsApi32.pWTSUnRegisterSessionNotification &&
        YuiContext.RegisteredSessionNotifications) {

        DllWtsApi32.pWTSUnRegisterSessionNotification(hWndPrimaryTaskbar);
    }

    if (YuiContext.RunHotKeyRegistered) {
        UnregisterHotKey(hWndPrimaryTaskbar, YUI_MENU_RUN);
    }

    if (YuiContext.StartHotKeyRegistered) {
        UnregisterHotKey(hWndPrimaryTaskbar, YUI_START_BUTTON);
    }

    if (YuiContext.ClockTimerId != 0) {
        KillTimer(hWndPrimaryTaskbar, YUI_CLOCK_TIMER);
        YuiContext.ClockTimerId = 0;
    }

    if (YuiContext.SyncTimerId != 0) {
        KillTimer(hWndPrimaryTaskbar, YUI_WINDOW_POLL_TIMER);
        YuiContext.SyncTimerId = 0;
    }

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(&YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
        NextMonitor = YuiGetNextMonitor(&YuiContext, YuiMonitor);
        YuiCleanupMonitor(YuiMonitor);
        YuiMonitor = NextMonitor;
    }

    if (YuiContext.hWndDesktop != NULL) {
        DestroyWindow(YuiContext.hWndDesktop);
        YuiContext.hWndDesktop = NULL;
    }

    if (YuiContext.StartIcon) {
        DestroyIcon(YuiContext.StartIcon);
        YuiContext.StartIcon = NULL;
    }

    if (YuiContext.BackgroundBrush) {
        DeleteObject(YuiContext.BackgroundBrush);
        YuiContext.BackgroundBrush = NULL;
    }

    if (YuiContext.SavedMinimizedMetrics.cbSize != 0) {
        SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(YORI_MINIMIZEDMETRICS), &YuiContext.SavedMinimizedMetrics, 0);
    }

    if (YuiContext.OleInitialized) {
        YuiOleUninitialize();
    }

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(&YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
        NextMonitor = YuiGetNextMonitor(&YuiContext, YuiMonitor);
        YoriLibFree(YuiMonitor);
        YuiMonitor = NextMonitor;
    }

    ListEntry = YoriLibGetNextListEntry(&YuiContext.ExplorerTaskbarList, NULL);
    while (ListEntry != NULL) {
        ExplorerTaskbar = CONTAINING_RECORD(ListEntry, YUI_EXPLORER_TASKBAR, ListEntry);
        YoriLibRemoveListItem(ListEntry);
        if (ExplorerTaskbar->Hidden) {
            DllUser32.pShowWindow(ExplorerTaskbar->hWnd, SW_SHOW);
        }
        YoriLibFree(ExplorerTaskbar);
        ListEntry = YoriLibGetNextListEntry(&YuiContext.ExplorerTaskbarList, NULL);
    }
}

/**
 Create a taskbar and its children for a particular monitor.

 @param YuiMonitor Pointer to the monitor to create windows on.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiInitializeMonitor(
    __in PYUI_MONITOR YuiMonitor
    )
{
    PYUI_CONTEXT Context;
    DWORD NoActivate;
    RECT ClientRect;

    Context = YuiMonitor->YuiContext;
    YuiMonitor->TaskbarHeight = YuiGetTaskbarHeight(YuiMonitor);

#if DBG
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Creating taskbar on monitor %p\n"), YuiMonitor->MonitorHandle);
#endif

    NoActivate = WS_EX_NOACTIVATE;

    //
    //  Try to create the taskbar window.  If this fails, remove
    //  WS_EX_NOACTIVATE and try again.
    //

    while(TRUE) {
        YuiMonitor->hWndTaskbar = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | NoActivate,
                                                 YUI_TASKBAR_CLASS,
                                                 _T("Yui"),
                                                 WS_POPUP | WS_CLIPCHILDREN,
                                                 YuiMonitor->ScreenLeft,
                                                 YuiMonitor->ScreenTop + YuiMonitor->ScreenHeight - YuiMonitor->TaskbarHeight,
                                                 YuiMonitor->ScreenWidth,
                                                 YuiMonitor->TaskbarHeight,
                                                 NULL, NULL, NULL, 0);
        if (YuiMonitor->hWndTaskbar != NULL) {
            break;
        }

        if (NoActivate != 0) {
            NoActivate = 0;
        } else {
            return FALSE;
        }
    }

    //
    //  Attempt to set up drag and drop.
    //

    if (Context->OleInitialized) {
        YuiMonitor->DropHandle = YuiRegisterDropWindow(YuiMonitor, YuiMonitor->hWndTaskbar);

#if DBG
        if (YuiMonitor->DropHandle == NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("RegisterDropWindow failed for monitor %p\n"), YuiMonitor->MonitorHandle);
        }
#endif
    }

    YuiNotifyResolutionChange(YuiMonitor);
    if (YuiMonitor->hFont == NULL || YuiMonitor->hBoldFont == NULL) {
        return FALSE;
    }

    SendMessage(YuiMonitor->hWndTaskbar, WM_SETFONT, (WPARAM)YuiMonitor->hFont, MAKELPARAM(TRUE, 0));

    DllUser32.pGetClientRect(YuiMonitor->hWndTaskbar, &ClientRect);

    YuiMonitor->hWndStart = CreateWindow(_T("BUTTON"),
                                         _T("Start"),
                                         BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                                         YuiMonitor->TaskbarPaddingHorizontal,
                                         YuiMonitor->TaskbarPaddingVertical,
                                         YuiMonitor->StartButtonWidth,
                                         ClientRect.bottom - 2 * YuiMonitor->TaskbarPaddingVertical,
                                         YuiMonitor->hWndTaskbar,
                                         (HMENU)(DWORD_PTR)YUI_START_BUTTON,
                                         NULL,
                                         NULL);

    if (YuiMonitor->hWndStart == NULL) {
        return FALSE;
    }

    if (Context->DefaultButtonWndProc == NULL) {
        Context->DefaultButtonWndProc = (WNDPROC)GetWindowLongPtr(YuiMonitor->hWndStart, GWLP_WNDPROC);
        if (Context->DefaultButtonWndProc == NULL) {
            return FALSE;
        }
    }
    SetWindowLongPtr(YuiMonitor->hWndStart, GWLP_WNDPROC, (LONG_PTR)YuiStartButtonWndProc);

    SendMessage(YuiMonitor->hWndStart, WM_SETFONT, (WPARAM)YuiMonitor->hBoldFont, MAKELPARAM(TRUE, 0));

    YuiMonitor->hWndClock = CreateWindowEx(0,
                                           _T("STATIC"),
                                           _T(""),
                                           SS_OWNERDRAW | SS_NOTIFY | WS_VISIBLE | WS_CHILD,
                                           ClientRect.right - YuiMonitor->ClockWidth - YuiMonitor->TaskbarPaddingHorizontal,
                                           YuiMonitor->TaskbarPaddingVertical,
                                           YuiMonitor->ClockWidth,
                                           ClientRect.bottom - 2 * YuiMonitor->TaskbarPaddingVertical,
                                           YuiMonitor->hWndTaskbar,
                                           (HMENU)(DWORD_PTR)YUI_CLOCK_DISPLAY,
                                           NULL,
                                           NULL);

    if (YuiMonitor->hWndClock == NULL) {
        YuiCleanupGlobalState();
        return FALSE;
    }

    SendMessage(YuiMonitor->hWndClock, WM_SETFONT, (WPARAM)YuiMonitor->hFont, MAKELPARAM(TRUE, 0));

    if (Context->DisplayBattery) {
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
            YuiCleanupGlobalState();
            return FALSE;
        }

        SendMessage(YuiMonitor->hWndBattery,
                    WM_SETFONT,
                    (WPARAM)YuiMonitor->hFont,
                    MAKELPARAM(TRUE, 0));
        YuiMonitor->RightmostTaskbarOffset = (WORD)(1 + YuiMonitor->ClockWidth + 3 + YuiMonitor->BatteryWidth + 1);
    }

    return TRUE;
}

/**
 Create the taskbar window, start button, and other assorted global elements,
 including populating the start menu and task bar with current state.

 @param Context Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiInitializeApplication(
    __in PYUI_CONTEXT Context
    )
{
    WNDCLASS wc;
    BOOL Result;
    YORI_MINIMIZEDMETRICS MinimizedMetrics;
    PYUI_MONITOR YuiMonitor;
    DWORD NoActivate;
    DWORD MajorVersion;
    DWORD MinorVersion;
    DWORD BuildNumber;
    HWND hWnd;

    //
    //  Multiple monitor functions have existed since Windows 2000, but only
    //  Windows 8 indicates window movement across monitors which is necessary
    //  to support multiple taskbars.
    //

    YoriLibGetOsVersion(&MajorVersion, &MinorVersion, &BuildNumber);
    if (DllUser32.pGetMonitorInfoW != NULL &&
        DllUser32.pEnumDisplayMonitors != NULL &&
        DllUser32.pMonitorFromWindow != NULL &&
        BuildNumber >= 9600) {

        Context->MultiTaskbarsSupported = TRUE;
    }

    //
    //  Initialize OLE early with the options we want to support dragging
    //  and dropping.
    //

    if (YuiOleInitialize()) {
        Context->OleInitialized = TRUE;
    }

    //
    //  Explorer or other shells will register as a magic "Taskman" window.
    //  If this isn't set for the session, assume we must be the main shell.
    //

    if (DllUser32.pGetTaskmanWindow != NULL &&
        DllUser32.pGetShellWindow != NULL &&
        DllUser32.pGetTaskmanWindow() == NULL &&
        DllUser32.pGetShellWindow() == NULL) {

        Context->LoginShell = TRUE;
    }
    if (!YuiIconCacheInitializeContext(Context)) {
        return FALSE;
    }

    Context->SmallTaskbarIconWidth = (WORD)GetSystemMetrics(SM_CXSMICON);
    Context->SmallTaskbarIconHeight = (WORD)GetSystemMetrics(SM_CYSMICON);
    Context->TallIconWidth = (WORD)GetSystemMetrics(SM_CXICON);
    Context->TallIconHeight = (WORD)GetSystemMetrics(SM_CYICON);

    //
    //  Adjust the small start menu icon to the next multiple of 4.
    //  This is just a visual improvement in case the small system icon size
    //  is irregular.
    //

    Context->SmallStartIconWidth = Context->SmallTaskbarIconWidth;
    Context->SmallStartIconHeight = Context->SmallTaskbarIconHeight;
    if (Context->SmallStartIconWidth == Context->SmallStartIconHeight) {
        Context->SmallStartIconWidth = (WORD)((Context->SmallStartIconWidth + 3) & ~(3));
        Context->SmallStartIconHeight = (WORD)((Context->SmallStartIconHeight + 3) & ~(3));
    }

#if DBG
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                  _T("Large system icon %ix%i, Taskbar icon %ix%i, Small start icon %ix%i\n"),
                  Context->TallIconWidth, Context->TallIconHeight,
                  Context->SmallTaskbarIconWidth, Context->SmallTaskbarIconHeight,
                  Context->SmallStartIconWidth, Context->SmallStartIconHeight);
#endif


    if (!YuiMenuInitializeContext(Context)) {
        YuiCleanupGlobalState();
        return FALSE;
    }

    if (!YuiMenuMonitorFileSystemChanges(Context)) {
        YuiCleanupGlobalState();
        return FALSE;
    }
    YuiMenuPopulateInBackground(Context);

    Context->BackgroundBrush = CreateSolidBrush(YuiGetWindowBackgroundColor());

    wc.style = 0;
    wc.lpfnWndProc = YuiTaskbarWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = NULL;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(YuiGetWindowBackgroundColor());
    wc.lpszMenuName = NULL;
    wc.lpszClassName = YUI_TASKBAR_CLASS;

    Result = RegisterClass(&wc);
    if (!Result) {
        YuiCleanupGlobalState();
        return FALSE;
    }

    wc.lpfnWndProc = YuiCalendarWindowProc;
    wc.hbrBackground = CreateSolidBrush(YuiGetWindowBackgroundColor());
    wc.lpszClassName = YUI_CALENDAR_CLASS;

    Result = RegisterClass(&wc);
    if (!Result) {
        YuiCleanupGlobalState();
        return FALSE;
    }

    wc.lpfnWndProc = YuiWifiWindowProc;
    wc.hbrBackground = CreateSolidBrush(YuiGetWindowBackgroundColor());
    wc.lpszClassName = YUI_WIFI_CLASS;

    Result = RegisterClass(&wc);
    if (!Result) {
        YuiCleanupGlobalState();
        return FALSE;
    }

    //
    //  If the OS would benefit from a shell desktop window, register a
    //  class for one.
    //

    if (Context->LoginShell && DllUser32.pSetShellWindow != NULL) {
        wc.lpfnWndProc = YuiDesktopWindowProc;
        wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
        wc.lpszClassName = YUI_DESKTOP_CLASS;
        Result = RegisterClass(&wc);
        if (!Result) {
            YuiCleanupGlobalState();
            return FALSE;
        }
    }

    if (DllUser32.pLoadImageW == NULL) {
        Context->StartIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(STARTICON));
    } else {
        Context->StartIcon = DllUser32.pLoadImageW(GetModuleHandle(NULL),
                                                   MAKEINTRESOURCE(STARTICON),
                                                   IMAGE_ICON,
                                                   Context->SmallStartIconWidth,
                                                   Context->SmallStartIconHeight,
                                                   0);
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

    YoriLibInitEmptyString(&Context->ClockDisplayedValue);
    Context->ClockDisplayedValue.StartOfString = Context->ClockDisplayedValueBuffer;
    Context->ClockDisplayedValue.LengthAllocated = sizeof(Context->ClockDisplayedValueBuffer)/sizeof(Context->ClockDisplayedValueBuffer[0]);

    YoriLibInitEmptyString(&Context->BatteryDisplayedValue);
    Context->BatteryDisplayedValue.StartOfString = Context->BatteryDisplayedValueBuffer;
    Context->BatteryDisplayedValue.LengthAllocated = sizeof(Context->BatteryDisplayedValueBuffer)/sizeof(Context->BatteryDisplayedValueBuffer[0]);

    if (DllKernel32.pGetSystemPowerStatus != NULL) {
        YORI_SYSTEM_POWER_STATUS PowerStatus;

        if (DllKernel32.pGetSystemPowerStatus(&PowerStatus) &&
            (PowerStatus.BatteryFlag & YORI_BATTERY_FLAG_NO_BATTERY) == 0) {

            Context->DisplayBattery = TRUE;
        }
    }

    YuiInitializeMonitors(Context);
    NoActivate = WS_EX_NOACTIVATE;

    //
    //  If this is the first shell instance, and the OS supports shell
    //  desktop windows, create a desktop.  If the OS doesn't support
    //  shell desktop windows, it really doesn't need one - the user32
    //  desktop should display the background correctly.
    //

    if (Context->LoginShell && DllUser32.pSetShellWindow != NULL) {

        while(TRUE) {
            Context->hWndDesktop = CreateWindowEx(NoActivate,
                                                  YUI_DESKTOP_CLASS,
                                                  _T(""),
                                                  WS_POPUP | WS_VISIBLE,
                                                  Context->VirtualScreenLeft,
                                                  Context->VirtualScreenTop,
                                                  Context->VirtualScreenWidth,
                                                  Context->VirtualScreenHeight,
                                                  NULL, NULL, NULL, 0);

            if (Context->hWndDesktop != NULL) {
                break;
            }

            if (NoActivate != 0) {
                NoActivate = 0;
            } else {
                YuiCleanupGlobalState();
                return FALSE;
            }
        }

        //
        //  This tells the window manager to render the desktop underneath
        //  every other window.
        //

        DllUser32.pSetShellWindow(Context->hWndDesktop);
    }

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(&YuiContext, YuiMonitor);
    while (YuiMonitor != NULL) {
        if (!YuiInitializeMonitor(YuiMonitor)) {
            YuiCleanupGlobalState();
            return FALSE;
        }
        YuiMonitor = YuiGetNextMonitor(&YuiContext, YuiMonitor);
    }

    //
    //  Grab a random window for application wide state initialization
    //

    hWnd = Context->PrimaryMon->hWndTaskbar;

    //
    //  Register as the Task Manager window.  This is required for
    //  notification about window state changes.
    //

    if (DllUser32.pSetTaskmanWindow != NULL) {
        DllUser32.pSetTaskmanWindow(hWnd);
    }


    if (DllWtsApi32.pWTSRegisterSessionNotification) {
        if (DllWtsApi32.pWTSRegisterSessionNotification(hWnd, 0)) {
            Context->RegisteredSessionNotifications = TRUE;
        }
    }

    Context->ClockTimerId = SetTimer(hWnd, YUI_CLOCK_TIMER, 5000, NULL);

    YuiTaskbarPopulateWindows(Context);

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
        if (!DllUser32.pRegisterShellHookWindow(hWnd)) {
            Context->TaskbarRefreshFrequency = 250;
        }
    }

    //
    //  If the refresh frequency is specified, or the OS doesn't support
    //  notifications, or setting up notifications failed, set up polling
    //  now.
    //

    if (Context->TaskbarRefreshFrequency != 0) {
        Context->SyncTimerId = SetTimer(hWnd,
                                        YUI_WINDOW_POLL_TIMER,
                                        Context->TaskbarRefreshFrequency,
                                        NULL);
    }

    if (Context->LoginShell) {
        if (RegisterHotKey(hWnd, YUI_MENU_RUN, MOD_WIN, 'R')) {
            Context->RunHotKeyRegistered = TRUE;
        }
        if (RegisterHotKey(hWnd, YUI_MENU_START, MOD_WIN, VK_LWIN)) {
            Context->StartHotKeyRegistered = TRUE;
        }
    }

    YuiClockUpdate(Context);

    YuiAdjustAllWorkAreasAndHideExplorer(Context);

    YuiMonitor = NULL;
    YuiMonitor = YuiGetNextMonitor(Context, YuiMonitor);
    while (YuiMonitor != NULL) {
        DllUser32.pShowWindow(YuiMonitor->hWndTaskbar, SW_SHOW);
        YuiMonitor = YuiGetNextMonitor(Context, YuiMonitor);
    }

    return TRUE;
}

#if DBG
/**
 Launch the current winlogon shell.  This code runs after tearing down all
 state from this program so that a new shell can be launched that will
 perceive itself as freshly launched.
 */
VOID
YuiLaunchWinlogonShell(VOID)
{
    HKEY hKey;
    DWORD Err;
    DWORD LengthRequired;
    YORI_STRING ExistingValue;
    YORI_STRING ValueName;
    YORI_STRING KeyName;

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegOpenKeyExW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllShell32.pShellExecuteW == NULL) {

        return;
    }

    YoriLibConstantString(&KeyName, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"));
    YoriLibConstantString(&ValueName, _T("Shell"));

    Err = DllAdvApi32.pRegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                     KeyName.StartOfString,
                                     0,
                                     KEY_QUERY_VALUE,
                                     &hKey);

    if (Err != ERROR_SUCCESS) {
        return;
    }

    LengthRequired = 0;
    YoriLibInitEmptyString(&ExistingValue);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, ValueName.StartOfString, NULL, NULL, NULL, &LengthRequired);
    if (Err == ERROR_MORE_DATA || LengthRequired > 0) {
        DWORD CharsRequired = LengthRequired / sizeof(TCHAR) + 1;
        if (!YoriLibIsSizeAllocatable(CharsRequired)) {
            DllAdvApi32.pRegCloseKey(hKey);
            return;
        }
        if (!YoriLibAllocateString(&ExistingValue, (YORI_ALLOC_SIZE_T)CharsRequired)) {
            DllAdvApi32.pRegCloseKey(hKey);
            return;
        }

        Err = DllAdvApi32.pRegQueryValueExW(hKey, ValueName.StartOfString, NULL, NULL, (LPBYTE)ExistingValue.StartOfString, &LengthRequired);
        if (Err != ERROR_SUCCESS) {
            YoriLibFreeStringContents(&ExistingValue);
            DllAdvApi32.pRegCloseKey(hKey);
            return;
        }

        ExistingValue.LengthInChars = (YORI_ALLOC_SIZE_T)(LengthRequired / sizeof(TCHAR) - 1);

        DllShell32.pShellExecuteW(NULL, NULL, ExistingValue.StartOfString, NULL, NULL, SW_SHOWNORMAL);

        YoriLibFreeStringContents(&ExistingValue);
    }

    DllAdvApi32.pRegCloseKey(hKey);
}
#endif


/**
 The main entrypoint for the yui cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero on success, nonzero on failure.
 */
DWORD
ymain(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;

    ZeroMemory(&YuiContext, sizeof(YuiContext));
    YoriLibInitializeListHead(&YuiContext.RecentProcessList);
    YoriLibInitializeListHead(&YuiContext.MonitorList);
    YoriLibInitializeListHead(&YuiContext.ExplorerTaskbarList);

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
                YuiDisplayMitLicense(_T("2019-2024"));
                return EXIT_SUCCESS;
#if DBG
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                if (AllocConsole()) {
                    SetConsoleTitle(_T("Yui debug log"));
                    YuiContext.DebugLogEnabled = TRUE;
                }
                ArgumentUnderstood = TRUE;
#endif
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YORI_STRING Err;
            YoriLibInitEmptyString(&Err);
            YoriLibYPrintf(&Err, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
            if (Err.StartOfString != NULL) {
                MessageBox(NULL, Err.StartOfString, _T("yui"), MB_ICONSTOP);
                YoriLibFreeStringContents(&Err);
            }
        }
    }

    if (DllShell32.pSHGetSpecialFolderPathW == NULL) {
        YoriLibLoadShfolderFunctions();
    }

    if (DllUser32.pDrawIconEx == NULL ||
        DllUser32.pGetClientRect == NULL ||
        DllUser32.pGetWindowRect == NULL ||
        DllUser32.pMoveWindow == NULL ||
        DllUser32.pSetForegroundWindow == NULL ||
        DllUser32.pSetWindowTextW == NULL ||
        DllUser32.pShowWindow == NULL ||
        (DllShell32.pSHGetSpecialFolderPathW == NULL && DllShfolder.pSHGetFolderPathW == NULL)) {

        MessageBox(NULL, _T("yui: OS support not present"), _T("yui"), MB_ICONSTOP);
        return EXIT_FAILURE;
    }

    if (!YuiInitializeApplication(&YuiContext)) {
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

#if DBG
    if (YuiContext.LaunchWinlogonShell) {
        YuiLaunchWinlogonShell();
    }
#endif

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
