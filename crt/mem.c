/**
 * @file crt/mem.c
 *
 * Implementations for mem* library functions.
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

#ifndef WIN32_LEAN_AND_MEAN
/**
 Standard define to include somewhat less of the Windows headers.  Command
 line software doesn't need much.
 */
#define WIN32_LEAN_AND_MEAN 1
#endif

#pragma warning(disable: 4001) /* Single line comment */
#pragma warning(disable: 4127) // conditional expression constant
#pragma warning(disable: 4201) // nameless struct/union
#pragma warning(disable: 4214) // bit field type other than int
#pragma warning(disable: 4514) // unreferenced inline function

#if defined(_MSC_VER) && (_MSC_VER >= 1910)
#pragma warning(disable: 5045) // spectre mitigation 
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(push)
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(disable: 4668) // preprocessor conditional with nonexistent macro, SDK bug
#pragma warning(disable: 4255) // no function prototype given.  8.1 and earlier SDKs exhibit this.
#pragma warning(disable: 4820) // implicit padding added in structure
#endif

/**
 Indicate support for compiling for ARM32 if an SDK is available.
 */
#define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1

#include <windows.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(pop)
#endif

/**
 Indicate to the standard CRT header that we're compiling the CRT itself.
 */
#define MINICRT_BUILD
#include "yoricrt.h"

/**
 Copy the contents of one memory block into another memory block where the
 two memory blocks must be disjoint so no consideration is made for writing
 to a destination block before reading from the same block.

 @param dest Pointer to the memory block to write to.

 @param src Pointer to the memory block to read from.

 @param len The number of bytes to copy.

 @return Pointer to the destination memory block, for some unknown reason.
 */
void *
MCRT_FN
mini_memcpy(void * dest, const void * src, unsigned int len)
{
    unsigned int i;
    char * char_src = (char *)src;
    char * char_dest = (char *)dest;
    for (i = 0; i < len; i++) {
        char_dest[i] = char_src[i];
    }
    return dest;
}

/**
 Copy the contents of one memory block into another memory block in cases
 where the two memory blocks may overlap so the values must be read before
 they are overwritten.

 @param dest Pointer to the memory block to write to.

 @param src Pointer to the memory block to read from.

 @param len The number of bytes to copy.

 @return Pointer to the destination memory block, for some unknown reason.
 */
void *
MCRT_FN
mini_memmove(void * dest, const void * src, unsigned int len)
{
    unsigned int i;
    char * char_src = (char *)src;
    char * char_dest = (char *)dest;
    if (char_dest > char_src) {
        if (len == 0) {
            return dest;
        }
        for (i = len - 1; ; i--) {
            char_dest[i] = char_src[i];
            if (i==0) break;
        }
    } else {
        for (i = 0; i < len; i++) {
            char_dest[i] = char_src[i];
        }
    }
    return dest;
}

//
//  Sigh, turn off optimizations now the optimizer is too aggressive to
//  be helpful, and MSVC doesn't have finer grained tools
//

#if defined(_MSC_VER) && (_MSC_VER >= 1940) && defined(_M_IX86)
#pragma optimize("", off)
#endif

/**
 Set a block of memory to a specific byte value.

 @param dest Pointer to the block of memory to initialize.

 @param c The character value to replicate across the memory block.

 @param len The length of the memory block, in bytes.

 @return A pointer to the memory block, for some unknown reason.
 */
void *
MCRT_FN
mini_memset(void * dest, char c, unsigned int len)
#ifdef __clang__
__attribute__((no_builtin("memset")))
#endif
{
    unsigned int i;
    unsigned int fill;
    unsigned int chunks = len / sizeof(fill);
    char * char_dest = (char *)dest;
    unsigned int * uint_dest = (unsigned int *)dest;

    //
    //  Note we go from the back to the front.  This is to 
    //  prevent newer compilers from noticing what we're doing
    //  and trying to invoke the built-in memset instead of us.
    //

    fill = (c<<24) + (c<<16) + (c<<8) + c;

    for (i = len; i > chunks * sizeof(fill); i--) {
        char_dest[i - 1] = c;
    }

    for (i = chunks; i > 0; i--) {
        uint_dest[i - 1] = fill;
    }

    return dest;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1940) && defined(_M_IX86)
#pragma optimize("", on)
#endif

/**
 Compare two blocks of memory and indicate if the first is less than the
 second, the second is less than the first, or if the two are equal.

 @param buf1 Pointer to the first memory block.

 @param buf2 Pointer to the second memory block.

 @param len The number of bytes to compare.

 @return -1 to indicate the first block is less than the second, 1 to
         indicate the first is greater than the second, and 0 to indicate
         the two are equal.
 */
int
MCRT_FN
mini_memcmp(const void * buf1, const void * buf2, unsigned int len)
{
    unsigned int i = 0;
    unsigned char * char_buf1 = (unsigned char *)buf1;
    unsigned char * char_buf2 = (unsigned char *)buf2;
    for (i = 0; i < len; i++) {
        if (char_buf1[i] < char_buf2[i]) {
            return -1;
        } else if (char_buf1[i] > char_buf2[i]) {
            return 1;
        }
    }
    return 0;
}

// vim:sw=4:ts=4:et:
