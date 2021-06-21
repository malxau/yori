/**
 * @file make/scope.c
 *
 * Yori shell scope support routines
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
 Allocate a new scope context.

 @param MakeContext Pointer to the process global context.

 @param Directory Pointer to a fully qualified directory string to identify
        the directory.  This allocation is cloned into the scope (ie., the
        contents are immutable after this point.)

 @return Pointer to the newly allocated scope context, or NULL on failure.
 */
PMAKE_SCOPE_CONTEXT
MakeAllocateNewScope(
    __in PMAKE_CONTEXT MakeContext,
    __in PYORI_STRING Directory
    )
{
    PMAKE_SCOPE_CONTEXT ScopeContext;
    YORI_STRING Default;

    ScopeContext = YoriLibReferencedMalloc(sizeof(MAKE_SCOPE_CONTEXT) + (Directory->LengthInChars + 1) * sizeof(TCHAR));
    if (ScopeContext == NULL) {
        return NULL;
    }

    ScopeContext->ParentScope = NULL;
    ScopeContext->PreviousScope = NULL;
    ScopeContext->MakeContext = MakeContext;
    ScopeContext->ReferenceCount = 2; // One for the caller, one for the hash

    ScopeContext->Variables = YoriLibAllocateHashTable(1000);
    if (ScopeContext->Variables == NULL) {
        YoriLibDereference(ScopeContext);
        return NULL;
    }

    //
    //  Copy and NULL terminate the directory
    //

    YoriLibInitEmptyString(&ScopeContext->CurrentIncludeDirectory);
    YoriLibReference(ScopeContext);
    ScopeContext->CurrentIncludeDirectory.LengthInChars = Directory->LengthInChars;
    ScopeContext->CurrentIncludeDirectory.LengthAllocated = Directory->LengthInChars + 1;
    ScopeContext->CurrentIncludeDirectory.StartOfString = (LPTSTR)(ScopeContext + 1);
    ScopeContext->CurrentIncludeDirectory.MemoryToFree = ScopeContext;

    memcpy(ScopeContext->CurrentIncludeDirectory.StartOfString, Directory->StartOfString, Directory->LengthInChars * sizeof(TCHAR));
    ScopeContext->CurrentIncludeDirectory.StartOfString[Directory->LengthInChars] = '\0';

    YoriLibHashInsertByKey(MakeContext->Scopes, &ScopeContext->CurrentIncludeDirectory, ScopeContext, &ScopeContext->HashEntry);

    YoriLibInitializeListHead(&ScopeContext->VariableList);
    YoriLibInitializeListHead(&ScopeContext->InferenceRuleList);
    YoriLibInitializeListHead(&ScopeContext->InferenceRuleNeededList);
    YoriLibAppendList(&MakeContext->ScopesList, &ScopeContext->ListEntry);

    ScopeContext->CurrentConditionalNestingLevel = 0;
    ScopeContext->ActiveConditionalNestingLevel = 0;
    ScopeContext->ParserState = MakeParserDefault;
    ScopeContext->ActiveConditionalNestingLevelExecutionEnabled = TRUE;
    ScopeContext->ActiveConditionalNestingLevelExecutionOccurred = FALSE;

    YoriLibConstantString(&Default, MAKE_DEFAULT_SCOPE_TARGET_NAME);
    ScopeContext->FirstUserTarget = NULL;
    ScopeContext->DefaultTarget = MakeLookupOrCreateTarget(ScopeContext, &Default, FALSE);
    if (ScopeContext->DefaultTarget == NULL) {
        if (ScopeContext->Variables != NULL) {
            YoriLibFreeEmptyHashTable(ScopeContext->Variables);
        }
        YoriLibRemoveListItem(&ScopeContext->ListEntry);
        YoriLibHashRemoveByEntry(&ScopeContext->HashEntry);
        YoriLibFreeStringContents(&ScopeContext->CurrentIncludeDirectory);
        YoriLibDereference(ScopeContext);
        return NULL;
    }

    return ScopeContext;
}

/**
 Reference a scope context.

 @param ScopeContext Pointer to the scope context.
 */
VOID
MakeReferenceScope(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    )
{
    InterlockedIncrement(&ScopeContext->ReferenceCount);
}

/**
 Dereference and potentially free a scope context.

 @param ScopeContext Pointer to the scope context.
 */
VOID
MakeDereferenceScope(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    )
{
    if (InterlockedDecrement(&ScopeContext->ReferenceCount) == 0) {

#if MAKE_DEBUG_SCOPE
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Deleting scope %y\n"), &ScopeContext->HashEntry.Key);
#endif

        YoriLibHashRemoveByEntry(&ScopeContext->HashEntry);
        YoriLibFreeStringContents(&ScopeContext->CurrentIncludeDirectory);
        MakeDeleteAllVariables(ScopeContext);
        if (ScopeContext->Variables != NULL) {
            YoriLibFreeEmptyHashTable(ScopeContext->Variables);
        }

        YoriLibDereference(ScopeContext);
    }
}

/**
 Find an existing scope or allocate a new scope for a child directory and
 initialize it as needed.

 @param MakeContext Pointer to the global make context.

 @param DirName Pointer to the relative directory name to create a scope for.
        This name is relative to the name of the active scope.

 @param FoundExisting On successful completion, set to TRUE to indicate a
        previously parsed scope was found, or FALSE if a new scope was
        created which requires its makefile to be parsed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
MakeActivateScope(
    __in PMAKE_CONTEXT MakeContext,
    __in PYORI_STRING DirName,
    __out PBOOLEAN FoundExisting
    )
{
    PMAKE_SCOPE_CONTEXT ScopeContext;
    PYORI_HASH_ENTRY HashEntry;
    YORI_STRING FullDir;

    YoriLibInitEmptyString(&FullDir);

    YoriLibYPrintf(&FullDir, _T("%y\\%y"), &MakeContext->ActiveScope->HashEntry.Key, DirName);
    if (FullDir.StartOfString == NULL) {
        return FALSE;
    }

    HashEntry = YoriLibHashLookupByKey(MakeContext->Scopes, &FullDir);
    if (HashEntry != NULL) {
        YoriLibFreeStringContents(&FullDir);
        ScopeContext = HashEntry->Context;
#if MAKE_DEBUG_SCOPE
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Entering existing scope %y\n"), &ScopeContext->HashEntry.Key);
#endif
        ScopeContext->PreviousScope = MakeContext->ActiveScope;
        MakeReferenceScope(ScopeContext);
        MakeContext->ActiveScope = ScopeContext;
        *FoundExisting = TRUE;
        return TRUE;
    }

    *FoundExisting = FALSE;
    ScopeContext = MakeAllocateNewScope(MakeContext, &FullDir);
    YoriLibFreeStringContents(&FullDir);
    if (ScopeContext == NULL) {
        return FALSE;
    }

#if MAKE_DEBUG_SCOPE
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Entering new scope %y\n"), &ScopeContext->HashEntry.Key);
#endif
    ScopeContext->ParentScope = MakeContext->ActiveScope;
    ScopeContext->PreviousScope = MakeContext->ActiveScope;
    ScopeContext->ActiveConditionalNestingLevelExecutionEnabled = TRUE;
    MakeContext->ActiveScope = ScopeContext;
    return TRUE;
}

/**
 Indicate that a scope is no longer active, dereferencing it by virtue of
 no longer being active.  This scope may still be referenced by targets
 including inference rules.

 @param ScopeContext Pointer to the context to deactivate.
 */
VOID
MakeDeactivateScope(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    )
{
    PMAKE_SCOPE_CONTEXT PreviousScopeContext;
    PMAKE_CONTEXT MakeContext;

    MakeContext = ScopeContext->MakeContext;
    ASSERT(MakeContext->ActiveScope == ScopeContext ||
           (MakeContext->RootScope == ScopeContext && MakeContext->ActiveScope == NULL));

    MakeFindInferenceRulesForScope(ScopeContext);

#if MAKE_DEBUG_SCOPE
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Leaving scope %y\n"), &ScopeContext->HashEntry.Key);
#endif
    MakeDeactivateAllInferenceRules(ScopeContext);
    PreviousScopeContext = ScopeContext->PreviousScope;
    ScopeContext->PreviousScope = NULL;
    MakeDereferenceScope(ScopeContext);
    MakeContext->ActiveScope = PreviousScopeContext;
}

/**
 Deallocate all scopes within the specified context.

 @param MakeContext Pointer to the context.
 */
VOID
MakeDeleteAllScopes(
    __inout PMAKE_CONTEXT MakeContext
    )
{ 
    PYORI_LIST_ENTRY ListEntry = NULL;
    PMAKE_SCOPE_CONTEXT ScopeContext;

    ListEntry = YoriLibGetNextListEntry(&MakeContext->ScopesList, NULL);
    while (ListEntry != NULL) {
        ScopeContext = CONTAINING_RECORD(ListEntry, MAKE_SCOPE_CONTEXT, ListEntry);
        YoriLibRemoveListItem(&ScopeContext->ListEntry);
        MakeDereferenceScope(ScopeContext);
        ListEntry = YoriLibGetNextListEntry(&MakeContext->ScopesList, NULL);
    }
    YoriLibFreeEmptyHashTable(MakeContext->Scopes);
}

// vim:sw=4:ts=4:et:
