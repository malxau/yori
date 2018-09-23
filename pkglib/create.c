/**
 * @file pkglib/create.c
 *
 * Yori shell create packages
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
 Creates a binary (installable) package.  This could be architecture specific
 or architecture neutral.

 @param FileName The name of the CAB file to create.

 @param PackageName The name of the package described by the CAB file.

 @param Version The version of the package.

 @param Architecture The architecture of the package.

 @param FileListFile A pointer to a file name whose contents describe the list
        of files that should be included in the package.  This file contains
        one file per line, no wildcards.

 @param UpgradePath Optionally points to a URL to upgrade to the latest
        version of the package from.  If not specified, no UpgradePath is
        included in the package.

 @param SourcePath Optionally points to a URL to download source code for the
        package.  If not specified, no SourcePath is included in the package.

 @param SymbolPath Optionally points to a URL to download debugging symbols
        for the package.  If not specified, no SourcePath is included in the
        package.

 @param Replaces Pointer to an array of strings containing package names that
        this package should replace.  If this package should not replace
        packages with other names, this can be NULL.

 @param ReplaceCount Specifies the number of elements in the Replaces array.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgCreateBinaryPackage(
    __in PYORI_STRING FileName,
    __in PYORI_STRING PackageName,
    __in PYORI_STRING Version,
    __in PYORI_STRING Architecture,
    __in PYORI_STRING FileListFile,
    __in_opt PYORI_STRING UpgradePath,
    __in_opt PYORI_STRING SourcePath,
    __in_opt PYORI_STRING SymbolPath,
    __in_opt PYORI_STRING Replaces,
    __in DWORD ReplaceCount
    )
{
    YORI_STRING TempPath;
    YORI_STRING TempFile;
    YORI_STRING PkgInfoName;
    YORI_STRING LineString;
    YORI_STRING FullFileListFile;
    PVOID LineContext = NULL;
    HANDLE FileListSource;
    DWORD Count;

    PVOID CabHandle;

    //
    //  Query for a temporary directory
    //

    TempPath.LengthAllocated = GetTempPath(0, NULL);
    if (!YoriLibAllocateString(&TempPath, TempPath.LengthAllocated)) {
        return FALSE;
    }
    TempPath.LengthInChars = GetTempPath(TempPath.LengthAllocated, TempPath.StartOfString);

    if (!YoriLibAllocateString(&TempFile, TempPath.LengthAllocated + MAX_PATH)) {
        YoriLibFreeStringContents(&TempPath);
        return FALSE;
    }

    //
    //  Generate a temporary file name to stage pkginfo.ini in to
    //

    if (GetTempFileName(TempPath.StartOfString, _T("ypm"), 0, TempFile.StartOfString) == 0) {
        YoriLibFreeStringContents(&TempPath);
        YoriLibFreeStringContents(&TempFile);
        return FALSE;
    }

    TempFile.LengthInChars = _tcslen(TempFile.StartOfString);
    YoriLibFreeStringContents(&TempPath);

    WritePrivateProfileString(_T("Package"), _T("Name"), PackageName->StartOfString, TempFile.StartOfString);
    WritePrivateProfileString(_T("Package"), _T("Architecture"), Architecture->StartOfString, TempFile.StartOfString);
    WritePrivateProfileString(_T("Package"), _T("Version"), Version->StartOfString, TempFile.StartOfString);
    if (UpgradePath != NULL) {
        WritePrivateProfileString(_T("Package"), _T("UpgradePath"), UpgradePath->StartOfString, TempFile.StartOfString);
    }
    if (SourcePath != NULL) {
        WritePrivateProfileString(_T("Package"), _T("SourcePath"), SourcePath->StartOfString, TempFile.StartOfString);
    }
    if (SymbolPath != NULL) {
        WritePrivateProfileString(_T("Package"), _T("SymbolPath"), SymbolPath->StartOfString, TempFile.StartOfString);
    }

    for (Count = 0; Count < ReplaceCount; Count++) {
        WritePrivateProfileString(_T("Replaces"), Replaces[Count].StartOfString, _T("1"), TempFile.StartOfString);
    }

    if (!YoriLibUserStringToSingleFilePath(FileListFile, TRUE, &FullFileListFile)) {
        YoriLibFreeStringContents(&TempFile);
        return FALSE;
    }

    FileListSource = CreateFile(FullFileListFile.StartOfString,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (FileListSource == INVALID_HANDLE_VALUE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Cannot open %y\n"), &FullFileListFile);
        DeleteFile(TempFile.StartOfString);
        YoriLibFreeStringContents(&TempFile);
        YoriLibFreeStringContents(&FullFileListFile);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullFileListFile);

    if (!YoriLibCreateCab(FileName, &CabHandle)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibCreateCab failure\n"));
        DeleteFile(TempFile.StartOfString);
        YoriLibFreeStringContents(&TempFile);
        CloseHandle(FileListSource);
        return FALSE;
    }
    YoriLibConstantString(&PkgInfoName, _T("pkginfo.ini"));
    if (!YoriLibAddFileToCab(CabHandle, &TempFile, &PkgInfoName)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibAddFileToCab failure\n"));
        DeleteFile(TempFile.StartOfString);
        YoriLibFreeStringContents(&TempFile);
        YoriLibCloseCab(CabHandle);
        CloseHandle(FileListSource);
        return FALSE;
    }

    YoriLibInitEmptyString(&LineString);
    while(TRUE) {
        if (!YoriLibReadLineToString(&LineString, &LineContext, FileListSource)) {
            break;
        }
        if (!YoriLibAddFileToCab(CabHandle, &LineString, &LineString)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibAddFileToCab cannot add %y\n"), &LineString);
            DeleteFile(TempFile.StartOfString);
            YoriLibFreeStringContents(&TempFile);
            YoriLibCloseCab(CabHandle);
            YoriLibLineReadClose(LineContext);
            YoriLibFreeStringContents(&LineString);
            CloseHandle(FileListSource);
            return FALSE;
        }

    }

    YoriLibLineReadClose(LineContext);
    CloseHandle(FileListSource);
    YoriLibFreeStringContents(&LineString);
    YoriLibCloseCab(CabHandle);
    DeleteFile(TempFile.StartOfString);
    YoriLibFreeStringContents(&TempFile);

    return TRUE;
}

/**
 A single item to exclude or include.  Note this can refer to multiple files.
 */
typedef struct _YORIPKG_MATCH_ITEM {

    /**
     List of items to match.
     */
    YORI_LIST_ENTRY MatchList;

    /**
     A string describing the object to match, which may include
     wildcards.
     */
    YORI_STRING MatchCriteria;
} YORIPKG_MATCH_ITEM, *PYORIPKG_MATCH_ITEM;

/**
 Context passed between the source package creation operation and every file
 found while creating the source package.
 */
typedef struct _YORIPKG_CREATE_SOURCE_CONTEXT {

    /**
     A handle to the Cabinet being created.
     */
    PVOID CabHandle;

    /**
     Pointer to the name of the package.
     */
    PYORI_STRING PackageName;

    /**
     Pointer to the version of the package.
     */
    PYORI_STRING PackageVersion;

    /**
     A list of criteria to exclude.
     */
    YORI_LIST_ENTRY ExcludeList;

    /**
     A list of criteria to include, even if they have been excluded by the
     ExcludeList.
     */
    YORI_LIST_ENTRY IncludeList;
} YORIPKG_CREATE_SOURCE_CONTEXT, *PYORIPKG_CREATE_SOURCE_CONTEXT;

/**
 Add a new match criteria to the list.

 @param List Pointer to the list to add the match criteria to.

 @param NewCriteria Pointer to the new criteria to add, which may include
        wildcards.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgCreateSourceAddMatch(
    __in PYORI_LIST_ENTRY List,
    __in PYORI_STRING NewCriteria
    )
{
    PYORIPKG_MATCH_ITEM MatchItem;
    MatchItem = YoriLibReferencedMalloc(sizeof(YORIPKG_MATCH_ITEM) + (NewCriteria->LengthInChars + 1) * sizeof(TCHAR));

    if (MatchItem == NULL) {
        return FALSE;
    }

    ZeroMemory(MatchItem, sizeof(YORIPKG_MATCH_ITEM));
    MatchItem->MatchCriteria.StartOfString = (LPTSTR)(MatchItem + 1);
    MatchItem->MatchCriteria.LengthInChars = NewCriteria->LengthInChars;
    MatchItem->MatchCriteria.LengthAllocated = NewCriteria->LengthInChars + 1;
    memcpy(MatchItem->MatchCriteria.StartOfString, NewCriteria->StartOfString, MatchItem->MatchCriteria.LengthInChars * sizeof(TCHAR));
    MatchItem->MatchCriteria.StartOfString[MatchItem->MatchCriteria.LengthInChars] = '\0';
    YoriLibAppendList(List, &MatchItem->MatchList);
    return TRUE;
}

/**
 Free all previously added exclude or include criteria.

 @param CreateSourceContext Pointer to the create source context to free all
        exclude or include criteria from.
 */
VOID
YoriPkgCreateSourceFreeMatchLists(
    __in PYORIPKG_CREATE_SOURCE_CONTEXT CreateSourceContext
    )
{
    PYORIPKG_MATCH_ITEM MatchItem;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&CreateSourceContext->ExcludeList, NULL);
    while (ListEntry != NULL) {
        MatchItem = CONTAINING_RECORD(ListEntry, YORIPKG_MATCH_ITEM, MatchList);
        YoriLibRemoveListItem(&MatchItem->MatchList);
        YoriLibDereference(MatchItem);
        ListEntry = YoriLibGetNextListEntry(&CreateSourceContext->ExcludeList, NULL);
    }

    ListEntry = YoriLibGetNextListEntry(&CreateSourceContext->IncludeList, NULL);
    while (ListEntry != NULL) {
        MatchItem = CONTAINING_RECORD(ListEntry, YORIPKG_MATCH_ITEM, MatchList);
        YoriLibRemoveListItem(&MatchItem->MatchList);
        YoriLibDereference(MatchItem);
        ListEntry = YoriLibGetNextListEntry(&CreateSourceContext->IncludeList, NULL);
    }
}

/**
 Returns TRUE to indicate that an object should be excluded based on the
 exclude criteria, or FALSE if it should be included.

 @param CreateSourceContext Pointer to the create source context to check the
        new object against.

 @param RelativeSourcePath Pointer to a string describing the file relative
        to the root of the source of the create operation.

 @return TRUE to exclude the file, FALSE to include it.
 */
BOOL
YoriPkgCreateSourceShouldExclude(
    __in PYORIPKG_CREATE_SOURCE_CONTEXT CreateSourceContext,
    __in PYORI_STRING RelativeSourcePath
    )
{
    PYORIPKG_MATCH_ITEM MatchItem;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&CreateSourceContext->ExcludeList, NULL);
    while (ListEntry != NULL) {
        MatchItem = CONTAINING_RECORD(ListEntry, YORIPKG_MATCH_ITEM, MatchList);
        if (YoriLibDoesFileMatchExpression(RelativeSourcePath, &MatchItem->MatchCriteria)) {

            ListEntry = YoriLibGetNextListEntry(&CreateSourceContext->IncludeList, NULL);
            while (ListEntry != NULL) {
                MatchItem = CONTAINING_RECORD(ListEntry, YORIPKG_MATCH_ITEM, MatchList);
                if (YoriLibDoesFileMatchExpression(RelativeSourcePath, &MatchItem->MatchCriteria)) {
                    return FALSE;
                }
                ListEntry = YoriLibGetNextListEntry(&CreateSourceContext->IncludeList, ListEntry);
            }
            return TRUE;
        }
        ListEntry = YoriLibGetNextListEntry(&CreateSourceContext->ExcludeList, ListEntry);
    }
    return FALSE;
}

/**
 A callback that is invoked when a file is found within the tree root that is
 being turned into a source package.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Indicates the recursion depth.  Used to determine the relative
        path to check if an object should be included or excluded.

 @param Context Pointer to a context block specifying the destination of 
        package.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
YoriPkgCreateSourceFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PYORIPKG_CREATE_SOURCE_CONTEXT CreateSourceContext = (PYORIPKG_CREATE_SOURCE_CONTEXT)Context;
    YORI_STRING RelativePathFromSource;
    YORI_STRING PathInCab;
    DWORD SlashesFound;
    DWORD Index;

    UNREFERENCED_PARAMETER(FileInfo);

    YoriLibInitEmptyString(&RelativePathFromSource);

    SlashesFound = 0;
    for (Index = FilePath->LengthInChars; Index > 0; Index--) {
        if (FilePath->StartOfString[Index - 1] == '\\') {
            SlashesFound++;
            if (SlashesFound == Depth + 1) {
                break;
            }
        }
    }

    ASSERT(Index > 0);
    ASSERT(SlashesFound == Depth + 1);

    RelativePathFromSource.StartOfString = &FilePath->StartOfString[Index];
    RelativePathFromSource.LengthInChars = FilePath->LengthInChars - Index;

    //
    //  Skip any object starting with .git or .svn
    //

    if (YoriLibCompareStringWithLiteralCount(&RelativePathFromSource, _T(".git"), sizeof(".git") - 1) == 0 ||
        YoriLibCompareStringWithLiteralCount(&RelativePathFromSource, _T(".svn"), sizeof(".svn") - 1) == 0) {

        return TRUE;
    }

    //
    //  Skip anything .gitinclude said should be skipped
    //

    if (YoriPkgCreateSourceShouldExclude(CreateSourceContext, &RelativePathFromSource)) {
        return TRUE;
    }

    YoriLibInitEmptyString(&PathInCab);
    YoriLibYPrintf(&PathInCab, _T("src\\%y-%y\\%y"), CreateSourceContext->PackageName, CreateSourceContext->PackageVersion, &RelativePathFromSource);

    if (!YoriLibAddFileToCab(CreateSourceContext->CabHandle, FilePath, &PathInCab)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibAddFileToCab cannot add %y\n"), &RelativePathFromSource);
    }
    YoriLibFreeStringContents(&PathInCab);

    return TRUE;
}

/**
 Creates a source package.  This is intrinsically architecture neutral and is
 comprised of a directory tree rather than a file list.

 @param FileName The name of the CAB file to create.

 @param PackageName The name of the package described by the CAB file.

 @param Version The version of the package.

 @param FileRoot A pointer to a file tree root that contains the source tree.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgCreateSourcePackage(
    __in PYORI_STRING FileName,
    __in PYORI_STRING PackageName,
    __in PYORI_STRING Version,
    __in PYORI_STRING FileRoot
    )
{
    YORI_STRING TempPath;
    YORI_STRING TempFile;
    YORI_STRING PkgInfoName;
    YORI_STRING ExcludeFilePath;
    YORIPKG_CREATE_SOURCE_CONTEXT CreateSourceContext;

    ZeroMemory(&CreateSourceContext, sizeof(CreateSourceContext));
    YoriLibInitializeListHead(&CreateSourceContext.ExcludeList);
    YoriLibInitializeListHead(&CreateSourceContext.IncludeList);

    //
    //  Query for a temporary directory
    //

    TempPath.LengthAllocated = GetTempPath(0, NULL);
    if (!YoriLibAllocateString(&TempPath, TempPath.LengthAllocated)) {
        return FALSE;
    }
    TempPath.LengthInChars = GetTempPath(TempPath.LengthAllocated, TempPath.StartOfString);

    if (!YoriLibAllocateString(&TempFile, TempPath.LengthAllocated + MAX_PATH)) {
        YoriLibFreeStringContents(&TempPath);
        return FALSE;
    }

    //
    //  Generate a temporary file name to stage pkginfo.ini in to
    //

    if (GetTempFileName(TempPath.StartOfString, _T("ypm"), 0, TempFile.StartOfString) == 0) {
        YoriLibFreeStringContents(&TempPath);
        YoriLibFreeStringContents(&TempFile);
        return FALSE;
    }

    TempFile.LengthInChars = _tcslen(TempFile.StartOfString);
    YoriLibFreeStringContents(&TempPath);

    WritePrivateProfileString(_T("Package"), _T("Name"), PackageName->StartOfString, TempFile.StartOfString);
    WritePrivateProfileString(_T("Package"), _T("Version"), Version->StartOfString, TempFile.StartOfString);
    WritePrivateProfileString(_T("Package"), _T("Architecture"), _T("noarch"), TempFile.StartOfString);

    if (!YoriLibCreateCab(FileName, &CreateSourceContext.CabHandle)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibCreateCab failure\n"));
        DeleteFile(TempFile.StartOfString);
        YoriLibFreeStringContents(&TempFile);
        return FALSE;
    }
    YoriLibConstantString(&PkgInfoName, _T("pkginfo.ini"));
    if (!YoriLibAddFileToCab(CreateSourceContext.CabHandle, &TempFile, &PkgInfoName)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibAddFileToCab failure\n"));
        DeleteFile(TempFile.StartOfString);
        YoriLibFreeStringContents(&TempFile);
        YoriLibCloseCab(CreateSourceContext.CabHandle);
        return FALSE;
    }

    DeleteFile(TempFile.StartOfString);
    YoriLibFreeStringContents(&TempFile);

    YoriLibInitEmptyString(&ExcludeFilePath);
    YoriLibYPrintf(&ExcludeFilePath, _T("%y\\.gitignore"), FileRoot);
    if (ExcludeFilePath.StartOfString != NULL) {
        HANDLE ExcludeFileHandle;
        YORI_STRING LineString;
        PVOID LineContext = NULL;
        DWORD Index;

        ExcludeFileHandle = CreateFile(ExcludeFilePath.StartOfString,
                                       GENERIC_READ,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);
    
        if (ExcludeFileHandle != INVALID_HANDLE_VALUE) {
            YoriLibInitEmptyString(&LineString);
            while(TRUE) {
                if (!YoriLibReadLineToString(&LineString, &LineContext, ExcludeFileHandle)) {
                    break;
                }

                if (LineString.LengthInChars > 0) {

                    for (Index = 0; Index < LineString.LengthInChars; Index++) {
                        if (YoriLibIsSep(LineString.StartOfString[Index])) {
                            LineString.StartOfString[Index] = '\\';
                        }
                    }

                    if (LineString.StartOfString[0] == '!') {
                        LineString.StartOfString++;
                        LineString.LengthInChars--;
                        if (LineString.LengthInChars > 0) {
                            YoriPkgCreateSourceAddMatch(&CreateSourceContext.IncludeList, &LineString);
                        }
                        LineString.StartOfString--;
                        LineString.LengthInChars++;
                    } else {
                        YoriPkgCreateSourceAddMatch(&CreateSourceContext.ExcludeList, &LineString);
                    }
                }
            }
            YoriLibLineReadClose(LineContext);
            YoriLibFreeStringContents(&LineString);
            CloseHandle(ExcludeFileHandle);
        }
    }
    YoriLibFreeStringContents(&ExcludeFilePath);

    CreateSourceContext.PackageName = PackageName;
    CreateSourceContext.PackageVersion = Version;

    YoriLibForEachFile(FileRoot,
                       YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS | YORILIB_FILEENUM_RECURSE_AFTER_RETURN | YORILIB_FILEENUM_NO_LINK_TRAVERSE,
                       0,
                       YoriPkgCreateSourceFileFoundCallback,
                       &CreateSourceContext);

    YoriLibCloseCab(CreateSourceContext.CabHandle);
    YoriPkgCreateSourceFreeMatchLists(&CreateSourceContext);
    return TRUE;
}

// vim:sw=4:ts=4:et:
