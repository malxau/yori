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

/**
 If the compiler has unsigned 64 bit values, use them for file size.
 */
typedef DWORDLONG SDIR_FILESIZE;

/**
 Macro to return the file size from a large integer.
 */
#define SdirFileSizeFromLargeInt(Li) ((Li)->QuadPart)

//
//  File size lengths
//

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
 If any bits here are set, the requested color will be rejected.
 */
#define SDIR_ATTRCTRL_INVALID_METADATA ~(YORILIB_ATTRCTRL_FILE|YORILIB_ATTRCTRL_WINDOW_BG|YORILIB_ATTRCTRL_WINDOW_FG)

/**
 Any bits set here will be ignored when applying color information into
 files.
 */
#define SDIR_ATTRCTRL_INVALID_FILE     ~(YORILIB_ATTRCTRL_INVERT|YORILIB_ATTRCTRL_HIDE|YORILIB_ATTRCTRL_CONTINUE|YORILIB_ATTRCTRL_WINDOW_BG|YORILIB_ATTRCTRL_WINDOW_FG)

/**
 Specifies a pointer to a function which can return the amount of characters
 needed to display a piece of file metadata.
 */
typedef YORI_ALLOC_SIZE_T (* SDIR_METADATA_WIDTH_FN)(PSDIR_FMTCHAR, YORILIB_COLOR_ATTRIBUTES, PYORI_FILE_INFO);

/**
 Specifies a pointer to a function which can compare two directory entries
 in some fashion.
 */
typedef DWORD (* SDIR_COMPARE_FN)(PYORI_FILE_INFO, PYORI_FILE_INFO);

/**
 Specifies a pointer to a function which can collect file information from
 the disk or file system for some particular piece of data.
 */
typedef BOOL (* SDIR_COLLECT_FN)(PYORI_FILE_INFO, PWIN32_FIND_DATA, PYORI_STRING);

/**
 Specifies a pointer to a function which can generate in memory file
 information from a user provided string.
 */
typedef BOOL (* SDIR_GENERATE_FROM_STRING_FN)(PYORI_FILE_INFO, PYORI_STRING);


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
     Can be set to YORI_LIB_EQUAL, YORI_LIB_GREATER_THAN, YORI_LIB_LESS_THAN.
     Items are inserted into order by comparing against existing entries
     until this condition is met, at which point the item can be inserted
     into its sorted position.
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

#pragma pack(push, 4)

/**
 A structure describing volatile, run time state describing program execution.
 */
typedef struct _SDIR_OPTS {

    /**
     Points to a user supplied string indicating file criteria to display
     with different colors.
     */
    YORI_STRING     CustomFileColor;

    /**
     Points to a user supplied string indicating file criteria to not
     display.
     */
    YORI_STRING     CustomFileFilter;

    /**
     Specifies the name of the directory that is currently being enumerated.
     This can change during execution.
     */
    YORI_STRING     ParentName;

    /**
     TRUE if any specification should be recursively enumerated, or
     FALSE to only display a single level.
     */
    BOOL            Recursive;

    /**
     Specifies the effective height of the console window, in characters.
     */
    WORD            ConsoleHeight;

    /**
     Specifies the effective width of the console buffer, in characters.
     Note this can be larger than the window width (implying horizontal
     scroll bars.)
     */
    WORD            ConsoleBufferWidth;

    /**
     Specifies the effective width of the console window, in characters.
     */
    WORD            ConsoleWidth;

    /**
     Specifies the total number of characters needed to display file metadata
     (ie., not the file's name.)
     */
    WORD            MetadataWidth;

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
     The volatile configuration for the directory's case sensitivity.
     */
    SDIR_FEATURE    FtCaseSensitivity;

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
     The volatile configuration for the executable's description.
     */
    SDIR_FEATURE    FtDescription;

    /**
     The volatile configuration for file's directory attribute.
     */
    SDIR_FEATURE    FtDirectory;

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
     The volatile configuration for the file's version string.
     */
    SDIR_FEATURE    FtFileVersionString;

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
     TRUE if enumeration should be basic only, treating {} and [] characters
     as literals.
     */
    BOOLEAN         BasicEnumeration:1;

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

} SDIR_OPTS, *PSDIR_OPTS;
#pragma pack(pop)

/**
 Summary information.  This is used to display the final line after
 enumerating a single directory.
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

/**
 A structure containing state that is global for each instance of sdir.
 */
typedef struct _SDIR_GLOBAL {

    /**
     An array of structures describing the criteria to use when applying
     colors to files.
     */
    YORI_LIB_FILE_FILTER FileColorCriteria;

    /**
     An array of structures describing the criteria to use to determine
     which files to hide.
     */
    YORI_LIB_FILE_FILTER FileHideCriteria;
} SDIR_GLOBAL, *PSDIR_GLOBAL;

extern SDIR_GLOBAL SdirGlobal;

extern PSDIR_OPTS Opts;
extern PSDIR_SUMMARY Summary;
extern const SDIR_OPT SdirOptions[];
extern const SDIR_EXEC SdirExec[];
extern PYORI_FILE_INFO SdirDirCollection;
extern PYORI_FILE_INFO * SdirDirSorted;
extern WORD SdirWriteStringLinesDisplayed;

//
//  Functions from display.c
//

BOOL
SdirWriteRawStringToOutputDevice(
    __in HANDLE hConsole,
    __in LPCTSTR OutputString,
    __in YORI_ALLOC_SIZE_T Length
    );

BOOL
SdirSetConsoleTextAttribute(
    __in HANDLE hConsole,
    __in YORILIB_COLOR_ATTRIBUTES Attribute
    );

BOOL
SdirWrite (
    __in_ecount(count) PSDIR_FMTCHAR str,
    __in YORI_ALLOC_SIZE_T count
    );

BOOL
SdirPasteStrAndPad (
    __out_ecount(padsize) PSDIR_FMTCHAR str,
    __in_ecount_opt(count) LPTSTR src,
    __in YORILIB_COLOR_ATTRIBUTES attr,
    __in YORI_ALLOC_SIZE_T count,
    __in YORI_ALLOC_SIZE_T padsize
    );

BOOL
SdirPressAnyKey (VOID);

BOOL
SdirRowDisplayed(VOID);

BOOL
SdirWriteStringWithAttribute (
    __in LPCTSTR str,
    __in YORILIB_COLOR_ATTRIBUTES DefaultAttribute
    );

BOOL
SdirDisplayError (
    __in LONG ErrorCode,
    __in_opt LPCTSTR Prefix
    );

BOOL
SdirDisplayYsError (
    __in LONG ErrorCode,
    __in PYORI_STRING YsPrefix
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

YORI_ALLOC_SIZE_T
SdirGetNumSdirOptions(VOID);

YORI_ALLOC_SIZE_T
SdirGetNumSdirExec(VOID);

YORI_ALLOC_SIZE_T
SdirDisplaySummary(
    __in YORILIB_COLOR_ATTRIBUTES DefaultAttributes
    );

BOOL
SdirCollectSummary(
    __in PYORI_FILE_INFO Entry
    );

YORI_ALLOC_SIZE_T
SdirDisplayShortName (
    PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PYORI_FILE_INFO Entry
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
    __in YORI_ALLOC_SIZE_T StringSize
    );

BOOL
SdirParseAttributeApplyString(VOID);

BOOL
SdirParseMetadataAttributeString(VOID);

BOOL
SdirApplyAttribute(
    __in PYORI_FILE_INFO DirEnt,
    __in BOOL ForceDisplay,
    __out PYORILIB_COLOR_ATTRIBUTES Attribute
    );

//
//  Functions from init.c
//

BOOL
SdirInit(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    );

VOID
SdirAppCleanup(VOID);

//
//  Functions from usage.c
//

BOOL
SdirUsage(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    );

//
//  Functions from utils.c
//

DWORD
SdirStringToNum32(
    __in LPCTSTR str,
    __out_opt LPCTSTR* endptr
    );

YORI_ALLOC_SIZE_T
SdirDisplayGenericSize(
    __out_ecount(6) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PLARGE_INTEGER GenericSize
    );

YORI_ALLOC_SIZE_T
SdirDisplayHex64 (
    __out_ecount(18) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PLARGE_INTEGER Hex
    );

YORI_ALLOC_SIZE_T
SdirDisplayHex32 (
    __out_ecount(9) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in DWORD Hex
    );

YORI_ALLOC_SIZE_T
SdirDisplayGenericHexBuffer (
    __out_ecount(Size * 2 + 1) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in PUCHAR InputBuffer,
    __in YORI_ALLOC_SIZE_T Size
    );

YORI_ALLOC_SIZE_T
SdirDisplayFileDate (
    __out_ecount(11) PSDIR_FMTCHAR Buffer,
    __in YORILIB_COLOR_ATTRIBUTES Attributes,
    __in SYSTEMTIME * Time
    );

YORI_ALLOC_SIZE_T
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
