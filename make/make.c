/**
 * @file make/make.c
 *
 * Yori shell make program
 *
 * Copyright (c) 2020-2021 Malcolm J. Smith
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
 Help text to display to the user.
 */
const
CHAR strMakeHelpText[] =
        "\n"
        "Execute makefiles.\n"
        "\n"
        "YMAKE [-license] [-f file] [-j n] [-m] [-perf] [-pru] [-s] [var=value] [target]\n"
        "\n"
        "   --             Treat all further arguments as display parameters\n"
        "   -f             Name of the makefile to use, default YMkFile or Makefile\n"
        "   -j             The number of child processes, default number of processors+1\n"
        "   -k             Keep executing jobs after errors\n"
        "   -m             Perform tasks at low priority\n"
        "   -mm            Perform tasks at very low priority\n"
        "   -perf          Display how much time was spent in each phase of processing\n"
        "   -pru           Keep a cache of preprocessor recently executed results\n"
        "   -s             Silently launch child processes\n";


/**
 Display usage text to the user.
 */
BOOL
MakeHelp(VOID)
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
 An array of options which have a parameter following them.  This is used
 to skip that extra parameter when parsing variables or targets.
 */
CONST YORI_STRING MakeArgsWithParameter[] = {
    YORILIB_CONSTANT_STRING(_T("f")),
    YORILIB_CONSTANT_STRING(_T("j"))
};

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

/**
 A structure defining a mapping between a command name and a function to
 execute.  This is used to populate builtin commands.
 */
typedef struct _MAKE_BUILTIN_NAME_MAPPING {

    /**
     The command name.
     */
    LPTSTR CommandName;

    /**
     Pointer to the function to execute.
     */
    PYORI_CMD_BUILTIN BuiltinFn;
} MAKE_BUILTIN_NAME_MAPPING, *PMAKE_BUILTIN_NAME_MAPPING;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YECHO;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YMKDIR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YRMDIR;

/**
 The list of builtin commands supported by this build of Yori.
 */
CONST MAKE_BUILTIN_NAME_MAPPING
MakeBuiltinCmds[] = {
    {_T("ECHO"),      YoriCmd_YECHO},
    {_T("MKDIR"),     YoriCmd_YMKDIR},
    {_T("RMDIR"),     YoriCmd_YRMDIR},
    {NULL,            NULL}
};

/**
 The set of priorities that make and its child processes can execute at.
 */
typedef enum _MAKE_PRIORITY {
    MakePriorityNormal = 0,
    MakePriorityLow = 1,
    MakePriorityVeryLow = 2
} MAKE_PRIORITY;


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
    DWORD i, j;
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
    MAKE_PRIORITY Priority;
    BOOLEAN ExplicitTargetFound;

    FileName = NULL;
    RootTarget = NULL;
    ZeroMemory(&MakeContext, sizeof(MakeContext));
    YoriLibInitializeListHead(&MakeContext.ScopesList);
    YoriLibInitializeListHead(&MakeContext.InlineFileList);
    YoriLibInitializeListHead(&MakeContext.TargetsList);
    YoriLibInitializeListHead(&MakeContext.TargetsFinished);
    YoriLibInitializeListHead(&MakeContext.TargetsRunning);
    YoriLibInitializeListHead(&MakeContext.TargetsReady);
    YoriLibInitializeListHead(&MakeContext.TargetsWaiting);
    YoriLibInitializeListHead(&MakeContext.PreprocessorCacheList);
    YoriLibInitEmptyString(&FullFileName);
    Priority = MakePriorityNormal;
    ExplicitTargetFound = FALSE;

    {
        MAKE_BUILTIN_NAME_MAPPING CONST *BuiltinNameMapping = MakeBuiltinCmds;

        while (BuiltinNameMapping->CommandName != NULL) {
            YORI_STRING YsCommandName;

            YoriLibConstantString(&YsCommandName, BuiltinNameMapping->CommandName);
            if (!YoriLibShBuiltinRegister(&YsCommandName, BuiltinNameMapping->BuiltinFn)) {
                Result = EXIT_FAILURE;
                goto Cleanup;
            }
            BuiltinNameMapping++;
        }
    }

    CharsConsumed = GetCurrentDirectory(0, NULL);
    if (CharsConsumed == 0) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    if (!YoriLibAllocateString(&MakeContext.ProcessCurrentDirectory, CharsConsumed)) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    MakeContext.ProcessCurrentDirectory.LengthInChars = GetCurrentDirectory(MakeContext.ProcessCurrentDirectory.LengthAllocated, MakeContext.ProcessCurrentDirectory.StartOfString);
    if (MakeContext.ProcessCurrentDirectory.LengthInChars == 0) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    ASSERT(YoriLibIsStringNullTerminated(&MakeContext.ProcessCurrentDirectory));

    if (!YoriLibGetTempPath(&MakeContext.TempPath, 0)) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    MakeContext.Scopes = YoriLibAllocateHashTable(1000);
    if (MakeContext.Scopes == NULL) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    MakeContext.Targets = YoriLibAllocateHashTable(4000);
    if (MakeContext.Targets == NULL) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                MakeHelp();
                Result = EXIT_SUCCESS;
                goto Cleanup;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2021"));
                Result = EXIT_SUCCESS;
                goto Cleanup;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (i + 1 < ArgC) {
                    FileName = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("j")) == 0) {
                if (i + 1 < ArgC) {
                    if (YoriLibStringToNumber(&ArgV[i + 1], FALSE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
                        MakeContext.NumberProcesses = (DWORD)llTemp;
                        ArgumentUnderstood = TRUE;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("k")) == 0) {
                MakeContext.KeepGoing = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("m")) == 0) {
                Priority = MakePriorityLow;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("mm")) == 0) {
                Priority = MakePriorityVeryLow;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("nologo")) == 0) {
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("perf")) == 0) {
                MakeContext.PerfDisplay = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("pru")) == 0) {
                if (MakeContext.PreprocessorCache == NULL) {
                    MakeContext.PreprocessorCache = YoriLibAllocateHashTable(100);
                    if (MakeContext.PreprocessorCache == NULL) {
                        Result = EXIT_FAILURE;
                        goto Cleanup;
                    }
                }
                ArgumentUnderstood = TRUE;

            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                MakeContext.SilentCommandLaunching = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("wundef")) == 0) {
                MakeContext.WarnOnUndefinedVariable = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i + 1;
                ArgumentUnderstood = TRUE;
                break;
            }

            //
            //  If the arg had a parameter, skip it.  Unlike other Yori tools,
            //  this logic is done seperately here so it can be replicated
            //  when scanning for non-argument parameters below.
            //

            for (j = 0; j < sizeof(MakeArgsWithParameter)/sizeof(MakeArgsWithParameter[0]); j++) {
                if (YoriLibCompareStringInsensitive(&Arg, &MakeArgsWithParameter[j]) == 0) {
                    i++;
                    break;
                }
            }
        } else {
            ArgumentUnderstood = TRUE;
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
    //  If -j isn't specified, attempt to set the number of jobs based on the
    //  environment variable.  If that's not available, set a default number
    //  of jobs to execute as the number of logical processors plus one.
    //

    if (MakeContext.NumberProcesses == 0) {
        YORI_STRING JobCount;
        YoriLibInitEmptyString(&JobCount);
        if (YoriLibAllocateAndGetEnvironmentVariable(_T("YMAKE_JOB_COUNT"), &JobCount)) {
            if (YoriLibStringToNumber(&JobCount, FALSE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
                MakeContext.NumberProcesses = (DWORD)llTemp;
            }
            YoriLibFreeStringContents(&JobCount);
        }
    }

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

    if (Priority == MakePriorityVeryLow) {
        DWORD MajorVersion;
        DWORD MinorVersion;
        DWORD BuildNumber;

        YoriLibGetOsVersion(&MajorVersion, &MinorVersion, &BuildNumber);

        if (MajorVersion >= 6) {
            SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);
        } else {
            Priority = MakePriorityLow;
        }
    }

    if (Priority == MakePriorityLow) {
        SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
    }

    //
    //  Find the directory containing the makefile and populate it as the
    //  initial scope.
    //

    if (FileName == NULL) {
        YoriLibInitEmptyString(&RootDir);
        YoriLibConstantString(&Arg, _T("."));
        if (!YoriLibGetFullPathNameReturnAllocation(&Arg, FALSE, &RootDir, NULL)) {
            Result = EXIT_FAILURE;
            goto Cleanup;
        }
    } else {
        if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullFileName)) {
            YoriLibInitEmptyString(&FullFileName);
            Result = EXIT_FAILURE;
            goto Cleanup;
        }

        YoriLibCloneString(&RootDir, &FullFileName);

        for (i = RootDir.LengthInChars; i > 0; i--) {
            if (YoriLibIsSep(RootDir.StartOfString[i - 1])) {
                RootDir.LengthInChars = i - 1;
                break;
            }
        }
    }

    //
    //  Allocate and initialize the scope.
    //

    MakeContext.RootScope = MakeAllocateNewScope(&MakeContext, &RootDir);
    YoriLibFreeStringContents(&RootDir);
    if (MakeContext.RootScope == NULL) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    MakeContext.RootScope->ActiveConditionalNestingLevelExecutionEnabled = TRUE;

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

    MakeContext.ActiveScope = MakeContext.RootScope;

    //
    //  Find or open the makefile
    //

    if (FileName == NULL) {
        if (!MakeFindMakefileInDirectory(MakeContext.RootScope, &FullFileName)) {
            YoriLibInitEmptyString(&FullFileName);
            Result = EXIT_FAILURE;
            goto Cleanup;
        }
    } else {
        ASSERT(FullFileName.LengthInChars > 0);
    }

    //
    //  Loop through the arguments again, finding any variable definitions,
    //  and apply those now.
    //

    for (i = 1; i < ArgC; i++) {
        PYORI_STRING ThisArg;
        LPTSTR Equals;

        if (i < StartArg && YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {
            for (j = 0; j < sizeof(MakeArgsWithParameter)/sizeof(MakeArgsWithParameter[0]); j++) {
                if (YoriLibCompareStringInsensitive(&Arg, &MakeArgsWithParameter[j]) == 0) {
                    i++;
                    break;
                }
            }
        } else {

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
            }
        }
    }

#if YORI_BUILTIN
    YoriLibCancelEnable();
#endif

    //
    //  When using a cache, try to load any cached preprocessor conditions for
    //  this makefile.
    //

    if (MakeContext.PreprocessorCache != NULL) {
        MakeLoadPreprocessorCacheEntries(&MakeContext, &FullFileName);
    }

    hStream = CreateFile(FullFileName.StartOfString, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (hStream == INVALID_HANDLE_VALUE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No makefile found\n"));
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    //
    //  Preprocess the makefile
    //

    QueryPerformanceCounter(&StartTime);
    MakeProcessStream(hStream, &MakeContext, &FullFileName);
    QueryPerformanceCounter(&EndTime);

    MakeContext.TimeInPreprocessor = EndTime.QuadPart - StartTime.QuadPart;

    CloseHandle(hStream);

    if (MakeContext.ErrorTermination) {
        Result = EXIT_FAILURE;
        goto Cleanup;
    }

    MakeFindInferenceRulesForScope(MakeContext.RootScope);

    //
    //  Determine the tasks to execute
    //

    QueryPerformanceCounter(&StartTime);

    //
    //  Scan through command line arguments again, this time looking for
    //  targets to execute.
    //

    for (i = 1; i < ArgC; i++) {
        PYORI_STRING ThisArg;
        LPTSTR Equals;

        if (i < StartArg && YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {
            for (j = 0; j < sizeof(MakeArgsWithParameter)/sizeof(MakeArgsWithParameter[0]); j++) {
                if (YoriLibCompareStringInsensitive(&Arg, &MakeArgsWithParameter[j]) == 0) {
                    i++;
                    break;
                }
            }
        } else {

            ThisArg = &ArgV[i];

            Equals = YoriLibFindLeftMostCharacter(ThisArg, '=');
            if (Equals == NULL) {
                if (!MakeMarkCommandLineTargetForBuild(&MakeContext, ThisArg)) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not determine how to build target %y\n"), ThisArg);
                    Result = EXIT_FAILURE;
                    goto Cleanup;
                }

                //
                //  MSFIX There's currently no way to indicate that everything
                //  from the first target is done before the second target
                //  starts, so there's no way to express  "clean then all."
                //

                ExplicitTargetFound = TRUE;
            }
        }
    }

    //
    //  If the user didn't explicitly indicate a target, try the first.
    //

    if (!ExplicitTargetFound) {
        if (!MakeDetermineDependencies(&MakeContext)) {
            Result = EXIT_FAILURE;
            goto Cleanup;
        }
    }

    QueryPerformanceCounter(&EndTime);
    MakeContext.TimeBuildingGraph = EndTime.QuadPart - StartTime.QuadPart;

    //
    //  Execute the tasks
    //

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

    MakeDeleteInlineFiles(&MakeContext);
    MakeCleanupTemporaryDirectories(&MakeContext);

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
    MakeSaveAndDeleteAllPreprocessorCacheEntries(&MakeContext, &FullFileName);

    YoriLibFreeStringContents(&FullFileName);

    YoriLibFreeStringContents(&MakeContext.TempPath);
    YoriLibFreeStringContents(&MakeContext.ProcessCurrentDirectory);
    YoriLibFreeStringContents(&MakeContext.FilesToProbe[0]);
    YoriLibFreeStringContents(&MakeContext.FilesToProbe[1]);
    YoriLibShBuiltinUnregisterAll();

    YoriLibLineReadCleanupCache();

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
