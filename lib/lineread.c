/**
 * @file lib/lineread.c
 *
 * Implementations for reading lines from files.
 *
 * Copyright (c) 2014-2020 Malcolm J. Smith
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
 Context to be passed between repeated line read calls to contain data
 that doesn't constitute a whole line but cannot be left in the incoming
 buffer (eg. the incoming buffer is a pipe.)
 */
typedef struct _YORI_LIB_LINE_READ_CONTEXT {

    /**
     The number of lines successfully read.
     */
    LONGLONG LinesRead;

    /**
     Characters that have been read from the input stream but not yet
     returned as an entire line to the caller.
     */
    LPSTR PreviousBuffer;

    /**
     The number of characters in PreviousBuffer.  Note that PreviousBuffer
     is currently an 8 bit string.
     */
    DWORD BytesInBuffer;

    /**
     The size of the PreviousBuffer allocation, in characters.
     */
    DWORD LengthOfBuffer;

    /**
     Offset within the buffer to the data that has not yet been returned.
     */
    DWORD CurrentBufferOffset;

    /**
     The type of the handle (file, pipe, etc.)
     */
    DWORD FileType;

    /**
     If TRUE, the read operation is performed on 16 bit characters.  If FALSE,
     the input contains 8 bit characters.  Unlike most other encodings, this
     matters because it defines the form of the newlines that this module is
     looking for.
     */
    BOOLEAN ReadWChars;

    /**
     If TRUE, the operation has been terminated for some reason, and future
     operations should fail.  This exists because the operation may terminate
     but successfully return the trailing portion of a buffer, and the
     enumerator can be invoked again even though a previous call determined
     no further processing should occur.
     */
    BOOLEAN Terminated;

} YORI_LIB_LINE_READ_CONTEXT, *PYORI_LIB_LINE_READ_CONTEXT;

/**
 Copy the contents of a line into a user specified buffer.  If the buffer
 is not large enough, it is reallocated.  This function performs encoding
 conversions to ensure the resulting string is in host (UTF16) encoding.

 @param UserString The user provided string to populate with a line.

 @param SourceBuffer Pointer to a buffer in input encoding format that
        should be returned in UserString.

 @param CharsToCopy The number of characters to copy.  Note this may mean
        8 bit or 16 bit characters depending on input encoding.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCopyLineToUserBufferW(
    __inout PYORI_STRING UserString,
    __in LPSTR SourceBuffer,
    __in DWORD CharsToCopy
    )
{
    DWORD CharsNeeded;

    if (CharsToCopy == 0) {
        CharsNeeded = 1;
    } else {
        CharsNeeded = YoriLibGetMultibyteInputSizeNeeded(SourceBuffer, CharsToCopy) + 1;
    }

    if (CharsNeeded > UserString->LengthAllocated) {
        UserString->LengthInChars = 0;
        if (!YoriLibReallocateString(UserString, CharsNeeded + 64)) {
            return FALSE;
        }
    }

    if (CharsToCopy > 0) {
        YoriLibMultibyteInput(SourceBuffer,
                              CharsToCopy,
                              UserString->StartOfString,
                              UserString->LengthAllocated);
    }

    UserString->LengthInChars = CharsNeeded - 1;
    UserString->StartOfString[UserString->LengthInChars] = '\0';
    return TRUE;
}

/**
 Check for the existence of a byte order mark in the string, and return how
 many bytes are in it.

 @param StringToCheck Pointer to a string to check for the existence of a BOM.
        Note: It is important this is unsigned, because comparisons here will
        be upconverted to int, and if the char is sign extended the
        comparisons will be incorrect.

 @param BytesInString Specifies the number of bytes in StringToCheck.

 @return The count of bytes in the BOM, which may be zero.
 */
DWORD
YoriLibBytesInBom(
    __in PUCHAR StringToCheck,
    __in DWORD BytesInString
    )
{
    DWORD Encoding = YoriLibGetMultibyteInputEncoding();
    if (BytesInString >= 3 && Encoding == CP_UTF8) {

        if (StringToCheck[0] == 0xEF &&
            StringToCheck[1] == 0xBB &&
            StringToCheck[2] == 0xBF) {

            return 3;
        }
    }

    if (BytesInString >= 2 && Encoding == CP_UTF16) {
        if (StringToCheck[0] == 0xFF &&
            StringToCheck[1] == 0xFE) {

            return 2;
        }

        if (StringToCheck[0] == 0xFE &&
            StringToCheck[1] == 0xFF) {

            return 2;
        }
    }

    return 0;
}

/**
 The number of cached read line contexts to keep.
 */
#define YORI_LIB_READ_LINE_CACHE_ENTRIES 4

/**
 An array of line read contexts.  Each element is synchronized with
 interlocked operations.  In the event of a race, it is valid to
 ignore the cache and perform a fresh allocation.
 */
PYORI_LIB_LINE_READ_CONTEXT YoriLibReadLineCachedContext[YORI_LIB_READ_LINE_CACHE_ENTRIES];

#if defined(_M_IX86) && (_M_IX86 >= 400)
/**
 Newer compilers provide an intrinsic, but the situation is awful, so this
 links to an assembly routine instead.

 Visual C++ 2.0-4.0 do not support the intrinsic.
 Visual C++ 4.1-4.2 generate a "lock mov" and a non-lock "cmpxchg", which
     causes programs to hit an illegal instruction fault.
 Visual C++ 5.0 appears to not emit the actual cmpxchg instruction.
 Visual C++ 6.0 works.
 */
LONG lock_cmpxchg(__inout _Interlocked_operand_ LONG volatile *, __in LONG, __in LONG);
#endif

/**
 Return TRUE if InterlockedCompareExchangePointer is available.  This exists
 on all 64 bit systems, and on 32 bit x86 it requires a 486 or newer.  The
 implementation is provided as assembly due to incomplete and buggy
 implementations of a compiler intrinsic on older compilers.  If compiled
 for 386 or RISC, fallback to using the export, requiring NT 4 or above.

 @return TRUE if the system supports an implementation of
         InterlockedCompareExchangePointer, or FALSE if it does not.
 */
BOOLEAN
YoriLibIsInterlockedCompareExchangePointerAvailable(VOID)
{
#if defined(_WIN64)
#if defined(_M_ALPHA)
    return FALSE;
#else
    return TRUE;
#endif
#elif defined(_M_IX86) && (_M_IX86 >= 400)
    return TRUE;
#else
    if (DllKernel32.pInterlockedCompareExchange) {
        return TRUE;
    }
    return FALSE;
#endif
}

#if defined(_WIN64) || !defined(_M_IX86) || (_M_IX86 < 400)
/**
 Perform an InterlockedCompareExchange of pointer size.  This function assumes
 the caller has firstly called
 YoriLibIsInterlockedCompareExchangePointerAvailable to verify that the
 functionality exists.

 @param Dest Pointer to the value to compare and change.

 @param Exchange The value to store into Dest if the comparison is successful.

 @param Comperand The value to compare to the value in Dest.

 @return The original value of Dest.
 */
PVOID
YoriLibInterlockedCompareExchangePointer(volatile PVOID * Dest, PVOID Exchange, PVOID Comperand)
{
#if defined(_WIN64)
#if defined(_M_ALPHA)
    UNREFERENCED_PARAMETER(Dest);
    UNREFERENCED_PARAMETER(Exchange);
    UNREFERENCED_PARAMETER(Comperand);
    return NULL;
#else
    return InterlockedCompareExchangePointer(Dest, Exchange, Comperand);
#endif
#else
    return (PVOID)(DllKernel32.pInterlockedCompareExchange((volatile LONG *)Dest, (LONG)Exchange, (LONG)Comperand));
#endif
}

#else

/**
 Since this is a 32 bit system, the pointer function is the same as the 32 bit
 integer function.  Cast the types in both directions to keep C happy.
 */
#define YoriLibInterlockedCompareExchangePointer(a, b, c) (PVOID)lock_cmpxchg((volatile LONG *)a, (LONG)b, (LONG)c)

#endif

/**
 Allocate a line read context, which may be from a previously saved cache
 entry, or may be from the heap.
 */
PYORI_LIB_LINE_READ_CONTEXT
YoriLibReadLineAllocateContext(VOID)
{
    PYORI_LIB_LINE_READ_CONTEXT ReadContext;

    if (YoriLibIsInterlockedCompareExchangePointerAvailable()) {
        DWORD ProbeIndex;

        ReadContext = NULL;
        for (ProbeIndex = 0;
             ProbeIndex < YORI_LIB_READ_LINE_CACHE_ENTRIES;
             ProbeIndex++) {

            ReadContext = YoriLibReadLineCachedContext[ProbeIndex];
            if (ReadContext != NULL) {
                break;
            }
        }

        if (ReadContext != NULL) {
            if (YoriLibInterlockedCompareExchangePointer((volatile PVOID *)&YoriLibReadLineCachedContext[ProbeIndex], NULL, ReadContext) == ReadContext) {
                return ReadContext;
            }
        }
    }

    ReadContext = YoriLibMalloc(sizeof(YORI_LIB_LINE_READ_CONTEXT));
    if (ReadContext == NULL) {
        return NULL;
    }
    ReadContext->PreviousBuffer = NULL;
    ReadContext->LengthOfBuffer = 0;
    return ReadContext;
}

/**
 Close a line read context, and store it in the cache if there is an
 available slot for it.  After using this routine, a caller is expected to
 call YoriLibLineReadCleanupCache to teardown any saved cache entries.

 @param Context Pointer to the context to free.
 */
VOID
YoriLibLineReadCloseOrCache(
    __in_opt PVOID Context
    )
{
    if (YoriLibIsInterlockedCompareExchangePointerAvailable()) {
        PYORI_LIB_LINE_READ_CONTEXT ReadContext = (PYORI_LIB_LINE_READ_CONTEXT)Context;
        PYORI_LIB_LINE_READ_CONTEXT OldContext;
        DWORD ProbeIndex;

        OldContext = NULL;
        for (ProbeIndex = 0;
             ProbeIndex < YORI_LIB_READ_LINE_CACHE_ENTRIES;
             ProbeIndex++) {

            OldContext = YoriLibReadLineCachedContext[ProbeIndex];
            if (OldContext == NULL) {
                break;
            }
        }

        if (OldContext == NULL) {
            if (YoriLibInterlockedCompareExchangePointer((volatile PVOID *)&YoriLibReadLineCachedContext[ProbeIndex], ReadContext, NULL) == NULL) {
                return;
            }
        }
    }

    YoriLibLineReadClose(Context);
}

/**
 Tear down any unused cached line read context entries.
 */
VOID
YORI_BUILTIN_FN
YoriLibLineReadCleanupCache(VOID)
{
    if (YoriLibIsInterlockedCompareExchangePointerAvailable()) {
        PYORI_LIB_LINE_READ_CONTEXT OldContext;
        DWORD ProbeIndex;

        for (ProbeIndex = 0;
             ProbeIndex < YORI_LIB_READ_LINE_CACHE_ENTRIES;
             ProbeIndex++) {

            OldContext = YoriLibReadLineCachedContext[ProbeIndex];
            if (YoriLibInterlockedCompareExchangePointer((volatile PVOID *)&YoriLibReadLineCachedContext[ProbeIndex], NULL, OldContext) == OldContext) {
                YoriLibLineReadClose(OldContext);
            }
        }
    }
}


/**
 Read a line from an input stream.

 @param UserString Pointer to a string to be updated to contain data for a
        line.  This must be initialized by the caller and the caller's buffer
        will be used if it is large enough.  If not, this function may
        reallocate the string to point to a new buffer.

 @param Context Pointer to a PVOID sized block of memory that should be
        initialized to NULL for the first line read, and will be updated by
        this function.

 @param ReturnFinalNonTerminatedLine If TRUE, treat any line at the end of the
        stream without a line ending character to be a line to return.  If
        FALSE, assume new input could arrive that means we just haven't
        observed the line break yet.

 @param MaximumDelay Specifies the maximum amount of time to wait for a
        complete line.  This value can be INFINITE or a specified number of
        milliseconds.  If the timeout value is reached, TimeoutReached will
        be set to true and the function will return NULL.

 @param FileHandle Specifies the handle to the file to read the line from.

 @param LineEnding On successful completion, set to indicate the string of
        characters used to terminate the line.  Can be YoriLibLineEndingNone
        to indicate no line end was found, which can happen if
        ReturnFinalNonTerminatedLine is TRUE or MaximumDelay is less than
        infinite and a partial line was found.

 @param TimeoutReached On successful completion, set to TRUE to indicate that
        the timeout value in MaximumDelay was reached.  If MaximumDelay is
        INFINITE, this cannot happen.

 @return Pointer to the Line buffer for success, NULL on failure.
 */
PVOID
YoriLibReadLineToStringEx(
    __in PYORI_STRING UserString,
    __inout PVOID * Context,
    __in BOOL ReturnFinalNonTerminatedLine,
    __in DWORD MaximumDelay,
    __in HANDLE FileHandle,
    __out PYORI_LIB_LINE_ENDING LineEnding,
    __out PBOOL TimeoutReached
    )
{
    PYORI_LIB_LINE_READ_CONTEXT ReadContext;
    DWORD Count = 0;
    DWORD LastError;
    DWORD BytesToRead;
    DWORD BytesRead;
    DWORD CharsToCopy;
    DWORD CharsToSkip;
    BOOL BomFound = FALSE;
    BOOL TerminateProcessing;
    HANDLE HandleArray[2];
    DWORD HandleCount;
    DWORD WaitResult;
    DWORD DelayTime;
    DWORD CharsRemaining;
    DWORD CumulativeDelay;
    YORI_LIB_LINE_ENDING LocalLineEnding;

    *TimeoutReached = FALSE;

    //
    //  If we don't have a line read context yet, allocate one.
    //

    if (*Context == NULL) {
        ReadContext = YoriLibReadLineAllocateContext();
        if (ReadContext == NULL) {
            UserString->LengthInChars = 0;
            *LineEnding = YoriLibLineEndingNone;
            return NULL;
        }
        *Context = ReadContext;
        ReadContext->BytesInBuffer = 0;
        ReadContext->CurrentBufferOffset = 0;
        ReadContext->LinesRead = 0;
        ReadContext->FileType = GetFileType(FileHandle);
        if (YoriLibGetMultibyteInputEncoding() == CP_UTF16) {
            ReadContext->ReadWChars = TRUE;
        } else {
            ReadContext->ReadWChars = FALSE;
        }
        ReadContext->Terminated = FALSE;
    } else {
        ReadContext = *Context;
        if (ReadContext->Terminated) {
            return NULL;
        }
    }

    //
    //  If the line read context doesn't have a buffer yet, allocate it
    //

    if (ReadContext->PreviousBuffer == NULL || UserString->LengthAllocated > ReadContext->LengthOfBuffer) {
        if (ReadContext->PreviousBuffer != NULL) {
            YoriLibFree(ReadContext->PreviousBuffer);
            ReadContext->PreviousBuffer = NULL;
        }
        ReadContext->LengthOfBuffer = UserString->LengthAllocated;
        if (ReadContext->LengthOfBuffer < 256 * 1024) {
            ReadContext->LengthOfBuffer = 256 * 1024;
        }
        ReadContext->PreviousBuffer = YoriLibMalloc(ReadContext->LengthOfBuffer);
        if (ReadContext->PreviousBuffer == NULL) {
            UserString->LengthInChars = 0;
            *LineEnding = YoriLibLineEndingNone;
            ReadContext->Terminated = TRUE;
            return NULL;
        }
    }

    do {

        BOOL ProcessThisLine;
        ASSERT(ReadContext->CurrentBufferOffset <= ReadContext->BytesInBuffer);

        //
        //  Scan through the buffer looking for newlines.  If we find one,
        //  copy the data back into the caller's buffer.  Copy any remaining
        //  buffer back to the beginning of the holdover buffer, and
        //  decrement chars there accordingly.
        //

        if (ReadContext->ReadWChars) {
            PWCHAR WideBuffer = (PWCHAR)YoriLibAddToPointer(ReadContext->PreviousBuffer, ReadContext->CurrentBufferOffset);
            CharsRemaining = (ReadContext->BytesInBuffer - ReadContext->CurrentBufferOffset) / sizeof(WCHAR);
            for (Count = 0; Count < CharsRemaining; Count++) {
                if (WideBuffer[Count] == 0xD ||
                    WideBuffer[Count] == 0xA) {

                    ProcessThisLine = TRUE;

                    CharsToCopy = Count;
                    LocalLineEnding = YoriLibLineEndingCR;
                    if (WideBuffer[Count] == 0xD) {
                        if ((Count + 1) * sizeof(WCHAR) < (ReadContext->BytesInBuffer - ReadContext->CurrentBufferOffset)) {
                            if (WideBuffer[Count + 1] == 0xA) {
                                Count++;
                                LocalLineEnding = YoriLibLineEndingCRLF;
                            }
                        } else if (ReadContext->CurrentBufferOffset > 0) {
                            ProcessThisLine = FALSE;
                        }
                    } else {
                        LocalLineEnding = YoriLibLineEndingLF;
                    }

                    Count++;

                    if (ProcessThisLine) {

                        CharsToSkip = 0;
                        if (!BomFound && ReadContext->LinesRead == 0) {
                            CharsToSkip = YoriLibBytesInBom((PUCHAR)ReadContext->PreviousBuffer, CharsToCopy * sizeof(WCHAR));
                            if (CharsToSkip > 0) {
                                BomFound = TRUE;
                                CharsToSkip = CharsToSkip / sizeof(WCHAR);
                                CharsToCopy -= CharsToSkip;
                            }
                        }
                        if (YoriLibCopyLineToUserBufferW(UserString, (LPSTR)&WideBuffer[CharsToSkip], CharsToCopy)) {
                            ReadContext->CurrentBufferOffset += Count * sizeof(WCHAR);
                            ReadContext->LinesRead++;
                            *LineEnding = LocalLineEnding;
                            return UserString->StartOfString;
                        } else {
                            UserString->LengthInChars = 0;
                            *LineEnding = YoriLibLineEndingNone;
                            ReadContext->Terminated = TRUE;
                            return NULL;
                        }
                    }
                }
            }
        } else {
            PUCHAR Buffer = YoriLibAddToPointer(ReadContext->PreviousBuffer, ReadContext->CurrentBufferOffset);
            CharsRemaining = ReadContext->BytesInBuffer - ReadContext->CurrentBufferOffset;
            for (Count = 0; Count < CharsRemaining; Count++) {

                if (Buffer[Count] == 0xD ||
                    Buffer[Count] == 0xA) {

                    ProcessThisLine = TRUE;

                    CharsToCopy = Count;
                    LocalLineEnding = YoriLibLineEndingCR;
                    if (Buffer[Count] == 0xD) {
                        if (Count + 1 < (ReadContext->BytesInBuffer - ReadContext->CurrentBufferOffset)) {
                            if (Buffer[Count + 1] == 0xA) {
                                Count++;
                                LocalLineEnding = YoriLibLineEndingCRLF;
                            }
                        } else if (ReadContext->CurrentBufferOffset > 0) {
                            ProcessThisLine = FALSE;
                        }
                    } else {
                        LocalLineEnding = YoriLibLineEndingLF;
                    }

                    Count++;

                    if (ProcessThisLine) {

                        CharsToSkip = 0;
                        if (!BomFound && ReadContext->LinesRead == 0) {
                            CharsToSkip = YoriLibBytesInBom((PUCHAR)ReadContext->PreviousBuffer, CharsToCopy);
                            if (CharsToSkip > 0) {
                                BomFound = TRUE;
                                CharsToCopy -= CharsToSkip;
                            }
                        }
                        if (YoriLibCopyLineToUserBufferW(UserString, (LPSTR)&Buffer[CharsToSkip], CharsToCopy)) {
                            ReadContext->CurrentBufferOffset += Count;
                            ReadContext->LinesRead++;
                            *LineEnding = LocalLineEnding;
                            return UserString->StartOfString;
                        } else {
                            UserString->LengthInChars = 0;
                            *LineEnding = YoriLibLineEndingNone;
                            ReadContext->Terminated = TRUE;
                            return NULL;
                        }
                    }
                }
            }
        }

        //
        //  We haven't found any lines.  Move the contents that are still
        //  unprocessed to the front of the buffer.
        //

        if (ReadContext->CurrentBufferOffset != 0) {
            memmove(ReadContext->PreviousBuffer, YoriLibAddToPointer(ReadContext->PreviousBuffer, ReadContext->CurrentBufferOffset), ReadContext->BytesInBuffer - ReadContext->CurrentBufferOffset);
            ReadContext->BytesInBuffer -= ReadContext->CurrentBufferOffset;
            ReadContext->CurrentBufferOffset = 0;
        }

        //
        //  If we haven't found a newline yet and the buffer is full,
        //  we're at the end of the road.  Although we could increase the
        //  buffer size, the caller isn't able to process a line of this
        //  length anyway.
        //

        if (ReadContext->LengthOfBuffer == ReadContext->BytesInBuffer) {
            UserString->LengthInChars = 0;
            *LineEnding = YoriLibLineEndingNone;
            ReadContext->Terminated = TRUE;
            return NULL;
        }

        //
        //  Wait for more data, or for cancellation if it's enabled.
        //
        //  This stupid dance about waiting and sleeping is for the following
        //  brain-damaged comment in MSDN under "Named Pipe Operations":
        //
        //    The pipe server should not perform a blocking read operation
        //    until the pipe client has started. Otherwise, a race condition
        //    can occur. This typically occurs when initialization code, such
        //    as that of the C run-time library, needs to lock and examine
        //    inherited handles.
        //
        //  Of course, we don't control the behavior of the pipe client.  We
        //  do, however, observe that the pipe can be signalled prior to the
        //  client performing operations on it, which requires us to
        //  distinguish between "signalled due to correct operation" and
        //  "signalled due to a documented bug on MSDN."
        //
        //  The cancel event is listed first because in the case where the
        //  pipe is overactively signalled, we still want to detect cancel,
        //  which will not be overactively signalled.
        //

        CumulativeDelay = 0;
        DelayTime = 1;
        TerminateProcessing = FALSE;
        while(TRUE) {
            DWORD BytesAvailable;

            if (YoriLibCancelGetEvent() != NULL) {
                HandleCount = 2;
                HandleArray[0] = YoriLibCancelGetEvent();
                HandleArray[1] = FileHandle;
            } else {
                HandleCount = 1;
                HandleArray[0] = FileHandle;
            }

            WaitResult = WaitForMultipleObjectsEx(HandleCount, HandleArray, FALSE, INFINITE, FALSE);
            if (WaitResult == WAIT_OBJECT_0 && HandleCount > 1) {
                TerminateProcessing = TRUE;
                break;
            }

            if (ReadContext->FileType != FILE_TYPE_PIPE) {
                break;
            }

            if (!PeekNamedPipe(FileHandle, NULL, 0, NULL, &BytesAvailable, NULL)) {
                TerminateProcessing = TRUE;
                break;
            }

            if (BytesAvailable > 0) {
                break;
            }

            if (MaximumDelay != INFINITE && CumulativeDelay >= MaximumDelay) {
                *TimeoutReached = TRUE;
                TerminateProcessing = TRUE;
                break;
            }

            ResetEvent(FileHandle);

            //
            //  Note that this delay is not exercised once the process
            //  starts pushing data into the pipe.  Think of this as
            //  the maximum interval that we're waiting for the process
            //  to start.
            //

            Sleep(DelayTime);
            CumulativeDelay += DelayTime;
            if (DelayTime < 10) {
                DelayTime++;
            } else {
                DelayTime = DelayTime * 5 / 4;
            }
            if (DelayTime > 500) {
                DelayTime = 500;
            }
        }

        //
        //  If we haven't found a newline yet, check if we can read more
        //  data and see if it helps.  If we fail to read more data,
        //  just treat any buffer remainder as a line.
        //

        BytesRead = 0;
        if (!TerminateProcessing) {

            BytesToRead = ReadContext->LengthOfBuffer - ReadContext->BytesInBuffer;
            LastError = ERROR_SUCCESS;

            while(TRUE) {
                if (ReadFile(FileHandle, YoriLibAddToPointer(ReadContext->PreviousBuffer, ReadContext->BytesInBuffer), BytesToRead, &BytesRead, NULL)) {
                    LastError = ERROR_SUCCESS;
                    break;
                }

                //
                //  NT 3.1 can fail reads with NOT_ENOUGH_MEMORY if the buffer
                //  is too large.  Work around this by shrinking the requested
                //  number of bytes to read.
                //

                LastError = GetLastError();
                if (LastError == ERROR_NOT_ENOUGH_MEMORY && BytesToRead > 16384) {
                    BytesToRead = 16384;
                    continue;
                }

                break;
            }

            if (LastError != ERROR_SUCCESS) {
#if DBG
                //
                //  Most of these indicate the source has gone away or ended.
                //  ERROR_INVALID_PARAMETER happens when we're trying to
                //  perform an unaligned read on a noncached handle, which
                //  is crazy, but Windows will silently allow cached opens to
                //  devices to be noncached opens, which inconveniently means
                //  the detection of the problem happens later than it should.
                //

                ASSERT(LastError == ERROR_BROKEN_PIPE ||
                       LastError == ERROR_NO_DATA ||
                       LastError == ERROR_HANDLE_EOF ||
                       LastError == ERROR_INVALID_PARAMETER);
#endif
                TerminateProcessing = TRUE;
            }

            if (ReadContext->FileType != FILE_TYPE_PIPE && BytesRead == 0) {

                TerminateProcessing = TRUE;
            }
        }

        if (TerminateProcessing) {
            if (ReturnFinalNonTerminatedLine) {

                //
                //  Timeouts are used to return partial lines if a full line
                //  doesn't arrive promptly.  A timeout does not mean that
                //  there is no more data to read ever, it just means data
                //  isn't arriving immediately (and may be arriving
                //  interactively in cvtvt -exec.)
                //

                if (*TimeoutReached == FALSE) {
                    ReadContext->Terminated = TRUE;
                }

                if (ReadContext->BytesInBuffer > 0) {

                    //
                    //  We're at the end of the file.  Return what we have, even if
                    //  there's not a newline character.
                    //

                    CharsToSkip = 0;
                    CharsToCopy = ReadContext->BytesInBuffer;
                    if (!BomFound && ReadContext->LinesRead == 0) {
                        CharsToSkip = YoriLibBytesInBom((PUCHAR)ReadContext->PreviousBuffer, CharsToCopy);
                        if (CharsToSkip > 0) {
                            BomFound = TRUE;
                            CharsToCopy -= CharsToSkip;
                        }
                    }
                    if (ReadContext->ReadWChars) {
                        CharsToCopy = CharsToCopy / sizeof(WCHAR);
                    }
                    if (YoriLibCopyLineToUserBufferW(UserString, &ReadContext->PreviousBuffer[CharsToSkip], CharsToCopy)) {
                        ReadContext->BytesInBuffer = 0;
                        *LineEnding = YoriLibLineEndingNone;
                        return UserString->StartOfString;
                    }
                }
            }
            UserString->LengthInChars = 0;
            *LineEnding = YoriLibLineEndingNone;
            return NULL;
        }

        ReadContext->BytesInBuffer += BytesRead;

    } while(TRUE);
}

/**
 Read a line from an input stream.

 @param UserString Pointer to a string to be updated to contain data for a
        line.  This must be initialized by the caller and the caller's buffer
        will be used if it is large enough.  If not, this function may
        reallocate the string to point to a new buffer.

 @param Context Pointer to a PVOID sized block of memory that should be
        initialized to NULL for the first line read, and will be updated by
        this function.

 @param FileHandle Specifies the handle to the file to read the line from.

 @return Pointer to the Line buffer for success, NULL on failure.
 */
PVOID
YoriLibReadLineToString(
    __in PYORI_STRING UserString,
    __inout PVOID * Context,
    __in HANDLE FileHandle
    )
{
    YORI_LIB_LINE_ENDING LineEnding;
    BOOL TimeoutReached;

    return YoriLibReadLineToStringEx(UserString, Context, TRUE, INFINITE, FileHandle, &LineEnding, &TimeoutReached);
}

/**
 Free any context allocated by YoriLibReadLineFromFile .

 @param Context Pointer to the context to free.
 */
VOID
YoriLibLineReadClose(
    __in_opt PVOID Context
    )
{
    PYORI_LIB_LINE_READ_CONTEXT ReadContext = (PYORI_LIB_LINE_READ_CONTEXT)Context;
    if (ReadContext != NULL) {
        if (ReadContext->PreviousBuffer != NULL) {
            YoriLibFree(ReadContext->PreviousBuffer);
        }
        YoriLibFree(ReadContext);
    }
}

// vim:sw=4:ts=4:et:
