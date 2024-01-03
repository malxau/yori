/**
 * @file lib/filecomp.c
 *
 * Yori lib perform transparent individual file compression on background
 * threads
 *
 * Copyright (c) 2018 Malcolm J. Smith
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

/**
 A single item to compress or decompress.
 */
typedef struct _YORILIB_PENDING_ACTION {

    /**
     A list of files requiring compression.
     */
    YORI_LIST_ENTRY CompressList;

    /**
     The file name to compress.
     */
    YORI_STRING FileName;

    /**
     If the file should be compressed, set to TRUE.  If the file should be
     decompressed, set to FALSE.
     */
    BOOL Compress;

} YORILIB_PENDING_ACTION, *PYORILIB_PENDING_ACTION;

/**
 Set up the compress context to contain support for the compression thread pool.

 @param CompressContext Pointer to the compress context.

 @param CompressionAlgorithm The compression algorithm to use for this set of
        compressed files.

 @return TRUE if the context was successfully initialized for compression,
         FALSE if it was not.
 */
BOOL
YoriLibInitializeCompressContext(
    __in PYORILIB_COMPRESS_CONTEXT CompressContext,
    __in YORILIB_COMPRESS_ALGORITHM CompressionAlgorithm
    )
{
    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);

    CompressContext->CompressionAlgorithm = CompressionAlgorithm;

    //
    //  Create threads equal to the number of CPUs.  The system can compress
    //  chunks of data on background threads, so this is just the number of
    //  threads initiating work.  Unfortunately, the call to CreateFile
    //  after copy has a tendency to block, so we need this to be part of
    //  the threadpool to prevent bottlenecking the copy.
    //

    CompressContext->MaxThreads = (YORI_ALLOC_SIZE_T)SystemInfo.dwNumberOfProcessors;
    if (CompressContext->MaxThreads < 1) {
        CompressContext->MaxThreads = 1;
    }
    if (CompressContext->MaxThreads > 32) {
        CompressContext->MaxThreads = 32;
    }

    YoriLibInitializeListHead(&CompressContext->PendingList);
    CompressContext->WorkerWaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (CompressContext->WorkerWaitEvent == NULL) {
        return FALSE;
    }

    CompressContext->WorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (CompressContext->WorkerShutdownEvent == NULL) {
        return FALSE;
    }

    CompressContext->Mutex = CreateMutex(NULL, FALSE, NULL);
    if (CompressContext->Mutex == NULL) {
        return FALSE;
    }

    CompressContext->Threads = YoriLibMalloc(sizeof(HANDLE) * CompressContext->MaxThreads);
    if (CompressContext->Threads == NULL) {
        return FALSE;
    }

    return TRUE;
}

/**
 Free the internal allocations and state of a compress context.  This
 also includes waiting for all outstanding compression tasks to complete.
 Note the CompressContext allocation itself is not freed, since this is
 typically on the stack.

 @param CompressContext Pointer to the compress context to clean up.
 */
VOID
YoriLibFreeCompressContext(
    __in PYORILIB_COMPRESS_CONTEXT CompressContext
    )
{
    if (CompressContext->ThreadsAllocated > 0) {
        DWORD Index;
        SetEvent(CompressContext->WorkerShutdownEvent);
        WaitForMultipleObjectsEx(CompressContext->ThreadsAllocated, CompressContext->Threads, TRUE, INFINITE, FALSE);
        for (Index = 0; Index < CompressContext->ThreadsAllocated; Index++) {
            CloseHandle(CompressContext->Threads[Index]);
            CompressContext->Threads[Index] = NULL;
        }
        ASSERT(YoriLibIsListEmpty(&CompressContext->PendingList));
    }
    if (CompressContext->WorkerWaitEvent != NULL) {
        CloseHandle(CompressContext->WorkerWaitEvent);
        CompressContext->WorkerWaitEvent = NULL;
    }
    if (CompressContext->WorkerShutdownEvent != NULL) {
        CloseHandle(CompressContext->WorkerShutdownEvent);
        CompressContext->WorkerShutdownEvent = NULL;
    }
    if (CompressContext->Mutex != NULL) {
        CloseHandle(CompressContext->Mutex);
        CompressContext->Mutex = NULL;
    }
    if (CompressContext->Threads != NULL) {
        YoriLibFree(CompressContext->Threads);
        CompressContext->Threads = NULL;
    }
}

/**
 Compress a single file.  This can be called on worker threads, or occasionally
 on the main thread if the worker threads are backlogged.

 @param PendingAction Pointer to the object that needs to be compressed.
        This structure is deallocated within this function.

 @param CompressionAlgorithm Specifies the compression algorithm to compress
        the file with.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibCompressSingleFile(
    __in PYORILIB_PENDING_ACTION PendingAction,
    __in YORILIB_COMPRESS_ALGORITHM CompressionAlgorithm
    )
{
    HANDLE DestFileHandle;
    DWORD AccessRequired;
    BY_HANDLE_FILE_INFORMATION FileInfo;
    DWORD BytesReturned;
    BOOL Result = FALSE;
    BOOL CompressFile = TRUE;

    //
    //  In order to compress system files, we can't open for write access.
    //  WOF doesn't require write access, although NTFS does.
    //

    AccessRequired = FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE;
    if (CompressionAlgorithm.NtfsAlgorithm != 0) {
        AccessRequired |= FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES;
    }


    DestFileHandle = CreateFile(PendingAction->FileName.StartOfString,
                                AccessRequired,
                                FILE_SHARE_READ|FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);

    if (DestFileHandle == INVALID_HANDLE_VALUE) {
        YoriLibFree(PendingAction);
        return FALSE;
    }

    //
    //  File system compression works by storing the data in fewer allocation
    //  units.  What this means is for files that are very small the
    //  possibility and quantity of allocation units reclaimed can't justify
    //  the overhead, so just skip them.
    //

    if (!GetFileInformationByHandle(DestFileHandle, &FileInfo)) {
        goto Exit;
    }

    if (FileInfo.nFileSizeHigh == 0 &&
        FileInfo.nFileSizeLow < 10 * 1024) {

        goto Exit;
    }


    if (CompressionAlgorithm.NtfsAlgorithm != 0) {
        USHORT Algorithm = (USHORT)CompressionAlgorithm.NtfsAlgorithm;

        Result = DeviceIoControl(DestFileHandle,
                                 FSCTL_SET_COMPRESSION,
                                 &Algorithm,
                                 sizeof(Algorithm),
                                 NULL,
                                 0,
                                 &BytesReturned,
                                 NULL);

    } else {
        struct {
            WOF_EXTERNAL_INFO WofInfo;
            FILE_PROVIDER_EXTERNAL_INFO FileInfo;
        } CompressInfo;

        ZeroMemory(&CompressInfo, sizeof(CompressInfo));

        Result = DeviceIoControl(DestFileHandle,
                                 FSCTL_GET_EXTERNAL_BACKING,
                                 NULL,
                                 0,
                                 &CompressInfo,
                                 sizeof(CompressInfo),
                                 &BytesReturned,
                                 NULL);

        if (Result) {
            if (CompressInfo.WofInfo.Version == 1 &&
                CompressInfo.WofInfo.Provider == WOF_PROVIDER_FILE &&
                CompressInfo.FileInfo.Version == 1 &&
                CompressInfo.FileInfo.Algorithm == CompressionAlgorithm.WofAlgorithm) {

                CompressFile = FALSE;
            }
        }

        if (CompressFile) {
            ZeroMemory(&CompressInfo, sizeof(CompressInfo));
            CompressInfo.WofInfo.Version = 1;
            CompressInfo.WofInfo.Provider = WOF_PROVIDER_FILE;
            CompressInfo.FileInfo.Version = 1;
            CompressInfo.FileInfo.Algorithm = CompressionAlgorithm.WofAlgorithm;

            Result = DeviceIoControl(DestFileHandle,
                                     FSCTL_SET_EXTERNAL_BACKING,
                                     &CompressInfo,
                                     sizeof(CompressInfo),
                                     NULL,
                                     0,
                                     &BytesReturned,
                                     NULL);
        }
    }

Exit:
    if (DestFileHandle != NULL) {
        CloseHandle(DestFileHandle);
    }
    YoriLibFree(PendingAction);
    return Result;
}

/**
 Decompress a single file.  This can be called on worker threads, or
 occasionally on the main thread if the worker threads are backlogged.

 @param PendingAction Pointer to the object that needs to be decompressed.
        This structure is deallocated within this function.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibDecompressSingleFile(
    __in PYORILIB_PENDING_ACTION PendingAction
    )
{
    BOOL GlobalResult = TRUE;
    DWORD BytesReturned;
    DWORD AccessRequired;
    BOOL LocalResult;
    USHORT Algorithm = 0;
    HANDLE DestFileHandle;

    //
    //  Normally WOF will decompress if a file is opened for FILE_WRITE_DATA
    //  and NTFS requires it to attempt decompression
    //

    AccessRequired = FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE | FILE_WRITE_DATA;

    DestFileHandle = CreateFile(PendingAction->FileName.StartOfString,
                                AccessRequired,
                                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);

    if (DestFileHandle == INVALID_HANDLE_VALUE) {

        DWORD Error;

        Error = GetLastError();

        // 
        //  If we can't get a handle with FILE_WRITE_DATA, try without write
        //  access.  This is to allow decompression of system files, which
        //  can be performed with FSCTL_DELETE_EXTERNAL_BACKING .
        //

        if (Error == ERROR_ACCESS_DENIED || Error == ERROR_SHARING_VIOLATION) {

            AccessRequired = FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE;

            DestFileHandle = CreateFile(PendingAction->FileName.StartOfString,
                                        AccessRequired,
                                        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_FLAG_BACKUP_SEMANTICS,
                                        NULL);
        }
    }

    if (DestFileHandle == INVALID_HANDLE_VALUE) {
        YoriLibFree(PendingAction);
        return FALSE;
    }

    if ((AccessRequired & FILE_WRITE_DATA) != 0) {
        LocalResult = DeviceIoControl(DestFileHandle,
                                      FSCTL_SET_COMPRESSION,
                                      &Algorithm,
                                      sizeof(Algorithm),
                                      NULL,
                                      0,
                                      &BytesReturned,
                                      NULL);

        if (!LocalResult) {
            GlobalResult = FALSE;
        }
    } else {

        //
        //  If we can't decompress NTFS due to no access, check whether the
        //  file required decompression.  If it didn't, this can be
        //  treated as success.
        //

        LocalResult = DeviceIoControl(DestFileHandle,
                                      FSCTL_GET_COMPRESSION,
                                      &Algorithm,
                                      sizeof(Algorithm),
                                      NULL,
                                      0,
                                      &BytesReturned,
                                      NULL);

        if (!LocalResult) {
            GlobalResult = FALSE;
        } else if (Algorithm != 0) {
            GlobalResult = FALSE;
        }
    }

    LocalResult = DeviceIoControl(DestFileHandle,
                                  FSCTL_DELETE_EXTERNAL_BACKING,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  &BytesReturned,
                                  NULL);

    if (!LocalResult) {
        GlobalResult = FALSE;
    }

    CloseHandle(DestFileHandle);
    YoriLibFree(PendingAction);
    return GlobalResult;
}


/**
 A background thread which will attempt to compress any items that it finds on
 a list of files requiring compression.

 @param Context Pointer to the compress context.

 @return TRUE to indicate success, FALSE to indicate one or more compression
         operations failed.
 */
DWORD WINAPI
YoriLibCompressWorker(
    __in LPVOID Context
    )
{
    PYORILIB_COMPRESS_CONTEXT CompressContext = (PYORILIB_COMPRESS_CONTEXT)Context;
    DWORD FoundEvent;
    PYORILIB_PENDING_ACTION PendingAction;
    BOOL Result = TRUE;

    while (TRUE) {

        //
        //  Wait for an indication of more work or shutdown.
        //

        FoundEvent = WaitForMultipleObjectsEx(2, &CompressContext->WorkerWaitEvent, FALSE, INFINITE, FALSE);

        //
        //  Process any queued work.
        //

        while (TRUE) {
            WaitForSingleObject(CompressContext->Mutex, INFINITE);
            if (!YoriLibIsListEmpty(&CompressContext->PendingList)) {
                PendingAction = CONTAINING_RECORD(CompressContext->PendingList.Next, YORILIB_PENDING_ACTION, CompressList);
                ASSERT(CompressContext->ItemsQueued > 0);
                CompressContext->ItemsQueued--;
                YoriLibRemoveListItem(&PendingAction->CompressList);
                ReleaseMutex(CompressContext->Mutex);

                if (PendingAction->Compress) {
                    if (!YoriLibCompressSingleFile(PendingAction, CompressContext->CompressionAlgorithm)) {
                        Result = FALSE;
                    }
                } else {
                    if (!YoriLibDecompressSingleFile(PendingAction)) {
                        Result = FALSE;
                    }
                }

            } else {
                ASSERT(CompressContext->ItemsQueued == 0);
                ReleaseMutex(CompressContext->Mutex);
                break;
            }
        }

        //
        //  If shutdown was requested, terminate the thread.
        //

        if (FoundEvent == (WAIT_OBJECT_0 + 1)) {
            break;
        }
    }

    return Result;
}

/**
 Add a pending action to the queue of items to be performed by background
 threads.  If the background threads already have an excessively large
 queue of work, this function returns FALSE to indicate it should be
 completed by the foreground thread.

 @param CompressContext Pointer to the compress context describing the state
        of background threads.

 @param PendingAction Pointer to the action to perform.

 @return TRUE if the action was queued to be processed by background threads,
         or FALSE if it should be completed by the foreground thread.
 */
BOOL
YoriLibAddToBackgroundCompressQueue(
    __in PYORILIB_COMPRESS_CONTEXT CompressContext,
    __in PYORILIB_PENDING_ACTION PendingAction
    )
{
    BOOL Result = FALSE;
    DWORD ThreadId;

    WaitForSingleObject(CompressContext->Mutex, INFINITE);
    if (CompressContext->ThreadsAllocated == 0 ||
        (CompressContext->ItemsQueued > CompressContext->ThreadsAllocated * 2 &&
         CompressContext->ThreadsAllocated < CompressContext->MaxThreads)) {

        CompressContext->Threads[CompressContext->ThreadsAllocated] = CreateThread(NULL, 0, YoriLibCompressWorker, CompressContext, 0, &ThreadId);
        if (CompressContext->Threads[CompressContext->ThreadsAllocated] != NULL) {
            CompressContext->ThreadsAllocated++;
            if (CompressContext->Verbose) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Created compression thread %i\n"), CompressContext->ThreadsAllocated);
            }
        }
    }

    if (CompressContext->ThreadsAllocated > 0 &&
        CompressContext->ItemsQueued < CompressContext->MaxThreads * 2) {

        YoriLibAppendList(&CompressContext->PendingList, &PendingAction->CompressList);
        CompressContext->ItemsQueued++;
        Result = TRUE;
    }

    ReleaseMutex(CompressContext->Mutex);

    SetEvent(CompressContext->WorkerWaitEvent);
    return Result;
}

/**
 Compress a given file with a specified algorithm.  This routine will skip
 small files that do not benefit from compression.

 @param CompressContext Pointer to the compress context specifying where to
        queue compression tasks and which compression algorithm to use.

 @param FileName Pointer to the file name to compress.

 @return TRUE to indicate the file was successfully compressed, FALSE if it
         was not.
 */
BOOL
YoriLibCompressFileInBackground(
    __in PYORILIB_COMPRESS_CONTEXT CompressContext,
    __in PYORI_STRING FileName
    )
{
    PYORILIB_PENDING_ACTION PendingAction;
    BOOL Result = FALSE;

    ASSERT(YoriLibIsStringNullTerminated(FileName));

    PendingAction = YoriLibMalloc(sizeof(YORILIB_PENDING_ACTION) + (FileName->LengthInChars + 1) * sizeof(TCHAR));
    if (PendingAction == NULL) {
        goto Exit;
    }
    PendingAction->Compress = TRUE;
    YoriLibInitEmptyString(&PendingAction->FileName);
    PendingAction->FileName.StartOfString = (LPTSTR)(PendingAction + 1);
    PendingAction->FileName.LengthInChars = FileName->LengthInChars;
    PendingAction->FileName.LengthAllocated = FileName->LengthInChars + 1;
    memcpy(PendingAction->FileName.StartOfString, FileName->StartOfString, (FileName->LengthInChars + 1) * sizeof(TCHAR));

    if (YoriLibAddToBackgroundCompressQueue(CompressContext, PendingAction)) {
        PendingAction = NULL;
    }

    Result = TRUE;

    //
    //  If the threads in the pool are all busy (we have too many items
    //  waiting) do the compression on the main thread.  This is mainly done to
    //  prevent the main thread from continuing to pile in more items that the
    //  pool can't get to.
    //

    if (PendingAction != NULL) {
        if (CompressContext->Verbose) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Compressing %y on main thread for back pressure\n"), FileName);
        }
        if (!YoriLibCompressSingleFile(PendingAction, CompressContext->CompressionAlgorithm)) {
            Result = FALSE;
        }
    }

Exit:
    return Result;
}

/**
 Decompress a given file.

 @param CompressContext Pointer to the compress context specifying where to
        queue compression tasks.

 @param FileName Pointer to the file name to compress.

 @return TRUE to indicate the file was successfully queued for decompression,
         FALSE if it was not.
 */
BOOL
YoriLibDecompressFileInBackground(
    __in PYORILIB_COMPRESS_CONTEXT CompressContext,
    __in PYORI_STRING FileName
    )
{
    PYORILIB_PENDING_ACTION PendingAction;
    BOOL Result = FALSE;

    ASSERT(YoriLibIsStringNullTerminated(FileName));

    PendingAction = YoriLibMalloc(sizeof(YORILIB_PENDING_ACTION) + (FileName->LengthInChars + 1) * sizeof(TCHAR));
    if (PendingAction == NULL) {
        goto Exit;
    }
    PendingAction->Compress = FALSE;
    YoriLibInitEmptyString(&PendingAction->FileName);
    PendingAction->FileName.StartOfString = (LPTSTR)(PendingAction + 1);
    PendingAction->FileName.LengthInChars = FileName->LengthInChars;
    PendingAction->FileName.LengthAllocated = FileName->LengthInChars + 1;
    memcpy(PendingAction->FileName.StartOfString, FileName->StartOfString, (FileName->LengthInChars + 1) * sizeof(TCHAR));

    if (YoriLibAddToBackgroundCompressQueue(CompressContext, PendingAction)) {
        PendingAction = NULL;
    }

    Result = TRUE;

    //
    //  If the threads in the pool are all busy (we have too many items
    //  waiting) do the decompression on the main thread.  This is mainly done
    //  to prevent the main thread from continuing to pile in more items that
    //  the pool can't get to.
    //

    if (PendingAction != NULL) {
        if (CompressContext->Verbose) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Decompressing %y on main thread for back pressure\n"), FileName);
        }
        if (!YoriLibDecompressSingleFile(PendingAction)) {
            Result = FALSE;
        }
    }

Exit:
    return Result;
}

/**
 Return the version of WOF available on a specified path.  This will return
 zero if WOF is not attached to the path, or does not support individual file
 compression.

 @param FileName The file path to check for WOF support.

 @return The version of WOF, in GetVersion form.  Zero if WOF is not
         available.
 */
DWORD
YoriLibGetWofVersionAvailable(
    __in PYORI_STRING FileName
    )
{
    HANDLE FileHandle;
    DWORD AccessRequired;
    DWORD WofVersion;
    DWORD BytesReturned;
    WOF_EXTERNAL_INFO WofInfo;

    ASSERT(YoriLibIsStringNullTerminated(FileName));

    AccessRequired = FILE_READ_ATTRIBUTES | SYNCHRONIZE;

    FileHandle = CreateFile(FileName->StartOfString,
                            AccessRequired,
                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if (FileHandle == INVALID_HANDLE_VALUE) {
        return 0;
    }

    WofInfo.Version = 1;
    WofInfo.Provider = WOF_PROVIDER_FILE;

    WofVersion = 0;
    DeviceIoControl(FileHandle,
                    FSCTL_GET_WOF_VERSION,
                    &WofInfo,
                    sizeof(WofInfo),
                    &WofVersion,
                    sizeof(WofVersion),
                    &BytesReturned,
                    NULL);

    CloseHandle(FileHandle);
    return WofVersion;
}


// vim:sw=4:ts=4:et:
