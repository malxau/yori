/**
 * @file sh/main.c
 *
 * Yori shell entrypoint
 *
 * Copyright (c) 2017-2025 Malcolm J. Smith
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
        "   -k <cmd>       Execute command and continue as an interactive shell\n"
        "   -nouser        Do not execute per-user AutoInit scripts\n";

/**
 Display usage text to the user.
 */
BOOL
YoriShHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Yori %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 A callback function for every file found in the YoriInit.d directory.

 @param Filename Pointer to the fully qualified file name to execute.

 @param FileInfo Pointer to information about the file.  Ignored in this
        function.

 @param Depth The recursion depth.  Ignored in this function.

 @param Context Pointer to context.  Ignored in this function.

 @return TRUE to continue enumerating, FALSE to terminate.
 */
BOOL
YoriShExecuteYoriInit(
    __in PYORI_STRING Filename,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING InitNameWithQuotes;
    LPTSTR szExt;
    YORI_STRING UnescapedPath;
    PYORI_STRING NameToUse;

    UNREFERENCED_PARAMETER(FileInfo);
    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(Context);

    YoriLibInitEmptyString(&UnescapedPath);
    NameToUse = Filename;
    szExt = YoriLibFindRightMostCharacter(Filename, '.');
    if (szExt != NULL) {
        YORI_STRING YsExt;

        YoriLibInitEmptyString(&YsExt);
        YsExt.StartOfString = szExt;
        YsExt.LengthInChars = Filename->LengthInChars - (YORI_ALLOC_SIZE_T)(szExt - Filename->StartOfString);
        if (YoriLibCompareStringLitIns(&YsExt, _T(".cmd")) == 0 ||
            YoriLibCompareStringLitIns(&YsExt, _T(".bat")) == 0) {

            if (YoriLibUnescapePath(Filename, &UnescapedPath)) {
                NameToUse = &UnescapedPath;
            }
        }
    }

    YoriLibInitEmptyString(&InitNameWithQuotes);
    YoriLibYPrintf(&InitNameWithQuotes, _T("\"%y\""), NameToUse);
    if (InitNameWithQuotes.LengthInChars > 0) {
        YoriShExecuteExpression(&InitNameWithQuotes);
    }
    YoriLibFreeStringContents(&InitNameWithQuotes);
    YoriLibFreeStringContents(&UnescapedPath);
    return TRUE;
}

/**
 Initialize the console and populate the shell's environment with default
 values.
 */
BOOL
YoriShInit(VOID)
{
    TCHAR Letter;
    TCHAR AliasName[3];
    TCHAR AliasValue[16];
    YORI_SH_BUILTIN_NAME_MAPPING CONST *BuiltinNameMapping = YoriShBuiltins;

    //
    //  Attempt to enable backup privilege so an administrator can access more
    //  objects successfully.
    //

    YoriLibEnableBackupPrivilege();

    //
    //  Translate the constant builtin function mapping into dynamic function
    //  mappings.
    //

    while (BuiltinNameMapping->CommandName != NULL) {
        YORI_STRING YsCommandName;

        YoriLibConstantString(&YsCommandName, BuiltinNameMapping->CommandName);
        if (!YoriLibShBuiltinRegister(&YsCommandName, BuiltinNameMapping->BuiltinFn)) {
            return FALSE;
        }
        BuiltinNameMapping++;
    }

    //
    //  If we don't have a prompt defined, set a default.  If outputting to
    //  the console directly, use VT color; otherwise, default to monochrome.
    //

    if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIPROMPT"), NULL, 0, NULL) == 0) {
        DWORD ConsoleMode;
        if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &ConsoleMode)) {
            SetEnvironmentVariable(_T("YORIPROMPT"), _T("$E$[35;1m$P$$E$[0m$G_OR_ADMIN_G$"));
        } else {
            SetEnvironmentVariable(_T("YORIPROMPT"), _T("$P$$G_OR_ADMIN_G$"));
        }
    }

    //
    //  If we don't have defined break characters, set them to the default.
    //  This allows the user to see the current set and manipulate them.
    //

    if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIQUICKEDITBREAKCHARS"), NULL, 0, NULL) == 0) {
        YORI_STRING BreakChars;
        YORI_STRING ExpandedBreakChars;
        YoriLibGetSelectionDoubleClickBreakChars(&BreakChars);
        if (YoriLibAllocateString(&ExpandedBreakChars, BreakChars.LengthInChars * 7)) {
            YORI_ALLOC_SIZE_T ReadIndex;
            YORI_ALLOC_SIZE_T WriteIndex;
            YORI_STRING Substring;

            WriteIndex = 0;
            YoriLibInitEmptyString(&Substring);
            for (ReadIndex = 0; ReadIndex < BreakChars.LengthInChars; ReadIndex++) {
                Substring.StartOfString = &ExpandedBreakChars.StartOfString[WriteIndex];
                Substring.LengthAllocated = ExpandedBreakChars.LengthAllocated - WriteIndex;
                if (BreakChars.StartOfString[ReadIndex] <= 0xFF) {
                    Substring.LengthInChars = YoriLibSPrintf(Substring.StartOfString, _T("0x%02x,"), BreakChars.StartOfString[ReadIndex]);
                } else {
                    Substring.LengthInChars = YoriLibSPrintf(Substring.StartOfString, _T("0x%04x,"), BreakChars.StartOfString[ReadIndex]);
                }

                WriteIndex = WriteIndex + Substring.LengthInChars;
            }

            if (WriteIndex > 0) {
                WriteIndex--;
                ExpandedBreakChars.StartOfString[WriteIndex] = '\0';
            }
            SetEnvironmentVariable(_T("YORIQUICKEDITBREAKCHARS"), ExpandedBreakChars.StartOfString);
            YoriLibFreeStringContents(&ExpandedBreakChars);
        }
        YoriLibFreeStringContents(&BreakChars);
    }

    //
    //  If we're running Yori and don't have a YORISPEC, assume this is the
    //  path to the shell the user wants to keep using.
    //

    if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORISPEC"), NULL, 0, NULL) == 0) {
        YORI_STRING ModuleName;

        //
        //  Unlike most other Win32 APIs, this one has no way to indicate
        //  how much space it needs.  We can be wasteful here though, since
        //  it'll be freed immediately.
        //

        if (!YoriLibAllocateString(&ModuleName, 32768)) {
            return FALSE;
        }

        ModuleName.LengthInChars = (YORI_ALLOC_SIZE_T)GetModuleFileName(NULL, ModuleName.StartOfString, ModuleName.LengthAllocated);
        if (ModuleName.LengthInChars > 0 && ModuleName.LengthInChars < ModuleName.LengthAllocated) {
            SetEnvironmentVariable(_T("YORISPEC"), ModuleName.StartOfString);
            while (ModuleName.LengthInChars > 0) {
                ModuleName.LengthInChars--;
                if (YoriLibIsSep(ModuleName.StartOfString[ModuleName.LengthInChars])) {

                    YoriLibAddEnvComponent(_T("PATH"), &ModuleName, TRUE);
                    break;
                }
            }
        }

        if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORICOMPLETEPATH"), NULL, 0, NULL) == 0) {
            YORI_STRING CompletePath;

            if (YoriLibAllocateString(&CompletePath, ModuleName.LengthInChars + sizeof("\\completion"))) {
                CompletePath.LengthInChars = YoriLibSPrintf(CompletePath.StartOfString, _T("%y\\completion"), &ModuleName);
                YoriLibAddEnvComponent(_T("YORICOMPLETEPATH"), &CompletePath, FALSE);

                //
                //  Convert "completion" into "complete" so there's an 8.3
                //  compliant name in the search path
                //

                CompletePath.LengthInChars = (YORI_ALLOC_SIZE_T)(CompletePath.LengthInChars - 2);
                CompletePath.StartOfString[CompletePath.LengthInChars - 1] = 'e';
                CompletePath.StartOfString[CompletePath.LengthInChars] = '\0';
                YoriLibAddEnvComponent(_T("YORICOMPLETEPATH"), &CompletePath, FALSE);
                YoriLibFreeStringContents(&CompletePath);
            }
        }

        YoriLibFreeStringContents(&ModuleName);
    }

    //
    //  Add .YS1 to PATHEXT if it's not there already.
    //

    if (YoriShGetEnvironmentVariableWithoutSubstitution(_T("PATHEXT"), NULL, 0, NULL) == 0) {
        SetEnvironmentVariable(_T("PATHEXT"), _T(".YS1;.COM;.EXE;.CMD;.BAT"));
    } else {
        YORI_STRING NewExt;
        YoriLibConstantString(&NewExt, _T(".YS1"));
        YoriLibAddEnvComponent(_T("PATHEXT"), &NewExt, TRUE);
    }

    YoriLibCancelEnable(TRUE);

    //
    //  Register any builtin aliases, including drive letter colon commands.
    //

    YoriShRegisterDefaultAliases();

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
 Execute any system or user init scripts.

 @param IgnoreUserScripts If TRUE, system scripts are executed but user
        scripts are not.  This is useful to ensure that a script executes
        consistently in any user context.

 @return TRUE to indicate success.
 */
BOOL
YoriShExecuteInitScripts(
    __in BOOLEAN IgnoreUserScripts
    )
{
    YORI_STRING RelativeYoriInitName;

    //
    //  Execute all system YoriInit scripts.
    //

    YoriLibConstantString(&RelativeYoriInitName, _T("~AppDir\\YoriInit.d\\*"));
    YoriLibForEachFile(&RelativeYoriInitName, YORILIB_FILEENUM_RETURN_FILES, 0, YoriShExecuteYoriInit, NULL, NULL);
    YoriLibConstantString(&RelativeYoriInitName, _T("~AppDir\\YoriInit*"));
    YoriLibForEachFile(&RelativeYoriInitName, YORILIB_FILEENUM_RETURN_FILES, 0, YoriShExecuteYoriInit, NULL, NULL);

    //
    //  Execute all user YoriInit scripts.
    //

    if (!IgnoreUserScripts) {
        YoriLibConstantString(&RelativeYoriInitName, _T("~\\YoriInit.d\\*"));
        YoriLibForEachFile(&RelativeYoriInitName, YORILIB_FILEENUM_RETURN_FILES, 0, YoriShExecuteYoriInit, NULL, NULL);
        YoriLibConstantString(&RelativeYoriInitName, _T("~\\YoriInit*"));
        YoriLibForEachFile(&RelativeYoriInitName, YORILIB_FILEENUM_RETURN_FILES, 0, YoriShExecuteYoriInit, NULL, NULL);
    }

    //
    //  Reload any state next time it's requested.
    //

    YoriShGlobal.EnvironmentGeneration++;

    return TRUE;
}

/**
 If Yori is running on NT 4.0 and was not launched from an existing console,
 set the console window icon to the first icon group in the executable. This
 works around NT 4.0 not utilizing the correctly sized icon in the console
 title bar.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShFixWindowIcon(VOID)
{
    DWORD OsVerMajor;
    DWORD OsVerMinor;
    DWORD OsBuildNumber;
    HWND ConsoleWindow;
    DWORD CurrentProcessId;
    DWORD WindowProcessId;
    HMODULE Module;
    HICON LargeIcon;
    HICON SmallIcon;

    YoriLibGetOsVersion(&OsVerMajor, &OsVerMinor, &OsBuildNumber);
    if (OsVerMajor != 4) {
        return TRUE;
    }

    YoriLibLoadUser32Functions();
    if (DllUser32.pFindWindowExW == NULL ||
        DllUser32.pGetSystemMetrics == NULL ||
        DllUser32.pGetWindowThreadProcessId == NULL ||
        DllUser32.pLoadImageW == NULL ||
        DllUser32.pSendMessageW == NULL) {

        return FALSE;
    }

    CurrentProcessId = GetCurrentProcessId();

    //
    //  Loop through all the console windows in the system.  If one of them
    //  hosts this process, break out and operate on that window.  If none
    //  do, run to completion and exit this function.
    //
    //  Frustratingly, EnumThreadWindows should allow this to be done
    //  without a usermode loop, but it doesn't think these windows are
    //  owned by this thread.
    //

    ConsoleWindow = DllUser32.pFindWindowExW(NULL, NULL, _T("ConsoleWindowClass"), NULL);
    while (ConsoleWindow != NULL) {
        DllUser32.pGetWindowThreadProcessId(ConsoleWindow, &WindowProcessId);
        if (WindowProcessId == CurrentProcessId) {
            break;
        }
        ConsoleWindow = DllUser32.pFindWindowExW(NULL, ConsoleWindow, _T("ConsoleWindowClass"), NULL);
    }

    if (ConsoleWindow == NULL) {
        return FALSE;
    }

    //
    //  Load the application icon and apply it to the window.
    //

    Module = GetModuleHandle(NULL);

    LargeIcon = DllUser32.pLoadImageW(Module,
                                      MAKEINTRESOURCE(YORI_ICON_APPLICATION),
                                      IMAGE_ICON,
                                      DllUser32.pGetSystemMetrics(SM_CXICON),
                                      DllUser32.pGetSystemMetrics(SM_CYICON),
                                      0);
    SmallIcon = DllUser32.pLoadImageW(Module,
                                      MAKEINTRESOURCE(YORI_ICON_APPLICATION),
                                      IMAGE_ICON,
                                      DllUser32.pGetSystemMetrics(SM_CXSMICON),
                                      DllUser32.pGetSystemMetrics(SM_CYSMICON),
                                      0);
    if (SmallIcon != NULL) {
        DllUser32.pSendMessageW(ConsoleWindow, WM_SETICON, ICON_SMALL, (LPARAM)SmallIcon);
    }
    if (LargeIcon != NULL) {
        DllUser32.pSendMessageW(ConsoleWindow, WM_SETICON, ICON_BIG, (LPARAM)LargeIcon);
    }

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
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[],
    __out PBOOLEAN TerminateApp,
    __out PDWORD ExitCode
    )
{
    BOOL ArgumentUnderstood;
    YORI_ALLOC_SIZE_T StartArgToExec = 0;
    YORI_ALLOC_SIZE_T i;
    YORI_STRING Arg;
    BOOLEAN ExecuteStartupScripts = TRUE;
    BOOLEAN IgnoreUserScripts = FALSE;
    BOOLEAN Interactive = TRUE;
    BOOLEAN IgnoreInteractive = FALSE;
    BOOLEAN FixWindowIcon = TRUE;

    *TerminateApp = FALSE;
    *ExitCode = 0;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                YoriShHelp();
                *TerminateApp = TRUE;
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2025"));
                *TerminateApp = TRUE;
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    Interactive = FALSE;
                    *TerminateApp = TRUE;
                    StartArgToExec = i + 1;
                    ArgumentUnderstood = TRUE;
                    break;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("k")) == 0) {
                if (ArgC > i + 1) {
                    *TerminateApp = FALSE;
                    StartArgToExec = i + 1;
                    ArgumentUnderstood = TRUE;
                    break;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("nouser")) == 0) {
                IgnoreUserScripts = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("restart")) == 0) {
                if (ArgC > i + 1) {
                    YoriShLoadSavedRestartState(&ArgV[i + 1]);
                    YoriShDiscardSavedRestartState(&ArgV[i + 1]);
                    i++;
                    ExecuteStartupScripts = FALSE;
                    ArgumentUnderstood = TRUE;
                    break;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("ss")) == 0) {
                if (ArgC > i + 1) {
                    Interactive = FALSE;
                    IgnoreInteractive = TRUE;
                    YoriShGlobal.RecursionDepth++;
                    YoriShGlobal.SubShell = TRUE;
                    ExecuteStartupScripts = FALSE;
                    FixWindowIcon = FALSE;
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

    if (Interactive) {
        SetEnvironmentVariable(_T("YORIINTERACTIVE"), _T("1"));
    } else if (!IgnoreInteractive) {
        if (GetEnvironmentVariable(_T("YORIINTERACTIVE"), NULL, 0) == 0) {
            SetEnvironmentVariable(_T("YORIINTERACTIVE"), _T("0"));
        }
    }

    if (FixWindowIcon) {
        YoriShFixWindowIcon();
    }

    if (ExecuteStartupScripts) {
        YoriShExecuteInitScripts(IgnoreUserScripts);
    }

    if (StartArgToExec > 0) {
        YORI_STRING YsCmdToExec;

        if (YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArgToExec, &ArgV[StartArgToExec], TRUE, TRUE, &YsCmdToExec)) {
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

#if YORI_BUILD_ID
/**
 The number of days before suggesting the user upgrade on a testing build.
 */
#define YORI_SH_DAYS_BEFORE_WARNING (40)
#else
/**
 The number of days before suggesting the user upgrade on a release build.
 */
#define YORI_SH_DAYS_BEFORE_WARNING (180)
#endif

/**
 If the user hasn't suppressed warning displays, display warnings for the age
 of the program and suboptimal architecture.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShDisplayWarnings(VOID)
{
    YORI_ALLOC_SIZE_T EnvVarLength;
    YORI_STRING ModuleName;
    BOOLEAN WarningDisplayed;

    EnvVarLength = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORINOWARNINGS"), NULL, 0, NULL);
    if (EnvVarLength > 0) {
        YORI_STRING NoWarningsVar;
        if (!YoriLibAllocateString(&NoWarningsVar, EnvVarLength + 1)) {
            return FALSE;
        }

        NoWarningsVar.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORINOWARNINGS"), NoWarningsVar.StartOfString, NoWarningsVar.LengthAllocated, NULL);
        if (EnvVarLength < NoWarningsVar.LengthAllocated &&
            YoriLibCompareStringLit(&NoWarningsVar, _T("1")) == 0) {

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

    WarningDisplayed = FALSE;

    ModuleName.LengthInChars = (YORI_ALLOC_SIZE_T)GetModuleFileName(NULL, ModuleName.StartOfString, ModuleName.LengthAllocated);
    if (ModuleName.LengthInChars > 0 && ModuleName.LengthInChars < ModuleName.LengthAllocated) {
        HANDLE ExeHandle;

        ExeHandle = CreateFile(ModuleName.StartOfString,
                               FILE_READ_ATTRIBUTES,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                               NULL);
        if (ExeHandle != INVALID_HANDLE_VALUE) {
            FILETIME CreationTime;
            FILETIME AccessTime;
            FILETIME WriteTime;
            LARGE_INTEGER liWriteTime;
            LARGE_INTEGER liNow;

            GetFileTime(ExeHandle, &CreationTime, &AccessTime, &WriteTime);
            CloseHandle(ExeHandle);

            liNow.QuadPart = YoriLibGetSystemTimeAsInteger();
            liWriteTime.LowPart = WriteTime.dwLowDateTime;
            liWriteTime.HighPart = WriteTime.dwHighDateTime;

            liNow.QuadPart = YoriLibDivide32((DWORDLONG)liNow.QuadPart, 10 * 1000 * 1000);
            liNow.QuadPart = YoriLibDivide32((DWORDLONG)liNow.QuadPart, 60 * 60 * 24);
            liWriteTime.QuadPart = YoriLibDivide32((DWORDLONG)liWriteTime.QuadPart, 10 * 1000 * 1000);
            liWriteTime.QuadPart = YoriLibDivide32((DWORDLONG)liWriteTime.QuadPart, 60 * 60 * 24);

            if (liNow.QuadPart > liWriteTime.QuadPart &&
                liWriteTime.QuadPart + YORI_SH_DAYS_BEFORE_WARNING < liNow.QuadPart) {

                DWORD DaysOld;
                DWORD UnitToDisplay;
                LPTSTR UnitLabel;

                DaysOld = (DWORD)(liNow.QuadPart - liWriteTime.QuadPart);
                if (DaysOld > 2 * 365) {
                    UnitToDisplay = DaysOld / 365;
                    UnitLabel = _T("years");
                } else if (DaysOld > 3 * 30) {
                    UnitToDisplay = DaysOld / 30;
                    UnitLabel = _T("months");
                } else {
                    UnitToDisplay = DaysOld;
                    UnitLabel = _T("days");
                }

                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                              _T("Warning: This build of Yori is %i %s old.  Run ypm -u to upgrade.\n"),
                              UnitToDisplay,
                              UnitLabel);
                WarningDisplayed = TRUE;
            }
        }
    }

    if (DllKernel32.pIsWow64Process != NULL) {
        BOOL IsWow = FALSE;
        if (DllKernel32.pIsWow64Process(GetCurrentProcess(), &IsWow) && IsWow) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Warning: This a 32 bit version of Yori on a 64 bit system.\n   Run 'ypm -a amd64 -u' to switch to the 64 bit version.\n"));
            WarningDisplayed = TRUE;
        }
    }

    YoriLibFreeStringContents(&ModuleName);
    if (WarningDisplayed) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  (Set YORINOWARNINGS=1 to suppress these messages)\n"));
    }
    return TRUE;
}


/**
 Reset the console after one process has finished.
 */
VOID
YoriShPostCommand(VOID)
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE ConsoleHandle;
    BOOL ConsoleMode;

    //
    //  This will only do anything if this process has already set the state
    //  previously.
    //

    if (YoriShGlobal.ErrorLevel == 0) {
        YoriShSetWindowState(YORI_SH_TASK_SUCCESS);
    } else {
        YoriShSetWindowState(YORI_SH_TASK_FAILED);
    }

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    ConsoleMode = GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenInfo);
    if (ConsoleMode)  {

        if (YoriShGlobal.OutputSupportsVt) {
            SetConsoleMode(ConsoleHandle, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        } else {
            SetConsoleMode(ConsoleHandle, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
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
 Load the current directory.  If the current directory has changed and the
 terminal supports VT escapes, indicate to the terminal what the current
 directory now is.  This allows the terminal to inherit and apply that
 state into other shells.
 */
VOID
YoriShCaptureCurrentDirectoryAndInformTerminal(VOID)
{
    HANDLE ConsoleHandle;
    UCHAR NextCurrentDirectoryIndex;
    YORI_ALLOC_SIZE_T CurrentDirectoryLength;
    PYORI_STRING NextCurrentDirectory;

    NextCurrentDirectoryIndex = (UCHAR)((YoriShGlobal.ActiveCurrentDirectory + 1) % (sizeof(YoriShGlobal.CurrentDirectoryBuffers)/sizeof(YoriShGlobal.CurrentDirectoryBuffers[0])));

    NextCurrentDirectory = &YoriShGlobal.CurrentDirectoryBuffers[NextCurrentDirectoryIndex];

    CurrentDirectoryLength = (YORI_ALLOC_SIZE_T)GetCurrentDirectory(0, NULL);
    if (CurrentDirectoryLength > NextCurrentDirectory->LengthAllocated) {
        if (!YoriLibReallocStringNoContents(NextCurrentDirectory, CurrentDirectoryLength + 0x40)) {
            return;
        }
    }

    NextCurrentDirectory->LengthInChars = (YORI_ALLOC_SIZE_T)GetCurrentDirectory(NextCurrentDirectory->LengthAllocated, NextCurrentDirectory->StartOfString);

    if (!YoriShGlobal.OutputSupportsVtDetermined) {
        ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

        //
        //  Old versions will fail and ignore any call that contains a flag
        //  they don't understand, so attempt a lowest common denominator
        //  setting and try to upgrade it, which might fail.
        //

        SetConsoleMode(ConsoleHandle, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
        if (SetConsoleMode(ConsoleHandle, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            YoriShGlobal.OutputSupportsVt = TRUE;
        }
        YoriShGlobal.OutputSupportsVtDetermined = TRUE;
    } else if (YoriShGlobal.OutputSupportsVt) {
        ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleMode(ConsoleHandle, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    //
    //  Don't output this on Nano which will just dump it on the console.
    //  This should probably apply to all builds <= DecentLevelOfVtSupport,
    //  but OpenConsole supports running on 8.1, so testing for OS versions
    //  doesn't tell us much about the terminal's capabilities.
    //

    if (YoriShGlobal.OutputSupportsVt && !YoriLibIsNanoServer()) {
        if (YoriLibCompareString(&YoriShGlobal.CurrentDirectoryBuffers[YoriShGlobal.ActiveCurrentDirectory], NextCurrentDirectory) != 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT | YORI_LIB_OUTPUT_PASSTHROUGH_VT, _T("\x1b]9;9;\"%y\"\x1b\\"), NextCurrentDirectory);
        }
    }

    YoriShGlobal.ActiveCurrentDirectory = NextCurrentDirectoryIndex;
}

/**
 Prepare the console for entry of the next command.

 @param EnableVt If TRUE, VT processing should be enabled if the console
        supports it.  In general Yori leaves this enabled for the benefit of
        child processes, but it is disabled when displaying the prompt.  The
        prompt is written by the shell process, and depends on moving the
        cursor to the next line after the final cell in a line is written,
        which is not the behavior that Windows VT support provides.  Note
        this behavior isn't a problem for programs that continue to output -
        it's a problem for programs that output and then wait for input.
 */
VOID
YoriShPreCommand(
    __in BOOLEAN EnableVt
    )
{
    HANDLE ConsoleHandle;

    YoriShCleanupRestartSaveThreadIfCompleted();
    YoriLibCancelEnable(TRUE);
    YoriLibCancelReset();


    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (YoriShGlobal.OutputSupportsVt && EnableVt) {
        SetConsoleMode(ConsoleHandle, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    } else {
        SetConsoleMode(ConsoleHandle, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
    }
}

/**
 The entrypoint function for Yori.

 @param ArgC Count of the number of arguments.

 @param ArgV An array of arguments.

 @return Exit code for the application.
 */
DWORD
ymain (
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_STRING CurrentExpression;
    BOOLEAN TerminateApp = FALSE;

    YoriShInit();
    YoriShParseArgs(ArgC, ArgV, &TerminateApp, &YoriShGlobal.ExitProcessExitCode);

    if (!TerminateApp) {

        YoriShDisplayWarnings();
        YoriShLoadHistoryFromFile();

        while(TRUE) {

            YoriShPostCommand();
            YoriShScanJobsReportCompletion(FALSE);
            YoriLibShScanProcessBuffersForTeardown(FALSE);
            if (YoriShGlobal.ExitProcess) {
                break;
            }

            YoriShCaptureCurrentDirectoryAndInformTerminal();

            //
            //  Don't enable VT processing while displaying the prompt.  This
            //  behavior is subtly different in that when it displays in the
            //  final cell of a line it doesn't move the cursor to the next
            //  line.  For prompts this is broken because it leaves user
            //  input overwriting the final character of the prompt.
            //

            YoriShPreCommand(FALSE);
            YoriShDisplayPrompt();
            YoriShPreCommand(FALSE);

            if (!YoriShGetExpression(&CurrentExpression)) {
                break;
            }
            if (YoriShGlobal.ExitProcess) {
                break;
            }
            YoriShPreCommand(TRUE);
            YoriShExecPreCommandString();
            if (CurrentExpression.LengthInChars > 0) {
                YoriShExecuteExpression(&CurrentExpression);
            }
            YoriLibFreeStringContents(&CurrentExpression);
        }

        YoriShSaveHistoryToFile();
    }

    YoriLibShScanProcessBuffersForTeardown(TRUE);
    YoriShScanJobsReportCompletion(TRUE);
    YoriShClearAllHistory();
    YoriShClearAllAliases();
    YoriLibShBuiltinUnregisterAll();
    YoriShDiscardSavedRestartState(NULL);
    YoriShCleanupInputContext();
    YoriLibLineReadCleanupCache();
    YoriLibCleanupCurrentDirectory();
    YoriLibFreeStringContents(&YoriShGlobal.PreCmdVariable);
    YoriLibFreeStringContents(&YoriShGlobal.PostCmdVariable);
    YoriLibFreeStringContents(&YoriShGlobal.PromptVariable);
    YoriLibFreeStringContents(&YoriShGlobal.TitleVariable);
    YoriLibFreeStringContents(&YoriShGlobal.NextCommand);
    YoriLibFreeStringContents(&YoriShGlobal.YankBuffer);
    YoriLibFreeStringContents(&YoriShGlobal.CurrentDirectoryBuffers[0]);
    YoriLibFreeStringContents(&YoriShGlobal.CurrentDirectoryBuffers[1]);
    YoriLibEmptyProcessClipboard();

    return YoriShGlobal.ExitProcessExitCode;
}

// vim:sw=4:ts=4:et:
