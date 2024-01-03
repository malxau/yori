/**
 * @file builtins/setlocal.c
 *
 * Yori shell push and pop current directories
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
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strSetlocalHelpText[] =
        "\n"
        "Push attributes onto a saved stack to restore later.  By default, the current\n"
        " directory and environment are saved.\n"
        "\n"
        "SETLOCAL [-license] [-c | [-a] [-d] [-e] [-t]]\n"
        "\n"
        "   -a             Save and restore the current aliases\n"
        "   -c             Display the number of entries on the setlocal stack\n"
        "   -d             Save and restore the current directory\n"
        "   -e             Save and restore the environment\n"
        "   -t             Save and restore the window title\n";

/**
 Display usage text to the user.
 */
BOOL
SetlocalHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Setlocal %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSetlocalHelpText);
    return TRUE;
}

/**
 Help text to display to the user.
 */
const
CHAR strEndlocalHelpText[] =
        "\n"
        "Pop a previous saved context from the stack.\n"
        "\n"
        "ENDLOCAL [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
EndlocalHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Endlocal %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strEndlocalHelpText);
    return TRUE;
}

/**
 GetConsoleTitle doesn't say how long the title is until after we've fetched
 it, so this is the maximum allocation setlocal will use to record the window
 title.
 */
#define SETLOCAL_MAX_WINDOW_TITLE_LENGTH (8192)

/**
 If set, the current directory is saved by this setlocal stack entry.
 */
#define SETLOCAL_ATTRIBUTE_DIRECTORY     (0x00000001)

/**
 If set, the environment is saved by this setlocal stack entry.
 */
#define SETLOCAL_ATTRIBUTE_ENVIRONMENT   (0x00000002)

/**
 If set, the window title is saved by this setlocal stack entry.
 */
#define SETLOCAL_ATTRIBUTE_TITLE         (0x00000004)

/**
 If set, the window title is saved by this setlocal stack entry.
 */
#define SETLOCAL_ATTRIBUTE_ALIASES       (0x00000008)

/**
 Information describing saved state at the time of a setlocal call.
 */
typedef struct _SETLOCAL_STACK {

    /**
     Pointers to the next most recent and previous in time setlocal calls.
     */
    YORI_LIST_ENTRY StackLinks;

    /**
     The attributes saved by this setlocal entry.
     */
    DWORD AttributesSaved;

    /**
     A string containing the current directory at the time setlocal was
     executed.
     */
    YORI_STRING PreviousDirectory;

    /**
     A string containing the window title at the time setlocal was executed.
     */
    YORI_STRING PreviousTitle;

    /**
     Pointer to the environment block that was saved when setlocal was
     executed.
     */
    YORI_STRING PreviousEnvironment;

    /**
     Pointer to the alias block that was saved when setlocal was executed.
     */
    YORI_STRING PreviousAliases;
} SETLOCAL_STACK, *PSETLOCAL_STACK;

/**
 A linked list of entries describing when setlocal was executed.  The tail
 of this list corresponds to the most recent setlocal call.
 */
YORI_LIST_ENTRY SetlocalStack;

/**
 Notification that the module is being unloaded or the shell is exiting,
 used to indicate any pending stack should be cleaned up.
 */
VOID
YORI_BUILTIN_FN
SetlocalNotifyUnload(VOID)
{
    PYORI_LIST_ENTRY ListEntry;
    PSETLOCAL_STACK StackLocation;

    if (SetlocalStack.Next == NULL) {
        return;
    }

    ListEntry = YoriLibGetPreviousListEntry(&SetlocalStack, NULL);
    while(ListEntry != NULL) {
        StackLocation = CONTAINING_RECORD(ListEntry, SETLOCAL_STACK, StackLinks);
        ListEntry = YoriLibGetPreviousListEntry(&SetlocalStack, ListEntry);
        YoriLibRemoveListItem(&StackLocation->StackLinks);
        YoriLibFreeStringContents(&StackLocation->PreviousEnvironment);
        YoriLibFree(StackLocation);
    }
}

/**
 Free a saved context, including all child allocations.

 @param StackLocation Pointer to the context to free.
 */
VOID
SetlocalFreeStack(
    __in PSETLOCAL_STACK StackLocation
    )
{
    YoriLibFreeStringContents(&StackLocation->PreviousEnvironment);
    if (StackLocation->PreviousAliases.MemoryToFree != NULL) {
        YoriCallFreeYoriString(&StackLocation->PreviousAliases);
    }
    YoriLibFree(StackLocation);
}

