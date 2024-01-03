/**
 * @file libdlg/device.c
 *
 * Yori shell device selection dialog
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

#include <yoripch.h>
#include <yorilib.h>
#include <yoriwin.h>
#include <yoridlg.h>

/**
 A set of well known control IDs so the dialog can manipulate its elements.
 */
typedef enum _YORI_DLG_DEV_CONTROLS {
    YoriDlgDevControlFileText = 1,
    YoriDlgDevControlDeviceList = 2,
    YoriDlgDevControlDeviceOffset = 3,
    YoriDlgDevControlDeviceLength = 4,
    YoriDlgDevFirstCustomCombo = 5
} YORI_DLG_DEV_CONTROLS;

/**
 An entry to insert into the dialog describing a known device.
 */
typedef struct _YORI_DLG_DEV_KNOWN_DEVICE {
    /**
     The list entry for this device.  Paired with
     @ref YORI_DLG_DEV_STATE::DeviceEntryList .
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     A string to display for this device.
     */
    YORI_STRING DisplayName;
} YORI_DLG_DEV_KNOWN_DEVICE, *PYORI_DLG_DEV_KNOWN_DEVICE;

/**
 State specific to a device dialog that is in operation.  Currently this is
 process global (can we really nest open dialogs anyway?)  But this should
 be moved to a per-window allocation.
 */
typedef struct _YORI_DLG_DEV_STATE {

    /**
     Specifies the file to return to the caller.  This is a full escaped
     path when populated.
     */
    YORI_STRING FileToReturn;

    /**
     The device offset to return to the caller.  Zero indicates the start of
     the device.
     */
    DWORDLONG OffsetToReturn;

    /**
     The length to return to the caller.  Zero indicates whatever length the
     device has.
     */
    DWORDLONG LengthToReturn;

    /**
     A sorted list of entries to insert into the list of known devices.
     */
    YORI_LIST_ENTRY DeviceEntryList;

    /**
     The number of entries in DeviceEntryList.
     */
    YORI_ALLOC_SIZE_T DeviceEntryCount;

} YORI_DLG_DEV_STATE, *PYORI_DLG_DEV_STATE;

/**
 Display a dialog indicating that an offset or length is not a numeric value.

 @param Parent The parent window.
 */
VOID
YoriDlgDevWarnNonNumeric(
    __in PYORI_WIN_CTRL_HANDLE Parent
    )
{
    YORI_STRING Title;
    YORI_STRING DialogText;
    YORI_STRING ButtonText;

    YoriLibConstantString(&DialogText, _T("The offset or length is non-numeric"));
    YoriLibConstantString(&Title, _T("Error"));
    YoriLibConstantString(&ButtonText, _T("Ok"));

    YoriDlgMessageBox(YoriWinGetWindowManagerHandle(Parent),
                      &Title,
                      &DialogText,
                      1,
                      &ButtonText,
                      0,
                      0);
}


/**
 A callback invoked when the ok button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgDevOkButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_CTRL_HANDLE EditCtrl;
    YORI_STRING DeviceName;
    YORI_STRING OffsetLengthText;
    PYORI_DLG_DEV_STATE State;
    YORI_ALLOC_SIZE_T CharsConsumed;
    LONGLONG TempNumber;
    LONGLONG OffsetNumber;
    LONGLONG LengthNumber;
    DWORD Pass;

    Parent = YoriWinGetControlParent(Ctrl);
    State = YoriWinGetControlContext(Parent);

    EditCtrl = YoriWinFindControlById(Parent, YoriDlgDevControlFileText);
    ASSERT(EditCtrl != NULL);
    __analysis_assume(EditCtrl != NULL);
    YoriLibInitEmptyString(&DeviceName);
    if (!YoriWinEditGetText(EditCtrl, &DeviceName)) {
        return;
    }

    __analysis_assume(DeviceName.StartOfString != NULL);

    //
    //  Add a prefix if the user didn't provide one.
    //

    if (!YoriLibIsPathPrefixed(&DeviceName)) {
        YORI_STRING DeviceNameWithPrefix;

        if (YoriLibAllocateString(&DeviceNameWithPrefix, DeviceName.LengthInChars + 5)) {
            DeviceNameWithPrefix.LengthInChars = YoriLibSPrintfS(DeviceNameWithPrefix.StartOfString, DeviceNameWithPrefix.LengthAllocated, _T("\\\\.\\%y"), &DeviceName);
            YoriLibFreeStringContents(&DeviceName);
            memcpy(&DeviceName, &DeviceNameWithPrefix, sizeof(YORI_STRING));
        }
    }

    //
    //  Truncate any trailing slashes since this dialog is trying to
    //  open devices, not root directories
    //

    while (DeviceName.LengthInChars > 0 &&
           YoriLibIsSep(DeviceName.StartOfString[DeviceName.LengthInChars - 1])) {

        DeviceName.LengthInChars--;
        DeviceName.StartOfString[DeviceName.LengthInChars] = '\0';
    }


    OffsetNumber = 0;
    LengthNumber = 0;

    for (Pass = 0; Pass < 2; Pass++) {
        EditCtrl = YoriWinFindControlById(Parent, YoriDlgDevControlDeviceOffset + Pass);
        ASSERT(EditCtrl != NULL);
        __analysis_assume(EditCtrl != NULL);
        YoriLibInitEmptyString(&OffsetLengthText);
        if (!YoriWinEditGetText(EditCtrl, &OffsetLengthText)) {
            YoriLibFreeStringContents(&DeviceName);
            return;
        }
    
        YoriLibTrimSpaces(&OffsetLengthText);
        TempNumber = 0;
        if (OffsetLengthText.LengthInChars > 0) {
            if (!YoriLibStringToNumber(&OffsetLengthText, FALSE, &TempNumber, &CharsConsumed) ||
                CharsConsumed < OffsetLengthText.LengthInChars) {

                //
                //  With numeric edit controls, this shouldn't be possible
                //

                ASSERT(FALSE);

                YoriDlgDevWarnNonNumeric(Parent);
                YoriLibFreeStringContents(&DeviceName);
                YoriLibFreeStringContents(&OffsetLengthText);
                return;
            }
        }

        YoriLibFreeStringContents(&OffsetLengthText);
        if (Pass == 0) {
            OffsetNumber = TempNumber;
        } else {
            LengthNumber = TempNumber;
        }
    }
    
    State->OffsetToReturn = OffsetNumber;
    State->LengthToReturn = LengthNumber;
    YoriLibFreeStringContents(&State->FileToReturn);
    memcpy(&State->FileToReturn, &DeviceName, sizeof(YORI_STRING));

    YoriWinCloseWindow(Parent, TRUE);
}

/**
 A callback invoked when the cancel button is clicked.

 @param Ctrl Pointer to the button that was clicked.
 */
VOID
YoriDlgDevCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 A callback invoked when the selection within the device list changes.

 @param Ctrl Pointer to the list whose selection changed.
 */
VOID
YoriDlgDevDeviceSelectionChanged(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_ALLOC_SIZE_T ActiveOption;
    YORI_STRING String;
    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_CTRL_HANDLE EditCtrl;
    PYORI_DLG_DEV_STATE State;

    if (!YoriWinListGetActiveOption(Ctrl, &ActiveOption)) {
        return;
    }

    Parent = YoriWinGetControlParent(Ctrl);
    State = YoriWinGetControlContext(Parent);
    YoriLibInitEmptyString(&String);
    if (!YoriWinListGetItemText(Ctrl, ActiveOption, &String)) {
        return;
    }

    EditCtrl = YoriWinFindControlById(Parent, YoriDlgDevControlFileText);
    ASSERT(EditCtrl != NULL);
    __analysis_assume(EditCtrl != NULL);

    YoriWinEditSetText(EditCtrl, &String);
    YoriWinEditSetSelectionRange(EditCtrl, 0, String.LengthInChars);
    YoriLibFreeStringContents(&String);
}

/**
 Callback to invoke when an object is found as part of object manager
 enumeration.

 @param FullPath Pointer to the fully qualified path to the object.

 @param NameOnly Pointer to the name of the object within its directory.

 @param ObjectType Pointer to the type of the object.

 @param Context Pointer to the dialog state.

 @return TRUE to indicate enumeration should continue, FALSE to indicate
         enumeration should terminate immediately.
 */
BOOL
YoriDlgDevObjectFoundCallback(
    __in PCYORI_STRING FullPath,
    __in PCYORI_STRING NameOnly,
    __in PCYORI_STRING ObjectType,
    __in PVOID Context
    )
{

    BOOLEAN IsLink;
    BOOLEAN IncludeObject;
    YORI_STRING MatchArray[3];
    PYORI_DLG_DEV_STATE State;

    UNREFERENCED_PARAMETER(FullPath);

    IsLink = FALSE;
    IncludeObject = FALSE;

    //
    //  The only things we can insert are devices or links to devices.
    //  If it's any other object type, exit now.
    //

    if (YoriLibCompareStringWithLiteral(ObjectType, _T("SymbolicLink")) == 0) {
        IsLink = TRUE;
    } else if (YoriLibCompareStringWithLiteral(ObjectType, _T("Device")) != 0) {
        return TRUE;
    }

    if (NameOnly->LengthInChars == 2 && NameOnly->StartOfString[1] == ':') {
        IncludeObject = TRUE;
    }

    if (!IncludeObject) {
        YORI_ALLOC_SIZE_T MatchOffset;

        YoriLibConstantString(&MatchArray[0], _T("PhysicalDrive"));
        YoriLibConstantString(&MatchArray[1], _T("HardDisk"));
        YoriLibConstantString(&MatchArray[2], _T("CDROM"));

        if (YoriLibFindFirstMatchingSubstringInsensitive(NameOnly, sizeof(MatchArray)/sizeof(MatchArray[0]), MatchArray, &MatchOffset) != NULL &&
            MatchOffset == 0) {

            IncludeObject = TRUE;
        }
    }

    State = (PYORI_WIN_CTRL_HANDLE)Context;

    if (IncludeObject) {
        PYORI_DLG_DEV_KNOWN_DEVICE NewDevice;

        NewDevice = YoriLibReferencedMalloc(sizeof(YORI_DLG_DEV_KNOWN_DEVICE) + (NameOnly->LengthInChars + 1) * sizeof(TCHAR));
        if (NewDevice == NULL) {
            return TRUE;
        }

        YoriLibInitEmptyString(&NewDevice->DisplayName);
        NewDevice->DisplayName.MemoryToFree = NewDevice;
        YoriLibReference(NewDevice);
        NewDevice->DisplayName.StartOfString = (LPTSTR)(NewDevice + 1);
        memcpy(NewDevice->DisplayName.StartOfString, NameOnly->StartOfString, NameOnly->LengthInChars * sizeof(TCHAR));
        NewDevice->DisplayName.LengthInChars = NameOnly->LengthInChars;
        NewDevice->DisplayName.LengthAllocated = NameOnly->LengthInChars + 1;
        NewDevice->DisplayName.StartOfString[NewDevice->DisplayName.LengthInChars] = '\0';

        YoriLibAppendList(&State->DeviceEntryList, &NewDevice->ListEntry);
        State->DeviceEntryCount++;

    }

    return TRUE;
}

/**
 A callback function to invoke if object manager enumeration fails.  There's
 not much we can do here - return TRUE to indicate that enumeration should
 continue as much as possible.

 @param FullName Pointer to the directory that could not be enumerated.

 @param NtStatus Indicates the status code of the enumeration error.

 @param Context Pointer to the dialog state.

 @return TRUE to indicate enumeration should continue as much as possible,
         FALSE to terminate immediately.
 */
BOOL
YoriDlgDevObjectErrorCallback(
    __in PCYORI_STRING FullName,
    __in LONG NtStatus,
    __in PVOID Context
    )
{
    UNREFERENCED_PARAMETER(FullName);
    UNREFERENCED_PARAMETER(NtStatus);
    UNREFERENCED_PARAMETER(Context);

    return TRUE;
}

/**
 Remove all entries from the known device list.

 @param State Pointer to the dialog state, containing the known device list.
 */
VOID
YoriDlgDevClearDeviceList(
    __in PYORI_DLG_DEV_STATE State
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_DLG_DEV_KNOWN_DEVICE Device;

    while (TRUE) {
        ListEntry = YoriLibGetNextListEntry(&State->DeviceEntryList, NULL);
        if (ListEntry == NULL) {
            break;
        }

        Device = CONTAINING_RECORD(ListEntry, YORI_DLG_DEV_KNOWN_DEVICE, ListEntry);
        YoriLibRemoveListItem(&Device->ListEntry);
        YoriLibFreeStringContents(&Device->DisplayName);
        YoriLibDereference(Device);
    }

    State->DeviceEntryCount = 0;
}

/**
 Refresh the dialog by searching the system for devices, and populating the
 UI elements with those items.

 @param Dialog Pointer to the dialog window.
 */
VOID
YoriDlgDevRefreshView(
    __in PYORI_WIN_CTRL_HANDLE Dialog
    )
{
    PYORI_WIN_CTRL_HANDLE DeviceList;
    PYORI_DLG_DEV_STATE State;
    YORI_STRING SearchPath;

    DeviceList = YoriWinFindControlById(Dialog, YoriDlgDevControlDeviceList);
    ASSERT(DeviceList != NULL);
    __analysis_assume(DeviceList != NULL);

    State = YoriWinGetControlContext(Dialog);

    YoriWinListClearAllItems(DeviceList);
    YoriDlgDevClearDeviceList(State);

    YoriLibConstantString(&SearchPath, _T("\\Global??"));
    YoriLibForEachObjectEnum(&SearchPath, 0, YoriDlgDevObjectFoundCallback, YoriDlgDevObjectErrorCallback, State);

    if (State->DeviceEntryCount > 0) {
        PYORI_STRING DeviceArray;
        PYORI_LIST_ENTRY ListEntry;
        YORI_ALLOC_SIZE_T Index;
        PYORI_DLG_DEV_KNOWN_DEVICE Device;

        DeviceArray = YoriLibMalloc(sizeof(YORI_STRING) * State->DeviceEntryCount);
        if (DeviceArray == NULL) {
            return;
        }

        ListEntry = NULL;
        Index = 0;
        while (TRUE) {
            ListEntry = YoriLibGetNextListEntry(&State->DeviceEntryList, ListEntry);
            if (ListEntry == NULL) {
                break;
            }

            Device = CONTAINING_RECORD(ListEntry, YORI_DLG_DEV_KNOWN_DEVICE, ListEntry);

            YoriLibInitEmptyString(&DeviceArray[Index]);
            DeviceArray[Index].StartOfString = Device->DisplayName.StartOfString;
            DeviceArray[Index].LengthInChars = Device->DisplayName.LengthInChars;
            Index++;
        }

        ASSERT(Index == State->DeviceEntryCount);
        YoriLibSortStringArray(DeviceArray, Index);

        YoriWinListAddItems(DeviceList, DeviceArray, Index);

        YoriLibFree(DeviceArray);
    }
}

/**
 Display a device selection dialog box.

 @param WinMgrHandle Pointer to the window manager.

 @param Title The string to display in the title of the dialog.

 @param OptionCount The number of custom pull down list controls at the bottom
        of the dialog.

 @param Options Pointer to an array of structures describing each custom pull
        down list at the bottom of the dialog.

 @param Text On input, specifies an initialized string.  On output, populated
        with the path to the file that the user entered.

 @param DeviceOffset On successful completion, updated to contain the offset
        within the device to open, in bytes.

 @param DeviceLength On successful completion, updated to contain the number
        of bytes to open from within the device.

 @return TRUE to indicate that the user successfully specified a device.
         FALSE to indicate a failure or the user cancelling the operation.
 */
__success(return)
BOOLEAN
YoriDlgDevice(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in DWORD OptionCount,
    __in_opt PYORI_DLG_FILE_CUSTOM_OPTION Options,
    __inout PYORI_STRING Text,
    __out PDWORDLONG DeviceOffset,
    __out PDWORDLONG DeviceLength
    )
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    SMALL_RECT Area;
    YORI_STRING Caption;
    WORD ButtonWidth;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE Edit;
    DWORD_PTR Result;
    COORD WinMgrSize;
    YORI_DLG_DEV_STATE State;
    DWORD Index;
    DWORD LongestOptionDescription;

    if (!YoriWinGetWinMgrDimensions(WinMgrHandle, &WinMgrSize)) {
        WinMgrSize.X = 60;
        WinMgrSize.Y = 20;
    } else {
        WinMgrSize.X = (SHORT)(WinMgrSize.X * 7 / 10);
        if (WinMgrSize.X < 50) {
            WinMgrSize.X = 50;
        }

        //
        //  If the window height is less than 30 rows, use 80%.  Otherwise,
        //  use 66%.  To prevent the discontinuity, if 80% of 30 rows is
        //  larger than 66% of actual rows, use that.
        //

        if (WinMgrSize.Y < 30) {
            WinMgrSize.Y = (SHORT)(WinMgrSize.Y * 4 / 5);
        } else {
            WinMgrSize.Y = (SHORT)(WinMgrSize.Y * 2 / 3);
            if ((SHORT)(30 * 3 / 4) > WinMgrSize.Y) {
                WinMgrSize.Y = (SHORT)(30 * 4 / 5);
            }
        }
    }

    if ((WORD)WinMgrSize.Y < 13 + OptionCount) {
        WinMgrSize.Y = (SHORT)(13 + OptionCount);
    }

    YoriLibInitializeListHead(&State.DeviceEntryList);
    State.DeviceEntryCount = 0;
    YoriLibInitEmptyString(&State.FileToReturn);
    if (!YoriWinCreateWindow(WinMgrHandle, 50, (WORD)(13 + OptionCount), WinMgrSize.X, WinMgrSize.Y, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, Title, &Parent)) {
        return FALSE;
    }

    YoriWinSetControlContext(Parent, &State);
    YoriWinGetClientSize(Parent, &WindowSize);

    YoriLibConstantString(&Caption, _T("Device &Name:"));

    Area.Left = 1;
    Area.Top = 1;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);
    Area.Bottom = 1;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Top = 0;
    Area.Bottom = 2;
    Area.Left = (WORD)(Area.Right + 1);
    Area.Right = (WORD)(WindowSize.X - 2);

    YoriLibConstantString(&Caption, _T(""));

    Edit = YoriWinEditCreate(Parent, &Area, &Caption, 0);
    if (Edit == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinSetControlId(Edit, YoriDlgDevControlFileText);

    YoriLibConstantString(&Caption, _T("&Devices:"));

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Bottom = Area.Top;
    Area.Left = 3;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Left = 1;
    Area.Bottom = (WORD)(WindowSize.Y - OptionCount - 6);
    Area.Right = (WORD)(WindowSize.X - 2);

    Ctrl = YoriWinListCreate(Parent, &Area, YORI_WIN_LIST_STYLE_VSCROLLBAR | YORI_WIN_LIST_STYLE_DESELECT_ON_LOSE_FOCUS);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriWinControlSetFocusOnMouseClick(Ctrl, FALSE);
    YoriWinSetControlId(Ctrl, YoriDlgDevControlDeviceList);
    YoriWinListSetSelectionNotifyCallback(Ctrl, YoriDlgDevDeviceSelectionChanged);

    LongestOptionDescription = sizeof("Offset");
    for (Index = 0; Index < OptionCount; Index++) {
        if (Options[Index].Description.LengthInChars > LongestOptionDescription) {
            LongestOptionDescription = Options[Index].Description.LengthInChars;
        }
    }

    if (LongestOptionDescription > (DWORD)(WindowSize.X - 10)) {
        LongestOptionDescription = (DWORD)(WindowSize.X - 10);
    }


    YoriLibConstantString(&Caption, _T("O&ffset:"));

    Area.Left = 1;
    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Right = (WORD)(Area.Left + LongestOptionDescription);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("0"));

    Area.Left = (WORD)(LongestOptionDescription + 2);
    Area.Right = (WORD)(WindowSize.X - 2);

    Ctrl = YoriWinEditCreate(Parent, &Area, &Caption, YORI_WIN_EDIT_STYLE_RIGHT_ALIGN | YORI_WIN_EDIT_STYLE_NUMERIC);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, YoriDlgDevControlDeviceOffset);
    YoriWinEditSetSelectionRange(Ctrl, 0, Caption.LengthInChars);

    YoriLibConstantString(&Caption, _T("&Length:"));

    Area.Left = 1;
    Area.Top = (WORD)(Area.Bottom + 1);
    Area.Right = (WORD)(Area.Left + LongestOptionDescription);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("0"));

    Area.Left = (WORD)(LongestOptionDescription + 2);
    Area.Right = (WORD)(WindowSize.X - 2);

    Ctrl = YoriWinEditCreate(Parent, &Area, &Caption, YORI_WIN_EDIT_STYLE_RIGHT_ALIGN | YORI_WIN_EDIT_STYLE_NUMERIC);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, YoriDlgDevControlDeviceLength);
    YoriWinEditSetSelectionRange(Ctrl, 0, Caption.LengthInChars);

    for (Index = 0; Index < OptionCount; Index++) {
        Area.Left = 1;
        Area.Right = (SHORT)(Area.Left + LongestOptionDescription);
        Area.Top = (SHORT)(WindowSize.Y - 3 - OptionCount + Index);
        Area.Bottom = Area.Top;

        Ctrl = YoriWinLabelCreate(Parent, &Area, &Options[Index].Description, 0);
        if (Ctrl == NULL) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        Area.Left = (SHORT)(LongestOptionDescription + 2);
        Area.Right = (SHORT)(WindowSize.X - 2);

        ButtonWidth = 5;
        if (Options[Index].ValueCount < ButtonWidth) {
            ButtonWidth = (WORD)Options[Index].ValueCount;
        }

        Ctrl = YoriWinComboCreate(Parent, &Area, ButtonWidth, &Options[Index].Values[0].ValueText, 0, NULL);
        if (Ctrl == NULL) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }

        YoriWinSetControlId(Ctrl, YoriDlgDevFirstCustomCombo + Index);

        if (!YoriWinComboAddItems(Ctrl, (PYORI_STRING)Options[Index].Values, Options[Index].ValueCount)) {
            YoriWinDestroyWindow(Parent);
            return FALSE;
        }
        YoriWinComboSetActiveOption(Ctrl, Options[Index].SelectedValue);
    }

    ButtonWidth = (WORD)(8);

    Area.Top = (SHORT)(WindowSize.Y - 3);
    Area.Bottom = (SHORT)(Area.Top + 2);

    YoriLibConstantString(&Caption, _T("&Ok"));

    Area.Left = (SHORT)(1);
    Area.Right = (WORD)(Area.Left + 1 + ButtonWidth);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, YoriDlgDevOkButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    Area.Left = (WORD)(Area.Left + ButtonWidth + 3);
    Area.Right = (WORD)(Area.Right + ButtonWidth + 3);

    YoriLibConstantString(&Caption, _T("&Cancel"));
    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, YoriDlgDevCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        return FALSE;
    }

    YoriDlgDevRefreshView(Parent);

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    YoriDlgDevClearDeviceList(&State);

    if (Result) {
        *DeviceOffset = State.OffsetToReturn;
        *DeviceLength = State.LengthToReturn;
        if (Text->LengthAllocated >= State.FileToReturn.LengthInChars + 1) {
            memcpy(Text->StartOfString, State.FileToReturn.StartOfString, State.FileToReturn.LengthInChars * sizeof(TCHAR));
            Text->StartOfString[State.FileToReturn.LengthInChars] = '\0';
            Text->LengthInChars = State.FileToReturn.LengthInChars;
        } else {
            YoriLibFreeStringContents(Text);
            memcpy(Text, &State.FileToReturn, sizeof(YORI_STRING));
            YoriLibInitEmptyString(&State.FileToReturn);
        }

        for (Index = 0; Index < OptionCount; Index++) {
            Ctrl = YoriWinFindControlById(Parent, YoriDlgDevFirstCustomCombo + Index);
            ASSERT(Ctrl != NULL);
            __analysis_assume(Ctrl != NULL);
            YoriWinComboGetActiveOption(Ctrl, &Options[Index].SelectedValue);
        }
    }

    YoriLibFreeStringContents(&State.FileToReturn);
    YoriWinDestroyWindow(Parent);
    return (BOOLEAN)Result;
}

// vim:sw=4:ts=4:et:
