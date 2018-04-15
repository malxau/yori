/**
 * @file cvtvt/html.c
 *
 * Convert VT100/ANSI escape sequences into HTML.
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

#include "cvtvt.h"


/**
 The default color to use when all else fails.
 */
#define CVTVT_DEFAULT_COLOR (7)

/**
 The dialect of HTML to use.
 */
DWORD CvtvtHtmlVersion = 5;

/**
 While parsing, TRUE if a tag is currently open so that it can be closed on
 the next font change.
 */
BOOLEAN CvtvtTagOpen = FALSE;

/**
 While parsing, TRUE if underlining is in effect.
 */
BOOLEAN CvtvtUnderlineOn = FALSE;

/**
 On systems that support it, populated with font information from the console
 so the output display can match the input.
 */
YORI_CONSOLE_FONT_INFOEX CvtvtFontInfo = {0};

/**
 Attempt to capture the current console font.  This is only available on
 newer systems.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtCaptureConsoleFont()
{
    HANDLE hConsole;
    HMODULE hKernel;
    PGET_CURRENT_CONSOLE_FONT_EX GetCurrentConsoleFontExFn;

    hKernel = GetModuleHandle(_T("KERNEL32"));
    if (hKernel == NULL) {
        return FALSE;
    }

    GetCurrentConsoleFontExFn = (PGET_CURRENT_CONSOLE_FONT_EX)GetProcAddress(hKernel, "GetCurrentConsoleFontEx");
    if (GetCurrentConsoleFontExFn == NULL) {
        return FALSE;
    }

    hConsole = CreateFile(_T("CONOUT$"),
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          NULL,
                          OPEN_EXISTING,
                          0,
                          NULL);

    if (hConsole == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    CvtvtFontInfo.cbSize = sizeof(CvtvtFontInfo);

    if (!GetCurrentConsoleFontExFn(hConsole, FALSE, &CvtvtFontInfo)) {
        CloseHandle(hConsole);
        return FALSE;
    }

    CloseHandle(hConsole);
    return TRUE;
}

/**
 Output to include at the beginning of any HTML stream.
 */
CONST TCHAR CvtvtHtmlHeader[] = _T("<HTML><HEAD><TITLE>cvtvt output</TITLE></HEAD>");

/**
 Output to include at the beginning of any version 4 HTML body.
 */
CONST TCHAR CvtvtVer4Header[] = _T("<BODY>")
                                _T("<DIV STYLE=\"background-color: #000000; font-family: ");

/**
 Output to include at the beginning of any version 5 HTML body.
 */
CONST TCHAR CvtvtVer5Header[] = _T("<BODY><DIV STYLE=\"background-color: #000000; color: #c0c0c0; border-style: ridge; border-width: 4px; display: inline-block; font-family: ");

/**
 The end of the HTML body section, after font information has been populated.
 This is used for version 4 and 5.
 */
CONST TCHAR CvtvtVerHeaderEnd[] = _T(";\">");

/**
 Indicate the beginning of a stream and perform any initial output.

 @param hOutput The stream to output any footer information to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtHtmlInitializeStream(
    __in HANDLE hOutput
    )
{
    TCHAR szFontNames[LF_FACESIZE + 100];
    TCHAR szFontSize[100];

    CvtvtCaptureConsoleFont();

    if (CvtvtFontInfo.FaceName[0] != '\0') {
        YoriLibSPrintf(szFontNames, _T("'%ls', monospace"), CvtvtFontInfo.FaceName);
    } else {
        YoriLibSPrintf(szFontNames, _T("monospace"));
    }

    if (CvtvtFontInfo.FontWeight == 0) {
        CvtvtFontInfo.FontWeight = 700;
    }
    if (CvtvtFontInfo.dwFontSize.Y == 0) {
        CvtvtFontInfo.dwFontSize.Y = 12;
    }

    YoriLibOutputTextToMultibyteDevice(hOutput, CvtvtHtmlHeader, sizeof(CvtvtHtmlHeader)/sizeof(CvtvtHtmlHeader[0]) - 1);
    YoriLibSPrintf(szFontSize, _T("; font-size: %ipx"), CvtvtFontInfo.dwFontSize.Y);
    if (CvtvtHtmlVersion == 4) {
        YoriLibOutputTextToMultibyteDevice(hOutput, CvtvtVer4Header, sizeof(CvtvtVer4Header)/sizeof(CvtvtVer4Header[0]) - 1);
        YoriLibOutputTextToMultibyteDevice(hOutput, szFontNames, (DWORD)_tcslen(szFontNames));
        YoriLibOutputTextToMultibyteDevice(hOutput, szFontSize, (DWORD)_tcslen(szFontSize));
        YoriLibOutputTextToMultibyteDevice(hOutput, CvtvtVerHeaderEnd, sizeof(CvtvtVerHeaderEnd)/sizeof(CvtvtVerHeaderEnd[0]) - 1);
        if (CvtvtFontInfo.FontWeight >= 600) {
            YoriLibOutputTextToMultibyteDevice(hOutput, _T("<B>"), sizeof("<B>") - 1);
        }
    } else {
        TCHAR szFontWeight[100];
        YoriLibSPrintf(szFontWeight, _T("; font-weight: %i"), CvtvtFontInfo.FontWeight);
        YoriLibOutputTextToMultibyteDevice(hOutput, CvtvtVer5Header, sizeof(CvtvtVer5Header)/sizeof(CvtvtVer5Header[0]) - 1);
        YoriLibOutputTextToMultibyteDevice(hOutput, szFontNames, (DWORD)_tcslen(szFontNames));
        YoriLibOutputTextToMultibyteDevice(hOutput, szFontWeight, (DWORD)_tcslen(szFontWeight));
        YoriLibOutputTextToMultibyteDevice(hOutput, szFontSize, (DWORD)_tcslen(szFontSize));
        YoriLibOutputTextToMultibyteDevice(hOutput, CvtvtVerHeaderEnd, sizeof(CvtvtVerHeaderEnd)/sizeof(CvtvtVerHeaderEnd[0]) - 1);
    }
    return TRUE;
}

/**
 Final text to output at the end of any HTML stream.
 */
CONST TCHAR CvtvtFooter[] = _T("</DIV></BODY></HTML>");

/**
 Indicate the end of the stream has been reached and perform any final
 output.

 @param hOutput The stream to output any footer information to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtHtmlEndStream(
    __in HANDLE hOutput
    )
{
    if (CvtvtTagOpen) {
        if (CvtvtUnderlineOn) {
            YoriLibOutputTextToMultibyteDevice(hOutput, _T("</U>"), 4);
        }
        if (CvtvtHtmlVersion == 4) {
            YoriLibOutputTextToMultibyteDevice(hOutput, _T("</FONT>"), 7);
        } else {
            YoriLibOutputTextToMultibyteDevice(hOutput, _T("</SPAN>"), 7);
        }
    }

    if (CvtvtHtmlVersion == 4) {
        if (CvtvtFontInfo.FontWeight >= 600) {
            YoriLibOutputTextToMultibyteDevice(hOutput, _T("</B>"), 4);
        }
    }
    YoriLibOutputTextToMultibyteDevice(hOutput, CvtvtFooter, sizeof(CvtvtFooter)/sizeof(CvtvtFooter[0]) - 1);

    return TRUE;
}

/**
 Parse text between VT100 escape sequences and generate correct output for
 either HTML4 or HTML5.

 @param hOutput The output stream to populate with the translated text
        information.

 @param StringBuffer Pointer to a string buffer containing the text to
        process.

 @param BufferLength Indicates the number of characters in StringBuffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtHtmlProcessAndOutputText(
    __in HANDLE hOutput,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    TCHAR  LookFor[] = {'<', '>', ' ', '\n', '\r', '\0'};
    LPTSTR CurrentPoint;
    DWORD  CurrentOffset = 0;
    DWORD  PreviouslyConsumed = 0;
    YORI_STRING SearchString;

    //
    //  Scan through the string again looking for text that needs to be
    //  escaped in HTML, in particular greater than and less than since
    //  those denote tags
    //

    CurrentPoint = StringBuffer;

    do {
        SearchString.StartOfString = CurrentPoint;
        SearchString.LengthInChars = BufferLength - PreviouslyConsumed;
        CurrentOffset = YoriLibCountStringNotContainingChars(&SearchString, LookFor);
        if (CurrentOffset > 0) {
            YoriLibOutputTextToMultibyteDevice(hOutput, CurrentPoint, CurrentOffset);

            CurrentPoint = CurrentPoint + CurrentOffset;
            PreviouslyConsumed += CurrentOffset;
        }

        if (PreviouslyConsumed < BufferLength) {

            //
            //  Escape things that could be tags, and parse escape sequences
            //  we understand
            //
    
            if (*CurrentPoint == '<') {
                YoriLibOutputTextToMultibyteDevice(hOutput, _T("&lt;"), 4);
                CurrentPoint++;
                PreviouslyConsumed++;
            } else if (*CurrentPoint == '>') {
                YoriLibOutputTextToMultibyteDevice(hOutput, _T("&gt;"), 4);
                CurrentPoint++;
                PreviouslyConsumed++;
            } else if (*CurrentPoint == '\n') {
                YoriLibOutputTextToMultibyteDevice(hOutput, _T("<BR>"), 4);
                CurrentPoint++;
                PreviouslyConsumed++;
            } else if (*CurrentPoint == ' ') {
                YoriLibOutputTextToMultibyteDevice(hOutput, _T("&nbsp;"), 6);
                CurrentPoint++;
                PreviouslyConsumed++;
            } else if (*CurrentPoint == '\r') {
                CurrentPoint++;
                PreviouslyConsumed++;
            }
        }

    } while (PreviouslyConsumed < BufferLength);

    return TRUE;
}

/**
 Parse a VT100 escape sequence and generate the correct output for either
 HTML4 or HTML5.

 @param hOutput The output stream to populate with the translated escape
        information.

 @param StringBuffer Pointer to a string buffer containing the escape to
        process.

 @param BufferLength Indicates the number of characters in StringBuffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtHtmlProcessAndOutputEscape(
    __in HANDLE hOutput,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    LPTSTR CurrentPoint = StringBuffer;
    DWORD  CurrentOffset = 0;
    WORD   NewColor = CVTVT_DEFAULT_COLOR;
    DWORD  RemainingLength;
    YORI_STRING SearchString;

    RemainingLength = BufferLength;

    //
    //  We expect an escape initiator (two chars) and a 'm' for color
    //  formatting.  This should already be validated, the check here
    //  is redundant.
    //

    if (BufferLength >= 3 &&
        CurrentPoint[BufferLength - 1] == 'm') {

        DWORD code;
        TCHAR NewTag[128];
        BOOLEAN NewUnderline = FALSE;
        DWORD ColorTable[] = {0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0xc0c0c0,
                              0x808080, 0xff0000, 0x00ff00, 0xffff00, 0x0000ff, 0xff00ff, 0x00ffff, 0xffffff};

        CurrentPoint++;
        RemainingLength--;

        //
        //  For something manipulating colors, go through the semicolon
        //  delimited list and apply the changes to the current color
        //

        while (TRUE) {

            //
            //  Swallow either the ';' or '[' depending on whether we're
            //  at the start or an intermediate component
            //

            CurrentPoint++;
            RemainingLength--;

            SearchString.StartOfString = CurrentPoint;
            SearchString.LengthInChars = RemainingLength;
            code = YoriLibDecimalStringToInt(&SearchString);
            if (code == 0) {
                NewColor = CVTVT_DEFAULT_COLOR;
                NewUnderline = FALSE;
            } else if (code == 1) {
                NewColor |= 8;
            } else if (code == 4) {
                NewUnderline = TRUE;
            } else if (code == 7) {
                NewColor = (WORD)(((NewColor & 0xf) << 4) | ((NewColor & 0xf0) >> 4));
            } else if (code == 39) {
                NewColor = (WORD)((NewColor & ~(0xf)) | (CVTVT_DEFAULT_COLOR & 0xf));
            } else if (code == 49) {
                NewColor = (WORD)((NewColor & ~(0xf0)) | (CVTVT_DEFAULT_COLOR & 0xf0));
            } else if (code >= 30 && code <= 37) {
                NewColor = (WORD)((NewColor & ~(0xf)) | (code - 30));
            } else if (code >= 40 && code <= 47) {
                NewColor = (WORD)((NewColor & ~(0xf0)) | ((code - 40)<<4));
            } else if (code >= 90 && code <= 97) {
                NewColor = (WORD)((NewColor & ~(0xf)) | 0x8 | (code - 90));
            } else if (code >= 100 && code <= 107) {
                NewColor = (WORD)((NewColor & ~(0xf0)) | 0x80 | ((code - 100)<<4));
            }

            //
            //  We expect this to end with an 'm' at the end of the string, or
            //  some other delimiter in the middle.  For paranoia maintain
            //  length checking.
            //

            YoriLibInitEmptyString(&SearchString);
            SearchString.StartOfString = CurrentPoint;
            SearchString.LengthInChars = RemainingLength;
            CurrentOffset = YoriLibCountStringContainingChars(&SearchString, _T("0123456789"));

            CurrentPoint += CurrentOffset;
            RemainingLength -= CurrentOffset;

            if (RemainingLength == 0 ||
                *CurrentPoint != ';') {
                
                break;
            }
        }

        if (CvtvtTagOpen) {
            if (CvtvtUnderlineOn) {
                YoriLibOutputTextToMultibyteDevice(hOutput, _T("</U>"), 4);
            }
            if (CvtvtHtmlVersion == 4) {
                YoriLibOutputTextToMultibyteDevice(hOutput, _T("</FONT>"), 7);
            } else {
                YoriLibOutputTextToMultibyteDevice(hOutput, _T("</SPAN>"), 7);
            }
        }

        //
        //  Output the appropriate tag depending on the version the user wanted
        //

        if (CvtvtHtmlVersion == 4) {
            YoriLibSPrintf(NewTag,
                           _T("<FONT COLOR=#%06x>"),
                           (int)ColorTable[NewColor & 0xf]);
        } else {
            YoriLibSPrintf(NewTag,
                           _T("<SPAN STYLE=\"color:#%06x;background-color:#%06x\">"),
                          (int)ColorTable[NewColor & 0xf],
                          (int)ColorTable[(NewColor & 0xf0) >> 4]);
        }

        YoriLibOutputTextToMultibyteDevice(hOutput, NewTag, (DWORD)_tcslen(NewTag));

        if (NewUnderline) {
            YoriLibOutputTextToMultibyteDevice(hOutput, _T("<U>"), 3);
        }
        CvtvtUnderlineOn = NewUnderline;
        CvtvtTagOpen = TRUE;
    }
    return TRUE;
}

/**
 Set parsing functions to generate HTML4 output.

 @param CallbackFunctions Function pointers to populate with those that can
        generate HTML4 output.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtHtml4SetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    )
{
    CvtvtHtmlVersion = 4;
    CallbackFunctions->InitializeStream = CvtvtHtmlInitializeStream;
    CallbackFunctions->EndStream = CvtvtHtmlEndStream;
    CallbackFunctions->ProcessAndOutputText = CvtvtHtmlProcessAndOutputText;
    CallbackFunctions->ProcessAndOutputEscape = CvtvtHtmlProcessAndOutputEscape;
    return TRUE;
}

/**
 Set parsing functions to generate HTML5 output.

 @param CallbackFunctions Function pointers to populate with those that can
        generate HTML5 output.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtHtml5SetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    )
{
    CvtvtHtmlVersion = 5;
    CallbackFunctions->InitializeStream = CvtvtHtmlInitializeStream;
    CallbackFunctions->EndStream = CvtvtHtmlEndStream;
    CallbackFunctions->ProcessAndOutputText = CvtvtHtmlProcessAndOutputText;
    CallbackFunctions->ProcessAndOutputEscape = CvtvtHtmlProcessAndOutputEscape;
    return TRUE;
}

// vim:sw=4:ts=4:et:
