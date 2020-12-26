/**
 * @file lib/priv.c
 *
 * Yori privilege manipulation routines
 *
 * Copyright (c) 2014-2019 Malcolm J. Smith
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
 Attempt to enable a specified privilege.

 @return TRUE to indicate that the privilege enablement was successful.
 */
BOOL
YoriLibEnableNamedPrivilege(
    LPTSTR PrivilegeName
    )
{
    HANDLE ProcessToken;
    YoriLibLoadAdvApi32Functions();

    //
    //  Attempt to enable backup privilege.  This allows us to enumerate and recurse
    //  through objects which normally ACLs would prevent.
    //

    if (DllAdvApi32.pOpenProcessToken != NULL &&
        DllAdvApi32.pLookupPrivilegeValueW != NULL &&
        DllAdvApi32.pAdjustTokenPrivileges != NULL &&
        DllAdvApi32.pOpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &ProcessToken)) {
        struct {
            TOKEN_PRIVILEGES TokenPrivileges;
            LUID_AND_ATTRIBUTES BackupPrivilege;
        } PrivilegesToChange;

        if (!DllAdvApi32.pLookupPrivilegeValueW(NULL, PrivilegeName, &PrivilegesToChange.TokenPrivileges.Privileges[0].Luid)) {
            CloseHandle(ProcessToken);
            return FALSE;
        }

        PrivilegesToChange.TokenPrivileges.PrivilegeCount = 1;
        PrivilegesToChange.TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!DllAdvApi32.pAdjustTokenPrivileges(ProcessToken, FALSE, (PTOKEN_PRIVILEGES)&PrivilegesToChange, sizeof(PrivilegesToChange), NULL, NULL)) {
            CloseHandle(ProcessToken);
            return FALSE;
        }
        CloseHandle(ProcessToken);
        return TRUE;
    }

    return FALSE;
}

/**
 Attempt to enable backup privilege to allow Administrators to enumerate and
 open more objects successfully.  If this fails the application may encounter
 more objects it cannot accurately account for, but it is not fatal, or
 unexpected.

 @return TRUE to indicate that the privilege enablement was attempted
         successfully.
 */
BOOL
YoriLibEnableBackupPrivilege(VOID)
{
    YoriLibEnableNamedPrivilege(SE_BACKUP_NAME);
    return TRUE;
}

/**
 Attempt to enable debug privilege to allow Administrators to take kernel
 dumps.

 @return TRUE to indicate that the privilege enablement was successful.
 */
BOOL
YoriLibEnableDebugPrivilege(VOID)
{
    return YoriLibEnableNamedPrivilege(SE_DEBUG_NAME);
}

// vim:sw=4:ts=4:et:
