/**
 * @file yui/yui.h
 *
 * Yori shell display lightweight graphical UI master header
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
     The name of this directory.  This is the final path component only, not
     a fully qualified path.
     */
    YORI_STRING DirName;

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
     A fully qualified path to this file (typically a .lnk file.)
     */
    YORI_STRING FilePath;

    /**
     The name to display for this file within the start menu.
     */
    YORI_STRING FriendlyName;

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
 In memory state corresponding to a taskbar button.
 */
typedef struct _YUI_TASKBAR_BUTTON {

    /**
     The entry for this taskbar button within the list of taskbar buttons.
     Paired with @ref YUI_CONTEXT::TaskbarButtons .
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The window handle for the button control for this taskbar button.
     */
    HWND hWndButton;

    /**
     The window to activate when this taskbar button is clicked.
     */
    HWND hWndToActivate;

    /**
     The identifier of the button control.
     */
    WORD ControlId;

    /**
     The left offset of the button, in pixels, relative to the client area.
     This is used to detect clicks that are in the parent window outside of
     the button area.
     */
    WORD LeftOffset;

    /**
     The right offset of the button, in pixels, relative to the client area.
     This is used to detect clicks that are in the parent window outside of
     the button area.
     */
    WORD RightOffset;

    /**
     TRUE if the button is the currently selected button, indicating the
     taskbar believes this window to be active.
     */
    BOOLEAN WindowActive;

    /**
     TRUE if this entry has been located when syncing the current set of
     windows with the current set of taskbar buttons.
     */
    BOOLEAN AssociatedWindowFound;

    /**
     TRUE if the associated window is requesting attention.  This will be
     cleared as soon as the window is activated, via any mechanism.
     */
    BOOLEAN Flashing;

    /**
     The text to display on the taskbar button.
     */
    YORI_STRING ButtonText;
} YUI_TASKBAR_BUTTON, *PYUI_TASKBAR_BUTTON;

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _YUI_CONTEXT {

    /**
     The directory object corresponding to the top level start menu directory.
     */
    YUI_MENU_DIRECTORY StartDirectory;

    /**
     The directory object corresponding to the programs directory.
     */
    YUI_MENU_DIRECTORY ProgramsDirectory;

    /**
     The directory to filter from enumerate.  This is the "Programs" directory
     when enumerating the start menu directory.  Because this is typically
     nested, but not required to be nested, we enumerate both and filter any
     matches from the start menu directory that overlap with the programs
     directory.  If this string is not empty, it implies that the start menu
     directory is being enumerated, which also implies different rules for
     where found files should be placed.
     */
    YORI_STRING FilterDirectory;

    /**
     Change notification handles to detect if the contents of the start menu
     have changed.  There are four of these, being per-user and per-system as
     well as Programs and Start Menu directories.
     */
    HANDLE StartChangeNotifications[4];

    /**
     The next identifier to allocate for subsequent menu entries.
     */
    DWORD NextMenuIdentifier;

    /**
     The minimized window metrics to restore on exit.  Only meaningful if
     cbSize is nonzero.
     */
    YORI_MINIMIZEDMETRICS SavedMinimizedMetrics;

    /**
     The width of the screen, in pixels.
     */
    DWORD ScreenWidth;

    /**
     The height of the screen, in pixels.
     */
    DWORD ScreenHeight;

    /**
     The default window procedure for a push button.  Stored here so we can
     override it and call it recursively.
     */
    WNDPROC DefaultButtonWndProc;

    /**
     Handle to an icon to display on the start button.
     */
    HICON StartIcon;

    /**
     The window handle for the taskbar.
     */
    HWND hWnd;

    /**
     The window handle for the "Desktop."  This isn't really a desktop in the
     user32 sense; it's a window created by this application for the purpose
     of rendering a background color.  Newer versions of Windows won't do this
     automatically on the real desktop.
     */
    HWND hWndDesktop;

    /**
     The window handle for the start button.
     */
    HWND hWndStart;

    /**
     The window handle for the clock.
     */
    HWND hWndClock;

    /**
     The window handle for the battery indicator.
     */
    HWND hWndBattery;

    /**
     A window handle hidden on program start describing the explorer taskbar.
     If non-NULL, this can be displayed again on exit.
     */
    HWND hWndExplorerTaskbar;

    /**
     The message identifier used to communicate shell hook messages.  This is
     only meaningful when using SetWindowsHookEx to monitor changes.
     */
    DWORD ShellHookMsg;

    /**
     A handle to a font used to display buttons on the task bar.
     */
    HFONT hFont;

    /**
     A handle to a bold font used to display buttons on the task bar.
     */
    HFONT hBoldFont;

    /**
     The top level menu handle for the start menu.
     */
    HMENU StartMenu;

    /**
     The menu handle for the nested debug menu.
     */
    HMENU DebugMenu;

    /**
     The menu handle for the nested shutdown menu.
     */
    HMENU ShutdownMenu;

    /**
     The list of currently known taskbar buttons.
     */
    YORI_LIST_ENTRY TaskbarButtons;

    /**
     An identifier for a periodic timer used to refresh taskbar buttons.  This
     is only meaningful when polling is used to sync taskbar state.
     */
    DWORD_PTR SyncTimerId;

    /**
     An identifier for a periodic timer used to update the clock.
     */
    DWORD_PTR ClockTimerId;

    /**
     The string containing the current value of the clock display.  It is only
     updated if the value changes.
     */
    YORI_STRING ClockDisplayedValue;

    /**
     The buffer containing the current displayed clock value.
     */
    TCHAR ClockDisplayedValueBuffer[16];

    /**
     The string containing the current value of the battery display.  It is only
     updated if the value changes.
     */
    YORI_STRING BatteryDisplayedValue;

    /**
     The buffer containing the current displayed battery value.
     */
    TCHAR BatteryDisplayedValueBuffer[16];

    /**
     The left offset of the start button, in pixels, relative to the client
     area.  This is used to detect clicks that are in the parent window
     outside of the button area.
     */
    WORD StartLeftOffset;

    /**
     The right offset of the start button, in pixels, relative to the client
     area.  This is used to detect clicks that are in the parent window
     outside of the button area.
     */
    WORD StartRightOffset;

    /**
     The number of buttons currently displayed in the task bar.
     */
    DWORD TaskbarButtonCount;

    /**
     The offset in pixels from the beginning of the taskbar window to the
     first task button.  This is to allow space for the start button.
     */
    DWORD LeftmostTaskbarOffset;

    /**
     The offset in pixels from the end of the taskbar window to the
     last task button.  This is to allow space for the clock.
     */
    DWORD RightmostTaskbarOffset;

    /**
     The next control ID to allocate for the next taskbar button.
     */
    WORD NextTaskbarId;

    /**
     The maximum width for a taskbar button in pixels.  This scales up a
     little with monitor size.  Taskbar buttons can always be less than
     this value once the bar becomes full.
     */
    WORD MaximumTaskbarButtonWidth;

    /**
     A timer frequency of how often to poll for window changes to refresh
     the taskbar.  Generally notifications are used instead and this value
     is zero.  A nonzero value implies the user forced a value.
     */
    DWORD TaskbarRefreshFrequency;

    /**
     Set to TRUE if this instance of Yui is the master login shell for the
     session.  Set to FALSE if another shell is detected.
     */
    BOOLEAN LoginShell;

    /**
     Set to TRUE if a display resolution change message is being processed.
     This happens because moving an app bar triggers a display resolution
     change, and we don't want to recurse infinitely.
     */
    BOOLEAN DisplayResolutionChangeInProgress;

    /**
     Set to TRUE if a menu is being displayed.  Opening a dialog under the
     menu can cause a start click to be re-posted, so this is used to
     prevent recursing indefinitely.
     */
    BOOLEAN MenuActive;

    /**
     Set to TRUE if the program has an active console for the purpose of
     logging debug messages.  Set to FALSE if one is not active.
     */
    BOOLEAN DebugLogEnabled;

    /**
     Set to TRUE if the check mark is displayed on the debug log menu item.
     If this does not equal DebugLogEnabled above, the menu is recalculated.
     */
    BOOLEAN DebugMenuItemChecked;

    /**
     Set to TRUE if on exit Yui should launch the current winlogon shell.
     This is a debug facility to allow the shell to change without needing
     to log on and off.
     */
    BOOLEAN LaunchWinlogonShell;

    /**
     Set to TRUE if a fullscreen window is active, meaning the taskbar is
     hidden.  Set to FALSE if the taskbar is visible.
     */
    BOOLEAN FullscreenModeActive;

    /**
     Set to TRUE if RegisterHotKey has succeeded for the run command.  This
     is only attempted if LoginShell is TRUE and may still fail if another
     application is handling it.
     */
    BOOLEAN RunHotKeyRegistered;

    /**
     Set to TRUE if the application has successfully registered for session
     change notifications (implying it should unregister on exit.)
     */
    BOOLEAN RegisteredSessionNotifications;

    /**
     Set to TRUE if battery status is displayed on the task bar.
     */
    BOOLEAN DisplayBattery;

} YUI_CONTEXT, *PYUI_CONTEXT;

/**
 The number of pixels to include in the start button.
 */
#define YUI_START_BUTTON_WIDTH (75)

/**
 The number of pixels to include in the clock.
 */
#define YUI_CLOCK_WIDTH (80)

/**
 The number of pixels to include in the battery indicator.
 */
#define YUI_BATTERY_WIDTH (45)

/**
 The control identifier for the start button.
 */
#define YUI_START_BUTTON (1)

/**
 The control identifier for the battery display.
 */
#define YUI_BATTERY_DISPLAY (2)

/**
 The control identifier for the clock.
 */
#define YUI_CLOCK_DISPLAY (3)

/**
 The control identifier for the first taskbar button.  Later taskbar buttons
 have identifiers higher than this one.
 */
#define YUI_FIRST_TASKBAR_BUTTON (100)

/**
 The timer identifier of the timer that polls for window change events on
 systems that do not support notifications.
 */
#define YUI_WINDOW_POLL_TIMER (1)

/**
 The timer identifier of the timer that updates the clock in the task bar.
 */
#define YUI_CLOCK_TIMER (2)

/**
 The first identifier for a user program within the start menu.  Other
 programs will have an identifier higher than this one.
 */
#define YUI_MENU_FIRST_PROGRAM_MENU_ID (100)

/**
 An identifier for the menu item to exit the program.
 */
#define YUI_MENU_EXIT                  (1)

/**
 An identifier for the menu item to disconnect the session.
 */
#define YUI_MENU_DISCONNECT            (10)

/**
 An identifier for the menu item to lock the session.
 */
#define YUI_MENU_LOCK                  (11)

/**
 An identifier for the menu item to log off the session.
 */
#define YUI_MENU_LOGOFF                (12)

/**
 An identifier for the menu item to reboot the system.
 */
#define YUI_MENU_REBOOT                (13)

/**
 An identifier for the menu item to shut down the system.
 */
#define YUI_MENU_SHUTDOWN              (14)

/**
 An identifier for the menu item to run a program.
 */
#define YUI_MENU_RUN                   (20)

/**
 An identifier for the menu item to refresh the taskbar.
 */
#define YUI_MENU_REFRESH               (30)

/**
 An identifier for the menu item to enable or disable logging.
 */
#define YUI_MENU_LOGGING               (31)

/**
 An identifier for the menu item to launch Winlogon shell and exit.
 */
#define YUI_MENU_LAUNCHWINLOGONSHELL   (32)

/**
 An identifier for the menu item to simulate a display change and update
 taskbar position.
 */
#define YUI_MENU_DISPLAYCHANGE         (33)

WNDPROC
YuiGetDefaultButtonWndProc(VOID);

BOOL
YuiNotifyResolutionChange(
    __in HWND hWnd
    );

BOOL
YuiMenuPopulate(
    __in PYUI_CONTEXT YuiContext
    );

BOOL
YuiMenuDisplayAndExecute(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    );

VOID
YuiMenuFreeAll(
    __in PYUI_CONTEXT YuiContext
    );

BOOL
YuiMenuExecuteById(
    __in PYUI_CONTEXT YuiContext,
    __in DWORD MenuId
    );

VOID
YuiDrawButton(
    __in PDRAWITEMSTRUCT DrawItemStruct,
    __in BOOLEAN Pushed,
    __in BOOLEAN Flashing,
    __in_opt HICON Icon,
    __in PYORI_STRING Text,
    __in BOOLEAN CenterText
    );

BOOL
YuiTaskbarPopulateWindows(
    __in PYUI_CONTEXT YuiContext,
    __in HWND TaskbarHwnd
    );

VOID
YuiTaskbarSwitchToTask(
    __in PYUI_CONTEXT YuiContext,
    __in DWORD CtrlId
    );

VOID
YuiTaskbarSwitchToActiveTask(
    __in PYUI_CONTEXT YuiContext
    );

VOID
YuiTaskbarNotifyResolutionChange(
    __in PYUI_CONTEXT YuiContext
    );

VOID
YuiTaskbarNotifyNewWindow(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    );

VOID
YuiTaskbarNotifyDestroyWindow(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    );

VOID
YuiTaskbarNotifyActivateWindow(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    );

VOID
YuiTaskbarNotifyTitleChange(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    );

VOID
YuiTaskbarNotifyFlash(
    __in PYUI_CONTEXT YuiContext,
    __in HWND hWnd
    );

WORD
YuiTaskbarFindByOffset(
    __in PYUI_CONTEXT YuiContext,
    __in SHORT XPos
    );

VOID
YuiTaskbarDrawButton(
    __in PYUI_CONTEXT YuiContext,
    __in DWORD CtrlId,
    __in PDRAWITEMSTRUCT DrawItemStruct
    );

VOID
YuiTaskbarFreeButtons(
    __in PYUI_CONTEXT YuiContext
    );

VOID
YuiTaskbarSyncWithCurrent(
    __in PYUI_CONTEXT YuiContext
    );

VOID
YuiClockDisplayInfo(
    __in PYUI_CONTEXT YuiContext
    );

VOID
YuiClockDisplayBatteryInfo(
    __in PYUI_CONTEXT YuiContext
    );

VOID
YuiClockUpdate(
    __in PYUI_CONTEXT YuiContext
    );

// vim:sw=4:ts=4:et:
