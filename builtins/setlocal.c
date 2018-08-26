/**
 * @file builtins/setlocal.c
 *
 * Yori shell push and pop current directories
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
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strSetlocalHelpText[] =
        "\n"
        "Push the current directory and environment onto a saved stack.\n"
        "\n"
        "SETLOCAL [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
SetlocalHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Setlocal %i.%i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
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
        "Pop a previous saved environment from the stack.\n"
        "\n"
        "ENDLOCAL [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
EndlocalHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Endlocal %i.%i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strEndlocalHelpText);
    return TRUE;
}

/**
 Information describing saved state at the time of a setlocal call.
 */
typedef struct _SETLOCAL_STACK {

    /**
     Pointers to the next most recent and previous in time setlocal calls.
     */
    YORI_LIST_ENTRY StackLinks;

    /**
     A string containing the current directory at the time setlocal was
     executed.
     */
    YORI_STRING PreviousDirectory;

    /**
     Pointer to the environment block that was saved when setlocal was
     executed.
     */
    YORI_STRING PreviousEnvironment;
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
SetlocalNotifyUnload()
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
 Pop an environment onto the stack.  This function is only
 registered/available if the stack has something to pop.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_ENDLOCAL(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    DWORD i;
    BOOL ArgumentUnderstood;
    PYORI_LIST_ENTRY ListEntry;
    PSETLOCAL_STACK StackLocation;
    YORI_STRING CurrentEnvironment;
    DWORD VarLen;
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

    if (!SetCurrentDirectory(StackLocation->PreviousDirectory.StartOfString)) {
        YoriLibFreeStringContents(&StackLocation->PreviousEnvironment);
        YoriLibFree(StackLocation);
        return EXIT_FAILURE;
    }

    //
    //  Query the current environment and delete everything in it.
    //

    if (!YoriLibGetEnvironmentStrings(&CurrentEnvironment)) {
        YoriLibFreeStringContents(&StackLocation->PreviousEnvironment);
        YoriLibFree(StackLocation);
        return EXIT_FAILURE;
    }
    ThisVar = CurrentEnvironment.StartOfString;
    while (*ThisVar != '\0') {
        VarLen = _tcslen(ThisVar);

        //
        //  We know there's at least one char.  Skip it if it's equals since
        //  that's how drive current directories are recorded.
        //

        ThisValue = _tcschr(&ThisVar[1], '=');
        if (ThisValue != NULL) {
            ThisValue[0] = '\0';
            SetEnvironmentVariable(ThisVar, NULL);
        }

        ThisVar += VarLen;
        ThisVar++;
    }
    YoriLibFreeStringContents(&CurrentEnvironment);

    //
    //  Now restore the saved environment.
    //

    ThisVar = StackLocation->PreviousEnvironment.StartOfString;
    while (*ThisVar != '\0') {
        VarLen = _tcslen(ThisVar);

        //
        //  We know there's at least one char.  Skip it if it's equals since
        //  that's how drive current directories are recorded.
        //

        ThisValue = _tcschr(&ThisVar[1], '=');
        if (ThisValue != NULL) {
            ThisValue[0] = '\0';
            ThisValue++;
            SetEnvironmentVariable(ThisVar, ThisValue);
        }

        ThisVar += VarLen;
        ThisVar++;
    }

    YoriLibFreeStringContents(&StackLocation->PreviousEnvironment);
    YoriLibFree(StackLocation);
    return EXIT_SUCCESS;
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
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    PSETLOCAL_STACK NewStackEntry;
    DWORD CurrentDirectoryLength;
    YORI_STRING Arg;

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
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    CurrentDirectoryLength = GetCurrentDirectory(0, NULL);

    NewStackEntry = YoriLibMalloc(sizeof(SETLOCAL_STACK) + CurrentDirectoryLength * sizeof(TCHAR));
    if (NewStackEntry == NULL) {
        return EXIT_FAILURE;
    }

    YoriLibInitEmptyString(&NewStackEntry->PreviousDirectory);
    NewStackEntry->PreviousDirectory.StartOfString = (LPTSTR)(NewStackEntry + 1);
    NewStackEntry->PreviousDirectory.LengthInChars = GetCurrentDirectory(CurrentDirectoryLength, NewStackEntry->PreviousDirectory.StartOfString);

    if (!YoriLibGetEnvironmentStrings(&NewStackEntry->PreviousEnvironment)) {
        YoriLibFree(NewStackEntry);
        return EXIT_FAILURE;
    }

    if (SetlocalStack.Next == NULL) {
        YoriLibInitializeListHead(&SetlocalStack);
    }
    if (SetlocalStack.Next == &SetlocalStack) {
        YORI_STRING EndlocalCmd;
        YoriLibConstantString(&EndlocalCmd, _T("ENDLOCAL"));
        if (!YoriCallBuiltinRegister(&EndlocalCmd, YoriCmd_ENDLOCAL)) {
            YoriLibFreeStringContents(&NewStackEntry->PreviousEnvironment);
            YoriLibFree(NewStackEntry);
            return EXIT_FAILURE;
        }

        YoriCallSetUnloadRoutine(SetlocalNotifyUnload);
    }

    YoriLibAppendList(&SetlocalStack, &NewStackEntry->StackLinks);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
