/**
 * @file lib/yorilib.h
 *
 * Header for library routines that may be of value from the shell as well
 * as external tools.
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

/**
 A yori list entry structure.
 */
typedef struct _YORI_LIST_ENTRY {

    /**
     Pointer to the next entry.  If the list is empty, this may point
     to itself.
     */
    struct _YORI_LIST_ENTRY * Next;

    /**
     Pointer to the previous entry.  If the list is empty, this may
     point to itself.
     */
    struct _YORI_LIST_ENTRY * Prev;
} YORI_LIST_ENTRY, *PYORI_LIST_ENTRY;

/**
 A string type that is counted and can be used to describe part of another
 allocation.
 */
typedef struct _YORI_STRING {

    /**
     The referenced allocation backing the string.  May be NULL.
     */
    PVOID MemoryToFree;

    /**
     Pointer to the beginning of the string.
     */
    LPTSTR StartOfString;

    /**
     The number of characters currently in the string.
     */
    DWORD LengthInChars;

    /**
     The maximum number of characters that could be put into this allocation.
     */
    DWORD LengthAllocated;
} YORI_STRING, *PYORI_STRING;

/**
 A structure describing an entry that is an element of a hash table.
 */
typedef struct _YORI_HASH_ENTRY {

    /**
     The links of this entry within the hash bucket.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     A string that represents the key for the object within the table.
     */
    YORI_STRING Key;

    /**
     An opaque context block that can be used by the user of the hash
     table to identify the entry.
     */
    PVOID Context;
} YORI_HASH_ENTRY, *PYORI_HASH_ENTRY;

/**
 A structure describing a hash bucket in a hash table.
 */
typedef struct _YORI_HASH_BUCKET {

    /**
     A list of matching entries within the hash bucket.
     */
    YORI_LIST_ENTRY ListHead;
} YORI_HASH_BUCKET, *PYORI_HASH_BUCKET;

/**
 A structure describing a hash table.
 */
typedef struct _YORI_HASH_TABLE {

    /**
     The number of buckets in the hash table.
     */
    DWORD NumberBuckets;

    /**
     An array of hash buckets.
     */
    PYORI_HASH_BUCKET Buckets;
} YORI_HASH_TABLE, *PYORI_HASH_TABLE;

#pragma pack(push, 1)

/**
 A structure defining Win32 color information combined with extra information
 describing things other than explicit colors like transparent, inversion,
 etc.
 */
typedef struct _YORILIB_COLOR_ATTRIBUTES {

    /**
     Extra information specifying how to determine the correct color.
     */
    UCHAR Ctrl;

    /**
     An explicitly specified color.
     */
    UCHAR Win32Attr;
} YORILIB_COLOR_ATTRIBUTES, *PYORILIB_COLOR_ATTRIBUTES;

/**
 A single entry mapping a string into a color.
 */
typedef struct _YORILIB_ATTRIBUTE_COLOR_STRING {

    /**
     The string to match against.
     */
    LPTSTR String;

    /**
     The color to use.
     */
    YORILIB_COLOR_ATTRIBUTES Attr;
} YORILIB_ATTRIBUTE_COLOR_STRING, *PYORILIB_ATTRIBUTE_COLOR_STRING;

#pragma pack(pop)

/**
 The set of compression algorithms known to this program.  The order
 corresponds to sort order.
 */
enum {
    YoriLibCompressionNone = 0,
    YoriLibCompressionWofFileUnknown,
    YoriLibCompressionLznt,
    YoriLibCompressionLzx,
    YoriLibCompressionNtfsUnknown,
    YoriLibCompressionWim,
    YoriLibCompressionWofUnknown,
    YoriLibCompressionXpress4k,
    YoriLibCompressionXpress8k,
    YoriLibCompressionXpress16k
} YoriLibCompressionAlgorithms;

/**
 Information about a single file.  This is typically only partially populated
 depending on the information of interest to the user.  Note this is expected
 to change over time and should not be used across any dynamic interface.
 */
typedef struct _YORI_FILE_INFO {

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
     A string containing the file description string text from the version
     resource.
     */
    TCHAR         Description[65];

    /**
     A string containing the file version string text from the version
     resource.
     */
    TCHAR         FileVersionString[33];

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
     be MAX_FILE_AND_STREAM_NAME characters.  Note this refers to file name
     only, not path.
     */
    TCHAR         FileName[MAX_PATH];

    /**
     Pointer to the extension within the file name string.
     */
    TCHAR *       Extension;
} YORI_FILE_INFO, *PYORI_FILE_INFO;

/**
 Adds a specified number of bytes to a pointer value and returns the
 added value.
 */
#define YoriLibAddToPointer(PTR, OFFSET) \
    (((PUCHAR)(PTR)) + (OFFSET))


#ifdef _M_IX86

/**
 On x86 systems, use __stdcall for builtins.
 */
#define YORI_BUILTIN_FN __stdcall

#else

/**
 On non-x86 architectures, use the default calling convention.
 */
#define YORI_BUILTIN_FN

#endif

/**
 Function prototype for a builtin entrypoint.
 */
typedef
DWORD
YORI_BUILTIN_FN
YORI_CMD_BUILTIN (
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    );

/**
 Pointer to a builtin entrypoint function.
 */
typedef YORI_CMD_BUILTIN *PYORI_CMD_BUILTIN;

// *** CANCEL.C ***

BOOL
YoriLibCancelEnable();

BOOL
YoriLibCancelDisable();

BOOL
YoriLibCancelIgnore();

BOOL
YoriLibIsOperationCancelled();

VOID
YoriLibCancelReset();

HANDLE
YoriLibCancelGetEvent();

// *** CLIP.C ***

BOOL
YoriLibBuildHtmlClipboardBuffer(
    __in PYORI_STRING TextToCopy,
    __out PHANDLE HandleForClipboard
    );

// *** CMDLINE.C ***

BOOL
YoriLibIsCommandLineOptionChar(
    __in TCHAR Char
    );

BOOL
YoriLibIsCommandLineOption(
    __in PYORI_STRING String,
    __out PYORI_STRING Arg
    );

BOOL
YoriLibCheckIfArgNeedsQuotes(
    __in PYORI_STRING Arg
    );

BOOL
YoriLibBuildCmdlineFromArgcArgv(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[],
    __in BOOLEAN EncloseInQuotes,
    __out PYORI_STRING CmdLine
    );

/**
 A prototype for a callback function to invoke for variable expansion.
 */
typedef DWORD YORILIB_VARIABLE_EXPAND_FN(PYORI_STRING OutputBuffer, PYORI_STRING VariableName, PVOID Context);

/**
 A pointer to a callback function to invoke for variable expansion.
 */
typedef YORILIB_VARIABLE_EXPAND_FN *PYORILIB_VARIABLE_EXPAND_FN;

BOOL
YoriLibExpandCommandVariables(
    __in PYORI_STRING String,
    __in TCHAR MatchChar,
    __in BOOLEAN PreserveEscapes,
    __in PYORILIB_VARIABLE_EXPAND_FN Function,
    __in PVOID Context,
    __inout PYORI_STRING ExpandedString
    );

// *** COLOR.C ***


extern YORILIB_ATTRIBUTE_COLOR_STRING ColorString[];

/**
 A flag defining inversion in the Ctrl member of YORILIB_COLOR_ATTRIBUTES.
 */
#define YORILIB_ATTRCTRL_INVERT           0x1

/**
 A flag defining hiding in the Ctrl member of YORILIB_COLOR_ATTRIBUTES.
 */
#define YORILIB_ATTRCTRL_HIDE             0x2

/**
 A flag defining an instruction to continue looking for another color in the
 Ctrl member of YORILIB_COLOR_ATTRIBUTES.
 */
#define YORILIB_ATTRCTRL_CONTINUE         0x4

/**
 A flag defining an instruction to apply the file's color in the Ctrl member
 of YORILIB_COLOR_ATTRIBUTES.
 */
#define YORILIB_ATTRCTRL_FILE             0x8

/**
 A flag indicating use of the current window background in the Ctrl member
 of YORILIB_COLOR_ATTRIBTUES.
 */
#define YORILIB_ATTRCTRL_WINDOW_BG        0x10

/**
 A flag indicating use of the current window foreground in the Ctrl member
 of YORILIB_COLOR_ATTRIBUTES.
 */
#define YORILIB_ATTRCTRL_WINDOW_FG        0x20

/**
 A mask that represents all possible colors in the Win32Attr member
 of YORILIB_COLOR_ATTRIBUTES, consisting of both a background and a foreground.
 */
#define YORILIB_ATTRIBUTE_FULLCOLOR_MASK   0xFF

/**
 A mask that represents a single color, either a background or foreground, in
 the Win32Attr member of YORILIB_COLOR_ATTRIBUTES.
 */
#define YORILIB_ATTRIBUTE_ONECOLOR_MASK    0x0F

YORILIB_COLOR_ATTRIBUTES
YoriLibAttributeFromString(
    __in LPCTSTR String
    );

BOOL
YoriLibSetColorToWin32(
    __out PYORILIB_COLOR_ATTRIBUTES Attributes,
    __in UCHAR Win32Attribute
    );

YORILIB_COLOR_ATTRIBUTES
YoriLibCombineColors(
    __in YORILIB_COLOR_ATTRIBUTES Color1,
    __in YORILIB_COLOR_ATTRIBUTES Color2
    );

YORILIB_COLOR_ATTRIBUTES
YoriLibResolveWindowColorComponents(
    __in YORILIB_COLOR_ATTRIBUTES Color,
    __in YORILIB_COLOR_ATTRIBUTES WindowColor,
    __in BOOL RetainWindowCtrlFlags
    );

BOOL
YoriLibAreColorsIdentical(
    __in YORILIB_COLOR_ATTRIBUTES Color1,
    __in YORILIB_COLOR_ATTRIBUTES Color2
    );

// *** CSHOT.C ***

BOOL
YoriLibRewriteConsoleContents(
    __in HANDLE hTarget,
    __in DWORD LineCount,
    __in DWORD SkipCount
    );

BOOL
YoriLibGenerateVtStringFromConsoleBuffers(
    __inout PYORI_STRING String,
    __in COORD BufferSize,
    __in LPWSTR CharBuffer,
    __in PWORD AttrBuffer
    );

// *** CVTHTML.C ***

BOOL
YoriLibHtmlGenerateInitialString(
    __inout PYORI_STRING TextString
    );

BOOL
YoriLibHtmlGenerateEndString(
    __inout PYORI_STRING TextString
    );

BOOL
YoriLibHtmlGenerateTextString(
    __inout PYORI_STRING TextString,
    __out PDWORD BufferSizeNeeded,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    );

BOOL
YoriLibHtmlGenerateEscapeString(
    __inout PYORI_STRING TextString,
    __out PDWORD BufferSizeNeeded,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    );

BOOL
YoriLibHtmlSetVersion(
    __in DWORD HtmlVersion
    );

BOOL
YoriLibHtmlConvertToHtmlFromVt(
    __in PYORI_STRING VtText,
    __inout PYORI_STRING HtmlText,
    __in DWORD HtmlVersion
    );

// *** DEBUG.C ***


#ifndef __FUNCTION__
/**
 Older compilers don't automatically generate function names, so this stubs
 it out.  Newer compilers will define this for themselves.
 */
#define __FUNCTION__ ""
#endif

VOID
YoriLibDbgRealAssert(
    __in LPCSTR Condition,
    __in LPCSTR Function,
    __in LPCSTR File,
    __in DWORD Line
    );

#if DBG
/**
 Break into the debugger on debug builds if the specified check fails.
 */
#define ASSERT(x) \
    if (!(x)) {     \
        YoriLibDbgRealAssert( #x, __FUNCTION__, __FILE__, __LINE__); \
    }
#else
/**
 On non debug builds, do nothing if the check fails.
 */
#define ASSERT(x)
#endif

// *** DYLD.C ***

BOOL
YoriLibLoadNtDllFunctions();

BOOL
YoriLibLoadKernel32Functions();

BOOL
YoriLibLoadAdvApi32Functions();

BOOL
YoriLibLoadOle32Functions();

BOOL
YoriLibLoadShell32Functions();

BOOL
YoriLibLoadUser32Functions();

BOOL
YoriLibLoadVersionFunctions();

// *** FILEENUM.C ***

/**
 A prototype for a callback function to invoke for each matching file.
 */
typedef BOOL YORILIB_FILE_ENUM_FN(PYORI_STRING FileName, PWIN32_FIND_DATA FileInfo, DWORD Depth, PVOID Context);

/**
 A pointer to a callback function to invoke for each matching file.
 */
typedef YORILIB_FILE_ENUM_FN *PYORILIB_FILE_ENUM_FN;

/**
 Indicates the enumeration callback should be invoked for each file found.
 */
#define YORILIB_FILEENUM_RETURN_FILES            0x00000001

/**
 Indicates the enumeration callback should be invoked for each directory
 found.
 */
#define YORILIB_FILEENUM_RETURN_DIRECTORIES      0x00000002

/**
 Indicates any child directories should be traversed after returning all
 results from a given directory.
 */
#define YORILIB_FILEENUM_RECURSE_AFTER_RETURN    0x00000004

/**
 Indicates any child directories should be traversed before returning all
 results from a given directory.
 */
#define YORILIB_FILEENUM_RECURSE_BEFORE_RETURN   0x00000008

/**
 Indicates that when traversing a directory heirarchy, only files matching
 the specified enumeration criteria should be returned.  If not specified,
 all child objects will be returned.
 */
#define YORILIB_FILEENUM_RECURSE_PRESERVE_WILD   0x00000010

/**
 Indicates that basic file name expansion should be used only.
 */
#define YORILIB_FILEENUM_BASIC_EXPANSION         0x00000020

/**
 Indicates that symbolic links and mount points should not be traversed
 when recursing.
 */
#define YORILIB_FILEENUM_NO_LINK_TRAVERSE        0x00000040

/**
 Include dot and dotdot files during enumerate.
 */
#define YORILIB_FILEENUM_INCLUDE_DOTFILES        0x00000080

/**
 If the top level object is a directory, enumerate its children without
 requiring explicit wildcards.
 */
#define YORILIB_FILEENUM_DIRECTORY_CONTENTS      0x00000100

BOOL
YoriLibForEachFile(
    __in PYORI_STRING FileSpec,
    __in DWORD MatchFlags,
    __in DWORD Depth,
    __in PYORILIB_FILE_ENUM_FN Callback,
    __in PVOID Context
    );

BOOL
YoriLibDoesFileMatchExpression (
    __in PYORI_STRING FileName,
    __in PYORI_STRING Wildcard
    );

// *** FILEINFO.C ***

BOOL
YoriLibIsExecutableGui(
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectAccessTime (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectAllocatedRangeCount (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectAllocationSize (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectArch (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectCompressionAlgorithm (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectCompressedFileSize (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectCreateTime (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectDescription (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectEffectivePermissions (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectFileAttributes (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectFileExtension (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectFileId (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectFileName (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectFileSize (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectFileVersionString (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectFragmentCount (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectLinkCount (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectObjectId (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectOsVersion (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectOwner (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectReparseTag (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectShortName (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectSubsystem (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectStreamCount (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectUsn (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectVersion (
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibCollectWriteTime(
    __inout PYORI_FILE_INFO Entry,
    __in PWIN32_FIND_DATA FindData,
    __in PYORI_STRING FullPath
    );

BOOL
YoriLibGenerateAccessDate(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateAccessTime(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateAllocatedRangeCount(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateAllocationSize(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateArch(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateCompressionAlgorithm(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateCompressedFileSize(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateCreateDate(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateCreateTime(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateDescription(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateFileExtension(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateFileName(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateFileSize(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateFileVersionString(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateFragmentCount(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateLinkCount(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateObjectId(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateOsVersion(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateOwner(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateReparseTag(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateShortName(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateSubsystem (
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateStreamCount(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateUsn(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateVersion(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateWriteDate(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateWriteTime(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

// *** FULLPATH.C ***

/**
 Resolves to TRUE if the specified character is a path component seperator.
 */
#define YoriLibIsSep(x) ((x) == '\\' || (x) == '/')

BOOL
YoriLibGetCurrentDirectoryOnDrive(
    __in TCHAR Drive,
    __inout PYORI_STRING DriveCurrentDirectory
    );

BOOL
YoriLibSetCurrentDirectoryOnDrive(
    __in TCHAR Drive,
    __in PYORI_STRING DriveCurrentDirectory
    );

BOOL
YoriLibIsDriveLetterWithColon(
    __in PYORI_STRING Path
    );

BOOL
YoriLibIsDriveLetterWithColonAndSlash(
    __in PYORI_STRING Path
    );

BOOL
YoriLibIsFullPathUnc(
    __in PYORI_STRING Path
    );

BOOL
YoriLibGetFullPathNameReturnAllocation(
    __in PYORI_STRING FileName,
    __in BOOL bReturnEscapedPath,
    __inout PYORI_STRING Buffer,
    __deref_opt_out LPTSTR* lpFilePart
    );

BOOL
YoriLibExpandHomeDirectories(
    __in PYORI_STRING FileString,
    __inout PYORI_STRING ExpandedString
    );

BOOL
YoriLibUserStringToSingleFilePath(
    __in PYORI_STRING UserString,
    __in BOOL bReturnEscapedPath,
    __inout PYORI_STRING FullPath
    );

BOOL
YoriLibUnescapePath(
    __in PYORI_STRING Path,
    __inout PYORI_STRING UnescapedPath
    );

// *** HASH.C ***

PYORI_HASH_TABLE
YoriLibAllocateHashTable(
    __in DWORD NumberBuckets
    );

VOID
YoriLibFreeEmptyHashTable(
    __in PYORI_HASH_TABLE HashTable
    );

VOID
YoriLibHashInsertByKey(
    __in PYORI_HASH_TABLE HashTable,
    __in PYORI_STRING KeyString,
    __in PVOID Context,
    __out PYORI_HASH_ENTRY HashEntry
    );

PYORI_HASH_ENTRY
YoriLibHashLookupByKey(
    __in PYORI_HASH_TABLE HashTable,
    __in PYORI_STRING KeyString
    );

VOID
YoriLibHashRemoveByEntry(
    __in PYORI_HASH_ENTRY HashEntry
    );

PYORI_HASH_ENTRY
YoriLibHashRemoveByKey(
    __in PYORI_HASH_TABLE HashTable,
    __in PYORI_STRING KeyString
    );

// *** HEXDUMP.C ***

BOOL
YoriLibHexDump(
    __in LPCSTR Buffer,
    __in DWORD BufferLength,
    __in DWORD BytesPerWord,
    __in DWORD DumpFlags
    );

// *** ICONV.C ***

#ifndef CP_UTF16

/**
 A code to describe UTF16 encoding.  This encoding is implemented within this
 library.
 */
#define CP_UTF16 0xFEFF
#endif

#ifndef CP_UTF8

/**
 A code to describe UTF8 encoding, if the compiler doesn't already have it.
 */
#define CP_UTF8 65001
#endif

DWORD
YoriLibGetMultibyteOutputEncoding();

DWORD
YoriLibGetMultibyteInputEncoding();

VOID
YoriLibSetMultibyteOutputEncoding(
    __in DWORD Encoding
    );

VOID 
YoriLibSetMultibyteInputEncoding(
    __in DWORD Encoding
    );

DWORD
YoriLibGetMultibyteOutputSizeNeeded(
    __in LPCTSTR StringBuffer,
    __in DWORD BufferLength
    );

VOID
YoriLibMultibyteOutput(
    __in LPCTSTR InputStringBuffer,
    __in DWORD InputBufferLength,
    __out LPSTR OutputStringBuffer,
    __in DWORD OutputBufferLength
    );

DWORD
YoriLibGetMultibyteInputSizeNeeded(
    __in LPCSTR StringBuffer,
    __in DWORD BufferLength
    );

VOID
YoriLibMultibyteInput(
    __in LPCSTR InputStringBuffer,
    __in DWORD InputBufferLength,
    __out LPTSTR OutputStringBuffer,
    __in DWORD OutputBufferLength
    );

// *** JOBOBJ.C ***

HANDLE
YoriLibCreateJobObject(
    );

BOOL
YoriLibAssignProcessToJobObject(
    __in HANDLE hJob,
    __in HANDLE hProcess
    );

BOOL
YoriLibLimitJobObjectPriority(
    __in HANDLE hJob,
    __in DWORD Priority
    );

// *** LICENSE.C ***

BOOL
YoriLibMitLicenseText(
    __in LPTSTR CopyrightYear,
    __out PYORI_STRING String
    );

// *** LINEREAD.C ***

PVOID
YoriLibReadLineToString(
    __in PYORI_STRING UserString,
    __in PVOID * Context,
    __in HANDLE FileHandle
    );

VOID
YoriLibLineReadClose(
    __in PVOID Context
    );

// *** LIST.C ***

VOID
YoriLibInitializeListHead(
    __out PYORI_LIST_ENTRY ListEntry
    );

VOID
YoriLibAppendList(
    __in PYORI_LIST_ENTRY ListHead,
    __out PYORI_LIST_ENTRY ListEntry
    );

VOID
YoriLibInsertList(
    __in PYORI_LIST_ENTRY ListHead,
    __out PYORI_LIST_ENTRY ListEntry
    );

VOID
YoriLibRemoveListItem(
    __in PYORI_LIST_ENTRY ListEntry
    );

PYORI_LIST_ENTRY
YoriLibGetNextListEntry(
    __in PYORI_LIST_ENTRY ListHead,
    __in_opt PYORI_LIST_ENTRY PreviousEntry
    );

PYORI_LIST_ENTRY
YoriLibGetPreviousListEntry(
    __in PYORI_LIST_ENTRY ListHead,
    __in_opt PYORI_LIST_ENTRY PreviousEntry
    );

// *** MALLOC.C ***

PVOID
YoriLibMalloc(
    __in DWORD Bytes
    );

VOID
YoriLibFree(
    __in PVOID Ptr
    );

VOID
YoriLibDisplayMemoryUsage();

PVOID
YoriLibReferencedMalloc(
    __in DWORD Bytes
    );

VOID
YoriLibReference(
    __in PVOID Allocation
    );

VOID
YoriLibDereference(
    __in PVOID Allocation
    );

// *** OSVER.C ***

VOID
YoriLibGetOsVersion(
    __out PDWORD MajorVersion,
    __out PDWORD MinorVersion,
    __out PDWORD BuildNumber
    );

BOOL
YoriLibIsProcess32Bit(
    __in HANDLE ProcessHandle
    );

BOOL
YoriLibDoesProcessHave32BitPeb(
    __in HANDLE ProcessHandle
    );

// *** RECYCLE.C ***

BOOL
YoriLibRecycleBinFile(
    __in PYORI_STRING FilePath
    );

// *** VT.C ***

BOOL
YoriLibOutputTextToMultibyteDevice(
    __in HANDLE hOutput,
    __in LPCTSTR StringBuffer,
    __in DWORD BufferLength
    );

/**
 Output the string to the standard output device.
 */
#define YORI_LIB_OUTPUT_STDOUT 0

/**
 Output the string to the standard error device.
 */
#define YORI_LIB_OUTPUT_STDERR 1

/**
 Remove VT100 escapes if the target is not expecting to handle these.
 */
#define YORI_LIB_OUTPUT_STRIP_VT 2

/**
 Initialize the output stream with any header information.
*/
typedef BOOL (* YORI_LIB_VT_INITIALIZE_STREAM_FN)(HANDLE);

/**
 End processing for the specified stream.
*/
typedef BOOL (* YORI_LIB_VT_END_STREAM_FN)(HANDLE);

/**
 Output text between escapes to the output device.
*/
typedef BOOL (* YORI_LIB_VT_PROCESS_AND_OUTPUT_TEXT_FN)(HANDLE, LPTSTR, DWORD);

/**
 A callback function to receive an escape and translate it into the
 appropriate action.
*/
typedef BOOL (* YORI_LIB_VT_PROCESS_AND_OUTPUT_ESCAPE_FN)(HANDLE, LPTSTR, DWORD);

/**
 A set of callback functions that can be invoked when processing VT100
 enhanced text and format it for the appropriate output device.
 */
typedef struct _YORI_LIB_VT_CALLBACK_FUNCTIONS {

    /**
     Initialize the output stream with any header information.
    */
    YORI_LIB_VT_INITIALIZE_STREAM_FN InitializeStream;

    /**
     End processing for the specified stream.
    */
    YORI_LIB_VT_END_STREAM_FN EndStream;

    /**
     Output text between escapes to the output device.
    */
    YORI_LIB_VT_PROCESS_AND_OUTPUT_TEXT_FN ProcessAndOutputText;

    /**
     A callback function to receive an escape and translate it into the
     appropriate action.
    */
    YORI_LIB_VT_PROCESS_AND_OUTPUT_ESCAPE_FN ProcessAndOutputEscape;

} YORI_LIB_VT_CALLBACK_FUNCTIONS, *PYORI_LIB_VT_CALLBACK_FUNCTIONS;

BOOL
YoriLibConsoleSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    );

BOOL
YoriLibConsoleNoEscapeSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    );

BOOL
YoriLibUtf8TextWithEscapesSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    );

BOOL
YoriLibUtf8TextNoEscapesSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    );

BOOL
YoriLibProcessVtEscapesOnOpenStream(
    __in LPTSTR String,
    __in DWORD StringLength,
    __in HANDLE hOutput,
    __in PYORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks
    );

BOOL
YoriLibOutput(
    __in DWORD Flags,
    __in LPCTSTR szFmt,
    ...
    );

BOOL
YoriLibOutputToDevice(
    __in HANDLE hOut,
    __in DWORD Flags,
    __in LPCTSTR szFmt,
    ...
    );

BOOL
YoriLibOutputString(
    __in HANDLE hOut,
    __in DWORD Flags,
    __in PYORI_STRING String
    );

BOOL
YoriLibVtSetConsoleTextAttributeOnDevice(
    __in HANDLE hOut,
    __in DWORD Flags,
    __in WORD Attribute
    );

BOOL
YoriLibVtStringForTextAttribute(
    __inout PYORI_STRING String,
    __in WORD Attribute
    );

BOOL
YoriLibVtSetConsoleTextAttribute(
    __in DWORD Flags,
    __in WORD Attribute
    );

VOID
YoriLibVtSetDefaultColor(
    __in WORD NewDefaultColor
    );

WORD
YoriLibVtGetDefaultColor();

// *** PATH.C ***

/**
 Prototype for a function to invoke on every potential path match.
 */
typedef BOOL YORI_LIB_PATH_MATCH_FN(PYORI_STRING Match, PVOID Context);

/**
 Pointer to a function to invoke on every potential path match.
 */
typedef YORI_LIB_PATH_MATCH_FN *PYORI_LIB_PATH_MATCH_FN;

BOOL
YoriLibPathLocateKnownExtensionUnknownLocation(
    __in PYORI_STRING SearchFor,
    __in PYORI_STRING PathVariable,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING FoundPath
    );

BOOL
YoriLibPathLocateUnknownExtensionUnknownLocation(
    __in PYORI_STRING SearchFor,
    __in PYORI_STRING PathVariable,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING FoundPath
    );

BOOL
YoriLibLocateExecutableInPath(
    __in PYORI_STRING SearchFor,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __out PYORI_STRING PathName
    );

BOOL
YoriLibAddEnvironmentComponent(
    __in LPTSTR EnvironmentVariable,
    __in PYORI_STRING NewComponent,
    __in BOOL InsertAtFront
    );

BOOL
YoriLibRemoveEnvironmentComponent(
    __in LPTSTR EnvironmentVariable,
    __in PYORI_STRING ComponentToRemove
    );

// *** PRINTF.C ***

int
YoriLibSPrintf(
    __out LPTSTR szDest,
    __in LPCTSTR szFmt,
    ...
    );

int
YoriLibSPrintfS(
    __out LPTSTR szDest,
    __in DWORD len,
    __in LPCTSTR szFmt,
    ...
    );

int
YoriLibSPrintfA(
    __out LPSTR szDest,
    __in LPCSTR szFmt,
    ...
    );

/**
 Process a printf format string and determine the size of a buffer that would
 be requried to contain the result.

 @param szFmt The format string to process.

 @param marker The existing va_args context to use to find variables to 
        substitute in the format string.

 @return The number of characters that the buffer would require, or -1 on
         error.
 */
int
YoriLibVSPrintfSize(
    __in LPCTSTR szFmt,
    __in va_list marker
    );

/**
 Process a printf format string and output the result into a NULL terminated
 buffer of specified size.

 @param szDest The buffer to populate with the result.

 @param len The number of characters in the buffer.

 @param szFmt The format string to process.

 @param marker The existing va_args context to use to find variables to 
        substitute in the format string.

 @return The number of characters successfully populated into the buffer, or
         -1 on error.
 */
int
YoriLibVSPrintf(
    __out LPTSTR szDest,
    __in DWORD len,
    __in LPCTSTR szFmt,
    __in va_list marker
    );

int
YoriLibYPrintf(
    __inout PYORI_STRING Dest,
    __in LPCTSTR szFmt,
    ...
    );

// *** ENV.C ***

BOOL
YoriLibGetEnvironmentStrings(
    __out PYORI_STRING EnvStrings
    );

BOOL
YoriLibSetEnvironmentStrings(
    __in PYORI_STRING NewEnv
    );

BOOL
YoriLibAreEnvironmentStringsValid(
    __inout PYORI_STRING EnvStrings
    );

BOOL
YoriLibAreAnsiEnvironmentStringsValid(
    __in PUCHAR AnsiEnvStringBuffer,
    __in DWORD BufferLength,
    __out PYORI_STRING UnicodeStrings
    );

// *** STRING.C ***

VOID
YoriLibInitEmptyString(
    __out PYORI_STRING String
   );

VOID
YoriLibFreeStringContents(
    __inout PYORI_STRING String
    );

BOOL
YoriLibAllocateString(
    __out PYORI_STRING String,
    __in DWORD CharsToAllocate
    );

BOOL
YoriLibReallocateString(
    __inout PYORI_STRING String,
    __in DWORD CharsToAllocate
    );

VOID
YoriLibConstantString(
    __out PYORI_STRING String,
    __in LPCTSTR Value
    );

LPTSTR
YoriLibCStringFromYoriString(
    __in PYORI_STRING String
    );

VOID
YoriLibCloneString(
    __out PYORI_STRING Dest,
    __in PYORI_STRING Src
    );

BOOL
YoriLibIsStringNullTerminated(
    __in PYORI_STRING String
    );

DWORD
YoriLibDecimalStringToInt(
    __in PYORI_STRING String
    );

BOOL
YoriLibStringToNumber(
    __in PYORI_STRING String,
    __in BOOL IgnoreSeperators,
    __out PLONGLONG Number,
    __out PDWORD CharsConsumed
    );

BOOL
YoriLibNumberToString(
    __inout PYORI_STRING String,
    __in LONGLONG Number,
    __in DWORD Base,
    __in DWORD DigitsPerGroup,
    __in TCHAR GroupSeperator
    );

VOID
YoriLibTrimSpaces(
    __in PYORI_STRING String
    );

TCHAR
YoriLibUpcaseChar(
    __in TCHAR c
    );

int
YoriLibCompareStringWithLiteral(
    __in PYORI_STRING Str1,
    __in LPCTSTR str2
    );

int
YoriLibCompareStringWithLiteralCount(
    __in PYORI_STRING Str1,
    __in LPCTSTR str2,
    __in DWORD count
    );

int
YoriLibCompareStringWithLiteralInsensitive(
    __in PYORI_STRING Str1,
    __in LPCTSTR str2
    );

int
YoriLibCompareStringWithLiteralInsensitiveCount(
    __in PYORI_STRING Str1,
    __in LPCTSTR str2,
    __in DWORD count
    );

int
YoriLibCompareString(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    );

int
YoriLibCompareStringInsensitive(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    );

int
YoriLibCompareStringInsensitiveCount(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2,
    __in DWORD count
    );

int
YoriLibCompareStringCount(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2,
    __in DWORD count
    );

DWORD
YoriLibCountStringContainingChars(
    __in PYORI_STRING String,
    __in LPCTSTR chars
    );

DWORD
YoriLibCountStringNotContainingChars(
    __in PYORI_STRING String,
    __in LPCTSTR match
    );

PYORI_STRING
YoriLibFindFirstMatchingSubstring(
    __in PYORI_STRING String,
    __in DWORD NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PDWORD StringOffsetOfMatch
    );

PYORI_STRING
YoriLibFindFirstMatchingSubstringInsensitive(
    __in PYORI_STRING String,
    __in DWORD NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PDWORD StringOffsetOfMatch
    );

LPTSTR
YoriLibFindLeftMostCharacter(
    __in PYORI_STRING String,
    __in TCHAR CharToFind
    );

LPTSTR
YoriLibFindRightMostCharacter(
    __in PYORI_STRING String,
    __in TCHAR CharToFind
    );

BOOL
YoriLibStringToHexBuffer(
    __in PYORI_STRING String,
    __out PUCHAR Buffer,
    __in DWORD BufferSize
    );

LARGE_INTEGER
YoriLibStringToFileSize(
    __in PYORI_STRING String
    );

BOOL
YoriLibStringToDate(
    __in PYORI_STRING String,
    __out LPSYSTEMTIME Date,
    __out_opt PDWORD CharsConsumed
    );

BOOL
YoriLibStringToTime(
    __in PYORI_STRING String,
    __out LPSYSTEMTIME Date
    );

BOOL
YoriLibStringToDateTime(
    __in PYORI_STRING String,
    __out LPSYSTEMTIME Date
    );

#undef  _tcscpy
/**
 A null terminated strcpy function defined in terms of printf.
 */
#define _tcscpy(a,b) YoriLibSPrintf(a, _T("%s"), b)

#undef  strcpy
/**
 A null terminated strcpy function defined in terms of printf.
 */
#define strcpy(a,b)  YoriLibSPrintfA(a, "%s", b)

#undef  wcscpy
/**
 A null terminated strcpy function defined in terms of printf.
 */
#define wcscpy(a,b)  YoriLibSPrintf(a, L"%s", b)

// *** UPDATE.C ***

/**
 If creating a build that supports updating, the root of the update site
 */
#define YORI_UPDATE_SITE "http://www.malsmith.net/download/?obj="

/**
 A set of error codes that can be returned from update attempts.
 */
typedef enum {
    YoriLibUpdErrorSuccess = 0,
    YoriLibUpdErrorInetInit,
    YoriLibUpdErrorInetConnect,
    YoriLibUpdErrorInetRead,
    YoriLibUpdErrorInetContents,
    YoriLibUpdErrorFileWrite,
    YoriLibUpdErrorFileReplace,
    YoriLibUpdErrorMax
} YoriLibUpdError;

BOOL
YoriLibUpdateBinaryFromFile(
    __in_opt LPTSTR ExistingPath,
    __in LPTSTR NewPath
    );

YoriLibUpdError
YoriLibUpdateBinaryFromUrl(
    __in LPTSTR Url,
    __in_opt LPTSTR TargetName,
    __in LPTSTR Agent
    );

LPCTSTR
YoriLibUpdateErrorString(
    __in YoriLibUpdError Error
    );

// *** UTIL.C ***

BOOL
YoriLibIsEscapeChar(
    __in TCHAR Char
    );

BOOL
YoriLibMakeInheritableHandle(
    __in HANDLE OriginalHandle,
    __out PHANDLE InheritableHandle
    );

LPTSTR
YoriLibGetWinErrorText(
    __in DWORD ErrorCode
    );

VOID
YoriLibFreeWinErrorText(
    __in LPTSTR ErrText
    );

// vim:sw=4:ts=4:et:
