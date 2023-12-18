/**
 * @file env/env.c
 *
 * Yori shell set environment state and execute command
 *
 * Copyright (c) 2019-2023 Malcolm J. Smith
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
        "ENV [-license] [-iv Var] [Var=Value] Command\n"
        "\n"
        "   -iv            Set a variable whose value is provided as standard input\n"
        "   --             Treat all further arguments as variables or commands\n";

/**
 Display usage text to the user.
 */
BOOL
EnvHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Env %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strEnvHelpText);
    return TRUE;
}

/**
 A buffer for a single data stream.
 */
typedef struct _ENV_BUFFER {

    /**
     A handle to a pipe which is the source of data for this buffer.
     */
    HANDLE hSource;

    /**
     The data buffer.
     */
    YORI_LIB_BYTE_BUFFER ByteBuffer;

} ENV_BUFFER, *PENV_BUFFER;

/**
 Allocate and initialize a buffer for an input stream.

 @param Buffer Pointer to the buffer to allocate structures for.

 @return TRUE if the buffer is successfully initialized, FALSE if it is not.
 */
BOOL
EnvAllocateBuffer(
    __out PENV_BUFFER Buffer
    )
{
    return YoriLibByteBufferInitialize(&Buffer->ByteBuffer, 1024);
}

/**
 Free structures associated with a single input stream.

 @param ThisBuffer Pointer to the single stream's buffers to deallocate.
 */
VOID
EnvFreeBuffer(
    __in PENV_BUFFER ThisBuffer
    )
{
    YoriLibByteBufferCleanup(&ThisBuffer->ByteBuffer);
}

/**
 Populate data from stdin into an in memory buffer.

 @param ThisBuffer A pointer to the process buffer set.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
EnvBufferPump(
    __in PENV_BUFFER ThisBuffer
    )
{
    DWORD BytesRead;
    BOOLEAN Result = FALSE;
    PUCHAR WriteBuffer;
    YORI_ALLOC_SIZE_T BytesAvailable;

    while (TRUE) {

        WriteBuffer = YoriLibByteBufferGetPointerToEnd(&ThisBuffer->ByteBuffer, 16384, &BytesAvailable);
        if (WriteBuffer == NULL) {
            break;
        }

        if (ReadFile(ThisBuffer->hSource,
                     WriteBuffer,
                     BytesAvailable,
                     &BytesRead,
                     NULL)) {

            if (BytesRead == 0) {
                Result = TRUE;
                break;
            }

            YoriLibByteBufferAddToPopulatedLength(&ThisBuffer->ByteBuffer, BytesRead);

        } else {
            Result = TRUE;
            break;
        }
    }

    return Result;
}

/**
 Process any arguments to the command that set or delete environment
 variables, and indicate on completion the first argument that indicates
 a command to execute.

 @param StartEnvArg The array index of the first argument to parse to see if
        it will change an environment variable.

 @param ArgC The total number of array elements.

 @param ArgV Pointer to the array of arguments.

 @param StdinVariableName Optionally points to a variable name corresponding
        to the value in Stdin.

 @param Stdin Pointer to a context describing a buffer of data from stdin.
        May be zero length if stdin does not supply data.

 @param StartCmdArg On successful completion, updated to point to the first
        argument to scan for a command to execute.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
EnvModifyEnvironment(
    __in YORI_ALLOC_SIZE_T StartEnvArg,
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[],
    __in_opt PYORI_STRING StdinVariableName,
    __in PENV_BUFFER Stdin,
    __out PYORI_ALLOC_SIZE_T StartCmdArg
    )
{
    YORI_STRING Variable;
    YORI_STRING Value;
    LPTSTR Equals;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StdinLength;
    YORI_STRING StdinString;

    *StartCmdArg = 0;

    YoriLibInitEmptyString(&Variable);
    for (i = StartEnvArg; i < ArgC; i++) {
        Equals = YoriLibFindLeftMostCharacter(&ArgV[i], '=');
        if (Equals == NULL) {
            *StartCmdArg = i;
            break;
        }

        Variable.StartOfString = ArgV[i].StartOfString;
        Variable.LengthInChars = (YORI_ALLOC_SIZE_T)(Equals - ArgV[i].StartOfString);
        Variable.LengthAllocated = Variable.LengthInChars + 1;
        Variable.StartOfString[Variable.LengthInChars] = '\0';

        Value.StartOfString = &Equals[1];
        Value.LengthInChars = ArgV[i].LengthInChars - Variable.LengthAllocated;
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

    if (StdinVariableName != NULL) {
        YoriLibInitEmptyString(&StdinString);

        if (YoriLibByteBufferGetValidBytes(&Stdin->ByteBuffer) != 0) {
            YORI_ALLOC_SIZE_T BytesPopulated;
            PUCHAR Buffer;

            Buffer = YoriLibByteBufferGetPointerToValidData(&Stdin->ByteBuffer, 0, &BytesPopulated);
            ASSERT(Buffer != NULL);
            if (Buffer == NULL) {
                return FALSE;
            }

            StdinLength = YoriLibGetMultibyteInputSizeNeeded(Buffer, BytesPopulated);
            if (!YoriLibAllocateString(&StdinString, StdinLength + 1)) {
                return FALSE;
            }
            YoriLibMultibyteInput(Buffer, BytesPopulated, StdinString.StartOfString, StdinLength);
            StdinString.LengthInChars = StdinLength;

            //
            //  Truncate any newlines from the output, which tools
            //  frequently emit but are of no value here
            //

            YoriLibTrimTrailingNewlines(&StdinString);
            StdinString.StartOfString[StdinString.LengthInChars] = '\0';
        }
#ifdef YORI_BUILTIN
        if (StdinString.LengthInChars == 0) {
            YoriCallSetEnvironmentVariable(StdinVariableName, NULL);
        } else {
            YoriCallSetEnvironmentVariable(StdinVariableName, &StdinString);
        }
#else
        if (StdinString.LengthInChars == 0) {
            SetEnvironmentVariable(StdinVariableName->StartOfString, NULL);
        } else {
            SetEnvironmentVariable(StdinVariableName->StartOfString, StdinString.StartOfString);
        }
#endif

        YoriLibFreeStringContents(&StdinString);
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
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_STRING CmdLine;
    PYORI_STRING StdinVariableName;
    DWORD ExitCode;
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartEnvArg = 0;
    YORI_ALLOC_SIZE_T StartCmdArg = 0;
    YORI_STRING Arg;
    BOOLEAN LoadStdin;
    ENV_BUFFER Stdin;

    LoadStdin = FALSE;
    StdinVariableName = NULL;
    ZeroMemory(&Stdin, sizeof(Stdin));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                EnvHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019-2023"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("iv")) == 0) {
                if (i + 1 < ArgC) {
                    LoadStdin = TRUE;
                    StdinVariableName = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }
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

    if (LoadStdin) {
        DWORD ConsoleMode;

        if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &ConsoleMode)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("env: no file or pipe for input\n"));
            return EXIT_FAILURE;
        }

        if (!EnvAllocateBuffer(&Stdin)) {
            return EXIT_FAILURE;
        }
        Stdin.hSource = GetStdHandle(STD_INPUT_HANDLE);

        if (!EnvBufferPump(&Stdin)) {
            EnvFreeBuffer(&Stdin);
            return EXIT_FAILURE;
        }
    }

#ifdef YORI_BUILTIN
    {
        YORI_STRING SavedEnvironment;

        if (!YoriLibGetEnvironmentStrings(&SavedEnvironment)) {
            EnvFreeBuffer(&Stdin);
            return EXIT_FAILURE;
        }

        EnvModifyEnvironment(StartEnvArg, ArgC, ArgV, StdinVariableName, &Stdin, &StartCmdArg);
        EnvFreeBuffer(&Stdin);

        if (StartCmdArg == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("env: argument not specified\n"));
            YoriLibBuiltinSetEnvironmentStrings(&SavedEnvironment);
            return EXIT_FAILURE;
        }
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartCmdArg, &ArgV[StartCmdArg], TRUE, TRUE, &CmdLine)) {
            YoriLibBuiltinSetEnvironmentStrings(&SavedEnvironment);
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

        EnvModifyEnvironment(StartEnvArg, ArgC, ArgV, StdinVariableName, &Stdin, &StartCmdArg);
        EnvFreeBuffer(&Stdin);

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

        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartCmdArg, ChildArgs, TRUE, TRUE, &CmdLine)) {
            YoriLibFree(ChildArgs);
            YoriLibFreeStringContents(&Executable);
            return EXIT_FAILURE;
        }

        ASSERT(YoriLibIsStringNullTerminated(&CmdLine));

        memset(&StartupInfo, 0, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);

        if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &StartupInfo, &ProcessInfo)) {
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
