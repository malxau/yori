/**
 * @file ypm/remote.c
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
#include "ypm.h"


/**
 Information about a single package that was found on the remote source.
 */
typedef struct _YPM_REMOTE_PACKAGE {

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
} YPM_REMOTE_PACKAGE, *PYPM_REMOTE_PACKAGE;

/**
 Information about a single remote source that contains a set of packages.
 */
typedef struct _YPM_REMOTE_SOURCE {

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
} YPM_REMOTE_SOURCE, *PYPM_REMOTE_SOURCE;

/**
 Allocate and populate a remote source object.

 @param RemoteSourceUrl The root of the remote source.  This can be a URL or
        a local path.

 @return Pointer to the source object.  This can be freed with
         @ref YpmFreeRemoteSource .  This will be NULL on allocation failure.

 */
PYPM_REMOTE_SOURCE
YpmAllocateRemoteSource(
    __in PYORI_STRING RemoteSourceUrl
    )
{
    PYPM_REMOTE_SOURCE RemoteSource;
    DWORD SizeToAllocate;

    SizeToAllocate = sizeof(YPM_REMOTE_SOURCE) +
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
    if (YpmIsPathRemote(&RemoteSource->SourceRootUrl)) {
        RemoteSource->SourcePkgList.LengthInChars = YoriLibSPrintf(RemoteSource->SourcePkgList.StartOfString, _T("%y/pkglist.ini"), &RemoteSource->SourceRootUrl);
    } else {
        RemoteSource->SourcePkgList.LengthInChars = YoriLibSPrintf(RemoteSource->SourcePkgList.StartOfString, _T("%y\\pkglist.ini"), &RemoteSource->SourceRootUrl);
    }
    RemoteSource->SourcePkgList.LengthAllocated = RemoteSource->SourcePkgList.LengthInChars + 1;

    return RemoteSource;
}

/**
 Frees a remote source object previously allocated with
 @ref YpmAllocateRemoteSource .

 @param Source Pointer to the remote source object to free.
 */
VOID
YpmFreeRemoteSource(
    __in PYPM_REMOTE_SOURCE Source
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
         @ref YpmFreeRemotePackage .  This will be NULL on allocation failure.

 */
PYPM_REMOTE_PACKAGE
YpmAllocateRemotePackage(
    __in PYORI_STRING PackageName,
    __in PYORI_STRING Version,
    __in PYORI_STRING Architecture,
    __in PYORI_STRING SourceRootUrl,
    __in PYORI_STRING RelativePackageUrl
    )
{
    PYPM_REMOTE_PACKAGE Package;
    LPTSTR WritePtr;
    DWORD SizeToAllocate;

    SizeToAllocate = sizeof(YPM_REMOTE_PACKAGE) + (PackageName->LengthInChars + 1 + Version->LengthInChars + 1 + Architecture->LengthInChars + 1 + SourceRootUrl->LengthInChars + 1 + RelativePackageUrl->LengthInChars + 1) * sizeof(TCHAR);
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
    if (YpmIsPathRemote(SourceRootUrl)) {
        Package->InstallUrl.LengthInChars = YoriLibSPrintf(Package->InstallUrl.StartOfString, _T("%y/%y"), SourceRootUrl, RelativePackageUrl);
    } else {
        Package->InstallUrl.LengthInChars = YoriLibSPrintf(Package->InstallUrl.StartOfString, _T("%y\\%y"), SourceRootUrl, RelativePackageUrl);
    }

    Package->InstallUrl.LengthAllocated = Package->InstallUrl.LengthInChars + 1;

    return Package;
}

/**
 Frees a remote package object previously allocated with
 @ref YpmAllocateRemotePackage .

 @param Package Pointer to the remote package object to free.
 */
VOID
YpmFreeRemotePackage(
    __in PYPM_REMOTE_PACKAGE Package
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
YpmCollectSourcesFromIni(
    __in PYORI_STRING IniPath,
    __inout PYORI_LIST_ENTRY SourcesList
    )
{
    YORI_STRING IniValue;
    YORI_STRING IniKey;
    DWORD Index;
    BOOL Result = FALSE;
    PYPM_REMOTE_SOURCE Source;

    YoriLibInitEmptyString(&IniValue);
    YoriLibInitEmptyString(&IniKey);

    if (!YoriLibAllocateString(&IniValue, YPM_MAX_FIELD_LENGTH)) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&IniKey, YPM_MAX_FIELD_LENGTH)) {
        goto Exit;
    }

    Index = 1;
    while (TRUE) {
        IniKey.LengthInChars = YoriLibSPrintf(IniKey.StartOfString, _T("Source%i"), Index);
        IniValue.LengthInChars = GetPrivateProfileString(_T("Sources"), IniKey.StartOfString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, IniPath->StartOfString);
        if (IniValue.LengthInChars == 0) {
            break;
        }

        Source = YpmAllocateRemoteSource(&IniValue);
        if (Source == NULL) {
            goto Exit;
        }

        //
        //  MSFIX Check for duplicates/cycles?
        //

        YoriLibAppendList(SourcesList, &Source->SourceList);

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

 @param PackageList Pointer to a list to update with any new packages found.

 @param SourcesList Pointer to a list of sources to update with any new
        sources to check.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YpmCollectPackagesFromSource(
    __in PYPM_REMOTE_SOURCE Source,
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
    BOOL Result = FALSE;

    YoriLibInitEmptyString(&LocalPath);
    YoriLibInitEmptyString(&ProvidesSection);
    YoriLibInitEmptyString(&IniValue);
    YoriLibInitEmptyString(&PkgVersion);

    if (!YpmPackagePathToLocalPath(&Source->SourcePkgList, &LocalPath, &DeleteWhenFinished)) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&ProvidesSection, 64 * 1024)) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&PkgVersion, YPM_MAX_FIELD_LENGTH)) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&IniValue, YPM_MAX_FIELD_LENGTH)) {
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
                    PYPM_REMOTE_PACKAGE Package;
                    Package = YpmAllocateRemotePackage(&PkgNameOnly, &PkgVersion, &Architecture, &Source->SourceRootUrl, &IniValue);
                    if (Package != NULL) {
                        YoriLibAppendList(PackageList, &Package->PackageList);
                    }
                }
            }
        }
    }

    if (!YpmCollectSourcesFromIni(&LocalPath, SourcesList)) {
        goto Exit;
    }

    Result = TRUE;

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
 Query all of the known sources for available packages and display them on
 the console.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YpmDisplayAvailableRemotePackages()
{
    YORI_LIST_ENTRY SourcesList;
    PYORI_LIST_ENTRY SourceEntry;
    PYPM_REMOTE_SOURCE Source;
    YORI_LIST_ENTRY PackageList;
    PYORI_LIST_ENTRY PackageEntry;
    PYPM_REMOTE_PACKAGE Package;
    YORI_STRING PackagesIni;


    YoriLibInitializeListHead(&PackageList);
    YoriLibInitializeListHead(&SourcesList);

    if (!YpmGetPackageIniFile(&PackagesIni)) {
        return FALSE;
    }

    YpmCollectSourcesFromIni(&PackagesIni, &SourcesList);
    YoriLibFreeStringContents(&PackagesIni);

    //
    //  If the INI file provides no place to search, default to malsmith.net
    //

    if (YoriLibIsListEmpty(&SourcesList)) {
        YORI_STRING DummySource;
        YoriLibConstantString(&DummySource, _T("http://www.malsmith.net"));
        Source = YpmAllocateRemoteSource(&DummySource);
        if (Source != NULL) {
            YoriLibAppendList(&SourcesList, &Source->SourceList);
        }
    }

    //
    //  Go through all known sources collecting packages and additional
    //  sources.
    //

    SourceEntry = NULL;
    SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
    while (SourceEntry != NULL) {
        Source = CONTAINING_RECORD(SourceEntry, YPM_REMOTE_SOURCE, SourceList);
        YpmCollectPackagesFromSource(Source, &PackageList, &SourcesList);
        SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
    }


    //
    //  Display the packages we found.
    //

    PackageEntry = NULL;
    PackageEntry = YoriLibGetNextListEntry(&PackageList, PackageEntry);
    while (PackageEntry != NULL) {
        Package = CONTAINING_RECORD(PackageEntry, YPM_REMOTE_PACKAGE, PackageList);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y %y %y %y\n"), Package->PackageName, Package->Version, Package->Architecture, Package->InstallUrl);
        PackageEntry = YoriLibGetNextListEntry(&PackageList, PackageEntry);
    }

    //
    //  Free the packages now we've displayed them.
    //

    PackageEntry = NULL;
    PackageEntry = YoriLibGetNextListEntry(&PackageList, PackageEntry);
    while (PackageEntry != NULL) {
        Package = CONTAINING_RECORD(PackageEntry, YPM_REMOTE_PACKAGE, PackageList);
        PackageEntry = YoriLibGetNextListEntry(&PackageList, PackageEntry);
        YoriLibRemoveListItem(&Package->PackageList);
        YpmFreeRemotePackage(Package);
    }

    //
    //  Free the sources.
    //

    SourceEntry = NULL;
    SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
    while (SourceEntry != NULL) {
        Source = CONTAINING_RECORD(SourceEntry, YPM_REMOTE_SOURCE, SourceList);
        SourceEntry = YoriLibGetNextListEntry(&SourcesList, SourceEntry);
        YoriLibRemoveListItem(&Source->SourceList);
        YpmFreeRemoteSource(Source);
    }

    return TRUE;
}

// vim:sw=4:ts=4:et:
