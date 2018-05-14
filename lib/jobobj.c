/**
 * @file lib/jobobj.c
 *
 * Yori wrappers around Windows Job Object functionality.  Loads dynamically
 * to allow for fallback if the host OS doesn't support it.
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Attempt to create an unnamed job object.  If the functionality is not
 supported by the host OS, returns NULL.

 @return Handle to a job object, or NULL on failure.
 */
HANDLE
YoriLibCreateJobObject(
    )
{
    if (Kernel32.pCreateJobObjectW == NULL) {
        return NULL;
    }
    return Kernel32.pCreateJobObjectW(NULL, NULL);
}

/**
 Assign a process to a job object.  If the functionality is not
 supported by the host OS, returns FALSE.

 @return TRUE on success, FALSE on failure.
 */
BOOL
YoriLibAssignProcessToJobObject(
    __in HANDLE hJob,
    __in HANDLE hProcess
    )
{
    if (Kernel32.pAssignProcessToJobObject == NULL) {
        return FALSE;
    }
    return Kernel32.pAssignProcessToJobObject(hJob, hProcess);
}

/**
 Set the process priority to be used by a job object.  If this functionality
 is not supported by the host OS, returns FALSE.

 @return TRUE on success, FALSE on failure.
 */
BOOL
YoriLibLimitJobObjectPriority(
    __in HANDLE hJob,
    __in DWORD Priority
    )
{
    YORI_JOB_BASIC_LIMIT_INFORMATION LimitInfo;
    if (Kernel32.pSetInformationJobObject == NULL) {
        return FALSE;
    }
    ZeroMemory(&LimitInfo, sizeof(LimitInfo));
    LimitInfo.Flags = 0x20;
    LimitInfo.Priority = Priority;
    return Kernel32.pSetInformationJobObject(hJob, 2, &LimitInfo, sizeof(LimitInfo));
}

// vim:sw=4:ts=4:et:
