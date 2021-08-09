/**
 * @file lib/dyld_cab.c
 *
 * Yori dynamically loaded cabinet function support with static fallback
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

#include "yoripch.h"
#include "yorilib.h"

#ifdef YORI_FDI_SUPPORT
/**
 Forward declaration of FDICreate when static linking.
 */
CAB_FDI_CREATE FDICreate;

/**
 Forward declaration of FDICreate when static linking.
 */
CAB_FDI_COPY FDICopy;

/**
 Forward declaration of FDIDestroy when static linking.
 */
CAB_FDI_DESTROY FDIDestroy;
#endif

/**
 A structure containing pointers to cabinet.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_CABINET_FUNCTIONS DllCabinet;

/**
 Load pointers to all optional cabinet.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadCabinetFunctions(VOID)
{
    if (DllCabinet.hDll != NULL) {
        return TRUE;
    }

    DllCabinet.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("CABINET.DLL"));
    if (DllCabinet.hDll == NULL) {
#ifdef YORI_FDI_SUPPORT
        DllCabinet.pFdiCreate = FDICreate;
        DllCabinet.pFdiCopy = FDICopy;
        DllCabinet.pFdiDestroy = FDIDestroy;
#endif
        return FALSE;
    }

    DllCabinet.pFciAddFile = (PCAB_FCI_ADD_FILE)GetProcAddress(DllCabinet.hDll, "FCIAddFile");
    DllCabinet.pFciCreate = (PCAB_FCI_CREATE)GetProcAddress(DllCabinet.hDll, "FCICreate");
    DllCabinet.pFciDestroy = (PCAB_FCI_DESTROY)GetProcAddress(DllCabinet.hDll, "FCIDestroy");
    DllCabinet.pFciFlushCabinet = (PCAB_FCI_FLUSH_CABINET)GetProcAddress(DllCabinet.hDll, "FCIFlushCabinet");
    DllCabinet.pFciFlushFolder = (PCAB_FCI_FLUSH_FOLDER)GetProcAddress(DllCabinet.hDll, "FCIFlushFolder");
    DllCabinet.pFdiCreate = (PCAB_FDI_CREATE)GetProcAddress(DllCabinet.hDll, "FDICreate");
    DllCabinet.pFdiCopy = (PCAB_FDI_COPY)GetProcAddress(DllCabinet.hDll, "FDICopy");
    DllCabinet.pFdiDestroy = (PCAB_FDI_DESTROY)GetProcAddress(DllCabinet.hDll, "FDIDestroy");
    return TRUE;
}

// vim:sw=4:ts=4:et:
