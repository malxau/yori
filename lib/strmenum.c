/**
 * @file lib/strmenum.c
 *
 * Yori file stream enumeration routines
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Context information passed between the file enumerator and each file found.
 */
typedef struct _YORI_LIB_STREAM_ENUM_CONTEXT {

    /**
     The callback to invoke for each matching stream.
     */
    PYORILIB_FILE_ENUM_FN UserCallback;

    /**
     The callback to invoke on enumerate error.
     */
    PYORILIB_FILE_ENUM_ERROR_FN UserErrorCallback;

    /**
     The context to pass to the stream match callbacks.
     */
    PVOID UserContext;

    /**
     The stream name from the initial enumeration criteria.
     */
    YORI_STRING TrailingStreamName;

    /**
     A temporary buffer used for each stream found out of a single
     allocation.
     */
    YORI_STRING FullPathWithStream;

    /**
     TRUE if TrailingStreamName contains wildcards which implies that stream
     enumeration is needed to check for matches.  If FALSE, the stream is
     fully specified so each file only needs to check for its existence.
     */
    BOOLEAN StreamNameHasWild;
} YORI_LIB_STREAM_ENUM_CONTEXT, *PYORI_LIB_STREAM_ENUM_CONTEXT;

/**
 A callback that is invoked when a file is found that matches the search
 criteria specified for the file component.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth.

 @param Context Pointer to the stream enumeration context structure indicating
        the streams to look for and callback to invoke on any match.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
YoriLibStreamEnumFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PYORI_LIB_STREAM_ENUM_CONTEXT StreamContext = (PYORI_LIB_STREAM_ENUM_CONTEXT)Context;
    WIN32_FIND_DATA BogusFileInfo;
    DWORD LengthNeeded;
    BOOL Result;

    //
    //  Make sure we have enough to hold the file name with the stream name.
    //  We must have two extra chars, one for a colon and one for a NULL.
    //  When doing stream enumerate, it's convenient to ensure we have enough
    //  for any conceivable stream; otherwise, we know the stream name length.
    //  Try to allocate a little more to avoid repeat allocations on later
    //  file matches.
    //

    if (StreamContext->StreamNameHasWild) {
        LengthNeeded = FilePath->LengthInChars + 1 + YORI_LIB_MAX_STREAM_NAME + 1;
    } else {
        LengthNeeded = FilePath->LengthInChars + 1 + StreamContext->TrailingStreamName.LengthInChars + 1;
    }
    if (StreamContext->FullPathWithStream.LengthAllocated < LengthNeeded) {
        YoriLibFreeStringContents(&StreamContext->FullPathWithStream);
        if (!YoriLibIsSizeAllocatable(LengthNeeded + 200)) {
            return FALSE;
        }
        if (!YoriLibAllocateString(&StreamContext->FullPathWithStream, (YORI_ALLOC_SIZE_T)(LengthNeeded + 200))) {
            return FALSE;
        }
    }

    if (!StreamContext->StreamNameHasWild) {

        //
        //  Build the full path to the stream
        //

        StreamContext->FullPathWithStream.LengthInChars =
            YoriLibSPrintf(StreamContext->FullPathWithStream.StartOfString,
                           _T("%y:%y"),
                           FilePath,
                           &StreamContext->TrailingStreamName);

        //
        //  Check if it exists, and if so, call the user's callback
        //

        if (GetFileAttributes(StreamContext->FullPathWithStream.StartOfString) != 0xFFFFFFFF) {
            //
            //  Assume file state is stream state
            //

            memcpy(&BogusFileInfo, FileInfo, FIELD_OFFSET(WIN32_FIND_DATA, cFileName));
            BogusFileInfo.cAlternateFileName[0] = '\0';

            //
            //  Populate stream name
            //

            YoriLibSPrintfS(BogusFileInfo.cFileName,
                            sizeof(BogusFileInfo.cFileName)/sizeof(BogusFileInfo.cFileName[0]),
                            _T("%s:%y"),
                            FileInfo->cFileName,
                            &StreamContext->TrailingStreamName);

            //
            //  Update stream information
            //

            YoriLibUpdateFindDataFromFileInformation(&BogusFileInfo, StreamContext->FullPathWithStream.StartOfString, FALSE);


            Result = StreamContext->UserCallback(&StreamContext->FullPathWithStream,
                                                 &BogusFileInfo,
                                                 Depth,
                                                 StreamContext->UserContext);

            return Result;
        }
    } else {
        HANDLE hFind;
        WIN32_FIND_STREAM_DATA FindStreamData;
        YORI_STRING FoundStreamName;

        hFind = DllKernel32.pFindFirstStreamW(FilePath->StartOfString,
                                              0,
                                              &FindStreamData,
                                              0);
        if (hFind != INVALID_HANDLE_VALUE) {
            YoriLibInitEmptyString(&FoundStreamName);
            do {
                FoundStreamName.StartOfString = FindStreamData.cStreamName;
                FoundStreamName.LengthInChars = (YORI_ALLOC_SIZE_T)_tcslen(FindStreamData.cStreamName);

                //
                //  Truncate any trailing :$DATA attribute name
                //

                if (FoundStreamName.LengthInChars > 6 && _tcscmp(FindStreamData.cStreamName + FoundStreamName.LengthInChars - 6, L":$DATA") == 0) {
                    FoundStreamName.LengthInChars = FoundStreamName.LengthInChars - 6;
                }

                FoundStreamName.StartOfString[FoundStreamName.LengthInChars] = '\0';

                //
                //  Remove any leading colon
                //

                if (FoundStreamName.LengthInChars >= 1 && FoundStreamName.StartOfString[0] == ':') {
                    FoundStreamName.StartOfString++;
                    FoundStreamName.LengthInChars--;
                }

                //
                //  Check if it matches the specified criteria
                //

                if (YoriLibDoesFileMatchExpression(&FoundStreamName, &StreamContext->TrailingStreamName)) {

                    if (FoundStreamName.LengthInChars == 0) {
                        Result = StreamContext->UserCallback(FilePath,
                                                             FileInfo,
                                                             Depth,
                                                             StreamContext->UserContext);

                        if (Result == FALSE) {
                            FindClose(hFind);
                            return Result;
                        }
                    } else {

                        //
                        //  Generate a full path to the stream
                        //

                        StreamContext->FullPathWithStream.LengthInChars =
                            YoriLibSPrintfS(StreamContext->FullPathWithStream.StartOfString,
                                            StreamContext->FullPathWithStream.LengthAllocated,
                                            _T("%y:%y"),
                                            FilePath,
                                            &FoundStreamName);

                        //
                        //  Assume file state is stream state
                        //

                        memcpy(&BogusFileInfo, FileInfo, FIELD_OFFSET(WIN32_FIND_DATA, cFileName));
                        BogusFileInfo.cAlternateFileName[0] = '\0';

                        //
                        //  Populate stream name
                        //

                        YoriLibSPrintfS(BogusFileInfo.cFileName,
                                        sizeof(BogusFileInfo.cFileName)/sizeof(BogusFileInfo.cFileName[0]),
                                        _T("%s:%y"),
                                        FileInfo->cFileName,
                                        &FoundStreamName);

                        //
                        //  Update stream information
                        //

                        YoriLibUpdateFindDataFromFileInformation(&BogusFileInfo, StreamContext->FullPathWithStream.StartOfString, FALSE);

                        Result = StreamContext->UserCallback(&StreamContext->FullPathWithStream,
                                                             &BogusFileInfo,
                                                             Depth,
                                                             StreamContext->UserContext);

                        if (Result == FALSE) {
                            FindClose(hFind);
                            return Result;
                        }
                    }
                }
            } while(DllKernel32.pFindNextStreamW(hFind, &FindStreamData));

            FindClose(hFind);
        }
    }

    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the stream enumeration context, indicating which
        callback and context to invoke on error.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
YoriLibStreamEnumErrorCallback(
    __in PYORI_STRING FilePath,
    __in SYSERR ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PYORI_LIB_STREAM_ENUM_CONTEXT StreamContext = (PYORI_LIB_STREAM_ENUM_CONTEXT)Context;

    if (StreamContext->UserErrorCallback != NULL) {
        return StreamContext->UserErrorCallback(FilePath, ErrorCode, Depth, StreamContext->UserContext);
    }

    return TRUE;
}


/**
 Enumerate the set of possible streams matching a user specified pattern.
 This function is responsible for initiating enumeration of files, and then
 applying a filter for matching streams, and invoking a caller specified
 callback for matching streams.

 @param FileSpec The user provided file specification to enumerate matches on.

 @param MatchFlags Specifies the behavior of the match, including whether
        it should be applied recursively and the recursing behavior.

 @param Depth Indicates the current recursion depth.  If this function is
        reentered, this value is incremented.

 @param Callback The callback to invoke on each match.

 @param ErrorCallback Optionally points to a function to invoke if a
        directory cannot be enumerated.  If NULL, the caller does not care
        about failures and wants to silently continue.

 @param Context Caller provided context to pass to the callback.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibForEachStream(
    __in PYORI_STRING FileSpec,
    __in WORD MatchFlags,
    __in DWORD Depth,
    __in PYORILIB_FILE_ENUM_FN Callback,
    __in_opt PYORILIB_FILE_ENUM_ERROR_FN ErrorCallback,
    __in PVOID Context
    )
{
    YORI_STRING FilePart;
    YORI_STRING FileSpecNoStream;
    YORI_ALLOC_SIZE_T Index;
    BOOL Result;
    YORI_LIB_STREAM_ENUM_CONTEXT StreamContext;

    //
    //  Look backwards through the FileSpec to see if there's a seperator.
    //  Any stream specification is only valid after that point.
    //

    YoriLibInitEmptyString(&FilePart);
    for (Index = FileSpec->LengthInChars; Index > 0; Index--) {
        if (YoriLibIsSep(FileSpec->StartOfString[Index - 1])) {
            FilePart.StartOfString = &FileSpec->StartOfString[Index];
            FilePart.LengthInChars = FileSpec->LengthInChars - Index;
            break;
        }
    }

    if (FilePart.StartOfString == NULL) {
        FilePart.StartOfString = FileSpec->StartOfString;
        FilePart.LengthInChars = FileSpec->LengthInChars;
    }

    //
    //  Now look through the file component for a stream seperator.
    //

    StreamContext.StreamNameHasWild = FALSE;
    YoriLibInitEmptyString(&StreamContext.TrailingStreamName);
    for (Index = FilePart.LengthInChars; Index > 0; Index--) {
        if (FilePart.StartOfString[Index - 1] == ':') {

            // 
            //  If the colon is the second character, and the first char is
            //  A through Z, assume this is a drive specification, not a
            //  stream specification.  "C:Foo" is ambiguous, but Windows will
            //  treat it as a drive, so we do too.
            //

            if (Index == 2) {
                TCHAR UpcasedFirstChar;
                UpcasedFirstChar = YoriLibUpcaseChar(FilePart.StartOfString[0]);
                if (UpcasedFirstChar >= 'A' && UpcasedFirstChar <= 'Z') {
                    break;
                }
            }
            StreamContext.TrailingStreamName.StartOfString = &FilePart.StartOfString[Index];
            StreamContext.TrailingStreamName.LengthInChars = FilePart.LengthInChars - Index;
            break;
        } else if (FilePart.StartOfString[Index - 1] == '*' || FilePart.StartOfString[Index - 1] == '?') {
            StreamContext.StreamNameHasWild = TRUE;
        }
    }

    //
    //  If there is no stream specification, we only need to consider any
    //  file matches, so pass this to the file enumerator.
    //

    if (StreamContext.TrailingStreamName.LengthInChars == 0) {
        return YoriLibForEachFile(FileSpec, MatchFlags, Depth, Callback, ErrorCallback, Context);
    }

    YoriLibInitEmptyString(&FileSpecNoStream);
    FileSpecNoStream.StartOfString = FileSpec->StartOfString;
    FileSpecNoStream.LengthInChars = FileSpec->LengthInChars - StreamContext.TrailingStreamName.LengthInChars - 1;

    FileSpecNoStream.MemoryToFree = YoriLibCStringFromYoriString(&FileSpecNoStream);
    if (FileSpecNoStream.MemoryToFree == NULL) {
        return FALSE;
    }
    FileSpecNoStream.LengthAllocated = FileSpecNoStream.LengthInChars + 1;
    FileSpecNoStream.StartOfString = FileSpecNoStream.MemoryToFree;

    StreamContext.UserCallback = Callback;
    StreamContext.UserErrorCallback = ErrorCallback;
    StreamContext.UserContext = Context;
    YoriLibInitEmptyString(&StreamContext.FullPathWithStream);

    //
    //  If the system doesn't support it, don't try to enumerate streams.
    //  This limits the allowable expressions but fully specified stream
    //  names will still work.
    //

    if (DllKernel32.pFindFirstStreamW == NULL) {
        StreamContext.StreamNameHasWild = FALSE;
    }

    Result = YoriLibForEachFile(&FileSpecNoStream,
                                MatchFlags,
                                Depth,
                                YoriLibStreamEnumFileFoundCallback,
                                YoriLibStreamEnumErrorCallback,
                                &StreamContext);

    YoriLibFreeStringContents(&StreamContext.FullPathWithStream);
    YoriLibFreeStringContents(&FileSpecNoStream);
    return Result;
}

// vim:sw=4:ts=4:et:
