/**
 * @file make/preproc.c
 *
 * Yori shell make preprocessor
 *
 * Copyright (c) 2020 Malcolm J. Smith
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
#include <yorish.h>
#include "make.h"

/**
 Traverse the line, and if a comment is found within the line, remove the
 portion of the string following that comment from further processing.

 @param Line Pointer to the line to parse.  On output its length may be
        updated.
 */
VOID
MakeTruncateComments(
    __inout PYORI_STRING Line
    )
{
    DWORD Index;

    for (Index = 0; Index < Line->LengthInChars; Index++) {
        if (Line->StartOfString[Index] == '#') {
            Line->LengthInChars = Index;
            break;
        }
    }
}

/**
 Return TRUE if a single character is a whitespace character, FALSE if it is
 not.

 @param Char The character to check.
 
 @return TRUE to indicate a character is whitespace, FALSE if it is not.
 */
BOOLEAN
MakeIsCharWhitespace(
    __in TCHAR Char
    )
{
    if (Char == ' ' || Char == '\t') {
        return TRUE;
    }

    return FALSE;
}

/**
 Remove spaces from the beginning and end of a Yori string.
 Note this implies advancing the StartOfString pointer, so a caller
 cannot assume this pointer is unchanged across the call.

 @param String Pointer to the Yori string to remove spaces from.
 */
VOID
MakeTrimWhitespace(
    __in PYORI_STRING String
    )
{
    while (String->LengthInChars > 0) {
        if (MakeIsCharWhitespace(String->StartOfString[0])) {
            String->StartOfString++;
            String->LengthInChars--;
        } else {
            break;
        }
    }

    while (String->LengthInChars > 0) {
        if (MakeIsCharWhitespace(String->StartOfString[String->LengthInChars - 1])) {
            String->LengthInChars--;
        } else {
            break;
        }
    }
}

/**
 Remove separators from the beginning and end of a Yori string.
 Note this implies advancing the StartOfString pointer, so a caller
 cannot assume this pointer is unchanged across the call.

 @param String Pointer to the Yori string to remove spaces from.
 */
VOID
MakeTrimSeparators(
    __in PYORI_STRING String
    )
{
    while (String->LengthInChars > 0) {

        if (String->LengthInChars >= 2 &&
            String->StartOfString[0] == '.' &&
            YoriLibIsSep(String->StartOfString[1])) {

            String->StartOfString = String->StartOfString + 2;
            String->LengthInChars = String->LengthInChars - 2;

        } else if (String->LengthInChars == 1 &&
                   String->StartOfString[0] == '.') {
            String->LengthInChars--;
        } else if (YoriLibIsSep(String->StartOfString[0])) {
            String->StartOfString++;
            String->LengthInChars--;
        } else {
            break;
        }
    }

    while (String->LengthInChars > 0) {
        if (YoriLibIsSep(String->StartOfString[String->LengthInChars - 1])) {
            String->LengthInChars--;
        } else {
            break;
        }
    }
}

/**
 When a line ends with a trailing backslash, it needs to be concatenated.
 This function performs that concatenation where a new part can be joined
 with an existing part of a line.  Note that this concatenation removes
 the trailing backslash and ensures there is a single space between the
 new line and existing lines.

 @param CombinedLine On input, points to an initialized Yori string.  This
        may be empty or may contain an existing string.

 @param NewEnd Points to a line which should be appended to CombinedLine.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeJoinLines(
    __inout PYORI_STRING CombinedLine,
    __in PYORI_STRING NewEnd
    )
{
    DWORD CharsToCopy;
    DWORD AllocationNeeded;

    CharsToCopy = NewEnd->LengthInChars;
    if (CharsToCopy == 0) {
        return TRUE;
    }

    if (NewEnd->StartOfString[CharsToCopy - 1] == '\\') {
        CharsToCopy--;
        while (CharsToCopy > 0) {
            if (MakeIsCharWhitespace(NewEnd->StartOfString[CharsToCopy - 1])) {

                CharsToCopy--;
            } else {
                break;
            }
        }
    }

    if (CharsToCopy == 0) {
        return TRUE;
    }

    //
    //  We need the already combined portion, a space, the new portion, and we
    //  reserve space for a NULL terminator.
    //

    AllocationNeeded = CombinedLine->LengthInChars + 1 + CharsToCopy + 1;
    if (AllocationNeeded > CombinedLine->LengthAllocated) {
        DWORD NewLength;
        NewLength = CombinedLine->LengthInChars * 2;
        if (NewLength < AllocationNeeded) {
            NewLength = AllocationNeeded;
        }

        if (!YoriLibReallocateString(CombinedLine, NewLength)) {
            return FALSE;
        }
    }

    CombinedLine->StartOfString[CombinedLine->LengthInChars] = ' ';
    CombinedLine->LengthInChars++;
    memcpy(&CombinedLine->StartOfString[CombinedLine->LengthInChars], NewEnd->StartOfString, CharsToCopy * sizeof(TCHAR));
    CombinedLine->LengthInChars = CombinedLine->LengthInChars + CharsToCopy;
    return TRUE;
}

/**
 The different types of lines within a makefile that are supported by this
 program.
 */
typedef enum _MAKE_LINE_TYPE {
    MakeLineTypeEmpty = 0,
    MakeLineTypePreprocessor = 1,
    MakeLineTypeSetVariable = 2,
    MakeLineTypeRule = 3,
    MakeLineTypeRecipe = 4,
    MakeLineTypeInlineFile = 5,
    MakeLineTypeDebugBreak = 6,
    MakeLineTypeError = 7
} MAKE_LINE_TYPE;


/**
 Parse the line to determine what action it is to perform.

 @param Line Pointer to the line in the makefile.

 @param ScopeContext Pointer to the scope context which indicates the current
        parsing state.  This is not modified in this function, because this
        function has not determined whether the line is within the
        preprocessor scope.

 @return The type of the line.
 */
MAKE_LINE_TYPE
MakeDetermineLineType(
    __in PYORI_STRING Line,
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    )
{
    DWORD Index;
    DWORD BraceDepth;
    DWORD WhitespaceChars;

    if (Line->LengthInChars == 0) {
        return MakeLineTypeEmpty;
    }

    if (Line->StartOfString[0] == '!') {
        return MakeLineTypePreprocessor;
    }

    if (ScopeContext->ParserState == MakeParserRecipeActive) {
        if (MakeIsCharWhitespace(Line->StartOfString[0])) {
            return MakeLineTypeRecipe;
        }
    } else if (ScopeContext->ParserState == MakeParserInlineFileActive) {
        return MakeLineTypeInlineFile;
    }

    BraceDepth = 0;
    WhitespaceChars = 0;
    for (Index = 0; Index < Line->LengthInChars; Index++) {
        if (MakeIsCharWhitespace(Line->StartOfString[Index])) {
            WhitespaceChars++;
            continue;
        }
        if (Line->StartOfString[Index] == '[') {
            BraceDepth++;
        } else if (Line->StartOfString[Index] == ']' && BraceDepth > 0) {
            BraceDepth--;
        }

        if (BraceDepth == 0) {
            if (Line->StartOfString[Index] == '=') {
                return MakeLineTypeSetVariable;
            } else if (Line->StartOfString[Index] == ':') {
                ASSERT(ScopeContext->ParserState == MakeParserDefault);
                return MakeLineTypeRule;
            }
        }
    }

    if (WhitespaceChars == Line->LengthInChars) {
        return MakeLineTypeEmpty;
    }

    if (YoriLibCompareStringWithLiteralInsensitive(Line, _T("DebugBreak")) == 0) {
        return MakeLineTypeDebugBreak;
    }

    return MakeLineTypeError;
}

/**
 A list of preprocessor commands known to this program.
 */
typedef enum _MAKE_PREPROCESSOR_LINE_TYPE {
    MakePreprocessorLineTypeUnknown = 0,
    MakePreprocessorLineTypeElse = 1,
    MakePreprocessorLineTypeElseIf = 2,
    MakePreprocessorLineTypeElseIfDef = 3,
    MakePreprocessorLineTypeElseIfNDef = 4,
    MakePreprocessorLineTypeEndIf = 5,
    MakePreprocessorLineTypeError = 6,
    MakePreprocessorLineTypeIf = 7,
    MakePreprocessorLineTypeIfDef = 8,
    MakePreprocessorLineTypeIfNDef = 9,
    MakePreprocessorLineTypeInclude = 10,
    MakePreprocessorLineTypeMessage = 11,
    MakePreprocessorLineTypeUndef = 12
} MAKE_PREPROCESSOR_LINE_TYPE;

/**
 A map of preprocessor commands from their string form to their numeric
 form.
 */
typedef struct _MAKE_PREPROCESSOR_LINE_TYPE_MAP {

    /**
     The string form of the preprocessor command.
     */
    LPTSTR String;

    /**
     The numeric type of the preprocessor command.
     */
    DWORD Type;
} MAKE_PREPROCESSOR_LINE_TYPE_MAP;

/**
 An array of preprocessor commands.  Note that there are subcommands within
 !ELSE which are not included here, because they can follow the ELSE after a
 space.
 */
CONST MAKE_PREPROCESSOR_LINE_TYPE_MAP MakePreprocessorLineTypeMap[] = {
    {_T("ELSE"),    MakePreprocessorLineTypeElse },
    {_T("ENDIF"),   MakePreprocessorLineTypeEndIf },
    {_T("ERROR"),   MakePreprocessorLineTypeError },
    {_T("IFDEF"),   MakePreprocessorLineTypeIfDef },
    {_T("IFNDEF"),  MakePreprocessorLineTypeIfNDef },
    {_T("IF"),      MakePreprocessorLineTypeIf },
    {_T("INCLUDE"), MakePreprocessorLineTypeInclude },
    {_T("MESSAGE"), MakePreprocessorLineTypeMessage },
    {_T("UNDEF"),   MakePreprocessorLineTypeUndef },
};

/**
 Parse a preprocessor line and determine which type of line it is.

 @param Line Pointer to the line to parse.

 @param ArgumentOffset Optionally points to a DWORD to return the offset
        within the string where any arguments exist (ie., after the
        preprocessor command.)

 @return The type of the preprocessor line.
 */
MAKE_PREPROCESSOR_LINE_TYPE
MakeDeterminePreprocessorLineType(
    __in PYORI_STRING Line,
    __out_opt PDWORD ArgumentOffset
    )
{
    YORI_STRING Substring;
    MAKE_PREPROCESSOR_LINE_TYPE FoundType;
    DWORD SubstringOffset;
    DWORD Index;

    //
    //  We shouldn't ever hit this condition because the line has already been
    //  determined to be a preprocessor directive.
    //

    if (Line->LengthInChars < 1 || Line->StartOfString[0] != '!') {
        ASSERT(Line->LengthInChars >= 1 && Line->StartOfString[0] == '!');
        if (ArgumentOffset != NULL) {
            *ArgumentOffset = 0;
        }
        return MakePreprocessorLineTypeUnknown;
    }

    //
    //  Per the documentation, there can be zero or more spaces or tabs
    //  between the exclamation point at the command, so swallow.
    //

    for (Index = 1; Index < Line->LengthInChars; Index++) {
        if (Line->StartOfString[Index] != ' ' &&
            Line->StartOfString[Index] != '\t') {

            break;
        }
    }

    SubstringOffset = Index;
    Substring.StartOfString = &Line->StartOfString[SubstringOffset];
    Substring.LengthInChars = Line->LengthInChars - SubstringOffset;

    //
    //  Most commands are straightforward.  !ELSE is a bit funny, because it's
    //  valid to have !ELSEIF or !ELSE IF syntax, but they really mean the
    //  same thing.
    //

    FoundType = MakePreprocessorLineTypeUnknown;
    for (Index = 0; Index < sizeof(MakePreprocessorLineTypeMap)/sizeof(MakePreprocessorLineTypeMap[0]); Index++) {
        if (YoriLibCompareStringWithLiteralInsensitiveCount(&Substring, MakePreprocessorLineTypeMap[Index].String, _tcslen(MakePreprocessorLineTypeMap[Index].String)) == 0) {
            FoundType = MakePreprocessorLineTypeMap[Index].Type;
            if (ArgumentOffset != NULL) {
                *ArgumentOffset = SubstringOffset + _tcslen(MakePreprocessorLineTypeMap[Index].String);
            }
            break;
        }
    }

    if (FoundType != MakePreprocessorLineTypeElse) {
        return FoundType;
    }

    //
    //  If it's !ELSE, trim any following spaces, and compare with known
    //  qualifiers.
    //

    SubstringOffset = SubstringOffset + sizeof("ELSE") - 1;

    for (Index = SubstringOffset; Index < Line->LengthInChars; Index++) {
        if (Line->StartOfString[Index] != ' ' &&
            Line->StartOfString[Index] != '\t') {

            break;
        }
    }

    SubstringOffset = Index;

    Substring.StartOfString = &Line->StartOfString[SubstringOffset];
    Substring.LengthInChars = Line->LengthInChars - SubstringOffset;

    if (YoriLibCompareStringWithLiteralInsensitiveCount(&Substring, _T("IFNDEF"), sizeof("IFNDEF") - 1) == 0) {
        FoundType = MakePreprocessorLineTypeElseIfNDef;
        SubstringOffset = SubstringOffset + sizeof("IFNDEF") - 1;
    } else if (YoriLibCompareStringWithLiteralInsensitiveCount(&Substring, _T("IFDEF"), sizeof("IFDEF") - 1) == 0) {
        FoundType = MakePreprocessorLineTypeElseIfDef;
        SubstringOffset = SubstringOffset + sizeof("IFDEF") - 1;
    } else if (YoriLibCompareStringWithLiteralInsensitiveCount(&Substring, _T("IF"), sizeof("IF") - 1) == 0) {
        FoundType = MakePreprocessorLineTypeElseIf;
        SubstringOffset = SubstringOffset + sizeof("IF") - 1;
    }

    if (ArgumentOffset != NULL) {
        *ArgumentOffset = SubstringOffset;
    }

    return FoundType;
}

/**
 Determine based on the current if nesting level whether the line should be
 executed.  Note that for preprocessor lines, this can be a bit convoluted,
 because if we're currently out of execution scope but about to evaluate an
 "else if" line then that line needs to have variables expanded and executed.
 If it's nested however, within a condition where we're not evaluating
 anything, it must not be executed.

 @param ScopeContext Pointer to the scope context.

 @param Line Pointer to the line to consider executing.

 @param LineType Specifies the type of the line.

 @return TRUE if the line should be fully parsed and executed, including
         variable expansion.  FALSE if only minimal processing should occur,
         including tracking the nesting level of conditionals.
 */
BOOLEAN
MakePreprocessorShouldExecuteLine(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Line,
    __in MAKE_LINE_TYPE LineType
    )
{
    MAKE_PREPROCESSOR_LINE_TYPE PreprocessorLineType;

    //
    //  We're in a nested conditional block, but something above has evaluated
    //  to FALSE.
    //

    ASSERT(ScopeContext->CurrentConditionalNestingLevel >= ScopeContext->ActiveConditionalNestingLevel);

    if (ScopeContext->CurrentConditionalNestingLevel > ScopeContext->ActiveConditionalNestingLevel) {
        return FALSE;
    }

    //
    //  At the conditional block level, a previous test has evaluated to TRUE,
    //  so no other test should be evaluated.
    //

    if (ScopeContext->ActiveConditionalNestingLevelExecutionOccurred &&
        !ScopeContext->ActiveConditionalNestingLevelExecutionEnabled) {

        return FALSE;
    }

    //
    //  If the line is not a preprocessor directive, then either the current
    //  block should be evaluated, or it should be excluded.
    //

    if (LineType != MakeLineTypePreprocessor) {
        if (ScopeContext->ActiveConditionalNestingLevelExecutionEnabled) {
            return TRUE;
        }

        return FALSE;
    }

    //
    //  If it is a preprocessor directive, it matters which one.  If the
    //  current scope is inactive but we're executing a conditional that
    //  might possibly introduce an active scope, we need to evaluate that
    //  condition.  If we are active and see an else conditional, we should
    //  not evaluate it.
    //

    PreprocessorLineType = MakeDeterminePreprocessorLineType(Line, NULL);

    if (PreprocessorLineType == MakePreprocessorLineTypeEndIf) {
        return FALSE;
    }

    if (PreprocessorLineType == MakePreprocessorLineTypeElse ||
        PreprocessorLineType == MakePreprocessorLineTypeElseIf ||
        PreprocessorLineType == MakePreprocessorLineTypeElseIfDef ||
        PreprocessorLineType == MakePreprocessorLineTypeElseIfNDef) {

        if (ScopeContext->ActiveConditionalNestingLevelExecutionEnabled) {
            return FALSE;
        }

        return TRUE;

    } else {

        if (ScopeContext->ActiveConditionalNestingLevelExecutionEnabled) {
            return TRUE;
        }

        return FALSE;
    }

    return FALSE;
}

/**
 Execute a preprocessor line when the line is out of scope (excluded by an
 earlier if condition.)  This still needs to track execution scope but should
 not execute any commands or perform any user visible state changes.

 @param ScopeContext Pointer to the context.

 @param Line Pointer to the line to execute.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakePreprocessorPerformMinimalConditionalTracking(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Line
    )
{
    MAKE_PREPROCESSOR_LINE_TYPE PreprocessorLineType;
    PreprocessorLineType = MakeDeterminePreprocessorLineType(Line, NULL);

    switch(PreprocessorLineType) {
        case MakePreprocessorLineTypeIf:
        case MakePreprocessorLineTypeIfDef:
        case MakePreprocessorLineTypeIfNDef:
            ScopeContext->CurrentConditionalNestingLevel++;
            break;
        case MakePreprocessorLineTypeEndIf:

            ScopeContext->CurrentConditionalNestingLevel--;
            if (ScopeContext->ActiveConditionalNestingLevel > ScopeContext->CurrentConditionalNestingLevel) {
                ScopeContext->ActiveConditionalNestingLevel = ScopeContext->CurrentConditionalNestingLevel;
                ScopeContext->ActiveConditionalNestingLevelExecutionOccurred = TRUE;
                ScopeContext->ActiveConditionalNestingLevelExecutionEnabled = TRUE;
            }
            break;
        case MakePreprocessorLineTypeElse:
        case MakePreprocessorLineTypeElseIf:
        case MakePreprocessorLineTypeElseIfDef:
        case MakePreprocessorLineTypeElseIfNDef:
            if (ScopeContext->ActiveConditionalNestingLevelExecutionEnabled) {
                ASSERT(ScopeContext->ActiveConditionalNestingLevelExecutionOccurred);
                ScopeContext->ActiveConditionalNestingLevelExecutionEnabled = FALSE;
            }
            break;
    }

    return TRUE;
}

/**
 Commence execution within in !IF block where the condition has evaluated
 TRUE.  This means execution should continue, but we are nested one level.

 @param ScopeContext Pointer to the scope context to update.
 */
VOID
MakeBeginNestedConditionTrue(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    )
{
    ScopeContext->ActiveConditionalNestingLevelExecutionEnabled = TRUE;
    ScopeContext->ActiveConditionalNestingLevelExecutionOccurred = TRUE;
    ScopeContext->ActiveConditionalNestingLevel++;
    ScopeContext->CurrentConditionalNestingLevel++;
}

/**
 Commence execution within in !IF block where the condition has evaluated
 FALSE.  This means execution should not occur, but both the current and
 active scope is now nested one level, so we need to monitor for !ELSE
 conditions that would cause execution to resume.

 @param ScopeContext Pointer to the scope context to update.
 */
VOID
MakeBeginNestedConditionFalse(
    __in PMAKE_SCOPE_CONTEXT ScopeContext
    )
{
    ScopeContext->ActiveConditionalNestingLevelExecutionEnabled = FALSE;
    ScopeContext->ActiveConditionalNestingLevelExecutionOccurred = FALSE;
    ScopeContext->ActiveConditionalNestingLevel++;
    ScopeContext->CurrentConditionalNestingLevel++;
}

/**
 Generate the name of the preprocessor cache file from the specified
 make file name.

 @param MakeFileName Pointer to the make file name.

 @param CacheFileName On successful completion, updated to contain a newly
        allocated string referring to the file name of the cache file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeGetCacheFileNameFromMakeFileName(
    __in PYORI_STRING MakeFileName,
    __out PYORI_STRING CacheFileName
    )
{
    YoriLibInitEmptyString(CacheFileName);
    if (MakeFileName->LengthInChars > 0) {
        if (YoriLibAllocateString(CacheFileName, MakeFileName->LengthInChars + sizeof(".pru"))) {
            CacheFileName->LengthInChars = YoriLibSPrintf(CacheFileName->StartOfString, _T("%y.pru"), MakeFileName);
            return TRUE;
        }
    }

    return FALSE;
}

/**
 Load preprocessor cache entries from cache file.

 @param MakeContext Pointer to the context.

 @param MakeFileName Pointer to the file name of the makefile.  If this
        contains a string, it will be used as the base name for the cache.
 */
VOID
MakeLoadPreprocessorCacheEntries(
    __inout PMAKE_CONTEXT MakeContext,
    __in PYORI_STRING MakeFileName
    )
{
    PMAKE_PREPROC_EXEC_CACHE_ENTRY Entry;
    YORI_STRING CacheFileName;
    YORI_STRING Key;
    YORI_STRING LineString;
    DWORD CharsConsumed;
    LONGLONG llTemp;
    HANDLE hCache;
    PVOID LineContext = NULL;

    if (!MakeGetCacheFileNameFromMakeFileName(MakeFileName, &CacheFileName)) {
        return;
    }

    hCache = CreateFile(CacheFileName.StartOfString, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    YoriLibFreeStringContents(&CacheFileName);
    if (hCache == INVALID_HANDLE_VALUE) {
        return;
    }

    YoriLibInitEmptyString(&LineString);

    while (TRUE) {
        if (!YoriLibReadLineToString(&LineString, &LineContext, hCache)) {
            break;
        }

        Entry = YoriLibMalloc(sizeof(MAKE_PREPROC_EXEC_CACHE_ENTRY));
        if (Entry == NULL) {
            break;
        }

        ZeroMemory(Entry, sizeof(MAKE_PREPROC_EXEC_CACHE_ENTRY));

        //
        //  The format of each line is expected to be:
        //  ExitCode:HashKey
        //
        //  Check that the first portion is numeric
        //

        if (!YoriLibStringToNumber(&LineString, FALSE, &llTemp, &CharsConsumed) ||
            CharsConsumed == 0) {

            YoriLibFree(Entry);
            break;
        }
        Entry->ExitCode = (DWORD)llTemp;

        //
        //  Check that the line is long enough to contain an actual command
        //

        if (CharsConsumed + 1 + sizeof(DWORD) * 2 * 2 >= LineString.LengthInChars) {
            YoriLibFree(Entry);
            break;
        }

        //
        //  Check that the seperator is where it should be
        //

        if (LineString.StartOfString[CharsConsumed] != ':') {
            YoriLibFree(Entry);
            break;
        }

        //
        //  Copy the trailing portion of the line so the hash package has
        //  an allocation that won't go away
        //

        if (!YoriLibAllocateString(&Key, LineString.LengthInChars - CharsConsumed - 1)) {
            YoriLibFree(Entry);
            break;
        }

        memcpy(Key.StartOfString, &LineString.StartOfString[CharsConsumed + 1], (LineString.LengthInChars - CharsConsumed - 1) * sizeof(TCHAR));
        Key.LengthInChars = LineString.LengthInChars - CharsConsumed - 1;

        YoriLibHashInsertByKey(MakeContext->PreprocessorCache, &Key, Entry, &Entry->HashEntry);

        YoriLibAppendList(&MakeContext->PreprocessorCacheList, &Entry->ListEntry);

        YoriLibFreeStringContents(&Key);
    }

    YoriLibLineReadCloseOrCache(LineContext);
    YoriLibFreeStringContents(&LineString);
    CloseHandle(hCache);
}

/**
 Deallocate all preprocessor cache entries and optionally write them to a
 file.

 @param MakeContext Pointer to the context.

 @param MakeFileName Pointer to the file name of the makefile.  If this
        contains a string, it will be used as the base name for the cache.
 */
VOID
MakeSaveAndDeleteAllPreprocessorCacheEntries(
    __inout PMAKE_CONTEXT MakeContext,
    __in PYORI_STRING MakeFileName
    )
{ 
    PYORI_LIST_ENTRY ListEntry = NULL;
    PMAKE_PREPROC_EXEC_CACHE_ENTRY Entry;
    YORI_STRING CacheFileName;
    HANDLE hCache;

    if (MakeContext->PreprocessorCache == NULL) {
        return;
    }

    hCache = NULL;
    YoriLibInitEmptyString(&CacheFileName);
    if (MakeGetCacheFileNameFromMakeFileName(MakeFileName, &CacheFileName)) {
        hCache = CreateFile(CacheFileName.StartOfString, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hCache == INVALID_HANDLE_VALUE) {
            hCache = NULL;
        }
        YoriLibFreeStringContents(&CacheFileName);
    }

    ListEntry = YoriLibGetNextListEntry(&MakeContext->PreprocessorCacheList, NULL);
    while (ListEntry != NULL) {
        Entry = CONTAINING_RECORD(ListEntry, MAKE_PREPROC_EXEC_CACHE_ENTRY, ListEntry);

        if (hCache != NULL) {
            YoriLibOutputToDevice(hCache, 0, _T("%i:%y\n"), Entry->ExitCode, &Entry->HashEntry.Key);
        }
        YoriLibRemoveListItem(&Entry->ListEntry);
        YoriLibHashRemoveByEntry(&Entry->HashEntry);
        YoriLibFree(Entry);
        ListEntry = YoriLibGetNextListEntry(&MakeContext->PreprocessorCacheList, NULL);
    }
    YoriLibFreeEmptyHashTable(MakeContext->PreprocessorCache);

    if (hCache != NULL) {
        CloseHandle(hCache);
    }
}

/**
 Given a command and a point in time in execution, calculate the cache key
 for the command.  The key consists of a hash of the environment, a hash of
 all makefile variables at this point in execution, and the command itself.

 @param ScopeContext Pointer to the scope context containing all currently
        in scope variables.

 @param Cmd Pointer to the command to execute.

 @param Key On successful completion, updated to contain a newly allocated
        string containing the key.  The caller is expected to free this
        with YoriLibFreeStringContents.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
MakeBuildKeyForCacheCmd(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Cmd,
    __out PYORI_STRING Key
    )
{
    YORI_STRING Env;
    YORI_STRING Substring;
    PMAKE_CONTEXT MakeContext;
    DWORD EnvHash;
    DWORD VarHash;

    MakeContext = ScopeContext->MakeContext;

    if (!MakeContext->EnvHashCalculated) {
        if (!YoriLibGetEnvironmentStrings(&Env)) {
            return FALSE;
        }

        MakeContext->EnvHash = YoriLibHashString32(0, &Env);
        MakeContext->EnvHashCalculated = TRUE;

        YoriLibFreeStringContents(&Env);
    }

    EnvHash = MakeContext->EnvHash;

    VarHash = MakeHashAllVariables(ScopeContext);

    if (!YoriLibAllocateString(Key, (sizeof(EnvHash) + sizeof(VarHash)) * 2 + Cmd->LengthInChars + 1)) {
        return FALSE;
    }

    YoriLibInitEmptyString(&Substring);
    Substring.StartOfString = Key->StartOfString;
    Substring.LengthAllocated = Key->LengthAllocated;
    YoriLibHexBufferToString((PUCHAR)&EnvHash, sizeof(EnvHash), &Substring);

#if defined(_MSC_VER) && (_MSC_VER >= 1500)

    //
    //  "Potential" mismatch between sizeof and countof.  I meant sizeof.
    //

#pragma warning(push)
#pragma warning(disable: 6305)
#endif

    Substring.StartOfString = Substring.StartOfString + sizeof(EnvHash) * 2;
    Substring.LengthInChars = Substring.LengthInChars - sizeof(EnvHash) * 2;

    YoriLibHexBufferToString((PUCHAR)&VarHash, sizeof(VarHash), &Substring);

    Substring.StartOfString = Substring.StartOfString + sizeof(VarHash) * 2;
    Substring.LengthInChars = Substring.LengthInChars - sizeof(VarHash) * 2;

    memcpy(Substring.StartOfString, Cmd->StartOfString, Cmd->LengthInChars * sizeof(TCHAR));
    Key->LengthInChars = (sizeof(EnvHash) + sizeof(VarHash)) * 2 + Cmd->LengthInChars;

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(pop)
#endif

    return TRUE;
}

/**
 Look for a matching entry in the cache for the specified preprocessor
 command.

 @param ScopeContext Pointer to the scope context.

 @param Cmd Pointer to the command to execute.

 @return Pointer to the cache entry if one was found, or NULL if no match
         was found.
 */
PMAKE_PREPROC_EXEC_CACHE_ENTRY
MakeLookupPreprocessorCache(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Cmd
    )
{
    PYORI_HASH_ENTRY HashEntry;
    PMAKE_PREPROC_EXEC_CACHE_ENTRY Entry;
    YORI_STRING Key;

    if (!MakeBuildKeyForCacheCmd(ScopeContext, Cmd, &Key)) {
        return NULL;
    }

    HashEntry = YoriLibHashLookupByKey(ScopeContext->MakeContext->PreprocessorCache, &Key);
    YoriLibFreeStringContents(&Key);
    if (HashEntry == NULL) {
        return NULL;
    }

    Entry = CONTAINING_RECORD(HashEntry, MAKE_PREPROC_EXEC_CACHE_ENTRY, HashEntry);
    return Entry;
}

/**
 Add an entry to the preprocessor cache.  This occurs after the child process
 has completed to record its result.

 @param ScopeContext Pointer to the scope context.

 @param Cmd Pointer to the command string.

 @param ExitCode Indicates the result of the command.
 */
VOID
MakeAddToPreprocessorCache(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Cmd,
    __in DWORD ExitCode
    )
{
    PMAKE_PREPROC_EXEC_CACHE_ENTRY Entry;
    PYORI_HASH_ENTRY HashEntry;
    YORI_STRING Key;

    if (!MakeBuildKeyForCacheCmd(ScopeContext, Cmd, &Key)) {
        return;
    }

    HashEntry = YoriLibHashLookupByKey(ScopeContext->MakeContext->PreprocessorCache, &Key);
    if (HashEntry != NULL) {
        YoriLibFreeStringContents(&Key);
        return;
    }

    Entry = YoriLibMalloc(sizeof(MAKE_PREPROC_EXEC_CACHE_ENTRY));
    if (Entry == NULL) {
        YoriLibFreeStringContents(&Key);
        return;
    }

    ZeroMemory(Entry, sizeof(MAKE_PREPROC_EXEC_CACHE_ENTRY));
    Entry->ExitCode = ExitCode;

    YoriLibHashInsertByKey(ScopeContext->MakeContext->PreprocessorCache, &Key, Entry, &Entry->HashEntry);

    YoriLibAppendList(&ScopeContext->MakeContext->PreprocessorCacheList, &Entry->ListEntry);
    YoriLibFreeStringContents(&Key);
}

/**
 Execute a subcommand and capture the result.  Currently this is used to
 evaluate preprocessor if statements only.

 @param ScopeContext Pointer to the scope context.

 @param Cmd Pointer to the command to execute.

 @return The exit code from the process, or 255 being the DOS exit code for
         a command that cannot execute.
 */
DWORD
MakeExecuteCommandCaptureExitCode(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Cmd
    )
{
    PMAKE_PREPROC_EXEC_CACHE_ENTRY Entry;
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    YORI_LIBSH_EXEC_PLAN ExecPlan;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    DWORD ExitCode;

    //
    //  Because DOS
    //

    ExitCode = 255;

#if MAKE_DEBUG_PREPROCESSOR_CREATEPROCESS
    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Executing preprocessor command: %y\n"), Cmd);
#endif

    QueryPerformanceCounter(&StartTime);

    if (ScopeContext->MakeContext->PreprocessorCache != NULL) {
        Entry = MakeLookupPreprocessorCache(ScopeContext, Cmd);
        if (Entry != NULL) {
            ExitCode = Entry->ExitCode;
            goto Complete;
        }
    }

    if (!YoriLibShParseCmdlineToCmdContext(Cmd, 0, &CmdContext)) {
        goto Complete;
    }

    if (!YoriLibShParseCmdContextToExecPlan(&CmdContext, &ExecPlan, NULL, NULL, NULL, NULL)) {
        YoriLibShFreeCmdContext(&CmdContext);
        goto Complete;
    }

    ExitCode = MakeShExecExecPlan(&ExecPlan, NULL);

    YoriLibShFreeExecPlan(&ExecPlan);
    YoriLibShFreeCmdContext(&CmdContext);

    if (ScopeContext->MakeContext->PreprocessorCache != NULL) {
        MakeAddToPreprocessorCache(ScopeContext, Cmd, ExitCode);
    }

Complete:

    QueryPerformanceCounter(&EndTime);
    ScopeContext->MakeContext->TimeInPreprocessorCreateProcess = ScopeContext->MakeContext->TimeInPreprocessorCreateProcess + EndTime.QuadPart - StartTime.QuadPart;
#if MAKE_DEBUG_PREPROCESSOR_CREATEPROCESS
    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("...took %lli\n"), EndTime.QuadPart - StartTime.QuadPart);
#endif

    return ExitCode;
}

/**
 Search through a string looking to see if any substrings can be located.
 Returns the first match in offet from the beginning of the string order.
 This routine looks for matches case sensitively.

 @param String The string to search through.

 @param NumberMatches The number of substrings to look for.

 @param MatchArray An array of strings corresponding to the matches to
        look for.

 @param StringOffsetOfMatch On successful completion, returns the offset
        within the string of the match.

 @return If a match is found, returns a pointer to the entry in MatchArray
         corresponding to the substring that was matched.  If no match is
         found, returns NULL.
 */
PYORI_STRING
MakeFindFirstMatchingSubstringSkipQuotes(
    __in PYORI_STRING String,
    __in DWORD NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PDWORD StringOffsetOfMatch
    )
{
    YORI_STRING RemainingString;
    DWORD CheckCount;
    BOOLEAN QuoteOpen;
    BOOLEAN BraceOpen;

    YoriLibInitEmptyString(&RemainingString);
    RemainingString.StartOfString = String->StartOfString;
    RemainingString.LengthInChars = String->LengthInChars;

    QuoteOpen = FALSE;
    BraceOpen = FALSE;

    //
    //  MSFIX Need to figure out the correct grammar for stupid expressions
    //  like ["]"
    //

    while (RemainingString.LengthInChars > 0) {

        if (RemainingString.StartOfString[0] == '"') {
            if (!QuoteOpen) {
                QuoteOpen = TRUE;
            } else {
                QuoteOpen = FALSE;
            }
        }

        if (!BraceOpen && RemainingString.StartOfString[0] == '[') {
            BraceOpen = TRUE;
        }

        if (BraceOpen && RemainingString.StartOfString[0] == ']') {
            BraceOpen = FALSE;
        }

        if (QuoteOpen || BraceOpen) {
            RemainingString.LengthInChars--;
            RemainingString.StartOfString++;
            continue;
        }

        for (CheckCount = 0; CheckCount < NumberMatches; CheckCount++) {
            if (YoriLibCompareStringCount(&RemainingString, &MatchArray[CheckCount], MatchArray[CheckCount].LengthInChars) == 0) {
                if (StringOffsetOfMatch != NULL) {
                    *StringOffsetOfMatch = String->LengthInChars - RemainingString.LengthInChars;
                }
                return &MatchArray[CheckCount];
            }
        }

        RemainingString.LengthInChars--;
        RemainingString.StartOfString++;
    }

    if (StringOffsetOfMatch != NULL) {
        *StringOffsetOfMatch = 0;
    }

    return NULL;
}

/**
 An array index for an operator indicating a match.
 */
#define MAKE_IF_OPERATOR_EXACT_MATCH      0

/**
 An array index for an operator indicating a mismatch.
 */
#define MAKE_IF_OPERATOR_NO_MATCH         1

/**
 An array index for the greater than or equal operator.
 */
#define MAKE_IF_OPERATOR_GREATER_OR_EQUAL 2

/**
 An array index for the less than or equal operator.
 */
#define MAKE_IF_OPERATOR_LESS_OR_EQUAL    3

/**
 An array index for the greater than operator.
 */
#define MAKE_IF_OPERATOR_GREATER          4

/**
 An array index for the less than operator.
 */
#define MAKE_IF_OPERATOR_LESS             5

/**
 An array index beyond the array, ie., the number of elements in the array.
 */
#define MAKE_IF_OPERATOR_BEYOND_MAX       6


/**
 Evaluate a single comparison in a preprocessor !IF expression, and return
 TRUE if the expression is TRUE or FALSE if it is FALSE.  On error, this
 function informs the user and marks execution to terminate.

 @param ScopeContext Pointer to the scope context.

 @param Expression Pointer to the expression to evaluate.

 @return TRUE if the condition is TRUE, FALSE if the condition is FALSE.
 */
BOOLEAN
MakePreprocessorEvaluateSingleCondition(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Expression
    )
{
    YORI_STRING OperatorMatches[MAKE_IF_OPERATOR_BEYOND_MAX];
    PMAKE_CONTEXT MakeContext;
    PYORI_STRING MatchingOperator;
    YORI_STRING FirstPart;
    YORI_STRING SecondPart;
    DWORD OperatorIndex;
    BOOLEAN FirstPartIsString;
    BOOLEAN SecondPartIsString;

    MakeContext = ScopeContext->MakeContext;

    YoriLibConstantString(&OperatorMatches[MAKE_IF_OPERATOR_EXACT_MATCH], _T("=="));
    YoriLibConstantString(&OperatorMatches[MAKE_IF_OPERATOR_NO_MATCH], _T("!="));
    YoriLibConstantString(&OperatorMatches[MAKE_IF_OPERATOR_GREATER_OR_EQUAL], _T(">="));
    YoriLibConstantString(&OperatorMatches[MAKE_IF_OPERATOR_LESS_OR_EQUAL], _T("<="));
    YoriLibConstantString(&OperatorMatches[MAKE_IF_OPERATOR_GREATER], _T(">"));
    YoriLibConstantString(&OperatorMatches[MAKE_IF_OPERATOR_LESS], _T("<"));

    MatchingOperator = MakeFindFirstMatchingSubstringSkipQuotes(Expression, sizeof(OperatorMatches)/sizeof(OperatorMatches[0]), OperatorMatches, &OperatorIndex);
    if (MatchingOperator == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Syntax error in expression: %y\n"), Expression);
        MakeContext->ErrorTermination = TRUE;
        return FALSE;
    }

    YoriLibInitEmptyString(&FirstPart);

    FirstPart.StartOfString = Expression->StartOfString;
    FirstPart.LengthInChars = OperatorIndex;

    YoriLibInitEmptyString(&SecondPart);

    SecondPart.StartOfString = Expression->StartOfString + OperatorIndex + MatchingOperator->LengthInChars;
    SecondPart.LengthInChars = Expression->LengthInChars - OperatorIndex - MatchingOperator->LengthInChars;

    MakeTrimWhitespace(&FirstPart);
    MakeTrimWhitespace(&SecondPart);

    FirstPartIsString = FALSE;
    if (FirstPart.LengthInChars > 0 && FirstPart.StartOfString[0] == '"') {
        FirstPartIsString = TRUE;
    }

    SecondPartIsString = FALSE;
    if (SecondPart.LengthInChars > 0 && SecondPart.StartOfString[0] == '"') {
        SecondPartIsString = TRUE;
    }

    if (FirstPartIsString != SecondPartIsString) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Syntax error in expression: %y\n"), Expression);
        MakeContext->ErrorTermination = TRUE;
        return FALSE;
    }

    if (FirstPartIsString) {

        if (MatchingOperator == &OperatorMatches[MAKE_IF_OPERATOR_EXACT_MATCH]) {
            if (YoriLibCompareString(&FirstPart, &SecondPart) == 0) {
                return TRUE;
            } else {
                return FALSE;
            }
        } else if (MatchingOperator == &OperatorMatches[MAKE_IF_OPERATOR_NO_MATCH]) {
            if (YoriLibCompareString(&FirstPart, &SecondPart) == 0) {
                return FALSE;
            } else {
                return TRUE;
            }
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Syntax error in expression: %y\n"), Expression);
            MakeContext->ErrorTermination = TRUE;
            return FALSE;
        }

    } else {
        LONGLONG FirstNumber;
        LONGLONG SecondNumber;
        DWORD CharsConsumed;

        //
        //  If nothing is specified, they're deemed to be zero.  If something
        //  is specified, it must be numeric.
        //

        if (FirstPart.LengthInChars == 0) {
            FirstNumber = 0;
        } else if (FirstPart.LengthInChars > 2 &&
                   FirstPart.StartOfString[0] == '[' &&
                   FirstPart.StartOfString[FirstPart.LengthInChars - 1] == ']') {
            YORI_STRING Substring;
            YoriLibInitEmptyString(&Substring);
            Substring.StartOfString = &FirstPart.StartOfString[1];
            Substring.LengthInChars = FirstPart.LengthInChars - 2;
            FirstNumber = MakeExecuteCommandCaptureExitCode(ScopeContext, &Substring);
        } else {
            if (!YoriLibStringToNumber(&FirstPart, TRUE, &FirstNumber, &CharsConsumed) || CharsConsumed == 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Syntax error in expression: %y\n"), Expression);
                MakeContext->ErrorTermination = TRUE;
                return FALSE;
            }
        }

        if (SecondPart.LengthInChars == 0) {
            SecondNumber = 0;
        } else if (SecondPart.LengthInChars > 2 &&
                   SecondPart.StartOfString[0] == '[' &&
                   SecondPart.StartOfString[SecondPart.LengthInChars - 1] == ']') {
            YORI_STRING Substring;
            YoriLibInitEmptyString(&Substring);
            Substring.StartOfString = &SecondPart.StartOfString[1];
            Substring.LengthInChars = SecondPart.LengthInChars - 2;
            SecondNumber = MakeExecuteCommandCaptureExitCode(ScopeContext, &Substring);
        } else {
            if (!YoriLibStringToNumber(&SecondPart, TRUE, &SecondNumber, &CharsConsumed) || CharsConsumed == 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Syntax error in expression: %y\n"), Expression);
                MakeContext->ErrorTermination = TRUE;
                return FALSE;
            }
        }

        if (MatchingOperator == &OperatorMatches[MAKE_IF_OPERATOR_EXACT_MATCH]) {

            if (FirstNumber == SecondNumber) {
                return TRUE;
            } else {
                return FALSE;
            }

        } else if (MatchingOperator == &OperatorMatches[MAKE_IF_OPERATOR_NO_MATCH]) {

            if (FirstNumber != SecondNumber) {
                return TRUE;
            } else {
                return FALSE;
            }

        } else if (MatchingOperator == &OperatorMatches[MAKE_IF_OPERATOR_GREATER_OR_EQUAL]) {

            if (FirstNumber >= SecondNumber) {
                return TRUE;
            } else {
                return FALSE;
            }

        } else if (MatchingOperator == &OperatorMatches[MAKE_IF_OPERATOR_LESS_OR_EQUAL]) {

            if (FirstNumber <= SecondNumber) {
                return TRUE;
            } else {
                return FALSE;
            }

        } else if (MatchingOperator == &OperatorMatches[MAKE_IF_OPERATOR_GREATER]) {

            if (FirstNumber > SecondNumber) {
                return TRUE;
            } else {
                return FALSE;
            }

        } else if (MatchingOperator == &OperatorMatches[MAKE_IF_OPERATOR_LESS]) {

            if (FirstNumber < SecondNumber) {
                return TRUE;
            } else {
                return FALSE;
            }

        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Syntax error in expression: %y\n"), Expression);
            MakeContext->ErrorTermination = TRUE;
            return FALSE;
        }
    }

    ASSERT(FALSE);
    return FALSE;
}

/**
 An array index for an operator indicating an and condition.
 */
#define MAKE_IF_COMPOUND_OPERATOR_AND        0

/**
 An array index for an operator indicating an or condition.
 */
#define MAKE_IF_COMPOUND_OPERATOR_OR         1

/**
 An array index beyond the array, ie., the number of elements in the array.
 */
#define MAKE_IF_COMPOUND_OPERATOR_BEYOND_MAX 2


/**
 Evaluate a preprocessor !IF expression, and return TRUE if the expression
 is TRUE or FALSE if it is FALSE.  On error, this function informs the user
 and marks execution to terminate.

 @param ScopeContext Pointer to the scope context.

 @param Expression Pointer to the expression to evaluate.

 @return TRUE if the condition is TRUE, FALSE if the condition is FALSE.
 */
BOOLEAN
MakePreprocessorEvaluateCondition(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Expression
    )
{
    YORI_STRING OperatorMatches[MAKE_IF_COMPOUND_OPERATOR_BEYOND_MAX];
    PMAKE_CONTEXT MakeContext;
    PYORI_STRING MatchingOperator;
    PYORI_STRING PreviousMatchingOperator;
    YORI_STRING Current;
    YORI_STRING Remaining;
    DWORD OperatorIndex;
    BOOLEAN CumulativeResult;
    BOOLEAN CurrentResult;

    MakeContext = ScopeContext->MakeContext;

    YoriLibConstantString(&OperatorMatches[MAKE_IF_COMPOUND_OPERATOR_AND], _T("&&"));
    YoriLibConstantString(&OperatorMatches[MAKE_IF_COMPOUND_OPERATOR_OR], _T("||"));

    YoriLibInitEmptyString(&Remaining);
    YoriLibInitEmptyString(&Current);
    Remaining.StartOfString = Expression->StartOfString;
    Remaining.LengthInChars = Expression->LengthInChars;
    CumulativeResult = FALSE;
    PreviousMatchingOperator = NULL;

    while(Remaining.LengthInChars > 0) {

        MatchingOperator = MakeFindFirstMatchingSubstringSkipQuotes(&Remaining, sizeof(OperatorMatches)/sizeof(OperatorMatches[0]), OperatorMatches, &OperatorIndex);
        if (MatchingOperator == NULL) {
            Current.StartOfString = Remaining.StartOfString;
            Current.LengthInChars = Remaining.LengthInChars;
            Remaining.LengthInChars = 0;
        } else {

            Current.StartOfString = Remaining.StartOfString;
            Current.LengthInChars = OperatorIndex;

            Remaining.StartOfString = Remaining.StartOfString + OperatorIndex + MatchingOperator->LengthInChars;
            Remaining.LengthInChars = Remaining.LengthInChars - OperatorIndex - MatchingOperator->LengthInChars;
        }

        MakeTrimWhitespace(&Current);
        MakeTrimWhitespace(&Remaining);

        CurrentResult = MakePreprocessorEvaluateSingleCondition(ScopeContext, &Current);
        if (MakeContext->ErrorTermination) {
            return FALSE;
        }

        if (PreviousMatchingOperator == &OperatorMatches[MAKE_IF_COMPOUND_OPERATOR_AND]) {
            if (CumulativeResult && CurrentResult) {
                CumulativeResult = TRUE;
            } else {
                CumulativeResult = FALSE;
            }
        } else if (PreviousMatchingOperator == &OperatorMatches[MAKE_IF_COMPOUND_OPERATOR_OR]) {
            if (CumulativeResult || CurrentResult) {
                CumulativeResult = TRUE;
            } else {
                CumulativeResult = FALSE;
            }
        } else {
            ASSERT(PreviousMatchingOperator == NULL);
            CumulativeResult = CurrentResult;
        }

        PreviousMatchingOperator = MatchingOperator;
    }

    return CumulativeResult;
}

/**
 Include a new makefile at the current line.

 @param ScopeContext Pointer to the scope context.

 @param Name Pointer to a string indicating the file to include.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeInclude(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Name
    )
{
    YORI_STRING FullPath;
    YORI_STRING SavedCurrentIncludeDirectory;
    LPTSTR FilePart;
    HANDLE hStream;

    while(Name->LengthInChars > 0 && Name->StartOfString[0] == '"') {
        Name->StartOfString++;
        Name->LengthInChars--;
    }

    while(Name->LengthInChars > 0 && Name->StartOfString[Name->LengthInChars - 1] == '"') {
        Name->LengthInChars--;
    }

    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibGetFullPathNameRelativeTo(&ScopeContext->CurrentIncludeDirectory, Name, FALSE, &FullPath, &FilePart)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not open include file: %y\n"), Name);
        ScopeContext->MakeContext->ErrorTermination = TRUE;
        return FALSE;
    }

    hStream = CreateFile(FullPath.StartOfString, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (hStream == INVALID_HANDLE_VALUE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not open include file: %y\n"), &FullPath);
        YoriLibFreeStringContents(&FullPath);
        ScopeContext->MakeContext->ErrorTermination = TRUE;
        return FALSE;
    }

    memcpy(&SavedCurrentIncludeDirectory, &ScopeContext->CurrentIncludeDirectory, sizeof(YORI_STRING));
    YoriLibCloneString(&ScopeContext->CurrentIncludeDirectory, &FullPath);
    ScopeContext->CurrentIncludeDirectory.LengthInChars = (DWORD)((FilePart - ScopeContext->CurrentIncludeDirectory.StartOfString) - 1);

    if (!MakeProcessStream(hStream, ScopeContext->MakeContext, &FullPath)) {
#if MAKE_DEBUG_PREPROCESSOR
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ERROR: MakeProcessStream failed: %y\n"), &FullPath);
#endif
    }
    CloseHandle(hStream);

    YoriLibFreeStringContents(&ScopeContext->CurrentIncludeDirectory);
    memcpy(&ScopeContext->CurrentIncludeDirectory, &SavedCurrentIncludeDirectory, sizeof(YORI_STRING));
    YoriLibFreeStringContents(&FullPath);
    return TRUE;
}

/**
 Execute a preprocessor line when the line is in scope (not excluded by an
 earlier if condition.)

 @param ScopeContext Pointer to the scope context.

 @param Line Pointer to the line to execute.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakePreprocessor(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Line
    )
{
    MAKE_PREPROCESSOR_LINE_TYPE PreprocessorLineType;
    DWORD ArgOffset;
    YORI_STRING Arg;
    PMAKE_CONTEXT MakeContext;

    MakeContext = ScopeContext->MakeContext;

    PreprocessorLineType = MakeDeterminePreprocessorLineType(Line, &ArgOffset);
    YoriLibInitEmptyString(&Arg);
    if (ArgOffset < Line->LengthInChars) {
        Arg.StartOfString = &Line->StartOfString[ArgOffset];
        Arg.LengthInChars = Line->LengthInChars - ArgOffset;
        MakeTrimWhitespace(&Arg);
    }

    switch(PreprocessorLineType) {
        case MakePreprocessorLineTypeElse:
            if (ScopeContext->CurrentConditionalNestingLevel == ScopeContext->ActiveConditionalNestingLevel &&
                !ScopeContext->ActiveConditionalNestingLevelExecutionOccurred) {

                ScopeContext->ActiveConditionalNestingLevelExecutionEnabled = TRUE;
                ScopeContext->ActiveConditionalNestingLevelExecutionOccurred = TRUE;
            }
            break;
        case MakePreprocessorLineTypeElseIf:
            if (ScopeContext->CurrentConditionalNestingLevel == ScopeContext->ActiveConditionalNestingLevel &&
                !ScopeContext->ActiveConditionalNestingLevelExecutionOccurred) {

                if (MakePreprocessorEvaluateCondition(ScopeContext, &Arg)) {
                    ScopeContext->ActiveConditionalNestingLevelExecutionEnabled = TRUE;
                    ScopeContext->ActiveConditionalNestingLevelExecutionOccurred = TRUE;
                }
            }
            break;
        case MakePreprocessorLineTypeElseIfDef:
            ASSERT(ScopeContext->CurrentConditionalNestingLevel == ScopeContext->ActiveConditionalNestingLevel);
            ASSERT(!ScopeContext->ActiveConditionalNestingLevelExecutionOccurred);

            if (MakeIsVariableDefined(ScopeContext, &Arg)) {
                ScopeContext->ActiveConditionalNestingLevelExecutionEnabled = TRUE;
                ScopeContext->ActiveConditionalNestingLevelExecutionOccurred = TRUE;
            }
            break;
        case MakePreprocessorLineTypeElseIfNDef:
            ASSERT(ScopeContext->CurrentConditionalNestingLevel == ScopeContext->ActiveConditionalNestingLevel);
            ASSERT(!ScopeContext->ActiveConditionalNestingLevelExecutionOccurred);

            if (!MakeIsVariableDefined(ScopeContext, &Arg)) {
                ScopeContext->ActiveConditionalNestingLevelExecutionEnabled = TRUE;
                ScopeContext->ActiveConditionalNestingLevelExecutionOccurred = TRUE;
            }
            break;
        case MakePreprocessorLineTypeEndIf:
            ASSERT(!"Preprocessor endif should be handled in minimal path only");
            break;
        case MakePreprocessorLineTypeError:
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y\n"), &Arg);
            MakeContext->ErrorTermination = TRUE;
            break;
        case MakePreprocessorLineTypeIf:
            if (MakePreprocessorEvaluateCondition(ScopeContext, &Arg)) {
                MakeBeginNestedConditionTrue(ScopeContext);
            } else {
                MakeBeginNestedConditionFalse(ScopeContext);
            }
            break;
        case MakePreprocessorLineTypeIfDef:
            ASSERT(ScopeContext->CurrentConditionalNestingLevel == ScopeContext->ActiveConditionalNestingLevel);

            if (MakeIsVariableDefined(ScopeContext, &Arg)) {
                MakeBeginNestedConditionTrue(ScopeContext);
            } else {
                MakeBeginNestedConditionFalse(ScopeContext);
            }
            break;
        case MakePreprocessorLineTypeIfNDef:
            ASSERT(ScopeContext->CurrentConditionalNestingLevel == ScopeContext->ActiveConditionalNestingLevel);

            if (!MakeIsVariableDefined(ScopeContext, &Arg)) {
                MakeBeginNestedConditionTrue(ScopeContext);
            } else {
                MakeBeginNestedConditionFalse(ScopeContext);
            }
            break;
        case MakePreprocessorLineTypeInclude:
            MakeInclude(ScopeContext, &Arg);
            break;
        case MakePreprocessorLineTypeMessage:
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &Arg);
            break;
        case MakePreprocessorLineTypeUndef:
            MakeSetVariable(ScopeContext, &Arg, NULL, FALSE, MakeVariablePrecedenceMakefile);
            break;
    }

    return TRUE;
}

/**
 Return TRUE if the specified target is a rule describing an inference
 rule, which converts files of one type into another type regardless of
 file name.

 @param Line Pointer to the line.  This is expected to be a rule type line.

 @param FromDir Optionally points to a string that is populated with the
        source directory defined by this inference rule if the line
        is an inference rule.  This may be an empty string to refer to the
        current scope directory.

 @param FromExt Optionally points to a string that is populated with the
        source file extension defined by this inference rule if the line
        is an inference rule.

 @param ToDir Optionally points to a string that is populated with the
        target directory defined by this inference rule if the line
        is an inference rule.  This may be an empty string to refer to the
        current scope directory.

 @param ToExt Optionally points to a string that is populated with the
        target file extension defined by this inference rule if the line
        is an inference rule.

 @return TRUE to indicate that this line is an inference rule, FALSE if it
         is a regular file rule.
 */
__success(return)
BOOLEAN
MakeIsTargetInferenceRule(
    __in PYORI_STRING Line,
    __out_opt PYORI_STRING FromDir,
    __out_opt PYORI_STRING FromExt,
    __out_opt PYORI_STRING ToDir,
    __out_opt PYORI_STRING ToExt
    )
{
    DWORD Index;
    BOOLEAN FoundFirstExt;
    YORI_STRING LocalFromDir;
    YORI_STRING LocalFromExt;
    YORI_STRING LocalToDir;
    YORI_STRING LocalToExt;
    YORI_STRING Remaining;

    YoriLibInitEmptyString(&LocalFromDir);
    YoriLibInitEmptyString(&LocalFromExt);
    YoriLibInitEmptyString(&LocalToDir);
    YoriLibInitEmptyString(&LocalToExt);
    FoundFirstExt = FALSE;

    if (Line->LengthInChars < 1 ||
        (Line->StartOfString[0] != '.' &&
         Line->StartOfString[0] != '{')) {

        return FALSE;
    }

    Remaining.StartOfString = Line->StartOfString;
    Remaining.LengthInChars = Line->LengthInChars;

    if (Remaining.StartOfString[0] == '{') {
        LocalFromDir.StartOfString = &Remaining.StartOfString[1];
        for (Index = 1; Index < Remaining.LengthInChars; Index++) {
            if (Remaining.StartOfString[Index] == '}') {
                LocalFromDir.LengthInChars = Index - 1;
                break;
            }
        }
        if (Index == Remaining.LengthInChars) {
            return FALSE;
        }
        Remaining.StartOfString = Remaining.StartOfString + LocalFromDir.LengthInChars + 2;
        Remaining.LengthInChars = Remaining.LengthInChars - LocalFromDir.LengthInChars - 2;
    }

    if (Remaining.LengthInChars < 1 || 
        Remaining.StartOfString[0] != '.') {
        return FALSE;
    }

    LocalFromExt.StartOfString = &Remaining.StartOfString[1];
    for (Index = 1; Index < Remaining.LengthInChars; Index++) {
        if (Remaining.StartOfString[Index] == ':' ||
            YoriLibIsSep(Remaining.StartOfString[Index])) {
            return FALSE;
        }
        if (Remaining.StartOfString[Index] == '.' ||
            Remaining.StartOfString[Index] == '{') {

            LocalFromExt.LengthInChars = Index - 1;
            break;
        }
    }

    if (Index == Remaining.LengthInChars) {
        return FALSE;
    }

    Remaining.StartOfString = &Remaining.StartOfString[Index];
    Remaining.LengthInChars = Remaining.LengthInChars - Index;

    if (Remaining.LengthInChars < 1) {
        return FALSE;
    }

    if (Remaining.StartOfString[0] == '{') {
        LocalToDir.StartOfString = &Remaining.StartOfString[1];
        for (Index = 1; Index < Remaining.LengthInChars; Index++) {
            if (Remaining.StartOfString[Index] == '}') {
                LocalToDir.LengthInChars = Index - 1;
                break;
            }
        }
        if (Index == Remaining.LengthInChars) {
            return FALSE;
        }
        Remaining.StartOfString = Remaining.StartOfString + LocalToDir.LengthInChars + 2;
        Remaining.LengthInChars = Remaining.LengthInChars - LocalToDir.LengthInChars - 2;
    }

    if (Remaining.LengthInChars < 1 || 
        Remaining.StartOfString[0] != '.') {
        return FALSE;
    }

    LocalToExt.StartOfString = &Remaining.StartOfString[1];
    for (Index = 1; Index < Remaining.LengthInChars; Index++) {
        if (YoriLibIsSep(Remaining.StartOfString[Index])) {
            return FALSE;
        }
        if (Remaining.StartOfString[Index] == ':') {
            LocalToExt.LengthInChars = Index - 1;
            break;
        }
    }

    if (Index == Remaining.LengthInChars) {
        return FALSE;
    }

    MakeTrimWhitespace(&LocalFromDir);
    MakeTrimWhitespace(&LocalFromExt);
    MakeTrimWhitespace(&LocalToDir);
    MakeTrimWhitespace(&LocalToExt);
    MakeTrimSeparators(&LocalFromDir);
    MakeTrimSeparators(&LocalToDir);

    if (FromDir != NULL) {
        memcpy(FromDir, &LocalFromDir, sizeof(YORI_STRING));
    }

    if (FromExt != NULL) {
        memcpy(FromExt, &LocalFromExt, sizeof(YORI_STRING));
    }

    if (ToDir != NULL) {
        memcpy(ToDir, &LocalToDir, sizeof(YORI_STRING));
    }

    if (ToExt != NULL) {
        memcpy(ToExt, &LocalToExt, sizeof(YORI_STRING));
    }

    return TRUE;
}

/**
 A list of file names to search for in each subdirectory when looking for
 a valid makefile.
 */
CONST YORI_STRING MakefileNameCandidates[] = {
    YORILIB_CONSTANT_STRING(_T("YMkFile")),
    YORILIB_CONSTANT_STRING(_T("Makefile"))
};

/**
 Find the first existing makefile in a directory specified by the scope
 context.

 @param ScopeContext Pointer to the scope context.

 @param FileName On successful completion, populated with a newly allocated
        string indicating the full path name to the makefile.

 @return TRUE to indicate that a makefile was found, FALSE to indicate it was
         not found or an error occurred.
 */
__success(return)
BOOLEAN
MakeFindMakefileInDirectory(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __out PYORI_STRING FileName
    )
{
    YORI_STRING ProbeName;
    DWORD Index;
    DWORD LongestName;

    LongestName = 0;
    for (Index = 0; Index < sizeof(MakefileNameCandidates)/sizeof(MakefileNameCandidates[0]); Index++) {
        if (MakefileNameCandidates[Index].LengthInChars > LongestName) {
            LongestName = MakefileNameCandidates[Index].LengthInChars;
        }
    }

    if (!YoriLibAllocateString(&ProbeName, ScopeContext->HashEntry.Key.LengthInChars + 1 + LongestName + 1)) {
        return FALSE;
    }

    for (Index = 0; Index < sizeof(MakefileNameCandidates)/sizeof(MakefileNameCandidates[0]); Index++) {
        ProbeName.LengthInChars = YoriLibSPrintf(ProbeName.StartOfString, _T("%y\\%y"), &ScopeContext->HashEntry.Key, &MakefileNameCandidates[Index]);
        if (GetFileAttributes(ProbeName.StartOfString) != (DWORD)-1) {
            memcpy(FileName, &ProbeName, sizeof(YORI_STRING));
            return TRUE;
        }
    }

    YoriLibFreeStringContents(&ProbeName);
    return FALSE;
}

/**
 Parse extended information about a target.  These options are enclosed in
 square braces.

 @param TargetName Pointer to the target name, including all extended options.

 @param DependenciesAreDirectories On successful completion, set to TRUE to
        indicate that the targets which are required before this target can
        be built refer to child directory makefile rules as opposed to files
        or targets.

 @param ChildTargetName On successful completion, populated with the name of
        a target to search for in child directory makefiles.  This is only
        meaningful if DependenciesAreDirectories is set to TRUE.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
MakeDetermineTargetOptions(
    __inout PYORI_STRING TargetName,
    __out PBOOLEAN DependenciesAreDirectories,
    __inout PYORI_STRING ChildTargetName
    )
{
    LPTSTR FirstBrace;
    LPTSTR LastBrace;
    LPTSTR Space;
    LPTSTR Equals;
    YORI_STRING Component;
    YORI_STRING OptionString;
    YORI_STRING ValueString;

    *DependenciesAreDirectories = FALSE;
    FirstBrace = YoriLibFindLeftMostCharacter(TargetName, '[');

    if (FirstBrace == NULL) {
        return TRUE;
    }

    YoriLibInitEmptyString(&OptionString);
    OptionString.StartOfString = &FirstBrace[1];
    OptionString.LengthInChars = TargetName->LengthInChars - (DWORD)(FirstBrace - TargetName->StartOfString) - 1;

    LastBrace = YoriLibFindLeftMostCharacter(&OptionString, ']');
    if (LastBrace == NULL) {
        return FALSE;
    }

    if (LastBrace != &OptionString.StartOfString[OptionString.LengthInChars - 1]) {
        return FALSE;
    }
    OptionString.LengthInChars = OptionString.LengthInChars - 1;

    TargetName->LengthInChars = (DWORD)(FirstBrace - TargetName->StartOfString);

    Space = YoriLibFindLeftMostCharacter(&OptionString, ' ');
    while (TRUE) {

        Component.StartOfString = OptionString.StartOfString;
        if (Space != NULL) {
            Component.LengthInChars = (DWORD)(Space - OptionString.StartOfString);
            OptionString.StartOfString = OptionString.StartOfString + Component.LengthInChars + 1;
            OptionString.LengthInChars = OptionString.LengthInChars - Component.LengthInChars - 1;
        } else {
            Component.LengthInChars = OptionString.LengthInChars;
        }

        if (YoriLibCompareStringWithLiteralInsensitive(&Component, _T("dirs")) == 0) {
            *DependenciesAreDirectories = TRUE;
        } else {
            Equals = YoriLibFindLeftMostCharacter(&Component, '=');
            if (Equals != NULL) {
                YoriLibInitEmptyString(&ValueString);
                ValueString.StartOfString = Equals + 1;
                ValueString.LengthInChars = Component.LengthInChars - (DWORD)(Equals - Component.StartOfString) - 1;
                Component.LengthInChars = (DWORD)(Equals - Component.StartOfString);

                if (YoriLibCompareStringWithLiteralInsensitive(&Component, _T("target")) == 0) {
                    ChildTargetName->StartOfString = ValueString.StartOfString;
                    ChildTargetName->LengthInChars = ValueString.LengthInChars;
                }
            }
        }

        if (Space == NULL) {
            break;
        }

        Space = YoriLibFindLeftMostCharacter(&OptionString, ' ');
    }

    if (YoriLibCompareStringWithLiteralInsensitive(&OptionString, _T("dirs")) == 0) {
        *DependenciesAreDirectories = TRUE;
    }

    return TRUE;
}

/**
 Add a single target as a prerequisite for another target.

 @param MakeContext The global context.

 @param ChildTarget Pointer to the target which depends on this entry.

 @param ParentDependency Pointer to the name of the target to execute before
        ChildTarget.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeCreateRuleDependency(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET ChildTarget,
    __in PYORI_STRING ParentDependency
    )
{
    PMAKE_TARGET RequiredParentTarget;
    PMAKE_SCOPE_CONTEXT ScopeContext;

    ScopeContext = MakeContext->ActiveScope;

    RequiredParentTarget = MakeLookupOrCreateTarget(ScopeContext, ParentDependency, FALSE);
    if (RequiredParentTarget == NULL) {
        return FALSE;
    }

    MakeMarkTargetInferenceRuleNeededIfNeeded(ScopeContext, RequiredParentTarget);

    if (!MakeCreateParentChildDependency(MakeContext, RequiredParentTarget, ChildTarget)) {
        return FALSE;
    }

    return TRUE;
}

/**
 Enumerate the contents of a file and treat each list as a prerequisite target
 for another target.

 @param MakeContext The global context.

 @param ChildTarget Pointer to the target which depends on every line of the
        specified file.

 @param ParentDependency Pointer to the name of the file, prepended with an
        "@".

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeCreateFileListDependency(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET ChildTarget,
    __in PYORI_STRING ParentDependency
    )
{
    YORI_STRING FullPath;
    PMAKE_SCOPE_CONTEXT ScopeContext;
    YORI_STRING FileName;
    YORI_STRING LineString;
    PVOID LineContext;
    HANDLE hStream;
    DWORD Index;
    BOOLEAN Result;

    ScopeContext = MakeContext->ActiveScope;

    YoriLibInitEmptyString(&FileName);
    FileName.StartOfString = &ParentDependency->StartOfString[1];
    FileName.LengthInChars = ParentDependency->LengthInChars - 1;

    if (FileName.LengthInChars == 0) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&FullPath, ScopeContext->HashEntry.Key.LengthInChars + 1 + FileName.LengthInChars + 1)) {
        return FALSE;
    }

    FullPath.LengthInChars = YoriLibSPrintf(FullPath.StartOfString, _T("%y\\%y"), &ScopeContext->HashEntry.Key, &FileName);

    hStream = CreateFile(FullPath.StartOfString, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (hStream == INVALID_HANDLE_VALUE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not open include file: %y\n"), &FullPath);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    LineContext = NULL;
    Result = TRUE;
    YoriLibInitEmptyString(&LineString);

    while(TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, hStream)) {
            break;
        }

        //
        //  A pipe is an illegal character in a file name.  Treat it as the
        //  end of the string, just so other tools can add extra characters.
        //  (This is really a horrible hack for ypm to create packages.)
        //

        for (Index = 0; Index < LineString.LengthInChars; Index++) {
            if (LineString.StartOfString[Index] == '|') {
                LineString.LengthInChars = Index;
            }
        }

        if (!MakeCreateRuleDependency(MakeContext, ChildTarget, &LineString)) {
            Result = FALSE;
            break;
        }
    }

    YoriLibLineReadCloseOrCache(LineContext);
    YoriLibFreeStringContents(&LineString);
    YoriLibFreeStringContents(&FullPath);
    CloseHandle(hStream);

    return Result;
}

/**
 Add a single entry as a prerequisite for the specified target.  The entry
 refers to a child directory and a target that is defined by a makefile
 within that directory.

 @param MakeContext The global context.

 @param ChildTarget Pointer to the target which depends on this entry.

 @param ParentDependencyDirectory Pointer to a relative subdirectory name
        that defines the execution rule.

 @param ParentDependencyTarget Pointer to the name of the target defined
        within a makefile in the above directory.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeCreateSubdirectoryDependency(
    __in PMAKE_CONTEXT MakeContext,
    __in PMAKE_TARGET ChildTarget,
    __in PYORI_STRING ParentDependencyDirectory,
    __in PYORI_STRING ParentDependencyTarget
    )
{
    BOOLEAN Return;
    HANDLE hStream;
    YORI_STRING FullPath;
    BOOLEAN FoundExisting;

    if (!MakeActivateScope(MakeContext, ParentDependencyDirectory, &FoundExisting)) {
        return FALSE;
    }

    Return = FALSE;
    YoriLibInitEmptyString(&FullPath);

    if (!FoundExisting) {

        if (!MakeFindMakefileInDirectory(MakeContext->ActiveScope, &FullPath)) {
            YoriLibInitEmptyString(&FullPath);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not find makefile in directory: %y\n"), &MakeContext->ActiveScope->HashEntry.Key);
            goto Exit;
        }

        hStream = CreateFile(FullPath.StartOfString, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
        if (hStream == INVALID_HANDLE_VALUE) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not open include file: %y\n"), &FullPath);
            goto Exit;
        }

        if (!MakeProcessStream(hStream, MakeContext, &FullPath)) {
#if MAKE_DEBUG_PREPROCESSOR
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ERROR: MakeProcessStream failed: %y\n"), &FullPath);
#endif
        }
        CloseHandle(hStream);
    }

    if (!MakeCreateRuleDependency(MakeContext, ChildTarget, ParentDependencyTarget)) {
        goto Exit;
    }

    Return = TRUE;

Exit:
    YoriLibFreeStringContents(&FullPath);
    MakeDeactivateScope(MakeContext->ActiveScope);
    return Return;
}


/**
 Parse a single dependency line into a series of targets.  The target of all
 of the dependencies is returned from this function, but the dependencies are
 also parsed and populated as targets themselves.

 @param ScopeContext Pointer to the scope context.

 @param Line Pointer to the line to parse.

 @return Pointer to the target that this line is describing how to build.
 */
PMAKE_TARGET
MakeAddRule(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Line
    )
{
    YORI_STRING Substring;
    LPTSTR Colon;
    DWORD ReadIndex;
    BOOLEAN SwallowingWhitespace;
    PMAKE_CONTEXT MakeContext;
    PMAKE_TARGET Target;
    YORI_STRING FromDir;
    YORI_STRING FromExt;
    YORI_STRING ToDir;
    YORI_STRING ToExt;
    YORI_STRING ParentTargetName;
    BOOLEAN Subdirectories;
    BOOLEAN QuoteOpen;

    ASSERT(ScopeContext->ParserState == MakeParserRecipeActive);

    YoriLibInitEmptyString(&Substring);

    Substring.StartOfString = Line->StartOfString;
    Colon = YoriLibFindLeftMostCharacter(Line, ':');
    Substring.LengthInChars = (DWORD)(Colon - Line->StartOfString);
    ReadIndex = Substring.LengthInChars + 2;

    MakeTrimWhitespace(&Substring);

    Subdirectories = FALSE;
    YoriLibInitEmptyString(&ParentTargetName);
    if (!MakeDetermineTargetOptions(&Substring, &Subdirectories, &ParentTargetName)) {
        Subdirectories = FALSE;
    }

    //
    //  Ignore .SUFFIXES.  Currently YMAKE doesn't care about these and
    //  just applies the inference rules it has seen.  The main reason
    //  for discarding them completely is so they're not used as the
    //  first target/default target in a makefile.
    //
    //  MSFIX I think the real reason .SUFFIXES exist is to specify the
    //  order in which file extensions are probed.  For a build system
    //  it's good for this to be deterministic, and it's not always
    //  clear from reading a series of nested makefiles what the real
    //  order is.
    //

    if (YoriLibCompareStringWithLiteralInsensitive(&Substring, _T(".SUFFIXES")) == 0) {
        ScopeContext->ParserState = MakeParserDefault;
        return NULL;
    }

    //
    //  If a target is found, NMAKE preserves any existing recipe, to support
    //  having lines specify dependencies that are different to the ones
    //  providing recipes.  Here any recipes are effectively concatenated,
    //  except for inference rules.
    //

    if (MakeIsTargetInferenceRule(Line, &FromDir, &FromExt, &ToDir, &ToExt)) {
        PMAKE_INFERENCE_RULE InferenceRule;

        Target = MakeLookupOrCreateTarget(ScopeContext, &Substring, TRUE);
        if (Target == NULL) {
            return NULL;
        }

        YoriLibFreeStringContents(&Target->Recipe);
        InferenceRule = MakeCreateInferenceRule(ScopeContext, &FromDir, &FromExt, &ToDir, &ToExt, Target);
        if (InferenceRule == NULL) {
            return NULL;
        }

        Target->InferenceRulePseudoTarget = TRUE;
    } else {

        Target = MakeLookupOrCreateTarget(ScopeContext, &Substring, FALSE);
        if (Target == NULL) {
            return NULL;
        }

        MakeMarkTargetInferenceRuleNeededIfNeeded(ScopeContext, Target);
    }

#if MAKE_DEBUG_TARGET
    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Explicit recipe found for %y\n"), &Target->HashEntry.Key);
#endif

    Target->ExplicitRecipeFound = TRUE;
    if (Target->ScopeContext != NULL) {
        MakeDereferenceScope(Target->ScopeContext);
    }
    MakeReferenceScope(ScopeContext);
    Target->ScopeContext = ScopeContext;

    MakeContext = ScopeContext->MakeContext;

    SwallowingWhitespace = TRUE;
    QuoteOpen = FALSE;
    Substring.LengthInChars = 0;
    for (; ReadIndex < Line->LengthInChars; ReadIndex++) {

        if (Line->StartOfString[ReadIndex] == '"') {
            if (QuoteOpen) {
                QuoteOpen = FALSE;
            } else {
                QuoteOpen = TRUE;
            }
        }

        if (SwallowingWhitespace) {

            if (Line->StartOfString[ReadIndex] == ' ' ||
                Line->StartOfString[ReadIndex] == '\t') {

                continue;
            }

            SwallowingWhitespace = FALSE;
            Substring.StartOfString = &Line->StartOfString[ReadIndex];
            Substring.LengthInChars = 1;
        } else {
            if (!QuoteOpen &&
                (Line->StartOfString[ReadIndex] == ' ' ||
                 Line->StartOfString[ReadIndex] == '\t')) {

                SwallowingWhitespace = TRUE;

                //
                //  If the string is quoted and has contents, strip off the
                //  quotes.
                //

                if (Substring.LengthInChars >= 3 &&
                    Substring.StartOfString[0] == '"' &&
                    Substring.StartOfString[Substring.LengthInChars - 1] == '"') {

                    Substring.StartOfString++;
                    Substring.LengthInChars = Substring.LengthInChars - 2;
                }

                if (Subdirectories) {
                    if (!MakeCreateSubdirectoryDependency(MakeContext, Target, &Substring, &ParentTargetName)) {
                        return NULL;
                    }
                } else {
                    if (Substring.StartOfString[0] == '@') {
                        if (!MakeCreateFileListDependency(MakeContext, Target, &Substring)) {
                            return NULL;
                        }
                    } else {
                        if (!MakeCreateRuleDependency(MakeContext, Target, &Substring)) {
                            return NULL;
                        }
                    }
                }

                Substring.LengthInChars = 0;
                continue;
            }

            Substring.LengthInChars++;
        }
    }

    if (Substring.LengthInChars) {

        //
        //  If the string is quoted and has contents, strip off the
        //  quotes.
        //

        if (Substring.LengthInChars >= 3 &&
            Substring.StartOfString[0] == '"' &&
            Substring.StartOfString[Substring.LengthInChars - 1] == '"') {

            Substring.StartOfString++;
            Substring.LengthInChars = Substring.LengthInChars - 2;
        }

        if (Subdirectories) {
            if (!MakeCreateSubdirectoryDependency(MakeContext, Target, &Substring, &ParentTargetName)) {
                return NULL;
            }
        } else {
            if (Substring.StartOfString[0] == '@') {
                if (!MakeCreateFileListDependency(MakeContext, Target, &Substring)) {
                    return NULL;
                }
            } else {
                if (!MakeCreateRuleDependency(MakeContext, Target, &Substring)) {
                    return NULL;
                }
            }
        }
    }

    return Target;
}

/**
 Create a temporary file to contain inline file contents.

 @param MakeContext Pointer to the global make context.

 @param TrailingLine Pointer to the line after the inline file operator.
        NMAKE allows the file name to be specified here.

 @return Pointer to the inline file structure on success, NULL on failure.
 */
PMAKE_INLINE_FILE
MakeCreateInlineFile(
    __in PMAKE_CONTEXT MakeContext,
    __in PYORI_STRING TrailingLine
    )
{
    PMAKE_INLINE_FILE InlineFile;
    YORI_STRING TempPrefix;
    YORI_STRING TempPath;

    //
    //  MSFIX: NMAKE allows the user to specify a file name after the inline
    //  file operator, which is not implemented here yet
    //

    UNREFERENCED_PARAMETER(TrailingLine);

    InlineFile = YoriLibReferencedMalloc(sizeof(MAKE_INLINE_FILE));
    if (InlineFile == NULL) {
        return NULL;
    }

    //
    //  Take the globally collected temp path and truncate any trailing
    //  seperators from it
    //

    YoriLibInitEmptyString(&TempPath);
    TempPath.StartOfString = MakeContext->TempPath.StartOfString;
    TempPath.LengthInChars = MakeContext->TempPath.LengthInChars;

    while (TempPath.LengthInChars > 0 &&
           YoriLibIsSep(TempPath.StartOfString[TempPath.LengthInChars - 1])) {

        TempPath.LengthInChars--;
    }

    YoriLibConstantString(&TempPrefix, _T("YMK"));

    //
    //  Generate a temporary name and keep the handle open
    //

    if (!YoriLibGetTempFileName(&TempPath, &TempPrefix, &InlineFile->FileHandle, &InlineFile->FileName)) {
        YoriLibDereference(InlineFile);
        return NULL;
    }

    YoriLibAppendList(&MakeContext->InlineFileList, &InlineFile->InlineFileList);
    return InlineFile;
}

/**
 Delete all inline files on process exit.

 @param MakeContext Pointer to the global make context.
 */
VOID
MakeDeleteInlineFiles(
    __in PMAKE_CONTEXT MakeContext
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PMAKE_INLINE_FILE InlineFile;

    ListEntry = YoriLibGetNextListEntry(&MakeContext->InlineFileList, NULL);
    while (ListEntry != NULL) {
        InlineFile = CONTAINING_RECORD(ListEntry, MAKE_INLINE_FILE, InlineFileList);
        YoriLibRemoveListItem(&InlineFile->InlineFileList);
        if (InlineFile->FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(InlineFile->FileHandle);
            InlineFile->FileHandle = INVALID_HANDLE_VALUE;
        }
        DeleteFile(InlineFile->FileName.StartOfString);
        YoriLibFreeStringContents(&InlineFile->FileName);
        YoriLibDereference(InlineFile);
        ListEntry = YoriLibGetNextListEntry(&MakeContext->InlineFileList, NULL);
    }
}

/**
 Add a line to an opened inline file.  Note that depending on the line the
 inline file may terminate here and the parsing mode will switch back to
 recipe.

 @param ScopeContext Pointer to the scope context, which can be used to
        locate the current inline file.

 @param Line Pointer to the line to add.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeAddInlineFileLine(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PYORI_STRING Line
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PMAKE_INLINE_FILE InlineFile;
    YORI_STRING Newline;

    //
    //  MSFIX This takes the most recent inline file which seems correct but
    //  also doesn't match the more explicit tracking style used for scope
    //  contexts and targets
    //

    ListEntry = YoriLibGetPreviousListEntry(&ScopeContext->MakeContext->InlineFileList, NULL);
    ASSERT(ListEntry != NULL);

    InlineFile = CONTAINING_RECORD(ListEntry, MAKE_INLINE_FILE, InlineFileList);

    //
    //  If the line starts with <<, this inline file is over.  Close the
    //  handle so the file can be opened by the build task, and move the
    //  parser state back to recipe.
    //
    //  MSFIX NMAKE supports a KEEP and NOKEEP qualifier after this which
    //  indicates whether the file should be deleted or not on process exit
    //

    if (Line->LengthInChars >= 2 &&
        Line->StartOfString[0] == '<' &&
        Line->StartOfString[1] == '<') {

        ASSERT(InlineFile->FileHandle != NULL && InlineFile->FileHandle != INVALID_HANDLE_VALUE);
        CloseHandle(InlineFile->FileHandle);
        InlineFile->FileHandle = INVALID_HANDLE_VALUE;
        ScopeContext->ParserState = MakeParserRecipeActive;
        return TRUE;
    }

    YoriLibOutputTextToMultibyteDevice(InlineFile->FileHandle, Line);
    YoriLibConstantString(&Newline, _T("\r\n"));
    YoriLibOutputTextToMultibyteDevice(InlineFile->FileHandle, &Newline);
    return TRUE;
}

/**
 Add a command to a recipe.  Note the command may indicate that an inline
 file should be created, which will switch the parsing mode to inline file.

 @param ScopeContext Pointer to the scope context.

 @param Target Pointer to the target to add recipe lines to.

 @param Line Pointer to the line to add.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
MakeAddRecipeCommand(
    __in PMAKE_SCOPE_CONTEXT ScopeContext,
    __in PMAKE_TARGET Target,
    __in PYORI_STRING Line
    )
{

    PMAKE_INLINE_FILE InlineFile;
    DWORD CharsNeeded;
    DWORD Index;
    YORI_STRING LineSubset;

    //
    //  Start by assuming the entire line should be added to the recipe.
    //

    YoriLibInitEmptyString(&LineSubset);
    LineSubset.StartOfString = Line->StartOfString;
    LineSubset.LengthInChars = Line->LengthInChars;
    InlineFile = NULL;

    //
    //  Look through the line for an indication that an inline file should
    //  be created.  When this happens, the file name is substituted into
    //  the recipe line.
    //
    //  MSFIX Because the recipe is just a pile of text, the inline file
    //  needs to be created in the sense of having a file name assigned
    //  immediately.  This seems inefficient if a makefile contains inline
    //  files which are part of targets that are never executed.  Avoiding
    //  this requires a recipe to have a smarter data structure.
    //

    if (Line->LengthInChars >= 2) {
        for (Index = 0; Index < Line->LengthInChars - 1; Index++) {
            if (Line->StartOfString[Index] == '<' &&
                Line->StartOfString[Index + 1] == '<') {

                YORI_STRING TrailingText;
                YoriLibInitEmptyString(&TrailingText);
                TrailingText.StartOfString = &Line->StartOfString[Index + 2];
                TrailingText.LengthInChars = Line->LengthInChars - Index - 2;

                InlineFile = MakeCreateInlineFile(ScopeContext->MakeContext, &TrailingText);
                if (InlineFile == NULL) {
                    return FALSE;
                }

                LineSubset.LengthInChars = Index;
                ScopeContext->ParserState = MakeParserInlineFileActive;
            }
        }
    }

    CharsNeeded = Target->Recipe.LengthInChars + sizeof("\n") + LineSubset.LengthInChars + 1;
    if (InlineFile != NULL) {
        CharsNeeded = CharsNeeded + InlineFile->FileName.LengthInChars;
    }

    if (CharsNeeded > Target->Recipe.LengthAllocated) {
        if (!YoriLibReallocateString(&Target->Recipe, CharsNeeded * 2)) {
            return FALSE;
        }
    }

    memcpy(&Target->Recipe.StartOfString[Target->Recipe.LengthInChars], LineSubset.StartOfString, LineSubset.LengthInChars * sizeof(TCHAR));
    Target->Recipe.LengthInChars = Target->Recipe.LengthInChars + LineSubset.LengthInChars;

    if (InlineFile != NULL) {
        memcpy(&Target->Recipe.StartOfString[Target->Recipe.LengthInChars], InlineFile->FileName.StartOfString, InlineFile->FileName.LengthInChars * sizeof(TCHAR));
        Target->Recipe.LengthInChars = Target->Recipe.LengthInChars + InlineFile->FileName.LengthInChars;
    }

    Target->Recipe.StartOfString[Target->Recipe.LengthInChars] = '\n';
    Target->Recipe.StartOfString[Target->Recipe.LengthInChars + 1] = '\0';
    Target->Recipe.LengthInChars = Target->Recipe.LengthInChars + 1;

    return TRUE;
}

/**
 Process a single opened stream, enumerating through all lines and displaying
 the set requested by the user.

 @param hSource The opened source stream.

 @param MakeContext Pointer to context information specifying which lines to
        display.

 @param FileName Pointer to the file name string, used in error reporting.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MakeProcessStream(
    __in HANDLE hSource,
    __in PMAKE_CONTEXT MakeContext,
    __in PYORI_STRING FileName
    )
{
    PVOID LineContext = NULL;
    YORI_STRING JoinedLine;
    YORI_STRING LineString;
    YORI_STRING LineToProcess;
    YORI_STRING ExpandedLine;
    YORI_STRING VariableNotFound;
    MAKE_LINE_TYPE LineType;
    BOOLEAN MoreLinesNeeded;
    LPTSTR PrefixString;
    PMAKE_TARGET ActiveRecipeTarget = NULL;
    PMAKE_SCOPE_CONTEXT ScopeContext;
    DWORD LineNumber;

    ScopeContext = MakeContext->ActiveScope;

    YoriLibInitEmptyString(&LineString);
    YoriLibInitEmptyString(&JoinedLine);
    YoriLibInitEmptyString(&LineToProcess);
    YoriLibInitEmptyString(&ExpandedLine);
    LineNumber = 0;

#if MAKE_DEBUG_PREPROCESSOR
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Processing %y\n"), FileName);
#endif

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
            break;
        }
        LineNumber++;

        //
        //  Line might be:
        //   - Commented (truncate)
        //   - Joined line (ends with \)
        //   - Have variables (expand), including $@ etc
        //   - Preprocessor command (!)
        //   - Set variable (= found)
        //   - Target (: found)
        //   - Recipe (lines after target until first blank line)
        //   - Inline file (lines between << and << within a recipe)
        //

        LineToProcess.StartOfString = LineString.StartOfString;
        LineToProcess.LengthInChars = LineString.LengthInChars;
        MakeTruncateComments(&LineToProcess);

        MoreLinesNeeded = FALSE;

        if (LineToProcess.LengthInChars > 0 && LineToProcess.StartOfString[LineToProcess.LengthInChars - 1] == '\\') {
            MoreLinesNeeded = TRUE;
        }

        if (JoinedLine.LengthInChars > 0 || MoreLinesNeeded) {
            MakeTrimWhitespace(&LineToProcess);
            MakeJoinLines(&JoinedLine, &LineToProcess);
            if (MoreLinesNeeded) {
                continue;
            }
            LineToProcess.StartOfString = JoinedLine.StartOfString;
            LineToProcess.LengthInChars = JoinedLine.LengthInChars;
        }

        LineType = MakeDetermineLineType(&LineToProcess, ScopeContext);
        MakeTrimWhitespace(&LineToProcess);

        switch(LineType) {
            case MakeLineTypeEmpty:
                PrefixString = _T("Empty");
                break;
            case MakeLineTypePreprocessor:
                PrefixString = _T("Preprocessor");
                break;
            case MakeLineTypeSetVariable:
                PrefixString = _T("SetVariable");
                break;
            case MakeLineTypeRule:
                PrefixString = _T("Rule");
                break;
            case MakeLineTypeRecipe:
                PrefixString = _T("Recipe");
                break;
            case MakeLineTypeInlineFile:
                PrefixString = _T("InlineFile");
                break;
            case MakeLineTypeDebugBreak:
                PrefixString = _T("DebugBreak");
                DebugBreak();
                break;
            default:
                PrefixString = _T("**ERROR**");
                break;
        }

        //
        //  If based on the current state of conditional evaluation the line
        //  should not be processed, we only need to perform minimal
        //  processing to track the state of conditional evaluation.
        //

        if (!MakePreprocessorShouldExecuteLine(ScopeContext, &LineToProcess, LineType)) {

            if (LineType != MakeLineTypePreprocessor) {
                JoinedLine.LengthInChars = 0;
                continue;
            }

#if MAKE_DEBUG_PREPROCESSOR
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%04i MiniPreprocessor: %y\n"), LineNumber, &LineToProcess);
#endif
            MakePreprocessorPerformMinimalConditionalTracking(ScopeContext, &LineToProcess);
            JoinedLine.LengthInChars = 0;
            continue;
        }

        if (LineType == MakeLineTypeError) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y(%i) Parse error: %y\n"), FileName, LineNumber, &LineToProcess);
            MakeContext->ErrorTermination = TRUE;
        }

        //
        //  Now that the line has been determined to be included by the
        //  preprocessor and not in error, apply any state transformations.
        //

        if (LineType == MakeLineTypeEmpty) {
            if (ScopeContext->ParserState == MakeParserInlineFileActive) {
                LineType = MakeLineTypeInlineFile;
            } else if (ScopeContext->ParserState == MakeParserRecipeActive) {
                ScopeContext->ParserState = MakeParserDefault;
            }
        } else if (LineType == MakeLineTypeRule) {
            ASSERT(ScopeContext->ParserState == MakeParserDefault);
            ScopeContext->ParserState = MakeParserRecipeActive;
        }

        //
        //  Variables should not be expanded for inference rule recipes.
        //  NMAKE expands these only when they are instantiated into commands
        //  to execute.  Note the implication is they need to be expanded
        //  with scope-specific variables, either when the scope exits or we
        //  keep scope state alive until processing the dependency graph to
        //  execute
        //

        ASSERT(LineType != MakeLineTypeRecipe || ActiveRecipeTarget != NULL);
        if (LineType != MakeLineTypeRecipe ||
            ActiveRecipeTarget == NULL ||
            !ActiveRecipeTarget->InferenceRulePseudoTarget) {

            YoriLibInitEmptyString(&VariableNotFound);
            if (!MakeExpandVariables(ScopeContext, NULL, &ExpandedLine, &LineToProcess, &VariableNotFound)) {
                MakeContext->ErrorTermination = TRUE;
            } else if (VariableNotFound.LengthInChars > 0 && MakeContext->WarnOnUndefinedVariable) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y(%i) Undefined variable %y\n"), FileName, LineNumber, &VariableNotFound);
            }
        } else {
            MakeAddRecipeCommand(ScopeContext, ActiveRecipeTarget, &LineToProcess);
        }

        if (LineType == MakeLineTypeSetVariable) {
            MakeExecuteSetVariable(ScopeContext, &ExpandedLine);
        } else if (LineType == MakeLineTypePreprocessor) {
            MakePreprocessor(ScopeContext, &ExpandedLine);
        } else if (LineType == MakeLineTypeInlineFile) {
            MakeAddInlineFileLine(ScopeContext, &ExpandedLine);
        } else if (LineType == MakeLineTypeRule) {
            ActiveRecipeTarget = MakeAddRule(ScopeContext, &ExpandedLine);
            if (ActiveRecipeTarget == NULL) {
                ScopeContext->ParserState = MakeParserDefault;
            }
        } else if (LineType == MakeLineTypeRecipe &&
                   ActiveRecipeTarget != NULL &&
                   !ActiveRecipeTarget->InferenceRulePseudoTarget) {
            MakeAddRecipeCommand(ScopeContext, ActiveRecipeTarget, &ExpandedLine);
        }
#if MAKE_DEBUG_PREPROCESSOR
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%04i %12s: %y\n"), LineNumber, PrefixString, &ExpandedLine);
#endif
        if (MakeContext->ErrorTermination) {
            break;
        }
        JoinedLine.LengthInChars = 0;
    }

    YoriLibLineReadCloseOrCache(LineContext);
    YoriLibFreeStringContents(&LineString);
    YoriLibFreeStringContents(&JoinedLine);
    YoriLibFreeStringContents(&ExpandedLine);

    return TRUE;
}

// vim:sw=4:ts=4:et:
