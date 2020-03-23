/**
 * @file pkglib/remote.c
 *
 * Yori package manager remote source query and search
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
     A string indicating the minimum build of Windows needed to run the
     package.  Note that this may be an empty string.
     */
    YORI_STRING MinimumOSBuild;

    /**
     A string for the path for a version of the package suitable for systems
     older than MinimumOSBuild.  Note that this may recurse multiple times.
     This is meaningless unless MinimumOSBuild is specified.
     */
    YORI_STRING PackagePathForOlderBuilds;

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

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6260) // sizeof*sizeof is used for character size
                                // flexibility
#endif
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

 @param MinimumOSBuild The minimum build number of Windows needed to install
        this package.  Note that this can be an empty string.

 @param PackagePathForOlderBuilds The URL to an alternate package to use if
        the host version of Windows does not meet the minimum build
        requirement.

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
    __in PYORI_STRING MinimumOSBuild,
    __in PYORI_STRING PackagePathForOlderBuilds,
    __in PYORI_STRING SourceRootUrl,
    __in PYORI_STRING RelativePackageUrl
    )
{
    PYORIPKG_REMOTE_PACKAGE Package;
    LPTSTR WritePtr;
    DWORD SizeToAllocate;

    SizeToAllocate = sizeof(YORIPKG_REMOTE_PACKAGE) +
                     (PackageName->LengthInChars + 1 +
                      Version->LengthInChars + 1 +
                      Architecture->LengthInChars + 1 +
                      MinimumOSBuild->LengthInChars + 1 +
                      PackagePathForOlderBuilds->LengthInChars + 1 +
                      SourceRootUrl->LengthInChars + 1 +
                      RelativePackageUrl->LengthInChars + 1) * sizeof(TCHAR);
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
    Package->MinimumOSBuild.MemoryToFree = Package;
    Package->MinimumOSBuild.StartOfString = WritePtr;
    Package->MinimumOSBuild.LengthInChars = MinimumOSBuild->LengthInChars;
    memcpy(Package->MinimumOSBuild.StartOfString, MinimumOSBuild->StartOfString, MinimumOSBuild->LengthInChars * sizeof(TCHAR));
    Package->MinimumOSBuild.StartOfString[MinimumOSBuild->LengthInChars] = '\0';
    Package->MinimumOSBuild.LengthAllocated = Package->MinimumOSBuild.LengthInChars + 1;

    WritePtr += Package->MinimumOSBuild.LengthAllocated;

    YoriLibReference(Package);
    Package->PackagePathForOlderBuilds.MemoryToFree = Package;
    Package->PackagePathForOlderBuilds.StartOfString = WritePtr;
    Package->PackagePathForOlderBuilds.LengthInChars = PackagePathForOlderBuilds->LengthInChars;
    memcpy(Package->PackagePathForOlderBuilds.StartOfString, PackagePathForOlderBuilds->StartOfString, PackagePathForOlderBuilds->LengthInChars * sizeof(TCHAR));
    Package->PackagePathForOlderBuilds.StartOfString[PackagePathForOlderBuilds->LengthInChars] = '\0';
    Package->PackagePathForOlderBuilds.LengthAllocated = Package->PackagePathForOlderBuilds.LengthInChars + 1;

    WritePtr += Package->PackagePathForOlderBuilds.LengthAllocated;

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
    YoriLibFreeStringContents(&Package->MinimumOSBuild);
    YoriLibFreeStringContents(&Package->PackagePathForOlderBuilds);
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
    __inout_opt PYORI_LIST_ENTRY SourcesList
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

    if (SourcesList != NULL) {
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
    }

    Result = TRUE;
Exit:
    YoriLibFreeStringContents(&IniValue);
    YoriLibFreeStringContents(&IniKey);
    return Result;
}

/**
 Collect the set of remote sources from the system local INI file, and if no
 sources are present, initialize some default sources.

 @param PackagesIni Pointer to the local copy of the INI file.

 @param SourcesList Pointer to the list of sources which can be updated with
        newly found sources.

 @param DefaultsUsed Optionally points to a boolean which is set to TRUE to
        indicate that default sources were used because the INI file contained
        no entries, or FALSE to indicate the INI's sources were used.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgCollectSourcesFromIniWithDefaults(
    __in PYORI_STRING PackagesIni,
    __inout PYORI_LIST_ENTRY SourcesList,
    __out_opt PBOOLEAN DefaultsUsed
    )
{
    PYORIPKG_REMOTE_SOURCE Source;
    YoriPkgCollectSourcesFromIni(PackagesIni, SourcesList);

    //
    //  If the INI file provides no place to search, default to malsmith.net
    //

    if (YoriLibIsListEmpty(SourcesList)) {
        YORI_STRING DummySource;
        if (DefaultsUsed != NULL) {
            *DefaultsUsed = TRUE;
        }
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
    } else if (DefaultsUsed != NULL) {
        *DefaultsUsed = FALSE;
    }

    return TRUE;
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
    __inout_opt PYORI_LIST_ENTRY SourcesList
    )
{
    YORI_STRING LocalPath;
    YORI_STRING ProvidesSection;
    YORI_STRING PkgNameOnly;
    YORI_STRING PkgVersion;
    YORI_STRING IniValue;
    YORI_STRING Architecture;
    YORI_STRING MinimumOSBuild;
    YORI_STRING PackagePathForOlderBuilds;
    BOOL DeleteWhenFinished = FALSE;
    LPTSTR ThisLine;
    LPTSTR Equals;
    LPTSTR KnownArchitectures[] = {_T("noarch"), _T("win32"), _T("amd64")};

    // Worst case architecture and worst case key
    TCHAR IniKey[sizeof("noarch.packagepathforolderbuilds")];
    DWORD ArchIndex;
    DWORD Result;

    YoriLibInitEmptyString(&LocalPath);
    YoriLibInitEmptyString(&ProvidesSection);
    YoriLibInitEmptyString(&IniValue);
    YoriLibInitEmptyString(&PkgVersion);
    YoriLibInitEmptyString(&MinimumOSBuild);
    YoriLibInitEmptyString(&PackagePathForOlderBuilds);

    Result = YoriPkgPackagePathToLocalPath(&Source->SourcePkgList, PackagesIni, &LocalPath, &DeleteWhenFinished);
    if (Result != ERROR_SUCCESS) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&ProvidesSection, YORIPKG_MAX_SECTION_LENGTH * 5)) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    ProvidesSection.LengthAllocated = YORIPKG_MAX_SECTION_LENGTH;

    YoriLibCloneString(&PkgVersion, &ProvidesSection);
    PkgVersion.StartOfString += YORIPKG_MAX_SECTION_LENGTH;

    YoriLibCloneString(&IniValue, &PkgVersion);
    IniValue.StartOfString += YORIPKG_MAX_SECTION_LENGTH;

    YoriLibCloneString(&MinimumOSBuild, &IniValue);
    MinimumOSBuild.StartOfString += YORIPKG_MAX_SECTION_LENGTH;

    YoriLibCloneString(&PackagePathForOlderBuilds, &MinimumOSBuild);
    PackagePathForOlderBuilds.StartOfString += YORIPKG_MAX_SECTION_LENGTH;

    ProvidesSection.LengthInChars = GetPrivateProfileSection(_T("Provides"),
                                                             ProvidesSection.StartOfString,
                                                             ProvidesSection.LengthAllocated,
                                                             LocalPath.StartOfString);

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

        PkgVersion.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString,
                                                           _T("Version"),
                                                           _T(""),
                                                           PkgVersion.StartOfString,
                                                           PkgVersion.LengthAllocated,
                                                           LocalPath.StartOfString);

        if (PkgVersion.LengthInChars > 0) {
            for (ArchIndex = 0; ArchIndex < sizeof(KnownArchitectures)/sizeof(KnownArchitectures[0]); ArchIndex++) {
                YoriLibConstantString(&Architecture, KnownArchitectures[ArchIndex]);
                IniValue.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString,
                                                                 Architecture.StartOfString,
                                                                 _T(""),
                                                                 IniValue.StartOfString,
                                                                 IniValue.LengthAllocated,
                                                                 LocalPath.StartOfString);
                if (IniValue.LengthInChars > 0) {
                    PYORIPKG_REMOTE_PACKAGE Package;

                    PackagePathForOlderBuilds.LengthInChars = 0;

                    YoriLibSPrintf(IniKey, _T("%y.minimumosbuild"), &Architecture);

                    MinimumOSBuild.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, IniKey, _T(""), MinimumOSBuild.StartOfString, MinimumOSBuild.LengthAllocated, LocalPath.StartOfString);
                    if (MinimumOSBuild.LengthInChars > 0) {
                        YoriLibSPrintf(IniKey, _T("%y.packagepathforolderbuilds"), &Architecture);
                        PackagePathForOlderBuilds.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, IniKey, _T(""), PackagePathForOlderBuilds.StartOfString, PackagePathForOlderBuilds.LengthAllocated, LocalPath.StartOfString);
                    }



                    Package = YoriPkgAllocateRemotePackage(&PkgNameOnly,
                                                           &PkgVersion,
                                                           &Architecture,
                                                           &MinimumOSBuild,
                                                           &PackagePathForOlderBuilds,
                                                           &Source->SourceRootUrl,
                                                           &IniValue);
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
    YoriLibFreeStringContents(&MinimumOSBuild);
    YoriLibFreeStringContents(&PackagePathForOlderBuilds);
    return Result;
}

/**
 Examine the currently configured set of sources, query each of those
 including any sources they refer to, and build a complete list of packages
 found from all sources.

 @param CustomSource Optionally specifies a source for packages.  If not
        specified, the INI from NewDirectory is used, and if that's not
        present, fallback to default sources.

 @param NewDirectory Optionally specifies an install directory.  If not
        specified, the directory of the currently running application is
        used.

 @param SourcesList On successful completion, populated with a list of sources
        that were referenced.

 @param PackageList On successful completion, populated with a list of
        packages that were found.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriPkgCollectAllSourcesAndPackages(
    __in_opt PYORI_STRING CustomSource,
    __in_opt PYORI_STRING NewDirectory,
    __out PYORI_LIST_ENTRY SourcesList,
    __out PYORI_LIST_ENTRY PackageList
    )
{
    PYORI_LIST_ENTRY SourceEntry;
    PYORIPKG_REMOTE_SOURCE Source;
    DWORD Result;
    YORI_STRING PackagesIni;

    if (!YoriPkgGetPackageIniFile(NewDirectory, &PackagesIni)) {
        return FALSE;
    }

    YoriLibInitializeListHead(PackageList);
    YoriLibInitializeListHead(SourcesList);

    if (CustomSource != NULL) {
        Source = YoriPkgAllocateRemoteSource(CustomSource);
        if (Source == NULL) {
            YoriLibFreeStringContents(&PackagesIni);
            return FALSE;
        }
        YoriLibAppendList(SourcesList, &Source->SourceList);
    } else {
        if (!YoriPkgCollectSourcesFromIniWithDefaults(&PackagesIni, SourcesList, NULL)) {
            YoriLibFreeStringContents(&PackagesIni);
            return FALSE;
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
    YoriLibFreeStringContents(&PackagesIni);

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

    YoriPkgCollectAllSourcesAndPackages(NULL, NULL, &SourcesList, &PackageList);

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
 Enumerate all packages on a server from its pkglist.ini, download all of the
 packages to a local directory, and generate a pkglist.ini in that directory
 from the contents.

 @param Source Pointer to a remote path from which to download packages.

 @param DownloadPath Pointer to a local path to save packages.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgDownloadRemotePackages(
    __in PYORI_STRING Source,
    __in PYORI_STRING DownloadPath
    )
{
    YORI_LIST_ENTRY SourcesList;
    YORI_LIST_ENTRY PackageList;
    PYORI_LIST_ENTRY PackageEntry;
    PYORIPKG_REMOTE_PACKAGE Package;
    YORI_STRING FinalFileName;
    YORI_STRING FullFinalName;
    YORI_STRING TempLocalPath;
    YORI_STRING PackagesIni;
    DWORD Index;
    DWORD Err;
    BOOL DeleteWhenFinished;

    YoriPkgCollectAllSourcesAndPackages(Source, NULL, &SourcesList, &PackageList);

    YoriLibInitEmptyString(&PackagesIni);
    YoriLibYPrintf(&PackagesIni, _T("%y\\pkglist.ini"), DownloadPath);
    if (PackagesIni.StartOfString == NULL) {
        YoriPkgFreeAllSourcesAndPackages(&SourcesList, &PackageList);
        return FALSE;
    }

    //
    //  Download the packages we found.
    //

    PackageEntry = NULL;
    PackageEntry = YoriLibGetNextListEntry(&PackageList, PackageEntry);
    while (PackageEntry != NULL) {
        Package = CONTAINING_RECORD(PackageEntry, YORIPKG_REMOTE_PACKAGE, PackageList);

        //
        //  Find the final file component in the URL
        //

        YoriLibInitEmptyString(&FinalFileName);
        for (Index = Package->InstallUrl.LengthInChars; Index > 0; Index--) {
            if (YoriLibIsSep(Package->InstallUrl.StartOfString[Index - 1])) {
                FinalFileName.StartOfString = &Package->InstallUrl.StartOfString[Index];
                FinalFileName.LengthInChars = Package->InstallUrl.LengthInChars - Index;
                break;
            }
        }

        YoriLibInitEmptyString(&FullFinalName);
        if (FinalFileName.LengthInChars > 0) {

            //
            //  Download the package, build a local path with the final file
            //  component from the URL, and copy or move the package into
            //  place.
            //

            YoriLibInitEmptyString(&TempLocalPath);
            Err = YoriPkgPackagePathToLocalPath(&Package->InstallUrl, NULL, &TempLocalPath, &DeleteWhenFinished);
            if (Err == ERROR_SUCCESS) {
                YoriLibYPrintf(&FullFinalName, _T("%y\\%y"), DownloadPath, &FinalFileName);
                if (FullFinalName.LengthInChars == 0) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                }
                if (Err == ERROR_SUCCESS) {
                    if (DeleteWhenFinished) {
                        if (!MoveFileEx(TempLocalPath.StartOfString, FullFinalName.StartOfString, MOVEFILE_REPLACE_EXISTING)) {
                            Err = GetLastError();
                            DeleteFile(TempLocalPath.StartOfString);
                        }
                    } else {
                        if (!CopyFile(TempLocalPath.StartOfString, FullFinalName.StartOfString, FALSE)) {
                            Err = GetLastError();
                        }
                    }
                }

                YoriLibFreeStringContents(&TempLocalPath);
            }

            //
            //  Write INI entries for the package that has been found on the
            //  remote source.  Note all this can do is propagate the values
            //  that this version of the code understands.
            //

            if (Err == ERROR_SUCCESS) {
                YORI_STRING TempKeyString;
                WritePrivateProfileString(_T("Provides"), Package->PackageName.StartOfString, Package->Version.StartOfString, PackagesIni.StartOfString);
                WritePrivateProfileString(Package->PackageName.StartOfString, _T("Version"), Package->Version.StartOfString, PackagesIni.StartOfString);
                WritePrivateProfileString(Package->PackageName.StartOfString, Package->Architecture.StartOfString, FinalFileName.StartOfString, PackagesIni.StartOfString);

                if (Package->MinimumOSBuild.LengthInChars != 0) {
                    YoriLibInitEmptyString(&TempKeyString);
                    YoriLibYPrintf(&TempKeyString, _T("%y.minimumosbuild"), &Package->Architecture);
                    if (TempKeyString.LengthInChars > 0) {
                        WritePrivateProfileString(Package->PackageName.StartOfString, TempKeyString.StartOfString, Package->MinimumOSBuild.StartOfString, PackagesIni.StartOfString);
                        YoriLibFreeStringContents(&TempKeyString);
                    }

                }

                if (Package->PackagePathForOlderBuilds.LengthInChars != 0) {
                    YoriLibInitEmptyString(&TempKeyString);
                    YoriLibYPrintf(&TempKeyString, _T("%y.packagepathforolderbuilds"), &Package->Architecture);
                    if (TempKeyString.LengthInChars > 0) {
                        WritePrivateProfileString(Package->PackageName.StartOfString, TempKeyString.StartOfString, Package->PackagePathForOlderBuilds.StartOfString, PackagesIni.StartOfString);
                        YoriLibFreeStringContents(&TempKeyString);
                    }

                }
            }

            if (Err == ERROR_SUCCESS) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Saved %y to %y\n"), &Package->InstallUrl, &FullFinalName);
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Error saving %y to %y: "), &Package->InstallUrl, &FullFinalName);
                YoriPkgDisplayErrorStringForInstallFailure(Err);
            }

            YoriLibFreeStringContents(&FullFinalName);
        }

        PackageEntry = YoriLibGetNextListEntry(&PackageList, PackageEntry);
    }

    YoriPkgFreeAllSourcesAndPackages(&SourcesList, &PackageList);
    YoriLibFreeStringContents(&PackagesIni);

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

 @param CustomSource Optionally specifies a source for packages.  If not
        specified, the INI from NewDirectory is used, and if that's not
        present, fallback to default sources.

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
    __in_opt PYORI_STRING CustomSource,
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

    YoriPkgCollectAllSourcesAndPackages(CustomSource, NewDirectory, &SourcesList, &PackageList);

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

 @param CustomSource Optionally specifies a source for packages.  If not
        specified, the INI from NewDirectory is used, and if that's not
        present, fallback to default sources.

 @param NewDirectory Optionally specifies an install directory.  If not
        specified, the directory of the currently running application is
        used.

 @param MatchVersion Optionally specifies a version of the packages to
        install.  If not specified, the latest version is used.

 @param MatchArch Optionally specifies the architecture of the packages to
        install.  If not specified, determined from the host operaitng
        system.

 @return TRUE to indicate that all specified packages that required upgrading
         were found and installed.  FALSE if one or more packages could not
         be found or installed.
 */
BOOL
YoriPkgInstallRemotePackages(
    __in PYORI_STRING PackageNames,
    __in DWORD PackageNameCount,
    __in_opt PYORI_STRING CustomSource,
    __in_opt PYORI_STRING NewDirectory,
    __in_opt PYORI_STRING MatchVersion,
    __in_opt PYORI_STRING MatchArch
    )
{
    DWORD MatchingPackageCount;
    DWORD AttemptedCount;
    BOOL Result;
    YORI_LIST_ENTRY PackagesMatchingCriteria;
    PYORI_LIST_ENTRY PackageEntry;
    PYORIPKG_REMOTE_PACKAGE Package;
    YORI_STRING IniFile;
    YORI_STRING IniValue;
    YORIPKG_PACKAGES_PENDING_INSTALL PendingPackages;
    DWORD Error;

    Result = FALSE;

    if (!YoriPkgInitializePendingPackages(&PendingPackages)) {
        return Result;
    }

    if (!YoriPkgGetPackageIniFile(NewDirectory, &IniFile)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        return Result;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriPkgDeletePendingPackages(&PendingPackages);
        YoriLibFreeStringContents(&IniFile);
        return Result;
    }

    YoriLibInitializeListHead(&PackagesMatchingCriteria);

    //
    //  Find the number of packages that can be resolved from remote
    //  sources.
    //

    MatchingPackageCount = YoriPkgFindRemotePackages(PackageNames,
                                                     PackageNameCount,
                                                     CustomSource,
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

        Error = YoriPkgPreparePackageForInstallRedirectBuild(&IniFile, NewDirectory, &PendingPackages, &Package->InstallUrl);
        if (Error != ERROR_SUCCESS && Error != ERROR_OLD_WIN_VERSION) {
            YoriPkgDisplayErrorStringForInstallFailure(Error);
            goto Exit;
        }

        AttemptedCount++;
    }

    if (YoriPkgInstallPendingPackages(&IniFile, NewDirectory, &PendingPackages)) {
        if (MatchingPackageCount == PackageNameCount) {
            Result = TRUE;
        }
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

    return Result;
}

/**
 Return the remote package URLs for a specified set of packages.

 @param PackageNames Pointer to an array of one or more package names.

 @param PackageNameCount The number of elements in the PackageName array.

 @param CustomSource Optionally specifies a source for packages.  If not
        specified, the INI from NewDirectory is used, and if that's not
        present, fallback to default sources.

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
    __in_opt PYORI_STRING CustomSource,
    __in_opt PYORI_STRING NewDirectory,
    __deref_out_opt PYORI_STRING * PackageUrls
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
                                                     CustomSource,
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
        *PackageUrls = NULL;
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

/**
 Consult with an in memory cache to see if a newer version of a package is
 available.

 @param PendingPackages The set of packages being operated on.  This contains
        a cache of URLs that are known.

 @param UpgradePath Points to a URL (local or remote) of a package which is
        being upgraded, where the parent directory should be probed for a
        pkglist.ini to see if the package should not be upgraded.

 @param ExistingVersion Points to the existing version to check against.

 @param NewerVersionAvailable On successful completion, set to TRUE to
        indicate a newer version is available, and set to FALSE to indicate
        the currently installed version is current.

 @param RedirectToPackageUrl On completion may be updated to contain a
        referenced string to a version of the package which should be
        attempted on this system because the PackageUrl requires a newer
        host operating system.  This implies the function is returning TRUE
        to indicate a cache match was found.

 @return TRUE if the cache contains the URL of the package being upgraded,
         so the version has been compared for whether it is current.  FALSE
         if the URL is not found, so it is unknown whether the installed
         package is current.
 */
__success(return)
BOOL
YoriPkgIsNewerVersionAvailableFromCache(
    __in PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages,
    __in PYORI_STRING UpgradePath,
    __in PYORI_STRING ExistingVersion,
    __out PBOOLEAN NewerVersionAvailable,
    __out PYORI_STRING RedirectToPackageUrl
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORIPKG_REMOTE_PACKAGE KnownPackage;

    YoriLibInitEmptyString(RedirectToPackageUrl);

    ListEntry = YoriLibGetNextListEntry(&PendingPackages->KnownPackages, ListEntry);
    while (ListEntry != NULL) {
        KnownPackage = CONTAINING_RECORD(ListEntry, YORIPKG_REMOTE_PACKAGE, PackageList);
        ListEntry = YoriLibGetNextListEntry(&PendingPackages->KnownPackages, ListEntry);
        if (YoriLibCompareString(UpgradePath, &KnownPackage->InstallUrl) == 0) {
            if (KnownPackage->MinimumOSBuild.LengthInChars != 0) {
                DWORD OsMajor;
                DWORD OsMinor;
                DWORD OsBuild;
                DWORD CharsConsumed;
                LONGLONG RequiredBuildNumber = 0;
                YoriLibStringToNumber(&KnownPackage->MinimumOSBuild, FALSE, &RequiredBuildNumber, &CharsConsumed);

                YoriLibGetOsVersion(&OsMajor, &OsMinor, &OsBuild);
                if (RequiredBuildNumber > OsBuild) {
                    if (KnownPackage->PackagePathForOlderBuilds.LengthInChars != 0) {
                        YoriLibCloneString(RedirectToPackageUrl, &KnownPackage->PackagePathForOlderBuilds);
                        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y requires newer OS, attempting %y\n"), UpgradePath, RedirectToPackageUrl);

                        *NewerVersionAvailable = FALSE;
                    } else {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y requires newer OS and cannot be installed\n"), UpgradePath);
                        *NewerVersionAvailable = FALSE;
                    }
                }
            }
            if (YoriLibCompareString(ExistingVersion, &KnownPackage->Version) == 0) {
                *NewerVersionAvailable = FALSE;
            } else {
                *NewerVersionAvailable = TRUE;
            }
            return TRUE;
        }
    }

    return FALSE;
}


/**
 Consult with pkglist.ini in a Url's parent directory to see if a newer
 version is available.  Note that there is no guarantee that pkglist.ini
 even exists; this is simply to avoid downloading an entire package
 wherever possible.  Because this function can only really know that a
 newer package is _not_ available, any errors translate to a new package
 _is_ available, indicating that we should fall back to downloading the
 URL to see if the package there is really newer.

 @param PendingPackages The set of packages being operated on.  Note that
        this contains a cache of URLs that are known, because during upgrade
        we will frequently have multiple packages from the same directory on
        a server, so having processed one package we know the result for
        all later packages.

 @param PackagesIni Points to the system's packages.ini file so that
        mirroring can be applied.

 @param UpgradePath Points to a URL (local or remote) of a package which is
        being upgraded, where the parent directory should be probed for a
        pkglist.ini to see if the package should not be upgraded.

 @param ExistingVersion Points to the existing version to check against.

 @param RedirectToPackageUrl Points to a string to update with a referenced
        pointer to a package Url to install.  This can be different from the
        input Url due to mirroring or because that Url is incompatible with
        the running version of Windows and an older package should be
        installed instead.  Note this value is only useful if the function
        indicates a newer package is available; if the currently installed
        package is the current package, there's no point redirecting anywhere.

 @return TRUE to indicate that either we know a newer version is available
         or were unable to complete the check, suggesting that the
         complete package should be downloaded.  FALSE to indicate that
         pkglist.ini was processed successfully and we know for certain that
         the current package version remains current.
 */
__success(return)
BOOL
YoriPkgIsNewerVersionAvailable(
    __inout PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages,
    __in PYORI_STRING PackagesIni,
    __in PYORI_STRING UpgradePath,
    __in PYORI_STRING ExistingVersion,
    __out PYORI_STRING RedirectToPackageUrl
    )
{
    DWORD Index;
    DWORD Err;
    YORI_STRING Substring;
    YORI_STRING MirroredPath;
    YORI_STRING RedirectedUrl;
    YORI_STRING PreviousRedirectedUrl;
    PYORI_STRING UrlToCheck;
    PYORIPKG_REMOTE_SOURCE RemoteSource;
    BOOLEAN NewerVersionAvailable;

    NewerVersionAvailable = FALSE;
    UrlToCheck = UpgradePath;
    YoriLibInitEmptyString(&RedirectedUrl);
    YoriLibInitEmptyString(&PreviousRedirectedUrl);
    while (TRUE) {

        YoriLibInitEmptyString(&MirroredPath);
        if (!YoriPkgConvertUserPackagePathToMirroredPath(UrlToCheck, PackagesIni, &MirroredPath)) {
            YoriLibCloneString(&MirroredPath, UrlToCheck);
        }
        YoriLibFreeStringContents(&PreviousRedirectedUrl);
        YoriLibInitEmptyString(&RedirectedUrl);

        //
        //  First consult the cache.  If we find a match for the MirroredPath,
        //  check the corresponding version for a match.
        //

        if (YoriPkgIsNewerVersionAvailableFromCache(PendingPackages, &MirroredPath, ExistingVersion, &NewerVersionAvailable, &RedirectedUrl)) {
            if (RedirectedUrl.LengthInChars > 0) {
                YoriLibFreeStringContents(&MirroredPath);
                memcpy(&PreviousRedirectedUrl, &RedirectedUrl, sizeof(YORI_STRING));
                UrlToCheck = &PreviousRedirectedUrl;
                continue;
            }
            break;
        }

        //
        //  This implies we haven't found the URL in the cache.  Cook up a URL for
        //  the remote source
        //

        RemoteSource = NULL;
        for (Index = MirroredPath.LengthInChars; Index > 0; Index--) {
            if (YoriLibIsSep(MirroredPath.StartOfString[Index - 1])) {
                YoriLibInitEmptyString(&Substring);
                Substring.StartOfString = MirroredPath.StartOfString;
                Substring.LengthInChars = Index - 1;
                RemoteSource = YoriPkgAllocateRemoteSource(&Substring);
                break;
            }
        }

        if (RemoteSource == NULL) {
            NewerVersionAvailable = TRUE;
            break;
        }

        //
        //  Collect any packages from the remote source
        //

        Err = YoriPkgCollectPackagesFromSource(RemoteSource, PackagesIni, &PendingPackages->KnownPackages, NULL);
        YoriPkgFreeRemoteSource(RemoteSource);
        if (Err != ERROR_SUCCESS) {
            NewerVersionAvailable = TRUE;
            break;
        }

        //
        //  Check the cache again.
        //

        ASSERT(RedirectedUrl.LengthInChars == 0);
        if (YoriPkgIsNewerVersionAvailableFromCache(PendingPackages, &MirroredPath, ExistingVersion, &NewerVersionAvailable, &RedirectedUrl)) {
            if (RedirectedUrl.LengthInChars > 0) {
                YoriLibFreeStringContents(&MirroredPath);
                memcpy(&PreviousRedirectedUrl, &RedirectedUrl, sizeof(YORI_STRING));
                UrlToCheck = &PreviousRedirectedUrl;
                continue;
            }
            break;
        }

        //
        //  If we couldn't find any entry in the cache after loading
        //  pkglist.ini, assume that the package may be newer and download
        //  it to verify.
        //

        NewerVersionAvailable = TRUE;
        break;
    }

    memcpy(RedirectToPackageUrl, &MirroredPath, sizeof(YORI_STRING));

    if (!NewerVersionAvailable) {
        return FALSE;
    }

    return TRUE;
}

/**
 Query the configured sources and display them on the console.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgDisplaySources()
{
    YORI_LIST_ENTRY SourcesList;
    PYORI_LIST_ENTRY SourceEntry;
    PYORIPKG_REMOTE_SOURCE Source;
    BOOLEAN DefaultsUsed;
    YORI_STRING PackagesIni;

    YoriLibInitializeListHead(&SourcesList);

    if (!YoriPkgGetPackageIniFile(NULL, &PackagesIni)) {
        return FALSE;
    }

    if (!YoriPkgCollectSourcesFromIniWithDefaults(&PackagesIni, &SourcesList, &DefaultsUsed)) {
        YoriLibFreeStringContents(&PackagesIni);
        return TRUE;
    }

    //
    //  Display the sources we found.
    //

    SourceEntry = NULL;
    SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
    while (SourceEntry != NULL) {
        Source = CONTAINING_RECORD(SourceEntry, YORIPKG_REMOTE_SOURCE, SourceList);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &Source->SourceRootUrl);
        SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
    }

    YoriPkgFreeAllSourcesAndPackages(&SourcesList, NULL);
    YoriLibFreeStringContents(&PackagesIni);

    return TRUE;
}

/**
 Install a new remote source and add it to packages.ini

 @param SourcePath The new source to install

 @param InstallAsFirst If TRUE, the new source is added to the beginning of
        the list.  If FALSE, it is added to the end.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgAddNewSource(
    __in PYORI_STRING SourcePath,
    __in BOOLEAN InstallAsFirst
    )
{
    YORI_LIST_ENTRY SourcesList;
    PYORI_LIST_ENTRY SourceEntry;
    PYORIPKG_REMOTE_SOURCE Source;
    BOOLEAN DefaultsUsed;
    YORI_STRING PackagesIni;
    DWORD Index;
    YORI_STRING IniKey;

    YoriLibInitializeListHead(&SourcesList);

    if (!YoriPkgGetPackageIniFile(NULL, &PackagesIni)) {
        return FALSE;
    }

    if (!YoriPkgCollectSourcesFromIniWithDefaults(&PackagesIni, &SourcesList, &DefaultsUsed)) {
        YoriLibFreeStringContents(&PackagesIni);
        return FALSE;
    }

    //
    //  Go through the list.  If we find a matching entry, remove it.
    //

    SourceEntry = NULL;
    SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
    while (SourceEntry != NULL) {
        Source = CONTAINING_RECORD(SourceEntry, YORIPKG_REMOTE_SOURCE, SourceList);
        SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
        if (YoriLibCompareString(&Source->SourceRootUrl, SourcePath) == 0) {
            YoriLibRemoveListItem(&Source->SourceList);
            YoriPkgFreeRemoteSource(Source);
        }
    }

    //
    //  Create a new source and insert it at the beginning or the end
    //

    Source = YoriPkgAllocateRemoteSource(SourcePath);
    if (InstallAsFirst) {
        YoriLibInsertList(&SourcesList, &Source->SourceList);
    } else {
        YoriLibAppendList(&SourcesList, &Source->SourceList);
    }

    //
    //  Delete the existing sources section and write the new list
    //

    if (!YoriLibAllocateString(&IniKey, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriPkgFreeAllSourcesAndPackages(&SourcesList, NULL);
        YoriLibFreeStringContents(&PackagesIni);
        return FALSE;
    }
    WritePrivateProfileString(_T("Sources"), NULL, NULL, PackagesIni.StartOfString);
    SourceEntry = NULL;
    Index = 1;
    SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
    while (SourceEntry != NULL) {
        Source = CONTAINING_RECORD(SourceEntry, YORIPKG_REMOTE_SOURCE, SourceList);
        SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
        IniKey.LengthInChars = YoriLibSPrintf(IniKey.StartOfString, _T("Source%i"), Index);
        WritePrivateProfileString(_T("Sources"), IniKey.StartOfString, Source->SourceRootUrl.StartOfString, PackagesIni.StartOfString);
        Index++;
    }

    YoriLibFreeStringContents(&IniKey);
    YoriPkgFreeAllSourcesAndPackages(&SourcesList, NULL);
    YoriLibFreeStringContents(&PackagesIni);

    return TRUE;
}

/**
 Delete a package source and update packages.ini

 @param SourcePath The new source to remove

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgDeleteSource(
    __in PYORI_STRING SourcePath
    )
{
    YORI_LIST_ENTRY SourcesList;
    PYORI_LIST_ENTRY SourceEntry;
    PYORIPKG_REMOTE_SOURCE Source;
    BOOLEAN DefaultsUsed;
    YORI_STRING PackagesIni;
    DWORD Index;
    YORI_STRING IniKey;

    YoriLibInitializeListHead(&SourcesList);

    if (!YoriPkgGetPackageIniFile(NULL, &PackagesIni)) {
        return FALSE;
    }

    if (!YoriPkgCollectSourcesFromIniWithDefaults(&PackagesIni, &SourcesList, &DefaultsUsed)) {
        YoriLibFreeStringContents(&PackagesIni);
        return TRUE;
    }

    //
    //  Go through the list.  If we find a matching entry, remove it.
    //

    SourceEntry = NULL;
    SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
    while (SourceEntry != NULL) {
        Source = CONTAINING_RECORD(SourceEntry, YORIPKG_REMOTE_SOURCE, SourceList);
        SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
        if (YoriLibCompareString(&Source->SourceRootUrl, SourcePath) == 0) {
            YoriLibRemoveListItem(&Source->SourceList);
            YoriPkgFreeRemoteSource(Source);
        }
    }

    //
    //  Delete the existing sources section and write the new list
    //

    if (!YoriLibAllocateString(&IniKey, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriPkgFreeAllSourcesAndPackages(&SourcesList, NULL);
        YoriLibFreeStringContents(&PackagesIni);
        return FALSE;
    }
    WritePrivateProfileString(_T("Sources"), NULL, NULL, PackagesIni.StartOfString);
    SourceEntry = NULL;
    Index = 1;
    SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
    while (SourceEntry != NULL) {
        Source = CONTAINING_RECORD(SourceEntry, YORIPKG_REMOTE_SOURCE, SourceList);
        SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
        IniKey.LengthInChars = YoriLibSPrintf(IniKey.StartOfString, _T("Source%i"), Index);
        WritePrivateProfileString(_T("Sources"), IniKey.StartOfString, Source->SourceRootUrl.StartOfString, PackagesIni.StartOfString);
        Index++;
    }

    YoriLibFreeStringContents(&IniKey);
    YoriPkgFreeAllSourcesAndPackages(&SourcesList, NULL);
    YoriLibFreeStringContents(&PackagesIni);

    return TRUE;
}

// vim:sw=4:ts=4:et:
