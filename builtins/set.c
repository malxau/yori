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
SetHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Set %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSetHelpText);
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

    DWORD EscapedArgC;
    PYORI_STRING EscapedArgV;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    if (!YoriCallGetEscapedArguments(&EscapedArgC, &EscapedArgV)) {
        EscapedArgC = ArgC;
        EscapedArgV = ArgV;
    }

    for (i = 1; i < EscapedArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&EscapedArgV[i]));

        if (YoriLibIsCommandLineOption(&EscapedArgV[i], &Arg)) {

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
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &EscapedArgV[i]);
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
        YORI_STRING Value;
        YORI_STRING CmdLine;
        YORI_STRING Variable;

        if (!YoriLibBuildCmdlineFromArgcArgv(EscapedArgC - StartArg, &EscapedArgV[StartArg], FALSE, FALSE, &CmdLine)) {
            return EXIT_FAILURE;
        }

        memcpy(&Variable, &CmdLine, sizeof(YORI_STRING));

        //
        //  At this point escapes are still present but it's never valid to
        //  have an '=' in a variable name, escape or not.
        //

        YoriLibInitEmptyString(&Value);
        Value.StartOfString = YoriLibFindLeftMostCharacter(&Variable, '=');
        if (Value.StartOfString) {
            Value.StartOfString[0] = '\0';
            Value.StartOfString++;
            Variable.LengthAllocated = (DWORD)(Value.StartOfString - CmdLine.StartOfString);
            Variable.LengthInChars = Variable.LengthAllocated - 1;
            Value.LengthInChars = CmdLine.LengthInChars - Variable.LengthAllocated;
            Value.LengthAllocated = CmdLine.LengthAllocated - Variable.LengthAllocated;
        }

        SetRemoveEscapes(&Variable);

        if (Value.StartOfString != NULL) {

            //
            //  Scan through the value looking for any unexpanded environment
            //  variables.
            //

            YoriLibBuiltinRemoveEmptyVariables(&Value);
            Value.StartOfString[Value.LengthInChars] = '\0';
            
            if (Value.LengthInChars == 0) {
                YoriCallSetEnvironmentVariable(&Variable, NULL);
            } else {
                YORI_STRING CombinedValue;
                SetRemoveEscapes(&Value);
                if (IncludeComponent) {
                    if (YoriLibAddEnvironmentComponentReturnString(&Variable, &Value, TRUE, &CombinedValue)) {
                        YoriCallSetEnvironmentVariable(&Variable, &CombinedValue);
                        YoriLibFreeStringContents(&CombinedValue);
                    }
                } else if (AppendComponent) {
                    if (YoriLibAddEnvironmentComponentReturnString(&Variable, &Value, FALSE, &CombinedValue)) {
                        YoriCallSetEnvironmentVariable(&Variable, &CombinedValue);
                        YoriLibFreeStringContents(&CombinedValue);
                    }
                } else if (RemoveComponent) {
                    if (YoriLibRemoveEnvironmentComponentReturnString(&Variable, &Value, &CombinedValue)) {
                        if (CombinedValue.LengthAllocated > 0) {
                            YoriCallSetEnvironmentVariable(&Variable, &CombinedValue);
                        } else {
                            YoriCallSetEnvironmentVariable(&Variable, NULL);
                        }
                        YoriLibFreeStringContents(&CombinedValue);
                    }
                } else {
                    YoriCallSetEnvironmentVariable(&Variable, &Value);
                }
            }
        } else {
            YORI_STRING EnvironmentStrings;
            LPTSTR ThisVar;
    
            if (!YoriLibGetEnvironmentStrings(&EnvironmentStrings)) {
                YoriLibFreeStringContents(&CmdLine);
                return EXIT_FAILURE;
            }
            ThisVar = EnvironmentStrings.StartOfString;
            while (*ThisVar != '\0') {
                if (YoriLibCompareStringWithLiteralInsensitiveCount(&Variable, ThisVar, Variable.LengthInChars) == 0) {
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
