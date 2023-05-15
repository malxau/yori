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
 Create a single instanced icon structure.

 @param YuiContext Pointer to the application context.

 @param FileName Optional pointer to the file name to load the icon from.
        If not specified, the icon is loaded from the running executable.

 @param IconIndex The index of the icon within the file, or the resource
        identifier of the icon within the executable.

 @param LargeIcon TRUE if the large (32x32) icon should be loaded; FALSE if
        the small (20x20) icon should be loaded.

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

    if (FileName != NULL) {
        return NULL;
    }

    Icon = YoriLibReferencedMalloc(sizeof(YUI_MENU_SHARED_ICON));
    if (Icon == NULL) {
        return NULL;
    }

    ZeroMemory(Icon, sizeof(YUI_MENU_SHARED_ICON));

    Icon->ReferenceCount = 1;
    if (LargeIcon || DllUser32.pLoadImageW == NULL) {
        Icon->Icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IconIndex));
    } else {
        Icon->Icon = DllUser32.pLoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCE(IconIndex), IMAGE_ICON, YuiContext->SmallIconWidth, YuiContext->SmallIconHeight, 0);
    }

    if (Icon->Icon == NULL) {
        YoriLibDereference(Icon);
        return FALSE;
    }

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
    ASSERT(Icon->ReferenceCount == 1);
    if (Icon->Icon != NULL) {
        DestroyIcon(Icon->Icon);
        Icon->Icon = NULL;
    }
    YoriLibDereference(Icon);
}

/**
 Cleanup state associated with the icon cache module.
 */
VOID
YuiIconCacheCleanupContext(VOID)
{
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
    return TRUE;
}

// vim:sw=4:ts=4:et:
