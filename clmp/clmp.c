/**
 * @file clmp/clmp.c
 *
 * Multi processor support for older versions of Visual C++ that don't implement
 * it natively.
 *
 * Copyright (c) 2015-2023 Malcolm J. Smith
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
 Help text to display to the user for this application.
 */
const
CHAR strHelpText[] =
        "\n"
        "Multi process compiler wrapper\n"
        "\n"
        "CLMP [-license] [-MP[n]] <arguments to CL>\n"
        "\n"
        "   -MP[n]         Use up to 'n' processes for compilation\n";

/**
 Display the help and license information for this application.
 */
BOOL
ClmpHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Clmp %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpText);
    return TRUE;
}

/**
 The exit code for the application, populated if any child process fails in
 error.
 */
DWORD GlobalExitCode = 0;

/**
 A mutex to use to synchronize output so only one line of output is being
 rendered at a time.
 */
HANDLE hOutputMutex;

/**
 Information about a pipe and a buffer attached to that pipe for reading
 data being output by a child process.
 */
typedef struct _CLMP_PIPE_BUFFER {

    /**
     A handle to the pipe to read from the child process.
     */
    HANDLE Pipe;

    /**
     A handle to the thread processing this stream.
     */
    HANDLE hPumpThread;

    /**
     The flags to use when outputting anything from this stream.
     */
    WORD OutputFlags;
} CLMP_PIPE_BUFFER, *PCLMP_PIPE_BUFFER;

/**
 Information about a single outstanding child process.
 */
typedef struct _CLMP_PROCESS_INFO {

    /**
     Information as returned by Windows when launching the child process.
     */
    PROCESS_INFORMATION WindowsProcessInfo;

    /**
     Standard output and standard error pipe buffers to the child process.
     */
    CLMP_PIPE_BUFFER Pipes[2];

    /**
     A NULL terminated string corresponding to the file being processed
     by this child process.
     */
    LPTSTR Filename;

    /**
     If TRUE, process launch was started.  If FALSE, the process has been
     waited upon and no new process launch commenced.
     */
    BOOLEAN ProcessLaunchStarted;
} CLMP_PROCESS_INFO, *PCLMP_PROCESS_INFO;

/**
 A worker thread function that fetches entire lines from an input stream,
 and writes them to the current process output stream, with synchronization
 to ensure that each line is written as a line.

 @param Param Pointer to a CLMP_PIPE_BUFFER structure indicating a single
        input stream to process.

 @return Exit code for the thread.
 */
DWORD WINAPI
ClmpPumpSingleStream(
    __in LPVOID Param
    )
{
    PCLMP_PIPE_BUFFER Buffer = (PCLMP_PIPE_BUFFER)Param;
    PVOID LineContext = NULL;
    YORI_STRING LineString;

    YoriLibInitEmptyString(&LineString);

    while (TRUE) {
        if (!YoriLibReadLineToString(&LineString, &LineContext, Buffer->Pipe)) {
            break;
        }

        //
        //  Synchronize with other things writing to output
        //

        WaitForSingleObject(hOutputMutex, INFINITE);
        YoriLibOutput(Buffer->OutputFlags, _T("%y\n"), &LineString);
        ReleaseMutex(hOutputMutex);
    }

    YoriLibLineReadClose(LineContext);
    YoriLibFreeStringContents(&LineString);

    return 0;
}

/**
 Wait for a child process.  If it failed, we fail.  This function can be
 called if a process failed to launch, or has already been waited upon,
 so it must be willing to clean up whatever state exists.

 @param Process The process to wait for.
 */
VOID
ClmpWaitOnProcess (
    __in PCLMP_PROCESS_INFO Process
    )
{
    DWORD ExitCode;
    DWORD PipeNum;
    DWORD WaitResult;

    ASSERT(Process->ProcessLaunchStarted);

    if (Process->WindowsProcessInfo.hProcess != NULL) {
        WaitResult = WaitForSingleObject(Process->WindowsProcessInfo.hProcess, INFINITE);
        ASSERT(WaitResult == WAIT_OBJECT_0);
        GetExitCodeProcess(Process->WindowsProcessInfo.hProcess, &ExitCode);
    } else {

        //
        //  It's safe to use a failure as an exit code so long as the
        //  process started to launch.
        //

        ExitCode = EXIT_FAILURE;
    }

    for (PipeNum = 0; PipeNum < sizeof(Process->Pipes)/sizeof(Process->Pipes[0]); PipeNum++) {
        if (Process->Pipes[PipeNum].hPumpThread != NULL) {
            WaitResult = WaitForSingleObject(Process->Pipes[PipeNum].hPumpThread, INFINITE);
            ASSERT(WaitResult == WAIT_OBJECT_0);
            CloseHandle(Process->Pipes[PipeNum].hPumpThread);
            Process->Pipes[PipeNum].hPumpThread = NULL;
        }

        if (Process->Pipes[PipeNum].Pipe) {
            CloseHandle(Process->Pipes[PipeNum].Pipe);
            Process->Pipes[PipeNum].Pipe = NULL;
        }
    }

    //
    //  If a child failed and the parent is still going, fail with
    //  the same code.
    //

    if (ExitCode != 0 && GlobalExitCode == 0) {
        GlobalExitCode = ExitCode;
    }

    //
    //  Clean up and tear down so the process slot can be reused as
    //  necessary.
    //

    if (Process->WindowsProcessInfo.hProcess != NULL) {
        CloseHandle(Process->WindowsProcessInfo.hProcess);
        Process->WindowsProcessInfo.hProcess = NULL;
    }

    if (Process->WindowsProcessInfo.hThread != NULL) {
        CloseHandle(Process->WindowsProcessInfo.hThread);
        Process->WindowsProcessInfo.hThread = NULL;
    }

    Process->ProcessLaunchStarted = FALSE;
}

/**
 The entrypoint for the clmp application.

 @param ArgC Count of arguments.

 @param ArgV Array of argument strings.

 @return Exit code for the application, typically zero for success and
         nonzero for failure.
 */
DWORD
ymain(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_STRING CommonString;
    YORI_STRING CompleteString;
    YORI_ALLOC_SIZE_T i, j;
    PCLMP_PROCESS_INFO ProcessInfo;
    YORI_ALLOC_SIZE_T CurrentProcess = 0;
    YORI_ALLOC_SIZE_T NumberProcesses = 0;
    BOOLEAN MultiProcPossible = FALSE;
    BOOLEAN MultiProcNotPossible = FALSE;
    YORI_STRING Arg;
    SYSTEM_INFO SysInfo;

    GetSystemInfo(&SysInfo);

    YoriLibInitEmptyString(&CommonString);
    YoriLibInitEmptyString(&CompleteString);

    if (!YoriLibStringConcatWithLiteral(&CommonString, _T("cl "))) {
        return EXIT_FAILURE;
    }

    //
    //  Scan the command line looking for switches that should
    //  be common to all of the processes we spawn.  Also look
    //  for switches which tell us that multi processing is
    //  incompatible with the requested operation, and in that
    //  case, disable it.
    //

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            //
            //  If /?, display Clmp help, but continue to execute the
            //  compiler
            //

            if (YoriLibCompareStringLitInsCnt(&Arg, _T("?"), 1) == 0) {
                ClmpHelp();
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2015-2023"));
                return EXIT_SUCCESS;
            }

            //
            //  Check for MP switch and adjust the number of processes
            //  as requested.  Don't tell the compiler about this.
            //

            if (YoriLibCompareStringLitInsCnt(&Arg, _T("MP"), 2) == 0) {
                if (Arg.LengthInChars > 2) {
                    YORI_STRING NumberProcessesString;
                    YORI_MAX_SIGNED_T LlNumberProcesses;
                    YORI_ALLOC_SIZE_T CharsConsumed;

                    YoriLibInitEmptyString(&NumberProcessesString);
                    NumberProcessesString.StartOfString = &Arg.StartOfString[2];
                    NumberProcessesString.LengthInChars = Arg.LengthInChars - 2;
                    if (YoriLibStringToNumber(&NumberProcessesString, FALSE, &LlNumberProcesses, &CharsConsumed) && CharsConsumed > 0) {
                        NumberProcesses = (YORI_ALLOC_SIZE_T)LlNumberProcesses;
                    }
                }

            } else {
                if (!YoriLibStringConcatWithLiteral(&CommonString, _T(" ")) ||
                    !YoriLibStringConcat(&CommonString, &ArgV[i])) {

                    YoriLibFreeStringContents(&CommonString);
                    return EXIT_FAILURE;
                }
            }

            //
            //  Compile without linking - we need this for multiproc, because
            //  if we're linking in the same pass issuing different processes
            //  with different portions would generate the wrong result
            //

            if (YoriLibCompareStringLitCnt(&Arg, _T("c"), 1) == 0) {
                MultiProcPossible = TRUE;
            }

            //
            //  Preprocess to stdout - we can't do this in parallel without
            //  generating garbage
            //

            if (YoriLibCompareStringLitCnt(&Arg, _T("E"), 1) == 0) {
                MultiProcNotPossible = TRUE;
            }

            //
            //  Generating debug into a PDB - we can't do this in parallel
            //  or we'll get sharing violations on the PDB
            //

            if (YoriLibCompareStringLitCnt(&Arg, _T("Z"), 1) == 0) {
                for (j = 1; j < Arg.LengthInChars; j++) {
                    if (Arg.StartOfString[j] == 'i' ||
                        Arg.StartOfString[j] == 'I') {

                        MultiProcNotPossible = TRUE;
                    }
                }
            }

            //
            //  Precompiled header - we can't do this in parallel or we'll
            //  get sharing violations on the precompiled header file
            //

            if (YoriLibCompareStringLitCnt(&Arg, _T("Y"), 1) == 0) {
                for (j = 1; j < Arg.LengthInChars; j++) {
                    if (Arg.StartOfString[j] == 'c' ||
                        Arg.StartOfString[j] == 'X') {

                        MultiProcNotPossible = TRUE;
                    }
                }
            }
        }
    }

    //
    //  If we disabled multi processing, we still want to have one child
    //  or we won't get far.
    //

    if (!MultiProcPossible || MultiProcNotPossible) {
        NumberProcesses = 1;
    } else if (NumberProcesses == 0) {
        NumberProcesses = (YORI_ALLOC_SIZE_T)SysInfo.dwNumberOfProcessors + 1;
    }

    ProcessInfo = YoriLibMalloc(sizeof(CLMP_PROCESS_INFO) * NumberProcesses);

    if (ProcessInfo == NULL) {
        YoriLibFreeStringContents(&CommonString);
        return EXIT_FAILURE;
    }

    ZeroMemory(ProcessInfo, sizeof(CLMP_PROCESS_INFO) * NumberProcesses);

    hOutputMutex = CreateMutex(NULL, FALSE, NULL);
    if (hOutputMutex == NULL) {
        YoriLibFreeStringContents(&CommonString);
        YoriLibFree(ProcessInfo);
        return EXIT_FAILURE;
    }

    //
    //  Scan again looking for source files, and spawn one child
    //  process per argument found, combined with the command
    //  line we built earlier.
    //

    for (i = 1; i < ArgC; i++) {
        if (!YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            STARTUPINFO StartupInfo;
            DWORD MyProcess = CurrentProcess;
            SECURITY_ATTRIBUTES SecurityAttributes;
            HANDLE WriteOutPipe, WriteErrPipe;
            DWORD ThreadId;

            CompleteString.LengthInChars = 0;
            if (!YoriLibStringConcat(&CompleteString, &CommonString) ||
                !YoriLibStringConcatWithLiteral(&CompleteString, _T(" ")) ||
                !YoriLibStringConcat(&CompleteString, &ArgV[i])) {

                GlobalExitCode = EXIT_FAILURE;
                goto drain;
            }

            ZeroMemory(&StartupInfo, sizeof(StartupInfo));
            StartupInfo.cb = sizeof(StartupInfo);

            MyProcess %= NumberProcesses;

            //
            //  If we've run out of processors, wait for the next child
            //  process and reuse its slot.
            //

            if (CurrentProcess >= NumberProcesses) {
                ClmpWaitOnProcess(&ProcessInfo[MyProcess]);
                if (GlobalExitCode) {
                    goto drain;
                }
            }

            //
            //  We need to specify security attributes because we want our
            //  standard output and standard error handles to be inherited.
            //

            ZeroMemory(&SecurityAttributes, sizeof(SecurityAttributes));

            SecurityAttributes.nLength = sizeof(SecurityAttributes);
            SecurityAttributes.bInheritHandle = TRUE;

            //
            //  Mark launch as having started.  Any failure after this point
            //  is considered an error when this process exits.
            //

            ProcessInfo[MyProcess].ProcessLaunchStarted = TRUE;

            //
            //  Create the aforementioned handles.
            //

            if (!CreatePipe(&ProcessInfo[MyProcess].Pipes[0].Pipe, &WriteOutPipe, &SecurityAttributes, 0)) {

                GlobalExitCode = EXIT_FAILURE;
                goto drain;
            }

            if (!CreatePipe(&ProcessInfo[MyProcess].Pipes[1].Pipe, &WriteErrPipe, &SecurityAttributes, 0)) {

                GlobalExitCode = EXIT_FAILURE;
                goto drain;
            }

            ProcessInfo[MyProcess].Pipes[0].OutputFlags = YORI_LIB_OUTPUT_STDOUT;

            ProcessInfo[MyProcess].Pipes[0].hPumpThread = CreateThread(NULL, 0, ClmpPumpSingleStream, &ProcessInfo[MyProcess].Pipes[0], 0, &ThreadId);
            if (ProcessInfo[MyProcess].Pipes[0].hPumpThread == NULL) {
                GlobalExitCode = EXIT_FAILURE;
                goto drain;
            }

            ProcessInfo[MyProcess].Pipes[1].OutputFlags = YORI_LIB_OUTPUT_STDERR;

            ProcessInfo[MyProcess].Pipes[1].hPumpThread = CreateThread(NULL, 0, ClmpPumpSingleStream, &ProcessInfo[MyProcess].Pipes[1], 0, &ThreadId);
            if (ProcessInfo[MyProcess].Pipes[1].hPumpThread == NULL) {
                GlobalExitCode = EXIT_FAILURE;
                goto drain;
            }

            ProcessInfo[MyProcess].Filename = ArgV[i].StartOfString;

            //
            //  The child process should write to the write handles, but
            //  this process doesn't want those, so we close them immediately
            //  below.
            //

            StartupInfo.dwFlags = STARTF_USESTDHANDLES;
            if (GetStdHandle(STD_OUTPUT_HANDLE) == GetStdHandle(STD_ERROR_HANDLE)) {
                StartupInfo.hStdOutput = WriteOutPipe;
                StartupInfo.hStdError = WriteOutPipe;
            } else {
                StartupInfo.hStdOutput = WriteOutPipe;
                StartupInfo.hStdError = WriteErrPipe;
            }

            if (!CreateProcess(NULL, CompleteString.StartOfString, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &StartupInfo, &ProcessInfo[MyProcess].WindowsProcessInfo)) {
                GlobalExitCode = EXIT_FAILURE;
                goto drain;
            }

            CloseHandle(WriteOutPipe);
            CloseHandle(WriteErrPipe);

            CurrentProcess++;
        }
    }

    //
    //  If we didn't find any source file, just execute the
    //  command verbatim.  Since there's only one child here we just
    //  let it do IO to this process output and error handles.
    //

    if (CurrentProcess == 0) {
        STARTUPINFO StartupInfo;
        DWORD MyProcess = CurrentProcess;

        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);

        if (!CreateProcess(NULL, CommonString.StartOfString, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &StartupInfo, &ProcessInfo[MyProcess].WindowsProcessInfo)) {
            return EXIT_FAILURE;
        }

        CurrentProcess++;
    }

drain:

    //
    //  Now wait on all of the child processes.  Note that if any
    //  fail we will also fail with the same error code.
    //

    if (CurrentProcess > NumberProcesses) {
        CurrentProcess = NumberProcesses;
    }

    do {
        CurrentProcess--;

        if (ProcessInfo[CurrentProcess].ProcessLaunchStarted) {
            ClmpWaitOnProcess(&ProcessInfo[CurrentProcess]);
        }

    } while (CurrentProcess > 0);

    YoriLibFree(ProcessInfo);
    YoriLibFreeStringContents(&CommonString);
    YoriLibFreeStringContents(&CompleteString);
    CloseHandle(hOutputMutex);

    if (GlobalExitCode) {
        ExitProcess(GlobalExitCode);
    }

    //
    //  All of the child processes succeeded, so we did too.
    //

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
