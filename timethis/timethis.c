/**
 * @file timethis/timethis.c
 *
 * Yori shell child process timer tool
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
CHAR strTimeThisHelpText[] =
        "\n"
        "Runs a child program and times its execution.\n"
        "\n"
        "TIMETHIS [-license] [-r] [-f <fmt>] <command>\n"
        "\n"
        "   -r                 Wait for all processes within the tree\n"
        "\n"
        "Format specifiers are:\n"
        "   $CHILDCPU$         Amount of CPU time used by the child process\n"
        "   $CHILDCPUMS$       Amount of CPU time used by the child process in ms\n"
        "   $CHILDKERNEL$      Amount of kernel time used by the child process\n"
        "   $CHILDKERNELMS$    Amount of kernel time used by the child process in ms\n"
        "   $CHILDUSER$        Amount of user time used by the child process\n"
        "   $CHILDUSERMS$      Amount of user time used by the child process in ms\n"
        "   $ELAPSEDTIME$      Amount of time taken to execute the child process\n"
        "   $ELAPSEDTIMEMS$    Amount of time taken to execute the child process in ms\n"
        "   $TREECPU$          Amount of CPU time used by all child processes\n"
        "   $TREECPUMS$        Amount of CPU time used by all child processes in ms\n"
        "   $TREEKERNEL$       Amount of kernel time used by all child processes\n"
        "   $TREEKERNELMS$     Amount of kernel time used by all child processes in ms\n"
        "   $TREEUSER$         Amount of user time used by all child processes\n"
        "   $TREEUSERMS$       Amount of user time used by all child processes in ms\n";

/**
 Display usage text to the user.
 */
BOOL
TimeThisHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("TimeThis %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strTimeThisHelpText);
    return TRUE;
}

/**
 Output a 64 bit integer.

 @param LargeInt A large integer to output.

 @param NumberBase Specifies the numeric base to use.  Should be 10 for
        decimal or 16 for hex.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
TimeThisOutputLargeInteger(
    __in LARGE_INTEGER LargeInt,
    __in WORD NumberBase,
    __inout PYORI_STRING OutputString
    )
{
    YORI_STRING String;
    TCHAR StringBuffer[32];
    YORI_MAX_SIGNED_T Value;

    YoriLibInitEmptyString(&String);
    String.StartOfString = StringBuffer;
    String.LengthAllocated = sizeof(StringBuffer)/sizeof(StringBuffer[0]);

    Value = (YORI_MAX_SIGNED_T)LargeInt.QuadPart;
    YoriLibNumberToString(&String, Value, NumberBase, 0, ' ');

    if (OutputString->LengthAllocated >= String.LengthInChars) {
        memcpy(OutputString->StartOfString, String.StartOfString, String.LengthInChars * sizeof(TCHAR));
    }

    return String.LengthInChars;
}

/**
 Output a time string from 64 bit integer of milliseconds.

 @param LargeInt A large integer to output.

 @param OutputString Pointer to a string to populate with the contents of
        the variable.

 @return The number of characters populated into the variable, or the number
         of characters required to successfully populate the contents into
         the variable.
 */
YORI_ALLOC_SIZE_T
TimeThisOutputTimestamp(
    __in LARGE_INTEGER LargeInt,
    __inout PYORI_STRING OutputString
    )
{
    YORI_STRING String;
    TCHAR StringBuffer[32];
    LARGE_INTEGER Remainder;
    WORD Milliseconds;
    WORD Seconds;
    WORD Minutes;
    WORD Hours;

    Remainder.QuadPart = LargeInt.QuadPart;
    Milliseconds = (WORD)(Remainder.LowPart % 1000);
    Remainder.QuadPart = Remainder.QuadPart / 1000;
    Seconds = (WORD)(Remainder.LowPart % 60);
    Remainder.QuadPart = Remainder.QuadPart / 60;
    Minutes = (WORD)(Remainder.LowPart % 60);
    Remainder.QuadPart = Remainder.QuadPart / 60;
    Hours = (WORD)Remainder.LowPart;

    YoriLibInitEmptyString(&String);
    String.StartOfString = StringBuffer;
    String.LengthAllocated = sizeof(StringBuffer)/sizeof(StringBuffer[0]);
    String.LengthInChars = YoriLibSPrintf(String.StartOfString, _T("%i:%02i:%02i.%03i"), Hours, Minutes, Seconds, Milliseconds);

    if (OutputString->LengthAllocated >= String.LengthInChars) {
        memcpy(OutputString->StartOfString, String.StartOfString, String.LengthInChars * sizeof(TCHAR));
    }

    return String.LengthInChars;
}


/**
 Context containing the results of execution to pass to helper function used
 to format output.
 */
typedef struct _TIMETHIS_CONTEXT {

    /**
     Amount of time in milliseconds that the immediate child process spent in
     kernel execution.
     */
    LARGE_INTEGER KernelTimeInMs;

    /**
     Amount of time in milliseconds that the immediate child process spent in
     user mode execution.
     */
    LARGE_INTEGER UserTimeInMs;

    /**
     Amount of time in milliseconds that the child process tree spent in
     kernel execution.
     */
    LARGE_INTEGER KernelTimeTreeInMs;

    /**
     Amount of time in milliseconds that the child process tree spent in
     user mode execution.
     */
    LARGE_INTEGER UserTimeTreeInMs;

    /**
     Amount of time taken to execute the child process.
     */
    LARGE_INTEGER WallTimeInMs;
} TIMETHIS_CONTEXT, *PTIMETHIS_CONTEXT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputBuffer A pointer to the output buffer to populate with data
        if a known variable is found.

 @param VariableName The variable name to expand.

 @param Context Pointer to a SYSTEMTIME structure containing the data to
        populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
YORI_ALLOC_SIZE_T
TimeThisExpandVariables(
    __inout PYORI_STRING OutputBuffer,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    LARGE_INTEGER CpuTime;
    PTIMETHIS_CONTEXT TimeThisContext = (PTIMETHIS_CONTEXT)Context;

    if (YoriLibCompareStringLit(VariableName, _T("CHILDCPU")) == 0) {
        CpuTime.QuadPart = TimeThisContext->KernelTimeInMs.QuadPart + TimeThisContext->UserTimeInMs.QuadPart;
        return TimeThisOutputTimestamp(CpuTime, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("CHILDCPUMS")) == 0) {
        CpuTime.QuadPart = TimeThisContext->KernelTimeInMs.QuadPart + TimeThisContext->UserTimeInMs.QuadPart;
        return TimeThisOutputLargeInteger(CpuTime, 10, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("CHILDKERNEL")) == 0) {
        return TimeThisOutputTimestamp(TimeThisContext->KernelTimeInMs, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("CHILDKERNELMS")) == 0) {
        return TimeThisOutputLargeInteger(TimeThisContext->KernelTimeInMs, 10, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("CHILDUSER")) == 0) {
        return TimeThisOutputTimestamp(TimeThisContext->UserTimeInMs, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("CHILDUSERMS")) == 0) {
        return TimeThisOutputLargeInteger(TimeThisContext->UserTimeInMs, 10, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("ELAPSEDTIME")) == 0) {
        return TimeThisOutputTimestamp(TimeThisContext->WallTimeInMs, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("ELAPSEDTIMEMS")) == 0) {
        return TimeThisOutputLargeInteger(TimeThisContext->WallTimeInMs, 10, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("TREECPU")) == 0) {
        CpuTime.QuadPart = TimeThisContext->KernelTimeTreeInMs.QuadPart + TimeThisContext->UserTimeTreeInMs.QuadPart;
        return TimeThisOutputTimestamp(CpuTime, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("TREECPUMS")) == 0) {
        CpuTime.QuadPart = TimeThisContext->KernelTimeTreeInMs.QuadPart + TimeThisContext->UserTimeTreeInMs.QuadPart;
        return TimeThisOutputLargeInteger(CpuTime, 10, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("TREEKERNEL")) == 0) {
        return TimeThisOutputTimestamp(TimeThisContext->KernelTimeTreeInMs, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("TREEKERNELMS")) == 0) {
        return TimeThisOutputLargeInteger(TimeThisContext->KernelTimeTreeInMs, 10, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("TREEUSER")) == 0) {
        return TimeThisOutputTimestamp(TimeThisContext->UserTimeTreeInMs, OutputBuffer);
    } else if (YoriLibCompareStringLit(VariableName, _T("TREEUSERMS")) == 0) {
        return TimeThisOutputLargeInteger(TimeThisContext->UserTimeTreeInMs, 10, OutputBuffer);
    }
    return 0;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the timethis builtin command.
 */
#define ENTRYPOINT YoriCmd_TIMETHIS
#else
/**
 The main entrypoint for the timethis standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the timethis cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_STRING CmdLine;
    DWORD ExitCode;
    BOOLEAN ArgumentUnderstood;
    BOOLEAN Recursive = FALSE;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_ALLOC_SIZE_T i;
    YORI_STRING Arg;
    YORI_STRING DisplayString;
    YORI_STRING AllocatedFormatString;
    TIMETHIS_CONTEXT TimeThisContext;
    YORI_STRING Executable;
    PYORI_STRING ChildArgs;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    HANDLE hJob = NULL;
    HANDLE hPort = NULL;
    FILETIME ftCreationTime;
    FILETIME ftExitTime;
    FILETIME ftKernelTime;
    FILETIME ftUserTime;

    LARGE_INTEGER liCreationTime;
    LARGE_INTEGER liExitTime;
    LPTSTR DefaultFormatString = _T("Elapsed time:      $ELAPSEDTIME$\n")
                                 _T("Child CPU time:    $CHILDCPU$\n")
                                 _T("Child kernel time: $CHILDKERNEL$\n")
                                 _T("Child user time:   $CHILDUSER$\n")
                                 _T("Tree CPU time:     $TREECPU$\n")
                                 _T("Tree kernel time:  $TREEKERNEL$\n")
                                 _T("Tree user time:    $TREEUSER$\n");

    YoriLibInitEmptyString(&AllocatedFormatString);
    YoriLibConstantString(&AllocatedFormatString, DefaultFormatString);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                TimeThisHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("r")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("f")) == 0) {
                if (ArgC > i + 1) {
                    YoriLibFreeStringContents(&AllocatedFormatString);
                    YoriLibCloneString(&AllocatedFormatString, &ArgV[i + 1]);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("timethis: missing argument\n"));
        return EXIT_FAILURE;
    }

    ChildArgs = YoriLibMalloc((ArgC - StartArg) * sizeof(YORI_STRING));
    if (ChildArgs == NULL) {
        YoriLibFreeStringContents(&AllocatedFormatString);
        return EXIT_FAILURE;
    }

    YoriLibInitEmptyString(&Executable);
    if (!YoriLibLocateExecutableInPath(&ArgV[StartArg], NULL, NULL, &Executable) ||
        Executable.LengthInChars == 0) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("timethis: unable to find executable\n"));
        YoriLibFreeStringContents(&AllocatedFormatString);
        YoriLibFree(ChildArgs);
        YoriLibFreeStringContents(&Executable);
        return EXIT_FAILURE;
    }


    memcpy(&ChildArgs[0], &Executable, sizeof(YORI_STRING));
    if (StartArg + 1 < (YORI_ALLOC_SIZE_T)ArgC) {
        memcpy(&ChildArgs[1], &ArgV[StartArg + 1], (ArgC - StartArg - 1) * sizeof(YORI_STRING));
    }

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, ChildArgs, TRUE, TRUE, &CmdLine)) {
        YoriLibFree(ChildArgs);
        YoriLibFreeStringContents(&Executable);
        YoriLibFreeStringContents(&AllocatedFormatString);
        return EXIT_FAILURE;
    }

    ASSERT(YoriLibIsStringNullTerminated(&CmdLine));

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, CREATE_SUSPENDED | CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &StartupInfo, &ProcessInfo)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("timethis: execution failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFree(ChildArgs);
        YoriLibFreeStringContents(&CmdLine);
        YoriLibFreeStringContents(&Executable);
        YoriLibFreeStringContents(&AllocatedFormatString);
        return EXIT_FAILURE;
    }

    hJob = YoriLibCreateJobObject();
    if (hJob != NULL) {
        if (Recursive) {
            hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
            if (hPort != NULL) {
                JOBOBJECT_ASSOCIATE_COMPLETION_PORT Port;
                Port.CompletionKey = hJob;
                Port.CompletionPort = hPort;
                SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &Port, sizeof(Port));
            }
        }
        YoriLibAssignProcessToJobObject(hJob, ProcessInfo.hProcess);
    }

    ResumeThread(ProcessInfo.hThread);

    YoriLibFreeStringContents(&Executable);
    YoriLibFreeStringContents(&CmdLine);
    YoriLibFree(ChildArgs);

    //
    //  Wait for the immediate child process to terminate.
    //

#if YORI_BUILTIN
    {
        HANDLE HandleArray[2];
        DWORD WaitResult;

        YoriLibCancelEnable(FALSE);
        HandleArray[1] = YoriLibCancelGetEvent();
        HandleArray[0] = ProcessInfo.hProcess;

        WaitResult = WaitForMultipleObjectsEx(2, HandleArray, FALSE, INFINITE, FALSE);

        //
        //  If cancelled, abort
        //

        if (WaitResult == WAIT_OBJECT_0 + 1) {
            CloseHandle(ProcessInfo.hProcess);
            CloseHandle(ProcessInfo.hThread);
            if (hJob != NULL) {
                CloseHandle(hJob);
            }
            YoriLibFreeStringContents(&AllocatedFormatString);

            return EXIT_FAILURE;
        }
    }
#else
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
#endif
    GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);

    //
    //  Save off times from the child process.
    //

    GetProcessTimes(ProcessInfo.hProcess, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime);

    liCreationTime.HighPart = ftCreationTime.dwHighDateTime;
    liCreationTime.LowPart = ftCreationTime.dwLowDateTime;
    liExitTime.HighPart = ftExitTime.dwHighDateTime;
    liExitTime.LowPart = ftExitTime.dwLowDateTime;
    TimeThisContext.KernelTimeInMs.HighPart = ftKernelTime.dwHighDateTime;
    TimeThisContext.KernelTimeInMs.LowPart = ftKernelTime.dwLowDateTime;
    TimeThisContext.KernelTimeInMs.QuadPart = TimeThisContext.KernelTimeInMs.QuadPart / (10 * 1000);
    TimeThisContext.UserTimeInMs.HighPart = ftUserTime.dwHighDateTime;
    TimeThisContext.UserTimeInMs.LowPart = ftUserTime.dwLowDateTime;
    TimeThisContext.UserTimeInMs.QuadPart = TimeThisContext.UserTimeInMs.QuadPart / (10 * 1000);

    TimeThisContext.WallTimeInMs.QuadPart = (liExitTime.QuadPart - liCreationTime.QuadPart) / (10 * 1000);

    //
    //  Save off times from all processes within the job, if it exists.
    //

    TimeThisContext.KernelTimeTreeInMs.QuadPart = TimeThisContext.KernelTimeInMs.QuadPart;
    TimeThisContext.UserTimeTreeInMs.QuadPart = TimeThisContext.UserTimeInMs.QuadPart;

    if (hJob != NULL) {
        YORI_JOB_BASIC_ACCOUNTING_INFORMATION JobInfo;
        DWORD BytesReturned;

        if (hPort != NULL) {
            DWORD CompletionCode;
            ULONG_PTR CompletionKey;
            LPOVERLAPPED Overlapped;
            while (GetQueuedCompletionStatus(hPort, &CompletionCode, &CompletionKey, &Overlapped, INFINITE) &&
                !((HANDLE)CompletionKey == hJob && CompletionCode == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO)) {
            }
            CloseHandle(hPort);
        }

        if (DllKernel32.pQueryInformationJobObject != NULL &&
            DllKernel32.pQueryInformationJobObject(hJob, 1, &JobInfo, sizeof(JobInfo), &BytesReturned)) {

            TimeThisContext.KernelTimeTreeInMs.QuadPart = JobInfo.TotalKernelTime.QuadPart / (10 * 1000);
            TimeThisContext.UserTimeTreeInMs.QuadPart = JobInfo.TotalUserTime.QuadPart / (10 * 1000);
        }
        CloseHandle(hJob);
    }

    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);

    YoriLibInitEmptyString(&DisplayString);
    YoriLibExpandCommandVariables(&AllocatedFormatString, '$', FALSE, TimeThisExpandVariables, &TimeThisContext, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        YoriLibFreeStringContents(&DisplayString);
    }
    YoriLibFreeStringContents(&AllocatedFormatString);

    return ExitCode;
}

// vim:sw=4:ts=4:et:
