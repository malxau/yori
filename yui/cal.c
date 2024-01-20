/**
 * @file yui/cal.c
 *
 * Yori shell graphical calendar
 *
 * Copyright (c) 2023 Malcolm J. Smith
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
 Hack - keep a global pointer to the application context so the message pump
 can find it.  Ideally this would be part of extra window state or somesuch.
 */
PYUI_CONTEXT YuiCalYuiContext;

/**
 The number of days in each month.  This is a static set, but due to leap
 years this static set is re-evaluated for the second month at run time.
 */
WORD CalStaticDaysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/**
 A list of names for each month.
 */
LPTSTR CalMonthNames[12] = {_T("January"),
                            _T("February"),
                            _T("March"),
                            _T("April"),
                            _T("May"),
                            _T("June"),
                            _T("July"),
                            _T("August"),
                            _T("September"),
                            _T("October"),
                            _T("November"),
                            _T("December")};

/**
 A list of names for each day.
 */
LPTSTR CalDayNames[7] = {_T("Sunday"),
                         _T("Monday"),
                         _T("Tuesday"),
                         _T("Wednesday"),
                         _T("Thursday"),
                         _T("Friday"),
                         _T("Saturday")};


/**
 The number of days per week.  This can't really change because this program
 is depending on SYSTEMTIME's concept of which day in a week a particular day
 falls.
 */
#define CAL_DAYS_PER_WEEK  (7)

/**
 The maximum number of days per month.  This is used to calculate the maximum
 number of rows needed to display a given month.
 */
#define CAL_MAX_DAYS_PER_MONTH   (31)

/**
 The number of rows needed to display a month.
 */
#define CAL_ROWS_PER_MONTH       ((CAL_MAX_DAYS_PER_MONTH + 2 * CAL_DAYS_PER_WEEK - 1) / CAL_DAYS_PER_WEEK)

/**
 Return the width to use in pixels for each calendar cell.

 @return The number of pixels wide for each cell in the calendar view.
 */
WORD
YuiCalendarGetCellWidth(VOID)
{
    return YuiCalYuiContext->CalendarCellWidth;
}

/**
 Return the height to use in pixels for each calendar cell.

 @return The number of pixels in height for each cell in the calendar view.
 */
WORD
YuiCalendarGetCellHeight(VOID)
{
    return YuiCalYuiContext->CalendarCellHeight;
}

/**
 Return the padding to use between each horizontal calendar cell.

 @return The number of pixels of padding between each horizontal calendar
         cell.
 */
WORD
YuiCalendarGetCellHorizPadding(VOID)
{
    return 2;
}

/**
 Return the padding to use between each vertical calendar cell.

 @return The number of pixels of padding between each vertical calendar
         cell.
 */
WORD
YuiCalendarGetCellVertPadding(VOID)
{
    return 2;
}

/**
 Return the padding to use on each side of the calendar window.

 @return The number of pixels of padding on the left and right of the
         calendar window.
 */
WORD
YuiCalendarGetWindowHorizPadding(VOID)
{
    return 8;
}

/**
 Return the padding to use on the top and bottom of the calendar window.

 @return The number of pixels of padding on the top and bottom of the
         calendar window.
 */
WORD
YuiCalendarGetWindowVertPadding(VOID)
{
    return 8;
}

/**
 Return the rectangle to use to contain a specified calendar cell.
 
 @param ClientRect The dimensions of the client area of the window.

 @param Row The vertical coordinate of the calendar cell.

 @param Column The horizontal coordinate of the calendar cell.

 @param TextRect On successful completion, the rectangle to display the cell
        into.
 */
VOID
YuiCalendarTextRectForCell(
    __in PRECT ClientRect,
    __in WORD Row,
    __in WORD Column,
    __out PRECT TextRect
    )
{
    TextRect->top = ClientRect->top + 
                    YuiCalendarGetWindowVertPadding() +
                    (Row * (YuiCalendarGetCellHeight() + YuiCalendarGetCellVertPadding()));
    TextRect->left = ClientRect->left +
                     YuiCalendarGetWindowHorizPadding() +
                     (Column * (YuiCalendarGetCellWidth() + YuiCalendarGetCellHorizPadding()));
    TextRect->bottom = TextRect->top + YuiCalendarGetCellHeight();
    TextRect->right = TextRect->left + YuiCalendarGetCellWidth();
}

/**
 Display the calendar for a specified calendar month.

 @param hWnd The window to render the calendar into.

 @param Year The year number to display the calendar for.

 @param Month The month number to display the calendar for.

 @param Today Optionally points to a systemtime structure indicating the
        the current date.  If this is specified, the current day is highlighted
        within the calendar.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiCalendarOutputMonth(
    __in HWND hWnd,
    __in WORD Year,
    __in WORD Month,
    __in_opt PSYSTEMTIME Today
    )
{
    SYSTEMTIME SysTimeAtYearStart;
    FILETIME FileTime;
    WORD LineCount;
    WORD DayCount;
    WORD ThisDayNumber;
    WORD MonthIndex;
    YORI_ALLOC_SIZE_T MonthNameLength;
    WORD RealDaysInMonth[12];
    WORD DayIndexAtStartOfMonth[12];
    RECT ClientRect;
    RECT TextRect;
    HDC hDC;
    PAINTSTRUCT PaintStruct;
    WORD Row;
    DWORD TextFlags;
    HGDIOBJ OldObject;
    TCHAR Str[3];
    COLORREF BackColor;
    COLORREF ForeColor;

    //
    //  If the window has no repainting to do, stop.
    //

    if (!GetUpdateRect(hWnd, &ClientRect, FALSE)) {
        return FALSE;
    }

    ZeroMemory(&SysTimeAtYearStart, sizeof(SysTimeAtYearStart));

    SysTimeAtYearStart.wYear = Year;
    SysTimeAtYearStart.wMonth = 1;
    SysTimeAtYearStart.wDay = 1;

    if (!SystemTimeToFileTime(&SysTimeAtYearStart, &FileTime)) {
        return FALSE;
    }
    if (!FileTimeToSystemTime(&FileTime, &SysTimeAtYearStart)) {
        return FALSE;
    }

    BackColor = YuiGetWindowBackgroundColor();
    ForeColor = YuiGetMenuTextColor();

    //
    //  If it does, redraw everything.
    //

    hDC = BeginPaint(hWnd, &PaintStruct);
    if (hDC == NULL) {
        return FALSE;
    }

    GetClientRect(hWnd, &ClientRect);
    YuiDrawThreeDBox(hDC, &ClientRect, FALSE);
    OldObject = SelectObject(hDC, YuiCalYuiContext->hFont);

    SetBkColor(hDC, BackColor);
    SetTextColor(hDC, ForeColor);

    //
    //  Calculate the number of days in each month and which day of the week
    //  each month starts on.
    //

    for (MonthIndex = 0; MonthIndex < 12; MonthIndex++) {
        RealDaysInMonth[MonthIndex] = CalStaticDaysInMonth[MonthIndex];
        if (MonthIndex == 1 && (Year % 4) == 0 && (Year % 100) != 0) {
            RealDaysInMonth[MonthIndex] = 29;
        }
        if (MonthIndex == 0) {
            DayIndexAtStartOfMonth[MonthIndex] = SysTimeAtYearStart.wDayOfWeek;
        } else {
            DayIndexAtStartOfMonth[MonthIndex] = (WORD)((DayIndexAtStartOfMonth[MonthIndex - 1] + RealDaysInMonth[MonthIndex - 1]) % 7);
        }
    }

    Row = 0;

    //
    //  Print the name of the month
    //

    MonthNameLength = (YORI_ALLOC_SIZE_T)_tcslen(CalMonthNames[Month]);

    TextRect.top = ClientRect.top + 
                   YuiCalendarGetWindowVertPadding();
    TextRect.left = ClientRect.left +
                    YuiCalendarGetWindowHorizPadding();
    TextRect.bottom = TextRect.top + YuiCalendarGetCellHeight();
    TextRect.right = ClientRect.right -
                     YuiCalendarGetWindowHorizPadding();
    TextFlags = DT_SINGLELINE | DT_VCENTER | DT_CENTER;

    DrawText(hDC, CalMonthNames[Month], MonthNameLength, &TextRect, TextFlags);

    //
    //  Print the day names
    //

    Row = 1;
    for (DayCount = 0; DayCount < CAL_DAYS_PER_WEEK; DayCount++) {
        YuiCalendarTextRectForCell(&ClientRect, Row, DayCount, &TextRect);
        YoriLibSPrintf(Str, _T("%2s"), CalDayNames[DayCount]);
        DrawText(hDC, Str, 2, &TextRect, TextFlags);
    }


    //
    //  Print the day numbers for each month.
    //

    Row = 2;
    for (LineCount = 0; LineCount < CAL_ROWS_PER_MONTH; LineCount++) {
        for (DayCount = 0; DayCount < CAL_DAYS_PER_WEEK; DayCount++) {
            if (LineCount > 0 || DayCount >= DayIndexAtStartOfMonth[Month]) {
                ThisDayNumber = (WORD)(LineCount * CAL_DAYS_PER_WEEK + DayCount - DayIndexAtStartOfMonth[Month] + 1);
                if (ThisDayNumber <= RealDaysInMonth[Month]) {
                    YuiCalendarTextRectForCell(&ClientRect, (WORD)(Row + LineCount), DayCount, &TextRect);
                    YoriLibSPrintf(Str, _T("%02i"), ThisDayNumber);

                    if (Today != NULL &&
                        Today->wYear == Year &&
                        Today->wMonth == (Month + 1) &&
                        Today->wDay == ThisDayNumber) {

                        COLORREF ActiveBackColor;
                        COLORREF ActiveForeColor;
                        HBRUSH BackBrush;

                        ActiveBackColor = YuiGetMenuSelectedBackgroundColor();
                        ActiveForeColor = YuiGetMenuSelectedTextColor();
                        BackBrush = CreateSolidBrush(ActiveBackColor);

                        FillRect(hDC, &TextRect, BackBrush);

                        DeleteObject(BackBrush);

                        SetBkColor(hDC, ActiveBackColor);
                        SetTextColor(hDC, ActiveForeColor);
                        DrawText(hDC, Str, 2, &TextRect, TextFlags);
                        SetBkColor(hDC, BackColor);
                        SetTextColor(hDC, ForeColor);
                    } else {
                        DrawText(hDC, Str, 2, &TextRect, TextFlags);
                    }
                }
            }
        }
    }

    SelectObject(hDC, OldObject);
    EndPaint(hWnd, &PaintStruct);

    return TRUE;
}

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
YuiCalendarWindowProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch(uMsg) {
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                DestroyWindow(hwnd);
            }
            break;
        case WM_ACTIVATEAPP:
            if ((BOOL)wParam == FALSE) {
                DestroyWindow(hwnd);
            }
            break;
        case WM_PAINT:
            {
                SYSTEMTIME Now;
                GetSystemTime(&Now);
                YuiCalendarOutputMonth(hwnd,
                                       Now.wYear,
                                       (WORD)(Now.wMonth - 1),
                                       &Now);
            }
            break;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 Display the Yui calendar window.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiCalendar(
    __in PYUI_CONTEXT YuiContext
    )
{
    HWND hWnd;
    WORD WindowWidth;
    WORD WindowHeight;
    WORD CellWidth;
    WORD CellHeight;
    WORD CellHorizPadding;
    WORD CellVertPadding;
    WORD WindowHorizPadding;
    WORD WindowVertPadding;

    YuiCalYuiContext = YuiContext;

    CellWidth = YuiCalendarGetCellWidth();
    CellHeight = YuiCalendarGetCellHeight();
    CellHorizPadding = YuiCalendarGetCellHorizPadding();
    CellVertPadding = YuiCalendarGetCellVertPadding();
    WindowHorizPadding = YuiCalendarGetWindowHorizPadding();
    WindowVertPadding = YuiCalendarGetWindowVertPadding();

    WindowWidth = (WORD)(CellWidth * CAL_DAYS_PER_WEEK +
                         CellHorizPadding * (CAL_DAYS_PER_WEEK - 1) +
                         WindowHorizPadding * 2);
    WindowHeight = (WORD)(CellHeight * (CAL_ROWS_PER_MONTH + 2) +
                         CellVertPadding * (CAL_ROWS_PER_MONTH + 1) +
                         WindowVertPadding * 2);

    hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                          YUI_CALENDAR_CLASS,
                          _T(""),
                          WS_POPUP | WS_CLIPCHILDREN,
                          YuiContext->ScreenWidth - WindowWidth,
                          YuiContext->ScreenHeight - YuiContext->TaskbarHeight - WindowHeight,
                          WindowWidth,
                          WindowHeight,
                          NULL, NULL, NULL, 0);
    if (hWnd == NULL) {
        return;
    }

    ShowWindow(hWnd, SW_SHOW);
    DllUser32.pSetForegroundWindow(hWnd);
    SetFocus(hWnd);
}


// vim:sw=4:ts=4:et:
