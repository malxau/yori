/**
 * @file lib/osver.c
 *
 * Yori OS version query routines
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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
#include <winver.h>

/**
 Previously returned value for the major OS version number.
 */
DWORD CachedMajorOsVersion;

/**
 Previously returned value for the minor OS version number.
 */
DWORD CachedMinorOsVersion;

/**
 Previously returned value for the build number.
 */
DWORD CachedBuildNumber;

#if _WIN64
/**
 On 64 bit builds, the current process PEB is 64 bit.
 */
#define PYORI_LIB_PEB_NATIVE PYORI_LIB_PEB64
#else
/**
 On 32 bit builds, the current process PEB is 32 bit.
 */
#define PYORI_LIB_PEB_NATIVE PYORI_LIB_PEB32_NATIVE
#endif

/**
 Try to obtain Windows version numbers from the PEB directly.

 @param MajorVersion On successful completion, updated to contain the Windows
        major version number.

 @param MinorVersion On successful completion, updated to contain the Windows
        minor version number.

 @param BuildNumber On successful completion, updated to contain the Windows
        build number.
 */
__success(return)
BOOL
YoriLibGetOsVersionFromPeb(
    __out PDWORD MajorVersion,
    __out PDWORD MinorVersion,
    __out PDWORD BuildNumber
    )
{
    PYORI_LIB_PEB_NATIVE Peb;
    LONG Status;
    PROCESS_BASIC_INFORMATION BasicInfo;
    DWORD dwBytesReturned;

    if (DllNtDll.pNtQueryInformationProcess == NULL) {
        return FALSE;
    }

    Status = DllNtDll.pNtQueryInformationProcess(GetCurrentProcess(), 0, &BasicInfo, sizeof(BasicInfo), &dwBytesReturned);
    if (Status != 0) {
        return FALSE;
    }

    Peb = (PYORI_LIB_PEB_NATIVE)BasicInfo.PebBaseAddress;

    *MajorVersion = Peb->OSMajorVersion;
    *MinorVersion = Peb->OSMinorVersion;
    *BuildNumber = Peb->OSBuildNumber;

    return TRUE;
}

//
//  Disable warning about using deprecated GetVersion.
//

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(disable: 28159)
#endif

/**
 Return Windows version numbers.

 @param MajorVersion On successful completion, updated to contain the Windows
        major version number.

 @param MinorVersion On successful completion, updated to contain the Windows
        minor version number.

 @param BuildNumber On successful completion, updated to contain the Windows
        build number.
 */
VOID
YoriLibGetOsVersion(
    __out PDWORD MajorVersion,
    __out PDWORD MinorVersion,
    __out PDWORD BuildNumber
    )
{
    DWORD RawVersion;
    DWORD LocalMajorVersion;
    DWORD LocalMinorVersion;
    DWORD LocalBuildNumber;

    if (CachedMajorOsVersion != 0) {
        *MajorVersion = CachedMajorOsVersion;
        *MinorVersion = CachedMinorOsVersion;
        *BuildNumber = CachedBuildNumber;
        return;
    }

    if (DllKernel32.pGetVersionExW != NULL) {
        YORI_OS_VERSION_INFO OsVersionInfo;
        ZeroMemory(&OsVersionInfo, sizeof(OsVersionInfo));
        OsVersionInfo.dwOsVersionInfoSize = sizeof(OsVersionInfo);
        DllKernel32.pGetVersionExW(&OsVersionInfo);

        LocalMajorVersion = OsVersionInfo.dwMajorVersion;
        LocalMinorVersion = OsVersionInfo.dwMinorVersion;
        LocalBuildNumber = OsVersionInfo.dwBuildNumber;
    } else {
        RawVersion = GetVersion();

        LocalMajorVersion = LOBYTE(LOWORD(RawVersion));
        LocalMinorVersion = HIBYTE(LOWORD(RawVersion));
        LocalBuildNumber = HIWORD(RawVersion);
    }

    //
    //  On good versions of Windows, we stop here.  On broken versions of
    //  Windows, which lie about version numbers, we need to insist via
    //  a much more expensive mechanism.
    //

    if (LocalMajorVersion == 6 &&
        LocalMinorVersion == 2 &&
        LocalBuildNumber == 9200) {

        YoriLibGetOsVersionFromPeb(&LocalMajorVersion, &LocalMinorVersion, &LocalBuildNumber);
    }

    CachedMajorOsVersion = LocalMajorVersion;
    CachedMinorOsVersion = LocalMinorVersion;
    CachedBuildNumber = LocalBuildNumber;

    *MajorVersion = LocalMajorVersion;
    *MinorVersion = LocalMinorVersion;
    *BuildNumber = LocalBuildNumber;
}

/**
 Return TRUE to indicate the target process is 32 bit, or FALSE if the target
 process is 64 bit.

 @param ProcessHandle A handle to the process to examine.

 @return TRUE to indicate the process is 32 bit, FALSE to indicate the target
         is 64 bit.
 */
__success(return)
BOOL
YoriLibIsProcess32Bit(
    __in HANDLE ProcessHandle
    )
{
    BOOL TargetProcess32Bit = TRUE;

    if (DllKernel32.pIsWow64Process != NULL) {

        //
        //  If this program is 32 bit, and it's Wow (ie., the system is 64
        //  bit), check the bitness of the target.  If this program is 64
        //  bit, check the target process.  If this program is 32 bit on a
        //  32 bit system (ie., we're 32 bit and not Wow) then the target
        //  must be 32 bit.
        // 

        if (sizeof(PVOID) == sizeof(ULONG)) {
            BOOL ThisProcessWow = FALSE;
            DllKernel32.pIsWow64Process(GetCurrentProcess(), &ThisProcessWow);

            if (ThisProcessWow) {
                DllKernel32.pIsWow64Process(ProcessHandle, &TargetProcess32Bit);
            }
        } else {
            DllKernel32.pIsWow64Process(ProcessHandle, &TargetProcess32Bit);
        }
    }

    return TargetProcess32Bit;
}

/**
 Return TRUE if the target process has a 32 bit PEB.

 @param ProcessHandle Handle to the target process.

 @return TRUE if the PEB is in 32 bit form, FALSE if it's in 64 bit form.
 */
__success(return)
BOOL
YoriLibDoesProcessHave32BitPeb(
    __in HANDLE ProcessHandle
    )
{

    UNREFERENCED_PARAMETER(ProcessHandle);

    //
    //  If the system doesn't support Wow64, this must be a 32 bit process
    //  checking another 32 bit process.
    //

    if (DllKernel32.pIsWow64Process == NULL) {
        return TRUE;
    }

    //
    //  If this is a 32 bit process, it can't debug a 64 bit process, so the
    //  target had better be 32 bit.
    //

    if (sizeof(PVOID) == sizeof(ULONG)) {

        return TRUE;
    }

    return FALSE;

}

// vim:sw=4:ts=4:et:
