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
 Take an array of arguments, which may contain an equals sign somewhere in the
 middle.  Convert these into a variable name (left of equals) and value (right
 of equals.)  Quotes are preserved in the value component, but not in the
 variable component.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @param Variable On successful completion, updated to contain a variable name.
        This string is allocated within this routine and should be freed with
        @ref YoriLibFreeStringContents .

 @param Value On successful completion, updated to contain a value.  This
        string is allocated within this routine and should be freed with
        @ref YoriLibFreeStringContents .

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AliasArgArrayToVariableValue(
    __in DWORD ArgC,
    __in PYORI_STRING ArgV,
    __out PYORI_STRING Variable,
    __out PYORI_STRING Value
    )
{
    DWORD ArgWithEquals;
    DWORD EqualsOffset;
    LPTSTR Equals;
    YORI_STRING SavedArg;
    DWORD i;

    Equals = NULL;
    ArgWithEquals = 0;
    EqualsOffset = 0;

    for (i = 0; i < ArgC; i++) {
        Equals = YoriLibFindLeftMostCharacter(&ArgV[i], '=');
        if (Equals != NULL) {
            ArgWithEquals = i;
            EqualsOffset = (DWORD)(Equals - ArgV[i].StartOfString);
            break;
        }
    }

    YoriLibInitEmptyString(Variable);
    YoriLibInitEmptyString(Value);

    //
    //  If there's no equals, treat everything as the variable component.
    //

    if (Equals == NULL) {
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC, ArgV, FALSE, FALSE, Variable)) {
            return FALSE;
        }

        return TRUE;
    }

    //
    //  What follows interprets the single ArgV array as two arrays, with one
    //  component being shared across both (different substrings on different
    //  sides of the equals sign.)  Currently this works by manipulating that
    //  component (changing the input array.)  In order to not confuse the
    //  caller, save and restore the component being modified.
    //

    YoriLibInitEmptyString(&SavedArg);
    SavedArg.StartOfString = ArgV[ArgWithEquals].StartOfString;
    SavedArg.LengthInChars = ArgV[ArgWithEquals].LengthInChars;

    //
    //  Truncate the arg containing the equals, and build a string for it.
    //  This is the variable name.  Note quotes are not inserted here.
    //

    ArgV[ArgWithEquals].LengthInChars = EqualsOffset;
    if (!YoriLibBuildCmdlineFromArgcArgv(ArgWithEquals + 1, ArgV, FALSE, FALSE, Variable)) {
        return FALSE;
    }

    //
    //  If there's anything left after the equals sign, start from that
    //  argument, after the equals; if not, start from the next one.
    //  If starting from the next, indicate there's nothing to restore
    //  from the one we just skipped over.
    //

    if (SavedArg.LengthInChars - EqualsOffset - 1 > 0) {
        ArgV[ArgWithEquals].StartOfString = ArgV[ArgWithEquals].StartOfString + EqualsOffset + 1;
        ArgV[ArgWithEquals].LengthInChars = SavedArg.LengthInChars - EqualsOffset - 1;
    } else {
        ArgV[ArgWithEquals].LengthInChars = SavedArg.LengthInChars;
        ArgWithEquals++;

        SavedArg.StartOfString = NULL;
    }

    //
    //  If there are any arguments, construct the value string.
    //

    if (ArgC - ArgWithEquals > 0) {
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - ArgWithEquals, &ArgV[ArgWithEquals], TRUE, FALSE, Value)) {
            YoriLibFreeStringContents(Variable);
            return FALSE;
        }
    }

    //
    //  If there's something to restore, go restore it.
    //

    if (SavedArg.StartOfString != NULL) {
        ArgV[ArgWithEquals].StartOfString = SavedArg.StartOfString;
        ArgV[ArgWithEquals].LengthInChars = SavedArg.LengthInChars;
    }

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
    BOOLEAN SystemAlias = FALSE;

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
                YoriLibDisplayMitLicense(_T("2017-2023"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
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
        DWORD VarLen;
        BOOL Result;

        if (SystemAlias) {
            Result = YoriCallGetSystemAliasStrings(&AliasStrings);
        } else {
            Result = YoriCallGetAliasStrings(&AliasStrings);
        }

        if (Result) {
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

        if (!AliasArgArrayToVariableValue(ArgC - StartArg, &ArgV[StartArg], &Variable, &Value)) {
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
