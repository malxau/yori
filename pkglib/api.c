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
    YORI_STRING RedirectedPath;
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
            YoriLibInitEmptyString(&RedirectedPath);
            if (NewArchitecture != NULL) {
                YoriPkgBuildUpgradeLocationForNewArchitecture(&PkgNameOnly, NewArchitecture, &PkgIniFile, &UpgradePath);
            } else {
                if (!YoriPkgIsNewerVersionAvailable(&PendingPackages, &PkgIniFile, &UpgradePath, &InstalledVersion, &RedirectedPath)) {
                    YoriLibFreeStringContents(&RedirectedPath);
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y version %y is already installed\n"), &PkgNameOnly, &InstalledVersion);
                    UpgradeThisPackage = FALSE;
                }
            }
            if (UpgradeThisPackage) {
                if (RedirectedPath.LengthInChars > 0) {
                    Error = YoriPkgPreparePackageForInstallRedirectBuild(&PkgIniFile, NULL, &PendingPackages, &RedirectedPath);
                    YoriLibFreeStringContents(&RedirectedPath);
                } else {
                    Error = YoriPkgPreparePackageForInstallRedirectBuild(&PkgIniFile, NULL, &PendingPackages, &UpgradePath);
                }
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
    Error = YoriPkgPreparePackageForInstallRedirectBuild(&PkgIniFile, NULL, &PendingPackages, &IniValue);
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
    Error = YoriPkgPreparePackageForInstall(&PkgIniFile, TargetDirectory, &PendingPackages, PackagePath, NULL);
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
            Error = YoriPkgPreparePackageForInstall(&PkgIniFile, NULL, &PendingPackages, &SourcePath, NULL);
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
    Error = YoriPkgPreparePackageForInstall(&PkgIniFile, NULL, &PendingPackages, &IniValue, NULL);
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
            Error = YoriPkgPreparePackageForInstall(&PkgIniFile, NULL, &PendingPackages, &SymbolPath, NULL);
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
    Error = YoriPkgPreparePackageForInstall(&PkgIniFile, NULL, &PendingPackages, &IniValue, NULL);
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

 @param Verbose If TRUE, display package version and architecture in addition
        to name.  If FALSE, only package names are displayed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgListInstalledPackages(
    __in BOOL Verbose
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING InstalledSection;
    LPTSTR ThisLine;
    LPTSTR Equals;
    YORI_STRING PkgNameOnly;
    YORI_STRING PkgVersion;
    YORI_STRING PkgArch;

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&InstalledSection, YORIPKG_MAX_SECTION_LENGTH)) {
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    if (!YoriLibAllocateString(&PkgArch, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&InstalledSection);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    InstalledSection.LengthInChars = GetPrivateProfileSection(_T("Installed"), InstalledSection.StartOfString, InstalledSection.LengthAllocated, PkgIniFile.StartOfString);

    YoriLibInitEmptyString(&PkgNameOnly);
    YoriLibInitEmptyString(&PkgVersion);
    ThisLine = InstalledSection.StartOfString;

    while (*ThisLine != '\0') {
        PkgNameOnly.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        if (Equals != NULL) {
            PkgNameOnly.LengthInChars = (DWORD)(Equals - ThisLine);
            PkgVersion.StartOfString = Equals + 1;
            PkgVersion.LengthInChars = _tcslen(Equals + 1);
        } else {
            PkgNameOnly.LengthInChars = _tcslen(ThisLine);
            PkgVersion.StartOfString = NULL;
            PkgVersion.LengthInChars = 0;
        }

        ThisLine += _tcslen(ThisLine);
        ThisLine++;

        PkgNameOnly.StartOfString[PkgNameOnly.LengthInChars] = '\0';

        PkgArch.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, _T("Architecture"), _T(""), PkgArch.StartOfString, PkgArch.LengthAllocated, PkgIniFile.StartOfString);

        if (Verbose) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y %y (%y)\n"), &PkgNameOnly, &PkgVersion, &PkgArch);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &PkgNameOnly);
        }
    }

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&InstalledSection);
    YoriLibFreeStringContents(&PkgArch);

    return TRUE;
}

/**
 Delete a specified package from the system.

 @param TargetDirectory Pointer to a string specifying the directory
        containing the package.  If NULL, the directory containing the
        application is used.

 @param PackageName Specifies the package name to delete.

 @param WarnIfNotInstalled If TRUE, display to the console indication if the
        package is not installed.  If FALSE, silently fail the operation.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgDeletePackage(
    __in_opt PYORI_STRING TargetDirectory,
    __in PYORI_STRING PackageName,
    __in BOOL WarnIfNotInstalled
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING IniValue;
    DWORD FileCount;
    BOOL Result;

    if (!YoriPkgGetPackageIniFile(TargetDirectory, &PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(_T("Installed"), PackageName->StartOfString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);
    if (IniValue.LengthInChars == 0) {
        if (WarnIfNotInstalled) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not an installed package\n"), PackageName);
        }
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    FileCount = GetPrivateProfileInt(PackageName->StartOfString, _T("FileCount"), 0, PkgIniFile.StartOfString);
    if (FileCount == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y contains nothing to remove\n"), PackageName);
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    Result = YoriPkgDeletePackageInternal(&PkgIniFile, TargetDirectory, PackageName, FALSE);
    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&IniValue);
    return Result;
}

/**
 Delete all installed packages from the system.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgDeleteAllPackages(
    )
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

        ThisLine += _tcslen(ThisLine);
        ThisLine++;

        PkgNameOnly.StartOfString[PkgNameOnly.LengthInChars] = '\0';
        if (!YoriPkgDeletePackageInternal(&PkgIniFile, NULL, &PkgNameOnly, TRUE)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not remove package %y\n"), &PkgNameOnly);
            break;
        }
    }

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&InstalledSection);

    return TRUE;
}

/**
 Register a pseudo package.  This is a collection of files which is being
 tracked but is otherwise installed outside of this application.  Note in
 particular that paths may be fully specified as opposed to relative to the
 installation root.  These are currently distinguished via the \\?\ prefix;
 ie., a full path must be a prefixed path.

 @param Name Pointer to the name of the package to register.

 @param Version Pointer to the version of the package to register.

 @param Architecture Pointer to the architecture of the package to register.

 @param FileArray Pointer to an array of strings corresponding to the files
        in the package.

 @param FileCount The number of files in the FileArray.

 @param TargetDirectory Optionally points to an installation location.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgInstallPseudoPackage(
    __in PYORI_STRING Name,
    __in PYORI_STRING Version,
    __in PYORI_STRING Architecture,
    __in PYORI_STRING FileArray,
    __in DWORD FileCount,
    __in_opt PYORI_STRING TargetDirectory
    )
{
    YORI_STRING PkgIniFile;
    DWORD FileIndex;
    TCHAR FileIndexString[16];

    if (!YoriPkgGetPackageIniFile(TargetDirectory, &PkgIniFile)) {
        return FALSE;
    }

    WritePrivateProfileString(_T("Installed"), Name->StartOfString, Version->StartOfString, PkgIniFile.StartOfString);
    WritePrivateProfileString(Name->StartOfString, _T("Version"), Version->StartOfString, PkgIniFile.StartOfString);
    WritePrivateProfileString(Name->StartOfString, _T("Architecture"), Architecture->StartOfString, PkgIniFile.StartOfString);

    for (FileIndex = 1; FileIndex <= FileCount; FileIndex++) {
        YoriLibSPrintf(FileIndexString, _T("File%i"), FileIndex);

        WritePrivateProfileString(Name->StartOfString, FileIndexString, FileArray[FileIndex - 1].StartOfString, PkgIniFile.StartOfString);
    }
    YoriLibSPrintf(FileIndexString, _T("%i"), FileCount);
    WritePrivateProfileString(Name->StartOfString, _T("FileCount"), FileIndexString, PkgIniFile.StartOfString);


    YoriLibFreeStringContents(&PkgIniFile);

    return TRUE;
}

/**
 Remove all installed packages, remove the application directory from the
 path, and attempt to remove the package manager, packages INI file, and
 directory on the next reboot.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgUninstallAll(
    )
{
    YORI_STRING TargetDirectory;
    YORI_STRING ExecutableFile;
    YORI_STRING PkgIniFile;
    BOOLEAN DelayDeleteFailed;

    //
    //  TODO: Add routine in reg.c to check for a pending delete on reboot
    //  and not allow a reinstallation if it's present
    //

    if (!YoriPkgDeleteAllPackages()) {
        return FALSE;
    }
    if (!YoriPkgGetApplicationDirectory(&TargetDirectory)) {
        return FALSE;
    }
    if (!YoriPkgGetPackageIniFile(&TargetDirectory, &PkgIniFile)) {
        YoriLibFreeStringContents(&TargetDirectory);
        return FALSE;
    }

    //
    //  Note this can fail if the current user doesn't have access to the
    //  system wide path, which is probably benign.  If the packages have
    //  been deleted above, that implies the user has sufficient access
    //  to remove this installation.  Ideally this would check whether the
    //  system path contains a component to remove, and only fail if it's
    //  there.
    //

    if (!YoriPkgRemoveInstallDirFromPath(&TargetDirectory, TRUE, TRUE)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is present in the path and could not be removed.  This requires manual removal.\n"), &TargetDirectory);
    }

    if (!YoriPkgGetExecutableFile(&ExecutableFile)) {
        YoriLibFreeStringContents(&TargetDirectory);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    YoriPkgRemoveUninstallEntry();

    if (!DeleteFile(PkgIniFile.StartOfString)) {
        return FALSE;
    }

    //
    //  Deleting on reboot requires admin, but an unprivileged user can
    //  install yori and then be unable to delete it since it's in use.
    //  Indicate to the user if there are files that can't be deleted
    //  automatically.
    //

    DelayDeleteFailed = FALSE;
    if (!MoveFileEx(ExecutableFile.StartOfString, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
        DelayDeleteFailed = TRUE;
    }

    if (!MoveFileEx(TargetDirectory.StartOfString, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
        DelayDeleteFailed = TRUE;
    }

    if (DelayDeleteFailed) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Some files are in use and cannot be deleted.  The following should be deleted manually:\n"));
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y\n%y\n"), &ExecutableFile, &TargetDirectory);
        Sleep(5000);
    }

    YoriLibFreeStringContents(&TargetDirectory);
    YoriLibFreeStringContents(&ExecutableFile);
    YoriLibFreeStringContents(&PkgIniFile);

    if (DelayDeleteFailed) {
        return FALSE;
    }

    return TRUE;
}


// vim:sw=4:ts=4:et:
