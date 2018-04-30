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
 A prototype for the GetConsoleScreenBufferEx function.
 */
typedef BOOL WINAPI GET_CONSOLE_SCREEN_BUFFER_INFO_EX(HANDLE, PYORI_CONSOLE_SCREEN_BUFFER_INFOEX);

/**
 A prototype for a pointer to the GetConsoleScreenBufferEx function.
 */
typedef GET_CONSOLE_SCREEN_BUFFER_INFO_EX *PGET_CONSOLE_SCREEN_BUFFER_INFO_EX;

/**
 A prototype for the GetCurrentConsoleFontEx function.
 */
typedef
BOOL WINAPI GET_CURRENT_CONSOLE_FONT_EX(HANDLE, BOOL, PYORI_CONSOLE_FONT_INFOEX);

/**
 A prototype for a pointer to the GetCurrentConsoleFontEx function.
 */
typedef GET_CURRENT_CONSOLE_FONT_EX *PGET_CURRENT_CONSOLE_FONT_EX;

/**
 A prototype for the RegisterApplicationRestart function.
 */
typedef LONG WINAPI REGISTER_APPLICATION_RESTART(LPCWSTR, DWORD);

/**
 A prototype for a pointer to the RegisterApplicationRestart function.
 */
typedef REGISTER_APPLICATION_RESTART *PREGISTER_APPLICATION_RESTART;

/**
 A prototype for the SetConsoleScreenBufferEx function.
 */
typedef BOOL WINAPI SET_CONSOLE_SCREEN_BUFFER_INFO_EX(HANDLE, PYORI_CONSOLE_SCREEN_BUFFER_INFOEX);

/**
 A prototype for a pointer to the the SetConsoleScreenBufferEx function.
 */
typedef SET_CONSOLE_SCREEN_BUFFER_INFO_EX *PSET_CONSOLE_SCREEN_BUFFER_INFO_EX;

/**
 A prototype for the SetCurrentConsoleFontEx function.
 */
typedef
BOOL WINAPI SET_CURRENT_CONSOLE_FONT_EX(HANDLE, BOOL, PYORI_CONSOLE_FONT_INFOEX);

/**
 A prototype for a pointer to the SetCurrentConsoleFontEx function.
 */
typedef SET_CURRENT_CONSOLE_FONT_EX *PSET_CURRENT_CONSOLE_FONT_EX;



// vim:sw=4:ts=4:et:
