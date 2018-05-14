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

#ifndef FSCTL_GET_EXTERNAL_BACKING

/**
 Specifies the FSCTL_GET_EXTERNAL_BACKING numerical representation if the
 compilation environment doesn't provide it.
 */
#define FSCTL_GET_EXTERNAL_BACKING       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 196, METHOD_BUFFERED, FILE_ANY_ACCESS)
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
GET_FILE_INFORMATION_BY_HANDLE_EX(HANDLE, ULONG, PVOID, DWORD);

/**
 A prototype for a pointer to the GetFileInformationByHandleEx function.
 */
typedef GET_FILE_INFORMATION_BY_HANDLE_EX *PGET_FILE_INFORMATION_BY_HANDLE_EX;

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
 A structure containing optional function pointers to kernel32.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_KERNEL32_FUNCTIONS {

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
     If it's available on the current system, a pointer to FindNextStreamW.
     */
    PFIND_NEXT_STREAMW pFindNextStreamW;

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
} YORI_KERNEL32_FUNCTIONS, *PYORI_KERNEL32_FUNCTIONS;

extern YORI_KERNEL32_FUNCTIONS Kernel32;

// vim:sw=4:ts=4:et:
