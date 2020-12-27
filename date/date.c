/**
 * @file date/date.c
 *
 * Yori shell display command line output
 *
 * Copyright (c) 2017-2019 Malcolm J. Smith
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
CHAR strDateHelpText[] =
        "\n"
        "Outputs the system date and time in a specified format.\n"
        "\n"
        "DATE [-license] [-t] [-u] [<fmt>]\n"
        "\n"
        "   -t             Include time in output when format not specified\n"
        "   -u             Display UTC rather than local time\n"
        "\n"
        "Format specifiers are:\n"
        "   $COUNT_MS$     The number of milliseconds since epoch with leading zero\n"
        "   $count_ms$     The number of milliseconds since epoch without leading zero\n"
        "   $DAY$          The current numeric day of the month with leading zero\n"
        "   $day$          The current numeric day of the month without leading zero\n"
        "   $HOUR$         The current hour in 24 hour format with leading zero\n"
        "   $hour$         The current hour in 24 hour format without leading zero\n"
        "   $MIN$          The current minute with leading zero\n"
        "   $min$          The current minute without leading zero\n"
        "   $MON$          The current numeric month with leading zero\n"
        "   $mon$          The current numeric month without leading zero\n"
        "   $MS$           The current number of milliseconds with leading zero\n"
        "   $ms$           The current number of milliseconds without leading zero\n"
        "   $SEC$          The current second with leading zero\n"
        "   $sec$          The current second without leading zero\n"
        "   $TICK$         The number of milliseconds since boot with leading zero\n"
        "   $tick$         The number of milliseconds since boot without leading zero\n"
        "   $YEAR$         The current year as four digits\n"
        "   $year$         The current year as two digits\n";

/**
 Display usage text to the user.
 */
BOOL
DateHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Date %i.%02i\n"), DATE_VER_MAJOR, DATE_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strDateHelpText);
    return TRUE;
}

/**
 Context structure to provide information needed to expand format variables.
 */
typedef struct _DATE_CONTEXT {

    /**
     The current system time.
     */
    SYSTEMTIME Time;

    /**
     The current tick count.
     */
    LARGE_INTEGER Tick;

} DATE_CONTEXT, *PDATE_CONTEXT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputBuffer A pointer to the output buffer to populate with data
        if a known variable is found.

 @param VariableName The variable name to expand.

 @param Context Pointer to a SYSTEMTIME structure containing the data to
        populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
DWORD
DateExpandVariables(
    __inout PYORI_STRING OutputBuffer,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    DWORD CharsNeeded;
    PDATE_CONTEXT DateContext = (PDATE_CONTEXT)Context;
    FILETIME Clock;
    LARGE_INTEGER liClock;

    liClock.QuadPart = 0;

    if (YoriLibCompareStringWithLiteral(VariableName, _T("YEAR")) == 0) {
        CharsNeeded = 4;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("year")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("MON")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("mon")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("DAY")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("day")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("HOUR")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("hour")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("MIN")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("min")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SEC")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("sec")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("MS")) == 0) {
        CharsNeeded = 4;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("ms")) == 0) {
        CharsNeeded = 4;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COUNT_MS")) == 0) {
        if (!SystemTimeToFileTime(&DateContext->Time, &Clock)) {
            return 0;
        }
        liClock.HighPart = Clock.dwHighDateTime;
        liClock.LowPart = Clock.dwLowDateTime;
        liClock.QuadPart = liClock.QuadPart / 10000;
        CharsNeeded = YoriLibSPrintfSize(_T("%016lli"), liClock.QuadPart);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("count_ms")) == 0) {
        if (!SystemTimeToFileTime(&DateContext->Time, &Clock)) {
            return 0;
        }
        liClock.HighPart = Clock.dwHighDateTime;
        liClock.LowPart = Clock.dwLowDateTime;
        liClock.QuadPart = liClock.QuadPart / 10000;
        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), liClock.QuadPart);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("TICK")) == 0) {
        CharsNeeded = YoriLibSPrintfSize(_T("%016lli"), DateContext->Tick.QuadPart);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("tick")) == 0) {
        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), DateContext->Tick.QuadPart);
    } else {
        return 0;
    }

    if (OutputBuffer->LengthAllocated <= CharsNeeded) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringWithLiteral(VariableName, _T("YEAR")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%04i"), DateContext->Time.wYear);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("year")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%02i"), DateContext->Time.wYear % 100);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("MON")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%02i"), DateContext->Time.wMonth);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("mon")) == 0) {
        if (DateContext->Time.wMonth < 100) {
            CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i"), DateContext->Time.wMonth);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("DAY")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%02i"), DateContext->Time.wDay);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("day")) == 0) {
        if (DateContext->Time.wDay < 100) {
            CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i"), DateContext->Time.wDay);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("HOUR")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%02i"), DateContext->Time.wHour);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("hour")) == 0) {
        if (DateContext->Time.wHour < 100) {
            CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i"), DateContext->Time.wHour);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("MIN")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%02i"), DateContext->Time.wMinute);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("min")) == 0) {
        if (DateContext->Time.wMinute < 100) {
            CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i"), DateContext->Time.wMinute);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SEC")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%02i"), DateContext->Time.wSecond);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("sec")) == 0) {
        if (DateContext->Time.wSecond < 100) {
            CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i"), DateContext->Time.wSecond);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("MS")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%04i"), DateContext->Time.wMilliseconds);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("ms")) == 0) {
        if (DateContext->Time.wMilliseconds < 10000) {
            CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i"), DateContext->Time.wMilliseconds);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COUNT_MS")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%016lli"), liClock.QuadPart);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("count_ms")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%lli"), liClock.QuadPart);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("TICK")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%016lli"), DateContext->Tick.QuadPart);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("tick")) == 0) {
        CharsNeeded = YoriLibSPrintf(OutputBuffer->StartOfString, _T("%lli"), DateContext->Tick.QuadPart);
    }

    return CharsNeeded;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the date builtin command.
 */
#define ENTRYPOINT YoriCmd_YDATE
#else
/**
 The main entrypoint for the date standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the date cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    BOOL UseUtc;
    BOOL DisplayTime;
    YORI_STRING DisplayString;
    LPTSTR DefaultDateFormatString = _T("$YEAR$$MON$$DAY$");
    LPTSTR DefaultTimeFormatString = _T("$YEAR$/$MON$/$DAY$ $HOUR$:$MIN$:$SEC$");
    YORI_STRING AllocatedFormatString;
    DWORD StartArg;
    DWORD i;
    YORI_STRING Arg;
    DATE_CONTEXT DateContext;

    StartArg = 0;
    UseUtc = FALSE;
    DisplayTime = FALSE;
    YoriLibInitEmptyString(&AllocatedFormatString);

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                DateHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                DisplayTime = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("u")) == 0) {
                UseUtc = TRUE;
                ArgumentUnderstood = TRUE;
            }
        } else {
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg > 0) {
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, &AllocatedFormatString)) {
            return EXIT_FAILURE;
        }
    } else {
        if (DisplayTime) {
            YoriLibConstantString(&AllocatedFormatString, DefaultTimeFormatString);
        } else {
            YoriLibConstantString(&AllocatedFormatString, DefaultDateFormatString);
        }
    }

    if (UseUtc) {
        GetSystemTime(&DateContext.Time);
    } else {
        GetLocalTime(&DateContext.Time);
    }

    if (DllKernel32.pGetTickCount64 != NULL) {
        DateContext.Tick.QuadPart = DllKernel32.pGetTickCount64();
    } else {
        DateContext.Tick.HighPart = 0;
        //
        //  Warning about using a deprecated function and how we should use
        //  GetTickCount64 instead.  The analyzer isn't smart enough to notice
        //  that when it's available, that's what we do.
        //
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 28159)
#endif
        DateContext.Tick.LowPart = GetTickCount();
    }

    YoriLibInitEmptyString(&DisplayString);
    YoriLibExpandCommandVariables(&AllocatedFormatString, '$', FALSE, DateExpandVariables, &DateContext, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        YoriLibFreeStringContents(&DisplayString);
    }
    YoriLibFreeStringContents(&AllocatedFormatString);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
