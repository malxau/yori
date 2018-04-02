/**
 * @file sdir/sdir.h
 *
 * Colorful, sorted and optionally rich directory enumeration
 * for Windows.
 *
 * Copyright (c) 2014-2018 Malcolm J. Smith
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

//
//  Compile time configuration
//

/**
 The maximum window width to attempt to support.  If the window is larger
 than this value, the program will assume the window is only as large as this
 value.
 */
#define SDIR_MAX_WIDTH   500

/**
 Fallback color for when all else fails.
 */
#define SDIR_DEFAULT_COLOR        (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE)

/**
 Color to display files with that require a newer OS version than the
 one that's currently executing.
 */
#define SDIR_FUTURE_VERSION_COLOR (FOREGROUND_RED|FOREGROUND_INTENSITY)

//
//  Hacks for older compilers
//

#ifdef _ULONGLONG_

/**
 If the compiler has unsigned 64 bit values, use them for file size.
 */
typedef ULONGLONG SDIR_FILESIZE;

#elif !defined(_MSC_VER) || _MSC_VER >= 900

/**
 If the compiler has only has signed 64 bit values, use them for file size.
 */
typedef LONGLONG SDIR_FILESIZE;
#else
#error "Compiler does not have 64 bit support"
#endif

/**
 Macro to return the file size from a large integer.
 */
#define SdirFileSizeFromLargeInt(Li) ((Li)->QuadPart)

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

//
//  File size lengths
//

/**
 The maximum stream name length in characters.  This is from
 WIN32_FIND_STREAM_DATA.
 */
#define SDIR_MAX_STREAM_NAME          (MAX_PATH + 36)

/**
 The maximum file name size in characters, exclusive of path.  This is from
 WIN32_FIND_DATA.
 */
#define SDIR_MAX_FILE_NAME            (MAX_PATH)

/**
 The maximum size of a file name followed by a stream name in characters.
 */
#define SDIR_MAX_FILE_AND_STREAM_NAME (SDIR_MAX_FILE_NAME + SDIR_MAX_STREAM_NAME)

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
    LARGE_INTEGER  Usn;

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

} SDIR_USN_RECORD;

/**
 A pointer to a USN record, provided for environments where the compiler
 doesn't define it.
 */
typedef SDIR_USN_RECORD *PSDIR_USN_RECORD;

/**
 A structure returned by FindFirstStreamNameW.  Supplied for when the
 compilation environment doesn't provide it.
 */
typedef struct _SDIR_WIN32_FIND_STREAM_DATA {

    /**
     The length of the stream, in bytes.
     */
    LARGE_INTEGER StreamSize;

    /**
     The stream name in a NULL terminated string.
     */
    WCHAR cStreamName[SDIR_MAX_STREAM_NAME];
} SDIR_WIN32_FIND_STREAM_DATA, *PSDIR_WIN32_FIND_STREAM_DATA;

/**
 A mapping structure between file attributes and letters to display.
 */
typedef struct _SDIR_ATTRPAIR {

    /**
     The file attribute in native representation.
     */
    DWORD FileAttribute;

    /**
     The character to display to the user.
     */
    TCHAR DisplayLetter;

    /**
     Unused in order to ensure structure alignment.
     */
    WORD  AlignmentPadding;
} SDIR_ATTRPAIR, *PSDIR_ATTRPAIR;

extern const SDIR_ATTRPAIR AttrPairs[];

#pragma pack(push, 1)

/**
 Specifies a character that contains color information.  Typically used in
 arrays to create strings with color information.
 */
typedef struct _SDIR_FMTCHAR {

    /**
     The character.
     */
    TCHAR Char;

    /**
     The color information.
     */
    YORILIB_COLOR_ATTRIBUTES Attr;
} SDIR_FMTCHAR, *PSDIR_FMTCHAR;
#pragma pack(pop)

/**
 Specifies a value indicating that the background and foreground colors
 should be swapped.
 */
#define SDIR_ATTRCTRL_INVERT           0x1

/**
 Specifies a value indicating that the object should not be displayed at all.
 */
#define SDIR_ATTRCTRL_HIDE             0x2

/**
 Specifies a value indicating that the search should continue along more
 criteria to determine the color.
 */
#define SDIR_ATTRCTRL_CONTINUE         0x4

/**
 Specifies a value indicating that the file's color should be used (for a
 metadata attribute.)
 */
#define SDIR_ATTRCTRL_FILE             0x8

/**
 Specifies a value indicating that the window's current background color
 should be used.
 */
#define SDIR_ATTRCTRL_WINDOW_BG        0x10

/**
 Specifies a value indicating that the window's current foreground color
 should be used.
 */
#define SDIR_ATTRCTRL_WINDOW_FG        0x20


/**
 If any bits here are set, the requested color will be rejected.
 */
#define SDIR_ATTRCTRL_INVALID_METADATA ~(SDIR_ATTRCTRL_FILE|SDIR_ATTRCTRL_WINDOW_BG|SDIR_ATTRCTRL_WINDOW_FG)

/**
 Any bits set here will be ignored when applying color information into
 files.
 */
#define SDIR_ATTRCTRL_INVALID_FILE     ~(SDIR_ATTRCTRL_INVERT|SDIR_ATTRCTRL_HIDE|SDIR_ATTRCTRL_CONTINUE|SDIR_ATTRCTRL_WINDOW_BG|SDIR_ATTRCTRL_WINDOW_FG)

/**
 If any of the flags are set here (or if a color is specified) no further
 processing is performed to find color information.
 */
#define SDIR_ATTRCTRL_TERMINATE_MASK   (SDIR_ATTRCTRL_HIDE)

/**
 Specifies the set of bits that correspond to a full color, comprising of both
 a foreground and background.
 */
#define SDIR_ATTRIBUTE_FULLCOLOR_MASK   0xFF

/**
 Specifies the set of bits that correspond to a single color.  After applying
 shifts, this could refer to either a foreground and background color.
 */
#define SDIR_ATTRIBUTE_ONECOLOR_MASK    0x0F

/**
 The set of compression algorithms known to this program.  The order
 corresponds to sort order.
 */
enum {
    SdirCompressionNone = 0,
    SdirCompressionWofFileUnknown,
    SdirCompressionLznt,
    SdirCompressionLzx,
    SdirCompressionNtfsUnknown,
    SdirCompressionWim,
    SdirCompressionWofUnknown,
    SdirCompressionXpress4k,
    SdirCompressionXpress8k,
    SdirCompressionXpress16k
} SdirCompressionAlgorithms;

/**
 Information about a single file.  This is typically only partially populated
 depending on the information of interest to the user.
 */
typedef struct _SDIR_DIRENT {

    /**
     The file system's identifier for the file.
     */
    LARGE_INTEGER FileId;

    /**
     The size of the file when rounded up to the next file system allocation
     unit.
     */
    LARGE_INTEGER AllocationSize;

    /**
     The logical length of the file.
     */
    LARGE_INTEGER FileSize;

    /**
     The amount of space on disk used by the file.
     */
    LARGE_INTEGER CompressedFileSize;

    /**
     The file's USN.
     */
    LARGE_INTEGER Usn;

    /**
     The executable file version.
     */
    LARGE_INTEGER FileVersion;

    /**
     Flags describing extra information about the executable file version.
     */
    DWORD         FileVersionFlags;

    /**
     The high word of the executable's minimum OS version.
     */
    WORD          OsVersionHigh;

    /**
     The low word of the executable's minimum OS version.
     */
    WORD          OsVersionLow;

    /**
     The time and date of when the file was read.
     */
    SYSTEMTIME    AccessTime;

    /**
     The time and date of when the file was created.
     */
    SYSTEMTIME    CreateTime;

    /**
     The time and date of when the file was last written to.
     */
    SYSTEMTIME    WriteTime;

    /**
     The object ID on the file (this is really a GUID.)
     */
    BYTE          ObjectId[16];

    /**
     The number of hardlinks on the file.
     */
    DWORD         LinkCount;

    /**
     The number of named streams on the file.
     */
    DWORD         StreamCount;

    /**
     The reparse tag attached to the file.
     */
    DWORD         ReparseTag;

    /**
     The file's attributes.
     */
    DWORD         FileAttributes;

    /**
     An integer representation of the compression algorithm used to compress
     the file.
     */
    DWORD         CompressionAlgorithm;

    /**
     The number of fragments used to store the file.
     */
    LARGE_INTEGER FragmentCount;

    /**
     The number of allocated ranges in the file.
     */
    LARGE_INTEGER AllocatedRangeCount;

    /**
     The access that the current user has to the file.
     */
    ACCESS_MASK   EffectivePermissions;

    /**
     The color attributes to use to display the file.
     */
    YORILIB_COLOR_ATTRIBUTES RenderAttributes;

    /**
     The executable's target CPU.
     */
    WORD          Architecture;

    /**
     The executable's target subsystem.
     */
    WORD          Subsystem;

    /**
     A string description of the file's owner.
     */
    TCHAR         Owner[17];

    /**
     The number of characters in the file name.
     */
    DWORD         FileNameLengthInChars;

    /**
     The short file name (8.3 compliant.)
     */
    TCHAR         ShortFileName[14];

    /**
     The file name, possibly including stream information.  This should really
     be SDIR_MAX_FILE_AND_STREAM_NAME characters.
     */
    TCHAR         FileName[SDIR_MAX_FILE_NAME];

    /**
     Pointer to the extension within the file name string.
     */
    TCHAR *       Extension;
} SDIR_DIRENT, *PSDIR_DIRENT;

/**
 Specifies a pointer to a function which can be used to disable Wow64
 redirection.
 */
typedef BOOL (WINAPI * DISABLE_WOW_REDIRECT_FN)(PVOID*);

/**
 Specifies a pointer to a function which can enumerate named streams on a
 file.
 */
typedef HANDLE (WINAPI * FIND_FIRST_STREAM_FN)(LPCTSTR, DWORD, PSDIR_WIN32_FIND_STREAM_DATA, DWORD);

/**
 Specifies a pointer to a function which can enumerate named streams on a
 file.
 */
typedef BOOL (WINAPI * FIND_NEXT_STREAM_FN)(HANDLE, PSDIR_WIN32_FIND_STREAM_DATA);

/**
 Specifies a pointer to a function which can return the compressed file size
 for a file.
 */
typedef DWORD (WINAPI * GET_COMPRESSED_FILE_SIZE_FN)(LPCTSTR, LPDWORD);

/**
 Specifies a pointer to a function which can return disk free space without
 depending on the program providing 64 bit math.
 */
typedef BOOL (WINAPI * GET_DISK_FREE_SPACE_EX_FN)(LPCTSTR, PLARGE_INTEGER, PLARGE_INTEGER, PLARGE_INTEGER);

/**
 Specifies a pointer to a function which can return extra information about
 an opened file.
 */
typedef BOOL (WINAPI * GET_FILE_INFORMATION_BY_HANDLE_EX_FN)(HANDLE, ULONG, PVOID, DWORD);

/**
 Specifies a pointer to a function which can return version information about
 executable files.
 */
typedef BOOL  (WINAPI * GET_FILE_VERSION_INFO_FN)(LPTSTR, DWORD, DWORD, LPVOID);

/**
 Specifies a pointer to a function which can return the size of version
 information about executable files.
 */
typedef DWORD (WINAPI * GET_FILE_VERSION_INFO_SIZE_FN)(LPTSTR, LPDWORD);

/**
 Specifies a pointer to a function which can return individual attributes
 from version information about executable files.
 */
typedef BOOL  (WINAPI * VER_QUERY_VALUE_FN)(const LPVOID, LPTSTR, LPVOID *, PUINT);

/**
 Specifies a pointer to a function which can return the amount of characters
 needed to display a piece of file metadata.
 */
typedef DWORD (* SDIR_METADATA_WIDTH_FN)(PSDIR_FMTCHAR, YORILIB_COLOR_ATTRIBUTES, PSDIR_DIRENT);

/**
 Specifies a pointer to a function which can compare two directory entries
 in some fashion.
 */
typedef DWORD (* SDIR_COMPARE_FN)(PSDIR_DIRENT, PSDIR_DIRENT);

/**
 Specifies a pointer to a function which can collect file information from
 the disk or file system for some particular piece of data.
 */
typedef BOOL (* SDIR_COLLECT_FN)(PSDIR_DIRENT, PWIN32_FIND_DATA, LPCTSTR);

/**
 Specifies a pointer to a function which can generate in memory file
 information from a user provided string.
 */
typedef BOOL (* SDIR_GENERATE_FROM_STRING_FN)(PSDIR_DIRENT, LPCTSTR);


/**
 Information describing how to compare two directory entries and what to do
 in response.
 */
typedef struct _SDIR_COMPARE {

    /**
     Pointer to a comparison function.
     */
    SDIR_COMPARE_FN CompareFn;

    /**
     Can be set to SDIR_EQUAL, SDIR_GREATER_THAN, SDIR_LESS_THAN.  Items are
     inserted into order by comparing against existing entries until this
     condition is met, at which point the item can be inserted into its
     sorted position.
     */
    DWORD           CompareBreakCondition;

    /**
     The inverse condition of BreakCondition above.  Used to check if things
     are already in correct order.
     */
    DWORD           CompareInverseCondition;
} SDIR_COMPARE, *PSDIR_COMPARE;


/**
 Indicates that the value for this feature needs to be collected as part of
 enumerate, typically because it is going to be displayed or is the basis for
 sorted comparisons.
 */
#define SDIR_FEATURE_COLLECT              (0x01)

/**
 Indicates that this feature is currently configured to be displayed as part
 of file metadata.
 */
#define SDIR_FEATURE_DISPLAY              (0x02)

/**
 Indicates that this feature can be displayed as part of file metadata.
 */
#define SDIR_FEATURE_ALLOW_DISPLAY        (0x04)

/**
 Indicates that this feature can be used to compare two dirents and determine
 which to display first.
 */
#define SDIR_FEATURE_ALLOW_SORT           (0x08)

/**
 Indicates that the specified color for this feature cannot be changed.
 */
#define SDIR_FEATURE_FIXED_COLOR          (0x10)

/**
 Indicates that the feature value should be displayed with the same color
 as the file name.
 */
#define SDIR_FEATURE_USE_FILE_COLOR       (0x20)

/**
 Information describing the current configuration for a metadata attribute
 or feature.
 */
typedef struct _SDIR_FEATURE {

    /**
     Extra information describing the configuration and support for this
     feature.
     */
    WORD Flags;

    /**
     The color used to display this metadata attribute.
     */
    YORILIB_COLOR_ATTRIBUTES HighlightColor;
} SDIR_FEATURE, *PSDIR_FEATURE;

/**
 Pointer to a feature that cannot change.
 */
typedef SDIR_FEATURE CONST *PCSDIR_FEATURE;

extern YORILIB_ATTRIBUTE_COLOR_STRING ColorString[];

#pragma pack(push, 4)

/**
 A structure describing volatile, run time state describing program execution.
 */
typedef struct _SDIR_OPTS {

    /**
     Points to a user supplied string indicating file criteria to display
     with different colors.
     */
    LPTSTR          CustomFileColor;

    /**
     The length, in characters, of CustomFileColor.
     */
    DWORD           CustomFileColorLength;


    /**
     Points to a user supplied string indicating file criteria to not
     display.
     */
    LPTSTR          CustomFileFilter;

    /**
     Specifies the length, in characters, of CustomFileFilter.
     */
    DWORD           CustomFileFilterLength;

    /**
     Specifies the root path that should be recursively enumerated, or NULL
     if no recursive enumerate is requested.
     */
    LPTSTR          SubDirWalk;

    /**
     Specifies the name of the directory that is currently being enumerated.
     This can change during execution.
     */
    YORI_STRING     ParentName;

    /**
     Specifies the amount of space that must be used within a subtree before
     it should be displayed in brief recurse mode.
     */
    SDIR_FILESIZE   BriefRecurseSize;

    /**
     Specifies the number of levels of depth that the user wants to display
     in brief recurse mode.
     */
    DWORD           BriefRecurseDepth;

    /**
     Specifies the effective height of the console window, in characters.
     */
    DWORD           ConsoleHeight;

    /**
     Specifies the effective width of the console buffer, in characters.
     Note this can be larger than the window width (implying horizontal
     scroll bars.)
     */
    DWORD           ConsoleBufferWidth;

    /**
     Specifies the effective width of the console window, in characters.
     */
    DWORD           ConsoleWidth;

    /**
     Specifies the total number of characters needed to display file metadata
     (ie., not the file's name.)
     */
    DWORD           MetadataWidth;

    /**
     Specifies the version of the hosting OS, in GetVersion format.
     */
    DWORD           OsVersion;

    /**
     The volatile configuration for a file's last access date.
     */
    SDIR_FEATURE    FtAccessDate;

    /**
     The volatile configuration for a file's last access time.
     */
    SDIR_FEATURE    FtAccessTime;

    /**
     The volatile configuration for a file's allocated range count.
     */
    SDIR_FEATURE    FtAllocatedRangeCount;

    /**
     The volatile configuration for a file's allocation size.
     */
    SDIR_FEATURE    FtAllocationSize;

    /**
     The volatile configuration for an executable's architecture.
     */
    SDIR_FEATURE    FtArch;

    /**
     The volatile configuration for how to display alternate lines in brief
     recurse mode.
     */
    SDIR_FEATURE    FtBriefAlternate;

    /**
     The volatile configuration for the file's compression algorithm.
     */
    SDIR_FEATURE    FtCompressionAlgorithm;

    /**
     The volatile configuration for the file's compressed file size.
     */
    SDIR_FEATURE    FtCompressedFileSize;

    /**
     The volatile configuration for the file's creation date.
     */
    SDIR_FEATURE    FtCreateDate;

    /**
     The volatile configuration for the file's creation time.
     */
    SDIR_FEATURE    FtCreateTime;

    /**
     The volatile configuration for how to display errors.
     */
    SDIR_FEATURE    FtError;

    /**
     The volatile configuration for the access allowed to the file by the
     current user.
     */
    SDIR_FEATURE    FtEffectivePermissions;

    /**
     The volatile configuration for the file's attributes.
     */
    SDIR_FEATURE    FtFileAttributes;

    /**
     The volatile configuration for the file's extension.
     */
    SDIR_FEATURE    FtFileExtension;

    /**
     The volatile configuration for the file's ID.
     */
    SDIR_FEATURE    FtFileId;

    /**
     The volatile configuration for the file's name.
     */
    SDIR_FEATURE    FtFileName;

    /**
     The volatile configuration for the file's size.
     */
    SDIR_FEATURE    FtFileSize;

    /**
     The volatile configuration for the file's fragmentation.
     */
    SDIR_FEATURE    FtFragmentCount;

    /**
     The volatile configuration for the display grid.
     */
    SDIR_FEATURE    FtGrid;

    /**
     The volatile configuration for the hard link count.
     */
    SDIR_FEATURE    FtLinkCount;

    /**
     The volatile configuration for the named streams attached to files.
     */
    SDIR_FEATURE    FtNamedStreams;

    /**
     The volatile configuration for the number of files in a directory.
     */
    SDIR_FEATURE    FtNumberFiles;

    /**
     The volatile configuration for the file's object ID.
     */
    SDIR_FEATURE    FtObjectId;

    /**
     The volatile configuration for the executable's minimum OS version.
     */
    SDIR_FEATURE    FtOsVersion;

    /**
     The volatile configuration for the file's owner.
     */
    SDIR_FEATURE    FtOwner;

    /**
     The volatile configuration for the file's reparse tag.
     */
    SDIR_FEATURE    FtReparseTag;

    /**
     The volatile configuration for the file's short name.
     */
    SDIR_FEATURE    FtShortName;

    /**
     The volatile configuration for the file's stream count.
     */
    SDIR_FEATURE    FtStreamCount;

    /**
     The volatile configuration for the executable's subsystem.
     */
    SDIR_FEATURE    FtSubsystem;

    /**
     The volatile configuration for the directory's summary.
     */
    SDIR_FEATURE    FtSummary;

    /**
     The volatile configuration for the file's USN.
     */
    SDIR_FEATURE    FtUsn;

    /**
     The volatile configuration for the executable version.
     */
    SDIR_FEATURE    FtVersion;

    /**
     The volatile configuration for the last write date.
     */
    SDIR_FEATURE    FtWriteDate;

    /**
     The volatile configuration for the last write time.
     */
    SDIR_FEATURE    FtWriteTime;

    /**
     TRUE if when calculating file usage in directories in brief recurse
     mode, the file size divided by link count should be used rather than
     the raw file size (so that the file size is only counted once per disk.)
     */
    BOOLEAN         EnableAverageLinkSize:1;

    /**
     TRUE if once the screen is full of output, the program should wait for
     the user to press a key before generating more.
     */
    BOOLEAN         EnablePause:1;

    /**
     TRUE if long file names should be heuristically shortened by inserting
     "..." in the middle.  This is TRUE by default.
     */
    BOOLEAN         EnableNameTruncation:1;

    /**
     TRUE if outputting to a console.  This controls whether things like
     pause make sense.
     */
    BOOLEAN         OutputWithConsoleApi:1;

    /**
     Set to TRUE if the user has indicated that they no longer want to
     continue enumeration.
     */
    BOOLEAN         Cancelled:1;

    /**
     TRUE if some error was encountered during enumerate that might result
     in incomplete results.
     */
    BOOLEAN         ErrorsFound:1;

    /**
     TRUE if a recursive enumeration operation should look through symbolic
     links or mount points.
     */
    BOOLEAN         TraverseLinks:1;

    /**
     TRUE if the output device will move to the next line once the current
     line has been filled without an explicit line break.
     */
    BOOLEAN         OutputHasAutoLineWrap:1;

    /**
     TRUE if the output device wants/can support extended characters
     including nice line drawing.
     */
    BOOLEAN         OutputExtendedCharacters:1;

    /**
     The color attributes from when the program was started, that should
     be restored on exit.
     */
    YORILIB_COLOR_ATTRIBUTES PreviousAttributes;

    /**
     Unused, just to restore structure alignment.
     */
    WORD            AlignmentPadding;

    /**
     Specifies the number of populated compare functions in the sort array,
     below.
     */
    DWORD           CurrentSort;

    /**
     Supplies pointers to up to 8 functions to use to compare two dirents
     and criteria to apply to determine which should be displayed first.
     */
    SDIR_COMPARE    Sort[8];

    /**
     Pointer to the FindFirstStream function from kernel32.dll.
     */
    FIND_FIRST_STREAM_FN                 FindFirstStreamW;

    /**
     Pointer to the FindNextStream function from kernel32.dll.
     */
    FIND_NEXT_STREAM_FN                  FindNextStreamW;

    /**
     Pointer to the GetCompressedFileSize function from kernel32.dll.
     */
    GET_COMPRESSED_FILE_SIZE_FN          GetCompressedFileSize;

    /**
     Pointer to the GetDiskFreeSpaceEx function from kernel32.dll.
     */
    GET_DISK_FREE_SPACE_EX_FN            GetDiskFreeSpaceEx;

    /**
     Pointer to the GetFileInformationByHandleEx function from kernel32.dll.
     */
    GET_FILE_INFORMATION_BY_HANDLE_EX_FN GetFileInformationByHandleEx;

    /**
     Pointer to the GetFileVersionInfo function from version.dll.
     */
    GET_FILE_VERSION_INFO_FN             GetFileVersionInfo;

    /**
     Pointer to the GetFileVersionInfoSize function from version.dll.
     */
    GET_FILE_VERSION_INFO_SIZE_FN        GetFileVersionInfoSize;

    /**
     Pointer to the VerQueryValue function from version.dll.
     */
    VER_QUERY_VALUE_FN                   VerQueryValue;

} SDIR_OPTS, *PSDIR_OPTS;
#pragma pack(pop)

/**
 Summary information.  This is used to display the final line after
 enumerating a single directory, and also to calculate changes when
 enumerating across multiple directories in a brief recurse mode.
 */
typedef struct _SDIR_SUMMARY {
    /**
     The total number of files enumerated.
     */
    ULONG           NumFiles;

    /**
     The total number of directories enumerated.
     */
    ULONG           NumDirs;

    /**
     The total amount of bytes used by files enumerated, counting size
     before any compression has been applied.
     */
    SDIR_FILESIZE   TotalSize;

    /**
     The total amount of bytes used by files enumerated, counting size
     after any compression has been applied.
     */
    SDIR_FILESIZE   CompressedSize;

    /**
     The amount of free space on the volume, in bytes.
     */
    LARGE_INTEGER   FreeSize;

    /**
     The size of the volume, in bytes.
     */
    LARGE_INTEGER   VolumeSize;
} SDIR_SUMMARY, *PSDIR_SUMMARY;

#pragma pack(push, 1)

/**
 A single (read only) metadata configuration.
 */
typedef struct _SDIR_OPT {

    /**
     Offset from the top of the SDIR_OPTS structure to the volatile piece of
     memory that controls the current behavior for this feature.
     */
    DWORD                        FtOffset;

    /**
     Indicates the two character NULL terminated command line string that
     is used to describe this feature.
     */
    TCHAR                        Switch[3];

    /**
     Indicates the default color, collection behavior and supported
     functionality for this feature.
     */
    SDIR_FEATURE                 Default;

    /**
     Pointer to a function that indicates how many characters are required
     to display the value from this feature.
     */
    SDIR_METADATA_WIDTH_FN       WidthFn;

    /**
     Pointer to a function that can collect the value for this feature from
     an enumerated file.
     */
    SDIR_COLLECT_FN              CollectFn;

    /**
     Pointer to a function that compares two in memory dirents value for
     this feature using a typical string or integer compare.
     */
    SDIR_COMPARE_FN              CompareFn;

    /**
     Pointer to a function that compares two in memory dirents value for
     this feature, using a bitwise comparison.  For file names, this also
     implements wildcard matching.
     */
    SDIR_COMPARE_FN              BitwiseCompareFn;

    /**
     Pointer to a function which can parse a user string and generate an
     in memory representation for the value of this feature.
     */
    SDIR_GENERATE_FROM_STRING_FN GenerateFromStringFn;

    /**
     A NULL terminated string containing help text to display for this
     feature.
     */
    CHAR                         Help[24];
} SDIR_OPT, *PSDIR_OPT;
#pragma pack(pop)

/**
 For string/integer compares, indicates that the first value is less than the
 second.
 */
#define SDIR_LESS_THAN    0

/**
 For string/integer compares, indicates that the first value is equal to the
 second.
 */
#define SDIR_EQUAL        1

/**
 For string/integer compares, indicates that the first value is greater than
 the second.
 */
#define SDIR_GREATER_THAN 2

/**
 For bitwise or wildcard compares, indicates a mismatch.
 */
#define SDIR_NOT_EQUAL    0

#pragma pack(push, 4)

/**
 A single metadata attribute that should be processed.
 */
typedef struct _SDIR_EXEC {
    /**
     Indicates the offset within the volatile options structure describing
     this metadata attribute.
     */
    DWORD FtOffset;

    /**
     Pointer to a function which can indicate how many characters are needed
     to display this metadata attribute.
     */
    SDIR_METADATA_WIDTH_FN Function;
} SDIR_EXEC, *PSDIR_EXEC;
#pragma pack(pop)

extern PSDIR_OPTS Opts;
extern PSDIR_SUMMARY Summary;
extern const SDIR_OPT SdirOptions[];
extern const SDIR_EXEC SdirExec[];
extern const CHAR SdirAttributeDefaultApplyString[];

//
//  Functions from display.c
//

BOOL
SdirWriteRawStringToOutputDevice(
    __in HANDLE hConsole,
    __in LPCTSTR OutputString,
    __in DWORD Length
    );

BOOL
SdirSetConsoleTextAttribute(
    __in HANDLE hConsole,
    __in YORILIB_COLOR_ATTRIBUTES Attribute
    );

BOOL
SdirWrite (
    __in_ecount(count) PSDIR_FMTCHAR str,
    __in DWORD count
    );

BOOL
SdirPasteStrAndPad (
    __out_ecount(padsize) PSDIR_FMTCHAR str,
    __in_ecount(count) LPTSTR src,
    __in YORILIB_COLOR_ATTRIBUTES attr,
    __in DWORD count,
    __in DWORD padsize
    );

BOOL
SdirPressAnyKey ();

BOOL
SdirRowDisplayed();

BOOL
SdirWriteStringWithAttribute (
    __in LPCTSTR str,
    __in YORILIB_COLOR_ATTRIBUTES DefaultAttribute
    );

BOOL
SdirDisplayError (
    __in LONG ErrorCode,
    __in LPCTSTR Prefix
    );

extern YORILIB_COLOR_ATTRIBUTES SdirDefaultColor;

/**
 Output a string with no extra color information.
 */
#define SdirWriteString(str) SdirWriteStringWithAttribute(str, SdirDefaultColor)

/**
 Output an array of characters with a specified color value into an array
 of formatted characters, with no extra padding.
 */
#define SdirPasteStr(str, src, attr, count) \
    SdirPasteStrAndPad(str, src, attr, count, count)

//
//  Functions from callbacks.c
//

DWORD
SdirGetNumSdirOptions();

DWORD
SdirGetNumSdirExec();

ULONG
SdirDisplaySummary(
    __in YORILIB_COLOR_ATTRIBUTES DefaultAttributes
    );

BOOL
SdirCollectSummary(
    __in PSDIR_DIRENT Entry
    );

ULONG
SdirDisplayShortName (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PSDIR_DIRENT Entry
    );

DWORD
SdirCompareFileName (
    __in PSDIR_DIRENT Left,
    __in PSDIR_DIRENT Right
    );

/**
 Return a pointer to a feature's volatile configuration from its array
 index.
 */
#define SdirFeatureByOptionNumber(OPTNUM) \
    (PSDIR_FEATURE)((PUCHAR)Opts + SdirOptions[(OPTNUM)].FtOffset)

//
//  Functions from color.c
//

BOOL
SdirColorStringFromFeature(
    __in PCSDIR_FEATURE Feature,
    __out LPTSTR String,
    __in DWORD StringSize
    );

BOOL
SdirParseAttributeApplyString();

BOOL
SdirParseMetadataAttributeString();

BOOL
SdirApplyAttribute(
    __in PSDIR_DIRENT DirEnt,
    __out PYORILIB_COLOR_ATTRIBUTES Attribute
    );

//
//  Functions from init.c
//

BOOL
SdirInit(
    __in int argc,
    __in_ecount(argc) LPTSTR argv[]
    );

BOOL
SdirLoadVerFunctions();

//
//  Functions from sdir.c
//

DWORD
SdirGetNumAttrPairs();

//
//  Functions from usage.c
//

BOOL
SdirUsage(
    __in int argc,
    __in_ecount(argc) LPTSTR argv[]
    );

//
//  Functions from utils.c
//

DWORD
SdirCompareLargeInt (
    __in PULARGE_INTEGER Left,
    __in PULARGE_INTEGER Right
    );

DWORD
SdirCompareString (
    __in LPCTSTR Left,
    __in LPCTSTR Right
    );

DWORD
SdirCompareDate (
    __in LPSYSTEMTIME Left,
    __in LPSYSTEMTIME Right
    );

DWORD
SdirCompareTime (
    __in LPSYSTEMTIME Left,
    __in LPSYSTEMTIME Right
    );

DWORD
SdirStringToNum32(
    __in LPCTSTR str,
    __out_opt LPCTSTR* endptr
    );

LARGE_INTEGER
SdirStringToNum64(
    __in LPCTSTR str,
    __out_opt LPCTSTR* endptr
    );

BOOL
SdirStringToHexBuffer(
    __in LPCTSTR str,
    __out PUCHAR buffer,
    __in DWORD size
    );

LARGE_INTEGER
SdirStringToFileSize(
    __in LPCTSTR str
    );

BOOL
SdirStringToDate(
    __in LPCTSTR str,
    __out LPSYSTEMTIME date
    );

BOOL
SdirStringToTime(
    __in LPCTSTR str,
    __out LPSYSTEMTIME date
    );

BOOL
SdirCopyFileName(
    __out LPTSTR Dest,
    __in LPCTSTR Src,
    __in DWORD MaxLength,
    __out_opt PDWORD ValidCharCount
    );

ULONG
SdirDisplayGenericSize(
    __out_ecount(6) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PLARGE_INTEGER GenericSize
    );

ULONG
SdirDisplayHex64 (
    __out_ecount(18) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PLARGE_INTEGER Hex
    );

ULONG
SdirDisplayHex32 (
    __out_ecount(9) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in DWORD Hex
    );

ULONG
SdirDisplayGenericHexBuffer (
    __out_ecount(Size * 2 + 1) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PUCHAR InputBuffer,
    __in DWORD Size
    );

ULONG
SdirDisplayFileDate (
    __out_ecount(11) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in SYSTEMTIME * Time
    );

ULONG
SdirDisplayFileTime (
    __out_ecount(9) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in SYSTEMTIME * Time
    );

/**
 Convert an 8 bit string into Unicode characters.
 */
#define SdirAnsiToUnicode(u, l, a) YoriLibSPrintfS(u, l, _T("%hs"), a)

LARGE_INTEGER
SdirMultiplyViaShift(
    __in LARGE_INTEGER OriginalValue,
    __in DWORD MultiplyBy
    );

BOOL
SdirPopulateSummaryWithGetDiskFreeSpace(
    __in LPTSTR Path,
    __inout PSDIR_SUMMARY Summary
    );

// vim:sw=4:ts=4:et:
