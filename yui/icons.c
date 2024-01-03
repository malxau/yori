/**
 * @file yui/icons.c
 *
 * Yori shell start menu icon cache
 *
 * Copyright (c) 2023 Malcolm J. Smith
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
#include "yui.h"
#include "resource.h"


/**
 A global structure describing the state of the icon cache.
 */
typedef struct _YUI_ICON_CACHE_STATE {

    /**
     A list of icons known to the icon cache.
     */
    YORI_LIST_ENTRY CachedIcons;

    /**
     The number of times an icon lookup was resolved from the cache.
     */
    DWORD CacheHits;

    /**
     The number of times an icon lookup required a new allocation and icon
     extraction.
     */
    DWORD CacheMisses;

    /**
     The number of times an icon was not in the cache and could not be brought
     into the cache, implying that the specified icon could not be loaded.
     */
    DWORD CacheFailures;
} YUI_ICON_CACHE_STATE, *PYUI_ICON_CACHE_STATE;

/**
 Global state for the icon cache.
 */
YUI_ICON_CACHE_STATE YuiIconCacheState;

/**
 Cleanup state associated with the icon cache module.
 */
VOID
YuiIconCacheCleanupContext(VOID)
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_MENU_SHARED_ICON Icon;

    ListEntry = YoriLibGetNextListEntry(&YuiIconCacheState.CachedIcons, NULL);

    while (ListEntry != NULL) {
        Icon = CONTAINING_RECORD(ListEntry, YUI_MENU_SHARED_ICON, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&YuiIconCacheState.CachedIcons, ListEntry);
        ASSERT(Icon->ReferenceCount == 1);
        YuiIconCacheDereference(Icon);
    }
}

/**
 Initialize the icon cache module.

 @param YuiContext Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YuiIconCacheInitializeContext(
    __in PYUI_CONTEXT YuiContext
    )
{
    UNREFERENCED_PARAMETER(YuiContext);
    YoriLibInitializeListHead(&YuiIconCacheState.CachedIcons);
    return TRUE;
}

/**
 Lookup an icon in the cache.  If it exists, return a referenced pointer to
 it.

 @param FileName Optional pointer to the file name to load the icon from.
        If not specified, the icon is loaded from the running executable.

 @param IconIndex The index of the icon within the file, or the resource
        identifier of the icon within the executable.

 @param LargeIcon TRUE if the large icon should be loaded; FALSE if the small
        icon should be loaded.

 @return On successful completion, a referenced pointer to the icon structure.
         If the entry does not exist in the cache, NULL.
 */
PYUI_MENU_SHARED_ICON
YuiIconCacheLookupAndReference(
    __in_opt PYORI_STRING FileName,
    __in DWORD IconIndex,
    __in BOOLEAN LargeIcon
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_MENU_SHARED_ICON Icon;

    ListEntry = YoriLibGetNextListEntry(&YuiIconCacheState.CachedIcons, NULL);

    while (ListEntry != NULL) {
        Icon = CONTAINING_RECORD(ListEntry, YUI_MENU_SHARED_ICON, ListEntry);
        if (((FileName == NULL && Icon->FileName.StartOfString == NULL) ||
             (FileName != NULL && YoriLibCompareStringInsensitive(FileName, &Icon->FileName) == 0)) &&
            Icon->IconIndex == IconIndex &&
            Icon->LargeIcon == LargeIcon) {

            Icon->ReferenceCount = Icon->ReferenceCount + 1;
            return Icon;
        }
        ListEntry = YoriLibGetNextListEntry(&YuiIconCacheState.CachedIcons, ListEntry);
    }

    return NULL;
}


/**
 Create a single instanced icon structure.

 @param YuiContext Pointer to the application context.

 @param FileName Optional pointer to the file name to load the icon from.
        If not specified, the icon is loaded from the running executable.

 @param IconIndex The index of the icon within the file, or the resource
        identifier of the icon within the executable.

 @param LargeIcon TRUE if the large icon should be loaded; FALSE if the small
        icon should be loaded.

 @return On successful completion, a referenced pointer to the icon structure.
         On failure, NULL.
 */
PYUI_MENU_SHARED_ICON
YuiIconCacheCreateOrReference(
    __in PYUI_CONTEXT YuiContext,
    __in_opt PYORI_STRING FileName,
    __in DWORD IconIndex,
    __in BOOLEAN LargeIcon
    )
{
    PYUI_MENU_SHARED_ICON Icon;
    DWORD IconHeight;
    DWORD IconWidth;
    YORI_ALLOC_SIZE_T FileNameLength;

    Icon = YuiIconCacheLookupAndReference(FileName, IconIndex, LargeIcon);
    if (Icon != NULL) {
        YuiIconCacheState.CacheHits++;
        return Icon;
    }

    FileNameLength = 0;
    if (FileName != NULL) {
        FileNameLength = FileName->LengthInChars + 1;
    }

    Icon = YoriLibReferencedMalloc(sizeof(YUI_MENU_SHARED_ICON) + FileNameLength * sizeof(TCHAR));
    if (Icon == NULL) {
        YuiIconCacheState.CacheFailures++;
        return NULL;
    }

    ZeroMemory(Icon, sizeof(YUI_MENU_SHARED_ICON));

    Icon->ReferenceCount = 1;

    if (LargeIcon) {
        IconWidth = YuiContext->TallIconWidth;
        IconHeight = YuiContext->TallIconHeight;
    } else {
        IconWidth = YuiContext->SmallIconWidth;
        IconHeight = YuiContext->SmallIconHeight;
    }

    //
    //  If FileName is NULL, load an icon from the executable's resource
    //  section.  Otherwise, extract the icon from the file.
    //

    if (FileName == NULL) {

        if (DllUser32.pLoadImageW == NULL) {
            Icon->Icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IconIndex));
        } else {
            Icon->Icon = DllUser32.pLoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCE(IconIndex), IMAGE_ICON, IconWidth, IconHeight, 0);
        }

        if (Icon->Icon == NULL) {
            YoriLibDereference(Icon);
            YuiIconCacheState.CacheFailures++;
            return FALSE;
        }
    } else if (DllShell32.pExtractIconExW != NULL) {

        HICON *LargeIconPtr;
        HICON *SmallIconPtr;

        LargeIconPtr = NULL;
        SmallIconPtr = NULL;
        if (LargeIcon) {
            LargeIconPtr = &Icon->Icon;
        } else {
            SmallIconPtr = &Icon->Icon;
        }
        DllShell32.pExtractIconExW(FileName->StartOfString, IconIndex, LargeIconPtr, SmallIconPtr, 1);
        if (Icon->Icon == NULL) {
            YoriLibDereference(Icon);
            YuiIconCacheState.CacheFailures++;
            return FALSE;
        }
    } else {
        YoriLibDereference(Icon);
        YuiIconCacheState.CacheFailures++;
        return FALSE;
    }

    YuiIconCacheState.CacheMisses++;
    Icon->ReferenceCount = Icon->ReferenceCount + 1;
    if (FileName != NULL) {
        YoriLibReference(Icon);
        Icon->FileName.MemoryToFree = Icon;
        Icon->FileName.StartOfString = (LPWSTR)(Icon + 1);
        Icon->FileName.LengthAllocated = FileName->LengthInChars + 1;
        Icon->FileName.LengthInChars = FileName->LengthInChars;
        memcpy(Icon->FileName.StartOfString, FileName->StartOfString, FileName->LengthInChars * sizeof(TCHAR));
        Icon->FileName.StartOfString[Icon->FileName.LengthInChars] = '\0';
    }
    Icon->IconIndex = IconIndex;
    Icon->LargeIcon = LargeIcon;
    YoriLibAppendList(&YuiIconCacheState.CachedIcons, &Icon->ListEntry);

    return Icon;
}

/**
 Dereference a shared icon structure.  On final dereference, the structure is
 freed.

 @param Icon Pointer to the shared icon structure.
 */
VOID
YuiIconCacheDereference(
    __in PYUI_MENU_SHARED_ICON Icon
    )
{
    ASSERT(Icon->ReferenceCount > 0);
    Icon->ReferenceCount = Icon->ReferenceCount - 1;
    if (Icon->ReferenceCount == 0) {
        if (Icon->ListEntry.Next != NULL) {
            YoriLibRemoveListItem(&Icon->ListEntry);
        }

        if (Icon->Icon != NULL) {
            DestroyIcon(Icon->Icon);
            Icon->Icon = NULL;
        }
        YoriLibFreeStringContents(&Icon->FileName);
        YoriLibDereference(Icon);
    }
}

// vim:sw=4:ts=4:et:
