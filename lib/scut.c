/**
 * @file lib/scut.c
 *
 * Helper routines for manipulating shortcuts
 *
 * Copyright (c) 2004-2018 Malcolm Smith
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

// vim:sw=4:ts=4:et:
