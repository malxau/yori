/**
 * @file lib/yorilib.h
 *
 * Header for library routines that may be of value from the shell as well
 * as external tools.
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

/**
 The data type that can describe a single memory allocation.  Note this value
 is an implicit limit on string length (a string cannot exceed a single
 allocation.)
 */
typedef DWORD                    YORI_ALLOC_SIZE_T;

/**
 Pointer to a type describing a memory allocation length.
 */
typedef YORI_ALLOC_SIZE_T        *PYORI_ALLOC_SIZE_T;

/**
 The maximum length of a memory allocation, in bytes.
 */
#define YORI_MAX_ALLOC_SIZE      ((YORI_ALLOC_SIZE_T)-1)

/**
 A signed value corresponding to memory allocation length.  This is used for
 interfaces where the sign bit denotes an error.  The implication is this
 may be larger than the unsigned value or shorter, depending on whether the
 error bit is reducing the allowable allocation size or not.
 */
typedef LONG                     YORI_SIGNED_ALLOC_SIZE_T;

/**
 Pointer to a type describing a signed memory allocation length.
 */
typedef YORI_SIGNED_ALLOC_SIZE_T *PYORI_SIGNED_ALLOC_SIZE_T;

/**
 A type describing the largest describable unsigned integer in this
 environment.  This must implicitly be equal to or larger than the
 maximum allocation size (we can't allocate a number of bytes that
 can't be represented.)
 */
typedef DWORDLONG                YORI_MAX_UNSIGNED_T;

/**
 Pointer to a type describing the largest describable unsigned integer in
 this environment.
 */
typedef YORI_MAX_UNSIGNED_T      *PYORI_MAX_UNSIGNED_T;

/**
 A type describing the largest describable signed integer in this
 environment.  This must implicitly be equal to or larger than the maximum
 allocation size (we can't allocate a number of bytes that can't be
 represented.)
 */
typedef LONGLONG                 YORI_MAX_SIGNED_T;

/**
 Pointer to a type describing the largest describable signed integer in this
 environment.
 */
typedef YORI_MAX_SIGNED_T        *PYORI_MAX_SIGNED_T;

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
    YORI_ALLOC_SIZE_T LengthInChars;

    /**
     The maximum number of characters that could be put into this allocation.
     */
    YORI_ALLOC_SIZE_T LengthAllocated;
} YORI_STRING, *PYORI_STRING;

/**
 A pointer to a constant yori string.
 */
typedef YORI_STRING CONST *PCYORI_STRING;

/**
 An array of strings that can be appended to and sorted.
 */
typedef struct _YORI_STRING_ARRAY {

    /**
     Count of items with meaningful data in the array.
     */
    YORI_ALLOC_SIZE_T Count;

    /**
     Number of elements allocated in the Items array.
     */
    YORI_ALLOC_SIZE_T CountAllocated;

    /**
     The base of the string allocation (to reference when consuming.)
     */
    LPTSTR StringAllocationBase;

    /**
     The current end of the string allocation (to write to when consuming.)
     */
    LPTSTR StringAllocationCurrent;

    /**
     The number of characters remaining in the string allocation.
     */
    YORI_ALLOC_SIZE_T StringAllocationRemaining;

    /**
     An array of items in memory.  This allocation is referenced because it
     is used here, and may be referenced by each string that is contained
     within the allocation.
     */
    PYORI_STRING Items;

} YORI_STRING_ARRAY, *PYORI_STRING_ARRAY;

/**
 A buffer for a single data stream.
 */
typedef struct _YORI_LIB_BYTE_BUFFER {

    /**
     The data buffer.
     */
    PUCHAR Buffer;

    /**
     The number of bytes currently allocated to this buffer.
     */
    YORI_MAX_UNSIGNED_T BytesAllocated;

    /**
     The number of bytes populated with data in this buffer.
     */
    YORI_MAX_UNSIGNED_T BytesPopulated;

} YORI_LIB_BYTE_BUFFER, *PYORI_LIB_BYTE_BUFFER;

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
    YORI_ALLOC_SIZE_T NumberBuckets;

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
typedef enum {
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
     The directory's case sensitivity status.
     */
    BOOLEAN       CaseSensitive;

    /**
     Unused space used for alignment.
     */
    BOOLEAN       ReservedForAlignment;

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
    YORI_ALLOC_SIZE_T FileNameLengthInChars;

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
    __in YORI_ALLOC_SIZE_T ArgC,
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
YORI_BUILTIN_UNLOAD_NOTIFY(VOID);

/**
 A prototype for a pointer to a function to invoke when the module is being
 unloaded from the shell or if the shell is exiting.
 */
typedef YORI_BUILTIN_UNLOAD_NOTIFY *PYORI_BUILTIN_UNLOAD_NOTIFY;

// *** AIRPLANE.C ***

__success(return)
BOOLEAN
YoriLibGetAirplaneMode(
    __out PBOOLEAN AirplaneModeEnabled,
    __out PBOOLEAN AirplaneModeChangable
    );

__success(return)
BOOLEAN
YoriLibSetAirplaneMode(
    __in BOOLEAN AirplaneModeEnabled
    );

// *** BARGRAPH.C ***

BOOLEAN
YoriLibDisplayBarGraph(
    __in HANDLE hOutput,
    __in DWORD TenthsOfPercent,
    __in DWORD GreenThreshold,
    __in DWORD RedThreshold
    );

// *** BUILTIN.C ***

BOOL
YoriLibBuiltinSetEnvironmentStrings(
    __in PYORI_STRING NewEnvironment
    );

BOOL
YoriLibBuiltinRemoveEmptyVariables(
    __inout PYORI_STRING Value
    );

// *** BYTEBUF.C ***

BOOL
YoriLibByteBufferInitialize(
    __out PYORI_LIB_BYTE_BUFFER Buffer,
    __in YORI_MAX_UNSIGNED_T InitialSize
    );

VOID
YoriLibByteBufferCleanup(
    __in PYORI_LIB_BYTE_BUFFER Buffer
    );

VOID
YoriLibByteBufferReset(
    __inout PYORI_LIB_BYTE_BUFFER Buffer
    );

__success(return != NULL)
PUCHAR
YoriLibByteBufferGetPointerToEnd(
    __in PYORI_LIB_BYTE_BUFFER Buffer,
    __in YORI_MAX_UNSIGNED_T MinimumLengthRequired,
    __out_opt PYORI_ALLOC_SIZE_T BytesAvailable
    );

BOOLEAN
YoriLibByteBufferAddToPopulatedLength(
    __in PYORI_LIB_BYTE_BUFFER Buffer,
    __in YORI_MAX_UNSIGNED_T NewBytesPopulated
    );

__success(return != NULL)
PUCHAR
YoriLibByteBufferGetPointerToValidData(
    __in PYORI_LIB_BYTE_BUFFER Buffer,
    __in YORI_MAX_UNSIGNED_T BufferOffset,
    __out_opt PYORI_ALLOC_SIZE_T BytesAvailable
    );

YORI_MAX_UNSIGNED_T
YoriLibByteBufferGetValidBytes(
    __in PYORI_LIB_BYTE_BUFFER Buffer
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
    __inout_opt PDWORD ErrorCode,
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

VOID
YoriLibCancelSet(VOID);

BOOL
YoriLibCancelEnable(
    __in BOOLEAN Ignore
    );

BOOL
YoriLibCancelDisable(VOID);

BOOL
YoriLibCancelIgnore(VOID);

BOOL
YoriLibIsOperationCancelled(VOID);

VOID
YoriLibCancelReset(VOID);

HANDLE
YoriLibCancelGetEvent(VOID);

VOID
YoriLibCancelInheritedIgnore(VOID);

VOID
YoriLibCancelInheritedProcess(VOID);

BOOL
YoriLibSetInputConsoleMode(
    __in HANDLE Handle,
    __in DWORD ConsoleMode
    );

BOOL
YoriLibSetInputConsoleModeWithoutExtended(
    __in HANDLE Handle,
    __in DWORD ConsoleMode
    );

// *** CLIP.C ***

BOOLEAN
YoriLibOpenClipboard(VOID);

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

__success(return)
BOOL
YoriLibCopyBinaryData(
    __in PUCHAR Buffer,
    __in YORI_ALLOC_SIZE_T BufferLength
    );

__success(return)
BOOL
YoriLibPasteBinaryData(
    __out PUCHAR *Buffer,
    __out PYORI_ALLOC_SIZE_T BufferLength
    );

BOOLEAN
YoriLibIsSystemClipboardAvailable(VOID);

__success(return)
BOOL
YoriLibCopyTextWithProcessFallback(
    __in PYORI_STRING Buffer
    );

__success(return)
BOOL
YoriLibPasteTextWithProcessFallback(
    __inout PYORI_STRING Buffer
    );

VOID
YoriLibEmptyProcessClipboard(VOID);

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
BOOLEAN
YoriLibArgArrayToVariableValue(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in PYORI_STRING ArgV,
    __out PYORI_STRING Variable,
    __out PBOOLEAN ValueSpecified,
    __out PYORI_STRING Value
    );

__success(return)
BOOL
YoriLibBuildCmdlineFromArgcArgv(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[],
    __in BOOLEAN EncloseInQuotes,
    __in BOOLEAN ApplyChildProcessEscapes,
    __out PYORI_STRING CmdLine
    );

PYORI_STRING
YoriLibCmdlineToArgcArgv(
    __in LPCTSTR szCmdLine,
    __in YORI_ALLOC_SIZE_T MaxArgs,
    __in BOOLEAN ApplyCaretAsEscape,
    __out PYORI_ALLOC_SIZE_T argc
    );

/**
 A prototype for a callback function to invoke for variable expansion.
 */
typedef YORI_ALLOC_SIZE_T YORILIB_VARIABLE_EXPAND_FN(PYORI_STRING OutputBuffer, PYORI_STRING VariableName, PVOID Context);

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

VOID
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
YoriLibGetDefaultFileColorString(VOID);

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

// *** CONDRV.C ***

__success(return)
BOOL
YoriLibSetConsoleDisplayMode(
    __in HANDLE hConsoleOutput,
    __in DWORD dwFlags,
    __out PCOORD lpNewScreenBufferDimensions
    );

// *** CPUINFO.C ***

VOID
YoriLibQueryCpuCount(
    __out PWORD PerformanceLogicalProcessors,
    __out PWORD EfficiencyLogicalProcessors
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

// *** CURDIR.C ***

__success(return)
BOOLEAN
YoriLibGetOnDiskCaseForPath(
    __in PYORI_STRING Path,
    __out PYORI_STRING OnDiskCasePath
    );

__success(return)
BOOLEAN
YoriLibGetCurrentDirectoryOnDrive(
    __in TCHAR Drive,
    __out PYORI_STRING DriveCurrentDirectory
    );

__success(return)
BOOLEAN
YoriLibSetCurrentDirectoryOnDrive(
    __in TCHAR Drive,
    __in PYORI_STRING DriveCurrentDirectory
    );

BOOLEAN
YoriLibSetCurrentDirectorySaveDriveCurrentDirectory(
    __in PYORI_STRING NewCurrentDirectory
    );

__success(return)
BOOLEAN
YoriLibGetCurrentDirectory(
    __out PYORI_STRING CurrentDirectory
    );

__success(return)
BOOLEAN
YoriLibGetCurrentDirectoryForDisplay(
    __out PYORI_STRING CurrentDirectory
    );

BOOLEAN
YoriLibSetCurrentDirectory(
    __in PYORI_STRING NewCurrentDirectory
    );

VOID
YoriLibCleanupCurrentDirectory(VOID);

// *** CVTCONS.C ***

BOOLEAN
YoriLibConsoleVtConvertGetCursorLocation(
    __in PVOID Context,
    __out PDWORD X,
    __out PDWORD Y
    );

BOOLEAN
YoriLibConsoleVtConvertDisplayToConsole(
    __in PVOID Context,
    __in HANDLE hConsole,
    __in DWORD X,
    __in DWORD Y
    );

PVOID
YoriLibAllocateConsoleVtConvertContext(
    __in WORD Columns,
    __in DWORD AllocateLines,
    __in DWORD MaximumLines,
    __in WORD DefaultAttributes,
    __in WORD CurrentAttributes,
    __in BOOLEAN LineWrap
    );

// *** CVTHTML.C ***

/**
 When generating HTML, context recording which operations are in effect
 that will affect future parsing.
 */
typedef struct _YORILIB_HTML_GENERATE_CONTEXT {
    /**
     The dialect of HTML to use.
     */
    DWORD HtmlVersion;

    /**
     While parsing, TRUE if a tag is currently open so that it can be closed
     on the next font change.
     */
    BOOLEAN TagOpen;

    /**
     While parsing, TRUE if underlining is in effect.
     */
    BOOLEAN UnderlineOn;

    /**
     While parsing, TRUE if bold is in effect.
     */
    BOOLEAN BoldOn;
} YORILIB_HTML_GENERATE_CONTEXT, *PYORILIB_HTML_GENERATE_CONTEXT;

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
    __inout PYORI_STRING TextString,
    __inout PYORILIB_HTML_GENERATE_CONTEXT GenerateContext
    );

__success(return)
BOOL
YoriLibHtmlGenerateEndString(
    __inout PYORI_STRING TextString,
    __inout PYORILIB_HTML_GENERATE_CONTEXT GenerateContext
    );

__success(return)
BOOL
YoriLibHtmlGenerateTextString(
    __inout PYORI_STRING TextString,
    __out PYORI_ALLOC_SIZE_T BufferSizeNeeded,
    __in PCYORI_STRING SrcString
    );

__success(return)
BOOL
YoriLibHtmlGenerateEscapeString(
    __inout PYORI_STRING TextString,
    __out PYORI_ALLOC_SIZE_T BufferSizeNeeded,
    __in PCYORI_STRING SrcString,
    __inout PYORILIB_HTML_GENERATE_CONTEXT GenerateContext
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
    __out PYORI_ALLOC_SIZE_T BufferSizeNeeded,
    __in PCYORI_STRING SrcString
    );

__success(return)
BOOL
YoriLibRtfGenerateEscapeString(
    __inout PYORI_STRING TextString,
    __out PYORI_ALLOC_SIZE_T BufferSizeNeeded,
    __in PCYORI_STRING SrcString,
    __inout PBOOLEAN UnderlineState
    );

__success(return)
BOOL
YoriLibRtfConvertToRtfFromVt(
    __in PYORI_STRING VtText,
    __inout PYORI_STRING RtfText,
    __in_opt PDWORD ColorTable
    );

// *** DBLCLK.C ***

BOOL
YoriLibGetSelectionDoubleClickBreakChars(
    __out PYORI_STRING BreakChars
    );

BOOL
YoriLibIsYoriQuickEditEnabled(VOID);

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

__success(return)
BOOLEAN
YoriLibFullPathToSystemDirectory(
    __in PCYORI_STRING FileName,
    __out PYORI_STRING FullPath
    );

HMODULE
YoriLibLoadLibraryFromSystemDirectory(
    __in LPCTSTR DllName
    );

BOOL
YoriLibLoadNtDllFunctions(VOID);

BOOL
YoriLibLoadKernel32Functions(VOID);

BOOL
YoriLibLoadAdvApi32Functions(VOID);

BOOL
YoriLibLoadCabinetFunctions(VOID);

BOOL
YoriLibLoadCrypt32Functions(VOID);

BOOL
YoriLibLoadCtl3d32Functions(VOID);

BOOL
YoriLibLoadDbgHelpFunctions(VOID);

BOOL
YoriLibLoadImageHlpFunctions(VOID);

BOOL
YoriLibLoadOle32Functions(VOID);

BOOL
YoriLibLoadPowrprofFunctions(VOID);

BOOL
YoriLibLoadPsapiFunctions(VOID);

BOOL
YoriLibLoadShell32Functions(VOID);

BOOL
YoriLibLoadShfolderFunctions(VOID);

BOOL
YoriLibLoadUser32Functions(VOID);

BOOL
YoriLibLoadUserEnvFunctions(VOID);

BOOL
YoriLibLoadVersionFunctions(VOID);

BOOL
YoriLibLoadVirtDiskFunctions(VOID);

BOOL
YoriLibLoadWinBrandFunctions(VOID);

BOOL
YoriLibLoadWinHttpFunctions(VOID);

BOOL
YoriLibLoadWinInetFunctions(VOID);

BOOL
YoriLibLoadWlanApiFunctions(VOID);

BOOL
YoriLibLoadWsock32Functions(VOID);

BOOL
YoriLibLoadWtsApi32Functions(VOID);

// *** ENV.C ***

__success(return)
BOOL
YoriLibGetEnvironmentStrings(
    __out PYORI_STRING EnvStrings
    );

__success(return)
BOOL
YoriLibAreEnvStringsValid(
    __inout PYORI_STRING EnvStrings
    );

#ifdef UNICODE

__success(return)
BOOL
YoriLibAreAnsiEnvironmentStringsValid(
    __in PUCHAR AnsiEnvStringBuffer,
    __in YORI_ALLOC_SIZE_T BufferLength,
    __out PYORI_STRING UnicodeStrings
    );

#endif

__success(return)
BOOL
YoriLibAllocateAndGetEnvVar(
    __in LPCTSTR Name,
    __inout PYORI_STRING Value
    );

__success(return)
BOOL
YoriLibGetEnvVarAsNumber(
    __in LPCTSTR Name,
    __out PYORI_MAX_SIGNED_T Value
    );

__success(return)
BOOL
YoriLibAddEnvCompToString(
    __inout PYORI_STRING ExistingString,
    __in PCYORI_STRING NewComponent,
    __in BOOL InsertAtFront
    );

__success(return)
BOOL
YoriLibAddEnvCompReturnString(
    __in PYORI_STRING EnvironmentVariable,
    __in PYORI_STRING NewComponent,
    __in BOOL InsertAtFront,
    __out PYORI_STRING Result
    );

__success(return)
BOOL
YoriLibAddEnvComponent(
    __in LPTSTR EnvironmentVariable,
    __in PYORI_STRING NewComponent,
    __in BOOL InsertAtFront
    );

__success(return)
BOOL
YoriLibRmEnvCompFromString(
    __in PYORI_STRING String,
    __in PYORI_STRING ComponentToRemove,
    __out PYORI_STRING Result
    );

__success(return)
BOOL
YoriLibRmEnvCompReturnString(
    __in PYORI_STRING EnvironmentVariable,
    __in PYORI_STRING ComponentToRemove,
    __out PYORI_STRING Result
    );

__success(return)
BOOL
YoriLibRemoveEnvComponent(
    __in LPTSTR EnvironmentVariable,
    __in PYORI_STRING ComponentToRemove
    );

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
    YORI_ALLOC_SIZE_T MaxThreads;

    /**
     The number of threads allocated to compress file contents.  This is
     less than or equal to MaxThreads.
     */
    YORI_ALLOC_SIZE_T ThreadsAllocated;

    /**
     The number of items currently queued in the list.
     */
    YORI_ALLOC_SIZE_T ItemsQueued;

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
    __in WORD MatchFlags,
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
YoriLibFileFiltHelp(VOID);

__success(return)
BOOL
YoriLibFileFiltParseFilterString(
    __out PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING FilterString,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorSubstring
    );

__success(return)
BOOL
YoriLibFileFiltParseColorString(
    __out PYORI_LIB_FILE_FILTER Filter,
    __in PYORI_STRING ColorString,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorSubstring
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
    __out PYORI_ALLOC_SIZE_T Count,
    __out PCYORI_LIB_CHAR_TO_DWORD_FLAG * Pairs
    );

VOID
YoriLibGetDirectoryPairs(
    __out PYORI_ALLOC_SIZE_T Count,
    __out PCYORI_LIB_CHAR_TO_DWORD_FLAG * Pairs
    );

VOID
YoriLibGetFilePermissionPairs(
    __out PYORI_ALLOC_SIZE_T Count,
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
YoriLibCollectCaseSensitivity (
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
YoriLibCompareCaseSensitivity (
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
YoriLibCompareDirectory (
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
YoriLibGenerateCaseSensitivity(
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
YoriLibGenerateDirectory(
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
    __in PCYORI_STRING Path
    );

BOOL
YoriLibIsDriveLetterWithColon(
    __in PCYORI_STRING Path
    );

BOOL
YoriLibIsDriveLetterWithColonAndSlash(
    __in PCYORI_STRING Path
    );

BOOL
YoriLibIsPrefixedDriveLetterWithColon(
    __in PCYORI_STRING Path
    );

BOOL
YoriLibIsPrefixedDriveLetterWithColonAndSlash(
    __in PCYORI_STRING Path
    );

BOOL
YoriLibIsFullPathUnc(
    __in PCYORI_STRING Path
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
    __in PCYORI_STRING File
    );

__success(return)
BOOL
YoriLibUserStringToSingleFilePath(
    __in PCYORI_STRING UserString,
    __in BOOL bReturnEscapedPath,
    __out PYORI_STRING FullPath
    );

__success(return)
BOOL
YoriLibUserStringToSingleFilePathOrDevice(
    __in PCYORI_STRING UserString,
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

__success(return)
BOOLEAN
YoriLibPathSupportsLongNames(
    __in PCYORI_STRING PathName,
    __out PBOOLEAN LongNameSupport
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
YoriLibCheckTokenMembership(
    __in HANDLE TokenHandle,
    __in PSID SidToCheck,
    __out PBOOL IsMember
    );

__success(return)
BOOL
YoriLibIsCurrentUserInGroup(
    __in PYORI_STRING GroupName,
    __out PBOOL IsMember
    );

__success(return)
BOOL
YoriLibIsCurrentUserInWellKnownGroup(
    __in DWORD GroupId,
    __out PBOOL IsMember
    );

// *** HASH.C ***

DWORD
YoriLibHashString32(
    __in DWORD InitialHash,
    __in PCYORI_STRING String
    );

PYORI_HASH_TABLE
YoriLibAllocateHashTable(
    __in YORI_ALLOC_SIZE_T NumberBuckets
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
 If set, display the one byte characters as well as the hex values.
 */
#define YORI_LIB_HEX_FLAG_DISPLAY_CHARS        (0x00000001)

/**
 If set, display the two byte characters as well as the hex values.
 */
#define YORI_LIB_HEX_FLAG_DISPLAY_WCHARS       (0x00000002)

/**
 If set, display the buffer offset as a 32 bit value.
 */
#define YORI_LIB_HEX_FLAG_DISPLAY_OFFSET       (0x00000004)

/**
 If set, display the buffer offset as a 64 bit value.
 */
#define YORI_LIB_HEX_FLAG_DISPLAY_LARGE_OFFSET (0x00000008)

/**
 If set, output as comma-delimited C include style.  Incompatible with
 YORI_LIB_HEX_FLAG_DISPLAY_CHARS .
 */
#define YORI_LIB_HEX_FLAG_C_STYLE              (0x00000010)

TCHAR
YoriLibHexDigitFromValue(
    __in DWORD Value
    );

BOOL
YoriLibHexDump(
    __in LPCSTR Buffer,
    __in LONGLONG StartOfBufferOffset,
    __in YORI_ALLOC_SIZE_T BufferLength,
    __in DWORD BytesPerWord,
    __in DWORD DumpFlags
    );

BOOL
YoriLibHexDiff(
    __in LONGLONG StartOfBufferOffset,
    __in LPCSTR Buffer1,
    __in YORI_ALLOC_SIZE_T Buffer1Length,
    __in LPCSTR Buffer2,
    __in YORI_ALLOC_SIZE_T Buffer2Length,
    __in DWORD BytesPerWord,
    __in DWORD DumpFlags
    );

VOID
YoriLibHexLineToString(
    __in CONST UCHAR * Buffer,
    __in LONGLONG StartOfBufferOffset,
    __in YORI_ALLOC_SIZE_T BufferLength,
    __in DWORD BytesPerWord,
    __in DWORD DumpFlags,
    __in BOOLEAN MoreFollowing,
    __inout PYORI_STRING LineBuffer
    );

// *** HTTP.C ***

LPVOID WINAPI
YoriLibInternetOpen(
    __in LPCTSTR UserAgent,
    __in DWORD AccessType,
    __in_opt LPCTSTR ProxyName,
    __in_opt LPCTSTR ProxyBypass,
    __in DWORD Flags
    );

BOOL WINAPI
YoriLibInternetCloseHandle(
    __in LPVOID hInternet
    );

LPVOID WINAPI
YoriLibInternetOpenUrl(
    __in LPVOID hInternet,
    __in LPCTSTR Url,
    __in LPCTSTR Headers,
    __in DWORD HeadersLength,
    __in DWORD Flags,
    __in DWORD_PTR Context
    );

__success(return)
BOOL WINAPI
YoriLibInternetReadFile(
    __in LPVOID hRequest,
    __out_bcount(BytesToRead) LPVOID Buffer,
    __in DWORD BytesToRead,
    __out PDWORD BytesRead
    );

__success(return)
BOOL WINAPI
YoriLibHttpQueryInfo(
    __in LPVOID hInternet,
    __in DWORD InfoLevel,
    __out LPVOID Buffer,
    __inout PDWORD BufferLength,
    __inout PDWORD Index
    );

// *** ICONV.C ***

#ifndef CP_UTF8_OR_16

/**
 A code to describe UTF8_OR_16 encoding.  This encoding is implemented within
 this library, and refers to UTF16 text if the BOM explicitly indicates it,
 and UTF8 otherwise.
 */
#define CP_UTF8_OR_16 0xFEF8
#endif

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
YoriLibIsUtf8Supported(VOID);

DWORD
YoriLibGetMultibyteOutputEncoding(VOID);

DWORD
YoriLibGetMultibyteInputEncoding(VOID);

VOID
YoriLibSetMultibyteOutputEncoding(
    __in DWORD Encoding
    );

VOID 
YoriLibSetMultibyteInputEncoding(
    __in DWORD Encoding
    );

YORI_ALLOC_SIZE_T
YoriLibGetMbyteOutputSizeNeeded(
    __in LPCTSTR StringBuffer,
    __in YORI_ALLOC_SIZE_T BufferLength
    );

VOID
YoriLibMultibyteOutput(
    __in_ecount(InputBufferLength) LPCTSTR InputStringBuffer,
    __in YORI_ALLOC_SIZE_T InputBufferLength,
    __out_ecount(OutputBufferLength) LPSTR OutputStringBuffer,
    __in YORI_ALLOC_SIZE_T OutputBufferLength
    );

YORI_ALLOC_SIZE_T
YoriLibGetMultibyteInputSizeNeeded(
    __in LPCSTR StringBuffer,
    __in YORI_ALLOC_SIZE_T BufferLength
    );

VOID
YoriLibMultibyteInput(
    __in_ecount(InputBufferLength) LPCSTR InputStringBuffer,
    __in YORI_ALLOC_SIZE_T InputBufferLength,
    __out_ecount(OutputBufferLength) LPTSTR OutputStringBuffer,
    __in YORI_ALLOC_SIZE_T OutputBufferLength
    );

// *** JOBOBJ.C ***

HANDLE
YoriLibCreateJobObject(VOID);

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

VOID
YoriLibLineReadCloseOrCache(
    __in_opt PVOID Context
    );

VOID
YORI_BUILTIN_FN
YoriLibLineReadCleanupCache(VOID);

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
    __in YORI_ALLOC_SIZE_T Bytes,
    __in LPCSTR Function,
    __in LPCSTR File,
    __in DWORD Line
    );

PVOID
YoriLibReferencedMallocSpecialHeap(
    __in YORI_ALLOC_SIZE_T Bytes,
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
    __in YORI_ALLOC_SIZE_T Bytes
    );

PVOID
YoriLibReferencedMalloc(
    __in YORI_ALLOC_SIZE_T Bytes
    );
#endif

VOID
YoriLibFree(
    __in PVOID Ptr
    );

VOID
YoriLibDisplayMemoryUsage(VOID);


VOID
YoriLibReference(
    __in PVOID Allocation
    );

VOID
YoriLibDereference(
    __in PVOID Allocation
    );

BOOLEAN
YoriLibIsSizeAllocatable(
    __in YORI_MAX_UNSIGNED_T Size
    );

YORI_ALLOC_SIZE_T
YoriLibMaximumAllocationInRange(
    __in YORI_MAX_UNSIGNED_T RequiredSize,
    __in YORI_MAX_UNSIGNED_T DesiredSize
    );

YORI_ALLOC_SIZE_T
YoriLibIsAllocationExtendable(
    __in YORI_ALLOC_SIZE_T ExistingSize,
    __in YORI_ALLOC_SIZE_T RequiredExtraSize,
    __in YORI_ALLOC_SIZE_T DesiredExtraSize
    );

// *** MOVEFILE.C ***

DWORD
YoriLibMoveFile(
    __in PYORI_STRING Source,
    __in PYORI_STRING FullDest,
    __in BOOLEAN ReplaceExisting,
    __in BOOLEAN PosixSemantics
    );

DWORD
YoriLibCopyFile(
    __in PYORI_STRING SourceFile,
    __in PYORI_STRING DestFile
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

// *** OBENUM.C ***

/**
 A definition for a callback function to invoke for each object enumerated in
 an object manager directory.
 */
typedef BOOL YORILIB_OBJECT_ENUM_FN(PCYORI_STRING FullName, PCYORI_STRING NameOnly, PCYORI_STRING Type, PVOID Context);

/**
 A pointer to a callback function to invoke for each object enumerated in an
 object manager directory.
 */
typedef YORILIB_OBJECT_ENUM_FN *PYORILIB_OBJECT_ENUM_FN;

/**
 A definition for a callback function to invoke if any errors are encountered
 when enumerating an object manager directory.
 */
typedef BOOL YORILIB_OBJECT_ENUM_ERROR_FN(PCYORI_STRING FullName, LONG NtStatus, PVOID Context);

/**
 A pointer to a callback function to invoke if any errors are encountered
 when enumerating an object manager directory.
 */
typedef YORILIB_OBJECT_ENUM_ERROR_FN *PYORILIB_OBJECT_ENUM_ERROR_FN;

VOID
YoriLibInitializeObjectAttributes(
    __out PYORI_OBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt HANDLE RootDirectory,
    __in_opt PCYORI_STRING Name,
    __in DWORD Attributes
    );

__success(return)
BOOL
YoriLibForEachObjectEnum(
    __in PCYORI_STRING DirectoryName,
    __in DWORD MatchFlags,
    __in PYORILIB_OBJECT_ENUM_FN Callback,
    __in_opt PYORILIB_OBJECT_ENUM_ERROR_FN ErrorCallback,
    __in_opt PVOID Context
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
YoriLibLoadOsEdition(
    __out PYORI_STRING Edition
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

DWORD
YoriLibGetArchitecture(VOID);

BOOLEAN
YoriLibIsNanoServer(VOID);

BOOLEAN
YoriLibDoesSystemSupportBackgroundColors(VOID);

VOID
YoriLibResetSystemBackgroundColorSupport(VOID);

BOOLEAN
YoriLibIsRunningUnderSsh(VOID);

YORI_ALLOC_SIZE_T
YoriLibGetPageSize(VOID);

BOOLEAN
YoriLibEnsureProcessSubsystemVersionAtLeast(
    __in WORD NewMajor,
    __in WORD NewMinor
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
    __in BOOLEAN SearchCurrentDirectory,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __inout PYORI_STRING FoundPath
    );

__success(return)
BOOLEAN
YoriLibLocateExecutableInPath(
    __in PYORI_STRING SearchFor,
    __in_opt PYORI_LIB_PATH_MATCH_FN MatchAllCallback,
    __in_opt PVOID MatchAllContext,
    __out _When_(MatchAllCallback != NULL, _Post_invalid_) PYORI_STRING PathName
    );

// *** PRINTF.C ***

YORI_SIGNED_ALLOC_SIZE_T
YoriLibSPrintf(
    __out LPTSTR szDest,
    __in LPCTSTR szFmt,
    ...
    );

YORI_SIGNED_ALLOC_SIZE_T
YoriLibSPrintfS(
    __out_ecount(len) LPTSTR szDest,
    __in YORI_ALLOC_SIZE_T len,
    __in LPCTSTR szFmt,
    ...
    );

YORI_SIGNED_ALLOC_SIZE_T
YoriLibSPrintfSize(
    __in LPCTSTR szFmt,
    ...
    );

YORI_SIGNED_ALLOC_SIZE_T
YoriLibSPrintfA(
    __out LPSTR szDest,
    __in LPCSTR szFmt,
    ...
    );

YORI_SIGNED_ALLOC_SIZE_T
YoriLibSPrintfSA(
    __out_ecount(len) LPSTR szDest,
    __in YORI_ALLOC_SIZE_T len,
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
YORI_SIGNED_ALLOC_SIZE_T
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
YORI_SIGNED_ALLOC_SIZE_T
YoriLibVSPrintf(
    __out_ecount(len) LPTSTR szDest,
    __in YORI_ALLOC_SIZE_T len,
    __in LPCTSTR szFmt,
    __in va_list marker
    );

__success(return >= 0)
YORI_SIGNED_ALLOC_SIZE_T
YoriLibYPrintf(
    __inout PYORI_STRING Dest,
    __in LPCTSTR szFmt,
    ...
    );

// *** PRIV.C ***

BOOL
YoriLibEnableBackupPrivilege(VOID);

BOOL
YoriLibEnableDebugPrivilege(VOID);

BOOL
YoriLibEnableManageVolumePrivilege(VOID);

BOOL
YoriLibEnableShutdownPrivilege(VOID);

BOOL
YoriLibEnableSymbolicLinkPrivilege(VOID);

BOOL
YoriLibEnableSystemTimePrivilege(VOID);

BOOL
YoriLibEnableTakeOwnershipPrivilege(VOID);

// *** PROCESS.C ***

__success(return)
BOOL
YoriLibGetSystemProcessList(
    __out PYORI_SYSTEM_PROCESS_INFORMATION *ProcessInfo
    );

__success(return)
BOOL
YoriLibGetSystemHandlesList(
    __out PYORI_SYSTEM_HANDLE_INFORMATION_EX *HandlesInfo
    );

// *** PROGMAN.C ***

__success(return)
BOOL
YoriLibAddProgmanItem(
    __in PCYORI_STRING GroupName,
    __in PCYORI_STRING ItemName,
    __in PCYORI_STRING ItemPath,
    __in_opt PCYORI_STRING WorkingDirectory,
    __in_opt PCYORI_STRING IconPath,
    __in DWORD IconIndex
    );

// *** RECYCLE.C ***

BOOL
YoriLibRecycleBinFile(
    __in PYORI_STRING FilePath
    );

// *** RSRC.C ***

__success(return)
BOOLEAN
YoriLibLoadAndVerifyStringResourceArray(
    __in YORI_ALLOC_SIZE_T InitialElement,
    __in YORI_ALLOC_SIZE_T NumberElements,
    __out PYORI_STRING * StringArray
    );

// *** SCHEME.C ***

__success(return)
BOOL
YoriLibLoadColorTableFromScheme(
    __in PCYORI_STRING IniFileName,
    __out_ecount(16) COLORREF *ColorTable
    );

__success(return)
BOOL
YoriLibLoadWindowColorFromScheme(
    __in PCYORI_STRING IniFileName,
    __out PUCHAR WindowColor
    );

__success(return)
BOOL
YoriLibLoadPopupColorFromScheme(
    __in PCYORI_STRING IniFileName,
    __out PUCHAR WindowColor
    );

BOOL
YoriLibSaveColorTableToScheme(
    __in PCYORI_STRING IniFileName,
    __in COLORREF *ColorTable
    );

BOOL
YoriLibSaveWindowColorToScheme(
    __in PCYORI_STRING IniFileName,
    __in UCHAR WindowColor
    );

BOOL
YoriLibSavePopupColorToScheme(
    __in PCYORI_STRING IniFileName,
    __in UCHAR WindowColor
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
    __in_opt PISHELLLINKDATALIST_CONSOLE_PROPS ConsoleProps,
    __in DWORD IconIndex,
    __in DWORD ShowState,
    __in WORD Hotkey,
    __in BOOL MergeWithExisting,
    __in BOOL CreateNewIfNeeded
    );

__success(return)
BOOL
YoriLibLoadShortcutIconPath(
    __in PYORI_STRING ShortcutFileName,
    __out PYORI_STRING IconPath,
    __out PDWORD IconIndex
    );

__success(return == S_OK)
HRESULT
YoriLibExecuteShortcut(
    __in PYORI_STRING ShortcutFileName,
    __in BOOLEAN ForceElevate,
    __out_opt PDWORD LaunchedProcessId
    );

PISHELLLINKDATALIST_CONSOLE_PROPS
YoriLibAllocateDefaultConsoleProperties(VOID);

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

WORD
YoriLibGetSelectionColor(
    __in HANDLE ConsoleHandle
    );

VOID
YoriLibSetSelectionColor(
    __in PYORILIB_SELECTION Selection,
    __in WORD SelectionColor
    );

// *** STRARRAY.C ***

VOID
YoriStringArrayInitialize(
    __out PYORI_STRING_ARRAY StringArray
    );

VOID
YoriStringArrayCleanup(
    __inout PYORI_STRING_ARRAY StringArray
    );

__success(return)
BOOLEAN
YoriStringArrayAddItems(
    __inout PYORI_STRING_ARRAY StringArray,
    __in PCYORI_STRING NewItems,
    __in YORI_ALLOC_SIZE_T NumNewItems
    );

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
    __in YORI_ALLOC_SIZE_T CharsToAllocate
    );

BOOL
YoriLibReallocString(
    __inout PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T CharsToAllocate
    );

BOOL
YoriLibReallocStringNoContents(
    __inout PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T CharsToAllocate
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

BOOLEAN
YoriLibCopyString(
    __out PYORI_STRING Dest,
    __in PCYORI_STRING Src
    );

BOOL
YoriLibIsStringNullTerminated(
    __in PCYORI_STRING String
    );

DWORD
YoriLibDecimalStringToInt(
    __in PYORI_STRING String
    );

__success(return)
BOOL
YoriLibStringToNumberBase(
    __in PCYORI_STRING String,
    __in WORD Base,
    __in BOOL IgnoreSeperators,
    __out PYORI_MAX_SIGNED_T Number,
    __out PYORI_ALLOC_SIZE_T CharsConsumed
    );

__success(return)
BOOL
YoriLibStringToNumber(
    __in PCYORI_STRING String,
    __in BOOL IgnoreSeperators,
    __out PYORI_MAX_SIGNED_T Number,
    __out PYORI_ALLOC_SIZE_T CharsConsumed
    );

__success(return)
BOOL
YoriLibNumberToString(
    __inout PYORI_STRING String,
    __in YORI_MAX_SIGNED_T Number,
    __in WORD Base,
    __in WORD DigitsPerGroup,
    __in TCHAR GroupSeperator
    );

VOID
YoriLibTrimSpaces(
    __in PYORI_STRING String
    );

VOID
YoriLibTrimTrailingNewlines(
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

VOID
YoriLibRightAlignString(
    __in PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T Align
    );

int
YoriLibCompareStringLit(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2
    );

int
YoriLibCompareStringLitCnt(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2,
    __in YORI_ALLOC_SIZE_T count
    );

int
YoriLibCompareStringLitIns(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2
    );

int
YoriLibCompareStringLitInsCnt(
    __in PCYORI_STRING Str1,
    __in LPCTSTR str2,
    __in YORI_ALLOC_SIZE_T count
    );

int
YoriLibCompareString(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2
    );

int
YoriLibCompareStringIns(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2
    );

int
YoriLibCompareStringInsCnt(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2,
    __in YORI_ALLOC_SIZE_T count
    );

int
YoriLibCompareStringCnt(
    __in PCYORI_STRING Str1,
    __in PCYORI_STRING Str2,
    __in YORI_ALLOC_SIZE_T count
    );

YORI_ALLOC_SIZE_T
YoriLibCntStringMatchChars(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    );

YORI_ALLOC_SIZE_T
YoriLibCntStringMatchCharsIns(
    __in PYORI_STRING Str1,
    __in PYORI_STRING Str2
    );

YORI_ALLOC_SIZE_T
YoriLibCntStringWithChars(
    __in PCYORI_STRING String,
    __in LPCTSTR chars
    );

YORI_ALLOC_SIZE_T
YoriLibCntStringNotWithChars(
    __in PCYORI_STRING String,
    __in LPCTSTR match
    );

YORI_ALLOC_SIZE_T
YoriLibCntStringTrailingChars(
    __in PCYORI_STRING String,
    __in LPCTSTR chars
    );

PYORI_STRING
YoriLibFindFirstMatchSubstr(
    __in PCYORI_STRING String,
    __in YORI_ALLOC_SIZE_T NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PYORI_ALLOC_SIZE_T StringOffsetOfMatch
    );

PYORI_STRING
YoriLibFindFirstMatchSubstrIns(
    __in PCYORI_STRING String,
    __in YORI_ALLOC_SIZE_T NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PYORI_ALLOC_SIZE_T StringOffsetOfMatch
    );

PYORI_STRING
YoriLibFindLastMatchSubstr(
    __in PCYORI_STRING String,
    __in YORI_ALLOC_SIZE_T NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PYORI_ALLOC_SIZE_T StringOffsetOfMatch
    );

PYORI_STRING
YoriLibFindLastMatchSubstrIns(
    __in PCYORI_STRING String,
    __in YORI_ALLOC_SIZE_T NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PYORI_ALLOC_SIZE_T StringOffsetOfMatch
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
    __in YORI_ALLOC_SIZE_T BufferSize
    );

__success(return)
BOOL
YoriLibHexBufferToString(
    __in PUCHAR Buffer,
    __in YORI_ALLOC_SIZE_T BufferSize,
    __in PYORI_STRING String
    );

LARGE_INTEGER
YoriLibStringToFileSize(
    __in PCYORI_STRING String
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
    __in PCYORI_STRING String,
    __out LPSYSTEMTIME Date,
    __out_opt PYORI_ALLOC_SIZE_T CharsConsumed
    );

__success(return)
BOOL
YoriLibStringToTime(
    __in PCYORI_STRING String,
    __out LPSYSTEMTIME Date
    );

__success(return)
BOOL
YoriLibStringToDateTime(
    __in PCYORI_STRING String,
    __out LPSYSTEMTIME Date
    );

VOID
YoriLibSortStringArray(
    __in_ecount(Count) PYORI_STRING StringArray,
    __in YORI_ALLOC_SIZE_T Count
    );

BOOLEAN
YoriLibStringConcat(
    __inout PYORI_STRING String,
    __in PCYORI_STRING AppendString
    );

BOOLEAN
YoriLibStringConcatWithLiteral(
    __inout PYORI_STRING String,
    __in LPCTSTR AppendString
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

// *** STRMENUM.C ***

BOOL
YoriLibForEachStream(
    __in PYORI_STRING FileSpec,
    __in WORD MatchFlags,
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

BOOL
YoriLibGetTempPath(
    __out PYORI_STRING TempPathName,
    __in YORI_ALLOC_SIZE_T ExtraChars
    );

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
} YORI_LIB_UPDATE_ERROR;

YORI_LIB_UPDATE_ERROR
YoriLibUpdateBinaryFromUrl(
    __in PCYORI_STRING Url,
    __in_opt PCYORI_STRING TargetName,
    __in PCYORI_STRING Agent,
    __in_opt PSYSTEMTIME IfModifiedSince
    );

LPCTSTR
YoriLibUpdateErrorString(
    __in YORI_LIB_UPDATE_ERROR Error
    );

// *** UTIL.C ***

BOOL
YoriLibIsEscapeChar(
    __in TCHAR Char
    );

VOID
YoriLibLiAssignUnsigned(
    __out PLARGE_INTEGER Li,
    __in YORI_MAX_UNSIGNED_T Value
    );

BOOLEAN
YoriLibIsDoubleWideChar(
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
    __in PCYORI_STRING FullPath,
    __out PYORI_STRING NewName
    );

BOOL
YoriLibIsPathUrl(
    __in PCYORI_STRING PackagePath
    );

BOOL YoriLibIsStdInConsole(VOID);

LONGLONG
YoriLibGetSystemTimeAsInteger(VOID);

BOOL
YoriLibPosixDeleteFile(
    __in PYORI_STRING FileName
    );

DWORDLONG
YoriLibDivide32(
    __in DWORDLONG Numerator,
    __in DWORD Denominator
    );

BOOLEAN
YoriLibIsCharPrintable(
    __in WCHAR Char
    );

DWORD
YoriLibGetHandleSectorSize(
    __in HANDLE FileHandle
    );

__success(return == ERROR_SUCCESS)
DWORD
YoriLibGetFileOrDeviceSize(
    __in HANDLE FileHandle,
    __out PDWORDLONG FileSizePtr
    );

INT
YoriLibMulDiv(
    __in INT Number,
    __in INT Numerator,
    __in INT Denominator
    );

__success(return)
BOOL
YoriLibFileTimeToDosDateTime(
    __in CONST FILETIME *FileTime,
    __out LPWORD FatDate,
    __out LPWORD FatTime
    );

__success(return)
BOOL
YoriLibDosDateTimeToFileTime(
    __in WORD FatDate,
    __in WORD FatTime,
    __out LPFILETIME FileTime
    );

DWORD
YoriLibShellExecuteInstanceToError(
    __in_opt HINSTANCE hInst
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
#define YORI_MAX_VT_ESCAPE_CHARS sizeof("E[0;999;999;1m")

BOOL
YoriLibOutputTextToMbyteDev(
    __in HANDLE hOutput,
    __in PCYORI_STRING String
    );

/**
 Output the string to the standard output device.
 */
#define YORI_LIB_OUTPUT_STDOUT 0x00

/**
 Output the string to the standard error device.
 */
#define YORI_LIB_OUTPUT_STDERR 0x01

/**
 Output the string to the debugger device.
 */
#define YORI_LIB_OUTPUT_DEBUG  0x02

/**
 Remove VT100 escapes if the target is not expecting to handle these.
 */
#define YORI_LIB_OUTPUT_STRIP_VT 0x10

/**
 Include VT100 escapes in the output stream with no processing.
 */
#define YORI_LIB_OUTPUT_PASSTHROUGH_VT 0x20

/**
 Initialize the output stream with any header information.
*/
typedef BOOL (* YORI_LIB_VT_INITIALIZE_STREAM_FN)(HANDLE, YORI_MAX_UNSIGNED_T*);

/**
 End processing for the specified stream.
*/
typedef BOOL (* YORI_LIB_VT_END_STREAM_FN)(HANDLE, YORI_MAX_UNSIGNED_T*);

/**
 Output text between escapes to the output device.
*/
typedef BOOL (* YORI_LIB_VT_PROCESS_AND_OUTPUT_TEXT_FN)(HANDLE, PCYORI_STRING, YORI_MAX_UNSIGNED_T*);

/**
 A callback function to receive an escape and translate it into the
 appropriate action.
*/
typedef BOOL (* YORI_LIB_VT_PROCESS_AND_OUTPUT_ESCAPE_FN)(HANDLE, PCYORI_STRING, YORI_MAX_UNSIGNED_T*);

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

    /**
     Context which is passed between functions and is opaque to the VT engine.
     */
    YORI_MAX_UNSIGNED_T Context;

} YORI_LIB_VT_CALLBACK_FUNCTIONS, *PYORI_LIB_VT_CALLBACK_FUNCTIONS;

BOOL
YoriLibConsoleSetFn(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    );

BOOL
YoriLibConsoleNoEscSetFn(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    );

BOOL
YoriLibUtf8TextWithEscSetFn(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    );

BOOL
YoriLibUtf8TextNoEscSetFn(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    );

BOOL
YoriLibDbgSetFn(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    );

BOOL
YoriLibProcVtEscOnOpenStream(
    __in LPTSTR String,
    __in YORI_ALLOC_SIZE_T StringLength,
    __in HANDLE hOutput,
    __in PYORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks
    );

BOOL
YoriLibOutput(
    __in WORD Flags,
    __in LPCTSTR szFmt,
    ...
    );

BOOL
YoriLibOutputToDevice(
    __in HANDLE hOut,
    __in WORD Flags,
    __in LPCTSTR szFmt,
    ...
    );

BOOL
YoriLibOutputString(
    __in HANDLE hOut,
    __in WORD Flags,
    __in PYORI_STRING String
    );

BOOL
YoriLibVtSetConsoleTextAttrDev(
    __in HANDLE hOut,
    __in WORD Flags,
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
YoriLibVtSetConsoleTextAttr(
    __in WORD Flags,
    __in WORD Attribute
    );

VOID
YoriLibVtSetLineEnding(
    __in LPTSTR LineEnding
    );

LPTSTR
YoriLibVtGetLineEnding(VOID);

VOID
YoriLibVtSetDefaultColor(
    __in WORD NewDefaultColor
    );

WORD
YoriLibVtGetDefaultColor(VOID);

BOOL
YoriLibVtFinalColorFromEsc(
    __in WORD InitialColor,
    __in PCYORI_STRING EscapeSequence,
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
    __out_opt PWORD Width,
    __out_opt PWORD Height
    );

BOOL
YoriLibQueryConsoleCapabilities(
    __in HANDLE OutputHandle,
    __out_opt PBOOL SupportsColor,
    __out_opt PBOOL SupportsExtendedChars,
    __out_opt PBOOL SupportsAutoLineWrap
    );


// MSFIX Out of order here

// vim:sw=4:ts=4:et:
