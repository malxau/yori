/**
 * @file lib/ylstralc.c
 *
 * Yori string allocation routines
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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
 Initialize a Yori string with no contents.

 @param String Pointer to the string to initialize.
 */
VOID
YoriLibInitEmptyString(
    __out PYORI_STRING String
   )
{
    String->MemoryToFree = NULL;
    String->StartOfString = NULL;
    String->LengthAllocated = 0;
    String->LengthInChars = 0;
}

/**
 Free any memory being used by a Yori string.  This frees the internal
 string buffer, but the structure itself is caller allocated.

 @param String Pointer to the string to free.
 */
VOID
YoriLibFreeStringContents(
    __inout PYORI_STRING String
    )
{
    if (String->MemoryToFree != NULL) {
        YoriLibDereference(String->MemoryToFree);
    }

    YoriLibInitEmptyString(String);
}

/**
 Allocates memory in a Yori string to hold a specified number of characters.
 This routine will not free any previous allocation or copy any previous
 string contents.

 @param String Pointer to the string to allocate.

 @param CharsToAllocate The number of characters to allocate in the string.

 @return TRUE to indicate the allocate was successful, FALSE if it was not.
 */
BOOL
YoriLibAllocateString(
    __out PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T CharsToAllocate
    )
{
    YoriLibInitEmptyString(String);
    if (CharsToAllocate > YORI_MAX_ALLOC_SIZE / sizeof(TCHAR)) {
        return FALSE;
    }
    String->MemoryToFree = YoriLibReferencedMalloc(CharsToAllocate * sizeof(TCHAR));
    if (String->MemoryToFree == NULL) {
        return FALSE;
    }
    String->LengthAllocated = CharsToAllocate;
    String->StartOfString = String->MemoryToFree;
    return TRUE;
}

/**
 Reallocates memory in a Yori string to hold a specified number of characters,
 preserving any previous contents and deallocating any previous buffer.

 @param String Pointer to the string to reallocate.

 @param CharsToAllocate The number of characters to allocate in the string.

 @return TRUE to indicate the allocate was successful, FALSE if it was not.
 */
BOOL
YoriLibReallocString(
    __inout PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T CharsToAllocate
    )
{
    LPTSTR NewMemoryToFree;

    if (CharsToAllocate <= String->LengthInChars) {
        return FALSE;
    }

    NewMemoryToFree = YoriLibReferencedMalloc(CharsToAllocate * sizeof(TCHAR));
    if (NewMemoryToFree == NULL) {
        return FALSE;
    }

    if (String->LengthInChars > 0) {
        memcpy(NewMemoryToFree, String->StartOfString, String->LengthInChars * sizeof(TCHAR));
    }

    if (String->MemoryToFree) {
        YoriLibDereference(String->MemoryToFree);
    }

    String->MemoryToFree = NewMemoryToFree;
    String->LengthAllocated = CharsToAllocate;
    String->StartOfString = String->MemoryToFree;
    return TRUE;
}

/**
 Reallocates memory in a Yori string to hold a specified number of characters,
 without preserving contents, and deallocate the previous buffer on success.

 @param String Pointer to the string to reallocate.

 @param CharsToAllocate The number of characters to allocate in the string.

 @return TRUE to indicate the allocate was successful, FALSE if it was not.
 */
BOOL
YoriLibReallocStringNoContents(
    __inout PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T CharsToAllocate
    )
{
    LPTSTR NewMemoryToFree;

    if (CharsToAllocate <= String->LengthInChars) {
        return FALSE;
    }

    NewMemoryToFree = YoriLibReferencedMalloc(CharsToAllocate * sizeof(TCHAR));
    if (NewMemoryToFree == NULL) {
        return FALSE;
    }

    if (String->MemoryToFree) {
        YoriLibDereference(String->MemoryToFree);
    }

    String->MemoryToFree = NewMemoryToFree;
    String->LengthAllocated = CharsToAllocate;
    String->StartOfString = String->MemoryToFree;
    String->LengthInChars = 0;
    return TRUE;
}

/**
 Allocate a new buffer to hold a NULL terminated form of the contents of a
 Yori string.  The caller should free this buffer when it is no longer needed
 by calling @ref YoriLibDereference .

 @param String Pointer to the Yori string to convert into a NULL terminated
        string.

 @return Pointer to a NULL terminated string, or NULL on failure.
 */
LPTSTR
YoriLibCStringFromYoriString(
    __in PYORI_STRING String
    )
{
    LPTSTR Return;

    Return = YoriLibReferencedMalloc((String->LengthInChars + 1) * sizeof(TCHAR));
    if (Return == NULL) {
        return NULL;
    }

    memcpy(Return, String->StartOfString, String->LengthInChars * sizeof(TCHAR));
    Return[String->LengthInChars] = '\0';
    return Return;
}

/**
 Create a Yori string that points to a previously existing NULL terminated
 string constant.  The lifetime of the buffer containing the string is
 managed by the caller.

 @param String Pointer to the Yori string to initialize with a preexisting
        NULL terminated string.

 @param Value Pointer to the NULL terminated string to initialize String with.
 */
VOID
YoriLibConstantString(
    __out _Post_satisfies_(String->StartOfString != NULL) PYORI_STRING String,
    __in LPCTSTR Value
    )
{
    String->MemoryToFree = NULL;
    String->StartOfString = (LPTSTR)Value;
    String->LengthInChars = (YORI_ALLOC_SIZE_T)_tcslen(Value);
    String->LengthAllocated = String->LengthInChars + 1;
}

/**
 Copy the contents of one Yori string to another by referencing any existing
 allocation.

 @param Dest The Yori string to be populated with new contents.  This string
        will be reinitialized, and this function makes no attempt to free or
        preserve previous contents.

 @param Src The string that contains contents to propagate into the new
        string.
 */
VOID
YoriLibCloneString(
    __out PYORI_STRING Dest,
    __in PYORI_STRING Src
    )
{
    if (Src->MemoryToFree != NULL) {
        YoriLibReference(Src->MemoryToFree);
    }

    memcpy(Dest, Src, sizeof(YORI_STRING));
}

/**
 Copy the contents of one Yori string to another by deep copying the string
 contents.

 @param Dest The Yori string to be populated with new contents.  This string
        will be allocated, and this function makes no attempt to free or
        preserve previous contents.

 @param Src The string that contains contents to propagate into the new
        string.
 */
BOOLEAN
YoriLibCopyString(
    __out PYORI_STRING Dest,
    __in PCYORI_STRING Src
    )
{
    if (!YoriLibAllocateString(Dest, Src->LengthInChars + 1)) {
        return FALSE;
    }

    memcpy(Dest->StartOfString, Src->StartOfString, Src->LengthInChars * sizeof(TCHAR));
    Dest->LengthInChars = Src->LengthInChars;
    Dest->StartOfString[Dest->LengthInChars] = '\0';
    return TRUE;
}

/**
 Return TRUE if the Yori string is NULL terminated.  If it is not NULL
 terminated, return FALSE.

 @param String Pointer to the string to check.

 @return TRUE if the string is NULL terminated, FALSE if it is not.
 */
BOOL
YoriLibIsStringNullTerminated(
    __in PCYORI_STRING String
    )
{
    //
    //  Check that the string is of a sane size.  This is really to check
    //  whether the string has been initialized and populated correctly.
    //

    ASSERT(String->LengthAllocated <= 0x1000000);

    if (String->LengthAllocated > String->LengthInChars &&
        String->StartOfString[String->LengthInChars] == '\0') {

        return TRUE;
    }
    return FALSE;
}

// vim:sw=4:ts=4:et:
