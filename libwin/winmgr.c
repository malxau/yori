/**
 * @file libwin/winmgr.c
 *
 * Yori manage multiple overlapping windows
 *
 * Copyright (c) 2019-2020 Malcolm J. Smith
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
#include "yoriwin.h"

#include "winpriv.h"

/**
 A timer that can be attached to the window manager.
 */
typedef struct _YORI_WIN_TIMER {

    /**
     An entry for this timer within the window manager's timer list.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     The control to notify on a timer tick.
     */
    PYORI_WIN_CTRL NotifyCtrl;

    /**
     The next time (in system time terms) when the timer should be invoked.
     This is reevaluated based on start time and tick length below.
     */
    LONGLONG ExpirationTime;

    /**
     The time of the system when the timer was first created.
     */
    LONGLONG PeriodicStartTime;

    /**
     The time between each timer tick.
     */
    DWORD    PeriodicIntervalInMs;

    /**
     The number of times the timer has already ticked.
     */
    DWORD    PeriodsExpired;
} YORI_WIN_TIMER, *PYORI_WIN_TIMER;

/**
 A structure describing a window manager
 */
typedef struct _YORI_WIN_WINDOW_MANAGER {

    /**
     A handle to the output for the console
     */
    HANDLE hConOut;

    /**
     A handle to the input from the console
     */
    HANDLE hConIn;

    /**
     A handle to the original console screen buffer, if screen buffers
     have been switched.
     */
    HANDLE hConOriginal;

    /**
     A stack of top level windows that are processing events.  The head of
     the list is the topmost window, and the tail is the bottom most.
     */
    YORI_LIST_ENTRY TopLevelWindowList;

    /**
     The list of timers active in the window manager.
     */
    YORI_LIST_ENTRY TimerList;

    /**
     Information about the cursor from when the window manager was started.
     */
    CONSOLE_CURSOR_INFO SavedCursorInfo;

    /**
     Information about the screen buffer from when the window manager was
     started.
     */
    CONSOLE_SCREEN_BUFFER_INFO SavedScreenBufferInfo;

    /**
     The color table of default control colors to use.
     */
    YORI_WIN_COLOR_TABLE_HANDLE ColorTable;

    /**
     The mouse buttons that were pressed last time a mouse event was
     processed.  This is used to detect which buttons were pressed or
     released.
     */
    DWORD PreviousMouseButtonState;

    /**
     Set to TRUE if the console is a v2 console.  These appear to have fixed
     bugs relating to the coordinates of mouse wheel events.  As of this
     writing, mouse wheel is only supported here on v2 - supporting on v1
     would require some manual fudging of coordinates.
     */
    BOOLEAN IsConhostv2;

    /**
     Set to TRUE to indicate that SavedCursorInfo is valid and should be
     restored on exit.
     */
    BOOLEAN HaveSavedCursorInfo;

    /**
     Set to TRUE to indicate that SavedScreenBufferInfo is valid and should
     be restored on exit.
     */
    BOOLEAN HaveSavedScreenBufferInfo;

    /**
     Set to TRUE to use characters from the first 127 for drawing.  If FALSE,
     Unicode characters can be used.
     */
    BOOLEAN UseAsciiDrawing;

} YORI_WIN_WINDOW_MANAGER, *PYORI_WIN_WINDOW_MANAGER;

/**
 Close and free the window manager.

 @param WinMgrHandle Pointer to the window manager.
 */
VOID
YoriWinCloseWindowManager(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{

    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    if (WinMgr->HaveSavedScreenBufferInfo) {
        SetConsoleCursorPosition(WinMgr->hConOut, WinMgr->SavedScreenBufferInfo.dwCursorPosition);
    }

    if (WinMgr->HaveSavedCursorInfo) {
        SetConsoleCursorInfo(WinMgr->hConOut, &WinMgr->SavedCursorInfo);
    }

    if (WinMgr->hConOriginal != NULL) {
        SetConsoleActiveScreenBuffer(WinMgr->hConOriginal);
        CloseHandle(WinMgr->hConOriginal);
    }

    if (WinMgr->hConOut != NULL) {
        CloseHandle(WinMgr->hConOut);
    }

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6001)
#endif
    if (WinMgr->hConIn != NULL) {
        CloseHandle(WinMgr->hConIn);
    }

    YoriLibFree(WinMgr);
    YoriLibEmptyProcessClipboard();
}


/**
 Initialize and open a window manager.  This should be called once per process
 that is interacting with the display via UI.

 @param UseAlternateBuffer If TRUE, switch the display to an alternate screen
        buffer.  This is useful for full screen applications.  If FALSE, the
        existing buffer is used.

 @param WinMgrHandle On successful completion, populated with a pointer to the
        window manager.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinOpenWindowManager(
    __in BOOLEAN UseAlternateBuffer,
    __out PYORI_WIN_WINDOW_MANAGER_HANDLE *WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr;

    WinMgr = YoriLibMalloc(sizeof(YORI_WIN_WINDOW_MANAGER));
    if (WinMgr == NULL) {
        return FALSE;
    }

    WinMgr->hConOriginal = NULL;
    YoriLibInitializeListHead(&WinMgr->TopLevelWindowList);
    YoriLibInitializeListHead(&WinMgr->TimerList);

    WinMgr->ColorTable = YoriWinGetDefaultColorTable();
    WinMgr->hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (WinMgr->hConOut == INVALID_HANDLE_VALUE) {
        WinMgr->hConOut = NULL;
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    WinMgr->hConIn = CreateFile(_T("CONIN$"), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (WinMgr->hConIn == INVALID_HANDLE_VALUE) {
        WinMgr->hConIn = NULL;
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (UseAlternateBuffer) {
        WinMgr->hConOriginal = WinMgr->hConOut;
        WinMgr->hConOut = CreateConsoleScreenBuffer(GENERIC_READ|GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
        if (WinMgr->hConOut == INVALID_HANDLE_VALUE) {
            WinMgr->hConOut = WinMgr->hConOriginal;
            WinMgr->hConOriginal = NULL;
            YoriWinCloseWindowManager(WinMgr);
            return FALSE;
        }

        SetConsoleActiveScreenBuffer(WinMgr->hConOut);
    }

    if (GetConsoleCursorInfo(WinMgr->hConOut, &WinMgr->SavedCursorInfo)) {
        WinMgr->HaveSavedCursorInfo = TRUE;
    }
    if (GetConsoleScreenBufferInfo(WinMgr->hConOut, &WinMgr->SavedScreenBufferInfo)) {
        WinMgr->HaveSavedScreenBufferInfo = TRUE;
    } else {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    //
    //  Probe for Conhostv2 by asking for a flag that only it supports.
    //  Conhostv2 reports coordinates differently (correctly) for mouse
    //  wheel events, whereas v1's coordinates are a mess.
    //

    WinMgr->IsConhostv2 = FALSE;
    if (SetConsoleMode(WinMgr->hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        WinMgr->IsConhostv2 = TRUE;
    }

    SetConsoleMode(WinMgr->hConOut, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);

    //
    //  Set the standard input flags and clear any extended flags.  This can
    //  fail on old systems that don't understand extended flags, so do it
    //  again without extended flags since on systems with them they're
    //  already clear.
    //

    SetConsoleMode(WinMgr->hConIn, ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
    SetConsoleMode(WinMgr->hConIn, ENABLE_MOUSE_INPUT);

    WinMgr->UseAsciiDrawing = FALSE;
    if (YoriLibIsNanoServer()) {
        WinMgr->UseAsciiDrawing = TRUE;
    }

    *WinMgrHandle = WinMgr;
    return TRUE;
}

/**
 Change if the window manager should use 7 bit characters or extended
 characters to display visual elements.

 @param WinMgrHandle Pointer to the window manager.

 @param UseAsciiDrawing If TRUE, only 7 bit characters should be used for
        visual elements.  If FALSE, extended characters should be used.
 */
VOID
YoriWinMgrSetAsciiDrawing(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in BOOLEAN UseAsciiDrawing
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    WinMgr->UseAsciiDrawing = UseAsciiDrawing;
}

/**
 Characters forming a single line rectangle border, in order of appearance:
 Top left corner, top line, top right corner, left line, right line,
 bottom left corner, bottom line, bottom right corner
 */
CONST TCHAR YoriWinSingleLineBorder[] = { 0x250c, 0x2500, 0x2510, 0x2502, 0x2502, 0x2514, 0x2500, 0x2518 };

/**
 Characters forming a double line rectangle border, in order of appearance
 */
CONST TCHAR YoriWinDoubleLineBorder[] = { 0x2554, 0x2550, 0x2557, 0x2551, 0x2551, 0x255A, 0x2550, 0x255D };

/**
 Characters forming a solid full height character border, in order of appearance
 */
CONST TCHAR YoriWinFullSolidBorder[] =  { 0x2588, 0x2588, 0x2588, 0x2588, 0x2588, 0x2588, 0x2588, 0x2588 };

/**
 Characters forming a solid half height character border, in order of appearance
 */
CONST TCHAR YoriWinHalfSolidBorder[] =  { 0x2588, 0x2580, 0x2588, 0x2588, 0x2588, 0x2588, 0x2584, 0x2588 };

/**
 Characters forming a single line rectangular border using only ASCII
 characters, in order of appearance
 */
CONST TCHAR YoriWinSingleLineAsciiBorder[] = { '+', '-', '+', '|', '|', '+', '-', '+' };

/**
 Characters forming a double line rectangular border using only ASCII
 characters, in order of appearance
 */
CONST TCHAR YoriWinDoubleLineAsciiBorder[] = { '+', '=', '+', '|', '|', '+', '=', '+' };

/**
 Characters forming a menu, left T, horizontal line, right T, check
 */
CONST TCHAR YoriWinMenu[] =  { 0x251c, 0x2500, 0x2524, 0x221a };

/**
 Characters forming a menu, left T, horizontal line, right T, check,
 using only ASCII characters
 */
CONST TCHAR YoriWinAsciiMenu[] =  { '+', '-', '+', '*' };

/**
 Characters forming a scroll bar, in order of appearance
 */
CONST TCHAR YoriWinScrollBar[] = { 0x2191, 0x2588, 0x2591, 0x2193 };

/**
 Characters forming a scroll bar using only ASCII characters, in order of
 appearance
 */
CONST TCHAR YoriWinAsciiScrollBar[] = { '^', '#', ' ', 'v' };

/**
 Characters forming a window shadow, from least dense to most dense
 */
CONST TCHAR YoriWinShadow[] = { 0x2591, 0x2592, 0x2593, 0x2588 };

/**
 Characters forming a window shadow using only ASCII characters
 */
CONST TCHAR YoriWinAsciiShadow[] = { '#', '#', '#', '#' };

/**
 Characters forming a combo box down arrow
 */
CONST TCHAR YoriWinComboDown[] = { 0x2193 };

/**
 Characters forming a combo box down arrow using only ASCII characters
 */
CONST TCHAR YoriWinAsciiComboDown[] = { 'v' };

/**
 Characters forming a radio selection
 */
CONST TCHAR YoriWinRadioSelection[] = { 0x2219 };

/**
 Characters forming a radio selection using only ASCII characters
 */
CONST TCHAR YoriWinAsciiRadioSelection[] = { 'o' };


/**
 An array of known characters used for visuals.  This must correspond to the
 order defined in winpriv.h.
 */
LPCTSTR YoriWinCharacterSetChars[] = {
    YoriWinSingleLineBorder,
    YoriWinDoubleLineBorder,
    YoriWinFullSolidBorder,
    YoriWinHalfSolidBorder,
    YoriWinSingleLineAsciiBorder,
    YoriWinDoubleLineAsciiBorder,
    YoriWinMenu,
    YoriWinAsciiMenu,
    YoriWinScrollBar,
    YoriWinAsciiScrollBar,
    YoriWinShadow,
    YoriWinAsciiShadow,
    YoriWinComboDown,
    YoriWinAsciiComboDown,
    YoriWinRadioSelection,
    YoriWinAsciiRadioSelection
};


/**
 Return the specified set of characters for drawing visuals.  If the display
 is configured for text only display with no extended characters, those are
 substituted here.

 @param WinMgrHandle Pointer to the window manager.

 @param CharacterSet Specifies the set of characters to obtain.

 @return A pointer to a global set of characters for this visual element.
 */
LPCTSTR
YoriWinGetDrawingCharacters(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in YORI_WIN_CHARACTERS CharacterSet
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    YORI_WIN_CHARACTERS EffectiveCharacterSet;

    EffectiveCharacterSet = CharacterSet;

    if (WinMgr->UseAsciiDrawing) {
        if (EffectiveCharacterSet == YoriWinCharsDoubleLineBorder) {
            EffectiveCharacterSet = YoriWinCharsDoubleLineAsciiBorder;
        } else if (EffectiveCharacterSet == YoriWinCharsMenu) {
            EffectiveCharacterSet = YoriWinCharsAsciiMenu;
        } else if (EffectiveCharacterSet == YoriWinCharsScrollBar) {
            EffectiveCharacterSet = YoriWinCharsAsciiScrollBar;
        } else if (EffectiveCharacterSet == YoriWinCharsShadow) {
            EffectiveCharacterSet = YoriWinCharsAsciiShadow;
        } else if (EffectiveCharacterSet == YoriWinCharsComboDown) {
            EffectiveCharacterSet = YoriWinCharsAsciiComboDown;
        } else if (EffectiveCharacterSet == YoriWinCharsRadioSelection) {
            EffectiveCharacterSet = YoriWinCharsAsciiRadioSelection;
        } else {
            EffectiveCharacterSet = YoriWinCharsSingleLineAsciiBorder;
        }
    }

    return YoriWinCharacterSetChars[EffectiveCharacterSet];
}

/**
 Lookup a specified color value within the window manager's configuration.

 @param WinMgrHandle Pointer to the window manager.

 @param ColorId An identifier of a system color.

 @return The attribute value to use.
 */
UCHAR
YoriWinMgrDefaultColorLookup(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in YORI_WIN_COLOR_ID ColorId
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    return YoriWinDefaultColorLookup(WinMgr->ColorTable, ColorId);
}

/**
 Returns the console input handle associated with the window manager.

 @param WinMgrHandle Pointer to the window manager.

 @return A handle to the console input.
 */
HANDLE
YoriWinGetConsoleInputHandle(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    return WinMgr->hConIn;
}

/**
 Returns the console output handle associated with the window manager.

 @param WinMgrHandle Pointer to the window manager.

 @return A handle to the console output.
 */
HANDLE
YoriWinGetConsoleOutputHandle(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    return WinMgr->hConOut;
}

/**
 Return TRUE if the console being used to manage the windows is a v2 console,
 which happens to handle mouse wheel events in a correct fashion.

 @param WinMgrHandle Pointer to the window manager.

 @return TRUE to indicate the console is a v2 console, FALSE if it is a v1
         console.
 */
BOOLEAN
YoriWinIsConhostv2(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    return WinMgr->IsConhostv2;
}

/**
 Return the size of the window manager, meaning the size of the addressable
 window when the window manager was initialized.

 @param WinMgrHandle Pointer to the window manager.

 @param Size On successful completion, populated with the size area known
        to the window manager.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinGetWinMgrDimensions(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __out PCOORD Size
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    PSMALL_RECT Rect;

    if (!WinMgr->HaveSavedScreenBufferInfo) {
        return FALSE;
    }

    Rect = &WinMgr->SavedScreenBufferInfo.srWindow;
    Size->X = (SHORT)(Rect->Right - Rect->Left + 1);
    Size->Y = (SHORT)(Rect->Bottom - Rect->Top + 1);
    return TRUE;
}

/**
 Return the location of the window manager, meaning the location of it
 in screen buffer coordinates from the time the window manager was
 initialized.

 @param WinMgrHandle Pointer to the window manager.

 @param Rect On successful completion, populated with location of the window
        manager.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinGetWinMgrLocation(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __out PSMALL_RECT Rect
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    PSMALL_RECT SrcRect;

    if (!WinMgr->HaveSavedScreenBufferInfo) {
        return FALSE;
    }

    SrcRect = &WinMgr->SavedScreenBufferInfo.srWindow;
    Rect->Left = SrcRect->Left;
    Rect->Top = SrcRect->Top;
    Rect->Right = SrcRect->Right;
    Rect->Bottom = SrcRect->Bottom;
    return TRUE;
}

/**
 Return the location of the cursor at the time the window manager was
 started.

 @param WinMgrHandle Pointer to the window manager.

 @param CursorLocation On successful completion, populated with the location
        of the cursor.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinGetWinMgrInitialCursorLocation(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __out PCOORD CursorLocation
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    if (!WinMgr->HaveSavedScreenBufferInfo) {
        return FALSE;
    }

    CursorLocation->X = WinMgr->SavedScreenBufferInfo.dwCursorPosition.X;
    CursorLocation->Y = WinMgr->SavedScreenBufferInfo.dwCursorPosition.Y;

    return TRUE;
}


/**
 Return the mask of mouse buttons which were pressed last time mouse input was
 processed.

 @param WinMgrHandle Pointer to the window manager.

 @return A mask of which mouse buttons were pressed last time mouse input was
         processed.
 */
DWORD
YoriWinGetPreviousMouseButtonState(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    return WinMgr->PreviousMouseButtonState;
}

/**
 Set the mask of mouse buttons which were pressed after performing mouse
 processing which will be used to calculate which buttons were pressed and
 released on the next processing pass.

 @param WinMgrHandle Pointer to the window manager.

 @param PreviousMouseButtonState Specifies the mask of mouse buttons which
        are pressed after processing an input event.
 */
VOID
YoriWinSetPreviousMouseButtonState(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in DWORD PreviousMouseButtonState
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    WinMgr->PreviousMouseButtonState = PreviousMouseButtonState;
}

/**
 Add a window to the stack of windows which will process events.  The newly
 added window is on top of the stack (and conceptually on top of other
 windows as far as the user is concerned.

 @param WinMgrHandle Pointer to the window manager.

 @param TopLevelWindowListEntry Pointer to the list within the window
        structure to link on the stack.
 */
VOID
YoriWinMgrPushWindow(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_LIST_ENTRY TopLevelWindowListEntry
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    YoriLibInsertList(&WinMgr->TopLevelWindowList, TopLevelWindowListEntry);
}

/**
 Remove the topmost window from the stack of windows which will process
 events.

 @param WinMgrHandle Pointer to the window manager.

 @param TopLevelWindowListEntry Pointer to the list within the window
        structure to unlink from the stack.
 */
VOID
YoriWinMgrPopWindow(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_LIST_ENTRY TopLevelWindowListEntry
    )
{
#if DBG
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    ASSERT(WinMgr->TopLevelWindowList.Next == TopLevelWindowListEntry);
#else
    UNREFERENCED_PARAMETER(WinMgrHandle);
#endif
    YoriLibRemoveListItem(TopLevelWindowListEntry);
}

/**
 Determine the next time, in absolute time terms, that a recurring timer
 should fire.

 @param Timer The timer to recalculate.
 */
VOID
YoriWinMgrCalculateNextExpiration(
    __in PYORI_WIN_TIMER Timer
    )
{
    LONGLONG IntervalInNtUnits;

    IntervalInNtUnits = Timer->PeriodicIntervalInMs;
    IntervalInNtUnits = IntervalInNtUnits * 1000 * 10;

    Timer->ExpirationTime = Timer->PeriodicStartTime + IntervalInNtUnits * (Timer->PeriodsExpired + 1);
}

/**
 Allocate and initialize a new recurring timer.

 @param WinMgrHandle Pointer to the window manager.

 @param Ctrl Pointer to the control to send events to when the timer event
        fires.

 @param PeriodicInterval The time in milliseconds between each timer event.

 @return A pointer to the timer object.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinMgrAllocateRecurringTimer(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_CTRL Ctrl,
    __in DWORD PeriodicInterval
    )
{
    PYORI_WIN_TIMER Timer;
    PYORI_WIN_WINDOW_MANAGER WinMgr;

    Timer = YoriLibMalloc(sizeof(YORI_WIN_TIMER));
    if (Timer == NULL) {
        return NULL;
    }

    WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    Timer->PeriodicStartTime = YoriLibGetSystemTimeAsInteger();
    Timer->NotifyCtrl = Ctrl;
    Timer->PeriodicIntervalInMs = PeriodicInterval;
    Timer->PeriodsExpired = 0;
    YoriWinMgrCalculateNextExpiration(Timer);
    YoriLibInsertList(&WinMgr->TimerList, &Timer->ListEntry);
    return (PYORI_WIN_CTRL_HANDLE)Timer;
}

/**
 Free a specified timer.

 @param TimerHandle Pointer to the timer to free.
 */
VOID
YoriWinMgrFreeTimer(
    __in PYORI_WIN_CTRL_HANDLE TimerHandle
    )
{
    PYORI_WIN_TIMER Timer;

    Timer = (PYORI_WIN_TIMER)TimerHandle;
    YoriLibRemoveListItem(&Timer->ListEntry);
    YoriLibFree(Timer);
}

/**
 When a control is being torn down, tear down all associated timers.

 @param WinMgrHandle Pointer to the window manager.

 @param Ctrl The control being torn down.
 */
VOID
YoriWinMgrRemoveTimersForControl(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_CTRL Ctrl
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr;
    PYORI_WIN_TIMER Timer;
    PYORI_LIST_ENTRY ListEntry;
    WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    ListEntry = YoriLibGetNextListEntry(&WinMgr->TimerList, NULL);
    while(TRUE) {
        if (ListEntry == NULL) {
            break;
        }

        Timer = CONTAINING_RECORD(ListEntry, YORI_WIN_TIMER, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&WinMgr->TimerList, ListEntry);
        if (Timer->NotifyCtrl == Ctrl) {
            YoriWinMgrFreeTimer(Timer);
        }
    }
}

/**
 Search through the stack of opened windows from top to bottom, finding the
 first window that is not in the process of closing.  If all windows in the
 stack are in the process of closing, returns NULL.

 @param WinMgr Pointer to the window manager.

 @return Pointer to the control within the topmost window that is not
         closing.
 */
PYORI_WIN_CTRL
YoriWinMgrTopMostNonClosingWindow(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_WINDOW_HANDLE WindowHandle;

    WindowHandle = NULL;
    ListEntry = NULL;

    while(TRUE) {
        ListEntry = YoriLibGetNextListEntry(&WinMgr->TopLevelWindowList, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        WindowHandle = YoriWinWindowFromTopLevelListEntry(ListEntry);
        if (!YoriWinIsWindowClosing(WindowHandle)) {
            break;
        }

        WindowHandle = NULL;
    }

    if (WindowHandle != NULL) {
        PYORI_WIN_CTRL WinCtrl;
        WinCtrl = YoriWinGetCtrlFromWindow(WindowHandle);
        return WinCtrl;
    }

    return NULL;
}

/**
 Read console input, and periodically check if the window has changed at all.
 New versions of conhost emit WINDOW_BUFFER_SIZE_EVENT when the window is
 resized to the right, but resizing the bottom does not emit this event and
 was possible with traditional consoles which didn't emit it either.  So this
 function periodically polls the window dimensions, and if a change is
 found, it synthesises a buffer size event.

 @param WinMgr Pointer to the window manager.

 @param Buffer Pointer to one or more input record structures to populate
        with input events.

 @param BufferLength Specifies the number of elements in the Buffer array.

 @param NumberOfEventsRead On successful completion, indicates the number of
        elements populated into the Buffer array.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriWinReadConsoleInputDetectWindowChange(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr,
    __out_ecount(BufferLength) PINPUT_RECORD Buffer,
    __in DWORD BufferLength,
    __out PDWORD NumberOfEventsRead
    )
{
    DWORD Err;
    DWORD TimeoutInMs;

    //
    //  Timeout every 400ms to check for buffer resize.
    //

    TimeoutInMs = 400;

    //
    //  If there's a timer set to expire before then, timeout when that
    //  timer is due.
    //

    if (!YoriLibIsListEmpty(&WinMgr->TimerList)) {
        PYORI_WIN_TIMER Timer;
        PYORI_LIST_ENTRY ListEntry;
        LONGLONG CurrentTime;
        LONGLONG MinimumFoundDelayTime;

        MinimumFoundDelayTime = TimeoutInMs * 1000 * 10;

        CurrentTime = YoriLibGetSystemTimeAsInteger();

        ListEntry = YoriLibGetNextListEntry(&WinMgr->TimerList, NULL);
        while(TRUE) {
            if (ListEntry == NULL) {
                break;
            }

            Timer = CONTAINING_RECORD(ListEntry, YORI_WIN_TIMER, ListEntry);
            ListEntry = YoriLibGetNextListEntry(&WinMgr->TimerList, ListEntry);
            if (Timer->ExpirationTime - CurrentTime < MinimumFoundDelayTime) {
                MinimumFoundDelayTime = Timer->ExpirationTime - CurrentTime;
            }
        }

        if (MinimumFoundDelayTime < 0) {
            MinimumFoundDelayTime = 0;
        }

        TimeoutInMs = (DWORD)(MinimumFoundDelayTime / (1000 * 10));
    }

    while (TRUE) {

        Err = WaitForSingleObject(WinMgr->hConIn, TimeoutInMs);

        if (Err == WAIT_TIMEOUT) {
            CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

            GetConsoleScreenBufferInfo(WinMgr->hConOut, &ScreenInfo);
            if (ScreenInfo.srWindow.Left != WinMgr->SavedScreenBufferInfo.srWindow.Left ||
                ScreenInfo.srWindow.Top != WinMgr->SavedScreenBufferInfo.srWindow.Top ||
                ScreenInfo.srWindow.Right != WinMgr->SavedScreenBufferInfo.srWindow.Right ||
                ScreenInfo.srWindow.Bottom != WinMgr->SavedScreenBufferInfo.srWindow.Bottom) {

                Buffer->EventType = WINDOW_BUFFER_SIZE_EVENT;
                Buffer->Event.WindowBufferSizeEvent.dwSize.X = ScreenInfo.dwSize.X;
                Buffer->Event.WindowBufferSizeEvent.dwSize.Y = ScreenInfo.dwSize.Y;

                *NumberOfEventsRead = 1;
                return TRUE;
            } else if (!YoriLibIsListEmpty(&WinMgr->TimerList)) {
                *NumberOfEventsRead = 0;
                return TRUE;
            }
        } else {
            return ReadConsoleInput(WinMgr->hConIn, Buffer, BufferLength, NumberOfEventsRead);
        }
    }
}

/**
 Process the input events from the system and send them to the window for
 processing.

 @param WinMgrHandle Pointer to the window manager.

 @param WindowHandle Pointer to the window to process input events for.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMgrProcessEvents(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    HANDLE hConIn;
    HANDLE hConOut;
    INPUT_RECORD InputRecords[10];
    CONSOLE_CURSOR_INFO NewCursorInfo;
    PINPUT_RECORD InputRecord;
    PYORI_WIN_WINDOW_MANAGER WinMgr;
    DWORD ActuallyRead;
    DWORD Index;
    DWORD RepeatIndex;
    YORI_WIN_EVENT Event;
    PYORI_WIN_CTRL WinCtrl;
    BOOLEAN Result;

    WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    WinCtrl = YoriWinGetCtrlFromWindow(WindowHandle);

    hConIn = YoriWinGetConsoleInputHandle(WinMgrHandle);
    hConOut = YoriWinGetConsoleOutputHandle(WinMgrHandle);

    //
    //  Make the cursor invisible while the window display is active.
    //

    YoriWinSaveCursorState(WindowHandle);

    NewCursorInfo.dwSize = 20;
    NewCursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConOut, &NewCursorInfo);

    YoriWinSetInitialFocus(WindowHandle);

    Result = FALSE;

    while(TRUE) {

        //
        //  Process any expired timers.
        //

        if (!YoriLibIsListEmpty(&WinMgr->TimerList)) {
            PYORI_WIN_TIMER Timer;
            PYORI_LIST_ENTRY ListEntry;
            LONGLONG CurrentTime;

            CurrentTime = YoriLibGetSystemTimeAsInteger();

            ListEntry = YoriLibGetNextListEntry(&WinMgr->TimerList, NULL);
            while(TRUE) {
                if (ListEntry == NULL) {
                    break;
                }

                Timer = CONTAINING_RECORD(ListEntry, YORI_WIN_TIMER, ListEntry);
                ListEntry = YoriLibGetNextListEntry(&WinMgr->TimerList, ListEntry);
                if (Timer->ExpirationTime < CurrentTime) {
                    Event.EventType = YoriWinEventTimer;
                    Event.Timer.Timer = Timer;
                    Timer->NotifyCtrl->NotifyEventFn(Timer->NotifyCtrl, &Event);
                    Timer->PeriodsExpired++;
                    YoriWinMgrCalculateNextExpiration(Timer);
                }
            }
        }

        //
        //  Process any posted events for the window.
        //

        while (TRUE) {
            PYORI_WIN_EVENT PostedEvent;
            PostedEvent = YoriWinGetNextPostedEvent(WinCtrl);

            if (PostedEvent == NULL) {
                break;
            }

            WinCtrl->NotifyEventFn(WinCtrl, PostedEvent);
            YoriWinFreePostedEvent(PostedEvent);
        }

        //
        //  Display window contents if they have changed.
        //

        if (!YoriWinDisplayWindowContents(WindowHandle)) {
            break;
        }

        if (!YoriWinReadConsoleInputDetectWindowChange(WinMgr, InputRecords, sizeof(InputRecords)/sizeof(InputRecords[0]), &ActuallyRead)) {
            break;
        }

        for (Index = 0; Index < ActuallyRead; Index++) {
            InputRecord = &InputRecords[Index];
            if (InputRecord->EventType == KEY_EVENT) {
                for (RepeatIndex = 0; RepeatIndex < InputRecord->Event.KeyEvent.wRepeatCount; RepeatIndex++) {
                    if (InputRecord->Event.KeyEvent.bKeyDown) {
                        Event.EventType = YoriWinEventKeyDown;

                        Event.KeyDown.CtrlMask = InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED | ENHANCED_KEY);
                        Event.KeyDown.VirtualKeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;
                        Event.KeyDown.VirtualScanCode = InputRecord->Event.KeyEvent.wVirtualScanCode;
                        Event.KeyDown.Char = InputRecord->Event.KeyEvent.uChar.UnicodeChar;
                    } else {
                        Event.EventType = YoriWinEventKeyUp;
                        Event.KeyUp.CtrlMask = InputRecord->Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED | ENHANCED_KEY);
                        Event.KeyUp.VirtualKeyCode = InputRecord->Event.KeyEvent.wVirtualKeyCode;
                        Event.KeyUp.VirtualScanCode = InputRecord->Event.KeyEvent.wVirtualScanCode;
                        Event.KeyUp.Char = InputRecord->Event.KeyEvent.uChar.UnicodeChar;
                    }

                    WinCtrl->NotifyEventFn(WinCtrl, &Event);
                }
            } else if (InputRecord->EventType == MOUSE_EVENT) {
                DWORD ButtonsPressed;
                DWORD ButtonsReleased;
                DWORD PreviousMouseButtonState;
                COORD Location;
                BOOLEAN InWindowRange;
                BOOLEAN InWindowClientRange;
                PreviousMouseButtonState = YoriWinGetPreviousMouseButtonState(WinMgrHandle);

                ButtonsPressed = InputRecord->Event.MouseEvent.dwButtonState - (PreviousMouseButtonState & InputRecord->Event.MouseEvent.dwButtonState);
                ButtonsReleased = PreviousMouseButtonState - (PreviousMouseButtonState & InputRecord->Event.MouseEvent.dwButtonState);
                YoriWinSetPreviousMouseButtonState(WinMgrHandle, InputRecord->Event.MouseEvent.dwButtonState);

                YoriWinTranslateScreenCoordinatesToWindow(WinCtrl, InputRecord->Event.MouseEvent.dwMousePosition, &InWindowRange, &InWindowClientRange, &Location);

                if (InputRecord->Event.MouseEvent.dwEventFlags == 0) {

                    if (InWindowClientRange) {
                        if (ButtonsReleased > 0) {
                            Event.EventType = YoriWinEventMouseUpInClient;
                            Event.MouseUp.ButtonsReleased = ButtonsReleased;
                            Event.MouseUp.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseUp.Location.X = Location.X;
                            Event.MouseUp.Location.Y = Location.Y;
                            WinCtrl->NotifyEventFn(WinCtrl, &Event);
                        }
                        if (ButtonsPressed > 0) {
                            Event.EventType = YoriWinEventMouseDownInClient;
                            Event.MouseDown.ButtonsPressed = ButtonsPressed;
                            Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseDown.Location.X = Location.X;
                            Event.MouseDown.Location.Y = Location.Y;
                            WinCtrl->NotifyEventFn(WinCtrl, &Event);
                        }
                    } else if (InWindowRange) {
                        if (ButtonsReleased > 0) {
                            Event.EventType = YoriWinEventMouseUpInNonClient;
                            Event.MouseUp.ButtonsReleased = ButtonsReleased;
                            Event.MouseUp.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseUp.Location.X = Location.X;
                            Event.MouseUp.Location.Y = Location.Y;
                            WinCtrl->NotifyEventFn(WinCtrl, &Event);
                        }
                        if (ButtonsPressed > 0) {
                            Event.EventType = YoriWinEventMouseDownInNonClient;
                            Event.MouseDown.ButtonsPressed = ButtonsPressed;
                            Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseDown.Location.X = Location.X;
                            Event.MouseDown.Location.Y = Location.Y;
                            WinCtrl->NotifyEventFn(WinCtrl, &Event);
                        }
                    } else {
                        if (ButtonsReleased > 0) {
                            Event.EventType = YoriWinEventMouseUpOutsideWindow;
                            Event.MouseUp.ButtonsReleased = ButtonsReleased;
                            Event.MouseUp.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseUp.Location.X = 0;
                            Event.MouseUp.Location.Y = 0;
                            if (!WinCtrl->NotifyEventFn(WinCtrl, &Event)) {
                                if (YoriWinIsWindowClosing(WindowHandle)) {
                                }
                            }
                        }
                        if (ButtonsPressed > 0) {
                            Event.EventType = YoriWinEventMouseDownOutsideWindow;
                            Event.MouseDown.ButtonsPressed = ButtonsPressed;
                            Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                            Event.MouseDown.Location.X = 0;
                            Event.MouseDown.Location.Y = 0;
                            if (!WinCtrl->NotifyEventFn(WinCtrl, &Event)) {
                                if (YoriWinIsWindowClosing(WindowHandle)) {
                                    PYORI_WIN_CTRL TopNonClosingWindow = YoriWinMgrTopMostNonClosingWindow(WinMgr);
                                    BOOLEAN SubInWindowRange;
                                    BOOLEAN SubInWindowClientRange;

                                    YoriWinTranslateScreenCoordinatesToWindow(TopNonClosingWindow, InputRecord->Event.MouseEvent.dwMousePosition, &SubInWindowRange, &SubInWindowClientRange, &Event.MouseDown.Location);

                                    if (SubInWindowClientRange) {
                                        Event.EventType = YoriWinEventMouseDownInClient;
                                    } else if (SubInWindowRange) {
                                        Event.EventType = YoriWinEventMouseDownInNonClient;
                                    }

                                    YoriWinPostEvent(TopNonClosingWindow, &Event);
                                }
                            }
                        }
                    }
                }

                if (InputRecord->Event.MouseEvent.dwEventFlags & DOUBLE_CLICK) {
                    if (InWindowClientRange) {
                        Event.EventType = YoriWinEventMouseDoubleClickInClient;
                        Event.MouseDown.ButtonsPressed = InputRecord->Event.MouseEvent.dwButtonState;
                        Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseDown.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - WinCtrl->FullRect.Left - WinCtrl->ClientRect.Left);
                        Event.MouseDown.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - WinCtrl->FullRect.Top - WinCtrl->ClientRect.Top);
                        WinCtrl->NotifyEventFn(WinCtrl, &Event);
                    } else if (InWindowRange) {
                        Event.EventType = YoriWinEventMouseDoubleClickInNonClient;
                        Event.MouseDown.ButtonsPressed = InputRecord->Event.MouseEvent.dwButtonState;
                        Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseDown.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - WinCtrl->FullRect.Left);
                        Event.MouseDown.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - WinCtrl->FullRect.Top);
                        WinCtrl->NotifyEventFn(WinCtrl, &Event);
                    }
                }

                if (InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_MOVED) {
                    if (InWindowClientRange) {
                        Event.EventType = YoriWinEventMouseMoveInClient;
                        Event.MouseMove.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseMove.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - WinCtrl->FullRect.Left - WinCtrl->ClientRect.Left);
                        Event.MouseMove.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - WinCtrl->FullRect.Top - WinCtrl->ClientRect.Top);
                        WinCtrl->NotifyEventFn(WinCtrl, &Event);
                    } else if (InWindowRange) {
                        Event.EventType = YoriWinEventMouseMoveInNonClient;
                        Event.MouseMove.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseMove.Location.X = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.X - WinCtrl->FullRect.Left);
                        Event.MouseMove.Location.Y = (SHORT)(InputRecord->Event.MouseEvent.dwMousePosition.Y - WinCtrl->FullRect.Top);
                        WinCtrl->NotifyEventFn(WinCtrl, &Event);
                    } else {
                        YORI_WIN_BOUNDED_COORD MousePos;

                        Event.EventType = YoriWinEventMouseMoveOutsideWindow;

                        MousePos.Left = FALSE;
                        MousePos.Right = FALSE;
                        MousePos.Above = FALSE;
                        MousePos.Below = FALSE;

                        MousePos.Pos.X = InputRecord->Event.MouseEvent.dwMousePosition.X;
                        MousePos.Pos.Y = InputRecord->Event.MouseEvent.dwMousePosition.Y;

                        YoriWinBoundCoordInSubRegion(&MousePos, &WinCtrl->FullRect, &Event.MouseMoveOutsideWindow.Location);

                        ASSERT(Event.MouseMoveOutsideWindow.Location.Left ||
                               Event.MouseMoveOutsideWindow.Location.Right ||
                               Event.MouseMoveOutsideWindow.Location.Above ||
                               Event.MouseMoveOutsideWindow.Location.Below);

                        Event.MouseMoveOutsideWindow.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        WinCtrl->NotifyEventFn(WinCtrl, &Event);
                    }
                }

                //
                //  Conhost v1 sends these events with the mouse location
                //  based on screen coordinates, then caps the returned values
                //  to console buffer size.  This capping means that
                //  information has been lost which prevents this code from
                //  re-translating the supplied coordinates back to screen
                //  coordinates and calculating the buffer location correctly.
                //  In addition, there appears to no way to get the "current"
                //  mouse position, which is obviously generally a bad idea.
                //  For these reasons, currently mouse wheel support is
                //  limited to Conhostv2.
                //
                if (InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_WHEELED &&
                    YoriWinIsConhostv2(WinMgrHandle)) {

                    BOOLEAN MoveUp;
                    SHORT MoveAmount;

                    Location.X = InputRecord->Event.MouseEvent.dwMousePosition.X;
                    Location.Y = InputRecord->Event.MouseEvent.dwMousePosition.Y;

                    MoveAmount = HIWORD(InputRecord->Event.MouseEvent.dwButtonState);
                    MoveUp = TRUE;
                    if (MoveAmount < 0) {
                        MoveUp = FALSE;
                        MoveAmount = (WORD)(-1 * MoveAmount);
                    }

                    MoveAmount = (WORD)(MoveAmount / 60);
                    if (MoveAmount == 0) {
                        MoveAmount = 1;
                    }

                    if (InWindowClientRange) {
                        if (MoveUp) {
                            Event.EventType = YoriWinEventMouseWheelUpInClient;
                        } else {
                            Event.EventType = YoriWinEventMouseWheelDownInClient;
                        }
                        Event.MouseWheel.LinesToMove = MoveAmount;
                        Event.MouseWheel.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseWheel.Location.X = Location.X;
                        Event.MouseWheel.Location.Y = Location.Y;
                        WinCtrl->NotifyEventFn(WinCtrl, &Event);
                    } else if (InWindowRange) {
                        if (MoveUp) {
                            Event.EventType = YoriWinEventMouseWheelUpInNonClient;
                        } else {
                            Event.EventType = YoriWinEventMouseWheelDownInNonClient;
                        }
                        Event.MouseWheel.LinesToMove = MoveAmount;
                        Event.MouseWheel.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                        Event.MouseWheel.Location.X = Location.X;
                        Event.MouseWheel.Location.Y = Location.Y;
                        WinCtrl->NotifyEventFn(WinCtrl, &Event);
                    }
                }
            } else if (InputRecord->EventType == WINDOW_BUFFER_SIZE_EVENT) {

                //
                //  Check if anything changed
                //  Go from top to bottom through the stack, hiding everything
                //  Update winmgr structures for new viewport
                //  Go from bottom to top through the stack, resizing
                //  Go from bottom to top through the stack, showing
                //

                PYORI_LIST_ENTRY ListEntry;
                PYORI_WIN_WINDOW_HANDLE CurrentWinHandle;
                PYORI_WIN_CTRL CurrentWinCtrl;
                CONSOLE_SCREEN_BUFFER_INFO OldScreenBufferInfo;
                CONSOLE_SCREEN_BUFFER_INFO NewScreenBufferInfo;

                memcpy(&OldScreenBufferInfo, &WinMgr->SavedScreenBufferInfo, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
                GetConsoleScreenBufferInfo(hConOut, &NewScreenBufferInfo);
                ListEntry = NULL;

                //
                //  Move from the top to the bottom of open windows, hiding
                //  them.  This is restoring the contents of the underlying
                //  window, which may want to rearrange itself in response
                //  to the resize.
                //

                while(TRUE) {
                    ListEntry = YoriLibGetNextListEntry(&WinMgr->TopLevelWindowList, ListEntry);
                    if (ListEntry == NULL) {
                        break;
                    }

                    CurrentWinHandle = YoriWinWindowFromTopLevelListEntry(ListEntry);
                    CurrentWinCtrl = YoriWinGetCtrlFromWindow(CurrentWinHandle);
                    Event.EventType = YoriWinEventHideWindow;
                    CurrentWinCtrl->NotifyEventFn(CurrentWinCtrl, &Event);
                }

                memcpy(&WinMgr->SavedScreenBufferInfo, &NewScreenBufferInfo, sizeof(CONSOLE_SCREEN_BUFFER_INFO));

                //
                //  From the bottom of the stack to the top of the stack,
                //  show all windows again, capturing the new contents of
                //  what is underneath.
                //

                while(TRUE) {
                    ListEntry = YoriLibGetPreviousListEntry(&WinMgr->TopLevelWindowList, ListEntry);
                    if (ListEntry == NULL) {
                        break;
                    }

                    CurrentWinHandle = YoriWinWindowFromTopLevelListEntry(ListEntry);
                    CurrentWinCtrl = YoriWinGetCtrlFromWindow(CurrentWinHandle);
                    Event.EventType = YoriWinEventWindowManagerResize;

                    //
                    //  Tell the window about the resize that is occurring
                    //

                    Event.WindowManagerResize.OldWinMgrDimensions.Left = OldScreenBufferInfo.srWindow.Left;
                    Event.WindowManagerResize.OldWinMgrDimensions.Top = OldScreenBufferInfo.srWindow.Top;
                    Event.WindowManagerResize.OldWinMgrDimensions.Right = OldScreenBufferInfo.srWindow.Right;
                    Event.WindowManagerResize.OldWinMgrDimensions.Bottom = OldScreenBufferInfo.srWindow.Bottom;

                    Event.WindowManagerResize.NewWinMgrDimensions.Left = NewScreenBufferInfo.srWindow.Left;
                    Event.WindowManagerResize.NewWinMgrDimensions.Top = NewScreenBufferInfo.srWindow.Top;
                    Event.WindowManagerResize.NewWinMgrDimensions.Right = NewScreenBufferInfo.srWindow.Right;
                    Event.WindowManagerResize.NewWinMgrDimensions.Bottom = NewScreenBufferInfo.srWindow.Bottom;

                    CurrentWinCtrl->NotifyEventFn(CurrentWinCtrl, &Event);

                    //
                    //  Then display the result.  Force the result to the
                    //  console so that windows on top of it can read the
                    //  result back
                    //

                    Event.EventType = YoriWinEventShowWindow;
                    CurrentWinCtrl->NotifyEventFn(CurrentWinCtrl, &Event);
                    YoriWinDisplayWindowContents(CurrentWinHandle);
                }
            }
        }


        if (YoriWinIsWindowClosing(WindowHandle)) {
            YoriWinDisplayWindowContents(WindowHandle);
            Result = TRUE;
            break;
        }
    }

    return Result;
}

// vim:sw=4:ts=4:et:
