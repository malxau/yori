/**
 * @file pkglib/install.c
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
#include "yoripkg.h"

/**
 Delete a file that was installed by a package.  If it works, try to delete
 the parent directory.  The file system will fail this if the directory
 still has files in it.  If it succeeds, keep moving through the parents to
 see what can be removed.

 @param FilePath Pointer to the file to remove.

 @return TRUE if one or more objects were successfully scheduled for deletion.
 */
BOOL
YoriPkgDeleteInstalledPackageFile(
    __in PYORI_STRING FilePath
    )
{
    DWORD Index;
    DWORD RetryCount = 0;
    BOOL FileDeleted = FALSE;

    if (FilePath->LengthInChars == 0) {
        return FALSE;
    }

    //
    //  Retry a few times in case the file is transiently in use.
    //

    for (RetryCount = 0; RetryCount < 3; RetryCount++) {
        if (DeleteFile(FilePath->StartOfString)) {
            FileDeleted = TRUE;
            break;
        }
        Sleep(50);
    }

    if (!FileDeleted) {
        return FALSE;
    }

    for (Index = FilePath->LengthInChars - 1; Index > 0; Index--) {

        //
        //  If we find a seperator, turn it into a NULL and try to remove the
        //  directory.  If it fails (eg. the directory has files in it), stop.
        //  If it succeeds, put back the seperator and look for the next
        //  parent.
        //

        if (YoriLibIsSep(FilePath->StartOfString[Index])) {
            FilePath->StartOfString[Index] = '\0';
            if (!RemoveDirectory(FilePath->StartOfString)) {
                FilePath->StartOfString[Index] = '\\';
                return TRUE;
            }
            FilePath->StartOfString[Index] = '\\';
        }
    }

    return TRUE;
}

/**
 Delete a specified package from the system.

 @param TargetDirectory Pointer to a string specifying the directory
        containing the package.  If NULL, the directory containing the
        application is used.

 @param PackageName Specifies the package name to delete.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgDeletePackage(
    __in_opt PYORI_STRING TargetDirectory,
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

    if (!YoriPkgGetPackageIniFile(TargetDirectory, &PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(_T("Installed"), PackageName->StartOfString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);
    if (IniValue.LengthInChars == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not an installed package\n"), PackageName);
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

    if (!YoriPkgGetApplicationDirectory(&AppPath)) {
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    YoriLibInitEmptyString(&FileToDelete);
    if (!YoriLibAllocateString(&FileToDelete, AppPath.LengthInChars + YORIPKG_MAX_FIELD_LENGTH)) {
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
            YoriPkgDeleteInstalledPackageFile(&FileToDelete);
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

    WritePrivateProfileString(PackageName->StartOfString, NULL, NULL, PkgIniFile.StartOfString);


    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&IniValue);
    YoriLibFreeStringContents(&AppPath);
    YoriLibFreeStringContents(&FileToDelete);

    return TRUE;
}


/**
 A context structure passed for each file installed as part of a package.
 */
typedef struct _YORIPKG_INSTALL_PKG_CONTEXT {

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
} YORIPKG_INSTALL_PKG_CONTEXT, *PYORIPKG_INSTALL_PKG_CONTEXT;

/**
 A callback function invoked for each file installed as part of a package.

 @param FullPath The full path name of the file on disk.

 @param RelativePath The relative path name of the file as stored within the
        package.

 @param Context Pointer to the YORIPKG_INSTALL_PKG_CONTEXT structure.

 @return TRUE to continue to apply the file, FALSE to skip the file.
 */
BOOL
YoriPkgInstallPackageFileCallback(
    __in PYORI_STRING FullPath,
    __in PYORI_STRING RelativePath,
    __in PVOID Context
    )
{
    PYORIPKG_INSTALL_PKG_CONTEXT InstallContext = (PYORIPKG_INSTALL_PKG_CONTEXT)Context;
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
        application is used.

 @param UpgradeOnly If TRUE, a package is only installed if the supplied
        package is a different version to the one currently installed

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgInstallPackage(
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
    YORIPKG_INSTALL_PKG_CONTEXT InstallContext;

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

    //
    //  Create path to system packages.ini
    //

    if (!YoriPkgGetPackageIniFile(TargetDirectory, &PkgIniFile)) {
        goto Exit;
    }

    if (!YoriPkgPackagePathToLocalPath(PackagePath, &PkgIniFile, &PackageFile, &DeleteLocalFile)) {
        goto Exit;
    }

    if (TargetDirectory != NULL) {
        if (!YoriLibUserStringToSingleFilePath(TargetDirectory, FALSE, &FullTargetDirectory)) {
            goto Exit;
        }
    } else {
        if (!YoriPkgGetApplicationDirectory(&FullTargetDirectory)) {
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibExtractCab failed on %y: %y\n"), &PackageFile, &ErrorString);
        YoriLibFreeStringContents(&ErrorString);
        goto Exit;
    }

    //
    //  Query fields of interest from pkginfo.ini
    //

    YoriLibSPrintf(&TempPath.StartOfString[TempPath.LengthInChars], _T("%y"), &PkgInfoFile);
    TempPath.LengthInChars += PkgInfoFile.LengthInChars;

    if (!YoriPkgGetPackageInfo(&TempPath, &PackageName, &PackageVersion, &PackageArch, &UpgradePath, &SourcePath, &SymbolPath)) {
        goto Exit;
    }

    if (UpgradeOnly) {
        YORI_STRING CurrentlyInstalledVersion;
        if (!YoriLibAllocateString(&CurrentlyInstalledVersion, YORIPKG_MAX_FIELD_LENGTH)) {
            goto Exit;
        }

        CurrentlyInstalledVersion.LengthInChars = GetPrivateProfileString(_T("Installed"), PackageName.StartOfString, _T(""), CurrentlyInstalledVersion.StartOfString, CurrentlyInstalledVersion.LengthAllocated, PkgIniFile.StartOfString);

        if (YoriLibCompareString(&CurrentlyInstalledVersion, &PackageVersion) == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y version %y is already installed\n"), &PackageName, &PackageVersion);
            YoriLibFreeStringContents(&CurrentlyInstalledVersion);
            Result = TRUE;
            goto Exit;
        } else if (CurrentlyInstalledVersion.LengthInChars > 0) {
            YoriPkgDeletePackage(TargetDirectory, &PackageName);

        }
        YoriLibFreeStringContents(&CurrentlyInstalledVersion);
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Installing package %y version %y\n"), &PackageName, &PackageVersion);


    //
    //  Before starting, indicate that the package is installed with a
    //  version of zero.  This ensures that if anything goes wrong, an
    //  upgrade will detect a new version and will retry.
    //

    WritePrivateProfileString(_T("Installed"), PackageName.StartOfString, _T("0"), PkgIniFile.StartOfString);
    if (UpgradePath.LengthInChars > 0) {
        WritePrivateProfileString(PackageName.StartOfString, _T("UpgradePath"), UpgradePath.StartOfString, PkgIniFile.StartOfString);
    }

    //
    //  Extract the package contents, without pkginfo.ini, to the desired
    //  location
    //

    InstallContext.IniFileName = &PkgIniFile;
    InstallContext.PackageName = &PackageName;
    InstallContext.NumberFiles = 0;
    if (!YoriLibExtractCab(&PackageFile, &FullTargetDirectory, TRUE, 1, &PkgInfoFile, 0, NULL, YoriPkgInstallPackageFileCallback, &InstallContext, &ErrorString)) {
        WritePrivateProfileString(_T("Installed"), PackageName.StartOfString, NULL, PkgIniFile.StartOfString);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibExtractCab failed on %y: %y\n"), &PackageFile, &ErrorString);
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
    WritePrivateProfileString(_T("Installed"), PackageName.StartOfString, PackageVersion.StartOfString, PkgIniFile.StartOfString);

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
YoriPkgBuildUpgradeLocationForNewArchitecture(
    __in PYORI_STRING PackageName,
    __in PYORI_STRING NewArchitecture,
    __in PYORI_STRING PkgIniFile,
    __inout PYORI_STRING UpgradePath
    )
{
    YORI_STRING IniValue;
    YORI_STRING ExistingArchAndExtension;

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
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
YoriPkgUpgradeInstalledPackages(
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

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&InstalledSection, 64 * 1024)) {
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    if (!YoriLibAllocateString(&UpgradePath, YORIPKG_MAX_FIELD_LENGTH)) {
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
                YoriPkgBuildUpgradeLocationForNewArchitecture(&PkgNameOnly, NewArchitecture, &PkgIniFile, &UpgradePath);
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Upgrading %y (%i/%i), downloading %y...\n"), &PkgNameOnly, CurrentIndex, TotalCount, &UpgradePath);
            if (!YoriPkgInstallPackage(&UpgradePath, NULL, TRUE)) {
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
YoriPkgUpgradeSinglePackage(
    __in PYORI_STRING PackageName,
    __in_opt PYORI_STRING NewArchitecture
    )
{
    YORI_STRING PkgIniFile;
    YORI_STRING IniValue;
    BOOL Result;

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
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
        YoriPkgBuildUpgradeLocationForNewArchitecture(PackageName, NewArchitecture, &PkgIniFile, &IniValue);
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Installing %y...\n"), &IniValue);
    Result = YoriPkgInstallPackage(&IniValue, NULL, TRUE);

    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&IniValue);

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

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&InstalledSection, 64 * 1024)) {
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    if (!YoriLibAllocateString(&SourcePath, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&InstalledSection);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    InstalledSection.LengthInChars = GetPrivateProfileSection(_T("Installed"), InstalledSection.StartOfString, InstalledSection.LengthAllocated, PkgIniFile.StartOfString);

    YoriLibInitEmptyString(&PkgNameOnly);
    ThisLine = InstalledSection.StartOfString;

    while (*ThisLine != '\0') {
        LineLength = _tcslen(ThisLine);
        ThisLine += LineLength;
        ThisLine++;
    }

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
        SourcePath.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, _T("SourcePath"), _T(""), SourcePath.StartOfString, SourcePath.LengthAllocated, PkgIniFile.StartOfString);
        if (SourcePath.LengthInChars > 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Installing source for %y, downloading %y...\n"), &PkgNameOnly, &SourcePath);
            if (!YoriPkgInstallPackage(&SourcePath, NULL, TRUE)) {
                break;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }
        ThisLine += LineLength;
        ThisLine++;
    }

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

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
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

    IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, _T("SourcePath"), _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);

    if (IniValue.LengthInChars == 0) {
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y does not specify a source path\n"), PackageName);
        return FALSE;
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Installing %y...\n"), &IniValue);
    Result = YoriPkgInstallPackage(&IniValue, NULL, TRUE);

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

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&InstalledSection, 64 * 1024)) {
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    if (!YoriLibAllocateString(&SymbolPath, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&InstalledSection);
        YoriLibFreeStringContents(&PkgIniFile);
        return FALSE;
    }

    InstalledSection.LengthInChars = GetPrivateProfileSection(_T("Installed"), InstalledSection.StartOfString, InstalledSection.LengthAllocated, PkgIniFile.StartOfString);

    YoriLibInitEmptyString(&PkgNameOnly);
    ThisLine = InstalledSection.StartOfString;

    while (*ThisLine != '\0') {
        LineLength = _tcslen(ThisLine);
        ThisLine += LineLength;
        ThisLine++;
    }

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
        SymbolPath.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, _T("SymbolPath"), _T(""), SymbolPath.StartOfString, SymbolPath.LengthAllocated, PkgIniFile.StartOfString);
        if (SymbolPath.LengthInChars > 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Installing symbols for %y, downloading %y...\n"), &PkgNameOnly, &SymbolPath);
            if (!YoriPkgInstallPackage(&SymbolPath, NULL, TRUE)) {
                break;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }
        ThisLine += LineLength;
        ThisLine++;
    }

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
    BOOL Result;

    if (!YoriPkgGetPackageIniFile(NULL, &PkgIniFile)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
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

    IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, _T("SymbolPath"), _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile.StartOfString);

    if (IniValue.LengthInChars == 0) {
        YoriLibFreeStringContents(&PkgIniFile);
        YoriLibFreeStringContents(&IniValue);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y does not specify a source path\n"), PackageName);
        return FALSE;
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Installing %y...\n"), &IniValue);
    Result = YoriPkgInstallPackage(&IniValue, NULL, TRUE);

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

    if (!YoriLibAllocateString(&InstalledSection, 64 * 1024)) {
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
