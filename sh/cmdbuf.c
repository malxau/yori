/**
 * @file sh/cmdbuf.c
 *
 * Facilities for managing buffers of executing processes
 *
 * Copyright (c) 2017-2019 Malcolm J. Smith
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

#include "yori.h"


/**
 A buffer for a single data stream.  A process may have a different buffered
 data stream for stdout as well as stderr.
 */
typedef struct _YORI_SH_PROCESS_BUFFER {

    /**
     The number of bytes currently allocated to this buffer.
     */
    DWORD BytesAllocated;

    /**
     The number of bytes populated with data in this buffer.
     */
    DWORD BytesPopulated;

    /**
     A handle to the buffer processing thread.
     */
    HANDLE hPumpThread;

    /**
     A lock for the data and sizes referred to in this structure.
     */
    HANDLE Mutex;

    /**
     A handle to a pipe which is the source of data for this buffer.
     */
    HANDLE hSource;

    /**
     A handle to a pipe which will have data pushed to it in real time
     (to support 'fg.')
     */
    HANDLE hMirror;

    /**
     The number of bytes which have been sent to hMirror.
     */
    DWORD BytesSent;

    /**
     The data buffer.
     */
    PCHAR Buffer;

} YORI_SH_PROCESS_BUFFER, *PYORI_SH_PROCESS_BUFFER;

/**
 A structure to record a buffered process.
 */
typedef struct _YORI_SH_BUFFERED_PROCESS {

    /**
     The link into the global list of buffered processes.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The number of references on this buffered process.
     */
    DWORD ReferenceCount;

    /**
     A handle indicating that the buffer pumping thread should terminate.
     */
    HANDLE hCancelPumpEvent;

    /**
     A buffer corresponding to the output stream from the process.
     */
    YORI_SH_PROCESS_BUFFER OutputBuffer;

    /**
     A buffer corresponding to the error stream from the process.
     */
    YORI_SH_PROCESS_BUFFER ErrorBuffer;

} YORI_SH_BUFFERED_PROCESS, *PYORI_SH_BUFFERED_PROCESS;

/**
 The global list of active buffered processes.
 */
YORI_LIST_ENTRY BufferedProcessList;

/**
 Acquire a Win32 mutex, because for some unknowable reason this isn't a
 Win32 function.

 @param Mutex The mutex to acquire.
 */
_Acquires_lock_(Mutex)
VOID
AcquireMutex(
    __in HANDLE Mutex
    )
{

    //
    //  Whether the mutex is acquired is determined by the return value and
    //  the acceptable return values are defined by the parameters.  Analyze
    //  might think we're doing something like closing the handle while using
    //  it as a synchronization primitive, but if that happened, we have
    //  bigger problems.
    //

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 26166) 
#endif
    WaitForSingleObject(Mutex, INFINITE);
}

/**
 Free structures associated with a single input stream.

 @param ThisBuffer Pointer to the single stream's buffers to deallocate.
 */
VOID
YoriShFreeProcessBuffer(
    __in PYORI_SH_PROCESS_BUFFER ThisBuffer
    )
{
    if (ThisBuffer->Buffer != NULL) {
        YoriLibFree(ThisBuffer->Buffer);
    }
    if (ThisBuffer->hMirror != NULL) {
        CloseHandle(ThisBuffer->hMirror);
    }
    if (ThisBuffer->hPumpThread != NULL) {
        CloseHandle(ThisBuffer->hPumpThread);
    }
    if (ThisBuffer->Mutex != NULL) {
        CloseHandle(ThisBuffer->Mutex);
    }
}

/**
 Free a set of process buffers.  By this point the buffers are expected to
 have no further use and no synchronization is performed.

 @param ThisBuffer The set of process buffers to free.
 */
VOID
YoriShFreeProcessBuffers(
    __in PYORI_SH_BUFFERED_PROCESS ThisBuffer
    )
{
    YoriShFreeProcessBuffer(&ThisBuffer->OutputBuffer);
    YoriShFreeProcessBuffer(&ThisBuffer->ErrorBuffer);
    if (ThisBuffer->hCancelPumpEvent != NULL) {
        CloseHandle(ThisBuffer->hCancelPumpEvent);
    }
    YoriLibFree(ThisBuffer);
}

/**
 Code running on a dedicated thread for the duration of an outstanding process
 to populate data into its pipe.

 @param Param A pointer to the process buffer set.

 @return Thread return code, which is ignored for this thread.
 */
DWORD WINAPI
YoriShCmdBufferPumpToNextProcess(
    __in LPVOID Param
    )
{
    PYORI_SH_PROCESS_BUFFER ThisBuffer = (PYORI_SH_PROCESS_BUFFER)Param;
    DWORD BytesSent = 0;
    DWORD BytesWritten;
    DWORD BytesToWrite;

    while (TRUE) {

        BytesToWrite = 4096;
        AcquireMutex(ThisBuffer->Mutex);
        if (BytesSent + BytesToWrite > ThisBuffer->BytesPopulated) {
            BytesToWrite = ThisBuffer->BytesPopulated - BytesSent;
        }

        if (WriteFile(ThisBuffer->hSource,
                      YoriLibAddToPointer(ThisBuffer->Buffer, BytesSent),
                      BytesToWrite,
                      &BytesWritten,
                      NULL)) {

            BytesSent += BytesWritten;
        } else {
            ReleaseMutex(ThisBuffer->Mutex);
            break;
        }
        ReleaseMutex(ThisBuffer->Mutex);

        ASSERT(BytesSent <= ThisBuffer->BytesPopulated);
        if (BytesSent >= ThisBuffer->BytesPopulated) {
            break;
        }
    }

    CloseHandle(ThisBuffer->hSource);
    ThisBuffer->hSource = NULL;

    return 0;
}


/**
 Code running on a dedicated thread for the duration of an outstanding process
 to populate data into its process buffer set.

 @param Param A pointer to the process buffer set.

 @return Thread return code, which is ignored for this thread.
 */
DWORD WINAPI
YoriShCmdBufferPump(
    __in LPVOID Param
    )
{
    PYORI_SH_PROCESS_BUFFER ThisBuffer = (PYORI_SH_PROCESS_BUFFER)Param;
    DWORD BytesRead;
    HANDLE hTemp;

    while (ThisBuffer->hSource != NULL) {

        if (ReadFile(ThisBuffer->hSource,
                     YoriLibAddToPointer(ThisBuffer->Buffer, ThisBuffer->BytesPopulated),
                     ThisBuffer->BytesAllocated - ThisBuffer->BytesPopulated,
                     &BytesRead,
                     NULL)) {

            AcquireMutex(ThisBuffer->Mutex);

            if (BytesRead == 0) {
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
            DWORD LastError = GetLastError();

            AcquireMutex(ThisBuffer->Mutex);

            //
            //  Close the source but let the mirror drain if present.
            //

            if (LastError == ERROR_BROKEN_PIPE) {
                hTemp = ThisBuffer->hSource;
                ThisBuffer->hSource = NULL;
                CloseHandle(hTemp);
            } else {
                break;
            }
        }

        if (ThisBuffer->hMirror != NULL) {
            while (ThisBuffer->BytesSent < ThisBuffer->BytesPopulated) {
                DWORD BytesToWrite;
                DWORD BytesWritten;
                BytesToWrite = 4096;
                if (ThisBuffer->BytesSent + BytesToWrite > ThisBuffer->BytesPopulated) {
                    BytesToWrite = ThisBuffer->BytesPopulated - ThisBuffer->BytesSent;
                }

                if (WriteFile(ThisBuffer->hMirror,
                              YoriLibAddToPointer(ThisBuffer->Buffer, ThisBuffer->BytesSent),
                              BytesToWrite,
                              &BytesWritten,
                              NULL)) {
        
                    ThisBuffer->BytesSent += BytesWritten;
                } else {
                    hTemp = ThisBuffer->hMirror;
                    ThisBuffer->hMirror = NULL;
                    CloseHandle(hTemp);
                    ThisBuffer->BytesSent = 0;
                    break;
                }

                ASSERT(ThisBuffer->BytesSent <= ThisBuffer->BytesPopulated);
            }

        }
        ReleaseMutex(ThisBuffer->Mutex);
    }

    if (ThisBuffer->hSource != NULL) {
        hTemp = ThisBuffer->hSource;
        ThisBuffer->hSource = NULL;
        CloseHandle(hTemp);
    }

    if (ThisBuffer->hMirror != NULL) {
        hTemp = ThisBuffer->hMirror;
        ThisBuffer->hMirror = NULL;
        CloseHandle(hTemp);
    }

    ReleaseMutex(ThisBuffer->Mutex);

    return 0;
}

/**
 Allocate and initialize a buffer for a single input stream.

 @param Buffer Pointer to the buffer to allocate structures for.

 @return TRUE if the buffer is successfully initialized, FALSE if it is not.
 */
__success(return)
BOOL
YoriShAllocateSingleProcessBuffer(
    __out PYORI_SH_PROCESS_BUFFER Buffer
    )
{
    Buffer->BytesAllocated = 1024;
    Buffer->Buffer = YoriLibMalloc(Buffer->BytesAllocated);
    if (Buffer->Buffer == NULL) {
        return FALSE;
    }

    Buffer->Mutex = CreateMutex(NULL, FALSE, NULL);
    if (Buffer->Mutex == NULL) {
        return FALSE;
    }

    return TRUE;
}

/**
 Allocate a new buffered process item.

 @param ExecContext The process whose output requires buffering.

 @return TRUE to indicate a buffer was successfully allocated, FALSE if it
         was not.
 */
__success(return)
BOOL
YoriShCreateNewProcessBuffer(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    PYORI_SH_BUFFERED_PROCESS ThisBuffer;
    DWORD ThreadId;

    if (BufferedProcessList.Next == NULL) {
        YoriLibInitializeListHead(&BufferedProcessList);
    }

    ThisBuffer = YoriLibMalloc(sizeof(YORI_SH_BUFFERED_PROCESS));
    if (ThisBuffer == NULL) {
        return FALSE;
    }

    ZeroMemory(ThisBuffer, sizeof(YORI_SH_BUFFERED_PROCESS));

    //
    //  Create the buffer with two references: one for the ExecContext, and
    //  one for the pump thread, which is released in
    //  YoriShScanProcessBuffersForTeardown
    //

    ThisBuffer->ReferenceCount = 2;

    ThisBuffer->hCancelPumpEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ThisBuffer->hCancelPumpEvent == NULL) {
        YoriShFreeProcessBuffers(ThisBuffer);
        return FALSE;
    }

    //
    //  MSFIX Error handling.  If we fail during this routine, we need to
    //  ensure the caller still owns the pipes, ie., we can't just call
    //  FreeProcessBuffers at different points in this routine after some
    //  pipes are already populated.
    //

    if (ExecContext->StdOutType == StdOutTypeBuffer) {
        if (!YoriShAllocateSingleProcessBuffer(&ThisBuffer->OutputBuffer)) {
            YoriShFreeProcessBuffers(ThisBuffer);
            return FALSE;
        }

        ThisBuffer->OutputBuffer.hSource = ExecContext->StdOut.Buffer.PipeFromProcess;
        ExecContext->StdOut.Buffer.ProcessBuffers = ThisBuffer;

        ThisBuffer->OutputBuffer.hPumpThread = CreateThread(NULL, 0, YoriShCmdBufferPump, &ThisBuffer->OutputBuffer, 0, &ThreadId);
        if (ThisBuffer->OutputBuffer.hPumpThread == NULL) {
            YoriShFreeProcessBuffers(ThisBuffer);
            return FALSE;
        }

    }

    if (ExecContext->StdErrType == StdErrTypeBuffer) {
        if (!YoriShAllocateSingleProcessBuffer(&ThisBuffer->ErrorBuffer)) {
            YoriShFreeProcessBuffers(ThisBuffer);
            return FALSE;
        }

        ThisBuffer->ErrorBuffer.hSource = ExecContext->StdErr.Buffer.PipeFromProcess;
        ExecContext->StdErr.Buffer.ProcessBuffers = ThisBuffer;

        ThisBuffer->ErrorBuffer.hPumpThread = CreateThread(NULL, 0, YoriShCmdBufferPump, &ThisBuffer->ErrorBuffer, 0, &ThreadId);
        if (ThisBuffer->ErrorBuffer.hPumpThread == NULL) {
            YoriShFreeProcessBuffers(ThisBuffer);
            return FALSE;
        }
    }

    YoriLibAppendList(&BufferedProcessList, &ThisBuffer->ListEntry);

    return TRUE;
}

/**
 Append to an existing buffered process output.

 @param ExecContext The process whose output requires buffering.

 @return TRUE to indicate a buffer was successfully initialized, FALSE if it
         was not.
 */
__success(return)
BOOL
YoriShAppendToExistingProcessBuffer(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    PYORI_SH_BUFFERED_PROCESS ThisBuffer;
    DWORD ThreadId;

    //
    //  MSFIX It's not possible today to have a second process append to
    //  a previous process error stream.  In the long run this should
    //  be a child shell process, so as far as the parent is concerned
    //  there's only one process writing to buffers, and as far as the
    //  child's concerned nothing special is happening.
    //

    ASSERT(ExecContext->StdOutType == StdOutTypeBuffer);
    ASSERT(ExecContext->StdErrType != StdErrTypeBuffer);

    ThisBuffer = ExecContext->StdOut.Buffer.ProcessBuffers;
    YoriShWaitForProcessBufferToFinalize(ThisBuffer);
    ASSERT(WaitForSingleObject(ThisBuffer->OutputBuffer.hPumpThread, 0) == WAIT_OBJECT_0);
    CloseHandle(ThisBuffer->OutputBuffer.hPumpThread);
    ThisBuffer->OutputBuffer.hPumpThread = CreateThread(NULL, 0, YoriShCmdBufferPump, &ThisBuffer->OutputBuffer, 0, &ThreadId);
    if (ThisBuffer->OutputBuffer.hPumpThread == NULL) {
        return FALSE;
    }

    //
    //  Add one reference for the ExecContext.  This buffer must have
    //  already had a reference for the output pump, and it's reused
    //  for the new output pump.
    //

    YoriShReferenceProcessBuffer(ThisBuffer);

    return TRUE;
}

/**
 Create a background thread and pipes so that the next process will
 receive its stdin from a background thread pushing data from a buffer
 created by the previous process.  This is used on builtin commands which
 execute on the primary thread but want to output a lot of data to the
 next process.

 @param ExecContext The process whose output requires pushing to the next
        process in the chain.

 @return TRUE to indicate a pump was successfully initialized, FALSE if it
         was not.
 */
__success(return)
BOOL
YoriShForwardProcessBufferToNextProcess(
    __in PYORI_SH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    PYORI_SH_BUFFERED_PROCESS ThisBuffer = ExecContext->StdOut.Buffer.ProcessBuffers;
    DWORD ThreadId;
    HANDLE ReadHandle, WriteHandle;

    ASSERT(ExecContext->StdOutType == StdOutTypeBuffer);
    ASSERT(ExecContext->StdErrType != StdErrTypeBuffer);

    if (ExecContext->NextProgram != NULL &&
        ExecContext->NextProgram->StdInType == StdInTypePipe &&
        CreatePipe(&ReadHandle, &WriteHandle, NULL, 0)) {

        ExecContext->NextProgram->StdIn.Pipe.PipeFromPriorProcess = ReadHandle;

        //
        //  If we're forwarding to the next process, the previous one
        //  should be finished
        //

        if (ThisBuffer->OutputBuffer.hPumpThread != NULL) {
            if (WaitForSingleObject(ThisBuffer->OutputBuffer.hPumpThread, INFINITE) == WAIT_OBJECT_0) {
                CloseHandle(ThisBuffer->OutputBuffer.hPumpThread);
                ThisBuffer->OutputBuffer.hPumpThread = NULL;
            }
        }

        //
        //  Reverse the flow and create a thread to pump data out
        //

        ThisBuffer->OutputBuffer.hSource = WriteHandle;
        ThisBuffer->OutputBuffer.hPumpThread = CreateThread(NULL, 0, YoriShCmdBufferPumpToNextProcess, &ThisBuffer->OutputBuffer, 0, &ThreadId);
        if (ThisBuffer->OutputBuffer.hPumpThread == NULL) {
            return FALSE;
        }

        return TRUE;

    } else {
        return FALSE;
    }
}

/**
 Dereference an existing outstanding process buffer set.

 @param ThisBuffer The buffer set to dereference and possibly free.
 */
VOID
YoriShDereferenceProcessBuffer(
    __in PVOID ThisBuffer
    )
{
    PYORI_SH_BUFFERED_PROCESS ThisBufferNonOpaque = (PYORI_SH_BUFFERED_PROCESS)ThisBuffer;
    ThisBufferNonOpaque->ReferenceCount--;

    if (ThisBufferNonOpaque->ReferenceCount == 0) {
        YoriLibRemoveListItem(&ThisBufferNonOpaque->ListEntry);
        YoriShFreeProcessBuffers(ThisBufferNonOpaque);
    }
}

/**
 Add a reference to an existing process buffer set.

 @param ThisBuffer The buffer set to reference.
 */
VOID
YoriShReferenceProcessBuffer(
    __in PVOID ThisBuffer
    )
{
    PYORI_SH_BUFFERED_PROCESS ThisBufferNonOpaque = (PYORI_SH_BUFFERED_PROCESS)ThisBuffer;
    ASSERT(ThisBufferNonOpaque->ReferenceCount > 0);
    ThisBufferNonOpaque->ReferenceCount++;
}

/**
 Return contents of a process buffer.

 @param ThisBuffer Pointer to the buffer to any output from.

 @param String Pointer to a string to contain the resulting buffer.
        This buffer is expected to be unallocated on entry and is
        allocated to the correct size in this routine.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShGetProcessBuffer(
    __in PYORI_SH_PROCESS_BUFFER ThisBuffer,
    __out PYORI_STRING String
    )
{
    DWORD LengthNeeded;

    if (ThisBuffer->Buffer == NULL) {
        return FALSE;
    }

    AcquireMutex(ThisBuffer->Mutex);

    LengthNeeded = YoriLibGetMultibyteInputSizeNeeded(ThisBuffer->Buffer, ThisBuffer->BytesPopulated);
    if (LengthNeeded == 0 && ThisBuffer->BytesPopulated == 0) {
        ReleaseMutex(ThisBuffer->Mutex);
        YoriLibInitEmptyString(String);
        return TRUE;
    }

    if (!YoriLibAllocateString(String, LengthNeeded)) {
        ReleaseMutex(ThisBuffer->Mutex);
        return FALSE;
    }

    YoriLibMultibyteInput(ThisBuffer->Buffer, ThisBuffer->BytesPopulated, String->StartOfString, String->LengthAllocated);
    String->LengthInChars = LengthNeeded;
    ReleaseMutex(ThisBuffer->Mutex);

    return TRUE;
}
    

/**
 Return contents of a process standard output buffer.

 @param ThisBuffer Pointer to the process buffers to fetch standard output
        information from.

 @param String Pointer to a string to contain the resulting standard output
        buffer.  This buffer is expected to be unallocated on entry and is
        allocated to the correct size in this routine.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShGetProcessOutputBuffer(
    __in PVOID ThisBuffer,
    __out PYORI_STRING String
    )
{
    BOOL Result;
    PYORI_SH_BUFFERED_PROCESS ThisBufferNonOpaque = (PYORI_SH_BUFFERED_PROCESS)ThisBuffer;
    Result = YoriShGetProcessBuffer(&ThisBufferNonOpaque->OutputBuffer, String);
    return Result;
}

/**
 Return contents of a process standard error buffer.

 @param ThisBuffer Pointer to the process buffers to fetch standard error
        information from.

 @param String Pointer to a string to contain the resulting standard error
        buffer.  This buffer is expected to be unallocated on entry and is
        allocated to the correct size in this routine.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShGetProcessErrorBuffer(
    __in PVOID ThisBuffer,
    __out PYORI_STRING String
    )
{
    BOOL Result;
    PYORI_SH_BUFFERED_PROCESS ThisBufferNonOpaque = (PYORI_SH_BUFFERED_PROCESS)ThisBuffer;
    Result = YoriShGetProcessBuffer(&ThisBufferNonOpaque->ErrorBuffer, String);
    return Result;
}

/**
 Either check whether a single input stream has completed or wait for it to
 complete.

 @param ThisBuffer Pointer to the single input stream to check.

 @param TeardownAll If TRUE, wait for the buffer to complete; if FALSE, check
        whether it has completed without waiting.

 @return TRUE to indicate the input stream has completed, FALSE if it has not.
 */
BOOL
YoriShTeardownSingleProcessBuffer(
    __in PYORI_SH_PROCESS_BUFFER ThisBuffer,
    __in BOOL TeardownAll
    )
{
    BOOL Torndown = FALSE;
    if (TeardownAll) {

        //
        //  TerminateThread is inherently evil, and its use stems from the way
        //  pipes can't be waited on to indicate input, and are created
        //  synchronously, so the pump thread can be blocked in a read call
        //  with no cooperative way to communicate with it.  This call only
        //  works because it's invoked on process teardown by the main thread,
        //  so any waiters will die soon enough, and the pump thread NULLs
        //  objects before closing them so we err on the side of failing to
        //  close things rather than double close because the process is about
        //  to die anyway.  But it's still evil.
        //
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6258)
#endif
        TerminateThread(ThisBuffer->hPumpThread, 0);
        if (WaitForSingleObject(ThisBuffer->hPumpThread, INFINITE) == WAIT_OBJECT_0) {
            CloseHandle(ThisBuffer->hPumpThread);
            ThisBuffer->hPumpThread = NULL;
            Torndown = TRUE;
        }
    } else {
        if (WaitForSingleObject(ThisBuffer->hPumpThread, 0) == WAIT_OBJECT_0) {
            CloseHandle(ThisBuffer->hPumpThread);
            ThisBuffer->hPumpThread = NULL;
            Torndown = TRUE;
        }
    }
    return Torndown;
}

/**
 Scan the set of outstanding process buffers and delete any that have
 completed.

 @param TeardownAll TRUE if the shell is exiting and wants to tear down all
        state.  FALSE if this is a periodic check to tear down things that
        have been around a while.

 @return Always TRUE currently.
 */
__success(return)
BOOL
YoriShScanProcessBuffersForTeardown(
    __in BOOL TeardownAll
    )
{
    PYORI_SH_BUFFERED_PROCESS ThisBuffer;
    PYORI_LIST_ENTRY ListEntry;
    DWORD ThreadsFound;

    if (BufferedProcessList.Next == NULL) {
        return TRUE;
    }

    ListEntry = YoriLibGetNextListEntry(&BufferedProcessList, NULL);
    while (ListEntry != NULL) {
        ThisBuffer = CONTAINING_RECORD(ListEntry, YORI_SH_BUFFERED_PROCESS, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&BufferedProcessList, ListEntry);
        if (ThisBuffer->hCancelPumpEvent != NULL && TeardownAll) {
            SetEvent(ThisBuffer->hCancelPumpEvent);
        }
        ThreadsFound = 0;
        if (ThisBuffer->OutputBuffer.hPumpThread != NULL) {
            ThreadsFound++;
            YoriShTeardownSingleProcessBuffer(&ThisBuffer->OutputBuffer, TeardownAll);
        }
        if (ThisBuffer->ErrorBuffer.hPumpThread != NULL) {
            ThreadsFound++;
            YoriShTeardownSingleProcessBuffer(&ThisBuffer->ErrorBuffer, TeardownAll);
        }

        //
        //  If there are no longer any active threads but there was before
        //  this function ran, the buffers are no longer referenced by active
        //  threads
        //

        if (ThisBuffer->OutputBuffer.hPumpThread == NULL &&
            ThisBuffer->ErrorBuffer.hPumpThread == NULL &&
            ThreadsFound > 0) {

            YoriShDereferenceProcessBuffer(ThisBuffer);
        }
    }

    return TRUE;
}

/**
 Wait for a buffer to have complete contents by waiting for the thread that
 is inputting contents to terminate.  Note this thread is not synchronized
 with any process termination, so ensuring complete contents requires waiting
 for the thread to drain, which may occur after any process generating output
 has terminated.

 @param ThisBuffer Pointer to the buffer to wait for completion.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriShWaitForProcessBufferToFinalize(
    __in PVOID ThisBuffer
    )
{
    PYORI_SH_BUFFERED_PROCESS ThisBufferNonOpaque = (PYORI_SH_BUFFERED_PROCESS)ThisBuffer;
    if (ThisBufferNonOpaque->OutputBuffer.hPumpThread != NULL) {
        if (WaitForSingleObject(ThisBufferNonOpaque->OutputBuffer.hPumpThread, INFINITE) != WAIT_OBJECT_0) {
            ASSERT(FALSE);
            return FALSE;
        }
    }
    if (ThisBufferNonOpaque->ErrorBuffer.hPumpThread != NULL) {
        if (WaitForSingleObject(ThisBufferNonOpaque->ErrorBuffer.hPumpThread, INFINITE) != WAIT_OBJECT_0) {
            ASSERT(FALSE);
            return FALSE;
        }
    }
    return TRUE;
}

/**
 Take any existing output from a set of buffers and send it to a pipe handle,
 and continue sending further output into the pipe handle.

 @param ThisBuffer Pointer to the buffers to forward output from.

 @param hPipeOutput Specifies a pipe to forward job standard output into.

 @param hPipeErrors Specifies a pipe to forward job standard error into.

 @return TRUE to indicate success, FALSE to indicate error.
 */
__success(return)
BOOL
YoriShPipeProcessBuffers(
    __in PVOID ThisBuffer,
    __in_opt HANDLE hPipeOutput,
    __in_opt HANDLE hPipeErrors
    )
{
    BOOL HaveOutput;
    BOOL HaveErrors;
    BOOL Collision;
    PYORI_SH_BUFFERED_PROCESS ThisBufferNonOpaque = (PYORI_SH_BUFFERED_PROCESS)ThisBuffer;

    HaveOutput = FALSE;
    HaveErrors = FALSE;

    //
    //  Check if data exists for the streams that redirection is requested
    //  for.
    //

    if (hPipeOutput != NULL) {
        if (ThisBufferNonOpaque->OutputBuffer.Buffer != NULL) {
            HaveOutput = TRUE;
        } else {
            return FALSE;
        }
    }

    if (hPipeErrors != NULL) {
        if (ThisBufferNonOpaque->ErrorBuffer.Buffer != NULL) {
            HaveErrors = TRUE;
        } else {
            return FALSE;
        }
    }

    if (HaveOutput) {
        AcquireMutex(ThisBufferNonOpaque->OutputBuffer.Mutex);
    }
    if (HaveErrors) {
        AcquireMutex(ThisBufferNonOpaque->ErrorBuffer.Mutex);
    }

    //
    //  Check if the redirection requested is already being performed.
    //

    Collision = FALSE;

    if (HaveOutput) {
        if (ThisBufferNonOpaque->OutputBuffer.hMirror != NULL ||
            ThisBufferNonOpaque->OutputBuffer.hSource == NULL) {
            Collision = TRUE;
        }
    }

    if (HaveErrors) {
        if (ThisBufferNonOpaque->ErrorBuffer.hMirror != NULL ||
            ThisBufferNonOpaque->ErrorBuffer.hSource == NULL) {
            Collision = TRUE;
        }
    }

    if (Collision) {
        if (HaveOutput) {
            ReleaseMutex(ThisBufferNonOpaque->OutputBuffer.Mutex);
        }
        if (HaveErrors) {
            ReleaseMutex(ThisBufferNonOpaque->ErrorBuffer.Mutex);
        }
        return FALSE;
    }

    //
    //  While locks are acquired, update the mirror handle.
    //

    if (HaveOutput) {
        ThisBufferNonOpaque->OutputBuffer.hMirror = hPipeOutput;
        ASSERT(ThisBufferNonOpaque->OutputBuffer.BytesSent == 0);
    }

    if (HaveErrors) {
        ThisBufferNonOpaque->ErrorBuffer.hMirror = hPipeErrors;
        ASSERT(ThisBufferNonOpaque->ErrorBuffer.BytesSent == 0);
    }

    if (HaveOutput) {
        ReleaseMutex(ThisBufferNonOpaque->OutputBuffer.Mutex);
    }
    if (HaveErrors) {
        ReleaseMutex(ThisBufferNonOpaque->ErrorBuffer.Mutex);
    }

    return TRUE;
}

// vim:sw=4:ts=4:et:
