/**
 * @file lib/rsrc.c
 *
 * Yori shell send files to the recycle bin.
 *
 * Copyright (c) 2024 Malcolm J. Smith
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
 * FITNESS ERASE A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ERASE ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>

/**
 Load a consecutive range of string resources into an in-memory array.  The
 string array is allocated within this routine, however the strings
 themselves are stored within the executable's resource section and are
 accessible via a direct memory mapping into that resource section.  So
 while the string array needs to be deallocated with @ref YoriLibDereference ,
 the elements have no allocation.

 @param InitialElement Specifies the first string resource ID to load.  This
        must be a multiple of 16, since string resources are stored as tables
        containing 16 elements each.

 @param NumberElements Specifies the number of string resources to load.

 @param StringArray On successful completion, updated to contain a pointer to
        an array of YORI_STRINGs, where the array is NumberElements in length.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibLoadStringResourceArray(
    __in YORI_ALLOC_SIZE_T InitialElement,
    __in YORI_ALLOC_SIZE_T NumberElements,
    __out PYORI_STRING * StringArray
    )
{
    YORI_ALLOC_SIZE_T Index;
    LPWSTR Ptr;
    PYORI_STRING LocalStringArray;
    PYORI_STRING String;
    YORI_ALLOC_SIZE_T StringIndex;
    YORI_ALLOC_SIZE_T StringTableId;
    YORI_ALLOC_SIZE_T StringTableCount;
    YORI_ALLOC_SIZE_T StringCount;
    HGLOBAL hRsrcMem;
    HRSRC hRsrc;

    //
    //  Strings are arranged as tables with 16 elements in each.  For
    //  simplicity, require strings to be aligned on table boundaries.
    //
    ASSERT((InitialElement % 16) == 0);
    if ((InitialElement % 16) != 0) {
        return FALSE;
    }

    LocalStringArray = YoriLibReferencedMalloc(sizeof(YORI_STRING) * NumberElements);
    if (LocalStringArray == NULL) {
        return FALSE;
    }

    for (Index = 0; Index < NumberElements; Index++) {
        YoriLibInitEmptyString(&LocalStringArray[Index]);
    }

    StringTableCount = (NumberElements + 15) / 16;
    StringTableId = InitialElement / 16 + 1;
    StringCount = NumberElements;

    for (Index = 0; Index < StringTableCount; Index++) {
        hRsrc = FindResource(NULL, MAKEINTRESOURCE(Index + StringTableId), RT_STRING);
        if (hRsrc != NULL) {
            hRsrcMem = LoadResource(NULL, hRsrc);
            if (hRsrcMem != NULL) {
                Ptr = LockResource(hRsrcMem);
                if (Ptr) {
                    for (StringIndex = 0; StringIndex < StringCount && StringIndex < 16; StringIndex++) {
                        String = &LocalStringArray[Index * 16 + StringIndex];
                        String->LengthInChars = *Ptr;
                        String->StartOfString = (Ptr + 1);

                        Ptr = Ptr + 1 + (*Ptr);
                    }
                }
            }
        }
        StringCount = StringCount - 16;
    }
    *StringArray = LocalStringArray;
    return TRUE;
}

/**
 Load a consecutive range of string resources into an in-memory array and
 validate that all strings have been successfully loaded.  The string array
 is allocated within this routine, however the strings themselves are stored
 within the executable's resource section and are accessible via a direct
 memory mapping into that resource section.  So while the string array needs
 to be deallocated with @ref YoriLibDereference , the elements have no
 allocation.

 @param InitialElement Specifies the first string resource ID to load.  This
        must be a multiple of 16, since string resources are stored as tables
        containing 16 elements each.

 @param NumberElements Specifies the number of string resources to load.

 @param StringArray On successful completion, updated to contain a pointer to
        an array of YORI_STRINGs, where the array is NumberElements in length.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibLoadAndVerifyStringResourceArray(
    __in YORI_ALLOC_SIZE_T InitialElement,
    __in YORI_ALLOC_SIZE_T NumberElements,
    __out PYORI_STRING * StringArray
    )
{
    PYORI_STRING LocalStringArray;
    YORI_ALLOC_SIZE_T Index;

    if (!YoriLibLoadStringResourceArray(InitialElement, NumberElements, &LocalStringArray)) {
        return FALSE;
    }

    for (Index = 0; Index < NumberElements; Index++) {
        if (LocalStringArray[Index].StartOfString == NULL) {
            YoriLibDereference(LocalStringArray);
            return FALSE;
        }
    }

    *StringArray = LocalStringArray;

    return TRUE;
}

// vim:sw=4:ts=4:et:
