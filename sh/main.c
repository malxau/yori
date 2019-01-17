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

/**
 Mutable state that is global across the shell process.
 */
YORI_SH_GLOBALS YoriShGlobal;

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
    YORI_SH_BUILTIN_NAME_MAPPING CONST *BuiltinNameMapping = YoriShBuiltins;

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
    //  If we don't have a prompt defined, set a default.  If outputting to
    //  the console directly, use VT color; otherwise, default to monochrome.
    //

    if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIPROMPT"), NULL, 0) == 0) {
        DWORD ConsoleMode;
        if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &ConsoleMode)) {
            SetEnvironmentVariable(_T("YORIPROMPT"), _T("$E$[35;1m$P$$E$[0m$G_OR_ADMIN_G$"));
        } else {
            SetEnvironmentVariable(_T("YORIPROMPT"), _T("$P$$G_OR_ADMIN_G$"));
        }
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

    for (Count = 0; Count < YoriShDefaultAliasEntriesCount(); Count++) {
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
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("ss")) == 0) {
                if (ArgC > i + 1) {
                    *TerminateApp = TRUE;
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
                if (YoriShExecuteExpression(&YsCmdToExec)) {
                    *ExitCode = YoriShGlobal.ErrorLevel;
                } else {
                    *ExitCode = EXIT_FAILURE;
                }
            }
            YoriLibFreeStringContents(&YsCmdToExec);
        }
    }

    return TRUE;
}

/**
 If the user hasn't suppressed warning displays, display warnings for the age
 of the program and suboptimal architecture.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShDisplayWarnings()
{
    DWORD EnvVarLength;
    YORI_STRING ModuleName;

    EnvVarLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORINOWARNINGS"), NULL, 0);
    if (EnvVarLength > 0) {
        YORI_STRING NoWarningsVar;
        if (!YoriLibAllocateString(&NoWarningsVar, EnvVarLength + 1)) {
            return FALSE;
        }

        NoWarningsVar.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORINOWARNINGS"), NoWarningsVar.StartOfString, NoWarningsVar.LengthAllocated);
        if (EnvVarLength < NoWarningsVar.LengthAllocated &&
            YoriLibCompareStringWithLiteral(&NoWarningsVar, _T("1")) == 0) {

            YoriLibFreeStringContents(&NoWarningsVar);
            return TRUE;
        }
        YoriLibFreeStringContents(&NoWarningsVar);
    }

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
        HANDLE ExeHandle;

        ExeHandle = CreateFile(ModuleName.StartOfString,
                               FILE_READ_ATTRIBUTES,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
        if (ExeHandle != INVALID_HANDLE_VALUE) {
            FILETIME CreationTime;
            FILETIME AccessTime;
            FILETIME WriteTime;
            SYSTEMTIME Now;
            FILETIME FtNow;
            LARGE_INTEGER liWriteTime;
            LARGE_INTEGER liNow;

            GetFileTime(ExeHandle, &CreationTime, &AccessTime, &WriteTime);
            GetSystemTime(&Now);
            SystemTimeToFileTime(&Now, &FtNow);
            liNow.LowPart = FtNow.dwLowDateTime;
            liNow.HighPart = FtNow.dwHighDateTime;
            liWriteTime.LowPart = WriteTime.dwLowDateTime;
            liWriteTime.HighPart = WriteTime.dwHighDateTime;

            liNow.QuadPart = liNow.QuadPart / (10 * 1000 * 1000);
            liNow.QuadPart = liNow.QuadPart / (60 * 60 * 24);
            liWriteTime.QuadPart = liWriteTime.QuadPart / (10 * 1000 * 1000);
            liWriteTime.QuadPart = liWriteTime.QuadPart / (60 * 60 * 24);

            if (liNow.QuadPart > liWriteTime.QuadPart &&
                liWriteTime.QuadPart + 120 < liNow.QuadPart) {

                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Warning: This build of Yori is %lli days old.  Run ypm -u to upgrade."), liNow.QuadPart - liWriteTime.QuadPart);
            }
        }
    }

    if (DllKernel32.pIsWow64Process != NULL) {
        BOOL IsWow = FALSE;
        if (DllKernel32.pIsWow64Process(GetCurrentProcess(), &IsWow) && IsWow) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Warning: This a 32 bit version of Yori on a 64 bit system.\n   Run 'ypm -a amd64 -u' to switch to the 64 bit version.\n"));
        }
    }

    YoriLibFreeStringContents(&ModuleName);
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
    BOOL ConsoleMode;

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    ConsoleMode = GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo);
    if (ConsoleMode)  {
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

    } else {

        //
        //  If output isn't to a console, we have no way to know if a
        //  newline is needed, so just output one unconditionally.  This
        //  is what CMD always does, ensuring that if you execute any
        //  command there's a blank line following.
        //

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
    YoriShParseArgs(ArgC, ArgV, &TerminateApp, &YoriShGlobal.ExitProcessExitCode);

    if (!TerminateApp) {

        YoriShDisplayWarnings();
        YoriShLoadHistoryFromFile();

        while(TRUE) {

            YoriShPostCommand();
            YoriShScanJobsReportCompletion(FALSE);
            YoriShScanProcessBuffersForTeardown(FALSE);
            if (YoriShGlobal.ExitProcess) {
                break;
            }
            YoriShPreCommand();
            YoriShDisplayPrompt();
            YoriShPreCommand();
            if (!YoriShGetExpression(&CurrentExpression)) {
                break;
            }
            if (YoriShGlobal.ExitProcess) {
                break;
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
    YoriShCleanupInputContext();

    return YoriShGlobal.ExitProcessExitCode;
}

// vim:sw=4:ts=4:et:
