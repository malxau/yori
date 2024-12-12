/**
 * @file builtins/ys.c
 *
 * Yori shell script interpreter
 *
 * Copyright (c) 2017-2020 Malcolm J. Smith
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
CHAR strYsHelpText[] =
        "\n"
        "Execute a script in Yori.\n"
        "\n"
        "YS [-license] <script>\n"
        "\n"
        "Yori scripts are different to CMD scripts.  Notable changes include:\n"
        " 1. Parameters are referred to as %1%, %2%, ... rather than %1, %2 ...\n"
        " 2. Call label will isolate state.  To return, use 'return' rather\n"
        "    than 'exit /b'.\n"
        " 3. The script name and location is in %~SCRIPTNAME%.  Use the path command\n"
        "    to decompose into parts.\n";

/**
 Help text to display to the user.
 */
const
CHAR strCallHelpText[] =
        "\n"
        "Call a subroutine.\n"
        "\n"
        "CALL [-license] <label>\n";

/**
 Help text to display to the user.
 */
const
CHAR strGotoHelpText[] =
        "\n"
        "Goto a label in a script.\n"
        "\n"
        "GOTO [-license] <label>\n";

/**
 Help text to display to the user.
 */
const
CHAR strIncludeHelpText[] =
        "\n"
        "Include a script within another script.\n"
        "\n"
        "INCLUDE [-license] <file>\n";

/**
 Help text to display to the user.
 */
const
CHAR strReturnHelpText[] =
        "\n"
        "Return from a subroutine or end a script with a return code.  When returning\n"
        "from a subroutine all environment variables are reset except for those listed\n"
        "following the return statement.\n"
        "\n"
        "RETURN [-license] <exitcode> [<variables to preserve>]\n";

/**
 Help text to display to the user.
 */
const
CHAR strShiftHelpText[] =
        "\n"
        "Shift command arguments left by one.\n"
        "\n"
        "SHIFT [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
YsHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ys %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYsHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\nHelp for commands available within scripts:\n"));
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strCallHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strGotoHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strIncludeHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strReturnHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strShiftHelpText);
    return TRUE;
}

/**
 Display usage text to the user.
 */
BOOL
CallHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ys %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strCallHelpText);
    return TRUE;
}

/**
 Display usage text to the user.
 */
BOOL
GotoHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ys %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strGotoHelpText);
    return TRUE;
}

/**
 Display usage text to the user.
 */
BOOL
IncludeHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ys %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strIncludeHelpText);
    return TRUE;
}

/**
 Display usage text to the user.
 */
BOOL
ReturnHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ys %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strReturnHelpText);
    return TRUE;
}

/**
 Display usage text to the user.
 */
BOOL
ShiftHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ys %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strShiftHelpText);
    return TRUE;
}

/**
 Information describing the %1% arguments within a script.
 */
typedef struct _YS_ARGUMENT_CONTEXT {

    /**
     The number of shifts applied to the arguments.
     */
    YORI_ALLOC_SIZE_T ShiftCount;

    /**
     The total number of arguments present.
     */
    YORI_ALLOC_SIZE_T ArgC;

    /**
     An array of strings describing the argument contents.
     */
    PYORI_STRING ArgV;

} YS_ARGUMENT_CONTEXT, *PYS_ARGUMENT_CONTEXT;

/**
 Information about a single line within a Yori script.
 */
typedef struct _YS_SCRIPT_LINE {

    /**
     A linked list pointing to the next and previous lines.
     */
    YORI_LIST_ENTRY LineLinks;

    /**
     A string containing the contents of the line.
     */
    YORI_STRING LineContents;

} YS_SCRIPT_LINE, *PYS_SCRIPT_LINE;

/**
 Information describing saved state at the time of a call.
 */
typedef struct _YS_CALL_STACK {

    /**
     Pointers to the next most recent and previous in time calls.
     */
    YORI_LIST_ENTRY StackLinks;

    /**
     A string containing the current directory at the time a call was
     executed.
     */
    YORI_STRING PreviousDirectory;

    /**
     Pointer to the environment block that was saved when call was
     executed.
     */
    YORI_STRING PreviousEnvironment;

    /**
     Pointer to the line that originated the call.
     */
    PYS_SCRIPT_LINE CallingLine;

    /**
     The argument context associated with this call.
     */
    YS_ARGUMENT_CONTEXT ArgContext;

} YS_CALL_STACK, *PYS_CALL_STACK;


/**
 A structure describing a Yori script.
 */
typedef struct _YS_SCRIPT {

    /**
     A linked list of lines within the script.
     */
    YORI_LIST_ENTRY LineLinks;

    /**
     A linked list of call context information.
     */
    YORI_LIST_ENTRY CallStackLinks;

    /**
     File name string.
     */
    YORI_STRING FileName;

    /**
     Pointer to the active line within the script.  This can be moved during
     execution via goto or similar.
     */
    PYS_SCRIPT_LINE ActiveLine;

    /**
     The global argument context of the script, describing the arguments that
     should be used when not executing functions within the script.
     */
    YS_ARGUMENT_CONTEXT GlobalArgContext;

    /**
     The current argument context of the script, describing how %1% et al
     should be expanded.
     */
    PYS_ARGUMENT_CONTEXT ArgContext;

} YS_SCRIPT, *PYS_SCRIPT;

/**
 Pointer to the active script.  This can be changed by executing a script
 within a script.
 */
PYS_SCRIPT YsActiveScript = NULL;

/**
 Advance execution to the end of the script.  This causes script processing
 to end, but ensures that it ends organically and all cleanup is performed.
 */
VOID
YsGotoScriptEnd(VOID)
{
    PYORI_LIST_ENTRY ListEntry;
    PYS_SCRIPT_LINE Line;

    ListEntry = YoriLibGetPreviousListEntry(&YsActiveScript->LineLinks, NULL);
    ASSERT(ListEntry != NULL);
    if (ListEntry != NULL) {
        Line = CONTAINING_RECORD(ListEntry, YS_SCRIPT_LINE, LineLinks);
        YsActiveScript->ActiveLine = Line;
    }
}

/**
 Switch the actively executing line within the script to the specified label,
 if it can be found.

 @param Label The label to make active.

 @return TRUE if the label was found and made active, FALSE if it was not
 found.
 */
BOOL
YsGotoLabel(
    __in LPTSTR Label
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYS_SCRIPT_LINE Line;

    //
    //  First special case :eof for no good reason other than CMD does.
    //

    if (_tcsicmp(Label, _T(":eof")) == 0) {
        YsGotoScriptEnd();
        return TRUE;
    }

    //
    //  Now look for user defined labels within the script.
    //

    ListEntry = YoriLibGetNextListEntry(&YsActiveScript->LineLinks, NULL);
    while (ListEntry != NULL) {
        Line = CONTAINING_RECORD(ListEntry, YS_SCRIPT_LINE, LineLinks);
        if (Line->LineContents.LengthInChars > 1 &&
            Line->LineContents.StartOfString[0] == ':') {

            YORI_STRING LabelString;

            YoriLibInitEmptyString(&LabelString);
            LabelString.StartOfString = &Line->LineContents.StartOfString[1];
            LabelString.LengthInChars = Line->LineContents.LengthInChars - 1;

            if (LabelString.LengthInChars >= 1 &&
                LabelString.StartOfString[LabelString.LengthInChars - 1] == '\0') {
                LabelString.LengthInChars--;
            }

            if (YoriLibCompareStringLitIns(&LabelString, Label) == 0) {
                YsActiveScript->ActiveLine = Line;
                return TRUE;
            }
        }
        ListEntry = YoriLibGetNextListEntry(&YsActiveScript->LineLinks, ListEntry);
    }

    return FALSE;
}

/**
 Deallocate any memory associated with recursing into an isolated stack
 state.

 @param StackLocation The stack location to deallocate.
 */
VOID
YsFreeCallStack(
    __in PYS_CALL_STACK StackLocation
    )
{
    YORI_ALLOC_SIZE_T i;
    YoriLibFreeStringContents(&StackLocation->PreviousEnvironment);
    for (i = 0; i < StackLocation->ArgContext.ArgC; i++) {
        YoriLibFreeStringContents(&StackLocation->ArgContext.ArgV[i]);
    }
    YoriLibFree(StackLocation);
}

/**
 Load script lines from an input stream into a linked list of lines.

 @param Handle The input stream to load lines from.

 @param ListHead The head of the list to populate with lines.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YsLoadLines(
    __in HANDLE Handle,
    __inout PYORI_LIST_ENTRY ListHead
    )
{
    PVOID LineContext = NULL;
    PYS_SCRIPT_LINE ThisLine;
    PYORI_LIST_ENTRY InsertPoint = ListHead;

    while (TRUE) {

        ThisLine = YoriLibMalloc(sizeof(YS_SCRIPT_LINE));
        if (ThisLine == NULL) {
            YoriLibLineReadCloseOrCache(LineContext);
            return FALSE;
        }

        YoriLibInitEmptyString(&ThisLine->LineContents);

        if (!YoriLibReadLineToString(&ThisLine->LineContents, &LineContext, Handle)) {
            YoriLibFree(ThisLine);
            YoriLibLineReadCloseOrCache(LineContext);
            return TRUE;
        }

        //
        //  LineContents is NULL terminated but doesn't include the NULL in
        //  the string.  Here it's convenient to include it since
        //  ExecuteExpression will want it.
        //

        ASSERT(ThisLine->LineContents.StartOfString[ThisLine->LineContents.LengthInChars] == '\0');
        ThisLine->LineContents.LengthInChars++;

        YoriLibInsertList(InsertPoint, &ThisLine->LineLinks);
        InsertPoint = &ThisLine->LineLinks;
    }
}

/**
 Return from an isolated stack state or script.

 @param ArgC Count of arguments.

 @param ArgV Array of arguments.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_RETURN(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg;
    BOOLEAN ArgumentUnderstood;
    PYORI_LIST_ENTRY ListEntry;
    PYS_CALL_STACK StackLocation;
    YORI_STRING CurrentEnvironment;
    YORI_ALLOC_SIZE_T VarLen;
    DWORD ExitCode;
    LPTSTR ThisVar;
    LPTSTR ThisValue;
    BOOLEAN PreserveVariable;
    YORI_STRING Arg;
    YORI_STRING VariableName;
    YORI_STRING ValueName;

    StartArg = 0;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                ReturnHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2020"));
                return EXIT_SUCCESS;
            }
        } else {
            StartArg = i;
            ArgumentUnderstood = TRUE;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (YsActiveScript == NULL) {
        return EXIT_FAILURE;
    }

    StackLocation = NULL;
    ListEntry = YoriLibGetPreviousListEntry(&YsActiveScript->CallStackLinks, NULL);
    if (ListEntry != NULL) {

        //
        //  Remove this stack entry.
        //

        StackLocation = CONTAINING_RECORD(ListEntry, YS_CALL_STACK, StackLinks);
        YoriLibRemoveListItem(&StackLocation->StackLinks);
    }


    ExitCode = 0;
    if (StartArg != 0) {
        YORI_MAX_SIGNED_T LlExitCode;
        YORI_ALLOC_SIZE_T CharsConsumed;

        if (!YoriLibStringToNumber(&ArgV[StartArg], TRUE, &LlExitCode, &CharsConsumed) ||
             CharsConsumed == 0) {
            ExitCode = 0;
        } else {
            ExitCode = (DWORD)LlExitCode;
        }
    }

    //
    //  When returning from a subroutine, a stack location is defined to
    //  indicate state to restore to the previous execution scope.  If leaving
    //  a script completely, there is no isolation: scripts that make
    //  modifications in global scope are typically intending to change the
    //  user's session after they exit.
    //

    if (StackLocation != NULL) {

        //
        //  Restore the current directory.
        //

        if (!YoriCallSetCurrentDirectory(&StackLocation->PreviousDirectory)) {
            YsFreeCallStack(StackLocation);
            return EXIT_FAILURE;
        }

        //
        //  Query the current environment and delete everything in it.
        //

        if (!YoriLibGetEnvironmentStrings(&CurrentEnvironment)) {
            YsFreeCallStack(StackLocation);
            return EXIT_FAILURE;
        }

        YoriLibInitEmptyString(&VariableName);
        YoriLibInitEmptyString(&ValueName);

        ThisVar = CurrentEnvironment.StartOfString;
        while (*ThisVar != '\0') {
            VarLen = (YORI_ALLOC_SIZE_T)_tcslen(ThisVar);

            //
            //  We know there's at least one char.  Skip it if it's equals since
            //  that's how drive current directories are recorded.
            //

            ThisValue = _tcschr(&ThisVar[1], '=');
            if (ThisValue != NULL) {
                ThisValue[0] = '\0';
                VariableName.StartOfString = ThisVar;
                VariableName.LengthInChars = (YORI_ALLOC_SIZE_T)(ThisValue - ThisVar);
                VariableName.LengthAllocated = VariableName.LengthInChars + 1;
                for (i = StartArg + 1, PreserveVariable = FALSE; i < ArgC; i++) {
                    if (YoriLibCompareStringLitIns(&ArgV[i], ThisVar) == 0) {
                        PreserveVariable = TRUE;
                    }
                }
                if (!PreserveVariable) {
                    YoriCallSetEnvironmentVariable(&VariableName, NULL);
                }
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
            VarLen = (YORI_ALLOC_SIZE_T)_tcslen(ThisVar);

            //
            //  We know there's at least one char.  Skip it if it's equals since
            //  that's how drive current directories are recorded.
            //

            ThisValue = _tcschr(&ThisVar[1], '=');
            if (ThisValue != NULL) {
                ThisValue[0] = '\0';
                VariableName.StartOfString = ThisVar;
                VariableName.LengthInChars = (YORI_ALLOC_SIZE_T)(ThisValue - ThisVar);
                VariableName.LengthAllocated = VariableName.LengthInChars + 1;
                ThisValue++;
                ValueName.StartOfString = ThisValue;
                ValueName.LengthInChars = VarLen - VariableName.LengthInChars - 1;
                ValueName.LengthAllocated = ValueName.LengthInChars + 1;
                for (i = StartArg + 1, PreserveVariable = FALSE; i < ArgC; i++) {
                    if (YoriLibCompareStringLitIns(&ArgV[i], ThisVar) == 0) {
                        PreserveVariable = TRUE;
                    }
                }
                if (!PreserveVariable) {
                    YoriCallSetEnvironmentVariable(&VariableName, &ValueName);
                }
            }

            ThisVar += VarLen;
            ThisVar++;
        }

        YsActiveScript->ActiveLine = StackLocation->CallingLine;
        YsFreeCallStack(StackLocation);
        ListEntry = YoriLibGetPreviousListEntry(&YsActiveScript->CallStackLinks, NULL);
        if (ListEntry != NULL) {
            StackLocation = CONTAINING_RECORD(ListEntry, YS_CALL_STACK, StackLinks);
            YsActiveScript->ArgContext = &StackLocation->ArgContext;
        } else {
            YsActiveScript->ArgContext = &YsActiveScript->GlobalArgContext;
        }

    } else {

        //
        //  If there's no scope so we're running at the global scope, treat
        //  this operation the same as "goto :eof"
        //

        YsGotoScriptEnd();
    }
    return ExitCode;
}

/**
 Call a subroutine.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_CALL(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    PYS_CALL_STACK NewStackEntry;
    YORI_ALLOC_SIZE_T CurrentDirectoryLength;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                CallHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
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

    ASSERT(YsActiveScript != NULL);
    if (YsActiveScript == NULL) {
        return EXIT_FAILURE;
    }

    CurrentDirectoryLength = (YORI_ALLOC_SIZE_T)GetCurrentDirectory(0, NULL);

    NewStackEntry = YoriLibMalloc(sizeof(YS_CALL_STACK) + ArgC * sizeof(YORI_STRING) + CurrentDirectoryLength * sizeof(TCHAR));
    if (NewStackEntry == NULL) {
        return EXIT_FAILURE;
    }

    ZeroMemory(NewStackEntry, sizeof(YS_CALL_STACK) + ArgC * sizeof(YORI_STRING));

    YoriLibInitEmptyString(&NewStackEntry->PreviousDirectory);
    NewStackEntry->ArgContext.ShiftCount = 1;
    NewStackEntry->ArgContext.ArgC = ArgC;
    NewStackEntry->ArgContext.ArgV = (PYORI_STRING)(NewStackEntry + 1);
    NewStackEntry->PreviousDirectory.StartOfString = (LPTSTR)YoriLibAddToPointer(NewStackEntry, sizeof(YS_CALL_STACK) + ArgC * sizeof(YORI_STRING));
    NewStackEntry->PreviousDirectory.LengthInChars = (YORI_ALLOC_SIZE_T)GetCurrentDirectory(CurrentDirectoryLength, NewStackEntry->PreviousDirectory.StartOfString);
    NewStackEntry->PreviousDirectory.LengthAllocated = CurrentDirectoryLength;

    if (!YoriLibGetEnvironmentStrings(&NewStackEntry->PreviousEnvironment)) {
        YsFreeCallStack(NewStackEntry);
        return EXIT_FAILURE;
    }

    for (i = 0; i < ArgC; i++) {
        YORI_ALLOC_SIZE_T ArgLength = ArgV[i].LengthInChars;
        if (!YoriLibAllocateString(&NewStackEntry->ArgContext.ArgV[i], ArgV[i].LengthInChars + 1)) {
            YsFreeCallStack(NewStackEntry);
            return EXIT_FAILURE;
        }
        NewStackEntry->ArgContext.ArgV[i].LengthAllocated = ArgV[i].LengthInChars + 1;
        NewStackEntry->ArgContext.ArgV[i].LengthInChars = ArgV[i].LengthInChars;
        memcpy(NewStackEntry->ArgContext.ArgV[i].StartOfString, ArgV[i].StartOfString, ArgLength * sizeof(TCHAR));
        NewStackEntry->ArgContext.ArgV[i].StartOfString[ArgV[i].LengthInChars] = '\0';
    }

    NewStackEntry->CallingLine = YsActiveScript->ActiveLine;

    if (!YsGotoLabel(ArgV[StartArg].StartOfString)) {
        YsFreeCallStack(NewStackEntry);
        return EXIT_FAILURE;
    }

    YsActiveScript->ArgContext = &NewStackEntry->ArgContext;

    YoriLibAppendList(&YsActiveScript->CallStackLinks, &NewStackEntry->StackLinks);

    return EXIT_SUCCESS;
}


/**
 Goto a label in the script without changing any isolation context.

 @param ArgC Count of arguments.

 @param ArgV Array of arguments.

 @return ExitCode, zero indicating success, nonzero indicating failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_GOTO(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    BOOLEAN ArgumentUnderstood;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                GotoHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("goto: missing argument\n"));
        return EXIT_FAILURE;
    }

    ASSERT(YsActiveScript != NULL);
    if (YsActiveScript == NULL) {
        return EXIT_FAILURE;
    }

    if (YsGotoLabel(ArgV[StartArg].StartOfString)) {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

/**
 Include another script file at the current location in the present script.

 @param ArgC Count of arguments.

 @param ArgV Array of arguments.

 @return ExitCode, zero indicating success, nonzero indicating failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_INCLUDE(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    BOOLEAN ArgumentUnderstood;
    HANDLE FileHandle;
    YORI_STRING FileName;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                IncludeHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("include: missing argument\n"));
        return EXIT_FAILURE;
    }

    ASSERT(YsActiveScript != NULL);
    if (YsActiveScript == NULL) {
        return EXIT_FAILURE;
    }

    if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &FileName)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ys: getfullpathname of %y failed: %s"), &ArgV[StartArg], ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return EXIT_FAILURE;
    }

    FileHandle = CreateFile(FileName.StartOfString,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ys: could not open %y\n"), &FileName);
        YoriLibFreeStringContents(&FileName);
        return EXIT_FAILURE;
    }

    YoriLibFreeStringContents(&FileName);

    if (!YsLoadLines(FileHandle, &YsActiveScript->ActiveLine->LineLinks)) {
        CloseHandle(FileHandle);
        return EXIT_FAILURE;
    }

    CloseHandle(FileHandle);

    return EXIT_SUCCESS;
}

/**
 Shift all script arguments left by one, such that the first argument
 becomes inaccessible, the second argument becomes the first, etc.

 @param ArgC Count of arguments (to this command.)

 @param ArgV Array of arguments (to this command.)

 @return ExitCode, zero indicating success, nonzero indicating failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_SHIFT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_ALLOC_SIZE_T i;
    BOOLEAN ArgumentUnderstood;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                ShiftHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    ASSERT(YsActiveScript != NULL);
    if (YsActiveScript == NULL) {
        return EXIT_FAILURE;
    }

    if (YsActiveScript->ArgContext->ShiftCount < YsActiveScript->ArgContext->ArgC) {
        YsActiveScript->ArgContext->ShiftCount++;
    }

    return EXIT_FAILURE;
}


/**
 Expand any variables that refer to script arguments, or special variables only
 meaningful when executing scripts.

 @param OutputString The output buffer to populate with the expanded result.

 @param VariableName The name of the variable to expand.

 @param Context Context describing the state of the script which contains the
        values used to substitute in event of expansion.

 @return The number of characters populated into the buffer, or the number
         of characters required in order to successfully populate the buffer.
 */
YORI_ALLOC_SIZE_T
YsExpandArgumentVariables(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    YORI_ALLOC_SIZE_T CharsConsumed;
    YORI_MAX_SIGNED_T ArgIndex;
    YORI_ALLOC_SIZE_T StringLength;
    PYS_ARGUMENT_CONTEXT ArgContext = (PYS_ARGUMENT_CONTEXT)Context;

    if (YoriLibCompareStringLit(VariableName, _T("~SCRIPTNAME")) == 0) {
        if (OutputString->LengthAllocated >= YsActiveScript->FileName.LengthInChars) {
            memcpy(OutputString->StartOfString, YsActiveScript->FileName.StartOfString, YsActiveScript->FileName.LengthInChars * sizeof(TCHAR));
        }
        return YsActiveScript->FileName.LengthInChars;
    }

    if (YoriLibCompareStringLit(VariableName, _T("*")) == 0) {
        YORI_STRING EntireLine;

        if (ArgContext->ArgC < ArgContext->ShiftCount + 1) {
            return 0;
        }

        YoriLibInitEmptyString(&EntireLine);
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgContext->ArgC - ArgContext->ShiftCount - 1, &ArgContext->ArgV[ArgContext->ShiftCount + 1], TRUE, FALSE, &EntireLine)) {
            return 0;
        }

        StringLength = EntireLine.LengthInChars;
        if (OutputString->LengthAllocated >= StringLength) {
            memcpy(OutputString->StartOfString, EntireLine.StartOfString, EntireLine.LengthInChars * sizeof(TCHAR));
        }
        YoriLibFreeStringContents(&EntireLine);
        return StringLength;
    }

    if (YoriLibStringToNumber(VariableName, TRUE, &ArgIndex, &CharsConsumed) && CharsConsumed > 0 && CharsConsumed == VariableName->LengthInChars) {
        if (ArgIndex > 0 && ArgIndex + ArgContext->ShiftCount < ArgContext->ArgC) {

            StringLength = ArgContext->ArgV[ArgIndex + ArgContext->ShiftCount].LengthInChars;
            if (OutputString->LengthAllocated >= StringLength) {
                memcpy(OutputString->StartOfString, ArgContext->ArgV[ArgIndex + ArgContext->ShiftCount].StartOfString, StringLength  * sizeof(TCHAR));
            }
            return StringLength;
        } else {
            return 0;
        }
    }

    if (OutputString->LengthAllocated >= VariableName->LengthInChars + 2) {
        OutputString->StartOfString[0] = '%';
        memcpy(&OutputString->StartOfString[1], VariableName->StartOfString, VariableName->LengthInChars * sizeof(TCHAR));
        OutputString->StartOfString[VariableName->LengthInChars + 1] = '%';
    }
    return VariableName->LengthInChars + 2;
}

/**
 A structure the maps a command name string into a function callback to
 invoke if that string is used as a command within a script.
 */
typedef struct _YS_SCRIPT_COMMANDS {

    /**
     The name of the command, in string form.
     */
    LPTSTR CommandName;

    /**
     The callback function to invoke if the string is used as an executable
     expression.
     */
    PYORI_CMD_BUILTIN Function;
} YS_SCRIPT_COMMANDS, *PYS_SCRIPT_COMMANDS;

/**
 An array of builtin command which are only meaningful during script
 execution.
 */
YS_SCRIPT_COMMANDS YsScriptCommands[] = {
    {_T("CALL"),        YoriCmd_CALL},
    {_T("GOTO"),        YoriCmd_GOTO},
    {_T("INCLUDE"),     YoriCmd_INCLUDE},
    {_T("RETURN"),      YoriCmd_RETURN},
    {_T("SHIFT"),       YoriCmd_SHIFT}
};

/**
 Given a script already loaded into memory, commence execution.  This will
 register builtin commands to execute, set this script to the active one,
 execute the script line by line, and unregister builtin functions on
 completion.  Note this function can be entered recursively (a script can
 invoke another script), and this implies that builtin functions can be
 recursively registered (with the newest taking precedence.)

 @param Script The script to execute.

 @return TRUE indicating success, FALSE indicating failure.
 */
BOOL
YsExecuteScript(
    __in PYS_SCRIPT Script
    )
{
    YORI_STRING LineWithArgumentsExpanded;
    YORI_STRING CommandName;
    DWORD Index;
    PYORI_LIST_ENTRY NextEntry;
    PYS_SCRIPT_LINE CurrentLine;
    PYS_SCRIPT PreviouslyActiveScript;

    for (Index = 0; Index < sizeof(YsScriptCommands)/sizeof(YsScriptCommands[0]); Index++) {
        YoriLibConstantString(&CommandName, YsScriptCommands[Index].CommandName);
        if (!YoriCallBuiltinRegister(&CommandName, YsScriptCommands[Index].Function)) {
            while(Index > 0) {
                Index--;
                YoriLibConstantString(&CommandName, YsScriptCommands[Index].CommandName);
                YoriCallBuiltinUnregister(&CommandName, YsScriptCommands[Index].Function);
            }
            return FALSE;
        }
    }

    PreviouslyActiveScript = YsActiveScript;
    YsActiveScript = Script;

    YoriLibInitEmptyString(&LineWithArgumentsExpanded);
    NextEntry = YoriLibGetNextListEntry(&Script->LineLinks, NULL);
    while(NextEntry != NULL) {
        CurrentLine = CONTAINING_RECORD(NextEntry, YS_SCRIPT_LINE, LineLinks);
        Script->ActiveLine = CurrentLine;

        if (CurrentLine->LineContents.LengthInChars > 1 &&
            CurrentLine->LineContents.StartOfString[0] != ':') {

            if (!YoriLibExpandCommandVariables(&CurrentLine->LineContents, '%', TRUE, YsExpandArgumentVariables, Script->ArgContext, &LineWithArgumentsExpanded)) {
                break;
            }

            //
            //  Lines are intentionally left with NULLs inside the string, so
            //  we'd normally truncate these here.  When an incomplete command
            //  expansion is used though, the NULL ends up in the variable name
            //  so it can get truncated.  YoriLibExpandCommandVariables
            //  also adds one, but it's not within the string, so check which
            //  case we're in.
            //

            if (LineWithArgumentsExpanded.LengthInChars > 0 &&
                LineWithArgumentsExpanded.StartOfString[LineWithArgumentsExpanded.LengthInChars - 1] == '\0') {
                LineWithArgumentsExpanded.LengthInChars--;
            }
            ASSERT(LineWithArgumentsExpanded.StartOfString[LineWithArgumentsExpanded.LengthInChars] == '\0');

            YoriCallExecuteExpression(&LineWithArgumentsExpanded);
            ASSERT(YsActiveScript == Script);

            if (YoriCallIsProcessExiting()) {
                YsGotoScriptEnd();
            }
        }

        NextEntry = YoriLibGetNextListEntry(&Script->LineLinks, &Script->ActiveLine->LineLinks);
    }

    YsActiveScript = PreviouslyActiveScript;
    YoriLibFreeStringContents(&LineWithArgumentsExpanded);

    Index = sizeof(YsScriptCommands)/sizeof(YsScriptCommands[0]);
    while(Index > 0) {
        Index--;
        YoriLibConstantString(&CommandName, YsScriptCommands[Index].CommandName);
        YoriCallBuiltinUnregister(&CommandName, YsScriptCommands[Index].Function);
    }

    return TRUE;
}

/**
 Deallocate any structures used to record a script in memory.

 @param Script The in memory script and state to deallocate.
 */
VOID
YsFreeScript(
    __in PYS_SCRIPT Script
    )
{
    PYS_SCRIPT_LINE CurrentLine;
    PYS_CALL_STACK StackLocation;
    PYORI_LIST_ENTRY NextEntry;
    BOOL CallStackFound;

    NextEntry = YoriLibGetNextListEntry(&Script->LineLinks, NULL);
    while(NextEntry != NULL) {
        CurrentLine = CONTAINING_RECORD(NextEntry, YS_SCRIPT_LINE, LineLinks);
        NextEntry = YoriLibGetNextListEntry(&Script->LineLinks, NextEntry);

        YoriLibFreeStringContents(&CurrentLine->LineContents);
        YoriLibFree(CurrentLine);
    }

    CallStackFound = FALSE;

    NextEntry = YoriLibGetNextListEntry(&Script->CallStackLinks, NULL);
    while(NextEntry != NULL) {
        StackLocation = CONTAINING_RECORD(NextEntry, YS_CALL_STACK, StackLinks);
        NextEntry = YoriLibGetNextListEntry(&Script->CallStackLinks, NextEntry);

        YsFreeCallStack(StackLocation);
        CallStackFound = TRUE;
    }

    YoriLibFreeStringContents(&Script->FileName);
}


/**
 Load a script from an incoming stream.

 @param Handle The handle to the stream that contains the script.

 @param Script On successful completion, populated with the contents of the
        script.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YsLoadScript(
    __in HANDLE Handle,
    __out PYS_SCRIPT Script
    )
{
    BOOL Result = TRUE;

    YoriLibInitializeListHead(&Script->LineLinks);
    YoriLibInitializeListHead(&Script->CallStackLinks);

    if (!YsLoadLines(Handle, &Script->LineLinks)) {
        Result = FALSE;
    }

    if (Result == FALSE) {
        YsFreeScript(Script);
    }
    return Result;
}

/**
 Execute a script.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_YS(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    HANDLE FileHandle;
    BOOLEAN ArgumentUnderstood;
    YORI_STRING FileName;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YS_SCRIPT Script;
    YORI_STRING Arg;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                YsHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ys: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &FileName)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ys: getfullpathname of %y failed: %s"), &ArgV[StartArg], ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return EXIT_FAILURE;
    }

    FileHandle = CreateFile(FileName.StartOfString,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ys: could not open %y\n"), &FileName);
        YoriLibFreeStringContents(&FileName);
        return EXIT_FAILURE;
    }

    if (!YoriCallSetUnloadRoutine(YoriLibLineReadCleanupCache)) {
        YoriLibFreeStringContents(&FileName);
        CloseHandle(FileHandle);
        return EXIT_FAILURE;
    }

    if (!YsLoadScript(FileHandle, &Script)) {
        YoriLibFreeStringContents(&FileName);
        CloseHandle(FileHandle);
        return EXIT_FAILURE;
    }

    memcpy(&Script.FileName, &FileName, sizeof(YORI_STRING));

    CloseHandle(FileHandle);

    Script.GlobalArgContext.ShiftCount = StartArg;
    Script.GlobalArgContext.ArgC = ArgC;
    Script.GlobalArgContext.ArgV = ArgV;

    Script.ArgContext = &Script.GlobalArgContext;

    if (!YsExecuteScript(&Script)) {
        YsFreeScript(&Script);
        return EXIT_FAILURE;
    }

    YsFreeScript(&Script);

    return YoriCallGetErrorLevel();
}

// vim:sw=4:ts=4:et:
