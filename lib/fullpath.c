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
TCHAR YoriLibPathPrefixChar;

/**
 Return TRUE if the path consists of a drive letter and colon, potentially
 followed by other characters.

 @param Path Pointer to the string to check.

 @return TRUE if the string starts with a drive letter and colon.
 */
BOOL
YoriLibIsDrvLetterColon(
    __in PCYORI_STRING Path
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
YoriLibIsDrvLetterColonSlash(
    __in PCYORI_STRING Path
    )
{
    if (Path->LengthInChars < 3) {
        return FALSE;
    }

    if (YoriLibIsDrvLetterColon(Path) &&
        YoriLibIsSep(Path->StartOfString[2])) {

        return TRUE;
    }

    return FALSE;
}

/**
 Return TRUE if the path contains the \\?\ prefix or equivalent.

 @param Path Pointer to the string to check.

 @return TRUE if the string starts with a prefix, FALSE otherwise.
 */
BOOL
YoriLibIsPathPrefixed(
    __in PCYORI_STRING Path
    )
{
    if (Path->LengthInChars < 4) {
        return FALSE;
    }

    if (Path->StartOfString[0] != '\\' ||
        Path->StartOfString[1] != '\\' ||
        (Path->StartOfString[2] != '?'  && Path->StartOfString[2] != '.') ||
        Path->StartOfString[3] != '\\') {

        return FALSE;
    }
    return TRUE;
}

/**
 Return TRUE if the path consists of a prefix, drive letter and colon,
 potentially followed by other characters.

 @param Path Pointer to the string to check.

 @return TRUE if the string starts with a prefix, drive letter and colon;
         FALSE otherwise.
 */
BOOL
YoriLibIsPfxDrvLetterColon(
    __in PCYORI_STRING Path
    )
{
    if (Path->LengthInChars < 6) {
        return FALSE;
    }

    if (!YoriLibIsPathPrefixed(Path)) {
        return FALSE;
    }

    if (((Path->StartOfString[4] >= 'A' && Path->StartOfString[4] <= 'Z') ||
         (Path->StartOfString[4] >= 'a' && Path->StartOfString[4] <= 'z')) &&
        Path->StartOfString[5] == ':') {

        return TRUE;
    }

    return FALSE;
}

/**
 Return TRUE if the path consists of a prefix, drive letter, colon, and path
 seperator, potentially followed by other characters.

 @param Path Pointer to the string to check.

 @return TRUE if the string starts with a prefix, drive letter, colon, and
         seperator; FALSE otherwise.
 */
BOOL
YoriLibIsPfxDrvLetterColonSlash(
    __in PCYORI_STRING Path
    )
{
    if (Path->LengthInChars < 7) {
        return FALSE;
    }

    if (!YoriLibIsPathPrefixed(Path)) {
        return FALSE;
    }

    if (((Path->StartOfString[4] >= 'A' && Path->StartOfString[4] <= 'Z') ||
         (Path->StartOfString[4] >= 'a' && Path->StartOfString[4] <= 'z')) &&
        Path->StartOfString[5] == ':' &&
        Path->StartOfString[6] == '\\') {

        return TRUE;
    }

    return FALSE;
}

#if YORI_UNC_SUPPORT


/**
 Return TRUE if the path is a UNC path.  This function assumes that the
 path must already have the \\?\ prefix.

 @param Path The path to check.

 @return TRUE to indicate a UNC path, FALSE to indicate that it is not a UNC
         path.
 */
BOOL
YoriLibIsFullPathUnc(
    __in PCYORI_STRING Path
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

#endif

/**
 Given a fully qualified path containing a drive letter or UNC root, determine
 where the root component is that cannot be traversed above.  The path may
 still contain .. components, it must have a parent directory specified
 explicitly.  Because this function is assuming a fully qualified path, it
 also assumes that forward slashes have been converted to backslashes by this
 point.

 @param Path Pointer to the fully qualified path which may have ..
        components remaining in it.

 @param PathHasPrefix TRUE if the path starts with a \\?\ or similar prefix.
        FALSE if it does not.

 @param PathIsUnc TRUE if the path is to a UNC share.  FALSE if it refers to
        a drive letter mapping.

 @param EffectiveRoot On successful completion, updated to point to the
        substring within Path that cannot be traversed above.

 @return TRUE to indicate a root was found, FALSE to indicate a parsing
         error.
 */
__success(return)
BOOL
YoriLibFindEffRootInternal(
    __in PYORI_STRING Path,
    __in BOOL PathHasPrefix,
#if YORI_UNC_SUPPORT
    __in BOOL PathIsUnc,
#endif
    __out PYORI_STRING EffectiveRoot
    )
{
    YORI_STRING Substring;
    LPTSTR StringPtr;

    YoriLibInitEmptyString(&Substring);

    YoriLibInitEmptyString(EffectiveRoot);
    EffectiveRoot->StartOfString = Path->StartOfString;

    if (PathHasPrefix) {

#if YORI_UNC_SUPPORT
        if (PathIsUnc) {

            ASSERT(Path->LengthInChars >= sizeof("\\\\?\\UNC\\") - 1);

            Substring.StartOfString = &Path->StartOfString[8];
            Substring.LengthInChars = Path->LengthInChars - 8;

            StringPtr = YoriLibFindLeftMostCharacter(&Substring, '\\');

            //
            //  If the path is \\server (no share), we can't handle it.
            //

            if (StringPtr == NULL) {
                return FALSE;
            }

            Substring.LengthInChars = Substring.LengthInChars - (YORI_ALLOC_SIZE_T)(StringPtr - Substring.StartOfString) - 1;
            Substring.StartOfString = &StringPtr[1];

            StringPtr = YoriLibFindLeftMostCharacter(&Substring, '\\');

            //
            //  If the path is \\?\UNC\server\share (exactly, no trailing
            //  component), it's as full as it'll ever get.  We should
            //  just return it here.
            //

        } else {
#endif

            //
            //  If a path has a prefix, it needs to have characters for one,
            //  and those are four characters in length.
            //

            ASSERT(Path->LengthInChars >= 4);

            Substring.StartOfString = &Path->StartOfString[4];
            Substring.LengthInChars = Path->LengthInChars - 4;

            StringPtr = YoriLibFindLeftMostCharacter(&Substring, '\\');

            //
            //  If we found a seperator, include it in the effective root.
            //

            if (StringPtr != NULL) {
                StringPtr++;
            }
#if YORI_UNC_SUPPORT
        }
#endif
    } else {
#if YORI_UNC_SUPPORT
        if (PathIsUnc) {

            ASSERT(Path->LengthInChars >= sizeof("\\\\") - 1);

            Substring.StartOfString = &Path->StartOfString[2];
            Substring.LengthInChars = Path->LengthInChars - 2;

            StringPtr = YoriLibFindLeftMostCharacter(&Substring, '\\');

            //
            //  If the path is \\server (no share), we can't handle it.
            //

            if (StringPtr == NULL) {
                return FALSE;
            }

            Substring.LengthInChars = Substring.LengthInChars - (YORI_ALLOC_SIZE_T)(StringPtr - Substring.StartOfString) - 1;
            Substring.StartOfString = &StringPtr[1];

            StringPtr = YoriLibFindLeftMostCharacter(&Substring, '\\');

            //
            //  If the path is \\server\share (exactly, no trailing
            //  component), it's as full as it'll ever get.  We should
            //  just return it here.
            //

        } else {
#endif

            StringPtr = YoriLibFindLeftMostCharacter(Path, '\\');

            //
            //  If we found a seperator, include it in the effective root.
            //

            if (StringPtr != NULL) {
                StringPtr++;
            }
#if YORI_UNC_SUPPORT
        }
#endif
    }

    if (StringPtr == NULL) {
        EffectiveRoot->LengthInChars = Path->LengthInChars;
        return TRUE;
    }

    EffectiveRoot->LengthInChars = (YORI_ALLOC_SIZE_T)(StringPtr - EffectiveRoot->StartOfString);
    return TRUE;
}

/**
 Given a fully qualified path containing a drive letter or UNC root, determine
 where the root component is that cannot be traversed above.  The path may
 still contain .. components, it must have a parent directory specified
 explicitly.  Because this function is assuming a fully qualified path, it
 also assumes that forward slashes have been converted to backslashes by this
 point.

 @param Path Pointer to the fully qualified path which may have ..
        components remaining in it.

 @param EffectiveRoot On successful completion, updated to point to the
        substring within Path that cannot be traversed above.

 @return TRUE to indicate a root was found, FALSE to indicate a parsing
         error.
 */
__success(return)
BOOL
YoriLibFindEffRoot(
    __in PYORI_STRING Path,
    __out PYORI_STRING EffectiveRoot
    )
{
    BOOL PrefixedPath = FALSE;
#if YORI_UNC_SUPPORT
    BOOL UncPath = FALSE;
#endif
    BOOL Result;

    if (YoriLibIsPathPrefixed(Path)) {
        PrefixedPath = TRUE;
#if YORI_UNC_SUPPORT
        if (YoriLibIsFullPathUnc(Path)) {
            UncPath = TRUE;
        }
    } else {
        if (Path->LengthInChars >= 2 &&
            Path->StartOfString[0] == '\\' &&
            Path->StartOfString[1] == '\\') {
            UncPath = TRUE;
        }
#endif
    }

    Result = YoriLibFindEffRootInternal(Path,
                                        PrefixedPath,
#if YORI_UNC_SUPPORT
                                        UncPath,
#endif
                                        EffectiveRoot);
    return Result;
}

/**
 A structure that can describe which type of relative or absolute path a 
 string refers to.
 */
typedef union _YORI_LIB_FULL_PATH_TYPE {

    /**
     All of the flags in this structure for simple initialization.
     */
    DWORD AllFlags;

    /**
     Individual flags components.
     */
    struct {

        /**
         If TRUE, a path which is relative to the current directory of a
         specified drive, such as "x:foo" .
         */
        BOOLEAN DriveRelativePath:1;

        /**
         If TRUE, a path which is relative to the current drive but specifies
         a full directory, such as "\foo".
         */
        BOOLEAN AbsoluteWithoutDrive:1;

        /**
         If TRUE, a path which is relative to the current drive and directory,
         such as "foo".
         */
        BOOLEAN RelativePath:1;

        /**
         If TRUE, the "\\?\" prefix is present in the path.
         */
        BOOLEAN PrefixPresent:1;

#if YORI_UNC_SUPPORT
        /**
         If TRUE, the "\\" UNC prefix, including the "\\?\UNC\" prefix, is
         present in a path.
         */
        BOOLEAN UncPath:1;
#endif
    } Flags;
} YORI_LIB_FULL_PATH_TYPE, *PYORI_LIB_FULL_PATH_TYPE;

//
//  Disable warning about using deprecated GetVersion.
//

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
#pragma warning(disable: 28159)
#endif

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

/**
 This function parses a path name to determine which type of path it is.
 There are four basic types of paths that need to be recognized:

   1. Absolute paths, starting with \\ or X:\.  We need to remove
      relative components from these.

   2. Drive relative paths (C:foo).  Here we need to merge with the
      current directory for the specified drive.

   3. Absolute paths without drive ("\foo").  Here we need to merge
      with the current drive letter.

   4. Relative paths, with no prefix.  These need to have current
      directory prepended.

 In addition to these four cases, a path can also be either a UNC path or
 a path to a drive letter.  The input may have a \\?\ escape or not, which
 changes some of the evaluation rules.

 @param FileName Pointer to the file name to parse.

 @param PathType On completion, populated with flags indicating the type of
        the path.

 @param StartOfRelativePath On successful completion, indicates the subset of
        the file name that is relative.  For a relative path, this is
        frequently the entire path, except for a drive relative path where it
        refers to the relative component without the drive letter which is
        absolute.  For an absolute path, it is meaningless.

 @return A win32 error code, or ERROR_SUCCESS to indicate successful
         completion.
 */
__success(return == ERROR_SUCCESS)
SYSERR
YoriLibGetFullPathPathType(
    __in PYORI_STRING FileName,
    __out PYORI_LIB_FULL_PATH_TYPE PathType,
    __out_opt PYORI_STRING StartOfRelativePath
    )
{
    PathType->AllFlags = 0;

    if (StartOfRelativePath != NULL) {
        YoriLibInitEmptyString(StartOfRelativePath);
        StartOfRelativePath->StartOfString = FileName->StartOfString;
        StartOfRelativePath->LengthInChars = FileName->LengthInChars;
    }

    //
    //  On NT 3.1, use \\.\ instead of \\?\ since the latter does not
    //  appear to be supported there
    //

    if (!YoriLibPathPrefixChar) {
        DWORD OsVer = GetVersion();

        YoriLibPathPrefixChar = '?';

        if (LOBYTE(LOWORD(OsVer)) < 4) {
            if (LOBYTE(LOWORD(OsVer)) == 3 && HIBYTE(LOWORD(OsVer)) < 50) {
                YoriLibPathPrefixChar = '.';
            }
        }
    }

    //
    //  First, determine which of the cases above we're processing.
    //

    if (FileName->LengthInChars >= 2 &&
        YoriLibIsSep(FileName->StartOfString[0]) &&
        YoriLibIsSep(FileName->StartOfString[1])) {

#if YORI_UNC_SUPPORT
        PathType->Flags.UncPath = TRUE;
#endif

        if (FileName->LengthInChars >= 4 &&
            (FileName->StartOfString[2] == '?' ||
             FileName->StartOfString[2] == '.' ) &&
            YoriLibIsSep(FileName->StartOfString[3])) {

            PathType->Flags.PrefixPresent = TRUE;

#if YORI_UNC_SUPPORT
            if (!YoriLibIsFullPathUnc(FileName)) {
                PathType->Flags.UncPath = FALSE;
            }
#endif
        }

    } else if (YoriLibIsDrvLetterColon(FileName)) {

        if (FileName->LengthInChars == 2 ||
            (FileName->LengthInChars >= 3 && !YoriLibIsSep(FileName->StartOfString[2]))) {

            PathType->Flags.DriveRelativePath = TRUE;
            if (StartOfRelativePath != NULL) {
                StartOfRelativePath->StartOfString += 2;
                StartOfRelativePath->LengthInChars -= 2;
            }
        }
    } else if (FileName->LengthInChars >= 1 && YoriLibIsSep(FileName->StartOfString[0])) {
        PathType->Flags.AbsoluteWithoutDrive = TRUE;
    } else {
        PathType->Flags.RelativePath = TRUE;
    }

    return ERROR_SUCCESS;
}


/**
 Combine a relative path with a primary path and return the result in a
 canonical form.  Note this function does not perform any squashing of
 relative components, it is a simple concatenation.

 @param PrimaryDirectory Pointer to the primary (often current) directory.

 @param RelativePath Pointer to the relative path to merge with the primary
        directory.

 @param ReturnEscapedPath If TRUE, the resulting path should be in \\?\ form.
        If FALSE, it should be a conventional Win32 path.

 @param PathType Pointer to information specifying the type of the path. On
        output, the UncPath member may be updated to conform to the type of
        PrimaryDirectory.

 @param Buffer An initialized buffer to populate with the concatenated string.
        This can be reallocated in this function if it is insufficient to
        hold the result; if this occurs, FreeOnFailure is set to TRUE.

 @param FreeOnFailure Pointer to a boolean to be set to TRUE if the Buffer
        was reallocated in this function.  If this occurs, later failure will
        cause the reallocated buffer to be freed.  This allows a caller to use
        an initialized but not allocated string and only receive an allocated
        string on success.

 @return A win32 error code, or ERROR_SUCCESS to indicate successful
         completion.
 */
SYSERR
YoriLibFullPathMergeRootWithRel(
    __in PYORI_STRING PrimaryDirectory,
    __in PYORI_STRING RelativePath,
    __in BOOL ReturnEscapedPath,
    __inout PYORI_LIB_FULL_PATH_TYPE PathType,
    __inout PYORI_STRING Buffer,
    __inout PBOOLEAN FreeOnFailure
    )
{
    YORI_STRING CurrentDirectory;
    YORI_ALLOC_SIZE_T Result;

    //
    //  This function is likely to want a substring of the primary directory,
    //  so take a copy of the string range so it can be manipulated.
    //

    CurrentDirectory.StartOfString = PrimaryDirectory->StartOfString;
    CurrentDirectory.LengthInChars = PrimaryDirectory->LengthInChars;
    CurrentDirectory.LengthAllocated = 0;
    CurrentDirectory.MemoryToFree = NULL;

    //
    //  Check if it's already escaped, and whether or not it's
    //  UNC.  If it's UNC, truncate the beginning so that it's
    //  pointing to a single backslash only, regardless of whether
    //  it was initially escaped or not.
    //
    //  If it's neither, assume it was a normal, boring Win32 form,
    //  such as "C:\Foo".
    //

    if (CurrentDirectory.LengthInChars >= 4 &&
        CurrentDirectory.StartOfString[0] == '\\' &&
        CurrentDirectory.StartOfString[1] == '\\' &&
        (CurrentDirectory.StartOfString[2] == '?' || CurrentDirectory.StartOfString[2] == '.') &&
        CurrentDirectory.StartOfString[3] == '\\') {

#if YORI_UNC_SUPPORT
        if (YoriLibIsFullPathUnc(&CurrentDirectory)) {
            CurrentDirectory.StartOfString += 7;
            CurrentDirectory.LengthInChars -= 7;
            PathType->Flags.UncPath = TRUE;
        } else {
#endif
            CurrentDirectory.StartOfString += 4;
            CurrentDirectory.LengthInChars -= 4;
#if YORI_UNC_SUPPORT
        }
    } else {

        if (CurrentDirectory.LengthInChars >= 2 &&
            YoriLibIsSep(CurrentDirectory.StartOfString[0]) &&
            YoriLibIsSep(CurrentDirectory.StartOfString[1])) {

            CurrentDirectory.StartOfString += 1;
            CurrentDirectory.LengthInChars -= 1;
            PathType->Flags.UncPath = TRUE;
        }
#endif
    }

    //
    //  Just truncate the current directory to two chars and let
    //  normal processing continue.
    //

    if (PathType->Flags.AbsoluteWithoutDrive) {
        if (CurrentDirectory.LengthInChars > 2) {
#if YORI_UNC_SUPPORT
            if (PathType->Flags.UncPath) {
                YORI_STRING CurrentDirectorySubstring;
                LPTSTR Slash;
                YoriLibInitEmptyString(&CurrentDirectorySubstring);

                //
                //  For a UNC path, we just chopped off the first slash
                //  or prefix so it's currently \server\share.  Skip
                //  the first slash; length was checked above.
                //

                CurrentDirectorySubstring.StartOfString = &CurrentDirectory.StartOfString[1];
                CurrentDirectorySubstring.LengthInChars = CurrentDirectory.LengthInChars - 1;

                Slash = YoriLibFindLeftMostCharacter(&CurrentDirectorySubstring, '\\');
                if (Slash != NULL) {
                    CurrentDirectorySubstring.StartOfString = Slash + 1;
                    CurrentDirectorySubstring.LengthInChars = CurrentDirectory.LengthInChars - (YORI_ALLOC_SIZE_T)(Slash - CurrentDirectory.StartOfString) - 1;
                    Slash = YoriLibFindLeftMostCharacter(&CurrentDirectorySubstring, '\\');
                    if (Slash != NULL) {
                        CurrentDirectory.LengthInChars = (YORI_ALLOC_SIZE_T)(Slash - CurrentDirectory.StartOfString);
                        CurrentDirectory.StartOfString[CurrentDirectory.LengthInChars] = '\0';
                    }
                }
            } else {
#endif

                //
                //  If it's a drive letter path, just truncate the string
                //  to where the drive letter should be.
                //

                CurrentDirectory.LengthInChars = 2;
#if YORI_UNC_SUPPORT
            }
#endif
        }
    }

    //
    //  Calculate the length of the "absolute" form of this path and
    //  generate it through concatenation.  This is the prefix, the
    //  current directory, a seperator, and the relative path.  Note
    //  the current directory length includes space for a NULL.
    //

    if (ReturnEscapedPath) {
        Result = 4 + CurrentDirectory.LengthInChars + 1 + RelativePath->LengthInChars + 1;
#if YORI_UNC_SUPPORT
        if (PathType->Flags.UncPath) {
            Result += 4; // Remove "\" and add "\UNC\"
        }
#endif
    } else {
        Result = CurrentDirectory.LengthInChars + 1 + RelativePath->LengthInChars + 1;
#if YORI_UNC_SUPPORT
        if (PathType->Flags.UncPath) {
            Result += 1;
        }
#endif
    }

    if (Result > Buffer->LengthAllocated) {
        YoriLibFreeStringContents(Buffer);
        if (!YoriLibAllocateString(Buffer, Result)) {
            YoriLibFreeStringContents(&CurrentDirectory);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        *FreeOnFailure = TRUE;
    }

    //
    //  Enter the matrix.
    //
    //  Return escaped path *
    //  Source is already UNC *
    //  Is there a file or are we just getting current directory (x:).
    //

    if (ReturnEscapedPath) {
#if YORI_UNC_SUPPORT
        if (PathType->Flags.UncPath) {
            if (RelativePath->LengthInChars > 0) {
                Buffer->LengthInChars =
                    YoriLibSPrintfS(Buffer->StartOfString,
                                    Buffer->LengthAllocated,
                                    _T("\\\\%c\\UNC%y\\%y"),
                                    YoriLibPathPrefixChar,
                                    &CurrentDirectory,
                                    RelativePath);
            } else {
                Buffer->LengthInChars =
                    YoriLibSPrintfS(Buffer->StartOfString,
                                    Buffer->LengthAllocated,
                                    _T("\\\\%c\\UNC%y"),
                                    YoriLibPathPrefixChar,
                                    &CurrentDirectory);
            }
        } else {
#endif
            if (RelativePath->LengthInChars > 0) {
                Buffer->LengthInChars =
                    YoriLibSPrintfS(Buffer->StartOfString,
                                    Buffer->LengthAllocated,
                                    _T("\\\\%c\\%y\\%y"),
                                    YoriLibPathPrefixChar,
                                    &CurrentDirectory,
                                    RelativePath);
            } else {
                Buffer->LengthInChars =
                    YoriLibSPrintfS(Buffer->StartOfString,
                                    Buffer->LengthAllocated,
                                    _T("\\\\%c\\%y"),
                                    YoriLibPathPrefixChar,
                                    &CurrentDirectory);
            }
#if YORI_UNC_SUPPORT
        }
#endif
    } else {
#if YORI_UNC_SUPPORT
        if (PathType->Flags.UncPath) {
            if (RelativePath->LengthInChars > 0) {
                Buffer->LengthInChars =
                    YoriLibSPrintfS(Buffer->StartOfString,
                                    Buffer->LengthAllocated,
                                    _T("\\%y\\%y"),
                                    &CurrentDirectory,
                                    RelativePath);
            } else {
                Buffer->LengthInChars =
                    YoriLibSPrintfS(Buffer->StartOfString,
                                    Buffer->LengthAllocated,
                                    _T("\\%y"),
                                    &CurrentDirectory);
            }
        } else {
#endif
            if (RelativePath->LengthInChars > 0) {
                Buffer->LengthInChars =
                    YoriLibSPrintfS(Buffer->StartOfString,
                                    Buffer->LengthAllocated,
                                    _T("%y\\%y"),
                                    &CurrentDirectory,
                                    RelativePath);
            } else {
                Buffer->LengthInChars =
                    YoriLibSPrintfS(Buffer->StartOfString,
                                    Buffer->LengthAllocated,
                                    _T("%y"),
                                    &CurrentDirectory);
            }
#if YORI_UNC_SUPPORT
        }
#endif
    }

    return ERROR_SUCCESS;
}

/**
 Convert a full path into the requested form.  This function can take as
 input a full path that is either prefixed with "\\?\" or not, and is either
 UNC or not, and emit the appropriate value given whether the result should
 include a "\\?\" prefix.

 @param FileName Pointer to a fully specified file name.

 @param ReturnEscapedPath If TRUE, the resulting path should have a "\\?\"
        prefix.  If FALSE, it should be a conventional Win32 path.

 @param PathType Specifies the type of the path in FileName, including
        whether it currently has a "\\?\" prefix, and whether it is currently
        a UNC path.

 @param Buffer An initialized buffer to populate with the concatenated string.
        This can be reallocated in this function if it is insufficient to
        hold the result; if this occurs, FreeOnFailure is set to TRUE.

 @param FreeOnFailure Pointer to a boolean to be set to TRUE if the Buffer
        was reallocated in this function.  If this occurs, later failure will
        cause the reallocated buffer to be freed.  This allows a caller to use
        an initialized but not allocated string and only receive an allocated
        string on success.

 @return A win32 error code, or ERROR_SUCCESS to indicate successful
         completion.
 */
SYSERR
YoriLibFullPathNormalize(
    __in PYORI_STRING FileName,
    __in BOOL ReturnEscapedPath,
    __inout PYORI_LIB_FULL_PATH_TYPE PathType,
    __inout PYORI_STRING Buffer,
    __inout PBOOLEAN FreeOnFailure
    )
{
    YORI_ALLOC_SIZE_T Result;
    YORI_STRING Subset;

    //
    //  Allocate a buffer for the "absolute" path so we can do compaction.
    //  If the path is UNC, convert to \\?\UNC\...
    //

    if (ReturnEscapedPath) {
        Result = FileName->LengthInChars + 1 + 4;
#if YORI_UNC_SUPPORT
        if (!PathType->Flags.PrefixPresent && PathType->Flags.UncPath) {
            Result += 4;
        }
#endif
    } else {
        Result = FileName->LengthInChars + 1;
    }

    if (Result > Buffer->LengthAllocated) {
        YoriLibFreeStringContents(Buffer);
        if (!YoriLibAllocateString(Buffer, Result)) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        *FreeOnFailure = TRUE;
    }

    Subset.StartOfString = FileName->StartOfString;
    Subset.LengthInChars = FileName->LengthInChars;
    Subset.LengthAllocated = 0;
    Subset.MemoryToFree = NULL;

#if YORI_UNC_SUPPORT
    if (ReturnEscapedPath) {
        if (!PathType->Flags.PrefixPresent && PathType->Flags.UncPath) {
            //
            //  Input string is \\server\share, output is
            //  \\?\UNC\server\share, chop off the first two slashes
            //

            Subset.StartOfString += 2;
            Subset.LengthInChars -= 2;
        }
    } else {
#endif
        if (PathType->Flags.PrefixPresent) {
#if YORI_UNC_SUPPORT
            if (PathType->Flags.UncPath) {

                //
                //  Input string is \\?\UNC\server\share, output is
                //  \\server\share, chop off \\?\UNC
                //

                Subset.StartOfString += 7;
                Subset.LengthInChars -= 7;
            } else {
#endif

                //
                //  Input string is \\?\C:\, output is C:\, chop off
                //  first four chars
                //

                Subset.StartOfString += 4;
                Subset.LengthInChars -= 4;
#if YORI_UNC_SUPPORT
            }
        }
#endif
    }

    if (ReturnEscapedPath) {

        if (PathType->Flags.PrefixPresent) {

            Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("%y"), &Subset);
#if YORI_UNC_SUPPORT
        } else if (PathType->Flags.UncPath) {
            Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\\\%c\\UNC\\%y"), YoriLibPathPrefixChar, &Subset);
        } else {
#endif

            Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\\\%c\\%y"), YoriLibPathPrefixChar, &Subset);
        }
    } else {
        if (PathType->Flags.PrefixPresent) {
#if YORI_UNC_SUPPORT
            if (PathType->Flags.UncPath) {
                Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("\\%y"), &Subset);
            } else {
#endif
                Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("%y"), &Subset);
#if YORI_UNC_SUPPORT
            }
#endif
        } else {
            Buffer->LengthInChars = YoriLibSPrintfS(Buffer->StartOfString, Buffer->LengthAllocated, _T("%y"), &Subset);
        }
    }

    return ERROR_SUCCESS;
}

/**
 Take a combined string that contains a full path, convert all forward slashes
 to backslashes, remove any "\.\" components, remove any "\blah\..\"
 components up to the effective root of the path, and optionally return the
 part of the path that contains the final file component.

 @param Buffer Pointer to a string which contains a full path that may still
        have . or .. components.

 @param PathType Pointer to information describing the type of path.

 @param ReturnEscapedPath If TRUE, the path to return should be in a "\\?\"
        prefixed form.

 @param lpFilePart If specified, on successful completion, updated to point
        to the beginning of the file name component of the path, in the same
        allocation as lpBuffer.

 @return A win32 error code, including ERROR_SUCCESS to indicate successful
         completion.
 */
__success(return == ERROR_SUCCESS)
SYSERR
YoriLibGetFullPathSquashRelCmp(
    __inout PYORI_STRING Buffer,
#if YORI_UNC_SUPPORT
    __in PYORI_LIB_FULL_PATH_TYPE PathType,
#endif
    __in BOOL ReturnEscapedPath,
    __deref_opt_out_opt LPTSTR* lpFilePart
    )
{

    SYSERR Result;
    LPTSTR EffectiveRoot;
    YORI_STRING EffectiveRootSubstring;
    LPTSTR CurrentReadChar;
    LPTSTR CurrentWriteChar;
    BOOLEAN PreviousWasSeperator;

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

#if YORI_UNC_SUPPORT
    if (ReturnEscapedPath) {
        if (YoriLibIsFullPathUnc(Buffer)) {
            PathType->Flags.UncPath = TRUE;
        } else {
            PathType->Flags.UncPath = FALSE;
        }
    } else {
        if (Buffer->StartOfString[0] == '\\' && Buffer->StartOfString[1] == '\\') {
            PathType->Flags.UncPath = TRUE;
        } else {
            PathType->Flags.UncPath = FALSE;
        }
    }
#endif

    if (!YoriLibFindEffRootInternal(Buffer,
                                    ReturnEscapedPath,
#if YORI_UNC_SUPPORT
                                    PathType->Flags.UncPath,
#endif
                                    &EffectiveRootSubstring)) {
        return ERROR_BAD_PATHNAME;
    }

    //
    //  If the root is the whole string, there are no more operations we
    //  can perform, so return.
    //

    if (EffectiveRootSubstring.LengthInChars == Buffer->LengthInChars) {
        if (lpFilePart != NULL) {
            *lpFilePart = NULL;
        }
        ASSERT(Buffer->StartOfString[Buffer->LengthInChars] == '\0');
        return ERROR_SUCCESS;
    }

    //
    //  Check if the effective root ends with a backslash or not.
    //

    PreviousWasSeperator = FALSE;
    if (EffectiveRootSubstring.LengthInChars > 0 &&
        EffectiveRootSubstring.StartOfString[EffectiveRootSubstring.LengthInChars - 1] == '\\') {

        PreviousWasSeperator = TRUE;
    }

    EffectiveRoot = &EffectiveRootSubstring.StartOfString[EffectiveRootSubstring.LengthInChars];

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
        //  Now that . and .. components are processed, if this is not an
        //  escaped path, trim any trailing periods or spaces from the
        //  component.  Currently this loops forward on these characters
        //  to see if they're followed by a seperator.  This isn't
        //  computationally ideal, but the risk of long strings of these
        //  characters not followed by a terminator doesn't seem likely.
        //

        if (!ReturnEscapedPath) {
            LPTSTR TestReadChar;
            TestReadChar = CurrentReadChar;
            while (*TestReadChar == '.' || *TestReadChar == ' ') {
                TestReadChar++;
            }
            if (TestReadChar > CurrentReadChar &&
                (*TestReadChar == '\\' || *TestReadChar == '\0')) {

                //
                //  If the whole component was truncated, remove the
                //  previous seperator.
                //

                if (PreviousWasSeperator && CurrentWriteChar > EffectiveRoot) {
                    CurrentWriteChar--;
                    PreviousWasSeperator = FALSE;
                }

                CurrentReadChar = TestReadChar;

                if (*TestReadChar == '\0') {
                    break;
                }
            }
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
    Buffer->LengthInChars = (YORI_ALLOC_SIZE_T)(CurrentWriteChar - Buffer->StartOfString);
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

    return ERROR_SUCCESS;
}

/**
 Private implementation of GetFullPathName.  This function will convert paths
 into their \\?\ (MAX_PATH exceeding) form, applying all of the necessary
 transformations.  This function will update a YORI_STRING, including
 allocating if necessary, to contain a buffer containing the path.  If this
 function allocates the buffer, the caller is expected to free this by calling
 @ref YoriLibFreeStringContents.

 @param FileName The file name to resolve to a full path form.

 @param ReturnEscapedPath If TRUE, the returned path is in \\?\ form.
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
__success(return)
BOOL
YoriLibGetFullPathNameAlloc(
    __in PYORI_STRING FileName,
    __in BOOL ReturnEscapedPath,
    __inout PYORI_STRING Buffer,
    __deref_opt_out_opt LPTSTR* lpFilePart
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
    //  Regardless of the case above, this function should flatten out
    //  any "." or ".." components in the path.
    //

    YORI_LIB_FULL_PATH_TYPE PathType;
    BOOLEAN FreeOnFailure = FALSE;

    YORI_STRING StartOfRelativePath;

    SYSERR Result;

    Result = YoriLibGetFullPathPathType(FileName,
                                        &PathType,
                                        &StartOfRelativePath);
    if (Result != ERROR_SUCCESS) {
        SetLastError(Result);
        return FALSE;
    }

    //
    //  If it's a relative case, get the current directory, and generate an
    //  "absolute" form of the name.  If it's an absolute case, prepend
    //  \\?\ and have a buffer we allocate for subsequent munging.
    //

    if (PathType.Flags.DriveRelativePath ||
        PathType.Flags.RelativePath ||
        PathType.Flags.AbsoluteWithoutDrive) {

        YORI_STRING CurrentDirectory;
        YORI_ALLOC_SIZE_T CurrentDirectoryLength;

        CurrentDirectoryLength = (YORI_ALLOC_SIZE_T)GetCurrentDirectory(0, NULL);
        if (!YoriLibAllocateString(&CurrentDirectory, CurrentDirectoryLength)) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        Result = GetCurrentDirectory(CurrentDirectory.LengthAllocated, CurrentDirectory.StartOfString);
        if (Result == 0 || Result >= CurrentDirectory.LengthAllocated) {
            YoriLibFreeStringContents(&CurrentDirectory);
            return FALSE;
        }

        CurrentDirectory.LengthInChars = (YORI_ALLOC_SIZE_T)Result;

        //
        //  If it's drive relative, and it's relative to a different drive,
        //  get the current directory of the requested drive.
        //

        if (PathType.Flags.DriveRelativePath) {
            if (CurrentDirectory.LengthInChars == 0 ||
                YoriLibUpcaseChar(CurrentDirectory.StartOfString[0]) != YoriLibUpcaseChar(FileName->StartOfString[0])) {

                YoriLibFreeStringContents(&CurrentDirectory);
                if (!YoriLibGetCurDirOnDrive(FileName->StartOfString[0], &CurrentDirectory)) {
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }
            }
        }

        Result = YoriLibFullPathMergeRootWithRel(&CurrentDirectory,
                                                 &StartOfRelativePath,
                                                 ReturnEscapedPath,
                                                 &PathType,
                                                 Buffer,
                                                 &FreeOnFailure);
        YoriLibFreeStringContents(&CurrentDirectory);

    } else {

        Result = YoriLibFullPathNormalize(FileName,
                                          ReturnEscapedPath,
                                          &PathType,
                                          Buffer,
                                          &FreeOnFailure);

    }

    if (Result != ERROR_SUCCESS) {
        if (FreeOnFailure) {
            YoriLibFreeStringContents(Buffer);
        }
        SetLastError(Result);
        return FALSE;
    }

    Result = YoriLibGetFullPathSquashRelCmp(Buffer,
#if YORI_UNC_SUPPORT
                                            &PathType,
#endif
                                            ReturnEscapedPath,
                                            lpFilePart);
    if (Result != ERROR_SUCCESS) {
        if (FreeOnFailure) {
            YoriLibFreeStringContents(Buffer);
        }
        SetLastError(Result);
        return FALSE;
    }

    SetLastError(0);
    return TRUE;
}

/**
 GetFullPathName where the "current" directory is specified.  Note that this
 version cannot traverse across drives without looking at the current
 directory for each drive, which the function does not take as input, so
 traversing drives is only possible with a fully formed path.
 This function will update a YORI_STRING, including allocating if necessary,
 to contain a buffer containing the path.  If this function allocates the
 buffer, the caller is expected to free this by calling
 @ref YoriLibFreeStringContents.

 @param PrimaryDirectory The directory to use as a "current" directory.

 @param FileName A file name, which may be fully specified or may be relative
        to PrimaryDirectory.

 @param ReturnEscapedPath If TRUE, the returned path is in \\?\ form.
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
__success(return)
BOOL
YoriLibGetFullPathNameRelTo(
    __in PYORI_STRING PrimaryDirectory,
    __in PYORI_STRING FileName,
    __in BOOL ReturnEscapedPath,
    __inout PYORI_STRING Buffer,
    __deref_opt_out_opt LPTSTR* lpFilePart
    )
{
    YORI_LIB_FULL_PATH_TYPE PathType;
    BOOLEAN FreeOnFailure = FALSE;

    YORI_STRING StartOfRelativePath;

    SYSERR Result;

    Result = YoriLibGetFullPathPathType(FileName,
                                        &PathType,
                                        &StartOfRelativePath);
    if (Result != ERROR_SUCCESS) {
        SetLastError(Result);
        return FALSE;
    }

    //
    //  If it's a relative case, get the current directory, and generate an
    //  "absolute" form of the name.  If it's an absolute case, prepend
    //  \\?\ and have a buffer we allocate for subsequent munging.
    //

    if (PathType.Flags.DriveRelativePath) {
        SetLastError(ERROR_BAD_PATHNAME);
        return FALSE;
    } else if (PathType.Flags.RelativePath ||
               PathType.Flags.AbsoluteWithoutDrive) {

        YORI_LIB_FULL_PATH_TYPE PrimaryDirPathType;
        Result = YoriLibGetFullPathPathType(PrimaryDirectory,
                                            &PrimaryDirPathType,
                                            NULL);
        if (Result != ERROR_SUCCESS) {
            SetLastError(Result);
            return FALSE;
        }

        //
        //  This function can handle a drive letter or UNC primary directory
        //  with and without an escape prefix, but it must still be a fully
        //  specified directory.
        //

        if (PrimaryDirPathType.Flags.RelativePath ||
            PrimaryDirPathType.Flags.AbsoluteWithoutDrive ||
            PrimaryDirPathType.Flags.DriveRelativePath) {

            SetLastError(ERROR_BAD_PATHNAME);
            return FALSE;
        }

        Result = YoriLibFullPathMergeRootWithRel(PrimaryDirectory,
                                                 &StartOfRelativePath,
                                                 ReturnEscapedPath,
                                                 &PathType,
                                                 Buffer,
                                                 &FreeOnFailure);
    } else {

        Result = YoriLibFullPathNormalize(FileName,
                                          ReturnEscapedPath,
                                          &PathType,
                                          Buffer,
                                          &FreeOnFailure);

    }

    if (Result != ERROR_SUCCESS) {
        if (FreeOnFailure) {
            YoriLibFreeStringContents(Buffer);
        }
        SetLastError(Result);
        return FALSE;
    }

    Result = YoriLibGetFullPathSquashRelCmp(Buffer,
#if YORI_UNC_SUPPORT
                                            &PathType,
#endif
                                            ReturnEscapedPath,
                                            lpFilePart);
    if (Result != ERROR_SUCCESS) {
        if (FreeOnFailure) {
            YoriLibFreeStringContents(Buffer);
        }
        SetLastError(Result);
        return FALSE;
    }

    SetLastError(0);
    return TRUE;
}

/**
 Converts a path from \\?\ or \\.\ form into a regular, non-escaped form.
 This requires a reallocate for UNC paths, which insert a character in the
 beginning.

 @param Path The user specified path, presumably escaped.

 @param UnescapedPath The path to output.

 @return TRUE if the path could be converted successfully, FALSE if not.
 */
__success(return)
BOOL
YoriLibUnescapePath(
    __in PYORI_STRING Path,
    __inout PYORI_STRING UnescapedPath
    )
{
    BOOLEAN HasEscape = FALSE;
#if YORI_UNC_SUPPORT
    BOOLEAN UncPath = FALSE;
#endif
    YORI_ALLOC_SIZE_T Offset;
    YORI_ALLOC_SIZE_T BufferLength;
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

#if YORI_UNC_SUPPORT
        if (YoriLibIsFullPathUnc(Path)) {
            UncPath = TRUE;
            Offset = 7;
        } else {
#endif
            Offset = 4;
#if YORI_UNC_SUPPORT
        }
#endif
    }

    //
    //  We need a buffer for the input string, minus the offset we're
    //  ignoring, plus a NULL.  A UNC path also needs an extra prefix
    //  backslash.
    //

    BufferLength = Path->LengthInChars - Offset + 1;
#if YORI_UNC_SUPPORT
    if (UncPath) {
        BufferLength++;
    }
#endif

    if (UnescapedPath->LengthAllocated < BufferLength) {
        YoriLibFreeStringContents(UnescapedPath);
        if (!YoriLibAllocateString(UnescapedPath, BufferLength)) {
            return FALSE;
        }
    }

    memcpy(&SubsetToCopy, Path, sizeof(YORI_STRING));
    SubsetToCopy.StartOfString += Offset;
    SubsetToCopy.LengthInChars = SubsetToCopy.LengthInChars - Offset;

#if YORI_UNC_SUPPORT
    if (UncPath) {
        YoriLibSPrintfS(UnescapedPath->StartOfString, BufferLength, _T("\\%y"), &SubsetToCopy);
        UnescapedPath->LengthInChars = SubsetToCopy.LengthInChars + 1;
    } else {
#endif
        YoriLibSPrintfS(UnescapedPath->StartOfString, BufferLength, _T("%y"), &SubsetToCopy);
        UnescapedPath->LengthInChars = SubsetToCopy.LengthInChars;
#if YORI_UNC_SUPPORT
    }
#endif

    return TRUE;
}

// vim:sw=4:ts=4:et:
