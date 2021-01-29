/**
 * @file cpuinfo/cpuinfo.c
 *
 * Yori shell display cpu topology information
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
CHAR strCpuInfoHelpText[] =
        "\n"
        "Display cpu topology information.\n"
        "\n"
        "CPUINFO [-license] [-a] [-c] [-g] [-n] [-s] [<fmt>]\n"
        "\n"
        "   -a             Display all information\n"
        "   -c             Display information about processor cores\n"
        "   -g             Display information about processor groups\n"
        "   -n             Display information about NUMA nodes\n"
        "   -s             Display information about processor sockets\n"
        "\n"
        "Format specifiers are:\n"
        "   $CORECOUNT$            The number of processor cores\n"
        "   $GROUPCOUNT$           The number of processor groups\n"
        "   $NUMANODECOUNT$        The number of NUMA nodes\n"
        "   $LOGICALCOUNT$         The number of logical processors\n";

/**
 Display usage text to the user.
 */
BOOL
CpuInfoHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("CpuInfo %i.%02i\n"), CPUINFO_VER_MAJOR, CPUINFO_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strCpuInfoHelpText);
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
CpuInfoOutputLargeInteger(
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
 Context about cpuinfoory state that is passed between cpuinfoory query and string
 expansion.
 */
typedef struct _CPUINFO_CONTEXT {

    /**
     The number of bytes in the ProcInfo allocation.
     */
    DWORD BytesInBuffer;

    /**
     An array of entries describing information about the current system.
     */
    PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ProcInfo;

    /**
     The total number of processor cores discovered in the above information.
     */
    LARGE_INTEGER CoreCount;

    /**
     The total number of logical processors discovered in the above
     information.
     */
    LARGE_INTEGER LogicalProcessorCount;

    /**
     The total number of NUMA nodes discovered in the above information.
     */
    LARGE_INTEGER NumaNodeCount;

    /**
     The total number of processor groups discovered in the above information.
     */
    LARGE_INTEGER GroupCount;

} CPUINFO_CONTEXT, *PCPUINFO_CONTEXT;

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
CpuInfoExpandVariables(
    __inout PYORI_STRING OutputBuffer,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    DWORD CharsNeeded;
    PCPUINFO_CONTEXT CpuInfoContext = (PCPUINFO_CONTEXT)Context;

    if (YoriLibCompareStringWithLiteral(VariableName, _T("CORECOUNT")) == 0) {
        return CpuInfoOutputLargeInteger(CpuInfoContext->CoreCount, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("GROUPCOUNT")) == 0) {
        return CpuInfoOutputLargeInteger(CpuInfoContext->GroupCount, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("LOGICALCOUNT")) == 0) {
        return CpuInfoOutputLargeInteger(CpuInfoContext->LogicalProcessorCount, 10, OutputBuffer);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("NUMANODECOUNT")) == 0) {
        return CpuInfoOutputLargeInteger(CpuInfoContext->NumaNodeCount, 10, OutputBuffer);
    } else {
        return 0;
    }

    return CharsNeeded;
}


//
//  The CPUINFO_CONTEXT structure records a pointer to an array and the size
//  of the array in bytes.  Unfortunately I can't see a way to describe this
//  relationship for older analyzers, so they believe accessing array elements
//  is walking off the end of the buffer.  Because this seems specific to
//  older versions, the suppression is limited to those also.
//

#if defined(_MSC_VER) && (_MSC_VER >= 1500) && (_MSC_VER <= 1600)
#pragma warning(push)
#pragma warning(disable: 6385)
#endif

/**
 Parse the array of information about processor topologies and count the
 number of elements in each.

 @param CpuInfoContext Pointer to the context which describes the processor
        information verbosely and should have summary counts populated.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CpuInfoCountSummaries(
    __inout PCPUINFO_CONTEXT CpuInfoContext
    )
{
    PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry;
    DWORD CurrentOffset = 0;
    DWORD GroupIndex;
    DWORD LogicalProcessorIndex;
    DWORD_PTR LogicalProcessorMask;
    PYORI_PROCESSOR_GROUP_AFFINITY Group;

    Entry = CpuInfoContext->ProcInfo;

    while (Entry != NULL) {

        if (Entry->Relationship == YoriProcessorRelationProcessorCore) {
            CpuInfoContext->CoreCount.QuadPart++;
            for (GroupIndex = 0; GroupIndex < Entry->u.Processor.GroupCount; GroupIndex++) {
                Group = &Entry->u.Processor.GroupMask[GroupIndex];
                for (LogicalProcessorIndex = 0; LogicalProcessorIndex < 8 * sizeof(DWORD_PTR); LogicalProcessorIndex++) {
                    LogicalProcessorMask = 1;
                    LogicalProcessorMask = LogicalProcessorMask<<LogicalProcessorIndex;
                    if (Group->Mask & LogicalProcessorMask) {
                        CpuInfoContext->LogicalProcessorCount.QuadPart++;
                    }
                }
            }
        } else if (Entry->Relationship == YoriProcessorRelationNumaNode) {
            CpuInfoContext->NumaNodeCount.QuadPart++;
        } else if (Entry->Relationship == YoriProcessorRelationGroup) {
            CpuInfoContext->GroupCount.QuadPart = Entry->u.Group.ActiveGroupCount;
        }

        CurrentOffset += Entry->SizeInBytes;
        if (CurrentOffset >= CpuInfoContext->BytesInBuffer) {
            break;
        }
        Entry = YoriLibAddToPointer(CpuInfoContext->ProcInfo, CurrentOffset);
    }

    return TRUE;
}

/**
 Display a list of logical processor numbers.

 @param GroupIndex The group that contains these logical processors.

 @param Processors A bitmask of processors.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CpuInfoDisplayProcessorMask(
    __in WORD GroupIndex,
    __in DWORD_PTR Processors
    )
{
    DWORD LogicalProcessorIndex;
    DWORD_PTR LogicalProcessorMask;

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Processors: "));

    for (LogicalProcessorIndex = 0; LogicalProcessorIndex < 8 * sizeof(DWORD_PTR); LogicalProcessorIndex++) {
        LogicalProcessorMask = 1;
        LogicalProcessorMask = LogicalProcessorMask<<LogicalProcessorIndex;
        if (Processors & LogicalProcessorMask) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%i "), GroupIndex * 64 + LogicalProcessorIndex);
        }
    }
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));

    return TRUE;
}

/**
 Display processors within each processor core.

 @param CpuInfoContext Pointer to the context containing processor layout.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CpuInfoDisplayCores(
    __in PCPUINFO_CONTEXT CpuInfoContext
    )
{
    PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry;
    DWORD CurrentOffset = 0;
    DWORD CoreIndex = 0;
    DWORD GroupIndex;

    Entry = CpuInfoContext->ProcInfo;

    while (Entry != NULL) {

        if (Entry->Relationship == YoriProcessorRelationProcessorCore) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Core %i\n"), CoreIndex);
            CoreIndex++;
            for (GroupIndex = 0; GroupIndex < Entry->u.Processor.GroupCount; GroupIndex++) {
                CpuInfoDisplayProcessorMask(Entry->u.Processor.GroupMask[GroupIndex].Group, Entry->u.Processor.GroupMask[GroupIndex].Mask);
            }

            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }

        CurrentOffset += Entry->SizeInBytes;
        if (CurrentOffset >= CpuInfoContext->BytesInBuffer) {
            break;
        }
        Entry = YoriLibAddToPointer(CpuInfoContext->ProcInfo, CurrentOffset);
    }

    return TRUE;
}

/**
 Display processors within each processor group.

 @param CpuInfoContext Pointer to the context containing processor layout.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CpuInfoDisplayGroups(
    __in PCPUINFO_CONTEXT CpuInfoContext
    )
{
    PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry;
    DWORD CurrentOffset = 0;
    WORD GroupIndex;

    Entry = CpuInfoContext->ProcInfo;

    while (Entry != NULL) {

        if (Entry->Relationship == YoriProcessorRelationGroup) {
            for (GroupIndex = 0; GroupIndex < Entry->u.Group.MaximumGroupCount; GroupIndex++) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Group %i\n"), GroupIndex);
                CpuInfoDisplayProcessorMask(GroupIndex, Entry->u.Group.GroupInfo[GroupIndex].ActiveProcessorMask);

            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }

        CurrentOffset += Entry->SizeInBytes;
        if (CurrentOffset >= CpuInfoContext->BytesInBuffer) {
            break;
        }
        Entry = YoriLibAddToPointer(CpuInfoContext->ProcInfo, CurrentOffset);
    }

    return TRUE;
}

/**
 Display processors within each NUMA node.

 @param CpuInfoContext Pointer to the context containing processor layout.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CpuInfoDisplayNuma(
    __in PCPUINFO_CONTEXT CpuInfoContext
    )
{
    PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry;
    DWORD CurrentOffset = 0;

    Entry = CpuInfoContext->ProcInfo;

    while (Entry != NULL) {

        if (Entry->Relationship == YoriProcessorRelationNumaNode) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Numa Node %i\n"), Entry->u.NumaNode.NodeNumber);
            CpuInfoDisplayProcessorMask(Entry->u.NumaNode.GroupMask.Group, Entry->u.NumaNode.GroupMask.Mask);
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }

        CurrentOffset += Entry->SizeInBytes;
        if (CurrentOffset >= CpuInfoContext->BytesInBuffer) {
            break;
        }
        Entry = YoriLibAddToPointer(CpuInfoContext->ProcInfo, CurrentOffset);
    }

    return TRUE;
}

/**
 Display processors within each processor package.

 @param CpuInfoContext Pointer to the context containing processor layout.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CpuInfoDisplaySockets(
    __in PCPUINFO_CONTEXT CpuInfoContext
    )
{
    PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry;
    DWORD CurrentOffset = 0;
    DWORD SocketIndex = 0;
    DWORD GroupIndex;

    Entry = CpuInfoContext->ProcInfo;

    while (Entry != NULL) {

        if (Entry->Relationship == YoriProcessorRelationProcessorPackage) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Socket %i\n"), SocketIndex);
            SocketIndex++;
            for (GroupIndex = 0; GroupIndex < Entry->u.Processor.GroupCount; GroupIndex++) {
                CpuInfoDisplayProcessorMask(Entry->u.Processor.GroupMask[GroupIndex].Group, Entry->u.Processor.GroupMask[GroupIndex].Mask);
            }

            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }

        CurrentOffset += Entry->SizeInBytes;
        if (CurrentOffset >= CpuInfoContext->BytesInBuffer) {
            break;
        }
        Entry = YoriLibAddToPointer(CpuInfoContext->ProcInfo, CurrentOffset);
    }

    return TRUE;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1500) && (_MSC_VER <= 1600)
#pragma warning(pop)
#endif

/**
 Load processor information from GetLogicalProcessorInformationEx.

 @param CpuInfoContext Pointer to the context to populate with processor
        information.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CpuInfoLoadProcessorInfo(
    __inout PCPUINFO_CONTEXT CpuInfoContext
    )
{
    DWORD Err;

    //
    //  Query processor information from the system.  This needs to allocate
    //  memory as needed to populate, so loop while the buffer is too small
    //  in order to allocate the correct amount.
    //

    Err = ERROR_SUCCESS;
    while(TRUE) {
        if (DllKernel32.pGetLogicalProcessorInformationEx(YoriProcessorRelationAll, CpuInfoContext->ProcInfo, &CpuInfoContext->BytesInBuffer)) {
            Err = ERROR_SUCCESS;
            break;
        }

        Err = GetLastError();
        if (Err == ERROR_INSUFFICIENT_BUFFER) {
            if (CpuInfoContext->ProcInfo != NULL) {
                YoriLibFree(CpuInfoContext->ProcInfo);
                CpuInfoContext->ProcInfo = NULL;
            }

            CpuInfoContext->ProcInfo = YoriLibMalloc(CpuInfoContext->BytesInBuffer);
            if (CpuInfoContext->ProcInfo == NULL) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        } else {
            break;
        }
    }

    //
    //  If the above query failed, indicate why.
    //

    if (Err != ERROR_SUCCESS) {
        LPTSTR ErrText;
        if (CpuInfoContext->ProcInfo != NULL) {
            YoriLibFree(CpuInfoContext->ProcInfo);
        }
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Query failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    return TRUE;
}

/**
 Load processor information from GetLogicalProcessorInformation and translate
 the result into the output that would have come from
 GetLogicalProcessorInformationEx.  This is only done on systems without
 GetLogicalProcessorInformationEx, meaning they only have a single processor
 group.

 @param CpuInfoContext Pointer to the context to populate with processor
        information.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CpuInfoLoadAndUpconvertProcessorInfo(
    __inout PCPUINFO_CONTEXT CpuInfoContext
    )
{
    DWORD Err;
    DWORD BytesInBuffer = 0;
    DWORD BytesRequired;
    DWORD CurrentOffset;
    DWORD NewOffset;
    DWORD_PTR ProcessorsFound;
    DWORD_PTR ProcessorMask;
    PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION ProcInfo = NULL;
    PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION Entry;
    PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX NewEntry;

    //
    //  Query processor information from the system.  This needs to allocate
    //  memory as needed to populate, so loop while the buffer is too small
    //  in order to allocate the correct amount.
    //

    Err = ERROR_SUCCESS;
    while(TRUE) {
        if (DllKernel32.pGetLogicalProcessorInformation(ProcInfo, &BytesInBuffer)) {
            Err = ERROR_SUCCESS;
            break;
        }

        Err = GetLastError();
        if (Err == ERROR_INSUFFICIENT_BUFFER) {
            if (ProcInfo != NULL) {
                YoriLibFree(ProcInfo);
                ProcInfo = NULL;
            }

            ProcInfo = YoriLibMalloc(BytesInBuffer);
            if (ProcInfo == NULL) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        } else {
            break;
        }
    }

    //
    //  If the above query failed, indicate why.
    //

    if (Err != ERROR_SUCCESS) {
        LPTSTR ErrText;
        if (ProcInfo != NULL) {
            YoriLibFree(ProcInfo);
        }
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Query failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    if (ProcInfo == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("cpuinfo: no processors\n"));
        return FALSE;
    }

    //
    //  Count the amount of memory needed for the full structures.
    //

    BytesRequired = 0;
    CurrentOffset = 0;
    Entry = ProcInfo;

    while (Entry != NULL) {

        if (Entry->Relationship == YoriProcessorRelationProcessorCore) {
            BytesRequired += sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
        } else if (Entry->Relationship == YoriProcessorRelationNumaNode) {
            BytesRequired += sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
        } else if (Entry->Relationship == YoriProcessorRelationCache) {
            BytesRequired += sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
        } else if (Entry->Relationship == YoriProcessorRelationProcessorPackage) {
            BytesRequired += sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
        }

        CurrentOffset += sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        if (CurrentOffset >= BytesInBuffer) {
            break;
        }
        Entry = YoriLibAddToPointer(ProcInfo, CurrentOffset);
    }

    //
    //  An extra one for the group information, which can only have one group.
    //

    BytesRequired += sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);

    CpuInfoContext->BytesInBuffer = BytesRequired;
    CpuInfoContext->ProcInfo = YoriLibMalloc(BytesRequired);
    if (CpuInfoContext->ProcInfo == NULL) {
        YoriLibFree(ProcInfo);
        return FALSE;
    }

    ZeroMemory(CpuInfoContext->ProcInfo, BytesRequired);

    //
    //  Port over the existing downlevel entries to the new format.
    //

    NewOffset = 0;
    CurrentOffset = 0;
    ProcessorsFound = 0;
    Entry = ProcInfo;
    NewEntry = CpuInfoContext->ProcInfo;

    //
    //  The first entry is for processor groups, but we don't know anything
    //  about them yet.  Reserve this entry and skip it.
    //

    NewEntry->Relationship = YoriProcessorRelationGroup;
    NewEntry->SizeInBytes = sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
    NewEntry->u.Group.MaximumGroupCount = 1;
    NewEntry->u.Group.ActiveGroupCount = 1;
    NewOffset += NewEntry->SizeInBytes;
    NewEntry = YoriLibAddToPointer(CpuInfoContext->ProcInfo, NewOffset);

    while (Entry != NULL) {

        if (Entry->Relationship == YoriProcessorRelationProcessorCore) {
            NewEntry->Relationship = YoriProcessorRelationProcessorCore;
            NewEntry->SizeInBytes = sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
            NewEntry->u.Processor.Flags = Entry->u.ProcessorCore.Flags;
            NewEntry->u.Processor.GroupCount = 1;
            NewEntry->u.Processor.GroupMask[0].Mask = Entry->ProcessorMask;
            ProcessorsFound |= Entry->ProcessorMask;
        } else if (Entry->Relationship == YoriProcessorRelationNumaNode) {
            NewEntry->Relationship = YoriProcessorRelationNumaNode;
            NewEntry->SizeInBytes = sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
            NewEntry->u.NumaNode.NodeNumber = Entry->u.NumaNode.NodeNumber;
            NewEntry->u.NumaNode.GroupMask.Mask = Entry->ProcessorMask;

        } else if (Entry->Relationship == YoriProcessorRelationCache) {
            NewEntry->Relationship = YoriProcessorRelationCache;
            NewEntry->SizeInBytes = sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
            memcpy(&NewEntry->u.Cache.Cache, &Entry->u.Cache, sizeof(YORI_PROCESSOR_CACHE_DESCRIPTOR));
            NewEntry->u.Cache.GroupMask.Mask = Entry->ProcessorMask;
        } else if (Entry->Relationship == YoriProcessorRelationProcessorPackage) {
            NewEntry->Relationship = YoriProcessorRelationProcessorPackage;
            NewEntry->SizeInBytes = sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
            NewEntry->u.Processor.Flags = Entry->u.ProcessorCore.Flags;
            NewEntry->u.Processor.GroupCount = 1;
            NewEntry->u.Processor.GroupMask[0].Mask = Entry->ProcessorMask;
        }

        CurrentOffset += sizeof(YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        NewOffset += NewEntry->SizeInBytes;
        if (CurrentOffset >= BytesInBuffer) {
            break;
        }
        Entry = YoriLibAddToPointer(ProcInfo, CurrentOffset);
        NewEntry = YoriLibAddToPointer(CpuInfoContext->ProcInfo, NewOffset);
    }

    //
    //  Now populate group information.
    //

    NewEntry = CpuInfoContext->ProcInfo;
    NewEntry->u.Group.GroupInfo[0].ActiveProcessorMask = ProcessorsFound;

    ProcessorMask = 1;
    for (CurrentOffset = 0; CurrentOffset < (sizeof(DWORD_PTR) * 8); CurrentOffset++) {
        ProcessorMask = ProcessorMask<<1;
        if (ProcessorsFound & ProcessorMask) {
            NewEntry->u.Group.GroupInfo[0].MaximumProcessorCount++;
            NewEntry->u.Group.GroupInfo[0].ActiveProcessorCount++;
        }
    }

    YoriLibFree(ProcInfo);

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the cpuinfo builtin command.
 */
#define ENTRYPOINT YoriCmd_YCPUINFO
#else
/**
 The main entrypoint for the cpuinfo standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the cpuinfo cmdlet.

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
    BOOLEAN DisplayCores = FALSE;
    BOOLEAN DisplayGroups = FALSE;
    BOOLEAN DisplayNuma = FALSE;
    BOOLEAN DisplaySockets = FALSE;
    YORI_STRING Arg;
    CPUINFO_CONTEXT CpuInfoContext;
    YORI_STRING DisplayString;
    YORI_STRING AllocatedFormatString;
    LPTSTR DefaultFormatString = _T("Core count: $CORECOUNT$\n")
                                 _T("Group count: $GROUPCOUNT$\n")
                                 _T("Logical processors: $LOGICALCOUNT$\n")
                                 _T("Numa nodes: $NUMANODECOUNT$\n");

    ZeroMemory(&CpuInfoContext, sizeof(CpuInfoContext));
    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                CpuInfoHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                DisplayCores = TRUE;
                DisplayGroups = TRUE;
                DisplayNuma = TRUE;
                DisplaySockets = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                DisplayCores = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("g")) == 0) {
                DisplayGroups = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                DisplayNuma = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                DisplaySockets = TRUE;
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

    //
    //  If the Win7 API is not present, should fall back to the 2003 API and
    //  emulate the Win7 one.  If neither are present this app can't produce
    //  meaningful output.
    //

    if (DllKernel32.pGetLogicalProcessorInformationEx == NULL &&
        DllKernel32.pGetLogicalProcessorInformation == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("OS support not present\n"));
        return EXIT_FAILURE;
    } else if (DllKernel32.pGetLogicalProcessorInformationEx != NULL) {
        if (!CpuInfoLoadProcessorInfo(&CpuInfoContext)) {
            return EXIT_FAILURE;
        }
    } else {
        if (!CpuInfoLoadAndUpconvertProcessorInfo(&CpuInfoContext)) {
            return EXIT_FAILURE;
        }
    }

    //
    //  Parse the processor information into summary counts.
    //

    CpuInfoCountSummaries(&CpuInfoContext);

    if (DisplayCores) {
        CpuInfoDisplayCores(&CpuInfoContext);
    }

    if (DisplayGroups) {
        CpuInfoDisplayGroups(&CpuInfoContext);
    }

    if (DisplayNuma) {
        CpuInfoDisplayNuma(&CpuInfoContext);
    }

    if (DisplaySockets) {
        CpuInfoDisplaySockets(&CpuInfoContext);
    }

    //
    //  Obtain a format string.
    //

    if (StartArg > 0) {
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, FALSE, &AllocatedFormatString)) {
            YoriLibFree(CpuInfoContext.ProcInfo);
            return EXIT_FAILURE;
        }
    } else {
        YoriLibConstantString(&AllocatedFormatString, DefaultFormatString);
    }

    //
    //  Output the format string with summary counts.
    //

    YoriLibInitEmptyString(&DisplayString);
    YoriLibExpandCommandVariables(&AllocatedFormatString, '$', FALSE, CpuInfoExpandVariables, &CpuInfoContext, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        YoriLibFreeStringContents(&DisplayString);
    }
    YoriLibFreeStringContents(&AllocatedFormatString);
    YoriLibFree(CpuInfoContext.ProcInfo);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
