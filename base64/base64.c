/**
 * @file base64/base64.c
 *
 * Yori shell base64 encode or decode
 *
 * Copyright (c) 2023 Malcolm J. Smith
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
 Help text to display to the user.
 */
const
CHAR strBase64HelpText[] =
        "\n"
        "Base64 encode or decode a file or standard input.\n"
        "\n"
        "HASH [-license] [-d] [<file>]\n"
        "\n"
        "   -d             Decode the file or standard input.  Default is encode.\n";

/**
 Display usage text to the user.
 */
BOOL
Base64Help(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Base64 %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strBase64HelpText);
    return TRUE;
}

/**
 A buffer for a single data stream.
 */
typedef struct _BASE64_BUFFER {

    /**
     A handle to a pipe which is the source of data for this buffer.
     */
    HANDLE hSource;

    /**
     The data buffer.
     */
    YORI_LIB_BYTE_BUFFER ByteBuffer;

} BASE64_BUFFER, *PBASE64_BUFFER;

/**
 Populate data from stdin into an in memory buffer.

 @param ThisBuffer A pointer to the process buffer set.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
Base64BufferPump(
    __in PBASE64_BUFFER ThisBuffer
    )
{
    DWORD BytesRead;
    BOOL Result = FALSE;
    PUCHAR WriteBuffer;
    YORI_ALLOC_SIZE_T BytesAvailable;

    while (TRUE) {

        WriteBuffer = YoriLibByteBufferGetPointerToEnd(&ThisBuffer->ByteBuffer, 16384, &BytesAvailable);
        if (WriteBuffer == NULL) {
            break;
        }

        if (ReadFile(ThisBuffer->hSource,
                     WriteBuffer,
                     BytesAvailable,
                     &BytesRead,
                     NULL)) {

            if (BytesRead == 0) {
                Result = TRUE;
                break;
            }

            YoriLibByteBufferAddToPopulatedLength(&ThisBuffer->ByteBuffer, BytesRead);

        } else {
            Result = TRUE;
            break;
        }
    }

    return Result;
}

/**
 Allocate and initialize a buffer for an input stream.

 @param Buffer Pointer to the buffer to allocate structures for.

 @return TRUE if the buffer is successfully initialized, FALSE if it is not.
 */
BOOL
Base64AllocateBuffer(
    __out PBASE64_BUFFER Buffer
    )
{
    return YoriLibByteBufferInitialize(&Buffer->ByteBuffer, 1024);
}

/**
 Free structures associated with a single input stream.

 @param ThisBuffer Pointer to the single stream's buffers to deallocate.
 */
VOID
Base64FreeBuffer(
    __in PBASE64_BUFFER ThisBuffer
    )
{
    YoriLibByteBufferCleanup(&ThisBuffer->ByteBuffer);
}

/**
 Perform base64 encode and output to the requested device.

 @param ThisBuffer Pointer to the buffer to output.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
Base64Encode(
    __inout PBASE64_BUFFER ThisBuffer
    )
{
    DWORD CharsRequired;
    YORI_STRING Buffer;
    DWORD Err;
    LPTSTR ErrText;
    PUCHAR SourceBuffer;
    YORI_ALLOC_SIZE_T BytesPopulated;

    //
    //  Calculate the buffer size needed
    //

    SourceBuffer = YoriLibByteBufferGetPointerToValidData(&ThisBuffer->ByteBuffer, 0, &BytesPopulated);
    if (!DllCrypt32.pCryptBinaryToStringW(SourceBuffer, BytesPopulated, CRYPT_STRING_BASE64, NULL, &CharsRequired)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: failure to calculate buffer length in CryptBinaryToString: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    //
    //  Check if the buffer size would overflow, and fail if so
    //

    if (!YoriLibIsSizeAllocatable(CharsRequired)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: supplied data too large\n"));
        return FALSE;
    }

    //
    //  Allocate buffer
    //

    if (!YoriLibAllocateString(&Buffer, (YORI_ALLOC_SIZE_T)CharsRequired)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: allocation failure: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    //
    //  Perform the encode
    //

    if (!DllCrypt32.pCryptBinaryToStringW(SourceBuffer, BytesPopulated, CRYPT_STRING_BASE64, Buffer.StartOfString, &CharsRequired)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: failure to encode in CryptBinaryToString: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&Buffer);
        return FALSE;
    }

    Buffer.LengthInChars = (YORI_ALLOC_SIZE_T)CharsRequired;

    //
    //  Free the source buffer.  We're done with it by this point, and
    //  output may need to double buffer for encoding.
    //

    Base64FreeBuffer(ThisBuffer);

    //
    //  Output the encoded form.
    //

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Buffer);
    YoriLibFreeStringContents(&Buffer);

    return TRUE;
}

/**
 Perform base64 decode and output to the requested device.

 @param ThisBuffer Pointer to the buffer to output.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
Base64Decode(
    __inout PBASE64_BUFFER ThisBuffer
    )
{
    YORI_ALLOC_SIZE_T CharsRequired;
    DWORD BytesRequired;
    YORI_STRING Buffer;
    PUCHAR BinaryBuffer;
    DWORD Skip;
    DWORD Flags;
    DWORD BytesSent;
    BOOL Result;
    HANDLE hTarget;
    DWORD Err;
    LPTSTR ErrText;
    PUCHAR SourceBuffer;
    YORI_ALLOC_SIZE_T BytesPopulated;

    //
    //  Convert the input buffer into a UTF16 string.
    //

    SourceBuffer = YoriLibByteBufferGetPointerToValidData(&ThisBuffer->ByteBuffer, 0, &BytesPopulated);
    if (SourceBuffer == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: allocation failure\n"));
        return FALSE;
    }
    CharsRequired = YoriLibGetMultibyteInputSizeNeeded(SourceBuffer, BytesPopulated);
    if (!YoriLibAllocateString(&Buffer, CharsRequired + 1)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: allocation failure: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    YoriLibMultibyteInput(SourceBuffer, BytesPopulated, Buffer.StartOfString, CharsRequired);
    Buffer.LengthInChars = CharsRequired;
    Buffer.StartOfString[CharsRequired] = '\0';

    //
    //  Free the source buffer.  We're done with it by this point, and
    //  output may need to double buffer for encoding.
    //

    Base64FreeBuffer(ThisBuffer);

    //
    //  Calculate the buffer size needed
    //

    if (!DllCrypt32.pCryptStringToBinaryW(Buffer.StartOfString, Buffer.LengthInChars, CRYPT_STRING_BASE64, NULL, &BytesRequired, NULL, NULL)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: failure to calculate buffer length in CryptStringToBinary: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&Buffer);
        return FALSE;
    }

    //
    //  Allocate binary buffer
    //

    BinaryBuffer = YoriLibMalloc((YORI_ALLOC_SIZE_T)BytesRequired);
    if (BinaryBuffer == NULL) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: allocation failure: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&Buffer);
        return FALSE;
    }

    //
    //  Perform the decode
    //

    if (!DllCrypt32.pCryptStringToBinaryW(Buffer.StartOfString, Buffer.LengthInChars, CRYPT_STRING_BASE64, BinaryBuffer, &BytesRequired, &Skip, &Flags)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: failure to encode in CryptBinaryToString: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&Buffer);
        YoriLibFree(BinaryBuffer);
        return FALSE;
    }

    //
    //  We're done with the string form of the source, deallocate it.
    //

    YoriLibFreeStringContents(&Buffer);

    //
    //  Output the decoded form.
    //
    BytesSent = 0;
    Result = TRUE;
    hTarget = GetStdHandle(STD_OUTPUT_HANDLE);

    while (BytesSent < BytesRequired) {
        DWORD BytesToWrite;
        DWORD BytesWritten;
        BytesToWrite = 4096;
        if (BytesSent + BytesToWrite > BytesRequired) {
            BytesToWrite = BytesRequired - BytesSent;
        }

        if (WriteFile(hTarget,
                      YoriLibAddToPointer(BinaryBuffer, BytesSent),
                      BytesToWrite,
                      &BytesWritten,
                      NULL)) {

            BytesSent += BytesWritten;
        } else {
            Err = GetLastError();
            ErrText = YoriLibGetWinErrorText(Err);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: failure to write to output: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            Result = FALSE;
        }

        ASSERT(BytesSent <= BytesRequired);
    }
    YoriLibFree(BinaryBuffer);

    return Result;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the base64 builtin command.
 */
#define ENTRYPOINT YoriCmd_YBASE64
#else
/**
 The main entrypoint for the base64 standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the base64 cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, typically zero for success and nonzero
         for failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    BOOLEAN Decode = FALSE;
    BASE64_BUFFER Base64Buffer;
    YORI_STRING FullFilePath;
    DWORD Err;
    LPTSTR ErrText;

    ZeroMemory(&Base64Buffer, sizeof(Base64Buffer));
    YoriLibInitEmptyString(&FullFilePath);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                Base64Help();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2023"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                Decode = TRUE;
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

    YoriLibLoadCrypt32Functions();
    if (DllCrypt32.pCryptBinaryToStringW == NULL ||
        DllCrypt32.pCryptStringToBinaryW == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: operating system support not present\n"));
        return EXIT_FAILURE;
    }

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    YoriLibInitEmptyString(&FullFilePath);
    Base64Buffer.hSource = GetStdHandle(STD_INPUT_HANDLE);
    if (StartArg == 0 || StartArg == ArgC) {
        if (YoriLibIsStdInConsole()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: no file or pipe for input\n"));
            return EXIT_FAILURE;
        }
    } else {
        if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &FullFilePath)) {
            Err = GetLastError();
            ErrText = YoriLibGetWinErrorText(Err);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: resolving path failed: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return EXIT_FAILURE;
        }

        Base64Buffer.hSource = CreateFile(FullFilePath.StartOfString, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (Base64Buffer.hSource == INVALID_HANDLE_VALUE) {
            Err = GetLastError();
            ErrText = YoriLibGetWinErrorText(Err);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: opening file failed: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            YoriLibFreeStringContents(&FullFilePath);
            return EXIT_FAILURE;
        }
    }

    if (!Base64AllocateBuffer(&Base64Buffer)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("base64: allocating buffer failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        if (FullFilePath.LengthInChars > 0) {
            CloseHandle(Base64Buffer.hSource);
        }
        YoriLibFreeStringContents(&FullFilePath);
        return EXIT_FAILURE;
    }

    if (!Base64BufferPump(&Base64Buffer)) {
        if (FullFilePath.LengthInChars > 0) {
            CloseHandle(Base64Buffer.hSource);
        }
        YoriLibFreeStringContents(&FullFilePath);
        Base64FreeBuffer(&Base64Buffer);
        return EXIT_FAILURE;
    }

    if (!Decode) {
        if (!Base64Encode(&Base64Buffer)) {
            if (FullFilePath.LengthInChars > 0) {
                CloseHandle(Base64Buffer.hSource);
            }
            YoriLibFreeStringContents(&FullFilePath);
            Base64FreeBuffer(&Base64Buffer);
            return EXIT_FAILURE;
        }
    } else {
        if (!Base64Decode(&Base64Buffer)) {
            if (FullFilePath.LengthInChars > 0) {
                CloseHandle(Base64Buffer.hSource);
            }
            YoriLibFreeStringContents(&FullFilePath);
            Base64FreeBuffer(&Base64Buffer);
            return EXIT_FAILURE;
        }
    }

    if (FullFilePath.LengthInChars > 0) {
        CloseHandle(Base64Buffer.hSource);
    }
    YoriLibFreeStringContents(&FullFilePath);
    Base64FreeBuffer(&Base64Buffer);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
