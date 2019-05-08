/**
 * @file env/env.c
 *
 * Yori shell set environment state and execute command
 *
 * Copyright (c) 2019 Malcolm J. Smith
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
#if YORI_BUILTIN
#include <yoricall.h>
#endif

/**
 Help text to display to the user.
 */
const
CHAR strEnvHelpText[] =
        "\n"
        "Set environment state and execute a command.\n"
        "\n"
        "ENV [-license] [Var=Value] Command\n"
        "\n"
        "   --             Treat all further arguments as variables or commands\n";

/**
 Display usage text to the user.
 */
BOOL
EnvHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Env %i.%02i\n"), ENV_VER_MAJOR, ENV_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strEnvHelpText);
    return TRUE;
}

/**
 Process any arguments to the command that set or delete environment
 variables, and indicate on completion the first argument that indicates
 a command to execute.

 @param StartEnvArg The array index of the first argument to parse to see if
        it will change an environment variable.

 @param ArgC The total number of array elements.

 @param ArgV Pointer to the array of arguments.

 @param StartCmdArg On successful completion, updated to point to the first
        argument to scan for a command to execute.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
EnvModifyEnvironment(
    __in DWORD StartEnvArg,
    __in DWORD ArgC,
    __in YORI_STRING ArgV[],
    __out PDWORD StartCmdArg
    )
{
    YORI_STRING Variable;
    YORI_STRING Value;
    LPTSTR Equals;
    DWORD i;

    *StartCmdArg = 0;

    YoriLibInitEmptyString(&Variable);
    for (i = StartEnvArg; i < ArgC; i++) {
        Equals = YoriLibFindLeftMostCharacter(&ArgV[i], '=');
        if (Equals == NULL) {
            *StartCmdArg = i;
            break;
        }

        Variable.StartOfString = ArgV[i].StartOfString;
        Variable.LengthInChars = (DWORD)(Equals - ArgV[i].StartOfString);
        Variable.LengthAllocated = Variable.LengthInChars + 1;
        Variable.StartOfString[Variable.LengthInChars] = '\0';

        Value.StartOfString = &Equals[1];
        Value.LengthInChars = (DWORD)(ArgV[i].LengthInChars - Variable.LengthAllocated);
        Value.LengthAllocated = Value.LengthInChars + 1;

#ifdef YORI_BUILTIN
        if (Value.LengthInChars == 0) {
            YoriCallSetEnvironmentVariable(&Variable, NULL);
        } else {
            YoriCallSetEnvironmentVariable(&Variable, &Value);
        }
#else
        if (Value.LengthInChars == 0) {
            SetEnvironmentVariable(Variable.StartOfString, NULL);
        } else {
            SetEnvironmentVariable(Variable.StartOfString, Value.StartOfString);
        }
#endif
    }

    return TRUE;
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the env builtin command.
 */
#define ENTRYPOINT YoriCmd_YENV
#else
/**
 The main entrypoint for the env standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the env cmdlet.

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
    YORI_STRING CmdLine;
    DWORD ExitCode;
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartEnvArg = 0;
    DWORD StartCmdArg = 0;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                EnvHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartEnvArg = i + 1;
                break;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartEnvArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartEnvArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("env: argument not specified\n"));
        return EXIT_FAILURE;
    }

#ifdef YORI_BUILTIN
    {
        YORI_STRING SavedEnvironment;

        if (!YoriLibGetEnvironmentStrings(&SavedEnvironment)) {
            return EXIT_FAILURE;
        }

        EnvModifyEnvironment(StartEnvArg, ArgC, ArgV, &StartCmdArg);

        if (StartCmdArg == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("env: argument not specified\n"));
            YoriLibBuiltinSetEnvironmentStrings(&SavedEnvironment);
            return EXIT_FAILURE;
        }
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartCmdArg, &ArgV[StartCmdArg], TRUE, &CmdLine)) {
            return EXIT_FAILURE;
        }

        YoriCallExecuteExpression(&CmdLine);
        YoriLibFreeStringContents(&CmdLine);
        YoriLibBuiltinSetEnvironmentStrings(&SavedEnvironment);

        ExitCode = YoriCallGetErrorLevel();
    }
#else
    {
        YORI_STRING Executable;
        PYORI_STRING ChildArgs;
        PROCESS_INFORMATION ProcessInfo;
        STARTUPINFO StartupInfo;

        EnvModifyEnvironment(StartEnvArg, ArgC, ArgV, &StartCmdArg);

        if (StartCmdArg == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("env: argument not specified\n"));
            return EXIT_FAILURE;
        }

        ChildArgs = YoriLibMalloc((ArgC - StartCmdArg) * sizeof(YORI_STRING));
        if (ChildArgs == NULL) {
            return EXIT_FAILURE;
        }

        YoriLibInitEmptyString(&Executable);
        if (!YoriLibLocateExecutableInPath(&ArgV[StartCmdArg], NULL, NULL, &Executable) ||
            Executable.LengthInChars == 0) {

            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("env: unable to find executable\n"));
            YoriLibFree(ChildArgs);
            YoriLibFreeStringContents(&Executable);
            return EXIT_FAILURE;
        }


        memcpy(&ChildArgs[0], &Executable, sizeof(YORI_STRING));
        if (StartCmdArg + 1 < ArgC) {
            memcpy(&ChildArgs[1], &ArgV[StartCmdArg + 1], (ArgC - StartCmdArg - 1) * sizeof(YORI_STRING));
        }

        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartCmdArg, ChildArgs, TRUE, &CmdLine)) {
            YoriLibFree(ChildArgs);
            YoriLibFreeStringContents(&Executable);
            return EXIT_FAILURE;
        }

        ASSERT(YoriLibIsStringNullTerminated(&CmdLine));

        memset(&StartupInfo, 0, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);

        if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInfo)) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("env: execution failed: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            YoriLibFree(ChildArgs);
            YoriLibFreeStringContents(&CmdLine);
            YoriLibFreeStringContents(&Executable);
            return EXIT_FAILURE;
        }

        WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
        GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
        CloseHandle(ProcessInfo.hProcess);
        CloseHandle(ProcessInfo.hThread);
        YoriLibFreeStringContents(&Executable);
        YoriLibFreeStringContents(&CmdLine);
        YoriLibFree(ChildArgs);
    }
#endif

    return ExitCode;
}

// vim:sw=4:ts=4:et:
