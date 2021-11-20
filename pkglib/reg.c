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

 @param TargetDirectory Pointer to the directory to append to a path.

 @param AppendToUserPath If TRUE, the directory should be appended to the
        user's path environment variable.

 @param AppendToSystemPath If TRUE, the directory should be appended to the
        system's path environment variable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgAppendInstallDirToPath(
    __in PCYORI_STRING TargetDirectory,
    __in BOOL AppendToUserPath,
    __in BOOL AppendToSystemPath
    )
{
    BOOL Result = TRUE;

    YoriLibLoadAdvApi32Functions();
    YoriLibLoadUser32Functions();

    if (AppendToUserPath) {
        if (!YoriPkgAppendPath(HKEY_CURRENT_USER, _T("Environment"), _T("Path"), TargetDirectory)) {
            Result = FALSE;
        }
    }

    if (AppendToSystemPath) {
        if (!YoriPkgAppendPath(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), _T("Path"), TargetDirectory)) {
            Result = FALSE;
        }
    }

    if (DllUser32.pSendMessageTimeoutW != NULL &&
        (AppendToUserPath || AppendToSystemPath)) {
        DWORD_PTR NotifyResult;
        DllUser32.pSendMessageTimeoutW(HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)_T("Environment"), SMTO_NORMAL, 200, &NotifyResult);
    }

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

// vim:sw=4:ts=4:et:
