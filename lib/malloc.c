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
    DWORD OffsetToData;

    /**
     Number of pages in the allocation.  Special heap allocations are
     always a multiple of pages.
     */
    DWORD PagesInAllocation;

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
 The number of bytes in a page on this architecture.
 */
#define PAGE_SIZE (0x1000)

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
    __in DWORD Bytes
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
    __in DWORD Bytes,
    __in LPCSTR Function,
    __in LPCSTR File,
    __in DWORD Line
    )
{
    DWORD TotalPagesNeeded;
    PYORI_SPECIAL_HEAP_HEADER Header;
    PYORI_SPECIAL_HEAP_HEADER Commit;
    DWORD StackSize;
    DWORD OldAccess;
#if defined(_M_MRX000) || defined(_M_ARM64)
    DWORD Alignment = sizeof(DWORD);
#else
    DWORD Alignment = sizeof(UCHAR);
#endif

    StackSize = 0;
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        StackSize = sizeof(PVOID) * YORI_SPECIAL_HEAP_STACK_FRAMES;
    }

    TotalPagesNeeded = (Bytes + sizeof(YORI_SPECIAL_HEAP_HEADER) + StackSize + 2 * PAGE_SIZE - 1) / PAGE_SIZE;

    if (YoriLibSpecialHeap.Mutex == NULL) {
        YoriLibSpecialHeap.Mutex = CreateMutex(NULL, FALSE, NULL);
    }

    if (YoriLibSpecialHeap.Mutex == NULL) {
        return NULL;
    }

    if (YoriLibSpecialHeap.ActiveAllocationsList.Next == NULL) {
        YoriLibInitializeListHead(&YoriLibSpecialHeap.ActiveAllocationsList);
    }

    Header = VirtualAlloc(NULL, TotalPagesNeeded * PAGE_SIZE, MEM_RESERVE, PAGE_READWRITE);
    if (Header == NULL) {
        return NULL;
    }

    Commit = VirtualAlloc(Header, TotalPagesNeeded * PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
    if (Commit == NULL) {
        VirtualFree(Header, 0, MEM_RELEASE);
        return NULL;
    }

    if (!VirtualProtect((PUCHAR)Header + (TotalPagesNeeded - 1) * PAGE_SIZE, PAGE_SIZE, PAGE_NOACCESS, &OldAccess)) {
        VirtualFree(Header, TotalPagesNeeded * PAGE_SIZE, MEM_DECOMMIT);
        VirtualFree(Header, 0, MEM_RELEASE);
        return NULL;
    }

    FillMemory(Header, (TotalPagesNeeded - 1) * PAGE_SIZE, '@');

    Header->PagesInAllocation = TotalPagesNeeded;
    Header->OffsetToData = ((TotalPagesNeeded - 1) * PAGE_SIZE - Bytes) & ~(Alignment - 1);
    Header->Function = Function;
    Header->File = File;
    Header->Line = Line;
    ASSERT(Header->OffsetToData < (PAGE_SIZE + sizeof(YORI_SPECIAL_HEAP_HEADER) + StackSize));
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        DllKernel32.pRtlCaptureStackBackTrace(1, YORI_SPECIAL_HEAP_STACK_FRAMES, YoriLibAddToPointer(Header, sizeof(YORI_SPECIAL_HEAP_HEADER)), NULL);
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
    DWORD BytesToFree;
    DWORD StackSize;

    StackSize = 0;
    if (DllKernel32.pRtlCaptureStackBackTrace != NULL) {
        StackSize = sizeof(PVOID) * YORI_SPECIAL_HEAP_STACK_FRAMES;
    }

    Header = YoriLibSubtractFromPointer(Ptr, sizeof(YORI_SPECIAL_HEAP_HEADER) + StackSize);
    Header = (PYORI_SPECIAL_HEAP_HEADER)((DWORD_PTR)Header & ~(PAGE_SIZE - 1));

    TestChar = (PUCHAR)Header;
    ASSERT(TestChar + Header->OffsetToData == Ptr);

    TestChar = (PUCHAR)YoriLibAddToPointer(Header, sizeof(YORI_SPECIAL_HEAP_HEADER) + StackSize);
    while (TestChar < (PUCHAR)Ptr) {
        ASSERT(*TestChar == '@');
        TestChar++;
    }

    BytesToFree = (Header->PagesInAllocation - 1) * PAGE_SIZE - Header->OffsetToData;

    WaitForSingleObject(YoriLibSpecialHeap.Mutex, INFINITE);

    MyEntry = YoriLibSpecialHeap.NumberFreed % (sizeof(YoriLibSpecialHeap.RecentlyFreed)/sizeof(YoriLibSpecialHeap.RecentlyFreed[0]));
    YoriLibSpecialHeap.NumberFreed++;
    YoriLibSpecialHeap.BytesCurrentlyAllocated -= BytesToFree;
    YoriLibRemoveListItem(&Header->ListEntry);

    if (YoriLibSpecialHeap.RecentlyFreed[MyEntry] != NULL) {

        PYORI_SPECIAL_HEAP_HEADER OldHeader;

        OldHeader = YoriLibSpecialHeap.RecentlyFreed[MyEntry];

        if (!VirtualProtect(OldHeader, PAGE_SIZE, PAGE_READWRITE, &OldAccess)) {
            ASSERT(!"VirtualProtect failure");
        }
        if (!VirtualFree(OldHeader, OldHeader->PagesInAllocation * PAGE_SIZE, MEM_DECOMMIT)) {
            ASSERT(!"VirtualFree failure");
        }
        if (!VirtualFree(OldHeader, 0, MEM_RELEASE)) {
            ASSERT(!"VirtualFree failure");
        }
    }

    if (!VirtualProtect(Header, Header->PagesInAllocation * PAGE_SIZE, PAGE_NOACCESS, &OldAccess)) {
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
    if (YoriLibSpecialHeap.BytesCurrentlyAllocated > 0 ||
        (YoriLibSpecialHeap.NumberAllocated - YoriLibSpecialHeap.NumberFreed > 0)) {

        PYORI_LIST_ENTRY Entry;
        PYORI_SPECIAL_HEAP_HEADER Header;
        DWORD BytesAllocated;

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%i bytes allocated in %i allocations\n"), YoriLibSpecialHeap.BytesCurrentlyAllocated, YoriLibSpecialHeap.NumberAllocated - YoriLibSpecialHeap.NumberFreed);

        Entry = YoriLibGetNextListEntry(&YoriLibSpecialHeap.ActiveAllocationsList, NULL);
        while (Entry != NULL) {
            Header = CONTAINING_RECORD(Entry, YORI_SPECIAL_HEAP_HEADER, ListEntry);
            BytesAllocated = (Header->PagesInAllocation - 1) * PAGE_SIZE - Header->OffsetToData;
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs (%hs:%i) allocated %i bytes\n"), Header->Function, Header->File, Header->Line, BytesAllocated);
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
    __in DWORD Bytes
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
    __in DWORD Bytes,
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


// vim:sw=4:ts=4:et:
