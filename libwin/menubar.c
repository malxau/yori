/**
 * @file libwin/menubar.c
 *
 * Yori window menubar control
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

// ==================================================================
// Helper structures and functions
// ==================================================================
//


/**
 A structure describing an internal representation of a menu item.
 */
typedef struct _YORI_WIN_CTRL_MENU_ENTRY {

    /**
     The string to display for this menu item.  Note that any accelerator
     character is referred to below, so this string does not have an
     indication of which character, if any, is the accelerator character.
     */
    YORI_STRING DisplayCaption;

    /**
     A callback function to invoke when this menu item is activated.
     */
    PYORI_WIN_NOTIFY NotifyCallback;

    /**
     Pointer to an array of child menu items.  This is only meaningful if
     ChildItemCount below is nonzero.
     */
    struct _YORI_WIN_CTRL_MENU_ENTRY *ChildItems;

    /**
     The number of child menu items associated with this menu item, or zero
     to indicate no child menu items are present.
     */
    DWORD ChildItemCount;

    /**
     Flags associated with the menu item.
     */
    DWORD Flags;

    /**
     Specifies the offset, within DisplayCaption, of the character that is
     the accelerator character.
     */
    DWORD AcceleratorOffset;

    /**
     Specifies the character that is an accelerator key for this menu item.
     This can be zero to indicate the item is not associated with any
     accelerator.
     */
    TCHAR AcceleratorChar;
} YORI_WIN_CTRL_MENU_ENTRY, *PYORI_WIN_CTRL_MENU_ENTRY;

/**
 A structure that describes a single hotkey specification in a form that a
 keystroke can be compared against.
 */
typedef struct _YORI_WIN_CTRL_MENU_HOTKEY {

    /**
     The set of control keys that should be considered when testing for a
     match.
     */
    DWORD CtrlKeyMaskToCheck;

    /**
     The set of control key states out of CtrlKeyMaskToCheck that must match
     when finding a match.
     */
    DWORD CtrlKeyMaskToEqual;

    /**
     The virtual key code to check against for a match.
     */
    DWORD VirtualKeyCode;

    /**
     When a match is found, the menu entry to invoke.  When this entry is
     torn down, it must find the hotkey reference and remove it.
     */
    PYORI_WIN_CTRL_MENU_ENTRY EntryToInvoke;
} YORI_WIN_CTRL_MENU_HOTKEY, *PYORI_WIN_CTRL_MENU_HOTKEY;

/**
 An array of hotkeys that an incoming keystroke can be compared against.
 */
typedef struct _YORI_WIN_CTRL_MENU_HOTKEY_ARRAY {

    /**
     The number of entries that have been allocated.
     */
    DWORD Allocated;

    /**
     The number of entries that have been populated with entries.
     */
    DWORD Populated;

    /**
     An array of entries.
     */
    PYORI_WIN_CTRL_MENU_HOTKEY Keys;
} YORI_WIN_CTRL_MENU_HOTKEY_ARRAY, *PYORI_WIN_CTRL_MENU_HOTKEY_ARRAY;

/**
 Parse a hotkey string into a keystroke that can be compared against.

 @param HotkeyString Pointer to the string to display in the menu item.

 @param HotkeyInfo On successful completion, populated with the keystroke
        to test against for a match.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMenuGenerateHotkey(
    __in PYORI_STRING HotkeyString,
    __out PYORI_WIN_CTRL_MENU_HOTKEY HotkeyInfo
    )
{
    BOOLEAN RequiresCtrl = FALSE;
    BOOLEAN RequiresShift = FALSE;
    YORI_STRING Remainder;

    YoriLibInitEmptyString(&Remainder);
    Remainder.StartOfString = HotkeyString->StartOfString;
    Remainder.LengthInChars = HotkeyString->LengthInChars;

    if (Remainder.LengthInChars > sizeof("Ctrl+") - 1 &&
        YoriLibCompareStringWithLiteralCount(&Remainder, _T("Ctrl+"), sizeof("Ctrl+") - 1) == 0) {

        RequiresCtrl = TRUE;
        Remainder.LengthInChars -= sizeof("Ctrl+") - 1;
        Remainder.StartOfString += sizeof("Ctrl+") - 1;
    }

    if (Remainder.LengthInChars > sizeof("Shift+") - 1 &&
        YoriLibCompareStringWithLiteralCount(&Remainder, _T("Shift+"), sizeof("Shift+") - 1) == 0) {

        RequiresShift = TRUE;
        Remainder.LengthInChars -= sizeof("Shift+") - 1;
        Remainder.StartOfString += sizeof("Shift+") - 1;
    }

    if (Remainder.LengthInChars >= 2 &&
        Remainder.StartOfString[0] == 'F' &&
        (Remainder.StartOfString[1] >= '1' && Remainder.StartOfString[1] <= '9')) {

        HotkeyInfo->CtrlKeyMaskToCheck = LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED;
        HotkeyInfo->CtrlKeyMaskToEqual = 0;
        if (RequiresCtrl) {
            HotkeyInfo->CtrlKeyMaskToEqual |= LEFT_CTRL_PRESSED;
        }
        if (RequiresShift) {
            HotkeyInfo->CtrlKeyMaskToEqual |= SHIFT_PRESSED;
        }

        if (Remainder.LengthInChars >= 3 &&
            Remainder.StartOfString[1] == '1' &&
            (Remainder.StartOfString[2] >= '0' && Remainder.StartOfString[2] <= '2')) {
            HotkeyInfo->VirtualKeyCode = VK_F10 + (Remainder.StartOfString[2] - '0');
        } else {
            HotkeyInfo->VirtualKeyCode = VK_F1 + (Remainder.StartOfString[1] - '1');
        }
        HotkeyInfo->EntryToInvoke = NULL;
        return TRUE;

    } else if (Remainder.LengthInChars == 1) {
        if (RequiresCtrl || RequiresShift) {
            HotkeyInfo->CtrlKeyMaskToCheck = LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED;
            HotkeyInfo->CtrlKeyMaskToEqual = 0;
            if (RequiresCtrl) {
                HotkeyInfo->CtrlKeyMaskToEqual |= LEFT_CTRL_PRESSED;
            }
            if (RequiresShift) {
                HotkeyInfo->CtrlKeyMaskToEqual |= SHIFT_PRESSED;
            }
            HotkeyInfo->VirtualKeyCode = YoriLibUpcaseChar(Remainder.StartOfString[0]);
            HotkeyInfo->EntryToInvoke = NULL;
            return TRUE;
        }
    } else if (YoriLibCompareStringWithLiteralInsensitiveCount(&Remainder, _T("Del"), sizeof("Del") - 1) == 0) {
        HotkeyInfo->CtrlKeyMaskToCheck = LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED;
        HotkeyInfo->CtrlKeyMaskToEqual = 0;
        if (RequiresCtrl) {
            HotkeyInfo->CtrlKeyMaskToEqual |= LEFT_CTRL_PRESSED;
        }
        if (RequiresShift) {
            HotkeyInfo->CtrlKeyMaskToEqual |= SHIFT_PRESSED;
        }
        HotkeyInfo->VirtualKeyCode = VK_DELETE;
        return TRUE;
    }

    return FALSE;
}

/**
 Add a keystroke specification to an array of known keystrokes.  This may need
 to reallocate the array to contain the new item.

 @param Array Pointer to the array of known hotkeys.

 @param Hotkey Pointer to the keystroke to insder into the array.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMenuAddHotkeyToArray(
    __in PYORI_WIN_CTRL_MENU_HOTKEY_ARRAY Array,
    __in PYORI_WIN_CTRL_MENU_HOTKEY Hotkey
    )
{
    ASSERT(Hotkey->EntryToInvoke != NULL);

    if (Array->Populated + 1 >= Array->Allocated) {
        DWORD NewAllocated;
        PYORI_WIN_CTRL_MENU_HOTKEY NewKeysArray;

        NewAllocated = Array->Allocated + 100;
        NewKeysArray = YoriLibReferencedMalloc(NewAllocated * sizeof(YORI_WIN_CTRL_MENU_HOTKEY));
        if (NewKeysArray == NULL) {
            return FALSE;
        }

        if (Array->Populated > 0) {
            memcpy(NewKeysArray, Array->Keys, Array->Populated * sizeof(YORI_WIN_CTRL_MENU_HOTKEY));
            YoriLibDereference(Array->Keys);
        }
        Array->Keys = NewKeysArray;
        Array->Allocated = NewAllocated;
    }

    memcpy(&Array->Keys[Array->Populated], Hotkey, sizeof(YORI_WIN_CTRL_MENU_HOTKEY));
    Array->Populated++;
    return TRUE;
}

/**
 Remove a hotkey from an array of hotkeys.  This is identified by the menu
 entry that the hotkey would invoke, since the location of the hotkey is
 unknown.

 @param Array Pointer to an array of known hotkeys.

 @param MenuEntry Pointer to the menu entry being removed.
 */
VOID
YoriWinMenuRemoveHotkeyFromArray(
    __in PYORI_WIN_CTRL_MENU_HOTKEY_ARRAY Array,
    __in PYORI_WIN_CTRL_MENU_ENTRY MenuEntry
    )
{
    DWORD Index;

    for (Index = 0; Index < Array->Populated; Index++) {
        if (Array->Keys[Index].EntryToInvoke == MenuEntry) {
            if (Index + 1 < Array->Populated) {
                DWORD NumberToCopy;
                NumberToCopy = Array->Populated - Index - 1;
                memcpy(&Array->Keys[Index], &Array->Keys[Index + 1], NumberToCopy * sizeof(YORI_WIN_CTRL_MENU_HOTKEY));
                Array->Populated--;
                return;
            }
        }
    }
}


VOID
YoriWinMenuFreeMenuEntry(
    __in PYORI_WIN_CTRL_MENU_HOTKEY_ARRAY HotkeyArray,
    __in PYORI_WIN_CTRL_MENU_ENTRY Entry
    );

/**
 Free an array of menu items, including an array that describes a sub menu.

 @param HotkeyArray Pointer to an array of known hotkeys which can be updated
        to remove references to these menu items.

 @param ItemArray Pointer to the array of menu items to free.

 @param ItemCount Specifies the number of entries in the menu item array.
 */
VOID
YoriWinMenuFreeEntryArray(
    __in PYORI_WIN_CTRL_MENU_HOTKEY_ARRAY HotkeyArray,
    __in PYORI_WIN_CTRL_MENU_ENTRY ItemArray,
    __in DWORD ItemCount
    )
{
    DWORD Index;

    for (Index = 0; Index < ItemCount; Index++) {
        YoriWinMenuFreeMenuEntry(HotkeyArray, &ItemArray[Index]);
    }
}

/**
 Free a single entry that is allocated to describe a menu item.  Note this may
 recursively free any sub menu items.

 @param HotkeyArray Pointer to an array of known hotkeys which can be updated
        to remove references to these menu items.

 @param Entry Pointer to the menu item to free.
 */
VOID
YoriWinMenuFreeMenuEntry(
    __in PYORI_WIN_CTRL_MENU_HOTKEY_ARRAY HotkeyArray,
    __in PYORI_WIN_CTRL_MENU_ENTRY Entry
    )
{
    YoriWinMenuRemoveHotkeyFromArray(HotkeyArray, Entry);
    YoriLibFreeStringContents(&Entry->DisplayCaption);
    if (Entry->ChildItemCount > 0) {
        YoriWinMenuFreeEntryArray(HotkeyArray, Entry->ChildItems, Entry->ChildItemCount);
        YoriLibDereference(Entry->ChildItems);
        Entry->ChildItems = NULL;
        Entry->ChildItemCount = 0;
    }
}


__success(return)
BOOLEAN
YoriWinMenuCopySubMenu(
    __in PYORI_WIN_CTRL_MENU_HOTKEY_ARRAY HotkeyArray,
    __in PYORI_WIN_MENU_ENTRY Input,
    __inout PYORI_WIN_CTRL_MENU_ENTRY Output
    );

/**
 Copy a menu entry from a user provided structure into the representation
 used by the control.  Note that this routine can recursively copy any sub
 menu.

 @param HotkeyArray Pointer to an array of known hotkeys which can be updated
        to contain references to these menu items.

 @param Input Pointer to the user provided structure.

 @param MaxCaption Specifies the longest caption text of any entry that will
        be populated on this submenu.  This is used to align any hotkey
        strings.

 @param Output Pointer to a caller allocated control structure to receive the
        information.  Note this routine will allocate space for the string
        of the menu item, and hence, this routine may fail.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMenuCopyUserEntry(
    __in PYORI_WIN_CTRL_MENU_HOTKEY_ARRAY HotkeyArray,
    __in PYORI_WIN_MENU_ENTRY Input,
    __in DWORD MaxCaption,
    __out PYORI_WIN_CTRL_MENU_ENTRY Output
    )
{
    DWORD CharsNeeded;
    DWORD Index;
    YORI_WIN_CTRL_MENU_HOTKEY Hotkey;

    if (Input->Hotkey.LengthInChars > 0) {
        CharsNeeded = MaxCaption + 2 + Input->Hotkey.LengthInChars + 1;
        if (!YoriWinMenuGenerateHotkey(&Input->Hotkey, &Hotkey)) {
            return FALSE;
        }
    } else {
        CharsNeeded = Input->Caption.LengthInChars + 1;
    }

    if (!YoriLibAllocateString(&Output->DisplayCaption, CharsNeeded)) {
        return FALSE;
    }

    YoriWinLabelParseAccelerator(&Input->Caption,
                                 &Output->DisplayCaption,
                                 &Output->AcceleratorChar,
                                 &Output->AcceleratorOffset,
                                 NULL);

    if (Input->Hotkey.LengthInChars > 0) {
        for (Index = Output->DisplayCaption.LengthInChars; Index < MaxCaption + 2; Index++) {
            Output->DisplayCaption.StartOfString[Index] = ' ';
        }

        memcpy(&Output->DisplayCaption.StartOfString[Index], Input->Hotkey.StartOfString, Input->Hotkey.LengthInChars * sizeof(TCHAR));
        Output->DisplayCaption.LengthInChars = Index + Input->Hotkey.LengthInChars;
        Output->DisplayCaption.StartOfString[Output->DisplayCaption.LengthInChars] = '\0';

        Hotkey.EntryToInvoke = Output;
        YoriWinMenuAddHotkeyToArray(HotkeyArray, &Hotkey);
    }

    Output->NotifyCallback = Input->NotifyCallback;
    Output->Flags = Input->Flags;

    if (Input->ChildMenu.ItemCount > 0) {
        if (!YoriWinMenuCopySubMenu(HotkeyArray, Input, Output)) {
            if (Input->Hotkey.LengthInChars > 0) {
                YoriWinMenuRemoveHotkeyFromArray(HotkeyArray, Output);
            }
            YoriLibFreeStringContents(&Output->DisplayCaption);
            return FALSE;
        }
    } else {
        Output->ChildItemCount = 0;
        Output->ChildItems = NULL;
    }

    return TRUE;
}

/**
 Copy an array of menu items from the caller provided format into a format
 used by the control.  Note that this routine will recursively copy any sub
 menus.

 @param HotkeyArray Pointer to an array of known hotkeys which can be updated
        to contain references to these menu items.

 @param SourceArray Pointer to the caller provided array of menu items.

 @param DestArray Pointer to a caller allocated array of menu items in the
        format used by the control.

 @param ItemCount Specifies the number of items in both arrays.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMenuCopyMultipleItems(
    __in PYORI_WIN_CTRL_MENU_HOTKEY_ARRAY HotkeyArray,
    __in PYORI_WIN_MENU_ENTRY SourceArray,
    __out_ecount(ItemCount) PYORI_WIN_CTRL_MENU_ENTRY DestArray,
    __in DWORD ItemCount
    )
{
    DWORD Index;
    DWORD MaxCaption;
    DWORD MaxHotkey;

    MaxCaption = 0;
    MaxHotkey = 0;

    //
    //  MSFIX MaxCaption here includes any ampersands that will be
    //  removed later.  The display allocation doesn't need to
    //  include these, nor does alignment.
    //

    for (Index = 0; Index < ItemCount; Index++) {
        if (SourceArray[Index].Caption.LengthInChars > MaxCaption) {
            MaxCaption = SourceArray[Index].Caption.LengthInChars;
        }

        if (SourceArray[Index].Hotkey.LengthInChars > MaxHotkey) {
            MaxHotkey = SourceArray[Index].Hotkey.LengthInChars;
        }
    }


    for (Index = 0; Index < ItemCount; Index++) {
        if (!YoriWinMenuCopyUserEntry(HotkeyArray, &SourceArray[Index], MaxCaption, &DestArray[Index])) {
            for (;Index > 0; Index--) {
                YoriWinMenuFreeMenuEntry(HotkeyArray, &DestArray[Index - 1]);
            }
            return FALSE;
        }
    }

    return TRUE;
}

/**
 Allocate an array corresponding to the items within a sub menu, and copy
 each of the items from a single menu item's child menu into the newly
 allocated child menu associated with a menu item in the control's format.

 @param HotkeyArray Pointer to an array of known hotkeys which can be updated
        to contain references to these menu items.

 @param Input Pointer to a caller provided menu item that contains a sub menu.

 @param Output Pointer to a caller allocated menu item.  A child menu array
        will be allocated within this entry.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriWinMenuCopySubMenu(
    __in PYORI_WIN_CTRL_MENU_HOTKEY_ARRAY HotkeyArray,
    __in PYORI_WIN_MENU_ENTRY Input,
    __inout PYORI_WIN_CTRL_MENU_ENTRY Output
    )
{
    PYORI_WIN_CTRL_MENU_ENTRY NewItems;

    NewItems = YoriLibReferencedMalloc(Input->ChildMenu.ItemCount * sizeof(YORI_WIN_CTRL_MENU_ENTRY));
    if (NewItems == NULL) {
        return FALSE;
    }

    if (!YoriWinMenuCopyMultipleItems(HotkeyArray, Input->ChildMenu.Items, NewItems, Input->ChildMenu.ItemCount)) {
        YoriLibDereference(NewItems);
        return FALSE;
    }

    Output->ChildItems = NewItems;
    Output->ChildItemCount = Input->ChildMenu.ItemCount;

    return TRUE;
}

// ==================================================================
// Popup menu control
// ==================================================================


/**
 A structure of information to populate with information about the action to
 perform when a popup menu has completed and the user has indicated an item
 to execute or another action to perform.
 */
typedef struct _YORI_WIN_MENU_OUTCOME {
    /**
     The type of the outcome.
     */
    enum {
        YoriWinMenuOutcomeCancel                = 1,
        YoriWinMenuOutcomeExecute               = 2,
        YoriWinMenuOutcomeMenuLeft              = 3,
        YoriWinMenuOutcomeMenuRight             = 4,
    } Outcome;

    union {
        struct {
            /**
             Pointer to a callback function to invoke if the outcome of the
             popup menu is that it should execute an item.
             */
            PYORI_WIN_NOTIFY Callback;
        } Execute;
    };
} YORI_WIN_MENU_OUTCOME, *PYORI_WIN_MENU_OUTCOME;

/**
 A structure describing the contents of a popup menu control.
 */
typedef struct _YORI_WIN_CTRL_MENU_POPUP {

    /**
     A common header for all controls
     */
    YORI_WIN_CTRL Ctrl;

    /**
     When the control terminates the parent window, this structure is
     populated with information about the action to perform.
     */
    PYORI_WIN_MENU_OUTCOME Outcome;

    /**
     Information about the complete heirarchy of menu options.
     */
    PYORI_WIN_CTRL_MENU_ENTRY Items;

    /**
     the number of elements in the Items and ItemCtrlArray arrays.
     */
    DWORD ItemCount;

    /**
     Specifies the array index of any currently active menu item.
     */
    DWORD ActiveItemIndex;

    /**
     TRUE if the menu popup should highlight a specific menu item because
     it is active.  FALSE if the menu popup contains no active menu item.
     */
    BOOLEAN ActiveMenuItem;

} YORI_WIN_CTRL_MENU_POPUP, *PYORI_WIN_CTRL_MENU_POPUP;

/**
 Return relevant key combinations that would affect the operation of the
 menu.  As in, ignore num lock and the rest.

 @param ControlMask The mask of currently held control keys.

 @return The mask of control keys that the menu should act upon.
 */
DWORD
YoriWinMenuControlMask(
    __in DWORD ControlMask
    )
{
    return (ControlMask & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED | SHIFT_PRESSED));
}


/**
 Draw the popup menu control with its current state applied.

 @param MenuPopup Pointer to the menu popup to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMenuPopupPaint(
    __in PYORI_WIN_CTRL_MENU_POPUP MenuPopup
    )
{
    DWORD Index;
    DWORD CharIndex;
    WORD TextAttributes;
    WORD ItemAttributes;
    WORD CharAttributes;
    PYORI_WIN_CTRL_MENU_ENTRY Item;
    COORD ClientSize;
    CONST TCHAR* Chars;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;

    TextAttributes = MenuPopup->Ctrl.DefaultAttributes;
    YoriWinGetControlClientSize(&MenuPopup->Ctrl, &ClientSize);

    YoriWinDrawBorderOnControl(&MenuPopup->Ctrl, &MenuPopup->Ctrl.ClientRect, TextAttributes, YORI_WIN_BORDER_TYPE_SINGLE);
    WinMgrHandle = YoriWinGetWindowManagerHandle(YoriWinGetTopLevelWindow(&MenuPopup->Ctrl));
    Chars = YoriWinGetDrawingCharacters(WinMgrHandle, YoriWinCharsMenu);

    for (Index = 0; Index < MenuPopup->ItemCount; Index++) {
        Item = &MenuPopup->Items[Index];
        ItemAttributes = TextAttributes;
        if (MenuPopup->ActiveMenuItem &&
            Index == MenuPopup->ActiveItemIndex) {

            ItemAttributes = (WORD)(((ItemAttributes & 0xF0) >> 4) | ((ItemAttributes & 0x0F) << 4));
        } else if (Item->Flags & YORI_WIN_MENU_ENTRY_DISABLED) {
            ItemAttributes = (WORD)((ItemAttributes & 0xF0) | FOREGROUND_INTENSITY);
        }

        if (Item->Flags & YORI_WIN_MENU_ENTRY_SEPERATOR) {
            YoriWinSetControlClientCell(&MenuPopup->Ctrl, 0, (WORD)(Index + 1), Chars[0], ItemAttributes);
            for (CharIndex = 1; (SHORT)CharIndex < ClientSize.X - 1; CharIndex++) {
                YoriWinSetControlClientCell(&MenuPopup->Ctrl, (WORD)(CharIndex), (WORD)(Index + 1), Chars[1], ItemAttributes);
            }
            YoriWinSetControlClientCell(&MenuPopup->Ctrl, (WORD)(CharIndex), (WORD)(Index + 1), Chars[2], ItemAttributes);
        } else {
            YoriWinSetControlClientCell(&MenuPopup->Ctrl, 1, (WORD)(Index + 1), ' ', ItemAttributes);
            if (Item->Flags & YORI_WIN_MENU_ENTRY_CHECKED) {
                YoriWinSetControlClientCell(&MenuPopup->Ctrl, 2, (WORD)(Index + 1), Chars[3], ItemAttributes);
            } else {
                YoriWinSetControlClientCell(&MenuPopup->Ctrl, 2, (WORD)(Index + 1), ' ', ItemAttributes);
            }
            YoriWinSetControlClientCell(&MenuPopup->Ctrl, 3, (WORD)(Index + 1), ' ', ItemAttributes);
            for (CharIndex = 0; CharIndex < Item->DisplayCaption.LengthInChars; CharIndex++) {
                CharAttributes = ItemAttributes;
                if ((Item->Flags & YORI_WIN_MENU_ENTRY_DISABLED) == 0 &&
                    Item->AcceleratorChar != '\0' &&
                    Item->AcceleratorOffset == CharIndex) {

                    CharAttributes = (WORD)((CharAttributes & 0xF0) | (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY));
                }
                YoriWinSetControlClientCell(&MenuPopup->Ctrl, (WORD)(CharIndex + 4), (WORD)(Index + 1), Item->DisplayCaption.StartOfString[CharIndex], CharAttributes);
            }

            for (; (SHORT)CharIndex < ClientSize.X - 5; CharIndex++) {
                YoriWinSetControlClientCell(&MenuPopup->Ctrl, (WORD)(CharIndex + 4), (WORD)(Index + 1), ' ', ItemAttributes);
            }
        }

    }
    return TRUE;
}

/**
 Return TRUE to indicate that a popup menu item can be highlighted, or FALSE
 to indicate that it cannot be active.

 @param MenuPopup Pointer to the popup menu control.

 @param Index Specifies the index of the menu item to test for whether it can
        be active or not.

 @return TRUE to indicate the specified item can be highlighted, or FALSE if
         it cannot be highlighted.
 */
BOOLEAN
YoriWinMenuPopupCanItemBeActive(
    __in PYORI_WIN_CTRL_MENU_POPUP MenuPopup,
    __in DWORD Index
    )
{
    PYORI_WIN_CTRL_MENU_ENTRY Item;
    Item = &MenuPopup->Items[Index];
    if (Item->Flags & (YORI_WIN_MENU_ENTRY_SEPERATOR | YORI_WIN_MENU_ENTRY_DISABLED)) {
        return FALSE;
    }

    return TRUE;
}

/**
 Set the next item in the menu popup control to be active.  If no item is
 currently active, the first item is made active.  Note that if no item is
 capable of being activated, this routine can return without activating
 any item.

 @param MenuPopup Pointer to the menu popup control.
 */
VOID
YoriWinMenuPopupSetNextItemActive(
    __in PYORI_WIN_CTRL_MENU_POPUP MenuPopup
    )
{
    DWORD ProbeIndex;
    DWORD TerminateIndex;
    PYORI_WIN_CTRL_MENU_ENTRY Item;

    if (MenuPopup->ItemCount == 0) {
        MenuPopup->ActiveMenuItem = FALSE;
        return;
    }

    if (MenuPopup->ActiveMenuItem) {
        TerminateIndex = MenuPopup->ActiveItemIndex;
        ProbeIndex = (TerminateIndex + 1) % MenuPopup->ItemCount;
        if (ProbeIndex == TerminateIndex) {
            return;
        }
    } else {
        ProbeIndex = 0;
        TerminateIndex = 0;
    }

    do {
        Item = &MenuPopup->Items[ProbeIndex];
        if (!YoriWinMenuPopupCanItemBeActive(MenuPopup, ProbeIndex)) {
            ProbeIndex = (ProbeIndex + 1) % MenuPopup->ItemCount;
        } else {
            MenuPopup->ActiveMenuItem = TRUE;
            MenuPopup->ActiveItemIndex = ProbeIndex;
            break;
        }

        if (ProbeIndex == TerminateIndex) {
            if (!YoriWinMenuPopupCanItemBeActive(MenuPopup, ProbeIndex)) {
                MenuPopup->ActiveMenuItem = FALSE;
            } else {
                MenuPopup->ActiveItemIndex = ProbeIndex;
                MenuPopup->ActiveMenuItem = TRUE;
            }
            break;
        }
    } while(TRUE);
}

/**
 Set the previous item in the menu popup control to be active.  If no item
 is currently active, the last item is made active.  Note that if no item is
 capable of being activated, this routine can return without activating any
 item.

 @param MenuPopup Pointer to the menu popup control.
 */
VOID
YoriWinMenuPopupSetPreviousItemActive(
    __in PYORI_WIN_CTRL_MENU_POPUP MenuPopup
    )
{
    DWORD ProbeIndex;
    DWORD TerminateIndex;
    PYORI_WIN_CTRL_MENU_ENTRY Item;

    if (MenuPopup->ItemCount == 0) {
        MenuPopup->ActiveMenuItem = FALSE;
        return;
    }

    if (MenuPopup->ActiveMenuItem) {
        TerminateIndex = MenuPopup->ActiveItemIndex;
        if (TerminateIndex == 0) {
            ProbeIndex = MenuPopup->ItemCount - 1;
        } else {
            ProbeIndex = TerminateIndex - 1;
        }
        if (ProbeIndex == TerminateIndex) {
            return;
        }
    } else {
        ProbeIndex = MenuPopup->ItemCount - 1;
        TerminateIndex = MenuPopup->ItemCount - 1;
    }

    do {
        Item = &MenuPopup->Items[ProbeIndex];
        if (!YoriWinMenuPopupCanItemBeActive(MenuPopup, ProbeIndex)) {
            if (ProbeIndex == 0) {
                ProbeIndex = MenuPopup->ItemCount - 1;
            } else {
                ProbeIndex = ProbeIndex - 1;
            }
        } else {
            MenuPopup->ActiveMenuItem = TRUE;
            MenuPopup->ActiveItemIndex = ProbeIndex;
            break;
        }

        if (ProbeIndex == TerminateIndex) {
            if (!YoriWinMenuPopupCanItemBeActive(MenuPopup, ProbeIndex)) {
                MenuPopup->ActiveMenuItem = FALSE;
            } else {
                MenuPopup->ActiveItemIndex = ProbeIndex;
                MenuPopup->ActiveMenuItem = TRUE;
            }
            break;
        }
    } while(TRUE);
}

/**
 Set a specific item in a popup menu to be the highlighted item.  This may
 not be possible if the specified item does not support being highlighted.

 @param MenuPopup Pointer to the popup menu control.

 @param ProbeIndex Specifies the index of the item that should be highlighted
        if possible.
 */
VOID
YoriWinMenuPopupSetActiveItem(
    __in PYORI_WIN_CTRL_MENU_POPUP MenuPopup,
    __in DWORD ProbeIndex
    )
{
    if (ProbeIndex >= MenuPopup->ItemCount) {
        return;
    }

    if (!YoriWinMenuPopupCanItemBeActive(MenuPopup, ProbeIndex)) {
        return;
    }

    MenuPopup->ActiveMenuItem = TRUE;
    MenuPopup->ActiveItemIndex = ProbeIndex;
}

/**
 Set the action to perform when the popup menu is closed, and initiate
 closure of the popup menu.

 @param MenuPopup Pointer to the popup menu control.

 @param Index Specifies the index of the item within the popup menu to invoke.
 */
VOID
YoriWinMenuPopupInvokeItem(
    __in PYORI_WIN_CTRL_MENU_POPUP MenuPopup,
    __in DWORD Index
    )
{
    PYORI_WIN_WINDOW Window;

    Window = YoriWinGetTopLevelWindow(&MenuPopup->Ctrl);

    if (MenuPopup->Items[Index].NotifyCallback != NULL) {
        MenuPopup->Outcome->Outcome = YoriWinMenuOutcomeExecute;
        MenuPopup->Outcome->Execute.Callback = MenuPopup->Items[Index].NotifyCallback;
        YoriWinCloseWindow(Window, 1);
    }
}

/**
 Process input events for a menu popup control.

 @param Ctrl Pointer to the menu popup control.

 @param Event Pointer to the input event.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate that regular processing
         should continue (although this does not imply that no processing
         has already occurred.)
 */
BOOLEAN
YoriWinMenuPopupEventHandler(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_CTRL_MENU_POPUP MenuPopup;
    PYORI_WIN_WINDOW Window;

    MenuPopup = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MENU_POPUP, Ctrl);
    switch(Event->EventType) {
        case YoriWinEventParentDestroyed:
            YoriWinDestroyControl(Ctrl);
            YoriLibDereference(MenuPopup);
            break;
        case YoriWinEventKeyDown:
            if (Event->KeyDown.VirtualKeyCode == VK_DOWN) {
                YoriWinMenuPopupSetNextItemActive(MenuPopup);
                YoriWinMenuPopupPaint(MenuPopup);
            } else if (Event->KeyDown.VirtualKeyCode == VK_UP) {
                YoriWinMenuPopupSetPreviousItemActive(MenuPopup);
                YoriWinMenuPopupPaint(MenuPopup);
            } else if (Event->KeyDown.VirtualKeyCode == VK_LEFT) {
                Window = YoriWinGetTopLevelWindow(Ctrl);
                MenuPopup->Outcome->Outcome = YoriWinMenuOutcomeMenuLeft;
                YoriWinCloseWindow(Window, 1);
            } else if (Event->KeyDown.VirtualKeyCode == VK_RIGHT) {
                Window = YoriWinGetTopLevelWindow(Ctrl);
                MenuPopup->Outcome->Outcome = YoriWinMenuOutcomeMenuRight;
                YoriWinCloseWindow(Window, 1);
            } else if (Event->KeyDown.VirtualKeyCode == VK_RETURN) {
                if (MenuPopup->ActiveMenuItem) {
                    YoriWinMenuPopupInvokeItem(MenuPopup, MenuPopup->ActiveItemIndex);
                }
            } else if (Event->KeyDown.VirtualKeyCode == VK_ESCAPE) {
                Window = YoriWinGetTopLevelWindow(Ctrl);
                MenuPopup->Outcome->Outcome = YoriWinMenuOutcomeCancel;
                YoriWinCloseWindow(Window, 1);
            } else if ((Event->KeyDown.CtrlMask & ~(LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED | SHIFT_PRESSED)) == 0) {
                DWORD Index;
                for (Index = 0; Index < MenuPopup->ItemCount; Index++) {
                    if ((MenuPopup->Items[Index].Flags & YORI_WIN_MENU_ENTRY_DISABLED) == 0 &&
                        MenuPopup->Items[Index].AcceleratorChar != '\0' &&
                        YoriLibUpcaseChar(Event->KeyDown.Char) == YoriLibUpcaseChar(MenuPopup->Items[Index].AcceleratorChar)) {
                        YoriWinMenuPopupInvokeItem(MenuPopup, Index);
                        break;
                    }
                }

            }
            break;
        case YoriWinEventMouseDownInClient:
            if (YoriWinMenuControlMask(Event->MouseDown.ControlKeyState) == 0 &&
                Event->MouseDown.ButtonsPressed & FROM_LEFT_1ST_BUTTON_PRESSED &&
                Event->MouseDown.Location.Y >= 1 && (DWORD)Event->MouseDown.Location.Y <= MenuPopup->ItemCount) {

                YoriWinMenuPopupSetActiveItem(MenuPopup, Event->MouseDown.Location.Y - 1);
                YoriWinMenuPopupPaint(MenuPopup);
            }
            break;
        case YoriWinEventMouseUpInClient:
            if (YoriWinMenuControlMask(Event->MouseUp.ControlKeyState) == 0 &&
                Event->MouseUp.ButtonsReleased & FROM_LEFT_1ST_BUTTON_PRESSED &&
                MenuPopup->ActiveMenuItem &&
                MenuPopup->ActiveItemIndex == (DWORD)(Event->MouseUp.Location.Y - 1)) {

                YoriWinMenuPopupInvokeItem(MenuPopup, MenuPopup->ActiveItemIndex);
            } 
            break;
    }

    return FALSE;
}

/**
 Create a popup menu control and add it to a window.  This is destroyed when
 the window is destroyed.

 @param ParentHandle Pointer to the parent control.

 @param Size Specifies the size and location of the control within the parent
        window.  Typically for this control this should be the entire client
        area of the parent.

 @param Items Specifies the set of items to display within the control.  This
        memory is allocated and owned by the caller (ie., this control will
        not free it.)

 @param ItemCount Specifies the number of items in the Items array.

 @param Outcome On successful completion, populated with information about
        the action to perform.

 @param Style Specifies style flags for the popup menu.

 @return Pointer to the newly created control or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinMenuPopupCreate(
    __in PYORI_WIN_CTRL_HANDLE ParentHandle,
    __in PSMALL_RECT Size,
    __in PYORI_WIN_CTRL_MENU_ENTRY Items,
    __in DWORD ItemCount,
    __in PYORI_WIN_MENU_OUTCOME Outcome,
    __in DWORD Style
    )
{
    PYORI_WIN_CTRL_MENU_POPUP MenuPopup;
    PYORI_WIN_CTRL Parent;

    UNREFERENCED_PARAMETER(Style);

    Parent = (PYORI_WIN_CTRL)ParentHandle;

    MenuPopup = YoriLibReferencedMalloc(sizeof(YORI_WIN_CTRL_MENU_POPUP));
    if (MenuPopup == NULL) {
        return NULL;
    }

    ZeroMemory(MenuPopup, sizeof(YORI_WIN_CTRL_MENU_POPUP));
    MenuPopup->Items = Items;
    MenuPopup->ItemCount = ItemCount;
    MenuPopup->Ctrl.NotifyEventFn = YoriWinMenuPopupEventHandler;
    MenuPopup->Outcome = Outcome;
    if (!YoriWinCreateControl(Parent, Size, TRUE, &MenuPopup->Ctrl)) {
        YoriLibDereference(MenuPopup);
        return NULL;
    }

    YoriWinMenuPopupSetNextItemActive(MenuPopup);
    YoriWinMenuPopupPaint(MenuPopup);
    YoriWinSetControlId(MenuPopup, 1);

    return &MenuPopup->Ctrl;
}

/**
 Calculate the area required to display the specified set of items.  This is
 used to size the parent window before the menu popup control can be created.

 @param Items Pointer to the set of items to display.

 @param ItemCount Specifies the number of items in the Items array.

 @param SizeNeeded On successful completion, populated with the width and
        height of the control needed to display the set of items.
 */
VOID
YoriWinMenuGetPopupSizeNeededForItems(
    __in PYORI_WIN_CTRL_MENU_ENTRY Items,
    __in DWORD ItemCount,
    __out PCOORD SizeNeeded
    )
{
    DWORD LongestChildItem;
    DWORD Index;

    LongestChildItem = 0;
    for (Index = 0; Index < ItemCount; Index++) {
        if (Items[Index].DisplayCaption.LengthInChars > LongestChildItem) {
            LongestChildItem = Items[Index].DisplayCaption.LengthInChars;
        }
    }

    //
    //  The size needed is one char for the border, three chars to the left
    //  of each item for a space, status, and another space, the longest item,
    //  three spaces to the right of that, and a border character.
    //

    SizeNeeded->X = (SHORT)(1 + 3 + LongestChildItem + 3 + 1);
    SizeNeeded->Y = (SHORT)(ItemCount + 2);
}



// ==================================================================
// Popup menu window
// ==================================================================

/**
 A function to be invoked when an event of interest occurs when the menu popup
 window is displayed.

 @param Ctrl Pointer to the window control.

 @param Event Pointer to the event information.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate regular processing
         should continue.
 */
BOOLEAN
YoriWinMenuPopupChildEvent(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_WINDOW Window;
    PYORI_WIN_CTRL_MENU_POPUP MenuPopup;

    Window = YoriWinGetWindowFromWindowCtrl(Ctrl);
    MenuPopup = YoriWinFindControlById(Ctrl, 1);
    if (MenuPopup == NULL) {
        return FALSE;
    }

    switch(Event->EventType) {
        case YoriWinEventMouseDownOutsideWindow:
            MenuPopup->Outcome->Outcome = YoriWinMenuOutcomeCancel;
            YoriWinCloseWindow(Window, 1);
            break;
    }
    return FALSE;
}

// ==================================================================
// Menubar control
// ==================================================================

/**
 A structure describing the contents of a menubar control.
 */
typedef struct _YORI_WIN_CTRL_MENUBAR {

    /**
     A common header for all controls
     */
    YORI_WIN_CTRL Ctrl;

    /**
     Information about the complete heirarchy of menu options.
     */
    PYORI_WIN_CTRL_MENU_ENTRY Items;

    /**
     the number of elements in the Items and ItemCtrlArray arrays.
     */
    DWORD ItemCount;

    /**
     Specifies the array index of any currently active menu item.  Note this
     is only meaningful if ActiveMenuItem is TRUE.
     */
    DWORD ActiveItemIndex;

    /**
     An array of hotkeys that could reside anywhere within the menu bar
     heirarchy.
     */
    YORI_WIN_CTRL_MENU_HOTKEY_ARRAY HotkeyArray;

    /**
     TRUE if the menu should display the accelerator character, FALSE if it
     should not.  This becomes TRUE when the user presses the Alt key.
     */
    BOOLEAN DisplayAccelerator;

    /**
     TRUE if the menu bar should highlight a specific menu bar entry because
     it is active, typically implying that menu has a popup menu displayed.
     FALSE if the menu bar contains no active menu item.
     */
    BOOLEAN ActiveMenuItem;

} YORI_WIN_CTRL_MENUBAR, *PYORI_WIN_CTRL_MENUBAR;

/**
 Draw the menubar with its current state applied.

 @param MenuBar Pointer to the menubar to draw.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMenuBarPaint(
    __in PYORI_WIN_CTRL_MENUBAR MenuBar
    )
{
    DWORD ItemIndex;
    DWORD CharIndex;
    WORD CellIndex;
    WORD TextAttributes;
    WORD ItemAttributes;
    WORD AcceleratorAttributes;
    COORD ClientSize;

    TextAttributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;

    YoriWinGetControlClientSize(&MenuBar->Ctrl, &ClientSize);
    for (CellIndex = 0; CellIndex < 1; CellIndex++) {
        YoriWinSetControlClientCell(&MenuBar->Ctrl, CellIndex, 0, ' ', TextAttributes);
    }

    for (ItemIndex = 0; ItemIndex < MenuBar->ItemCount; ItemIndex++) {

        ItemAttributes = TextAttributes;

        if (MenuBar->ActiveMenuItem && ItemIndex == MenuBar->ActiveItemIndex) {
            ItemAttributes = (WORD)(((ItemAttributes & 0xf0) >> 4) | ((ItemAttributes & 0x0f) << 4));
        }
        YoriWinSetControlClientCell(&MenuBar->Ctrl, CellIndex, 0, ' ', ItemAttributes);
        CellIndex++;
        for (CharIndex = 0; CharIndex < MenuBar->Items[ItemIndex].DisplayCaption.LengthInChars; CharIndex++) {
            if (MenuBar->DisplayAccelerator &&
                MenuBar->Items[ItemIndex].AcceleratorChar != '\0' &&
                CharIndex == MenuBar->Items[ItemIndex].AcceleratorOffset) {

                AcceleratorAttributes = (WORD)(ItemAttributes & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE) | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                YoriWinSetControlClientCell(&MenuBar->Ctrl, CellIndex, 0, MenuBar->Items[ItemIndex].DisplayCaption.StartOfString[CharIndex], AcceleratorAttributes);
            } else {
                YoriWinSetControlClientCell(&MenuBar->Ctrl, CellIndex, 0, MenuBar->Items[ItemIndex].DisplayCaption.StartOfString[CharIndex], ItemAttributes);
            }
            CellIndex++;
        }
        YoriWinSetControlClientCell(&MenuBar->Ctrl, CellIndex, 0, ' ', ItemAttributes);
        CellIndex++;

    }

    for (;CellIndex < ClientSize.X; CellIndex++) {
        YoriWinSetControlClientCell(&MenuBar->Ctrl, CellIndex, 0, ' ', TextAttributes);
    }


    return TRUE;
}

/**
 Display a popup menu associated with a top level menu bar submenu.

 @param MenuBar Pointer to the menu bar control.

 @param ItemIndex Specifies the array index within the menu bar control of
        the item whose child menu should be displayed.

 @param Outcome On successful completion, populated with information about
        the action to perform.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMenuBarOpenMenu(
    __in PYORI_WIN_CTRL_MENUBAR MenuBar,
    __in DWORD ItemIndex,
    __in PYORI_WIN_MENU_OUTCOME Outcome
    )
{
    PYORI_WIN_CTRL Ctrl;
    COORD CtrlCoord;
    COORD ScreenCoord;
    PYORI_WIN_WINDOW TopLevelWindow;
    PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle;
    PYORI_WIN_WINDOW_HANDLE PopupWindow;
    PYORI_WIN_CTRL MenuPopup;
    SMALL_RECT ChildRect;
    SMALL_RECT MenuPopupRect;
    DWORD_PTR ChildResult;
    PYORI_WIN_CTRL_MENU_ENTRY ChildItems;
    DWORD ChildItemCount;
    WORD HorizontalOffset;
    DWORD Index;
    COORD ClientSize;

    Ctrl = &MenuBar->Ctrl;
    TopLevelWindow = YoriWinGetTopLevelWindow(Ctrl);
    MenuBar->ActiveMenuItem = TRUE;
    MenuBar->ActiveItemIndex = ItemIndex;
    YoriWinMenuBarPaint(MenuBar);
    YoriWinDisplayWindowContents(TopLevelWindow);

    CtrlCoord.X = 0;
    CtrlCoord.Y = 0;
    YoriWinTranslateCtrlCoordinatesToScreenCoordinates(Ctrl, FALSE, CtrlCoord, &ScreenCoord);
    WinMgrHandle = YoriWinGetWindowManagerHandle(TopLevelWindow);

    ChildItems = MenuBar->Items[ItemIndex].ChildItems;
    ChildItemCount = MenuBar->Items[ItemIndex].ChildItemCount;

    YoriWinMenuGetPopupSizeNeededForItems(ChildItems, ChildItemCount, &ClientSize);

    HorizontalOffset = 0;
    for (Index = 0; Index < ItemIndex; Index++) {
        HorizontalOffset = (WORD)(HorizontalOffset + MenuBar->Items[Index].DisplayCaption.LengthInChars + 2);
    }

    // 
    //  MSFIX: Check if the popup doesn't fit on the screen and move it left
    //  if necessary.  If it doesn't fit vertically, do we need some fancy
    //  scroll thing?
    //

    ChildRect.Left = (SHORT)(ScreenCoord.X + HorizontalOffset);
    ChildRect.Top = (SHORT)(ScreenCoord.Y + 1);

    //
    //  The extra space added here is for the window shadow.  Ideally this
    //  could also be queried to avoid the hardcoded value.
    //

    ChildRect.Right = (SHORT)(ChildRect.Left + ClientSize.X + 2 - 1);
    ChildRect.Bottom = (SHORT)(ChildRect.Top + ClientSize.Y + 1 - 1);

    if (!YoriWinCreateWindowEx(WinMgrHandle, &ChildRect, YORI_WIN_WINDOW_STYLE_SHADOW_TRANSPARENT, NULL, &PopupWindow)) {
        return FALSE;
    }

    YoriWinGetControlClientSize(YoriWinGetCtrlFromWindow(PopupWindow), &ClientSize);
    MenuPopupRect.Left = 0;
    MenuPopupRect.Top = 0;
    MenuPopupRect.Right = (SHORT)(ClientSize.X - 1);
    MenuPopupRect.Bottom = (SHORT)(ClientSize.Y - 1);
    MenuPopup = YoriWinMenuPopupCreate(PopupWindow, &MenuPopupRect, ChildItems, ChildItemCount, Outcome, 0);
    if (MenuPopup == NULL) {
        YoriWinDestroyWindow(PopupWindow);
        return FALSE;
    }

    YoriWinSetCustomNotification(PopupWindow, YoriWinEventMouseDownOutsideWindow, YoriWinMenuPopupChildEvent);
    YoriWinProcessInputForWindow(PopupWindow, &ChildResult);

    YoriWinDestroyWindow(PopupWindow);

    MenuBar->ActiveMenuItem = FALSE;

    YoriWinMenuBarPaint(MenuBar);
    YoriWinDisplayWindowContents(TopLevelWindow);

    return TRUE;
}

/**
 Open a pulldown menu from a menubar control.

 @param MenuBar Pointer to the menubar control.

 @param Index specifies the index of the pulldown menu to open.

 @return TRUE to indicate a menu was opened and processed.  FALSE if no menu
         could be opened.
 */
BOOLEAN
YoriWinMenuBarExecuteTopMenu(
    __in PYORI_WIN_CTRL_MENUBAR MenuBar,
    __in DWORD Index
    )
{
    YORI_WIN_MENU_OUTCOME Outcome;
    PYORI_WIN_CTRL PriorFocusCtrl;
    PYORI_WIN_WINDOW ParentWindow;
    DWORD DisplayIndex;

    ParentWindow = YoriWinGetTopLevelWindow(&MenuBar->Ctrl);
    PriorFocusCtrl = YoriWinGetFocus(ParentWindow);
    YoriWinSetFocus(ParentWindow, NULL);

    DisplayIndex = Index;

    while(TRUE) {

        if (MenuBar->Items[DisplayIndex].NotifyCallback != NULL) {
            MenuBar->Items[DisplayIndex].NotifyCallback(&MenuBar->Ctrl);
        }

        if (MenuBar->Items[DisplayIndex].ChildItemCount > 0) {
            ZeroMemory(&Outcome, sizeof(Outcome));
            YoriWinMenuBarOpenMenu(MenuBar, DisplayIndex, &Outcome);

            if (Outcome.Outcome == YoriWinMenuOutcomeCancel) {
                break;
            } else if (Outcome.Outcome == YoriWinMenuOutcomeMenuLeft) {
                if (DisplayIndex > 0) {
                    DisplayIndex--;
                } else {
                    DisplayIndex = MenuBar->ItemCount - 1;
                }
                continue;
            } else if (Outcome.Outcome == YoriWinMenuOutcomeMenuRight) {
                DisplayIndex++;
                if (DisplayIndex == MenuBar->ItemCount) {
                    DisplayIndex = 0;
                }
                continue;
            } else if (Outcome.Outcome == YoriWinMenuOutcomeExecute) {

                if (Outcome.Execute.Callback != NULL) {
                    Outcome.Execute.Callback(&MenuBar->Ctrl);
                }
                YoriWinSetFocus(ParentWindow, PriorFocusCtrl);
                return TRUE;
            }
        }
    }

    YoriWinSetFocus(ParentWindow, PriorFocusCtrl);
    return FALSE;
}

/**
 For a specified accelerator key, scan through the menu bar looking for any
 submenu which should be activated as a result of the accelerator key.
 Unlike other controls, a menu bar can have many accelerators, and each
 perform a different task.

 @param MenuBar Pointer to the menu bar control.

 @param Char Specifies the accelerator key that was pressed.

 @return TRUE to indicate that the accelerator key was found and handled,
         FALSE to indicate that the accelerator key is not owned by this
         control.
 */
BOOLEAN
YoriWinMenuBarAccelerator(
    __in PYORI_WIN_CTRL_MENUBAR MenuBar,
    __in TCHAR Char
    )
{
    DWORD ItemIndex;


    for (ItemIndex = 0; ItemIndex < MenuBar->ItemCount; ItemIndex++) {
        if (MenuBar->Items[ItemIndex].AcceleratorChar != '\0' &&
            MenuBar->Items[ItemIndex].AcceleratorChar == Char) {

            YoriWinMenuBarExecuteTopMenu(MenuBar, ItemIndex);
            return TRUE;
        }
    }

    return FALSE;
}

/**
 For a specified hotkey, scan through any known menu items with hotkeys
 looking for a match, and if found, invoke that item's callback.  Note this
 only occurs when the menu is not displayed.

 @param MenuBar Pointer to the menu bar control.

 @param Event Specifies the event describing the key that was pressed.

 @return TRUE to indicate that the hotkey was found and handled,
         FALSE to indicate that the hotkey is not owned by this control.
 */
BOOLEAN
YoriWinMenuBarHotkey(
    __in PYORI_WIN_CTRL_MENUBAR MenuBar,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_CTRL_MENU_HOTKEY Hotkey;
    DWORD EffectiveCtrlMask;
    DWORD Index;

    EffectiveCtrlMask = Event->KeyDown.CtrlMask;

    //
    //  If right control is pressed, indicate left control is pressed
    //  for easy comparison.
    //

    if (EffectiveCtrlMask & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
        EffectiveCtrlMask = EffectiveCtrlMask & ~(RIGHT_CTRL_PRESSED) | LEFT_CTRL_PRESSED;
    }

    for (Index = 0; Index < MenuBar->HotkeyArray.Populated; Index++) {
        Hotkey = &MenuBar->HotkeyArray.Keys[Index];
        if ((EffectiveCtrlMask & Hotkey->CtrlKeyMaskToCheck) == Hotkey->CtrlKeyMaskToEqual &&
            Hotkey->VirtualKeyCode == Event->KeyDown.VirtualKeyCode) {

            if (Hotkey->EntryToInvoke->NotifyCallback != NULL) {
                Hotkey->EntryToInvoke->NotifyCallback(&MenuBar->Ctrl);
                return TRUE;
            }
        }
    }

    return FALSE;
}

/**
 Process input events for a menubar control.

 @param Ctrl Pointer to the menubar control.

 @param Event Pointer to the input event.

 @return TRUE to indicate that the event was processed and no further
         processing should occur.  FALSE to indicate that regular processing
         should continue (although this does not imply that no processing
         has already occurred.)
 */
BOOLEAN
YoriWinMenuBarEventHandler(
    __in PYORI_WIN_CTRL Ctrl,
    __in PYORI_WIN_EVENT Event
    )
{
    PYORI_WIN_CTRL_MENUBAR MenuBar;
    MenuBar = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MENUBAR, Ctrl);
    switch(Event->EventType) {
        case YoriWinEventParentDestroyed:
            if (MenuBar->Items != NULL) {
                YoriWinMenuFreeEntryArray(&MenuBar->HotkeyArray, MenuBar->Items, MenuBar->ItemCount);
                YoriLibDereference(MenuBar->Items);
                MenuBar->Items = NULL;
                MenuBar->ItemCount = 0;
            }
            if (MenuBar->HotkeyArray.Keys != NULL) {
                YoriLibDereference(MenuBar->HotkeyArray.Keys);
                MenuBar->HotkeyArray.Allocated = 0;
                MenuBar->HotkeyArray.Populated = 0;
                MenuBar->HotkeyArray.Keys = NULL;
            }
            YoriWinDestroyControl(Ctrl);
            YoriLibDereference(MenuBar);
            break;
        case YoriWinEventDisplayAccelerators:
            MenuBar->DisplayAccelerator = TRUE;
            YoriWinMenuBarPaint(MenuBar);
            break;
        case YoriWinEventHideAccelerators:
            MenuBar->DisplayAccelerator = FALSE;
            YoriWinMenuBarPaint(MenuBar);
            break;
        case YoriWinEventAccelerator:
            if (YoriWinMenuBarAccelerator(MenuBar, Event->Accelerator.Char)) {
                return TRUE;
            }
            break;
        case YoriWinEventHotKeyDown:
            if (YoriWinMenuBarHotkey(MenuBar, Event)) {
                return TRUE;
            }
            break;
        case YoriWinEventMouseDownInClient:
            if (YoriWinMenuControlMask(Event->MouseDown.ControlKeyState) == 0 &&
                Event->MouseDown.ButtonsPressed & FROM_LEFT_1ST_BUTTON_PRESSED) {

                DWORD HorizFound = 1;
                DWORD Index;
                for (Index = 0; Index < MenuBar->ItemCount; Index++) {
                    if ((DWORD)Event->MouseDown.Location.X >= HorizFound &&
                        (DWORD)Event->MouseDown.Location.X < HorizFound + MenuBar->Items[Index].DisplayCaption.LengthInChars + 2) {

                        YoriWinMenuBarExecuteTopMenu(MenuBar, Index);
                        break;
                    } else {
                        HorizFound += MenuBar->Items[Index].DisplayCaption.LengthInChars + 2;
                    }
                }
            }
            break;

    }

    return FALSE;
}


/**
 Append an array of items to a menu bar control.

 @param CtrlHandle Pointer to the menu bar control.

 @param Items Pointer to a menu which contains one or more menu entries to
        append to the end of the menu bar control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMenuBarAppendItems(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PYORI_WIN_MENU Items
    )
{
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MENUBAR MenuBar;
    DWORD NewCount;
    PYORI_WIN_CTRL_MENU_ENTRY NewItems;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MenuBar = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MENUBAR, Ctrl);

    NewCount = MenuBar->ItemCount + Items->ItemCount;
    NewItems = YoriLibReferencedMalloc(NewCount * sizeof(YORI_WIN_CTRL_MENU_ENTRY));
    if (NewItems == NULL) {
        return FALSE;
    }

    ZeroMemory(NewItems, NewCount * sizeof(YORI_WIN_CTRL_MENU_ENTRY));

    if (!YoriWinMenuCopyMultipleItems(&MenuBar->HotkeyArray, Items->Items, &NewItems[MenuBar->ItemCount], Items->ItemCount)) {
        YoriLibDereference(NewItems);
        return FALSE;
    }

    // MSFIX: Blind copy of original items, direct free existing array

    if (MenuBar->Items != NULL) {
        YoriLibDereference(MenuBar->Items);
    }

    MenuBar->Items = NewItems;
    MenuBar->ItemCount = NewCount;

    YoriWinMenuBarPaint(MenuBar);

    return TRUE;
}

/**
 Mark a specified menu item as disabled.

 @param ItemHandle Pointer to the menu item.
 */
VOID
YoriWinMenuBarDisableMenuItem(
    __in PYORI_WIN_CTRL_HANDLE ItemHandle
    )
{
    PYORI_WIN_CTRL_MENU_ENTRY Item;
    Item = (PYORI_WIN_CTRL_MENU_ENTRY)ItemHandle;
    Item->Flags = (Item->Flags | YORI_WIN_MENU_ENTRY_DISABLED);
}

/**
 Mark a specified menu item as enabled.

 @param ItemHandle Pointer to the menu item.
 */
VOID
YoriWinMenuBarEnableMenuItem(
    __in PYORI_WIN_CTRL_HANDLE ItemHandle
    )
{
    PYORI_WIN_CTRL_MENU_ENTRY Item;
    Item = (PYORI_WIN_CTRL_MENU_ENTRY)ItemHandle;
    Item->Flags = (Item->Flags & ~(YORI_WIN_MENU_ENTRY_DISABLED));
}

/**
 Mark a specified menu item as checked.

 @param ItemHandle Pointer to the menu item.
 */
VOID
YoriWinMenuBarCheckMenuItem(
    __in PYORI_WIN_CTRL_HANDLE ItemHandle
    )
{
    PYORI_WIN_CTRL_MENU_ENTRY Item;
    Item = (PYORI_WIN_CTRL_MENU_ENTRY)ItemHandle;
    Item->Flags = (Item->Flags | YORI_WIN_MENU_ENTRY_CHECKED);
}

/**
 Mark a specified menu item as enabled.

 @param ItemHandle Pointer to the menu item.
 */
VOID
YoriWinMenuBarUncheckMenuItem(
    __in PYORI_WIN_CTRL_HANDLE ItemHandle
    )
{
    PYORI_WIN_CTRL_MENU_ENTRY Item;
    Item = (PYORI_WIN_CTRL_MENU_ENTRY)ItemHandle;
    Item->Flags = (Item->Flags & ~(YORI_WIN_MENU_ENTRY_CHECKED));
}

/**
 Obtain a handle to a submenu given its parent menu and index.

 @param CtrlHandle Pointer to the menubar control.

 @param ParentItemHandle Optionally points to a parent menu.  If not
        specified, the top level menu from the menubar is used.

 @param SubIndex The index of the child menu item.

 @return Pointer to the menu item handle, or NULL if the menu item is out
         of range.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinMenuBarGetSubmenuHandle(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in_opt PYORI_WIN_CTRL_HANDLE ParentItemHandle,
    __in DWORD SubIndex
    )
{
    PYORI_WIN_CTRL_MENU_ENTRY Items;
    PYORI_WIN_CTRL Ctrl;
    PYORI_WIN_CTRL_MENUBAR MenuBar;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MenuBar = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MENUBAR, Ctrl);
    
    if (ParentItemHandle == NULL) {
        if (SubIndex < MenuBar->ItemCount) {
            return &MenuBar->Items[SubIndex];
        }
        return NULL;
    }

    Items = (PYORI_WIN_CTRL_MENU_ENTRY)ParentItemHandle;

    if (SubIndex < Items->ChildItemCount) {
        return &Items->ChildItems[SubIndex];
    }

    return NULL;
}

/**
 Set the size and location of a menu bar control, and redraw the contents.

 @param CtrlHandle Pointer to the menu bar to resize or reposition.

 @param CtrlRect Specifies the new size and position of the menu bar.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriWinMenuBarReposition(
    __in PYORI_WIN_CTRL_HANDLE CtrlHandle,
    __in PSMALL_RECT CtrlRect
    )
{
    PYORI_WIN_CTRL Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    PYORI_WIN_CTRL_MENUBAR MenuBar;

    Ctrl = (PYORI_WIN_CTRL)CtrlHandle;
    MenuBar = CONTAINING_RECORD(Ctrl, YORI_WIN_CTRL_MENUBAR, Ctrl);

    if (!YoriWinControlReposition(Ctrl, CtrlRect)) {
        return FALSE;
    }

    YoriWinMenuBarPaint(MenuBar);
    return TRUE;
}

/**
 Create a menubar control and add it to a window.  This is destroyed when the
 window is destroyed.

 @param ParentHandle Pointer to the parent control.

 @param Style Specifies style flags for the menubar.

 @return Pointer to the newly created control or NULL on failure.
 */
PYORI_WIN_CTRL_HANDLE
YoriWinMenuBarCreate(
    __in PYORI_WIN_CTRL_HANDLE ParentHandle,
    __in DWORD Style
    )
{
    PYORI_WIN_CTRL_MENUBAR MenuBar;
    PYORI_WIN_CTRL Parent;
    SMALL_RECT Size;
    COORD ParentClientSize;

    UNREFERENCED_PARAMETER(Style);

    Parent = (PYORI_WIN_CTRL)ParentHandle;

    MenuBar = YoriLibReferencedMalloc(sizeof(YORI_WIN_CTRL_MENUBAR));
    if (MenuBar == NULL) {
        return NULL;
    }

    ZeroMemory(MenuBar, sizeof(YORI_WIN_CTRL_MENUBAR));

    YoriWinGetControlClientSize(Parent, &ParentClientSize);

    Size.Left = 0;
    Size.Top = 0;
    Size.Right = (SHORT)(ParentClientSize.X - 1);
    Size.Bottom = 0;

    MenuBar->Ctrl.NotifyEventFn = YoriWinMenuBarEventHandler;
    if (!YoriWinCreateControl(Parent, &Size, FALSE, &MenuBar->Ctrl)) {
        YoriLibDereference(MenuBar);
        return NULL;
    }

    MenuBar->Ctrl.RelativeToParentClient = FALSE;
    MenuBar->Ctrl.FullRect.Top = (SHORT)(MenuBar->Ctrl.FullRect.Top + Parent->ClientRect.Top);
    MenuBar->Ctrl.FullRect.Bottom = (SHORT)(MenuBar->Ctrl.FullRect.Bottom + Parent->ClientRect.Top);
    MenuBar->Ctrl.FullRect.Left = (SHORT)(MenuBar->Ctrl.FullRect.Left + Parent->ClientRect.Left);
    MenuBar->Ctrl.FullRect.Right = (SHORT)(MenuBar->Ctrl.FullRect.Right + Parent->ClientRect.Left);
    Parent->ClientRect.Top++;

    YoriWinMenuBarPaint(MenuBar);

    return &MenuBar->Ctrl;
}


// vim:sw=4:ts=4:et:
