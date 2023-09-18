/**
 * @file ysetup/tui.c
 *
 * Yori shell TUI installer
 *
 * Copyright (c) 2021 Malcolm J. Smith
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
#include <yoriwin.h>
#include <yoridlg.h>
#include "ysetup.h"
#include "resource.h"

/**
 Update the status control within the TUI installer dialog.

 @param Text Pointer to the current status.

 @param Context Pointer to the window handle.
 */
VOID
SetupTuiUpdateStatus(
    __in PCYORI_STRING Text,
    __in PVOID Context
    )
{
    COORD ClientSize;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_WINDOW_HANDLE Window;
    YORI_STRING DisplayText;

    Window = (PYORI_WIN_WINDOW_HANDLE)Context;

    Ctrl = YoriWinFindControlById(Window, IDC_STATUS);
    YoriWinGetControlClientSize(Ctrl, &ClientSize);

    YoriLibInitEmptyString(&DisplayText);
    DisplayText.StartOfString = Text->StartOfString;
    if (Text->LengthInChars > (DWORD)ClientSize.X) {
        DisplayText.LengthInChars = (DWORD)ClientSize.X;
    } else {
        DisplayText.LengthInChars = Text->LengthInChars;
    }

    YoriWinLabelSetCaption(Ctrl, &DisplayText);
    YoriWinDisplayWindowContents(Window);
}


/**
 Return TRUE if a checkbox is checked, identifying the control via a parent
 and control ID (similar to Win32.)

 @param Window Pointer to the parent window.

 @param CtrlId The control ID within the parent.

 @return TRUE to indicate the checkbox is checked, FALSE if it is not.
 */
BOOLEAN
SetupIsCheckboxChecked(
    __in PYORI_WIN_WINDOW_HANDLE Window,
    __in DWORD CtrlId
    )
{
    PYORI_WIN_CTRL_HANDLE Ctrl;
    Ctrl = YoriWinFindControlById(Window, CtrlId);
    return YoriWinCheckboxIsChecked(Ctrl);
}

/**
 Install the user specified set of packages and options from the dialog.

 @param WinMgr Specifies the window manager of the dialog box.

 @param Window Specifies the window of the dialog box.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetupTuiInstallSelectedFromDialog(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr,
    __in PYORI_WIN_WINDOW_HANDLE Window
    )
{
    PYORI_WIN_CTRL_HANDLE Ctrl;
    YORI_STRING InstallDir;
    BOOL Result = FALSE;
    YSETUP_INSTALL_TYPE InstallType;
    DWORD InstallOptions;
    YORI_STRING ErrorText;
    YORI_STRING ButtonText;
    YORI_STRING Title;
    BOOLEAN LongNameSupport;

    YoriLibInitEmptyString(&ErrorText);
    InstallOptions = YSETUP_INSTALL_COMPLETION;
    YoriLibConstantString(&ButtonText, _T("&Ok"));

    //
    //  Query the install directory and attempt to create it
    //

    YoriLibInitEmptyString(&InstallDir);
    Ctrl = YoriWinFindControlById(Window, IDC_INSTALLDIR);
    YoriWinEditGetText(Ctrl, &InstallDir);

    //
    //  Count the number of packages we want to install
    //

    Ctrl = YoriWinFindControlById(Window, IDC_COMPLETE);
    if (YoriWinRadioIsSelected(Ctrl)) {
        InstallType = InstallTypeComplete;
    } else {
        Ctrl = YoriWinFindControlById(Window, IDC_COREONLY);
        if (YoriWinRadioIsSelected(Ctrl)) {
            InstallType = InstallTypeCore;
        } else {
            InstallType = InstallTypeTypical;
        }
    }

    if (SetupIsCheckboxChecked(Window, IDC_SYMBOLS)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_SYMBOLS;
    }

    if (SetupIsCheckboxChecked(Window, IDC_SOURCE)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_SOURCE;
    }

    if (SetupIsCheckboxChecked(Window, IDC_DESKTOP_SHORTCUT)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_DESKTOP_SHORTCUT;
    }

    if (SetupIsCheckboxChecked(Window, IDC_START_SHORTCUT)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_START_SHORTCUT;
    }

    if (SetupIsCheckboxChecked(Window, IDC_TERMINAL_PROFILE)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_TERMINAL_PROFILE;
    }

    if (SetupIsCheckboxChecked(Window, IDC_USER_PATH)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_USER_PATH;
    }

    if (SetupIsCheckboxChecked(Window, IDC_SYSTEM_PATH)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_SYSTEM_PATH;
    }

    if (SetupIsCheckboxChecked(Window, IDC_UNINSTALL)) {
        InstallOptions = InstallOptions | YSETUP_INSTALL_UNINSTALL;
    }

    LongNameSupport = TRUE;
    if (!YoriLibPathSupportsLongNames(&InstallDir, &LongNameSupport)) {
        LongNameSupport = TRUE;
    }

    if (!LongNameSupport) {
        LPCSTR LongNameMessage;
        LongNameMessage = SetupGetNoLongFileNamesMessage(InstallOptions);
        YoriLibConstantString(&Title, _T("No long file name support"));
        YoriLibInitEmptyString(&ErrorText);
        YoriLibYPrintf(&ErrorText, _T("%hs"), LongNameMessage);
        if (ErrorText.StartOfString != NULL) {
            YoriDlgMessageBox(WinMgr,
                              &Title,
                              &ErrorText,
                              1,
                              &ButtonText,
                              0,
                              0);
            YoriLibFreeStringContents(&ErrorText);
        }
        InstallOptions = InstallOptions & ~(YSETUP_INSTALL_SOURCE | YSETUP_INSTALL_COMPLETION);
    }

    Result = SetupInstallSelectedWithOptions(&InstallDir, InstallType, InstallOptions, SetupTuiUpdateStatus, Window, &ErrorText);

    if (Result) {
        YoriLibConstantString(&Title, _T("Installation complete."));
    } else {
        YoriLibConstantString(&Title, _T("Installation failed."));
    }

    YoriDlgMessageBox(WinMgr,
                      &Title,
                      &ErrorText,
                      1,
                      &ButtonText,
                      0,
                      0);

    YoriLibFreeStringContents(&InstallDir);
    YoriLibFreeStringContents(&ErrorText);
    return Result;
}

/**
 A global variable to communciate the window manager to button click event
 handlers.  This should probably be obtained from the control/window.
 */
PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgr;

/**
 Indicates that the browse button was clicked within the TUI setup frontend.

 @param Ctrl Pointer to the button control.
 */
VOID
SetupTuiBrowseButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    YORI_STRING Directory;
    YORI_STRING UnescapedDirectory;
    YORI_STRING Title;
    PYORI_WIN_CTRL_HANDLE Parent;
    PYORI_WIN_CTRL_HANDLE EditCtrl;

    Parent = YoriWinGetControlParent(Ctrl);

    YoriLibInitEmptyString(&Directory);
    YoriLibConstantString(&Title, _T("Browse"));
    if (!YoriDlgDir(WinMgr, &Title, 0, NULL, &Directory)) {
        return;
    }

    YoriLibInitEmptyString(&UnescapedDirectory);
    if (!YoriLibUnescapePath(&Directory, &UnescapedDirectory)) {
        YoriLibCloneString(&UnescapedDirectory, &Directory);
    }

    EditCtrl = YoriWinFindControlById(Parent, IDC_INSTALLDIR);
    ASSERT(EditCtrl != NULL);
    __analysis_assume(EditCtrl != NULL);

    YoriWinEditSetText(EditCtrl, &UnescapedDirectory);
    YoriLibFreeStringContents(&Directory);
    YoriLibFreeStringContents(&UnescapedDirectory);
}

/**
 Indicates that the install button was clicked within the TUI setup frontend.

 @param Ctrl Pointer to the button control.
 */
VOID
SetupTuiInstallButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, TRUE);
}

/**
 Indicates that the cancel button was clicked within the TUI setup frontend.

 @param Ctrl Pointer to the button control.
 */
VOID
SetupTuiCancelButtonClicked(
    __in PYORI_WIN_CTRL_HANDLE Ctrl
    )
{
    PYORI_WIN_CTRL_HANDLE Parent;
    Parent = YoriWinGetControlParent(Ctrl);
    YoriWinCloseWindow(Parent, FALSE);
}

/**
 A list of installation types.  Controls will be created with these strings,
 in order.
 */
LPCSTR InstallTypeInfo[] = {
    YSETUP_DLGTEXT_INSTALLCORE,
    YSETUP_DLGTEXT_INSTALLTYPICAL,
    YSETUP_DLGTEXT_INSTALLCOMPLETE
};

/**
 A list of installation options.  Controls will be created with these strings,
 in order.
 */
LPCSTR InstallOptionInfo[] = {
    YSETUP_DLGTEXT_DESKTOPSHORTCUT,
    YSETUP_DLGTEXT_STARTSHORTCUT,
    YSETUP_DLGTEXT_TERMINALPROFILE,
    YSETUP_DLGTEXT_SYSTEMPATH,
    YSETUP_DLGTEXT_USERPATH,
    YSETUP_DLGTEXT_SOURCE,
    YSETUP_DLGTEXT_SYMBOLS,
    YSETUP_DLGTEXT_UNINSTALL
};

/**
 Display a dialog allowing the user to select the installation options and
 perform the requested operation.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
BOOL
SetupTuiDisplayUi(VOID)
{
    PYORI_WIN_WINDOW_HANDLE Parent;
    COORD WindowSize;
    YORI_STRING Title;
    YORI_STRING Caption;
    SMALL_RECT Area;
    YORI_STRING InstallDir;
    PYORI_WIN_CTRL_HANDLE Ctrl;
    PYORI_WIN_CTRL_HANDLE FirstRadioCtrl;
    DWORD Index;
    DWORD OptionAreaHeight;
    DWORD TopPadding;
    DWORD ElementPadding;
    DWORD ElementCount;
    DWORD_PTR Result;

    if (!YoriWinOpenWindowManager(FALSE, &WinMgr)) {
        return FALSE;
    }

    YoriLibConstantString(&Title, _T("Setup"));

    if (DllCabinet.pFdiCopy == NULL) {
        YORI_STRING MessageString;
        YORI_STRING ButtonText;
        LPCSTR DllMissingWarning;
        DllMissingWarning = SetupGetDllMissingMessage();
        YoriLibInitEmptyString(&MessageString);
        YoriLibYPrintf(&MessageString, _T("%hs"), DllMissingWarning);
        YoriLibConstantString(&ButtonText, _T("&Ok"));
        YoriDlgMessageBox(WinMgr,
                          &Title,
                          &MessageString,
                          1,
                          &ButtonText,
                          0,
                          0);
        YoriLibFreeStringContents(&MessageString);
        YoriWinCloseWindowManager(WinMgr);
        return TRUE;
    }

    if (!YoriWinGetWinMgrDimensions(WinMgr, &WindowSize)) {
        WindowSize.X = 80;
        WindowSize.Y = 24;
    } else {
        WindowSize.X = (SHORT)(WindowSize.X * 9 / 10);
        if (WindowSize.X < 80) {
            WindowSize.X = 80;
        }

        WindowSize.Y = (SHORT)(WindowSize.Y * 4 / 5);
        if (WindowSize.Y < 24) {
            WindowSize.Y = 24;
        }
    }

    if (!YoriWinCreateWindow(WinMgr, WindowSize.X, WindowSize.Y, WindowSize.X, WindowSize.Y, YORI_WIN_WINDOW_STYLE_BORDER_SINGLE | YORI_WIN_WINDOW_STYLE_SHADOW_SOLID, &Title, &Parent)) {
        YoriWinCloseWindowManager(WinMgr);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ysetup: Could not display window: terminal too small?\n"));
        return FALSE;
    }

    YoriWinGetClientSize(Parent, &WindowSize);

    if (!YoriLibAllocateString(&Caption, 100)) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriLibYPrintf(&Caption, _T("%hs"), YSETUP_DLGTEXT_INSTALLDIR);

    Area.Left = 1;
    Area.Top = 1;
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars - 1);
    Area.Bottom = 1;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        YoriLibFreeStringContents(&Caption);
        return FALSE;
    }

    Area.Top = 0;
    Area.Bottom = 2;
    Area.Left = (WORD)(Area.Right + 1);
    Area.Right = (WORD)(WindowSize.X - 2 - sizeof(YSETUP_DLGTEXT_BROWSE) - 2);

    Caption.LengthInChars = 0;

    Ctrl = YoriWinEditCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        YoriLibFreeStringContents(&Caption);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, IDC_INSTALLDIR);

    SetupGetDefaultInstallDir(&InstallDir);
    YoriWinEditSetText(Ctrl, &InstallDir);
    YoriLibFreeStringContents(&InstallDir);

    YoriLibYPrintf(&Caption, _T("%hs"), YSETUP_DLGTEXT_BROWSE);

    Area.Left = (WORD)(Area.Right + 1);
    Area.Right = (WORD)(WindowSize.X - 2);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, 0, SetupTuiBrowseButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        YoriLibFreeStringContents(&Caption);
        return FALSE;
    }

    YoriWinSetControlId(Ctrl, IDC_BROWSE);

    //
    //  There's three rows used at the top, four at the bottom, so the
    //  options have to fit in between
    //

    OptionAreaHeight = WindowSize.Y - 5 - Area.Bottom - 1;
    ElementCount = sizeof(InstallTypeInfo)/sizeof(InstallTypeInfo[0]);
    ElementPadding = OptionAreaHeight / ElementCount;

    //
    //  Calculate how much space would be needed, removing the padding from
    //  the bottom element, so it counts as one line rather than
    //  ElementPadding.  Cut that value in half, and move all items down so
    //  they're vertically centered.
    //

    TopPadding = (OptionAreaHeight - (ElementCount - 1) * ElementPadding + 1) / 2;

    FirstRadioCtrl = NULL;

    for (Index = 0; Index < ElementCount; Index++) {

        Area.Left = 3;
        Area.Top = (WORD)(3 + TopPadding + Index * ElementPadding);
        Area.Right = (WORD)(WindowSize.X / 2 - 2);
        Area.Bottom = Area.Top;

        YoriLibYPrintf(&Caption, _T("%hs"), InstallTypeInfo[Index]);

        Ctrl = YoriWinRadioCreate(Parent, &Area, &Caption, FirstRadioCtrl, 0, NULL);
        if (Ctrl == NULL) {
            YoriWinDestroyWindow(Parent);
            YoriWinCloseWindowManager(WinMgr);
            YoriLibFreeStringContents(&Caption);
            return FALSE;
        }

        YoriWinSetControlId(Ctrl, IDC_COREONLY + Index);

        if (FirstRadioCtrl == NULL) {
            FirstRadioCtrl = Ctrl;
        }

        if (IDC_COREONLY + Index == IDC_TYPICAL) {
            YoriWinRadioSelect(Ctrl);
        }
    }

    ElementCount = sizeof(InstallOptionInfo)/sizeof(InstallOptionInfo[0]);
    ElementPadding = OptionAreaHeight / ElementCount;
    TopPadding = (OptionAreaHeight - (ElementCount - 1) * ElementPadding + 1) / 2;

    for (Index = 0; Index < ElementCount; Index++) {
        Area.Left = (WORD)(WindowSize.X / 2);
        Area.Top = (WORD)(3 + TopPadding + Index * ElementPadding);
        Area.Right = (WORD)(WindowSize.X - 2);
        Area.Bottom = Area.Top;
    
        YoriLibYPrintf(&Caption, _T("%hs"), InstallOptionInfo[Index]);

        //
        //  Cap the label to be no more than the label width.  The label
        //  is four cells narrower than the entire control due to the
        //  checkbox part.
        //

        if (Caption.LengthInChars > (DWORD)(Area.Right - Area.Left - 3)) {
            Caption.LengthInChars = (DWORD)(Area.Right - Area.Left - 3);
        }
    
        Ctrl = YoriWinCheckboxCreate(Parent, &Area, &Caption, 0, NULL);
        if (Ctrl == NULL) {
            YoriWinDestroyWindow(Parent);
            YoriWinCloseWindowManager(WinMgr);
            YoriLibFreeStringContents(&Caption);
            return FALSE;
        }

        YoriWinSetControlId(Ctrl, IDC_DESKTOP_SHORTCUT + Index);
    }

    YoriLibYPrintf(&Caption, _T("%hs"), YSETUP_DLGTEXT_PLEASESELECT);

    Area.Left = 1;
    Area.Top = (WORD)(WindowSize.Y - 4);
    Area.Right = (WORD)(WindowSize.X - 1);
    Area.Bottom = Area.Top;

    Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, 0);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        YoriLibFreeStringContents(&Caption);
        return FALSE;
    }
    YoriWinSetControlId(Ctrl, IDC_STATUS);
    YoriLibFreeStringContents(&Caption);

    YoriLibConstantString(&Caption, _T("Install"));

    Area.Left = 1;
    Area.Top = (WORD)(WindowSize.Y - 3);
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars + 3);
    Area.Bottom = (WORD)(Area.Top + 2);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_DEFAULT, SetupTuiInstallButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    YoriLibConstantString(&Caption, _T("Cancel"));

    Area.Left = (WORD)(Area.Right + 2);
    Area.Right = (WORD)(Area.Left + Caption.LengthInChars + 3);

    Ctrl = YoriWinButtonCreate(Parent, &Area, &Caption, YORI_WIN_BUTTON_STYLE_CANCEL, SetupTuiCancelButtonClicked);
    if (Ctrl == NULL) {
        YoriWinDestroyWindow(Parent);
        YoriWinCloseWindowManager(WinMgr);
        return FALSE;
    }

    Area.Top = (WORD)(Area.Top + 1);
    Area.Bottom = Area.Top;
    Area.Left = (WORD)(Area.Right + 2);
    Area.Right = (WORD)(WindowSize.X - 2);

    {
        TCHAR Version[32];
        Caption.StartOfString = Version;
        Caption.LengthAllocated = sizeof(Version)/sizeof(Version[0]);
        Caption.LengthInChars = 0;

#if YORI_BUILD_ID
        YoriLibYPrintf(&Caption, _T("%i.%02i.%i"), YORI_VER_MAJOR, YORI_VER_MINOR, YORI_BUILD_ID);
#else
        YoriLibYPrintf(&Caption, _T("%i.%02i"), YORI_VER_MAJOR, YORI_VER_MINOR);
#endif
        Ctrl = YoriWinLabelCreate(Parent, &Area, &Caption, YORI_WIN_LABEL_STYLE_RIGHT_ALIGN);
        if (Ctrl == NULL) {
            YoriWinDestroyWindow(Parent);
            YoriWinCloseWindowManager(WinMgr);
            return FALSE;
        }
    }

    Result = FALSE;
    if (!YoriWinProcessInputForWindow(Parent, &Result)) {
        Result = FALSE;
    }

    if (Result) {
        SetupTuiInstallSelectedFromDialog(WinMgr, Parent);
    }

    YoriWinDestroyWindow(Parent);
    YoriWinCloseWindowManager(WinMgr);

    return TRUE;
}

// vim:sw=4:ts=4:et:
