/**
 * @file ypm/install.c
 *
 * Yori shell install packages
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
#include "ypm.h"

/**
 Delete a specified package from the system.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YpmDeletePackage(
    __in PYORI_STRING PackageName
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING AppPath;
    YORI_STRING IniValue;
    YORI_STRING FileToDelete;
    DWORD FileCount;
    DWORD FileIndex;
    TCHAR FileIndexString[16];

    if (!YpmGetPackageIniFile(&PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YPM_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(_T("Installed"), PackageName->StartOfString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);
    if (IniValue.LengthInChars == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not an installed package\n"), &PackageName);
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    FileCount = GetPrivateProfileInt(PackageName->StartOfString, _T("FileCount"), 0, PkgIniFile.StartOfString);
    if (FileCount == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y contains nothing to remove\n"), &PackageName);
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    if (!YpmGetApplicationDirectory(&AppPath)) {
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    YoriLibInitEmptyString(&FileToDelete);
    if (!YoriLibAllocateString(&FileToDelete, AppPath.LengthInChars + YPM_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&AppPath);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    for (FileIndex = 1; FileIndex <= FileCount; FileIndex++) {
        YoriLibSPrintf(FileIndexString, _T("File%i"), FileIndex);

        IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, FileIndexString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);
        if (IniValue.LengthInChars > 0) {
            YoriLibYPrintf(&FileToDelete, _T("%y\\%y"), &AppPath, &IniValue);
            DeleteFile(FileToDelete.StartOfString);
        }

        WritePrivateProfileString(PackageName->StartOfString, FileIndexString, NULL, PkgIniFile.StartOfString);
    }

    WritePrivateProfileString(PackageName->StartOfString, _T("FileCount"), NULL, PkgIniFile.StartOfString);
    WritePrivateProfileString(PackageName->StartOfString, _T("Architecture"), NULL, PkgIniFile.StartOfString);
    WritePrivateProfileString(PackageName->StartOfString, _T("UpgradePath"), NULL, PkgIniFile.StartOfString);
    WritePrivateProfileString(PackageName->StartOfString, _T("SourcePath"), NULL, PkgIniFile.StartOfString);
    WritePrivateProfileString(PackageName->StartOfString, _T("SymbolPath"), NULL, PkgIniFile.StartOfString);
    WritePrivateProfileString(PackageName->StartOfString, _T("Version"), NULL, PkgIniFile.StartOfString);
    WritePrivateProfileString(_T("Installed"), PackageName->StartOfString, NULL, PkgIniFile.StartOfString);


    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&IniValue);
    YoriLibFreeStringContents(&AppPath);
    YoriLibFreeStringContents(&FileToDelete);

    return TRUE;
}


/**
 A context structure passed for each file installed as part of a package.
 */
typedef struct _YPM_INSTALL_PKG_CONTEXT {

    /**
     Path to the INI file recording package installation.
     */
    PYORI_STRING IniFileName;

    /**
     The name of the package being installed.
     */
    PYORI_STRING PackageName;

    /**
     The number of files installed as part of this package.  THis value is
     incremented each time a file is found.
     */
    DWORD NumberFiles;
} YPM_INSTALL_PKG_CONTEXT, *PYPM_INSTALL_PKG_CONTEXT;

/**
 A callback function invoked for each file installed as part of a package.

 @param FullPath The full path name of the file on disk.

 @param RelativePath The relative path name of the file as stored within the
        package.

 @param Context Pointer to the YPM_INSTALL_PKG_CONTEXT structure.

 @return TRUE to continue to apply the file, FALSE to skip the file.
 */
BOOL
YpmInstallPackageFileCallback(
    __in PYORI_STRING FullPath,
    __in PYORI_STRING RelativePath,
    __in PVOID Context
    )
{
    PYPM_INSTALL_PKG_CONTEXT InstallContext = (PYPM_INSTALL_PKG_CONTEXT)Context;
    TCHAR FileIndexString[16];

    UNREFERENCED_PARAMETER(FullPath);

    InstallContext->NumberFiles++;
    YoriLibSPrintf(FileIndexString, _T("File%i"), InstallContext->NumberFiles);

    WritePrivateProfileString(InstallContext->PackageName->StartOfString, FileIndexString, RelativePath->StartOfString, InstallContext->IniFileName->StartOfString);
    return TRUE;
}


/**
 Install a package into the system.

 @param PackagePath Pointer to a string specifying a local or remote path to
        a package to install.

 @param TargetDirectory Pointer to a string specifying the directory to
        install the package.  If NULL, the directory containing the
        ypm application is used.

 @param UpgradeOnly If TRUE, a package is only installed if the supplied
        package is a different version to the one currently installed

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YpmInstallPackage(
    __in PYORI_STRING PackagePath,
    __in_opt PYORI_STRING TargetDirectory,
    __in BOOL UpgradeOnly
    )
{
    YORI_STRING PkgInfoFile;
    YORI_STRING PkgIniFile;
    YORI_STRING TempPath;
    YORI_STRING FullTargetDirectory;
    YORI_STRING PackageFile;

    YORI_STRING PackageName;
    YORI_STRING PackageVersion;
    YORI_STRING PackageArch;
    YORI_STRING UpgradePath;
    YORI_STRING SourcePath;
    YORI_STRING SymbolPath;
    YORI_STRING ErrorString;
    YPM_INSTALL_PKG_CONTEXT InstallContext;

    BOOL Result = FALSE;
    BOOL DeleteLocalFile = FALSE;
    TCHAR FileIndexString[16];

    YoriLibConstantString(&PkgInfoFile, _T("pkginfo.ini"));

    YoriLibInitEmptyString(&FullTargetDirectory);
    YoriLibInitEmptyString(&TempPath);
    YoriLibInitEmptyString(&PkgIniFile);
    YoriLibInitEmptyString(&PackageName);
    YoriLibInitEmptyString(&PackageVersion);
    YoriLibInitEmptyString(&PackageArch);
    YoriLibInitEmptyString(&UpgradePath);
    YoriLibInitEmptyString(&SourcePath);
    YoriLibInitEmptyString(&SymbolPath);
    YoriLibInitEmptyString(&PackageFile);

    if (!YpmPackagePathToLocalPath(PackagePath, &PackageFile, &DeleteLocalFile)) {
        goto Exit;
    }

    if (TargetDirectory != NULL) {
        if (!YoriLibUserStringToSingleFilePath(TargetDirectory, FALSE, &FullTargetDirectory)) {
            goto Exit;
        }
    } else {
        if (!YpmGetApplicationDirectory(&FullTargetDirectory)) {
            goto Exit;
        }
    }

    //
    //  Query for a temporary directory
    //

    TempPath.LengthAllocated = GetTempPath(0, NULL);
    if (!YoriLibAllocateString(&TempPath, TempPath.LengthAllocated + PkgInfoFile.LengthInChars + 1)) {
        goto Exit;
    }
    TempPath.LengthInChars = GetTempPath(TempPath.LengthAllocated, TempPath.StartOfString);

    //
    //  Extract pkginfo.ini to the temporary directory
    //

    YoriLibInitEmptyString(&ErrorString);
    if (!YoriLibExtractCab(&PackageFile, &TempPath, FALSE, 0, NULL, 1, &PkgInfoFile, NULL, NULL, &ErrorString)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: YoriLibExtractCab failed on %y: %y\n"), &PackageFile, &ErrorString);
        YoriLibFreeStringContents(&ErrorString);
        goto Exit;
    }

    //
    //  Create path to system packages.ini
    //

    if (!YpmGetPackageIniFile(&PkgIniFile)) {
        goto Exit;
    }

    //
    //  Query fields of interest from pkginfo.ini
    //

    YoriLibSPrintf(&TempPath.StartOfString[TempPath.LengthInChars], _T("%y"), &PkgInfoFile);
    TempPath.LengthInChars += PkgInfoFile.LengthInChars;

    if (!YpmGetPackageInfo(&TempPath, &PackageName, &PackageVersion, &PackageArch, &UpgradePath, &SourcePath, &SymbolPath)) {
        goto Exit;
    }

    if (UpgradeOnly) {
        YORI_STRING CurrentlyInstalledVersion;
        if (!YoriLibAllocateString(&CurrentlyInstalledVersion, YPM_MAX_FIELD_LENGTH)) {
            goto Exit;
        }

        CurrentlyInstalledVersion.LengthInChars = GetPrivateProfileString(_T("Installed"), PackageName.StartOfString, _T(""), CurrentlyInstalledVersion.StartOfString, CurrentlyInstalledVersion.LengthAllocated, PkgIniFile.StartOfString);

        if (YoriLibCompareString(&CurrentlyInstalledVersion, &PackageVersion) == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y version %y is already installed\n"), &PackageName, &PackageVersion);
            YoriLibFreeStringContents(&CurrentlyInstalledVersion);
            Result = TRUE;
            goto Exit;
        } else {
            YpmDeletePackage(&PackageName);

        }
        YoriLibFreeStringContents(&CurrentlyInstalledVersion);
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Installing package %y version %y\n"), &PackageName, &PackageVersion);


    //
    //  Extract the package contents, without pkginfo.ini, to the desired
    //  location
    //

    WritePrivateProfileString(_T("Installed"), PackageName.StartOfString, PackageVersion.StartOfString, PkgIniFile.StartOfString);

    InstallContext.IniFileName = &PkgIniFile;
    InstallContext.PackageName = &PackageName;
    InstallContext.NumberFiles = 0;
    if (!YoriLibExtractCab(&PackageFile, &FullTargetDirectory, TRUE, 1, &PkgInfoFile, 0, NULL, YpmInstallPackageFileCallback, &InstallContext, &ErrorString)) {
        WritePrivateProfileString(_T("Installed"), PackageName.StartOfString, NULL, PkgIniFile.StartOfString);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ypm: YoriLibExtractCab failed on %y: %y\n"), &PackageFile, &ErrorString);
        YoriLibFreeStringContents(&ErrorString);
        goto Exit;
    }

    WritePrivateProfileString(PackageName.StartOfString, _T("Version"), PackageVersion.StartOfString, PkgIniFile.StartOfString);
    WritePrivateProfileString(PackageName.StartOfString, _T("Architecture"), PackageArch.StartOfString, PkgIniFile.StartOfString);
    if (UpgradePath.LengthInChars > 0) {
        WritePrivateProfileString(PackageName.StartOfString, _T("UpgradePath"), UpgradePath.StartOfString, PkgIniFile.StartOfString);
    }
    if (SourcePath.LengthInChars > 0) {
        WritePrivateProfileString(PackageName.StartOfString, _T("SourcePath"), SourcePath.StartOfString, PkgIniFile.StartOfString);
    }
    if (SymbolPath.LengthInChars > 0) {
        WritePrivateProfileString(PackageName.StartOfString, _T("SymbolPath"), SymbolPath.StartOfString, PkgIniFile.StartOfString);
    }

    YoriLibSPrintf(FileIndexString, _T("%i"), InstallContext.NumberFiles);

    WritePrivateProfileString(PackageName.StartOfString, _T("FileCount"), FileIndexString, PkgIniFile.StartOfString);

    Result = TRUE;

Exit:
    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&TempPath);
    YoriLibFreeStringContents(&FullTargetDirectory);
    YoriLibFreeStringContents(&PackageName);
    YoriLibFreeStringContents(&PackageVersion);
    YoriLibFreeStringContents(&PackageArch);
    YoriLibFreeStringContents(&UpgradePath);
    YoriLibFreeStringContents(&SourcePath);
    YoriLibFreeStringContents(&SymbolPath);
    YoriLibFreeStringContents(&FullTargetDirectory);
    if (DeleteLocalFile) {
        DeleteFile(PackageFile.StartOfString);
    }
    YoriLibFreeStringContents(&PackageFile);

    return Result;
}

/**
 Given a package name of an installed package and an existing upgrade path for
 the current architecture, try to munge a path for a new architecture.  This
 routine intentionally leaves "noarch" packages alone, because there's never
 a need to get a different type of noarch package.

 @param PackageName The name of an installed package.

 @param NewArchitecture The new architecture to apply.

 @param PkgIniFile Path to the system global store of installed packages.

 @param UpgradePath On input, refers to a fully qualified upgrade path for
        the package.  On successful completion, this is updated to contain
        a path for the new architecture.  Note this modification is performed
        on the existing allocation; the assumption is the caller needs to
        have a pessimistically sized buffer to read from the INI file, and
        any modifications made need to fit in the same limits anyway.

 @return TRUE to indicate the path was successfully updated to the new
         architecture; FALSE if it was not updated.
 */
BOOL
YpmBuildUpgradeLocationForNewArchitecture(
    __in PYORI_STRING PackageName,
    __in PYORI_STRING NewArchitecture,
    __in PYORI_STRING PkgIniFile,
    __inout PYORI_STRING UpgradePath
    )
{
    YORI_STRING IniValue;
    YORI_STRING ExistingArchAndExtension;

    if (!YoriLibAllocateString(&IniValue, YPM_MAX_FIELD_LENGTH)) {
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, _T("Architecture"), _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile->StartOfString);
    if (IniValue.LengthInChars == 0) {
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    if (UpgradePath->LengthInChars < IniValue.LengthInChars + sizeof(".cab") - 1) {
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    YoriLibInitEmptyString(&ExistingArchAndExtension);
    ExistingArchAndExtension.LengthInChars = sizeof(".cab") - 1;
    ExistingArchAndExtension.StartOfString = &UpgradePath->StartOfString[UpgradePath->LengthInChars - ExistingArchAndExtension.LengthInChars];

    if (YoriLibCompareStringWithLiteralInsensitive(&ExistingArchAndExtension, _T(".cab")) != 0) {
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    ExistingArchAndExtension.StartOfString -= IniValue.LengthInChars;
    ExistingArchAndExtension.LengthInChars = IniValue.LengthInChars;

    if (YoriLibCompareStringInsensitive(&ExistingArchAndExtension, &IniValue) != 0) {
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteralInsensitive(&ExistingArchAndExtension, _T("noarch")) == 0) {
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    if (UpgradePath->LengthInChars - IniValue.LengthInChars + NewArchitecture->LengthInChars < UpgradePath->LengthAllocated) {
        YoriLibSPrintf(ExistingArchAndExtension.StartOfString, _T("%y.cab"), NewArchitecture);
        UpgradePath->LengthInChars = UpgradePath->LengthInChars - IniValue.LengthInChars + NewArchitecture->LengthInChars;

        YoriLibFreeStringContents(&IniValue);
        return TRUE;
    }

    YoriLibFreeStringContents(&IniValue);
    return FALSE;
}

/**
 Upgrade all installed packages in the system.

 @param NewArchitecture Optionally points to the new architecture to apply.
        If not specified, the current architecture is retained.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YpmUpgradeInstalledPackages(
    __in_opt PYORI_STRING NewArchitecture
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING InstalledSection;
    LPTSTR ThisLine;
    LPTSTR Equals;
    YORI_STRING PkgNameOnly;
    YORI_STRING UpgradePath;
    DWORD LineLength;
    DWORD TotalCount;
    DWORD CurrentIndex;

    if (!YpmGetPackageIniFile(&PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&InstalledSection, 64 * 1024)) {
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    if (!YoriLibAllocateString(&UpgradePath, YPM_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&InstalledSection);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    InstalledSection.LengthInChars = GetPrivateProfileSection(_T("Installed"), InstalledSection.StartOfString, InstalledSection.LengthAllocated, PkgIniFile.StartOfString);

    YoriLibInitEmptyString(&PkgNameOnly);
    ThisLine = InstalledSection.StartOfString;

    TotalCount = 0;
    while (*ThisLine != '\0') {
        LineLength = _tcslen(ThisLine);
        TotalCount++;
        ThisLine += LineLength;
        ThisLine++;
    }

    CurrentIndex = 0;
    ThisLine = InstalledSection.StartOfString;

    while (*ThisLine != '\0') {
        LineLength = _tcslen(ThisLine);
        PkgNameOnly.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        if (Equals != NULL) {
            PkgNameOnly.LengthInChars = (DWORD)(Equals - ThisLine);
        } else {
            PkgNameOnly.LengthInChars = LineLength;
        }
        PkgNameOnly.StartOfString[PkgNameOnly.LengthInChars] = '\0';
        CurrentIndex++;
        UpgradePath.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, _T("UpgradePath"), _T(""), UpgradePath.StartOfString, UpgradePath.LengthAllocated, PkgIniFile.StartOfString);
        if (UpgradePath.LengthInChars > 0) {
            if (NewArchitecture != NULL) {
                YpmBuildUpgradeLocationForNewArchitecture(&PkgNameOnly, NewArchitecture, &PkgIniFile, &UpgradePath);
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Upgrading %y (%i/%i), downloading %y...\n"), &PkgNameOnly, CurrentIndex, TotalCount, &UpgradePath);
            if (!YpmInstallPackage(&UpgradePath, NULL, TRUE)) {
                break;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }
        ThisLine += LineLength;
        ThisLine++;
    }

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
YpmUpgradeSinglePackage(
    __in PYORI_STRING PackageName,
    __in_opt PYORI_STRING NewArchitecture
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING IniValue;
    BOOL Result;

    if (!YpmGetPackageIniFile(&PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YPM_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(_T("Installed"), PackageName->StartOfString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);
    if (IniValue.LengthInChars == 0) {
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y is not installed\n"), PackageName);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, _T("UpgradePath"), _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);

    if (IniValue.LengthInChars == 0) {
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y does not specify an upgrade path\n"), PackageName);
        return FALSE;
    }

    if (NewArchitecture != NULL) {
        YpmBuildUpgradeLocationForNewArchitecture(PackageName, NewArchitecture, &PkgIniFile, &IniValue);
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Installing %y...\n"), &IniValue);
    Result = YpmInstallPackage(&IniValue, NULL, TRUE);

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&IniValue);

    return Result;
}

// vim:sw=4:ts=4:et:
