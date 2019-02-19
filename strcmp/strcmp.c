/**
 * @file strcmp/strcmp.c
 *
 * Yori shell compare strings
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
CHAR strStrCmpHelpText[] =
        "\n"
        "Compare two strings.\n"
        "\n"
        "STRCMP [-license] [-i] [--] <string><operator><string>\n"
        "\n"
        "   --             Treat all further arguments as comparison parameters\n"
        "   -i             Match case insensitively\n"
        "\n"
        "Operators are:\n"
        "   ==             Strings match exactly\n"
        "   !=             Strings do not match\n";

/**
 Display usage text to the user.
 */
BOOL
StrCmpHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("StrCmp %i.%02i\n"), STRCMP_VER_MAJOR, STRCMP_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strStrCmpHelpText);
    return TRUE;
}

/**
 An array index for an operator indicating a match.
 */
#define STRCMP_OPERATOR_EXACT_MATCH 0

/**
 An array index for an operator indicating a mismatch.
 */
#define STRCMP_OPERATOR_NO_MATCH    1

/**
 An array index beyond the array, ie., the number of elements in the array.
 */
#define STRCMP_OPERATOR_BEYOND_MAX  2

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the strcmp builtin command.
 */
#define ENTRYPOINT YoriCmd_STRCMP
#else
/**
 The main entrypoint for the strcmp standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the strcmp cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero for true and nonzero for false.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD StartArg = 0;
    DWORD i;
    YORI_STRING Expression;
    YORI_STRING Arg;
    YORI_STRING FirstPart;
    YORI_STRING SecondPart;
    YORI_STRING OperatorMatches[STRCMP_OPERATOR_BEYOND_MAX];
    PYORI_STRING MatchingOperator;
    DWORD OperatorIndex;
    BOOL MatchInsensitive;
    DWORD Result;

    MatchInsensitive = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                StrCmpHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                MatchInsensitive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                if (i + 1 < ArgC) {
                    ArgumentUnderstood = TRUE;
                    StartArg = i + 1;
                    break;
                }
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("strcmp: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, &Expression)) {
        return EXIT_FAILURE;
    }

    YoriLibConstantString(&OperatorMatches[STRCMP_OPERATOR_EXACT_MATCH], _T("=="));
    YoriLibConstantString(&OperatorMatches[STRCMP_OPERATOR_NO_MATCH], _T("!="));

    MatchingOperator = YoriLibFindFirstMatchingSubstring(&Expression, sizeof(OperatorMatches)/sizeof(OperatorMatches[0]), OperatorMatches, &OperatorIndex);
    if (MatchingOperator == NULL) {
        YoriLibFreeStringContents(&Expression);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("strcmp: missing operator\n"));
        return EXIT_FAILURE;
    }

    YoriLibInitEmptyString(&FirstPart);

    FirstPart.StartOfString = Expression.StartOfString;
    FirstPart.LengthInChars = OperatorIndex;

    YoriLibInitEmptyString(&SecondPart);

    SecondPart.StartOfString = Expression.StartOfString + OperatorIndex + MatchingOperator->LengthInChars;
    SecondPart.LengthInChars = Expression.LengthInChars - OperatorIndex - MatchingOperator->LengthInChars;

    Result = EXIT_FAILURE;

    if (MatchingOperator == &OperatorMatches[STRCMP_OPERATOR_EXACT_MATCH]) {

        if (MatchInsensitive) {
            if (YoriLibCompareStringInsensitive(&FirstPart, &SecondPart) == 0) {
                Result = EXIT_SUCCESS;
            } else {
                Result = EXIT_FAILURE;
            }
        } else {
            if (YoriLibCompareString(&FirstPart, &SecondPart) == 0) {
                Result = EXIT_SUCCESS;
            } else {
                Result = EXIT_FAILURE;
            }
        }

    } else if (MatchingOperator == &OperatorMatches[STRCMP_OPERATOR_NO_MATCH]) {

        if (MatchInsensitive) {
            if (YoriLibCompareStringInsensitive(&FirstPart, &SecondPart) == 0) {
                Result = EXIT_FAILURE;
            } else {
                Result = EXIT_SUCCESS;
            }
        } else {
            if (YoriLibCompareString(&FirstPart, &SecondPart) == 0) {
                Result = EXIT_FAILURE;
            } else {
                Result = EXIT_SUCCESS;
            }
        }

    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("strcmp: operator not implemented\n"));
        return EXIT_FAILURE;
    }

    YoriLibFreeStringContents(&Expression);

    return Result;
}

// vim:sw=4:ts=4:et:
