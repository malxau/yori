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
const CHAR DummyHeader[] = 
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
const CHAR DummyFragStart[] = 
                  "<!--StartFragment-->";

/**
 The length of the fragment start string, in bytes.
 */
#define HTMLCLIP_FRAGSTART_SIZE (sizeof(DummyFragStart)-1)

/**
 A string indicating the end of a fragment.
 Dummy because it is only used to count its size, not used as a string.
 */
const CHAR DummyFragEnd[] = 
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

    Return = WideCharToMultiByte(EncodingToUse, 0, TextToCopy->StartOfString, TextToCopy->LengthInChars, pMem + HTMLCLIP_HDR_SIZE + HTMLCLIP_FRAGSTART_SIZE, UserBytes, NULL, NULL);
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

// vim:sw=4:ts=4:et:
