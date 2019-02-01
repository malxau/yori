/**
 * @file more/ingest.c
 *
 * Yori shell more input strings and record them in memory
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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
    PMORE_PHYSICAL_LINE NewLine;
    DWORD TabCount;
    DWORD CharIndex;
    DWORD DestIndex;
    DWORD TabIndex;
    DWORD BytesRequired;
    PUCHAR Buffer = NULL;
    DWORD BytesRemainingInBuffer = 0;
    DWORD BufferOffset = 0;
    DWORD Alignment;
    WORD PreviousColor;

    YoriLibInitEmptyString(&LineString);

    MoreContext->FilesFound++;
    PreviousColor = MoreContext->InitialColor;

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
            break;
        }

        //
        //  Count the number of tabs.  These are replaced at ingestion time, 
        //  since the width can't change while the program is running and to
        //  save the complexity of accounting for carryover spaces due to tab
        //  expansion at end of logical line
        //

        TabCount = 0;
        for (CharIndex = 0; CharIndex < LineString.LengthInChars; CharIndex++) {
            if (LineString.StartOfString[CharIndex] == '\t') {
                TabCount++;
            }
        }

        //
        //  We need space for the structure, all characters in the source, a NULL,
        //  and since tabs will be replaced with spaces the number of spaces per
        //  tab minus one (for the tab character being removed.)
        //

        BytesRequired = sizeof(MORE_PHYSICAL_LINE) + (LineString.LengthInChars + TabCount * (MoreContext->TabWidth - 1) + 1) * sizeof(TCHAR);

        //
        //  If we need a buffer, allocate a buffer that typically has space for
        //  multiple lines
        //

        if (Buffer == NULL || BytesRequired > BytesRemainingInBuffer) {
            if (Buffer != NULL) {
                YoriLibDereference(Buffer);
            }
            BytesRemainingInBuffer = 64 * 1024;
            if (BytesRequired > BytesRemainingInBuffer) {
                BytesRemainingInBuffer = BytesRequired;
            }
            BufferOffset = 0;

            //
            //  MSFIX What to do on out of memory?  This tool can actually run
            //  out
            //

            Buffer = YoriLibReferencedMalloc(BytesRemainingInBuffer);
            if (Buffer == NULL) {
                break;
            }
        }

        //
        //  Write this line into the current buffer
        //

        NewLine = (PMORE_PHYSICAL_LINE)YoriLibAddToPointer(Buffer, BufferOffset);

        YoriLibReference(Buffer);
        NewLine->MemoryToFree = Buffer;
        NewLine->InitialColor = PreviousColor;
        NewLine->LineNumber = MoreContext->LineCount + 1;
        YoriLibReference(Buffer);
        NewLine->LineContents.MemoryToFree = Buffer;
        NewLine->LineContents.StartOfString = (LPTSTR)(NewLine + 1);

        for (CharIndex = 0, DestIndex = 0; CharIndex < LineString.LengthInChars; CharIndex++) {
            //
            //  If the string is <ESC>[, then treat it as an escape sequence.
            //  Look for the final letter after any numbers or semicolon.
            //
    
            if (LineString.LengthInChars > CharIndex + 2 &&
                LineString.StartOfString[CharIndex] == 27 &&
                LineString.StartOfString[CharIndex + 1] == '[') {
    
                YORI_STRING EscapeSubset;
                DWORD EndOfEscape;
    
                YoriLibInitEmptyString(&EscapeSubset);
                EscapeSubset.StartOfString = &LineString.StartOfString[CharIndex + 2];
                EscapeSubset.LengthInChars = LineString.LengthInChars - CharIndex - 2;
                EndOfEscape = YoriLibCountStringContainingChars(&EscapeSubset, _T("0123456789;"));
    
                //
                //  Count everything as consuming the source and needing buffer
                //  space in the destination but consuming no display cells.  This
                //  may include the final letter, if we found one.
                //
    
                if (LineString.LengthInChars > CharIndex + 2 + EndOfEscape) {
                    EscapeSubset.StartOfString -= 2;
                    EscapeSubset.LengthInChars = EndOfEscape + 3;
                    YoriLibVtFinalColorFromSequence(PreviousColor, &EscapeSubset, &PreviousColor);
                }
            }
            if (LineString.StartOfString[CharIndex] == '\t') {
                for (TabIndex = 0; TabIndex < MoreContext->TabWidth; TabIndex++) {
                    NewLine->LineContents.StartOfString[DestIndex] = ' ';
                    DestIndex++;
                }
            } else {
                NewLine->LineContents.StartOfString[DestIndex] = LineString.StartOfString[CharIndex];
                DestIndex++;
            }
        }
        NewLine->LineContents.StartOfString[DestIndex] = '\0';
        NewLine->LineContents.LengthInChars = DestIndex;
        NewLine->LineContents.LengthAllocated = DestIndex + 1;

        BufferOffset += BytesRequired;
        BytesRemainingInBuffer -= BytesRequired;

        //
        //  Align the buffer to 8 bytes.  There's no length checking because
        //  the allocation is assumed to be aligned to 8 bytes.
        //

        Alignment = BufferOffset % 8;
        if (Alignment > 0) {
            Alignment = 8 - Alignment;
            BufferOffset += Alignment;
            BytesRemainingInBuffer -= Alignment;
        }

        //
        //  Insert the new line into the list
        //

        WaitForSingleObject(MoreContext->PhysicalLineMutex, INFINITE);
        MoreContext->LineCount++;
        YoriLibAppendList(&MoreContext->PhysicalLineList, &NewLine->LineList);
        ReleaseMutex(MoreContext->PhysicalLineMutex);

        SetEvent(MoreContext->PhysicalLineAvailableEvent);

        if (WaitForSingleObject(MoreContext->ShutdownEvent, 0) == WAIT_OBJECT_0) {
            break;
        }
    }

    if (Buffer != NULL) {
        YoriLibDereference(Buffer);
    }

    YoriLibLineReadClose(LineContext);
    YoriLibFreeStringContents(&LineString);

    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param MoreContext Pointer to the more context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
MoreFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PMORE_CONTEXT MoreContext
    )
{
    HANDLE FileHandle;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    if (WaitForSingleObject(MoreContext->ShutdownEvent, 0) == WAIT_OBJECT_0) {
        return FALSE;
    }

    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        FileHandle = CreateFile(FilePath->StartOfString,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

        if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("more: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        MoreProcessStream(FileHandle, MoreContext);

        CloseHandle(FileHandle);
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
        DWORD FileMore = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
        FileMore = FileMore & ~(FILE_TYPE_REMOTE);
        if (FileMore == FILE_TYPE_CHAR) {
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
    
            YoriLibForEachFile(&MoreContext->InputSources[i], MatchFlags, 0, MoreFileFoundCallback, NULL, MoreContext);
        }
    }

    if (MoreContext->FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("more: no matching files found\n"));
        return 0;
    }

    return 0;
}

// vim:sw=4:ts=4:et:
