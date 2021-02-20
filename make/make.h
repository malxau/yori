/**
 * @file make/make.h
 *
 * Yori shell make master header
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


/**
 Set to nonzero to output extra information as part of preprocessing.
 */
#define MAKE_DEBUG_PREPROCESSOR 0

/**
 Set to nonzero to output extra information as part of launching child
 processes during preprocessing.
 */
#define MAKE_DEBUG_PREPROCESSOR_CREATEPROCESS 0

/**
 Set to nonzero to output extra information as part nested scope processing.
 */
#define MAKE_DEBUG_SCOPE        0

/**
 Set to nonzero to output extra information as part of variable state.
 */
#define MAKE_DEBUG_VARIABLES    0

/**
 Set to nonzero to output extra information as part of targets and dependency
 evaluation.
 */
#define MAKE_DEBUG_TARGETS      0

/**
 Set to nonzero to output extra information about time spent in different
 parts of the build process.
 */
#define MAKE_DEBUG_PERF         0


/**
 A structure to record information about how to allocate fixed sized
 structures in groups.
 */
typedef struct _MAKE_SLAB_ALLOC {

    /**
     The size of each element.
     */
    DWORD ElementSize;

    /**
     The number of elements allocated from the heap in the buffer array
     below.
     */
    DWORD NumberAllocatedFromSystem;

    /**
     The number of elements from the buffer array below that have been used.
     */
    DWORD NumberAllocatedToCaller;

    /**
     An array of elements.
     */
    PVOID Buffer;
} MAKE_SLAB_ALLOC, *PMAKE_SLAB_ALLOC;

/**
 A record of the exitcode of a preprocessor command.  These can be recorded
 to save time on a subsequent compilation.
 */
typedef struct _MAKE_PREPROC_EXEC_CACHE_ENTRY {

    /**
     The hash entry of the preprocessor command.  This includes the command
     string as well as a hash of the environment and variable state so that
     if a different compilation environment is used, the cache will not
     match.
     */
    YORI_HASH_ENTRY HashEntry;

    /**
     A list of all entries to facilitate efficient teardown.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The exit code of the preprocessor command.
     */
    DWORD ExitCode;

} MAKE_PREPROC_EXEC_CACHE_ENTRY, *PMAKE_PREPROC_EXEC_CACHE_ENTRY;

/**
 Context describing a scope.  In this program a scope generally refers to
 a single makefile, although note that one makefile can include others
 without changing scope.  Scopes are when one makefile target depends on
 targets in a child makefile.
 */
typedef struct _MAKE_SCOPE_CONTEXT {

    /**
     A reference count for this scope.  It is deallocated when it has
     zero references.
     */
    DWORD ReferenceCount;

    /**
     Points to the parent scope.  Variables are inherited from all parent
     scopes but changes can only be made to the child most scope.
     */
    struct _MAKE_SCOPE_CONTEXT *ParentScope;

    /**
     Points to the previously activated scope.  Reactivated when this scope
     is deactivated.
     */
    struct _MAKE_SCOPE_CONTEXT *PreviousScope;

    /**
     Pointer to the process global make context.  This is here so we don't
     need to pass both contexts to many functions.
     */
    struct _MAKE_CONTEXT *MakeContext;

    /**
     The hash entry for the scope.  Paired with MAKE_CONTEXT::Scopes.  The key
     is the directory being treated as the current operating directory.  This
     refers to the path that contains the original makefile being built,
     without being updated for includes.
     */
    YORI_HASH_ENTRY HashEntry;

    /**
     The list entry for the scope to facilitate easy cleanup.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The directory being treated as the current include directory.  This
     refers to the path that contains the makefile being parsed, and is
     updated when a makefile includes a child to refer to the location of
     the child.
     */
    YORI_STRING CurrentIncludeDirectory;

    /**
     A hash table of known variables, used for regular lookup during
     line evaluation.
     */
    PYORI_HASH_TABLE Variables;

    /**
     A list of known variables, used to facilitate bulk delete.
     */
    YORI_LIST_ENTRY VariableList;

    /**
     A list of known inference rules.
     */
    YORI_LIST_ENTRY InferenceRuleList;

    /**
     A list of targets that have not had inference rules located yet.
     This is paired with the scope context so that targets accessed by a
     scope will attempt to resolve inference rules on scope exit.
     */
    YORI_LIST_ENTRY InferenceRuleNeededList;

    /**
     The current preprocessor conditional nesting level (number of nested if
     statements.)  This needs to be tracked so we know when it ends.  It's
     possible that none of the rest of the statements are being evaluated.
     */
    DWORD CurrentConditionalNestingLevel;

    /**
     The active preprocessor conditional nesting level (the innermost nested
     if where all parent tests are true so expressions should be executed.)
     */
    DWORD ActiveConditionalNestingLevel;

    /**
     If a rule starts on a nesting level and at that time execution should
     not occur, it is tracked for parsing purposes but is deemed to be
     terminated when the nesting level moves upwards (!ENDIF) or when an
     !ELSE type block occurs.
     */
    DWORD RuleExcludedOnNestingLevel;

    /**
     TRUE if the condition at the ActiveConditionalNestingLevel hsa been found
     to be true so statements at that level should be executed.  FALSE if the
     statements at that level should not be executed (ie., we are in an else
     clause or the condition evaluated to false such that statements should
     not currently execute at this level.
     */
    BOOLEAN ActiveConditionalNestingLevelExecutionEnabled;

    /**
     TRUE if any previous branch in the ActiveConditionalNestingLevel has been
     found to be TRUE such that any else if clause should not be evaluated and
     any else clause should not execute.
     */
    BOOLEAN ActiveConditionalNestingLevelExecutionOccurred;

    /**
     TRUE if a recipe is currently active.  This is typically cleared when a
     blank line is found, but can be cleared by preprocessor directives too.
     */
    BOOLEAN RecipeActive;

} MAKE_SCOPE_CONTEXT, *PMAKE_SCOPE_CONTEXT;


/**
 A list of variable precedences.  A variable defined via a higher precedence
 cannot be overridden by a lower precedence.
 */
typedef enum _MAKE_VARIABLE_PRECEDENCE {
    MakeVariablePrecedencePredefined = 0,
    MakeVariablePrecedenceMakefile = 1,
    MakeVariablePrecedenceCommandLine = 2
} MAKE_VARIABLE_PRECEDENCE;


/**
 Structure describing a variable within a makefile.  This structure is
 followed in memory by the variable name, and is followed by the initial
 value of the variable.  Note that the value can change however, so the
 current value is found via the Value member within the structure.
 */
typedef struct _MAKE_VARIABLE {

    /**
     The hash entry for the variable.  Paired with
     MAKE_SCOPE_CONTEXT::Variables.
     */
    YORI_HASH_ENTRY HashEntry;

    /**
     The list entry for the variable.  Paired with
     MAKE_SCOPE_CONTEXT::VariableList.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The precedence level of this variable's contents.
     */
    MAKE_VARIABLE_PRECEDENCE Precedence;

    /**
     Set to TRUE if the variable should be considered undefined.  !IFDEF will
     match on empty values, and a variable can be undefined only via !UNDEF.
     The obvious way to do this is to remove the variable from the hash table;
     having a value here is done so we can support nested scopes where an
     !UNDEF can override a previous definition without discarding the original
     scoped definition.
     */
    BOOL Undefined;

    /**
     The current value of the variable.
     */
    YORI_STRING Value;

} MAKE_VARIABLE, *PMAKE_VARIABLE;

/**
 A fallback rule to transform files with one extension into another
 extension, often implying compilation.
 */
typedef struct _MAKE_INFERENCE_RULE {

    /**
     The list of active inference rules.  Everything on this list will be
     traversed when trying to find a match.  This is paired with
     MAKE_SCOPE_CONTEXT::InferenceRuleList.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     Pointer to the ScopeContext owning this inference rule.  Note this is
     not referenced.
     */
    PMAKE_SCOPE_CONTEXT ScopeContext;

    /**
     A reference count of the number of targets which could use this rule if
     needed.  A reference is also taken if it is active.
     */
    DWORD ReferenceCount;

    /**
     The source extension of the rule.
     */
    YORI_STRING SourceExtension;

    /**
     The target extension of the rule.
     */
    YORI_STRING TargetExtension;

    /**
     The generic psuedo-target for this inference rule.
     */
    struct _MAKE_TARGET *Target;

} MAKE_INFERENCE_RULE, *PMAKE_INFERENCE_RULE;

/**
 A dependency structure to describe the relationship between an object that
 requires something to be built first and the object being built first and
 those that can be built afterwards.
 */
typedef struct _MAKE_TARGET_DEPENDENCY {

    /**
     The target that must be built before the child.
     */
    struct _MAKE_TARGET *Parent;

    /**
     A list of all children that are enabled by building the parent.  That is,
     this list should contain one MAKE_TARGET, being the same parent pointed
     to by Parent above, and many MAKE_TARGET_DEPENDENCY children.
     */
    YORI_LIST_ENTRY ParentDependents;

    /**
     The child that can be built after the parent.
     */
    struct _MAKE_TARGET *Child;

    /**
     A list of all parents that are required by building the child.  That is,
     this list should contain one MAKE_TARGET, being the same child pointed to
     by Child above, and many MAKE_TARGET_DEPENDENCY structures describing all
     the parents to build first.
     */
    YORI_LIST_ENTRY ChildDependents;

} MAKE_TARGET_DEPENDENCY, *PMAKE_TARGET_DEPENDENCY;

/**
 A single command to execute as part of building a target.

 MSFIX At some future point, certain commands could be evaluated by this
 program internally.  Notably, this may include commands like cd or set.
 In order to implement these, each target needs its own notion of current
 directory or environment.  CreateProcess allows these to be specified
 explicitly for any process not builtin.

 It might also make sense to hack in echo just for performance.
 */
typedef struct _MAKE_CMD_TO_EXEC {

    /**
     The linkage between all of the commands for the target.  Paired with
     MAKE_TARGET::ExecCmds.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     Normally each command should be displayed when it is executed.  If the
     command is prefixed with @, this display is suppressed.
     */
    BOOLEAN DisplayCmd;

    /**
     If the user prefixes a command with -, then any failures executing it
     should be ignored and the next command or target should be evaluated.
     */
    BOOLEAN IgnoreErrors;

    /**
     The command string to execute.
     */
    YORI_STRING Cmd;
} MAKE_CMD_TO_EXEC, *PMAKE_CMD_TO_EXEC;

/**
 Information describing a make target.  Note that a target is something that
 we might want to build, or may not be part of the current build process, or
 may just refer to a static source file that will never be built, but we
 need the parent/child relationship to compare timestamps.
 */
typedef struct _MAKE_TARGET {

    /**
     A referenced pointer to the scope that this target was defined in.
     This is only meaningful if a rule is defined to build this target
     or if an inference rule is found for it.
     */
    PMAKE_SCOPE_CONTEXT ScopeContext;

    /**
     The hash entry.  Key is fully qualified path name.
     */
    YORI_HASH_ENTRY HashEntry;

    /**
     A list of MAKE_TARGET_DEPENDENCY objects that this target depends upon.
     Paired with MAKE_TARGET_DEPENDENCY::ChildDependents .
     */
    YORI_LIST_ENTRY ParentDependents;

    /**
     A list of MAKE_TARGET_DEPENDENCY objects that depend on this target.
     Paired with MAKE_TARGET_DEPENDENCY::ParentDependents.
     */
    YORI_LIST_ENTRY ChildDependents;

    /**
     A list of targets that must be rebuilt.  Note that many targets will
     not be built because the user did not request the target to be built,
     or because it is already current.
     */
    YORI_LIST_ENTRY RebuildList;

    /**
     A list of all known targets to facilitate cleanup.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     A list of targets that have not had inference rules located yet.
     This is paired with the scope context so that targets accessed by a
     scope will attempt to resolve inference rules on scope exit.
     */
    YORI_LIST_ENTRY InferenceRuleNeededList;

    /**
     The number of references.  Normally each target will have one reference
     so long as it is discoverable, but it might have a second reference if
     an inference rule refers to it.
     */
    DWORD ReferenceCount;

    /**
     The number of parents that need to be built before this target can
     be built.  If zero, this target can be built now.
     */
    DWORD NumberParentsToBuild;

    /**
     TRUE if an explicit recipe has been found.  That line may list
     dependencies only (ie., the recipe may have no tasks to perform) but
     it means the inference rule should not be used.  Note that the only
     way for this target to depend on other targets is if a recipe has been
     found.
     */
    BOOLEAN ExplicitRecipeFound;

    /**
     TRUE if this target has had its recipe executed.  This doesn't mean
     that a file necessarily exists, but nothing else needs to wait for it.
     */
    BOOLEAN Executed;

    /**
     TRUE if the file is known to exist, FALSE if it does not.
     */
    BOOLEAN FileExists;

    /**
     TRUE if dependencies have been evaluated and don't need to be evaluated
     again.
     */
    BOOLEAN DependenciesEvaluated;

    /**
     TRUE if the commands to execute require a shell.  This occurs if the
     commands include any redirection operator, or there are multiple
     commands present.
     */
    BOOLEAN ExecuteViaShell;

    /**
     TRUE if this target needs to be rebuilt.
     */
    BOOLEAN RebuildRequired;

    /**
     TRUE if this target isn't a real target, but a pretend target that
     contains a recipe and other state to implement an inference rule.
     FALSE if this target is for a real target, which still may or may
     not be a file.
     */
    BOOLEAN InferenceRulePseudoTarget;

    /**
     The timestamp of the file.  This is only meaningful if FileExists is
     TRUE.
     */
    LARGE_INTEGER ModifiedTime;

    /**
     Pointer to the best matching inference rule in effect at the time the
     target was referenced.  This may be superseded by a later explicit
     recipe, but if another target depends on this one and no explicit
     recipe is provided, the inference rule can be used as a fallback.
     */
    PMAKE_INFERENCE_RULE InferenceRule;

    /**
     Pointer to the target that was found as the source of the inference
     rule.  This can be promoted to a parent target if the inference rule is
     used.
     */
    struct _MAKE_TARGET *InferenceRuleParentTarget;

    /**
     The recipe to execute to construct this specific target.  Note this may
     be multi-line, and may contain unexpanded variables that are specific
     to the target.
     */
    YORI_STRING Recipe;

    /**
     The set of commands to execute to construct this target.  Paired with
     MAKE_CMD_TO_EXEC::ListEntry .
     */
    YORI_LIST_ENTRY ExecCmds;

} MAKE_TARGET, *PMAKE_TARGET;

/**
 Current state of the operation.
 */
typedef struct _MAKE_CONTEXT {

    /**
     Pointer to the currently active scope.
     */
    PMAKE_SCOPE_CONTEXT ActiveScope;

    /**
     Pointer to the initial scope of the first makefile to execute.
     */
    PMAKE_SCOPE_CONTEXT RootScope;

    /**
     An allocator used to preallocate and suballocate target structures.
     */
    MAKE_SLAB_ALLOC TargetAllocator;

    /**
     An allocator used to preallocate and suballocate dependency structures.
     */
    MAKE_SLAB_ALLOC DependencyAllocator;

    /**
     A hash table of scopes whose key is their directory.
     */
    PYORI_HASH_TABLE Scopes;

    /**
     A list of known scopes, used to facilitate bulk delete.
     */
    YORI_LIST_ENTRY ScopesList;

    /**
     A hash table of known targets, meaning both objects that the makefile
     knows how to construct as well as objects that the makefile describes
     as dependencies even if they are assumed to already exist (source files.)
     The key of this hash table is fully qualified path.
     */
    PYORI_HASH_TABLE Targets;

    /**
     A list of known targets, used to facilitate bulk delete.
     */
    YORI_LIST_ENTRY TargetsList;

    /**
     A list of targets that have finished being built.
     */
    YORI_LIST_ENTRY TargetsFinished;

    /**
     A list of targets that are currently being built.
     */
    YORI_LIST_ENTRY TargetsRunning;

    /**
     A list of targets which can be built when there is a processor to
     build them.
     */
    YORI_LIST_ENTRY TargetsReady;

    /**
     A list of targets which need to be built, but cannot be built now due
     to not having dependencies satisfied.
     */
    YORI_LIST_ENTRY TargetsWaiting;

    /**
     A hash table of cached preprocessor commands.
     */
    PYORI_HASH_TABLE PreprocessorCache;

    /**
     A list of known preprocessor scope entries, used to facilitate bulk delete.
     */
    YORI_LIST_ENTRY PreprocessorCacheList;

    /**
     An allocation used to generate files to look for when determining which
     inference rules to apply.  Because this is very temporary, it is only
     used in one place, but is retained to avoid repeated allocations.
     */
    YORI_STRING FileToProbe;

    /**
     The temporary path applied to the ymake process.  Child processes are
     assigned subdirectories under this path to use for their own temporary
     file needs.
     */
    YORI_STRING TempPath;

    /**
     A bitmask representing which job IDs have been allocated.
     */
    DWORDLONG JobIdsAllocated;

    /**
     A bitmask representing temporary directories which have been created.
     */
    DWORDLONG TempDirectoriesCreated;

    /**
     The time taken to execute processes as part of preprocessor commands.
     */
    DWORDLONG TimeInPreprocessorCreateProcess;

    /**
     The time in the preprocessor outside of executing processes.
     */
    DWORDLONG TimeInPreprocessor;

    /**
     The time spent to calculate the dependency graph.
     */
    DWORDLONG TimeBuildingGraph;

    /**
     The time spent executing the graph.
     */
    DWORDLONG TimeInExecute;

    /**
     The time spent cleaning up state, mainly freeing memory.
     */
    DWORDLONG TimeInCleanup;

    /**
     The number of inference rule allocations.
     */
    DWORD AllocInferenceRule;

    /**
     The number of target allocations.
     */
    DWORD AllocTarget;

    /**
     The number of dependency allocations.
     */
    DWORD AllocDependency;

    /**
     The number of variable allocations.
     */
    DWORD AllocVariable;

    /**
     The number of target variable data allocations.
     */
    DWORD AllocVariableData;

    /**
     The number of expanded line allocations.
     */
    DWORD AllocExpandedLine;

    /**
     The number of child processes to execute concurrently.  This defaults
     to the number of logical processors, but is limited to 64 due to
     WaitForMultipleObjects.
     */
    DWORD NumberProcesses;

    /**
     The 32 bit hash of the environment block. This process does not modify
     its own environment, so this can be calculated once for the lifetime
     of the process.  Its purpose is to distinguish preprocessor commands
     executed after the environment has changed.
     */
    DWORD EnvHash;

    /**
     TRUE if an error has been encountered that should cause further
     processing to stop.
     */
    BOOLEAN ErrorTermination;

    /**
     TRUE to display time spent at various points on exit.
     */
    BOOLEAN PerfDisplay;

    /**
     TRUE to indicate that EnvHash above has been calculated.
     */
    BOOLEAN EnvHashCalculated;

    /**
     TRUE to indicate that execution should continue after failure as much
     as possible.
     */
    BOOLEAN KeepGoing;

} MAKE_CONTEXT, *PMAKE_CONTEXT;

// *** ALLOC.C ***

PVOID
MakeSlabAlloc(
    __in PMAKE_SLAB_ALLOC Alloc,
    __in DWORD SizeInBytes
    );

VOID
MakeSlabFree(
    __in PVOID Ptr
    );

VOID
MakeSlabCleanup(
    __in PMAKE_SLAB_ALLOC Alloc
    );


// *** VAR.C ***

BOOLEAN
MakeIsVariableDefined(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Variable
    );

BOOLEAN
MakeSetVariable(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PCYORI_STRING Variable,
    __in_opt PCYORI_STRING Value,
    __in BOOLEAN Defined,
    __in MAKE_VARIABLE_PRECEDENCE Precedence
    );

BOOLEAN
MakeExpandVariables(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in_opt PMAKE_TARGET Target,
    __inout PYORI_STRING ExpandedLine,
    __in PYORI_STRING Line
    );

VOID
MakeDeleteAllVariables(
    __inout PMAKE_SCOPE_CONTEXT ScopeContext
    );

DWORD
MakeHashAllVariables(
    __inout PMAKE_SCOPE_CONTEXT ScopeContext
    );

BOOLEAN
MakeExecuteSetVariable(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Line
    );

// *** MINISH.C ***

DWORD
MakeShExecuteInProc(
    __in PYORI_CMD_BUILTIN Fn,
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    );

DWORD
MakeShExecExecPlan(
    __in PYORI_LIBSH_EXEC_PLAN ExecPlan,
    __out_opt PVOID * OutputBuffer
    );

// *** PREPROC.C ***

VOID
MakeTrimWhitespace(
    __in PYORI_STRING String
    );

__success(return)
BOOLEAN
MakeFindMakefileInDirectory(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __out PYORI_STRING FileName
    );

BOOLEAN
MakeCreateRuleDependency(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET ChildTarget,
    __in PYORI_STRING ParentDependency
    );

BOOL
MakeProcessStream(
    __in HANDLE hSource,
    __in PMAKE_CONTEXT MakeContext
    );

VOID
MakeLoadPreprocessorCacheEntries(
    __inout PMAKE_CONTEXT MakeContext,
    __in PYORI_STRING MakeFileName
    );

VOID
MakeSaveAndDeleteAllPreprocessorCacheEntries(
    __inout PMAKE_CONTEXT MakeContext,
    __in PYORI_STRING MakeFileName
    );

// *** SCOPE.C ***

PMAKE_SCOPE_CONTEXT
MakeAllocateNewScope(
    __in PMAKE_CONTEXT MakeContext,
    __in PYORI_STRING Directory
    );

VOID
MakeReferenceScope(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    );

VOID
MakeDereferenceScope(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    );

__success(return)
BOOLEAN
MakeActivateScope(
    __in PMAKE_CONTEXT MakeContext,
    __in PYORI_STRING DirName,
    __out PBOOLEAN FoundExisting
    );

VOID
MakeDeactivateScope(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    );

VOID
MakeDeleteAllScopes(
    __inout PMAKE_CONTEXT MakeContext
    );

// *** TARGET.C ***

VOID
MakeDeleteAllTargets(
    __inout PMAKE_CONTEXT MakeContext
    );

VOID
MakeDeactivateAllInferenceRules(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    );

PMAKE_TARGET
MakeLookupOrCreateTarget(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING TargetName
    );

PMAKE_INFERENCE_RULE
MakeCreateInferenceRule(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING SourceExt,
    __in PYORI_STRING TargetExt,
    __in PMAKE_TARGET Target
    );

VOID
MakeMarkTargetInferenceRuleNeededIfNeeded(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PMAKE_TARGET Target
    );

BOOLEAN
MakeFindInferenceRulesForScope(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    );

BOOLEAN
MakeCreateParentChildDependency(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET Parent,
    __in PMAKE_TARGET Child
    );

BOOLEAN
MakeDetermineDependencies(
    __in PMAKE_CONTEXT MakeContext
    );

__success(return)
BOOLEAN
MakeExpandTargetVariable(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET Target,
    __in PCYORI_STRING VariableName,
    __out PYORI_STRING VariableData
    );

// *** EXEC.C ***

VOID
MakeCleanupTemporaryDirectories(
    __in PMAKE_CONTEXT MakeContext
    );

BOOLEAN
MakeExecuteRequiredTargets(
    __in PMAKE_CONTEXT MakeContext
    );
