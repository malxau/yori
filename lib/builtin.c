/**
 * @file lib/builtin.c
 *
 * Yori routines that are specific to builtin modules
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

#include "yoripch.h"
#include "yorilib.h"
#include "yoricall.h"

/**
 Retore a set of environment strings into the current environment.  This
 implies removing all currently defined variables and replacing them with
 the specified set.  This version of the routine is specific to builtin
 modules because it manipulates the environment through the YoriCall
 interface.  Note that the input buffer is modified temporarily (ie.,
 it is not immutable.)

 @param NewEnvironment Pointer to the new environment strings to apply.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibBuiltinSetEnvironmentStrings(
    __in PYORI_STRING NewEnvironment
    )
{
    YORI_STRING CurrentEnvironment;
    YORI_STRING VariableName;
    YORI_STRING ValueName;
    LPTSTR ThisVar;
    LPTSTR ThisValue;
    DWORD VarLen;

    if (!YoriLibGetEnvironmentStrings(&CurrentEnvironment)) {
        return FALSE;
    }

    YoriLibInitEmptyString(&VariableName);
    YoriLibInitEmptyString(&ValueName);

    ThisVar = CurrentEnvironment.StartOfString;
    while (*ThisVar != '\0') {
        VarLen = _tcslen(ThisVar);

        //
        //  We know there's at least one char.  Skip it if it's equals since
        //  that's how drive current directories are recorded.
        //

        ThisValue = _tcschr(&ThisVar[1], '=');
        if (ThisValue != NULL) {
            ThisValue[0] = '\0';
            VariableName.StartOfString = ThisVar;
            VariableName.LengthInChars = (DWORD)(ThisValue - ThisVar);
            VariableName.LengthAllocated = VariableName.LengthInChars + 1;
            YoriCallSetEnvironmentVariable(&VariableName, NULL);
        }

        ThisVar += VarLen;
        ThisVar++;
    }
    YoriLibFreeStringContents(&CurrentEnvironment);

    //
    //  Now restore the saved environment.
    //

    ThisVar = NewEnvironment->StartOfString;
    while (*ThisVar != '\0') {
        VarLen = _tcslen(ThisVar);

        //
        //  We know there's at least one char.  Skip it if it's equals since
        //  that's how drive current directories are recorded.
        //

        ThisValue = _tcschr(&ThisVar[1], '=');
        if (ThisValue != NULL) {
            ThisValue[0] = '\0';
            VariableName.StartOfString = ThisVar;
            VariableName.LengthInChars = (DWORD)(ThisValue - ThisVar);
            VariableName.LengthAllocated = VariableName.LengthInChars + 1;
            ThisValue++;
            ValueName.StartOfString = ThisValue;
            ValueName.LengthInChars = VarLen - VariableName.LengthInChars - 1;
            ValueName.LengthAllocated = ValueName.LengthInChars + 1;
            YoriCallSetEnvironmentVariable(&VariableName, &ValueName);
            ThisValue--;
            ThisValue[0] = '=';
        }

        ThisVar += VarLen;
        ThisVar++;
    }

    return TRUE;
}

/**
 Normally when running a command if a variable does not define any contents,
 the variable name is preserved in the command.  This does not occur with the
 set or if command, where if a variable contains no contents the variable name
 is removed from the result.  Unfortunately this is ambiguous when the variable
 name is found, because we don't know if the variable was not expanded by the
 shell due to no contents or because it was escaped.  So if it has no contents
 it is removed here.  But this still can't distinguish between an escaped
 variable that points to something and an escaped variable which points to
 nothing, both of which presumably should be retained.

 @param Value Pointer to a string containing the value part of the set
        command (ie., what to set a variable to.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibBuiltinRemoveEmptyVariables(
    __inout PYORI_STRING Value
    )
{
    DWORD StartOfVariableName;
    DWORD ReadIndex = 0;
    DWORD WriteIndex = 0;

    while (ReadIndex < Value->LengthInChars) {
        if (YoriLibIsEscapeChar(Value->StartOfString[ReadIndex])) {

            //
            //  If the character is an escape, copy it to the destination
            //  and skip the check for the environment variable character
            //  so we fall into the below case and copy the next character
            //  too
            //

            if (ReadIndex != WriteIndex) {
                Value->StartOfString[WriteIndex] = Value->StartOfString[ReadIndex];
            }
            ReadIndex++;
            WriteIndex++;

        } else if (Value->StartOfString[ReadIndex] == '%') {

            //
            //  We found the first %, scan ahead looking for the next
            //  one
            //

            StartOfVariableName = ReadIndex;
            do {
                if (YoriLibIsEscapeChar(Value->StartOfString[ReadIndex])) {
                    ReadIndex++;
                }
                ReadIndex++;
            } while (ReadIndex < Value->LengthInChars && Value->StartOfString[ReadIndex] != '%');

            //
            //  If we found a well formed variable, check if it refers
            //  to anything.  If it does, that means the shell didn't
            //  expand it because it was escaped, so preserve it here.
            //  If it doesn't, that means it may not refer to anything,
            //  so remove it.
            //

            if (ReadIndex < Value->LengthInChars && Value->StartOfString[ReadIndex] == '%') {
                YORI_STRING YsVariableName;
                YORI_STRING YsVariableValue;

                YsVariableName.StartOfString = &Value->StartOfString[StartOfVariableName + 1];
                YsVariableName.LengthInChars = ReadIndex - StartOfVariableName - 1;

                ReadIndex++;
                YoriLibInitEmptyString(&YsVariableValue);
                if (YoriCallGetEnvironmentVariable(&YsVariableName, &YsVariableValue)) {
                    if (WriteIndex != StartOfVariableName) {
                        memmove(&Value->StartOfString[WriteIndex], &Value->StartOfString[StartOfVariableName], (ReadIndex - StartOfVariableName) * sizeof(TCHAR));
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
            Value->StartOfString[WriteIndex] = Value->StartOfString[ReadIndex];
        }
        ReadIndex++;
        WriteIndex++;
    }
    Value->LengthInChars = WriteIndex;
    return TRUE;
}



// vim:sw=4:ts=4:et:
