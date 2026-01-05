/**
 * @file moninfo/moninfo.c
 *
 * Yori display and monitor information
 *
 * Copyright (c) 2026 Malcolm J. Smith
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
CHAR strMonInfoHelpText[] =
        "\n"
        "Return information about displays and monitors.\n"
        "\n"
        "MONINFO [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
MonInfoHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("MonInfo %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strMonInfoHelpText);
    return TRUE;
}

/**
 Context structure passed between the top level application and monitor
 enumeration callback.
 */
typedef struct _MONINFO_CONTEXT {

    /**
     The number of monitors that have been enumerated.
     */
    DWORD MonitorCount;
} MONINFO_CONTEXT, *PMONINFO_CONTEXT;

/**
 A callback invoked when enumerating monitors.

 @param hMonitor Handle to the monitor.

 @param hDC Device context, ignored in this program.

 @param lprcMonitor Pointer to the region of the virtual screen occupied by
        this monitor.

 @param dwData Context information; in this program, a pointer to the global
        application context.

 @return TRUE to continue enumerating, FALSE to terminate.
 */
BOOL WINAPI
MonInfoMonitorCallback(
    __in HANDLE hMonitor,
    __in HDC hDC,
    __in LPRECT lprcMonitor,
    __in LPARAM dwData
    )
{
    PMONINFO_CONTEXT Context;
    YORI_MONITORINFOEX MonitorInfo;
    YORI_DISPLAY_DEVICE DeviceInfo;
    BOOLEAN IsPrimary;
    YORI_STRING Left;
    YORI_STRING Top;
    YORI_STRING Right;
    YORI_STRING Bottom;
    YORI_STRING Flags;
    TCHAR LeftBuffer[10];
    TCHAR TopBuffer[10];
    TCHAR RightBuffer[10];
    TCHAR BottomBuffer[10];

    UNREFERENCED_PARAMETER(hDC);

    Context = (PMONINFO_CONTEXT)dwData;

    if (DllUser32.pGetMonitorInfoW == NULL) {
        return FALSE;
    }

    ZeroMemory(&MonitorInfo, sizeof(MonitorInfo));
    MonitorInfo.cbSize = sizeof(MonitorInfo);
    if (!DllUser32.pGetMonitorInfoW(hMonitor, (PYORI_MONITORINFO)&MonitorInfo)) {
        return FALSE;
    }

    IsPrimary = FALSE;
    if (MonitorInfo.dwFlags & MONITORINFOF_PRIMARY) {
        IsPrimary = TRUE;
    }

    YoriLibInitEmptyString(&Left);
    Left.StartOfString = LeftBuffer;
    Left.LengthAllocated = sizeof(LeftBuffer)/sizeof(LeftBuffer[0]);
    YoriLibInitEmptyString(&Top);
    Top.StartOfString = TopBuffer;
    Top.LengthAllocated = sizeof(TopBuffer)/sizeof(TopBuffer[0]);
    YoriLibInitEmptyString(&Right);
    Right.StartOfString = RightBuffer;
    Right.LengthAllocated = sizeof(RightBuffer)/sizeof(RightBuffer[0]);
    YoriLibInitEmptyString(&Bottom);
    Bottom.StartOfString = BottomBuffer;
    Bottom.LengthAllocated = sizeof(BottomBuffer)/sizeof(BottomBuffer[0]);

    YoriLibNumberToString(&Left, lprcMonitor->left, 10, 0, '\0');
    YoriLibNumberToString(&Top, lprcMonitor->top, 10, 0, '\0');
    YoriLibNumberToString(&Right, lprcMonitor->right, 10, 0, '\0');
    YoriLibNumberToString(&Bottom, lprcMonitor->bottom, 10, 0, '\0');


    if (Context->MonitorCount > 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("[%02i] %s%y,%y-%y,%y %s\n"), Context->MonitorCount, IsPrimary?_T("(Primary) "):_T(""), &Left, &Top, &Right, &Bottom, MonitorInfo.szDevice);
    ZeroMemory(&DeviceInfo, sizeof(DeviceInfo));
    DeviceInfo.cbSize = sizeof(DeviceInfo);
    if (DllUser32.pEnumDisplayDevicesW(MonitorInfo.szDevice, 0, &DeviceInfo, 0)) {
        YoriLibInitEmptyString(&Flags);
        YoriLibYPrintf(&Flags,
                       _T("%s%s%s%s%s"),
                       (DeviceInfo.dwStateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)?_T(" AttachedToDesktop"):_T(""),
                       (DeviceInfo.dwStateFlags & DISPLAY_DEVICE_MULTI_DRIVER)?_T(" MultiDriver"):_T(""),
                       (DeviceInfo.dwStateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)?_T(" PrimaryDevice"):_T(""),
                       (DeviceInfo.dwStateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)?_T(" MirroringDriver"):_T(""),
                       (DeviceInfo.dwStateFlags & DISPLAY_DEVICE_VGA_COMPATIBLE)?_T(" VgaCompatible"):_T(""));

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT,
                _T("  Name[%02i]: %s\n")
                _T("  Id[%02i]: %s\n")
                _T("  Registry[%02i]: %s\n")
                _T("  Flags[%02i]:%y\n"),
                Context->MonitorCount, DeviceInfo.szDeviceString,
                Context->MonitorCount, DeviceInfo.szDeviceId,
                Context->MonitorCount, DeviceInfo.szDeviceKey,
                Context->MonitorCount, &Flags);

        YoriLibFreeStringContents(&Flags);
    }
    Context->MonitorCount++;
    return TRUE;
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the moninfo builtin command.
 */
#define ENTRYPOINT YoriCmd_MONINFO
#else
/**
 The main entrypoint for the moninfo standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the moninfo cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_ALLOC_SIZE_T i;
    YORI_STRING Arg;
    MONINFO_CONTEXT Context;

    Context.MonitorCount = 0;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                MonInfoHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2026"));
                return EXIT_SUCCESS;
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

    YoriLibLoadUser32Functions();

    if (DllUser32.pEnumDisplayMonitors == NULL ||
        DllUser32.pEnumDisplayDevicesW == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("moninfo: OS support not present\n"));
        return EXIT_FAILURE;
    }

    DllUser32.pEnumDisplayMonitors(NULL, NULL, MonInfoMonitorCallback, (LPARAM)&Context);
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
