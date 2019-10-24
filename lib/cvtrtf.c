/**
 * @file lib/cvtrtf.c
 *
 * Convert VT100/ANSI escape sequences into RTF.
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
 While parsing, TRUE if underlining is in effect.  This global is updated
 when text is generated.
 */
BOOLEAN YoriLibRtfUnderlineOn = FALSE;

/**
 While parsing, TRUE if underlining is in effect.  This global is updated
 when buffer size checks are made but no text is generated.
 */
BOOLEAN YoriLibRtfUnderlineOnBufferCheck = FALSE;

/**
 Output to include at the beginning of any RTF stream.
 */
CONST TCHAR YoriLibRtfHeader[] = _T("{\\rtf1\\ansi\\ansicpg1252\\deff0{\\fonttbl{\\f0\\fmodern\\fprq1\\fcharset0 %s;}}\n");

/**
 The longest single color table entry, used for buffer sizing.
 */
CONST TCHAR YoriLibRtfWorstColorTableEntry[] = _T("\\red255\\green255\\blue255;");

/**
 The color table header.
 */
CONST TCHAR YoriLibRtfColorTableHeader[] = _T("{\\colortbl ;");

/**
 The color table footer.
 */
CONST TCHAR YoriLibRtfColorTableFooter[] = _T("}\n");

/**
 The paragraph header.
 */
CONST TCHAR YoriLibRtfParagraphHeader[] = _T("\\pard\\sl240\\slmult1\\sa0\\sb0\\f0\\fs%i\\cbpat%i\\uc1%s");

/**
 Final text to output at the end of any RTF stream.
 */
CONST TCHAR YoriLibRtfFooter[] = _T("\\pard\n}\n");

/**
 Generate a string of text for the current console font and state to commence
 an RTF output stream.

 @param TextString On successful completion, updated to contain the start of
        an RTF stream.  Note this string maybe allocated in this function.

 @param ColorTable Optionally points to a color table to include in the RTF
        file.  If not specified, the console's color table is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibRtfGenerateInitialString(
    __inout PYORI_STRING TextString,
    __in_opt PDWORD ColorTable
    )
{
    YORI_CONSOLE_FONT_INFOEX FontInfo;
    YORI_STRING Substring;
    DWORD Index;
    PDWORD ColorTableToUse;
    DWORD CharsNeeded;
    WORD CurrentAttributes;
    BOOL FreeColorTable = FALSE;

    ColorTableToUse = ColorTable;
    CurrentAttributes = CVTVT_DEFAULT_COLOR;
    if (ColorTableToUse == NULL) {
        if (YoriLibCaptureConsoleColorTable(&ColorTableToUse, &CurrentAttributes)) {
            FreeColorTable = TRUE;
        } else {
            ColorTableToUse = YoriLibDefaultColorTable;
        }
    }

    YoriLibCaptureConsoleFont(&FontInfo);

    if (FontInfo.FaceName[0] == '\0') {
        YoriLibSPrintf(FontInfo.FaceName, _T("Courier New"));
    }

    if (FontInfo.dwFontSize.Y == 0) {
        FontInfo.dwFontSize.Y = 14;
    }

    if (FontInfo.FontWeight == 0) {
        FontInfo.FontWeight = 700;
    }

    CharsNeeded = sizeof(YoriLibRtfHeader)/sizeof(YoriLibRtfHeader[0]) +
                  _tcslen(FontInfo.FaceName) +
                  sizeof(YoriLibRtfColorTableHeader)/sizeof(YoriLibRtfColorTableHeader[0]) +
                  16 * sizeof(YoriLibRtfWorstColorTableEntry)/sizeof(YoriLibRtfWorstColorTableEntry[0]) +
                  sizeof(YoriLibRtfColorTableFooter)/sizeof(YoriLibRtfColorTableFooter[0]) +
                  sizeof(YoriLibRtfParagraphHeader)/sizeof(YoriLibRtfParagraphHeader[0]) + 10;

    if (TextString->LengthAllocated < CharsNeeded) {
        YoriLibFreeStringContents(TextString);
        if (!YoriLibAllocateString(TextString, CharsNeeded)) {
            if (FreeColorTable) {
                YoriLibDereference(ColorTableToUse);
            }
            return FALSE;
        }
    }

    //
    //  Generate RTF header specifying console font.
    //

    TextString->LengthInChars = YoriLibSPrintf(TextString->StartOfString, YoriLibRtfHeader, FontInfo.FaceName);

    YoriLibInitEmptyString(&Substring);

    Substring.StartOfString = &TextString->StartOfString[TextString->LengthInChars];
    Substring.LengthAllocated = TextString->LengthAllocated - TextString->LengthInChars;
    //
    //  Generate table of console colors.
    //

    Substring.LengthInChars = YoriLibSPrintf(Substring.StartOfString, YoriLibRtfColorTableHeader);

    TextString->LengthInChars += Substring.LengthInChars;

    for (Index = 0; Index < 16; Index++) {
        Substring.StartOfString = &TextString->StartOfString[TextString->LengthInChars];
        Substring.LengthAllocated = TextString->LengthAllocated - TextString->LengthInChars;
        Substring.LengthInChars = YoriLibSPrintf(Substring.StartOfString,
                                                 _T("\\red%i\\green%i\\blue%i;"),
                                                 GetRValue(ColorTableToUse[Index]),
                                                 GetGValue(ColorTableToUse[Index]),
                                                 GetBValue(ColorTableToUse[Index]));

        TextString->LengthInChars += Substring.LengthInChars;
    }

    Substring.StartOfString = &TextString->StartOfString[TextString->LengthInChars];
    Substring.LengthAllocated = TextString->LengthAllocated - TextString->LengthInChars;
    Substring.LengthInChars = YoriLibSPrintf(Substring.StartOfString, YoriLibRtfColorTableFooter);
    TextString->LengthInChars += Substring.LengthInChars;

    Substring.StartOfString = &TextString->StartOfString[TextString->LengthInChars];
    Substring.LengthAllocated = TextString->LengthAllocated - TextString->LengthInChars;
    //
    //  This is a rough approximation of converting vertical pixels to points,
    //  then multiplying by two because RTF uses half points as a unit.
    //  Setting the background color is a bit of a joke, because it's applying
    //  the _current_ background color to be the default background color.
    //  What we really want is the initial background color.  Note this will
    //  be explicitly overwritten by any text though.
    //

    Substring.LengthInChars = YoriLibSPrintf(Substring.StartOfString,
                                             YoriLibRtfParagraphHeader,
                                             FontInfo.dwFontSize.Y * 15 / 10,
                                             ((CurrentAttributes >> 4) & 0xf) + 1,
                                             (FontInfo.FontWeight >= 600)?_T("\\b"):_T(""));
    TextString->LengthInChars += Substring.LengthInChars;

    if (FreeColorTable) {
        YoriLibDereference(ColorTableToUse);
    }
    return TRUE;
}

/**
 Generate a string of text for the current console font and state to end
 an RTF output stream.

 @param TextString On successful completion, updated to contain the end of
        an RTF stream.  Note this string maybe allocated in this function.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibRtfGenerateEndString(
    __inout PYORI_STRING TextString
    )
{
    YoriLibYPrintf(TextString, YoriLibRtfFooter);

    if (TextString->StartOfString == NULL) {
        return FALSE;
    }

    return TRUE;
}

/**
 Generate a string of text that encodes regular text for inclusion in RTF.

 @param TextString On successful completion, updated to contain the escaped
        RTF text.  If this buffer is not large enough, the routine succeeds
        and returns the required size in BufferSizeNeeded.

 @param BufferSizeNeeded On successful completion, indicates the number of
        characters needed in the TextString buffer to complete the operation.

 @param StringBuffer Pointer to the string containing a VT100 escape to
        convert to RTF.

 @param BufferLength Specifies the number of characters in StringBuffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibRtfGenerateTextString(
    __inout PYORI_STRING TextString,
    __out PDWORD BufferSizeNeeded,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    )
{

    LPTSTR SrcPoint;
    DWORD  SrcConsumed = 0;
    DWORD  DestOffset = 0;

    //
    //  Scan through the string again looking for text that needs to be
    //  escaped in RTF
    //

    TextString->LengthInChars = 0;
    SrcPoint = StringBuffer;

    do {

        //
        //  Escape things that could be tags, and parse escape sequences
        //  we understand
        //

        if (*SrcPoint == '\\') {
            if (DestOffset + sizeof("\\\\") - 1 < TextString->LengthAllocated) {
                memcpy(&TextString->StartOfString[DestOffset], _T("\\\\"), sizeof(_T("\\\\")) - sizeof(TCHAR));
            }
            DestOffset += sizeof("\\\\") - 1;
            SrcPoint++;
            SrcConsumed++;
        } else if (*SrcPoint == ' ') {
            if (DestOffset + sizeof("\\~") - 1 < TextString->LengthAllocated) {
                memcpy(&TextString->StartOfString[DestOffset], _T("\\~"), sizeof(_T("\\~")) - sizeof(TCHAR));
            }
            DestOffset += sizeof("\\~") - 1;
            SrcPoint++;
            SrcConsumed++;
        } else if (*SrcPoint == '\n') {
            if (DestOffset + sizeof("\n\\par ") - 1 < TextString->LengthAllocated) {
                memcpy(&TextString->StartOfString[DestOffset], _T("\n\\par "), sizeof(_T("\n\\par ")) - sizeof(TCHAR));
            }
            DestOffset += sizeof("\n\\par ") - 1;
            SrcPoint++;
            SrcConsumed++;
        } else if (*SrcPoint == '\r') {
            SrcPoint++;
            SrcConsumed++;
        } else if (*SrcPoint == '{') {
            if (DestOffset + sizeof("\\{") - 1 < TextString->LengthAllocated) {
                memcpy(&TextString->StartOfString[DestOffset], _T("\\{"), sizeof(_T("\\{")) - sizeof(TCHAR));
            }
            DestOffset += sizeof("\\{") - 1;
            SrcPoint++;
            SrcConsumed++;
        } else if (*SrcPoint == '}') {
            if (DestOffset + sizeof("\\}") - 1 < TextString->LengthAllocated) {
                memcpy(&TextString->StartOfString[DestOffset], _T("\\}"), sizeof(_T("\\}")) - sizeof(TCHAR));
            }
            DestOffset += sizeof("\\}") - 1;
            SrcPoint++;
            SrcConsumed++;
        } else if (*SrcPoint >= 128) {
            DWORD LengthNeeded;

            //
            //  We need enough space for \u, the decimal char value, \',
            //  and two digits to use as a fallback.
            //

            if (*SrcPoint >= 10000) {
                LengthNeeded = 5 + 6;
            } else if (*SrcPoint >= 1000) {
                LengthNeeded = 4 + 6;
            } else {
                LengthNeeded = 3 + 6;
            }

            //
            //  Use printf to generate the string form of the number.
            //  This will NULL terminate so don't add the final char
            //  in the printf, clobber the NULL with it explicitly.
            //

            if (DestOffset + LengthNeeded < TextString->LengthAllocated) {
                YoriLibSPrintf(&TextString->StartOfString[DestOffset], _T("\\u%i\\'3"), *SrcPoint);
                TextString->StartOfString[DestOffset + LengthNeeded - 1] = 'f';
            }
            DestOffset += LengthNeeded;
            SrcPoint++;
            SrcConsumed++;
        } else {
            if (DestOffset + 1 < TextString->LengthAllocated) {
                TextString->StartOfString[DestOffset] = *SrcPoint;
            }
            SrcPoint++;
            DestOffset++;
            SrcConsumed++;
        }

    } while (SrcConsumed < BufferLength);

    if (DestOffset < TextString->LengthAllocated) {
        TextString->StartOfString[DestOffset] = '\0';
        TextString->LengthInChars = DestOffset;
    }

    *BufferSizeNeeded = DestOffset + 1;

    return TRUE;
}

/**
 Generate a string of text that descibes a VT100 escape action in terms of
 RTF.

 @param TextString On successful completion, updated to contain the escaped
        RTF text.  If this buffer is not large enough, the routine succeeds
        and returns the required size in BufferSizeNeeded.

 @param BufferSizeNeeded On successful completion, indicates the number of
        characters needed in the TextString buffer to complete the operation.

 @param StringBuffer Pointer to the string containing a VT100 escape to
        convert to RTF.

 @param BufferLength Specifies the number of characters in StringBuffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibRtfGenerateEscapeString(
    __inout PYORI_STRING TextString,
    __out PDWORD BufferSizeNeeded,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    LPTSTR SrcPoint = StringBuffer;
    DWORD  SrcOffset = 0;
    WORD   NewColor = CVTVT_DEFAULT_COLOR;
    DWORD  RemainingLength;
    DWORD  DestOffset;
    YORI_STRING SearchString;
    LPTSTR UnderlineString;
    BOOLEAN PreviousUnderlineOn;

    RemainingLength = BufferLength;

    TextString->LengthInChars = 0;
    DestOffset = 0;

    //
    //  We expect an escape initiator (two chars) and a 'm' for color
    //  formatting.  This should already be validated, the check here
    //  is redundant.
    //

    if (BufferLength >= 3 &&
        SrcPoint[BufferLength - 1] == 'm') {

        DWORD code;
        TCHAR NewTag[128];
        BOOLEAN NewUnderline = FALSE;

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
            RemainingLength -= SrcOffset;

            if (RemainingLength == 0 ||
                *SrcPoint != ';') {
                
                break;
            }
        }

        if (TextString->StartOfString != NULL) {
            PreviousUnderlineOn = YoriLibRtfUnderlineOn;
        } else {
            PreviousUnderlineOn = YoriLibRtfUnderlineOnBufferCheck;
        }

        UnderlineString = _T("");
        if (NewUnderline && !PreviousUnderlineOn) {
            UnderlineString = _T("\\ul");
            PreviousUnderlineOn = TRUE;
        } else if (!NewUnderline && PreviousUnderlineOn) {
            UnderlineString = _T("\\ul0");
            PreviousUnderlineOn = FALSE;
        }

        //
        //  Convert the color to a Windows color so that it maps into the
        //  Windows colortable
        //

        NewColor = YoriLibAnsiToWindowsByte(NewColor);

        //
        //  Output the appropriate tag depending on the version the user wanted
        //

        SrcOffset = YoriLibSPrintf(NewTag,
                                   _T("\\cf%i\\highlight%i%s "),
                                   (NewColor & 0xf) + 1,
                                   ((NewColor >> 4) & 0xf) + 1,
                                   UnderlineString);

        if (DestOffset + SrcOffset < TextString->LengthAllocated) {
            memcpy(&TextString->StartOfString[DestOffset], NewTag, SrcOffset * sizeof(TCHAR));
        }
        DestOffset += SrcOffset;

        if (TextString->StartOfString != NULL) {
            YoriLibRtfUnderlineOn = PreviousUnderlineOn;
        } else {
            YoriLibRtfUnderlineOnBufferCheck = PreviousUnderlineOn;
        }
    }

    if (DestOffset < TextString->LengthAllocated) {
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6011) // Dereferencing NULL pointer; if LengthAllocated
                                // is nonzero, there should be a buffer
#endif
        TextString->StartOfString[DestOffset] = '\0';
        TextString->LengthInChars = DestOffset;
    }

    *BufferSizeNeeded = DestOffset + 1;

    return TRUE;
}

/**
 A context which can be passed around as a "handle" when generating an RTF
 output string from VT100 text.
 */
typedef struct _YORI_LIB_RTF_CONVERT_CONTEXT {

    /**
     Pointer to the Rtf buffer generated thus far.  This may be periodically
     reallocated.
     */
    PYORI_STRING RtfText;

    /**
     Pointer to a color table describing how to convert the 16 colors into
     RGB.  If NULL, a default mapping is used.
     */
    PDWORD ColorTable;
} YORI_LIB_RTF_CONVERT_CONTEXT, *PYORI_LIB_RTF_CONVERT_CONTEXT;

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
YoriLibRtfCvtAppendWithReallocate(
    __inout PYORI_STRING StringToAppendTo,
    __in PYORI_STRING StringToAdd
    )
{
    if (StringToAppendTo->LengthInChars + StringToAdd->LengthInChars > StringToAppendTo->LengthAllocated) {
        if (!YoriLibReallocateString(StringToAppendTo, (StringToAppendTo->LengthInChars + StringToAdd->LengthInChars) * 4)) {
            return FALSE;
        }
    }

    memcpy(&StringToAppendTo->StartOfString[StringToAppendTo->LengthInChars], StringToAdd->StartOfString, StringToAdd->LengthInChars * sizeof(TCHAR));
    StringToAppendTo->LengthInChars += StringToAdd->LengthInChars;

    return TRUE;
}

/**
 Indicate the beginning of a stream and perform any initial output.

 @param hOutput The context to output any footer information to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibRtfCnvInitializeStream(
    __in HANDLE hOutput
    )
{
    YORI_STRING OutputString;
    PYORI_LIB_RTF_CONVERT_CONTEXT Context = (PYORI_LIB_RTF_CONVERT_CONTEXT)hOutput;

    YoriLibInitEmptyString(&OutputString);
    if (!YoriLibRtfGenerateInitialString(&OutputString, Context->ColorTable)) {
        return FALSE;
    }

    if (!YoriLibRtfCvtAppendWithReallocate(Context->RtfText, &OutputString)) {
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

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibRtfCnvEndStream(
    __in HANDLE hOutput
    )
{
    YORI_STRING OutputString;
    PYORI_LIB_RTF_CONVERT_CONTEXT Context = (PYORI_LIB_RTF_CONVERT_CONTEXT)hOutput;
    YoriLibInitEmptyString(&OutputString);

    if (!YoriLibRtfGenerateEndString(&OutputString)) {
        return FALSE;
    }

    if (!YoriLibRtfCvtAppendWithReallocate(Context->RtfText, &OutputString)) {
        YoriLibFreeStringContents(&OutputString);
        return FALSE;
    }

    YoriLibFreeStringContents(&OutputString);
    return TRUE;
}

/**
 Parse text between VT100 escape sequences and generate correct output for
 either RTF.

 @param hOutput The output context to populate with the translated text
        information.

 @param StringBuffer Pointer to a string buffer containing the text to
        process.

 @param BufferLength Indicates the number of characters in StringBuffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibRtfCnvProcessAndOutputText(
    __in HANDLE hOutput,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    )
{

    YORI_STRING TextString;
    DWORD BufferSizeNeeded;
    PYORI_LIB_RTF_CONVERT_CONTEXT Context = (PYORI_LIB_RTF_CONVERT_CONTEXT)hOutput;

    YoriLibInitEmptyString(&TextString);
    BufferSizeNeeded = 0;
    if (!YoriLibRtfGenerateTextString(&TextString, &BufferSizeNeeded, StringBuffer, BufferLength)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&TextString, BufferSizeNeeded)) {
        return FALSE;
    }

    BufferSizeNeeded = 0;
    if (!YoriLibRtfGenerateTextString(&TextString, &BufferSizeNeeded, StringBuffer, BufferLength)) {
        YoriLibFreeStringContents(&TextString);
        return FALSE;
    }

    if (!YoriLibRtfCvtAppendWithReallocate(Context->RtfText, &TextString)) {
        YoriLibFreeStringContents(&TextString);
        return FALSE;
    }

    YoriLibFreeStringContents(&TextString);
    return TRUE;
}


/**
 Parse a VT100 escape sequence and generate the correct output for RTF.

 @param hOutput The output context to populate with the translated escape
        information.

 @param StringBuffer Pointer to a string buffer containing the escape to
        process.

 @param BufferLength Indicates the number of characters in StringBuffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibRtfCnvProcessAndOutputEscape(
    __in HANDLE hOutput,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    YORI_STRING TextString;
    DWORD BufferSizeNeeded;
    PYORI_LIB_RTF_CONVERT_CONTEXT Context = (PYORI_LIB_RTF_CONVERT_CONTEXT)hOutput;

    YoriLibInitEmptyString(&TextString);
    BufferSizeNeeded = 0;
    if (!YoriLibRtfGenerateEscapeString(&TextString, &BufferSizeNeeded, StringBuffer, BufferLength)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&TextString, BufferSizeNeeded)) {
        return FALSE;
    }

    BufferSizeNeeded = 0;
    if (!YoriLibRtfGenerateEscapeString(&TextString, &BufferSizeNeeded, StringBuffer, BufferLength)) {
        YoriLibFreeStringContents(&TextString);
        return FALSE;
    }

    if (!YoriLibRtfCvtAppendWithReallocate(Context->RtfText, &TextString)) {
        YoriLibFreeStringContents(&TextString);
        return FALSE;
    }

    YoriLibFreeStringContents(&TextString);
    return TRUE;
}


/**
 Convert a Yori string containing VT100 text into RTF with the specified
 format.

 @param VtText Pointer to the string to convert.

 @param RtfText On successful completion, updated to point to an RTF
        representation.  This string will be reallocated within this routine.

 @param ColorTable Pointer to a color table describing how to convert the 16
        colors into RGB.  If NULL, a default mapping is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibRtfConvertToRtfFromVt(
    __in PYORI_STRING VtText,
    __inout PYORI_STRING RtfText,
    __in_opt PDWORD ColorTable
    )
{
    YORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions;
    YORI_LIB_RTF_CONVERT_CONTEXT Context;
    BOOL FreeColorTable = FALSE;

    Context.RtfText = RtfText;
    Context.ColorTable = ColorTable;
    if (ColorTable == NULL) {
        if (YoriLibCaptureConsoleColorTable(&Context.ColorTable, NULL)) {
            FreeColorTable = TRUE;
        } else {
            Context.ColorTable = YoriLibDefaultColorTable;
        }
    }

    CallbackFunctions.InitializeStream = YoriLibRtfCnvInitializeStream;
    CallbackFunctions.EndStream = YoriLibRtfCnvEndStream;
    CallbackFunctions.ProcessAndOutputText = YoriLibRtfCnvProcessAndOutputText;
    CallbackFunctions.ProcessAndOutputEscape = YoriLibRtfCnvProcessAndOutputEscape;

    if (!YoriLibRtfCnvInitializeStream((HANDLE)&Context)) {
        if (FreeColorTable) {
            YoriLibDereference(Context.ColorTable);
        }
        return FALSE;
    }

    if (!YoriLibProcessVtEscapesOnOpenStream(VtText->StartOfString,
                                             VtText->LengthInChars,
                                             (HANDLE)&Context,
                                             &CallbackFunctions)) {

        if (FreeColorTable) {
            YoriLibDereference(Context.ColorTable);
        }
        return FALSE;
    }

    if (!YoriLibRtfCnvEndStream((HANDLE)&Context)) {
        if (FreeColorTable) {
            YoriLibDereference(Context.ColorTable);
        }
        return FALSE;
    }

    if (FreeColorTable) {
        YoriLibDereference(Context.ColorTable);
    }
    return TRUE;
}

// vim:sw=4:ts=4:et:
