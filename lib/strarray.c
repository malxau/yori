/**
 * @file lib/strarray.c
 *
 * Yori string array
 *
 * Copyright (c) 2019-2023 Malcolm J. Smith
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
#include "yoriwin.h"
#include "winpriv.h"

/**
 Initialize an item array.

 @param StringArray Pointer to the item array to initialize.
 */
VOID
YoriStringArrayInitialize(
    __out PYORI_STRING_ARRAY StringArray
    )
{
    StringArray->Items = NULL;
    StringArray->Count = 0;
    StringArray->CountAllocated = 0;
    StringArray->StringAllocationBase = NULL;
    StringArray->StringAllocationCurrent = NULL;
    StringArray->StringAllocationRemaining = 0;
}

/**
 Deallocate all string allocations and the array allocation within an item
 array, allowing the array to be reused.

 @param StringArray Pointer to the array to clean up.
 */
VOID
YoriStringArrayCleanup(
    __inout PYORI_STRING_ARRAY StringArray
    )
{
    DWORD Index;

    for (Index = 0; Index < StringArray->Count; Index++) {
        YoriLibFreeStringContents(&StringArray->Items[Index]);
    }
    if (StringArray->Items != NULL) {
        YoriLibDereference(StringArray->Items);
        StringArray->Items = NULL;
    }

    if (StringArray->StringAllocationBase != NULL) {
        YoriLibDereference(StringArray->StringAllocationBase);
        StringArray->StringAllocationBase = NULL;
    }

    StringArray->Count = 0;
    StringArray->CountAllocated = 0;
    StringArray->StringAllocationCurrent = NULL;
    StringArray->StringAllocationRemaining = 0;
}

/**
 Ensure that the array of items has enough space for new items being added.

 @param StringArray Pointer to the item array.

 @param NumNewItems The new items that will be added to the existing set.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriStringArrayReallocateArrayForNewItems(
    __inout PYORI_STRING_ARRAY StringArray,
    __in YORI_ALLOC_SIZE_T NumNewItems
    )
{
    //
    //  If needed, reallocate the array of items.  Allocate 20% of the
    //  current number of items, or the new number of items, or 256,
    //  whichever is larger.  This is done to reduce the number of
    //  reallocations and copies of this array.
    //

    if (NumNewItems > StringArray->CountAllocated - StringArray->Count) {
        PYORI_STRING CombinedOptions;
        DWORD BytesRequired;
        DWORD ItemsToAllocate;

        ItemsToAllocate = (StringArray->CountAllocated / 5);

        if (ItemsToAllocate < NumNewItems) {
            ItemsToAllocate = NumNewItems;
        }

        if (ItemsToAllocate < 0x100) {
            ItemsToAllocate = 0x100;
        }

        ItemsToAllocate = ItemsToAllocate + StringArray->CountAllocated;
        BytesRequired = ItemsToAllocate * sizeof(YORI_STRING);

        if (!YoriLibIsSizeAllocatable(BytesRequired)) {
            return FALSE;
        }

        CombinedOptions = YoriLibReferencedMalloc(BytesRequired);
        if (CombinedOptions == NULL) {
            return FALSE;
        }
        if (StringArray->Count > 0) {
            memcpy(CombinedOptions, StringArray->Items, StringArray->Count * sizeof(YORI_STRING));
            YoriLibDereference(StringArray->Items);
        }

        StringArray->Items = CombinedOptions;
        StringArray->CountAllocated = (YORI_ALLOC_SIZE_T)ItemsToAllocate;
    }

    return TRUE;
}

/**
 Ensure that there is space in a buffer used to store strings for the new
 items being inserted.  This buffer can over allocate so that the same
 allocation can be used for later inserts, but since memory cannot be freed
 unless all items referencing the allocation have been removed, this
 overallocation must be lightweight.  Here the allocation will be up to a
 4Kb page minus special heap headers, so around 1900 chars, unless the
 caller requires more for a single insert, implying it is already batching.

 @param StringArray Pointer to the item array.

 @param CharsRequired The number of characters required to satisfy an
        insert operation.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriStringArrayEnsureSpaceForStrings(
    __inout PYORI_STRING_ARRAY StringArray,
    __in YORI_ALLOC_SIZE_T CharsRequired
    )
{
    LPTSTR NewStringBase;
    YORI_ALLOC_SIZE_T CharsToAllocate;

    if (StringArray->StringAllocationRemaining >= CharsRequired) {
        return TRUE;
    }

    CharsToAllocate = CharsRequired;
    if (CharsToAllocate * sizeof(TCHAR) < 4096 - 128) {
        CharsToAllocate = (4096 - 128) / sizeof(TCHAR);
    }

    NewStringBase = YoriLibReferencedMalloc(CharsToAllocate * sizeof(TCHAR));
    if (NewStringBase == NULL) {
        return FALSE;
    }

    if (StringArray->StringAllocationBase != NULL) {
        YoriLibDereference(StringArray->StringAllocationBase);
    }

    StringArray->StringAllocationBase = NewStringBase;
    StringArray->StringAllocationCurrent = NewStringBase;
    StringArray->StringAllocationRemaining = CharsToAllocate;

    return TRUE;
}

/**
 Adds new items to an item array.

 @param StringArray Pointer to the item array to add items to.

 @param NewItems Pointer to an array of new options to add.

 @param NumNewItems The number of new items to add.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriStringArrayAddItems(
    __inout PYORI_STRING_ARRAY StringArray,
    __in PCYORI_STRING NewItems,
    __in YORI_ALLOC_SIZE_T NumNewItems
    )
{
    LPTSTR StringAllocation;
    LPTSTR WritePtr;
    YORI_ALLOC_SIZE_T LengthInChars;
    YORI_ALLOC_SIZE_T Index;

    if (!YoriStringArrayReallocateArrayForNewItems(StringArray, NumNewItems)) {
        return FALSE;
    }

    //
    //  Now count the number of characters in all of the new items being
    //  inserted, and perform a single allocation for those.  This allocation
    //  may be a little larger to provide space for repeated calls.
    //

    LengthInChars = 0;
    for (Index = 0; Index < NumNewItems; Index++) {
        LengthInChars += (NewItems[Index].LengthInChars + 1) * sizeof(TCHAR);
    }

    if (!YoriStringArrayEnsureSpaceForStrings(StringArray, LengthInChars)) {
        return FALSE;
    }

    StringAllocation = StringArray->StringAllocationBase;
    WritePtr = StringArray->StringAllocationCurrent;

    for (Index = 0; Index < NumNewItems; Index++) {
        YoriLibReference(StringAllocation);
        StringArray->Items[Index + StringArray->Count].MemoryToFree = StringAllocation;
        LengthInChars = NewItems[Index].LengthInChars;
        StringArray->Items[Index + StringArray->Count].LengthInChars = LengthInChars;
        StringArray->Items[Index + StringArray->Count].LengthAllocated = LengthInChars + 1;
        StringArray->Items[Index + StringArray->Count].StartOfString = WritePtr;
        memcpy(WritePtr, NewItems[Index].StartOfString, LengthInChars * sizeof(TCHAR));
        StringArray->Items[Index + StringArray->Count].StartOfString[LengthInChars] = '\0';
        StringArray->StringAllocationRemaining -= LengthInChars + 1;
        WritePtr += LengthInChars + 1;
    }

    StringArray->StringAllocationCurrent = WritePtr;
    StringArray->Count = StringArray->Count + NumNewItems;
    return TRUE;
}

// vim:sw=4:ts=4:et:
