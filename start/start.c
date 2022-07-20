/**
 * @file start/start.c
 *
 * Yori shell start process via shell tool
 *
 * Copyright (c) 2017-2021 Malcolm J. Smith
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

/**
 Help text to display to the user.
 */
const
CHAR strStartHelpText[] =
        "\n"
        "Ask the shell to open a file.\n"
        "\n"
        "START [-license] [-e] [-s:b|-s:h|-s:m] <file>\n"
        "\n"
        "   -e             Start elevated\n"
        "   -s:b           Start in the background\n"
        "   -s:h           Start hidden\n"
        "   -s:m           Start minimized\n";

/**
 Display usage text to the user.
 */
BOOL
StartHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Start %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strStartHelpText);
    return TRUE;
}

/**
 Try to launch a single program via CreateProcess.  This branch is only used
 on OS editions that do not support ShellExecute.

 @param ArgC Count of arguments.

 @param ArgV Array of arguments.

 @param ShowState The state of the window for the new program.

 @return TRUE to indicate success, FALSE on failure.
 */
BOOL
StartCreateProcess(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[],
    __in INT ShowState
    )
{
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    YORI_STRING CmdLine;
    DWORD CreationFlags;

    YoriLibInitEmptyString(&CmdLine);
    if (!YoriLibBuildCmdlineFromArgcArgv(ArgC, ArgV, TRUE, TRUE, &CmdLine)) {
        return FALSE;
    }
    ASSERT(YoriLibIsStringNullTerminated(&CmdLine));

    ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));

    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = (WORD)ShowState;

    CreationFlags = CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP | CREATE_DEFAULT_ERROR_MODE;

    if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, CreationFlags, NULL, NULL, &StartupInfo, &ProcessInfo)) {
        DWORD Err = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(Err);
        if (ErrText != NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("start: execution failed: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);

        }
        YoriLibFreeStringContents(&CmdLine);
        return FALSE;
    }

    YoriLibFreeStringContents(&CmdLine);

    if (ProcessInfo.hThread != NULL) {
        CloseHandle(ProcessInfo.hThread);
    }

    if (ProcessInfo.hProcess != NULL) {
        CloseHandle(ProcessInfo.hProcess);
    }

    return TRUE;
}

/**
 Try to launch a single program via ShellExecute rather than CreateProcess.

 @param ArgC Count of arguments.

 @param ArgV Array of arguments.

 @param ShowState The state of the window for the new program.

 @param Elevate TRUE to indicate the program should prompt for elevation.

 @return TRUE to indicate success, FALSE on failure.
 */
BOOL
StartShellExecute(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[],
    __in INT ShowState,
    __in BOOL Elevate
    )
{
    YORI_STRING Args;
    HINSTANCE hInst;
    LPTSTR ErrText = NULL;
    BOOL AllocatedError = FALSE;
    YORI_SHELLEXECUTEINFO sei;
    DWORD LastError;

    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_FLAG_NO_UI |
                SEE_MASK_NOZONECHECKS |
                SEE_MASK_UNICODE;

    ASSERT(YoriLibIsStringNullTerminated(&ArgV[0]));
    sei.lpFile = ArgV[0].StartOfString;

    YoriLibInitEmptyString(&Args);
    if (ArgC > 1) {
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - 1, &ArgV[1], TRUE, TRUE, &Args)) {
            return FALSE;
        }
        ASSERT(YoriLibIsStringNullTerminated(&Args));
    }

    sei.lpParameters = Args.StartOfString;
    sei.nShow = ShowState;

    LastError = ERROR_SUCCESS;

    if (DllShell32.pShellExecuteExW != NULL) {
        if (Elevate) {
            sei.lpVerb = _T("runas");
        }

        if (!DllShell32.pShellExecuteExW(&sei)) {
            LastError = GetLastError();
        } else {
            YoriLibFreeStringContents(&Args);
            return TRUE;
        }
    }

    if (LastError != ERROR_SUCCESS &&
        (Elevate || LastError != ERROR_CALL_NOT_IMPLEMENTED)) {

        YoriLibFreeStringContents(&Args);
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("start: execution failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    hInst = DllShell32.pShellExecuteW(NULL, NULL, sei.lpFile, sei.lpParameters, NULL, sei.nShow);

    YoriLibFreeStringContents(&Args);
    if ((DWORD_PTR)hInst >= 32) {
        return TRUE;
    }

    switch((DWORD_PTR)hInst) {
        case SE_ERR_ASSOCINCOMPLETE:
            ErrText = _T("The filename association is incomplete or invalid.\n");
            break;
        case SE_ERR_DDEBUSY:
            ErrText = _T("The DDE transaction could not be completed because other DDE transactions were being processed.\n");
            break;
        case SE_ERR_DDEFAIL:
            ErrText = _T("The DDE transaction failed.\n");
            break;
        case SE_ERR_DDETIMEOUT:
            ErrText = _T("The DDE transaction could not be completed because the request timed out.\n");
            break;
        case SE_ERR_NOASSOC:
            ErrText = _T("There is no application associated with the given filename extension.\n");
            break;
        case SE_ERR_SHARE:
            ErrText = _T("A sharing violation occurred.\n");
            break;
         default:
            ErrText = YoriLibGetWinErrorText((DWORD)(DWORD_PTR)hInst);
            AllocatedError = TRUE;
            break;
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("start: execution failed: %s"), ErrText);

    if (AllocatedError) {
        YoriLibFreeWinErrorText(ErrText);
    }

    return FALSE;
}

/**
 Try to launch a single program using the best available API.

 @param ArgC Count of arguments.

 @param ArgV Array of arguments.

 @param ShowState The state of the window for the new program.

 @param Elevate TRUE to indicate the program should prompt for elevation.

 @return TRUE to indicate success, FALSE on failure.
 */
BOOL
StartExecute(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[],
    __in INT ShowState,
    __in BOOL Elevate
    )
{
    YoriLibLoadShell32Functions();

    if (Elevate && DllShell32.pShellExecuteExW == NULL) {
        return FALSE;
    }

    if (DllShell32.pShellExecuteW != NULL) {
        return StartShellExecute(ArgC, ArgV, ShowState, Elevate);
    }

    return StartCreateProcess(ArgC, ArgV, ShowState);
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the start builtin command.
 */
#define ENTRYPOINT YoriCmd_YSTART
#else
/**
 The main entrypoint for the start standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the start cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD StartArg = 0;
    DWORD i;
    YORI_STRING Arg;
    INT ShowState;
    YORI_STRING FoundExecutable;
    BOOL PrependYori = FALSE;
    BOOL Elevate = FALSE;
    BOOL Result;

    ShowState = SW_SHOWNORMAL;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                StartHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                Elevate = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s:b")) == 0) {
                ShowState = SW_SHOWNOACTIVATE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s:h")) == 0) {
                ShowState = SW_HIDE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s:m")) == 0) {
                ShowState = SW_SHOWMINNOACTIVE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i + 1;
                ArgumentUnderstood = TRUE;
                break;
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

    if (StartArg == 0 || StartArg == ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("start: missing argument\n"));
        return EXIT_FAILURE;
    }

    //
    //  Look for an executable in the path.  Note that since ShellExecute can
    //  do more than launch executables, there's no guarantee that this will
    //  find anything, nor any guarantee that what it finds is the same thing
    //  ShellExecute will run.  But if it does find something that ends in
    //  .ys1, add in "yori /c" so that ShellExecute knows what to do with it.
    //

    YoriLibInitEmptyString(&FoundExecutable);
    if (!YoriLibLocateExecutableInPath(&ArgV[StartArg], NULL, NULL, &FoundExecutable)) {
        YoriLibInitEmptyString(&FoundExecutable);
    }

    if (FoundExecutable.LengthInChars > 0) {
        LPTSTR Ext;
        YORI_STRING YsExt;

        YoriLibInitEmptyString(&YsExt);
        Ext = YoriLibFindRightMostCharacter(&FoundExecutable, '.');
        if (Ext != NULL) {
            YsExt.StartOfString = Ext;
            YsExt.LengthInChars = FoundExecutable.LengthInChars - (DWORD)(Ext - FoundExecutable.StartOfString);
            if (YoriLibCompareStringWithLiteralInsensitive(&YsExt, _T(".ys1")) == 0) {
                PrependYori = TRUE;
            }
        }
    }
    YoriLibFreeStringContents(&FoundExecutable);

    //
    //  Note that the path resolved name is not what gets sent to
    //  ShellExecute.  That is only used to detect if an extension is
    //  present.
    //

    Result = FALSE;
    if (PrependYori) {
        DWORD ArgCount = ArgC - StartArg;
        DWORD Index;
        YORI_STRING * ArgArray;

        ArgArray = YoriLibMalloc((ArgCount + 2) * sizeof(YORI_STRING));
        if (ArgArray == NULL) {
            return EXIT_FAILURE;
        }

        YoriLibInitEmptyString(&ArgArray[0]);
        if (!YoriLibAllocateAndGetEnvironmentVariable(_T("YORISPEC"), &ArgArray[0])) {
            YoriLibConstantString(&ArgArray[0], _T("Yori"));
        }
        YoriLibConstantString(&ArgArray[1], _T("/c"));

        //
        //  Note this is not updating any references
        //

        for (Index = 0; Index < ArgCount; Index++) {
            memcpy(&ArgArray[Index + 2], &ArgV[StartArg + Index], sizeof(YORI_STRING));
            ArgArray[Index + 2].MemoryToFree = NULL;
        }

        Result = StartExecute(ArgCount + 2, ArgArray, ShowState, Elevate);
        YoriLibFreeStringContents(&ArgArray[0]);
        YoriLibFree(ArgArray);
    } else {
        Result = StartExecute(ArgC - StartArg, &ArgV[StartArg], ShowState, Elevate);
    }

    if (Result) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

// vim:sw=4:ts=4:et:
