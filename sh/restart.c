/**
 * @file sh/restart.c
 *
 * Yori shell application recovery on restart
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

#include "yori.h"

/**
 Set to TRUE once the process has been registered for restart processing.
 This is only done once for the lifetime of the process.
 */
BOOL YoriShProcessRegisteredForRestart = FALSE;

/**
 The time that YoriShCleanupStaleRestartStates was last run.  This is used
 to avoid scanning the temp directory too aggressively.
 */
LONGLONG YoriShLastRestartCleanupScan;

#if defined(_MSC_VER) && (_MSC_VER == 1500)
#pragma warning(push)
#pragma warning(disable: 6309 6387) // GetTempPath is mis-annotated in old SDKs
#endif


#if defined(_MSC_VER) && (_MSC_VER == 1500)
#pragma warning(pop)
#endif

/**
 Scan restart files in the temp directory and delete any that haven't been
 written in a long time and there are no active processes with a matching
 PID.

 Note that this function is called when saving a restart file, which happens
 after the process has been running for tens of seconds.  The assumption is
 that if the machine did recently restart and the user was logged on, if
 this process is running, so are other restarted processes.
 */
VOID
YoriShCleanupStaleRestartStates(VOID)
{
    YORI_STRING RestartFileName;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    LONGLONG LongProcessId;
    DWORD FileNameBufferLength;
    DWORD ProcessId;
    DWORD Index;
    DWORD CharsConsumed;
    DWORD Err;
    LONGLONG CurrentTime;
    LARGE_INTEGER FileTime;
    LONGLONG TimeDelta;
    YORI_STRING FileName;
    YORI_STRING PidString;
    HANDLE ProcessHandle;
    BOOLEAN ProcessIsRunning;
    BOOLEAN FileTimeExpired;

    //
    //  Check for OS restart support.  Don't scan if the OS couldn't support
    //  the feature (we should never have created these files.)
    //

    if (DllKernel32.pRegisterApplicationRestart == NULL ||
        DllKernel32.pGetConsoleScreenBufferInfoEx == NULL ||
        DllKernel32.pGetCurrentConsoleFontEx == NULL) {

        return;
    }

    //
    //  If this scan has run in the last hour, don't run it again.
    //

    CurrentTime = YoriLibGetSystemTimeAsInteger();
    TimeDelta = 10 * 1000 * 1000 * 60;
    TimeDelta = TimeDelta * 60;
    if (YoriShLastRestartCleanupScan + TimeDelta > CurrentTime) {
        return;
    }


    //
    //  Allocate a path to the temp directory with enough space to refer to
    //  any restart file containing a process ID
    //

    YoriLibInitEmptyString(&RestartFileName);
    FileNameBufferLength = sizeof("\\yori-restart-.ini") + 2 * sizeof(DWORD);
    if (!YoriLibGetTempPath(&RestartFileName, FileNameBufferLength)) {
        return;
    }
    ASSERT(RestartFileName.LengthInChars > 0 &&
           YoriLibIsSep(RestartFileName.StartOfString[RestartFileName.LengthInChars - 1]));

    //
    //  Look for any existing restart file in the directory
    //

    YoriLibSPrintf(RestartFileName.StartOfString + RestartFileName.LengthInChars,
                   _T("\\yori-restart-*"));

    FindHandle = FindFirstFile(RestartFileName.StartOfString, &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE) {
        YoriLibFreeStringContents(&RestartFileName);
        return;
    }

    YoriLibInitEmptyString(&FileName);
    YoriLibInitEmptyString(&PidString);

    while (TRUE) {

        FileName.StartOfString = FindData.cFileName;
        FileName.LengthInChars = _tcslen(FileName.StartOfString);

        //
        //  Check if the file has not been written in a long time.  The code
        //  below currently checks for one day.
        //

        FileTimeExpired = FALSE;

        FileTime.HighPart = FindData.ftLastWriteTime.dwHighDateTime;
        FileTime.LowPart = FindData.ftLastWriteTime.dwLowDateTime;

        TimeDelta = 10 * 1000 * 1000 * 60;
        TimeDelta = TimeDelta * 60 * 24;

        if (FileTime.QuadPart + TimeDelta < CurrentTime) {
            FileTimeExpired = TRUE;
        }

        //
        //  Check if the file name matches.  FindFirstFile takes a search
        //  criteria but this is to validate that the file name in this
        //  buffer matches what we asked for.  Extract the PID, and see if
        //  it can be opened.  If the open fails with invalid parameter,
        //  interpret that as meaning that the process is not running.
        //
        //  Note that the restart files are in the user's temp directory,
        //  so ordinarily these processes would run in the same security
        //  domain.
        //

        ProcessId = 0;
        ProcessIsRunning = TRUE;
        if (FileTimeExpired &&
            YoriLibCompareStringWithLiteralInsensitiveCount(&FileName, _T("yori-restart-"), sizeof("yori-restart-") - 1) == 0) {

            //
            //  Find the process ID in the file name.
            //

            PidString.StartOfString = FileName.StartOfString + sizeof("yori-restart-") - 1;
            PidString.LengthInChars = FileName.LengthInChars - sizeof("yori-restart-") + 1;

            for (Index = 0; Index < PidString.LengthInChars; Index++) {
                if (PidString.StartOfString[Index] == '.') {
                    PidString.LengthInChars = Index;
                    break;
                }
            }

            if (YoriLibStringToNumberSpecifyBase(&PidString, 16, FALSE, &LongProcessId, &CharsConsumed) &&
                CharsConsumed > 0) {

                ProcessId = (DWORD)LongProcessId;

                ProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, ProcessId);
                if (ProcessHandle == NULL) {
                    Err = GetLastError();
                    if (Err == ERROR_INVALID_PARAMETER) {
                        ProcessIsRunning = FALSE;
                    }
                } else {
                    CloseHandle(ProcessHandle);
                }
            }
        }

        if (FileTimeExpired && !ProcessIsRunning) {
            if (FileName.LengthInChars + 1 < FileNameBufferLength) {
                YoriLibSPrintf(RestartFileName.StartOfString + RestartFileName.LengthInChars,
                               _T("%y"), &FileName);
                DeleteFile(RestartFileName.StartOfString);
            }
        }

        if (!FindNextFile(FindHandle, &FindData)) {
            break;
        }
    }

    FindClose(FindHandle);
    YoriLibFreeStringContents(&RestartFileName);

    YoriShLastRestartCleanupScan = CurrentTime;
}


/**
 Try to save the current state of the process so that it can be recovered
 from this state after a subsequent unexpected termination.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
DWORD WINAPI
YoriShSaveRestartStateWorker(
    __in PVOID Ignored
    )
{
    YORI_CONSOLE_SCREEN_BUFFER_INFOEX ScreenBufferInfo;
    YORI_CONSOLE_FONT_INFOEX FontInfo;

    YORI_STRING WriteBuffer;
    YORI_STRING RestartFileName;
    YORI_STRING RestartBufferFileName;
    YORI_STRING Env;
    LPTSTR Comma;
    DWORD Count;
    DWORD LineCount;

    UNREFERENCED_PARAMETER(Ignored);

    //
    //  The restart APIs are available in Vista+.  By happy coincidence, so
    //  is GetConsoleScreenBufferInfoEx, so if either don't exist, just give
    //  up; this allows us to keep a single piece of logic for recording
    //  console state.
    //

    if (DllKernel32.pRegisterApplicationRestart == NULL ||
        DllKernel32.pGetConsoleScreenBufferInfoEx == NULL ||
        DllKernel32.pGetCurrentConsoleFontEx == NULL ||
        DllKernel32.pGetConsoleWindow == NULL ||
        DllKernel32.pWritePrivateProfileStringW == NULL) {

        return 0;
    }

    //
    //  If the user hasn't opted in by setting YORIAUTORESTART, do nothing.
    //

    Count = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIAUTORESTART"), NULL, 0, NULL);
    if (Count == 0) {
        return 0;
    }

    YoriLibLoadUser32Functions();

    if (!YoriLibAllocateString(&RestartFileName, Count)) {
        YoriLibFreeStringContents(&RestartFileName);
        return 0;
    }

    RestartFileName.LengthInChars = YoriShGetEnvironmentVariableWithoutSubstitution(_T("YORIAUTORESTART"), RestartFileName.StartOfString, RestartFileName.LengthAllocated, NULL);
    if (RestartFileName.LengthInChars == 0) {
        YoriLibFreeStringContents(&RestartFileName);
        return 0;
    }

    //
    //  If the user has specified a line count in YORIAUTORESTART, fish it out
    //  and convert it to a number.
    //

    LineCount = 0;
    Comma = YoriLibFindLeftMostCharacter(&RestartFileName, ',');
    if (Comma != NULL) {
        YORI_STRING LineCountAsString;
        DWORD CharsConsumed;
        LONGLONG llTemp;

        YoriLibInitEmptyString(&LineCountAsString);
        LineCountAsString.StartOfString = Comma + 1;
        LineCountAsString.LengthInChars = RestartFileName.LengthInChars - (DWORD)(Comma - RestartFileName.StartOfString + 1);

        if (YoriLibStringToNumber(&LineCountAsString, TRUE, &llTemp, &CharsConsumed) &&
            CharsConsumed > 0) {

            LineCount = (DWORD)llTemp;
        }

        RestartFileName.LengthInChars = (DWORD)(Comma - RestartFileName.StartOfString);
        Comma[0] = '\0';
    }

    if (YoriLibCompareStringWithLiteral(&RestartFileName, _T("1")) != 0) {
        YoriLibFreeStringContents(&RestartFileName);
        return 0;
    }
    YoriLibFreeStringContents(&RestartFileName);

    //
    //  Query console window size and state, and save it.
    //

    ZeroMemory(&ScreenBufferInfo, sizeof(ScreenBufferInfo));
    ScreenBufferInfo.cbSize = sizeof(ScreenBufferInfo);

    if (!DllKernel32.pGetConsoleScreenBufferInfoEx(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenBufferInfo)) {
        return 0;
    }

    if (!YoriLibGetTempPath(&RestartFileName, sizeof("\\yori-restart-.ini") + 2 * sizeof(DWORD))) {
        return 0;
    }

    YoriLibSPrintf(RestartFileName.StartOfString + RestartFileName.LengthInChars,
                   _T("\\yori-restart-%x.ini"),
                   GetCurrentProcessId());

    if (!YoriLibAllocateString(&WriteBuffer, 64 * 1024)) {
        YoriLibFreeStringContents(&RestartFileName);
        return 0;
    }

    YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), ScreenBufferInfo.dwSize.X);
    DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("BufferWidth"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
    YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), ScreenBufferInfo.dwSize.Y);
    DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("BufferHeight"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
    YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), ScreenBufferInfo.srWindow.Right - ScreenBufferInfo.srWindow.Left + 1);
    DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("WindowWidth"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
    YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), ScreenBufferInfo.srWindow.Bottom - ScreenBufferInfo.srWindow.Top + 1);
    DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("WindowHeight"), WriteBuffer.StartOfString, RestartFileName.StartOfString);

    YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), YoriLibVtGetDefaultColor());
    DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("DefaultColor"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
    YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), ScreenBufferInfo.wPopupAttributes);
    DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("PopupColor"), WriteBuffer.StartOfString, RestartFileName.StartOfString);

    for (Count = 0; Count < sizeof(ScreenBufferInfo.ColorTable)/sizeof(ScreenBufferInfo.ColorTable[0]); Count++) {
        TCHAR ColorName[32];
        YoriLibSPrintf(ColorName, _T("Color%i"), Count);
        YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), ScreenBufferInfo.ColorTable[Count]);
        DllKernel32.pWritePrivateProfileStringW(_T("Window"), ColorName, WriteBuffer.StartOfString, RestartFileName.StartOfString);
    }

    //
    //  Query window position and save it.  Note this is not trying to save
    //  the window size (in GUI terms) since the window size can be determined
    //  by font and console size instead.
    //

    if (DllUser32.pGetWindowRect != NULL) {
        RECT WindowRect;

        if (DllUser32.pGetWindowRect(DllKernel32.pGetConsoleWindow(), &WindowRect)) {
            YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), WindowRect.left);
            DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("WindowLeft"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
            YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), WindowRect.top);
            DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("WindowTop"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
        }
    }

    //
    //  Query the window title and save it.
    //

    WriteBuffer.LengthInChars = GetConsoleTitle(WriteBuffer.StartOfString, 4095);
    if (WriteBuffer.LengthInChars > 0) {
        DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("Title"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Error getting window title: %i\n"), GetLastError());
    }

    //
    //  Query window font information and save it.
    //

    ZeroMemory(&FontInfo, sizeof(FontInfo));
    FontInfo.cbSize = sizeof(FontInfo);
    if (DllKernel32.pGetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &FontInfo)) {
        YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), FontInfo.nFont);
        DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("FontIndex"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
        YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), FontInfo.dwFontSize.X);
        DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("FontWidth"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
        YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), FontInfo.dwFontSize.Y);
        DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("FontHeight"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
        YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), FontInfo.FontFamily);
        DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("FontFamily"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
        YoriLibSPrintf(WriteBuffer.StartOfString, _T("%i"), FontInfo.FontWeight);
        DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("FontWeight"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
        DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("FontName"), FontInfo.FaceName, RestartFileName.StartOfString);
    }

    //
    //  Query the current directory and save it.
    //

    WriteBuffer.LengthInChars = GetCurrentDirectory(WriteBuffer.LengthAllocated, WriteBuffer.StartOfString);
    if (WriteBuffer.LengthInChars > 0 && WriteBuffer.LengthInChars < WriteBuffer.LengthAllocated) {
        DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("CurrentDirectory"), WriteBuffer.StartOfString, RestartFileName.StartOfString);
    }

    //
    //  Write the current environment
    //

    if (YoriLibGetEnvironmentStrings(&Env)) {
        LPTSTR ThisPair;
        LPTSTR ThisVar;
        LPTSTR ThisValue;

        ThisPair = Env.StartOfString;
        while (*ThisPair != '\0') {
            ThisVar = ThisPair;
            ThisPair += _tcslen(ThisPair) + 1;

            if (ThisVar[0] != '=') {
                ThisValue = _tcschr(ThisVar, '=');
                if (ThisValue) {
                    ThisValue[0] = '\0';
                    ThisValue++;

                    DllKernel32.pWritePrivateProfileStringW(_T("Environment"), ThisVar, ThisValue, RestartFileName.StartOfString);

                    ThisValue--;
                    ThisValue[0] = '=';
                }
            }
        }

        //
        //  With the "regular" environment done, go through and write a new
        //  section for current directories on alternate drives.  These are
        //  part of the environment but inexpressible in the INI format as
        //  regular entries, so they get their own section.
        //

        ThisPair = Env.StartOfString;
        while (*ThisPair != '\0') {
            ThisVar = ThisPair;
            ThisPair += _tcslen(ThisPair) + 1;

            if (ThisVar[0] == '=' &&
                ((ThisVar[1] >= 'A' && ThisVar[1] <= 'Z') ||
                 (ThisVar[1] >= 'a' && ThisVar[1] <= 'z')) &&
                ThisVar[2] == ':' &&
                ThisVar[3] == '=') {

                ThisValue = &ThisVar[3];
                ThisVar++;
                ThisValue[0] = '\0';
                ThisValue++;

                DllKernel32.pWritePrivateProfileStringW(_T("CurrentDirectories"), ThisVar, ThisValue, RestartFileName.StartOfString);

                ThisValue--;
                ThisValue[0] = '=';
            }
        }

        YoriLibFreeStringContents(&Env);
    }

    //
    //  Write the current aliases
    //

    YoriLibInitEmptyString(&Env);
    if (YoriShGetAliasStrings(YORI_SH_GET_ALIAS_STRINGS_INCLUDE_USER, &Env)) {
        LPTSTR ThisPair;
        LPTSTR ThisVar;
        LPTSTR ThisValue;

        ThisPair = Env.StartOfString;
        while (*ThisPair != '\0') {
            ThisVar = ThisPair;
            ThisPair += _tcslen(ThisPair) + 1;
            if (ThisVar[0] != '=') {
                ThisValue = _tcschr(ThisVar, '=');
                if (ThisValue) {
                    ThisValue[0] = '\0';
                    ThisValue++;

                    DllKernel32.pWritePrivateProfileStringW(_T("Aliases"), ThisVar, ThisValue, RestartFileName.StartOfString);
                }
            }
        }

        YoriLibFreeStringContents(&Env);
    }

    //
    //  Write history.  We only care about values here but want to maintain
    //  sort order, so zero prefix count to use as a key.
    //

    if (YoriShGetHistoryStrings(100, &Env)) {
        LPTSTR ThisValue;

        ThisValue = Env.StartOfString;
        Count = 1;
        while (*ThisValue != '\0') {
            YoriLibSPrintf(WriteBuffer.StartOfString, _T("%03i"), Count);
            DllKernel32.pWritePrivateProfileStringW(_T("History"), WriteBuffer.StartOfString, ThisValue, RestartFileName.StartOfString);
            ThisValue += _tcslen(ThisValue) + 1;
            Count++;
        }

        YoriLibFreeStringContents(&Env);
    }

    //
    //  Write the window contents
    //

    if (YoriLibAllocateString(&RestartBufferFileName, RestartFileName.LengthAllocated)) {

        HANDLE hBufferFile;

        memcpy(RestartBufferFileName.StartOfString, RestartFileName.StartOfString, RestartFileName.LengthInChars * sizeof(TCHAR));
        RestartBufferFileName.LengthInChars = RestartFileName.LengthInChars;
        YoriLibSPrintf(RestartBufferFileName.StartOfString + RestartBufferFileName.LengthInChars,
                       _T("\\yori-restart-%x.txt"),
                       GetCurrentProcessId());

        hBufferFile = CreateFile(RestartBufferFileName.StartOfString,
                                 GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_DELETE,
                                 NULL,
                                 CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);

        if (hBufferFile != INVALID_HANDLE_VALUE) {
            YoriLibRewriteConsoleContents(hBufferFile, LineCount, 0);
            DllKernel32.pWritePrivateProfileStringW(_T("Window"), _T("Contents"), RestartBufferFileName.StartOfString, RestartFileName.StartOfString);
            CloseHandle(hBufferFile);
        }

        YoriLibFreeStringContents(&RestartBufferFileName);
    }

    //
    //  Register the process to be restarted on failure
    //

    if (!YoriShProcessRegisteredForRestart) {
        YoriLibSPrintf(WriteBuffer.StartOfString, _T("-restart %x"), GetCurrentProcessId());

        DllKernel32.pRegisterApplicationRestart(WriteBuffer.StartOfString, 0);
        YoriShProcessRegisteredForRestart = TRUE;
    }

    YoriLibFreeStringContents(&RestartFileName);
    YoriLibFreeStringContents(&WriteBuffer);

    YoriShCleanupStaleRestartStates();

    return 0;
}

/**
 Try to save the current state of the process so that it can be recovered
 from this state after a subsequent unexpected termination.  This operation
 occurs on a background thread and this function makes no attempt to wait for
 completion or determine success or failure.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShSaveRestartState(VOID)
{
    DWORD ThreadId;

    //
    //  If there's a previous restart save thread, see if it's completed.
    //  If so, close the handle and prepare for a new thread.  If it's
    //  still active just return since that implies a save is in progress.
    //

    if (YoriShGlobal.RestartSaveThread != NULL) {
        if (WaitForSingleObject(YoriShGlobal.RestartSaveThread, 0) == WAIT_OBJECT_0) {
            CloseHandle(YoriShGlobal.RestartSaveThread);
            YoriShGlobal.RestartSaveThread = NULL;
        } else {
            return FALSE;
        }
    }

    YoriShGlobal.RestartSaveThread = CreateThread(NULL, 0, YoriShSaveRestartStateWorker, NULL, 0, &ThreadId);

    return TRUE;
}

/**
 Check if a restart thread has been created, and if it has finished.  If it
 has finished, close the handle to allow the thread to be cleaned up from
 the system.
 */
VOID
YoriShCleanupRestartSaveThreadIfCompleted(VOID)
{
    if (YoriShGlobal.RestartSaveThread != NULL) {
        if (WaitForSingleObject(YoriShGlobal.RestartSaveThread, 0) == WAIT_OBJECT_0) {
            CloseHandle(YoriShGlobal.RestartSaveThread);
            YoriShGlobal.RestartSaveThread = NULL;
        }
    }
}

/**
 Try to recover a previous process ID that terminated unexpectedly.

 @param ProcessId Pointer to the process ID to try to recover.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShLoadSavedRestartState(
    __in PYORI_STRING ProcessId
    )
{
    YORI_STRING RestartFileName;
    YORI_STRING ReadBuffer;
    YORI_CONSOLE_SCREEN_BUFFER_INFOEX ScreenBufferInfo;
    DWORD Count;
    YORI_CONSOLE_FONT_INFOEX FontInfo;
    HWND ConsoleWindow;
    INT WindowLeft;
    INT WindowTop;

    YoriLibLoadUser32Functions();

    if (DllKernel32.pSetConsoleScreenBufferInfoEx == NULL ||
        DllKernel32.pSetCurrentConsoleFontEx == NULL ||
        DllKernel32.pGetConsoleWindow == NULL ||
        DllKernel32.pGetPrivateProfileIntW == NULL ||
        DllKernel32.pGetPrivateProfileSectionW == NULL ||
        DllKernel32.pGetPrivateProfileStringW == NULL ||
        DllUser32.pGetWindowRect == NULL ||
        DllUser32.pSetWindowPos == NULL) {

        return FALSE;
    }

    if (!YoriLibGetTempPath(&RestartFileName, sizeof("\\yori-restart-.ini") + 2 * sizeof(DWORD))) {
        return FALSE;
    }

    ZeroMemory(&ScreenBufferInfo, sizeof(ScreenBufferInfo));
    ScreenBufferInfo.cbSize = sizeof(ScreenBufferInfo);

    YoriLibSPrintf(RestartFileName.StartOfString + RestartFileName.LengthInChars,
                   _T("\\yori-restart-%y.ini"),
                   ProcessId);

    //
    //  Read and populate window settings
    //

    ScreenBufferInfo.dwSize.X = (USHORT)DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("BufferWidth"), 0, RestartFileName.StartOfString);
    ScreenBufferInfo.dwSize.Y = (USHORT)DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("BufferHeight"), 0, RestartFileName.StartOfString);

    if (ScreenBufferInfo.dwSize.X == 0 || ScreenBufferInfo.dwSize.Y == 0) {
        YoriLibFreeStringContents(&RestartFileName);
        return FALSE;
    }

    ScreenBufferInfo.dwMaximumWindowSize.X = (USHORT)DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("WindowWidth"), 0, RestartFileName.StartOfString);
    ScreenBufferInfo.dwMaximumWindowSize.Y = (USHORT)DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("WindowHeight"), 0, RestartFileName.StartOfString);

    if (ScreenBufferInfo.dwMaximumWindowSize.X == 0 || ScreenBufferInfo.dwMaximumWindowSize.Y == 0) {
        YoriLibFreeStringContents(&RestartFileName);
        return FALSE;
    }

    ScreenBufferInfo.srWindow.Bottom = (USHORT)(ScreenBufferInfo.dwMaximumWindowSize.Y);
    ScreenBufferInfo.srWindow.Right = (USHORT)(ScreenBufferInfo.dwMaximumWindowSize.X);

    ScreenBufferInfo.wAttributes = (USHORT)DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("DefaultColor"), 0, RestartFileName.StartOfString);
    ScreenBufferInfo.wPopupAttributes = (USHORT)DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("PopupColor"), 0, RestartFileName.StartOfString);

    for (Count = 0; Count < sizeof(ScreenBufferInfo.ColorTable)/sizeof(ScreenBufferInfo.ColorTable[0]); Count++) {
        TCHAR ColorName[32];
        YoriLibSPrintf(ColorName, _T("Color%i"), Count);
        ScreenBufferInfo.ColorTable[Count] = DllKernel32.pGetPrivateProfileIntW(_T("Window"), ColorName, 0, RestartFileName.StartOfString);
    }

    YoriLibVtSetDefaultColor(ScreenBufferInfo.wAttributes);

    //
    //  Apparently GetConsoleTitle can't tell us how much memory it needs, but
    //  it needs less than 64Kb
    //

    if (!YoriLibAllocateString(&ReadBuffer, 64 * 1024)) {
        YoriLibFreeStringContents(&RestartFileName);
        return FALSE;
    }

    //
    //  Read and populate window fonts
    //

    ZeroMemory(&FontInfo, sizeof(FontInfo));
    FontInfo.cbSize = sizeof(FontInfo);
    FontInfo.nFont = DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("FontIndex"), 0, RestartFileName.StartOfString);
    FontInfo.dwFontSize.X = (USHORT)DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("FontWidth"), 0, RestartFileName.StartOfString);
    FontInfo.dwFontSize.Y = (USHORT)DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("FontHeight"), 0, RestartFileName.StartOfString);
    FontInfo.FontFamily = DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("FontFamily"), 0, RestartFileName.StartOfString);
    FontInfo.FontWeight = DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("FontWeight"), 0, RestartFileName.StartOfString);
    DllKernel32.pGetPrivateProfileStringW(_T("Window"), _T("FontName"), _T(""), FontInfo.FaceName, sizeof(FontInfo.FaceName)/sizeof(FontInfo.FaceName[0]), RestartFileName.StartOfString);

    if (FontInfo.dwFontSize.X > 0 && FontInfo.dwFontSize.Y > 0 && FontInfo.FontWeight > 0) {
        DllKernel32.pSetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &FontInfo);
    }

    DllKernel32.pSetConsoleScreenBufferInfoEx(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenBufferInfo);

    //
    //  Read and set window position
    //

    ConsoleWindow = DllKernel32.pGetConsoleWindow();
    WindowLeft = DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("WindowLeft"), INT_MAX, RestartFileName.StartOfString);
    WindowTop = DllKernel32.pGetPrivateProfileIntW(_T("Window"), _T("WindowTop"), INT_MAX, RestartFileName.StartOfString);

    if (WindowLeft != INT_MAX &&
        WindowTop != INT_MAX) {

        DllUser32.pSetWindowPos(ConsoleWindow, NULL, WindowLeft, WindowTop, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    }

    //
    //  Read and populate the window title
    //

    DllKernel32.pGetPrivateProfileStringW(_T("Window"), _T("Title"), _T("Yori"), ReadBuffer.StartOfString, ReadBuffer.LengthAllocated, RestartFileName.StartOfString);
    SetConsoleTitle(ReadBuffer.StartOfString);

    //
    //  Read and populate the current directory
    //

    ReadBuffer.LengthInChars = DllKernel32.pGetPrivateProfileStringW(_T("Window"), _T("CurrentDirectory"), _T(""), ReadBuffer.StartOfString, ReadBuffer.LengthAllocated, RestartFileName.StartOfString);
    if (ReadBuffer.LengthInChars > 0) {
        YoriLibSetCurrentDirectory(&ReadBuffer);
    }

    //
    //  Populate the environment.
    //

    ReadBuffer.LengthInChars = DllKernel32.pGetPrivateProfileSectionW(_T("Environment"), ReadBuffer.StartOfString, ReadBuffer.LengthAllocated, RestartFileName.StartOfString);

    if (ReadBuffer.LengthInChars > 0) {
        LPTSTR ThisPair;
        LPTSTR ThisVar;
        LPTSTR ThisValue;

        ThisPair = ReadBuffer.StartOfString;
        while (*ThisPair != '\0') {
            ThisVar = ThisPair;
            ThisPair += _tcslen(ThisPair) + 1;
            if (ThisVar[0] != '=') {
                ThisValue = _tcschr(ThisVar, '=');
                if (ThisValue) {
                    ThisValue[0] = '\0';
                    ThisValue++;

                    SetEnvironmentVariable(ThisVar, ThisValue);
                }
            }
        }
    }

    //
    //  Populate current directories.
    //

    ReadBuffer.LengthInChars = DllKernel32.pGetPrivateProfileSectionW(_T("CurrentDirectories"), ReadBuffer.StartOfString, ReadBuffer.LengthAllocated, RestartFileName.StartOfString);

    if (ReadBuffer.LengthInChars > 0) {
        LPTSTR ThisPair;
        LPTSTR ThisVar;
        LPTSTR ThisValue;
        TCHAR DriveLetterBuffer[sizeof("=C:")];

        ThisPair = ReadBuffer.StartOfString;
        while (*ThisPair != '\0') {
            ThisVar = ThisPair;
            ThisPair += _tcslen(ThisPair) + 1;
            if (ThisVar[0] != '=') {
                ThisValue = _tcschr(ThisVar, '=');
                if (ThisValue) {
                    ThisValue[0] = '\0';
                    ThisValue++;

                    DriveLetterBuffer[0] = '=';
                    DriveLetterBuffer[1] = ThisVar[0];
                    DriveLetterBuffer[2] = ':';
                    DriveLetterBuffer[3] = '\0';

                    SetEnvironmentVariable(DriveLetterBuffer, ThisValue);
                }
            }
        }
    }

    //
    //  Populate aliases
    //

    ReadBuffer.LengthInChars = DllKernel32.pGetPrivateProfileSectionW(_T("Aliases"), ReadBuffer.StartOfString, ReadBuffer.LengthAllocated, RestartFileName.StartOfString);

    if (ReadBuffer.LengthInChars > 0) {
        LPTSTR ThisPair;
        LPTSTR ThisVar;
        LPTSTR ThisValue;

        ThisPair = ReadBuffer.StartOfString;
        while (*ThisPair != '\0') {
            ThisVar = ThisPair;
            ThisPair += _tcslen(ThisPair) + 1;
            if (ThisVar[0] != '=') {
                ThisValue = _tcschr(ThisVar, '=');
                if (ThisValue) {
                    ThisValue[0] = '\0';
                    ThisValue++;

                    YoriShAddAliasLiteral(ThisVar, ThisValue, FALSE);
                }
            }
        }
    }

    //
    //  Populate history
    //

    ReadBuffer.LengthInChars = DllKernel32.pGetPrivateProfileSectionW(_T("History"), ReadBuffer.StartOfString, ReadBuffer.LengthAllocated, RestartFileName.StartOfString);

    if (ReadBuffer.LengthInChars > 0) {
        LPTSTR ThisPair;
        LPTSTR ThisVar;
        LPTSTR ThisValue;
        YORI_STRING ThisEntry;
        DWORD ValueLength;

        YoriShInitHistory();

        ThisPair = ReadBuffer.StartOfString;
        while (*ThisPair != '\0') {
            ThisVar = ThisPair;
            ThisPair += _tcslen(ThisPair) + 1;
            if (ThisVar[0] != '=') {
                ThisValue = _tcschr(ThisVar, '=');
                if (ThisValue) {
                    ThisValue[0] = '\0';
                    ThisValue++;
                    ValueLength = _tcslen(ThisValue);
                    if (YoriLibAllocateString(&ThisEntry, ValueLength + 1)) {
                        memcpy(ThisEntry.StartOfString, ThisValue, (ValueLength + 1) * sizeof(TCHAR));
                        ThisEntry.LengthInChars = ValueLength;

                        YoriShAddToHistory(&ThisEntry, FALSE);
                        YoriLibFreeStringContents(&ThisEntry);
                    }
                }
            }
        }
    }

    //
    //  Populate window contents
    //

    ReadBuffer.LengthInChars = DllKernel32.pGetPrivateProfileStringW(_T("Window"), _T("Contents"), _T(""), ReadBuffer.StartOfString, ReadBuffer.LengthAllocated, RestartFileName.StartOfString);

    if (ReadBuffer.LengthInChars > 0) {
        HANDLE hBufferFile;

        hBufferFile = CreateFile(ReadBuffer.StartOfString,
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_DELETE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);

        if (hBufferFile != INVALID_HANDLE_VALUE) {
            YORI_STRING LineString;
            PVOID LineContext = NULL;
            YoriLibInitEmptyString(&LineString);
            while (TRUE) {
                if (!YoriLibReadLineToString(&LineString, &LineContext, hBufferFile)) {
                    break;
                }

                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &LineString);
            }
            YoriLibLineReadCloseOrCache(LineContext);
            YoriLibFreeStringContents(&LineString);
            CloseHandle(hBufferFile);
        }
    }

    YoriLibFreeStringContents(&ReadBuffer);
    YoriLibFreeStringContents(&RestartFileName);

    //
    //  Reload any state next time it's requested.
    //

    YoriShGlobal.EnvironmentGeneration++;

    return TRUE;
}

/**
 Delete any restart information from disk.

 @param ProcessId Optionally points to a process ID corresponding to the
        information to remove.  This is used after recovering that process ID.
        If this value is NULL, the current process ID is used.
 */
VOID
YoriShDiscardSavedRestartState(
    __in_opt PYORI_STRING ProcessId
    )
{
    YORI_STRING RestartFileName;

    if (YoriShGlobal.RestartSaveThread != NULL) {
        WaitForSingleObject(YoriShGlobal.RestartSaveThread, INFINITE);
        CloseHandle(YoriShGlobal.RestartSaveThread);
        YoriShGlobal.RestartSaveThread = NULL;
    }

    if (!YoriLibGetTempPath(&RestartFileName, sizeof("\\yori-restart-.ini") + 2 * sizeof(DWORD))) {
        return;
    }

    if (ProcessId != NULL) {
        YoriLibSPrintf(RestartFileName.StartOfString + RestartFileName.LengthInChars,
                       _T("\\yori-restart-%y.ini"),
                       ProcessId);
    } else {
        YoriLibSPrintf(RestartFileName.StartOfString + RestartFileName.LengthInChars,
                       _T("\\yori-restart-%x.ini"),
                       GetCurrentProcessId());
    }

    DeleteFile(RestartFileName.StartOfString);

    if (ProcessId != NULL) {
        YoriLibSPrintf(RestartFileName.StartOfString + RestartFileName.LengthInChars,
                       _T("\\yori-restart-%y.txt"),
                       ProcessId);
    } else {
        YoriLibSPrintf(RestartFileName.StartOfString + RestartFileName.LengthInChars,
                       _T("\\yori-restart-%x.txt"),
                       GetCurrentProcessId());
    }

    DeleteFile(RestartFileName.StartOfString);
    YoriLibFreeStringContents(&RestartFileName);
}

// vim:sw=4:ts=4:et:
