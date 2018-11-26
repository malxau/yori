/**
 * @file expr/expr.c
 *
 * Yori shell perform simple math operations
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
CHAR strExprHelpText[] =
        "\n"
        "Evaluate simple arithmetic expressions.\n"
        "\n"
        "EXPR [-license] [-h] <Number>[+|-|*|/|%|<|>|^]<Number>...\n"
        "\n"
        "   -h             Output result as hex rather than decimal\n";

/**
 Display usage text to the user.
 */
BOOL
ExprHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Expr %i.%i\n"), EXPR_VER_MAJOR, EXPR_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strExprHelpText);
    return TRUE;
}

/**
 An internal (to expr) description of a number.  Currently just a signed
 64 bit integer.
 */
typedef struct _EXPR_NUMBER {

    /**
     The number value.
     */
    LONGLONG Value;
} EXPR_NUMBER, *PEXPR_NUMBER;

/**
 This routine attempts to convert a string to a number using all available
 parsing.  As of this writing, it understands 0x and 0n prefixes as well
 as negative numbers.

 @param String Pointer to the string to convert into integer form.

 @param Number On successful completion, this is updated to contain the
        resulting number.

 @param CharsConsumed On successful completion, this is updated to indicate
        the number of characters from the string that were used to generate
        the number.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ExprStringToNumber(
    __in PYORI_STRING String,
    __out PEXPR_NUMBER Number,
    __out PDWORD CharsConsumed
    )
{
    return YoriLibStringToNumber(String, TRUE, &Number->Value, CharsConsumed);
}

/**
 Display an expr number to stdout.

 @param Number The number to display.

 @param Base The case to use when displaying the number.
 */
VOID
ExprOutputNumber(
    __in EXPR_NUMBER Number,
    __in DWORD Base
    )
{
    YORI_STRING String;

    if (!YoriLibAllocateString(&String, 32)) {
        return;
    }

    YoriLibNumberToString(&String, Number.Value, Base, 0, '\0');
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &String);
    YoriLibFreeStringContents(&String);
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the expr builtin command.
 */
#define ENTRYPOINT YoriCmd_YEXPR
#else
/**
 The main entrypoint for the expr standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the expr cmdlet.

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
    BOOL OutputHex = FALSE;
    DWORD i;
    DWORD StartArg = 0;
    DWORD CharsConsumed;
    YORI_STRING RemainingString;
    EXPR_NUMBER Temp;
    EXPR_NUMBER Result;
    YORI_STRING Arg;

    Result.Value = 0;

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));
        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ExprHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                OutputHex = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i;
                ArgumentUnderstood = TRUE;
                break;
            } else if (YoriLibCompareStringWithLiteralInsensitiveCount(&Arg, _T("0"), 1) >= 0 &&
                       YoriLibCompareStringWithLiteralInsensitiveCount(&Arg, _T("9"), 1) <= 0) {
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("expr: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, &RemainingString)) {
        return EXIT_FAILURE;
    }

    ExprStringToNumber(&RemainingString, &Result, &CharsConsumed);
    RemainingString.StartOfString += CharsConsumed;
    RemainingString.LengthInChars -= CharsConsumed;

    while (RemainingString.LengthInChars > 0) {
        YoriLibTrimSpaces(&RemainingString);
        ArgumentUnderstood = FALSE;
        switch(RemainingString.StartOfString[0]) {
            case '+':
                RemainingString.StartOfString++;
                RemainingString.LengthInChars--;
                YoriLibTrimSpaces(&RemainingString);
                ExprStringToNumber(&RemainingString, &Temp, &CharsConsumed);
                Result.Value += Temp.Value;
                RemainingString.StartOfString += CharsConsumed;
                RemainingString.LengthInChars -= CharsConsumed;
                ArgumentUnderstood = TRUE;
                break;
            case '*':
                RemainingString.StartOfString++;
                RemainingString.LengthInChars--;
                YoriLibTrimSpaces(&RemainingString);
                ExprStringToNumber(&RemainingString, &Temp, &CharsConsumed);
                Result.Value *= Temp.Value;
                RemainingString.StartOfString += CharsConsumed;
                RemainingString.LengthInChars -= CharsConsumed;
                ArgumentUnderstood = TRUE;
                break;
            case '-':
                RemainingString.StartOfString++;
                RemainingString.LengthInChars--;
                YoriLibTrimSpaces(&RemainingString);
                ExprStringToNumber(&RemainingString, &Temp, &CharsConsumed);
                Result.Value -= Temp.Value;
                RemainingString.StartOfString += CharsConsumed;
                RemainingString.LengthInChars -= CharsConsumed;
                ArgumentUnderstood = TRUE;
                break;
            case '/':
                RemainingString.StartOfString++;
                RemainingString.LengthInChars--;
                YoriLibTrimSpaces(&RemainingString);
                ExprStringToNumber(&RemainingString, &Temp, &CharsConsumed);
                Result.Value /= Temp.Value;
                RemainingString.StartOfString += CharsConsumed;
                RemainingString.LengthInChars -= CharsConsumed;
                ArgumentUnderstood = TRUE;
                break;
            case '%':
                RemainingString.StartOfString++;
                RemainingString.LengthInChars--;
                YoriLibTrimSpaces(&RemainingString);
                ExprStringToNumber(&RemainingString, &Temp, &CharsConsumed);
                Result.Value %= Temp.Value;
                RemainingString.StartOfString += CharsConsumed;
                RemainingString.LengthInChars -= CharsConsumed;
                ArgumentUnderstood = TRUE;
                break;
            case '<':
                RemainingString.StartOfString++;
                RemainingString.LengthInChars--;
                YoriLibTrimSpaces(&RemainingString);
                ExprStringToNumber(&RemainingString, &Temp, &CharsConsumed);
                Result.Value <<= Temp.Value;
                RemainingString.StartOfString += CharsConsumed;
                RemainingString.LengthInChars -= CharsConsumed;
                ArgumentUnderstood = TRUE;
                break;
            case '>':
                RemainingString.StartOfString++;
                RemainingString.LengthInChars--;
                YoriLibTrimSpaces(&RemainingString);
                ExprStringToNumber(&RemainingString, &Temp, &CharsConsumed);
                Result.Value >>= Temp.Value;
                RemainingString.StartOfString += CharsConsumed;
                RemainingString.LengthInChars -= CharsConsumed;
                ArgumentUnderstood = TRUE;
                break;
            case '^':
                RemainingString.StartOfString++;
                RemainingString.LengthInChars--;
                YoriLibTrimSpaces(&RemainingString);
                ExprStringToNumber(&RemainingString, &Temp, &CharsConsumed);
                {
                    EXPR_NUMBER Temp2 = Result;
                    while (Temp.Value > 1) {
                        Result.Value *= Temp2.Value;
                        Temp.Value--;
                    }
                }
                RemainingString.StartOfString += CharsConsumed;
                RemainingString.LengthInChars -= CharsConsumed;
                ArgumentUnderstood = TRUE;
                break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Operator not understood, terminating: %c\n"), RemainingString.StartOfString[0]);
            break;
        }
    }

    YoriLibFreeStringContents(&RemainingString);

    if (OutputHex) {
        ExprOutputNumber(Result, 16);
    } else {
        ExprOutputNumber(Result, 10);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
