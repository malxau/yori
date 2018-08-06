/**
 * @file ypm/util.c
 *
 * Yori package manager helper functions
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
 Return a fully qualified path to the directory containing the program.

 @param AppDirectory On successful completion, populated with the directory
        containing the application.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YpmGetApplicationDirectory(
    __out PYORI_STRING AppDirectory
    )
{
    YORI_STRING ModuleName;
    LPTSTR Ptr;

    if (!YoriLibAllocateString(&ModuleName, 32768)) {
        return FALSE;
    }

    ModuleName.LengthInChars = GetModuleFileName(NULL, ModuleName.StartOfString, ModuleName.LengthAllocated);
    if (ModuleName.LengthInChars == 0) {
        YoriLibFreeStringContents(&ModuleName);
        return FALSE;
    }

    Ptr = YoriLibFindRightMostCharacter(&ModuleName, '\\');
    if (Ptr == NULL) {
        YoriLibFreeStringContents(&ModuleName);
        return FALSE;
    }

    *Ptr = '\0';

    ModuleName.LengthInChars = (DWORD)(Ptr - ModuleName.StartOfString);
    memcpy(AppDirectory, &ModuleName, sizeof(YORI_STRING));
    return TRUE;
}

/**
 Return a fully qualified path to the global package INI file.

 @param IniFileName On successful completion, populated with a path to the
        package INI file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YpmGetPackageIniFile(
    __out PYORI_STRING IniFileName
    )
{
    YORI_STRING AppDirectory;

    if (!YpmGetApplicationDirectory(&AppDirectory)) {
        return FALSE;
    }

    if (AppDirectory.LengthAllocated - AppDirectory.LengthInChars < sizeof("\\packages.ini")) {
        YoriLibFreeStringContents(&AppDirectory);
        return FALSE;
    }

    YoriLibSPrintf(&AppDirectory.StartOfString[AppDirectory.LengthInChars], _T("\\packages.ini"));
    AppDirectory.LengthInChars += sizeof("\\packages.ini") - 1;

    memcpy(IniFileName, &AppDirectory, sizeof(YORI_STRING));
    return TRUE;
}

/**
 Given a fully qualified path to a package's INI file, extract package
 information.

 @param IniPath Pointer to a path to the package's INI file.  Note this is the
        file embedded in each package, not the global INI file.

 @param PackageName On successful completion, populated with the package's
        canonical name.

 @param PackageVersion On successful completion, populated with the package
        version.

 @param PackageArch On successful completion, populated with the package
        architecture.

 @param UpgradePath On successful completion, populated with a URL that will
        return the latest version of the package.

 @param SourcePath On successful completion, populated with a URL that will
        return the source code matching the package.

 @param SymbolPath On successful completion, populated with a URL that will
        return the symbols for the package.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YpmGetPackageInfo(
    __in PYORI_STRING IniPath,
    __out PYORI_STRING PackageName,
    __out PYORI_STRING PackageVersion,
    __out PYORI_STRING PackageArch,
    __out PYORI_STRING UpgradePath,
    __out PYORI_STRING SourcePath,
    __out PYORI_STRING SymbolPath
    )
{
    YORI_STRING TempBuffer;
    DWORD MaxFieldSize = YPM_MAX_FIELD_LENGTH;

    if (!YoriLibAllocateString(&TempBuffer, 6 * MaxFieldSize)) {
        return FALSE;
    }

    YoriLibCloneString(PackageName, &TempBuffer);
    PackageName->LengthAllocated = MaxFieldSize;

    PackageName->LengthInChars = GetPrivateProfileString(_T("Package"), _T("Name"), _T(""), PackageName->StartOfString, PackageName->LengthAllocated, IniPath->StartOfString);

    YoriLibCloneString(PackageVersion, &TempBuffer);
    PackageVersion->StartOfString += 1 * MaxFieldSize;
    PackageVersion->LengthAllocated = MaxFieldSize;

    PackageVersion->LengthInChars = GetPrivateProfileString(_T("Package"), _T("Version"), _T(""), PackageVersion->StartOfString, PackageVersion->LengthAllocated, IniPath->StartOfString);

    YoriLibCloneString(PackageArch, &TempBuffer);
    PackageArch->StartOfString += 2 * MaxFieldSize;
    PackageArch->LengthAllocated = MaxFieldSize;

    PackageArch->LengthInChars = GetPrivateProfileString(_T("Package"), _T("Architecture"), _T(""), PackageArch->StartOfString, PackageArch->LengthAllocated, IniPath->StartOfString);

    YoriLibCloneString(UpgradePath, &TempBuffer);
    UpgradePath->StartOfString += 3 * MaxFieldSize;
    UpgradePath->LengthAllocated = MaxFieldSize;

    UpgradePath->LengthInChars = GetPrivateProfileString(_T("Package"), _T("UpgradePath"), _T(""), UpgradePath->StartOfString, UpgradePath->LengthAllocated, IniPath->StartOfString);

    YoriLibCloneString(SourcePath, &TempBuffer);
    SourcePath->StartOfString += 4 * MaxFieldSize;
    SourcePath->LengthAllocated = MaxFieldSize;

    SourcePath->LengthInChars = GetPrivateProfileString(_T("Package"), _T("SourcePath"), _T(""), SourcePath->StartOfString, SourcePath->LengthAllocated, IniPath->StartOfString);

    YoriLibCloneString(SymbolPath, &TempBuffer);
    SymbolPath->StartOfString += 5 * MaxFieldSize;
    SymbolPath->LengthAllocated = MaxFieldSize;

    SymbolPath->LengthInChars = GetPrivateProfileString(_T("Package"), _T("SymbolPath"), _T(""), SymbolPath->StartOfString, SymbolPath->LengthAllocated, IniPath->StartOfString);

    YoriLibFreeStringContents(&TempBuffer);
    return TRUE;
}

/**
 Returns TRUE if the specified path is an Internet path that requires wininet.
 Technically this can return FALSE for paths that are still remote (SMB paths),
 but those are functionally the same as local paths.

 @param PackagePath Pointer to the path to check.

 @return TRUE if the path is an Internet path, FALSE if it is a local path.
 */
BOOL
YpmIsPathRemote(
    __in PYORI_STRING PackagePath
    )
{
    if (YoriLibCompareStringWithLiteralInsensitiveCount(PackagePath, _T("http://"), sizeof("http://") - 1) == 0 ||
        YoriLibCompareStringWithLiteralInsensitiveCount(PackagePath, _T("https://"), sizeof("https://") - 1) == 0 ||
        YoriLibCompareStringWithLiteralInsensitiveCount(PackagePath, _T("ftp://"), sizeof("ftp://") - 1) == 0) {

        return TRUE;
    }

    return FALSE;
}

/**
 Download a remote package into a temporary location and return the
 temporary location to allow for subsequent processing.

 @param PackagePath Pointer to a string referring to the package which can
        be local or remote.

 @param LocalPath On successful completion, populated with a string containing
        a fully qualified local path to the package.

 @param DeleteWhenFinished On successful completion, set to TRUE to indicate
        the caller should delete the file (it is temporary); set to FALSE to
        indicate the file should be retained.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YpmPackagePathToLocalPath(
    __in PYORI_STRING PackagePath,
    __out PYORI_STRING LocalPath,
    __out PBOOL DeleteWhenFinished
    )
{
    if (YpmIsPathRemote(PackagePath)) {

        YORI_STRING TempPath;
        YORI_STRING TempFileName;
        YORI_STRING UserAgent;
        YoriLibUpdError Error;
        YoriLibInitEmptyString(&TempPath);

        //
        //  Query for a temporary directory
        //

        TempPath.LengthAllocated = GetTempPath(0, NULL);
        if (!YoriLibAllocateString(&TempPath, TempPath.LengthAllocated)) {
            return FALSE;
        }
        TempPath.LengthInChars = GetTempPath(TempPath.LengthAllocated, TempPath.StartOfString);

        if (!YoriLibAllocateString(&TempFileName, MAX_PATH)) {
            YoriLibFreeStringContents(&TempPath);
            return FALSE;
        }

        if (GetTempFileName(TempPath.StartOfString, _T("ypm"), 0, TempFileName.StartOfString) == 0) {
            YoriLibFreeStringContents(&TempPath);
            YoriLibFreeStringContents(&TempFileName);
            return FALSE;
        }

        TempFileName.LengthInChars = _tcslen(TempFileName.StartOfString);

        YoriLibInitEmptyString(&UserAgent);
        YoriLibYPrintf(&UserAgent, _T("ypm %i.%i\r\n"), YPM_VER_MAJOR, YPM_VER_MINOR);
        if (UserAgent.StartOfString == NULL) {
            YoriLibFreeStringContents(&TempPath);
            YoriLibFreeStringContents(&TempFileName);
            return FALSE;
        }

        Error = YoriLibUpdateBinaryFromUrl(PackagePath->StartOfString, TempFileName.StartOfString, UserAgent.StartOfString);

        if (Error != YoriLibUpdErrorSuccess) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Network result: %s\n"), YoriLibUpdateErrorString(Error));
        }

        YoriLibFreeStringContents(&TempPath);
        YoriLibFreeStringContents(&UserAgent);

        if (Error != YoriLibUpdErrorSuccess) {
            YoriLibFreeStringContents(&TempFileName);
            return FALSE;
        }

        memcpy(LocalPath, &TempFileName, sizeof(YORI_STRING));
        *DeleteWhenFinished = TRUE;
        return TRUE;

    } else {
        *DeleteWhenFinished = FALSE;
        return YoriLibUserStringToSingleFilePath(PackagePath, FALSE, LocalPath);
    }
}

// vim:sw=4:ts=4:et:
