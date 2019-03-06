/**
 * @file sh/prompt.c
 *
 * Yori shell prompt display
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

#include "yori.h"

/**
 Set to TRUE once the process has queried whether it is running as part of the
 administrator group.  FALSE at start.
 */
BOOL YoriShPromptAdminDetermined;

/**
 Set to TRUE if the process has determined that it is running as part of the
 administrator group.
 */
BOOL YoriShPromptAdminPresent;

/**
 Return TRUE if the process is running as part of the administrator group,
 FALSE if not.
 */
BOOL
YoriShPromptIsAdmin()
{
    YORI_STRING AdminName;

    if (YoriShPromptAdminDetermined) {
        return YoriShPromptAdminPresent;
    }

    YoriLibConstantString(&AdminName, _T("Administrators"));

    YoriLibIsCurrentUserInGroup(&AdminName, &YoriShPromptAdminPresent);
    YoriShPromptAdminDetermined = TRUE;

    return YoriShPromptAdminPresent;
}

/**
 Expand variables in a prompt environment variable to form a displayable
 string.

 @param OutputString The string to output the result of variable expansion to.

 @param VariableName The name of the variable that requires expansion.

 @param Context Ignored.

 @return The number of characters populated or the number of characters
         required if the buffer is too small.
 */
DWORD
YoriShExpandPrompt(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    DWORD CharsNeeded = 0;

    UNREFERENCED_PARAMETER(Context);

    if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("A")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = '&';
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("B")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = '|';
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("C")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = '(';
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("E")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = 27;
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("F")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = ')';
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("G")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = '>';
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("G_OR_ADMIN_G")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            if (YoriShPromptIsAdmin()) {
                OutputString->StartOfString[0] = 0xBB;
            } else {
                OutputString->StartOfString[0] = '>';
            }
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("L")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = '<';
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("P")) == 0) {
        CharsNeeded = GetCurrentDirectory(0, NULL);
        if (OutputString->LengthAllocated > CharsNeeded) {
            CharsNeeded = GetCurrentDirectory(OutputString->LengthAllocated, OutputString->StartOfString);
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("PID")) == 0) {
        CharsNeeded = 10;
        if (OutputString->LengthAllocated > CharsNeeded) {
            CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%x"), GetCurrentProcessId());
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("Q")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = '=';
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("S")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = ' ';
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("_")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = '\n';
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("$")) == 0) {
        CharsNeeded = 1;
        if (OutputString->LengthAllocated > CharsNeeded) {
            OutputString->StartOfString[0] = '$';
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(VariableName, _T("+")) == 0) {
        CharsNeeded = YoriShGlobal.PromptRecursionDepth;
        if (OutputString->LengthAllocated > YoriShGlobal.PromptRecursionDepth) {
            DWORD Index;
            for (Index = 0; Index < YoriShGlobal.PromptRecursionDepth; Index++) {
                OutputString->StartOfString[Index] = '+';
            }
        }
    }

    return CharsNeeded;
}

/**
 Displays the current prompt string on the console.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShDisplayPrompt()
{
    DWORD EnvVarLength;
    YORI_STRING PromptVar;
    YORI_STRING PromptAfterBackquoteExpansion;
    YORI_STRING PromptAfterEnvExpansion;
    YORI_STRING DisplayString;
    PYORI_STRING StringToUse;
    DWORD SavedErrorLevel = YoriShGlobal.ErrorLevel;


    //
    //  Don't update taskbar UI while executing processes executed as part of
    //  the prompt or title
    //

    YoriShGlobal.SuppressTaskUi = TRUE;

    EnvVarLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIPROMPT"), NULL, 0);
    if (EnvVarLength > 0) {
        if (YoriLibAllocateString(&PromptVar, EnvVarLength)) {

            YoriLibInitEmptyString(&PromptAfterBackquoteExpansion);
            YoriLibInitEmptyString(&PromptAfterEnvExpansion);
            YoriLibInitEmptyString(&DisplayString);

            //
            //  Get the raw prompt expression.
            //

            PromptVar.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIPROMPT"), PromptVar.StartOfString, PromptVar.LengthAllocated);
            StringToUse = &PromptVar;

            //
            //  If there are any backquotes, expand them.  If not, or if
            //  expansion fails, we'll end up pointing at the previous string.
            //

            if (YoriShExpandBackquotes(&PromptVar, &PromptAfterBackquoteExpansion)) {
                StringToUse = &PromptAfterBackquoteExpansion;
            }

            //
            //  If there are any environment variables, expand them.  If not,
            //  or if expansion fails, we'll end up pointing at the previous
            //  string.
            //

            if (YoriShExpandEnvironmentVariables(StringToUse, &PromptAfterEnvExpansion)) {
                StringToUse = &PromptAfterEnvExpansion;
            }

            //
            //  Expand any prompt command variables.
            //

            YoriLibExpandCommandVariables(StringToUse, '$', FALSE, YoriShExpandPrompt, NULL, &DisplayString);

            //
            //  Display the result.
            //

            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
                YoriLibFreeStringContents(&DisplayString);
            }

            //
            //  If any step involved generating a new string, free those now.
            //

            if (PromptAfterEnvExpansion.StartOfString != PromptAfterBackquoteExpansion.StartOfString) {
                YoriLibFreeStringContents(&PromptAfterEnvExpansion);
            }
            if (PromptAfterBackquoteExpansion.StartOfString != PromptVar.StartOfString) {
                YoriLibFreeStringContents(&PromptAfterBackquoteExpansion);
            }
            YoriLibFreeStringContents(&PromptVar);
        }
    } else {

        LPTSTR PromptString;
        EnvVarLength = GetCurrentDirectory(0, NULL);

        //
        //  If YORIPROMPT wasn't set, fall back to something generic.
        //

        PromptString = YoriLibMalloc((EnvVarLength + sizeof("E[35;1m") + sizeof("E[0m>")) * sizeof(TCHAR));
        if (PromptString != NULL) {
            DWORD CharsCopied;
            DWORD CurrentPos;

            CurrentPos = YoriLibSPrintf(PromptString, _T("%c[35;1m"), 27);
            CharsCopied = GetCurrentDirectory(EnvVarLength, &PromptString[CurrentPos]);
            CurrentPos += CharsCopied;
            YoriLibSPrintf(&PromptString[CurrentPos], _T("%c[0m>"), 27);
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s"), PromptString);
            YoriLibFree(PromptString);
        }
    }

    //
    //  If we have a dynamic title, do that too.
    //

    EnvVarLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORITITLE"), NULL, 0);
    if (EnvVarLength > 0) {
        if (YoriLibAllocateString(&PromptVar, EnvVarLength)) {

            YoriLibInitEmptyString(&PromptAfterBackquoteExpansion);
            YoriLibInitEmptyString(&PromptAfterEnvExpansion);
            YoriLibInitEmptyString(&DisplayString);

            //
            //  Get the raw title expression.
            //

            PromptVar.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORITITLE"), PromptVar.StartOfString, PromptVar.LengthAllocated);
            StringToUse = &PromptVar;

            //
            //  If there are any backquotes, expand them.  If not, or if
            //  expansion fails, we'll end up pointing at the previous string.
            //

            if (YoriShExpandBackquotes(&PromptVar, &PromptAfterBackquoteExpansion)) {
                StringToUse = &PromptAfterBackquoteExpansion;
            }

            //
            //  If there are any environment variables, expand them.  If not,
            //  or if expansion fails, we'll end up pointing at the previous
            //  string.
            //

            if (YoriShExpandEnvironmentVariables(StringToUse, &PromptAfterEnvExpansion)) {
                StringToUse = &PromptAfterEnvExpansion;
            }

            //
            //  Expand any prompt command variables.
            //

            YoriLibExpandCommandVariables(StringToUse, '$', FALSE, YoriShExpandPrompt, NULL, &DisplayString);

            //
            //  Display the result.
            //

            if (DisplayString.StartOfString != NULL) {
                SetConsoleTitle(DisplayString.StartOfString);
                YoriLibFreeStringContents(&DisplayString);
            }

            //
            //  If any step involved generating a new string, free those now.
            //

            if (PromptAfterEnvExpansion.StartOfString != PromptAfterBackquoteExpansion.StartOfString) {
                YoriLibFreeStringContents(&PromptAfterEnvExpansion);
            }
            if (PromptAfterBackquoteExpansion.StartOfString != PromptVar.StartOfString) {
                YoriLibFreeStringContents(&PromptAfterBackquoteExpansion);
            }
            YoriLibFreeStringContents(&PromptVar);
        }
    }

    YoriShGlobal.SuppressTaskUi = FALSE;

    //
    //  Restore the error level since the prompt or title may execute
    //  commands which alter it.
    //

    YoriShGlobal.ErrorLevel = SavedErrorLevel;

    return TRUE;
}

// vim:sw=4:ts=4:et:
