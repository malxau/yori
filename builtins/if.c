/**
 * @file builtins/if.c
 *
 * Yori shell invoke an expression and perform different actions based on
 * the result
 *
 * Copyright (c) 2018-2025 Malcolm J. Smith
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
BOOLEAN
IfFindOffsetOfNextComponent(
    __in PCYORI_STRING String,
    __out PYORI_ALLOC_SIZE_T Offset
    )
{
    YORI_ALLOC_SIZE_T CharIndex;
    for (CharIndex = 0; CharIndex < String->LengthInChars; CharIndex++) {
        if (YoriLibIsEscapeChar(String->StartOfString[CharIndex])) {
            CharIndex++;
            continue;
        }
        if (String->LengthInChars > CharIndex + 2 &&
            String->StartOfString[CharIndex] == 27 &&
            String->StartOfString[CharIndex + 1] == '[') {

            YORI_STRING EscapeSubset;
            YORI_ALLOC_SIZE_T EndOfEscape;

            YoriLibInitEmptyString(&EscapeSubset);
            EscapeSubset.StartOfString = &String->StartOfString[CharIndex + 2];
            EscapeSubset.LengthInChars = String->LengthInChars - CharIndex - 2;
            EndOfEscape = YoriLibCntStringWithChars(&EscapeSubset, _T("0123456789;"));
            CharIndex += 2 + EndOfEscape;
        } else if (String->StartOfString[CharIndex] == ';') {
            *Offset = CharIndex;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 Look forward in an array of arguments for the next seperator between if test
 and execution expressions.  If one is found, return a single string
 corresponding to the range between the start of the search and the seperator.
 Return the index of the arg containing the seperator, and index of the next
 char to start searching from, so this function can be called repeatedly.

 @param ArgC The number of arguments to search.

 @param ArgV An array of arguments with ArgC elements.

 @param ArgContainsQuotes An array of booleans indicating whether the argument
        is quoted.

 @param FirstArgStartChar The character offset to start searching within the
        first argument.  This is to allow this function to be called on a
        single argument array and resume searching partway through an
        argument.

 @param TempArgV A temporary ArgV array containing ArgC elements.  This is
        constructed within this routine as part of building strings, and is
        supplied to avoid repeated allocation and deallocation.  Contents are
        undefined both on entry and exit.

 @param Command On successful completion, populated with a single string
        indicating the text between the start of the first argument and the
        next seperator.

 @param FinalArgC On successful completion, updated to indicate the argument
        that contained the seperator.

 @param FinalChar On successful completion, updated to indicate the character
        offset following any found seperator.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
IfFindOffsetOfNextComponentInArgs(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in PCYORI_STRING ArgV,
    __in_opt PBOOLEAN ArgContainsQuotes,
    __in YORI_ALLOC_SIZE_T FirstArgStartChar,
    __in PYORI_STRING TempArgV,
    __out PYORI_STRING Command,
    __out PYORI_ALLOC_SIZE_T FinalArgC,
    __out PYORI_ALLOC_SIZE_T FinalChar
    )
{
    YORI_ALLOC_SIZE_T ArgIndex;
    YORI_ALLOC_SIZE_T CharIndex;
    YORI_ALLOC_SIZE_T StartOffset;
    YORI_STRING TmpCommand;

    //
    //  If there are no args, successfully return an empty command
    //

    if (ArgC == 0) {
        YoriLibInitEmptyString(Command);
        *FinalArgC = 0;
        *FinalChar = 0;
        return TRUE;
    }

    //
    //  Scan through the args.  For the first arg, bias based on the
    //  initial offset.  If any arg is not quoted, look for a seperator.
    //  Args within quotes are preserved regardless of containing a
    //  seperator.  If a seperator is found, return the command as well
    //  as the arg and offset within the arg to resume searching from.
    //

    CharIndex = 0;
    YoriLibInitEmptyString(&TmpCommand);
    StartOffset = FirstArgStartChar;
    for (ArgIndex = 0; ArgIndex < ArgC; ArgIndex++) {
        YoriLibInitEmptyString(&TempArgV[ArgIndex]);
        TempArgV[ArgIndex].StartOfString = ArgV[ArgIndex].StartOfString;
        TempArgV[ArgIndex].LengthInChars = ArgV[ArgIndex].LengthInChars;
        if (ArgIndex == 0) {
            ASSERT(FirstArgStartChar <= TempArgV[ArgIndex].LengthInChars);
            TempArgV[ArgIndex].StartOfString = TempArgV[ArgIndex].StartOfString + StartOffset;
            TempArgV[ArgIndex].LengthInChars = TempArgV[ArgIndex].LengthInChars - StartOffset;
        }
        if (ArgContainsQuotes == NULL || ArgContainsQuotes[ArgIndex] == FALSE) {
            if (IfFindOffsetOfNextComponent(&TempArgV[ArgIndex], &CharIndex)) {
                TempArgV[ArgIndex].LengthInChars = CharIndex;
                ArgIndex++;
                if (!YoriLibBuildCmdlineFromArgcArgv(ArgIndex, TempArgV, TRUE, TRUE, &TmpCommand)) {
                    return FALSE;
                }
                *FinalArgC = ArgIndex - 1;
                *FinalChar = StartOffset + CharIndex + 1;
                memcpy(Command, &TmpCommand, sizeof(YORI_STRING));
                return TRUE;
            }
        }
        StartOffset = 0;
    }

    //
    //  If no seperator is found, all of the remaining text is part of this
    //  command.
    //

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgIndex, TempArgV, TRUE, TRUE, &TmpCommand)) {
        return FALSE;
    }

    memcpy(Command, &TmpCommand, sizeof(YORI_STRING));
    *FinalArgC = ArgIndex - 1;
    *FinalChar = ArgV[ArgIndex - 1].LengthInChars;
    return TRUE;
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
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    BOOL Result;
    DWORD ErrorLevel;
    YORI_ALLOC_SIZE_T ArgIndex;
    YORI_ALLOC_SIZE_T StartCharIndex;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_ALLOC_SIZE_T EscapedStartArg = 0;
    YORI_STRING TestCommand;
    YORI_STRING TrueCommand;
    YORI_STRING FalseCommand;
    YORI_STRING Arg;
    DWORD SavedErrorLevel;

    DWORD TempArgC;
    YORI_ALLOC_SIZE_T EscapedArgC;
    PYORI_STRING EscapedArgV;
    PYORI_STRING TempArgV;
    PBOOLEAN ArgContainsQuotes;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();
    SavedErrorLevel = YoriCallGetErrorLevel();

    if (!YoriCallGetEscapedArgumentsEx(&TempArgC, &EscapedArgV, &ArgContainsQuotes)) {
        ArgContainsQuotes = NULL;
        if (!YoriCallGetEscapedArguments(&TempArgC, &EscapedArgV)) {
            EscapedArgC = ArgC;
            EscapedArgV = ArgV;
        } else {
            EscapedArgC = (YORI_ALLOC_SIZE_T)TempArgC;
        }
    } else {
        EscapedArgC = (YORI_ALLOC_SIZE_T)TempArgC;
    }

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {
            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                IfHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2025"));
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

    ASSERT(StartArg == EscapedStartArg);

    TempArgV = YoriLibMalloc(sizeof(YORI_STRING) * EscapedArgC);
    if (TempArgV == NULL) {
        return EXIT_FAILURE;
    }

    //
    //  NOTES
    //  Need to scan arg by arg for semicolons
    //  Note that YoriLibShRemoveEscapesFromArgCArgV ensures ArgC is identical,
    //    args just have escapes removed from each component
    //  ...But we don't know the offsets of semicolons across these
    //  It might be a good time to use escaped ArgV for everything, despite
    //    breaking compatibility

    YoriLibInitEmptyString(&TestCommand);
    YoriLibInitEmptyString(&TrueCommand);
    YoriLibInitEmptyString(&FalseCommand);

    //
    //  YoriLibShRemoveEscapesFromArgCArgV walks component-by-component
    //  removing escapes, which means the number of components is required to
    //  match
    //

    ASSERT(EscapedArgC == ArgC);

    StartArg = EscapedStartArg;
    if (!IfFindOffsetOfNextComponentInArgs(EscapedArgC - StartArg,
                                           &EscapedArgV[StartArg],
                                           (ArgContainsQuotes != NULL?&ArgContainsQuotes[StartArg]:NULL),
                                           0,
                                           TempArgV,
                                           &TestCommand,
                                           &ArgIndex,
                                           &StartCharIndex)) {

        YoriLibFree(TempArgV);
        return EXIT_FAILURE;
    }

    StartArg = StartArg + ArgIndex;

    StartCharIndex = 0;
    if (StartArg < ArgC) {
        if (IfFindOffsetOfNextComponent(&ArgV[StartArg], &StartCharIndex)) {
            StartCharIndex++;
        } else {
            StartCharIndex = ArgV[StartArg].LengthInChars;
        }
    }

    if (!IfFindOffsetOfNextComponentInArgs(ArgC - StartArg,
                                           &EscapedArgV[StartArg],
                                           (ArgContainsQuotes != NULL?&ArgContainsQuotes[StartArg]:NULL),
                                           StartCharIndex,
                                           TempArgV,
                                           &TrueCommand,
                                           &ArgIndex,
                                           &StartCharIndex)) {

        YoriLibFreeStringContents(&TestCommand);
        YoriLibFree(TempArgV);
        return EXIT_FAILURE;
    }

    StartArg = StartArg + ArgIndex;

    if (!IfFindOffsetOfNextComponentInArgs(ArgC - StartArg,
                                           &EscapedArgV[StartArg],
                                           (ArgContainsQuotes != NULL?&ArgContainsQuotes[StartArg]:NULL),
                                           StartCharIndex,
                                           TempArgV,
                                           &FalseCommand,
                                           &ArgIndex,
                                           &StartCharIndex)) {

        YoriLibFreeStringContents(&TestCommand);
        YoriLibFreeStringContents(&TrueCommand);
        YoriLibFree(TempArgV);
        return EXIT_FAILURE;
    }

    YoriLibFree(TempArgV);

    YoriLibBuiltinRemoveEmptyVariables(&TestCommand);
    TestCommand.StartOfString[TestCommand.LengthInChars] = '\0';

    Result = YoriCallExecuteExpression(&TestCommand);
    if (!Result) {
        YoriLibFreeStringContents(&FalseCommand);
        YoriLibFreeStringContents(&TrueCommand);
        YoriLibFreeStringContents(&TestCommand);
        return EXIT_FAILURE;
    }

    ErrorLevel = YoriCallGetErrorLevel();
    if (ErrorLevel == 0) {
        if (TrueCommand.LengthInChars > 0) {
            Result = YoriCallExecuteExpression(&TrueCommand);
            if (!Result) {
                YoriLibFreeStringContents(&FalseCommand);
                YoriLibFreeStringContents(&TrueCommand);
                YoriLibFreeStringContents(&TestCommand);
                return EXIT_FAILURE;
            }
        }
    } else {
        if (FalseCommand.LengthInChars > 0) {
            Result = YoriCallExecuteExpression(&FalseCommand);
            if (!Result) {
                YoriLibFreeStringContents(&FalseCommand);
                YoriLibFreeStringContents(&TrueCommand);
                YoriLibFreeStringContents(&TestCommand);
                return EXIT_FAILURE;
            }
        }
    }

    YoriLibFreeStringContents(&FalseCommand);
    YoriLibFreeStringContents(&TrueCommand);
    YoriLibFreeStringContents(&TestCommand);
    return SavedErrorLevel;
}

// vim:sw=4:ts=4:et:
