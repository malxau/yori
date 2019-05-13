/**
 * @file sponge/sponge.c
 *
 * Yori shell filter within a line of output
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
 Help text to display to the user.
 */
const
CHAR strSpongeHelpText[] =
        "\n"
        "Read input into memory and output once all input is read,\n"
        "  allowing the output to modify the source stream.\n"
        "\n"
        "SPONGE [-license] [file]\n"
        ;

/**
 Display usage text to the user.
 */
BOOL
SpongeHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Sponge %i.%02i\n"), SPONGE_VER_MAJOR, SPONGE_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strSpongeHelpText);
    return TRUE;
}

/**
 A buffer for a single data stream.
 */
typedef struct _SPONGE_BUFFER {

    /**
     The number of bytes currently allocated to this buffer.
     */
    DWORD BytesAllocated;

    /**
     The number of bytes populated with data in this buffer.
     */
    DWORD BytesPopulated;

    /**
     A handle to a pipe which is the source of data for this buffer.
     */
    HANDLE hSource;

    /**
     The data buffer.
     */
    PCHAR Buffer;

} SPONGE_BUFFER, *PSPONGE_BUFFER;

/**
 Populate data from stdin into an in memory buffer.

 @param ThisBuffer A pointer to the process buffer set.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SpongeBufferPump(
    __in PSPONGE_BUFFER ThisBuffer
    )
{
    DWORD BytesRead;
    BOOL Result = FALSE;

    while (TRUE) {

        if (ReadFile(ThisBuffer->hSource,
                     YoriLibAddToPointer(ThisBuffer->Buffer, ThisBuffer->BytesPopulated),
                     ThisBuffer->BytesAllocated - ThisBuffer->BytesPopulated,
                     &BytesRead,
                     NULL)) {

            if (BytesRead == 0) {
                Result = TRUE;
                break;
            }

            ThisBuffer->BytesPopulated += BytesRead;
            ASSERT(ThisBuffer->BytesPopulated <= ThisBuffer->BytesAllocated);
            if (ThisBuffer->BytesPopulated >= ThisBuffer->BytesAllocated) {
                DWORD NewBytesAllocated;
                PCHAR NewBuffer;

                if (ThisBuffer->BytesAllocated >= ((DWORD)-1) / 4) {
                    break;
                }

                NewBytesAllocated = ThisBuffer->BytesAllocated * 4;

                NewBuffer = YoriLibMalloc(NewBytesAllocated);
                if (NewBuffer == NULL) {
                    break;
                }

                memcpy(NewBuffer, ThisBuffer->Buffer, ThisBuffer->BytesAllocated);
                YoriLibFree(ThisBuffer->Buffer);
                ThisBuffer->Buffer = NewBuffer;
                ThisBuffer->BytesAllocated = NewBytesAllocated;
            }
        } else {
            Result = TRUE;
            break;
        }
    }

    return Result;
}

/**
 Output the collected buffer to a stream.

 @param ThisBuffer Pointer to the buffer to output.

 @param hTarget Handle to the target stream to output the buffer to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SpongeBufferForward(
    __in PSPONGE_BUFFER ThisBuffer,
    __in HANDLE hTarget
    )
{
    DWORD BytesSent;
    BOOL Result;

    BytesSent = 0;
    Result = TRUE;

    while (BytesSent < ThisBuffer->BytesPopulated) {
        DWORD BytesToWrite;
        DWORD BytesWritten;
        BytesToWrite = 4096;
        if (BytesSent + BytesToWrite > ThisBuffer->BytesPopulated) {
            BytesToWrite = ThisBuffer->BytesPopulated - BytesSent;
        }

        if (WriteFile(hTarget,
                      YoriLibAddToPointer(ThisBuffer->Buffer, BytesSent),
                      BytesToWrite,
                      &BytesWritten,
                      NULL)) {

            BytesSent += BytesWritten;
        } else {
            Result = FALSE;
        }

        ASSERT(BytesSent <= ThisBuffer->BytesPopulated);
    }

    return Result;
}

/**
 Allocate and initialize a buffer for an input stream.

 @param Buffer Pointer to the buffer to allocate structures for.

 @return TRUE if the buffer is successfully initialized, FALSE if it is not.
 */
BOOL
SpongeAllocateBuffer(
    __out PSPONGE_BUFFER Buffer
    )
{
    Buffer->BytesAllocated = 1024;
    Buffer->Buffer = YoriLibMalloc(Buffer->BytesAllocated);
    if (Buffer->Buffer == NULL) {
        return FALSE;
    }

    return TRUE;
}

/**
 Free structures associated with a single input stream.

 @param ThisBuffer Pointer to the single stream's buffers to deallocate.
 */
VOID
SpongeFreeBuffer(
    __in PSPONGE_BUFFER ThisBuffer
    )
{
    if (ThisBuffer->Buffer != NULL) {
        YoriLibFree(ThisBuffer->Buffer);
    }
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the sponge builtin command.
 */
#define ENTRYPOINT YoriCmd_YSPONGE
#else
/**
 The main entrypoint for the sponge standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the sponge cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of zero to indicate success, nonzero to indicate failure.
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
    YORI_STRING Arg;
    SPONGE_BUFFER SpongeBuffer;
    YORI_STRING FullFilePath;
    HANDLE hTarget;

    ZeroMemory(&SpongeBuffer, sizeof(SpongeBuffer));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                SpongeHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartArg = i + 1;
                break;
            }
        } else {
            StartArg = i;
            ArgumentUnderstood = TRUE;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }


#if YORI_BUILTIN
    YoriLibCancelEnable();
#endif

    if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &i)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
        return EXIT_FAILURE;
    }

    if (!SpongeAllocateBuffer(&SpongeBuffer)) {
        return EXIT_FAILURE;
    }
    SpongeBuffer.hSource = GetStdHandle(STD_INPUT_HANDLE);

    YoriLibInitEmptyString(&FullFilePath);
    hTarget = GetStdHandle(STD_OUTPUT_HANDLE);
    if (StartArg != 0 && StartArg < ArgC) {
        YoriLibInitEmptyString(&FullFilePath);
        if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &FullFilePath)) {
            SpongeFreeBuffer(&SpongeBuffer);
            return EXIT_FAILURE;
        }
    }

    if (!SpongeBufferPump(&SpongeBuffer)) {
        SpongeFreeBuffer(&SpongeBuffer);
        return EXIT_FAILURE;
    }

    if (FullFilePath.LengthInChars > 0) {
        hTarget = CreateFile(FullFilePath.StartOfString,
                             GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_DELETE,
                             NULL,
                             CREATE_ALWAYS,
                             0,
                             NULL);
        if (hTarget == INVALID_HANDLE_VALUE) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            SpongeFreeBuffer(&SpongeBuffer);
            YoriLibFreeStringContents(&FullFilePath);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("sponge: open file failed: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return EXIT_FAILURE;
        }
    }

    SpongeBufferForward(&SpongeBuffer, hTarget);

    if (FullFilePath.LengthInChars > 0) {
        CloseHandle(hTarget);
        YoriLibFreeStringContents(&FullFilePath);
    }

    SpongeFreeBuffer(&SpongeBuffer);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
