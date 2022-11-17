/**
 * @file ysetup/ysetup.c
 *
 * Yori shell GUI installer
 *
 * Copyright (c) 2018-2021 Malcolm J. Smith
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

VOID
SetupGuiUpdateStatus(
    __in PCYORI_STRING Text,
    __in PVOID Context
    )
{
    HWND hDlg;

    hDlg = (HWND)Context;

    ASSERT(YoriLibIsStringNullTerminated(Text));
    SetDlgItemText(hDlg, IDC_STATUS, Text->StartOfString);
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
    DWORD LengthNeeded;
    YORI_STRING InstallDir;
    BOOL Result = FALSE;
    YSETUP_INSTALL_TYPE InstallType;
    DWORD InstallOptions;
    YORI_STRING ErrorText;

    YoriLibInitEmptyString(&ErrorText);
    InstallOptions = 0;

    //
    //  Query the install directory and attempt to create it
    //

    LengthNeeded = (DWORD)SendDlgItemMessage(hDlg, IDC_INSTALLDIR, WM_GETTEXTLENGTH, 0, 0);
    if (!YoriLibAllocateString(&InstallDir, LengthNeeded + 1)) {
        MessageBox(hDlg, _T("Installation failed."), _T("Installation failed."), MB_ICONSTOP);
        return FALSE;
    }
    InstallDir.LengthInChars = GetDlgItemText(hDlg, IDC_INSTALLDIR, InstallDir.StartOfString, InstallDir.LengthAllocated);


    //
    //  Count the number of packages we want to install
    //

    if (IsDlgButtonChecked(hDlg, IDC_COMPLETE)) {
        InstallType = InstallTypeComplete;
    } else if (IsDlgButtonChecked(hDlg, IDC_COREONLY)) {
        InstallType = InstallTypeCore;
    } else {
        InstallType = InstallTypeTypical;
    }

    if (IsDlgButtonChecked(hDlg, IDC_SYMBOLS)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_SYMBOLS;
    }

    if (IsDlgButtonChecked(hDlg, IDC_SOURCE)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_SOURCE;
    }

    if (IsDlgButtonChecked(hDlg, IDC_DESKTOP_SHORTCUT)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_DESKTOP_SHORTCUT;
    }

    if (IsDlgButtonChecked(hDlg, IDC_START_SHORTCUT)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_START_SHORTCUT;
    }

    if (IsDlgButtonChecked(hDlg, IDC_TERMINAL_PROFILE)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_TERMINAL_PROFILE;
    }

    if (IsDlgButtonChecked(hDlg, IDC_USER_PATH)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_USER_PATH;
    }

    if (IsDlgButtonChecked(hDlg, IDC_SYSTEM_PATH)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_SYSTEM_PATH;
    }

    if (IsDlgButtonChecked(hDlg, IDC_UNINSTALL)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_UNINSTALL;
    }

    Result = SetupInstallSelectedWithOptions(&InstallDir, InstallType, InstallOptions, SetupGuiUpdateStatus, hDlg, &ErrorText);

    if (Result) {
        MessageBox(hDlg, ErrorText.StartOfString, _T("Installation complete."), MB_ICONINFORMATION);
    } else {
        MessageBox(hDlg, ErrorText.StartOfString, _T("Installation failed."), MB_ICONSTOP);
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
                        CheckDlgButton(hDlg, CtrlId, FALSE);
                    }
                    CheckDlgButton(hDlg, LOWORD(wParam), TRUE);
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
                    if (IsDlgButtonChecked(hDlg, CtrlId)) {
                        CheckDlgButton(hDlg, CtrlId, FALSE);
                    } else {
                        CheckDlgButton(hDlg, CtrlId, TRUE);
                    }
                    break;
                case IDC_OK:
                    if (!SetupGuiInstallSelectedFromDialog(hDlg)) {
                        EndDialog(hDlg, FALSE);
                    } else {
                        EndDialog(hDlg, TRUE);
                    }
                    return TRUE;
                case IDC_CANCEL:
                    EndDialog(hDlg, FALSE);
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
                            SetDlgItemText(hDlg, IDC_INSTALLDIR, InstallDir.StartOfString);
                            YoriLibFreeStringContents(&InstallDir);
                            if (DllOle32.pCoTaskMemFree != NULL) {
                                DllOle32.pCoTaskMemFree(ShellIdentifierForPath);
                            }
                        }
                    }
            }
            break;
        case WM_CLOSE:
            EndDialog(hDlg, 0);
            return TRUE;
        case WM_INITDIALOG:
            hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

            //
            //  Get the primary monitor's display size.  This is reduced by
            //  the size of the taskbar on systems which have one.  If not,
            //  use the entire desktop.
            //

            if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0)) {
                GetWindowRect(GetDesktopWindow(), &rcDesktop);
            }

            //
            //  Center the dialog on the display
            //

            GetWindowRect(hDlg, &rcDlg);

            rcNew.left = ((rcDesktop.right - rcDesktop.left) - (rcDlg.right - rcDlg.left)) / 2;
            rcNew.top = ((rcDesktop.bottom - rcDesktop.top) - (rcDlg.bottom - rcDlg.top)) / 2;

            SetWindowPos(hDlg, HWND_TOP, rcNew.left, rcNew.top, 0, 0, SWP_NOSIZE);

            {
                TCHAR Version[32];

#if YORI_BUILD_ID
                YoriLibSPrintf(Version, _T("%i.%02i.%i"), YORI_VER_MAJOR, YORI_VER_MINOR, YORI_BUILD_ID);
#else
                YoriLibSPrintf(Version, _T("%i.%02i"), YORI_VER_MAJOR, YORI_VER_MINOR);
#endif
                SetDlgItemText(hDlg, IDC_VERSION, Version);
            }

            SetupGetDefaultInstallDir(&InstallDir);
            SetDlgItemText(hDlg, IDC_INSTALLDIR, InstallDir.StartOfString);
            YoriLibFreeStringContents(&InstallDir);
            CheckDlgButton(hDlg, IDC_TYPICAL, TRUE);

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

                hDC = GetWindowDC(hDlg);
                hFont = CreateFont(-YoriLibMulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),
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
                ReleaseDC(hDlg, hDC);

                for (Index = 0; Index < sizeof(ControlArray)/sizeof(ControlArray[0]); Index++) {
                    SendDlgItemMessage(hDlg, ControlArray[Index], WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
                }
                SendMessage(hDlg, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

                //
                //  Since we already have an NT 3.5x branch, disable controls
                //  that depend on explorer
                //

                EnableWindow(GetDlgItem(hDlg, IDC_BROWSE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_DESKTOP_SHORTCUT), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_TERMINAL_PROFILE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_UNINSTALL), FALSE);

                SetDlgItemText(hDlg, IDC_START_SHORTCUT, _T("Install Program Manager &shortcut"));
            } else if (!SetupPlatformSupportsShortcuts()) {

                //
                //  On NT 4 RTM, we can create a start menu shortcut via DDE,
                //  but not a Desktop shortcut.
                //

                EnableWindow(GetDlgItem(hDlg, IDC_DESKTOP_SHORTCUT), FALSE);
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

    if (DllCabinet.pFdiCopy == NULL ||
        (DllWinInet.hDll == NULL && DllWinHttp.hDll == NULL)) {

        YORI_STRING MessageString;
        LPCSTR DllMissingWarning;
        DllMissingWarning = SetupGetDllMissingMessage();
        YoriLibInitEmptyString(&MessageString);
        YoriLibYPrintf(&MessageString, _T("%hs"), MessageString);
        MessageBox(NULL,
                   MessageString.StartOfString,
                   _T("YSetup"),
                   MB_ICONEXCLAMATION);
        YoriLibFreeStringContents(&MessageString);
        return TRUE;
    }

    DialogBox(NULL, MAKEINTRESOURCE(SETUPDIALOG), NULL, (DLGPROC)SetupGuiDialogProc);
    return TRUE;
}

// vim:sw=4:ts=4:et:
