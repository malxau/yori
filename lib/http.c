/**
 * @file lib/http.c
 *
 * Yori fallback HTTP support
 *
 * Copyright (c) 2023 Malcolm J. Smith
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
 The type of the handle.  This occurs because the WinInet interface returns an
 HINTERNET for many APIs but it means different things in different contexts.
 */
typedef enum _YORI_LIB_INTERNET_HANDLE_TYPE {
    YoriLibUndefinedHandle = 0,
    YoriLibInternetHandle = 1,
    YoriLibUrlHandle = 2
} YORI_LIB_INTERNET_HANDLE_TYPE;

/**
 Information describing each response header in an HTTP response.
 */
typedef struct _YORI_LIB_HTTP_HEADER_LINE {

    /**
     The list of headers returned from an HTTP request.
     */
    YORI_LIST_ENTRY ListEntry;

    /**
     A string describing the entire line without parsing.
     */
    YORI_STRING EntireLine;

    /**
     The variable component of a response header.
     */
    YORI_STRING Variable;

    /**
     The value component of a response header.
     */
    YORI_STRING Value;

} YORI_LIB_HTTP_HEADER_LINE, *PYORI_LIB_HTTP_HEADER_LINE;

/**
 A nonopaque representation of an HINTERNET handle.
 */
typedef struct _YORI_LIB_INTERNET_HANDLE {

    /**
     The type of the handle.
     */
    YORI_LIB_INTERNET_HANDLE_TYPE HandleType;

    /**
     Different types of data depending on the type of the handle.
     */
    union {
        struct {
            /**
             For handles opened with YoriLibInternetOpen, contains the user
             agent, being the only value supported with YoriLibInternetOpen.
             */
            YORI_STRING UserAgent;
        } Internet;
        struct {

            /**
             For handles opened with YoriLibInternetOpenUrl, points back to
             the handle opened with YoriLibInternetOpen.
             */
            struct _YORI_LIB_INTERNET_HANDLE *InternetHandle;

            /**
             The Url.  This allocation is owned within this module and can be
             updated in response to redirects.
             */
            YORI_STRING Url;

            /**
             Headers supplied by the user.  This allocation is owned by the
             caller since this module never modifies it.
             */
            YORI_STRING UserRequestHeaders;

            /**
             The byte buffer containing the response from the server.
             */
            YORI_LIB_BYTE_BUFFER ByteBuffer;

            /**
             A list of response headers once they have been parsed.
             */
            YORI_LIST_ENTRY HttpResponseHeaders;

            /**
             The current offset within the buffer for InternetRead requests.
             */
            DWORDLONG CurrentReadOffset;

            /**
             The offset within ByteBuffer containing the HTTP payload.
             */
            DWORD HttpBodyOffset;

            /**
             Once the response has been processed, the major version of the
             HTTP response.
             */
            DWORD HttpMajorVersion;

            /**
             Once the response has been processed, the minor version of the
             HTTP response.
             */
            DWORD HttpMinorVersion;

            /**
             Once the response has been processed, the HTTP status code of
             the response.
             */
            DWORD HttpStatusCode;

        } Url;
    } u;
} YORI_LIB_INTERNET_HANDLE, *PYORI_LIB_INTERNET_HANDLE;


/**
 Initializes the use of the WinInet compatibility library.

 @param UserAgent Pointer to a NULL terminated string that specifies the
        value to supply HTTP servers for a user agent.

 @param AccessType Proxies are not supported by this library; must be zero.

 @param ProxyName Proxies are not supported by this library; must be NULL.

 @param ProxyBypass Proxies are not supported by this library; must be NULL.

 @param Flags Flags for the operation.  No flags are supported by this
        library; must be zero.

 @return On successful completion, returns a handle for use in later
         functions.  On failure, returns NULL.  This handle must be closed
         with @ref YoriLibInternetCloseHandle .
 */
LPVOID WINAPI
YoriLibInternetOpen(
    __in LPCTSTR UserAgent,
    __in DWORD AccessType,
    __in_opt LPCTSTR ProxyName,
    __in_opt LPCTSTR ProxyBypass,
    __in DWORD Flags
    )
{
    PYORI_LIB_INTERNET_HANDLE Handle;
    WSADATA WSAData;
    DWORD Length;

    //
    //  This library is super minimal.  Fail if the request is for anything
    //  fancy.
    //

    if (AccessType != 0 ||
        ProxyName != NULL ||
        ProxyBypass != NULL ||
        Flags != 0) {

        return NULL;
    }

    //
    //  This library is implementing HTTP on TCP.  No TCP support, no HTTP support.
    //

    YoriLibLoadWsock32Functions();
    if (DllWsock32.pclosesocket == NULL ||
        DllWsock32.pconnect == NULL ||
        DllWsock32.pgethostbyname == NULL ||
        DllWsock32.precv == NULL ||
        DllWsock32.psend == NULL ||
        DllWsock32.psocket == NULL ||
        DllWsock32.pWSACleanup == NULL ||
        DllWsock32.pWSAStartup == NULL) {

        return NULL;
    }

    if (DllWsock32.pWSAStartup(MAKEWORD(1, 1), &WSAData) != 0) {
        return NULL;
    }

    Handle = YoriLibMalloc(sizeof(YORI_LIB_INTERNET_HANDLE));
    if (Handle == NULL) {
        return NULL;
    }

    ZeroMemory(Handle, sizeof(YORI_LIB_INTERNET_HANDLE));
    Handle->HandleType = YoriLibInternetHandle;

    if (UserAgent != NULL) {
        Length = _tcslen(UserAgent);
        if (!YoriLibAllocateString(&Handle->u.Internet.UserAgent, Length + 1)) {
            YoriLibFree(Handle);
            return NULL;
        }

        memcpy(Handle->u.Internet.UserAgent.StartOfString, UserAgent, Length * sizeof(TCHAR));
        Handle->u.Internet.UserAgent.LengthInChars = Length;
        YoriLibTrimTrailingNewlines(&Handle->u.Internet.UserAgent);
        Handle->u.Internet.UserAgent.StartOfString[Length] = '\0';
    }

    return Handle;
}

/**
 Clean up a Url handle to prepare for reuse.  The same handle can be used for
 multiple requests due to HTTP redirects.

 @param UrlRequest Pointer to the URL handle.
 */
VOID
YoriLibHttpResetUrlRequest(
    __inout PYORI_LIB_INTERNET_HANDLE UrlRequest
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIB_HTTP_HEADER_LINE ResponseLine;

    YoriLibByteBufferReset(&UrlRequest->u.Url.ByteBuffer);

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&UrlRequest->u.Url.HttpResponseHeaders, NULL);
    while (ListEntry != NULL) {
        ResponseLine = CONTAINING_RECORD(ListEntry, YORI_LIB_HTTP_HEADER_LINE, ListEntry);
        YoriLibRemoveListItem(ListEntry);
        YoriLibFreeStringContents(&ResponseLine->EntireLine);
        YoriLibFreeStringContents(&ResponseLine->Variable);
        YoriLibFreeStringContents(&ResponseLine->Value);
        YoriLibDereference(ResponseLine);
        ListEntry = YoriLibGetNextListEntry(&UrlRequest->u.Url.HttpResponseHeaders, NULL);
    }
}

/**
 Close a HINTERNET handle.  Note this can be a Url handle or an Internet handle.

 @param hInternet The HINTERNET handle to close.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL WINAPI
YoriLibInternetCloseHandle(
    __in LPVOID hInternet
    )
{
    PYORI_LIB_INTERNET_HANDLE Handle;

    if (hInternet == NULL) {
        return FALSE;
    }

    Handle = (PYORI_LIB_INTERNET_HANDLE)hInternet;

    ASSERT(Handle->HandleType == YoriLibInternetHandle ||
           Handle->HandleType == YoriLibUrlHandle);

    if (Handle->HandleType != YoriLibInternetHandle &&
        Handle->HandleType != YoriLibUrlHandle) {
        return FALSE;
    }

    if (Handle->HandleType == YoriLibInternetHandle) {
        YoriLibFreeStringContents(&Handle->u.Internet.UserAgent);
        DllWsock32.pWSACleanup();
    } else if (Handle->HandleType == YoriLibUrlHandle) {
        YoriLibHttpResetUrlRequest(Handle);
        YoriLibFreeStringContents(&Handle->u.Url.Url);
        YoriLibInitEmptyString(&Handle->u.Url.UserRequestHeaders);
        YoriLibByteBufferCleanup(&Handle->u.Url.ByteBuffer);
    }

    YoriLibFree(Handle);
    return TRUE;
}

/**
 Find a header by name in the HTTP response.

 @param UrlRequest Pointer to the URL handle containing all parsed response
        headers.

 @param Header The name of the header to find.

 @return If the header is found, pointer to the response header.  If no header
         is found, returns NULL.
 */
PYORI_LIB_HTTP_HEADER_LINE
YoriLibHttpFindResponseHeader(
    __in PYORI_LIB_INTERNET_HANDLE UrlRequest,
    __in LPCTSTR Header
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIB_HTTP_HEADER_LINE ResponseLine;

    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&UrlRequest->u.Url.HttpResponseHeaders, ListEntry);
    while (ListEntry != NULL) {
        ResponseLine = CONTAINING_RECORD(ListEntry, YORI_LIB_HTTP_HEADER_LINE, ListEntry);
        if (YoriLibCompareStringWithLiteralInsensitive(&ResponseLine->Variable, Header) == 0) {
            return ResponseLine;
        }
        ListEntry = YoriLibGetNextListEntry(&UrlRequest->u.Url.HttpResponseHeaders, ListEntry);
    }

    return NULL;
}


/**
 Once the HTTP response payload has been received, parse the response into
 a series of headers, and parse the status line into major/minor versions and
 HTTP status code.  If the status code is a redirect code, construct a new
 redirect URL and prepare the request to be reissued to a new URL.

 @param UrlRequest Pointer to the request containing the received payload.

 @param RedirectUrl On successful completion, updated to contain a new URL if
        the request should be redirected.  Will be left as an empty string if
        no redirection should be performed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibHttpProcessResponseHeaders(
    __in PYORI_LIB_INTERNET_HANDLE UrlRequest,
    __inout PYORI_STRING RedirectUrl
    )
{
    UCHAR * AnsiBuffer;
    UCHAR * LineStart;
    DWORD Index;
    DWORD CharIndex;
    DWORD LineLengthInChars;
    DWORD CharsConsumed;
    LONGLONG llTemp;
    PYORI_LIB_HTTP_HEADER_LINE ResponseLine;
    PYORI_LIST_ENTRY ListEntry;
    YORI_STRING Str;
    LPTSTR Colon;

    UNREFERENCED_PARAMETER(RedirectUrl);

    Index = 0;
    AnsiBuffer = UrlRequest->u.Url.ByteBuffer.Buffer;
    LineStart = AnsiBuffer;
    LineLengthInChars = 0;

    //
    //  MSFIX: Should impose some fairly arbitrary limit on the maximum
    //  size of headers.  This is because we don't want to limit the size
    //  of the HTTP payload needlessly, but still want to be robust to
    //  malicious input trying to cause this to overflow or degrade.
    //

    for (Index = 0; Index < UrlRequest->u.Url.ByteBuffer.BytesPopulated; Index++) {
        if (AnsiBuffer[Index] == '\r' || AnsiBuffer[Index] == '\n') {

            //
            //  Advance beyond one line break, but only one line break.  If
            //  there's \r\n, advance two chars; if it's \r or \n, advance by
            //  one.  One of these will be done automatically at the next
            //  pass through the loop.
            //

            if (AnsiBuffer[Index] == '\r' &&
                Index + 1 < UrlRequest->u.Url.ByteBuffer.BytesPopulated &&
                AnsiBuffer[Index + 1] == '\n') {

                Index = Index + 1;
            }

            //
            //  An empty line indicates no more headers and the start of
            //  payload.  Note that we just swalled the newline, so payload
            //  starts at the current Index offset.
            //

            if (LineLengthInChars == 0) {
                Index++;
                break;
            }

            ResponseLine = YoriLibReferencedMalloc(sizeof(YORI_LIB_HTTP_HEADER_LINE) + (LineLengthInChars + 1) * sizeof(TCHAR));
            if (ResponseLine == NULL) {
                return FALSE;
            }
            ZeroMemory(ResponseLine, sizeof(YORI_LIB_HTTP_HEADER_LINE));

            ResponseLine->EntireLine.StartOfString = (LPTSTR)(ResponseLine + 1);
            ResponseLine->EntireLine.MemoryToFree = ResponseLine;
            YoriLibReference(ResponseLine);

            for (CharIndex = 0; CharIndex < LineLengthInChars; CharIndex++) {
                // MSFIX: Filter characters here?
                ResponseLine->EntireLine.StartOfString[CharIndex] = LineStart[CharIndex];
            }
            ResponseLine->EntireLine.StartOfString[LineLengthInChars] = '\0';
            ResponseLine->EntireLine.LengthInChars = LineLengthInChars;

            ResponseLine->Variable.StartOfString = ResponseLine->EntireLine.StartOfString;
            ResponseLine->Variable.MemoryToFree = ResponseLine;
            YoriLibReference(ResponseLine);
            ResponseLine->Variable.LengthInChars = LineLengthInChars;

            Colon = YoriLibFindLeftMostCharacter(&ResponseLine->EntireLine, ':');

            if (Colon != NULL) {
                ResponseLine->Variable.LengthInChars = (DWORD)(Colon - ResponseLine->Variable.StartOfString);
                YoriLibTrimSpaces(&ResponseLine->Variable);

                ResponseLine->Value.StartOfString = Colon + 1;
                ResponseLine->Value.LengthInChars = ResponseLine->EntireLine.LengthInChars - ResponseLine->Variable.LengthInChars - 1;
                ResponseLine->Value.MemoryToFree = ResponseLine;
                YoriLibReference(ResponseLine);
                YoriLibTrimSpaces(&ResponseLine->Value);
            }

            YoriLibAppendList(&UrlRequest->u.Url.HttpResponseHeaders, &ResponseLine->ListEntry);

            LineStart = &AnsiBuffer[Index + 1];
            LineLengthInChars = 0;
        } else {
            LineLengthInChars++;
        }
    }

    UrlRequest->u.Url.HttpBodyOffset = Index;

    ASSERT(Index <= UrlRequest->u.Url.ByteBuffer.BytesPopulated);

    ASSERT(Index >= UrlRequest->u.Url.ByteBuffer.BytesPopulated ||
           (UrlRequest->u.Url.ByteBuffer.Buffer[Index] != '\n' &&
            UrlRequest->u.Url.ByteBuffer.Buffer[Index] != '\r'));
    UrlRequest->u.Url.CurrentReadOffset = 0;

    ListEntry = YoriLibGetNextListEntry(&UrlRequest->u.Url.HttpResponseHeaders, NULL);
    if (ListEntry == NULL) {
        return FALSE;
    }

    //
    //  An HTTP status response should be
    //
    //  HTTP/1.0 200 Ok
    //

    ResponseLine = CONTAINING_RECORD(ListEntry, YORI_LIB_HTTP_HEADER_LINE, ListEntry);
    if (YoriLibCompareStringWithLiteralCount(&ResponseLine->EntireLine, _T("HTTP/"), sizeof("HTTP/") - 1) != 0) {
        return FALSE;
    }

    YoriLibInitEmptyString(&Str);
    Str.StartOfString = &ResponseLine->EntireLine.StartOfString[sizeof("HTTP/") - 1];
    Str.LengthInChars = ResponseLine->EntireLine.LengthInChars - sizeof("HTTP/") + 1;

    if (!YoriLibStringToNumberSpecifyBase(&Str, 10, FALSE, &llTemp, &CharsConsumed)) {
        return FALSE;
    }

    if (CharsConsumed == 0 || llTemp != 1) {
        return FALSE;
    }

    UrlRequest->u.Url.HttpMajorVersion = (DWORD)llTemp;

    Str.StartOfString = Str.StartOfString + CharsConsumed;
    Str.LengthInChars = Str.LengthInChars - CharsConsumed;

    if (Str.LengthInChars == 0 || Str.StartOfString[0] != '.') {
        return FALSE;
    }

    Str.StartOfString = Str.StartOfString + 1;
    Str.LengthInChars = Str.LengthInChars - 1;

    if (!YoriLibStringToNumberSpecifyBase(&Str, 10, FALSE, &llTemp, &CharsConsumed)) {
        return FALSE;
    }

    if (CharsConsumed == 0 || (llTemp != 0 && llTemp != 1)) {
        return FALSE;
    }

    UrlRequest->u.Url.HttpMinorVersion = (DWORD)llTemp;

    Str.StartOfString = Str.StartOfString + CharsConsumed;
    Str.LengthInChars = Str.LengthInChars - CharsConsumed;

    YoriLibTrimSpaces(&Str);

    if (Str.LengthInChars == 0) {
        return FALSE;
    }

    if (!YoriLibStringToNumberSpecifyBase(&Str, 10, FALSE, &llTemp, &CharsConsumed)) {
        return FALSE;
    }

    UrlRequest->u.Url.HttpStatusCode = (DWORD)llTemp;

    if (UrlRequest->u.Url.HttpStatusCode == 301 ||
        UrlRequest->u.Url.HttpStatusCode == 302) {

        ResponseLine = YoriLibHttpFindResponseHeader(UrlRequest, _T("Location"));
        if (ResponseLine != NULL) {
            YoriLibCloneString(RedirectUrl, &ResponseLine->Value);
        }
    }

    return TRUE;
}

/**
 Connect to a specified URL, download contents, parse headers, and redirect as
 necessary.

 @param UrlRequest Pointer to a URL handle containing a URL to connect to.

 @param RedirectUrl On successful completion, updated to contain a new URL if
        the request should be redirected.  Will be left as an empty string if
        no redirection should be performed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibHttpProcessUrlRequest(
    __in PYORI_LIB_INTERNET_HANDLE UrlRequest,
    __inout PYORI_STRING RedirectUrl
    )
{
    YORI_STRING HostSubset;
    YORI_STRING Request;
    LPCTSTR EndOfHost;
    struct hostent * addr;
    struct sockaddr_in sin;
    UCHAR * AnsiBuffer;
    SOCKET s;
    DWORD Length;
    DWORD TotalLength;
    DWORDLONG BytesRemaining;

    //
    //  MSFIX Implement this
    //

    YoriLibInitEmptyString(RedirectUrl);

    //
    //  Currently this code only speaks http.  Note there is no TLS support
    //  here.
    //

    if (YoriLibCompareStringWithLiteralInsensitiveCount(&UrlRequest->u.Url.Url, _T("http://"), sizeof("http://") - 1) != 0) {
        return FALSE;
    }

    YoriLibInitEmptyString(&HostSubset);
    HostSubset.StartOfString = &UrlRequest->u.Url.Url.StartOfString[sizeof("http://") - 1];
    HostSubset.LengthInChars = UrlRequest->u.Url.Url.LengthInChars - sizeof("http://") + 1;
    EndOfHost = YoriLibFindLeftMostCharacter(&HostSubset, '/');
    if (EndOfHost != NULL) {
        HostSubset.LengthInChars = (DWORD)(EndOfHost - HostSubset.StartOfString);
    } else {
        return FALSE;
    }

    YoriLibInitEmptyString(&Request);
    YoriLibYPrintf(&Request,
                   _T("GET %s HTTP/1.0\r\n%y\r\nUser-Agent: %y(YoriWinInet %i.%02i)\r\n\r\n"),
                   EndOfHost,
                   &UrlRequest->u.Url.UserRequestHeaders,
                   &UrlRequest->u.Url.InternetHandle->u.Internet.UserAgent,
                   YORI_VER_MAJOR, YORI_VER_MINOR);

    if (Request.StartOfString == NULL) {
        return FALSE;
    }

    AnsiBuffer = YoriLibMalloc(HostSubset.LengthInChars + 1);
    if (AnsiBuffer == NULL) {
        YoriLibFreeStringContents(&Request);
        return FALSE;
    }

    YoriLibSPrintfA(AnsiBuffer, "%y", &HostSubset);
    addr = DllWsock32.pgethostbyname(AnsiBuffer);
    if (addr == NULL || addr->h_addrtype != AF_INET || addr->h_length != sizeof(DWORD)) {
        YoriLibFree(AnsiBuffer);
        YoriLibFreeStringContents(&Request);
        return FALSE;
    }

    YoriLibFree(AnsiBuffer);

    s = DllWsock32.psocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        YoriLibFreeStringContents(&Request);
        return FALSE;
    }

    ZeroMemory(&sin, sizeof(sin));

    // MSFIX Probably should parse the port from the host name
    sin.sin_family = AF_INET;
    sin.sin_port = 0x5000; // 80, in hex, in big endian
    memcpy(&sin.sin_addr.s_addr, addr->h_addr, addr->h_length);

    if (DllWsock32.pconnect(s, &sin, sizeof(sin)) != 0) {
        YoriLibFreeStringContents(&Request);
        DllWsock32.pclosesocket(s);
        return FALSE;
    }

    AnsiBuffer = YoriLibMalloc(Request.LengthInChars + 1);
    if (AnsiBuffer == NULL) {
        YoriLibFreeStringContents(&Request);
        DllWsock32.pclosesocket(s);
        return FALSE;
    }

    YoriLibSPrintfA(AnsiBuffer, "%y", &Request);
    if (DllWsock32.psend(s, AnsiBuffer, Request.LengthInChars, 0) != (int)Request.LengthInChars) {
        YoriLibFree(AnsiBuffer);
        YoriLibFreeStringContents(&Request);
        DllWsock32.pclosesocket(s);
        return FALSE;
    }

    YoriLibFreeStringContents(&Request);
    YoriLibFree(AnsiBuffer);

    YoriLibByteBufferReset(&UrlRequest->u.Url.ByteBuffer);
    TotalLength = 0;
    while(TRUE) {
        AnsiBuffer = YoriLibByteBufferGetPointerToEnd(&UrlRequest->u.Url.ByteBuffer, 256 * 1024, &BytesRemaining);
        Length = DllWsock32.precv(s, AnsiBuffer, (DWORD)BytesRemaining, 0);
        if (Length == 0) {
            break;
        }
        YoriLibByteBufferAddToPopulatedLength(&UrlRequest->u.Url.ByteBuffer, Length);
    }

    DllWsock32.pclosesocket(s);

    if (!YoriLibHttpProcessResponseHeaders(UrlRequest, RedirectUrl)) {
        return FALSE;
    }

    // YoriLibHttpOutputUrlResponse(UrlRequest);

    return TRUE;
}

/**
 Munge an original URL and a Location redirect header into a fully specified
 new URL.

 @param OriginalUrl The original URL that returned a redirect.

 @param LocationHeader The redirection URL returned from the original URL.

 @param RedirectUrl On successful completion, updated to contain a new URL
        which is a fully specified location, removing any /../ components.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriLibHttpMergeRedirectUrl(
    __in PYORI_STRING OriginalUrl,
    __in PYORI_STRING LocationHeader,
    __out PYORI_STRING RedirectUrl
    )
{
    YORI_STRING CombinedString;
    DWORD EffectiveRoot;
    DWORD CurrentReadIndex;
    DWORD CurrentWriteIndex;
    BOOLEAN PreviousWasSeperator;

    //
    //  There are three types of URLs to consider:
    //  - Absolute URL, where the location header is the complete URL
    //  - Host relative URL, starting in / - append the LocationHeader to the
    //    host part of the OriginalUrl
    //  - URL purely relative to the original URL
    //

    if (YoriLibCompareStringWithLiteralInsensitiveCount(LocationHeader, _T("http://"), sizeof("http://") - 1) == 0) {
        YoriLibCloneString(RedirectUrl, LocationHeader);
        return TRUE;
    }

    YoriLibInitEmptyString(&CombinedString);

    //
    //  MSFIX Support host relative URLs.  Should be simple but not currently
    //  used.
    //

    if (LocationHeader->LengthInChars > 0 &&
        LocationHeader->StartOfString[0] == '/') {

        return FALSE;
    } else {

        if (!YoriLibAllocateString(&CombinedString, OriginalUrl->LengthInChars + LocationHeader->LengthInChars + 1)) {
            return FALSE;
        }

        memcpy(CombinedString.StartOfString, OriginalUrl->StartOfString, OriginalUrl->LengthInChars * sizeof(TCHAR));
        CombinedString.LengthInChars = OriginalUrl->LengthInChars;
        memcpy(&CombinedString.StartOfString[OriginalUrl->LengthInChars], LocationHeader->StartOfString, LocationHeader->LengthInChars * sizeof(TCHAR));
        CombinedString.LengthInChars = OriginalUrl->LengthInChars + LocationHeader->LengthInChars;
    }

    //
    //  Check if we have a fully qualified URL.  If not, this program doesn't
    //  know what to do next.
    //

    if (YoriLibCompareStringWithLiteralInsensitiveCount(&CombinedString, _T("http://"), sizeof("http://") - 1) != 0) {
        YoriLibFreeStringContents(&CombinedString);
        return FALSE;
    }

    EffectiveRoot = sizeof("http://") - 1;
    PreviousWasSeperator = FALSE;

    for (; EffectiveRoot < CombinedString.LengthInChars; EffectiveRoot++) {
        if (CombinedString.StartOfString[EffectiveRoot] == '/') {
            PreviousWasSeperator = TRUE;
            break;
        }
        if (CombinedString.StartOfString[EffectiveRoot] == '?') {
            break;
        }
    }

    CurrentWriteIndex = EffectiveRoot;
    for (CurrentReadIndex = EffectiveRoot; CurrentReadIndex < CombinedString.LengthInChars; CurrentReadIndex++) {

        //
        //  If the component is /../, back up, but don't continue beyond
        //  EffectiveRoot
        //

        if (PreviousWasSeperator && 
            CurrentReadIndex + 2 < CombinedString.LengthInChars &&
            CombinedString.StartOfString[CurrentReadIndex] == '.' &&
            CombinedString.StartOfString[CurrentReadIndex + 1] == '.' &&
            CombinedString.StartOfString[CurrentReadIndex + 2] == '/') {

            PreviousWasSeperator = FALSE;

            //
            //  Walk back one component or until the root
            //

            CurrentWriteIndex--;
            while (CurrentWriteIndex > EffectiveRoot) {

                CurrentWriteIndex--;

                if (CombinedString.StartOfString[CurrentWriteIndex] == '/') {
                    break;
                }
            }

            //
            //  If we were already on effective root when entering
            //  this block, we backed up one char too many already
            //

            if (CurrentWriteIndex < EffectiveRoot) {
                CurrentWriteIndex = EffectiveRoot;
            }

            //
            //  If we get to the root, check if the previous
            //  char was a seperator.  Messy because UNC and
            //  drive letters resolve this differently
            //

            if (CurrentWriteIndex == EffectiveRoot) {
                CurrentWriteIndex--;
                if (CombinedString.StartOfString[CurrentWriteIndex] == '/') {
                    PreviousWasSeperator = TRUE;
                }
                CurrentWriteIndex++;
            }

            CurrentReadIndex++;
            continue;
        }

        //
        //  Note if this is a seperator or not.  If it's the final char and
        //  a seperator, drop it
        //

        if (CombinedString.StartOfString[CurrentReadIndex] == '/') {
            PreviousWasSeperator = TRUE;
        } else {
            PreviousWasSeperator = FALSE;
        }

        CombinedString.StartOfString[CurrentWriteIndex] = CombinedString.StartOfString[CurrentReadIndex];
        CurrentWriteIndex++;
    }

    CombinedString.StartOfString[CurrentWriteIndex] = '\0';
    CombinedString.LengthInChars = CurrentWriteIndex;

    memcpy(RedirectUrl, &CombinedString, sizeof(YORI_STRING));
    return TRUE;
}

/**
 Opens a specified URL resource.  This includes downloading all contents.

 @param hInternet Handle to an internet resource opened with
        @ref YoriLibInternetOpen .

 @param Url Specifies a NULL terminated URL to access.  This library only
        supports non-SSL HTTP connections.

 @param Headers Specifies any additional HTTP headers that should be supplied.

 @param HeadersLength The length of Headers, in characters.  If -1, Headers
        is assumed to be a NULL terminated string.

 @param Flags Not supported by this library; must be zero.

 @param Context Not supported by this library; must be zero.

 @return On successful completion, returns a handle to the URL resource.
         On failure, returns NULL.  The handle must be closed with
         @ref YoriLibInternetCloseHandle .
 */
LPVOID WINAPI
YoriLibInternetOpenUrl(
    __in LPVOID hInternet,
    __in LPCTSTR Url,
    __in LPCTSTR Headers,
    __in DWORD HeadersLength,
    __in DWORD Flags,
    __in DWORD_PTR Context
    )
{
    PYORI_LIB_INTERNET_HANDLE UrlHandle;
    PYORI_LIB_INTERNET_HANDLE Handle;
    YORI_STRING RedirectUrl;
    YORI_STRING LocationHeader;
    DWORD Length;

    if (hInternet == NULL || Flags != 0 || Context != 0) {
        return NULL;
    }
    Handle = (PYORI_LIB_INTERNET_HANDLE)hInternet;
    if (Handle->HandleType != YoriLibInternetHandle) {
        return NULL;
    }

    UrlHandle = YoriLibMalloc(sizeof(YORI_LIB_INTERNET_HANDLE));
    if (UrlHandle == NULL) {
        return NULL;
    }

    ZeroMemory(UrlHandle, sizeof(YORI_LIB_INTERNET_HANDLE));
    UrlHandle->HandleType = YoriLibUrlHandle;
    YoriLibInitializeListHead(&UrlHandle->u.Url.HttpResponseHeaders);

    Length = _tcslen(Url);
    if (!YoriLibAllocateString(&UrlHandle->u.Url.Url, Length + 1)) {
        YoriLibInternetCloseHandle(UrlHandle);
        return NULL;
    }

    memcpy(UrlHandle->u.Url.Url.StartOfString, Url, Length * sizeof(TCHAR));
    UrlHandle->u.Url.Url.LengthInChars = Length;
    UrlHandle->u.Url.Url.StartOfString[Length] = '\0';

    UrlHandle->u.Url.UserRequestHeaders.StartOfString = (LPTSTR)Headers;
    if (HeadersLength == (DWORD)-1 && Headers != NULL) {
        UrlHandle->u.Url.UserRequestHeaders.LengthInChars = _tcslen(Headers);
    } else {
        UrlHandle->u.Url.UserRequestHeaders.LengthInChars = HeadersLength;
    }

    YoriLibTrimTrailingNewlines(&UrlHandle->u.Url.UserRequestHeaders);
    UrlHandle->u.Url.InternetHandle = Handle;

    if (!YoriLibByteBufferInitialize(&UrlHandle->u.Url.ByteBuffer, 1024 * 1024)) {
        YoriLibInternetCloseHandle(UrlHandle);
        return NULL;
    }

    while (TRUE) {
        YoriLibHttpResetUrlRequest(UrlHandle);
        YoriLibInitEmptyString(&LocationHeader);
        if (!YoriLibHttpProcessUrlRequest(UrlHandle, &LocationHeader)) {
            YoriLibInternetCloseHandle(UrlHandle);
            return NULL;
        }

        if (LocationHeader.StartOfString == NULL) {
            break;
        }

        YoriLibInitEmptyString(&RedirectUrl);
        if (!YoriLibHttpMergeRedirectUrl(&UrlHandle->u.Url.Url, &LocationHeader, &RedirectUrl)) {
            YoriLibFreeStringContents(&LocationHeader);
            YoriLibInternetCloseHandle(UrlHandle);
            return NULL;
        }

        YoriLibFreeStringContents(&LocationHeader);
        YoriLibFreeStringContents(&UrlHandle->u.Url.Url);
        YoriLibCloneString(&UrlHandle->u.Url.Url, &RedirectUrl);
        YoriLibFreeStringContents(&RedirectUrl);
    }

    return UrlHandle;
}


/**
 Reads data from a successful handle opened via @ref YoriLibInternetOpenUrl .

 @param hRequest A handle returned from @ref YoriLibInternetOpenUrl .

 @param Buffer On successful completion, updated to contain data read from the
        handle.

 @param BytesToRead The maximum number of bytes that can be read into Buffer.

 @param BytesRead On successful completion, updated to contain the number of
        bytes successfully read into Buffer.  This may be less than
        BytesToRead .

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL WINAPI
YoriLibInternetReadFile(
    __in LPVOID hRequest,
    __out_bcount(BytesToRead) LPVOID Buffer,
    __in DWORD BytesToRead,
    __out PDWORD BytesRead
    )
{
    PYORI_LIB_INTERNET_HANDLE UrlHandle;
    DWORDLONG BufferOffset;
    DWORD BytesToCopy;
    PUCHAR Source;

    UrlHandle = (PYORI_LIB_INTERNET_HANDLE)hRequest;

    if (UrlHandle->HandleType != YoriLibUrlHandle) {
        return FALSE;
    }

    BufferOffset = UrlHandle->u.Url.HttpBodyOffset;
    BufferOffset = BufferOffset + UrlHandle->u.Url.CurrentReadOffset;

    BytesToCopy = BytesToRead;
    if (BufferOffset + BytesToCopy > UrlHandle->u.Url.ByteBuffer.BytesPopulated) {
        BytesToCopy = (DWORD)(UrlHandle->u.Url.ByteBuffer.BytesPopulated - BufferOffset);
    }

    if (BytesToCopy == 0) {
        *BytesRead = 0;
        return TRUE;
    }

    Source = YoriLibAddToPointer(UrlHandle->u.Url.ByteBuffer.Buffer, BufferOffset);
    memcpy(Buffer, Source, BytesToCopy);
    *BytesRead = BytesToCopy;

    UrlHandle->u.Url.CurrentReadOffset = UrlHandle->u.Url.CurrentReadOffset + BytesToCopy;
    return TRUE;
}

/**
 Query information associated with an HTTP request.

 @param hRequest A handle returned from @ref YoriLibInternetOpenUrl .

 @param InfoLevel The type of information requested.

 @param Buffer On successful completion, populated with the requested
        information.

 @param BufferLength Specifies the length of Buffer, in bytes.  On successful
        completion, updated to indicate the number of bytes copied into
        Buffer.

 @param Index Pointer to a zero based header index for repeated queries.  Not
        supported by this library.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
__success(return)
BOOL WINAPI
YoriLibHttpQueryInfo(
    __in LPVOID hRequest,
    __in DWORD InfoLevel,
    __out LPVOID Buffer,
    __inout PDWORD BufferLength,
    __inout PDWORD Index
    )
{
    DWORD InfoLevelModifier;
    DWORD InfoLevelIndex;
    PYORI_LIB_INTERNET_HANDLE UrlHandle;
    PDWORD OutputNumber;

    UrlHandle = (PYORI_LIB_INTERNET_HANDLE)hRequest;

    if (UrlHandle->HandleType != YoriLibUrlHandle) {
        return FALSE;
    }

    InfoLevelModifier = (InfoLevel & 0xF0000000);
    InfoLevelIndex = (InfoLevel & 0x0000FFFF);

    if (InfoLevelModifier != HTTP_QUERY_FLAG_NUMBER ||
        InfoLevelIndex != HTTP_QUERY_STATUS_CODE) {

        return FALSE;
    }

    if (Buffer == NULL ||
        BufferLength == NULL ||
        (*BufferLength) < sizeof(DWORD)) {

        return FALSE;
    }

    if (Index != NULL) {
        *Index = 0;
    }

    OutputNumber = (PDWORD)Buffer;
    *OutputNumber = UrlHandle->u.Url.HttpStatusCode;
    *BufferLength = sizeof(DWORD);

    return TRUE;
}

// vim:sw=4:ts=4:et:
