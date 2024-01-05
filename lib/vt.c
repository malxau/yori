/**
 * @file lib/vt.c
 *
 * Convert VT100/ANSI escape sequences into other formats, including the 
 * console.
 *
 * Copyright (c) 2015-2021 Malcolm J. Smith
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
 Pseudohandle to refer to the debugger, since this cannot be opened as a
 regular file.  Note the same value can be used for the current process or
 thread, but the collision here does not matter since this handle will never
 be sent to the kernel as a handle, and it would be invalid to write to a
 process or thread.
 */
#define YORI_LIB_DEBUGGER_HANDLE (HANDLE)((DWORD_PTR)-1)

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
YoriLibVtGetDefaultColor(VOID)
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
YoriLibVtGetLineEnding(VOID)
{
    return YoriLibVtLineEnding;
}

/**
 Convert any incoming string to the active output encoding, and send it to
 the output device.

 @param hOutput Handle to the device to receive any output.

 @param String Pointer to the string to output which is in host (UTF16)
        encoding.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibOutputTextToMultibyteDevice(
    __in HANDLE hOutput,
    __in PCYORI_STRING String
    )
{
    DWORD BytesTransferred;
    BOOL Result;

#ifdef UNICODE
    {
        CHAR AnsiStackBuf[64 + 1];
        YORI_ALLOC_SIZE_T AnsiBytesNeeded;
        LPSTR AnsiBuf;

        AnsiBytesNeeded = (YORI_ALLOC_SIZE_T)YoriLibGetMultibyteOutputSizeNeeded(String->StartOfString, String->LengthInChars);

        if (AnsiBytesNeeded > (int)sizeof(AnsiStackBuf)) {
            AnsiBuf = YoriLibMalloc(AnsiBytesNeeded);
        } else {
            AnsiBuf = AnsiStackBuf;
        }

        if (AnsiBuf != NULL) {
            YoriLibMultibyteOutput(String->StartOfString,
                                   String->LengthInChars,
                                   AnsiBuf,
                                   AnsiBytesNeeded);

            Result = WriteFile(hOutput, AnsiBuf, AnsiBytesNeeded, &BytesTransferred, NULL);

            if (AnsiBuf != AnsiStackBuf) {
                YoriLibFree(AnsiBuf);
            }
        } else {
            Result = FALSE;
        }
    }
#else
    Result = WriteFile(hOutput,
                       String->StartOfString,
                       String->LengthInChars*sizeof(TCHAR),
                       &BytesTransferred,
                       NULL);
#endif
    return Result;
}

/**
 Convert any incoming string to contain specified line endings, and pass the
 result for conversion into the active output encoding.

 @param hOutput Handle to the device to receive any output.

 @param String Pointer to the string to output, which can have any line
        ending and is in host (UTF16) encoding.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibOutputTextToMultibyteNormalizeLineEnding(
    __in HANDLE hOutput,
    __in PCYORI_STRING String
    )
{
    YORI_STRING SearchString;
    YORI_STRING DisplayString;
    YORI_ALLOC_SIZE_T NonLineEndLength;
    YORI_ALLOC_SIZE_T CharsToIgnore;
    BOOL GenerateLineEnd;

    YoriLibInitEmptyString(&DisplayString);
    YoriLibInitEmptyString(&SearchString);
    SearchString.StartOfString = String->StartOfString;
    SearchString.LengthInChars = String->LengthInChars;

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

        if (NonLineEndLength > 0) {
            DisplayString.StartOfString = SearchString.StartOfString;
            DisplayString.LengthInChars = NonLineEndLength;

            if (!YoriLibOutputTextToMultibyteDevice(hOutput, &DisplayString)) {
                return FALSE;
            }
        }

        if (GenerateLineEnd) {
            YoriLibConstantString(&DisplayString, YoriLibVtLineEnding);
            if (!YoriLibOutputTextToMultibyteDevice(hOutput, &DisplayString)) {
                return FALSE;
            }
        }

        if (NonLineEndLength + CharsToIgnore >= SearchString.LengthInChars) {
            break;
        }
        SearchString.StartOfString = &SearchString.StartOfString[NonLineEndLength + CharsToIgnore];
        SearchString.LengthInChars = SearchString.LengthInChars - (NonLineEndLength + CharsToIgnore);
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

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibConsoleInitializeStream(
    __in HANDLE hOutput,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(Context);

    return TRUE;
}

/**
 End processing for the specified stream.  For console output, this is
 pointless.

 @param hOutput The output stream to end.

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibConsoleEndStream(
    __in HANDLE hOutput,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(Context);

    return TRUE;
}

/**
 Output text between escapes to the output Console.

 @param hOutput Handle to the output console.

 @param String Pointer to the string to output.

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibConsoleProcessAndOutputText(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    DWORD BytesTransferred;
    UNREFERENCED_PARAMETER(Context);

    WriteConsole(hOutput,
                 String->StartOfString,
                 String->LengthInChars,
                 &BytesTransferred,
                 NULL);
    return TRUE;
}

/**
 A callback function to receive an escape and ignore it.
 
 @param hOutput Handle to the output console.

 @param String Pointer to a buffer describing the escape.

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE for failure.  Note this particular variant
         can't exactly fail.
 */
BOOL
YoriLibConsoleProcessAndIgnoreEscape(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(Context);

    return TRUE;
}

/**
 Specifies the value to indicate a new color derives its foreground from the
 existing color.
 */
#define INITIAL_COMPONENT_FOREGROUND 0x0001

/**
 Specifies the value to indicate a new color derives its background from the
 existing color.
 */
#define INITIAL_COMPONENT_BACKGROUND 0x0002

/**
 Specifies the value to indicate a new color derives its underline from the
 existing color.
 */
#define INITIAL_COMPONENT_UNDERLINE  0x0004

/**
 Given a starting color and a VT sequence which may change it, generate the
 final color.  Both colors are in Win32 attribute form.

 @param InitialColor The starting color, in Win32 form.

 @param EscapeSequence The VT100 sequence to apply to the starting color.

 @param FinalColor On successful completion, updated to contain the final
        color.

 @param InitialComponentsUsed On successful completion, updated to indicate
        which components of the initial color are propagated into the new
        color.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
BOOL
YoriLibVtFinalColorFromSequenceEx(
    __in WORD InitialColor,
    __in PCYORI_STRING EscapeSequence,
    __out PWORD FinalColor,
    __out PWORD InitialComponentsUsed
    )
{
    LPTSTR CurrentPoint = EscapeSequence->StartOfString;
    YORI_ALLOC_SIZE_T CurrentOffset = 0;
    YORI_ALLOC_SIZE_T RemainingLength;
    WORD  ComponentsUsed = INITIAL_COMPONENT_FOREGROUND | INITIAL_COMPONENT_BACKGROUND | INITIAL_COMPONENT_UNDERLINE;
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

        DWORD Code;
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
            Code = YoriLibDecimalStringToInt(&SearchString);
            if (Code == 0) {
                ComponentsUsed = 0;
                NewColor = YoriLibVtResetColor;
                NewUnderline = FALSE;
            } else if (Code == 1) {
                NewColor |= FOREGROUND_INTENSITY;
            } else if (Code == 4) {
                ComponentsUsed = (WORD)(ComponentsUsed & ~(INITIAL_COMPONENT_UNDERLINE));
                NewUnderline = TRUE;
            } else if (Code == 7) {
                NewColor = (WORD)(((NewColor & 0xf) << 4) | ((NewColor & 0xf0) >> 4));
            } else if (Code == 39) {
                ComponentsUsed = (WORD)(ComponentsUsed & ~(INITIAL_COMPONENT_FOREGROUND));
                NewColor = (WORD)((NewColor & ~(0xf)) | (YoriLibVtResetColor & 0xf));
            } else if (Code == 49) {
                ComponentsUsed = (WORD)(ComponentsUsed & ~(INITIAL_COMPONENT_BACKGROUND));
                NewColor = (WORD)((NewColor & ~(0xf0)) | (YoriLibVtResetColor & 0xf0));
            } else if (Code >= 30 && Code <= 37) {
                ComponentsUsed = (WORD)(ComponentsUsed & ~(INITIAL_COMPONENT_FOREGROUND));
                NewColor = (WORD)((NewColor & ~(0xf)) | ColorTable[Code - 30]);
            } else if (Code >= 40 && Code <= 47) {
                ComponentsUsed = (WORD)(ComponentsUsed & ~(INITIAL_COMPONENT_BACKGROUND));
                NewColor = (WORD)((NewColor & ~(0xf0)) | (ColorTable[Code - 40]<<4));
            } else if (Code >= 90 && Code <= 97) {
                ComponentsUsed = (WORD)(ComponentsUsed & ~(INITIAL_COMPONENT_FOREGROUND));
                NewColor = (WORD)((NewColor & ~(0xf)) | FOREGROUND_INTENSITY | ColorTable[Code - 90]);
            } else if (Code >= 100 && Code <= 107) {
                ComponentsUsed = (WORD)(ComponentsUsed & ~(INITIAL_COMPONENT_BACKGROUND));
                NewColor = (WORD)((NewColor & ~(0xf0)) | BACKGROUND_INTENSITY | (ColorTable[Code - 100]<<4));
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
            RemainingLength = RemainingLength - CurrentOffset;

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
    *InitialComponentsUsed = ComponentsUsed;

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
    __in PCYORI_STRING EscapeSequence,
    __out PWORD FinalColor
    )
{
    WORD InitialComponentsUsed;

    return YoriLibVtFinalColorFromSequenceEx(InitialColor, EscapeSequence, FinalColor, &InitialComponentsUsed);
}

/**
 A callback function to receive an escape and translate it into the
 appropriate Win32 console action.
 
 @param hOutput Handle to the output console.

 @param String Pointer to a buffer describing the escape.

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibConsoleProcessAndOutputEscape(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    WORD NewColor;
    DWORD PreviousAttributes;
    BOOLEAN NeedExistingColor;

    PreviousAttributes = (DWORD)*Context;

    //
    //  Save the current color in the low 16 bits.  If a bit is set above
    //  the 16 bits, it indicates those bits are meaningful.  If not,
    //  the existing color requires a query from the console.
    //

    NewColor = LOWORD(PreviousAttributes);
    NeedExistingColor = TRUE;
    if (HIWORD(PreviousAttributes) != 0) {
        NeedExistingColor = FALSE;
    } else if (YoriLibVtResetColorSet) {
        WORD InitialComponentsUsed;

        //
        //  Check if the escape sequence can be resolved without querying
        //  from the console, because it is fully specified.  If so, apply
        //  the result without querying the previous color.
        //

        if (YoriLibVtFinalColorFromSequenceEx(0, String, &NewColor, &InitialComponentsUsed)) {
            if (InitialComponentsUsed == 0) {
                SetConsoleTextAttribute(hOutput, NewColor);
                *Context = ((1 << 16) | NewColor);
                return TRUE;
            }
        }
    }

    if (NeedExistingColor) {
        ConsoleInfo.wAttributes = DEFAULT_COLOR;
        GetConsoleScreenBufferInfo(hOutput, &ConsoleInfo);
        NewColor = ConsoleInfo.wAttributes;
    
        if (!YoriLibVtResetColorSet) {
            YoriLibVtResetColor = ConsoleInfo.wAttributes;
            YoriLibVtResetColorSet = TRUE;
        }

    }

    if (YoriLibVtFinalColorFromSequence(NewColor, String, &NewColor)) {
        SetConsoleTextAttribute(hOutput, NewColor);
        *Context = ((1 << 16) | NewColor);
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
    CallbackFunctions->Context = 0;
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
    CallbackFunctions->Context = 0;
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
    CallbackFunctions->Context = 0;
    return TRUE;
}

//
//  Text functions
//

/**
 Initialize the output stream with any header information.  For text
 output, this is pointless.

 @param hOutput The output stream to initialize.

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibUtf8TextInitializeStream(
    __in HANDLE hOutput,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(Context);

    return TRUE;
}

/**
 End processing for the specified stream.  For text output, this is
 pointless.

 @param hOutput Handle to the output device.

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibUtf8TextEndStream(
    __in HANDLE hOutput,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(Context);

    return TRUE;
}

/**
 Output text between escapes to the output device.

 @param hOutput Handle to the output device.

 @param String Pointer to the string to output.

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibUtf8TextProcessAndOutputText(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    UNREFERENCED_PARAMETER(Context);
    return YoriLibOutputTextToMultibyteNormalizeLineEnding(hOutput, String);
}

/**
 A dummy callback function to receive an escape and not do anything with it.
 
 @param hOutput Handle to the output device (ignored.)

 @param String Pointer to a buffer describing the escape (ignored.)

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibUtf8TextProcessAndOutputEscape(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(Context);

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
    CallbackFunctions->Context = 0;
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
    CallbackFunctions->Context = 0;
    return TRUE;
}

//
//  Debugger functions
//

/**
 Initialize the output stream with any header information.  For debugger
 output, this is pointless.

 @param hOutput The output stream to initialize.

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibDebuggerInitializeStream(
    __in HANDLE hOutput,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(Context);

    return TRUE;
}

/**
 End processing for the specified stream.  For debugger output, this is
 pointless.

 @param hOutput Handle to the output device, ignored for debugger.

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibDebuggerEndStream(
    __in HANDLE hOutput,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(Context);

    return TRUE;
}

/**
 Output text between escapes to the debugger.

 @param hOutput Handle to the output device, ignored for debugger.

 @param String Pointer to the string to output.

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE on failure.
 */
BOOL
YoriLibDebuggerProcessAndOutputText(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    TCHAR StackBuf[64 + 1];
    LPTSTR Buffer;
    DWORD ReadIndex;
    DWORD WriteIndex;

    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(Context);

    if (String->LengthInChars <= 64) {
        Buffer = StackBuf;
    } else {
        Buffer = YoriLibMalloc((String->LengthInChars + 1) * sizeof(TCHAR));
        if (Buffer == NULL) {
            return FALSE;
        }
    }

    WriteIndex = 0;
    for (ReadIndex = 0; ReadIndex < String->LengthInChars; ReadIndex++) {
        if (String->StartOfString[ReadIndex] == '\r') {
            if (ReadIndex + 1 < String->LengthInChars &&
                String->StartOfString[ReadIndex] == '\n') {

                ReadIndex++;
            } else {
                Buffer[WriteIndex++] = '\n';
            }
        } else {
            Buffer[WriteIndex++] = String->StartOfString[ReadIndex];
        }
    }

    Buffer[WriteIndex] = '\0';

    OutputDebugString(Buffer);
    if (Buffer != StackBuf) {
        YoriLibFree(Buffer);
    }

    return TRUE;
}

/**
 A dummy callback function to receive an escape and not do anything with it.
 
 @param hOutput Handle to the output device (ignored.)

 @param String Pointer to a buffer describing the escape (ignored.)

 @param Context Pointer to context (unused.)

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibDebuggerProcessAndOutputEscape(
    __in HANDLE hOutput,
    __in PCYORI_STRING String,
    __inout PYORI_MAX_UNSIGNED_T Context
    )
{
    UNREFERENCED_PARAMETER(hOutput);
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(Context);

    return TRUE;
}

/**
 Initialize callback functions to a set which will output text to the debugger
 and remove any escape sequences.

 @param CallbackFunctions The callback functions to initialize.

 @return TRUE for success, FALSE for failure.
 */
BOOL
YoriLibDebuggerSetFunctions(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    )
{
    CallbackFunctions->InitializeStream = YoriLibDebuggerInitializeStream;
    CallbackFunctions->EndStream = YoriLibDebuggerEndStream;
    CallbackFunctions->ProcessAndOutputText = YoriLibDebuggerProcessAndOutputText;
    CallbackFunctions->ProcessAndOutputEscape = YoriLibDebuggerProcessAndOutputEscape;
    CallbackFunctions->Context = 0;
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
    __in YORI_ALLOC_SIZE_T StringLength,
    __in HANDLE hOutput,
    __in PYORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks
    )
{
    LPTSTR CurrentPoint;
    YORI_ALLOC_SIZE_T CurrentOffset;
    YORI_ALLOC_SIZE_T PreviouslyConsumed;
    TCHAR VtEscape[] = {27, '\0'};
    YORI_STRING SearchString;
    YORI_STRING DisplayString;

    CurrentPoint = String;
    YoriLibInitEmptyString(&SearchString);
    YoriLibInitEmptyString(&DisplayString);
    SearchString.StartOfString = CurrentPoint;
    SearchString.LengthInChars = StringLength;
    CurrentOffset = YoriLibCountStringNotContainingChars(&SearchString, VtEscape);
    PreviouslyConsumed = 0;

    while (TRUE) {

        //
        //  If we have any text, perform required processing and output the text
        //

        if (CurrentOffset > 0) {
            DisplayString.StartOfString = CurrentPoint;
            DisplayString.LengthInChars = CurrentOffset;
            if (!Callbacks->ProcessAndOutputText(hOutput, &DisplayString, &Callbacks->Context)) {
                return FALSE;
            }

            CurrentPoint = CurrentPoint + CurrentOffset;
            PreviouslyConsumed = PreviouslyConsumed + CurrentOffset;
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

                YORI_ALLOC_SIZE_T EndOfEscape;
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

                DisplayString.StartOfString = CurrentPoint;
                DisplayString.LengthInChars = EndOfEscape + 3;

                if (!Callbacks->ProcessAndOutputEscape(hOutput, &DisplayString, &Callbacks->Context)) {
                    return FALSE;
                }
                CurrentPoint = CurrentPoint + EndOfEscape + 3;
                PreviouslyConsumed = PreviouslyConsumed + EndOfEscape + 3;
            } else {

                //
                //  Output just the escape character and move to the next
                //  match
                //

                DisplayString.StartOfString = CurrentPoint;
                DisplayString.LengthInChars = 1;
                if (!Callbacks->ProcessAndOutputText(hOutput, &DisplayString, &Callbacks->Context)) {
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
    __in YORI_ALLOC_SIZE_T StringLength,
    __in HANDLE hOutput,
    __in PYORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks
    )
{
    BOOL Result;

    Callbacks->InitializeStream(hOutput, &Callbacks->Context);

    Result = YoriLibProcessVtEscapesOnOpenStream(String, StringLength, hOutput, Callbacks);

    Callbacks->EndStream(hOutput, &Callbacks->Context);
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
#ifdef __WATCOMC__
    va_list savedmarker;
#else
    va_list savedmarker = marker;
#endif
    YORI_SIGNED_ALLOC_SIZE_T len;
    TCHAR stack_buf[64];
    TCHAR * buf;
    YORI_LIB_VT_CALLBACK_FUNCTIONS Callbacks;
    DWORD CurrentMode;
    BOOL Result;

#ifdef __WATCOMC__
    savedmarker[0] = marker[0];
#endif

    //
    //  Check if we're writing to a console supporting color or a file
    //  that doesn't
    //

    if (hOut == YORI_LIB_DEBUGGER_HANDLE) {
        YoriLibDebuggerSetFunctions(&Callbacks);
    } else if (GetConsoleMode(hOut, &CurrentMode)) {
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

    if (len>(YORI_SIGNED_ALLOC_SIZE_T)(sizeof(stack_buf)/sizeof(stack_buf[0]))) {
        buf = YoriLibMalloc(len * sizeof(TCHAR));
        if (buf == NULL) {
            return 0;
        }
    } else {
        buf = stack_buf;
    }

    marker = savedmarker;
    len = YoriLibVSPrintf(buf, len, szFmt, marker);

    __analysis_assume(hOut != 0);
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
    } else if ((Flags & YORI_LIB_OUTPUT_DEBUG) != 0) {
        hOut = (HANDLE)(YORI_LIB_DEBUGGER_HANDLE);
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
    YORI_ALLOC_SIZE_T CharIndex;
    YORI_ALLOC_SIZE_T DestIndex;
    YORI_ALLOC_SIZE_T EscapeChars;
    YORI_ALLOC_SIZE_T EndOfEscape;
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
    __out_opt PWORD Width,
    __out_opt PWORD Height
    )
{
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    YORI_MAX_SIGNED_T Temp;

    if (GetConsoleScreenBufferInfo(OutputHandle, &ConsoleInfo)) {
        if (Width != NULL) {
            *Width = (WORD)(ConsoleInfo.srWindow.Right - ConsoleInfo.srWindow.Left + 1);
        }
        if (Height != NULL) {
            *Height = (WORD)(ConsoleInfo.srWindow.Bottom - ConsoleInfo.srWindow.Top + 1);
        }

        return TRUE;
    }

    if (Width != NULL) {
        if (!YoriLibGetEnvironmentVariableAsNumber(_T("COLUMNS"), &Temp)) {
            *Width = 80;
        } else {
            *Width = (WORD)Temp;
        }
    }

    if (Height != NULL) {
        if (!YoriLibGetEnvironmentVariableAsNumber(_T("LINES"), &Temp)) {
            *Height = 25;
        } else {
            *Height = (WORD)Temp;
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
            if (!YoriLibIsNanoServer()) {
                *SupportsExtendedChars = TRUE;
            } else {
                *SupportsExtendedChars = FALSE;
            }
        }
        if (SupportsAutoLineWrap != NULL) {

            //
            //  Nano gives auto line wrap whether you want it or not.
            //

            if (YoriLibIsNanoServer()) {
                *SupportsAutoLineWrap = TRUE;
            } else if (Mode & ENABLE_WRAP_AT_EOL_OUTPUT) {
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
            SubString.LengthInChars = (YORI_ALLOC_SIZE_T)(NextSeperator - SubString.StartOfString);
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
        SubString.LengthInChars = TermString.LengthInChars - (YORI_ALLOC_SIZE_T)(SubString.StartOfString - TermString.StartOfString);
    }

    YoriLibFreeStringContents(&TermString);
    return TRUE;
}



// vim:sw=4:ts=4:et:
