/**
 * @file ysetup/ysetup.h
 *
 * Yori shell bootstrap installer
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

__success(return)
BOOL
SetupFindLocalPkgPath(
    __out PYORI_STRING LocalPath
    );

BOOL
SetupPlatformSupportsShortcuts(VOID);

BOOL
SetupGetDefaultInstallDir(
    __out PYORI_STRING InstallDir
    );

LPCSTR
SetupGetDllMissingMessage(VOID);

LPCSTR
SetupGetNoLongFileNamesMessage(
    __in DWORD InstallOptions
    );

/**
 The set of installation types supported by this program.
 */
typedef enum _YSETUP_INSTALL_TYPE {
    InstallTypeDefault = 0,
    InstallTypeCore = 1,
    InstallTypeTypical = 2,
    InstallTypeComplete = 3
} YSETUP_INSTALL_TYPE;

/**
 If set, debugging symbols should be installed.
 */
#define YSETUP_INSTALL_SYMBOLS           0x00000001

/**
 If set, source code should be installed.
 */
#define YSETUP_INSTALL_SOURCE            0x00000002

/**
 If set, a desktop shortcut should be installed.
 */
#define YSETUP_INSTALL_DESKTOP_SHORTCUT  0x00000004

/**
 If set, a start menu shortcut should be installed.
 */
#define YSETUP_INSTALL_START_SHORTCUT    0x00000008

/**
 If set, a Windows Terminal profile should be installed.
 */
#define YSETUP_INSTALL_TERMINAL_PROFILE  0x00000010

/**
 If set, the user path environment should include the install directory.
 */
#define YSETUP_INSTALL_USER_PATH         0x00000020

/**
 If set, the system path environment should include the install directory.
 */
#define YSETUP_INSTALL_SYSTEM_PATH       0x00000040

/**
 If set, an uninstall entry should be installed.
 */
#define YSETUP_INSTALL_UNINSTALL         0x00000080

/**
 If set, completion scripts should be installed.
 */
#define YSETUP_INSTALL_COMPLETION        0x00000100

/**
 A callback function that can be invoked to indicate status as installation
 progresses.
 */
typedef VOID YSETUP_STATUS_CALLBACK(PCYORI_STRING, PVOID);

/**
 A pointer to a callback function that can be invoked to indicate status as
 installation progresses.
 */
typedef YSETUP_STATUS_CALLBACK *PYSETUP_STATUS_CALLBACK;

__success(return)
BOOL
SetupInstallSelectedWithOptions(
    __in PYORI_STRING InstallDir,
    __in YSETUP_INSTALL_TYPE InstallType,
    __in DWORD InstallOptions,
    __in PYSETUP_STATUS_CALLBACK StatusCallback,
    __in_opt PVOID StatusContext,
    __out _On_failure_(_Post_valid_) PYORI_STRING ErrorText
    );

BOOLEAN
SetupGuiInitialize(VOID);

BOOL
SetupGuiDisplayUi(VOID);

BOOL
SetupTuiDisplayUi(VOID);

