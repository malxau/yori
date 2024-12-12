/**
 * @file wifi/wifi.c
 *
 * Yori shell display and join wireless networks
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

/**
 Help text to display to the user.
 */
const
CHAR strWifiHelpText[] =
        "\n"
        "Display or join wireless networks.\n"
        "\n"
        "WIFI [-license] [-op=list]\n";

/**
 Display usage text to the user.
 */
BOOL
WifiHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Wifi %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strWifiHelpText);
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
WifiNotifyCallback(
    __in PYORI_WLAN_NOTIFICATION_DATA NotifyData,
    __in PVOID Context
    )
{
    if (NotifyData->NotificationSource == YORI_WLAN_NOTIFICATION_SOURCE_ACM &&
        (NotifyData->NotificationCode == YORI_WLAN_ACM_SCAN_COMPLETE ||
         NotifyData->NotificationCode == YORI_WLAN_ACM_SCAN_FAIL)) {

        HANDLE ScanCompleteEvent;
        ScanCompleteEvent = (HANDLE)Context;
        SetEvent(ScanCompleteEvent);
    }
}

/**
 List available networks and display the result to stdout.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
WifiListNetworks(
    __in BOOLEAN KnownOnly
    )
{
    DWORD NegotiatedVersion;
    PYORI_WLAN_INTERFACE_INFO_LIST InterfaceList;
    PYORI_WLAN_AVAILABLE_NETWORK_LIST NetworkList;
    PYORI_WLAN_AVAILABLE_NETWORK Network;
    HANDLE WlanHandle;
    YORI_STRING Ssid;
    TCHAR SsidBuffer[YORI_DOT11_SSID_MAX_LENGTH];
    DWORD CharIndex;
    HANDLE ScanCompleteEvent;
    GUID * InterfaceGuid;
    DWORD Error;
    DWORD Index;

    YoriLibInitEmptyString(&Ssid);
    Ssid.StartOfString = SsidBuffer;
    Ssid.LengthAllocated = sizeof(SsidBuffer)/sizeof(SsidBuffer[0]);

    Error = DllWlanApi.pWlanOpenHandle(1, NULL, &NegotiatedVersion, &WlanHandle);
    if (Error != ERROR_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: failed to open handle\n"));
        return FALSE;
    }

    Error = DllWlanApi.pWlanEnumInterfaces(WlanHandle, NULL, &InterfaceList);
    if (Error != ERROR_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: failed to enumerate network adapters\n"));
        DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
        return FALSE;
    }

    if (InterfaceList->NumberOfItems == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: Wireless adapter not found\n"));
        DllWlanApi.pWlanFreeMemory(InterfaceList);
        DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
        return FALSE;
    }

    InterfaceGuid = &InterfaceList->InterfaceInfo[0].InterfaceGuid;

    ScanCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (ScanCompleteEvent == NULL) {
        DllWlanApi.pWlanFreeMemory(InterfaceList);
        DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
        return FALSE;
    }

    DllWlanApi.pWlanRegisterNotification(WlanHandle, YORI_WLAN_NOTIFICATION_SOURCE_ACM, FALSE, WifiNotifyCallback, ScanCompleteEvent, NULL, NULL);
    DllWlanApi.pWlanScan(WlanHandle, InterfaceGuid, NULL, NULL, NULL);

    //
    //  Wait for 10 seconds for the event to be signalled.  Note if the
    //  event registration or scan above failed, this will time out, and
    //  we'll return whatever exists.
    //

    WaitForSingleObject(ScanCompleteEvent, 10 * 1000);
    DllWlanApi.pWlanRegisterNotification(WlanHandle, 0, FALSE, WifiNotifyCallback, NULL, NULL, NULL);
    CloseHandle(ScanCompleteEvent);

    Error = DllWlanApi.pWlanGetAvailableNetworkList(WlanHandle, InterfaceGuid, 0, NULL, &NetworkList);
    if (Error != ERROR_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: Could not list available networks\n"));
        DllWlanApi.pWlanFreeMemory(InterfaceList);
        DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
        return FALSE;
    }
    for (Index = 0; Index < NetworkList->NumberOfItems; Index++) {
        Network = &NetworkList->Network[Index];
        if (KnownOnly) {
            if (Network->Flags & YORI_WLAN_AVAILABLE_NETWORK_HAS_PROFILE) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-33s %i\n"), Network->ProfileName, Network->SignalQuality);
            }
        } else {
            for (CharIndex = 0; CharIndex < Network->Ssid.Length; CharIndex++) {
                SsidBuffer[CharIndex] = Network->Ssid.Ssid[CharIndex];
            }
            Ssid.LengthInChars = CharIndex;
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%-33y %i\n"), &Ssid, Network->SignalQuality);
        }
    }
    DllWlanApi.pWlanFreeMemory(NetworkList);
    DllWlanApi.pWlanFreeMemory(InterfaceList);

    DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
    return TRUE;
}

/**
 Disconnect from any currently connected network.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
WifiDisconnect(VOID)
{
    DWORD NegotiatedVersion;
    PYORI_WLAN_INTERFACE_INFO_LIST InterfaceList;
    HANDLE WlanHandle;
    GUID * InterfaceGuid;
    DWORD Error;

    Error = DllWlanApi.pWlanOpenHandle(1, NULL, &NegotiatedVersion, &WlanHandle);
    if (Error != ERROR_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: failed to open handle\n"));
        return FALSE;
    }

    Error = DllWlanApi.pWlanEnumInterfaces(WlanHandle, NULL, &InterfaceList);
    if (Error != ERROR_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: failed to enumerate network adapters\n"));
        DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
        return FALSE;
    }

    if (InterfaceList->NumberOfItems == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: Wireless adapter not found\n"));
        DllWlanApi.pWlanFreeMemory(InterfaceList);
        DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
        return FALSE;
    }

    InterfaceGuid = &InterfaceList->InterfaceInfo[0].InterfaceGuid;

    Error = DllWlanApi.pWlanDisconnect(WlanHandle, InterfaceGuid, NULL);
    if (Error != ERROR_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: failed to disconnect\n"));
        DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
        return FALSE;
    }

    DllWlanApi.pWlanFreeMemory(InterfaceList);
    DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
    return TRUE;
}

/**
 Connect to a known network, identified by profile name.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
WifiConnect(
    __in PYORI_STRING ProfileName
    )
{
    DWORD NegotiatedVersion;
    PYORI_WLAN_INTERFACE_INFO_LIST InterfaceList;
    YORI_WLAN_CONNECTION_PARAMETERS Parameters;
    HANDLE WlanHandle;
    GUID * InterfaceGuid;
    DWORD Error;

    Error = DllWlanApi.pWlanOpenHandle(1, NULL, &NegotiatedVersion, &WlanHandle);
    if (Error != ERROR_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: failed to open handle\n"));
        return FALSE;
    }

    Error = DllWlanApi.pWlanEnumInterfaces(WlanHandle, NULL, &InterfaceList);
    if (Error != ERROR_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: failed to enumerate network adapters\n"));
        DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
        return FALSE;
    }

    if (InterfaceList->NumberOfItems == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: Wireless adapter not found\n"));
        DllWlanApi.pWlanFreeMemory(InterfaceList);
        DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
        return FALSE;
    }

    InterfaceGuid = &InterfaceList->InterfaceInfo[0].InterfaceGuid;

    Parameters.ConnectionMode = 0;
    Parameters.ProfileName = ProfileName->StartOfString;
    Parameters.Ssid = NULL;
    Parameters.DesiredBssidList = NULL;
    Parameters.BssType = 1;
    Parameters.Flags = 0;

    Error = DllWlanApi.pWlanConnect(WlanHandle, InterfaceGuid, &Parameters, NULL);
    if (Error != ERROR_SUCCESS) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: failed to connect\n"));
        DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
        return FALSE;
    }

    DllWlanApi.pWlanFreeMemory(InterfaceList);
    DllWlanApi.pWlanCloseHandle(WlanHandle, NULL);
    return TRUE;
}


/**
 The set of operations supported by this program.
 */
typedef enum _YWIFI_OP {
    WifiOpInvalid = 0,
    WifiOpListNetworks = 1,
    WifiOpListKnownNetworks = 2,
    WifiOpDisconnect = 3,
    WifiOpConnect = 4
} YWIFI_OP;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the wifi builtin command.
 */
#define ENTRYPOINT YoriCmd_YWIFI
#else
/**
 The main entrypoint for the wifi standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the wifi cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero indicating success or nonzero on
         failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    PYORI_STRING ProfileName;
    YWIFI_OP Op;

    Op = WifiOpInvalid;
    ProfileName = NULL;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                WifiHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2023"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("op=connect")) == 0) {
                if (i + 1 < ArgC) {
                    Op = WifiOpConnect;
                    ProfileName = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringLitIns(&Arg, _T("op=disconnect")) == 0) {
                Op = WifiOpDisconnect;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("op=list")) == 0) {
                Op = WifiOpListNetworks;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("op=listknown")) == 0) {
                Op = WifiOpListKnownNetworks;
                ArgumentUnderstood = TRUE;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (Op == WifiOpInvalid) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: operation not specified\n"));
        return EXIT_FAILURE;
    }

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

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("wifi: OS support not present\n"));
        return EXIT_FAILURE;
    }

    if (Op == WifiOpListNetworks) {
        if (!WifiListNetworks(FALSE)) {
            return EXIT_FAILURE;
        }
    } else if (Op == WifiOpListKnownNetworks) {
        if (!WifiListNetworks(TRUE)) {
            return EXIT_FAILURE;
        }
    } else if (Op == WifiOpDisconnect) {
        if (!WifiDisconnect()) {
            return EXIT_FAILURE;
        }
    } else if (Op == WifiOpConnect) {
        if (!WifiConnect(ProfileName)) {
            return EXIT_FAILURE;
        }
    }


    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
