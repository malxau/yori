/**
 * @file lib/dyld_adv.c
 *
 * Yori dynamically loaded OS function support
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
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
 Map a function name to a location in memory to store a function pointer.
 */
typedef struct _YORI_DLL_NAME_MAP {

    /**
     Pointer to a memory location to be updated with a function pointer when
     the specified function is resolved.
     */
    FARPROC * FnPtr;

    /**
     The name of the function to resolve.
     */
    LPCSTR FnName;
} YORI_DLL_NAME_MAP, *PYORI_DLL_NAME_MAP;

/**
 A structure containing pointers to advapi32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_ADVAPI32_FUNCTIONS DllAdvApi32;

/**
 The set of functions to resolve from advapi32.dll.
 */
CONST YORI_DLL_NAME_MAP DllAdvApi32Symbols[] = {
    {(FARPROC *)&DllAdvApi32.pAccessCheck, "AccessCheck"},
    {(FARPROC *)&DllAdvApi32.pAdjustTokenPrivileges, "AdjustTokenPrivileges"},
    {(FARPROC *)&DllAdvApi32.pAllocateAndInitializeSid, "AllocateAndInitializeSid"},
    {(FARPROC *)&DllAdvApi32.pCheckTokenMembership, "CheckTokenMembership"},
    {(FARPROC *)&DllAdvApi32.pCryptAcquireContextW, "CryptAcquireContextW"},
    {(FARPROC *)&DllAdvApi32.pCryptCreateHash, "CryptCreateHash"},
    {(FARPROC *)&DllAdvApi32.pCryptDestroyHash, "CryptDestroyHash"},
    {(FARPROC *)&DllAdvApi32.pCryptGetHashParam, "CryptGetHashParam"},
    {(FARPROC *)&DllAdvApi32.pCryptHashData, "CryptHashData"},
    {(FARPROC *)&DllAdvApi32.pCryptReleaseContext, "CryptReleaseContext"},
    {(FARPROC *)&DllAdvApi32.pFreeSid, "FreeSid"},
    {(FARPROC *)&DllAdvApi32.pGetFileSecurityW, "GetFileSecurityW"},
    {(FARPROC *)&DllAdvApi32.pGetSecurityDescriptorOwner, "GetSecurityDescriptorOwner"},
    {(FARPROC *)&DllAdvApi32.pImpersonateSelf, "ImpersonateSelf"},
    {(FARPROC *)&DllAdvApi32.pInitializeAcl, "InitializeAcl"},
    {(FARPROC *)&DllAdvApi32.pInitiateShutdownW, "InitiateShutdownW"},
    {(FARPROC *)&DllAdvApi32.pLookupAccountNameW, "LookupAccountNameW"},
    {(FARPROC *)&DllAdvApi32.pLookupAccountSidW, "LookupAccountSidW"},
    {(FARPROC *)&DllAdvApi32.pLookupPrivilegeValueW, "LookupPrivilegeValueW"},
    {(FARPROC *)&DllAdvApi32.pOpenProcessToken, "OpenProcessToken"},
    {(FARPROC *)&DllAdvApi32.pOpenThreadToken, "OpenThreadToken"},
    {(FARPROC *)&DllAdvApi32.pRegCloseKey, "RegCloseKey"},
    {(FARPROC *)&DllAdvApi32.pRegCreateKeyExW, "RegCreateKeyExW"},
    {(FARPROC *)&DllAdvApi32.pRegDeleteKeyW, "RegDeleteKeyW"},
    {(FARPROC *)&DllAdvApi32.pRegDeleteValueW, "RegDeleteValueW"},
    {(FARPROC *)&DllAdvApi32.pRegEnumKeyExW, "RegEnumKeyExW"},
    {(FARPROC *)&DllAdvApi32.pRegEnumValueW, "RegEnumValueW"},
    {(FARPROC *)&DllAdvApi32.pRegOpenKeyExW, "RegOpenKeyExW"},
    {(FARPROC *)&DllAdvApi32.pRegQueryValueExW, "RegQueryValueExW"},
    {(FARPROC *)&DllAdvApi32.pRegSetValueExW, "RegSetValueExW"},
    {(FARPROC *)&DllAdvApi32.pRevertToSelf, "RevertToSelf"},
    {(FARPROC *)&DllAdvApi32.pSetNamedSecurityInfoW, "SetNamedSecurityInfoW"}
};

/**
 Load pointers to all optional advapi32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadAdvApi32Functions(VOID)
{
    DWORD Count;

    if (DllAdvApi32.hDll != NULL) {
        return TRUE;
    }

    DllAdvApi32.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("ADVAPI32.DLL"));
    if (DllAdvApi32.hDll == NULL) {

        //
        //  Nano server includes advapi32legacy.dll (and kernel32legacy.dll.)
        //  Unfortunately it redirects on the import table but not on dynamic
        //  load, so we need to explicitly indicate that we want the advapi32
        //  implementation here.
        //
        //  (It is amusing that functions for the registry or ACLs are
        //   considered legacy.  For whatever reason, there's a temptation to
        //   declare anything that exists as legacy without understanding why
        //   it's there.)
        //

        if (YoriLibIsNanoServer()) {
            DllAdvApi32.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("ADVAPI32LEGACY.DLL"));
        }
        if (DllAdvApi32.hDll == NULL) {
            return FALSE;
        }
    }

    for (Count = 0; Count < sizeof(DllAdvApi32Symbols)/sizeof(DllAdvApi32Symbols[0]); Count++) {
        *(DllAdvApi32Symbols[Count].FnPtr) = GetProcAddress(DllAdvApi32.hDll, DllAdvApi32Symbols[Count].FnName);
    }


    return TRUE;
}

// vim:sw=4:ts=4:et:
