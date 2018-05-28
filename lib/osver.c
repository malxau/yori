/**
 * @file lib/osver.c
 *
 * Yori OS version query routines
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

    if (CachedMajorOsVersion != 0) {
        *MajorVersion = CachedMajorOsVersion;
        *MinorVersion = CachedMinorOsVersion;
        *BuildNumber = CachedBuildNumber;
        return;
    }

    RawVersion = GetVersion();

    *MajorVersion = LOBYTE(LOWORD(RawVersion));
    *MinorVersion = HIBYTE(LOWORD(RawVersion));
    *BuildNumber = HIWORD(RawVersion);

    //
    //  On good versions of Windows, we stop here.  On broken versions of
    //  Windows, which lie about version numbers, we need to insist via
    //  a much more expensive mechanism.
    //

    if (*BuildNumber >= 9200) {
        LPTSTR Kernel32FileName;
        VS_FIXEDFILEINFO * FixedVerInfo;
        PVOID VerBuffer;
        DWORD SystemFileNameLength;
        DWORD Kernel32FileNameLength;
        DWORD VerSize;
        DWORD Junk;

	YoriLibLoadVersionFunctions();

        if (DllVersion.pGetFileVersionInfoSizeW == NULL ||
            DllVersion.pGetFileVersionInfoW == NULL ||
            DllVersion.pVerQueryValueW == NULL) {

            return;
        }

        SystemFileNameLength = GetSystemDirectory(NULL, 0);
        Kernel32FileNameLength = SystemFileNameLength + sizeof("\\KERNEL32.DLL");
        Kernel32FileName = YoriLibMalloc(Kernel32FileNameLength * sizeof(TCHAR));
        if (Kernel32FileName == NULL) {
            return;
        }

        GetSystemDirectory(Kernel32FileName, SystemFileNameLength);
        _tcscpy(&Kernel32FileName[SystemFileNameLength - 1], _T("\\KERNEL32.DLL"));

        VerSize = DllVersion.pGetFileVersionInfoSizeW(Kernel32FileName, &Junk);
        if (VerSize == 0) {
            YoriLibFree(Kernel32FileName);
            return;
        }

        VerBuffer = YoriLibMalloc(VerSize);
        if (VerBuffer == NULL) {
            YoriLibFree(Kernel32FileName);
            return;
        }

        if (!DllVersion.pGetFileVersionInfoW(Kernel32FileName, 0, VerSize, VerBuffer)) {
            YoriLibFree(VerBuffer);
            YoriLibFree(Kernel32FileName);
            return;
        }

        YoriLibFree(Kernel32FileName);

        if (!DllVersion.pVerQueryValueW(VerBuffer, _T("\\"), &FixedVerInfo, (PUINT)&Junk)) {
            YoriLibFree(VerBuffer);
            return;
        }

        CachedMajorOsVersion = HIWORD(FixedVerInfo->dwProductVersionMS);
        CachedMinorOsVersion = LOWORD(FixedVerInfo->dwProductVersionMS);
        CachedBuildNumber = HIWORD(FixedVerInfo->dwProductVersionLS);

        *MajorVersion = CachedMajorOsVersion;
        *MinorVersion = CachedMinorOsVersion;
        *BuildNumber = CachedBuildNumber;

        YoriLibFree(VerBuffer);
    } else {
        CachedMajorOsVersion = LOBYTE(LOWORD(RawVersion));
        CachedMinorOsVersion = HIBYTE(LOWORD(RawVersion));
        CachedBuildNumber = HIWORD(RawVersion);
    }
}

/**
 Return TRUE to indicate the target process is 32 bit, or FALSE if the target
 process is 64 bit.

 @param ProcessHandle A handle to the process to examine.

 @return TRUE to indicate the process is 32 bit, FALSE to indicate the target
         is 64 bit.
 */
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
