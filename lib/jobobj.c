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
 Prototype for a function to create a job object.
 */
typedef HANDLE WINAPI CREATE_JOB_OBJECTW(LPSECURITY_ATTRIBUTES, LPCWSTR);

/**
 Prototype for a pointer to a function to create a job object.
 */
typedef CREATE_JOB_OBJECTW *PCREATE_JOB_OBJECTW;

/**
 Pointer to a function to create a job object.
 */
PCREATE_JOB_OBJECTW pCreateJobObjectW;

/**
 Attempt to create an unnamed job object.  If the functionality is not
 supported by the host OS, returns NULL.

 @return Handle to a job object, or NULL on failure.
 */
HANDLE
YoriLibCreateJobObject(
    )
{
    if (pCreateJobObjectW == NULL) {
        HMODULE hKernel;

        hKernel = GetModuleHandle(_T("KERNEL32"));
        pCreateJobObjectW = (PCREATE_JOB_OBJECTW)GetProcAddress(hKernel, "CreateJobObjectW");
        if (pCreateJobObjectW == NULL) {
            return NULL;
        }
    }
    return pCreateJobObjectW(NULL, NULL);
}

/**
 Prototype for a function to assign a process to a job object.
 */
typedef BOOL WINAPI ASSIGN_PROCESS_TO_JOB_OBJECT(HANDLE, HANDLE);

/**
 Prototype for a pointer to a function to assign a process to a job object.
 */
typedef ASSIGN_PROCESS_TO_JOB_OBJECT *PASSIGN_PROCESS_TO_JOB_OBJECT;

/**
 Pointer to a function to assign a process to a job object.
 */
PASSIGN_PROCESS_TO_JOB_OBJECT pAssignProcessToJobObject;

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
    if (pAssignProcessToJobObject == NULL) {
        HMODULE hKernel;

        hKernel = GetModuleHandle(_T("KERNEL32"));
        pAssignProcessToJobObject = (PASSIGN_PROCESS_TO_JOB_OBJECT)GetProcAddress(hKernel, "AssignProcessToJobObject");
        if (pAssignProcessToJobObject == NULL) {
            return FALSE;
        }
    }
    return pAssignProcessToJobObject(hJob, hProcess);
}

/**
 Prototype for a function to manipulate information about a job.
 */
typedef BOOL WINAPI SET_INFORMATION_JOB_OBJECT(HANDLE, DWORD, PVOID, DWORD);

/**
 Prototype for a pointer to a function to manipulate information about a job.
 */
typedef SET_INFORMATION_JOB_OBJECT *PSET_INFORMATION_JOB_OBJECT;

/**
 Pointer to a function to manipulate information about a job.
 */
PSET_INFORMATION_JOB_OBJECT pSetInformationJobObject;

/**
 Structure to change basic information about a job.
 */
typedef struct _YORI_JOB_BASIC_LIMIT_INFORMATION {

    /**
     Field not needed/supported by YoriLib.
     */
    LARGE_INTEGER Unused1;

    /**
     Field not needed/supported by YoriLib.
     */
    LARGE_INTEGER Unused2;

    /**
     Indicates which fields should be interpreted when setting information on
     a job.
     */
    DWORD Flags;

    /**
     Field not needed/supported by YoriLib.
     */
    SIZE_T Unused3;

    /**
     Field not needed/supported by YoriLib.
     */
    SIZE_T Unused4;

    /**
     Field not needed/supported by YoriLib.
     */
    DWORD Unused5;

    /**
     Field not needed/supported by YoriLib.
     */
    SIZE_T Unused6;

    /**
     The base process priority to assign to the job.
     */
    DWORD Priority;

    /**
     Field not needed/supported by YoriLib.
     */
    DWORD Unused7;
} YORI_JOB_BASIC_LIMIT_INFORMATION, *PYORI_JOB_BASIC_LIMIT_INFORMATION;


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
    if (pSetInformationJobObject == NULL) {
        HMODULE hKernel;

        hKernel = GetModuleHandle(_T("KERNEL32"));
        pSetInformationJobObject = (PSET_INFORMATION_JOB_OBJECT)GetProcAddress(hKernel, "SetInformationJobObject");
        if (pSetInformationJobObject == NULL) {
            return FALSE;
        }
    }
    ZeroMemory(&LimitInfo, sizeof(LimitInfo));
    LimitInfo.Flags = 0x20;
    LimitInfo.Priority = Priority;
    return pSetInformationJobObject(hJob, 2, &LimitInfo, sizeof(LimitInfo));
}

// vim:sw=4:ts=4:et:
