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
     Links between the registered aliases.
     */
    YORI_LIST_ENTRY ListEntry;

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
YORI_LIST_ENTRY YoriShAliases;

/**
 Prototype for a function to the length of get aliases in the console for a
 particular application.
 */
typedef DWORD GET_CONSOLE_ALIASES_LENGTHW(LPTSTR);

/**
 Prototype for a pointer to a function to the length of get aliases in the
 console for a particular application.
 */
typedef GET_CONSOLE_ALIASES_LENGTHW *PGET_CONSOLE_ALIASES_LENGTHW;

/**
 Prototype for a function to get aliases from the console for a particular
 application.
 */
typedef DWORD GET_CONSOLE_ALIASESW(LPTSTR, DWORD, LPTSTR);

/**
 Prototype for a pointer to a function to get aliases from the console
 for a particular application.
 */
typedef GET_CONSOLE_ALIASESW *PGET_CONSOLE_ALIASESW;

/**
 Prototype for a function to add an alias with the console.
 */
typedef BOOL ADD_CONSOLE_ALIASW(LPCTSTR, LPCTSTR, LPCTSTR);

/**
 Prototype for a pointer to a function to add an alias with the console.
 */
typedef ADD_CONSOLE_ALIASW *PADD_CONSOLE_ALIASW;

/**
 Pointer to a function to obtain the length of alisaes in the console for
 a particular application.
 */
PGET_CONSOLE_ALIASES_LENGTHW pGetConsoleAliasesLengthW;

/**
 Pointer to a function to get aliases from from the console for a particular
 application.
 */
PGET_CONSOLE_ALIASESW pGetConsoleAliasesW;

/**
 Pointer to a function to add an alias with the console.
 */
PADD_CONSOLE_ALIASW pAddConsoleAliasW;

/**
 The app name to use when asking conhost for alias information.  Note this
 really has nothing to do with the actual binary name.
 */
#define ALIAS_APP_NAME _T("YORI.EXE")

/**
 Delete an existing shell alias.

 @param Alias The alias to delete.

 @return TRUE if the alias was successfully deleted, FALSE if it was not
         found.
 */
BOOL
YoriShDeleteAlias(
    __in PYORI_STRING Alias
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    if (YoriShAliases.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShAliases, ListEntry);
        while (ListEntry != NULL) {
            PYORI_ALIAS ExistingAlias = CONTAINING_RECORD(ListEntry, YORI_ALIAS, ListEntry);
            if (YoriLibCompareStringInsensitive(&ExistingAlias->Alias, Alias) == 0) {
                if (!ExistingAlias->Internal && pAddConsoleAliasW) {
                    pAddConsoleAliasW(ExistingAlias->Alias.StartOfString, NULL, ALIAS_APP_NAME);
                }
                YoriLibRemoveListItem(&ExistingAlias->ListEntry);
                YoriLibDereference(ExistingAlias);
                return TRUE;
            }
            ListEntry = YoriLibGetNextListEntry(&YoriShAliases, ListEntry);
        }
    }
    return FALSE;
}


/**
 Add a new, or replace an existing, shell alias.

 @param Alias The alias to add or update.

 @param Value The value to assign to the alias.

 @param Internal TRUE if the alias is defined internally by Yori and should
        not be enumerated by default; FALSE if it is defined by the user.

 @return TRUE if the alias was successfully updated, FALSE if it was not.
 */
BOOL
YoriShAddAlias(
    __in PYORI_STRING Alias,
    __in PYORI_STRING Value,
    __in BOOL Internal
    )
{
    PYORI_ALIAS NewAlias;
    PYORI_LIST_ENTRY ListEntry = NULL;
    DWORD AliasNameLengthInChars;
    DWORD ValueNameLengthInChars;

    if (YoriShAliases.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShAliases, ListEntry);
        while (ListEntry != NULL) {
            PYORI_ALIAS ExistingAlias = CONTAINING_RECORD(ListEntry, YORI_ALIAS, ListEntry);
            if (YoriLibCompareStringInsensitive(&ExistingAlias->Alias, Alias) == 0) {
                YoriShDeleteAlias(Alias);
                break;
            }
            ListEntry = YoriLibGetNextListEntry(&YoriShAliases, ListEntry);
        }
    } else {
        YoriLibInitializeListHead(&YoriShAliases);
    }

    AliasNameLengthInChars = Alias->LengthInChars;
    ValueNameLengthInChars = Value->LengthInChars;

    NewAlias = YoriLibReferencedMalloc(sizeof(YORI_ALIAS) + (AliasNameLengthInChars + ValueNameLengthInChars + 2) * sizeof(TCHAR));
    if (NewAlias == NULL) {
        return FALSE;
    }

    NewAlias->Alias.MemoryToFree = NULL;
    NewAlias->Alias.StartOfString = (LPTSTR)(NewAlias + 1);
    NewAlias->Alias.LengthInChars = AliasNameLengthInChars;
    NewAlias->Alias.LengthAllocated = AliasNameLengthInChars + 1;
    NewAlias->Value.MemoryToFree = NULL;
    NewAlias->Value.StartOfString = NewAlias->Alias.StartOfString + AliasNameLengthInChars + 1;
    NewAlias->Value.LengthInChars = ValueNameLengthInChars;
    NewAlias->Value.LengthAllocated = ValueNameLengthInChars + 1;
    NewAlias->Internal = Internal;

    memcpy(NewAlias->Alias.StartOfString, Alias->StartOfString, AliasNameLengthInChars * sizeof(TCHAR));
    memcpy(NewAlias->Value.StartOfString, Value->StartOfString, ValueNameLengthInChars * sizeof(TCHAR));
    NewAlias->Alias.StartOfString[AliasNameLengthInChars] = '\0';
    NewAlias->Value.StartOfString[ValueNameLengthInChars] = '\0';

    if (!Internal && pAddConsoleAliasW) {
        pAddConsoleAliasW(NewAlias->Alias.StartOfString, NewAlias->Value.StartOfString, ALIAS_APP_NAME);
    }

    YoriLibAppendList(&YoriShAliases, &NewAlias->ListEntry);
    
    return FALSE;
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
    __in PYORI_CMD_CONTEXT CmdContext
    )
{
    DWORD CmdIndex;
    DWORD ArgLength;

    if (VariableName->LengthInChars == 1 && VariableName->StartOfString[0] == '*') {
        LPTSTR CmdLine;
        YORI_CMD_CONTEXT ArgContext;

        ZeroMemory(&ArgContext, sizeof(ArgContext));

        memcpy(&ArgContext, CmdContext, sizeof(YORI_CMD_CONTEXT));

        ArgContext.ArgC = ArgContext.ArgC - 1;
        ArgContext.ArgV = &ArgContext.ArgV[1];
        ArgContext.ArgContexts = &ArgContext.ArgContexts[1];

        CmdLine = YoriShBuildCmdlineFromCmdContext(&ArgContext, TRUE, NULL, NULL);
        if (CmdLine != NULL) {
            ArgLength = _tcslen(CmdLine);
            if (ArgLength < OutputString->LengthAllocated) {
                YoriLibYPrintf(OutputString, _T("%s"), CmdLine);
            }
            YoriLibDereference(CmdLine);
            return ArgLength;
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
BOOL
YoriShExpandAlias(
    __inout PYORI_CMD_CONTEXT CmdContext
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    YORI_CMD_CONTEXT NewCmdContext;

    if (YoriShAliases.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShAliases, ListEntry);
        while (ListEntry != NULL) {
            PYORI_ALIAS ExistingAlias = CONTAINING_RECORD(ListEntry, YORI_ALIAS, ListEntry);
            YORI_STRING NewCmdString;
            YoriLibInitEmptyString(&NewCmdString);
            if (YoriLibCompareStringInsensitive(&ExistingAlias->Alias, &CmdContext->ArgV[0]) == 0) {
                YoriLibExpandCommandVariables(&ExistingAlias->Value, '$', FALSE, YoriShExpandAliasHelper, CmdContext, &NewCmdString);
                if (NewCmdString.LengthInChars > 0) {
                    if (YoriShParseCmdlineToCmdContext(&NewCmdString, 0, &NewCmdContext) && NewCmdContext.ArgC > 0) {
                        YoriShFreeCmdContext(CmdContext);
                        memcpy(CmdContext, &NewCmdContext, sizeof(YORI_CMD_CONTEXT));
                        YoriLibFreeStringContents(&NewCmdString);
                        return TRUE;
                    }
                }
                YoriLibFreeStringContents(&NewCmdString);
            }
            ListEntry = YoriLibGetNextListEntry(&YoriShAliases, ListEntry);
        }
    }
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
BOOL
YoriShExpandAliasFromString(
    __in PYORI_STRING CommandString,
    __out PYORI_STRING ExpandedString
    )
{
    YORI_CMD_CONTEXT CmdContext;
    LPTSTR NewString;

    if (!YoriShParseCmdlineToCmdContext(CommandString, 0, &CmdContext)) {
        return FALSE;
    }

    if (!YoriShExpandAlias(&CmdContext)) {
        YoriShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    NewString = YoriShBuildCmdlineFromCmdContext(&CmdContext, FALSE, NULL, NULL);
    YoriShFreeCmdContext(&CmdContext);

    YoriLibConstantString(ExpandedString, NewString);
    ExpandedString->MemoryToFree = NewString;
    return TRUE;
}


/**
 Free all aliases.
 */
VOID
YoriShClearAllAliases()
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORI_ALIAS Alias;

    if (YoriShAliases.Next == NULL) {
        return;
    }

    ListEntry = YoriLibGetNextListEntry(&YoriShAliases, NULL);
    while (ListEntry != NULL) {
        Alias = CONTAINING_RECORD(ListEntry, YORI_ALIAS, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&YoriShAliases, ListEntry);
        YoriLibRemoveListItem(&Alias->ListEntry);
        YoriLibDereference(Alias);
    }
}

/**
 Build the complete set of aliases into a an array of key value pairs
 and return a pointer to the result.  This must be freed with a
 subsequent call to @ref YoriLibFreeStringContents .

 @param IncludeInternal TRUE if all aliases, including internal aliases,
        should be included.  FALSE if only user defined aliases should
        be included.

 @param AliasStrings On successful completion, populated with the set of
        alias strings.

 @return Return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShGetAliasStrings(
    __in BOOL IncludeInternal,
    __inout PYORI_STRING AliasStrings
    )
{
    DWORD CharsNeeded = 0;
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORI_ALIAS Alias;
    DWORD StringOffset;

    if (YoriShAliases.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShAliases, NULL);
        while (ListEntry != NULL) {
            Alias = CONTAINING_RECORD(ListEntry, YORI_ALIAS, ListEntry);
            if (IncludeInternal || !Alias->Internal) {
                CharsNeeded += Alias->Alias.LengthInChars + Alias->Value.LengthInChars + 2;
            }
            ListEntry = YoriLibGetNextListEntry(&YoriShAliases, ListEntry);
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

    if (YoriShAliases.Next != NULL) {
        ListEntry = YoriLibGetNextListEntry(&YoriShAliases, NULL);
        while (ListEntry != NULL) {
            Alias = CONTAINING_RECORD(ListEntry, YORI_ALIAS, ListEntry);
            if (IncludeInternal || !Alias->Internal) {
                YoriLibSPrintf(&AliasStrings->StartOfString[StringOffset], _T("%y=%y"), &Alias->Alias, &Alias->Value);
                StringOffset += Alias->Alias.LengthInChars + Alias->Value.LengthInChars + 2;
            }
            ListEntry = YoriLibGetNextListEntry(&YoriShAliases, ListEntry);
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
 Load aliases from the console and incorporate those into the shell's internal
 alias system.  This allows aliases to be inherited across subshells.

 @return TRUE to indicate success or FALSE to indicate failure.
 */
BOOL
YoriShLoadSystemAliases()
{
    HANDLE hKernel;
    DWORD LengthRequired;
    DWORD LengthReturned;
    LPTSTR AliasBuffer;
    LPTSTR ThisVar;
    LPTSTR Value;
    DWORD VarLen;
    DWORD BytesConsumed;

    hKernel = GetModuleHandle(_T("KERNEL32"));
    if (hKernel == NULL) {
        return FALSE;
    }

    pAddConsoleAliasW = (PADD_CONSOLE_ALIASW)GetProcAddress(hKernel, "AddConsoleAliasW");
    pGetConsoleAliasesW = (PGET_CONSOLE_ALIASESW)GetProcAddress(hKernel, "GetConsoleAliasesW");
    pGetConsoleAliasesLengthW = (PGET_CONSOLE_ALIASES_LENGTHW)GetProcAddress(hKernel, "GetConsoleAliasesLengthW");

    if (pAddConsoleAliasW == NULL || pGetConsoleAliasesW == NULL || pGetConsoleAliasesLengthW == NULL) {
        return FALSE;
    }

    LengthRequired = pGetConsoleAliasesLengthW(ALIAS_APP_NAME);
    if (LengthRequired == 0) {
        return TRUE;
    }

    AliasBuffer = YoriLibMalloc(LengthRequired);
    if (AliasBuffer == NULL) {
        return FALSE;
    }

    LengthReturned = pGetConsoleAliasesW(AliasBuffer, LengthRequired, ALIAS_APP_NAME);
    if (LengthReturned == 0 || LengthReturned > LengthRequired) {
        YoriLibFree(AliasBuffer);
        return FALSE;
    }

    ThisVar = AliasBuffer;
    BytesConsumed = 0;
    while (BytesConsumed < LengthReturned && BytesConsumed < LengthRequired) {
        VarLen = _tcslen(ThisVar);
        BytesConsumed += VarLen * sizeof(TCHAR);
        Value = _tcschr(ThisVar, '=');
        if (Value) {
            *Value = '\0';
            Value++;
            YoriShAddAliasLiteral(ThisVar, Value, FALSE);
        }
        ThisVar += VarLen;
        ThisVar++;
        BytesConsumed += sizeof(TCHAR);
    }

    YoriLibFree(AliasBuffer);
    return TRUE;
}


// vim:sw=4:ts=4:et:
