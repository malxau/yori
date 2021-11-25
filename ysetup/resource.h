/**
 * @file ysetup/resource.h
 *
 * Yori shell bootstrap installer resource definitions
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

/**
 Dialog text for install directory
 */
#define YSETUP_DLGTEXT_INSTALLDIR      "&Install directory:"

/**
 Dialog text for browse
 */
#define YSETUP_DLGTEXT_BROWSE          "&Browse..."

/**
 Dialog text for install core
 */
#define YSETUP_DLGTEXT_INSTALLCORE     "Install C&ore"

/**
 Dialog text for install typical
 */
#define YSETUP_DLGTEXT_INSTALLTYPICAL  "Install &Typical"

/**
 Dialog text for install complete
 */
#define YSETUP_DLGTEXT_INSTALLCOMPLETE "Install &Complete"

/**
 Dialog text for creating a desktop shortcut
 */
#define YSETUP_DLGTEXT_DESKTOPSHORTCUT  "Install &Desktop shortcut"

/**
 Dialog text for creating a start menu shortcut
 */
#define YSETUP_DLGTEXT_STARTSHORTCUT    "Install &Start Menu shortcut"

/**
 Dialog text for creating a windows terminal profile
 */
#define YSETUP_DLGTEXT_TERMINALPROFILE  "Install &Windows Terminal profile"

/**
 Dialog text for updating system path
 */
#define YSETUP_DLGTEXT_SYSTEMPATH       "Add Yori to s&ystem path"

/**
 Dialog text for updating user path
 */
#define YSETUP_DLGTEXT_USERPATH         "Add Yori to user &path"

/**
 Dialog text for installing source code
 */
#define YSETUP_DLGTEXT_SOURCE           "Install sou&rce code"

/**
 Dialog text for installing debugging symbols
 */
#define YSETUP_DLGTEXT_SYMBOLS          "Install debugging symbols"

/**
 Dialog text for installing an uninstall entry
 */
#define YSETUP_DLGTEXT_UNINSTALL        "Register &uninstall handler"

/**
 Dialog text for initial status line
 */
#define YSETUP_DLGTEXT_PLEASESELECT     "Please select installation options"

/**
 The identifier for the dialog box.
 */
#define SETUPDIALOG 100

/**
 The edit control in which the user can enter the install directory.
 */
#define IDC_INSTALLDIR 201

/**
 The OK or Install button.
 */
#define IDC_OK 202

/**
 The cancel button.
 */
#define IDC_CANCEL 203

/**
 The browse button.
 */
#define IDC_BROWSE 204

/**
 The text to update with installation progress.
 */
#define IDC_STATUS 205

/**
 The text to update with the setup version.
 */
#define IDC_VERSION 206

/**
 The text label for the install directory.
 */
#define IDC_LABEL_INSTALLDIR 250

/**
 The text label for the install type.
 */
#define IDC_LABEL_INSTALLTYPE 251

/**
 The text label for the core installation option.
 */
#define IDC_LABEL_COREDESC 252

/**
 The text label for the typical installation option.
 */
#define IDC_LABEL_TYPICALDESC 253

/**
 The text label for the complete installation option.
 */
#define IDC_LABEL_COMPLETEDESC 254

/**
 The text label for the installation options.
 */
#define IDC_LABEL_INSTALLOPTIONS 255

//
//  Code depends on these being sequential
//

/**
 The radio button for installing core options only.
 */
#define IDC_COREONLY 300

/**
 The radio button for installing typical options.
 */
#define IDC_TYPICAL 301

/**
 The radio button for installing everything.
 */
#define IDC_COMPLETE 302

//
//  Code depends on these being sequential
//


/**
 The checkbox for creating a desktop shortcut.
 */
#define IDC_DESKTOP_SHORTCUT 400

/**
 The checkbox for creating a start menu shortcut.
 */
#define IDC_START_SHORTCUT 401

/**
 The checkbox for creating a Windows Terminal profile.
 */
#define IDC_TERMINAL_PROFILE 402

/**
 The checkbox for updating the system path.
 */
#define IDC_SYSTEM_PATH 403

/**
 The checkbox for updating the user path.
 */
#define IDC_USER_PATH 404

/**
 The checkbox for installing source.
 */
#define IDC_SOURCE 405

/**
 The checkbox for installing debug symbols.
 */
#define IDC_SYMBOLS 406

/**
 The checkbox for registering an uninstall handler.
 */
#define IDC_UNINSTALL 407


