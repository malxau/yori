/**
 * @file pkglib/util.c
 *
 * Yori package manager helper functions
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

#include <yoripch.h>
#include <yorilib.h>
#include "yoripkgp.h"

/**
 Return a fully qualified path to the currently running program.

 @param ExecutableFile On successful completion, populated with the program
        file that is currently running.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgGetExecutableFile(
    __out PYORI_STRING ExecutableFile
    )
{
    YORI_STRING ModuleName;

    if (!YoriLibAllocateString(&ModuleName, 32768)) {
        return FALSE;
    }

    ModuleName.LengthInChars = (YORI_ALLOC_SIZE_T)GetModuleFileName(NULL, ModuleName.StartOfString, ModuleName.LengthAllocated);
    if (ModuleName.LengthInChars == 0) {
        YoriLibFreeStringContents(&ModuleName);
        return FALSE;
    }

    memcpy(ExecutableFile, &ModuleName, sizeof(YORI_STRING));
    return TRUE;
}


/**
 Return a fully qualified path to the directory containing the program.

 @param AppDirectory On successful completion, populated with the directory
        containing the application.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgGetApplicationDirectory(
    __out PYORI_STRING AppDirectory
    )
{
    YORI_STRING ModuleName;
    LPTSTR Ptr;

    if (!YoriPkgGetExecutableFile(&ModuleName)) {
        return FALSE;
    }

    Ptr = YoriLibFindRightMostCharacter(&ModuleName, '\\');
    if (Ptr == NULL) {
        YoriLibFreeStringContents(&ModuleName);
        return FALSE;
    }

    *Ptr = '\0';

    ModuleName.LengthInChars = (YORI_ALLOC_SIZE_T)(Ptr - ModuleName.StartOfString);
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
__success(return)
BOOL
YoriPkgGetPackageIniFile(
    __in_opt PCYORI_STRING InstallDirectory,
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

 @param MinimumOSBuild On successful completion, populated with the minimum
        OS build needed to install the package.

 @param PackagePathForOlderBuilds On successful completion, populated with a
        URL that will return a version of the package suitable for use on
        systems with less than MinimumOSBuild.

 @param UpgradePath On successful completion, populated with a URL that will
        return the latest version of the package.

 @param SourcePath On successful completion, populated with a URL that will
        return the source code matching the package.

 @param SymbolPath On successful completion, populated with a URL that will
        return the symbols for the package.

 @param UpgradeToDailyPath On successful completion, populated with a URL
        that will return the latest daily version of the package.

 @param UpgradeToStablePath On successful completion, populated with a URL
        that will return the latest stable version of the package.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgGetPackageInfo(
    __in PCYORI_STRING IniPath,
    __out PYORI_STRING PackageName,
    __out PYORI_STRING PackageVersion,
    __out PYORI_STRING PackageArch,
    __out PYORI_STRING MinimumOSBuild,
    __out PYORI_STRING PackagePathForOlderBuilds,
    __out PYORI_STRING UpgradePath,
    __out PYORI_STRING SourcePath,
    __out PYORI_STRING SymbolPath,
    __out PYORI_STRING UpgradeToDailyPath,
    __out PYORI_STRING UpgradeToStablePath
    )
{
    YORI_STRING TempBuffer;
    YORI_ALLOC_SIZE_T MaxFieldSize = YORIPKG_MAX_FIELD_LENGTH;

    if (DllKernel32.pGetPrivateProfileStringW == NULL) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&TempBuffer, 10 * MaxFieldSize)) {
        return FALSE;
    }

    YoriLibCloneString(PackageName, &TempBuffer);
    PackageName->LengthAllocated = MaxFieldSize;

    PackageName->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(_T("Package"),
                                              _T("Name"),
                                              _T(""),
                                              PackageName->StartOfString,
                                              PackageName->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(PackageVersion, &TempBuffer);
    PackageVersion->StartOfString += 1 * MaxFieldSize;
    PackageVersion->LengthAllocated = MaxFieldSize;

    PackageVersion->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(_T("Package"),
                                              _T("Version"),
                                              _T(""),
                                              PackageVersion->StartOfString,
                                              PackageVersion->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(PackageArch, &TempBuffer);
    PackageArch->StartOfString += 2 * MaxFieldSize;
    PackageArch->LengthAllocated = MaxFieldSize;

    PackageArch->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(_T("Package"),
                                              _T("Architecture"),
                                              _T(""),
                                              PackageArch->StartOfString,
                                              PackageArch->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(MinimumOSBuild, &TempBuffer);
    MinimumOSBuild->StartOfString += 3 * MaxFieldSize;
    MinimumOSBuild->LengthAllocated = MaxFieldSize;

    MinimumOSBuild->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(_T("Package"),
                                              _T("MinimumOSBuild"),
                                              _T(""),
                                              MinimumOSBuild->StartOfString,
                                              MinimumOSBuild->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(PackagePathForOlderBuilds, &TempBuffer);
    PackagePathForOlderBuilds->StartOfString += 4 * MaxFieldSize;
    PackagePathForOlderBuilds->LengthAllocated = MaxFieldSize;

    PackagePathForOlderBuilds->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(_T("Package"),
                                              _T("PackagePathForOlderBuilds"),
                                              _T(""),
                                              PackagePathForOlderBuilds->StartOfString,
                                              PackagePathForOlderBuilds->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(UpgradePath, &TempBuffer);
    UpgradePath->StartOfString += 5 * MaxFieldSize;
    UpgradePath->LengthAllocated = MaxFieldSize;

    UpgradePath->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(_T("Package"),
                                              _T("UpgradePath"),
                                              _T(""),
                                              UpgradePath->StartOfString,
                                              UpgradePath->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(SourcePath, &TempBuffer);
    SourcePath->StartOfString += 6 * MaxFieldSize;
    SourcePath->LengthAllocated = MaxFieldSize;

    SourcePath->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(_T("Package"),
                                              _T("SourcePath"),
                                              _T(""),
                                              SourcePath->StartOfString,
                                              SourcePath->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(SymbolPath, &TempBuffer);
    SymbolPath->StartOfString += 7 * MaxFieldSize;
    SymbolPath->LengthAllocated = MaxFieldSize;

    SymbolPath->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(_T("Package"),
                                              _T("SymbolPath"),
                                              _T(""),
                                              SymbolPath->StartOfString,
                                              SymbolPath->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(UpgradeToDailyPath, &TempBuffer);
    UpgradeToDailyPath->StartOfString += 8 * MaxFieldSize;
    UpgradeToDailyPath->LengthAllocated = MaxFieldSize;

    UpgradeToDailyPath->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(_T("Package"),
                                              _T("UpgradeToDailyPath"),
                                              _T(""),
                                              UpgradeToDailyPath->StartOfString,
                                              UpgradeToDailyPath->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(UpgradeToStablePath, &TempBuffer);
    UpgradeToStablePath->StartOfString += 9 * MaxFieldSize;
    UpgradeToStablePath->LengthAllocated = MaxFieldSize;

    UpgradeToStablePath->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(_T("Package"),
                                              _T("UpgradeToStablePath"),
                                              _T(""),
                                              UpgradeToStablePath->StartOfString,
                                              UpgradeToStablePath->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibFreeStringContents(&TempBuffer);
    return TRUE;
}

/**
 Given a fully qualified path to the system package INI file and a package
 name, extract fixed sized information.

 @param IniPath Pointer to a path to the system's INI file.

 @param PackageName Pointer to the package's canonical name.

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

 @param UpgradeToDailyPath On successful completion, populated with a URL
        that will return the latest daily version of the package.

 @param UpgradeToStablePath On successful completion, populated with a URL
        that will return the latest stable version of the package.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgGetInstalledPackageInfo(
    __in PYORI_STRING IniPath,
    __in PYORI_STRING PackageName,
    __out PYORI_STRING PackageVersion,
    __out PYORI_STRING PackageArch,
    __out PYORI_STRING UpgradePath,
    __out PYORI_STRING SourcePath,
    __out PYORI_STRING SymbolPath,
    __out PYORI_STRING UpgradeToDailyPath,
    __out PYORI_STRING UpgradeToStablePath
    )
{
    YORI_STRING TempBuffer;
    YORI_ALLOC_SIZE_T MaxFieldSize = YORIPKG_MAX_FIELD_LENGTH;

    ASSERT(YoriLibIsStringNullTerminated(IniPath));
    ASSERT(YoriLibIsStringNullTerminated(PackageName));

    if (DllKernel32.pGetPrivateProfileStringW == NULL) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&TempBuffer, 7 * MaxFieldSize)) {
        return FALSE;
    }

    YoriLibCloneString(PackageVersion, &TempBuffer);
    PackageVersion->LengthAllocated = MaxFieldSize;

    PackageVersion->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(PackageName->StartOfString,
                                              _T("Version"),
                                              _T(""),
                                              PackageVersion->StartOfString,
                                              PackageVersion->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(PackageArch, &TempBuffer);
    PackageArch->StartOfString += 1 * MaxFieldSize;
    PackageArch->LengthAllocated = MaxFieldSize;

    PackageArch->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(PackageName->StartOfString,
                                              _T("Architecture"),
                                              _T(""),
                                              PackageArch->StartOfString,
                                              PackageArch->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(UpgradePath, &TempBuffer);
    UpgradePath->StartOfString += 2 * MaxFieldSize;
    UpgradePath->LengthAllocated = MaxFieldSize;

    UpgradePath->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(PackageName->StartOfString,
                                              _T("UpgradePath"),
                                              _T(""),
                                              UpgradePath->StartOfString,
                                              UpgradePath->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(SourcePath, &TempBuffer);
    SourcePath->StartOfString += 3 * MaxFieldSize;
    SourcePath->LengthAllocated = MaxFieldSize;

    SourcePath->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(PackageName->StartOfString,
                                              _T("SourcePath"),
                                              _T(""),
                                              SourcePath->StartOfString,
                                              SourcePath->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(SymbolPath, &TempBuffer);
    SymbolPath->StartOfString += 4 * MaxFieldSize;
    SymbolPath->LengthAllocated = MaxFieldSize;

    SymbolPath->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(PackageName->StartOfString,
                                              _T("SymbolPath"),
                                              _T(""),
                                              SymbolPath->StartOfString,
                                              SymbolPath->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(UpgradeToDailyPath, &TempBuffer);
    UpgradeToDailyPath->StartOfString += 5 * MaxFieldSize;
    UpgradeToDailyPath->LengthAllocated = MaxFieldSize;

    UpgradeToDailyPath->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(PackageName->StartOfString,
                                              _T("UpgradeToDailyPath"),
                                              _T(""),
                                              UpgradeToDailyPath->StartOfString,
                                              UpgradeToDailyPath->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibCloneString(UpgradeToStablePath, &TempBuffer);
    UpgradeToStablePath->StartOfString += 5 * MaxFieldSize;
    UpgradeToStablePath->LengthAllocated = MaxFieldSize;

    UpgradeToStablePath->LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileStringW(PackageName->StartOfString,
                                              _T("UpgradeToStablePath"),
                                              _T(""),
                                              UpgradeToStablePath->StartOfString,
                                              UpgradeToStablePath->LengthAllocated,
                                              IniPath->StartOfString);

    YoriLibFreeStringContents(&TempBuffer);
    return TRUE;
}

/**
 A structure identifying the mapping from one source location to a mirrored
 location.
 */
typedef struct _YORIPKG_MIRROR {

    /**
     The item of this mirror within a list of mirrors.
     */
    YORI_LIST_ENTRY MirrorList;

    /**
     The source path, meaning the path that packages refer to.
     */
    YORI_STRING SourceName;

    /**
     The target path, meaning the path that should be used instead of the
     path contained in packages.
     */
    YORI_STRING TargetName;
} YORIPKG_MIRROR, *PYORIPKG_MIRROR;

/**
 Load mirrors from the Ini file into a list.

 @param IniFilePath Pointer to the path to the packages.ini file.

 @param MirrorsList On input, contains an initialized list, which will be
        populated with mirrors found in the INI file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgLoadMirrorsFromIni(
    __in PYORI_STRING IniFilePath,
    __inout PYORI_LIST_ENTRY MirrorsList
    )
{
    YORI_STRING IniSection;
    YORI_STRING Find;
    YORI_STRING Replace;
    LPTSTR ThisLine;
    LPTSTR Equals;
    BOOL Result = FALSE;
    YORI_ALLOC_SIZE_T Index;
    PYORIPKG_MIRROR Mirror;

    YoriLibInitEmptyString(&IniSection);
    YoriLibInitEmptyString(&Find);
    YoriLibInitEmptyString(&Replace);

    if (DllKernel32.pGetPrivateProfileSectionW == NULL) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&IniSection, YORIPKG_MAX_SECTION_LENGTH)) {
        goto Exit;
    }

    IniSection.LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileSectionW(_T("Mirrors"),
                                               IniSection.StartOfString,
                                               IniSection.LengthAllocated,
                                               IniFilePath->StartOfString);

    ThisLine = IniSection.StartOfString;

    while (*ThisLine != '\0') {
        Find.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        if (Equals == NULL) {
            ThisLine += _tcslen(ThisLine);
            ThisLine++;
            continue;
        }

        Find.LengthInChars = (YORI_ALLOC_SIZE_T)(Equals - ThisLine);

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
        Replace.LengthInChars = (YORI_ALLOC_SIZE_T)_tcslen(Replace.StartOfString);

        ThisLine += Find.LengthInChars + 1 + Replace.LengthInChars + 1;

        Equals[0] = '\0';

        //
        //  In order to allow '=' to be in the find/replace strings, which are
        //  not describable in an INI file, interpret '%' as '='.
        //

        for (Index = 0; Index < Replace.LengthInChars; Index++) {
            if (Replace.StartOfString[Index] == '%') {
                Replace.StartOfString[Index] = '=';
            }
        }

        Mirror = YoriLibReferencedMalloc(sizeof(YORIPKG_MIRROR) + (Find.LengthInChars + 1 + Replace.LengthInChars + 1) * sizeof(TCHAR));
        if (Mirror == NULL) {
            goto Exit;
        }

        YoriLibInitEmptyString(&Mirror->SourceName);
        Mirror->SourceName.StartOfString = (LPTSTR)(Mirror + 1);
        Mirror->SourceName.LengthInChars = Find.LengthInChars;
        memcpy(Mirror->SourceName.StartOfString, Find.StartOfString, Find.LengthInChars * sizeof(TCHAR));
        Mirror->SourceName.StartOfString[Mirror->SourceName.LengthInChars] = '\0';
        Mirror->SourceName.LengthAllocated = Mirror->SourceName.LengthInChars + 1;
        YoriLibInitEmptyString(&Mirror->TargetName);
        Mirror->TargetName.StartOfString = (LPTSTR)(Mirror->SourceName.StartOfString + Mirror->SourceName.LengthAllocated);
        Mirror->TargetName.LengthInChars = Replace.LengthInChars;
        memcpy(Mirror->TargetName.StartOfString, Replace.StartOfString, Replace.LengthInChars * sizeof(TCHAR));
        Mirror->TargetName.StartOfString[Mirror->TargetName.LengthInChars] = '\0';
        Mirror->TargetName.LengthAllocated = Mirror->TargetName.LengthInChars + 1;

        YoriLibAppendList(MirrorsList, &Mirror->MirrorList);
    }

    Result = TRUE;

Exit:
    YoriLibFreeStringContents(&IniSection);
    return Result;
}

/**
 Free a list of allocated mirrors.

 @param MirrorsList Pointer to the list of mirrors to free.
 */
