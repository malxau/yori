/**
 * @file sh/clip.c
 *
 * Basic clipboard support for the shell.
 *
 * Copyright (c) 2015-2017 Malcolm J. Smith
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

#include "yori.h"

/**
 Retrieve any text from the clipboard and output it into a Yori string.

 @param Buffer The string to populate with the contents of the clipboard.
        If the string does not contain a large enough buffer to contain the
        clipboard contents, it is reallocated.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShPasteText(
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

// vim:sw=4:ts=4:et:
