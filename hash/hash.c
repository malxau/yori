/**
 * @file hash/hash.c
 *
 * Yori shell hash a file
 *
 * Copyright (c) 2019-2021 Malcolm J. Smith
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

/**
 Specifies the builtin Microsoft hash provider.
 */
#define MS_PRIMITIVE_PROVIDER L"Microsoft Primitive Provider"

/**
 Specifies the NT success error code.
 */
#define STATUS_SUCCESS (0)

/**
 Help text to display to the user.
 */
const
CHAR strHashHelpText[] =
        "\n"
        "Hash a file.\n"
        "\n"
        "HASH [-license] [-a <algorithm>] [-b] [-s] [<file>]\n"
        "\n"
        "   -a <algorithm> Specify the hash algorithm. Supported algorithms:\n"
        "                    MD4, MD5, SHA1, SHA256, SHA384, or SHA512\n"
        "   -b             Use basic search criteria for files only\n"
        "   -s             Hash files in subdirectories\n";

/**
 Display usage text to the user.
 */
BOOL
HashHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Hash %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHashHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _HASH_CONTEXT {

    /**
     TRUE if file enumeration is being performed recursively; FALSE if it is
     in one directory only.
     */
    BOOLEAN Recursive;

    /**
     WinCrypt handle to the algorithm provider.  If 0, the algorithm provider
     has not been initialized.
     */
    DWORD_PTR Provider;

    /**
     The first error encountered when enumerating objects from a single arg.
     This is used to preserve file not found/path not found errors so that
     when the program falls back to interpreting the argument as a literal,
     if that still doesn't work, this is the error code that is displayed.
     */
    DWORD SavedErrorThisArg;

    /**
     The algorithm to use in CALG_* format.
     */
    DWORD Algorithm;

    /**
     Pointer to a blob of memory containing the result of the hash calculation
     for each file.
     */
    PUCHAR HashBuffer;

    /**
     Specifies the number of bytes in HashBuffer.
     */
    DWORD HashLength;

    /**
     Pointer to a buffer to read data from the file into.
     */
    PVOID ReadBuffer;

    /**
     Specifies the number of bytes in ReadBuffer.
     */
    DWORD ReadBufferLength;

    /**
     A string which contains enough characters to contain the hex
     representation of HashBuffer plus a NULL terminator.
     */
    YORI_STRING HashString;

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     Records the total number of files processed within a single command line
     argument.
     */
    LONGLONG FilesFoundThisArg;

} HASH_CONTEXT, *PHASH_CONTEXT;

/**
 Take a single incoming stream and break it into pieces.

 @param hSource A handle to the incoming stream, which may be a file or a
        pipe.
 
 @param HashContext Pointer to a context describing the actions to perform.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
HashProcessStream(
    __in HANDLE hSource,
    __in PHASH_CONTEXT HashContext
    )
{
    DWORD Err;
    DWORD_PTR hHash;
    DWORD BytesRead;

    HashContext->FilesFound++;
    HashContext->FilesFoundThisArg++;

    if (!DllAdvApi32.pCryptCreateHash(HashContext->Provider, HashContext->Algorithm, 0, 0, &hHash)) {
        return FALSE;
    }


    Err = ERROR_SUCCESS;
    while (TRUE) {
        if (!ReadFile(hSource, HashContext->ReadBuffer, HashContext->ReadBufferLength, &BytesRead, NULL)) {
            // MSFIX: Distinguish errors here better? EOF means success,
            // read error means hash is wrong.  Could be reading from a pipe
            // etc though
            break;
        }

        if (BytesRead == 0) {
            break;
        }

        if (!DllAdvApi32.pCryptHashData(hHash, HashContext->ReadBuffer, BytesRead, 0)) {
            Err = GetLastError();
            break;
        }

    }

    if (Err == ERROR_SUCCESS) {
        DWORD HashLength;
        HashLength = HashContext->HashLength;
        if (DllAdvApi32.pCryptGetHashParam(hHash, HP_HASHVAL, HashContext->HashBuffer, &HashLength, 0)) {
            if (!YoriLibHexBufferToString(HashContext->HashBuffer, HashContext->HashLength, &HashContext->HashString)) {
                Err = !(ERROR_SUCCESS);
            }
        }
    }

    DllAdvApi32.pCryptDestroyHash(hHash);

    if (Err != STATUS_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}

/**
 A callback that is invoked when a file is found within the tree root whose
 hash is requested.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.  This can be NULL if the file
        was not found by enumeration.

 @param Depth Indicates the recursion depth.

 @param Context Pointer to a context block specifying the hash to generate.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
HashFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PHASH_CONTEXT HashContext = (PHASH_CONTEXT)Context;
    YORI_STRING RelativePathFrom;
    HANDLE FileHandle;
    DWORD SlashesFound;
    DWORD Index;

    UNREFERENCED_PARAMETER(FileInfo);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    YoriLibInitEmptyString(&RelativePathFrom);

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

    RelativePathFrom.StartOfString = &FilePath->StartOfString[Index];
    RelativePathFrom.LengthInChars = FilePath->LengthInChars - Index;

    FileHandle = CreateFile(FilePath->StartOfString,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        if (HashContext->SavedErrorThisArg == ERROR_SUCCESS) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
        }
        return TRUE;
    }

    HashContext->SavedErrorThisArg = ERROR_SUCCESS;

    if (HashProcessStream(FileHandle, HashContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y %y\n"), &HashContext->HashString, &RelativePathFrom);
    }

    CloseHandle(FileHandle);
    return TRUE;
}


/**
 Cleanup any internal allocations within the hash context.  The context
 itself is a stack allocation and is not freed.

 @param HashContext Pointer to the hash context to clean up.
 */
VOID
HashCleanupContext(
    __in PHASH_CONTEXT HashContext
    )
{
    BOOL Result;

    if (HashContext->HashBuffer != NULL) {
        YoriLibFree(HashContext->HashBuffer);
        HashContext->HashBuffer = NULL;
    }

    if (HashContext->ReadBuffer != NULL) {
        YoriLibFree(HashContext->ReadBuffer);
        HashContext->ReadBuffer = NULL;
    }

    YoriLibFreeStringContents(&HashContext->HashString);

    if (HashContext->Provider != 0) {
        Result = DllAdvApi32.pCryptReleaseContext(HashContext->Provider, 0);
        ASSERT(Result);
        HashContext->Provider = 0;
    }
}

/**
 A structure that describes a hashing provider and options to use it.  Newer
 systems include newer providers and capabilities, so this is used to allow
 the code to work backwards in time until it finds something that works.
 */
typedef struct _HASH_ACQUIRE_CONFIG {

    /**
     The provider name.
     */
    LPCTSTR Provider;

    /**
     The provider type.
     */
    DWORD ProviderType;

    /**
     Options to open the provider with.
     */
    DWORD Flags;
} HASH_ACQUIRE_CONFIG, *PHASH_ACQUIRE_CONFIG;

/**
 A table of providers, ordered from newest to oldest.  The code will
 iterate through this table until it finds one that works.
 */
CONST HASH_ACQUIRE_CONFIG HashAcquireConfigOptions[] = {
    {MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT},    // Vista+
    {MS_ENH_RSA_AES_PROV_XP, PROV_RSA_AES, CRYPT_VERIFYCONTEXT}, // XP SP 3
    {MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT},           // 2000/NT SP ?
    {MS_DEF_PROV, PROV_RSA_FULL, 0}                              // NT 4 RTM
};

/**
 Allocate any internal allocations within the hash context needed for the
 specified hash algorithm.

 @param HashContext Pointer to the hash context to initialize.

 @param Algorithm Specifies a CALG_* algorithm indicating the algorithm to
        initialize.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
HashInitializeContext(
    __in PHASH_CONTEXT HashContext,
    __in DWORD Algorithm
    )
{
    DWORD_PTR hHash;
    DWORD LastError;
    LPTSTR ErrText;
    DWORD Index;

    LastError = ERROR_SUCCESS;

    //
    //  Iterate through the supported providers, looking for one that works.
    //

    for (Index = 0; Index < sizeof(HashAcquireConfigOptions)/sizeof(HashAcquireConfigOptions[0]); Index++) {
        if (DllAdvApi32.pCryptAcquireContextW(&HashContext->Provider,
                                              NULL,
                                              HashAcquireConfigOptions[Index].Provider,
                                              HashAcquireConfigOptions[Index].ProviderType,
                                              HashAcquireConfigOptions[Index].Flags)) {
            LastError = ERROR_SUCCESS;
            break;
        } else {
            LastError = GetLastError();

            //
            //  NTE_BAD_KEYSET indicates that a keyset may need to be created.
            //  The documentation suggests code should always handle this,
            //  although it's less clear on why.  In practice this appears
            //  necessary on NT 4 RTM (perhaps nothing else has used it first?)
            //
            if (LastError != (DWORD)NTE_BAD_KEYSET) {
                continue;
            }

            if (DllAdvApi32.pCryptAcquireContextW(&HashContext->Provider,
                                                  NULL,
                                                  HashAcquireConfigOptions[Index].Provider,
                                                  HashAcquireConfigOptions[Index].ProviderType,
                                                  HashAcquireConfigOptions[Index].Flags | CRYPT_NEWKEYSET)) {
                LastError = ERROR_SUCCESS;
                break;
            }
        }
    }

    if (LastError != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: algorithm provider not functional: %s\n"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        HashCleanupContext(HashContext);
        return FALSE;
    }

    if (!DllAdvApi32.pCryptCreateHash(HashContext->Provider, Algorithm, 0, 0, &hHash)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: operating system support not present\n"));
        HashCleanupContext(HashContext);
        return FALSE;
    }

    if (!DllAdvApi32.pCryptGetHashParam(hHash, HP_HASHVAL, NULL, &HashContext->HashLength, 0)) {
        LastError = GetLastError();
        if (LastError != ERROR_MORE_DATA) {
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: could not determine hash length: %s\n"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            DllAdvApi32.pCryptDestroyHash(hHash);
            HashCleanupContext(HashContext);
            return FALSE;
        }
    }

    DllAdvApi32.pCryptDestroyHash(hHash);

    HashContext->Algorithm = Algorithm;

    HashContext->HashBuffer = YoriLibMalloc(HashContext->HashLength);
    if (HashContext->HashBuffer == NULL) {
        HashCleanupContext(HashContext);
        return FALSE;
    }

    if (!YoriLibAllocateString(&HashContext->HashString, HashContext->HashLength * 2 + 1)) {
        HashCleanupContext(HashContext);
        return FALSE;
    }

    HashContext->ReadBufferLength = 1024 * 1024;

    HashContext->ReadBuffer = YoriLibMalloc(HashContext->ReadBufferLength);
    if (HashContext->ReadBuffer == NULL) {
        HashCleanupContext(HashContext);
        return FALSE;
    }

    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the context block indicating whether the
        enumeration was recursive.  Recursive enumerates do not complain
        if a matching file is not in every single directory, because
        common usage expects files to be in a subset of directories only.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
HashFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;
    PHASH_CONTEXT HashContext = (PHASH_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        if (!HashContext->Recursive) {
            HashContext->SavedErrorThisArg = ErrorCode;
        }
        Result = TRUE;
    } else {
        LPTSTR ErrText = YoriLibGetWinErrorText(ErrorCode);
        YORI_STRING DirName;
        LPTSTR FilePart;
        YoriLibInitEmptyString(&DirName);
        DirName.StartOfString = UnescapedFilePath.StartOfString;
        FilePart = YoriLibFindRightMostCharacter(&UnescapedFilePath, '\\');
        if (FilePart != NULL) {
            DirName.LengthInChars = (DWORD)(FilePart - DirName.StartOfString);
        } else {
            DirName.LengthInChars = UnescapedFilePath.LengthInChars;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Enumerate of %y failed: %s"), &DirName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
    }
    YoriLibFreeStringContents(&UnescapedFilePath);
    return Result;
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the hash builtin command.
 */
#define ENTRYPOINT YoriCmd_YHASH
#else
/**
 The main entrypoint for the hash standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the hash cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, typically zero for success and nonzero
         for failure.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    DWORD MatchFlags;
    BOOL BasicEnumeration = FALSE;
    HASH_CONTEXT HashContext;
    YORI_STRING Arg;
    DWORD Algorithm = CALG_SHA1;

    ZeroMemory(&HashContext, sizeof(HashContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                HashHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                if (i + 1 < ArgC) {
                    if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("MD4")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = CALG_MD4;
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("MD5")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = CALG_MD5;
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("SHA1")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = CALG_SHA1;
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("SHA256")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = CALG_SHA_256;
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("SHA384")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = CALG_SHA_384;
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("SHA512")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = CALG_SHA_512;
                    } else {
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: algorithm not recognized.  Supported algorithms are MD4, MD5, SHA1, SHA256, SHA384, and SHA512\n"));
                        return EXIT_FAILURE;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                HashContext.Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i + 1;
                ArgumentUnderstood = TRUE;
                break;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    YoriLibLoadAdvApi32Functions();
    if (DllAdvApi32.pCryptAcquireContextW == NULL ||
        DllAdvApi32.pCryptCreateHash == NULL ||
        DllAdvApi32.pCryptDestroyHash == NULL ||
        DllAdvApi32.pCryptGetHashParam == NULL ||
        DllAdvApi32.pCryptHashData == NULL ||
        DllAdvApi32.pCryptReleaseContext == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: operating system support not present\n"));
        return EXIT_FAILURE;
    }

    if (!HashInitializeContext(&HashContext, Algorithm)) {
        return EXIT_FAILURE;
    }

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    //
    //  Attempt to enable backup privilege so an administrator can access more
    //  objects successfully.
    //

    YoriLibEnableBackupPrivilege();

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0 || StartArg == ArgC) {
        if (YoriLibIsStdInConsole()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: no file or pipe for input\n"));
            HashCleanupContext(&HashContext);
            return EXIT_FAILURE;
        }

        if (!HashProcessStream(GetStdHandle(STD_INPUT_HANDLE), &HashContext)) {
            HashCleanupContext(&HashContext);
            return EXIT_FAILURE;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &HashContext.HashString);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }
        if (HashContext.Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_AFTER_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }

        for (i = StartArg; i < ArgC; i++) {

            HashContext.FilesFoundThisArg = 0;
            HashContext.SavedErrorThisArg = ERROR_SUCCESS;

            YoriLibForEachStream(&ArgV[i],
                                 MatchFlags,
                                 0,
                                 HashFileFoundCallback,
                                 HashFileEnumerateErrorCallback,
                                 &HashContext);

            if (HashContext.FilesFoundThisArg == 0) {
                YORI_STRING FullPath;
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    HashFileFoundCallback(&FullPath, NULL, 0, &HashContext);
                    YoriLibFreeStringContents(&FullPath);
                }
                if (HashContext.SavedErrorThisArg != ERROR_SUCCESS) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File or directory not found: %y\n"), &ArgV[i]);
                }
            }
        }
    }

    HashCleanupContext(&HashContext);

    if (HashContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
