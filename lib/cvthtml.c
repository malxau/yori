/**
 * @file lib/cvthtml.c
 *
 * Convert VT100/ANSI escape sequences into HTML.
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

#include "yoripch.h"
#include "yorilib.h"

/**
 The default color to use when all else fails.
 */
#define CVTVT_DEFAULT_COLOR (7)

/**
 Attempt to capture the current console font.  This is only available on
 newer systems.

 @param FontInfo On successful completion, populated with information about
        the font in use by the console.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibCaptureConsoleFont(
    __out PYORI_CONSOLE_FONT_INFOEX FontInfo
    )
{
    HANDLE hConsole;

    ZeroMemory(FontInfo, sizeof(YORI_CONSOLE_FONT_INFOEX));

    if (DllKernel32.pGetCurrentConsoleFontEx == NULL) {
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

    FontInfo->cbSize = sizeof(YORI_CONSOLE_FONT_INFOEX);

    if (!DllKernel32.pGetCurrentConsoleFontEx(hConsole, FALSE, FontInfo)) {
        CloseHandle(hConsole);
        return FALSE;
    }

    CloseHandle(hConsole);
    return TRUE;
}

/**
 The default color table to use.  This is used on pre-Vista systems which
 don't let console programs query the color table that the console is using.
 */
DWORD YoriLibDefaultColorTable[] = {0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0xc0c0c0,
                                    0x808080, 0xff0000, 0x00ff00, 0xffff00, 0x0000ff, 0xff00ff, 0x00ffff, 0xffffff};

/**
 Return a referenced allocation to the color table.  This might be the
 console's color table or it might be a copy of the default table.

 @param ColorTable On successful completion, updated to point to a color
        table.  This should be freed with @ref YoriLibDereference.

 @param CurrentAttributes Optionally points to a location to receive the
        currently active color.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibCaptureConsoleColorTable(
    __out PDWORD * ColorTable,
    __out_opt PWORD CurrentAttributes
    )
{
    YORI_CONSOLE_SCREEN_BUFFER_INFOEX ScreenInfoEx;
    HANDLE hConsole;
    PDWORD TableToReturn;

    if (DllKernel32.pGetConsoleScreenBufferInfoEx) {
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

        ScreenInfoEx.cbSize = sizeof(ScreenInfoEx);

        if (DllKernel32.pGetConsoleScreenBufferInfoEx(hConsole, &ScreenInfoEx)) {
            TableToReturn = YoriLibReferencedMalloc(sizeof(DWORD) * 16);
            if (TableToReturn != NULL) {
                memcpy(TableToReturn, ScreenInfoEx.ColorTable, 16 * sizeof(DWORD));
                CloseHandle(hConsole);
                *ColorTable = TableToReturn;
                if (CurrentAttributes != NULL) {
                    *CurrentAttributes = ScreenInfoEx.wAttributes;
                }
                return TRUE;
            }
        }

        CloseHandle(hConsole);
    }

    TableToReturn = YoriLibReferencedMalloc(sizeof(DWORD) * 16);
    if (TableToReturn != NULL) {
        memcpy(TableToReturn, YoriLibDefaultColorTable, sizeof(DWORD) * 16);
        *ColorTable = TableToReturn;
        if (CurrentAttributes != NULL) {
            *CurrentAttributes = CVTVT_DEFAULT_COLOR;
        }
        return TRUE;
    }

    return FALSE;
}

/**
 Output to include at the beginning of any HTML stream.
 */
CONST TCHAR YoriLibHtmlHeader[] = _T("<HTML><HEAD><TITLE>cvtvt output</TITLE></HEAD>");

/**
 Output to include at the beginning of any version 4 HTML body.
 */
CONST TCHAR YoriLibHtmlVer4Header[] = _T("<BODY>")
                                _T("<DIV STYLE=\"background-color: #000000; font-family: ");

/**
 Output to include at the beginning of any version 5 HTML body.
 */
CONST TCHAR YoriLibHtmlVer5Header[] = _T("<BODY><DIV STYLE=\"background-color: #000000; color: #c0c0c0; border-style: ridge; border-width: 4px; display: inline-block; font-family: ");

/**
 The end of the HTML body section, after font information has been populated.
 This is used for version 4 and 5.
 */
CONST TCHAR YoriLibHtmlVerHeaderEnd[] = _T(";\">");

/**
 Final text to output at the end of any HTML stream.
 */
CONST TCHAR YoriLibHtmlFooter[] = _T("</DIV></BODY></HTML>");

/**
 Generate a string of text for the current console font and state to commence
 an HTML output stream.

 @param TextString On successful completion, updated to contain the start of
        an HTML stream.  Note this string maybe allocated in this function.

 @param GenerateContext Pointer to the context recording state while
        generation is in progress.  On input, this specifies the HTML dialect
        to use; on output, other state fields are initialized.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlGenerateInitialString(
    __inout PYORI_STRING TextString,
    __inout PYORILIB_HTML_GENERATE_CONTEXT GenerateContext
    )
{
    YORI_CONSOLE_FONT_INFOEX FontInfo;
    TCHAR szFontNames[LF_FACESIZE + 100];
    TCHAR szFontWeight[100];
    TCHAR szFontSize[100];

    if (!YoriLibCaptureConsoleFont(&FontInfo)) {
        FontInfo.FaceName[0] = '\0';
    }

    if (FontInfo.FaceName[0] != '\0') {
        YoriLibSPrintf(szFontNames, _T("'%ls', monospace"), FontInfo.FaceName);
    } else {
        YoriLibSPrintf(szFontNames, _T("monospace"));
    }

    if (FontInfo.FontWeight == 0) {
        FontInfo.FontWeight = 700;
    }
    if (FontInfo.dwFontSize.Y == 0) {
        FontInfo.dwFontSize.Y = 12;
    }

    YoriLibSPrintf(szFontSize, _T("; font-size: %ipx"), FontInfo.dwFontSize.Y);
    if (GenerateContext->HtmlVersion == 4) {
        szFontWeight[0] = '\0';
    } else {
        YoriLibSPrintf(szFontWeight, _T("; font-weight: %i"), FontInfo.FontWeight);
    }

    GenerateContext->TagOpen = FALSE;
    GenerateContext->UnderlineOn = FALSE;

    if (FontInfo.FontWeight >= 600) {
        GenerateContext->BoldOn = TRUE;
    } else {
        GenerateContext->BoldOn = FALSE;
    }

    YoriLibYPrintf(TextString,
                   _T("%s%s%s%s%s%s%s"),
                   YoriLibHtmlHeader,
                   (GenerateContext->HtmlVersion == 4)?YoriLibHtmlVer4Header:YoriLibHtmlVer5Header,
                   szFontNames,
                   (GenerateContext->HtmlVersion == 5)?szFontWeight:_T(""),
                   szFontSize,
                   YoriLibHtmlVerHeaderEnd,
                   (GenerateContext->HtmlVersion == 4 && GenerateContext->BoldOn)?_T("<B>"):_T(""));

    if (TextString->StartOfString == NULL) {
        return FALSE;
    }

    return TRUE;
}

/**
 Generate a string of text for the current console font and state to end
 an HTML output stream.

 @param TextString On successful completion, updated to contain the end of
        an HTML stream.  Note this string maybe allocated in this function.

 @param GenerateContext Pointer to the context recording state while
        generation is in progress.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlGenerateEndString(
    __inout PYORI_STRING TextString,
    __inout PYORILIB_HTML_GENERATE_CONTEXT GenerateContext
    )
{
    YoriLibYPrintf(TextString,
                   _T("%s%s%s%s"),
                   (GenerateContext->TagOpen?(GenerateContext->UnderlineOn?_T("</U>"):_T("")):_T("")),
                   (GenerateContext->TagOpen?(GenerateContext->HtmlVersion == 4?_T("</FONT>"):_T("</SPAN>")):_T("")),
                   (GenerateContext->HtmlVersion == 4 && GenerateContext->BoldOn)?_T("</B>"):_T(""),
                   YoriLibHtmlFooter);

    if (TextString->StartOfString == NULL) {
        return FALSE;
    }

    return TRUE;
}

/**
 Generate a string of text that encodes regular text for inclusion in HTML.

 @param TextString On successful completion, updated to contain the escaped
        HTML text.  If this buffer is not large enough, the routine succeeds
        and returns the required size in BufferSizeNeeded.

 @param BufferSizeNeeded On successful completion, indicates the number of
        characters needed in the TextString buffer to complete the operation.

 @param SrcString Pointer to the string containing a VT100 escape to
        convert to HTML.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlGenerateTextString(
    __inout PYORI_STRING TextString,
    __out PYORI_ALLOC_SIZE_T BufferSizeNeeded,
    __in PCYORI_STRING SrcString
    )
{

    TCHAR  LookFor[] = {'<', '>', ' ', '\n', '\r', '\0'};
    LPTSTR SrcPoint;
    YORI_ALLOC_SIZE_T SrcOffset = 0;
    YORI_ALLOC_SIZE_T SrcConsumed = 0;
    YORI_ALLOC_SIZE_T DestOffset = 0;
    YORI_ALLOC_SIZE_T AddToDestOffset;
    YORI_STRING SearchString;

    //
    //  Scan through the string again looking for text that needs to be
    //  escaped in HTML, in particular greater than and less than since
    //  those denote tags
    //

    TextString->LengthInChars = 0;
    SrcPoint = SrcString->StartOfString;

    do {
        AddToDestOffset = 0;
        SearchString.StartOfString = SrcPoint;
        SearchString.LengthInChars = SrcString->LengthInChars - SrcConsumed;
        SrcOffset = YoriLibCountStringNotContainingChars(&SearchString, LookFor);
        if (SrcOffset > 0) {
            if (DestOffset + SrcOffset < TextString->LengthAllocated) {
                memcpy(&TextString->StartOfString[DestOffset], SrcPoint, SrcOffset * sizeof(TCHAR));
            }

            SrcPoint = SrcPoint + SrcOffset;
            SrcConsumed = SrcConsumed + SrcOffset;

            DestOffset = YoriLibIsAllocationExtendable(DestOffset, SrcOffset, SrcOffset);
            if (DestOffset == 0) {
                return FALSE;
            }
        }

        if (SrcConsumed < SrcString->LengthInChars) {

            //
            //  Escape things that could be tags, and parse escape sequences
            //  we understand
            //

            if (*SrcPoint == '<') {
                if (DestOffset + sizeof("&lt;") - 1 < TextString->LengthAllocated) {
                    memcpy(&TextString->StartOfString[DestOffset], _T("&lt;"), sizeof(_T("&lt;")) - sizeof(TCHAR));
                }
                AddToDestOffset = sizeof("&lt;") - 1;
                SrcPoint++;
                SrcConsumed++;
            } else if (*SrcPoint == '>') {
                if (DestOffset + sizeof("&gt;") - 1 < TextString->LengthAllocated) {
                    memcpy(&TextString->StartOfString[DestOffset], _T("&gt;"), sizeof(_T("&gt;")) - sizeof(TCHAR));
                }
                AddToDestOffset = sizeof("&gt;") - 1;
                SrcPoint++;
                SrcConsumed++;
            } else if (*SrcPoint == '\n') {
                if (DestOffset + sizeof("<BR>") - 1 < TextString->LengthAllocated) {
                    memcpy(&TextString->StartOfString[DestOffset], _T("<BR>"), sizeof(_T("<BR>")) - sizeof(TCHAR));
                }
                AddToDestOffset = sizeof("<BR>") - 1;
                SrcPoint++;
                SrcConsumed++;
            } else if (*SrcPoint == ' ') {
                if (DestOffset + sizeof("&nbsp;") - 1 < TextString->LengthAllocated) {
                    memcpy(&TextString->StartOfString[DestOffset], _T("&nbsp;"), sizeof(_T("&nbsp;")) - sizeof(TCHAR));
                }
                AddToDestOffset = sizeof("&nbsp;") - 1;
                SrcPoint++;
                SrcConsumed++;
            } else if (*SrcPoint == '\r') {
                SrcPoint++;
                SrcConsumed++;
            }
        }

        if (AddToDestOffset != 0) {
            DestOffset = YoriLibIsAllocationExtendable(DestOffset, AddToDestOffset, AddToDestOffset);
            if (DestOffset == 0) {
                return FALSE;
            }
        }

    } while (SrcConsumed < SrcString->LengthInChars);

    if (DestOffset < TextString->LengthAllocated) {
        TextString->StartOfString[DestOffset] = '\0';
        TextString->LengthInChars = DestOffset;
    }

    DestOffset = YoriLibIsAllocationExtendable(DestOffset, 1, 1);
    if (DestOffset == 0) {
        return FALSE;
    }

    *BufferSizeNeeded = DestOffset;
    return TRUE;
}

/**
 Generate a string of text that descibes a VT100 escape action in terms of
 HTML.

 @param TextString On successful completion, updated to contain the escaped
        HTML text.  If this buffer is not large enough, the routine succeeds
        and returns the required size in BufferSizeNeeded.

 @param BufferSizeNeeded On successful completion, indicates the number of
        characters needed in the TextString buffer to complete the operation.

 @param ColorTable Pointer to a color table describing how to convert the 16
        colors into RGB.  If NULL, a default mapping is used.

 @param SrcString Pointer to the string containing a VT100 escape to convert
        to HTML.

 @param GenerateContext Pointer to the context recording state while
        generation is in progress.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlGenerateEscapeStringInternal(
    __inout PYORI_STRING TextString,
    __out PYORI_ALLOC_SIZE_T BufferSizeNeeded,
    __in_opt PDWORD ColorTable,
    __in PCYORI_STRING SrcString,
    __inout PYORILIB_HTML_GENERATE_CONTEXT GenerateContext
    )
{
    LPTSTR SrcPoint;
    YORI_ALLOC_SIZE_T SrcOffset;
    WORD NewColor = CVTVT_DEFAULT_COLOR;
    YORI_ALLOC_SIZE_T RemainingLength;
    YORI_ALLOC_SIZE_T DestOffset;
    YORI_ALLOC_SIZE_T AddToDestOffset;
    YORI_STRING SearchString;

    SrcPoint = SrcString->StartOfString;
    RemainingLength = SrcString->LengthInChars;

    TextString->LengthInChars = 0;
    DestOffset = 0;
    SrcOffset = 0;

    //
    //  We expect an escape initiator (two chars) and a 'm' for color
    //  formatting.  This should already be validated, the check here
    //  is redundant.
    //

    if (SrcString->LengthInChars >= 3 &&
        SrcPoint[SrcString->LengthInChars - 1] == 'm') {

        DWORD code;
        TCHAR NewTag[128];
        BOOLEAN NewUnderline = FALSE;
        PDWORD ColorTableToUse;
        ColorTableToUse = ColorTable;
        if (ColorTableToUse == NULL) {
            ColorTableToUse = YoriLibDefaultColorTable;
        }

        AddToDestOffset = 0;

        SrcPoint++;
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

            SrcPoint++;
            RemainingLength--;

            SearchString.StartOfString = SrcPoint;
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
            SearchString.StartOfString = SrcPoint;
            SearchString.LengthInChars = RemainingLength;
            SrcOffset = YoriLibCountStringContainingChars(&SearchString, _T("0123456789"));

            SrcPoint += SrcOffset;
            RemainingLength = RemainingLength - SrcOffset;

            if (RemainingLength == 0 ||
                *SrcPoint != ';') {

                break;
            }
        }

        if (GenerateContext->TagOpen) {
            if (GenerateContext->UnderlineOn) {
                if (DestOffset + sizeof("</U>") - 1 < TextString->LengthAllocated) {
                    memcpy(&TextString->StartOfString[DestOffset], _T("</U>"), sizeof(_T("</U>")) - sizeof(TCHAR));
                }
                AddToDestOffset = sizeof("</U>") - 1;
            }
            if (GenerateContext->HtmlVersion == 4) {
                if (DestOffset + sizeof("</FONT>") - 1 < TextString->LengthAllocated) {
                    memcpy(&TextString->StartOfString[DestOffset], _T("</FONT>"), sizeof(_T("</FONT>")) - sizeof(TCHAR));
                }
                AddToDestOffset = sizeof("</FONT>") - 1;
            } else {
                if (DestOffset + sizeof("</SPAN>") - 1 < TextString->LengthAllocated) {
                    memcpy(&TextString->StartOfString[DestOffset], _T("</SPAN>"), sizeof(_T("</SPAN>")) - sizeof(TCHAR));
                }
                AddToDestOffset = sizeof("</SPAN>") - 1;
            }

            if (AddToDestOffset != 0) {
                DestOffset = YoriLibIsAllocationExtendable(DestOffset, AddToDestOffset, AddToDestOffset);
                if (DestOffset == 0) {
                    return FALSE;
                }
            }
        }

        //
        //  Convert the color to a Windows color so that it maps into the
        //  Windows colortable
        //

        NewColor = YoriLibAnsiToWindowsByte(NewColor);

        //
        //  Output the appropriate tag depending on the version the user wanted
        //

        if (GenerateContext->HtmlVersion == 4) {
            SrcOffset = YoriLibSPrintf(NewTag,
                                       _T("<FONT COLOR=#%02x%02x%02x>"),
                                       GetRValue(ColorTableToUse[NewColor & 0xf]),
                                       GetGValue(ColorTableToUse[NewColor & 0xf]),
                                       GetBValue(ColorTableToUse[NewColor & 0xf]));
        } else {
            SrcOffset = YoriLibSPrintf(NewTag,
                                       _T("<SPAN STYLE=\"color:#%02x%02x%02x;background-color:#%02x%02x%02x\">"),
                                       GetRValue(ColorTableToUse[NewColor & 0xf]),
                                       GetGValue(ColorTableToUse[NewColor & 0xf]),
                                       GetBValue(ColorTableToUse[NewColor & 0xf]),
                                       GetRValue(ColorTableToUse[(NewColor & 0xf0) >> 4]),
                                       GetGValue(ColorTableToUse[(NewColor & 0xf0) >> 4]),
                                       GetBValue(ColorTableToUse[(NewColor & 0xf0) >> 4]));
        }


        if (DestOffset + SrcOffset < TextString->LengthAllocated) {
            memcpy(&TextString->StartOfString[DestOffset], NewTag, SrcOffset * sizeof(TCHAR));
        }

        if (SrcOffset > 0) {
            DestOffset = YoriLibIsAllocationExtendable(DestOffset, SrcOffset, SrcOffset);
            if (DestOffset == 0) {
                return FALSE;
            }
        }

        if (NewUnderline) {
            if (DestOffset + sizeof("<U>") - 1 < TextString->LengthAllocated) {
                memcpy(&TextString->StartOfString[DestOffset], _T("<U>"), sizeof(_T("<U>")) - sizeof(TCHAR));
            }
            AddToDestOffset = sizeof("<U>") - 1;
            DestOffset = YoriLibIsAllocationExtendable(DestOffset, AddToDestOffset, AddToDestOffset);
            if (DestOffset == 0) {
                return FALSE;
            }
        }

        GenerateContext->UnderlineOn = NewUnderline;
        GenerateContext->TagOpen = TRUE;
    }

    if (DestOffset < TextString->LengthAllocated) {
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6011) // Dereferencing NULL pointer; if LengthAllocated
                                // is nonzero, there should be a buffer
#endif
        TextString->StartOfString[DestOffset] = '\0';
        TextString->LengthInChars = DestOffset;
    }
    DestOffset = YoriLibIsAllocationExtendable(DestOffset, 1, 1);
    if (DestOffset == 0) {
        return FALSE;
    }

    *BufferSizeNeeded = DestOffset + 1;

    return TRUE;
}

/**
 Generate a string of text that descibes a VT100 escape action in terms of
 HTML.

 @param TextString On successful completion, updated to contain the escaped
        HTML text.  If this buffer is not large enough, the routine succeeds
        and returns the required size in BufferSizeNeeded.

 @param BufferSizeNeeded On successful completion, indicates the number of
        characters needed in the TextString buffer to complete the operation.

 @param SrcString Pointer to the string containing a VT100 escape to convert
        to HTML.

 @param GenerateContext Pointer to the context recording state while
        generation is in progress.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlGenerateEscapeString(
    __inout PYORI_STRING TextString,
    __out PYORI_ALLOC_SIZE_T BufferSizeNeeded,
    __in PCYORI_STRING SrcString,
    __inout PYORILIB_HTML_GENERATE_CONTEXT GenerateContext
    )
{
    return YoriLibHtmlGenerateEscapeStringInternal(TextString, BufferSizeNeeded, NULL, SrcString, GenerateContext);
}

/**
 A context which can be passed around as a "handle" when generating an HTML
 output string from VT100 text.
 */
typedef struct _YORI_LIB_HTML_CONVERT_CONTEXT {

    /**
     Pointer to the Html buffer generated thus far.  This may be periodically
     reallocated.
     */
    PYORI_STRING HtmlText;

    /**
     Pointer to a color table describing how to convert the 16 colors into
     RGB.  If NULL, a default mapping is used.
     */
    PDWORD ColorTable;

    /**
     The context recording state while generation is in progress.
     */
    YORILIB_HTML_GENERATE_CONTEXT GenerateContext;

} YORI_LIB_HTML_CONVERT_CONTEXT, *PYORI_LIB_HTML_CONVERT_CONTEXT;

/**
 Append one Yori string to the tail of another, reallocating the combined
 buffer as required.

 @param StringToAppendTo The first string which should have the contents of
        the second string appended to it.  This string may be reallocated.

 @param StringToAdd The string to append to the first string.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlCvtAppendWithReallocate(
    __inout PYORI_STRING StringToAppendTo,
    __in PYORI_STRING StringToAdd
    )
{
    DWORD LengthNeeded;

    LengthNeeded = StringToAppendTo->LengthInChars + StringToAdd->LengthInChars;
    if (!YoriLibIsSizeAllocatable(LengthNeeded)) {
        return FALSE;
    }
    if (LengthNeeded > StringToAppendTo->LengthAllocated) {
        YORI_ALLOC_SIZE_T AllocSize;
        AllocSize = YoriLibMaximumAllocationInRange(LengthNeeded, LengthNeeded * 4);
        if (!YoriLibReallocateString(StringToAppendTo, AllocSize)) {
            return FALSE;
        }
    }

    memcpy(&StringToAppendTo->StartOfString[StringToAppendTo->LengthInChars], StringToAdd->StartOfString, StringToAdd->LengthInChars * sizeof(TCHAR));
    StringToAppendTo->LengthInChars = StringToAppendTo->LengthInChars + StringToAdd->LengthInChars;

    return TRUE;
}

/**
 Indicate the beginning of a stream and perform any initial output.

 @param hOutput The context to output any footer information to.

 @param Context Pointer to context (unused.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlCnvInitializeStream(
    __in HANDLE hOutput,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    YORI_STRING OutputString;
    PYORI_LIB_HTML_CONVERT_CONTEXT HtmlContext = (PYORI_LIB_HTML_CONVERT_CONTEXT)hOutput;

    UNREFERENCED_PARAMETER(Context);

    YoriLibInitEmptyString(&OutputString);
    if (!YoriLibHtmlGenerateInitialString(&OutputString, &HtmlContext->GenerateContext)) {
        return FALSE;
    }

    if (!YoriLibHtmlCvtAppendWithReallocate(HtmlContext->HtmlText, &OutputString)) {
        YoriLibFreeStringContents(&OutputString);
        return FALSE;
    }

    YoriLibFreeStringContents(&OutputString);
    return TRUE;
}

/**
 Indicate the end of the stream has been reached and perform any final
 output.

 @param hOutput The context to output any footer information to.

 @param Context Pointer to context (unused.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlCnvEndStream(
    __in HANDLE hOutput,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    YORI_STRING OutputString;
    PYORI_LIB_HTML_CONVERT_CONTEXT HtmlContext = (PYORI_LIB_HTML_CONVERT_CONTEXT)hOutput;

    UNREFERENCED_PARAMETER(Context);

    YoriLibInitEmptyString(&OutputString);

    if (!YoriLibHtmlGenerateEndString(&OutputString, &HtmlContext->GenerateContext)) {
        return FALSE;
    }

    if (!YoriLibHtmlCvtAppendWithReallocate(HtmlContext->HtmlText, &OutputString)) {
        YoriLibFreeStringContents(&OutputString);
        return FALSE;
    }

    YoriLibFreeStringContents(&OutputString);
    return TRUE;
}

/**
 Parse text between VT100 escape sequences and generate correct output for
 either HTML4 or HTML5.

 @param hOutput The output context to populate with the translated text
        information.

 @param String Pointer to a string buffer containing the text to process.

 @param Context Pointer to context (unused.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlCnvProcessAndOutputText(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{

    YORI_STRING TextString;
    YORI_ALLOC_SIZE_T BufferSizeNeeded;
    PYORI_LIB_HTML_CONVERT_CONTEXT HtmlContext = (PYORI_LIB_HTML_CONVERT_CONTEXT)hOutput;

    UNREFERENCED_PARAMETER(Context);

    YoriLibInitEmptyString(&TextString);
    BufferSizeNeeded = 0;
    if (!YoriLibHtmlGenerateTextString(&TextString, &BufferSizeNeeded, String)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&TextString, BufferSizeNeeded)) {
        return FALSE;
    }

    BufferSizeNeeded = 0;
    if (!YoriLibHtmlGenerateTextString(&TextString, &BufferSizeNeeded, String)) {
        YoriLibFreeStringContents(&TextString);
        return FALSE;
    }

    if (!YoriLibHtmlCvtAppendWithReallocate(HtmlContext->HtmlText, &TextString)) {
        YoriLibFreeStringContents(&TextString);
        return FALSE;
    }

    YoriLibFreeStringContents(&TextString);
    return TRUE;
}


/**
 Parse a VT100 escape sequence and generate the correct output for either
 HTML4 or HTML5.

 @param hOutput The output context to populate with the translated escape
        information.

 @param String Pointer to a string buffer containing the escape to process.

 @param Context Pointer to context (unused.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlCnvProcessAndOutputEscape(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    YORI_STRING TextString;
    YORI_ALLOC_SIZE_T BufferSizeNeeded;
    PYORI_LIB_HTML_CONVERT_CONTEXT HtmlContext = (PYORI_LIB_HTML_CONVERT_CONTEXT)hOutput;
    YORILIB_HTML_GENERATE_CONTEXT DummyGenerateContext;

    UNREFERENCED_PARAMETER(Context);

    memcpy(&DummyGenerateContext, &HtmlContext->GenerateContext, sizeof(YORILIB_HTML_GENERATE_CONTEXT));

    YoriLibInitEmptyString(&TextString);
    BufferSizeNeeded = 0;
    if (!YoriLibHtmlGenerateEscapeStringInternal(&TextString, &BufferSizeNeeded, HtmlContext->ColorTable, String, &DummyGenerateContext)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&TextString, BufferSizeNeeded)) {
        return FALSE;
    }

    BufferSizeNeeded = 0;
    if (!YoriLibHtmlGenerateEscapeStringInternal(&TextString, &BufferSizeNeeded, HtmlContext->ColorTable, String, &HtmlContext->GenerateContext)) {
        YoriLibFreeStringContents(&TextString);
        return FALSE;
    }

    if (!YoriLibHtmlCvtAppendWithReallocate(HtmlContext->HtmlText, &TextString)) {
        YoriLibFreeStringContents(&TextString);
        return FALSE;
    }

    YoriLibFreeStringContents(&TextString);
    return TRUE;
}


/**
 Convert a Yori string containing VT100 text into HTML with the specified
 format.

 @param VtText Pointer to the string to convert.

 @param HtmlText On successful completion, updated to point to an HTML
        representation.  This string will be reallocated within this routine.

 @param ColorTable Pointer to a color table describing how to convert the 16
        colors into RGB.  If NULL, the current console mapping is used if
        available, and if not available, a default mapping is used.

 @param HtmlVersion Specifies the format of HTML to use.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibHtmlConvertToHtmlFromVt(
    __in PYORI_STRING VtText,
    __inout PYORI_STRING HtmlText,
    __in_opt PDWORD ColorTable,
    __in DWORD HtmlVersion
    )
{
    YORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions;
    YORI_LIB_HTML_CONVERT_CONTEXT HtmlContext;
    BOOL FreeColorTable = FALSE;

    HtmlContext.HtmlText = HtmlText;
    HtmlContext.ColorTable = ColorTable;
    if (ColorTable == NULL) {
        if (YoriLibCaptureConsoleColorTable(&HtmlContext.ColorTable, NULL)) {
            FreeColorTable = TRUE;
        } else {
            HtmlContext.ColorTable = YoriLibDefaultColorTable;
        }
    }
    HtmlContext.GenerateContext.HtmlVersion = HtmlVersion;

    CallbackFunctions.InitializeStream = YoriLibHtmlCnvInitializeStream;
    CallbackFunctions.EndStream = YoriLibHtmlCnvEndStream;
    CallbackFunctions.ProcessAndOutputText = YoriLibHtmlCnvProcessAndOutputText;
    CallbackFunctions.ProcessAndOutputEscape = YoriLibHtmlCnvProcessAndOutputEscape;
    CallbackFunctions.Context = 0;

    if (!YoriLibHtmlCnvInitializeStream((HANDLE)&HtmlContext, &CallbackFunctions.Context)) {
        if (FreeColorTable) {
            YoriLibDereference(HtmlContext.ColorTable);
        }
        return FALSE;
    }

    if (!YoriLibProcessVtEscapesOnOpenStream(VtText->StartOfString,
                                             VtText->LengthInChars,
                                             (HANDLE)&HtmlContext,
                                             &CallbackFunctions)) {

        if (FreeColorTable) {
            YoriLibDereference(HtmlContext.ColorTable);
        }

        return FALSE;
    }

    if (!YoriLibHtmlCnvEndStream((HANDLE)&HtmlContext, &CallbackFunctions.Context)) {
        if (FreeColorTable) {
            YoriLibDereference(HtmlContext.ColorTable);
        }
        return FALSE;
    }
    if (FreeColorTable) {
        YoriLibDereference(HtmlContext.ColorTable);
    }

    return TRUE;
}

// vim:sw=4:ts=4:et:
