/**
 * @file lib/update.c
 *
 * Code to update a file from the internet including the running
 * executable.
 *
 * Copyright (c) 2016-2023 Malcolm J. Smith
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
    _T("Could not write data to temporary local file"),
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
    __in_opt PCYORI_STRING ExistingPath,
    __in PCYORI_STRING NewPath
    )
{
    YORI_STRING MyPath;
    YORI_STRING OldPath;
    PCYORI_STRING PathToReplace;
    HANDLE hMyBinary;

    ASSERT(ExistingPath == NULL || YoriLibIsStringNullTerminated(ExistingPath));
    ASSERT(YoriLibIsStringNullTerminated(NewPath));
    YoriLibInitEmptyString(&MyPath);
    YoriLibInitEmptyString(&OldPath);

    if (ExistingPath == NULL) {

        //
        //  Unlike most other Win32 APIs, this one has no way to indicate how
        //  much space it needs.
        //

        if (!YoriLibAllocateString(&MyPath, 32768)) {
            return FALSE;
        }

        //
        //  If the file name to replace is NULL, replace the currently
        //  existing binary.
        //

        MyPath.LengthInChars = GetModuleFileName(NULL, MyPath.StartOfString, MyPath.LengthAllocated);
        if (MyPath.LengthInChars == 0) {
            YoriLibFreeStringContents(&MyPath);
            return FALSE;
        }

        PathToReplace = &MyPath;
    } else {
        LPTSTR FinalBackslash;

        //
        //  If the file name to replace is a full path, defined as containing
        //  a backslash, replace that file path.
        //

        FinalBackslash = YoriLibFindRightMostCharacter(ExistingPath, '\\');

        if (FinalBackslash != NULL) {
            PathToReplace = ExistingPath;
        } else {

            //
            //  Unlike most other Win32 APIs, this one has no way to indicate how
            //  much space it needs.
            //

            if (!YoriLibAllocateString(&MyPath, 32768)) {
                return FALSE;
            }

            //
            //  If it's a file name only, assume that it refers to a file in
            //  the same path as the existing binary.
            //

            MyPath.LengthInChars = GetModuleFileName(NULL, MyPath.StartOfString, MyPath.LengthAllocated);
            if (MyPath.LengthInChars == 0) {
                YoriLibFreeStringContents(&MyPath);
                return FALSE;
            }

            FinalBackslash = YoriLibFindRightMostCharacter(&MyPath, '\\');
            if (FinalBackslash != NULL) {
                DWORD RemainingLength = (DWORD)(MyPath.LengthAllocated - (FinalBackslash - MyPath.StartOfString + 1));
                if (ExistingPath->LengthInChars >= RemainingLength) {
                    return FALSE;
                }
                YoriLibSPrintfS(FinalBackslash + 1, RemainingLength, _T("%y"), ExistingPath);
                MyPath.LengthInChars = MyPath.LengthInChars + ExistingPath->LengthInChars;
                PathToReplace = &MyPath;
            } else {
                PathToReplace = ExistingPath;
            }
        }
    }

    //
    //  If the file already exists, move it to a backup name.
    //

    if (GetFileAttributes(PathToReplace->StartOfString) != (DWORD)-1) {
        if (!YoriLibRenameFileToBackupName(PathToReplace, &OldPath)) {
            YoriLibFreeStringContents(&MyPath);
            return FALSE;
        }
    }

    //
    //  Rename the new file to where the old file was.  If it fails, try to
    //  move the old binary back.  If that fails, there's not much we can do.
    //

    if (!MoveFileEx(NewPath->StartOfString, PathToReplace->StartOfString, MOVEFILE_COPY_ALLOWED)) {
        if (OldPath.LengthInChars > 0) {
            MoveFileEx(OldPath.StartOfString, PathToReplace->StartOfString, MOVEFILE_COPY_ALLOWED);
            YoriLibFreeStringContents(&OldPath);
        }
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

    if (OldPath.LengthInChars > 0) {
        hMyBinary = CreateFile(OldPath.StartOfString,
                               DELETE,
                               FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_DELETE_ON_CLOSE,
                               NULL);
        YoriLibFreeStringContents(&OldPath);
    }

    return TRUE;
}

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
 Construct the HTTP headers to attach to the request.  This code is shared
 between WinInet and WinHttp.

 @param Url Pointer to the Url to access.

 @param IfModifiedSince Optionally points to a timestamp where only newer
        resources should be downloaded.

 @param OutputHeader On successful completion, populated with a newly
        allocated string containing all of the necessary HTTP headers.

 @param HostSubset On successful completion, updated to point to the substring
        within Url that refers to the host name to access.

 @param ObjectSubset On successful completion, updated to point to the
        beginning of the Url string containing the object to access.  This is
        immediately following the host name and continues to the end of the
        string, so this is null terminated, and no new allocation is needed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriLibUpdateBuildHttpHeaders(
    __in PCYORI_STRING Url,
    __in_opt PSYSTEMTIME IfModifiedSince,
    __out PYORI_STRING OutputHeader,
    __out PYORI_STRING HostSubset,
    __out LPTSTR *ObjectSubset
    )
{
    LPTSTR EndOfHost;
    YORI_STRING HostHeader;
    YORI_STRING IfModifiedSinceHeader;
    YORI_STRING CombinedHeader;
    YORI_STRING ProtocolDelimiter;
    DWORD StartOfHost;

    //
    //  Newer versions of Windows will add a Host: header.  Old versions send
    //  an HTTP 1.1 request without one, which Apache doesn't like.
    //

    YoriLibInitEmptyString(&HostHeader);
    YoriLibInitEmptyString(HostSubset);
    *ObjectSubset = NULL;

    YoriLibConstantString(&ProtocolDelimiter, _T("://"));
    if (YoriLibFindFirstMatchingSubstring(Url, 1, &ProtocolDelimiter, &StartOfHost)) {
        StartOfHost = StartOfHost + ProtocolDelimiter.LengthInChars;
        HostSubset->StartOfString = &Url->StartOfString[StartOfHost];
        HostSubset->LengthInChars = Url->LengthInChars - StartOfHost;
        EndOfHost = YoriLibFindLeftMostCharacter(HostSubset, '/');
        if (EndOfHost != NULL) {
            HostSubset->LengthInChars = (DWORD)(EndOfHost - HostSubset->StartOfString);
            *ObjectSubset = EndOfHost;
            YoriLibYPrintf(&HostHeader, _T("Host: %y\r\n"), HostSubset);
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
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

    memcpy(OutputHeader, &CombinedHeader, sizeof(YORI_STRING));
    return TRUE;
}

/**
 Download a file from the internet and store it in a local location using
 WinInet.dll.  This function is only used once WinInet is loaded.

 @param Dll Pointer to the Dll function table to use.  This allows this
        function to operate against WinInet.dll or a different structure
        with the same function signatures, which is used by the mini-HTTP
        client.

 @param Url The Url to download the file from.

 @param TargetName If specified, the local location to store the file.
        If not specified, the current executable name is used.

 @param Agent The user agent to report to the remote web server.

 @param IfModifiedSince If specified, indicates a timestamp where a new
        object should only be downloaded if it is newer.

 @return An update error code indicating success or appropriate error.
 */
YORI_LIB_UPDATE_ERROR
YoriLibUpdateBinaryFromUrlWinInet(
    __in PYORI_WININET_FUNCTIONS Dll,
    __in PCYORI_STRING Url,
    __in_opt PCYORI_STRING TargetName,
    __in PCYORI_STRING Agent,
    __in_opt PSYSTEMTIME IfModifiedSince
    )
{
    PVOID hInternet = NULL;
    PVOID NewBinary = NULL;
    PUCHAR NewBinaryData = NULL;
    DWORD ErrorBufferSize = 0;
    DWORD ActualBinarySize;
    YORI_STRING TempName;
    YORI_STRING TempPath;
    YORI_STRING PrefixString;
    HANDLE hTempFile = INVALID_HANDLE_VALUE;
    BOOL SuccessfullyComplete = FALSE;
    BOOL WinInetOnlySupportsAnsi = FALSE;
    DWORD dwError;
    YORI_LIB_UPDATE_ERROR Return = YoriLibUpdErrorSuccess;
    YORI_STRING CombinedHeader;
    YORI_STRING HostSubset;
    LPTSTR ObjectName;

    ASSERT(YoriLibIsStringNullTerminated(Url));
    ASSERT(YoriLibIsStringNullTerminated(Agent));
    ASSERT(TargetName == NULL || YoriLibIsStringNullTerminated(TargetName));

    YoriLibInitEmptyString(&TempName);
    YoriLibInitEmptyString(&TempPath);

    //
    //  Open an internet connection with default proxy settings.
    //

    hInternet = Dll->pInternetOpenW(Agent->StartOfString,
                                    0,
                                    NULL,
                                    NULL,
                                    0);

    if (hInternet == NULL) {
        DWORD LastError;
        LastError = GetLastError();

        //
        //  Internet Explorer 3 helpfully exports Unicode functions and then
        //  doesn't implement them.  In this case we have to downconvert to
        //  ANSI ourselves.  If a resource needs a particular encoding (ie.,
        //  goes beyond 7 bit ASCII) then this will likely fail, but typically
        //  the resources we use fit within that limitation.
        //

        if (LastError == ERROR_CALL_NOT_IMPLEMENTED) {
            LPSTR AnsiAgent;
            DWORD BytesForAnsiAgent;

            if (Dll->pInternetOpenA == NULL ||
                Dll->pInternetOpenUrlA == NULL) {

                Return = YoriLibUpdErrorInetInit;
                goto Exit;
            }

            WinInetOnlySupportsAnsi = TRUE;

            BytesForAnsiAgent = WideCharToMultiByte(CP_ACP,
                                                    0,
                                                    Agent->StartOfString,
                                                    Agent->LengthInChars,
                                                    NULL,
                                                    0,
                                                    NULL,
                                                    NULL);

            AnsiAgent = YoriLibMalloc(BytesForAnsiAgent + 1);
            if (AnsiAgent == NULL) {
                Return = YoriLibUpdErrorInetInit;
                return FALSE;
            }

            WideCharToMultiByte(CP_ACP,
                                0,
                                Agent->StartOfString,
                                Agent->LengthInChars,
                                AnsiAgent,
                                BytesForAnsiAgent,
                                NULL,
                                NULL);
            AnsiAgent[BytesForAnsiAgent] = '\0';

            hInternet = Dll->pInternetOpenA(AnsiAgent,
                                            0,
                                            NULL,
                                            NULL,
                                            0);

            YoriLibFree(AnsiAgent);
        }
    }

    if (hInternet == NULL) {
        Return = YoriLibUpdErrorInetInit;
        goto Exit;
    }

    if (!YoriLibUpdateBuildHttpHeaders(Url, IfModifiedSince, &CombinedHeader, &HostSubset, &ObjectName)) {
        Return = YoriLibUpdErrorInetInit;
        goto Exit;
    }

    //
    //  Request the desired URL and check the status is HTTP success.
    //

    if (WinInetOnlySupportsAnsi) {
        DWORD AnsiCombinedHeaderLength;
        LPSTR AnsiCombinedHeader;
        DWORD AnsiUrlLength;
        LPSTR AnsiUrl;

        AnsiCombinedHeaderLength = WideCharToMultiByte(CP_ACP,
                                                       0,
                                                       CombinedHeader.StartOfString,
                                                       CombinedHeader.LengthInChars,
                                                       NULL,
                                                       0,
                                                       NULL,
                                                       NULL);
        AnsiUrlLength = WideCharToMultiByte(CP_ACP,
                                            0,
                                            Url->StartOfString,
                                            Url->LengthInChars,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL);

        AnsiCombinedHeader = YoriLibMalloc(AnsiCombinedHeaderLength + 1);
        if (AnsiCombinedHeader == NULL) {
            YoriLibFreeStringContents(&CombinedHeader);
            Return = YoriLibUpdErrorInetInit;
            goto Exit;
        }

        AnsiUrl = YoriLibMalloc(AnsiUrlLength + 1);
        if (AnsiUrl == NULL) {
            YoriLibFree(AnsiCombinedHeader);
            YoriLibFreeStringContents(&CombinedHeader);
            Return = YoriLibUpdErrorInetInit;
            goto Exit;
        }

        WideCharToMultiByte(CP_ACP,
                            0,
                            CombinedHeader.StartOfString,
                            CombinedHeader.LengthInChars,
                            AnsiCombinedHeader,
                            AnsiCombinedHeaderLength,
                            NULL,
                            NULL);
        AnsiCombinedHeader[AnsiCombinedHeaderLength] = '\0';

        WideCharToMultiByte(CP_ACP,
                            0,
                            Url->StartOfString,
                            Url->LengthInChars,
                            AnsiUrl,
                            AnsiUrlLength,
                            NULL,
                            NULL);
        AnsiUrl[AnsiUrlLength] = '\0';

        NewBinary = Dll->pInternetOpenUrlA(hInternet,
                                           AnsiUrl,
                                           AnsiCombinedHeader,
                                           AnsiCombinedHeaderLength,
                                           0,
                                           0);
        YoriLibFree(AnsiUrl);
        YoriLibFree(AnsiCombinedHeader);

    } else {

        NewBinary = Dll->pInternetOpenUrlW(hInternet,
                                           Url->StartOfString,
                                           CombinedHeader.StartOfString,
                                           CombinedHeader.LengthInChars,
                                           0,
                                           0);
    }

    if (NewBinary == NULL) {
        YoriLibFreeStringContents(&CombinedHeader);
        Return = YoriLibUpdErrorInetConnect;
        goto Exit;
    }

    YoriLibFreeStringContents(&CombinedHeader);

    ErrorBufferSize = sizeof(dwError);
    ActualBinarySize = 0;
    dwError = 0;

    if (WinInetOnlySupportsAnsi) {
        if (!Dll->pHttpQueryInfoA(NewBinary,
                                  HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
                                  &dwError,
                                  &ErrorBufferSize,
                                  &ActualBinarySize)) {
            Return = YoriLibUpdErrorInetConnect;
            goto Exit;
        }
    } else {
        if (!Dll->pHttpQueryInfoW(NewBinary,
                                  HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
                                  &dwError,
                                  &ErrorBufferSize,
                                  &ActualBinarySize)) {
            Return = YoriLibUpdErrorInetConnect;
            goto Exit;
        }
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

    if (!YoriLibGetTempPath(&TempPath, 0)) {
        Return = YoriLibUpdErrorFileWrite;
        goto Exit;
    }

    YoriLibConstantString(&PrefixString, _T("UPD"));
    if (!YoriLibGetTempFileName(&TempPath, &PrefixString, &hTempFile, &TempName)) {
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

    while (Dll->pInternetReadFile(NewBinary, NewBinaryData, UPDATE_READ_SIZE, &ActualBinarySize)) {

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
    NewBinaryData = NULL;
    hTempFile = INVALID_HANDLE_VALUE;

    if (YoriLibUpdateBinaryFromFile(TargetName, &TempName)) {
        Return = YoriLibUpdErrorSuccess;
    } else {
        Return = YoriLibUpdErrorFileReplace;
    }

Exit:

    if (NewBinaryData != NULL) {
        YoriLibFree(NewBinaryData);
    }

    if (hTempFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hTempFile);
        DeleteFile(TempName.StartOfString);
    }

    YoriLibFreeStringContents(&TempPath);
    YoriLibFreeStringContents(&TempName);

    if (NewBinary != NULL) {
        Dll->pInternetCloseHandle(NewBinary);
    }

    if (hInternet != NULL) {
        Dll->pInternetCloseHandle(hInternet);
    }

    return Return;
}

/**
 Download a file from the internet and store it in a local location using
 WinHttp.dll.  This function is only used once WinHttp is loaded.

 @param Url The Url to download the file from.

 @param TargetName If specified, the local location to store the file.
        If not specified, the current executable name is used.

 @param Agent The user agent to report to the remote web server.

 @param IfModifiedSince If specified, indicates a timestamp where a new
        object should only be downloaded if it is newer.

 @return An update error code indicating success or appropriate error.
 */
YORI_LIB_UPDATE_ERROR
YoriLibUpdateBinaryFromUrlWinHttp(
    __in PCYORI_STRING Url,
    __in_opt PCYORI_STRING TargetName,
    __in PCYORI_STRING Agent,
    __in_opt PSYSTEMTIME IfModifiedSince
    )
{
    PVOID hInternet = NULL;
    PVOID hConnect = NULL;
    PVOID hRequest = NULL;
    YORI_LIB_UPDATE_ERROR Return = YoriLibUpdErrorSuccess;
    YORI_STRING HostSubset;
    YORI_STRING CombinedHeader;
    LPTSTR HostName;
    LPTSTR ObjectName;
    DWORD dwError;
    YORI_STRING TempName;
    YORI_STRING TempPath;
    YORI_STRING PrefixString;
    HANDLE hTempFile = INVALID_HANDLE_VALUE;
    PUCHAR NewBinaryData = NULL;
    DWORD ActualBinarySize;
    DWORD ErrorBufferSize = 0;
    BOOL SuccessfullyComplete = FALSE;

    ASSERT(YoriLibIsStringNullTerminated(Url));
    ASSERT(YoriLibIsStringNullTerminated(Agent));
    ASSERT(TargetName == NULL || YoriLibIsStringNullTerminated(TargetName));

    YoriLibInitEmptyString(&CombinedHeader);
    YoriLibInitEmptyString(&TempName);
    YoriLibInitEmptyString(&TempPath);

    //
    //  Open an internet connection with default proxy settings.
    //

    hInternet = DllWinHttp.pWinHttpOpen(Agent->StartOfString,
                                        0,
                                        NULL,
                                        NULL,
                                        0);

    if (hInternet == NULL) {
        Return = YoriLibUpdErrorInetInit;
        goto Exit;
    }

    YoriLibInitEmptyString(&CombinedHeader);
    if (!YoriLibUpdateBuildHttpHeaders(Url, IfModifiedSince, &CombinedHeader, &HostSubset, &ObjectName)) {
        Return = YoriLibUpdErrorInetInit;
        goto Exit;
    }

    HostName = YoriLibCStringFromYoriString(&HostSubset);
    if (HostName == NULL) {
        Return = YoriLibUpdErrorInetInit;
        goto Exit;
    }

    hConnect = DllWinHttp.pWinHttpConnect(hInternet, HostName, 0, 0);
    YoriLibDereference(HostName);
    if (hConnect == NULL) {
        Return = YoriLibUpdErrorInetInit;
        goto Exit;
    }

    hRequest = DllWinHttp.pWinHttpOpenRequest(hConnect, _T("GET"), ObjectName, NULL, NULL, NULL, 0);
    if (hRequest == NULL) {
        Return = YoriLibUpdErrorInetInit;
        goto Exit;
    }

    if (!DllWinHttp.pWinHttpSendRequest(hRequest,
                                        CombinedHeader.StartOfString,
                                        CombinedHeader.LengthInChars,
                                        NULL,
                                        0,
                                        0,
                                        0)) {
        Return = YoriLibUpdErrorInetConnect;
        goto Exit;
    }

    if (!DllWinHttp.pWinHttpReceiveResponse(hRequest, NULL)) {
        Return = YoriLibUpdErrorInetConnect;
        goto Exit;
    }

    ErrorBufferSize = sizeof(dwError);
    if (!DllWinHttp.pWinHttpQueryHeaders(hRequest,
                                         HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
                                         NULL,
                                         &dwError,
                                         &ErrorBufferSize,
                                         NULL)) {
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

    if (!YoriLibGetTempPath(&TempPath, 0)) {
        Return = YoriLibUpdErrorFileWrite;
        goto Exit;
    }

    YoriLibConstantString(&PrefixString, _T("UPD"));
    if (!YoriLibGetTempFileName(&TempPath, &PrefixString, &hTempFile, &TempName)) {
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

    while (DllWinHttp.pWinHttpReadData(hRequest, NewBinaryData, UPDATE_READ_SIZE, &ActualBinarySize)) {

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
    hTempFile = INVALID_HANDLE_VALUE;

    if (!YoriLibUpdateBinaryFromFile(TargetName, &TempName)) {
        Return = YoriLibUpdErrorFileReplace;
    }

Exit:

    if (NewBinaryData != NULL) {
        YoriLibFree(NewBinaryData);
    }

    if (hTempFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hTempFile);
        DeleteFile(TempName.StartOfString);
    }

    YoriLibFreeStringContents(&CombinedHeader);
    YoriLibFreeStringContents(&TempPath);
    YoriLibFreeStringContents(&TempName);

    if (hConnect != NULL) {
        DllWinHttp.pWinHttpCloseHandle(hConnect);
    }

    if (hRequest != NULL) {
        DllWinHttp.pWinHttpCloseHandle(hRequest);
    }

    if (hInternet != NULL) {
        DllWinHttp.pWinHttpCloseHandle(hInternet);
    }

    return Return;
}

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
YORI_LIB_UPDATE_ERROR
YoriLibUpdateBinaryFromUrl(
    __in PCYORI_STRING Url,
    __in_opt PCYORI_STRING TargetName,
    __in PCYORI_STRING Agent,
    __in_opt PSYSTEMTIME IfModifiedSince
    )
{
    YORI_WININET_FUNCTIONS StubWinInet;

    ASSERT(YoriLibIsStringNullTerminated(Url));
    ASSERT(YoriLibIsStringNullTerminated(Agent));

    //
    //  Dynamically load WinInet.  This means we don't have to resolve
    //  imports unless we're really using it for something, and we can
    //  degrade gracefully if it's not there (original 95/NT.)
    //

    YoriLibLoadWinInetFunctions();

    if (DllWinInet.pInternetOpenW != NULL &&
        DllWinInet.pInternetOpenUrlW != NULL &&
        DllWinInet.pHttpQueryInfoW != NULL &&
        DllWinInet.pInternetReadFile != NULL &&
        DllWinInet.pInternetCloseHandle != NULL) {

        return YoriLibUpdateBinaryFromUrlWinInet(&DllWinInet, Url, TargetName, Agent, IfModifiedSince);
    }

    //
    //  If WinInet isn't present, load WinHttp.  This path is taken on
    //  Nano server.
    //

    YoriLibLoadWinHttpFunctions();

    if (DllWinHttp.pWinHttpCloseHandle != NULL &&
        DllWinHttp.pWinHttpConnect != NULL &&
        DllWinHttp.pWinHttpOpen != NULL &&
        DllWinHttp.pWinHttpOpenRequest != NULL &&
        DllWinHttp.pWinHttpQueryHeaders != NULL &&
        DllWinHttp.pWinHttpReadData != NULL &&
        DllWinHttp.pWinHttpReceiveResponse != NULL &&
        DllWinHttp.pWinHttpSendRequest != NULL) {

        return YoriLibUpdateBinaryFromUrlWinHttp(Url, TargetName, Agent, IfModifiedSince);
    }

    //
    //  If neither of the above work, use our hard coded fallback.  This is
    //  really intended for NT 3.1 or other HTTP-less environments.
    //

    ZeroMemory(&StubWinInet, sizeof(StubWinInet));

    StubWinInet.pInternetOpenW = YoriLibInternetOpen;
    StubWinInet.pInternetOpenUrlW = YoriLibInternetOpenUrl;
    StubWinInet.pHttpQueryInfoW = YoriLibHttpQueryInfo;
    StubWinInet.pInternetReadFile = YoriLibInternetReadFile;
    StubWinInet.pInternetCloseHandle = YoriLibInternetCloseHandle;

    return YoriLibUpdateBinaryFromUrlWinInet(&StubWinInet, Url, TargetName, Agent, IfModifiedSince);
}

/**
 Returns a constant (not allocated) string corresponding to the specified
 update error code.

 @param Error The update error code.

 @return A pointer to a constant string with the error text.
 */
LPCTSTR
YoriLibUpdateErrorString(
    __in YORI_LIB_UPDATE_ERROR Error
    )
{
    if (Error >= 0 && Error < YoriLibUpdErrorMax) {
        return YoriLibUpdErrorStrings[Error];
    }
    return _T("Not an update error");
}
