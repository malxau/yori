/**
 * @file crt/ep_dll.c
 *
 * Entrypoint code for DLLs.  Currently just a stub.
 *
 * Copyright (c) 2014 Malcolm J. Smith
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

#ifndef WIN32_LEAN_AND_MEAN
/**
 Standard define to include somewhat less of the Windows headers.  Command
 line software doesn't need much.
 */
#define WIN32_LEAN_AND_MEAN 1
#endif

#pragma warning(disable: 4001) /* Single line comment */
#pragma warning(disable: 4127) // conditional expression constant
#pragma warning(disable: 4201) // nameless struct/union
#pragma warning(disable: 4214) // bit field type other than int
#pragma warning(disable: 4514) // unreferenced inline function

#if defined(_MSC_VER) && (_MSC_VER <= 1400)
#pragma warning(disable: 4705) // statement has no effect
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(push)
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(disable: 4668) // preprocessor conditional with nonexistent macro, SDK bug
#pragma warning(disable: 4255) // no function prototype given.  8.1 and earlier SDKs exhibit this.
#pragma warning(disable: 4820) // implicit padding added in structure
#endif

#include <windows.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(pop)
#endif

/**
 Indicate to the standard CRT header that we're compiling the CRT itself.
 */
#define MINICRT_BUILD
#include "yoricrt.h"

/**
 The entrypoint function that the Windows loader will invoke when the DLL is
 attached or detached from a process or thread.
 */
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(lpvReserved);

    return TRUE;
}


// vim:sw=4:ts=4:et:
