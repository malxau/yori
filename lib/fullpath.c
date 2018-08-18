/**
 * @file lib/fullpath.c
 *
 * A custom implementation of GetFullPathName() to work around MAX_PATH and
 * absurd DOS file name limitations.
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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
 The character to use in a full path prefix.  Generally this is '?', as in
 \\?\ .  For very old versions of Windows it is '.', as those versions do
 not support the \\?\ prefix but do have a \\.\ prefix which is semantically
 similar, except that it is limited to MAX_PATH.
 */
TCHAR PathPrefixChar;

/**
 Set to TRUE once the module has been initialized, which will happen after
 setting @ref PathPrefixChar to a meaningful value.
 */
BOOL YoriLibFullPathInitialized = FALSE;

/**
 Return TRUE if the path consists of a drive letter and colon, potentially
 followed by other characters.

 @param Path Pointer to the string to check.

 @return TRUE if the string starts with a drive letter and colon.
 */
BOOL
YoriLibIsDriveLetterWithColon(
    __in PYORI_STRING Path
    )
{
    if (Path->LengthInChars < 2) {
        return FALSE;
    }

    if (((Path->StartOfString[0] >= 'A' && Path->StartOfString[0] <= 'Z') ||
         (Path->StartOfString[0] >= 'a' && Path->StartOfString[0] <= 'z')) &&
        Path->StartOfString[1] == ':') {

        return TRUE;
    }

    return FALSE;
}

/**
 Return TRUE if the path consists of a drive letter, colon, and path
 seperator, potentially followed by other characters.

 @param Path Pointer to the string to check.

 @return TRUE if the string starts with a drive letter, colon, and
         seperator; FALSE otherwise.
 */
BOOL
YoriLibIsDriveLetterWithColonAndSlash(
    __in PYORI_STRING Path
    )
{
    if (Path->LengthInChars < 3) {
        return FALSE;
    }

    if (YoriLibIsDriveLetterWithColon(Path) &&
        YoriLibIsSep(Path->StartOfString[2])) {

        return TRUE;
    }

    return FALSE;
}


/**
 Return TRUE if the path is a UNC path.  This function assumes that the
 path must already have the \\?\ prefix.

 @param Path The path to check.

 @return TRUE to indicate a UNC path, FALSE to indicate that it is not a UNC
         path.
 */
BOOL
YoriLibIsFullPathUnc(
    __in PYORI_STRING Path
    )
{
    if (Path->LengthInChars < 8) {
        return FALSE;
    }

    //
    //  This function assumes it's dealing with paths already prepended
    //  with \\?\ and does not attempt to detect other forms of UNC paths.
    //

    if ((Path->StartOfString[4] == 'U' || Path->StartOfString[4] == 'u') &&
        (Path->StartOfString[5] == 'N' || Path->StartOfString[5] == 'n') &&
        (Path->StartOfString[6] == 'C' || Path->StartOfString[6] == 'c') &&
        (Path->StartOfString[7] == '\\')) {

        return TRUE;
    }
    return FALSE;
}

/**
 Return the current directory on a drive without changing the current
 directory.

 @param Drive The drive letter whose current directory should be returned.

 @param DriveCurrentDirectory On successful completion, populated with the
        current directory for the specified drive.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGetCurrentDirectoryOnDrive(
    __in TCHAR Drive,
    __inout PYORI_STRING DriveCurrentDirectory
    )
{
    TCHAR EnvVariableName[4];
    DWORD DriveCurrentDirectoryLength;

    EnvVariableName[0] = '=';
    EnvVariableName[1] = Drive;
    EnvVariableName[2] = ':';
    EnvVariableName[3] = '\0';

    DriveCurrentDirectoryLength = GetEnvironmentVariable(EnvVariableName, NULL, 0);

    if (DriveCurrentDirectoryLength > 0) {
        if (!YoriLibAllocateString(DriveCurrentDirectory, DriveCurrentDirectoryLength)) {
            return FALSE;
        }

        DriveCurrentDirectory->LengthInChars = GetEnvironmentVariable(EnvVariableName, DriveCurrentDirectory->StartOfString, DriveCurrentDirectory->LengthAllocated);
        if (DriveCurrentDirectory->LengthInChars == 0 || DriveCurrentDirectory->LengthInChars >= DriveCurrentDirectoryLength) {
            YoriLibFreeStringContents(DriveCurrentDirectory);
            return FALSE;
        }
    } else {
        if (!YoriLibAllocateString(DriveCurrentDirectory, 4)) {
            return FALSE;
        }
        YoriLibSPrintf(DriveCurrentDirectory->StartOfString, _T("%c:\\"), Drive);
        DriveCurrentDirectory->LengthInChars = 3;
    }
    return TRUE;
}

/**
 Update the current directory on a drive without changing the current
 directory.

 @param Drive The drive letter whose current directory should be updated.

 @param DriveCurrentDirectory The directory to set as the current directory
        on the specified drive.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibSetCurrentDirectoryOnDrive(
    __in TCHAR Drive,
    __in PYORI_STRING DriveCurrentDirectory
    )
{
    TCHAR EnvVariableName[4];
    LPTSTR szDriveCurrentDirectory;
    BOOL Result;

    EnvVariableName[0] = '=';
    EnvVariableName[1] = Drive;
    EnvVariableName[2] = ':';
    EnvVariableName[3] = '\0';

    szDriveCurrentDirectory = YoriLibMalloc((DriveCurrentDirectory->LengthInChars + 1) * sizeof(TCHAR));
    if (szDriveCurrentDirectory == NULL) {
        return FALSE;
    }

    YoriLibSPrintf(szDriveCurrentDirectory, _T("%y"), DriveCurrentDirectory);

    Result = SetEnvironmentVariable(EnvVariableName, szDriveCurrentDirectory);
    YoriLibFree(szDriveCurrentDirectory);
    return Result;
}

/**
 Private implementation of GetFullPathName.  This function will convert paths
 into their \\?\ (MAX_PATH exceeding) form, applying all of the necessary
 transformations.  This function will update a YORI_STRING, including
 allocating if necessary, to contain a buffer containing the path.  If this
 function allocates the buffer, the caller is expected to free this by calling
 @ref YoriLibFreeStringContents.

 @param FileName The file name to resolve to a full path form.

 @param bReturnEscapedPath If TRUE, the returned path is in \\?\ form.
        If FALSE, it is in regular Win32 form.

 @param Buffer On successful completion, updated to point to a newly
        allocated buffer containing the full path form of the file name.
        Note this is returned as a NULL terminated YORI_STRING, suitable
        for use in YORI_STRING or NULL terminated functions.

 @param lpFilePart If specified, on successful completion, updated to point
        to the beginning of the file name component of the path, in the same
        allocation as lpBuffer.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibGetFullPathNameReturnAllocation(
    __in PYORI_STRING FileName,
    __in BOOL bReturnEscapedPath,
    __inout PYORI_STRING Buffer,
    __deref_opt_out LPTSTR* lpFilePart
    )
{
    //
    //  This function is a drop in replacement for GetFullPathName.  It
    //  returns paths prepended with \\?\, which is advantageous for a
    //  few reasons:
    //  1. "C:\con" (et al) is treated like a legitimate file, not a
    //     console.
    //  2. "C:\file " (trailing space) is preserved without truncation.
    //
    //  Cases this function needs to handle:
    //  1. Absolute paths, starting with \\ or X:\.  We need to remove
    //     relative components from these.
    //  2. Drive relative paths (C:foo).  Here we need to merge with the
    //     current directory for the specified drive.
    //  3. Absolute paths without drive (\foo).  Here we need to merge
    //     with the current drive letter.
    //  4. Relative paths, with no prefix.  These need to have current
    //     directory prepended.
    //
    //  Regardless of the case above, this function should flatten out
    //  any "." or ".." components in the path.
    //

    BOOLEAN DriveRelativePath = FALSE;
    BOOLEAN AbsoluteWithoutDrive = FALSE;
    BOOLEAN RelativePath = FALSE;
    BOOLEAN PrefixPresent = FALSE;
    BOOLEAN UncPath = FALSE;
    BOOLEAN PreviousWasSeperator;
    BOOLEAN FreeOnFailure = FALSE;

    LPTSTR EffectiveRoot;
    YORI_STRING StartOfRelativePath;

    LPTSTR CurrentReadChar;
    LPTSTR CurrentWriteChar;

    DWORD Result;

    YoriLibInitEmptyString(&StartOfRelativePath);
    StartOfRelativePath.StartOfString = FileName->StartOfString;
    StartOfRelativePath.LengthInChars = FileName->LengthInChars;

    //
    //  On NT 3.x, use \\.\ instead of \\?\ since the latter does not
    //  appear to be supported there
    //

    if (!YoriLibFullPathInitialized) {
        DWORD OsVer = GetVersion();

        PathPrefixChar = '?';

        if (LOBYTE(LOWORD(OsVer)) < 4) {
            if (LOBYTE(LOWORD(OsVer)) == 3 && HIBYTE(LOWORD(OsVer)) < 51) {
                PathPrefixChar = '.';
            }
        }
        YoriLibFullPathInitialized = TRUE;
    }

    //
    //  First, determine which of the cases above we're processing.
    //

    if (FileName->LengthInChars >= 2 &&
        YoriLibIsSep(FileName->StartOfString[0]) &&
        YoriLibIsSep(FileName->StartOfString[1])) {

        UncPath = TRUE;

        if (FileName->LengthInChars >= 4 &&
            (FileName->StartOfString[2] == '?' ||
             FileName->StartOfString[2] == '.' ) &&
            YoriLibIsSep(FileName->StartOfString[3])) {

            PrefixPresent = TRUE;

            if (!YoriLibIsFullPathUnc(FileName)) {
                UncPath = FALSE;
            }
        }

    } else if (YoriLibIsDriveLetterWithColon(FileName)) {

        if (FileName->LengthInChars == 2 ||
            (FileName->LengthInChars >= 3 && !YoriLibIsSep(FileName->StartOfString[2]))) {

            DriveRelativePath = TRUE;
            StartOfRelativePath.StartOfString += 2;
            StartOfRelativePath.LengthInChars -= 2;
        }
    } else if (FileName->LengthInChars >= 1 && YoriLibIsSep(FileName->StartOfString[0])) {
        AbsoluteWithoutDrive = TRUE;
    } else {
        RelativePath = TRUE;
    }

    //
    //  If it's a relative case, get the current directory, and generate an
    //  "absolute" form of the name.  If it's an absolute case, prepend
    //  \\?\ and have a buffer we allocate for subsequent munging.
    //

    if (DriveRelativePath || RelativePath || AbsoluteWithoutDrive) {

        YORI_STRING CurrentDirectory;

        //
        //  If it's drive relative, get the current directory of the requested
        //  drive.  The only documented way to do this is to call
        //  GetFullPathName itself, which looks a little goofy given where
        //  we are.
        //

        if (DriveRelativePath) {
            if (!YoriLibGetCurrentDirectoryOnDrive(FileName->StartOfString[0], &CurrentDirectory)) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
        } else {
            DWORD CurrentDirectoryLength;
            CurrentDirectoryLength = GetCurrentDirectory(0, NULL);
            if (!YoriLibAllocateString(&CurrentDirectory, CurrentDirectoryLength)) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
    
            Result = GetCurrentDirectory(CurrentDirectory.LengthAllocated, CurrentDirectory.StartOfString);
            if (Result == 0 || Result >= CurrentDirectory.LengthAllocated) {
                YoriLibFreeStringContents(&CurrentDirectory);
                return FALSE;
            }

            CurrentDirectory.LengthInChars = Result;
        }

        //
        //  Assume the current directory is a normal, boring Win32 form,
        //  such as "C:\Foo".
        //

        //
        //  Check if it's already escaped, and whether or not it's
        //  UNC.  If it's UNC, truncate the beginning so that it's
        //  pointing to a single backslash only, regardless of whether
        //  it was initially escaped or not.
        //

        if (CurrentDirectory.LengthInChars >= 4 &&
            CurrentDirectory.StartOfString[0] == '\\' &&
            CurrentDirectory.StartOfString[1] == '\\' &&
            (CurrentDirectory.StartOfString[2] == '?' || CurrentDirectory.StartOfString[2] == '.') &&
            CurrentDirectory.StartOfString[3] == '\\') {

            if (YoriLibIsFullPathUnc(&CurrentDirectory)) {
                CurrentDirectory.StartOfString += 7;
                CurrentDirectory.LengthInChars -= 7;
                UncPath = TRUE;
            } else {
                CurrentDirectory.StartOfString += 4;
                CurrentDirectory.LengthInChars -= 4;
            }
        } else {
            if (CurrentDirectory.LengthInChars >= 2 &&
                YoriLibIsSep(CurrentDirectory.StartOfString[0]) &&
                YoriLibIsSep(CurrentDirectory.StartOfString[1])) {

                CurrentDirectory.StartOfString += 1;
                CurrentDirectory.LengthInChars -= 1;
                UncPath = TRUE;
            }
        }

        //
        //  Just truncate the current directory to two chars and let
        //  normal processing continue.
        //

        if (AbsoluteWithoutDrive) {
            if (CurrentDirectory.LengthInChars > 2) {
                CurrentDirectory.LengthInChars = 2;
                CurrentDirectory.StartOfString[2] = '\0';
            }
        }

        //
        //  Calculate the length of the "absolute" form of this path and
        //  generate it through concatenation.  This is the prefix, the
        //  current directory, a seperator, and the relative path.  Note
        //  the current directory length includes space for a NULL.
        //

        if (bReturnEscapedPath) {
            Result = 4 + CurrentDirectory.LengthInChars + 1 + StartOfRelativePath.LengthInChars + 1;
            if (UncPath) {
                Result += 4; // Remove "\" and add "\UNC\"
            }
        } else {
            Result = CurrentDirectory.LengthInChars + 1 + StartOfRelativePath.LengthInChars + 1;
            if (UncPath) {
                Result += 1;
            }
        }

        if (Result > Buffer->LengthAllocated) {
            YoriLibFreeStringContents(Buffer);
            if (!YoriLibAllocateString(Buffer, Result)) {
                YoriLibFreeStringContents(&CurrentDirectory);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            FreeOnFailure = TRUE;
        }

        //
        //  Enter the matrix.
        //
        //  Return escaped path *
        //  Source is already UNC *
        //  Is there a file or are we just getting current directory (x:).
        //

        if (bReturnEscapedPath) {
            if (UncPath) {
                if (StartOfRelativePath.LengthInChars > 0) {
                    Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\\\%c\\UNC%y\\%y"), PathPrefixChar, &CurrentDirectory, &StartOfRelativePath);
                } else {
                    Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\\\%c\\UNC%y"), PathPrefixChar, &CurrentDirectory);
                }
            } else {
                if (StartOfRelativePath.LengthInChars > 0) {
                    Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\\\%c\\%y\\%y"), PathPrefixChar, &CurrentDirectory, &StartOfRelativePath);
                } else {
                    Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\\\%c\\%y"), PathPrefixChar, &CurrentDirectory);
                }
            }
        } else {
            if (UncPath) {
                if (StartOfRelativePath.LengthInChars > 0) {
                    Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\%y\\%y"), &CurrentDirectory, &StartOfRelativePath);
                } else {
                    Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\%y"), &CurrentDirectory);
                }
            } else {
                if (StartOfRelativePath.LengthInChars > 0) {
                    Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("%y\\%y"), &CurrentDirectory, &StartOfRelativePath);
                } else {
                    Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("%y"), &CurrentDirectory);
                }
            }
        }
        YoriLibFreeStringContents(&CurrentDirectory);
    } else {

        //
        //  Allocate a buffer for the "absolute" path so we can do compaction.
        //  If the path is UNC, convert to \\?\UNC\...
        //

        if (bReturnEscapedPath) {
            Result = FileName->LengthInChars + 1 + 4;
            if (!PrefixPresent && UncPath) {
                Result += 4;
            }
        } else {
            Result = FileName->LengthInChars + 1;
        }

        if (Result > Buffer->LengthAllocated) {
            YoriLibFreeStringContents(Buffer);
            if (!YoriLibAllocateString(Buffer, Result)) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            FreeOnFailure = TRUE;
        }

        if (bReturnEscapedPath) {
            if (!PrefixPresent && UncPath) {
                //
                //  Input string is \\server\share, output is
                //  \\?\UNC\server\share, chop off the first two slashes
                //

                StartOfRelativePath.StartOfString += 2;
                StartOfRelativePath.LengthInChars -= 2;
            }
        } else {
            if (PrefixPresent) {
                if (UncPath) {

                    //
                    //  Input string is \\?\UNC\server\share, output is
                    //  \\server\share, chop off \\?\UNC
                    //

                    StartOfRelativePath.StartOfString += 7;
                    StartOfRelativePath.LengthInChars -= 7;
                } else {

                    //
                    //  Input string is \\?\C:\, output is C:\, chop off
                    //  first four chars
                    //

                    StartOfRelativePath.StartOfString += 4;
                    StartOfRelativePath.LengthInChars -= 4;
                }
            }
        }

        if (bReturnEscapedPath) {

            if (PrefixPresent) {

                Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("%y"), &StartOfRelativePath);
            } else if (UncPath) {
                Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\\\%c\\UNC\\%y"), PathPrefixChar, &StartOfRelativePath);
            } else {
                Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\\\%c\\%y"), PathPrefixChar, &StartOfRelativePath);
            }
        } else {
            if (PrefixPresent) {
                if (UncPath) {
                    Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\%y"), &StartOfRelativePath);
                } else {
                    Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("%y"), &StartOfRelativePath);
                }
            } else {
                Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("%y"), &StartOfRelativePath);
            }
        }
    }

    //
    //  Convert forward slashes to backslashes across the entire path
    //

    for (Result = 0; Result < Buffer->LengthInChars; Result++) {

        if (Buffer->StartOfString[Result] == '/') {
            Buffer->StartOfString[Result] = '\\';
        }
    }

    //
    //  At this point we should have an allocated, "absolute" name.  We still
    //  need to process relative components, and as we do so, there's a point
    //  we can't traverse back through.  Eg. we don't do relative paths
    //  before \\?\X:\ or \\?\UNC\server\share.
    //

    if (bReturnEscapedPath) {
        if (YoriLibIsFullPathUnc(Buffer)) {
            UncPath = TRUE;
        } else {
            UncPath = FALSE;
        }

        if (UncPath) {

            EffectiveRoot = _tcschr(&Buffer->StartOfString[8], '\\');

            //
            //  If the path is \\server (no share), we can't handle it.
            //

            if (EffectiveRoot == NULL) {
                SetLastError(ERROR_BAD_PATHNAME);
                if (FreeOnFailure) {
                    YoriLibFreeStringContents(Buffer);
                }
                return FALSE;
            }

            EffectiveRoot = _tcschr(&EffectiveRoot[1], '\\');

            //
            //  If the path is \\?\UNC\server\share (exactly, no trailing
            //  component), it's as full as it'll ever get.  We should
            //  just return it here.
            //

            if (EffectiveRoot == NULL) {
                if (lpFilePart != NULL) {
                    *lpFilePart = NULL;
                }
                ASSERT(Buffer->StartOfString[Buffer->LengthInChars] == '\0');
                return TRUE;
            }

            PreviousWasSeperator = FALSE;

        } else {

            for (Result = 4; Result < Buffer->LengthInChars && Buffer->StartOfString[Result] != '\\'; Result++);

            if (Buffer->StartOfString[Result] == '\\') {
                EffectiveRoot = &Buffer->StartOfString[Result + 1];
                PreviousWasSeperator = TRUE;
            } else {
                EffectiveRoot = &Buffer->StartOfString[Result];
                PreviousWasSeperator = FALSE;
            }
        }
    } else {
        if (Buffer->StartOfString[0] == '\\' && Buffer->StartOfString[1] == '\\') {
            EffectiveRoot = _tcschr(&Buffer->StartOfString[2], '\\');

            //
            //  If the path is \\server (no share), we can't handle it.
            //

            if (EffectiveRoot == NULL) {
                SetLastError(ERROR_BAD_PATHNAME);
                if (FreeOnFailure) {
                    YoriLibFreeStringContents(Buffer);
                }
                return FALSE;
            }

            EffectiveRoot = _tcschr(&EffectiveRoot[1], '\\');

            //
            //  If the path is \\server\share (exactly, no trailing
            //  component), it's as full as it'll ever get.  We should
            //  just return it here.
            //

            if (EffectiveRoot == NULL) {
                if (lpFilePart != NULL) {
                    *lpFilePart = NULL;
                }
                ASSERT(Buffer->StartOfString[Buffer->LengthInChars] == '\0');
                return TRUE;
            }

            PreviousWasSeperator = FALSE;
        } else {
            EffectiveRoot = &Buffer->StartOfString[3];
            PreviousWasSeperator = TRUE;
        }
    }

    //
    //  Now process the path to remove duplicate slashes, remove .
    //  components, and process .. components.
    //

    CurrentWriteChar = EffectiveRoot;

    for (CurrentReadChar = EffectiveRoot; *CurrentReadChar != '\0'; CurrentReadChar++) {

        //
        //  Strip duplicate backslashes
        //

        if (PreviousWasSeperator &&
            CurrentReadChar[0] == '\\') {

            continue;
        }

        //
        //  If the component is \.\, strip it
        //

        if (PreviousWasSeperator && 
            CurrentReadChar[0] == '.' &&
            (CurrentReadChar[1] == '\\' || CurrentReadChar[1] == '\0')) {

            if (CurrentWriteChar > EffectiveRoot) {
                CurrentWriteChar--;
                PreviousWasSeperator = FALSE;
            }

            continue;
        }

        //
        //  If the component is \..\, back up, but don't continue beyond
        //  EffectiveRoot
        //

        if (PreviousWasSeperator && 
            CurrentReadChar[0] == '.' &&
            CurrentReadChar[1] == '.' &&
            (CurrentReadChar[2] == '\\' || CurrentReadChar[2] == '\0')) {

            PreviousWasSeperator = FALSE;

            //
            //  Walk back one component or until the root
            //

            CurrentWriteChar--;
            while (CurrentWriteChar > EffectiveRoot) {

                CurrentWriteChar--;

                if (*CurrentWriteChar == '\\') {
                    break;
                }
            }

            //
            //  If we were already on effective root when entering
            //  this block, we backed up one char too many already
            //

            if (CurrentWriteChar < EffectiveRoot) {
                CurrentWriteChar = EffectiveRoot;
            }

            //
            //  If we get to the root, check if the previous
            //  char was a seperator.  Messy because UNC and
            //  drive letters resolve this differently
            //

            if (CurrentWriteChar == EffectiveRoot) {
                CurrentWriteChar--;
                if (*CurrentWriteChar == '\\') {
                    PreviousWasSeperator = TRUE;
                }
                CurrentWriteChar++;
            }

            CurrentReadChar++;
            continue;
        }

        //
        //  Note if this is a seperator or not.  If it's the final char and
        //  a seperator, drop it
        //

        if (*CurrentReadChar == '\\') {
            PreviousWasSeperator = TRUE;
        } else {
            PreviousWasSeperator = FALSE;
        }
 
        *CurrentWriteChar = *CurrentReadChar;
        CurrentWriteChar++;
    }

    //
    //  One more to copy the NULL
    //

    *CurrentWriteChar = *CurrentReadChar;
    Buffer->LengthInChars = (DWORD)(CurrentWriteChar - Buffer->StartOfString);
    ASSERT(Buffer->StartOfString[Buffer->LengthInChars] == '\0');

    //
    //  Walk back to find the file part
    //

    while (CurrentWriteChar >= EffectiveRoot) {

        CurrentWriteChar--;

        if (*CurrentWriteChar == '\\') {
            CurrentWriteChar++;
            break;
        }
    }

    if (CurrentWriteChar < EffectiveRoot || *CurrentWriteChar == '\0') {
        CurrentWriteChar = NULL;
    }

    //
    //  Return a pointer to the allocation and file part.
    //

    if (lpFilePart != NULL) {
        *lpFilePart = CurrentWriteChar;
    }
    SetLastError(0);
    return TRUE;
}


/**
 Convert a specified shell folder, by a known folder GUID, into its string
 form.  This function is only available in Vista+.

 @param FolderType The identifier of the directory.

 @param ExpandedSymbol On successful completion, populated with a string
        identifying the path to the directory.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibExpandShellDirectoryGuid(
    __in CONST GUID * FolderType,
    __inout PYORI_STRING ExpandedSymbol
    )
{
    PWSTR ExpandedString;
    DWORD LocationLength;

    YoriLibLoadShell32Functions();
    YoriLibLoadOle32Functions();

    if (DllShell32.pSHGetKnownFolderPath == NULL ||
        DllOle32.pCoTaskMemFree == NULL) {
        return FALSE;
    }


    if (DllShell32.pSHGetKnownFolderPath(FolderType, 0, NULL, &ExpandedString) != 0) {
        return FALSE;
    }

    LocationLength = _tcslen(ExpandedString);

    if (!YoriLibAllocateString(ExpandedSymbol, LocationLength + 1)) {
        DllOle32.pCoTaskMemFree(ExpandedString);
        return FALSE;
    }

    memcpy(ExpandedSymbol->StartOfString, ExpandedString, (LocationLength + 1) * sizeof(TCHAR));
    ExpandedSymbol->LengthInChars = LocationLength;

    DllOle32.pCoTaskMemFree(ExpandedString);
    return TRUE;
}


/**
 Convert a specified shell folder, by identifier, into its string form.

 @param FolderType The identifier of the directory.

 @param ExpandedSymbol On successful completion, populated with a string
        identifying the path to the directory.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibExpandShellDirectory(
    __in INT FolderType,
    __inout PYORI_STRING ExpandedSymbol
    )
{
    YoriLibLoadShell32Functions();
    if (DllShell32.pSHGetSpecialFolderPathW == NULL) {
        YoriLibLoadShfolderFunctions();
        if (DllShfolder.pSHGetFolderPathW == NULL) {
            return FALSE;
        }
    }

    if (!YoriLibAllocateString(ExpandedSymbol, MAX_PATH)) {
        return FALSE;
    }

    ExpandedSymbol->StartOfString[0] = '\0';

    if (DllShell32.pSHGetSpecialFolderPathW != NULL) {
        if (DllShell32.pSHGetSpecialFolderPathW(NULL, ExpandedSymbol->StartOfString, FolderType, FALSE) < 0) {
            YoriLibFreeStringContents(ExpandedSymbol);
            return FALSE;
        }
    } else {
        if (!SUCCEEDED(DllShfolder.pSHGetFolderPathW(NULL, FolderType, NULL, 0, ExpandedSymbol->StartOfString))) {
            YoriLibFreeStringContents(ExpandedSymbol);
            return FALSE;
        }
    }

    ExpandedSymbol->LengthInChars = _tcslen(ExpandedSymbol->StartOfString);

    return TRUE;
}

/**
 Expand a directory component in a path, specified via a tilde and description,
 into its corresponding physical directory.

 @param SymbolToExpand The tilde based directory reference.

 @param ExpandedSymbol On successful completion, populated with the directory
        corresponding to the reference.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibExpandHomeSymbol(
    __in PYORI_STRING SymbolToExpand,
    __inout PYORI_STRING ExpandedSymbol
    )
{
    if (YoriLibCompareStringWithLiteralInsensitive(SymbolToExpand, _T("~")) == 0) {
        if (!YoriLibAllocateString(ExpandedSymbol, GetEnvironmentVariable(_T("HOMEDRIVE"), NULL, 0) + GetEnvironmentVariable(_T("HOMEPATH"), NULL, 0))) {
            return FALSE;
        }

        ExpandedSymbol->LengthInChars = GetEnvironmentVariable(_T("HOMEDRIVE"), ExpandedSymbol->StartOfString, ExpandedSymbol->LengthAllocated);
        ExpandedSymbol->LengthInChars += GetEnvironmentVariable(_T("HOMEPATH"), &ExpandedSymbol->StartOfString[ExpandedSymbol->LengthInChars], ExpandedSymbol->LengthAllocated - ExpandedSymbol->LengthInChars);
        return TRUE;
    } else if (YoriLibCompareStringWithLiteralInsensitive(SymbolToExpand, _T("~APPDATA")) == 0) {
        return YoriLibExpandShellDirectory(CSIDL_APPDATA, ExpandedSymbol);
    } else if (YoriLibCompareStringWithLiteralInsensitive(SymbolToExpand, _T("~DESKTOP")) == 0) {
        return YoriLibExpandShellDirectory(CSIDL_DESKTOPDIRECTORY, ExpandedSymbol);
    } else if (YoriLibCompareStringWithLiteralInsensitive(SymbolToExpand, _T("~DOCUMENTS")) == 0) {
        return YoriLibExpandShellDirectory(CSIDL_PERSONAL, ExpandedSymbol);
    } else if (YoriLibCompareStringWithLiteralInsensitive(SymbolToExpand, _T("~DOWNLOADS")) == 0) {
        return YoriLibExpandShellDirectoryGuid(&FOLDERID_Downloads, ExpandedSymbol);
    } else if (YoriLibCompareStringWithLiteralInsensitive(SymbolToExpand, _T("~LOCALAPPDATA")) == 0) {
        return YoriLibExpandShellDirectory(CSIDL_LOCALAPPDATA, ExpandedSymbol);
    } else if (YoriLibCompareStringWithLiteralInsensitive(SymbolToExpand, _T("~PROGRAMS")) == 0) {
        return YoriLibExpandShellDirectory(CSIDL_PROGRAMS, ExpandedSymbol);
    } else if (YoriLibCompareStringWithLiteralInsensitive(SymbolToExpand, _T("~START")) == 0) {
        return YoriLibExpandShellDirectory(CSIDL_STARTMENU, ExpandedSymbol);
    } else if (YoriLibCompareStringWithLiteralInsensitive(SymbolToExpand, _T("~STARTUP")) == 0) {
        return YoriLibExpandShellDirectory(CSIDL_STARTUP, ExpandedSymbol);
    }

    ExpandedSymbol->StartOfString = SymbolToExpand->StartOfString;
    ExpandedSymbol->LengthInChars = SymbolToExpand->LengthInChars;
    return TRUE;
}

/**
 Expand all tilde based home references in a file path and return the
 expanded form.

 @param FileString A string specifying a path that may include tilde based
        directory references.

 @param ExpandedString On successful completion, contains the corresponding
        full path.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibExpandHomeDirectories(
    __in PYORI_STRING FileString,
    __inout PYORI_STRING ExpandedString
    )
{
    DWORD CharIndex;
    YORI_STRING BeforeSymbol;
    YORI_STRING AfterSymbol;
    YORI_STRING SymbolToExpand;
    YORI_STRING ExpandedSymbol;
    BOOL PreviousWasSeperator = TRUE;

    for (CharIndex = 0; CharIndex < FileString->LengthInChars; CharIndex++) {
        if (FileString->StartOfString[CharIndex] == '~' &&
            (CharIndex == 0 || YoriLibIsSep(FileString->StartOfString[CharIndex - 1]))) {

            YoriLibInitEmptyString(&BeforeSymbol);
            YoriLibInitEmptyString(&AfterSymbol);
            YoriLibInitEmptyString(&SymbolToExpand);

            BeforeSymbol.StartOfString = FileString->StartOfString;
            BeforeSymbol.LengthInChars = CharIndex;

            SymbolToExpand.StartOfString = &FileString->StartOfString[CharIndex];
            SymbolToExpand.LengthInChars = 0;

            while (CharIndex < FileString->LengthInChars && !YoriLibIsSep(FileString->StartOfString[CharIndex])) {
                SymbolToExpand.LengthInChars++;
                CharIndex++;
            }

            AfterSymbol.StartOfString = &FileString->StartOfString[CharIndex];
            AfterSymbol.LengthInChars = FileString->LengthInChars - CharIndex;

            YoriLibInitEmptyString(&ExpandedSymbol);

            if (!YoriLibExpandHomeSymbol(&SymbolToExpand, &ExpandedSymbol)) {
                return FALSE;
            }

            YoriLibInitEmptyString(ExpandedString);
            YoriLibYPrintf(ExpandedString, _T("%y%y%y"), &BeforeSymbol, &ExpandedSymbol, &AfterSymbol);
            YoriLibFreeStringContents(&ExpandedSymbol);
            if (ExpandedString->StartOfString != NULL) {
                return TRUE;
            }
            return FALSE;
        }

        if (YoriLibIsSep(FileString->StartOfString[CharIndex])) {
            PreviousWasSeperator = TRUE;
        } else {
            PreviousWasSeperator = FALSE;
        }
    }

    return FALSE;
}

/**
 Resolves a user string which must refer to a single file into a physical path
 for that file.

 @param UserString The string as specified by the user.  This is currently
        required to be NULL terminated.

 @param bReturnEscapedPath TRUE if the resulting path should be prefixed with
        \\?\, FALSE to indicate a traditional Win32 path.

 @param FullPath On successful completion, this pointer is updated to point to
        the full path string.  This string should be uninitialized on input and
        is allocated within this routine.  The caller is expected to free this
        with @ref YoriLibFreeStringContents.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriLibUserStringToSingleFilePath(
    __in PYORI_STRING UserString,
    __in BOOL bReturnEscapedPath,
    __inout PYORI_STRING FullPath
    )
{
    YORI_STRING PathToTranslate;
    YORI_STRING YsFilePrefix;
    YORI_STRING ExpandedString;
    BOOL ReturnValue = FALSE;

    //
    //  MSFIX This routine is assuming it gets NULL terminated YORI_STRINGs.
    //  The YORI_STRING is for the benefit of home directory expansion, but
    //  full path expansion is NULL terminated.  Sigh.
    //

    ASSERT(YoriLibIsStringNullTerminated(UserString));

    YoriLibInitEmptyString(&YsFilePrefix);
    YsFilePrefix.StartOfString = UserString->StartOfString;
    YsFilePrefix.LengthInChars = sizeof("file:///") - 1;

    YoriLibInitEmptyString(&PathToTranslate);
    if (YoriLibCompareStringWithLiteralInsensitive(&YsFilePrefix, _T("file:///")) == 0) {
        PathToTranslate.StartOfString = &UserString->StartOfString[YsFilePrefix.LengthInChars];
        PathToTranslate.LengthInChars = UserString->LengthInChars - YsFilePrefix.LengthInChars;
    } else {
        PathToTranslate.StartOfString = UserString->StartOfString;
        PathToTranslate.LengthInChars = UserString->LengthInChars;
    }

    YoriLibInitEmptyString(FullPath);

    if (YoriLibExpandHomeDirectories(&PathToTranslate, &ExpandedString)) {
        ReturnValue = YoriLibGetFullPathNameReturnAllocation(&ExpandedString, bReturnEscapedPath, FullPath, NULL);
        YoriLibFreeStringContents(&ExpandedString);
    } else {
        ReturnValue = YoriLibGetFullPathNameReturnAllocation(&PathToTranslate, bReturnEscapedPath, FullPath, NULL);
    }

    if (ReturnValue == 0) {
        YoriLibFreeStringContents(FullPath);
        return 0;
    }

    return ReturnValue;
}

/**
 Converts a path from \\?\ or \\.\ form into a regular, non-escaped form.
 This requires a reallocate for UNC paths, which insert a character in the
 beginning.

 @param Path The user specified path, presumably escaped.

 @param UnescapedPath The path to output.

 @return TRUE if the path could be converted successfully, FALSE if not.
 */
BOOL
YoriLibUnescapePath(
    __in PYORI_STRING Path,
    __inout PYORI_STRING UnescapedPath
    )
{
    BOOLEAN HasEscape = FALSE;
    BOOLEAN UncPath = FALSE;
    DWORD Offset;
    DWORD BufferLength;
    YORI_STRING SubsetToCopy;

    Offset = 0;

    //
    //  Check if the path is prefixed (and needs that removing.)
    //  Check if it's prefixed as \\?\UNC which needs to have that
    //  prefix removed but also have an extra backslash inserted.
    //

    if (Path->LengthInChars >= 4 &&
        YoriLibIsSep(Path->StartOfString[0]) &&
        YoriLibIsSep(Path->StartOfString[1]) &&
        (Path->StartOfString[2] == '?' ||
         Path->StartOfString[2] == '.' ) &&
        YoriLibIsSep(Path->StartOfString[3])) {

        HasEscape = TRUE;

        if (YoriLibIsFullPathUnc(Path)) {
            UncPath = TRUE;
            Offset = 7;
        } else {
            Offset = 4;
        }
    }

    //
    //  We need a buffer for the input string, minus the offset we're
    //  ignoring, plus a NULL.  A UNC path also needs an extra prefix
    //  backslash.
    //

    BufferLength = Path->LengthInChars - Offset + 1;
    if (UncPath) {
        BufferLength++;
    }

    if (UnescapedPath->LengthAllocated < BufferLength) {
        YoriLibFreeStringContents(UnescapedPath);
        if (!YoriLibAllocateString(UnescapedPath, BufferLength)) {
            return FALSE;
        }
    }

    memcpy(&SubsetToCopy, Path, sizeof(YORI_STRING));
    SubsetToCopy.StartOfString += Offset;
    SubsetToCopy.LengthInChars -= Offset;

    if (UncPath) {
        YoriLibSPrintfS(UnescapedPath->StartOfString, BufferLength, _T("\\%y"), &SubsetToCopy);
        UnescapedPath->LengthInChars = SubsetToCopy.LengthInChars + 1;
    } else {
        YoriLibSPrintfS(UnescapedPath->StartOfString, BufferLength, _T("%y"), &SubsetToCopy);
        UnescapedPath->LengthInChars = SubsetToCopy.LengthInChars;
    }

    return TRUE;
}


// vim:sw=4:ts=4:et:
