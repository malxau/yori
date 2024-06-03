/**
 * @file lib/clip.c
 *
 * Convert a Yori string containing HTML into a Utf-8 formatted text stream for
 * use in the clipboard.
 *
 * Copyright (c) 2015-2021 Malcolm J. Smith
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
const CHAR YoriLibClipDummyHeader[] = 
                  "Version:0.9\n"
                  "StartHTML:12345678\n"
                  "EndHTML:12345678\n"
                  "StartFragment:12345678\n"
                  "EndFragment:12345678\n";

/**
 The length of the header, in bytes.
 */
#define HTMLCLIP_HDR_SIZE (sizeof(YoriLibClipDummyHeader)-1)

/**
 A string indicating the start of a fragment.
 Dummy because it is only used to count its size, not used as a string.
 */
const CHAR YoriLibClipDummyFragStart[] = 
                  "<!--StartFragment-->";

/**
 The length of the fragment start string, in bytes.
 */
#define HTMLCLIP_FRAGSTART_SIZE (sizeof(YoriLibClipDummyFragStart)-1)

/**
 A string indicating the end of a fragment.
 Dummy because it is only used to count its size, not used as a string.
 */
static const CHAR YoriLibClipDummyFragEnd[] = 
                  "<!--EndFragment-->";

/**
 The length of the fragment end string, in bytes.
 */
#define HTMLCLIP_FRAGEND_SIZE (sizeof(YoriLibClipDummyFragEnd)-1)

/**
 Attempt to open the clipboard with some retries in case it is currently in
 use.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibOpenClipboard(VOID)
{
    DWORD Attempt;

    if (DllUser32.pOpenClipboard == NULL) {
        return FALSE;
    }

    for (Attempt = 0; Attempt < 6; Attempt++) {
        if (Attempt > 0) {
            Sleep(1<<Attempt);
        }

        if (DllUser32.pOpenClipboard(NULL)) {
            return TRUE;
        }
    }

    return FALSE;
}

//
//  Older versions of the analysis engine don't understand that a buffer
//  allocated with GlobalAlloc and subsequently locked with GlobalLock has
//  the same writable size as specified in the call to GlobalAlloc.  Since
//  a generic overflow warning is useful, this is suppressed only for a
//  couple of versions that have an analysis engine but can't interpret
//  this condition.
//

#if defined(_MSC_VER) && (_MSC_VER >= 1500) && (_MSC_VER <= 1600)
#pragma warning(push)
#pragma warning(disable: 6386)
#endif

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

    if (DllKernel32.pGlobalLock == NULL ||
        DllKernel32.pGlobalUnlock == NULL) {

        return FALSE;
    }

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
    if (!YoriLibIsSizeAllocatable(BytesNeeded)) {
        return FALSE;
    }

    //
    //  Allocate space to copy the text contents.
    //

    hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, BytesNeeded);
    if (hMem == NULL) {
        return FALSE;
    }

    pMem = DllKernel32.pGlobalLock(hMem);
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

    YoriLibSPrintfSA((PCHAR)pMem,
                     (YORI_ALLOC_SIZE_T)BytesNeeded,
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
        DllKernel32.pGlobalUnlock(hMem);
        GlobalFree(hMem);
        return FALSE;
    }

    //
    //  Fill in the footer of the protocol.
    //

    YoriLibSPrintfA((PCHAR)(pMem + UserBytes + HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE + 1),
                    "%s",
                    YoriLibClipDummyFragEnd);

    DllKernel32.pGlobalUnlock(hMem);

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
    YORI_ALLOC_SIZE_T StringLength;

    YoriLibLoadUser32Functions();

    if (DllKernel32.pGlobalLock == NULL ||
        DllKernel32.pGlobalUnlock == NULL ||
        DllUser32.pOpenClipboard == NULL ||
        DllUser32.pGetClipboardData == NULL ||
        DllUser32.pCloseClipboard == NULL) {

        return FALSE;
    }

    //
    //  Open the clipboard and fetch its contents.
    //

    if (!YoriLibOpenClipboard()) {
        return FALSE;
    }

    hMem = DllUser32.pGetClipboardData(CF_UNICODETEXT);
    if (hMem == NULL) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    pMem = DllKernel32.pGlobalLock(hMem);
    if (pMem == NULL) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }
    StringLength = (YORI_ALLOC_SIZE_T)_tcslen(pMem);

    if (StringLength >= Buffer->LengthAllocated) {
        if (!YoriLibReallocateStringWithoutPreservingContents(Buffer, (YORI_ALLOC_SIZE_T)(StringLength + 1))) {
            DllKernel32.pGlobalUnlock(hMem);
            DllUser32.pCloseClipboard();
            return FALSE;
        }
    }
    memcpy(Buffer->StartOfString, pMem, (StringLength + 1) * sizeof(WCHAR));
    Buffer->LengthInChars = StringLength;
    DllKernel32.pGlobalUnlock(hMem);

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

    if (DllKernel32.pGlobalLock == NULL ||
        DllKernel32.pGlobalUnlock == NULL ||
        DllUser32.pOpenClipboard == NULL ||
        DllUser32.pEmptyClipboard == NULL ||
        DllUser32.pSetClipboardData == NULL ||
        DllUser32.pCloseClipboard == NULL) {
        return FALSE;
    }

    //
    //  Open the clipboard and empty its contents.
    //

    if (!YoriLibOpenClipboard()) {
        return FALSE;
    }

    DllUser32.pEmptyClipboard();

    hMem = GlobalAlloc(GMEM_MOVEABLE, (Buffer->LengthInChars + 1) * sizeof(TCHAR));
    if (hMem == NULL) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    pMem = DllKernel32.pGlobalLock(hMem);
    if (pMem == NULL) {
        GlobalFree(hMem);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    memcpy(pMem, Buffer->StartOfString, Buffer->LengthInChars * sizeof(TCHAR));
    pMem[Buffer->LengthInChars] = '\0';
    DllKernel32.pGlobalUnlock(hMem);
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

    if (DllKernel32.pGlobalLock == NULL ||
        DllKernel32.pGlobalUnlock == NULL ||
        DllUser32.pOpenClipboard == NULL ||
        DllUser32.pEmptyClipboard == NULL ||
        DllUser32.pRegisterClipboardFormatW == NULL ||
        DllUser32.pSetClipboardData == NULL ||
        DllUser32.pCloseClipboard == NULL) {
        return FALSE;
    }

    //
    //  Open the clipboard and empty its contents.
    //

    if (!YoriLibOpenClipboard()) {
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

    pWideMem = DllKernel32.pGlobalLock(hText);
    if (pWideMem == NULL) {
        GlobalFree(hText);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    memcpy(pWideMem, TextVersion->StartOfString, TextVersion->LengthInChars * sizeof(TCHAR));
    pWideMem[TextVersion->LengthInChars] = '\0';
    DllKernel32.pGlobalUnlock(hText);
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

    pNarrowMem = DllKernel32.pGlobalLock(hRtf);
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

    DllKernel32.pGlobalUnlock(hRtf);
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

/**
 Copy binary data into the clipboard.

 @param Buffer The data to populate into the clipboard.
 
 @param BufferLength The length of the data to populate into the clipboard, in
        bytes.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibCopyBinaryData(
    __in PUCHAR Buffer,
    __in YORI_ALLOC_SIZE_T BufferLength
    )
{
    HANDLE hMem;
    HANDLE hClip;
    PUCHAR pMem;
    SIZE_T ClipboardBufferLength;
    PDWORD InternalBufferLength;
    DWORD BinFmt;

    YoriLibLoadUser32Functions();

    if (DllKernel32.pGlobalLock == NULL ||
        DllKernel32.pGlobalUnlock == NULL ||
        DllUser32.pOpenClipboard == NULL ||
        DllUser32.pEmptyClipboard == NULL ||
        DllUser32.pRegisterClipboardFormatW == NULL ||
        DllUser32.pSetClipboardData == NULL ||
        DllUser32.pCloseClipboard == NULL) {
        return FALSE;
    }

    ClipboardBufferLength = BufferLength;
    ClipboardBufferLength = ClipboardBufferLength + sizeof(DWORD);
    if (!YoriLibIsSizeAllocatable(ClipboardBufferLength)) {
        return FALSE;
    }

    BinFmt = DllUser32.pRegisterClipboardFormatW(_T("BinaryData"));
    if (BinFmt == 0) {
        return FALSE;
    }

    //
    //  Open the clipboard and empty its contents.
    //

    if (!YoriLibOpenClipboard()) {
        return FALSE;
    }

    DllUser32.pEmptyClipboard();

    hMem = GlobalAlloc(GMEM_MOVEABLE, ClipboardBufferLength);
    if (hMem == NULL) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    pMem = DllKernel32.pGlobalLock(hMem);
    if (pMem == NULL) {
        GlobalFree(hMem);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    InternalBufferLength = (PDWORD)pMem;
    *InternalBufferLength = BufferLength;

    pMem = pMem + sizeof(DWORD);
    memcpy(pMem, Buffer, BufferLength);
    DllKernel32.pGlobalUnlock(hMem);
    pMem = NULL;

    hClip = DllUser32.pSetClipboardData(BinFmt, hMem);
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
 Retrieve any binary data from the clipboard.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibPasteBinaryData(
    __out PUCHAR *Buffer,
    __out PYORI_ALLOC_SIZE_T BufferLength
    )
{
    HANDLE hMem;
    PUCHAR pMem;
    SIZE_T ClipboardBufferLength;
    YORI_ALLOC_SIZE_T LocalBufferLength;
    PUCHAR LocalBuffer;
    PDWORD InternalBufferLength;
    DWORD BinFmt;

    YoriLibLoadUser32Functions();

    if (DllKernel32.pGlobalLock == NULL ||
        DllKernel32.pGlobalUnlock == NULL ||
        DllUser32.pOpenClipboard == NULL ||
        DllUser32.pRegisterClipboardFormatW == NULL ||
        DllUser32.pGetClipboardData == NULL ||
        DllUser32.pCloseClipboard == NULL) {

        return FALSE;
    }

    BinFmt = DllUser32.pRegisterClipboardFormatW(_T("BinaryData"));
    if (BinFmt == 0) {
        return FALSE;
    }

    //
    //  Open the clipboard and fetch its contents.
    //

    if (!YoriLibOpenClipboard()) {
        return FALSE;
    }

    hMem = DllUser32.pGetClipboardData(BinFmt);
    if (hMem == NULL) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    ClipboardBufferLength = DllKernel32.pGlobalSize(hMem);
    if (ClipboardBufferLength < sizeof(DWORD)) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    pMem = DllKernel32.pGlobalLock(hMem);
    if (pMem == NULL) {
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    InternalBufferLength = (PDWORD)pMem;
    if (*InternalBufferLength > ClipboardBufferLength - sizeof(DWORD)) {
        DllKernel32.pGlobalUnlock(hMem);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    ClipboardBufferLength = *InternalBufferLength;
    if (!YoriLibIsSizeAllocatable(ClipboardBufferLength)) {
        DllKernel32.pGlobalUnlock(hMem);
        DllUser32.pCloseClipboard();
        return FALSE;
    }


    LocalBufferLength = (YORI_ALLOC_SIZE_T)ClipboardBufferLength;
    LocalBuffer = YoriLibReferencedMalloc(LocalBufferLength);
    if (LocalBuffer == NULL) {
        DllKernel32.pGlobalUnlock(hMem);
        DllUser32.pCloseClipboard();
        return FALSE;
    }

    pMem = pMem + sizeof(DWORD);
    memcpy(LocalBuffer, pMem, LocalBufferLength);
    DllKernel32.pGlobalUnlock(hMem);
    DllUser32.pCloseClipboard();

    *Buffer = LocalBuffer;
    *BufferLength = LocalBufferLength;

    return TRUE;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1500) && (_MSC_VER <= 1600)
#pragma warning(pop)
#endif

/**
 Returns TRUE to indicate the system clipboard is available.  The clipboard is
 not available on Nano Server where user32.dll is not present.

 @return TRUE if the system clipboard is available, FALSE if it is not.
 */
BOOLEAN
YoriLibIsSystemClipboardAvailable(VOID)
{
    YoriLibLoadUser32Functions();

    if (DllUser32.pOpenClipboard == NULL ||
        DllUser32.pEmptyClipboard == NULL ||
        DllUser32.pGetClipboardData == NULL ||
        DllUser32.pSetClipboardData == NULL ||
        DllUser32.pCloseClipboard == NULL) {

        return FALSE;
    }

    return TRUE;
}

/**
 A process wide buffer to use as a clipboard in the event that the system
 clipboard is not available.  Note this is one buffer so it can only
 describe on format, being unicode text.
 */
YORI_STRING YoriLibProcessClipboard;

/**
 Empty the process clipboard.  This is used on process termination to free
 memory since the process clipboard is inaccessible after process termination
 anyway.
 */
VOID
YoriLibEmptyProcessClipboard(VOID)
{
    YoriLibFreeStringContents(&YoriLibProcessClipboard);
}

/**
 Copy a Yori string into the clipboard in text format only.  If the system
 clipboard is not available (on Nano) use a process-global buffer to act
 as a clipboard.

 @param Buffer The string to populate into the clipboard.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibCopyTextWithProcessFallback(
    __in PYORI_STRING Buffer
    )
{
    if (YoriLibIsSystemClipboardAvailable()) {
        return YoriLibCopyText(Buffer);
    }

    if (Buffer->LengthInChars > YoriLibProcessClipboard.LengthAllocated) {
        if (!YoriLibReallocateStringWithoutPreservingContents(&YoriLibProcessClipboard, Buffer->LengthInChars)) {
            return FALSE;
        }
    }

    memcpy(YoriLibProcessClipboard.StartOfString, Buffer->StartOfString, Buffer->LengthInChars * sizeof(TCHAR));
    YoriLibProcessClipboard.LengthInChars = Buffer->LengthInChars;
    return TRUE;
}

/**
 Retrieve any text from the clipboard and output it into a Yori string.  If
 the system clipboard is not available (on Nano) use a process-global buffer
 to act as a clipboard.

 @param Buffer The string to populate with the contents of the clipboard.
        If the string does not contain a large enough buffer to contain the
        clipboard contents, it is reallocated.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibPasteTextWithProcessFallback(
    __inout PYORI_STRING Buffer
    )
{
    if (YoriLibIsSystemClipboardAvailable()) {
        return YoriLibPasteText(Buffer);
    }

    if (YoriLibProcessClipboard.LengthInChars + 1 > Buffer->LengthAllocated) {
        if (!YoriLibReallocateStringWithoutPreservingContents(Buffer, (YORI_ALLOC_SIZE_T)(YoriLibProcessClipboard.LengthInChars + 1))) {
            return FALSE;
        }
    }

    memcpy(Buffer->StartOfString, YoriLibProcessClipboard.StartOfString, YoriLibProcessClipboard.LengthInChars * sizeof(TCHAR));


    Buffer->LengthInChars = YoriLibProcessClipboard.LengthInChars;
    Buffer->StartOfString[Buffer->LengthInChars] = '\0';

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

    return TRUE;
}

// vim:sw=4:ts=4:et:
