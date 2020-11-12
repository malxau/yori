/**
 * @file lib/vt.c
 *
 * Convert VT100/ANSI escape sequences into other formats, including the 
 * console.
 *
 * Copyright (c) 2015-20 Malcolm J. Smith
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
 The color to use when a VT100 reset command is issued.
 */
#define DEFAULT_COLOR 7

/**
 The color to use when a VT100 reset command is issued.
 */
WORD YoriLibVtResetColor;

/**
 True if the value for YoriLibVtResetColor has already been determined.  FALSE
 if the value has not yet been determined, and will be queried before changing
 the console color.
 */
BOOLEAN YoriLibVtResetColorSet;

/**
 The line ending to apply when writing to a file.  Note that this is not
 a dynamic allocation and will never be freed.  It is a pointer that can
 point to constant data within any process, which seems sufficient for the
 very small number of expected values.
 */
LPTSTR YoriLibVtLineEnding = _T("\r\n");

/**
 Set the default color for the process.  The default color is the one that
 will be used when a reset command is issued to the terminal.  For most
 processes, this is the color that was active when the process started,
 but it can be changed later with this function.

 @param NewDefaultColor The new default color, in Win32 format.
 */
VOID
YoriLibVtSetDefaultColor(
    __in WORD NewDefaultColor
    )
{
    YoriLibVtResetColor = NewDefaultColor;
    YoriLibVtResetColorSet = TRUE;
}

/**
 Return the current default color for the process.

 @return The current default color for the process.
 */
WORD
YoriLibVtGetDefaultColor()
{
    if (YoriLibVtResetColorSet) {
        return YoriLibVtResetColor;
    } else {
        CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ConsoleInfo)) {
            YoriLibVtResetColor = ConsoleInfo.wAttributes;
            YoriLibVtResetColorSet = TRUE;
            return YoriLibVtResetColor;
        }
    }
    return DEFAULT_COLOR;
}

/**
 Set the line ending to apply when writing to a file.  Note that this is not
 a dynamic allocation and will never be freed.  It is a pointer that can
 point to constant data within any process, which seems sufficient for the
 very small number of expected values.

 @param LineEnding Pointer to the new line ending string.
 */
VOID
YoriLibVtSetLineEnding(
    __in LPTSTR LineEnding
    )
{
    YoriLibVtLineEnding = LineEnding;
}

/**
 Get the line ending to apply when writing to a file.  Note that this is not
 a dynamic allocation and will never be freed.  It is a pointer that can
 point to constant data within any process, which seems sufficient for the
 very small number of expected values.

 @return The current line ending string.
 */
LPTSTR
YoriLibVtGetLineEnding()
{
    return YoriLibVtLineEnding;
}

/**
 Convert any incoming string to the active output encoding, and send it to
 the output device.

 @param hOutput Handle to the device to receive any output.

 @param StringBuffer Pointer to the string to output which is in host (UTF16)
        encoding.

 @param BufferLength Length of StringBuffer, in characters.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibOutputTextToMultibyteDevice(
    __in HANDLE hOutput,
    __in LPCTSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    DWORD  BytesTransferred;
    BOOL Result;

#ifdef UNICODE
    {
        CHAR ansi_stack_buf[64 + 1];
        DWORD AnsiBytesNeeded;
        LPSTR ansi_buf;

        AnsiBytesNeeded = YoriLibGetMultibyteOutputSizeNeeded(StringBuffer, BufferLength);
    
        if (AnsiBytesNeeded > (int)sizeof(ansi_stack_buf)) {
            ansi_buf = YoriLibMalloc(AnsiBytesNeeded);
        } else {
            ansi_buf = ansi_stack_buf;
        }

        if (ansi_buf != NULL) {
            YoriLibMultibyteOutput(StringBuffer,
                                   BufferLength,
                                   ansi_buf,
                                   AnsiBytesNeeded);

            Result = WriteFile(hOutput, ansi_buf, AnsiBytesNeeded, &BytesTransferred, NULL);

            if (ansi_buf != ansi_stack_buf) {
                YoriLibFree(ansi_buf);
            }
        } else {
            Result = FALSE;
            BufferLength = 0;
        }
    }
#else
    Result = WriteFile(hOutput,StringBuffer,BufferLength*sizeof(TCHAR),&BytesTransferred,NULL);
#endif
    return Result;
}

/**
 Convert any incoming string to contain specified line endings, and pass the
 result for conversion into the active output encoding.

 @param hOutput Handle to the device to receive any output.

 @param StringBuffer Pointer to the string to output, which can have any line
        ending and is in host (UTF16) encoding.

 @param BufferLength Length of StringBuffer, in characters.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibOutputTextToMultibyteNormalizeLineEnding(
    __in HANDLE hOutput,
    __in LPCTSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    YORI_STRING SearchString;
    DWORD NonLineEndLength;
    DWORD CharsToIgnore;
    BOOL GenerateLineEnd;

    SearchString.StartOfString = (LPTSTR)StringBuffer;
    SearchString.LengthInChars = BufferLength;

    while(TRUE) {
        NonLineEndLength = YoriLibCountStringNotContainingChars(&SearchString, _T("\r\n"));

        CharsToIgnore = 0;
        GenerateLineEnd = FALSE;

        if (NonLineEndLength + 2 <= SearchString.LengthInChars &&
            SearchString.StartOfString[NonLineEndLength] == '\r' &&
            SearchString.StartOfString[NonLineEndLength + 1] == '\n') {

            CharsToIgnore = 0;
            NonLineEndLength += 2;

        } else if (NonLineEndLength + 1 <= SearchString.LengthInChars) {
            if (SearchString.StartOfString[NonLineEndLength] == '\r' ||
                SearchString.StartOfString[NonLineEndLength] == '\n') {

                CharsToIgnore = 1;
                GenerateLineEnd = TRUE;
            }
        }

        if (NonLineEndLength > 0 &&
            !YoriLibOutputTextToMultibyteDevice(hOutput, SearchString.StartOfString, NonLineEndLength)) {

            return FALSE;
        }

        if (GenerateLineEnd) {
            if (!YoriLibOutputTextToMultibyteDevice(hOutput, YoriLibVtLineEnding, _tcslen(YoriLibVtLineEnding))) {
                return FALSE;
            }
        }

        if (NonLineEndLength + CharsToIgnore >= SearchString.LengthInChars) {
            break;
        }
        SearchString.StartOfString = &SearchString.StartOfString[NonLineEndLength + CharsToIgnore];
        SearchString.LengthInChars -= NonLineEndLength + CharsToIgnore;
    }

    return TRUE;
}


//
//  YoriLibConsole functions
//

/**
 Initialize the output stream with any header information.  For console
 output, this is pointless.

 @param hOutput The output stream to initialize.

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibConsoleInitializeStream(
    HANDLE hOutput
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    
    return TRUE;
}

/**
 End processing for the specified stream.  For console output, this is
 pointless.

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibConsoleEndStream(
    HANDLE hOutput
    )
{
    UNREFERENCED_PARAMETER(hOutput);

    return TRUE;
}

/**
 Output text between escapes to the output Console.

 @param hOutput Handle to the output console.

 @param StringBuffer Pointer to the string to output.

 @param BufferLength Number of characters in the string.

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibConsoleProcessAndOutputText(
    HANDLE hOutput,
    LPTSTR StringBuffer,
    DWORD BufferLength
    )
{
    DWORD  BytesTransferred;

    UNREFERENCED_PARAMETER(hOutput);

    WriteConsole(hOutput,StringBuffer,BufferLength,&BytesTransferred,NULL);
    return TRUE;
}

/**
 A callback function to receive an escape and ignore it.
 
 @param hOutput Handle to the output console.

 @param StringBuffer Pointer to a buffer describing the escape.

 @param BufferLength The number of characters in the escape.

 @return TRUE for success, FALSE for failure.  Note this particular variant
         can't exactly fail.
 */
BOOL
YoriLibConsoleProcessAndIgnoreEscape(
    HANDLE hOutput,
    LPTSTR StringBuffer,
    DWORD BufferLength
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(StringBuffer);
    UNREFERENCED_PARAMETER(BufferLength);

    return TRUE;
}

/**
 Given a starting color and a VT sequence which may change it, generate the
 final color.  Both colors are in Win32 attribute form.

 @param InitialColor The starting color, in Win32 form.

 @param EscapeSequence The VT100 sequence to apply to the starting color.

 @param FinalColor On successful completion, updated to contain the final
        color.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
BOOL
YoriLibVtFinalColorFromSequence(
    __in WORD InitialColor,
    __in PYORI_STRING EscapeSequence,
    __out PWORD FinalColor
    )
{
    LPTSTR CurrentPoint = EscapeSequence->StartOfString;
    DWORD CurrentOffset = 0;
    DWORD RemainingLength;
    WORD  NewColor;
    YORI_STRING SearchString;

    RemainingLength = EscapeSequence->LengthInChars;
    NewColor = InitialColor;

    //
    //  We expect an escape initiator (two chars) and a 'm' for color
    //  formatting.  This should already be validated, the check here
    //  is redundant.
    //

    if (EscapeSequence->LengthInChars >= 3 &&
        CurrentPoint[EscapeSequence->LengthInChars - 1] == 'm') {

        DWORD code;
        BOOLEAN NewUnderline = FALSE;
        DWORD ColorTable[] = {0,
                              FOREGROUND_RED,
                              FOREGROUND_GREEN,
                              FOREGROUND_RED|FOREGROUND_GREEN,
                              FOREGROUND_BLUE,
                              FOREGROUND_BLUE|FOREGROUND_RED,
                              FOREGROUND_BLUE|FOREGROUND_GREEN,
                              FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED};

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
                NewColor = YoriLibVtResetColor;
                NewUnderline = FALSE;
            } else if (code == 1) {
                NewColor |= FOREGROUND_INTENSITY;
            } else if (code == 4) {
                NewUnderline = TRUE;
            } else if (code == 7) {
                NewColor = (WORD)(((NewColor & 0xf) << 4) | ((NewColor & 0xf0) >> 4));
            } else if (code == 39) {
                NewColor = (WORD)((NewColor & ~(0xf)) | (YoriLibVtResetColor & 0xf));
            } else if (code == 49) {
                NewColor = (WORD)((NewColor & ~(0xf0)) | (YoriLibVtResetColor & 0xf0));
            } else if (code >= 30 && code <= 37) {
                NewColor = (WORD)((NewColor & ~(0xf)) | ColorTable[code - 30]);
            } else if (code >= 40 && code <= 47) {
                NewColor = (WORD)((NewColor & ~(0xf0)) | (ColorTable[code - 40]<<4));
            } else if (code >= 90 && code <= 97) {
                NewColor = (WORD)((NewColor & ~(0xf)) | FOREGROUND_INTENSITY | ColorTable[code - 90]);
            } else if (code >= 100 && code <= 107) {
                NewColor = (WORD)((NewColor & ~(0xf0)) | BACKGROUND_INTENSITY | (ColorTable[code - 100]<<4));
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

        if (NewUnderline) {
            NewColor |= COMMON_LVB_UNDERSCORE;
        }
    }

    *FinalColor = NewColor;

    return TRUE;
}

/**
 A callback function to receive an escape and translate it into the
 appropriate Win32 console action.
 
 @param hOutput Handle to the output console.

 @param StringBuffer Pointer to a buffer describing the escape.

 @param BufferLength The number of characters in the escape.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibConsoleProcessAndOutputEscape(
    __in HANDLE hOutput,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    YORI_STRING EscapeCode;
    WORD NewColor;

    ConsoleInfo.wAttributes = DEFAULT_COLOR;
    GetConsoleScreenBufferInfo(hOutput, &ConsoleInfo);
    NewColor = ConsoleInfo.wAttributes;

    if (!YoriLibVtResetColorSet) {
        YoriLibVtResetColor = ConsoleInfo.wAttributes;
        YoriLibVtResetColorSet = TRUE;
    }

    YoriLibInitEmptyString(&EscapeCode);
    EscapeCode.StartOfString = StringBuffer;
    EscapeCode.LengthInChars = BufferLength;

    if (YoriLibVtFinalColorFromSequence(NewColor, &EscapeCode, &NewColor)) {

        SetConsoleTextAttribute(hOutput, NewColor);
    }
    return TRUE;
}

/**
 Initialize callback functions to a set which will output all text, and 
 convert any escape sequences into Win32 console commands.

 @param CallbackFunctions The callback functions to initialize.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibConsoleSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    )
{
    CallbackFunctions->InitializeStream = YoriLibConsoleInitializeStream;
    CallbackFunctions->EndStream = YoriLibConsoleEndStream;
    CallbackFunctions->ProcessAndOutputText = YoriLibConsoleProcessAndOutputText;
    CallbackFunctions->ProcessAndOutputEscape = YoriLibConsoleProcessAndOutputEscape;
    return TRUE;
}

/**
 Initialize callback functions to a set which will output all text, and remove
 any escape sequences.

 @param CallbackFunctions The callback functions to initialize.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibConsoleNoEscapeSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    )
{
    CallbackFunctions->InitializeStream = YoriLibConsoleInitializeStream;
    CallbackFunctions->EndStream = YoriLibConsoleEndStream;
    CallbackFunctions->ProcessAndOutputText = YoriLibConsoleProcessAndOutputText;
    CallbackFunctions->ProcessAndOutputEscape = YoriLibConsoleProcessAndIgnoreEscape;
    return TRUE;
}

/**
 Initialize callback functions to a set which will output all text, and output
 escapes without any processing.

 @param CallbackFunctions The callback functions to initialize.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibConsoleIncludeEscapeSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    )
{
    CallbackFunctions->InitializeStream = YoriLibConsoleInitializeStream;
    CallbackFunctions->EndStream = YoriLibConsoleEndStream;
    CallbackFunctions->ProcessAndOutputText = YoriLibConsoleProcessAndOutputText;
    CallbackFunctions->ProcessAndOutputEscape = YoriLibConsoleProcessAndOutputText;
    return TRUE;
}

//
//  Text functions
//

/**
 Initialize the output stream with any header information.  For text
 output, this is pointless.

 @param hOutput The output stream to initialize.

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibUtf8TextInitializeStream(
    __in HANDLE hOutput
    )
{
    UNREFERENCED_PARAMETER(hOutput);

    return TRUE;
}

/**
 End processing for the specified stream.  For text output, this is
 pointless.

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibUtf8TextEndStream(
    __in HANDLE hOutput
    )
{
    UNREFERENCED_PARAMETER(hOutput);

    return TRUE;
}

/**
 Output text between escapes to the output device.

 @param hOutput Handle to the output device.

 @param StringBuffer Pointer to the string to ouput.

 @param BufferLength Number of characters in the string.

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibUtf8TextProcessAndOutputText(
    __in HANDLE hOutput,
    __in LPTSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    return YoriLibOutputTextToMultibyteNormalizeLineEnding(hOutput, StringBuffer, BufferLength);
}

/**
 A dummy callback function to receive an escape and not do anything with it.
 
 @param hOutput Handle to the output device (ignored.)

 @param StringBuffer Pointer to a buffer describing the escape (ignored.)

 @param BufferLength The number of characters in the escape (ignored.)

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibUtf8TextProcessAndOutputEscape(
    HANDLE hOutput,
    LPTSTR StringBuffer,
    DWORD BufferLength
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(StringBuffer);
    UNREFERENCED_PARAMETER(BufferLength);

    return TRUE;
}

/**
 Initialize callback functions to a set which will output all text, and remove
 any escape sequences.

 @param CallbackFunctions The callback functions to initialize.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibUtf8TextNoEscapesSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    )
{
    CallbackFunctions->InitializeStream = YoriLibUtf8TextInitializeStream;
    CallbackFunctions->EndStream = YoriLibUtf8TextEndStream;
    CallbackFunctions->ProcessAndOutputText = YoriLibUtf8TextProcessAndOutputText;
    CallbackFunctions->ProcessAndOutputEscape = YoriLibUtf8TextProcessAndOutputEscape;
    return TRUE;
}

/**
 Initialize callback functions to a set which will output all text, and include
 escape sequences verbatim without further processing.

 @param CallbackFunctions The callback functions to initialize.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibUtf8TextWithEscapesSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    )
{
    CallbackFunctions->InitializeStream = YoriLibUtf8TextInitializeStream;
    CallbackFunctions->EndStream = YoriLibUtf8TextEndStream;
    CallbackFunctions->ProcessAndOutputText = YoriLibUtf8TextProcessAndOutputText;
    CallbackFunctions->ProcessAndOutputEscape = YoriLibUtf8TextProcessAndOutputText;
    return TRUE;
}

/**
 Walk through an input string and process any VT100/ANSI escapes by invoking
 a device specific callback function to perform the requested action.

 @param String Pointer to the string to process.

 @param StringLength The length of the string, in characters.

 @param hOutput A handle to the device to output the result to.

 @param Callbacks Pointer to a block of callback functions to invoke when
        escape sequences or text is encountered.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibProcessVtEscapesOnOpenStream(
    __in LPTSTR String,
    __in DWORD StringLength,
    __in HANDLE hOutput,
    __in PYORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks
    )
{
    LPTSTR CurrentPoint;
    DWORD CurrentOffset;
    DWORD PreviouslyConsumed;
    TCHAR VtEscape[] = {27, '\0'};
    YORI_STRING SearchString;

    CurrentPoint = String;
    YoriLibInitEmptyString(&SearchString);
    SearchString.StartOfString = CurrentPoint;
    SearchString.LengthInChars = StringLength;
    CurrentOffset = YoriLibCountStringNotContainingChars(&SearchString, VtEscape);
    PreviouslyConsumed = 0;

    while (TRUE) {

        //
        //  If we have any text, perform required processing and output the text
        //

        if (CurrentOffset > 0) {
            if (!Callbacks->ProcessAndOutputText(hOutput, CurrentPoint, CurrentOffset)) {
                return FALSE;
            }

            CurrentPoint = CurrentPoint + CurrentOffset;
            PreviouslyConsumed += CurrentOffset;
        }

        if (PreviouslyConsumed >= StringLength) {
            break;
        }

        //
        //  Locate VT100 escapes and process those
        //

        if (*CurrentPoint == 27) {

            if (PreviouslyConsumed + 2 < StringLength &&
                CurrentPoint[1] == '[') {
    
                DWORD EndOfEscape;
                SearchString.StartOfString = &CurrentPoint[2];
                SearchString.LengthInChars = StringLength - PreviouslyConsumed - 2;
                EndOfEscape = YoriLibCountStringContainingChars(&SearchString, _T("0123456789;"));

                //
                //  If our buffer is full and we still have an incomplete escape,
                //  there's no more processing we can perform.  This input is
                //  bogus.
                //

                if (PreviouslyConsumed == 0 && EndOfEscape == StringLength - 2) {
                    return FALSE;
                }

                //
                //  If we have an incomplete escape for this chunk, stop
                //  processing here, loop back and read more data.
                //

                if (EndOfEscape == StringLength - PreviouslyConsumed - 2) {
                    break;
                }

                if (!Callbacks->ProcessAndOutputEscape(hOutput, CurrentPoint, EndOfEscape + 3)) {
                    return FALSE;
                }
                CurrentPoint = CurrentPoint + EndOfEscape + 3;
                PreviouslyConsumed += EndOfEscape + 3;
            } else {

                //
                //  Output just the escape character and move to the next
                //  match
                //

                if (!Callbacks->ProcessAndOutputText(hOutput, CurrentPoint, 1)) {
                    return FALSE;
                }
                CurrentPoint++;
                PreviouslyConsumed++;
            }
        }

        if (PreviouslyConsumed >= StringLength) {
            break;
        }

        SearchString.StartOfString = CurrentPoint;
        SearchString.LengthInChars = StringLength - PreviouslyConsumed;
        CurrentOffset = YoriLibCountStringNotContainingChars(&SearchString, VtEscape);
    }

    return TRUE;
}


/**
 Given an input string of specified length, process all VT100 escape sequences
 by calling callback functions that do the appropriate thing for the given
 output device.  There is one callback for processing the escape sequence, and
 one callback for the text between escapes.

 @param String The string to process.

 @param StringLength The number of characters in the string.

 @param hOutput The device to output the result to.

 @param Callbacks The callback functions to invoke when generating the result.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibProcessVtEscapesOnNewStream(
    __in LPTSTR String,
    __in DWORD StringLength,
    __in HANDLE hOutput,
    __in PYORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks
    )
{
    BOOL Result;

    Callbacks->InitializeStream(hOutput);

    Result = YoriLibProcessVtEscapesOnOpenStream(String, StringLength, hOutput, Callbacks);

    Callbacks->EndStream(hOutput);
    return Result;
}

/**
 Output a printf-style formatted string to the specified output stream.

 @param hOut The output stream to write any result to.

 @param Flags Flags, indicating behavior.

 @param szFmt The format string, followed by appropriate arguments.

 @param marker The arguments that correspond to the format string.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibOutputInternal(
    __in HANDLE hOut,
    __in DWORD Flags,
    __in LPCTSTR szFmt,
    __in va_list marker
    )
{
    va_list savedmarker = marker;
    int len;
    TCHAR stack_buf[64];
    TCHAR * buf;
    YORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks;
    DWORD CurrentMode;
    BOOL Result;

    //
    //  Check if we're writing to a console supporting color or a file
    //  that doesn't
    //

    if (GetConsoleMode(hOut, &CurrentMode)) {
        if ((Flags & YORI_LIB_OUTPUT_STRIP_VT) != 0) {
            YoriLibConsoleNoEscapeSetFunctions(&Callbacks);
        } else if ((Flags & YORI_LIB_OUTPUT_PASSTHROUGH_VT) != 0) {
            YoriLibConsoleIncludeEscapeSetFunctions(&Callbacks);
        } else {
            YoriLibConsoleSetFunctions(&Callbacks);
        }
    } else if ((Flags & YORI_LIB_OUTPUT_STRIP_VT) != 0) {
        YoriLibUtf8TextNoEscapesSetFunctions(&Callbacks);
    } else {
        YoriLibUtf8TextWithEscapesSetFunctions(&Callbacks);
    }

    len = YoriLibVSPrintfSize(szFmt, marker);

    if (len>(int)(sizeof(stack_buf)/sizeof(stack_buf[0]))) {
        buf = YoriLibMalloc(len * sizeof(TCHAR));
        if (buf == NULL) {
            return 0;
        }
    } else {
        buf = stack_buf;
    }

    marker = savedmarker;
    len = YoriLibVSPrintf(buf, len, szFmt, marker);

    Result = YoriLibProcessVtEscapesOnNewStream(buf, len, hOut, &Callbacks);

    if (buf != stack_buf) {
        YoriLibFree(buf);
    }
    return Result;
}

/**
 Output a printf-style formatted string to the specified output stream.

 @param Flags Flags, indicating the output stream and its behavior.

 @param szFmt The format string, followed by appropriate arguments.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibOutput(
    __in DWORD Flags,
    __in LPCTSTR szFmt,
    ...
    )
{
    va_list marker;
    BOOL Result;
    HANDLE hOut;

    //
    //  Based on caller specification, see which stream we're writing to
    //

    if ((Flags & YORI_LIB_OUTPUT_STDERR) != 0) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    } else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    va_start(marker, szFmt);
    Result = YoriLibOutputInternal(hOut, Flags, szFmt, marker);
    va_end(marker);
    return Result;
}

/**
 Output a Yori string to a specified device.  This will perform ANSI escape
 processing but has no mechanism for expanding extra tokens in the stream.

 @param hOut The output stream to write any result to.

 @param Flags Flags, indicating behavior.

 @param String The string to output.
 
 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibOutputString(
    __in HANDLE hOut,
    __in DWORD Flags,
    __in PYORI_STRING String
    )
{
    YORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks;
    DWORD CurrentMode;
    BOOL Result;

    //
    //  Check if we're writing to a console supporting color or a file
    //  that doesn't
    //

    if (GetConsoleMode(hOut, &CurrentMode)) {
        if ((Flags & YORI_LIB_OUTPUT_STRIP_VT) != 0) {
            YoriLibConsoleNoEscapeSetFunctions(&Callbacks);
        } else if ((Flags & YORI_LIB_OUTPUT_PASSTHROUGH_VT) != 0) {
            YoriLibConsoleIncludeEscapeSetFunctions(&Callbacks);
        } else {
            YoriLibConsoleSetFunctions(&Callbacks);
        }
    } else if ((Flags & YORI_LIB_OUTPUT_STRIP_VT) != 0) {
        YoriLibUtf8TextNoEscapesSetFunctions(&Callbacks);
    } else {
        YoriLibUtf8TextWithEscapesSetFunctions(&Callbacks);
    }

    Result = YoriLibProcessVtEscapesOnNewStream(String->StartOfString, String->LengthInChars, hOut, &Callbacks);

    return Result;
}

/**
 Output a printf-style formatted string to the specified output stream.

 @param hOut The output stream to write any result to.

 @param Flags Flags, indicating behavior.

 @param szFmt The format string, followed by appropriate arguments.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibOutputToDevice(
    __in HANDLE hOut,
    __in DWORD Flags,
    __in LPCTSTR szFmt,
    ...
    )
{
    va_list marker;
    BOOL Result;

    va_start(marker, szFmt);
    Result = YoriLibOutputInternal(hOut, Flags, szFmt, marker);
    va_end(marker);
    return Result;
}

/**
 Generate a string that is the VT100 representation for the specified Win32
 attribute.

 @param String On successful completion, updated to contain the VT100
        representation for the attribute.  This string can be reallocated
        within this routine.

 @param Ctrl The control part of the attribute that specifies whether the
        default window foreground or background should be applied.

 @param Attribute The attribute to convert to a VT100 escape sequence.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibVtStringForTextAttribute(
    __inout PYORI_STRING String,
    __in UCHAR Ctrl,
    __in WORD Attribute
    )
{
    CHAR  AnsiForeground;
    CHAR  AnsiBackground;

    if (String->LengthAllocated < YORI_MAX_INTERNAL_VT_ESCAPE_CHARS) {
        YoriLibFreeStringContents(String);
        if (!YoriLibAllocateString(String, YORI_MAX_INTERNAL_VT_ESCAPE_CHARS)) {
            return FALSE;
        }
    }

    if (Ctrl & YORILIB_ATTRCTRL_WINDOW_BG) {
        AnsiBackground = 49;
    } else {
        AnsiBackground = (CHAR)YoriLibWindowsToAnsi((Attribute>>4)&7);
        if (Attribute & BACKGROUND_INTENSITY) {
            AnsiBackground += 100;
        } else {
            AnsiBackground += 40;
        }
    }

    if (Ctrl & YORILIB_ATTRCTRL_WINDOW_FG) {
        AnsiForeground = 39;
    } else {
        AnsiForeground = (CHAR)YoriLibWindowsToAnsi(Attribute&7);
        AnsiForeground += 30;
    }

    String->LengthInChars = YoriLibSPrintfS(String->StartOfString,
                                            String->LengthAllocated,
                                            _T("%c[0;%i;%i%sm"),
                                            27,
                                            AnsiBackground,
                                            AnsiForeground,
                                            (Attribute & FOREGROUND_INTENSITY)?_T(";1"):_T("")
                                            );

    return TRUE;
}

/**
 Change the console to output a specified Win32 color code by emitting an
 ANSI escape sequence and processing it if the output device is a console,
 or leaving it in the output stream for other types of output.

 @param hOut The output device.

 @param Flags Flags indicating output behavior.

 @param Ctrl The control part of the attribute that specifies whether the
        default window foreground or background should be applied.

 @param Attribute The Win32 color code to make active.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibVtSetConsoleTextAttributeOnDevice(
    __in HANDLE hOut,
    __in DWORD Flags,
    __in UCHAR Ctrl,
    __in WORD Attribute
    )
{
    TCHAR OutputStringBuffer[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];
    YORI_STRING OutputString;

    YoriLibInitEmptyString(&OutputString);
    OutputString.StartOfString = OutputStringBuffer;
    OutputString.LengthAllocated = sizeof(OutputStringBuffer)/sizeof(OutputStringBuffer[0]);
    OutputString.StartOfString[0] = '\0';

    //
    //  Because the buffer is on the stack, this call shouldn't
    //  fail.
    //

    if (!YoriLibVtStringForTextAttribute(&OutputString, Ctrl, Attribute)) {
        ASSERT(FALSE);
        return FALSE;
    }

    //
    //  Now that we've generated the string, output it in the
    //  correct form for the device.
    //

    return YoriLibOutputToDevice(hOut, Flags, OutputString.StartOfString);
}

/**
 An adaptation of the Win32 SetConsoleTextAttribute API that outputs VT100
 escapes to the specified output stream.

 @param Flags Flags, indicating the output stream.

 @param Attribute The Win32 attribute code corresponding to the escape to
        generate.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibVtSetConsoleTextAttribute(
    __in DWORD Flags,
    __in WORD Attribute
    )
{
    HANDLE hOut;
    if ((Flags & YORI_LIB_OUTPUT_STDERR) != 0) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    } else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    return YoriLibVtSetConsoleTextAttributeOnDevice(hOut, Flags, 0, Attribute);
}

/**
 Given a string containing VT100 escapes, remove all escapes and return the
 result in a newly allocated string.

 @param VtText The text, containing VT100 escapes.

 @param PlainText On successful completion, updated to contain a string
        that has the contents of VtText without any escapes.  This string
        will be reallocated within this function if the supplied string
        is too small.
 
 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
BOOL
YoriLibStripVtEscapes(
    __in PYORI_STRING VtText,
    __inout PYORI_STRING PlainText
    )
{
    DWORD CharIndex;
    DWORD DestIndex;
    DWORD EscapeChars;
    DWORD EndOfEscape;
    YORI_STRING EscapeSubset;

    EscapeChars = 0;

    for (CharIndex = 0; CharIndex < VtText->LengthInChars; CharIndex++) {
        if (VtText->LengthInChars > CharIndex + 2 &&
            VtText->StartOfString[CharIndex] == 27 &&
            VtText->StartOfString[CharIndex + 1] == '[') {

            YoriLibInitEmptyString(&EscapeSubset);
            EscapeSubset.StartOfString = &VtText->StartOfString[CharIndex + 2];
            EscapeSubset.LengthInChars = VtText->LengthInChars - CharIndex - 2;
            EndOfEscape = YoriLibCountStringContainingChars(&EscapeSubset, _T("0123456789;"));
            if (VtText->LengthInChars > CharIndex + 2 + EndOfEscape) {
                EscapeChars += 3 + EndOfEscape;
                CharIndex += 2 + EndOfEscape;
            }
        }
    }

    if (PlainText->LengthAllocated < VtText->LengthInChars - EscapeChars + 1) {
        YoriLibFreeStringContents(PlainText);
        if (!YoriLibAllocateString(PlainText, VtText->LengthInChars - EscapeChars + 1)) {
            return FALSE;
        }
    }

    DestIndex = 0;
    for (CharIndex = 0; CharIndex < VtText->LengthInChars; CharIndex++) {
        if (VtText->LengthInChars > CharIndex + 2 &&
            VtText->StartOfString[CharIndex] == 27 &&
            VtText->StartOfString[CharIndex + 1] == '[') {

            YoriLibInitEmptyString(&EscapeSubset);
            EscapeSubset.StartOfString = &VtText->StartOfString[CharIndex + 2];
            EscapeSubset.LengthInChars = VtText->LengthInChars - CharIndex - 2;
            EndOfEscape = YoriLibCountStringContainingChars(&EscapeSubset, _T("0123456789;"));
            if (VtText->LengthInChars > CharIndex + 2 + EndOfEscape) {
                EscapeChars += 3 + EndOfEscape;
                CharIndex += 2 + EndOfEscape;
            }
        } else {
            PlainText->StartOfString[DestIndex] = VtText->StartOfString[CharIndex];
            DestIndex++;
        }
    }

    PlainText->StartOfString[DestIndex] = '\0';
    PlainText->LengthInChars = DestIndex;
    return TRUE;
}

/**
 Query the console for the size of the window.  If the output device is not
 a console, use the COLUMNS and LINES environment variable to indicate the
 user's preference for how to size output.

 @param OutputHandle Handle to the output device.

 @param Width If specified, populated with the width of the console.

 @param Height If specified, populated with the height of the console.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGetWindowDimensions(
    __in HANDLE OutputHandle,
    __out_opt PDWORD Width,
    __out_opt PDWORD Height
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    LONGLONG Temp;

    if (GetConsoleScreenBufferInfo(OutputHandle, &ConsoleInfo)) {
        if (Width != NULL) {
            *Width = ConsoleInfo.srWindow.Right - ConsoleInfo.srWindow.Left + 1;
        }
        if (Height != NULL) {
            *Height = ConsoleInfo.srWindow.Bottom - ConsoleInfo.srWindow.Top + 1;
        }

        return TRUE;
    }

    if (Width != NULL) {
        if (!YoriLibGetEnvironmentVariableAsNumber(_T("COLUMNS"), &Temp)) {
            *Width = 80;
        } else {
            *Width = (DWORD)Temp;
        }
    }

    if (Height != NULL) {
        if (!YoriLibGetEnvironmentVariableAsNumber(_T("LINES"), &Temp)) {
            *Height = 25;
        } else {
            *Height = (DWORD)Temp;
        }
    }

    return TRUE;
}

/**
 Query the capabilities of the console.  If the output is going to the
 console, this is just an API call.  If it's being redirected, parse the
 environment string to determine the output that the user wants.

 @param OutputHandle Handle to the output device.

 @param SupportsColor If specified, set to TRUE to indicate the output can
        handle color escape information.

 @param SupportsExtendedChars If specified, set to TRUE to indicate the
        output can handle UTF8 characters.

 @param SupportsAutoLineWrap If specified, set to TRUE to indicate that text
        will automatically wrap one the end of the window is reached.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibQueryConsoleCapabilities(
    __in HANDLE OutputHandle,
    __out_opt PBOOL SupportsColor,
    __out_opt PBOOL SupportsExtendedChars,
    __out_opt PBOOL SupportsAutoLineWrap
    )
{
    YORI_STRING TermString;
    YORI_STRING SubString;
    LPTSTR NextSeperator;
    DWORD Mode;

    if (GetConsoleMode(OutputHandle, &Mode)) {
        if (SupportsColor != NULL) {
            *SupportsColor = TRUE;
        }
        if (SupportsExtendedChars != NULL) {
            *SupportsExtendedChars = TRUE;
        }
        if (SupportsAutoLineWrap != NULL) {
            if (Mode & ENABLE_WRAP_AT_EOL_OUTPUT) {
                *SupportsAutoLineWrap = TRUE;
            } else {
                *SupportsAutoLineWrap = FALSE;
            }
        }

        return TRUE;
    }

    if (SupportsColor != NULL) {
        *SupportsColor = FALSE;
    }
    if (SupportsExtendedChars != NULL) {
        *SupportsExtendedChars = FALSE;
    }
    if (SupportsAutoLineWrap != NULL) {
        *SupportsAutoLineWrap = FALSE;
    }

    //
    //  Load any user specified support from the environment.
    //

    YoriLibInitEmptyString(&TermString);
    if (!YoriLibAllocateAndGetEnvironmentVariable(_T("YORITERM"), &TermString)) {
        return TRUE;
    }

    YoriLibInitEmptyString(&SubString);
    SubString.StartOfString = TermString.StartOfString;
    SubString.LengthInChars = TermString.LengthInChars;

    //
    //  Now go through the list and see what support is present.
    //

    while (TRUE) {

        NextSeperator = YoriLibFindLeftMostCharacter(&SubString, ';');
        if (NextSeperator != NULL) {
            SubString.LengthInChars = (DWORD)(NextSeperator - SubString.StartOfString);
        }

        if (YoriLibCompareStringWithLiteralInsensitive(&SubString, _T("color")) == 0) {
            if (SupportsColor != NULL) {
                *SupportsColor = TRUE;
            }
        }

        if (YoriLibCompareStringWithLiteralInsensitive(&SubString, _T("extendedchars")) == 0) {
            if (SupportsExtendedChars != NULL) {
                *SupportsExtendedChars = TRUE;
            }
        }

        if (YoriLibCompareStringWithLiteralInsensitive(&SubString, _T("autolinewrap")) == 0) {
            if (SupportsAutoLineWrap != NULL) {
                *SupportsAutoLineWrap = TRUE;
            }
        }

        if (NextSeperator == NULL) {
            break;
        }

        SubString.StartOfString = NextSeperator + 1;
        SubString.LengthInChars = TermString.LengthInChars - (DWORD)(SubString.StartOfString - TermString.StartOfString);
    }

    YoriLibFreeStringContents(&TermString);
    return TRUE;
}



// vim:sw=4:ts=4:et:
