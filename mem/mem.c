/**
 * @file mem/mem.c
 *
 * Yori shell display memory usage
 *
 * Copyright (c) 2019 Malcolm J. Smith
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
CHAR strMemHelpText[] =
        "\n"
        "Display memory usage.\n"
        "\n"
        "MEM [-license] [-c [-g]] [<fmt>]\n"
        "\n"
        "   -c             Display memory usage of processes the user has access to\n"
        "   -g             Count all processes with the same name together\n"
        "\n"
        "Format specifiers are:\n"
        "   $AVAILABLECOMMIT$      The amount of memory that the system has available\n"
        "                          for allocation in human friendly format\n"
        "   $AVAILABLECOMMITBYTES$ The amount of memory that the system has available\n"
        "                          for allocation in raw bytes\n"
        "   $AVAILABLEMEM$         The amount of free physical memory in human friendly\n"
        "                          format\n"
        "   $AVAILABLEMEMBYTES$    The amount of free physical memory in raw bytes\n"
        "   $COMMITLIMIT$          The maximum amount of memory the system can allocate\n"
        "                          in human friendly format\n"
        "   $COMMITLIMITBYTES$     The maximum amount of memory the system can allocate\n"
        "                          in raw bytes\n"
        "   $TOTALMEM$             The amount of physical memory in human friendly\n"
        "                          format\n"
        "   $TOTALMEMBYTES$        The amount of physical memory in raw bytes\n";

/**
 Display usage text to the user.
 */
BOOL
MemHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Mem %i.%02i\n"), MEM_VER_MAJOR, MEM_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strMemHelpText);
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
MemOutputLargeInteger(
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
 Context about memory state that is passed between memory query and string
 expansion.
 */
typedef struct _MEM_CONTEXT {

    /**
     The amount of physical memory that the system can address in bytes.
     */
    LARGE_INTEGER TotalPhysical;

    /**
     The amount of physical memory that the system can allocate in bytes.
     */
    LARGE_INTEGER AvailablePhysical;

    /**
     The amount of allocatable memory in bytes.  This includes physical memory
     as well as useable page file memory.
     */
    LARGE_INTEGER TotalCommit;

    /**
     The amount of free allocatable memory in bytes.  This includes physical
     memory as well as useable page file memory.
     */
    LARGE_INTEGER AvailableCommit;

    /**
     The amount of virtual address space in the process.  Note this is a per
     process concept.
     */
    LARGE_INTEGER TotalVirtual;

    /**
     The amount of virtual address space available.  Note this is a per
     process concept.
     */
    LARGE_INTEGER AvailableVirtual;
} MEM_CONTEXT, *PMEM_CONTEXT;

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
DWORD
MemExpandVariables(
    __inout PYORI_STRING OutputBuffer,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    DWORD CharsNeeded;
    PMEM_CONTEXT MemContext = (PMEM_CONTEXT)Context;

    if (YoriLibCompareStringWithLiteral(VariableName, _T("TOTALMEM")) == 0) {
        CharsNeeded = 5;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("AVAILABLEMEM")) == 0) {
        CharsNeeded = 5;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COMMITLIMIT")) == 0) {
        CharsNeeded = 5;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("AVAILABLECOMMIT")) == 0) {
        CharsNeeded = 5;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("TOTALMEMBYTES")) == 0) {
        return MemOutputLargeInteger(MemContext->TotalPhysical, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("AVAILABLEMEMBYTES")) == 0) {
        return MemOutputLargeInteger(MemContext->AvailablePhysical, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COMMITLIMITBYTES")) == 0) {
        return MemOutputLargeInteger(MemContext->TotalCommit, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("AVAILABLECOMMITBYTES")) == 0) {
        return MemOutputLargeInteger(MemContext->AvailableCommit, 10, OutputBuffer);
    } else {
        return 0;
    }

    if (OutputBuffer->LengthAllocated <= CharsNeeded) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringWithLiteral(VariableName, _T("TOTALMEM")) == 0) {
        if (YoriLibFileSizeToString(OutputBuffer, &MemContext->TotalPhysical)) {
            CharsNeeded = 5;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("AVAILABLEMEM")) == 0) {
        if (YoriLibFileSizeToString(OutputBuffer, &MemContext->AvailablePhysical)) {
            CharsNeeded = 5;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("COMMITLIMIT")) == 0) {
        if (YoriLibFileSizeToString(OutputBuffer, &MemContext->TotalCommit)) {
            CharsNeeded = 5;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("AVAILABLECOMMIT")) == 0) {
        if (YoriLibFileSizeToString(OutputBuffer, &MemContext->AvailableCommit)) {
            CharsNeeded = 5;
        }
    }

    return CharsNeeded;
}

/**
 If the user has requested it, go through the set of found processes, and
 merge later entries with the same image name into the first entry.  On
 successful completion, update the count of processes to be reduced by the
 number of merges performed.

 @param ProcessInfo Pointer to a linked list of processes.

 @param NumberOfProcesses Pointer to a count of the number of processes.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MemGroupProcessNames(
    __inout PYORI_SYSTEM_PROCESS_INFORMATION ProcessInfo,
    __inout PDWORD NumberOfProcesses
    )
{
    PYORI_SYSTEM_PROCESS_INFORMATION FirstEntryWithName;
    PYORI_SYSTEM_PROCESS_INFORMATION CurrentEntry;
    PYORI_SYSTEM_PROCESS_INFORMATION PreviousEntry;
    DWORD ProcessCount;
    YORI_STRING PrimaryName;
    YORI_STRING FoundName;

    YoriLibInitEmptyString(&PrimaryName);
    YoriLibInitEmptyString(&FoundName);
    FirstEntryWithName = ProcessInfo;
    ProcessCount = *NumberOfProcesses;

    do {

        PrimaryName.StartOfString = FirstEntryWithName->ImageName;
        PrimaryName.LengthInChars = FirstEntryWithName->ImageNameLengthInBytes / sizeof(WCHAR);

        FirstEntryWithName->ProcessId = 1;
        CurrentEntry = FirstEntryWithName;
        PreviousEntry = CurrentEntry;

        while (CurrentEntry->NextEntryOffset != 0) {

            CurrentEntry = YoriLibAddToPointer(CurrentEntry, CurrentEntry->NextEntryOffset);
            FoundName.StartOfString = CurrentEntry->ImageName;
            FoundName.LengthInChars = CurrentEntry->ImageNameLengthInBytes / sizeof(WCHAR);

            if (YoriLibCompareStringInsensitive(&PrimaryName, &FoundName) == 0) {
                FirstEntryWithName->WorkingSetSize += CurrentEntry->WorkingSetSize;
                FirstEntryWithName->CommitSize += CurrentEntry->CommitSize;
                FirstEntryWithName->ProcessId++;
                if (CurrentEntry->NextEntryOffset == 0) {
                    PreviousEntry->NextEntryOffset = 0;
                } else {
                    PreviousEntry->NextEntryOffset += CurrentEntry->NextEntryOffset;
                }
                ProcessCount--;
            } else {
                PreviousEntry = CurrentEntry;
            }
        }

        if (FirstEntryWithName->NextEntryOffset == 0) {
            break;
        }

        FirstEntryWithName = YoriLibAddToPointer(FirstEntryWithName, FirstEntryWithName->NextEntryOffset);
    } while (TRUE);

    *NumberOfProcesses = ProcessCount;

    return TRUE;
}


/**
 Display the memory used by all processes that the current user has access
 to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MemDisplayProcessMemoryUsage(
    __in BOOLEAN GroupProcesses
    )
{
    PYORI_SYSTEM_PROCESS_INFORMATION ProcessInfo = NULL;
    PYORI_SYSTEM_PROCESS_INFORMATION CurrentEntry;
    DWORD BytesReturned;
    DWORD BytesAllocated;
    DWORD NumberOfProcesses;
    DWORD CurrentProcessIndex;
    PYORI_SYSTEM_PROCESS_INFORMATION *SortedProcesses;
    LONG Status;
    YORI_STRING BaseName;
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

    if (DllNtDll.pNtQuerySystemInformation == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("OS support not present\n"));
        return FALSE;
    }

    BytesAllocated = 0;

    do {

        if (ProcessInfo != NULL) {
            YoriLibFree(ProcessInfo);
        }

        if (BytesAllocated == 0) {
            BytesAllocated = 64 * 1024;
        } else if (BytesAllocated <= 1024 * 1024) {
            BytesAllocated = BytesAllocated * 4;
        } else {
            return FALSE;
        }

        ProcessInfo = YoriLibMalloc(BytesAllocated);
        if (ProcessInfo == NULL) {
            return FALSE;
        }

        Status = DllNtDll.pNtQuerySystemInformation(SystemProcessInformation, ProcessInfo, BytesAllocated, &BytesReturned);
    } while (Status == (LONG)0xc0000004);


    if (Status != 0) {
        YoriLibFree(ProcessInfo);
        return FALSE;
    }

    if (BytesReturned == 0) {
        YoriLibFree(ProcessInfo);
        return FALSE;
    }

    //
    //  Count the number of processes so we can allocate a sort array.
    //

    NumberOfProcesses = 0;
    CurrentEntry = ProcessInfo;
    do {
        NumberOfProcesses++;
        if (CurrentEntry->NextEntryOffset == 0) {
            break;
        }
        CurrentEntry = YoriLibAddToPointer(CurrentEntry, CurrentEntry->NextEntryOffset);
    } while(TRUE);

    //
    //  If the user asked to combine all processes with the same name, do the
    //  combining now.
    //

    if (GroupProcesses) {
        MemGroupProcessNames(ProcessInfo, &NumberOfProcesses);
    }

    //
    //  Allocate the sort array.
    //

    SortedProcesses = YoriLibMalloc(NumberOfProcesses * sizeof(PYORI_SYSTEM_PROCESS_INFORMATION));
    if (SortedProcesses == NULL) {
        YoriLibFree(ProcessInfo);
        return FALSE;
    }

    //
    //  Go through the list of processes and calculate it into sorted order.
    //

    NumberOfProcesses = 0;
    CurrentEntry = ProcessInfo;
    SortedProcesses[0] = CurrentEntry;
    do {

        NumberOfProcesses++;
        if (CurrentEntry->NextEntryOffset == 0) {
            break;
        }
        CurrentEntry = YoriLibAddToPointer(CurrentEntry, CurrentEntry->NextEntryOffset);

        CurrentProcessIndex = NumberOfProcesses;
        do {
            if (SortedProcesses[CurrentProcessIndex - 1]->WorkingSetSize + SortedProcesses[CurrentProcessIndex - 1]->CommitSize < CurrentEntry->WorkingSetSize + CurrentEntry->CommitSize) {
                SortedProcesses[CurrentProcessIndex] = SortedProcesses[CurrentProcessIndex - 1];
            } else {
                SortedProcesses[CurrentProcessIndex] = CurrentEntry;
                break;
            }
            CurrentProcessIndex--;
            if (CurrentProcessIndex == 0) {
                SortedProcesses[CurrentProcessIndex] = CurrentEntry;
            }
        } while(CurrentProcessIndex > 0);
    } while(TRUE);

    YoriLibInitEmptyString(&BaseName);

    //
    //  Now display the result in sorted order
    //

    if (GroupProcesses) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T(" Count | Process         | WorkingSet | Commit\n"));
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Pid  | Process         | WorkingSet | Commit\n"));
    }
    for (CurrentProcessIndex = 0; CurrentProcessIndex < NumberOfProcesses; CurrentProcessIndex++) {
        CurrentEntry = SortedProcesses[CurrentProcessIndex];

        //
        //  Hack to fix formats
        //

        liCommit.QuadPart = CurrentEntry->CommitSize;
        liWorkingSet.QuadPart = CurrentEntry->WorkingSetSize;
        YoriLibFileSizeToString(&CommitString, &liCommit);
        YoriLibFileSizeToString(&WorkingSetString, &liWorkingSet);

        BaseName.StartOfString = CurrentEntry->ImageName;
        BaseName.LengthInChars = CurrentEntry->ImageNameLengthInBytes / sizeof(WCHAR);

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-6i | %-15y | %-10y | %y\n"), CurrentEntry->ProcessId, &BaseName, &WorkingSetString, &CommitString);

    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));

    YoriLibFree(ProcessInfo);
    YoriLibFree(SortedProcesses);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the mem builtin command.
 */
#define ENTRYPOINT YoriCmd_YMEM
#else
/**
 The main entrypoint for the mem standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the mem cmdlet.

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
    BOOLEAN DisplayProcesses = FALSE;
    BOOLEAN GroupProcesses = FALSE;
    YORI_STRING Arg;
    MEM_CONTEXT MemContext;
    YORI_STRING DisplayString;
    YORI_STRING AllocatedFormatString;
    LPTSTR DefaultFormatString = _T("Total Physical: $TOTALMEM$\n")
                                 _T("Available Physical: $AVAILABLEMEM$\n")
                                 _T("Commit Limit: $COMMITLIMIT$\n")
                                 _T("Available Commit: $AVAILABLECOMMIT$\n");

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                MemHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                DisplayProcesses = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("g")) == 0) {
                GroupProcesses = TRUE;
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

    if (StartArg > 0) {
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, &AllocatedFormatString)) {
            return EXIT_FAILURE;
        }
    } else {
        YoriLibConstantString(&AllocatedFormatString, DefaultFormatString);
    }

    if (DisplayProcesses) {
        MemDisplayProcessMemoryUsage(GroupProcesses);
    }

    if (DllKernel32.pGlobalMemoryStatusEx) {
        YORI_MEMORYSTATUSEX MemStatusEx;
        MemStatusEx.dwLength = sizeof(MemStatusEx);
        if (!DllKernel32.pGlobalMemoryStatusEx(&MemStatusEx)) {
            DWORD Err = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(Err);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Query of memory failed: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            YoriLibFreeStringContents(&AllocatedFormatString);
            return EXIT_FAILURE;
        }

        MemContext.TotalPhysical.QuadPart = MemStatusEx.ullTotalPhys;
        MemContext.AvailablePhysical.QuadPart = MemStatusEx.ullAvailPhys;
        MemContext.TotalCommit.QuadPart = MemStatusEx.ullTotalPageFile;
        MemContext.AvailableCommit.QuadPart = MemStatusEx.ullAvailPageFile;
        MemContext.TotalVirtual.QuadPart = MemStatusEx.ullTotalVirtual;
        MemContext.AvailableVirtual.QuadPart = MemStatusEx.ullAvailVirtual;
    } else {
        MEMORYSTATUS MemStatus;
        //
        //  Warning about using a deprecated function and how we should use
        //  GlobalMemoryStatusEx instead.  The analyzer isn't smart enough to
        //  notice that when it's available, that's what we do.
        //
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 28159)
#endif
        GlobalMemoryStatus(&MemStatus);
        MemContext.TotalPhysical.QuadPart = MemStatus.dwTotalPhys;
        MemContext.AvailablePhysical.QuadPart = MemStatus.dwAvailPhys;
        MemContext.TotalCommit.QuadPart = MemStatus.dwTotalPageFile;
        MemContext.AvailableCommit.QuadPart = MemStatus.dwAvailPageFile;
        MemContext.TotalVirtual.QuadPart = MemStatus.dwTotalVirtual;
        MemContext.AvailableVirtual.QuadPart = MemStatus.dwAvailVirtual;
    }


    YoriLibInitEmptyString(&DisplayString);
    YoriLibExpandCommandVariables(&AllocatedFormatString, '$', FALSE, MemExpandVariables, &MemContext, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        YoriLibFreeStringContents(&DisplayString);
    }
    YoriLibFreeStringContents(&AllocatedFormatString);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