VOID
YoriPkgFreeMirrorList(
    __in PYORI_LIST_ENTRY MirrorsList
    )
{
    PYORI_LIST_ENTRY MirrorEntry;
    PYORIPKG_MIRROR Mirror;

    MirrorEntry = NULL;
    MirrorEntry = YoriLibGetNextListEntry(MirrorsList, MirrorEntry);
    while (MirrorEntry != NULL) {
        Mirror = CONTAINING_RECORD(MirrorEntry, YORIPKG_MIRROR, MirrorList);
        MirrorEntry = YoriLibGetNextListEntry(MirrorsList, MirrorEntry);
        YoriLibRemoveListItem(&Mirror->MirrorList);
        YoriLibDereference(Mirror);
    }

}

/**
 Read the current list of mirrors from the system global packages.ini file
 and display the result to output.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgDisplayMirrors(VOID)
{
    YORI_LIST_ENTRY MirrorsList;
    PYORI_LIST_ENTRY MirrorEntry;
    PYORIPKG_MIRROR Mirror;
    YORI_STRING PackagesIni;

    YoriLibInitializeListHead(&MirrorsList);

    if (!YoriPkgGetPackageIniFile(NULL, &PackagesIni)) {
        return FALSE;
    }

    if (!YoriPkgLoadMirrorsFromIni(&PackagesIni, &MirrorsList)) {
        YoriLibFreeStringContents(&PackagesIni);
        return TRUE;
    }

    YoriLibFreeStringContents(&PackagesIni);

    //
    //  Display the mirrors we found.
    //

    MirrorEntry = NULL;
    MirrorEntry = YoriLibGetNextListEntry(&MirrorsList, MirrorEntry);
    while (MirrorEntry != NULL) {
        Mirror = CONTAINING_RECORD(MirrorEntry, YORIPKG_MIRROR, MirrorList);
        MirrorEntry = YoriLibGetNextListEntry(&MirrorsList, MirrorEntry);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y %y\n"), &Mirror->SourceName, &Mirror->TargetName);
    }

    //
    //  Free the mirrors we found.
    //

    YoriPkgFreeMirrorList(&MirrorsList);
    return TRUE;
}

/**
 Install a new mirror and add it to packages.ini

 @param SourceName The new source path that is being mirrored

 @param TargetName The new target containing mirrored contents

 @param InstallAsFirst If TRUE, the new source is added to the beginning of
        the list.  If FALSE, it is added to the end.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgAddNewMirror(
    __in PYORI_STRING SourceName,
    __in PYORI_STRING TargetName,
    __in BOOLEAN InstallAsFirst
    )
{
    YORI_LIST_ENTRY MirrorsList;
    PYORI_LIST_ENTRY MirrorEntry;
    PYORIPKG_MIRROR Mirror;
    PYORIPKG_MIRROR NewMirror;
    YORI_STRING PackagesIni;
    DWORD Index;

    YoriLibInitializeListHead(&MirrorsList);

    if (DllKernel32.pWritePrivateProfileStringW == NULL) {
        return FALSE;
    }

    if (!YoriPkgGetPackageIniFile(NULL, &PackagesIni)) {
        return FALSE;
    }

    if (!YoriPkgLoadMirrorsFromIni(&PackagesIni, &MirrorsList)) {
        YoriLibFreeStringContents(&PackagesIni);
        return FALSE;
    }

    //
    //  Allocate a new mirror entry for the user's request.
    //

    NewMirror = YoriLibReferencedMalloc(sizeof(YORIPKG_MIRROR) + (SourceName->LengthInChars + 1 + TargetName->LengthInChars + 1) * sizeof(TCHAR));
    if (NewMirror == NULL) {
        YoriLibFreeStringContents(&PackagesIni);
        return FALSE;
    }

    YoriLibInitEmptyString(&NewMirror->SourceName);
    NewMirror->SourceName.StartOfString = (LPTSTR)(NewMirror + 1);
    NewMirror->SourceName.LengthInChars = SourceName->LengthInChars;
    memcpy(NewMirror->SourceName.StartOfString, SourceName->StartOfString, SourceName->LengthInChars * sizeof(TCHAR));
    NewMirror->SourceName.StartOfString[NewMirror->SourceName.LengthInChars] = '\0';
    NewMirror->SourceName.LengthAllocated = NewMirror->SourceName.LengthInChars + 1;
    YoriLibInitEmptyString(&NewMirror->TargetName);
    NewMirror->TargetName.StartOfString = (LPTSTR)(NewMirror->SourceName.StartOfString + NewMirror->SourceName.LengthAllocated);
    NewMirror->TargetName.LengthInChars = TargetName->LengthInChars;
    memcpy(NewMirror->TargetName.StartOfString, TargetName->StartOfString, TargetName->LengthInChars * sizeof(TCHAR));
    NewMirror->TargetName.StartOfString[NewMirror->TargetName.LengthInChars] = '\0';
    NewMirror->TargetName.LengthAllocated = NewMirror->TargetName.LengthInChars + 1;

    //
    //  Replace = with %
    //

    for (Index = 0; Index < NewMirror->SourceName.LengthInChars; Index++) {
        if (NewMirror->SourceName.StartOfString[Index] == '=') {
            NewMirror->SourceName.StartOfString[Index] = '%';
        }
    }

    for (Index = 0; Index < NewMirror->TargetName.LengthInChars; Index++) {
        if (NewMirror->TargetName.StartOfString[Index] == '=') {
            NewMirror->TargetName.StartOfString[Index] = '%';
        }
    }

    //
    //  Go through the list.  If we find a matching entry, remove it.
    //

    MirrorEntry = NULL;
    MirrorEntry = YoriLibGetNextListEntry(&MirrorsList, MirrorEntry);
    while (MirrorEntry != NULL) {
        Mirror = CONTAINING_RECORD(MirrorEntry, YORIPKG_MIRROR, MirrorList);
        MirrorEntry = YoriLibGetNextListEntry(&MirrorsList, MirrorEntry);

        //
        //  Convert each loaded entry back into its storage format.  We need
        //  to do this before writing them, and need the entries to be
        //  consistent for the compare.
        //

        for (Index = 0; Index < Mirror->SourceName.LengthInChars; Index++) {
            if (Mirror->SourceName.StartOfString[Index] == '=') {
                Mirror->SourceName.StartOfString[Index] = '%';
            }
        }

        for (Index = 0; Index < Mirror->TargetName.LengthInChars; Index++) {
            if (Mirror->TargetName.StartOfString[Index] == '=') {
                Mirror->TargetName.StartOfString[Index] = '%';
            }
        }

        if (YoriLibCompareString(&Mirror->SourceName, &NewMirror->SourceName) == 0) {
            YoriLibRemoveListItem(&Mirror->MirrorList);
            YoriLibDereference(Mirror);
        }
    }

    //
    //  Insert the new item at the beginning or end per request
    //

    if (InstallAsFirst) {
        YoriLibInsertList(&MirrorsList, &NewMirror->MirrorList);
    } else {
        YoriLibAppendList(&MirrorsList, &NewMirror->MirrorList);
    }

    //
    //  Rewrite the section
    //

    DllKernel32.pWritePrivateProfileStringW(_T("Mirrors"), NULL, NULL, PackagesIni.StartOfString);
    MirrorEntry = NULL;
    MirrorEntry = YoriLibGetNextListEntry(&MirrorsList, MirrorEntry);
    while (MirrorEntry != NULL) {
        Mirror = CONTAINING_RECORD(MirrorEntry, YORIPKG_MIRROR, MirrorList);
        MirrorEntry = YoriLibGetNextListEntry(&MirrorsList, MirrorEntry);
        DllKernel32.pWritePrivateProfileStringW(_T("Mirrors"), Mirror->SourceName.StartOfString, Mirror->TargetName.StartOfString, PackagesIni.StartOfString);
    }

    //
    //  Free the mirrors we found.
    //

    YoriPkgFreeMirrorList(&MirrorsList);
    YoriLibFreeStringContents(&PackagesIni);
    return TRUE;
}

/**
 Delete a mirror from packages.ini

 @param SourceName The source path that was being mirrored

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgDeleteMirror(
    __in PYORI_STRING SourceName
    )
{
    YORI_LIST_ENTRY MirrorsList;
    PYORI_LIST_ENTRY MirrorEntry;
    PYORIPKG_MIRROR Mirror;
    YORI_STRING PackagesIni;
    DWORD Index;

    YoriLibInitializeListHead(&MirrorsList);

    if (DllKernel32.pWritePrivateProfileStringW == NULL) {
        return FALSE;
    }

    if (!YoriPkgGetPackageIniFile(NULL, &PackagesIni)) {
        return FALSE;
    }

    if (!YoriPkgLoadMirrorsFromIni(&PackagesIni, &MirrorsList)) {
        YoriLibFreeStringContents(&PackagesIni);
        return FALSE;
    }

    //
    //  Go through the list.  If we find a matching entry, remove it.
    //

    MirrorEntry = NULL;
    MirrorEntry = YoriLibGetNextListEntry(&MirrorsList, MirrorEntry);
    while (MirrorEntry != NULL) {
        Mirror = CONTAINING_RECORD(MirrorEntry, YORIPKG_MIRROR, MirrorList);
        MirrorEntry = YoriLibGetNextListEntry(&MirrorsList, MirrorEntry);

        if (YoriLibCompareString(&Mirror->SourceName, SourceName) == 0) {
            YoriLibRemoveListItem(&Mirror->MirrorList);
            YoriLibDereference(Mirror);
        } else {

            //
            //  Convert each loaded entry back into its storage format.
            //

            for (Index = 0; Index < Mirror->SourceName.LengthInChars; Index++) {
                if (Mirror->SourceName.StartOfString[Index] == '=') {
                    Mirror->SourceName.StartOfString[Index] = '%';
                }
            }

            for (Index = 0; Index < Mirror->TargetName.LengthInChars; Index++) {
                if (Mirror->TargetName.StartOfString[Index] == '=') {
                    Mirror->TargetName.StartOfString[Index] = '%';
                }
            }
        }
    }

    //
    //  Rewrite the section
    //

    DllKernel32.pWritePrivateProfileStringW(_T("Mirrors"), NULL, NULL, PackagesIni.StartOfString);
    MirrorEntry = NULL;
    MirrorEntry = YoriLibGetNextListEntry(&MirrorsList, MirrorEntry);
    while (MirrorEntry != NULL) {
        Mirror = CONTAINING_RECORD(MirrorEntry, YORIPKG_MIRROR, MirrorList);
        MirrorEntry = YoriLibGetNextListEntry(&MirrorsList, MirrorEntry);
        DllKernel32.pWritePrivateProfileStringW(_T("Mirrors"), Mirror->SourceName.StartOfString, Mirror->TargetName.StartOfString, PackagesIni.StartOfString);
    }

    //
    //  Free the mirrors we found.
    //

    YoriPkgFreeMirrorList(&MirrorsList);
    YoriLibFreeStringContents(&PackagesIni);
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
__success(return)
BOOL
YoriPkgConvertUserPackagePathToMirroredPath(
    __in PYORI_STRING PackagePath,
    __in PCYORI_STRING IniFilePath,
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
    YORI_ALLOC_SIZE_T Index;

    YoriLibInitEmptyString(&IniSection);
    YoriLibInitEmptyString(&HumanFullPath);
    YoriLibInitEmptyString(MirroredPath);
    YoriLibInitEmptyString(&Find);
    YoriLibInitEmptyString(&Replace);

    if (DllKernel32.pGetPrivateProfileSectionW == NULL) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&IniSection, YORIPKG_MAX_SECTION_LENGTH)) {
        goto Exit;
    }

    if (!YoriLibIsPathUrl(PackagePath)) {
        if (!YoriLibUserToSingleFilePath(PackagePath, FALSE, &HumanFullPath)) {
            YoriLibInitEmptyString(&HumanFullPath);
            goto Exit;
        }
        ReturnHumanPathIfNoMirrorFound = TRUE;
    } else {
        YoriLibCloneString(&HumanFullPath, PackagePath);
    }

    IniSection.LengthInChars = (YORI_ALLOC_SIZE_T)
        DllKernel32.pGetPrivateProfileSectionW(_T("Mirrors"),
                                               IniSection.StartOfString,
                                               IniSection.LengthAllocated,
                                               IniFilePath->StartOfString);

    ThisLine = IniSection.StartOfString;

    while (*ThisLine != '\0') {
        Find.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        if (Equals == NULL) {
            ThisLine += _tcslen(ThisLine);
            ThisLine++;
            continue;
        }

        Find.LengthInChars = (YORI_ALLOC_SIZE_T)(Equals - ThisLine);

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
        Replace.LengthInChars = (YORI_ALLOC_SIZE_T)_tcslen(Replace.StartOfString);

        ThisLine += Find.LengthInChars + 1 + Replace.LengthInChars + 1;

        Equals[0] = '\0';

        if (YoriLibCompareStringInsCnt(&Find, &HumanFullPath, Find.LengthInChars) == 0) {
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

 @return ERROR_SUCCESS to indicate success, or other Win32 error to indicate
         the type of failure.
 */
__success(return == ERROR_SUCCESS)
DWORD
YoriPkgPackagePathToLocalPath(
    __in PYORI_STRING PackagePath,
    __in_opt PCYORI_STRING IniFilePath,
    __out PYORI_STRING LocalPath,
    __out PBOOLEAN DeleteWhenFinished
    )
{
    YORI_STRING MirroredPath;
    DWORD Result = ERROR_SUCCESS;

    YoriLibInitEmptyString(&MirroredPath);

    //
    //  See if there's a mirror for the package.  If anything goes wrong in
    //  this process, just keep using the original path.  If there's no INI
    //  file, there's no possibility of a mirror, so perform path expansion
    //  on local paths and keep URLs verbatim.
    //

    if (IniFilePath == NULL) {
        if (!YoriLibIsPathUrl(PackagePath)) {
            if (!YoriLibUserToSingleFilePath(PackagePath, FALSE, &MirroredPath)) {
                YoriLibCloneString(&MirroredPath, PackagePath);
            }
        } else {
            YoriLibCloneString(&MirroredPath, PackagePath);
        }
    } else if (!YoriPkgConvertUserPackagePathToMirroredPath(PackagePath, IniFilePath, &MirroredPath)) {
        YoriLibCloneString(&MirroredPath, PackagePath);
    }

    if (YoriLibIsPathUrl(&MirroredPath)) {

        YORI_STRING TempPath;
        YORI_STRING TempFileName;
        YORI_STRING UserAgent;
        YORI_LIB_UPDATE_ERROR Error;
        YoriLibInitEmptyString(&TempPath);

        //
        //  Query for a temporary directory
        //

        if (!YoriLibGetTempPath(&TempPath, 0)) {
            Result = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        if (!YoriLibAllocateString(&TempFileName, MAX_PATH)) {
            YoriLibFreeStringContents(&TempPath);
            Result = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        //
        //  This will attempt to create a temporary file.  If it fails, that
        //  implies the temp directory is not writable, does not exist, or
        //  is unusable for some other reason.
        //

        if (GetTempFileName(TempPath.StartOfString, _T("ypm"), 0, TempFileName.StartOfString) == 0) {
            YoriLibFreeStringContents(&TempPath);
            YoriLibFreeStringContents(&TempFileName);
            Result = ERROR_BAD_ENVIRONMENT;
            goto Exit;
        }

        TempFileName.LengthInChars = (YORI_ALLOC_SIZE_T)_tcslen(TempFileName.StartOfString);

        YoriLibInitEmptyString(&UserAgent);
        YoriLibYPrintf(&UserAgent, _T("ypm %i.%02i\r\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
        if (UserAgent.StartOfString == NULL) {
            YoriLibFreeStringContents(&TempPath);
            YoriLibFreeStringContents(&TempFileName);
            Result = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        Error = YoriLibUpdateBinaryFromUrl(&MirroredPath, &TempFileName, &UserAgent, NULL);

        if (Error != YoriLibUpdErrorSuccess) {
            switch(Error) {
                case YoriLibUpdErrorInetInit:
                case YoriLibUpdErrorInetConnect:
                case YoriLibUpdErrorInetRead:
                case YoriLibUpdErrorInetContents:
                    Result = ERROR_NO_NETWORK;
                    break;
                case YoriLibUpdErrorFileWrite:
                case YoriLibUpdErrorFileReplace:
                    Result = ERROR_WRITE_FAULT;
                    break;
                default:
                    Result = ERROR_NOT_SUPPORTED;
                    break;
            }
        }

        YoriLibFreeStringContents(&TempPath);
        YoriLibFreeStringContents(&UserAgent);

        if (Result != ERROR_SUCCESS) {
            YoriLibFreeStringContents(&TempFileName);
            goto Exit;
        }

        memcpy(LocalPath, &TempFileName, sizeof(YORI_STRING));
        *DeleteWhenFinished = TRUE;

    } else {
        *DeleteWhenFinished = FALSE;
        YoriLibCloneString(LocalPath, &MirroredPath);
    }

Exit:

    YoriLibFreeStringContents(&MirroredPath);
    return Result;
}

/**
 Display the best available error text given an installation failure with the
 specified Win32 error code.

 @param ErrorCode The error code to display a message for.
 */
VOID
YoriPkgDisplayErrorStringForInstallFailure(
    __in SYSERR ErrorCode
    )
{
    LPTSTR ErrText;
    YORI_STRING AdminName;
    BOOL RunningAsAdmin = FALSE;

    switch(ErrorCode) {
        case ERROR_ALREADY_ASSIGNED:
            //
            //  This error means "we already told the user about it in a
            //  more specific way so please do nothing later."
            //
            break;
        case ERROR_WRITE_FAULT:
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not write to temporary directory.\n"));
            break;
        case ERROR_NO_NETWORK:
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not download package from the network.\n"));
            break;
        case ERROR_BAD_ENVIRONMENT:
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("The temporary directory does not exist or cannot be written to.\n"));
            break;
        case ERROR_ACCESS_DENIED:
            YoriLibConstantString(&AdminName, _T("Administrators"));
            if (!YoriLibIsCurrentUserInGroup(&AdminName, &RunningAsAdmin) || RunningAsAdmin) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Access denied when writing to files.\n"));
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Not running as Administrator and could not write to files.  Perhaps elevation is required?\n"));
            }
            break;

        default:
            ErrText = YoriLibGetWinErrorText(ErrorCode);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);

    }
}

// vim:sw=4:ts=4:et:
