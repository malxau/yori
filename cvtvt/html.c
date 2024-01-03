/**
 * @file cvtvt/html.c
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

#include "cvtvt.h"

/**
 The context recording state while generation is in progress, including the
 HTML dialect to generate.
 */
YORILIB_HTML_GENERATE_CONTEXT CvtvtHtmlGenerateContext;

/**
 Indicate the beginning of a stream and perform any initial output.

 @param hOutput The stream to output any footer information to.

 @param Context Pointer to context (unused.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtHtmlInitializeStream(
    __in HANDLE hOutput,
    __inout PDWORDLONG Context
    )
{
    YORI_STRING OutputString;

    UNREFERENCED_PARAMETER(Context);

    YoriLibInitEmptyString(&OutputString);
    if (!YoriLibHtmlGenerateInitialString(&OutputString, &CvtvtHtmlGenerateContext)) {
        return FALSE;
    }

    YoriLibOutputTextToMultibyteDevice(hOutput, &OutputString);
    YoriLibFreeStringContents(&OutputString);
    return TRUE;
}

/**
 Indicate the end of the stream has been reached and perform any final
 output.

 @param hOutput The stream to output any footer information to.

 @param Context Pointer to context (unused.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtHtmlEndStream(
    __in HANDLE hOutput,
    __inout PDWORDLONG Context
    )
{
    YORI_STRING OutputString;
    UNREFERENCED_PARAMETER(Context);

    YoriLibInitEmptyString(&OutputString);

    if (!YoriLibHtmlGenerateEndString(&OutputString, &CvtvtHtmlGenerateContext)) {
        return FALSE;
    }

    YoriLibOutputTextToMultibyteDevice(hOutput, &OutputString);
    YoriLibFreeStringContents(&OutputString);

    return TRUE;
}


/**
 Parse text between VT100 escape sequences and generate correct output for
 either HTML4 or HTML5.

 @param hOutput The output stream to populate with the translated text
        information.

 @param String Pointer to a string buffer containing the text to process.

 @param Context Pointer to context (unused.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtHtmlProcessAndOutputText(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PDWORDLONG Context
    )
{
    YORI_STRING TextString;
    YORI_ALLOC_SIZE_T BufferSizeNeeded;

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

    YoriLibOutputTextToMultibyteDevice(hOutput, &TextString);

    YoriLibFreeStringContents(&TextString);
    return TRUE;
}


/**
 Parse a VT100 escape sequence and generate the correct output for either
 HTML4 or HTML5.

 @param hOutput The output stream to populate with the translated escape
        information.

 @param String Pointer to a string buffer containing the escape to process.

 @param Context Pointer to context (unused.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CvtvtHtmlProcessAndOutputEscape(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PDWORDLONG Context
    )
{
    YORI_STRING TextString;
    YORI_ALLOC_SIZE_T BufferSizeNeeded;
    YORILIB_HTML_GENERATE_CONTEXT DummyGenerateContext;

    UNREFERENCED_PARAMETER(Context);

    YoriLibInitEmptyString(&TextString);
    BufferSizeNeeded = 0;
    memcpy(&DummyGenerateContext, &CvtvtHtmlGenerateContext, sizeof(YORILIB_HTML_GENERATE_CONTEXT));
    if (!YoriLibHtmlGenerateEscapeString(&TextString, &BufferSizeNeeded, String, &DummyGenerateContext)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&TextString, BufferSizeNeeded)) {
        return FALSE;
    }

    BufferSizeNeeded = 0;
    if (!YoriLibHtmlGenerateEscapeString(&TextString, &BufferSizeNeeded, String, &CvtvtHtmlGenerateContext)) {
        YoriLibFreeStringContents(&TextString);
        return FALSE;
    }

    YoriLibOutputTextToMultibyteDevice(hOutput, &TextString);

    YoriLibFreeStringContents(&TextString);
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
    CvtvtHtmlGenerateContext.HtmlVersion = 4;
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
    CvtvtHtmlGenerateContext.HtmlVersion = 5;
    CallbackFunctions->InitializeStream = CvtvtHtmlInitializeStream;
    CallbackFunctions->EndStream = CvtvtHtmlEndStream;
    CallbackFunctions->ProcessAndOutputText = CvtvtHtmlProcessAndOutputText;
    CallbackFunctions->ProcessAndOutputEscape = CvtvtHtmlProcessAndOutputEscape;
    return TRUE;
}

// vim:sw=4:ts=4:et:
