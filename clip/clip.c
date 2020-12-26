/**
 * @file clip/clip.c
 *
 * Copy HTML text onto the clipboard in HTML formatting for use in applications
 * that support rich text.
 *
 * Copyright (c) 2015-2020 Malcolm J. Smith
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

/**
 Help text for this application.
 */
const
CHAR strClipHelpText[] = 
        "\n"
        "Manipulate clipboard state including copy and paste.\n"
        "\n"
        "CLIP [-license] [-e|-h|-l|-p|-r|-t] [filename]\n"
        "\n"
        "   -e             Empty clipboard\n"
        "   -h             Copy to the clipboard in HTML format\n"
        "   -l             List formats available on the clipboard\n"
        "   -p             Paste text from the clipboard\n"
        "   -ph            Paste HTML from the clipboard\n"
        "   -pr            Paste rich text from the clipboard\n"
        "   -r             Copy to the clipboard in RTF format\n"
        "   -t             Retain only plain text in the clipboard\n";

/**
 Display usage text for clip.
 */
BOOL
ClipHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Clip %i.%02i\n"), CLIP_VER_MAJOR, CLIP_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strClipHelpText);
    return TRUE;
}

/**
 Dummy header.  0.9 is part of the protocol.  The rest contain strings
 that are 8 characters long that can be overwritten with real zero padded
 strings.  The zero padding helps keep computation sane.
 Dummy because it is only used to count its size, not used as a string.
 */
static const CHAR DummyHeader[] = 
                  "Version:0.9\n"
                  "StartHTML:12345678\n"
                  "EndHTML:12345678\n"
                  "StartFragment:12345678\n"
                  "EndFragment:12345678\n";

/**
 The length of the header, in bytes.
 */
#define HTMLCLIP_HDR_SIZE (sizeof(DummyHeader)-1)

/**
 A string indicating the start of a fragment.
 Dummy because it is only used to count its size, not used as a string.
 */
static const CHAR DummyFragStart[] = 
                  "<!--StartFragment-->";

/**
 The length of the fragment start string, in bytes.
 */
#define HTMLCLIP_FRAGSTART_SIZE (sizeof(DummyFragStart)-1)

/**
 A string indicating the end of a fragment.
 Dummy because it is only used to count its size, not used as a string.
 */
static const CHAR DummyFragEnd[] = 
                  "<!--EndFragment-->";

/**
 The length of the fragment end string, in bytes.
 */
#define HTMLCLIP_FRAGEND_SIZE (sizeof(DummyFragEnd)-1)

/**
 The maximum amount of data to buffer from a pipe before acting upon it.  The
 clipboard requires a full buffer of data to oeprate with.
 */
#define MAX_PIPE_SIZE (4*1024*1024)

/**
 Copy the contents of a file or pipe to the clipboard in HTML format.

 @param hFile Handle to the file or pipe.

 @param FileSize The length of the file, in bytes.  For a pipe, this value is
        not known beforehand, and is specified as MAX_PIPE_SIZE.

 @return TRUE to indicate success or FALSE to indicate failure.
 */
BOOL
ClipCopyAsHtml(
    __in HANDLE hFile,
    __in DWORD FileSize
    )
{
    DWORD  AllocSize;
    HANDLE hMem;
    PUCHAR pMem;
    DWORD  BytesTransferred;
    DWORD  CurrentOffset;
    UINT   ClipFmt;
    DWORD  Err;
    LPTSTR ErrText;

    //
    //  Allocate space to copy the file or pipe contents.
    //

    AllocSize = FileSize + HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + HTMLCLIP_FRAGEND_SIZE + 2;
    hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, AllocSize);
    if (hMem == NULL) {
        return FALSE;
    }

    pMem = GlobalLock(hMem);
    if (pMem == NULL) {
        GlobalFree(hMem);
        return FALSE;
    }

    //
    //  Note this is not Unicode.
    //
    //  First, prepare the header.  Leave a magic space at the end of the
    //  header.
    //

    YoriLibSPrintfA((PCHAR)pMem,
                    "Version:0.9\n"
                    "StartHTML:%08i\n"
                    "EndHTML:%08i\n"
                    "StartFragment:%08i\n"
                    "EndFragment:%08i\n"
                    "<!--StartFragment-->"
                    " ",
                    (int)HTMLCLIP_HDR_SIZE,
                    (int)(HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + HTMLCLIP_FRAGEND_SIZE + 1 + FileSize),
                    (int)(HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE),
                    (int)(HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + 1 + FileSize));

    CurrentOffset = 0;

    //
    //  Read text immediately following the header that we just prepared.
    //

    while (ReadFile(hFile, pMem + HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + 1 + CurrentOffset, FileSize - CurrentOffset, &BytesTransferred, NULL)) {
        CurrentOffset += BytesTransferred;

        //
        //  If we're reading from the pipe and have exceeded our storage,
        //  fail.
        //

        if (CurrentOffset == FileSize &&
            FileSize == MAX_PIPE_SIZE &&
            hFile == GetStdHandle(STD_INPUT_HANDLE)) {

            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Pipe data too large for buffer.  Limit is %i bytes\n"), MAX_PIPE_SIZE);
            return FALSE;
        }

        if (BytesTransferred == 0) {
            break;
        }
    }

    if (CurrentOffset == 0) {
        ClipHelp();
        return FALSE;
    }

    //
    //  If we don't have as much text as we expected, the header is wrong
    //  and needs to be rewritten with correct values.
    //

    if (CurrentOffset < FileSize) {
        FileSize = CurrentOffset;
        YoriLibSPrintfA((PCHAR)pMem,
                        "Version:0.9\n"
                        "StartHTML:%08i\n"
                        "EndHTML:%08i\n"
                        "StartFragment:%08i\n"
                        "EndFragment:%08i\n"
                        "<!--StartFragment-->",
                        (int)HTMLCLIP_HDR_SIZE,
                        (int)(HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + HTMLCLIP_FRAGEND_SIZE + 1 + FileSize),
                        (int)(HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE),
                        (int)(HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + 1 + FileSize));

        //
        //  That magic space from above has now been set to NULL by printf.
        //  Put the magic space back.
        //

        pMem[HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE] = ' ';
    }

    //
    //  Fill in the footer of the protocol.
    //

    YoriLibSPrintfA((PCHAR)(pMem + FileSize + HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + 1),
                    "%s",
                    DummyFragEnd);

    GlobalUnlock(hMem);

    //
    //  Send the buffer to the clipboard.
    //

    if (!DllUser32.pOpenClipboard(NULL)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not open clipboard: %s\n"), ErrText);
        return FALSE;
    }

    if (!DllUser32.pEmptyClipboard()) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not empty clipboard: %s\n"), ErrText);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    ClipFmt = DllUser32.pRegisterClipboardFormatW(_T("HTML Format"));
    if (ClipFmt == 0) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not register clipboard format: %s\n"), ErrText);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    if (DllUser32.pSetClipboardData(ClipFmt, hMem) == NULL) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not set clipboard data: %s\n"), ErrText);
        DllUser32.pCloseClipboard();
        return FALSE;
    }
    DllUser32.pCloseClipboard();
    GlobalFree(hMem);
    return TRUE;
}

/**
 Copy the contents of a file or pipe to the clipboard in RTF format.

 @param hFile Handle to the file or pipe.

 @param FileSize The length of the file, in bytes.  For a pipe, this value is
        not known beforehand, and is specified as MAX_PIPE_SIZE.

 @return TRUE to indicate success or FALSE to indicate failure.
 */
BOOL
ClipCopyAsRtf(
    __in HANDLE hFile,
    __in DWORD FileSize
    )
{
    LPSTR  AnsiBuffer;
    DWORD  AllocSize;
    HANDLE hMem;
    PCHAR  pMem;
    DWORD  BytesTransferred;
    DWORD  CurrentOffset;
    DWORD  Err;
    LPTSTR ErrText;
    UINT   ClipFmt;

    //
    //  Allocate space to copy the file or pipe contents.
    //

    AllocSize = FileSize + 1;
    AnsiBuffer = YoriLibMalloc(AllocSize);
    if (AnsiBuffer == NULL) {
        return FALSE;
    }

    //
    //  Note this is not Unicode.
    //

    CurrentOffset = 0;

    //
    //  Read text.
    //

    while (ReadFile(hFile, AnsiBuffer + CurrentOffset, FileSize - CurrentOffset, &BytesTransferred, NULL)) {
        CurrentOffset += BytesTransferred;

        //
        //  If we're reading from the pipe and have exceeded our storage,
        //  fail.
        //

        if (CurrentOffset == FileSize &&
            FileSize == MAX_PIPE_SIZE &&
            hFile == GetStdHandle(STD_INPUT_HANDLE)) {

            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Pipe data too large for buffer.  Limit is %i bytes\n"), MAX_PIPE_SIZE);
            return FALSE;
        }

        if (BytesTransferred == 0) {
            break;
        }
    }

    if (CurrentOffset == 0) {
        YoriLibFree(AnsiBuffer);
        ClipHelp();
        return FALSE;
    }


    AllocSize = CurrentOffset;
    hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, (AllocSize + 1) * sizeof(TCHAR));
    if (hMem == NULL) {
        YoriLibFree(AnsiBuffer);
        return FALSE;
    }

    pMem = GlobalLock(hMem);
    if (pMem == NULL) {
        GlobalFree(hMem);
        YoriLibFree(AnsiBuffer);
        return FALSE;
    }

    for (CurrentOffset = 0; CurrentOffset < AllocSize; CurrentOffset++) {

        //
        //  Analyze gets really confused here and thinks AnsiBuffer is
        //  of size BytesTransferred (ie., it doesn't understand that
        //  the read started partway through the buffer.)  At this point,
        //  it is of size CurrentOffset.
        //
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6385)
#endif
        pMem[CurrentOffset] = (CHAR)(AnsiBuffer[CurrentOffset] & 0x7f);
    }

    pMem[AllocSize] = '\0';
    GlobalUnlock(hMem);

    YoriLibFree(AnsiBuffer);

    //
    //  Send the buffer to the clipboard.
    //

    if (!DllUser32.pOpenClipboard(NULL)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not open clipboard: %s\n"), ErrText);
        GlobalFree(hMem);
        return FALSE;
    }

    if (!DllUser32.pEmptyClipboard()) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not empty clipboard: %s\n"), ErrText);
        DllUser32.pCloseClipboard();
        GlobalFree(hMem);
        return FALSE;
    }

    ClipFmt = DllUser32.pRegisterClipboardFormatW(_T("Rich Text Format"));
    if (ClipFmt == 0) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not register clipboard format: %s\n"), ErrText);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    if (DllUser32.pSetClipboardData(ClipFmt, hMem) == NULL) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not set clipboard data: %s\n"), ErrText);
        DllUser32.pCloseClipboard();
        GlobalFree(hMem);
        return FALSE;
    }

    DllUser32.pCloseClipboard();
    GlobalFree(hMem);
    return TRUE;
}

/**
 Copy the contents of a file or pipe to the clipboard in text format.

 @param hFile Handle to the file or pipe.

 @param FileSize The length of the file, in bytes.  For a pipe, this value is
        not known beforehand, and is specified as MAX_PIPE_SIZE.

 @return TRUE to indicate success or FALSE to indicate failure.
 */
BOOL
ClipCopyAsText(
    __in HANDLE hFile,
    __in DWORD FileSize
    )
{
    LPSTR  AnsiBuffer;
    DWORD  AllocSize;
    HANDLE hMem;
    PTCHAR pMem;
    DWORD  BytesTransferred;
    DWORD  CurrentOffset;
    DWORD  Err;
    LPTSTR ErrText;

    //
    //  Allocate space to copy the file or pipe contents.
    //

    AllocSize = FileSize + 1;
    AnsiBuffer = YoriLibMalloc(AllocSize);
    if (AnsiBuffer == NULL) {
        return FALSE;
    }

    //
    //  Note this is not Unicode.
    //

    CurrentOffset = 0;

    //
    //  Read text.
    //

    while (ReadFile(hFile, AnsiBuffer + CurrentOffset, FileSize - CurrentOffset, &BytesTransferred, NULL)) {
        CurrentOffset += BytesTransferred;

        //
        //  If we're reading from the pipe and have exceeded our storage,
        //  fail.
        //

        if (CurrentOffset == FileSize &&
            FileSize == MAX_PIPE_SIZE &&
            hFile == GetStdHandle(STD_INPUT_HANDLE)) {

            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Pipe data too large for buffer.  Limit is %i bytes\n"), MAX_PIPE_SIZE);
            return FALSE;
        }

        if (BytesTransferred == 0) {
            break;
        }
    }

    if (CurrentOffset == 0) {
        YoriLibFree(AnsiBuffer);
        ClipHelp();
        return FALSE;
    }


    AllocSize = YoriLibGetMultibyteInputSizeNeeded(AnsiBuffer, CurrentOffset);
    hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, (AllocSize + 1) * sizeof(TCHAR));
    if (hMem == NULL) {
        YoriLibFree(AnsiBuffer);
        return FALSE;
    }

    pMem = GlobalLock(hMem);
    if (pMem == NULL) {
        GlobalFree(hMem);
        YoriLibFree(AnsiBuffer);
        return FALSE;
    }

    //
    //  Analyze gets really confused here and thinks AnsiBuffer is
    //  of size BytesTransferred (ie., it doesn't understand that
    //  the read started partway through the buffer.)  At this point,
    //  it is of size CurrentOffset.
    //
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6385)
#endif
    YoriLibMultibyteInput(AnsiBuffer, CurrentOffset, pMem, AllocSize);

    pMem[AllocSize] = '\0';
    GlobalUnlock(hMem);

    YoriLibFree(AnsiBuffer);

    //
    //  Send the buffer to the clipboard.
    //

    if (!DllUser32.pOpenClipboard(NULL)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not open clipboard: %s\n"), ErrText);
        GlobalFree(hMem);
        return FALSE;
    }

    if (!DllUser32.pEmptyClipboard()) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not empty clipboard: %s\n"), ErrText);
        DllUser32.pCloseClipboard();
        GlobalFree(hMem);
        return FALSE;
    }

    if (DllUser32.pSetClipboardData(CF_UNICODETEXT, hMem) == NULL) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not set clipboard data: %s\n"), ErrText);
        DllUser32.pCloseClipboard();
        GlobalFree(hMem);
        return FALSE;
    }

    DllUser32.pCloseClipboard();
    GlobalFree(hMem);
    return TRUE;
}

/**
 Enumerate clipboard formats and find a registered format that matches a
 specified name.  If no matching registered format is found, return zero.
 Note this function assumes the clipboard is already open.

 @param SearchFormatName Pointer to a string specifying the format name.

 @return The format number matching the name, or zero if no matching format
         is found.
 */
DWORD
ClipFindFormatByName(
    __in PCYORI_STRING SearchFormatName
    )
{
    DWORD  Format;
    TCHAR  FoundFormatName[100];
    INT    CharsCopied;

    Format = 0;
    while (TRUE) {
        Format = DllUser32.pEnumClipboardFormats(Format);
        if (Format == 0) {
            break;
        }

        FoundFormatName[0] = '\0';
        CharsCopied = DllUser32.pGetClipboardFormatNameW(Format, FoundFormatName, sizeof(FoundFormatName)/sizeof(FoundFormatName[0]));
        FoundFormatName[sizeof(FoundFormatName)/sizeof(FoundFormatName[0]) - 1] = '\0';

        if (FoundFormatName[0] != '\0' &&
            YoriLibCompareStringWithLiteralInsensitive(SearchFormatName, FoundFormatName) == 0) {

            return Format;
        }
    }

    return 0;
}

/**
 Given a clipboard buffer in HTML format, parse the headers to find the
 region of the buffer that contains the HTML contents, and return a pointer
 to the contents.  Note this function can modify the buffer to insert a NULL
 terminator at the end of the HTML range.

 @param Buffer Pointer to the buffer in HTML clipboard format.

 @param BufferLength Specifies the length of the buffer in bytes.

 @return Pointer to the start of the HTML range in the buffer, or NULL if the
         headers cannot be parsed correctly.
 */
PUCHAR
ClipExtractHtmlRange(
    __in PUCHAR Buffer,
    __in DWORD BufferLength
    )
{
    DWORD StartOffset;
    DWORD EndOffset;
    DWORD Index;

    PUCHAR HeaderNameStart;
    DWORD HeaderNameLength;
    PUCHAR HeaderValueStart;
    DWORD HeaderValueLength;

    StartOffset = 0;
    EndOffset = 0;

    HeaderNameStart = Buffer;
    HeaderNameLength = 0;
    HeaderValueStart = NULL;
    HeaderValueLength = 0;

    for (Index = 0; Index < BufferLength; Index++) {
        if (Buffer[Index] == '\n') {
            if (HeaderValueStart != NULL) {
                if (strnicmp(HeaderNameStart, "StartHTML", HeaderNameLength) == 0) {
                    StartOffset = atoi(HeaderValueStart);
                    if (StartOffset >= BufferLength) {
                        StartOffset = 0;
                    }
                }
                if (strnicmp(HeaderNameStart, "EndHTML", HeaderNameLength) == 0) {
                    EndOffset = atoi(HeaderValueStart);
                    if (EndOffset > BufferLength) {
                        EndOffset = 0;
                    }
                }
            }

            if (StartOffset != 0 && EndOffset != 0) {
                break;
            }
            HeaderValueStart = NULL;
            HeaderNameStart = &Buffer[Index + 1];
            HeaderNameLength = 0;
            continue;
        }

        if (Buffer[Index] == ':') {
            HeaderValueStart = &Buffer[Index + 1];
            HeaderValueLength = 0;
            continue;
        }

        if (HeaderValueStart != NULL) {
            HeaderValueLength++;
        } else {
            HeaderNameLength++;
        }
    }

    if (StartOffset != 0 && EndOffset != 0) {
        if (EndOffset < BufferLength) {
            Buffer[EndOffset] = '\0';
        }
        return &Buffer[StartOffset];
    }

    return NULL;
}

/**
 Paste the contents from the clipboard to an output pipe or file.

 @param hFile The device to output clipboard contents to.

 @param FormatString If not NULL, contains a string specifying the requested
        format.  If NULL, unicode text is assumed as the format.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ClipPasteSpecifiedFormat(
    __in HANDLE hFile,
    __in_opt PCYORI_STRING FormatString
    )
{
    HANDLE hMem;
    PUCHAR pMem;
    DWORD  CurrentOffset;
    DWORD  BufferSize;
    DWORD  Err;
    LPTSTR ErrText;
    DWORD  Format;

    //
    //  Open the clipboard and fetch its contents.
    //

    if (!DllUser32.pOpenClipboard(NULL)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not open clipboard: %s\n"), ErrText);
        return FALSE;
    }

    if (FormatString != NULL) {
        Format = ClipFindFormatByName(FormatString);
        if (Format == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: data not available in the specified format\n"));
            DllUser32.pCloseClipboard();
            return FALSE;
        }
    } else {
        Format = CF_UNICODETEXT;
    }

    SetLastError(0);
    hMem = DllUser32.pGetClipboardData(Format);
    if (hMem == NULL) {
        Err = GetLastError();
        if (Err != ERROR_SUCCESS) {
            ErrText = YoriLibGetWinErrorText(Err);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not get clipboard data: %s\n"), ErrText);
        }
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    CurrentOffset = 0;
    BufferSize = (DWORD)GlobalSize(hMem);
    pMem = GlobalLock(hMem);
    if (pMem == NULL) {
        Err = GetLastError();
        if (Err != ERROR_SUCCESS) {
            ErrText = YoriLibGetWinErrorText(Err);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not get clipboard data: %s\n"), ErrText);
        }
        DllUser32.pCloseClipboard();
        return FALSE;
    }
    if (FormatString != NULL &&
        YoriLibCompareStringWithLiteralInsensitive(FormatString, _T("HTML Format")) == 0) {
        pMem = ClipExtractHtmlRange(pMem, BufferSize);
        if (pMem != NULL) {
            YoriLibOutputToDevice(hFile, 0, _T("%hs"), pMem);
        }
    } else if (Format == CF_UNICODETEXT) {
        YoriLibOutputToDevice(hFile, 0, _T("%s"), pMem);
    } else {
        YoriLibOutputToDevice(hFile, 0, _T("%hs"), pMem);
    }
    GlobalUnlock(hMem);

    DllUser32.pCloseClipboard();
    return TRUE;
}

/**
 Take the existing contents of the clipboard, and place them back on the
 clipboard.  This implicitly destroys any other formats for the data (ie.,
 all rich text formats are purged.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ClipPreserveText(VOID)
{
    HANDLE hMem;
    PUCHAR pMem;
    DWORD  CurrentOffset;
    DWORD  BufferSize;
    DWORD  Err;
    LPTSTR ErrText;

    //
    //  Open the clipboard and fetch its contents.
    //

    if (!DllUser32.pOpenClipboard(NULL)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not open clipboard: %s\n"), ErrText);
        return FALSE;
    }

    hMem = DllUser32.pGetClipboardData(CF_UNICODETEXT);
    if (hMem == NULL) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not get clipboard data: %s\n"), ErrText);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    CurrentOffset = 0;
    BufferSize = (DWORD)GlobalSize(hMem);
    pMem = GlobalLock(hMem);

    if (!DllUser32.pEmptyClipboard()) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not empty clipboard: %s\n"), ErrText);
        GlobalUnlock(hMem);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    if (DllUser32.pSetClipboardData(CF_UNICODETEXT, hMem) == NULL) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not set clipboard data: %s\n"), ErrText);
        GlobalUnlock(hMem);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    GlobalUnlock(hMem);

    DllUser32.pCloseClipboard();
    return TRUE;
}

/**
 Remove the contents of the clipboard.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ClipEmptyClipboard(VOID)
{
    DWORD  Err;
    LPTSTR ErrText;

    //
    //  Open and empty the clipboard.
    //

    if (!DllUser32.pOpenClipboard(NULL)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not open clipboard: %s\n"), ErrText);
        return FALSE;
    }

    if (!DllUser32.pEmptyClipboard()) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not empty clipboard: %s\n"), ErrText);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    DllUser32.pCloseClipboard();
    return TRUE;
}


/**
 List all of the available clipboard formats.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ClipListFormats(VOID)
{
    DWORD  Err;
    LPTSTR ErrText;
    DWORD  Format;
    TCHAR  FormatName[100];
    INT    CharsCopied;

    //
    //  Open and empty the clipboard.
    //

    if (!DllUser32.pOpenClipboard(NULL)) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: could not open clipboard: %s\n"), ErrText);
        return FALSE;
    }

    Format = 0;
    while (TRUE) {
        Format = DllUser32.pEnumClipboardFormats(Format);
        if (Format == 0) {
            break;
        }

        FormatName[0] = '\0';
        CharsCopied = DllUser32.pGetClipboardFormatNameW(Format, FormatName, sizeof(FormatName)/sizeof(FormatName[0]));
        FormatName[sizeof(FormatName)/sizeof(FormatName[0]) - 1] = '\0';
        if (CharsCopied == 0) {
            switch(Format) {
                case CF_TEXT:
                    YoriLibSPrintf(FormatName, _T("Text"));
                    break;
                case CF_BITMAP:
                    YoriLibSPrintf(FormatName, _T("Bitmap"));
                    break;
                case CF_METAFILEPICT:
                    YoriLibSPrintf(FormatName, _T("Metafile"));
                    break;
                case CF_SYLK:
                    YoriLibSPrintf(FormatName, _T("SYLK"));
                    break;
                case CF_DIF:
                    YoriLibSPrintf(FormatName, _T("DIF"));
                    break;
                case CF_TIFF:
                    YoriLibSPrintf(FormatName, _T("TIFF"));
                    break;
                case CF_OEMTEXT:
                    YoriLibSPrintf(FormatName, _T("Ascii Text"));
                    break;
                case CF_DIB:
                    YoriLibSPrintf(FormatName, _T("DIB"));
                    break;
                case CF_PALETTE:
                    YoriLibSPrintf(FormatName, _T("Palette"));
                    break;
                case CF_PENDATA:
                    YoriLibSPrintf(FormatName, _T("Pen data"));
                    break;
                case CF_RIFF:
                    YoriLibSPrintf(FormatName, _T("RIFF"));
                    break;
                case CF_WAVE:
                    YoriLibSPrintf(FormatName, _T("WAVE"));
                    break;
                case CF_UNICODETEXT:
                    YoriLibSPrintf(FormatName, _T("Unicode Text"));
                    break;
                case CF_ENHMETAFILE:
                    YoriLibSPrintf(FormatName, _T("Enhanced Metafile"));
                    break;
            }
        }

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%04x %s\n"), Format, FormatName);
    }

    DllUser32.pCloseClipboard();
    return TRUE;
}

/**
 The set of operations supported by this program.
 */
typedef enum _CLIP_OPERATION {
    ClipOperationUnknown = 0,
    ClipOperationEmpty = 1,
    ClipOperationPreserveText = 2,
    ClipOperationCopyText = 3,
    ClipOperationCopyRtf = 4,
    ClipOperationCopyHtml = 5,
    ClipOperationPasteText = 6,
    ClipOperationListFormats = 7,
    ClipOperationPasteRichText = 8,
    ClipOperationPasteHTML = 9,
} CLIP_OPERATION;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the clip builtin command.
 */
#define ENTRYPOINT YoriCmd_YCLIP
#else
/**
 The main entrypoint for the clip standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The entrypoint for the clip application.

 @param ArgC Count of arguments.

 @param ArgV Array of argument strings.

 @return Exit code for the application, typically zero for success and
         nonzero for failure.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    HANDLE hFile = NULL;
    DWORD  Err;
    LPTSTR ErrText;
    DWORD  FileSize = 0;
    CLIP_OPERATION Op;
    BOOL   OpenedFile = FALSE;
    DWORD  CurrentArg;
    YORI_STRING Arg;
    DWORD  Result;

    Op = ClipOperationUnknown;

    //
    //  If the user specified a file, go read it.  If not, read
    //  from standard input.
    //

    if (ArgC > 1) {

        for (CurrentArg = 1; CurrentArg < ArgC; CurrentArg++) {

            ASSERT(YoriLibIsStringNullTerminated(&ArgV[CurrentArg]));

            if (YoriLibIsCommandLineOption(&ArgV[CurrentArg], &Arg)) {
                BOOL ArgParsed = FALSE;

                if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                    ClipHelp();
                    return EXIT_SUCCESS;
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                    YoriLibDisplayMitLicense(_T("2015-2020"));
                    return EXIT_SUCCESS;
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                    if (Op == ClipOperationUnknown) {
                        ArgParsed = TRUE;
                        Op = ClipOperationEmpty;
                    }
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                    if (Op == ClipOperationUnknown) {
                        ArgParsed = TRUE;
                        Op = ClipOperationCopyHtml;
                    }
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                    if (Op == ClipOperationUnknown) {
                        ArgParsed = TRUE;
                        Op = ClipOperationListFormats;
                    }
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                    if (Op == ClipOperationUnknown) {
                        ArgParsed = TRUE;
                        Op = ClipOperationPasteText;
                    }
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("ph")) == 0) {
                    if (Op == ClipOperationUnknown) {
                        ArgParsed = TRUE;
                        Op = ClipOperationPasteHTML;
                    }
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("pr")) == 0) {
                    if (Op == ClipOperationUnknown) {
                        ArgParsed = TRUE;
                        Op = ClipOperationPasteRichText;
                    }
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                    if (Op == ClipOperationUnknown) {
                        ArgParsed = TRUE;
                        Op = ClipOperationCopyRtf;
                    }
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                    if (Op == ClipOperationUnknown) {
                        ArgParsed = TRUE;
                        Op = ClipOperationPreserveText;
                    }
                }
    
                if (!ArgParsed) {
                    ClipHelp();
                    return EXIT_FAILURE;
                }
            }
        }
        if (Op == ClipOperationUnknown) {
            Op = ClipOperationCopyText;
        }

        if (Op == ClipOperationCopyText ||
            Op == ClipOperationCopyRtf ||
            Op == ClipOperationCopyHtml ||
            Op == ClipOperationPasteText ||
            Op == ClipOperationPasteRichText ||
            Op == ClipOperationPasteHTML) {

            for (CurrentArg = 1; CurrentArg < ArgC; CurrentArg++) {
                if (!YoriLibIsCommandLineOption(&ArgV[CurrentArg], &Arg)) {
                    if (Op == ClipOperationPasteText ||
                        Op == ClipOperationPasteRichText ||
                        Op == ClipOperationPasteHTML) {
                        hFile = CreateFile(ArgV[CurrentArg].StartOfString,
                                           GENERIC_WRITE,
                                           FILE_SHARE_READ|FILE_SHARE_DELETE,
                                           NULL,
                                           OPEN_ALWAYS,
                                           FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
                                           NULL);
                    } else {
                        hFile = CreateFile(ArgV[CurrentArg].StartOfString,
                                           GENERIC_READ,
                                           FILE_SHARE_READ|FILE_SHARE_DELETE,
                                           NULL,
                                           OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
                                           NULL);
                    }
                    if (hFile == INVALID_HANDLE_VALUE) {
                        Err = GetLastError();
                        ErrText = YoriLibGetWinErrorText(Err);
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("clip: open failed: %s\n"), ErrText);
                        return EXIT_FAILURE;
                    }
        
                    FileSize = GetFileSize(hFile, NULL);
                    OpenedFile = TRUE;
                    break;
                }
            }
        }
    }

    if (!OpenedFile &&
        (Op == ClipOperationCopyText ||
         Op == ClipOperationCopyRtf ||
         Op == ClipOperationCopyHtml ||
         Op == ClipOperationPasteText ||
         Op == ClipOperationPasteRichText ||
         Op == ClipOperationPasteHTML)) {

        if (Op == ClipOperationPasteText ||
            Op == ClipOperationPasteRichText ||
            Op == ClipOperationPasteHTML) {
            hFile = GetStdHandle(STD_OUTPUT_HANDLE);
        } else {

            FileSize = MAX_PIPE_SIZE;
            hFile = GetStdHandle(STD_INPUT_HANDLE);

            //
            //  As a bit of a hack, if the input is stdin and stdin is to
            //  a console (as opposed to a pipe or file) assume the user
            //  isn't sure how to run this program and help them along.
            //

            if (YoriLibIsStdInConsole()) {
                ClipHelp();
                return EXIT_FAILURE;
            }
        }
    }

    YoriLibLoadUser32Functions();
    Result = EXIT_SUCCESS;

    if (Op == ClipOperationEmpty) {
        if (!ClipEmptyClipboard()) {
            Result = EXIT_FAILURE;
        }
    } else if (Op == ClipOperationPasteText) {
        if (!ClipPasteSpecifiedFormat(hFile, NULL)) {
            Result = EXIT_FAILURE;
        }
    } else if (Op == ClipOperationPasteRichText) {
        YORI_STRING RichTextFormat = YORILIB_CONSTANT_STRING(_T("Rich Text Format"));
        if (!ClipPasteSpecifiedFormat(hFile, &RichTextFormat)) {
            Result = EXIT_FAILURE;
        }
    } else if (Op == ClipOperationPasteHTML) {
        YORI_STRING HTMLFormat = YORILIB_CONSTANT_STRING(_T("HTML Format"));
        if (!ClipPasteSpecifiedFormat(hFile, &HTMLFormat)) {
            Result = EXIT_FAILURE;
        }
    } else if (Op == ClipOperationCopyHtml) {
        if (!ClipCopyAsHtml(hFile, FileSize)) {
            Result = EXIT_FAILURE;
        }
    } else if (Op == ClipOperationCopyRtf) {
        if (!ClipCopyAsRtf(hFile, FileSize)) {
            Result = EXIT_FAILURE;
        }
    } else if (Op == ClipOperationPreserveText) {
        if (!ClipPreserveText()) {
            Result = EXIT_FAILURE;
        }
    } else if (Op == ClipOperationListFormats) {
        if (!ClipListFormats()) {
            Result = EXIT_FAILURE;
        }
    } else if (Op == ClipOperationCopyText) {
        if (!ClipCopyAsText(hFile, FileSize)) {
            Result = EXIT_FAILURE;
        }
    } else {
        Result = EXIT_FAILURE;
    }

    if (OpenedFile) {
        CloseHandle(hFile);
    }
    return Result;
}

// vim:sw=4:ts=4:et:
