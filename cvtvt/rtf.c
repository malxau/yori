/**
 * @file cvtvt/rtf.c
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

#include "cvtvt.h"

/**
 While parsing, TRUE if underlining is in effect.  This global is updated
 when text is generated.
 */
BOOLEAN CvtvtRtfUnderlineOn = FALSE;

/**
 Indicate the beginning of a stream and perform any initial output.

 @param hOutput The stream to output any footer information to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtRtfInitializeStream(
    __in HANDLE hOutput
    )
{
    YORI_STRING OutputString;

    YoriLibInitEmptyString(&OutputString);
    if (!YoriLibRtfGenerateInitialString(&OutputString, NULL)) {
        return FALSE;
    }

    YoriLibOutputTextToMultibyteDevice(hOutput, OutputString.StartOfString, OutputString.LengthInChars);
    YoriLibFreeStringContents(&OutputString);
    return TRUE;
}

/**
 Indicate the end of the stream has been reached and perform any final
 output.

 @param hOutput The stream to output any footer information to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtRtfEndStream(
    __in HANDLE hOutput
    )
{
    YORI_STRING OutputString;
    YoriLibInitEmptyString(&OutputString);

    if (!YoriLibRtfGenerateEndString(&OutputString)) {
        return FALSE;
    }

    YoriLibOutputTextToMultibyteDevice(hOutput, OutputString.StartOfString, OutputString.LengthInChars);
    YoriLibFreeStringContents(&OutputString);

    return TRUE;
}


/**
 Parse text between VT100 escape sequences and generate correct output for
 RTF.

 @param hOutput The output stream to populate with the translated text
        information.

 @param StringBuffer Pointer to a string buffer containing the text to
        process.

 @param BufferLength Indicates the number of characters in StringBuffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtRtfProcessAndOutputText(
    __in HANDLE hOutput,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    )
{

    YORI_STRING TextString;
    DWORD BufferSizeNeeded;

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

    YoriLibOutputTextToMultibyteDevice(hOutput, TextString.StartOfString, TextString.LengthInChars);

    YoriLibFreeStringContents(&TextString);
    return TRUE;
}


/**
 Parse a VT100 escape sequence and generate the correct output for RTF.

 @param hOutput The output stream to populate with the translated escape
        information.

 @param StringBuffer Pointer to a string buffer containing the escape to
        process.

 @param BufferLength Indicates the number of characters in StringBuffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtRtfProcessAndOutputEscape(
    __in HANDLE hOutput,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    YORI_STRING TextString;
    DWORD BufferSizeNeeded;
    BOOLEAN DummyUnderlineState;

    YoriLibInitEmptyString(&TextString);
    BufferSizeNeeded = 0;
    DummyUnderlineState = CvtvtRtfUnderlineOn;
    if (!YoriLibRtfGenerateEscapeString(&TextString, &BufferSizeNeeded, StringBuffer, BufferLength, &DummyUnderlineState)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&TextString, BufferSizeNeeded)) {
        return FALSE;
    }

    BufferSizeNeeded = 0;
    if (!YoriLibRtfGenerateEscapeString(&TextString, &BufferSizeNeeded, StringBuffer, BufferLength, &CvtvtRtfUnderlineOn)) {
        YoriLibFreeStringContents(&TextString);
        return FALSE;
    }

    YoriLibOutputTextToMultibyteDevice(hOutput, TextString.StartOfString, TextString.LengthInChars);

    YoriLibFreeStringContents(&TextString);
    return TRUE;
}

/**
 Set parsing functions to generate RTF output.

 @param CallbackFunctions Function pointers to populate with those that can
        generate RTF output.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtRtfSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    )
{
    CallbackFunctions->InitializeStream = CvtvtRtfInitializeStream;
    CallbackFunctions->EndStream = CvtvtRtfEndStream;
    CallbackFunctions->ProcessAndOutputText = CvtvtRtfProcessAndOutputText;
    CallbackFunctions->ProcessAndOutputEscape = CvtvtRtfProcessAndOutputEscape;
    return TRUE;
}

// vim:sw=4:ts=4:et:
