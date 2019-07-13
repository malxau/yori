/**
 * @file lib/yoricmpt.h
 *
 * Yori shell header file to define OS things that the compilation environment
 * doesn't support.
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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


#ifndef DWORD_PTR
#ifndef _WIN64

/**
 Definition for pointer size integer for compilers that don't contain it.
 */
typedef DWORD DWORD_PTR;

/**
 Definition for pointer size integer for compilers that don't contain it.
 */
typedef ULONG ULONG_PTR;
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
typedef long HRESULT;

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
    DWORD Ignored1[4];

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored2[10];

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
    YORI_LIB_PTR32 Ignored[3];

    /**
     Pointer to the process parameters.
     */
    PYORI_LIB_PROCESS_PARAMETERS32 ProcessParameters;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored2[17];

    /**
     Ignored for alignment.
     */
    DWORD Ignored3[19];

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
    YORI_LIB_PTR32 Ignored[3];

    /**
     Pointer to the process parameters.
     */
    PYORI_LIB_PROCESS_PARAMETERS32 ProcessParameters;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR32 Ignored2[17];

    /**
     Ignored for alignment.
     */
    DWORD Ignored3[18];

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
    DWORD Ignored1[4];

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR64 Ignored2[10];

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
    DWORD Ignored3;

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
    DWORD Ignored4;

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
    YORI_LIB_PTR64 Ignored[3];

    /**
     Pointer to the process parameters.
     */
    PYORI_LIB_PROCESS_PARAMETERS64 ProcessParameters;

    /**
     Ignored for alignment.
     */
    YORI_LIB_PTR64 Ignored2[17];

    /**
     Ignored for alignment.
     */
    DWORD Ignored3[26];

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

/**
 Definition of the system process information enumeration class for
 NtQuerySystemInformation .
 */
#define SystemProcessInformation (5)

/**
 Information returned about every process in the system.
 */
typedef struct _YORI_SYSTEM_PROCESS_INFORMATION {

    /**
     Offset from the beginning of this structure to the next entry, in bytes.
     */
    ULONG NextEntryOffset;

    /**
     Ignored in this application.
     */
    BYTE Reserved1[52];

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
     Ignored in this application.
     */
    PVOID Reserved3[5];

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
} YORI_SYSTEM_PROCESS_INFORMATION, *PYORI_SYSTEM_PROCESS_INFORMATION;


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
#define FILE_ATTRIBUTE_REPARSE_POINT      (0x400)
#endif

#ifndef FILE_ATTRIBUTE_SPARSE_FILE
/**
 Specifies the value for a file that is sparse if the compilation environment
 doesn't provide it.
 */
#define FILE_ATTRIBUTE_SPARSE_FILE        (0x200)
#endif

#ifndef FILE_ATTRIBUTE_COMPRESSED
/**
 Specifies the value for a file that has been compressed if the compilation
 environment doesn't provide it.
 */
#define FILE_ATTRIBUTE_COMPRESSED         (0x800)
#endif

#ifndef FILE_ATTRIBUTE_OFFLINE
/**
 Specifies the value for a file that is on slow storage if the compilation
 environment doesn't provide it.
 */
#define FILE_ATTRIBUTE_OFFLINE           (0x1000)
#endif

#ifndef FILE_ATTRIBUTE_ENCRYPTED
/**
 Specifies the value for a file that has been encrypted if the compilation
 environment doesn't provide it.
 */
#define FILE_ATTRIBUTE_ENCRYPTED         (0x4000)
#endif

#ifndef FILE_ATTRIBUTE_INTEGRITY_STREAM
/**
 Specifies the value for a file subject to CRC integrity detection if the
 compilation environment doesn't provide it.
 */
#define FILE_ATTRIBUTE_INTEGRITY_STREAM  (0x8000)
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
#endif

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

#ifndef HSHELL_RUDEAPPACTIVATED
/**
 A definition for HSHELL_RUDEAPPACTIVATED if it is not defined by the current
 compilation environment.
 */
#define HSHELL_RUDEAPPACTIVATED (0x8000 | HSHELL_WINDOWACTIVATED)
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

#ifndef DIAMONDAPI
/**
 If not defined by the compilation environment, the calling convention used
 by the Cabinet APIs.
 */
#define DIAMONDAPI __cdecl
#endif

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

/**
 A message structure that describes a shell app bar.
 */
typedef struct _YORI_APPBARDATA {

    /**
     The number of bytes in this structure.
     */
    DWORD cbSize;

    /**
     The window that is requesting app bar services.
     */
    HWND hWnd;

    /**
     A message to use to indicate back to the application app bar
     notifications.
     */
    UINT uCallbackMessage;

    /**
     The edge of the screen to attach to.
     */
    UINT uEdge;

    /**
     The window coordinates to use.
     */
    RECT rc;

    /**
     Extra information.
     */
    LPARAM lParam;
} YORI_APPBARDATA, *PYORI_APPBARDATA;


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
 A prototype for the RtlGetLastNtStatus function.
 */
typedef
LONG WINAPI
RTL_GET_LAST_NT_STATUS();

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
     NtQuerySystemInformation.
     */
    PNT_QUERY_SYSTEM_INFORMATION pNtQuerySystemInformation;

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
BOOL WINAPI
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
GET_CONSOLE_WINDOW();

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
GET_ENVIRONMENT_STRINGS();

/**
 A prototype for a pointer to the GetEnvironmentStrings function.
 */
typedef GET_ENVIRONMENT_STRINGS *PGET_ENVIRONMENT_STRINGS;

/**
 A prototype for the GetEnvironmentStringsW function.
 */
typedef
LPWSTR WINAPI
GET_ENVIRONMENT_STRINGSW();

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
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to AddConsoleAliasW.
     */
    PADD_CONSOLE_ALIASW pAddConsoleAliasW;

    /**
     If it's available on the current system, a pointer to AssignProcessToJobObject.
     */
    PASSIGN_PROCESS_TO_JOB_OBJECT pAssignProcessToJobObject;

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
     If it's available on the current system, a pointer to GetNativeSystemInfo.
     */
    PGET_NATIVE_SYSTEM_INFO pGetNativeSystemInfo;

    /**
     If it's available on the current system, a pointer to GetPrivateProfileSectionNamesW.
     */
    PGET_PRIVATE_PROFILE_SECTION_NAMESW pGetPrivateProfileSectionNamesW;

    /**
     If it's available on the current system, a pointer to GetProductInfo.
     */
    PGET_PRODUCT_INFO pGetProductInfo;

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
     If it's available on the current system, a pointer to GlobalMemoryStatusEx.
     */
    PGLOBAL_MEMORY_STATUS_EX pGlobalMemoryStatusEx;

    /**
     If it's available on the current system, a pointer to IsWow64Process.
     */
    PIS_WOW64_PROCESS pIsWow64Process;

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
     If it's available on the current system, a pointer to SetConsoleScreenBufferInfoEx.
     */
    PSET_CONSOLE_SCREEN_BUFFER_INFO_EX pSetConsoleScreenBufferInfoEx;

    /**
     If it's available on the current system, a pointer to SetCurrentConsoleFontEx.
     */
    PSET_CURRENT_CONSOLE_FONT_EX pSetCurrentConsoleFontEx;

    /**
     If it's available on the current system, a pointer to SetInformationJobObject.
     */
    PSET_INFORMATION_JOB_OBJECT pSetInformationJobObject;

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
 A prototype for the RevertToSelf function.
 */
typedef
BOOL WINAPI
REVERT_TO_SELF();

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
 A structure containing optional function pointers to advapi32.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_ADVAPI32_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to AccessCheck.
     */
    PACCESS_CHECK pAccessCheck;

    /**
     If it's available on the current system, a pointer to AdjustTokenPrivileges.
     */
    PADJUST_TOKEN_PRIVILEGES pAdjustTokenPrivileges;

    /**
     If it's available on the current system, a pointer to CheckTokenMembership.
     */
    PCHECK_TOKEN_MEMBERSHIP pCheckTokenMembership;

    /**
     If it's available on the current system, a pointer to GetFileSecurityW.
     */
    PGET_FILE_SECURITYW pGetFileSecurityW;

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
     If it's available on the current system, a pointer to RevertToSelf.
     */
    PREVERT_TO_SELF pRevertToSelf;

    /**
     If it's available on the current system, a pointer to SetNamedSecurityInfoW.
     */
    PSET_NAMED_SECURITY_INFOW pSetNamedSecurityInfoW;

} YORI_ADVAPI32_FUNCTIONS, *PYORI_ADVAPI32_FUNCTIONS;

extern YORI_ADVAPI32_FUNCTIONS DllAdvApi32;

/**
 A prototype for the BCryptCloseAlgorithmProvider function.
 */
typedef
LONG WINAPI
BCRYPT_CLOSE_ALGORITHM_PROVIDER(PVOID, DWORD);

/**
 A prototype for a pointer to the BCryptCloseAlgorithmProvider function.
 */
typedef BCRYPT_CLOSE_ALGORITHM_PROVIDER *PBCRYPT_CLOSE_ALGORITHM_PROVIDER;

/**
 A prototype for the BCryptCreateHash function.
 */
typedef
LONG WINAPI
BCRYPT_CREATE_HASH(PVOID, PVOID*, PVOID, DWORD, PVOID, DWORD, DWORD);

/**
 A prototype for a pointer to the BCryptCreateHash function.
 */
typedef BCRYPT_CREATE_HASH *PBCRYPT_CREATE_HASH;

/**
 A prototype for the BCryptDestroyHash function.
 */
typedef
LONG WINAPI
BCRYPT_DESTROY_HASH(PVOID);

/**
 A prototype for a pointer to the BCryptDestroyHash function.
 */
typedef BCRYPT_DESTROY_HASH *PBCRYPT_DESTROY_HASH;

/**
 A prototype for the BCryptFinishHash function.
 */
typedef
LONG WINAPI
BCRYPT_FINISH_HASH(PVOID, PVOID, DWORD, DWORD);

/**
 A prototype for a pointer to the BCryptFinishHash function.
 */
typedef BCRYPT_FINISH_HASH *PBCRYPT_FINISH_HASH;

/**
 A prototype for the BCryptGetProperty function.
 */
typedef
LONG WINAPI
BCRYPT_GET_PROPERTY(PVOID, LPCWSTR, PVOID, DWORD, PDWORD, DWORD);

/**
 A prototype for a pointer to the BCryptGetProperty function.
 */
typedef BCRYPT_GET_PROPERTY *PBCRYPT_GET_PROPERTY;

/**
 A prototype for the BCryptHashData function.
 */
typedef
LONG WINAPI
BCRYPT_HASH_DATA(PVOID, PVOID, DWORD, DWORD);

/**
 A prototype for a pointer to the BCryptHashData function.
 */
typedef BCRYPT_HASH_DATA *PBCRYPT_HASH_DATA;

/**
 A prototype for the BCryptOpenAlgorithmProvider function.
 */
typedef
LONG WINAPI
BCRYPT_OPEN_ALGORITHM_PROVIDER(PVOID*, LPCWSTR, LPCWSTR, DWORD);

/**
 A prototype for a pointer to the BCryptOpenAlgorithmProvider function.
 */
typedef BCRYPT_OPEN_ALGORITHM_PROVIDER *PBCRYPT_OPEN_ALGORITHM_PROVIDER;

/**
 A structure containing optional function pointers to bcrypt.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_BCRYPT_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to BCryptCloseAlgorithmProvider.
     */
    PBCRYPT_CLOSE_ALGORITHM_PROVIDER pBCryptCloseAlgorithmProvider;

    /**
     If it's available on the current system, a pointer to BCryptCreateHash.
     */
    PBCRYPT_CREATE_HASH pBCryptCreateHash;

    /**
     If it's available on the current system, a pointer to BCryptDestroyHash.
     */
    PBCRYPT_DESTROY_HASH pBCryptDestroyHash;

    /**
     If it's available on the current system, a pointer to BCryptFinishHash.
     */
    PBCRYPT_FINISH_HASH pBCryptFinishHash;

    /**
     If it's available on the current system, a pointer to BCryptGetProperty.
     */
    PBCRYPT_GET_PROPERTY pBCryptGetProperty;

    /**
     If it's available on the current system, a pointer to BCryptHashData.
     */
    PBCRYPT_HASH_DATA pBCryptHashData;

    /**
     If it's available on the current system, a pointer to BCryptOpenAlgorithmProvider.
     */
    PBCRYPT_OPEN_ALGORITHM_PROVIDER pBCryptOpenAlgorithmProvider;

} YORI_BCRYPT_FUNCTIONS, *PYORI_BCRYPT_FUNCTIONS;

extern YORI_BCRYPT_FUNCTIONS DllBCrypt;


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
 A prototype for the SHAppBarMessage function.
 */
typedef
DWORD_PTR WINAPI
SH_APP_BAR_MESSAGE(DWORD, PYORI_APPBARDATA);

/**
 A prototype for a pointer to the SHAppBarMessage function.
 */
typedef SH_APP_BAR_MESSAGE *PSH_APP_BAR_MESSAGE;

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
     If it's available on the current system, a pointer to SHAppBarMessage.
     */
    PSH_APP_BAR_MESSAGE pSHAppBarMessage;

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
CLOSE_CLIPBOARD();

/**
 A prototype for a pointer to the CloseClipboard function.
 */
typedef CLOSE_CLIPBOARD *PCLOSE_CLIPBOARD;

/**
 A prototype for the EmptyClipboard function.
 */
typedef
BOOL WINAPI
EMPTY_CLIPBOARD();

/**
 A prototype for a pointer to the EmptyClipboard function.
 */
typedef EMPTY_CLIPBOARD *PEMPTY_CLIPBOARD;

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
 A prototype for the GetDesktopWindow function.
 */
typedef
HWND WINAPI
GET_DESKTOP_WINDOW();

/**
 A prototype for a pointer to the GetDesktopWindow function.
 */
typedef GET_DESKTOP_WINDOW *PGET_DESKTOP_WINDOW;

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
 A prototype for the LockWorkStation function.
 */
typedef
BOOL WINAPI
LOCK_WORKSTATION();

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
OPEN_CLIPBOARD();

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
     If it's available on the current system, a pointer to EmptyClipboard.
     */
    PEMPTY_CLIPBOARD pEmptyClipboard;

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
     If it's available on the current system, a pointer to GetDesktopWindow.
     */
    PGET_DESKTOP_WINDOW pGetDesktopWindow;

    /**
     If it's available on the current system, a pointer to GetWindowRect.
     */
    PGET_WINDOW_RECT pGetWindowRect;

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
     If it's available on the current system, a pointer to SetClipboardData.
     */
    PSET_CLIPBOARD_DATA pSetClipboardData;

    /**
     If it's available on the current system, a pointer to SetForegroundWindow.
     */
    PSET_FOREGROUND_WINDOW pSetForegroundWindow;

    /**
     If it's available on the current system, a pointer to SetWindowTextW.
     */
    PSET_WINDOW_TEXTW pSetWindowTextW;

    /**
     If it's available on the current system, a pointer to ShowWindow.
     */
    PSHOW_WINDOW pShowWindow;

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

} YORI_WTSAPI32_FUNCTIONS, *PYORI_WTSAPI32_FUNCTIONS;

extern YORI_WTSAPI32_FUNCTIONS DllWtsApi32;

// vim:sw=4:ts=4:et:
