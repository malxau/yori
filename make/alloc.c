/**
 * @file make/alloc.c
 *
 * Yori shell make slab allocator
 *
 * Copyright (c) 2020 Malcolm J. Smith
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

#include <yoripch.h>
#include <yorilib.h>
#include "make.h"

/**
 A structure before each allocation returned to callers.
 */
typedef struct _MAKE_SLAB_ALLOC_HDR {

    /**
     Pointer to the base of the memory allocation which should be dereferenced
     on free.
     */
    PVOID MemoryToFree;
} MAKE_SLAB_ALLOC_HDR, *PMAKE_SLAB_ALLOC_HDR;

/**
 Allocate a new fixed sized structure.

 @param Alloc Pointer to the allocator which tracks the allocation backing
        multiple fixed sized structures.

 @param SizeInBytes The size of the item to allocate.

 @return Pointer to the newly allocated object or NULL on allocation failure.
 */
PVOID
MakeSlabAlloc(
    __in PMAKE_SLAB_ALLOC Alloc,
    __in DWORD SizeInBytes
    )
{
    PMAKE_SLAB_ALLOC_HDR Hdr;

    if (Alloc->ElementSize != 0 &&
        Alloc->ElementSize != SizeInBytes) {

        return NULL;
    }

    if (Alloc->ElementSize == 0) {
        Alloc->ElementSize = SizeInBytes;
    }

    if (Alloc->NumberAllocatedFromSystem == Alloc->NumberAllocatedToCaller) {
        if (Alloc->Buffer != NULL) {
            YoriLibDereference(Alloc->Buffer);
        }

        Alloc->Buffer = YoriLibReferencedMalloc(0x100 * (sizeof(MAKE_SLAB_ALLOC_HDR) + Alloc->ElementSize));
        if (Alloc->Buffer == NULL) {
            return NULL;
        }

        Alloc->NumberAllocatedFromSystem = 0x100;
        Alloc->NumberAllocatedToCaller = 0;
    }

    Hdr = YoriLibAddToPointer(Alloc->Buffer, Alloc->NumberAllocatedToCaller * (Alloc->ElementSize + sizeof(MAKE_SLAB_ALLOC_HDR)));
    Alloc->NumberAllocatedToCaller++;
    Hdr->MemoryToFree = Alloc->Buffer;
    YoriLibReference(Alloc->Buffer);
    return (Hdr + 1);
}

/**
 Return the header location given a pointer that was handed out from a
 previous call to MakeSlabAlloc.

 @param Ptr The previously allocated element.

 @return Pointer to the header.
 */
PMAKE_SLAB_ALLOC_HDR
MakeSlabHdrFromPtr(
    __in PVOID Ptr
    )
{
    return YoriLibSubtractFromPointer(Ptr, sizeof(MAKE_SLAB_ALLOC_HDR));
}

/**
 Free an element that was previously allocated from MakeSlabAlloc.

 @param Ptr Pointer to the element to free.
 */
VOID
MakeSlabFree(
    __in PVOID Ptr
    )
{
    PMAKE_SLAB_ALLOC_HDR Hdr;
    Hdr = MakeSlabHdrFromPtr(Ptr);
    YoriLibDereference(Hdr->MemoryToFree);
}

/**
 Cleanup a slab allocation structure, freeing any memory that was allocated
 from the system but not yet consumed.

 @param Alloc Pointer to the slab allocator.
 */
VOID
MakeSlabCleanup(
    __in PMAKE_SLAB_ALLOC Alloc
    )
{
    if (Alloc->Buffer != NULL) {
        YoriLibDereference(Alloc->Buffer);
    }

    Alloc->Buffer = NULL;
    Alloc->NumberAllocatedFromSystem = 0;
    Alloc->NumberAllocatedToCaller = 0;
    Alloc->ElementSize = 0;
}


// vim:sw=4:ts=4:et:
