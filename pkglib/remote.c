/**
 * @file pkglib/remote.c
 *
 * Yori package manager remote source query and search
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
#include "yoripkgp.h"

/**
 Information about a single package that was found on the remote source.
 */
typedef struct _YORIPKG_REMOTE_PACKAGE {

    /**
     A list of packages that have currently been discovered.
     */
    YORI_LIST_ENTRY PackageList;

    /**
     The name of the package.
     */
    YORI_STRING PackageName;

    /**
     The version of the package.
     */
    YORI_STRING Version;

    /**
     The CPU architecture of the package.
     */
    YORI_STRING Architecture;

    /**
     A fully qualified path name or URL that contains the package.
     */
    YORI_STRING InstallUrl;

    /**
     If attempting an upgrade, points to a backup of the previous version of
     the package.
     */
    PYORIPKG_BACKUP_PACKAGE Backup;

} YORIPKG_REMOTE_PACKAGE, *PYORIPKG_REMOTE_PACKAGE;

/**
 Information about a single remote source that contains a set of packages.
 */
typedef struct _YORIPKG_REMOTE_SOURCE {

    /**
     The links between all of the remote sources.
     */
    YORI_LIST_ENTRY SourceList;

    /**
     The root of the remote source (parent of pkglist.ini).
     */
    YORI_STRING SourceRootUrl;

    /**
     The path to the pkglist.ini file within the remote source.
     */
    YORI_STRING SourcePkgList;
} YORIPKG_REMOTE_SOURCE, *PYORIPKG_REMOTE_SOURCE;

/**
 Allocate and populate a remote source object.

 @param RemoteSourceUrl The root of the remote source.  This can be a URL or
        a local path.

 @return Pointer to the source object.  This can be freed with
         @ref YoriPkgFreeRemoteSource .  This will be NULL on allocation failure.

 */
PYORIPKG_REMOTE_SOURCE
YoriPkgAllocateRemoteSource(
    __in PYORI_STRING RemoteSourceUrl
    )
{
    PYORIPKG_REMOTE_SOURCE RemoteSource;
    DWORD SizeToAllocate;

    SizeToAllocate = sizeof(YORIPKG_REMOTE_SOURCE) +
                     (2 * (RemoteSourceUrl->LengthInChars + 1)) * sizeof(TCHAR) +
                     sizeof("/pkglist.ini") * sizeof(TCHAR);

    RemoteSource = YoriLibReferencedMalloc(SizeToAllocate);
    if (RemoteSource == NULL) {
        return NULL;
    }

    ZeroMemory(RemoteSource, SizeToAllocate);
    YoriLibInitializeListHead(&RemoteSource->SourceList);

    YoriLibReference(RemoteSource);
    RemoteSource->SourceRootUrl.MemoryToFree = RemoteSource;
    RemoteSource->SourceRootUrl.StartOfString = (LPTSTR)(RemoteSource + 1);
    RemoteSource->SourceRootUrl.LengthInChars = RemoteSourceUrl->LengthInChars;
    memcpy(RemoteSource->SourceRootUrl.StartOfString, RemoteSourceUrl->StartOfString, RemoteSourceUrl->LengthInChars * sizeof(TCHAR));
    if (YoriLibIsSep(RemoteSource->SourceRootUrl.StartOfString[RemoteSource->SourceRootUrl.LengthInChars - 1])) {
        RemoteSource->SourceRootUrl.LengthInChars--;
    }
    RemoteSource->SourceRootUrl.StartOfString[RemoteSource->SourceRootUrl.LengthInChars] = '\0';
    RemoteSource->SourceRootUrl.LengthAllocated = RemoteSource->SourceRootUrl.LengthInChars + 1;

    YoriLibReference(RemoteSource);
    RemoteSource->SourcePkgList.MemoryToFree = RemoteSource;
    RemoteSource->SourcePkgList.StartOfString = (LPTSTR)YoriLibAddToPointer(RemoteSource->SourceRootUrl.StartOfString, (RemoteSource->SourceRootUrl.LengthInChars + 1) * sizeof(TCHAR));
    if (YoriLibIsPathUrl(&RemoteSource->SourceRootUrl)) {
        RemoteSource->SourcePkgList.LengthInChars = YoriLibSPrintf(RemoteSource->SourcePkgList.StartOfString, _T("%y/pkglist.ini"), &RemoteSource->SourceRootUrl);
    } else {
        RemoteSource->SourcePkgList.LengthInChars = YoriLibSPrintf(RemoteSource->SourcePkgList.StartOfString, _T("%y\\pkglist.ini"), &RemoteSource->SourceRootUrl);
    }
    RemoteSource->SourcePkgList.LengthAllocated = RemoteSource->SourcePkgList.LengthInChars + 1;

    return RemoteSource;
}

/**
 Frees a remote source object previously allocated with
 @ref YoriPkgAllocateRemoteSource .

 @param Source Pointer to the remote source object to free.
 */
VOID
YoriPkgFreeRemoteSource(
    __in PYORIPKG_REMOTE_SOURCE Source
    )
{
    YoriLibFreeStringContents(&Source->SourcePkgList);
    YoriLibFreeStringContents(&Source->SourceRootUrl);
    YoriLibDereference(Source);
}

/**
 Allocate and populate a remote package object.

 @param PackageName The name of the remote package.
 
 @param Version The version of the remote package.

 @param Architecture The architecture of the remote package.

 @param SourceRootUrl The root URL of the source repository.

 @param RelativePackageUrl The path to the package relative to the
        SourceRootUrl.

 @return Pointer to the package object.  This can be freed with
         @ref YoriPkgFreeRemotePackage .  This will be NULL on allocation failure.

 */
PYORIPKG_REMOTE_PACKAGE
YoriPkgAllocateRemotePackage(
    __in PYORI_STRING PackageName,
    __in PYORI_STRING Version,
    __in PYORI_STRING Architecture,
    __in PYORI_STRING SourceRootUrl,
    __in PYORI_STRING RelativePackageUrl
    )
{
    PYORIPKG_REMOTE_PACKAGE Package;
    LPTSTR WritePtr;
    DWORD SizeToAllocate;

    SizeToAllocate = sizeof(YORIPKG_REMOTE_PACKAGE) + (PackageName->LengthInChars + 1 + Version->LengthInChars + 1 + Architecture->LengthInChars + 1 + SourceRootUrl->LengthInChars + 1 + RelativePackageUrl->LengthInChars + 1) * sizeof(TCHAR);
    Package = YoriLibReferencedMalloc(SizeToAllocate);

    if (Package == NULL) {
        return NULL;
    }
    ZeroMemory(Package, SizeToAllocate);

    WritePtr = (LPTSTR)(Package + 1);

    YoriLibReference(Package);
    Package->PackageName.MemoryToFree = Package;
    Package->PackageName.StartOfString = WritePtr;
    Package->PackageName.LengthInChars = PackageName->LengthInChars;
    memcpy(Package->PackageName.StartOfString, PackageName->StartOfString, PackageName->LengthInChars * sizeof(TCHAR));
    Package->PackageName.StartOfString[PackageName->LengthInChars] = '\0';
    Package->PackageName.LengthAllocated = Package->PackageName.LengthInChars + 1;

    WritePtr += Package->PackageName.LengthAllocated;

    YoriLibReference(Package);
    Package->Version.MemoryToFree = Package;
    Package->Version.StartOfString = WritePtr;
    Package->Version.LengthInChars = Version->LengthInChars;
    memcpy(Package->Version.StartOfString, Version->StartOfString, Version->LengthInChars * sizeof(TCHAR));
    Package->Version.StartOfString[Version->LengthInChars] = '\0';
    Package->Version.LengthAllocated = Package->Version.LengthInChars + 1;

    WritePtr += Package->Version.LengthAllocated;

    YoriLibReference(Package);
    Package->Architecture.MemoryToFree = Package;
    Package->Architecture.StartOfString = WritePtr;
    Package->Architecture.LengthInChars = Architecture->LengthInChars;
    memcpy(Package->Architecture.StartOfString, Architecture->StartOfString, Architecture->LengthInChars * sizeof(TCHAR));
    Package->Architecture.StartOfString[Architecture->LengthInChars] = '\0';
    Package->Architecture.LengthAllocated = Package->Architecture.LengthInChars + 1;

    WritePtr += Package->Architecture.LengthAllocated;

    YoriLibReference(Package);
    Package->InstallUrl.MemoryToFree = Package;
    Package->InstallUrl.StartOfString = WritePtr;
    if (YoriLibIsPathUrl(SourceRootUrl)) {
        Package->InstallUrl.LengthInChars = YoriLibSPrintf(Package->InstallUrl.StartOfString, _T("%y/%y"), SourceRootUrl, RelativePackageUrl);
    } else {
        Package->InstallUrl.LengthInChars = YoriLibSPrintf(Package->InstallUrl.StartOfString, _T("%y\\%y"), SourceRootUrl, RelativePackageUrl);
    }

    Package->InstallUrl.LengthAllocated = Package->InstallUrl.LengthInChars + 1;

    return Package;
}

/**
 Frees a remote package object previously allocated with
 @ref YoriPkgAllocateRemotePackage .

 @param Package Pointer to the remote package object to free.
 */
VOID
YoriPkgFreeRemotePackage(
    __in PYORIPKG_REMOTE_PACKAGE Package
    )
{
    YoriLibFreeStringContents(&Package->PackageName);
    YoriLibFreeStringContents(&Package->Version);
    YoriLibFreeStringContents(&Package->Architecture);
    YoriLibFreeStringContents(&Package->InstallUrl);
    YoriLibDereference(Package);
}


/**
 Collect the set of remote sources from an INI file.  This might be the system
 local packages.ini file or it might be a pkglist.ini file on a remote source
 (ie., remote sources can refer to other remote sources.)

 @param IniPath Pointer to the local copy of the INI file.

 @param SourcesList Pointer to the list of sources which can be updated with
        newly found sources.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgCollectSourcesFromIni(
    __in PYORI_STRING IniPath,
    __inout PYORI_LIST_ENTRY SourcesList
    )
{
    YORI_STRING IniValue;
    YORI_STRING IniKey;
    DWORD Index;
    BOOL Result = FALSE;
    PYORIPKG_REMOTE_SOURCE Source;
    PYORIPKG_REMOTE_SOURCE ExistingSource;
    PYORI_LIST_ENTRY ListEntry;
    BOOL DuplicateFound;

    YoriLibInitEmptyString(&IniValue);
    YoriLibInitEmptyString(&IniKey);

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&IniKey, YORIPKG_MAX_FIELD_LENGTH)) {
        goto Exit;
    }

    Index = 1;
    while (TRUE) {
        IniKey.LengthInChars = YoriLibSPrintf(IniKey.StartOfString, _T("Source%i"), Index);
        IniValue.LengthInChars = GetPrivateProfileString(_T("Sources"), IniKey.StartOfString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, IniPath->StartOfString);
        if (IniValue.LengthInChars == 0) {
            break;
        }

        Source = YoriPkgAllocateRemoteSource(&IniValue);
        if (Source == NULL) {
            goto Exit;
        }

        DuplicateFound = FALSE;

        ListEntry = YoriLibGetNextListEntry(SourcesList, NULL);
        while (ListEntry != NULL) {
            ExistingSource = CONTAINING_RECORD(ListEntry, YORIPKG_REMOTE_SOURCE, SourceList);
            if (YoriLibCompareStringInsensitive(&ExistingSource->SourceRootUrl, &Source->SourceRootUrl) == 0) {
                DuplicateFound = TRUE;
                break;
            }
            ListEntry = YoriLibGetNextListEntry(SourcesList, ListEntry);
        }

        if (DuplicateFound) {
            YoriPkgFreeRemoteSource(Source);
        } else {
            YoriLibAppendList(SourcesList, &Source->SourceList);
        }

        Index++;
    }

    Result = TRUE;
Exit:
    YoriLibFreeStringContents(&IniValue);
    YoriLibFreeStringContents(&IniKey);
    return Result;
}


/**
 Scan a repository of packages and collect all packages it contains into a
 caller provided list.

 @param Source Pointer to the source of the repository.

 @param PackagesIni Pointer to a string containing a path to the package INI
        file.

 @param PackageList Pointer to a list to update with any new packages found.

 @param SourcesList Pointer to a list of sources to update with any new
        sources to check.

 @return ERROR_SUCCESS to indicate packages were collected from source,
         or a Win32 error code indicating the reason for any failure.
 */
DWORD
YoriPkgCollectPackagesFromSource(
    __in PYORIPKG_REMOTE_SOURCE Source,
    __in PYORI_STRING PackagesIni,
    __inout PYORI_LIST_ENTRY PackageList,
    __inout PYORI_LIST_ENTRY SourcesList
    )
{
    YORI_STRING LocalPath;
    YORI_STRING ProvidesSection;
    YORI_STRING PkgNameOnly;
    YORI_STRING PkgVersion;
    YORI_STRING IniValue;
    YORI_STRING Architecture;
    BOOL DeleteWhenFinished = FALSE;
    LPTSTR ThisLine;
    LPTSTR Equals;
    LPTSTR KnownArchitectures[] = {_T("noarch"), _T("win32"), _T("amd64")};
    DWORD ArchIndex;
    DWORD Result;

    YoriLibInitEmptyString(&LocalPath);
    YoriLibInitEmptyString(&ProvidesSection);
    YoriLibInitEmptyString(&IniValue);
    YoriLibInitEmptyString(&PkgVersion);

    Result = YoriPkgPackagePathToLocalPath(&Source->SourcePkgList, PackagesIni, &LocalPath, &DeleteWhenFinished);
    if (Result != ERROR_SUCCESS) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&ProvidesSection, YORIPKG_MAX_SECTION_LENGTH)) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    if (!YoriLibAllocateString(&PkgVersion, YORIPKG_MAX_FIELD_LENGTH)) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    ProvidesSection.LengthInChars = GetPrivateProfileSection(_T("Provides"), ProvidesSection.StartOfString, ProvidesSection.LengthAllocated, LocalPath.StartOfString);

    YoriLibInitEmptyString(&PkgNameOnly);
    ThisLine = ProvidesSection.StartOfString;

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

        PkgVersion.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, _T("Version"), _T(""), PkgVersion.StartOfString, PkgVersion.LengthAllocated, LocalPath.StartOfString);

        if (PkgVersion.LengthInChars > 0) {
            for (ArchIndex = 0; ArchIndex < sizeof(KnownArchitectures)/sizeof(KnownArchitectures[0]); ArchIndex++) {
                YoriLibConstantString(&Architecture, KnownArchitectures[ArchIndex]);
                IniValue.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, Architecture.StartOfString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, LocalPath.StartOfString);
                if (IniValue.LengthInChars > 0) {
                    PYORIPKG_REMOTE_PACKAGE Package;
                    Package = YoriPkgAllocateRemotePackage(&PkgNameOnly, &PkgVersion, &Architecture, &Source->SourceRootUrl, &IniValue);
                    if (Package != NULL) {
                        YoriLibAppendList(PackageList, &Package->PackageList);
                    }
                }
            }
        }
    }

    if (!YoriPkgCollectSourcesFromIni(&LocalPath, SourcesList)) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

Exit:
    if (DeleteWhenFinished) {
        DeleteFile(LocalPath.StartOfString);
    }
    YoriLibFreeStringContents(&LocalPath);
    YoriLibFreeStringContents(&ProvidesSection);
    YoriLibFreeStringContents(&IniValue);
    YoriLibFreeStringContents(&PkgVersion);
    return Result;
}

/**
 Examine the currently configured set of sources, query each of those
 including any sources they refer to, and build a complete list of packages
 found from all sources.

 @param NewDirectory Optionally specifies an install directory.  If not
        specified, the directory of the currently running application is
        used.

 @param SourcesList On successful completion, populated with a list of sources
        that were referenced.

 @param PackageList On successful completion, populated with a list of
        packages that were found.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
BOOL
YoriPkgCollectAllSourcesAndPackages(
    __in_opt PYORI_STRING NewDirectory,
    __out PYORI_LIST_ENTRY SourcesList,
    __out PYORI_LIST_ENTRY PackageList
    )
{
    YORI_STRING PackagesIni;
    PYORI_LIST_ENTRY SourceEntry;
    PYORIPKG_REMOTE_SOURCE Source;
    DWORD Result;

    YoriLibInitializeListHead(PackageList);
    YoriLibInitializeListHead(SourcesList);

    if (!YoriPkgGetPackageIniFile(NewDirectory, &PackagesIni)) {
        return FALSE;
    }

    YoriPkgCollectSourcesFromIni(&PackagesIni, SourcesList);
    YoriLibFreeStringContents(&PackagesIni);

    //
    //  If the INI file provides no place to search, default to malsmith.net
    //

    if (YoriLibIsListEmpty(SourcesList)) {
        YORI_STRING DummySource;
#if YORI_BUILD_ID
        YoriLibConstantString(&DummySource, _T("http://www.malsmith.net/testing"));
        Source = YoriPkgAllocateRemoteSource(&DummySource);
        if (Source != NULL) {
            YoriLibAppendList(SourcesList, &Source->SourceList);
        }
#endif
        YoriLibConstantString(&DummySource, _T("http://www.malsmith.net"));
        Source = YoriPkgAllocateRemoteSource(&DummySource);
        if (Source != NULL) {
            YoriLibAppendList(SourcesList, &Source->SourceList);
        }
    }

    //
    //  Go through all known sources collecting packages and additional
    //  sources.
    //

    SourceEntry = NULL;
    SourceEntry = YoriLibGetNextListEntry(SourcesList, SourceEntry);
    while (SourceEntry != NULL) {
        Source = CONTAINING_RECORD(SourceEntry, YORIPKG_REMOTE_SOURCE, SourceList);
        Result = YoriPkgCollectPackagesFromSource(Source, &PackagesIni, PackageList, SourcesList);
        if (Result != ERROR_SUCCESS) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Error obtaining package list from %y: "), &Source->SourceRootUrl);
            YoriPkgDisplayErrorStringForInstallFailure(Result);
        }
        SourceEntry = YoriLibGetNextListEntry(SourcesList, SourceEntry);
    }

    return TRUE;
}

/**
 Free a list of packages and/or sources.

 @param SourcesList The list of sources to free.

 @param PackageList The list of packages to free.
 */
VOID
YoriPkgFreeAllSourcesAndPackages(
    __in_opt PYORI_LIST_ENTRY SourcesList,
    __in_opt PYORI_LIST_ENTRY PackageList
    )
{
    PYORI_LIST_ENTRY SourceEntry;
    PYORIPKG_REMOTE_SOURCE Source;
    PYORI_LIST_ENTRY PackageEntry;
    PYORIPKG_REMOTE_PACKAGE Package;

    if (PackageList != NULL) {
        PackageEntry = NULL;
        PackageEntry = YoriLibGetNextListEntry(PackageList, PackageEntry);
        while (PackageEntry != NULL) {
            Package = CONTAINING_RECORD(PackageEntry, YORIPKG_REMOTE_PACKAGE, PackageList);
            PackageEntry = YoriLibGetNextListEntry(PackageList, PackageEntry);
            YoriLibRemoveListItem(&Package->PackageList);
            YoriPkgFreeRemotePackage(Package);
        }
    }

    //
    //  Free the sources.
    //

    if (SourcesList != NULL) {
        SourceEntry = NULL;
        SourceEntry = YoriLibGetNextListEntry(SourcesList, SourceEntry);
        while (SourceEntry != NULL) {
            Source = CONTAINING_RECORD(SourceEntry, YORIPKG_REMOTE_SOURCE, SourceList);
            SourceEntry = YoriLibGetNextListEntry(SourcesList, SourceEntry);
            YoriLibRemoveListItem(&Source->SourceList);
            YoriPkgFreeRemoteSource(Source);
        }
    }
}

/**
 Query all of the known sources for available packages and display them on
 the console.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgDisplayAvailableRemotePackages()
{
    YORI_LIST_ENTRY SourcesList;
    YORI_LIST_ENTRY PackageList;
    PYORI_LIST_ENTRY PackageEntry;
    PYORIPKG_REMOTE_PACKAGE Package;

    YoriPkgCollectAllSourcesAndPackages(NULL, &SourcesList, &PackageList);

    //
    //  Display the packages we found.
    //

    PackageEntry = NULL;
    PackageEntry = YoriLibGetNextListEntry(&PackageList, PackageEntry);
    while (PackageEntry != NULL) {
        Package = CONTAINING_RECORD(PackageEntry, YORIPKG_REMOTE_PACKAGE, PackageList);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y %y %y %y\n"), &Package->PackageName, &Package->Version, &Package->Architecture, &Package->InstallUrl);
        PackageEntry = YoriLibGetNextListEntry(&PackageList, PackageEntry);
    }

    YoriPkgFreeAllSourcesAndPackages(&SourcesList, &PackageList);

    return TRUE;
}

/**
 Process a list of packages which match a given package name and desired
 version, find any with the matching architecture and if it is found, insert
 it into the list of matches.

 @param PackageList The list of packages to walk through.

 @param Architecture The architecture to find.

 @param MatchingPackages On successful completion, updated to include the
        URL for the architecture of the package.

 @return TRUE to indicate a package matching the requested architecture
         was found.
 */
BOOL
YoriPkgFindRemotePackageMatchingArchitecture(
    __in PYORI_LIST_ENTRY PackageList,
    __in PYORI_STRING Architecture,
    __inout PYORI_LIST_ENTRY MatchingPackages
    )
{
    PYORI_LIST_ENTRY PackageEntry;
    PYORIPKG_REMOTE_PACKAGE Package;

    PackageEntry = NULL;
    PackageEntry = YoriLibGetNextListEntry(PackageList, PackageEntry);
    while (PackageEntry != NULL) {
        Package = CONTAINING_RECORD(PackageEntry, YORIPKG_REMOTE_PACKAGE, PackageList);
        PackageEntry = YoriLibGetNextListEntry(PackageList, PackageEntry);
        if (YoriLibCompareStringInsensitive(Architecture, &Package->Architecture) == 0) {
            YoriLibRemoveListItem(&Package->PackageList);
            YoriLibAppendList(MatchingPackages, &Package->PackageList);
            return TRUE;
        }
    }
    return FALSE;
}

/**
 Install packages from remote source by name, optionally with version and
 architecture.

 @param PackageNames Pointer to an array of one or more package names.

 @param PackageNameCount The number of elements in the PackageName array.

 @param NewDirectory Optionally specifies an install directory.  If not
        specified, the directory of the currently running application is
        used.

 @param MatchVersion Optionally specifies a version of the packages to
        install.  If not specified, the latest version is used.

 @param MatchArch Optionally specifies the architecture of the packages to
        install.  If not specified, determined from the host operaitng
        system.

 @param PackagesMatchingCriteria A pre initialized list to update with
        matching package URLs.

 @return The number of matching packages returned.  Typically success means
         this is equal to PackageNameCount.
 */
DWORD
YoriPkgFindRemotePackages(
    __in PYORI_STRING PackageNames,
    __in DWORD PackageNameCount,
    __in_opt PYORI_STRING NewDirectory,
    __in_opt PYORI_STRING MatchVersion,
    __in_opt PYORI_STRING MatchArch,
    __inout PYORI_LIST_ENTRY PackagesMatchingCriteria
    )
{
    YORI_LIST_ENTRY SourcesList;
    YORI_LIST_ENTRY PackageList;
    YORI_LIST_ENTRY PackagesMatchingName;
    YORI_LIST_ENTRY PackagesMatchingVersion;
    PYORI_LIST_ENTRY PackageEntry;
    PYORIPKG_REMOTE_PACKAGE Package;
    DWORD PkgIndex;
    PYORI_STRING LookingForVersion;
    DWORD InstallCount = 0;
    BOOL PkgInstalled;

    YoriPkgCollectAllSourcesAndPackages(NewDirectory, &SourcesList, &PackageList);

    for (PkgIndex = 0; PkgIndex < PackageNameCount; PkgIndex++) {
        YoriLibInitializeListHead(&PackagesMatchingName);
        YoriLibInitializeListHead(&PackagesMatchingVersion);

        //
        //  Find all packages matching the specified name.
        //

        PackageEntry = NULL;
        PackageEntry = YoriLibGetNextListEntry(&PackageList, PackageEntry);
        while (PackageEntry != NULL) {
            Package = CONTAINING_RECORD(PackageEntry, YORIPKG_REMOTE_PACKAGE, PackageList);
            PackageEntry = YoriLibGetNextListEntry(&PackageList, PackageEntry);
            if (YoriLibCompareStringInsensitive(&PackageNames[PkgIndex], &Package->PackageName) == 0) {
                YoriLibRemoveListItem(&Package->PackageList);
                YoriLibAppendList(&PackagesMatchingName, &Package->PackageList);
            }
        }

        //
        //  If a version wasn't specified, find the highest match
        //

        LookingForVersion = NULL;
        if (MatchVersion != NULL) {
            LookingForVersion = MatchVersion;
        } else {
            PackageEntry = NULL;
            PackageEntry = YoriLibGetNextListEntry(&PackagesMatchingName, PackageEntry);
            while (PackageEntry != NULL) {
                Package = CONTAINING_RECORD(PackageEntry, YORIPKG_REMOTE_PACKAGE, PackageList);
                PackageEntry = YoriLibGetNextListEntry(&PackagesMatchingName, PackageEntry);
                if (LookingForVersion == NULL ||
                    YoriLibCompareStringInsensitive(&Package->Version, LookingForVersion) > 0) {

                    LookingForVersion = &Package->Version;
                }
            }
        }

        //
        //  If we couldn't find any version, we don't have the package.
        //

        if (LookingForVersion == NULL) {
            YoriPkgFreeAllSourcesAndPackages(NULL, &PackagesMatchingName);
            continue;
        }

        //
        //  Scan through the name matches and migrate the version matches.
        //  Free everything else.
        //

        PackageEntry = NULL;
        PackageEntry = YoriLibGetNextListEntry(&PackagesMatchingName, PackageEntry);
        while (PackageEntry != NULL) {
            Package = CONTAINING_RECORD(PackageEntry, YORIPKG_REMOTE_PACKAGE, PackageList);
            PackageEntry = YoriLibGetNextListEntry(&PackagesMatchingName, PackageEntry);
            if (YoriLibCompareStringInsensitive(LookingForVersion, &Package->Version) == 0) {
                YoriLibRemoveListItem(&Package->PackageList);
                YoriLibAppendList(&PackagesMatchingVersion, &Package->PackageList);
            }
        }

        YoriPkgFreeAllSourcesAndPackages(NULL, &PackagesMatchingName);

        //
        //  If the user requested an arch, go look if we found it.  If not,
        //  try to determine the "best" arch from what we've found.
        //

        PkgInstalled = FALSE;
        if (MatchArch != NULL) {
            if (YoriPkgFindRemotePackageMatchingArchitecture(&PackagesMatchingVersion, MatchArch, PackagesMatchingCriteria)) {
                InstallCount++;
            }
        } else {
            BOOL WantAmd64;
            YORI_STRING YsArch;
#ifdef _WIN64
            WantAmd64 = TRUE;
#else
            WantAmd64 = FALSE;
            if (DllKernel32.pIsWow64Process) {
                DllKernel32.pIsWow64Process(GetCurrentProcess(), &WantAmd64);
            }
#endif

            YoriLibConstantString(&YsArch, _T("amd64"));
            if (WantAmd64 && YoriPkgFindRemotePackageMatchingArchitecture(&PackagesMatchingVersion, &YsArch, PackagesMatchingCriteria)) {
                InstallCount++;
            } else {
                YoriLibConstantString(&YsArch, _T("win32"));
                if (YoriPkgFindRemotePackageMatchingArchitecture(&PackagesMatchingVersion, &YsArch, PackagesMatchingCriteria)) {
                    InstallCount++;
                } else {
                    YoriLibConstantString(&YsArch, _T("noarch"));
                    if (YoriPkgFindRemotePackageMatchingArchitecture(&PackagesMatchingVersion, &YsArch, PackagesMatchingCriteria)) {
                        InstallCount++;
                    }
                }
            }
        }

        YoriPkgFreeAllSourcesAndPackages(NULL, &PackagesMatchingVersion);
    }

    YoriPkgFreeAllSourcesAndPackages(&SourcesList, &PackageList);

    return InstallCount;
}

/**
 Install packages from remote source by name, optionally with version and
 architecture.

 @param PackageNames Pointer to an array of one or more package names.

 @param PackageNameCount The number of elements in the PackageName array.

 @param NewDirectory Optionally specifies an install directory.  If not
        specified, the directory of the currently running application is
        used.

 @param MatchVersion Optionally specifies a version of the packages to
        install.  If not specified, the latest version is used.

 @param MatchArch Optionally specifies the architecture of the packages to
        install.  If not specified, determined from the host operaitng
        system.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgInstallRemotePackages(
    __in PYORI_STRING PackageNames,
    __in DWORD PackageNameCount,
    __in_opt PYORI_STRING NewDirectory,
    __in_opt PYORI_STRING MatchVersion,
    __in_opt PYORI_STRING MatchArch
    )
{
    DWORD MatchingPackageCount;
    DWORD AttemptedCount;
    DWORD InstallCount = 0;
    YORI_LIST_ENTRY PackagesMatchingCriteria;
    PYORI_LIST_ENTRY PackageEntry;
    PYORIPKG_REMOTE_PACKAGE Package;
    YORI_STRING IniFile;
    YORI_STRING IniValue;
    YORIPKG_PACKAGES_PENDING_INSTALL PendingPackages;
    DWORD Error;

    if (!YoriPkgInitializePendingPackages(&PendingPackages)) {
        return FALSE;
    }

    if (!YoriPkgGetPackageIniFile(NewDirectory, &IniFile)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        return 0;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&IniFile);
        return 0;
    }

    YoriLibInitializeListHead(&PackagesMatchingCriteria);

    //
    //  Find the number of packages that can be resolved from remote
    //  sources.
    //

    MatchingPackageCount = YoriPkgFindRemotePackages(PackageNames,
                                                     PackageNameCount,
                                                     NewDirectory,
                                                     MatchVersion,
                                                     MatchArch,
                                                     &PackagesMatchingCriteria);

    //
    //  Find if any of these are installed and back them up.
    //

    AttemptedCount = 0;
    PackageEntry = NULL;
    PackageEntry = YoriLibGetNextListEntry(&PackagesMatchingCriteria, PackageEntry);
    while (PackageEntry != NULL) {
        Package = CONTAINING_RECORD(PackageEntry, YORIPKG_REMOTE_PACKAGE, PackageList);
        PackageEntry = YoriLibGetNextListEntry(&PackagesMatchingCriteria, PackageEntry);

        if (YoriLibIsPathUrl(&Package->InstallUrl)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Downloading %y...\n"), &Package->InstallUrl);
        }
        Error = YoriPkgPreparePackageForInstall(&IniFile, NewDirectory, &PendingPackages, &Package->InstallUrl);
        if (Error != ERROR_SUCCESS) {
            YoriPkgDisplayErrorStringForInstallFailure(Error);
            goto Exit;
        }

        AttemptedCount++;
    }

    if (YoriPkgInstallPendingPackages(&IniFile, NewDirectory, &PendingPackages)) {
        InstallCount = AttemptedCount;
    }

Exit:

    //
    //  Abort anything that wasn't committed.  This means if we took a backup
    //  attempt to uninstall any new version of the package.  This may or may
    //  not exist.
    //

    if (!YoriLibIsListEmpty(&PendingPackages.BackupPackages)) {
        YoriPkgRollbackAndFreeBackupPackageList(&IniFile, NewDirectory, &PendingPackages.BackupPackages);
    }

    YoriPkgDeletePendingPackages(&PendingPackages);

    YoriPkgFreeAllSourcesAndPackages(NULL, &PackagesMatchingCriteria);
    YoriLibFreeStringContents(&IniFile);
    YoriLibFreeStringContents(&IniValue);

    return InstallCount;
}

/**
 Return the remote package URLs for a specified set of packages.

 @param PackageNames Pointer to an array of one or more package names.

 @param PackageNameCount The number of elements in the PackageName array.

 @param NewDirectory Optionally specifies an install directory.  If not
        specified, the directory of the currently running application is
        used.

 @param PackageUrls On successful completion, updated to point to an array of
        YORI_STRINGs containing URLs for the packages.

 @return The number of package URLs returned.  Typically success means this
         is equal to PackageNameCount.
 */
DWORD
YoriPkgGetRemotePackageUrls(
    __in PYORI_STRING PackageNames,
    __in DWORD PackageNameCount,
    __in_opt PYORI_STRING NewDirectory,
    __out PYORI_STRING * PackageUrls
    )
{
    DWORD MatchingPackageCount;
    YORI_LIST_ENTRY PackagesMatchingCriteria;
    PYORI_LIST_ENTRY PackageEntry;
    PYORIPKG_REMOTE_PACKAGE Package;
    DWORD CharsNeeded;
    PYORI_STRING LocalPackageUrls;
    LPTSTR WritePtr;
    DWORD PkgIndex;

    YoriLibInitializeListHead(&PackagesMatchingCriteria);

    MatchingPackageCount = YoriPkgFindRemotePackages(PackageNames,
                                                     PackageNameCount,
                                                     NewDirectory,
                                                     NULL,
                                                     NULL,
                                                     &PackagesMatchingCriteria);

    CharsNeeded = 0;
    PackageEntry = NULL;
    PackageEntry = YoriLibGetNextListEntry(&PackagesMatchingCriteria, PackageEntry);
    while (PackageEntry != NULL) {
        Package = CONTAINING_RECORD(PackageEntry, YORIPKG_REMOTE_PACKAGE, PackageList);
        PackageEntry = YoriLibGetNextListEntry(&PackagesMatchingCriteria, PackageEntry);
        CharsNeeded += Package->InstallUrl.LengthInChars + 1;
    }

    LocalPackageUrls = YoriLibReferencedMalloc(CharsNeeded * sizeof(TCHAR) + MatchingPackageCount * sizeof(YORI_STRING));
    if (LocalPackageUrls == NULL) {
        YoriPkgFreeAllSourcesAndPackages(NULL, &PackagesMatchingCriteria);
        return 0;
    }

    WritePtr = (LPTSTR)(LocalPackageUrls + MatchingPackageCount);

    PkgIndex = 0;
    PackageEntry = NULL;
    PackageEntry = YoriLibGetNextListEntry(&PackagesMatchingCriteria, PackageEntry);
    while (PackageEntry != NULL) {
        Package = CONTAINING_RECORD(PackageEntry, YORIPKG_REMOTE_PACKAGE, PackageList);
        PackageEntry = YoriLibGetNextListEntry(&PackagesMatchingCriteria, PackageEntry);
        YoriLibInitEmptyString(&LocalPackageUrls[PkgIndex]);

        YoriLibReference(LocalPackageUrls);
        LocalPackageUrls[PkgIndex].MemoryToFree = LocalPackageUrls;
        LocalPackageUrls[PkgIndex].StartOfString = WritePtr;
        LocalPackageUrls[PkgIndex].LengthInChars = Package->InstallUrl.LengthInChars;
        LocalPackageUrls[PkgIndex].LengthAllocated = LocalPackageUrls[PkgIndex].LengthInChars + 1;
        memcpy(WritePtr, Package->InstallUrl.StartOfString, Package->InstallUrl.LengthInChars * sizeof(TCHAR));
        WritePtr[Package->InstallUrl.LengthInChars] = '\0';
        WritePtr += Package->InstallUrl.LengthAllocated;
        PkgIndex++;
    }

    ASSERT(PkgIndex == MatchingPackageCount);
    YoriPkgFreeAllSourcesAndPackages(NULL, &PackagesMatchingCriteria);
    *PackageUrls = LocalPackageUrls;

    return PkgIndex;
}

// vim:sw=4:ts=4:et:
