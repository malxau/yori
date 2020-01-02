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
    ItemArray->Count = 0;
}


/**
 Adds new items to an item array.

 @param ItemArray Pointer to the item array to add items to.

 @param NewItems Pointer to an array of new options to add.

 @param NumNewItems The number of new items to add.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriWinItemArrayAddItems(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray,
    __in PYORI_STRING NewItems,
    __in DWORD NumNewItems
    )
{
    PYORI_WIN_ITEM_ENTRY CombinedOptions;
    DWORD BytesToAllocate;
    LPTSTR WritePtr;
    DWORD LengthInChars;
    DWORD Index;

    BytesToAllocate = (ItemArray->Count + NumNewItems) * sizeof(YORI_WIN_ITEM_ENTRY);
    for (Index = 0; Index < NumNewItems; Index++) {
        BytesToAllocate += (NewItems[Index].LengthInChars + 1) * sizeof(TCHAR);
    }

    CombinedOptions = YoriLibReferencedMalloc(BytesToAllocate);
    if (CombinedOptions == NULL) {
        return FALSE;
    }

    WritePtr = YoriLibAddToPointer(CombinedOptions, (ItemArray->Count + NumNewItems) * sizeof(YORI_WIN_ITEM_ENTRY));

    if (ItemArray->Count > 0) {
        memcpy(CombinedOptions, ItemArray->Items, ItemArray->Count * sizeof(YORI_WIN_ITEM_ENTRY));
    }

    for (Index = 0; Index < NumNewItems; Index++) {
        YoriLibReference(CombinedOptions);
        CombinedOptions[Index + ItemArray->Count].String.MemoryToFree = CombinedOptions;
        LengthInChars = NewItems[Index].LengthInChars;
        CombinedOptions[Index + ItemArray->Count].String.LengthInChars = LengthInChars;
        CombinedOptions[Index + ItemArray->Count].String.LengthAllocated = LengthInChars + 1;
        CombinedOptions[Index + ItemArray->Count].String.StartOfString = WritePtr;
        memcpy(WritePtr, NewItems[Index].StartOfString, NewItems[Index].LengthInChars * sizeof(TCHAR));
        CombinedOptions[Index + ItemArray->Count].String.StartOfString[LengthInChars] = '\0';
        CombinedOptions[Index + ItemArray->Count].Flags = 0;
        WritePtr += LengthInChars + 1;
    }

    if (ItemArray->Items != NULL) {
        YoriLibDereference(ItemArray->Items);
    }
    ItemArray->Items = CombinedOptions;

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
BOOL
YoriWinItemArrayAddItemArray(
    __inout PYORI_WIN_ITEM_ARRAY ItemArray,
    __in PYORI_WIN_ITEM_ARRAY NewItems
    )
{
    PYORI_WIN_ITEM_ENTRY CombinedOptions;
    DWORD BytesToAllocate;
    LPTSTR WritePtr;
    DWORD LengthInChars;
    DWORD Index;

    BytesToAllocate = (ItemArray->Count + NewItems->Count) * sizeof(YORI_WIN_ITEM_ENTRY);
    for (Index = 0; Index < NewItems->Count; Index++) {
        BytesToAllocate += (NewItems->Items[Index].String.LengthInChars + 1) * sizeof(TCHAR);
    }

    CombinedOptions = YoriLibReferencedMalloc(BytesToAllocate);
    if (CombinedOptions == NULL) {
        return FALSE;
    }

    WritePtr = YoriLibAddToPointer(CombinedOptions, (ItemArray->Count + NewItems->Count) * sizeof(YORI_WIN_ITEM_ENTRY));

    if (ItemArray->Count > 0) {
        memcpy(CombinedOptions, ItemArray->Items, ItemArray->Count * sizeof(YORI_WIN_ITEM_ENTRY));
    }

    for (Index = 0; Index < NewItems->Count; Index++) {
        YoriLibReference(CombinedOptions);
        CombinedOptions[Index + ItemArray->Count].String.MemoryToFree = CombinedOptions;
        LengthInChars = NewItems->Items[Index].String.LengthInChars;
        CombinedOptions[Index + ItemArray->Count].String.LengthInChars = LengthInChars;
        CombinedOptions[Index + ItemArray->Count].String.LengthAllocated = LengthInChars + 1;
        CombinedOptions[Index + ItemArray->Count].String.StartOfString = WritePtr;
        memcpy(WritePtr, NewItems->Items[Index].String.StartOfString, NewItems->Items[Index].String.LengthInChars * sizeof(TCHAR));
        CombinedOptions[Index + ItemArray->Count].String.StartOfString[LengthInChars] = '\0';
        CombinedOptions[Index + ItemArray->Count].Flags = NewItems->Items[Index].Flags;
        WritePtr += LengthInChars + 1;
    }

    if (ItemArray->Items != NULL) {
        YoriLibDereference(ItemArray->Items);
    }
    ItemArray->Items = CombinedOptions;

    ItemArray->Count += NewItems->Count;
    return TRUE;
}

// vim:sw=4:ts=4:et:
