/**
 * @file lib/clip.c
 *
 * Convert a Yori string containing HTML into a Utf-8 formatted text stream for
 * use in the clipboard.
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
 Convert a Yori string into clipboard format.

 @param TextToCopy Pointer to the string to convert.

 @param HandleForClipboard On successful completion, points to a handle
        suitable for sending to the clipboard.

 @return TRUE to indicate success or FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibBuildHtmlClipboardBuffer(
    __in PYORI_STRING TextToCopy,
    __out PHANDLE HandleForClipboard
    )
{
    HANDLE hMem;
    PUCHAR pMem;

    DWORD EncodingToUse;
    DWORD UserBytes;
    DWORD BytesNeeded;
    DWORD Return;

    DWORD WinMajorVer;
    DWORD WinMinorVer;
    DWORD BuildNumber;

    YoriLibGetOsVersion(&WinMajorVer, &WinMinorVer, &BuildNumber);

    if (WinMajorVer < 4) {
        EncodingToUse = CP_OEMCP;
    } else {
        EncodingToUse = CP_UTF8;
    }

    UserBytes = WideCharToMultiByte(EncodingToUse, 0, TextToCopy->StartOfString, TextToCopy->LengthInChars, NULL, 0, NULL, NULL);
    if (UserBytes == 0) {
        return FALSE;
    }

    BytesNeeded = UserBytes + HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + HTMLCLIP_FRAGEND_SIZE + 2;

    //
    //  Allocate space to copy the text contents.
    //

    hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, BytesNeeded);
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
                    (int)(HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + HTMLCLIP_FRAGEND_SIZE + 1 + UserBytes),
                    (int)(HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE),
                    (int)(HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + 1 + UserBytes));

    Return = WideCharToMultiByte(EncodingToUse, 0, TextToCopy->StartOfString, TextToCopy->LengthInChars, (LPSTR)(pMem + HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE), UserBytes, NULL, NULL);
    if (Return == 0) {
        GlobalUnlock(hMem);
        GlobalFree(hMem);
        return FALSE;
    }

    //
    //  Fill in the footer of the protocol.
    //

    YoriLibSPrintfA((PCHAR)(pMem + UserBytes + HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + 1),
                    "%s",
                    DummyFragEnd);

    GlobalUnlock(hMem);

    *HandleForClipboard = hMem;

    return TRUE;
}

/**
 Retrieve any text from the clipboard and output it into a Yori string.

 @param Buffer The string to populate with the contents of the clipboard.
        If the string does not contain a large enough buffer to contain the
        clipboard contents, it is reallocated.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibPasteText(
    __inout PYORI_STRING Buffer
    )
{
    HANDLE hMem;
    LPWSTR pMem;
    DWORD StringLength;

    YoriLibLoadUser32Functions();

    if (DllUser32.pOpenClipboard == NULL ||
        DllUser32.pGetClipboardData == NULL ||
        DllUser32.pCloseClipboard == NULL) {
        return FALSE;
    }

    //
    //  Open the clipboard and fetch its contents.
    //

    if (!DllUser32.pOpenClipboard(NULL)) {
        return FALSE;
    }

    hMem = DllUser32.pGetClipboardData(CF_UNICODETEXT);
    if (hMem == NULL) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    pMem = GlobalLock(hMem);
    if (pMem == NULL) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }
    StringLength = _tcslen(pMem);

    if (StringLength >= Buffer->LengthAllocated) {
        YoriLibFreeStringContents(Buffer);
        YoriLibAllocateString(Buffer, StringLength + 1);
    }
    memcpy(Buffer->StartOfString, pMem, (StringLength + 1) * sizeof(WCHAR));
    Buffer->LengthInChars = StringLength;
    GlobalUnlock(hMem);

    //
    //  Truncate any newlines which are not normally intended when pasting
    //  into the shell
    //

    while(Buffer->LengthInChars > 0) {
        TCHAR TestChar = Buffer->StartOfString[Buffer->LengthInChars - 1];
        if (TestChar == '\r' || TestChar == '\n') {
            Buffer->StartOfString[Buffer->LengthInChars - 1] = '\0';
            Buffer->LengthInChars--;
        } else {
            break;
        }
    }

    DllUser32.pCloseClipboard();
    return TRUE;
}

/**
 Copy a Yori string into the clipboard in text format only.

 @param Buffer The string to populate into the clipboard.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibCopyText(
    __in PYORI_STRING Buffer
    )
{
    HANDLE hMem;
    HANDLE hClip;
    LPWSTR pMem;

    YoriLibLoadUser32Functions();

    if (DllUser32.pOpenClipboard == NULL ||
        DllUser32.pEmptyClipboard == NULL ||
        DllUser32.pSetClipboardData == NULL ||
        DllUser32.pCloseClipboard == NULL) {
        return FALSE;
    }

    //
    //  Open the clipboard and empty its contents.
    //

    if (!DllUser32.pOpenClipboard(NULL)) {
        return FALSE;
    }

    DllUser32.pEmptyClipboard();

    hMem = GlobalAlloc(GMEM_MOVEABLE, (Buffer->LengthInChars + 1) * sizeof(TCHAR));
    if (hMem == NULL) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    pMem = GlobalLock(hMem);
    if (pMem == NULL) {
        GlobalFree(hMem);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    memcpy(pMem, Buffer->StartOfString, Buffer->LengthInChars * sizeof(TCHAR));
    pMem[Buffer->LengthInChars] = '\0';
    GlobalUnlock(hMem);
    pMem = NULL;

    hClip = DllUser32.pSetClipboardData(CF_UNICODETEXT, hMem);
    if (hClip == NULL) {
        DllUser32.pCloseClipboard();
        GlobalFree(hMem);
        return FALSE;
    }

    DllUser32.pCloseClipboard();
    GlobalFree(hMem);
    return TRUE;
}

/**
 Copy a Yori text string into the clipboard along with an HTML and RTF
 representation of the same string.

 @param TextVersion The string to populate into the clipboard in text format.

 @param RtfVersion The string to populate into the clipboard in RTF format.

 @param HtmlVersion The string to populate into the clipboard in HTML format.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibCopyTextRtfAndHtml(
    __in PYORI_STRING TextVersion,
    __in PYORI_STRING RtfVersion,
    __in PYORI_STRING HtmlVersion
    )
{
    HANDLE hText;
    HANDLE hHtml;
    HANDLE hRtf;
    HANDLE hClip;
    LPWSTR pWideMem;
    LPSTR  pNarrowMem;
    DWORD Index;
    UINT HtmlFmt;
    UINT RtfFmt;

    YoriLibLoadUser32Functions();

    if (DllUser32.pOpenClipboard == NULL ||
        DllUser32.pEmptyClipboard == NULL ||
        DllUser32.pRegisterClipboardFormatW == NULL ||
        DllUser32.pSetClipboardData == NULL ||
        DllUser32.pCloseClipboard == NULL) {
        return FALSE;
    }

    //
    //  Open the clipboard and empty its contents.
    //

    if (!DllUser32.pOpenClipboard(NULL)) {
        return FALSE;
    }

    //
    //  The clipboard uses global memory handles.  Copy the text form into
    //  a global memory allocation.
    //

    hText = GlobalAlloc(GMEM_MOVEABLE, (TextVersion->LengthInChars + 1) * sizeof(TCHAR));
    if (hText == NULL) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    pWideMem = GlobalLock(hText);
    if (pWideMem == NULL) {
        GlobalFree(hText);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    memcpy(pWideMem, TextVersion->StartOfString, TextVersion->LengthInChars * sizeof(TCHAR));
    pWideMem[TextVersion->LengthInChars] = '\0';
    GlobalUnlock(hText);
    pWideMem = NULL;

    //
    //  The clipboard uses global memory handles.  Copy the RTF form into
    //  a global memory allocation.  Because RTF predates Unicode, any
    //  extended characters must have already been encoded when generating
    //  RTF, so the high 8 bits per char are discarded here.
    //

    hRtf = GlobalAlloc(GMEM_MOVEABLE, RtfVersion->LengthInChars + 1);
    if (hRtf == NULL) {
        GlobalFree(hText);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    pNarrowMem = GlobalLock(hRtf);
    if (pNarrowMem == NULL) {
        GlobalFree(hText);
        GlobalFree(hRtf);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    for (Index = 0; Index < RtfVersion->LengthInChars; Index++) {
        pNarrowMem[Index] = (UCHAR)(RtfVersion->StartOfString[Index] & 0x7f);
    }
    pNarrowMem[RtfVersion->LengthInChars] = '\0';

    GlobalUnlock(hRtf);
    pNarrowMem = NULL;

    //
    //  Construct a global memory region for HTML.  This is more involved
    //  because unlike the others it has a header.
    //

    if (!YoriLibBuildHtmlClipboardBuffer(HtmlVersion, &hHtml)) {
        GlobalFree(hText);
        GlobalFree(hRtf);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    DllUser32.pEmptyClipboard();

    HtmlFmt = DllUser32.pRegisterClipboardFormatW(_T("HTML Format"));
    if (HtmlFmt == 0) {
        GlobalFree(hText);
        GlobalFree(hRtf);
        GlobalFree(hHtml);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    RtfFmt = DllUser32.pRegisterClipboardFormatW(_T("Rich Text Format"));
    if (RtfFmt == 0) {
        GlobalFree(hText);
        GlobalFree(hRtf);
        GlobalFree(hHtml);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    hClip = DllUser32.pSetClipboardData(CF_UNICODETEXT, hText);
    if (hClip == NULL) {
        DllUser32.pCloseClipboard();
        GlobalFree(hHtml);
        GlobalFree(hRtf);
        GlobalFree(hText);
        return FALSE;
    }

    hClip = DllUser32.pSetClipboardData(RtfFmt, hRtf);
    if (hClip == NULL) {
        DllUser32.pCloseClipboard();
        GlobalFree(hHtml);
        GlobalFree(hRtf);
        GlobalFree(hText);
        return FALSE;
    }

    hClip = DllUser32.pSetClipboardData(HtmlFmt, hHtml);
    if (hClip == NULL) {
        DllUser32.pCloseClipboard();
        GlobalFree(hHtml);
        GlobalFree(hRtf);
        GlobalFree(hText);
        return FALSE;
    }

    DllUser32.pCloseClipboard();
    GlobalFree(hText);
    GlobalFree(hRtf);
    GlobalFree(hHtml);
    return TRUE;
}

// vim:sw=4:ts=4:et:
