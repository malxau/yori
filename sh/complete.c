/**
 * @file sh/complete.c
 *
 * Yori shell tab completion
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

#include "yori.h"

/**
 Populates the list of matches for a command history tab completion.  This
 function searches the history for matching commands in MRU order and
 populates the list with the result.

 @param TabContext Pointer to the tab completion context.  This provides
        the search criteria and has its match list populated with results
        on success.

 @param ExpandFullPath Specifies if full path expansion should be performed.
        For executable matches, full path expansion is always performed.
 */
VOID
YoriShPerformHistoryTabCompletion(
    __inout PYORI_TAB_COMPLETE_CONTEXT TabContext,
    __in BOOL ExpandFullPath
    )
{
    LPTSTR FoundPath;
    DWORD CompareLength;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_HISTORY_ENTRY HistoryEntry;
    PYORI_TAB_COMPLETE_MATCH Match;

    UNREFERENCED_PARAMETER(ExpandFullPath);

    //
    //  Set up state necessary for different types of searching.
    //

    FoundPath = _tcschr(TabContext->SearchString.StartOfString, '*');
    if (FoundPath != NULL) {
        CompareLength = (DWORD)(FoundPath - TabContext->SearchString.StartOfString);
    } else {
        CompareLength = TabContext->SearchString.LengthInChars;
    }
    FoundPath = NULL;

    //
    //  Search the list of history.
    //

    ListEntry = YoriLibGetPreviousListEntry(&YoriShCommandHistory, NULL);
    while (ListEntry != NULL) {
        HistoryEntry = CONTAINING_RECORD(ListEntry, YORI_HISTORY_ENTRY, ListEntry);

        if (YoriLibCompareStringInsensitiveCount(&HistoryEntry->CmdLine, &TabContext->SearchString, CompareLength) == 0) {

            //
            //  Allocate a match entry for this file.
            //
    
            Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (HistoryEntry->CmdLine.LengthInChars + 1) * sizeof(TCHAR));
            if (Match == NULL) {
                return;
            }
    
            //
            //  Populate the file into the entry.
            //
    
            YoriLibInitEmptyString(&Match->YsValue);
            Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
            YoriLibReference(Match);
            Match->YsValue.MemoryToFree = Match;
            YoriLibSPrintf(Match->YsValue.StartOfString, _T("%y"), &HistoryEntry->CmdLine);
            Match->YsValue.LengthInChars = HistoryEntry->CmdLine.LengthInChars;
    
            //
            //  Append to the list.
            //
    
            YoriLibAppendList(&TabContext->MatchList, &Match->ListEntry);
        }
        ListEntry = YoriLibGetPreviousListEntry(&YoriShCommandHistory, ListEntry);
    }
}

/**
 A callback function that is invoked by the path resolver to add any
 candidate programs to the tab completion list.

 @param FoundPath A match that was found when enumerating through the path.

 @param Context Pointer to the tab complete context to populate with the new
        match.
 */
BOOL
YoriShAddToTabList(
    __in PYORI_STRING FoundPath,
    __in PVOID Context
    )
{
    PYORI_TAB_COMPLETE_MATCH Match;
    PYORI_TAB_COMPLETE_MATCH Existing;
    PYORI_TAB_COMPLETE_CONTEXT TabContext = (PYORI_TAB_COMPLETE_CONTEXT)Context;
    PYORI_LIST_ENTRY ListEntry;


    //
    //  Allocate a match entry for this file.
    //

    Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (FoundPath->LengthInChars + 1) * sizeof(TCHAR));
    if (Match == NULL) {
        return FALSE;
    }

    //
    //  Populate the file into the entry.
    //

    YoriLibInitEmptyString(&Match->YsValue);
    Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
    YoriLibReference(Match);
    Match->YsValue.MemoryToFree = Match;
    memcpy(Match->YsValue.StartOfString, FoundPath->StartOfString, FoundPath->LengthInChars * sizeof(TCHAR));
    Match->YsValue.LengthInChars = FoundPath->LengthInChars;
    Match->YsValue.LengthAllocated = FoundPath->LengthInChars + 1;
    Match->YsValue.StartOfString[Match->YsValue.LengthInChars] = '\0';

    //
    //  Insert into the list if no duplicate is found.
    //

    ListEntry = YoriLibGetNextListEntry(&TabContext->MatchList, NULL);
    do {
        if (ListEntry == NULL) {
            YoriLibAppendList(&TabContext->MatchList, &Match->ListEntry);
            Match = NULL;
            break;
        }
        Existing = CONTAINING_RECORD(ListEntry, YORI_TAB_COMPLETE_MATCH, ListEntry);
        if (YoriLibCompareStringInsensitive(&Match->YsValue, &Existing->YsValue) == 0) {
            break;
        }
        ListEntry = YoriLibGetNextListEntry(&TabContext->MatchList, ListEntry);

    } while(TRUE);

    if (Match != NULL) {
        YoriLibFreeStringContents(&Match->YsValue);
        YoriLibDereference(Match);
    }

    return TRUE;
}


/**
 Populates the list of matches for an executable tab completion.  This
 function searches the path for matching binaries in execution order
 and populates the list with the result.

 @param TabContext Pointer to the tab completion context.  This provides
        the search criteria and has its match list populated with results
        on success.

 @param ExpandFullPath Specifies if full path expansion should be performed.
        For executable matches, full path expansion is always performed.
 */
VOID
YoriShPerformExecutableTabCompletion(
    __inout PYORI_TAB_COMPLETE_CONTEXT TabContext,
    __in BOOL ExpandFullPath
    )
{
    LPTSTR FoundPath;
    YORI_STRING FoundExecutable;
    BOOL Result;
    PYORI_TAB_COMPLETE_MATCH Match;
    YORI_STRING AliasStrings;
    DWORD CompareLength;

    UNREFERENCED_PARAMETER(ExpandFullPath);

    //
    //  Set up state necessary for different types of searching.
    //

    FoundPath = _tcschr(TabContext->SearchString.StartOfString, '*');
    if (FoundPath != NULL) {
        CompareLength = (DWORD)(FoundPath - TabContext->SearchString.StartOfString);
    } else {
        CompareLength = TabContext->SearchString.LengthInChars;
    }

    //
    //  Firstly, search the table of aliases.
    //

    YoriLibInitEmptyString(&AliasStrings);
    if (YoriShGetAliasStrings(TRUE, &AliasStrings)) {
        LPTSTR ThisAlias;
        LPTSTR AliasValue;
        DWORD AliasLength;
        DWORD AliasNameLength;


        ThisAlias = AliasStrings.StartOfString;
        while (*ThisAlias != '\0') {
            AliasNameLength = 
            AliasLength = _tcslen(ThisAlias);

            //
            //  Look at the alias name only, not what it maps to.
            //

            AliasValue = _tcschr(ThisAlias, '=');
            ASSERT(AliasValue != NULL);
            if (AliasValue != NULL) {
                *AliasValue = '\0';
                AliasNameLength = (DWORD)((PUCHAR)AliasValue - (PUCHAR)ThisAlias);
            }

            if (YoriLibCompareStringWithLiteralInsensitiveCount(&TabContext->SearchString, ThisAlias, CompareLength) == 0) {

                //
                //  Allocate a match entry for this file.
                //
        
                Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (AliasNameLength + 1) * sizeof(TCHAR));
                if (Match == NULL) {
                    YoriLibFreeStringContents(&AliasStrings);
                    return;
                }
        
                //
                //  Populate the file into the entry.
                //
        
                YoriLibInitEmptyString(&Match->YsValue);
                Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
                YoriLibReference(Match);
                Match->YsValue.MemoryToFree = Match;
                _tcscpy(Match->YsValue.StartOfString, ThisAlias);
                Match->YsValue.LengthInChars = AliasNameLength;
        
                //
                //  Append to the list.
                //
        
                YoriLibAppendList(&TabContext->MatchList, &Match->ListEntry);
            }

            //
            //  Move to the next alias
            //

            ThisAlias += AliasLength;
            ThisAlias++;
        }
        YoriLibFreeStringContents(&AliasStrings);
    }

    //
    //  Secondly, search for the object in the PATH, resuming after the
    //  previous search
    //

    YoriLibInitEmptyString(&FoundExecutable);
    Result = YoriLibLocateExecutableInPath(&TabContext->SearchString, YoriShAddToTabList, TabContext, &FoundExecutable);
    ASSERT(FoundExecutable.StartOfString == NULL);

    //
    //  Thirdly, search the table of builtins.
    //

    if (YoriShBuiltins != NULL) {
        LPTSTR ThisBuiltin;
        DWORD BuiltinLength;

        ThisBuiltin = YoriShBuiltins;
        while (*ThisBuiltin != '\0') {
            BuiltinLength = _tcslen(ThisBuiltin);

            if (YoriLibCompareStringWithLiteralInsensitiveCount(&TabContext->SearchString, ThisBuiltin, CompareLength) == 0) {

                //
                //  Allocate a match entry for this file.
                //
        
                Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (BuiltinLength + 1) * sizeof(TCHAR));
                if (Match == NULL) {
                    return;
                }
        
                //
                //  Populate the file into the entry.
                //
        
                YoriLibInitEmptyString(&Match->YsValue);
                Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
                YoriLibReference(Match);
                Match->YsValue.MemoryToFree = Match;
                _tcscpy(Match->YsValue.StartOfString, ThisBuiltin);
                Match->YsValue.LengthInChars = BuiltinLength;
        
                //
                //  Append to the list.
                //
        
                YoriLibAppendList(&TabContext->MatchList, &Match->ListEntry);
            }

            //
            //  Move to the next alias
            //

            ThisBuiltin += BuiltinLength;
            ThisBuiltin++;
        }
    }
}

/**
 Context information for a file based tab completion.
 */
typedef struct _YORI_FILE_COMPLETE_CONTEXT {

    /**
     The tab completion context to populate with any matches.
     */
    PYORI_TAB_COMPLETE_CONTEXT TabContext;

    /**
     Extra characters to include at the beginning of any found match.
     */
    YORI_STRING Prefix;

    /**
     The string to search for.
     */
    LPTSTR SearchString;

    /**
     The number of characters in the SearchString until the final slash.
     This is used to distinguish where to search from what to search for.
     */
    DWORD CharsToFinalSlash;

    /**
     The number of files that have been found.
     */
    DWORD FilesFound;

    /**
     If TRUE, the resulting tab completion should expand the entire path,
     if FALSE it should only expand the file name (inside the specified
     directory, if present.)
     */
    BOOL ExpandFullPath;

} YORI_FILE_COMPLETE_CONTEXT, *PYORI_FILE_COMPLETE_CONTEXT;

/**
 Invoked for each file matching a file based tab completion pattern.

 @param Filename Pointer to a string containing the full file name.

 @param FileInfo Pointer to the block of information returned by directory
        enumeration describing the entry.

 @param Depth Specifies the recursion depth.  Ignored in this function.

 @param Context Pointer to a context block describing the tab completion
        context to populate with the entry and properties for the population.

 @return TRUE to continue enumerating, FALSE to stop.
 */
BOOL
YoriShFileTabCompletionCallback(
    __in PYORI_STRING Filename,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PYORI_FILE_COMPLETE_CONTEXT Context
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_TAB_COMPLETE_MATCH Match;
    PYORI_TAB_COMPLETE_MATCH Existing;

    UNREFERENCED_PARAMETER(Depth);

    if (Context->ExpandFullPath) {

        //
        //  Allocate a match entry for this file.
        //

        Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (Context->Prefix.LengthInChars + Filename->LengthInChars + 1) * sizeof(TCHAR));
        if (Match == NULL) {
            return FALSE;
        }

        //
        //  Populate the file into the entry.
        //

        YoriLibInitEmptyString(&Match->YsValue);
        Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
        YoriLibReference(Match);
        Match->YsValue.MemoryToFree = Match;

        if (Context->Prefix.LengthInChars > 0) {
            memcpy(Match->YsValue.StartOfString, Context->Prefix.StartOfString, Context->Prefix.LengthInChars * sizeof(TCHAR));
        }
        memcpy(&Match->YsValue.StartOfString[Context->Prefix.LengthInChars], Filename->StartOfString, Filename->LengthInChars * sizeof(TCHAR));
        Match->YsValue.StartOfString[Context->Prefix.LengthInChars + Filename->LengthInChars] = '\0';
        Match->YsValue.LengthInChars = Context->Prefix.LengthInChars + Filename->LengthInChars;
    } else {
        DWORD CharsInFileName;
        CharsInFileName = _tcslen(FileInfo->cFileName);

        //
        //  Allocate a match entry for this file.
        //
    
        Match = YoriLibReferencedMalloc(sizeof(YORI_TAB_COMPLETE_MATCH) + (Context->Prefix.LengthInChars + Context->CharsToFinalSlash + CharsInFileName + 1) * sizeof(TCHAR));
        if (Match == NULL) {
            return FALSE;
        }
    
        //
        //  Populate the file into the entry.
        //
    
        YoriLibInitEmptyString(&Match->YsValue);
        Match->YsValue.StartOfString = (LPTSTR)(Match + 1);
        YoriLibReference(Match);
        Match->YsValue.MemoryToFree = Match;
    
        if (Context->Prefix.LengthInChars > 0) {
            memcpy(Match->YsValue.StartOfString, Context->Prefix.StartOfString, Context->Prefix.LengthInChars * sizeof(TCHAR));
        }
        if (Context->CharsToFinalSlash > 0) {
            memcpy(&Match->YsValue.StartOfString[Context->Prefix.LengthInChars], Context->SearchString, Context->CharsToFinalSlash * sizeof(TCHAR));
        }
        memcpy(&Match->YsValue.StartOfString[Context->Prefix.LengthInChars + Context->CharsToFinalSlash], FileInfo->cFileName, CharsInFileName * sizeof(TCHAR));
        Match->YsValue.StartOfString[Context->Prefix.LengthInChars + Context->CharsToFinalSlash + CharsInFileName] = '\0';
        Match->YsValue.LengthInChars = Context->Prefix.LengthInChars + Context->CharsToFinalSlash + CharsInFileName;
    }

    //
    //  Insert into the list in lexicographical order.
    //

    ListEntry = YoriLibGetNextListEntry(&Context->TabContext->MatchList, NULL);
    do {
        if (ListEntry == NULL) {
            YoriLibAppendList(&Context->TabContext->MatchList, &Match->ListEntry);
            break;
        }
        Existing = CONTAINING_RECORD(ListEntry, YORI_TAB_COMPLETE_MATCH, ListEntry);
        if (YoriLibCompareStringInsensitive(&Match->YsValue, &Existing->YsValue) < 0) {
            YoriLibAppendList(ListEntry, &Match->ListEntry);
            break;
        }
        ListEntry = YoriLibGetNextListEntry(&Context->TabContext->MatchList, ListEntry);
    } while(TRUE);

    Context->FilesFound++;

    return TRUE;
}

/**
 A structure describing a string which when encountered in a string used for
 file tab completion may indicate the existence of a file.
 */
typedef struct _YORI_TAB_FILE_HEURISTIC_MATCH {

    /**
     The string to match against.
     */
    LPCTSTR MatchString;

    /**
     The offset, from the beginning of the matched string, to where the file
     name would be.  Note this value can be negative, indicating a match of a
     string within a file name.
     */
    INT CharsToSkip;
} YORI_TAB_FILE_HEURISTIC_MATCH, *PYORI_TAB_FILE_HEURISTIC_MATCH;

/**
 A list of strings which may, heuristically, indicate a good place to look for
 file names.
 */
YORI_TAB_FILE_HEURISTIC_MATCH YoriShTabHeuristicMatches[] = {
    {_T(":\\"), -1},
    {_T("\\\\"), 0},
    {_T(">>"), 2},
    {_T(">"), 1},
    {_T(":"), 1},
    {_T("="), 1}
};

/**
 Find the final seperator or colon in event of a drive letter colon
 prefix string, such that the criteria being searched for can be
 seperated from the location of the search.

 @param String Pointer to the string to locate the final seperator
        in.

 @return The index of the seperator, which may be zero to indicate
         one was not found.
 */
DWORD
YoriShFindFinalSlashIfSpecified(
    __in PYORI_STRING String
    )
{
    DWORD CharsInFileName;

    CharsInFileName = String->LengthInChars;

    while (CharsInFileName > 0) {
        if (YoriLibIsSep(String->StartOfString[CharsInFileName - 1])) {

            break;
        }

        if (CharsInFileName == 2 &&
            YoriLibIsDriveLetterWithColon(String)) {

            break;
        }

        CharsInFileName--;
    }

    return CharsInFileName;
}

/**
 Populates the list of matches for a file based tab completion.  This
 function searches the path for matching files in lexicographic order
 and populates the list with the result.

 @param TabContext Pointer to the tab completion context.  This provides
        the search criteria and has its match list populated with results
        on success.

 @param ExpandFullPath Specifies if full path expansion should be performed.

 @param IncludeDirectories TRUE if directories should be included in results,
        FALSE if they should be ommitted.

 @param IncludeFiles TRUE if files should be included in results, FALSE if
        they should be ommitted (used for directory only results.)
 */
VOID
YoriShPerformFileTabCompletion(
    __inout PYORI_TAB_COMPLETE_CONTEXT TabContext,
    __in BOOL ExpandFullPath,
    __in BOOL IncludeDirectories,
    __in BOOL IncludeFiles
    )
{
    YORI_FILE_COMPLETE_CONTEXT EnumContext;
    YORI_STRING YsSearchString;
    DWORD PrefixLen;
    DWORD MatchFlags = 0;

    YoriLibInitEmptyString(&YsSearchString);
    YsSearchString.StartOfString = TabContext->SearchString.StartOfString;
    YsSearchString.LengthInChars = TabContext->SearchString.LengthInChars;
    YsSearchString.LengthAllocated = TabContext->SearchString.LengthAllocated;

    PrefixLen = sizeof("file:///") - 1;

    if (YoriLibCompareStringWithLiteralInsensitiveCount(&YsSearchString, _T("file:///"), PrefixLen) == 0) {
        YsSearchString.StartOfString += PrefixLen;
        YsSearchString.LengthInChars -= PrefixLen;
        YsSearchString.LengthAllocated -= PrefixLen;
    }

    EnumContext.ExpandFullPath = ExpandFullPath;
    EnumContext.CharsToFinalSlash = YoriShFindFinalSlashIfSpecified(&YsSearchString);
    EnumContext.SearchString = YsSearchString.StartOfString;
    YoriLibInitEmptyString(&EnumContext.Prefix);
    EnumContext.TabContext = TabContext;
    EnumContext.FilesFound = 0;

    if (IncludeFiles) {
        MatchFlags |= YORILIB_FILEENUM_RETURN_FILES;
    }
    if (IncludeDirectories) {
        MatchFlags |= YORILIB_FILEENUM_RETURN_DIRECTORIES;
    }

    if (MatchFlags == 0) {
        return;
    }

    YoriLibForEachFile(&YsSearchString, MatchFlags, 0, YoriShFileTabCompletionCallback, &EnumContext);

    if (EnumContext.FilesFound == 0) {
        DWORD Count = sizeof(YoriShTabHeuristicMatches)/sizeof(YoriShTabHeuristicMatches[0]);
        DWORD Index;
        DWORD StringOffsetOfMatch;
        PYORI_STRING MatchArray;
        PYORI_STRING FoundMatch;

        MatchArray = YoriLibMalloc(sizeof(YORI_STRING) * Count);
        if (MatchArray == NULL) {
            return;
        }

        for (Index = 0; Index < Count; Index++) {
            YoriLibConstantString(&MatchArray[Index], YoriShTabHeuristicMatches[Index].MatchString);
        }

        FoundMatch = YoriLibFindFirstMatchingSubstring(&YsSearchString, Count, MatchArray, &StringOffsetOfMatch);
        if (FoundMatch == NULL) {
            YoriLibFree(MatchArray);
            return;
        }

        for (Index = 0; Index < Count; Index++) {
            if (FoundMatch->StartOfString == MatchArray[Index].StartOfString) {
                break;
            }
        }

        if (Index == Count) {
            ASSERT(Index != Count);
            YoriLibFree(MatchArray);
            return;
        }

        //
        //  If the file would begin before the beginning of the string, stop.
        //

        if (YoriShTabHeuristicMatches[Index].CharsToSkip < 0 &&
            (DWORD)(-1 * YoriShTabHeuristicMatches[Index].CharsToSkip) > YsSearchString.LengthInChars) {

            YoriLibFree(MatchArray);
            return;
        }

        //
        //  If the file would begin beyond the end of the string, stop.
        //

        if (YoriShTabHeuristicMatches[Index].CharsToSkip > 0 &&
            StringOffsetOfMatch + (DWORD)YoriShTabHeuristicMatches[Index].CharsToSkip >= YsSearchString.LengthInChars) {

            YoriLibFree(MatchArray);
            return;
        }

        //
        //  Seperate the string between the file portion (that we're looking
        //  for) and a prefix to append to any match.
        //

        EnumContext.Prefix.StartOfString = YsSearchString.StartOfString;
        EnumContext.Prefix.LengthInChars = StringOffsetOfMatch + YoriShTabHeuristicMatches[Index].CharsToSkip;

        YsSearchString.StartOfString += EnumContext.Prefix.LengthInChars;
        YsSearchString.LengthInChars -= EnumContext.Prefix.LengthInChars;

        EnumContext.CharsToFinalSlash = YoriShFindFinalSlashIfSpecified(&YsSearchString);
        EnumContext.SearchString = YsSearchString.StartOfString;

        YoriLibForEachFile(&YsSearchString, YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES, 0, YoriShFileTabCompletionCallback, &EnumContext);

        YoriLibFree(MatchArray);
    }
    return;
}

/**
 A context describing the actions that can be performed in response to a
 completion within a command argument.
 */
typedef struct _YORI_SH_ARG_TAB_COMPLETION_ACTION {

    /**
     The type of action to perform for argument completion.
     */
    enum {
        CompletionActionTypeFilesAndDirectories = 1,
        CompletionActionTypeFiles = 2,
        CompletionActionTypeDirectories = 3,
        CompletionActionTypeInsensitiveList = 4,
        CompletionActionTypeSensitiveList = 5
    } CompletionAction;
} YORI_SH_ARG_TAB_COMPLETION_ACTION, *PYORI_SH_ARG_TAB_COMPLETION_ACTION;

/**
 Check for the given executable or builtin command how to expand its arguments.
 
 @param Executable Pointer to the executable or builtin command.  Note this
        is a fully qualified path on entry.

 @param CurrentArg Specifies the index of the argument being completed for this
        command.

 @param Action On successful completion, populated with the action to perform
        for this command.
 */
BOOL
YoriShResolveTabCompletionActionForExecutable(
    __in PYORI_STRING Executable,
    __in DWORD CurrentArg,
    __out PYORI_SH_ARG_TAB_COMPLETION_ACTION Action
    )
{
    YORI_STRING FilePartOnly;
    DWORD FinalSeperator;

    UNREFERENCED_PARAMETER(CurrentArg);
    FinalSeperator = YoriShFindFinalSlashIfSpecified(Executable);

    YoriLibInitEmptyString(&FilePartOnly);
    FilePartOnly.StartOfString = &Executable->StartOfString[FinalSeperator];
    FilePartOnly.LengthInChars = Executable->LengthInChars - FinalSeperator;

    if (FilePartOnly.LengthInChars == 0) {
        Action->CompletionAction = CompletionActionTypeFilesAndDirectories;
        return TRUE;
    }

    if (YoriLibCompareStringWithLiteralInsensitive(&FilePartOnly, _T("CHDIR")) == 0) {
        Action->CompletionAction = CompletionActionTypeDirectories;
    } else if (YoriLibCompareStringWithLiteralInsensitive(&FilePartOnly, _T("CHDIR.COM")) == 0) {
        Action->CompletionAction = CompletionActionTypeDirectories;
    } else if (YoriLibCompareStringWithLiteralInsensitive(&FilePartOnly, _T("YMKDIR.EXE")) == 0) {
        Action->CompletionAction = CompletionActionTypeDirectories;
    } else if (YoriLibCompareStringWithLiteralInsensitive(&FilePartOnly, _T("YRMDIR.EXE")) == 0) {
        Action->CompletionAction = CompletionActionTypeDirectories;
    } else {
        Action->CompletionAction = CompletionActionTypeFilesAndDirectories;
    }
    return TRUE;
}

/**
 Populates the list of matches for a command argument based tab completion.
 This function uses command specific patterns to determine how to complete
 arguments.

 @param TabContext Pointer to the tab completion context.  This provides
        the search criteria and has its match list populated with results
        on success.

 @param ExpandFullPath Specifies if full path expansion should be performed.

 @param SrcCmdContext Pointer to a caller allocated command context since
        basic arguments have already been parsed.  Note this should not be
        modified in this routine (that's for the caller.)
 */
VOID
YoriShPerformArgumentTabCompletion(
    __inout PYORI_TAB_COMPLETE_CONTEXT TabContext,
    __in BOOL ExpandFullPath,
    __in PYORI_CMD_CONTEXT SrcCmdContext
    )
{
    YORI_CMD_CONTEXT CmdContext;
    YORI_SH_ARG_TAB_COMPLETION_ACTION CompletionAction;
    BOOL ExecutableFound;

    //
    //  MSFIX Things to do:
    //  1. Need to take the active character and locate which backquote
    //     scope it's in.  Note that "normal" backquote evaluation is
    //     left to right.  We can then calculate cursor position within
    //     the substring.
    //  2. Resolve the current string into a command context, capturing
    //     the current argument.  Note we still need to remember any
    //     "surrounding" backquote scope.
    //  3. Parse the command context into an exec plan, and capture which
    //     exec context contains the current argument.  Note the answer
    //     may be none, since the argument might be a seperator.  This
    //     should return which argument within the exec context is the
    //     thing we're trying to complete too, which may be none due to
    //     things like redirection, so we're in one command but not in
    //     its arguments.
    //  4. If we're looking at the first thing in an exec context, we
    //     probably should do executable completion.
    //  5. If we're an arg within an exec context, take the first thing
    //     from the exec context, resolve aliases, resolve path, and
    //     we should then either be looking at a located executable or
    //     be searching for a builtin.  Look for a matching script to
    //     handle this command.
    //  6. Note when performing any substitution we should just use the
    //     command context from #2 (we already know the active argument)
    //     and put this back together with backquote scope from 1.
    //
    //  Things to consider in the script-to-shell language:
    //  1. Insensitive?
    //  2. Match files only
    //  3. Match directories only
    //  4. Match files or directories
    //  5. Match list
    //


    if (!YoriShCopyCmdContext(&CmdContext, SrcCmdContext)) {
        return;
    }

    //
    //  Currently the caller won't call here for argument zero, and this
    //  routine doesn't do anything smart enough to discover it's really
    //  on argument zero, so everything is always an argument.
    //

    ASSERT(CmdContext.CurrentArg > 0);

    if (!YoriShResolveCommandToExecutable(&CmdContext, &ExecutableFound)) {
        YoriShFreeCmdContext(&CmdContext);
        return;
    }

    if (!YoriShResolveTabCompletionActionForExecutable(&CmdContext.ysargv[0], CmdContext.CurrentArg, &CompletionAction)) {
        YoriShFreeCmdContext(&CmdContext);
        return;
    }

    switch(CompletionAction.CompletionAction) {
        case CompletionActionTypeFilesAndDirectories:
            YoriShPerformFileTabCompletion(TabContext, ExpandFullPath, TRUE, TRUE);
            break;
        case CompletionActionTypeFiles:
            YoriShPerformFileTabCompletion(TabContext, ExpandFullPath, FALSE, TRUE);
            break;
        case CompletionActionTypeDirectories:
            YoriShPerformFileTabCompletion(TabContext, ExpandFullPath, TRUE, FALSE);
            break;
    }


    YoriShFreeCmdContext(&CmdContext);
}

/**
 Perform tab completion processing.  On error the buffer is left unchanged.

 @param Buffer Pointer to the current input context.

 @param ExpandFullPath If TRUE, the path should be expanded to contain a fully
        specified path.  If FALSE, a minimal or relative path should be used.

 @param SearchHistory Specifies that tab completion should search through
        command history for matches rather than executable matches.
 */
VOID
YoriShTabCompletion(
    __inout PYORI_INPUT_BUFFER Buffer,
    __in BOOL ExpandFullPath,
    __in BOOL SearchHistory
    )
{
    DWORD SearchLength;
    YORI_CMD_CONTEXT CmdContext;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_TAB_COMPLETE_MATCH Match;

    Buffer->TabContext.TabCount++;
    if (Buffer->String.LengthInChars == 0) {
        return;
    }
    if (!YoriShParseCmdlineToCmdContext(&Buffer->String, Buffer->CurrentOffset, &CmdContext)) {
        return;
    }

    if (CmdContext.argc == 0) {
        YoriShFreeCmdContext(&CmdContext);
        return;
    }

    //
    //  If we're searching for the first time, set up the search
    //  criteria and populate the list of matches.
    //

    if (Buffer->TabContext.TabCount == 1) {

        YORI_STRING CurrentArgString;

        YoriLibInitEmptyString(&CurrentArgString);

        YoriLibInitializeListHead(&Buffer->TabContext.MatchList);
        Buffer->TabContext.PreviousMatch = NULL;

        if (CmdContext.CurrentArg < CmdContext.argc) {
            memcpy(&CurrentArgString, &CmdContext.ysargv[CmdContext.CurrentArg], sizeof(YORI_STRING));
        }
        SearchLength = CurrentArgString.LengthInChars + 1;
        if (!YoriLibAllocateString(&Buffer->TabContext.SearchString, SearchLength + 1)) {
            YoriShFreeCmdContext(&CmdContext);
            return;
        }
    
        Buffer->TabContext.SearchString.LengthInChars = YoriLibSPrintfS(Buffer->TabContext.SearchString.StartOfString, SearchLength + 1, _T("%y*"), &CurrentArgString);

        if (CmdContext.CurrentArg == 0) {

            if (SearchHistory) {
                Buffer->TabContext.SearchType = YoriTabCompleteSearchHistory;
            } else if (!YoriShDoesExpressionSpecifyPath(&CmdContext.ysargv[0])) {
                Buffer->TabContext.SearchType = YoriTabCompleteSearchExecutables;
            } else {
                Buffer->TabContext.SearchType = YoriTabCompleteSearchFiles;
            }

        } else {
            Buffer->TabContext.SearchType = YoriTabCompleteSearchArguments;
        }

        if (Buffer->TabContext.SearchType == YoriTabCompleteSearchExecutables) {
            YoriShPerformExecutableTabCompletion(&Buffer->TabContext, ExpandFullPath);
        } else if (Buffer->TabContext.SearchType == YoriTabCompleteSearchHistory) {
            YoriShPerformHistoryTabCompletion(&Buffer->TabContext, ExpandFullPath);
        } else if (Buffer->TabContext.SearchType == YoriTabCompleteSearchArguments) {
            YoriShPerformArgumentTabCompletion(&Buffer->TabContext, ExpandFullPath, &CmdContext);
        } else {
            YoriShPerformFileTabCompletion(&Buffer->TabContext, ExpandFullPath, TRUE, TRUE);
        }
    }

    //
    //  Check if we have any match.  If we do, try to use it.  If not, leave
    //  the buffer unchanged.
    //

    ListEntry = YoriLibGetNextListEntry(&Buffer->TabContext.MatchList, &Buffer->TabContext.PreviousMatch->ListEntry);
    if (ListEntry == NULL) {
        if (Buffer->TabContext.TabCount != 1) {
            ListEntry = YoriLibGetNextListEntry(&Buffer->TabContext.MatchList, NULL);
        }
    }
    if (ListEntry == NULL) {
        YoriShFreeCmdContext(&CmdContext);
        return;
    }

    Match = CONTAINING_RECORD(ListEntry, YORI_TAB_COMPLETE_MATCH, ListEntry);
    Buffer->TabContext.PreviousMatch = Match;

    {
        DWORD BeginCurrentArg;
        DWORD EndCurrentArg;
        LPTSTR NewString;
        BOOL FreeNewString = FALSE;
        DWORD NewStringLen;

        //
        //  MSFIX This isn't updating the referenced memory.  This works because
        //  we'll free the "correct" one and not the one we just put here, but
        //  it seems dodgy.
        //

        if (Buffer->TabContext.SearchType != YoriTabCompleteSearchHistory) {
            PYORI_STRING OldArgv = NULL;
            PYORI_ARG_CONTEXT OldArgContext = NULL;
            DWORD OldArgCount = 0;

            if (CmdContext.CurrentArg >= CmdContext.argc) {
                DWORD Count;

                OldArgCount = CmdContext.argc;
                OldArgv = CmdContext.ysargv;
                OldArgContext = CmdContext.ArgContexts;

                CmdContext.ysargv = YoriLibMalloc((CmdContext.CurrentArg + 1) * (sizeof(YORI_STRING) + sizeof(YORI_ARG_CONTEXT)));
                if (CmdContext.ysargv == NULL) {
                    YoriShFreeCmdContext(&CmdContext);
                    return;
                }

                CmdContext.argc = CmdContext.CurrentArg + 1;
                ZeroMemory(CmdContext.ysargv, CmdContext.argc * (sizeof(YORI_STRING) + sizeof(YORI_ARG_CONTEXT)));
                CmdContext.ArgContexts = (PYORI_ARG_CONTEXT)YoriLibAddToPointer(CmdContext.ysargv, CmdContext.argc * sizeof(YORI_STRING));

                memcpy(CmdContext.ysargv, OldArgv, OldArgCount * sizeof(YORI_STRING));
                for (Count = 0; Count < OldArgCount; Count++) {
                    CmdContext.ArgContexts[Count] = OldArgContext[Count];
                }

                YoriLibInitEmptyString(&CmdContext.ysargv[CmdContext.CurrentArg]);
            }

            YoriLibFreeStringContents(&CmdContext.ysargv[CmdContext.CurrentArg]);
            YoriLibCloneString(&CmdContext.ysargv[CmdContext.CurrentArg], &Match->YsValue);
            CmdContext.ArgContexts[CmdContext.CurrentArg].Quoted = FALSE;
            YoriShCheckIfArgNeedsQuotes(&CmdContext, CmdContext.CurrentArg);
            NewString = YoriShBuildCmdlineFromCmdContext(&CmdContext, FALSE, &BeginCurrentArg, &EndCurrentArg);

            if (OldArgv != NULL) {
                YoriLibFree(CmdContext.ysargv);
                CmdContext.argc = OldArgCount;
                CmdContext.ysargv = OldArgv;
                CmdContext.ArgContexts = OldArgContext;
            }

            if (NewString == NULL) {
                YoriShFreeCmdContext(&CmdContext);
                return;
            }

            FreeNewString = TRUE;
            Buffer->CurrentOffset = EndCurrentArg + 1;
            NewStringLen = _tcslen(NewString);

        } else {
            NewString = Match->YsValue.StartOfString;
            NewStringLen = Match->YsValue.LengthInChars;
            Buffer->CurrentOffset = NewStringLen;
        }

        if (NewString != NULL) {
            if (!YoriShEnsureStringHasEnoughCharacters(&Buffer->String, NewStringLen)) {
                YoriShFreeCmdContext(&CmdContext);
                return;
            }
            YoriLibYPrintf(&Buffer->String, _T("%s"), NewString);
            if (Buffer->CurrentOffset > Buffer->String.LengthInChars) {
                Buffer->CurrentOffset = Buffer->String.LengthInChars;
            }

            if (FreeNewString) {
                YoriLibDereference(NewString);
            }

            //
            //  For successful tab completion, redraw everything.  It's rare
            //  and plenty of changes are possible.
            //

            Buffer->DirtyBeginOffset = 0;
            Buffer->DirtyLength = Buffer->String.LengthInChars;
        }
    }
    
    YoriShFreeCmdContext(&CmdContext);
}


// vim:sw=4:ts=4:et:
