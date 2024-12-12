/**
 * @file airplan/airplan.c
 *
 * Yori shell display or modify airplane mode
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

/**
 Help text to display to the user.
 */
const
CHAR strAirplaneHelpText[] =
        "\n"
        "Display or modify airplane mode.\n"
        "\n"
        "AIRPLAN [-license] [-enable|-disable] [<fmt>]\n"
        "\n"
        "   -disable       Turn airplane mode off\n"
        "   -enable        Turn airplane mode on\n"
        "\n"
        "Format specifiers are:\n"
        "   $ENABLED$              Whether the system has airplane mode enabled as binary\n"
        "   $ENABLED_STRING$       Whether the system has airplane mode enabled as string\n";

/**
 Display usage text to the user.
 */
BOOL
AirplaneHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Airplane %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strAirplaneHelpText);
    return TRUE;
}

/**
 Context about airplane mode state that is passed between query and string
 expansion.
 */
typedef struct _AIRPLANE_CONTEXT {

    /**
     Airplane mode enabled.
     */
    BOOLEAN AirplaneModeEnabled;

    /**
     Airplane mode changable.
     */
    BOOLEAN AirplaneModeChangable;

} AIRPLANE_CONTEXT, *PAIRPLANE_CONTEXT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputBuffer A pointer to the output buffer to populate with data
        if a known variable is found.

 @param VariableName The variable name to expand.

 @param Context Pointer to a airplane structure containing the data to
        populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
YORI_ALLOC_SIZE_T
AirplaneExpandVariables(
    __inout PYORI_STRING OutputBuffer,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    YORI_ALLOC_SIZE_T CharsNeeded;
    PAIRPLANE_CONTEXT AirplaneContext = (PAIRPLANE_CONTEXT)Context;

    if (YoriLibCompareStringLit(VariableName, _T("ENABLED_STRING")) == 0) {
        if (AirplaneContext->AirplaneModeEnabled) {
            CharsNeeded = YoriLibSPrintfSize(_T("Enabled"));
        } else {
            CharsNeeded = YoriLibSPrintfSize(_T("Disabled"));
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("ENABLED")) == 0) {
        CharsNeeded = YoriLibSPrintfSize(_T("%i"), AirplaneContext->AirplaneModeEnabled);
    } else {
        return 0;
    }

    if (OutputBuffer->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringLit(VariableName, _T("ENABLED_STRING")) == 0) {
        if (AirplaneContext->AirplaneModeEnabled) {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Enabled"));
        } else {
            YoriLibSPrintf(OutputBuffer->StartOfString, _T("Disabled"));
        }
    } else if (YoriLibCompareStringLit(VariableName, _T("ENABLED")) == 0) {
        YoriLibSPrintf(OutputBuffer->StartOfString, _T("%i"), AirplaneContext->AirplaneModeEnabled);
    }

    return CharsNeeded;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the airplan builtin command.
 */
#define ENTRYPOINT YoriCmd_YAIRPLAN
#else
/**
 The main entrypoint for the airplan standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the airplan cmdlet.

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
    AIRPLANE_CONTEXT AirplaneContext;
    YORI_STRING DisplayString;
    YORI_STRING AllocatedFormatString;
    LPTSTR DefaultFormatString = _T("Airplane mode: $ENABLED_STRING$\n");
    BOOLEAN SetAirplaneMode = FALSE;
    BOOLEAN NewAirplaneModeState = FALSE;

    ZeroMemory(&AirplaneContext, sizeof(AirplaneContext));
    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                AirplaneHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2023"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("disable")) == 0) {
                SetAirplaneMode = TRUE;
                NewAirplaneModeState = FALSE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("enable")) == 0) {
                SetAirplaneMode = TRUE;
                NewAirplaneModeState = TRUE;
                ArgumentUnderstood = TRUE;
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

    if (SetAirplaneMode) {
        if (!YoriLibSetAirplaneMode(NewAirplaneModeState)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("airplan: modification failed (OS support not present?)\n"));
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    if (!YoriLibGetAirplaneMode(&AirplaneContext.AirplaneModeEnabled,
                                &AirplaneContext.AirplaneModeChangable)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("airplan: query failed (OS support not present?)\n"));
        return EXIT_FAILURE;
    }

    //
    //  Obtain a format string.
    //

    YoriLibInitEmptyString(&AllocatedFormatString);
    if (StartArg > 0) {
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, FALSE, &AllocatedFormatString)) {
            return EXIT_FAILURE;
        }
    } else {
        YoriLibConstantString(&AllocatedFormatString, DefaultFormatString);
    }

    if (AllocatedFormatString.LengthInChars > 0) {

        //
        //  Output the format string with summary counts.
        //

        YoriLibInitEmptyString(&DisplayString);
        YoriLibExpandCommandVariables(&AllocatedFormatString, '$', FALSE, AirplaneExpandVariables, &AirplaneContext, &DisplayString);
        if (DisplayString.StartOfString != NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            YoriLibFreeStringContents(&DisplayString);
        }
    }

    YoriLibFreeStringContents(&AllocatedFormatString);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
