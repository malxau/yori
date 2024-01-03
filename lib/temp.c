/**
 * @file lib/temp.c
 *
 * Yori temporary file routines
 *
 * Copyright (c) 2020 Malcolm J. Smith
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
 The OS provided GetTempPathName is documented to depend on ANSI paths.  This
 is not something that can be relied upon when the purpose of a temporary
 file is to write to an arbitrary user controlled directory.  Like its OS
 counterpart, this function attempts to find a unique name and will increment
 a counter trying different names until one is found that is not already in
 use.

 @param PathName Pointer to the path name to generate the temporary file in.
        The caller should presumably wants to ensure this is a full path on
        entry, although that is not a fixed requirement.

 @param PrefixString A prefix string to use for the beginning of the temporary
        name.  This should uniquely identify the calling application and
        should be less than or equal to four characters in order to support
        file systems with an 8.3 limit.

 @param TempHandle Optionally points to a variable to receive a handle to the
        temporary file, opened for write.  If NULL, no handle is returned.

 @param TempFileName Optionally points to a string to receive the temporary
        file name.  If NULL, no name is returned.  Note that this is allocated
        within this routine and is not required to be initialized on entry.

 @return TRUE if a name can be found, FALSE if it cannot.  Note that one
         reason for failure may be inability to write to the selected
         directory.
 */
__success(return)
BOOLEAN
YoriLibGetTempFileName(
    __in PYORI_STRING PathName,
    __in PYORI_STRING PrefixString,
    __out_opt PHANDLE TempHandle,
    __out_opt PYORI_STRING TempFileName
    )
{
    DWORD Err;
    WORD Unique;
    WORD Terminate;
    HANDLE Handle;
    YORI_STRING TestFileName;

    if (!YoriLibAllocateString(&TestFileName, PathName->LengthInChars + 1 + PrefixString->LengthInChars + 4 + sizeof(".tmp"))) {
        return FALSE;
    }

    //
    //  Warning about using a deprecated function and how we should use
    //  GetTickCount64 to prevent wraps.  The analyzer hasn't noticed that
    //  this is only using the low four bits of it anyway as a source of
    //  entropy.
    //
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(suppress: 28159)
#endif
    Unique = (WORD)(GetCurrentProcessId() >> 2 ^ GetTickCount() << 12);
    Terminate = (WORD)(Unique - 1);
    Err = ERROR_SUCCESS;
    Handle = NULL;
    while(TRUE) {
        TestFileName.LengthInChars = YoriLibSPrintf(TestFileName.StartOfString, _T("%y\\%y%04x.tmp"), PathName, PrefixString, Unique);

        Err = ERROR_SUCCESS;
        Handle = CreateFile(TestFileName.StartOfString,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE) {
            break;
        }

        Err = GetLastError();
        if (Err != ERROR_FILE_EXISTS &&
            Err != ERROR_DIRECTORY) {

            break;
        }

        Unique = (WORD)(Unique + 1);
        if (Unique == Terminate) {
            Err = ERROR_FILE_EXISTS;
            break;
        }
    }

    if (Err != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&TestFileName);
        return FALSE;
    }

    if (TempFileName != NULL) {
        memcpy(TempFileName, &TestFileName, sizeof(YORI_STRING));
    } else {
        YoriLibFreeStringContents(&TestFileName);
    }

    if (TempHandle != NULL) {
        *TempHandle = Handle;
    } else {
        CloseHandle(Handle);
    }

    return TRUE;
}

/**
 Return a path to the temp directory, but allocate extra space for a file name
 to append to it.

 @param TempPathName On successful completion, returns a newly allocated
        string populated with the temp directory.

 @param ExtraChars Specifies the number of extra characters to allocate in the
        string in addition to the temp directory size.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGetTempPath(
    __out PYORI_STRING TempPathName,
    __in YORI_ALLOC_SIZE_T ExtraChars
    )
{
    DWORD MajorVersion;
    DWORD MinorVersion;
    DWORD BuildNumber;
    YORI_ALLOC_SIZE_T CharsToAllocate;

    TempPathName->LengthAllocated = (YORI_ALLOC_SIZE_T)GetTempPath(0, NULL);
    if (TempPathName->LengthAllocated == 0) {
        return FALSE;
    }

    TempPathName->LengthAllocated = TempPathName->LengthAllocated + ExtraChars;

    YoriLibGetOsVersion(&MajorVersion, &MinorVersion, &BuildNumber);

    //
    //  NT 3.1 has a horrible bug in GetTempPathW where it confuses byte
    //  offset with character offset and writes a NULL at a byte offset
    //  which is twice where it should be.  To tolerate that, allocate
    //  twice as many bytes as needed, and manually fix up the NULL below.
    //

    CharsToAllocate = TempPathName->LengthAllocated;
    if (MajorVersion == 3 && MinorVersion == 10) {
        CharsToAllocate = CharsToAllocate * 2;
    }

    if (!YoriLibAllocateString(TempPathName, CharsToAllocate)) {
        return FALSE;
    }

    TempPathName->LengthInChars = (YORI_ALLOC_SIZE_T)GetTempPath(TempPathName->LengthAllocated,
            TempPathName->StartOfString);

    if (MajorVersion == 3 && MinorVersion == 10) {
        TempPathName->StartOfString[TempPathName->LengthInChars] = '\0';
    }

    return TRUE;
}


// vim:sw=4:ts=4:et:
