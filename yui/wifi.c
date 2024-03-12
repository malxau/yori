/**
 * @file yui/wifi.c
 *
 * Yori shell wifi controls
 *
 * Copyright (c) 2023-2024 Malcolm J. Smith
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
 A structure describing a wireless network.
 */
typedef struct _YUI_WIFI_NETWORK {

    /**
     An entry for this network in a global list of networks.  Paired with
     YUI_WIFI_CONTEXT::NetworkList.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     A string describing the profile name to use for this network.  This may
     be empty if no profile exists for the network.
     */
    YORI_STRING ProfileName;

    /**
     A string describing the network SSID (network name.)
     */
    YORI_STRING Ssid;

    /**
     The signal strength for the network, on a scale from 0-100.
     */
    WORD SignalStrength;

    /**
     TRUE if a profile exists for this network (meaning ProfileName should be
     not empty.)  FALSE if no profile exists for the network yet.
     */
    BOOLEAN ProfilePresent;

    /**
     TRUE if the adapter is currently connected to this network, FALSE if it
     is not.
     */
    BOOLEAN Connected;
} YUI_WIFI_NETWORK;

/**
 Pointer to a structure describing a network.
 */
typedef YUI_WIFI_NETWORK *PYUI_WIFI_NETWORK;

/**
 Global structure for the YUI Wifi module.
 */
typedef struct _YUI_WIFI_CONTEXT {

    /**
     Pointer to the monitor context.
     */
    PYUI_MONITOR YuiMonitor;

    /**
     A window handle for the main Wifi network window, which is a flyout on
     the right side of the display.
     */
    HWND hWnd;

    /**
     A window handle for the list of available wireless networks.
     */
    HWND hWndList;

    /**
     A window handle for the airplane mode button.
     */
    HWND hWndButtonAirplane;

    /**
     A window handle for the connect button.  Note this doubles as a
     disconnect button if the selected network is currently connected,
     but since it's not possible to connect to and disconnect from a network
     simultaneously, a single button is used to keep the display compact.
     */
    HWND hWndButtonConnect;

    /**
     A handle used by the WLanApi functions.
     */
    HANDLE WlanHandle;

    /**
     A GUID describing the network interface/network adapter.  This program
     uses the first adapter returned.  It's unclear what the best behavior
     is if multiple Wifi adapters are present, since both must be configured
     independently.
     */
    GUID Interface;

    /**
     A list of known wireless networks.
     */
    YORI_LIST_ENTRY NetworkList;

    /**
     The height of a Wifi icon, in pixels.
     */
    WORD IconHeight;

    /**
     The width of a Wifi icon, in pixels.
     */
    WORD IconWidth;

    /**
     An array of Wifi icons, from weakest to strongest.
     */
    HICON WifiIcon[5];

    /**
     An icon indicating a connection to a wireless network.
     */
    HICON WifiConnectedIcon;

    /**
     Set to TRUE once the Wifi window has been activated, which kicks off an
     asynchronous scan for networks.  This value is used to ensure the scan
     is only initiated once.  In the future, it might make sense to perform
     periodic background scans every 30 seconds or somesuch.
     */
    BOOLEAN InitialScanStarted;

    /**
     Set to TRUE if the currently selected item in the list is connected.
     This means the Connect button should display Disconnect text rather
     than Connect text.  Because the text is rendered via owner draw, any
     time this value is changed, the Connect button needs to be redrawn.
     */
    BOOLEAN SelectedItemConnected;

    /**
     Set to TRUE if airplane mode is enabled, which means the airplane mode
     button should be rendered as pressed.  Set to FALSE if airplane mode is
     disabled and the button should be rendered as un-pressed.
     */
    BOOLEAN AirplaneModeEnabled;

    /**
     Set to TRUE if the Wifi window should not close when it loses focus.
     Normally selecting another window will cloes this one, however if the
     Wifi window is going to launch a child window itself, this behavior
     should be temporarily suppressed until the user is able to deselect the
     window again.
     */
    BOOLEAN DisableAutoClose;
} YUI_WIFI_CONTEXT, *PYUI_WIFI_CONTEXT;

/**
 A global instance of the Wifi module's context.
 */
YUI_WIFI_CONTEXT YuiWifiContext;

/**
 The control identifier for the list.
 */
#define YUI_WIFI_LIST (1)

/**
 The control identifier for the connect button.
 */
#define YUI_WIFI_CONNECT (2)

/**
 The control identifier for the airplane mode button.
 */
#define YUI_WIFI_AIRPLANEMODE (3)

/**
 Deallocate a structure describing a wireless network.

 @param Network Pointer to the structure to deallocate.
 */
VOID
YuiWifiFreeNetwork(
    __in PYUI_WIFI_NETWORK Network
    )
{
    YoriLibFreeStringContents(&Network->ProfileName);
    YoriLibFreeStringContents(&Network->Ssid);
    YoriLibDereference(Network);
}

/**
 Close the Wifi window and clean up any state.  Because this module runs as
 part of the Yui process, it is important to ensure state is ready for the
 module to be reopened, so it must be cleaned thoroughly.
 */
VOID
YuiWifiClose(VOID)
{
    PYORI_LIST_ENTRY ListEntry;
    PYUI_WIFI_NETWORK Network;
    WORD Index;

    //
    //  Close the main window.  This should destroy any child windows.
    //

    if (YuiWifiContext.hWnd != NULL) {
        DestroyWindow(YuiWifiContext.hWnd);
        YuiWifiContext.hWnd = NULL;
        YuiWifiContext.hWndList = NULL;
        YuiWifiContext.hWndButtonAirplane = NULL;
        YuiWifiContext.hWndButtonConnect = NULL;
    }


    //
    //  Close the WLanApi handle.
    //

    if (YuiWifiContext.WlanHandle != NULL) {
        DllWlanApi.pWlanCloseHandle(YuiWifiContext.WlanHandle, NULL);
        YuiWifiContext.WlanHandle = NULL;
    }

    //
    //  Deallocate any known network structures.
    //

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&YuiWifiContext.NetworkList, ListEntry);
    while (ListEntry != NULL) {
        Network = CONTAINING_RECORD(ListEntry, YUI_WIFI_NETWORK, ListEntry);
        ListEntry = YoriLibGetNextListEntry(&YuiWifiContext.NetworkList, ListEntry);
        YoriLibRemoveListItem(&Network->ListEntry);
        YuiWifiFreeNetwork(Network);
    }

    //
    //  Release any icons.
    //

    for (Index = 0; Index < 5; Index++) {
        if (YuiWifiContext.WifiIcon[Index] != NULL) {
            DestroyIcon(YuiWifiContext.WifiIcon[Index]);
            YuiWifiContext.WifiIcon[Index] = NULL;
        }
    }

    if (YuiWifiContext.WifiConnectedIcon != NULL) {
        DestroyIcon(YuiWifiContext.WifiConnectedIcon);
        YuiWifiContext.WifiConnectedIcon = NULL;
    }

    //
    //  Reset any variables to prepare for reuse.
    //

    YuiWifiContext.InitialScanStarted = FALSE;
    YuiWifiContext.SelectedItemConnected = FALSE;
    YuiWifiContext.DisableAutoClose = FALSE;
}

/**
 Compare two networks and determine which should be displayed "first."
 Currently any connected network appears before other networks; any known
 network with a profile appears before unknown networks; and within each
 category, networks are listed in order of network strength (strongest signal
 to weakest signal.)

 @param Lhs The first network to compare for sort order.

 @param Rhs The second network to compare for sort order.

 @return TRUE if Lhs should be displayed before Rhs.
 */
BOOLEAN
YuiWifiIsLhsLessThanRhs(
    __in PYORI_LIST_ENTRY Lhs,
    __in PYORI_LIST_ENTRY Rhs
    )
{
    PYUI_WIFI_NETWORK LhsNetwork;
    PYUI_WIFI_NETWORK RhsNetwork;

    LhsNetwork = CONTAINING_RECORD(Lhs, YUI_WIFI_NETWORK, ListEntry);
    RhsNetwork = CONTAINING_RECORD(Rhs, YUI_WIFI_NETWORK, ListEntry);
    
    if (LhsNetwork->Connected != RhsNetwork->Connected) {
        if (LhsNetwork->Connected) {
            return TRUE;
        }
        return FALSE;
    }

    if (LhsNetwork->ProfilePresent != RhsNetwork->ProfilePresent) {
        if (LhsNetwork->ProfilePresent) {
            return TRUE;
        }
        return FALSE;
    }

    if (LhsNetwork->SignalStrength > RhsNetwork->SignalStrength) {
        return TRUE;
    }

    return FALSE;
}

/**
 Insert a new network into the list of known networks, paying attention to
 sort order.  This means that the position of the insert will be determined
 based on comparisons performed within this routine.  The function signature
 is in case this should be turned into a generic YoriLib routine.

 @param ListHead Pointer to the list head.

 @param ListEntry Pointer to the new list element to insert.
 */
VOID
YuiWifiInsertListSorted(
    __in PYORI_LIST_ENTRY ListHead,
    __in PYORI_LIST_ENTRY ListEntry
    )
{
    PYORI_LIST_ENTRY ExistingEntry;

    ExistingEntry = NULL;
    while (TRUE) {
        ExistingEntry = YoriLibGetNextListEntry(ListHead, ExistingEntry);
        if (ExistingEntry == NULL) {
            YoriLibAppendList(ListHead, ListEntry);
            break;
        }

        if (YuiWifiIsLhsLessThanRhs(ListEntry, ExistingEntry)) {
            YoriLibAppendList(ExistingEntry, ListEntry);
            break;
        }
    }
}

/**
 Scan through the list of known networks and check if a new network duplicates
 an existing entry.  The WLanApi interfaces appear to return networks multiple
 times if a profile exists, to allow a network to be connected to without
 using an existing profile.  This program does not support that, so if a
 a network exists with and without a profile, the one with a profile should be
 kept.  This function implements that by keeping the "first" entry in terms of
 sort order.

 @param ListHead Pointer to the list head of known networks.

 @param NewNetwork Pointer to a new network that is about to be inserted into
        the list.

 @return TRUE if the network is a duplicate of something already in the list,
         and the new network should not be inserted.  FALSE if the new network
         is not currently in the list and should be inserted.  Note FALSE can
         be returned if a duplicate was found and removed within this routine,
         because the new network has a higher precedence to the one which was
         removed.
 */
BOOLEAN
YuiWifiCheckForDuplicates(
    __in PYORI_LIST_ENTRY ListHead,
    __in PYUI_WIFI_NETWORK NewNetwork
    )
{
    PYORI_LIST_ENTRY ExistingEntry;
    PYUI_WIFI_NETWORK ExistingNetwork;

    ExistingEntry = NULL;
    ExistingEntry = YoriLibGetNextListEntry(ListHead, ExistingEntry);
    while (ExistingEntry != NULL) {
        ExistingNetwork = CONTAINING_RECORD(ExistingEntry, YUI_WIFI_NETWORK, ListEntry);

        //
        //  Check if they match.  If they do, there's a duplicate.
        //

        if (YoriLibCompareString(&ExistingNetwork->Ssid, &NewNetwork->Ssid) == 0) {


            //
            //  If the new network is higher priority, free the existing one
            //  and tell the caller there is no more work to do.  If the new
            //  one is lower priority, tell the caller to free it.
            //

            if (YuiWifiIsLhsLessThanRhs(&NewNetwork->ListEntry, &ExistingNetwork->ListEntry)) {
                YoriLibRemoveListItem(&ExistingNetwork->ListEntry);
                YuiWifiFreeNetwork(ExistingNetwork);
                return FALSE;
            } else {
                return TRUE;
            }
        }
        ExistingEntry = YoriLibGetNextListEntry(ListHead, ExistingEntry);
    }

    return FALSE;
}

/**
 Load the list of known networks from WLanApi, create internal structures for
 them, and populate the list box with each newly found network.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiWifiPopulateList(VOID)
{
    PYORI_WLAN_AVAILABLE_NETWORK_LIST NetworkList;
    PYORI_WLAN_AVAILABLE_NETWORK FoundNetwork;
    PYORI_LIST_ENTRY ListEntry;
    PYUI_WIFI_NETWORK Network;
    YORI_ALLOC_SIZE_T ProfileLength;
    DWORD Error;
    DWORD Index;
    DWORD ConnectedIndex;
    DWORD CharIndex;

    Error = DllWlanApi.pWlanGetAvailableNetworkList(YuiWifiContext.WlanHandle,
                                                    &YuiWifiContext.Interface,
                                                    0,
                                                    NULL,
                                                    &NetworkList);

    if (Error != ERROR_SUCCESS) {
        return FALSE;
    }

    //
    //  Convert the entries from Windows format to our internal format, and
    //  insert them into a list in "sorted" order.  Sorted means any network
    //  currently connected goes first, then any known networks, then unknown
    //  networks; and within each category, signal strength from highest to
    //  lowest.
    //

    for (Index = 0; Index < NetworkList->NumberOfItems; Index++) {
        FoundNetwork = &NetworkList->Network[Index];
        if (FoundNetwork->Ssid.Length > 0) {
            ProfileLength = _tcslen(FoundNetwork->ProfileName);
            Network = YoriLibReferencedMalloc(sizeof(YUI_WIFI_NETWORK) + (ProfileLength + FoundNetwork->Ssid.Length + 2) * sizeof(TCHAR));
            if (Network != NULL) {
                ZeroMemory(Network, sizeof(YUI_WIFI_NETWORK));
                if (FoundNetwork->Flags & YORI_WLAN_AVAILABLE_NETWORK_CONNECTED) {
                    Network->Connected = TRUE;
                }
                if (FoundNetwork->Flags & YORI_WLAN_AVAILABLE_NETWORK_HAS_PROFILE) {
                    Network->ProfilePresent = TRUE;
                }
                Network->SignalStrength = (WORD)FoundNetwork->SignalQuality;
                Network->ProfileName.StartOfString = (LPTSTR)(Network + 1);
                Network->ProfileName.LengthInChars = ProfileLength;
                memcpy(Network->ProfileName.StartOfString, FoundNetwork->ProfileName, ProfileLength * sizeof(TCHAR));
                Network->ProfileName.StartOfString[Network->ProfileName.LengthInChars] = '\0';

                Network->Ssid.StartOfString = Network->ProfileName.StartOfString + ProfileLength + 1;
                for (CharIndex = 0; CharIndex < FoundNetwork->Ssid.Length; CharIndex++) {
                    Network->Ssid.StartOfString[CharIndex] = FoundNetwork->Ssid.Ssid[CharIndex];
                }
                Network->Ssid.LengthInChars = FoundNetwork->Ssid.Length;
                Network->Ssid.StartOfString[Network->Ssid.LengthInChars] = '\0';

                //
                //  If the same SSID already exists, keep the "highest
                //  priority" entry which means the one with a profile.
                //

                if (YuiWifiCheckForDuplicates(&YuiWifiContext.NetworkList, Network)) {
                    YuiWifiFreeNetwork(Network);
                } else {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Inserting network %y strength %i\n"), &Network->Ssid, Network->SignalStrength);
                    YuiWifiInsertListSorted(&YuiWifiContext.NetworkList, &Network->ListEntry);
                }
            }
        }
    }

    DllWlanApi.pWlanFreeMemory(NetworkList);

    //
    //  Update the list control with the list of known networks.
    //

    Index = 0;
    ListEntry = NULL;
    SendMessage(YuiWifiContext.hWndList, LB_RESETCONTENT, 0, 0);
    ConnectedIndex = (DWORD)-1;
    ListEntry = YoriLibGetNextListEntry(&YuiWifiContext.NetworkList, ListEntry);
    while (ListEntry != NULL) {
        Network = CONTAINING_RECORD(ListEntry, YUI_WIFI_NETWORK, ListEntry);
        if (ConnectedIndex == (DWORD)-1 &&
            Network->Connected) {

            ConnectedIndex = Index;
        }

        SendMessage(YuiWifiContext.hWndList, LB_ADDSTRING, 0, (LPARAM)Network);
        ListEntry = YoriLibGetNextListEntry(&YuiWifiContext.NetworkList, ListEntry);
        Index++;
    }

    //
    //  If a network is connected, ensure that list item is selected.  If
    //  none are connected, select the first/highest priority entry.
    //

    if (ConnectedIndex != (DWORD)-1) {
        YuiWifiContext.SelectedItemConnected = TRUE;
        SendMessage(YuiWifiContext.hWndList, LB_SETCURSEL, ConnectedIndex, 0);
        EnableWindow(YuiWifiContext.hWndButtonConnect, TRUE);
    } else if (Index > 0) {
        YuiWifiContext.SelectedItemConnected = FALSE;
        SendMessage(YuiWifiContext.hWndList, LB_SETCURSEL, 0, 0);
        EnableWindow(YuiWifiContext.hWndButtonConnect, TRUE);
    }

    return TRUE;
}

/**
 A notification callback for Wifi related events.  This program is only
 interested in when a network scan is complete so the results can be
 displayed.

 @param NotifyData Pointer to the notification information.

 @param Context Application defined context, in this case an event handle.
 */
VOID
YuiWifiNotifyCallback(
    __in PYORI_WLAN_NOTIFICATION_DATA NotifyData,
    __in PVOID Context
    )
{
    HWND hWnd;
    UNREFERENCED_PARAMETER(Context);

    if (NotifyData->NotificationSource == YORI_WLAN_NOTIFICATION_SOURCE_ACM &&
        (NotifyData->NotificationCode == YORI_WLAN_ACM_SCAN_COMPLETE ||
         NotifyData->NotificationCode == YORI_WLAN_ACM_SCAN_FAIL)) {

        hWnd = YuiWifiContext.hWnd;
        if (hWnd != NULL) {
            PostMessage(hWnd, WM_USER, 0, 0);
        }
    }
}


/**
 Start an asynchronous scan for Wifi networks.  Once the scan completes,
 queue a window message to populate the list.
 */
VOID
YuiWifiStartScan(VOID)
{
    DWORD Error;

    //
    //  Register for a notification when the scan is complete.
    //

    Error = DllWlanApi.pWlanRegisterNotification(YuiWifiContext.WlanHandle,
                                                 YORI_WLAN_NOTIFICATION_SOURCE_ACM,
                                                 FALSE,
                                                 YuiWifiNotifyCallback,
                                                 NULL,
                                                 NULL,
                                                 NULL);
    if (Error != ERROR_SUCCESS) {
        return;
    }

    //
    //  Initiate the scan.
    //

    Error = DllWlanApi.pWlanScan(YuiWifiContext.WlanHandle,
                                 &YuiWifiContext.Interface,
                                 NULL,
                                 NULL,
                                 NULL);
    if (Error != ERROR_SUCCESS) {
        return;
    }

    //
    //  Don't initiate any more scans.
    //

    YuiWifiContext.InitialScanStarted = TRUE;
}
                            
    
/**
 Paint the main Wifi window.  This means drawing a 3D border around it.

 @param hWnd Handle to the window to draw a border around.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YuiWifiPaint(
    __in HWND hWnd
    )
{
    RECT ClientRect;
    HDC hDC;
    PAINTSTRUCT PaintStruct;

    //
    //  If the window has no repainting to do, stop.
    //

    if (!GetUpdateRect(hWnd, &ClientRect, FALSE)) {
        return FALSE;
    }

    //
    //  If it does, redraw everything.
    //

    hDC = BeginPaint(hWnd, &PaintStruct);
    if (hDC == NULL) {
        return FALSE;
    }

    GetClientRect(hWnd, &ClientRect);
    YuiDrawWindowBackground(hDC, &ClientRect);
    YuiDrawThreeDBox(hDC, &ClientRect, YuiWifiContext.YuiMonitor->ControlBorderWidth, FALSE);

    //
    //  Find the location of the list within the parent window.
    //  (There has to be a better way to do this.  Right...Right?)
    //

    /*
    if (YuiWifiContext.hWndList != NULL) {
        RECT ParentWindowRect;
        RECT ChildWindowRect;
        GetWindowRect(hWnd, &ParentWindowRect);
        GetWindowRect(YuiWifiContext.hWndList, &ChildWindowRect);
        ClientRect.top = ChildWindowRect.top - ParentWindowRect.top - 1;
        ClientRect.left = ChildWindowRect.left - ParentWindowRect.left - 1;
        ClientRect.right = ChildWindowRect.right - ParentWindowRect.left + 1;
        ClientRect.bottom = ChildWindowRect.bottom - ParentWindowRect.top + 1;
        YuiDrawThreeDBox(hDC, &ClientRect, TRUE);
    }
    */

    EndPaint(hWnd, &PaintStruct);

    return TRUE;
}

/**
 Draw an item in the wifi list.  This function may draw an item as selected
 or not, and renders an appropriate icon based on signal strength.

 @param DrawItemStruct Pointer to the draw item structure.
 */
VOID
YuiWifiDrawListItem(
    __in PDRAWITEMSTRUCT DrawItemStruct
    )
{
    PYUI_WIFI_NETWORK Network;
    RECT TextRect;
    DWORD TextFlags;
    COLORREF ForeColor;
    COLORREF BackColor;
    HBRUSH Brush;
    WORD IconIndex;

    Network = (PYUI_WIFI_NETWORK)DrawItemStruct->itemData;

    if (DrawItemStruct->itemState & ODS_SELECTED) {
        ForeColor = YuiGetMenuSelectedTextColor();
        BackColor = YuiGetMenuSelectedBackgroundColor();
    } else {
        ForeColor = GetSysColor(COLOR_WINDOWTEXT);
        BackColor = GetSysColor(COLOR_WINDOW);
    }

    Brush = CreateSolidBrush(BackColor);
    FillRect(DrawItemStruct->hDC, &DrawItemStruct->rcItem, Brush);
    DeleteObject(Brush);

    if (Network != NULL) {
        WORD ItemHeight;
        WORD IconLeft;
        WORD IconTop;

        IconIndex = 0;
        if (Network->SignalStrength > 90) {
            IconIndex = 4;
        } else if (Network->SignalStrength > 80) {
            IconIndex = 3;
        } else if (Network->SignalStrength > 70) {
            IconIndex = 2;
        } else if (Network->SignalStrength > 50) {
            IconIndex = 1;
        }

        SetBkColor(DrawItemStruct->hDC, BackColor);
        SetTextColor(DrawItemStruct->hDC, ForeColor);

        ItemHeight = (WORD)(DrawItemStruct->rcItem.bottom - DrawItemStruct->rcItem.top);
        IconLeft = 3;
        IconTop = (WORD)((ItemHeight - YuiWifiContext.IconHeight) / 2 + DrawItemStruct->rcItem.top);

        DllUser32.pDrawIconEx(DrawItemStruct->hDC,
                              IconLeft,
                              IconTop,
                              YuiWifiContext.WifiIcon[IconIndex],
                              YuiWifiContext.IconWidth,
                              YuiWifiContext.IconHeight,
                              0, NULL, DI_NORMAL);

        if (Network->Connected) {
            DllUser32.pDrawIconEx(DrawItemStruct->hDC,
                                  IconLeft,
                                  IconTop,
                                  YuiWifiContext.WifiConnectedIcon,
                                  YuiWifiContext.IconWidth,
                                  YuiWifiContext.IconHeight,
                                  0, NULL, DI_NORMAL);
        }

        TextRect.left = DrawItemStruct->rcItem.left + 3 + YuiWifiContext.IconHeight + 3;
        TextRect.right = DrawItemStruct->rcItem.right - 3;
        TextRect.top = DrawItemStruct->rcItem.top + 1;
        TextRect.bottom = DrawItemStruct->rcItem.bottom - 1;
        TextFlags = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;

        DrawText(DrawItemStruct->hDC, Network->Ssid.StartOfString, Network->Ssid.LengthInChars, &TextRect, TextFlags);
    }
}

/**
 The main window procedure which processes messages sent to the Wifi
 window.

 @param hwnd The window handle.

 @param uMsg The message identifier.

 @param wParam The first parameter associated with the window message.

 @param lParam The second parameter associated with the window message.

 @return A value which depends on the type of message being processed.
 */
LRESULT CALLBACK
YuiWifiWindowProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    DWORD CtrlId;
    DWORD SelectedIndex;
    PYUI_WIFI_NETWORK Network;

    switch(uMsg) {
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                if (YuiWifiContext.DisableAutoClose == FALSE) {
                    YuiWifiClose();
                }
            } else if (LOWORD(wParam) == WA_ACTIVE &&
                       YuiWifiContext.InitialScanStarted == FALSE) {
                YuiWifiStartScan();
            }
            break;
        case WM_USER: // MSFIX
            DllWlanApi.pWlanRegisterNotification(YuiWifiContext.WlanHandle,
                                                 0,
                                                 FALSE,
                                                 YuiWifiNotifyCallback,
                                                 NULL,
                                                 NULL,
                                                 NULL);
            YuiWifiPopulateList();
            break;
        case WM_ACTIVATEAPP:
            if ((BOOL)wParam == FALSE) {
                if (YuiWifiContext.DisableAutoClose == FALSE) {
                    YuiWifiClose();
                }
            }
            break;
        case WM_PAINT:
            YuiWifiPaint(hwnd);
            break;
        case WM_MEASUREITEM:
            if (wParam == YUI_WIFI_LIST) {
                PMEASUREITEMSTRUCT MeasureItemStruct;
                MeasureItemStruct = (PMEASUREITEMSTRUCT)lParam;
                MeasureItemStruct->itemWidth = YuiWifiContext.IconWidth + 20;
                MeasureItemStruct->itemHeight = YuiWifiContext.IconHeight + 8;
                return TRUE;
            }
            break;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                if (LOWORD(wParam) == YUI_WIFI_CONNECT) {
                    DWORD Error;
                    SelectedIndex = (DWORD)SendMessage(YuiWifiContext.hWndList, LB_GETCURSEL, 0, 0);
                    if (SelectedIndex != LB_ERR) {
                        Network = (PYUI_WIFI_NETWORK)SendMessage(YuiWifiContext.hWndList, LB_GETITEMDATA, SelectedIndex, 0);
                        if (Network->Connected) {
                            Error = DllWlanApi.pWlanDisconnect(YuiWifiContext.WlanHandle, &YuiWifiContext.Interface, NULL);
                            if (Error == ERROR_SUCCESS) {
                                Network->Connected = FALSE;
                                YuiWifiContext.SelectedItemConnected = FALSE;
                                RedrawWindow(YuiWifiContext.hWndButtonConnect, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                            }
                        } else if (Network->ProfileName.LengthInChars == 0) {
                            YuiWifiContext.DisableAutoClose = TRUE;
                            MessageBox(hwnd, _T("This program does not support connecting to unknown networks yet."), _T("Error"), MB_ICONSTOP);
                            YuiWifiContext.DisableAutoClose = FALSE;
                        } else {
                            YORI_WLAN_CONNECTION_PARAMETERS Parameters;
                            Parameters.ConnectionMode = 0;
                            Parameters.ProfileName = Network->ProfileName.StartOfString;
                            Parameters.Ssid = NULL;
                            Parameters.DesiredBssidList = NULL;
                            Parameters.BssType = 1;
                            Parameters.Flags = 0;

                            Error = DllWlanApi.pWlanConnect(YuiWifiContext.WlanHandle, &YuiWifiContext.Interface, &Parameters, NULL);
                            if (Error == ERROR_SUCCESS) {
                                // MSFIX Need to disconnect any other connected network
                                Network->Connected = TRUE;
                                YuiWifiContext.SelectedItemConnected = TRUE;
                                RedrawWindow(YuiWifiContext.hWndButtonConnect, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                            }
                        }
                    }
                } else if (LOWORD(wParam) == YUI_WIFI_AIRPLANEMODE) {
                    BOOLEAN NewAirplaneMode;
                    if (YuiWifiContext.AirplaneModeEnabled) {
                        NewAirplaneMode = FALSE;
                    } else {
                        NewAirplaneMode = TRUE;
                    }

                    if (YoriLibSetAirplaneMode(NewAirplaneMode)) {
                        YuiWifiContext.AirplaneModeEnabled = NewAirplaneMode;
                        SendMessage(YuiWifiContext.hWndButtonAirplane, BM_SETSTATE, NewAirplaneMode, 0);
                    }
                }
            } else if (HIWORD(wParam) == LBN_SELCHANGE) {
                SelectedIndex = (DWORD)SendMessage(YuiWifiContext.hWndList, LB_GETCURSEL, 0, 0);
                if (SelectedIndex == (DWORD)-1) {
                    EnableWindow(YuiWifiContext.hWndButtonConnect, FALSE);
                } else {
                    Network = (PYUI_WIFI_NETWORK)SendMessage(YuiWifiContext.hWndList, LB_GETITEMDATA, SelectedIndex, 0);
                    EnableWindow(YuiWifiContext.hWndButtonConnect, TRUE);
                    if (Network->Connected) {
                        if (YuiWifiContext.SelectedItemConnected == FALSE) {
                            YuiWifiContext.SelectedItemConnected = TRUE;
                            RedrawWindow(YuiWifiContext.hWndButtonConnect, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                        }
                    } else {
                        if (YuiWifiContext.SelectedItemConnected) {
                            YuiWifiContext.SelectedItemConnected = FALSE;
                            RedrawWindow(YuiWifiContext.hWndButtonConnect, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                        }
                    }
                }
            }
            break;
        case WM_DRAWITEM:
            {
                PDRAWITEMSTRUCT DrawItemStruct;
                YORI_STRING Str;
                DrawItemStruct = (PDRAWITEMSTRUCT)lParam;
                CtrlId = LOWORD(wParam);
                if (DrawItemStruct->CtlType == ODT_BUTTON) {
                    DWORD ButtonState;
                    BOOLEAN Pushed;
                    BOOLEAN Disabled;

                    YoriLibConstantString(&Str, _T(""));
                    if (CtrlId == YUI_WIFI_CONNECT) {
                        if (YuiWifiContext.SelectedItemConnected) {
                            YoriLibConstantString(&Str, _T("Disconnect"));
                        } else {
                            YoriLibConstantString(&Str, _T("Connect"));
                        }
                    } else if (CtrlId == YUI_WIFI_AIRPLANEMODE) {
                        YoriLibConstantString(&Str, _T("Airplane Mode"));
                    }
                    ButtonState = (DWORD)SendMessage(DrawItemStruct->hwndItem, BM_GETSTATE, 0, 0);
                    Pushed = FALSE;
                    Disabled = FALSE;
                    if (ButtonState & BST_PUSHED) {
                        Pushed = TRUE;
                    }
                    if (DrawItemStruct->itemState & ODS_DISABLED) {
                        Disabled = TRUE;
                    }
                    YuiDrawButton(YuiWifiContext.YuiMonitor,
                                  DrawItemStruct,
                                  Pushed,
                                  FALSE,
                                  Disabled,
                                  NULL,
                                  &Str,
                                  TRUE);
                } else if (DrawItemStruct->CtlType == ODT_LISTBOX) {
                    YuiWifiDrawListItem(DrawItemStruct);
                }
                
            }
            break;


    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 Display the Yui wifi controls.

 @param YuiMonitor Pointer to the monitor context.
 */
VOID
YuiWifi(
    __in PYUI_MONITOR YuiMonitor
    )
{
    HWND hWnd;
    HWND hWndList;
    HWND hWndButtonAirplane;
    HWND hWndButtonConnect;
    WORD WindowWidth;
    WORD WindowHeight;
    WORD WindowPaddingHoriz;
    WORD WindowPaddingVert;
    WORD ButtonPadding;
    WORD ButtonHeight;
    WORD ButtonAreaHeight;
    WORD ListWidth;
    WORD ListHeight;
    RECT ClientRect;
    WORD Index;
    DWORD Error;
    DWORD NegotiatedVersion;
    PYORI_WLAN_INTERFACE_INFO_LIST InterfaceList;
    BOOLEAN AirplaneModeEnabled;
    BOOLEAN AirplaneModeChangable;

    YuiWifiContext.YuiMonitor = YuiMonitor;
    
    //
    //  If the window is already open for some reason, don't open it again.
    //

    if (YuiWifiContext.hWnd != NULL) {
        return;
    }

    //
    //  Load the system's Wifi functions.  If the code isn't there, this
    //  module can't be used.
    //

    YoriLibLoadWlanApiFunctions();
    if (DllWlanApi.pWlanCloseHandle == NULL ||
        DllWlanApi.pWlanConnect == NULL ||
        DllWlanApi.pWlanDisconnect == NULL ||
        DllWlanApi.pWlanEnumInterfaces == NULL ||
        DllWlanApi.pWlanFreeMemory == NULL ||
        DllWlanApi.pWlanGetAvailableNetworkList == NULL ||
        DllWlanApi.pWlanOpenHandle == NULL ||
        DllWlanApi.pWlanRegisterNotification == NULL ||
        DllWlanApi.pWlanScan == NULL) {

        return;
    }

    //
    //  Open a handle to the system's Wifi support, and enumerate Wifi
    //  adapters.  If we can't open a handle or find an adapter, this code
    //  can't be used.  If we find multiple adapters, this code will only
    //  attempt to work with the first discovered adapter.
    //

    Error = DllWlanApi.pWlanOpenHandle(1, NULL, &NegotiatedVersion, &YuiWifiContext.WlanHandle);
    if (Error != ERROR_SUCCESS) {
        YuiWifiContext.WlanHandle = NULL;
        return;
    }

    YoriLibInitializeListHead(&YuiWifiContext.NetworkList);

    Error = DllWlanApi.pWlanEnumInterfaces(YuiWifiContext.WlanHandle, NULL, &InterfaceList);
    if (Error != ERROR_SUCCESS ||
        InterfaceList == NULL ||
        InterfaceList->NumberOfItems == 0) {

        YuiWifiClose();
        return;
    }

    memcpy(&YuiWifiContext.Interface, &InterfaceList->InterfaceInfo[0].InterfaceGuid, sizeof(GUID));
    DllWlanApi.pWlanFreeMemory(InterfaceList);

    //
    //  These values correspond to the size of the icons in the resource.
    //  It'd be possible to record multiple icon sizes and be dynamic here,
    //  but note these sizes have nothing to do with User32 icon sizes.
    //

    YuiWifiContext.IconWidth = 32;
    YuiWifiContext.IconHeight = 32;

    for (Index = 0; Index < 5; Index++) {
        YuiWifiContext.WifiIcon[Index] = DllUser32.pLoadImageW(GetModuleHandle(NULL),
                MAKEINTRESOURCE(WIFI1ICON + Index),
                IMAGE_ICON,
                YuiWifiContext.IconWidth,
                YuiWifiContext.IconHeight,
                0);
        if (YuiWifiContext.WifiIcon[Index] == NULL) {
            YuiWifiClose();
            return;
        }
    }

    YuiWifiContext.WifiConnectedIcon = DllUser32.pLoadImageW(GetModuleHandle(NULL),
            MAKEINTRESOURCE(WIFICONNICON),
            IMAGE_ICON,
            YuiWifiContext.IconWidth,
            YuiWifiContext.IconHeight,
            0);
    if (YuiWifiContext.WifiConnectedIcon == NULL) {
        YuiWifiClose();
        return;
    }

    WindowWidth = 250;

    //
    //  Give an extra 30px for every increase in font size
    //

    if (YuiMonitor->FontSize > YUI_BASE_FONT_SIZE) {
        WindowWidth = (WORD)(WindowWidth + 30 * (YuiMonitor->FontSize - YUI_BASE_FONT_SIZE));
    }

    //
    //  Give an extra 5% of any pixels above 800
    //

    if (YuiMonitor->ScreenWidth > 800) {
        WindowWidth = (WORD)(WindowWidth + (YuiMonitor->ScreenWidth - 800) / 20);
    }
    WindowHeight = (WORD)(YuiMonitor->ScreenHeight - YuiMonitor->TaskbarHeight);
    WindowPaddingHoriz = 12;
    WindowPaddingVert = 12;

    //
    //  Create the main Wifi window.
    //

    hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                          YUI_WIFI_CLASS,
                          _T(""),
                          WS_POPUP | WS_CLIPCHILDREN,
                          YuiMonitor->ScreenLeft + YuiMonitor->ScreenWidth - WindowWidth,
                          YuiMonitor->ScreenTop + YuiMonitor->ScreenHeight - YuiMonitor->TaskbarHeight - WindowHeight,
                          WindowWidth,
                          WindowHeight,
                          NULL, NULL, NULL, 0);
    if (hWnd == NULL) {
        YuiWifiClose();
        return;
    }

    YuiWifiContext.hWnd = hWnd;

    //
    //  Check if the system supports Airplane mode and what the current
    //  setting is.  Create an airplane mode button unconditionally, but
    //  only enable it if the system supports airplane mode.  If airplane
    //  mode is enabled, set the button to have a pressed appearance.
    //

    AirplaneModeEnabled = FALSE;
    AirplaneModeChangable = FALSE;
    YoriLibGetAirplaneMode(&AirplaneModeEnabled, &AirplaneModeChangable);

    GetClientRect(hWnd, &ClientRect);

    //
    //  MSFIX ButtonHeight wants to be a function of font or taskbar size.
    //

    ListWidth = (WORD)(ClientRect.right - ClientRect.left - 2 * WindowPaddingHoriz);
    ButtonHeight = (WORD)(YuiMonitor->TaskbarHeight - 2 * YuiMonitor->TaskbarPaddingVertical);
    ButtonPadding = (WORD)(WindowPaddingVert - 2);
    ButtonAreaHeight = (WORD)(WindowPaddingVert + ButtonHeight);

    hWndButtonAirplane = CreateWindowEx(0,
                                        _T("BUTTON"),
                                        _T(""),
                                        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                        WindowPaddingHoriz,
                                        ClientRect.bottom - ClientRect.top - ButtonAreaHeight,
                                        ListWidth,
                                        ButtonHeight,
                                        hWnd,
                                        (HMENU)(DWORD_PTR)YUI_WIFI_AIRPLANEMODE,
                                        NULL, 0);
    if (hWndButtonAirplane == NULL) {
        YuiWifiClose();
        return;
    }

    SendMessage(hWndButtonAirplane, WM_SETFONT, (WPARAM)YuiMonitor->hFont, MAKELPARAM(TRUE, 0));
    if (AirplaneModeEnabled) {
        SendMessage(hWndButtonAirplane, BM_SETSTATE, TRUE, 0);
    }
    if (!AirplaneModeChangable) {
        EnableWindow(hWndButtonAirplane, FALSE);
    }

    YuiWifiContext.AirplaneModeEnabled = AirplaneModeEnabled;

    //
    //  Create a button to connect to or disconnect from wireless networks.
    //


    ButtonAreaHeight = (WORD)(ButtonAreaHeight + ButtonPadding + ButtonHeight);

    hWndButtonConnect = CreateWindowEx(0,
                                       _T("BUTTON"),
                                       _T(""),
                                       BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                       WindowPaddingHoriz,
                                       ClientRect.bottom - ClientRect.top - ButtonAreaHeight,
                                       ListWidth,
                                       ButtonHeight,
                                       hWnd,
                                       (HMENU)(DWORD_PTR)YUI_WIFI_CONNECT,
                                       NULL, 0);
    if (hWndButtonConnect == NULL) {
        YuiWifiClose();
        return;
    }
    EnableWindow(hWndButtonConnect, FALSE);

    SendMessage(hWndButtonConnect, WM_SETFONT, (WPARAM)YuiMonitor->hFont, MAKELPARAM(TRUE, 0));

    ButtonAreaHeight = (WORD)(ButtonAreaHeight + ButtonPadding);

    //
    //  Create a list of wireless networks with any screen area above the
    //  buttons.
    //

    ListWidth = (WORD)(ClientRect.right - ClientRect.left - 2 * WindowPaddingHoriz);
    ListHeight = (WORD)(ClientRect.bottom - ClientRect.top - WindowPaddingVert - ButtonAreaHeight);

    hWndList = CreateWindowEx(WS_EX_CLIENTEDGE,
                              _T("LISTBOX"),
                              _T(""),
                              WS_CHILD | WS_VSCROLL | WS_VISIBLE | LBS_OWNERDRAWFIXED | LBS_NOTIFY,
                              WindowPaddingHoriz,
                              WindowPaddingVert,
                              ListWidth,
                              ListHeight,
                              hWnd,
                              (HMENU)(DWORD_PTR)YUI_WIFI_LIST,
                              NULL, 0);
    if (hWndList == NULL) {
        YuiWifiClose();
        return;
    }

    SendMessage(hWndList, WM_SETFONT, (WPARAM)YuiMonitor->hFont, MAKELPARAM(TRUE, 0));

    ShowWindow(hWnd, SW_SHOW);
    DllUser32.pSetForegroundWindow(hWnd);
    SetFocus(hWndList);

    YuiWifiContext.hWndList = hWndList;
    YuiWifiContext.hWndButtonAirplane = hWndButtonAirplane;
    YuiWifiContext.hWndButtonConnect = hWndButtonConnect;
}


// vim:sw=4:ts=4:et:
