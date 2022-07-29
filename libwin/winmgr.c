/**
 * @file libwin/winmgr.c
 *
 * Yori manage multiple overlapping windows
 *
 * Copyright (c) 2019-2022 Malcolm J. Smith
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
 The number of cells to use to display a shadow to the right of a window.
 */
#define YORI_WIN_SHADOW_WIDTH (2)

/**
 The number of cells to use to display a shadow underneath a window.
 */
#define YORI_WIN_SHADOW_HEIGHT (1)

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
     The list of timers active in the window manager.
     */
    YORI_LIST_ENTRY TimerList;

    /**
     A list of Windows, where the head of the list is on the top of the
     display order.
     */
    YORI_LIST_ENTRY ZOrderList;

    /**
     Pointer to the window with focus.  Focus changes when events are being
     processed, NOT when a new window is pushed to the top of the Z-order.
     This means that this value will eventually be the window on top of the
     Z-order, but not until that window has fully constructed its controls
     and starts processing events.
     */
    PYORI_WIN_CTRL FocusWindow;

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
     The saved cursor position, in window manager coordinates (ie., not the
     console screen buffer.)  If a resize occurs while the window manager is
     active, it might render to a different location to the buffer, which
     means restoring saved screen contents to a different buffer location,
     and the cursor should be relative to the region just restored.
     */
    COORD SavedCursorPosition;

    /**
     An array of cells describing the contents of the buffer before the window
     manager was drawn, which will be restored when it terminates.
     */
    PCHAR_INFO SavedContents;

    /**
     The dimensions of the SavedContents array.  This does not change for the
     lifetime of the window manager.
     */
    COORD SavedContentsSize;

    /**
     Specifies the minimum size for the window manager.  If it is resized to
     be less than this, the application is not displayed, and a message
     indicating the terminal is too small is displayed instead.
     */
    COORD MinimumSize;

    /**
     An array of cells describing the current display.  This is updated before
     the console, so the contents here describe what is or what will be
     displayed.
     */
    PCHAR_INFO Contents;

    /**
     A single character which is repeated many times during rendering.  This
     is used to generate window shadows, where each character is the same,
     and none of it is backed by a window buffer.
     */
    CHAR_INFO RepeatingCell;

    /**
     TRUE if some region of the display buffer has been regenerated and needs
     to be pushed to the console.  When this occurs, DirtyRect below indicates
     the range.  Contents in the Contents buffer above have been updated.
     */
    BOOLEAN DisplayDirty;

    /**
     If DirtyRect above is TRUE, this contains the region of the Contents
     buffer which needs to be displayed.  If DisplayDirty above is FALSE,
     the values in DirtyRect are not meaningful.
     */
    SMALL_RECT DirtyRect;

    /**
     TRUE to indicate that the cursor should be redisplayed unconditionally.
     */
    BOOLEAN UpdateCursor;

    /**
     The current state of the cursor on the display.  If the active window
     changes or if it moves the cursor, this will be compared to the new
     window cursor state and the console will be updated accordingly.
     */
    YORI_WIN_CURSOR_STATE DisplayedCursorState;

    /**
     The color table of default control colors to use.
     */
    YORI_WIN_COLOR_TABLE_HANDLE ColorTable;

    /**
     The mouse buttons that were pressed last time a mouse event was
     processed and sent to a window.  This is used to detect which buttons
     were pressed or released so future releases for previously notified
     presses can be notified.
     */
    DWORD PreviousNotifiedMouseButtonState;

    /**
     The mouse buttons that were pressed last time a mouse event was
     processed according to the console.  There may be more buttons pressed
     than have been notified, if a button was pressed outside of the
     owning window.
     */
    DWORD PreviousObservedMouseButtonState;

    /**
     Specifies the window that owns mouse button notifications.  This is
     non-NULL once a button press has been sent to a window; from that point,
     all future button events are sent to the window until all mouse buttons
     have been released.
     */
    PYORI_WIN_CTRL MouseButtonOwningWindow;

    /**
     Set to TRUE to indicate all mouse press events should be sent to one
     specific window (MouseButtonOwningWindow) regardless of the mouse
     location.  When FALSE, the mouse press events should be sent to
     whichever window is under the mouse.
     */
    BOOLEAN MouseOwnedExclusively;

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
 Derive what the transparent shadow color should be for a specified color.
 This means only the low four bits are meaningful; there is no background
 color specified here.

 @param Color Specifies the color that is present without a shadow.

 @return The corresponding color that should be used if this color is to
         become a transparent shadow.
 */
WORD
YoriWinTransparentColorFromColor(
    __in WORD Color
    )
{
    if (Color & FOREGROUND_INTENSITY) {
        return (WORD)(Color & (FOREGROUND_INTENSITY - 1));
    } else if (Color == (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)) {
        return FOREGROUND_INTENSITY;
    }
    return 0;
}

/**
 Derive what the transparent shadow attributes should be for a specified
 set of attributes.  The attributes here refer to both foreground and
 background.

 @param Attributes Specifies the foreground and background attributes that
        are present without a shadow.

 @return The corresponding attributes that should be used if this color is
         to become a transparent shadow.
 */
WORD
YoriWinTransparentAttributesFromAttributes(
    __in WORD Attributes
    )
{
    return (WORD)(YoriWinTransparentColorFromColor((WORD)(Attributes & 0xF)) |
                  (YoriWinTransparentColorFromColor((WORD)((Attributes >> 4) & 0xF)) << 4));
}

/**
 Record the contents of the console that are in the range that the window
 will display over.

 @param WinMgr Pointer to the window manager.  This should already contain an
        allocation to store the saved window contents into.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMgrSavePreviousContents(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr
   )
{
    COORD BufferPosition;
    SMALL_RECT ReadRect;
    COORD BufferSize;
    SMALL_RECT LineReadWindow;
    COORD LineReadBufferSize;
    DWORD CellCount;
    WORD LineIndex;
    WORD LineCount;
    PCHAR_INFO SavedBuffer;

    BufferPosition.X = 0;
    BufferPosition.Y = 0;
    YoriWinGetWinMgrLocation(WinMgr, &ReadRect);
    YoriWinGetWinMgrDimensions(WinMgr, &BufferSize);

    CellCount = BufferSize.X * BufferSize.Y;

    WinMgr->SavedContents = YoriLibMalloc(sizeof(CHAR_INFO) * CellCount);
    if (WinMgr->SavedContents == NULL) {
        return FALSE;
    }

    LineCount = BufferSize.Y;

    for (LineIndex = 0; LineIndex < LineCount; LineIndex++) {

        LineReadWindow.Left = 0;
        LineReadWindow.Right = ReadRect.Right;
        LineReadWindow.Top = (WORD)(ReadRect.Top + LineIndex);
        LineReadWindow.Bottom = LineReadWindow.Top;

        LineReadBufferSize.X = BufferSize.X;
        LineReadBufferSize.Y = 1;

        SavedBuffer = &WinMgr->SavedContents[LineIndex * BufferSize.X];

        if (!ReadConsoleOutput(WinMgr->hConOut, SavedBuffer, LineReadBufferSize, BufferPosition, &LineReadWindow)) {
            return FALSE;
        }
    }

    WinMgr->SavedContentsSize.X = BufferSize.X;
    WinMgr->SavedContentsSize.Y = BufferSize.Y;

    return TRUE;
}

/**
 Redisplay the saved buffer of cells underneath the window manager.

 @param WinMgr Pointer to the window manager to restore the saved buffer for.
 */
VOID
YoriWinMgrRestorePreviousContents(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr
    )
{
    COORD BufferPosition;
    COORD BufferSize;
    SMALL_RECT WriteRect;
    DWORD CharsWritten;
    DWORD CellsRemaining;

    //
    //  It doesn't make sense to close a window manager while windows are
    //  still running on it.  The code below to restore window contents
    //  depends on nothing running running on it, so all rendered cells
    //  come from the saved buffer.
    //

    ASSERT(YoriLibIsListEmpty(&WinMgr->ZOrderList));

    BufferPosition.X = 0;
    BufferPosition.Y = 0;
    YoriWinGetWinMgrDimensions(WinMgr, &BufferSize);

    WriteRect.Left = 0;
    WriteRect.Top = 0;
    WriteRect.Right = (SHORT)(BufferSize.X - 1);
    WriteRect.Bottom = (SHORT)(BufferSize.Y - 1);
    YoriWinMgrRegenerateRegion(WinMgr, &WriteRect);
    YoriWinMgrDisplayContents(WinMgr);

    //
    //  If there's text following, clear all of it.  This happens when the
    //  window is resized while the window manager is active.  In that case,
    //  text following might have stale window manager artifacts and is not
    //  meaningful.
    //

    YoriWinGetWinMgrLocation(WinMgr, &WriteRect);
    if (WinMgr->SavedScreenBufferInfo.dwSize.Y > WriteRect.Bottom + 1) {
        CellsRemaining = WinMgr->SavedScreenBufferInfo.dwSize.X;
        CellsRemaining = CellsRemaining * (WinMgr->SavedScreenBufferInfo.dwSize.Y - WriteRect.Bottom - 1);
        BufferPosition.Y = (SHORT)(WriteRect.Bottom + 1);
        FillConsoleOutputCharacter(WinMgr->hConOut, ' ', CellsRemaining, BufferPosition, &CharsWritten);
        FillConsoleOutputAttribute(WinMgr->hConOut, YoriLibVtGetDefaultColor(), CellsRemaining, BufferPosition, &CharsWritten);
    }
}

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

    if (WinMgr->SavedContents != NULL) {
        YoriWinMgrRestorePreviousContents(WinMgr);
        YoriLibFree(WinMgr->SavedContents);
        WinMgr->SavedContents = NULL;
    }

    if (WinMgr->Contents != NULL) {
        YoriLibFree(WinMgr->Contents);
        WinMgr->Contents = NULL;
    }

    if (WinMgr->HaveSavedScreenBufferInfo) {
        COORD NewCursorPosition;
        NewCursorPosition.X = (SHORT)(WinMgr->SavedScreenBufferInfo.srWindow.Left + WinMgr->SavedCursorPosition.X);
        NewCursorPosition.Y = (SHORT)(WinMgr->SavedScreenBufferInfo.srWindow.Top + WinMgr->SavedCursorPosition.Y);

        //
        //  If the window is smaller than when we started, keep the cursor
        //  within the smaller window.  Contents will not be restored outside
        //  of this range, so this avoids the cursor being moved into a blank
        //  place in the scrollback buffer.
        //

        if (NewCursorPosition.X > WinMgr->SavedScreenBufferInfo.srWindow.Right) {
            NewCursorPosition.X = WinMgr->SavedScreenBufferInfo.srWindow.Right;
        }

        if (NewCursorPosition.Y > WinMgr->SavedScreenBufferInfo.srWindow.Bottom) {
            NewCursorPosition.Y = WinMgr->SavedScreenBufferInfo.srWindow.Bottom;
            if (NewCursorPosition.Y + 1 < WinMgr->SavedScreenBufferInfo.dwSize.Y) {
                NewCursorPosition.Y = (SHORT)(NewCursorPosition.Y + 1);
                NewCursorPosition.X = 0;
            }
        }

        SetConsoleCursorPosition(WinMgr->hConOut, NewCursorPosition);
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
    COORD BufferSize;
    DWORD CellCount;
    DWORD CellIndex;

    WinMgr = YoriLibMalloc(sizeof(YORI_WIN_WINDOW_MANAGER));
    if (WinMgr == NULL) {
        return FALSE;
    }

    WinMgr->hConOriginal = NULL;
    WinMgr->SavedContents = NULL;
    WinMgr->Contents = NULL;
    YoriLibInitializeListHead(&WinMgr->TimerList);
    YoriLibInitializeListHead(&WinMgr->ZOrderList);
    WinMgr->DisplayDirty = FALSE;
    WinMgr->UpdateCursor = FALSE;
    WinMgr->SavedCursorPosition.X = 0;
    WinMgr->SavedCursorPosition.Y = 0;
    WinMgr->PreviousObservedMouseButtonState = 0;
    WinMgr->PreviousNotifiedMouseButtonState = 0;
    WinMgr->MouseButtonOwningWindow = NULL;
    WinMgr->MouseOwnedExclusively = FALSE;
    WinMgr->FocusWindow = NULL;

    //
    //  MSFIX: This should be configurable
    //

    WinMgr->MinimumSize.X = 60;
    WinMgr->MinimumSize.Y = 20;

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

        //
        //  The console generally tries to keep the viewport where the cursor
        //  is.  If the cursor is somewhere else completely, we can't restore
        //  the cursor and the viewport and have the result be meaningful.
        //

        if (WinMgr->SavedScreenBufferInfo.dwCursorPosition.X >= WinMgr->SavedScreenBufferInfo.srWindow.Left && 
            WinMgr->SavedScreenBufferInfo.dwCursorPosition.X <= WinMgr->SavedScreenBufferInfo.srWindow.Right && 
            WinMgr->SavedScreenBufferInfo.dwCursorPosition.Y >= WinMgr->SavedScreenBufferInfo.srWindow.Top && 
            WinMgr->SavedScreenBufferInfo.dwCursorPosition.Y <= WinMgr->SavedScreenBufferInfo.srWindow.Bottom) {

            WinMgr->SavedCursorPosition.X = (SHORT)(WinMgr->SavedScreenBufferInfo.dwCursorPosition.X - WinMgr->SavedScreenBufferInfo.srWindow.Left);
            WinMgr->SavedCursorPosition.Y = (SHORT)(WinMgr->SavedScreenBufferInfo.dwCursorPosition.Y - WinMgr->SavedScreenBufferInfo.srWindow.Top);
        }
    } else {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    if (!YoriWinMgrSavePreviousContents(WinMgr)) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    __analysis_assume(WinMgr->SavedContents != NULL);

    YoriWinGetWinMgrDimensions(WinMgr, &BufferSize);
    CellCount = BufferSize.X * BufferSize.Y;

    WinMgr->Contents = YoriLibMalloc(CellCount * sizeof(CHAR_INFO));
    if (WinMgr->Contents == NULL) {
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    for (CellIndex = 0; CellIndex < CellCount; CellIndex++) {
        WinMgr->Contents[CellIndex].Attributes = WinMgr->SavedContents[CellIndex].Attributes;
        WinMgr->Contents[CellIndex].Char.UnicodeChar = WinMgr->SavedContents[CellIndex].Char.UnicodeChar;
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

    YoriLibSetInputConsoleMode(WinMgr->hConIn, ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
    YoriLibSetInputConsoleMode(WinMgr->hConIn, ENABLE_MOUSE_INPUT);

    WinMgr->UseAsciiDrawing = FALSE;
    if (YoriLibIsNanoServer()) {
        WinMgr->UseAsciiDrawing = TRUE;
    }

    *WinMgrHandle = WinMgr;
    return TRUE;
}

/**
 Return TRUE if the system is incapable of processing a key press of an Alt
 key (with no additional key.)  If the system cannot handle this key event,
 keyboard accelerators are always displayed.  If the system can handle this
 event, they are not displayed until the Alt key is pressed.
 */
BOOLEAN
YoriWinMgrAlwaysDisplayAccelerators(VOID)
{
    if (YoriLibIsRunningUnderSsh() || YoriLibIsNanoServer()) {
        return TRUE;
    }

    return FALSE;
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
 Indicate that mouse press events are to be sent to one specific window,
 regardless of where the press occurs.  This is used for popup menus and
 combo boxes where the UI can be considered super-modal: it's not valid
 to interact with any other element while this element is open.

 @param WinMgrHandle Pointer to the window manager.

 @param ExclusiveWindow Pointer to the window which should receive all mouse
        input.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMgrLockMouseExclusively(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE ExclusiveWindow
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    ASSERT(ExclusiveWindow != NULL);
    ASSERT(!WinMgr->MouseOwnedExclusively);
    if (WinMgr->MouseOwnedExclusively) {
        return FALSE;
    }

    //
    //  If a different window is receiving buttons, cut them off, so all
    //  future presses and releases go to the exclusive owner.
    //
    //  MSFIX This means the window that's currently observed a press
    //  never observes a release.  Perhaps a release should be sent here?
    //

    if (WinMgr->MouseButtonOwningWindow != NULL &&
        WinMgr->MouseButtonOwningWindow != ExclusiveWindow) {

        WinMgr->PreviousNotifiedMouseButtonState = 0;
    }

    WinMgr->MouseButtonOwningWindow = ExclusiveWindow;
    WinMgr->MouseOwnedExclusively = TRUE;

    return TRUE;
}

/**
 Release exclusive ownership of the mouse.  After this, mouse press events
 are to be sent to whichever window is under the mouse location.

 @param WinMgrHandle Pointer to the window manager.

 @param ExclusiveWindow Pointer to the window which should stop receiving all
        mouse input.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMgrUnlockMouseExclusively(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE ExclusiveWindow
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    ASSERT(WinMgr->MouseOwnedExclusively);
    ASSERT(WinMgr->MouseButtonOwningWindow == ExclusiveWindow);
    if (!WinMgr->MouseOwnedExclusively) {
        return FALSE;
    }

    if (WinMgr->MouseButtonOwningWindow != ExclusiveWindow) {
        return FALSE;
    }

    WinMgr->MouseOwnedExclusively = FALSE;
    if (WinMgr->PreviousNotifiedMouseButtonState == 0) {
        WinMgr->MouseButtonOwningWindow = NULL;
    }

    return TRUE;
}


/**
 Return the mask of mouse buttons which were pressed last time mouse input was
 processed.

 @param WinMgrHandle Pointer to the window manager.

 @param MouseButtonOwningWindow On successful completion, updated to point to
        the window receiving mouse events.

 @param PreviousNotifiedMouseButtonState On successful completion, updated to
        indicate which mouse buttons have been notified to
        MouseButtonOwningWindow .

 @return A mask of which mouse buttons were pressed according to the console
         last time mouse input was processed.
 */
DWORD
YoriWinGetPreviousMouseButtonState(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __out PYORI_WIN_CTRL* MouseButtonOwningWindow,
    __out PDWORD PreviousNotifiedMouseButtonState
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    *MouseButtonOwningWindow = WinMgr->MouseButtonOwningWindow;
    if (WinMgr->PreviousObservedMouseButtonState) {
        ASSERT((WinMgr->PreviousNotifiedMouseButtonState & WinMgr->PreviousObservedMouseButtonState) == WinMgr->PreviousNotifiedMouseButtonState);
        ASSERT((WinMgr->PreviousNotifiedMouseButtonState | WinMgr->PreviousObservedMouseButtonState) == WinMgr->PreviousObservedMouseButtonState);
        ASSERT(WinMgr->MouseButtonOwningWindow != NULL || WinMgr->PreviousNotifiedMouseButtonState == 0);
        *PreviousNotifiedMouseButtonState = WinMgr->PreviousNotifiedMouseButtonState;
        return WinMgr->PreviousObservedMouseButtonState;
    }
    *PreviousNotifiedMouseButtonState = 0;
    return 0;
}

/**
 Set the mask of mouse buttons which were pressed after performing mouse
 processing which will be used to calculate which buttons were pressed and
 released on the next processing pass.

 @param WinMgrHandle Pointer to the window manager.

 @param PreviousObservedMouseButtonState Specifies the mask of mouse buttons
        which the console believes are pressed after processing an input
        event.

 @param PreviousNotifiedMouseButtonState Specifies the mask of mouse buttons
        which have been notified to the owning window after processing an
        input event.  Only released buttons in this mask should be notified
        to the window.

 @param MouseButtonOwningWindow Optionally specifies the window receiving
        mouse button presses.  This can be NULL if
        ProcessNotifiedMouseButtonState is zero.
 */
VOID
YoriWinSetPreviousMouseButtonState(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in DWORD PreviousObservedMouseButtonState,
    __in DWORD PreviousNotifiedMouseButtonState,
    __in PYORI_WIN_CTRL MouseButtonOwningWindow
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    if (PreviousObservedMouseButtonState != 0) {
        ASSERT(MouseButtonOwningWindow != NULL || PreviousNotifiedMouseButtonState == 0);
        ASSERT(WinMgr->MouseButtonOwningWindow == NULL || WinMgr->MouseButtonOwningWindow == MouseButtonOwningWindow);
        ASSERT((PreviousNotifiedMouseButtonState & PreviousObservedMouseButtonState) == PreviousNotifiedMouseButtonState);
        ASSERT((PreviousNotifiedMouseButtonState | PreviousObservedMouseButtonState) == PreviousObservedMouseButtonState);
        WinMgr->MouseButtonOwningWindow = MouseButtonOwningWindow;
        WinMgr->PreviousObservedMouseButtonState = PreviousObservedMouseButtonState;
        WinMgr->PreviousNotifiedMouseButtonState = PreviousNotifiedMouseButtonState;
    } else {
        if (!WinMgr->MouseOwnedExclusively) {
            WinMgr->MouseButtonOwningWindow = NULL;
        } else {
            ASSERT(WinMgr->MouseButtonOwningWindow != NULL);
        }
        WinMgr->PreviousObservedMouseButtonState = 0;
        WinMgr->PreviousNotifiedMouseButtonState = 0;
    }
}

/**
 Find which window, if any, is the topmost window that is rendering a
 specific cell.  This is used to determine which window should receive mouse
 events for an action at the specified location.

 @param WinMgrHandle Pointer to the window manager.

 @param Pos Specifies the location within the window manager, in window
        manager coordinates.

 @return Pointer to the window control, or NULL if no window is found at the
         specified location.
 */
PYORI_WIN_CTRL
YoriWinMgrGetWindowAtPosition(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in COORD Pos
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    PYORI_WIN_WINDOW_HANDLE WindowHandle;
    PYORI_WIN_CTRL WindowCtrl;
    PYORI_LIST_ENTRY ListEntry;

    ListEntry = NULL;

    while(TRUE) {
        ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        WindowHandle = YoriWinWindowFromZOrderListEntry(ListEntry);
        if (YoriWinIsWindowHidden(WindowHandle)) {

            continue;
        }

        WindowCtrl = YoriWinGetCtrlFromWindow(WindowHandle);
        if (Pos.Y >= WindowCtrl->FullRect.Top &&
            Pos.Y <= WindowCtrl->FullRect.Bottom &&
            Pos.X >= WindowCtrl->FullRect.Left &&
            Pos.X <= WindowCtrl->FullRect.Right) {

            return WindowCtrl;
        }
    }

    return NULL;
}

/**
 Obtain an array of cells to display at a specified location in the window
 manager.  The array comes from either a window buffer or the saved
 background.  This routine aims to return the largest possible array, but
 must stop when it encounters a window border, because future cells may come
 from a higher or lower window.  This routine can also return a pointer to a
 static window manager buffer to render a solid shadow, which is not contained
 in any window buffer.

 @param WinMgr Pointer to the window manager.

 @param StartPoint The location within the window manager to regenerate.

 @param LengthOfRun On successful completion, updated to indicate the number
        of cells to consume from the returned array.

 @param ShadowType On successful completion, updated to indicate whether the
        array should be covered with a transparent shadow.  In this case the
        source array does not contain the shadow effect (since the array
        refers to window contents) but this array must be manipulated to
        display as a shadow.

 @return Pointer to an array of cells to render into the display.
 */
PCCHAR_INFO
YoriWinMgrGetNextDisplayRangeFromWindowBuffers(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr,
    __in COORD StartPoint,
    __out PWORD LengthOfRun,
    __out PYORI_WIN_SHADOW_TYPE ShadowType
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_WINDOW_HANDLE WindowHandle;
    YORI_WIN_SHADOW_TYPE LocalShadowType;
    WORD MaximumRemainingLength;
    PCCHAR_INFO WindowBuffer;
    PCSMALL_RECT WindowRect;
    SMALL_RECT BufferPos;
    COORD WindowSize;
    WORD LineInWindow;
    WORD CharInWindow;
    BOOLEAN TransparentShadowSeen;

    WindowHandle = NULL;
    ListEntry = NULL;
    MaximumRemainingLength = (WORD)-1;
    *ShadowType = YoriWinShadowNone;
    TransparentShadowSeen = FALSE;
    YoriWinGetWinMgrLocation(WinMgr, &BufferPos);

    while(TRUE) {
        ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        WindowHandle = YoriWinWindowFromZOrderListEntry(ListEntry);
        if (YoriWinIsWindowHidden(WindowHandle)) {
            continue;
        }

        YoriWinGetWindowSize(WindowHandle, &WindowSize);
        WindowBuffer = YoriWinGetWindowContentsBuffer(WindowHandle, &WindowRect, &LocalShadowType);

        if (WindowRect->Top <= StartPoint.Y &&
            WindowRect->Bottom >= StartPoint.Y) {

            //
            //  If the search point is within the window, return a pointer to
            //  this window's buffer.  If the start point is to the left of
            //  this window, keep searching but cap the maximum to be the
            //  point where this window starts.
            //

            if (WindowRect->Left <= StartPoint.X &&
                WindowRect->Right >= StartPoint.X) {
                if (WindowRect->Right - StartPoint.X + 1 < MaximumRemainingLength) {
                    MaximumRemainingLength = (WORD)(WindowRect->Right - StartPoint.X + 1);
                }
                *LengthOfRun = MaximumRemainingLength;
                LineInWindow = (WORD)(StartPoint.Y - WindowRect->Top);
                CharInWindow = (WORD)(StartPoint.X - WindowRect->Left);
                if (TransparentShadowSeen) {
                    *ShadowType = YoriWinShadowTransparent;
                }
                return &WindowBuffer[LineInWindow * WindowSize.X + CharInWindow];

            } else if (LocalShadowType != YoriWinShadowNone &&
                       StartPoint.Y > WindowRect->Top &&
                       StartPoint.X > WindowRect->Right &&
                       StartPoint.X <= WindowRect->Right + YORI_WIN_SHADOW_WIDTH) {

                if (WindowRect->Right + YORI_WIN_SHADOW_WIDTH - StartPoint.X + 1 < MaximumRemainingLength) {
                    MaximumRemainingLength = (WORD)(WindowRect->Right + YORI_WIN_SHADOW_WIDTH - StartPoint.X + 1);
                }
                if (LocalShadowType == YoriWinShadowSolid) {
                    CONST TCHAR* ShadowChars;
                    ShadowChars = YoriWinGetDrawingCharacters(WinMgr, YoriWinCharsShadow);
                    WinMgr->RepeatingCell.Char.UnicodeChar = ShadowChars[0];
                    WinMgr->RepeatingCell.Attributes = (WORD)((YoriLibVtGetDefaultColor() & 0xF0) | FOREGROUND_INTENSITY);
                    *LengthOfRun = MaximumRemainingLength;
                    *ShadowType = LocalShadowType;
                    return &WinMgr->RepeatingCell;
                } else {
                    TransparentShadowSeen = TRUE;
                }
            } else if (WindowRect->Left > StartPoint.X) {
                if (WindowRect->Left - StartPoint.X < MaximumRemainingLength) {
                    MaximumRemainingLength = (WORD)(WindowRect->Left - StartPoint.X);
                }
            }
        } else if (LocalShadowType != YoriWinShadowNone &&
                   StartPoint.Y > WindowRect->Bottom &&
                   WindowRect->Bottom + YORI_WIN_SHADOW_HEIGHT >= StartPoint.Y) {

            if (StartPoint.X >= WindowRect->Left + YORI_WIN_SHADOW_WIDTH &&
                StartPoint.X <= WindowRect->Right + YORI_WIN_SHADOW_WIDTH) {
                if (WindowRect->Right + YORI_WIN_SHADOW_WIDTH - StartPoint.X + 1 < MaximumRemainingLength) {
                    MaximumRemainingLength = (WORD)(WindowRect->Right + YORI_WIN_SHADOW_WIDTH - StartPoint.X + 1);
                }
                if (LocalShadowType == YoriWinShadowSolid) {
                    CONST TCHAR* ShadowChars;
                    ShadowChars = YoriWinGetDrawingCharacters(WinMgr, YoriWinCharsShadow);
                    WinMgr->RepeatingCell.Char.UnicodeChar = ShadowChars[0];
                    WinMgr->RepeatingCell.Attributes = (WORD)((YoriLibVtGetDefaultColor() & 0xF0) | FOREGROUND_INTENSITY);
                    *LengthOfRun = MaximumRemainingLength;
                    *ShadowType = LocalShadowType;
                    return &WinMgr->RepeatingCell;
                } else {
                    TransparentShadowSeen = TRUE;
                }
            } else if (WindowRect->Left + YORI_WIN_SHADOW_WIDTH > StartPoint.X) {
                if (WindowRect->Left + YORI_WIN_SHADOW_WIDTH - StartPoint.X < MaximumRemainingLength) {
                    MaximumRemainingLength = (WORD)(WindowRect->Left + YORI_WIN_SHADOW_WIDTH - StartPoint.X);
                }
            }
        }
    }

    //
    //  Getting here means no window overlaps this point.  Find the range
    //  from the saved contents.  It's possible that if the window has been
    //  expanded there are no saved contents, in which case an empty cell
    //  is used.
    //

    YoriWinGetWinMgrDimensions(WinMgr, &WindowSize);
    if (WindowSize.X - StartPoint.X < MaximumRemainingLength) {
        MaximumRemainingLength = (WORD)(WindowSize.X - StartPoint.X);
    }
    if (StartPoint.Y < WinMgr->SavedContentsSize.Y &&
        StartPoint.X < WinMgr->SavedContentsSize.X) {

        if (WinMgr->SavedContentsSize.X - StartPoint.X < MaximumRemainingLength) {
            MaximumRemainingLength = (WORD)(WinMgr->SavedContentsSize.X - StartPoint.X);
        }
        *LengthOfRun = MaximumRemainingLength;
        LineInWindow = StartPoint.Y;
        CharInWindow = StartPoint.X;
        if (TransparentShadowSeen) {
            *ShadowType = YoriWinShadowTransparent;
        }
        return &WinMgr->SavedContents[LineInWindow * WinMgr->SavedContentsSize.X + CharInWindow];
    } else {
        WinMgr->RepeatingCell.Char.UnicodeChar = ' ';
        WinMgr->RepeatingCell.Attributes = (WORD)YoriLibVtGetDefaultColor();
        *LengthOfRun = MaximumRemainingLength;
        if (TransparentShadowSeen) {
            *ShadowType = YoriWinShadowTransparent;
        }
        return &WinMgr->RepeatingCell;
    }
}

/**
 Indicate that a particular cell has changed so the dirty region needs to
 be expanded to include it.

 @param WinMgr Pointer to the window manager.

 @param Point The cell whose contents have changed.
 */
VOID
YoriWinMgrExpandDirtyToPoint(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr,
    __in COORD Point
    )
{
    if (!WinMgr->DisplayDirty) {
        WinMgr->DisplayDirty = TRUE;
        WinMgr->DirtyRect.Left = Point.X;
        WinMgr->DirtyRect.Top = Point.Y;
        WinMgr->DirtyRect.Right = Point.X;
        WinMgr->DirtyRect.Bottom = Point.Y;
    } else {
        if (Point.X < WinMgr->DirtyRect.Left) {
            WinMgr->DirtyRect.Left = Point.X;
        } else if (Point.X > WinMgr->DirtyRect.Right) {
            WinMgr->DirtyRect.Right = Point.X;
        }

        if (Point.Y < WinMgr->DirtyRect.Top) {
            WinMgr->DirtyRect.Top = Point.Y;
        } else if (Point.Y > WinMgr->DirtyRect.Bottom) {
            WinMgr->DirtyRect.Bottom = Point.Y;
        }
    }
}

/**
 If the viewport has been resized so that it is too small to continue with
 a TUI display a fixed message is rendered instead.  This function renders
 that message.  The application using a TUI does not know this is occurring,
 since part of the goal is to avoid the application trying to resize itself
 to zero or near-zero size.

 @param WinMgr Pointer to the window manager.
 */
VOID
YoriWinMgrRegenerateTooSmall(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr
    )
{
    YORI_STRING DisplayString = YORILIB_CONSTANT_STRING(_T("TOO SMALL"));
    UCHAR Attributes;
    COORD BufferSize;
    COORD StartPoint;
    COORD Point;
    PCHAR_INFO Cell;
    TCHAR NewChar;

    Attributes = BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

    YoriWinGetWinMgrDimensions(WinMgr, &BufferSize);

    StartPoint.Y = (SHORT)(BufferSize.Y / 2);
    if (DisplayString.LengthInChars > (WORD)BufferSize.X) {
        StartPoint.X = 0;
    } else {
        StartPoint.X = (WORD)((BufferSize.X - DisplayString.LengthInChars) / 2);
    }

    Cell = WinMgr->Contents;
    for (Point.Y = 0; Point.Y < BufferSize.Y; Point.Y++) {
        for (Point.X = 0; Point.X < BufferSize.X; Point.X++) {
            if (Point.Y == StartPoint.Y &&
                Point.X >= StartPoint.X &&
                Point.X < (WORD)(StartPoint.X + DisplayString.LengthInChars)) {

                NewChar = DisplayString.StartOfString[Point.X - StartPoint.X];
            } else {
                NewChar = ' ';
            }

            if (Cell->Attributes != Attributes ||
                Cell->Char.UnicodeChar != NewChar) {

                Cell->Char.UnicodeChar = NewChar;
                Cell->Attributes = Attributes;

                YoriWinMgrExpandDirtyToPoint(WinMgr, Point);
            }
            Cell++;
        }
    }
}

/**
 Indicate that a region of the display may have changed and recalculate what
 should be in that region.  This occurs when the contents of a window changes,
 and the corresponding area of the display needs to be reevaluated.  If the
 window is on top, those contents are propagated to the display; if not,
 the existing contents are retained.  Partial updates may occur if a window
 is partially obscured by a different window.

 @param WinMgrHandle Pointer to the window manager.

 @param Rect The region of the window manager to recalculate.  This refers to
        window manager coordinates, ie., {0,0} represents the upper left of
        the window manager.
 */
VOID
YoriWinMgrRegenerateRegion(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PSMALL_RECT Rect
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    PCHAR_INFO Cell;
    PCCHAR_INFO WindowCell;
    WORD LengthOfRun;
    WORD EffectiveAttributes;
    COORD Point;
    COORD BufferSize;
    SMALL_RECT RedrawRect;
    YORI_WIN_SHADOW_TYPE ShadowType;

    YoriWinGetWinMgrDimensions(WinMgr, &BufferSize);

    if (!YoriLibIsListEmpty(&WinMgr->ZOrderList) &&
        (BufferSize.X < WinMgr->MinimumSize.X ||
         BufferSize.Y < WinMgr->MinimumSize.Y)) {

        YoriWinMgrRegenerateTooSmall(WinMgr);
        return;
    }

    //
    //  Individual windows might be off the edge of the window manager.
    //  Limit the redraw range to what the window manager is displaying.
    //  If the update is entirely off-screen, return immediately.
    //

    RedrawRect.Top = Rect->Top;
    RedrawRect.Left = Rect->Left;
    RedrawRect.Bottom = Rect->Bottom;
    RedrawRect.Right = Rect->Right;

    if (RedrawRect.Left < 0) {
        RedrawRect.Left = 0;
    }

    if (RedrawRect.Top < 0) {
        RedrawRect.Top = 0;
    }

    if (RedrawRect.Bottom >= BufferSize.Y) {
        RedrawRect.Bottom = (SHORT)(BufferSize.Y - 1);
    }

    if (RedrawRect.Right >= BufferSize.X) {
        RedrawRect.Right = (SHORT)(BufferSize.X - 1);
    }

    if (RedrawRect.Left > RedrawRect.Right ||
        RedrawRect.Top > RedrawRect.Bottom) {

        return;
    }

    for (Point.Y = RedrawRect.Top;
         Point.Y <= RedrawRect.Bottom;
         Point.Y++) {
        Point.X = RedrawRect.Left;

        while (Point.X <= RedrawRect.Right) {

            Cell = &WinMgr->Contents[Point.Y * BufferSize.X + Point.X];
            WindowCell = YoriWinMgrGetNextDisplayRangeFromWindowBuffers(WinMgr, Point, &LengthOfRun, &ShadowType);
            if (Point.X + LengthOfRun > RedrawRect.Right) {
                LengthOfRun = (WORD)(RedrawRect.Right - Point.X + 1);
            }

            ASSERT(WindowCell != NULL);

            //
            //  Update the range in the window manager buffer from the
            //  viewable window buffer.  If the window manager buffer is
            //  changing, track its dirty region for subsequent display.
            //

            while (LengthOfRun > 0) {
                EffectiveAttributes = WindowCell[0].Attributes;
                if (ShadowType == YoriWinShadowTransparent) {
                    EffectiveAttributes = YoriWinTransparentAttributesFromAttributes(EffectiveAttributes);
                }
                if (Cell[0].Char.UnicodeChar != WindowCell[0].Char.UnicodeChar ||
                    Cell[0].Attributes != EffectiveAttributes) {

                    Cell[0].Char.UnicodeChar = WindowCell[0].Char.UnicodeChar;
                    Cell[0].Attributes = EffectiveAttributes;

                    YoriWinMgrExpandDirtyToPoint(WinMgr, Point);
                }
                Cell++;
                if (WindowCell != &WinMgr->RepeatingCell) {
                    WindowCell++;
                }
                LengthOfRun--;
                Point.X++;
            }
        }
    }
}

/**
 Display the contents of the staged display into the console.  Generating
 this display is done via @ref YoriWinMgrRegenerateRegion .

 @param WinMgrHandle The window manager handle.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMgrDisplayContents(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    COORD BufferPosition;
    COORD BufferSize;
    SMALL_RECT WinMgrPos;
    SMALL_RECT RedrawWindow;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_WINDOW_HANDLE WindowHandle;
    YORI_WIN_CURSOR_STATE NewCursorState;

    YoriWinGetWinMgrDimensions(WinMgr, &BufferSize);

    //
    //  If there are display differences on Nano, push those to the console.
    //  Nano wants to be able to position the cursor after doing this update.
    //  On a regular system, the display is updated last, so that the cursor
    //  is rendered before the text, which seems to work better visually
    //  since these operations cannot be atomic.
    //

    if (WinMgr->DisplayDirty && YoriLibIsNanoServer()) {

        YoriWinGetWinMgrLocation(WinMgr, &WinMgrPos);

        BufferPosition.X = WinMgr->DirtyRect.Left;
        BufferPosition.Y = WinMgr->DirtyRect.Top;

        RedrawWindow.Left = (SHORT)(WinMgr->DirtyRect.Left + WinMgrPos.Left);
        RedrawWindow.Right = (SHORT)(WinMgr->DirtyRect.Right + WinMgrPos.Left);
        RedrawWindow.Top = (SHORT)(WinMgr->DirtyRect.Top + WinMgrPos.Top);
        RedrawWindow.Bottom = (SHORT)(WinMgr->DirtyRect.Bottom + WinMgrPos.Top);

        if (!WriteConsoleOutput(WinMgr->hConOut, WinMgr->Contents, BufferSize, BufferPosition, &RedrawWindow)) {
            return FALSE;
        }

        if (YoriLibIsNanoServer() && WinMgr->DisplayedCursorState.Visible) {
            WinMgr->UpdateCursor = TRUE;
        }

        WinMgr->DisplayDirty = FALSE;
    }

    //
    //  If there are cursor differences, update those now.  The topmost window
    //  gets to decide what happens to the cursor.
    //

    ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, NULL);

    //
    //  If the window manager is below minimum size, there's already an
    //  overlay, so hide the cursor.
    //

    if (BufferSize.X < WinMgr->MinimumSize.X ||
        BufferSize.Y < WinMgr->MinimumSize.Y) {
        NewCursorState.Visible = FALSE;
        NewCursorState.SizePercentage = (UCHAR)WinMgr->SavedCursorInfo.dwSize;
        NewCursorState.Pos.X = 0;
        NewCursorState.Pos.Y = 0;

    //
    //  If there's a topmost window, use its cursor state.
    //

    } else if (ListEntry != NULL) {
        PYORI_WIN_CTRL WinCtrl;
        WindowHandle = YoriWinWindowFromZOrderListEntry(ListEntry);
        YoriWinGetCursorState(WindowHandle, &NewCursorState);
        YoriWinGetWinMgrLocation(WinMgr, &WinMgrPos);
        WinCtrl = YoriWinGetCtrlFromWindow(WindowHandle);
        NewCursorState.Pos.X = (SHORT)(NewCursorState.Pos.X + WinMgrPos.Left + WinCtrl->FullRect.Left);
        NewCursorState.Pos.Y = (SHORT)(NewCursorState.Pos.Y + WinMgrPos.Top + WinCtrl->FullRect.Top);

    //
    //  Otherwise, use the saved cursor state from before any windows were
    //  displayed.
    //

    } else {
        ASSERT(WinMgr->HaveSavedCursorInfo && WinMgr->HaveSavedScreenBufferInfo);
        if (WinMgr->SavedCursorInfo.bVisible) {
            NewCursorState.Visible = TRUE;
        } else {
            NewCursorState.Visible = FALSE;
        }
        NewCursorState.SizePercentage = (UCHAR)WinMgr->SavedCursorInfo.dwSize;
        NewCursorState.Pos.X = WinMgr->SavedScreenBufferInfo.dwCursorPosition.X;
        NewCursorState.Pos.Y = WinMgr->SavedScreenBufferInfo.dwCursorPosition.Y;
    }

    if (WinMgr->UpdateCursor ||
        WinMgr->DisplayedCursorState.Visible != NewCursorState.Visible ||
        WinMgr->DisplayedCursorState.SizePercentage != NewCursorState.SizePercentage) {

        CONSOLE_CURSOR_INFO CursorInfo;
        CursorInfo.bVisible = NewCursorState.Visible;
        CursorInfo.dwSize = NewCursorState.SizePercentage;
        if (!SetConsoleCursorInfo(WinMgr->hConOut, &CursorInfo)) {
            return FALSE;
        }

        WinMgr->DisplayedCursorState.Visible = NewCursorState.Visible;
        WinMgr->DisplayedCursorState.SizePercentage = NewCursorState.SizePercentage;
    }

    //
    //  Don't position the cursor unless it's visible.  Moving the cursor
    //  will scroll the viewport.
    //

    if (WinMgr->DisplayedCursorState.Visible &&
        (WinMgr->UpdateCursor ||
         WinMgr->DisplayedCursorState.Pos.X != NewCursorState.Pos.X ||
         WinMgr->DisplayedCursorState.Pos.Y != NewCursorState.Pos.Y)) {

        if (!SetConsoleCursorPosition(WinMgr->hConOut, NewCursorState.Pos)) {
            return FALSE;
        }

        WinMgr->DisplayedCursorState.Pos.X = NewCursorState.Pos.X;
        WinMgr->DisplayedCursorState.Pos.Y = NewCursorState.Pos.Y;
        WinMgr->UpdateCursor = FALSE;
    }

    //
    //  Once the cursor is updated, update the display.
    //

    if (WinMgr->DisplayDirty) {

        YoriWinGetWinMgrLocation(WinMgr, &WinMgrPos);

        BufferPosition.X = WinMgr->DirtyRect.Left;
        BufferPosition.Y = WinMgr->DirtyRect.Top;

        RedrawWindow.Left = (SHORT)(WinMgr->DirtyRect.Left + WinMgrPos.Left);
        RedrawWindow.Right = (SHORT)(WinMgr->DirtyRect.Right + WinMgrPos.Left);
        RedrawWindow.Top = (SHORT)(WinMgr->DirtyRect.Top + WinMgrPos.Top);
        RedrawWindow.Bottom = (SHORT)(WinMgr->DirtyRect.Bottom + WinMgrPos.Top);

        if (!WriteConsoleOutput(WinMgr->hConOut, WinMgr->Contents, BufferSize, BufferPosition, &RedrawWindow)) {
            return FALSE;
        }

        WinMgr->DisplayDirty = FALSE;
    }

    return TRUE;
}

/**
 Calculate the region occupied on the window manager by a window, and
 regenerate it.  This function takes into account the window shadow, so it
 can be used to recalculate a display after a window is hidden, made
 visible, or moved.

 @param WinMgrHandle Pointer to the window manager.

 @param WindowHandle Pointer to the window.
 */
VOID
YoriWinMgrRefreshWindowRegion(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    YORI_WIN_SHADOW_TYPE LocalShadowType;
    PCSMALL_RECT WindowRect;
    SMALL_RECT RefreshRect;
    PYORI_WIN_WINDOW_MANAGER WinMgr;

    WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    YoriWinGetWindowContentsBuffer(WindowHandle, &WindowRect, &LocalShadowType);
    RefreshRect.Left = WindowRect->Left;
    RefreshRect.Top = WindowRect->Top;
    if (LocalShadowType == YoriWinShadowNone) {
        RefreshRect.Right = WindowRect->Right;
        RefreshRect.Bottom = WindowRect->Bottom;
    } else {
        RefreshRect.Right = (SHORT)(WindowRect->Right + YORI_WIN_SHADOW_WIDTH);
        RefreshRect.Bottom = (SHORT)(WindowRect->Bottom + YORI_WIN_SHADOW_HEIGHT);
    }

    YoriWinMgrRegenerateRegion(WinMgr, &RefreshRect);
}

/**
 Iterate through all windows and request them to refresh their window manager
 buffers.

 @param WinMgrHandle Pointer to the window manager.
 */
VOID
YoriWinMgrFlushAllWindows(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_WINDOW_HANDLE WindowHandle;

    WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    WindowHandle = NULL;
    ListEntry = NULL;

    while(TRUE) {
        ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        WindowHandle = YoriWinWindowFromZOrderListEntry(ListEntry);
        YoriWinFlushWindowContents(WindowHandle);
    }
}

/**
 Indicates if the window is the top window and enabled.  This is used by the
 window to determine whether to display the active title bar color or the
 inactive title bar color.

 @param WinMgrHandle Pointer to the window manager.

 @param WindowHandle Pointer to the window handle.

 @return TRUE to indicate the specified window is topmost and enabled, meaning
         it should render the active title bar color.  FALSE indicates that it
         is not on top or not enabled, and should use the inactive title bar
         color.
 */
BOOLEAN
YoriWinMgrIsWindowTopmostAndActive(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_WINDOW_MANAGER WinMgr;

    WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, NULL);
    if (ListEntry == NULL) {
        return FALSE;
    }

    if (ListEntry != YoriWinZOrderListEntryFromWindow(WindowHandle)) {
        return FALSE;
    }

    if (!YoriWinIsWindowEnabled(WindowHandle)) {
        return FALSE;
    }

    return TRUE;
}

/**
 Add a new window to the top of the visual display.

 @param WinMgrHandle The window manager.

 @param WindowHandle Pointer to the window handle.
 */
VOID
YoriWinMgrPushWindowZOrder(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    PYORI_LIST_ENTRY ZOrderWindowListEntry;

    //
    //  Add the window to the top of the stack.
    //

    ZOrderWindowListEntry = YoriWinZOrderListEntryFromWindow(WindowHandle);
    YoriLibInsertList(&WinMgr->ZOrderList, ZOrderWindowListEntry);

    //
    //  If the window isn't hidden, refresh the display.  Note
    //  this has to take into account the window shadow.
    //

    if (YoriWinIsWindowHidden(WindowHandle)) {
        return;
    }

    YoriWinMgrRefreshWindowRegion(WinMgr, WindowHandle);
}

/**
 Remove a window from the top of the visual display.

 @param WinMgrHandle The window manager.

 @param WindowHandle Pointer to the window handle.
 */
VOID
YoriWinMgrPopWindowZOrder(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    PYORI_LIST_ENTRY TopListEntry;
    PYORI_LIST_ENTRY ZOrderWindowListEntry;

    TopListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, NULL);
    ZOrderWindowListEntry = YoriWinZOrderListEntryFromWindow(WindowHandle);
    YoriLibRemoveListItem(ZOrderWindowListEntry);

    if (WinMgr->FocusWindow == YoriWinGetCtrlFromWindow(WindowHandle)) {
        WinMgr->FocusWindow = NULL;
    }

    YoriWinMgrRefreshWindowRegion(WinMgr, WindowHandle);
}

/**
 Move an existing window to the top of the Z-order and update the display
 accordingly.

 @param WinMgrHandle Pointer to the window manager.

 @param WindowHandle Pointer to the window to move.
 */
VOID
YoriWinMgrMoveWindowToTopOfZOrder(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_WIN_WINDOW_HANDLE WindowHandle
    )
{
    PYORI_LIST_ENTRY TopListEntry;
    PYORI_LIST_ENTRY WindowListEntry;
    PYORI_WIN_WINDOW_MANAGER WinMgr;

    WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    //
    //  Find the window currently on top (if any.)  If it's the same
    //  window, return immediately.
    //

    WindowListEntry = YoriWinZOrderListEntryFromWindow(WindowHandle);
    TopListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, NULL);
    if (TopListEntry == WindowListEntry) {
        return;
    }

    //
    //  Make the new window topmost.
    //

    YoriLibRemoveListItem(WindowListEntry);
    YoriLibInsertList(&WinMgr->ZOrderList, WindowListEntry);

    YoriWinMgrRefreshWindowRegion(WinMgr, WindowHandle);
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
 first window.  If no windows are in the stack, returns NULL.

 @param WinMgr Pointer to the window manager.

 @param EnabledOnly If TRUE, only return enabled windows.  If FALSE, return
        whichever window is topmost, disabled or not.

 @return Pointer to the control within the topmost window.
 */
PYORI_WIN_CTRL
YoriWinMgrTopMostWindow(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr,
    __in BOOLEAN EnabledOnly
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_WINDOW_HANDLE WindowHandle;

    WindowHandle = NULL;
    ListEntry = NULL;

    while(TRUE) {
        ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        WindowHandle = YoriWinWindowFromZOrderListEntry(ListEntry);
        if (EnabledOnly && !YoriWinIsWindowEnabled(WindowHandle)) {
            WindowHandle = NULL;
            continue;
        }
        break;
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
 Copy a range of characters from one 2D array to another.  The arrays may be
 of different dimensions.  If the destination is smaller than the source,
 excess cells are omitted.  If the destination is larger than the source,
 extra cells are populated with default attributes.

 @param Dest Pointer to the destination array.

 @param Src Pointer to the source array.

 @param DestSize Specifies the size of the destination array.

 @param SrcSize Specifies the size of the source array.
 */
VOID
YoriWinMgrCopyCharInfo(
    __out_ecount(DestSize.X * DestSize.Y) PCHAR_INFO Dest,
    __in_ecount(SrcSize.X * SrcSize.Y) PCHAR_INFO Src,
    __in COORD DestSize,
    __in COORD SrcSize
    )
{
    WORD Row;
    WORD Column;

    DWORD SrcIndex;
    DWORD DestIndex;

    WORD DefaultColor;

    DefaultColor = YoriLibVtGetDefaultColor();

    for (Row = 0; Row < SrcSize.Y && Row < DestSize.Y; Row++) {
        DestIndex = Row;
        DestIndex = DestIndex * DestSize.X;
        SrcIndex = Row;
        SrcIndex = SrcIndex * SrcSize.X;
        for (Column = 0; Column < SrcSize.X && Column < DestSize.X; Column++, SrcIndex++, DestIndex++) {
            Dest[DestIndex].Char.UnicodeChar = Src[SrcIndex].Char.UnicodeChar;
            Dest[DestIndex].Attributes = Src[SrcIndex].Attributes;
        }

        for (; Column < DestSize.X; Column++, DestIndex++) {
            Dest[DestIndex].Char.UnicodeChar = ' ';
            Dest[DestIndex].Attributes = DefaultColor;
        }
    }

    for (; Row < DestSize.Y; Row++) {
        DestIndex = Row;
        DestIndex = DestIndex * DestSize.X;

        for (Column = 0; Column < DestSize.X; Column++, DestIndex++) {
            Dest[DestIndex].Char.UnicodeChar = ' ';
            Dest[DestIndex].Attributes = DefaultColor;
        }
    }
}

/**
 Determine if any timers have expired, and if so, dispatch events accordingly.

 @param WinMgr Pointer to the window manager.
 */
VOID
YoriWinMgrProcessExpiredTimers(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr
    )
{
    PYORI_WIN_TIMER Timer;
    PYORI_LIST_ENTRY ListEntry;
    LONGLONG CurrentTime;
    YORI_WIN_EVENT Event;

    if (!YoriLibIsListEmpty(&WinMgr->TimerList)) {

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
}


/**
 Locate all events which have been queued for delivery asynchronously and
 deliver them.  Currently these are attached to top level windows, so this
 walks through all top level windows to find events.

 @param WinMgr Pointer to the window manager.
 */
VOID
YoriWinMgrProcessPostedEvents(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_WINDOW_HANDLE WindowHandle;
    PYORI_WIN_CTRL WindowCtrl;
    BOOLEAN EventSent;

    WindowHandle = NULL;
    ListEntry = NULL;

    while(TRUE) {
        ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        WindowHandle = YoriWinWindowFromZOrderListEntry(ListEntry);
        WindowCtrl = YoriWinGetCtrlFromWindow(WindowHandle);
        if (!YoriWinIsWindowEnabled(WindowCtrl)) {
            continue;
        }

        EventSent = FALSE;
        while (TRUE) {
            PYORI_WIN_EVENT PostedEvent;
            PostedEvent = YoriWinGetNextPostedEvent(WindowCtrl);

            if (PostedEvent == NULL) {
                break;
            }

            WindowCtrl->NotifyEventFn(WindowCtrl, PostedEvent);
            EventSent = TRUE;
            YoriWinFreePostedEvent(PostedEvent);
        }

        if (EventSent) {
            YoriWinFlushWindowContents(WindowHandle);
        }

        WindowHandle = NULL;
    }
}


/**
 Notify windows as necessary for key press and release events.
 click events.

 @param WinMgr Pointer to the window manager.

 @param InputRecord Pointer to the event as returned from the console.
 */
VOID
YoriWinMgrProcessKeyEvent(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr,
    __in PINPUT_RECORD InputRecord
    )
{
    PYORI_WIN_CTRL KeyFocusWindow;
    YORI_WIN_EVENT Event;
    DWORD RepeatIndex;
    BOOLEAN RedrawWindow;

    //
    //  Note this is currently assuming that the topmost window (in terms of
    //  visibility) must be the one receiving input.  In future it might make
    //  sense to seperate these concepts so that a "floating" window can be
    //  present while an underlying window receives input.  Eg. consider a
    //  find dialog that floats on top of edit while the user interacts with
    //  the edit control.
    //

    KeyFocusWindow = YoriWinMgrTopMostWindow(WinMgr, FALSE);

    //
    //  If there's no topmost window or the topmost window is disabled,
    //  there's no events to generate or redraw to occur.
    //

    if (KeyFocusWindow == NULL ||
        !YoriWinIsWindowEnabled(YoriWinGetWindowFromWindowCtrl(KeyFocusWindow))) {

        return;
    }
    RedrawWindow = FALSE;

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

        KeyFocusWindow->NotifyEventFn(KeyFocusWindow, &Event);
        RedrawWindow = TRUE;
    }

    if (RedrawWindow) {
        YoriWinFlushWindowContents(YoriWinGetWindowFromWindowCtrl(KeyFocusWindow));
    }
}

/**
 Notify windows as necessary for mouse press, release, move, and double
 click events.

 @param WinMgr Pointer to the window manager.

 @param InputRecord Pointer to the event as returned from the console.
 */
VOID
YoriWinMgrProcessMouseEvent(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr,
    __in PINPUT_RECORD InputRecord
    )
{
    YORI_WIN_EVENT Event;
    DWORD NotifiedMouseButtonState;
    DWORD ButtonsPressed;
    DWORD ButtonsReleased;
    DWORD PreviousObservedMouseButtonState;
    DWORD PreviousNotifiedMouseButtonState;
    PYORI_WIN_CTRL MouseButtonOwningWindow;
    PYORI_WIN_CTRL MouseOverWindow;
    PYORI_WIN_CTRL EffectiveWindow;
    SMALL_RECT WinMgrRect;
    COORD Location;
    BOOLEAN InWindowRange;
    BOOLEAN InWindowClientRange;

    PreviousObservedMouseButtonState = YoriWinGetPreviousMouseButtonState(WinMgr, &MouseButtonOwningWindow, &PreviousNotifiedMouseButtonState);

    //
    //  Check which mouse buttons have been pressed or released.
    //

    ButtonsPressed = InputRecord->Event.MouseEvent.dwButtonState - (PreviousObservedMouseButtonState & InputRecord->Event.MouseEvent.dwButtonState);
    ButtonsReleased = PreviousObservedMouseButtonState - (PreviousObservedMouseButtonState & InputRecord->Event.MouseEvent.dwButtonState);

    //
    //  Find which window the click occurred on.
    //

    MouseOverWindow = NULL;
    Location.X = InputRecord->Event.MouseEvent.dwMousePosition.X;
    Location.Y = InputRecord->Event.MouseEvent.dwMousePosition.Y;
    YoriWinGetWinMgrLocation(WinMgr, &WinMgrRect);
    if (Location.X >= WinMgrRect.Left &&
        Location.X <= WinMgrRect.Right &&
        Location.Y >= WinMgrRect.Top &&
        Location.Y <= WinMgrRect.Bottom) {

        Location.X = (SHORT)(Location.X - WinMgrRect.Left);
        Location.Y = (SHORT)(Location.Y - WinMgrRect.Top);
        MouseOverWindow = YoriWinMgrGetWindowAtPosition(WinMgr, Location);
    }

    //
    //  If the window is not accepting input, don't send any events to it or
    //  perform further processing for it.
    //

    if (MouseOverWindow != NULL && !YoriWinIsWindowEnabled(YoriWinGetWindowFromWindowCtrl(MouseOverWindow))) {
        MouseOverWindow = NULL;
    }

    //
    //  If no window owns mouse clicks, use the mouse over window as receiving
    //  future clicks.  Note that the mouse over window at this point may be
    //  NULL.
    //

    if (MouseButtonOwningWindow == NULL && InputRecord->Event.MouseEvent.dwButtonState) {
        MouseButtonOwningWindow = MouseOverWindow;
        if (MouseOverWindow != NULL) {
            YoriWinMgrMoveWindowToTopOfZOrder(WinMgr, MouseOverWindow);
        }
    }

    NotifiedMouseButtonState = 0;
    if (MouseButtonOwningWindow != NULL) {
        ASSERT((PreviousNotifiedMouseButtonState & PreviousObservedMouseButtonState) == PreviousNotifiedMouseButtonState);
        ASSERT((PreviousNotifiedMouseButtonState | PreviousObservedMouseButtonState) == PreviousObservedMouseButtonState);
        ButtonsReleased = ButtonsReleased & PreviousNotifiedMouseButtonState;
        NotifiedMouseButtonState = PreviousNotifiedMouseButtonState;
        if (MouseOverWindow == MouseButtonOwningWindow ||
            WinMgr->MouseOwnedExclusively) {

            NotifiedMouseButtonState = NotifiedMouseButtonState | ButtonsPressed;
        } else {
            ButtonsPressed = 0;
        }
        NotifiedMouseButtonState = NotifiedMouseButtonState & ~(ButtonsReleased);
    }
    YoriWinSetPreviousMouseButtonState(WinMgr, InputRecord->Event.MouseEvent.dwButtonState, NotifiedMouseButtonState, MouseButtonOwningWindow);

    InWindowRange = FALSE;
    InWindowClientRange = FALSE;
    Location.X = 0;
    Location.Y = 0;
    EffectiveWindow = NULL;
    if (MouseButtonOwningWindow != NULL) {
        EffectiveWindow = MouseButtonOwningWindow;
        YoriWinTranslateScreenCoordinatesToWindow(WinMgr, EffectiveWindow, InputRecord->Event.MouseEvent.dwMousePosition, &InWindowRange, &InWindowClientRange, &Location);

        if (InputRecord->Event.MouseEvent.dwEventFlags == 0) {

            if (InWindowClientRange) {
                if (ButtonsReleased > 0) {
                    Event.EventType = YoriWinEventMouseUpInClient;
                    Event.MouseUp.ButtonsReleased = ButtonsReleased;
                    Event.MouseUp.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                    Event.MouseUp.Location.X = Location.X;
                    Event.MouseUp.Location.Y = Location.Y;
                    MouseButtonOwningWindow->NotifyEventFn(MouseButtonOwningWindow, &Event);
                }
                if (ButtonsPressed > 0) {
                    Event.EventType = YoriWinEventMouseDownInClient;
                    Event.MouseDown.ButtonsPressed = ButtonsPressed;
                    Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                    Event.MouseDown.Location.X = Location.X;
                    Event.MouseDown.Location.Y = Location.Y;
                    MouseButtonOwningWindow->NotifyEventFn(MouseButtonOwningWindow, &Event);
                }
            } else if (InWindowRange) {
                if (ButtonsReleased > 0) {
                    Event.EventType = YoriWinEventMouseUpInNonClient;
                    Event.MouseUp.ButtonsReleased = ButtonsReleased;
                    Event.MouseUp.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                    Event.MouseUp.Location.X = Location.X;
                    Event.MouseUp.Location.Y = Location.Y;
                    MouseButtonOwningWindow->NotifyEventFn(MouseButtonOwningWindow, &Event);
                }
                if (ButtonsPressed > 0) {
                    Event.EventType = YoriWinEventMouseDownInNonClient;
                    Event.MouseDown.ButtonsPressed = ButtonsPressed;
                    Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                    Event.MouseDown.Location.X = Location.X;
                    Event.MouseDown.Location.Y = Location.Y;
                    MouseButtonOwningWindow->NotifyEventFn(MouseButtonOwningWindow, &Event);
                }
            } else {
                if (ButtonsReleased > 0) {
                    Event.EventType = YoriWinEventMouseUpOutsideWindow;
                    Event.MouseUp.ButtonsReleased = ButtonsReleased;
                    Event.MouseUp.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                    Event.MouseUp.Location.X = 0;
                    Event.MouseUp.Location.Y = 0;
                    MouseButtonOwningWindow->NotifyEventFn(MouseButtonOwningWindow, &Event);
                }
                if (ButtonsPressed > 0) {
                    Event.EventType = YoriWinEventMouseDownOutsideWindow;
                    Event.MouseDown.ButtonsPressed = ButtonsPressed;
                    Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                    Event.MouseDown.Location.X = 0;
                    Event.MouseDown.Location.Y = 0;
                    if (!MouseButtonOwningWindow->NotifyEventFn(MouseButtonOwningWindow, &Event)) {

                        //
                        //  If the mouse press causes the active window to
                        //  start to terminate, continue to send the mouse
                        //  press to whatever window remains.  This is used
                        //  when the user clicks a different menu in the menu
                        //  bar while one is already open.
                        //
                        //  MSFIX This flow seems a little strange since the
                        //  lower window would normally be disabled, and this
                        //  is being sent while it's disabled on the
                        //  expectation that closing the topmost window
                        //  implies it's about to become enabled.  Ideally
                        //  closing the top window would implicitly enable
                        //  the thing underneath it so this would go to an
                        //  enabled window.
                        //

                        if (YoriWinIsWindowClosing(YoriWinGetWindowFromWindowCtrl(MouseButtonOwningWindow))) {
                            PYORI_WIN_CTRL TopNonClosingWindow = YoriWinMgrTopMostWindow(WinMgr, FALSE);
                            BOOLEAN SubInWindowRange;
                            BOOLEAN SubInWindowClientRange;

                            YoriWinTranslateScreenCoordinatesToWindow(WinMgr, TopNonClosingWindow, InputRecord->Event.MouseEvent.dwMousePosition, &SubInWindowRange, &SubInWindowClientRange, &Event.MouseDown.Location);

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
                Event.MouseDown.Location.X = Location.X;
                Event.MouseDown.Location.Y = Location.Y;
                MouseButtonOwningWindow->NotifyEventFn(MouseButtonOwningWindow, &Event);
            } else if (InWindowRange) {
                Event.EventType = YoriWinEventMouseDoubleClickInNonClient;
                Event.MouseDown.ButtonsPressed = InputRecord->Event.MouseEvent.dwButtonState;
                Event.MouseDown.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                Event.MouseDown.Location.X = Location.X;
                Event.MouseDown.Location.Y = Location.Y;
                MouseButtonOwningWindow->NotifyEventFn(MouseButtonOwningWindow, &Event);
            }
        }
    } else if (MouseOverWindow != NULL) {

        EffectiveWindow = MouseOverWindow;
        YoriWinTranslateScreenCoordinatesToWindow(WinMgr, EffectiveWindow, InputRecord->Event.MouseEvent.dwMousePosition, &InWindowRange, &InWindowClientRange, &Location);
    }

    if (EffectiveWindow != NULL) {
        if (InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_MOVED) {
            if (InWindowClientRange) {
                Event.EventType = YoriWinEventMouseMoveInClient;
                Event.MouseMove.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                Event.MouseMove.Location.X = Location.X;
                Event.MouseMove.Location.Y = Location.Y;
                EffectiveWindow->NotifyEventFn(EffectiveWindow, &Event);
            } else if (InWindowRange) {
                Event.EventType = YoriWinEventMouseMoveInNonClient;
                Event.MouseMove.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                Event.MouseMove.Location.X = Location.X;
                Event.MouseMove.Location.Y = Location.Y;
                EffectiveWindow->NotifyEventFn(EffectiveWindow, &Event);
            } else {
                YORI_WIN_BOUNDED_COORD MousePos;

                Event.EventType = YoriWinEventMouseMoveOutsideWindow;

                MousePos.Left = FALSE;
                MousePos.Right = FALSE;
                MousePos.Above = FALSE;
                MousePos.Below = FALSE;

                MousePos.Pos.X = InputRecord->Event.MouseEvent.dwMousePosition.X;
                MousePos.Pos.Y = InputRecord->Event.MouseEvent.dwMousePosition.Y;

                //
                //  Calculate if it's off the edge of the window manager
                //

                YoriWinBoundCoordInSubRegion(&MousePos, &WinMgr->SavedScreenBufferInfo.srWindow, &Event.MouseMoveOutsideWindow.Location);

                //
                //  Calculate if it's off the edge of the window
                //

                YoriWinBoundCoordInSubRegion(&Event.MouseMoveOutsideWindow.Location, &EffectiveWindow->FullRect, &Event.MouseMoveOutsideWindow.Location);

                //
                //  It had better be off the edge of the window, or we
                //  should be in InClientRange or InWindowRange above
                //

                ASSERT(Event.MouseMoveOutsideWindow.Location.Left ||
                       Event.MouseMoveOutsideWindow.Location.Right ||
                       Event.MouseMoveOutsideWindow.Location.Above ||
                       Event.MouseMoveOutsideWindow.Location.Below);

                Event.MouseMoveOutsideWindow.ControlKeyState = InputRecord->Event.MouseEvent.dwControlKeyState;
                EffectiveWindow->NotifyEventFn(EffectiveWindow, &Event);
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

        if ((InputRecord->Event.MouseEvent.dwEventFlags & MOUSE_WHEELED) &&
            YoriWinIsConhostv2(WinMgr)) {

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
                EffectiveWindow->NotifyEventFn(EffectiveWindow, &Event);
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
                EffectiveWindow->NotifyEventFn(EffectiveWindow, &Event);
            }
        }

        YoriWinFlushWindowContents(YoriWinGetWindowFromWindowCtrl(EffectiveWindow));
    }
}

/**
 Notify windows as necessary for buffer resize events, which is frequently
 the result of a window size change (not necessarily a buffer size change.)

 @param WinMgr Pointer to the window manager.

 @param InputRecord Pointer to the event as returned from the console.
 */
VOID
YoriWinMgrProcessBufferSizeEvent(
    __in PYORI_WIN_WINDOW_MANAGER WinMgr,
    __in PINPUT_RECORD InputRecord
    )
{
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
    PCHAR_INFO NewAllocation;
    PSMALL_RECT Rect;
    SMALL_RECT NewRect;
    DWORD CellCount;
    COORD NewSize;
    COORD OldSize;
    HANDLE hConOut;
    YORI_WIN_EVENT Event;

    UNREFERENCED_PARAMETER(InputRecord);

    hConOut = YoriWinGetConsoleOutputHandle(WinMgr);
    YoriWinGetWinMgrDimensions(WinMgr, &OldSize);
    memcpy(&OldScreenBufferInfo, &WinMgr->SavedScreenBufferInfo, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
    GetConsoleScreenBufferInfo(hConOut, &NewScreenBufferInfo);
    ListEntry = NULL;

    Rect = &NewScreenBufferInfo.srWindow;
    NewSize.X = (SHORT)(Rect->Right - Rect->Left + 1);
    NewSize.Y = (SHORT)(Rect->Bottom - Rect->Top + 1);
    CellCount = NewSize.Y;
    CellCount = CellCount * NewSize.X;

    //
    //  If this allocation fails, the console will reflow text, so we
    //  redisplay what we have.
    //
    //  MSFIX: I think this needs to distinguish between the "effective window
    //  manager" coordinates and "actual console" size.  We want to display
    //  the size of the  window manager into the actual console, and limit it
    //  to the size of the actual console.
    //

    NewAllocation = YoriLibMalloc(CellCount * sizeof(CHAR_INFO));
    if (NewAllocation != NULL) {

        //
        //  The new screen buffer info contains a cursor at whatever position
        //  we left it.  Preserve the original location, assuming it seems
        //  mildly sensible.
        //

        if (OldScreenBufferInfo.dwCursorPosition.Y < NewScreenBufferInfo.dwSize.Y &&
            OldScreenBufferInfo.dwCursorPosition.X < NewScreenBufferInfo.dwSize.X) {

            NewScreenBufferInfo.dwCursorPosition.Y = OldScreenBufferInfo.dwCursorPosition.Y;
            NewScreenBufferInfo.dwCursorPosition.X = OldScreenBufferInfo.dwCursorPosition.X;
        }

        //
        //  Apply the new screen buffer info to the window manager and copy
        //  the previous saved contents to the new one, up to as much as will
        //  fit.
        //

        memcpy(&WinMgr->SavedScreenBufferInfo, &NewScreenBufferInfo, sizeof(CONSOLE_SCREEN_BUFFER_INFO));

        if (WinMgr->Contents != NULL) {
            YoriLibFree(WinMgr->Contents);
        }
        WinMgr->Contents = NewAllocation;

        //
        //  From the bottom of the stack to the top of the stack, show all
        //  windows again, capturing the new contents of what is underneath.
        //

        if (NewSize.X >= WinMgr->MinimumSize.X &&
            NewSize.Y >= WinMgr->MinimumSize.Y) {
            while(TRUE) {
                ListEntry = YoriLibGetPreviousListEntry(&WinMgr->ZOrderList, ListEntry);
                if (ListEntry == NULL) {
                    break;
                }

                CurrentWinHandle = YoriWinWindowFromZOrderListEntry(ListEntry);
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
                //  Because the viewport has moved, force the console to
                //  update the cursor location, even if the last place we put
                //  it is the correct place.
                //

                WinMgr->UpdateCursor = TRUE;
                YoriWinFlushWindowContents(CurrentWinHandle);
            }
        }
    } else {
        NewSize.X = OldSize.X;
        NewSize.Y = OldSize.Y;
    }

    //
    //  For simplicity, regenerate the entire display from all windows now
    //  they've updated themselves and flushed if necessary
    //

    NewRect.Left = 0;
    NewRect.Top = 0;
    NewRect.Right = (SHORT)(NewSize.X - 1);
    NewRect.Bottom = (SHORT)(NewSize.Y - 1);

    YoriWinMgrRegenerateRegion(WinMgr, &NewRect);
}

/**
 Process the input events from the system and send them to the window for
 processing.

 @param WinMgrHandle Pointer to the window manager.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMgrProcessAllEvents(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    )
{
    HANDLE hConIn;
    HANDLE hConOut;
    INPUT_RECORD InputRecords[10];
    PINPUT_RECORD InputRecord;
    PYORI_WIN_WINDOW_MANAGER WinMgr;
    DWORD ActuallyRead;
    DWORD Index;
    PYORI_WIN_CTRL WindowCtrl;
    BOOLEAN Result;

    WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;

    hConIn = YoriWinGetConsoleInputHandle(WinMgrHandle);
    hConOut = YoriWinGetConsoleOutputHandle(WinMgrHandle);

    Result = FALSE;

    //
    //  Windows are created, then have controls populated, then events pumped.
    //  This means there hasn't been a point before here where window contents
    //  are flushed to the window manager.
    //

    YoriWinMgrFlushAllWindows(WinMgrHandle);

    while(TRUE) {

        //
        //  Check if focus has changed.
        //

        WindowCtrl = YoriWinMgrTopMostWindow(WinMgr, FALSE);
        if (WindowCtrl != NULL &&
            YoriWinIsWindowEnabled(YoriWinGetWindowFromWindowCtrl(WindowCtrl)) &&
            WinMgr->FocusWindow != WindowCtrl) {

            if (WinMgr->FocusWindow != NULL) {
                YoriWinLoseWindowFocus(WinMgr->FocusWindow);
            }
            WinMgr->FocusWindow = WindowCtrl;
            YoriWinSetWindowFocus(WindowCtrl);
        }

        //
        //  Process any expired timers.
        //

        YoriWinMgrProcessExpiredTimers(WinMgr);

        //
        //  Process any posted events for top level windows.
        //

        YoriWinMgrProcessPostedEvents(WinMgr);

        //
        //  Display window manager contents if they have changed.
        //

        if (!YoriWinMgrDisplayContents(WinMgr)) {
            break;
        }

        //
        //  Find any input events and dispatch them to controls.
        //

        if (!YoriWinReadConsoleInputDetectWindowChange(WinMgr, InputRecords, sizeof(InputRecords)/sizeof(InputRecords[0]), &ActuallyRead)) {
            break;
        }

        for (Index = 0; Index < ActuallyRead; Index++) {
            InputRecord = &InputRecords[Index];
            if (InputRecord->EventType == KEY_EVENT) {
                YoriWinMgrProcessKeyEvent(WinMgr, InputRecord);
            } else if (InputRecord->EventType == MOUSE_EVENT) {
                YoriWinMgrProcessMouseEvent(WinMgr, InputRecord);
            } else if (InputRecord->EventType == WINDOW_BUFFER_SIZE_EVENT) {
                YoriWinMgrProcessBufferSizeEvent(WinMgr, InputRecord);
            }
        }

        //
        //  If there are no enabled windows, then there's nothing that can
        //  process events and it's time to stop.  This happens when a single
        //  modal window closes (since everything below it is disabled), but
        //  works for the final window in a program too.
        //

        WindowCtrl = YoriWinMgrTopMostWindow(WinMgr, TRUE);
        if (WindowCtrl == NULL) {
            Result = TRUE;
            break;
        }
    }

    return Result;
}

/**
 Process the input events from the system and send them to the topmost
 window for processing, ie., treat this window as modal.

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
    PYORI_WIN_WINDOW_MANAGER WinMgr = (PYORI_WIN_WINDOW_MANAGER)WinMgrHandle;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_WIN_WINDOW_HANDLE ThisWindow;
    BOOLEAN Result;

    DWORD SavedPreviousNotifiedMouseButtonState;
    DWORD SavedPreviousObservedMouseButtonState;
    PYORI_WIN_WINDOW_HANDLE SavedMouseButtonOwningWindow;

    UNREFERENCED_PARAMETER(WindowHandle);

    //
    //  If we're being asked to process events for one window, there had
    //  better be at least one window.
    //

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, ListEntry);
    ASSERT(ListEntry != NULL);
    if (ListEntry == NULL) {
        return FALSE;
    }

    //
    //  This API exists mainly for compatibility.  It was written to process
    //  events for the topmost window, so the window it's called on is
    //  expected to be topmost.
    //

    ThisWindow = YoriWinWindowFromZOrderListEntry(ListEntry);
    ASSERT(ThisWindow == WindowHandle);

    //
    //  Move to the next lower window (if any) and disable it and all beneath
    //  it, to preserve the semantic of only a single window's events being
    //  processed.
    //

    ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, ListEntry);
    while (ListEntry != NULL) {
        ThisWindow = YoriWinWindowFromZOrderListEntry(ListEntry);
        YoriWinDisableWindow(ThisWindow);
        ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, ListEntry);
    }

    SavedPreviousNotifiedMouseButtonState = WinMgr->PreviousNotifiedMouseButtonState;
    SavedPreviousObservedMouseButtonState = WinMgr->PreviousObservedMouseButtonState;
    SavedMouseButtonOwningWindow = WinMgr->MouseButtonOwningWindow;

    if (!WinMgr->MouseOwnedExclusively) {
        WinMgr->MouseButtonOwningWindow = NULL;
        WinMgr->PreviousNotifiedMouseButtonState = 0;
    }

    Result = YoriWinMgrProcessAllEvents(WinMgrHandle);

    //
    //  MSFIX I think this is hinting a larger problem.  The window manager
    //  can't track button presses like this if it's valid to disable
    //  windows, possibly claim exclusive mouse ownership and reenter event
    //  processing.  The nested processing isn't allowed to tell the disabled
    //  window that a button was released, and the top level processing
    //  wouldn't know that it happened (since the input is already processed.)
    //

    WinMgr->MouseButtonOwningWindow = SavedMouseButtonOwningWindow;
    WinMgr->PreviousObservedMouseButtonState = SavedPreviousObservedMouseButtonState;
    WinMgr->PreviousNotifiedMouseButtonState = SavedPreviousNotifiedMouseButtonState;

    // YoriWinSetPreviousMouseButtonState(WinMgrHandle, WinMgr->PreviousObservedMouseButtonState, 0, NULL);

    //
    //  Move to the next lower window (if any) and enable it and all beneath
    //  it, to preserve the semantic of only a single window's events being
    //  processed.
    //

    ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, NULL);
    while (ListEntry != NULL) {
        ThisWindow = YoriWinWindowFromZOrderListEntry(ListEntry);
        YoriWinEnableWindow(ThisWindow);
        ListEntry = YoriLibGetNextListEntry(&WinMgr->ZOrderList, ListEntry);
    }

    return Result;

}

// vim:sw=4:ts=4:et:
