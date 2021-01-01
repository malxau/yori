/**
 * @file make/var.c
 *
 * Yori shell make variable support
 *
 * Copyright (c) 2020 Malcolm J. Smith
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
#include <yorish.h>
#include "make.h"

/**
 Deallocate a single variable.

 @param ScopeContext Pointer to the scope context.

 @param Variable Pointer to the variable to deallocate.
 */
VOID
MakeDeleteVariable(
    __inout PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PMAKE_VARIABLE Variable
    )
{
    UNREFERENCED_PARAMETER(ScopeContext);

    YoriLibRemoveListItem(&Variable->ListEntry);
    YoriLibHashRemoveByEntry(&Variable->HashEntry);
    YoriLibFreeStringContents(&Variable->Value);
    YoriLibDereference(Variable);
}

/**
 Deallocate all variables within the specified context.

 @param ScopeContext Pointer to the scope context.
 */
VOID
MakeDeleteAllVariables(
    __inout PMAKE_SCOPE_CONTEXT ScopeContext
    )
{ 
    PYORI_LIST_ENTRY ListEntry = NULL;
    PMAKE_VARIABLE Variable;

    ListEntry = YoriLibGetNextListEntry(&ScopeContext->VariableList, NULL);
    while (ListEntry != NULL) {
        Variable = CONTAINING_RECORD(ListEntry, MAKE_VARIABLE, ListEntry);
#if MAKE_DEBUG_VARIABLES
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Deleting variable: %y=%y (undefined %i)\n"), &Variable->HashEntry.Key, &Variable->Value, Variable->Undefined);
#endif
        MakeDeleteVariable(ScopeContext, Variable);
        ListEntry = YoriLibGetNextListEntry(&ScopeContext->VariableList, NULL);
    }
}

/**
 Lookup a variable by name.

 @param ScopeContext Pointer to the scope context.

 @param Variable Pointer to the variable name.

 @param FoundScopeContext On successful completion, populated with the scope
        context that contains the variable.

 @return A pointer to the variable structure if it is found, or NULL if it
         is not found.
 */
__success(return != NULL)
PMAKE_VARIABLE
MakeLookupVariable(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PCYORI_STRING Variable,
    __out_opt PMAKE_SCOPE_CONTEXT *FoundScopeContext
    )
{
    PYORI_HASH_ENTRY FoundVariableEntry;
    PMAKE_SCOPE_CONTEXT SearchScopeContext;
    PMAKE_VARIABLE FoundVariable;

    SearchScopeContext = ScopeContext;
    FoundVariable = NULL;

    do {
        FoundVariableEntry = YoriLibHashLookupByKey(SearchScopeContext->Variables, Variable);
        if (FoundVariableEntry != NULL) {
            FoundVariable = FoundVariableEntry->Context;
            break;
        }

        SearchScopeContext = SearchScopeContext->ParentScope;
        FoundVariable = NULL;
    } while (SearchScopeContext != NULL);

    if (FoundVariable != NULL) {
        if (FoundScopeContext != NULL) {
            *FoundScopeContext = SearchScopeContext;
        }
        return FoundVariable;
    }

    return NULL;
}

/**
 Return TRUE if this variable name is for a special target specific variable,
 or FALSE if it is for a user defined variable.

 @param VariableName Pointer to the variable name.

 @return TRUE if this variable name is for a special target specific variable,
         of FALSE if it is for a user defined variable.
 */
BOOLEAN
MakeIsVariableTargetSpecific(
    __in PCYORI_STRING VariableName
    )
{
    if (YoriLibCompareStringWithLiteralCount(VariableName, _T("@"), 1) == 0 ||
        YoriLibCompareStringWithLiteralCount(VariableName, _T("$@"), 2) == 0 ||
        YoriLibCompareStringWithLiteralCount(VariableName, _T("*"), 1) == 0 ||
        YoriLibCompareStringWithLiteralCount(VariableName, _T("**"), 2) == 0 ||
        YoriLibCompareStringWithLiteralCount(VariableName, _T("?"), 1) == 0 ||
        YoriLibCompareStringWithLiteralCount(VariableName, _T("<"), 1) == 0) {

        return TRUE;
    }

    return FALSE;
}

/**
 Given a variable name, obtain the data for the variable.  For user variables,
 this is a hashtable lookup.  This function also handles special target
 specific variables generated from the target state.

 @param ScopeContext Pointer to the scope context.

 @param Target Optionally points to the target.  If this is NULL, any target
        specific variables are retained as variables.

 @param VariableName Pointer to the variable name to expand.

 @param VariableData On successful completion, populated with the contents of
        the variable.  Typically this is just pointing to data stored in the
        variable hashtable.  For target specific variables, it may be
        dynamically generated.  The caller should free this with
        YoriLibFreeStringContents, although most of the time this does
        nothing.

 @return TRUE if the variable was successfully expanded, FALSE if it was not.
 */
__success(return)
BOOLEAN
MakeSubstituteNamedVariable(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in_opt PMAKE_TARGET Target,
    __in PCYORI_STRING VariableName,
    __inout PYORI_STRING VariableData
    )
{
    PMAKE_VARIABLE FoundVariable;
    YORI_STRING NameToFind;
    YORI_STRING SearchText;
    YORI_STRING ReplaceText;
    YORI_STRING RemainingText;
    PYORI_STRING FoundMatch;
    DWORD FoundAt;
    DWORD LengthNeeded;
    LPTSTR Ptr;

    if (MakeIsVariableTargetSpecific(VariableName)) {
        if (Target == NULL) {
            YoriLibYPrintf(VariableData, _T("$(%y)"), VariableName);
            return TRUE;
        } else {
            return MakeExpandTargetVariable(ScopeContext->MakeContext, Target, VariableName, VariableData);
        }
    }

    YoriLibInitEmptyString(&NameToFind);
    YoriLibInitEmptyString(&SearchText);
    YoriLibInitEmptyString(&ReplaceText);

    NameToFind.StartOfString = VariableName->StartOfString;
    NameToFind.LengthInChars = VariableName->LengthInChars;

    //
    //  Check if the variable name contains a search and replace expression, as
    //  in $(VARNAME:OLDTEXT=NEWTEXT).  Keep pointers to the old and new text,
    //  and trim the name of the variable to search for.
    //

    Ptr = YoriLibFindLeftMostCharacter(&NameToFind, ':');
    if (Ptr != NULL) {
        SearchText.StartOfString = Ptr + 1;
        SearchText.LengthInChars = NameToFind.LengthInChars - (DWORD)(Ptr - NameToFind.StartOfString) - 1;

        Ptr = YoriLibFindLeftMostCharacter(&SearchText, '=');
        if (Ptr != NULL) {
            ReplaceText.StartOfString = Ptr + 1;
            ReplaceText.LengthInChars = SearchText.LengthInChars - (DWORD)(Ptr - SearchText.StartOfString) - 1;
            SearchText.LengthInChars = SearchText.LengthInChars - ReplaceText.LengthInChars - 1;
            NameToFind.LengthInChars = NameToFind.LengthInChars - SearchText.LengthInChars - ReplaceText.LengthInChars - 2;
        } else {
            SearchText.LengthInChars = 0;
        }
    }

    FoundVariable = MakeLookupVariable(ScopeContext, &NameToFind, NULL);
    if (FoundVariable == NULL) {
        return FALSE;
    }

    //
    //  If there's no search and replace text, we're done.
    //

    if (SearchText.LengthInChars == 0) {
        VariableData->StartOfString = FoundVariable->Value.StartOfString;
        VariableData->LengthInChars = FoundVariable->Value.LengthInChars;
        return TRUE;
    }

    LengthNeeded = 0;

    //
    //  Count the length of the text after performing the replacement.
    //

    YoriLibInitEmptyString(&RemainingText);
    RemainingText.StartOfString = FoundVariable->Value.StartOfString;
    RemainingText.LengthInChars = FoundVariable->Value.LengthInChars;

    FoundMatch = YoriLibFindFirstMatchingSubstring(&RemainingText, 1, &SearchText, &FoundAt);
    while (FoundMatch) {
        LengthNeeded = LengthNeeded + FoundAt + ReplaceText.LengthInChars;
        RemainingText.StartOfString += FoundAt + SearchText.LengthInChars;
        RemainingText.LengthInChars -= FoundAt + SearchText.LengthInChars;
        FoundMatch = YoriLibFindFirstMatchingSubstring(&RemainingText, 1, &SearchText, &FoundAt);
    }

    LengthNeeded = LengthNeeded + RemainingText.LengthInChars + 1;

    //
    //  Allocate space for the text after replacement.
    //

    if (!YoriLibAllocateString(VariableData, LengthNeeded)) {
        return FALSE;
    }
    ScopeContext->MakeContext->AllocVariableData++;

    //
    //  Go through again, performing the replacement.
    //

    LengthNeeded = 0;
    RemainingText.StartOfString = FoundVariable->Value.StartOfString;
    RemainingText.LengthInChars = FoundVariable->Value.LengthInChars;

    FoundMatch = YoriLibFindFirstMatchingSubstring(&RemainingText, 1, &SearchText, &FoundAt);
    while (FoundMatch) {
        if (FoundAt > 0) {
            memcpy(&VariableData->StartOfString[LengthNeeded], RemainingText.StartOfString, FoundAt * sizeof(TCHAR));
            LengthNeeded = LengthNeeded + FoundAt;
        }

        memcpy(&VariableData->StartOfString[LengthNeeded], ReplaceText.StartOfString, ReplaceText.LengthInChars * sizeof(TCHAR));
        LengthNeeded = LengthNeeded + ReplaceText.LengthInChars;
        RemainingText.StartOfString += FoundAt + SearchText.LengthInChars;
        RemainingText.LengthInChars -= FoundAt + SearchText.LengthInChars;
        FoundMatch = YoriLibFindFirstMatchingSubstring(&RemainingText, 1, &SearchText, &FoundAt);
    }

    if (RemainingText.LengthInChars > 0) {
        memcpy(&VariableData->StartOfString[LengthNeeded], RemainingText.StartOfString, RemainingText.LengthInChars * sizeof(TCHAR));
        LengthNeeded = LengthNeeded + RemainingText.LengthInChars;
    }

    VariableData->LengthInChars = LengthNeeded;
    return TRUE;
}

/**
 Expand all of the variables in a given string.  This will always copy the
 string.  The reason for the copy is to support reusing the allocation that
 contains the expanded string rather than allocating and freeing for every
 line that requires expansion.  Note that this routine expands $(FOO) type
 variables, but not the more special $@ type variables, whose meaning is
 context dependent.

 @param ScopeContext Pointer to the scope context specifying all of the
        active variables.

 @param Target Optionally points to a target that is associated with the
        variables being expanded.  When this is NULL, only user defined
        variables are expanded.  When this is non-NULL, special variables
        including $@, $?, $< et al can be expanded based on the state of
        the target.

 @param ExpandedLine On input, points to an initialized Yori string.  On
        output, contains the string with variables expanded.  Note this
        string can be reallocated within this routine.

 @param Line Pointer to the line in the makefile to expand variables for.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeExpandVariables(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in_opt PMAKE_TARGET Target,
    __inout PYORI_STRING ExpandedLine,
    __in PYORI_STRING Line
    )
{
    YORI_STRING VariableName;
    YORI_STRING VariableContents;
    BOOLEAN BraceDelimited;

    DWORD StartVariableNameIndex;
    DWORD ReadIndex;
    DWORD WriteIndex;
    DWORD LengthNeeded;

    YoriLibInitEmptyString(&VariableName);
    YoriLibInitEmptyString(&VariableContents);

    LengthNeeded = 0;
    for (ReadIndex = 0; ReadIndex < Line->LengthInChars; ReadIndex++) {
        if (Line->StartOfString[ReadIndex] == '$' &&
            ReadIndex + 1 < Line->LengthInChars) {

            BraceDelimited = FALSE;
            StartVariableNameIndex = ReadIndex + 1;
            if (Line->StartOfString[StartVariableNameIndex] == '(' &&
                StartVariableNameIndex + 1 < Line->LengthInChars) {
                StartVariableNameIndex = StartVariableNameIndex + 1;
                BraceDelimited = TRUE;
            }

            VariableName.StartOfString = &Line->StartOfString[StartVariableNameIndex];

            if (BraceDelimited) {
                VariableName.LengthInChars = 0;
                for (ReadIndex = StartVariableNameIndex ;ReadIndex < Line->LengthInChars; ReadIndex++) {
                    if (Line->StartOfString[ReadIndex] == ')') {
                        VariableName.LengthInChars = ReadIndex - StartVariableNameIndex;
                        break;
                    }
                }
            } else {

                // 
                //  MSFIX Check what $$ means
                //

                if (ReadIndex + 2 < Line->LengthInChars &&
                    (VariableName.StartOfString[0] == '$' && VariableName.StartOfString[1] == '@') ||
                    (VariableName.StartOfString[0] == '*' && VariableName.StartOfString[1] == '*')) {
                    VariableName.LengthInChars = 2;
                } else {
                    VariableName.LengthInChars = 1;
                }
                ReadIndex = ReadIndex + VariableName.LengthInChars;
            }

            if (VariableName.LengthInChars > 0) {
                if (MakeSubstituteNamedVariable(ScopeContext, Target, &VariableName, &VariableContents)) {
                    LengthNeeded = LengthNeeded + VariableContents.LengthInChars;
                    YoriLibFreeStringContents(&VariableContents);
                }
            }
        } else if (ReadIndex + 1 < Line->LengthInChars &&
                   Line->StartOfString[ReadIndex] == '%' &&
                   Line->StartOfString[ReadIndex + 1] == '%') {
            ReadIndex++;
            LengthNeeded++;
        } else {
            LengthNeeded++;
        }
    }

    if (ExpandedLine->LengthAllocated < LengthNeeded + 1) {
        LengthNeeded = LengthNeeded + 1024;
        YoriLibFreeStringContents(ExpandedLine);
        if (!YoriLibAllocateString(ExpandedLine, LengthNeeded)) {
            return FALSE;
        }
        ScopeContext->MakeContext->AllocExpandedLine++;
    }

    WriteIndex = 0;
    for (ReadIndex = 0; ReadIndex < Line->LengthInChars; ReadIndex++) {
        if (Line->StartOfString[ReadIndex] == '$' &&
            ReadIndex + 1 < Line->LengthInChars) {

            BraceDelimited = FALSE;
            StartVariableNameIndex = ReadIndex + 1;
            if (Line->StartOfString[StartVariableNameIndex] == '(' &&
                StartVariableNameIndex + 1 < Line->LengthInChars) {
                StartVariableNameIndex = StartVariableNameIndex + 1;
                BraceDelimited = TRUE;
            }

            VariableName.StartOfString = &Line->StartOfString[StartVariableNameIndex];

            if (BraceDelimited) {
                VariableName.LengthInChars = 0;
                for (ReadIndex = StartVariableNameIndex ;ReadIndex < Line->LengthInChars; ReadIndex++) {
                    if (Line->StartOfString[ReadIndex] == ')') {
                        VariableName.LengthInChars = ReadIndex - StartVariableNameIndex;
                        break;
                    }
                }
            } else {

                // 
                //  MSFIX Check what $$ means
                //

                if (ReadIndex + 2 < Line->LengthInChars &&
                    (VariableName.StartOfString[0] == '$' && VariableName.StartOfString[1] == '@') ||
                    (VariableName.StartOfString[0] == '*' && VariableName.StartOfString[1] == '*')) {
                    VariableName.LengthInChars = 2;
                } else {
                    VariableName.LengthInChars = 1;
                }
                ReadIndex = ReadIndex + VariableName.LengthInChars;
            }

            if (VariableName.LengthInChars > 0) {
                if (MakeSubstituteNamedVariable(ScopeContext, Target, &VariableName, &VariableContents)) {
                    memcpy(&ExpandedLine->StartOfString[WriteIndex], VariableContents.StartOfString, VariableContents.LengthInChars * sizeof(TCHAR));
                    WriteIndex = WriteIndex + VariableContents.LengthInChars;
                    YoriLibFreeStringContents(&VariableContents);
                }
            }
        } else if (ReadIndex + 1 < Line->LengthInChars &&
                   Line->StartOfString[ReadIndex] == '%' &&
                   Line->StartOfString[ReadIndex + 1] == '%') {
            ExpandedLine->StartOfString[WriteIndex] = Line->StartOfString[ReadIndex];
            WriteIndex++;
            ReadIndex++;
        } else {
            ExpandedLine->StartOfString[WriteIndex] = Line->StartOfString[ReadIndex];
            WriteIndex++;
        }
    }

    ExpandedLine->StartOfString[WriteIndex] = '\0';
    ExpandedLine->LengthInChars = WriteIndex;
    return TRUE;
}

/**
 Set a variable to a value.

 @param ScopeContext Pointer to the scope context.

 @param Variable Pointer to the variable name.

 @param Value Optionally points to a value.  This can be NULL if the
        variable is being defined to an empty value or being undefined.

 @param Defined TRUE if the variable is being defined, FALSE if the variable
        is being undefined.

 @param Precedence The precedence level of the new variable definition.  If
        the variable is currently defined from a higher precedence, this
        operation is ignored.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeSetVariable(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PCYORI_STRING Variable,
    __in_opt PCYORI_STRING Value,
    __in BOOLEAN Defined,
    __in MAKE_VARIABLE_PRECEDENCE Precedence
    )
{
    PYORI_HASH_ENTRY FoundVariableEntry;
    PMAKE_VARIABLE FoundVariable;

    FoundVariableEntry = YoriLibHashLookupByKey(ScopeContext->Variables, Variable);
    if (FoundVariableEntry != NULL) {
        FoundVariable = FoundVariableEntry->Context;

        if (FoundVariable->Precedence <= Precedence) {

            if (Value != NULL && FoundVariable->Value.LengthAllocated < Value->LengthInChars) {
                YoriLibFreeStringContents(&FoundVariable->Value);
                if (!YoriLibAllocateString(&FoundVariable->Value, Value->LengthInChars)) {
                    return FALSE;
                }
                ScopeContext->MakeContext->AllocVariable++;
            }

            if (Value != NULL) {
                memcpy(FoundVariable->Value.StartOfString, Value->StartOfString, Value->LengthInChars * sizeof(TCHAR));
                FoundVariable->Value.LengthInChars = Value->LengthInChars;
            } else {
                FoundVariable->Value.LengthInChars = 0;
            }

            if (Defined) {
                FoundVariable->Undefined = FALSE;
            } else {
                FoundVariable->Undefined = TRUE;
            }

            FoundVariable->Precedence = Precedence;
        }

    } else {
        YORI_STRING VariableNameCopy;
        DWORD LengthNeeded;

        LengthNeeded = Variable->LengthInChars;
        if (Value != NULL) {
            LengthNeeded = LengthNeeded + Value->LengthInChars;
        }

        FoundVariable = YoriLibReferencedMalloc(sizeof(MAKE_VARIABLE) + LengthNeeded * sizeof(TCHAR));
        if (FoundVariable == NULL) {
            return FALSE;
        }
        ScopeContext->MakeContext->AllocVariable++;

        //
        //  The hash package will clone (reference) the string rather than
        //  copy it.  It has to be copied somewhere, so we copy it here, into
        //  the same allocation used to hold the value.  Note the variable
        //  name is effectively immutable, so we don't need referencing or
        //  to support reallocation.
        //

        YoriLibInitEmptyString(&VariableNameCopy);
        VariableNameCopy.StartOfString = (LPTSTR)(FoundVariable + 1);
        memcpy(VariableNameCopy.StartOfString, Variable->StartOfString, Variable->LengthInChars * sizeof(TCHAR));
        VariableNameCopy.LengthInChars = Variable->LengthInChars;

        YoriLibInitEmptyString(&FoundVariable->Value);
        if (Value != NULL) {
            YoriLibReference(FoundVariable);
            FoundVariable->Value.MemoryToFree = FoundVariable;
            FoundVariable->Value.StartOfString = VariableNameCopy.StartOfString + VariableNameCopy.LengthInChars;
            memcpy(FoundVariable->Value.StartOfString, Value->StartOfString, Value->LengthInChars * sizeof(TCHAR));
            FoundVariable->Value.LengthAllocated = Value->LengthInChars;
            FoundVariable->Value.LengthInChars = Value->LengthInChars;
        }

        if (Defined) {
            FoundVariable->Undefined = FALSE;
        } else {
            FoundVariable->Undefined = TRUE;
        }

        FoundVariable->Precedence = Precedence;

        YoriLibHashInsertByKey(ScopeContext->Variables, &VariableNameCopy, FoundVariable, &FoundVariable->HashEntry);
        YoriLibInsertList(&ScopeContext->VariableList, &FoundVariable->ListEntry);
    }

    return TRUE;
}

/**
 Modify the contents of a variable within a given makefile execution scope.

 @param ScopeContext Pointer to the scope context specifying the active
        variables.

 @param Line Pointer to the line, including the variable name and value for
        the variable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeExecuteSetVariable(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Line
    )
{
    YORI_STRING Variable;
    YORI_STRING Value;
    DWORD Index;

    YoriLibInitEmptyString(&Variable);
    YoriLibInitEmptyString(&Value);
    Variable.StartOfString = Line->StartOfString;

    for (Index = 0; Index < Line->LengthInChars; Index++) {
        if (Line->StartOfString[Index] == '=') {
            Variable.LengthInChars = Index;
            Value.StartOfString = &Line->StartOfString[Index + 1];
            Value.LengthInChars = Line->LengthInChars - Index - 1;
            break;
        }
    }

    MakeTrimWhitespace(&Variable);
    MakeTrimWhitespace(&Value);

    return MakeSetVariable(ScopeContext, &Variable, &Value, TRUE, MakeVariablePrecedenceMakefile);
}


/**
 Return TRUE if a variable is known and defined, or FALSE if the variable
 is either unknown or explicitly undefined.

 @param ScopeContext Pointer to the scope context.

 @param Variable Pointer to the variable name to check.
 
 @return TRUE if a variable is known and defined, or FALSE if the variable is
         either unknown or explicitly undefined.
 */
BOOLEAN
MakeIsVariableDefined(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Variable
    )
{
    PMAKE_VARIABLE FoundVariable;

    FoundVariable = MakeLookupVariable(ScopeContext, Variable, NULL);

    if (FoundVariable != NULL) {
        if (FoundVariable->Undefined) {
            return FALSE;
        }
        return TRUE;
    }

    return FALSE;
}

// vim:sw=4:ts=4:et:
