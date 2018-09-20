/**
 * @file pkglib/util.c
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
#include "yoripkg.h"

/**
 Return a fully qualified path to the directory containing the program.

 @param AppDirectory On successful completion, populated with the directory
        containing the application.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgGetApplicationDirectory(
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

 @param InstallDirectory Optionally specifies an install directory.  If not
        specified, the directory of the currently running application is
        used.  The INI file is always in this directory if it is specified.

 @param IniFileName On successful completion, populated with a path to the
        package INI file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgGetPackageIniFile(
    __in_opt PYORI_STRING InstallDirectory,
    __out PYORI_STRING IniFileName
    )
{
    YORI_STRING AppDirectory;

    YoriLibInitEmptyString(&AppDirectory);

    if (InstallDirectory == NULL) {
        if (!YoriPkgGetApplicationDirectory(&AppDirectory)) {
            return FALSE;
        }
    } else {
        if (!YoriLibAllocateString(&AppDirectory, InstallDirectory->LengthInChars + MAX_PATH)) {
            return FALSE;
        }
        memcpy(AppDirectory.StartOfString, InstallDirectory->StartOfString, InstallDirectory->LengthInChars * sizeof(TCHAR));
        AppDirectory.StartOfString[InstallDirectory->LengthInChars] = '\0';
        AppDirectory.LengthInChars = InstallDirectory->LengthInChars;
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
YoriPkgGetPackageInfo(
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
    DWORD MaxFieldSize = YORIPKG_MAX_FIELD_LENGTH;

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
 Expand a user specified package path into a full path if local, and compare
 the full path or URL against any mirrors in the INI file to determine if
 the path should be adjusted to refer to a mirrored location.

 @param PackagePath Pointer to a string referring to the package which can
        be local or remote.

 @param IniFilePath Pointer to a string containing a path to the package INI
        file.

 @param MirroredPath On successful completion, updated to refer to a
        substituted path from the user's mirrors list or to the full path of
        a local package.

 @return TRUE to indicate a substitution was performed, FALSE on error or
         if no substitution exists.
 */
BOOL
YoriPkgConvertUserPackagePathToMirroredPath(
    __in PYORI_STRING PackagePath,
    __in PYORI_STRING IniFilePath,
    __out PYORI_STRING MirroredPath
    )
{
    YORI_STRING IniSection;
    YORI_STRING HumanFullPath;
    YORI_STRING Find;
    YORI_STRING Replace;
    LPTSTR ThisLine;
    LPTSTR Equals;
    BOOL Result = FALSE;
    BOOL ReturnHumanPathIfNoMirrorFound = FALSE;
    DWORD Index;

    YoriLibInitEmptyString(&IniSection);
    YoriLibInitEmptyString(&HumanFullPath);
    YoriLibInitEmptyString(MirroredPath);
    YoriLibInitEmptyString(&Find);
    YoriLibInitEmptyString(&Replace);

    if (!YoriLibAllocateString(&IniSection, YORIPKG_MAX_SECTION_LENGTH)) {
        goto Exit;
    }

    if (!YoriLibIsPathUrl(PackagePath)) {
        if (!YoriLibUserStringToSingleFilePath(PackagePath, FALSE, &HumanFullPath)) {
            goto Exit;
        }
        ReturnHumanPathIfNoMirrorFound = TRUE;
    } else {
        YoriLibCloneString(&HumanFullPath, PackagePath);
    }

    IniSection.LengthInChars = GetPrivateProfileSection(_T("Mirrors"), IniSection.StartOfString, IniSection.LengthAllocated, IniFilePath->StartOfString);

    ThisLine = IniSection.StartOfString;

    while (*ThisLine != '\0') {
        Find.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        if (Equals == NULL) {
            ThisLine += _tcslen(ThisLine);
            ThisLine++;
            continue;
        }

        Find.LengthInChars = (DWORD)(Equals - ThisLine);

        //
        //  In order to allow '=' to be in the find/replace strings, which are
        //  not describable in an INI file, interpret '%' as '='.
        //

        for (Index = 0; Index < Find.LengthInChars; Index++) {
            if (Find.StartOfString[Index] == '%') {
                Find.StartOfString[Index] = '=';
            }
        }


        Replace.StartOfString = Equals + 1;
        Replace.LengthInChars = _tcslen(Replace.StartOfString);

        ThisLine += Find.LengthInChars + 1 + Replace.LengthInChars + 1;

        Equals[0] = '\0';

        if (YoriLibCompareStringInsensitiveCount(&Find, &HumanFullPath, Find.LengthInChars) == 0) {
            YORI_STRING SubstringToKeep;
            YoriLibInitEmptyString(&SubstringToKeep);
            SubstringToKeep.StartOfString = &HumanFullPath.StartOfString[Find.LengthInChars];
            SubstringToKeep.LengthInChars = HumanFullPath.LengthInChars - Find.LengthInChars;

            //
            //  In order to allow '=' to be in the find/replace strings, which are
            //  not describable in an INI file, interpret '%' as '='.
            //

            for (Index = 0; Index < Replace.LengthInChars; Index++) {
                if (Replace.StartOfString[Index] == '%') {
                    Replace.StartOfString[Index] = '=';
                }
            }

            YoriLibYPrintf(MirroredPath, _T("%y%y"), &Replace, &SubstringToKeep);
            if (MirroredPath->StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Converting %y to %y\n"), &HumanFullPath, MirroredPath);
                Result = TRUE;
            }
            goto Exit;
        }

    }

    //
    //  This doesn't really belong here, but since we've already converted the
    //  user string to a full path we can return it so the caller doesn't have
    //  to do it again
    //

    if (ReturnHumanPathIfNoMirrorFound) {
        YoriLibCloneString(MirroredPath, &HumanFullPath);
        Result = TRUE;
    }

Exit:
    YoriLibFreeStringContents(&IniSection);
    YoriLibFreeStringContents(&HumanFullPath);
    return Result;
}

/**
 Download a remote package into a temporary location and return the
 temporary location to allow for subsequent processing.

 @param PackagePath Pointer to a string referring to the package which can
        be local or remote.

 @param IniFilePath Pointer to a string containing a path to the package INI
        file.

 @param LocalPath On successful completion, populated with a string containing
        a fully qualified local path to the package.

 @param DeleteWhenFinished On successful completion, set to TRUE to indicate
        the caller should delete the file (it is temporary); set to FALSE to
        indicate the file should be retained.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgPackagePathToLocalPath(
    __in PYORI_STRING PackagePath,
    __in PYORI_STRING IniFilePath,
    __out PYORI_STRING LocalPath,
    __out PBOOL DeleteWhenFinished
    )
{
    YORI_STRING MirroredPath;
    BOOL Result = FALSE;

    YoriLibInitEmptyString(&MirroredPath);

    //
    //  See if there's a mirror for the package.  If anything goes wrong in
    //  this process, just keep using the original path.
    //

    if (!YoriPkgConvertUserPackagePathToMirroredPath(PackagePath, IniFilePath, &MirroredPath)) {
        YoriLibCloneString(&MirroredPath, PackagePath);
    }

    if (YoriLibIsPathUrl(&MirroredPath)) {

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
            goto Exit;
        }
        TempPath.LengthInChars = GetTempPath(TempPath.LengthAllocated, TempPath.StartOfString);

        if (!YoriLibAllocateString(&TempFileName, MAX_PATH)) {
            YoriLibFreeStringContents(&TempPath);
            return FALSE;
        }

        if (GetTempFileName(TempPath.StartOfString, _T("ypm"), 0, TempFileName.StartOfString) == 0) {
            YoriLibFreeStringContents(&TempPath);
            YoriLibFreeStringContents(&TempFileName);
            goto Exit;
        }

        TempFileName.LengthInChars = _tcslen(TempFileName.StartOfString);

        YoriLibInitEmptyString(&UserAgent);
        YoriLibYPrintf(&UserAgent, _T("ypm %i.%i\r\n"), YPM_VER_MAJOR, YPM_VER_MINOR);
        if (UserAgent.StartOfString == NULL) {
            YoriLibFreeStringContents(&TempPath);
            YoriLibFreeStringContents(&TempFileName);
            goto Exit;
        }

        Error = YoriLibUpdateBinaryFromUrl(MirroredPath.StartOfString, TempFileName.StartOfString, UserAgent.StartOfString);

        if (Error != YoriLibUpdErrorSuccess) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Network result: %s\n"), YoriLibUpdateErrorString(Error));
        }

        YoriLibFreeStringContents(&TempPath);
        YoriLibFreeStringContents(&UserAgent);

        if (Error != YoriLibUpdErrorSuccess) {
            YoriLibFreeStringContents(&TempFileName);
            goto Exit;
        }

        memcpy(LocalPath, &TempFileName, sizeof(YORI_STRING));
        *DeleteWhenFinished = TRUE;
        Result = TRUE;

    } else {
        *DeleteWhenFinished = FALSE;
        YoriLibCloneString(LocalPath, &MirroredPath);
        Result = TRUE;
    }

Exit:

    YoriLibFreeStringContents(&MirroredPath);
    return Result;
}

// vim:sw=4:ts=4:et:
