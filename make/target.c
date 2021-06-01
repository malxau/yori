/**
 * @file make/target.c
 *
 * Yori shell make target support
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

VOID
MakeDereferenceInferenceRule(
    __in PMAKE_INFERENCE_RULE InferenceRule
    );

/**
 Dereference and potentially free a target.

 @param Target Pointer to the target.
 */
VOID
MakeDereferenceTarget(
    __in PMAKE_TARGET Target
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PMAKE_CMD_TO_EXEC CmdToExec;

    if (InterlockedDecrement(&Target->ReferenceCount) == 0) {
        YoriLibFreeStringContents(&Target->Recipe);

        if (Target->InferenceRule != NULL) {
            MakeDereferenceInferenceRule(Target->InferenceRule);
            Target->InferenceRule = NULL;
        }

        if (Target->ScopeContext != NULL) {
            MakeDereferenceScope(Target->ScopeContext);
            Target->ScopeContext = NULL;
        }

        ListEntry = YoriLibGetNextListEntry(&Target->ExecCmds, NULL);
        while (ListEntry != NULL) {
            CmdToExec = CONTAINING_RECORD(ListEntry, MAKE_CMD_TO_EXEC, ListEntry);
            ListEntry = YoriLibGetNextListEntry(&Target->ExecCmds, ListEntry);

            YoriLibFreeStringContents(&CmdToExec->Cmd);
            YoriLibFree(CmdToExec);
        }

        if (Target->InferenceRuleParentTarget != NULL) {
            MakeDereferenceTarget(Target->InferenceRuleParentTarget);
            Target->InferenceRuleParentTarget = NULL;
        }

        MakeSlabFree(Target);
    }
}

/**
 Indicate that a target can no longer be resolved, dereferencing it since it
 is no longer active.  It may still be referenced by inference rules.

 @param Target Pointer to the target to deactivate.
 */
VOID
MakeDeactivateTarget(
    __in PMAKE_TARGET Target
    )
{
    ASSERT(YoriLibIsListEmpty(&Target->ParentDependents));
    ASSERT(YoriLibIsListEmpty(&Target->ChildDependents));

    YoriLibRemoveListItem(&Target->ListEntry);
    YoriLibHashRemoveByEntry(&Target->HashEntry);
    MakeDereferenceTarget(Target);
}

/**
 Deallocate a single dependency.

 @param Dependency Pointer to the dependency to deallocate.
 */
VOID
MakeDeleteDependency(
    __in PMAKE_TARGET_DEPENDENCY Dependency
    )
{
    YoriLibRemoveListItem(&Dependency->ParentDependents);
    YoriLibRemoveListItem(&Dependency->ChildDependents);

    MakeSlabFree(Dependency);
}

/**
 Deallocate all targets within the specified context.

 @param MakeContext Pointer to the context.
 */
VOID
MakeDeleteAllTargets(
    __inout PMAKE_CONTEXT MakeContext
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PMAKE_TARGET Target;
    PMAKE_TARGET_DEPENDENCY Dependency;

    ListEntry = YoriLibGetNextListEntry(&MakeContext->TargetsList, NULL);
    while (ListEntry != NULL) {
        Target = CONTAINING_RECORD(ListEntry, MAKE_TARGET, ListEntry);
#if MAKE_DEBUG_TARGETS
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Deleting target: %y (probed %i exists %i timestamp %llx)\n"), &Target->HashEntry.Key, Target->FileProbed, Target->FileExists, Target->ModifiedTime.QuadPart);
#endif

        ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, NULL);
        while (ListEntry != NULL) {
            Dependency = CONTAINING_RECORD(ListEntry, MAKE_TARGET_DEPENDENCY, ChildDependents);
            MakeDeleteDependency(Dependency);
            ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, NULL);
        }

        ListEntry = YoriLibGetNextListEntry(&Target->ChildDependents, NULL);
        while (ListEntry != NULL) {
            Dependency = CONTAINING_RECORD(ListEntry, MAKE_TARGET_DEPENDENCY, ParentDependents);
            MakeDeleteDependency(Dependency);
            ListEntry = YoriLibGetNextListEntry(&Target->ChildDependents, NULL);
        }

        ListEntry = YoriLibGetNextListEntry(&MakeContext->TargetsList, &Target->ListEntry);
        MakeDeactivateTarget(Target);
    }

}

/**
 Open the target and query its timestamp.  The target may not exist (implying
 it needs to be rebuilt.)

 @param Target Pointer to the target to query.
 */
VOID
MakeProbeTargetFile(
    __in PMAKE_TARGET Target
    )
{
    HANDLE FileHandle;
    BY_HANDLE_FILE_INFORMATION FileInfo;

    if (Target->FileProbed) {
        return;
    }

    //
    //  Check if the object already exists, and if so, when it was last
    //  modified.
    //
    //  MSFIX In the longer run, one thing to consider would be using the
    //  USN value rather than timestamps.  These will be updated for any
    //  metadata operation so may be overactive, but the strict ordering
    //  makes it effectively impossible to have identical timestamps or
    //  clocks going backwards in time that produce false negatives.
    //

    ASSERT(YoriLibIsStringNullTerminated(&Target->HashEntry.Key));
    FileHandle = CreateFile(Target->HashEntry.Key.StartOfString, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (FileHandle != INVALID_HANDLE_VALUE) {
        if (GetFileInformationByHandle(FileHandle, &FileInfo)) {
            Target->FileExists = TRUE;
            if (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                Target->ModifiedTime.LowPart = 0;
                Target->ModifiedTime.HighPart = 0;
            } else {
                Target->ModifiedTime.LowPart = FileInfo.ftLastWriteTime.dwLowDateTime;
                Target->ModifiedTime.HighPart = FileInfo.ftLastWriteTime.dwHighDateTime;
            }
        }
        CloseHandle(FileHandle);
    }

    Target->FileProbed = TRUE;
}

/**
 Lookup a target in the current hash table of targets, and if it doesn't
 exist, create a new entry for it.

 @param ScopeContext Pointer to the scope context.

 @param TargetName Pointer to the target name.  This function will resolve
        this to a fully qualified path name.

 @return Pointer to the newly created target, or NULL on allocation failure.
 */
PMAKE_TARGET
MakeLookupOrCreateTarget(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING TargetName
    )
{
    YORI_STRING FullPath;
    PMAKE_TARGET Target;
    PYORI_HASH_ENTRY HashEntry;
    PMAKE_CONTEXT MakeContext;

    //
    //  MSFIX Make this cheaper.  Maybe we can consume the directory and
    //  unqualified file name into a single hash and only build the string
    //  for more complex cases?
    //

    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibGetFullPathNameRelativeTo(&ScopeContext->HashEntry.Key, TargetName, FALSE, &FullPath, NULL)) {
        return NULL;
    }

    MakeContext = ScopeContext->MakeContext;

    HashEntry = YoriLibHashLookupByKey(MakeContext->Targets, &FullPath);
    if (HashEntry != NULL) {
        Target = HashEntry->Context;
        YoriLibFreeStringContents(&FullPath);
    } else {

        Target = MakeSlabAlloc(&ScopeContext->MakeContext->TargetAllocator, sizeof(MAKE_TARGET));
        if (Target == NULL) {
            YoriLibFreeStringContents(&FullPath);
            return NULL;
        }
        ScopeContext->MakeContext->AllocTarget++;

        YoriLibInitializeListHead(&Target->ParentDependents);
        YoriLibInitializeListHead(&Target->ChildDependents);
        YoriLibInitializeListHead(&Target->RebuildList);
        YoriLibInitializeListHead(&Target->InferenceRuleNeededList);

        Target->ScopeContext = NULL;
        Target->ReferenceCount = 1;
        Target->NumberParentsToBuild = 0;
        Target->ExplicitRecipeFound = FALSE;
        Target->Executed = FALSE;
        Target->FileProbed = FALSE;
        Target->FileExists = FALSE;
        Target->ExecuteViaShell = FALSE;
        Target->RebuildRequired = FALSE;
        Target->DependenciesEvaluated = FALSE;
        Target->EvaluatingDependencies = FALSE;
        Target->InferenceRulePseudoTarget = FALSE;
        Target->ModifiedTime.QuadPart = 0;
        Target->InferenceRule = NULL;
        Target->InferenceRuleParentTarget = NULL;
        YoriLibInitEmptyString(&Target->Recipe);
        YoriLibInitializeListHead(&Target->ExecCmds);
        YoriLibHashInsertByKey(MakeContext->Targets, &FullPath, Target, &Target->HashEntry);
        YoriLibAppendList(&MakeContext->TargetsList, &Target->ListEntry);

        YoriLibFreeStringContents(&FullPath);
    }

    //
    //  An empty target name in the given scope refers to the default target
    //  for the scope.
    //

    if (TargetName->LengthInChars > 0) {
        ScopeContext->TargetCount++;
    }
#if MAKE_DEBUG_TARGETS
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Scope %y TargetCount %i Target %y\n"), &ScopeContext->HashEntry.Key, ScopeContext->TargetCount, &Target->HashEntry.Key);
#endif

    //
    //  The first user defined target within the scope should be executed in
    //  response to executing the scope.
    //

    if (ScopeContext->TargetCount == 1) {
        ASSERT(ScopeContext->DefaultTarget != NULL);
        if (!MakeCreateParentChildDependency(ScopeContext->MakeContext, Target, ScopeContext->DefaultTarget)) {
            MakeDeactivateTarget(Target);
            return NULL;
        }
    }

    return Target;
}

/**
 Write a string to a buffer that has been preallocated of sufficient length,
 and update a pointer to the next write location.  Indicate the location and
 length of the string into an output string.

 @param Target The string to update containing the buffer location and length.

 @param Source The string to copy.

 @param WritePoint On input, points to a pointer containing the buffer
        location to write string contents.  On output, updated to point to a
        pointer indicating the location that any future string copy should
        use.
 */
VOID
MakePopulateEmbededString(
    __out PYORI_STRING Target,
    __in PYORI_STRING Source,
    __inout LPTSTR *WritePoint
    )
{
    Target->StartOfString = *WritePoint;
    Target->LengthInChars = Source->LengthInChars;
    Target->LengthAllocated = Source->LengthInChars + 1;
    if (Source->LengthInChars > 0) {
        memcpy(Target->StartOfString, Source->StartOfString, Source->LengthInChars * sizeof(TCHAR));
    }
    Target->StartOfString[Source->LengthInChars] = '\0';
    *WritePoint = Target->StartOfString + Target->LengthAllocated;
}

/**
 Create a new inference rule, and insert it into the head of the list such
 that the most recently defined rule takes precedence over previously
 defined rules.

 @param ScopeContext Pointer to the scope context.

 @param SourceDir Points to the relative directory to apply to the scope
        that will be used to locate the file used as the source file for
        this rule.

 @param SourceExt Pointer to the file extension that can be used as the source
        file for this rule.

 @param TargetDir Points to the relative directory to apply to the scope
        that will be added to a file generated by executing this rule.

 @param TargetExt Pointer to the file extension that can be generated by
        executing this rule.

 @param Target Pointer to the pseudo-target that describes how to compile
        files from the source extension into the target extension.

 @return Pointer to the inference rule, or NULL on failure.
 */
PMAKE_INFERENCE_RULE
MakeCreateInferenceRule(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING SourceDir,
    __in PYORI_STRING SourceExt,
    __in PYORI_STRING TargetDir,
    __in PYORI_STRING TargetExt,
    __in PMAKE_TARGET Target
    )
{
    PMAKE_INFERENCE_RULE InferenceRule;
    DWORD CharsNeeded;
    LPTSTR WritePoint;

    CharsNeeded = SourceDir->LengthInChars + 1 +
                  SourceExt->LengthInChars + 1 +
                  TargetDir->LengthInChars + 1 +
                  TargetExt->LengthInChars + 1;

    InferenceRule = YoriLibMalloc(sizeof(MAKE_INFERENCE_RULE) + CharsNeeded * sizeof(TCHAR));
    if (InferenceRule == NULL) {
        return NULL;
    }
    ScopeContext->MakeContext->AllocInferenceRule++;

    InferenceRule->ReferenceCount = 1;
    YoriLibInitEmptyString(&InferenceRule->RelativeSourceDirectory);
    YoriLibInitEmptyString(&InferenceRule->SourceExtension);
    YoriLibInitEmptyString(&InferenceRule->RelativeTargetDirectory);
    YoriLibInitEmptyString(&InferenceRule->TargetExtension);

    WritePoint = (LPTSTR)(InferenceRule + 1);

    MakePopulateEmbededString(&InferenceRule->RelativeSourceDirectory, SourceDir, &WritePoint);
    MakePopulateEmbededString(&InferenceRule->SourceExtension, SourceExt, &WritePoint);
    MakePopulateEmbededString(&InferenceRule->RelativeTargetDirectory, TargetDir, &WritePoint);
    MakePopulateEmbededString(&InferenceRule->TargetExtension, TargetExt, &WritePoint);

#if MAKE_DEBUG_TARGETS
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Inference rule FromDir=%y FromExt=%y ToDir=%y ToExt=%y\n"), &InferenceRule->RelativeSourceDirectory, &InferenceRule->SourceExtension, &InferenceRule->RelativeTargetDirectory, &InferenceRule->TargetExtension);
#endif

    InterlockedIncrement(&Target->ReferenceCount);
    InferenceRule->Target = Target;
    InferenceRule->ScopeContext = ScopeContext;
    YoriLibInsertList(&ScopeContext->InferenceRuleList, &InferenceRule->ListEntry);

    return InferenceRule;
}

/**
 Reference an inference rule.

 @param InferenceRule Pointer to the inference rule to reference.
 */
VOID
MakeReferenceInferenceRule(
    __in PMAKE_INFERENCE_RULE InferenceRule
    )
{
    InferenceRule->ReferenceCount++;
}

/**
 Dereference an inference rule, potentially tearing down its target.

 @param InferenceRule Pointer to the inference rule to dereference.
 */
VOID
MakeDereferenceInferenceRule(
    __in PMAKE_INFERENCE_RULE InferenceRule
    )
{
    InferenceRule->ReferenceCount--;
    if (InferenceRule->ReferenceCount == 0) {
        if (!YoriLibIsListEmpty(&InferenceRule->ListEntry)) {
            YoriLibRemoveListItem(&InferenceRule->ListEntry);
            YoriLibInitializeListHead(&InferenceRule->ListEntry);
        }
        if (InferenceRule->Target != NULL) {
            ASSERT(InferenceRule->Target->InferenceRule == NULL);
            MakeDereferenceTarget(InferenceRule->Target);
        }
        YoriLibFree(InferenceRule);
    }
}

/**
 Indicate that all inference rules associated with a scope can no longer be
 used to resolve new targets.

 @param ScopeContext Pointer to the scope context.
 */
VOID
MakeDeactivateAllInferenceRules(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    )
{
    PMAKE_INFERENCE_RULE InferenceRule;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&ScopeContext->InferenceRuleList, NULL);
    while (ListEntry != NULL) {
        InferenceRule = CONTAINING_RECORD(ListEntry, MAKE_INFERENCE_RULE, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&ScopeContext->InferenceRuleList, ListEntry);
        YoriLibRemoveListItem(&InferenceRule->ListEntry);
        YoriLibInitializeListHead(&InferenceRule->ListEntry);
        MakeDereferenceInferenceRule(InferenceRule);
    }
}

/**
 Get the next inference rule that applies to this scope.  This will inherit
 inference rules from parent scopes.

 @param TopScope Pointer to the scope to search from.

 @param PreviousRule Pointer to the previously enumerated rule.  If NULL,
        searching starts from the beginning.

 @return Pointer to the next rule, or NULL if enumeration is complete.
 */
PMAKE_INFERENCE_RULE
MakeGetNextInferenceRule(
    __in PMAKE_SCOPE_CONTEXT TopScope,
    __in_opt PMAKE_INFERENCE_RULE PreviousRule
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PMAKE_SCOPE_CONTEXT CurrentScope;
    PMAKE_INFERENCE_RULE NextRule;

    //
    //  If starting from the top, use the top scope and the beginning of
    //  the list.  If resuming, use the scope context of the previous entry
    //  and the list position of it.
    //

    if (PreviousRule == NULL) {
        CurrentScope = TopScope;
        ListEntry = NULL;
    } else {
        CurrentScope = PreviousRule->ScopeContext;
        ListEntry = &PreviousRule->ListEntry;
    }

    //
    //  Move to the next list entry within that scope.  If there's another
    //  item, return it.  If not, move to the parent scope and the beginnning
    //  of its list.  If there's no parent to move to, enumeration is
    //  complete.
    //

    while (CurrentScope != NULL) {
        ListEntry = YoriLibGetNextListEntry(&CurrentScope->InferenceRuleList, ListEntry);
        if (ListEntry != NULL) {
            NextRule = CONTAINING_RECORD(ListEntry, MAKE_INFERENCE_RULE, ListEntry);
            return NextRule;
        }

        CurrentScope = CurrentScope->ParentScope;
        ListEntry = NULL;
    }

    return NULL;
}

/**
 Get the next inference rule that applies to this scope that can generate a
 specific file extension.  This will inherit inference rules from parent
 scopes.

 @param TopScope Pointer to the scope to search from.

 @param TargetExt Pointer to the file extension that needs to be generated.

 @param PreviousRule Pointer to the previously enumerated rule.  If NULL,
        searching starts from the beginning.

 @return Pointer to the next rule, or NULL if enumeration is complete.
 */
PMAKE_INFERENCE_RULE
MakeGetNextInferenceRuleTargetExtension(
    __in PMAKE_SCOPE_CONTEXT TopScope,
    __in PYORI_STRING TargetExt,
    __in_opt PMAKE_INFERENCE_RULE PreviousRule
    )
{
    PMAKE_INFERENCE_RULE NextRule;

    NextRule = MakeGetNextInferenceRule(TopScope, PreviousRule);
    while (NextRule != NULL) {
        if (YoriLibCompareStringInsensitive(TargetExt, &NextRule->TargetExtension) == 0) {
            return NextRule;
        }
        NextRule = MakeGetNextInferenceRule(TopScope, NextRule);
    }

    return NULL;
}

/**
 Determine how large a buffer would need to be to contain a full file path
 that would be the source of this inference rule.  Because the rule is
 substituting one extension for another, and one intermediate path for
 another, the number of characters to adjust a file path by is constant for
 any file path.

 @param InferenceRule Pointer to the inference rule specifying the source
        and target extension, and source and target directories.

 @return The number of additional characters needed to apply this rule. A
         rule may require fewer characters than the file name, in which case
         this function returns zero.
 */
DWORD
MakeCountExtraInferenceRuleChars(
    __in PMAKE_INFERENCE_RULE InferenceRule
    )
{
    DWORD CharsAdded;
    DWORD CharsRemoved;

    //
    //  Inference rules are applied backwards: we know a specific target is
    //  needed, and that string needs to be transformed into a source path
    //  to probe for a file.  So we want to add the source parts, and remove
    //  the target parts.
    //

    CharsAdded = InferenceRule->RelativeSourceDirectory.LengthInChars + InferenceRule->SourceExtension.LengthInChars;
    CharsRemoved = InferenceRule->RelativeTargetDirectory.LengthInChars + InferenceRule->TargetExtension.LengthInChars;

    //
    //  If a directory is being added and didn't previously exist, a new
    //  seperator is needed.
    //

    if (InferenceRule->RelativeTargetDirectory.LengthInChars == 0 &&
        InferenceRule->RelativeSourceDirectory.LengthInChars != 0) {

        CharsAdded++;
    }

    if (CharsAdded > CharsRemoved) {
        return CharsAdded - CharsRemoved;
    }

    return 0;
}

/**
 Apply an inference rule to a target file path.  This will substitute
 extensions and may substitute intermediate paths too.

 @param InferenceRule Pointer to the inference rule indicating the extensions
        and possibly directories to change.

 @param TargetNoExt Pointer to the target file name.  The extension is
        removed because the only rules this function are invoked on will have
        a matching extension.

 @param FileToProbe Pointer to a buffer to populate with a source file name
        to probe for existence.  If this file exists, the rule can be used
        to generate the target.

 @return TRUE to indicate that the rule is applicable and source file should
         be tested.  FALSE to indicate that the rule requires a directory
         which this target does not have, so it should not be evaluated.
 */
BOOLEAN
MakeBuildProbeNameFromInferenceRule(
    __in PMAKE_INFERENCE_RULE InferenceRule,
    __in PYORI_STRING TargetNoExt,
    __inout PYORI_STRING FileToProbe
    )
{
    YORI_STRING FileName;
    YORI_STRING Prefix;
    DWORD Index;
    DWORD DirIndex;
    DWORD SepIndex;
    TCHAR PathChar;
    TCHAR DirChar;

    //
    //  Capture the file name component, and capture the location of the
    //  final seperator.
    //

    SepIndex = 0;
    YoriLibInitEmptyString(&FileName);
    for (Index = TargetNoExt->LengthInChars; Index > 0; Index--) {
        if (YoriLibIsSep(TargetNoExt->StartOfString[Index - 1])) {
            FileName.StartOfString = &TargetNoExt->StartOfString[Index];
            FileName.LengthInChars = TargetNoExt->LengthInChars - Index;
            SepIndex = Index - 1;
            break;
        }
    }

    ASSERT(FileName.StartOfString != NULL);

    //
    //  If there's a relative target directory, check that this target is
    //  actually in it.  If not, this rule is inapplicable.  If the target
    //  directory matches, capture the location of the previous seperator.
    //

    if (InferenceRule->RelativeTargetDirectory.LengthInChars > 0) {
        DirIndex = InferenceRule->RelativeTargetDirectory.LengthInChars;
        for (Index = SepIndex; Index > 0 && DirIndex > 0; Index--, DirIndex--) {
            PathChar = TargetNoExt->StartOfString[Index - 1];
            DirChar = InferenceRule->RelativeTargetDirectory.StartOfString[DirIndex - 1];

            if (YoriLibUpcaseChar(PathChar) != YoriLibUpcaseChar(DirChar)) {

                if (!(YoriLibIsSep(PathChar) && YoriLibIsSep(DirChar))) {
                    return FALSE;
                }
            }
        }

        //
        //  We ran out of file path before running out of relative directory.
        //  This is not a match.
        //

        if (Index == 0) {
            return FALSE;
        }

        //
        //  If the char before the relative path isn't a seperator, it's not
        //  a match.
        //

        if (!YoriLibIsSep(TargetNoExt->StartOfString[Index - 1])) {
            return FALSE;
        }

        SepIndex = Index - 1;
    }

    //
    //  Build a source file path consisting of the common prefix, plus the
    //  source path, plus the file name, plus the inference rule source
    //  extension.
    //

    YoriLibInitEmptyString(&Prefix);
    Prefix.StartOfString = TargetNoExt->StartOfString;
    Prefix.LengthInChars = SepIndex + 1;

    if (InferenceRule->RelativeSourceDirectory.LengthInChars > 0) {
        FileToProbe->LengthInChars = YoriLibSPrintf(FileToProbe->StartOfString, _T("%y%y\\%y%y"), &Prefix, &InferenceRule->RelativeSourceDirectory, &FileName, &InferenceRule->SourceExtension);
    } else {
        FileToProbe->LengthInChars = YoriLibSPrintf(FileToProbe->StartOfString, _T("%y%y%y"), &Prefix, &FileName, &InferenceRule->SourceExtension);
    }
    return TRUE;
}

/**
 Once an inference rule has been determined to apply to a target, assign it
 and update all structures as necessary.

 @param ScopeContext Pointer to the scope context.

 @param Target Pointer to the target to assign an inference rule to.

 @param InferenceRule Pointer to the inference rule to assign.

 @param SourceFileName Pointer to the source file name to use.  This should
        be the name of the target with the source file extension of the
        inference rule.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeAssignInferenceRuleToTarget(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PMAKE_TARGET Target,
    __in PMAKE_INFERENCE_RULE InferenceRule,
    __in PYORI_STRING SourceFileName
    )
{
    Target->InferenceRuleParentTarget = MakeLookupOrCreateTarget(ScopeContext, SourceFileName);
    if (Target->InferenceRuleParentTarget == NULL) {
        return FALSE;
    }
    InterlockedIncrement(&Target->InferenceRuleParentTarget->ReferenceCount);
    MakeReferenceInferenceRule(InferenceRule);
    Target->InferenceRule = InferenceRule;

    //
    //  If the target has an explicit recipe but that doesn't indicate how
    //  to construct it and an inference rule does, it may be populated with
    //  the scope of the recipe, which makes sense to preserve.
    //

    ASSERT(Target->ScopeContext == NULL || Target->ExplicitRecipeFound);
    if (Target->ScopeContext == NULL) {
        MakeReferenceScope(ScopeContext);
        Target->ScopeContext = ScopeContext;
    }

    return TRUE;
}

/**
 Attempt to find an inference rule that could compile a specific target.
 There may or may not be a rule present that can do so.  If the target already
 has an explicit recipe or already has this resolved, this function returns
 immediately.  Otherwise it needs to check for a rule that can generate this
 target's extension based on a source file, and that source file actually
 exists.

 @param ScopeContext Pointer to the scope context.

 @param Target Pointer to the target to find an inference rule for.

 @return TRUE if the operation succeeded, FALSE if it did not.  Note that
         successful completion does not guarantee a rule was found, but this
         can be inferred from the state of the target.
 */
BOOLEAN
MakeFindInferenceRuleForTarget(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PMAKE_TARGET Target
    )
{
    PMAKE_INFERENCE_RULE InferenceRule;
    PMAKE_INFERENCE_RULE NestedRule;
    YORI_STRING TargetExt;
    YORI_STRING TargetNoExt;
    PYORI_STRING FileToProbe;
    PYORI_STRING NestedFileToProbe;
    DWORD Index;
    DWORD CharsNeeded;
    DWORD LongestCharsNeeded;
    BOOLEAN FoundRuleWithTargetExtension;

    //
    //  If it has an explicit recipe, it doesn't need an inference rule.
    //  If it already has found an inference rule, don't do it again.
    //

    ASSERT(Target->Recipe.LengthInChars == 0 &&
           Target->InferenceRule == NULL);

#if MAKE_DEBUG_TARGETS
    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Searching for inference rule for: %y\n"), &Target->HashEntry.Key);
#endif

    //
    //  Find the file extension of the target.  If there isn't one, then we
    //  can't match an inference rule against it.
    //

    YoriLibInitEmptyString(&TargetExt);
    for (Index = Target->HashEntry.Key.LengthInChars; Index > 0; Index--) {
        if (Target->HashEntry.Key.StartOfString[Index - 1] == '.') {
            TargetExt.StartOfString = &Target->HashEntry.Key.StartOfString[Index];
            TargetExt.LengthInChars = Target->HashEntry.Key.LengthInChars - Index;
            break;
        } else if (YoriLibIsSep(Target->HashEntry.Key.StartOfString[Index - 1])) {
            return TRUE;
        }
    }

    if (TargetExt.LengthInChars == 0) {
        return TRUE;
    }

    //
    //  Find the longest source extension from the known set of inference
    //  rules.  This is used to size the full path name allocation when
    //  probing for existing files.  Note this considers all rules, not just
    //  those matching extensions, so that it can correctly allocate buffers
    //  for the recursive inference rule search below.
    //

    LongestCharsNeeded = 0;
    FoundRuleWithTargetExtension = FALSE;
    InferenceRule = MakeGetNextInferenceRule(ScopeContext, NULL);
    while (InferenceRule != NULL) {
        FoundRuleWithTargetExtension = TRUE;
        CharsNeeded = MakeCountExtraInferenceRuleChars(InferenceRule);
        if (CharsNeeded > LongestCharsNeeded) {
            LongestCharsNeeded = CharsNeeded;
        }
        InferenceRule = MakeGetNextInferenceRule(ScopeContext, InferenceRule);
    }

    if (!FoundRuleWithTargetExtension) {
        return TRUE;
    }

    FileToProbe = &ScopeContext->MakeContext->FilesToProbe[0];

    CharsNeeded = Target->HashEntry.Key.LengthInChars + LongestCharsNeeded + 1;
    if (CharsNeeded > FileToProbe->LengthAllocated) {
        if (!YoriLibReallocateStringWithoutPreservingContents(FileToProbe, CharsNeeded * 2)) {
            return FALSE;
        }
    }

    YoriLibInitEmptyString(&TargetNoExt);
    TargetNoExt.StartOfString = Target->HashEntry.Key.StartOfString;
    TargetNoExt.LengthInChars = Target->HashEntry.Key.LengthInChars - TargetExt.LengthInChars;

    //
    //  Copy the base name of the target (without the extension, but with the
    //  period.) 
    //

    FileToProbe->LengthInChars = Target->HashEntry.Key.LengthInChars - TargetExt.LengthInChars;
    memcpy(FileToProbe->StartOfString, Target->HashEntry.Key.StartOfString, FileToProbe->LengthInChars * sizeof(TCHAR));

    FoundRuleWithTargetExtension = FALSE;

    InferenceRule = MakeGetNextInferenceRuleTargetExtension(ScopeContext, &TargetExt, NULL);
    while (InferenceRule != NULL) {
        if (MakeBuildProbeNameFromInferenceRule(InferenceRule, &TargetNoExt, FileToProbe)) {
            FoundRuleWithTargetExtension = TRUE;
#if MAKE_DEBUG_TARGETS
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("GetFileAttributes for: %s\n"), FileToProbe->StartOfString);
#endif
            if (GetFileAttributes(FileToProbe->StartOfString) != (DWORD)-1) {
                FileToProbe->LengthInChars = FileToProbe->LengthInChars + InferenceRule->SourceExtension.LengthInChars;
                if (!MakeAssignInferenceRuleToTarget(ScopeContext, Target, InferenceRule, FileToProbe)) {
                    return FALSE;
                }
                return TRUE;
            }
        }
        InferenceRule = MakeGetNextInferenceRuleTargetExtension(ScopeContext, &TargetExt, InferenceRule);
    }

    //
    //  If there's no inference rule that can generate this extension in this
    //  path, give up.
    //

    if (!FoundRuleWithTargetExtension) {
        return TRUE;
    }

    //
    //  Getting here implies there is a rule that can generate this extension,
    //  but the source file for it could not be found.  If this occurs, probe
    //  one level deeper to see if there's a rule that could generate that
    //  intermediate extension.
    //

    NestedFileToProbe = &ScopeContext->MakeContext->FilesToProbe[1];
    InferenceRule = MakeGetNextInferenceRuleTargetExtension(ScopeContext, &TargetExt, NULL);
    while (InferenceRule != NULL) {
        if (MakeBuildProbeNameFromInferenceRule(InferenceRule, &TargetNoExt, FileToProbe)) {
            //
            //  Chop off the intermediate extension so the next call to
            //  MakeBuildProbeNameFromInferenceRule has the correct path
            //

            ASSERT(FileToProbe->LengthInChars > InferenceRule->SourceExtension.LengthInChars);

            CharsNeeded = FileToProbe->LengthInChars + LongestCharsNeeded + 1;
            if (CharsNeeded > NestedFileToProbe->LengthAllocated) {
                if (!YoriLibReallocateStringWithoutPreservingContents(NestedFileToProbe, CharsNeeded * 2)) {
                    return FALSE;
                }
            }

            FileToProbe->LengthInChars = FileToProbe->LengthInChars - InferenceRule->SourceExtension.LengthInChars;

            NestedRule = MakeGetNextInferenceRuleTargetExtension(ScopeContext, &InferenceRule->SourceExtension, NULL);
            while (NestedRule != NULL) {
                if (MakeBuildProbeNameFromInferenceRule(NestedRule, FileToProbe, NestedFileToProbe)) {

#if MAKE_DEBUG_TARGETS
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Nested GetFileAttributes for: %s\n"), NestedFileToProbe->StartOfString);
#endif
                    if (GetFileAttributes(NestedFileToProbe->StartOfString) != (DWORD)-1) {

                        //
                        //  First, generate the outer rule, assigning the
                        //  inference rule to the input target.  This will
                        //  lookup or create the nested target.
                        //

                        FileToProbe->LengthInChars = FileToProbe->LengthInChars + InferenceRule->SourceExtension.LengthInChars;

                        if (!MakeAssignInferenceRuleToTarget(ScopeContext, Target, InferenceRule, FileToProbe)) {
                            return FALSE;
                        }

                        //
                        //  Now generate the inner rule, rebuilding the test
                        //  file name.  This is done later because the nested
                        //  target is now known.
                        //

                        if (!MakeAssignInferenceRuleToTarget(ScopeContext, Target->InferenceRuleParentTarget, NestedRule, NestedFileToProbe)) {
                            return FALSE;
                        }
                        break;
                    }
                }
                NestedRule = MakeGetNextInferenceRuleTargetExtension(ScopeContext, &InferenceRule->SourceExtension, NestedRule);
            }
            if (Target->InferenceRule != NULL) {
                break;
            }
        }
        InferenceRule = MakeGetNextInferenceRuleTargetExtension(ScopeContext, &TargetExt, InferenceRule);
    }

    return TRUE;
}

/**
 Return TRUE if the target might benefit from an inference rule.  If the
 target already has an explicit recipe or an inference rule, then it would
 not benefit from one.  Otherwise, one may be needed to build the target.

 @param Target Pointer to the target to check.
 */
BOOLEAN
MakeWouldTargetBenefitFromInferenceRule(
    __in PMAKE_TARGET Target
    )
{
    if (Target->Recipe.LengthInChars == 0 &&
        Target->InferenceRule == NULL) {

        return TRUE;
    }

    return FALSE;
}

/**
 Indicate that a target might need to be built via an inference rule to
 complete this scope.

 @param ScopeContext Pointer to the scope context.

 @param Target Pointer to the target.
 */
VOID
MakeMarkTargetInferenceRuleNeededIfNeeded(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PMAKE_TARGET Target
    )
{
    if (!YoriLibIsListEmpty(&Target->InferenceRuleNeededList)) {
        YoriLibRemoveListItem(&Target->InferenceRuleNeededList);
        YoriLibInitializeListHead(&Target->InferenceRuleNeededList);
#if MAKE_DEBUG_TARGETS
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Removing target from potential inference rule search: %y\n"), &Target->HashEntry.Key);
#endif
    }

    if (MakeWouldTargetBenefitFromInferenceRule(Target)) {

#if MAKE_DEBUG_TARGETS
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Queueing target for potential inference rule search: %y\n"), &Target->HashEntry.Key);
#endif
        YoriLibAppendList(&ScopeContext->InferenceRuleNeededList, &Target->InferenceRuleNeededList);
    }
}

/**
 At scope termination, go through any targets which indicated that they might
 need to be built by an inference rule.  If a later rule specified how to
 build these targets, the process is complete.  If not, try to find matching
 inference rules that can be used to construct the target.

 @param ScopeContext Pointer to the scope context that is completing.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeFindInferenceRulesForScope(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    )
{
    PMAKE_TARGET Target;

    while (TRUE) {
        if (YoriLibIsListEmpty(&ScopeContext->InferenceRuleNeededList)) {
            break;
        }

        Target = CONTAINING_RECORD(ScopeContext->InferenceRuleNeededList.Next, MAKE_TARGET, InferenceRuleNeededList);
        YoriLibRemoveListItem(&Target->InferenceRuleNeededList);
        YoriLibInitializeListHead(&Target->InferenceRuleNeededList);

        if (!MakeWouldTargetBenefitFromInferenceRule(Target)) {
            continue;
        }

        if (!MakeFindInferenceRuleForTarget(ScopeContext, Target)) {
#if MAKE_DEBUG_TARGETS
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Search for inference rule failed for: %y\n"), &Target->HashEntry.Key);
#endif
            return FALSE;
        }
    }

    return TRUE;
}


/**
 Describe the relationship between a parent and a child in a dependency
 relationship.  A child can only be built once its parents have been built.

 @param MakeContext Pointer to the context.

 @param Parent Pointer to the parent target.

 @param Child Pointer to the child target.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeCreateParentChildDependency(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET Parent,
    __in PMAKE_TARGET Child
    )
{
    PMAKE_TARGET_DEPENDENCY Dependency;

    Dependency = MakeSlabAlloc(&MakeContext->DependencyAllocator, sizeof(MAKE_TARGET_DEPENDENCY));
    if (Dependency == NULL) {
        return FALSE;
    }

#if MAKE_DEBUG_TARGETS
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Creating parent %y for child %y\n"), &Parent->HashEntry.Key, &Child->HashEntry.Key);
#endif

    MakeContext->AllocDependency++;

    Dependency->Parent = Parent;
    Dependency->Child = Child;
    YoriLibAppendList(&Parent->ChildDependents, &Dependency->ParentDependents);
    YoriLibAppendList(&Child->ParentDependents, &Dependency->ChildDependents);

    return TRUE;
}

/**
 Expand a target specific special variable.

 @param MakeContext Pointer to the context.

 @param Target Pointer to the target.

 @param VariableName Pointer to the variable name to expand.

 @param VariableData On successful completion, updated to contain the
        variable contents.  This may point directly at previously generated
        data, or may be allocated and generated as part of this call.  The
        caller should call YoriLibFreeStringContents on this string which
        may or may not have any data to free.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
MakeExpandTargetVariable(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET Target,
    __in PCYORI_STRING VariableName,
    __out PYORI_STRING VariableData
    )
{
    DWORD Index;
    PYORI_LIST_ENTRY ListEntry;
    PMAKE_TARGET_DEPENDENCY DependentTarget;
    YORI_STRING BaseVariableName;
    YORI_STRING FileNamePartQualifier;
    DWORD SymbolChars;
    BOOLEAN Result;

    SymbolChars = YoriLibCountStringContainingChars(VariableName, _T("@*<?"));

    //
    //  We should only be here if the variable was target specific, which
    //  implies it starts with these chars
    //
    ASSERT(SymbolChars > 0);
    if (SymbolChars == 0) {
        return FALSE;
    }
    MakeProbeTargetFile(Target);

    YoriLibInitEmptyString(&BaseVariableName);
    BaseVariableName.StartOfString = VariableName->StartOfString;
    BaseVariableName.LengthInChars = VariableName->LengthInChars;

    YoriLibInitEmptyString(&FileNamePartQualifier);
    if (VariableName->LengthInChars > SymbolChars) {
        BaseVariableName.LengthInChars = SymbolChars;
        FileNamePartQualifier.StartOfString = &VariableName->StartOfString[SymbolChars];
        FileNamePartQualifier.LengthInChars = VariableName->LengthInChars - SymbolChars;
    }

    Result = FALSE;

    if (YoriLibCompareStringWithLiteral(&BaseVariableName, _T("@")) == 0) {
        VariableData->StartOfString = Target->HashEntry.Key.StartOfString;
        VariableData->LengthInChars = Target->HashEntry.Key.LengthInChars;
        Result = TRUE;
    } else if (YoriLibCompareStringWithLiteral(&BaseVariableName, _T("*")) == 0) {
        VariableData->StartOfString = Target->HashEntry.Key.StartOfString;

        //
        //  Look backwards for a file extension or path seperator.  If we find
        //  an extension first, truncate it; if we find a seperator, use the
        //  entire string.  If we don't find either, use the whole string.
        //

        for (Index = Target->HashEntry.Key.LengthInChars; Index > 0; Index--) {
            if (Target->HashEntry.Key.StartOfString[Index - 1] == '.') {
                Index = Index - 1;
                break;
            } else if (YoriLibIsSep(Target->HashEntry.Key.StartOfString[Index - 1])) {
                Index = Target->HashEntry.Key.LengthInChars;
                break;
            }
        }

        if (Index == 0) {
            Index = Target->HashEntry.Key.LengthInChars;
        }
        VariableData->LengthInChars = Index;
        Result = TRUE;
    } else if (YoriLibCompareStringWithLiteral(&BaseVariableName, _T("?")) == 0) {
        Index = 0;
        ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, NULL);
        while (ListEntry != NULL) {
            DependentTarget = CONTAINING_RECORD(ListEntry, MAKE_TARGET_DEPENDENCY, ChildDependents);
            MakeProbeTargetFile(DependentTarget->Parent);
            if (!Target->FileExists ||
                !DependentTarget->Parent->FileExists ||
                DependentTarget->Parent->ModifiedTime.QuadPart > Target->ModifiedTime.QuadPart) {

                Index = Index + DependentTarget->Parent->HashEntry.Key.LengthInChars + 1;
            }
            ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, ListEntry);
        }

        if (!YoriLibAllocateString(VariableData, Index + 1)) {
            return FALSE;
        }
        MakeContext->AllocVariableData++;

        Index = 0;
        ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, NULL);
        while (ListEntry != NULL) {
            DependentTarget = CONTAINING_RECORD(ListEntry, MAKE_TARGET_DEPENDENCY, ChildDependents);
            MakeProbeTargetFile(DependentTarget->Parent);
            if (!Target->FileExists ||
                !DependentTarget->Parent->FileExists ||
                DependentTarget->Parent->ModifiedTime.QuadPart > Target->ModifiedTime.QuadPart) {

                memcpy(&VariableData->StartOfString[Index],
                       DependentTarget->Parent->HashEntry.Key.StartOfString,
                       DependentTarget->Parent->HashEntry.Key.LengthInChars * sizeof(TCHAR));

                Index = Index + DependentTarget->Parent->HashEntry.Key.LengthInChars;
                VariableData->StartOfString[Index] = ' ';
            }
            ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, ListEntry);
        }
        VariableData->StartOfString[Index] = '\0';
        VariableData->LengthInChars = Index;
        Result = TRUE;
    } else if (YoriLibCompareStringWithLiteral(&BaseVariableName, _T("**")) == 0) {
        Index = 0;
        ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, NULL);
        while (ListEntry != NULL) {
            DependentTarget = CONTAINING_RECORD(ListEntry, MAKE_TARGET_DEPENDENCY, ChildDependents);
            Index = Index + DependentTarget->Parent->HashEntry.Key.LengthInChars + 1;
            ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, ListEntry);
        }

        if (!YoriLibAllocateString(VariableData, Index + 1)) {
            return FALSE;
        }
        MakeContext->AllocVariableData++;

        Index = 0;
        ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, NULL);
        while (ListEntry != NULL) {
            DependentTarget = CONTAINING_RECORD(ListEntry, MAKE_TARGET_DEPENDENCY, ChildDependents);

            memcpy(&VariableData->StartOfString[Index],
                   DependentTarget->Parent->HashEntry.Key.StartOfString,
                   DependentTarget->Parent->HashEntry.Key.LengthInChars * sizeof(TCHAR));

            Index = Index + DependentTarget->Parent->HashEntry.Key.LengthInChars;
            VariableData->StartOfString[Index] = ' ';
            ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, ListEntry);

            if (ListEntry != NULL) {
                Index = Index + 1;
            }
        }
        VariableData->StartOfString[Index] = '\0';
        VariableData->LengthInChars = Index;
        Result = TRUE;
    } else if (YoriLibCompareStringWithLiteral(&BaseVariableName, _T("<")) == 0 &&
               Target->InferenceRule != NULL) {

        YORI_STRING BaseName;
        DWORD CharsNeeded;
        BOOLEAN PathMatch;

        CharsNeeded = MakeCountExtraInferenceRuleChars(Target->InferenceRule);
        if (!YoriLibAllocateString(VariableData, Target->HashEntry.Key.LengthInChars + CharsNeeded + 1)) {
            return FALSE;
        }
        MakeContext->AllocVariableData++;

        YoriLibInitEmptyString(&BaseName);
        BaseName.StartOfString = Target->HashEntry.Key.StartOfString;
        BaseName.LengthInChars = Target->HashEntry.Key.LengthInChars;
        for (Index = Target->HashEntry.Key.LengthInChars; Index > 0; Index--) {
            if (Target->HashEntry.Key.StartOfString[Index - 1] == '.') {
                BaseName.LengthInChars = Index;
                break;
            }
        }

        PathMatch = MakeBuildProbeNameFromInferenceRule(Target->InferenceRule, &BaseName, VariableData);

        //
        //  An inference rule shouldn't be attached to the target if it
        //  can't process the target.
        //

        ASSERT(PathMatch);

        Result = TRUE;
    }

    if (Result == FALSE) {
        return Result;
    }

    if (FileNamePartQualifier.LengthInChars == 0) {
        DWORD LastFoundSepIndex;
        YORI_STRING ScopeDir;

        YoriLibInitEmptyString(&ScopeDir);
        ScopeDir.StartOfString = Target->ScopeContext->HashEntry.Key.StartOfString;
        ScopeDir.LengthInChars = Target->ScopeContext->HashEntry.Key.LengthInChars;
        LastFoundSepIndex = 0;

        if (YoriLibIsPathPrefixed(&ScopeDir)) {
            ScopeDir.StartOfString = ScopeDir.StartOfString + (sizeof("\\\\.\\") - 1);
            ScopeDir.LengthInChars = ScopeDir.LengthInChars - (sizeof("\\\\.\\") - 1);
        }

        for (Index = 0; Index < ScopeDir.LengthInChars; Index++) {
            if (Index >= VariableData->LengthInChars ||
                YoriLibUpcaseChar(VariableData->StartOfString[Index]) != YoriLibUpcaseChar(ScopeDir.StartOfString[Index]))

                break;
        }

        if (Index == ScopeDir.LengthInChars) {
            while (Index < VariableData->LengthInChars &&
                   YoriLibIsSep(VariableData->StartOfString[Index])) {
                Index++;
            }

            VariableData->StartOfString = VariableData->StartOfString + Index;
            VariableData->LengthInChars = VariableData->LengthInChars - Index;
        }
    } else if (YoriLibCompareStringWithLiteralInsensitive(&FileNamePartQualifier, _T("B")) == 0) {
        BOOLEAN FinalDotFound = FALSE;
        BOOLEAN FinalSeperatorFound = FALSE;
        DWORD FinalDotIndex = 0;

        for (Index = VariableData->LengthInChars; Index > 0; Index--) {
            if (FinalDotIndex == 0 && VariableData->StartOfString[Index - 1] == '.') {
                FinalDotIndex = Index - 1;
                FinalDotFound = TRUE;
            } else if (YoriLibIsSep(VariableData->StartOfString[Index - 1])) {
                FinalSeperatorFound = TRUE;
                break;
            }
        }

        if (FinalDotFound) {
            VariableData->LengthInChars = FinalDotIndex;
        }

        if (FinalSeperatorFound) {
            VariableData->LengthInChars = VariableData->LengthInChars - Index;
            VariableData->StartOfString = VariableData->StartOfString + Index;
        }

    } else if (YoriLibCompareStringWithLiteralInsensitive(&FileNamePartQualifier, _T("D")) == 0) {

        BOOLEAN FinalSeperatorFound = FALSE;

        for (Index = VariableData->LengthInChars; Index > 0; Index--) {
            if (YoriLibIsSep(VariableData->StartOfString[Index - 1])) {
                FinalSeperatorFound = TRUE;
                break;
            }
        }

        if (FinalSeperatorFound) {
            VariableData->LengthInChars = Index - 1;
        }

    } else if (YoriLibCompareStringWithLiteralInsensitive(&FileNamePartQualifier, _T("F")) == 0) {
        BOOLEAN FinalSeperatorFound = FALSE;

        for (Index = VariableData->LengthInChars; Index > 0; Index--) {
            if (YoriLibIsSep(VariableData->StartOfString[Index - 1])) {
                FinalSeperatorFound = TRUE;
                break;
            }
        }

        if (FinalSeperatorFound) {
            VariableData->LengthInChars = VariableData->LengthInChars - Index;
            VariableData->StartOfString = VariableData->StartOfString + Index;
        }


    } else if (YoriLibCompareStringWithLiteralInsensitive(&FileNamePartQualifier, _T("R")) == 0) {
        BOOLEAN FinalDotFound = FALSE;

        for (Index = VariableData->LengthInChars; Index > 0; Index--) {
            if (VariableData->StartOfString[Index - 1] == '.') {
                FinalDotFound = TRUE;
                break;
            } else if (YoriLibIsSep(VariableData->StartOfString[Index - 1])) {
                break;
            }
        }

        if (FinalDotFound) {
            VariableData->LengthInChars = Index - 1;
        }

    } else {
        YoriLibFreeStringContents(VariableData);
        Result = FALSE;
    }

    return Result;
}

/**
 Parse through a recipe or inference rule and generate the commands to
 execute.  This includes things like target specific variable expansion,
 and in future generating a target specific script from an inference rule
 as well as potentially more processing.

 @param MakeContext Pointer to the context.

 @param Target Pointer to the target to generate commands for.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeGenerateExecScriptForTarget(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET Target
    )
{
    YORI_STRING Line;
    DWORD StartLineIndex;
    DWORD Index;
    PYORI_STRING SourceString;
    PMAKE_CMD_TO_EXEC CmdToExec;

    UNREFERENCED_PARAMETER(MakeContext);

    //
    //  MSFIX: NMAKE will use the inference rule if the target's recipe is
    //  empty and an inference rule exists.  This allows a makefile to specify
    //  dependencies without recipes and have the inference rules supply
    //  recipes.  Note that a target with no string but has built dependencies
    //  is still successful.
    //

    SourceString = NULL;
    if (Target->Recipe.LengthInChars > 0) {
        SourceString = &Target->Recipe;
    } else if (Target->InferenceRule != NULL) {
        ASSERT(Target->InferenceRuleParentTarget != NULL);
        ASSERT(!YoriLibIsListEmpty(&Target->ParentDependents));
        SourceString = &Target->InferenceRule->Target->Recipe;
    } else if (Target->ExplicitRecipeFound) {
        SourceString = &Target->Recipe;
    }

    //
    //  There's nothing to do, and it's been done successfully.
    //

    if (SourceString == NULL) {
        return TRUE;
    }

    ASSERT(Target->ScopeContext != NULL);
    __analysis_assume(Target->ScopeContext != NULL);

    YoriLibInitEmptyString(&Line);
    StartLineIndex = 0;
    for (Index = 0; Index < SourceString->LengthInChars; Index++) {
        if (SourceString->StartOfString[Index] == '\n') {
            Line.StartOfString = &SourceString->StartOfString[StartLineIndex];
            Line.LengthInChars = Index - StartLineIndex;

            StartLineIndex = Index + 1;

            CmdToExec = YoriLibMalloc(sizeof(MAKE_CMD_TO_EXEC));
            if (CmdToExec == NULL) {
                return FALSE;
            }

            CmdToExec->DisplayCmd = TRUE;
            CmdToExec->IgnoreErrors = FALSE;

            while (TRUE) {
                if (Line.LengthInChars > 0) {
                    if (Line.StartOfString[0] == '@') {
                        CmdToExec->DisplayCmd = FALSE;
                        Line.StartOfString++;
                        Line.LengthInChars--;
                    } else if (Line.StartOfString[0] == '-') {
                        CmdToExec->IgnoreErrors = TRUE;
                        Line.StartOfString++;
                        Line.LengthInChars--;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }


            YoriLibInitEmptyString(&CmdToExec->Cmd);
            if (!MakeExpandVariables(Target->ScopeContext, Target, &CmdToExec->Cmd, &Line)) {
                return FALSE;
            }

            YoriLibAppendList(&Target->ExecCmds, &CmdToExec->ListEntry);
        }
    }

    return TRUE;
}

/**
 Indicate that a specified target requires rebuilding, and add it to the
 appropriate list for the execution engine to consume.

 @param MakeContext Pointer to the context.

 @param Target Pointer to the target to mark for rebuilding.

 @return TRUE to indicate that the target is marked for rebuilding.  FALSE to
         indicate this could not be performed because there is not enough
         information to describe how to build the target.
 */
BOOLEAN
MakeMarkTargetForRebuild(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET Target
    )
{
    ASSERT(!Target->RebuildRequired);
    if (Target->RebuildRequired) {
        return TRUE;
    }

    if (!Target->ExplicitRecipeFound &&
         Target->InferenceRule == NULL &&
        YoriLibIsListEmpty(&Target->ParentDependents)) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Don't know how to build %y!\n"), &Target->HashEntry.Key);
        MakeContext->ErrorTermination = TRUE;
        return FALSE;
    }

    if (!MakeGenerateExecScriptForTarget(MakeContext, Target)) {
        return FALSE;
    }

    //
    //  MSFIX Ideally these lists would be sorted or approximately sorted
    //  where the targets that have the most dependencies are done before
    //  those with fewer dependencies.  Doing this intelligently really
    //  requires knowledge of all ancestors.  Appending to the end means
    //  that depth first traversal should ensure that all dependencies are
    //  satisfied, and if many targets depend on one target that target
    //  should be uncovered relatively early.
    //

    Target->RebuildRequired = TRUE;
    if (Target->NumberParentsToBuild == 0) {
        YoriLibAppendList(&MakeContext->TargetsReady, &Target->RebuildList);
    } else {
        YoriLibAppendList(&MakeContext->TargetsWaiting, &Target->RebuildList);
    }

    return TRUE;
}

/**
 When an inference rule is being used to build a target, apply all of the
 parent dependencies of the inference rule to be parent dependencies of the
 target.  This includes the implied dependency of the source of the inference
 rule.

 @param MakeContext Pointer to the context.

 @param Target Pointer to the target to apply the inference rule to.  This
        target already knows which inference rule it would use to build if no
        explicit recipe is found.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeApplyInferenceRuleDependencyToTarget(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET Target
    )
{
    PMAKE_TARGET InferenceRuleTarget;
    PYORI_LIST_ENTRY ListEntry;
    PMAKE_TARGET Parent;
    PMAKE_TARGET_DEPENDENCY Dependency;

    if (!MakeCreateParentChildDependency(MakeContext, Target->InferenceRuleParentTarget, Target)) {
        return FALSE;
    }

    InferenceRuleTarget = Target->InferenceRule->Target;

    ListEntry = YoriLibGetNextListEntry(&InferenceRuleTarget->ParentDependents, NULL);
    while (ListEntry != NULL) {
        Dependency = CONTAINING_RECORD(ListEntry, MAKE_TARGET_DEPENDENCY, ChildDependents);
        ASSERT(Dependency->Child == InferenceRuleTarget);
        Parent = Dependency->Parent;

        if (!MakeCreateParentChildDependency(MakeContext, Parent, Target)) {
            return FALSE;
        }

        ListEntry = YoriLibGetNextListEntry(&InferenceRuleTarget->ParentDependents, ListEntry);
    }

    return TRUE;
}

/**
 For a specified target, check whether anything it depends up requires
 rebuilding, and if so, indicate that this target requires rebuilding also.

 @param MakeContext Pointer to the context.

 @param Target Pointer to the target to evaluate.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeDetermineDependenciesForTarget(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET Target
    )
{
    PMAKE_TARGET_DEPENDENCY Dependency;
    PMAKE_TARGET Parent;
    PYORI_LIST_ENTRY ListEntry;
    BOOLEAN SetRebuildRequired;

    if (Target->DependenciesEvaluated) {
        return TRUE;
    }

    if (Target->EvaluatingDependencies) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Circular dependency encountered on %y\n"), &Target->HashEntry.Key);
        return FALSE;
    }

    MakeProbeTargetFile(Target);

    Target->EvaluatingDependencies = TRUE;

    SetRebuildRequired = FALSE;

    //
    //  Every parent target needs to be recursively evaluated because it
    //  may depend on something that is newer than the current version of
    //  the parent, implying the parent must be rebuilt.
    //

    ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, NULL);
    while (ListEntry != NULL) {
        Dependency = CONTAINING_RECORD(ListEntry, MAKE_TARGET_DEPENDENCY, ChildDependents);
        ASSERT(Dependency->Child == Target);
        Parent = Dependency->Parent;

        //
        //  If it uses an inference rule and has no parent dependencies,
        //  populate one from the inference rule
        //
        //  MSFIX NMAKE allows a parent to have parent dependents and still
        //  apply inference rules to build it
        //

        if (YoriLibIsListEmpty(&Parent->ParentDependents) &&
            !Parent->ExplicitRecipeFound &&
            Parent->InferenceRuleParentTarget != NULL) {

            if (!MakeApplyInferenceRuleDependencyToTarget(MakeContext, Parent)) {
                goto Fail;
            }
        }

        if (!MakeDetermineDependenciesForTarget(MakeContext, Parent)) {
            goto Fail;
        }
        if (Dependency->Parent->RebuildRequired) {
            Target->NumberParentsToBuild = Target->NumberParentsToBuild + 1;
            SetRebuildRequired = TRUE;
        }
        MakeProbeTargetFile(Parent);
        if (Parent->FileExists && Target->FileExists && Parent->ModifiedTime.QuadPart > Target->ModifiedTime.QuadPart) {
            SetRebuildRequired = TRUE;
        }
        ListEntry = YoriLibGetNextListEntry(&Target->ParentDependents, ListEntry);
    }

    Target->EvaluatingDependencies = FALSE;
    Target->DependenciesEvaluated = TRUE;

    if (!Target->FileExists) {
        SetRebuildRequired = TRUE;
    }

    if (SetRebuildRequired && !Target->RebuildRequired) {
#if MAKE_DEBUG_TARGETS
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("RebuildRequired on %y\n"), &Target->HashEntry.Key);
#endif
        if (!MakeMarkTargetForRebuild(MakeContext, Target)) {
            return FALSE;
        }
    }

    return TRUE;

Fail:
    Target->EvaluatingDependencies = FALSE;
    return FALSE;
}

/**
 Evaluate all of the dependencies for the requested build target to determine
 what requires rebuilding.

 MSFIX Right now this means the first target in the makefile.

 @param MakeContext Pointer to the context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeDetermineDependencies(
    __in PMAKE_CONTEXT MakeContext
    )
{
    PMAKE_TARGET Target;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&MakeContext->TargetsList, NULL);
    Target = NULL;
    while (TRUE) {
        if (ListEntry == NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No target to make."));
            MakeContext->ErrorTermination = TRUE;
            return FALSE;
        }

        Target = CONTAINING_RECORD(ListEntry, MAKE_TARGET, ListEntry);
        if (!Target->InferenceRulePseudoTarget) {
            break;
        }

        ListEntry = YoriLibGetNextListEntry(&MakeContext->TargetsList, ListEntry);
    }

    return MakeDetermineDependenciesForTarget(MakeContext, Target);
}

// vim:sw=4:ts=4:et:
