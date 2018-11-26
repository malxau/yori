/**
 * @file intcmp/intcmp.c
 *
 * Yori shell compare integers
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
CHAR strIntCmpHelpText[] =
        "\n"
        "Compare two integer values.\n"
        "\n"
        "INTCMP [-license] [--] <string><operator><string>\n"
        "\n"
        "   --             Treat all further arguments as comparison parameters\n"
        "\n"
        "Operators are:\n"
        "   ==             Numbers match exactly\n"
        "   !=             Numbers do not match\n"
        "   >=             First number greater than or equal to second number\n"
        "   <=             First number less than or equal to second number\n"
        "   >              First number greater than second number\n"
        "   <              First number less than second number\n";

/**
 Display usage text to the user.
 */
BOOL
IntCmpHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("IntCmp %i.%i\n"), INTCMP_VER_MAJOR, INTCMP_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strIntCmpHelpText);
    return TRUE;
}

/**
 An array index for the exact match operator.
 */
#define INTCMP_OPERATOR_EXACT_MATCH      0

/**
 An array index for the no match operator.
 */
#define INTCMP_OPERATOR_NO_MATCH         1

/**
 An array index for the greater than or equal operator.
 */
#define INTCMP_OPERATOR_GREATER_OR_EQUAL 2

/**
 An array index for the less than or equal operator.
 */
#define INTCMP_OPERATOR_LESS_OR_EQUAL    3

/**
 An array index for the greater than operator.
 */
#define INTCMP_OPERATOR_GREATER          4

/**
 An array index for the less than operator.
 */
#define INTCMP_OPERATOR_LESS             5

/**
 The total number of array elements present.
 */
#define INTCMP_OPERATOR_BEYOND_MAX       6

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the intcmp builtin command.
 */
#define ENTRYPOINT YoriCmd_INTCMP
#else
/**
 The main entrypoint for the intcmp standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the intcmp cmdlet.

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
    DWORD StartArg = 0;
    DWORD i;
    YORI_STRING EntireExpression;
    YORI_STRING FirstPart;
    YORI_STRING SecondPart;
    YORI_STRING OperatorMatches[INTCMP_OPERATOR_BEYOND_MAX];
    YORI_STRING Arg;
    PYORI_STRING MatchingOperator;
    DWORD OperatorIndex;
    DWORD CharsConsumed;
    LONGLONG FirstNumber;
    LONGLONG SecondNumber;
    int Result;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                IntCmpHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("intcmp: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, &EntireExpression)) {
        return EXIT_FAILURE;
    }

    YoriLibConstantString(&OperatorMatches[INTCMP_OPERATOR_EXACT_MATCH], _T("=="));
    YoriLibConstantString(&OperatorMatches[INTCMP_OPERATOR_NO_MATCH], _T("!="));
    YoriLibConstantString(&OperatorMatches[INTCMP_OPERATOR_GREATER_OR_EQUAL], _T(">="));
    YoriLibConstantString(&OperatorMatches[INTCMP_OPERATOR_LESS_OR_EQUAL], _T("<="));
    YoriLibConstantString(&OperatorMatches[INTCMP_OPERATOR_GREATER], _T(">"));
    YoriLibConstantString(&OperatorMatches[INTCMP_OPERATOR_LESS], _T("<"));

    MatchingOperator = YoriLibFindFirstMatchingSubstring(&EntireExpression, sizeof(OperatorMatches)/sizeof(OperatorMatches[0]), OperatorMatches, &OperatorIndex);
    if (MatchingOperator == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("intcmp: missing operator\n"));
        YoriLibFreeStringContents(&EntireExpression);
        return EXIT_FAILURE;
    }

    YoriLibInitEmptyString(&FirstPart);

    FirstPart.StartOfString = EntireExpression.StartOfString;
    FirstPart.LengthInChars = OperatorIndex;

    YoriLibInitEmptyString(&SecondPart);

    SecondPart.StartOfString = EntireExpression.StartOfString + OperatorIndex + MatchingOperator->LengthInChars;
    SecondPart.LengthInChars = EntireExpression.LengthInChars - OperatorIndex - MatchingOperator->LengthInChars;

    YoriLibTrimSpaces(&FirstPart);
    YoriLibTrimSpaces(&SecondPart);

    if (!YoriLibStringToNumber(&FirstPart, TRUE, &FirstNumber, &CharsConsumed) || CharsConsumed == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("intcmp: non numeric argument\n"));
        YoriLibFreeStringContents(&EntireExpression);
        return EXIT_FAILURE;
    }

    if (!YoriLibStringToNumber(&SecondPart, TRUE, &SecondNumber, &CharsConsumed) || CharsConsumed == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("intcmp: non numeric argument\n"));
        YoriLibFreeStringContents(&EntireExpression);
        return EXIT_FAILURE;
    }

    YoriLibFreeStringContents(&EntireExpression);

    Result = EXIT_FAILURE;

    if (MatchingOperator == &OperatorMatches[INTCMP_OPERATOR_EXACT_MATCH]) {

        if (FirstNumber == SecondNumber) {
            Result = EXIT_SUCCESS;
        } else {
            Result = EXIT_FAILURE;
        }

    } else if (MatchingOperator == &OperatorMatches[INTCMP_OPERATOR_NO_MATCH]) {

        if (FirstNumber != SecondNumber) {
            Result = EXIT_SUCCESS;
        } else {
            Result = EXIT_FAILURE;
        }

    } else if (MatchingOperator == &OperatorMatches[INTCMP_OPERATOR_GREATER_OR_EQUAL]) {

        if (FirstNumber >= SecondNumber) {
            Result = EXIT_SUCCESS;
        } else {
            Result = EXIT_FAILURE;
        }

    } else if (MatchingOperator == &OperatorMatches[INTCMP_OPERATOR_LESS_OR_EQUAL]) {

        if (FirstNumber <= SecondNumber) {
            Result = EXIT_SUCCESS;
        } else {
            Result = EXIT_FAILURE;
        }

    } else if (MatchingOperator == &OperatorMatches[INTCMP_OPERATOR_GREATER]) {

        if (FirstNumber > SecondNumber) {
            Result = EXIT_SUCCESS;
        } else {
            Result = EXIT_FAILURE;
        }

    } else if (MatchingOperator == &OperatorMatches[INTCMP_OPERATOR_LESS]) {

        if (FirstNumber < SecondNumber) {
            Result = EXIT_SUCCESS;
        } else {
            Result = EXIT_FAILURE;
        }

    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("intcmp: operator not implemented %x\n"), MatchingOperator);
        return EXIT_FAILURE;
    }

    return Result;
}

// vim:sw=4:ts=4:et:
