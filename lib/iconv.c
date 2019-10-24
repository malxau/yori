/**
 * @file lib/iconv.c
 *
 * Yori text encoding routines
 *
 * Copyright (c) 2017 Malcolm J. Smith
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
 The current output encoding.  Only meaningful if
 YoriLibActiveOutputEncodingInitialized is TRUE.
 */
DWORD YoriLibActiveOutputEncoding;

/**
 The current input encoding.  Only meaningful if
 YoriLibActiveInputEncodingInitialized is TRUE.
 */
DWORD YoriLibActiveInputEncoding;

/**
 Set to TRUE once the default output encoding has been established.
 */
BOOL YoriLibActiveOutputEncodingInitialized;

/**
 Set to TRUE once the default input encoding has been established.
 */
BOOL YoriLibActiveInputEncodingInitialized;

/**
 Returns the active output encoding, establishing the default encoding if it
 has not yet been determined.  Currently the default is UTF8 except on old
 releases where this support is not available.
 */
DWORD
YoriLibGetMultibyteOutputEncoding()
{
    if (!YoriLibActiveOutputEncodingInitialized) {
        DWORD WinMajorVer;
        DWORD WinMinorVer;
        DWORD BuildNumber;
    
        YoriLibGetOsVersion(&WinMajorVer, &WinMinorVer, &BuildNumber);
    
        if (WinMajorVer < 4) {
            YoriLibActiveOutputEncoding = CP_OEMCP;
        } else {
            YoriLibActiveOutputEncoding = CP_UTF8;
        }
        YoriLibActiveOutputEncodingInitialized = TRUE;
    }
    return YoriLibActiveOutputEncoding;
}

/**
 Returns the active input encoding, establishing the default encoding if it
 has not yet been determined.  Currently the default is UTF8 except on old
 releases where this support is not available.
 */
DWORD
YoriLibGetMultibyteInputEncoding()
{
    if (!YoriLibActiveInputEncodingInitialized) {
        DWORD WinMajorVer;
        DWORD WinMinorVer;
        DWORD BuildNumber;
    
        YoriLibGetOsVersion(&WinMajorVer, &WinMinorVer, &BuildNumber);
    
        if (WinMajorVer < 4) {
            YoriLibActiveInputEncoding = CP_OEMCP;
        } else {
            YoriLibActiveInputEncoding = CP_UTF8;
        }
        YoriLibActiveInputEncodingInitialized = TRUE;
    }
    return YoriLibActiveInputEncoding;
}

/**
 Set the output encoding to a specific value.

 @param Encoding The new encoding to use.
 */
VOID
YoriLibSetMultibyteOutputEncoding(
    __in DWORD Encoding
    )
{
    YoriLibActiveOutputEncoding = Encoding;
    YoriLibActiveOutputEncodingInitialized = TRUE;
}

/**
 Set the input encoding to a specific value.

 @param Encoding The new encoding to use.
 */
VOID
YoriLibSetMultibyteInputEncoding(
    __in DWORD Encoding
    )
{
    YoriLibActiveInputEncoding = Encoding;
    YoriLibActiveInputEncodingInitialized = TRUE;
}

/**
 Returns the number of bytes needed to store a specified UTF16 string in
 the current output encoding.

 @param StringBuffer The UTF16 string.

 @param BufferLength The length of the string, in characters.

 @return The number of bytes needed to store the output form.
 */
DWORD
YoriLibGetMultibyteOutputSizeNeeded(
    __in LPCTSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    DWORD Return;
    DWORD Encoding = YoriLibGetMultibyteOutputEncoding();
    if (Encoding == CP_UTF16) {
        return BufferLength * sizeof(WCHAR);
    }
    Return = WideCharToMultiByte(Encoding, 0, StringBuffer, BufferLength, NULL, 0, NULL, NULL);
    ASSERT(Return > 0 || BufferLength == 0);
    return Return;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(disable: 6054) // Buffer might not be NULL terminated.
                               // This occurs when UTF16 invokes memcpy
                               // and trusts the caller to provide sane
                               // input
#endif

/**
 Convert a UTF16 string into the output encoding.

 @param InputStringBuffer Pointer to a UTF16 string.

 @param InputBufferLength The size of InputStringBuffer, in characters.

 @param OutputStringBuffer Pointer to a buffer to be populated with the string
        in the current output encoding.

 @param OutputBufferLength The length of the output buffer, in bytes.
 */
VOID
YoriLibMultibyteOutput(
    __in_ecount(InputBufferLength) LPCTSTR InputStringBuffer,
    __in DWORD InputBufferLength,
    __out_ecount(OutputBufferLength) LPSTR OutputStringBuffer,
    __in DWORD OutputBufferLength
    )
{
    DWORD Return;
    DWORD Encoding = YoriLibGetMultibyteOutputEncoding();
    if (Encoding == CP_UTF16) {
        ASSERT(OutputBufferLength >= InputBufferLength * sizeof(WCHAR));
        if (OutputBufferLength >= InputBufferLength * sizeof(WCHAR)) {
            memcpy(OutputStringBuffer, InputStringBuffer, InputBufferLength * sizeof(WCHAR));
        }
        return;
    }
    Return = WideCharToMultiByte(Encoding,
                                 0,
                                 InputStringBuffer,
                                 InputBufferLength,
                                 OutputStringBuffer,
                                 OutputBufferLength,
                                 NULL,
                                 NULL);

    if (Return == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("InputBufferLength %i OutputBufferLength %i\n"), InputBufferLength, OutputBufferLength);
        ASSERT(Return != 0);
    }
}

/**
 Returns the number of bytes needed to store a string in the current input
 encoding into UTF16.

 @param StringBuffer The string in the input encoding.

 @param BufferLength The length of the string, in bytes.

 @return The number of bytes needed to store the UTF16 form.
 */
DWORD
YoriLibGetMultibyteInputSizeNeeded(
    __in LPCSTR StringBuffer,
    __in DWORD BufferLength
    )
{
    DWORD Encoding = YoriLibGetMultibyteInputEncoding();
    if (Encoding == CP_UTF16) {
        return BufferLength;
    }
    return MultiByteToWideChar(Encoding, 0, StringBuffer, BufferLength, NULL, 0);
}

/**
 Convert a string from the input encoding into UTF16.

 @param InputStringBuffer Pointer to a string in input encoding form.

 @param InputBufferLength The size of InputStringBuffer, in bytes.

 @param OutputStringBuffer Pointer to a buffer to be populated with the string
        in UTF16 format.

 @param OutputBufferLength The length of the output buffer, in characters.
 */
VOID
YoriLibMultibyteInput(
    __in_ecount(InputBufferLength) LPCSTR InputStringBuffer,
    __in DWORD InputBufferLength,
    __out_ecount(OutputBufferLength) LPTSTR OutputStringBuffer,
    __in DWORD OutputBufferLength
    )
{
    DWORD Return;
    DWORD Encoding = YoriLibGetMultibyteInputEncoding();
    if (Encoding == CP_UTF16) {
        ASSERT(OutputBufferLength >= InputBufferLength);
        if (OutputBufferLength >= InputBufferLength) {
            memcpy(OutputStringBuffer, InputStringBuffer, InputBufferLength * sizeof(WCHAR));
        }
        return;
    }
    Return = MultiByteToWideChar(Encoding,
                                 0,
                                 InputStringBuffer,
                                 InputBufferLength,
                                 OutputStringBuffer,
                                 OutputBufferLength);

    ASSERT(Return != 0);
}

// vim:sw=4:ts=4:et:
