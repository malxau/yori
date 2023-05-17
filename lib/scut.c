/**
 * @file lib/scut.c
 *
 * Helper routines for manipulating shortcuts
 *
 * Copyright (c) 2004-2020 Malcolm Smith
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

#pragma warning(disable: 4226)

/**
 A declaration for a GUID defining the shell file API interface.
 */
const GUID IID_IPersistFile = { 0x0000010BL, 0, 0, { 0xC0, 0, 0, 0, 0, 0, 0, 0x46 } };

/**
 The ShellLink interface.
 */
const GUID CLSID_ShellLink = { 0x00021401L, 0, 0, { 0xC0, 0, 0, 0, 0, 0, 0, 0x46 } };

/**
 The IShellLink interface.
 */
const GUID IID_IShellLinkW = { 0x000214F9L, 0, 0, { 0xC0, 0, 0, 0, 0, 0, 0, 0x46 } };

/**
 The IShellLinkDataList interface.
 */
const GUID IID_IShellLinkDataList = { 0x45e2b4ae, 0xb1c3, 0x11d0, { 0xb9, 0x2f, 0x0, 0xa0, 0xc9, 0x3, 0x12, 0xe1 } };

/**
 Create or modify a shortcut file.

 @param ShortcutFileName Path of the shortcut file.  The caller is expected
        to resolve this to a full path before calling this function.

 @param Target If specified, the target that the shortcut should refer to.

 @param Arguments If specified, additional arguments to pass to the target.

 @param Description If specified, the description of the shortcut, including
        the window title to display for console applications.

 @param WorkingDir If specified, the current directory to set when launching
        the executable.

 @param IconPath If specified, the path to the binary containing the icon for
        the shortcut.

 @param ConsoleProps If specified, the block of console attributes to attach
        to the shortcut.  Note this is only available on NT 4 with the shell
        update installed or above.

 @param IconIndex The index of the icon within any executable or DLL used as
        the source of the icon.  This is ignored unless IconPath is specified.

 @param ShowState The ShowWindow style state to start the application in. If
        this value is -1, the current value is retained.

 @param Hotkey Any hotkey that should be used to launch the application.  If
        this value is -1, the current value is retained.

 @param MergeWithExisting If TRUE, existing shortcut values are loaded and the
        supplied values are merged with those.  If FALSE, any existing
        shortcut is overwritten and only the supplied values are used.

 @param CreateNewIfNeeded If TRUE, failure to load the existing values is not
        fatal and creation of a new shortcut can continue.  If FALSE, failure
        to load existing values causes the function to fail.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibCreateShortcut(
    __in PYORI_STRING ShortcutFileName,
    __in_opt PYORI_STRING Target,
    __in_opt PYORI_STRING Arguments,
    __in_opt PYORI_STRING Description,
    __in_opt PYORI_STRING WorkingDir,
    __in_opt PYORI_STRING IconPath,
    __in_opt PISHELLLINKDATALIST_CONSOLE_PROPS ConsoleProps,
    __in DWORD IconIndex,
    __in DWORD ShowState,
    __in WORD Hotkey,
    __in BOOL MergeWithExisting,
    __in BOOL CreateNewIfNeeded
    )
{
    IShellLinkW *Scut = NULL;
    IPersistFile *ScutFile = NULL;
    IShellLinkDataList *ShortcutDataList = NULL;
    BOOL Result = FALSE;
    HRESULT hRes;

    ASSERT(YoriLibIsStringNullTerminated(ShortcutFileName));
    ASSERT(Target == NULL || YoriLibIsStringNullTerminated(Target));
    ASSERT(Arguments == NULL || YoriLibIsStringNullTerminated(Arguments));
    ASSERT(Description == NULL || YoriLibIsStringNullTerminated(Description));
    ASSERT(WorkingDir == NULL || YoriLibIsStringNullTerminated(WorkingDir));
    ASSERT(IconPath == NULL || YoriLibIsStringNullTerminated(IconPath));

    YoriLibLoadOle32Functions();
    if (DllOle32.pCoCreateInstance == NULL || DllOle32.pCoInitialize == NULL) {
        return FALSE;
    }

    hRes = DllOle32.pCoInitialize(NULL);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    hRes = DllOle32.pCoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&Scut);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    hRes = Scut->Vtbl->QueryInterface(Scut, &IID_IPersistFile, (void **)&ScutFile);
    if (!SUCCEEDED(hRes)) {
        goto Exit;
    }

    //
    //  This doesn't exist on original NT4.  Don't explode if it's
    //  missing.
    //

    Scut->Vtbl->QueryInterface(Scut, &IID_IShellLinkDataList, (void **)&ShortcutDataList);

    if (MergeWithExisting) {
        hRes = ScutFile->Vtbl->Load(ScutFile, ShortcutFileName->StartOfString, TRUE);
        if (!CreateNewIfNeeded && !SUCCEEDED(hRes)) {
            goto Exit;
        }
    }

    if (Target != NULL) {
        if (Scut->Vtbl->SetPath(Scut, Target->StartOfString) != NOERROR) {
            goto Exit;
        }
    }

    if (Arguments != NULL) {
        if (Scut->Vtbl->SetArguments(Scut, Arguments->StartOfString) != NOERROR) {
            goto Exit;
        }
    }

    if (Description != NULL) {
        if (Scut->Vtbl->SetDescription(Scut, Description->StartOfString) != NOERROR) {
            goto Exit;
        }
    }

    if (Hotkey != (WORD)-1) {
        if (Scut->Vtbl->SetHotkey(Scut, Hotkey) != NOERROR) {
            goto Exit;
        }
    }

    if (IconPath != NULL) {
        if (Scut->Vtbl->SetIconLocation(Scut, IconPath->StartOfString, IconIndex) != NOERROR) {
            goto Exit;
        }
    }

    if (ShowState != (WORD)-1) {
        if (Scut->Vtbl->SetShowCmd(Scut, ShowState) != NOERROR) {
            goto Exit;
        }
    }

    if (WorkingDir != NULL) {
        if (Scut->Vtbl->SetWorkingDirectory(Scut, WorkingDir->StartOfString) != NOERROR) {
            goto Exit;
        }
    }

    if (ConsoleProps != NULL && ShortcutDataList != NULL) {
        ShortcutDataList->Vtbl->RemoveDataBlock(ShortcutDataList, ISHELLLINKDATALIST_CONSOLE_PROPS_SIG);
        if (ShortcutDataList->Vtbl->AddDataBlock(ShortcutDataList, ConsoleProps) != NOERROR) {
            goto Exit;
        }
    }

    hRes = ScutFile->Vtbl->Save(ScutFile, ShortcutFileName->StartOfString, TRUE);
    if (!SUCCEEDED(hRes)) {
        goto Exit;
    }

    Result = TRUE;

Exit:

    if (ShortcutDataList != NULL) {
        ShortcutDataList->Vtbl->Release(ShortcutDataList);
    }

    if (Scut != NULL) {
        Scut->Vtbl->Release(Scut);
    }

    if (ScutFile != NULL) {
        ScutFile->Vtbl->Release(ScutFile);
    }

    return Result;
}

/**
 Load the path to an icon resource from a specified shortcut file.

 @param ShortcutFileName Pointer to the shortcut file to resolve.

 @param IconPath On successful completion, populated with a path to a file
        containing the icon to display.

 @param IconIndex On successful completion, updated to indicate the icon index
        within the file to display.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibLoadShortcutIconPath(
    __in PYORI_STRING ShortcutFileName,
    __out PYORI_STRING IconPath,
    __out PDWORD IconIndex
    )
{
    YORI_STRING IconLocation;
    YORI_STRING ExpandedLocation;
    DWORD LocalIconIndex;
    IShellLinkW *Scut = NULL;
    IPersistFile *ScutFile = NULL;
    BOOL Result = FALSE;
    HRESULT hRes;
    DWORD SizeNeeded;

    ASSERT(YoriLibIsStringNullTerminated(ShortcutFileName));

    YoriLibLoadShell32Functions();
    YoriLibLoadOle32Functions();
    if (DllOle32.pCoCreateInstance == NULL || DllOle32.pCoInitialize == NULL) {
        return FALSE;
    }

    hRes = DllOle32.pCoInitialize(NULL);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    hRes = DllOle32.pCoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&Scut);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    YoriLibInitEmptyString(&IconLocation);
    YoriLibInitEmptyString(&ExpandedLocation);
    LocalIconIndex = 0;

    hRes = Scut->Vtbl->QueryInterface(Scut, &IID_IPersistFile, (void **)&ScutFile);
    if (!SUCCEEDED(hRes)) {
        goto Exit;
    }

    hRes = ScutFile->Vtbl->Load(ScutFile, ShortcutFileName->StartOfString, 0);
    if (!SUCCEEDED(hRes)) {
        goto Exit;
    }

    //
    //  The following code approximates how things should work, which is not
    //  how they actually work.  As far as I can tell, and as far as the
    //  documentation goes, these APIs don't return any indication of
    //  required buffer size, and end up truncating the return value instead
    //  which makes writing accurate code on these APIs basically impossible.
    //

    hRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    while (hRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        SizeNeeded = IconLocation.LengthAllocated * 4;
        if (SizeNeeded == 0) {
            SizeNeeded = 1024;
        }
        YoriLibFreeStringContents(&IconLocation);

        if (!YoriLibAllocateString(&IconLocation, SizeNeeded)) {
            goto Exit;
        }
        hRes = Scut->Vtbl->GetIconLocation(Scut,
                                           IconLocation.StartOfString,
                                           IconLocation.LengthAllocated,
                                           &LocalIconIndex);

        if (SUCCEEDED(hRes)) {
            IconLocation.LengthInChars = wcslen(IconLocation.StartOfString);
        }
    }

    if (IconLocation.LengthInChars == 0) {
        LocalIconIndex = 0;
        hRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        SizeNeeded = 0;
        while (hRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            if (SizeNeeded == 0) {
                SizeNeeded = 1024;
            } else {
                SizeNeeded = IconLocation.LengthAllocated * 4;
            }
            if (IconLocation.LengthAllocated < SizeNeeded) {
                YoriLibFreeStringContents(&IconLocation);

                if (!YoriLibAllocateString(&IconLocation, SizeNeeded)) {
                    goto Exit;
                }
            }
            hRes = Scut->Vtbl->GetPath(Scut,
                                       IconLocation.StartOfString,
                                       IconLocation.LengthAllocated,
                                       NULL,
                                       0);

            if (SUCCEEDED(hRes)) {
                IconLocation.LengthInChars = wcslen(IconLocation.StartOfString);
            }
        }
    }

    //
    //  If we still don't have a path, we can't find any icon.
    //

    if (IconLocation.LengthInChars == 0) {
        goto Exit;
    }

    //
    //  Newer versions of Windows expand the environment variables in the
    //  shortcut by default.  Older versions require us to do it manually
    //  here.
    //

    SizeNeeded = ExpandEnvironmentStrings(IconLocation.StartOfString, NULL, 0);
    if (SizeNeeded == 0) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&ExpandedLocation, SizeNeeded + 1)) {
        goto Exit;
    }

    ExpandedLocation.LengthInChars = ExpandEnvironmentStrings(IconLocation.StartOfString, ExpandedLocation.StartOfString, ExpandedLocation.LengthAllocated);
    YoriLibTrimNullTerminators(&ExpandedLocation);

    memcpy(IconPath, &ExpandedLocation, sizeof(YORI_STRING));
    YoriLibInitEmptyString(&ExpandedLocation);
    *IconIndex = LocalIconIndex;

    Result = TRUE;

Exit:

    YoriLibFreeStringContents(&IconLocation);
    YoriLibFreeStringContents(&ExpandedLocation);

    if (ScutFile != NULL) {
        ScutFile->Vtbl->Release(ScutFile);
        ScutFile = NULL;
    }

    if (Scut != NULL) {
        Scut->Vtbl->Release(Scut);
        Scut = NULL;
    }

    return Result;
}

/**
 Execute a specified shortcut file.

 @param ShortcutFileName Pointer to the shortcut file to execute.

 @param Elevate TRUE if the program should be run as an Administrator;
        FALSE to run in current user context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibExecuteShortcut(
    __in PYORI_STRING ShortcutFileName,
    __in BOOLEAN Elevate
    )
{
    YORI_STRING FileTarget;
    YORI_STRING Arguments;
    YORI_STRING WorkingDirectory;
    YORI_STRING ExpandedFileTarget;
    YORI_STRING ExpandedArguments;
    YORI_STRING ExpandedWorkingDirectory;
    INT nShow;
    HINSTANCE hApp;
    IShellLinkW *Scut = NULL;
    IPersistFile *ScutFile = NULL;
    BOOL Result = FALSE;
    HRESULT hRes;
    DWORD SizeNeeded;
    LPTSTR Extension;

    ASSERT(YoriLibIsStringNullTerminated(ShortcutFileName));

    YoriLibLoadShell32Functions();

    if (Elevate && DllShell32.pShellExecuteExW == NULL) {
        return FALSE;
    }

    YoriLibLoadOle32Functions();
    if (DllOle32.pCoCreateInstance == NULL || DllOle32.pCoInitialize == NULL) {
        return FALSE;
    }

    hRes = DllOle32.pCoInitialize(NULL);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    hRes = DllOle32.pCoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&Scut);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    YoriLibInitEmptyString(&FileTarget);
    YoriLibInitEmptyString(&Arguments);
    YoriLibInitEmptyString(&WorkingDirectory);
    YoriLibInitEmptyString(&ExpandedFileTarget);
    YoriLibInitEmptyString(&ExpandedArguments);
    YoriLibInitEmptyString(&ExpandedWorkingDirectory);

    hRes = Scut->Vtbl->QueryInterface(Scut, &IID_IPersistFile, (void **)&ScutFile);
    if (!SUCCEEDED(hRes)) {
        goto Exit;
    }

    hRes = ScutFile->Vtbl->Load(ScutFile, ShortcutFileName->StartOfString, 0);
    if (!SUCCEEDED(hRes)) {
        goto Exit;
    }

    //
    //  The following code approximates how things should work, which is not
    //  how they actually work.  As far as I can tell, and as far as the
    //  documentation goes, these APIs don't return any indication of
    //  required buffer size, and end up truncating the return value instead
    //  which makes writing accurate code on these APIs basically impossible.
    //

    hRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    while (hRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        SizeNeeded = WorkingDirectory.LengthAllocated * 4;
        if (SizeNeeded == 0) {
            SizeNeeded = 1024;
        }
        YoriLibFreeStringContents(&WorkingDirectory);

        if (!YoriLibAllocateString(&WorkingDirectory, SizeNeeded)) {
            goto Exit;
        }
        hRes = Scut->Vtbl->GetWorkingDirectory(Scut,
                                               WorkingDirectory.StartOfString,
                                               WorkingDirectory.LengthAllocated);
    }

    hRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    while (hRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        SizeNeeded = Arguments.LengthAllocated * 4;
        if (SizeNeeded == 0) {
            SizeNeeded = 1024;
        }
        YoriLibFreeStringContents(&Arguments);

        if (!YoriLibAllocateString(&Arguments, SizeNeeded)) {
            goto Exit;
        }
        hRes = Scut->Vtbl->GetArguments(Scut,
                                        Arguments.StartOfString,
                                        Arguments.LengthAllocated);
    }

    hRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    while (hRes == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        SizeNeeded = FileTarget.LengthAllocated * 4;
        if (SizeNeeded == 0) {
            SizeNeeded = 1024;
        }
        YoriLibFreeStringContents(&FileTarget);

        if (!YoriLibAllocateString(&FileTarget, SizeNeeded)) {
            goto Exit;
        }
        hRes = Scut->Vtbl->GetPath(Scut,
                                   FileTarget.StartOfString,
                                   FileTarget.LengthAllocated,
                                   NULL,
                                   0);
    }

    if (Scut->Vtbl->GetShowCmd(Scut, &nShow) != NOERROR) {
        goto Exit;
    }

    //
    //  Newer versions of Windows expand the environment variables in the
    //  shortcut by default.  Older versions require us to do it manually
    //  here.
    //

    SizeNeeded = ExpandEnvironmentStrings(FileTarget.StartOfString, NULL, 0);
    if (SizeNeeded == 0) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&ExpandedFileTarget, SizeNeeded + 1)) {
        goto Exit;
    }

    ExpandedFileTarget.LengthInChars = ExpandEnvironmentStrings(FileTarget.StartOfString, ExpandedFileTarget.StartOfString, ExpandedFileTarget.LengthAllocated);

    SizeNeeded = ExpandEnvironmentStrings(Arguments.StartOfString, NULL, 0);
    if (SizeNeeded == 0) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&ExpandedArguments, SizeNeeded + 1)) {
        goto Exit;
    }

    ExpandedArguments.LengthInChars = ExpandEnvironmentStrings(Arguments.StartOfString, ExpandedArguments.StartOfString, ExpandedArguments.LengthAllocated);

    SizeNeeded = ExpandEnvironmentStrings(WorkingDirectory.StartOfString, NULL, 0);
    if (SizeNeeded == 0) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&ExpandedWorkingDirectory, SizeNeeded + 1)) {
        goto Exit;
    }

    ExpandedWorkingDirectory.LengthInChars = ExpandEnvironmentStrings(WorkingDirectory.StartOfString, ExpandedWorkingDirectory.StartOfString, ExpandedWorkingDirectory.LengthAllocated);

    YoriLibTrimNullTerminators(&ExpandedFileTarget);
    YoriLibTrimNullTerminators(&ExpandedArguments);
    YoriLibTrimNullTerminators(&ExpandedWorkingDirectory);

    if (!Elevate) {
        Extension = YoriLibFindRightMostCharacter(&ExpandedFileTarget, '.');
        if (Extension != NULL) {
            YORI_STRING YsExt;
            YoriLibInitEmptyString(&YsExt);
            YsExt.StartOfString = Extension;
            YsExt.LengthInChars = ExpandedFileTarget.LengthInChars - (DWORD)(Extension - ExpandedFileTarget.StartOfString);

            if (YoriLibCompareStringWithLiteralInsensitive(&YsExt, _T(".exe")) == 0 ||
                YoriLibCompareStringWithLiteralInsensitive(&YsExt, _T(".com")) == 0) {

                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                YORI_STRING CmdLine;
                YORI_STRING UnescapedPath;
                DWORD CharsNeeded;
                BOOLEAN HasWhiteSpace;

                YoriLibInitEmptyString(&UnescapedPath);
                if (!YoriLibUnescapePath(ShortcutFileName, &UnescapedPath)) {
                    UnescapedPath.StartOfString = ShortcutFileName->StartOfString;
                }

                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);

                si.dwFlags = STARTF_TITLEISLINKNAME;
                si.lpTitle = UnescapedPath.StartOfString;

                HasWhiteSpace = YoriLibCheckIfArgNeedsQuotes(&ExpandedFileTarget);
                CharsNeeded = ExpandedFileTarget.LengthInChars + 1 + ExpandedArguments.LengthInChars + 1;
                if (HasWhiteSpace) {
                    CharsNeeded += 2;
                }

                if (!YoriLibAllocateString(&CmdLine, CharsNeeded)) {
                    YoriLibFreeStringContents(&UnescapedPath);
                    goto Exit;
                }

                if (HasWhiteSpace) {
                    CmdLine.LengthInChars = YoriLibSPrintf(CmdLine.StartOfString, _T("\"%y\" %y"), &ExpandedFileTarget, &ExpandedArguments);
                } else {
                    CmdLine.LengthInChars = YoriLibSPrintf(CmdLine.StartOfString, _T("%y %y"), &ExpandedFileTarget, &ExpandedArguments);
                }

                Result = CreateProcess(NULL,
                                       CmdLine.StartOfString,
                                       NULL,
                                       NULL,
                                       FALSE,
                                       CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE | CREATE_DEFAULT_ERROR_MODE,
                                       NULL,
                                       ExpandedWorkingDirectory.StartOfString,
                                       &si,
                                       &pi);

                if (Result) {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }

                YoriLibFreeStringContents(&CmdLine);
                YoriLibFreeStringContents(&UnescapedPath);
            }
        }
    }

    if (!Result) {
        YORI_SHELLEXECUTEINFO sei;

        ZeroMemory(&sei, sizeof(sei));
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_FLAG_NO_UI |
                    SEE_MASK_NOZONECHECKS |
                    SEE_MASK_UNICODE;

        sei.lpFile = ExpandedFileTarget.StartOfString;
        sei.lpParameters = ExpandedArguments.StartOfString;
        sei.lpDirectory = ExpandedWorkingDirectory.StartOfString;
        sei.nShow = nShow;

        if (DllShell32.pShellExecuteExW != NULL) {
            if (Elevate) {
                sei.lpVerb = _T("runas");
            }

            if (!DllShell32.pShellExecuteExW(&sei)) {
                goto Exit;
            }

            Result = TRUE;
        } else if (DllShell32.pShellExecuteW != NULL) {
            hApp = DllShell32.pShellExecuteW(NULL,
                                             NULL,
                                             sei.lpFile,
                                             sei.lpParameters,
                                             sei.lpDirectory,
                                             sei.nShow);
            if ((ULONG_PTR)hApp <= 32) {
                goto Exit;
            }

            Result = TRUE;
        }
    }

Exit:

    YoriLibFreeStringContents(&FileTarget);
    YoriLibFreeStringContents(&WorkingDirectory);
    YoriLibFreeStringContents(&Arguments);
    YoriLibFreeStringContents(&ExpandedFileTarget);
    YoriLibFreeStringContents(&ExpandedWorkingDirectory);
    YoriLibFreeStringContents(&ExpandedArguments);

    if (ScutFile != NULL) {
        ScutFile->Vtbl->Release(ScutFile);
        ScutFile = NULL;
    }

    if (Scut != NULL) {
        Scut->Vtbl->Release(Scut);
        Scut = NULL;
    }

    return Result;
}

/**
 Generate the default console properties for a shortcut based on the user's
 defaults.  This is required because if a shortcut contains any console
 setting, it must have all of them, so if Scut is asked to modify something
 it needs to approximately guess all of the rest.

 @return Pointer to the console properties, allocated in this routine.  The
         caller is expected to free these with @ref YoriLibDereference .
 */
PISHELLLINKDATALIST_CONSOLE_PROPS
YoriLibAllocateDefaultConsoleProperties(VOID)
{
    PISHELLLINKDATALIST_CONSOLE_PROPS ConsoleProps;
    YORI_STRING KeyName;
    TCHAR ValueNameBuffer[16];
    TCHAR FontNameBuffer[LF_FACESIZE];
    YORI_STRING ValueName;
    DWORD Err;
    DWORD Index;
    DWORD Temp;
    HKEY hKey;
    DWORD Disp;
    DWORD ValueType;
    DWORD ValueSize;

    YoriLibLoadAdvApi32Functions();

    ConsoleProps = YoriLibReferencedMalloc(sizeof(ISHELLLINKDATALIST_CONSOLE_PROPS));
    if (ConsoleProps == NULL) {
        return NULL;
    }

    //
    //  Start with hardcoded defaults that seem to match system behavior
    //

    ConsoleProps->dwSize = sizeof(ISHELLLINKDATALIST_CONSOLE_PROPS);
    ConsoleProps->dwSignature = ISHELLLINKDATALIST_CONSOLE_PROPS_SIG;
    ConsoleProps->WindowColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    ConsoleProps->PopupColor = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_BLUE;
    ConsoleProps->ScreenBufferSize.X = 80;
    ConsoleProps->ScreenBufferSize.Y = 500;
    ConsoleProps->WindowSize.X = 80;
    ConsoleProps->WindowSize.Y = 25;
    ConsoleProps->WindowPosition.X = 0;
    ConsoleProps->WindowPosition.Y = 0;
    ConsoleProps->FontNumber = 0;
    ConsoleProps->InputBufferSize = 0;
    ConsoleProps->FontSize.X = 8;
    ConsoleProps->FontSize.Y = 12;
    ConsoleProps->FontFamily = FF_MODERN;
    ConsoleProps->FontWeight = FW_NORMAL;
    _tcscpy(ConsoleProps->FaceName, _T("Terminal"));
    ConsoleProps->CursorSize = 25;
    ConsoleProps->FullScreen = FALSE;
    ConsoleProps->QuickEdit = TRUE;
    ConsoleProps->InsertMode = TRUE;
    ConsoleProps->AutoPosition = TRUE;
    ConsoleProps->HistoryBufferSize = 50;
    ConsoleProps->NumberOfHistoryBuffers = 4;
    ConsoleProps->RemoveHistoryDuplicates = FALSE;
    ConsoleProps->ColorTable[0] =  RGB(0x00, 0x00, 0x00);
    ConsoleProps->ColorTable[1] =  RGB(0x00, 0x00, 0x80);
    ConsoleProps->ColorTable[2] =  RGB(0x00, 0x80, 0x00);
    ConsoleProps->ColorTable[3] =  RGB(0x00, 0x80, 0x80);
    ConsoleProps->ColorTable[4] =  RGB(0x80, 0x00, 0x00);
    ConsoleProps->ColorTable[5] =  RGB(0x80, 0x00, 0x80);
    ConsoleProps->ColorTable[6] =  RGB(0x80, 0x80, 0x00);
    ConsoleProps->ColorTable[7] =  RGB(0xC0, 0xC0, 0xC0);
    ConsoleProps->ColorTable[8] =  RGB(0x80, 0x80, 0x80);
    ConsoleProps->ColorTable[9] =  RGB(0x00, 0x00, 0xFF);
    ConsoleProps->ColorTable[10] = RGB(0x00, 0xFF, 0x00);
    ConsoleProps->ColorTable[11] = RGB(0x00, 0xFF, 0xFF);
    ConsoleProps->ColorTable[12] = RGB(0xFF, 0x00, 0x00);
    ConsoleProps->ColorTable[13] = RGB(0xFF, 0x00, 0xFF);
    ConsoleProps->ColorTable[14] = RGB(0xFF, 0xFF, 0x00);
    ConsoleProps->ColorTable[15] = RGB(0xFF, 0xFF, 0xFF);

    //
    //  If the registry contains default values, use those instead.  Since
    //  the registry may not have entries for everything, proceed to the
    //  next setting no value or an invalid value is found.
    //

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllAdvApi32.pRegCreateKeyExW == NULL) {

        return ConsoleProps;
    }

    YoriLibConstantString(&KeyName, _T("Console"));

    Err = DllAdvApi32.pRegCreateKeyExW(HKEY_CURRENT_USER,
                                       KeyName.StartOfString,
                                       0,
                                       NULL,
                                       0,
                                       KEY_QUERY_VALUE,
                                       NULL,
                                       &hKey,
                                       &Disp);

    if (Err != ERROR_SUCCESS) {
        return ConsoleProps;
    }

    YoriLibInitEmptyString(&ValueName);
    ValueName.StartOfString = ValueNameBuffer;
    ValueName.LengthAllocated = sizeof(ValueNameBuffer)/sizeof(ValueNameBuffer[0]);

    for (Index = 0; Index < 16; Index++) {
        ValueName.LengthInChars = YoriLibSPrintfS(ValueName.StartOfString, ValueName.LengthAllocated, _T("ColorTable%02i"), Index);
        ValueSize = sizeof(Temp);
        Err = DllAdvApi32.pRegQueryValueExW(hKey, ValueName.StartOfString, 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
        if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
            ConsoleProps->ColorTable[Index] = Temp;
        }
    }

    ValueSize = sizeof(Temp);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("CursorSize"), 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
        ConsoleProps->CursorSize = Temp;
    }

    ValueSize = sizeof(FontNameBuffer);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("FaceName"), 0, &ValueType, (LPBYTE)FontNameBuffer, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_SZ && ValueSize <= sizeof(FontNameBuffer)) {
        memcpy(ConsoleProps->FaceName, FontNameBuffer, ValueSize);
    }

    ValueSize = sizeof(Temp);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("FontFamily"), 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
        ConsoleProps->FontFamily = Temp;
    }

    ValueSize = sizeof(Temp);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("FontSize"), 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
        ConsoleProps->FontSize.X = LOWORD(Temp);
        ConsoleProps->FontSize.Y = HIWORD(Temp);
    }

    ValueSize = sizeof(Temp);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("FontWeight"), 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
        ConsoleProps->FontWeight = Temp;
    }

    ValueSize = sizeof(Temp);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("InsertMode"), 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
        ConsoleProps->InsertMode = Temp;
    }

    ValueSize = sizeof(Temp);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("PopupColors"), 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
        ConsoleProps->PopupColor = (WORD)Temp;
    }

    ValueSize = sizeof(Temp);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("QuickEdit"), 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
        ConsoleProps->QuickEdit = Temp;
    }

    ValueSize = sizeof(Temp);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("ScreenBufferSize"), 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
        ConsoleProps->ScreenBufferSize.X = LOWORD(Temp);
        ConsoleProps->ScreenBufferSize.Y = HIWORD(Temp);
    }

    ValueSize = sizeof(Temp);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("ScreenColors"), 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
        ConsoleProps->WindowColor = (WORD)Temp;
    }

    ValueSize = sizeof(Temp);
    Err = DllAdvApi32.pRegQueryValueExW(hKey, _T("WindowSize"), 0, &ValueType, (LPBYTE)&Temp, &ValueSize);
    if (Err == ERROR_SUCCESS && ValueType == REG_DWORD && ValueSize == sizeof(Temp)) {
        ConsoleProps->WindowSize.X = LOWORD(Temp);
        ConsoleProps->WindowSize.Y = HIWORD(Temp);
    }

    DllAdvApi32.pRegCloseKey(hKey);
    return ConsoleProps;
}

// vim:sw=4:ts=4:et:
