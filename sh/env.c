/**
 * @file sh/env.c
 *
 * Fetches values from the environment including emulated values
 *
 * Copyright (c) 2017 Malcolm J. Smith
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
 Returns TRUE if the specified character is an environment variable marker.

 @param Char The character to check.

 @return TRUE to indicate the character is an environment variable marker,
         FALSE if it is a regular character.
 */
BOOL
YoriShIsEnvironmentVariableChar(
    __in TCHAR Char
    )
{
    if (Char == '%') {
        return TRUE;
    }
    return FALSE;
}

/**
 Wrapper around the Win32 GetEnvironmentVariable call, but augmented with
 "magic" things that appear to be variables but aren't, including %CD% and
 %ERRORLEVEL%.

 @param Name The name of the environment variable to get.

 @param Variable Pointer to the buffer to receive the variable's contents.

 @param Size The length of the Variable parameter, in characters.

 @return The number of characters copied (without NULL), of if the buffer
         is too small, the number of characters needed (including NULL.)
 */
DWORD
YoriShGetEnvironmentVariable(
    __in LPCTSTR Name,
    __out_opt LPTSTR Variable,
    __in DWORD Size
    )
{
    if (_tcsicmp(Name, _T("CD")) == 0) {
        return GetCurrentDirectory(Size, Variable);
    } else if (tcsicmp(Name, _T("ERRORLEVEL")) == 0) {
        DWORD ReturnValue;
        if (Variable != NULL) {
            ReturnValue = YoriLibSPrintfS(Variable, Size, _T("%i"), g_ErrorLevel);
        } else {
            ReturnValue = 10;
        }
        return ReturnValue;
    } else if (tcsicmp(Name, _T("LASTJOB")) == 0) {
        DWORD ReturnValue;
        if (Variable != NULL) {
            ReturnValue = YoriLibSPrintfS(Variable, Size, _T("%i"), g_PreviousJobId);
        } else {
            ReturnValue = 10;
        }
        return ReturnValue;
    } else {
        return GetEnvironmentVariable(Name, Variable, Size);
    }
}

/**
 Capture the value from an environment variable, allocating a Yori string of
 appropriate size to contain the contents.

 @param Name Pointer to the name of the variable to obtain.

 @param Value On successful completion, populated with a newly allocated
        string containing the environment variable's contents.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShAllocateAndGetEnvironmentVariable(
    __in LPCTSTR Name,
    __out PYORI_STRING Value
    )
{
    DWORD LengthNeeded;

    LengthNeeded = YoriShGetEnvironmentVariable(Name, NULL, 0);
    if (LengthNeeded == 0) {
        YoriLibInitEmptyString(Value);
        return TRUE;
    }

    if (!YoriLibAllocateString(Value, LengthNeeded)) {
        return FALSE;
    }

    Value->LengthInChars = YoriShGetEnvironmentVariable(Name, Value->StartOfString, Value->LengthAllocated);
    if (Value->LengthInChars == 0 || Value->LengthInChars >= Value->LengthAllocated) {
        YoriLibFreeStringContents(Value);
        return FALSE;
    }

    return TRUE;
}


/**
 Returns the expanded form of an environment variable.  For variables that are
 not defined, the expanded form is the name of the variable itself, keeping
 the seperators in place.

 @param Name Pointer to a string specifying the environment variable name.
        Note this is not NULL terminated.

 @param Seperator The seperator character to use when the variable is not
        found.

 @param Result Optionally points to a buffer to receive the result.  If not
        specified, only the length required is returned.

 @return The number of characters copied (without NULL), of if the buffer
         is too small, the number of characters needed (including NULL.)
 */
DWORD
YoriShGetEnvironmentExpandedText(
    __in PYORI_STRING Name,
    __in TCHAR Seperator,
    __inout PYORI_STRING Result
    )
{
    DWORD EnvVarCopied;
    LPTSTR EnvVarName;
    DWORD ReturnValue;

    EnvVarName = YoriLibCStringFromYoriString(Name);
    if (EnvVarName == NULL) {
        return 0;
    }

    EnvVarCopied = YoriShGetEnvironmentVariable(EnvVarName, Result->StartOfString, Result->LengthAllocated);

    if (EnvVarCopied > 0) {
        ReturnValue = EnvVarCopied;
        if (Result->LengthAllocated > EnvVarCopied) {
            Result->LengthInChars = EnvVarCopied;
        }
    } else {

        if (Result->LengthAllocated > 2 + Name->LengthInChars) {
            Result->LengthInChars = YoriLibSPrintf(Result->StartOfString, _T("%c%y%c"), Seperator, Name, Seperator);
            ReturnValue = Result->LengthInChars;
        } else {
            ReturnValue = Name->LengthInChars + 2 + 1;
        }
    }

    YoriLibDereference(EnvVarName);

    return ReturnValue;
}

/**
 Expand the environment variables in a string and return the result.

 @param Expression Pointer to the string which may contain variables to
        expand.

 @param ResultingExpression On successful completion, updated to point to
        a string containing the expanded form.  This may be a pointer to
        the same string as Expression; the caller should call
        @ref YoriLibDereference on this value if it is different to
        Expression.

 @return TRUE to indicate variables were successfully expanded, or FALSE to
         indicate a failure to expand.
 */
BOOL
YoriShExpandEnvironmentVariables(
    __in PYORI_STRING Expression,
    __out PYORI_STRING ResultingExpression
    )
{
    DWORD SrcIndex;
    DWORD EndVarIndex;
    DWORD DestIndex;
    DWORD ExpandResult;
    BOOLEAN VariableExpanded;
    BOOLEAN AnyVariableExpanded = FALSE;
    YORI_STRING VariableName;
    YORI_STRING ExpandedVariable;

    //
    //  First, scan through looking for environment variables to expand, and
    //  count the size needed to perform expansion.
    //

    YoriLibInitEmptyString(&ExpandedVariable);
    YoriLibInitEmptyString(&VariableName);

    for (SrcIndex = 0, DestIndex = 0; SrcIndex < Expression->LengthInChars; SrcIndex++) {

        if (YoriLibIsEscapeChar(Expression->StartOfString[SrcIndex])) {
            SrcIndex++;
            DestIndex++;
            if (SrcIndex >= Expression->LengthInChars) {
                break;
            }
            DestIndex++;
            continue;
        }

        if (YoriShIsEnvironmentVariableChar(Expression->StartOfString[SrcIndex])) {
            VariableExpanded = FALSE;
            VariableName.StartOfString = &Expression->StartOfString[SrcIndex + 1];

            for (EndVarIndex = SrcIndex + 1; EndVarIndex < Expression->LengthInChars; EndVarIndex++) {
                if (YoriLibIsEscapeChar(Expression->StartOfString[EndVarIndex])) {
                    EndVarIndex++;
                    if (EndVarIndex >= Expression->LengthInChars) {
                        break;
                    }
                    continue;
                }

                if (YoriShIsEnvironmentVariableChar(Expression->StartOfString[EndVarIndex])) {

                    VariableName.LengthInChars = EndVarIndex - SrcIndex - 1;
                    ExpandResult = YoriShGetEnvironmentExpandedText(&VariableName,
                                                                    Expression->StartOfString[SrcIndex],
                                                                    &ExpandedVariable);
                    if (ExpandResult == 0) {
                        return FALSE;
                    }

                    DestIndex += ExpandResult;
                    SrcIndex = EndVarIndex;
                    VariableExpanded = TRUE;
                    AnyVariableExpanded = TRUE;
                    break;
                }
            }

            if (!VariableExpanded) {
                DestIndex += (EndVarIndex - SrcIndex);
                SrcIndex = EndVarIndex;
                if (SrcIndex >= Expression->LengthInChars) {
                    break;
                }
            }
        } else {
            DestIndex++;
        }
    }

    //
    //  If no environment variables were found, we're done.
    //

    if (!AnyVariableExpanded) {
        memcpy(ResultingExpression, Expression, sizeof(YORI_STRING));
        return TRUE;
    }

    //
    //  If they were found, allocate a buffer and apply the same algorithm as
    //  before, this time populating the buffer.
    //

    DestIndex++;
    if (!YoriLibAllocateString(ResultingExpression, DestIndex)) {
        return FALSE;
    }

    for (SrcIndex = 0, DestIndex = 0; SrcIndex < Expression->LengthInChars; SrcIndex++) {

        if (YoriLibIsEscapeChar(Expression->StartOfString[SrcIndex])) {
            ResultingExpression->StartOfString[DestIndex] = Expression->StartOfString[SrcIndex];
            SrcIndex++;
            DestIndex++;
            if (SrcIndex >= Expression->LengthInChars) {
                break;
            }
            ResultingExpression->StartOfString[DestIndex] = Expression->StartOfString[SrcIndex];
            DestIndex++;
            continue;
        }

        if (YoriShIsEnvironmentVariableChar(Expression->StartOfString[SrcIndex])) {
            VariableExpanded = FALSE;
            VariableName.StartOfString = &Expression->StartOfString[SrcIndex + 1];
            for (EndVarIndex = SrcIndex + 1; EndVarIndex < Expression->LengthInChars; EndVarIndex++) {
                if (YoriLibIsEscapeChar(Expression->StartOfString[EndVarIndex])) {
                    EndVarIndex++;
                    if (EndVarIndex >= Expression->LengthInChars) {
                        break;
                    }
                    continue;
                }

                if (YoriShIsEnvironmentVariableChar(Expression->StartOfString[EndVarIndex])) {
                    VariableName.LengthInChars = EndVarIndex - SrcIndex - 1;
                    ExpandedVariable.StartOfString = &ResultingExpression->StartOfString[DestIndex];
                    ExpandedVariable.LengthAllocated = ResultingExpression->LengthAllocated - DestIndex;
                    ExpandResult = YoriShGetEnvironmentExpandedText(&VariableName,
                                                                    Expression->StartOfString[SrcIndex],
                                                                    &ExpandedVariable);
                    if (ExpandResult == 0) {
                        return FALSE;
                    }

                    SrcIndex = EndVarIndex;
                    DestIndex += ExpandResult;
                    VariableExpanded = TRUE;
                    break;
                }
            }

            if (!VariableExpanded) {
                memcpy(&ResultingExpression->StartOfString[DestIndex], &Expression->StartOfString[SrcIndex], (EndVarIndex - SrcIndex) * sizeof(TCHAR));
                DestIndex += (EndVarIndex - SrcIndex);
                SrcIndex = EndVarIndex;
                if (SrcIndex >= Expression->LengthInChars) {
                    break;
                }
            }
        } else {
            ResultingExpression->StartOfString[DestIndex] = Expression->StartOfString[SrcIndex];
            DestIndex++;
        }
    }

    ResultingExpression->StartOfString[DestIndex] = '\0';
    ResultingExpression->LengthInChars = DestIndex;
    return TRUE;
}



// vim:sw=4:ts=4:et:
