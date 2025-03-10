/**
 * @file cvtvt/main.c
 *
 * Main entry point code to convert and process VT100/ANSI escape sequences.
 *
 * Copyright (c) 2015-2018 Malcolm J. Smith
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

#include "cvtvt.h"

/**
 Help text to display for this application.
 */
const
CHAR strCvtvtHelpText[] =
        "\n"
        "Converts text with VT100 color escapes into another format.\n"
        "\n"
        "CVTVT [-license] [Options] [-exec binary|filename]\n"
        "\n"
        " Options include:\n"
        "   -exec binary   Run process and pipe its output into cvtvt\n"
        "   -html4         Generate output with FONT tags (no backgrounds)\n"
        "   -html5         Generate output with CSS\n"
        "   -rtf           Generate output as RTF\n"
        "   -text          Generate output as plain text\n"
        "   -win32         Convert to native Win32\n"
        "\n"
        " If filename and exec are not specified, operates on standard"
        " input.\n";

/**
 Display help text for this application.
 */
BOOL
CvtvtUsage(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Cvtvt %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strCvtvtHelpText);
    return TRUE;
}

/**
 Pump any incoming data from standard input to a specified pipe which will
 be used as the input for a child process.

 @param Context The context for the child, which in this case is the handle
        for the pipe to write data to.

 @return Exit code for the thread, currently unused.
 */
DWORD WINAPI
CvtvtInputPumpThread(
    __in LPVOID Context
    )
{
    HANDLE hOutput = (HANDLE)Context;
    HANDLE hInput;
    UCHAR StupidBuffer[256];
    DWORD BytesRead;
    DWORD BytesWritten;

    hInput = GetStdHandle(STD_INPUT_HANDLE);

    YoriLibSetInputConsoleModeWithoutExtended(hInput, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);

    while (ReadFile(hInput, StupidBuffer, sizeof(StupidBuffer), &BytesRead, NULL)) {
        if (!WriteFile(hOutput, StupidBuffer, BytesRead, &BytesWritten, NULL)) {
            break;
        }
    }

    return 0;
}

/**
 Launch a specified child process, with input and output handles created as
 pipes.  This function returns the ends of the pipe used to communicate with
 the child process.

 @param PathName Pointer to the string for the program to launch.  This will
        be located in the path before executing.

 @param CmdLine Pointer to the entire command line including arguments.

 @param hSource On successful completion, updated to contain a handle that
        reads the output from the child process.

 @param hControl On successful completion, updated to contain a handle that
        writes the input into the child process.

 @return Win32 error code, ERROR_SUCCESS for success.
 */
__success(return == ERROR_SUCCESS)
DWORD
CvtvtLaunchChildProcess(
    __in PYORI_STRING PathName,
    __in PYORI_STRING CmdLine,
    __out PHANDLE hSource,
    __out PHANDLE hControl
    )
{
    HANDLE hProcessInput;
    HANDLE hProcessOutput;
    HANDLE hParentInput;
    HANDLE hParentOutput;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    TCHAR szTermVar[256];
    DWORD dwConsoleMode;
    DWORD Err;
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    YORI_STRING ExpandedCmd;
    YORI_STRING FoundExecutable;

    if (!YoriLibExpandHomeDirectories(PathName, &ExpandedCmd)) {
        YoriLibCloneString(&ExpandedCmd, PathName);
    }

    YoriLibInitEmptyString(&FoundExecutable);
    if (!YoriLibLocateExecutableInPath(&ExpandedCmd, NULL, NULL, &FoundExecutable)) {
        YoriLibFreeStringContents(&ExpandedCmd);
        return ERROR_FILE_NOT_FOUND;
    }

    YoriLibFreeStringContents(&ExpandedCmd);

    if (FoundExecutable.LengthInChars == 0) {
        YoriLibFreeStringContents(&FoundExecutable);
        return ERROR_FILE_NOT_FOUND;
    }

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

    if (!CreatePipe(&hProcessInput, &hParentOutput, NULL, 2048)) {
        Err = GetLastError();
        YoriLibFreeStringContents(&FoundExecutable);
        return Err;
    }

    if (!CreatePipe(&hParentInput, &hProcessOutput, NULL, 2048)) {
        Err = GetLastError();
        CloseHandle(hProcessInput);
        CloseHandle(hParentOutput);
        YoriLibFreeStringContents(&FoundExecutable);
        return Err;
    }

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ConsoleInfo)) {
        YoriLibSPrintf(szTermVar, _T("%i"), ConsoleInfo.srWindow.Right - ConsoleInfo.srWindow.Left + 1);
        SetEnvironmentVariable(_T("COLUMNS"), szTermVar);
        YoriLibSPrintf(szTermVar, _T("%i"), ConsoleInfo.srWindow.Bottom - ConsoleInfo.srWindow.Top + 1);
        SetEnvironmentVariable(_T("LINES"), szTermVar);

        if (GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &dwConsoleMode)) {

            //
            //  If the window right edge is the end of the buffer, then
            //  auto line wrap is in effect if the console has it
            //  enabled.  If neither of these is true, then apps must
            //  explicitly emit newlines.
            //

            if (ConsoleInfo.dwSize.X != (ConsoleInfo.srWindow.Right - ConsoleInfo.srWindow.Left + 1)) {
                dwConsoleMode = 0;
            }
            YoriLibSPrintf(szTermVar, _T("color;extendedchars%s"), dwConsoleMode&ENABLE_WRAP_AT_EOL_OUTPUT?_T(";autolinewrap"):_T(""));
            SetEnvironmentVariable(_T("YORITERM"), szTermVar);
        }

    }

    YoriLibMakeInheritableHandle(hProcessInput, &hProcessInput);
    YoriLibMakeInheritableHandle(hProcessOutput, &hProcessOutput);

    StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    StartupInfo.hStdInput = hProcessInput;
    StartupInfo.hStdOutput = hProcessOutput;
    StartupInfo.hStdError = hProcessOutput;

    SetEnvironmentVariable(_T("PROMPT"), _T("$e[31;1m$p$e[37m$g"));

    if (!CreateProcess(FoundExecutable.StartOfString,
                       CmdLine->StartOfString,
                       NULL,
                       NULL,
                       TRUE,
                       CREATE_DEFAULT_ERROR_MODE,
                       NULL,
                       NULL,
                       &StartupInfo,
                       &ProcessInfo)) {

        YoriLibFreeStringContents(&FoundExecutable);

        CloseHandle(hProcessInput);
        CloseHandle(hProcessOutput);

        CloseHandle(hParentInput);
        CloseHandle(hParentOutput);

        return GetLastError();
    }

    YoriLibFreeStringContents(&FoundExecutable);

    CloseHandle(hProcessInput);
    CloseHandle(hProcessOutput);

    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);

    *hControl = hParentOutput;
    *hSource = hParentInput;
    return ERROR_SUCCESS;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the cvtvt builtin command.
 */
#define ENTRYPOINT YoriCmd_CVTVT
#else
/**
 The main entrypoint for the cvtvt standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The master entrypoint for cvtvt.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments to cvtvt.

 @return Exit code for the application, typically zero for success and
         nonzero for failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    HANDLE hSource = INVALID_HANDLE_VALUE;
    HANDLE hOutput = INVALID_HANDLE_VALUE;
    HANDLE hControl = INVALID_HANDLE_VALUE;
    HANDLE InputPumpThread = NULL;

    YORI_ALLOC_SIZE_T CurrentOffset;
    PVOID  LineReadContext = NULL;
    BOOL   Result;
    YORI_STRING LineString;
    YORI_STRING Arg;
    PYORI_STRING UserFileName = NULL;
    YORI_STRING FileName;
    YORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks;
    YORI_ALLOC_SIZE_T StartArg = 0;

    BOOLEAN StreamStarted = FALSE;
    BOOLEAN ExecMode = FALSE;
    BOOLEAN DisplayUsage = FALSE;
    BOOLEAN ArgParsed = FALSE;
    BOOLEAN StripEscapes = FALSE;

    YORI_LIB_LINE_ENDING LineEnding;
    BOOL TimeoutReached;

    Callbacks.Context = 0;
    CvtvtHtml4SetFunctions(&Callbacks);

    //
    //  Parse arguments
    //

    for (CurrentOffset = 1; CurrentOffset < ArgC; CurrentOffset++) {
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[CurrentOffset]));
        if (YoriLibIsCommandLineOption(&ArgV[CurrentOffset], &Arg)) {

            ArgParsed = FALSE;

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                ArgParsed = TRUE;
                DisplayUsage = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2015-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("exec")) == 0) {
                ArgParsed = TRUE;
                ExecMode = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("html4")) == 0) {
                ArgParsed = TRUE;
                StripEscapes = FALSE;
                CvtvtHtml4SetFunctions(&Callbacks);
            } else if (YoriLibCompareStringLitIns(&Arg, _T("html5")) == 0) {
                ArgParsed = TRUE;
                StripEscapes = FALSE;
                CvtvtHtml5SetFunctions(&Callbacks);
            } else if (YoriLibCompareStringLitIns(&Arg, _T("rtf")) == 0) {
                ArgParsed = TRUE;
                StripEscapes = FALSE;
                CvtvtRtfSetFunctions(&Callbacks);
            } else if (YoriLibCompareStringLitIns(&Arg, _T("text")) == 0) {
                ArgParsed = TRUE;
                StripEscapes = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("win32")) == 0) {
                ArgParsed = TRUE;
                StripEscapes = FALSE;
                YoriLibConsoleSetFn(&Callbacks);
            }

            if (!ArgParsed) {
                DisplayUsage = TRUE;
            }

        } else {
            StartArg = CurrentOffset;
            break;
        }
    }

    YoriLibInitEmptyString(&FileName);

    //
    //  As a bit of a hack, if the input is stdin and stdin is to
    //  a console (as opposed to a pipe or file) assume the user isn't
    //  sure how to run this program and help them along.
    //

    if (StartArg == 0) {
        if (YoriLibIsStdInConsole()) {
            DisplayUsage = TRUE;
        }
        UserFileName = NULL;
    } else {
        UserFileName = &ArgV[StartArg];
    }

    if (DisplayUsage) {
        CvtvtUsage();
        return EXIT_FAILURE;
    }

    //
    //  Attempt to enable backup privilege so an administrator can access more
    //  objects successfully.
    //

    YoriLibEnableBackupPrivilege();

    //
    //  If we have a file name, read it; otherwise read from stdin
    //

    if (ExecMode) {
        if (UserFileName == NULL) {
            CvtvtUsage();
            return EXIT_FAILURE;
        } else {
            YORI_STRING CmdLine;
            DWORD Err;

            YoriLibInitEmptyString(&CmdLine);
            if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], TRUE, TRUE, &CmdLine)) {
                return EXIT_FAILURE;
            }
            Err = CvtvtLaunchChildProcess(&ArgV[StartArg], &CmdLine, &hSource, &hControl);
            YoriLibFreeStringContents(&CmdLine);
            if (Err != ERROR_SUCCESS) {
                LPTSTR ErrText = YoriLibGetWinErrorText(Err);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("cvtvt: could not launch process: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
                return EXIT_FAILURE;
            }
            InputPumpThread = CreateThread(NULL, 0, CvtvtInputPumpThread, hControl, 0, NULL);
        }
    } else if (UserFileName != NULL) {
        if (!YoriLibUserStringToSingleFilePath(UserFileName, TRUE, &FileName)) {
            return EXIT_FAILURE;
        }

        hSource = CreateFile(FileName.StartOfString,
                             GENERIC_READ,
                             FILE_SHARE_READ|FILE_SHARE_DELETE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
                             NULL);
        if (hSource == INVALID_HANDLE_VALUE) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("cvtvt: could not open file, error %i\n"), (int)GetLastError());
            return EXIT_FAILURE;
        }
    } else {
        hSource = GetStdHandle(STD_INPUT_HANDLE);
    }

    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (StripEscapes) {
        DWORD CurrentMode;
        if (GetConsoleMode(hOutput, &CurrentMode)) {
            YoriLibConsoleNoEscSetFn(&Callbacks);
        } else {
            YoriLibUtf8TextNoEscSetFn(&Callbacks);
        }
    }

    YoriLibInitEmptyString(&LineString);

    Result = TRUE;

    while (YoriLibReadLineToStringEx(&LineString, &LineReadContext, TRUE, 100, hSource, &LineEnding, &TimeoutReached) || TimeoutReached) {

        //
        //  Start producing HTML
        //

        if (!StreamStarted) {
            Callbacks.InitializeStream(hOutput, &Callbacks.Context);
            StreamStarted = TRUE;
        }

        if (LineString.LengthInChars > 0) {

            if (!YoriLibProcVtEscOnOpenStream(LineString.StartOfString,
                                                     LineString.LengthInChars,
                                                     hOutput,
                                                     &Callbacks)) {

                Result = FALSE;
                break;
            }

        }

        if (LineEnding != YoriLibLineEndingNone) {
            if (!YoriLibProcVtEscOnOpenStream(_T("\n"),
                                                     1,
                                                     hOutput,
                                                     &Callbacks)) {

                Result = FALSE;
                break;
            }
        }
    }

    if (StreamStarted) {
        Callbacks.EndStream(hOutput, &Callbacks.Context);
    }

    YoriLibLineReadCloseOrCache(LineReadContext);
    YoriLibFreeStringContents(&LineString);
    if (hSource != INVALID_HANDLE_VALUE && UserFileName != NULL) {
        CloseHandle(hSource);
    }

    if (InputPumpThread != NULL) {
        //
        //  TerminateThread is evil because the thread can't do any cleanup.
        //  Here we "know" the thread is very simple, holds no locks, does
        //  no cleanup of its own, etc.
        //
#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(disable: 6258)
#endif
        TerminateThread(InputPumpThread, 0);
        CloseHandle(InputPumpThread);
    }

    if (hControl != INVALID_HANDLE_VALUE) {
        CloseHandle(hControl);
    }

    YoriLibFreeStringContents(&FileName);

#if !YORI_BUILTIN
    YoriLibLineReadCleanupCache();
#endif

    if (Result) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

// vim:sw=4:ts=4:et:
