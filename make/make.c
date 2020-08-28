/**
 * @file make/make.c
 *
 * Yori shell make program
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
#include "make.h"

/**
 Help text to display to the user.
 */
const
CHAR strMakeHelpText[] =
        "\n"
        "Execute makefiles.\n"
        "\n"
        "YMAKE [-license] [-f file] [-j n] [var=value] [target]\n"
        "\n"
        "   --             Treat all further arguments as display parameters\n"
        "   -f             Name of the makefile to use, default YMkFile or Makefile\n"
        "   -j             The number of child processes, default number of processors+1\n";


/**
 Display usage text to the user.
 */
BOOL
MakeHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("YMake %i.%02i\n"), MAKE_VER_MAJOR, MAKE_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strMakeHelpText);
    return TRUE;
}

/**
 A structure describing a string-to-string map of default macro names to
 default macro values.  This can be overridden later.
 */
typedef struct _MAKE_DEFAULT_MACRO {

    /**
     The default macro name.
     */
    YORI_STRING Variable;

    /**
     The default macro value.
     */
    YORI_STRING Value;
} MAKE_DEFAULT_MACRO;

/**
 An array of default macros to initialize on program entry.
 */
CONST MAKE_DEFAULT_MACRO MakeDefaultMacros[] = {
    { YORILIB_CONSTANT_STRING(_T("AS")),  YORILIB_CONSTANT_STRING(_T("ml")) },
    { YORILIB_CONSTANT_STRING(_T("CC")),  YORILIB_CONSTANT_STRING(_T("cl")) },
    { YORILIB_CONSTANT_STRING(_T("CPP")), YORILIB_CONSTANT_STRING(_T("cl")) },
    { YORILIB_CONSTANT_STRING(_T("CXX")), YORILIB_CONSTANT_STRING(_T("cl")) },
    { YORILIB_CONSTANT_STRING(_T("RC")),  YORILIB_CONSTANT_STRING(_T("rc")) }
};

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the ymake builtin command.
 */
#define ENTRYPOINT YoriCmd_YMAKE
#else
/**
 The main entrypoint for the ymake standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the ymake cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero on success, nonzero on failure.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    MAKE_CONTEXT MakeContext;
    PYORI_STRING FileName;
    PMAKE_TARGET RootTarget;
    YORI_STRING FullFileName;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    HANDLE hStream;
    YORI_STRING Arg;
    YORI_STRING RootDir;
    LONGLONG llTemp;
    DWORD Result;
    DWORD CharsConsumed;

    FileName = NULL;
    RootTarget = NULL;
    ZeroMemory(&MakeContext, sizeof(MakeContext));
    YoriLibInitializeListHead(&MakeContext.ScopesList);
    YoriLibInitializeListHead(&MakeContext.TargetsList);
    YoriLibInitializeListHead(&MakeContext.TargetsFinished);
    YoriLibInitializeListHead(&MakeContext.TargetsRunning);
    YoriLibInitializeListHead(&MakeContext.TargetsReady);
    YoriLibInitializeListHead(&MakeContext.TargetsWaiting);

    MakeContext.Scopes = YoriLibAllocateHashTable(1000);
    if (MakeContext.Scopes == NULL) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    YoriLibInitEmptyString(&RootDir);
    YoriLibConstantString(&Arg, _T("."));
    if (!YoriLibGetFullPathNameReturnAllocation(&Arg, FALSE, &RootDir, NULL)) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    MakeContext.RootScope = MakeAllocateNewScope(&MakeContext, &RootDir);
    YoriLibFreeStringContents(&RootDir);
    if (MakeContext.RootScope == NULL) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    MakeContext.RootScope->ActiveConditionalNestingLevelExecutionEnabled = TRUE;

    MakeContext.Targets = YoriLibAllocateHashTable(4000);
    if (MakeContext.Targets == NULL) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    for (i = 0; i < sizeof(MakeDefaultMacros)/sizeof(MakeDefaultMacros[0]); i++) {
        MakeSetVariable(MakeContext.RootScope, &MakeDefaultMacros[i].Variable, &MakeDefaultMacros[i].Value, TRUE, MakeVariablePrecedencePredefined);
    }

    {
        YORI_STRING VariableName;
        YORI_STRING YMakeVer;
        YoriLibInitEmptyString(&YMakeVer);
        YoriLibYPrintf(&YMakeVer, _T("%03i%03i"), MAKE_VER_MAJOR, MAKE_VER_MINOR);
        YoriLibConstantString(&VariableName, _T("_YMAKE_VER"));
        MakeSetVariable(MakeContext.RootScope, &VariableName, &YMakeVer, TRUE, MakeVariablePrecedencePredefined);
        YoriLibFreeStringContents(&YMakeVer);
    }

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                MakeHelp();
                Result = EXIT_SUCCESS;
                goto Cleanup;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2020"));
                Result = EXIT_SUCCESS;
                goto Cleanup;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (i + 1 < ArgC) {
                    FileName = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("j")) == 0) {
                if (i + 1 < ArgC) {
                    if (YoriLibStringToNumber(&ArgV[i + 1], FALSE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
                        MakeContext.NumberProcesses = (DWORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("perf")) == 0) {
                MakeContext.PerfDisplay = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i + 1;
                ArgumentUnderstood = TRUE;
                break;
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

    //
    //  If all command line arguments are options, start looking for
    //  variables and targets after that (ie., don't look.)
    //

    if (StartArg == 0) {
        StartArg = ArgC;
    }

    //
    //  If -j isn't specified, set a default number of jobs to execute as
    //  the number of logical processors plus one.
    //

    if (MakeContext.NumberProcesses == 0) {
        SYSTEM_INFO SysInfo;
        GetSystemInfo(&SysInfo);
        MakeContext.NumberProcesses = SysInfo.dwNumberOfProcessors + 1;
    }

    //
    //  WaitForMultipleObjects has a limit of 64 things to wait for, so this
    //  program can't currently have more than 64 children.
    //

    if (MakeContext.NumberProcesses > 64) {
        MakeContext.NumberProcesses = 64;
    }

    MakeContext.ActiveScope = MakeContext.RootScope;

    //
    //  Loop through the arguments again, finding any variable definitions or
    //  targets, and apply those now.
    //

    for (i = StartArg; i < ArgC; i++) {
        PYORI_STRING ThisArg;
        LPTSTR Equals;

        ThisArg = &ArgV[i];

        Equals = YoriLibFindLeftMostCharacter(ThisArg, '=');
        if (Equals != NULL) {
            YORI_STRING VariableName;
            YORI_STRING Value;

            YoriLibInitEmptyString(&VariableName);
            YoriLibInitEmptyString(&Value);
            VariableName.StartOfString = ThisArg->StartOfString;
            VariableName.LengthInChars = (DWORD)(Equals - VariableName.StartOfString);
            Value.StartOfString = Equals + 1;
            Value.LengthInChars = ThisArg->LengthInChars - VariableName.LengthInChars - 1;

            MakeSetVariable(MakeContext.RootScope, &VariableName, &Value, TRUE, MakeVariablePrecedenceCommandLine);
        } else {
            YORI_STRING EmptyString;
            YoriLibInitEmptyString(&EmptyString);

            //
            //  If the user specified any explicit targets, create a dummy
            //  root target.  This has no name to ensure that it cannot be
            //  specified within the make file, and is created first so
            //  it will be the first thing for execution to find.
            //
            //  MSFIX There's currently no way to indicate that everything
            //  from the first parent dependency is done before the second
            //  parent dependency starts, so there's no way to express
            //  "clean then all."
            //

            if (RootTarget == NULL) {
                RootTarget = MakeLookupOrCreateTarget(MakeContext.RootScope, &EmptyString);
                if (RootTarget == NULL) {
                    Result = EXIT_FAILURE;
                    goto Cleanup;
                }
                RootTarget->ExplicitRecipeFound = TRUE;
                MakeReferenceScope(MakeContext.RootScope);
                RootTarget->ScopeContext = MakeContext.RootScope;
            }

            if (!MakeCreateRuleDependency(&MakeContext, RootTarget, ThisArg)) {
                Result = EXIT_FAILURE;
                goto Cleanup;
            }
        }
    }

#if YORI_BUILTIN
    YoriLibCancelEnable();
#endif

    if (FileName == NULL) {
        if (!MakeFindMakefileInDirectory(MakeContext.RootScope, &FullFileName)) {
            Result = EXIT_FAILURE;
            goto Cleanup;
        }
    } else {
        if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullFileName)) {
            Result = EXIT_FAILURE;
            goto Cleanup;
        }
    }
    hStream = CreateFile(FullFileName.StartOfString, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (hStream == INVALID_HANDLE_VALUE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No makefile found\n"));
        YoriLibFreeStringContents(&FullFileName);
        Result = EXIT_FAILURE;
        goto Cleanup;
    }
    YoriLibFreeStringContents(&FullFileName);

    QueryPerformanceCounter(&StartTime);
    MakeProcessStream(hStream, &MakeContext);
    QueryPerformanceCounter(&EndTime);

    MakeContext.TimeInPreprocessor = EndTime.QuadPart - StartTime.QuadPart;

    CloseHandle(hStream);

    if (MakeContext.ErrorTermination) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    MakeFindInferenceRulesForScope(MakeContext.RootScope);

    QueryPerformanceCounter(&StartTime);
    if (!MakeDetermineDependencies(&MakeContext)) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }
    QueryPerformanceCounter(&EndTime);
    MakeContext.TimeBuildingGraph = EndTime.QuadPart - StartTime.QuadPart;

    StartTime.QuadPart = EndTime.QuadPart;
    if (!MakeExecuteRequiredTargets(&MakeContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Failed to build targets.\n"));
        Result = EXIT_FAILURE;
        goto Cleanup;
    }
    QueryPerformanceCounter(&EndTime);
    MakeContext.TimeInExecute = EndTime.QuadPart - StartTime.QuadPart;


    Result = EXIT_SUCCESS;

Cleanup:

    QueryPerformanceCounter(&StartTime);

    ASSERT(MakeContext.ActiveScope == MakeContext.RootScope ||
           MakeContext.ActiveScope == NULL);
    if (MakeContext.RootScope != NULL) {
        MakeDeactivateScope(MakeContext.RootScope);
        MakeContext.RootScope = NULL;
    }

    MakeSlabCleanup(&MakeContext.TargetAllocator);
    MakeSlabCleanup(&MakeContext.DependencyAllocator);

    MakeDeleteAllTargets(&MakeContext);

    if (MakeContext.Targets != NULL) {
        YoriLibFreeEmptyHashTable(MakeContext.Targets);
    }

    MakeDeleteAllScopes(&MakeContext);

    YoriLibFreeStringContents(&MakeContext.FileToProbe);

    if (MakeContext.ErrorTermination) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Parse error!!\n"));
        Result = EXIT_FAILURE;
    }

    QueryPerformanceCounter(&EndTime);
    MakeContext.TimeInCleanup = EndTime.QuadPart - StartTime.QuadPart;

    if (MakeContext.PerfDisplay && Result == EXIT_SUCCESS) {
        LARGE_INTEGER Frequency;
        QueryPerformanceFrequency(&Frequency);
        MakeContext.TimeInPreprocessor = MakeContext.TimeInPreprocessor - MakeContext.TimeInPreprocessorCreateProcess;

        MakeContext.TimeInPreprocessorCreateProcess = MakeContext.TimeInPreprocessorCreateProcess * 1000 / Frequency.QuadPart;
        MakeContext.TimeInPreprocessor = MakeContext.TimeInPreprocessor * 1000 / Frequency.QuadPart;
        MakeContext.TimeBuildingGraph = MakeContext.TimeBuildingGraph * 1000 / Frequency.QuadPart;
        MakeContext.TimeInExecute = MakeContext.TimeInExecute * 1000 / Frequency.QuadPart;
        MakeContext.TimeInCleanup = MakeContext.TimeInCleanup * 1000 / Frequency.QuadPart;
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("\n"));
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Time in preprocessor child processes: %lli ms\n"), MakeContext.TimeInPreprocessorCreateProcess);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Time in preprocessor: %lli ms\n"), MakeContext.TimeInPreprocessor);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Time building graph: %lli ms\n"), MakeContext.TimeBuildingGraph);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Time executing commands: %lli ms\n"), MakeContext.TimeInExecute);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Time cleaning up: %lli ms\n"), MakeContext.TimeInCleanup);

#if MAKE_DEBUG_PERF
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Number dependency allocs: %i\n"), MakeContext.AllocDependency);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Number inference rule allocs: %i\n"), MakeContext.AllocInferenceRule);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Number target allocs: %i\n"), MakeContext.AllocTarget);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Number variable allocs: %i\n"), MakeContext.AllocVariable);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Number variable data allocs: %i\n"), MakeContext.AllocVariableData);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Number expanded line allocs: %i\n"), MakeContext.AllocExpandedLine);
#endif
    }

    return Result;
}

// vim:sw=4:ts=4:et:
