/**
 * @file pkglib/yoripkg.h
 *
 * Master header for Yori package routines
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

/**
 When upgrading a package, indicate whether to prefer a stable package, a
 daily package, or keep the existing type of package.
 */
typedef enum _YORIPKG_UPGRADE_PREFER {
    YoriPkgUpgradePreferSame = 0,
    YoriPkgUpgradePreferStable = 1,
    YoriPkgUpgradePreferDaily = 2
} YORIPKG_UPGRADE_PREFER;

BOOL
YoriPkgCreateBinaryPackage(
    __in PYORI_STRING FileName,
    __in PYORI_STRING PackageName,
    __in PYORI_STRING Version,
    __in PYORI_STRING Architecture,
    __in PYORI_STRING FileListFile,
    __in_opt PYORI_STRING MinimumOSBuild,
    __in_opt PYORI_STRING PackagePathForOlderBuilds,
    __in_opt PYORI_STRING UpgradePath,
    __in_opt PYORI_STRING SourcePath,
    __in_opt PYORI_STRING SymbolPath,
    __in_opt PYORI_STRING UpgradeToStablePath,
    __in_opt PYORI_STRING UpgradeToDailyPath,
    __in_ecount_opt(ReplaceCount) PYORI_STRING Replaces,
    __in DWORD ReplaceCount
    );

BOOL
YoriPkgCreateSourcePackage(
    __in PYORI_STRING FileName,
    __in PYORI_STRING PackageName,
    __in PYORI_STRING Version,
    __in PYORI_STRING FileRoot
    );

BOOL
YoriPkgDeletePackage(
    __in_opt PCYORI_STRING TargetDirectory,
    __in PCYORI_STRING PackageName,
    __in BOOL WarnIfNotInstalled
    );

BOOL
YoriPkgInstallSinglePackage(
    __in PYORI_STRING PackagePath,
    __in_opt PCYORI_STRING TargetDirectory
    );

BOOL
YoriPkgUpgradeInstalledPackages(
    __in YORIPKG_UPGRADE_PREFER Prefer,
    __in_opt PYORI_STRING NewArchitecture
    );

BOOL
YoriPkgUpgradeSinglePackage(
    __in PYORI_STRING PackageName,
    __in YORIPKG_UPGRADE_PREFER Prefer,
    __in_opt PYORI_STRING NewArchitecture
    );

BOOL
YoriPkgInstallSourceForInstalledPackages(VOID);

BOOL
YoriPkgInstallSourceForSinglePackage(
    __in PYORI_STRING PackageName
    );

BOOL
YoriPkgInstallSymbolsForInstalledPackages(VOID);

BOOL
YoriPkgInstallSymbolForSinglePackage(
    __in PYORI_STRING PackageName
    );

BOOL
YoriPkgInstallPseudoPackage(
    __in PYORI_STRING Name,
    __in PYORI_STRING Version,
    __in PYORI_STRING Architecture,
    __in_ecount(FileCount) PYORI_STRING FileArray,
    __in DWORD FileCount,
    __in_opt PCYORI_STRING TargetDirectory
    );

BOOL
YoriPkgDisplayAvailableRemotePackages(VOID);

BOOL
YoriPkgDisplayAvailableRemotePackageNames(VOID);

BOOL
YoriPkgDisplaySources(VOID);

BOOL
YoriPkgAddNewSource(
    __in PYORI_STRING SourcePath,
    __in BOOLEAN InstallAsFirst
    );

BOOL
YoriPkgDeleteSource(
    __in PYORI_STRING SourcePath
    );

BOOL
YoriPkgDisplayMirrors(VOID);

BOOL
YoriPkgAddNewMirror(
    __in PYORI_STRING SourceName,
    __in PYORI_STRING TargetName,
    __in BOOLEAN InstallAsFirst
    );

BOOL
YoriPkgDeleteMirror(
    __in PYORI_STRING SourceName
    );

BOOL
YoriPkgInstallRemotePackages(
    __in PYORI_STRING PackageNames,
    __in DWORD PackageNameCount,
    __in_opt PYORI_STRING CustomSource,
    __in_opt PYORI_STRING NewDirectory,
    __in_opt PYORI_STRING MatchVersion,
    __in_opt PYORI_STRING MatchArch
    );

DWORD
YoriPkgGetRemotePackageUrls(
    __in PCYORI_STRING PackageNames,
    __in DWORD PackageNameCount,
    __in_opt PCYORI_STRING CustomSource,
    __in_opt PCYORI_STRING NewDirectory,
    __deref_out_opt PYORI_STRING * PackageUrls
    );

BOOL
YoriPkgListInstalledPackages(
    __in BOOL Verbose
    );

BOOL
YoriPkgDownloadRemotePackages(
    __in PYORI_STRING Source,
    __in PYORI_STRING DownloadPath
    );

BOOL
YoriPkgAppendInstallDirToPath(
    __in_opt PCYORI_STRING TargetDirectory,
    __in BOOL AppendToUserPath,
    __in BOOL AppendToSystemPath
    );

BOOL
YoriPkgAddUninstallEntry(
    __in PCYORI_STRING TargetDirectory,
    __in PCYORI_STRING InitialVersion
    );

BOOL
YoriPkgUninstallAll(VOID);

__success(return)
BOOL
YoriPkgGetTerminalProfilePath(
    __out PYORI_STRING TerminalProfilePath
    );

__success(return)
BOOL
YoriPkgWriteTerminalProfile(
    __in_opt PYORI_STRING YoriExeFullPath
    );

__success(return)
BOOL
YoriPkgCreateDesktopShortcut(
    __in_opt PYORI_STRING YoriExeFullPath
    );

__success(return)
BOOL
YoriPkgCreateStartMenuShortcut(
    __in_opt PYORI_STRING YoriExeFullPath
    );

// vim:sw=4:ts=4:et:
