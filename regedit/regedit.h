/**
 * @file regedit/regedit.h
 *
 * Yori shell registry editor master header
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

/**
 A set of well known control IDs so the dialog can manipulate its elements.
 */
typedef enum _REGEDIT_CONTROLS {
    RegeditControlKeyName = 1,
    RegeditControlKeyList = 2,
    RegeditControlValueList = 3,
    RegeditControlMenuBar = 4,
    RegeditControlCloseButton = 5,
    RegeditControlGoButton = 6,
    RegeditControlKeyCaption = 7,
    RegeditControlValueCaption = 8,
} REGEDIT_CONTROLS;


/**
 Context for the regedit application.
 */
typedef struct _REGEDIT_CONTEXT {

    /**
     The number of levels deep the active key is.  Zero means navigating the
     root keys.
     */
    DWORD TreeDepth;

    /**
     The root key used to enumerate the list of keys.  This is updated when
     navigating into a key.
     */
    HKEY ActiveRootKey;

    /**
     The subkey used to enumerate the list of keys.  This is updated when
     navigating into a key.
     */
    YORI_STRING Subkey;

    /**
     Indicates which list has had its selection changed most recently.  This
     is used to determine which list should be operated upon.
     */
    REGEDIT_CONTROLS MostRecentListSelectedControl;

    /**
     Index of the edit menu.  This is used to enable and disable menu items
     based on the state of the application.
     */
    DWORD EditMenuIndex;

    /**
     The index of the new menu item.
     */
    DWORD NewMenuIndex;

    /**
     The index of the delete menu item.
     */
    DWORD DeleteMenuIndex;

    /**
     TRUE to use only 7 bit ASCII characters for visual display.
     */
    BOOLEAN UseAsciiDrawing;

} REGEDIT_CONTEXT, *PREGEDIT_CONTEXT;

__success(return)
BOOLEAN
RegeditEditNumericValue(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr,
    __inout PYORI_STRING ValueName,
    __in BOOLEAN ValueNameReadOnly,
    __inout PDWORDLONG Value,
    __in BOOLEAN ValueReadOnly
    );

__success(return)
BOOLEAN
RegeditEditStringValue(
    __in PREGEDIT_CONTEXT RegeditContext,
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr,
    __inout PYORI_STRING ValueName,
    __in BOOLEAN ValueNameReadOnly,
    __inout PYORI_STRING Value,
    __in BOOLEAN ValueReadOnly
    );

// vim:sw=4:ts=4:et:
