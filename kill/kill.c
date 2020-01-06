/**
 * @file kill/kill.c
 *
 * Yori shell terminate processes
 *
 * Copyright (c) 2020 Malcolm J. Smith
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
CHAR strKillHelpText[] =
        "\n"
        "Terminate processes.\n"
        "\n"
        "KILL [-license] <pid>|<exename>\n";

/**
 Display usage text to the user.
 */
BOOL
KillHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Kill %i.%02i\n"), KILL_VER_MAJOR, KILL_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strKillHelpText);
    return TRUE;
}

/**
 Terminate a process by process ID.  Note that this routine displays errors
 to the user when the process cannot be opened or terminated.

 @param ProcessPid Specifies the process ID.

 @return TRUE to indicate the process was successfully terminated, FALSE to
         indicate failure.
 */
BOOLEAN
KillTerminateProcessById(
    __in DWORD ProcessPid
    )
{
    HANDLE ProcessHandle;
    DWORD LastError;
    LPTSTR ErrText;

    ProcessHandle = OpenProcess(PROCESS_TERMINATE, FALSE, ProcessPid);
    if (ProcessHandle == NULL) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("kill: could not terminate process %i: %s"), ProcessPid, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    if (!TerminateProcess(ProcessHandle, EXIT_FAILURE)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("kill: could not terminate process %i: %s"), ProcessPid, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(ProcessHandle);
        return FALSE;
    }

    CloseHandle(ProcessHandle);
    return TRUE;
}

/**
 Kill processes with an image name that matches the input string.

 @param ProcessList Pointer to a list of all processes in the system.
 
 @param ProcessName Specifies the process name to terminate.

 @return The number of processes successfully terminated.
 */
__success(return)
DWORD
KillTerminateProcessByName(
    __in PYORI_SYSTEM_PROCESS_INFORMATION ProcessList,
    __in PYORI_STRING ProcessName
    )
{
    PYORI_SYSTEM_PROCESS_INFORMATION CurrentEntry;
    YORI_STRING BaseName;
    YORI_STRING NameToCompare;
    DWORD CharsToCompare;
    DWORD KillCount;

    //
    //  If the process name ends in '*', only compare the characters up to
    //  that point, otherwise compare all.
    //

    YoriLibInitEmptyString(&NameToCompare);
    NameToCompare.StartOfString = ProcessName->StartOfString;
    NameToCompare.LengthInChars = ProcessName->LengthInChars;
    CharsToCompare = (DWORD)-1;

    if (NameToCompare.LengthInChars > 0 &&
        NameToCompare.StartOfString[NameToCompare.LengthInChars - 1] == '*') {

        NameToCompare.LengthInChars--;
        CharsToCompare = NameToCompare.LengthInChars;
    }

    YoriLibInitEmptyString(&BaseName);
    KillCount = 0;

    //
    //  Iterate through the list and kill the process IDs of anything that
    //  matches.
    //

    CurrentEntry = ProcessList;
    do {
        BaseName.StartOfString = CurrentEntry->ImageName;
        BaseName.LengthInChars = CurrentEntry->ImageNameLengthInBytes / sizeof(WCHAR);

        if (YoriLibCompareStringInsensitiveCount(&BaseName, &NameToCompare, CharsToCompare) == 0) {
            if (KillTerminateProcessById((DWORD)CurrentEntry->ProcessId)) {
                KillCount++;
            }
        }

        if (CurrentEntry->NextEntryOffset == 0) {
            break;
        }
        CurrentEntry = YoriLibAddToPointer(CurrentEntry, CurrentEntry->NextEntryOffset);
    } while(TRUE);

    return KillCount;
}


/**
 Load information about all processes currently executing in the system.

 @param ProcessInfo On successful completion, updated to point to a list of
        processes executing within the system.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
KillGetSystemProcessList(
    __out PYORI_SYSTEM_PROCESS_INFORMATION *ProcessInfo
    )
{
    PYORI_SYSTEM_PROCESS_INFORMATION LocalProcessInfo = NULL;
    DWORD BytesReturned;
    DWORD BytesAllocated;
    LONG Status;

    if (DllNtDll.pNtQuerySystemInformation == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("OS support not present\n"));
        return FALSE;
    }

    BytesAllocated = 0;

    do {

        if (LocalProcessInfo != NULL) {
            YoriLibFree(LocalProcessInfo);
        }

        if (BytesAllocated == 0) {
            BytesAllocated = 64 * 1024;
        } else if (BytesAllocated <= 1024 * 1024) {
            BytesAllocated = BytesAllocated * 4;
        } else {
            return FALSE;
        }

        LocalProcessInfo = YoriLibMalloc(BytesAllocated);
        if (LocalProcessInfo == NULL) {
            return FALSE;
        }

        Status = DllNtDll.pNtQuerySystemInformation(SystemProcessInformation, LocalProcessInfo, BytesAllocated, &BytesReturned);
    } while (Status == (LONG)0xc0000004);


    if (Status != 0) {
        YoriLibFree(LocalProcessInfo);
        return FALSE;
    }

    if (BytesReturned == 0) {
        YoriLibFree(LocalProcessInfo);
        return FALSE;
    }

    *ProcessInfo = LocalProcessInfo;
    return TRUE;
}

/**
 Display information about all processes that the current user has access
 to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the kill builtin command.
 */
#define ENTRYPOINT YoriCmd_YKILL
#else
/**
 The main entrypoint for the kill standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the kill cmdlet.

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
    DWORD ProcessPid = 0;
    LONGLONG llTemp;
    DWORD CharsConsumed;
    DWORD KillCount;
    PYORI_SYSTEM_PROCESS_INFORMATION ProcessInfo = NULL;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                KillHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2020"));
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

    if (StartArg == 0 || StartArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("kill: missing argument\n"));
        return EXIT_FAILURE;
    }

    KillCount = 0;
    for (i = StartArg; i < ArgC; i++) {
        if (YoriLibStringToNumber(&ArgV[i], TRUE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
            ProcessPid = (DWORD)llTemp;
            if (KillTerminateProcessById(ProcessPid)) {
                KillCount++;
            }
        } else {
            if (ProcessInfo == NULL) {
                KillGetSystemProcessList(&ProcessInfo);
            }

            if (ProcessInfo != NULL) {
                KillCount += KillTerminateProcessByName(ProcessInfo, &ArgV[i]);
            }
        }
    }

    if (ProcessInfo != NULL) {
        YoriLibFree(ProcessInfo);
    }

    if (KillCount == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("kill: no processes terminated\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
