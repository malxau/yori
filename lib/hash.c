/**
 * @file lib/hash.c
 *
 * Yori hash table manipulation routines
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
 Allocate an empty hash table.

 @param NumberBuckets The number of buckets to allocate into the hash table.

 @return On successful completion, points to the resulting hash table.
         On allocation failure, returns NULL.
 */
PYORI_HASH_TABLE
YoriLibAllocateHashTable(
    __in DWORD NumberBuckets
    )
{
    DWORD SizeNeeded = sizeof(YORI_HASH_TABLE) + NumberBuckets * sizeof(YORI_HASH_BUCKET);
    PYORI_HASH_TABLE HashTable;
    DWORD BucketIndex;

    HashTable = YoriLibReferencedMalloc(SizeNeeded);
    if (HashTable == NULL) {
        return NULL;
    }

    HashTable->NumberBuckets = NumberBuckets;
    HashTable->Buckets = (PYORI_HASH_BUCKET)(HashTable + 1);

    for (BucketIndex = 0; BucketIndex < NumberBuckets; BucketIndex++) {
        YoriLibInitializeListHead(&HashTable->Buckets[BucketIndex].ListHead);
    }

    return HashTable;
}

/**
 Free a hash table.  This assumes the caller has already removed and
 performed all necessary cleanup for any objects within it.

 @param HashTable Pointer to the hash table to deallocate.
 */
VOID
YoriLibFreeEmptyHashTable(
    __in PYORI_HASH_TABLE HashTable
    )
{
#if DBG
    DWORD BucketIndex;

    for (BucketIndex = 0; BucketIndex < HashTable->NumberBuckets; BucketIndex++) {
        ASSERT(YoriLibGetNextListEntry(&HashTable->Buckets[BucketIndex].ListHead, NULL) == NULL);
    }
#endif

    YoriLibDereference(HashTable);
}

/**
 Hash a yori string into a 16 bit hash value.

 @param String The string to generate a hash for.

 @return A 16 bit hash value for the string.
 */
WORD
YoriLibHashString(
    __in PCYORI_STRING String
    )
{
    DWORD Hash;
    DWORD Index;

    //
    //  Simple string xor hash
    //

    Hash = 0;
    for (Index = 0; Index < String->LengthInChars; Index++) {
        Hash = (Hash << 3) ^ YoriLibUpcaseChar(String->StartOfString[Index]) ^ (Hash >> 29);
    }

    //
    //  Move some high bits into the low bits since the low bits
    //  will likely be used as a bucket index.  Note we're moving
    //  bits 16 here and 3 above (ie., not divisible and won't
    //  cancel out.)
    //

    Hash = Hash ^ (Hash >> 16);
    return (WORD)Hash;
}

/**
 Insert an object with a string based key into the hash table.

 @param HashTable The hash table to insert the object into.

 @param KeyString Pointer to a Yori string describing the key for the
        entry.

 @param Context Pointer to a blob of data which is meaningful to the caller.

 @param HashEntry On successful completion, populated with structures
        describing the entry within the hash table.
 */
VOID
YoriLibHashInsertByKey(
    __in PYORI_HASH_TABLE HashTable,
    __in PYORI_STRING KeyString,
    __in PVOID Context,
    __out PYORI_HASH_ENTRY HashEntry
    )
{
    DWORD BucketIndex = YoriLibHashString(KeyString) % HashTable->NumberBuckets;

    YoriLibCloneString(&HashEntry->Key, KeyString);
    HashEntry->Context = Context;
    YoriLibInsertList(&HashTable->Buckets[BucketIndex].ListHead, &HashEntry->ListEntry);
}

/**
 Locate an object within the hash table by a specified key.

 @param HashTable Pointer to the hash table to search for the object.

 @param KeyString Pointer to the key to identify the object.

 @return Pointer to the entry within the hash table if a match is found.
         If no match is found, returns NULL.
 */
PYORI_HASH_ENTRY
YoriLibHashLookupByKey(
    __in PYORI_HASH_TABLE HashTable,
    __in PCYORI_STRING KeyString
    )
{
    DWORD BucketIndex = YoriLibHashString(KeyString) % HashTable->NumberBuckets;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_HASH_ENTRY HashEntry;

    HashEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&HashTable->Buckets[BucketIndex].ListHead, NULL);
    while (ListEntry != NULL) {
        HashEntry = CONTAINING_RECORD(ListEntry, YORI_HASH_ENTRY, ListEntry);
        if (YoriLibCompareStringInsensitive(KeyString, &HashEntry->Key) == 0) {
            break;
        }
        HashEntry = NULL;
        ListEntry = YoriLibGetNextListEntry(&HashTable->Buckets[BucketIndex].ListHead, ListEntry);
    }

    return HashEntry;
}

/**
 Remove an entry from a hash table.  This routine assumes the entry must
 already be inserted into a hash table.

 @param HashEntry The entry to remove.
 */
VOID
YoriLibHashRemoveByEntry(
    __in PYORI_HASH_ENTRY HashEntry
    )
{
    YoriLibRemoveListItem(&HashEntry->ListEntry);
    YoriLibFreeStringContents(&HashEntry->Key);
}

/**
 Remove a hash entry from a hash table by performing a lookup by key.

 @param HashTable The hash table to remove the entry from.

 @param KeyString The key matching the object to remove.

 @return Pointer to the entry if one was removed, or NULL if no match was
         found.
 */
PYORI_HASH_ENTRY
YoriLibHashRemoveByKey(
    __in PYORI_HASH_TABLE HashTable,
    __in PYORI_STRING KeyString
    )
{
    PYORI_HASH_ENTRY Entry;
    Entry = YoriLibHashLookupByKey(HashTable, KeyString);
    if (Entry != NULL) {
        YoriLibHashRemoveByEntry(Entry);
    }

    return Entry;
}

// vim:sw=4:ts=4:et:
