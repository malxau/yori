/**
 * @file ps/ps.c
 *
 * Yori shell display process list
 *
 * Copyright (c) 2019-2022 Malcolm J. Smith
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
CHAR strPsHelpText[] =
        "\n"
        "Display process list.\n"
        "\n"
        "PS [-license] [-a] [-f] [-l]\n"
        "\n"
        "   -a             Display all processes\n"
        "   -f             Display full format including command line\n"
        "   -l             Display long format including memory usage\n";

/**
 Display usage text to the user.
 */
BOOL
PsHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ps %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strPsHelpText);
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
DWORD
PsOutputLargeInteger(
    __in LARGE_INTEGER LargeInt,
    __in DWORD NumberBase,
    __inout PYORI_STRING OutputString
    )
{
    YORI_STRING String;
    TCHAR StringBuffer[32];

    YoriLibInitEmptyString(&String);
    String.StartOfString = StringBuffer;
    String.LengthAllocated = sizeof(StringBuffer)/sizeof(StringBuffer[0]);

    YoriLibNumberToString(&String, LargeInt.QuadPart, NumberBase, 0, ' ');

    if (OutputString->LengthAllocated >= String.LengthInChars) {
        memcpy(OutputString->StartOfString, String.StartOfString, String.LengthInChars * sizeof(TCHAR));
    }

    return String.LengthInChars;
}

/**
 Context about process enumeration tasks to perform.
 */
typedef struct _PS_CONTEXT {

    /**
     The current system time.
     */
    LARGE_INTEGER Now;

    /**
     TRUE to attempt to display the command line for the process.
     */
    BOOL DisplayCommandLine;

    /**
     TRUE to display memory statistics for the process.
     */
    BOOL DisplayMemory;

} PS_CONTEXT, *PPS_CONTEXT;

/**
 Display a header for the output including the fields and spacing that the
 requested output will have.

 @param PsContext Pointer to the ps context specifying which fields will be
        displayed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
PsDisplayHeader(
    __in PPS_CONTEXT PsContext
    )
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Pid  | Parent | LiveTime | ExecTime | Process         "));
    if (PsContext->DisplayMemory) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("| WorkingSet | Commit     "));
    }
    if (PsContext->DisplayCommandLine) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("| CommandLine"));
    }
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
    return TRUE;
}

/**
 Attempt to display the process command line.  Because this is grovelling
 memory from the target process, it frequently fails due to lack of access,
 and could fail for other reasons too.

 @param ProcessId Specifies the process which should have its command line
        displayed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
PsDisplayProcessCommandLine(
    __in DWORD_PTR ProcessId
    )
{
    HANDLE ProcessHandle;
    BOOL TargetProcess32BitPeb;
    LONG Status;
    PROCESS_BASIC_INFORMATION BasicInfo;
    SIZE_T BytesReturned;
    DWORD dwBytesReturned;
    PVOID ProcessParamsBlockToRead;
    PVOID CommandLineToRead;
    DWORD CommandLineBytes;
    YORI_STRING CommandLine;

    ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, (DWORD)ProcessId);
    if (ProcessHandle == NULL) {
        return FALSE;
    }

    if (DllNtDll.pNtQueryInformationProcess == NULL) {
        CloseHandle(ProcessHandle);
        return FALSE;
    }

    TargetProcess32BitPeb = YoriLibDoesProcessHave32BitPeb(ProcessHandle);

    Status = DllNtDll.pNtQueryInformationProcess(ProcessHandle, 0, &BasicInfo, sizeof(BasicInfo), &dwBytesReturned);
    if (Status != 0) {
        CloseHandle(ProcessHandle);
        return FALSE;
    }

    //
    //  Try to read the PEB and find the ProcessParameters.
    //

    if (TargetProcess32BitPeb) {
        YORI_LIB_PEB32_NATIVE ProcessPeb;

        if (!ReadProcessMemory(ProcessHandle, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            CloseHandle(ProcessHandle);
            return FALSE;
        }
        ProcessParamsBlockToRead = (PVOID)(ULONG_PTR)ProcessPeb.ProcessParameters;
    } else {
        YORI_LIB_PEB64 ProcessPeb;

        if (!ReadProcessMemory(ProcessHandle, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            CloseHandle(ProcessHandle);
            return FALSE;
        }
        ProcessParamsBlockToRead = (PVOID)(ULONG_PTR)ProcessPeb.ProcessParameters;
    }

    //
    //  Try to read the ProcessParameters to find the Command Line.
    //

    if (TargetProcess32BitPeb) {
        YORI_LIB_PROCESS_PARAMETERS32 ProcessParameters;

        if (!ReadProcessMemory(ProcessHandle, ProcessParamsBlockToRead, &ProcessParameters, sizeof(ProcessParameters), &BytesReturned)) {
            CloseHandle(ProcessHandle);
            return FALSE;
        }

        CommandLineToRead = (PVOID)(ULONG_PTR)ProcessParameters.CommandLine;
        CommandLineBytes = ProcessParameters.CommandLineLengthInBytes;
    } else {
        YORI_LIB_PROCESS_PARAMETERS64 ProcessParameters;

        if (!ReadProcessMemory(ProcessHandle, ProcessParamsBlockToRead, &ProcessParameters, sizeof(ProcessParameters), &BytesReturned)) {
            CloseHandle(ProcessHandle);
            return FALSE;
        }

        CommandLineToRead = (PVOID)(ULONG_PTR)ProcessParameters.CommandLine;
        CommandLineBytes = ProcessParameters.CommandLineLengthInBytes;
    }

    if (CommandLineBytes == 0) {
        CloseHandle(ProcessHandle);
        return FALSE;
    }

    //
    //  Try to read the command line.
    //

    if (!YoriLibAllocateString(&CommandLine, CommandLineBytes / sizeof(TCHAR))) {
        CloseHandle(ProcessHandle);
        return FALSE;
    }

    if (!ReadProcessMemory(ProcessHandle, CommandLineToRead, CommandLine.StartOfString, CommandLineBytes, &BytesReturned)) {
        CloseHandle(ProcessHandle);
        YoriLibFreeStringContents(&CommandLine);
        return FALSE;
    }

    CommandLine.LengthInChars = (DWORD)BytesReturned / sizeof(TCHAR);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T(" | %y"), &CommandLine);
    YoriLibFreeStringContents(&CommandLine);
    CloseHandle(ProcessHandle);
    return TRUE;
}


/**
 Display information about a process given a structure containing process
 information.

 @param PsContext Pointer to the ps context indicating what to display.

 @param ProcessInfo Pointer to information about the process to display.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
PsDisplayProcessByStructure(
    __in PPS_CONTEXT PsContext,
    __in PYORI_SYSTEM_PROCESS_INFORMATION ProcessInfo
    )
{
    YORI_STRING BaseName;
    LARGE_INTEGER ExecuteTime;
    LARGE_INTEGER LiveTime;
    DWORD LiveTimeSeconds;
    DWORD LiveTimeMinutes;
    DWORD LiveTimeHours;
    DWORD ExecTimeSeconds;
    DWORD ExecTimeMinutes;
    DWORD ExecTimeHours;
    YORI_STRING LiveTimeString;
    TCHAR LiveTimeBuffer[sizeof("12:34:56")];

    YoriLibInitEmptyString(&BaseName);
    BaseName.StartOfString = ProcessInfo->ImageName;
    BaseName.LengthInChars = ProcessInfo->ImageNameLengthInBytes / sizeof(WCHAR);

    if (BaseName.LengthInChars == 0 && ProcessInfo->ProcessId == 0) {
        YoriLibConstantString(&BaseName, _T("Idle"));
    }

    LiveTime.QuadPart = PsContext->Now.QuadPart - ProcessInfo->CreateTime.QuadPart;
    // LiveTime.QuadPart = ProcessInfo->CreateTime.QuadPart;
    if (LiveTime.QuadPart < 0) {
        LiveTime.QuadPart = 0;
    }
    LiveTime.QuadPart = LiveTime.QuadPart / (10 * 1000 * 1000);
    LiveTimeSeconds = (DWORD)(LiveTime.QuadPart % 60);
    LiveTimeMinutes = (DWORD)((LiveTime.QuadPart / 60) % 60);
    LiveTimeHours = (DWORD)(LiveTime.QuadPart / 3600);

    YoriLibInitEmptyString(&LiveTimeString);
    LiveTimeString.StartOfString = LiveTimeBuffer;
    LiveTimeString.LengthAllocated = sizeof(LiveTimeBuffer)/sizeof(LiveTimeBuffer[0]);
    if (LiveTimeHours > 99) {
        LiveTimeString.LengthInChars = YoriLibSPrintfS(LiveTimeString.StartOfString, LiveTimeString.LengthAllocated, _T("%id"), LiveTimeHours / 24);
    } else {
        LiveTimeString.LengthInChars = YoriLibSPrintfS(LiveTimeString.StartOfString, LiveTimeString.LengthAllocated, _T("%02i:%02i:%02i"), LiveTimeHours, LiveTimeMinutes, LiveTimeSeconds);
    }

    ExecuteTime.QuadPart = ProcessInfo->KernelTime.QuadPart + ProcessInfo->UserTime.QuadPart;
    ExecuteTime.QuadPart = ExecuteTime.QuadPart / (10 * 1000 * 1000);
    ExecTimeSeconds = (DWORD)(ExecuteTime.QuadPart % 60);
    ExecTimeMinutes = (DWORD)((ExecuteTime.QuadPart / 60) % 60);
    ExecTimeHours = (DWORD)(ExecuteTime.QuadPart / 3600);

    if (PsContext->DisplayMemory) {
        LARGE_INTEGER liCommit;
        LARGE_INTEGER liWorkingSet;
        YORI_STRING CommitString;
        YORI_STRING WorkingSetString;
        TCHAR CommitStringBuffer[6];
        TCHAR WorkingSetStringBuffer[6];

        YoriLibInitEmptyString(&CommitString);
        YoriLibInitEmptyString(&WorkingSetString);

        CommitString.StartOfString = CommitStringBuffer;
        CommitString.LengthAllocated = sizeof(CommitStringBuffer)/sizeof(CommitStringBuffer[0]);

        WorkingSetString.StartOfString = WorkingSetStringBuffer;
        WorkingSetString.LengthAllocated = sizeof(WorkingSetStringBuffer)/sizeof(WorkingSetStringBuffer[0]);

        //
        //  Hack to fix formats
        //

        liCommit.QuadPart = ProcessInfo->CommitSize;
        liWorkingSet.QuadPart = ProcessInfo->WorkingSetSize;
        YoriLibFileSizeToString(&CommitString, &liCommit);
        YoriLibFileSizeToString(&WorkingSetString, &liWorkingSet);

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-6i | %-6i | %8y | %02i:%02i:%02i | %-15y | %-10y | %-10y"), ProcessInfo->ProcessId, ProcessInfo->ParentProcessId, &LiveTimeString, ExecTimeHours, ExecTimeMinutes, ExecTimeSeconds, &BaseName, &WorkingSetString, &CommitString);
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-6i | %-6i | %8y | %02i:%02i:%02i | %-15y"), ProcessInfo->ProcessId, ProcessInfo->ParentProcessId, &LiveTimeString, ExecTimeHours, ExecTimeMinutes, ExecTimeSeconds, &BaseName);
    }

    if (PsContext->DisplayCommandLine) {
        PsDisplayProcessCommandLine(ProcessInfo->ProcessId);
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));

    return TRUE;
}

/**
 Display information about a process given a list of process information for
 all processes in the system and a PID of interest.

 @param PsContext Pointer to the ps context indicating what to display.

 @param ProcessList Pointer to a list of all processes in the system.
 
 @param ProcessId Specifies the process ID to display from within the list of
        known processes.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
PsDisplayProcessByPid(
    __in PPS_CONTEXT PsContext,
    __in PYORI_SYSTEM_PROCESS_INFORMATION ProcessList,
    __in DWORD_PTR ProcessId
    )
{
    PYORI_SYSTEM_PROCESS_INFORMATION CurrentEntry;
    BOOL Found = FALSE;

    CurrentEntry = ProcessList;
    do {
        if (CurrentEntry->ProcessId == ProcessId) {
            PsDisplayProcessByStructure(PsContext, CurrentEntry);
            Found = TRUE;
            break;
        }
        if (CurrentEntry->NextEntryOffset == 0) {
            break;
        }
        CurrentEntry = YoriLibAddToPointer(CurrentEntry, CurrentEntry->NextEntryOffset);
    } while(TRUE);

    return Found;
}

/**
 Display information about all processes that the current user has access
 to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
PsDisplayAllProcesses(
    __in PPS_CONTEXT PsContext
    )
{
    PYORI_SYSTEM_PROCESS_INFORMATION ProcessInfo = NULL;

    PYORI_SYSTEM_PROCESS_INFORMATION CurrentEntry;
    if (!YoriLibGetSystemProcessList(&ProcessInfo)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("yps: Unable to load system process list\n"));
        return FALSE;
    }

    PsDisplayHeader(PsContext);
    CurrentEntry = ProcessInfo;
    do {
        PsDisplayProcessByStructure(PsContext, CurrentEntry);
        if (CurrentEntry->NextEntryOffset == 0) {
            break;
        }
        CurrentEntry = YoriLibAddToPointer(CurrentEntry, CurrentEntry->NextEntryOffset);
    } while(TRUE);

    YoriLibFree(ProcessInfo);
    return TRUE;
}

/**
 Display information about processes attached to the current console.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
PsDisplayConsoleProcesses(
    __in PPS_CONTEXT PsContext
    )
{
    PYORI_SYSTEM_PROCESS_INFORMATION ProcessInfo = NULL;
    LPDWORD PidList = NULL;
    DWORD PidListSize = 0;
    DWORD PidCountReturned;
    DWORD Index;

    if (DllKernel32.pGetConsoleProcessList == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("OS support not present\n"));
        return FALSE;
    }

    if (!YoriLibGetSystemProcessList(&ProcessInfo)) {
        return FALSE;
    }

    do {
        PidCountReturned = DllKernel32.pGetConsoleProcessList(PidList, PidListSize);
        if (PidCountReturned == 0 && PidList != NULL) {
            YoriLibFree(ProcessInfo);
            if (PidList) {
                YoriLibFree(PidList);
            }
            return FALSE;
        }

        if (PidCountReturned > 0 && PidCountReturned <= PidListSize) {
            break;
        }

        if (PidCountReturned == 0) {
            PidCountReturned = 4;
        }

        if (PidList) {
            YoriLibFree(PidList);
        }

        PidListSize = PidCountReturned + 4;
        PidList = YoriLibMalloc(PidListSize * sizeof(DWORD));
        if (PidList == NULL) {
            YoriLibFree(ProcessInfo);
            return FALSE;
        }

    } while(TRUE);

    PsDisplayHeader(PsContext);
    for (Index = 0; Index < PidCountReturned; Index++) {
        PsDisplayProcessByPid(PsContext, ProcessInfo, PidList[Index]);
    }

    YoriLibFree(ProcessInfo);
    YoriLibFree(PidList);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the ps builtin command.
 */
#define ENTRYPOINT YoriCmd_YPS
#else
/**
 The main entrypoint for the ps standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the ps cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero indicating success or nonzero on
         failure.
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
    YORI_STRING Arg;
    BOOL DisplayAll;
    PS_CONTEXT PsContext;

    ZeroMemory(&PsContext, sizeof(PsContext));
    DisplayAll = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                PsHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019-2022"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                DisplayAll = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                PsContext.DisplayCommandLine = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                PsContext.DisplayMemory = TRUE;
                ArgumentUnderstood = TRUE;
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

    PsContext.Now.QuadPart = YoriLibGetSystemTimeAsInteger();

    if (DisplayAll) {
        PsDisplayAllProcesses(&PsContext);
    } else {
        PsDisplayConsoleProcesses(&PsContext);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
