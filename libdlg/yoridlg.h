/**
 * @file libdlg/yoridlg.h
 *
 * Header for generic dialog box routines
 *
 * Copyright (c) 2019 Malcolm J. Smith
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

BOOLEAN
YoriDlgInput(
    __in PYORI_WIN_WINDOW_MANAGER_HANDLE WinMgrHandle,
    __in PYORI_STRING Title,
    __inout PYORI_STRING Text
    );

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

// vim:sw=4:ts=4:et:
