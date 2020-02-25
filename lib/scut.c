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
    __in DWORD IconIndex,
    __in DWORD ShowState,
    __in WORD Hotkey,
    __in BOOL MergeWithExisting,
    __in BOOL CreateNewIfNeeded
    )
{
    IShellLinkW *scut = NULL;
    IPersistFile *savedfile = NULL;
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

    hRes = DllOle32.pCoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&scut);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    hRes = scut->Vtbl->QueryInterface(scut, &IID_IPersistFile, (void **)&savedfile);
    if (!SUCCEEDED(hRes)) {
        goto Exit;
    }

    if (MergeWithExisting) {
        hRes = savedfile->Vtbl->Load(savedfile, ShortcutFileName->StartOfString, TRUE);
        if (!CreateNewIfNeeded && !SUCCEEDED(hRes)) {
            goto Exit;
        }
    }

    if (Target != NULL) {
        if (scut->Vtbl->SetPath(scut, Target->StartOfString) != NOERROR) {
            goto Exit;
        }
    }

    if (Arguments != NULL) {
        if (scut->Vtbl->SetArguments(scut, Arguments->StartOfString) != NOERROR) {
            goto Exit;
        }
    }

    if (Description != NULL) {
        if (scut->Vtbl->SetDescription(scut, Description->StartOfString) != NOERROR) {
            goto Exit;
        }
    }

    if (Hotkey != (WORD)-1) {
        if (scut->Vtbl->SetHotkey(scut, Hotkey) != NOERROR) {
            goto Exit;
        }
    }

    if (IconPath != NULL) {
        if (scut->Vtbl->SetIconLocation(scut, IconPath->StartOfString, IconIndex) != NOERROR) {
            goto Exit;
        }
    }

    if (ShowState != (WORD)-1) {
        if (scut->Vtbl->SetShowCmd(scut, ShowState) != NOERROR) {
            goto Exit;
        }
    }

    if (WorkingDir != NULL) {
        if (scut->Vtbl->SetWorkingDirectory(scut, WorkingDir->StartOfString) != NOERROR) {
            goto Exit;
        }
    }

    hRes = savedfile->Vtbl->Save(savedfile, ShortcutFileName->StartOfString, TRUE);
    if (!SUCCEEDED(hRes)) {
        goto Exit;
    }

    Result = TRUE;

Exit:

    if (scut != NULL) {
        scut->Vtbl->Release(scut);
    }

    if (savedfile != NULL) {
        savedfile->Vtbl->Release(savedfile);
    }

    return Result;
}

/**
 Execute a specified shortcut file.

 @param ShortcutFileName Pointer to the shortcut file to execute.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibExecuteShortcut(
    __in PYORI_STRING ShortcutFileName
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
    IShellLinkW *scut = NULL;
    IPersistFile *savedfile = NULL;
    BOOL Result = FALSE;
    HRESULT hRes;
    DWORD SizeNeeded;
    LPTSTR Extension;

    ASSERT(YoriLibIsStringNullTerminated(ShortcutFileName));

    YoriLibLoadShell32Functions();

    if (DllShell32.pShellExecuteW == NULL) {
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

    hRes = DllOle32.pCoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&scut);
    if (!SUCCEEDED(hRes)) {
        return FALSE;
    }

    YoriLibInitEmptyString(&FileTarget);
    YoriLibInitEmptyString(&Arguments);
    YoriLibInitEmptyString(&WorkingDirectory);
    YoriLibInitEmptyString(&ExpandedFileTarget);
    YoriLibInitEmptyString(&ExpandedArguments);
    YoriLibInitEmptyString(&ExpandedWorkingDirectory);

    hRes = scut->Vtbl->QueryInterface(scut, &IID_IPersistFile, (void **)&savedfile);
    if (!SUCCEEDED(hRes)) {
        goto Exit;
    }

    hRes = savedfile->Vtbl->Load(savedfile, ShortcutFileName->StartOfString, 0);
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
        hRes = scut->Vtbl->GetWorkingDirectory(scut,
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
        hRes = scut->Vtbl->GetArguments(scut,
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
        hRes = scut->Vtbl->GetPath(scut,
                                   FileTarget.StartOfString,
                                   FileTarget.LengthAllocated,
                                   NULL,
                                   0);
    }

    if (scut->Vtbl->GetShowCmd(scut, &nShow) != NOERROR) {
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

    if (!YoriLibAllocateString(&ExpandedFileTarget, SizeNeeded)) {
        goto Exit;
    }

    ExpandedFileTarget.LengthInChars = ExpandEnvironmentStrings(FileTarget.StartOfString, ExpandedFileTarget.StartOfString, ExpandedFileTarget.LengthAllocated);

    SizeNeeded = ExpandEnvironmentStrings(Arguments.StartOfString, NULL, 0);
    if (SizeNeeded == 0) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&ExpandedArguments, SizeNeeded)) {
        goto Exit;
    }

    ExpandedArguments.LengthInChars = ExpandEnvironmentStrings(Arguments.StartOfString, ExpandedArguments.StartOfString, ExpandedArguments.LengthAllocated);

    SizeNeeded = ExpandEnvironmentStrings(WorkingDirectory.StartOfString, NULL, 0);
    if (SizeNeeded == 0) {
        goto Exit;
    }

    if (!YoriLibAllocateString(&ExpandedWorkingDirectory, SizeNeeded)) {
        goto Exit;
    }

    ExpandedWorkingDirectory.LengthInChars = ExpandEnvironmentStrings(WorkingDirectory.StartOfString, ExpandedWorkingDirectory.StartOfString, ExpandedWorkingDirectory.LengthAllocated);

    YoriLibTrimNullTerminators(&ExpandedFileTarget);
    YoriLibTrimNullTerminators(&ExpandedArguments);
    YoriLibTrimNullTerminators(&ExpandedWorkingDirectory);

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
                                   CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
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

    if (!Result) {
        hApp = DllShell32.pShellExecuteW(NULL,
                                         NULL,
                                         ExpandedFileTarget.StartOfString,
                                         ExpandedArguments.StartOfString,
                                         ExpandedWorkingDirectory.StartOfString,
                                         nShow);
        if ((ULONG_PTR)hApp <= 32) {
            goto Exit;
        }

        Result = TRUE;
    }

Exit:

    YoriLibFreeStringContents(&FileTarget);
    YoriLibFreeStringContents(&WorkingDirectory);
    YoriLibFreeStringContents(&Arguments);
    YoriLibFreeStringContents(&ExpandedFileTarget);
    YoriLibFreeStringContents(&ExpandedWorkingDirectory);
    YoriLibFreeStringContents(&ExpandedArguments);

    if (savedfile != NULL) {
        savedfile->Vtbl->Release(savedfile);
        savedfile = NULL;
    }

    if (scut != NULL) {
        scut->Vtbl->Release(scut);
        scut = NULL;
    }

    return Result;
}

// vim:sw=4:ts=4:et:
