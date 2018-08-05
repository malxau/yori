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
 A structure containing pointers to ntdll.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_NTDLL_FUNCTIONS DllNtDll;

/**
 Load pointers to all optional ntdll.dll functions.  Because ntdll.dll is
 effectively mandatory in any Win32 process, this uses GetModuleHandle rather
 than LoadLibrary and pointers are valid for the lifetime of the process.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadNtDllFunctions()
{
    if (DllNtDll.hDll != NULL) {
        return TRUE;
    }

    DllNtDll.hDll = GetModuleHandle(_T("NTDLL"));
    if (DllNtDll.hDll == NULL) {
        return FALSE;
    }
    DllNtDll.pNtQueryInformationProcess = (PNT_QUERY_INFORMATION_PROCESS)GetProcAddress(DllNtDll.hDll, "NtQueryInformationProcess");
    return TRUE;
}

/**
 A structure containing pointers to kernel32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_KERNEL32_FUNCTIONS DllKernel32;

/**
 Load pointers to all optional kernel32.dll functions.  Because kernel32.dll is
 effectively mandatory in any Win32 process, this uses GetModuleHandle rather
 than LoadLibrary and pointers are valid for the lifetime of the process.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadKernel32Functions()
{
    if (DllKernel32.hDll != NULL) {
        return TRUE;
    }

    DllKernel32.hDll = GetModuleHandle(_T("KERNEL32"));
    if (DllKernel32.hDll == NULL) {
        return FALSE;
    }

    DllKernel32.pAddConsoleAliasW = (PADD_CONSOLE_ALIASW)GetProcAddress(DllKernel32.hDll, "AddConsoleAliasW");
    DllKernel32.pAssignProcessToJobObject = (PASSIGN_PROCESS_TO_JOB_OBJECT)GetProcAddress(DllKernel32.hDll, "AssignProcessToJobObject");
    DllKernel32.pCreateHardLinkW = (PCREATE_HARD_LINKW)GetProcAddress(DllKernel32.hDll, "CreateHardLinkW");
    DllKernel32.pCreateJobObjectW = (PCREATE_JOB_OBJECTW)GetProcAddress(DllKernel32.hDll, "CreateJobObjectW");
    DllKernel32.pCreateSymbolicLinkW = (PCREATE_SYMBOLIC_LINKW)GetProcAddress(DllKernel32.hDll, "CreateSymbolicLinkW");
    DllKernel32.pFindFirstStreamW = (PFIND_FIRST_STREAMW)GetProcAddress(DllKernel32.hDll, "FindFirstStreamW");
    DllKernel32.pFindNextStreamW = (PFIND_NEXT_STREAMW)GetProcAddress(DllKernel32.hDll, "FindNextStreamW");
    DllKernel32.pFreeEnvironmentStringsW = (PFREE_ENVIRONMENT_STRINGSW)GetProcAddress(DllKernel32.hDll, "FreeEnvironmentStringsW");
    DllKernel32.pGetCompressedFileSizeW = (PGET_COMPRESSED_FILE_SIZEW)GetProcAddress(DllKernel32.hDll, "GetCompressedFileSizeW");
    DllKernel32.pGetConsoleAliasesLengthW = (PGET_CONSOLE_ALIASES_LENGTHW)GetProcAddress(DllKernel32.hDll, "GetConsoleAliasesLengthW");
    DllKernel32.pGetConsoleAliasesW = (PGET_CONSOLE_ALIASESW)GetProcAddress(DllKernel32.hDll, "GetConsoleAliasesW");
    DllKernel32.pGetConsoleScreenBufferInfoEx = (PGET_CONSOLE_SCREEN_BUFFER_INFO_EX)GetProcAddress(DllKernel32.hDll, "GetConsoleScreenBufferInfoEx");
    DllKernel32.pGetConsoleWindow = (PGET_CONSOLE_WINDOW)GetProcAddress(DllKernel32.hDll, "GetConsoleWindow");
    DllKernel32.pGetCurrentConsoleFontEx = (PGET_CURRENT_CONSOLE_FONT_EX)GetProcAddress(DllKernel32.hDll, "GetCurrentConsoleFontEx");
    DllKernel32.pGetDiskFreeSpaceExW = (PGET_DISK_FREE_SPACE_EXW)GetProcAddress(DllKernel32.hDll, "GetDiskFreeSpaceExW");
    DllKernel32.pGetEnvironmentStrings = (PGET_ENVIRONMENT_STRINGS)GetProcAddress(DllKernel32.hDll, "GetEnvironmentStrings");
    DllKernel32.pGetEnvironmentStringsW = (PGET_ENVIRONMENT_STRINGSW)GetProcAddress(DllKernel32.hDll, "GetEnvironmentStringsW");
    DllKernel32.pGetFileInformationByHandleEx = (PGET_FILE_INFORMATION_BY_HANDLE_EX)GetProcAddress(DllKernel32.hDll, "GetFileInformationByHandleEx");
    DllKernel32.pGetPrivateProfileSectionNamesW = (PGET_PRIVATE_PROFILE_SECTION_NAMESW)GetProcAddress(DllKernel32.hDll, "GetPrivateProfileSectionNamesW");
    DllKernel32.pGetVersionExW = (PGET_VERSION_EXW)GetProcAddress(DllKernel32.hDll, "GetVersionExW");
    DllKernel32.pIsWow64Process = (PIS_WOW64_PROCESS)GetProcAddress(DllKernel32.hDll, "IsWow64Process");
    DllKernel32.pRegisterApplicationRestart = (PREGISTER_APPLICATION_RESTART)GetProcAddress(DllKernel32.hDll, "RegisterApplicationRestart");
    DllKernel32.pSetConsoleScreenBufferInfoEx = (PSET_CONSOLE_SCREEN_BUFFER_INFO_EX)GetProcAddress(DllKernel32.hDll, "SetConsoleScreenBufferInfoEx");
    DllKernel32.pSetCurrentConsoleFontEx = (PSET_CURRENT_CONSOLE_FONT_EX)GetProcAddress(DllKernel32.hDll, "SetCurrentConsoleFontEx");
    DllKernel32.pSetInformationJobObject = (PSET_INFORMATION_JOB_OBJECT)GetProcAddress(DllKernel32.hDll, "SetInformationJobObject");
    DllKernel32.pWow64DisableWow64FsRedirection = (PWOW64_DISABLE_WOW64_FS_REDIRECTION)GetProcAddress(DllKernel32.hDll, "Wow64DisableWow64FsRedirection");

    return TRUE;
}

/**
 A structure containing pointers to advapi32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_ADVAPI32_FUNCTIONS DllAdvApi32;

/**
 Load pointers to all optional advapi32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadAdvApi32Functions()
{
    if (DllAdvApi32.hDll != NULL) {
        return TRUE;
    }

    DllAdvApi32.hDll = LoadLibrary(_T("ADVAPI32.DLL"));
    if (DllAdvApi32.hDll == NULL) {
        return FALSE;
    }

    DllAdvApi32.pAccessCheck = (PACCESS_CHECK)GetProcAddress(DllAdvApi32.hDll, "AccessCheck");
    DllAdvApi32.pAdjustTokenPrivileges = (PADJUST_TOKEN_PRIVILEGES)GetProcAddress(DllAdvApi32.hDll, "AdjustTokenPrivileges");
    DllAdvApi32.pCheckTokenMembership = (PCHECK_TOKEN_MEMBERSHIP)GetProcAddress(DllAdvApi32.hDll, "CheckTokenMembership");
    DllAdvApi32.pGetFileSecurityW = (PGET_FILE_SECURITYW)GetProcAddress(DllAdvApi32.hDll, "GetFileSecurityW");
    DllAdvApi32.pGetSecurityDescriptorOwner = (PGET_SECURITY_DESCRIPTOR_OWNER)GetProcAddress(DllAdvApi32.hDll, "GetSecurityDescriptorOwner");
    DllAdvApi32.pImpersonateSelf = (PIMPERSONATE_SELF)GetProcAddress(DllAdvApi32.hDll, "ImpersonateSelf");
    DllAdvApi32.pInitializeAcl = (PINITIALIZE_ACL)GetProcAddress(DllAdvApi32.hDll, "InitializeAcl");
    DllAdvApi32.pLookupAccountNameW = (PLOOKUP_ACCOUNT_NAMEW)GetProcAddress(DllAdvApi32.hDll, "LookupAccountNameW");
    DllAdvApi32.pLookupAccountSidW = (PLOOKUP_ACCOUNT_SIDW)GetProcAddress(DllAdvApi32.hDll, "LookupAccountSidW");
    DllAdvApi32.pLookupPrivilegeValueW = (PLOOKUP_PRIVILEGE_VALUEW)GetProcAddress(DllAdvApi32.hDll, "LookupPrivilegeValueW");
    DllAdvApi32.pOpenProcessToken = (POPEN_PROCESS_TOKEN)GetProcAddress(DllAdvApi32.hDll, "OpenProcessToken");
    DllAdvApi32.pOpenThreadToken = (POPEN_THREAD_TOKEN)GetProcAddress(DllAdvApi32.hDll, "OpenThreadToken");
    DllAdvApi32.pRevertToSelf = (PREVERT_TO_SELF)GetProcAddress(DllAdvApi32.hDll, "RevertToSelf");
    DllAdvApi32.pSetNamedSecurityInfoW = (PSET_NAMED_SECURITY_INFOW)GetProcAddress(DllAdvApi32.hDll, "SetNamedSecurityInfoW");

    return TRUE;
}

/**
 A structure containing pointers to cabinet.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_CABINET_FUNCTIONS DllCabinet;

/**
 Load pointers to all optional cabinet.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadCabinetFunctions()
{
    if (DllCabinet.hDll != NULL) {
        return TRUE;
    }

    DllCabinet.hDll = LoadLibrary(_T("CABINET.DLL"));
    if (DllCabinet.hDll == NULL) {
        return FALSE;
    }

    DllCabinet.pFciAddFile = (PCAB_FCI_ADD_FILE)GetProcAddress(DllCabinet.hDll, "FCIAddFile");
    DllCabinet.pFciCreate = (PCAB_FCI_CREATE)GetProcAddress(DllCabinet.hDll, "FCICreate");
    DllCabinet.pFciDestroy = (PCAB_FCI_DESTROY)GetProcAddress(DllCabinet.hDll, "FCIDestroy");
    DllCabinet.pFciFlushCabinet = (PCAB_FCI_FLUSH_CABINET)GetProcAddress(DllCabinet.hDll, "FCIFlushCabinet");
    DllCabinet.pFciFlushFolder = (PCAB_FCI_FLUSH_FOLDER)GetProcAddress(DllCabinet.hDll, "FCIFlushFolder");
    DllCabinet.pFdiCreate = (PCAB_FDI_CREATE)GetProcAddress(DllCabinet.hDll, "FDICreate");
    DllCabinet.pFdiCopy = (PCAB_FDI_COPY)GetProcAddress(DllCabinet.hDll, "FDICopy");
    DllCabinet.pFdiDestroy = (PCAB_FDI_DESTROY)GetProcAddress(DllCabinet.hDll, "FDIDestroy");
    return TRUE;
}

/**
 A structure containing pointers to ole32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_OLE32_FUNCTIONS DllOle32;

/**
 Load pointers to all optional ole32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadOle32Functions()
{

    if (DllOle32.hDll != NULL) {
        return TRUE;
    }

    DllOle32.hDll = LoadLibrary(_T("OLE32.DLL"));
    if (DllOle32.hDll == NULL) {
        return FALSE;
    }

    DllOle32.pCoCreateInstance = (PCO_CREATE_INSTANCE)GetProcAddress(DllOle32.hDll, "CoCreateInstance");
    DllOle32.pCoInitialize = (PCO_INITIALIZE)GetProcAddress(DllOle32.hDll, "CoInitialize");
    DllOle32.pCoTaskMemFree = (PCO_TASK_MEM_FREE)GetProcAddress(DllOle32.hDll, "CoTaskMemFree");

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
YORI_SHELL32_FUNCTIONS DllShell32;

/**
 Load pointers to all optional shell32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadShell32Functions()
{

    if (DllShell32.hDll != NULL) {
        return TRUE;
    }

    DllShell32.hDll = LoadLibrary(_T("SHELL32.DLL"));
    if (DllShell32.hDll == NULL) {
        return FALSE;
    }

    DllShell32.pSHFileOperationW = (PSH_FILE_OPERATIONW)GetProcAddress(DllShell32.hDll, "SHFileOperationW");
    DllShell32.pSHGetKnownFolderPath = (PSH_GET_KNOWN_FOLDER_PATH)GetProcAddress(DllShell32.hDll, "SHGetKnownFolderPath");
    DllShell32.pSHGetSpecialFolderPathW = (PSH_GET_SPECIAL_FOLDER_PATHW)GetProcAddress(DllShell32.hDll, "SHGetSpecialFolderPathW");
    DllShell32.pShellExecuteExW = (PSHELL_EXECUTE_EXW)GetProcAddress(DllShell32.hDll, "ShellExecuteExW");
    DllShell32.pShellExecuteW = (PSHELL_EXECUTEW)GetProcAddress(DllShell32.hDll, "ShellExecuteW");
    return TRUE;
}

/**
 A structure containing pointers to user32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_USER32_FUNCTIONS DllUser32;

/**
 Load pointers to all optional user32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadUser32Functions()
{
    if (DllUser32.hDll != NULL) {
        return TRUE;
    }

    DllUser32.hDll = LoadLibrary(_T("USER32.DLL"));
    if (DllUser32.hDll == NULL) {
        return FALSE;
    }

    DllUser32.pCloseClipboard = (PCLOSE_CLIPBOARD)GetProcAddress(DllUser32.hDll, "CloseClipboard");
    DllUser32.pEmptyClipboard = (PEMPTY_CLIPBOARD)GetProcAddress(DllUser32.hDll, "EmptyClipboard");
    DllUser32.pGetClipboardData = (PGET_CLIPBOARD_DATA)GetProcAddress(DllUser32.hDll, "GetClipboardData");
    DllUser32.pOpenClipboard = (POPEN_CLIPBOARD)GetProcAddress(DllUser32.hDll, "OpenClipboard");
    DllUser32.pRegisterClipboardFormatW = (PREGISTER_CLIPBOARD_FORMATW)GetProcAddress(DllUser32.hDll, "RegisterClipboardFormatW");
    DllUser32.pSetClipboardData = (PSET_CLIPBOARD_DATA)GetProcAddress(DllUser32.hDll, "SetClipboardData");

    return TRUE;
}

/**
 A structure containing pointers to version.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_VERSION_FUNCTIONS DllVersion;

/**
 Load pointers to all optional version.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadVersionFunctions()
{

    if (DllVersion.hDll != NULL) {
        return TRUE;
    }

    DllVersion.hDll = LoadLibrary(_T("VERSION.DLL"));
    if (DllVersion.hDll == NULL) {
        return FALSE;
    }

    DllVersion.pGetFileVersionInfoSizeW = (PGET_FILE_VERSION_INFO_SIZEW)GetProcAddress(DllVersion.hDll, "GetFileVersionInfoSizeW");
    DllVersion.pGetFileVersionInfoW = (PGET_FILE_VERSION_INFOW)GetProcAddress(DllVersion.hDll, "GetFileVersionInfoW");
    DllVersion.pVerQueryValueW = (PVER_QUERY_VALUEW)GetProcAddress(DllVersion.hDll, "VerQueryValueW");

    return TRUE;
}


// vim:sw=4:ts=4:et:
