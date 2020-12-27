/**
 * @file lib/path.c
 *
 * Yori lookup expression in path and determine if it's an external executable
 *
 * Copyright (c) 2017 Malcolm J. Smith
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Searches an environment variable with semicolon delimited elements for a file
 name match.

 @param FileName The file name to search for within the environment variable.

 @param EnvVarData The contents of the environment variable to search.

 @param ScratchArea A buffer for this function to use while probing for files.
        This must be OutLen chars in size.

 @param MatchAllCallback An optional callback to invoke each time a
        candidate match is found.

 @param MatchAllContext Context information to supply to MatchAllCallback
        if it is specified.

 @param Out On successful completion, a buffer to copy the resulting match.
        If no match is found, this is initialized as an empty string.

 @param FullPath If TRUE, the caller wants an absolute, escaped path name.
        If FALSE, a non-escaped absolute path name is desired.
 */
VOID
YoriLibSearchEnv(
    __in PYORI_STRING FileName,
    __in PYORI_STRING EnvVarData,
    __in PYORI_STRING ScratchArea,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING Out,
    __in BOOL FullPath
    )
{
    TCHAR CONST * begin;
    TCHAR CONST * end;
    TCHAR search;
    HANDLE hFind;
    WIN32_FIND_DATA FindData;
    LPTSTR fn;

    ASSERT(YoriLibIsStringNullTerminated(FileName));
    ASSERT(YoriLibIsStringNullTerminated(EnvVarData));

    //
    //  If we can't possibly do anything, stop.
    //

    if (Out->LengthAllocated == 0) {
        return;
    }

    //
    //  Check the current directory first
    //

    hFind = FindFirstFile(FileName->StartOfString, &FindData);

    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        if (!YoriLibGetFullPathNameReturnAllocation(FileName, FullPath, Out, &fn) || fn == NULL) {
            Out->LengthInChars = 0;
            Out->StartOfString[0] = '\0';
            return;
        }
        Out->LengthInChars -= Out->LengthInChars - (DWORD)(fn - Out->StartOfString);
        Out->LengthInChars += YoriLibSPrintfS(fn, Out->LengthAllocated - (DWORD)(fn-Out->StartOfString), _T("%s"), FindData.cFileName);
        if (MatchAllCallback != NULL) {
            if (!MatchAllCallback(Out, MatchAllContext)) {
                return;
            }
        } else {
            return;
        }
    }

    Out->StartOfString[0] = '\0';

    begin = EnvVarData->StartOfString;

    while (*begin == ';') begin++;

    while (*begin) {

        DWORD componentlen;

        search = ';';

        if (*begin == '"') {
            begin++;
            search = '"';
        }

        end = _tcschr(begin, search);

        if (end == NULL) {

            end = begin + _tcslen(begin);
        }

        componentlen = (DWORD)(end - begin);

        if (componentlen != 0 && componentlen + 1 + FileName->LengthInChars < Out->LengthAllocated) {

            memcpy(ScratchArea->StartOfString, (LPTSTR)begin, componentlen * sizeof(TCHAR));
            memcpy(ScratchArea->StartOfString + componentlen, _T("\\"), sizeof(TCHAR));
            YoriLibSPrintf(ScratchArea->StartOfString + componentlen + 1, _T("%y"), FileName);
            ScratchArea->LengthInChars = componentlen + 1 + FileName->LengthInChars;

            hFind = FindFirstFile(ScratchArea->StartOfString, &FindData);

            if (hFind != INVALID_HANDLE_VALUE) {
                FindClose(hFind);
                if (!YoriLibGetFullPathNameReturnAllocation(ScratchArea, FullPath, Out, &fn) || fn == NULL) {
                    Out->LengthInChars = 0;
                    Out->StartOfString[0] = '\0';
                    return;
                }
                Out->LengthInChars -= Out->LengthInChars - (DWORD)(fn - Out->StartOfString);
                Out->LengthInChars += YoriLibSPrintfS(fn, Out->LengthAllocated - (DWORD)(fn-Out->StartOfString), _T("%s"), FindData.cFileName);
                if (MatchAllCallback != NULL) {
                    if (!MatchAllCallback(Out, MatchAllContext)) {
                        return;
                    }
                } else {
                    return;
                }
            }
        }

        begin = end;
        if (search == '"') {
            while (*begin == '"') begin++;
        }

        while (*begin == ';') begin++;
    }
}

/**
 A decomposed form of the PathExt environment variable, containing an array
 of this structure, indicating the extension name as a counted string and
 a boolean flag to indicate whether or not a match was found.  This allows
 a searcher to enumerate all files marking what was found, then later checking
 which one should be "first."
 */
typedef struct _YORI_PATHEXT_COMPONENT {

    /**
     The extension to search for.
     */
    YORI_STRING Extension;

    /**
     Set to TRUE if the extension was found, remains FALSE if it was not.
     */
    BOOL Found;
} YORI_PATHEXT_COMPONENT, *PYORI_PATHEXT_COMPONENT;

/**
 Convert a directory name and matched file within that directory into a fully
 qualified file path.

 @param SearchPath Pointer to the directory being searched.

 @param Match Pointer to the object found within the directory.

 @param Out On successful completion, updated to point to a fully qualified
        path to the file.

 @param FullPath If TRUE, return an escaped form of the path; if FALSE, return
        a Win32 path without any escape.

 @return TRUE to indicate the lookup was successful, and FALSE to indicate a
         lookup failure.  Success does not imply a match was found; if a
         lookup successfully found nothing, FoundPath will contain an empty
         string.
 */
__success(return)
BOOL
YoriLibLocateBuildFullName(
    __in PYORI_STRING SearchPath,
    __in PWIN32_FIND_DATA Match,
    __inout PYORI_STRING Out,
    __in BOOL FullPath
    )
{

    YORI_STRING FoundPath;
    LPTSTR fn;
    BOOL NeedsSeperator;
    DWORD FileNameLen = _tcslen(Match->cFileName);

    if (!YoriLibAllocateString(&FoundPath, SearchPath->LengthInChars + 1 + FileNameLen + 1)) {
        return FALSE;
    }

    memcpy(FoundPath.StartOfString,
           SearchPath->StartOfString,
           SearchPath->LengthInChars * sizeof(TCHAR));

    //
    //  Normally we'd build the search path, a seperator, and the file
    //  criteria.  If the search path is just an X: prefix though, we
    //  really don't want to add the seperator, which would completely
    //  change the meaning of the request.
    //

    NeedsSeperator = TRUE;
    if (SearchPath->LengthInChars == 2 &&
        SearchPath->StartOfString[1] == ':') {

        NeedsSeperator = FALSE;
    }

    if (NeedsSeperator) {
        FoundPath.StartOfString[SearchPath->LengthInChars] = '\\';
        FoundPath.LengthInChars = SearchPath->LengthInChars + 1;
    } else {
        FoundPath.LengthInChars = SearchPath->LengthInChars;
    }

    memcpy(&FoundPath.StartOfString[FoundPath.LengthInChars], Match->cFileName, (FileNameLen + 1) * sizeof(TCHAR));
    FoundPath.LengthInChars += FileNameLen;

    if (!YoriLibGetFullPathNameReturnAllocation(&FoundPath, FullPath, Out, &fn)) {
        YoriLibFreeStringContents(&FoundPath);
        return FALSE;
    }
    YoriLibFreeStringContents(&FoundPath);

    return TRUE;
}

/**
 A hardcoded search order for file extensions if the environment variable is
 not defined.
 */
LPCTSTR YoriLibDefaultPathExt = _T(".com;.exe;.bat;.cmd");

/**
 Convert the string based pathext environment variable into an array of
 potential components that are stored in search order and can be marked
 if a match is found in any directory.  Because search order is not the
 same as enumerate order, we need a process to determine what is found
 and a seperate step for which is first.

 @param ComponentCount On successful completion, the number of elements
        stored in the path ext array.

 @return On successful completion, points to an array of pathext components.
         The caller is expected to free this with
         @ref YoriLibPathFreePathExtComponents .
 */
__success(return != NULL)
PYORI_PATHEXT_COMPONENT
YoriLibPathBuildPathExtComponentList(
    __out PDWORD ComponentCount
    )
{
    BOOLEAN UseDefaultPathExt = FALSE;
    TCHAR * PathExtString;
    DWORD PathExtLength;
    DWORD PathExtCount;
    DWORD PathExtIndex;
    TCHAR * ThisExt;
    TCHAR * TokCtx;
    PYORI_PATHEXT_COMPONENT PathExtComponents;

    //
    //  Check if the PathExt environment variable is defined, and if so,
    //  how big it is.  Allocate enough memory to capture it.  If it's
    //  not there, fall back to the default string.
    //

    UseDefaultPathExt = FALSE;
    PathExtLength = GetEnvironmentVariable(_T("PATHEXT"), NULL, 0);
    if (PathExtLength == 0) {
        PathExtLength = _tcslen(YoriLibDefaultPathExt) + sizeof(TCHAR);
        UseDefaultPathExt = TRUE;
    }

    PathExtString = YoriLibReferencedMalloc(PathExtLength * sizeof(TCHAR));
    if (PathExtString == NULL) {
        return NULL;
    }

    if (UseDefaultPathExt) {
        _tcscpy(PathExtString, YoriLibDefaultPathExt);
    } else {
        GetEnvironmentVariable(_T("PATHEXT"), PathExtString, PathExtLength);
    }

    //
    //  Count the number of elements in pathext
    //

    ThisExt = _tcstok_s(PathExtString, _T(";"), &TokCtx);
    PathExtCount = 0;
    while (ThisExt != NULL) {
        if (ThisExt[0] != '\0') {
            PathExtCount++;
        }
        ThisExt = _tcstok_s(NULL, _T(";"), &TokCtx);
    }

    //
    //  Allocate and populate an array of pathext values
    //

    PathExtComponents = YoriLibMalloc(sizeof(YORI_PATHEXT_COMPONENT) * PathExtCount);
    if (PathExtComponents == NULL) {
        YoriLibFree(PathExtString);
        return NULL;
    }

    if (UseDefaultPathExt) {
        _tcscpy(PathExtString, YoriLibDefaultPathExt);
    } else {
        GetEnvironmentVariable(_T("PATHEXT"), PathExtString, PathExtLength);
    }

    ThisExt = _tcstok_s(PathExtString, _T(";"), &TokCtx);
    PathExtIndex = 0;
    while (ThisExt != NULL) {
        if (ThisExt[0] != '\0') {
            YoriLibReference(PathExtString);
            PathExtComponents[PathExtIndex].Extension.MemoryToFree = PathExtString;
            PathExtComponents[PathExtIndex].Extension.StartOfString = ThisExt;
            PathExtComponents[PathExtIndex].Extension.LengthInChars = _tcslen(ThisExt);
            PathExtComponents[PathExtIndex].Extension.LengthAllocated = PathExtComponents[PathExtIndex].Extension.LengthInChars;
            PathExtIndex++;
        }
        ThisExt = _tcstok_s(NULL, _T(";"), &TokCtx);
    }

    YoriLibDereference(PathExtString);

    *ComponentCount = PathExtCount;
    return PathExtComponents;
}

/**
 Frees a path ext component array previously allocated with @ref
 YoriLibPathBuildPathExtComponentList .

 @param PathExtComponents Pointer to the array of path ext components,
        as returned by @ref YoriLibPathBuildPathExtComponentList .

 @param PathExtComponentCount The number of elements in PathExtComponents .
 */
VOID
YoriLibPathFreePathExtComponents(
    __in PYORI_PATHEXT_COMPONENT PathExtComponents,
    __in DWORD PathExtComponentCount
    )
{
    DWORD Count;

    for (Count = 0; Count < PathExtComponentCount; Count++) {
        if (PathExtComponents[Count].Extension.MemoryToFree != NULL) {
            YoriLibDereference(PathExtComponents[Count].Extension.MemoryToFree);
            PathExtComponents[Count].Extension.MemoryToFree = NULL;
        }
    }

    YoriLibFree(PathExtComponents);
}

/**
 Search through a single path matching against desired file extensions.

 @param FileName Pointer to the base file name to search for.

 @param SearchPath The directory to search through for matches.

 @param ScratchArea Pointer to a buffer that this routine can freely use.
        It is returned in allocated form to the caller so if this function
        is invoked multiple times the same allocation can be recycled.

 @param PathExtData Points to an array of file name extensions to search for.

 @param PathExtCount The number of elements in PathExtData.

 @param MatchAllCallback Optionally points to a callback function to be invoked
       on every potential match found.  If not specified, the first match is
       returned in Out.

 @param MatchAllContext Optionally points to context to supply to
        MatchAllCallback .

 @param Out Points to a buffer to populate with the first match if
        MatchAllCallback is not specified.

 @param FullPath If TRUE, return an escaped form of the path; if FALSE, return
        a Win32 path without any escape.

 @return TRUE to indicate the lookup was successful, and FALSE to indicate a
         lookup failure.  Success does not imply a match was found; if a
         lookup successfully found nothing, FoundPath will contain an empty
         string.
 */
__success(return)
BOOL
YoriLibLocateFileExtensionsInOnePath(
    __in PYORI_STRING FileName,
    __in PYORI_STRING SearchPath,
    __in PYORI_STRING ScratchArea,
    __inout PYORI_PATHEXT_COMPONENT PathExtData,
    __in DWORD PathExtCount,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING Out,
    __in BOOL FullPath
    )
{
    HANDLE hFind;
    DWORD FileNameLen;
    WIN32_FIND_DATA FindData;
    WIN32_FIND_DATA *PathExtMatches;
    YORI_STRING SearchName;
    BOOL PartialMatchOkay;
    BOOL NeedsSeperator;
    DWORD Count;

    //
    //  If we can't possibly do anything, stop.
    //

    if (Out->LengthAllocated == 0 || FileName->LengthInChars == 0) {
        return FALSE;
    }

    Out->StartOfString[0] = '\0';

    //
    //  If the caller has specified a file name with a wildcard, they're
    //  okay finding anything.  If not, we need to make sure the file
    //  name specified plus the extension we find is an exact match.
    //

    if (FileName->StartOfString[FileName->LengthInChars - 1] == '*') {
        PartialMatchOkay = TRUE;
    } else {
        PartialMatchOkay = FALSE;
    }

    //
    //  Allocate a scratch area for the directory, search file name, a
    //  seperator, a wildcard, and a terminator; as well as find results
    //  for each matching extension.  In case this function is invoked
    //  repeatedly, overallocate somewhat if we're forced to allocate
    //  here.
    //

    FileNameLen = SearchPath->LengthInChars + FileName->LengthInChars + 3;

    if (ScratchArea->LengthAllocated < (FileNameLen * sizeof(TCHAR) + sizeof(WIN32_FIND_DATA) * PathExtCount)) {
        YoriLibFreeStringContents(ScratchArea);
        if (!YoriLibAllocateString(ScratchArea, (FileNameLen + 0x40) * sizeof(TCHAR) + sizeof(WIN32_FIND_DATA) * PathExtCount)) {
            return FALSE;
        }
    }

    YoriLibInitEmptyString(&SearchName);
    SearchName.StartOfString = ScratchArea->StartOfString;
    SearchName.LengthAllocated = FileNameLen;

    PathExtMatches = (PWIN32_FIND_DATA)YoriLibAddToPointer(ScratchArea->StartOfString, SearchName.LengthAllocated * sizeof(TCHAR));

    //
    //  Normally we'd build the search path, a seperator, and the file
    //  criteria.  If the search path is just an X: prefix though, we
    //  really don't want to add the seperator, which would completely
    //  change the meaning of the request.  If the search path ends in
    //  a seperator already, don't add another.
    //

    NeedsSeperator = TRUE;
    if (SearchPath->LengthInChars == 2 &&
        SearchPath->StartOfString[1] == ':') {

        NeedsSeperator = FALSE;
    } else if (SearchPath->LengthInChars > 0 &&
               YoriLibIsSep(SearchPath->StartOfString[SearchPath->LengthInChars - 1])) {

        NeedsSeperator = FALSE;
    }

    //
    //  Populate the scratch area with the aforementioned directory,
    //  seperator, file name, wildcard, and terminator
    //

    memcpy(SearchName.StartOfString,
           SearchPath->StartOfString,
           SearchPath->LengthInChars * sizeof(TCHAR));

    if (NeedsSeperator) {
        SearchName.StartOfString[SearchPath->LengthInChars] = '\\';
        SearchName.LengthInChars = SearchPath->LengthInChars + 1;
    } else {
        SearchName.LengthInChars = SearchPath->LengthInChars;
    }

    memcpy(&SearchName.StartOfString[SearchName.LengthInChars],
           FileName->StartOfString,
           FileName->LengthInChars * sizeof(TCHAR));

    SearchName.StartOfString[SearchName.LengthInChars + FileName->LengthInChars] = '*';
    SearchName.StartOfString[SearchName.LengthInChars + FileName->LengthInChars + 1] = '\0';
    SearchName.LengthInChars += FileName->LengthInChars;
    ASSERT(SearchName.LengthInChars < SearchName.LengthAllocated);

    //
    //  Before we start searching, indicate that we haven't found anything.
    //

    for (Count = 0; Count < PathExtCount; Count++) {
        PathExtData[Count].Found = FALSE;
    }

    //
    //  Search the directory for all files with this prefix.
    //

    hFind = FindFirstFile(SearchName.StartOfString, &FindData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return TRUE;
    }

    //
    //  For every file we find in the pathext list, mark it as found.
    //

    do {
        FileNameLen = _tcslen(FindData.cFileName);
        for (Count = 0; Count < PathExtCount; Count++) {
            if (!PathExtData[Count].Found &&
                FileNameLen > PathExtData[Count].Extension.LengthInChars) {

                if (_tcsnicmp(PathExtData[Count].Extension.StartOfString, &FindData.cFileName[FileNameLen - PathExtData[Count].Extension.LengthInChars], PathExtData[Count].Extension.LengthInChars) == 0) {

                    //
                    //  If we are looking for all matches of a partial match,
                    //  recurse looking for all extensions of a non-partial
                    //  match.  This may return duplicates; this logic currently
                    //  assumes the upper layer can handle that.
                    //

                    if (PartialMatchOkay && MatchAllCallback != NULL) {
                        PYORI_PATHEXT_COMPONENT ChildPathExtComponents;
                        DWORD ChildPathExtCount;
                        YORI_STRING ChildFileName;
                        YORI_STRING ChildScratchArea;

                        ChildPathExtComponents = YoriLibPathBuildPathExtComponentList(&ChildPathExtCount);
                        if (ChildPathExtComponents == NULL) {
                            FindClose(hFind);
                            return FALSE;
                        }

                        YoriLibInitEmptyString(&ChildScratchArea);
                        ChildFileName.StartOfString = FindData.cFileName;
                        ChildFileName.LengthInChars = FileNameLen - PathExtData[Count].Extension.LengthInChars;
                        ChildFileName.LengthAllocated = FileNameLen + 1;

                        if (!YoriLibLocateFileExtensionsInOnePath(&ChildFileName,
                                                                  SearchPath,
                                                                  &ChildScratchArea,
                                                                  ChildPathExtComponents,
                                                                  ChildPathExtCount,
                                                                  MatchAllCallback,
                                                                  MatchAllContext,
                                                                  Out,
                                                                  FullPath)) {

                            YoriLibPathFreePathExtComponents(ChildPathExtComponents, ChildPathExtCount);
                            YoriLibFreeStringContents(&ChildScratchArea);
                            FindClose(hFind);
                            return FALSE;
                        }

                        YoriLibFreeStringContents(&ChildScratchArea);
                        YoriLibPathFreePathExtComponents(ChildPathExtComponents, ChildPathExtCount);

                    } else if (FileNameLen - PathExtData[Count].Extension.LengthInChars == FileName->LengthInChars) {

                        PathExtData[Count].Found = TRUE;
                        memcpy(&PathExtMatches[Count], &FindData, sizeof(WIN32_FIND_DATA));
                    }
                }
            }
        }
        
    } while(FindNextFile(hFind, &FindData));

    FindClose(hFind);

    if (MatchAllCallback != NULL) {
        for (Count = 0; Count < PathExtCount; Count++) {
            if (PathExtData[Count].Found) {
                if (!YoriLibLocateBuildFullName(SearchPath,
                                                &PathExtMatches[Count],
                                                Out,
                                                FullPath)) {
                    return FALSE;
                }
                if (!MatchAllCallback(Out, MatchAllContext)) {
                    return FALSE;
                }
                Out->StartOfString[0] = '\0';
            }
        }
        Out->StartOfString[0] = '\0';
        return TRUE;
    }

    //
    //  Based on the initial search criteria, decide what if anything to
    //  return
    //

    for (Count = 0; Count < PathExtCount; Count++) {
        if (PathExtData[Count].Found) {

            if (!YoriLibLocateBuildFullName(SearchPath,
                                            &PathExtMatches[Count],
                                            Out,
                                            FullPath)) {
                return FALSE;
            }

            break;
        }
    }

    return TRUE;
}

/**
 Perform a path search for a file which could have any path and could have any
 extension.

 @param SearchFor The file name to search for.

 @param PathVariable The contents of the environment variable to search through.

 @param MatchAllCallback Optional callback to be invoked on every single match.
        If not specified, this function returns the first match.

 @param MatchAllContext Optional context to pass to the callback, if it is
        specified.

 @param FoundPath If not specified, this string is updated to contain any
        matching file.

 @return TRUE to indicate the lookup was successful, and FALSE to indicate a
         lookup failure.  Success does not imply a match was found; if a
         lookup successfully found nothing, FoundPath will contain an empty
         string.
 */
__success(return)
BOOL
YoriLibPathLocateUnknownExtensionUnknownLocation(
    __in PYORI_STRING SearchFor,
    __in PYORI_STRING PathVariable,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING FoundPath
    )
{
    TCHAR * TokCtx;
    PYORI_PATHEXT_COMPONENT PathExtComponents;
    DWORD PathExtCount;
    LPTSTR ThisPath;
    YORI_STRING SearchPath;
    YORI_STRING ScratchArea;

    ASSERT(YoriLibIsStringNullTerminated(PathVariable));

    PathExtComponents = YoriLibPathBuildPathExtComponentList(&PathExtCount);
    if (PathExtComponents == NULL) {
        return FALSE;
    }

    //
    //  MSFIX Should probably be quote aware
    //

    //
    //  First, check the current directory.
    //

    YoriLibInitEmptyString(&ScratchArea);
    YoriLibConstantString(&SearchPath, _T("."));
    FoundPath->StartOfString[0] = '\0';
    FoundPath->LengthInChars = 0;

    if (!YoriLibLocateFileExtensionsInOnePath(SearchFor,
                                              &SearchPath,
                                              &ScratchArea,
                                              PathExtComponents,
                                              PathExtCount,
                                              MatchAllCallback,
                                              MatchAllContext,
                                              FoundPath,
                                              FALSE)) {
        YoriLibFreeStringContents(&ScratchArea);
        YoriLibPathFreePathExtComponents(PathExtComponents, PathExtCount);
        return FALSE;
    }

    //
    //  If we don't have a match, check each of the path components
    //  until we find one.
    //

    if (FoundPath->StartOfString[0] == '\0') {
        ThisPath = _tcstok_s(PathVariable->StartOfString, _T(";"), &TokCtx);
        while (ThisPath != NULL) {

            if (ThisPath[0] != '\0') {
                SearchPath.MemoryToFree = NULL;
                SearchPath.StartOfString = ThisPath;
                SearchPath.LengthInChars = _tcslen(ThisPath);
                SearchPath.LengthAllocated = SearchPath.LengthInChars + 1;

                FoundPath->LengthInChars = 0;

                if (!YoriLibLocateFileExtensionsInOnePath(SearchFor,
                                                          &SearchPath,
                                                          &ScratchArea,
                                                          PathExtComponents,
                                                          PathExtCount,
                                                          MatchAllCallback,
                                                          MatchAllContext,
                                                          FoundPath,
                                                          FALSE)) {
                    YoriLibFreeStringContents(&ScratchArea);
                    YoriLibPathFreePathExtComponents(PathExtComponents, PathExtCount);
                    return FALSE;
                }
     
                if (FoundPath->StartOfString[0] != '\0') {
                    break;
                }
            }

            ThisPath = _tcstok_s(NULL, _T(";"), &TokCtx);
        }
    }

    YoriLibPathFreePathExtComponents(PathExtComponents, PathExtCount);
    YoriLibFreeStringContents(&ScratchArea);
    return TRUE;
}

/**
 Perform a path search for a file with a known path that could have any
 extension.

 @param SearchFor The file name to search for.

 @param MatchAllCallback Optional callback to be invoked on every single match.
        If not specified, this function returns the first match.

 @param MatchAllContext Optional context to pass to the callback, if it is
        specified.

 @param FoundPath If not specified, this string is updated to contain any
        matching file.

 @return TRUE to indicate the lookup was successful, and FALSE to indicate a
         lookup failure.  Success does not imply a match was found; if a
         lookup successfully found nothing, FoundPath will contain an empty
         string.
 */
__success(return)
BOOL
YoriLibPathLocateUnknownExtensionKnownLocation(
    __in PYORI_STRING SearchFor,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING FoundPath
    )
{
    PYORI_PATHEXT_COMPONENT PathExtComponents;
    DWORD PathExtCount;
    YORI_STRING FileNameToFind;
    YORI_STRING DirectoryToSearch;
    YORI_STRING ScratchArea;
    DWORD PathSeperator;

    PathExtComponents = YoriLibPathBuildPathExtComponentList(&PathExtCount);
    if (PathExtComponents == NULL) {
        return FALSE;
    }

    YoriLibInitEmptyString(&DirectoryToSearch);
    DirectoryToSearch.StartOfString = SearchFor->StartOfString;
    DirectoryToSearch.LengthInChars = SearchFor->LengthInChars;

    for (PathSeperator = DirectoryToSearch.LengthInChars - 1;
         TRUE;
         PathSeperator--) {

        if (YoriLibIsSep(DirectoryToSearch.StartOfString[PathSeperator]) ||
            DirectoryToSearch.StartOfString[PathSeperator] == ':') {

            YoriLibInitEmptyString(&FileNameToFind);
            FileNameToFind.StartOfString = &DirectoryToSearch.StartOfString[PathSeperator + 1];
            FileNameToFind.LengthInChars = DirectoryToSearch.LengthInChars - PathSeperator - 1;

            //
            //  If the seperator is a slash, and it's in the middle of a
            //  path specification, remove it.  If the seperator is a slash
            //  but it indicates the root of a drive, or if it's a colon,
            //  retain it.
            //

            if (YoriLibIsSep(DirectoryToSearch.StartOfString[PathSeperator])) {
                if (PathSeperator == 2 &&
                    YoriLibIsDriveLetterWithColonAndSlash(&DirectoryToSearch)) {

                    DirectoryToSearch.LengthInChars = PathSeperator + 1;
                } else if (PathSeperator == 6 &&
                           YoriLibIsPrefixedDriveLetterWithColonAndSlash(&DirectoryToSearch)) {
                    DirectoryToSearch.LengthInChars = PathSeperator + 1;
                } else {
                    DirectoryToSearch.LengthInChars = PathSeperator;
                }
            } else {
                DirectoryToSearch.LengthInChars = PathSeperator + 1;
            }
            break;
        } else if (PathSeperator == 0) {
            ASSERT(PathSeperator > 0);
            YoriLibFree(PathExtComponents);
            return FALSE;
        }
    }

    YoriLibInitEmptyString(&ScratchArea);

    if (!YoriLibLocateFileExtensionsInOnePath(&FileNameToFind,
                                              &DirectoryToSearch,
                                              &ScratchArea,
                                              PathExtComponents,
                                              PathExtCount,
                                              MatchAllCallback,
                                              MatchAllContext,
                                              FoundPath,
                                              FALSE)) {

        YoriLibFreeStringContents(&ScratchArea);
        YoriLibPathFreePathExtComponents(PathExtComponents, PathExtCount);
        return FALSE;
    }

    YoriLibFreeStringContents(&ScratchArea);
    YoriLibPathFreePathExtComponents(PathExtComponents, PathExtCount);
    return TRUE;
}

/**
 Perform a path search for a file with a known extension that could be in any
 location.

 @param SearchFor The file name to search for.

 @param PathVariable The contents of the environment variable to search through.

 @param MatchAllCallback Optional callback to be invoked on every single match.
        If not specified, this function returns the first match.

 @param MatchAllContext Optional context to pass to the callback, if it is
        specified.

 @param FoundPath If not specified, this string is updated to contain any
        matching file.

 @return TRUE to indicate the lookup was successful, and FALSE to indicate a
         lookup failure.  Success does not imply a match was found; if a
         lookup successfully found nothing, FoundPath will contain an empty
         string.
 */
__success(return)
BOOL
YoriLibPathLocateKnownExtensionUnknownLocation(
    __in PYORI_STRING SearchFor,
    __in PYORI_STRING PathVariable,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING FoundPath
    )
{

    YORI_STRING ScratchArea;

    if (!YoriLibAllocateString(&ScratchArea, SearchFor->LengthAllocated + 256)) {
        return FALSE;
    }

    //
    //  If we have a fully specified extension, just look for it directly.
    //

    YoriLibSearchEnv(SearchFor, PathVariable, &ScratchArea, MatchAllCallback, MatchAllContext, FoundPath, FALSE);
    YoriLibFreeStringContents(&ScratchArea);
    return TRUE;
}


/**
 Search for a file name within the path.  If it's found, output the string
 matching.  The caller is expected to free this string with
 @ref YoriLibDereference .

 @param SearchFor The file name to search for.

 @param MatchAllCallback An optional callback to invoke each time a
        candidate match is found.

 @param MatchAllContext Context information to supply to MatchAllCallback
        if it is specified.

 @param PathName On successful completion, updated to point to a newly
        allocated string matching the next match.

 @return TRUE to indicate a match was found, FALSE if it was not.
 */
__success(return)
BOOL
YoriLibLocateExecutableInPath(
    __in PYORI_STRING SearchFor,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __out _When_(MatchAllCallback != NULL, _Post_invalid_) PYORI_STRING PathName
    )
{
    BOOL SearchPathExt = TRUE;
    BOOL SearchPath = TRUE;
    YORI_STRING FoundPath;
    YORI_STRING PathData;
    DWORD PathLength;
    DWORD CurDirLength;

    ASSERT(YoriLibIsStringNullTerminated(SearchFor));

    //
    //  We have four cases:
    //  1. The path is fully specified, and we have nothing to do.
    //  2. There is a path component, but no extension, so we need
    //     to search PATHEXT only.
    //  3. There is an extension, but no path component, so we need
    //     to search PATH only.
    //  4. There is neither a path component nor extension, so we
    //     need to search PATH and PATHEXT.
    //
    //  To achieve this we scan backwards through the name, looking for a
    //  period or seperator.  If we see a period before any seperator, we
    //  have an extension; if we see any seperator anywhere we have a path.
    //

    PathLength = SearchFor->LengthInChars;
    while (PathLength > 0) {
        PathLength--;
        if (SearchFor->StartOfString[PathLength] == '.') {

            if (SearchPath) {
                SearchPathExt = FALSE;
            }

        } else if (YoriLibIsSep(SearchFor->StartOfString[PathLength]) ||
                   SearchFor->StartOfString[PathLength] == ':') {

            SearchPath = FALSE;
            break;
        }
    }

    YoriLibInitEmptyString(&FoundPath);

    //
    //  If we're not searching PATH or PATHEXT, then just check if the string
    //  resolves to anything.  If it does, return the string so as to indicate
    //  it can be executed.  If it doesn't, try appending the PathExt
    //  extensions to it and see if it's located.
    //

    if (!SearchPath && !SearchPathExt) {

        HANDLE hFind;
        DWORD Index;
        WIN32_FIND_DATA FindData;
        YORI_STRING SearchDirectory;
        YORI_STRING FoundFile;

        hFind = FindFirstFile(SearchFor->StartOfString, &FindData);

        if (hFind != INVALID_HANDLE_VALUE) {

            FindClose(hFind);

            //
            //  Search backwards to find if there's a seperator.  That char
            //  and all before it are the directory part.
            //

            SearchDirectory.StartOfString = SearchFor->StartOfString;
            SearchDirectory.LengthInChars = 0;
            for (Index = SearchFor->LengthInChars; Index > 0; Index--) {
                if (YoriLibIsSep(SearchFor->StartOfString[Index - 1])) {
                    SearchDirectory.LengthInChars = Index;
                    break;
                }
            }

            //
            //  Take the file part from the enumerate.
            //

            YoriLibConstantString(&FoundFile, FindData.cFileName);

            if (!YoriLibAllocateString(PathName, SearchDirectory.LengthInChars + FoundFile.LengthInChars + 1)) {
                return FALSE;
            }
            PathName->LengthInChars = YoriLibSPrintf(PathName->StartOfString, _T("%y%y"), &SearchDirectory, &FoundFile);
            if (MatchAllCallback) {
                if (!MatchAllCallback(PathName, MatchAllContext)) {
                    YoriLibFreeStringContents(PathName);
                    return FALSE;
                } else {
                    YoriLibFreeStringContents(PathName);
                    return TRUE;
                }
            }
            return TRUE;
        }
        SearchPathExt = TRUE;
    }

    YoriLibInitEmptyString(PathName);

    //
    //  The worst case file name length is the size of the prefix, plus the
    //  PATH variable or current directory, a seperator, and the longest
    //  possible file name component in Windows, which is 256.
    //

    CurDirLength = GetCurrentDirectory(0, NULL);
    PathLength = GetEnvironmentVariable(_T("PATH"), NULL, 0);
    if (PathLength < CurDirLength + 1) {
        PathLength = CurDirLength + 1;
    }
    if (PathLength < MAX_PATH) {
        PathLength = MAX_PATH;
    }

    PathLength += sizeof("\\\\?\\");

    if (!YoriLibAllocateString(&FoundPath, PathLength + 256)) {
        return FALSE;
    }

    //
    //  The contents of the PATH environment variable.
    //

    if (!YoriLibAllocateString(&PathData, PathLength)) {
        YoriLibFreeStringContents(&FoundPath);
        return FALSE;
    }

    PathData.StartOfString[0] = '\0';
    PathData.LengthInChars = GetEnvironmentVariable(_T("PATH"), PathData.StartOfString, PathData.LengthAllocated);
    
    if (SearchPath && SearchPathExt) {
        if (!YoriLibPathLocateUnknownExtensionUnknownLocation(SearchFor, &PathData, MatchAllCallback, MatchAllContext, &FoundPath)) {
            YoriLibFreeStringContents(&FoundPath);
            YoriLibFreeStringContents(&PathData);
            return FALSE;
        }
    } else if (SearchPath) {

        if (!YoriLibPathLocateKnownExtensionUnknownLocation(SearchFor, &PathData, MatchAllCallback, MatchAllContext, &FoundPath)) {
            YoriLibFreeStringContents(&FoundPath);
            YoriLibFreeStringContents(&PathData);
            return FALSE;
        }

        if (MatchAllCallback != NULL || FoundPath.StartOfString[0] == '\0') {
            if (!YoriLibPathLocateUnknownExtensionUnknownLocation(SearchFor, &PathData, MatchAllCallback, MatchAllContext, &FoundPath)) {
                YoriLibFreeStringContents(&FoundPath);
                YoriLibFreeStringContents(&PathData);
                return FALSE;
            }
        }

    } else if (SearchPathExt) {

        if (!YoriLibPathLocateUnknownExtensionKnownLocation(SearchFor, MatchAllCallback, MatchAllContext, &FoundPath)) {
            YoriLibFreeStringContents(&FoundPath);
            YoriLibFreeStringContents(&PathData);
            return FALSE;
        }
    }

    YoriLibFreeStringContents(&PathData);

    if (MatchAllCallback != NULL) {
        YoriLibFreeStringContents(&FoundPath);
    } else {
        memcpy(PathName, &FoundPath, sizeof(YORI_STRING));
        ASSERT(YoriLibIsStringNullTerminated(PathName));
    }
    return TRUE;
}

// vim:sw=4:ts=4:et:
