/**
 * @file sh/alias.c
 *
 * Yori alias support
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

#include "yori.h"

#pragma warning(disable: 4220) // Varargs matches remaining parameters

/**
 A structure containing an individual Yori alias.
 */
typedef struct _YORI_ALIAS {

    /**
     Links between all registered aliases.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     Hash link for efficient lookup of aliases.
     */
    YORI_HASH_ENTRY HashEntry;

    /**
     The name of the alias.
     */
    YORI_STRING Alias;

    /**
     The values to substitute when an alias match is discovered.
     */
    YORI_STRING Value;

    /**
     TRUE if the alias is defined internally by Yori; FALSE if it is defined
     by the user.  Internal aliases are not enumerated by default.
     */
    BOOL Internal;
} YORI_ALIAS, *PYORI_ALIAS;

/**
 List of aliases currently registered with Yori.
 */
YORI_LIST_ENTRY YoriShAliasesList;

/**
 Hashtable of aliases currently registered with Yori.
 */
PYORI_HASH_TABLE YoriShAliasesHash;

/**
 The app name to use when asking conhost for alias information.  Note this
 really has nothing to do with the actual binary name.
 */
#define ALIAS_APP_NAME _T("YORI.EXE")

/**
 The app name to use when asking conhost for alias information that is
 being imported from CMD.
 */
#define ALIAS_IMPORT_APP_NAME _T("CMD.EXE")

/**
 Delete an existing shell alias.

 @param Alias The alias to delete.

 @return TRUE if the alias was successfully deleted, FALSE if it was not
         found.
 */
__success(return)
BOOL
YoriShDeleteAlias(
    __in PYORI_STRING Alias
    )
{
    PYORI_HASH_ENTRY HashEntry;
    PYORI_ALIAS ExistingAlias;

    if (YoriShAliasesHash == NULL) {
        return FALSE;
    }

    HashEntry = YoriLibHashRemoveByKey(YoriShAliasesHash, Alias);
    if (HashEntry == NULL) {
        return FALSE;
    }

    ExistingAlias = HashEntry->Context;

    if (!ExistingAlias->Internal && DllKernel32.pAddConsoleAliasW) {
        DllKernel32.pAddConsoleAliasW(ExistingAlias->Alias.StartOfString, NULL, ALIAS_APP_NAME);
    }
    YoriLibRemoveListItem(&ExistingAlias->ListEntry);
    YoriLibFreeStringContents(&ExistingAlias->Alias);
    YoriLibFreeStringContents(&ExistingAlias->Value);
    YoriLibDereference(ExistingAlias);
    return TRUE;
}


/**
 Add a new, or replace an existing, shell alias.

 @param Alias The alias to add or update.

 @param Value The value to assign to the alias.

 @param Internal TRUE if the alias is defined internally by Yori and should
        not be enumerated by default; FALSE if it is defined by the user.

 @return TRUE if the alias was successfully updated, FALSE if it was not.
 */
__success(return)
BOOL
YoriShAddAlias(
    __in PYORI_STRING Alias,
    __in PYORI_STRING Value,
    __in BOOL Internal
    )
{
    PYORI_ALIAS NewAlias;
    DWORD AliasNameLengthInChars;
    DWORD ValueNameLengthInChars;

    if (YoriShAliasesHash != NULL) {
        if (Internal) {
            PYORI_HASH_ENTRY HashEntry;
            PYORI_ALIAS ExistingAlias;
            HashEntry = YoriLibHashLookupByKey(YoriShAliasesHash, Alias);
            if (HashEntry != NULL) {
                ExistingAlias = HashEntry->Context;
                if (!ExistingAlias->Internal) {
                    return FALSE;
                }
            }
        }
        YoriShDeleteAlias(Alias);
    } else {
        YoriLibInitializeListHead(&YoriShAliasesList);
        YoriShAliasesHash = YoriLibAllocateHashTable(250);
        if (YoriShAliasesHash == NULL) {
            return FALSE;
        }
    }

    AliasNameLengthInChars = Alias->LengthInChars;
    ValueNameLengthInChars = Value->LengthInChars;

    NewAlias = YoriLibReferencedMalloc(sizeof(YORI_ALIAS) + (AliasNameLengthInChars + ValueNameLengthInChars + 2) * sizeof(TCHAR));
    if (NewAlias == NULL) {
        return FALSE;
    }

    YoriLibReference(NewAlias);
    NewAlias->Alias.MemoryToFree = NewAlias;
    NewAlias->Alias.StartOfString = (LPTSTR)(NewAlias + 1);
    NewAlias->Alias.LengthInChars = AliasNameLengthInChars;
    NewAlias->Alias.LengthAllocated = AliasNameLengthInChars + 1;

    YoriLibReference(NewAlias);
    NewAlias->Value.MemoryToFree = NewAlias;
    NewAlias->Value.StartOfString = NewAlias->Alias.StartOfString + AliasNameLengthInChars + 1;
    NewAlias->Value.LengthInChars = ValueNameLengthInChars;
    NewAlias->Value.LengthAllocated = ValueNameLengthInChars + 1;
    NewAlias->Internal = Internal;

    memcpy(NewAlias->Alias.StartOfString, Alias->StartOfString, AliasNameLengthInChars * sizeof(TCHAR));
    memcpy(NewAlias->Value.StartOfString, Value->StartOfString, ValueNameLengthInChars * sizeof(TCHAR));
    NewAlias->Alias.StartOfString[AliasNameLengthInChars] = '\0';
    NewAlias->Value.StartOfString[ValueNameLengthInChars] = '\0';

    if (!Internal && DllKernel32.pAddConsoleAliasW) {
        DllKernel32.pAddConsoleAliasW(NewAlias->Alias.StartOfString, NewAlias->Value.StartOfString, ALIAS_APP_NAME);
    }

    YoriLibAppendList(&YoriShAliasesList, &NewAlias->ListEntry);
    YoriLibHashInsertByKey(YoriShAliasesHash, &NewAlias->Alias, NewAlias, &NewAlias->HashEntry);

    return TRUE;
}

/**
 Expand variables in an alias.

 @param OutputString The buffer to output the result of variable expansion to.

 @param VariableName The name of the variable that requires expansion.

 @param CmdContext Pointer to the original CmdContext without the alias
        present.

 @return The number of characters populated or the number of characters
         required if the buffer is too small.
 */
DWORD
YoriShExpandAliasHelper(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext
    )
{
    DWORD CmdIndex;

    if (VariableName->LengthInChars == 1 && VariableName->StartOfString[0] == '*') {
        YORI_STRING CmdLine;
        DWORD CmdLineLength;
        YORI_LIBSH_CMD_CONTEXT ArgContext;

        ZeroMemory(&ArgContext, sizeof(ArgContext));

        memcpy(&ArgContext, CmdContext, sizeof(YORI_LIBSH_CMD_CONTEXT));

        ArgContext.ArgC = ArgContext.ArgC - 1;
        ArgContext.ArgV = &ArgContext.ArgV[1];
        ArgContext.ArgContexts = &ArgContext.ArgContexts[1];

        YoriLibInitEmptyString(&CmdLine);
        if(YoriLibShBuildCmdlineFromCmdContext(&ArgContext, &CmdLine, FALSE, NULL, NULL)) {
            if (CmdLine.LengthInChars < OutputString->LengthAllocated) {
                YoriLibYPrintf(OutputString, _T("%y"), &CmdLine);
            }
            CmdLineLength = CmdLine.LengthInChars;
            YoriLibFreeStringContents(&CmdLine);
            return CmdLineLength;
        }
    } else {
        CmdIndex = YoriLibDecimalStringToInt(VariableName);
        if (CmdIndex > 0 && CmdContext->ArgC > CmdIndex) {
            if (CmdContext->ArgV[CmdIndex].LengthInChars < OutputString->LengthAllocated) {
                YoriLibYPrintf(OutputString, _T("%y"), &CmdContext->ArgV[CmdIndex]);
            }
            return CmdContext->ArgV[CmdIndex].LengthInChars;
        }
    }
    return 0;
}


/**
 Check if the command in the specified command context is an alias, and if
 so, update the command context to contain the new command and arguments,
 as specified by the alias.

 @param CmdContext Pointer to the command context to check for aliases and
        update on alias match.

 @return TRUE if an alias match was found, FALSE if not.
 */
__success(return)
BOOL
YoriShExpandAlias(
    __inout PYORI_LIBSH_CMD_CONTEXT CmdContext
    )
{
    YORI_LIBSH_CMD_CONTEXT NewCmdContext;
    YORI_STRING NewCmdString;

    PYORI_HASH_ENTRY HashEntry;
    PYORI_ALIAS ExistingAlias;

    if (YoriShAliasesHash == NULL) {
        return FALSE;
    }

    HashEntry = YoriLibHashLookupByKey(YoriShAliasesHash, &CmdContext->ArgV[0]);
    if (HashEntry == NULL) {
        return FALSE;
    }

    ExistingAlias = HashEntry->Context;

    YoriLibInitEmptyString(&NewCmdString);
    YoriLibExpandCommandVariables(&ExistingAlias->Value, '$', TRUE, YoriShExpandAliasHelper, CmdContext, &NewCmdString);
    if (NewCmdString.LengthInChars > 0) {
        if (YoriLibShParseCmdlineToCmdContext(&NewCmdString, 0, &NewCmdContext) &&
            NewCmdContext.ArgC > 0) {

            YoriShExpandEnvironmentInCmdContext(&NewCmdContext);
            YoriLibShFreeCmdContext(CmdContext);
            memcpy(CmdContext, &NewCmdContext, sizeof(YORI_LIBSH_CMD_CONTEXT));
            YoriLibFreeStringContents(&NewCmdString);
            return TRUE;
        }
    }
    YoriLibFreeStringContents(&NewCmdString);
    return FALSE;
}

/**
 Expand aliases in an arbitrary (unparsed) string and return the result as a
 string.

 @param CommandString The string to expand any aliases in.

 @param ExpandedString If aliases were expanded, updated to point to a newly
        allocated string containing the expanded result.

 @return TRUE to indicate aliases were successfully expanded, FALSE to
         indicate no aliases required expansion.
 */
__success(return)
BOOL
YoriShExpandAliasFromString(
    __in PYORI_STRING CommandString,
    __out PYORI_STRING ExpandedString
    )
{
    YORI_LIBSH_CMD_CONTEXT CmdContext;

    if (!YoriLibShParseCmdlineToCmdContext(CommandString, 0, &CmdContext)) {
        return FALSE;
    }

    if (!YoriShExpandEnvironmentInCmdContext(&CmdContext)) {
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (!YoriShExpandAlias(&CmdContext)) {
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriLibInitEmptyString(ExpandedString);
    if (!YoriLibShBuildCmdlineFromCmdContext(&CmdContext, ExpandedString, FALSE, NULL, NULL)) {
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }
    YoriLibShFreeCmdContext(&CmdContext);

    return TRUE;
}


/**
 Free all aliases.
 */
VOID
YoriShClearAllAliases(VOID)
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORI_ALIAS Alias;

    if (YoriShAliasesList.Next == NULL) {
        return;
    }

    ListEntry = YoriLibGetNextListEntry(&YoriShAliasesList, NULL);
    while (ListEntry != NULL) {
        Alias = CONTAINING_RECORD(ListEntry, YORI_ALIAS, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&YoriShAliasesList, ListEntry);
        YoriLibHashRemoveByEntry(&Alias->HashEntry);
        YoriLibRemoveListItem(&Alias->ListEntry);
        YoriLibFreeStringContents(&Alias->Alias);
        YoriLibFreeStringContents(&Alias->Value);
        YoriLibDereference(Alias);
    }

    YoriLibFreeEmptyHashTable(YoriShAliasesHash);
    YoriShAliasesHash = NULL;
}

/**
 Build the complete set of aliases into a an array of key value pairs
 and return a pointer to the result.  This must be freed with a
 subsequent call to @ref YoriLibFreeStringContents .

 @param IncludeFlags Specifies whether the result should include user defined
        aliases, system defined aliases, or both.  Valid flags are
        YORI_SH_GET_ALIAS_STRINGS_INCLUDE_USER and
        YORI_SH_GET_ALIAS_STRINGS_INCLUDE_INTERNAL, and these can be combined
        to return both.

 @param AliasStrings On successful completion, populated with the set of
        alias strings.

 @return Return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShGetAliasStrings(
    __in DWORD IncludeFlags,
    __inout PYORI_STRING AliasStrings
    )
{
    DWORD CharsNeeded = 0;
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORI_ALIAS Alias;
    DWORD StringOffset;
    BOOLEAN IncludeThisEntry;

    if (YoriShAliasesList.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShAliasesList, NULL);
        while (ListEntry != NULL) {
            Alias = CONTAINING_RECORD(ListEntry, YORI_ALIAS, ListEntry);
            IncludeThisEntry = FALSE;
            if (Alias->Internal) {
                if (IncludeFlags & YORI_SH_GET_ALIAS_STRINGS_INCLUDE_INTERNAL) {
                    IncludeThisEntry = TRUE;
                }
            } else {
                if (IncludeFlags & YORI_SH_GET_ALIAS_STRINGS_INCLUDE_USER) {
                    IncludeThisEntry = TRUE;
                }
            }
            if (IncludeThisEntry) {
                CharsNeeded += Alias->Alias.LengthInChars + Alias->Value.LengthInChars + 2;
            }
            ListEntry = YoriLibGetNextListEntry(&YoriShAliasesList, ListEntry);
        }
    }

    CharsNeeded += 1;

    if (AliasStrings->LengthAllocated < CharsNeeded) {
        YoriLibFreeStringContents(AliasStrings);
        if (!YoriLibAllocateString(AliasStrings, CharsNeeded)) {
            return FALSE;
        }
    }

    StringOffset = 0;

    if (YoriShAliasesList.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShAliasesList, NULL);
        while (ListEntry != NULL) {
            Alias = CONTAINING_RECORD(ListEntry, YORI_ALIAS, ListEntry);
            IncludeThisEntry = FALSE;
            if (Alias->Internal) {
                if (IncludeFlags & YORI_SH_GET_ALIAS_STRINGS_INCLUDE_INTERNAL) {
                    IncludeThisEntry = TRUE;
                }
            } else {
                if (IncludeFlags & YORI_SH_GET_ALIAS_STRINGS_INCLUDE_USER) {
                    IncludeThisEntry = TRUE;
                }
            }
            if (IncludeThisEntry) {
                YoriLibSPrintf(&AliasStrings->StartOfString[StringOffset], _T("%y=%y"), &Alias->Alias, &Alias->Value);
                StringOffset += Alias->Alias.LengthInChars + Alias->Value.LengthInChars + 2;
            }
            ListEntry = YoriLibGetNextListEntry(&YoriShAliasesList, ListEntry);
        }
    }
    AliasStrings->StartOfString[StringOffset] = '\0';

    return TRUE;
}

/**
 Add a new, or replace an existing, shell alias using NULL terminated strings.

 @param Alias The alias to add or update.

 @param Value The value to assign to the alias.

 @param Internal TRUE if the alias is defined internally by Yori and should
        not be enumerated by default; FALSE if it is defined by the user.

 @return TRUE if the alias was successfully updated, FALSE if it was not.
 */
__success(return)
BOOL
YoriShAddAliasLiteral(
    __in LPTSTR Alias,
    __in LPTSTR Value,
    __in BOOL Internal
    )
{
    YORI_STRING YsAlias;
    YORI_STRING YsValue;

    YoriLibConstantString(&YsAlias, Alias);
    YoriLibConstantString(&YsValue, Value);

    return YoriShAddAlias(&YsAlias, &YsValue, Internal);
}

/**
 Convert a CMD alias into a Yori alias.  This means converting $1 et al to
 $1$, and $* to $*$.  $T is not currently supported, and this routine will
 fail for those to prevent importing them.

 @param CmdAliasValue The value of the alias to import, in CMD format.

 @param YoriAliasValue On successful completion, updated to point to the alias
        in Yori format.  The caller is expected to free this with
        @ref YoriLibDereference.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShImportAliasValue(
    __in LPTSTR CmdAliasValue,
    __out LPTSTR * YoriAliasValue
    )
{
    ULONG Index;
    ULONG CharsNeeded;
    YORI_STRING NewString;
    YORI_STRING ExpandedString;

    CharsNeeded = 0;
    for (Index = 0; CmdAliasValue[Index] != '\0'; Index++) {
        if (CmdAliasValue[Index] == '$') {
            if (CmdAliasValue[Index + 1] >= '0' && CmdAliasValue[Index + 1] <= '9') {
                CharsNeeded++;
            } else if (CmdAliasValue[Index + 1] == '*') {
                CharsNeeded++;
            } else if (YoriLibUpcaseChar(CmdAliasValue[Index + 1]) == 'T') {
                return FALSE;
            }
        }
        CharsNeeded++;
    }

    CharsNeeded++;
    if (!YoriLibAllocateString(&NewString, CharsNeeded)) {
        return FALSE;
    }

    CharsNeeded = 0;
    for (Index = 0; CmdAliasValue[Index] != '\0'; Index++) {
        NewString.StartOfString[CharsNeeded] = CmdAliasValue[Index];
        if (CmdAliasValue[Index] == '$' && CmdAliasValue[Index + 1] != '\0') {
            CharsNeeded++;
            Index++;
            NewString.StartOfString[CharsNeeded] = CmdAliasValue[Index];
            if (CmdAliasValue[Index] >= '0' && CmdAliasValue[Index] <= '9') {
                CharsNeeded++;
                NewString.StartOfString[CharsNeeded] = '$';
            } else if (CmdAliasValue[Index] == '*') {
                CharsNeeded++;
                NewString.StartOfString[CharsNeeded] = '$';
            }
        }
        CharsNeeded++;
    }

    NewString.StartOfString[CharsNeeded] ='\0';
    NewString.LengthInChars = CharsNeeded;

    if (YoriShExpandAliasFromString(&NewString, &ExpandedString)) {
        YoriLibFreeStringContents(&NewString);
        *YoriAliasValue = ExpandedString.StartOfString;
    } else {
        *YoriAliasValue = NewString.StartOfString;
    }
    return TRUE;
}

/**
 Return a NULL terminated list of alias strings from the console host.

 @param LoadFromCmd If TRUE, the alias strings for CMD are loaded.  If FALSE,
        the alias strings for Yori are loaded.

 @param AliasBuffer On successful completion, updated to point to a newly
        allocated set of alias strings.  This should be freed with @ref
        YoriLibFreeStringContents .

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShGetSystemAliasStrings(
    __in BOOL LoadFromCmd,
    __out PYORI_STRING AliasBuffer
    )
{
    LPTSTR AppName;
    DWORD LengthRequired;

    YoriLibInitEmptyString(AliasBuffer);

    if (DllKernel32.pAddConsoleAliasW == NULL ||
        DllKernel32.pGetConsoleAliasesW == NULL ||
        DllKernel32.pGetConsoleAliasesLengthW == NULL) {

        return FALSE;
    }

    if (LoadFromCmd) {
        AppName = ALIAS_IMPORT_APP_NAME;
    } else {
        AppName = ALIAS_APP_NAME;
    }

    LengthRequired = DllKernel32.pGetConsoleAliasesLengthW(AppName);
    if (LengthRequired == 0) {
        return TRUE;
    }

    if (!YoriLibAllocateString(AliasBuffer, (LengthRequired + 1) / sizeof(TCHAR))) {
        return FALSE;
    }

    AliasBuffer->LengthInChars = DllKernel32.pGetConsoleAliasesW(AliasBuffer->StartOfString, AliasBuffer->LengthAllocated * sizeof(TCHAR), AppName);
    AliasBuffer->LengthInChars = AliasBuffer->LengthInChars / sizeof(TCHAR);
    if (AliasBuffer->LengthInChars == 0 || AliasBuffer->LengthInChars > AliasBuffer->LengthAllocated) {
        YoriLibFreeStringContents(AliasBuffer);
        return FALSE;
    }

    return TRUE;
}

/**
 Find an alias with a specified name within a list of NULL terminated strings.

 @param AliasStrings The list of strings to search through.

 @param AliasName The name of the alias to find.

 @param AliasValue On successful completion, this is updated to point to the
        value of the found match.  Note this value is not referenced.

 @return TRUE to indicate a match was found, FALSE to indicate it was not.
 */
__success(return)
BOOL
YoriShFindAliasWithinStrings(
    __in PYORI_STRING AliasStrings,
    __in PYORI_STRING AliasName,
    __out PYORI_STRING AliasValue
    )
{
    DWORD CharsConsumed;
    YORI_STRING FoundAliasName;
    YORI_STRING FoundAliasValue;

    YoriLibInitEmptyString(&FoundAliasName);
    YoriLibInitEmptyString(&FoundAliasValue);

    CharsConsumed = 0;
    while (CharsConsumed < AliasStrings->LengthInChars && CharsConsumed < AliasStrings->LengthAllocated) {
        FoundAliasName.StartOfString = &AliasStrings->StartOfString[CharsConsumed];
        FoundAliasName.LengthInChars = 0;
        while (FoundAliasName.StartOfString[FoundAliasName.LengthInChars] != '\0' &&
               FoundAliasName.StartOfString[FoundAliasName.LengthInChars] != '=') {

            FoundAliasName.LengthInChars++;
        }

        CharsConsumed += FoundAliasName.LengthInChars + 1;

        if (FoundAliasName.StartOfString[FoundAliasName.LengthInChars] == '=') {
            FoundAliasValue.LengthInChars = 0;
            FoundAliasValue.StartOfString = &FoundAliasName.StartOfString[FoundAliasName.LengthInChars + 1];
            while (FoundAliasValue.StartOfString[FoundAliasValue.LengthInChars] != '\0') {
                FoundAliasValue.LengthInChars++;
            }

            CharsConsumed += FoundAliasValue.LengthInChars + 1;
        }

        if (YoriLibCompareStringInsensitive(AliasName, &FoundAliasName) == 0) {
            memcpy(AliasValue, &FoundAliasValue, sizeof(YORI_STRING));
            return TRUE;
        }
    }

    return FALSE;
}

/**
 Incorporate changes into the current set of aliases.  This function scans
 two NULL terminated lists of aliases to find changes in the new set over
 the old set and incorporate those into the current environment.

 @param MergeFromCmd If TRUE, these alias lists are treated as CMD format
        and need to be migrated in order to incorporate them into Yori.

 @param OldStrings Pointer to the original NULL terminated list of aliases.

 @param NewStrings Pointer to the new NULL terminated list of aliases.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShMergeChangedAliasStrings(
    __in BOOL MergeFromCmd,
    __in PYORI_STRING OldStrings,
    __in PYORI_STRING NewStrings
    )
{
    DWORD CharsConsumed;
    YORI_STRING FoundAliasName;
    YORI_STRING FoundAliasValue;
    YORI_STRING OldAliasValue;

    YoriLibInitEmptyString(&FoundAliasName);
    YoriLibInitEmptyString(&FoundAliasValue);

    //
    //  Navigate through the new alias strings.
    //

    CharsConsumed = 0;
    while (CharsConsumed < NewStrings->LengthInChars && CharsConsumed < NewStrings->LengthAllocated) {
        FoundAliasName.StartOfString = &NewStrings->StartOfString[CharsConsumed];
        FoundAliasName.LengthInChars = 0;
        while (FoundAliasName.StartOfString[FoundAliasName.LengthInChars] != '\0' &&
               FoundAliasName.StartOfString[FoundAliasName.LengthInChars] != '=') {

            FoundAliasName.LengthInChars++;
        }

        CharsConsumed += FoundAliasName.LengthInChars + 1;

        if (FoundAliasName.StartOfString[FoundAliasName.LengthInChars] == '=') {
            FoundAliasValue.LengthInChars = 0;
            FoundAliasValue.StartOfString = &FoundAliasName.StartOfString[FoundAliasName.LengthInChars + 1];
            while (FoundAliasValue.StartOfString[FoundAliasValue.LengthInChars] != '\0') {
                FoundAliasValue.LengthInChars++;
            }

            CharsConsumed += FoundAliasValue.LengthInChars + 1;

            //
            //  Now we've found something, check its state in the old
            //  alias strings.  If it's not there or changed, add it now.
            //

            if (!YoriShFindAliasWithinStrings(OldStrings, &FoundAliasName, &OldAliasValue) ||
                YoriLibCompareString(&FoundAliasValue, &OldAliasValue) != 0) {
                if (MergeFromCmd) {
                    LPTSTR MigratedAlias;
                    YORI_STRING YsMigratedAlias;
                    if (YoriShImportAliasValue(FoundAliasValue.StartOfString, &MigratedAlias)) {
                        YoriLibConstantString(&YsMigratedAlias, MigratedAlias);
                        YoriShAddAlias(&FoundAliasName, &YsMigratedAlias, FALSE);
                        YoriLibDereference(MigratedAlias);
                    }
                } else {
                    YoriShAddAlias(&FoundAliasName, &FoundAliasValue, FALSE);
                }
            }
        }
    }

    //
    //  Navigate through the old alias strings.
    //

    CharsConsumed = 0;
    while (CharsConsumed < OldStrings->LengthInChars && CharsConsumed < OldStrings->LengthAllocated) {
        FoundAliasName.StartOfString = &OldStrings->StartOfString[CharsConsumed];
        FoundAliasName.LengthInChars = 0;
        while (FoundAliasName.StartOfString[FoundAliasName.LengthInChars] != '\0' &&
               FoundAliasName.StartOfString[FoundAliasName.LengthInChars] != '=') {

            FoundAliasName.LengthInChars++;
        }

        CharsConsumed += FoundAliasName.LengthInChars + 1;

        if (FoundAliasName.StartOfString[FoundAliasName.LengthInChars] == '=') {
            FoundAliasValue.LengthInChars = 0;
            FoundAliasValue.StartOfString = &FoundAliasName.StartOfString[FoundAliasName.LengthInChars + 1];
            while (FoundAliasValue.StartOfString[FoundAliasValue.LengthInChars] != '\0') {
                FoundAliasValue.LengthInChars++;
            }

            CharsConsumed += FoundAliasValue.LengthInChars + 1;

            //
            //  Now we've found something, check its state in the new
            //  alias strings.  If it's not there, delete it.
            //

            if (!YoriShFindAliasWithinStrings(NewStrings, &FoundAliasName, &OldAliasValue)) {
                YoriShDeleteAlias(&FoundAliasName);
            }
        }
    }

    return FALSE;
}

/**
 Load aliases from the console and incorporate those into the shell's internal
 alias system.  This allows aliases to be inherited across subshells.

 @param ImportFromCmd If TRUE, aliases are loaded for the CMD.EXE process and
        migrated to match Yori syntax.  If FALSE, aliases are loaded for the
        YORI.EXE process.

 @return TRUE to indicate success or FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShLoadSystemAliases(
    __in BOOL ImportFromCmd
    )
{
    YORI_STRING AliasBuffer;
    LPTSTR ThisVar;
    LPTSTR Value;
    DWORD VarLen;
    DWORD CharsConsumed;

    if (!YoriShGetSystemAliasStrings(ImportFromCmd, &AliasBuffer)) {
        return FALSE;
    }

    ThisVar = AliasBuffer.StartOfString;
    CharsConsumed = 0;
    while (CharsConsumed < AliasBuffer.LengthInChars && CharsConsumed < AliasBuffer.LengthAllocated) {
        VarLen = _tcslen(ThisVar);
        CharsConsumed += VarLen;
        Value = _tcschr(ThisVar, '=');
        if (Value) {
            *Value = '\0';
            Value++;
            if (ImportFromCmd) {
                LPTSTR MigratedValue = NULL;

                if (YoriShImportAliasValue(Value, &MigratedValue)) {
                    YoriShAddAliasLiteral(ThisVar, MigratedValue, FALSE);
                    YoriLibDereference(MigratedValue);
                }

            } else {
                YoriShAddAliasLiteral(ThisVar, Value, FALSE);
            }
        }
        ThisVar += VarLen;
        ThisVar++;
        CharsConsumed++;
    }

    YoriLibFreeStringContents(&AliasBuffer);
    return TRUE;
}


// vim:sw=4:ts=4:et:
