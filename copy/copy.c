/**
 * @file copy/copy.c
 *
 * Yori shell perform simple math operations
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, COPYESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>

#ifndef SE_CREATE_SYMBOLIC_LINK_NAME
/**
 If the compilation environment hasn't defined it, define the privilege
 for creating symbolic links.
 */
#define SE_CREATE_SYMBOLIC_LINK_NAME _T("SeCreateSymbolicLinkPrivilege")
#endif

#ifndef FILE_FLAG_OPEN_NO_RECALL
/**
 If the compilation environment hasn't defined it, define the flag for not
 recalling objects from slow storage.
 */
#define FILE_FLAG_OPEN_NO_RECALL 0x100000
#endif

/**
 Help text to display to the user.
 */
const
CHAR strHelpText[] =
        "\n"
        "Copies one or more files.\n"
        "\n"
        "COPY [-license] [-b] [-c:algorithm] [-l] [-s] [-v] [-x exclude] <src>\n"
        "COPY [-license] [-b] [-c:algorithm] [-l] [-s] [-v] [-x exclude]\n"
        "      <src> [<src> ...] <dest>\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Compress targets with specified algorithm.  Options are:\n"
        "                    lzx, ntfs, xp4k, xp8k, xp16k\n"
        "   -l             Copy links as links rather than contents\n"
        "   -s             Copy subdirectories as well as files\n"
        "   -v             Verbose output\n"
        "   -x             Exclude files matching specified pattern\n";

/**
 Display usage text to the user.
 */
BOOL
CopyHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Copy %i.%i\n"), COPY_VER_MAJOR, COPY_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 A single item to exclude.  Note this can refer to multiple files.
 */
typedef struct _COPY_EXCLUDE_ITEM {

    /**
     List of items to exclude.
     */
    YORI_LIST_ENTRY ExcludeList;

    /**
     A string describing the object to exclude, which may include
     wildcards.
     */
    YORI_STRING ExcludeCriteria;
} COPY_EXCLUDE_ITEM, *PCOPY_EXCLUDE_ITEM;

/**
 A context passed between each source file match when copying multiple
 files.
 */
typedef struct _COPY_CONTEXT {
    /**
     Path to the destination for the copy operation.
     */
    YORI_STRING Dest;

    /**
     Files matching any of the exclude rules will not be copied.
     */
    YORI_LIST_ENTRY ExcludeList;

    /**
     State related to background compression of files after copy.
     */
    YORILIB_COMPRESS_CONTEXT CompressContext;

    /**
     The file system attributes of the destination.  Used to determine if
     the destination exists and is a directory.
     */
    DWORD DestAttributes;

    /**
     The number of files that have been previously copied to this
     destination.  This can be used to determine if we're about to copy
     a second object over the top of an earlier copied file.
     */
    DWORD FilesCopied;

    /**
     If TRUE, targets should be compressed.
     */
    BOOL CompressDest;

    /**
     If TRUE, links are copied as links rather than having their contents
     copied.
     */
    BOOL CopyAsLinks;

    /**
     If TRUE, output is generated for each object copied.
     */
    BOOL Verbose;
} COPY_CONTEXT, *PCOPY_CONTEXT;

/**
 Add a new exclude criteria to the list.

 @param CopyContext Pointer to the copy context to populate with a new
        exclude criteria.

 @param NewCriteria Pointer to the new criteria to add, which may include
        wildcards.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CopyAddExclude(
    __in PCOPY_CONTEXT CopyContext,
    __in PYORI_STRING NewCriteria
    )
{
    PCOPY_EXCLUDE_ITEM ExcludeItem;
    ExcludeItem = YoriLibReferencedMalloc(sizeof(COPY_EXCLUDE_ITEM) + (NewCriteria->LengthInChars + 1) * sizeof(TCHAR));

    if (ExcludeItem == NULL) {
        return FALSE;
    }

    ZeroMemory(ExcludeItem, sizeof(COPY_EXCLUDE_ITEM));
    ExcludeItem->ExcludeCriteria.StartOfString = (LPTSTR)(ExcludeItem + 1);
    ExcludeItem->ExcludeCriteria.LengthInChars = NewCriteria->LengthInChars;
    ExcludeItem->ExcludeCriteria.LengthAllocated = NewCriteria->LengthInChars + 1;
    memcpy(ExcludeItem->ExcludeCriteria.StartOfString, NewCriteria->StartOfString, ExcludeItem->ExcludeCriteria.LengthInChars * sizeof(TCHAR));
    ExcludeItem->ExcludeCriteria.StartOfString[ExcludeItem->ExcludeCriteria.LengthInChars] = '\0';
    YoriLibAppendList(&CopyContext->ExcludeList, &ExcludeItem->ExcludeList);
    return TRUE;
}

/**
 Free all previously added exclude criteria.

 @param CopyContext Pointer to the copy context to free all exclude criteria
        from.
 */
VOID
CopyFreeExcludes(
    __in PCOPY_CONTEXT CopyContext
    )
{
    PCOPY_EXCLUDE_ITEM ExcludeItem;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&CopyContext->ExcludeList, NULL);
    while (ListEntry != NULL) {
        ExcludeItem = CONTAINING_RECORD(ListEntry, COPY_EXCLUDE_ITEM, ExcludeList);
        YoriLibRemoveListItem(&ExcludeItem->ExcludeList);
        YoriLibDereference(ExcludeItem);
        ListEntry = YoriLibGetNextListEntry(&CopyContext->ExcludeList, NULL);
    }
}

/**
 Returns TRUE to indicate that an object should be excluded based on the
 exclude criteria, or FALSE if it should be included.

 @param CopyContext Pointer to the copy context to check the new object
        against.

 @param RelativeSourcePath Pointer to a string describing the file relative
        to the root of the source of the copy operation.

 @return TRUE to exclude the file, FALSE to include it.
 */
BOOL
CopyShouldExclude(
    __in PCOPY_CONTEXT CopyContext,
    __in PYORI_STRING RelativeSourcePath
    )
{
    PCOPY_EXCLUDE_ITEM ExcludeItem;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = YoriLibGetNextListEntry(&CopyContext->ExcludeList, NULL);
    while (ListEntry != NULL) {
        ExcludeItem = CONTAINING_RECORD(ListEntry, COPY_EXCLUDE_ITEM, ExcludeList);
        if (YoriLibDoesFileMatchExpression(RelativeSourcePath, &ExcludeItem->ExcludeCriteria)) {
            return TRUE;
        }
        ListEntry = YoriLibGetNextListEntry(&CopyContext->ExcludeList, ListEntry);
    }
    return FALSE;
}

/**
 Copy a single file from the source to the target by preserving its link
 contents.

 @param SourceFileName A NULL terminated string specifying the source file.

 @param DestFileName A NULL terminated string specifying the destination file.

 @param IsDirectory TRUE if the object being copied is a directory, FALSE if
        it is not.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CopyAsLink(
    __in LPTSTR SourceFileName,
    __in LPTSTR DestFileName,
    __in BOOL IsDirectory
    )
{
    PVOID ReparseData;
    HANDLE SourceFileHandle;
    HANDLE DestFileHandle;
    DWORD LastError;
    DWORD BytesReturned;
    LPTSTR ErrText;

    SourceFileHandle = CreateFile(SourceFileName,
                                  GENERIC_READ,
                                  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL|FILE_FLAG_BACKUP_SEMANTICS,
                                  NULL);

    if (SourceFileHandle == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %s: %s"), SourceFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    if (IsDirectory) {
        if (!CreateDirectory(DestFileName, NULL)) {
            LastError = GetLastError();
            if (LastError != ERROR_ALREADY_EXISTS) {
                ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Create of destination failed: %s: %s"), DestFileName, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                CloseHandle(SourceFileHandle);
                return FALSE;
            }
        }

        DestFileHandle = CreateFile(DestFileName,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL|FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL);
    
        if (DestFileHandle == INVALID_HANDLE_VALUE) {
            LastError = GetLastError();
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of destination failed: %s: %s"), DestFileName, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            CloseHandle(SourceFileHandle);
            RemoveDirectory(DestFileName);
            return FALSE;
        }
    } else {
        DestFileHandle = CreateFile(DestFileName,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_OPEN_NO_RECALL|FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL);
    
        if (DestFileHandle == INVALID_HANDLE_VALUE) {
            LastError = GetLastError();
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of destination failed: %s: %s"), DestFileName, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            CloseHandle(SourceFileHandle);
            return FALSE;
        }
    }

    ReparseData = YoriLibMalloc(64 * 1024);
    if (ReparseData == NULL) {
        CloseHandle(DestFileHandle);
        CloseHandle(SourceFileHandle);
        if (IsDirectory) {
            RemoveDirectory(DestFileName);
        } else {
            DeleteFile(DestFileName);
        }
        return FALSE;
    }

    if (!DeviceIoControl(SourceFileHandle, FSCTL_GET_REPARSE_POINT, NULL, 0, ReparseData, 64 * 1024, &BytesReturned, NULL)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Querying reparse data of source failed: %s: %s"), SourceFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(SourceFileHandle);
        CloseHandle(DestFileHandle);
        YoriLibFree(ReparseData);
        if (IsDirectory) {
            RemoveDirectory(DestFileName);
        } else {
            DeleteFile(DestFileName);
        }
        return FALSE;
    }

    if (!DeviceIoControl(DestFileHandle, FSCTL_SET_REPARSE_POINT, ReparseData, BytesReturned, NULL, 0, &BytesReturned, NULL)) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Setting reparse data on dest failed: %s: %s"), DestFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(SourceFileHandle);
        CloseHandle(DestFileHandle);
        YoriLibFree(ReparseData);
        if (IsDirectory) {
            RemoveDirectory(DestFileName);
        } else {
            DeleteFile(DestFileName);
        }
        return FALSE;
    }

    CloseHandle(SourceFileHandle);
    CloseHandle(DestFileHandle);
    YoriLibFree(ReparseData);
    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Indicates the recursion depth.  Used by copy to check if it
        needs to create new directories in the destination path.

 @param Context Pointer to a context block specifying the destination of the
        copy, indicating parameters to the copy operation, and tracking how
        many objects have been copied.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
CopyFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PCOPY_CONTEXT CopyContext = (PCOPY_CONTEXT)Context;
    YORI_STRING RelativePathFromSource;
    YORI_STRING FullDest;
    YORI_STRING HumanSourcePath;
    YORI_STRING HumanDestPath;
    PYORI_STRING SourceNameToDisplay;
    PYORI_STRING DestNameToDisplay;
    DWORD SlashesFound;
    DWORD Index;

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    YoriLibInitEmptyString(&FullDest);
    YoriLibInitEmptyString(&RelativePathFromSource);
    YoriLibInitEmptyString(&HumanSourcePath);
    YoriLibInitEmptyString(&HumanDestPath);
    SourceNameToDisplay = FilePath;

    SlashesFound = 0;
    for (Index = FilePath->LengthInChars; Index > 0; Index--) {
        if (FilePath->StartOfString[Index - 1] == '\\') {
            SlashesFound++;
            if (SlashesFound == Depth + 1) {
                break;
            }
        }
    }

    ASSERT(Index > 0);
    ASSERT(SlashesFound == Depth + 1);

    RelativePathFromSource.StartOfString = &FilePath->StartOfString[Index];
    RelativePathFromSource.LengthInChars = FilePath->LengthInChars - Index;

    //
    //  Check if the user wanted to exclude this file
    //

    if (CopyShouldExclude(CopyContext, &RelativePathFromSource)) {

        if (CopyContext->Verbose) {
            if (YoriLibUnescapePath(FilePath, &HumanSourcePath)) {
                SourceNameToDisplay = &HumanSourcePath;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Skipping %y\n"), SourceNameToDisplay);
            YoriLibFreeStringContents(&HumanSourcePath);
        }

        return TRUE;
    }

    //
    //  If the target is a directory, construct a full path to the object
    //  within the target's directory tree.  Otherwise, the target is just
    //  a regular file with no path.
    //

    if (CopyContext->DestAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        YORI_STRING DestWithFile;

        if (!YoriLibAllocateString(&DestWithFile, CopyContext->Dest.LengthInChars + 1 + RelativePathFromSource.LengthInChars + 1)) {
            return FALSE;
        }
        DestWithFile.LengthInChars = YoriLibSPrintf(DestWithFile.StartOfString, _T("%y\\%y"), &CopyContext->Dest, &RelativePathFromSource);
        if (!YoriLibGetFullPathNameReturnAllocation(&DestWithFile, TRUE, &FullDest, NULL)) {
            return FALSE;
        }
        YoriLibFreeStringContents(&DestWithFile);
    } else {
        if (!YoriLibGetFullPathNameReturnAllocation(&CopyContext->Dest, TRUE, &FullDest, NULL)) {
            return FALSE;
        }
        if (CopyContext->FilesCopied > 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Attempting to copy multiple files over a single file (%s)\n"), FullDest.StartOfString);
            YoriLibFreeStringContents(&FullDest);
            return FALSE;
        }
    }

    DestNameToDisplay = &FullDest;

    if (CopyContext->Verbose) {
        if (YoriLibUnescapePath(FilePath, &HumanSourcePath)) {
            SourceNameToDisplay = &HumanSourcePath;
        }
        if (YoriLibUnescapePath(&FullDest, &HumanDestPath)) {
            DestNameToDisplay = &HumanDestPath;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Copying %y to %y\n"), SourceNameToDisplay, DestNameToDisplay);
    }

    if (FileInfo->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT &&
        CopyContext->CopyAsLinks &&
        (FileInfo->dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT || FileInfo->dwReserved0 == IO_REPARSE_TAG_SYMLINK)) {

        CopyAsLink(FilePath->StartOfString, FullDest.StartOfString, ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0));

    } else if (FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (!CreateDirectory(FullDest.StartOfString, NULL)) {
            DWORD LastError = GetLastError();
            if (LastError != ERROR_ALREADY_EXISTS) {
                LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CreateDirectory failed: %s: %s"), FullDest.StartOfString, ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
        }
    } else {
        if (!CopyFile(FilePath->StartOfString, FullDest.StartOfString, FALSE)) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            if (SourceNameToDisplay != &HumanSourcePath) {
                if (YoriLibUnescapePath(FilePath, &HumanSourcePath)) {
                    SourceNameToDisplay = &HumanSourcePath;
                }
            }
            if (DestNameToDisplay != &HumanDestPath) {
                if (YoriLibUnescapePath(&FullDest, &HumanDestPath)) {
                    DestNameToDisplay = &HumanDestPath;
                }
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CopyFile failed: %y to %y: %s"), SourceNameToDisplay, DestNameToDisplay, ErrText);
            YoriLibFreeWinErrorText(ErrText);
        }

        if (CopyContext->CompressDest) {

            YoriLibCompressFileInBackground(&CopyContext->CompressContext, &FullDest);
        }
    }

    CopyContext->FilesCopied++;
    YoriLibFreeStringContents(&FullDest);
    YoriLibFreeStringContents(&HumanSourcePath);
    YoriLibFreeStringContents(&HumanDestPath);
    return TRUE;
}

/**
 If the privilege for creating symbolic links is available to the process,
 enable it.

 @return TRUE to indicate the privilege was enabled, FALSE if it was not.
 */
BOOL
CopyEnableSymlinkPrivilege()
{
    TOKEN_PRIVILEGES TokenPrivileges;
    LUID SymlinkLuid;
    HANDLE TokenHandle;

    YoriLibLoadAdvApi32Functions();
    if (DllAdvApi32.pLookupPrivilegeValueW == NULL ||
        DllAdvApi32.pOpenProcessToken == NULL ||
        DllAdvApi32.pAdjustTokenPrivileges == NULL) {

        return FALSE;
    }

    if (!DllAdvApi32.pLookupPrivilegeValueW(NULL, SE_CREATE_SYMBOLIC_LINK_NAME, &SymlinkLuid)) {
        return FALSE;
    }

    if (!DllAdvApi32.pOpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &TokenHandle)) {
        return FALSE;
    }

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = SymlinkLuid;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!DllAdvApi32.pAdjustTokenPrivileges(TokenHandle, FALSE, &TokenPrivileges, 0, NULL, 0)) {
        CloseHandle(TokenHandle);
        return FALSE;
    }

    return TRUE;
}

/**
 Free the structures allocated within a copy context.  The structure itself
 is on the stack and is not freed.  This will wait for any outstanding
 compression work to complete.

 @param CopyContext Pointer to the context to free.
 */
VOID
CopyFreeCopyContext(
    __in PCOPY_CONTEXT CopyContext
    )
{
    YoriLibFreeCompressContext(&CopyContext->CompressContext);
    YoriLibFreeStringContents(&CopyContext->Dest);
    CopyFreeExcludes(CopyContext);
}

/**
 The main entrypoint for the copy cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD FilesProcessed;
    DWORD FileCount;
    DWORD LastFileArg = 0;
    DWORD FirstFileArg = 0;
    DWORD MatchFlags;
    BOOL BasicEnumeration;
    BOOL Recursive;
    DWORD i;
    DWORD Result;
    COPY_CONTEXT CopyContext;
    YORILIB_COMPRESS_ALGORITHM CompressionAlgorithm;
    YORI_STRING Arg;

    FileCount = 0;
    Recursive = FALSE;
    BasicEnumeration = FALSE;
    ZeroMemory(&CopyContext, sizeof(CopyContext));
    CompressionAlgorithm.EntireAlgorithm = 0;

    YoriLibInitializeListHead(&CopyContext.ExcludeList);

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));
        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                CopyHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:lzx")) == 0) {

                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_LZX;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:ntfs")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.NtfsAlgorithm = COMPRESSION_FORMAT_DEFAULT;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xpress")) == 0 ||
                       YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xp4k")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS4K;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xp8k")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS8K;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xp16k")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS16K;
                CopyContext.CompressDest = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                CopyContext.CopyAsLinks = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("v")) == 0) {
                CopyContext.Verbose = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("x")) == 0) {
                if (i + 1 < ArgC) {
                    CopyAddExclude(&CopyContext, &ArgV[i + 1]);
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            }
        } else {
            ArgumentUnderstood = TRUE;
            FileCount++;
            LastFileArg = i;
            if (FirstFileArg == 0) {
                FirstFileArg = i;
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (FileCount == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("copy: argument missing\n"));
        CopyFreeCopyContext(&CopyContext);
        return EXIT_FAILURE;
    } else if (FileCount == 1) {
        YoriLibConstantString(&CopyContext.Dest, _T("."));
    } else {
        if (!YoriLibUserStringToSingleFilePath(&ArgV[LastFileArg], TRUE, &CopyContext.Dest)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("copy: could not resolve %y\n"), &ArgV[LastFileArg]);
            CopyFreeCopyContext(&CopyContext);
            return EXIT_FAILURE;
        }
        FileCount--;
    }

    ASSERT(YoriLibIsStringNullTerminated(&CopyContext.Dest));

    if (CopyContext.CopyAsLinks) {
        if (!CopyEnableSymlinkPrivilege()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("copy: warning: could not enable symlink privilege\n"));
        }
    }

    CopyContext.DestAttributes = GetFileAttributes(CopyContext.Dest.StartOfString);
    if (CopyContext.DestAttributes == 0xFFFFFFFF) {
        if (Recursive) {
            if (!CreateDirectory(CopyContext.Dest.StartOfString, NULL)) {
                DWORD LastError = GetLastError();
                LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("CreateDirectory failed: %y: %s"), &CopyContext.Dest, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                CopyFreeCopyContext(&CopyContext);
                return EXIT_FAILURE;
            }
            CopyContext.DestAttributes = GetFileAttributes(CopyContext.Dest.StartOfString);
        }

        if (CopyContext.DestAttributes == 0xFFFFFFFF) {
            CopyContext.DestAttributes = 0;
        }
    }

    if (CopyContext.CompressDest) {
        if (!YoriLibInitializeCompressContext(&CopyContext.CompressContext, CompressionAlgorithm)) {
            CopyFreeCopyContext(&CopyContext);
            return EXIT_FAILURE;
        }
    }
    CopyContext.FilesCopied = 0;

    FilesProcessed = 0;

    for (i = FirstFileArg; i <= LastFileArg; i++) {
        if (!YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {
            MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
            if (BasicEnumeration) {
                MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
            }
            if (Recursive) {
                MatchFlags |= YORILIB_FILEENUM_RECURSE_AFTER_RETURN | YORILIB_FILEENUM_RETURN_DIRECTORIES;
                if (CopyContext.CopyAsLinks) {
                    MatchFlags |= YORILIB_FILEENUM_NO_LINK_TRAVERSE;
                }
            }

            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, CopyFileFoundCallback, &CopyContext);
            FilesProcessed++;
            if (FilesProcessed == FileCount) {
                break;
            }
        }
    }

    Result = EXIT_SUCCESS;

    if (CopyContext.FilesCopied == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("copy: no matching files found\n"));
        Result = EXIT_FAILURE;
    }

    CopyFreeCopyContext(&CopyContext);

    return Result;
}

// vim:sw=4:ts=4:et:
