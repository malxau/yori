/**
 * @file ysetup/resource.h
 *
 * Yori shell bootstrap installer resource definitions
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
 The checkbox for updating the system path.
 */
#define IDC_SYSTEM_PATH 402

/**
 The checkbox for updating the user path.
 */
#define IDC_USER_PATH 403

/**
 The checkbox for installing source.
 */
#define IDC_SOURCE 404

/**
 The checkbox for installing debug symbols.
 */
#define IDC_SYMBOLS 405

/**
 The checkbox for registering an uninstall handler.
 */
#define IDC_UNINSTALL 406


