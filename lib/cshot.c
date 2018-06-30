/**
 * @file lib/cshot.c
 *
 * Yori lib capture console text and reformat it
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
 * FITNESS CSHOT A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE CSHOT ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>


/**
 Read contents from the console window and send the contents to a device.

 @param hTarget Handle to the target device.  Can be a file or standard output.

 @param LineCount Specifies the number of lines to capture.  If zero, the entire
        buffer is captured.

 @param SkipCount Specifies the number of lines to skip.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibRewriteConsoleContents(
    __in HANDLE hTarget,
    __in DWORD LineCount,
    __in DWORD SkipCount
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    HANDLE hConsole;
    SMALL_RECT ReadWindow;
    PCHAR_INFO ReadBuffer;
    COORD ReadBufferSize;
    COORD ReadBufferOffset;
    DWORD LastAttribute;
    WORD LineIndex;

    hConsole = CreateFile(_T("CONOUT$"), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (hConsole == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (!GetConsoleScreenBufferInfo(hConsole, &ScreenInfo)) {
        CloseHandle(hConsole);
        return FALSE;
    }

    if (LineCount == 0 || LineCount > (DWORD)ScreenInfo.dwCursorPosition.Y) {
        LineCount = ScreenInfo.dwCursorPosition.Y;
    }

    if (SkipCount >= (DWORD)ScreenInfo.dwCursorPosition.Y) {
        CloseHandle(hConsole);
        return FALSE;
    }

    if (LineCount + SkipCount > (DWORD)ScreenInfo.dwCursorPosition.Y) {
        LineCount = ScreenInfo.dwCursorPosition.Y - SkipCount;
    }

    if (LineCount == 0) {
        CloseHandle(hConsole);
        return FALSE;
    }

    ReadWindow.Left = 0;
    ReadWindow.Right = (WORD)(ScreenInfo.dwSize.X - 1);
    ReadWindow.Top = (WORD)(ScreenInfo.dwCursorPosition.Y - SkipCount - LineCount);
    ReadWindow.Bottom = (WORD)(ScreenInfo.dwCursorPosition.Y - SkipCount - 1);

    ReadBufferSize.X = (WORD)(ReadWindow.Right - ReadWindow.Left + 1);
    ReadBufferSize.Y = (WORD)(ReadWindow.Bottom - ReadWindow.Top + 1);

    ReadBuffer = YoriLibMalloc(ReadBufferSize.X * ReadBufferSize.Y * sizeof(CHAR_INFO));
    if (ReadBuffer == NULL) {
        CloseHandle(hConsole);
        return FALSE;
    }

    ReadBufferOffset.X = 0;
    ReadBufferOffset.Y = 0;

    //
    //  ReadConsoleOutput fails if it's given a large request, so give it
    //  a pile of small (one line) requests.
    //

    for (LineIndex = 0; LineIndex < (WORD)ReadBufferSize.Y; LineIndex++) {
        SMALL_RECT LineReadWindow;
        COORD LineReadBufferSize;

        LineReadWindow.Left = ReadWindow.Left;
        LineReadWindow.Right = ReadWindow.Right;
        LineReadWindow.Top = (WORD)(ReadWindow.Top + LineIndex);
        LineReadWindow.Bottom = LineReadWindow.Top;

        LineReadBufferSize.X = ReadBufferSize.X;
        LineReadBufferSize.Y = 1;

        if (!ReadConsoleOutput(hConsole, &ReadBuffer[LineIndex * ReadBufferSize.X], LineReadBufferSize, ReadBufferOffset, &LineReadWindow)) {
            CloseHandle(hConsole);
            YoriLibFree(ReadBuffer);
            return FALSE;
        }
    }

    YoriLibVtSetConsoleTextAttributeOnDevice(hTarget, 0, ReadBuffer[0].Attributes);
    LastAttribute = ReadBuffer[0].Attributes;

    for (ReadBufferOffset.Y = 0; ReadBufferOffset.Y < ReadBufferSize.Y; ReadBufferOffset.Y++) {
        DWORD CurrentMode;

        for (ReadBufferOffset.X = 0; ReadBufferOffset.X < ReadBufferSize.X; ReadBufferOffset.X++) {
            DWORD CellIndex;

            CellIndex = ReadBufferOffset.Y * ReadBufferSize.X + ReadBufferOffset.X;
            
            if (ReadBuffer[CellIndex].Attributes != LastAttribute) {
                YoriLibVtSetConsoleTextAttributeOnDevice(hTarget, 0, ReadBuffer[CellIndex].Attributes);
                LastAttribute = ReadBuffer[CellIndex].Attributes;
            }
            YoriLibOutputToDevice(hTarget, 0, _T("%c"), ReadBuffer[CellIndex].Char.UnicodeChar);
        }
        if (!GetConsoleMode(hTarget, &CurrentMode)) {
            YoriLibOutputToDevice(hTarget, 0, _T("\n"));
        }
    }

    YoriLibFree(ReadBuffer);
    return TRUE;
}

/**
 Convert a two dimensional array of characters and attributes into a single
 VT100 stream that describes the characters and attributes.

 @param String On successful completion, updated to contain the VT100 string.
        This string may be reallocated within this routine.

 @param BufferSize Specifies the dimensions of the array.  Both buffers are
        expected to contain X*Y elements.

 @param CharBuffer Pointer to the buffer containing text.

 @param AttrBuffer Pointer to the buffer containing attributes.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGenerateVtStringFromConsoleBuffers(
    __inout PYORI_STRING String,
    __in COORD BufferSize,
    __in LPWSTR CharBuffer,
    __in PWORD AttrBuffer
    )
{
    DWORD BufferSizeNeeded;
    SHORT LineIndex;
    SHORT CharIndex;
    WORD LastAttribute;
    TCHAR EscapeStringBuffer[sizeof("E[0;999;999;1m")];
    YORI_STRING EscapeString;

    //
    //  A temporary string to hold a single Vt escape sequence
    //

    YoriLibInitEmptyString(&EscapeString);
    EscapeString.StartOfString = EscapeStringBuffer;
    EscapeString.LengthAllocated = sizeof(EscapeStringBuffer)/sizeof(EscapeStringBuffer[0]);

    //
    //  We'll need a buffer that's at least big enough to hold all the text
    //  with some newlines and a terminator
    //

    BufferSizeNeeded = (BufferSize.X + 2) * BufferSize.Y + 1;

    //
    //  Now go through the attributes and add in the size of any escapes
    //  needed
    //

    LastAttribute = AttrBuffer[0];
    YoriLibVtStringForTextAttribute(&EscapeString, LastAttribute);
    BufferSizeNeeded += EscapeString.LengthInChars;

    for (LineIndex = 0; LineIndex < BufferSize.Y; LineIndex++) {
        for (CharIndex = 0; CharIndex < BufferSize.X; CharIndex++) {
            if (AttrBuffer[LineIndex * BufferSize.X + CharIndex] != LastAttribute) {
                LastAttribute = AttrBuffer[LineIndex * BufferSize.X + CharIndex];
                YoriLibVtStringForTextAttribute(&EscapeString, LastAttribute);
                BufferSizeNeeded += EscapeString.LengthInChars;
            }
        }
    }

    //
    //  Allocate a buffer of sufficient size if it's not allocated already
    //

    if (String->LengthAllocated < BufferSizeNeeded) {
        YoriLibFreeStringContents(String);
        if (!YoriLibAllocateString(String, BufferSizeNeeded)) {
            return FALSE;
        }
    }

    //
    //  Go through again populating both text and escapes into the output
    //  buffer
    //

    String->LengthInChars = 0;
    LastAttribute = AttrBuffer[0];
    YoriLibVtStringForTextAttribute(&EscapeString, LastAttribute);

    memcpy(&String->StartOfString[String->LengthInChars], EscapeString.StartOfString, EscapeString.LengthInChars * sizeof(TCHAR));
    String->LengthInChars += EscapeString.LengthInChars;

    for (LineIndex = 0; LineIndex < BufferSize.Y; LineIndex++) {
        for (CharIndex = 0; CharIndex < BufferSize.X; CharIndex++) {
            if (AttrBuffer[LineIndex * BufferSize.X + CharIndex] != LastAttribute) {
                LastAttribute = AttrBuffer[LineIndex * BufferSize.X + CharIndex];
                YoriLibVtStringForTextAttribute(&EscapeString, LastAttribute);
                memcpy(&String->StartOfString[String->LengthInChars], EscapeString.StartOfString, EscapeString.LengthInChars * sizeof(TCHAR));
                String->LengthInChars += EscapeString.LengthInChars;
            }
            String->StartOfString[String->LengthInChars] = CharBuffer[LineIndex * BufferSize.X + CharIndex];
            String->LengthInChars++;
        }

        String->StartOfString[String->LengthInChars] = '\r';
        String->StartOfString[String->LengthInChars + 1] = '\n';
        String->LengthInChars += 2;
    }

    ASSERT(EscapeString.StartOfString == EscapeStringBuffer);

    return TRUE;
}

// vim:sw=4:ts=4:et:
