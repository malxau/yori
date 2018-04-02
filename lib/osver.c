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
 Prototype for the GetFileVersionInfoSize function.
 */
typedef DWORD WINAPI GET_FILE_VERSION_INFO_SIZEW_FN(LPWSTR, LPDWORD);

/**
 Pointer to the GetFileVersionInfoSize function.
 */
typedef GET_FILE_VERSION_INFO_SIZEW_FN *PGET_FILE_VERSION_INFO_SIZEW_FN;

/**
 Prototype for the GetFileVersionInfo function.
 */
typedef BOOL WINAPI GET_FILE_VERSION_INFOW_FN(LPWSTR, DWORD, DWORD, LPVOID);

/**
 Pointer to the GetFileVersionInfo function.
 */
typedef GET_FILE_VERSION_INFOW_FN *PGET_FILE_VERSION_INFOW_FN;

/**
 Prototype for the VerQueryValue function.
 */
typedef BOOL WINAPI VER_QUERY_VALUEW_FN(CONST LPVOID, LPWSTR, LPVOID *, PUINT);

/**
 Pointer to the VerQueryValue function.
 */
typedef VER_QUERY_VALUEW_FN *PVER_QUERY_VALUEW_FN;

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
        PGET_FILE_VERSION_INFO_SIZEW_FN GetFileVersionInfoSizeFn;
        PGET_FILE_VERSION_INFOW_FN GetFileVersionInfoFn;
        PVER_QUERY_VALUEW_FN VerQueryValueFn;
        HINSTANCE hVerDll;

        hVerDll = LoadLibrary(_T("VERSION.DLL"));
        GetFileVersionInfoSizeFn = (PGET_FILE_VERSION_INFO_SIZEW_FN)GetProcAddress(hVerDll, "GetFileVersionInfoSizeW");
        GetFileVersionInfoFn = (PGET_FILE_VERSION_INFOW_FN)GetProcAddress(hVerDll, "GetFileVersionInfoW");
        VerQueryValueFn = (PVER_QUERY_VALUEW_FN)GetProcAddress(hVerDll, "VerQueryValueW");

        if (GetFileVersionInfoSizeFn == NULL ||
            GetFileVersionInfoFn == NULL ||
            VerQueryValueFn == NULL) {

            FreeLibrary(hVerDll);
            return;
        }

        SystemFileNameLength = GetSystemDirectory(NULL, 0);
        Kernel32FileNameLength = SystemFileNameLength + sizeof("\\KERNEL32.DLL");
        Kernel32FileName = YoriLibMalloc(Kernel32FileNameLength * sizeof(TCHAR));
        if (Kernel32FileName == NULL) {
            FreeLibrary(hVerDll);
            return;
        }

        GetSystemDirectory(Kernel32FileName, SystemFileNameLength);
        _tcscpy(&Kernel32FileName[SystemFileNameLength - 1], _T("\\KERNEL32.DLL"));

        VerSize = GetFileVersionInfoSizeFn(Kernel32FileName, &Junk);
        if (VerSize == 0) {
            FreeLibrary(hVerDll);
            YoriLibFree(Kernel32FileName);
            return;
        }

        VerBuffer = YoriLibMalloc(VerSize);
        if (VerBuffer == NULL) {
            FreeLibrary(hVerDll);
            YoriLibFree(Kernel32FileName);
            return;
        }

        if (!GetFileVersionInfoFn(Kernel32FileName, 0, VerSize, VerBuffer)) {
            FreeLibrary(hVerDll);
            YoriLibFree(VerBuffer);
            YoriLibFree(Kernel32FileName);
            return;
        }

        YoriLibFree(Kernel32FileName);

        if (!VerQueryValueFn(VerBuffer, _T("\\"), &FixedVerInfo, (PUINT)&Junk)) {
            YoriLibFree(VerBuffer);
            FreeLibrary(hVerDll);
            return;
        }

        CachedMajorOsVersion = HIWORD(FixedVerInfo->dwProductVersionMS);
        CachedMinorOsVersion = LOWORD(FixedVerInfo->dwProductVersionMS);
        CachedBuildNumber = HIWORD(FixedVerInfo->dwProductVersionLS);

        *MajorVersion = CachedMajorOsVersion;
        *MinorVersion = CachedMinorOsVersion;
        *BuildNumber = CachedBuildNumber;

        YoriLibFree(VerBuffer);
        FreeLibrary(hVerDll);
    } else {
        CachedMajorOsVersion = LOBYTE(LOWORD(RawVersion));
        CachedMinorOsVersion = HIBYTE(LOWORD(RawVersion));
        CachedBuildNumber = HIWORD(RawVersion);
    }
}
