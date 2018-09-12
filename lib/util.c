/**
 * @file /lib/util.c
 *
 * Yori trivial utility routines
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
 Returns TRUE if the character should be treated as an escape character.

 @param Char The character to check.

 @return TRUE if the character is an escape character, FALSE if it is not.
 */
BOOL
YoriLibIsEscapeChar(
    __in TCHAR Char
    )
{
    if (Char == '^') {
        return TRUE;
    }
    return FALSE;
}

/**
 Convert a noninheritable handle into an inheritable handle.

 @param OriginalHandle A handle to convert, which is presumably not
        inheritable.  On success, this handle is closed.

 @param InheritableHandle On successful completion, populated with a new
        handle which is inheritable.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibMakeInheritableHandle(
    __in HANDLE OriginalHandle,
    __out PHANDLE InheritableHandle
    )
{
    HANDLE NewHandle;

    if (DuplicateHandle(GetCurrentProcess(),
                        OriginalHandle,
                        GetCurrentProcess(),
                        &NewHandle,
                        0,
                        TRUE,
                        DUPLICATE_SAME_ACCESS)) {

        CloseHandle(OriginalHandle);
        *InheritableHandle = NewHandle;
        return TRUE;
    }

    return FALSE;
}

/**
 A constant string to return if detailed error text could not be returned.
 */
CONST LPTSTR NoWinErrorText = _T("Could not fetch error text.");

/**
 Lookup a Win32 error code and return a pointer to the error string.
 If the string could not be located, returns NULL.  The returned string
 should be freed with @ref YoriLibFreeWinErrorText.

 @param ErrorCode The error code to fetch a string for.

 @return Pointer to the error string on success, NULL on failure.
 */
LPTSTR
YoriLibGetWinErrorText(
    __in DWORD ErrorCode
    )
{
    LPTSTR OutputBuffer = NULL;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  ErrorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&OutputBuffer,
                  0,
                  NULL);
    
    if (OutputBuffer == NULL) {
        OutputBuffer = NoWinErrorText;
    }

    return OutputBuffer;
}

/**
 Free an error string previously allocated with @ref YoriLibGetWinErrorText .

 @param ErrText Pointer to the error text string.
 */
VOID
YoriLibFreeWinErrorText(
    __in LPTSTR ErrText
    )
{
    if (ErrText != NULL && ErrText != NoWinErrorText) {
        LocalFree(ErrText);
    }
}

/**
 Create a directory, and any parent directories that do not yet exist.
 Note that this routine temporarily alters the DirName buffer - it is not
 const, but will be restored to original contents on exit.

 @param DirName The directory to create.
 */
BOOL
YoriLibCreateDirectoryAndParents(
    __in PYORI_STRING DirName
    )
{
    DWORD MaxIndex = DirName->LengthInChars - 1;
    DWORD Err;
    DWORD SepIndex = MaxIndex;
    BOOL StartedSucceeding = FALSE;

    while (TRUE) {
        if (!CreateDirectory(DirName->StartOfString, NULL)) {
            Err = GetLastError();
            if (Err == ERROR_PATH_NOT_FOUND && !StartedSucceeding) {

                //
                //  MSFIX Check for truncation beyond \\?\ or \\?\UNC\ ?
                //

                for (;!YoriLibIsSep(DirName->StartOfString[SepIndex]) && SepIndex > 0; SepIndex--) {
                }

                if (!YoriLibIsSep(DirName->StartOfString[SepIndex])) {
                    return FALSE;
                }

                DirName->StartOfString[SepIndex] = '\0';
                DirName->LengthInChars = SepIndex;
                continue;

            } else {
                if (Err == ERROR_ALREADY_EXISTS) {
                    return TRUE;
                }
                return FALSE;
            }
        } else {
            StartedSucceeding = TRUE;
            if (SepIndex < MaxIndex) {
                ASSERT(DirName->StartOfString[SepIndex] == '\0');

                DirName->StartOfString[SepIndex] = '\\';
                for (;DirName->StartOfString[SepIndex] != '\0' && SepIndex <= MaxIndex; SepIndex++);
                DirName->LengthInChars = SepIndex;
                continue;
            } else {
                return TRUE;
            }
        }
    }
}

/**
 Returns TRUE if the specified path is an Internet path that requires wininet.
 Technically this can return FALSE for paths that are still remote (SMB paths),
 but those are functionally the same as local paths.

 @param PackagePath Pointer to the path to check.

 @return TRUE if the path is an Internet path, FALSE if it is a local path.
 */
BOOL
YoriLibIsPathUrl(
    __in PYORI_STRING PackagePath
    )
{
    if (YoriLibCompareStringWithLiteralInsensitiveCount(PackagePath, _T("http://"), sizeof("http://") - 1) == 0 ||
        YoriLibCompareStringWithLiteralInsensitiveCount(PackagePath, _T("https://"), sizeof("https://") - 1) == 0 ||
        YoriLibCompareStringWithLiteralInsensitiveCount(PackagePath, _T("ftp://"), sizeof("ftp://") - 1) == 0) {

        return TRUE;
    }

    return FALSE;
}

// vim:sw=4:ts=4:et:
