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
 Context passed when enumerating resources in an executable.  Each found
 resource will have a pointer to this structure.
 */
typedef struct _YUI_EXTRACT_ICON_CONTEXT {

    /**
     The index of the icon within the executable that is being searched for.
     */
    DWORD IndexToFind;

    /**
     The current number if icons that have already been processed.
     */
    DWORD CurrentIndex;

    /**
     Initialized to FALSE, and set to TRUE if the requested icon index is
     found.
     */
    BOOLEAN ResourceFound;

    /**
     For resources identified by integer, contains the resource ID.
     */
    WORD ResourceId;

    /**
     For resources identified by string, contains a dynamically allocated
     string describing the resource name.
     */
    YORI_STRING ResourceName;
} YUI_EXTRACT_ICON_CONTEXT, *PYUI_EXTRACT_ICON_CONTEXT;

/**
 A callback invoked for each icon group resource found within an executable
 file.

 @param Module Pointer to the base of the module in memory.  This is
        ignored in this function because we're only enumerating within a
        single module.

 @param Type Pointer to the type of the resource.  Ignored here since we
        only enumerate icons.

 @param Name Pointer to the resource name, or the MAKEINTRESOURCE integer
        identifier of the resource.

 @param lParam Pointer to a YUI_EXTRACT_ICON_CONTEXT.

 @return TRUE to continue enumerating, or FALSE to stop.
 */
BOOL WINAPI
YuiExtractIconCallback(
    __in HANDLE Module,
    __in LPCTSTR Type,
    __in LPTSTR Name,
    __in LPARAM lParam
    )
{
    PYUI_EXTRACT_ICON_CONTEXT Context;
    YORI_ALLOC_SIZE_T Length;
    WORD ResourceId;

    UNREFERENCED_PARAMETER(Module);
    UNREFERENCED_PARAMETER(Type);

    Context = (PYUI_EXTRACT_ICON_CONTEXT)lParam;

    //
    //  Check if this resource is the index requested.
    //
    if (Context->IndexToFind == Context->CurrentIndex) {

        //
        //  Check if the resource name is an integer or string, and copy the
        //  resource name into the context structure.
        //

        ResourceId = (WORD)(LONG_PTR)Name;
        if (Name == MAKEINTRESOURCE(ResourceId)) {
            Context->ResourceId = ResourceId;
        } else {
            ASSERT(Context->ResourceName.StartOfString == NULL);
            Length = _tcslen(Name);
            if (!YoriLibAllocateString(&Context->ResourceName, Length + 1)) {
                return FALSE;
            }

            memcpy(Context->ResourceName.StartOfString, Name, (Length + 1) * sizeof(TCHAR));
            Context->ResourceName.LengthInChars = Length;
        }
        Context->ResourceFound = TRUE;
        return FALSE;
    }

    Context->CurrentIndex++;
    return TRUE;
}

/**
 Obtain a handle to an icon with arbitrary dimensions.  This is a
 reimplementation of ExtractIconEx, which frustratingly assumes exactly two
 icon sizes.

 @param FileName Pointer to the file name to extract an icon from.

 @param IconIndex The zero based index of the icon within the file to locate.

 @param Width The requested width of the resulting icon.

 @param Height The requested height of the resulting icon.

 @return Handle to the icon, or NULL on failure.
 */
HICON
YuiExtractIcon(
    __in PCYORI_STRING FileName,
    __in DWORD IconIndex,
    __in WORD Width,
    __in WORD Height
    )
{
    HMODULE Module;
    HICON Icon;
    YUI_EXTRACT_ICON_CONTEXT Context;
    LPTSTR NameToRequest;
    LPTSTR Extension;

    ASSERT(YoriLibIsStringNullTerminated(FileName));

    if (DllUser32.pLoadImageW == NULL) {
        return NULL;
    }

    //
    //  If the file name ends in .ico, there are no resources to parse.
    //  Luckily LoadImage can handle these formats, find the "best" source
    //  size and perform best effort scaling.
    //

    Extension = YoriLibFindRightMostCharacter(FileName, '.');
    if (Extension != NULL) {
        YORI_STRING Substr;
        YoriLibInitEmptyString(&Substr);
        Substr.StartOfString = Extension + 1;
        Substr.LengthInChars = FileName->LengthInChars - (YORI_ALLOC_SIZE_T)(Extension - FileName->StartOfString) - 1;
        if (YoriLibCompareStringWithLiteralInsensitive(&Substr, _T("ico")) == 0) {
            Icon = DllUser32.pLoadImageW(NULL, FileName->StartOfString, IMAGE_ICON, Width, Height, LR_LOADFROMFILE);
            return Icon;
        }
    }

    //
    //  Otherwise, try to load the file as an EXE/DLL.
    //

    Module = LoadLibraryEx(FileName->StartOfString, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (Module == NULL) {
        return NULL;
    }

    Context.IndexToFind = IconIndex;
    Context.CurrentIndex = 0;
    Context.ResourceFound = FALSE;
    Context.ResourceId = 0;
    YoriLibInitEmptyString(&Context.ResourceName);

    //
    //  For compatibility with ExtractIconEx, if the index is negative, it
    //  refers to a resource identifier, so we don't need to scan the
    //  resource section looking it up.  If it's positive, it's an offset, so
    //  enumerate resources to find the name or ID of the corresponding
    //  offset.
    //

    if ((INT)IconIndex < 0) {
        Context.ResourceFound = TRUE;
        Context.ResourceId = (WORD)(-1 * IconIndex);
    } else {
        EnumResourceNames(Module, RT_GROUP_ICON, YuiExtractIconCallback, (LONG_PTR)&Context);

        if (!Context.ResourceFound) {
            FreeLibrary(Module);
            return NULL;
        }
    }

    //
    //  Try to load the requested resource by name or identifier, with a
    //  requested icon size.
    //

    NameToRequest = MAKEINTRESOURCE(Context.ResourceId);
    if (Context.ResourceName.StartOfString != NULL) {
        NameToRequest = Context.ResourceName.StartOfString;
    }

    Icon = DllUser32.pLoadImageW(Module, NameToRequest, IMAGE_ICON, Width, Height, 0);
    YoriLibFreeStringContents(&Context.ResourceName);
    FreeLibrary(Module);

    return Icon;
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
        IconWidth = YuiContext->SmallStartIconWidth;
        IconHeight = YuiContext->SmallStartIconHeight;
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
    } else {
        WORD Width;
        WORD Height;
        if (LargeIcon) {
            Width = YuiContext->TallIconWidth;
            Height = YuiContext->TallIconHeight;
        } else {
            Width = YuiContext->SmallStartIconWidth;
            Height = YuiContext->SmallStartIconHeight;
        }
        Icon->Icon = YuiExtractIcon(FileName, IconIndex, Width, Height);
        if (Icon->Icon == NULL) {
            YoriLibDereference(Icon);
            YuiIconCacheState.CacheFailures++;
            return FALSE;
        }
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
