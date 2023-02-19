/**
 * @file pkglib/reg.c
 *
 * Yori shell registry updates
 *
 * Copyright (c) 2018-2019 Malcolm J. Smith
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
#include "yoripkgp.h"

/**
 HWND_BROADCAST casts an integer to a pointer, which in 64 bit means
 increasing the size.
 */
#pragma warning(disable: 4306)

/**
 Append a new path component to an existing registry path.

 @param hRootKey The root of the registry hive to update.

 @param SubKey The sub key to update.

 @param ValueName The name of the value to update.

 @param PathToAdd The path that should be added to the specified value.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgAppendPath(
    __in HKEY hRootKey,
    __in LPTSTR SubKey,
    __in LPTSTR ValueName,
    __in PCYORI_STRING PathToAdd
    )
{
    HKEY hKey;
    DWORD Err;
    DWORD Disposition;
    DWORD LengthRequired;
    YORI_STRING ExistingValue;

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllAdvApi32.pRegSetValueExW == NULL) {

        return FALSE;
    }

    Err = DllAdvApi32.pRegCreateKeyExW(hRootKey, SubKey, 0, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hKey, &Disposition);
    if (Err != ERROR_SUCCESS) {
        return FALSE;
    }

    LengthRequired = 0;
    YoriLibInitEmptyString(&ExistingValue);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, ValueName, NULL, NULL, NULL, &LengthRequired);
    if (Err == ERROR_MORE_DATA || LengthRequired > 0) {
        if (!YoriLibAllocateString(&ExistingValue, (LengthRequired / sizeof(TCHAR)) + PathToAdd->LengthInChars + 1)) {
            DllAdvApi32.pRegCloseKey(hKey);
            return FALSE;
        }

        Err = DllAdvApi32.pRegQueryValueExW(hKey, ValueName, NULL, NULL, (LPBYTE)ExistingValue.StartOfString, &LengthRequired);
        if (Err != ERROR_SUCCESS) {
            YoriLibFreeStringContents(&ExistingValue);
            DllAdvApi32.pRegCloseKey(hKey);
            return FALSE;
        }

        ExistingValue.LengthInChars = LengthRequired / sizeof(TCHAR) - 1;

        if (!YoriLibAddEnvironmentComponentToString(&ExistingValue, PathToAdd, TRUE)) {
            YoriLibFreeStringContents(&ExistingValue);
            DllAdvApi32.pRegCloseKey(hKey);
            return FALSE;
        }

        Err = DllAdvApi32.pRegSetValueExW(hKey, ValueName, 0, REG_EXPAND_SZ, (LPBYTE)ExistingValue.StartOfString, (ExistingValue.LengthInChars + 1) * sizeof(TCHAR));
        DllAdvApi32.pRegCloseKey(hKey);
        YoriLibFreeStringContents(&ExistingValue);
    } else {
        Err = DllAdvApi32.pRegSetValueExW(hKey, ValueName, 0, REG_EXPAND_SZ, (LPBYTE)PathToAdd->StartOfString, (PathToAdd->LengthInChars + 1) * sizeof(TCHAR));
        DllAdvApi32.pRegCloseKey(hKey);
    }

    if (Err == ERROR_SUCCESS) {
        return TRUE;
    }
    return FALSE;
}

/**
 Remove a path component to an existing registry path.

 @param hRootKey The root of the registry hive to update.

 @param SubKey The sub key to update.

 @param ValueName The name of the value to update.

 @param PathToRemove The path that should be removed from the specified value.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgRemoveInstalledPath(
    __in HKEY hRootKey,
    __in LPTSTR SubKey,
    __in LPTSTR ValueName,
    __in PYORI_STRING PathToRemove
    )
{
    HKEY hKey;
    DWORD Err;
    DWORD Disposition;
    DWORD LengthRequired;
    YORI_STRING ExistingValue;
    YORI_STRING NewValue;

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegDeleteValueW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllAdvApi32.pRegSetValueExW == NULL) {

        return FALSE;
    }

    Err = DllAdvApi32.pRegCreateKeyExW(hRootKey, SubKey, 0, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hKey, &Disposition);
    if (Err != ERROR_SUCCESS) {
        Err = DllAdvApi32.pRegCreateKeyExW(hRootKey, SubKey, 0, NULL, 0, KEY_QUERY_VALUE, NULL, &hKey, &Disposition);
        if (Err != ERROR_SUCCESS) {
            return FALSE;
        }
    }

    LengthRequired = 0;
    YoriLibInitEmptyString(&ExistingValue);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, ValueName, NULL, NULL, NULL, &LengthRequired);
    if (Err == ERROR_MORE_DATA || LengthRequired > 0) {
        if (!YoriLibAllocateString(&ExistingValue, (LengthRequired / sizeof(TCHAR)) + 1)) {
            DllAdvApi32.pRegCloseKey(hKey);
            return FALSE;
        }

        Err = DllAdvApi32.pRegQueryValueExW(hKey, ValueName, NULL, NULL, (LPBYTE)ExistingValue.StartOfString, &LengthRequired);
        if (Err != ERROR_SUCCESS) {
            YoriLibFreeStringContents(&ExistingValue);
            DllAdvApi32.pRegCloseKey(hKey);
            return FALSE;
        }

        ExistingValue.LengthInChars = LengthRequired / sizeof(TCHAR) - 1;

        if (!YoriLibRemoveEnvironmentComponentFromString(&ExistingValue, PathToRemove, &NewValue)) {

            YoriLibFreeStringContents(&ExistingValue);
            DllAdvApi32.pRegCloseKey(hKey);
            return FALSE;
        }

        // 
        //  If no change was made (the component is not present), then return
        //  success immediately.  Note this means the function succeeds if the
        //  user doesn't have access to a system location but the value to
        //  remove from it wasn't present anyway.
        //

        if (YoriLibCompareString(&ExistingValue, &NewValue) == 0) {
            DllAdvApi32.pRegCloseKey(hKey);
            YoriLibFreeStringContents(&ExistingValue);
            YoriLibFreeStringContents(&NewValue);
            return TRUE;
        }

        if (NewValue.LengthInChars == 0) {
            Err = DllAdvApi32.pRegDeleteValueW(hKey, ValueName);
        } else {
            Err = DllAdvApi32.pRegSetValueExW(hKey, ValueName, 0, REG_EXPAND_SZ, (LPBYTE)NewValue.StartOfString, (NewValue.LengthInChars + 1) * sizeof(TCHAR));
        }
        DllAdvApi32.pRegCloseKey(hKey);
        YoriLibFreeStringContents(&ExistingValue);
        YoriLibFreeStringContents(&NewValue);
    } else {
        Err = ERROR_SUCCESS;
        DllAdvApi32.pRegCloseKey(hKey);
    }

    if (Err == ERROR_SUCCESS) {
        return TRUE;
    }
    return FALSE;
}

/**
 Check if a specified path is currently scheduled to be deleted on next
 reboot.

 @param FilePath The path to check the registry for.

 @return TRUE to indicate that the file is marked to be deleted on reboot,
         FALSE if not.  Note this function returns FALSE on failure.
 */
BOOL
YoriPkgIsFileToBeDeletedOnReboot(
    __in PYORI_STRING FilePath
    )
{
    HKEY hKey;
    DWORD Err;
    DWORD Disposition;
    DWORD LengthRequired;
    YORI_STRING ExistingValue;
    YORI_STRING FoundFileEntry;
    BOOLEAN LookingForEndOfSource;
    DWORD Index;

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL) {

        return FALSE;
    }

    Err = DllAdvApi32.pRegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                       _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager"),
                                       0,
                                       NULL,
                                       0,
                                       KEY_QUERY_VALUE,
                                       NULL,
                                       &hKey,
                                       &Disposition);
    if (Err != ERROR_SUCCESS) {
        return FALSE;
    }

    LengthRequired = 0;
    YoriLibInitEmptyString(&ExistingValue);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("PendingFileRenameOperations"), NULL, NULL, NULL, &LengthRequired);
    if (Err == ERROR_MORE_DATA || LengthRequired > 0) {
        if (!YoriLibAllocateString(&ExistingValue, (LengthRequired / sizeof(TCHAR)) + 1)) {
            DllAdvApi32.pRegCloseKey(hKey);
            return FALSE;
        }

        __analysis_assume(ExistingValue.StartOfString != NULL);

        Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("PendingFileRenameOperations"), NULL, NULL, (LPBYTE)ExistingValue.StartOfString, &LengthRequired);
        if (Err != ERROR_SUCCESS) {
            YoriLibFreeStringContents(&ExistingValue);
            DllAdvApi32.pRegCloseKey(hKey);
            return FALSE;
        }

        ExistingValue.LengthInChars = LengthRequired / sizeof(TCHAR) - 1;

        DllAdvApi32.pRegCloseKey(hKey);

        YoriLibInitEmptyString(&FoundFileEntry);
        FoundFileEntry.StartOfString = ExistingValue.StartOfString;
        LookingForEndOfSource = TRUE;
        for (Index = 0; Index < ExistingValue.LengthInChars; Index++) {
            if (ExistingValue.StartOfString[Index] == '\0') {
                if (FoundFileEntry.StartOfString == NULL) {
                    FoundFileEntry.StartOfString = &ExistingValue.StartOfString[Index + 1];
                } else {
                    if (YoriLibCompareStringWithLiteralCount(&FoundFileEntry, _T("\\??\\"), sizeof("\\??\\") - 1) == 0) {
                        FoundFileEntry.StartOfString += sizeof("\\??\\") - 1;
                        FoundFileEntry.LengthInChars -= sizeof("\\??\\") - 1;
                    } else if (YoriLibCompareStringWithLiteralCount(&FoundFileEntry, _T("\\\\?\\"), sizeof("\\\\?\\") - 1) == 0) {
                        FoundFileEntry.StartOfString += sizeof("\\\\?\\") - 1;
                        FoundFileEntry.LengthInChars -= sizeof("\\\\?\\") - 1;
                    }
                    if (YoriLibCompareStringInsensitive(&FoundFileEntry, FilePath) == 0) {
                        YoriLibFreeStringContents(&ExistingValue);
                        return TRUE;
                    }
                    FoundFileEntry.StartOfString = NULL;
                    FoundFileEntry.LengthInChars = 0;

                }
            } else if (FoundFileEntry.StartOfString != NULL) {
                FoundFileEntry.LengthInChars++;
            }
        }
        YoriLibFreeStringContents(&ExistingValue);
    }

    return FALSE;
}

/**
 Append the specified directory to one or more of the system path or user
 path.  Note that the system path requires privilege and it is expected that
 this can fail if the currently executing user does not have access to it.

 @param TargetDirectory Pointer to the directory to append to a path.  If not
        specified, the directory containing the current executable is used.

 @param AppendToUserPath If TRUE, the directory should be appended to the
        user's path environment variable.

 @param AppendToSystemPath If TRUE, the directory should be appended to the
        system's path environment variable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgAppendInstallDirToPath(
    __in_opt PCYORI_STRING TargetDirectory,
    __in BOOL AppendToUserPath,
    __in BOOL AppendToSystemPath
    )
{
    BOOL Result = TRUE;
    YORI_STRING LocalTargetDirectory;

    YoriLibLoadAdvApi32Functions();
    YoriLibLoadUser32Functions();

    if (TargetDirectory == NULL) {
        YORI_STRING AppDir;
        YoriLibConstantString(&AppDir, _T("~APPDIR"));
        if (!YoriLibUserStringToSingleFilePath(&AppDir, FALSE, &LocalTargetDirectory)) {
            return FALSE;
        }
    } else {
        YoriLibInitEmptyString(&LocalTargetDirectory);
        LocalTargetDirectory.StartOfString = TargetDirectory->StartOfString;
        LocalTargetDirectory.LengthInChars = TargetDirectory->LengthInChars;
        LocalTargetDirectory.LengthAllocated = TargetDirectory->LengthAllocated;
    }

    if (AppendToUserPath) {
        if (!YoriPkgAppendPath(HKEY_CURRENT_USER, _T("Environment"), _T("Path"), &LocalTargetDirectory)) {
            Result = FALSE;
        }
    }

    if (AppendToSystemPath) {
        if (!YoriPkgAppendPath(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), _T("Path"), &LocalTargetDirectory)) {
            Result = FALSE;
        }
    }

    if (DllUser32.pSendMessageTimeoutW != NULL &&
        (AppendToUserPath || AppendToSystemPath)) {
        DWORD_PTR NotifyResult;
        DllUser32.pSendMessageTimeoutW(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)_T("Environment"), SMTO_NORMAL, 200, &NotifyResult);
    }

    YoriLibFreeStringContents(&LocalTargetDirectory);

    return Result;
}

/**
 Remove the specified directory from one or more of the system path or user
 path.  Note that the system path requires privilege and it is expected that
 this can fail if the currently executing user does not have access to it.

 @param TargetDirectory Pointer to the directory to remove from a path.

 @param RemoveFromUserPath If TRUE, the directory should be removed from the
        user's path environment variable.

 @param RemoveFromSystemPath If TRUE, the directory should be removed from the
        system's path environment variable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgRemoveInstallDirFromPath(
    __in PYORI_STRING TargetDirectory,
    __in BOOL RemoveFromUserPath,
    __in BOOL RemoveFromSystemPath
    )
{
    BOOL Result = TRUE;

    YoriLibLoadAdvApi32Functions();
    YoriLibLoadUser32Functions();

    if (RemoveFromUserPath) {
        if (!YoriPkgRemoveInstalledPath(HKEY_CURRENT_USER, _T("Environment"), _T("Path"), TargetDirectory)) {
            Result = FALSE;
        }
    }

    if (RemoveFromSystemPath) {
        if (!YoriPkgRemoveInstalledPath(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), _T("Path"), TargetDirectory)) {
            Result = FALSE;
        }
    }

    if (DllUser32.pSendMessageTimeoutW != NULL &&
        (RemoveFromUserPath || RemoveFromSystemPath)) {

        DWORD_PTR NotifyResult;
        DllUser32.pSendMessageTimeoutW(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)_T("Environment"), SMTO_NORMAL, 200, &NotifyResult);
    }

    return Result;
}

/**
 Attempt to add an uninstall entry so that the program can be uninstalled via
 the control panel.  Note this function is using per-user support, which
 wasn't always present, but users of very old systems are expected to face
 a few more rough edges.  Note that this function accepts a version to
 register with control panel, but this isn't updated via ypm, so it's really
 just a record of the version of ysetup that originally installed the program.

 @param TargetDirectory Pointer to the installation directory for the program.

 @param InitialVersion Pointer to the version to populate into the control
        panel.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgAddUninstallEntry(
    __in PCYORI_STRING TargetDirectory,
    __in PCYORI_STRING InitialVersion
    )
{
    HKEY hKey;
    DWORD Err;
    DWORD Disposition;
    YORI_STRING IconString;
    YORI_STRING UninstallString;

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegSetValueExW == NULL) {

        return FALSE;
    }

    Err = DllAdvApi32.pRegCreateKeyExW(HKEY_CURRENT_USER,
                                       _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Yori"),
                                       0,
                                       NULL,
                                       0,
                                       KEY_QUERY_VALUE | KEY_SET_VALUE,
                                       NULL,
                                       &hKey,
                                       &Disposition);
    if (Err != ERROR_SUCCESS) {
        return FALSE;
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("Comments"), 0, REG_SZ, (LPBYTE)_T("Yori"), sizeof(_T("Yori")));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    YoriLibInitEmptyString(&IconString);
    YoriLibYPrintf(&IconString, _T("%y\\Yori.exe,0"), TargetDirectory);
    if (IconString.LengthInChars > 0) {
        Err = DllAdvApi32.pRegSetValueExW(hKey, _T("DisplayIcon"), 0, REG_SZ, (LPBYTE)IconString.StartOfString, (IconString.LengthInChars + 1) * sizeof(TCHAR));
        YoriLibFreeStringContents(&IconString);
        if (Err != ERROR_SUCCESS) {
            goto Exit;
        }
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("DisplayName"), 0, REG_SZ, (LPBYTE)_T("Yori"), sizeof(_T("Yori")));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("DisplayVersion"), 0, REG_SZ, (LPBYTE)InitialVersion->StartOfString, (InitialVersion->LengthInChars + 1) * sizeof(TCHAR));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("HelpLink"), 0, REG_SZ, (LPBYTE)_T("http://www.malsmith.net/yori/"), sizeof(_T("http://www.malsmith.net/yori/")));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("InstallLocation"), 0, REG_SZ, (LPBYTE)TargetDirectory->StartOfString, (TargetDirectory->LengthInChars + 1) * sizeof(TCHAR));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("Publisher"), 0, REG_SZ, (LPBYTE)_T("malsmith.net"), sizeof(_T("malsmith.net")));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    YoriLibInitEmptyString(&UninstallString);
    YoriLibYPrintf(&UninstallString, _T("\"%y\\Ypm.exe\" -uninstall"), TargetDirectory);
    if (UninstallString.LengthInChars > 0) {
        Err = DllAdvApi32.pRegSetValueExW(hKey, _T("UninstallString"), 0, REG_SZ, (LPBYTE)UninstallString.StartOfString, (UninstallString.LengthInChars + 1) * sizeof(TCHAR));
        YoriLibFreeStringContents(&UninstallString);
        if (Err != ERROR_SUCCESS) {
            goto Exit;
        }
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("URLInfoAbout"), 0, REG_SZ, (LPBYTE)_T("http://www.malsmith.net/yori/"), sizeof(_T("http://www.malsmith.net/yori/")));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("URLUpdateInfo"), 0, REG_SZ, (LPBYTE)_T("http://www.malsmith.net/yori/changelog/"), sizeof(_T("http://www.malsmith.net/yori/changelog/")));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("NoModify"), 0, REG_SZ, (LPBYTE)_T("1"), sizeof(_T("1")));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("NoRepair"), 0, REG_SZ, (LPBYTE)_T("1"), sizeof(_T("1")));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

Exit:

    DllAdvApi32.pRegCloseKey(hKey);

    if (Err == ERROR_SUCCESS) {
        return TRUE;
    }
    return FALSE;
}

/**
 Attempt to remove the entry installed to allow control panel to uninstall
 the application.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgRemoveUninstallEntry(VOID)
{
    HKEY hKey;
    DWORD Err;
    DWORD Disposition;

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegDeleteKeyW == NULL) {

        return FALSE;
    }

    Err = DllAdvApi32.pRegCreateKeyExW(HKEY_CURRENT_USER,
                                       _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
                                       0,
                                       NULL,
                                       0,
                                       DELETE,
                                       NULL,
                                       &hKey,
                                       &Disposition);
    if (Err != ERROR_SUCCESS) {
        return FALSE;
    }

    Err = DllAdvApi32.pRegDeleteKeyW(hKey, _T("Yori"));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

Exit:

    DllAdvApi32.pRegCloseKey(hKey);

    if (Err == ERROR_SUCCESS) {
        return TRUE;
    }

    return FALSE;
}

/**
 Attempt to gain access to a system registry key.  This is because some keys
 are restricted to TrustedInstaller inappropriately - they contain
 configuration that users are expected to change, but are ACL'd to prevent
 Administrators changing them.  This code automates changing ownership and ACL
 to allow Administrators to modify the values.

 @param KeyRoot A handle to a registry key, typically one of the well known
        root keys.

 @param KeyName Pointer to a NULL-terminated string specifying the name of the
        key whose ACL should be adjusted.

 @return TRUE if adjustment was successful, FALSE on failure.
 */
BOOL
YoriPkgGetAccessToRegistryKey(
    __in HKEY KeyRoot,
    __in PCYORI_STRING KeyName
    )
{
    BOOL TakeOwnershipPrivilegeEnabled;
    HKEY hKey;
    DWORD Err;
    DWORD Disposition;
    DWORD BytesRequired;
    PSID AdministratorSid;
    PSID UsersSid;
    PACL NewAcl;
    SECURITY_DESCRIPTOR NewDescriptor;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (DllAdvApi32.pAddAccessAllowedAce == NULL ||
        DllAdvApi32.pAllocateAndInitializeSid == NULL ||
        DllAdvApi32.pFreeSid == NULL ||
        DllAdvApi32.pInitializeAcl == NULL ||
        DllAdvApi32.pInitializeSecurityDescriptor == NULL ||
        DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegSetKeySecurity == NULL ||
        DllAdvApi32.pSetSecurityDescriptorDacl == NULL ||
        DllAdvApi32.pSetSecurityDescriptorOwner == NULL) {

        return FALSE;
    }

    ASSERT(YoriLibIsStringNullTerminated(KeyName));
    AdministratorSid = NULL;
    UsersSid = NULL;
    hKey = NULL;
    NewAcl = NULL;

    TakeOwnershipPrivilegeEnabled = YoriLibEnableTakeOwnershipPrivilege();

    //
    //  Get a SID for the Administrators group, which can be used as an owner
    //  or ACE.  Since the key being manipulated is system wide, any
    //  modification should be to include some administrative rights,
    //  otherwise this code would allow a later unprivileged user to modify
    //  login settings.
    //

    if (!DllAdvApi32.pAllocateAndInitializeSid(&NtAuthority,
                                               2,
                                               SECURITY_BUILTIN_DOMAIN_RID,
                                               DOMAIN_ALIAS_RID_ADMINS,
                                               0,
                                               0,
                                               0,
                                               0,
                                               0,
                                               0,
                                               &AdministratorSid)) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    if (!DllAdvApi32.pAllocateAndInitializeSid(&NtAuthority,
                                               2,
                                               SECURITY_BUILTIN_DOMAIN_RID,
                                               DOMAIN_ALIAS_RID_USERS,
                                               0,
                                               0,
                                               0,
                                               0,
                                               0,
                                               0,
                                               &UsersSid)) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    Err = DllAdvApi32.pRegCreateKeyExW(KeyRoot,
                                       KeyName->StartOfString,
                                       0,
                                       NULL,
                                       0,
                                       WRITE_OWNER,
                                       NULL,
                                       &hKey,
                                       &Disposition);
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    //
    //  Set owner to Administrators (as opposed to TrustedInstaller.)
    //

    if (!DllAdvApi32.pInitializeSecurityDescriptor(&NewDescriptor, SECURITY_DESCRIPTOR_REVISION)) {
        Err = GetLastError();
        goto Exit;
    }

    if (!DllAdvApi32.pSetSecurityDescriptorOwner(&NewDescriptor, AdministratorSid, FALSE)) {
        Err = GetLastError();
        goto Exit;
    }

    Err = DllAdvApi32.pRegSetKeySecurity(hKey, OWNER_SECURITY_INFORMATION, &NewDescriptor);
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    //
    //  Close and reopen, this time with WRITE_DAC.  The ownership change
    //  should ensure that this succeeds if the calling process is part of
    //  the Administrators group.
    //

    DllAdvApi32.pRegCloseKey(hKey);
    hKey = NULL;

    Err = DllAdvApi32.pRegCreateKeyExW(KeyRoot,
                                       KeyName->StartOfString,
                                       0,
                                       NULL,
                                       0,
                                       WRITE_DAC,
                                       NULL,
                                       &hKey,
                                       &Disposition);
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

    //
    //  Now adjust the ACL.  This code uses two ACEs, full access for
    //  Administrators and read only access for Users.  This is minimal but
    //  typically appropriate for any kind of system key.
    //

    BytesRequired = sizeof(ACL) + 2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                    DllAdvApi32.pGetLengthSid(AdministratorSid) +
                    DllAdvApi32.pGetLengthSid(UsersSid);

    NewAcl = YoriLibMalloc(BytesRequired);
    if (NewAcl == NULL) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    if (!DllAdvApi32.pInitializeAcl(NewAcl, BytesRequired, ACL_REVISION)) {
        Err = GetLastError();
        goto Exit;
    }

    if (!DllAdvApi32.pAddAccessAllowedAce(NewAcl, ACL_REVISION, KEY_ALL_ACCESS, AdministratorSid)) {
        Err = GetLastError();
        goto Exit;
    }

    if (!DllAdvApi32.pAddAccessAllowedAce(NewAcl, ACL_REVISION, KEY_READ, UsersSid)) {
        Err = GetLastError();
        goto Exit;
    }

    if (!DllAdvApi32.pInitializeSecurityDescriptor(&NewDescriptor, SECURITY_DESCRIPTOR_REVISION)) {
        Err = GetLastError();
        goto Exit;
    }

    if (!DllAdvApi32.pSetSecurityDescriptorDacl(&NewDescriptor, TRUE, NewAcl, FALSE)) {
        Err = GetLastError();
        goto Exit;
    }

    Err = DllAdvApi32.pRegSetKeySecurity(hKey, DACL_SECURITY_INFORMATION, &NewDescriptor);
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

Exit:

    if (hKey != NULL) {
        DllAdvApi32.pRegCloseKey(hKey);
    }

    if (NewAcl != NULL) {
        YoriLibFree(NewAcl);
    }

    if (AdministratorSid != NULL) {
        DllAdvApi32.pFreeSid(AdministratorSid);
    }

    if (UsersSid != NULL) {
        DllAdvApi32.pFreeSid(UsersSid);
    }

    if (Err == ERROR_SUCCESS) {
        return TRUE;
    }
    return FALSE;
}

/**
 Set the logon shell to a new path, specifying the location of the entry
 in the registry.

 @param KeyName Pointer to the registry key to update.

 @param ValueName Pointer to the registry value to update.

 @param NewShellFullPath Pointer to the new login shell.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgUpdateRegistryShell(
    __in PCYORI_STRING KeyName,
    __in PCYORI_STRING ValueName,
    __in PCYORI_STRING NewShellFullPath
    )
{
    HKEY hKey;
    DWORD Err;
    DWORD Disposition;

    ASSERT(YoriLibIsStringNullTerminated(KeyName));
    ASSERT(YoriLibIsStringNullTerminated(ValueName));
    ASSERT(YoriLibIsStringNullTerminated(NewShellFullPath));

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegSetValueExW == NULL) {

        return FALSE;
    }

    Err = DllAdvApi32.pRegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                       KeyName->StartOfString,
                                       0,
                                       NULL,
                                       0,
                                       KEY_QUERY_VALUE | KEY_SET_VALUE,
                                       NULL,
                                       &hKey,
                                       &Disposition);

    //
    //  If we don't have access to change things in the key, try to
    //  obtain access, and retry.
    //

    if (Err == ERROR_ACCESS_DENIED) {
        YoriPkgGetAccessToRegistryKey(HKEY_LOCAL_MACHINE, KeyName);

        Err = DllAdvApi32.pRegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                           KeyName->StartOfString,
                                           0,
                                           NULL,
                                           0,
                                           KEY_QUERY_VALUE | KEY_SET_VALUE,
                                           NULL,
                                           &hKey,
                                           &Disposition);
    }

    if (Err != ERROR_SUCCESS) {
        return FALSE;
    }

    Err = DllAdvApi32.pRegSetValueExW(hKey, ValueName->StartOfString, 0, REG_SZ, (LPBYTE)NewShellFullPath->StartOfString, (NewShellFullPath->LengthInChars + 1) * sizeof(TCHAR));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

Exit:

    DllAdvApi32.pRegCloseKey(hKey);

    if (Err == ERROR_SUCCESS) {
        return TRUE;
    }
    return FALSE;
}

/**
 Save the current logon shell in the registry into a new value so it can
 be restored later.

 @param KeyName Pointer to the registry key to update.

 @param MasterValueName Pointer to the registry value to backup.

 @param BackupValueName Pointer to the registry value to save the previous
        contents to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgBackupRegistryShell(
    __in PCYORI_STRING KeyName,
    __in PCYORI_STRING MasterValueName,
    __in PCYORI_STRING BackupValueName
    )
{
    HKEY hKey;
    DWORD Err;
    DWORD Disposition;
    YORI_STRING ExistingValue;
    DWORD LengthRequired;

    ASSERT(YoriLibIsStringNullTerminated(KeyName));
    ASSERT(YoriLibIsStringNullTerminated(MasterValueName));
    ASSERT(YoriLibIsStringNullTerminated(BackupValueName));

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegCreateKeyExW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllAdvApi32.pRegSetValueExW == NULL) {

        return FALSE;
    }

    YoriLibInitEmptyString(&ExistingValue);

    Err = DllAdvApi32.pRegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                       KeyName->StartOfString,
                                       0,
                                       NULL,
                                       0,
                                       KEY_QUERY_VALUE | KEY_SET_VALUE,
                                       NULL,
                                       &hKey,
                                       &Disposition);

    //
    //  If we don't have access to change things in the key, try to
    //  obtain access, and retry.
    //

    if (Err == ERROR_ACCESS_DENIED) {
        YoriPkgGetAccessToRegistryKey(HKEY_LOCAL_MACHINE, KeyName);

        Err = DllAdvApi32.pRegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                           KeyName->StartOfString,
                                           0,
                                           NULL,
                                           0,
                                           KEY_QUERY_VALUE | KEY_SET_VALUE,
                                           NULL,
                                           &hKey,
                                           &Disposition);
    }

    if (Err != ERROR_SUCCESS) {
        return FALSE;
    }


    //
    //  First, query the backup value.  If it exists, return success.
    //  This is an intentional policy choice: the goal of this code is
    //  to allow a system to be restored to a pre-Yori configuration,
    //  not to restore it to a previous modification.
    //

    LengthRequired = 0;
    Err = DllAdvApi32.pRegQueryValueExW(hKey, BackupValueName->StartOfString, NULL, NULL, NULL, &LengthRequired);
    if (Err == ERROR_MORE_DATA || LengthRequired > 0) {
        DllAdvApi32.pRegCloseKey(hKey);
        return TRUE;
    }

    //
    //  Now query the master value, so it can be written into the backup
    //  value.
    //

    LengthRequired = 0;
    Err = DllAdvApi32.pRegQueryValueExW(hKey, MasterValueName->StartOfString, NULL, NULL, NULL, &LengthRequired);
    if (Err == ERROR_MORE_DATA || LengthRequired > 0) {
        if (!YoriLibAllocateString(&ExistingValue, (LengthRequired / sizeof(TCHAR)) + 1)) {
            goto Exit;
        }

        Err = DllAdvApi32.pRegQueryValueExW(hKey, MasterValueName->StartOfString, NULL, NULL, (LPBYTE)ExistingValue.StartOfString, &LengthRequired);
        if (Err != ERROR_SUCCESS) {
            goto Exit;
        }

        ExistingValue.LengthInChars = LengthRequired / sizeof(TCHAR) - 1;
    }

    //
    //  Save the master value into the backup value.
    //

    Err = DllAdvApi32.pRegSetValueExW(hKey, BackupValueName->StartOfString, 0, REG_SZ, (LPBYTE)ExistingValue.StartOfString, (ExistingValue.LengthInChars + 1) * sizeof(TCHAR));
    if (Err != ERROR_SUCCESS) {
        goto Exit;
    }

Exit:

    YoriLibFreeStringContents(&ExistingValue);
    DllAdvApi32.pRegCloseKey(hKey);

    if (Err == ERROR_SUCCESS) {
        return TRUE;
    }
    return FALSE;
}

/**
 Set the logon shell to a new path.  This involves detecting Server Core or a
 full GUI server based on it, and in that case, updating the AvailableShells
 key.  If a different edition, update the regular Shell value.

 @param NewShellFullPath Pointer to the new login shell.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgUpdateLogonShell(
    __in PYORI_STRING NewShellFullPath
    )
{
    YORI_STRING KeyName;
    YORI_STRING ValueName;
    YORI_STRING BackupValueName;
    DWORD Err;
    HKEY hKey;

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegOpenKeyExW == NULL) {

        return FALSE;
    }

    //
    //  Check if we're running on a system with Server Core shell support,
    //  where multiple shells are listed in ranked order.  If so, insert
    //  the new entry under that key.  If not, use the one-and-only shell
    //  key instead.
    //

    YoriLibConstantString(&KeyName, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\AlternateShells\\AvailableShells"));
    YoriLibConstantString(&ValueName, _T("98052"));

    Err = DllAdvApi32.pRegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                     KeyName.StartOfString,
                                     0,
                                     KEY_QUERY_VALUE,
                                     &hKey);

    if (Err == ERROR_SUCCESS) {
        DllAdvApi32.pRegCloseKey(hKey);
    } else {
        YoriLibConstantString(&KeyName, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"));
        YoriLibConstantString(&ValueName, _T("Shell"));
        YoriLibConstantString(&BackupValueName, _T("YoriBackupShell"));
        if (!YoriPkgBackupRegistryShell(&KeyName, &ValueName, &BackupValueName)) {
            return FALSE;
        }

    }
    return YoriPkgUpdateRegistryShell(&KeyName, &ValueName, NewShellFullPath);
}

/**
 Set settings for the console in the user's registry.

 @param ConsoleTitle Optionally points to a string containing the console
        title to apply these settings to.  If not specified, the global
        default console values are changed.

 @param ColorTable Pointer to an array of the RGB values for the base 16
        colors.

 @param WindowColor Specifies the default window color.

 @param PopupColor Specifies the popup color.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgSetConsoleDefaults(
    __in_opt PCYORI_STRING ConsoleTitle,
    __in_ecount(16) COLORREF* ColorTable,
    __in UCHAR WindowColor,
    __in UCHAR PopupColor
    )
{
    YORI_STRING KeyName;
    TCHAR ValueNameBuffer[16];
    YORI_STRING ValueName;
    DWORD Err;
    DWORD Index;
    DWORD Temp;
    TCHAR Char;
    HKEY hKey;
    DWORD Disp;

    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegOpenKeyExW == NULL) {

        return FALSE;
    }

    //
    //  Construct the registry key which is either "Console" (for user
    //  default) or "Console\Title" (for a specific program.)  The default
    //  title is just the path to the program, although that can't be
    //  described in the registry since it contains path seperators; the
    //  registry format substitutes underscores for path seperators.
    //

    if (ConsoleTitle != NULL && ConsoleTitle->LengthInChars != 0) {
        YoriLibInitEmptyString(&KeyName);
        if (!YoriLibAllocateString(&KeyName, sizeof("Console\\") + ConsoleTitle->LengthInChars)) {
            return FALSE;
        }
        KeyName.LengthInChars = YoriLibSPrintfS(KeyName.StartOfString, KeyName.LengthAllocated, _T("Console\\"));
        for (Index = 0; Index < ConsoleTitle->LengthInChars; Index++) {
            Char = ConsoleTitle->StartOfString[Index];
            if (Char == '\\') {
                Char = '_';
            }
            KeyName.StartOfString[KeyName.LengthInChars + Index] = Char;
        }

        KeyName.LengthInChars = KeyName.LengthInChars + Index;
        KeyName.StartOfString[KeyName.LengthInChars] = '\0';
    } else {
        YoriLibConstantString(&KeyName, _T("Console"));
    }

    Err = DllAdvApi32.pRegCreateKeyExW(HKEY_CURRENT_USER,
                                       KeyName.StartOfString,
                                       0,
                                       NULL,
                                       0,
                                       KEY_SET_VALUE,
                                       NULL,
                                       &hKey,
                                       &Disp);

    YoriLibFreeStringContents(&KeyName);
    if (Err != ERROR_SUCCESS) {
        return FALSE;
    }

    YoriLibInitEmptyString(&ValueName);
    ValueName.StartOfString = ValueNameBuffer;
    ValueName.LengthAllocated = sizeof(ValueNameBuffer)/sizeof(ValueNameBuffer[0]);

    for (Index = 0; Index < 16; Index++) {
        ValueName.LengthInChars = YoriLibSPrintfS(ValueName.StartOfString, ValueName.LengthAllocated, _T("ColorTable%02i"), Index);
        Err = DllAdvApi32.pRegSetValueExW(hKey, ValueName.StartOfString, 0, REG_DWORD, (LPBYTE)&ColorTable[Index], sizeof(DWORD));
        if (Err != ERROR_SUCCESS) {
            break;
        }
    }

    if (Err != ERROR_SUCCESS) {
        DllAdvApi32.pRegCloseKey(hKey);
        return FALSE;
    }

    Temp = WindowColor;
    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("ScreenColors"), 0, REG_DWORD, (LPBYTE)&Temp, sizeof(DWORD));

    if (Err != ERROR_SUCCESS) {
        DllAdvApi32.pRegCloseKey(hKey);
        return FALSE;
    }

    Temp = PopupColor;
    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("PopupColors"), 0, REG_DWORD, (LPBYTE)&Temp, sizeof(DWORD));

    if (Err != ERROR_SUCCESS) {
        DllAdvApi32.pRegCloseKey(hKey);
        return FALSE;
    }

    Temp = ColorTable[WindowColor & 0x0F];
    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("DefaultForeground"), 0, REG_DWORD, (LPBYTE)&Temp, sizeof(DWORD));

    if (Err != ERROR_SUCCESS) {
        DllAdvApi32.pRegCloseKey(hKey);
        return FALSE;
    }

    Temp = ColorTable[(WindowColor & 0xF0) >> 4];
    Err = DllAdvApi32.pRegSetValueExW(hKey, _T("DefaultBackground"), 0, REG_DWORD, (LPBYTE)&Temp, sizeof(DWORD));

    if (Err != ERROR_SUCCESS) {
        DllAdvApi32.pRegCloseKey(hKey);
        return FALSE;
    }

    DllAdvApi32.pRegCloseKey(hKey);
    return TRUE;
}

// vim:sw=4:ts=4:et:
