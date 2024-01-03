/**
 * @file ysetup/gui.c
 *
 * Yori shell GUI installer
 *
 * Copyright (c) 2018-2023 Malcolm J. Smith
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
#include <yoripkg.h>
#include "resource.h"
#include "ysetup.h"

/**
 A prototype for the CreateFontW function.
 */
typedef
HFONT WINAPI
CREATE_FONTW(INT, INT, INT, INT, INT, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCWSTR);

/**
 A prototype for a pointer to the CreateFontW function.
 */
typedef CREATE_FONTW *PCREATE_FONTW;

/**
 A prototype for the DialogBoxParamW function.
 */
typedef
LONG_PTR WINAPI
DIALOG_BOX_PARAMW(HINSTANCE, LPCWSTR, HWND, DLGPROC, LPARAM);

/**
 A prototype for a pointer to the DialogBoxParamW function.
 */
typedef DIALOG_BOX_PARAMW *PDIALOG_BOX_PARAMW;

/**
 A prototype for the EnableWindow function.
 */
typedef
BOOL WINAPI
ENABLE_WINDOW(HWND, BOOL);

/**
 A prototype for a pointer to the EnableWindow function.
 */
typedef ENABLE_WINDOW *PENABLE_WINDOW;

/**
 A prototype for the EndDialog function.
 */
typedef
BOOL WINAPI
END_DIALOG(HWND, LONG_PTR);

/**
 A prototype for a pointer to the EndDialog function.
 */
typedef END_DIALOG *PEND_DIALOG;

/**
 A prototype for the GetDeviceCaps function.
 */
typedef
int WINAPI
GET_DEVICE_CAPS(HDC, INT);

/**
 A prototype for a pointer to the GetDeviceCaps function.
 */
typedef GET_DEVICE_CAPS *PGET_DEVICE_CAPS;

/**
 A prototype for the GetDlgItem function.
 */
typedef
HWND WINAPI
GET_DLG_ITEM(HWND, INT);

/**
 A prototype for a pointer to the GetDlgItem function.
 */
typedef GET_DLG_ITEM *PGET_DLG_ITEM;

/**
 A prototype for the GetWindowDC function.
 */
typedef
HDC WINAPI
GET_WINDOW_DC(HWND);

/**
 A prototype for a pointer to the GetWindowDC function.
 */
typedef GET_WINDOW_DC *PGET_WINDOW_DC;

/**
 A prototype for the LoadIconW function.
 */
typedef
HICON WINAPI
LOAD_ICONW(HINSTANCE, LPCWSTR);

/**
 A prototype for a pointer to the LoadIconW function.
 */
typedef LOAD_ICONW *PLOAD_ICONW;

/**
 A prototype for the MessageBoxW function.
 */
typedef
INT WINAPI
MESSAGE_BOXW(HWND, LPCWSTR, LPCWSTR, UINT);

/**
 A prototype for a pointer to the MessageBoxW function.
 */
typedef MESSAGE_BOXW *PMESSAGE_BOXW;

/**
 A prototype for the ReleaseDC function.
 */
typedef
INT WINAPI
RELEASE_DC(HWND, HDC);

/**
 A prototype for a pointer to the ReleaseDC function.
 */
typedef RELEASE_DC *PRELEASE_DC;

/**
 A prototype for the SendMessageW function.
 */
typedef
LRESULT WINAPI
SEND_MESSAGEW(HWND, DWORD, WPARAM, LPARAM);

/**
 A prototype for a pointer to the SendMessageW function.
 */
typedef SEND_MESSAGEW *PSEND_MESSAGEW;

/**
 A prototype for the SystemParametersInfoW function.
 */
typedef
BOOL WINAPI
SYSTEM_PARAMETERS_INFOW(DWORD, DWORD, PVOID, DWORD);

/**
 A prototype for a pointer to the SystemParametersInfoW function.
 */
typedef SYSTEM_PARAMETERS_INFOW *PSYSTEM_PARAMETERS_INFOW;

/**
 A structure containing all of the dynamically loaded functions from Gdi32
 and User32 used by this program.
 */
typedef struct _YSETUP_GUI_DLL {

    /**
     A handle to gdi32.dll if it can be loaded.
     */
    HMODULE hGdi32;

    /**
     A handle to user32.dll if it can be loaded.
     */
    HMODULE hUser32;

    /**
     If it exists on the system, a pointer to CreateFontW.
     */
    PCREATE_FONTW pCreateFontW;

    /**
     If it exists on the system, a pointer to GetDeviceCaps.
     */
    PGET_DEVICE_CAPS pGetDeviceCaps;

    /**
     If it exists on the system, a pointer to DialogBoxParamW.
     */
    PDIALOG_BOX_PARAMW pDialogBoxParamW;

    /**
     If it exists on the system, a pointer to EnableWindow.
     */
    PENABLE_WINDOW pEnableWindow;

    /**
     If it exists on the system, a pointer to EndDialog.
     */
    PEND_DIALOG pEndDialog;

    /**
     If it exists on the system, a pointer to GetDlgItem.
     */
    PGET_DLG_ITEM pGetDlgItem;

    /**
     If it exists on the system, a pointer to GetWindowDC.
     */
    PGET_WINDOW_DC pGetWindowDC;

    /**
     If it exists on the system, a pointer to LoadIconW.
     */
    PLOAD_ICONW pLoadIconW;

    /**
     If it exists on the system, a pointer to MessageBoxW.
     */
    PMESSAGE_BOXW pMessageBoxW;

    /**
     If it exists on the system, a pointer to ReleaseDC.
     */
    PRELEASE_DC pReleaseDC;

    /**
     If it exists on the system, a pointer to SendMessageW.
     */
    PSEND_MESSAGEW pSendMessageW;

    /**
     If it exists on the system, a pointer to SystemParametersInfoW.
     */
    PSYSTEM_PARAMETERS_INFOW pSystemParametersInfoW;
} YSETUP_GUI_DLL, *PYSETUP_GUI_DLL;

/**
 Pointers to User32 and Gdi32 functions which are only needed if Ysetup
 is invoked in GUI mode.
 */
YSETUP_GUI_DLL DllYsetupGui;

/**
 Attempt to load Gdi32 and User32, returning TRUE if all required functions
 for GUI support are available.

 @return TRUE if GUI support is available, FALSE if it is not.
 */
BOOLEAN
SetupGuiInitialize(VOID)
{
    DWORD MajorVersion;
    DWORD MinorVersion;
    DWORD BuildNumber;


    //
    //  This function should only ever be called once and doesn't try to
    //  handle repeated calls.
    //

    ASSERT(DllYsetupGui.hGdi32 == NULL);
    ASSERT(DllYsetupGui.hUser32 == NULL);

    //
    //  Check if running on NT 4 and up.  If so, update the PE header to
    //  indicate this is a 4.0 executable before loading and initializing
    //  UI code to ensure it gets 4.0 visuals.
    //

    YoriLibGetOsVersion(&MajorVersion, &MinorVersion, &BuildNumber);
    if (MajorVersion >= 4) {
        if (!YoriLibEnsureProcessSubsystemVersionAtLeast(4, 0)) {
            return FALSE;
        }
    }

    YoriLibLoadUser32Functions();
    DllYsetupGui.hGdi32 = YoriLibLoadLibraryFromSystemDirectory(_T("GDI32.DLL"));
    DllYsetupGui.hUser32 = YoriLibLoadLibraryFromSystemDirectory(_T("USER32.DLL"));

    if (DllYsetupGui.hGdi32 == NULL ||
        DllYsetupGui.hUser32 == NULL) {

        return FALSE;
    }

    DllYsetupGui.pCreateFontW = (PCREATE_FONTW)GetProcAddress(DllYsetupGui.hGdi32, "CreateFontW");
    DllYsetupGui.pGetDeviceCaps = (PGET_DEVICE_CAPS)GetProcAddress(DllYsetupGui.hGdi32, "GetDeviceCaps");

    DllYsetupGui.pDialogBoxParamW = (PDIALOG_BOX_PARAMW)GetProcAddress(DllYsetupGui.hUser32, "DialogBoxParamW");
    DllYsetupGui.pEnableWindow = (PENABLE_WINDOW)GetProcAddress(DllYsetupGui.hUser32, "EnableWindow");
    DllYsetupGui.pEndDialog = (PEND_DIALOG)GetProcAddress(DllYsetupGui.hUser32, "EndDialog");
    DllYsetupGui.pGetDlgItem = (PGET_DLG_ITEM)GetProcAddress(DllYsetupGui.hUser32, "GetDlgItem");
    DllYsetupGui.pGetWindowDC = (PGET_WINDOW_DC)GetProcAddress(DllYsetupGui.hUser32, "GetWindowDC");
    DllYsetupGui.pLoadIconW = (PLOAD_ICONW)GetProcAddress(DllYsetupGui.hUser32, "LoadIconW");
    DllYsetupGui.pMessageBoxW = (PMESSAGE_BOXW)GetProcAddress(DllYsetupGui.hUser32, "MessageBoxW");
    DllYsetupGui.pReleaseDC = (PRELEASE_DC)GetProcAddress(DllYsetupGui.hUser32, "ReleaseDC");
    DllYsetupGui.pSendMessageW = (PSEND_MESSAGEW)GetProcAddress(DllYsetupGui.hUser32, "SendMessageW");
    DllYsetupGui.pSystemParametersInfoW = (PSYSTEM_PARAMETERS_INFOW)GetProcAddress(DllYsetupGui.hUser32, "SystemParametersInfoW");

    if (DllUser32.pGetDesktopWindow != NULL &&
        DllUser32.pGetWindowRect != NULL &&
        DllUser32.pSetWindowPos != NULL &&
        DllYsetupGui.pCreateFontW != NULL &&
        DllYsetupGui.pGetDeviceCaps != NULL &&
        DllYsetupGui.pDialogBoxParamW != NULL &&
        DllYsetupGui.pEnableWindow != NULL &&
        DllYsetupGui.pEndDialog != NULL &&
        DllYsetupGui.pGetDlgItem != NULL &&
        DllYsetupGui.pGetWindowDC != NULL &&
        DllYsetupGui.pLoadIconW != NULL &&
        DllYsetupGui.pMessageBoxW != NULL &&
        DllYsetupGui.pReleaseDC != NULL &&
        DllYsetupGui.pSendMessageW != NULL &&
        DllYsetupGui.pSystemParametersInfoW != NULL) {

        return TRUE;
    }

    return FALSE;
}

/**
 GetDlgItemText implemented as a SendMessage wrapper to avoid additional
 imports.

 @param hWnd A handle to the dialog window.

 @param DlgId The control within the dialog.

 @param String Pointer to a string buffer to populate with the text of the
        control.

 @param StringLength The length of the buffer specified by String.

 @return The number of characters obtained.
 */
DWORD
SetupGuiGetDlgItemText(
    __in HWND hWnd,
    __in INT DlgId,
    __in LPCWSTR String,
    __in INT StringLength
    )
{
    HWND hWndDlg;
    hWndDlg = DllYsetupGui.pGetDlgItem(hWnd, DlgId);
    return (DWORD)DllYsetupGui.pSendMessageW(hWndDlg, WM_GETTEXT, StringLength, (DWORD_PTR)String);
}

/**
 SetDlgItemText implemented as a SendMessage wrapper to avoid additional
 imports.

 @param hWnd A handle to the dialog window.

 @param DlgId The control within the dialog.

 @param String Pointer to a string to set into the control.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetupGuiSetDlgItemText(
    __in HWND hWnd,
    __in INT DlgId,
    __in LPCWSTR String
    )
{
    HWND hWndDlg;
    hWndDlg = DllYsetupGui.pGetDlgItem(hWnd, DlgId);
    return (DWORD)DllYsetupGui.pSendMessageW(hWndDlg, WM_SETTEXT, 0, (DWORD_PTR)String);
}

/**
 CheckDlgButton implemented as a SendMessage wrapper to avoid additional
 imports.

 @param hWnd A handle to the dialog window.

 @param DlgId The control within the dialog.

 @param Check The new state of the check box.
 */
VOID
SetupGuiCheckDlgButton(
    __in HWND hWnd,
    __in INT DlgId,
    __in DWORD Check
    )
{
    HWND hWndDlg;
    hWndDlg = DllYsetupGui.pGetDlgItem(hWnd, DlgId);
    DllYsetupGui.pSendMessageW(hWndDlg, BM_SETCHECK, Check, 0);
}

/**
 IsDlgButtonChecked implemented as a SendMessage wrapper to avoid additional
 imports.

 @param hWnd A handle to the dialog window.

 @param DlgId The control within the dialog.

 @return The check state of the check box.
 */
DWORD
SetupGuiIsDlgButtonChecked(
    __in HWND hWnd,
    __in INT DlgId
    )
{
    HWND hWndDlg;
    hWndDlg = DllYsetupGui.pGetDlgItem(hWnd, DlgId);
    return (DWORD)DllYsetupGui.pSendMessageW(hWndDlg, BM_GETCHECK, 0, 0);
}

/**
 If the current operating system has a version of User32 that can access
 data beyond the end of the string, return TRUE to indicate that it should
 copy into a larger string to avoid faults.
 
 @return TRUE if the current system depends on extra bytes at the end of
         strings, FALSE if the system will respect string length.
 */
BOOLEAN
SetupGuiUserRequiresStringPadding(VOID)
{
    DWORD OsVerMajor;
    DWORD OsVerMinor;
    DWORD OsBuildNumber;

    //
    //  NT 3.1 User32 walks off the end of strings.  Add an extra WCHAR so
    //  that 32 bit reads won't fault after the end.
    //

    YoriLibGetOsVersion(&OsVerMajor, &OsVerMinor, &OsBuildNumber);
    if (OsBuildNumber <= 528) {
        return TRUE;
    }
    return FALSE;
}

/**
 Update the status line at the bottom of the dialog as install is underway.

 @param Text Pointer to the text to display in the status line.

 @param Context Context, which for the GUI module refers to a dialog handle.
 */
VOID
SetupGuiUpdateStatus(
    __in PCYORI_STRING Text,
    __in PVOID Context
    )
{
    HWND hDlg;
    YORI_STRING PaddedString;

    hDlg = (HWND)Context;

    ASSERT(YoriLibIsStringNullTerminated(Text));

    if (SetupGuiUserRequiresStringPadding()) {
        if (YoriLibAllocateString(&PaddedString, Text->LengthInChars + 2)) {
            memcpy(PaddedString.StartOfString, Text->StartOfString, Text->LengthInChars * sizeof(TCHAR));
            PaddedString.LengthInChars = Text->LengthInChars;
            PaddedString.StartOfString[PaddedString.LengthInChars] = '\0';
            SetupGuiSetDlgItemText(hDlg, IDC_STATUS, PaddedString.StartOfString);
            YoriLibFreeStringContents(&PaddedString);
        }
        return;
    }

    SetupGuiSetDlgItemText(hDlg, IDC_STATUS, Text->StartOfString);
}

/**
 Upconvert a constant ANSI string to a UNICODE_STRING, adding padding as
 required to work around NT 3.1's beyond-buffer-end walk.

 @param ConstString Pointer to a constant ANSI string.

 @param UnicodeString On successful completion, populated with a newly
        allocated unicode string containing the message in ConstString.
        The caller is expected to free this with
        @ref YoriLibFreeStringContents .

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
SetupGuiConstAnsiToUserUnicode(
    __in LPCSTR ConstString,
    __out PYORI_STRING UnicodeString
    )
{
    YORI_ALLOC_SIZE_T Length;
    Length = (YORI_ALLOC_SIZE_T)strlen(ConstString);
    YoriLibInitEmptyString(UnicodeString);

    //
    //  Add space for a NULL.
    //

    Length = Length + 1;

    //
    //  NT 3.1 User32 walks off the end of strings.  Add an extra WCHAR so
    //  that 32 bit reads won't fault after the end.
    //

    if (SetupGuiUserRequiresStringPadding()) {
        Length = Length + 1;
    }

    if (YoriLibAllocateString(UnicodeString, Length)) {
        YoriLibYPrintf(UnicodeString, _T("%hs"), ConstString);
        return TRUE;
    }

    return FALSE;
}

/**
 Install the user specified set of packages and options from the dialog.

 @param hDlg Specifies the hWnd of the dialog box.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetupGuiInstallSelectedFromDialog(
    __in HWND hDlg
    )
{
    YORI_ALLOC_SIZE_T LengthNeeded;
    YORI_STRING InstallDir;
    BOOL Result = FALSE;
    YSETUP_INSTALL_TYPE InstallType;
    DWORD InstallOptions;
    YORI_STRING ErrorText;
    BOOLEAN LongNameSupport;

    YoriLibInitEmptyString(&ErrorText);
    InstallOptions = YSETUP_INSTALL_COMPLETION;

    //
    //  Query the install directory
    //

    LengthNeeded = (YORI_ALLOC_SIZE_T)DllYsetupGui.pSendMessageW(DllYsetupGui.pGetDlgItem(hDlg, IDC_INSTALLDIR), WM_GETTEXTLENGTH, 0, 0);
    if (!YoriLibAllocateString(&InstallDir, LengthNeeded + 1)) {
        DllYsetupGui.pMessageBoxW(hDlg, _T("Installation failed."), _T("Installation failed."), MB_ICONSTOP);
        return FALSE;
    }
    InstallDir.LengthInChars = (YORI_ALLOC_SIZE_T)
        SetupGuiGetDlgItemText(hDlg, IDC_INSTALLDIR, InstallDir.StartOfString, InstallDir.LengthAllocated);


    //
    //  Count the number of packages we want to install
    //

    if (SetupGuiIsDlgButtonChecked(hDlg, IDC_COMPLETE)) {
        InstallType = InstallTypeComplete;
    } else if (SetupGuiIsDlgButtonChecked(hDlg, IDC_COREONLY)) {
        InstallType = InstallTypeCore;
    } else {
        InstallType = InstallTypeTypical;
    }

    if (SetupGuiIsDlgButtonChecked(hDlg, IDC_SYMBOLS)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_SYMBOLS;
    }

    if (SetupGuiIsDlgButtonChecked(hDlg, IDC_SOURCE)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_SOURCE;
    }

    if (SetupGuiIsDlgButtonChecked(hDlg, IDC_DESKTOP_SHORTCUT)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_DESKTOP_SHORTCUT;
    }

    if (SetupGuiIsDlgButtonChecked(hDlg, IDC_START_SHORTCUT)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_START_SHORTCUT;
    }

    if (SetupGuiIsDlgButtonChecked(hDlg, IDC_TERMINAL_PROFILE)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_TERMINAL_PROFILE;
    }

    if (SetupGuiIsDlgButtonChecked(hDlg, IDC_USER_PATH)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_USER_PATH;
    }

    if (SetupGuiIsDlgButtonChecked(hDlg, IDC_SYSTEM_PATH)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_SYSTEM_PATH;
    }

    if (SetupGuiIsDlgButtonChecked(hDlg, IDC_UNINSTALL)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_UNINSTALL;
    }

    LongNameSupport = TRUE;
    if (!YoriLibPathSupportsLongNames(&InstallDir, &LongNameSupport)) {
        LongNameSupport = TRUE;
    }

    if (!LongNameSupport) {
        YORI_STRING MessageString;
        LPCSTR LongNameMessage;
        LongNameMessage = SetupGetNoLongFileNamesMessage(InstallOptions);
        if (SetupGuiConstAnsiToUserUnicode(LongNameMessage, &MessageString)) {
            DllYsetupGui.pMessageBoxW(hDlg,
                                      MessageString.StartOfString,
                                      _T("No long file name support"),
                                      MB_ICONEXCLAMATION);
            InstallOptions = InstallOptions & ~(YSETUP_INSTALL_SOURCE | YSETUP_INSTALL_COMPLETION);
            YoriLibFreeStringContents(&MessageString);
        }
    }

    Result = SetupInstallSelectedWithOptions(&InstallDir, InstallType, InstallOptions, SetupGuiUpdateStatus, hDlg, &ErrorText);

    if (Result) {
        DllYsetupGui.pMessageBoxW(hDlg, ErrorText.StartOfString, _T("Installation complete."), MB_ICONINFORMATION);
    } else {
        DllYsetupGui.pMessageBoxW(hDlg, ErrorText.StartOfString, _T("Installation failed."), MB_ICONSTOP);
    }

    YoriLibFreeStringContents(&InstallDir);
    YoriLibFreeStringContents(&ErrorText);
    return Result;
}

/**
 The DialogProc for the setup dialog box.

 @param hDlg Specifies the hWnd of the dialog box.

 @param uMsg Specifies the window message received by the dialog box.

 @param wParam Specifies the first parameter to the window message.

 @param lParam Specifies the second parameter to the window message.

 @return TRUE to indicate the message was successfully processed.
 */
BOOL CALLBACK
SetupGuiDialogProc(
    __in HWND hDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    RECT rcDesktop, rcDlg, rcNew;
    WORD CtrlId;
    HICON hIcon;
    YORI_STRING InstallDir;
    DWORD OsVerMajor;
    DWORD OsVerMinor;
    DWORD OsBuildNumber;

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg) {
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_COREONLY:
                case IDC_TYPICAL:
                case IDC_COMPLETE:
                    for (CtrlId = IDC_COREONLY; CtrlId <= IDC_COMPLETE; CtrlId++) {
                        SetupGuiCheckDlgButton(hDlg, CtrlId, FALSE);
                    }
                    SetupGuiCheckDlgButton(hDlg, LOWORD(wParam), TRUE);
                    break;
                case IDC_DESKTOP_SHORTCUT:
                case IDC_START_SHORTCUT:
                case IDC_TERMINAL_PROFILE:
                case IDC_SYSTEM_PATH:
                case IDC_USER_PATH:
                case IDC_SOURCE:
                case IDC_SYMBOLS:
                case IDC_UNINSTALL:
                    CtrlId = LOWORD(wParam);
                    if (SetupGuiIsDlgButtonChecked(hDlg, CtrlId)) {
                        SetupGuiCheckDlgButton(hDlg, CtrlId, FALSE);
                    } else {
                        SetupGuiCheckDlgButton(hDlg, CtrlId, TRUE);
                    }
                    break;
                case IDC_OK:
                    if (!SetupGuiInstallSelectedFromDialog(hDlg)) {
                        DllYsetupGui.pEndDialog(hDlg, FALSE);
                    } else {
                        DllYsetupGui.pEndDialog(hDlg, TRUE);
                    }
                    return TRUE;
                case IDC_CANCEL:
                    DllYsetupGui.pEndDialog(hDlg, FALSE);
                    return TRUE;
                case IDC_BROWSE:
                    if (DllShell32.pSHBrowseForFolderW != NULL &&
                        DllShell32.pSHGetPathFromIDListW != NULL) {
                        YORI_BROWSEINFO BrowseInfo;
                        PVOID ShellIdentifierForPath;
                        ZeroMemory(&BrowseInfo, sizeof(BrowseInfo));
                        BrowseInfo.hWndOwner = hDlg;
                        BrowseInfo.Title = _T("Please select a folder to install Yori:");
                        BrowseInfo.Flags = 0x51;
                        ShellIdentifierForPath = DllShell32.pSHBrowseForFolderW(&BrowseInfo);
                        if (ShellIdentifierForPath != NULL) {
                            YoriLibAllocateString(&InstallDir, MAX_PATH);
                            DllShell32.pSHGetPathFromIDListW(ShellIdentifierForPath, InstallDir.StartOfString);
                            SetupGuiSetDlgItemText(hDlg, IDC_INSTALLDIR, InstallDir.StartOfString);
                            YoriLibFreeStringContents(&InstallDir);
                            if (DllOle32.pCoTaskMemFree != NULL) {
                                DllOle32.pCoTaskMemFree(ShellIdentifierForPath);
                            }
                        }
                    }
            }
            break;
        case WM_CLOSE:
            DllYsetupGui.pEndDialog(hDlg, 0);
            return TRUE;
        case WM_INITDIALOG:
            hIcon = DllYsetupGui.pLoadIconW(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
            DllYsetupGui.pSendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            DllYsetupGui.pSendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

            //
            //  Get the primary monitor's display size.  This is reduced by
            //  the size of the taskbar on systems which have one.  If not,
            //  use the entire desktop.
            //

            if (!DllYsetupGui.pSystemParametersInfoW(SPI_GETWORKAREA, 0, &rcDesktop, 0)) {
                DllUser32.pGetWindowRect(DllUser32.pGetDesktopWindow(), &rcDesktop);
            }

            //
            //  Center the dialog on the display
            //

            DllUser32.pGetWindowRect(hDlg, &rcDlg);

            rcNew.left = ((rcDesktop.right - rcDesktop.left) - (rcDlg.right - rcDlg.left)) / 2;
            rcNew.top = ((rcDesktop.bottom - rcDesktop.top) - (rcDlg.bottom - rcDlg.top)) / 2;

            DllUser32.pSetWindowPos(hDlg, HWND_TOP, rcNew.left, rcNew.top, 0, 0, SWP_NOSIZE);

            {
                TCHAR Version[32];

#if YORI_BUILD_ID
                YoriLibSPrintf(Version, _T("%i.%02i.%i"), YORI_VER_MAJOR, YORI_VER_MINOR, YORI_BUILD_ID);
#else
                YoriLibSPrintf(Version, _T("%i.%02i"), YORI_VER_MAJOR, YORI_VER_MINOR);
#endif
                SetupGuiSetDlgItemText(hDlg, IDC_VERSION, Version);
            }

            SetupGetDefaultInstallDir(&InstallDir);
            SetupGuiSetDlgItemText(hDlg, IDC_INSTALLDIR, InstallDir.StartOfString);
            YoriLibFreeStringContents(&InstallDir);
            SetupGuiCheckDlgButton(hDlg, IDC_TYPICAL, TRUE);

            //
            //  On NT 3.5x try to set the font to something not bold that has
            //  similar geometry to NT 4.0.  This helps ensure the text fits
            //  within the controls, and it just looks nicer.  Unfortunately
            //  the dialog has already been created by this point, so the size
            //  of the controls and the dialog is set according to the default
            //  font's specification.  Since the default font is larger than
            //  this one, the result is typically a needlessly large window.
            //

            YoriLibGetOsVersion(&OsVerMajor, &OsVerMinor, &OsBuildNumber);
            if (OsVerMajor < 4) {

                INT FontHeight;
                HFONT hFont;
                HDC hDC;
                DWORD Index;
                UINT ControlArray[] = {
                    IDC_INSTALLDIR,
                    IDC_OK,
                    IDC_CANCEL,
                    IDC_BROWSE,
                    IDC_STATUS,
                    IDC_VERSION,
                    IDC_LABEL_INSTALLDIR,
                    IDC_LABEL_INSTALLTYPE,
                    IDC_LABEL_COREDESC,
                    IDC_LABEL_TYPICALDESC,
                    IDC_LABEL_COMPLETEDESC,
                    IDC_LABEL_INSTALLOPTIONS,
                    IDC_COREONLY,
                    IDC_TYPICAL,
                    IDC_COMPLETE,
                    IDC_DESKTOP_SHORTCUT,
                    IDC_START_SHORTCUT,
                    IDC_TERMINAL_PROFILE,
                    IDC_SYSTEM_PATH,
                    IDC_USER_PATH,
                    IDC_SOURCE,
                    IDC_SYMBOLS,
                    IDC_UNINSTALL,
                };

                hDC = DllYsetupGui.pGetWindowDC(hDlg);
                FontHeight = -YoriLibMulDiv(8, DllYsetupGui.pGetDeviceCaps(hDC, LOGPIXELSY), 72);
                hFont = DllYsetupGui.pCreateFontW(FontHeight,
                                                  0,
                                                  0,
                                                  0,
                                                  FW_NORMAL,
                                                  FALSE,
                                                  FALSE,
                                                  FALSE,
                                                  DEFAULT_CHARSET,
                                                  OUT_DEFAULT_PRECIS,
                                                  CLIP_DEFAULT_PRECIS,
                                                  DEFAULT_QUALITY,
                                                  FF_DONTCARE,
                                                  _T("MS Sans Serif"));
                DllYsetupGui.pReleaseDC(hDlg, hDC);

                for (Index = 0; Index < sizeof(ControlArray)/sizeof(ControlArray[0]); Index++) {
                    DllYsetupGui.pSendMessageW(DllYsetupGui.pGetDlgItem(hDlg, ControlArray[Index]), WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
                }
                DllYsetupGui.pSendMessageW(hDlg, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

                //
                //  Since we already have an NT 3.5x branch, disable controls
                //  that depend on explorer
                //

                DllYsetupGui.pEnableWindow(DllYsetupGui.pGetDlgItem(hDlg, IDC_BROWSE), FALSE);
                DllYsetupGui.pEnableWindow(DllYsetupGui.pGetDlgItem(hDlg, IDC_DESKTOP_SHORTCUT), FALSE);
                DllYsetupGui.pEnableWindow(DllYsetupGui.pGetDlgItem(hDlg, IDC_TERMINAL_PROFILE), FALSE);
                DllYsetupGui.pEnableWindow(DllYsetupGui.pGetDlgItem(hDlg, IDC_UNINSTALL), FALSE);

                SetupGuiSetDlgItemText(hDlg, IDC_START_SHORTCUT, _T("Install Program Manager &shortcut"));
            } else if (!SetupPlatformSupportsShortcuts()) {

                //
                //  On NT 4 RTM, we can create a start menu shortcut via DDE,
                //  but not a Desktop shortcut.
                //

                DllYsetupGui.pEnableWindow(DllYsetupGui.pGetDlgItem(hDlg, IDC_DESKTOP_SHORTCUT), FALSE);
            }

            return TRUE;

    }
    return FALSE;
}

/**
 Display a dialog allowing the user to select the installation options and
 perform the requested operation.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
BOOL
SetupGuiDisplayUi(VOID)
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    DWORD OsVerMajor;
    DWORD OsVerMinor;
    DWORD OsBuildNumber;

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo)) {
        if (ScreenInfo.dwCursorPosition.X == 0 && ScreenInfo.dwCursorPosition.Y == 0) {
            FreeConsole();
        }
    }

    //
    //  When running on NT 3.5x, attempt to provide a 3D appearence from
    //  Ctl3D32.dll.  Since this is cosmetic, just continue on failure.
    //

    YoriLibGetOsVersion(&OsVerMajor, &OsVerMinor, &OsBuildNumber);
    if (OsVerMajor < 4) {
        YoriLibLoadCtl3d32Functions();
        if (DllCtl3d.pCtl3dRegister != NULL && DllCtl3d.pCtl3dAutoSubclass != NULL) {
            DllCtl3d.pCtl3dRegister(NULL);
            DllCtl3d.pCtl3dAutoSubclass(NULL);
        }
    }

    if (DllCabinet.pFdiCopy == NULL) {
        YORI_STRING MessageString;

        if (SetupGuiConstAnsiToUserUnicode(SetupGetDllMissingMessage(), &MessageString)) {
            DllYsetupGui.pMessageBoxW(NULL,
                                      MessageString.StartOfString,
                                      _T("YSetup"),
                                      MB_ICONEXCLAMATION);
            YoriLibFreeStringContents(&MessageString);
        }
        return TRUE;
    }

    DllYsetupGui.pDialogBoxParamW(NULL, MAKEINTRESOURCE(SETUPDIALOG), NULL, (DLGPROC)SetupGuiDialogProc, 0);
    return TRUE;
}

// vim:sw=4:ts=4:et:
