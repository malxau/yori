/**
 * @file pkglib/yoripkgp.h
 *
 * Private/internal header for Yori package routines
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

/**
 Description of a file which was part of a package that is being overwritten.
 The file has been renamed from one name to a backup name.
 */
typedef struct _YORIPKG_BACKUP_FILE {

    /**
     A list entry of all files that have been renamed out of the way.  The
     list head is YORIPKG_BACKUP_PACKAGE's PackageList member.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     A fully specified path to the original file name.  This must be freed
     when the structure is deallocated.
     */
    YORI_STRING OriginalName;

    /**
     The temporary, backup name that the file has been renamed to.  Note this
     may be an empty string if the source file was not found on disk.
     */
    YORI_STRING BackupName;

    /**
     Pointer within the OriginalName allocation to a substring containing the
     file name relative to the installation root.  This is the string that is
     stored in the master .ini file for the file.
     */
    YORI_STRING OriginalRelativeName;
} YORIPKG_BACKUP_FILE, *PYORIPKG_BACKUP_FILE;

/**
 Description of a package which is being overwritten, so its files have been
 renamed into backup locations and its settings in the INI file have been
 captured into RAM so they can be reapplied back to the INI file as needed.
 */
typedef struct _YORIPKG_BACKUP_PACKAGE {

    /**
     A list of packages which have been backed up and may need to be restored.
     */
    YORI_LIST_ENTRY PackageList;

    /**
     A list of files which have been renamed to backup locations so the
     previous file names can be overwritten by a later package.
     */
    YORI_LIST_ENTRY FileList;

    /**
     The canonical name of this package.
     */
    YORI_STRING PackageName;

    /**
     The number of files in the package, as described by the INI file.  Note
     that aborting requires that all files in FileList will be reapplied, in
     order, and FileCount file names will be restored back into the INI file.
     */
    DWORD FileCount;

    /**
     The version of the package that was backed up.
     */
    YORI_STRING Version;

    /**
     The architecture of the package that was backed up.
     */
    YORI_STRING Architecture;

    /**
     The upgrade path for the package that was backed up.  This can be an
     empty string indicating no upgrade path, in which case this entry will
     not be restored into the INI file.
     */
    YORI_STRING UpgradePath;

    /**
     The source path for the package that was backed up.  This can be an
     empty string indicating no source path, in which case this entry will
     not be restored into the INI file.
     */
    YORI_STRING SourcePath;

    /**
     The symbol path for the package that was backed up.  This can be an
     empty string indicating no symbol path, in which case this entry will
     not be restored into the INI file.
     */
    YORI_STRING SymbolPath;
} YORIPKG_BACKUP_PACKAGE, *PYORIPKG_BACKUP_PACKAGE;

/**
 A list of packages awaiting installation.  These have been downloaded and
 parsed, and any existing packages that conflict with the new packages have
 been packed up.
 */
typedef struct _YORIPKG_PACKAGES_PENDING_INSTALL {

    /**
     A list of packages to install.  Paired with
     @ref YORIPKG_PACKAGE_PENDING_INSTALL::PackageList .
     */
    YORI_LIST_ENTRY PackageList;

    /**
     A list of packages to install.  Paired with
     @ref YORIPKG_BACKUP_PACKAGE::PackageList .
     */
    YORI_LIST_ENTRY BackupPackages;

} YORIPKG_PACKAGES_PENDING_INSTALL, *PYORIPKG_PACKAGES_PENDING_INSTALL;

/**
 Information about a package that has been downloaded and is ready to install.
 */
typedef struct _YORIPKG_PACKAGE_PENDING_INSTALL {

    /**
     Entry on a list of packages awaiting installation.  Paired with
     @ref YORIPKG_PACKAGES_PENDING_INSTALL::PackageList .
     */
    YORI_LIST_ENTRY PackageList;

    /**
     A string for the human readable package name.
     */
    YORI_STRING PackageName;

    /**
     A string for the package version.
     */
    YORI_STRING Version;

    /**
     A string for the package architecture.
     */
    YORI_STRING Architecture;

    /**
     A string for the path to upgrade the package from.  This can be an empty
     string if the package does not support upgrade.
     */
    YORI_STRING UpgradePath;

    /**
     A string for the path to obtain source for the package from.  This can be
     an empty string if the package does not link to a source package.
     */
    YORI_STRING SourcePath;

    /**
     A string for the path to obtain symbols for the package from.  This can be
     an empty string if the package does not link to a symbol package.
     */
    YORI_STRING SymbolPath;

    /**
     A path to a local file containing the CAB file to install.
     */
    YORI_STRING LocalPackagePath;

    /**
     TRUE if the CAB file should be deleted when processing is complete.  This
     occurs if the package has been downloaded from a remote path and is
     stored in a temporary location.
     */
    BOOL DeleteLocalPackagePath;
} YORIPKG_PACKAGE_PENDING_INSTALL, *PYORIPKG_PACKAGE_PENDING_INSTALL;

/**
 The maximum length of a value in an INI file.  The APIs aren't very good
 about telling us how much space we need, so this is the size we allocate
 and the effective limit.
 */
#define YORIPKG_MAX_FIELD_LENGTH (256)

/**
 The maximum length of a section in an INI file.  The APIs aren't very good
 about telling us how much space we need, so this is the size we allocate
 and the effective limit.
 */
#define YORIPKG_MAX_SECTION_LENGTH (64 * 1024)

BOOL
YoriPkgGetApplicationDirectory(
    __out PYORI_STRING AppDirectory
    );

BOOL
YoriPkgGetPackageIniFile(
    __in_opt PYORI_STRING InstallDirectory,
    __out PYORI_STRING IniFileName
    );

BOOL
YoriPkgGetPackageInfo(
    __in PYORI_STRING IniPath,
    __out PYORI_STRING PackageName,
    __out PYORI_STRING PackageVersion,
    __out PYORI_STRING PackageArch,
    __out PYORI_STRING UpgradePath,
    __out PYORI_STRING SourcePath,
    __out PYORI_STRING SymbolPath
    );

BOOL
YoriPkgGetInstalledPackageInfo(
    __in PYORI_STRING IniPath,
    __in PYORI_STRING PackageName,
    __out PYORI_STRING PackageVersion,
    __out PYORI_STRING PackageArch,
    __out PYORI_STRING UpgradePath,
    __out PYORI_STRING SourcePath,
    __out PYORI_STRING SymbolPath
    );

BOOL
YoriPkgBuildUpgradeLocationForNewArchitecture(
    __in PYORI_STRING PackageName,
    __in PYORI_STRING NewArchitecture,
    __in PYORI_STRING PkgIniFile,
    __inout PYORI_STRING UpgradePath
    );

BOOL
YoriPkgInstallPackage(
    __in PYORIPKG_PACKAGE_PENDING_INSTALL Package,
    __in_opt PYORI_STRING TargetDirectory
    );

BOOL
YoriPkgPackagePathToLocalPath(
    __in PYORI_STRING PackagePath,
    __in PYORI_STRING IniFilePath,
    __out PYORI_STRING LocalPath,
    __out PBOOL DeleteWhenFinished
    );

VOID
YoriPkgFreeBackupPackage(
    __in PYORIPKG_BACKUP_PACKAGE PackageBackup
    );

VOID
YoriPkgRollbackPackage(
    __in PYORI_STRING IniPath,
    __in PYORIPKG_BACKUP_PACKAGE PackageBackup
    );

BOOL
YoriPkgBackupPackage(
    __in PYORI_STRING IniPath,
    __in PYORI_STRING PackageName,
    __in_opt PYORI_STRING TargetDirectory,
    __out PYORIPKG_BACKUP_PACKAGE * PackageBackup
    );

VOID
YoriPkgRemoveSystemReferencesToPackage(
    __in PYORI_STRING IniPath,
    __in PYORIPKG_BACKUP_PACKAGE PackageBackup
    );

VOID
YoriPkgRollbackAndFreeBackupPackageList(
    __in PYORI_STRING IniPath,
    __in_opt PYORI_STRING NewDirectory,
    __in PYORI_LIST_ENTRY ListHead
    );

VOID
YoriPkgCommitAndFreeBackupPackageList(
    __in PYORI_LIST_ENTRY ListHead
    );

VOID
YoriPkgInitializePendingPackages(
    __out PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages
    );

VOID
YoriPkgDeletePendingPackage(
    __in PYORIPKG_PACKAGE_PENDING_INSTALL PendingPackage
    );

VOID
YoriPkgDeletePendingPackages(
    __in PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages
    );

BOOL
YoriPkgPreparePackageForInstall(
    __in PYORI_STRING PkgIniFile,
    __in_opt PYORI_STRING TargetDirectory,
    __inout PYORIPKG_PACKAGES_PENDING_INSTALL PackageList,
    __in PYORI_STRING PackageUrl
    );

// vim:sw=4:ts=4:et:
