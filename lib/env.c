/**
 * @file lib/env.c
 *
 * Yori lib environment variable control
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

#include <yoripch.h>
#include <yorilib.h>

/**
 An implementation of GetEnvironmentStrings that can do appropriate dances
 to work with versions of Windows supporting free, and versions that don't
 support free, as well as those with a W suffix and those without.

 @param EnvStrings On successful completion, populated with the array of
        environment strings.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGetEnvironmentStrings(
    __out PYORI_STRING EnvStrings
    )
{
    LPTSTR OsEnvStrings;
    LPSTR OsEnvStringsA;
    DWORD CharCount;

    //
    //  Use GetEnvironmentStringsW where it exists.  Where it doesn't
    //  exist (NT 3.1) we need to upconvert to Unicode.
    //

    if (DllKernel32.pGetEnvironmentStringsW != NULL) {
        OsEnvStrings = DllKernel32.pGetEnvironmentStringsW();
        for (CharCount = 0; OsEnvStrings[CharCount] != '\0' || OsEnvStrings[CharCount + 1] != '\0'; CharCount++);

        CharCount += 2;
        if (!YoriLibAllocateString(EnvStrings, CharCount)) {
            if (DllKernel32.pFreeEnvironmentStringsW) {
                DllKernel32.pFreeEnvironmentStringsW(OsEnvStrings);
            }
            return FALSE;
        }

        memcpy(EnvStrings->StartOfString, OsEnvStrings, CharCount * sizeof(TCHAR));

        if (DllKernel32.pFreeEnvironmentStringsW) {
            DllKernel32.pFreeEnvironmentStringsW(OsEnvStrings);
        }

        return TRUE;
    }

    if (DllKernel32.pGetEnvironmentStrings == NULL) {
        return FALSE;
    }

    OsEnvStringsA = DllKernel32.pGetEnvironmentStrings();

    for (CharCount = 0; OsEnvStringsA[CharCount] != '\0' || OsEnvStringsA[CharCount + 1] != '\0'; CharCount++);

    CharCount += 2;
    if (!YoriLibAllocateString(EnvStrings, CharCount)) {
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, OsEnvStringsA, CharCount, EnvStrings->StartOfString, CharCount);

    return TRUE;
}

/**
 Apply an environment block into the running process.  Variables not explicitly
 included in this block are discarded.

 @param NewEnv Pointer to the new environment block to apply.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibSetEnvironmentStrings(
    __in PYORI_STRING NewEnv
    )
{
    YORI_STRING CurrentEnvironment;
    LPTSTR ThisVar;
    LPTSTR ThisValue;
    DWORD VarLen;

    //
    //  Query the current environment and delete everything in it.
    //

    if (!YoriLibGetEnvironmentStrings(&CurrentEnvironment)) {
        return FALSE;
    }
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
            SetEnvironmentVariable(ThisVar, NULL);
        }

        ThisVar += VarLen;
        ThisVar++;
    }
    YoriLibFreeStringContents(&CurrentEnvironment);

    //
    //  Now load the new environment.
    //

    ThisVar = NewEnv->StartOfString;
    while (*ThisVar != '\0') {
        VarLen = _tcslen(ThisVar);

        //
        //  We know there's at least one char.  Skip it if it's equals since
        //  that's how drive current directories are recorded.
        //

        ThisValue = _tcschr(&ThisVar[1], '=');
        if (ThisValue != NULL) {
            ThisValue[0] = '\0';
            ThisValue++;
            SetEnvironmentVariable(ThisVar, ThisValue);
        }

        ThisVar += VarLen;
        ThisVar++;
    }

    return TRUE;
}

/**
 Returns TRUE if a set of environment strings are double NULL terminated within
 the bounds of the allocation, and populates LengthInChars to encompass the
 double terminator.  If no double terminator is found within the allocation,
 returns FALSE.

 @param EnvStrings Pointer to the environment strings to validate.

 @return TRUE to indicate the environment strings are valid, FALSE if they are
         not.
 */
BOOL
YoriLibAreEnvironmentStringsValid(
    __inout PYORI_STRING EnvStrings
    )
{
    DWORD Index;

    if (EnvStrings->LengthAllocated < 2) {
        return FALSE;
    }

    for (Index = 0; Index < EnvStrings->LengthAllocated - 1; Index++) {
        if (EnvStrings->StartOfString[Index] == '\0' &&
            EnvStrings->StartOfString[Index + 1] == '\0') {

            EnvStrings->LengthInChars = Index + 1;

            return TRUE;
        }
    }

    return FALSE;
}

/**
 Checks if ANSI environment strings are double NULL terminated within
 the bounds of the allocation.  If so, allocates a new Yori string to describe
 the Unicode form of the environment block and populates it with the correct
 length of the buffer.

 @param AnsiEnvStringBuffer Pointer to the environment strings to validate.

 @param BufferLength Number of bytes in the environment string allocation.

 @param UnicodeStrings On successful completion, updated to point to a newly
        allocated set of Unicode strings for the block.

 @return TRUE to indicate the environment strings are valid and could be
         converted to Unicode, FALSE if invalid or not convertible.
 */
BOOL
YoriLibAreAnsiEnvironmentStringsValid(
    __in PUCHAR AnsiEnvStringBuffer,
    __in DWORD BufferLength,
    __out PYORI_STRING UnicodeStrings
    )
{
    DWORD Index;

    YoriLibInitEmptyString(UnicodeStrings);

    if (BufferLength < 2) {
        return FALSE;
    }

    for (Index = 0; Index < BufferLength - 1; Index++) {
        if (AnsiEnvStringBuffer[Index] == '\0' &&
            AnsiEnvStringBuffer[Index + 1] == '\0') {

            if (!YoriLibAllocateString(UnicodeStrings, Index + 2)) {
                return FALSE;
            }

            MultiByteToWideChar(CP_ACP, 0, AnsiEnvStringBuffer, Index + 2, UnicodeStrings->StartOfString, Index + 2);

            UnicodeStrings->LengthInChars = Index + 2;

            return TRUE;
        }
    }

    return FALSE;
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
YoriLibAllocateAndGetEnvironmentVariable(
    __in LPCTSTR Name,
    __out PYORI_STRING Value
    )
{
    DWORD LengthNeeded;

    LengthNeeded = GetEnvironmentVariable(Name, NULL, 0);
    if (LengthNeeded == 0) {
        YoriLibInitEmptyString(Value);
        return TRUE;
    }

    if (!YoriLibAllocateString(Value, LengthNeeded)) {
        return FALSE;
    }

    Value->LengthInChars = GetEnvironmentVariable(Name, Value->StartOfString, Value->LengthAllocated);
    if (Value->LengthInChars == 0 ||
        Value->LengthInChars >= Value->LengthAllocated) {

        YoriLibFreeStringContents(Value);
        return FALSE;
    }

    return TRUE;
}

/**
 Capture the value from an environment variable, convert it to a number and
 return the result.

 @param Name Pointer to the name of the variable to obtain.

 @param Value On successful completion, populated with the contents of the
        variable in numeric form.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGetEnvironmentVariableAsNumber(
    __in LPCTSTR Name,
    __out PLONGLONG Value
    )
{
    DWORD LengthNeeded;
    YORI_STRING Temp;
    TCHAR Buffer[32];
    DWORD CharsConsumed;

    LengthNeeded = GetEnvironmentVariable(Name, NULL, 0);

    if (LengthNeeded == 0) {
        return FALSE;
    }

    if (LengthNeeded < sizeof(Buffer)/sizeof(Buffer[0])) {
        YoriLibInitEmptyString(&Temp);
        Temp.StartOfString = Buffer;
        Temp.LengthAllocated = sizeof(Buffer)/sizeof(Buffer[0]);
    } else if (!YoriLibAllocateString(&Temp, LengthNeeded)) {
        return FALSE;
    }

    Temp.LengthInChars = GetEnvironmentVariable(Name, Temp.StartOfString, Temp.LengthAllocated);
    if (Temp.LengthInChars == 0 ||
        Temp.LengthInChars >= Temp.LengthAllocated) {

        YoriLibFreeStringContents(&Temp);
        return FALSE;
    }

    if (!YoriLibStringToNumber(&Temp, FALSE, Value, &CharsConsumed)) {
        return FALSE;
    }

    YoriLibFreeStringContents(&Temp);

    if (CharsConsumed == 0) {
        return FALSE;
    }

    return TRUE;
}




// vim:sw=4:ts=4:et:
