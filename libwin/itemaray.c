/**
 * @file libwin/itemaray.c
 *
 * Yori window list array
 *
 * Copyright (c) 2019-2020 Malcolm J. Smith
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

 @param ItemArray Pointer to the item array to initialize.
 */
VOID
YoriWinItemArrayInitialize(
    __out PYORI_WIN_ITEM_ARRAY ItemArray
    )
{
    ItemArray->Items = NULL;
    ItemArray->Count = 0;
    ItemArray->CountAllocated = 0;
    ItemArray->StringAllocationBase = NULL;
    ItemArray->StringAllocationCurrent = NULL;
    ItemArray->StringAllocationRemaining = 0;
}

/**
 Deallocate all string allocations and the array allocation within an item
 array, allowing the array to be reused.

 @param ItemArray Pointer to the array to clean up.
 */
VOID
YoriWinItemArrayCleanup(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray
    )
{
    DWORD Index;

    for (Index = 0; Index < ItemArray->Count; Index++) {
        YoriLibFreeStringContents(&ItemArray->Items[Index].String);
    }
    if (ItemArray->Items != NULL) {
        YoriLibDereference(ItemArray->Items);
        ItemArray->Items = NULL;
    }

    if (ItemArray->StringAllocationBase != NULL) {
        YoriLibDereference(ItemArray->StringAllocationBase);
        ItemArray->StringAllocationBase = NULL;
    }

    ItemArray->Count = 0;
    ItemArray->CountAllocated = 0;
    ItemArray->StringAllocationCurrent = NULL;
    ItemArray->StringAllocationRemaining = 0;
}

/**
 Ensure that the array of items has enough space for new items being added.

 @param ItemArray Pointer to the item array.

 @param NumNewItems The new items that will be added to the existing set.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinItemArrayReallocateArrayForNewItems(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray,
    __in DWORD NumNewItems
    )
{
    //
    //  If needed, reallocate the array of items.  Allocate 20% of the
    //  current number of items, or the new number of items, or 256,
    //  whichever is larger.  This is done to reduce the number of
    //  reallocations and copies of this array.
    //

    if (NumNewItems > ItemArray->CountAllocated - ItemArray->Count) {
        DWORD ItemsToAllocate;
        PYORI_WIN_ITEM_ENTRY CombinedOptions;

        ItemsToAllocate = (ItemArray->CountAllocated / 5);

        if (ItemsToAllocate < NumNewItems) {
            ItemsToAllocate = NumNewItems;
        }

        if (ItemsToAllocate < 0x100) {
            ItemsToAllocate = 0x100;
        }

        ItemsToAllocate = ItemsToAllocate + ItemArray->CountAllocated;

        CombinedOptions = YoriLibReferencedMalloc(ItemsToAllocate * sizeof(YORI_WIN_ITEM_ENTRY));
        if (CombinedOptions == NULL) {
            return FALSE;
        }
        if (ItemArray->Count > 0) {
            memcpy(CombinedOptions, ItemArray->Items, ItemArray->Count * sizeof(YORI_WIN_ITEM_ENTRY));
            YoriLibDereference(ItemArray->Items);
        }

        ItemArray->Items = CombinedOptions;
        ItemArray->CountAllocated = ItemsToAllocate;
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

 @param ItemArray Pointer to the item array.

 @param CharsRequired The number of characters required to satisfy an
        insert operation.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinItemArrayEnsureSpaceForStrings(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray,
    __in DWORD CharsRequired
    )
{
    LPTSTR NewStringBase;
    DWORD CharsToAllocate;

    if (ItemArray->StringAllocationRemaining >= CharsRequired) {
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

    if (ItemArray->StringAllocationBase != NULL) {
        YoriLibDereference(ItemArray->StringAllocationBase);
    }

    ItemArray->StringAllocationBase = NewStringBase;
    ItemArray->StringAllocationCurrent = NewStringBase;
    ItemArray->StringAllocationRemaining = CharsToAllocate;

    return TRUE;
}

/**
 Adds new items to an item array.

 @param ItemArray Pointer to the item array to add items to.

 @param NewItems Pointer to an array of new options to add.

 @param NumNewItems The number of new items to add.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinItemArrayAddItems(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray,
    __in PYORI_STRING NewItems,
    __in DWORD NumNewItems
    )
{
    LPTSTR StringAllocation;
    LPTSTR WritePtr;
    DWORD LengthInChars;
    DWORD Index;

    if (!YoriWinItemArrayReallocateArrayForNewItems(ItemArray, NumNewItems)) {
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

    if (!YoriWinItemArrayEnsureSpaceForStrings(ItemArray, LengthInChars)) {
        return FALSE;
    }

    StringAllocation = ItemArray->StringAllocationBase;
    WritePtr = ItemArray->StringAllocationCurrent;

    for (Index = 0; Index < NumNewItems; Index++) {
        YoriLibReference(StringAllocation);
        ItemArray->Items[Index + ItemArray->Count].String.MemoryToFree = StringAllocation;
        LengthInChars = NewItems[Index].LengthInChars;
        ItemArray->Items[Index + ItemArray->Count].String.LengthInChars = LengthInChars;
        ItemArray->Items[Index + ItemArray->Count].String.LengthAllocated = LengthInChars + 1;
        ItemArray->Items[Index + ItemArray->Count].String.StartOfString = WritePtr;
        memcpy(WritePtr, NewItems[Index].StartOfString, LengthInChars * sizeof(TCHAR));
        ItemArray->Items[Index + ItemArray->Count].String.StartOfString[LengthInChars] = '\0';
        ItemArray->Items[Index + ItemArray->Count].Flags = 0;
        ItemArray->StringAllocationRemaining -= LengthInChars + 1;
        WritePtr += LengthInChars + 1;
    }

    ItemArray->StringAllocationCurrent = WritePtr;
    ItemArray->Count += NumNewItems;
    return TRUE;
}

/**
 Adds new items from one item array to an existing item array.

 @param ItemArray Pointer to the item array to add items to.

 @param NewItems Pointer to an array of new options to add.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinItemArrayAddItemArray(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray,
    __in PYORI_WIN_ITEM_ARRAY NewItems
    )
{
    LPTSTR StringAllocation;
    LPTSTR WritePtr;
    DWORD LengthInChars;
    DWORD Index;

    if (!YoriWinItemArrayReallocateArrayForNewItems(ItemArray, NewItems->Count)) {
        return FALSE;
    }

    //
    //  Now count the number of characters in all of the new items being
    //  inserted, and perform a single allocation for those.
    //

    LengthInChars = 0;
    for (Index = 0; Index < NewItems->Count; Index++) {
        LengthInChars += (NewItems->Items[Index].String.LengthInChars + 1) * sizeof(TCHAR);
    }

    if (!YoriWinItemArrayEnsureSpaceForStrings(ItemArray, LengthInChars)) {
        return FALSE;
    }

    StringAllocation = ItemArray->StringAllocationBase;
    WritePtr = ItemArray->StringAllocationCurrent;

    for (Index = 0; Index < NewItems->Count; Index++) {
        YoriLibReference(StringAllocation);
        ItemArray->Items[Index + ItemArray->Count].String.MemoryToFree = StringAllocation;
        LengthInChars = NewItems->Items[Index].String.LengthInChars;
        ItemArray->Items[Index + ItemArray->Count].String.LengthInChars = LengthInChars;
        ItemArray->Items[Index + ItemArray->Count].String.LengthAllocated = LengthInChars + 1;
        ItemArray->Items[Index + ItemArray->Count].String.StartOfString = WritePtr;
        memcpy(WritePtr, NewItems->Items[Index].String.StartOfString, LengthInChars * sizeof(TCHAR));
        ItemArray->Items[Index + ItemArray->Count].String.StartOfString[LengthInChars] = '\0';
        ItemArray->Items[Index + ItemArray->Count].Flags = NewItems->Items[Index].Flags;
        ItemArray->StringAllocationRemaining -= LengthInChars + 1;
        WritePtr += LengthInChars + 1;
    }

    ItemArray->StringAllocationCurrent = WritePtr;
    ItemArray->Count += NewItems->Count;
    return TRUE;
}

// vim:sw=4:ts=4:et:
