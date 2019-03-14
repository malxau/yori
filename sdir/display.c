/**
 * @file sdir/display.c
 *
 * Colorful, sorted and optionally rich directory enumeration
 * for Windows.
 *
 * This module implements low level routines to display formatted text to the
 * console or to files, in unicode or ansi as desired, pausing if too much
 * output has occurred, etc.
 *
 * Copyright (c) 2014-2018 Malcolm J. Smith
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

#include "sdir.h"

//
//  Display support
//

/**
 Write a specified number of characters to the output device.

 @param hConsole Handle to the output device.

 @param OutputString An array of characters to write to the output device.

 @param Length The number of characters in the array to write.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirWriteRawStringToOutputDevice(
    __in HANDLE hConsole,
    __in LPCTSTR OutputString,
    __in DWORD Length
    )
{
    YORI_STRING String;
    YoriLibInitEmptyString(&String);
    String.StartOfString = (LPTSTR)OutputString;
    String.LengthInChars = Length;

    return YoriLibOutputString(hConsole, 0, &String);
}

/**
 The most recently written attribute to the output.  This is used to ensure
 we only sent VT escapes to a device when the color is actually changing.
 */
YORILIB_COLOR_ATTRIBUTES SdirCurrentAttribute = {0};

/**
 Sent a VT escape sequence to the output device if the new color is
 different to the previous one.

 @param hConsole Handle to the output device.

 @param Attribute The new color to set.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirSetConsoleTextAttribute(
    __in HANDLE hConsole,
    __in YORILIB_COLOR_ATTRIBUTES Attribute
    )
{
    if (YoriLibAreColorsIdentical(Attribute, SdirCurrentAttribute)) {
        return TRUE;
    }

    SdirCurrentAttribute = Attribute;

    return YoriLibVtSetConsoleTextAttributeOnDevice(hConsole, 0, 0, Attribute.Win32Attr);
}

/**
 Write a string of characters with color attribute information to the current
 output device.

 @param str Pointer to the string of characters to write.

 @param count Number of characters to write.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirWrite (
    __in_ecount(count) PSDIR_FMTCHAR str,
    __in DWORD count
    )
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD i, j;

    YORILIB_COLOR_ATTRIBUTES CacheAttr = SdirDefaultColor;
    TCHAR CharCache[64];

    //
    //  We firstly load as much as we can into a stack buffer, then write it
    //  out.  Those syscalls are expensive - run in a slow VM to see.  We
    //  have to write out the buffer either when the next character has 
    //  different color (so we can't use the same WriteConsole call) or when
    //  the stack buffer is full.
    //

    j = 0;
    for (i = 0; i < count; i++) { 

        CacheAttr = str[i].Attr;
        CharCache[j] = str[i].Char;
        j++;

        if (( i + 1 < count && !YoriLibAreColorsIdentical(str[i + 1].Attr, CacheAttr)) || (j >= sizeof(CharCache)/sizeof(CharCache[0]))) {

            SdirSetConsoleTextAttribute(hConsole, CacheAttr);
            SdirWriteRawStringToOutputDevice(hConsole, CharCache, j);
            j = 0;
        }
    }

    //
    //  If we have anything left, flush it now.
    //

    if (j > 0) {

        SdirSetConsoleTextAttribute(hConsole, CacheAttr);
        SdirWriteRawStringToOutputDevice(hConsole, CharCache, j);
    }

    return TRUE;
}

/**
 Count of the number of lines written by this application.  Used to determine
 when "Press any key" should be displayed.
 */
DWORD SdirWriteStringLinesDisplayed = 0;

/**
 Write a NULL terminated string with a single color attribute to the output
 device.

 @param str Pointer to the NULL terminated string.

 @param DefaultAttribute The character color attribute to display.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirWriteStringWithAttribute (
    __in LPCTSTR str,
    __in YORILIB_COLOR_ATTRIBUTES DefaultAttribute
    )
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD LinesInBuffer = 0;
    DWORD TCharsInBuffer = 0;
    DWORD OffsetOfLastLineBreak;
    BOOLEAN HitLineLimit = FALSE;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    SdirSetConsoleTextAttribute(hConsole, DefaultAttribute);

    if (Opts->OutputHasAutoLineWrap) {

        GetConsoleScreenBufferInfo(hConsole, &ScreenInfo);

        while (str[TCharsInBuffer] != '\0') {
            OffsetOfLastLineBreak = 0;
            while (str[TCharsInBuffer] != '\0') {
                if (str[TCharsInBuffer] == '\n') {
                    LinesInBuffer++;
                    OffsetOfLastLineBreak = TCharsInBuffer;
                } else if (TCharsInBuffer + ScreenInfo.dwCursorPosition.X - OffsetOfLastLineBreak >= Opts->ConsoleBufferWidth) {
                    LinesInBuffer++;
                    ScreenInfo.dwCursorPosition.X = 0;
                    OffsetOfLastLineBreak = TCharsInBuffer;
                }
                TCharsInBuffer++;
                if (SdirWriteStringLinesDisplayed + LinesInBuffer >= Opts->ConsoleHeight - 1) {
                    HitLineLimit = TRUE;
                    break;
                }
            }

            SdirWriteStringLinesDisplayed += LinesInBuffer;

            SdirWriteRawStringToOutputDevice(hConsole, str, TCharsInBuffer);

            LinesInBuffer = 0;
            str += TCharsInBuffer;
            TCharsInBuffer = 0;

            //
            //  This is screwy because we call it and it calls back into us.
            //  It works because we've just reset lines displayed to zero
            //  and we "know" we have enough space to display it.  Just in
            //  case it uses different attributes, reset those now.
            //

            if (HitLineLimit) {
                SdirWriteStringLinesDisplayed = 0;
                if (Opts->EnablePause && !SdirPressAnyKey()) {
                    return FALSE;
                }
                SdirSetConsoleTextAttribute(hConsole, DefaultAttribute);
                HitLineLimit = FALSE;
            }
        }

    } else {

        SdirSetConsoleTextAttribute(hConsole, DefaultAttribute);
        SdirWriteRawStringToOutputDevice(hConsole, str, (DWORD)_tcslen(str));
    }

    return TRUE;
}

/**
 Indicate that a row of text has been displayed external to the display
 package to ensure that the count of rows displayed remains accurate, 
 and check if "Press any key" processing is needed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirRowDisplayed()
{
    if (Opts->EnablePause) {
        SdirWriteStringLinesDisplayed++;

        if (SdirWriteStringLinesDisplayed >= Opts->ConsoleHeight - 1) {

            SdirWriteStringLinesDisplayed = 0;
            if (!SdirPressAnyKey()) {
                return FALSE;
            }
            SdirWriteStringLinesDisplayed = 0;
        }
    }
    return TRUE;
}

/**
 Copy an array of characters with color information into a format character
 array where each character is recorded with its color.  Ensure that the
 amount of characters written is equal to a certain value, so that if the
 source string is shorter the destination buffer is populated with a constant
 number of characters.

 @param str The format string to populate with characters specifying color
        information.

 @param src An array of characters to populate into the formatted string.

 @param attr The color information to write for each of these characters.

 @param count The number of characters to populate from src.

 @param padsize The number of characters to write to the destination.  If
        more than count, the target string is padded with spaces with the
        same color information.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirPasteStrAndPad (
    __out_ecount(padsize) PSDIR_FMTCHAR str,
    __in_ecount(count) LPTSTR src,
    __in YORILIB_COLOR_ATTRIBUTES attr,
    __in DWORD count,
    __in DWORD padsize
    )
{
    ULONG i;

    for (i = 0; i < count && i < padsize; i++) {
        str[i].Char = src[i];
        str[i].Attr = attr;
    }

    for (; i < padsize; i++) {
        str[i].Char = ' ';
        str[i].Attr = attr;
    }

    return TRUE;
}


/**
 Display a Win32 error string with a prefix string describing the context of
 the error.

 @param ErrorCode The Win32 error code to display the error string for.

 @param Prefix A NULL terminated prefix string describing the context of the
        error.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirDisplayError (
    __in LONG ErrorCode,
    __in_opt LPCTSTR Prefix
    )
{
    LPTSTR OutputBuffer;

    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ErrorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&OutputBuffer,
            0,
            NULL)) {

        if (Prefix) {
            SdirWriteStringWithAttribute(Prefix, Opts->FtError.HighlightColor);
            SdirWriteStringWithAttribute(_T(": "), Opts->FtError.HighlightColor);
        }
        if (!SdirWriteStringWithAttribute(OutputBuffer, Opts->FtError.HighlightColor)) {
            LocalFree((PVOID)OutputBuffer);
            return FALSE;
        }
        LocalFree((PVOID)OutputBuffer);
    }
    return TRUE;
}

/**
 Display a Win32 error string with a prefix string describing the context of
 the error.

 @param ErrorCode The Win32 error code to display the error string for.

 @param YsPrefix A prefix string describing the context of the error.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirDisplayYsError (
    __in LONG ErrorCode,
    __in PYORI_STRING YsPrefix
    )
{
    LPTSTR OutputBuffer;
    YORI_STRING EntireMsg;
    BOOL RetVal = FALSE;

    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ErrorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&OutputBuffer,
            0,
            NULL)) {

        YoriLibInitEmptyString(&EntireMsg);
        YoriLibYPrintf(&EntireMsg, _T("%y: %s"), YsPrefix, OutputBuffer);
        if (EntireMsg.StartOfString != NULL) {
            SdirWriteStringWithAttribute(EntireMsg.StartOfString, Opts->FtError.HighlightColor);
            YoriLibFreeStringContents(&EntireMsg);
            RetVal = TRUE;
        }
        LocalFree((PVOID)OutputBuffer);
    }
    return RetVal;
}

/**
 Tell the user to press a key to continue outputting information, and wait
 for the resulting key press.

 @return TRUE to indicate the application should continue processing and
         generating more output, FALSE to indicate the application should
         terminate.
 */
BOOL
SdirPressAnyKey ()
{
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD InputBuffer;
    DWORD NumRead;

    SdirWriteString(_T("Press any key to continue..."));

    //
    //  Loop throwing away events until we get a key pressed
    //

    while (ReadConsoleInput(hConsole, &InputBuffer, 1, &NumRead) &&
           NumRead > 0 &&
           (InputBuffer.EventType != KEY_EVENT ||
            !InputBuffer.Event.KeyEvent.bKeyDown ||
            InputBuffer.Event.KeyEvent.uChar.UnicodeChar == '\0'));

    if (InputBuffer.Event.KeyEvent.uChar.UnicodeChar == 'q' ||
        InputBuffer.Event.KeyEvent.uChar.UnicodeChar == 'Q') {

        return FALSE;
    }

    //
    //  Keep throwing away any remaining events so we're not left
    //  with stale events next time we run
    //

    while (PeekConsoleInput(hConsole, &InputBuffer, 1, &NumRead) &&
           NumRead > 0) {

        ReadConsoleInput(hConsole, &InputBuffer, 1, &NumRead);
    }

    SdirWriteString(_T("\n"));
    SdirWriteStringLinesDisplayed = 0;
    return TRUE;
}

// vim:sw=4:ts=4:et:
