/**
 * @file hash/hash.c
 *
 * Yori shell hash a file
 *
 * Copyright (c) 2019 Malcolm J. Smith
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
HashHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Hash %i.%02i\n"), HASH_VER_MAJOR, HASH_VER_MINOR);
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
    BOOL Recursive;

    /**
     BCrypt handle to the algorithm provider.  If NULL, the algorithm provider
     has not been initialized.
     */
    PVOID Algorithm;

    /**
     Pointer to an opaque blob of memory which is used by BCrypt to generate
     the hash.
     */
    PVOID ScratchBuffer;

    /**
     Specifies the number of bytes in ScratchBuffer.
     */
    DWORD ScratchBufferLength;

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
    LONG Status;
    PVOID hHash;
    DWORD BytesRead;

    Status = DllBCrypt.pBCryptCreateHash(HashContext->Algorithm, &hHash, HashContext->ScratchBuffer, HashContext->ScratchBufferLength, NULL, 0, 0);

    if (Status != STATUS_SUCCESS) {
        return FALSE;
    }

    Status = STATUS_SUCCESS;

    while (TRUE) {
        if (!ReadFile(hSource, HashContext->ReadBuffer, HashContext->ReadBufferLength, &BytesRead, NULL)) {
            break;
        }

        if (BytesRead == 0) {
            break;
        }

        Status = DllBCrypt.pBCryptHashData(hHash, HashContext->ReadBuffer, BytesRead, 0);
        if (Status != STATUS_SUCCESS) {
            break;
        }

    }

    if (Status == STATUS_SUCCESS) {
        Status = DllBCrypt.pBCryptFinishHash(hHash, HashContext->HashBuffer, HashContext->HashLength, 0);
        if (Status == STATUS_SUCCESS) {
            if (!YoriLibHexBufferToString(HashContext->HashBuffer, HashContext->HashLength, &HashContext->HashString)) {
                Status = !(STATUS_SUCCESS);
            }
        }
    }

    DllBCrypt.pBCryptDestroyHash(hHash);

    if (Status != STATUS_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}

/**
 A callback that is invoked when a file is found within the tree root whose
 hash is requested.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Indicates the recursion depth.

 @param Context Pointer to a context block specifying the hash to generate.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
HashFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
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
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: open of %y failed: %s"), FilePath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return TRUE;
    }

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
    LONG Status;

    if (HashContext->ScratchBuffer != NULL) {
        YoriLibFree(HashContext->ScratchBuffer);
        HashContext->ScratchBuffer = NULL;
    }

    if (HashContext->HashBuffer != NULL) {
        YoriLibFree(HashContext->HashBuffer);
        HashContext->HashBuffer = NULL;
    }

    if (HashContext->ReadBuffer != NULL) {
        YoriLibFree(HashContext->ReadBuffer);
        HashContext->ReadBuffer = NULL;
    }

    YoriLibFreeStringContents(&HashContext->HashString);

    if (HashContext->Algorithm != NULL) {
        Status = DllBCrypt.pBCryptCloseAlgorithmProvider(HashContext->Algorithm, 0);
        ASSERT(Status == STATUS_SUCCESS);
        HashContext->Algorithm = NULL;
    }
}

/**
 Allocate any internal allocations within the hash context needed for the
 specified hash algorithm.

 @param HashContext Pointer to the hash context to initialize.

 @param Algorithm Specifies a NULL terminated string indicating the BCrypt
        hash algorithm to initialize.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
HashInitializeContext(
    __in PHASH_CONTEXT HashContext,
    __in LPCWSTR Algorithm
    )
{
    LONG Status;
    DWORD BytesReturned;

    Status = DllBCrypt.pBCryptOpenAlgorithmProvider(&HashContext->Algorithm, Algorithm, MS_PRIMITIVE_PROVIDER, 0);
    if (Status != STATUS_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: algorithm provider not functional, status 0x%08x\n"), Status);
        HashCleanupContext(HashContext);
        return FALSE;
    }

    Status = DllBCrypt.pBCryptGetProperty(HashContext->Algorithm, L"HashDigestLength", &HashContext->HashLength, sizeof(HashContext->HashLength), &BytesReturned, 0);
    if (Status != STATUS_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: algorithm provider did not return required hash length, status 0x%08x\n"), Status);
        HashCleanupContext(HashContext);
        return FALSE;
    }

    HashContext->HashBuffer = YoriLibMalloc(HashContext->HashLength);
    if (HashContext->HashBuffer == NULL) {
        HashCleanupContext(HashContext);
        return FALSE;
    }

    Status = DllBCrypt.pBCryptGetProperty(HashContext->Algorithm, L"ObjectLength", &HashContext->ScratchBufferLength, sizeof(HashContext->ScratchBufferLength), &BytesReturned, 0);
    if (Status != STATUS_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: algorithm provider did not return required scratch space, status 0x%08x\n"), Status);
        HashCleanupContext(HashContext);
        return FALSE;
    }

    HashContext->ScratchBuffer = YoriLibMalloc(HashContext->ScratchBufferLength);
    if (HashContext->ScratchBuffer == NULL) {
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
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File or directory not found: %y\n"), &UnescapedFilePath);
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
    LPTSTR Algorithm = L"SHA1";

    ZeroMemory(&HashContext, sizeof(HashContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                HashHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                if (i + 1 < ArgC) {
                    if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("MD4")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = _T("MD4");
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("MD5")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = _T("MD5");
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("SHA1")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = _T("SHA1");
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("SHA256")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = _T("SHA256");
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("SHA384")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = _T("SHA384");
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], _T("SHA512")) == 0) {
                        ArgumentUnderstood = TRUE;
                        i++;
                        Algorithm = _T("SHA512");
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

    YoriLibLoadBCryptFunctions();
    if (DllBCrypt.pBCryptCloseAlgorithmProvider == NULL ||
        DllBCrypt.pBCryptCreateHash == NULL ||
        DllBCrypt.pBCryptDestroyHash == NULL ||
        DllBCrypt.pBCryptFinishHash == NULL ||
        DllBCrypt.pBCryptGetProperty == NULL ||
        DllBCrypt.pBCryptHashData == NULL ||
        DllBCrypt.pBCryptOpenAlgorithmProvider == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hash: operating system support not present\n"));
        return EXIT_FAILURE;
    }

    if (!HashInitializeContext(&HashContext, Algorithm)) {
        return EXIT_FAILURE;
    }

#if YORI_BUILTIN
    YoriLibCancelEnable();
#endif

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0) {
        DWORD FileType = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
        FileType = FileType & ~(FILE_TYPE_REMOTE);
        if (FileType == FILE_TYPE_CHAR) {
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
            YoriLibForEachStream(&ArgV[i],
                                 MatchFlags,
                                 0,
                                 HashFileFoundCallback,
                                 HashFileEnumerateErrorCallback,
                                 &HashContext);
        }
    }
    HashCleanupContext(&HashContext);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
