/**
 * @file lib/cpuinfo.c
 *
 * Yori CPU query routines
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

#include "yoripch.h"
#include "yorilib.h"


/**
 Query the system to find the number of high performance and high efficiency
 logical processors.  This is used when determining how many child tasks to
 execute.  On older systems (or processors!) this just returns the total 
 number of processors as performance processors.

 @param PerformanceLogicalProcessors On successful completion, updated to
        indicate the number of high performance processors.

 @param EfficiencyLogicalProcessors On successful completion, updated to
        indicate the number of high efficiency processors.
 */
VOID
YoriLibQueryCpuCount(
    __out PWORD PerformanceLogicalProcessors,
    __out PWORD EfficiencyLogicalProcessors
    )
{
    YORI_SYSTEM_INFO SysInfo;

    //
    //  If this function doesn't exist, the system is incapable of performing
    //  different scheduling on different types of processor, so just return
    //  the total number of processors (at the end of this function.)
    //

    if (DllKernel32.pGetLogicalProcessorInformationEx != NULL) {
        DWORD Err;
        DWORD BytesInBuffer = 0;
        DWORD CurrentOffset = 0;
        DWORD GroupIndex;
        WORD LocalEfficiencyProcessorCount;
        WORD LocalPerformanceProcessorCount;
        WORD LogicalProcessorCount;
        WORD LogicalProcessorIndex;
        DWORD_PTR LogicalProcessorMask;
        PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ProcInfo = NULL;
        PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry;
        PYORI_PROCESSOR_GROUP_AFFINITY Group;
    
        //
        //  Query processor information from the system.  This needs to allocate
        //  memory as needed to populate, so loop while the buffer is too small
        //  in order to allocate the correct amount.
        //
    
        Err = ERROR_SUCCESS;
        while(TRUE) {
            if (DllKernel32.pGetLogicalProcessorInformationEx(YoriProcessorRelationAll, ProcInfo, &BytesInBuffer)) {
                Err = ERROR_SUCCESS;
                break;
            }
    
            Err = GetLastError();
            if (Err == ERROR_INSUFFICIENT_BUFFER) {
                if (ProcInfo != NULL) {
                    YoriLibFree(ProcInfo);
                    ProcInfo = NULL;
                }

                if (!YoriLibIsSizeAllocatable(BytesInBuffer)) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
    
                ProcInfo = YoriLibMalloc((YORI_ALLOC_SIZE_T)BytesInBuffer);
                if (ProcInfo == NULL) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            } else {
                break;
            }
        }
    
        if (Err == ERROR_SUCCESS) {
            LocalEfficiencyProcessorCount = 0;
            LocalPerformanceProcessorCount = 0;
            Entry = ProcInfo;
            while (Entry != NULL) {
                if (Entry->Relationship == YoriProcessorRelationProcessorCore) {

                    //
                    //  Count how many logical processors are implemented by
                    //  this core.
                    //

                    LogicalProcessorCount = 0;
                    for (GroupIndex = 0; GroupIndex < Entry->u.Processor.GroupCount; GroupIndex++) {
                        Group = &Entry->u.Processor.GroupMask[GroupIndex];
                        for (LogicalProcessorIndex = 0; LogicalProcessorIndex < 8 * sizeof(DWORD_PTR); LogicalProcessorIndex++) {
                            LogicalProcessorMask = 1;
                            LogicalProcessorMask = LogicalProcessorMask<<LogicalProcessorIndex;
                            if (Group->Mask & LogicalProcessorMask) {
                                LogicalProcessorCount++;
                            }
                        }
                    }
                    if (Entry->u.Processor.EfficiencyClass == 0) {
                        LocalEfficiencyProcessorCount = (WORD)(LocalEfficiencyProcessorCount + LogicalProcessorCount);
                    } else {
                        LocalPerformanceProcessorCount = (WORD)(LocalPerformanceProcessorCount + LogicalProcessorCount);
                    }
                }
                CurrentOffset += Entry->SizeInBytes;
                if (CurrentOffset >= BytesInBuffer) {
                    break;
                }
                Entry = YoriLibAddToPointer(ProcInfo, CurrentOffset);
            }

            if (ProcInfo != NULL) {
                YoriLibFree(ProcInfo);
            }

            //
            //  On homogenous systems, all cores will report efficiency class
            //  zero, which is the most efficient class.  For human
            //  compatibility, report these as performance cores instead.
            //

            if (LocalPerformanceProcessorCount == 0) {
                LocalPerformanceProcessorCount = LocalEfficiencyProcessorCount;
                LocalEfficiencyProcessorCount = 0;
            }

            //
            //  If this counted any processors, return them.  Otherwise fall
            //  back to the legacy approach of not distinguishing processor
            //  classes.
            //

            if (LocalPerformanceProcessorCount != 0) {
                *PerformanceLogicalProcessors = LocalPerformanceProcessorCount;
                *EfficiencyLogicalProcessors = LocalEfficiencyProcessorCount;
                return;
            }
        }
    }

    //
    //  Ultimate fallback: just return the processor count.  Note
    //  GetSystemInfo cannot fail.
    //

    GetSystemInfo((LPSYSTEM_INFO)&SysInfo);
    *PerformanceLogicalProcessors = (WORD)SysInfo.dwNumberOfProcessors;
    *EfficiencyLogicalProcessors = 0;
}

// vim:sw=4:ts=4:et:
