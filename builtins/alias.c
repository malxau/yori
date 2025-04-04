/**
 * @file builtins/alias.c
 *
 * Yori shell alias control
 *
 * Copyright (c) 2017-2023 Malcolm J. Smith
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
        "ALIAS [-s] [<alias>=<value>]\n"
        "ALIAS <alias to delete>=\n"
        "\n"
        "   -s             Display or set a system alias\n";

/**
 Display usage text to the user.
 */
BOOL
AliasHelp(VOID)
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
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    BOOLEAN SystemAlias = FALSE;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                AliasHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2023"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("s")) == 0) {
                SystemAlias = TRUE;
                ArgumentUnderstood = TRUE;
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
        YORI_ALLOC_SIZE_T VarLen;
        BOOL Result;

        if (SystemAlias) {
            Result = YoriCallGetSystemAliasStrings(&AliasStrings);
        } else {
            Result = YoriCallGetAliasStrings(&AliasStrings);
        }

        if (Result) {
            ThisVar = AliasStrings.StartOfString;
            while (*ThisVar != '\0') {
                VarLen = (YORI_ALLOC_SIZE_T)_tcslen(ThisVar);
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s\n"), ThisVar);
                ThisVar += VarLen;
                ThisVar++;
            }
            YoriCallFreeYoriString(&AliasStrings);
        }
    } else {

        YORI_STRING Variable;
        YORI_STRING Value;
        BOOLEAN ValueSpecified;

        if (!YoriLibArgArrayToVariableValue(ArgC - StartArg, &ArgV[StartArg], &Variable, &ValueSpecified, &Value)) {
            return EXIT_FAILURE;
        }

        if (Value.LengthInChars > 0) {
            YORI_STRING ExpandedAlias;
            YoriLibInitEmptyString(&ExpandedAlias);
            if (YoriCallExpandAlias(&Value, &ExpandedAlias)) {
                if (SystemAlias) {
                    YoriCallAddSystemAlias(&Variable, &ExpandedAlias);
                } else {
                    YoriCallAddAlias(&Variable, &ExpandedAlias);
                }
                YoriCallFreeYoriString(&ExpandedAlias);
            } else {
                if (SystemAlias) {
                    YoriCallAddSystemAlias(&Variable, &Value);
                } else {
                    YoriCallAddAlias(&Variable, &Value);
                }
            }
        } else {
            YoriCallDeleteAlias(&Variable);
        }
        YoriLibFreeStringContents(&Value);
        YoriLibFreeStringContents(&Variable);
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
