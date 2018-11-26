/**
 * @file hexdump/hexdump.c
 *
 * Yori shell display the final lines in a file
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

#include <yoripch.h>
#include <yorilib.h>

#pragma warning(disable: 4220) // Varargs matches remaining parameters

/**
 Help text to display to the user.
 */
const
CHAR strHexDumpHelpText[] =
        "\n"
        "Output the contents of one or more files in hex.\n"
        "\n"
        "HEXDUMP [-license] [-b] [-d] [-g1|-g2|-g4|-g8] [-hc] [-ho]\n"
        "        [-l length] [-o offset] [-s] [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -d             Display the differences between two files\n"
        "   -g             Number of bytes per display group\n"
        "   -hc            Hide character display\n"
        "   -ho            Hide offset within buffer\n"
        "   -l             Length of the section to display\n"
        "   -o             Offset within the stream to display\n"
        "   -s             Process files from all subdirectories\n";

/**
 Display usage text to the user.
 */
BOOL
HexDumpHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("HexDump %i.%i\n"), HEXDUMP_VER_MAJOR, HEXDUMP_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHexDumpHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _HEXDUMP_CONTEXT {

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     Offset within each stream to display.
     */
    LONGLONG OffsetToDisplay;

    /**
     Length within each stream to display.
     */
    LONGLONG LengthToDisplay;

    /**
     Number of bytes to display per group.
     */
    DWORD BytesPerGroup;

    /**
     If TRUE, hide the offset display within the buffer.
     */
    BOOL HideOffset;

    /**
     If TRUE, hide the character display within the buffer.
     */
    BOOL HideCharacters;

} HEXDUMP_CONTEXT, *PHEXDUMP_CONTEXT;

/**
 Process a single opened stream, enumerating through all lines and displaying
 the set requested by the user.

 @param hSource The opened source stream.

 @param HexDumpContext Pointer to context information specifying which lines to
        display.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
HexDumpProcessStream(
    __in HANDLE hSource,
    __in PHEXDUMP_CONTEXT HexDumpContext
    )
{
    PUCHAR Buffer;
    DWORD BufferSize;
    DWORD BufferOffset;
    DWORD BytesReturned;
    DWORD LengthToDisplay;
    DWORD DisplayFlags;
    LARGE_INTEGER StreamOffset;

    HexDumpContext->FilesFound++;

    BufferSize = 64 * 1024;
    Buffer = YoriLibMalloc(BufferSize);
    if (Buffer == NULL) {
        return FALSE;
    }
    DisplayFlags = 0;
    if (!HexDumpContext->HideOffset) {
        DisplayFlags |= YORI_LIB_HEX_FLAG_DISPLAY_LARGE_OFFSET;
    }
    if (!HexDumpContext->HideCharacters) {
        DisplayFlags |= YORI_LIB_HEX_FLAG_DISPLAY_CHARS;
    }


    //
    //  If it's a file, start at the offset requested by the user.  If it's
    //  not a file (it's a pipe), the only way to move forward is by
    //  reading.
    //

    StreamOffset.QuadPart = HexDumpContext->OffsetToDisplay;

    if (!SetFilePointer(hSource, StreamOffset.LowPart, &StreamOffset.HighPart, FILE_BEGIN)) {
        StreamOffset.QuadPart = 0;
    }

    while (TRUE) {
        BytesReturned = 0;
        if (!ReadFile(hSource, Buffer, BufferSize, &BytesReturned, NULL)) {
            break;
        }

        if (BytesReturned == 0) {
            break;
        }

        if (StreamOffset.QuadPart + BytesReturned <= HexDumpContext->OffsetToDisplay) {
            StreamOffset.QuadPart += BytesReturned;
            continue;
        }

        LengthToDisplay = BytesReturned;

        BufferOffset = 0;
        if (StreamOffset.QuadPart < HexDumpContext->OffsetToDisplay) {
            BufferOffset = (DWORD)(HexDumpContext->OffsetToDisplay - StreamOffset.QuadPart);
            LengthToDisplay -= BufferOffset;
        }

        ASSERT(BufferOffset + LengthToDisplay == BytesReturned);

        if (HexDumpContext->LengthToDisplay != 0) {
            if (StreamOffset.QuadPart + BufferOffset + LengthToDisplay > HexDumpContext->OffsetToDisplay + HexDumpContext->LengthToDisplay) {
                LengthToDisplay = (DWORD)(HexDumpContext->OffsetToDisplay + HexDumpContext->LengthToDisplay - StreamOffset.QuadPart - BufferOffset);
            }
        }

        if (!YoriLibHexDump(&Buffer[BufferOffset], StreamOffset.QuadPart + BufferOffset, LengthToDisplay, HexDumpContext->BytesPerGroup, DisplayFlags)) {
            break;
        }

        StreamOffset.QuadPart += BytesReturned;
        if (HexDumpContext->LengthToDisplay != 0 && StreamOffset.QuadPart >= HexDumpContext->OffsetToDisplay + HexDumpContext->LengthToDisplay) {
            break;
        }
    }

    YoriLibFree(Buffer);

    return TRUE;
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param HexDumpContext Pointer to the hexdump context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
HexDumpFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PHEXDUMP_CONTEXT HexDumpContext
    )
{
    HANDLE FileHandle;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

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
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hexdump: open of %y failed: %s"), FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return TRUE;
        }

        HexDumpProcessStream(FileHandle, HexDumpContext);

        CloseHandle(FileHandle);
    }

    return TRUE;
}

/**
 Context corresponding to a single source when displaying differences
 between two sources.
 */
typedef struct _HEXDUMP_ONE_OBJECT {

    /**
     A full path expanded for this source.
     */
    YORI_STRING FullFileName;

    /**
     A handle to the source of this data.
     */
    HANDLE FileHandle;

    /**
     A buffer to hold data read from this source.
     */
    PUCHAR Buffer;

    /**
     The number of bytes read from this source.
     */
    DWORD BytesReturned;

    /**
     Set to TRUE if a read operation from this source has failed.
     */
    BOOL ReadFailed;

    /**
     The number of bytes to display for a given line from this
     buffer.  This is recalculated for each line based on the source's
     buffer length.
     */
    DWORD DisplayLength;
} HEXDUMP_ONE_OBJECT, *PHEXDUMP_ONE_OBJECT;

/**
 Display the differences between two files in hex form.

 @param FileA The name of the first file, without any full path expansion.

 @param FileB The name of the second file, without any full path expansion.

 @param HexDumpContext Pointer to the context indicating display parameters.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
HexDumpDisplayDiff(
    __in PYORI_STRING FileA,
    __in PYORI_STRING FileB,
    __in PHEXDUMP_CONTEXT HexDumpContext
    )
{
    HEXDUMP_ONE_OBJECT Objects[2];
    DWORD BufferSize;
    DWORD BufferOffset;
    DWORD LengthToDisplay;
    DWORD LengthThisLine;
    DWORD DisplayFlags;
    LARGE_INTEGER StreamOffset;
    DWORD Count;
    BOOL Result = FALSE;
    BOOL LineDifference;

    BufferSize = 64 * 1024;
    DisplayFlags = 0;
    if (!HexDumpContext->HideOffset) {
        DisplayFlags |= YORI_LIB_HEX_FLAG_DISPLAY_LARGE_OFFSET;
    }
    if (!HexDumpContext->HideCharacters) {
        DisplayFlags |= YORI_LIB_HEX_FLAG_DISPLAY_CHARS;
    }
    StreamOffset.QuadPart = HexDumpContext->OffsetToDisplay;

    ZeroMemory(Objects, sizeof(Objects));

    for (Count = 0; Count < sizeof(Objects)/sizeof(Objects[0]); Count++) {

        //
        //  Resolve the file to a full path
        //

        YoriLibInitEmptyString(&Objects[Count].FullFileName);
        if (Count == 0) {
            if (!YoriLibUserStringToSingleFilePath(FileA, TRUE, &Objects[Count].FullFileName)) {
                goto Exit;
            }
        } else {
            if (!YoriLibUserStringToSingleFilePath(FileB, TRUE, &Objects[Count].FullFileName)) {
                goto Exit;
            }
        }

        //
        //  Open each file
        //

        Objects[Count].FileHandle = CreateFile(Objects[Count].FullFileName.StartOfString,
                                               GENERIC_READ,
                                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                               NULL,
                                               OPEN_EXISTING,
                                               FILE_ATTRIBUTE_NORMAL,
                                               NULL);

        if (Objects[Count].FileHandle == NULL || Objects[Count].FileHandle == INVALID_HANDLE_VALUE) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hexdump: open of %y failed: %s"), &Objects[Count].FullFileName, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            goto Exit;
        }

        //
        //  Allocate a read buffer for the file
        //

        Objects[Count].Buffer = YoriLibMalloc(BufferSize);
        if (Objects[Count].Buffer == NULL) {
            goto Exit;
        }

        //
        //  Seek to the requested offset in the file.  Note that in the diff
        //  case we have files, so seeking is valid.
        //

        SetFilePointer(Objects[Count].FileHandle, StreamOffset.LowPart, &StreamOffset.HighPart, FILE_BEGIN);
        if (GetLastError() != NO_ERROR) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hexdump: seek of %y failed: %s"), &Objects[Count].FullFileName, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            goto Exit;
        }
    }

    while (TRUE) {

        //
        //  Read from each file
        //

        for (Count = 0; Count < sizeof(Objects)/sizeof(Objects[0]); Count++) {
            Objects[Count].ReadFailed = FALSE;
            Objects[Count].BytesReturned = 0;
            if (!ReadFile(Objects[Count].FileHandle, Objects[Count].Buffer, BufferSize, &Objects[Count].BytesReturned, NULL)) {
                Objects[Count].ReadFailed = TRUE;
                Objects[Count].BytesReturned = 0;
            } else if (Objects[Count].BytesReturned == 0) {
                Objects[Count].ReadFailed = TRUE;
            }
        }

        //
        //  If we've finished both sources, we are done.
        //

        if (Objects[0].ReadFailed && Objects[1].ReadFailed) {
            break;
        }

        //
        //  Display the maximum of what was read between the two
        //

        if (Objects[0].BytesReturned > Objects[1].BytesReturned) {
            LengthToDisplay = Objects[0].BytesReturned;
        } else {
            LengthToDisplay = Objects[1].BytesReturned;
        }

        //
        //  Truncate the display to the range the user requested
        //

        if (HexDumpContext->LengthToDisplay != 0) {
            if (StreamOffset.QuadPart + LengthToDisplay >= HexDumpContext->OffsetToDisplay + HexDumpContext->LengthToDisplay) {
                LengthToDisplay = (DWORD)(HexDumpContext->OffsetToDisplay + HexDumpContext->LengthToDisplay - StreamOffset.QuadPart);
                if (LengthToDisplay == 0) {
                    break;
                }
            }
        }

        BufferOffset = 0;

        while(LengthToDisplay > 0) {

            //
            //  Check each line to see if it's different
            //

            LineDifference = FALSE;
            if (LengthToDisplay >= 16) {
                LengthThisLine = 16;
            } else {
                LengthThisLine = LengthToDisplay;
            }
            for (Count = 0; Count < sizeof(Objects)/sizeof(Objects[0]); Count++) {
                Objects[Count].DisplayLength = LengthThisLine;
                if (BufferOffset + LengthThisLine > Objects[Count].BytesReturned) {
                    LineDifference = TRUE;
                    Objects[Count].DisplayLength = 0;
                    if (Objects[Count].BytesReturned > BufferOffset) {
                        Objects[Count].DisplayLength = Objects[0].BytesReturned - BufferOffset;
                    }
                }
            }

            if (!LineDifference &&
                memcmp(&Objects[0].Buffer[BufferOffset], &Objects[1].Buffer[BufferOffset], LengthThisLine) != 0) {
                LineDifference = TRUE;
            }

            //
            //  If it's different, display it
            //

            if (LineDifference) {
                if (!YoriLibHexDiff(StreamOffset.QuadPart + BufferOffset,
                                    &Objects[0].Buffer[BufferOffset],
                                    Objects[0].DisplayLength,
                                    &Objects[1].Buffer[BufferOffset],
                                    Objects[1].DisplayLength,
                                    HexDumpContext->BytesPerGroup,
                                    DisplayFlags)) {
                    break;
                }
            }

            //
            //  Move to the next line
            //

            LengthToDisplay -= LengthThisLine;
            BufferOffset += LengthThisLine;
            /*
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                          _T("LengthToDisplay %i LengthThisLine %i BufferOffset %i BR1 %i BR2 %i B1DL %i B2DL %i\n"),
                          LengthToDisplay,
                          LengthThisLine,
                          BufferOffset,
                          Objects[0].BytesReturned,
                          Objects[1].BytesReturned,
                          Buffer1DisplayLength,
                          Buffer2DisplayLength);
                          */
        }
    }

Exit:

    //
    //  Clean up state from each source
    //

    for (Count = 0; Count < sizeof(Objects)/sizeof(Objects[0]); Count++) {
        if (Objects[Count].FileHandle != NULL && Objects[Count].FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(Objects[Count].FileHandle);
        }
        if (Objects[Count].Buffer != NULL) {
            YoriLibFree(Objects[Count].Buffer);
        }
        YoriLibFreeStringContents(&Objects[Count].FullFileName);
    }

    return Result;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the hexdump builtin command.
 */
#define ENTRYPOINT YoriCmd_HEXDUMP
#else
/**
 The main entrypoint for the hexdump standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the hexdump cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero indicating success or nonzero on
         failure.
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
    DWORD MatchFlags;
    DWORD CharsConsumed;
    BOOL Recursive = FALSE;
    BOOL BasicEnumeration = FALSE;
    BOOL DiffMode = FALSE;
    HEXDUMP_CONTEXT HexDumpContext;
    YORI_STRING Arg;

    ZeroMemory(&HexDumpContext, sizeof(HexDumpContext));
    HexDumpContext.BytesPerGroup = 4;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                HexDumpHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                DiffMode = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("g1")) == 0) {
                HexDumpContext.BytesPerGroup = 1;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("g2")) == 0) {
                HexDumpContext.BytesPerGroup = 2;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("g4")) == 0) {
                HexDumpContext.BytesPerGroup = 4;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("g8")) == 0) {
                HexDumpContext.BytesPerGroup = 8;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("hc")) == 0) {
                HexDumpContext.HideCharacters = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("ho")) == 0) {
                HexDumpContext.HideOffset = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                if (ArgC > i + 1) {
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &HexDumpContext.LengthToDisplay, &CharsConsumed);
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("o")) == 0) {
                if (ArgC > i + 1) {
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &HexDumpContext.OffsetToDisplay, &CharsConsumed);
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
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

    if (DiffMode) {
        if (StartArg == 0 || StartArg + 2 > ArgC) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hexdump: insufficient arguments\n"));
            return EXIT_FAILURE;
        }

        if (!HexDumpDisplayDiff(&ArgV[StartArg], &ArgV[StartArg + 1], &HexDumpContext)) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0) {
        DWORD FileType = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
        FileType = FileType & ~(FILE_TYPE_REMOTE);
        if (FileType == FILE_TYPE_CHAR) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
            return EXIT_FAILURE;
        }

        HexDumpProcessStream(GetStdHandle(STD_INPUT_HANDLE), &HexDumpContext);
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
        if (Recursive) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
        if (BasicEnumeration) {
            MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
        }

        for (i = StartArg; i < ArgC; i++) {

            YoriLibForEachFile(&ArgV[i], MatchFlags, 0, HexDumpFileFoundCallback, &HexDumpContext);
        }
    }

    if (HexDumpContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("hexdump: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
