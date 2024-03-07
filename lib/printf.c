/**
 * @file printf.c
 *
 * Unicode versions of printf functions.
 *
 * Copyright (c) 2014 Malcolm J. Smith
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

/**
 Indicate that the printf routine should generate YoriLibVSPrintf as a 
 Unicode native form.
 */
#define UNICODE 1

/**
 Indicate that the printf routine should generate YoriLibVSPrintf as a 
 Unicode native form.
 */
#define _UNICODE 1

#include "yoripch.h"
#include "yorilib.h"

#ifdef __WATCOMC__
#pragma disable_message(201)
#pragma disable_message(302)
#endif

/**
 Indicate to the printf code that whether we're compiling for unicode or not,
 the system supports unicode, so conversions should be included.
 */
#define PRINTF_UNICODE_SUPPORTED 1

#include "printf.inc"

/**
 Indicate that the printf routine should generate YoriLibVSPrintfSize.
 */
#define PRINTF_SIZEONLY 1
#include "printf.inc"

/**
 Process a printf format string and output the result into a NULL terminated
 buffer of specified size.

 @param szDest The buffer to populate with the result.

 @param len The number of characters in the buffer.

 @param szFmt The format string to process.

 @return The number of characters successfully populated into the buffer, or
         -1 on error.
 */
YORI_SIGNED_ALLOC_SIZE_T
YoriLibSPrintfS(
    __out_ecount(len) LPTSTR szDest,
    __in YORI_ALLOC_SIZE_T len,
    __in LPCTSTR szFmt,
    ...
    )
{
    va_list marker;
    YORI_SIGNED_ALLOC_SIZE_T out_len;

    va_start( marker, szFmt );
    out_len = YoriLibVSPrintf(szDest, len, szFmt, marker);
    va_end( marker );
    return out_len;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(push)
#pragma warning(disable: 6386) // Obviously this is the buffer unsafe version
#endif

/**
 Process a printf format string and output the result into a NULL terminated
 buffer which is assumed to be large enough to hold the result.

 @param szDest The buffer to populate with the result.

 @param szFmt The format string to process.

 @return The number of characters successfully populated into the buffer, or
         -1 on error.
 */
YORI_SIGNED_ALLOC_SIZE_T
YoriLibSPrintf(
    __out LPTSTR szDest,
    __in LPCTSTR szFmt,
    ...
    )
{
    va_list marker;
    YORI_SIGNED_ALLOC_SIZE_T out_len;

    va_start( marker, szFmt );
    out_len = YoriLibVSPrintf(szDest, YORI_MAX_ALLOC_SIZE, szFmt, marker);
    va_end( marker );
    return out_len;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(pop)
#endif

/**
 Process a printf format string and output the result into a Yori string.
 If the string is not large enough to contain the result, it is reallocated
 internally.

 @param Dest The string to populate with the result.

 @param szFmt The format string to process.

 @param marker The arguments that are specific to the format string.

 @return The number of characters successfully populated into the buffer, or
         -1 on error.
 */
__success(return >= 0)
YORI_SIGNED_ALLOC_SIZE_T
YoriLibYPrintfInternal(
    __inout PYORI_STRING Dest,
    __in LPCTSTR szFmt,
    __in va_list marker
    )
{
    YORI_SIGNED_ALLOC_SIZE_T required_len;
    YORI_SIGNED_ALLOC_SIZE_T out_len;

    required_len = YoriLibVSPrintfSize(szFmt, marker);
    if (required_len < 0) {
        return required_len;
    }
    if ((YORI_ALLOC_SIZE_T)required_len > Dest->LengthAllocated) {
        YoriLibFreeStringContents(Dest);

        Dest->MemoryToFree = YoriLibReferencedMalloc(required_len * sizeof(TCHAR));
        if (Dest->MemoryToFree == NULL) {
            return -1;
        }

        Dest->StartOfString = Dest->MemoryToFree;
        Dest->LengthAllocated = required_len;
    }

    out_len = YoriLibVSPrintf(Dest->StartOfString, Dest->LengthAllocated, szFmt, marker);
    if (out_len >= 0) {
        Dest->LengthInChars = out_len;
    }
    return out_len;
}

/**
 Process a printf format string and output the result into a Yori string.
 If the string is not large enough to contain the result, it is reallocated
 internally.

 @param Dest The string to populate with the result.

 @param szFmt The format string to process.

 @return The number of characters successfully populated into the buffer, or
         -1 on error.
 */
__success(return >= 0)
YORI_SIGNED_ALLOC_SIZE_T
YoriLibYPrintf(
    __inout PYORI_STRING Dest,
    __in LPCTSTR szFmt,
    ...
    )
{
    va_list marker;
    YORI_SIGNED_ALLOC_SIZE_T out_len;

    va_start( marker, szFmt );
    out_len = YoriLibYPrintfInternal(Dest, szFmt, marker);
    va_end( marker );
    return out_len;
}

/**
 Process a printf format string and count the number of characters required
 to contain the result, including the NULL terminator character.

 @param szFmt The format string to process.

 @return The number of characters that could be populated into the buffer, or
         -1 on error.
 */
YORI_SIGNED_ALLOC_SIZE_T
YoriLibSPrintfSize(
    __in LPCTSTR szFmt,
    ...
    )
{
    va_list marker;
    YORI_SIGNED_ALLOC_SIZE_T out_len;

    va_start( marker, szFmt );
    out_len = YoriLibVSPrintfSize(szFmt, marker);
    va_end( marker );
    return out_len;
}

// vim:sw=4:ts=4:et:
