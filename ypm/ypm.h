/**
 * @file ypm/ypm.h
 *
 * Master header for Yori package manager
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
 The maximum length of a value in an INI file.  The APIs aren't very good
 about telling us how much space we need, so this is the size we allocate
 and the effective limit.
 */
#define YPM_MAX_FIELD_LENGTH (256)

BOOL
YpmGetApplicationDirectory(
    __out PYORI_STRING AppDirectory
    );

BOOL
YpmGetPackageIniFile(
    __out PYORI_STRING IniFileName
    );

BOOL
YpmGetPackageInfo(
    __in PYORI_STRING IniPath,
    __out PYORI_STRING PackageName,
    __out PYORI_STRING PackageVersion,
    __out PYORI_STRING PackageArch,
    __out PYORI_STRING UpgradePath,
    __out PYORI_STRING SourcePath,
    __out PYORI_STRING SymbolPath
    );

BOOL
YpmPackagePathToLocalPath(
    __in PYORI_STRING PackagePath,
    __out PYORI_STRING LocalPath,
    __out PBOOL DeleteWhenFinished
    );

BOOL
YpmCreateBinaryPackage(
    __in PYORI_STRING FileName,
    __in PYORI_STRING PackageName,
    __in PYORI_STRING Version,
    __in PYORI_STRING Architecture,
    __in PYORI_STRING FileListFile,
    __in_opt PYORI_STRING UpgradePath,
    __in_opt PYORI_STRING SourcePath,
    __in_opt PYORI_STRING SymbolPath
    );

BOOL
YpmCreateSourcePackage(
    __in PYORI_STRING FileName,
    __in PYORI_STRING PackageName,
    __in PYORI_STRING Version,
    __in PYORI_STRING FileRoot
    );

BOOL
YpmDeletePackage(
    __in PYORI_STRING PackageName
    );

BOOL
YpmInstallPackage(
    __in PYORI_STRING PackagePath,
    __in_opt PYORI_STRING TargetDirectory,
    __in BOOL UpgradeOnly
    );

BOOL
YpmUpgradeInstalledPackages(
    __in_opt PYORI_STRING NewArchitecture
    );

BOOL
YpmUpgradeSinglePackage(
    __in PYORI_STRING PackageName,
    __in_opt PYORI_STRING NewArchitecture
    );
