/**
 * @file lsof/lsof.c
 *
 * Yori determine which processes are keeping files open.
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

#include <yoripch.h>
#include <yorilib.h>

/**
 Help text to display to the user.
 */
const
CHAR strLsofHelpText[] =
        "\n"
        "Determine which processes are keeping files open.\n"
        "\n"
        "LSOF [-license] [-b] [-s] <file>...\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -s             Process files from all subdirectories\n";

/**
 Display usage text to the user.
 */
BOOL
LsofHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Lsof %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strLsofHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _LSOF_CONTEXT {

    /**
     Counts the number of files processed in an enumerate.  If this is zero,
     the program assumes the request is to create a new file.
     */
    DWORD FilesFoundThisArg;

    /**
     The number of bytes in the Buffer below.
     */
    DWORD BufferLength;

    /**
     A buffer that is populated with the array of process IDs using a given
     file.
     */
    PFILE_PROCESS_IDS_USING_FILE_INFORMATION Buffer;

} LSOF_CONTEXT, *PLSOF_CONTEXT;

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.  Note in this application this can
        be NULL when it is operating on files that do not yet exist.

 @param Depth Specifies the recursion depth.  Ignored in this application.

 @param Context Pointer to the lsof context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
LsofFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    PLSOF_CONTEXT LsofContext = (PLSOF_CONTEXT)Context;
    IO_STATUS_BLOCK IoStatus;
    DWORD Index;
    LONG Status;

    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FileInfo);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    LsofContext->FilesFoundThisArg++;

    FileHandle = CreateFile(FilePath->StartOfString,
                            FILE_READ_ATTRIBUTES,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        if (LastError == ERROR_ACCESS_DENIED &&
            DllNtDll.pRtlGetLastNtStatus != NULL &&
            DllNtDll.pRtlGetLastNtStatus() == STATUS_DELETE_PENDING) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("lsof: open of %y failed: the file is delete pending\n"), FilePath);
        } else {
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("lsof: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
        }
        return TRUE;
    }

    Status = DllNtDll.pNtQueryInformationFile(FileHandle, &IoStatus, LsofContext->Buffer, LsofContext->BufferLength, FileProcessIdsUsingFileInformation);
    if (Status == 0) {
        for (Index = 0; Index < LsofContext->Buffer->NumberOfProcesses; Index++) {
            HANDLE ProcessHandle;
            TCHAR ProcessName[300];
            DWORD ProcessNameSize;

            ProcessName[0] = '\0';
            ProcessNameSize = sizeof(ProcessName)/sizeof(ProcessName[0]);
            ProcessHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)LsofContext->Buffer->ProcessIds[Index]);
            if (ProcessHandle != NULL) {
                DllKernel32.pQueryFullProcessImageNameW(ProcessHandle, 0, ProcessName, &ProcessNameSize);
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%10i %s\n"), LsofContext->Buffer->ProcessIds[Index], ProcessName);
        }
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("lsof: query of %y failed: %08x"), FilePath, Status);
    }

    CloseHandle(FileHandle);
    return TRUE;
}

/**
 Display information about handles opened for all processes.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
LsofDumpHandles(VOID)
{
    PYORI_SYSTEM_HANDLE_INFORMATION_EX Handles;
    DWORD_PTR Index;
    PYORI_SYSTEM_HANDLE_ENTRY_EX ThisHandle;
    HANDLE LocalProcessHandle;
    PYORI_OBJECT_NAME_INFORMATION ObjectName;
    PYORI_OBJECT_TYPE_INFORMATION ObjectType;
    YORI_STRING ModuleNameString;
    YORI_STRING ObjectNameString;
    YORI_STRING ObjectTypeString;
    DWORD ObjectNameLength;
    DWORD ObjectTypeLength;
    DWORD LengthReturned;
    HANDLE ProcessHandle;
    DWORD LastPid;

    ProcessHandle = INVALID_HANDLE_VALUE;
    LastPid = 0;

    YoriLibLoadPsapiFunctions();

    if (!YoriLibGetSystemHandlesList(&Handles)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("lsof: Error getting system handle list\n"));
        return FALSE;
    }

    ObjectNameLength = 0x10000;
    ObjectName = YoriLibMalloc(ObjectNameLength);
    if (ObjectName == NULL) {
        YoriLibFree(Handles);
        return FALSE;
    }

    ObjectTypeLength = 0x1000;
    ObjectType = YoriLibMalloc(ObjectTypeLength);
    if (ObjectName == NULL) {
        YoriLibFree(ObjectName);
        YoriLibFree(Handles);
        return FALSE;
    }

    if (!YoriLibAllocateString(&ModuleNameString, 0x8000)) {
        YoriLibFree(ObjectType);
        YoriLibFree(ObjectName);
        YoriLibFree(Handles);
        return FALSE;
    }

    for (Index = 0; Index < Handles->NumberOfHandles; Index++) {
        ThisHandle = &Handles->Handles[Index];
        if (ProcessHandle == INVALID_HANDLE_VALUE ||
            LastPid != ThisHandle->ProcessId) {

            if (ProcessHandle != INVALID_HANDLE_VALUE &&
                ProcessHandle != NULL) {

                CloseHandle(ProcessHandle);
            }

            //
            //  This open may fail.  If it does, we can't get information
            //  about this process, which makes displaying numeric values
            //  pointless.  Leaving this value as NULL is used to suppress
            //  processing this process, but when we get to the next
            //  process (LastPid != ThisHandle->ProcessId), the next
            //  process will be opened.
            //

            ProcessHandle = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, (DWORD)ThisHandle->ProcessId);
            if (ProcessHandle == NULL) {
                ProcessHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, (DWORD)ThisHandle->ProcessId);
            }
            LastPid = (DWORD)ThisHandle->ProcessId;

            ModuleNameString.LengthInChars = 0;
            if (DllPsapi.pGetModuleFileNameExW != NULL &&
                ProcessHandle != NULL) {

                ModuleNameString.LengthInChars = DllPsapi.pGetModuleFileNameExW(ProcessHandle, NULL, ModuleNameString.StartOfString, ModuleNameString.LengthAllocated);
            }

            if (ProcessHandle == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Process %i ** NO ACCESS **\n"), LastPid);
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Process %i %y\n"), LastPid, &ModuleNameString);
            }
        }

        if (ProcessHandle != NULL) {

            ObjectName->Name.LengthInBytes = 0;
            ObjectType->TypeName.LengthInBytes = 0;

            //
            //  Get a local instance of the handle and see what information
            //  can be extracted from them
            //

            LocalProcessHandle = INVALID_HANDLE_VALUE;
            if (DuplicateHandle(ProcessHandle, (HANDLE)ThisHandle->HandleValue, GetCurrentProcess(), &LocalProcessHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
                DllNtDll.pNtQueryObject(LocalProcessHandle, 1, ObjectName, ObjectNameLength, &LengthReturned);
                DllNtDll.pNtQueryObject(LocalProcessHandle, 2, ObjectType, ObjectTypeLength, &LengthReturned);
            } else {
                LocalProcessHandle = INVALID_HANDLE_VALUE;
            }

            //
            //  Convert any output into strings for display
            //

            YoriLibInitEmptyString(&ObjectNameString);
            if (ObjectName->Name.LengthInBytes > 0) {
                ObjectNameString.LengthInChars = ObjectName->Name.LengthInBytes / sizeof(WCHAR);
                ObjectNameString.StartOfString = ObjectName->Name.Buffer;
            }
            YoriLibInitEmptyString(&ObjectTypeString);
            if (ObjectType->TypeName.LengthInBytes > 0) {
                ObjectTypeString.LengthInChars = ObjectType->TypeName.LengthInBytes / sizeof(WCHAR);
                ObjectTypeString.StartOfString = ObjectType->TypeName.Buffer;
            }

            //
            //  Only display files, since that's part of the point of the
            //  program.
            //

            if (YoriLibCompareStringWithLiteralInsensitive(&ObjectTypeString, _T("File")) == 0) {

                PYORI_STRING NameToDisplay;

                NameToDisplay = &ObjectNameString;

                //
                //  If it's possible to get a Win32 path name, display that.
                //  Otherwise, use what we have.
                //

                ModuleNameString.LengthInChars = 0;
                if (DllKernel32.pGetFinalPathNameByHandleW != NULL) {

                    ModuleNameString.LengthInChars = DllKernel32.pGetFinalPathNameByHandleW(LocalProcessHandle, ModuleNameString.StartOfString, ModuleNameString.LengthAllocated, 0);

                    if (ModuleNameString.LengthInChars > 0 &&
                        ModuleNameString.LengthInChars < ModuleNameString.LengthAllocated) {
                        NameToDisplay = &ModuleNameString;
                    }
                }


                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Handle %lli Object %p  %y\n"), ThisHandle->HandleValue, ThisHandle->Object, NameToDisplay);
            }

            if (LocalProcessHandle != INVALID_HANDLE_VALUE) {
                CloseHandle(LocalProcessHandle);
            }
        }
    }

    YoriLibFreeStringContents(&ModuleNameString);
    YoriLibFree(ObjectType);
    YoriLibFree(ObjectName);
    YoriLibFree(Handles);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the lsof builtin command.
 */
#define ENTRYPOINT YoriCmd_YLSOF
#else
/**
 The main entrypoint for the lsof standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the lsof cmdlet.

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
    DWORD StartArg = 0;
    DWORD MatchFlags;
    BOOL Recursive = FALSE;
    BOOL BasicEnumeration = FALSE;
    LSOF_CONTEXT LsofContext;
    YORI_STRING Arg;

    ZeroMemory(&LsofContext, sizeof(LsofContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                LsofHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i + 1;
                ArgumentUnderstood = TRUE;
                break;
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

    if (StartArg == 0 || StartArg == ArgC) {
        if (!LsofDumpHandles()) {
            return EXIT_FAILURE;
        }
    } else {

        if (DllNtDll.pNtQueryInformationFile == NULL ||
            DllKernel32.pQueryFullProcessImageNameW == NULL) {

            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("lsof: OS support not present\n"));
            return EXIT_FAILURE;
        }

        //
        //  Attempt to enable backup privilege so an administrator can access
        //  more objects successfully.
        //

        YoriLibEnableBackupPrivilege();

        LsofContext.BufferLength = 16 * 1024;
        LsofContext.Buffer = YoriLibMalloc(LsofContext.BufferLength);
        if (LsofContext.Buffer == NULL) {
            return EXIT_FAILURE;
        }

        //
        //  If no file name is specified, use stdin; otherwise open
        //  the file and use that
        //

        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES;
        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }

        for (i = StartArg; i < ArgC; i++) {

            LsofContext.FilesFoundThisArg = 0;
            YoriLibForEachStream(&ArgV[i], MatchFlags, 0, LsofFileFoundCallback, NULL, &LsofContext);
            if (LsofContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    LsofFileFoundCallback(&FullPath, NULL, 0, &LsofContext);
                    YoriLibFreeStringContents(&FullPath);
                }
            }
        }

        YoriLibFree(LsofContext.Buffer);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
