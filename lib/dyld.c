/**
 * @file lib/dyld.c
 *
 * Yori dynamically loaded OS function support
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
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
 Map a function name to a location in memory to store a function pointer.
 */
typedef struct _YORI_DLL_NAME_MAP {

    /**
     Pointer to a memory location to be updated with a function pointer when
     the specified function is resolved.
     */
    FARPROC * FnPtr;

    /**
     The name of the function to resolve.
     */
    LPCSTR FnName;
} YORI_DLL_NAME_MAP, *PYORI_DLL_NAME_MAP;

/**
 Convert a file name into a fully specified path to the System32 directory.

 @param FileName Pointer to a string containing the file name component.

 @param FullPath On successful completion, updated to contain a newly
        allocated string that is fully specified to the System32 directory.
        The caller is expected to free this with
        @ref YoriLibFreeStringContents.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriLibFullPathToSystemDirectory(
    __in PCYORI_STRING FileName,
    __out PYORI_STRING FullPath
    )
{
    YORI_ALLOC_SIZE_T LengthRequired;
    LengthRequired = (YORI_ALLOC_SIZE_T)GetSystemDirectory(NULL, 0);

    if (!YoriLibAllocateString(FullPath, LengthRequired + 1 + FileName->LengthInChars + 1)) {
        return FALSE;
    }

    LengthRequired = (YORI_ALLOC_SIZE_T)GetSystemDirectory(FullPath->StartOfString, LengthRequired);
    if (LengthRequired == 0) {
        YoriLibFreeStringContents(FullPath);
        return FALSE;
    }

    FullPath->LengthInChars = LengthRequired + YoriLibSPrintf(&FullPath->StartOfString[LengthRequired], _T("\\%y"), FileName);
    return TRUE;
}

/**
 Load a DLL from the System32 directory.

 @param DllName Pointer to the name of the DLL to load.

 @return Module to the DLL, or NULL on failure.
 */
HMODULE
YoriLibLoadLibraryFromSystemDirectory(
    __in LPCTSTR DllName
    )
{
    YORI_STRING YsDllName;
    YORI_STRING FullPath;
    HMODULE DllModule;

    YoriLibConstantString(&YsDllName, DllName);
    if (!YoriLibFullPathToSystemDirectory(&YsDllName, &FullPath)) {
        return NULL;
    }

    DllModule = NULL;
    if (DllKernel32.pLoadLibraryW != NULL) {
        DllModule = DllKernel32.pLoadLibraryW(FullPath.StartOfString);
    } else if (DllKernel32.pLoadLibraryExW != NULL) {
        DllModule = DllKernel32.pLoadLibraryExW(FullPath.StartOfString, NULL, 0);
    }

    YoriLibFreeStringContents(&FullPath);
    return DllModule;
}

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
YoriLibLoadNtDllFunctions(VOID)
{
    if (DllNtDll.hDll != NULL) {
        return TRUE;
    }

    DllNtDll.hDll = GetModuleHandle(_T("NTDLL"));
    if (DllNtDll.hDll == NULL) {
        return FALSE;
    }
    DllNtDll.pNtOpenDirectoryObject = (PNT_OPEN_DIRECTORY_OBJECT)GetProcAddress(DllNtDll.hDll, "NtOpenDirectoryObject");
    DllNtDll.pNtOpenSymbolicLinkObject = (PNT_OPEN_SYMBOLIC_LINK_OBJECT)GetProcAddress(DllNtDll.hDll, "NtOpenSymbolicLinkObject");
    DllNtDll.pNtQueryDirectoryObject = (PNT_QUERY_DIRECTORY_OBJECT)GetProcAddress(DllNtDll.hDll, "NtQueryDirectoryObject");
    DllNtDll.pNtQueryInformationFile = (PNT_QUERY_INFORMATION_FILE)GetProcAddress(DllNtDll.hDll, "NtQueryInformationFile");
    DllNtDll.pNtQueryInformationProcess = (PNT_QUERY_INFORMATION_PROCESS)GetProcAddress(DllNtDll.hDll, "NtQueryInformationProcess");
    DllNtDll.pNtQueryInformationThread = (PNT_QUERY_INFORMATION_THREAD)GetProcAddress(DllNtDll.hDll, "NtQueryInformationThread");
    DllNtDll.pNtQueryObject = (PNT_QUERY_OBJECT)GetProcAddress(DllNtDll.hDll, "NtQueryObject");
    DllNtDll.pNtQuerySymbolicLinkObject = (PNT_QUERY_SYMBOLIC_LINK_OBJECT)GetProcAddress(DllNtDll.hDll, "NtQuerySymbolicLinkObject");
    DllNtDll.pNtQuerySystemInformation = (PNT_QUERY_SYSTEM_INFORMATION)GetProcAddress(DllNtDll.hDll, "NtQuerySystemInformation");
    DllNtDll.pNtSetInformationFile = (PNT_SET_INFORMATION_FILE)GetProcAddress(DllNtDll.hDll, "NtSetInformationFile");
    DllNtDll.pNtSystemDebugControl = (PNT_SYSTEM_DEBUG_CONTROL)GetProcAddress(DllNtDll.hDll, "NtSystemDebugControl");
    DllNtDll.pRtlGetLastNtStatus = (PRTL_GET_LAST_NT_STATUS)GetProcAddress(DllNtDll.hDll, "RtlGetLastNtStatus");
    return TRUE;
}

/**
 A structure containing pointers to kernel32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_KERNEL32_FUNCTIONS DllKernel32;

/**
 The set of functions to resolve from kernel32.dll.
 */
CONST YORI_DLL_NAME_MAP DllKernel32Symbols[] = {
    {(FARPROC *)&DllKernel32.pAddConsoleAliasW, "AddConsoleAliasW"},
    {(FARPROC *)&DllKernel32.pAssignProcessToJobObject, "AssignProcessToJobObject"},
    {(FARPROC *)&DllKernel32.pCopyFileW, "CopyFileW"},
    {(FARPROC *)&DllKernel32.pCopyFileExW, "CopyFileExW"},
    {(FARPROC *)&DllKernel32.pCreateHardLinkW, "CreateHardLinkW"},
    {(FARPROC *)&DllKernel32.pCreateJobObjectW, "CreateJobObjectW"},
    {(FARPROC *)&DllKernel32.pCreateSymbolicLinkW, "CreateSymbolicLinkW"},
    {(FARPROC *)&DllKernel32.pFindFirstStreamW, "FindFirstStreamW"},
    {(FARPROC *)&DllKernel32.pFindFirstVolumeW, "FindFirstVolumeW"},
    {(FARPROC *)&DllKernel32.pFindNextStreamW, "FindNextStreamW"},
    {(FARPROC *)&DllKernel32.pFindNextVolumeW, "FindNextVolumeW"},
    {(FARPROC *)&DllKernel32.pFindVolumeClose, "FindVolumeClose"},
    {(FARPROC *)&DllKernel32.pFreeEnvironmentStringsW, "FreeEnvironmentStringsW"},
    {(FARPROC *)&DllKernel32.pGetCompressedFileSizeW, "GetCompressedFileSizeW"},
    {(FARPROC *)&DllKernel32.pGetConsoleAliasesLengthW, "GetConsoleAliasesLengthW"},
    {(FARPROC *)&DllKernel32.pGetConsoleAliasesW, "GetConsoleAliasesW"},
    {(FARPROC *)&DllKernel32.pGetConsoleDisplayMode, "GetConsoleDisplayMode"},
    {(FARPROC *)&DllKernel32.pGetConsoleScreenBufferInfoEx, "GetConsoleScreenBufferInfoEx"},
    {(FARPROC *)&DllKernel32.pGetConsoleProcessList, "GetConsoleProcessList"},
    {(FARPROC *)&DllKernel32.pGetConsoleWindow, "GetConsoleWindow"},
    {(FARPROC *)&DllKernel32.pGetCurrentConsoleFontEx, "GetCurrentConsoleFontEx"},
    {(FARPROC *)&DllKernel32.pGetDiskFreeSpaceExW, "GetDiskFreeSpaceExW"},
    {(FARPROC *)&DllKernel32.pGetEnvironmentStrings, "GetEnvironmentStrings"},
    {(FARPROC *)&DllKernel32.pGetEnvironmentStringsW, "GetEnvironmentStringsW"},
    {(FARPROC *)&DllKernel32.pGetFileInformationByHandleEx, "GetFileInformationByHandleEx"},
    {(FARPROC *)&DllKernel32.pGetFinalPathNameByHandleW, "GetFinalPathNameByHandleW"},
    {(FARPROC *)&DllKernel32.pGetLargestConsoleWindowSize, "GetLargestConsoleWindowSize"},
    {(FARPROC *)&DllKernel32.pGetLogicalProcessorInformation, "GetLogicalProcessorInformation"},
    {(FARPROC *)&DllKernel32.pGetLogicalProcessorInformationEx, "GetLogicalProcessorInformationEx"},
    {(FARPROC *)&DllKernel32.pGetNativeSystemInfo, "GetNativeSystemInfo"},
    {(FARPROC *)&DllKernel32.pGetPrivateProfileIntW, "GetPrivateProfileIntW"},
    {(FARPROC *)&DllKernel32.pGetPrivateProfileSectionW, "GetPrivateProfileSectionW"},
    {(FARPROC *)&DllKernel32.pGetPrivateProfileSectionNamesW, "GetPrivateProfileSectionNamesW"},
    {(FARPROC *)&DllKernel32.pGetPrivateProfileStringW, "GetPrivateProfileStringW"},
    {(FARPROC *)&DllKernel32.pGetProcessIoCounters, "GetProcessIoCounters"},
    {(FARPROC *)&DllKernel32.pGetProductInfo, "GetProductInfo"},
    {(FARPROC *)&DllKernel32.pGetSystemPowerStatus, "GetSystemPowerStatus"},
    {(FARPROC *)&DllKernel32.pGetTickCount64, "GetTickCount64"},
    {(FARPROC *)&DllKernel32.pGetVersionExW, "GetVersionExW"},
    {(FARPROC *)&DllKernel32.pGetVolumePathNamesForVolumeNameW, "GetVolumePathNamesForVolumeNameW"},
    {(FARPROC *)&DllKernel32.pGetVolumePathNameW, "GetVolumePathNameW"},
    {(FARPROC *)&DllKernel32.pGlobalLock, "GlobalLock"},
    {(FARPROC *)&DllKernel32.pGlobalMemoryStatus, "GlobalMemoryStatus"},
    {(FARPROC *)&DllKernel32.pGlobalMemoryStatusEx, "GlobalMemoryStatusEx"},
    {(FARPROC *)&DllKernel32.pGlobalSize, "GlobalSize"},
    {(FARPROC *)&DllKernel32.pGlobalUnlock, "GlobalUnlock"},
    {(FARPROC *)&DllKernel32.pInterlockedCompareExchange, "InterlockedCompareExchange"},
    {(FARPROC *)&DllKernel32.pIsWow64Process, "IsWow64Process"},
    {(FARPROC *)&DllKernel32.pIsWow64Process2, "IsWow64Process2"},
    {(FARPROC *)&DllKernel32.pLoadLibraryW, "LoadLibraryW"},
    {(FARPROC *)&DllKernel32.pLoadLibraryExW, "LoadLibraryExW"},
    {(FARPROC *)&DllKernel32.pOpenThread, "OpenThread"},
    {(FARPROC *)&DllKernel32.pQueryFullProcessImageNameW, "QueryFullProcessImageNameW"},
    {(FARPROC *)&DllKernel32.pQueryInformationJobObject, "QueryInformationJobObject"},
    {(FARPROC *)&DllKernel32.pRegisterApplicationRestart, "RegisterApplicationRestart"},
    {(FARPROC *)&DllKernel32.pReplaceFileW, "ReplaceFileW"},
    {(FARPROC *)&DllKernel32.pRtlCaptureStackBackTrace, "RtlCaptureStackBackTrace"},
    {(FARPROC *)&DllKernel32.pSetConsoleDisplayMode, "SetConsoleDisplayMode"},
    {(FARPROC *)&DllKernel32.pSetConsoleScreenBufferInfoEx, "SetConsoleScreenBufferInfoEx"},
    {(FARPROC *)&DllKernel32.pSetConsoleScreenBufferSize, "SetConsoleScreenBufferSize"},
    {(FARPROC *)&DllKernel32.pSetCurrentConsoleFontEx, "SetCurrentConsoleFontEx"},
    {(FARPROC *)&DllKernel32.pSetFileInformationByHandle, "SetFileInformationByHandle"},
    {(FARPROC *)&DllKernel32.pSetInformationJobObject, "SetInformationJobObject"},
    {(FARPROC *)&DllKernel32.pSetSystemPowerState, "SetSystemPowerState"},
    {(FARPROC *)&DllKernel32.pWritePrivateProfileStringW, "WritePrivateProfileStringW"},
    {(FARPROC *)&DllKernel32.pWow64DisableWow64FsRedirection, "Wow64DisableWow64FsRedirection"},
    {(FARPROC *)&DllKernel32.pWow64GetThreadContext, "Wow64GetThreadContext"},
    {(FARPROC *)&DllKernel32.pWow64SetThreadContext, "Wow64SetThreadContext"},
};

/**
 Try to resolve function pointers for kernel32 functions which could be in
 the specified DLL module.

 @param hDll The DLL to load functions from.
 */
VOID
YoriLibLoadKernel32FunctionsFromDll(
    __in HMODULE hDll
    )
{
    DWORD Count;

    for (Count = 0; Count < sizeof(DllKernel32Symbols)/sizeof(DllKernel32Symbols[0]); Count++) {
        if (*(DllKernel32Symbols[Count].FnPtr) == NULL) {
            *(DllKernel32Symbols[Count].FnPtr) = GetProcAddress(hDll, DllKernel32Symbols[Count].FnName);
        }
    }
}


/**
 Load pointers to all optional kernel32.dll functions.  Because kernel32.dll is
 effectively mandatory in any Win32 process, this uses GetModuleHandle rather
 than LoadLibrary and pointers are valid for the lifetime of the process.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadKernel32Functions(VOID)
{
    if (DllKernel32.hDllKernelBase != NULL ||
        DllKernel32.hDllKernel32 != NULL) {
        return TRUE;
    }

    //
    //  Try to resolve everything that can be resolved against kernelbase
    //  directly.
    //

    DllKernel32.hDllKernelBase = GetModuleHandle(_T("KERNELBASE"));
    if (DllKernel32.hDllKernelBase != NULL) {
        YoriLibLoadKernel32FunctionsFromDll(DllKernel32.hDllKernelBase);
    }

    //
    //  On a kernelbase only build, kernel32 is not part of the import table.
    //  Nonetheless on mainstream editions it gets mapped into the process
    //  automatically, so GetModuleHandle succeeds.
    //
    //  On editions without kernel32, hopefully this will fail, and it'll
    //  be forced to load and probe the legacy DLL instead.
    //

    DllKernel32.hDllKernel32 = GetModuleHandle(_T("KERNEL32"));
    if (DllKernel32.hDllKernel32 != NULL) {
        YoriLibLoadKernel32FunctionsFromDll(DllKernel32.hDllKernel32);
    } else {
        DllKernel32.hDllKernel32Legacy = YoriLibLoadLibraryFromSystemDirectory(_T("KERNEL32LEGACY.DLL"));
        if (DllKernel32.hDllKernel32Legacy != NULL) {
            YoriLibLoadKernel32FunctionsFromDll(DllKernel32.hDllKernel32Legacy);
        }
    }


    return TRUE;
}

/**
 A structure containing pointers to crypt32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_CRYPT32_FUNCTIONS DllCrypt32;

/**
 Load pointers to all optional crypt32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadCrypt32Functions(VOID)
{
    if (DllCrypt32.hDll != NULL) {
        return TRUE;
    }

    DllCrypt32.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("CRYPT32.DLL"));
    if (DllCrypt32.hDll == NULL) {
        return FALSE;
    }

    DllCrypt32.pCryptBinaryToStringW = (PCRYPT_BINARY_TO_STRINGW)GetProcAddress(DllCrypt32.hDll, "CryptBinaryToStringW");
    DllCrypt32.pCryptStringToBinaryW = (PCRYPT_STRING_TO_BINARYW)GetProcAddress(DllCrypt32.hDll, "CryptStringToBinaryW");

    return TRUE;
}

/**
 A structure containing pointers to ctl3d32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_CTL3D_FUNCTIONS DllCtl3d;

/**
 Load pointers to all optional Ctl3d32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadCtl3d32Functions(VOID)
{

    if (DllCtl3d.hDll != NULL) {
        return TRUE;
    }

    DllCtl3d.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("CTL3D32.DLL"));
    if (DllCtl3d.hDll == NULL) {
        return FALSE;
    }

    DllCtl3d.pCtl3dAutoSubclass = (PCTL3D_AUTOSUBCLASS)GetProcAddress(DllCtl3d.hDll, "Ctl3dAutoSubclass");
    DllCtl3d.pCtl3dRegister = (PCTL3D_REGISTER)GetProcAddress(DllCtl3d.hDll, "Ctl3dRegister");

    return TRUE;
}

/**
 A structure containing pointers to dbghelp.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_DBGHELP_FUNCTIONS DllDbgHelp;

/**
 Load pointers to all optional dbghelp.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadDbgHelpFunctions(VOID)
{

    if (DllDbgHelp.hDll != NULL) {
        return TRUE;
    }

    DllDbgHelp.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("DBGHELP.DLL"));
    if (DllDbgHelp.hDll == NULL) {
        return FALSE;
    }

    DllDbgHelp.pMiniDumpWriteDump = (PMINI_DUMP_WRITE_DUMP)GetProcAddress(DllDbgHelp.hDll, "MiniDumpWriteDump");

    return TRUE;
}

/**
 A structure containing pointers to imagehlp.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_IMAGEHLP_FUNCTIONS DllImageHlp;

/**
 Load pointers to all optional imagehlp.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadImageHlpFunctions(VOID)
{

    if (DllImageHlp.hDll != NULL) {
        return TRUE;
    }

    DllImageHlp.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("IMAGEHLP.DLL"));
    if (DllImageHlp.hDll == NULL) {
        return FALSE;
    }

    DllImageHlp.pCheckSumMappedFile = (PCHECK_SUM_MAPPED_FILE)GetProcAddress(DllImageHlp.hDll, "CheckSumMappedFile");
    DllImageHlp.pMapFileAndCheckSumW = (PMAP_FILE_AND_CHECKSUMW)GetProcAddress(DllImageHlp.hDll, "MapFileAndCheckSumW");

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
YoriLibLoadOle32Functions(VOID)
{

    if (DllOle32.hDll != NULL) {
        return TRUE;
    }

    DllOle32.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("OLE32.DLL"));
    if (DllOle32.hDll == NULL) {
        return FALSE;
    }

    DllOle32.pCoCreateInstance = (PCO_CREATE_INSTANCE)GetProcAddress(DllOle32.hDll, "CoCreateInstance");
    DllOle32.pCoInitialize = (PCO_INITIALIZE)GetProcAddress(DllOle32.hDll, "CoInitialize");
    DllOle32.pCoLockObjectExternal = (PCO_LOCK_OBJECT_EXTERNAL)GetProcAddress(DllOle32.hDll, "CoLockObjectExternal");
    DllOle32.pCoTaskMemFree = (PCO_TASK_MEM_FREE)GetProcAddress(DllOle32.hDll, "CoTaskMemFree");
    DllOle32.pOleInitialize = (POLE_INITIALIZE)GetProcAddress(DllOle32.hDll, "OleInitialize");
    DllOle32.pOleUninitialize = (POLE_UNINITIALIZE)GetProcAddress(DllOle32.hDll, "OleUninitialize");
    DllOle32.pRegisterDragDrop = (PREGISTER_DRAG_DROP)GetProcAddress(DllOle32.hDll, "RegisterDragDrop");
    DllOle32.pRevokeDragDrop = (PREVOKE_DRAG_DROP)GetProcAddress(DllOle32.hDll, "RevokeDragDrop");

    return TRUE;
}

/**
 A structure containing pointers to powrprof.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_POWRPROF_FUNCTIONS DllPowrprof;

/**
 Load pointers to all optional powrprof.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadPowrprofFunctions(VOID)
{

    if (DllPowrprof.hDll != NULL) {
        return TRUE;
    }

    DllPowrprof.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("POWRPROF.DLL"));
    if (DllPowrprof.hDll == NULL) {
        return FALSE;
    }

    DllPowrprof.pIsPwrHibernateAllowed = (PIS_PWR_SUSPEND_ALLOWED)GetProcAddress(DllPowrprof.hDll, "IsPwrHibernateAllowed");
    DllPowrprof.pIsPwrSuspendAllowed = (PIS_PWR_SUSPEND_ALLOWED)GetProcAddress(DllPowrprof.hDll, "IsPwrSuspendAllowed");
    DllPowrprof.pSetSuspendState = (PSET_SUSPEND_STATE)GetProcAddress(DllPowrprof.hDll, "SetSuspendState");

    return TRUE;
}

/**
 A structure containing pointers to psapi.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_PSAPI_FUNCTIONS DllPsapi;

/**
 Load pointers to all optional psapi.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadPsapiFunctions(VOID)
{

    if (DllPsapi.hDll != NULL) {
        return TRUE;
    }

    DllPsapi.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("PSAPI.DLL"));
    if (DllPsapi.hDll == NULL) {
        return FALSE;
    }

    DllPsapi.pGetModuleFileNameExW = (PGET_MODULE_FILE_NAME_EXW)GetProcAddress(DllPsapi.hDll, "GetModuleFileNameExW");

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
YoriLibLoadShell32Functions(VOID)
{

    if (DllShell32.hDll != NULL) {
        return TRUE;
    }

    DllShell32.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("SHELL32.DLL"));
    if (DllShell32.hDll == NULL) {
        return FALSE;
    }

    DllShell32.pSHBrowseForFolderW = (PSH_BROWSE_FOR_FOLDERW)GetProcAddress(DllShell32.hDll, "SHBrowseForFolderW");
    DllShell32.pSHFileOperationW = (PSH_FILE_OPERATIONW)GetProcAddress(DllShell32.hDll, "SHFileOperationW");
    DllShell32.pSHGetKnownFolderPath = (PSH_GET_KNOWN_FOLDER_PATH)GetProcAddress(DllShell32.hDll, "SHGetKnownFolderPath");
    DllShell32.pSHGetPathFromIDListW = (PSH_GET_PATH_FROM_ID_LISTW)GetProcAddress(DllShell32.hDll, "SHGetPathFromIDListW");
    DllShell32.pSHGetSpecialFolderPathW = (PSH_GET_SPECIAL_FOLDER_PATHW)GetProcAddress(DllShell32.hDll, "SHGetSpecialFolderPathW");
    DllShell32.pShellExecuteExW = (PSHELL_EXECUTE_EXW)GetProcAddress(DllShell32.hDll, "ShellExecuteExW");
    DllShell32.pShellExecuteW = (PSHELL_EXECUTEW)GetProcAddress(DllShell32.hDll, "ShellExecuteW");
    return TRUE;
}

/**
 A structure containing pointers to shfolder.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_SHFOLDER_FUNCTIONS DllShfolder;

/**
 Load pointers to all optional shfolder.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadShfolderFunctions(VOID)
{

    if (DllShfolder.hDll != NULL) {
        return TRUE;
    }

    DllShfolder.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("SHFOLDER.DLL"));
    if (DllShfolder.hDll == NULL) {
        return FALSE;
    }

    DllShfolder.pSHGetFolderPathW = (PSH_GET_FOLDER_PATHW)GetProcAddress(DllShfolder.hDll, "SHGetFolderPathW");
    return TRUE;
}

/**
 A structure containing pointers to userenv.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_USERENV_FUNCTIONS DllUserEnv;

/**
 Load pointers to all optional userenv.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadUserEnvFunctions(VOID)
{

    if (DllUserEnv.hDll != NULL) {
        return TRUE;
    }

    DllUserEnv.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("USERENV.DLL"));
    if (DllUserEnv.hDll == NULL) {
        return FALSE;
    }

    DllUserEnv.pCreateEnvironmentBlock = (PCREATE_ENVIRONMENT_BLOCK)GetProcAddress(DllUserEnv.hDll, "CreateEnvironmentBlock");
    DllUserEnv.pDestroyEnvironmentBlock = (PDESTROY_ENVIRONMENT_BLOCK)GetProcAddress(DllUserEnv.hDll, "DestroyEnvironmentBlock");
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
YoriLibLoadVersionFunctions(VOID)
{

    if (DllVersion.hDll != NULL) {
        return TRUE;
    }

    DllVersion.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("VERSION.DLL"));
    if (DllVersion.hDll == NULL) {
        return FALSE;
    }

    DllVersion.pGetFileVersionInfoSizeW = (PGET_FILE_VERSION_INFO_SIZEW)GetProcAddress(DllVersion.hDll, "GetFileVersionInfoSizeW");
    DllVersion.pGetFileVersionInfoW = (PGET_FILE_VERSION_INFOW)GetProcAddress(DllVersion.hDll, "GetFileVersionInfoW");
    DllVersion.pVerQueryValueW = (PVER_QUERY_VALUEW)GetProcAddress(DllVersion.hDll, "VerQueryValueW");

    return TRUE;
}

/**
 GUID for an unknown virtual storage implementations.
 */
const GUID VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN = { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

/**
 GUID for Microsoft provided virtual storage implementations.
 */
const GUID VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT = { 0xec984aec, 0xa0f9, 0x47e9, { 0x90, 0x1f, 0x71, 0x41, 0x5a, 0x66, 0x34, 0x5b } };

/**
 A structure containing pointers to virtdisk.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_VIRTDISK_FUNCTIONS DllVirtDisk;

/**
 Load pointers to all optional virtdisk.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadVirtDiskFunctions(VOID)
{

    if (DllVirtDisk.hDll != NULL) {
        return TRUE;
    }

    DllVirtDisk.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("VIRTDISK.DLL"));
    if (DllVirtDisk.hDll == NULL) {
        return FALSE;
    }

    DllVirtDisk.pAttachVirtualDisk = (PATTACH_VIRTUAL_DISK)GetProcAddress(DllVirtDisk.hDll, "AttachVirtualDisk");
    DllVirtDisk.pCompactVirtualDisk = (PCOMPACT_VIRTUAL_DISK)GetProcAddress(DllVirtDisk.hDll, "CompactVirtualDisk");
    DllVirtDisk.pCreateVirtualDisk = (PCREATE_VIRTUAL_DISK)GetProcAddress(DllVirtDisk.hDll, "CreateVirtualDisk");
    DllVirtDisk.pDetachVirtualDisk = (PDETACH_VIRTUAL_DISK)GetProcAddress(DllVirtDisk.hDll, "DetachVirtualDisk");
    DllVirtDisk.pExpandVirtualDisk = (PEXPAND_VIRTUAL_DISK)GetProcAddress(DllVirtDisk.hDll, "ExpandVirtualDisk");
    DllVirtDisk.pGetVirtualDiskPhysicalPath = (PGET_VIRTUAL_DISK_PHYSICAL_PATH)GetProcAddress(DllVirtDisk.hDll, "GetVirtualDiskPhysicalPath");
    DllVirtDisk.pOpenVirtualDisk = (POPEN_VIRTUAL_DISK)GetProcAddress(DllVirtDisk.hDll, "OpenVirtualDisk");
    DllVirtDisk.pMergeVirtualDisk = (PMERGE_VIRTUAL_DISK)GetProcAddress(DllVirtDisk.hDll, "MergeVirtualDisk");
    DllVirtDisk.pResizeVirtualDisk = (PRESIZE_VIRTUAL_DISK)GetProcAddress(DllVirtDisk.hDll, "ResizeVirtualDisk");

    return TRUE;
}

/**
 A structure containing pointers to winbrand.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_WINBRAND_FUNCTIONS DllWinBrand;

/**
 Load pointers to all optional WinBrand.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadWinBrandFunctions(VOID)
{

    if (DllWinBrand.hDll != NULL) {
        return TRUE;
    }

    DllWinBrand.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("WINBRAND.DLL"));
    if (DllWinBrand.hDll == NULL) {
        return FALSE;
    }

    DllWinBrand.pBrandingFormatString = (PBRANDING_FORMAT_STRING)GetProcAddress(DllWinBrand.hDll, "BrandingFormatString");
    return TRUE;
}

/**
 A structure containing pointers to wlanapi.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_WLANAPI_FUNCTIONS DllWlanApi;

/**
 Load pointers to all optional WlanApi.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadWlanApiFunctions(VOID)
{

    if (DllWlanApi.hDll != NULL) {
        return TRUE;
    }

    DllWlanApi.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("WLANAPI.DLL"));
    if (DllWlanApi.hDll == NULL) {
        return FALSE;
    }

    DllWlanApi.pWlanCloseHandle = (PWLAN_CLOSE_HANDLE)GetProcAddress(DllWlanApi.hDll, "WlanCloseHandle");
    DllWlanApi.pWlanConnect = (PWLAN_CONNECT)GetProcAddress(DllWlanApi.hDll, "WlanConnect");
    DllWlanApi.pWlanDisconnect = (PWLAN_DISCONNECT)GetProcAddress(DllWlanApi.hDll, "WlanDisconnect");
    DllWlanApi.pWlanEnumInterfaces = (PWLAN_ENUM_INTERFACES)GetProcAddress(DllWlanApi.hDll, "WlanEnumInterfaces");
    DllWlanApi.pWlanFreeMemory = (PWLAN_FREE_MEMORY)GetProcAddress(DllWlanApi.hDll, "WlanFreeMemory");
    DllWlanApi.pWlanGetAvailableNetworkList = (PWLAN_GET_AVAILABLE_NETWORK_LIST)GetProcAddress(DllWlanApi.hDll, "WlanGetAvailableNetworkList");
    DllWlanApi.pWlanOpenHandle = (PWLAN_OPEN_HANDLE)GetProcAddress(DllWlanApi.hDll, "WlanOpenHandle");
    DllWlanApi.pWlanRegisterNotification = (PWLAN_REGISTER_NOTIFICATION)GetProcAddress(DllWlanApi.hDll, "WlanRegisterNotification");
    DllWlanApi.pWlanScan = (PWLAN_SCAN)GetProcAddress(DllWlanApi.hDll, "WlanScan");
    return TRUE;
}


/**
 A structure containing pointers to wsock32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_WSOCK32_FUNCTIONS DllWsock32;

/**
 Load pointers to all optional Wsock32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadWsock32Functions(VOID)
{

    if (DllWsock32.hDll != NULL) {
        return TRUE;
    }

    DllWsock32.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("WSOCK32.DLL"));
    if (DllWsock32.hDll == NULL) {
        return FALSE;
    }

    DllWsock32.pclosesocket = (PCLOSE_SOCKET_FN)GetProcAddress(DllWsock32.hDll, "closesocket");
    DllWsock32.pconnect = (PCONNECT_FN)GetProcAddress(DllWsock32.hDll, "connect");
    DllWsock32.pgethostbyname = (PGETHOSTBYNAME)GetProcAddress(DllWsock32.hDll, "gethostbyname");
    DllWsock32.precv = (PRECV_FN)GetProcAddress(DllWsock32.hDll, "recv");
    DllWsock32.psend = (PSEND_FN)GetProcAddress(DllWsock32.hDll, "send");
    DllWsock32.psocket = (PSOCKET_FN)GetProcAddress(DllWsock32.hDll, "socket");
    DllWsock32.pWSACleanup = (PWSA_CLEANUP)GetProcAddress(DllWsock32.hDll, "WSACleanup");
    DllWsock32.pWSAStartup = (PWSA_STARTUP)GetProcAddress(DllWsock32.hDll, "WSAStartup");

    return TRUE;
}

/**
 A structure containing pointers to wtsapi32.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_WTSAPI32_FUNCTIONS DllWtsApi32;

/**
 Load pointers to all optional WtsApi32.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadWtsApi32Functions(VOID)
{

    if (DllWtsApi32.hDll != NULL) {
        return TRUE;
    }

    DllWtsApi32.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("WTSAPI32.DLL"));
    if (DllWtsApi32.hDll == NULL) {
        return FALSE;
    }

    DllWtsApi32.pWTSDisconnectSession = (PWTS_DISCONNECT_SESSION)GetProcAddress(DllWtsApi32.hDll, "WTSDisconnectSession");
    DllWtsApi32.pWTSRegisterSessionNotification = (PWTS_REGISTER_SESSION_NOTIFICATION)GetProcAddress(DllWtsApi32.hDll, "WTSRegisterSessionNotification");
    DllWtsApi32.pWTSUnRegisterSessionNotification = (PWTS_UNREGISTER_SESSION_NOTIFICATION)GetProcAddress(DllWtsApi32.hDll, "WTSUnRegisterSessionNotification");

    return TRUE;
}


// vim:sw=4:ts=4:et:
