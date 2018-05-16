/**
 * @file lib/dyld.c
 *
 * Yori dynamically loaded OS function support
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

#include "yoripch.h"
#include "yorilib.h"

/**
 A structure containing pointers to kernel32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_KERNEL32_FUNCTIONS Kernel32;

/**
 Load pointers to all optional kernel32.dll functions.  Because kernel32.dll is
 effectively mandatory in any Win32 process, this uses GetModuleHandle rather
 than LoadLibrary and pointers are valid for the lifetime of the process.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadKernel32Functions()
{
    HANDLE hKernel;
    hKernel = GetModuleHandle(_T("KERNEL32"));
    if (hKernel == NULL) {
        return FALSE;
    }
    Kernel32.pAddConsoleAliasW = (PADD_CONSOLE_ALIASW)GetProcAddress(hKernel, "AddConsoleAliasW");
    Kernel32.pAssignProcessToJobObject = (PASSIGN_PROCESS_TO_JOB_OBJECT)GetProcAddress(hKernel, "AssignProcessToJobObject");
    Kernel32.pCreateHardLinkW = (PCREATE_HARD_LINKW)GetProcAddress(hKernel, "CreateHardLinkW");
    Kernel32.pCreateJobObjectW = (PCREATE_JOB_OBJECTW)GetProcAddress(hKernel, "CreateJobObjectW");
    Kernel32.pCreateSymbolicLinkW = (PCREATE_SYMBOLIC_LINKW)GetProcAddress(hKernel, "CreateSymbolicLinkW");
    Kernel32.pFindFirstStreamW = (PFIND_FIRST_STREAMW)GetProcAddress(hKernel, "FindFirstStreamW");
    Kernel32.pFindNextStreamW = (PFIND_NEXT_STREAMW)GetProcAddress(hKernel, "FindNextStreamW");
    Kernel32.pFreeEnvironmentStringsW = (PFREE_ENVIRONMENT_STRINGSW)GetProcAddress(hKernel, "FreeEnvironmentStringsW");
    Kernel32.pGetCompressedFileSizeW = (PGET_COMPRESSED_FILE_SIZEW)GetProcAddress(hKernel, "GetCompressedFileSizeW");
    Kernel32.pGetConsoleAliasesLengthW = (PGET_CONSOLE_ALIASES_LENGTHW)GetProcAddress(hKernel, "GetConsoleAliasesLengthW");
    Kernel32.pGetConsoleAliasesW = (PGET_CONSOLE_ALIASESW)GetProcAddress(hKernel, "GetConsoleAliasesW");
    Kernel32.pGetConsoleScreenBufferInfoEx = (PGET_CONSOLE_SCREEN_BUFFER_INFO_EX)GetProcAddress(hKernel, "GetConsoleScreenBufferInfoEx");
    Kernel32.pGetCurrentConsoleFontEx = (PGET_CURRENT_CONSOLE_FONT_EX)GetProcAddress(hKernel, "GetCurrentConsoleFontEx");
    Kernel32.pGetDiskFreeSpaceExW = (PGET_DISK_FREE_SPACE_EXW)GetProcAddress(hKernel, "GetDiskFreeSpaceExW");
    Kernel32.pGetEnvironmentStrings = (PGET_ENVIRONMENT_STRINGS)GetProcAddress(hKernel, "GetEnvironmentStrings");
    Kernel32.pGetEnvironmentStringsW = (PGET_ENVIRONMENT_STRINGSW)GetProcAddress(hKernel, "GetEnvironmentStringsW");
    Kernel32.pGetFileInformationByHandleEx = (PGET_FILE_INFORMATION_BY_HANDLE_EX)GetProcAddress(hKernel, "GetFileInformationByHandleEx");
    Kernel32.pRegisterApplicationRestart = (PREGISTER_APPLICATION_RESTART)GetProcAddress(hKernel, "RegisterApplicationRestart");
    Kernel32.pSetConsoleScreenBufferInfoEx = (PSET_CONSOLE_SCREEN_BUFFER_INFO_EX)GetProcAddress(hKernel, "SetConsoleScreenBufferInfoEx");
    Kernel32.pSetCurrentConsoleFontEx = (PSET_CURRENT_CONSOLE_FONT_EX)GetProcAddress(hKernel, "SetCurrentConsoleFontEx");
    Kernel32.pSetInformationJobObject = (PSET_INFORMATION_JOB_OBJECT)GetProcAddress(hKernel, "SetInformationJobObject");
    Kernel32.pWow64DisableWow64FsRedirection = (PWOW64_DISABLE_WOW64_FS_REDIRECTION)GetProcAddress(hKernel, "Wow64DisableWow64FsRedirection");

    return TRUE;
}

/**
 A structure containing pointers to ole32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_OLE32_FUNCTIONS Ole32;

/**
 Load pointers to all optional ole32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadOle32Functions()
{

    if (Ole32.hDll != NULL) {
        return TRUE;
    }

    Ole32.hDll = LoadLibrary(_T("OLE32.DLL"));
    if (Ole32.hDll == NULL) {
        return FALSE;
    }

    Ole32.pCoTaskMemFree = (PCO_TASK_MEM_FREE)GetProcAddress(Ole32.hDll, "CoTaskMemFree");

    return TRUE;
}

/**
 GUID to fetch the downloads folder on Vista+.
 */
const GUID FOLDERID_Downloads = { 0x374de290, 0x123f, 0x4565, { 0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b } };

/**
 A structure containing pointers to shell32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_SHELL32_FUNCTIONS Shell32;

/**
 Load pointers to all optional shell32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadShell32Functions()
{

    if (Shell32.hDll != NULL) {
        return TRUE;
    }

    Shell32.hDll = LoadLibrary(_T("SHELL32.DLL"));
    if (Shell32.hDll == NULL) {
        return FALSE;
    }

    Shell32.pSHFileOperationW = (PSH_FILE_OPERATIONW)GetProcAddress(Shell32.hDll, "SHFileOperationW");
    Shell32.pSHGetKnownFolderPath = (PSH_GET_KNOWN_FOLDER_PATH)GetProcAddress(Shell32.hDll, "SHGetKnownFolderPath");
    Shell32.pSHGetSpecialFolderPathW = (PSH_GET_SPECIAL_FOLDER_PATHW)GetProcAddress(Shell32.hDll, "SHGetSpecialFolderPathW");
    Shell32.pShellExecuteExW = (PSHELL_EXECUTE_EXW)GetProcAddress(Shell32.hDll, "ShellExecuteExW");
    Shell32.pShellExecuteW = (PSHELL_EXECUTEW)GetProcAddress(Shell32.hDll, "ShellExecuteW");
    return TRUE;
}
/**
 A structure containing pointers to version.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_USER32_FUNCTIONS User32;

/**
 Load pointers to all optional user32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadUser32Functions()
{
    if (User32.hDll != NULL) {
        return TRUE;
    }

    User32.hDll = LoadLibrary(_T("USER32.DLL"));
    if (User32.hDll == NULL) {
        return FALSE;
    }

    User32.pCloseClipboard = (PCLOSE_CLIPBOARD)GetProcAddress(User32.hDll, "CloseClipboard");
    User32.pGetClipboardData = (PGET_CLIPBOARD_DATA)GetProcAddress(User32.hDll, "GetClipboardData");
    User32.pOpenClipboard = (POPEN_CLIPBOARD)GetProcAddress(User32.hDll, "OpenClipboard");


    return TRUE;
}


/**
 A structure containing pointers to version.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_VERSION_FUNCTIONS Version;

/**
 Load pointers to all optional version.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadVersionFunctions()
{

    if (Version.hDll != NULL) {
        return TRUE;
    }

    Version.hDll = LoadLibrary(_T("VERSION.DLL"));
    if (Version.hDll == NULL) {
        return FALSE;
    }

    Version.pGetFileVersionInfoSizeW = (PGET_FILE_VERSION_INFO_SIZEW)GetProcAddress(Version.hDll, "GetFileVersionInfoSizeW");
    Version.pGetFileVersionInfoW = (PGET_FILE_VERSION_INFOW)GetProcAddress(Version.hDll, "GetFileVersionInfoW");
    Version.pVerQueryValueW = (PVER_QUERY_VALUEW)GetProcAddress(Version.hDll, "VerQueryValueW");

    return TRUE;
}


// vim:sw=4:ts=4:et:
