/**
 * @file lib/ylstrsrt.c
 *
 * Yori string sorting routines
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
 Swap the contents of two strings.  This leaves the string data in its
 current location and is only updating the pointers and lengths within the
 string structures.

 @param Str1 Pointer to the first string.

 @param Str2 Pointer to the second string.
 */
VOID
YoriLibSwapStrings(
    __inout PYORI_STRING Str1,
    __inout PYORI_STRING Str2
    )
{
    YORI_STRING SwapString;

    memcpy(&SwapString, Str2, sizeof(YORI_STRING));
    memcpy(Str2, Str1, sizeof(YORI_STRING));
    memcpy(Str1, &SwapString, sizeof(YORI_STRING));
}

/**
 Sort an array of strings.  This is currently implemented using a handrolled
 quicksort.

 @param StringArray Pointer to an array of strings.

 @param Count The number of elements in the array.
 */
VOID
YoriLibSortStringArray(
    __in_ecount(Count) PYORI_STRING StringArray,
    __in YORI_ALLOC_SIZE_T Count
    )
{
    YORI_ALLOC_SIZE_T FirstOffset;
    YORI_ALLOC_SIZE_T Index;

    if (Count <= 1) {
        return;
    }

    FirstOffset = 0;

    while (TRUE) {
        YORI_ALLOC_SIZE_T BreakPoint;
        YORI_ALLOC_SIZE_T LastOffset;
        YORI_STRING MidPoint;
        volatile DWORD LastSwapFirst;
        volatile DWORD LastSwapLast;

        BreakPoint = Count / 2;
        memcpy(&MidPoint, &StringArray[BreakPoint], sizeof(YORI_STRING));
    
        FirstOffset = 0;
        LastOffset = Count - 1;
    
        //
        //  Scan from the beginning looking for an item that should be sorted
        //  after the midpoint.
        //
    
        for (FirstOffset = 0; FirstOffset < LastOffset; FirstOffset++) {
            if (YoriLibCompareStringIns(&StringArray[FirstOffset],&MidPoint) >= 0) {
                for (; LastOffset > FirstOffset; LastOffset--) {
                    if (YoriLibCompareStringIns(&StringArray[LastOffset],&MidPoint) < 0) {
                        LastSwapFirst = FirstOffset;
                        LastSwapLast = LastOffset;
                        YoriLibSwapStrings(&StringArray[FirstOffset], &StringArray[LastOffset]);
                        LastOffset--;
                        break;
                    }
                }
                if (LastOffset <= FirstOffset) {
                    break;
                }
            }
        }

        ASSERT(FirstOffset == LastOffset);
        FirstOffset = LastOffset;
        if (YoriLibCompareStringIns(&StringArray[FirstOffset], &MidPoint) < 0) {
            FirstOffset++;
        }

        //
        //  If no copies occurred, check if everything in the list is
        //  sorted.
        //

        if (FirstOffset == 0 || FirstOffset == Count) {
            for (Index = 0; Index < Count - 1; Index++) {
                if (YoriLibCompareStringIns(&StringArray[Index], &StringArray[Index + 1]) > 0) {
                    break;
                }
            }
            if (Index == Count - 1) {
                return;
            }

            //
            //  Nothing was copied and it's not because it's identical.  This
            //  implies a very bad choice of midpoint that belongs either at
            //  the beginning or the end (so nothing could move past it.)
            //  Move it around and repeat the process (which will get a new
            //  midpoint.)
            //
    
            if (FirstOffset == 0) {
                ASSERT(YoriLibCompareStringIns(&StringArray[0], &MidPoint) > 0);

                if (YoriLibCompareStringIns(&StringArray[0], &MidPoint) > 0) {
                    YoriLibSwapStrings(&StringArray[0], &StringArray[BreakPoint]);
                }
            } else {
                ASSERT(YoriLibCompareStringIns(&MidPoint, &StringArray[Count - 1]) > 0);
                YoriLibSwapStrings(&StringArray[Count - 1], &StringArray[BreakPoint]);
            }
        } else {
            break;
        }
    }

    //
    //  If there are two sets (FirstOffset is not at either end), recurse.
    //

    if (FirstOffset && (Count - FirstOffset)) {
        if (FirstOffset > 0) {
            YoriLibSortStringArray(StringArray, FirstOffset);
        }
        if ((Count - FirstOffset) > 0) {
            YoriLibSortStringArray(&StringArray[FirstOffset], Count - FirstOffset);
        }
    }

    //
    //  Check that it's now sorted
    //
    for (Index = 0; Index < Count - 1; Index++) {
        if (YoriLibCompareStringIns(&StringArray[Index], &StringArray[Index + 1]) > 0) {
            break;
        }
    }
    ASSERT (Index == Count - 1);
}

// vim:sw=4:ts=4:et:
