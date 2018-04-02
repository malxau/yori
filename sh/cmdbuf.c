/**
 * @file sh/cmdbuf.c
 *
 * Facilities for managing buffers of executing processes
 *
 * Copyright (c) 2017 Malcolm J. Smith
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
typedef struct _YORI_PROCESS_BUFFER {

    /**
     The number of bytes currently allocated to this buffer.
     MSFIX As of now, this will never change for the lifetime of the process.
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

} YORI_PROCESS_BUFFER, *PYORI_PROCESS_BUFFER;

/**
 A structure to record a buffered process.
 */
typedef struct _YORI_BUFFERED_PROCESS {

    /**
     The link into the global list of buffered processes.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The number of references on this buffered process.
     */
    DWORD ReferenceCount;

    /**
     A handle to the buffer processing thread.
     */
    HANDLE hPumpThread;

    /**
     A lock for the data and sizes referred to in this structure.
     */
    HANDLE Mutex;

    /**
     A buffer corresponding to the output stream from the process.
     */
    YORI_PROCESS_BUFFER OutputBuffer;

    /**
     A buffer corresponding to the error stream from the process.
     */
    YORI_PROCESS_BUFFER ErrorBuffer;

} YORI_BUFFERED_PROCESS, *PYORI_BUFFERED_PROCESS;

/**
 The global list of active buffered processes.
 */
YORI_LIST_ENTRY BufferedProcessList;

/**
 Acquire a Win32 mutex, because for some unknowable reason this isn't a
 Win32 function.

 @param Mutex The mutex to acquire.
 */
VOID
AcquireMutex(
    __in HANDLE Mutex
    )
{
    WaitForSingleObject(Mutex, INFINITE);
}

/**
 Free a set of process buffers.  By this point the buffers are expected to
 have no further use and no synchronization is performed.

 @param ThisBuffer The set of process buffers to free.
 */
VOID
YoriShFreeProcessBuffers(
    __in PYORI_BUFFERED_PROCESS ThisBuffer
    )
{
    if (ThisBuffer->OutputBuffer.Buffer != NULL) {
        YoriLibFree(ThisBuffer->OutputBuffer.Buffer);
    }
    if (ThisBuffer->OutputBuffer.hMirror != NULL) {
        CloseHandle(ThisBuffer->OutputBuffer.hMirror);
    }
    if (ThisBuffer->ErrorBuffer.Buffer != NULL) {
        YoriLibFree(ThisBuffer->ErrorBuffer.Buffer);
    }
    if (ThisBuffer->ErrorBuffer.hMirror != NULL) {
        CloseHandle(ThisBuffer->ErrorBuffer.hMirror);
    }
    if (ThisBuffer->hPumpThread != NULL) {
        CloseHandle(ThisBuffer->hPumpThread);
    }
    if (ThisBuffer->Mutex) {
        CloseHandle(ThisBuffer->Mutex);
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
    PYORI_BUFFERED_PROCESS ThisBuffer = (PYORI_BUFFERED_PROCESS)Param;
    DWORD BytesSent = 0;
    DWORD BytesWritten;
    DWORD BytesToWrite;

    //
    //  This code is used to pipe builtins to an external program, but
    //  right now there's no way to pipe errors as distinct from output
    //

    ASSERT(ThisBuffer->ErrorBuffer.Buffer == NULL);

    while (TRUE) {

        BytesToWrite = 4096;
        AcquireMutex(ThisBuffer->Mutex);
        if (BytesSent + BytesToWrite > ThisBuffer->OutputBuffer.BytesPopulated) {
            BytesToWrite = ThisBuffer->OutputBuffer.BytesPopulated - BytesSent;
        }

        if (WriteFile(ThisBuffer->OutputBuffer.hSource,
                      YoriLibAddToPointer(ThisBuffer->OutputBuffer.Buffer, BytesSent),
                      BytesToWrite,
                      &BytesWritten,
                      NULL)) {

            BytesSent += BytesWritten;
        } else {
            ReleaseMutex(ThisBuffer->Mutex);
            break;
        }
        ReleaseMutex(ThisBuffer->Mutex);

        ASSERT(BytesSent <= ThisBuffer->OutputBuffer.BytesPopulated);
        if (BytesSent >= ThisBuffer->OutputBuffer.BytesPopulated) {
            break;
        }
    }

    CloseHandle(ThisBuffer->OutputBuffer.hSource);
    ThisBuffer->OutputBuffer.hSource = NULL;

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
    PYORI_BUFFERED_PROCESS ThisBuffer = (PYORI_BUFFERED_PROCESS)Param;
    DWORD BytesRead;
    HANDLE HandlesToWait[2];
    DWORD HandleCount;
    DWORD Error;
    PYORI_PROCESS_BUFFER SignalledBuffer;

    while (TRUE) {
        HandleCount = 0;
        if (ThisBuffer->OutputBuffer.hSource != NULL) {
            HandlesToWait[HandleCount] = ThisBuffer->OutputBuffer.hSource;
            HandleCount++;
        }
        if (ThisBuffer->ErrorBuffer.hSource != NULL) {
            HandlesToWait[HandleCount] = ThisBuffer->ErrorBuffer.hSource;
            HandleCount++;
        }

        if (HandleCount == 0) {
            AcquireMutex(ThisBuffer->Mutex);
            break;
        }

        Error = WaitForMultipleObjects(HandleCount, HandlesToWait, FALSE, INFINITE);
        if (Error != WAIT_OBJECT_0 && Error != (WAIT_OBJECT_0 + 1)) {
            AcquireMutex(ThisBuffer->Mutex);
            break;
        }

        if (Error == WAIT_OBJECT_0) {
            if (HandlesToWait[0] == ThisBuffer->OutputBuffer.hSource) {
                SignalledBuffer = &ThisBuffer->OutputBuffer;
            } else {
                ASSERT(HandlesToWait[0] == ThisBuffer->ErrorBuffer.hSource);
                SignalledBuffer = &ThisBuffer->ErrorBuffer;
            }
        } else {
            ASSERT(Error == WAIT_OBJECT_0 + 1);
            if (HandlesToWait[1] == ThisBuffer->OutputBuffer.hSource) {
                SignalledBuffer = &ThisBuffer->OutputBuffer;
            } else {
                ASSERT(HandlesToWait[1] == ThisBuffer->ErrorBuffer.hSource);
                SignalledBuffer = &ThisBuffer->ErrorBuffer;
            }
        }

        if (ReadFile(SignalledBuffer->hSource,
                     YoriLibAddToPointer(SignalledBuffer->Buffer, SignalledBuffer->BytesPopulated),
                     SignalledBuffer->BytesAllocated - SignalledBuffer->BytesPopulated,
                     &BytesRead,
                     NULL)) {

            AcquireMutex(ThisBuffer->Mutex);

            SignalledBuffer->BytesPopulated += BytesRead;
            ASSERT(SignalledBuffer->BytesPopulated <= SignalledBuffer->BytesAllocated);
            if (SignalledBuffer->BytesPopulated >= SignalledBuffer->BytesAllocated) {
                DWORD NewBytesAllocated = SignalledBuffer->BytesAllocated * 4;
                PCHAR NewBuffer;

                NewBuffer = YoriLibMalloc(NewBytesAllocated);
                if (NewBuffer == NULL) {
                    break;
                }

                CopyMemory(NewBuffer, SignalledBuffer->Buffer, SignalledBuffer->BytesAllocated);
                YoriLibFree(SignalledBuffer->Buffer);
                SignalledBuffer->Buffer = NewBuffer;
                SignalledBuffer->BytesAllocated = NewBytesAllocated;
            }
        } else {
            DWORD LastError = GetLastError();

            AcquireMutex(ThisBuffer->Mutex);

            if (LastError == ERROR_BROKEN_PIPE) {
                CloseHandle(SignalledBuffer->hSource);
                SignalledBuffer->hSource = NULL;
            } else {
                break;
            }
        }

        if (SignalledBuffer->hMirror != NULL) {
            while (SignalledBuffer->BytesSent < SignalledBuffer->BytesPopulated) {
                DWORD BytesToWrite;
                DWORD BytesWritten;
                BytesToWrite = 4096;
                if (SignalledBuffer->BytesSent + BytesToWrite > SignalledBuffer->BytesPopulated) {
                    BytesToWrite = SignalledBuffer->BytesPopulated - SignalledBuffer->BytesSent;
                }

                if (WriteFile(SignalledBuffer->hMirror,
                              YoriLibAddToPointer(SignalledBuffer->Buffer, SignalledBuffer->BytesSent),
                              BytesToWrite,
                              &BytesWritten,
                              NULL)) {
        
                    SignalledBuffer->BytesSent += BytesWritten;
                } else {
                    CloseHandle(SignalledBuffer->hMirror);
                    SignalledBuffer->hMirror = NULL;
                    SignalledBuffer->BytesSent = 0;
                    break;
                }

                ASSERT(SignalledBuffer->BytesSent <= SignalledBuffer->BytesPopulated);
            }

        }
        ReleaseMutex(ThisBuffer->Mutex);
    }

    if (ThisBuffer->OutputBuffer.hSource != NULL) {
        CloseHandle(ThisBuffer->OutputBuffer.hSource);
        ThisBuffer->OutputBuffer.hSource = NULL;
    }

    if (ThisBuffer->OutputBuffer.hMirror != NULL) {
        CloseHandle(ThisBuffer->OutputBuffer.hMirror);
        ThisBuffer->OutputBuffer.hMirror = NULL;
    }

    if (ThisBuffer->ErrorBuffer.hSource != NULL) {
        CloseHandle(ThisBuffer->ErrorBuffer.hSource);
        ThisBuffer->ErrorBuffer.hSource = NULL;
    }

    if (ThisBuffer->ErrorBuffer.hMirror != NULL) {
        CloseHandle(ThisBuffer->ErrorBuffer.hMirror);
        ThisBuffer->ErrorBuffer.hMirror = NULL;
    }

    ReleaseMutex(ThisBuffer->Mutex);

    return 0;
}

/**
 Allocate a new buffered process item.

 @param ExecContext The process whose output requires buffering.

 @return TRUE to indicate a buffer was successfully allocated, FALSE if it
         was not.
 */
BOOL
YoriShCreateNewProcessBuffer(
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    PYORI_BUFFERED_PROCESS ThisBuffer;
    DWORD ThreadId;

    if (BufferedProcessList.Next == NULL) {
        YoriLibInitializeListHead(&BufferedProcessList);
    }

    ThisBuffer = YoriLibMalloc(sizeof(YORI_BUFFERED_PROCESS));
    if (ThisBuffer == NULL) {
        return FALSE;
    }

    ZeroMemory(ThisBuffer, sizeof(YORI_BUFFERED_PROCESS));

    //
    //  Create the buffer with two references: one for the ExecContext, and
    //  one for the pump thread, which is released in
    //  YoriShScanProcessBuffersForTeardown
    //

    ThisBuffer->ReferenceCount = 2;

    ThisBuffer->Mutex = CreateMutex(NULL, FALSE, NULL);
    if (ThisBuffer->Mutex == NULL) {
        YoriShFreeProcessBuffers(ThisBuffer);
        return FALSE;
    }

    if (ExecContext->StdOutType == StdOutTypeBuffer) {
        ThisBuffer->OutputBuffer.BytesAllocated = 1024;
        ThisBuffer->OutputBuffer.Buffer = YoriLibMalloc(ThisBuffer->OutputBuffer.BytesAllocated);
        if (ThisBuffer->OutputBuffer.Buffer == NULL) {
            YoriShFreeProcessBuffers(ThisBuffer);
            return FALSE;
        }

        ThisBuffer->OutputBuffer.hSource = ExecContext->StdOut.Buffer.PipeFromProcess;
        ExecContext->StdOut.Buffer.ProcessBuffers = ThisBuffer;
    }

    if (ExecContext->StdErrType == StdErrTypeBuffer) {
        ThisBuffer->ErrorBuffer.BytesAllocated = 1024;
        ThisBuffer->ErrorBuffer.Buffer = YoriLibMalloc(ThisBuffer->ErrorBuffer.BytesAllocated);
        if (ThisBuffer->ErrorBuffer.Buffer == NULL) {
            YoriShFreeProcessBuffers(ThisBuffer);
            return FALSE;
        }

        ThisBuffer->ErrorBuffer.hSource = ExecContext->StdErr.Buffer.PipeFromProcess;
        ExecContext->StdErr.Buffer.ProcessBuffers = ThisBuffer;
    }

    ThisBuffer->hPumpThread = CreateThread(NULL, 0, YoriShCmdBufferPump, ThisBuffer, 0, &ThreadId);
    if (ThisBuffer->hPumpThread == NULL) {
        YoriShFreeProcessBuffers(ThisBuffer);
        return FALSE;
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
BOOL
YoriShAppendToExistingProcessBuffer(
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    PYORI_BUFFERED_PROCESS ThisBuffer;
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
    ASSERT(WaitForSingleObject(ThisBuffer->hPumpThread, 0) == WAIT_OBJECT_0);
    CloseHandle(ThisBuffer->hPumpThread);
    ThisBuffer->hPumpThread = CreateThread(NULL, 0, YoriShCmdBufferPump, ThisBuffer, 0, &ThreadId);
    if (ThisBuffer->hPumpThread == NULL) {
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
BOOL
YoriShForwardProcessBufferToNextProcess(
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    PYORI_BUFFERED_PROCESS ThisBuffer = ExecContext->StdOut.Buffer.ProcessBuffers;
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

        if (ThisBuffer->hPumpThread != NULL) {
            if (WaitForSingleObject(ThisBuffer->hPumpThread, INFINITE) == WAIT_OBJECT_0) {
                CloseHandle(ThisBuffer->hPumpThread);
                ThisBuffer->hPumpThread = NULL;
            }
        }

        //
        //  Reverse the flow and create a thread to pump data out
        //

        ThisBuffer->OutputBuffer.hSource = WriteHandle;
        ThisBuffer->hPumpThread = CreateThread(NULL, 0, YoriShCmdBufferPumpToNextProcess, ThisBuffer, 0, &ThreadId);
        if (ThisBuffer->hPumpThread == NULL) {
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
    __in PYORI_BUFFERED_PROCESS ThisBuffer
    )
{
    ThisBuffer->ReferenceCount--;

    if (ThisBuffer->ReferenceCount == 0) {
        YoriLibRemoveListItem(&ThisBuffer->ListEntry);
        YoriShFreeProcessBuffers(ThisBuffer);
    }
}

/**
 Add a reference to an existing process buffer set.

 @param ThisBuffer The buffer set to reference.
 */
VOID
YoriShReferenceProcessBuffer(
    __in PYORI_BUFFERED_PROCESS ThisBuffer
    )
{
    ASSERT(ThisBuffer->ReferenceCount > 0);
    ThisBuffer->ReferenceCount++;
}

/**
 Return contents of a process buffer.

 @param ThisBuffer Pointer to the buffer to any output from.

 @param String Pointer to a string to contain the resulting buffer.
        This buffer is expected to be unallocated on entry and is
        allocated to the correct size in this routine.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShGetProcessBuffer(
    __in PYORI_PROCESS_BUFFER ThisBuffer,
    __out PYORI_STRING String
    )
{
    DWORD LengthNeeded;

    if (ThisBuffer->Buffer == NULL) {
        return FALSE;
    }


    LengthNeeded = YoriLibGetMultibyteInputSizeNeeded(ThisBuffer->Buffer, ThisBuffer->BytesPopulated);
    if (LengthNeeded == 0 && ThisBuffer->BytesPopulated == 0) {
        YoriLibInitEmptyString(String);
        return TRUE;
    }

    if (!YoriLibAllocateString(String, LengthNeeded)) {
        return FALSE;
    }

    YoriLibMultibyteInput(ThisBuffer->Buffer, ThisBuffer->BytesPopulated, String->StartOfString, String->LengthAllocated);
    String->LengthInChars = LengthNeeded;

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
BOOL
YoriShGetProcessOutputBuffer(
    __in PYORI_BUFFERED_PROCESS ThisBuffer,
    __out PYORI_STRING String
    )
{
    BOOL Result;
    AcquireMutex(ThisBuffer->Mutex);
    Result = YoriShGetProcessBuffer(&ThisBuffer->OutputBuffer, String);
    ReleaseMutex(ThisBuffer->Mutex);
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
BOOL
YoriShGetProcessErrorBuffer(
    __in PYORI_BUFFERED_PROCESS ThisBuffer,
    __out PYORI_STRING String
    )
{
    BOOL Result;
    AcquireMutex(ThisBuffer->Mutex);
    Result = YoriShGetProcessBuffer(&ThisBuffer->ErrorBuffer, String);
    ReleaseMutex(ThisBuffer->Mutex);
    return Result;
}

/**
 Scan the set of outstanding process buffers and delete any that have
 completed.

 @return Always TRUE currently.
 */
BOOL
YoriShScanProcessBuffersForTeardown()
{
    PYORI_BUFFERED_PROCESS ThisBuffer;
    PYORI_LIST_ENTRY ListEntry;

    if (BufferedProcessList.Next == NULL) {
        return TRUE;
    }

    ListEntry = YoriLibGetNextListEntry(&BufferedProcessList, NULL);
    while (ListEntry != NULL) {
        ThisBuffer = CONTAINING_RECORD(ListEntry, YORI_BUFFERED_PROCESS, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&BufferedProcessList, ListEntry);
        if (ThisBuffer->hPumpThread != NULL) {
            if (WaitForSingleObject(ThisBuffer->hPumpThread, 0) == WAIT_OBJECT_0) {
                CloseHandle(ThisBuffer->hPumpThread);
                ThisBuffer->hPumpThread = NULL;
                YoriShDereferenceProcessBuffer(ThisBuffer);
            }
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
BOOL
YoriShWaitForProcessBufferToFinalize(
    __in PYORI_BUFFERED_PROCESS ThisBuffer
    )
{
    if (WaitForSingleObject(ThisBuffer->hPumpThread, INFINITE) != WAIT_OBJECT_0) {
        ASSERT(FALSE);
        return FALSE;
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
BOOL
YoriShPipeProcessBuffers(
    __in PYORI_BUFFERED_PROCESS ThisBuffer,
    __in HANDLE hPipeOutput,
    __in HANDLE hPipeErrors
    )
{
    BOOL HaveOutput;
    BOOL HaveErrors;
    BOOL Collision;

    HaveOutput = FALSE;
    HaveErrors = FALSE;

    if (hPipeOutput != NULL) {
        if (ThisBuffer->OutputBuffer.Buffer != NULL) {
            HaveOutput = TRUE;
        } else {
            return FALSE;
        }
    }

    if (hPipeErrors != NULL) {
        if (ThisBuffer->ErrorBuffer.Buffer != NULL) {
            HaveErrors = TRUE;
        } else {
            return FALSE;
        }
    }

    AcquireMutex(ThisBuffer->Mutex);

    Collision = FALSE;

    if (HaveOutput) {
        if (ThisBuffer->OutputBuffer.hMirror != NULL ||
            ThisBuffer->OutputBuffer.hSource == NULL) {
            Collision = TRUE;
        }
    }

    if (HaveErrors) {
        if (ThisBuffer->ErrorBuffer.hMirror != NULL ||
            ThisBuffer->ErrorBuffer.hSource == NULL) {
            Collision = TRUE;
        }
    }

    if (Collision) {
        ReleaseMutex(ThisBuffer->Mutex);
        return FALSE;
    }

    if (HaveOutput) {
        ThisBuffer->OutputBuffer.hMirror = hPipeOutput;
        ASSERT(ThisBuffer->OutputBuffer.BytesSent == 0);
    }

    if (HaveErrors) {
        ThisBuffer->ErrorBuffer.hMirror = hPipeErrors;
        ASSERT(ThisBuffer->ErrorBuffer.BytesSent == 0);
    }

    ReleaseMutex(ThisBuffer->Mutex);

    return TRUE;
}

// vim:sw=4:ts=4:et:
