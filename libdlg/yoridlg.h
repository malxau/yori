/**
 * @file libdlg/yoridlg.h
 *
 * Header for generic dialog box routines
 *
 * Copyright (c) 2019-2020 Malcolm J. Smith
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
 A structure to describe a single value within a custom pull down list at the
 bottom of the dialog.
 */
typedef struct _YORI_DLG_FILE_CUSTOM_VALUE {

    /**
     The text to display in the value.
     */
    YORI_STRING ValueText;
} YORI_DLG_FILE_CUSTOM_VALUE, *PYORI_DLG_FILE_CUSTOM_VALUE;

/**
 A structure to describe a single pull down list at the bottom of the dialog.
 */
typedef struct _YORI_DLG_FILE_CUSTOM_OPTION {

    /**
     The caption to display before the pull down list control.
     */
    YORI_STRING Description;

    /**
     On successful completion of the dialog, updated to contain the index of
     the selected value.
     */
    DWORD SelectedValue;

    /**
     The number of values in the pull down list control.
     */
    DWORD ValueCount;

    /**
     An array of values to display in the pull down list control.
     */
    PYORI_DLG_FILE_CUSTOM_VALUE Values;
} YORI_DLG_FILE_CUSTOM_OPTION, *PYORI_DLG_FILE_CUSTOM_OPTION;

__success(return)
BOOLEAN
YoriDlgDir(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in DWORD OptionCount,
    __in_opt PYORI_DLG_FILE_CUSTOM_OPTION Options,
    __inout PYORI_STRING Text
    );

__success(return)
BOOLEAN
YoriDlgFile(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in DWORD OptionCount,
    __in_opt PYORI_DLG_FILE_CUSTOM_OPTION Options,
    __inout PYORI_STRING Text
    );

__success(return)
BOOLEAN
YoriDlgFindText(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in PYORI_STRING InitialText,
    __out PBOOLEAN MatchCase,
    __inout PYORI_STRING Text
    );

__success(return)
BOOLEAN
YoriDlgInput(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __inout PYORI_STRING Text
    );

__success(return)
DWORD
YoriDlgMessageBox(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __in PYORI_STRING Text,
    __in DWORD NumButtons,
    __in PYORI_STRING ButtonTexts,
    __in DWORD DefaultIndex,
    __in DWORD CancelIndex
    );

WORD
YoriDlgReplaceGetDialogHeight(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle
    );

__success(return)
BOOLEAN
YoriDlgReplaceText(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in WORD DesiredLeft,
    __in WORD DesiredTop,
    __in PYORI_STRING Title,
    __in PYORI_STRING InitialBeforeText,
    __in PYORI_STRING InitialAfterText,
    __out PBOOLEAN MatchCase,
    __out PBOOLEAN ReplaceAll,
    __inout PYORI_STRING BeforeText,
    __inout PYORI_STRING AfterText
    );

// vim:sw=4:ts=4:et:
