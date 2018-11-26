/**
 * @file clip/clip.c
 *
 * Copy HTML text onto the clipboard in HTML formatting for use in applications
 * that support rich text.
 *
 * Copyright (c) 2015-2018 Malcolm J. Smith
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
        "CLIP [-license] [-e|-h|-p|-t] [filename]\n"
        "\n"
        "   -e             Empty clipboard\n"
        "   -h             Copy to the clipboard in HTML format\n"
        "   -p             Paste the clipboard\n"
        "   -t             Retain only plain text in the clipboard\n";

/**
 Display usage text for clip.
 */
BOOL
ClipHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Clip %i.%i\n"), CLIP_VER_MAJOR, CLIP_VER_MINOR);
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
 Paste the text contents from the clipboard to an output pipe or file.

 @param hFile The device to output clipboard contents to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ClipPasteText(
    __in HANDLE hFile
    )
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
    YoriLibOutputToDevice(hFile, 0, _T("%s"), pMem);
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
ClipPreserveText(
    )
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
ClipEmptyClipboard(
    )
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
    BOOL   HtmlMode = FALSE;
    BOOL   OpenedFile = FALSE;
    BOOL   Empty = FALSE;
    BOOL   Paste = FALSE;
    BOOL   PreserveText = FALSE;
    
    DWORD  CurrentArg;
    YORI_STRING Arg;

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
                    YoriLibDisplayMitLicense(_T("2015-2018"));
                    return EXIT_SUCCESS;
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                    if (!Paste && !HtmlMode && !PreserveText) {
                        ArgParsed = TRUE;
                        Empty = TRUE;
                    }
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("h")) == 0) {
                    if (!Paste && !Empty && !PreserveText) {
                        ArgParsed = TRUE;
                        HtmlMode = TRUE;
                    }
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                    if (!HtmlMode && !Empty && !PreserveText) {
                        ArgParsed = TRUE;
                        Paste = TRUE;
                    }
                } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                    if (!HtmlMode && !Empty && !Paste) {
                        ArgParsed = TRUE;
                        PreserveText = TRUE;
                    }
                }
    
                if (!ArgParsed) {
                    ClipHelp();
                    return EXIT_FAILURE;
                }
            }
        }

        if (!Empty && !PreserveText) {
            for (CurrentArg = 1; CurrentArg < ArgC; CurrentArg++) {
                if (!YoriLibIsCommandLineOption(&ArgV[CurrentArg], &Arg)) {
                    if (!Paste) {
                        hFile = CreateFile(ArgV[CurrentArg].StartOfString, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);
                    } else {
                        hFile = CreateFile(ArgV[CurrentArg].StartOfString, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,NULL);
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

    if (!OpenedFile && !Empty && !PreserveText) {

        if (Paste) {
            hFile = GetStdHandle(STD_OUTPUT_HANDLE);
        } else {

            DWORD FileType;

            FileSize = MAX_PIPE_SIZE;
            hFile = GetStdHandle(STD_INPUT_HANDLE);

            //
            //  As a bit of a hack, if the input is stdin and stdin is to
            //  a console (as opposed to a pipe or file) assume the user
            //  isn't sure how to run this program and help them along.
            //

            FileType = GetFileType(hFile);
            FileType = FileType & ~(FILE_TYPE_REMOTE);
            if (FileType == FILE_TYPE_CHAR) {
                ClipHelp();
                return EXIT_FAILURE;
            }
        }
    }

    YoriLibLoadUser32Functions();

    if (Empty) {
        if (!ClipEmptyClipboard()) {
            return EXIT_FAILURE;
        }
    } else if (Paste) {
        if (!ClipPasteText(hFile)) {
            return EXIT_FAILURE;
        }
    } else if (HtmlMode) {
        if (!ClipCopyAsHtml(hFile, FileSize)) {
            return EXIT_FAILURE;
        }
    } else if (PreserveText) {
        if (!ClipPreserveText()) {
            return EXIT_FAILURE;
        }
    } else {
        if (!ClipCopyAsText(hFile, FileSize)) {
            return EXIT_FAILURE;
        }
    }

    if (OpenedFile) {
        CloseHandle(hFile);
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
