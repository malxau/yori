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
#ifndef CSIDL_LOCALAPPDATA
/**
 The identifier of the AppData local directory to ShGetSpecialFolderPath.
 */
#define CSIDL_LOCALAPPDATA 0x001c
#endif
#ifndef CSIDL_DESKTOPDIRECTORY
/**
 The identifier of the Desktop directory to ShGetSpecialFolderPath.
 */
#define CSIDL_DESKTOPDIRECTORY 0x0010
#endif
#ifndef CSIDL_PERSONAL
/**
 The identifier of the Documents directory to ShGetSpecialFolderPath.
 */
#define CSIDL_PERSONAL 0x0005
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
     If it's available on the current system, a pointer to CoTaskMemFree.
     */
    PCO_TASK_MEM_FREE pCoTaskMemFree;
} YORI_OLE32_FUNCTIONS, *PYORI_OLE32_FUNCTIONS;

extern YORI_OLE32_FUNCTIONS Ole32;

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
     If it's available on the current system, a pointer to SHFileOperationW.
     */
    PSH_FILE_OPERATIONW pSHFileOperationW;

    /**
     If it's available on the current system, a pointer to SHGetKnownFolderPath.
     */
    PSH_GET_KNOWN_FOLDER_PATH pSHGetKnownFolderPath;

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

extern YORI_SHELL32_FUNCTIONS Shell32;

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
 A structure containing optional function pointers to user32.dll exported
 functions which programs can operate without having hard dependencies on.
 */
typedef struct _YORI_USER32_FUNCTIONS {
    /**
     A handle to the Dll module.
     */
    HINSTANCE hDll;

    /**
     If it's available on the current system, a pointer to CloseClipboard.
     */
    PCLOSE_CLIPBOARD pCloseClipboard;

    /**
     If it's available on the current system, a pointer to GetClipboardData.
     */
    PGET_CLIPBOARD_DATA pGetClipboardData;

    /**
     If it's available on the current system, a pointer to OpenClipboard.
     */
    POPEN_CLIPBOARD pOpenClipboard;
} YORI_USER32_FUNCTIONS, *PYORI_USER32_FUNCTIONS;

extern YORI_USER32_FUNCTIONS User32;

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

extern YORI_VERSION_FUNCTIONS Version;



// vim:sw=4:ts=4:et:
