/**
 * @file yui/menu.c
 *
 * Yori shell populate and display the start menu
 *
 * Copyright (c) 2019-2023 Malcolm J. Smith
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
 A structure describing a directory within the start menu.
 */
typedef struct _YUI_MENU_DIRECTORY {

    /**
     The entry for this directory within the children of its parent.  This is
     associated with ChildDirectories, below.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     Information about the item to draw in the menu, including its display
     name and optional icon.
     */
    YUI_MENU_OWNERDRAW_ITEM Item;

    /**
     A list of child directories within this directory.  This is paired with
     ListEntry above.
     */
    YORI_LIST_ENTRY ChildDirectories;

    /**
     A list of child files (launchable applications) underneath this
     directory.  This is paired with @ref YUI_MENU_FILE::ListEntry .
     */
    YORI_LIST_ENTRY ChildFiles;

    /**
     A handle to the menu that contains subdirectories and files within this
     directory.
     */
    HMENU MenuHandle;

    /**
     The depth of this directory.  The root is zero, and all subitems start
     from 1.
     */
    DWORD Depth;
} YUI_MENU_DIRECTORY, *PYUI_MENU_DIRECTORY;

/**
 A structure describing a launchable program within the start menu.
 */
typedef struct _YUI_MENU_FILE {

    /**
     The list linkage associating this program with its parent directory.
     This is paired with @ref YUI_MENU_DIRECTORY::ChildFiles .
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     Information about the item to draw in the menu, including its display
     name and optional icon.
     */
    YUI_MENU_OWNERDRAW_ITEM Item;

    /**
     A fully qualified path to this file (typically a .lnk file.)
     */
    YORI_STRING FilePath;

    /**
     The depth of this entry.  All objects underneath the root start at 1.
     */
    DWORD Depth;

    /**
     The unique identifier for this menu item.
     */
    DWORD MenuId;
} YUI_MENU_FILE, *PYUI_MENU_FILE;


/**
 A context structure for the menu module.
 */
typedef struct _YUI_MENU_CONTEXT {

    /**
     The directory object corresponding to the top level start menu directory.
     */
    YUI_MENU_DIRECTORY StartDirectory;

    /**
     The directory object corresponding to the programs directory.
     */
    YUI_MENU_DIRECTORY ProgramsDirectory;

    /**
     Owner draw state for the Programs menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM Programs;

#if DBG
    /**
     Owner draw state for the Debug menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM Debug;

    /**
     Owner draw state for the Debug refresh taskbar menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM DebugRefreshTaskbar;

    /**
     Owner draw state for the Debug toggle debug logging menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM DebugToggleLogging;

    /**
     Owner draw state for the Debug launch winlogon shell menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM DebugLaunchWinlogonShell;

    /**
     Owner draw state for the Debug simulate display change menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM DebugSimulateDisplayChange;
#endif

    /**
     Owner draw state for the Run menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM Run;

    /**
     Owner draw state for the Exit menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM Exit;

    /**
     Owner draw state for the Shutdown menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM Shutdown;

    /**
     Owner draw state for the Shutdown Disconnect menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ShutdownDisconnect;

    /**
     Owner draw state for the Shutdown Lock menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ShutdownLock;

    /**
     Owner draw state for the Shutdown Exit menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ShutdownExit;

    /**
     Owner draw state for the Shutdown Log off menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ShutdownLogoff;

    /**
     Owner draw state for the Shutdown Reboot menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ShutdownReboot;

    /**
     Owner draw state for the Shutdown Shut down menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ShutdownShutdown;
} YUI_MENU_CONTEXT, *PYUI_MENU_CONTEXT;

/**
 Global state for the menu module.
 */
YUI_MENU_CONTEXT YuiMenuContext;

/**
 Initialize an empty ownerdraw menu item.

 @param Item Pointer to the item to initialize.
 */
VOID
YuiMenuInitializeItem(
    __out PYUI_MENU_OWNERDRAW_ITEM Item
    )
{
    Item->Icon = NULL;
    YoriLibInitEmptyString(&Item->Text);
    Item->TallItem = FALSE;
}

/**
 Cleanup an ownerdraw menu item.

 @param Item Pointer to the item to clean up.
 */
VOID
YuiMenuCleanupItem(
    __in PYUI_MENU_OWNERDRAW_ITEM Item
    )
{
    if (Item->Icon != NULL) {
        YuiIconCacheDereference(Item->Icon);
    }
    YoriLibFreeStringContents(&Item->Text);
}

/**
 Cleanup state associated with the menu module.
 */
VOID
YuiMenuCleanupContext(VOID)
{
    YuiMenuCleanupItem(&YuiMenuContext.Programs);

#if DBG
    YuiMenuCleanupItem(&YuiMenuContext.Debug);
    YuiMenuCleanupItem(&YuiMenuContext.DebugRefreshTaskbar);
    YuiMenuCleanupItem(&YuiMenuContext.DebugToggleLogging);
    YuiMenuCleanupItem(&YuiMenuContext.DebugLaunchWinlogonShell);
    YuiMenuCleanupItem(&YuiMenuContext.DebugSimulateDisplayChange);
#endif

    YuiMenuCleanupItem(&YuiMenuContext.Run);
    YuiMenuCleanupItem(&YuiMenuContext.Exit);
    YuiMenuCleanupItem(&YuiMenuContext.Shutdown);
    YuiMenuCleanupItem(&YuiMenuContext.ShutdownDisconnect);
    YuiMenuCleanupItem(&YuiMenuContext.ShutdownLock);
    YuiMenuCleanupItem(&YuiMenuContext.ShutdownExit);
    YuiMenuCleanupItem(&YuiMenuContext.ShutdownLogoff);
    YuiMenuCleanupItem(&YuiMenuContext.ShutdownReboot);
    YuiMenuCleanupItem(&YuiMenuContext.ShutdownShutdown);

    YuiMenuCleanupItem(&YuiMenuContext.ProgramsDirectory.Item);
    YuiMenuCleanupItem(&YuiMenuContext.StartDirectory.Item);
}

/**
 Initialize the menu module.

 @param YuiContext Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YuiMenuInitializeContext(
    __in PYUI_CONTEXT YuiContext
    )
{
    YoriLibInitializeListHead(&YuiMenuContext.ProgramsDirectory.ListEntry);
    YoriLibInitializeListHead(&YuiMenuContext.ProgramsDirectory.ChildDirectories);
    YoriLibInitializeListHead(&YuiMenuContext.ProgramsDirectory.ChildFiles);
    YuiMenuInitializeItem(&YuiMenuContext.ProgramsDirectory.Item);
    YoriLibInitializeListHead(&YuiMenuContext.StartDirectory.ListEntry);
    YoriLibInitializeListHead(&YuiMenuContext.StartDirectory.ChildDirectories);
    YoriLibInitializeListHead(&YuiMenuContext.StartDirectory.ChildFiles);
    YuiMenuInitializeItem(&YuiMenuContext.StartDirectory.Item);

    YuiMenuContext.Programs.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, PROGRAMSICON, TRUE);
    YoriLibConstantString(&YuiMenuContext.Programs.Text, _T("Programs"));
    YuiMenuContext.Programs.TallItem = TRUE;

#if DBG
    YuiMenuContext.Debug.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, DEBUGICON, TRUE);
    YoriLibConstantString(&YuiMenuContext.Debug.Text, _T("Debug"));
    YuiMenuContext.Debug.TallItem = TRUE;

    YuiMenuInitializeItem(&YuiMenuContext.DebugRefreshTaskbar);
    YoriLibConstantString(&YuiMenuContext.DebugRefreshTaskbar.Text, _T("Refresh Taskbar"));
    YuiMenuInitializeItem(&YuiMenuContext.DebugToggleLogging);
    YoriLibConstantString(&YuiMenuContext.DebugToggleLogging.Text, _T("Debug logging"));

    YuiMenuInitializeItem(&YuiMenuContext.DebugLaunchWinlogonShell);
    YoriLibConstantString(&YuiMenuContext.DebugLaunchWinlogonShell.Text, _T("Launch Winlogon shell and exit"));

    YuiMenuInitializeItem(&YuiMenuContext.DebugSimulateDisplayChange);
    YoriLibConstantString(&YuiMenuContext.DebugSimulateDisplayChange.Text, _T("Simulate display change"));
#endif

    YuiMenuContext.Run.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, RUNICON, TRUE);
    YoriLibConstantString(&YuiMenuContext.Run.Text, _T("Run..."));
    YuiMenuContext.Run.TallItem = TRUE;

    if (YuiContext->LoginShell) {
        YuiMenuContext.Exit.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, LOGOFFICON, TRUE);
        YoriLibConstantString(&YuiMenuContext.Exit.Text, _T("Log off"));
    } else {
        YuiMenuContext.Exit.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, EXITICON, TRUE);
        YoriLibConstantString(&YuiMenuContext.Exit.Text, _T("Exit"));
    }
    YuiMenuContext.Exit.TallItem = TRUE;

    YuiMenuInitializeItem(&YuiMenuContext.Shutdown);
    YuiMenuContext.Shutdown.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, SHUTDOWNICON, TRUE);
    YoriLibConstantString(&YuiMenuContext.Shutdown.Text, _T("Shutdown"));
    YuiMenuContext.Shutdown.TallItem = TRUE;

    YuiMenuInitializeItem(&YuiMenuContext.ShutdownDisconnect);
    YoriLibConstantString(&YuiMenuContext.ShutdownDisconnect.Text, _T("Disconnect"));

    YuiMenuInitializeItem(&YuiMenuContext.ShutdownLock);
    YoriLibConstantString(&YuiMenuContext.ShutdownLock.Text, _T("Lock"));

    YuiMenuInitializeItem(&YuiMenuContext.ShutdownExit);
    YoriLibConstantString(&YuiMenuContext.ShutdownExit.Text, _T("Exit"));

    YuiMenuInitializeItem(&YuiMenuContext.ShutdownLogoff);
    YoriLibConstantString(&YuiMenuContext.ShutdownLogoff.Text, _T("Log off"));

    YuiMenuInitializeItem(&YuiMenuContext.ShutdownReboot);
    YoriLibConstantString(&YuiMenuContext.ShutdownReboot.Text, _T("Reboot"));

    YuiMenuInitializeItem(&YuiMenuContext.ShutdownShutdown);
    YoriLibConstantString(&YuiMenuContext.ShutdownShutdown.Text, _T("Shut down"));

    return TRUE;
}



#if DBG
/**
 Turn the console logging window on or off.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiMenuToggleLogging(
    __inout PYUI_CONTEXT YuiContext
    )
{
    if (YuiContext->DebugLogEnabled) {
        if (FreeConsole()) {
            YuiContext->DebugLogEnabled = FALSE;
        }
    } else {
        if (AllocConsole()) {
            SetConsoleTitle(_T("Yui debug log"));
            YuiContext->DebugLogEnabled = TRUE;
        }
    }
}

/**
 Indicate that Yui should launch the current winlogon shell.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiExitAndLaunchWinlogonShell(
    __inout PYUI_CONTEXT YuiContext
    )
{
    YuiContext->LaunchWinlogonShell = TRUE;
    PostQuitMessage(0);
}

#endif

/**
 Given a fully qualified directory name, return the substring from the name
 that corresponds to the name of a path at a specified depth level.

 @param DirName Pointer to the fully qualified directory name.

 @param Component On successful completion, populated with the substring that
        corresponds to the name at a particular depth.

 @param Depth The depth level to return the substring for.  A depth of 1
        indicates the file name.  A depth of 2 indicates the parent directory,
        etc.

 @param RemoveExtension If TRUE, the returned component should not include any
        file extension.  If FALSE, the entire name is returned.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YuiFindDepthComponent(
    __in PYORI_STRING DirName,
    __out PYORI_STRING Component,
    __in DWORD Depth,
    __in BOOLEAN RemoveExtension
    )
{
    YORI_STRING Current;
    LPTSTR PreviousSep = NULL;
    LPTSTR Sep;
    DWORD Count;

    YoriLibInitEmptyString(&Current);
    Current.StartOfString = DirName->StartOfString;
    Current.LengthInChars = DirName->LengthInChars;

    //
    //  Note this is beyond the end of the allocation and is done purely for
    //  accounting.  It should not be dereferenced.
    //

    PreviousSep = &DirName->StartOfString[DirName->LengthInChars];
    Sep = YoriLibFindRightMostCharacter(&Current, '\\');
    ASSERT(Sep != NULL);
    if (Sep == NULL) {
        return FALSE;
    }

    for (Count = 0; Count < Depth; Count++) {
        PreviousSep = Sep;
        Current.LengthInChars = (DWORD)(Sep - Current.StartOfString) - 1;
        Sep = YoriLibFindRightMostCharacter(&Current, '\\');
        ASSERT(Sep != NULL);
        if (Sep == NULL) {
            return FALSE;
        }
    }

    YoriLibInitEmptyString(Component);
    Component->StartOfString = Sep + 1;
    Component->LengthInChars = (DWORD)(PreviousSep - Sep) - 1;
    Component->LengthAllocated = Component->LengthInChars;
    Component->MemoryToFree = NULL;

    if (RemoveExtension) {
        Sep = YoriLibFindRightMostCharacter(Component, '.');
        if (Sep != NULL) {
            Component->LengthInChars = (DWORD)(Sep - Component->StartOfString);
        }
    }

    return TRUE;
}

/**
 Allocate and initialize a new directory object within the start menu.

 @param YuiContext Pointer to the application context.

 @param DirName Pointer to the human readable name for the directory.

 @return A pointer to the directory object, or NULL on failure.
 */
PYUI_MENU_DIRECTORY
YuiCreateMenuDirectory(
    __in PYUI_CONTEXT YuiContext,
    __in PYORI_STRING DirName
    )
{
    PYUI_MENU_DIRECTORY Entry;

    Entry = YoriLibReferencedMalloc(sizeof(YUI_MENU_DIRECTORY) + (DirName->LengthInChars + 1) * sizeof(TCHAR));
    if (Entry == NULL) {
        return NULL;
    }

    YoriLibReference(Entry);

    YoriLibInitializeListHead(&Entry->ListEntry);
    YoriLibInitializeListHead(&Entry->ChildDirectories);
    YoriLibInitializeListHead(&Entry->ChildFiles);
    YuiMenuInitializeItem(&Entry->Item);
    Entry->Item.Text.StartOfString = (LPTSTR)(Entry + 1);
    Entry->Item.Text.LengthAllocated = DirName->LengthInChars + 1;
    Entry->Item.Text.LengthInChars = DirName->LengthInChars;
    Entry->Item.Text.MemoryToFree = Entry;

    memcpy(Entry->Item.Text.StartOfString, DirName->StartOfString, DirName->LengthInChars * sizeof(TCHAR));
    Entry->Item.Text.StartOfString[Entry->Item.Text.LengthInChars] = '\0';

    Entry->Item.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, PROGRAMSICON, Entry->Item.TallItem);

    Entry->MenuHandle = NULL;

    return Entry;
}

/**
 Free the memory associated with a directory within the start menu.

 @param Directory Pointer to the object to deallocate.

 @param Context Ignored in this function.

 @return TRUE to continue to the next object.
 */
BOOL
YuiDeleteMenuDirectory(
    __in PYUI_MENU_DIRECTORY Directory,
    __in PVOID Context
    )
{
    PYUI_MENU_DIRECTORY Root;
    Root = (PYUI_MENU_DIRECTORY)Context;

    ASSERT(YoriLibIsListEmpty(&Directory->ChildFiles));
    ASSERT(YoriLibIsListEmpty(&Directory->ChildDirectories));

    if (Directory->MenuHandle != NULL) {
        DestroyMenu(Directory->MenuHandle);
        Directory->MenuHandle = NULL;
    }

    if (Directory != Root) {
        YoriLibRemoveListItem(&Directory->ListEntry);

        YuiMenuCleanupItem(&Directory->Item);
        YoriLibDereference(Directory);
    }
    return TRUE;
}

/**
 Allocate and initialize a new file object within the start menu.

 @param YuiContext Pointer to the application context.

 @param FilePath Pointer to the full path to a shortcut file for this entry.

 @param FriendlyName Pointer to the human readable name for the file.

 @param TallItem TRUE if the item should be a full height item, FALSE if the
        item should be a short item.  This determines which icon to load.

 @return A pointer to the directory object, or NULL on failure.
 */
PYUI_MENU_FILE
YuiCreateMenuFile(
    __in PYUI_CONTEXT YuiContext,
    __in PYORI_STRING FilePath,
    __in PYORI_STRING FriendlyName,
    __in BOOLEAN TallItem
    )
{
    PYUI_MENU_FILE Entry;
    YORI_STRING IconPath;
    DWORD IconIndex;

    Entry = YoriLibReferencedMalloc(sizeof(YUI_MENU_FILE) + (FilePath->LengthInChars + 1 + FriendlyName->LengthInChars + 1) * sizeof(TCHAR));
    if (Entry == NULL) {
        return NULL;
    }

    YoriLibReference(Entry);
    YoriLibReference(Entry);

    YoriLibInitializeListHead(&Entry->ListEntry);
    YoriLibInitEmptyString(&Entry->FilePath);
    YuiMenuInitializeItem(&Entry->Item);

    Entry->FilePath.StartOfString = (LPTSTR)(Entry + 1);
    Entry->FilePath.LengthAllocated = FilePath->LengthInChars + 1;
    Entry->FilePath.LengthInChars = FilePath->LengthInChars;
    Entry->FilePath.MemoryToFree = Entry;
    memcpy(Entry->FilePath.StartOfString, FilePath->StartOfString, FilePath->LengthInChars * sizeof(TCHAR));
    Entry->FilePath.StartOfString[Entry->FilePath.LengthInChars] = '\0';

    Entry->Item.TallItem = TallItem;
    Entry->Item.Text.StartOfString = Entry->FilePath.StartOfString + Entry->FilePath.LengthAllocated;
    Entry->Item.Text.LengthAllocated = FriendlyName->LengthInChars + 1;
    Entry->Item.Text.LengthInChars = FriendlyName->LengthInChars;
    Entry->Item.Text.MemoryToFree = Entry;
    memcpy(Entry->Item.Text.StartOfString, FriendlyName->StartOfString, FriendlyName->LengthInChars * sizeof(TCHAR));
    Entry->Item.Text.StartOfString[Entry->Item.Text.LengthInChars] = '\0';

    if (YoriLibLoadShortcutIconPath(FilePath, &IconPath, &IconIndex)) {
        YORI_STRING Ext;

        YoriLibInitEmptyString(&Ext);

        Ext.StartOfString = YoriLibFindRightMostCharacter(&IconPath, '.');
        if (Ext.StartOfString != NULL) {
            Ext.LengthInChars = IconPath.LengthInChars - (DWORD)(Ext.StartOfString - IconPath.StartOfString);
        }

        if (YoriLibCompareStringWithLiteralInsensitive(&Ext, _T(".exe")) == 0 ||
            YoriLibCompareStringWithLiteralInsensitive(&Ext, _T(".dll")) == 0 ||
            YoriLibCompareStringWithLiteralInsensitive(&Ext, _T(".ico")) == 0) {

            Entry->Item.Icon = YuiIconCacheCreateOrReference(YuiContext, &IconPath, IconIndex, Entry->Item.TallItem);
        }

        YoriLibFreeStringContents(&IconPath);
    }

    return Entry;
}

/**
 Free the memory associated with a shortcut/program within the start menu.

 @param File Pointer to the object to deallocate.

 @param Context Ignored in this function.

 @return TRUE to continue to the next object.
 */
BOOL
YuiDeleteMenuFile(
    __in PYUI_MENU_FILE File,
    __in PVOID Context
    )
{
    UNREFERENCED_PARAMETER(Context);
    YoriLibRemoveListItem(&File->ListEntry);

    YoriLibFreeStringContents(&File->FilePath);
    YuiMenuCleanupItem(&File->Item);
    YoriLibDereference(File);
    return TRUE;
}

/**
 Check whether a directory name exists within a parent.  This can occur
 because the start menu is a merged view of system and user menus, so the
 same directory name can exist on disk twice, and the intention is to
 display the results in a single start menu view.

 @param Parent Pointer to the parent directory.

 @param ChildName Pointer to the child name to check whether it exists as a
        direct descendant of the parent or not.

 @return TRUE if the child exists as a direct descendant of the parent, FALSE
         if it does not.
 */
BOOL
YuiDirectoryNodeExists(
    __in PYUI_MENU_DIRECTORY Parent,
    __in PYORI_STRING ChildName
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_MENU_DIRECTORY Existing;
    int CompareResult;

    ListEntry = NULL;
    ListEntry = YoriLibGetPreviousListEntry(&Parent->ChildDirectories, ListEntry);
    while (ListEntry != NULL) {
        Existing = CONTAINING_RECORD(ListEntry, YUI_MENU_DIRECTORY, ListEntry);
        CompareResult = YoriLibCompareStringInsensitive(&Existing->Item.Text, ChildName);
        if (CompareResult == 0) {
            return TRUE;
        }

        ListEntry = YoriLibGetPreviousListEntry(&Parent->ChildDirectories, ListEntry);
    }
    return FALSE;
}

/**
 Insert a newly found directory into an existing start menu directory,
 maintaining sort order.

 @param Parent Pointer to a directory to insert the directory into.

 @param Child Pointer to the item to insert.
 */
VOID
YuiInsertDirectoryInOrder(
    __in PYUI_MENU_DIRECTORY Parent,
    __in PYUI_MENU_DIRECTORY Child
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_MENU_DIRECTORY Existing;
    int CompareResult;

    ListEntry = NULL;
    ListEntry = YoriLibGetPreviousListEntry(&Parent->ChildDirectories, ListEntry);
    while (ListEntry != NULL) {
        Existing = CONTAINING_RECORD(ListEntry, YUI_MENU_DIRECTORY, ListEntry);
        CompareResult = YoriLibCompareStringInsensitive(&Existing->Item.Text, &Child->Item.Text);
        if (CompareResult < 0) {
            break;
        }

        ListEntry = YoriLibGetPreviousListEntry(&Parent->ChildDirectories, ListEntry);
    }

    if (ListEntry == NULL) {
        YoriLibInsertList(&Parent->ChildDirectories, &Child->ListEntry);
    } else {
        YoriLibInsertList(ListEntry, &Child->ListEntry);
    }
}

/**
 Insert a newly found file (link to program) into a start menu directory,
 maintaining sort order.

 @param Parent Pointer to a directory to insert the program into.

 @param Child Pointer to the item to insert.
 */
VOID
YuiInsertFileInOrder(
    __in PYUI_MENU_DIRECTORY Parent,
    __in PYUI_MENU_FILE Child
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_MENU_FILE Existing;
    int CompareResult;

    ListEntry = NULL;
    ListEntry = YoriLibGetPreviousListEntry(&Parent->ChildFiles, ListEntry);
    while (ListEntry != NULL) {
        Existing = CONTAINING_RECORD(ListEntry, YUI_MENU_FILE, ListEntry);
        CompareResult = YoriLibCompareStringInsensitive(&Existing->Item.Text, &Child->Item.Text);
        if (CompareResult < 0) {
            break;
        }
        ListEntry = YoriLibGetPreviousListEntry(&Parent->ChildFiles, ListEntry);
    }

    if (ListEntry == NULL) {
        YoriLibInsertList(&Parent->ChildFiles, &Child->ListEntry);
    } else {
        YoriLibInsertList(ListEntry, &Child->ListEntry);
    }
}

/**
 Navigate down the menu structure comparing path components to find the
 directory which should contain a given path.  This is done because the
 start menu is a composite view merged from multiple physical directories,
 so another directory may have created objects that belong as the start menu
 node for contents returned from a different directory.

 @param Root Pointer to the root directory to start enumerating from.

 @param NewNode Pointer to a fully qualified name for the node being inserted.

 @param Depth The number of path components that this node is from the root.

 @return Pointer to the parent for this object, or NULL if a parent is not
         found.  Note the expectation is that a parent will always be found,
         because file system enumeration will return parents before children,
         so a parent must have been created by one directory or another.
 */
PYUI_MENU_DIRECTORY
YuiFindStartingNode(
    __in PYUI_MENU_DIRECTORY Root,
    __in PYORI_STRING NewNode,
    __in DWORD Depth
    )
{
    PYUI_MENU_DIRECTORY Current = Root;
    DWORD Count;
    YORI_STRING Component;
    PYORI_LIST_ENTRY ListEntry;
    PYUI_MENU_DIRECTORY Child;

    for (Count = 0; Count < Depth; Count++) {
        if (!YuiFindDepthComponent(NewNode, &Component, Depth - Count, FALSE)) {
            ASSERT(FALSE);
            return NULL;
        }

        ListEntry = NULL;
        Child = NULL;
        ListEntry = YoriLibGetNextListEntry(&Current->ChildDirectories, ListEntry);
        while (ListEntry != NULL) {
            Child = CONTAINING_RECORD(ListEntry, YUI_MENU_DIRECTORY, ListEntry);
            if (YoriLibCompareStringInsensitive(&Child->Item.Text, &Component) == 0) {
                break;
            }
            ListEntry = YoriLibGetNextListEntry(&Current->ChildDirectories, ListEntry);
            Child = NULL;
        }

        if (Child == NULL) {
            ASSERT(Child != NULL);
            return NULL;
        }

        Current = Child;
    }

    return Current;
}

/**
 A callback function invoked for each directory in the start menu to populate
 its associated Win32 menu with items.  Note this function assumes a depth
 first construction so that a parent object can point to the menus of its
 children.

 @param Parent Pointer to the start menu directory to populate.

 @param Context Pointer to the application context.

 @return TRUE to continue enumerating, FALSE to terminate.
 */
BOOL
YuiPopulateMenuOnDirectory(
    __in PYUI_MENU_DIRECTORY Parent,
    __in PVOID Context
    )
{
    PYUI_CONTEXT EnumContext;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIST_ENTRY NextEntry;
    PYUI_MENU_DIRECTORY ExistingDir;
    PYUI_MENU_FILE ExistingFile;

    ListEntry = NULL;
    NextEntry = NULL;
    EnumContext = (PYUI_CONTEXT)Context;

    //
    //  Normal directories create their menus here.  The programs directly
    //  under the start menu are given a menu handle already.
    //

    if (Parent->MenuHandle == NULL) {
        Parent->MenuHandle = CreatePopupMenu();
        if (Parent->MenuHandle == NULL) {
            return FALSE;
        }
    }

    //
    //  Recurse down for any child objects first
    //

    ListEntry = YoriLibGetNextListEntry(&Parent->ChildDirectories, ListEntry);
    while (ListEntry != NULL) {
        NextEntry = YoriLibGetNextListEntry(&Parent->ChildDirectories, ListEntry);
        ExistingDir = CONTAINING_RECORD(ListEntry, YUI_MENU_DIRECTORY, ListEntry);
        AppendMenu(Parent->MenuHandle, MF_OWNERDRAW | MF_POPUP, (DWORD_PTR)ExistingDir->MenuHandle, (LPCTSTR)&ExistingDir->Item);
        ListEntry = NextEntry;
    }

    ListEntry = YoriLibGetNextListEntry(&Parent->ChildFiles, ListEntry);
    while (ListEntry != NULL) {
        NextEntry = YoriLibGetNextListEntry(&Parent->ChildFiles, ListEntry);
        ExistingFile = CONTAINING_RECORD(ListEntry, YUI_MENU_FILE, ListEntry);
        EnumContext->NextMenuIdentifier++;
        ExistingFile->MenuId = EnumContext->NextMenuIdentifier;
        AppendMenu(Parent->MenuHandle, MF_OWNERDRAW, ExistingFile->MenuId, (LPCTSTR)&ExistingFile->Item);
        ListEntry = NextEntry;
    }

    return TRUE;
}

/**
 A callback function that is invoked on all files/programs within the start
 menu tree to find the item corresponding to the user selection.  Obviously
 this could be implemented much more efficiently given the ID is known and
 specified, but this is a simple implementation given existing logic.

 @param Item Pointer to the item being enumerated.

 @param Context Pointer to a context which in this function refers to a DWORD
        indicating the identifier of the program to execute.

 @return TRUE to continue enumerating, FALSE to terminate.  For this function,
         that means TRUE if the item has not yet been found, and FALSE when it
         has been found.
 */
BOOL
YuiFindMenuCommandToExecute(
    __in PYUI_MENU_FILE Item,
    __in PVOID Context
    )
{
    PDWORD ItemToFind;
    BOOLEAN Elevated;

    ItemToFind = (PDWORD)Context;
    if (*ItemToFind != Item->MenuId) {
        return TRUE;
    }

    Elevated = FALSE;
    if (GetKeyState(VK_SHIFT) < 0) {
        Elevated = TRUE;
    }
    YoriLibExecuteShortcut(&Item->FilePath, Elevated);
    return FALSE;
}

/**
 A function prototype for a callback to be invoked for each file (aka program)
 found within the start menu tree.
 */
typedef
BOOL
YUI_MENU_FILE_CALLBACK(PYUI_MENU_FILE, PVOID);

/**
 A pointer to a function to be invoked for each file (aka program) found
 within the start menu tree.
 */
typedef YUI_MENU_FILE_CALLBACK *PYUI_MENU_FILE_CALLBACK;

/**
 A function prototype for a callback to be invoked for each directory
 found within the start menu tree.
 */
typedef
BOOL
YUI_MENU_DIRECTORY_CALLBACK(PYUI_MENU_DIRECTORY, PVOID);

/**
 A pointer to a function to be invoked for each directory found within the
 start menu tree.
 */
typedef YUI_MENU_DIRECTORY_CALLBACK *PYUI_MENU_DIRECTORY_CALLBACK;

/**
 Enumerate all known programs and subdirectories within the start menu and
 call a callback function for each.  Entries are called back depth first,
 meaning objects within a given directory are returned before their parents
 are returned.

 @param Parent The current directory node being enumerated.

 @param Context Caller specified context to pass along to each callback
        function.

 @param FileFn A callback function to invoke for each file (aka program)
        found within the start menu tree.  If NULL, no notifications are
        made for programs within the tree.

 @param DirFn A callback function to invoke for each subdirectory found within
        the start menu tree.  If NULL, no notifications are made for
        directories within the tree.

 @return TRUE to continue enumerating, or FALSE on failure.
 */
BOOL
YuiForEachFileOrDirectoryDepthFirst(
    __in PYUI_MENU_DIRECTORY Parent,
    __in_opt PVOID Context,
    __in_opt PYUI_MENU_FILE_CALLBACK FileFn,
    __in_opt PYUI_MENU_DIRECTORY_CALLBACK DirFn
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIST_ENTRY NextEntry;
    PYUI_MENU_DIRECTORY ExistingDir;
    PYUI_MENU_FILE ExistingFile;

    ListEntry = NULL;
    NextEntry = NULL;

    //
    //  Recurse down for any child objects first
    //

    ListEntry = YoriLibGetNextListEntry(&Parent->ChildDirectories, ListEntry);
    while (ListEntry != NULL) {
        NextEntry = YoriLibGetNextListEntry(&Parent->ChildDirectories, ListEntry);
        ExistingDir = CONTAINING_RECORD(ListEntry, YUI_MENU_DIRECTORY, ListEntry);
        if (!YuiForEachFileOrDirectoryDepthFirst(ExistingDir, Context, FileFn, DirFn)) {
            return FALSE;
        }
        ListEntry = NextEntry;
    }

    if (FileFn != NULL) {
        ListEntry = YoriLibGetNextListEntry(&Parent->ChildFiles, ListEntry);
        while (ListEntry != NULL) {
            NextEntry = YoriLibGetNextListEntry(&Parent->ChildFiles, ListEntry);
            ExistingFile = CONTAINING_RECORD(ListEntry, YUI_MENU_FILE, ListEntry);
            if (!FileFn(ExistingFile, Context)) {
                return FALSE;
            }
            ListEntry = NextEntry;
        }
    }

    if (DirFn != NULL) {
        if (!DirFn(Parent, Context)) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 Enumerate all known programs and subdirectories within the start menu and
 call a callback function for each.  Entries are called back breadth first,
 meaning objects within a given directory are returned before subdirectories
 within that directory.

 @param Parent The current directory node being enumerated.

 @param Context Caller specified context to pass along to each callback
        function.

 @param FileFn A callback function to invoke for each file (aka program)
        found within the start menu tree.  If NULL, no notifications are
        made for programs within the tree.

 @param DirFn A callback function to invoke for each subdirectory found within
        the start menu tree.  If NULL, no notifications are made for
        directories within the tree.

 @return TRUE to continue enumerating, or FALSE on failure.
 */
BOOL
YuiForEachFileOrDirectoryBreadthFirst(
    __in PYUI_MENU_DIRECTORY Parent,
    __in_opt PVOID Context,
    __in_opt PYUI_MENU_FILE_CALLBACK FileFn,
    __in_opt PYUI_MENU_DIRECTORY_CALLBACK DirFn
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIST_ENTRY NextEntry;
    PYUI_MENU_DIRECTORY ExistingDir;
    PYUI_MENU_FILE ExistingFile;

    ListEntry = NULL;
    NextEntry = NULL;

    if (DirFn != NULL) {
        if (!DirFn(Parent, Context)) {
            return FALSE;
        }
    }

    if (FileFn != NULL) {
        ListEntry = YoriLibGetNextListEntry(&Parent->ChildFiles, ListEntry);
        while (ListEntry != NULL) {
            NextEntry = YoriLibGetNextListEntry(&Parent->ChildFiles, ListEntry);
            ExistingFile = CONTAINING_RECORD(ListEntry, YUI_MENU_FILE, ListEntry);
            if (!FileFn(ExistingFile, Context)) {
                return FALSE;
            }
            ListEntry = NextEntry;
        }
    }

    //
    //  Recurse down for any child objects last
    //

    ListEntry = YoriLibGetNextListEntry(&Parent->ChildDirectories, ListEntry);
    while (ListEntry != NULL) {
        NextEntry = YoriLibGetNextListEntry(&Parent->ChildDirectories, ListEntry);
        ExistingDir = CONTAINING_RECORD(ListEntry, YUI_MENU_DIRECTORY, ListEntry);
        if (!YuiForEachFileOrDirectoryBreadthFirst(ExistingDir, Context, FileFn, DirFn)) {
            return FALSE;
        }
        ListEntry = NextEntry;
    }

    return TRUE;
}


/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param Context Pointer to the yui context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
YuiFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PYUI_CONTEXT YuiContext = (PYUI_CONTEXT)Context;
    PYUI_MENU_DIRECTORY Parent;
    YORI_STRING FriendlyName;
    PYUI_MENU_FILE NewFile;
    YORI_STRING Ext;

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    //
    //  If there's a filter directory and this file is under it, don't do
    //  anything but continue enumerating.
    //

    if (YuiContext->FilterDirectory.LengthInChars > 0) {
        if (FilePath->LengthInChars >= YuiContext->FilterDirectory.LengthInChars &&
            YoriLibCompareStringInsensitiveCount(FilePath, &YuiContext->FilterDirectory, YuiContext->FilterDirectory.LengthInChars) == 0) {
            return TRUE;
        }


        //
        //  If it's not a directory and is directly under the specified
        //  directory, insert it into the start directory and return.
        //  If it's a directory or nested, put it in the programs directory.
        //
        if (Depth == 0 &&
            (FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

            YoriLibInitEmptyString(&Ext);
            Ext.StartOfString = YoriLibFindRightMostCharacter(FilePath, '.');
            if (Ext.StartOfString != NULL) {
                Ext.LengthInChars = FilePath->LengthInChars - (DWORD)(Ext.StartOfString - FilePath->StartOfString);
            }

            if (YoriLibCompareStringWithLiteralInsensitive(&Ext, _T(".lnk")) == 0 &&
                YuiFindDepthComponent(FilePath, &FriendlyName, 0, TRUE)) {

                NewFile = YuiCreateMenuFile(YuiContext, FilePath, &FriendlyName, TRUE);
                if (NewFile != NULL) {
                    NewFile->Depth = Depth + 1;
                    YuiInsertFileInOrder(&YuiMenuContext.StartDirectory, NewFile);
                }
            }

            return TRUE;
        }
    }


    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        PYUI_MENU_DIRECTORY NewDir;

        Parent = YuiFindStartingNode(&YuiMenuContext.ProgramsDirectory, FilePath, Depth);

        if (Parent != NULL &&
            YuiFindDepthComponent(FilePath, &FriendlyName, 0, FALSE)) {

            if (!YuiDirectoryNodeExists(Parent, &FriendlyName)) {
                NewDir = YuiCreateMenuDirectory(YuiContext, &FriendlyName);
                if (NewDir != NULL) {
                    NewDir->Depth = Depth + 1;
                    YuiInsertDirectoryInOrder(Parent, NewDir);
                }
            }
        }
    } else {

        Parent = YuiFindStartingNode(&YuiMenuContext.ProgramsDirectory, FilePath, Depth);

        YoriLibInitEmptyString(&Ext);
        Ext.StartOfString = YoriLibFindRightMostCharacter(FilePath, '.');
        if (Ext.StartOfString != NULL) {
            Ext.LengthInChars = FilePath->LengthInChars - (DWORD)(Ext.StartOfString - FilePath->StartOfString);
        }

        if (Parent != NULL &&
            YoriLibCompareStringWithLiteralInsensitive(&Ext, _T(".lnk")) == 0 &&
            YuiFindDepthComponent(FilePath, &FriendlyName, 0, TRUE)) {
            NewFile = YuiCreateMenuFile(YuiContext, FilePath, &FriendlyName, FALSE);
            if (NewFile != NULL) {
                NewFile->Depth = Depth + 1;
                YuiInsertFileInOrder(Parent, NewFile);
            }
        }
    }

    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the context block indicating whether the
        enumeration was recursive.  Recursive enumerates do not complain
        if a matching file is not in every single directory, because
        common usage expects files to be in a subset of directories only.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
YuiFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Depth);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        Result = TRUE;
    } else {
        LPTSTR ErrText = YoriLibGetWinErrorText(ErrorCode);
        YORI_STRING DirName;
        LPTSTR FilePart;
        YoriLibInitEmptyString(&DirName);
        DirName.StartOfString = UnescapedFilePath.StartOfString;
        FilePart = YoriLibFindRightMostCharacter(&UnescapedFilePath, '\\');
        if (FilePart != NULL) {
            DirName.LengthInChars = (DWORD)(FilePart - DirName.StartOfString);
        } else {
            DirName.LengthInChars = UnescapedFilePath.LengthInChars;
        }
        YoriLibFreeWinErrorText(ErrText);
    }
    YoriLibFreeStringContents(&UnescapedFilePath);
    return Result;
}

/**
 Enumerate all shortcuts in known folders and populate the start menu with
 shortcut files that have been found.

 @param YuiContext Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiMenuPopulate(
    __in PYUI_CONTEXT YuiContext
    )
{
    YORI_STRING EnumDir;
    DWORD MatchFlags;

    //
    //  If no change notifications exist because this is the first pass,
    //  create them now.  This is done before enumerating so if anything
    //  changes after this point we may enumerate again.
    //

    if (YuiContext->StartChangeNotifications[0] == NULL) {
        YORI_STRING FullPath;
        YoriLibInitEmptyString(&FullPath);

        YoriLibConstantString(&EnumDir, _T("~PROGRAMS"));
        if (!YoriLibUserStringToSingleFilePath(&EnumDir, TRUE, &FullPath)) {
            return FALSE;
        }

        if (YuiContext->StartChangeNotifications[0] != NULL) {
            FindCloseChangeNotification(YuiContext->StartChangeNotifications[0]);
            YuiContext->StartChangeNotifications[0] = NULL;
        }
        YuiContext->StartChangeNotifications[0] =
            FindFirstChangeNotification(FullPath.StartOfString,
                                        TRUE,
                                        FILE_NOTIFY_CHANGE_FILE_NAME |
                                          FILE_NOTIFY_CHANGE_DIR_NAME |
                                          FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                          FILE_NOTIFY_CHANGE_SIZE |
                                          FILE_NOTIFY_CHANGE_LAST_WRITE);
        if (YuiContext->StartChangeNotifications[0] == NULL ||
            YuiContext->StartChangeNotifications[0] == INVALID_HANDLE_VALUE) {

            YuiContext->StartChangeNotifications[0] = NULL;
            YoriLibFreeStringContents(&FullPath);
            return FALSE;
        }
        YoriLibFreeStringContents(&FullPath);

        YoriLibConstantString(&EnumDir, _T("~START"));
        if (!YoriLibUserStringToSingleFilePath(&EnumDir, TRUE, &FullPath)) {
            return FALSE;
        }

        if (YuiContext->StartChangeNotifications[1] != NULL) {
            FindCloseChangeNotification(YuiContext->StartChangeNotifications[1]);
            YuiContext->StartChangeNotifications[1] = NULL;
        }
        YuiContext->StartChangeNotifications[1] =
            FindFirstChangeNotification(FullPath.StartOfString,
                                        TRUE,
                                        FILE_NOTIFY_CHANGE_FILE_NAME |
                                          FILE_NOTIFY_CHANGE_DIR_NAME |
                                          FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                          FILE_NOTIFY_CHANGE_SIZE |
                                          FILE_NOTIFY_CHANGE_LAST_WRITE);
        if (YuiContext->StartChangeNotifications[1] == NULL ||
            YuiContext->StartChangeNotifications[1] == INVALID_HANDLE_VALUE) {

            YuiContext->StartChangeNotifications[1] = NULL;
            YoriLibFreeStringContents(&FullPath);
            return FALSE;
        }
        YoriLibFreeStringContents(&FullPath);

        YoriLibConstantString(&EnumDir, _T("~COMMONPROGRAMS"));
        if (!YoriLibUserStringToSingleFilePath(&EnumDir, TRUE, &FullPath)) {
            return FALSE;
        }

        if (YuiContext->StartChangeNotifications[2] != NULL) {
            FindCloseChangeNotification(YuiContext->StartChangeNotifications[2]);
            YuiContext->StartChangeNotifications[2] = NULL;
        }
        YuiContext->StartChangeNotifications[2] =
            FindFirstChangeNotification(FullPath.StartOfString,
                                        TRUE,
                                        FILE_NOTIFY_CHANGE_FILE_NAME |
                                          FILE_NOTIFY_CHANGE_DIR_NAME |
                                          FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                          FILE_NOTIFY_CHANGE_SIZE |
                                          FILE_NOTIFY_CHANGE_LAST_WRITE);
        if (YuiContext->StartChangeNotifications[2] == NULL ||
            YuiContext->StartChangeNotifications[2] == INVALID_HANDLE_VALUE) {

            YuiContext->StartChangeNotifications[2] = NULL;
            YoriLibFreeStringContents(&FullPath);
            return FALSE;
        }
        YoriLibFreeStringContents(&FullPath);

        YoriLibConstantString(&EnumDir, _T("~COMMONSTART"));
        if (!YoriLibUserStringToSingleFilePath(&EnumDir, TRUE, &FullPath)) {
            return FALSE;
        }

        if (YuiContext->StartChangeNotifications[3] != NULL) {
            FindCloseChangeNotification(YuiContext->StartChangeNotifications[3]);
            YuiContext->StartChangeNotifications[3] = NULL;
        }
        YuiContext->StartChangeNotifications[3] =
            FindFirstChangeNotification(FullPath.StartOfString,
                                        TRUE,
                                        FILE_NOTIFY_CHANGE_FILE_NAME |
                                          FILE_NOTIFY_CHANGE_DIR_NAME |
                                          FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                          FILE_NOTIFY_CHANGE_SIZE |
                                          FILE_NOTIFY_CHANGE_LAST_WRITE);
        if (YuiContext->StartChangeNotifications[3] == NULL ||
            YuiContext->StartChangeNotifications[3] == INVALID_HANDLE_VALUE) {

            YuiContext->StartChangeNotifications[3] = NULL;
            YoriLibFreeStringContents(&FullPath);
            return FALSE;
        }
        YoriLibFreeStringContents(&FullPath);
    }

    MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES;
    MatchFlags |= YORILIB_FILEENUM_RECURSE_AFTER_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;

    //
    //  Load everything from the user's start menu directory, ignoring
    //  anything that's also under the programs directory.
    //

    YoriLibInitEmptyString(&YuiContext->FilterDirectory);
    YoriLibConstantString(&EnumDir, _T("~PROGRAMS"));
    if (!YoriLibUserStringToSingleFilePath(&EnumDir, TRUE, &YuiContext->FilterDirectory)) {
        YoriLibInitEmptyString(&YuiContext->FilterDirectory);
    }

    YoriLibConstantString(&EnumDir, _T("~START\\*"));

    YoriLibForEachFile(&EnumDir,
                       MatchFlags,
                       0,
                       YuiFileFoundCallback,
                       YuiFileEnumerateErrorCallback,
                       YuiContext);

    YoriLibFreeStringContents(&YuiContext->FilterDirectory);

    //
    //  Load everything from the user's programs directory.
    //

    YoriLibConstantString(&EnumDir, _T("~PROGRAMS\\*"));

    YoriLibForEachFile(&EnumDir,
                       MatchFlags,
                       0,
                       YuiFileFoundCallback,
                       YuiFileEnumerateErrorCallback,
                       YuiContext);

    //
    //  Load everything from the systems's start menu directory, ignoring
    //  anything that's also under the programs directory.
    //

    YoriLibConstantString(&EnumDir, _T("~COMMONPROGRAMS"));
    if (!YoriLibUserStringToSingleFilePath(&EnumDir, TRUE, &YuiContext->FilterDirectory)) {
        YoriLibInitEmptyString(&YuiContext->FilterDirectory);
    }

    YoriLibConstantString(&EnumDir, _T("~COMMONSTART\\*"));

    YoriLibForEachFile(&EnumDir,
                       MatchFlags,
                       0,
                       YuiFileFoundCallback,
                       YuiFileEnumerateErrorCallback,
                       YuiContext);

    YoriLibFreeStringContents(&YuiContext->FilterDirectory);

    //
    //  Load everything from the system's programs directory.
    //

    YoriLibConstantString(&EnumDir, _T("~COMMONPROGRAMS\\*"));

    YoriLibForEachFile(&EnumDir,
                       MatchFlags,
                       0,
                       YuiFileFoundCallback,
                       YuiFileEnumerateErrorCallback,
                       YuiContext);

    //
    //  Populate the menus with human readable strings from the entries we
    //  just loaded, and assign each menu an identifier that corresponds
    //  to an entry in the tree.
    //

    YuiContext->NextMenuIdentifier = YUI_MENU_FIRST_PROGRAM_MENU_ID;

    YuiContext->StartMenu = CreatePopupMenu();
    if (YuiContext->StartMenu == NULL) {
        return FALSE;
    }

    YuiMenuContext.StartDirectory.MenuHandle = YuiContext->StartMenu;

    YuiForEachFileOrDirectoryDepthFirst(&YuiMenuContext.StartDirectory,
                                        YuiContext,
                                        NULL,
                                        YuiPopulateMenuOnDirectory);

    if (!YoriLibIsListEmpty(&YuiMenuContext.StartDirectory.ChildFiles)) {
        AppendMenu(YuiContext->StartMenu, MF_SEPARATOR, 0, NULL);
    }

    YuiForEachFileOrDirectoryDepthFirst(&YuiMenuContext.ProgramsDirectory,
                                        YuiContext,
                                        NULL,
                                        YuiPopulateMenuOnDirectory);

#if DBG
    YuiContext->DebugMenu = CreatePopupMenu();
    if (YuiContext->DebugMenu == NULL) {
        return FALSE;
    }

    YuiContext->DebugMenuItemChecked = YuiContext->DebugLogEnabled;

    AppendMenu(YuiContext->DebugMenu, MF_OWNERDRAW, YUI_MENU_REFRESH, (LPCWSTR)&YuiMenuContext.DebugRefreshTaskbar);
    AppendMenu(YuiContext->DebugMenu, MF_OWNERDRAW | (YuiContext->DebugMenuItemChecked?MF_CHECKED:0), YUI_MENU_LOGGING, (LPCWSTR)&YuiMenuContext.DebugToggleLogging);
    AppendMenu(YuiContext->DebugMenu, MF_OWNERDRAW, YUI_MENU_LAUNCHWINLOGONSHELL, (LPCWSTR)&YuiMenuContext.DebugLaunchWinlogonShell);
    AppendMenu(YuiContext->DebugMenu, MF_OWNERDRAW, YUI_MENU_DISPLAYCHANGE, (LPCWSTR)&YuiMenuContext.DebugSimulateDisplayChange);
#endif

    //
    //  Add in all of the predefined menu entries.
    //

    AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW | MF_POPUP, (DWORD_PTR)YuiMenuContext.ProgramsDirectory.MenuHandle, (LPCWSTR)&YuiMenuContext.Programs);
#if DBG
    AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW | MF_POPUP, (DWORD_PTR)YuiContext->DebugMenu, (LPCWSTR)&YuiMenuContext.Debug);
#endif
    AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW, YUI_MENU_RUN, (LPCWSTR)&YuiMenuContext.Run);
    AppendMenu(YuiContext->StartMenu, MF_SEPARATOR, 0, NULL);
    if (YuiContext->LoginShell) {
        AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW, YUI_MENU_LOGOFF, (LPCWSTR)&YuiMenuContext.Exit);
    } else {
        AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW, YUI_MENU_EXIT, (LPCWSTR)&YuiMenuContext.Exit);
    }

    YuiContext->ShutdownMenu = CreatePopupMenu();
    if (YuiContext->ShutdownMenu == NULL) {
        return FALSE;
    }
    AppendMenu(YuiContext->ShutdownMenu, MF_OWNERDRAW, YUI_MENU_DISCONNECT, (LPCWSTR)&YuiMenuContext.ShutdownDisconnect);
    AppendMenu(YuiContext->ShutdownMenu, MF_OWNERDRAW, YUI_MENU_LOCK, (LPCWSTR)&YuiMenuContext.ShutdownLock);
    if (YuiContext->LoginShell) {
        AppendMenu(YuiContext->ShutdownMenu, MF_OWNERDRAW, YUI_MENU_EXIT, (LPCWSTR)&YuiMenuContext.ShutdownExit);
    } else {
        AppendMenu(YuiContext->ShutdownMenu, MF_OWNERDRAW, YUI_MENU_LOGOFF, (LPCWSTR)&YuiMenuContext.ShutdownLogoff);
    }
    AppendMenu(YuiContext->ShutdownMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(YuiContext->ShutdownMenu, MF_OWNERDRAW, YUI_MENU_REBOOT, (LPCWSTR)&YuiMenuContext.ShutdownReboot);
    AppendMenu(YuiContext->ShutdownMenu, MF_OWNERDRAW, YUI_MENU_SHUTDOWN, (LPCWSTR)&YuiMenuContext.ShutdownShutdown);
    AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW | MF_POPUP, (DWORD_PTR)YuiContext->ShutdownMenu, (LPCWSTR)&YuiMenuContext.Shutdown);

    return TRUE;
}

/**
 Deallocate all contexts associated with found start menu shortcuts or
 directories.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiMenuFreeAll(
    __in PYUI_CONTEXT YuiContext
    )
{
    YuiForEachFileOrDirectoryDepthFirst(&YuiMenuContext.ProgramsDirectory,
                                        &YuiMenuContext.ProgramsDirectory,
                                        YuiDeleteMenuFile,
                                        YuiDeleteMenuDirectory);

    YuiForEachFileOrDirectoryDepthFirst(&YuiMenuContext.StartDirectory,
                                        &YuiMenuContext.StartDirectory,
                                        YuiDeleteMenuFile,
                                        YuiDeleteMenuDirectory);

    if (YuiContext->ShutdownMenu != NULL) {
        DestroyMenu(YuiContext->ShutdownMenu);
        YuiContext->ShutdownMenu = NULL;
    }

    if (YuiContext->DebugMenu != NULL) {
        DestroyMenu(YuiContext->DebugMenu);
        YuiContext->DebugMenu = NULL;
    }

    //
    //  Because this is associated with StartDirectory, it's already destroyed
    //

    YuiContext->StartMenu = NULL;
}

/**
 Check if any change notification that is monitoring start menu changes has
 detected a change.  If no changes are detected, return immediately and
 allow the previously generated start menu to be displayed.  If changes are
 detected, purge the old start menu and reload the new one, then allow this
 to be displayed.  The change notifications are re-queued to detect a 
 subsequent change.

 @param YuiContext Pointer to the context containing the start menu and the
        change notifications that are monitoring it.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiMenuReloadIfChanged(
    __in PYUI_CONTEXT YuiContext
    )
{
    DWORD WaitStatus;
    DWORD HandleCount;
    BOOLEAN FoundChange;

    FoundChange = FALSE;
#if DBG
    if (YuiContext->DebugMenuItemChecked != YuiContext->DebugLogEnabled) {
        FoundChange = TRUE;
    }
#endif

    if (!FoundChange) {
        HandleCount = sizeof(YuiContext->StartChangeNotifications)/sizeof(YuiContext->StartChangeNotifications[0]);
        while(TRUE) {
            WaitStatus = WaitForMultipleObjectsEx(HandleCount,
                                                  YuiContext->StartChangeNotifications,
                                                  FALSE,
                                                  0,
                                                  FALSE);
            if (WaitStatus == WAIT_TIMEOUT) {
                if (!FoundChange) {
                    return TRUE;
                }
                break;
            }

            FoundChange = TRUE;
            if (WaitStatus < WAIT_OBJECT_0 + HandleCount) {
                FindNextChangeNotification(YuiContext->StartChangeNotifications[WaitStatus - WAIT_OBJECT_0]);
            }
        }
    }

    YuiMenuFreeAll(YuiContext);
    return YuiMenuPopulate(YuiContext);
}

/**
 A structure defining state for the run browse (file open) dialog box.
 */
typedef struct _OPENFILENAMEW {

    /**
     The size of the structure, in bytes.
     */
    DWORD StructureSize;

    /**
     The parent window for the dialog box.
     */
    HWND hWndParent;

    /**
     The instance to obtain any custom dialog templates from.  Unused in this
     program.
     */
    HINSTANCE hInstance;

    /**
     The list of file criteria that the user can select.
     */
    LPCWSTR Filter;

    /**
     A custom criteria to populate if the user selects a file criteria, which
     may be part of the list or user specified.  Unused in this program.
     */
    LPWSTR CustomFilter;

    /**
     The number of characters in the custom criteria.
     */
    DWORD MaxCustFilter;

    /**
     Specifies the default filter from the Filter list.
     */
    DWORD FilterIndex;

    /**
     On successful completion, populated with the user selected file name.
     */
    LPWSTR File;

    /**
     The number of characters in the file name buffer.
     */
    DWORD MaxFile;

    /**
     On successful completion, populated with the file name and extension
     (without path.)  Unused in this program.
     */
    LPWSTR FileTitle;

    /**
     The number of characters in the file title buffer.
     */
    DWORD MaxFileTitle;

    /**
     The initial directory to start the dialog box in.  Note that newer
     versions of Windows have broken this concept so badly that this field
     can essentially never be used.
     */
    LPCWSTR InitialDir;

    /**
     The title of the dialog box.
     */
    LPCWSTR Title;

    /**
     Flags specifying the behavior of the dialog box.
     */
    DWORD Flags;

    /**
     On successful completion, indicates the offset within the FileName
     buffer that is the beginning of the file name portion, without any
     path.
     */
    WORD FileOffset;

    /**
     On successful completion, indicates the offset within the FileName
     buffer that is the beginning of the file extension portion.
     */
    WORD FileExtension;

    /**
     The default extension to append if the user doesn't type one.
     */
    LPCWSTR DefaultExtension;

    /**
     Specifies a custom hook for the dialog.  Unused in this program.
     */
    LPARAM CustomData;

    /**
     Specifies a custom hook for the dialog.  Unused in this program.
     */
    LPVOID FnHook;

    /**
     Specifies a custom template for the dialog.  Unused in this program.
     */
    LPCWSTR TemplateName;
} OPENFILENAMEW, *LPOPENFILENAMEW;

/**
 Flag to indicate that the read only check box should not be displayed.
 */
#define OFN_HIDEREADONLY 0x00000004

/**
 Flag to indicate that long file names should be returned.
 */
#define OFN_LONGNAMES 0x00200000

/**
 Flag to indicate that the dialog should be resizable.
 */
#define OFN_ENABLESIZING 0x00800000

/**
 Declaration for the GetOpenFileName API for compilation environments that
 don't include it.
 */
BOOL WINAPI GetOpenFileNameW(LPOPENFILENAMEW);

/**
 Launch the browse (file open) dialog and populate a supplied buffer with the
 file that the user selected.

 @param hWndParent Handle to the parent window of the file open dialog.

 @param StringToPopulate On successful completion, the contents of this string
        are updated to contain the file name that the user selected.  Note
        this string is allocated by the caller.

 @return TRUE to indicate the user successfully selected a file, FALSE if not.
 */
BOOL
YuiMenuRunBrowse(
    __in HWND hWndParent,
    __in PYORI_STRING StringToPopulate
    )
{
    OPENFILENAMEW ofn;

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.StructureSize = sizeof(ofn);
    ofn.hWndParent = hWndParent;
    ofn.Filter = _T("Program files (*.exe;*.com)\0*.EXE;*.COM\0All Files\0*.*\0");
    ofn.Title = _T("Run");
    ofn.File = StringToPopulate->StartOfString;
    ofn.MaxFile = StringToPopulate->LengthAllocated;
    StringToPopulate->StartOfString[0] = '\0';
    ofn.DefaultExtension = _T("EXE");
    ofn.Flags = OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ENABLESIZING;

    return GetOpenFileNameW(&ofn);
}

/**
 Context that is preserved from when the run dialog is initialized so long
 as it remains active.  Currently this just points to the global context.
 */
PYUI_CONTEXT RunDlgContext;

/**
 A callback function which processes notifications about actions that the user
 has selected from the dialog.

 @param hDlg The handle to the dialog window.

 @param uMsg The dialog message.

 @param wParam The first parameter to the dialog message.

 @param lParam The second parameter to the dialog message.

 @return TRUE if the message was processed in this routine, FALSE if not.
 */
BOOL CALLBACK
RunDialogProc(
    __in HWND hDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    YORI_STRING Cmd;

    switch (uMsg) {
        case WM_INITDIALOG:
            RunDlgContext = (PYUI_CONTEXT)lParam;
            if (DllShell32.pShellExecuteW == NULL) {
                return FALSE;
            }

            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BROWSE:
                    if (YoriLibAllocateString(&Cmd, 1024)) {
                        if (YuiMenuRunBrowse(hDlg, &Cmd)) {
                            SetDlgItemText(hDlg, IDC_RUNCMD, Cmd.StartOfString);
                        }
                        YoriLibFreeStringContents(&Cmd);
                    }
                    return TRUE;
                case IDC_OK:
                    {
                        DWORD Length;
                        HINSTANCE hInst;
                        DWORD Err;

                        EnableWindow(GetDlgItem(hDlg, IDC_OK), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_CANCEL), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_RUNCMD), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BROWSE), FALSE);

                        Length = GetWindowTextLength(GetDlgItem(hDlg, IDC_RUNCMD));
                        if (YoriLibAllocateString(&Cmd, Length + 1)) {
                            PYORI_STRING ArgV;
                            DWORD Index;
                            DWORD ArgC;
                            LPTSTR ArgString;

                            Cmd.LengthInChars = GetDlgItemText(hDlg, IDC_RUNCMD, Cmd.StartOfString, Cmd.LengthAllocated);

                            ArgV = YoriLibCmdlineToArgcArgv(Cmd.StartOfString, 2, FALSE, &ArgC);
                            if (ArgV != NULL) {
                                ArgString = NULL;
                                if (ArgC > 1) {
                                    ArgString = ArgV[1].StartOfString;
                                }
                                hInst = DllShell32.pShellExecuteW(NULL, NULL, ArgV[0].StartOfString, ArgString, NULL, SW_SHOWNORMAL);
                                Err = YoriLibShellExecuteInstanceToError(hInst);
                                if (Err != ERROR_SUCCESS) {
                                    LPTSTR WinErrText;
                                    YORI_STRING ErrText;
                                    YoriLibInitEmptyString(&ErrText);

                                    WinErrText = YoriLibGetWinErrorText(Err);
                                    if (WinErrText != NULL) {
                                        YoriLibYPrintf(&ErrText, _T("Could not execute \"%y\": %s"), &Cmd, YoriLibGetWinErrorText(Err));
                                        if (ErrText.StartOfString != NULL) {
                                            MessageBox(hDlg, ErrText.StartOfString, _T("Yui"), MB_ICONSTOP);
                                            YoriLibFreeStringContents(&ErrText);
                                        }
                                        YoriLibFreeWinErrorText(WinErrText);
                                    }
                                }

                                YoriLibFreeStringContents(&Cmd);
                                for (Index = 0; Index < ArgC; Index++) {
                                    YoriLibFreeStringContents(&ArgV[Index]);
                                }
                                YoriLibDereference(ArgV);
                            }
                        }

                        EndDialog(hDlg, TRUE);
                        return TRUE;
                    }
                case IDC_CANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
            }
    }

    return FALSE;
}

/**
 Display the run dialog box to allow the user to launch a custom program.

 @param YuiContext Pointer the global context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiMenuRun(
    __in PYUI_CONTEXT YuiContext
    )
{
    DialogBoxParam(NULL, MAKEINTRESOURCE(RUNDIALOG), YuiContext->hWnd, (DLGPROC)RunDialogProc, (DWORD_PTR)YuiContext);
    return TRUE;
}

/**
 Execute the item selected from the start menu.

 @param YuiContext Pointer to the application context.

 @param MenuId The identifier of the menu item that the user selected.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiMenuExecuteById(
    __in PYUI_CONTEXT YuiContext,
    __in DWORD MenuId
    )
{
    switch(MenuId) {
        case YUI_MENU_EXIT:
            PostQuitMessage(0);
            break;
        case YUI_MENU_DISCONNECT:
            if (DllWtsApi32.pWTSDisconnectSession != NULL) {
                DllWtsApi32.pWTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, FALSE);
            }
            break;
        case YUI_MENU_LOCK:
            if (DllUser32.pLockWorkStation != NULL) {
                DllUser32.pLockWorkStation();
            }
            break;
        case YUI_MENU_LOGOFF:
            if (DllUser32.pExitWindowsEx != NULL) {
                DllUser32.pExitWindowsEx(EWX_LOGOFF, 0);
            }
            break;
        case YUI_MENU_REBOOT:
            if (DllUser32.pExitWindowsEx != NULL) {
                YoriLibEnableShutdownPrivilege();
                DllUser32.pExitWindowsEx(EWX_REBOOT, 0);
            }
            break;
        case YUI_MENU_SHUTDOWN:
            if (DllUser32.pExitWindowsEx != NULL) {
                DWORD OsMajor;
                DWORD OsMinor;
                DWORD BuildNumber;
                YoriLibEnableShutdownPrivilege();
                YoriLibGetOsVersion(&OsMajor, &OsMinor, &BuildNumber);

                //
                //  When asked to power down in VirtualBox, older versions
                //  reboot instead.
                //

                if (OsMajor >= 5) {
                    DllUser32.pExitWindowsEx(EWX_SHUTDOWN | EWX_POWEROFF, 0);
                } else {
                    DllUser32.pExitWindowsEx(EWX_SHUTDOWN, 0);
                }
            }
            break;
        case YUI_MENU_RUN:
            YuiMenuRun(YuiContext);
            break;
#if DBG
        case YUI_MENU_REFRESH:
            YuiTaskbarSyncWithCurrent(YuiContext);
            break;
        case YUI_MENU_LOGGING:
            YuiMenuToggleLogging(YuiContext);
            break;
        case YUI_MENU_LAUNCHWINLOGONSHELL:
            YuiExitAndLaunchWinlogonShell(YuiContext);
            break;
        case YUI_MENU_DISPLAYCHANGE:
            YuiNotifyResolutionChange(YuiContext->hWnd);
            break;
#endif
        default:
            ASSERT(MenuId >= YUI_MENU_FIRST_PROGRAM_MENU_ID);

            if (YuiForEachFileOrDirectoryDepthFirst(&YuiMenuContext.StartDirectory,
                                                    &MenuId,
                                                    YuiFindMenuCommandToExecute,
                                                    NULL)) {
                YuiForEachFileOrDirectoryDepthFirst(&YuiMenuContext.ProgramsDirectory,
                                                    &MenuId,
                                                    YuiFindMenuCommandToExecute,
                                                    NULL);
            }
    }

    return TRUE;
}


/**
 Display the start menu and execute any item selected.

 @param YuiContext Pointer to the application context.

 @param hWnd The handle of the taskbar window.

 @return TRUE if the menu was displayed successfully, FALSE if not.
 */
BOOL
YuiMenuDisplayAndExecute(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    )
{
    RECT WindowRect;
    DWORD MenuId;

    if (!YuiMenuReloadIfChanged(YuiContext)) {
        return FALSE;
    }

    DllUser32.pGetWindowRect(hWnd, &WindowRect);

    DllUser32.pSetForegroundWindow(hWnd);

    MenuId = TrackPopupMenu(YuiContext->StartMenu,
                            TPM_NONOTIFY | TPM_RETURNCMD | TPM_BOTTOMALIGN,
                            0,
                            WindowRect.top,
                            0,
                            hWnd,
                            NULL);

    PostMessage(hWnd, WM_NULL, 0, 0);

    //
    //  If the start button is pressed, un-press it before starting new
    //  work.  The window manager will recursively un-press it if focus moves,
    //  and we need to make sure it's redrawn.
    //

    if (YuiContext->MenuActive) {
        YuiContext->MenuActive = FALSE;
        SendMessage(YuiContext->hWndStart, BM_SETSTATE, FALSE, 0);
    }

    if (MenuId > 0) {
        YuiMenuExecuteById(YuiContext, MenuId);
    } else {
        YuiTaskbarSwitchToActiveTask(YuiContext);
    }

    return TRUE;
}

// vim:sw=4:ts=4:et:
