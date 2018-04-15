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
        ;

/**
 Display usage text to the user.
 */
BOOL
YoriShHelp()
{
    YORI_STRING License;
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Yori %i.%i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibMitLicenseText(_T("2017-2018"), &License);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &License);
    YoriLibFreeStringContents(&License);
    return TRUE;
}

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

    if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("PATHEXT"), NULL, 0) == 0) {
        SetEnvironmentVariable(_T("PATHEXT"), _T(".YS1;.COM;.EXE;.CMD;.BAT"));
    } else {
        YORI_STRING NewExt;
        YoriLibConstantString(&NewExt, _T(".YS1"));
        YoriLibAddEnvironmentComponent(_T("PATHEXT"), &NewExt, TRUE);
    }

    YoriLibCancelEnable();
    YoriLibCancelIgnore();

    YoriShAddAliasLiteral(_T("cd"),       _T("chdir $*$"), TRUE);
    YoriShAddAliasLiteral(_T("clip"),     _T("yclip $*$"), TRUE);
    YoriShAddAliasLiteral(_T("cls"),      _T("ycls $*$"), TRUE);
    YoriShAddAliasLiteral(_T("copy"),     _T("ycopy $*$"), TRUE);
    YoriShAddAliasLiteral(_T("cut"),      _T("ycut $*$"), TRUE);
    YoriShAddAliasLiteral(_T("date"),     _T("ydate $*$"), TRUE);
    YoriShAddAliasLiteral(_T("del"),      _T("yerase $*$"), TRUE);
    YoriShAddAliasLiteral(_T("dir"),      _T("ydir $*$"), TRUE);
    YoriShAddAliasLiteral(_T("echo"),     _T("yecho $*$"), TRUE);
    YoriShAddAliasLiteral(_T("erase"),    _T("yerase $*$"), TRUE);
    YoriShAddAliasLiteral(_T("expr"),     _T("yexpr $*$"), TRUE);
    YoriShAddAliasLiteral(_T("for"),      _T("yfor $*$"), TRUE);
    YoriShAddAliasLiteral(_T("head"),     _T("ytype -h $*$"), TRUE);
    YoriShAddAliasLiteral(_T("help"),     _T("yhelp -h $*$"), TRUE);
    YoriShAddAliasLiteral(_T("htmlclip"), _T("yclip -h $*$"), TRUE);
    YoriShAddAliasLiteral(_T("md"),       _T("ymkdir $*$"), TRUE);
    YoriShAddAliasLiteral(_T("mkdir"),    _T("ymkdir $*$"), TRUE);
    YoriShAddAliasLiteral(_T("mklink"),   _T("ymklink $*$"), TRUE);
    YoriShAddAliasLiteral(_T("move"),     _T("ymove $*$"), TRUE);
    YoriShAddAliasLiteral(_T("paste"),    _T("yclip -p $*$"), TRUE);
    YoriShAddAliasLiteral(_T("path"),     _T("ypath $*$"), TRUE);
    YoriShAddAliasLiteral(_T("pause"),    _T("ypause $*$"), TRUE);
    YoriShAddAliasLiteral(_T("pwd"),      _T("ypath . $*$"), TRUE);
    YoriShAddAliasLiteral(_T("rd"),       _T("yrmdir $*$"), TRUE);
    YoriShAddAliasLiteral(_T("ren"),      _T("ymove $*$"), TRUE);
    YoriShAddAliasLiteral(_T("rename"),   _T("ymove $*$"), TRUE);
    YoriShAddAliasLiteral(_T("rmdir"),    _T("yrmdir $*$"), TRUE);
    YoriShAddAliasLiteral(_T("start"),    _T("ystart $*$"), TRUE);
    YoriShAddAliasLiteral(_T("split"),    _T("ysplit $*$"), TRUE);
    YoriShAddAliasLiteral(_T("time"),     _T("ydate -t $*$"), TRUE);
    YoriShAddAliasLiteral(_T("title"),    _T("ytitle $*$"), TRUE);
    YoriShAddAliasLiteral(_T("type"),     _T("ytype $*$"), TRUE);
    YoriShAddAliasLiteral(_T("vol"),      _T("yvol $*$"), TRUE);

    YoriShAddAliasLiteral(_T("?"),        _T("yexpr $*$"), TRUE);

    AliasName[1] = ':';
    AliasName[2] = '\0';

    YoriLibSPrintf(AliasValue, _T("chdir a:"));

    for (Letter = 'A'; Letter <= 'Z'; Letter++) {
        AliasName[0] = Letter;
        AliasValue[6] = Letter;

        YoriShAddAliasLiteral(AliasName, AliasValue, TRUE);
    }

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
 Reset the console after one process has finished in preparation for the next
 command.
 */
VOID
YoriShPostCommand()
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("GetConsoleScreenBufferInfo failed with %i\n"), GetLastError());
    }
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%c[0m"), 27);
    if (ScreenInfo.dwCursorPosition.X != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
    }

    YoriLibCancelEnable();
    YoriLibCancelIgnore();
    YoriLibCancelReset();
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);
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
    DWORD ExitCode;
    BOOL TerminateApp = FALSE;

    YoriShInit();
    if (YoriShParseArgs(ArgC, ArgV, &TerminateApp, &ExitCode)) {
        if (TerminateApp) {
            YoriShClearAllHistory();
            YoriShClearAllAliases();
            return ExitCode;
        }
    }

    YoriShLoadHistoryFromFile();

    while(TRUE) {

        YoriShPostCommand();
        YoriShScanJobsReportCompletion();
        YoriShScanProcessBuffersForTeardown();
        if (g_ExitProcess) {
            break;
        }
        YoriShDisplayPrompt();
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
    YoriShClearAllHistory();
    YoriShClearAllAliases();
    YoriShBuiltinUnregisterAll();

    return g_ExitProcessExitCode;
}

// vim:sw=4:ts=4:et:
