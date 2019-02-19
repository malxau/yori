/**
 * @file ydbg/dbg.c
 *
 * Yori shell debug processes
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
CHAR strYDbgHelpText[] =
        "\n"
        "Debugs processes.\n"
        "\n"
        "YDBG [-license] [-d <pid> <file>]\n"
        "\n"
        "   -d             Dump memory from a process to a file\n";

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
        return EXIT_FAILURE;
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
 The set of operations supported by this program.
 */
typedef enum _YDBG_OP {
    YDbgOperationNone = 0,
    YDbgOperationDump = 1
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

    Op = YDbgOperationNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YDbgHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                if (ArgC > i + 2) {
                    Op = YDbgOperationDump;
                    if (!YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed)) {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid pid.\n"), &ArgV[i + 1]);
                        return EXIT_FAILURE;
                    }
                    ProcessPid = (DWORD)llTemp;
                    FileName = &ArgV[i + 2];
                    ArgumentUnderstood = TRUE;
                    i += 2;
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

    if (Op == YDbgOperationDump) {
        YDbgDumpProcess(ProcessPid, FileName);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
