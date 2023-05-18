/**
 * @file lib/yoricmpt.h
 *
 * Yori shell header file to define OS things that the compilation environment
 * doesn't support.
 *
 * Copyright (c) 2017-2021 Malcolm J. Smith
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

#ifndef FILE_SHARE_DELETE
/**
 Definition for share delete for compilers that don't contain it.
 */
#define FILE_SHARE_DELETE 4
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
/**
 If the console supports it, use the VT100 processing it provides.
 */
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#ifndef ENABLE_QUICK_EDIT_MODE
/**
 Mouse selection capability owned by the console.
 */
#define ENABLE_QUICK_EDIT_MODE 0x0040
#endif

#ifndef ENABLE_EXTENDED_FLAGS
/**
 Allow SetConsoleMode to alter QuickEdit behavior.
 */
#define ENABLE_EXTENDED_FLAGS 0x0080
#endif

#ifndef COMMON_LVB_UNDERSCORE
/**
 Define for the console's underline functionality if the compiler doesn't
 know about it.
 */
#define COMMON_LVB_UNDERSCORE      0x8000
#endif

#if defined(__clang__)
/**
 A macro that defines interlocked as requiring the volatile keyword for
 compilers that require it.
 */
#define INTERLOCKED_VOLATILE volatile
#else
/**
 A macro that defines interlocked as not requiring the volatile keyword for
 compilers that do not expect it.
 */
#define INTERLOCKED_VOLATILE
#endif

#ifndef DWORD_PTR
#ifndef _WIN64

/**
 Definition for pointer size integer for compilers that don't contain it.
 */
typedef DWORD DWORD_PTR;

/**
 Definition for a pointer to a pointer size integer for compilers that
 don't contain it.
 */
typedef DWORD_PTR *PDWORD_PTR;

/**
 Definition for pointer size integer for compilers that don't contain it.
 */
typedef ULONG ULONG_PTR;

/**
 Definition for pointer size integer for compilers that don't contain it.
 */
typedef LONG LONG_PTR;

#endif
#endif

#ifndef MAXINT_PTR
#ifndef _WIN64

/**
 Definition for native size type for compilers that don't contain it.
 */
typedef ULONG SIZE_T;
#endif
#endif

#ifndef FIELD_OFFSET
/**
 Macro to find the offset of a member of a structure if the compilation
 environment doesn't already define it.
 */
#define FIELD_OFFSET(T, F) ((DWORD)&(((T*)0)->F))
#endif

/**
 Define the error type for HRESULT.
 */
typedef _Return_type_success_(return >= 0) long HRESULT;

#ifndef MOUSE_WHEELED
/**
 Definition for mouse wheel support for compilers that don't contain it.
 */
#define MOUSE_WHEELED 0x0004
#endif

#ifndef ERROR_ELEVATION_REQUIRED
/**
 Define for the error indicating than an executable needs to be launched with
 ShellExecute so the user can be prompted for elevation.
 */
#define ERROR_ELEVATION_REQUIRED 740
#endif

#ifndef ERROR_OLD_WIN_VERSION
/**
 Define for the error indicating than an executable requires a newer version
 of Windows.
 */
#define ERROR_OLD_WIN_VERSION 1150
#endif

#ifndef PROCESS_QUERY_LIMITED_INFORMATION
/**
 Definition for opening processes with very limited access for compilation
 environments that don't define it.
 */
#define PROCESS_QUERY_LIMITED_INFORMATION  (0x1000)
#endif

#ifndef SE_MANAGE_VOLUME_NAME
/**
 Definition for manage volume privilege for compilation environments that
 don't define it.
 */
#define SE_MANAGE_VOLUME_NAME             _T("SeManageVolumePrivilege")
#endif

#ifndef SE_CREATE_SYMBOLIC_LINK_NAME
/**
 If the compilation environment hasn't defined it, define the privilege
 for creating symbolic links.
 */
#define SE_CREATE_SYMBOLIC_LINK_NAME      _T("SeCreateSymbolicLinkPrivilege")
#endif

#ifndef DIRECTORY_QUERY
/**
 The security flag indicating a request to open an object manager directory
 for enumeration.
 */
#define DIRECTORY_QUERY (0x0001)
#endif

#ifndef STATUS_MORE_ENTRIES
/**
 If not defined by the current compilation environment, the NTSTATUS code
 indicating more entries should be enumerated.
 */
#define STATUS_MORE_ENTRIES ((LONG)0x00000105)
#endif

#ifndef STATUS_NO_MORE_ENTRIES
/**
 If not defined by the current compilation environment, the NTSTATUS code
 indicating no more entries should be enumerated.
 */
#define STATUS_NO_MORE_ENTRIES ((LONG)0x8000001A)
#endif

#ifndef STATUS_NOT_IMPLEMENTED
/**
 If not defined by the current compilation environment, the NTSTATUS code
 indicating a call is not implemented.
 */
#define STATUS_NOT_IMPLEMENTED ((LONG)0xC0000002)
#endif

#ifndef STATUS_INVALID_INFO_CLASS
/**
 If not defined by the current compilation environment, the NTSTATUS code
 indicating an unknown information class.
 */
#define STATUS_INVALID_INFO_CLASS ((LONG)0xC0000003)
#endif

#ifndef STATUS_INFO_LENGTH_MISMATCH
/**
 If not defined by the current compilation environment, the NTSTATUS code
 indicating an incorrect buffer size for an information class.
 */
#define STATUS_INFO_LENGTH_MISMATCH ((LONG)0xC0000004)
#endif

#ifndef STATUS_DELETE_PENDING
/**
 If not defined by the current compilation environment, the NTSTATUS code
 indicating a file will be deleted on final handle close.
 */
#define STATUS_DELETE_PENDING ((LONG)0xC0000056)
#endif

#ifndef STATUS_INSUFFICIENT_RESOURCES
/**
 If not defined by the current compilation environment, the NTSTATUS code
 indicating an out of memory condition.
 */
#define STATUS_INSUFFICIENT_RESOURCES ((LONG)0xC000009A)
#endif

#ifndef STATUS_CANCELLED
/**
 If not defined by the current compilation environment, the NTSTATUS code
 indicating an operation was cancelled.
 */
#define STATUS_CANCELLED ((LONG)0xC0000120)
#endif

#ifndef STATUS_DEBUGGER_INACTIVE
/**
 If not defined by the current compilation environment, the NTSTATUS code
 indicating a debugger is not currently operational.
 */
#define STATUS_DEBUGGER_INACTIVE ((LONG)0xC0000354)
#endif


/**
 Definition of an IO_STATUS_BLOCK for compilation environments that don't
 define it.
 */
typedef struct _IO_STATUS_BLOCK {

    /**
     The result of the operation.
     */
    union {
        LONG Status;
        PVOID Ptr;
    };

    /**
     The information from the operation, typically number of bytes
     transferred.
     */
    DWORD_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

/**
 An NT Unicode string structure so it's available to be embedded in later
 structures.
 */
typedef struct _YORI_UNICODE_STRING {

    /**
     The length of the string buffer, in bytes.
     */
    WORD LengthInBytes;

    /**
     The maximum length of the string buffer, in bytes.
     */
    WORD LengthAllocatedInBytes;

    /**
     Pointer to the string buffer.
     */
    LPWSTR Buffer;

} YORI_UNICODE_STRING, *PYORI_UNICODE_STRING;

/**
 An NT OBJECT_ATTRIBUTES structure.
 */
typedef struct _YORI_OBJECT_ATTRIBUTES {

    /**
     The length of this structure, in bytes.
     */
    DWORD Length;

    /**
     A handle to an object.  If non-NULL, Name is relative to this object;
     if NULL, Name is fully specified.
     */
    HANDLE RootDirectory;

    /**
     The name of the object.
     */
    PYORI_UNICODE_STRING Name;

    /**
     Attributes.
     */
    DWORD Attributes;

    /**
     Security descriptor.
     */
    PVOID SecurityDescriptor;

    /**
     Security QOS.
     */
    PVOID SecurityQOS;

    /**
     NOTE This is not part of the original structure.

     Physical storage for the name.  This happens because the rest of
     Yori wants to think in YORI_STRINGs which are similar but not
     identical, and Name needs to point to a UNICODE_STRING.
     */
    YORI_UNICODE_STRING NameStorage;

} YORI_OBJECT_ATTRIBUTES, *PYORI_OBJECT_ATTRIBUTES;

/**
 Definition of the information class to enumerate process IDs using a file
 for compilation environments that don't define it.
 */
#define FileProcessIdsUsingFileInformation (47)

/**
 A structure that is returned by NtQueryInformationFile describing
 information about processes using a given file.
 */
typedef struct _FILE_PROCESS_IDS_USING_FILE_INFORMATION {

    /**
     Ignored for alignment.
     */
    DWORD NumberOfProcesses;

    /**
     An array of process IDs for users of this file.
     */
    DWORD_PTR ProcessIds[1];

} FILE_PROCESS_IDS_USING_FILE_INFORMATION, *PFILE_PROCESS_IDS_USING_FILE_INFORMATION;

/**
 Definition of the information class to obtain case sensitivity information
 for a directory for compilation environments that don't define it.
 */
#define FileCaseSensitiveInformation (71)

/**
 Information about the case sensitivity state for a directory.
 */
typedef struct _YORI_FILE_CASE_SENSITIVE_INFORMATION {

    /**
     Flags indicating case sensitivity information for a directory.  The only
     known flag is 1, indicating that per-directory case sensitivity is
     enabled for that directory.
     */
    DWORD Flags;

} YORI_FILE_CASE_SENSITIVE_INFORMATION, *PYORI_FILE_CASE_SENSITIVE_INFORMATION;

/**
 Definition of the information class to query memory usage of a process for
 compilation environments that don't define it.
 */
#define ProcessVmCounters (3)

/**
 A structure that is returned by NtQueryInformationProcess describing
 information about a process, including the location of its PEB.
 */
typedef struct _PROCESS_BASIC_INFORMATION {
    /**
     Ignored for alignment.
     */
    PVOID Reserved1;

    /**
     Pointer to the PEB within the target process address space.
     */
    PVOID PebBaseAddress;

    /**
     Ignored for alignment.
     */
    PVOID Reserved2[4];
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;


/**
 A structure that is returned by NtQueryInformationProcess describing virtual
 memory usage.
 */
typedef struct _PROCESS_VM_COUNTERS {

    /**
     The maximum amount of virtual address space used by this process.
     */
    DWORD_PTR PeakVirtualSize;

    /**
     The amount of virtual address space used by this process.
     */
    DWORD_PTR VirtualSize;

    /**
     The number of page faults taken by this process.
     */
    DWORD PageFaultCount;

    /**
     The maximum size of the working set for this process.
     */
    DWORD_PTR PeakWorkingSetSize;

    /**
     The size of the working set for this process.
     */
    DWORD_PTR WorkingSetSize;

    /**
     Kernel memory, unused in this application.
     */
    DWORD_PTR Ignored[4];

    /**
     The amount of bytes the process has committed.
     */
    DWORD_PTR CommitUsage;

    /**
     The maximum amount of bytes the process has committed.
     */
    DWORD_PTR PeakCommitUsage;
} PROCESS_VM_COUNTERS, *PPROCESS_VM_COUNTERS;

/**
 The size of a 32 bit pointer.
 */
typedef DWORD YORI_LIB_PTR32;

/**
 A structure corresponding to process parameters in a 32 bit child process.
 */
typedef struct _YORI_LIB_PROCESS_PARAMETERS32 {
    /**
     Ignored for alignment.
     */
    DWORD Ignored1[3];

    /**
     Console flags.
     */
    DWORD ConsoleFlags;

    /**
     Handle to the console driver.
     */
    HANDLE ConsoleHandle;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored2[4];

    /**
     The number of bytes in the CurrentDirectory.
     */
    WORD CurrentDirectoryLengthInBytes;

    /**
     The number of bytes allocated for the CurrentDirectory.
     */
    WORD CurrentDirectoryMaximumLengthInBytes;

    /**
     Pointer to the CurrentDirectory.
     */
    YORI_LIB_PTR32 CurrentDirectory;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored3[3];

    /**
     The number of bytes in the ImagePathName.
     */
    WORD ImagePathNameLengthInBytes;

    /**
     The number of bytes allocated for the ImagePathName.
     */
    WORD ImagePathNameMaximumLengthInBytes;

    /**
     Pointer to the ImagePathName.
     */
    YORI_LIB_PTR32 ImagePathName;

    /**
     The number of bytes in CommandLine.
     */
    WORD CommandLineLengthInBytes;

    /**
     The number of bytes allocated for CommandLine.
     */
    WORD CommandLineMaximumLengthInBytes;

    /**
     Pointer to the CommandLine.
     */
    YORI_LIB_PTR32 CommandLine;

    /**
     Pointer to the process environment block.
     */
    YORI_LIB_PTR32 EnvironmentBlock;
} YORI_LIB_PROCESS_PARAMETERS32, *PYORI_LIB_PROCESS_PARAMETERS32;

/**
 A structure corresponding to a PEB in a 32 bit child process when viewed from
 a 32 bit debugger process.
 */
typedef struct _YORI_LIB_PEB32_NATIVE {
    /**
     Ignored for alignment.
     */
    DWORD Flags;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored;

    /**
     The address of the executable module.
     */
    YORI_LIB_PTR32 ImageBaseAddress;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored2;

    /**
     Pointer to the process parameters.
     */
    PYORI_LIB_PROCESS_PARAMETERS32 ProcessParameters;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored3[17];

    /**
     Ignored for alignment.
     */
    DWORD Ignored4[19];

    /**
     The major OS version to report to the application.
     */
    DWORD OSMajorVersion;

    /**
     The minor OS version to report to the application.
     */
    DWORD OSMinorVersion;

    /**
     The build number to report to the application.
     */
    WORD OSBuildNumber;

    /**
     The servicing number.
     */
    WORD OSCSDVersion;

} YORI_LIB_PEB32_NATIVE, *PYORI_LIB_PEB32_NATIVE;

/**
 A structure corresponding to a PEB in a 32 bit child process on 64 bit
 versions of Windows when viewed through a 64 bit debugger.
 */
typedef struct _YORI_LIB_PEB32_WOW {
    /**
     Ignored for alignment.
     */
    DWORD Flags;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored;

    /**
     The address of the executable module.
     */
    YORI_LIB_PTR32 ImageBaseAddress;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored2;

    /**
     Pointer to the process parameters.
     */
    PYORI_LIB_PROCESS_PARAMETERS32 ProcessParameters;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored3[17];

    /**
     Ignored for alignment.
     */
    DWORD Ignored4[18];

    /**
     The major OS version to report to the application.
     */
    DWORD OSMajorVersion;

    /**
     The minor OS version to report to the application.
     */
    DWORD OSMinorVersion;

    /**
     The build number to report to the application.
     */
    WORD OSBuildNumber;

    /**
     The servicing number.
     */
    WORD OSCSDVersion;

} YORI_LIB_PEB32_WOW, *PYORI_LIB_PEB32_WOW;

/**
 A minimal definition of a 32 bit TEB, suitable for finding a 32 bit PEB.
 */
typedef struct _YORI_LIB_TEB32 {

    /**
     Unknown and reserved for alignment.
     */
    DWORD Ignored[12];

    /**
     A 32 bit pointer to the 32 bit PEB.
     */
    DWORD Peb32Address;
} YORI_LIB_TEB32, *PYORI_LIB_TEB32;

/**
 Indicates that control registers (eip etc) be captured.
 */
#define YORI_WOW64_CONTEXT_CONTROL (0x00010001)

/**
 Indicates that integer registers (eax, ebx et al) be captured.
 */
#define YORI_WOW64_CONTEXT_INTEGER (0x00010002)

/**
 Saved registers from a 32 bit process running within a 64 bit OS.  Not all
 64 bit compilation environments define this.
 */
typedef struct _YORI_LIB_WOW64_CONTEXT {

    /**
     Flags indicating the set of registers to capture.
     */
    DWORD ContextFlags;

    /**
     CPU debug registers, unused in this application.
     */
    DWORD DebugRegisters[6];

    /**
     State about floating point, unused in this application.
     */
    DWORD FloatRegisters[28];

    /**
     The gs register.
     */
    DWORD SegGs;

    /**
     The fs register.
     */
    DWORD SegFs;

    /**
     The extra segment register.
     */
    DWORD SegEs;

    /**
     The data segment register.
     */
    DWORD SegDs;

    /**
     The edi register.
     */
    DWORD Edi;

    /**
     The esi register.
     */
    DWORD Esi;

    /**
     The ebx register.
     */
    DWORD Ebx;

    /**
     The edx register.
     */
    DWORD Edx;

    /**
     The ecx register.
     */
    DWORD Ecx;

    /**
     The eax register.
     */
    DWORD Eax;


    /**
     The stack base pointer.
     */
    DWORD Ebp;

    /**
     The instruction pointer.
     */
    DWORD Eip;

    /**
     The code segment register.
     */
    DWORD SegCs;

    /**
     Processor flags.
     */
    DWORD EFlags;

    /**
     The stack pointer register.
     */
    DWORD Esp;

    /**
     The stack segment register.
     */
    DWORD SegSs;

    /**
     Extra space used for some unknown OS specific reason.
     */
    DWORD Padding[128];

} YORI_LIB_WOW64_CONTEXT, *PYORI_LIB_WOW64_CONTEXT;

/**
 The size of a 64 bit pointer.
 */
typedef LONGLONG YORI_LIB_PTR64;

/**
 A structure corresponding to process parameters in a 64 bit child process.
 */
typedef struct _YORI_LIB_PROCESS_PARAMETERS64 {
    /**
     Ignored for alignment.
     */
    DWORD Ignored1[3];

    /**
     Console flags.
     */
    DWORD ConsoleFlags;

    /**
     Handle to the console driver.
     */
    HANDLE ConsoleHandle;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR64 Ignored2[4];

    /**
     The number of bytes in the CurrentDirectory.
     */
    WORD CurrentDirectoryLengthInBytes;

    /**
     The number of bytes allocated for the CurrentDirectory.
     */
    WORD CurrentDirectoryMaximumLengthInBytes;

    /**
     Pointer to the CurrentDirectory.
     */
    YORI_LIB_PTR64 CurrentDirectory;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR64 Ignored3[3];

    /**
     The number of bytes in the ImagePathName.
     */
    WORD ImagePathNameLengthInBytes;

    /**
     The number of bytes allocated for the ImagePathName.
     */
    WORD ImagePathNameMaximumLengthInBytes;

    /**
     Padding in 64 bit
     */
    DWORD Ignored4;

    /**
     Pointer to the ImagePathName.
     */
    YORI_LIB_PTR64 ImagePathName;

    /**
     The number of bytes in CommandLine.
     */
    WORD CommandLineLengthInBytes;

    /**
     The number of bytes allocated for CommandLine.
     */
    WORD CommandLineMaximumLengthInBytes;

    /**
     Padding in 64 bit
     */
    DWORD Ignored5;

    /**
     Pointer to the CommandLine.
     */
    YORI_LIB_PTR64 CommandLine;

    /**
     Pointer to the process environment block.
     */
    YORI_LIB_PTR64 EnvironmentBlock;
} YORI_LIB_PROCESS_PARAMETERS64, *PYORI_LIB_PROCESS_PARAMETERS64;

/**
 A structure corresponding to a PEB in a 64 bit child process.
 */
typedef struct _YORI_LIB_PEB64 {
    /**
     Ignored for alignment.
     */
    DWORD Flags[2];

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR64 Ignored;

    /**
     The address of the executable module.
     */
    YORI_LIB_PTR64 ImageBaseAddress;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR64 Ignored2;

    /**
     Pointer to the process parameters.
     */
    PYORI_LIB_PROCESS_PARAMETERS64 ProcessParameters;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR64 Ignored3[17];

    /**
     Ignored for alignment.
     */
    DWORD Ignored4[26];

    /**
     The major OS version to report to the application.
     */
    DWORD OSMajorVersion;

    /**
     The minor OS version to report to the application.
     */
    DWORD OSMinorVersion;

    /**
     The build number to report to the application.
     */
    WORD OSBuildNumber;

    /**
     The servicing number.
     */
    WORD OSCSDVersion;
} YORI_LIB_PEB64, *PYORI_LIB_PEB64;

#if _WIN64
/**
 On 64 bit builds, the current process PEB is 64 bit.
 */
#define PYORI_LIB_PEB_NATIVE PYORI_LIB_PEB64
#else
/**
 On 32 bit builds, the current process PEB is 32 bit.
 */
#define PYORI_LIB_PEB_NATIVE PYORI_LIB_PEB32_NATIVE
#endif

/**
 Definition of the system process information enumeration class for
 NtQuerySystemInformation .
 */
#define SystemProcessInformation (5)

/**
 Definition of the system handle information enumeration class for
 NtQuerySystemInformation .
 */
#define SystemHandleInformation (16)

/**
 Definition of the system extended handle information enumeration class for
 NtQuerySystemInformation .
 */
#define SystemExtendedHandleInformation (64)


/**
 Information returned about every process in the system.
 */
typedef struct _YORI_SYSTEM_PROCESS_INFORMATION {

    /**
     Offset from the beginning of this structure to the next entry, in bytes.
     */
    ULONG NextEntryOffset;

    /**
     The number of threads in the process.
     */
    ULONG NumberOfThreads;

    /**
     Ignored in this application.
     */
    BYTE Reserved1[24];

    /**
     The system time when the process was launched.
     */
    LARGE_INTEGER CreateTime;

    /**
     The amount of time the process has spent executing in user mode.
     */
    LARGE_INTEGER UserTime;

    /**
     The amount of time the process has spent executing in kernel mode.
     */
    LARGE_INTEGER KernelTime;

    /**
     The number of bytes in the image name.
     */
    USHORT ImageNameLengthInBytes;

    /**
     The number of bytes allocated for the image name.
     */
    USHORT ImageNameMaximumLengthInBytes;

    /**
     Pointer to the image name.
     */
    LPWSTR ImageName;

    /**
     Ignored in this application.
     */
    ULONG Reserved2;

    /**
     The process identifier.
     */
    DWORD_PTR ProcessId;

    /**
     The parent process identifier.
     */
    DWORD_PTR ParentProcessId;

    /**
     Ignored in this application.
     */
    PVOID Reserved3[4];

    /**
     Ignored in this application.
     */
    ULONG Reserved4[3];

    /**
     The number of bytes in the working set of the process.
     */
    SIZE_T WorkingSetSize;

    /**
     Ignored in this application.
     */
    PVOID Reserved5[4];

    /**
     The number of bytes committed by the process.
     */
    SIZE_T CommitSize;

    /**
     Ignored in this application.
     */
    PVOID Reserved6[2];

    /**
     Ignored in this application.
     */
    LARGE_INTEGER Reserved7[6];

} YORI_SYSTEM_PROCESS_INFORMATION, *PYORI_SYSTEM_PROCESS_INFORMATION;

/**
 Information returned about every thread in the process.
 */
typedef struct _YORI_SYSTEM_THREAD_INFORMATION {

    /**
     Ignored in this application.
     */
    LARGE_INTEGER Reserved[3];

    /**
     Ignored in this application.
     */
    ULONG Reserved2;

    /**
     Ignored in this application.
     */
    PVOID Reserved3;

    /**
     The process identifier.  This should match the process that contains this
     thread entry.
     */
    HANDLE ProcessId;

    /**
     The thread identifier.
     */
    HANDLE ThreadId;

    /**
     Ignored in this application.
     */
    ULONG Reserved4[5];
} YORI_SYSTEM_THREAD_INFORMATION, *PYORI_SYSTEM_THREAD_INFORMATION;

/**
 Information about each opened handle in the system.
 */
typedef struct YORI_SYSTEM_HANDLE_ENTRY {

    /**
     The process that has the handle opened.
     */
    WORD ProcessId;

    /**
     An index indicating the stack that created the handle.
     */
    WORD CreatorStack;

    /**
     The object type for the handle.
     */
    UCHAR ObjectType;

    /**
     Attributes for the handle.
     */
    UCHAR HandleAttributes;

    /**
     The handle identifier.
     */
    WORD HandleValue;

    /**
     Pointer to the object.
     */
    PVOID Object;

    /**
     The access that the handle was opened with.
     */
    DWORD GrantedAccess;
} YORI_SYSTEM_HANDLE_ENTRY, *PYORI_SYSTEM_HANDLE_ENTRY;

/**
 Information about handles currently opened in the system.
 */
typedef struct _YORI_SYSTEM_HANDLE_INFORMATION {

    /**
     The number of handles in the array.
     */
    DWORD NumberOfHandles;

    /**
     Array of information about handles.
     */
    YORI_SYSTEM_HANDLE_ENTRY Handles[1];
} YORI_SYSTEM_HANDLE_INFORMATION, *PYORI_SYSTEM_HANDLE_INFORMATION;

/**
 Information about each opened handle in the system.
 */
typedef struct YORI_SYSTEM_HANDLE_ENTRY_EX {

    /**
     Pointer to the object.
     */
    PVOID Object;

    /**
     The process that has the handle opened.
     */
    DWORD_PTR ProcessId;

    /**
     The handle identifier.
     */
    DWORD_PTR HandleValue;

    /**
     The access that the handle was opened with.
     */
    DWORD GrantedAccess;

    /**
     An index indicating the stack that created the handle.
     */
    WORD CreatorStack;

    /**
     The object type for the handle.
     */
    WORD ObjectType;

    /**
     Attributes for the handle.
     */
    DWORD HandleAttributes;

    /**
     Reserved for future use.
     */
    DWORD Reserved;
} YORI_SYSTEM_HANDLE_ENTRY_EX, *PYORI_SYSTEM_HANDLE_ENTRY_EX;

/**
 Information about handles currently opened in the system.
 */
typedef struct _YORI_SYSTEM_HANDLE_INFORMATION_EX {

    /**
     The number of handles in the array.
     */
    DWORD_PTR NumberOfHandles;

    /**
     Reserved for future use.
     */
    DWORD_PTR Reserved;

    /**
     Array of information about handles.
     */
    YORI_SYSTEM_HANDLE_ENTRY_EX Handles[1];
} YORI_SYSTEM_HANDLE_INFORMATION_EX, *PYORI_SYSTEM_HANDLE_INFORMATION_EX;

/**
 A structure describing an object name.  The string itself is returned
 following this structure.
 */
typedef struct _YORI_OBJECT_NAME_INFORMATION {

    /**
     The name of the object.
     */
    YORI_UNICODE_STRING Name;
} YORI_OBJECT_NAME_INFORMATION, *PYORI_OBJECT_NAME_INFORMATION;

/**
 A structure describing an object type.  The string itself is returned
 following this structure.
 */
typedef struct _YORI_OBJECT_TYPE_INFORMATION {

    /**
     The type name in a UNICODE_STRING.
     */
    YORI_UNICODE_STRING TypeName;

    /**
     Documented as reserved and not used in this program.
     */
    DWORD Reserved[22];
} YORI_OBJECT_TYPE_INFORMATION, *PYORI_OBJECT_TYPE_INFORMATION;

/**
 A structure describing how to take a live dump.
 */
typedef struct _YORI_SYSDBG_LIVEDUMP_CONTROL {

    /**
     The version of this structure.
     */
    DWORD Version;

    /**
     The bugcheck code to include in the dump.
     */
    DWORD BugcheckCode;

    /**
     The bugcheck parameters to include in the dump.
     */
    DWORD_PTR BugcheckParameters[4];

    /**
     A file handle opened for write to store the dump.
     */
    HANDLE File;

    /**
     Optionally an event handle to indicate that dump capture should be
     cancelled.
     */
    HANDLE CancelEvent;

    /**
     Flags indicating how to write the dump and what to include.
     */
    DWORD Flags;

    /**
     Flags indicating what to include in the dump.
     */
    DWORD AddPagesFlags;

} YORI_SYSDBG_LIVEDUMP_CONTROL, *PYORI_SYSDBG_LIVEDUMP_CONTROL;

/**
 Include usermode pages in addition to kernel pages.
 */
#define SYSDBG_LIVEDUMP_FLAG_USER_PAGES             0x00000004

/**
 Include hypervisor pages in addition to kernel pages.
 */
#define SYSDBG_LIVEDUMP_ADD_PAGES_FLAG_HYPERVISOR   0x00000001


/**
 A structure describing how to take a triage dump.
 */
typedef struct _YORI_SYSDBG_TRIAGE_DUMP_CONTROL {

    /**
     Flags.
     */
    DWORD Flags;

    /**
     The bugcheck code to include in the dump.
     */
    DWORD BugcheckCode;

    /**
     The bugcheck parameters to include in the dump.
     */
    DWORD_PTR BugcheckParameters[4];

    /**
     The number of process handles.  Must be zero.
     */
    DWORD ProcessHandleCount;

    /**
     The number of thread handles.
     */
    DWORD ThreadHandleCount;

    /**
     Pointer to an array of handles.
     */
    PHANDLE HandleArray;

} YORI_SYSDBG_TRIAGE_DUMP_CONTROL, *PYORI_SYSDBG_TRIAGE_DUMP_CONTROL;

/**
 If not defined by the compilation environment, the product identifier for
 an unknown product.
 */
#define PRODUCT_UNDEFINED                           0x00000000

/**
 If not defined by the compilation environment, the product identifier for
 the datacenter server core product.
 */
#define PRODUCT_DATACENTER_SERVER_CORE              0x0000000C

/**
 If not defined by the compilation environment, the product identifier for
 the standard server core product.
 */
#define PRODUCT_STANDARD_SERVER_CORE                0x0000000D

/**
 If not defined by the compilation environment, the product identifier for
 the enterprise server core product.
 */
#define PRODUCT_ENTERPRISE_SERVER_CORE              0x0000000E

/**
 If not defined by the compilation environment, the product identifier for
 the web server core product.
 */
#define PRODUCT_WEB_SERVER_CORE                     0x0000001D

/**
 If not defined by the compilation environment, the product identifier for
 the datacenter server core product without hyper-v.
 */
#define PRODUCT_DATACENTER_SERVER_CORE_V            0x00000027

/**
 If not defined by the compilation environment, the product identifier for
 the standard server core product without hyper-v.
 */
#define PRODUCT_STANDARD_SERVER_CORE_V              0x00000028

/**
 If not defined by the compilation environment, the product identifier for
 the enterprise server core product without hyper-v.
 */
#define PRODUCT_ENTERPRISE_SERVER_CORE_V            0x00000029

/**
 If not defined by the compilation environment, the product identifier for
 the hyper-v server product.
 */
#define PRODUCT_HYPERV                              0x0000002A

/**
 If not defined by the compilation environment, the product identifier for
 the express storage server core product.
 */
#define PRODUCT_STORAGE_EXPRESS_SERVER_CORE         0x0000002B

/**
 If not defined by the compilation environment, the product identifier for
 the standard storage server core product.
 */
#define PRODUCT_STORAGE_STANDARD_SERVER_CORE        0x0000002C

/**
 If not defined by the compilation environment, the product identifier for
 the workgroup storage server core product.
 */
#define PRODUCT_STORAGE_WORKGROUP_SERVER_CORE       0x0000002D

/**
 If not defined by the compilation environment, the product identifier for
 the enterprise storage server core product.
 */
#define PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE      0x0000002E

/**
 If not defined by the compilation environment, the product identifier for
 the standard solutions server core product.
 */
#define PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE      0x00000035

/**
 If not defined by the compilation environment, the product identifier for
 the embedded solutions server core product.
 */
#define PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE        0x00000039

/**
 If not defined by the compilation environment, the product identifier for
 the small business server premium server core product.
 */
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE   0x0000003F

/**
 If not defined by the compilation environment, the product identifier for
 the datacenter server core product.
 */
#define PRODUCT_DATACENTER_A_SERVER_CORE            0x00000091

/**
 If not defined by the compilation environment, the product identifier for
 the standard server core product.
 */
#define PRODUCT_STANDARD_A_SERVER_CORE              0x00000092

/**
 If not defined by the compilation environment, the product identifier for
 the datacenter server core product.
 */
#define PRODUCT_DATACENTER_WS_SERVER_CORE           0x00000093

/**
 If not defined by the compilation environment, the product identifier for
 the standard server core product.
 */
#define PRODUCT_STANDARD_WS_SERVER_CORE             0x00000094

/**
 If not defined by the compilation environment, the product identifier for
 the datacenter server core product.
 */
#define PRODUCT_DATACENTER_EVALUATION_SERVER_CORE   0x0000009F

/**
 If not defined by the compilation environment, the product identifier for
 the standard server core product.
 */
#define PRODUCT_STANDARD_EVALUATION_SERVER_CORE     0x000000A0

/**
 If not defined by the compilation environment, the product identifier for
 the azure server core product.
 */
#define PRODUCT_AZURE_SERVER_CORE                   0x000000A8

/**
 If not defined by the compilation environment, the product identifier for
 an unlicensed product.
 */
#define PRODUCT_UNLICENSED                          0xABCDABCD

/**
 An implementation of the OSVERSIONINFO structure.
 */
typedef struct _YORI_OS_VERSION_INFO {

    /**
     The size of the structure in bytes.
     */
    DWORD dwOsVersionInfoSize;

    /**
     On successful completion, the major version of the operating system.
     */
    DWORD dwMajorVersion;

    /**
     On successful completion, the minor version of the operating system.
     */
    DWORD dwMinorVersion;

    /**
     On successful completion, the build number of the operating system.
     */
    DWORD dwBuildNumber;

    /**
     On successful completion, the type of the operating system.
     */
    DWORD dwPlatformId;

    /**
     On successful completion, the servicing state of the operating system.
     */
    TCHAR szCSDVersion[128];
} YORI_OS_VERSION_INFO, *PYORI_OS_VERSION_INFO;

/**
 An implementation of the OSVERSIONINFOEX structure.
 */
typedef struct _YORI_OS_VERSION_INFO_EX {

    /**
     The base form of this structure.
     */
    YORI_OS_VERSION_INFO Core;

    /**
     On successful completion, the service pack major version.
     */
    WORD wServicePackMajor;

    /**
     On successful completion, the service pack minor version.
     */
    WORD wServicePackMinor;

    /**
     On successful completion, a set of capability flags indicating the
     capabilities of the edition.
     */
    WORD wSuiteMask;

    /**
     On successful completion, indicates the broad class of product.
     */
    BYTE wProductType;

    /**
     Reserved for future use.
     */
    BYTE wReserved;
} YORI_OS_VERSION_INFO_EX, *PYORI_OS_VERSION_INFO_EX;

#ifndef VER_SUITE_SMALLBUSINESS
/**
 Definition of the suite mask flag for small business server for compilation
 environments which don't provide it.
 */
#define VER_SUITE_SMALLBUSINESS 0x0001
#endif

#ifndef VER_SUITE_ENTERPRISE
/**
 Definition of the suite mask flag for enterprise edition for compilation
 environments which don't provide it.
 */
#define VER_SUITE_ENTERPRISE 0x0002
#endif

#ifndef VER_SUITE_BACKOFFICE
/**
 Definition of the suite mask flag for backoffice server for compilation
 environments which don't provide it.
 */
#define VER_SUITE_BACKOFFICE 0x0004
#endif

#ifndef VER_SUITE_TERMINAL
/**
 Definition of the suite mask flag for terminal server for compilation
 environments which don't provide it.
 */
#define VER_SUITE_TERMINAL 0x0010
#endif

#ifndef VER_SUITE_SMALLBUSINESS_RESTRICTED
/**
 Definition of the suite mask flag for small business server for compilation
 environments which don't provide it.
 */
#define VER_SUITE_SMALLBUSINESS_RESTRICTED 0x0020
#endif

#ifndef VER_SUITE_EMBEDDEDNT
/**
 Definition of the suite mask flag for embedded for compilation environments
 which don't provide it.
 */
#define VER_SUITE_EMBEDDEDNT 0x0040
#endif

#ifndef VER_SUITE_DATACENTER
/**
 Definition of the suite mask flag for datacenter server for compilation
 environments which don't provide it.
 */
#define VER_SUITE_DATACENTER 0x0080
#endif

#ifndef VER_SUITE_PERSONAL
/**
 Definition of the suite mask flag for home edition for compilation
 environments which don't provide it.
 */
#define VER_SUITE_PERSONAL 0x0200
#endif

#ifndef VER_SUITE_BLADE
/**
 Definition of the suite mask flag for web server for compilation environments
 which don't provide it.
 */
#define VER_SUITE_BLADE 0x0400
#endif

#ifndef VER_NT_WORKSTATION
/**
 Definition of the product type flag for a workstation for compilation
 environments which don't provide it.
 */
#define VER_NT_WORKSTATION 0x0001
#endif

#ifndef VER_NT_DOMAIN_CONTROLLER
/**
 Definition of the product type flag for a domain controller for compilation
 environments which don't provide it.
 */
#define VER_NT_DOMAIN_CONTROLLER 0x0002
#endif

#ifndef VER_NT_SERVER
/**
 Definition of the product type flag for a server for compilation environments
 which don't provide it.
 */
#define VER_NT_SERVER 0x0003
#endif

/**
 Output from the GetSystemInfo system call.  This is defined here so that it
 can contain newer fields than older compilers include, which may be returned
 from an OS regardless of how old the compiler is.
 */
typedef struct _YORI_SYSTEM_INFO {
    union {
        /**
         Historic representation of a system architecture, used in NT 3.x
         */
        DWORD dwOemId;
        struct {

            /**
             Current representation of a system architecture, used in NT4+
             */
            WORD wProcessorArchitecture;

            /**
             "Unused" except as above
             */
            WORD wReserved;
        };
    };

    /**
     The size of a memory page, in bytes
     */
    DWORD dwPageSize;

    /**
     The base address of usermode memory
     */
    LPVOID lpMinimumApplicationAddress;

    /**
     The upper address of usermode memory
     */
    LPVOID lpMaximumApplicationAddress;

    /**
     A mask of CPUs that are currently in use
     */
    DWORD_PTR dwActiveProcessorMask;

    /**
     The number of CPUs that are currently in use
     */
    DWORD dwNumberOfProcessors;

    /**
     The type of CPUs that are currently in use.  Note this refers to the
     specific type of processor, not the process architecture, which is
     returned above
     */
    DWORD dwProcessorType;

    /**
     The minimum number of bytes that can be allocated from the system heap
     */
    DWORD dwAllocationGranularity;

    /**
     Information about the specific model of processor
     */
    WORD wProcessorLevel;

    /**
     Information about the specific model of processor
     */
    WORD wProcessorRevision;
} YORI_SYSTEM_INFO, *PYORI_SYSTEM_INFO;

/**
 Information about memory usage for the system and the process.
 */
typedef struct _YORI_MEMORYSTATUSEX {

    /**
     The length of the structure, in bytes.
     */
    DWORD dwLength;

    /**
     The percentage of memory used.
     */
    DWORD dwMemoryLoad;

    /**
     The amount of physical memory, in bytes.
     */
    DWORDLONG ullTotalPhys;

    /**
     The amount of available physical memory, in bytes.
     */
    DWORDLONG ullAvailPhys;

    /**
     The amount of physical memory plus page file size, in bytes.
     */
    DWORDLONG ullTotalPageFile;

    /**
     The amount of available physical memory plus page file size, in bytes.
     */
    DWORDLONG ullAvailPageFile;

    /**
     The amount of virtual address space in the process, in bytes.
     */
    DWORDLONG ullTotalVirtual;

    /**
     The amount of available virtual address space in the process, in bytes.
     */
    DWORDLONG ullAvailVirtual;

    /**
     The amount of virtual address space that is addressable but not available
     to the process, in bytes.
     */
    DWORDLONG ullAvailExtendedVirtual;
} YORI_MEMORYSTATUSEX, *PYORI_MEMORYSTATUSEX;

/**
 Information about the IO requests generated by the process.
 */
typedef struct _YORI_IO_COUNTERS {

    /**
     The number of read requests issued by the process.
     */
    DWORDLONG ReadOperations;

    /**
     The number of write requests issued by the process.
     */
    DWORDLONG WriteOperations;

    /**
     The number of other IO requests issued by the process.
     */
    DWORDLONG OtherOperations;

    /**
     The number of bytes read by the process.
     */
    DWORDLONG ReadBytes;

    /**
     The number of bytes written by the process.
     */
    DWORDLONG WriteBytes;

    /**
     The number of bytes transferred by other IO requests from the process.
     */
    DWORDLONG OtherBytes;

} YORI_IO_COUNTERS, *PYORI_IO_COUNTERS;

/**
 A set of processor property relationships that are known to
 GetLogicalProcessorInformationEx.
 */
typedef enum _YORI_LOGICAL_PROCESSOR_RELATIONSHIP {
    YoriProcessorRelationProcessorCore = 0,
    YoriProcessorRelationNumaNode = 1,
    YoriProcessorRelationCache = 2,
    YoriProcessorRelationProcessorPackage = 3,
    YoriProcessorRelationGroup = 4,
    YoriProcessorRelationAll = 0xFFFF
} YORI_LOGICAL_PROCESSOR_RELATIONSHIP;

/**
 Different types of processor caches.
 */
typedef enum _YORI_PROCESSOR_CACHE_TYPE {
    YoriProcessorCacheUnified = 0,
    YoriProcessorCacheInstruction = 1,
    YoriProcessorCacheData = 2,
    YoriProcessorCacheTrace = 3
} YORI_PROCESSOR_CACHE_TYPE;

/**
 Information about a single level and type of processor cache.
 */
typedef struct _YORI_PROCESSOR_CACHE_DESCRIPTOR {

    /**
     The level of the processor cache (L1, L2, L3 etc.)
     */
    UCHAR Level;

    /**
     Cache associativity.  0xff indicates a fully associative cache and no
     other value is explicitly documented.
     */
    UCHAR Associativity;

    /**
     The number of bytes in each cache line.
     */
    WORD  LineSize;

    /**
     The number of bytes in the cache.
     */
    DWORD SizeInBytes;

    /**
     The type of information that is stored in the cache.
     */
    YORI_PROCESSOR_CACHE_TYPE Type;
} YORI_PROCESSOR_CACHE_DESCRIPTOR, *PYORI_PROCESSOR_CACHE_DESCRIPTOR;

/**
 Information returned from GetLogicalProcessorInformation describing processor
 relationships.  This information is available on 2003+.
 */
typedef struct _YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION {

    /**
     The mask of logical processors which this information pertains to.
     */
    DWORD_PTR ProcessorMask;

    /**
     The type of relationship described by this structure.
     */
    YORI_LOGICAL_PROCESSOR_RELATIONSHIP Relationship;

    /**
     Information specific to the type of the relationship.
     */
    union {

        /**
         If the below flag contains 1, it indicates that the set of processors
         share hardware resources.
         */
        struct {
            UCHAR Flags;
        } ProcessorCore;

        /**
         The NUMA node that these logical processors are associated with.
         */
        struct {
            DWORD NodeNumber;
        } NumaNode;

        /**
         Information about a processor cache.
         */
        YORI_PROCESSOR_CACHE_DESCRIPTOR Cache;

        /**
         Reserved space for future use.
         */
        DWORDLONG Reserved[2];
    } u;
} YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION, *PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION;

/**
 Information about logical processors within a processor group.
 */
typedef struct _YORI_PROCESSOR_GROUP_AFFINITY {

    /**
     The set of logical processors active within this processor group.
     */
    DWORD_PTR Mask;

    /**
     The number of this processor group.
     */
    WORD      Group;

    /**
     Reserved space for future use.
     */
    WORD      Reserved[3];
} YORI_PROCESSOR_GROUP_AFFINITY, *PYORI_PROCESSOR_GROUP_AFFINITY;

/**
 Information about logical processors within a processor core.
 */
typedef struct _YORI_PROCESSOR_RELATIONSHIP {

    /**
     If the below flag contains 1, it indicates that the device contains more
     than one logical processor, otherwise it contains exactly one logical
     processor.
     */
    UCHAR Flags;

    /**
     Indicates the performance and power draw of the device.  The higher the
     value, the higher performance and power consumption.
     */
    UCHAR EfficiencyClass;

    /**
     Reserved space for future use.
     */
    UCHAR Reserved[20];

    /**
     The number of processor groups in the array below.
     */
    WORD  GroupCount;

    /**
     An array of processor groups each of which describe logical processors
     that are part of this relationship.
     */
    YORI_PROCESSOR_GROUP_AFFINITY GroupMask[ANYSIZE_ARRAY];
} YORI_PROCESSOR_RELATIONSHIP, *PYORI_PROCESSOR_RELATIONSHIP;

/**
 Information about logical processors within a NUMA node.
 */
typedef struct _YORI_NUMA_NODE_RELATIONSHIP {

    /**
     The number of this NUMA node.
     */
    DWORD NodeNumber;

    /**
     Reserved space for future use.
     */
    UCHAR Reserved[20];

    /**
     Logical processors which are part of this NUMA node, including their
     processor group.
     */
    YORI_PROCESSOR_GROUP_AFFINITY GroupMask;
} YORI_NUMA_NODE_RELATIONSHIP, *PYORI_NUMA_NODE_RELATIONSHIP;

/**
 Information about a processor cache.
 */
typedef struct _YORI_PROCESSOR_CACHE_RELATIONSHIP {

    /**
     Information about the cache.
     */
    YORI_PROCESSOR_CACHE_DESCRIPTOR Cache;

    /**
     Reserved space for future use.
     */
    UCHAR Reserved[20];

    /**
     Logical processors which use this processor cache.
     */
    YORI_PROCESSOR_GROUP_AFFINITY GroupMask;
} YORI_PROCESSOR_CACHE_RELATIONSHIP, *PYORI_PROCESSOR_CACHE_RELATIONSHIP;

/**
 Information about logical processors within a processor group.
 */
typedef struct _YORI_PROCESSOR_GROUP_INFORMATION {

    /**
     The maximum number of logical processors within this processor group.
     */
    UCHAR MaximumProcessorCount;

    /**
     The number of logical processors within this processor group that are
     currently active.
     */
    UCHAR ActiveProcessorCount;

    /**
     Reserved space for future use.
     */
    UCHAR Reserved[38];

    /**
     A bitmap of the processors that are currently active within this
     processor group.
     */
    DWORD_PTR ActiveProcessorMask;
} YORI_PROCESSOR_GROUP_INFORMATION, *PYORI_PROCESSOR_GROUP_INFORMATION;

/**
 Information about the set of processor groups in the system.
 */
typedef struct _YORI_PROCESSOR_GROUP_RELATIONSHIP {

    /**
     The maximum number of processor groups.
     */
    WORD  MaximumGroupCount;

    /**
     The number of currently active processor groups.
     */
    WORD  ActiveGroupCount;

    /**
     Reserved space for future use.
     */
    UCHAR Reserved[20];

    /**
     An array of information about each processor group.  Documentation is
     unclear about whether this has MaximumGroupCount of ActiveGroupCount
     members although the situation where these values are not identical is
     extremely rare.
     */
    YORI_PROCESSOR_GROUP_INFORMATION GroupInfo[ANYSIZE_ARRAY];

} YORI_PROCESSOR_GROUP_RELATIONSHIP, *PYORI_PROCESSOR_GROUP_RELATIONSHIP;

/**
 Information returned from GetLogicalProcessorInformationEx describing
 processor relationships.  This information is available on Win7+.
 */
typedef struct _YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX {

    /**
     The type of relationship described by this structure.
     */
    YORI_LOGICAL_PROCESSOR_RELATIONSHIP Relationship;

    /**
     The size of this element in bytes.  This allows the structure to be
     extended in future.
     */
    DWORD SizeInBytes;

    /**
     Information specific to the type of the relationship.
     */
    union {
        /**
         Information describing the relationship between a processor core and
         its logical processors.
         */
        YORI_PROCESSOR_RELATIONSHIP Processor;

        /**
         Information describing the relationship between a NUMA node and its
         logical processors.
         */
        YORI_NUMA_NODE_RELATIONSHIP NumaNode;

        /**
         Information describing the relationship between a processor cache and
         its logical processors.
         */
        YORI_PROCESSOR_CACHE_RELATIONSHIP Cache;

        /**
         Information describing the relationship between a processor group and
         its logical processors.
         */
        YORI_PROCESSOR_GROUP_RELATIONSHIP Group;
    } u;
} YORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, *PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;

#ifndef REG_QWORD
/**
 A define for REG_QWORD, if not provided by the compilation environment.
 */
#define REG_QWORD (11)
#endif

#ifndef PROCESS_MODE_BACKGROUND_BEGIN
/**
 Define the value for low CPU, disk and memory priority for compilation
 environments that don't define it.
 */
#define PROCESS_MODE_BACKGROUND_BEGIN 0x00100000
#endif

#ifndef IOCTL_DISK_GET_LENGTH_INFO
/**
 The IOCTL code to query a volume or disk length, if it's not already defined.
 */
#define IOCTL_DISK_GET_LENGTH_INFO CTL_CODE(IOCTL_DISK_BASE, 23, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

#ifndef FSCTL_SET_REPARSE_POINT

/**
 The FSCTL code to set a reparse point, if it's not already defined.
 */
#define FSCTL_SET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 The FSCTL code to get a reparse point, if it's not already defined.
 */
#define FSCTL_GET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef IO_REPARSE_TAG_MOUNT_POINT

/**
 The reparse tag indicating a mount point or directory junction.
 */
#define IO_REPARSE_TAG_MOUNT_POINT (0xA0000003)
#endif

#ifndef IO_REPARSE_TAG_SYMLINK
/**
 The reparse tag indicating a symbolic link.
 */
#define IO_REPARSE_TAG_SYMLINK     (0xA000000C)
#endif

#ifndef IO_REPARSE_TAG_APPEXECLINK
/**
 The reparse tag indicating a modern app link.
 */
#define IO_REPARSE_TAG_APPEXECLINK (0x8000001B)
#endif

/**
 A structure recording reparse data on a file.
 */
typedef struct _YORI_REPARSE_DATA_BUFFER {

    /**
     The reparse tag.
     */
    DWORD ReparseTag;

    /**
     The size of this structure in bytes.
     */
    WORD  ReparseDataLength;

    /**
     Reserved field to ensure alignment of later structures.
     */
    WORD  ReservedForAlignment;

    /**
     Reparse tag specific structure information.
     */
    union {
        struct {

            /**
             The offset to the name to substitute on reparse.  This is in
             bytes from the Buffer field.
             */
            WORD RealNameOffsetInBytes;

            /**
             The length of the name to substitute on reparse.  This is in
             bytes.
             */
            WORD RealNameLengthInBytes;

            /**
             The offset to the name to display.  This is in bytes from the
             Buffer field.
             */
            WORD DisplayNameOffsetInBytes;

            /**
             The length of the name to display.  This is in bytes.
             */
            WORD DisplayNameLengthInBytes;

            /**
             Flags, unused in this application.
             */
            DWORD Flags;

            /**
             The buffer containing the substitute name and display name.
             */
            WCHAR Buffer[1];

        } SymLink;

        struct {

            /**
             The offset to the name to substitute on reparse.  This is in
             bytes from the Buffer field.
             */
            WORD RealNameOffsetInBytes;

            /**
             The length of the name to substitute on reparse.  This is in
             bytes.
             */
            WORD RealNameLengthInBytes;

            /**
             The offset to the name to display.  This is in bytes from the
             Buffer field.
             */
            WORD DisplayNameOffsetInBytes;

            /**
             The length of the name to display.  This is in bytes.
             */
            WORD DisplayNameLengthInBytes;

            /**
             The buffer containing the substitute name and display name.
             */
            WCHAR Buffer[1];

        } MountPoint;

        struct {

            /**
             The number of strings in the buffer.
             */
            DWORD StringCount;

            /**
             The buffer containing application name and executable name.
             */
            WCHAR Buffer[1];

        } AppxLink;

        struct {

            /**
             A generic buffer of unknown contents.
             */
            UCHAR Buffer[1];
        } Generic;
    } u;
} YORI_REPARSE_DATA_BUFFER, *PYORI_REPARSE_DATA_BUFFER;

#ifndef FILE_FLAG_OPEN_REPARSE_POINT
/**
 The open flag to open a reparse point rather than any link target.
 */
#define FILE_FLAG_OPEN_REPARSE_POINT 0x200000
#endif

#ifndef FILE_ATTRIBUTE_REPARSE_POINT
/**
 The file attribute indicating presence of a reparse point.
 */
#define FILE_ATTRIBUTE_REPARSE_POINT         (0x00000400)
#endif

#ifndef FILE_ATTRIBUTE_SPARSE_FILE
/**
 Specifies the value for a file that is sparse if the compilation environment
 doesn't provide it.
 */
#define FILE_ATTRIBUTE_SPARSE_FILE           (0x00000200)
#endif

#ifndef FILE_ATTRIBUTE_COMPRESSED
/**
 Specifies the value for a file that has been compressed if the compilation
 environment doesn't provide it.
 */
#define FILE_ATTRIBUTE_COMPRESSED            (0x00000800)
#endif

#ifndef FILE_ATTRIBUTE_OFFLINE
/**
 Specifies the value for a file that is on slow storage if the compilation
 environment doesn't provide it.
 */
#define FILE_ATTRIBUTE_OFFLINE               (0x00001000)
#endif

#ifndef FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
/**
 Specifies the value for a file that should not be indexed by the search
 indexer if the compilation environment doesn't provide it.
 */
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED   (0x00002000)
#endif

#ifndef FILE_ATTRIBUTE_ENCRYPTED
/**
 Specifies the value for a file that has been encrypted if the compilation
 environment doesn't provide it.
 */
#define FILE_ATTRIBUTE_ENCRYPTED             (0x00004000)
#endif

#ifndef FILE_ATTRIBUTE_INTEGRITY_STREAM
/**
 Specifies the value for a file subject to CRC integrity detection if the
 compilation environment doesn't provide it.
 */
#define FILE_ATTRIBUTE_INTEGRITY_STREAM      (0x00008000)
#endif

#ifndef FILE_ATTRIBUTE_NO_SCRUB_DATA
/**
 Specifies the value for a file that should not be read by background
 scrubbing.
 */
#define FILE_ATTRIBUTE_NO_SCRUB_DATA         (0x00020000)
#endif

#ifndef FILE_ATTRIBUTE_PINNED
/**
 Specifies the value for a file that should be always available offline.
 */
#define FILE_ATTRIBUTE_PINNED                (0x00080000)
#endif

#ifndef FILE_ATTRIBUTE_UNPINNED
/**
 Specifies the value for a file that should not be stored locally if possible.
 */
#define FILE_ATTRIBUTE_UNPINNED              (0x00100000)
#endif

#ifndef FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL
/**
 Specifies the value for a file that should not be stored locally if possible.
 */
#define FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL   (0x20000000)
#endif

#ifndef FILE_FLAG_OPEN_NO_RECALL
/**
 Specifies the value for opening a file without recalling from slow storage
 if the compilation environment doesn't provide it.
 */
#define FILE_FLAG_OPEN_NO_RECALL         (0x00100000)
#endif

#ifndef FSCTL_GET_COMPRESSION
/**
 Specifies the FSCTL_GET_RETRIEVAL_POINTERS numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_GET_COMPRESSION           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 Specifies the identifier for a file not subject to NTFS compression.
 */
#define COMPRESSION_FORMAT_NONE         (0x0000)

/**
 Specifies the identifier for a file compressed with NTFS LZNT1 algorithm.
 */
#define COMPRESSION_FORMAT_LZNT1        (0x0002)
#endif

#ifndef FSCTL_GET_NTFS_VOLUME_DATA
/**
 Specifies the FSCTL_GET_NTFS_VOLUME_DATA numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_GET_NTFS_VOLUME_DATA       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 25,  METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 Information returned from FSCTL_GET_NTFS_VOLUME_DATA.
 */
typedef struct {

    /**
     The full 64 bit serial number.
     */
    LARGE_INTEGER VolumeSerialNumber;

    /**
     The number of sectors on the volume.
     */
    LARGE_INTEGER NumberSectors;

    /**
     The number of clusters on the volume.
     */
    LARGE_INTEGER TotalClusters;

    /**
     The number of free clusters on the volume.
     */
    LARGE_INTEGER FreeClusters;

    /**
     The number of reserved clusters on the volume.
     */
    LARGE_INTEGER TotalReserved;

    /**
     The bytes in each logical sector.
     */
    DWORD BytesPerSector;

    /**
     The bytes in each file system allocation unit.
     */
    DWORD BytesPerCluster;

    /**
     The bytes in each MFT file record.
     */
    DWORD BytesPerFileRecordSegment;

    /**
     The clusters in each file record.
     */
    DWORD ClustersPerFileRecordSegment;

    /**
     The amount of space ever used in the MFT for file records.
     */
    LARGE_INTEGER MftValidDataLength;

    /**
     The volume offset of the first extent of the MFT.
     */
    LARGE_INTEGER MftStartLcn;

    /**
     The volume offset of the first extent of the MFT backup.
     */
    LARGE_INTEGER Mft2StartLcn;

    /**
     The beginning of the region of the volume used to host MFT allocations.
     */
    LARGE_INTEGER MftZoneStart;

    /**
     The end of the region of the volume used to host MFT allocations.
     */
    LARGE_INTEGER MftZoneEnd;

} NTFS_VOLUME_DATA_BUFFER;

/**
 Pointer to information returned from FSCTL_GET_NTFS_VOLUME_DATA.
 */
typedef NTFS_VOLUME_DATA_BUFFER *PNTFS_VOLUME_DATA_BUFFER;

#endif

#ifndef FSCTL_GET_REFS_VOLUME_DATA
/**
 Specifies the FSCTL_GET_REFS_VOLUME_DATA numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_GET_REFS_VOLUME_DATA       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 182, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 Information returned from FSCTL_GET_REFS_VOLUME_DATA.
 */
typedef struct {

    /**
     The number of bytes populated into the output structure.
     */
    DWORD ByteCount;

    /**
     The major version of the file system.
     */
    DWORD MajorVersion;

    /**
     The minor version of the file system.
     */
    DWORD MinorVersion;

    /**
     The bytes in each physical sector.
     */
    DWORD BytesPerPhysicalSector;

    /**
     The full 64 bit serial number.
     */
    LARGE_INTEGER VolumeSerialNumber;

    /**
     The number of sectors on the volume.
     */
    LARGE_INTEGER NumberSectors;

    /**
     The number of clusters on the volume.
     */
    LARGE_INTEGER TotalClusters;

    /**
     The number of free clusters on the volume.
     */
    LARGE_INTEGER FreeClusters;

    /**
     The number of reserved clusters on the volume.
     */
    LARGE_INTEGER TotalReserved;

    /**
     The bytes in each logical sector.
     */
    DWORD BytesPerSector;

    /**
     The bytes in each file system allocation unit.
     */
    DWORD BytesPerCluster;

    /**
     The largest file that may be stored directly in the directory.
     */
    LARGE_INTEGER MaximumSizeOfResidentFile;

    /**
     Reserved space to add extra pieces of information without needing to
     recompile all code using this structure.
     */
    LARGE_INTEGER Reserved[10];

} REFS_VOLUME_DATA_BUFFER;

/**
 Pointer to information returned from FSCTL_GET_REFS_VOLUME_DATA.
 */
typedef REFS_VOLUME_DATA_BUFFER *PREFS_VOLUME_DATA_BUFFER;

#endif

#ifndef FSCTL_GET_RETRIEVAL_POINTERS
/**
 Specifies the FSCTL_GET_RETRIEVAL_POINTERS numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_GET_RETRIEVAL_POINTERS     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 28,  METHOD_NEITHER, FILE_ANY_ACCESS)

/**
 Specifies information required to request file fragmentation information.
 */
typedef struct {

    /**
     Specifies the starting offset within the file where fragment information
     is requested.
     */
    LARGE_INTEGER StartingVcn;

} STARTING_VCN_INPUT_BUFFER;

/**
 Pointer to information required to request file fragmentation information.
 */
typedef STARTING_VCN_INPUT_BUFFER *PSTARTING_VCN_INPUT_BUFFER;

/**
 A buffer returned when enumerating file fragments.  Defined here for when
 the compilation environment doesn't define it.
 */
typedef struct RETRIEVAL_POINTERS_BUFFER {

    /**
     The number of extents/fragments.
     */
    DWORD ExtentCount;

    /**
     The file offset corresponding to the information that this structure
     describes.
     */
    LARGE_INTEGER StartingVcn;

    /**
     An array of extents returned as part of this query.  The IOCTL result
     indicates how many array entries must be present.
     */
    struct {

        /**
         Indicates the file offset described by the next entry in this array
         (ie., not this one.)
         */
        LARGE_INTEGER NextVcn;

        /**
         Indicates the volume offset described by this entry in the array.
         */
        LARGE_INTEGER Lcn;
    } Extents[1];

} RETRIEVAL_POINTERS_BUFFER, *PRETRIEVAL_POINTERS_BUFFER;
#endif

#ifndef FSCTL_QUERY_ALLOCATED_RANGES
/**
 Specifies the FSCTL_QUERY_ALLOCATED_RANGES numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_QUERY_ALLOCATED_RANGES    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 51,  METHOD_NEITHER, FILE_READ_DATA)

/**
 Specifies a single range of the file that is currently allocated.
 */
typedef struct _FILE_ALLOCATED_RANGE_BUFFER {

    /**
     The beginning of the range, in bytes.
     */
    LARGE_INTEGER FileOffset;

    /**
     The length of the range, in bytes.
     */
    LARGE_INTEGER Length;

} FILE_ALLOCATED_RANGE_BUFFER, *PFILE_ALLOCATED_RANGE_BUFFER;

#endif

#ifndef FSCTL_GET_OBJECT_ID
/**
 Specifies the FSCTL_GET_OBJECT_ID numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_GET_OBJECT_ID             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 39, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 Information about a file's object ID if the compilation environment doesn't
 provide it.
 */
typedef struct _FILE_OBJECTID_BUFFER {

    /**
     The object ID of the file (really a GUID.)
     */
    BYTE  ObjectId[16];

    /**
     The extended object ID information (three GUIDs specifying volume and
     other things we don't care about.)
     */
    BYTE  ExtendedInfo[48];

} FILE_OBJECTID_BUFFER, *PFILE_OBJECTID_BUFFER;
#endif

#ifndef FSCTL_READ_FILE_USN_DATA

/**
 Specifies the FSCTL_READ_FILE_USN_DATA numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_READ_FILE_USN_DATA         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 58,  METHOD_NEITHER, FILE_ANY_ACCESS)

/**
 A USN record, provided for environments where the compiler doesn't define
 it.
 */
typedef struct {

    /**
     Specifies the offset in bytes from the beginning of this record to the
     next record in any list.  Zero indicates the end of the list.
     */
    DWORD          RecordLength;

    /**
     Specifies the major version of the USN structure.
     */
    WORD           MajorVersion;

    /**
     Specifies the minor version of the USN structure.
     */
    WORD           MinorVersion;

    /**
     Specifies the file's file ID.
     */
    DWORDLONG      FileReferenceNumber;

    /**
     Specifies the parent directory's file ID.
     */
    DWORDLONG      ParentFileReferenceNumber;

    /**
     Specifies the USN associated with this change.
     */
    LONGLONG       Usn;

    /**
     Specifies the time the record was generated, in NT units.
     */
    LARGE_INTEGER  TimeStamp;

    /**
     Specifies the set of changes that happened to the file.
     */
    DWORD          Reason;

    /**
     Specifies whether the record was a result of background processing
     of some type of user actions.
     */
    DWORD          SourceInfo;

    /**
     Specifies the file's security information at the time the record was
     generated.
     */
    DWORD          SecurityId;

    /**
     Specifies file attributes at the time the record was generated.
     */
    DWORD          FileAttributes;

    /**
     Specifies the length of the file name, in bytes (not characters.)
     */
    WORD           FileNameLength;

    /**
     Specifies the offset in bytes from the beginning of this structure
     to the file name.
     */
    WORD           FileNameOffset;

    /**
     Specifies an array of characters corresponding to a file's name,
     not including any parent directory.
     */
    WCHAR          FileName[1];

} USN_RECORD;

/**
 A pointer to a USN record, provided for environments where the compiler
 doesn't define it.
 */
typedef USN_RECORD *PUSN_RECORD;
#endif

#ifndef FSCTL_QUERY_USN_JOURNAL

/**
 Specifies the FSCTL_QUERY_USN_JOURNAL numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_QUERY_USN_JOURNAL         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 61,  METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 Information returned from FSCTL_QUERY_USN_JOURNAL where compiler doesn't
 define it.
 */
typedef struct {

    /**
     The USN journal identifier. This is generated each time a new journal is
     created.
     */
    DWORDLONG UsnJournalID;

    /**
     The first valid USN record within the journal.
     */
    DWORDLONG FirstUsn;

    /**
     The next USN number to allocate.  All numbers between FirstUsn and
     NextUsn exclusive are valid in the journal.
     */
    DWORDLONG NextUsn;

    /**
     The lowest valid USN number.
     */
    DWORDLONG LowestValidUsn;

    /**
     The maximum valid USN number.
     */
    DWORDLONG MaxUsn;

    /**
     The maximum size of the journal in bytes.
     */
    DWORDLONG MaximumSize;

    /**
     The amount of allocation to add and remove from a journal in a single
     operation.
     */
    DWORDLONG AllocationDelta;

} USN_JOURNAL_DATA;

/**
 Pointer to information returned from FSCTL_QUERY_USN_JOURNAL where compiler
 doesn't define it.
 */
typedef USN_JOURNAL_DATA *PUSN_JOURNAL_DATA;

#endif


#ifndef FSCTL_GET_EXTERNAL_BACKING

/**
 Specifies the FSCTL_GET_EXTERNAL_BACKING numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_GET_EXTERNAL_BACKING       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 196, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef FSCTL_SET_EXTERNAL_BACKING

/**
 Specifies the FSCTL_SET_EXTERNAL_BACKING numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_SET_EXTERNAL_BACKING       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 195, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef FSCTL_DELETE_EXTERNAL_BACKING

/**
 Specifies the FSCTL_DELETE_EXTERNAL_BACKING numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_DELETE_EXTERNAL_BACKING    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 197, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef FSCTL_GET_WOF_VERSION
/**
 Specifies the FSCTL_GET_WOF_VERSION numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_GET_WOF_VERSION            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 218, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef WOF_PROVIDER_WIM

/**
 Identifies the WIM provider within WOF.
 */
#define WOF_PROVIDER_WIM                 (0x0001)

/**
 Information about a file whose contents are provided via WOF.
 */
typedef struct _WOF_EXTERNAL_INFO {

    /**
     The version for this structure.
     */
    DWORD Version;

    /**
     Indicates the sub provider that provides file data for this file.
     */
    DWORD Provider;
} WOF_EXTERNAL_INFO, *PWOF_EXTERNAL_INFO;

/**
 Indicates the number of bytes used for the hash identifying files within
 a WIM.  This is 20 bytes (SHA1.)
 */
#define WIM_PROVIDER_HASH_SIZE           20

/**
 Information about a WOF WIM backed file.
 */
typedef struct _WIM_PROVIDER_EXTERNAL_INFO {

    /**
     The version for this structure.
     */
    DWORD Version;

    /**
     Flags associated with this file.
     */
    DWORD Flags;

    /**
     Identifier for the WIM file that provides data for this file.
     */
    LARGE_INTEGER DataSourceId;

    /**
     Hash identifying the contents of this file from within the WIM.
     */
    BYTE  ResourceHash[WIM_PROVIDER_HASH_SIZE];
} WIM_PROVIDER_EXTERNAL_INFO, *PWIM_PROVIDER_EXTERNAL_INFO;
#endif

#ifndef WOF_PROVIDER_FILE

/**
 Identifier for individual file compression with WOF.
 */
#define WOF_PROVIDER_FILE                   (0x0002)

/**
 Identifier for a file compressed via WOF with XPress 4Kb chunk size
 compression.
 */
#define FILE_PROVIDER_COMPRESSION_XPRESS4K  (0x0000)

/**
 Identifier for a file compressed via WOF with LZX 32Kb chunk size
 compression.
 */
#define FILE_PROVIDER_COMPRESSION_LZX       (0x0001)

/**
 Identifier for a file compressed via WOF with XPress 8Kb chunk size
 compression.
 */
#define FILE_PROVIDER_COMPRESSION_XPRESS8K  (0x0002)

/**
 Identifier for a file compressed via WOF with XPress 16Kb chunk size
 compression.
 */
#define FILE_PROVIDER_COMPRESSION_XPRESS16K (0x0003)

/**
 Information about a WOF individually compressed file.
 */
typedef struct _FILE_PROVIDER_EXTERNAL_INFO {

    /**
     The version of this structure.
     */
    DWORD Version;

    /**
     The algorithm used for compressing this WOF individually compressed file.
     */
    DWORD Algorithm;

    /**
     Flags for individual WOF compressed files.
     */
    DWORD Flags;
} FILE_PROVIDER_EXTERNAL_INFO, *PFILE_PROVIDER_EXTERNAL_INFO;

#endif

//
//  These were added in Vista but because they're not preprocessor
//  they're very hard to test for, so we test for something else
//

#ifndef GetFinalPathNameByHandle

/**
 A structure describing file information, provided here for when the
 compilation environment doesn't provide it.
 */
typedef struct _FILE_STANDARD_INFO {

    /**
     The file system's allocation size fo the file, in bytes.
     */
    LARGE_INTEGER AllocationSize;

    /**
     The file size, in bytes.
     */
    LARGE_INTEGER EndOfFile;

    /**
     The number of hardlinks on the file.
     */
    DWORD NumberOfLinks;

    /**
     TRUE if the file is awaiting deletion (which is hard since we opened
     it somehow.)
     */
    BOOLEAN DeletePending;

    /**
     TRUE if the file is a directory.
     */
    BOOLEAN Directory;
} FILE_STANDARD_INFO, *PFILE_STANDARD_INFO;

/**
 The identifier of the request type that returns the above structure.
 */
#define FileStandardInfo    (0x000000001)

/**
 The identifier of the request type that renames a file.  This is here
 because it's already defined when SetFileInformationByHandle is defined,
 but the structure for it needs to be supplied later.
 */
#define FileRenameInfo      (0x000000003)

/**
 A structure to set or clear the delete disposition on a stream or link.
 */
typedef struct _FILE_DISPOSITION_INFO {

    /**
     TRUE to mark the link or stream for deletion on last handle close.
     FALSE to clear this intention.
     */
    BOOL DeleteFile;

} FILE_DISPOSITION_INFO, *PFILE_DISPOSITION_INFO;

/**
 The identifier of the request type that issues requests with the above
 structure.
 */
#define FileDispositionInfo (0x000000004)

#endif

#ifndef FILE_DISPOSITION_FLAG_DELETE

/**
 Indicates that the file should be marked for delete.
 */
#define FILE_DISPOSITION_FLAG_DELETE          0x0001

/**
 Indicates that the file should be deleted with POSIX semantics.
 */
#define FILE_DISPOSITION_FLAG_POSIX_SEMANTICS 0x0002

/**
 A structure to set or clear the delete disposition on a stream or link.
 */
typedef struct _FILE_DISPOSITION_INFO_EX {

    /**
     A combination of FILE_DISPOSITION_FLAG_ values .
     */
    DWORD Flags;

} FILE_DISPOSITION_INFO_EX, *PFILE_DISPOSITION_INFO_EX;

/**
 The identifier of the request type that issues requests with the above
 structure.
 */
#define FileDispositionInfoEx (0x000000015)

/**
 The identifier of the request type that renames a file with extended flags.
 */
#define FileRenameInfoEx      (0x000000016)

#endif

#ifndef FILE_RENAME_FLAG_REPLACE_IF_EXISTS
/**
 A flag to replace an already existing file on superseding rename if the
 current compilation environment doesn't define it.
 */
#define FILE_RENAME_FLAG_REPLACE_IF_EXISTS 0x0001
#endif

#ifndef FILE_RENAME_FLAG_POSIX_SEMANTICS
/**
 A flag to apply POSIX rename semantics by removing existing in use files
 from the namespace immediately if the current compilation environment doesn't
 define it.
 */
#define FILE_RENAME_FLAG_POSIX_SEMANTICS 0x0002
#endif

/**
 An extended rename structure.  This is defined unconditionally with a
 different name because the SDK version of this structure changes depending
 on which OS version is being targeted.
 */
typedef struct _YORI_FILE_RENAME_INFO {

    /**
     Flags, combined from FILE_RENAME_FLAG_* above.
     */
    DWORD Flags;

    /**
     Handle to the target directory if performing a relative rename.  The
     kernel needs to reopen this anyway, so there's not much point using it.
     */
    HANDLE RootDirectory;

    /**
     Filename length in bytes.
     */
    DWORD FileNameLength;

    /**
     Filename string, trailing this structure.  Windows expects this to be
     NULL terminated despite taking a length value above.
     */
    WCHAR FileName[1];
} YORI_FILE_RENAME_INFO, *PYORI_FILE_RENAME_INFO;

#ifndef STORAGE_INFO_FLAGS_ALIGNED_DEVICE

/**
 A structure describing sector information about a storage device.
 */
typedef struct _FILE_STORAGE_INFO {

    /**
     The traditional sector size describing the smallest unit that can be
     read or written to the device via its interface.
     */
    ULONG LogicalBytesPerSector;

    /**
     The smallest unit that the device can write in a single operation. A
     device may write a larger amount internally than it is willing to
     accept via its interface by performing reading and writing internally.
     */
    ULONG PhysicalBytesPerSectorForAtomicity;

    /**
     The unit that a device can read or write without incurring a performance
     penalty.
     */
    ULONG PhysicalBytesPerSectorForPerformance;

    /**
     The smallest unit that the file system is going to treat as an atomic
     write.  This value may have adjustments applied over the raw device
     requirement.
     */
    ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;

    /**
     Flags, unused in this program.
     */
    ULONG Flags;

    /**
     Alignment within the first physical sector of the first logical sector.
     */
    ULONG ByteOffsetForSectorAlignment;

    /**
     Alignment of partitions on the device to ensure physical sector
     alignment.
     */
    ULONG ByteOffsetForPartitionAlignment;
} FILE_STORAGE_INFO, *PFILE_STORAGE_INFO;

/**
 The identifier of the request type that returns the above structure.
 */
#define FileStorageInfo    (0x000000010)

#endif

#ifndef IMAGE_FILE_MACHINE_AMD64
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is for an AMD64 NT based machine.
 */
#define IMAGE_FILE_MACHINE_AMD64   0x8664
#endif

#ifndef IMAGE_FILE_MACHINE_ARMNT
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is for an ARM32 NT based machine.
 */
#define IMAGE_FILE_MACHINE_ARMNT   0x01c4
#endif

#ifndef IMAGE_FILE_MACHINE_ARM64
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is for an ARM64 machine.
 */
#define IMAGE_FILE_MACHINE_ARM64   0xAA64
#endif

#ifndef IMAGE_FILE_MACHINE_IA64
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is for an IA64 machine.
 */
#define IMAGE_FILE_MACHINE_IA64    0x0200
#endif

#ifndef IMAGE_SUBSYSTEM_NATIVE_WINDOWS
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is a native NT application.
 */
#define IMAGE_SUBSYSTEM_NATIVE_WINDOWS 8
#endif

#ifndef IMAGE_SUBSYSTEM_WINDOWS_CE_GUI
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is a Windows CE GUI application.
 */
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI 9
#endif

#ifndef IMAGE_SUBSYSTEM_EFI_APPLICATION
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is an EFI application.
 */
#define IMAGE_SUBSYSTEM_EFI_APPLICATION 10
#endif

#ifndef IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is an EFI boot service driver.
 */
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER 11
#endif

#ifndef IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is an EFI runtime driver.
 */
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER 12
#endif

#ifndef IMAGE_SUBSYSTEM_EFI_ROM
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is an EFI ROM.
 */
#define IMAGE_SUBSYSTEM_EFI_ROM 13
#endif

#ifndef IMAGE_SUBSYSTEM_XBOX
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is for an XBox.
 */
#define IMAGE_SUBSYSTEM_XBOX 14
#endif

#ifndef IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is a Windows Boot Application.
 */
#define IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION 16
#endif

#ifndef IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG
/**
 If the compilation environment doesn't provide it, the value for an
 executable file that is an XBox Code Catalog.
 */
#define IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG 17
#endif

#ifndef NeedCurrentDirectoryForExePath

/**
 A structure returned by FindFirstStreamW.  Supplied for when the
 compilation environment doesn't provide it.
 */
typedef struct _WIN32_FIND_STREAM_DATA {

    /**
     The length of the stream, in bytes.
     */
    LARGE_INTEGER StreamSize;

    /**
     The stream name in a NULL terminated string.
     */
    WCHAR cStreamName[YORI_LIB_MAX_STREAM_NAME];
} WIN32_FIND_STREAM_DATA, *PWIN32_FIND_STREAM_DATA;

#endif

#ifndef GUID_DEFINED

/**
 Indicate that this file just created a definition for a GUID.
 */
#define GUID_DEFINED

/**
 A structure definition for a GUID allocation.  We don't care about the
 parts in this structure.
 */
typedef struct _GUID {

    /**
     The first 32 bits of the GUID.
     */
    DWORD Part1;

    /**
     The next 16 bits of the GUID.
     */
    WORD Part2;

    /**
     The next 16 bits of the GUID.
     */
    WORD Part3;

    /**
     The next 64 bits of the GUID.
     */
    BYTE Part4[8];
} GUID;

#endif

/**
 A processor architecture identifier for i386.
 */
#define YORI_PROCESSOR_ARCHITECTURE_INTEL           0

/**
 A processor architecture identifier for MIPS.
 */
#define YORI_PROCESSOR_ARCHITECTURE_MIPS            1

/**
 A processor architecture identifier for Alpha.
 */
#define YORI_PROCESSOR_ARCHITECTURE_ALPHA           2

/**
 A processor architecture identifier for PowerPC.
 */
#define YORI_PROCESSOR_ARCHITECTURE_PPC             3

/**
 A processor architecture identifier for ARM.
 */
#define YORI_PROCESSOR_ARCHITECTURE_ARM             5

/**
 A processor architecture identifier for Itanium.
 */
#define YORI_PROCESSOR_ARCHITECTURE_IA64            6

/**
 A processor architecture identifier for AMD64.
 */
#define YORI_PROCESSOR_ARCHITECTURE_AMD64           9

/**
 A virtual processor architecture identifier for an i386 binary running under
 emulation on any 64 bit host environment.
 */
#define YORI_PROCESSOR_ARCHITECTURE_IA32_ON_WIN64   10

/**
 A processor architecture identifier for ARM64.
 */
#define YORI_PROCESSOR_ARCHITECTURE_ARM64           12

/**
 A virtual processor architecture identifier for an arm binary running under
 emulation on any 64 bit host environment.
 */
#define YORI_PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64  13

/**
 A virtual processor architecture identifier for an i386 binary running under
 emulation on an arm64 host environment.
 */
#define YORI_PROCESSOR_ARCHITECTURE_IA32_ON_ARM64   14

/**
 An unknown processor architecture identifier.
 */
#define YORI_PROCESSOR_ARCHITECTURE_UNKNOWN         0xFFFF

/**
 A processor identifier for a 386.  This processor is part of the
 YORI_PROCESSOR_ARCHITECTURE_INTEL family.
 */
#define YORI_PROCESSOR_INTEL_386                    386

/**
 A processor identifier for a 486.  This processor is part of the
 YORI_PROCESSOR_ARCHITECTURE_INTEL family.
 */
#define YORI_PROCESSOR_INTEL_486                    486

/**
 A processor identifier for a Pentium.  This processor is part of the
 YORI_PROCESSOR_ARCHITECTURE_INTEL family.
 */
#define YORI_PROCESSOR_INTEL_PENTIUM                586

/**
 A processor identifier for a 686.  This processor is part of the
 YORI_PROCESSOR_ARCHITECTURE_INTEL family.
 */
#define YORI_PROCESSOR_INTEL_686                    686

/**
 A processor identifier for an R4000.  This processor is part of the
 YORI_PROCESSOR_ARCHITECTURE_MIPS family.
 */
#define YORI_PROCESSOR_MIPS_R4000                   4000

/**
 A processor identifier for a 21064.  This processor is part of the
 YORI_PROCESSOR_ARCHITECTURE_ALPHA family.
 */
#define YORI_PROCESSOR_ALPHA_21064                  21064

/**
 A processor identifier for a 601.  This processor is part of the
 YORI_PROCESSOR_ARCHITECTURE_PPC family.
 */
#define YORI_PROCESSOR_PPC_601                      601

/**
 A processor identifier for a 603.  This processor is part of the
 YORI_PROCESSOR_ARCHITECTURE_PPC family.
 */
#define YORI_PROCESSOR_PPC_603                      603

/**
 A processor identifier for a 604.  This processor is part of the
 YORI_PROCESSOR_ARCHITECTURE_PPC family.
 */
#define YORI_PROCESSOR_PPC_604                      604

/**
 A processor identifier for a 620.  This processor is part of the
 YORI_PROCESSOR_ARCHITECTURE_PPC family.
 */
#define YORI_PROCESSOR_PPC_620                      620

#ifndef IMAGE_FILE_MACHINE_R10000
/**
 An executable image machine type for MIPS R10000 if the compilation
 environment didn't supply it.
 */
#define IMAGE_FILE_MACHINE_R10000                  (0x0168)
#endif

#ifndef IMAGE_FILE_MACHINE_POWERPCFP
/**
 An executable image machine type for PowerPC with floating point if the
 compilation environment didn't supply it.
 */
#define IMAGE_FILE_MACHINE_POWERPCFP               (0x01f1)
#endif

#ifndef IMAGE_FILE_MACHINE_IA64
/**
 An executable image machine type for Itanium if the compilation environment
 didn't supply it.
 */
#define IMAGE_FILE_MACHINE_IA64                    (0x0200)
#endif

#ifndef IMAGE_FILE_MACHINE_ARMNT
/**
 An executable image machine type for ARM if the compilation environment
 didn't supply it.
 */
#define IMAGE_FILE_MACHINE_ARMNT                   (0x01c4)
#endif

#ifndef IMAGE_FILE_MACHINE_AMD64
/**
 An executable image machine type for AMD64 if the compilation environment
 didn't supply it.
 */
#define IMAGE_FILE_MACHINE_AMD64                   (0x8664)
#endif

#ifndef IMAGE_FILE_MACHINE_ARM64
/**
 An executable image machine type for ARM64 if the compilation environment
 didn't supply it.
 */
#define IMAGE_FILE_MACHINE_ARM64                   (0xAA64)
#endif

/**
 A structure containing the core fields of a PE header.
 */
typedef struct _YORILIB_PE_HEADERS {
    /**
     The signature indicating a PE file.
     */
    DWORD Signature;

    /**
     The base PE header.
     */
    IMAGE_FILE_HEADER ImageHeader;

    /**
     The contents of the PE optional header.  This isn't really optional in
     NT since it contains core fields needed for NT to run things.
     */
    IMAGE_OPTIONAL_HEADER OptionalHeader;
} YORILIB_PE_HEADERS, *PYORILIB_PE_HEADERS;

#ifndef CONSOLE_FULLSCREEN
/**
 A private definition of CONSOLE_FULLSCREEN in case the compilation
 environment doesn't provide it.
 */
#define CONSOLE_FULLSCREEN 1
#endif

#ifndef CONSOLE_FULLSCREEN_MODE
/**
 A private definition of CONSOLE_FULLSCREEN_MODE in case the compilation
 environment doesn't provide it.
 */
#define CONSOLE_FULLSCREEN_MODE 1
#endif

#ifndef CONSOLE_WINDOWED_MODE
/**
 A private definition of CONSOLE_WINDOWED_MODE in case the compilation
 environment doesn't provide it.
 */
#define CONSOLE_WINDOWED_MODE 2
#endif

/**
 A private definition of CONSOLE_FONT_INFOEX in case the compilation
 environment doesn't provide it.
 */
typedef struct _YORI_CONSOLE_FONT_INFOEX {
    /**
     The size of the structure in bytes.
     */
    DWORD cbSize;

    /**
     The index of the font in the console font table.  This is really quite
     useless for applications.
     */
    DWORD nFont;

    /**
     The dimensions to each character in the font.
     */
    COORD dwFontSize;

    /**
     The family of the font.
     */
    UINT FontFamily;

    /**
     The weight (boldness) of the font.
     */
    UINT FontWeight;

    /**
     The font's name.  This is actually useful for applications.
     */
    WCHAR FaceName[LF_FACESIZE];
} YORI_CONSOLE_FONT_INFOEX, *PYORI_CONSOLE_FONT_INFOEX;

/**
 A private definition of CONSOLE_SCREEN_BUFFER_INFOEX in case the compilation
 environment doesn't provide it.
 */
typedef struct _YORI_CONSOLE_SCREEN_BUFFER_INFOEX {

    /**
     The number of bytes in this structure.
     */
    DWORD cbSize;

    /**
     The size of the window buffer.
     */
    COORD dwSize;

    /**
     The position of the cursor within the window buffer.
     */
    COORD dwCursorPosition;

    /**
     The color attribute to use when writing new characters to the console.
     */
    WORD wAttributes;

    /**
     The subset of the buffer that is currently displayed on the window.
     */
    SMALL_RECT srWindow;

    /**
     The maximum size that the window could become.
     */
    COORD dwMaximumWindowSize;

    /**
     The color attribute to use for popups.
     */
    WORD wPopupAttributes;

    /**
     TRUE if the console can be made to run in full screen mode.
     */
    BOOL bFullScreenSupported;

    /**
     A mapping table describing the RGB values to use for the 16 console
     colors.
     */
    DWORD ColorTable[16];
} YORI_CONSOLE_SCREEN_BUFFER_INFOEX, *PYORI_CONSOLE_SCREEN_BUFFER_INFOEX;

/**
 Structure to change basic accounting information about a job.
 */
typedef struct _YORI_JOB_BASIC_ACCOUNTING_INFORMATION {

    /**
     The total amount of user mode processing consumed by the job.
     */
    LARGE_INTEGER TotalUserTime;

    /**
     The total amount of kernel mode processing consumed by the job.
     */
    LARGE_INTEGER TotalKernelTime;

    /**
     Field not needed/supported by YoriLib.
     */
    LARGE_INTEGER Unused1;

    /**
     Field not needed/supported by YoriLib.
     */
    LARGE_INTEGER Unused2;

    /**
     Field not needed/supported by YoriLib.
     */
    DWORD Unused3;

    /**
     The total number of processes that have been initiated.
     */
    DWORD TotalProcesses;

    /**
     The number of currently active processes.
     */
    DWORD ActiveProcesses;

    /**
     Field not needed/supported by YoriLib.
     */
    DWORD Unused4;

} YORI_JOB_BASIC_ACCOUNTING_INFORMATION, *PYORI_JOB_BASIC_ACCOUNTING_INFORMATION;

/**
 Structure to change basic information about a job.
 */
typedef struct _YORI_JOB_BASIC_LIMIT_INFORMATION {

    /**
     Field not needed/supported by YoriLib.
     */
    LARGE_INTEGER Unused1;

    /**
     Field not needed/supported by YoriLib.
     */
    LARGE_INTEGER Unused2;

    /**
     Indicates which fields should be interpreted when setting information on
     a job.
     */
    DWORD Flags;

    /**
     Field not needed/supported by YoriLib.
     */
    SIZE_T Unused3;

    /**
     Field not needed/supported by YoriLib.
     */
    SIZE_T Unused4;

    /**
     Field not needed/supported by YoriLib.
     */
    DWORD Unused5;

    /**
     Field not needed/supported by YoriLib.
     */
    SIZE_T Unused6;

    /**
     The base process priority to assign to the job.
     */
    DWORD Priority;

    /**
     Field not needed/supported by YoriLib.
     */
    DWORD Unused7;
} YORI_JOB_BASIC_LIMIT_INFORMATION, *PYORI_JOB_BASIC_LIMIT_INFORMATION;

/**
 Information specifying how to associate a job object handle with a completion
 port.
 */
typedef struct _YORI_JOB_ASSOCIATE_COMPLETION_PORT {

    /**
     A context pointer to associate with messages arriving on the completion
     port.
     */
    PVOID Key;

    /**
     The completion port to associate the job object with.
     */
    HANDLE Port;
} YORI_JOB_ASSOCIATE_COMPLETION_PORT, *PYORI_JOB_ASSOCIATE_COMPLETION_PORT;


#ifndef SYMBOLIC_LINK_FLAG_DIRECTORY
/**
 A definition for SYMBOLIC_LINK_FLAG_DIRECTORY if it is not defined by the
 current compilation environment.
 */
#define SYMBOLIC_LINK_FLAG_DIRECTORY 1
#endif

#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
/**
 A definition for SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE if it is not
 defined by the current compilation environment.
 */
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 2
#endif

#ifndef LOCALE_RETURN_NUMBER
/**
 A definition for LOCALE_RETURN_NUMBER if it is not defined by the current
 compilation environment.
 */
#define LOCALE_RETURN_NUMBER (0x20000000)
#endif

/**
 Indicates the system is currently running from a battery.
 */
#define YORI_POWER_SOURCE_BATTERY    (0x00)

/**
 Indicates the system is currently running from AC power.
 */
#define YORI_POWER_SOURCE_POWERED    (0x01)

/**
 Indicates the system is running from an unknown power source.
 */
#define YORI_POWER_SOURCE_UNKNOWN    (0xFF)

/**
 Indicates the battery has a large amount of remaining capacity.
 */
#define YORI_BATTERY_FLAG_HIGH       (0x01)

/**
 Indicates the battery has a small amount of remaining capacity.
 */
#define YORI_BATTERY_FLAG_LOW        (0x02)

/**
 Indicates the battery has passed a critical threshold indicating the system
 needs to save state and shut down.
 */
#define YORI_BATTERY_FLAG_CRITICAL   (0x04)

/**
 Indicates the battery is currently charging.
 */
#define YORI_BATTERY_FLAG_CHARGING   (0x08)

/**
 Indicates no battery has been found.
 */
#define YORI_BATTERY_FLAG_NO_BATTERY (0x80)

/**
 Indicates an unknown battery state.
 */
#define YORI_BATTERY_FLAG_UNKNOWN    (0xFF)

/**
 Indicates an unknown battery state.
 */
#define YORI_BATTERY_PERCENT_UNKNOWN (0xFF)

/**
 A definition for SYSTEM_POWER_STATUS for compilation environments that
 don't define it.
 */
typedef struct _YORI_SYSTEM_POWER_STATUS {

    /**
     Indicates if the system is running on AC power or not.
     */
    UCHAR PowerSource;

    /**
     Indicates the current battery charge level and charging state.
     */
    UCHAR BatteryFlag;

    /**
     Indicates the percentage of the battery that remains.
     */
    UCHAR BatteryLifePercent;

    /**
     Reserved.
     */
    UCHAR Reserved;

    /**
     Indicates the estimated number of seconds that the system can remain on
     battery power.
     */
    DWORD BatterySecondsRemaining;

    /**
     Indicates the total number of seconds that the system could remain on
     battery power if the battery is fully charged.
     */
    DWORD BatteryFullSeconds;
} YORI_SYSTEM_POWER_STATUS, *PYORI_SYSTEM_POWER_STATUS;

#ifndef GetClassLongPtr
/**
 If not defined by the compilation environment, GetClassLongPtr must refer
 to GetClassLong (no 64 bit support.)
 */
#define GetClassLongPtr GetClassLong
#endif

#ifndef GetWindowLongPtr
/**
 If not defined by the compilation environment, GetWindowLongPtr must refer
 to GetWindowLong (no 64 bit support.)
 */
#define GetWindowLongPtr GetWindowLong
#endif

#ifndef SetWindowLongPtr
/**
 If not defined by the compilation environment, SetWindowLongPtr must refer
 to SetWindowLong (no 64 bit support.)
 */
#define SetWindowLongPtr SetWindowLong
#endif

#ifndef GCLP_HICONSM
/**
 If not defined by the compilation environment, the slot indicating the
 small icon associated with a window class.
 */
#define GCLP_HICONSM (-34)
#endif

#ifndef GWLP_WNDPROC
/**
 If not defined by the compilation environment, the slot indicating the
 window procedure on a window.
 */
#define GWLP_WNDPROC (-4)
#endif

#ifndef SPI_GETMINIMIZEDMETRICS
/**
 If not defined by the compilation environment, the parameter indicating how
 to display minimized windows.
 */
#define SPI_GETMINIMIZEDMETRICS 0x2b
#endif

#ifndef SPI_SETMINIMIZEDMETRICS
/**
 If not defined by the compilation environment, the parameter indicating how
 to display minimized windows.
 */
#define SPI_SETMINIMIZEDMETRICS 0x2c
#endif

#ifndef SPI_GETWORKAREA
/**
 Value to SystemParametersInfo to query the size of the primary screen,
 reduced by any task bar area.
 */
#define SPI_GETWORKAREA (48)
#endif

#ifndef SPI_SETWORKAREA
/**
 A definition for SPI_SETWORKAREA if it is not defined by the current
 compilation environment.
 */
#define SPI_SETWORKAREA 47
#endif

#ifndef SM_CXSMICON
/**
 If not defined by the compilation environment, the metric indicating the
 width of a small icon.
 */
#define SM_CXSMICON 49
#endif

#ifndef SM_CYSMICON
/**
 If not defined by the compilation environment, the metric indicating the
 height of a small icon.
 */
#define SM_CYSMICON 50
#endif

#ifndef HSHELL_REDRAW
/**
 If not defined by the compilation environment, the flag indicating a top
 level window title has changed.
 */
#define HSHELL_REDRAW 6
#endif

#ifndef HSHELL_WINDOWACTIVATED
/**
 If not defined by the compilation environment, the flag indicating the active
 top level window has changed.
 */
#define HSHELL_WINDOWACTIVATED 4
#endif

#ifndef HSHELL_RUDEAPPACTIVATED
/**
 A definition for HSHELL_RUDEAPPACTIVATED if it is not defined by the current
 compilation environment.
 */
#define HSHELL_RUDEAPPACTIVATED (0x8000 | HSHELL_WINDOWACTIVATED)
#endif

#ifndef HSHELL_FLASH
/**
 A definition for HSHELL_FLASH if it is not defined by the current compilation
 environment.
 */
#define HSHELL_FLASH (0x8000 | HSHELL_REDRAW)
#endif

#ifndef ARW_HIDE
/**
 If not defined by the compilation environment, the flag that minimized
 windows should be hidden.
 */
#define ARW_HIDE 0x8
#endif

#ifndef MOD_WIN
/**
 If not defined by the compilation environment, the flag that indicates a
 hotkey is a combination including the Windows key.
 */
#define MOD_WIN 0x8
#endif

/**
 If not defined by the compilation environment, a structure definint how
 minimized windows should behave.
 */
typedef struct _YORI_MINIMIZEDMETRICS {

    /**
     The size of this structure, in bytes.
     */
    DWORD cbSize;

    /**
     Width of minimized windows, in pixels.
     */
    INT iWidth;

    /**
     Horizontal space between minimized windows, in pixels.
     */
    INT iHorizontalGap;

    /**
     Vertical space between minimized windows, in pixels.
     */
    INT iVerticalGap;

    /**
     Vertical space between minimized windows, in pixels.
     */
    INT iArrange;
} YORI_MINIMIZEDMETRICS, *PYORI_MINIMIZEDMETRICS;

#ifndef WM_DISPLAYCHANGE
/**
 If not defined by the compilation environment, the message indicating that
 the screen resolution has changed.
 */
#define WM_DISPLAYCHANGE 0x007e
#endif

#ifndef WM_WTSSESSION_CHANGE
/**
 If not defined by the compilation environment, the message indicating that
 the session has changed.
 */
#define WM_WTSSESSION_CHANGE 0x02b1
#endif

#ifndef WM_SETICON
/**
 A definition for WM_SETICON if it is not defined by the current compilation
 environment.
 */
#define WM_SETICON (0x0080)
#endif

#ifndef ICON_SMALL
/**
 A definition for a small icon in WM_SETICON if it is not defined by the
 current compilation environment.
 */
#define ICON_SMALL 0
#endif

#ifndef ICON_BIG
/**
 A definition for a big icon in WM_SETICON if it is not defined by the
 current compilation environment.
 */
#define ICON_BIG 1
#endif

#ifndef IMAGE_ICON
/**
 A definition for the icon type in LoadImage if it is not defined by the
 current compilation environment.
 */
#define IMAGE_ICON 1
#endif

#ifndef MONO_FONT
/**
 A definition for a monospaced font if it is not defined by the current
 compilation environment.
 */
#define MONO_FONT 8
#endif

#ifndef STARTF_TITLEISLINKNAME
/**
 Indicate that the title field in STARTUPINFO is really a shortcut name so the
 console can populate console properties from it.
 */
#define STARTF_TITLEISLINKNAME 0x800
#endif

#ifndef EWX_POWEROFF
/**
 Flag to tell the system to power off after shutdown.
 */
#define EWX_POWEROFF 0x00000008
#endif

#ifndef DFC_BUTTON
/**
 If not defined by the compilation environment, the index indicating a push
 button control..
 */
#define DFC_BUTTON 0x4
#endif

#ifndef DFCS_BUTTONPUSH
/**
 If not defined by the compilation environment, the style indicating a push
 button.
 */
#define DFCS_BUTTONPUSH 0x10
#endif

#ifndef DFCS_PUSHED
/**
 If not defined by the compilation environment, the style indicating a push
 button that is currently "pushed".
 */
#define DFCS_PUSHED 0x200
#endif

#ifndef DI_NORMAL
/**
 If not defined by the compilation environment, the style indicating a regular
 icon draw.
 */
#define DI_NORMAL 0x3
#endif

#ifndef DT_END_ELLIPSIS
/**
 If not defined by the compilation environment, the style indicating text
 that doesn't fit in its bounding rectangle should display an ellipsis.
 */
#define DT_END_ELLIPSIS 0x8000
#endif

#ifndef WS_EX_TOOLWINDOW
/**
 If not defined by the compilation environment, a flag indicating a window is
 a helper window that should not be included in the taskbar.
 */
#define WS_EX_TOOLWINDOW 0x0080
#endif

#ifndef WS_EX_STATICEDGE
/**
 If not defined by the compilation environment, a flag indicating a window 
 should have a 3D border indicating it does not accept user input.
 */
#define WS_EX_STATICEDGE 0x20000
#endif

#ifndef WS_EX_NOACTIVATE
/**
 If not defined by the compilation environment, a flag indicating a window 
 should not receive input focus.
 */
#define WS_EX_NOACTIVATE 0x8000000
#endif

#ifndef BS_LEFT
/**
 If not defined by the compilation environment, the flag indicating button
 text should be left aligned.
 */
#define BS_LEFT 0x100
#endif

#ifndef BS_CENTER
/**
 If not defined by the compilation environment, the flag indicating button
 text should be centered.
 */
#define BS_CENTER 0x300
#endif

#ifndef SS_NOTIFY
/**
 If not defined by the compilation environment, the flag indicating a static
 control should notify its parent about mouse clicks.
 */
#define SS_NOTIFY 0x200
#endif

#ifndef SS_CENTERIMAGE
/**
 If not defined by the compilation environment, the flag indicating a static
 control should be vertically centered.
 */
#define SS_CENTERIMAGE 0x200
#endif

#ifndef SS_SUNKEN
/**
 If not defined by the compilation environment, the flag indicating a static
 control should have a sunken appearance.
 */
#define SS_SUNKEN 0x1000
#endif

#ifndef TPM_BOTTOMALIGN
/**
 If not defined by the compilation environment, the flag indicating a popup
 menu should be bottom aligned.
 */
#define TPM_BOTTOMALIGN 0x0020
#endif

#ifndef TPM_NONOTIFY
/**
 If not defined by the compilation environment, the flag indicating a popup
 menu should not generate notification messages.
 */
#define TPM_NONOTIFY    0x0080
#endif

#ifndef TPM_RETURNCMD
/**
 If not defined by the compilation environment, the flag indicating a popup
 menu should indicate the selected option as a return value.
 */
#define TPM_RETURNCMD   0x0100
#endif


#ifndef MS_DEF_PROV
/**
 A definition for the base crypto provider if it is not defined by the current
 compilation environment.
 */
#define MS_DEF_PROV L"Microsoft Base Cryptographic Provider v1.0"
#endif

#ifndef MS_ENH_RSA_AES_PROV_XP
/**
 A definition for the prototype enhanced crypto provider if it is not defined
 by the current compilation environment.
 */
#define MS_ENH_RSA_AES_PROV_XP L"Microsoft Enhanced RSA and AES Cryptographic Provider (Prototype)"
#endif

#ifndef MS_ENH_RSA_AES_PROV
/**
 A definition for the enhanced crypto provider if it is not defined by the
 current compilation environment.
 */
#define MS_ENH_RSA_AES_PROV L"Microsoft Enhanced RSA and AES Cryptographic Provider"
#endif

#ifndef CALG_MD4
/**
 A definition for the MD4 hash algorithm if it is not defined by the current
 compilation environment.
 */
#define CALG_MD4  ((4 << 13) | 2)
#endif

#ifndef CALG_MD4
/**
 A definition for the MD4 hash algorithm if it is not defined by the current
 compilation environment.
 */
#define CALG_MD4  ((4 << 13) | 2)
#endif

#ifndef CALG_MD5
/**
 A definition for the MD5 hash algorithm if it is not defined by the current
 compilation environment.
 */
#define CALG_MD5  ((4 << 13) | 3)
#endif

#ifndef CALG_SHA1
/**
 A definition for the SHA1 hash algorithm if it is not defined by the current
 compilation environment.
 */
#define CALG_SHA1 ((4 << 13) | 4)
#endif

#ifndef CALG_SHA_256
/**
 A definition for the SHA256 hash algorithm if it is not defined by the
 current compilation environment.
 */
#define CALG_SHA_256 ((4 << 13) | 12)
#endif

#ifndef CALG_SHA_384
/**
 A definition for the SHA384 hash algorithm if it is not defined by the
 current compilation environment.
 */
#define CALG_SHA_384 ((4 << 13) | 13)
#endif

#ifndef CALG_SHA_512
/**
 A definition for the SHA512 hash algorithm if it is not defined by the
 current compilation environment.
 */
#define CALG_SHA_512 ((4 << 13) | 14)
#endif

#ifndef PROV_RSA_FULL
/**
 A definition for the RSA provider if it is not defined by the current
 compilation environment.
 */
#define PROV_RSA_FULL 1
#endif

#ifndef PROV_RSA_AES
/**
 A definition for the RSA provider if it is not defined by the current
 compilation environment.
 */
#define PROV_RSA_AES 24
#endif

#ifndef NTE_BAD_KEYSET
/**
 A definition for an HRESULT that is communicated via Win32 lasterror (huh?)
 to indicate the lack of a key store if the compilation environment doesn't
 define it.
 */
#define NTE_BAD_KEYSET 0x80090016
#endif

#ifndef CRYPT_VERIFYCONTEXT
/**
 A definition for a flag to tell the provider that no private key material is
 required if it is not defined by the current compilation environment.
 */
#define CRYPT_VERIFYCONTEXT 0xF0000000
#endif

#ifndef CRYPT_NEWKEYSET
/**
 A definition for a flag to tell the provider to create a new key store if
 it is not defined by the current compilation environment.
 */
#define CRYPT_NEWKEYSET 0x00000008
#endif

#ifndef HP_HASHVAL
/**
 A definition for the value to obtain the hash result if it is not defined by
 the current compilation environment.
 */
#define HP_HASHVAL 2
#endif

#ifndef HP_HASHSIZE
/**
 A definition for the value to obtain the size of the hash if it is not
 defined by the current compilation environment.
 */
#define HP_HASHSIZE 4
#endif

#ifndef SHUTDOWN_FORCE_OTHERS
/**
 A definition for the value to force shutdown if it is not defined by the
 current compilation environment.
 */
#define SHUTDOWN_FORCE_OTHERS    0x0001
#endif

#ifndef SHUTDOWN_FORCE_SELF
/**
 A definition for the value to force shutdown if it is not defined by the
 current compilation environment.
 */
#define SHUTDOWN_FORCE_SELF      0x0002
#endif

#ifndef SHUTDOWN_NOREBOOT
/**
 A definition for the value to shutdown without rebooting if it is not
 defined by the current compilation environment.
 */
#define SHUTDOWN_NOREBOOT        0x0010
#endif

#ifndef SHUTDOWN_RESTART
/**
 A definition for the value to reboot if it is not defined by the current
 compilation environment.
 */
#define SHUTDOWN_RESTART         0x0004
#endif

#ifndef SHUTDOWN_POWEROFF
/**
 A definition for the value to shutdown and power off if it is not defined
 by the current compilation environment.
 */
#define SHUTDOWN_POWEROFF        0x0008
#endif

#ifndef DIAMONDAPI
/**
 If not defined by the compilation environment, the calling convention used
 by the Cabinet APIs.
 */
#define DIAMONDAPI __cdecl
#endif

/**
 Indicates a file name is UTF8 encoded.  If not, it's ambiguous, but is
 interpreted as the active code page.  This program UTF8 encodes anything
 that can't be expressed in 7 bits.
 */
#define YORI_CAB_NAME_IS_UTF (0x80)

/**
 A prototype for FDICreate's allocation callback.
 */
typedef
LPVOID DIAMONDAPI
CAB_CB_ALLOC(ULONG);

/**
 A prototype for a pointer to FDICreate's allocation callback.
 */
typedef CAB_CB_ALLOC *PCAB_CB_ALLOC;

/**
 A prototype for FDICreate's free callback.
 */
typedef
VOID DIAMONDAPI
CAB_CB_FREE(LPVOID);

/**
 A prototype for a pointer to FDICreate's free callback.
 */
typedef CAB_CB_FREE *PCAB_CB_FREE;

/**
 A prototype for FDICreate's file open callback.
 */
typedef
DWORD_PTR DIAMONDAPI
CAB_CB_FDI_FILE_OPEN(LPSTR, INT, INT);

/**
 A prototype for a pointer to FDICreate's file open callback.
 */
typedef CAB_CB_FDI_FILE_OPEN *PCAB_CB_FDI_FILE_OPEN;

/**
 A prototype for FDICreate's file read callback.
 */
typedef
DWORD DIAMONDAPI
CAB_CB_FDI_FILE_READ(DWORD_PTR, LPVOID, DWORD);

/**
 A prototype for a pointer to FDICreate's file read callback.
 */
typedef CAB_CB_FDI_FILE_READ *PCAB_CB_FDI_FILE_READ;

/**
 A prototype for FDICreate's file write callback.
 */
typedef
DWORD DIAMONDAPI
CAB_CB_FDI_FILE_WRITE(DWORD_PTR, LPVOID, DWORD);

/**
 A prototype for a pointer to FDICreate's file write callback.
 */
typedef CAB_CB_FDI_FILE_WRITE *PCAB_CB_FDI_FILE_WRITE;

/**
 A prototype for FDICreate's file close callback.
 */
typedef
INT DIAMONDAPI
CAB_CB_FDI_FILE_CLOSE(DWORD_PTR);

/**
 A prototype for a pointer to FDICreate's file close callback.
 */
typedef CAB_CB_FDI_FILE_CLOSE *PCAB_CB_FDI_FILE_CLOSE;

/**
 A prototype for FDICreate's file seek callback.
 */
typedef
DWORD DIAMONDAPI
CAB_CB_FDI_FILE_SEEK(DWORD_PTR, DWORD, INT);

/**
 A prototype for a pointer to FDICreate's file seek callback.
 */
typedef CAB_CB_FDI_FILE_SEEK *PCAB_CB_FDI_FILE_SEEK;

/**
 The set of notification types that FDICopy can invoke its notification
 callback with.
 */
typedef enum _CAB_CB_FDI_NOTIFYTYPE {
    YoriLibCabNotifyCabinetInfo,
    YoriLibCabNotifyPartialFile,
    YoriLibCabNotifyCopyFile,
    YoriLibCabNotifyCloseFile,
    YoriLibCabNotifyNextCabinet,
    YoriLibCabNotifyEnumerate
} CAB_CB_FDI_NOTIFYTYPE;

/**
 A structure of data that FDICopy invokes its notification callback with.
 */
typedef struct _CAB_CB_FDI_NOTIFICATION {
    /**
     The meaning of this field depends on the type of notification, and the
     documentation is awful.
     */
    DWORD StructureSize;

    /**
     The meaning of this field depends on the type of notification, and the
     documentation is awful.
     */
    LPSTR String1;

    /**
     The meaning of this field depends on the type of notification, and the
     documentation is awful.
     */
    LPSTR String2;

    /**
     The meaning of this field depends on the type of notification, and the
     documentation is awful.
     */
    LPSTR String3;

    /**
     This is a pointer to the context supplied to FDICopy.
     */
    LPVOID Context;

    /**
     The file handle being operated on.  This is used when opening or closing
     target files.
     */
    DWORD_PTR FileHandle;

    /**
     File date in MSDOS format.
     */
    USHORT TinyDate;

    /**
     File time in MSDOS format.
     */
    USHORT TinyTime;

    /**
     File attributes in MSDOS format.
     */
    USHORT HalfAttributes;

    /**
     An identifier for which set of Cabinets is being used, which would be
     meaningful if this feature were being used.
     */
    USHORT CabSetId;

    /**
     The number of Cabinets in the set, which would be meaningful if
     multi-Cab sets still made any sense.
     */
    USHORT CabinetsInSetCount;

    /**
     Indicates the folder within the Cabinet.
     */
    USHORT CabinetFolderCount;

    /**
     Error code.
     */
    DWORD FdiError;
} CAB_CB_FDI_NOTIFICATION, *PCAB_CB_FDI_NOTIFICATION;

/**
 A prototype for FDICopy's notification callback.
 */
typedef
DWORD_PTR DIAMONDAPI
CAB_CB_FDI_NOTIFY(CAB_CB_FDI_NOTIFYTYPE, PCAB_CB_FDI_NOTIFICATION);

/**
 A prototype for a pointer to FDICopy's notification callback.
 */
typedef CAB_CB_FDI_NOTIFY *PCAB_CB_FDI_NOTIFY;

/**
 A structure that describes a CAB file and its characteristics for the
 Cabinet API.
 */
typedef struct _CAB_FCI_CONTEXT {

    /**
     The maximum amount of size for a given cabinet.
     */
    DWORD SizeAvailable;

    /**
     The maximum amount of size for a given folder within a cabinet.
     */
    DWORD ThresholdForNextFolder;

    /**
     The amount of space to reserve in a CAB header.
     */
    DWORD ReserveCfHeader;

    /**
     The amount of space to reserve in a CAB folder header.
     */
    DWORD ReserveCfFolder;

    /**
     The amount of space to reserve in a CAB data block (file?)
     */
    DWORD ReserveCfData;

    /**
     The cabinet number in a cabinet chain.
     */
    DWORD CabNumber;

    /**
     The disk number when splitting across disks.
     */
    DWORD DiskNumber;

    /**
     Fail if a block cannot be compressed.
     */
    DWORD FailOnIncompressible;

    /**
     The set ID, presumably to distinguish multiple cabinet chains.
     */
    WORD SetId;

    /**
     The name of the disk.  Note NULL terminated ANSI.  Also note this
     structure is paired with what cabinet.dll expects, so the size here
     is not changeable.
     */
    CHAR DiskName[256];

    /**
     The name of the cabinet.  Note NULL terminated ANSI.  Also note this
     structure is paired with what cabinet.dll expects, so the size here
     is not changeable.
     */
    CHAR CabName[256];

    /**
     The path to the cabinet.  Note NULL terminated ANSI.  Also note this
     structure is paired with what cabinet.dll expects, so the size here
     is not changeable.
     */
    CHAR CabPath[256];
} CAB_FCI_CONTEXT, *PCAB_FCI_CONTEXT;

/**
 A value to pass to FCIAddFile to request no compression.
 */
#define CAB_FCI_ALGORITHM_NONE (0x0000)

/**
 A value to pass to FCIAddFile to request MSZIP (old, lightweight)
 compression.
 */
#define CAB_FCI_ALGORITHM_MSZIP (0x0001)

/**
 A value to pass to FCIAddFile to request LZX (aggressive) compression.
 */
#define CAB_FCI_ALGORITHM_LZX (0x1503)

/**
 A prototype for FCICreate's file placed callback.
 */
typedef
DWORD_PTR DIAMONDAPI
CAB_CB_FCI_FILE_PLACED(PCAB_FCI_CONTEXT, LPSTR, DWORD, BOOL, PVOID);

/**
 A prototype for a pointer to FCICreate's file placed callback.
 */
typedef CAB_CB_FCI_FILE_PLACED *PCAB_CB_FCI_FILE_PLACED;

/**
 A prototype for FCICreate's file open callback.
 */
typedef
DWORD_PTR DIAMONDAPI
CAB_CB_FCI_FILE_OPEN(LPSTR, INT, INT, PINT, PVOID);

/**
 A prototype for a pointer to FCICreate's file open callback.
 */
typedef CAB_CB_FCI_FILE_OPEN *PCAB_CB_FCI_FILE_OPEN;

/**
 A prototype for FCICreate's file read callback.
 */
typedef
DWORD DIAMONDAPI
CAB_CB_FCI_FILE_READ(DWORD_PTR, LPVOID, DWORD, PINT, PVOID);

/**
 A prototype for a pointer to FCICreate's file read callback.
 */
typedef CAB_CB_FCI_FILE_READ *PCAB_CB_FCI_FILE_READ;

/**
 A prototype for FCICreate's file write callback.
 */
typedef
DWORD DIAMONDAPI
CAB_CB_FCI_FILE_WRITE(DWORD_PTR, LPVOID, DWORD, PINT, PVOID);

/**
 A prototype for a pointer to FCICreate's file write callback.
 */
typedef CAB_CB_FCI_FILE_WRITE *PCAB_CB_FCI_FILE_WRITE;

/**
 A prototype for FCICreate's file close callback.
 */
typedef
INT DIAMONDAPI
CAB_CB_FCI_FILE_CLOSE(DWORD_PTR, PINT, PVOID);

/**
 A prototype for a pointer to FCICreate's file close callback.
 */
typedef CAB_CB_FCI_FILE_CLOSE *PCAB_CB_FCI_FILE_CLOSE;

/**
 A prototype for FCICreate's file seek callback.
 */
typedef
DWORD DIAMONDAPI
CAB_CB_FCI_FILE_SEEK(DWORD_PTR, DWORD, INT, PINT, PVOID);

/**
 A prototype for a pointer to FCICreate's file seek callback.
 */
typedef CAB_CB_FCI_FILE_SEEK *PCAB_CB_FCI_FILE_SEEK;

/**
 A prototype for FCICreate's file delete callback.
 */
typedef
DWORD DIAMONDAPI
CAB_CB_FCI_FILE_DELETE(LPSTR, PINT, PVOID);

/**
 A prototype for a pointer to FCICreate's file delete callback.
 */
typedef CAB_CB_FCI_FILE_DELETE *PCAB_CB_FCI_FILE_DELETE;

/**
 A prototype for FCICreate's temporary file callback.
 */
typedef
BOOL DIAMONDAPI
CAB_CB_FCI_GET_TEMP_FILE(LPSTR, INT, PVOID);

/**
 A prototype for a pointer to FCICreate's temporary file callback.
 */
typedef CAB_CB_FCI_GET_TEMP_FILE *PCAB_CB_FCI_GET_TEMP_FILE;

/**
 A prototype for FCIAddFile's get next cabinet callback.
 */
typedef
BOOL DIAMONDAPI
CAB_CB_FCI_GET_NEXT_CABINET(PCAB_FCI_CONTEXT, DWORD, PVOID);

/**
 A prototype for a pointer to FCIAddFile's get next cabinet callback.
 */
typedef CAB_CB_FCI_GET_NEXT_CABINET *PCAB_CB_FCI_GET_NEXT_CABINET;

/**
 A prototype for FCIAddFile's status callback.
 */
typedef
DWORD DIAMONDAPI
CAB_CB_FCI_STATUS(DWORD, DWORD, DWORD, PVOID);

/**
 A prototype for a pointer to FCIAddFile's status callback.
 */
typedef CAB_CB_FCI_STATUS *PCAB_CB_FCI_STATUS;

/**
 A prototype for FCIAddFile's get open info callback.
 */
typedef
DWORD_PTR DIAMONDAPI
CAB_CB_FCI_GET_OPEN_INFO(LPSTR, PWORD, PWORD, PWORD, PINT, PVOID);

/**
 A prototype for a pointer to FCIAddFile's status callback.
 */
typedef CAB_CB_FCI_GET_OPEN_INFO *PCAB_CB_FCI_GET_OPEN_INFO;

/**
 A structure describing error conditions encountered in FCI or FDI operations.
 */
typedef struct _CAB_CB_ERROR {
    /**
     FCI/FDI error code.
     */
    INT ErrorCode;

    /**
     Probably C run time error code of whatever C runtime that Cabinet.dll
     happens to use.
     */
    INT ErrorType;

    /**
     Set to TRUE if an error occurred.
     */
    BOOL ErrorPresent;
} CAB_CB_ERROR, *PCAB_CB_ERROR;


#ifndef UNPROTECTED_DACL_SECURITY_INFORMATION
/**
 A private definition of this OS value in case the compilation environment
 doesn't define it.
 */
#define UNPROTECTED_DACL_SECURITY_INFORMATION (0x20000000)
#endif

#ifndef SE_FILE_OBJECT
/**
 A private definition of this OS value in case the compilation environment
 doesn't define it.
 */
#define SE_FILE_OBJECT 1
#endif

#ifndef SE_ERR_SHARE
/**
 A sharing violation occurred.
 */
#define SE_ERR_SHARE                    26
#endif

#ifndef SE_ERR_ASSOCINCOMPLETE
/**
 The file name association is incomplete.
 */
#define SE_ERR_ASSOCINCOMPLETE          27
#endif

#ifndef SE_ERR_DDETIMEOUT
/**
 A DDE timeout error occurred.
 */
#define SE_ERR_DDETIMEOUT               28
#endif

#ifndef SE_ERR_DDEFAIL
/**
 The DDE transaction failed.
 */
#define SE_ERR_DDEFAIL                  29
#endif

#ifndef SE_ERR_DDEBUSY
/**
 The DDE server is busy.
 */
#define SE_ERR_DDEBUSY                  30
#endif

#ifndef SE_ERR_NOASSOC
/**
 There is no application associated with the file.
 */
#define SE_ERR_NOASSOC                  31
#endif

/**
 Command to the shell to delete an object.
 */
#define YORI_SHFILEOP_DELETE              0x003

/**
 Flag to the shell to avoid UI.
 */
#define YORI_SHFILEOP_FLAG_SILENT         0x004

/**
 Flag to the shell to suppress confirmation.
 */
#define YORI_SHFILEOP_FLAG_NOCONFIRMATION 0x010

/**
 Flag to the shell to place objects in the recycle bin.
 */
#define YORI_SHFILEOP_FLAG_ALLOWUNDO      0x040

/**
 Flag to the shell to suppress errors.
 */
#define YORI_SHFILEOP_FLAG_NOERRORUI      0x400

/**
 Shell defined structure describing a file operation.
 */
typedef struct _YORI_SHFILEOP {

    /**
     hWnd to use for UI, which we don't have an don't want.
     */
    HWND hWndIgnored;

    /**
     The function requested from the shell.
     */
    UINT Function;

    /**
     A NULL terminated list of NULL terminated strings of files to operate
     on.
     */
    LPCTSTR Source;

    /**
     Another NULL terminated list of NULL terminated strings, which is not
     used in this program.
     */
    LPCTSTR Dest;

    /**
     Flags for the operation.
     */
    DWORD Flags;

    /**
     Set to TRUE if the operation was cancelled.
     */
    BOOL Aborted;

    /**
     Shell voodoo.  No seriously, who comes up with this stuff?
     */
    PVOID NameMappings;

    /**
     A title that would be used by some types of UI, but not other types of
     UI, which we don't have and don't want.
     */
    LPCTSTR ProgressTitle;
} YORI_SHFILEOP, *PYORI_SHFILEOP;

#ifndef CSIDL_APPDATA
/**
 The identifier of the AppData directory to ShGetSpecialFolderPath.
 */
#define CSIDL_APPDATA 0x001a
#endif

#ifndef CSIDL_COMMON_APPDATA
/**
 The identifier of the common AppData directory to ShGetSpecialFolderPath.
 */
#define CSIDL_COMMON_APPDATA 0x0023
#endif

#ifndef CSIDL_COMMON_DESKTOPDIRECTORY
/**
 The identifier of the common Desktop directory to ShGetSpecialFolderPath.
 */
#define CSIDL_COMMON_DESKTOPDIRECTORY 0x0019
#endif

#ifndef CSIDL_COMMON_DOCUMENTS
/**
 The identifier of the common Documents directory to ShGetSpecialFolderPath.
 */
#define CSIDL_COMMON_DOCUMENTS 0x002e
#endif

#ifndef CSIDL_COMMON_PROGRAMS
/**
 The identifier of the common Programs directory to ShGetSpecialFolderPath.
 */
#define CSIDL_COMMON_PROGRAMS 0x0017
#endif

#ifndef CSIDL_COMMON_STARTMENU
/**
 The identifier of the common Start Menu directory to ShGetSpecialFolderPath.
 */
#define CSIDL_COMMON_STARTMENU 0x0016
#endif

#ifndef CSIDL_DESKTOPDIRECTORY
/**
 The identifier of the Desktop directory to ShGetSpecialFolderPath.
 */
#define CSIDL_DESKTOPDIRECTORY 0x0010
#endif

#ifndef CSIDL_LOCALAPPDATA
/**
 The identifier of the AppData local directory to ShGetSpecialFolderPath.
 */
#define CSIDL_LOCALAPPDATA 0x001c
#endif

#ifndef CSIDL_PERSONAL
/**
 The identifier of the Documents directory to ShGetSpecialFolderPath.
 */
#define CSIDL_PERSONAL 0x0005
#endif

#ifndef CSIDL_PROGRAM_FILES
/**
 The identifier of the Program Files directory to ShGetSpecialFolderPath.
 */
#define CSIDL_PROGRAM_FILES 0x0026
#endif

#ifndef CSIDL_PROGRAM_FILESX86
/**
 The identifier of the Program Files (x86) directory to ShGetSpecialFolderPath.
 */
#define CSIDL_PROGRAM_FILESX86 0x002a
#endif

#ifndef CSIDL_PROGRAMS
/**
 The identifier of the Start Menu Programs directory to ShGetSpecialFolderPath.
 */
#define CSIDL_PROGRAMS 0x0002
#endif

#ifndef CSIDL_STARTMENU
/**
 The identifier of the Start Menu directory to ShGetSpecialFolderPath.
 */
#define CSIDL_STARTMENU 0x000b
#endif

#ifndef CSIDL_STARTUP
/**
 The identifier of the Start Menu startup directory to ShGetSpecialFolderPath.
 */
#define CSIDL_STARTUP 0x0007
#endif

#ifndef CSIDL_SYSTEM
/**
 The identifier of the System32 directory to ShGetSpecialFolderPath.
 */
#define CSIDL_SYSTEM 0x0025
#endif

#ifndef CSIDL_WINDOWS
/**
 The identifier of the System32 directory to ShGetSpecialFolderPath.
 */
#define CSIDL_WINDOWS 0x0024
#endif

extern const GUID FOLDERID_Downloads;

/**
 The definition of SHELLEXECUTEINFO so we can use it without a
 compile time dependency on NT4+, or bringing in all of the shell headers.
 */
typedef struct _YORI_SHELLEXECUTEINFO {
    /**
     The number of bytes in this structure.
     */
    DWORD cbSize;

    /**
     The features we're using.
     */
    DWORD fMask;

    /**
     Our window handle, if we had one.
     */
    HWND hWnd;

    /**
     A shell verb, if we knew what to use it for.
     */
    LPCTSTR lpVerb;

    /**
     The program we're trying to launch.
     */
    LPCTSTR lpFile;

    /**
     The arguments supplied to the program.
     */
    LPCTSTR lpParameters;

    /**
     The initial directory for the program, if we didn't want to use the
     current directory that the user gave us.
     */
    LPCTSTR lpDirectory;

    /**
     The form to display the process child window in, if it is a GUI process.
     */
    int nShow;

    /**
     Mislabelled error code, carried forward from 16 bit land for no apparent
     reason since this function doesn't exist there.
     */
    HINSTANCE hInstApp;

    /**
     Some shell crap.
     */
    LPVOID lpIDList;

    /**
     More shell crap.
     */
    LPCTSTR lpClass;

    /**
     Is there an end to the shell crap?
     */
    HKEY hKeyClass;

    /**
     A hotkey? We just launched the thing. I'd understand associating a hotkey
     with something that might get launched, but associating it after we
     launched it? What on earth could possibly come from this?
     */
    DWORD dwHotKey;

    /**
     Not something of interest to command prompts.
     */
    HANDLE hIcon;

    /**
     Something we really care about! If process launch succeeds, this gets
     populated with a process handle, which we can wait on, and that's kind
     of desirable.
     */
    HANDLE hProcess;
} YORI_SHELLEXECUTEINFO, *PYORI_SHELLEXECUTEINFO;

#ifndef SEE_MASK_NOCLOSEPROCESS
/**
 Return the process handle where possible.
 */
#define SEE_MASK_NOCLOSEPROCESS (0x00000040)
#endif

#ifndef SEE_MASK_FLAG_NO_UI
/**
 Don't display UI in the context of our command prompt.
 */
#define SEE_MASK_FLAG_NO_UI     (0x00000400)
#endif

#ifndef SEE_MASK_UNICODE
/**
 We're supplying Unicode parameters.
 */
#define SEE_MASK_UNICODE        (0x00004000)
#endif

#ifndef SEE_MASK_NO_CONSOLE
/**
 The process should be launched on the existing console if possible.
 */
#define SEE_MASK_NO_CONSOLE     (0x00008000)
#endif

#ifndef SEE_MASK_NOZONECHECKS
/**
 No, just no.  Yes, it came from the Internet, just like everything else.
 The user shouldn't see random complaints because that fairly obvious fact
 was tracked by IE vs. when it wasn't.
 */
#define SEE_MASK_NOZONECHECKS   (0x00800000)
#endif

/**
 A structure to pass to SHBrowseForFolder.
 */
typedef struct _YORI_BROWSEINFO {

    /**
     Parent HWND for the child dialog.
     */
    HWND hWndOwner;

    /**
     A PIDL for the root of the tree.  We currently only use NULL so this
     doesn't need to be defined accurately.
     */
    PVOID PidlRoot;

    /**
     On successful completion, updated with the display name of the selected
     object.
     */
    LPTSTR DisplayName;

    /**
     A string to display at the top of the dialog.
     */
    LPCTSTR Title;

    /**
     Flags controlling the behavior of the dialog.
     */
    UINT Flags;

    /**
     A callback function to be invoked on certain events, not currently
     used.
     */
    PVOID CallbackFn;

    /**
     lParam to pass to the dialog on initialization.
     */
    LPARAM lParam;

    /**
     On successful completion, updated to contain the index of the object's icon.
     */
    INT ImageIndex;
} YORI_BROWSEINFO, *PYORI_BROWSEINFO;

#ifndef STDMETHODCALLTYPE

/**
 Define the Windows standard calling convention if it hasn't been defined
 already.
 */
#define STDMETHODCALLTYPE __stdcall
#endif

/**
 Standard COM QueryInterface method.
 */
typedef HRESULT STDMETHODCALLTYPE IUnknown_QueryInterface (PVOID This, const GUID * riid, LPVOID * ppvObj);

/**
 Standard COM AddRef method.
 */
typedef ULONG STDMETHODCALLTYPE IUnknown_AddRef (PVOID This);

/**
 Standard COM Release method.
 */
typedef ULONG STDMETHODCALLTYPE IUnknown_Release (PVOID This);

/**
 The in process type identifier when instantiating objects.
 */
#define CLSCTX_INPROC_SERVER 0x1

/**
 The IPersistFile interface, composed of a pointer to a set of functions.
 */
typedef struct IPersistFile {

    /**
     The function pointer table associated with this object.
     */
     struct IPersistFileVtbl *Vtbl;
} IPersistFile;

/**
 Indicates the GUID of the class implementing the functionality.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_GetClassID (IPersistFile * This, GUID *pClassID);

/**
 Indicates whether the object has been modified since it was last written
 to disk.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_IsDirty (IPersistFile * This);

/**
 Load the object from disk.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_Load (IPersistFile * This, LPCWSTR pszFileName, DWORD dwMode);

/**
 Save the object to disk.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_Save (IPersistFile * This, LPCWSTR pszFileName, BOOL fRemember);

/**
 Indicate that a save has completed and the object can be modified again.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_SaveCompleted (IPersistFile * This, LPCWSTR pszFileName);

/**
 Get the current file name associated with the object.
 */
typedef HRESULT STDMETHODCALLTYPE IPersistFile_GetCurFile (IPersistFile * This, LPCWSTR *ppszFileName);


/**
 A set of functions defined by the IPersistFile interface.
 */
typedef struct IPersistFileVtbl {

    /**
     Standard COM QueryInterface method.
     */
    IUnknown_QueryInterface * QueryInterface;

    /**
     Standard COM AddRef method.
     */
    IUnknown_AddRef * AddRef;

    /**
     Standard COM Release method.
     */
    IUnknown_Release * Release;

    /**
     Indicates the GUID of the class implementing the functionality.
     */
    IPersistFile_GetClassID * GetClassID;

    /**
     Indicates whether the object has been modified since it was last written
     to disk.
     */
    IPersistFile_IsDirty * IsDirty;

    /**
     Load the object from disk.
     */
    IPersistFile_Load * Load;

    /**
     Save the object to disk.
     */
    IPersistFile_Save * Save;

    /**
     Indicate that a save has completed and the object can be modified again.
     */
    IPersistFile_SaveCompleted * SaveCompleted;

    /**
     Get the current file name associated with the object.
     */
    IPersistFile_GetCurFile * GetCurFile;

} IPersistFileVtbl;

/**
 An instance of the IShellLink interface, consisting only of function pointers.
 */
typedef struct IShellLinkW {

    /**
     The function pointer table associated with this object.
     */
    struct IShellLinkWVtbl * Vtbl;
} IShellLinkW;

typedef struct IShellLinkWVtbl IShellLinkWVtbl;

/**
 Get the path to the target on a shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetPath (IShellLinkW * This, LPWSTR pszFile, int cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags);

/**
 Get a PIDL associated with a shortcut.  Used for shortcuts to objects
 other than files, and not used by this application.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetIDList (IShellLinkW * This, PVOID ppidl);

/**
 Set a PIDL to associate with a shortcut.  Used for shortcuts to objects
 other than files, and not used by this application.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetIDList (IShellLinkW * This, PVOID pidl);

/**
 Get the description associated with the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetDescription (IShellLinkW * This, LPWSTR pszName, int cchMaxName);

/**
 Set a description to be associated with the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetDescription (IShellLinkW * This, LPCWSTR pszName);

/**
 Get the directory to make current when launching from the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetWorkingDirectory (IShellLinkW * This, LPWSTR pszDir, int cchMaxPath);

/**
 Set the directory to make current when launching from the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetWorkingDirectory (IShellLinkW * This, LPCWSTR pszDir);

/**
 Get the arguments (parameters) to pass to the object pointed to by the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetArguments (IShellLinkW * This, LPWSTR pszArgs, int cchMaxPath);

/**
 Set the arguments (parameters) to pass to the object pointed to by the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetArguments (IShellLinkW * This, LPCWSTR pszArgs);

/**
 Get the hotkey associated with the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetHotkey (IShellLinkW * This, WORD *pwHotkey);

/**
 Set the hotkey associated with the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetHotkey (IShellLinkW * This, WORD wHotkey);

/**
 Get the window display state to use when launching from the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetShowCmd (IShellLinkW * This, int *piShowCmd);

/**
 Set the window display state to use when launching from the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetShowCmd (IShellLinkW * This, int iShowCmd);

/**
 Get the location of the icon to use in the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_GetIconLocation (IShellLinkW * This, LPWSTR pszIconPath, int cchIconPath, int *piIcon);

/**
 Set the location of the icon to use in the shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetIconLocation (IShellLinkW * This, LPCWSTR pszIconPath, int iIcon);

/**
 Set a relative path on a shortcut.  Not used by this application.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetRelativePath (IShellLinkW * This, LPCWSTR pszPathRel, DWORD dwReserved);

/**
 Resolve a shell link (useful if the target has moved.)  Not used by this application.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_Resolve (IShellLinkW * This, HWND hwnd, DWORD fFlags);

/**
 Set the path to the target in a shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLink_SetPath (IShellLinkW * This, LPCWSTR pszFile);

/**
 A set of functions defined by the IShellLink interface.
 */
struct IShellLinkWVtbl {

    /**
     Standard COM QueryInterface method.
     */
    IUnknown_QueryInterface * QueryInterface;

    /**
     Standard COM AddRef method.
     */
    IUnknown_AddRef * AddRef;

    /**
     Standard COM Release method.
     */
    IUnknown_Release * Release;


    /**
     Get the path to the target on a shortcut.
     */
    IShellLink_GetPath * GetPath;

    /**
     Get a PIDL associated with a shortcut.  Used for shortcuts to objects
     other than files, and not used by this application.
     */
    IShellLink_GetIDList * GetIDList;

    /**
     Set a PIDL to associate with a shortcut.  Used for shortcuts to objects
     other than files, and not used by this application.
     */
    IShellLink_SetIDList * SetIDList;

    /**
     Get the description associated with the shortcut.
     */
    IShellLink_GetDescription * GetDescription;

    /**
     Set a description to be associated with the shortcut.
     */
    IShellLink_SetDescription * SetDescription;

    /**
     Get the directory to make current when launching from the shortcut.
     */
    IShellLink_GetWorkingDirectory * GetWorkingDirectory;

    /**
     Set the directory to make current when launching from the shortcut.
     */
    IShellLink_SetWorkingDirectory * SetWorkingDirectory;

    /**
     Get the arguments (parameters) to pass to the object pointed to by the shortcut.
     */
    IShellLink_GetArguments * GetArguments;

    /**
     Set the arguments (parameters) to pass to the object pointed to by the shortcut.
     */
    IShellLink_SetArguments * SetArguments;

    /**
     Get the hotkey associated with the shortcut.
     */
    IShellLink_GetHotkey * GetHotkey;

    /**
     Set the hotkey associated with the shortcut.
     */
    IShellLink_SetHotkey * SetHotkey;

    /**
     Get the window display state to use when launching from the shortcut.
     */
    IShellLink_GetShowCmd * GetShowCmd;

    /**
     Set the window display state to use when launching from the shortcut.
     */
    IShellLink_SetShowCmd * SetShowCmd;

    /**
     Get the location of the icon to use in the shortcut.
     */
    IShellLink_GetIconLocation * GetIconLocation;

    /**
     Set the location of the icon to use in the shortcut.
     */
    IShellLink_SetIconLocation * SetIconLocation;

    /**
     Set a relative path on a shortcut.  Not used by this application.
     */
    IShellLink_SetRelativePath * SetRelativePath;

    /**
     Resolve a shell link (useful if the target has moved.)  Not used by this application.
     */
    IShellLink_Resolve * Resolve;

    /**
     Set the path to the target in a shortcut.
     */
    IShellLink_SetPath * SetPath;
};

/**
 An instance of the IShellLinkDataList interface, consisting only of function
 pointers.
 */
typedef struct IShellLinkDataList {

    /**
     The function pointer table associated with this object.
     */
    struct IShellLinkDataListVtbl * Vtbl;
} IShellLinkDataList;

typedef struct IShellLinkDataListVtbl IShellLinkDataListVtbl;

/**
 Add a block of data to a shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLinkDataList_AddDataBlock (IShellLinkDataList * This, LPVOID DataBlock);

/**
 Read (aka copy out) a block of data from a shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLinkDataList_CopyDataBlock (IShellLinkDataList * This, DWORD Signature, LPVOID * DataBlock);

/**
 Remove a block of data from a shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLinkDataList_RemoveDataBlock (IShellLinkDataList * This, DWORD Signature);

/**
 Get the flags from a shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLinkDataList_GetFlags (IShellLinkDataList * This, PDWORD Flags);

/**
 Set the flags from a shortcut.
 */
typedef HRESULT STDMETHODCALLTYPE IShellLinkDataList_SetFlags (IShellLinkDataList * This, DWORD Flags);

/**
 A set of functions defined by the IShellLinkDataList interface.
 */
struct IShellLinkDataListVtbl {

    /**
     Standard COM QueryInterface method.
     */
    IUnknown_QueryInterface * QueryInterface;

    /**
     Standard COM AddRef method.
     */
    IUnknown_AddRef * AddRef;

    /**
     Standard COM Release method.
     */
    IUnknown_Release * Release;


    /**
     Add a block of data to the shortcut.
     */
    IShellLinkDataList_AddDataBlock * AddDataBlock;

    /**
     Read (aka copy out) a block of data from the shortcut.
     */
    IShellLinkDataList_CopyDataBlock * CopyDataBlock;

    /**
     Remove a block of data from the shortcut.
     */
    IShellLinkDataList_RemoveDataBlock * RemoveDataBlock;

    /**
     Get the flags from the shortcut.
     */
    IShellLinkDataList_GetFlags * GetFlags;

    /**
     Set the flags on a shortcut.
     */
    IShellLinkDataList_SetFlags * SetFlags;
};

/**
 A structure defining extra information which can be attached to a
 shortcut to configure console state.
 */
typedef struct _ISHELLLINKDATALIST_CONSOLE_PROPS {

    /**
     The size of this structure.
     */
    DWORD dwSize;

    /**
     The signature for this type of structure.
     */
    DWORD dwSignature;

    /**
     The default window color, in Win32 format.
     */
    WORD  WindowColor;

    /**
     The popup color, in Win32 format.
     */
    WORD  PopupColor;

    /**
     The dimensions of the screen buffer.
     */
    COORD ScreenBufferSize;

    /**
     The dimensions of the window.
     */
    COORD WindowSize;

    /**
     The position of the window on the screen.  Not meaningful if
     AutoPosition is TRUE.
     */
    COORD WindowPosition;

    /**
     The number of the font.
     */
    DWORD FontNumber;

    /**
     No idea.  Seriously.
     */
    DWORD InputBufferSize;

    /**
     The size of each cell, not really a classic font size.
     */
    COORD FontSize;

    /**
     The font family.
     */
    DWORD FontFamily;

    /**
     Font weight, 400 = Normal, 700 = Bold, etc.
     */
    DWORD FontWeight;

    /**
     The name of the font.
     */
    WCHAR FaceName[LF_FACESIZE];

    /**
     The size of the cursor, in percent.
     */
    DWORD CursorSize;

    /**
     TRUE if the console should open in a full screen, FALSE if it should
     open in a window.
     */
    BOOL  FullScreen;

    /**
     TRUE if QuickEdit should be enabled, FALSE if it should be disabled.
     */
    BOOL  QuickEdit;

    /**
     TRUE if Insert should be enabled, FALSE if it should be disabled.
     */
    BOOL  InsertMode;

    /**
     TRUE if the window should be automatically positioned.  If FALSE, the
     position is specified in WindowPosition above.
     */
    BOOL  AutoPosition;

    /**
     The number of lines in each history buffer.
     */
    DWORD HistoryBufferSize;

    /**
     The number of history buffers.
     */
    DWORD NumberOfHistoryBuffers;

    /**
     If TRUE, repeated identical commands should be removed from history.
     If FALSE, identical items are recorded.
     */
    BOOL  RemoveHistoryDuplicates;

    /**
     A table of Win32 colors to their RGB representation.
     */
    COLORREF ColorTable[16];
} ISHELLLINKDATALIST_CONSOLE_PROPS, *PISHELLLINKDATALIST_CONSOLE_PROPS;

/**
 The signature for console properties within a ShellLinkDataList.
 */
#define ISHELLLINKDATALIST_CONSOLE_PROPS_SIG (0xA0000002)

#ifndef _VIRTUAL_STORAGE_TYPE_DEFINED
/**
 A structure describing the underlying storage provider when accessing
 virtual disks.
 */
typedef struct _VIRTUAL_STORAGE_TYPE {

    /**
     The type of the storage provider.
     */
    DWORD DeviceId;

    /**
     A GUID describing the vendor of the storage provider.
     */
    GUID  VendorId;
} VIRTUAL_STORAGE_TYPE, *PVIRTUAL_STORAGE_TYPE;
#endif

/**
 An unknown storage provider.
 */
extern const GUID VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

/**
 The only known storage provider to this application, Microsoft.
 */
extern const GUID VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;

/**
 An unknown storage provider.
 */
#define VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN                (0x0)

/**
 The ISO image storage provider.
 */
#define VIRTUAL_STORAGE_TYPE_DEVICE_ISO                    (0x1)

/**
 The VHD image storage provider.
 */
#define VIRTUAL_STORAGE_TYPE_DEVICE_VHD                    (0x2)

/**
 The VHDX image storage provider.
 */
#define VIRTUAL_STORAGE_TYPE_DEVICE_VHDX                   (0x3)

/**
 The VHD set storage provider.
 */
#define VIRTUAL_STORAGE_TYPE_DEVICE_VHDSET                 (0x4)

/**
 The normal recursion depth when mounting a VHD for read write.  If a VHD
 is mounted from physical storage, the depth should be one.  If a VHD is
 mounted from within another VHD, the depth should be two.
 */
#define OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT                 (0x1)

/**
 Version one of the parameters when opening a virtual disk.
 */
#define OPEN_VIRTUAL_DISK_VERSION_1                        (0x1)

/**
 Version two of the parameters when opening a virtual disk.
 */
#define OPEN_VIRTUAL_DISK_VERSION_2                        (0x2)

/**
 Parameters to pass when opening a virtual disk.
 */
typedef struct _OPEN_VIRTUAL_DISK_PARAMETERS {

    /**
     The version of the parameters included below.
     */
    DWORD Version;

    union {
        struct {
            DWORD RWDepth;
        } Version1;
        struct {
            BOOL GetInfoOnly;
            BOOL ReadOnly;
            GUID ResiliencyGuid;
        } Version2;
    };
} OPEN_VIRTUAL_DISK_PARAMETERS, *POPEN_VIRTUAL_DISK_PARAMETERS;

/**
 No access is needed to the virtual disk.
 */
#define VIRTUAL_DISK_ACCESS_NONE                           (0x00000000)

/**
 The opened virtual disk needs to be capable of attaching in read only mode.
 */
#define VIRTUAL_DISK_ACCESS_ATTACH_RO                      (0x00010000)

/**
 The opened virtual disk needs to be capable of attaching in read write mode.
 */
#define VIRTUAL_DISK_ACCESS_ATTACH_RW                      (0x00020000)

/**
 The opened virtual disk needs to be capable of detaching.
 */
#define VIRTUAL_DISK_ACCESS_DETACH                         (0x00040000)

/**
 The opened virtual disk needs to be capable of querying information.
 */
#define VIRTUAL_DISK_ACCESS_GET_INFO                       (0x00080000)

/**
 The opened virtual disk needs to be capable of being created.
 */
#define VIRTUAL_DISK_ACCESS_CREATE                         (0x00100000)

/**
 The opened virtual disk needs to be capable of having metadata modifications
 performed.
 */
#define VIRTUAL_DISK_ACCESS_METAOPS                        (0x00200000)

/**
 The opened virtual disk including all access needed for read only operations.
 */
#define VIRTUAL_DISK_ACCESS_READ                           (0x000d0000)


/**
 Open the virtual disk with no special options.
 */
#define OPEN_VIRTUAL_DISK_FLAG_NONE                        (0x00000000)

/**
 Skip any verification and validation of a virtual disk, since it may be an
 uninitialized file.
 */
#define OPEN_VIRTUAL_DISK_FLAG_BLANK_FILE                  (0x00000002)

/**
 Use cached IO when working on the virtual disk.  Note that files within
 virtual disks are cached normally; this applies global caching to the disk
 file itself, which is typically redundant and/or dangerous.
 */
#define OPEN_VIRTUAL_DISK_FLAG_CACHED_IO                   (0x00000008)

/**
 Version one of the parameters when attaching a virtual disk.
 */
#define ATTACH_VIRTUAL_DISK_VERSION_1                      (0x1)

/**
 Parameters to pass when attaching a virtual disk.
 */
typedef struct _ATTACH_VIRTUAL_DISK_PARAMETERS {

    /**
     Specifies the version of the attach parameters.  Currently this
     structure is only defined with support for 1.
     */
    DWORD Version;

    /**
     32 bits of padding because version 1 of the structure expects it.
     */
    DWORD Reserved;
} ATTACH_VIRTUAL_DISK_PARAMETERS, *PATTACH_VIRTUAL_DISK_PARAMETERS;


/**
 Perform a normal attach.
 */
#define ATTACH_VIRTUAL_DISK_FLAG_NONE                      (0x00000000)

/**
 Attach the virtual disk as read only.
 */
#define ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY                 (0x00000001)

/**
 Don't assign drive letter(s) for the volumes within the virtual disk.
 */
#define ATTACH_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER           (0x00000002)

/**
 Keep the attachment active after the handle to the virtual disk is closed.
 */
#define ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME        (0x00000004)

/**
 According to MSDN, use the default security descriptor, although the API
 also allows a NULL SD to be passed which sounds similar.
 */
#define ATTACH_VIRTUAL_DISK_FLAG_NO_SECURITY_DESCRIPTOR    (0x00000010)


/**
 If this flag is set, the file is created as fully allocated.  If it is not
 set, the file allocation is created as needed.
 */
#define CREATE_VIRTUAL_DISK_FLAG_FULL_PHYSICAL_ALLOCATION  (0x00000001)

/**
 Information about how to create a VHD.
 */
typedef struct _CREATE_VIRTUAL_DISK_PARAMETERS {

    /**
     The version of this structure.  This program currently only uses version
     1.
     */
    DWORD Version;
    union {
        struct {

            /**
             A unique identifier.  Set to zero to let the system determine it.
             */
            GUID UniqueId;

            /**
             The size of the VHD, in bytes.
             */
            DWORDLONG MaximumSize;

            /**
             For a differencing VHD, the size of the block to copy to the child
             when a modification occurs.  Must be zero for a non-differencing
             VHD.
             */
            DWORD BlockSizeInBytes;

            /**
             The sector size of the VHD.  VHD (version 1) only supports 512
             bytes.
             */
            DWORD SectorSizeInBytes;

            /**
             Path to the parent VHD file when creating a differencing VHD.
             */
            PCWSTR ParentPath;

            /**
             Path to a device to provide data for the newly created VHD.
             */
            PCWSTR SourcePath;
         } Version1;

        struct {

            /**
             A unique identifier.  Set to zero to let the system determine it.
             */
            GUID UniqueId;

            /**
             The size of the VHD, in bytes.
             */
            DWORDLONG MaximumSize;

            /**
             For a differencing VHD, the size of the block to copy to the child
             when a modification occurs.  Must be zero for a non-differencing
             VHD.
             */
            DWORD BlockSizeInBytes;

            /**
             The sector size of the VHD.  VHD (version 1) only supports 512
             bytes.
             */
            DWORD SectorSizeInBytes;

            /**
             The physical sector size of the VHD.
             */
            DWORD PhysicalSectorSizeInBytes;

            /**
             The open flags to apply to the handle.
             */
            DWORD OpenFlags;

            /**
             Path to the parent VHD file when creating a differencing VHD.
             */
            PCWSTR ParentPath;

            /**
             Path to a device to provide data for the newly created VHD.
             */
            PCWSTR SourcePath;

            /**
             The type of the parent.
             */
            DWORD ParentVirtualStorageType;

            /**
             The type of the source.
             */
            DWORD SourceVirtualStorageType;

            GUID ResiliencyGuid;
         } Version2;
    };
} CREATE_VIRTUAL_DISK_PARAMETERS, *PCREATE_VIRTUAL_DISK_PARAMETERS;

/**
 Information about how to compact a VHD.
 */
typedef struct _COMPACT_VIRTUAL_DISK_PARAMETERS {

    /**
     The version of this structure.
     */
    DWORD Version;
    union {
        struct {

            /**
             The new size for the VHD in bytes.
             */
            DWORD Unused;
        } Version1;
    };
} COMPACT_VIRTUAL_DISK_PARAMETERS, *PCOMPACT_VIRTUAL_DISK_PARAMETERS;

/**
 Information about how to expand a VHD.
 */
typedef struct _EXPAND_VIRTUAL_DISK_PARAMETERS {

    /**
     The version of this structure.
     */
    DWORD Version;
    union {
        struct {

            /**
             The new size for the VHD in bytes.
             */
            DWORDLONG NewSizeInBytes;
        } Version1;
    };
} EXPAND_VIRTUAL_DISK_PARAMETERS, *PEXPAND_VIRTUAL_DISK_PARAMETERS;

/**
 Information about how to resize a VHD.
 */
typedef struct _MERGE_VIRTUAL_DISK_PARAMETERS {

    /**
     The version of this structure.
     */
    DWORD Version;
    union {
        struct {

            /**
             The number of levels in the chain to merge.
             */
            DWORD DepthToMerge;
        } Version1;
    };
} MERGE_VIRTUAL_DISK_PARAMETERS, *PMERGE_VIRTUAL_DISK_PARAMETERS;

/**
 Information about how to resize a VHD.
 */
typedef struct _RESIZE_VIRTUAL_DISK_PARAMETERS {

    /**
     The version of this structure.
     */
    DWORD Version;
    union {
        struct {

            /**
             The new size for the VHD in bytes.
             */
            DWORDLONG NewSizeInBytes;
        } Version1;
    };
} RESIZE_VIRTUAL_DISK_PARAMETERS, *PRESIZE_VIRTUAL_DISK_PARAMETERS;

/** 
 A pseudo handle indicating the current terminal server server.
 */
#define WTS_CURRENT_SERVER_HANDLE NULL

/**
 An identifier for the current terminal server session.
 */
#define WTS_CURRENT_SESSION ((DWORD)-1)

/**
 A prototype for the NtOpenDirectoryObject function.
 */
typedef
LONG WINAPI
NT_OPEN_DIRECTORY_OBJECT(PHANDLE, DWORD, PYORI_OBJECT_ATTRIBUTES);

/**
 A prototype for a pointer to the NtOpenDirectoryObject function.
 */
typedef NT_OPEN_DIRECTORY_OBJECT *PNT_OPEN_DIRECTORY_OBJECT;

/**
 A prototype for the NtOpenSymbolicLinkObject function.
 */
typedef
LONG WINAPI
NT_OPEN_SYMBOLIC_LINK_OBJECT(PHANDLE, DWORD, PYORI_OBJECT_ATTRIBUTES);

/**
 A prototype for a pointer to the NtOpenSymbolicLinkObject function.
 */
typedef NT_OPEN_SYMBOLIC_LINK_OBJECT *PNT_OPEN_SYMBOLIC_LINK_OBJECT;

/**
 A prototype for the NtQueryDirectoryObject function.
 */
typedef
LONG WINAPI
NT_QUERY_DIRECTORY_OBJECT(HANDLE, PVOID, DWORD, BOOLEAN, BOOLEAN, PDWORD, PDWORD);

/**
 A prototype for a pointer to the NtQueryDirectoryObject function.
 */
typedef NT_QUERY_DIRECTORY_OBJECT *PNT_QUERY_DIRECTORY_OBJECT;

/**
 A prototype for the NtQueryInformationFile function.
 */
typedef
LONG WINAPI
NT_QUERY_INFORMATION_FILE(HANDLE, PIO_STATUS_BLOCK, PVOID, DWORD, DWORD);

/**
 A prototype for a pointer to the NtQueryInformationFile function.
 */
typedef NT_QUERY_INFORMATION_FILE *PNT_QUERY_INFORMATION_FILE;

/**
 A prototype for the NtQueryInformationProcess function..
 */
typedef
LONG WINAPI
NT_QUERY_INFORMATION_PROCESS(HANDLE, DWORD, PVOID, DWORD, PDWORD);

/**
 A prototype for a pointer to the NtQueryInformationProcess function.
 */
typedef NT_QUERY_INFORMATION_PROCESS *PNT_QUERY_INFORMATION_PROCESS;

/**
 A prototype for the NtQueryInformationThread function.
 */
typedef
LONG WINAPI
NT_QUERY_INFORMATION_THREAD(HANDLE, DWORD, PVOID, DWORD, PDWORD);

/**
 A prototype for a pointer to the NtQueryInformationThread function.
 */
typedef NT_QUERY_INFORMATION_THREAD *PNT_QUERY_INFORMATION_THREAD;

/**
 A prototype for the NtQueryObject function.
 */
typedef
LONG WINAPI
NT_QUERY_OBJECT(HANDLE, DWORD, PVOID, DWORD, PDWORD);

/**
 A prototype for a pointer to the NtQueryObject function.
 */
typedef NT_QUERY_OBJECT *PNT_QUERY_OBJECT;

/**
 A prototype for the NtQuerySymbolicLinkObject function.
 */
typedef
LONG WINAPI
NT_QUERY_SYMBOLIC_LINK_OBJECT(HANDLE, PYORI_UNICODE_STRING, PDWORD);

/**
 A prototype for a pointer to the NtQuerySymbolicLinkObject function.
 */
typedef NT_QUERY_SYMBOLIC_LINK_OBJECT *PNT_QUERY_SYMBOLIC_LINK_OBJECT;

/**
 A prototype for the NtQuerySystemInformation function.
 */
typedef
LONG WINAPI
NT_QUERY_SYSTEM_INFORMATION(DWORD, PVOID, DWORD, PDWORD);

/**
 A prototype for a pointer to the NtQuerySystemInformation function.
 */
typedef NT_QUERY_SYSTEM_INFORMATION *PNT_QUERY_SYSTEM_INFORMATION;

/**
 A prototype for the NtSetInformationFile function.
 */
typedef
LONG WINAPI
NT_SET_INFORMATION_FILE(HANDLE, PIO_STATUS_BLOCK, PVOID, DWORD, DWORD);

/**
 A prototype for a pointer to the NtSetInformationFile function.
 */
typedef NT_SET_INFORMATION_FILE *PNT_SET_INFORMATION_FILE;

/**
 A prototype for the NtSystemDebugControl function.
 */
typedef
LONG WINAPI
NT_SYSTEM_DEBUG_CONTROL(DWORD, PVOID, DWORD, PVOID, DWORD, PDWORD);

/**
 A prototype for a pointer to the NtSystemDebugControl function.
 */
typedef NT_SYSTEM_DEBUG_CONTROL *PNT_SYSTEM_DEBUG_CONTROL;

/**
 A prototype for the RtlGetLastNtStatus function.
 */
typedef
LONG WINAPI
RTL_GET_LAST_NT_STATUS(VOID);

/**
 A prototype for a pointer to the RtlGetLastNtStatus function.
 */
typedef RTL_GET_LAST_NT_STATUS *PRTL_GET_LAST_NT_STATUS;

/**
 A structure containing optional function pointers to ntdll.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_NTDLL_FUNCTIONS {

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to
     NtOpenDirectoryObject.
     */
    PNT_OPEN_DIRECTORY_OBJECT pNtOpenDirectoryObject;

    /**
     If it's available on the current system, a pointer to
     NtOpenSymbolicLinkObject.
     */
    PNT_OPEN_SYMBOLIC_LINK_OBJECT pNtOpenSymbolicLinkObject;

    /**
     If it's available on the current system, a pointer to
     NtQueryDirectoryObject.
     */
    PNT_QUERY_DIRECTORY_OBJECT pNtQueryDirectoryObject;

    /**
     If it's available on the current system, a pointer to
     NtQueryInformationFile.
     */
    PNT_QUERY_INFORMATION_FILE pNtQueryInformationFile;

    /**
     If it's available on the current system, a pointer to
     NtQueryInformationProcess.
     */
    PNT_QUERY_INFORMATION_PROCESS pNtQueryInformationProcess;

    /**
     If it's available on the current system, a pointer to
     NtQueryInformationThread.
     */
    PNT_QUERY_INFORMATION_THREAD pNtQueryInformationThread;

    /**
     If it's available on the current system, a pointer to
     NtQueryObject.
     */
    PNT_QUERY_OBJECT pNtQueryObject;

    /**
     If it's available on the current system, a pointer to
     NtQuerySymbolicLinkObject.
     */
    PNT_QUERY_SYMBOLIC_LINK_OBJECT pNtQuerySymbolicLinkObject;

    /**
     If it's available on the current system, a pointer to
     NtQuerySystemInformation.
     */
    PNT_QUERY_SYSTEM_INFORMATION pNtQuerySystemInformation;

    /**
     If it's available on the current system, a pointer to
     NtSetInformationFile.
     */
    PNT_SET_INFORMATION_FILE pNtSetInformationFile;

    /**
     If it's available on the current system, a pointer to
     NtSystemDebugControl.
     */
    PNT_SYSTEM_DEBUG_CONTROL pNtSystemDebugControl;

    /**
     If it's available on the current system, a pointer to
     RtlGetLastNtStatus.
     */
    PRTL_GET_LAST_NT_STATUS pRtlGetLastNtStatus;

} YORI_NTDLL_FUNCTIONS, *PYORI_NTDLL_FUNCTIONS;

extern YORI_NTDLL_FUNCTIONS DllNtDll;

/**
 A prototype for the AddConsoleAliasW function.
 */
typedef
BOOL WINAPI
ADD_CONSOLE_ALIASW(LPCTSTR, LPCTSTR, LPCTSTR);

/**
 A prototype for a pointer to the AddConsoleAliasW function.
 */
typedef ADD_CONSOLE_ALIASW *PADD_CONSOLE_ALIASW;

/**
 A prototype for the AssignProcessToJobObject function.
 */
typedef
BOOL WINAPI
ASSIGN_PROCESS_TO_JOB_OBJECT(HANDLE, HANDLE);

/**
 A prototype for a pointer to the AssignProcessToJobObject function.
 */
typedef ASSIGN_PROCESS_TO_JOB_OBJECT *PASSIGN_PROCESS_TO_JOB_OBJECT;

/**
 A prototype for the CopyFileExW function.
 */
typedef
BOOL
WINAPI
COPY_FILE_EXW(LPCWSTR, LPCWSTR, PVOID, PVOID, LPBOOL, DWORD);

/**
 A prototype for a pointer to the CopyFileExW function.
 */
typedef COPY_FILE_EXW *PCOPY_FILE_EXW;

/**
 A prototype for the CopyFileW function.
 */
typedef
BOOL
WINAPI
COPY_FILEW(LPCWSTR, LPCWSTR, BOOL);

/**
 A prototype for a pointer to the CopyFileW function.
 */
typedef COPY_FILEW *PCOPY_FILEW;

/**
 A prototype for the CreateHardLinkW function.
 */
typedef
BOOL
WINAPI
CREATE_HARD_LINKW(LPWSTR, LPWSTR, PVOID);

/**
 A prototype for a pointer to the CreateHardLinkW function.
 */
typedef CREATE_HARD_LINKW *PCREATE_HARD_LINKW;

/**
 A prototype for the CreateJobObjectW function.
 */
typedef
HANDLE WINAPI
CREATE_JOB_OBJECTW(LPSECURITY_ATTRIBUTES, LPCWSTR);

/**
 A prototype for a pointer to the CreateJobObjectW function.
 */
typedef CREATE_JOB_OBJECTW *PCREATE_JOB_OBJECTW;

/**
 A prototype for the CreateSymbolicLinkW function.
 */
typedef
BOOLEAN WINAPI
CREATE_SYMBOLIC_LINKW(LPTSTR, LPTSTR, DWORD);

/**
 A prototype for a pointer to the CreateSymbolicLinkW function.
 */
typedef CREATE_SYMBOLIC_LINKW *PCREATE_SYMBOLIC_LINKW;

/**
 A prototype for the FindFirstStreamW function.
 */
typedef
HANDLE WINAPI
FIND_FIRST_STREAMW(LPCWSTR, DWORD, PWIN32_FIND_STREAM_DATA, DWORD);

/**
 A prototype for a pointer to the FindFirstStreamW function.
 */
typedef FIND_FIRST_STREAMW *PFIND_FIRST_STREAMW;

/**
 A prototype for the FindFirstVolumeW function.
 */
typedef
HANDLE WINAPI
FIND_FIRST_VOLUMEW(LPWSTR, DWORD);

/**
 A prototype for a pointer to the FindFirstVolumeW function.
 */
typedef FIND_FIRST_VOLUMEW *PFIND_FIRST_VOLUMEW;

/**
 A prototype for the FindNextStreamW function.
 */
typedef
BOOL WINAPI
FIND_NEXT_STREAMW(HANDLE, PWIN32_FIND_STREAM_DATA);

/**
 A prototype for a pointer to the FindNextStreamW function.
 */
typedef FIND_NEXT_STREAMW *PFIND_NEXT_STREAMW;

/**
 A prototype for the FindNextVolumeW function.
 */
typedef
BOOL WINAPI
FIND_NEXT_VOLUMEW(HANDLE, LPWSTR, DWORD);

/**
 A prototype for a pointer to the FindNextVolumeW function.
 */
typedef FIND_NEXT_VOLUMEW *PFIND_NEXT_VOLUMEW;

/**
 A prototype for the FindVolumeClose function.
 */
typedef
BOOL WINAPI
FIND_VOLUME_CLOSE(HANDLE);

/**
 A prototype for a pointer to the FindVolumeClose function.
 */
typedef FIND_VOLUME_CLOSE *PFIND_VOLUME_CLOSE;

/**
 A prototype for the FreeEnvironmentStringsW function.
 */
typedef
BOOL WINAPI
FREE_ENVIRONMENT_STRINGSW(LPWSTR);

/**
 Prototype for a pointer to the FreeEnvironmentStringsW function.
 */
typedef FREE_ENVIRONMENT_STRINGSW *PFREE_ENVIRONMENT_STRINGSW;

/**
 A prototype for the GetCompressedFileSizeW function.
 */
typedef
DWORD WINAPI
GET_COMPRESSED_FILE_SIZEW(LPCWSTR, LPDWORD);

/**
 A prototype for a pointer to the GetCompressedFileSizeW function.
 */
typedef GET_COMPRESSED_FILE_SIZEW *PGET_COMPRESSED_FILE_SIZEW;

/**
 A prototype for the GetConsoleAliasesLengthW function.
 */
typedef
DWORD WINAPI
GET_CONSOLE_ALIASES_LENGTHW(LPTSTR);

/**
 A prototype for a pointer to the GetConsoleAliasesLengthW function.
 */
typedef GET_CONSOLE_ALIASES_LENGTHW *PGET_CONSOLE_ALIASES_LENGTHW;

/**
 A prototype for the GetConsoleAliasesW function.
 */
typedef
DWORD WINAPI
GET_CONSOLE_ALIASESW(LPTSTR, DWORD, LPTSTR);

/**
 A prototype for a pointer to the GetConsoleAliasesW function.
 */
typedef GET_CONSOLE_ALIASESW *PGET_CONSOLE_ALIASESW;

/**
 A prototype for the GetConsoleDisplayMode function.
 */
typedef
BOOL WINAPI
GET_CONSOLE_DISPLAY_MODE(LPDWORD);

/**
 A prototype for a pointer to the GetConsoleDisplayMode function.
 */
typedef GET_CONSOLE_DISPLAY_MODE *PGET_CONSOLE_DISPLAY_MODE;

/**
 A prototype for the GetConsoleProcessList function.
 */
typedef
DWORD WINAPI
GET_CONSOLE_PROCESS_LIST(LPDWORD, DWORD);

/**
 A prototype for a pointer to the GetConsoleProcessList function.
 */
typedef GET_CONSOLE_PROCESS_LIST *PGET_CONSOLE_PROCESS_LIST;

/**
 A prototype for the GetConsoleScreenBufferEx function.
 */
typedef
BOOL WINAPI
GET_CONSOLE_SCREEN_BUFFER_INFO_EX(HANDLE, PYORI_CONSOLE_SCREEN_BUFFER_INFOEX);

/**
 A prototype for a pointer to the GetConsoleScreenBufferEx function.
 */
typedef GET_CONSOLE_SCREEN_BUFFER_INFO_EX *PGET_CONSOLE_SCREEN_BUFFER_INFO_EX;

/**
 A prototype for the GetConsoleWindow function.
 */
typedef
HWND WINAPI
GET_CONSOLE_WINDOW(VOID);

/**
 A prototype for a pointer to the GetConsoleWindow function.
 */
typedef GET_CONSOLE_WINDOW *PGET_CONSOLE_WINDOW;

/**
 A prototype for the GetCurrentConsoleFontEx function.
 */
typedef
BOOL WINAPI
GET_CURRENT_CONSOLE_FONT_EX(HANDLE, BOOL, PYORI_CONSOLE_FONT_INFOEX);

/**
 A prototype for a pointer to the GetCurrentConsoleFontEx function.
 */
typedef GET_CURRENT_CONSOLE_FONT_EX *PGET_CURRENT_CONSOLE_FONT_EX;

/**
 A prototype for the GetDiskFreeSpaceExW function.
 */
typedef
BOOL WINAPI
GET_DISK_FREE_SPACE_EXW(LPCWSTR, PLARGE_INTEGER, PLARGE_INTEGER, PLARGE_INTEGER);

/**
 A prototype for a pointer to the GetDiskFreeSpaceExW function.
 */
typedef GET_DISK_FREE_SPACE_EXW *PGET_DISK_FREE_SPACE_EXW;

/**
 A prototype for the GetEnvironmentStrings function.
 */
typedef
LPSTR WINAPI
GET_ENVIRONMENT_STRINGS(VOID);

/**
 A prototype for a pointer to the GetEnvironmentStrings function.
 */
typedef GET_ENVIRONMENT_STRINGS *PGET_ENVIRONMENT_STRINGS;

/**
 A prototype for the GetEnvironmentStringsW function.
 */
typedef
LPWSTR WINAPI
GET_ENVIRONMENT_STRINGSW(VOID);

/**
 A prototype for a pointer to the GetEnvironmentStringsW function.
 */
typedef GET_ENVIRONMENT_STRINGSW *PGET_ENVIRONMENT_STRINGSW;

/**
 A prototype for the GetFileInformationByHandleEx function.
 */
typedef
BOOL WINAPI
GET_FILE_INFORMATION_BY_HANDLE_EX(HANDLE, DWORD, PVOID, DWORD);

/**
 A prototype for a pointer to the GetFileInformationByHandleEx function.
 */
typedef GET_FILE_INFORMATION_BY_HANDLE_EX *PGET_FILE_INFORMATION_BY_HANDLE_EX;

/**
 A prototype for the GetFinalPathNameByHandleW function.
 */
typedef
BOOL WINAPI
GET_FINAL_PATH_NAME_BY_HANDLEW(HANDLE, LPCWSTR, DWORD, DWORD);

/**
 Prototype for a pointer to the GetFinalPathNameByHandleW function.
 */
typedef GET_FINAL_PATH_NAME_BY_HANDLEW *PGET_FINAL_PATH_NAME_BY_HANDLEW;

/**
 A prototype for the GetLogicalProcessorInformation function.
 */
typedef
BOOL WINAPI
GET_LOGICAL_PROCESSOR_INFORMATION(PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

/**
 A prototype for a pointer to the GetLogicalProcessorInformation function.
 */
typedef GET_LOGICAL_PROCESSOR_INFORMATION *PGET_LOGICAL_PROCESSOR_INFORMATION;

/**
 A prototype for the GetLogicalProcessorInformationEx function.
 */
typedef
BOOL WINAPI
GET_LOGICAL_PROCESSOR_INFORMATION_EX(YORI_LOGICAL_PROCESSOR_RELATIONSHIP, PYORI_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);

/**
 A prototype for a pointer to the GetLogicalProcessorInformationEx function.
 */
typedef GET_LOGICAL_PROCESSOR_INFORMATION_EX *PGET_LOGICAL_PROCESSOR_INFORMATION_EX;

/**
 A prototype for the GetNativeSystemInfo function.
 */
typedef
BOOL WINAPI
GET_NATIVE_SYSTEM_INFO(PVOID);

/**
 A prototype for a pointer to the GetNativeSystemInfo function.
 */
typedef GET_NATIVE_SYSTEM_INFO *PGET_NATIVE_SYSTEM_INFO;

/**
 A prototype for the GetPrivateProfileIntW function.
 */
typedef
DWORD WINAPI
GET_PRIVATE_PROFILE_INTW(LPCWSTR, LPCWSTR, INT, LPCWSTR);

/**
 A prototype for a pointer to the GetPrivateProfileIntW function.
 */
typedef GET_PRIVATE_PROFILE_INTW *PGET_PRIVATE_PROFILE_INTW;

/**
 A prototype for the GetPrivateProfileSectionW function.
 */
typedef
DWORD WINAPI
GET_PRIVATE_PROFILE_SECTIONW(LPCWSTR, LPWSTR, DWORD, LPCWSTR);

/**
 A prototype for a pointer to the GetPrivateProfileSectionW function.
 */
typedef GET_PRIVATE_PROFILE_SECTIONW *PGET_PRIVATE_PROFILE_SECTIONW;

/**
 A prototype for the GetPrivateProfileSectionNamesW function.
 */
typedef
DWORD WINAPI
GET_PRIVATE_PROFILE_SECTION_NAMESW(LPWSTR, DWORD, LPCWSTR);

/**
 A prototype for a pointer to the GetPrivateProfileSectionNamesW function.
 */
typedef GET_PRIVATE_PROFILE_SECTION_NAMESW *PGET_PRIVATE_PROFILE_SECTION_NAMESW;

/**
 A prototype for the GetPrivateProfileStringW function.
 */
typedef
DWORD WINAPI
GET_PRIVATE_PROFILE_STRINGW(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);

/**
 A prototype for a pointer to the GetPrivateProfileStringW function.
 */
typedef GET_PRIVATE_PROFILE_STRINGW *PGET_PRIVATE_PROFILE_STRINGW;

/**
 A prototype for the GetProcessIoCounters function.
 */
typedef
BOOL WINAPI
GET_PROCESS_IO_COUNTERS(HANDLE, PYORI_IO_COUNTERS);

/**
 A prototype for a pointer to the GetProcessIoCounters function.
 */
typedef GET_PROCESS_IO_COUNTERS *PGET_PROCESS_IO_COUNTERS;

/**
 A prototype for the GetProductInfo function.
 */
typedef
BOOL WINAPI
GET_PRODUCT_INFO(DWORD, DWORD, DWORD, DWORD, PDWORD);

/**
 A prototype for a pointer to the GetProductInfo function.
 */
typedef GET_PRODUCT_INFO *PGET_PRODUCT_INFO;

/**
 A prototype for the GetSystemPowerStatus function.
 */
typedef
DWORDLONG WINAPI
GET_SYSTEM_POWER_STATUS(PYORI_SYSTEM_POWER_STATUS);

/**
 A prototype for a pointer to the GetSystemPowerStatus function.
 */
typedef GET_SYSTEM_POWER_STATUS *PGET_SYSTEM_POWER_STATUS;

/**
 A prototype for the GetTickCount64 function.
 */
typedef
DWORDLONG WINAPI
GET_TICK_COUNT_64(VOID);

/**
 A prototype for a pointer to the GetTickCount64 function.
 */
typedef GET_TICK_COUNT_64 *PGET_TICK_COUNT_64;

/**
 A prototype for the GetVersionExW function.
 */
typedef
BOOL WINAPI
GET_VERSION_EXW(PYORI_OS_VERSION_INFO);

/**
 A prototype for a pointer to the GetVersionExW function.
 */
typedef GET_VERSION_EXW *PGET_VERSION_EXW;

/**
 A prototype for the GetVolumePathNamesForVolumeNameW function.
 */
typedef
BOOL WINAPI
GET_VOLUME_PATH_NAMES_FOR_VOLUME_NAMEW(LPCWSTR, LPWSTR, DWORD, PDWORD);

/**
 A prototype for a pointer to the GetVolumePathNamesForVolumeNameW function.
 */
typedef GET_VOLUME_PATH_NAMES_FOR_VOLUME_NAMEW *PGET_VOLUME_PATH_NAMES_FOR_VOLUME_NAMEW;

/**
 A prototype for the GetVolumePathNameW function.
 */
typedef
BOOL WINAPI
GET_VOLUME_PATH_NAMEW(LPCWSTR, LPWSTR, DWORD);

/**
 A prototype for a pointer to the GetVolumePathNameW function.
 */
typedef GET_VOLUME_PATH_NAMEW *PGET_VOLUME_PATH_NAMEW;

/**
 A prototype for the GlobalLock function.
 */
typedef
LPVOID WINAPI
GLOBAL_LOCK(HGLOBAL);

/**
 A prototype for a pointer to the GlobalLock function.
 */
typedef GLOBAL_LOCK *PGLOBAL_LOCK;

/**
 A prototype for the GlobalMemoryStatus function.
 */
typedef
BOOL WINAPI
GLOBAL_MEMORY_STATUS(LPMEMORYSTATUS);

/**
 A prototype for a pointer to the GlobalMemoryStatus function.
 */
typedef GLOBAL_MEMORY_STATUS *PGLOBAL_MEMORY_STATUS;

/**
 A prototype for the GlobalMemoryStatusEx function.
 */
typedef
BOOL WINAPI
GLOBAL_MEMORY_STATUS_EX(PYORI_MEMORYSTATUSEX);

/**
 A prototype for a pointer to the GlobalMemoryStatusEx function.
 */
typedef GLOBAL_MEMORY_STATUS_EX *PGLOBAL_MEMORY_STATUS_EX;

/**
 A prototype for the GlobalSize function.
 */
typedef
SIZE_T WINAPI
GLOBAL_SIZE(HGLOBAL);

/**
 A prototype for a pointer to the GlobalSize function.
 */
typedef GLOBAL_SIZE *PGLOBAL_SIZE;

/**
 A prototype for the GlobalUnlock function.
 */
typedef
BOOL WINAPI
GLOBAL_UNLOCK(HGLOBAL);

/**
 A prototype for a pointer to the GlobalUnlock function.
 */
typedef GLOBAL_UNLOCK *PGLOBAL_UNLOCK;

/**
 A prototype for the InterlockedCompareExchange function.
 */
typedef
LONG WINAPI
INTERLOCKED_COMPARE_EXCHANGE(LONG volatile *, LONG, LONG);

/**
 A prototype for a pointer to the InterlockedCompareExchange function.
 */
typedef INTERLOCKED_COMPARE_EXCHANGE *PINTERLOCKED_COMPARE_EXCHANGE;

/**
 A prototype for the IsWow64Process function.
 */
typedef
BOOL WINAPI
IS_WOW64_PROCESS(HANDLE, PBOOL);

/**
 A prototype for a pointer to the IsWow64Process function.
 */
typedef IS_WOW64_PROCESS *PIS_WOW64_PROCESS;

/**
 A prototype for the IsWow64Process2 function.
 */
typedef
BOOL WINAPI
IS_WOW64_PROCESS2(HANDLE, PWORD, PWORD);

/**
 A prototype for a pointer to the IsWow64Process2 function.
 */
typedef IS_WOW64_PROCESS2 *PIS_WOW64_PROCESS2;

/**
 A prototype for the LoadLibraryW function.
 */
typedef
HINSTANCE WINAPI
LOAD_LIBRARYW(LPCWSTR);

/**
 A prototype for a pointer to the LoadLibraryW function.
 */
typedef LOAD_LIBRARYW *PLOAD_LIBRARYW;

/**
 A prototype for the LoadLibraryExW function.
 */
typedef
HINSTANCE WINAPI
LOAD_LIBRARY_EXW(LPCWSTR, HANDLE, DWORD);

/**
 A prototype for a pointer to the LoadLibraryExW function.
 */
typedef LOAD_LIBRARY_EXW *PLOAD_LIBRARY_EXW;

/**
 A prototype for the OpenThread function.
 */
typedef
HANDLE WINAPI
OPEN_THREAD(DWORD, BOOL, DWORD);

/**
 A prototype for a pointer to the OpenThread function.
 */
typedef OPEN_THREAD *POPEN_THREAD;

/**
 A prototype for the QueryFullProcessImageNameW function.
 */
typedef
BOOL WINAPI
QUERY_FULL_PROCESS_IMAGE_NAMEW(HANDLE, DWORD, LPTSTR, PDWORD);

/**
 A prototype for a pointer to the the QueryFullProcessImageNameW function.
 */
typedef QUERY_FULL_PROCESS_IMAGE_NAMEW *PQUERY_FULL_PROCESS_IMAGE_NAMEW;

/**
 A prototype for the QueryInformationJobObject function.
 */
typedef
BOOL WINAPI
QUERY_INFORMATION_JOB_OBJECT(HANDLE, DWORD, PVOID, DWORD, LPDWORD);

/**
 A prototype for a pointer to the QueryInformationJobObject function.
 */
typedef QUERY_INFORMATION_JOB_OBJECT *PQUERY_INFORMATION_JOB_OBJECT;

/**
 A prototype for the RegisterApplicationRestart function.
 */
typedef
LONG WINAPI
REGISTER_APPLICATION_RESTART(LPCWSTR, DWORD);

/**
 A prototype for a pointer to the RegisterApplicationRestart function.
 */
typedef REGISTER_APPLICATION_RESTART *PREGISTER_APPLICATION_RESTART;

/**
 A prototype for the ReplaceFileW function.
 */
typedef
BOOL WINAPI
REPLACE_FILEW(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, LPVOID, LPVOID);

/**
 A prototype for a pointer to the ReplaceFileW function.
 */
typedef REPLACE_FILEW *PREPLACE_FILEW;

/**
 A prototype for the RtlCaptureStackBackTrace function.
 */
typedef
WORD WINAPI
RTL_CAPTURE_STACK_BACK_TRACE(DWORD, DWORD, PVOID *, PDWORD);

/**
 A prototype for a pointer to the RtlCaptureStackBackTrace function.
 */
typedef RTL_CAPTURE_STACK_BACK_TRACE *PRTL_CAPTURE_STACK_BACK_TRACE;

/**
 A prototype for the SetConsoleDisplayMode function.
 */
typedef
BOOL WINAPI
SET_CONSOLE_DISPLAY_MODE(HANDLE, DWORD, PCOORD);

/**
 A prototype for a pointer to the the SetConsoleDisplayMode function.
 */
typedef SET_CONSOLE_DISPLAY_MODE *PSET_CONSOLE_DISPLAY_MODE;

/**
 A prototype for the SetConsoleScreenBufferEx function.
 */
typedef
BOOL WINAPI
SET_CONSOLE_SCREEN_BUFFER_INFO_EX(HANDLE, PYORI_CONSOLE_SCREEN_BUFFER_INFOEX);

/**
 A prototype for a pointer to the the SetConsoleScreenBufferEx function.
 */
typedef SET_CONSOLE_SCREEN_BUFFER_INFO_EX *PSET_CONSOLE_SCREEN_BUFFER_INFO_EX;

/**
 A prototype for the SetCurrentConsoleFontEx function.
 */
typedef
BOOL WINAPI
SET_CURRENT_CONSOLE_FONT_EX(HANDLE, BOOL, PYORI_CONSOLE_FONT_INFOEX);

/**
 A prototype for a pointer to the SetCurrentConsoleFontEx function.
 */
typedef SET_CURRENT_CONSOLE_FONT_EX *PSET_CURRENT_CONSOLE_FONT_EX;

/**
 A prototype for the SetFileInformationByHandle function.
 */
typedef
BOOL WINAPI
SET_FILE_INFORMATION_BY_HANDLE(HANDLE, DWORD, PVOID, DWORD);

/**
 A prototype for a pointer to the SetFileInformationByHandle function.
 */
typedef SET_FILE_INFORMATION_BY_HANDLE *PSET_FILE_INFORMATION_BY_HANDLE;

/**
 A prototype for the SetInformationJobObject function.
 */
typedef
BOOL WINAPI
SET_INFORMATION_JOB_OBJECT(HANDLE, DWORD, PVOID, DWORD);

/**
 A prototype for a pointer to the SetInformationJobObject function.
 */
typedef SET_INFORMATION_JOB_OBJECT *PSET_INFORMATION_JOB_OBJECT;

/**
 A prototype for the WritePrivateProfileStringW function.
 */
typedef
BOOL WINAPI
WRITE_PRIVATE_PROFILE_STRINGW(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR);

/**
 A prototype for a pointer to the WritePrivateProfileStringW function.
 */
typedef WRITE_PRIVATE_PROFILE_STRINGW *PWRITE_PRIVATE_PROFILE_STRINGW;

/**
 A prototype for the Wow64DisableWow64FsRedirection function.
 */
typedef
BOOL WINAPI
WOW64_DISABLE_WOW64_FS_REDIRECTION(PVOID*);

/**
 A prototype for a pointer to the Wow64DisableWow64FsRedirection function.
 */
typedef WOW64_DISABLE_WOW64_FS_REDIRECTION *PWOW64_DISABLE_WOW64_FS_REDIRECTION;

/**
 A prototype for the Wow64GetThreadContext function.
 */
typedef
BOOL WINAPI
WOW64_GET_THREAD_CONTEXT(HANDLE, PYORI_LIB_WOW64_CONTEXT);

/**
 A prototype for a pointer to the Wow64GetThreadContext function.
 */
typedef WOW64_GET_THREAD_CONTEXT *PWOW64_GET_THREAD_CONTEXT;

/**
 A prototype for the Wow64SetThreadContext function.
 */
typedef
BOOL WINAPI
WOW64_SET_THREAD_CONTEXT(HANDLE, PYORI_LIB_WOW64_CONTEXT);

/**
 A prototype for a pointer to the Wow64SetThreadContext function.
 */
typedef WOW64_SET_THREAD_CONTEXT *PWOW64_SET_THREAD_CONTEXT;

/**
 A structure containing optional function pointers to kernel32.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_KERNEL32_FUNCTIONS {

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDllKernelBase;

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDllKernel32;

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDllKernel32Legacy;

    /**
     If it's available on the current system, a pointer to AddConsoleAliasW.
     */
    PADD_CONSOLE_ALIASW pAddConsoleAliasW;

    /**
     If it's available on the current system, a pointer to AssignProcessToJobObject.
     */
    PASSIGN_PROCESS_TO_JOB_OBJECT pAssignProcessToJobObject;

    /**
     If it's available on the current system, a pointer to CopyFileExW.
     */
    PCOPY_FILE_EXW pCopyFileExW;

    /**
     If it's available on the current system, a pointer to CopyFileW.
     */
    PCOPY_FILEW pCopyFileW;

    /**
     If it's available on the current system, a pointer to CreateHardLinkW.
     */
    PCREATE_HARD_LINKW pCreateHardLinkW;

    /**
     If it's available on the current system, a pointer to CreateJobObjectW.
     */
    PCREATE_JOB_OBJECTW pCreateJobObjectW;

    /**
     If it's available on the current system, a pointer to CreateSymbolicLinkW.
     */
    PCREATE_SYMBOLIC_LINKW pCreateSymbolicLinkW;

    /**
     If it's available on the current system, a pointer to FindFirstStreamW.
     */
    PFIND_FIRST_STREAMW pFindFirstStreamW;

    /**
     If it's available on the current system, a pointer to FindFirstVolumeW.
     */
    PFIND_FIRST_VOLUMEW pFindFirstVolumeW;

    /**
     If it's available on the current system, a pointer to FindNextStreamW.
     */
    PFIND_NEXT_STREAMW pFindNextStreamW;

    /**
     If it's available on the current system, a pointer to FindNextVolumeW.
     */
    PFIND_NEXT_VOLUMEW pFindNextVolumeW;

    /**
     If it's available on the current system, a pointer to FindVolumeClose.
     */
    PFIND_VOLUME_CLOSE pFindVolumeClose;

    /**
     If it's available on the current system, a pointer to FreeEnvironmentStringsW.
     */
    PFREE_ENVIRONMENT_STRINGSW pFreeEnvironmentStringsW;

    /**
     If it's available on the current system, a pointer to GetCompressedFileSizeW.
     */
    PGET_COMPRESSED_FILE_SIZEW pGetCompressedFileSizeW;

    /**
     If it's available on the current system, a pointer to GetConsoleScreenBufferInfoEx.
     */
    PGET_CONSOLE_SCREEN_BUFFER_INFO_EX pGetConsoleScreenBufferInfoEx;

    /**
     If it's available on the current system, a pointer to GetConsoleAliasesLengthW.
     */
    PGET_CONSOLE_ALIASES_LENGTHW pGetConsoleAliasesLengthW;

    /**
     If it's available on the current system, a pointer to GetConsoleAliasesW.
     */
    PGET_CONSOLE_ALIASESW pGetConsoleAliasesW;

    /**
     If it's available on the current system, a pointer to GetConsoleDisplayMode.
     */
    PGET_CONSOLE_DISPLAY_MODE pGetConsoleDisplayMode;

    /**
     If it's available on the current system, a pointer to GetConsoleProcessList.
     */
    PGET_CONSOLE_PROCESS_LIST pGetConsoleProcessList;

    /**
     If it's available on the current system, a pointer to GetConsoleWindow.
     */
    PGET_CONSOLE_WINDOW pGetConsoleWindow;

    /**
     If it's available on the current system, a pointer to GetCurrentConsoleFontEx.
     */
    PGET_CURRENT_CONSOLE_FONT_EX pGetCurrentConsoleFontEx;

    /**
     If it's available on the current system, a pointer to GetDiskFreeSpaceExW.
     */
    PGET_DISK_FREE_SPACE_EXW pGetDiskFreeSpaceExW;

    /**
     If it's available on the current system, a pointer to GetEnvironmentStrings.
     */
    PGET_ENVIRONMENT_STRINGS pGetEnvironmentStrings;

    /**
     If it's available on the current system, a pointer to GetEnvironmentStringsW.
     */
    PGET_ENVIRONMENT_STRINGSW pGetEnvironmentStringsW;

    /**
     If it's available on the current system, a pointer to GetFileInformationByHandleEx.
     */
    PGET_FILE_INFORMATION_BY_HANDLE_EX pGetFileInformationByHandleEx;

    /**
     If it's available on the current system, a pointer to GetFinalPathNameByHandleW.
     */
    PGET_FINAL_PATH_NAME_BY_HANDLEW pGetFinalPathNameByHandleW;

    /**
     If it's available on the current system, a pointer to GetLogicalProcessorInformation.
     */
    PGET_LOGICAL_PROCESSOR_INFORMATION pGetLogicalProcessorInformation;

    /**
     If it's available on the current system, a pointer to GetLogicalProcessorInformationEx.
     */
    PGET_LOGICAL_PROCESSOR_INFORMATION_EX pGetLogicalProcessorInformationEx;

    /**
     If it's available on the current system, a pointer to GetNativeSystemInfo.
     */
    PGET_NATIVE_SYSTEM_INFO pGetNativeSystemInfo;

    /**
     If it's available on the current system, a pointer to GetPrivateProfileIntW.
     */
    PGET_PRIVATE_PROFILE_INTW pGetPrivateProfileIntW;

    /**
     If it's available on the current system, a pointer to GetPrivateProfileSectionW.
     */
    PGET_PRIVATE_PROFILE_SECTIONW pGetPrivateProfileSectionW;

    /**
     If it's available on the current system, a pointer to GetPrivateProfileSectionNamesW.
     */
    PGET_PRIVATE_PROFILE_SECTION_NAMESW pGetPrivateProfileSectionNamesW;

    /**
     If it's available on the current system, a pointer to GetPrivateProfileStringW.
     */
    PGET_PRIVATE_PROFILE_STRINGW pGetPrivateProfileStringW;

    /**
     If it's available on the current system, a pointer to GetProcessIoCounters.
     */
    PGET_PROCESS_IO_COUNTERS pGetProcessIoCounters;

    /**
     If it's available on the current system, a pointer to GetProductInfo.
     */
    PGET_PRODUCT_INFO pGetProductInfo;

    /**
     If it's available on the current system, a pointer to GetSystemPowerStatus.
     */
    PGET_SYSTEM_POWER_STATUS pGetSystemPowerStatus;

    /**
     If it's available on the current system, a pointer to GetTickCount64.
     */
    PGET_TICK_COUNT_64 pGetTickCount64;

    /**
     If it's available on the current system, a pointer to GetVersionExW.
     */
    PGET_VERSION_EXW pGetVersionExW;

    /**
     If it's available on the current system, a pointer to GetVolumePathNamesForVolumeNameW.
     */
    PGET_VOLUME_PATH_NAMES_FOR_VOLUME_NAMEW pGetVolumePathNamesForVolumeNameW;

    /**
     If it's available on the current system, a pointer to GetVolumePathNameW.
     */
    PGET_VOLUME_PATH_NAMEW pGetVolumePathNameW;

    /**
     If it's available on the current system, a pointer to GlobalLock.
     */
    PGLOBAL_LOCK pGlobalLock;

    /**
     If it's available on the current system, a pointer to GlobalMemoryStatus.
     */
    PGLOBAL_MEMORY_STATUS pGlobalMemoryStatus;

    /**
     If it's available on the current system, a pointer to GlobalMemoryStatusEx.
     */
    PGLOBAL_MEMORY_STATUS_EX pGlobalMemoryStatusEx;

    /**
     If it's available on the current system, a pointer to GlobalSize.
     */
    PGLOBAL_SIZE pGlobalSize;

    /**
     If it's available on the current system, a pointer to GlobalUnlock.
     */
    PGLOBAL_UNLOCK pGlobalUnlock;

    /**
     If it's available on the current system, a pointer to InterlockedCompareExchange.
     */
    PINTERLOCKED_COMPARE_EXCHANGE pInterlockedCompareExchange;

    /**
     If it's available on the current system, a pointer to IsWow64Process.
     */
    PIS_WOW64_PROCESS pIsWow64Process;

    /**
     If it's available on the current system, a pointer to IsWow64Process2.
     */
    PIS_WOW64_PROCESS2 pIsWow64Process2;

    /**
     If it's available on the current system, a pointer to LoadLibraryW.
     */
    PLOAD_LIBRARYW pLoadLibraryW;

    /**
     If it's available on the current system, a pointer to LoadLibraryExW.
     */
    PLOAD_LIBRARY_EXW pLoadLibraryExW;

    /**
     If it's available on the current system, a pointer to OpenThread.
     */
    POPEN_THREAD pOpenThread;

    /**
     If it's available on the current system, a pointer to QueryFullProcessImageNameW.
     */
    PQUERY_FULL_PROCESS_IMAGE_NAMEW pQueryFullProcessImageNameW;

    /**
     If it's available on the current system, a pointer to QueryInformationJobObject.
     */
    PQUERY_INFORMATION_JOB_OBJECT pQueryInformationJobObject;

    /**
     If it's available on the current system, a pointer to RegisterApplicationRestart.
     */
    PREGISTER_APPLICATION_RESTART pRegisterApplicationRestart;

    /**
     If it's available on the current system, a pointer to ReplaceFileW.
     */
    PREPLACE_FILEW pReplaceFileW;

    /**
     If it's available on the current system, a pointer to RtlCaptureStackBackTrace.
     */
    PRTL_CAPTURE_STACK_BACK_TRACE pRtlCaptureStackBackTrace;

    /**
     If it's available on the current system, a pointer to SetConsoleDisplayMode.
     */
    PSET_CONSOLE_DISPLAY_MODE pSetConsoleDisplayMode;

    /**
     If it's available on the current system, a pointer to SetConsoleScreenBufferInfoEx.
     */
    PSET_CONSOLE_SCREEN_BUFFER_INFO_EX pSetConsoleScreenBufferInfoEx;

    /**
     If it's available on the current system, a pointer to SetCurrentConsoleFontEx.
     */
    PSET_CURRENT_CONSOLE_FONT_EX pSetCurrentConsoleFontEx;

    /**
     If it's available on the current system, a pointer to SetFileInformationByHandle.
     */
    PSET_FILE_INFORMATION_BY_HANDLE pSetFileInformationByHandle;

    /**
     If it's available on the current system, a pointer to SetInformationJobObject.
     */
    PSET_INFORMATION_JOB_OBJECT pSetInformationJobObject;

    /**
     If it's available on the current system, a pointer to WritePrivateProfileStringW.
     */
    PWRITE_PRIVATE_PROFILE_STRINGW pWritePrivateProfileStringW;

    /**
     If it's available on the current system, a pointer to Wow64DisableWow64FsRedirection.
     */
    PWOW64_DISABLE_WOW64_FS_REDIRECTION pWow64DisableWow64FsRedirection;

    /**
     If it's available on the current system, a pointer to Wow64GetThreadContext.
     */
    PWOW64_GET_THREAD_CONTEXT pWow64GetThreadContext;

    /**
     If it's available on the current system, a pointer to Wow64SetThreadContext.
     */
    PWOW64_SET_THREAD_CONTEXT pWow64SetThreadContext;
} YORI_KERNEL32_FUNCTIONS, *PYORI_KERNEL32_FUNCTIONS;

extern YORI_KERNEL32_FUNCTIONS DllKernel32;

/**
 A prototype for the AccessCheck function.
 */
typedef
BOOL WINAPI
ACCESS_CHECK(PSECURITY_DESCRIPTOR, HANDLE, DWORD, PGENERIC_MAPPING, PPRIVILEGE_SET, LPDWORD, LPDWORD, LPBOOL);

/**
 A prototype for a pointer to the AccessCheck function.
 */
typedef ACCESS_CHECK *PACCESS_CHECK;

/**
 A prototype for the AddAccessAllowedAce function.
 */
typedef
BOOL WINAPI
ADD_ACCESS_ALLOWED_ACE(PACL, DWORD, DWORD, PSID);

/**
 A prototype for a pointer to the AddAccessAllowedAce function.
 */
typedef ADD_ACCESS_ALLOWED_ACE *PADD_ACCESS_ALLOWED_ACE;

/**
 A prototype for the AdjustTokenPrivileges function.
 */
typedef
BOOL WINAPI
ADJUST_TOKEN_PRIVILEGES(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);

/**
 A prototype for a pointer to the AdjustTokenPrivileges function.
 */
typedef ADJUST_TOKEN_PRIVILEGES *PADJUST_TOKEN_PRIVILEGES;

/**
 Prototype for the AllocateAndInitializeSid function.
 */
typedef
BOOL WINAPI
ALLOCATE_AND_INITIALIZE_SID(PSID_IDENTIFIER_AUTHORITY, BYTE, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, PSID*);

/**
 Prototype for a pointer to the AllocateAndInitializeSid function.
 */
typedef ALLOCATE_AND_INITIALIZE_SID *PALLOCATE_AND_INITIALIZE_SID;

/**
 Prototype for the CheckTokenMembership function.
 */
typedef
BOOL WINAPI
CHECK_TOKEN_MEMBERSHIP(HANDLE, PSID, PBOOL);

/**
 Prototype for a pointer to the CheckTokenMembership function.
 */
typedef CHECK_TOKEN_MEMBERSHIP *PCHECK_TOKEN_MEMBERSHIP;

/**
 Prototype for the CryptAcquireContext function.
 */
typedef
BOOL WINAPI
CRYPT_ACQUIRE_CONTEXTW(PDWORD_PTR, LPCWSTR, LPCWSTR, DWORD, DWORD);

/**
 Prototype for a pointer to the CryptAcquireContext function.
 */
typedef CRYPT_ACQUIRE_CONTEXTW *PCRYPT_ACQUIRE_CONTEXTW;

/**
 Prototype for the CryptCreateHash function.
 */
typedef
BOOL WINAPI
CRYPT_CREATE_HASH(DWORD_PTR, DWORD, DWORD_PTR, DWORD, PDWORD_PTR);

/**
 Prototype for a pointer to the CryptCreateHash function.
 */
typedef CRYPT_CREATE_HASH *PCRYPT_CREATE_HASH;

/**
 Prototype for the CryptDestroyHash function.
 */
typedef
BOOL WINAPI
CRYPT_DESTROY_HASH(DWORD_PTR);

/**
 Prototype for a pointer to the CryptDestroyHash function.
 */
typedef CRYPT_DESTROY_HASH *PCRYPT_DESTROY_HASH;

/**
 Prototype for the CryptGetHashParam function.
 */
typedef
BOOL WINAPI
CRYPT_GET_HASH_PARAM(DWORD_PTR, DWORD, BYTE*, DWORD*, DWORD);

/**
 Prototype for a pointer to the CryptGetHashParam function.
 */
typedef CRYPT_GET_HASH_PARAM *PCRYPT_GET_HASH_PARAM;

/**
 Prototype for the CryptHashData function.
 */
typedef
BOOL WINAPI
CRYPT_HASH_DATA(DWORD_PTR, BYTE*, DWORD, DWORD);

/**
 Prototype for a pointer to the CryptHashData function.
 */
typedef CRYPT_HASH_DATA *PCRYPT_HASH_DATA;

/**
 Prototype for the CryptReleaseContext function.
 */
typedef
BOOL WINAPI
CRYPT_RELEASE_CONTEXT(DWORD_PTR, DWORD);

/**
 Prototype for a pointer to the CryptReleaseContext function.
 */
typedef CRYPT_RELEASE_CONTEXT *PCRYPT_RELEASE_CONTEXT;

/**
 Prototype for the FreeSid function.
 */
typedef
PVOID WINAPI
FREE_SID(PSID);

/**
 Prototype for a pointer to the FreeSid function.
 */
typedef FREE_SID *PFREE_SID;

/**
 A prototype for the GetFileSecurityW function.
 */
typedef
BOOL WINAPI
GET_FILE_SECURITYW(LPCWSTR, SECURITY_INFORMATION, PSECURITY_DESCRIPTOR, DWORD, LPDWORD);

/**
 Prototype for a pointer to the GetFileSecurityW function.
 */
typedef GET_FILE_SECURITYW *PGET_FILE_SECURITYW;

/**
 A prototype for the GetLengthSid function.
 */
typedef
DWORD WINAPI
GET_LENGTH_SID(PSID);

/**
 Prototype for a pointer to the GetLengthSid function.
 */
typedef GET_LENGTH_SID *PGET_LENGTH_SID;

/**
 A prototype for the GetSecurityDescriptorOwner function.
 */
typedef
BOOL WINAPI
GET_SECURITY_DESCRIPTOR_OWNER(PSECURITY_DESCRIPTOR, PSID, LPBOOL);

/**
 Prototype for a pointer to the GetSecurityDescriptorOwner function.
 */
typedef GET_SECURITY_DESCRIPTOR_OWNER *PGET_SECURITY_DESCRIPTOR_OWNER;

/**
 A prototype for the ImpersonateSelf function.
 */
typedef
BOOL WINAPI
IMPERSONATE_SELF(SECURITY_IMPERSONATION_LEVEL);

/**
 A prototype for a pointer to the ImpersonateSelf function.
 */
typedef IMPERSONATE_SELF *PIMPERSONATE_SELF;

/**
 A prototype for the InitializeAcl function.
 */
typedef
BOOL WINAPI
INITIALIZE_ACL(PACL, DWORD, DWORD);

/**
 A prototype for a pointer to the InitializeAcl function.
 */
typedef INITIALIZE_ACL *PINITIALIZE_ACL;

/**
 A prototype for the InitializeSecurityDescriptor function.
 */
typedef
BOOL WINAPI
INITIALIZE_SECURITY_DESCRIPTOR(PSECURITY_DESCRIPTOR, DWORD);

/**
 A prototype for a pointer to the InitializeSecurityDescriptor function.
 */
typedef INITIALIZE_SECURITY_DESCRIPTOR *PINITIALIZE_SECURITY_DESCRIPTOR;

/**
 A prototype for the InitiateShutdownW function.
 */
typedef
BOOL WINAPI
INITIATE_SHUTDOWNW(LPWSTR, LPWSTR, DWORD, DWORD, DWORD);

/**
 A prototype for a pointer to the InitiateShutdownW function.
 */
typedef INITIATE_SHUTDOWNW *PINITIATE_SHUTDOWNW;

/**
 A prototype for the LookupAccountNameW function.
 */
typedef
BOOL WINAPI
LOOKUP_ACCOUNT_NAMEW(LPCWSTR, LPCWSTR, PSID, LPDWORD, LPWSTR, LPDWORD, PSID_NAME_USE);

/**
 A prototype for a pointer to the LookupAccountNameW function.
 */
typedef LOOKUP_ACCOUNT_NAMEW *PLOOKUP_ACCOUNT_NAMEW;

/**
 A prototype for the LookupAccountSidW function.
 */
typedef
BOOL WINAPI
LOOKUP_ACCOUNT_SIDW(LPCWSTR, PSID, LPWSTR, LPDWORD, LPWSTR, LPDWORD, PSID_NAME_USE);

/**
 A prototype for a pointer to the LookupAccountSidW function.
 */
typedef LOOKUP_ACCOUNT_SIDW *PLOOKUP_ACCOUNT_SIDW;

/**
 A prototype for the LookupPrivilegeValueW function.
 */
typedef
BOOL WINAPI
LOOKUP_PRIVILEGE_VALUEW(LPCWSTR, LPCWSTR, PLUID);

/**
 A prototype for a pointer to the LookupPrivilegeValueW function.
 */
typedef LOOKUP_PRIVILEGE_VALUEW *PLOOKUP_PRIVILEGE_VALUEW;

/**
 A prototype for the OpenProcessToken function.
 */
typedef
BOOL WINAPI
OPEN_PROCESS_TOKEN(HANDLE, DWORD, PHANDLE);

/**
 A prototype for a pointer to the OpenProcessToken function.
 */
typedef OPEN_PROCESS_TOKEN *POPEN_PROCESS_TOKEN;

/**
 A prototype for the OpenThreadToken function.
 */
typedef
BOOL WINAPI
OPEN_THREAD_TOKEN(HANDLE, DWORD, BOOL, PHANDLE);

/**
 A prototype for a pointer to the OpenThreadToken function.
 */
typedef OPEN_THREAD_TOKEN *POPEN_THREAD_TOKEN;

/**
 A prototype for the RegCloseKey function.
 */
typedef
LONG WINAPI
REG_CLOSE_KEY(HANDLE);

/**
 A prototype for a pointer to the RegCloseKey function.
 */
typedef REG_CLOSE_KEY *PREG_CLOSE_KEY;

/**
 A prototype for the RegCreateKeyExW function.
 */
typedef
LONG WINAPI
REG_CREATE_KEY_EXW(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, DWORD, PVOID, PHKEY, LPDWORD);

/**
 A prototype for a pointer to the RegCreateKeyExW function.
 */
typedef REG_CREATE_KEY_EXW *PREG_CREATE_KEY_EXW;

/**
 A prototype for the RegDeleteKeyW function.
 */
typedef
LONG WINAPI
REG_DELETE_KEYW(HANDLE, LPCWSTR);

/**
 A prototype for a pointer to the RegDeleteKeyW function.
 */
typedef REG_DELETE_KEYW *PREG_DELETE_KEYW;

/**
 A prototype for the RegDeleteValueW function.
 */
typedef
LONG WINAPI
REG_DELETE_VALUEW(HANDLE, LPCWSTR);

/**
 A prototype for a pointer to the RegDeleteValueW function.
 */
typedef REG_DELETE_VALUEW *PREG_DELETE_VALUEW;

/**
 A prototype for the RegEnumKeyExW function.
 */
typedef
LONG WINAPI
REG_ENUM_KEY_EXW(HANDLE, DWORD, LPWSTR, LPDWORD, LPDWORD, LPWSTR, LPDWORD, PFILETIME);

/**
 A prototype for a pointer to the RegEnumKeyExW function.
 */
typedef REG_ENUM_KEY_EXW *PREG_ENUM_KEY_EXW;

/**
 A prototype for the RegEnumValueW function.
 */
typedef
LONG WINAPI
REG_ENUM_VALUEW(HANDLE, DWORD, LPWSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

/**
 A prototype for a pointer to the RegEnumValueW function.
 */
typedef REG_ENUM_VALUEW *PREG_ENUM_VALUEW;

/**
 A prototype for the RegGetKeySecurity function.
 */
typedef
LONG WINAPI
REG_GET_KEY_SECURITY(HKEY, SECURITY_INFORMATION, PSECURITY_DESCRIPTOR, LPDWORD);

/**
 A prototype for a pointer to the RegGetKeySecurity function.
 */
typedef REG_GET_KEY_SECURITY *PREG_GET_KEY_SECURITY;

/**
 A prototype for the RegOpenKeyExW function.
 */
typedef
LONG WINAPI
REG_OPEN_KEY_EXW(HKEY, LPCWSTR, DWORD, DWORD, PHKEY);

/**
 A prototype for a pointer to the RegOpenKeyExW function.
 */
typedef REG_OPEN_KEY_EXW *PREG_OPEN_KEY_EXW;

/**
 A prototype for the RegQueryInfoKeyW function.
 */
typedef
LONG WINAPI
REG_QUERY_INFO_KEYW(HANDLE, LPCWSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, PFILETIME);

/**
 A prototype for a pointer to the RegQueryInfoKeyW function.
 */
typedef REG_QUERY_INFO_KEYW *PREG_QUERY_INFO_KEYW;

/**
 A prototype for the RegQueryValueExW function.
 */
typedef
LONG WINAPI
REG_QUERY_VALUE_EXW(HANDLE, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

/**
 A prototype for a pointer to the RegQueryValueExW function.
 */
typedef REG_QUERY_VALUE_EXW *PREG_QUERY_VALUE_EXW;

/**
 A prototype for the RegSetKeySecurity function.
 */
typedef
LONG WINAPI
REG_SET_KEY_SECURITY(HKEY, SECURITY_INFORMATION, PSECURITY_DESCRIPTOR);

/**
 A prototype for a pointer to the RegSetKeySecurity function.
 */
typedef REG_SET_KEY_SECURITY *PREG_SET_KEY_SECURITY;

/**
 A prototype for the RegSetValueExW function.
 */
typedef
LONG WINAPI
REG_SET_VALUE_EXW(HANDLE, LPCWSTR, DWORD, DWORD, LPBYTE, DWORD);

/**
 A prototype for a pointer to the RegSetValueExW function.
 */
typedef REG_SET_VALUE_EXW *PREG_SET_VALUE_EXW;

/**
 A prototype for the RevertToSelf function.
 */
typedef
BOOL WINAPI
REVERT_TO_SELF(VOID);

/**
 A prototype for a pointer to the RevertToSelf function.
 */
typedef REVERT_TO_SELF *PREVERT_TO_SELF;

/**
 A prototype for the SetNamedSecurityInfoW function.
 */
typedef
DWORD WINAPI
SET_NAMED_SECURITY_INFOW(LPWSTR, DWORD, SECURITY_INFORMATION, PSID, PSID, PACL, PACL);

/**
 A prototype for a pointer to the SetNamedSecurityInfoW function.
 */
typedef SET_NAMED_SECURITY_INFOW *PSET_NAMED_SECURITY_INFOW;

/**
 A prototype for the SetSecurityDescriptorDacl function.
 */
typedef
BOOL WINAPI
SET_SECURITY_DESCRIPTOR_DACL(PSECURITY_DESCRIPTOR, BOOL, PACL, BOOL);

/**
 A prototype for a pointer to the SetSecurityDescriptorDacl function.
 */
typedef SET_SECURITY_DESCRIPTOR_DACL *PSET_SECURITY_DESCRIPTOR_DACL;

/**
 A prototype for the SetSecurityDescriptorOwner function.
 */
typedef
BOOL WINAPI
SET_SECURITY_DESCRIPTOR_OWNER(PSECURITY_DESCRIPTOR, PSID, BOOL);

/**
 A prototype for a pointer to the SetSecurityDescriptorOwner function.
 */
typedef SET_SECURITY_DESCRIPTOR_OWNER *PSET_SECURITY_DESCRIPTOR_OWNER;

/**
 A structure containing optional function pointers to advapi32.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_ADVAPI32_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     A handle to ntmarta.dll.  This is only used on Nano to resolve
     SetNamedSecurityInfoW, which is neither in advapi32 nor kernel32.
     */
    HINSTANCE hDllNtMarta;

    /**
     A handle to cryptsp.dll.  This is used on Nano to resolve Crypt*
     functions, which are neither in advapi32 nor kernel32.
     */
    HINSTANCE hDllCryptSp;

    /**
     If it's available on the current system, a pointer to AccessCheck.
     */
    PACCESS_CHECK pAccessCheck;

    /**
     If it's available on the current system, a pointer to AddAccessAllowedAce.
     */
    PADD_ACCESS_ALLOWED_ACE pAddAccessAllowedAce;

    /**
     If it's available on the current system, a pointer to AdjustTokenPrivileges.
     */
    PADJUST_TOKEN_PRIVILEGES pAdjustTokenPrivileges;

    /**
     If it's available on the current system, a pointer to AllocateAndInitializeSid.
     */
    PALLOCATE_AND_INITIALIZE_SID pAllocateAndInitializeSid;

    /**
     If it's available on the current system, a pointer to CheckTokenMembership.
     */
    PCHECK_TOKEN_MEMBERSHIP pCheckTokenMembership;

    /**
     If it's available on the current system, a pointer to CryptAcquireContextW.
     */
    PCRYPT_ACQUIRE_CONTEXTW pCryptAcquireContextW;

    /**
     If it's available on the current system, a pointer to CryptCreateHash.
     */
    PCRYPT_CREATE_HASH pCryptCreateHash;

    /**
     If it's available on the current system, a pointer to CryptDestroyHash.
     */
    PCRYPT_DESTROY_HASH pCryptDestroyHash;

    /**
     If it's available on the current system, a pointer to CryptGetHashParam.
     */
    PCRYPT_GET_HASH_PARAM pCryptGetHashParam;

    /**
     If it's available on the current system, a pointer to CryptHashData.
     */
    PCRYPT_HASH_DATA pCryptHashData;

    /**
     If it's available on the current system, a pointer to CryptReleaseContext.
     */
    PCRYPT_RELEASE_CONTEXT pCryptReleaseContext;

    /**
     If it's available on the current system, a pointer to FreeSid.
     */
    PFREE_SID pFreeSid;

    /**
     If it's available on the current system, a pointer to GetFileSecurityW.
     */
    PGET_FILE_SECURITYW pGetFileSecurityW;

    /**
     If it's available on the current system, a pointer to GetLengthSid.
     */
    PGET_LENGTH_SID pGetLengthSid;

    /**
     If it's available on the current system, a pointer to GetSecurityDescriptorOwner.
     */
    PGET_SECURITY_DESCRIPTOR_OWNER pGetSecurityDescriptorOwner;

    /**
     If it's available on the current system, a pointer to ImpersonateSelf.
     */
    PIMPERSONATE_SELF pImpersonateSelf;

    /**
     If it's available on the current system, a pointer to InitializeAcl.
     */
    PINITIALIZE_ACL pInitializeAcl;

    /**
     If it's available on the current system, a pointer to InitializeSecurityDescriptor.
     */
    PINITIALIZE_SECURITY_DESCRIPTOR pInitializeSecurityDescriptor;

    /**
     If it's available on the current system, a pointer to InitiateShutdownW.
     */
    PINITIATE_SHUTDOWNW pInitiateShutdownW;

    /**
     If it's available on the current system, a pointer to LookupAccountNameW.
     */
    PLOOKUP_ACCOUNT_NAMEW pLookupAccountNameW;

    /**
     If it's available on the current system, a pointer to LookupAccountSidW.
     */
    PLOOKUP_ACCOUNT_SIDW pLookupAccountSidW;

    /**
     If it's available on the current system, a pointer to LookupPrivilegeValueW.
     */
    PLOOKUP_PRIVILEGE_VALUEW pLookupPrivilegeValueW;

    /**
     If it's available on the current system, a pointer to OpenProcessToken.
     */
    POPEN_PROCESS_TOKEN pOpenProcessToken;

    /**
     If it's available on the current system, a pointer to OpenThreadToken.
     */
    POPEN_THREAD_TOKEN pOpenThreadToken;

    /**
     If it's available on the current system, a pointer to RegCloseKey.
     */
    PREG_CLOSE_KEY pRegCloseKey;

    /**
     If it's available on the current system, a pointer to RegCreateKeyExW.
     */
    PREG_CREATE_KEY_EXW pRegCreateKeyExW;

    /**
     If it's available on the current system, a pointer to RegDeleteKeyW.
     */
    PREG_DELETE_KEYW pRegDeleteKeyW;

    /**
     If it's available on the current system, a pointer to RegDeleteValueW.
     */
    PREG_DELETE_VALUEW pRegDeleteValueW;

    /**
     If it's available on the current system, a pointer to RegEnumKeyExW.
     */
    PREG_ENUM_KEY_EXW pRegEnumKeyExW;

    /**
     If it's available on the current system, a pointer to RegEnumValueW.
     */
    PREG_ENUM_VALUEW pRegEnumValueW;

    /**
     If it's available on the current system, a pointer to RegGetKeySecurity.
     */
    PREG_GET_KEY_SECURITY pRegGetKeySecurity;

    /**
     If it's available on the current system, a pointer to RegOpenKeyExW.
     */
    PREG_OPEN_KEY_EXW pRegOpenKeyExW;

    /**
     If it's available on the current system, a pointer to RegQueryInfoKeyW.
     */
    PREG_QUERY_INFO_KEYW pRegQueryInfoKeyW;

    /**
     If it's available on the current system, a pointer to RegQueryValueExW.
     */
    PREG_QUERY_VALUE_EXW pRegQueryValueExW;

    /**
     If it's available on the current system, a pointer to RegSetKeySecurity.
     */
    PREG_SET_KEY_SECURITY pRegSetKeySecurity;

    /**
     If it's available on the current system, a pointer to RegSetValueExW.
     */
    PREG_SET_VALUE_EXW pRegSetValueExW;

    /**
     If it's available on the current system, a pointer to RevertToSelf.
     */
    PREVERT_TO_SELF pRevertToSelf;

    /**
     If it's available on the current system, a pointer to SetNamedSecurityInfoW.
     */
    PSET_NAMED_SECURITY_INFOW pSetNamedSecurityInfoW;
    /**
     *
     If it's available on the current system, a pointer to
     SetSecurityDescriptorDacl.
     */
    PSET_SECURITY_DESCRIPTOR_DACL pSetSecurityDescriptorDacl;

    /**
     If it's available on the current system, a pointer to
     SetSecurityDescriptorOwner.
     */
    PSET_SECURITY_DESCRIPTOR_OWNER pSetSecurityDescriptorOwner;

} YORI_ADVAPI32_FUNCTIONS, *PYORI_ADVAPI32_FUNCTIONS;

extern YORI_ADVAPI32_FUNCTIONS DllAdvApi32;

/**
 A prototype for the FDICreate function.
 */
typedef
LPVOID DIAMONDAPI
CAB_FDI_CREATE(PCAB_CB_ALLOC, PCAB_CB_FREE, PCAB_CB_FDI_FILE_OPEN, PCAB_CB_FDI_FILE_READ, PCAB_CB_FDI_FILE_WRITE, PCAB_CB_FDI_FILE_CLOSE, PCAB_CB_FDI_FILE_SEEK, INT, PCAB_CB_ERROR);

/**
 A prototype for a pointer to the FDICreate function.
 */
typedef CAB_FDI_CREATE *PCAB_FDI_CREATE;

/**
 A prototype for the FDICopy function.
 */
typedef
LPVOID DIAMONDAPI
CAB_FDI_COPY(LPVOID, LPSTR, LPSTR, INT, PCAB_CB_FDI_NOTIFY, LPVOID, LPVOID);

/**
 A prototype for a pointer to the FDICopy function.
 */
typedef CAB_FDI_COPY *PCAB_FDI_COPY;

/**
 A prototype for the FDIDestroy function.
 */
typedef
BOOL DIAMONDAPI
CAB_FDI_DESTROY(LPVOID);

/**
 A prototype for a pointer to the FDIDestroy function.
 */
typedef CAB_FDI_DESTROY *PCAB_FDI_DESTROY;

/**
 A prototype for the FCICreate function.
 */
typedef
LPVOID DIAMONDAPI
CAB_FCI_CREATE(PCAB_CB_ERROR, PCAB_CB_FCI_FILE_PLACED, PCAB_CB_ALLOC, PCAB_CB_FREE, PCAB_CB_FCI_FILE_OPEN, PCAB_CB_FCI_FILE_READ, PCAB_CB_FCI_FILE_WRITE, PCAB_CB_FCI_FILE_CLOSE, PCAB_CB_FCI_FILE_SEEK, PCAB_CB_FCI_FILE_DELETE, PCAB_CB_FCI_GET_TEMP_FILE, PCAB_FCI_CONTEXT, PVOID);

/**
 A prototype for a pointer to the FCICreate function.
 */
typedef CAB_FCI_CREATE *PCAB_FCI_CREATE;

/**
 A prototype for the FCIAddFile function.
 */
typedef
BOOL DIAMONDAPI
CAB_FCI_ADD_FILE(PCAB_FCI_CONTEXT, LPSTR, LPSTR, BOOL, PCAB_CB_FCI_GET_NEXT_CABINET, PCAB_CB_FCI_STATUS, PCAB_CB_FCI_GET_OPEN_INFO, WORD);

/**
 A prototype for a pointer to the FCIAddFile function.
 */
typedef CAB_FCI_ADD_FILE *PCAB_FCI_ADD_FILE;

/**
 A prototype for the FCIFlushCabinet function.
 */
typedef
BOOL DIAMONDAPI
CAB_FCI_FLUSH_CABINET(PCAB_FCI_CONTEXT, BOOL, PCAB_CB_FCI_GET_NEXT_CABINET, PCAB_CB_FCI_STATUS);

/**
 A prototype for a pointer to the FCIFlushCabinet function.
 */
typedef CAB_FCI_FLUSH_CABINET *PCAB_FCI_FLUSH_CABINET;

/**
 A prototype for the FCIFlushFolder function.
 */
typedef
BOOL DIAMONDAPI
CAB_FCI_FLUSH_FOLDER(PCAB_FCI_CONTEXT, PCAB_CB_FCI_GET_NEXT_CABINET, PCAB_CB_FCI_STATUS);

/**
 A prototype for a pointer to the FCIFlushFolder function.
 */
typedef CAB_FCI_FLUSH_FOLDER *PCAB_FCI_FLUSH_FOLDER;

/**
 A prototype for the FCIDestroy function.
 */
typedef
BOOL DIAMONDAPI
CAB_FCI_DESTROY(PCAB_FCI_CONTEXT);

/**
 A prototype for a pointer to the FCIDestroy function.
 */
typedef CAB_FCI_DESTROY *PCAB_FCI_DESTROY;

/**
 A structure containing optional function pointers to cabinet.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_CABINET_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to FCIAddFile.
     */
    PCAB_FCI_ADD_FILE pFciAddFile;

    /**
     If it's available on the current system, a pointer to FCICreate.
     */
    PCAB_FCI_CREATE pFciCreate;

    /**
     If it's available on the current system, a pointer to FCIDestroy.
     */
    PCAB_FCI_DESTROY pFciDestroy;

    /**
     If it's available on the current system, a pointer to FCIFlushCabinet.
     */
    PCAB_FCI_FLUSH_CABINET pFciFlushCabinet;

    /**
     If it's available on the current system, a pointer to FCIFlushFolder.
     */
    PCAB_FCI_FLUSH_FOLDER pFciFlushFolder;

    /**
     If it's available on the current system, a pointer to FDICreate.
     */
    PCAB_FDI_CREATE pFdiCreate;

    /**
     If it's available on the current system, a pointer to FDICopy.
     */
    PCAB_FDI_COPY pFdiCopy;

    /**
     If it's available on the current system, a pointer to FDIDestroy.
     */
    PCAB_FDI_DESTROY pFdiDestroy;
} YORI_CABINET_FUNCTIONS, *PYORI_CABINET_FUNCTIONS;

extern YORI_CABINET_FUNCTIONS DllCabinet;

/**
 A prototype for the Ctl3dRegister function.
 */
typedef
BOOL WINAPI
CTL3D_REGISTER(HANDLE);

/**
 A prototype for a pointer to the Ctl3dRegister function.
 */
typedef CTL3D_REGISTER *PCTL3D_REGISTER;

/**
 A prototype for the Ctl3dAutoSubclass function.
 */
typedef
BOOL WINAPI
CTL3D_AUTOSUBCLASS(HANDLE);

/**
 A prototype for a pointer to the Ctl3dAutoSubclass function.
 */
typedef CTL3D_AUTOSUBCLASS *PCTL3D_AUTOSUBCLASS;

/**
 A structure containing optional function pointers to ctl3d.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_CTL3D_FUNCTIONS {

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to Ctl3dAutoSubclass.
     */
    PCTL3D_AUTOSUBCLASS pCtl3dAutoSubclass;

    /**
     If it's available on the current system, a pointer to Ctl3dRegister.
     */
    PCTL3D_REGISTER pCtl3dRegister;

} YORI_CTL3D_FUNCTIONS, *PYORI_CTL3D_FUNCTIONS;

extern YORI_CTL3D_FUNCTIONS DllCtl3d;

/**
 A prototype for the MiniDumpWriteDump function.
 */
typedef
BOOL WINAPI
MINI_DUMP_WRITE_DUMP(HANDLE, DWORD, HANDLE, DWORD, PVOID, PVOID, PVOID);

/**
 A prototype for a pointer to the MiniDumpWriteDump function.
 */
typedef MINI_DUMP_WRITE_DUMP *PMINI_DUMP_WRITE_DUMP;

/**
 A structure containing optional function pointers to dbghelp.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_DBGHELP_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to MiniDumpWriteDump.
     */
    PMINI_DUMP_WRITE_DUMP pMiniDumpWriteDump;
} YORI_DBGHELP_FUNCTIONS, *PYORI_DBGHELP_FUNCTIONS;

extern YORI_DBGHELP_FUNCTIONS DllDbgHelp;

/**
 A prototype for the MapFileAndCheckSumW function.
 */
typedef
DWORD WINAPI
MAP_FILE_AND_CHECKSUMW(LPCWSTR, PDWORD, PDWORD);

/**
 A prototype for a pointer to the MapFileAndCheckSumW function.
 */
typedef MAP_FILE_AND_CHECKSUMW *PMAP_FILE_AND_CHECKSUMW;

/**
 A structure containing optional function pointers to imagehlp.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_IMAGEHLP_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to
     MapFileAndCheckSumW.
     */
    PMAP_FILE_AND_CHECKSUMW pMapFileAndCheckSumW;
} YORI_IMAGEHLP_FUNCTIONS, *PYORI_IMAGEHLP_FUNCTIONS;

extern YORI_IMAGEHLP_FUNCTIONS DllImageHlp;

/**
 A prototype for the CoCreateInstance function.
 */
typedef
HRESULT WINAPI
CO_CREATE_INSTANCE(CONST GUID *, LPVOID, DWORD, CONST GUID *, LPVOID *);

/**
 A prototype for a pointer to the CoCreateInstance function.
 */
typedef CO_CREATE_INSTANCE *PCO_CREATE_INSTANCE;

/**
 A prototype for the CoInitialize function.
 */
typedef
HRESULT WINAPI
CO_INITIALIZE(LPVOID);

/**
 A prototype for a pointer to the CoInitialize function.
 */
typedef CO_INITIALIZE *PCO_INITIALIZE;

/**
 A prototype for the CoTaskMemFree function.
 */
typedef
VOID WINAPI
CO_TASK_MEM_FREE(PVOID);

/**
 A prototype for a pointer to the CoTaskMemFree function.
 */
typedef CO_TASK_MEM_FREE *PCO_TASK_MEM_FREE;

/**
 A structure containing optional function pointers to ole32.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_OLE32_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to CoCreateInstance.
     */
    PCO_CREATE_INSTANCE pCoCreateInstance;

    /**
     If it's available on the current system, a pointer to CoInitialize.
     */
    PCO_INITIALIZE pCoInitialize;

    /**
     If it's available on the current system, a pointer to CoTaskMemFree.
     */
    PCO_TASK_MEM_FREE pCoTaskMemFree;
} YORI_OLE32_FUNCTIONS, *PYORI_OLE32_FUNCTIONS;

extern YORI_OLE32_FUNCTIONS DllOle32;

/**
 A prototype for the GetModuleFileNameExW function.
 */
typedef
DWORD WINAPI
GET_MODULE_FILE_NAME_EXW(HANDLE, HANDLE, LPTSTR, DWORD);

/**
 A prototype for a pointer to the GetModuleFileNameExW function.
 */
typedef GET_MODULE_FILE_NAME_EXW *PGET_MODULE_FILE_NAME_EXW;

/**
 A structure containing optional function pointers to psapi.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_PSAPI_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to
     GetModuleFileNameEx.
     */
    PGET_MODULE_FILE_NAME_EXW pGetModuleFileNameExW;

} YORI_PSAPI_FUNCTIONS, *PYORI_PSAPI_FUNCTIONS;

extern YORI_PSAPI_FUNCTIONS DllPsapi;

/**
 A prototype for the ExtractIconExW function.
 */
typedef
DWORD WINAPI
EXTRACT_ICON_EXW(LPCWSTR, INT, HICON *, HICON *, DWORD);

/**
 A prototype for a pointer to the ExtractIconExW function.
 */
typedef EXTRACT_ICON_EXW *PEXTRACT_ICON_EXW;

/**
 A prototype for the SHBrowseForFolderW function.
 */
typedef
PVOID WINAPI
SH_BROWSE_FOR_FOLDERW(PYORI_BROWSEINFO);

/**
 A prototype for a pointer to the SHBrowseForFolderW function.
 */
typedef SH_BROWSE_FOR_FOLDERW *PSH_BROWSE_FOR_FOLDERW;

/**
 A prototype for the SHFileOperationW function.
 */
typedef
int WINAPI
SH_FILE_OPERATIONW(PYORI_SHFILEOP);

/**
 A prototype for a pointer to the SHFileOperationW function.
 */
typedef SH_FILE_OPERATIONW *PSH_FILE_OPERATIONW;

/**
 A prototype for the SHGetKnownFolderPath function.
 */
typedef
LONG WINAPI
SH_GET_KNOWN_FOLDER_PATH(CONST GUID *, DWORD, HANDLE, PWSTR *);

/**
 A prototype for a pointer to the SHGetKnownFolderPath function.
 */
typedef SH_GET_KNOWN_FOLDER_PATH *PSH_GET_KNOWN_FOLDER_PATH;

/**
 A prototype for the SHGetPathFromIDListW function.
 */
typedef
LONG WINAPI
SH_GET_PATH_FROM_ID_LISTW(PVOID, LPWSTR);

/**
 A prototype for a pointer to the SHGetPathFromIDListW function.
 */
typedef SH_GET_PATH_FROM_ID_LISTW *PSH_GET_PATH_FROM_ID_LISTW;

/**
 A prototype for the SHGetSpecialFolderPathW function.
 */
typedef
LONG WINAPI
SH_GET_SPECIAL_FOLDER_PATHW(HWND, LPWSTR, INT, BOOL);

/**
 A prototype for a pointer to the SHGetSpecialFolderPathW function.
 */
typedef SH_GET_SPECIAL_FOLDER_PATHW *PSH_GET_SPECIAL_FOLDER_PATHW;

/**
 A prototype for the ShellExecuteExW function.
 */
typedef
BOOL WINAPI
SHELL_EXECUTE_EXW(PYORI_SHELLEXECUTEINFO);

/**
 A prototype for a pointer to the ShellExecuteExW function.
 */
typedef SHELL_EXECUTE_EXW *PSHELL_EXECUTE_EXW;

/**
 A prototype for the ShellExecuteW function.
 */
typedef
HINSTANCE WINAPI
SHELL_EXECUTEW(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);

/**
 A prototype for a pointer to the ShellExecuteW function.
 */
typedef SHELL_EXECUTEW *PSHELL_EXECUTEW;

/**
 A structure containing optional function pointers to shell32.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_SHELL32_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to ExtractIconExW.
     */
    PEXTRACT_ICON_EXW pExtractIconExW;

    /**
     If it's available on the current system, a pointer to SHBrowseForFolderW.
     */
    PSH_BROWSE_FOR_FOLDERW pSHBrowseForFolderW;

    /**
     If it's available on the current system, a pointer to SHFileOperationW.
     */
    PSH_FILE_OPERATIONW pSHFileOperationW;

    /**
     If it's available on the current system, a pointer to SHGetKnownFolderPath.
     */
    PSH_GET_KNOWN_FOLDER_PATH pSHGetKnownFolderPath;

    /**
     If it's available on the current system, a pointer to SHGetPathFromIDListW.
     */
    PSH_GET_PATH_FROM_ID_LISTW pSHGetPathFromIDListW;

    /**
     If it's available on the current system, a pointer to SHGetSpecialFolderPathW.
     */
    PSH_GET_SPECIAL_FOLDER_PATHW pSHGetSpecialFolderPathW;

    /**
     If it's available on the current system, a pointer to ShellExecuteExW.
     */
    PSHELL_EXECUTE_EXW pShellExecuteExW;

    /**
     If it's available on the current system, a pointer to ShellExecuteW.
     */
    PSHELL_EXECUTEW pShellExecuteW;
} YORI_SHELL32_FUNCTIONS, *PYORI_SHELL32_FUNCTIONS;

extern YORI_SHELL32_FUNCTIONS DllShell32;

/**
 A prototype for the SHGetFolderPathW function.
 */
typedef
HRESULT WINAPI
SH_GET_FOLDER_PATHW(HWND, INT, HANDLE, DWORD, LPTSTR);

/**
 A prototype for a pointer to the SHGetFolderPathW function.
 */
typedef SH_GET_FOLDER_PATHW *PSH_GET_FOLDER_PATHW;

/**
 A structure containing optional function pointers to shfolder.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_SHFOLDER_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to SHGetFolderPathW.
     */
    PSH_GET_FOLDER_PATHW pSHGetFolderPathW;
} YORI_SHFOLDER_FUNCTIONS, *PYORI_SHFOLDER_FUNCTIONS;

extern YORI_SHFOLDER_FUNCTIONS DllShfolder;

/**
 A prototype for the CascadeWindows function.
 */
typedef
WORD WINAPI
CASCADE_WINDOWS(HWND, UINT, RECT*, UINT, HWND*);

/**
 A prototype for a pointer to the CascadeWindows function.
 */
typedef CASCADE_WINDOWS *PCASCADE_WINDOWS;

/**
 A prototype for the CloseClipboard function.
 */
typedef
BOOL WINAPI
CLOSE_CLIPBOARD(VOID);

/**
 A prototype for a pointer to the CloseClipboard function.
 */
typedef CLOSE_CLIPBOARD *PCLOSE_CLIPBOARD;

/**
 A prototype for the DdeClientTransaction function.
 */
typedef
HDDEDATA WINAPI
DDE_CLIENT_TRANSACTION(LPBYTE, DWORD, HCONV, HSZ, DWORD, DWORD, DWORD, LPDWORD);

/**
 A prototype for a pointer to the DdeClientTransaction function.
 */
typedef DDE_CLIENT_TRANSACTION *PDDE_CLIENT_TRANSACTION;

/**
 A prototype for the DdeConnect function.
 */
typedef
HCONV WINAPI
DDE_CONNECT(DWORD, HSZ, HSZ, PCONVCONTEXT);

/**
 A prototype for a pointer to the DdeConnect function.
 */
typedef DDE_CONNECT *PDDE_CONNECT;

/**
 A prototype for the DdeCreateDataHandle function.
 */
typedef
HDDEDATA WINAPI
DDE_CREATE_DATA_HANDLE(DWORD, LPBYTE, DWORD, DWORD, HSZ, DWORD, DWORD);

/**
 A prototype for a pointer to the DdeCreateDataHandle function.
 */
typedef DDE_CREATE_DATA_HANDLE *PDDE_CREATE_DATA_HANDLE;

/**
 A prototype for the DdeCreateStringHandle function.
 */
typedef
HSZ WINAPI
DDE_CREATE_STRING_HANDLEW(DWORD, LPWSTR, INT);

/**
 A prototype for a pointer to the DdeCreateStringHandle function.
 */
typedef DDE_CREATE_STRING_HANDLEW *PDDE_CREATE_STRING_HANDLEW;

/**
 A prototype for the DdeDisconnect function.
 */
typedef
BOOL WINAPI
DDE_DISCONNECT(HCONV);

/**
 A prototype for a pointer to the DdeDisconnect function.
 */
typedef DDE_DISCONNECT *PDDE_DISCONNECT;

/**
 A prototype for the DdeFreeStringHandle function.
 */
typedef
BOOL WINAPI
DDE_FREE_STRING_HANDLE(DWORD, HSZ);

/**
 A prototype for a pointer to the DdeFreeStringHandle function.
 */
typedef DDE_FREE_STRING_HANDLE *PDDE_FREE_STRING_HANDLE;

/**
 A prototype for the DdeInitializeW function.
 */
typedef
DWORD WINAPI
DDE_INITIALIZEW(LPDWORD, PFNCALLBACK, DWORD, DWORD);

/**
 A prototype for a pointer to the DdeInitializeW function.
 */
typedef DDE_INITIALIZEW *PDDE_INITIALIZEW;

/**
 A prototype for the DdeUninitialize function.
 */
typedef
BOOL WINAPI
DDE_UNINITIALIZE(DWORD);

/**
 A prototype for a pointer to the DdeUninitialize function.
 */
typedef DDE_UNINITIALIZE *PDDE_UNINITIALIZE;

/**
 A prototype for the DrawFrameControl function.
 */
typedef
BOOL WINAPI
DRAW_FRAME_CONTROL(HDC, LPRECT, DWORD, DWORD);

/**
 A prototype for a pointer to the DrawFrameControl function.
 */
typedef DRAW_FRAME_CONTROL *PDRAW_FRAME_CONTROL;

/**
 A prototype for the DrawIconEx function.
 */
typedef
BOOL WINAPI
DRAW_ICON_EX(HDC, INT, INT, HICON, INT, INT, DWORD, HBRUSH, DWORD);

/**
 A prototype for a pointer to the DrawIconEx function.
 */
typedef DRAW_ICON_EX *PDRAW_ICON_EX;

/**
 A prototype for the EmptyClipboard function.
 */
typedef
BOOL WINAPI
EMPTY_CLIPBOARD(VOID);

/**
 A prototype for a pointer to the EmptyClipboard function.
 */
typedef EMPTY_CLIPBOARD *PEMPTY_CLIPBOARD;

/**
 A prototype for the EnumClipboardFormats function.
 */
typedef
DWORD WINAPI
ENUM_CLIPBOARD_FORMATS(DWORD);

/**
 A prototype for a pointer to the EnumClipboardFormats function.
 */
typedef ENUM_CLIPBOARD_FORMATS *PENUM_CLIPBOARD_FORMATS;

/**
 A prototype for the ExitWindowsEx function.
 */
typedef
BOOL WINAPI
EXIT_WINDOWS_EX(DWORD, DWORD);

/**
 A prototype for a pointer to the ExitWindowsEx function.
 */
typedef EXIT_WINDOWS_EX *PEXIT_WINDOWS_EX;

/**
 A prototype for the FindWindowW function.
 */
typedef
HWND WINAPI
FIND_WINDOWW(LPCTSTR, LPCTSTR);

/**
 A prototype for a pointer to the FindWindowW function.
 */
typedef FIND_WINDOWW *PFIND_WINDOWW;

/**
 A prototype for the GetClientRect function.
 */
typedef
BOOL WINAPI
GET_CLIENT_RECT(HWND, LPRECT);

/**
 A prototype for a pointer to the GetClientRect function.
 */
typedef GET_CLIENT_RECT *PGET_CLIENT_RECT;

/**
 A prototype for the GetClipboardData function.
 */
typedef
HANDLE WINAPI
GET_CLIPBOARD_DATA(UINT);

/**
 A prototype for a pointer to the GetClipboardData function.
 */
typedef GET_CLIPBOARD_DATA *PGET_CLIPBOARD_DATA;

/**
 A prototype for the GetClipboardFormatNameW function.
 */
typedef
INT WINAPI
GET_CLIPBOARD_FORMAT_NAMEW(UINT, LPWSTR, DWORD);

/**
 A prototype for a pointer to the GetClipboardFormatNameW function.
 */
typedef GET_CLIPBOARD_FORMAT_NAMEW *PGET_CLIPBOARD_FORMAT_NAMEW;

/**
 A prototype for the GetDesktopWindow function.
 */
typedef
HWND WINAPI
GET_DESKTOP_WINDOW(VOID);

/**
 A prototype for a pointer to the GetDesktopWindow function.
 */
typedef GET_DESKTOP_WINDOW *PGET_DESKTOP_WINDOW;

/**
 A prototype for the GetKeyboardLayout function.
 */
typedef
HKL WINAPI
GET_KEYBOARD_LAYOUT(DWORD);

/**
 A prototype for a pointer to the GetKeyboardLayout function.
 */
typedef GET_KEYBOARD_LAYOUT *PGET_KEYBOARD_LAYOUT;

/**
 A prototype for the GetTaskmanWindow function.
 */
typedef
HWND WINAPI
GET_TASKMAN_WINDOW(VOID);

/**
 A prototype for a pointer to the GetTaskmanWindow function.
 */
typedef GET_TASKMAN_WINDOW *PGET_TASKMAN_WINDOW;

/**
 A prototype for the GetWindowRect function.
 */
typedef
BOOL WINAPI
GET_WINDOW_RECT(HWND, LPRECT);

/**
 A prototype for a pointer to the GetWindowRect function.
 */
typedef GET_WINDOW_RECT *PGET_WINDOW_RECT;

/**
 A prototype for the LoadImageW function.
 */
typedef
HICON WINAPI
LOAD_IMAGEW(HINSTANCE, LPCWSTR, DWORD, INT, INT, DWORD);

/**
 A prototype for a pointer to the LoadImageW function.
 */
typedef LOAD_IMAGEW *PLOAD_IMAGEW;

/**
 A prototype for the LockWorkStation function.
 */
typedef
BOOL WINAPI
LOCK_WORKSTATION(VOID);

/**
 A prototype for a pointer to the LockWorkStation function.
 */
typedef LOCK_WORKSTATION *PLOCK_WORKSTATION;

/**
 A prototype for the MoveWindow function.
 */
typedef
BOOL WINAPI
MOVE_WINDOW(HWND, INT, INT, INT, INT, BOOL);

/**
 A prototype for a pointer to the MoveWindow function.
 */
typedef MOVE_WINDOW *PMOVE_WINDOW;

/**
 A prototype for the OpenClipboard function.
 */
typedef
BOOL WINAPI
OPEN_CLIPBOARD(HANDLE);

/**
 A prototype for a pointer to the OpenClipboard function.
 */
typedef OPEN_CLIPBOARD *POPEN_CLIPBOARD;

/**
 A prototype for the RegisterClipboardFormatW function.
 */
typedef
UINT WINAPI
REGISTER_CLIPBOARD_FORMATW(LPCWSTR);

/**
 A prototype for a pointer to the RegisterClipboardFormatW function.
 */
typedef REGISTER_CLIPBOARD_FORMATW *PREGISTER_CLIPBOARD_FORMATW;

/**
 A prototype for the RegisterShellHookWindow function.
 */
typedef
BOOL WINAPI
REGISTER_SHELL_HOOK_WINDOW(HWND);

/**
 A prototype for a pointer to the RegisterShellHookWindow function.
 */
typedef REGISTER_SHELL_HOOK_WINDOW *PREGISTER_SHELL_HOOK_WINDOW;

/**
 A prototype for the SendMessageTimeoutW function.
 */
typedef
LRESULT WINAPI
SEND_MESSAGE_TIMEOUTW(HWND, UINT, WPARAM, LPARAM, UINT, UINT, PDWORD_PTR);

/**
 A prototype for a pointer to the SendMessageTimeoutW function.
 */
typedef SEND_MESSAGE_TIMEOUTW *PSEND_MESSAGE_TIMEOUTW;

/**
 A prototype for the SetClipboardData function.
 */
typedef
HANDLE WINAPI
SET_CLIPBOARD_DATA(UINT, HANDLE);

/**
 A prototype for a pointer to the SetClipboardData function.
 */
typedef SET_CLIPBOARD_DATA *PSET_CLIPBOARD_DATA;

/**
 A prototype for the SetForegroundWindow function.
 */
typedef
BOOL WINAPI
SET_FOREGROUND_WINDOW(HWND);

/**
 A prototype for a pointer to the SetForegroundWindow function.
 */
typedef SET_FOREGROUND_WINDOW *PSET_FOREGROUND_WINDOW;

/**
 A prototype for the SetShellWindow function.
 */
typedef
BOOL WINAPI
SET_SHELL_WINDOW(HWND);

/**
 A prototype for a pointer to the SetShellWindow function.
 */
typedef SET_SHELL_WINDOW *PSET_SHELL_WINDOW;

/**
 A prototype for the SetTaskmanWindow function.
 */
typedef
BOOL WINAPI
SET_TASKMAN_WINDOW(HWND);

/**
 A prototype for a pointer to the SetTaskmanWindow function.
 */
typedef SET_TASKMAN_WINDOW *PSET_TASKMAN_WINDOW;

/**
 A prototype for the SetWindowPos function.
 */
typedef
BOOL WINAPI
SET_WINDOW_POS(HWND, HWND, INT, INT, INT, INT, UINT);

/**
 A prototype for a pointer to the SetWindowPos function.
 */
typedef SET_WINDOW_POS *PSET_WINDOW_POS;

/**
 A prototype for the SetWindowTextW function.
 */
typedef
BOOL WINAPI
SET_WINDOW_TEXTW(HWND, LPCTSTR);

/**
 A prototype for a pointer to the SetWindowTextW function.
 */
typedef SET_WINDOW_TEXTW *PSET_WINDOW_TEXTW;

/**
 A prototype for the ShowWindow function.
 */
typedef
BOOL WINAPI
SHOW_WINDOW(HWND, INT);

/**
 A prototype for a pointer to the ShowWindow function.
 */
typedef SHOW_WINDOW *PSHOW_WINDOW;

/**
 A prototype for the ShowWindowAsync function.
 */
typedef
BOOL WINAPI
SHOW_WINDOW_ASYNC(HWND, INT);

/**
 A prototype for a pointer to the ShowWindowAsync function.
 */
typedef SHOW_WINDOW_ASYNC *PSHOW_WINDOW_ASYNC;

/**
 A prototype for the TileWindows function.
 */
typedef
WORD WINAPI
TILE_WINDOWS(HWND, UINT, RECT*, UINT, HWND*);

/**
 A prototype for a pointer to the TileWindows function.
 */
typedef TILE_WINDOWS *PTILE_WINDOWS;

/**
 A structure containing optional function pointers to user32.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_USER32_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to CascadeWindows.
     */
    PCASCADE_WINDOWS pCascadeWindows;

    /**
     If it's available on the current system, a pointer to CloseClipboard.
     */
    PCLOSE_CLIPBOARD pCloseClipboard;

    /**
     If it's available on the current system, a pointer to DdeClientTransaction.
     */
    PDDE_CLIENT_TRANSACTION pDdeClientTransaction;

    /**
     If it's available on the current system, a pointer to DdeClientTransaction.
     */
    PDDE_CONNECT pDdeConnect;

    /**
     If it's available on the current system, a pointer to DdeCreateDataHandle.
     */
    PDDE_CREATE_DATA_HANDLE pDdeCreateDataHandle;

    /**
     If it's available on the current system, a pointer to DdeCreateStringHandleW.
     */
    PDDE_CREATE_STRING_HANDLEW pDdeCreateStringHandleW;

    /**
     If it's available on the current system, a pointer to DdeDisconnect.
     */
    PDDE_DISCONNECT pDdeDisconnect;

    /**
     If it's available on the current system, a pointer to DdeFreeStringHandle.
     */
    PDDE_FREE_STRING_HANDLE pDdeFreeStringHandle;

    /**
     If it's available on the current system, a pointer to DdeInitializeW.
     */
    PDDE_INITIALIZEW pDdeInitializeW;

    /**
     If it's available on the current system, a pointer to DdeUninitialize.
     */
    PDDE_UNINITIALIZE pDdeUninitialize;

    /**
     If it's available on the current system, a pointer to DrawFrameControl.
     */
    PDRAW_FRAME_CONTROL pDrawFrameControl;

    /**
     If it's available on the current system, a pointer to DrawIconEx.
     */
    PDRAW_ICON_EX pDrawIconEx;

    /**
     If it's available on the current system, a pointer to EmptyClipboard.
     */
    PEMPTY_CLIPBOARD pEmptyClipboard;

    /**
     If it's available on the current system, a pointer to EnumClipboardFormats.
     */
    PENUM_CLIPBOARD_FORMATS pEnumClipboardFormats;

    /**
     If it's available on the current system, a pointer to ExitWindowsEx.
     */
    PEXIT_WINDOWS_EX pExitWindowsEx;

    /**
     If it's available on the current system, a pointer to FindWindowW.
     */
    PFIND_WINDOWW pFindWindowW;

    /**
     If it's available on the current system, a pointer to GetClientRect.
     */
    PGET_CLIENT_RECT pGetClientRect;

    /**
     If it's available on the current system, a pointer to GetClipboardData.
     */
    PGET_CLIPBOARD_DATA pGetClipboardData;

    /**
     If it's available on the current system, a pointer to GetClipboardFormatNameW.
     */
    PGET_CLIPBOARD_FORMAT_NAMEW pGetClipboardFormatNameW;

    /**
     If it's available on the current system, a pointer to GetDesktopWindow.
     */
    PGET_DESKTOP_WINDOW pGetDesktopWindow;

    /**
     If it's available on the current system, a pointer to GetKeyboardLayout.
     */
    PGET_KEYBOARD_LAYOUT pGetKeyboardLayout;

    /**
     If it's available on the current system, a pointer to GetTaskmanWindow.
     */
    PGET_TASKMAN_WINDOW pGetTaskmanWindow;

    /**
     If it's available on the current system, a pointer to GetWindowRect.
     */
    PGET_WINDOW_RECT pGetWindowRect;

    /**
     If it's available on the current system, a pointer to LoadImageW.
     */
    PLOAD_IMAGEW pLoadImageW;

    /**
     If it's available on the current system, a pointer to LockWorkStation.
     */
    PLOCK_WORKSTATION pLockWorkStation;

    /**
     If it's available on the current system, a pointer to MoveWindow.
     */
    PMOVE_WINDOW pMoveWindow;

    /**
     If it's available on the current system, a pointer to OpenClipboard.
     */
    POPEN_CLIPBOARD pOpenClipboard;

    /**
     If it's available on the current system, a pointer to RegisterClipboardFormatW.
     */
    PREGISTER_CLIPBOARD_FORMATW pRegisterClipboardFormatW;

    /**
     If it's available on the current system, a pointer to RegisterShellHookWindow.
     */
    PREGISTER_SHELL_HOOK_WINDOW pRegisterShellHookWindow;

    /**
     If it's available on the current system, a pointer to SendMessageTimeoutW.
     */
    PSEND_MESSAGE_TIMEOUTW pSendMessageTimeoutW;

    /**
     If it's available on the current system, a pointer to SetClipboardData.
     */
    PSET_CLIPBOARD_DATA pSetClipboardData;

    /**
     If it's available on the current system, a pointer to SetForegroundWindow.
     */
    PSET_FOREGROUND_WINDOW pSetForegroundWindow;

    /**
     If it's available on the current system, a pointer to SetShellWindow.
     */
    PSET_SHELL_WINDOW pSetShellWindow;

    /**
     If it's available on the current system, a pointer to SetTaskmanWindow.
     */
    PSET_TASKMAN_WINDOW pSetTaskmanWindow;

    /**
     If it's available on the current system, a pointer to SetWindowPos.
     */
    PSET_WINDOW_POS pSetWindowPos;

    /**
     If it's available on the current system, a pointer to SetWindowTextW.
     */
    PSET_WINDOW_TEXTW pSetWindowTextW;

    /**
     If it's available on the current system, a pointer to ShowWindow.
     */
    PSHOW_WINDOW pShowWindow;

    /**
     If it's available on the current system, a pointer to ShowWindowAsync.
     */
    PSHOW_WINDOW_ASYNC pShowWindowAsync;

    /**
     If it's available on the current system, a pointer to TileWindows.
     */
    PTILE_WINDOWS pTileWindows;

} YORI_USER32_FUNCTIONS, *PYORI_USER32_FUNCTIONS;

extern YORI_USER32_FUNCTIONS DllUser32;

/**
 A prototype for the GetFileVersionInfoSizeW function.
 */
typedef
DWORD WINAPI
GET_FILE_VERSION_INFO_SIZEW(LPWSTR, LPDWORD);

/**
 A prototype for a pointer to the GetFileVersionInfoSizeW function.
 */
typedef GET_FILE_VERSION_INFO_SIZEW *PGET_FILE_VERSION_INFO_SIZEW;

/**
 A prototype for the GetFileVersionInfoW function.
 */
typedef
BOOL WINAPI
GET_FILE_VERSION_INFOW(LPWSTR, DWORD, DWORD, LPVOID);

/**
 A prototype for a pointer to the GetFileVersionInfoW function.
 */
typedef GET_FILE_VERSION_INFOW *PGET_FILE_VERSION_INFOW;

/**
 A prototype for the VerQueryValueW function.
 */
typedef
BOOL WINAPI
VER_QUERY_VALUEW(CONST LPVOID, LPWSTR, LPVOID *, PUINT);

/**
 A prototype for a pointer to the VerQueryValueW function.
 */
typedef VER_QUERY_VALUEW *PVER_QUERY_VALUEW;

/**
 A structure containing optional function pointers to version.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_VERSION_FUNCTIONS {

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to GetFileVersionInfoSizeW.
     */
    PGET_FILE_VERSION_INFO_SIZEW pGetFileVersionInfoSizeW;

    /**
     If it's available on the current system, a pointer to GetFileVersionInfoW.
     */
    PGET_FILE_VERSION_INFOW pGetFileVersionInfoW;

    /**
     If it's available on the current system, a pointer to VerQueryValueW.
     */
    PVER_QUERY_VALUEW pVerQueryValueW;
} YORI_VERSION_FUNCTIONS, *PYORI_VERSION_FUNCTIONS;

extern YORI_VERSION_FUNCTIONS DllVersion;

/**
 A prototype for the AttachVirtualDisk function.
 */
typedef
DWORD WINAPI
ATTACH_VIRTUAL_DISK(HANDLE, PSECURITY_DESCRIPTOR, DWORD, DWORD, LPVOID, LPOVERLAPPED);

/**
 A prototype for a pointer to the AttachVirtualDisk function.
 */
typedef ATTACH_VIRTUAL_DISK *PATTACH_VIRTUAL_DISK;

/**
 A prototype for the CompactVirtualDisk function.
 */
typedef
DWORD WINAPI
COMPACT_VIRTUAL_DISK(HANDLE, DWORD, PCOMPACT_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED);

/**
 A prototype for a pointer to the CompactVirtualDisk function.
 */
typedef COMPACT_VIRTUAL_DISK *PCOMPACT_VIRTUAL_DISK;

/**
 A prototype for the CreateVirtualDisk function.
 */
typedef
DWORD WINAPI
CREATE_VIRTUAL_DISK(PVIRTUAL_STORAGE_TYPE, PCWSTR, DWORD, PSECURITY_DESCRIPTOR, DWORD, DWORD, LPVOID, LPOVERLAPPED, PHANDLE);

/**
 A prototype for a pointer to the CreateVirtualDisk function.
 */
typedef CREATE_VIRTUAL_DISK *PCREATE_VIRTUAL_DISK;

/**
 A prototype for the DetachVirtualDisk function.
 */
typedef
DWORD WINAPI
DETACH_VIRTUAL_DISK(HANDLE, DWORD, DWORD);

/**
 A prototype for a pointer to the DetachVirtualDisk function.
 */
typedef DETACH_VIRTUAL_DISK *PDETACH_VIRTUAL_DISK;

/**
 A prototype for the ExpandVirtualDisk function.
 */
typedef
DWORD WINAPI
EXPAND_VIRTUAL_DISK(HANDLE, DWORD, PEXPAND_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED);

/**
 A prototype for a pointer to the ExpandVirtualDisk function.
 */
typedef EXPAND_VIRTUAL_DISK *PEXPAND_VIRTUAL_DISK;

/**
 A prototype for the GetVirtualDiskPhysicalPath function.
 */
typedef
DWORD WINAPI
GET_VIRTUAL_DISK_PHYSICAL_PATH(HANDLE, PDWORD, LPWSTR);

/**
 A prototype for a pointer to the GetVirtualDiskPhysicalPath function.
 */
typedef GET_VIRTUAL_DISK_PHYSICAL_PATH *PGET_VIRTUAL_DISK_PHYSICAL_PATH;

/**
 A prototype for the MergeVirtualDisk function.
 */
typedef
DWORD WINAPI
MERGE_VIRTUAL_DISK(HANDLE, DWORD, PMERGE_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED);

/**
 A prototype for a pointer to the MergeVirtualDisk function.
 */
typedef MERGE_VIRTUAL_DISK *PMERGE_VIRTUAL_DISK;

/**
 A prototype for the OpenVirtualDisk function.
 */
typedef
DWORD WINAPI
OPEN_VIRTUAL_DISK(PVIRTUAL_STORAGE_TYPE, LPCWSTR, DWORD, DWORD, POPEN_VIRTUAL_DISK_PARAMETERS, PHANDLE);

/**
 A prototype for a pointer to the OpenVirtualDisk function.
 */
typedef OPEN_VIRTUAL_DISK *POPEN_VIRTUAL_DISK;

/**
 A prototype for the ResizeVirtualDisk function.
 */
typedef
DWORD WINAPI
RESIZE_VIRTUAL_DISK(HANDLE, DWORD, PRESIZE_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED);

/**
 A prototype for a pointer to the ResizeVirtualDisk function.
 */
typedef RESIZE_VIRTUAL_DISK *PRESIZE_VIRTUAL_DISK;

/**
 A structure containing optional function pointers to virtdisk.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_VIRTDISK_FUNCTIONS {

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to AttachVirtualDisk.
     */
    PATTACH_VIRTUAL_DISK pAttachVirtualDisk;

    /**
     If it's available on the current system, a pointer to CompactVirtualDisk.
     */
    PCOMPACT_VIRTUAL_DISK pCompactVirtualDisk;

    /**
     If it's available on the current system, a pointer to CreateVirtualDisk.
     */
    PCREATE_VIRTUAL_DISK pCreateVirtualDisk;

    /**
     If it's available on the current system, a pointer to DetachVirtualDisk.
     */
    PDETACH_VIRTUAL_DISK pDetachVirtualDisk;

    /**
     If it's available on the current system, a pointer to ExpandVirtualDisk.
     */
    PEXPAND_VIRTUAL_DISK pExpandVirtualDisk;

    /**
     If it's available on the current system, a pointer to GetVirtualDiskPhysicalPath.
     */
    PGET_VIRTUAL_DISK_PHYSICAL_PATH pGetVirtualDiskPhysicalPath;

    /**
     If it's available on the current system, a pointer to MergeVirtualDisk.
     */
    PMERGE_VIRTUAL_DISK pMergeVirtualDisk;

    /**
     If it's available on the current system, a pointer to OpenVirtualDisk.
     */
    POPEN_VIRTUAL_DISK pOpenVirtualDisk;

    /**
     If it's available on the current system, a pointer to ResizeVirtualDisk.
     */
    PRESIZE_VIRTUAL_DISK pResizeVirtualDisk;

} YORI_VIRTDISK_FUNCTIONS, *PYORI_VIRTDISK_FUNCTIONS;

extern YORI_VIRTDISK_FUNCTIONS DllVirtDisk;

/**
 A prototype for the BrandingFormatString function.
 */
typedef
LPWSTR WINAPI
BRANDING_FORMAT_STRING(LPCWSTR);

/**
 A prototype for a pointer to the BrandingFormatString function.
 */
typedef BRANDING_FORMAT_STRING *PBRANDING_FORMAT_STRING;

/**
 A structure containing optional function pointers to winbrand.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_WINBRAND_FUNCTIONS {

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to
     BrandingFormatString.
     */
    PBRANDING_FORMAT_STRING pBrandingFormatString;

} YORI_WINBRAND_FUNCTIONS, *PYORI_WINBRAND_FUNCTIONS;

extern YORI_WINBRAND_FUNCTIONS DllWinBrand;

/**
 A prototype for the WinHttpCloseHandle function.
 */
typedef
BOOL WINAPI
WIN_HTTP_CLOSE_HANDLE(LPVOID);

/**
 A prototype for a pointer to the WinHttpCloseHandle function.
 */
typedef WIN_HTTP_CLOSE_HANDLE *PWIN_HTTP_CLOSE_HANDLE;

/**
 A prototype for the WinHttpConnect function.
 */
typedef
LPVOID WINAPI
WIN_HTTP_CONNECT(LPVOID, LPCWSTR, WORD, DWORD);

/**
 A prototype for a pointer to the WinHttpConnect function.
 */
typedef WIN_HTTP_CONNECT *PWIN_HTTP_CONNECT;

/**
 A prototype for the WinHttpOpen function.
 */
typedef
LPVOID WINAPI
WIN_HTTP_OPEN(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);

/**
 A prototype for a pointer to the WinHttpOpen function.
 */
typedef WIN_HTTP_OPEN *PWIN_HTTP_OPEN;

/**
 A prototype for the WinHttpOpenRequest function.
 */
typedef
LPVOID WINAPI
WIN_HTTP_OPEN_REQUEST(LPVOID, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR *, DWORD);

/**
 A prototype for a pointer to the WinHttpOpenRequest function.
 */
typedef WIN_HTTP_OPEN_REQUEST *PWIN_HTTP_OPEN_REQUEST;

/**
 A prototype for the WinHttpQueryHeaders function.
 */
typedef
BOOL WINAPI
WIN_HTTP_QUERY_HEADERS(LPVOID, DWORD, LPCWSTR, LPVOID, LPDWORD, LPDWORD);

/**
 A prototype for a pointer to the WinHttpQueryHeaders function.
 */
typedef WIN_HTTP_QUERY_HEADERS *PWIN_HTTP_QUERY_HEADERS;

/**
 A prototype for the WinHttpReadData function.
 */
typedef
BOOL WINAPI
WIN_HTTP_READ_DATA(LPVOID, LPVOID, DWORD, LPDWORD);

/**
 A prototype for a pointer to the WinHttpReadData function.
 */
typedef WIN_HTTP_READ_DATA *PWIN_HTTP_READ_DATA;

/**
 A prototype for the WinHttpReceiveResponse function.
 */
typedef
BOOL WINAPI
WIN_HTTP_RECEIVE_RESPONSE(LPVOID, LPVOID);

/**
 A prototype for a pointer to the WinHttpReceiveResponse function.
 */
typedef WIN_HTTP_RECEIVE_RESPONSE *PWIN_HTTP_RECEIVE_RESPONSE;

/**
 A prototype for the WinHttpSendRequest function.
 */
typedef
BOOL WINAPI
WIN_HTTP_SEND_REQUEST(LPVOID, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);

/**
 A prototype for a pointer to the WinHttpSendRequest function.
 */
typedef WIN_HTTP_SEND_REQUEST *PWIN_HTTP_SEND_REQUEST;

/**
 A structure containing optional function pointers to winhttp.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_WINHTTP_FUNCTIONS {

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to WinHttpCloseHandle.
     */
    PWIN_HTTP_CLOSE_HANDLE pWinHttpCloseHandle;

    /**
     If it's available on the current system, a pointer to WinHttpConnect.
     */
    PWIN_HTTP_CONNECT pWinHttpConnect;

    /**
     If it's available on the current system, a pointer to WinHttpOpen.
     */
    PWIN_HTTP_OPEN pWinHttpOpen;

    /**
     If it's available on the current system, a pointer to WinHttpOpenRequest.
     */
    PWIN_HTTP_OPEN_REQUEST pWinHttpOpenRequest;

    /**
     If it's available on the current system, a pointer to
     WinHttpQueryHeaders.
     */
    PWIN_HTTP_QUERY_HEADERS pWinHttpQueryHeaders;

    /**
     If it's available on the current system, a pointer to
     WinHttpReadData.
     */
    PWIN_HTTP_READ_DATA pWinHttpReadData;

    /**
     If it's available on the current system, a pointer to
     WinHttpReceiveResponse.
     */
    PWIN_HTTP_RECEIVE_RESPONSE pWinHttpReceiveResponse;

    /**
     If it's available on the current system, a pointer to WinHttpSendRequest.
     */
    PWIN_HTTP_SEND_REQUEST pWinHttpSendRequest;

} YORI_WINHTTP_FUNCTIONS, *PYORI_WINHTTP_FUNCTIONS;

extern YORI_WINHTTP_FUNCTIONS DllWinHttp;


/**
 A prototype for the InternetOpenA function.
 */
typedef
LPVOID WINAPI
INTERNET_OPENA(LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD);

/**
 A prototype for a pointer to the InternetOpenA function.
 */
typedef INTERNET_OPENA *PINTERNET_OPENA;

/**
 A prototype for the InternetOpenW function.
 */
typedef
LPVOID WINAPI
INTERNET_OPENW(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);

/**
 A prototype for a pointer to the InternetOpenW function.
 */
typedef INTERNET_OPENW *PINTERNET_OPENW;

/**
 A prototype for the InternetOpenUrlA function.
 */
typedef
LPVOID WINAPI
INTERNET_OPEN_URLA(LPVOID, LPCSTR, LPCSTR, DWORD, DWORD, DWORD);

/**
 A prototype for a pointer to the InternetOpenUrlA function.
 */
typedef INTERNET_OPEN_URLA *PINTERNET_OPEN_URLA;

/**
 A prototype for the InternetOpenUrlW function.
 */
typedef
LPVOID WINAPI
INTERNET_OPEN_URLW(LPVOID, LPCWSTR, LPCWSTR, DWORD, DWORD, DWORD);

/**
 A prototype for a pointer to the InternetOpenUrlW function.
 */
typedef INTERNET_OPEN_URLW *PINTERNET_OPEN_URLW;

/**
 A prototype for the HttpQueryInfoA function.
 */
typedef
BOOL WINAPI
HTTP_QUERY_INFOA(LPVOID, DWORD, LPVOID, LPDWORD, LPDWORD);

/**
 A prototype for a pointer to the HttpQueryInfoA function.
 */
typedef HTTP_QUERY_INFOA *PHTTP_QUERY_INFOA;

/**
 A prototype for the HttpQueryInfoW function.
 */
typedef
BOOL WINAPI
HTTP_QUERY_INFOW(LPVOID, DWORD, LPVOID, LPDWORD, LPDWORD);

/**
 A prototype for a pointer to the HttpQueryInfoW function.
 */
typedef HTTP_QUERY_INFOW *PHTTP_QUERY_INFOW;

/**
 A prototype for the InternetReadFile function.
 */
typedef
BOOL WINAPI
INTERNET_READ_FILE(LPVOID, LPVOID, DWORD, LPDWORD);

/**
 A prototype for a pointer to the InternetReadFile function.
 */
typedef INTERNET_READ_FILE *PINTERNET_READ_FILE;

/**
 A prototype for the InternetCloseHandle function.
 */
typedef
BOOL WINAPI
INTERNET_CLOSE_HANDLE(LPVOID);

/**
 A prototype for a pointer to the InternetCloseHandle function.
 */
typedef INTERNET_CLOSE_HANDLE *PINTERNET_CLOSE_HANDLE;

/**
 A structure containing optional function pointers to wininet.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_WININET_FUNCTIONS {

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to HttpQueryInfoA.
     */
    PHTTP_QUERY_INFOA pHttpQueryInfoA;

    /**
     If it's available on the current system, a pointer to HttpQueryInfoW.
     */
    PHTTP_QUERY_INFOW pHttpQueryInfoW;

    /**
     If it's available on the current system, a pointer to InternetCloseHandle.
     */
    PINTERNET_CLOSE_HANDLE pInternetCloseHandle;

    /**
     If it's available on the current system, a pointer to InternetOpenA.
     */
    PINTERNET_OPENA pInternetOpenA;

    /**
     If it's available on the current system, a pointer to InternetOpenW.
     */
    PINTERNET_OPENW pInternetOpenW;

    /**
     If it's available on the current system, a pointer to InternetOpenUrlA.
     */
    PINTERNET_OPEN_URLA pInternetOpenUrlA;

    /**
     If it's available on the current system, a pointer to InternetOpenUrlW.
     */
    PINTERNET_OPEN_URLW pInternetOpenUrlW;

    /**
     If it's available on the current system, a pointer to InternetReadFile.
     */
    PINTERNET_READ_FILE pInternetReadFile;

} YORI_WININET_FUNCTIONS, *PYORI_WININET_FUNCTIONS;

extern YORI_WININET_FUNCTIONS DllWinInet;

/**
 A prototype for the WTSDisconnectSession function.
 */
typedef
BOOL WINAPI
WTS_DISCONNECT_SESSION(HANDLE, DWORD, BOOL);

/**
 A prototype for a pointer to the WTSDisconnectSession function.
 */
typedef WTS_DISCONNECT_SESSION *PWTS_DISCONNECT_SESSION;

/**
 A prototype for the WTSRegisterSessionNotification function.
 */
typedef
BOOL WINAPI
WTS_REGISTER_SESSION_NOTIFICATION(HWND, DWORD);

/**
 A prototype for a pointer to the WTSRegisterSessionNotification function.
 */
typedef WTS_REGISTER_SESSION_NOTIFICATION *PWTS_REGISTER_SESSION_NOTIFICATION;

/**
 A prototype for the WTSUnRegisterSessionNotification function.
 */
typedef
BOOL WINAPI
WTS_UNREGISTER_SESSION_NOTIFICATION(HWND);

/**
 A prototype for a pointer to the WTSUnRegisterSessionNotification function.
 */
typedef WTS_UNREGISTER_SESSION_NOTIFICATION *PWTS_UNREGISTER_SESSION_NOTIFICATION;

/**
 A structure containing optional function pointers to wtsapi32.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_WTSAPI32_FUNCTIONS {

    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to WTSDisconnectSession.
     */
    PWTS_DISCONNECT_SESSION pWTSDisconnectSession;

    /**
     If it's available on the current system, a pointer to
     WTSRegisterSessionNotification.
     */
    PWTS_REGISTER_SESSION_NOTIFICATION pWTSRegisterSessionNotification;

    /**
     If it's available on the current system, a pointer to
     WTSUnRegisterSessionNotification.
     */
    PWTS_UNREGISTER_SESSION_NOTIFICATION pWTSUnRegisterSessionNotification;

} YORI_WTSAPI32_FUNCTIONS, *PYORI_WTSAPI32_FUNCTIONS;

extern YORI_WTSAPI32_FUNCTIONS DllWtsApi32;



// vim:sw=4:ts=4:et:
