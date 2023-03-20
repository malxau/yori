/**
 * @file more/ingest.c
 *
 * Yori shell more input strings and record them in memory
 *
 * Copyright (c) 2017-2021 Malcolm J. Smith
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

#include "more.h"

/**
 A context structure to allow a new physical line to share an allocation with
 previous physical lines.  Each added line can consume from and populate this
 allocation.
 */
typedef struct _MORE_LINE_ALLOC_CONTEXT {

    /**
     A pointer to a block of memory to allocate from.
     */
    PUCHAR Buffer;

    /**
     The currently used number of bytes in the buffer above.
     */
    DWORD BufferOffset;

    /**
     The number of bytes remaining in the buffer above.
     */
    DWORD BytesRemainingInBuffer;

    /**
     The color that the next physical line should start with.  This is
     updated to refer to the final color at the end of each line.
     */
    WORD PreviousColor;
} MORE_LINE_ALLOC_CONTEXT, *PMORE_LINE_ALLOC_CONTEXT;

/**
 Add a new physical line to the allocation.

 @param MoreContext Pointer to the context describing process behavior.

 @param LineString Pointer to a string of text containing the physical line
        to add.

 @param AllocContext Pointer to the allocation context describing the buffer
        that can be used for a new physical line.  This may be reallocated
        within this routine.

 @return TRUE to indicate success, FALSE to indicate failure.  Failure
         implies allocation failure, suggesting execution cannot continue.
 */
BOOL
MoreAddPhysicalLineToBuffer(
    __in PMORE_CONTEXT MoreContext,
    __in PYORI_STRING LineString,
    __in PMORE_LINE_ALLOC_CONTEXT AllocContext
    )
{
    PMORE_PHYSICAL_LINE NewLine;
    DWORD TabCount;
    DWORD CharIndex;
    DWORD DestIndex;
    DWORD TabIndex;
    DWORD Alignment;
    DWORD BytesRequired;

    //
    //  Count the number of tabs.  These are replaced at ingestion time, 
    //  since the width can't change while the program is running and to
    //  save the complexity of accounting for carryover spaces due to tab
    //  expansion at end of logical line
    //

    TabCount = 0;
    for (CharIndex = 0; CharIndex < LineString->LengthInChars; CharIndex++) {
        if (LineString->StartOfString[CharIndex] == '\t') {
            TabCount++;
        }
    }

    //
    //  We need space for the structure, all characters in the source, a NULL,
    //  and since tabs will be replaced with spaces the number of spaces per
    //  tab minus one (for the tab character being removed.)
    //

    BytesRequired = sizeof(MORE_PHYSICAL_LINE) + (LineString->LengthInChars + TabCount * (MoreContext->TabWidth - 1) + 1) * sizeof(TCHAR);

    //
    //  If we need a buffer, allocate a buffer that typically has space for
    //  multiple lines
    //

    if (AllocContext->Buffer == NULL ||
        BytesRequired > AllocContext->BytesRemainingInBuffer) {
        if (AllocContext->Buffer != NULL) {
            YoriLibDereference(AllocContext->Buffer);
        }
        AllocContext->BytesRemainingInBuffer = 64 * 1024;
        if (BytesRequired > AllocContext->BytesRemainingInBuffer) {
            AllocContext->BytesRemainingInBuffer = BytesRequired;
        }
        AllocContext->BufferOffset = 0;

        AllocContext->Buffer = YoriLibReferencedMalloc(AllocContext->BytesRemainingInBuffer);
        if (AllocContext->Buffer == NULL) {
            MoreContext->OutOfMemory = TRUE;
            return FALSE;
        }
    }

    //
    //  Write this line into the current buffer
    //

    NewLine = (PMORE_PHYSICAL_LINE)YoriLibAddToPointer(AllocContext->Buffer, AllocContext->BufferOffset);

    YoriLibReference(AllocContext->Buffer);
    NewLine->FilteredLineList.Next = NULL;
    NewLine->FilteredLineList.Prev = NULL;
    NewLine->MemoryToFree = AllocContext->Buffer;
    NewLine->InitialColor = AllocContext->PreviousColor;
    NewLine->LineNumber = MoreContext->LineCount + 1;
    NewLine->FilteredLineNumber = NewLine->LineNumber;
    YoriLibReference(AllocContext->Buffer);
    NewLine->LineContents.MemoryToFree = AllocContext->Buffer;
    NewLine->LineContents.StartOfString = (LPTSTR)(NewLine + 1);

    for (CharIndex = 0, DestIndex = 0; CharIndex < LineString->LengthInChars; CharIndex++) {
        //
        //  If the string is <ESC>[, then treat it as an escape sequence.
        //  Look for the final letter after any numbers or semicolon.
        //

        if (LineString->LengthInChars > CharIndex + 2 &&
            LineString->StartOfString[CharIndex] == 27 &&
            LineString->StartOfString[CharIndex + 1] == '[') {

            YORI_STRING EscapeSubset;
            DWORD EndOfEscape;

            YoriLibInitEmptyString(&EscapeSubset);
            EscapeSubset.StartOfString = &LineString->StartOfString[CharIndex + 2];
            EscapeSubset.LengthInChars = LineString->LengthInChars - CharIndex - 2;
            EndOfEscape = YoriLibCountStringContainingChars(&EscapeSubset, _T("0123456789;"));

            //
            //  Look for color changes so any later line can be marked as
            //  starting with this color.
            //

            if (LineString->LengthInChars > CharIndex + 2 + EndOfEscape) {
                EscapeSubset.StartOfString -= 2;
                EscapeSubset.LengthInChars = EndOfEscape + 3;
                YoriLibVtFinalColorFromSequence(AllocContext->PreviousColor, &EscapeSubset, &AllocContext->PreviousColor);
            }
        }
        if (LineString->StartOfString[CharIndex] == '\t') {
            for (TabIndex = 0; TabIndex < MoreContext->TabWidth; TabIndex++) {
                NewLine->LineContents.StartOfString[DestIndex] = ' ';
                DestIndex++;
            }
        } else {
            NewLine->LineContents.StartOfString[DestIndex] = LineString->StartOfString[CharIndex];
            DestIndex++;
        }
    }
    NewLine->LineContents.StartOfString[DestIndex] = '\0';
    NewLine->LineContents.LengthInChars = DestIndex;
    NewLine->LineContents.LengthAllocated = DestIndex + 1;

    AllocContext->BufferOffset += BytesRequired;
    AllocContext->BytesRemainingInBuffer -= BytesRequired;

    //
    //  Align the buffer to 8 bytes.  There's no length checking because
    //  the allocation is assumed to be aligned to 8 bytes.
    //

    Alignment = AllocContext->BufferOffset % 8;
    if (Alignment > 0) {
        Alignment = 8 - Alignment;
        AllocContext->BufferOffset += Alignment;
        AllocContext->BytesRemainingInBuffer -= Alignment;
    }

    //
    //  Insert the new line into the list
    //

    WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);
    MoreContext->LineCount++;
    YoriLibAppendList(&MoreContext->PhysicalLineList, &NewLine->LineList);
    if (!MoreContext->FilterToSearch ||
        MoreFindNextSearchMatch(MoreContext, &NewLine->LineContents, NULL, NULL)) {

        YoriLibAppendList(&MoreContext->FilteredPhysicalLineList, &NewLine->FilteredLineList);
        MoreContext->FilteredLineCount++;
        NewLine->FilteredLineNumber = MoreContext->FilteredLineCount;
    }
    ReleaseMutex(MoreContext->PhysicalLineMutex);

    SetEvent(MoreContext->PhysicalLineAvailableEvent);

    return TRUE;
}

/**
 Process a single opened stream, enumerating through all lines and displaying
 the set requested by the user.

 @param hSource The opened source stream.

 @param MoreContext Pointer to context information specifying which lines to
        display.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MoreProcessStream(
    __in HANDLE hSource,
    __in PMORE_CONTEXT MoreContext
    )
{
    PVOID LineContext = NULL;
    YORI_STRING LineString;
    YORI_LIB_LINE_ENDING LineEnding;
    BOOL TimeoutReached;
    MORE_LINE_ALLOC_CONTEXT AllocContext;
    BOOLEAN Terminate;

    YoriLibInitEmptyString(&LineString);

    MoreContext->FilesFound++;

    Terminate = FALSE;
    AllocContext.Buffer = NULL;
    AllocContext.BytesRemainingInBuffer = 0;
    AllocContext.BufferOffset = 0;
    AllocContext.PreviousColor = MoreContext->InitialColor;

    while (TRUE) {

        if (!YoriLibReadLineToStringEx(&LineString, &LineContext, !MoreContext->WaitForMore, INFINITE, hSource, &LineEnding, &TimeoutReached)) {
            break;
        }

        if (!MoreAddPhysicalLineToBuffer(MoreContext, &LineString, &AllocContext)) {
            Terminate = TRUE;
            break;
        }

        if (WaitForSingleObject(MoreContext->ShutdownEvent, 0) == WAIT_OBJECT_0) {
            Terminate = TRUE;
            break;
        }

    }

    //
    //  If waiting for more, try to read another line.  If there's not
    //  enough data for an entire line, sleep for a bit and try again. If
    //  there is enough for another line, add it.
    //

    if (MoreContext->WaitForMore && !Terminate) {
        while (TRUE) {
            if (!YoriLibReadLineToStringEx(&LineString, &LineContext, FALSE, INFINITE, hSource, &LineEnding, &TimeoutReached)) {
                if (WaitForSingleObject(MoreContext->ShutdownEvent, 0) == WAIT_OBJECT_0) {
                    Terminate = TRUE;
                    break;
                }

                Sleep(200);
                continue;
            }

            if (!MoreAddPhysicalLineToBuffer(MoreContext, &LineString, &AllocContext)) {
                Terminate = TRUE;
                break;
            }
    
            if (WaitForSingleObject(MoreContext->ShutdownEvent, 0) == WAIT_OBJECT_0) {
                Terminate = TRUE;
                break;
            }
        }
    }

    if (AllocContext.Buffer != NULL) {
        YoriLibDereference(AllocContext.Buffer);
    }

    YoriLibLineReadCloseOrCache(LineContext);
    YoriLibFreeStringContents(&LineString);

    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param Context Pointer to the more context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
MoreFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    HANDLE FileHandle;
    PMORE_CONTEXT MoreContext = (PMORE_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    if (WaitForSingleObject(MoreContext->ShutdownEvent, 0) == WAIT_OBJECT_0) {
        return FALSE;
    }

    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        UCHAR LeadingBytes[3];
        DWORD SavedEncoding;
        DWORD BytesRead;

        FileHandle = CreateFile(FilePath->StartOfString,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);

        if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("more: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        SavedEncoding = YoriLibGetMultibyteInputEncoding();

        //
        //  If the file starts with a UTF-16 BOM, interpret it as UTF-16.
        //

        if (ReadFile(FileHandle, LeadingBytes, sizeof(LeadingBytes), &BytesRead, NULL)) {
            if (BytesRead >= 2 &&
                LeadingBytes[0] == 0xFF &&
                LeadingBytes[1] == 0xFE) {

                YoriLibSetMultibyteInputEncoding(CP_UTF16);
            }
        }
        SetFilePointer(FileHandle, 0, NULL, FILE_BEGIN);

        MoreProcessStream(FileHandle, MoreContext);

        YoriLibSetMultibyteInputEncoding(SavedEncoding);

        CloseHandle(FileHandle);
    }

    if (MoreContext->OutOfMemory) {
        return FALSE;
    }
    return TRUE;
}

/**
 A background thread that is tasked with collecting any input lines and adding
 them into the structure of lines, and signalling the foreground UI thread to
 indicate when it has done so.

 @param Context Pointer to the MORE_CONTEXT.

 @return DWORD, ignored.
 */
DWORD WINAPI
MoreIngestThread(
    __in LPVOID Context
    )
{
    PMORE_CONTEXT MoreContext = (PMORE_CONTEXT)Context;
    DWORD MatchFlags;
    DWORD i;

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (MoreContext->InputSourceCount == 0) {
        if (YoriLibIsStdInConsole()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
            return 0;
        }

        MoreProcessStream(GetStdHandle(STD_INPUT_HANDLE), MoreContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (MoreContext->Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (MoreContext->BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }

        for (i = 0; i < MoreContext->InputSourceCount; i++) {

            YoriLibForEachStream(&MoreContext->InputSources[i], MatchFlags, 0, MoreFileFoundCallback, NULL, MoreContext);
        }
    }

    if (MoreContext->FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("more: no matching files found\n"));
        return 0;
    }

    return 0;
}

// vim:sw=4:ts=4:et:
