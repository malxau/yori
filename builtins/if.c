/**
 * @file builtins/if.c
 *
 * Yori shell invoke an expression and perform different actions based on
 * the result
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
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
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strIfHelpText[] =
        "\n"
        "Execute a command to evaluate a condition.\n"
        "\n"
        "IF [-license] <test cmd>; <true cmd>; <false cmd>\n";

/**
 Display usage text to the user.
 */
BOOL
IfHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("If %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strIfHelpText);
    return TRUE;
}

/**
 Look forward in the string for the next seperator between if test or
 execution expressions.  If one is found, replace the seperator char
 with a NULL terminator and return the offset.

 @param String The string to parse.

 @param Offset On successful completion, updated with the character offset of
        the seperator character.

 @return TRUE to indicate a component was found, FALSE if it was not.
 */
__success(return)
BOOL
IfFindOffsetOfNextComponent(
    __in PYORI_STRING String,
    __out PDWORD Offset
    )
{
    DWORD CharIndex;
    for (CharIndex = 0; CharIndex < String->LengthInChars; CharIndex++) {
        if (YoriLibIsEscapeChar(String->StartOfString[CharIndex])) {
            CharIndex++;
            continue;
        }
        if (String->LengthInChars > CharIndex + 2 &&
            String->StartOfString[CharIndex] == 27 &&
            String->StartOfString[CharIndex + 1] == '[') {

            YORI_STRING EscapeSubset;
            DWORD EndOfEscape;

            YoriLibInitEmptyString(&EscapeSubset);
            EscapeSubset.StartOfString = &String->StartOfString[CharIndex + 2];
            EscapeSubset.LengthInChars = String->LengthInChars - CharIndex - 2;
            EndOfEscape = YoriLibCountStringContainingChars(&EscapeSubset, _T("0123456789;"));
            CharIndex += 2 + EndOfEscape;
        } else if (String->StartOfString[CharIndex] == ';') {
            String->StartOfString[CharIndex] = '\0';
            *Offset = CharIndex;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 Yori shell test a condition and execute a command in response

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_IF(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    BOOL Result;
    YORI_STRING CmdLine;
    YORI_STRING EscapedCmdLine;
    DWORD ErrorLevel;
    DWORD CharIndex;
    DWORD i;
    DWORD StartArg = 0;
    DWORD EscapedStartArg = 0;
    YORI_STRING TestCommand;
    YORI_STRING TrueCommand;
    YORI_STRING FalseCommand;
    YORI_STRING Arg;
    DWORD SavedErrorLevel;

    DWORD EscapedArgC;
    PYORI_STRING EscapedArgV;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();
    SavedErrorLevel = YoriCallGetErrorLevel();

    if (!YoriCallGetEscapedArguments(&EscapedArgC, &EscapedArgV)) {
        EscapedArgC = ArgC;
        EscapedArgV = ArgV;
    }

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                IfHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2021"));
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

    for (i = 1; i < EscapedArgC; i++) {
        if (!YoriLibIsCommandLineOption(&EscapedArgV[i], &Arg)) {
            EscapedStartArg = i;
            break;
        }
    }

    if (StartArg == 0 || EscapedStartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("if: missing argument\n"));
        return EXIT_FAILURE;
    }

    //
    //  Parse the arguments including escape characters.  This will be used
    //  for the test condition.
    //

    if (!YoriLibBuildCmdlineFromArgcArgv(EscapedArgC - EscapedStartArg, &EscapedArgV[EscapedStartArg], TRUE, TRUE, &EscapedCmdLine)) {
        return EXIT_FAILURE;
    }

    //
    //  Parse the arguments after removing escape characters.  This will be used
    //  for the execution conditions.
    //

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, TRUE, &CmdLine)) {
        YoriLibFreeStringContents(&EscapedCmdLine);
        return EXIT_FAILURE;
    }

    YoriLibInitEmptyString(&TestCommand);
    YoriLibInitEmptyString(&TrueCommand);
    YoriLibInitEmptyString(&FalseCommand);
    YoriLibInitEmptyString(&Arg);

    Arg.StartOfString = CmdLine.StartOfString;
    Arg.LengthInChars = CmdLine.LengthInChars;

    TestCommand.StartOfString = EscapedCmdLine.StartOfString;
    TestCommand.LengthInChars = EscapedCmdLine.LengthInChars;
    if (IfFindOffsetOfNextComponent(&TestCommand, &CharIndex)) {
        TestCommand.LengthInChars = CharIndex;
    }

    if (!IfFindOffsetOfNextComponent(&Arg, &CharIndex)) {
        CharIndex = Arg.LengthInChars;
    }

    Arg.StartOfString += CharIndex;
    Arg.LengthInChars -= CharIndex;

    if (Arg.LengthInChars > 0) {
        Arg.StartOfString++;
        Arg.LengthInChars--;
        TrueCommand.StartOfString = Arg.StartOfString;
        TrueCommand.LengthInChars = Arg.LengthInChars;
        if (IfFindOffsetOfNextComponent(&Arg, &CharIndex)) {
            TrueCommand.LengthInChars = CharIndex;
        }
        Arg.StartOfString += TrueCommand.LengthInChars;
        Arg.LengthInChars -= TrueCommand.LengthInChars;
    }

    if (Arg.LengthInChars > 0) {
        Arg.StartOfString++;
        Arg.LengthInChars--;
        FalseCommand.StartOfString = Arg.StartOfString;
        FalseCommand.LengthInChars = Arg.LengthInChars;
        if (IfFindOffsetOfNextComponent(&Arg, &CharIndex)) {
            FalseCommand.LengthInChars = CharIndex;
        }
    }

    YoriLibBuiltinRemoveEmptyVariables(&TestCommand);
    TestCommand.StartOfString[TestCommand.LengthInChars] = '\0';

    Result = YoriCallExecuteExpression(&TestCommand);
    if (!Result) {
        YoriLibFreeStringContents(&TestCommand);
        YoriLibFreeStringContents(&CmdLine);
        return EXIT_FAILURE;
    }

    ErrorLevel = YoriCallGetErrorLevel();
    if (ErrorLevel == 0) {
        if (TrueCommand.LengthInChars > 0) {
            Result = YoriCallExecuteExpression(&TrueCommand);
            if (!Result) {
                YoriLibFreeStringContents(&TestCommand);
                YoriLibFreeStringContents(&CmdLine);
                YoriLibFreeStringContents(&EscapedCmdLine);
                return EXIT_FAILURE;
            }
        }
    } else {
        if (FalseCommand.LengthInChars > 0) {
            Result = YoriCallExecuteExpression(&FalseCommand);
            if (!Result) {
                YoriLibFreeStringContents(&TestCommand);
                YoriLibFreeStringContents(&CmdLine);
                YoriLibFreeStringContents(&EscapedCmdLine);
                return EXIT_FAILURE;
            }
        }
    }

    YoriLibFreeStringContents(&TestCommand);
    YoriLibFreeStringContents(&CmdLine);
    YoriLibFreeStringContents(&EscapedCmdLine);
    return SavedErrorLevel;
}

// vim:sw=4:ts=4:et:
