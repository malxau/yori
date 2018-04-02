/**
 * @file for/for.c
 *
 * Yori shell display command line output
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
 Help text to display to the user.
 */
const
CHAR strHelpText[] =
        "Enumerates through a list of strings or files.\n"
        "\n"
        "FOR [-b] [-c] [-d] [-p n] <var> in (<list>) do <cmd>\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Use cmd as a subshell rather than Yori\n"
        "   -d             Match directories rather than files\n"
        "   -l             Use (start,step,end) notation for the list\n"
        "   -p <n>         Execute with <n> concurrent processes\n";

/**
 Display usage text to the user.
 */
BOOL
ForHelp()
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2017"), &License);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("For %i.%i\n"), FOR_VER_MAJOR, FOR_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &License);
    YoriLibFreeStringContents(&License);
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
    DWORD ArgsNeeded = ExecContext->ArgC + 2;
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

    NewArgArray = YoriLibMalloc(ArgsNeeded * sizeof(YORI_STRING));
    if (NewArgArray == NULL) {
        return;
    }

    ZeroMemory(NewArgArray, ArgsNeeded * sizeof(YORI_STRING));

    //
    //  MSFIX Should use a YORISPEC environment for this
    //

    if (ExecContext->InvokeCmd) {
        YoriLibConstantString(&NewArgArray[0], _T("cmd.exe"));
    } else {
        YoriLibConstantString(&NewArgArray[0], _T("yori.exe"));
    }
    YoriLibConstantString(&NewArgArray[1], _T("/c"));

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
        if (!YoriLibAllocateString(&NewArgArray[Count + 2], ArgLengthNeeded)) {
            goto Cleanup;
        }

        YoriLibInitEmptyString(&NewArgWritePoint);
        NewArgWritePoint.StartOfString = NewArgArray[Count + 2].StartOfString;
        NewArgWritePoint.LengthAllocated = NewArgArray[Count + 2].LengthAllocated;

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
                
                NewArgArray[Count + 2].LengthInChars = (DWORD)(NewArgWritePoint.StartOfString - NewArgArray[Count + 2].StartOfString);
                ASSERT(NewArgArray[Count + 2].LengthInChars < NewArgArray[Count + 2].LengthAllocated);
                ASSERT(YoriLibIsStringNullTerminated(&NewArgArray[Count + 2]));
                break;
            }
        }
    }

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgsNeeded, NewArgArray, TRUE, &CmdLine)) {
        goto Cleanup;
    }

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

    for (Count = 2; Count < ArgsNeeded; Count++) {
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

 @param ExecContext The current state of the program and information needed to
        launch new child processes.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
ForFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PFOR_EXEC_CONTEXT ExecContext
    )
{
    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FileInfo);

    ASSERT(FilePath->StartOfString[FilePath->LengthInChars] == '\0');

    ForExecuteCommand(FilePath, ExecContext);
    return TRUE;
}

/**
 The main entrypoint for the for cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ymain(
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
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                ExecContext.InvokeCmd = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                MatchDirectories = TRUE;
                ArgumentUnderstood = TRUE;
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
        return EXIT_FAILURE;
    }

    ExecContext.SubstituteVariable = &ArgV[StartArg];

    //
    //  We need at least "%i in (*) do cmd"
    //

    if (ArgC < StartArg + 4) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[StartArg + 1], _T("in")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: 'in' not found\n"));
        return EXIT_FAILURE;
    }

    if (YoriLibCompareStringWithLiteralInsensitiveCount(&ArgV[StartArg + 2], _T("("), 1) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: left bracket not found\n"));
        return EXIT_FAILURE;
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
            return EXIT_FAILURE;
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: do not found\n"));
            return EXIT_FAILURE;
        }
    }

    if (CmdArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: command not found\n"));
        return EXIT_FAILURE;
    }

    ExecContext.ArgC = ArgC - CmdArg;
    ExecContext.ArgV = &ArgV[CmdArg];
    ExecContext.HandleArray = YoriLibMalloc(ExecContext.TargetConcurrentCount * sizeof(HANDLE));
    if (ExecContext.HandleArray == NULL) {
        return EXIT_FAILURE;
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
            return EXIT_FAILURE;
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
            return EXIT_FAILURE;
        }

        Criteria.StartOfString += CharsConsumed;
        Criteria.LengthInChars -= CharsConsumed;
        YoriLibTrimSpaces(&Criteria);

        if (Criteria.StartOfString[0] != ',') {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: seperator not found\n"));

            return EXIT_FAILURE;
        }
        
        Criteria.StartOfString += 1;
        Criteria.LengthInChars -= 1;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the step value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &Step, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            return EXIT_FAILURE;
        }

        Criteria.StartOfString += CharsConsumed;
        Criteria.LengthInChars -= CharsConsumed;
        YoriLibTrimSpaces(&Criteria);

        if (Criteria.StartOfString[0] != ',') {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: seperator not found\n"));

            return EXIT_FAILURE;
        }
        
        Criteria.StartOfString += 1;
        Criteria.LengthInChars -= 1;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the end value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &End, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            return EXIT_FAILURE;
        }

        YoriLibFreeStringContents(&Criteria);

        if (!YoriLibAllocateString(&FoundMatch, 32)) {
            return EXIT_FAILURE;
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
                    return EXIT_FAILURE;
                }
                ThisMatch.MemoryToFree = NullTerminatedString;
                ThisMatch.StartOfString = NullTerminatedString;
            }

            RequiresExpansion = FALSE;
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

            if (ThisMatch.LengthInChars > 0) {

                if (RequiresExpansion) {
                    YoriLibForEachFile(&ThisMatch, MatchFlags, 0, ForFileFoundCallback, &ExecContext);
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

    YoriLibFree(ExecContext.HandleArray);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
