/**
 * @file ydbg/dbg.c
 *
 * Yori shell debug processes
 *
 * Copyright (c) 2018-2020 Malcolm J. Smith
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
CHAR strYDbgHelpText[] =
        "\n"
        "Debugs processes.\n"
        "\n"
        "YDBG [-license] [-c <file>] [-d <pid> <file>] [-k <file>]\n"
        "\n"
        "   -c             Dump memory from kernel and user processes to a file\n"
        "   -d             Dump memory from a process to a file\n"
        "   -k             Dump memory from kernel to a file\n";

/**
 Display usage text to the user.
 */
BOOL
YDbgHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("YDbg %i.%02i\n"), YDBG_VER_MAJOR, YDBG_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strYDbgHelpText);
    return TRUE;
}

/**
 Write the memory from a process to a dump file.

 @param ProcessPid Specifies the process whose memory should be written.

 @param FileName Specifies the file name to write the memory to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YDbgDumpProcess(
    __in DWORD ProcessPid,
    __in PYORI_STRING FileName
    )
{
    HANDLE ProcessHandle;
    HANDLE FileHandle;
    DWORD LastError;
    LPTSTR ErrText;
    YORI_STRING FullPath;

    YoriLibLoadDbgHelpFunctions();
    if (DllDbgHelp.pMiniDumpWriteDump == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: OS support not present\n"));
        return FALSE;
    }

    ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessPid);
    if (ProcessHandle == NULL) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: OpenProcess of %i failed: %s"), ProcessPid, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);

    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: getfullpathname of %y failed: %s"), FileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(ProcessHandle);
        return FALSE;
    }

    FileHandle = CreateFile(FullPath.StartOfString, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: CreateFile of %y failed: %s"), FullPath.StartOfString, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        CloseHandle(ProcessHandle);
        return FALSE;
    }

    if (!DllDbgHelp.pMiniDumpWriteDump(ProcessHandle, ProcessPid, FileHandle, 2, NULL, NULL, NULL)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: MiniDumpWriteDump failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        CloseHandle(ProcessHandle);
        CloseHandle(FileHandle);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    CloseHandle(ProcessHandle);
    CloseHandle(FileHandle);
    return TRUE;
}

/**
 Write the memory from the kernel to a dump file.

 @param FileName Specifies the file name to write the memory to.

 @param IncludeAll If TRUE, capture user and hypervisor pages in addition
        to kernel pages.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YDbgDumpKernel(
    __in PYORI_STRING FileName,
    __in BOOLEAN IncludeAll
    )
{
    YORI_SYSDBG_LIVEDUMP_CONTROL Ctrl;
    HANDLE FileHandle;
    DWORD LastError;
    DWORD BytesWritten;
    LONG NtStatus;
    LPTSTR ErrText;
    YORI_STRING FullPath;

    ZeroMemory(&Ctrl, sizeof(Ctrl));
    if (DllNtDll.pNtSystemDebugControl == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: OS support not present\n"));
        return FALSE;
    }
    
    if (!YoriLibEnableDebugPrivilege()) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: could not enable debug privilege (access denied)\n"));
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);

    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: getfullpathname of %y failed: %s"), FileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    FileHandle = CreateFile(FullPath.StartOfString, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: CreateFile of %y failed: %s"), FullPath.StartOfString, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    Ctrl.Version = 1;
    Ctrl.File = FileHandle;

    if (IncludeAll) {
        Ctrl.Flags = SYSDBG_LIVEDUMP_FLAG_USER_PAGES;
        Ctrl.AddPagesFlags = SYSDBG_LIVEDUMP_ADD_PAGES_FLAG_HYPERVISOR;
    }

    NtStatus = DllNtDll.pNtSystemDebugControl(37, &Ctrl, sizeof(Ctrl), NULL, 0, &BytesWritten);
    if (NtStatus != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ydbg: NtSystemDebugControl failed: %08x"), NtStatus);
        YoriLibFreeStringContents(&FullPath);
        CloseHandle(FileHandle);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    CloseHandle(FileHandle);
    return TRUE;
}

/**
 The set of operations supported by this program.
 */
typedef enum _YDBG_OP {
    YDbgOperationNone = 0,
    YDbgOperationProcessDump = 1,
    YDbgOperationKernelDump = 2,
    YDbgOperationCompleteDump = 3,
} YDBG_OP;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the ydbg builtin command.
 */
#define ENTRYPOINT YoriCmd_YDBG
#else
/**
 The main entrypoint for the ydbg standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the ydbg cmdlet.

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
    DWORD i;
    DWORD StartArg = 1;
    YORI_STRING Arg;
    YDBG_OP Op;
    DWORD ProcessPid = 0;
    PYORI_STRING FileName = NULL;
    LONGLONG llTemp;
    DWORD CharsConsumed;
    DWORD ExitResult;

    Op = YDbgOperationNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YDbgHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2020"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    Op = YDbgOperationCompleteDump;
                    FileName = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i += 1;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                if (ArgC > i + 2) {
                    Op = YDbgOperationProcessDump;
                    if (!YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed)) {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid pid.\n"), &ArgV[i + 1]);
                        return EXIT_FAILURE;
                    }
                    ProcessPid = (DWORD)llTemp;
                    FileName = &ArgV[i + 2];
                    ArgumentUnderstood = TRUE;
                    i += 2;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("k")) == 0) {
                if (ArgC > i + 1) {
                    Op = YDbgOperationKernelDump;
                    FileName = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i += 1;
                }
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

    if (Op == YDbgOperationNone) {
        YDbgHelp();
        return EXIT_SUCCESS;
    }

    i = StartArg;

    ExitResult = EXIT_SUCCESS;
    if (Op == YDbgOperationProcessDump) {
        if (!YDbgDumpProcess(ProcessPid, FileName)) {
            ExitResult = EXIT_FAILURE;
        }
    } else if (Op == YDbgOperationKernelDump) {
        if (!YDbgDumpKernel(FileName, FALSE)) {
            ExitResult = EXIT_FAILURE;
        }
    } else if (Op == YDbgOperationCompleteDump) {
        if (!YDbgDumpKernel(FileName, TRUE)) {
            ExitResult = EXIT_FAILURE;
        }
    }

    return ExitResult;
}

// vim:sw=4:ts=4:et:
