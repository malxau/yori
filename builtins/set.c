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

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

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
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
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

        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], FALSE, &CmdLine)) {
            return EXIT_FAILURE;
        }

        YoriLibInitEmptyString(&YsVariable);
        YsVariable.StartOfString = CmdLine.StartOfString;
        Variable = CmdLine.StartOfString;
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

        if (Value != NULL) {

            DWORD StartOfVariableName;
            DWORD ReadIndex = 0;
            DWORD WriteIndex = 0;

            //
            //  Scan through the value looking for any unexpanded environment
            //  variables.
            //

            while (TRUE) {
                if (Value[ReadIndex] == '%') {

                    //
                    //  We found the first %, scan ahead looking for the next
                    //  one
                    //

                    StartOfVariableName = ReadIndex;
                    do {
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
                        LPTSTR VariableName;
                        YORI_STRING YsVariableName;

                        YsVariableName.StartOfString = &Value[StartOfVariableName + 1];
                        YsVariableName.LengthInChars = ReadIndex - StartOfVariableName - 1;

                        VariableName = YoriLibMalloc((YsVariableName.LengthInChars + 1) * sizeof(TCHAR));

                        ReadIndex++;
                        if (VariableName != NULL) {

                            YoriLibSPrintf(VariableName, _T("%y"), &YsVariableName);
                            if (GetEnvironmentVariable(VariableName, NULL, 0) > 0) {
                                if (WriteIndex != StartOfVariableName) {
                                    memmove(&Value[WriteIndex], &Value[StartOfVariableName], (ReadIndex - StartOfVariableName) * sizeof(TCHAR));
                                }
                                WriteIndex += (ReadIndex - StartOfVariableName);
                            }
                        }
                        YoriLibFree(VariableName);
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
            
            if (*Value == '\0') {
                YoriCallSetEnvironmentVariable(&YsVariable, NULL);
            } else {
                YORI_STRING YsValue;
                YORI_STRING CombinedValue;
                YoriLibConstantString(&YsValue, Value);
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
