/**
 * @file for/for.c
 *
 * Yori shell enumerate and operate on strings or files
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

#include <yoripch.h>
#include <yorilib.h>
#ifdef YORI_BUILTIN
#include <yoricall.h>
#endif

/**
 Help text to display to the user.
 */
const
CHAR strForHelpText[] =
        "Enumerates through a list of strings or files.\n"
        "\n"
        "FOR [-license] [-b] [-c] [-d] [-i <criteria>] [-p n] [-r] <var> in (<list>)\n"
        "    do <cmd>\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Use cmd as a subshell rather than Yori\n"
        "   -d             Match directories rather than files\n"
        "   -i <criteria>  Only treat match files if they meet criteria, see below\n"
        "   -l             Use (start,step,end) notation for the list\n"
        "   -p <n>         Execute with <n> concurrent processes\n"
        "   -r             Look for matches in subdirectories under the current directory\n"
        "\n"
        " The -i option will match files only if they meet criteria.  This is a\n"
        " semicolon delimited list of entries matching the following form:\n"
        "\n"
        "   [file attribute][operator][criteria]\n";

/**
 Display usage text to the user.
 */
BOOL
ForHelp()
{

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("For %i.%02i\n"), FOR_VER_MAJOR, FOR_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strForHelpText);

    //
    //  Display supported options and operators
    //

    YoriLibFileFiltHelp();
    return TRUE;
}

/**
 State about the currently running processes as well as information required
 to launch any new processes from this program.
 */
typedef struct _FOR_EXEC_CONTEXT {

    /**
     If TRUE, use CMD as a subshell.  If FALSE, use Yori.
     */
    BOOL InvokeCmd;

    /**
     The string that might be found in ArgV which should be changed to contain
     the value of any match.
     */
    PYORI_STRING SubstituteVariable;

    /**
     The number of elements in ArgV.
     */
    DWORD ArgC;

    /**
     The template form of an argv style argument array, before any
     substitution has taken place.
     */
    PYORI_STRING ArgV;

    /**
     The number of processes that this program would like to have concurrently
     running.
     */
    DWORD TargetConcurrentCount;

    /**
     The number of processes that are currently running as a result of this
     program.
     */
    DWORD CurrentConcurrentCount;

    /**
     An array of handles with CurrentConcurrentCount number of valid
     elements.  These correspond to processes that are currently running.
     */
    PHANDLE HandleArray;

    /**
     A list of criteria to filter matches against.
     */
    YORI_LIB_FILE_FILTER Filter;

} FOR_EXEC_CONTEXT, *PFOR_EXEC_CONTEXT;

/**
 Wait for any single process to complete.

 @param ExecContext Pointer to the for exec context containing information
        about currently running processes.
 */
VOID
ForWaitForProcessToComplete(
    __in PFOR_EXEC_CONTEXT ExecContext
    )
{
    DWORD Result;
    DWORD Index;
    DWORD Count;

    Result = WaitForMultipleObjects(ExecContext->CurrentConcurrentCount, ExecContext->HandleArray, FALSE, INFINITE);

    ASSERT(Result >= WAIT_OBJECT_0 && Result < (WAIT_OBJECT_0 + ExecContext->CurrentConcurrentCount));

    Index = Result - WAIT_OBJECT_0;

    CloseHandle(ExecContext->HandleArray[Index]);

    for (Count = Index; Count < ExecContext->CurrentConcurrentCount - 1; Count++) {
        ExecContext->HandleArray[Count] = ExecContext->HandleArray[Count + 1];
    }

    ExecContext->CurrentConcurrentCount--;
}

/**
 Execute a new command in response to a newly matched element.

 @param Match The match that was found from the set.

 @param ExecContext The current state of child processes and information about
        the arguments for any new child process.
 */
VOID
ForExecuteCommand(
    __in PYORI_STRING Match,
    __in PFOR_EXEC_CONTEXT ExecContext
    )
{
    DWORD ArgsNeeded = ExecContext->ArgC;
    DWORD PrefixArgCount;
    DWORD FoundOffset;
    DWORD Count;
    DWORD SubstitutesFound;
    DWORD ArgLengthNeeded;
    YORI_STRING OldArg;
    YORI_STRING NewArgWritePoint;
    PYORI_STRING NewArgArray;
    YORI_STRING CmdLine;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;

    YoriLibInitEmptyString(&CmdLine);

#ifdef YORI_BUILTIN
    if (!ExecContext->InvokeCmd &&
        ExecContext->TargetConcurrentCount == 1) {
        PrefixArgCount = 0;
    } else {
        PrefixArgCount = 2;
    }
#else
    PrefixArgCount = 2;
#endif

    ArgsNeeded += PrefixArgCount;

    NewArgArray = YoriLibMalloc(ArgsNeeded * sizeof(YORI_STRING));
    if (NewArgArray == NULL) {
        return;
    }

    ZeroMemory(NewArgArray, ArgsNeeded * sizeof(YORI_STRING));

    //
    //  If a full path is specified in the environment, use it.  If not,
    //  use a file name only and let PATH evaluation find an interpreter.
    //

    if (PrefixArgCount > 0) {
        if (ExecContext->InvokeCmd) {
            ArgLengthNeeded = GetEnvironmentVariable(_T("COMSPEC"), NULL, 0);
            if (ArgLengthNeeded != 0) {
                if (YoriLibAllocateString(&NewArgArray[0], ArgLengthNeeded)) {
                    NewArgArray[0].LengthInChars = GetEnvironmentVariable(_T("COMSPEC"), NewArgArray[0].StartOfString, NewArgArray[0].LengthAllocated);
                    if (NewArgArray[0].LengthInChars == 0) {
                        YoriLibFreeStringContents(&NewArgArray[0]);
                    }
                }
            }
            if (NewArgArray[0].LengthInChars == 0) {
                YoriLibConstantString(&NewArgArray[0], _T("cmd.exe"));
            }
        } else {
            ArgLengthNeeded = GetEnvironmentVariable(_T("YORISPEC"), NULL, 0);
            if (ArgLengthNeeded != 0) {
                if (YoriLibAllocateString(&NewArgArray[0], ArgLengthNeeded)) {
                    NewArgArray[0].LengthInChars = GetEnvironmentVariable(_T("YORISPEC"), NewArgArray[0].StartOfString, NewArgArray[0].LengthAllocated);
                    if (NewArgArray[0].LengthInChars == 0) {
                        YoriLibFreeStringContents(&NewArgArray[0]);
                    }
                }
            }
            if (NewArgArray[0].LengthInChars == 0) {
                YoriLibConstantString(&NewArgArray[0], _T("yori.exe"));
            }
        }
        YoriLibConstantString(&NewArgArray[1], _T("/c"));
    }

    for (Count = 0; Count < ExecContext->ArgC; Count++) {
        YoriLibInitEmptyString(&OldArg);
        OldArg.StartOfString = ExecContext->ArgV[Count].StartOfString;
        OldArg.LengthInChars = ExecContext->ArgV[Count].LengthInChars;
        SubstitutesFound = 0;

        while (YoriLibFindFirstMatchingSubstring(&OldArg, 1, ExecContext->SubstituteVariable, &FoundOffset)) {
            SubstitutesFound++;
            OldArg.StartOfString += FoundOffset + 1;
            OldArg.LengthInChars -= FoundOffset + 1;
        }

        ArgLengthNeeded = ExecContext->ArgV[Count].LengthInChars + SubstitutesFound * Match->LengthInChars - SubstitutesFound * ExecContext->SubstituteVariable->LengthInChars + 1;
        if (!YoriLibAllocateString(&NewArgArray[Count + PrefixArgCount], ArgLengthNeeded)) {
            goto Cleanup;
        }

        YoriLibInitEmptyString(&NewArgWritePoint);
        NewArgWritePoint.StartOfString = NewArgArray[Count + PrefixArgCount].StartOfString;
        NewArgWritePoint.LengthAllocated = NewArgArray[Count + PrefixArgCount].LengthAllocated;

        YoriLibInitEmptyString(&OldArg);
        OldArg.StartOfString = ExecContext->ArgV[Count].StartOfString;
        OldArg.LengthInChars = ExecContext->ArgV[Count].LengthInChars;

        while (TRUE) {
            if (YoriLibFindFirstMatchingSubstring(&OldArg, 1, ExecContext->SubstituteVariable, &FoundOffset)) {
                memcpy(NewArgWritePoint.StartOfString, OldArg.StartOfString, FoundOffset * sizeof(TCHAR));
                NewArgWritePoint.StartOfString += FoundOffset;
                NewArgWritePoint.LengthAllocated -= FoundOffset;
                memcpy(NewArgWritePoint.StartOfString, Match->StartOfString, Match->LengthInChars * sizeof(TCHAR));
                NewArgWritePoint.StartOfString += Match->LengthInChars;
                NewArgWritePoint.LengthAllocated -= Match->LengthInChars;

                OldArg.StartOfString += FoundOffset + ExecContext->SubstituteVariable->LengthInChars;
                OldArg.LengthInChars -= FoundOffset + ExecContext->SubstituteVariable->LengthInChars;
            } else {
                memcpy(NewArgWritePoint.StartOfString, OldArg.StartOfString, OldArg.LengthInChars * sizeof(TCHAR));
                NewArgWritePoint.StartOfString += OldArg.LengthInChars;
                NewArgWritePoint.LengthAllocated -= OldArg.LengthInChars;
                NewArgWritePoint.StartOfString[0] = '\0';

                NewArgArray[Count + PrefixArgCount].LengthInChars = (DWORD)(NewArgWritePoint.StartOfString - NewArgArray[Count + PrefixArgCount].StartOfString);
                ASSERT(NewArgArray[Count + PrefixArgCount].LengthInChars < NewArgArray[Count + PrefixArgCount].LengthAllocated);
                ASSERT(YoriLibIsStringNullTerminated(&NewArgArray[Count + PrefixArgCount]));
                break;
            }
        }
    }

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgsNeeded, NewArgArray, TRUE, &CmdLine)) {
        goto Cleanup;
    }

#ifdef YORI_BUILTIN
    if (PrefixArgCount == 0) {
        YoriCallExecuteExpression(&CmdLine);
        goto Cleanup;
    }
#endif

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInfo)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: execution failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        goto Cleanup;
    }

    CloseHandle(ProcessInfo.hThread);

    ExecContext->HandleArray[ExecContext->CurrentConcurrentCount] = ProcessInfo.hProcess;
    ExecContext->CurrentConcurrentCount++;

    if (ExecContext->CurrentConcurrentCount == ExecContext->TargetConcurrentCount) {
        ForWaitForProcessToComplete(ExecContext);
    }

Cleanup:

    for (Count = 0; Count < ArgsNeeded; Count++) {
        YoriLibFreeStringContents(&NewArgArray[Count]);
    }

    YoriLibFreeStringContents(&CmdLine);
    YoriLibFree(NewArgArray);
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param Context The current state of the program and information needed to
        launch new child processes.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
ForFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PFOR_EXEC_CONTEXT ExecContext;

    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FileInfo);

    ExecContext = (PFOR_EXEC_CONTEXT)Context;

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    if (!YoriLibFileFiltCheckFilterMatch(&ExecContext->Filter, FilePath, FileInfo)) {
        return TRUE;
    }

    ForExecuteCommand(FilePath, ExecContext);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the for builtin command.
 */
#define ENTRYPOINT YoriCmd_FOR
#else
/**
 The main entrypoint for the for standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the for cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    YORI_STRING Arg;
    BOOL LeftBraceOpen;
    BOOL RequiresExpansion;
    BOOL MatchDirectories;
    BOOL Recurse;
    BOOL BasicEnumeration;
    BOOL StepMode;
    DWORD CharIndex;
    DWORD MatchFlags;
    DWORD StartArg = 0;
    DWORD ListArg = 0;
    DWORD CmdArg = 0;
    DWORD ArgIndex;
    DWORD i;
    FOR_EXEC_CONTEXT ExecContext;

    ZeroMemory(&ExecContext, sizeof(ExecContext));

    ExecContext.TargetConcurrentCount = 1;
    ExecContext.CurrentConcurrentCount = 0;
    MatchDirectories = FALSE;
    Recurse = FALSE;
    StepMode = FALSE;
    BasicEnumeration = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ForHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                ExecContext.InvokeCmd = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                MatchDirectories = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                if (i + 1 < ArgC) {
                    YORI_STRING ErrorSubstring;
                    YoriLibInitEmptyString(&ErrorSubstring);

                    if (!YoriLibFileFiltParseFilterString(&ExecContext.Filter, &ArgV[i + 1], &ErrorSubstring)) {
                        if (ErrorSubstring.LengthInChars > 0) {
                            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: error parsing filter string '%y' at '%y'\n"), &ArgV[i + 1], &ErrorSubstring);
                        } else {
                            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: error parsing filter string '%y'\n"), &ArgV[i + 1]);
                        }
                        goto cleanup_and_exit;
                    }
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                StepMode = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                if (i + 1 < ArgC) {
                    LONGLONG LlNumberProcesses = 0;
                    DWORD CharsConsumed = 0;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &LlNumberProcesses, &CharsConsumed);
                    ExecContext.TargetConcurrentCount = (DWORD)LlNumberProcesses;
                    ArgumentUnderstood = TRUE;
                    if (ExecContext.TargetConcurrentCount < 1) {
                        ExecContext.TargetConcurrentCount = 1;
                    }
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                Recurse = TRUE;
                ArgumentUnderstood = TRUE;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: missing argument\n"));
        goto cleanup_and_exit;
    }

    ExecContext.SubstituteVariable = &ArgV[StartArg];

    //
    //  We need at least "%i in (*) do cmd"
    //

    if (ArgC < StartArg + 4) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: missing argument\n"));
        goto cleanup_and_exit;
    }

    if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[StartArg + 1], _T("in")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: 'in' not found\n"));
        goto cleanup_and_exit;
    }

    if (YoriLibCompareStringWithLiteralInsensitiveCount(&ArgV[StartArg + 2], _T("("), 1) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: left bracket not found\n"));
        goto cleanup_and_exit;
    }

    LeftBraceOpen = TRUE;

    //
    //  Walk through all arguments looking for a closing brace, then looking
    //  for "do".  Once we've done finding both, we have a trailing command
    //  string
    //

    ListArg = StartArg + 2;
    for (ArgIndex = ListArg; ArgIndex < ArgC; ArgIndex++) {
        if (LeftBraceOpen) {
            if (ArgV[ArgIndex].LengthInChars > 0 && ArgV[ArgIndex].StartOfString[ArgV[ArgIndex].LengthInChars - 1] == ')') {
                LeftBraceOpen = FALSE;
            }
        } else {
            if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[ArgIndex], _T("do")) == 0) {
                CmdArg = ArgIndex + 1;
                break;
            }
        }
    }

    if (CmdArg == 0) {
        if (LeftBraceOpen) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: right bracket not found\n"));
            goto cleanup_and_exit;
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: do not found\n"));
            goto cleanup_and_exit;
        }
    }

    if (CmdArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: command not found\n"));
        goto cleanup_and_exit;
    }

    ExecContext.ArgC = ArgC - CmdArg;
    ExecContext.ArgV = &ArgV[CmdArg];
    ExecContext.HandleArray = YoriLibMalloc(ExecContext.TargetConcurrentCount * sizeof(HANDLE));
    if (ExecContext.HandleArray == NULL) {
        goto cleanup_and_exit;
    }

    MatchFlags = 0;
    if (MatchDirectories) {
        MatchFlags = YORILIB_FILEENUM_RETURN_DIRECTORIES;
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES;
    }

    if (Recurse) {
        MatchFlags |= YORILIB_FILEENUM_RECURSE_AFTER_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
    }
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    if (StepMode) {
        LONGLONG Start;
        LONGLONG Step;
        LONGLONG End;
        LONGLONG Current;
        DWORD CharsConsumed;
        YORI_STRING FoundMatch;
        YORI_STRING Criteria;

        if (!YoriLibBuildCmdlineFromArgcArgv(CmdArg - ListArg, &ArgV[ListArg], FALSE, &Criteria)) {
            goto cleanup_and_exit;
        }

        //
        //  Remove the brackets
        //

        Criteria.StartOfString++;
        Criteria.LengthInChars -= 2;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the start value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &Start, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        Criteria.StartOfString += CharsConsumed;
        Criteria.LengthInChars -= CharsConsumed;
        YoriLibTrimSpaces(&Criteria);

        if (Criteria.StartOfString[0] != ',') {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: seperator not found\n"));

            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        Criteria.StartOfString += 1;
        Criteria.LengthInChars -= 1;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the step value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &Step, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        Criteria.StartOfString += CharsConsumed;
        Criteria.LengthInChars -= CharsConsumed;
        YoriLibTrimSpaces(&Criteria);

        if (Criteria.StartOfString[0] != ',') {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: seperator not found\n"));

            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        Criteria.StartOfString += 1;
        Criteria.LengthInChars -= 1;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the end value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &End, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        YoriLibFreeStringContents(&Criteria);

        if (!YoriLibAllocateString(&FoundMatch, 32)) {
            goto cleanup_and_exit;
        }

        Current = Start;

        do {
            if (Step > 0) {
                if (Current > End) {
                    break;
                }
            } else if (Step < 0) {
                if (Current < End) {
                    break;
                }
            }

            YoriLibNumberToString(&FoundMatch, Current, 10, 0, '\0');
            ForExecuteCommand(&FoundMatch, &ExecContext);
            Current += Step;
        } while(TRUE);

        YoriLibFreeStringContents(&FoundMatch);

    } else {

        for (ArgIndex = ListArg; ArgIndex < CmdArg - 1; ArgIndex++) {
            YORI_STRING ThisMatch;

            YoriLibInitEmptyString(&ThisMatch);

            ThisMatch.StartOfString = ArgV[ArgIndex].StartOfString;
            ThisMatch.LengthInChars = ArgV[ArgIndex].LengthInChars;
            ThisMatch.LengthAllocated = ArgV[ArgIndex].LengthAllocated;

            if (ArgIndex == ListArg) {
                ThisMatch.StartOfString++;
                ThisMatch.LengthInChars--;
                ThisMatch.LengthAllocated--;
            }

            if (ArgIndex == CmdArg - 2) {
                LPTSTR NullTerminatedString;

                ThisMatch.LengthInChars--;
                ThisMatch.LengthAllocated--;
                NullTerminatedString = YoriLibCStringFromYoriString(&ThisMatch);
                if (NullTerminatedString == NULL) {
                    goto cleanup_and_exit;
                }
                ThisMatch.MemoryToFree = NullTerminatedString;
                ThisMatch.StartOfString = NullTerminatedString;
            }

            RequiresExpansion = FALSE;
            if (Recurse) {
                RequiresExpansion = TRUE;
            } else {
                for (CharIndex = 0; CharIndex < ThisMatch.LengthInChars; CharIndex++) {
                    if (ThisMatch.StartOfString[CharIndex] == '*' ||
                        ThisMatch.StartOfString[CharIndex] == '?') {

                        RequiresExpansion = TRUE;
                        break;
                    }
                    if (!BasicEnumeration) {
                        if (ThisMatch.StartOfString[CharIndex] == '[' ||
                            ThisMatch.StartOfString[CharIndex] == '{') {

                            RequiresExpansion = TRUE;
                            break;
                        }
                    }
                }
            }

            if (ThisMatch.LengthInChars > 0) {

                if (RequiresExpansion) {
                    YoriLibForEachFile(&ThisMatch, MatchFlags, 0, ForFileFoundCallback, NULL, &ExecContext);
                } else {
                    ForExecuteCommand(&ThisMatch, &ExecContext);
                }
            }

            //
            //  Because MemoryToFree is not normally populated, this only
            //  really frees where the memory was double buffered
            //

            YoriLibFreeStringContents(&ThisMatch);
        }
    }

    while (ExecContext.CurrentConcurrentCount > 0) {
        ForWaitForProcessToComplete(&ExecContext);
    }

    YoriLibFileFiltFreeFilter(&ExecContext.Filter);
    YoriLibFree(ExecContext.HandleArray);

    return EXIT_SUCCESS;

cleanup_and_exit:

    YoriLibFileFiltFreeFilter(&ExecContext.Filter);

    return EXIT_FAILURE;
}

// vim:sw=4:ts=4:et:
