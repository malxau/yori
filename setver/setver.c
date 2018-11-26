/**
 * @file setver/setver.c
 *
 * Yori shell invoke a child process with an explicit Windows version number.
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
CHAR strSetVerHelpText[] =
        "\n"
        "Runs a child program with an explicit Windows version.\n"
        "\n"
        "SETVER [-license] <version> <command>\n";

/**
 Display usage text to the user.
 */
BOOL
SetVerHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("SetVer %i.%i\n"), SETVER_VER_MAJOR, SETVER_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSetVerHelpText);
    return TRUE;
}

/**
 Context to pass around between functions describing the operation to perform.
 */
typedef struct _SETVER_CONTEXT {

    /**
     The major version to report to applications.
     */
    DWORD AppVerMajor;

    /**
     The minor version to report to applications.
     */
    DWORD AppVerMinor;

    /**
     The build number to report to applications.
     */
    DWORD AppBuildNumber;

    /**
     Pointer to process information state about the initial process launched.
     */
    PPROCESS_INFORMATION ProcessInfo;
} SETVER_CONTEXT, *PSETVER_CONTEXT;

/**
 If TRUE, output information related to messages being processed by the
 debugger.
 */
#define SETVER_DEBUG 0

/**
 Apply the requested OS version into the PEB of an opened process.

 @param Context Pointer to the context describing the OS version to apply.

 @param hProcess Handle to the process to apply the OS version into.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetVerApplyVersionToProcess(
    __in PSETVER_CONTEXT Context,
    __in HANDLE hProcess
    )
{
    BOOL TargetProcess32BitPeb;
    LONG Status;
    PROCESS_BASIC_INFORMATION BasicInfo;
    SIZE_T BytesReturned;
    DWORD dwBytesReturned;

    if (DllNtDll.pNtQueryInformationProcess == NULL) {
        return FALSE;
    }

    TargetProcess32BitPeb = YoriLibDoesProcessHave32BitPeb(hProcess);

    Status = DllNtDll.pNtQueryInformationProcess(hProcess, 0, &BasicInfo, sizeof(BasicInfo), &dwBytesReturned);
    if (Status != 0) {
        return FALSE;
    }

#if SETVER_DEBUG
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Peb at %p, Target %i bit PEB\n"), BasicInfo.PebBaseAddress, TargetProcess32BitPeb?32:64);
#endif

    if (TargetProcess32BitPeb) {
        YORI_LIB_PEB32 ProcessPeb;

        if (!ReadProcessMemory(hProcess, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            return FALSE;
        }

        ProcessPeb.OSMajorVersion = Context->AppVerMajor;
        ProcessPeb.OSMinorVersion = Context->AppVerMinor;
        ProcessPeb.OSBuildNumber = (WORD)Context->AppBuildNumber;

        if (!WriteProcessMemory(hProcess, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            return FALSE;
        }
    } else {
        YORI_LIB_PEB64 ProcessPeb;

        if (!ReadProcessMemory(hProcess, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            return FALSE;
        }

        ProcessPeb.OSMajorVersion = Context->AppVerMajor;
        ProcessPeb.OSMinorVersion = Context->AppVerMinor;
        ProcessPeb.OSBuildNumber = (WORD)Context->AppBuildNumber;

        if (!WriteProcessMemory(hProcess, BasicInfo.PebBaseAddress, &ProcessPeb, sizeof(ProcessPeb), &BytesReturned)) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 Pump debug events for child processes and complete when the initial process
 has terminated.

 @param Context Pointer to context describing the operations to perform.

 @return TRUE to indicate successful termination of the initial child process,
         FALSE to indicate failure.
 */
BOOL
SetVerPumpDebugEvents(
    __in PSETVER_CONTEXT Context
    )
{
    while(TRUE) {
        DEBUG_EVENT DbgEvent;
        DWORD dwContinueStatus;

        ZeroMemory(&DbgEvent, sizeof(DbgEvent));
        if (!WaitForDebugEvent(&DbgEvent, INFINITE)) {
            break;
        }

#if SETVER_DEBUG
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("DbgEvent Pid %x Tid %x Event %x\n"), DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.dwDebugEventCode);
#endif

        dwContinueStatus = DBG_CONTINUE;

        switch(DbgEvent.dwDebugEventCode) {
            case CREATE_PROCESS_DEBUG_EVENT:
                if (!SetVerApplyVersionToProcess(Context, DbgEvent.u.CreateProcessInfo.hProcess)) {
                    break;
                }

                CloseHandle(DbgEvent.u.CreateProcessInfo.hFile);
                break;
            case LOAD_DLL_DEBUG_EVENT:
#if SETVER_DEBUG
                if (DbgEvent.u.LoadDll.lpImageName != NULL && DbgEvent.dwProcessId == Context->ProcessInfo->dwProcessId) {
                    SIZE_T BytesReturned;
                    PVOID DllNamePtr;
                    TCHAR DllName[128];

                    if (ReadProcessMemory(Context->ProcessInfo->hProcess, DbgEvent.u.LoadDll.lpImageName, &DllNamePtr, sizeof(DllNamePtr), &BytesReturned)) {
                        if (ReadProcessMemory(Context->ProcessInfo->hProcess, DllNamePtr, &DllName, sizeof(DllName), &BytesReturned)) {
                            if (DbgEvent.u.LoadDll.fUnicode) {
                                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Dll loaded: %s\n"), DllName);
                            } else {
                                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Dll loaded: %hs\n"), DllName);
                            }
                        }
                    }
                }
#endif
                CloseHandle(DbgEvent.u.LoadDll.hFile);
                break;
            case EXCEPTION_DEBUG_EVENT:

                //
                //  Wow64 processes throw a breakpoint once 32 bit code starts
                //  running, and the debugger is expected to handle it.  The
                //  two codes are for breakpoint and x86 breakpoint
                //

                dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                if (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) {

                    dwContinueStatus = DBG_CONTINUE;
                }

                if (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == 0x4000001F) {

                    dwContinueStatus = DBG_CONTINUE;
                }

#if SETVER_DEBUG
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ExceptionCode %x ContinueStatus %x\n"), DbgEvent.u.Exception.ExceptionRecord.ExceptionCode, dwContinueStatus);
#endif

                break;
        }

        ContinueDebugEvent(DbgEvent.dwProcessId, DbgEvent.dwThreadId, dwContinueStatus);
        if (DbgEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT &&
            DbgEvent.dwProcessId == Context->ProcessInfo->dwProcessId) {
            break;
        }
    }

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the setver builtin command.
 */
#define ENTRYPOINT YoriCmd_SETVER
#else
/**
 The main entrypoint for the setver standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the setver cmdlet.

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
    YORI_STRING Executable;
    PYORI_STRING ChildArgs;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    DWORD ExitCode;
    BOOL ArgumentUnderstood;
    DWORD StartArg = 0;
    DWORD AppArg;
    DWORD i;
    YORI_STRING Arg;
    YORI_STRING WinVer;
    LONGLONG llTemp;
    DWORD CharsConsumed;
    SETVER_CONTEXT Context;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SetVerHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("setver: missing argument\n"));
        return EXIT_FAILURE;
    }

    memcpy(&WinVer, &ArgV[StartArg], sizeof(YORI_STRING));

    AppArg = StartArg + 1;
    if (AppArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("setver: missing argument\n"));
        return EXIT_FAILURE;
    }

    Context.AppVerMajor = 6;
    Context.AppVerMinor = 1;
    Context.AppBuildNumber = 7600;

    if (YoriLibStringToNumber(&WinVer, FALSE, &llTemp, &CharsConsumed)) {
        Context.AppVerMajor = (DWORD)llTemp;
        WinVer.LengthInChars -= CharsConsumed;
        WinVer.StartOfString += CharsConsumed;
        if (WinVer.LengthInChars > 0) {
            WinVer.LengthInChars -= 1;
            WinVer.StartOfString += 1;
            if (YoriLibStringToNumber(&WinVer, FALSE, &llTemp, &CharsConsumed)) {
                Context.AppVerMinor = (DWORD)llTemp;
                WinVer.LengthInChars -= CharsConsumed;
                WinVer.StartOfString += CharsConsumed;
                if (WinVer.LengthInChars > 0) {
                    WinVer.LengthInChars -= 1;
                    WinVer.StartOfString += 1;
                    if (YoriLibStringToNumber(&WinVer, FALSE, &llTemp, &CharsConsumed)) {
                        Context.AppBuildNumber = (DWORD)llTemp;
                    }
                }
            }
        }
    }

    YoriLibInitEmptyString(&Executable);
    if (!YoriLibLocateExecutableInPath(&ArgV[AppArg], NULL, NULL, &Executable) ||
        Executable.LengthInChars == 0) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("setver: unable to find executable\n"));
        return EXIT_FAILURE;
    }

    ChildArgs = YoriLibMalloc((ArgC - AppArg) * sizeof(YORI_STRING));
    if (ChildArgs == NULL) {
        YoriLibFreeStringContents(&Executable);
        return EXIT_FAILURE;
    }

    memcpy(&ChildArgs[0], &Executable, sizeof(YORI_STRING));
    if (AppArg + 1 < ArgC) {
        memcpy(&ChildArgs[1], &ArgV[AppArg + 1], (ArgC - AppArg - 1) * sizeof(YORI_STRING));
    }

    if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - AppArg, ChildArgs, TRUE, &CmdLine)) {
        YoriLibFreeStringContents(&Executable);
        YoriLibFree(ChildArgs);
        return EXIT_FAILURE;
    }

    ASSERT(YoriLibIsStringNullTerminated(&CmdLine));

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, DEBUG_PROCESS, NULL, NULL, &StartupInfo, &ProcessInfo)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("setver: execution failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&Executable);
        YoriLibFreeStringContents(&CmdLine);
        YoriLibFree(ChildArgs);
        return EXIT_FAILURE;
    }

    YoriLibFreeStringContents(&Executable);
    YoriLibFreeStringContents(&CmdLine);
    YoriLibFree(ChildArgs);

    Context.ProcessInfo = &ProcessInfo;

    if (!SetVerPumpDebugEvents(&Context)) {
        return EXIT_FAILURE;
    }

    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
    CloseHandle(ProcessInfo.hProcess);
    CloseHandle(ProcessInfo.hThread);

    return ExitCode;
}

// vim:sw=4:ts=4:et:
