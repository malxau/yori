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
 A pointer to a constant yori string.
 */
typedef YORI_STRING CONST *PCYORI_STRING;

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
    (PVOID)(((PUCHAR)(PTR)) + (OFFSET))

/**
 Subtracts a specified number of bytes from a pointer value and returns the
 resulting value.
 */
#define YoriLibSubtractFromPointer(PTR, OFFSET) \
    (PVOID)(((PUCHAR)(PTR)) - (OFFSET))

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

/**
 A prototype for a function to invoke when the module is being unloaded from
 the shell or if the shell is exiting.
 */
typedef
VOID
YORI_BUILTIN_FN
YORI_BUILTIN_UNLOAD_NOTIFY();

/**
 A prototype for a pointer to a function to invoke when the module is being
 unloaded from the shell or if the shell is exiting.
 */
typedef YORI_BUILTIN_UNLOAD_NOTIFY *PYORI_BUILTIN_UNLOAD_NOTIFY;

// *** BUILTIN.C ***

BOOL
YoriLibBuiltinSetEnvironmentStrings(
    __in PYORI_STRING NewEnvironment
    );

BOOL
YoriLibBuiltinRemoveEmptyVariables(
    __inout PYORI_STRING Value
    );

// *** CABINET.C ***

/**
 A prototype for a function to call on each file found within the cab.
 */
typedef
BOOL
YORI_LIB_CAB_EXPAND_FILE_CALLBACK(
    __in PYORI_STRING FullPathName,
    __in PYORI_STRING FileNameFromCab,
    __in PVOID UserContext
    );

/**
 A pointer to a function to call on each file found within the cab.
 */
typedef YORI_LIB_CAB_EXPAND_FILE_CALLBACK *PYORI_LIB_CAB_EXPAND_FILE_CALLBACK;

__success(return)
BOOL
YoriLibExtractCab(
    __in PYORI_STRING CabFileName,
    __in PYORI_STRING TargetDirectory,
    __in BOOL IncludeAllByDefault,
    __in DWORD NumberFilesToExclude,
    __in_opt PYORI_STRING FilesToExclude,
    __in DWORD NumberFilesToInclude,
    __in_opt PYORI_STRING FilesToInclude,
    __in_opt PYORI_LIB_CAB_EXPAND_FILE_CALLBACK CommenceExtractCallback,
    __in_opt PYORI_LIB_CAB_EXPAND_FILE_CALLBACK CompleteExtractCallback,
    __in_opt PVOID UserContext,
    __inout_opt PYORI_STRING ErrorString
    );

__success(return)
BOOL
YoriLibCreateCab(
    __in PYORI_STRING CabFileName,
    __out PVOID * Handle
    );

__success(return)
BOOL
YoriLibAddFileToCab(
    __in PVOID Handle,
    __in PYORI_STRING FileNameOnDisk,
    __in PYORI_STRING FileNameInCab
    );

VOID
YoriLibCloseCab(
    __in PVOID Handle
    );

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

__success(return)
BOOL
YoriLibBuildHtmlClipboardBuffer(
    __in PYORI_STRING TextToCopy,
    __out PHANDLE HandleForClipboard
    );

__success(return)
BOOL
YoriLibPasteText(
    __inout PYORI_STRING Buffer
    );

__success(return)
BOOL
YoriLibCopyText(
    __in PYORI_STRING Buffer
    );

__success(return)
BOOL
YoriLibCopyTextRtfAndHtml(
    __in PYORI_STRING TextVersion,
    __in PYORI_STRING RtfVersion,
    __in PYORI_STRING HtmlVersion
    );

// *** CMDLINE.C ***

BOOL
YoriLibIsCommandLineOptionChar(
    __in TCHAR Char
    );

__success(return)
BOOL
YoriLibIsCommandLineOption(
    __in PYORI_STRING String,
    __out PYORI_STRING Arg
    );

BOOLEAN
YoriLibCheckIfArgNeedsQuotes(
    __in PYORI_STRING Arg
    );

__success(return)
BOOL
YoriLibBuildCmdlineFromArgcArgv(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[],
    __in BOOLEAN EncloseInQuotes,
    __out PYORI_STRING CmdLine
    );

PYORI_STRING
YoriLibCmdlineToArgcArgv(
    __in LPTSTR szCmdLine,
    __in DWORD MaxArgs,
    __out PDWORD argc
    );

/**
 A prototype for a callback function to invoke for variable expansion.
 */
typedef DWORD YORILIB_VARIABLE_EXPAND_FN(PYORI_STRING OutputBuffer, PYORI_STRING VariableName, PVOID Context);

/**
 A pointer to a callback function to invoke for variable expansion.
 */
typedef YORILIB_VARIABLE_EXPAND_FN *PYORILIB_VARIABLE_EXPAND_FN;

__success(return)
BOOL
YoriLibExpandCommandVariables(
    __in PYORI_STRING String,
    __in TCHAR MatchChar,
    __in BOOLEAN PreserveEscapes,
    __in PYORILIB_VARIABLE_EXPAND_FN Function,
    __in_opt PVOID Context,
    __inout PYORI_STRING ExpandedString
    );

// *** COLOR.C ***


extern YORILIB_ATTRIBUTE_COLOR_STRING YoriLibColorStringTable[];

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
 A flag indicating that underline should be enabled.
 */
#define YORILIB_ATTRCTRL_UNDERLINE        0x40

/**
 If any of the flags are set here (or if a color is specified) no further
 processing is performed to find color information.
 */
#define YORILIB_ATTRCTRL_TERMINATE_MASK   (YORILIB_ATTRCTRL_HIDE)

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

VOID
YoriLibAttributeFromString(
    __in PYORI_STRING String,
    __out PYORILIB_COLOR_ATTRIBUTES OutAttributes
    );

VOID
YoriLibAttributeFromLiteralString(
    __in LPCTSTR String,
    __out PYORILIB_COLOR_ATTRIBUTES OutAttributes
    );

__success(return)
BOOL
YoriLibSetColorToWin32(
    __out PYORILIB_COLOR_ATTRIBUTES Attributes,
    __in UCHAR Win32Attribute
    );

VOID
YoriLibCombineColors(
    __in YORILIB_COLOR_ATTRIBUTES Color1,
    __in YORILIB_COLOR_ATTRIBUTES Color2,
    __out PYORILIB_COLOR_ATTRIBUTES OutAttributes
    );

VOID
YoriLibResolveWindowColorComponents(
    __in YORILIB_COLOR_ATTRIBUTES Color,
    __in YORILIB_COLOR_ATTRIBUTES WindowColor,
    __in BOOL RetainWindowCtrlFlags,
    __out PYORILIB_COLOR_ATTRIBUTES OutAttributes
    );

BOOL
YoriLibAreColorsIdentical(
    __in YORILIB_COLOR_ATTRIBUTES Color1,
    __in YORILIB_COLOR_ATTRIBUTES Color2
    );

LPCSTR
YoriLibGetDefaultFileColorString();

__success(return)
BOOL
YoriLibLoadCombinedFileColorString(
    __in_opt PYORI_STRING Custom,
    __out PYORI_STRING Combined
    );

__success(return)
BOOL
YoriLibGetMetadataColor(
    __in PYORI_STRING RequestedAttributeCodeString,
    __out PYORILIB_COLOR_ATTRIBUTES Color
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

extern DWORD YoriLibDefaultColorTable[];

__success(return)
BOOL
YoriLibCaptureConsoleFont(
    __out PYORI_CONSOLE_FONT_INFOEX FontInfo
    );

__success(return)
BOOL
YoriLibCaptureConsoleColorTable(
    __out PDWORD * ColorTable,
    __out_opt PWORD CurrentAttributes
    );

__success(return)
BOOL
YoriLibHtmlGenerateInitialString(
    __inout PYORI_STRING TextString
    );

__success(return)
BOOL
YoriLibHtmlGenerateEndString(
    __inout PYORI_STRING TextString
    );

__success(return)
BOOL
YoriLibHtmlGenerateTextString(
    __inout PYORI_STRING TextString,
    __out PDWORD BufferSizeNeeded,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    );

__success(return)
BOOL
YoriLibHtmlGenerateEscapeString(
    __inout PYORI_STRING TextString,
    __out PDWORD BufferSizeNeeded,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    );

__success(return)
BOOL
YoriLibHtmlSetVersion(
    __in DWORD HtmlVersion
    );

__success(return)
BOOL
YoriLibHtmlConvertToHtmlFromVt(
    __in PYORI_STRING VtText,
    __inout PYORI_STRING HtmlText,
    __in_opt PDWORD ColorTable,
    __in DWORD HtmlVersion
    );

// *** CVTRTF.C ***

__success(return)
BOOL
YoriLibRtfGenerateInitialString(
    __inout PYORI_STRING TextString,
    __in_opt PDWORD ColorTable
    );

__success(return)
BOOL
YoriLibRtfGenerateEndString(
    __inout PYORI_STRING TextString
    );

__success(return)
BOOL
YoriLibRtfGenerateTextString(
    __inout PYORI_STRING TextString,
    __out PDWORD BufferSizeNeeded,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    );

__success(return)
BOOL
YoriLibRtfGenerateEscapeString(
    __inout PYORI_STRING TextString,
    __out PDWORD BufferSizeNeeded,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    );

__success(return)
BOOL
YoriLibRtfConvertToRtfFromVt(
    __in PYORI_STRING VtText,
    __inout PYORI_STRING RtfText,
    __in_opt PDWORD ColorTable
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
YoriLibLoadBCryptFunctions();

BOOL
YoriLibLoadCabinetFunctions();

BOOL
YoriLibLoadCtl3d32Functions();

BOOL
YoriLibLoadDbgHelpFunctions();

BOOL
YoriLibLoadOle32Functions();

BOOL
YoriLibLoadPsapiFunctions();

BOOL
YoriLibLoadShell32Functions();

BOOL
YoriLibLoadShfolderFunctions();

BOOL
YoriLibLoadUser32Functions();

BOOL
YoriLibLoadVersionFunctions();

BOOL
YoriLibLoadVirtDiskFunctions();

BOOL
YoriLibLoadWinInetFunctions();

BOOL
YoriLibLoadWtsApi32Functions();

// *** FILECOMP.C ***

/**
 An algorithm that can be used to compress individual files.
 */
typedef union _YORILIB_COMPRESS_ALGORITHM {

    /**
     Individual algorithm types.  Note that if the NTFS algorithm is zero
     it implies the WOF algorithm should be used, but the same is not true
     in reverse since zero is a common WOF algorithm.
     */
    struct {
        WORD NtfsAlgorithm;
        WORD WofAlgorithm;
    };

    /**
     A 32 bit value that spans the entire union above for easy initialization.
     */
    DWORD EntireAlgorithm;
} YORILIB_COMPRESS_ALGORITHM;

/**
 Context describing a background pool of threads and list of work that can
 compress individual files.
 */
typedef struct _YORILIB_COMPRESS_CONTEXT {
    /**
     The list of files requiring compression.
     */
    YORI_LIST_ENTRY PendingList;

    /**
     A mutex to synchronize the list of files requiring compression.
     */
    HANDLE Mutex;

    /**
     An event signalled when there is a file to be compressed inserted into
     the list.
     */
    HANDLE WorkerWaitEvent;

    /**
     An event signalled when compression threads should complete outstanding
     work then terminate.
     */
    HANDLE WorkerShutdownEvent;

    /**
     An array of handles to threads allocated to compress file contents.
     */
    PHANDLE Threads;

    /**
     If the target should be written as compressed, this specifies the
     compression algorithm.
     */
    YORILIB_COMPRESS_ALGORITHM CompressionAlgorithm;

    /**
     The maximum number of compress threads.  This corresponds to the size of
     the Threads array.
     */
    DWORD MaxThreads;

    /**
     The number of threads allocated to compress file contents.  This is
     less than or equal to MaxThreads.
     */
    DWORD ThreadsAllocated;

    /**
     The number of items currently queued in the list.
     */
    DWORD ItemsQueued;

    /**
     If TRUE, output is generated describing thread creation and throttling.
     */
    BOOL Verbose;

} YORILIB_COMPRESS_CONTEXT, *PYORILIB_COMPRESS_CONTEXT;

BOOL
YoriLibInitializeCompressContext(
    __in PYORILIB_COMPRESS_CONTEXT CompressContext,
    __in YORILIB_COMPRESS_ALGORITHM CompressionAlgorithm
    );

VOID
YoriLibFreeCompressContext(
    __in PYORILIB_COMPRESS_CONTEXT CompressContext
    );

BOOL
YoriLibCompressFileInBackground(
    __in PYORILIB_COMPRESS_CONTEXT CompressContext,
    __in PYORI_STRING FileName
    );

BOOL
YoriLibDecompressFileInBackground(
    __in PYORILIB_COMPRESS_CONTEXT CompressContext,
    __in PYORI_STRING FileName
    );

DWORD
YoriLibGetWofVersionAvailable(
    __in PYORI_STRING FileName
    );

// *** FILEENUM.C ***

/**
 A prototype for a callback function to invoke for each matching file.
 */
typedef BOOL YORILIB_FILE_ENUM_ERROR_FN(PYORI_STRING FileName, DWORD ErrorCode, DWORD Depth, PVOID Context);

/**
 A pointer to a callback function to invoke for each matching file.
 */
typedef YORILIB_FILE_ENUM_ERROR_FN *PYORILIB_FILE_ENUM_ERROR_FN;

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

__success(return)
BOOL
YoriLibForEachFile(
    __in PYORI_STRING FileSpec,
    __in DWORD MatchFlags,
    __in DWORD Depth,
    __in PYORILIB_FILE_ENUM_FN Callback,
    __in_opt PYORILIB_FILE_ENUM_ERROR_FN ErrorCallback,
    __in_opt PVOID Context
    );

__success(return)
BOOL
YoriLibDoesFileMatchExpression (
    __in PYORI_STRING FileName,
    __in PYORI_STRING Wildcard
    );

// *** FILEFILT.C ***

/**
 An object describing a set of criteria to apply against a file to check
 for whether it should be included.
 */
typedef struct _YORI_LIB_FILE_FILTER {

    /**
     The number of criteria to apply.
     */
    DWORD NumberCriteria;

    /**
     The size of each element.
     */
    DWORD ElementSize;

    /**
     An array of criteria to apply.
     */
    PVOID Criteria;
} YORI_LIB_FILE_FILTER, *PYORI_LIB_FILE_FILTER;

/**
 Specifies a pointer to a function which can compare two directory entries
 in some fashion.
 */
typedef DWORD (* YORI_LIB_FILE_FILT_COMPARE_FN)(PYORI_FILE_INFO, PYORI_FILE_INFO);

/**
 Specifies a pointer to a function which can collect file information from
 the disk or file system for some particular piece of data.
 */
typedef BOOL (* YORI_LIB_FILE_FILT_COLLECT_FN)(PYORI_FILE_INFO, PWIN32_FIND_DATA, PYORI_STRING);

/**
 Specifies a pointer to a function which can generate in memory file
 information from a user provided string.
 */
typedef BOOL (* YORI_LIB_FILE_FILT_GENERATE_FROM_STRING_FN)(PYORI_FILE_INFO, PYORI_STRING);

/**
 An in memory representation of a single match criteria, specifying whether
 a file matches a specified criteria.
 */
typedef struct _YORI_LIB_FILE_FILT_MATCH_CRITERIA {

    /**
     Pointer to a function to ingest an incoming directory entry so that we
     have two objects to compare against.
     */
    YORI_LIB_FILE_FILT_COLLECT_FN CollectFn;

    /**
     Pointer to a function to compare an incoming directory entry against the
     dummy one contained here.
     */
    YORI_LIB_FILE_FILT_COMPARE_FN CompareFn;

    /**
     An array indicating whether a match is found if the comparison returns
     less than, greater than, or equal.
     */
    BOOL TruthStates[3];

    /**
     A dummy directory entry containing values to compare against.  This is
     used to allow all compare functions to operate on two directory entries.
     */
    YORI_FILE_INFO CompareEntry;
} YORI_LIB_FILE_FILT_MATCH_CRITERIA, *PYORI_LIB_FILE_FILT_MATCH_CRITERIA;

/**
 An in memory representation of a single match criteria and color to apply
 in the event that a file matches the criteria.
 */
typedef struct _YORI_LIB_FILE_FILT_COLOR_CRITERIA {

    /**
     Information describing the criteria to match and how to determine whether
     a match took place.
     */
    YORI_LIB_FILE_FILT_MATCH_CRITERIA Match;

    /**
     The color to apply in event of a match.
     */
    YORILIB_COLOR_ATTRIBUTES Color;
} YORI_LIB_FILE_FILT_COLOR_CRITERIA, *PYORI_LIB_FILE_FILT_COLOR_CRITERIA;

BOOL
YoriLibFileFiltHelp();

__success(return)
BOOL
YoriLibFileFiltParseFilterString(
    __out PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING FilterString,
    __out PYORI_STRING ErrorSubstring
    );

__success(return)
BOOL
YoriLibFileFiltParseColorString(
    __out PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING ColorString,
    __out PYORI_STRING ErrorSubstring
    );

__success(return)
BOOL
YoriLibFileFiltCheckFilterMatch(
    __in PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo
    );

__success(return)
BOOL
YoriLibFileFiltCheckColorMatch(
    __in PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __out PYORILIB_COLOR_ATTRIBUTES Attribute
    );

VOID
YoriLibFileFiltFreeFilter(
    __in PYORI_LIB_FILE_FILTER Filter
    );

__success(return)
BOOL
YoriLibUpdateFindDataFromFileInformation (
    __out PWIN32_FIND_DATA FindData,
    __in LPTSTR FullPath,
    __in BOOL CopyName
    );

// *** FILEINFO.C ***

/**
 For string/integer compares, indicates that the first value is less than the
 second.
 */
#define YORI_LIB_LESS_THAN    0

/**
 For string/integer compares, indicates that the first value is equal to the
 second.
 */
#define YORI_LIB_EQUAL        1

/**
 For string/integer compares, indicates that the first value is greater than
 the second.
 */
#define YORI_LIB_GREATER_THAN 2

/**
 For bitwise or wildcard compares, indicates a mismatch.
 */
#define YORI_LIB_NOT_EQUAL    0

/**
 A structure to map a 32 bit flag value to a character to input or output
 when describing the flag to humans.  The character is expected to be
 unique to allow input by character to function.
 */
typedef struct _YORI_LIB_CHAR_TO_DWORD_FLAG {

    /**
     The flag in native representation.
     */
    DWORD Flag;

    /**
     The character to display to the user.
     */
    TCHAR DisplayLetter;

    /**
     Unused in order to ensure structure alignment.
     */
    WORD  AlignmentPadding;

} YORI_LIB_CHAR_TO_DWORD_FLAG, *PYORI_LIB_CHAR_TO_DWORD_FLAG;

/**
 Pointer to a constant flag to character array.  This should typically be
 used because the data is in read only memory.
 */
typedef YORI_LIB_CHAR_TO_DWORD_FLAG CONST *PCYORI_LIB_CHAR_TO_DWORD_FLAG;

VOID
YoriLibGetFileAttrPairs(
    __out PDWORD Count,
    __out PCYORI_LIB_CHAR_TO_DWORD_FLAG * Pairs
    );

VOID
YoriLibGetFilePermissionPairs(
    __out PDWORD Count,
    __out PCYORI_LIB_CHAR_TO_DWORD_FLAG * Pairs
    );

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

DWORD
YoriLibCompareLargeInt (
    __in PULARGE_INTEGER Left,
    __in PULARGE_INTEGER Right
    );

DWORD
YoriLibCompareNullTerminatedString (
    __in LPCTSTR Left,
    __in LPCTSTR Right
    );

DWORD
YoriLibCompareDate (
    __in LPSYSTEMTIME Left,
    __in LPSYSTEMTIME Right
    );

DWORD
YoriLibCompareTime (
    __in LPSYSTEMTIME Left,
    __in LPSYSTEMTIME Right
    );

DWORD
YoriLibCompareAccessDate (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareAccessTime (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareAllocatedRangeCount (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareAllocationSize (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareArch (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareCompressionAlgorithm (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareCompressedFileSize (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareCreateDate (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareCreateTime (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareDescription (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareEffectivePermissions (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareFileAttributes (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareFileExtension (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareFileId (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareFileName (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareFileSize (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareFileVersionString (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareFragmentCount (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareLinkCount (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareObjectId (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareOsVersion (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareOwner (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareReparseTag (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareShortName (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareSubsystem (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareStreamCount (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareUsn (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareVersion (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareWriteDate (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibCompareWriteTime (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibBitwiseEffectivePermissions (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibBitwiseFileAttributes (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
    );

DWORD
YoriLibBitwiseFileName (
    __in PYORI_FILE_INFO Left,
    __in PYORI_FILE_INFO Right
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
YoriLibGenerateEffectivePermissions(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateFileAttributes(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateFileExtension(
    __inout PYORI_FILE_INFO Entry,
    __in PYORI_STRING String
    );

BOOL
YoriLibGenerateFileId(
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
YoriLibIsPathPrefixed(
    __in PYORI_STRING Path
    );

__success(return)
BOOL
YoriLibGetCurrentDirectoryOnDrive(
    __in TCHAR Drive,
    __out PYORI_STRING DriveCurrentDirectory
    );

__success(return)
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
YoriLibIsPrefixedDriveLetterWithColon(
    __in PYORI_STRING Path
    );

BOOL
YoriLibIsPrefixedDriveLetterWithColonAndSlash(
    __in PYORI_STRING Path
    );

BOOL
YoriLibIsFullPathUnc(
    __in PYORI_STRING Path
    );

__success(return)
BOOL
YoriLibFindEffectiveRoot(
    __in PYORI_STRING Path,
    __out PYORI_STRING EffectiveRoot
    );

__success(return)
BOOL
YoriLibGetFullPathNameReturnAllocation(
    __in PYORI_STRING FileName,
    __in BOOL bReturnEscapedPath,
    __inout PYORI_STRING Buffer,
    __deref_opt_out_opt LPTSTR* lpFilePart
    );

__success(return)
BOOL
YoriLibGetFullPathNameRelativeTo(
    __in PYORI_STRING PrimaryDirectory,
    __in PYORI_STRING FileName,
    __in BOOL ReturnEscapedPath,
    __inout PYORI_STRING Buffer,
    __deref_opt_out_opt LPTSTR* lpFilePart
    );

__success(return)
BOOL
YoriLibExpandHomeDirectories(
    __in PYORI_STRING FileString,
    __out PYORI_STRING ExpandedString
    );

BOOL
YoriLibIsFileNameDeviceName(
    __in PYORI_STRING File
    );

__success(return)
BOOL
YoriLibUserStringToSingleFilePath(
    __in PYORI_STRING UserString,
    __in BOOL bReturnEscapedPath,
    __out PYORI_STRING FullPath
    );

__success(return)
BOOL
YoriLibUserStringToSingleFilePathOrDevice(
    __in PYORI_STRING UserString,
    __in BOOL bReturnEscapedPath,
    __out PYORI_STRING FullPath
    );

__success(return)
BOOL
YoriLibUnescapePath(
    __in PYORI_STRING Path,
    __inout PYORI_STRING UnescapedPath
    );

__success(return)
BOOL
YoriLibGetVolumePathName(
    __in PYORI_STRING FileName,
    __inout PYORI_STRING VolumeName
    );

__success(return != INVALID_HANDLE_VALUE)
HANDLE
YoriLibFindFirstVolume(
    __out LPWSTR VolumeName,
    __in DWORD BufferLength
    );

__success(return)
BOOL
YoriLibFindNextVolume(
    __in HANDLE FindHandle,
    __out_ecount(BufferLength) LPWSTR VolumeName,
    __in DWORD BufferLength
    );

__success(return)
BOOL
YoriLibFindVolumeClose(
    __in HANDLE FindHandle
    );

__success(return)
BOOL
YoriLibGetDiskFreeSpace(
    __in LPCTSTR DirectoryName,
    __out_opt PLARGE_INTEGER BytesAvailable,
    __out_opt PLARGE_INTEGER TotalBytes,
    __out_opt PLARGE_INTEGER FreeBytes
    );

// *** GROUP.C ***

__success(return)
BOOL
YoriLibIsCurrentUserInGroup(
    __in PYORI_STRING GroupName,
    __out PBOOL IsMember
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
    __in PCYORI_STRING KeyString
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

/**
 The number of bytes of data to display in hex form per line.
 */
#define YORI_LIB_HEXDUMP_BYTES_PER_LINE 16

/**
 If set, display the characters as well as the hex values.
 */
#define YORI_LIB_HEX_FLAG_DISPLAY_CHARS        (0x00000001)

/**
 If set, display the buffer offset as a 32 bit value.
 */
#define YORI_LIB_HEX_FLAG_DISPLAY_OFFSET       (0x00000002)

/**
 If set, display the buffer offset as a 64 bit value.
 */
#define YORI_LIB_HEX_FLAG_DISPLAY_LARGE_OFFSET (0x00000004)

/**
 If set, output as comma-delimited C include style.  Incompatible with
 YORI_LIB_HEX_FLAG_DISPLAY_CHARS .
 */
#define YORI_LIB_HEX_FLAG_C_STYLE              (0x00000008)

BOOL
YoriLibHexDump(
    __in LPCSTR Buffer,
    __in LONGLONG StartOfBufferOffset,
    __in DWORD BufferLength,
    __in DWORD BytesPerWord,
    __in DWORD DumpFlags
    );

BOOL
YoriLibHexDiff(
    __in LONGLONG StartOfBufferOffset,
    __in LPCSTR Buffer1,
    __in DWORD Buffer1Length,
    __in LPCSTR Buffer2,
    __in DWORD Buffer2Length,
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

BOOLEAN
YoriLibIsUtf8Supported();

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
    __in_ecount(InputBufferLength) LPCTSTR InputStringBuffer,
    __in DWORD InputBufferLength,
    __out_ecount(OutputBufferLength) LPSTR OutputStringBuffer,
    __in DWORD OutputBufferLength
    );

DWORD
YoriLibGetMultibyteInputSizeNeeded(
    __in LPCSTR StringBuffer,
    __in DWORD BufferLength
    );

VOID
YoriLibMultibyteInput(
    __in_ecount(InputBufferLength) LPCSTR InputStringBuffer,
    __in DWORD InputBufferLength,
    __out_ecount(OutputBufferLength) LPTSTR OutputStringBuffer,
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
    __in LPCTSTR CopyrightYear,
    __out PYORI_STRING String
    );

BOOL
YoriLibDisplayMitLicense(
    __in LPCTSTR CopyrightYear
    );

// *** LINEREAD.C ***

/**
 The set of line endings that can be recognized by the line parser.
 */
typedef enum _YORI_LIB_LINE_ENDING {
    YoriLibLineEndingNone = 0,
    YoriLibLineEndingCRLF = 1,
    YoriLibLineEndingLF = 2,
    YoriLibLineEndingCR = 3
} YORI_LIB_LINE_ENDING;

/**
 Pointer to a line ending that is recognized by the line parser.
 */
typedef YORI_LIB_LINE_ENDING *PYORI_LIB_LINE_ENDING;

PVOID
YoriLibReadLineToString(
    __in PYORI_STRING UserString,
    __inout PVOID * Context,
    __in HANDLE FileHandle
    );

PVOID
YoriLibReadLineToStringEx(
    __in PYORI_STRING UserString,
    __inout PVOID * Context,
    __in BOOL ReturnFinalNonTerminatedLine,
    __in DWORD MaximumDelay,
    __in HANDLE FileHandle,
    __out PYORI_LIB_LINE_ENDING LineEnding,
    __out PBOOL TimeoutReached
    );

VOID
YoriLibLineReadClose(
    __in_opt PVOID Context
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

BOOL
YoriLibIsListEmpty(
    __in PYORI_LIST_ENTRY ListHead
    );

// *** MALLOC.C ***
//

#if DBG
#define YORI_SPECIAL_HEAP 1
#endif

#if YORI_SPECIAL_HEAP

PVOID
YoriLibMallocSpecialHeap(
    __in DWORD Bytes,
    __in LPCSTR Function,
    __in LPCSTR File,
    __in DWORD Line
    );

PVOID
YoriLibReferencedMallocSpecialHeap(
    __in DWORD Bytes,
    __in LPCSTR Function,
    __in LPCSTR File,
    __in DWORD Line
    );

#ifndef __FUNCTION__
#define __FUNCTION__ ""
#endif

#define YoriLibMalloc(Bytes) \
    YoriLibMallocSpecialHeap(Bytes, __FUNCTION__, __FILE__, __LINE__);

#define YoriLibReferencedMalloc(Bytes) \
    YoriLibReferencedMallocSpecialHeap(Bytes, __FUNCTION__, __FILE__, __LINE__);

#else
PVOID
YoriLibMalloc(
    __in DWORD Bytes
    );

PVOID
YoriLibReferencedMalloc(
    __in DWORD Bytes
    );
#endif

VOID
YoriLibFree(
    __in PVOID Ptr
    );

VOID
YoriLibDisplayMemoryUsage();


VOID
YoriLibReference(
    __in PVOID Allocation
    );

VOID
YoriLibDereference(
    __in PVOID Allocation
    );

// *** MOVEFILE.C ***

BOOLEAN
YoriLibMoveFile(
    __in PYORI_STRING Source,
    __in PYORI_STRING FullDest
    );

// *** NUMKEY.C ***

/**
 Indicates how to interpret the NumericKeyValue.  Ascii uses CP_OEMCP,
 Ansi uses CP_ACP, Unicode is direct.  Also note that Unicode takes
 input in hexadecimal to match the normal U+xxxx specification.
 */
typedef enum {
    YoriLibNumericKeyAscii = 0,
    YoriLibNumericKeyAnsi = 1,
    YoriLibNumericKeyUnicode = 2
} YORI_LIB_NUMERIC_KEY_TYPE;

/**
 Pointer to a numeric key type.
 */
typedef YORI_LIB_NUMERIC_KEY_TYPE *PYORI_LIB_NUMERIC_KEY_TYPE;

BOOLEAN
YoriLibBuildNumericKey(
    __inout PDWORD NumericKeyValue,
    __inout PYORI_LIB_NUMERIC_KEY_TYPE NumericKeyType,
    __in DWORD KeyCode,
    __in DWORD ScanCode
    );

BOOLEAN
YoriLibTranslateNumericKeyToChar(
    __in DWORD NumericKeyValue,
    __in YORI_LIB_NUMERIC_KEY_TYPE NumericKeyType,
    __out PTCHAR Char
    );

// *** OSVER.C ***

VOID
YoriLibGetOsVersion(
    __out PDWORD MajorVersion,
    __out PDWORD MinorVersion,
    __out PDWORD BuildNumber
    );

__success(return)
BOOL
YoriLibIsProcess32Bit(
    __in HANDLE ProcessHandle
    );

__success(return)
BOOL
YoriLibDoesProcessHave32BitPeb(
    __in HANDLE ProcessHandle
    );

// *** PARSE.C ***

__success(return)
BOOLEAN
YoriLibIsArgumentSeperator(
    __in PYORI_STRING String,
    __inout_opt PDWORD BraceNestingLevel,
    __out_opt PDWORD CharsToConsumeOut,
    __out_opt PBOOLEAN TerminateArgOut
    );

// *** PRIV.C ***

BOOL
YoriLibEnableBackupPrivilege();

BOOL
YoriLibEnableDebugPrivilege();

// *** PROCESS.C ***

__success(return)
BOOL
YoriLibGetSystemProcessList(
    __out PYORI_SYSTEM_PROCESS_INFORMATION *ProcessInfo
    );

// *** RECYCLE.C ***

BOOL
YoriLibRecycleBinFile(
    __in PYORI_STRING FilePath
    );

// *** STRMENUM.C ***

BOOL
YoriLibForEachStream(
    __in PYORI_STRING FileSpec,
    __in DWORD MatchFlags,
    __in DWORD Depth,
    __in PYORILIB_FILE_ENUM_FN Callback,
    __in_opt PYORILIB_FILE_ENUM_ERROR_FN ErrorCallback,
    __in PVOID Context
    );

// *** TEMP.C ***

__success(return)
BOOLEAN
YoriLibGetTempFileName(
    __in PYORI_STRING PathName,
    __in PYORI_STRING PrefixString,
    __out_opt PHANDLE TempHandle,
    __out_opt PYORI_STRING TempFileName
    );

// *** VT.C ***

/**
 Convert an ANSI attribute (in RGB order) back into Windows BGR order.
 */
#define YoriLibAnsiToWindowsNibble(COL)      \
    (((COL) & 8?FOREGROUND_INTENSITY:0)|     \
     ((COL) & 4?FOREGROUND_BLUE:0)|          \
     ((COL) & 2?FOREGROUND_GREEN:0)|         \
     ((COL) & 1?FOREGROUND_RED:0))

/**
 Convert an ANSI attribute, containing both a background and a foreground
 (in RGB order) back into Windows BGR order.
 */
#define YoriLibAnsiToWindowsByte(COL)          \
    (UCHAR)(YoriLibAnsiToWindowsNibble(COL) |  \
     (YoriLibAnsiToWindowsNibble((COL)>>4)<<4));

/**
 Convert a Win32 text attribute into an ANSI escape value.  This happens
 because the two disagree on RGB order.
 */
#define YoriLibWindowsToAnsi(COL)      \
    (((COL) & FOREGROUND_BLUE?4:0)|    \
     ((COL) & FOREGROUND_GREEN?2:0)|   \
     ((COL) & FOREGROUND_RED?1:0))

/**
 The maximum length of a string used to describe a VT sequence generated
 internally by one of these tools.
 */
#define YORI_MAX_INTERNAL_VT_ESCAPE_CHARS sizeof("E[0;999;999;1m")

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
 Include VT100 escapes in the output stream with no processing.
 */
#define YORI_LIB_OUTPUT_PASSTHROUGH_VT 4

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
    __in UCHAR Ctrl,
    __in WORD Attribute
    );

__success(return)
BOOL
YoriLibVtStringForTextAttribute(
    __inout PYORI_STRING String,
    __in UCHAR Ctrl,
    __in WORD Attribute
    );

BOOL
YoriLibVtSetConsoleTextAttribute(
    __in DWORD Flags,
    __in WORD Attribute
    );

VOID
YoriLibVtSetLineEnding(
    __in LPTSTR LineEnding
    );

LPTSTR
YoriLibVtGetLineEnding();

VOID
YoriLibVtSetDefaultColor(
    __in WORD NewDefaultColor
    );

WORD
YoriLibVtGetDefaultColor();

BOOL
YoriLibVtFinalColorFromSequence(
    __in WORD InitialColor,
    __in PYORI_STRING EscapeSequence,
    __out PWORD FinalColor
    );

BOOL
YoriLibStripVtEscapes(
    __in PYORI_STRING VtText,
    __inout PYORI_STRING PlainText
    );

BOOL
YoriLibGetWindowDimensions(
    __in HANDLE OutputHandle,
    __out_opt PDWORD Width,
    __out_opt PDWORD Height
    );

BOOL
YoriLibQueryConsoleCapabilities(
    __in HANDLE OutputHandle,
    __out_opt PBOOL SupportsColor,
    __out_opt PBOOL SupportsExtendedChars,
    __out_opt PBOOL SupportsAutoLineWrap
    );

// *** PATH.C ***

/**
 Prototype for a function to invoke on every potential path match.
 */
typedef BOOL YORI_LIB_PATH_MATCH_FN(PYORI_STRING Match, PVOID Context);

/**
 Pointer to a function to invoke on every potential path match.
 */
typedef YORI_LIB_PATH_MATCH_FN *PYORI_LIB_PATH_MATCH_FN;

__success(return)
BOOL
YoriLibPathLocateKnownExtensionUnknownLocation(
    __in PYORI_STRING SearchFor,
    __in PYORI_STRING PathVariable,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING FoundPath
    );

__success(return)
BOOL
YoriLibPathLocateUnknownExtensionKnownLocation(
    __in PYORI_STRING SearchFor,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING FoundPath
    );

__success(return)
BOOL
YoriLibPathLocateUnknownExtensionUnknownLocation(
    __in PYORI_STRING SearchFor,
    __in PYORI_STRING PathVariable,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING FoundPath
    );

__success(return)
BOOL
YoriLibLocateExecutableInPath(
    __in PYORI_STRING SearchFor,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __out PYORI_STRING PathName
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
    __out_ecount(len) LPTSTR szDest,
    __in DWORD len,
    __in LPCTSTR szFmt,
    ...
    );

int
YoriLibSPrintfSize(
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
    __out_ecount(len) LPTSTR szDest,
    __in DWORD len,
    __in LPCTSTR szFmt,
    __in va_list marker
    );

__success(return >= 0)
int
YoriLibYPrintf(
    __inout PYORI_STRING Dest,
    __in LPCTSTR szFmt,
    ...
    );

// *** ENV.C ***

__success(return)
BOOL
YoriLibGetEnvironmentStrings(
    __out PYORI_STRING EnvStrings
    );

__success(return)
BOOL
YoriLibAreEnvironmentStringsValid(
    __inout PYORI_STRING EnvStrings
    );

__success(return)
BOOL
YoriLibAreAnsiEnvironmentStringsValid(
    __in PUCHAR AnsiEnvStringBuffer,
    __in DWORD BufferLength,
    __out PYORI_STRING UnicodeStrings
    );

__success(return)
BOOL
YoriLibAllocateAndGetEnvironmentVariable(
    __in LPCTSTR Name,
    __inout PYORI_STRING Value
    );

__success(return)
BOOL
YoriLibGetEnvironmentVariableAsNumber(
    __in LPCTSTR Name,
    __out PLONGLONG Value
    );

__success(return)
BOOL
YoriLibAddEnvironmentComponentToString(
    __inout PYORI_STRING ExistingString,
    __in PYORI_STRING NewComponent,
    __in BOOL InsertAtFront
    );

__success(return)
BOOL
YoriLibAddEnvironmentComponentReturnString(
    __in PYORI_STRING EnvironmentVariable,
    __in PYORI_STRING NewComponent,
    __in BOOL InsertAtFront,
    __out PYORI_STRING Result
    );

__success(return)
BOOL
YoriLibAddEnvironmentComponent(
    __in LPTSTR EnvironmentVariable,
    __in PYORI_STRING NewComponent,
    __in BOOL InsertAtFront
    );

__success(return)
BOOL
YoriLibRemoveEnvironmentComponentFromString(
    __in PYORI_STRING String,
    __in PYORI_STRING ComponentToRemove,
    __out PYORI_STRING Result
    );

__success(return)
BOOL
YoriLibRemoveEnvironmentComponentReturnString(
    __in PYORI_STRING EnvironmentVariable,
    __in PYORI_STRING ComponentToRemove,
    __out PYORI_STRING Result
    );

__success(return)
BOOL
YoriLibRemoveEnvironmentComponent(
    __in LPTSTR EnvironmentVariable,
    __in PYORI_STRING ComponentToRemove
    );

// *** SCUT.C ***

__success(return)
BOOL
YoriLibCreateShortcut(
    __in PYORI_STRING ShortcutFileName,
    __in_opt PYORI_STRING Target,
    __in_opt PYORI_STRING Arguments,
    __in_opt PYORI_STRING Description,
    __in_opt PYORI_STRING WorkingDir,
    __in_opt PYORI_STRING IconPath,
    __in DWORD IconIndex,
    __in DWORD ShowState,
    __in WORD Hotkey,
    __in BOOL MergeWithExisting,
    __in BOOL CreateNewIfNeeded
    );

__success(return)
BOOL
YoriLibExecuteShortcut(
    __in PYORI_STRING ShortcutFileName
    );

// *** SELECT.C ***

/**
 A buffer containing an array of console character attributes, along
 with the size of the allocation.
 */
typedef struct _YORILIB_PREVIOUS_SELECTION_BUFFER {

    /**
     An array of character attributes corresponding to the previous
     selection.
     */
    PWORD AttributeArray;

    /**
     The size of the AttributeArray allocation.
     */
    DWORD BufferSize;

} YORILIB_PREVIOUS_SELECTION_BUFFER, *PYORILIB_PREVIOUS_SELECTION_BUFFER;

/**
 Describes a region of the screen which is selected.  This typically means the
 application supports mouse based selection and needs to display and the
 result of the selection.
 */
typedef struct _YORILIB_SELECTION {

    /**
     The coordinates where a selection started from.
     */
    COORD InitialPoint;

    /**
     If the mouse has left the screen, this records the extent of that
     departure.  This means values can be negative (indicating mouse is
     to the left or top of the screen), or positive (indicating right
     or bottom.)
     */
    COORD PeriodicScrollAmount;

    /**
     The region that was selected on the last rendering pass.
     */
    SMALL_RECT PreviouslyDisplayed;

    /**
     The region that is selected on the next rendering pass.
     */
    SMALL_RECT CurrentlyDisplayed;

    /**
     The region that is logically part of the selection where text should be
     obtained from.  This may be larger than the region that is displayed.
     */
    SMALL_RECT CurrentlySelected;

    /**
     TRUE if a selection is active on the previous rendering pass.
     */
    BOOLEAN SelectionPreviouslyActive;

    /**
     TRUE if a selection is active on the next rendering pass.
     */
    BOOLEAN SelectionCurrentlyActive;

    /**
     TRUE if a selection's initial point is known.  It won't become active
     until it is updated to cover a cell.
     */
    BOOLEAN InitialSpecified;

    /**
     TRUE if the selection color has been determined.
     */
    BOOLEAN SelectionColorSet;

    /**
     The current selection color.
     */
    WORD SelectionColor;

    /**
     The current index of the previous selection buffers.
     */
    DWORD CurrentPreviousIndex;

    /**
     An array of two previous selection buffers.  This allows us to ping-pong
     between the two buffers as the selection changes and avoid reallocates.
     */
    YORILIB_PREVIOUS_SELECTION_BUFFER PreviousBuffer[2];

    /**
     The number of elements in the TempCharInfoBuffer allocation.
     */
    DWORD TempCharInfoBufferSize;

    /**
     Pointer to a temporary allocation that can be used by
     YoriLibReadConsoleOutputAttributeForSelection when reading from the
     console before returning only the attribute components.  This is saved
     here to avoid repeated reallocations.
     */
    PCHAR_INFO TempCharInfoBuffer;

} YORILIB_SELECTION, *PYORILIB_SELECTION;

BOOL
YoriLibIsPreviousSelectionActive(
    __in PYORILIB_SELECTION Selection
    );

BOOL
YoriLibIsSelectionActive(
    __in PYORILIB_SELECTION Selection
    );

BOOL
YoriLibSelectionInitialSpecified(
    __in PYORILIB_SELECTION Selection
    );

VOID
YoriLibClearPreviousSelectionDisplay(
    __in PYORILIB_SELECTION Selection
    );

VOID
YoriLibDrawCurrentSelectionDisplay(
    __in PYORILIB_SELECTION Selection
    );

VOID
YoriLibRedrawSelection(
    __inout PYORILIB_SELECTION Selection
    );

VOID
YoriLibCleanupSelection(
    __inout PYORILIB_SELECTION Selection
    );

BOOL
YoriLibClearSelection(
    __inout PYORILIB_SELECTION Selection
    );

BOOL
YoriLibPeriodicScrollForSelection(
    __inout PYORILIB_SELECTION Selection
    );

BOOL
YoriLibCreateSelectionFromPoint(
    __inout PYORILIB_SELECTION Selection,
    __in USHORT X,
    __in USHORT Y
    );

BOOL
YoriLibCreateSelectionFromRange(
    __inout PYORILIB_SELECTION Selection,
    __in USHORT StartX,
    __in USHORT StartY,
    __in USHORT EndX,
    __in USHORT EndY
    );

BOOL
YoriLibUpdateSelectionToPoint(
    __inout PYORILIB_SELECTION Selection,
    __in SHORT X,
    __in SHORT Y
    );

BOOL
YoriLibNotifyScrollBufferMoved(
    __inout PYORILIB_SELECTION Selection,
    __in SHORT LinesToMove
    );

BOOL
YoriLibIsPeriodicScrollActive(
    __in PYORILIB_SELECTION Selection
    );

VOID
YoriLibClearPeriodicScroll(
    __in PYORILIB_SELECTION Selection
    );

BOOL
YoriLibCopySelectionIfPresent(
    __in PYORILIB_SELECTION Selection
    );

BOOL
YoriLibGetSelectionDoubleClickBreakChars(
    __out PYORI_STRING BreakChars
    );

WORD
YoriLibGetSelectionColor(
    __in HANDLE ConsoleHandle
    );

VOID
YoriLibSetSelectionColor(
    __in PYORILIB_SELECTION Selection,
    __in WORD SelectionColor
    );

BOOL
YoriLibIsYoriQuickEditEnabled();


// *** STRING.C ***

/**
 A macro to enable static initialization of a yori string.
 */
#define YORILIB_CONSTANT_STRING(x) \
{ NULL, x, sizeof(x) / sizeof(TCHAR) - 1, 0 }

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

BOOL
YoriLibReallocateStringWithoutPreservingContents(
    __inout PYORI_STRING String,
    __in DWORD CharsToAllocate
    );

VOID
YoriLibConstantString(
    __out _Post_satisfies_(String->StartOfString != NULL) PYORI_STRING String,
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

__success(return)
BOOL
YoriLibStringToNumberSpecifyBase(
    __in PYORI_STRING String,
    __in DWORD Base,
    __in BOOL IgnoreSeperators,
    __out PLONGLONG Number,
    __out PDWORD CharsConsumed
    );

__success(return)
BOOL
YoriLibStringToNumber(
    __in PYORI_STRING String,
    __in BOOL IgnoreSeperators,
    __out PLONGLONG Number,
    __out PDWORD CharsConsumed
    );

__success(return)
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

VOID
YoriLibTrimNullTerminators(
    __in PYORI_STRING String
    );

TCHAR
YoriLibUpcaseChar(
    __in TCHAR c
    );

int
YoriLibCompareStringWithLiteral(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2
    );

int
YoriLibCompareStringWithLiteralCount(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2,
    __in DWORD count
    );

int
YoriLibCompareStringWithLiteralInsensitive(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2
    );

int
YoriLibCompareStringWithLiteralInsensitiveCount(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2,
    __in DWORD count
    );

int
YoriLibCompareString(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2
    );

int
YoriLibCompareStringInsensitive(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2
    );

int
YoriLibCompareStringInsensitiveCount(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2,
    __in DWORD count
    );

int
YoriLibCompareStringCount(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2,
    __in DWORD count
    );

DWORD
YoriLibCountStringMatchingChars(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    );

DWORD
YoriLibCountStringMatchingCharsInsensitive(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    );

DWORD
YoriLibCountStringContainingChars(
    __in PCYORI_STRING String,
    __in LPCTSTR chars
    );

DWORD
YoriLibCountStringNotContainingChars(
    __in PCYORI_STRING String,
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

PYORI_STRING
YoriLibFindLastMatchingSubstring(
    __in PYORI_STRING String,
    __in DWORD NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PDWORD StringOffsetOfMatch
    );

PYORI_STRING
YoriLibFindLastMatchingSubstringInsensitive(
    __in PYORI_STRING String,
    __in DWORD NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PDWORD StringOffsetOfMatch
    );

LPTSTR
YoriLibFindLeftMostCharacter(
    __in PCYORI_STRING String,
    __in TCHAR CharToFind
    );

LPTSTR
YoriLibFindRightMostCharacter(
    __in PCYORI_STRING String,
    __in TCHAR CharToFind
    );

__success(return)
BOOL
YoriLibStringToHexBuffer(
    __in PYORI_STRING String,
    __out_ecount(BufferSize) PUCHAR Buffer,
    __in DWORD BufferSize
    );

__success(return)
BOOL
YoriLibHexBufferToString(
    __in PUCHAR Buffer,
    __in DWORD BufferSize,
    __in PYORI_STRING String
    );

LARGE_INTEGER
YoriLibStringToFileSize(
    __in PYORI_STRING String
    );

__success(return)
BOOL
YoriLibFileSizeToString(
    __inout PYORI_STRING String,
    __in PLARGE_INTEGER FileSize
    );

__success(return)
BOOL
YoriLibStringToDate(
    __in PYORI_STRING String,
    __out LPSYSTEMTIME Date,
    __out_opt PDWORD CharsConsumed
    );

__success(return)
BOOL
YoriLibStringToTime(
    __in PYORI_STRING String,
    __out LPSYSTEMTIME Date
    );

__success(return)
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
    __in LPTSTR Agent,
    __in_opt PSYSTEMTIME IfModifiedSince
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

__success(return)
BOOL
YoriLibMakeInheritableHandle(
    __in HANDLE OriginalHandle,
    __out PHANDLE InheritableHandle
    );

LPTSTR
YoriLibGetWinErrorText(
    __in DWORD ErrorCode
    );

LPTSTR
YoriLibGetNtErrorText(
    __in DWORD ErrorCode
    );

VOID
YoriLibFreeWinErrorText(
    __in LPTSTR ErrText
    );

__success(return)
BOOL
YoriLibCreateDirectoryAndParents(
    __inout PYORI_STRING DirName
    );

__success(return)
BOOL
YoriLibRenameFileToBackupName(
    __in PYORI_STRING FullPath,
    __out PYORI_STRING NewName
    );

BOOL
YoriLibIsPathUrl(
    __in PYORI_STRING PackagePath
    );

BOOL YoriLibIsStdInConsole();

// vim:sw=4:ts=4:et:
