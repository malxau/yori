/**
 * @file lib/dyld.c
 *
 * Yori dynamically loaded OS function support for HTTP functions
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

/**
 A structure containing pointers to winhttp.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_WINHTTP_FUNCTIONS DllWinHttp;

/**
 Load pointers to all optional WinHttp.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadWinHttpFunctions(VOID)
{

    if (DllWinHttp.hDll != NULL) {
        return TRUE;
    }

    DllWinHttp.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("WINHTTP.DLL"));
    if (DllWinHttp.hDll == NULL) {
        return FALSE;
    }

    DllWinHttp.pWinHttpCloseHandle = (PWIN_HTTP_CLOSE_HANDLE)GetProcAddress(DllWinHttp.hDll, "WinHttpCloseHandle");
    DllWinHttp.pWinHttpConnect = (PWIN_HTTP_CONNECT)GetProcAddress(DllWinHttp.hDll, "WinHttpConnect");
    DllWinHttp.pWinHttpOpen = (PWIN_HTTP_OPEN)GetProcAddress(DllWinHttp.hDll, "WinHttpOpen");
    DllWinHttp.pWinHttpOpenRequest = (PWIN_HTTP_OPEN_REQUEST)GetProcAddress(DllWinHttp.hDll, "WinHttpOpenRequest");
    DllWinHttp.pWinHttpQueryHeaders = (PWIN_HTTP_QUERY_HEADERS)GetProcAddress(DllWinHttp.hDll, "WinHttpQueryHeaders");
    DllWinHttp.pWinHttpReadData = (PWIN_HTTP_READ_DATA)GetProcAddress(DllWinHttp.hDll, "WinHttpReadData");
    DllWinHttp.pWinHttpReceiveResponse = (PWIN_HTTP_RECEIVE_RESPONSE)GetProcAddress(DllWinHttp.hDll, "WinHttpReceiveResponse");
    DllWinHttp.pWinHttpSendRequest = (PWIN_HTTP_SEND_REQUEST)GetProcAddress(DllWinHttp.hDll, "WinHttpSendRequest");

    return TRUE;
}

/**
 A structure containing pointers to wininet.dll functions that can be used if
 they are found but programs do not have a hard dependency on.
 */
YORI_WININET_FUNCTIONS DllWinInet;

/**
 Load pointers to all optional WinInet.dll functions.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibLoadWinInetFunctions(VOID)
{

    if (DllWinInet.hDll != NULL) {
        return TRUE;
    }

    DllWinInet.hDll = YoriLibLoadLibraryFromSystemDirectory(_T("WININET.DLL"));
    if (DllWinInet.hDll == NULL) {
        return FALSE;
    }

    DllWinInet.pHttpQueryInfoA = (PHTTP_QUERY_INFOA)GetProcAddress(DllWinInet.hDll, "HttpQueryInfoA");
    DllWinInet.pHttpQueryInfoW = (PHTTP_QUERY_INFOW)GetProcAddress(DllWinInet.hDll, "HttpQueryInfoW");
    DllWinInet.pInternetCloseHandle = (PINTERNET_CLOSE_HANDLE)GetProcAddress(DllWinInet.hDll, "InternetCloseHandle");
    DllWinInet.pInternetOpenA = (PINTERNET_OPENA)GetProcAddress(DllWinInet.hDll, "InternetOpenA");
    DllWinInet.pInternetOpenW = (PINTERNET_OPENW)GetProcAddress(DllWinInet.hDll, "InternetOpenW");
    DllWinInet.pInternetOpenUrlA = (PINTERNET_OPEN_URLA)GetProcAddress(DllWinInet.hDll, "InternetOpenUrlA");
    DllWinInet.pInternetOpenUrlW = (PINTERNET_OPEN_URLW)GetProcAddress(DllWinInet.hDll, "InternetOpenUrlW");
    DllWinInet.pInternetReadFile = (PINTERNET_READ_FILE)GetProcAddress(DllWinInet.hDll, "InternetReadFile");

    return TRUE;
}

// vim:sw=4:ts=4:et:
