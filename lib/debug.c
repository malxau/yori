/**
 * @file lib/debug.c
 *
 * Yori debugging support routines
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Stop execution and break into the debugger.

 @param Condition A string describing the condition that failed.

 @param Function A string containing the function with the failure.

 @param File A string containing the File with the failure.

 @param Line The line number within the file.
 */
VOID
YoriLibDbgRealAssert(
    __in LPCSTR Condition,
    __in LPCSTR Function,
    __in LPCSTR File,
    __in DWORD Line
    )
{
    DWORD LastError;
    TCHAR szLine[1024];

    LastError = GetLastError();

    YoriLibSPrintf(szLine, _T("ASSERTION FAILURE: %hs\n%hs %hs:%i\nGLE: %i\n\n"), Condition, Function, File, Line, LastError);
    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%s"), szLine);
    OutputDebugString(szLine);

    DebugBreak();
}

// vim:sw=4:ts=4:et:
