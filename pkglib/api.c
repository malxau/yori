/**
 * @file pkglib/api.c
 *
 * Yori shell functions exported out of this module
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
#include "yoripkg.h"
#include "yoripkgp.h"

/**
 Upgrade all installed packages in the system.

 @param NewArchitecture Optionally points to the new architecture to apply.
        If not specified, the current architecture is retained.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgUpgradeInstalledPackages(
    __in_opt PYORI_STRING NewArchitecture
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING InstalledSection;
    LPTSTR ThisLine;
    LPTSTR Equals;
    YORI_STRING PkgNameOnly;
    YORI_STRING InstalledVersion;
    YORI_STRING UpgradePath;
    DWORD LineLength;
    DWORD Error;
    BOOL Result;
    BOOL UpgradeThisPackage;
    YORIPKG_PACKAGES_PENDING_INSTALL PendingPackages;

    if (!YoriPkgInitializePendingPackages(&PendingPackages)) {
        return FALSE;
    }

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        return FALSE;
    }

    if (!YoriLibAllocateString(&InstalledSection, YORIPKG_MAX_SECTION_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    if (!YoriLibAllocateString(&UpgradePath, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&InstalledSection);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    InstalledSection.LengthInChars = GetPrivateProfileSection(_T("Installed"), InstalledSection.StartOfString, InstalledSection.LengthAllocated, PkgIniFile.StartOfString);

    YoriLibInitEmptyString(&PkgNameOnly);
    ThisLine = InstalledSection.StartOfString;

    Result = FALSE;
    while (*ThisLine != '\0') {
        LineLength = _tcslen(ThisLine);
        PkgNameOnly.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        if (Equals != NULL) {
            PkgNameOnly.LengthInChars = (DWORD)(Equals - ThisLine);
            *Equals = '\0';
            YoriLibConstantString(&InstalledVersion, Equals + 1);
        } else {
            PkgNameOnly.LengthInChars = LineLength;
            YoriLibInitEmptyString(&InstalledVersion);
        }

        UpgradePath.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, _T("UpgradePath"), _T(""), UpgradePath.StartOfString, UpgradePath.LengthAllocated, PkgIniFile.StartOfString);
        if (UpgradePath.LengthInChars > 0) {
            UpgradeThisPackage = TRUE;
            if (NewArchitecture != NULL) {
                YoriPkgBuildUpgradeLocationForNewArchitecture(&PkgNameOnly, NewArchitecture, &PkgIniFile, &UpgradePath);
            } else if (!YoriPkgIsNewerVersionAvailable(&PendingPackages, &PkgIniFile, &UpgradePath, &InstalledVersion)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y version %y is already installed\n"), &PkgNameOnly, &InstalledVersion);
                UpgradeThisPackage = FALSE;
            }
            if (UpgradeThisPackage) {
                if (YoriLibIsPathUrl(&UpgradePath)) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Downloading %y...\n"), &UpgradePath);
                }
                Error = YoriPkgPreparePackageForInstall(&PkgIniFile, NULL, &PendingPackages, &UpgradePath);
                if (Error != ERROR_SUCCESS) {
                    YoriPkgDisplayErrorStringForInstallFailure(Error);
                    goto Exit;
                }
            }
        }
        if (Equals) {
            *Equals = '=';
        }

        ThisLine += LineLength;
        ThisLine++;
    }

    //
    //  Upgrade all packages which specify an upgrade path.
    //

    Result = YoriPkgInstallPendingPackages(&PkgIniFile, NULL, &PendingPackages);

Exit:

    //
    //  If there's any backup left, abort the install of those packages.
    //

    if (!YoriLibIsListEmpty(&PendingPackages.BackupPackages)) {
        YoriPkgRollbackAndFreeBackupPackageList(&PkgIniFile, NULL, &PendingPackages.BackupPackages);
    }

    YoriPkgDeletePendingPackages(&PendingPackages);

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&InstalledSection);
    YoriLibFreeStringContents(&UpgradePath);

    return TRUE;
}

/**
 Upgrade a single package installed on the system.

 @param PackageName The name of the package to upgrade.

 @param NewArchitecture Optionally points to the new architecture to apply.
        If not specified, the current architecture is retained.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgUpgradeSinglePackage(
    __in PYORI_STRING PackageName,
    __in_opt PYORI_STRING NewArchitecture
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING IniValue;
    BOOL Result;
    DWORD Error;
    YORIPKG_PACKAGES_PENDING_INSTALL PendingPackages;

    if (!YoriPkgInitializePendingPackages(&PendingPackages)) {
        return FALSE;
    }

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(_T("Installed"), PackageName->StartOfString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);
    if (IniValue.LengthInChars == 0) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y is not installed\n"), PackageName);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, _T("UpgradePath"), _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);

    if (IniValue.LengthInChars == 0) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y does not specify an upgrade path\n"), PackageName);
        return FALSE;
    }

    if (NewArchitecture != NULL) {
        YoriPkgBuildUpgradeLocationForNewArchitecture(PackageName, NewArchitecture, &PkgIniFile, &IniValue);
    }

    Result = FALSE;
    if (YoriLibIsPathUrl(&IniValue)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Downloading %y...\n"), &IniValue);
    }
    Error = YoriPkgPreparePackageForInstall(&PkgIniFile, NULL, &PendingPackages, &IniValue);
    if (Error != ERROR_SUCCESS) {
        YoriPkgDisplayErrorStringForInstallFailure(Error);
        goto Exit;
    }

    Result = YoriPkgInstallPendingPackages(&PkgIniFile, NULL, &PendingPackages);

Exit:
    if (!YoriLibIsListEmpty(&PendingPackages.BackupPackages)) {
        YoriPkgRollbackAndFreeBackupPackageList(&PkgIniFile, NULL, &PendingPackages.BackupPackages);
    }

    YoriPkgDeletePendingPackages(&PendingPackages);

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&IniValue);

    return Result;
}

/**
 Install a single package from a specified path to a package.

 @param PackagePath The path of the package file to install.

 @param TargetDirectory Pointer to a string specifying the directory to
        install the package.  If NULL, the directory containing the
        application is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgInstallSinglePackage(
    __in PYORI_STRING PackagePath,
    __in_opt PYORI_STRING TargetDirectory
    )
{
    YORI_STRING PkgIniFile;
    BOOL Result;
    DWORD Error;
    YORIPKG_PACKAGES_PENDING_INSTALL PendingPackages;

    if (!YoriPkgInitializePendingPackages(&PendingPackages)) {
        return FALSE;
    }

    if (!YoriPkgGetPackageIniFile(TargetDirectory, &PkgIniFile)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        return FALSE;
    }

    Result = FALSE;
    if (YoriLibIsPathUrl(PackagePath)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Downloading %y...\n"), PackagePath);
    }
    Error = YoriPkgPreparePackageForInstall(&PkgIniFile, TargetDirectory, &PendingPackages, PackagePath);
    if (Error != ERROR_SUCCESS) {
        YoriPkgDisplayErrorStringForInstallFailure(Error);
        goto Exit;
    }

    Result = YoriPkgInstallPendingPackages(&PkgIniFile, TargetDirectory, &PendingPackages);

Exit:
    if (!YoriLibIsListEmpty(&PendingPackages.BackupPackages)) {
        YoriPkgRollbackAndFreeBackupPackageList(&PkgIniFile, TargetDirectory, &PendingPackages.BackupPackages);
    }

    YoriPkgDeletePendingPackages(&PendingPackages);

    YoriLibFreeStringContents(&PkgIniFile);

    return Result;
}


/**
 Install source for all installed packages in the system.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgInstallSourceForInstalledPackages(
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING InstalledSection;
    LPTSTR ThisLine;
    LPTSTR Equals;
    YORI_STRING PkgNameOnly;
    YORI_STRING SourcePath;
    DWORD LineLength;
    DWORD Error;
    BOOL Result;
    YORIPKG_PACKAGES_PENDING_INSTALL PendingPackages;

    if (!YoriPkgInitializePendingPackages(&PendingPackages)) {
        return FALSE;
    }

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        return FALSE;
    }

    if (!YoriLibAllocateString(&InstalledSection, YORIPKG_MAX_SECTION_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    if (!YoriLibAllocateString(&SourcePath, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&InstalledSection);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    InstalledSection.LengthInChars = GetPrivateProfileSection(_T("Installed"), InstalledSection.StartOfString, InstalledSection.LengthAllocated, PkgIniFile.StartOfString);

    YoriLibInitEmptyString(&PkgNameOnly);
    ThisLine = InstalledSection.StartOfString;

    Result = FALSE;
    while (*ThisLine != '\0') {
        LineLength = _tcslen(ThisLine);
        PkgNameOnly.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        if (Equals != NULL) {
            PkgNameOnly.LengthInChars = (DWORD)(Equals - ThisLine);
            *Equals = '\0';
        } else {
            PkgNameOnly.LengthInChars = LineLength;
        }

        SourcePath.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, _T("SourcePath"), _T(""), SourcePath.StartOfString, SourcePath.LengthAllocated, PkgIniFile.StartOfString);
        if (SourcePath.LengthInChars > 0) {
            if (YoriLibIsPathUrl(&SourcePath)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Downloading source for %y from %y...\n"), &PkgNameOnly, &SourcePath);
            }
            Error = YoriPkgPreparePackageForInstall(&PkgIniFile, NULL, &PendingPackages, &SourcePath);
            if (Error != ERROR_SUCCESS) {
                YoriPkgDisplayErrorStringForInstallFailure(Error);
                goto Exit;
            }
        }
        if (Equals) {
            *Equals = '=';
        }

        ThisLine += LineLength;
        ThisLine++;
    }

    //
    //  Install all packages which specify a source path.
    //

    Result = YoriPkgInstallPendingPackages(&PkgIniFile, NULL, &PendingPackages);

Exit:

    //
    //  If there's any backup left, abort the install of those packages.
    //

    if (!YoriLibIsListEmpty(&PendingPackages.BackupPackages)) {
        YoriPkgRollbackAndFreeBackupPackageList(&PkgIniFile, NULL, &PendingPackages.BackupPackages);
    }

    YoriPkgDeletePendingPackages(&PendingPackages);

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&InstalledSection);
    YoriLibFreeStringContents(&SourcePath);

    return TRUE;
}

/**
 Install source for a single package installed on the system.

 @param PackageName The name of the package to obtain source for.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgInstallSourceForSinglePackage(
    __in PYORI_STRING PackageName
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING IniValue;
    BOOL Result;
    DWORD Error;
    YORIPKG_PACKAGES_PENDING_INSTALL PendingPackages;

    if (!YoriPkgInitializePendingPackages(&PendingPackages)) {
        return FALSE;
    }

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(_T("Installed"), PackageName->StartOfString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);
    if (IniValue.LengthInChars == 0) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y is not installed\n"), PackageName);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, _T("SourcePath"), _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);

    if (IniValue.LengthInChars == 0) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y does not specify a source path\n"), PackageName);
        return FALSE;
    }

    Result = FALSE;
    if (YoriLibIsPathUrl(&IniValue)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Downloading %y...\n"), &IniValue);
    }
    Error = YoriPkgPreparePackageForInstall(&PkgIniFile, NULL, &PendingPackages, &IniValue);
    if (Error != ERROR_SUCCESS) {
        YoriPkgDisplayErrorStringForInstallFailure(Error);
        goto Exit;
    }

    Result = YoriPkgInstallPendingPackages(&PkgIniFile, NULL, &PendingPackages);

Exit:
    if (!YoriLibIsListEmpty(&PendingPackages.BackupPackages)) {
        YoriPkgRollbackAndFreeBackupPackageList(&PkgIniFile, NULL, &PendingPackages.BackupPackages);
    }

    YoriPkgDeletePendingPackages(&PendingPackages);

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&IniValue);

    return Result;
}

/**
 Install symbols for all installed packages in the system.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgInstallSymbolsForInstalledPackages(
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING InstalledSection;
    LPTSTR ThisLine;
    LPTSTR Equals;
    YORI_STRING PkgNameOnly;
    YORI_STRING SymbolPath;
    DWORD LineLength;
    DWORD TotalCount;
    DWORD Error;
    BOOL Result;
    YORIPKG_PACKAGES_PENDING_INSTALL PendingPackages;

    if (!YoriPkgInitializePendingPackages(&PendingPackages)) {
        return FALSE;
    }

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        return FALSE;
    }

    if (!YoriLibAllocateString(&InstalledSection, YORIPKG_MAX_SECTION_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    if (!YoriLibAllocateString(&SymbolPath, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&InstalledSection);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    InstalledSection.LengthInChars = GetPrivateProfileSection(_T("Installed"), InstalledSection.StartOfString, InstalledSection.LengthAllocated, PkgIniFile.StartOfString);

    YoriLibInitEmptyString(&PkgNameOnly);
    ThisLine = InstalledSection.StartOfString;

    TotalCount = 0;
    Result = FALSE;
    while (*ThisLine != '\0') {
        LineLength = _tcslen(ThisLine);
        PkgNameOnly.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        if (Equals != NULL) {
            PkgNameOnly.LengthInChars = (DWORD)(Equals - ThisLine);
            *Equals = '\0';
        } else {
            PkgNameOnly.LengthInChars = LineLength;
        }

        SymbolPath.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, _T("SymbolPath"), _T(""), SymbolPath.StartOfString, SymbolPath.LengthAllocated, PkgIniFile.StartOfString);
        if (SymbolPath.LengthInChars > 0) {
            if (YoriLibIsPathUrl(&SymbolPath)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Downloading symbols for %y from %y...\n"), &PkgNameOnly, &SymbolPath);
            }
            Error = YoriPkgPreparePackageForInstall(&PkgIniFile, NULL, &PendingPackages, &SymbolPath);
            if (Error != ERROR_SUCCESS) {
                YoriPkgDisplayErrorStringForInstallFailure(Error);
                goto Exit;
            }
            TotalCount++;
        }
        if (Equals) {
            *Equals = '=';
        }

        ThisLine += LineLength;
        ThisLine++;
    }

    //
    //  Install all packages which specify a source path.
    //

    Result = YoriPkgInstallPendingPackages(&PkgIniFile, NULL, &PendingPackages);

Exit:

    //
    //  If there's any backup left, abort the install of those packages.
    //

    if (!YoriLibIsListEmpty(&PendingPackages.BackupPackages)) {
        YoriPkgRollbackAndFreeBackupPackageList(&PkgIniFile, NULL, &PendingPackages.BackupPackages);
    }

    YoriPkgDeletePendingPackages(&PendingPackages);

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&InstalledSection);
    YoriLibFreeStringContents(&SymbolPath);

    return TRUE;
}

/**
 Install symbols for a single package installed on the system.

 @param PackageName The name of the package to obtain source for.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgInstallSymbolForSinglePackage(
    __in PYORI_STRING PackageName
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING IniValue;
    DWORD Error;
    BOOL Result;
    YORIPKG_PACKAGES_PENDING_INSTALL PendingPackages;

    if (!YoriPkgInitializePendingPackages(&PendingPackages)) {
        return FALSE;
    }

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(_T("Installed"), PackageName->StartOfString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);
    if (IniValue.LengthInChars == 0) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y is not installed\n"), PackageName);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, _T("SymbolPath"), _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);

    if (IniValue.LengthInChars == 0) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y does not specify a source path\n"), PackageName);
        return FALSE;
    }

    Result = FALSE;
    if (YoriLibIsPathUrl(&IniValue)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Downloading %y...\n"), &IniValue);
    }
    Error = YoriPkgPreparePackageForInstall(&PkgIniFile, NULL, &PendingPackages, &IniValue);
    if (Error != ERROR_SUCCESS) {
        YoriPkgDisplayErrorStringForInstallFailure(Error);
        goto Exit;
    }

    Result = YoriPkgInstallPendingPackages(&PkgIniFile, NULL, &PendingPackages);

Exit:
    if (!YoriLibIsListEmpty(&PendingPackages.BackupPackages)) {
        YoriPkgRollbackAndFreeBackupPackageList(&PkgIniFile, NULL, &PendingPackages.BackupPackages);
    }

    YoriPkgDeletePendingPackages(&PendingPackages);

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&IniValue);

    return Result;
}

/**
 List all installed packages in the system.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgListInstalledPackages()
{
    YORI_STRING PkgIniFile;
    YORI_STRING InstalledSection;
    LPTSTR ThisLine;
    LPTSTR Equals;
    YORI_STRING PkgNameOnly;

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&InstalledSection, YORIPKG_MAX_SECTION_LENGTH)) {
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    InstalledSection.LengthInChars = GetPrivateProfileSection(_T("Installed"), InstalledSection.StartOfString, InstalledSection.LengthAllocated, PkgIniFile.StartOfString);

    YoriLibInitEmptyString(&PkgNameOnly);
    ThisLine = InstalledSection.StartOfString;

    while (*ThisLine != '\0') {
        PkgNameOnly.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        if (Equals != NULL) {
            PkgNameOnly.LengthInChars = (DWORD)(Equals - ThisLine);
        } else {
            PkgNameOnly.LengthInChars = _tcslen(ThisLine);
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &PkgNameOnly);
        ThisLine += _tcslen(ThisLine);
        ThisLine++;
    }

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&InstalledSection);

    return TRUE;
}


// vim:sw=4:ts=4:et:
