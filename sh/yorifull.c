/**
 * @file sh/yorifull.c
 *
 * Yori table of supported builtins for the full/regular build of Yori
 *
 * Copyright (c) 2017 Malcolm J. Smith
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

#include "yori.h"

/**
 The list of builtin commands supported by this build of Yori.
 */
CONST
LPTSTR YoriShBuiltins = _T("ALIAS\0")
                        _T("BUILTIN\0")
                        _T("CHDIR\0")
                        _T("COLOR\0")
                        _T("EXIT\0")
                        _T("FALSE\0")
                        _T("FG\0")
                        _T("IF\0")
                        _T("JOB\0")
                        _T("PUSHD\0")
                        _T("REM\0")
                        _T("SET\0")
                        _T("SETLOCAL\0")
                        _T("TRUE\0")
                        _T("VER\0")
                        _T("WAIT\0")
                        _T("YS\0")
                        _T("\0");

// vim:sw=4:ts=4:et:
