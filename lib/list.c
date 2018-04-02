/**
 * @file lib/list.c
 *
 * Yori list manipulation routines
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
 Initialize a list head.

 @param ListEntry The list head to initialize.
 */
VOID
YoriLibInitializeListHead(
    __out PYORI_LIST_ENTRY ListEntry
    )
{
    ListEntry->Next = ListEntry;
    ListEntry->Prev = ListEntry;
}

/**
 Append a new entry to the tail of a list.

 @param ListHead The list to insert the entry to.

 @param ListEntry The entry to insert.
 */
VOID
YoriLibAppendList(
    __in PYORI_LIST_ENTRY ListHead,
    __out PYORI_LIST_ENTRY ListEntry
    )
{
    ListEntry->Next = ListHead;
    ListEntry->Prev = ListHead->Prev;

    ListEntry->Prev->Next = ListEntry;
    ListHead->Prev = ListEntry;
}

/**
 Inserts a new entry to the head of a list.

 @param ListHead The list to insert the entry to.

 @param ListEntry The entry to insert.
 */
VOID
YoriLibInsertList(
    __in PYORI_LIST_ENTRY ListHead,
    __out PYORI_LIST_ENTRY ListEntry
    )
{
    ListEntry->Next = ListHead->Next;
    ListEntry->Prev = ListHead;

    ListEntry->Next->Prev = ListEntry;
    ListHead->Next = ListEntry;
}

/**
 Remove an entry that is currently on a list.

 @param ListEntry The entry to remove.
 */
VOID
YoriLibRemoveListItem(
    __in PYORI_LIST_ENTRY ListEntry
    )
{
    ListEntry->Prev->Next = ListEntry->Next;
    ListEntry->Next->Prev = ListEntry->Prev;
}

/**
 Enumerate through a list and return entries.  When the end of the list is
 reached, return NULL.

 @param ListHead The list to enumerate entries from.

 @param PreviousEntry If specified, the previously returned entry, where the
        next entry should be returned from.  Can be NULL to indicate that no
        entries have previously been returned and enumeration should commence
        from the beginning.

 @return Pointer to the next list entry, or NULL if the end of the list has
         been reached.
 */
PYORI_LIST_ENTRY
YoriLibGetNextListEntry(
    __in PYORI_LIST_ENTRY ListHead,
    __in_opt PYORI_LIST_ENTRY PreviousEntry
    )
{
    if (PreviousEntry == NULL) {
        if (ListHead->Next == ListHead) {
            return NULL;
        }
        return ListHead->Next;
    } else {
        if (PreviousEntry->Next == ListHead) {
            return NULL;
        }
        return PreviousEntry->Next;
    }
}

/**
 Enumerate through a list and return entries.  When the end of the list is
 reached, return NULL.

 @param ListHead The list to enumerate entries from.

 @param NextEntry If specified, the previously returned entry, where the
        next entry should be returned from.  Can be NULL to indicate that no
        entries have previously been returned and enumeration should commence
        from the beginning.

 @return Pointer to the previous list entry, or NULL if the end of the list
         has been reached.
 */
PYORI_LIST_ENTRY
YoriLibGetPreviousListEntry(
    __in PYORI_LIST_ENTRY ListHead,
    __in_opt PYORI_LIST_ENTRY NextEntry
    )
{
    if (NextEntry == NULL) {
        if (ListHead->Prev == ListHead) {
            return NULL;
        }
        return ListHead->Prev;
    } else {
        if (NextEntry->Prev == ListHead) {
            return NULL;
        }
        return NextEntry->Prev;
    }
}

// vim:sw=4:ts=4:et:
