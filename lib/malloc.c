/**
 * @file lib/malloc.c
 *
 * Yori memory allocation wrappers
 *
 * Copyright (c) 2017-2019 Malcolm J. Smith
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

#if YORI_SPECIAL_HEAP

#if YORI_MAX_ALLOC_SIZE < ((DWORD)-1)
/**
 This module allocates more bytes than specified to support page alignment
 and guard pages.  Since this is NT specific, use 32 bit integers here to
 support 16 bit allocations.
 */
typedef DWORD YORI_SPECIAL_ALLOC_SIZE_T;
#else
typedef YORI_ALLOC_SIZE_T YORI_SPECIAL_ALLOC_SIZE_T;
#endif

/**
 The number of stack frames to capture when the OS supports it.
 */
#define YORI_SPECIAL_HEAP_STACK_FRAMES (10)

/**
 A structure embedded at the top of special heap allocations.
 */
typedef struct _YORI_SPECIAL_HEAP_HEADER {

    /**
     Offset in bytes from the top of the page to the user's allocation.
     */
    YORI_SPECIAL_ALLOC_SIZE_T OffsetToData;

    /**
     Number of pages in the allocation.  Special heap allocations are
     always a multiple of pages.
     */
    YORI_SPECIAL_ALLOC_SIZE_T PagesInAllocation;

    /**
     List of all active memory allocations.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The function that allocated the memory.
     */
    LPCSTR Function;

    /**
     The source file that allocated the memory.
     */
    LPCSTR File;

    /**
     The line number that allocated the memory.
     */
    DWORD Line;

    /**
     An extra 32 bits that does nothing to ensure 64 bit alignment of this
     structure so that 64 bit builds will capture pointer values on 64 bit
     boundaries.
     */
    DWORD ReservedForAlignment;

    /**
     This structure may be followed by the stack that allocated this
     allocation.
     */

} YORI_SPECIAL_HEAP_HEADER, *PYORI_SPECIAL_HEAP_HEADER;

/**
 A structure containing process global state for the special heap allocator.
 */
typedef struct _YORI_SPECIAL_HEAP_GLOBAL {

    /**
     The number of allocations that have previously been allocated by the caller.
     */
    DWORD NumberAllocated;

    /**
     The number of allocations that have previously been freed by the caller.
     */
    DWORD NumberFreed;

    /**
     The number of bytes currently allocated, from the user's perspective.
     */
    DWORD BytesCurrentlyAllocated;

    /**
     A list of all active allocations in the system.
     */
    YORI_LIST_ENTRY ActiveAllocationsList;

    /**
     An array of recently freed allocations.  Each of these is accessed in a
     circular fashion, so that the least recently freed object is replaced by
     a new incoming free.  While on this array allocations are not readable
     or writable so that use after free bugs become visibly apparent.
     */
    PYORI_SPECIAL_HEAP_HEADER RecentlyFreed[8];

    /**
     A handle to a mutex to synchronize the debug heap.
     */
    HANDLE Mutex;

} YORI_SPECIAL_HEAP_GLOBAL, PYORI_SPECIAL_HEAP_GLOBAL;

/**
 Process global state for the special heap allocator.
 */
YORI_SPECIAL_HEAP_GLOBAL YoriLibSpecialHeap;

#endif

#if !YORI_SPECIAL_HEAP
/**
 Allocate memory.  This should be freed with @ref YoriLibFree when it is no
 longer needed.

 @param Bytes The number of bytes to allocate.

 @return A pointer to the newly allocated memory, or NULL on failure.
 */
PVOID
YoriLibMalloc(
    __in YORI_ALLOC_SIZE_T Bytes
    )
{
    PVOID Alloc;
    Alloc = HeapAlloc(GetProcessHeap(), 0, Bytes);
    return Alloc;
}
#else

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(disable: 6250) // Calling VirtualFree without MEM_RELEASE,
                               // this module uses two calls
#endif
/**
 Allocate memory.  This should be freed with @ref YoriLibFree when it is no
 longer needed.

 @param Bytes The number of bytes to allocate.

 @param Function Pointer to a constant string indicating the function that is
        allocating the memory.

 @param File Pointer to a constant string indicating the source file that is
        allocating the memory.

 @param Line Specifies the line number within the source file that is
        allocating the memory.

 @return A pointer to the newly allocated memory, or NULL on failure.
 */
PVOID
YoriLibMallocSpecialHeap(
    __in YORI_ALLOC_SIZE_T Bytes,
    __in LPCSTR Function,
    __in LPCSTR File,
    __in DWORD Line
    )
{
    YORI_SPECIAL_ALLOC_SIZE_T TotalPagesNeeded;
    PYORI_SPECIAL_HEAP_HEADER Header;
    PYORI_SPECIAL_HEAP_HEADER Commit;
    YORI_ALLOC_SIZE_T StackSize;
    DWORD OldAccess;
    YORI_SPECIAL_ALLOC_SIZE_T PageSize;
#if defined(_M_MRX000) || defined(_M_ARM64) || defined(_M_ALPHA) || defined(_M_PPC)
    YORI_SPECIAL_ALLOC_SIZE_T Alignment = sizeof(DWORD);
#else
    YORI_SPECIAL_ALLOC_SIZE_T Alignment = sizeof(UCHAR);
#endif

    StackSize = 0;
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        StackSize = sizeof(PVOID) * YORI_SPECIAL_HEAP_STACK_FRAMES;
    }

    PageSize = YoriLibGetPageSize();

    TotalPagesNeeded = Bytes;
    TotalPagesNeeded = (TotalPagesNeeded + sizeof(YORI_SPECIAL_HEAP_HEADER) + StackSize + 2 * PageSize - 1) / PageSize;

    if (YoriLibSpecialHeap.Mutex == NULL) {
        YoriLibSpecialHeap.Mutex = CreateMutex(NULL, FALSE, NULL);
    }

    if (YoriLibSpecialHeap.Mutex == NULL) {
        return NULL;
    }

    if (YoriLibSpecialHeap.ActiveAllocationsList.Next == NULL) {
        YoriLibInitializeListHead(&YoriLibSpecialHeap.ActiveAllocationsList);
    }

    Header = VirtualAlloc(NULL, TotalPagesNeeded * PageSize, MEM_RESERVE, PAGE_READWRITE);
    if (Header == NULL) {
        return NULL;
    }

    Commit = VirtualAlloc(Header, TotalPagesNeeded * PageSize, MEM_COMMIT, PAGE_READWRITE);
    if (Commit == NULL) {
        VirtualFree(Header, 0, MEM_RELEASE);
        return NULL;
    }

    if (!VirtualProtect((PUCHAR)Header + (TotalPagesNeeded - 1) * PageSize, PageSize, PAGE_NOACCESS, &OldAccess)) {
        VirtualFree(Header, TotalPagesNeeded * PageSize, MEM_DECOMMIT);
        VirtualFree(Header, 0, MEM_RELEASE);
        return NULL;
    }

    FillMemory(Header, (TotalPagesNeeded - 1) * PageSize, '@');

    Header->PagesInAllocation = TotalPagesNeeded;
    Header->OffsetToData = ((YORI_SPECIAL_ALLOC_SIZE_T)((TotalPagesNeeded - 1) * PageSize - Bytes)) & (YORI_SPECIAL_ALLOC_SIZE_T)~(Alignment - 1);
    Header->Function = Function;
    Header->File = File;
    Header->Line = Line;
    ASSERT(Header->OffsetToData < (PageSize + sizeof(YORI_SPECIAL_HEAP_HEADER) + StackSize));
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        DllKernel32.pRtlCaptureStackBackTrace(1,
                                              YORI_SPECIAL_HEAP_STACK_FRAMES,
                                              YoriLibAddToPointer(Header, sizeof(YORI_SPECIAL_HEAP_HEADER)),
                                              NULL);
    }

    WaitForSingleObject(YoriLibSpecialHeap.Mutex, INFINITE);
    YoriLibSpecialHeap.NumberAllocated++;
    YoriLibSpecialHeap.BytesCurrentlyAllocated += (Bytes + Alignment - 1) & ~(Alignment - 1);
    YoriLibAppendList(&YoriLibSpecialHeap.ActiveAllocationsList, &Header->ListEntry);
    ReleaseMutex(YoriLibSpecialHeap.Mutex);

    return (PUCHAR)Header + Header->OffsetToData;
}
#endif

/**
 Free memory previously allocated with @ref YoriLibMalloc.

 @param Ptr The memory to free.
 */
VOID
YoriLibFree(
    __in PVOID Ptr
    )
{
#if !YORI_SPECIAL_HEAP
    HeapFree(GetProcessHeap(), 0, Ptr);
#else
    PYORI_SPECIAL_HEAP_HEADER Header;
    PUCHAR TestChar;
    DWORD MyEntry;
    DWORD OldAccess;
    YORI_SPECIAL_ALLOC_SIZE_T BytesToFree;
    YORI_SPECIAL_ALLOC_SIZE_T StackSize;
    DWORD_PTR PageSize;

    StackSize = 0;
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        StackSize = sizeof(PVOID) * YORI_SPECIAL_HEAP_STACK_FRAMES;
    }

    PageSize = YoriLibGetPageSize();

    Header = YoriLibSubtractFromPointer(Ptr, sizeof(YORI_SPECIAL_HEAP_HEADER) + StackSize);
    Header = (PYORI_SPECIAL_HEAP_HEADER)((DWORD_PTR)Header & ~(PageSize - 1));

    TestChar = (PUCHAR)Header;
    ASSERT(TestChar + Header->OffsetToData == Ptr);

    TestChar = (PUCHAR)YoriLibAddToPointer(Header, sizeof(YORI_SPECIAL_HEAP_HEADER) + StackSize);
    while (TestChar < (PUCHAR)Ptr) {
        ASSERT(*TestChar == '@');
        TestChar++;
    }

    BytesToFree = (Header->PagesInAllocation - 1) * (YORI_SPECIAL_ALLOC_SIZE_T)PageSize - Header->OffsetToData;

    WaitForSingleObject(YoriLibSpecialHeap.Mutex, INFINITE);

    MyEntry = YoriLibSpecialHeap.NumberFreed % (sizeof(YoriLibSpecialHeap.RecentlyFreed)/sizeof(YoriLibSpecialHeap.RecentlyFreed[0]));
    YoriLibSpecialHeap.NumberFreed++;
    YoriLibSpecialHeap.BytesCurrentlyAllocated -= BytesToFree;
    YoriLibRemoveListItem(&Header->ListEntry);

    if (YoriLibSpecialHeap.RecentlyFreed[MyEntry] != NULL) {

        PYORI_SPECIAL_HEAP_HEADER OldHeader;

        OldHeader = YoriLibSpecialHeap.RecentlyFreed[MyEntry];

        if (!VirtualProtect(OldHeader, PageSize, PAGE_READWRITE, &OldAccess)) {
            ASSERT(!"VirtualProtect failure");
        }
        if (!VirtualFree(OldHeader, OldHeader->PagesInAllocation * PageSize, MEM_DECOMMIT)) {
            ASSERT(!"VirtualFree failure");
        }
        if (!VirtualFree(OldHeader, 0, MEM_RELEASE)) {
            ASSERT(!"VirtualFree failure");
        }
    }

    if (!VirtualProtect(Header, Header->PagesInAllocation * PageSize, PAGE_NOACCESS, &OldAccess)) {
        ASSERT(!"VirtualProtect failure");
    }

    YoriLibSpecialHeap.RecentlyFreed[MyEntry] = Header;

    ReleaseMutex(YoriLibSpecialHeap.Mutex);

#endif
}

/**
 When using memory debugging, display the number of bytes of allocation and
 number of allocations currently in use.  When memory debugging is not present,
 does nothing.
 */
VOID
YoriLibDisplayMemoryUsage(VOID)
{
#if YORI_SPECIAL_HEAP
    YORI_ALLOC_SIZE_T PageSize;
    PageSize = YoriLibGetPageSize();
    if (YoriLibSpecialHeap.BytesCurrentlyAllocated > 0 ||
        (YoriLibSpecialHeap.NumberAllocated - YoriLibSpecialHeap.NumberFreed > 0)) {

        PYORI_LIST_ENTRY Entry;
        PYORI_SPECIAL_HEAP_HEADER Header;
        YORI_SPECIAL_ALLOC_SIZE_T BytesAllocated;

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%i bytes allocated in %i allocations\n"),
                      YoriLibSpecialHeap.BytesCurrentlyAllocated,
                      YoriLibSpecialHeap.NumberAllocated - YoriLibSpecialHeap.NumberFreed);

        Entry = YoriLibGetNextListEntry(&YoriLibSpecialHeap.ActiveAllocationsList, NULL);
        while (Entry != NULL) {
            Header = CONTAINING_RECORD(Entry, YORI_SPECIAL_HEAP_HEADER, ListEntry);
            BytesAllocated = (Header->PagesInAllocation - 1) * PageSize - Header->OffsetToData;
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                          _T("%hs (%hs:%i) allocated %i bytes\n"),
                          Header->Function,
                          Header->File,
                          Header->Line,
                          BytesAllocated);
            Entry = YoriLibGetNextListEntry(&YoriLibSpecialHeap.ActiveAllocationsList, Entry);
        }

        DebugBreak();
    }
#endif
}


/**
 A structure that preceeds a reference counted malloc allocation.
 */
typedef struct _YORILIB_REFERENCED_MALLOC_HEADER {

    /**
     The number of references on the allocation.
     */
    ULONG ReferenceCount;
} YORILIB_REFERENCED_MALLOC_HEADER, *PYORILIB_REFERENCED_MALLOC_HEADER;

#if !YORI_SPECIAL_HEAP
/**
 Allocate a block of memory that can be reference counted and will be freed
 on final dereference.

 @param Bytes The number of bytes to allocate.

 @return Pointer to the allocated block of memory, or NULL on failure.
 */
PVOID
YoriLibReferencedMalloc(
    __in YORI_ALLOC_SIZE_T Bytes
    )
{
    PYORILIB_REFERENCED_MALLOC_HEADER Header;

    Header = YoriLibMalloc(Bytes + sizeof(YORILIB_REFERENCED_MALLOC_HEADER));
    if (Header == NULL) {
        return NULL;
    }

    Header->ReferenceCount = 1;

    return (PVOID)(Header + 1);
}
#else
/**
 Allocate a block of memory that can be reference counted and will be freed
 on final dereference.

 @param Bytes The number of bytes to allocate.

 @param Function Pointer to a constant string indicating the function that is
        allocating the memory.

 @param File Pointer to a constant string indicating the source file that is
        allocating the memory.

 @param Line Specifies the line number within the source file that is
        allocating the memory.

 @return Pointer to the allocated block of memory, or NULL on failure.
 */
PVOID
YoriLibReferencedMallocSpecialHeap(
    __in YORI_ALLOC_SIZE_T Bytes,
    __in LPCSTR Function,
    __in LPCSTR File,
    __in DWORD Line
    )
{
    PYORILIB_REFERENCED_MALLOC_HEADER Header;

    Header = YoriLibMallocSpecialHeap(Bytes + sizeof(YORILIB_REFERENCED_MALLOC_HEADER), Function, File, Line);
    if (Header == NULL) {
        return NULL;
    }

    Header->ReferenceCount = 1;

    return (PVOID)(Header + 1);
}
#endif

/**
 Reference a previously allocated block of reference counted memory.

 @param Allocation The allocation to add a reference to.
 */
VOID
YoriLibReference(
    __in PVOID Allocation
    )
{
    PYORILIB_REFERENCED_MALLOC_HEADER Header;

    Header = (PYORILIB_REFERENCED_MALLOC_HEADER)Allocation - 1;
    Header->ReferenceCount++;
}

/**
 Dereference a previously allocated block of reference counted memory.  If
 this is the final reference, the block is deallocated.

 @param Allocation The allocation to remove a reference from.
 */
VOID
YoriLibDereference(
    __in PVOID Allocation
    )
{
    PYORILIB_REFERENCED_MALLOC_HEADER Header;

    Header = (PYORILIB_REFERENCED_MALLOC_HEADER)Allocation - 1;
    Header->ReferenceCount--;
    if (Header->ReferenceCount == 0) {
        YoriLibFree(Header);
    }
}

/**
 Determine if the specified size is allocatable.  If the specified size
 exceeds an implementation or specified limit, the size cannot be allowed.

 @param Size The proposed allocation size.

 @return TRUE to indicate the new size is acceptable, FALSE if it is not.
 */
BOOLEAN
YoriLibIsSizeAllocatable(
    __in YORI_MAX_UNSIGNED_T Size
    )
{
    YORI_MAX_UNSIGNED_T AllocMask;
    YORI_ALLOC_SIZE_T MaxAlloc;

    MaxAlloc = (YORI_ALLOC_SIZE_T)-1;
    AllocMask = MaxAlloc;
    AllocMask = ~AllocMask;

    if (Size & AllocMask) {
        return FALSE;
    }

    return TRUE;
}

/**
 Determine if any value within the specified range is a valid allocation, and
 if so, return the largest possible size to allocate within the range.  If no
 value is allocatable, return zero.

 @param RequiredSize The minimum size to allocate, where if this is not
        available execution cannot continue.

 @param DesiredSize The maximum size to allocate if it is possible.
 
 @return The number of bytes that can be allocated.
 */
YORI_ALLOC_SIZE_T
YoriLibMaximumAllocationInRange(
    __in YORI_MAX_UNSIGNED_T RequiredSize,
    __in YORI_MAX_UNSIGNED_T DesiredSize
    )
{
    YORI_ALLOC_SIZE_T MaxAlloc;

    ASSERT(DesiredSize >= RequiredSize);

    MaxAlloc = (YORI_ALLOC_SIZE_T)-1;

    if (DesiredSize <= MaxAlloc) {
        return (YORI_ALLOC_SIZE_T)DesiredSize;
    }

    if (RequiredSize > MaxAlloc) {
        return 0;
    }

    return MaxAlloc;
}


/**
 Check if an existing allocation can be extended by the specified number of
 bytes.  This function takes multiple allocatable values and validates they
 can still be allocated if combined.

 @param ExistingSize The existing size of an allocation, which is implicitly
        required.

 @param RequiredExtraSize Any additional size that is required.

 @param DesiredExtraSize Any size which is beneficial but can be truncated
        as needed.

 @return The maximum number of bytes that can be allocated, or zero on
         failure.
 */
YORI_ALLOC_SIZE_T
YoriLibIsAllocationExtendable(
    __in YORI_ALLOC_SIZE_T ExistingSize,
    __in YORI_ALLOC_SIZE_T RequiredExtraSize,
    __in YORI_ALLOC_SIZE_T DesiredExtraSize
    )
{
    YORI_MAX_UNSIGNED_T AllocSize;

    ASSERT(DesiredExtraSize >= RequiredExtraSize);

    AllocSize = ExistingSize;
    return YoriLibMaximumAllocationInRange(AllocSize + RequiredExtraSize, AllocSize + DesiredExtraSize);
}


// vim:sw=4:ts=4:et:
