/**
 * @file lib/bytebuf.c
 *
 * Yori expandable memory buffer
 *
 * Copyright (c) 2017-2023 Malcolm J. Smith
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Initialize a byte buffer.  The structure itself is owned by the caller.

 @param Buffer Pointer to the buffer to initialize.

 @param InitialSize The number of bytes to pre-create the buffer with.  Can
        be zero if no initial allocation is required.

 @return TRUE if the buffer is successfully initialized, FALSE if it is not.
 */
BOOL
YoriLibByteBufferInitialize(
    __out PYORI_LIB_BYTE_BUFFER Buffer,
    __in DWORDLONG InitialSize
    )
{
    Buffer->Buffer = NULL;
    Buffer->BytesAllocated = InitialSize;
    Buffer->BytesPopulated = 0;

    if (InitialSize > 0) {
        if (!YoriLibIsSizeAllocatable(InitialSize)) {
            return FALSE;
        }
        Buffer->Buffer = YoriLibMalloc((YORI_ALLOC_SIZE_T)InitialSize);
        if (Buffer->Buffer == NULL) {
            return FALSE;
        }
        Buffer->BytesAllocated = InitialSize;
    }

    return TRUE;
}

/**
 Free structures associated with a single input stream.

 @param Buffer Pointer to the single stream's buffers to deallocate.
 */
VOID
YoriLibByteBufferCleanup(
    __in PYORI_LIB_BYTE_BUFFER Buffer
    )
{
    if (Buffer->Buffer != NULL) {
        YoriLibFree(Buffer->Buffer);
    }

    YoriLibByteBufferInitialize(Buffer, 0);
}

/**
 Reset the buffer to prepare for reuse.  This retains any previous allocation
 but indicates no valid contents within the buffer.

 @param Buffer Pointer to the byte buffer structure.
 */
VOID
YoriLibByteBufferReset(
    __inout PYORI_LIB_BYTE_BUFFER Buffer
    )
{
    Buffer->BytesPopulated = 0;
}

/**
 Extend the byte buffer to a specified number of bytes.  This extends the
 allocation only without populating any contents.

 @param Buffer Pointer to the byte buffer structure.

 @param NewTotalLength The new total length to allocate in the byte buffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibByteBufferExtend(
    __in PYORI_LIB_BYTE_BUFFER Buffer,
    __in DWORDLONG NewTotalLength
    )
{
    PUCHAR NewBuffer;

    if (Buffer->BytesAllocated >= NewTotalLength) {
        return FALSE;
    }

    if (!YoriLibIsSizeAllocatable(NewTotalLength)) {
        return FALSE;
    }

    NewBuffer = YoriLibMalloc((YORI_ALLOC_SIZE_T)NewTotalLength);
    if (NewBuffer == NULL) {
        return FALSE;
    }

    if (Buffer->BytesPopulated > 0) {
        memcpy(NewBuffer, Buffer->Buffer, (DWORD)Buffer->BytesPopulated);
    }

    if (Buffer->Buffer != NULL) {
        YoriLibFree(Buffer->Buffer);
    }

    Buffer->Buffer = NewBuffer;
    Buffer->BytesAllocated = NewTotalLength;

    return TRUE;
}

/**
 Get a pointer to the first invalid byte in the buffer so new data can be
 written to it.

 @param Buffer Pointer to the byte buffer structure.

 @param MinimumLengthRequired Indicates the number of bytes that must be
        available for newly valid data.  The buffer will be extended if it
        does not have this many bytes.

 @param BytesAvailable On successful completion, optionally updated to
        contain the number of bytes available in the buffer (ie., the number
        of bytes that are invalid and can be written to.)

 @return On successful completion, pointer to the buffer to write to.
         Returns NULL on failure.
 */
__success(return != NULL)
PUCHAR
YoriLibByteBufferGetPointerToEnd(
    __in PYORI_LIB_BYTE_BUFFER Buffer,
    __in DWORDLONG MinimumLengthRequired,
    __out_opt PDWORDLONG BytesAvailable
    )
{
    DWORDLONG BytesRemaining = Buffer->BytesAllocated - Buffer->BytesPopulated;
    PUCHAR EndOfBuffer;

    if (BytesRemaining < MinimumLengthRequired) {
        DWORDLONG NewLength;

        NewLength = Buffer->BytesAllocated * 2;
        if (NewLength == 0) {
            NewLength = 16384;
        }
        if (NewLength < MinimumLengthRequired) {
            NewLength = MinimumLengthRequired;
        }
        if (!YoriLibByteBufferExtend(Buffer, NewLength)) {
            return NULL;
        }

        BytesRemaining = Buffer->BytesAllocated - Buffer->BytesPopulated;
    }

    EndOfBuffer = YoriLibAddToPointer(Buffer->Buffer, Buffer->BytesPopulated);
    if (BytesAvailable != NULL) {
        *BytesAvailable = BytesRemaining;
    }

    return EndOfBuffer;
}

/**
 Indicate that the buffer has additional valid bytes.

 @param Buffer Pointer to the byte buffer structure.

 @param NewBytesPopulated The number of newly valid bytes.  Note this is not
        the total number of valid bytes.

 @return TRUE to indicate success, FALSE to indicate failure.  Failure implies
         caller error where a caller is indicating more bytes to be valid than
         the buffer contains.
 */
BOOLEAN
YoriLibByteBufferAddToPopulatedLength(
    __in PYORI_LIB_BYTE_BUFFER Buffer,
    __in DWORDLONG NewBytesPopulated
    )
{
    ASSERT(Buffer->BytesPopulated + NewBytesPopulated <= Buffer->BytesAllocated);
    if (Buffer->BytesPopulated + NewBytesPopulated > Buffer->BytesAllocated) {
        return FALSE;
    }

    Buffer->BytesPopulated = Buffer->BytesPopulated + NewBytesPopulated;
    return TRUE;
}


// vim:sw=4:ts=4:et:
