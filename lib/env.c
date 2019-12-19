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
__success(return)
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
 Returns TRUE if a set of environment strings are double NULL terminated within
 the bounds of the allocation, and populates LengthInChars to encompass the
 double terminator.  If no double terminator is found within the allocation,
 returns FALSE.

 @param EnvStrings Pointer to the environment strings to validate.

 @return TRUE to indicate the environment strings are valid, FALSE if they are
         not.
 */
__success(return)
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
__success(return)
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

            MultiByteToWideChar(CP_ACP, 0, (LPSTR)AnsiEnvStringBuffer, Index + 2, UnicodeStrings->StartOfString, Index + 2);

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
__success(return)
BOOL
YoriLibAllocateAndGetEnvironmentVariable(
    __in LPCTSTR Name,
    __inout PYORI_STRING Value
    )
{
    DWORD LengthNeeded;

    LengthNeeded = GetEnvironmentVariable(Name, NULL, 0);
    if (LengthNeeded == 0) {
        YoriLibInitEmptyString(Value);
        return TRUE;
    }

    if (LengthNeeded > Value->LengthAllocated) {
        if (!YoriLibReallocateString(Value, LengthNeeded)) {
            return FALSE;
        }
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
__success(return)
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

/**
 Add a a new component to a semicolon delimited string.  This routine assumes
 the caller has allocated enough space in ExistingString to hold the result.
 That is, ExistingString must be large enough to hold itself, plus a seperator,
 plus NewComponent, plus a NULL terminator.

 @param ExistingString Pointer to the existing string.  On successful
        completion, this is modified to contain the new string.

 @param NewComponent Pointer to the component to add to the existing string if
        it is not already there.

 @param InsertAtFront If TRUE, the new component is added before existing
        contents; if FALSE, it is added after existing contents.

 @return TRUE if the buffer was modified, FALSE if it was not.
 */
__success(return)
BOOL
YoriLibAddEnvironmentComponentToString(
    __inout PYORI_STRING ExistingString,
    __in PYORI_STRING NewComponent,
    __in BOOL InsertAtFront
    )
{
    LPTSTR ThisPath;
    TCHAR * TokCtx;

    ASSERT(ExistingString->LengthAllocated >= ExistingString->LengthInChars + 1 + NewComponent->LengthInChars + 1);

    if (ExistingString->LengthAllocated < ExistingString->LengthInChars + 1 + NewComponent->LengthInChars + 1) {
        return FALSE;
    }

    ThisPath = _tcstok_s(ExistingString->StartOfString, _T(";"), &TokCtx);
    while (ThisPath != NULL) {
        if (ThisPath[0] != '\0') {
            if (YoriLibCompareStringWithLiteralInsensitive(NewComponent, ThisPath) == 0) {
                if (TokCtx != NULL) {
                    TokCtx--;
                    ASSERT(*TokCtx == '\0');
                    *TokCtx = ';';
                }
                return FALSE;
            }
        }
        if (TokCtx != NULL) {
            TokCtx--;
            ASSERT(*TokCtx == '\0');
            *TokCtx = ';';
            TokCtx++;
        }
        ThisPath = _tcstok_s(NULL, _T(";"), &TokCtx);
    }

    //
    //  If it currently ends in a semicolon, back up one char
    //  so we don't add another
    //

    if (ExistingString->LengthInChars > 0 &&
        ExistingString->StartOfString[ExistingString->LengthInChars - 1] == ';') {
        ExistingString->LengthInChars--;
    }

    if (InsertAtFront) {

        //
        //  Move the existing contents back to after the new string plus a
        //  seperator, if any existing contents exist.  After inserting the
        //  new string, either add the seperator or terminate, depending
        //  on whether there were contents previously.
        //

        if (ExistingString->LengthInChars > 0) {
            memmove(&ExistingString->StartOfString[NewComponent->LengthInChars + 1],
                    ExistingString->StartOfString,
                    ExistingString->LengthInChars * sizeof(TCHAR));
            ExistingString->StartOfString[NewComponent->LengthInChars + 1 + ExistingString->LengthInChars] = '\0';
        }
        memcpy(ExistingString->StartOfString, NewComponent->StartOfString, NewComponent->LengthInChars * sizeof(TCHAR));
        if (ExistingString->LengthInChars > 0) {
            ExistingString->StartOfString[NewComponent->LengthInChars] = ';';
            ExistingString->LengthInChars += NewComponent->LengthInChars + 1;
        } else {
            ExistingString->StartOfString[NewComponent->LengthInChars] = '\0';
            ExistingString->LengthInChars = NewComponent->LengthInChars;
        }
    } else {

        //
        //  Copy the new path at the end of the previous one.  If it has any
        //  contents, add a seperator.
        //

        if (ExistingString->LengthInChars > 0) {
            ExistingString->StartOfString[ExistingString->LengthInChars] = ';';
            ExistingString->LengthInChars++;
        }
        memcpy(&ExistingString->StartOfString[ExistingString->LengthInChars], NewComponent->StartOfString, NewComponent->LengthInChars * sizeof(TCHAR));
        ExistingString->LengthInChars += NewComponent->LengthInChars;
        ExistingString->StartOfString[ExistingString->LengthInChars] = '\0';
    }
    return TRUE;
}

/**
 Add a component to a semicolon delimited environment variable if it's not
 already there and return the result as a string.

 @param EnvironmentVariable The environment variable to update.

 @param NewComponent The component to add.

 @param InsertAtFront If TRUE, insert the new component at the front of the
        list.  If FALSE, add the component to the end of the list.

 @param Result On successful completion, updated to contain the merged string.

 @return TRUE to indicate success, FALSE on failure.
 */
__success(return)
BOOL
YoriLibAddEnvironmentComponentReturnString(
    __in PYORI_STRING EnvironmentVariable,
    __in PYORI_STRING NewComponent,
    __in BOOL InsertAtFront,
    __out PYORI_STRING Result
    )
{
    DWORD EnvVarLength;

    ASSERT(YoriLibIsStringNullTerminated(EnvironmentVariable));

    //
    //  The contents of the environment variable.  Allocate enough
    //  space to append the specified directory in case we need to.
    //

    EnvVarLength = GetEnvironmentVariable(EnvironmentVariable->StartOfString, NULL, 0);
    if (!YoriLibAllocateString(Result, EnvVarLength + 1 + NewComponent->LengthInChars + 1)) {
        return FALSE;
    }

    Result->StartOfString[0] = '\0';
    Result->LengthInChars = GetEnvironmentVariable(EnvironmentVariable->StartOfString, Result->StartOfString, Result->LengthAllocated);

    YoriLibAddEnvironmentComponentToString(Result, NewComponent, InsertAtFront);
    return TRUE;
}


/**
 Add a component to a semicolon delimited environment variable if it's not
 already there.

 @param EnvironmentVariable The environment variable to update.

 @param NewComponent The component to add.

 @param InsertAtFront If TRUE, insert the new component at the front of the
        list.  If FALSE, add the component to the end of the list.

 @return TRUE to indicate success, FALSE on failure.
 */
__success(return)
BOOL
YoriLibAddEnvironmentComponent(
    __in LPTSTR EnvironmentVariable,
    __in PYORI_STRING NewComponent,
    __in BOOL InsertAtFront
    )
{
    YORI_STRING EnvData;
    YORI_STRING YsEnvironmentVariable;

    YoriLibConstantString(&YsEnvironmentVariable, EnvironmentVariable);

    if (!YoriLibAddEnvironmentComponentReturnString(&YsEnvironmentVariable, NewComponent, InsertAtFront, &EnvData)) {
        return FALSE;
    }

    if (!SetEnvironmentVariable(EnvironmentVariable, EnvData.StartOfString)) {
        YoriLibFreeStringContents(&EnvData);
        return FALSE;
    }
    
    YoriLibFreeStringContents(&EnvData);
    return TRUE;
}

/**
 Remove a component from a semicolon delimited string buffer if it's already
 there and return the combined string.

 @param String The string buffer that contains semicolon delimited elements.

 @param ComponentToRemove The component to remove.

 @param Result On successful completion, updated to contain the adjusted
        string.

 @return TRUE to indicate success, FALSE on failure.
 */
__success(return)
BOOL
YoriLibRemoveEnvironmentComponentFromString(
    __in PYORI_STRING String,
    __in PYORI_STRING ComponentToRemove,
    __out PYORI_STRING Result
    )
{
    LPTSTR NewPathData;
    LPTSTR ThisPath;
    TCHAR * TokCtx;
    DWORD NewOffset;
    DWORD CharsPopulated;

    ASSERT(YoriLibIsStringNullTerminated(String));

    NewPathData = YoriLibReferencedMalloc((String->LengthInChars + 1) * sizeof(TCHAR));
    if (NewPathData == NULL) {
        return FALSE;
    }

    NewOffset = 0;

    ThisPath = _tcstok_s(String->StartOfString, _T(";"), &TokCtx);
    while (ThisPath != NULL) {
        if (ThisPath[0] != '\0') {
            if (YoriLibCompareStringWithLiteralInsensitive(ComponentToRemove, ThisPath) != 0) {
                if (NewOffset != 0) {
                    CharsPopulated = YoriLibSPrintf(&NewPathData[NewOffset], _T(";%s"), ThisPath);
                } else {
                    CharsPopulated = YoriLibSPrintf(&NewPathData[NewOffset], _T("%s"), ThisPath);
                }

                if (CharsPopulated < 0) {
                    YoriLibDereference(NewPathData);
                    return FALSE;
                }
                NewOffset += CharsPopulated;
            }
        }
        if (TokCtx != NULL) {
            TokCtx--;
            ASSERT(*TokCtx == '\0');
            *TokCtx = ';';
            TokCtx++;
        }
        ThisPath = _tcstok_s(NULL, _T(";"), &TokCtx);
    }

    //
    //  Normally this would be NULL terminated by Printf, but if that wasn't
    //  invoked at least once nothing terminated the destination buffer, so
    //  do it explicitly here.
    //

    if (NewOffset == 0) {
        NewPathData[NewOffset] = '\0';
    }

    YoriLibInitEmptyString(Result);
    Result->MemoryToFree = NewPathData;
    Result->StartOfString = NewPathData;
    Result->LengthInChars = NewOffset;
    if (NewOffset > 0) {
        Result->LengthAllocated = NewOffset + 1;
    } else {
        Result->LengthAllocated = 0;
    }

    return TRUE;
}

/**
 Remove a component from a semicolon delimited environment variable if it's
 already there and return the combined string.

 @param EnvironmentVariable The environment variable to update.

 @param ComponentToRemove The component to remove.

 @param Result On successful completion, updated to contain the adjusted
        string.

 @return TRUE to indicate success, FALSE on failure.
 */
__success(return)
BOOL
YoriLibRemoveEnvironmentComponentReturnString(
    __in PYORI_STRING EnvironmentVariable,
    __in PYORI_STRING ComponentToRemove,
    __out PYORI_STRING Result
    )
{
    YORI_STRING PathData;
    DWORD PathLength;
    BOOL Success;

    ASSERT(YoriLibIsStringNullTerminated(EnvironmentVariable));

    //
    //  The contents of the environment variable.  Allocate enough
    //  space to append the specified directory in case we need to.
    //

    PathLength = GetEnvironmentVariable(EnvironmentVariable->StartOfString, NULL, 0);
    if (!YoriLibAllocateString(&PathData, PathLength + 1)) {
        return FALSE;
    }

    PathData.StartOfString[0] = '\0';
    PathData.LengthInChars = GetEnvironmentVariable(EnvironmentVariable->StartOfString, PathData.StartOfString, PathData.LengthAllocated);

    Success = YoriLibRemoveEnvironmentComponentFromString(&PathData, ComponentToRemove, Result);

    YoriLibFreeStringContents(&PathData);

    return Success;
}

/**
 Remove a component from a semicolon delimited environment variable if it's
 already there.

 @param EnvironmentVariable The environment variable to update.

 @param ComponentToRemove The component to remove.

 @return TRUE to indicate success, FALSE on failure.
 */
__success(return)
BOOL
YoriLibRemoveEnvironmentComponent(
    __in LPTSTR EnvironmentVariable,
    __in PYORI_STRING ComponentToRemove
    )
{
    YORI_STRING CombinedString;
    LPTSTR ValueToSet;
    YORI_STRING YsEnvironmentVariable;

    YoriLibConstantString(&YsEnvironmentVariable, EnvironmentVariable);

    if (!YoriLibRemoveEnvironmentComponentReturnString(&YsEnvironmentVariable, ComponentToRemove, &CombinedString)) {
        return FALSE;
    }

    //
    //  If no data was copied forward, delete the variable.  Note NewPathData
    //  contains uninitialized data at this point when that occurs.
    //

    if (CombinedString.LengthAllocated > 0) {
        ValueToSet = CombinedString.StartOfString;
    } else {
        ValueToSet = NULL;
    }

    if (!SetEnvironmentVariable(EnvironmentVariable, ValueToSet)) {
        YoriLibFreeStringContents(&CombinedString);
        return FALSE;
    }
    
    YoriLibFreeStringContents(&CombinedString);
    return TRUE;
}


// vim:sw=4:ts=4:et:
