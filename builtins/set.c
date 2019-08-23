/**
 * @file builtins/set.c
 *
 * Yori shell environment variable control
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
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strSetHelpText[] =
        "\n"
        "Displays or updates environment variables.\n"
        "\n"
        "SET -license\n"
        "SET [<variable prefix to display>]\n"
        "SET [-e | -i | -r] <variable>=<value>\n"
        "SET <variable to delete>=\n"
        "\n"
        "   -e             Include the string in a semicolon delimited variable at the end\n"
        "   -i             Include the string in a semicolon delimited variable at the start\n"
        "   -r             Remove the string from a semicolon delimited variable\n";

/**
 Display usage text to the user.
 */
BOOL
SetHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Set %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSetHelpText);
    return TRUE;
}

/**
 Normally when running a command if a variable does not define any contents,
 the variable name is preserved in the command.  This does not occur with the
 set command, where if a variable contains no contents the variable name is
 removed from the result.  Unfortunately this is ambiguous when the variable
 name is found, because we don't know if the variable was not expanded by the
 shell due to no contents or because it was escaped.  So if it has no contents
 it is removed here.  But this still can't distinguish between an escaped
 variable that points to something and an escaped variable which points to
 nothing, both of which presumably should be retained.

 @param Value Pointer to a NULL terminated string containing the value part
        of the set command (ie., what to set a variable to.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetRemoveEmptyVariables(
    __inout LPTSTR Value
    )
{
    DWORD StartOfVariableName;
    DWORD ReadIndex = 0;
    DWORD WriteIndex = 0;

    while (TRUE) {
        if (YoriLibIsEscapeChar(Value[ReadIndex])) {

            //
            //  If the character is an escape, copy it to the destination
            //  and skip the check for the environment variable character
            //  so we fall into the below case and copy the next character
            //  too
            //

            if (ReadIndex != WriteIndex) {
                Value[WriteIndex] = Value[ReadIndex];
            }
            ReadIndex++;
            WriteIndex++;

        } else if (Value[ReadIndex] == '%') {

            //
            //  We found the first %, scan ahead looking for the next
            //  one
            //

            StartOfVariableName = ReadIndex;
            do {
                if (YoriLibIsEscapeChar(Value[ReadIndex])) {
                    ReadIndex++;
                }
                ReadIndex++;
            } while (Value[ReadIndex] != '%' && Value[ReadIndex] != '\0');

            //
            //  If we found a well formed variable, check if it refers
            //  to anything.  If it does, that means the shell didn't
            //  expand it because it was escaped, so preserve it here.
            //  If it doesn't, that means it may not refer to anything,
            //  so remove it.
            //

            if (Value[ReadIndex] == '%') {
                YORI_STRING YsVariableName;
                YORI_STRING YsVariableValue;

                YsVariableName.StartOfString = &Value[StartOfVariableName + 1];
                YsVariableName.LengthInChars = ReadIndex - StartOfVariableName - 1;

                ReadIndex++;
                YoriLibInitEmptyString(&YsVariableValue);
                if (YoriCallGetEnvironmentVariable(&YsVariableName, &YsVariableValue)) {
                    if (WriteIndex != StartOfVariableName) {
                        memmove(&Value[WriteIndex], &Value[StartOfVariableName], (ReadIndex - StartOfVariableName) * sizeof(TCHAR));
                    }
                    WriteIndex += (ReadIndex - StartOfVariableName);
                    YoriCallFreeYoriString(&YsVariableValue);
                }
                continue;
            } else {
                ReadIndex = StartOfVariableName;
            }
        }
        if (ReadIndex != WriteIndex) {
            Value[WriteIndex] = Value[ReadIndex];
        }
        if (Value[ReadIndex] == '\0') {
            break;
        }
        ReadIndex++;
        WriteIndex++;
    }
    return TRUE;
}

/**
 Perform an in place removal of escape characters.  This function assumes the
 string is NULL terminated and will leave it NULL terminated on completion.

 @param String Pointer to the string to remove escape characters from.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetRemoveEscapes(
    __inout PYORI_STRING String
    )
{
    DWORD CharIndex;
    DWORD DestIndex;

    for (CharIndex = 0, DestIndex = 0; CharIndex < String->LengthInChars; CharIndex++, DestIndex++) {
        if (YoriLibIsEscapeChar(String->StartOfString[CharIndex])) {
            CharIndex++;
            if (CharIndex >= String->LengthInChars) {
                break;
            }
        }

        if (CharIndex != DestIndex) {
            String->StartOfString[DestIndex] = String->StartOfString[CharIndex];
        }
    }
    String->StartOfString[DestIndex] = '\0';
    String->LengthInChars = DestIndex;
    return TRUE;
}

/**
 Main entrypoint for the set builtin.

 @param ArgC The number of arguments.

 @param ArgV An array of string arguments.

 @return Exit code, typically zero for success and nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_SET(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    BOOL AppendComponent = FALSE;
    BOOL IncludeComponent = FALSE;
    BOOL RemoveComponent = FALSE;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;

    DWORD UnescapedArgC;
    PYORI_STRING UnescapedArgV;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    if (!YoriCallGetEscapedArguments(&UnescapedArgC, &UnescapedArgV)) {
        UnescapedArgC = ArgC;
        UnescapedArgV = ArgV;
    }

    for (i = 1; i < UnescapedArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&UnescapedArgV[i]));

        if (YoriLibIsCommandLineOption(&UnescapedArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SetHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                if (!RemoveComponent && !IncludeComponent) {
                    AppendComponent = TRUE;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                if (!RemoveComponent && !AppendComponent) {
                    IncludeComponent = TRUE;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                if (!IncludeComponent && !AppendComponent) {
                    RemoveComponent = TRUE;
                    ArgumentUnderstood = TRUE;
                }
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
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &UnescapedArgV[i]);
        }
    }

    if (StartArg == 0) {
        YORI_STRING EnvironmentStrings;
        LPTSTR ThisVar;
        DWORD VarLen;

        if (!YoriLibGetEnvironmentStrings(&EnvironmentStrings)) {
            return EXIT_FAILURE;
        }
        ThisVar = EnvironmentStrings.StartOfString;
        while (*ThisVar != '\0') {
            VarLen = _tcslen(ThisVar);
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s\n"), ThisVar);
            ThisVar += VarLen;
            ThisVar++;
        }
        YoriLibFreeStringContents(&EnvironmentStrings);
    } else {
        LPTSTR Variable;
        LPTSTR Value;
        YORI_STRING CmdLine;
        YORI_STRING YsVariable;

        if (!YoriLibBuildCmdlineFromArgcArgv(UnescapedArgC - StartArg, &UnescapedArgV[StartArg], FALSE, &CmdLine)) {
            return EXIT_FAILURE;
        }

        YoriLibInitEmptyString(&YsVariable);
        YsVariable.StartOfString = CmdLine.StartOfString;
        Variable = CmdLine.StartOfString;

        //
        //  At this point escapes are still present but it's never valid to
        //  have an '=' in a variable name, escape or not.
        //

        Value = _tcschr(Variable, '=');
        if (Value) {
            *Value = '\0';
            Value++;
            YsVariable.LengthAllocated = (DWORD)(Value - CmdLine.StartOfString);
            YsVariable.LengthInChars = YsVariable.LengthAllocated - 1;
        } else {
            YsVariable.LengthAllocated = CmdLine.LengthAllocated;
            YsVariable.LengthInChars = CmdLine.LengthInChars;
        }
        SetRemoveEscapes(&YsVariable);

        if (Value != NULL) {

            //
            //  Scan through the value looking for any unexpanded environment
            //  variables.
            //

            SetRemoveEmptyVariables(Value);
            
            if (*Value == '\0') {
                YoriCallSetEnvironmentVariable(&YsVariable, NULL);
            } else {
                YORI_STRING YsValue;
                YORI_STRING CombinedValue;
                YoriLibConstantString(&YsValue, Value);
                SetRemoveEscapes(&YsValue);
                if (IncludeComponent) {
                    if (YoriLibAddEnvironmentComponentReturnString(Variable, &YsValue, TRUE, &CombinedValue)) {
                        YoriCallSetEnvironmentVariable(&YsVariable, &CombinedValue);
                        YoriLibFreeStringContents(&CombinedValue);
                    }
                } else if (AppendComponent) {
                    if (YoriLibAddEnvironmentComponentReturnString(Variable, &YsValue, FALSE, &CombinedValue)) {
                        YoriCallSetEnvironmentVariable(&YsVariable, &CombinedValue);
                        YoriLibFreeStringContents(&CombinedValue);
                    }
                } else if (RemoveComponent) {
                    if (YoriLibRemoveEnvironmentComponentReturnString(Variable, &YsValue, &CombinedValue)) {
                        if (CombinedValue.LengthAllocated > 0) {
                            YoriCallSetEnvironmentVariable(&YsVariable, &CombinedValue);
                        } else {
                            YoriCallSetEnvironmentVariable(&YsVariable, NULL);
                        }
                        YoriLibFreeStringContents(&CombinedValue);
                    }
                } else {
                    YoriCallSetEnvironmentVariable(&YsVariable, &YsValue);
                }
            }
        } else {
            YORI_STRING UserVar;
            YORI_STRING EnvironmentStrings;
            LPTSTR ThisVar;
    
            if (!YoriLibGetEnvironmentStrings(&EnvironmentStrings)) {
                YoriLibFreeStringContents(&CmdLine);
                return EXIT_FAILURE;
            }
            ThisVar = EnvironmentStrings.StartOfString;
            YoriLibConstantString(&UserVar, Variable);
            while (*ThisVar != '\0') {
                if (YoriLibCompareStringWithLiteralInsensitiveCount(&UserVar, ThisVar, UserVar.LengthInChars) == 0) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s\n"), ThisVar);
                }
                ThisVar += _tcslen(ThisVar);
                ThisVar++;
            }
            YoriLibFreeStringContents(&EnvironmentStrings);
        }
        YoriLibFreeStringContents(&CmdLine);
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