/**
 Pop a saved context from the stack.  This function is only
 registered/available if the stack has something to pop.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_ENDLOCAL(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_ALLOC_SIZE_T i;
    BOOL ArgumentUnderstood;
    PYORI_LIST_ENTRY ListEntry;
    PSETLOCAL_STACK StackLocation;
    YORI_ALLOC_SIZE_T VarLen;
    LPTSTR ThisVar;
    LPTSTR ThisValue;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                EndlocalHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (SetlocalStack.Next == NULL) {
        return EXIT_FAILURE;
    }

    ListEntry = YoriLibGetPreviousListEntry(&SetlocalStack, NULL);
    if (ListEntry == NULL) {
        return EXIT_FAILURE;
    }

    //
    //  Remove this stack entry.  If the stack is now empty, unregister the
    //  endlocal command.
    //

    StackLocation = CONTAINING_RECORD(ListEntry, SETLOCAL_STACK, StackLinks);
    YoriLibRemoveListItem(&StackLocation->StackLinks);

    if (SetlocalStack.Next == &SetlocalStack) {
        YORI_STRING EndlocalCmd;
        YoriLibConstantString(&EndlocalCmd, _T("ENDLOCAL"));
        YoriCallBuiltinUnregister(&EndlocalCmd, YoriCmd_ENDLOCAL);
    }

    //
    //  Restore the current directory.
    //

    if (StackLocation->AttributesSaved & SETLOCAL_ATTRIBUTE_DIRECTORY) {
        if (!YoriCallSetCurrentDirectory(&StackLocation->PreviousDirectory)) {
            SetlocalFreeStack(StackLocation);
            return EXIT_FAILURE;
        }
    }

    //
    //  Restore the window title.
    //

    if (StackLocation->AttributesSaved & SETLOCAL_ATTRIBUTE_TITLE) {
        if (!SetConsoleTitle(StackLocation->PreviousTitle.StartOfString)) {
            SetlocalFreeStack(StackLocation);
            return EXIT_FAILURE;
        }
    }

    //
    //  Query the current environment and delete everything in it.
    //

    if (StackLocation->AttributesSaved & SETLOCAL_ATTRIBUTE_ENVIRONMENT) {
        if (!YoriLibBuiltinSetEnvironmentStrings(&StackLocation->PreviousEnvironment)) {
            SetlocalFreeStack(StackLocation);
            return EXIT_FAILURE;
        }
    }

    //
    //  Query the current aliases and delete them.
    //

    if (StackLocation->AttributesSaved & SETLOCAL_ATTRIBUTE_ALIASES) {
        YORI_STRING CurrentAliases;
        YORI_STRING AliasName;
        YORI_STRING AliasValue;

        if (!YoriCallGetAliasStrings(&CurrentAliases)) {
            SetlocalFreeStack(StackLocation);
            return EXIT_FAILURE;
        }

        YoriLibInitEmptyString(&AliasName);
        YoriLibInitEmptyString(&AliasValue);
        ThisVar = CurrentAliases.StartOfString;
        while (*ThisVar != '\0') {
            YoriLibConstantString(&AliasName, ThisVar);
            VarLen = AliasName.LengthInChars;
            ThisValue = YoriLibFindLeftMostCharacter(&AliasName, '=');
            if (ThisValue != NULL) {
                ThisValue[0] = '\0';
                AliasName.LengthInChars = (YORI_ALLOC_SIZE_T)(ThisValue - ThisVar);
                YoriCallDeleteAlias(&AliasName);
            }

            ThisVar += VarLen;
            ThisVar++;
        }
        YoriCallFreeYoriString(&CurrentAliases);

        //
        //  Now restore the saved aliases.
        //

        ThisVar = StackLocation->PreviousAliases.StartOfString;
        while (*ThisVar != '\0') {
            YoriLibConstantString(&AliasName, ThisVar);
            VarLen = AliasName.LengthInChars;
            ThisValue = YoriLibFindLeftMostCharacter(&AliasName, '=');
            if (ThisValue != NULL) {
                ThisValue[0] = '\0';
                AliasName.LengthInChars = (YORI_ALLOC_SIZE_T)(ThisValue - ThisVar);
                ThisValue++;
                AliasValue.StartOfString = ThisValue;
                AliasValue.LengthInChars = VarLen - AliasName.LengthInChars - 1;
                YoriCallAddAlias(&AliasName, &AliasValue);
            }

            ThisVar += VarLen;
            ThisVar++;
        }

    }
    SetlocalFreeStack(StackLocation);
    return EXIT_SUCCESS;
}

/**
 Display the number of entries on the current setlocal stack.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetlocalDisplayCurrentStackCount(VOID)
{
    PYORI_LIST_ENTRY ListEntry;
    PSETLOCAL_STACK StackLocation;
    DWORD Count = 0;

    if (SetlocalStack.Next != NULL) {
        ListEntry = YoriLibGetPreviousListEntry(&SetlocalStack, NULL);
        while(ListEntry != NULL) {
            Count++;
            StackLocation = CONTAINING_RECORD(ListEntry, SETLOCAL_STACK, StackLinks);
            ListEntry = YoriLibGetPreviousListEntry(&SetlocalStack, ListEntry);
        }
    }
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%i\n"), Count);
    return TRUE;
}

/**
 Push an environment onto the stack.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_SETLOCAL(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    PSETLOCAL_STACK NewStackEntry;
    YORI_ALLOC_SIZE_T CurrentDirectoryLength;
    YORI_STRING Arg;
    DWORD AttributesToSave = 0;
    YORI_ALLOC_SIZE_T ExtraChars;
    LPTSTR CharOffset;
    BOOLEAN CountStack = FALSE;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SetlocalHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                ArgumentUnderstood = TRUE;
                AttributesToSave |= SETLOCAL_ATTRIBUTE_ALIASES;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                ArgumentUnderstood = TRUE;
                CountStack = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                ArgumentUnderstood = TRUE;
                AttributesToSave |= SETLOCAL_ATTRIBUTE_DIRECTORY;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                ArgumentUnderstood = TRUE;
                AttributesToSave |= SETLOCAL_ATTRIBUTE_ENVIRONMENT;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                ArgumentUnderstood = TRUE;
                AttributesToSave |= SETLOCAL_ATTRIBUTE_TITLE;
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (CountStack) {
        if (SetlocalDisplayCurrentStackCount()) {
            return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
    }

    if (AttributesToSave == 0) {
        AttributesToSave = SETLOCAL_ATTRIBUTE_DIRECTORY | SETLOCAL_ATTRIBUTE_ENVIRONMENT;
    }

    ExtraChars = 0;
    CurrentDirectoryLength = 0;
    if (AttributesToSave & SETLOCAL_ATTRIBUTE_DIRECTORY) {
        CurrentDirectoryLength = (YORI_ALLOC_SIZE_T)GetCurrentDirectory(0, NULL);
        ExtraChars = ExtraChars + CurrentDirectoryLength;
    }

    //
    //  GetConsoleTitle doesn't seem to return the number of characters
    //  available, so we have to be pessimistic.
    //

    if (AttributesToSave & SETLOCAL_ATTRIBUTE_TITLE) {
        ExtraChars += SETLOCAL_MAX_WINDOW_TITLE_LENGTH;
    }

    NewStackEntry = YoriLibMalloc(sizeof(SETLOCAL_STACK) + ExtraChars * sizeof(TCHAR));
    if (NewStackEntry == NULL) {
        return EXIT_FAILURE;
    }

    YoriLibInitEmptyString(&NewStackEntry->PreviousDirectory);
    YoriLibInitEmptyString(&NewStackEntry->PreviousTitle);
    YoriLibInitEmptyString(&NewStackEntry->PreviousEnvironment);
    YoriLibInitEmptyString(&NewStackEntry->PreviousAliases);

    CharOffset = (LPTSTR)(NewStackEntry + 1);
    if (AttributesToSave & SETLOCAL_ATTRIBUTE_DIRECTORY) {
        NewStackEntry->PreviousDirectory.StartOfString = CharOffset;
        NewStackEntry->PreviousDirectory.LengthAllocated = CurrentDirectoryLength;
        NewStackEntry->PreviousDirectory.LengthInChars = (YORI_ALLOC_SIZE_T)GetCurrentDirectory(CurrentDirectoryLength, NewStackEntry->PreviousDirectory.StartOfString);
        CharOffset += CurrentDirectoryLength;
    }

    if (AttributesToSave & SETLOCAL_ATTRIBUTE_TITLE) {
        NewStackEntry->PreviousTitle.StartOfString = CharOffset;
        NewStackEntry->PreviousTitle.LengthAllocated = SETLOCAL_MAX_WINDOW_TITLE_LENGTH;
        NewStackEntry->PreviousTitle.LengthInChars = (YORI_ALLOC_SIZE_T)GetConsoleTitle(NewStackEntry->PreviousTitle.StartOfString, NewStackEntry->PreviousTitle.LengthAllocated);
        CharOffset += SETLOCAL_MAX_WINDOW_TITLE_LENGTH;
    }

    if (AttributesToSave & SETLOCAL_ATTRIBUTE_ENVIRONMENT) {
        if (!YoriLibGetEnvironmentStrings(&NewStackEntry->PreviousEnvironment)) {
            SetlocalFreeStack(NewStackEntry);
            return EXIT_FAILURE;
        }
    }

    if (AttributesToSave & SETLOCAL_ATTRIBUTE_ALIASES) {
        if (!YoriCallGetAliasStrings(&NewStackEntry->PreviousAliases)) {
            SetlocalFreeStack(NewStackEntry);
            return EXIT_FAILURE;
        }
    }

    NewStackEntry->AttributesSaved = AttributesToSave;

    if (SetlocalStack.Next == NULL) {
        YoriLibInitializeListHead(&SetlocalStack);
    }
    if (SetlocalStack.Next == &SetlocalStack) {
        YORI_STRING EndlocalCmd;
        YoriLibConstantString(&EndlocalCmd, _T("ENDLOCAL"));
        if (!YoriCallBuiltinRegister(&EndlocalCmd, YoriCmd_ENDLOCAL)) {
            SetlocalFreeStack(NewStackEntry);
            return EXIT_FAILURE;
        }

        YoriCallSetUnloadRoutine(SetlocalNotifyUnload);
    }

    YoriLibAppendList(&SetlocalStack, &NewStackEntry->StackLinks);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
