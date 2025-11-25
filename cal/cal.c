/**
 * @file cal/cal.c
 *
 * Yori shell display calendar
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

#include <yoripch.h>
#include <yorilib.h>

/**
 Help text to display to the user.
 */
const
CHAR strCalHelpText[] =
        "\n"
        "Display a calendar.\n"
        "\n"
        "CAL [-license] [year]\n";

/**
 Display usage text to the user.
 */
BOOL
CalHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Cal %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strCalHelpText);
    return TRUE;
}

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
 The number of rows of months.
 */
#define CAL_ROWS_OF_MONTHS (4)

/**
 The number of months displayed in each row.
 */
#define CAL_MONTHS_PER_ROW (3)

/**
 The number of days per week.  This can't really change because this program
 is depending on SYSTEMTIME's concept of which day in a week a particular day
 falls.
 */
#define CAL_DAYS_PER_WEEK  (7)

/**
 The number of characters to use on the console when displaying each day.
 */
#define CAL_CHARS_PER_DAY  (3)

/**
 The number of characters to display between months on the same row.
 */
#define CAL_CHARS_BETWEEN_MONTHS (3)

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
 Display the calendar for a specified calendar month.

 @param Year The year number to display the calendar for.

 @param Month The month number to display the calendar for.

 @param Today Optionally points to a systemtime structure indicating the
        the current date.  If this is specified, the current day is highlighted
        within the calendar.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CalOutputCalendarForMonth(
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
    YORI_ALLOC_SIZE_T DesiredOffset;
    YORI_ALLOC_SIZE_T CurrentOffset;
    WORD MonthIndex;
    YORI_ALLOC_SIZE_T MonthNameLength;
    WORD RealDaysInMonth[12];
    WORD DayIndexAtStartOfMonth[12];
    YORI_STRING Line;

    ZeroMemory(&SysTimeAtYearStart, (DWORD)sizeof(SysTimeAtYearStart));

    //
    //  To buffer a single console row, we need one month, which consists
    //  of a week worth of days.  We can also highlight the current day,
    //  which takes four chars to start highlighting and four to end
    //  highlighting, and a newline with a NULL terminator.
    //

    DesiredOffset = (CAL_DAYS_PER_WEEK * CAL_CHARS_PER_DAY) + 4 * 2 + 2;

    if (!YoriLibAllocateString(&Line, DesiredOffset)) {
        return FALSE;
    }

    SysTimeAtYearStart.wYear = Year;
    SysTimeAtYearStart.wMonth = 1;
    SysTimeAtYearStart.wDay = 1;

    if (!SystemTimeToFileTime(&SysTimeAtYearStart, &FileTime)) {
        return FALSE;
    }
    if (!FileTimeToSystemTime(&FileTime, &SysTimeAtYearStart)) {
        return FALSE;
    }

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

    //
    //  Print the name of the month
    //

    CurrentOffset = 0;
    MonthNameLength = (YORI_ALLOC_SIZE_T)_tcslen(CalMonthNames[Month]);

    DesiredOffset = ((CAL_DAYS_PER_WEEK * CAL_CHARS_PER_DAY) - MonthNameLength) / 2;
    while(CurrentOffset < DesiredOffset) {
        Line.StartOfString[CurrentOffset] = ' ';
        Line.LengthInChars += 1;
        CurrentOffset++;
    }

    memcpy(&Line.StartOfString[CurrentOffset], CalMonthNames[Month], MonthNameLength * sizeof(TCHAR));
    CurrentOffset += MonthNameLength;
    Line.LengthInChars = Line.LengthInChars + MonthNameLength;
    Line.StartOfString[Line.LengthInChars] = '\n';
    Line.LengthInChars++;

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Line);
    Line.LengthInChars = 0;

    //
    //  Print the day names
    //

    for (DayCount = 0; DayCount < CAL_DAYS_PER_WEEK; DayCount++) {
        YoriLibSPrintf(&Line.StartOfString[Line.LengthInChars], _T("%2s"), CalDayNames[DayCount]);
        Line.LengthInChars += 2;
        CurrentOffset = 2;
        DesiredOffset = CAL_CHARS_PER_DAY;
        while (CurrentOffset < DesiredOffset) {
            Line.StartOfString[Line.LengthInChars] = ' ';
            Line.LengthInChars++;
            CurrentOffset++;
        }
    }

    Line.StartOfString[Line.LengthInChars] = '\n';
    Line.LengthInChars++;
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Line);
    Line.LengthInChars = 0;

    //
    //  Print the day numbers for each month.
    //

    for (LineCount = 0; LineCount < CAL_ROWS_PER_MONTH; LineCount++) {
        for (DayCount = 0; DayCount < CAL_DAYS_PER_WEEK; DayCount++) {
            DesiredOffset = CAL_CHARS_PER_DAY;
            CurrentOffset = 0;
            if (LineCount > 0 || DayCount >= DayIndexAtStartOfMonth[Month]) {
                ThisDayNumber = (WORD)(LineCount * CAL_DAYS_PER_WEEK + DayCount - DayIndexAtStartOfMonth[Month] + 1);
                if (ThisDayNumber <= RealDaysInMonth[Month]) {
                    if (Today != NULL &&
                        Today->wYear == Year &&
                        Today->wMonth == (Month + 1) &&
                        Today->wDay == ThisDayNumber) {

                        YoriLibSPrintf(&Line.StartOfString[Line.LengthInChars], _T("%c[7m%02i%c[7m"), 27, ThisDayNumber, 27);
                        Line.LengthInChars += 4 + 2 + 4;
                    } else {
                        YoriLibSPrintf(&Line.StartOfString[Line.LengthInChars], _T("%02i"), ThisDayNumber);
                        Line.LengthInChars +=  2;
                    }
                    CurrentOffset = 2;
                }
            }
            while (CurrentOffset < DesiredOffset) {
                Line.StartOfString[Line.LengthInChars] = ' ';
                Line.LengthInChars++;
                CurrentOffset++;
            }
        }

        Line.StartOfString[Line.LengthInChars] = '\n';
        Line.LengthInChars++;
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Line);
        Line.LengthInChars = 0;
    }

    YoriLibFreeStringContents(&Line);

    return TRUE;
}

/**
 Display the calendar for a specified calendar year.

 @param Year The year number to display the calendar for.

 @param Today Optionally points to a systemtime structure indicating the
        the current date.  If this is specified, the current day is highlighted
        within the calendar.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CalOutputCalendarForYear(
    __in WORD Year,
    __in_opt PSYSTEMTIME Today
    )
{
    SYSTEMTIME SysTimeAtYearStart;
    FILETIME FileTime;
    WORD LineCount;
    WORD DayCount;
    WORD ThisDayNumber;
    WORD Quarter;
    WORD Month;
    WORD MonthIndex;
    YORI_ALLOC_SIZE_T DesiredOffset;
    YORI_ALLOC_SIZE_T CurrentOffset;
    YORI_ALLOC_SIZE_T MonthNameLength;
    WORD RealDaysInMonth[12];
    WORD DayIndexAtStartOfMonth[12];
    YORI_STRING Line;

    ZeroMemory(&SysTimeAtYearStart, (DWORD)sizeof(SysTimeAtYearStart));

    //
    //  To buffer a single console row, we need N months, each of which
    //  consists of a week worth of days, plus space between months.  We can
    //  also highlight the current day, which takes four chars to start
    //  highlighting and four to end highlighting, and a newline with a
    //  NULL terminator.
    //

    DesiredOffset = CAL_MONTHS_PER_ROW * (CAL_DAYS_PER_WEEK * CAL_CHARS_PER_DAY + CAL_CHARS_BETWEEN_MONTHS) + 4 * 2 + 2;

    if (!YoriLibAllocateString(&Line, DesiredOffset)) {
        return FALSE;
    }

    SysTimeAtYearStart.wYear = Year;
    SysTimeAtYearStart.wMonth = 1;
    SysTimeAtYearStart.wDay = 1;

    if (!SystemTimeToFileTime(&SysTimeAtYearStart, &FileTime)) {
        return FALSE;
    }
    if (!FileTimeToSystemTime(&FileTime, &SysTimeAtYearStart)) {
        return FALSE;
    }

    //
    //  Calculate the number of days in each month and which day of the week
    //  each month starts on.
    //

    for (Month = 0; Month < 12; Month++) {
        RealDaysInMonth[Month] = CalStaticDaysInMonth[Month];
        if (Month == 1 && (Year % 4) == 0 && (Year % 100) != 0) {
            RealDaysInMonth[Month] = 29;
        }
        if (Month == 0) {
            DayIndexAtStartOfMonth[Month] = SysTimeAtYearStart.wDayOfWeek;
        } else {
            DayIndexAtStartOfMonth[Month] = (WORD)((DayIndexAtStartOfMonth[Month - 1] + RealDaysInMonth[Month - 1]) % 7);
        }
    }

    for (Quarter = 0; Quarter < CAL_ROWS_OF_MONTHS; Quarter++) {

        //
        //  For each row of months, first display the name of the month,
        //  centered around where the day numbers below it will follow.
        //

        CurrentOffset = 0;
        for (MonthIndex = 0; MonthIndex < CAL_MONTHS_PER_ROW; MonthIndex++) {
            DesiredOffset = 0;
            if (MonthIndex > 0) {
                DesiredOffset = MonthIndex * ((CAL_DAYS_PER_WEEK * CAL_CHARS_PER_DAY) + CAL_CHARS_BETWEEN_MONTHS);
            }

            MonthNameLength = (YORI_ALLOC_SIZE_T)_tcslen(CalMonthNames[Quarter * CAL_MONTHS_PER_ROW + MonthIndex]);

            DesiredOffset += ((CAL_DAYS_PER_WEEK * CAL_CHARS_PER_DAY) - MonthNameLength) / 2;
            while(CurrentOffset < DesiredOffset) {
                Line.StartOfString[CurrentOffset] = ' ';
                Line.LengthInChars += 1;
                CurrentOffset++;
            }

            memcpy(&Line.StartOfString[CurrentOffset], CalMonthNames[Quarter * CAL_MONTHS_PER_ROW + MonthIndex], MonthNameLength * sizeof(TCHAR));
            CurrentOffset += MonthNameLength;
            Line.LengthInChars = Line.LengthInChars + MonthNameLength;
        }
        Line.StartOfString[Line.LengthInChars] = '\n';
        Line.LengthInChars++;

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Line);
        Line.LengthInChars = 0;

        //
        //  Print the day names
        //

        for (MonthIndex = 0; MonthIndex < CAL_MONTHS_PER_ROW; MonthIndex++) {
            for (DayCount = 0; DayCount < CAL_DAYS_PER_WEEK; DayCount++) {
                YoriLibSPrintf(&Line.StartOfString[Line.LengthInChars], _T("%2s"), CalDayNames[DayCount]);
                Line.LengthInChars += 2;
                CurrentOffset = 2;
                DesiredOffset = CAL_CHARS_PER_DAY;
                while (CurrentOffset < DesiredOffset) {
                    Line.StartOfString[Line.LengthInChars] = ' ';
                    Line.LengthInChars++;
                    CurrentOffset++;
                }
            }

            //
            //  Print spaces between two months.
            //

            if (MonthIndex != (CAL_MONTHS_PER_ROW - 1)) {
                DesiredOffset = CAL_CHARS_BETWEEN_MONTHS;
                CurrentOffset = 0;
                while (CurrentOffset < DesiredOffset) {
                    Line.StartOfString[Line.LengthInChars] = ' ';
                    Line.LengthInChars++;
                    CurrentOffset++;
                }
            }
        }
        Line.StartOfString[Line.LengthInChars] = '\n';
        Line.LengthInChars++;
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Line);
        Line.LengthInChars = 0;

        //
        //  Print the day numbers for each month.
        //

        for (LineCount = 0; LineCount < CAL_ROWS_PER_MONTH; LineCount++) {
            for (MonthIndex = 0; MonthIndex < CAL_MONTHS_PER_ROW; MonthIndex++) {
                Month = (WORD)(Quarter * CAL_MONTHS_PER_ROW + MonthIndex);
                for (DayCount = 0; DayCount < CAL_DAYS_PER_WEEK; DayCount++) {
                    DesiredOffset = CAL_CHARS_PER_DAY;
                    CurrentOffset = 0;
                    if (LineCount > 0 || DayCount >= DayIndexAtStartOfMonth[Month]) {
                        ThisDayNumber = (WORD)(LineCount * CAL_DAYS_PER_WEEK + DayCount - DayIndexAtStartOfMonth[Month] + 1);
                        if (ThisDayNumber <= RealDaysInMonth[Month]) {
                            if (Today != NULL &&
                                Today->wYear == Year &&
                                Today->wMonth == (Month + 1) &&
                                Today->wDay == ThisDayNumber) {

                                YoriLibSPrintf(&Line.StartOfString[Line.LengthInChars], _T("%c[7m%02i%c[7m"), 27, ThisDayNumber, 27);
                                Line.LengthInChars += 4 + 2 + 4;
                            } else {
                                YoriLibSPrintf(&Line.StartOfString[Line.LengthInChars], _T("%02i"), ThisDayNumber);
                                Line.LengthInChars +=  2;
                            }
                            CurrentOffset = 2;
                        }
                    }
                    while (CurrentOffset < DesiredOffset) {
                        Line.StartOfString[Line.LengthInChars] = ' ';
                        Line.LengthInChars++;
                        CurrentOffset++;
                    }
                }

                //
                //  Print spaces between two months.
                //

                if (MonthIndex != (CAL_MONTHS_PER_ROW - 1)) {
                    DesiredOffset = CAL_CHARS_BETWEEN_MONTHS;
                    CurrentOffset = 0;
                    while (CurrentOffset < DesiredOffset) {
                        Line.StartOfString[Line.LengthInChars] = ' ';
                        Line.LengthInChars++;
                        CurrentOffset++;
                    }
                }
            }
            Line.StartOfString[Line.LengthInChars] = '\n';
            Line.LengthInChars++;
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Line);
            Line.LengthInChars = 0;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
    }

    YoriLibFreeStringContents(&Line);

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the cal builtin command.
 */
#define ENTRYPOINT YoriCmd_YCAL
#else
/**
 The main entrypoint for the cal standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the cal cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    SYSTEMTIME CurrentSysTime;
    YORI_ALLOC_SIZE_T CharsConsumed;
    YORI_MAX_SIGNED_T TargetYear;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                CalHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
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

    if (StartArg != 0) {
        for (i = 0; i < sizeof(CalMonthNames)/sizeof(CalMonthNames[0]); i++) {
            if (YoriLibCompareStringLitIns(&ArgV[StartArg], CalMonthNames[i]) == 0) {
                GetLocalTime(&CurrentSysTime);
                CalOutputCalendarForMonth(CurrentSysTime.wYear, (WORD)i, &CurrentSysTime);
                return EXIT_SUCCESS;
            }
        }

        if (!YoriLibStringToNumber(&ArgV[StartArg], FALSE, &TargetYear, &CharsConsumed) ||
            CharsConsumed == 0) {

            TargetYear = 0;
        }

        if (TargetYear > 0 && TargetYear <= (YORI_MAX_SIGNED_T)(sizeof(CalMonthNames)/sizeof(CalMonthNames[0]))) {
            GetLocalTime(&CurrentSysTime);
            CalOutputCalendarForMonth(CurrentSysTime.wYear, (WORD)(TargetYear - 1), &CurrentSysTime);
            return EXIT_SUCCESS;
        }

        if (TargetYear < 1601 || TargetYear > 2100) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("cal: invalid year specified: %y\n"), &ArgV[StartArg]);
            return EXIT_FAILURE;
        }

        GetLocalTime(&CurrentSysTime);
        CalOutputCalendarForYear((WORD)TargetYear, &CurrentSysTime);
    } else {
        GetLocalTime(&CurrentSysTime);
        CalOutputCalendarForMonth(CurrentSysTime.wYear, (WORD)(CurrentSysTime.wMonth - 1), &CurrentSysTime);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
