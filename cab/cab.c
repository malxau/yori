/**
 * @file cab/cab.c
 *
 * Yori shell compress and uncompress archives
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

/**
 Help text to display to the user.
 */
const
CHAR strCabHelpText[] =
        "\n"
        "Compresses files into CAB files or extracts files from CAB files.\n"
        "\n"
        "CAB [-license] [-b] [-s] [-c|-u] <cabfile>\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Compress files into an archive\n"
        "   -s             Copy subdirectories as well as files\n"
        "   -u             Uncompress files from an archive\n";

/**
 Display usage text to the user.
 */
BOOL
CabHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Cab %i.%i\n"), CAB_VER_MAJOR, CAB_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strCabHelpText);
    return TRUE;
}


/**
 A single item to exclude or include.  Note this can refer to multiple files.
 */
typedef struct _CAB_MATCH_ITEM {

    /**
     List of items to match.
     */
    YORI_LIST_ENTRY MatchList;

    /**
     A string describing the object to match, which may include
     wildcards.
     */
    YORI_STRING MatchCriteria;
} CAB_MATCH_ITEM, *PCAB_MATCH_ITEM;

/**
 Context passed between the source package creation operation and every file
 found while creating the source package.
 */
typedef struct _CAB_CREATE_CONTEXT {

    /**
     A handle to the Cabinet being created.
     */
    PVOID CabHandle;

    /**
     A list of criteria to exclude.
     */
    YORI_LIST_ENTRY ExcludeList;

    /**
     A list of criteria to include, even if they have been excluded by the
     ExcludeList.
     */
    YORI_LIST_ENTRY IncludeList;
} CAB_CREATE_CONTEXT, *PCAB_CREATE_CONTEXT;

/**
 Add a new match criteria to the list.

 @param List Pointer to the list to add the match criteria to.

 @param NewCriteria Pointer to the new criteria to add, which may include
        wildcards.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CabCreateAddMatch(
    __in PYORI_LIST_ENTRY List,
    __in PYORI_STRING NewCriteria
    )
{
    PCAB_MATCH_ITEM MatchItem;
    MatchItem = YoriLibReferencedMalloc(sizeof(CAB_MATCH_ITEM) + (NewCriteria->LengthInChars + 1) * sizeof(TCHAR));

    if (MatchItem == NULL) {
        return FALSE;
    }

    ZeroMemory(MatchItem, sizeof(CAB_MATCH_ITEM));
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

 @param CreateContext Pointer to the create source context to free all
        exclude or include criteria from.
 */
VOID
CabCreateFreeMatchLists(
    __in PCAB_CREATE_CONTEXT CreateContext
    )
{
    PCAB_MATCH_ITEM MatchItem;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&CreateContext->ExcludeList, NULL);
    while (ListEntry != NULL) {
        MatchItem = CONTAINING_RECORD(ListEntry, CAB_MATCH_ITEM, MatchList);
        YoriLibRemoveListItem(&MatchItem->MatchList);
        YoriLibDereference(MatchItem);
        ListEntry = YoriLibGetNextListEntry(&CreateContext->ExcludeList, NULL);
    }

    ListEntry = YoriLibGetNextListEntry(&CreateContext->IncludeList, NULL);
    while (ListEntry != NULL) {
        MatchItem = CONTAINING_RECORD(ListEntry, CAB_MATCH_ITEM, MatchList);
        YoriLibRemoveListItem(&MatchItem->MatchList);
        YoriLibDereference(MatchItem);
        ListEntry = YoriLibGetNextListEntry(&CreateContext->IncludeList, NULL);
    }
}

/**
 Returns TRUE to indicate that an object should be excluded based on the
 exclude criteria, or FALSE if it should be included.

 @param CreateContext Pointer to the create source context to check the
        new object against.

 @param RelativePath Pointer to a string describing the file relative
        to the root of the source of the create operation.

 @return TRUE to exclude the file, FALSE to include it.
 */
BOOL
CabCreateShouldExclude(
    __in PCAB_CREATE_CONTEXT CreateContext,
    __in PYORI_STRING RelativePath
    )
{
    PCAB_MATCH_ITEM MatchItem;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&CreateContext->ExcludeList, NULL);
    while (ListEntry != NULL) {
        MatchItem = CONTAINING_RECORD(ListEntry, CAB_MATCH_ITEM, MatchList);
        if (YoriLibDoesFileMatchExpression(RelativePath, &MatchItem->MatchCriteria)) {

            ListEntry = YoriLibGetNextListEntry(&CreateContext->IncludeList, NULL);
            while (ListEntry != NULL) {
                MatchItem = CONTAINING_RECORD(ListEntry, CAB_MATCH_ITEM, MatchList);
                if (YoriLibDoesFileMatchExpression(RelativePath, &MatchItem->MatchCriteria)) {
                    return FALSE;
                }
                ListEntry = YoriLibGetNextListEntry(&CreateContext->IncludeList, ListEntry);
            }
            return TRUE;
        }
        ListEntry = YoriLibGetNextListEntry(&CreateContext->ExcludeList, ListEntry);
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
CabCreateFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PCAB_CREATE_CONTEXT CreateContext = (PCAB_CREATE_CONTEXT)Context;
    YORI_STRING RelativePathFrom;
    DWORD SlashesFound;
    DWORD Index;

    UNREFERENCED_PARAMETER(FileInfo);

    YoriLibInitEmptyString(&RelativePathFrom);

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

    RelativePathFrom.StartOfString = &FilePath->StartOfString[Index];
    RelativePathFrom.LengthInChars = FilePath->LengthInChars - Index;

    //
    //  Skip anything that should be excluded
    //

    if (CabCreateShouldExclude(CreateContext, &RelativePathFrom)) {
        return TRUE;
    }

    if (!YoriLibAddFileToCab(CreateContext->CabHandle, FilePath, &RelativePathFrom)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibAddFileToCab cannot add %y\n"), &RelativePathFrom);
    }

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the cab builtin command.
 */
#define ENTRYPOINT YoriCmd_CAB
#else
/**
 The main entrypoint for the cab standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the cab cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    BOOL Compress = FALSE;
    BOOL Uncompress = FALSE;
    BOOL Recursive = FALSE;
    BOOL BasicEnumeration = FALSE;
    DWORD i;
    DWORD StartArg = 1;
    YORI_STRING Arg;
    PYORI_STRING CabFileName;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                CabHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                Compress = TRUE;
                Uncompress = FALSE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("u")) == 0) {
                Compress = FALSE;
                Uncompress = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartArg = i + 1;
                break;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }


    i = StartArg;

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("cab: missing archive\n"));
        return EXIT_FAILURE;
    }

    if (!Compress && !Uncompress) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("cab: missing operation\n"));
        return EXIT_FAILURE;
    }

    if (Compress) {
        CAB_CREATE_CONTEXT CreateContext;
        DWORD MatchFlags;

        if (StartArg + 1 >= ArgC) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("cab: missing source files\n"));
            return EXIT_FAILURE;
        }

        CabFileName = &ArgV[StartArg];

        ZeroMemory(&CreateContext, sizeof(CreateContext));
        YoriLibInitializeListHead(&CreateContext.ExcludeList);
        YoriLibInitializeListHead(&CreateContext.IncludeList);

        if (!YoriLibCreateCab(CabFileName, &CreateContext.CabHandle)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibCreateCab failure\n"));
            return FALSE;
        }

        MatchFlags = YORILIB_FILEENUM_RETURN_FILES;
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }
        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_AFTER_RETURN;
        }

        for (i = StartArg + 1; i < ArgC; i++) {
            YoriLibForEachFile(&ArgV[i],
                               MatchFlags,
                               0,
                               CabCreateFileFoundCallback,
                               &CreateContext);
        }
        YoriLibCloseCab(CreateContext.CabHandle);
        CabCreateFreeMatchLists(&CreateContext);
    } else {
        YORI_STRING FullTargetDirectory;
        YORI_STRING TargetDirectory;
        YORI_STRING ErrorString;

        CabFileName = &ArgV[StartArg];

        YoriLibConstantString(&TargetDirectory, _T("."));
        if (!YoriLibUserStringToSingleFilePath(&TargetDirectory, FALSE, &FullTargetDirectory)) {
            return EXIT_FAILURE;
        }

        YoriLibInitEmptyString(&ErrorString);

        if (!YoriLibExtractCab(CabFileName, &FullTargetDirectory, TRUE, 0, NULL, 0, NULL, NULL, NULL, NULL, &ErrorString)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("YoriLibExtractCab failed on %y: %y\n"), CabFileName, &ErrorString);
            YoriLibFreeStringContents(&ErrorString);
            YoriLibFreeStringContents(&FullTargetDirectory);
            return EXIT_FAILURE;
        }
        YoriLibFreeStringContents(&FullTargetDirectory);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
