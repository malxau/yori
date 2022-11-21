/**
 * @file lib/process.c
 *
 * Yori process enumeration support routines
 *
 * Copyright (c) 2018-2020 Malcolm J. Smith
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
 Load information about all processes currently executing in the system.

 @param ProcessInfo On successful completion, updated to point to a list of
        processes executing within the system.  The caller is expected to
        free this with YoriLibFree.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibGetSystemProcessList(
    __out PYORI_SYSTEM_PROCESS_INFORMATION *ProcessInfo
    )
{
    PYORI_SYSTEM_PROCESS_INFORMATION LocalProcessInfo = NULL;
    DWORD BytesReturned;
    DWORD BytesAllocated;
    LONG Status;

    if (DllNtDll.pNtQuerySystemInformation == NULL) {
        return FALSE;
    }

    BytesAllocated = 0;

    do {

        if (LocalProcessInfo != NULL) {
            YoriLibFree(LocalProcessInfo);
        }

        if (BytesAllocated == 0) {
            BytesAllocated = 64 * 1024;
        } else if (BytesAllocated <= 16 * 1024 * 1024) {
            BytesAllocated = BytesAllocated * 4;
        } else {
            return FALSE;
        }

        LocalProcessInfo = YoriLibMalloc(BytesAllocated);
        if (LocalProcessInfo == NULL) {
            return FALSE;
        }

        Status = DllNtDll.pNtQuerySystemInformation(SystemProcessInformation, LocalProcessInfo, BytesAllocated, &BytesReturned);
    } while (Status == STATUS_INFO_LENGTH_MISMATCH);


    if (Status != 0) {
        YoriLibFree(LocalProcessInfo);
        return FALSE;
    }

    if (BytesReturned == 0) {
        YoriLibFree(LocalProcessInfo);
        return FALSE;
    }

    *ProcessInfo = LocalProcessInfo;
    return TRUE;
}

/**
 Load information about all handles currently open in the system.

 @param HandlesInfo On successful completion, updated to point to a list of
        handles open within the system.  The caller is expected to free this
        with YoriLibFree.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibGetSystemHandlesList(
    __out PYORI_SYSTEM_HANDLE_INFORMATION_EX *HandlesInfo
    )
{
    PYORI_SYSTEM_HANDLE_INFORMATION_EX LocalHandlesInfo = NULL;
    DWORD BytesReturned;
    DWORD BytesAllocated;
    LONG Status;

    if (DllNtDll.pNtQuerySystemInformation == NULL) {
        return FALSE;
    }

    BytesAllocated = 0;

    do {

        if (LocalHandlesInfo != NULL) {
            YoriLibFree(LocalHandlesInfo);
        }

        if (BytesAllocated == 0) {
            BytesAllocated = 64 * 1024;
        } else if (BytesAllocated <= 16 * 1024 * 1024) {
            BytesAllocated = BytesAllocated * 4;
        } else {
            return FALSE;
        }

        LocalHandlesInfo = YoriLibMalloc(BytesAllocated);
        if (LocalHandlesInfo == NULL) {
            return FALSE;
        }

        Status = DllNtDll.pNtQuerySystemInformation(SystemExtendedHandleInformation, LocalHandlesInfo, BytesAllocated, &BytesReturned);
    } while (Status == STATUS_INFO_LENGTH_MISMATCH);


    if (Status != 0) {
        YoriLibFree(LocalHandlesInfo);
        return FALSE;
    }

    if (BytesReturned == 0) {
        YoriLibFree(LocalHandlesInfo);
        return FALSE;
    }

    *HandlesInfo = LocalHandlesInfo;
    return TRUE;
}

// vim:sw=4:ts=4:et:
