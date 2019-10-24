/**
 * @file builtins/z.c
 *
 * Yori shell change directory based on a heuristic match
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strZHelpText[] =
        "\n"
        "Changes the current directory based on a heuristic match.\n"
        "\n"
        "Z [-license] <directory>\n";

/**
 Display usage text to the user.
 */
BOOL
ZHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Z %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strZHelpText);
    return TRUE;
}

/**
 The number of recent directories to remember in memory.
 */
#define Z_MAX_RECENT_DIRS (64)

/**
 A linked list element corresponding to a remembered directory.
 */
typedef struct _Z_RECENT_DIRECTORY {

    /**
     List element across the set of remembered directories.  Corresponds
     to ZRecentDirectories.RecentDirList .
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The fully qualified name of the remembered directory.
     */
    YORI_STRING DirectoryName;

    /**
     The number of times the directory has been encountered.
     */
    DWORD HitCount;
} Z_RECENT_DIRECTORY, *PZ_RECENT_DIRECTORY;

/**
 The head of the list of recent directories.
 */
typedef struct _Z_RECENT_DIRECTORIES {
    /**
     The head of the list of recent directories.
     */
    YORI_LIST_ENTRY RecentDirList;

    /**
     The number of items currently in the list of recent directories, so we
     can efficiently know when it's time to trim the list.
     */
    DWORD RecentDirCount;

    /**
     A monotonically increasing number corresponding to attempts to add items
     into the recent list.  Periodically this will trigger logic to trim the
     HitCount on all existing entries to prevent them counting to infinity.
     */
    DWORD MonotonicAddAttempt;

} Z_RECENT_DIRECTORIES, *PZ_RECENT_DIRECTORIES;

/**
 A directory name that matches the user's search criteria.  This structure
 is seperate from the above as it is arranged in an array form of matches
 with a score attached to each, and the score is determined based on the
 user criteria.
 */
typedef struct _Z_SCOREBOARD_ENTRY {

    /**
     The name of the directory.  Note that while the array is being
     constructed this string is not referenced, but still contains a
     populated MemoryToFree value so that it can be referenced and returned
     to the caller if it's the best match.
     */
    YORI_STRING DirectoryName;

    /**
     The score for this entry.
     */
    DWORD Score;
} Z_SCOREBOARD_ENTRY, *PZ_SCOREBOARD_ENTRY;

/**
 The set of recent directories known to the module.
 */
Z_RECENT_DIRECTORIES ZRecentDirectories;

/**
 Set to TRUE once the command has been invoked once to keep the module loaded.
 */
BOOL ZCallbacksRegistered;

/**
 Display the current known list of recent directories in order of most
 recently used to least recently used with their corresponding hit count.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ZListStack()
{
    PYORI_LIST_ENTRY ListEntry;
    PZ_RECENT_DIRECTORY FoundRecentDir;

    ListEntry = YoriLibGetNextListEntry(&ZRecentDirectories.RecentDirList, NULL);
    while (ListEntry != NULL) {
        FoundRecentDir = CONTAINING_RECORD(ListEntry, Z_RECENT_DIRECTORY, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&ZRecentDirectories.RecentDirList, ListEntry);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y HitCount %i\n"), &FoundRecentDir->DirectoryName, FoundRecentDir->HitCount);
    }
    return TRUE;
}

/**
 Check the recent directories for a match to DirectoryName.  If a match is
 found, promote it to be the most recent entry and update its HitCount.
 If no match is found, add a new match, potentially evicting the least
 recently used match.

 @param DirectoryName Pointer to the fully qualified directory name to add.

 @return TRUE if the entry was successfully added, FALSE if it was not.
 */
BOOL
ZAddDirectoryToRecent(
    __in PYORI_STRING DirectoryName
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PZ_RECENT_DIRECTORY FoundRecentDir;

    if (ZRecentDirectories.RecentDirList.Next == NULL) {
        YoriLibInitializeListHead(&ZRecentDirectories.RecentDirList);
    }

    //
    //  If this attempt to add is a multiple of 1/4th of the size of the
    //  list, decrease each HitCount by 1/4th of its current value.
    //  This math isn't completely perfect, but it will tend to keep
    //  HitCounts relatively low while still maintaining a measurable
    //  difference between entries hit a lot and entries rarely hit.
    //

    ZRecentDirectories.MonotonicAddAttempt++;
    if ((ZRecentDirectories.MonotonicAddAttempt % (Z_MAX_RECENT_DIRS >> 2)) == 0) {
        ListEntry = YoriLibGetNextListEntry(&ZRecentDirectories.RecentDirList, NULL);
        while (ListEntry != NULL) {
            FoundRecentDir = CONTAINING_RECORD(ListEntry, Z_RECENT_DIRECTORY, ListEntry);
            ListEntry = YoriLibGetNextListEntry(&ZRecentDirectories.RecentDirList, ListEntry);
            FoundRecentDir->HitCount -= (FoundRecentDir->HitCount >> 2);
            ASSERT(FoundRecentDir->HitCount > 0);
        }
    }

    //
    //  Check if the new directory already exists in the recent directory
    //  list, update its position to be head of the list, increase its
    //  HitCount, and return.
    //

    ListEntry = YoriLibGetNextListEntry(&ZRecentDirectories.RecentDirList, NULL);
    while (ListEntry != NULL) {
        FoundRecentDir = CONTAINING_RECORD(ListEntry, Z_RECENT_DIRECTORY, ListEntry);
        if (YoriLibCompareStringInsensitive(DirectoryName, &FoundRecentDir->DirectoryName) == 0) {
            YoriLibRemoveListItem(&FoundRecentDir->ListEntry);
            YoriLibInsertList(&ZRecentDirectories.RecentDirList, &FoundRecentDir->ListEntry);
            FoundRecentDir->HitCount++;
            return TRUE;
        }
        ListEntry = YoriLibGetNextListEntry(&ZRecentDirectories.RecentDirList, ListEntry);
    }

    //
    //  Since it's not in the list, evict the oldest entry if the list has
    //  reached its maximum size.
    //

    if (ZRecentDirectories.RecentDirCount >= Z_MAX_RECENT_DIRS) {
        ListEntry = YoriLibGetPreviousListEntry(&ZRecentDirectories.RecentDirList, NULL);
        FoundRecentDir = CONTAINING_RECORD(ListEntry, Z_RECENT_DIRECTORY, ListEntry);
        YoriLibRemoveListItem(&FoundRecentDir->ListEntry);
        YoriLibFreeStringContents(&FoundRecentDir->DirectoryName);
        YoriLibDereference(FoundRecentDir);
        ZRecentDirectories.RecentDirCount--;
    }

    //
    //  Attempt to insert a new entry corresponding to this directory.
    //

    FoundRecentDir = YoriLibReferencedMalloc(sizeof(Z_RECENT_DIRECTORY) + (DirectoryName->LengthInChars + 1) * sizeof(TCHAR));
    if (FoundRecentDir == NULL) {
        return FALSE;
    }

    YoriLibReference(FoundRecentDir);
    FoundRecentDir->DirectoryName.MemoryToFree = FoundRecentDir;
    FoundRecentDir->DirectoryName.StartOfString = (LPWSTR)(FoundRecentDir + 1);
    FoundRecentDir->DirectoryName.LengthAllocated = DirectoryName->LengthInChars + 1;
    FoundRecentDir->DirectoryName.LengthInChars = DirectoryName->LengthInChars;

    memcpy(FoundRecentDir->DirectoryName.StartOfString, DirectoryName->StartOfString, (DirectoryName->LengthInChars + 1) * sizeof(TCHAR));

    FoundRecentDir->HitCount = 1;

    YoriLibInsertList(&ZRecentDirectories.RecentDirList, &FoundRecentDir->ListEntry);
    ZRecentDirectories.RecentDirCount++;

    ASSERT(ZRecentDirectories.RecentDirCount <= Z_MAX_RECENT_DIRS);

    return TRUE;
}

/**
 Called when the module is unloaded to clean up state.
 */
VOID
YORI_BUILTIN_FN
ZNotifyUnload()
{
    PYORI_LIST_ENTRY ListEntry;
    PZ_RECENT_DIRECTORY FoundRecentDir;

    ListEntry = YoriLibGetNextListEntry(&ZRecentDirectories.RecentDirList, NULL);
    while (ListEntry != NULL) {
        FoundRecentDir = CONTAINING_RECORD(ListEntry, Z_RECENT_DIRECTORY, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&ZRecentDirectories.RecentDirList, ListEntry);
        YoriLibRemoveListItem(&FoundRecentDir->ListEntry);
        YoriLibFreeStringContents(&FoundRecentDir->DirectoryName);
        YoriLibDereference(FoundRecentDir);
    }
}

/**
 Attempt to resolve the user string into a full path and check if the path
 exists.  If it exists, return this path as a candidate.  If it doesn't
 exist, free the full path allocation since it cannot be considered a
 candidate.

 @param OldCurrentDirectory Pointer to the existing current directory.  This
        is used to determine whether the incoming request is a no-op.

 @param UserSpecification Pointer to the string as specified by the user.

 @param FullNewDir On success, implying the object resolves to a full path and
        that path exists, this is populated with the full path.

 @return If the object resolves to a full path and that path exists, returns
         TRUE; otherwise, returns FALSE.
 */
BOOL
ZResolveSpecificationToFullPath(
    __in PYORI_STRING OldCurrentDirectory,
    __in PYORI_STRING UserSpecification,
    __out PYORI_STRING FullNewDir
    )
{
    YoriLibInitEmptyString(FullNewDir);

    if (UserSpecification->LengthInChars == 2 &&
        ((UserSpecification->StartOfString[0] >= 'A' && UserSpecification->StartOfString[0] <= 'Z') ||
         (UserSpecification->StartOfString[0] >= 'a' && UserSpecification->StartOfString[0] <= 'z')) &&
        UserSpecification->StartOfString[1] == ':') {

        //
        //  Cases:
        //  1. Same drive, do nothing (return success)
        //  2. Different drive, prior dir exists
        //  3. Different drive, no prior dir exists
        //

        if (OldCurrentDirectory->StartOfString[0] >= 'a' && OldCurrentDirectory->StartOfString[0] <= 'z') {
            OldCurrentDirectory->StartOfString[0] = YoriLibUpcaseChar(OldCurrentDirectory->StartOfString[0]);
        }

        if (OldCurrentDirectory->StartOfString[1] == ':' &&
            (OldCurrentDirectory->StartOfString[0] == UserSpecification->StartOfString[0] ||
             (UserSpecification->StartOfString[0] >= 'a' &&
              UserSpecification->StartOfString[0] <= 'z' &&
              (UserSpecification->StartOfString[0] - 'a' + 'A') == OldCurrentDirectory->StartOfString[0]))) {


            return FALSE;
        }

        YoriLibGetCurrentDirectoryOnDrive(UserSpecification->StartOfString[0], FullNewDir);

    } else {

        YoriLibUserStringToSingleFilePath(UserSpecification, FALSE, FullNewDir);
    }

    //
    //  Check if the object exists.  If it doesn't exist, free the string and
    //  say we successfully couldn't generate a match.
    //

    if (GetFileAttributes(FullNewDir->StartOfString) == (DWORD)-1) {
        YoriLibFreeStringContents(FullNewDir);
    }

    return TRUE;
}

/**
 Take any fully resolved path based on the user specification, and any
 recent directories that match the user specification, heuristically assign
 each directory with a score, and return the entry with the highest score.
 If nothing matches the user specification, returns FALSE.

 @param UserSpecification Pointer to the user specification to match against.

 @param FullMatchToUserSpec Pointer to a string that is a fully qualified
        path resovled by the UserSpecification.  This may be an empty string
        if the user specification could not be resolved or resolved to an
        object that does not exist.

 @param BestMatch On successful completion, updated to point to a referenced
        string containing the best match for this directory change operation.

 @return TRUE if a match was found, FALSE if it was not.
 */
__success(return)
BOOL
ZBuildScoreboardAndSelectBest(
    __in PYORI_STRING UserSpecification,
    __in PYORI_STRING FullMatchToUserSpec,
    __out PYORI_STRING BestMatch
    )
{
    PZ_SCOREBOARD_ENTRY Entries;
    PYORI_LIST_ENTRY ListEntry;
    PZ_RECENT_DIRECTORY FoundRecentDir;
    YORI_STRING FinalComponent;
    YORI_STRING TrailingPortion;
    YORI_STRING StringToAdd;
    DWORD EntriesPopulated;
    DWORD Index;
    DWORD ScoreForThisEntry;
    DWORD BestScore;
    DWORD BestIndex;
    DWORD OffsetOfMatch;
    BOOL SeperatorBefore;
    BOOL SeperatorAfter;
    BOOL AddThisEntry;
    BOOL FoundAsParentOnly;

    //
    //  Allocate enough entries for everything we know about, including all
    //  history and the currently resolved full path
    //

    Entries = YoriLibMalloc(sizeof(Z_SCOREBOARD_ENTRY) * (Z_MAX_RECENT_DIRS + 1));
    if (Entries == NULL) {
        return FALSE;
    }

    EntriesPopulated = 0;

    //
    //  If we have a fully resolved match, add it unconditionally.  Don't
    //  check if it matches the user specification - we already know it
    //  does, and string compare might be misleading because the user
    //  specification may not contain any matching string (eg. "..").
    //

    if (FullMatchToUserSpec->LengthInChars > 0) {
        memcpy(&Entries[EntriesPopulated].DirectoryName, FullMatchToUserSpec, sizeof(YORI_STRING));
        Entries[EntriesPopulated].Score = Z_MAX_RECENT_DIRS * 16;
        EntriesPopulated++;
    }

    Index = Z_MAX_RECENT_DIRS;
    ListEntry = YoriLibGetNextListEntry(&ZRecentDirectories.RecentDirList, NULL);
    while (ListEntry != NULL) {
        FoundRecentDir = CONTAINING_RECORD(ListEntry, Z_RECENT_DIRECTORY, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&ZRecentDirectories.RecentDirList, ListEntry);
        AddThisEntry = FALSE;
        FoundAsParentOnly = FALSE;

        //
        //  Calculate a rough score for this entry.
        //

        ScoreForThisEntry = Index * (Z_MAX_RECENT_DIRS / 8);
        ScoreForThisEntry += FoundRecentDir->HitCount * (Z_MAX_RECENT_DIRS / 8);

        //
        //  Determine if it's a match and we should add it.
        //

        YoriLibInitEmptyString(&FinalComponent);
        YoriLibInitEmptyString(&TrailingPortion);
        FinalComponent.StartOfString = YoriLibFindRightMostCharacter(&FoundRecentDir->DirectoryName, '\\');

        if (FoundRecentDir->DirectoryName.LengthInChars >= UserSpecification->LengthInChars) {
            TrailingPortion.StartOfString = &FoundRecentDir->DirectoryName.StartOfString[FoundRecentDir->DirectoryName.LengthInChars - UserSpecification->LengthInChars];
            TrailingPortion.LengthInChars = UserSpecification->LengthInChars;
        }
        OffsetOfMatch = 0;

        //
        //  If it's a complete match of the final component, big bonus points.
        //  If it's a match up to the end of the string, moderate bonus points.
        //  If it's somewhere in the final component, small bonus points.
        //

        if (FinalComponent.StartOfString != NULL) {
            FinalComponent.StartOfString++;
            FinalComponent.LengthInChars = FoundRecentDir->DirectoryName.LengthInChars - (DWORD)(FinalComponent.StartOfString - FoundRecentDir->DirectoryName.StartOfString);

            if (YoriLibCompareStringInsensitive(&FinalComponent, UserSpecification) == 0) {
                ScoreForThisEntry += Z_MAX_RECENT_DIRS * 4;
                AddThisEntry = TRUE;
            } else if (TrailingPortion.LengthInChars > 0 &&
                       YoriLibCompareStringInsensitive(&TrailingPortion, UserSpecification) == 0) {
                ScoreForThisEntry += Z_MAX_RECENT_DIRS * 2;
                AddThisEntry = TRUE;
            } else if (YoriLibFindFirstMatchingSubstringInsensitive(&FinalComponent, 1, UserSpecification, NULL) != NULL) {

                ScoreForThisEntry += Z_MAX_RECENT_DIRS;
                AddThisEntry = TRUE;
            }
        }

        YoriLibInitEmptyString(&StringToAdd);
        if (AddThisEntry) {
            StringToAdd.StartOfString = FoundRecentDir->DirectoryName.StartOfString;
            StringToAdd.LengthInChars = FoundRecentDir->DirectoryName.LengthInChars;
        }

        //
        //  If it's in the string but not the final component, add it, but
        //  no bonus points.  If the user specification refers to a parent
        //  component, add up to that component only.
        //

        if (!AddThisEntry &&
            UserSpecification->LengthInChars > 0 &&
            YoriLibFindFirstMatchingSubstringInsensitive(&FoundRecentDir->DirectoryName, 1, UserSpecification, &OffsetOfMatch) != NULL) {

            SeperatorBefore = FALSE;
            SeperatorAfter = FALSE;

            if (OffsetOfMatch == 0 ||
                YoriLibIsSep(UserSpecification->StartOfString[0]) ||
                YoriLibIsSep(FoundRecentDir->DirectoryName.StartOfString[OffsetOfMatch - 1])) {
                SeperatorBefore = TRUE;
            }

            if (OffsetOfMatch + UserSpecification->LengthInChars == FoundRecentDir->DirectoryName.LengthInChars ||
                YoriLibIsSep(UserSpecification->StartOfString[UserSpecification->LengthInChars - 1]) ||
                YoriLibIsSep(FoundRecentDir->DirectoryName.StartOfString[OffsetOfMatch + UserSpecification->LengthInChars])) {
                SeperatorAfter = TRUE;
            }

            StringToAdd.StartOfString = FoundRecentDir->DirectoryName.StartOfString;
            if (SeperatorBefore && SeperatorAfter) {
                StringToAdd.LengthInChars = OffsetOfMatch + UserSpecification->LengthInChars;
            } else {
                StringToAdd.LengthInChars = FoundRecentDir->DirectoryName.LengthInChars;
            }
            AddThisEntry = TRUE;
            FoundAsParentOnly = TRUE;
        }

        //
        //  If the currently found directory has already been added by the
        //  fully resolved user specification or an earlier parent match,
        //  don't add it twice.  If it's a high quality match, such as
        //  against a user specification or final component, add the scores
        //  together.  Don't do this if the match was against a parent
        //  component, because many entries may have the same ancestors but
        //  that doesn't imply they have the quality of all children
        //  combined.
        //

        if (AddThisEntry) {
            for (BestIndex = 0; BestIndex < EntriesPopulated; BestIndex++) {
                if (YoriLibCompareStringInsensitive(&Entries[BestIndex].DirectoryName, &StringToAdd) == 0) {

                    if (!FoundAsParentOnly) {
                        Entries[BestIndex].Score += ScoreForThisEntry;
                    }
                    AddThisEntry = FALSE;
                    break;
                }
            }
        }

        //
        //  If we should add it, go add it with the calculated score.
        //

        if (AddThisEntry) {
            memcpy(&Entries[EntriesPopulated].DirectoryName, &StringToAdd, sizeof(YORI_STRING));
            Entries[EntriesPopulated].Score = ScoreForThisEntry;
            EntriesPopulated++;
        }
        if (Index > 0) {
            Index--;
        }
    }

    //
    //  If we have no matches, then we can't find anything that the user
    //  would be happy with, so do nothing.
    //

    if (EntriesPopulated == 0) {
        YoriLibFree(Entries);
        return FALSE;
    }

    //
    //  Find the highest score.
    //

    BestScore = 0;
    BestIndex = 0;
    for (Index = 0; Index < EntriesPopulated; Index++) {
        if (BestScore == 0 || Entries[Index].Score > BestScore) {
            BestScore = Entries[Index].Score;
            BestIndex = Index;
        }
    }

    //
    //  Return the highest score match as a referenced string, and free
    //  the scoreboard.  Perform a new allocation for this to ensure it's
    //  NULL terminated at the correct point.
    //

    if (!YoriLibAllocateString(BestMatch, Entries[BestIndex].DirectoryName.LengthInChars + 1)) {
        return FALSE;
    }
    memcpy(BestMatch->StartOfString, Entries[BestIndex].DirectoryName.StartOfString, Entries[BestIndex].DirectoryName.LengthInChars * sizeof(TCHAR));
    BestMatch->LengthInChars = Entries[BestIndex].DirectoryName.LengthInChars;
    BestMatch->StartOfString[BestMatch->LengthInChars] = '\0';

    YoriLibFree(Entries);
    return TRUE;
}

/**
 Change current directory heuristically builtin command.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_Z(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL Result;
    DWORD OldCurrentDirectoryLength;
    YORI_STRING OldCurrentDirectory;
    YORI_STRING FullyResolvedUserSpecification;
    YORI_STRING BestMatch;
    PYORI_STRING UserSpecification;
    BOOL ArgumentUnderstood;
    BOOL Unload = FALSE;
    BOOL ListStack = FALSE;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ZHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                ListStack = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("u")) == 0) {
                Unload = TRUE;
                ArgumentUnderstood = TRUE;
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

    if (ListStack) {
        ZListStack();
        return EXIT_SUCCESS;
    }

    if (Unload) {
        if (ZCallbacksRegistered) {
            YORI_STRING ZCmd;
            YoriLibConstantString(&ZCmd, _T("Z"));
            YoriCallBuiltinUnregister(&ZCmd, YoriCmd_Z);
            ZCallbacksRegistered = FALSE;
        }
        return EXIT_SUCCESS;
    }

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("z: missing argument\n"));
        return EXIT_FAILURE;
    }

    UserSpecification = &ArgV[StartArg];

    OldCurrentDirectoryLength = GetCurrentDirectory(0, NULL);
    if (!YoriLibAllocateString(&OldCurrentDirectory, OldCurrentDirectoryLength)) {
        return EXIT_FAILURE;
    }

    OldCurrentDirectory.LengthInChars = GetCurrentDirectory(OldCurrentDirectory.LengthAllocated, OldCurrentDirectory.StartOfString);
    if (OldCurrentDirectory.LengthInChars == 0 ||
        OldCurrentDirectory.LengthInChars >= OldCurrentDirectory.LengthAllocated) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("z: Could not query current directory: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&OldCurrentDirectory);
        return EXIT_FAILURE;
    }

    if (ArgC <= 1) {
        YoriLibFreeStringContents(&OldCurrentDirectory);
        return EXIT_FAILURE;
    }

    if (!ZResolveSpecificationToFullPath(&OldCurrentDirectory, UserSpecification, &FullyResolvedUserSpecification)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("z: invalid request\n"));
        YoriLibFreeStringContents(&OldCurrentDirectory);
        return EXIT_FAILURE;
    }

    if (!ZBuildScoreboardAndSelectBest(UserSpecification, &FullyResolvedUserSpecification, &BestMatch)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("z: could not determine appropriate directory\n"));
        YoriLibFreeStringContents(&OldCurrentDirectory);
        YoriLibFreeStringContents(&FullyResolvedUserSpecification);
        return EXIT_FAILURE;
    }

    YoriLibFreeStringContents(&FullyResolvedUserSpecification);

    ZAddDirectoryToRecent(&OldCurrentDirectory);
    ZAddDirectoryToRecent(&BestMatch);

    Result = SetCurrentDirectory(BestMatch.StartOfString);
    if (!Result) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("z: Could not change directory: %y: %s"), &BestMatch, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&OldCurrentDirectory);
        YoriLibFreeStringContents(&BestMatch);
        return EXIT_FAILURE;
    }

    //
    //  Convert the first character to uppercase for comparison later.
    //

    if (OldCurrentDirectory.StartOfString[0] >= 'a' && OldCurrentDirectory.StartOfString[0] <= 'z') {
        OldCurrentDirectory.StartOfString[0] = YoriLibUpcaseChar(OldCurrentDirectory.StartOfString[0]);
    }

    if (BestMatch.StartOfString[0] >= 'a' && BestMatch.StartOfString[0] <= 'z') {
        BestMatch.StartOfString[0] = YoriLibUpcaseChar(BestMatch.StartOfString[0]);
    }

    //
    //  If the old current directory is drive letter based, preserve the old
    //  current directory in the environment.
    //

    if (OldCurrentDirectory.StartOfString[0] >= 'A' &&
        OldCurrentDirectory.StartOfString[0] <= 'Z' &&
        OldCurrentDirectory.StartOfString[1] == ':') {

        if (BestMatch.StartOfString[0] != OldCurrentDirectory.StartOfString[0]) {
            YoriLibSetCurrentDirectoryOnDrive(OldCurrentDirectory.StartOfString[0], &OldCurrentDirectory);
        }
    }

    YoriLibFreeStringContents(&OldCurrentDirectory);
    YoriLibFreeStringContents(&BestMatch);

    if (!ZCallbacksRegistered) {
        YORI_STRING ZCmd;
        YoriLibConstantString(&ZCmd, _T("Z"));
        if (!YoriCallBuiltinRegister(&ZCmd, YoriCmd_Z)) {
            return EXIT_FAILURE;
        }
        YoriCallSetUnloadRoutine(ZNotifyUnload);
        ZCallbacksRegistered = TRUE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
