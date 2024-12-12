/**
 * @file lib/vt.c
 *
 * Support output to the debugger.
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
YoriLibDbgInitializeStream(
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
YoriLibDbgEndStream(
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
YoriLibDbgProcOutputText(
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
YoriLibDbgProcOutputEscape(
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
YoriLibDbgSetFn(
    __out PYORI_LIB_VT_CALLBACK_FUNCTIONS CallbackFunctions
    )
{
    CallbackFunctions->InitializeStream = YoriLibDbgInitializeStream;
    CallbackFunctions->EndStream = YoriLibDbgEndStream;
    CallbackFunctions->ProcessAndOutputText = YoriLibDbgProcOutputText;
    CallbackFunctions->ProcessAndOutputEscape = YoriLibDbgProcOutputEscape;
    CallbackFunctions->Context = 0;
    return TRUE;
}

// vim:sw=4:ts=4:et:
