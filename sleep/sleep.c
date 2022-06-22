/**
 * @file sleep/sleep.c
 *
 * Yori shell sleep for a specified number of seconds
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
CHAR strSleepHelpText[] =
        "\n"
        "Waits for a specified amount of time.\n"
        "\n"
        "SLEEP [-license] [-c] <time>[<suffix>]\n"
        "\n"
        "   -c             Display a countdown to zero from the specified time\n"
        "\n"
        "Suffix can be \"s\" for seconds, \"m\" for minutes, \"h\" for hours, \"ms\" for milliseconds.\n" ;

/**
 Display usage text to the user.
 */
BOOL
SleepHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Sleep %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSleepHelpText);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the sleep builtin command.
 */
#define ENTRYPOINT YoriCmd_SLEEP
#else
/**
 The main entrypoint for the sleep standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the sleep cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process indicating success or failure.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    BOOL CountdownMode = FALSE;
    DWORD StartArg;
    DWORD i;
    YORI_STRING Arg;
    YORI_STRING Suffix;
    DWORD TimeToSleep;
    LONGLONG llTemp;
    DWORD CharsConsumed;
    HANDLE CancelHandle;

    StartArg = 0;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SleepHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                CountdownMode = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i;
                ArgumentUnderstood = TRUE;
                break;
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("sleep: missing argument\n"));
        return EXIT_FAILURE;
    }

    llTemp = 0;
    if (!YoriLibStringToNumber(&ArgV[StartArg], TRUE, &llTemp, &CharsConsumed)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("sleep: parse error\n"));
        return EXIT_FAILURE;
    }
    if (CharsConsumed < ArgV[StartArg].LengthInChars) {
        YoriLibInitEmptyString(&Suffix);
        Suffix.StartOfString = &ArgV[StartArg].StartOfString[CharsConsumed];
        Suffix.LengthInChars = ArgV[StartArg].LengthInChars - CharsConsumed;
        if (YoriLibCompareStringWithLiteralInsensitive(&Suffix, _T("ms")) == 0) {
            TimeToSleep = (DWORD)llTemp;
        } else if (YoriLibCompareStringWithLiteralInsensitive(&Suffix, _T("s")) == 0) {
            TimeToSleep = (DWORD)llTemp * 1000;
        } else if (YoriLibCompareStringWithLiteralInsensitive(&Suffix, _T("m")) == 0) {
            TimeToSleep = (DWORD)llTemp * 1000 * 60;
        } else if (YoriLibCompareStringWithLiteralInsensitive(&Suffix, _T("h")) == 0) {
            TimeToSleep = (DWORD)llTemp * 1000 * 60 * 60;
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("sleep: unknown suffix\n"));
            return EXIT_FAILURE;
        }

    } else {
        TimeToSleep = (DWORD)llTemp * 1000;
    }

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    CancelHandle = YoriLibCancelGetEvent();

    if (CountdownMode) {
        DWORDLONG LongTimeToSleep;
        DWORD WaitResult;
        LARGE_INTEGER StartTime;
        LARGE_INTEGER CurrentTime;
        LARGE_INTEGER EndTime;
        CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
        BOOL ConsoleMode;

        ConsoleMode = FALSE;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo)) {
            ConsoleMode = TRUE;
        }

        StartTime.QuadPart = YoriLibGetSystemTimeAsInteger();

        LongTimeToSleep = TimeToSleep;
        EndTime.QuadPart = StartTime.QuadPart + LongTimeToSleep * 1000 * 10;

        while(TRUE) {

            CurrentTime.QuadPart = YoriLibGetSystemTimeAsInteger();

            if (CurrentTime.QuadPart > EndTime.QuadPart) {

                break;
            }

            LongTimeToSleep = EndTime.QuadPart - CurrentTime.QuadPart;
            LongTimeToSleep = LongTimeToSleep / (1000 * 10);

            if (ConsoleMode) {
                SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), ScreenInfo.dwCursorPosition);
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%02lli:%02lli:%02lli  "), LongTimeToSleep / (1000 * 60 * 60), (LongTimeToSleep / (1000 * 60)) % 60, (LongTimeToSleep / 1000) % 60);
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%02lli:%02lli:%02lli\n"), LongTimeToSleep / (1000 * 60 * 60), (LongTimeToSleep / (1000 * 60)) % 60, (LongTimeToSleep / 1000) % 60);
            }

            if (LongTimeToSleep > 970) {
                LongTimeToSleep = 970;
            }

            TimeToSleep = (DWORD)LongTimeToSleep;

            if (CancelHandle != NULL) {
                WaitResult = WaitForSingleObject(CancelHandle, TimeToSleep);
                if (WaitResult == WAIT_OBJECT_0) {
                    break;
                }
            } else {
                Sleep(TimeToSleep);
            }

        }
    } else {
        if (CancelHandle != NULL) {
            WaitForSingleObject(CancelHandle, TimeToSleep);
        } else {
            Sleep(TimeToSleep);
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
