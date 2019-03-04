/**
 * @file lib/update.c
 *
 * Code to update a file from the internet including the running
 * executable.
 *
 * Copyright (c) 2016 Malcolm J. Smith
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
 A table of constant error strings corresponding to an error number.
 */
LPCTSTR
YoriLibUpdErrorStrings[] = {
    _T("Success"),
    _T("Could not initialize WinInet"),
    _T("Could not connect to server"),
    _T("Could not read data from server"),
    _T("Data read from server is incorrect"),
    _T("Could not write data to local file"),
    _T("Could not replace existing file with new file")
};

/**
 Update an existing local file from a new local file.

 @param ExistingPath Path to the existing file.  If NULL, this means the
        currently running executable should be updated.

 @param NewPath Path to the file that should replace ExistingFile.

 @return TRUE to indicate success, FALSE to indicate error.
 */
BOOL
YoriLibUpdateBinaryFromFile(
    __in_opt LPTSTR ExistingPath,
    __in LPTSTR NewPath
    )
{
    TCHAR MyPath[MAX_PATH];
    TCHAR OldPath[MAX_PATH];
    LPTSTR PathToReplace;
    HANDLE hMyBinary;

    if (ExistingPath == NULL) {

        //
        //  If the file name to replace is NULL, replace the currently
        //  existing binary.
        //
    
        if (GetModuleFileName(NULL, MyPath, sizeof(MyPath)/sizeof(MyPath[0])) == 0) {
            return FALSE;
        }

        PathToReplace = MyPath;
    } else {
        LPTSTR FinalBackslash;

        //
        //  If the file name to replace is a full path, defined as containing
        //  a backslash, replace that file path.
        //

        FinalBackslash = _tcsrchr(ExistingPath, '\\');

        if (FinalBackslash != NULL) {
            PathToReplace = ExistingPath;
        } else {

            //
            //  If it's a file name only, assume that it refers to a file in
            //  the same path as the existing binary.
            //

            if (GetModuleFileName(NULL, MyPath, sizeof(MyPath)/sizeof(MyPath[0])) == 0) {
                return FALSE;
            }

            FinalBackslash = _tcsrchr(MyPath, '\\');
            if (FinalBackslash != NULL) {
                DWORD RemainingLength = (DWORD)(sizeof(MyPath)/sizeof(MyPath[0]) - (FinalBackslash - MyPath + 1));
                if ((DWORD)_tcslen(ExistingPath) >= RemainingLength) {
                    return FALSE;
                }
                YoriLibSPrintfS(FinalBackslash + 1, RemainingLength, _T("%s"), ExistingPath);
                PathToReplace = MyPath;
            } else {
                PathToReplace = ExistingPath;
            }
        }
    }

    //
    //  Create a temporary name to hold the existing binary.
    //

    if (_tcslen(PathToReplace) + 4 >= sizeof(OldPath) / sizeof(OldPath[0])) {
        return FALSE;
    }
    YoriLibSPrintfS(OldPath, sizeof(OldPath)/sizeof(OldPath[0]), _T("%s.old"), PathToReplace);

    //
    //  Rename the existing binary to temp.  If this process has
    //  been performed before and is incomplete the temp may exist,
    //  but we can just clobber it with the current version.
    //

    if (!MoveFileEx(PathToReplace, OldPath, MOVEFILE_REPLACE_EXISTING)) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            return FALSE;
        }
    }

    //
    //  Rename the new binary to where the old binary was.
    //  If it fails, try to move the old binary back.  If
    //  that fails, there's not much we can do.
    //

    if (!MoveFileEx(NewPath, PathToReplace, MOVEFILE_COPY_ALLOWED)) {
        MoveFile(OldPath, PathToReplace);
        return FALSE;
    }

    //
    //  Try to delete the old binary.  Do this by opening a delete
    //  on close handle and not closing it.  The close will occur
    //  when the process terminates, which hopefully means it won't
    //  conflict with the open that's running the program right now.
    //
    //  If this fails just leave the old binary around.  Note next
    //  time this process is run it is overwritten.
    //

    hMyBinary = CreateFile(OldPath,
                           DELETE,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_DELETE_ON_CLOSE,
                           NULL);


    return TRUE;
}

/**
 A prototype for InternetOpenW as provided by WinInet.
 */
typedef
LPVOID
WINAPI
INTERNET_OPEN(
    LPCTSTR szAgent,
    DWORD dwAccessType,
    LPCSTR szProxyName,
    LPCSTR szProxyBypass,
    DWORD dwFlags);

/**
 A pointer to InternetOpenW as provided by WinInet.
 */
typedef INTERNET_OPEN *PINTERNET_OPEN;

/**
 A prototype for InternetOpenUrlW as provided by WinInet.
 */
typedef
LPVOID
WINAPI
INTERNET_OPEN_URL(
    LPVOID hInternet,
    LPCTSTR szUrl,
    LPCTSTR szHeaders,
    DWORD dwHeaderLength,
    DWORD dwFlags,
    DWORD dwContext);

/**
 A pointer to InternetQueryInfoW as provided by WinInet.
 */
typedef INTERNET_OPEN_URL *PINTERNET_OPEN_URL;

/**
 A prototype for InternetQueryInfoW as provided by WinInet.
 */
typedef
BOOL
WINAPI
HTTP_QUERY_INFO(
    LPVOID hInternet,
    DWORD dwLevel,
    LPVOID Buffer,
    LPDWORD BufferLength,
    LPDWORD Index
    );

/**
 A pointer to InternetQueryInfoW as provided by WinInet.
 */
typedef HTTP_QUERY_INFO *PHTTP_QUERY_INFO;

/**
 A prototype for InternetReadFile as provided by WinInet.
 */
typedef
BOOL
WINAPI
INTERNET_READ_FILE(
    LPVOID hInternet,
    LPVOID pBuffer,
    DWORD BytesToRead,
    LPDWORD BytesRead
    );

/**
 A pointer to InternetReadFile as provided by WinInet.
 */
typedef INTERNET_READ_FILE *PINTERNET_READ_FILE;

/**
 A prototype for InternetCloseHandle as provided by WinInet.
 */
typedef
BOOL
WINAPI
INTERNET_CLOSE_HANDLE(
    LPVOID hInternet
    );

/**
 A pointer to InternetCloseHandle as provided by WinInet.
 */
typedef INTERNET_CLOSE_HANDLE *PINTERNET_CLOSE_HANDLE;

/**
 An array of human readable day names in HTTP format.
 */
LPCSTR YoriLibDayNames[] = {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"};

/**
 An array of human readable month names in HTTP format.
 */
LPCSTR YoriLibMonthNames[] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"};

/**
 The size of a single read buffer.  This will be given to WInInet as a single
 operation.  We can read more bytes than this, it will just be done in multiple
 operations to WinInet.
 */
#define UPDATE_READ_SIZE (1024 * 1024)

/**
 Download a file from the internet and store it in a local location.

 @param Url The Url to download the file from.

 @param TargetName If specified, the local location to store the file.
        If not specified, the current executable name is used.

 @param Agent The user agent to report to the remote web server.

 @param IfModifiedSince If specified, indicates a timestamp where a new
        object should only be downloaded if it is newer.

 @return An update error code indicating success or appropriate error.
 */
YoriLibUpdError
YoriLibUpdateBinaryFromUrl(
    __in LPTSTR Url,
    __in_opt LPTSTR TargetName,
    __in LPTSTR Agent,
    __in_opt PSYSTEMTIME IfModifiedSince
    )
{
    PVOID hInternet = NULL;
    PVOID NewBinary = NULL;
    PUCHAR NewBinaryData = NULL;
    DWORD ErrorBufferSize = 0;
    DWORD ActualBinarySize;
    TCHAR TempName[MAX_PATH];
    TCHAR TempPath[MAX_PATH];
    HANDLE hTempFile = INVALID_HANDLE_VALUE;
    BOOL SuccessfullyComplete = FALSE;
    HINSTANCE hWinInet = NULL;
    DWORD dwError;
    YoriLibUpdError Return = YoriLibUpdErrorSuccess;
    LPTSTR StartOfHost;
    LPTSTR EndOfHost;
    YORI_STRING HostHeader;
    YORI_STRING IfModifiedSinceHeader;
    YORI_STRING CombinedHeader;

    PINTERNET_OPEN InetOpen;
    PINTERNET_OPEN_URL InetOpenUrl;
    PINTERNET_READ_FILE InetReadFile;
    PINTERNET_CLOSE_HANDLE InetCloseHandle;
    PHTTP_QUERY_INFO HttpQueryInfo;

    //
    //  Dynamically load WinInet.  This means we don't have to resolve
    //  imports unless we're really using it for something, and we can
    //  degrade gracefully if it's not there (original 95/NT.)
    //

    hWinInet = LoadLibrary(_T("WinInet.dll"));
    if (hWinInet == NULL) {
        Return = YoriLibUpdErrorInetInit;
        return FALSE;
    }

#ifdef _UNICODE
    InetOpen = (PINTERNET_OPEN)GetProcAddress(hWinInet, "InternetOpenW");
    InetOpenUrl = (PINTERNET_OPEN_URL)GetProcAddress(hWinInet, "InternetOpenUrlW");
    HttpQueryInfo = (PHTTP_QUERY_INFO)GetProcAddress(hWinInet, "HttpQueryInfoW");
#else
    InetOpen = (PINTERNET_OPEN)GetProcAddress(hWinInet, "InternetOpenA");
    InetOpenUrl = (PINTERNET_OPEN_URL)GetProcAddress(hWinInet, "InternetOpenUrlA");
    HttpQueryInfo = (PHTTP_QUERY_INFO)GetProcAddress(hWinInet, "HttpQueryInfoA");
#endif
    InetReadFile = (PINTERNET_READ_FILE)GetProcAddress(hWinInet, "InternetReadFile");
    InetCloseHandle = (PINTERNET_CLOSE_HANDLE)GetProcAddress(hWinInet, "InternetCloseHandle");

    if (InetOpen == NULL ||
        InetOpenUrl == NULL ||
        HttpQueryInfo == NULL ||
        InetReadFile == NULL ||
        InetCloseHandle == NULL) {

        Return = YoriLibUpdErrorInetInit;
        goto Exit;
    }

    //
    //  Open an internet connection with default proxy settings.
    //

    hInternet = InetOpen(Agent,
                         0,
                         NULL,
                         NULL,
                         0);

    if (hInternet == NULL) {
        Return = YoriLibUpdErrorInetInit;
        goto Exit;
    }

    //
    //  Newer versions of Windows will add a Host: header.  Old versions send
    //  an HTTP 1.1 request without one, which Apache doesn't like.
    //

    YoriLibInitEmptyString(&HostHeader);
    StartOfHost = _tcsstr(Url, _T("://"));
    if (StartOfHost != NULL) {
        StartOfHost += 3;
        EndOfHost = _tcschr(StartOfHost, '/');
        if (EndOfHost != NULL) {
            YORI_STRING HostName;
            YoriLibInitEmptyString(&HostName);
            HostName.StartOfString = StartOfHost;
            HostName.LengthInChars = (DWORD)(EndOfHost - StartOfHost);
            YoriLibYPrintf(&HostHeader, _T("Host: %y\r\n"), &HostName);
        }
    }

    //
    //  If the caller only wanted to fetch a resource if it's newer than an
    //  existing file, generate that header now.
    //

    YoriLibInitEmptyString(&IfModifiedSinceHeader);
    if (IfModifiedSince != NULL) {
        YoriLibYPrintf(&IfModifiedSinceHeader,
                       _T("If-Modified-Since: %hs, %02i %hs %04i %02i:%02i:%02i GMT\r\n"),
                       YoriLibDayNames[IfModifiedSince->wDayOfWeek],
                       IfModifiedSince->wDay,
                       YoriLibMonthNames[IfModifiedSince->wMonth - 1],
                       IfModifiedSince->wYear,
                       IfModifiedSince->wHour,
                       IfModifiedSince->wMinute,
                       IfModifiedSince->wSecond);
    }


    //
    //  Merge headers.  If we have only one or the other, this is just a
    //  reference with no allocation.
    //

    YoriLibInitEmptyString(&CombinedHeader);
    if (IfModifiedSinceHeader.LengthInChars > 0 && HostHeader.LengthInChars > 0) {
        YoriLibYPrintf(&CombinedHeader, _T("%y%y"), &HostHeader, &IfModifiedSinceHeader);
    } else if (IfModifiedSinceHeader.LengthInChars > 0) {
        YoriLibCloneString(&CombinedHeader, &IfModifiedSinceHeader);
    } else if (HostHeader.LengthInChars > 0) {
        YoriLibCloneString(&CombinedHeader, &HostHeader);
    }

    //
    //  Now all the headers are merged, we don't need the component parts.
    //

    YoriLibFreeStringContents(&HostHeader);
    YoriLibFreeStringContents(&IfModifiedSinceHeader);

    //
    //  Request the desired URL and check the status is HTTP success.
    //

    NewBinary = InetOpenUrl(hInternet, Url, CombinedHeader.StartOfString, CombinedHeader.LengthInChars, 0, 0);
    if (NewBinary == NULL) {
        dwError = GetLastError();
        YoriLibFreeStringContents(&CombinedHeader);
        Return = YoriLibUpdErrorInetConnect;
        goto Exit;
    }

    YoriLibFreeStringContents(&CombinedHeader);

    ErrorBufferSize = sizeof(dwError);
    ActualBinarySize = 0;
    dwError = 0;
    if (!HttpQueryInfo(NewBinary, 0x20000013, &dwError, &ErrorBufferSize, &ActualBinarySize)) {
        dwError = GetLastError();
        Return = YoriLibUpdErrorInetConnect;
        goto Exit;
    }

    if (dwError != 200) {
        if (dwError != 304 || IfModifiedSince == NULL) {
            Return = YoriLibUpdErrorInetConnect;
        }
        goto Exit;
    }

    //
    //  Create a temporary file to hold the contents.
    //

    if (GetTempPath(sizeof(TempPath)/sizeof(TempPath[0]), TempPath) == 0) {
        Return = YoriLibUpdErrorFileWrite;
        goto Exit;
    }

    if (GetTempFileName(TempPath, _T("UPD"), 0, TempName) == 0) {
        Return = YoriLibUpdErrorFileWrite;
        goto Exit;
    }

    hTempFile = CreateFile(TempName,
                           FILE_WRITE_DATA|FILE_READ_DATA,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           CREATE_ALWAYS,
                           0,
                           NULL);

    if (hTempFile == INVALID_HANDLE_VALUE) {
        Return = YoriLibUpdErrorFileWrite;
        goto Exit;
    }

    NewBinaryData = YoriLibMalloc(UPDATE_READ_SIZE);
    if (NewBinaryData == NULL) {
        Return = YoriLibUpdErrorFileWrite;
        goto Exit;
    }

    //
    //  Read from the internet location and save to the temporary file.
    //

    while (InetReadFile(NewBinary, NewBinaryData, 1024 * 1024, &ActualBinarySize)) {

        DWORD DataWritten;

        if (ActualBinarySize == 0) {
            SuccessfullyComplete = TRUE;
            break;
        }

        if (!WriteFile(hTempFile, NewBinaryData, ActualBinarySize, &DataWritten, NULL) ||
            DataWritten != ActualBinarySize) {

            Return = YoriLibUpdErrorFileWrite;
            goto Exit;
        }
    }

    //
    //  The only acceptable reason to fail is all the data has been
    //  received.
    //

    if (!SuccessfullyComplete) {
        Return = YoriLibUpdErrorInetRead;
        goto Exit;
    }

    //
    //  For validation, if the request is to modify the current executable
    //  check that the result is an executable.
    //

    if (TargetName == NULL) {
        SetFilePointer(hTempFile, 0, NULL, FILE_BEGIN);
        if (!ReadFile(hTempFile, NewBinaryData, 2, &ActualBinarySize, NULL) ||
            ActualBinarySize != 2 ||
            NewBinaryData[0] != 'M' ||
            NewBinaryData[1] != 'Z' ) {

            Return = YoriLibUpdErrorInetContents;
            goto Exit;
        }
    }

    //
    //  Now update the binary with the local file.
    //

    CloseHandle(hTempFile);
    YoriLibFree(NewBinaryData);

    if (YoriLibUpdateBinaryFromFile(TargetName, TempName)) {
        return YoriLibUpdErrorSuccess;
    } else {
        return YoriLibUpdErrorFileReplace;
    }

Exit:

    if (NewBinaryData != NULL) {
        YoriLibFree(NewBinaryData);
    }

    if (hTempFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hTempFile);
        DeleteFile(TempName);
    }

    if (NewBinary != NULL) {
        InetCloseHandle(NewBinary);
    }

    if (hInternet != NULL) {
        InetCloseHandle(hInternet);
    }

    if (hWinInet != NULL) {
        FreeLibrary(hWinInet);
    }

    return Return;
}

/**
 Returns a constant (not allocated) string corresponding to the specified
 update error code.

 @param Error The update error code.

 @return A pointer to a constant string with the error text.
 */
LPCTSTR
YoriLibUpdateErrorString(
    __in YoriLibUpdError Error
    )
{
    if (Error < YoriLibUpdErrorMax) {
        return YoriLibUpdErrorStrings[Error];
    }
    return _T("Not an update error");
}
