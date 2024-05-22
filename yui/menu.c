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
 A structure passed when looking for an item to execute from the menu.  This
 is used because Windows returns a control ID, so we search again through the
 tree looking for a matching control.
 */
typedef struct _YUI_MENU_FIND_EXEC_CONTEXT {

    /**
     Pointer to the monitor context.
     */
    PYUI_MONITOR YuiMonitor;

    /**
     The menu control to search for.
     */
    DWORD CtrlId;

    /**
     If a match is found and execution commences, the process ID of the child
     process.
     */
    DWORD ProcessId;

} YUI_MENU_FIND_EXEC_CONTEXT, *PYUI_MENU_FIND_EXEC_CONTEXT;

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
     Owner draw state for a menu seperator.  This is reused for all of them,
     since they're all rendered the same.
     */
    YUI_MENU_OWNERDRAW_ITEM Seperator;

    /**
     Owner draw state for the Programs menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM Programs;

    /**
     Owner draw state for the System menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM System;

    /**
     Owner draw state for the System Wifi menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM SystemWifi;

    /**
     Owner draw state for the System Mixer menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM SystemMixer;

    /**
     Owner draw state for the System Control Panel menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM SystemControlPanel;

    /**
     Owner draw state for the System TaskMgr menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM SystemTaskMgr;

    /**
     Owner draw state for the System Cmd menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM SystemCmd;

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

    /**
     Owner draw state for the Cascade menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ContextCascade;

    /**
     Owner draw state for the Tile side by side menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ContextTileSideBySide;

    /**
     Owner draw state for the Tile stacked menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ContextTileStacked;

    /**
     Owner draw state for the show desktop menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ContextShowDesktop;

    /**
     Owner draw state for the refresh taskbar menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM ContextRefreshTaskbar;

    /**
     Owner draw state for the minimize window menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM WinContextMinimize;

    /**
     Owner draw state for the hide window menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM WinContextHide;

    /**
     Owner draw state for the close window menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM WinContextClose;

    /**
     Owner draw state for the terminate process menu item.
     */
    YUI_MENU_OWNERDRAW_ITEM WinContextTerminateProcess;

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
    Item->WidthByStringLength = TRUE;
    Item->AddFlyoutIcon = FALSE;
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
    YuiMenuCleanupItem(&YuiMenuContext.Seperator);

    YuiMenuCleanupItem(&YuiMenuContext.System);
    YuiMenuCleanupItem(&YuiMenuContext.SystemWifi);
    YuiMenuCleanupItem(&YuiMenuContext.SystemMixer);
    YuiMenuCleanupItem(&YuiMenuContext.SystemControlPanel);
    YuiMenuCleanupItem(&YuiMenuContext.SystemTaskMgr);
    YuiMenuCleanupItem(&YuiMenuContext.SystemCmd);

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

    YuiMenuCleanupItem(&YuiMenuContext.ContextCascade);
    YuiMenuCleanupItem(&YuiMenuContext.ContextTileSideBySide);
    YuiMenuCleanupItem(&YuiMenuContext.ContextTileStacked);
    YuiMenuCleanupItem(&YuiMenuContext.ContextShowDesktop);
    YuiMenuCleanupItem(&YuiMenuContext.ContextRefreshTaskbar);

    YuiMenuCleanupItem(&YuiMenuContext.WinContextMinimize);
    YuiMenuCleanupItem(&YuiMenuContext.WinContextHide);
    YuiMenuCleanupItem(&YuiMenuContext.WinContextClose);
    YuiMenuCleanupItem(&YuiMenuContext.WinContextTerminateProcess);
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

    YuiMenuInitializeItem(&YuiMenuContext.Seperator);
    YoriLibConstantString(&YuiMenuContext.Seperator.Text, _T(""));
    YuiMenuContext.Seperator.TallItem = FALSE;
    YuiMenuContext.Seperator.WidthByStringLength = TRUE;

    YuiMenuContext.Programs.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, PROGRAMSICON, TRUE);
    YoriLibConstantString(&YuiMenuContext.Programs.Text, _T("Programs"));
    YuiMenuContext.Programs.TallItem = TRUE;
    YuiMenuContext.Programs.WidthByStringLength = FALSE;

    YuiMenuContext.System.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, SYSTEMICON, TRUE);
    YoriLibConstantString(&YuiMenuContext.System.Text, _T("System"));
    YuiMenuContext.System.TallItem = TRUE;
    YuiMenuContext.System.WidthByStringLength = FALSE;

    YuiMenuInitializeItem(&YuiMenuContext.SystemWifi);
    YoriLibConstantString(&YuiMenuContext.SystemWifi.Text, _T("Wi-fi Configuration"));
    YuiMenuInitializeItem(&YuiMenuContext.SystemMixer);
    YoriLibConstantString(&YuiMenuContext.SystemMixer.Text, _T("Audio Mixer"));
    YuiMenuInitializeItem(&YuiMenuContext.SystemControlPanel);
    YoriLibConstantString(&YuiMenuContext.SystemControlPanel.Text, _T("Control Panel"));
    YuiMenuInitializeItem(&YuiMenuContext.SystemTaskMgr);
    YoriLibConstantString(&YuiMenuContext.SystemTaskMgr.Text, _T("Task Manager"));
    YuiMenuInitializeItem(&YuiMenuContext.SystemCmd);
    YoriLibConstantString(&YuiMenuContext.SystemCmd.Text, _T("Command Prompt"));

#if DBG
    YuiMenuContext.Debug.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, DEBUGICON, TRUE);
    YoriLibConstantString(&YuiMenuContext.Debug.Text, _T("Debug"));
    YuiMenuContext.Debug.TallItem = TRUE;
    YuiMenuContext.Debug.WidthByStringLength = FALSE;

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
    YuiMenuContext.Run.WidthByStringLength = FALSE;

    if (YuiContext->LoginShell) {
        YuiMenuContext.Exit.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, LOGOFFICON, TRUE);
        YoriLibConstantString(&YuiMenuContext.Exit.Text, _T("Log off"));
    } else {
        YuiMenuContext.Exit.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, EXITICON, TRUE);
        YoriLibConstantString(&YuiMenuContext.Exit.Text, _T("Exit"));
    }
    YuiMenuContext.Exit.TallItem = TRUE;
    YuiMenuContext.Exit.WidthByStringLength = FALSE;

    YuiMenuInitializeItem(&YuiMenuContext.Shutdown);
    YuiMenuContext.Shutdown.Icon = YuiIconCacheCreateOrReference(YuiContext, NULL, SHUTDOWNICON, TRUE);
    YoriLibConstantString(&YuiMenuContext.Shutdown.Text, _T("Shutdown"));
    YuiMenuContext.Shutdown.TallItem = TRUE;
    YuiMenuContext.Shutdown.WidthByStringLength = FALSE;

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

    YuiMenuInitializeItem(&YuiMenuContext.ContextCascade);
    YoriLibConstantString(&YuiMenuContext.ContextCascade.Text, _T("Cascade windows"));

    YuiMenuInitializeItem(&YuiMenuContext.ContextTileSideBySide);
    YoriLibConstantString(&YuiMenuContext.ContextTileSideBySide.Text, _T("Tile windows side by side"));

    YuiMenuInitializeItem(&YuiMenuContext.ContextTileStacked);
    YoriLibConstantString(&YuiMenuContext.ContextTileStacked.Text, _T("Tile windows stacked"));

    YuiMenuInitializeItem(&YuiMenuContext.ContextShowDesktop);
    YoriLibConstantString(&YuiMenuContext.ContextShowDesktop.Text, _T("Show desktop"));

    YuiMenuInitializeItem(&YuiMenuContext.ContextRefreshTaskbar);
    YoriLibConstantString(&YuiMenuContext.ContextRefreshTaskbar.Text, _T("Refresh Taskbar"));

    YuiMenuInitializeItem(&YuiMenuContext.WinContextMinimize);
    YoriLibConstantString(&YuiMenuContext.WinContextMinimize.Text, _T("Minimize window"));

    YuiMenuInitializeItem(&YuiMenuContext.WinContextHide);
    YoriLibConstantString(&YuiMenuContext.WinContextHide.Text, _T("Hide window"));

    YuiMenuInitializeItem(&YuiMenuContext.WinContextClose);
    YoriLibConstantString(&YuiMenuContext.WinContextClose.Text, _T("Close window"));

    YuiMenuInitializeItem(&YuiMenuContext.WinContextTerminateProcess);
    YoriLibConstantString(&YuiMenuContext.WinContextTerminateProcess.Text, _T("Terminate process"));

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
        Current.LengthInChars = (YORI_ALLOC_SIZE_T)(Sep - Current.StartOfString) - 1;
        Sep = YoriLibFindRightMostCharacter(&Current, '\\');
        ASSERT(Sep != NULL);
        if (Sep == NULL) {
            return FALSE;
        }
    }

    YoriLibInitEmptyString(Component);
    Component->StartOfString = Sep + 1;
    Component->LengthInChars = (YORI_ALLOC_SIZE_T)(PreviousSep - Sep) - 1;
    Component->LengthAllocated = Component->LengthInChars;
    Component->MemoryToFree = NULL;

    if (RemoveExtension) {
        Sep = YoriLibFindRightMostCharacter(Component, '.');
        if (Sep != NULL) {
            Component->LengthInChars = (YORI_ALLOC_SIZE_T)(Sep - Component->StartOfString);
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
    Entry->Item.AddFlyoutIcon = TRUE;
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

    //
    //  Even though no flyout is associated with a file, add one so the space
    //  consumed by file and directory text is consistent.
    //

    Entry->Item.AddFlyoutIcon = TRUE;

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
            Ext.LengthInChars = IconPath.LengthInChars - (YORI_ALLOC_SIZE_T)(Ext.StartOfString - IconPath.StartOfString);
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
    BOOLEAN ChildFound;

    ListEntry = NULL;
    NextEntry = NULL;
    EnumContext = (PYUI_CONTEXT)Context;
    ChildFound = FALSE;

    //
    //  Because this enumerates from the deepest to the shallowest, any
    //  child directory has already determined whether it has items to
    //  warrant a popup.  If it doesn't, don't bother including it here.
    //

    ListEntry = YoriLibGetNextListEntry(&Parent->ChildDirectories, ListEntry);
    while (ListEntry != NULL) {
        NextEntry = YoriLibGetNextListEntry(&Parent->ChildDirectories, ListEntry);
        ExistingDir = CONTAINING_RECORD(ListEntry, YUI_MENU_DIRECTORY, ListEntry);
        if (ExistingDir->MenuHandle != NULL) {
            ChildFound = TRUE;
            break;
        }
        ListEntry = NextEntry;
    }


    ListEntry = YoriLibGetNextListEntry(&Parent->ChildFiles, NULL);
    if (ListEntry != NULL) {
        ChildFound = TRUE;
    }

    //
    //  This directory contains no child objects, and there's no need to
    //  display it.
    //

    if (!ChildFound) {
        return TRUE;
    }

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

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&Parent->ChildDirectories, ListEntry);
    while (ListEntry != NULL) {
        NextEntry = YoriLibGetNextListEntry(&Parent->ChildDirectories, ListEntry);
        ExistingDir = CONTAINING_RECORD(ListEntry, YUI_MENU_DIRECTORY, ListEntry);
        if (ExistingDir->MenuHandle != NULL) {
            AppendMenu(Parent->MenuHandle, MF_OWNERDRAW | MF_POPUP, (DWORD_PTR)ExistingDir->MenuHandle, (LPCTSTR)&ExistingDir->Item);
        }
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
 Execute a shortcut.  If it opens successfully, register the process ID as a
 recently opened process in case a taskbar button arrives with the same
 process ID.

 @param YuiMonitor Pointer to the monitor context.  Indirectly this points to
        the application context including a list of recently opened programs;
        the monitor is only used to display error messages.

 @param FilePath Pointer to the shortcut to execute.

 @param Elevated TRUE if the program should be launched elevated, FALSE if
        not.

 @return TRUE to indicate the shortcut was executed, FALSE if it was not.
 */
BOOL
YuiExecuteShortcut(
    __in PYUI_MONITOR YuiMonitor,
    __in PYORI_STRING FilePath,
    __in BOOLEAN Elevated
    )
{
    DWORD ProcessId;
    HRESULT hRes;
    PYUI_CONTEXT YuiContext;

    YuiContext = YuiMonitor->YuiContext;
    hRes = YoriLibExecuteShortcut(FilePath, Elevated, &ProcessId);
    if (!SUCCEEDED(hRes)) {
        YORI_STRING Error;
        YoriLibInitEmptyString(&Error);

        //
        //  Check if it's a Win32 code, which it probably is.
        //

        if (HRESULT_FACILITY(hRes) == FACILITY_WIN32) {
            LPTSTR ErrText;
            ErrText = YoriLibGetWinErrorText(HRESULT_CODE(hRes));
            if (ErrText != NULL) {
                YoriLibYPrintf(&Error, _T("Could not execute shortcut: %s"), ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
        }

        if (Error.StartOfString == NULL) {
            YoriLibYPrintf(&Error, _T("Could not execute shortcut: %08x"), hRes);
        }
        if (Error.StartOfString != NULL) {
            MessageBox(YuiMonitor->hWndTaskbar,
                       Error.StartOfString,
                       _T("Error"),
                       MB_ICONSTOP);
        }
        YoriLibFreeStringContents(&Error);
        return FALSE;
    }

    if (ProcessId != 0) {
        PYUI_RECENT_CHILD_PROCESS ChildProcess;
        ChildProcess = YoriLibReferencedMalloc(sizeof(YUI_RECENT_CHILD_PROCESS) + 
                                               (FilePath->LengthInChars + 1) * sizeof(TCHAR));
        if (ChildProcess != NULL) {
            ChildProcess->TaskbarButtonCount = 0;
            ChildProcess->LastModifiedTime = YoriLibGetSystemTimeAsInteger();
            ChildProcess->ProcessId = ProcessId;
            ChildProcess->Elevated = Elevated;
            YoriLibInitEmptyString(&ChildProcess->FilePath);
            ChildProcess->FilePath.MemoryToFree = ChildProcess;
            YoriLibReference(ChildProcess);
            ChildProcess->FilePath.StartOfString = (LPTSTR)(ChildProcess + 1);
            ChildProcess->FilePath.LengthAllocated = FilePath->LengthInChars + 1;
            memcpy(ChildProcess->FilePath.StartOfString, FilePath->StartOfString, FilePath->LengthInChars * sizeof(TCHAR));
            ChildProcess->FilePath.LengthInChars = FilePath->LengthInChars;
            ChildProcess->FilePath.StartOfString[FilePath->LengthInChars] = '\0';

#if DBG
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Adding new process %x %y\n"), ChildProcess->ProcessId, &ChildProcess->FilePath);
#endif

            YoriLibInsertList(&YuiContext->RecentProcessList, &ChildProcess->ListEntry);
            YuiContext->RecentProcessCount++;
        }
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
    BOOLEAN Elevated;
    PYUI_MENU_FIND_EXEC_CONTEXT FindContext;

    FindContext = (PYUI_MENU_FIND_EXEC_CONTEXT)Context;

    if (FindContext->CtrlId != Item->MenuId) {
        return TRUE;
    }

    Elevated = FALSE;
    if (GetKeyState(VK_SHIFT) < 0) {
        Elevated = TRUE;
    }

    YuiExecuteShortcut(FindContext->YuiMonitor, &Item->FilePath, Elevated);
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
                Ext.LengthInChars = FilePath->LengthInChars - (YORI_ALLOC_SIZE_T)(Ext.StartOfString - FilePath->StartOfString);
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
            Ext.LengthInChars = FilePath->LengthInChars - (YORI_ALLOC_SIZE_T)(Ext.StartOfString - FilePath->StartOfString);
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
            DirName.LengthInChars = (YORI_ALLOC_SIZE_T)(FilePart - DirName.StartOfString);
        } else {
            DirName.LengthInChars = UnescapedFilePath.LengthInChars;
        }
        YoriLibFreeWinErrorText(ErrText);
    }
    YoriLibFreeStringContents(&UnescapedFilePath);
    return Result;
}

/**
 Check if any change notification that is monitoring start menu changes has
 detected a change.  The change notifications are re-queued to detect a 
 subsequent change.

 @param YuiContext Pointer to the context containing the start menu and the
        change notifications that are monitoring it.

 @return TRUE to indicate changes detected, FALSE if no changes detected.
 */
BOOLEAN
YuiMenuCheckForFileSystemChanges(
    __in PYUI_CONTEXT YuiContext
    )
{
    DWORD WaitStatus;
    DWORD HandleCount;
    BOOLEAN ChangeFound;

    ChangeFound = FALSE;

    HandleCount = sizeof(YuiContext->StartChangeNotifications)/sizeof(YuiContext->StartChangeNotifications[0]);
    while(TRUE) {
        WaitStatus = WaitForMultipleObjectsEx(HandleCount,
                                              YuiContext->StartChangeNotifications,
                                              FALSE,
                                              0,
                                              FALSE);
        if (WaitStatus == WAIT_TIMEOUT) {
            return ChangeFound;
        }

#if DBG
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Start menu change detected\n"));
#endif

        ChangeFound = TRUE;

        if (WaitStatus < WAIT_OBJECT_0 + HandleCount) {
            FindNextChangeNotification(YuiContext->StartChangeNotifications[WaitStatus - WAIT_OBJECT_0]);
        }
    }

    //
    //  This can't happen: the loop above never breaks out
    //

    return TRUE;
}

/**
 Start monitoring the start menu directory for changes.

 @param YuiContext Pointer to the application context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YuiMenuMonitorFileSystemChanges(
    __in PYUI_CONTEXT YuiContext
    )
{
    YORI_STRING FullPath;
    YORI_STRING EnumDir;

    ASSERT(YuiContext->StartChangeNotifications[0] == NULL);
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
    return TRUE;
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
    WORD MatchFlags;

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
        AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW | MF_SEPARATOR, 0, (LPCWSTR)&YuiMenuContext.Seperator);
    }

    YuiForEachFileOrDirectoryDepthFirst(&YuiMenuContext.ProgramsDirectory,
                                        YuiContext,
                                        NULL,
                                        YuiPopulateMenuOnDirectory);

    YuiContext->SystemMenu = CreatePopupMenu();
    if (YuiContext->SystemMenu == NULL) {
        return FALSE;
    }

    AppendMenu(YuiContext->SystemMenu, MF_OWNERDRAW, YUI_MENU_SYSTEM_WIFI, (LPCWSTR)&YuiMenuContext.SystemWifi);
    AppendMenu(YuiContext->SystemMenu, MF_OWNERDRAW, YUI_MENU_SYSTEM_MIXER, (LPCWSTR)&YuiMenuContext.SystemMixer);
    AppendMenu(YuiContext->SystemMenu, MF_OWNERDRAW, YUI_MENU_SYSTEM_CONTROL, (LPCWSTR)&YuiMenuContext.SystemControlPanel);
    AppendMenu(YuiContext->SystemMenu, MF_OWNERDRAW, YUI_MENU_SYSTEM_TASKMGR, (LPCWSTR)&YuiMenuContext.SystemTaskMgr);
    AppendMenu(YuiContext->SystemMenu, MF_OWNERDRAW, YUI_MENU_SYSTEM_CMD, (LPCWSTR)&YuiMenuContext.SystemCmd);

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
    AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW | MF_POPUP, (DWORD_PTR)YuiContext->SystemMenu, (LPCWSTR)&YuiMenuContext.System);
#if DBG
    AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW | MF_POPUP, (DWORD_PTR)YuiContext->DebugMenu, (LPCWSTR)&YuiMenuContext.Debug);
#endif
    AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW, YUI_MENU_RUN, (LPCWSTR)&YuiMenuContext.Run);
    AppendMenu(YuiContext->StartMenu, MF_OWNERDRAW | MF_SEPARATOR, 0, (LPCWSTR)&YuiMenuContext.Seperator);
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
    AppendMenu(YuiContext->ShutdownMenu, MF_OWNERDRAW | MF_SEPARATOR, 0, (LPCWSTR)&YuiMenuContext.Seperator);
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

    if (YuiContext->SystemMenu != NULL) {
        DestroyMenu(YuiContext->SystemMenu);
        YuiContext->SystemMenu = NULL;
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
 Populate the menu from within a background thread.

 @param lpParameter Pointer to the global application context containing the
        start menu.

 @return Thread exit value, not meaningful here.
 */
DWORD WINAPI
YuiMenuPopulateWorker(
    __in LPVOID lpParameter
    )
{
    PYUI_CONTEXT YuiContext = (PYUI_CONTEXT)lpParameter;
    YuiMenuPopulate(YuiContext);
    return 0;
}

/**
 Create a background thread to populate the menu, including loading any icons
 for start menu entries.  This allows the task bar to be displayed while the
 menu is still being populated.

 @param YuiContext Pointer to the context containing the start menu.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiMenuPopulateInBackground(
    __in PYUI_CONTEXT YuiContext
    )
{
    HANDLE ThreadHandle;
    DWORD ThreadId;

    if (YuiContext->MenuPopulateThread != NULL) {
        return FALSE;
    }

    ThreadHandle = CreateThread(NULL, 0, YuiMenuPopulateWorker, YuiContext, 0, &ThreadId);
    YuiContext->MenuPopulateThread = ThreadHandle;
    return TRUE;
}

/**
 Wait for any background thread populating the start menu to complete.
 Because this thread is initiated and waited upon from the main thread, this
 can check for the existence of the thread and avoid waiting if it is not
 present.

 @param YuiContext Pointer to the context containing the start menu.
 */
VOID
YuiMenuWaitForBackgroundReload(
    __in PYUI_CONTEXT YuiContext
    )
{
    if (YuiContext->MenuPopulateThread == NULL) {
        return;
    }

    WaitForSingleObject(YuiContext->MenuPopulateThread, INFINITE);
    CloseHandle(YuiContext->MenuPopulateThread);
    YuiContext->MenuPopulateThread = NULL;
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
    BOOLEAN FoundChange;

    FoundChange = FALSE;
#if DBG
    if (YuiContext->DebugMenuItemChecked != YuiContext->DebugLogEnabled) {
        FoundChange = TRUE;
    }
#endif

    if (YuiMenuCheckForFileSystemChanges(YuiContext)) {
        FoundChange = TRUE;
    }

    if (!FoundChange) {
        return TRUE;
    }

    YuiMenuFreeAll(YuiContext);
    return YuiMenuPopulate(YuiContext);
}

/**
 A callback function invoked for all windows found in the system which may
 minimize a window in order to show the desktop.

 @param hWnd The window to evaluate and possibly minimize.

 @param lParam Pointer to the application context.

 @return TRUE to continue enumerating, FALSE to terminate.
 */
BOOL WINAPI
YuiShowDesktopCallback(
    __in HWND hWnd,
    __in LPARAM lParam
    )
{
    PYUI_CONTEXT YuiContext;
    DWORD WinStyle;
    DWORD ExStyle;
    PYUI_MONITOR YuiMonitor;

    YuiContext = (PYUI_CONTEXT)lParam;

    if (!IsWindowVisible(hWnd)) {
        return TRUE;
    }

    if (!IsWindowEnabled(hWnd)) {
        return TRUE;
    }

    if (GetWindow(hWnd, GW_OWNER) != NULL) {
        return TRUE;
    }

    if (IsIconic(hWnd)) {
        return TRUE;
    }

    WinStyle = GetWindowLong(hWnd, GWL_STYLE);
    ExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

    if ((ExStyle & WS_EX_TOOLWINDOW) != 0) {
        return TRUE;
    }

    if ((WinStyle & WS_SYSMENU) == 0) {
        return TRUE;
    }

    //
    //  Look for a taskbar on any monitor matching this hWnd.  No taskbar
    //  should be hidden as part of this operation.
    //

    YuiMonitor = YuiMonitorFromTaskbarHwnd(YuiContext, hWnd);
    if (YuiMonitor != NULL) {
        return TRUE;
    }

    if (DllUser32.pShowWindowAsync != NULL) {
        DllUser32.pShowWindowAsync(hWnd, SW_SHOWMINNOACTIVE);
    } else {
        ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
    }

    return TRUE;
}

/**
 Show the desktop, aka, minimize all windows on the desktop.

 @param YuiContext Pointer to the application context.
 */
VOID
YuiShowDesktop(
    __in PYUI_CONTEXT YuiContext
    )
{
    EnumWindows(YuiShowDesktopCallback, (LPARAM)YuiContext);
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
PYUI_MONITOR RunDlgContext;

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
            RunDlgContext = (PYUI_MONITOR)lParam;
            if (DllShell32.pShellExecuteW == NULL) {
                return FALSE;
            }

            return TRUE;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
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
                            YORI_ALLOC_SIZE_T Length;
                            HINSTANCE hInst;
                            DWORD Err;

                            EnableWindow(GetDlgItem(hDlg, IDC_OK), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_CANCEL), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_RUNCMD), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_BROWSE), FALSE);

                            Length = (YORI_ALLOC_SIZE_T)GetWindowTextLength(GetDlgItem(hDlg, IDC_RUNCMD));
                            if (YoriLibAllocateString(&Cmd, Length + 1)) {
                                PYORI_STRING ArgV;
                                YORI_ALLOC_SIZE_T Index;
                                YORI_ALLOC_SIZE_T ArgC;
                                LPTSTR ArgString;

                                Cmd.LengthInChars = (YORI_ALLOC_SIZE_T)GetDlgItemText(hDlg, IDC_RUNCMD, Cmd.StartOfString, Cmd.LengthAllocated);

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
    }

    return FALSE;
}

/**
 Display the run dialog box to allow the user to launch a custom program.

 @param YuiMonitor Pointer to the monitor context.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiMenuRun(
    __in PYUI_MONITOR YuiMonitor
    )
{
    DialogBoxParam(NULL,
                   MAKEINTRESOURCE(RUNDIALOG),
                   YuiMonitor->hWndTaskbar,
                   (DLGPROC)RunDialogProc,
                   (DWORD_PTR)YuiMonitor);
    return TRUE;
}

/**
 Check if a specified file exists in the system directory.

 @param FileName Pointer to the file name to check for.

 @return TRUE to indicate the file exists, FALSE if it does not.
 */
BOOLEAN
YuiMenuCheckIfSystemProgramExists(
    __in LPCTSTR FileName
    )
{
    YORI_STRING FullPath;
    YORI_STRING YsFileName;
    DWORD Attributes;

    YoriLibConstantString(&YsFileName, FileName);
    if (!YoriLibFullPathToSystemDirectory(&YsFileName, &FullPath)) {
        return FALSE;
    }

    Attributes = GetFileAttributes(FullPath.StartOfString);
    YoriLibFreeStringContents(&FullPath);
    if (Attributes != (DWORD)-1) {
        return TRUE;
    }

    return FALSE;
}

/**
 Take a string literal for a program name, and append it to the System32
 directory, then launch the result as a fully qualified path.

 @param FileName Pointer to a NULL terminated file name to launch.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiMenuExecuteSystemProgram(
    __in LPCTSTR FileName
    )
{
    YORI_STRING FullPath;
    YORI_STRING YsFileName;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL Result;

    YoriLibConstantString(&YsFileName, FileName);
    if (!YoriLibFullPathToSystemDirectory(&YsFileName, &FullPath)) {
        return FALSE;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    Result = CreateProcess(NULL,
                           FullPath.StartOfString,
                           NULL,
                           NULL,
                           FALSE,
                           CREATE_NEW_PROCESS_GROUP | CREATE_DEFAULT_ERROR_MODE,
                           NULL,
                           NULL,
                           &si,
                           &pi);

    if (Result) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    YoriLibFreeStringContents(&FullPath);

    return Result;
}

/**
 Execute the item selected from the start menu.

 @param YuiMonitor Pointer to the monitor context.

 @param MenuId The identifier of the menu item that the user selected.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiMenuExecuteById(
    __in PYUI_MONITOR YuiMonitor,
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
            YuiMenuRun(YuiMonitor);
            break;
        case YUI_MENU_SYSTEM_WIFI:
            YuiWifi(YuiMonitor);
            break;
        case YUI_MENU_SYSTEM_MIXER:
            if (YuiMenuCheckIfSystemProgramExists(_T("SndVol.exe"))) {
                YuiMenuExecuteSystemProgram(_T("SndVol.exe"));
            } else if (YuiMenuCheckIfSystemProgramExists(_T("SndVol32.exe"))) {
                YuiMenuExecuteSystemProgram(_T("SndVol32.exe"));
            }
            break;
        case YUI_MENU_SYSTEM_CONTROL:
            YuiMenuExecuteSystemProgram(_T("Control.exe"));
            break;
        case YUI_MENU_SYSTEM_TASKMGR:
            YuiMenuExecuteSystemProgram(_T("TaskMgr.exe"));
            break;
        case YUI_MENU_SYSTEM_CMD:
            YuiMenuExecuteSystemProgram(_T("Cmd.exe"));
            break;
        case YUI_MENU_REFRESH:
            YuiRefreshMonitors(YuiMonitor->YuiContext);
            break;
#if DBG
        case YUI_MENU_LOGGING:
            YuiMenuToggleLogging(YuiMonitor->YuiContext);
            break;
        case YUI_MENU_LAUNCHWINLOGONSHELL:
            YuiExitAndLaunchWinlogonShell(YuiMonitor->YuiContext);
            break;
        case YUI_MENU_DISPLAYCHANGE:
            YuiNotifyResolutionChange(YuiMonitor);
            break;
#endif
        case YUI_MENU_CASCADE:
            if (DllUser32.pCascadeWindows != NULL) {
                DllUser32.pCascadeWindows(NULL,
                                          0,
                                          NULL,
                                          0,
                                          NULL);
            }
            break;
        case YUI_MENU_TILE_SIDEBYSIDE:
            if (DllUser32.pTileWindows != NULL) {
                DllUser32.pTileWindows(NULL,
                                       MDITILE_VERTICAL,
                                       NULL,
                                       0,
                                       NULL);
            }
            break;
        case YUI_MENU_TILE_STACKED:
            if (DllUser32.pTileWindows != NULL) {
                DllUser32.pTileWindows(NULL,
                                       MDITILE_HORIZONTAL,
                                       NULL,
                                       0,
                                       NULL);
            }
            break;

        case YUI_MENU_SHOW_DESKTOP:
            YuiShowDesktop(YuiMonitor->YuiContext);
            break;

        default:
            ASSERT(MenuId >= YUI_MENU_FIRST_PROGRAM_MENU_ID);

            {
                YUI_MENU_FIND_EXEC_CONTEXT FindContext;
                FindContext.YuiMonitor = YuiMonitor;
                FindContext.CtrlId = MenuId;
                FindContext.ProcessId = 0;

                if (YuiForEachFileOrDirectoryDepthFirst(&YuiMenuContext.StartDirectory,
                                                        &FindContext,
                                                        YuiFindMenuCommandToExecute,
                                                        NULL)) {
                    YuiForEachFileOrDirectoryDepthFirst(&YuiMenuContext.ProgramsDirectory,
                                                        &FindContext,
                                                        YuiFindMenuCommandToExecute,
                                                        NULL);
                }
            }
    }

    return TRUE;
}

/**
 Return the flags to use when calling TrackPopupMenu.  Some flags were added
 in later versions of Windows that are important, but cause earlier versions
 to fail completely.

 @return The flags to pass to TrackPopupMenu.
 */
DWORD
YuiMenuGetPopupMenuFlags(VOID)
{
    DWORD OsMajor;
    DWORD OsMinor;
    DWORD BuildNumber;
    DWORD Flags;

    YoriLibGetOsVersion(&OsMajor, &OsMinor, &BuildNumber);
    Flags = TPM_RETURNCMD | TPM_BOTTOMALIGN;
    if (OsMajor >= 5) {
        Flags = Flags | TPM_NOANIMATION | TPM_RECURSE;
    }
    return Flags;
}

/**
 Display the start menu and execute any item selected.

 @param YuiMonitor Pointer to the monitor context.

 @param hWnd The handle of the taskbar window.

 @return TRUE if the menu was displayed successfully, FALSE if not.
 */
BOOL
YuiMenuDisplayAndExecute(
    __in PYUI_MONITOR YuiMonitor,
    __in HWND hWnd
    )
{
    RECT WindowRect;
    DWORD MenuId;
    PYUI_CONTEXT YuiContext;

    YuiContext = YuiMonitor->YuiContext;
    YuiMenuWaitForBackgroundReload(YuiContext);

    if (!YuiMenuReloadIfChanged(YuiContext)) {
        return FALSE;
    }

    DllUser32.pGetWindowRect(hWnd, &WindowRect);

    //
    //  We need to make the taskbar the foreground so if the foreground
    //  changes the menu is dismissed.
    //

    DllUser32.pSetForegroundWindow(hWnd);

    MenuId = TrackPopupMenu(YuiContext->StartMenu,
                            YuiMenuGetPopupMenuFlags(),
                            WindowRect.left,
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
        SendMessage(YuiMonitor->hWndStart, BM_SETSTATE, FALSE, 0);
    }

    if (MenuId > 0) {
        YuiMenuExecuteById(YuiMonitor, MenuId);
    } else {
        YuiTaskbarSwitchToActiveTask(YuiContext);
    }

    return TRUE;
}

/**
 Display a menu corresponding to a right click on the taskbar.

 @param YuiMonitor Pointer to the monitor context.

 @param hWnd The handle of the taskbar window.

 @param CursorX The horizontal position of the mouse cursor in screen
        coordinates.

 @param CursorY The vertical position of the mouse cursor in screen
        coordinates.

 @return TRUE to indicate an action was invoked, FALSE if it was not.
 */
BOOL
YuiMenuDisplayContext(
    __in PYUI_MONITOR YuiMonitor,
    __in HWND hWnd,
    __in DWORD CursorX,
    __in DWORD CursorY
    )
{
    HMENU Menu;
    DWORD MenuId;

    Menu = CreatePopupMenu();

    AppendMenu(Menu, MF_OWNERDRAW, YUI_MENU_CASCADE, (LPCWSTR)&YuiMenuContext.ContextCascade);
    AppendMenu(Menu, MF_OWNERDRAW, YUI_MENU_TILE_SIDEBYSIDE, (LPCWSTR)&YuiMenuContext.ContextTileSideBySide);
    AppendMenu(Menu, MF_OWNERDRAW, YUI_MENU_TILE_STACKED, (LPCWSTR)&YuiMenuContext.ContextTileStacked);
    AppendMenu(Menu, MF_OWNERDRAW, YUI_MENU_SHOW_DESKTOP, (LPCWSTR)&YuiMenuContext.ContextShowDesktop);
    AppendMenu(Menu, MF_OWNERDRAW | MF_SEPARATOR, 0, (LPCWSTR)&YuiMenuContext.Seperator);
    AppendMenu(Menu, MF_OWNERDRAW, YUI_MENU_REFRESH, (LPCWSTR)&YuiMenuContext.ContextRefreshTaskbar);

    //
    //  We need to make the taskbar the foreground so if the foreground
    //  changes the menu is dismissed.
    //

    DllUser32.pSetForegroundWindow(hWnd);

    MenuId = TrackPopupMenu(Menu,
                            YuiMenuGetPopupMenuFlags(),
                            CursorX,
                            CursorY,
                            0,
                            hWnd,
                            NULL);

    PostMessage(hWnd, WM_NULL, 0, 0);
    DestroyMenu(Menu);

    if (MenuId > 0) {
        YuiMenuExecuteById(YuiMonitor, MenuId);
        return TRUE;
    }

    return FALSE;
}

/**
 Execute the item selected from window context menu.

 @param YuiMonitor Pointer to the monitor context.

 @param MenuId The identifier of the menu item that the user selected.

 @param hWnd The window handle corresponding to the window being acted upon.

 @param dwProcessId The process ID corresponding to the window being acted
        upon.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiMenuExecuteWindowContextById(
    __in PYUI_MONITOR YuiMonitor,
    __in DWORD MenuId,
    __in HWND hWnd,
    __in DWORD dwProcessId
    )
{
    UNREFERENCED_PARAMETER(YuiMonitor);

    switch(MenuId) {
        case YUI_MENU_MINIMIZEWINDOW:
            CloseWindow(hWnd);
            break;
        case YUI_MENU_HIDEWINDOW:
            if (DllUser32.pShowWindowAsync != NULL) {
                DllUser32.pShowWindowAsync(hWnd, SW_HIDE);
            } else {
                ShowWindow(hWnd, SW_HIDE);
            }
            break;
        case YUI_MENU_CLOSEWINDOW:
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        case YUI_MENU_TERMINATEPROCESS:
            {
                HANDLE hProcess;
                hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 255);
                    CloseHandle(hProcess);
                }
            }
            break;
        default:
            ASSERT(FALSE);
    }

    return TRUE;
}

/**
 Display a menu corresponding to a right click on a taskbar button.

 @param YuiMonitor Pointer to the monitor context.

 @param hWnd The handle of the taskbar window.

 @param hWndApp The window handle that is being right clicked on.

 @param dwProcessId The process ID that is being right clicked on.

 @param CursorX The horizontal position of the mouse cursor in screen
        coordinates.

 @param CursorY The vertical position of the mouse cursor in screen
        coordinates.

 @return TRUE to indicate an action was invoked, FALSE if it was not.
 */
BOOL
YuiMenuDisplayWindowContext(
    __in PYUI_MONITOR YuiMonitor,
    __in HWND hWnd,
    __in HWND hWndApp,
    __in DWORD dwProcessId,
    __in DWORD CursorX,
    __in DWORD CursorY
    )
{
    HMENU Menu;
    DWORD MenuId;

    Menu = CreatePopupMenu();

    AppendMenu(Menu, MF_OWNERDRAW, YUI_MENU_MINIMIZEWINDOW, (LPCWSTR)&YuiMenuContext.WinContextMinimize);
    AppendMenu(Menu, MF_OWNERDRAW, YUI_MENU_HIDEWINDOW, (LPCWSTR)&YuiMenuContext.WinContextHide);
    AppendMenu(Menu, MF_OWNERDRAW, YUI_MENU_CLOSEWINDOW, (LPCWSTR)&YuiMenuContext.WinContextClose);
    AppendMenu(Menu, MF_OWNERDRAW, YUI_MENU_TERMINATEPROCESS, (LPCWSTR)&YuiMenuContext.WinContextTerminateProcess);

    //
    //  We need to make the taskbar the foreground so if the foreground
    //  changes the menu is dismissed.
    //

    DllUser32.pSetForegroundWindow(hWnd);

    MenuId = TrackPopupMenu(Menu,
                            YuiMenuGetPopupMenuFlags(),
                            CursorX,
                            CursorY,
                            0,
                            hWnd,
                            NULL);

    PostMessage(hWnd, WM_NULL, 0, 0);
    DestroyMenu(Menu);

    if (MenuId > 0) {
        YuiMenuExecuteWindowContextById(YuiMonitor, MenuId, hWndApp, dwProcessId);
        return TRUE;
    }

    return FALSE;
}

// vim:sw=4:ts=4:et:
