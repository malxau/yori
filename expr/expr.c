/**
 * @file expr/expr.c
 *
 * Yori shell perform simple math operations
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
CHAR strExprHelpText[] =
        "\n"
        "Evaluate simple arithmetic expressions.\n"
        "\n"
        "EXPR [-license] [-h] [-s] [-int8|-int16|-int32|-uint8|-uint16|-uint32]\n"
        "     <Number>[+|-|*|/|%|<|>|^]<Number>...\n"
        "\n"
        "   -h             Output result as hex rather than decimal\n"
        "   -int8          Output result as a signed 8 bit value\n"
        "   -int16         Output result as a signed 16 bit value\n"
        "   -int32         Output result as a signed 32 bit value\n"
        "   -uint8         Output result as an unsigned 8 bit value\n"
        "   -uint16        Output result as an unsigned 16 bit value\n"
        "   -uint32        Output result as an unsigned 32 bit value\n"
        "   -s             Display digit group seperators\n";

/**
 Display usage text to the user.
 */
BOOL
ExprHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Expr %i.%02i\n"), EXPR_VER_MAJOR, EXPR_VER_MINOR);
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

 @param OutputSeperator TRUE if commas should be inserted at every third
        digit in decimal output.  FALSE if no seperators should be inserted.

 @param Base The case to use when displaying the number.
 */
VOID
ExprOutputNumber(
    __in EXPR_NUMBER Number,
    __in BOOL OutputSeperator,
    __in DWORD Base
    )
{
    YORI_STRING String;
    DWORD SeperatorDigits = 0;

    if (!YoriLibAllocateString(&String, 32)) {
        return;
    }

    if (OutputSeperator) {
        SeperatorDigits = 3;
    }

    YoriLibNumberToString(&String, Number.Value, Base, SeperatorDigits, ',');
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
    BOOL OutputSeperator = FALSE;
    DWORD i;
    DWORD StartArg = 0;
    DWORD CharsConsumed;
    DWORD BitsInOutput = 63;
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
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                OutputHex = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("int8")) == 0) {
                BitsInOutput = 7;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("int16")) == 0) {
                BitsInOutput = 15;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("int32")) == 0) {
                BitsInOutput = 31;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                OutputSeperator = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("uint8")) == 0) {
                BitsInOutput = 8;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("uint16")) == 0) {
                BitsInOutput = 16;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("uint32")) == 0) {
                BitsInOutput = 32;
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

    if (StartArg == 0 || StartArg == ArgC) {
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
                if (Temp.Value != 0) {
                    Result.Value /= Temp.Value;
                } else {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("err: divide by zero\n"));
                    YoriLibFreeStringContents(&RemainingString);
                    return EXIT_FAILURE;
                }
                RemainingString.StartOfString += CharsConsumed;
                RemainingString.LengthInChars -= CharsConsumed;
                ArgumentUnderstood = TRUE;
                break;
            case '%':
                RemainingString.StartOfString++;
                RemainingString.LengthInChars--;
                YoriLibTrimSpaces(&RemainingString);
                ExprStringToNumber(&RemainingString, &Temp, &CharsConsumed);
                if (Temp.Value != 0) {
                    Result.Value %= Temp.Value;
                } else {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("err: divide by zero\n"));
                    YoriLibFreeStringContents(&RemainingString);
                    return EXIT_FAILURE;
                }
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

    switch (BitsInOutput) {
        case 32:
            Result.Value = (LONGLONG)(DWORD)Result.Value;
            break;
        case 31:
            Result.Value = (LONGLONG)(INT)Result.Value;
            break;
        case 16:
            Result.Value = (LONGLONG)(WORD)Result.Value;
            break;
        case 15:
            Result.Value = (LONGLONG)(SHORT)Result.Value;
            break;
        case 8:
            Result.Value = (LONGLONG)(UCHAR)Result.Value;
            break;
        case 7:
            Result.Value = (LONGLONG)(CHAR)Result.Value;
            break;
    }

    YoriLibFreeStringContents(&RemainingString);

    if (OutputHex) {
        ExprOutputNumber(Result, FALSE, 16);
    } else if (OutputSeperator) {
        ExprOutputNumber(Result, TRUE, 10);
    } else {
        ExprOutputNumber(Result, FALSE, 10);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
