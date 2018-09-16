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
CHAR strHelpText[] =
        "\n"
        "Output the contents of one or more files in hex.\n"
        "\n"
        "HEXDUMP [-license] [-b] [-g1|-g2|-g4|-g8] [-hc] [-ho]\n"
        "        [-l length] [-o offset] [-s] [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
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
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
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
 The main entrypoint for the hexdump cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero indicating success or nonzero on
         failure.
 */
DWORD
ymain(
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
