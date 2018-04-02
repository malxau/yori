/**
 * @file clmp/clmp.c
 *
 * Multi processor support for older versions of Visual C++ that don't implement
 * it natively.
 *
 * Copyright (c) 2015-2017 Malcolm J. Smith
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
        "   -MP[n]         Use up to 'n' processes for compilation\n";

/**
 Display the help and license information for this application.
 */
BOOL
ClmpHelp()
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2015-2017"), &License);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Clmp %i.%i\n"), CLMP_VER_MAJOR, CLMP_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &License);
    YoriLibFreeStringContents(&License);
    return TRUE;
}

/**
 The exit code for the application, populated if any child process fails in
 error.
 */
DWORD GlobalExitCode = 0;

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
     A data buffer describing data that has been previously read from the
     pipe.
     */
    PUCHAR ReadBuffer;

    /**
     The number of bytes in ReadBuffer.
     */
    DWORD  ReadBufferSize;

    /**
     A pointless variable declared to make this structure have 8 byte
     alignment.
     */
    DWORD  AlignmentPadding;
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
} CLMP_PROCESS_INFO, *PCLMP_PROCESS_INFO;

/**
 Take the output from a single process and output it from this process.

 @param Process The process to collect and relay output from.

 @param Final TRUE if the process has finished and no further data can
        arrive.  FALSE if the process is still executing.
 */
VOID
ProcessSinglePipeOutput (
    __in PCLMP_PROCESS_INFO Process,
    __in BOOL Final
    )
{
    DWORD BytesToRead = 0;
    DWORD BytesToAllocate;
    DWORD PipeNum;
    PCLMP_PIPE_BUFFER Pipe;
    HANDLE OutHandle;

    //
    //  Each outstanding process has two handles to read from: one
    //  corresponding to its standard output, the other to standard
    //  error.  We read from each and write to the corresponding
    //  handle from this process.
    //
    //  If this seems like a lot of work, the reason is because we
    //  want to write line-by-line to ensure we don't get garbage
    //  output caused by racing child processes.  It also allows us
    //  to give the user more information about what's failing.
    //

    for (PipeNum = 0; PipeNum < sizeof(Process->Pipes)/sizeof(Process->Pipes[0]); PipeNum++) {

        Pipe = &Process->Pipes[PipeNum];
        if (PipeNum == 0) {
            OutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        } else {
            OutHandle = GetStdHandle(STD_ERROR_HANDLE);
        }

        //
        //  Check if we have an initialized process and if we have data to read.
        //

        if (Pipe->Pipe != NULL) {

            BytesToRead = 0;
            PeekNamedPipe( Pipe->Pipe, NULL, 0, NULL, &BytesToRead, NULL );

            if (BytesToRead != 0) {

                PUCHAR NewBuffer;
                DWORD  BytesRead;
                DWORD  ThisChar;

                //
                //  We allocate a buffer for any leftover data (that didn't have a \n)
                //  plus whatever new data we have now.  Copy forward the old data and
                //  read the new data.
                //

                BytesToAllocate = Pipe->ReadBufferSize + BytesToRead;

                NewBuffer = YoriLibMalloc(BytesToAllocate);

                if (Pipe->ReadBufferSize) {
                    memcpy(NewBuffer, Pipe->ReadBuffer, Pipe->ReadBufferSize);
                }

                ReadFile(Pipe->Pipe, NewBuffer + Pipe->ReadBufferSize, BytesToRead, &BytesRead, NULL);

                //
                //  Go look for \n's now.  If we find one, send that data to the correct
                //  handle for this process, and continue searching for more.
                //

                for (ThisChar = 0; ThisChar < BytesToAllocate; ThisChar++) {

                    if (NewBuffer[ThisChar] == '\n') {

                        WriteFile(OutHandle, NewBuffer, ThisChar + 1, &BytesRead, NULL);
                        memmove(NewBuffer, NewBuffer + ThisChar + 1, BytesToAllocate - ThisChar - 1);
                        BytesToAllocate -= ThisChar + 1;
                        ThisChar = 0;
                    }
                }

                //
                //  If we had any previous leftover data, it's moot now.  Tear down
                //  that buffer.
                //

                if (Pipe->ReadBuffer != NULL) {
                    YoriLibFree(Pipe->ReadBuffer);
                    Pipe->ReadBuffer = NULL;
                    Pipe->ReadBufferSize = 0;
                }

                //
                //  If we still have data, either carry it over or if the process is
                //  dead just write what we have.  If we don't have data, tear down
                //  our buffer.
                //

                if (BytesToAllocate > 0) {
                    if (Final) {
                        WriteFile(OutHandle, NewBuffer, BytesToAllocate, &BytesRead, NULL);
                        YoriLibFree(NewBuffer);
                    } else {
                        Pipe->ReadBuffer = NewBuffer;
                        Pipe->ReadBufferSize = BytesToAllocate;
                    }
                } else {
                    YoriLibFree(NewBuffer);
                }
            }
        }
    }
}

/**
 Collect any output from any process and write it as output from this
 process.  This is done to ensure that output from this process is at
 least consistent at the line level.

 @param Processes An array of processes to collect output from.

 @param NumberProcesses The number of processes in the array.
 */
VOID
ProcessPipeOutput (
    PCLMP_PROCESS_INFO Processes,
    DWORD NumberProcesses
    )
{
    DWORD i;

    //
    //  Look for outstanding data in any process and write it out.
    //  

    for (i = 0; i < NumberProcesses; i++) {
        ProcessSinglePipeOutput( &Processes[i], FALSE );
    }
}

/**
 Wait for a child process.  If it failed, we fail.

 @param Process The process to wait for.
 */
VOID
WaitOnProcess (
    __in PCLMP_PROCESS_INFO Process
    )
{
    DWORD ExitCode;
    DWORD PipeNum;

    while (Process->WindowsProcessInfo.hProcess != NULL) {

        HANDLE WaitHandles[3];
        DWORD  HandleNum;

        //
        //  We want to wait for the process to tear down, but we also need
        //  to break out if the process is generating output that we need
        //  to handle, lest we might prevent it from tearing down.
        //

        WaitHandles[0] = Process->WindowsProcessInfo.hProcess;
        WaitHandles[1] = Process->Pipes[0].Pipe;
        WaitHandles[2] = Process->Pipes[1].Pipe;

        if (WaitHandles[2] == NULL && WaitHandles[1] == NULL) {
            HandleNum = 1;
        } else {
            HandleNum = 3;
        }

        HandleNum = WaitForMultipleObjects(HandleNum, WaitHandles, FALSE, INFINITE);
        if (HandleNum > WAIT_OBJECT_0 && HandleNum <= WAIT_OBJECT_0 + 2) {
            ProcessSinglePipeOutput(Process, FALSE);
        } else {
            ProcessSinglePipeOutput(Process, TRUE);
            GetExitCodeProcess( Process->WindowsProcessInfo.hProcess, &ExitCode );

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

            CloseHandle(Process->WindowsProcessInfo.hProcess);
            CloseHandle(Process->WindowsProcessInfo.hThread);
        
            Process->WindowsProcessInfo.hProcess = NULL;
            Process->WindowsProcessInfo.hThread = NULL;

            for (PipeNum = 0; PipeNum < sizeof(Process->Pipes)/sizeof(Process->Pipes[0]); PipeNum++) {

                CloseHandle(Process->Pipes[PipeNum].Pipe);

                Process->Pipes[PipeNum].Pipe = NULL;

                if (Process->Pipes[PipeNum].ReadBuffer) {
                    YoriLibFree(Process->Pipes[PipeNum].ReadBuffer);
                    Process->Pipes[PipeNum].ReadBuffer = NULL;
                }
                Process->Pipes[PipeNum].ReadBufferSize = 0;
            }
        }
    }
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
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    TCHAR szCmdCommon[1024];
    TCHAR szCmdComplete[1024];
    DWORD i, j;
    PCLMP_PROCESS_INFO ProcessInfo;
    DWORD CurrentProcess = 0;
    DWORD NumberProcesses = 0;
    BOOLEAN MultiProcPossible = FALSE;
    BOOLEAN MultiProcNotPossible = FALSE;
    YORI_STRING Arg;
    SYSTEM_INFO SysInfo;

    GetSystemInfo(&SysInfo);

    _tcscpy(szCmdCommon, _T("cl "));

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

            if (YoriLibCompareStringWithLiteralInsensitiveCount(&Arg, _T("?"), 1) == 0) {
                ClmpHelp();
            }

            //
            //  Check for MP switch and adjust the number of processes
            //  as requested.  Don't tell the compiler about this.
            //

            if (YoriLibCompareStringWithLiteralInsensitiveCount(&Arg, _T("MP"), 2) == 0) {
                if (Arg.LengthInChars > 2) {
                    YORI_STRING NumberProcessesString;
                    LONGLONG LlNumberProcesses;
                    DWORD CharsConsumed;

                    YoriLibInitEmptyString(&NumberProcessesString);
                    NumberProcessesString.StartOfString = &Arg.StartOfString[2];
                    NumberProcessesString.LengthInChars = Arg.LengthInChars - 2;
                    if (YoriLibStringToNumber(&NumberProcessesString, FALSE, &LlNumberProcesses, &CharsConsumed) && CharsConsumed > 0) {
                        NumberProcesses = (DWORD)LlNumberProcesses;
                    }
                }

            } else {
                _tcscat(szCmdCommon, _T(" "));
                _tcscat(szCmdCommon, ArgV[i].StartOfString);
            }

            //
            //  Compile without linking - we need this for multiproc, because
            //  if we're linking in the same pass issuing different processes
            //  with different portions would generate the wrong result
            //

            if (YoriLibCompareStringWithLiteralCount(&Arg, _T("c"), 1) == 0) {
                MultiProcPossible = TRUE;
            }

            //
            //  Preprocess to stdout - we can't do this in parallel without
            //  generating garbage
            //

            if (YoriLibCompareStringWithLiteralCount(&Arg, _T("E"), 1) == 0) {
                MultiProcNotPossible = TRUE;
            }

            //
            //  Generating debug into a PDB - we can't do this in parallel
            //  or we'll get sharing violations on the PDB
            //

            if (YoriLibCompareStringWithLiteralCount(&Arg, _T("Z"), 1) == 0) {
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

            if (YoriLibCompareStringWithLiteralCount(&Arg, _T("Y"), 1) == 0) {
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
        NumberProcesses = SysInfo.dwNumberOfProcessors + 1;
    }

    ProcessInfo = YoriLibMalloc(sizeof(CLMP_PROCESS_INFO) * NumberProcesses);

    if (ProcessInfo == NULL) {
        return EXIT_FAILURE;
    }

    ZeroMemory(ProcessInfo, sizeof(CLMP_PROCESS_INFO) * NumberProcesses);

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

            YoriLibSPrintf(szCmdComplete, _T("%s %y"), szCmdCommon, &ArgV[i]);
            ZeroMemory( &StartupInfo, sizeof(StartupInfo) );
            StartupInfo.cb = sizeof(StartupInfo);

            MyProcess %= NumberProcesses;

            //
            //  Look for pending output in any process.  This is a little
            //  gratuitous, but a child may block waiting for us if we don't,
            //  so do it now.
            //

            ProcessPipeOutput(ProcessInfo, NumberProcesses);

            //
            //  If we've run out of processors, wait for the next child
            //  process and reuse its slot.
            //

            if (CurrentProcess >= NumberProcesses) {
                WaitOnProcess( &ProcessInfo[MyProcess] );
                if (GlobalExitCode) {
                    goto drain;
                }
            }

            //
            //  We need to specify security attributes because we want our
            //  standard output and standard error handles to be inherited.
            //

            ZeroMemory( &SecurityAttributes, sizeof(SecurityAttributes) );

            SecurityAttributes.nLength = sizeof(SecurityAttributes);
            SecurityAttributes.bInheritHandle = TRUE;

            //
            //  Create the aforementioned handles.
            //

            if (!CreatePipe( &ProcessInfo[MyProcess].Pipes[0].Pipe, &WriteOutPipe, &SecurityAttributes, 0)) {

                GlobalExitCode = EXIT_FAILURE;
                goto drain;
            }

            if (!CreatePipe( &ProcessInfo[MyProcess].Pipes[1].Pipe, &WriteErrPipe, &SecurityAttributes, 0)) {

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

            if (!CreateProcess( NULL, szCmdComplete, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInfo[MyProcess].WindowsProcessInfo )) {
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

        ZeroMemory( &StartupInfo, sizeof(StartupInfo) );
        StartupInfo.cb = sizeof(StartupInfo);

        if (!CreateProcess( NULL, szCmdCommon, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInfo[MyProcess].WindowsProcessInfo )) {
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

        WaitOnProcess( &ProcessInfo[CurrentProcess] );

    } while (CurrentProcess > 0);

    if (GlobalExitCode) {
        ExitProcess(GlobalExitCode);
    }

    YoriLibFree(ProcessInfo);

    //
    //  All of the child processes succeeded, so we did too.
    //

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
