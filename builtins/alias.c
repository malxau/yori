/**
 * @file builtins/alias.c
 *
 * Yori shell alias control
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
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strAliasHelpText[] =
        "\n"
        "Displays or updates command aliases.\n"
        "\n"
        "ALIAS -license\n"
        "ALIAS [<alias>=<value>]\n"
        "ALIAS <alias to delete>=\n";

/**
 Display usage text to the user.
 */
BOOL
AliasHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Alias %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strAliasHelpText);
    return TRUE;
}

/**
 Add, update or delete a yori shell alias builtin command.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_ALIAS(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
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
                AliasHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
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

    if (StartArg == 0) {
        YORI_STRING AliasStrings;
        LPTSTR ThisVar;
        DWORD VarLen;

        if (YoriCallGetAliasStrings(&AliasStrings)) {
            ThisVar = AliasStrings.StartOfString;
            while (*ThisVar != '\0') {
                VarLen = _tcslen(ThisVar);
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s\n"), ThisVar);
                ThisVar += VarLen;
                ThisVar++;
            }
            YoriCallFreeYoriString(&AliasStrings);
        }
    } else {
        YORI_STRING Variable;
        YORI_STRING Value;
        YORI_STRING ExpandedAlias;
        YORI_STRING CmdLine;

        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], FALSE, &CmdLine)) {
            return EXIT_FAILURE;
        }

        YoriLibInitEmptyString(&Variable);
        YoriLibInitEmptyString(&Value);
        Value.StartOfString = YoriLibFindLeftMostCharacter(&CmdLine, '=');
        if (Value.StartOfString) {
            Variable.StartOfString = CmdLine.StartOfString;
            Variable.LengthInChars = (DWORD)(Value.StartOfString - CmdLine.StartOfString);
            Value.StartOfString[0] = '\0';
            Value.StartOfString++;
            Value.LengthInChars = CmdLine.LengthInChars - Variable.LengthInChars - 1;
        } else {
            Variable.StartOfString = CmdLine.StartOfString;
            Variable.LengthInChars = CmdLine.LengthInChars;
        }

        if (Value.LengthInChars > 0) {
            YoriLibInitEmptyString(&ExpandedAlias);
            if (YoriCallExpandAlias(&Value, &ExpandedAlias)) {
                YoriCallAddAlias(&Variable, &ExpandedAlias);
                YoriCallFreeYoriString(&ExpandedAlias);
            } else {
                YoriCallAddAlias(&Variable, &Value);
            }
        } else {
            YoriCallDeleteAlias(&Variable);
        }
        YoriLibFreeStringContents(&CmdLine);
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
