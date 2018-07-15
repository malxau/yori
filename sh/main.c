/**
 * @file sh/main.c
 *
 * Yori shell entrypoint
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

DWORD g_ErrorLevel;

/**
 When set to TRUE, the process should end rather than seek another
 command.
 */
BOOL g_ExitProcess;

/**
 When g_ExitProcess is set to TRUE, this is set to the code that the shell
 should return as its exit code.
 */
DWORD g_ExitProcessExitCode;

/**
 A function pointer that can be used to enable or disable WOW64 redirection.
 */
typedef BOOL (WINAPI * DISABLE_WOW_REDIRECT_FN)(PVOID*);

/**
 Help text to display to the user.
 */
const
CHAR strHelpText[] =
        "\n"
        "Start a Yori shell instance.\n"
        "\n"
        "YORI [-license] [-c <cmd>] [-k <cmd>]\n"
        "\n"
        "   -license       Display license text\n"
        "   -c <cmd>       Execute command and terminate the shell\n"
        "   -k <cmd>       Execute command and continue as an interactive shell\n";

/**
 Display usage text to the user.
 */
BOOL
YoriShHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Yori %i.%i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 A structure defining an initial mapping of alias to value.
 */
typedef struct _YORI_SH_DEFAULT_ALIAS_ENTRY {

    /**
     The initial alias name.
     */
    LPTSTR Alias;

    /**
     The initial value.
     */
    LPTSTR Value;
} YORI_SH_DEFAULT_ALIAS_ENTRY, *PYORI_SH_DEFAULT_ALIAS_ENTRY;

/**
 A table of initial alias to value mappings to populate.
 */
YORI_SH_DEFAULT_ALIAS_ENTRY YoriShDefaultAliasEntries[] = {
    {_T("cd"),       _T("chdir $*$")},
    {_T("clip"),     _T("yclip $*$")},
    {_T("cls"),      _T("ycls $*$")},
    {_T("copy"),     _T("ycopy $*$")},
    {_T("cut"),      _T("ycut $*$")},
    {_T("date"),     _T("ydate $*$")},
    {_T("del"),      _T("yerase $*$")},
    {_T("dir"),      _T("ydir $*$")},
    {_T("echo"),     _T("yecho $*$")},
    {_T("erase"),    _T("yerase $*$")},
    {_T("expr"),     _T("yexpr $*$")},
    {_T("head"),     _T("ytype -h $*$")},
    {_T("help"),     _T("yhelp -h $*$")},
    {_T("htmlclip"), _T("yclip -h $*$")},
    {_T("md"),       _T("ymkdir $*$")},
    {_T("mkdir"),    _T("ymkdir $*$")},
    {_T("mklink"),   _T("ymklink $*$")},
    {_T("move"),     _T("ymove $*$")},
    {_T("paste"),    _T("yclip -p $*$")},
    {_T("path"),     _T("ypath $*$")},
    {_T("pause"),    _T("ypause $*$")},
    {_T("pwd"),      _T("ypath . $*$")},
    {_T("rd"),       _T("yrmdir $*$")},
    {_T("ren"),      _T("ymove $*$")},
    {_T("rename"),   _T("ymove $*$")},
    {_T("rmdir"),    _T("yrmdir $*$")},
    {_T("start"),    _T("ystart $*$")},
    {_T("split"),    _T("ysplit $*$")},
    {_T("time"),     _T("ydate -t $*$")},
    {_T("title"),    _T("ytitle $*$")},
    {_T("type"),     _T("ytype $*$")},
    {_T("vol"),      _T("yvol $*$")},
    {_T("?"),        _T("yexpr $*$")}
};

/**
 Initialize the console and populate the shell's environment with default
 values.
 */
BOOL
YoriShInit()
{
    TCHAR Letter;
    TCHAR AliasName[3];
    TCHAR AliasValue[16];
    DWORD Count;
    YORI_BUILTIN_NAME_MAPPING CONST *BuiltinNameMapping = YoriShBuiltins;

    //
    //  Translate the constant builtin function mapping into dynamic function
    //  mappings.
    //

    while (BuiltinNameMapping->CommandName != NULL) {
        YORI_STRING YsCommandName;

        YoriLibConstantString(&YsCommandName, BuiltinNameMapping->CommandName);
        if (!YoriShBuiltinRegister(&YsCommandName, BuiltinNameMapping->BuiltinFn)) {
            return FALSE;
        }
        BuiltinNameMapping++;
    }

    //
    //  If we don't have a prompt defined, set a default.
    //

    if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIPROMPT"), NULL, 0) == 0) {
        SetEnvironmentVariable(_T("YORIPROMPT"), _T("$E$[35;1m$P$$E$[0m$G$"));
    }

    //
    //  If we're running Yori and don't have a YORISPEC, assume this is the
    //  path to the shell the user wants to keep using.
    //

    if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORISPEC"), NULL, 0) == 0) {
        YORI_STRING ModuleName;

        //
        //  Unlike most other Win32 APIs, this one has no way to indicate
        //  how much space it needs.  We can be wasteful here though, since
        //  it'll be freed immediately.
        //

        if (!YoriLibAllocateString(&ModuleName, 32768)) {
            return FALSE;
        }

        ModuleName.LengthInChars = GetModuleFileName(NULL, ModuleName.StartOfString, ModuleName.LengthAllocated);
        if (ModuleName.LengthInChars > 0 && ModuleName.LengthInChars < ModuleName.LengthAllocated) {
            SetEnvironmentVariable(_T("YORISPEC"), ModuleName.StartOfString);
            while (ModuleName.LengthInChars > 0) {
                ModuleName.LengthInChars--;
                if (YoriLibIsSep(ModuleName.StartOfString[ModuleName.LengthInChars])) {

                    YoriLibAddEnvironmentComponent(_T("PATH"), &ModuleName, TRUE);
                    break;
                }
            }
        }

        if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORICOMPLETEPATH"), NULL, 0) == 0) {
            YORI_STRING CompletePath;

            if (YoriLibAllocateString(&CompletePath, ModuleName.LengthInChars + sizeof("\\completion"))) {
                CompletePath.LengthInChars = YoriLibSPrintf(CompletePath.StartOfString, _T("%y\\completion"), &ModuleName);
                YoriLibAddEnvironmentComponent(_T("YORICOMPLETEPATH"), &CompletePath, FALSE);
                YoriLibFreeStringContents(&CompletePath);
            }
        }

        YoriLibFreeStringContents(&ModuleName);
    }

    //
    //  Add .YS1 to PATHEXT if it's not there already.
    //

    if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("PATHEXT"), NULL, 0) == 0) {
        SetEnvironmentVariable(_T("PATHEXT"), _T(".YS1;.COM;.EXE;.CMD;.BAT"));
    } else {
        YORI_STRING NewExt;
        YoriLibConstantString(&NewExt, _T(".YS1"));
        YoriLibAddEnvironmentComponent(_T("PATHEXT"), &NewExt, TRUE);
    }

    YoriLibCancelEnable();
    YoriLibCancelIgnore();

    //
    //  Register any builtin aliases, including drive letter colon commands.
    //

    for (Count = 0; Count < sizeof(YoriShDefaultAliasEntries)/sizeof(YoriShDefaultAliasEntries[0]); Count++) {
        YoriShAddAliasLiteral(YoriShDefaultAliasEntries[Count].Alias, YoriShDefaultAliasEntries[Count].Value, TRUE);
    }

    AliasName[1] = ':';
    AliasName[2] = '\0';

    YoriLibSPrintf(AliasValue, _T("chdir a:"));

    for (Letter = 'A'; Letter <= 'Z'; Letter++) {
        AliasName[0] = Letter;
        AliasValue[6] = Letter;

        YoriShAddAliasLiteral(AliasName, AliasValue, TRUE);
    }

    //
    //  Load aliases registered with conhost.
    //

    YoriShLoadSystemAliases(TRUE);
    YoriShLoadSystemAliases(FALSE);

    return TRUE;
}

/**
 Parse the Yori command line and perform any requested actions.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @param TerminateApp On successful completion, set to TRUE if the shell should
        exit.

 @param ExitCode On successful completion, set to the exit code to return from
        the application.  Only meaningful if TerminateApp is set to TRUE.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShParseArgs(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[],
    __out PBOOL TerminateApp,
    __out PDWORD ExitCode
    )
{
    BOOL ArgumentUnderstood;
    DWORD StartArgToExec = 0;
    DWORD i;
    YORI_STRING Arg;

    *TerminateApp = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YoriShHelp();
                *TerminateApp = TRUE;
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                *TerminateApp = TRUE;
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    *TerminateApp = TRUE;
                    StartArgToExec = i + 1;
                    ArgumentUnderstood = TRUE;
                    break;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("k")) == 0) {
                if (ArgC > i + 1) {
                    *TerminateApp = FALSE;
                    StartArgToExec = i + 1;
                    ArgumentUnderstood = TRUE;
                    break;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("restart")) == 0) {
                if (ArgC > i + 1) {
                    YoriShLoadSavedRestartState(&ArgV[i + 1]);
                    YoriShDiscardSavedRestartState(&ArgV[i + 1]);
                    i++;
                    ArgumentUnderstood = TRUE;
                    break;
                }
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArgToExec > 0) {
        YORI_STRING YsCmdToExec;

        if (YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArgToExec, &ArgV[StartArgToExec], TRUE, &YsCmdToExec)) {
            if (YsCmdToExec.LengthInChars > 0) {
                *ExitCode = YoriShExecuteExpression(&YsCmdToExec);
            }
            YoriLibFreeStringContents(&YsCmdToExec);
        }
    }

    return TRUE;
}

/**
 Reset the console after one process has finished.
 */
VOID
YoriShPostCommand()
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE ConsoleHandle;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("GetConsoleScreenBufferInfo failed with %i\n"), GetLastError());
    }
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%c[0m"), 27);
    if (ScreenInfo.srWindow.Left > 0) {
        SHORT CharsToMoveLeft;
        CharsToMoveLeft = ScreenInfo.srWindow.Left;
        ScreenInfo.srWindow.Left = 0;
        ScreenInfo.srWindow.Right = (SHORT)(ScreenInfo.srWindow.Right - CharsToMoveLeft);
        SetConsoleWindowInfo(ConsoleHandle, TRUE, &ScreenInfo.srWindow);
    }
    if (ScreenInfo.dwCursorPosition.X != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
    }
}

/**
 Prepare the console for entry of the next command.
 */
VOID
YoriShPreCommand()
{
    YoriLibCancelEnable();
    YoriLibCancelIgnore();
    YoriLibCancelReset();
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

/**
 The entrypoint function for Yori.

 @param ArgC Count of the number of arguments.

 @param ArgV An array of arguments.

 @return Exit code for the application.
 */
DWORD
ymain (
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_STRING CurrentExpression;
    BOOL TerminateApp = FALSE;

    YoriShInit();
    YoriShParseArgs(ArgC, ArgV, &TerminateApp, &g_ExitProcessExitCode);

    if (!TerminateApp) {

        YoriShLoadHistoryFromFile();

        while(TRUE) {

            YoriShPostCommand();
            YoriShScanJobsReportCompletion(FALSE);
            YoriShScanProcessBuffersForTeardown(FALSE);
            if (g_ExitProcess) {
                break;
            }
            YoriShDisplayPrompt();
            YoriShPreCommand();
            if (!YoriShGetExpression(&CurrentExpression)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Failed to read expression!"));
                continue;
            }
            if (CurrentExpression.LengthInChars > 0) {
                YoriShExecuteExpression(&CurrentExpression);
            }
            YoriLibFreeStringContents(&CurrentExpression);
        }

        YoriShSaveHistoryToFile();
    }

    YoriShScanProcessBuffersForTeardown(TRUE);
    YoriShScanJobsReportCompletion(TRUE);
    YoriShClearAllHistory();
    YoriShClearAllAliases();
    YoriShBuiltinUnregisterAll();
    YoriShDiscardSavedRestartState(NULL);

    return g_ExitProcessExitCode;
}

// vim:sw=4:ts=4:et:
