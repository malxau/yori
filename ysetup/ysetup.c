/**
 * @file ysetup/ysetup.c
 *
 * Yori shell bootstrap installer
 *
 * Copyright (c) 2018-2020 Malcolm J. Smith
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

/**
 HWND_BROADCAST casts an integer to a pointer, which in 64 bit means
 increasing the size.
 */
#pragma warning(disable: 4306)

/**
 Help text to display to the user.
 */
const
CHAR strHelpText[] =
        "\n"
        "Installs a basic Yori system.\n"
        "\n"
        "YSETUP [-license] [directory]\n";

/**
 Display usage text to the user.
 */
BOOL
YsetupHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ysetup %i.%02i\n"), YSETUP_VER_MAJOR, YSETUP_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 A list of subdirectories from the application to check for packages.
 */
CONST LPTSTR SetupLocalPathsToCheck[] = {
    _T("pkg"),
    _T("yori"),
    _T("ypm"),
    _T("ysetup")
};

/**
 See if there are local packages for installation without using the internet.
 This probes subdirectories of the application's directory.  Subdirectories
 are used to avoid squatting in the downloads folder, ie., not allowing a
 website drive-by to populate the downloads folder with a file that would
 manipulate the installer.

 @param LocalPath On successful completion, populated with a referenced string
        describing the local package location to use.

 @return TRUE to indicate that a local package location has been found which
         should be used, and FALSE to indicate default package locations
         should be used instead.
 */
__success(return)
BOOL
SetupFindLocalPkgPath(
    __out PYORI_STRING LocalPath
    )
{
    DWORD Index;
    int Result;
    YORI_STRING RelativePathToProbe;
    YORI_STRING FullPathToProbe;

    YoriLibInitEmptyString(&RelativePathToProbe);
    YoriLibInitEmptyString(&FullPathToProbe);
    for (Index = 0; Index < sizeof(SetupLocalPathsToCheck)/sizeof(SetupLocalPathsToCheck[0]); Index++) {

        //
        //  Create a path using the specification for the directory of the
        //  running process
        //

        Result = YoriLibYPrintf(&RelativePathToProbe, _T("~APPDIR\\%s\\pkglist.ini"), SetupLocalPathsToCheck[Index]);
        if (Result == -1) {
            YoriLibFreeStringContents(&RelativePathToProbe);
            YoriLibFreeStringContents(&FullPathToProbe);
            return FALSE;
        }

        //
        //  Turn that into a full path for the benefit of Win32
        //

        if (!YoriLibUserStringToSingleFilePath(&RelativePathToProbe, TRUE, &FullPathToProbe)) {
            YoriLibFreeStringContents(&RelativePathToProbe);
            YoriLibFreeStringContents(&FullPathToProbe);
            return FALSE;
        }

        //
        //  See if it exists
        //

        if (GetFileAttributes(FullPathToProbe.StartOfString) != (DWORD)-1) {
            YoriLibFreeStringContents(&RelativePathToProbe);

            //
            //  Remove pkglist.ini.  Note that sizeof includes the NULL so
            //  this removes the trailing slash too
            //

            FullPathToProbe.LengthInChars -= sizeof("pkglist.ini");
            FullPathToProbe.StartOfString[FullPathToProbe.LengthInChars] = '\0';
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Found local packages at %y\n"), &FullPathToProbe);
            memcpy(LocalPath, &FullPathToProbe, sizeof(YORI_STRING));
            return TRUE;
        }

        YoriLibFreeStringContents(&FullPathToProbe);

    }

    //
    //  No local path found, try the internet
    //

    YoriLibFreeStringContents(&RelativePathToProbe);
    YoriLibFreeStringContents(&FullPathToProbe);
    return FALSE;
}

/**
 Install the default set of packages to a specified directory.

 @param InstallDirectory Specifies the directory to install to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetupInstallToDirectory(
    __inout PYORI_STRING InstallDirectory
    )
{
    YORI_STRING LocalPath;
    PYORI_STRING CustomSource;
    YORI_STRING PkgNames[4];

    YoriLibInitEmptyString(&LocalPath);
    CustomSource = NULL;

    if (SetupFindLocalPkgPath(&LocalPath)) {
        CustomSource = &LocalPath;
    }

    if (!YoriLibCreateDirectoryAndParents(InstallDirectory)) {
        DWORD Err = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ysetup: Could not create installation directory %y: %s\n"), InstallDirectory, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&LocalPath);
        return FALSE;
    }

    YoriLibConstantString(&PkgNames[0], _T("yori-ypm"));
    YoriLibConstantString(&PkgNames[1], _T("yori-core"));
    YoriLibConstantString(&PkgNames[2], _T("yori-typical"));
    YoriLibConstantString(&PkgNames[3], _T("yori-completion"));

    if (YoriPkgInstallRemotePackages(PkgNames, sizeof(PkgNames)/sizeof(PkgNames[0]), CustomSource, InstallDirectory, NULL, NULL)) {
        YoriLibFreeStringContents(&LocalPath);
        return TRUE;
    }

    YoriLibFreeStringContents(&LocalPath);
    return FALSE;
}

/**
 Install the user specified set of packages and options from the dialog.

 @param hDlg Specifies the hWnd of the dialog box.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SetupInstallSelectedFromDialog(
    __in HWND hDlg
    )
{
    DWORD LengthNeeded;
    YORI_STRING InstallDir;
    YORI_STRING StatusText;
    PYORI_STRING PkgNames;
    PYORI_STRING PackageUrls;
    YORI_STRING LocalPath;
    YORI_STRING ShortcutNameFullPath[2];
    DWORD ShortcutCount;
    PYORI_STRING CustomSource;
    DWORD PkgCount;
    DWORD PkgIndex;
    DWORD PkgUrlCount;
    BOOL Result = FALSE;
    enum {
        InstallTypeCore = 1,
        InstallTypeTypical = 2,
        InstallTypeComplete = 3
    } InstallType;

    YoriLibInitEmptyString(&LocalPath);
    CustomSource = NULL;
    ShortcutCount = 0;

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
    //  Truncate trailing seperators
    //

    while (InstallDir.LengthInChars > 0 &&
           YoriLibIsSep(InstallDir.StartOfString[InstallDir.LengthInChars - 1])) {

        InstallDir.StartOfString[InstallDir.LengthInChars - 1] = '\0';
        InstallDir.LengthInChars--;
    }

    if (InstallDir.LengthInChars == 0) {
        MessageBox(hDlg, _T("Installation failed."), _T("Installation failed."), MB_ICONSTOP);
        YoriLibFreeStringContents(&InstallDir);
        return FALSE;
    }

    if (!YoriLibCreateDirectoryAndParents(&InstallDir)) {
        MessageBox(hDlg, _T("Failed to create installation directory.  If installing into a system location, you may want to run the installer as Administrator."), _T("Installation failed."), MB_ICONSTOP);
        YoriLibFreeStringContents(&InstallDir);
        return FALSE;
    }

    //
    //  Count the number of packages we want to install
    //

    if (IsDlgButtonChecked(hDlg, IDC_COMPLETE)) {
        PkgCount = 4;
        InstallType = InstallTypeComplete;
    } else if (IsDlgButtonChecked(hDlg, IDC_COREONLY)) {
        PkgCount = 2;
        InstallType = InstallTypeCore;
    } else {
        PkgCount = 3;
        InstallType = InstallTypeTypical;
    }

    if (IsDlgButtonChecked(hDlg, IDC_SYMBOLS)) {
        PkgCount *= 2;
    }

    PkgCount++;

    if (IsDlgButtonChecked(hDlg, IDC_SOURCE)) {
        PkgCount++;
    }

    //
    //  Allocate a package array
    //

    PkgNames = YoriLibMalloc(PkgCount * sizeof(YORI_STRING));
    if (PkgNames == NULL) {
        MessageBox(hDlg, _T("Installation failed."), _T("Installation failed."), MB_ICONSTOP);
        YoriLibFreeStringContents(&InstallDir);
        return FALSE;
    }

    //
    //  Populate the array with the names of what we want to install
    //

    if (IsDlgButtonChecked(hDlg, IDC_SYMBOLS)) {
        YoriLibConstantString(&PkgNames[0], _T("yori-ypm"));
        YoriLibConstantString(&PkgNames[1], _T("yori-ypm-pdb"));
        YoriLibConstantString(&PkgNames[2], _T("yori-core"));
        YoriLibConstantString(&PkgNames[3], _T("yori-core-pdb"));
        PkgIndex = 4;
        if (InstallType >= InstallTypeTypical) {
            YoriLibConstantString(&PkgNames[4], _T("yori-typical"));
            YoriLibConstantString(&PkgNames[5], _T("yori-typical-pdb"));
            PkgIndex = 6;
        }
        if (InstallType >= InstallTypeComplete) {
            YoriLibConstantString(&PkgNames[6], _T("yori-extra"));
            YoriLibConstantString(&PkgNames[7], _T("yori-extra-pdb"));
            PkgIndex = 8;
        }
    } else {
        YoriLibConstantString(&PkgNames[0], _T("yori-ypm"));
        YoriLibConstantString(&PkgNames[1], _T("yori-core"));
        PkgIndex = 2;
        if (InstallType >= InstallTypeTypical) {
            YoriLibConstantString(&PkgNames[2], _T("yori-typical"));
            PkgIndex = 3;
        }
        if (InstallType >= InstallTypeComplete) {
            YoriLibConstantString(&PkgNames[3], _T("yori-extra"));
            PkgIndex = 4;
        }
    }

    YoriLibConstantString(&PkgNames[PkgIndex], _T("yori-completion"));
    PkgIndex++;

    if (IsDlgButtonChecked(hDlg, IDC_SOURCE)) {
        YoriLibConstantString(&PkgNames[PkgIndex], _T("yori-source"));
        PkgIndex++;
    }

    //
    //  Obtain URLs for the specified packages.
    //

    if (SetupFindLocalPkgPath(&LocalPath)) {
        CustomSource = &LocalPath;
    }
    YoriLibInitEmptyString(&StatusText);
    SetDlgItemText(hDlg, IDC_STATUS, _T("Obtaining package URLs..."));
    PkgUrlCount = YoriPkgGetRemotePackageUrls(PkgNames, PkgIndex, CustomSource, &InstallDir, &PackageUrls);

    if (PkgUrlCount != PkgCount) {
        MessageBox(hDlg, _T("Could not locate selected package files."), _T("Installation failed."), MB_ICONSTOP);
        goto Exit;
    }

    //
    //  Install the packages
    //

    for (PkgCount = 0; PkgCount < PkgUrlCount; PkgCount++) {
        YoriLibYPrintf(&StatusText, _T("Installing %i of %i: %y"), PkgCount + 1, PkgUrlCount, &PackageUrls[PkgCount]);
        if (StatusText.StartOfString != NULL) {
            SetDlgItemText(hDlg, IDC_STATUS, StatusText.StartOfString);
        }
        if (!YoriPkgInstallSinglePackage(&PackageUrls[PkgCount], &InstallDir)) {
            YoriLibYPrintf(&StatusText, _T("Failed to install %y from %y"), &PkgNames[PkgCount], &PackageUrls[PkgCount]);
            MessageBox(hDlg, StatusText.StartOfString, _T("Installation failed."), MB_ICONSTOP);
            goto Exit;
        }
    }

    SetDlgItemText(hDlg, IDC_STATUS, _T("Applying installation options..."));

    //
    //  Create shortcuts if requested
    //

    if (IsDlgButtonChecked(hDlg, IDC_DESKTOP_SHORTCUT) ||
        IsDlgButtonChecked(hDlg, IDC_START_SHORTCUT)) {

        YORI_STRING YSetupPkgName;
        YORI_STRING YSetupPkgVersion;
        YORI_STRING YSetupPkgArch;
        YORI_STRING YoriExeFullPath;
        YORI_STRING RelativeShortcutName;
        YORI_STRING Description;

        YoriLibInitEmptyString(&YoriExeFullPath);
        YoriLibConstantString(&Description, _T("Yori"));

        YoriLibYPrintf(&YoriExeFullPath, _T("%y\\yori.exe"), &InstallDir);
        if (YoriExeFullPath.StartOfString == NULL) {
            MessageBox(hDlg, _T("Installation failed."), _T("Installation failed."), MB_ICONSTOP);
            goto Exit;
        }

        if (IsDlgButtonChecked(hDlg, IDC_DESKTOP_SHORTCUT)) {
            YoriLibInitEmptyString(&ShortcutNameFullPath[ShortcutCount]);

            YoriLibConstantString(&RelativeShortcutName, _T("~Desktop\\Yori.lnk"));
            if (!YoriLibUserStringToSingleFilePath(&RelativeShortcutName, TRUE, &ShortcutNameFullPath[ShortcutCount])) {
                YoriLibFreeStringContents(&YoriExeFullPath);
                MessageBox(hDlg, _T("Installation failed."), _T("Installation failed."), MB_ICONSTOP);
                goto Exit;
            }
            ShortcutCount++;
            if (!YoriLibCreateShortcut(&ShortcutNameFullPath[ShortcutCount - 1], &YoriExeFullPath, NULL, &Description, NULL, &YoriExeFullPath, 0, 1, (WORD)-1, TRUE, TRUE)) {
                YoriLibFreeStringContents(&YoriExeFullPath);
                MessageBox(hDlg, _T("Failed to create desktop shortcut."), _T("Installation failed."), MB_ICONSTOP);
                goto Exit;
            }
        }

        if (IsDlgButtonChecked(hDlg, IDC_START_SHORTCUT)) {

            YoriLibInitEmptyString(&ShortcutNameFullPath[ShortcutCount]);

            YoriLibConstantString(&RelativeShortcutName, _T("~Programs\\Yori.lnk"));
            if (!YoriLibUserStringToSingleFilePath(&RelativeShortcutName, TRUE, &ShortcutNameFullPath[ShortcutCount])) {
                YoriLibFreeStringContents(&YoriExeFullPath);
                MessageBox(hDlg, _T("Installation failed."), _T("Installation failed."), MB_ICONSTOP);
                goto Exit;
            }
            ShortcutCount++;
            if (!YoriLibCreateShortcut(&ShortcutNameFullPath[ShortcutCount - 1], &YoriExeFullPath, NULL, &Description, NULL, &YoriExeFullPath, 0, 1, (WORD)-1, TRUE, TRUE)) {
                YoriLibFreeStringContents(&YoriExeFullPath);
                MessageBox(hDlg, _T("Failed to create start menu shortcut."), _T("Installation failed."), MB_ICONSTOP);
                goto Exit;
            }
        }

        YoriLibConstantString(&YSetupPkgName, _T("ysetup-shortcuts"));
        YoriLibConstantString(&YSetupPkgVersion, _T("latest"));
        YoriLibConstantString(&YSetupPkgArch, _T("noarch"));
        YoriPkgInstallPseudoPackage(&YSetupPkgName, &YSetupPkgVersion, &YSetupPkgArch, ShortcutNameFullPath, ShortcutCount, &InstallDir);

        YoriLibFreeStringContents(&YoriExeFullPath);
    }

    //
    //  Update paths if requested
    //

    if (IsDlgButtonChecked(hDlg, IDC_USER_PATH) ||
        IsDlgButtonChecked(hDlg, IDC_SYSTEM_PATH)) {

        BOOL AppendToUserPath = FALSE;
        BOOL AppendToSystemPath = FALSE;

        if (IsDlgButtonChecked(hDlg, IDC_USER_PATH)) {
            AppendToUserPath = TRUE;
        }

        if (IsDlgButtonChecked(hDlg, IDC_SYSTEM_PATH)) {
            AppendToSystemPath = TRUE;
        }

        YoriPkgAppendInstallDirToPath(&InstallDir, AppendToUserPath, AppendToSystemPath);
    }

    //
    //  Try to add an uninstall entry
    //

    if (IsDlgButtonChecked(hDlg, IDC_UNINSTALL)) {

        YORI_STRING VerString;
        TCHAR VerBuffer[32];

#if YORI_BUILD_ID
        YoriLibSPrintf(VerBuffer, _T("%i.%02i.%i"), YSETUP_VER_MAJOR, YSETUP_VER_MINOR, YORI_BUILD_ID);
#else
        YoriLibSPrintf(VerBuffer, _T("%i.%02i"), YSETUP_VER_MAJOR, YSETUP_VER_MINOR);
#endif

        YoriLibConstantString(&VerString, VerBuffer);

        if (!YoriPkgAddUninstallEntry(&InstallDir, &VerString)) {
            MessageBox(hDlg, _T("Could not add uninstall entry."), _T("Error"), MB_ICONINFORMATION);
        }
    }

    SetDlgItemText(hDlg, IDC_STATUS, _T("Installation complete."));
    MessageBox(hDlg, _T("Installation complete."), _T("Installation complete."), MB_ICONINFORMATION);
    Result = TRUE;
Exit:
    for (PkgCount = 0; PkgCount < PkgUrlCount; PkgCount++) {
        YoriLibFreeStringContents(&PackageUrls[PkgCount]);
    }
    for (PkgCount = 0; PkgCount < ShortcutCount; PkgCount++) {
        YoriLibFreeStringContents(&ShortcutNameFullPath[PkgCount]);
    }
    YoriLibDereference(PackageUrls);
    YoriLibFreeStringContents(&InstallDir);
    YoriLibFreeStringContents(&StatusText);
    YoriLibFreeStringContents(&LocalPath);
    YoriLibFree(PkgNames);
    return Result;
}

/**
 The default application install directory, under Program Files.
 */
#define SETUP_APP_DIR "\\Yori"

/**
 The default application install directory, under Program Files, as Unicode.
 */
#define TSETUP_APP_DIR _T("\\Yori")

/**
 Return the default installation directory.  This is normally
 "C:\Program Files\Yori" but the path can be reconfigured.

 @param InstallDir On successful completion, populated with the default
        install path.

 @return TRUE to indicate successful completion, FALSE on error.
 */
BOOL
SetupGetDefaultInstallDir(
    __out PYORI_STRING InstallDir
    )
{
    HKEY hKey;
    DWORD SizeNeeded;
    DWORD RegType;
    DWORD OsVerMajor;
    DWORD OsVerMinor;
    DWORD OsBuildNumber;
    BOOL IsAdmin;
    YORI_STRING Administrators;

    hKey = NULL;

    YoriLibLoadAdvApi32Functions();
    if (DllAdvApi32.pRegOpenKeyExW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllAdvApi32.pRegCloseKey == NULL) {

        goto ReturnDefault;
    }

    //
    //  If the OS provides support for querying group membership (Windows
    //  2000 and above) and the user is not an administrator, default to
    //  a per user installation location.  Otherwise default to a system
    //  location.  Note that local app data didn't exist prior to Windows
    //  2000.
    //

    YoriLibConstantString(&Administrators, _T("Administrators"));
    if (YoriLibIsCurrentUserInGroup(&Administrators, &IsAdmin) && !IsAdmin) {
        YORI_STRING PerUserDir;
        YoriLibConstantString(&PerUserDir, _T("~LOCALAPPDATA\\Yori"));
        if (YoriLibUserStringToSingleFilePath(&PerUserDir, FALSE, InstallDir)) {
            return TRUE;
        }
    }

    //
    //  Query the registry for the location of Program Files.  Because this
    //  installer will download a 64 bit version for a 64 bit system, it wants
    //  to find the "native" program files as opposed to the emulated x86 one.
    //  This registry entry won't exist on 32 bit systems, so it falls back to
    //  the normal program files after that.
    //

    if (DllAdvApi32.pRegOpenKeyExW(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        goto ReturnDefault;
    }

    if (DllAdvApi32.pRegQueryValueExW(hKey, _T("ProgramW6432Dir"), NULL, NULL, NULL, &SizeNeeded) == ERROR_SUCCESS) {
        if (!YoriLibAllocateString(InstallDir, SizeNeeded/sizeof(TCHAR) + sizeof(SETUP_APP_DIR))) {
            goto ReturnDefault;
        }

        SizeNeeded = InstallDir->LengthAllocated * sizeof(TCHAR);
        if (DllAdvApi32.pRegQueryValueExW(hKey, _T("ProgramW6432Dir"), NULL, &RegType, (LPBYTE)InstallDir->StartOfString, &SizeNeeded) == ERROR_SUCCESS) {
            if (RegType != REG_SZ && RegType != REG_EXPAND_SZ) {
                goto ReturnDefault;
            }
            InstallDir->LengthInChars = SizeNeeded / sizeof(TCHAR) - 1;
            if (InstallDir->LengthInChars + sizeof(SETUP_APP_DIR) <= InstallDir->LengthAllocated) {
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6260) // sizeof*sizeof is used for character size
                                // flexibility
#endif
                memcpy(&InstallDir->StartOfString[InstallDir->LengthInChars], TSETUP_APP_DIR, sizeof(SETUP_APP_DIR) * sizeof(TCHAR));
                InstallDir->LengthInChars += sizeof(SETUP_APP_DIR) - 1;
            } else {
                InstallDir->StartOfString[InstallDir->LengthInChars] = '\0';
            }
        }
        DllAdvApi32.pRegCloseKey(hKey);
        return TRUE;
    }

    if (DllAdvApi32.pRegQueryValueExW(hKey, _T("ProgramFilesDir"), NULL, NULL, NULL, &SizeNeeded) == ERROR_SUCCESS) {
        if (!YoriLibAllocateString(InstallDir, SizeNeeded/sizeof(TCHAR) + sizeof("\\Yori"))) {
            goto ReturnDefault;
        }

        SizeNeeded = InstallDir->LengthAllocated * sizeof(TCHAR);
        if (DllAdvApi32.pRegQueryValueExW(hKey, _T("ProgramFilesDir"), NULL, &RegType, (LPBYTE)InstallDir->StartOfString, &SizeNeeded) == ERROR_SUCCESS) {
            if (RegType != REG_SZ && RegType != REG_EXPAND_SZ) {
                goto ReturnDefault;
            }
            InstallDir->LengthInChars = SizeNeeded / sizeof(TCHAR) - 1;
            if (InstallDir->LengthInChars + sizeof(SETUP_APP_DIR) <= InstallDir->LengthAllocated) {
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 6260) // sizeof*sizeof is used for character size
                                // flexibility
#endif
                memcpy(&InstallDir->StartOfString[InstallDir->LengthInChars], TSETUP_APP_DIR, sizeof(SETUP_APP_DIR) * sizeof(TCHAR));
                InstallDir->LengthInChars += sizeof(SETUP_APP_DIR) - 1;
            } else {
                InstallDir->StartOfString[InstallDir->LengthInChars] = '\0';
            }
        }
        DllAdvApi32.pRegCloseKey(hKey);
        return TRUE;
    }

ReturnDefault:
    YoriLibGetOsVersion(&OsVerMajor, &OsVerMinor, &OsBuildNumber);
    {
        TCHAR WindowsDirectory[MAX_PATH];
        DWORD CharsCopied;

        CharsCopied = GetWindowsDirectory(WindowsDirectory, sizeof(WindowsDirectory)/sizeof(WindowsDirectory[0]));
        if (CharsCopied < 2) {
            YoriLibSPrintf(WindowsDirectory, _T("C:"));
        } else {
            WindowsDirectory[2] = '\0';
        }

        if (OsVerMajor < 4) {
            YoriLibYPrintf(InstallDir, _T("%s\\WIN32APP") TSETUP_APP_DIR, WindowsDirectory);
        } else {
            YoriLibYPrintf(InstallDir, _T("%s\\Program Files") TSETUP_APP_DIR, WindowsDirectory);
        }

        if (InstallDir->StartOfString == NULL) {
            YoriLibConstantString(InstallDir, _T("C:\\Program Files") TSETUP_APP_DIR);
        }
    }
    if (hKey != NULL) {
        DllAdvApi32.pRegCloseKey(hKey);
    }
    return TRUE;
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
SetupUiDialogProc(
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
                    if (!SetupInstallSelectedFromDialog(hDlg)) {
                        EndDialog(hDlg, FALSE);
                    } else {
                        EndDialog(hDlg, TRUE);
                    }
                    return TRUE;
                case IDC_CANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;
                case IDC_BROWSE:
                    YoriLibLoadShell32Functions();
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
            GetWindowRect(GetDesktopWindow(), &rcDesktop);
            GetWindowRect(hDlg, &rcDlg);

            rcNew.left = ((rcDesktop.right - rcDesktop.left) - (rcDlg.right - rcDlg.left)) / 2;
            rcNew.top = ((rcDesktop.bottom - rcDesktop.top) - (rcDlg.bottom - rcDlg.top)) / 2;

            SetWindowPos(hDlg, HWND_TOP, rcNew.left, rcNew.top, 0, 0, SWP_NOSIZE);

            {
                TCHAR Version[32];

#if YORI_BUILD_ID
                YoriLibSPrintf(Version, _T("%i.%02i.%i"), YSETUP_VER_MAJOR, YSETUP_VER_MINOR, YORI_BUILD_ID);
#else
                YoriLibSPrintf(Version, _T("%i.%02i"), YSETUP_VER_MAJOR, YSETUP_VER_MINOR);
#endif
                SetDlgItemText(hDlg, IDC_VERSION, Version);
            }

            YoriLibInitEmptyString(&InstallDir);
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
                    IDC_SYSTEM_PATH,
                    IDC_USER_PATH,
                    IDC_SOURCE,
                    IDC_SYMBOLS,
                    IDC_UNINSTALL,
                };

                hDC = GetWindowDC(hDlg);
                hFont = CreateFont(-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),
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
                EnableWindow(GetDlgItem(hDlg, IDC_START_SHORTCUT), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_UNINSTALL), FALSE);
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
SetupDisplayUi()
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    DWORD OsVerMajor;
    DWORD OsVerMinor;
    DWORD OsBuildNumber;

    //
    //  Initialize COM for the benefit of the shell's browse for folder
    //  dialog
    //

    YoriLibLoadOle32Functions();
    if (DllOle32.pCoInitialize != NULL) {
        DllOle32.pCoInitialize(NULL);
    }

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

    YoriLibLoadCabinetFunctions();
    YoriLibLoadWinInetFunctions();

    if (DllCabinet.hDll == NULL || DllWinInet.hDll == NULL) {
        MessageBox(NULL,
                   _T("Ysetup requires Cabinet.dll and WinInet.dll.  These are included with Internet Explorer 5 or later.  At a minimum, WinInet.dll from Internet Explorer 3 and Cabinet.dll from Internet Explorer 5 can be copied to the System32 directory to proceed."),
                   _T("YSetup"),
                   MB_ICONEXCLAMATION);
        return TRUE;
    }

    DialogBox(NULL, MAKEINTRESOURCE(SETUPDIALOG), NULL, (DLGPROC)SetupUiDialogProc);
    return TRUE;
}


/**
 The main entrypoint for the setup cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;
    YORI_STRING NewDirectory;

    YoriLibInitEmptyString(&NewDirectory);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                YsetupHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018-2020"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartArg = i + 1;
                break;
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

    if (StartArg > 0) {
        YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &NewDirectory);
        if (!SetupInstallToDirectory(&NewDirectory)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("ysetup: install failed\n"));
            YoriLibFreeStringContents(&NewDirectory);
            return EXIT_FAILURE;
        }
        YoriLibFreeStringContents(&NewDirectory);
    } else {
        SetupDisplayUi();
    }


    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
