/**
 * @file battery/battery.c
 *
 * Yori shell display battery information
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

/**
 Help text to display to the user.
 */
const
CHAR strBatteryHelpText[] =
        "\n"
        "Display battery information.\n"
        "\n"
        "BATTERY [-license] [<fmt>]\n"
        "\n"
        "Format specifiers are:\n"
        "   $CHARGING$             Whether the system is charging, draining or full\n"
        "   $PERCENTREMAINING$     Percent of the battery remaining\n"
        "   $POWERSOURCE$          Whether the system is running from AC or battery\n"
        "   $REMAINING_HOURS$      The estimated remaining hours of battery life\n"
        "   $REMAINING_MINUTES$    The estimated remaining minutes of battery life\n"
        "   $REMAINING_TIME$       The estimated remaining battery time in human form\n";

/**
 Display usage text to the user.
 */
BOOL
BatteryHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Battery %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strBatteryHelpText);
    return TRUE;
}

/**
 Context about battery state that is passed between query and string
 expansion.
 */
typedef struct _BATTERY_CONTEXT {

    /**
     System power information.
     */
    YORI_SYSTEM_POWER_STATUS PowerStatus;

    /**
     The estimated remaining battery capacity, in minutes.
     */
    DWORD RemainingTotalMinutes;

    /**
     The estimated remaining battery capacity, in hours.
     */
    DWORD RemainingHours;

    /**
     The estimated remaining battery capacity minutes in addition to
     RemainingHours above.
     */
    DWORD RemainingMinutes;

    /**
     Pointer to a constant string indicating how to display the hour value.
     This might be "hour" or "hours".
     */
    LPCTSTR RemainingHoursString;

    /**
     Pointer to a constant string indicating how to display the hour value.
     This might be "minute" or "minutes".
     */
    LPCTSTR RemainingMinutesString;

} BATTERY_CONTEXT, *PBATTERY_CONTEXT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputBuffer A pointer to the output buffer to populate with data
        if a known variable is found.

 @param VariableName The variable name to expand.

 @param Context Pointer to a battery structure containing the data to
        populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
YORI_ALLOC_SIZE_T
BatteryExpandVariables(
    __inout PYORI_STRING OutputBuffer,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    YORI_ALLOC_SIZE_T CharsNeeded;
    PBATTERY_CONTEXT BatteryContext = (PBATTERY_CONTEXT)Context;

    if (YoriLibCompareStringLit(VariableName, _T("CHARGING")) == 0) {
        if (BatteryContext->PowerStatus.BatteryFlag == YORI_BATTERY_FLAG_UNKNOWN) {
            CharsNeeded = YoriLibSPrintfSize(_T("Unknown"));
        } else if (BatteryContext->PowerStatus.BatteryFlag & YORI_BATTERY_FLAG_CHARGING) {
            CharsNeeded = YoriLibSPrintfSize(_T("Charging"));
        } else if ((BatteryContext->PowerStatus.PowerSource & YORI_POWER_SOURCE_POWERED) &&
                   (BatteryContext->PowerStatus.BatteryLifePercent == 100)) {
            CharsNeeded = YoriLibSPrintfSize(_T("Full"));
        } else if (BatteryContext->PowerStatus.PowerSource & YORI_POWER_SOURCE_POWERED) {
            CharsNeeded = YoriLibSPrintfSize(_T("Holding"));
        } else {
            CharsNeeded = YoriLibSPrintfSize(_T("Draining"));
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("PERCENTREMAINING")) == 0) {
        if (BatteryContext->PowerStatus.BatteryLifePercent >= 100) {
            CharsNeeded = 3;
        } else {
            CharsNeeded = 2;
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("POWERSOURCE")) == 0) {
        if (BatteryContext->PowerStatus.PowerSource & YORI_POWER_SOURCE_POWERED) {
            CharsNeeded = YoriLibSPrintfSize(_T("AC"));
        } else {
            CharsNeeded = YoriLibSPrintfSize(_T("Battery"));
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("REMAINING_HOURS")) == 0) {
        if (BatteryContext->PowerStatus.BatterySecondsRemaining == (DWORD)-1) {
            CharsNeeded = YoriLibSPrintfSize(_T("Unknown"));
        } else {
            CharsNeeded = YoriLibSPrintfSize(_T("%i"), BatteryContext->RemainingHours);
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("REMAINING_MINUTES")) == 0) {
        if (BatteryContext->PowerStatus.BatterySecondsRemaining == (DWORD)-1) {
            CharsNeeded = YoriLibSPrintfSize(_T("Unknown"));
        } else {
            CharsNeeded = YoriLibSPrintfSize(_T("%i"), BatteryContext->RemainingTotalMinutes);
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("REMAINING_TIME")) == 0) {
        if (BatteryContext->PowerStatus.BatterySecondsRemaining == (DWORD)-1) {
            CharsNeeded = YoriLibSPrintfSize(_T("Unknown"));
        } else {
            if (BatteryContext->RemainingHours > 0) {
                if (BatteryContext->RemainingMinutes == 0) {
                    CharsNeeded = YoriLibSPrintfSize(_T("%i %s"), BatteryContext->RemainingHours, BatteryContext->RemainingHoursString);
                } else {
                    CharsNeeded = YoriLibSPrintfSize(_T("%i %s, %i %s"),
                                                     BatteryContext->RemainingHours,
                                                     BatteryContext->RemainingHoursString,
                                                     BatteryContext->RemainingMinutes,
                                                     BatteryContext->RemainingMinutesString);
                }
            } else {
                CharsNeeded = YoriLibSPrintfSize(_T("%i %s"), BatteryContext->RemainingMinutes, BatteryContext->RemainingMinutesString);
            }
        }
    } else {
        return 0;
    }

    if (OutputBuffer->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringLit(VariableName, _T("CHARGING")) == 0) {
        if (BatteryContext->PowerStatus.BatteryFlag == YORI_BATTERY_FLAG_UNKNOWN) {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Unknown"));
        } else if (BatteryContext->PowerStatus.BatteryFlag & YORI_BATTERY_FLAG_CHARGING) {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Charging"));
        } else if ((BatteryContext->PowerStatus.PowerSource & YORI_POWER_SOURCE_POWERED) &&
                   (BatteryContext->PowerStatus.BatteryLifePercent == 100)) {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Full"));
        } else if (BatteryContext->PowerStatus.PowerSource & YORI_POWER_SOURCE_POWERED) {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Holding"));
        } else {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Draining"));
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("PERCENTREMAINING")) == 0) {
        if (BatteryContext->PowerStatus.BatteryLifePercent >= 100) {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("100"));
        } else {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("%02i"), BatteryContext->PowerStatus.BatteryLifePercent);
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("POWERSOURCE")) == 0) {
        if (BatteryContext->PowerStatus.PowerSource & YORI_POWER_SOURCE_POWERED) {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("AC"));
        } else {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Battery"));
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("REMAINING_HOURS")) == 0) {
        if (BatteryContext->PowerStatus.BatterySecondsRemaining == (DWORD)-1) {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Unknown"));
        } else {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i"), BatteryContext->RemainingHours);
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("REMAINING_MINUTES")) == 0) {
        if (BatteryContext->PowerStatus.BatterySecondsRemaining == (DWORD)-1) {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Unknown"));
        } else {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i"), BatteryContext->RemainingTotalMinutes);
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("REMAINING_TIME")) == 0) {
        if (BatteryContext->PowerStatus.BatterySecondsRemaining == (DWORD)-1) {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Unknown"));
        } else {
            if (BatteryContext->RemainingHours > 0) {
                if (BatteryContext->RemainingMinutes == 0) {
                    YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i %s"), BatteryContext->RemainingHours, BatteryContext->RemainingHoursString);
                } else {
                    YoriLibSPrintf(OutputBuffer->StartOfString,
                                   _T("%i %s, %i %s"),
                                   BatteryContext->RemainingHours,
                                   BatteryContext->RemainingHoursString,
                                   BatteryContext->RemainingMinutes,
                                   BatteryContext->RemainingMinutesString);
                }
            } else {
                YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i %s"), BatteryContext->RemainingMinutes, BatteryContext->RemainingMinutesString);
            }
        }
    }

    return CharsNeeded;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the battery builtin command.
 */
#define ENTRYPOINT YoriCmd_YBATTERY
#else
/**
 The main entrypoint for the battery standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the battery cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero indicating success or nonzero on
         failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    BATTERY_CONTEXT BatteryContext;
    YORI_STRING DisplayString;
    YORI_STRING AllocatedFormatString;
    BOOLEAN DisplayGraph;
    LPTSTR DefaultFormatString = _T("Power source: $POWERSOURCE$\n")
                                 _T("Charging: $CHARGING$\n")
                                 _T("Remaining battery: $REMAINING_TIME$\n");

    DisplayGraph = TRUE;
    ZeroMemory(&BatteryContext, sizeof(BatteryContext));
    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                BatteryHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019-2023"));
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

    if (DllKernel32.pGetSystemPowerStatus == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("battery: OS support not present\n"));
        return EXIT_FAILURE;
    }

    if (!DllKernel32.pGetSystemPowerStatus(&BatteryContext.PowerStatus)) {
        DWORD Err;
        LPTSTR ErrText;

        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        if (ErrText != NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("battery: query failed: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return EXIT_FAILURE;
        }
    }

    if (BatteryContext.PowerStatus.BatterySecondsRemaining == (DWORD)-1) {
        BatteryContext.RemainingTotalMinutes = 0;
        BatteryContext.RemainingHours = 0;
        BatteryContext.RemainingMinutes = 0;
        BatteryContext.RemainingHoursString = _T("");
        BatteryContext.RemainingMinutesString = _T("");
    } else {
        BatteryContext.RemainingTotalMinutes = BatteryContext.PowerStatus.BatterySecondsRemaining / 60;
        BatteryContext.RemainingHours = BatteryContext.RemainingTotalMinutes / 60;
        BatteryContext.RemainingMinutes = BatteryContext.RemainingTotalMinutes % 60;
        if (BatteryContext.RemainingHours == 1) {
            BatteryContext.RemainingHoursString = _T("hour");
        } else {
            BatteryContext.RemainingHoursString = _T("hours");
        }
        if (BatteryContext.RemainingMinutes == 1) {
            BatteryContext.RemainingMinutesString = _T("minute");
        } else {
            BatteryContext.RemainingMinutesString = _T("minutes");
        }
    }

    //
    //  Obtain a format string.
    //

    YoriLibInitEmptyString(&AllocatedFormatString);
    if (StartArg > 0) {
        DisplayGraph = FALSE;
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, FALSE, &AllocatedFormatString)) {
            return EXIT_FAILURE;
        }
    } else {
        YoriLibConstantString(&AllocatedFormatString, DefaultFormatString);
    }

    if (DisplayGraph) {
        UCHAR BatteryLifePercent;
        BatteryLifePercent = BatteryContext.PowerStatus.BatteryLifePercent;
        if (BatteryLifePercent > 100) {
            BatteryLifePercent = 100;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Percent remaining: %i%%\n"), BatteryLifePercent);
        YoriLibDisplayBarGraph(GetStdHandle(STD_OUTPUT_HANDLE), BatteryLifePercent * 10, 400, 200);
    }

    if (AllocatedFormatString.LengthInChars > 0) {

        //
        //  Output the format string with summary counts.
        //

        YoriLibInitEmptyString(&DisplayString);
        YoriLibExpandCommandVariables(&AllocatedFormatString, '$', FALSE, BatteryExpandVariables, &BatteryContext, &DisplayString);
        if (DisplayString.StartOfString != NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            YoriLibFreeStringContents(&DisplayString);
        }
    }

    YoriLibFreeStringContents(&AllocatedFormatString);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
