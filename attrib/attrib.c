/**
 * @file attrib/attrib.c
 *
 * Yori shell display and manipulate file attributes
 *
 * Copyright (c) 2021 Malcolm J. Smith
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

#include <yoripch.h>
#include <yorilib.h>

/**
 Help text to display to the user.
 */
const
CHAR strAttribHelpText[] =
        "\n"
        "Display or manipulate file attributes.\n"
        "\n"
        "ATTRIB [/license] [+Attrs] [-Attrs] [/b] [/d] [/s] [/v] [<file>...]\n"
        "\n"
        "   /b             Use basic search criteria for files only\n"
        "   /d             Include directories as well as files\n"
        "   /s             Process files from all subdirectories\n"
        "   /v             Verbose output\n"
        "\n";


/**
 A structure to map a 32 bit flag value to a character to input or output
 when describing the flag to humans.  The character is expected to be
 unique to allow input by character to function.
 */
typedef struct _ATTRIB_CHAR_TO_DWORD_FLAG {

    /**
     The flag in native representation.
     */
    DWORD Flag;

    /**
     The character to display to the user.
     */
    TCHAR DisplayLetter;

    /**
     Unused in order to ensure structure alignment.
     */
    WORD  AlignmentPadding;

    /**
     Help text for the flag.
     */
    LPCSTR HelpText;

} ATTRIB_CHAR_TO_DWORD_FLAG, *PATTRIB_CHAR_TO_DWORD_FLAG;

/**
 A table that maps file attribute flags as returned by the system to character
 representations used in UI or specified by the user.  The order in this table
 corresponds to the order that flags will be displayed in on query.
 */
const ATTRIB_CHAR_TO_DWORD_FLAG
AttribFileAttrPairs[] = {
    {FILE_ATTRIBUTE_ARCHIVE,             'A', 0, "Archive file attribute"},
    {FILE_ATTRIBUTE_SYSTEM,              'S', 0, "System file attribute"},
    {FILE_ATTRIBUTE_HIDDEN,              'H', 0, "Hidden file attribute"},
    {FILE_ATTRIBUTE_READONLY,            'R', 0, "Read-only file attribute"},
    {FILE_ATTRIBUTE_OFFLINE,             'O', 0, "Offline file attribute"},
    {FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, 'I', 0, "Not content indexed file attribute"},
    {FILE_ATTRIBUTE_NO_SCRUB_DATA,       'X', 0, "No scrub file attribute"},
    {FILE_ATTRIBUTE_INTEGRITY_STREAM,    'V', 0, "Integrity attribute"},
    {FILE_ATTRIBUTE_PINNED,              'P', 0, "Pinned attribute"},
    {FILE_ATTRIBUTE_UNPINNED,            'U', 0, "Unpinned attribute"},
    {FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL, 'B', 0, "SMR Blob attribute"},
    };


/**
 Display usage text to the user.
 */
BOOL
AttribHelp(VOID)
{
    DWORD Index;
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Attrib %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strAttribHelpText);

    for (Index = 0; Index < sizeof(AttribFileAttrPairs)/sizeof(AttribFileAttrPairs[0]); Index++) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("   %c              %hs\n"), AttribFileAttrPairs[Index].DisplayLetter, AttribFileAttrPairs[Index].HelpText);
    }

    return TRUE;
}

/**
 Context passed for each file found.
 */
typedef struct _ATTRIB_CONTEXT {

    /**
     TRUE if enumeration is recursive, FALSE if it is within one directory
     only.
     */
    BOOLEAN Recursive;

    /**
     TRUE if directories should be included on wildcard matches.  FALSE if
     only files should be included.
     */
    BOOLEAN IncludeDirectories;

    /**
     TRUE if output should be generated for each file processed.  FALSE for
     silent processing.
     */
    BOOLEAN Verbose;

    /**
     The first error encountered when enumerating objects from a single arg.
     This is used to preserve file not found/path not found errors so that
     when the program falls back to interpreting the argument as a literal,
     if that still doesn't work, this is the error code that is displayed.
     */
    DWORD SavedErrorThisArg;

    /**
     The set of attribute flags to set on each matching file.
     */
    DWORD AttributesToSet;

    /**
     The set of attribute flags to clear on each matching file.
     */
    DWORD AttributesToClear;

    /**
     A string to record the unescaped path to display for files.  This is
     kept here so the allocation can be reused for later files.
     */
    YORI_STRING UnescapedPath;

    /**
     Records the total number of files processed within a single command line
     argument.
     */
    DWORDLONG FilesFoundThisArg;

    /**
     Records the total number of files processed.
     */
    DWORDLONG FilesFound;

} ATTRIB_CONTEXT, *PATTRIB_CONTEXT;

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param Context Pointer to the attrib context structure indicating the action
        to perform and populated with the file found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
AttribFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PATTRIB_CONTEXT AttribContext = (PATTRIB_CONTEXT)Context;
    DWORD LastError;
    LPTSTR ErrText;
    DWORD ExistingAttributes;
    DWORD NewAttributes;

    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FileInfo);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    AttribContext->FilesFoundThisArg++;

    ExistingAttributes = GetFileAttributes(FilePath->StartOfString);
    if (ExistingAttributes == (DWORD)-1) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("attrib: query of attributes failed: %y %s"), FilePath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return TRUE;
    }

    if (AttribContext->AttributesToSet == 0 &&
        AttribContext->AttributesToClear == 0) {

        TCHAR StrAtts[sizeof(AttribFileAttrPairs)/sizeof(AttribFileAttrPairs[0]) + 1];
        DWORD Index;

        for (Index = 0; Index < sizeof(AttribFileAttrPairs)/sizeof(AttribFileAttrPairs[0]); Index++) {
            if (ExistingAttributes & AttribFileAttrPairs[Index].Flag) {
                StrAtts[Index] = AttribFileAttrPairs[Index].DisplayLetter;
            } else {
                StrAtts[Index] = ' ';
            }
        }

        StrAtts[Index] = '\0';

        if (!YoriLibUnescapePath(FilePath, &AttribContext->UnescapedPath)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s %y\n"), StrAtts, FilePath);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s %y\n"), StrAtts, &AttribContext->UnescapedPath);
        }
    } else {

        NewAttributes = ExistingAttributes & ~(AttribContext->AttributesToClear);
        NewAttributes = NewAttributes | AttribContext->AttributesToSet;

        if (NewAttributes != ExistingAttributes) {
            if (!SetFileAttributes(FilePath->StartOfString, NewAttributes)) {
                LastError = GetLastError();
                ErrText = YoriLibGetWinErrorText(LastError);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("attrib: modification of attributes failed: %y %s"), FilePath, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                return TRUE;
            } else if (AttribContext->Verbose) {
                if (!YoriLibUnescapePath(FilePath, &AttribContext->UnescapedPath)) {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Updating %y\n"), FilePath);
                } else {
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Updating %y\n"), &AttribContext->UnescapedPath);
                }
            }
        }
    }

    AttribContext->SavedErrorThisArg = ERROR_SUCCESS;
    AttribContext->FilesFound++;

    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to the context block indicating whether the
        enumeration was recursive.  Recursive enumerates do not complain
        if a matching file is not in every single directory, because
        common usage expects files to be in a subset of directories only.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
AttribFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;
    PATTRIB_CONTEXT AttribContext = (PATTRIB_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Depth);

    if (!YoriLibUnescapePath(FilePath, &AttribContext->UnescapedPath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    } else {
        UnescapedFilePath.StartOfString = AttribContext->UnescapedPath.StartOfString;
        UnescapedFilePath.LengthInChars = AttribContext->UnescapedPath.LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND || ErrorCode == ERROR_INVALID_NAME) {
        AttribContext->SavedErrorThisArg = ErrorCode;
        Result = TRUE;
    } else {
        LPTSTR ErrText = YoriLibGetWinErrorText(ErrorCode);
        YORI_STRING DirName;
        LPTSTR FilePart;
        YoriLibInitEmptyString(&DirName);
        DirName.StartOfString = UnescapedFilePath.StartOfString;
        FilePart = YoriLibFindRightMostCharacter(&UnescapedFilePath, '\\');
        if (FilePart != NULL) {
            DirName.LengthInChars = (DWORD)(FilePart - DirName.StartOfString);
        } else {
            DirName.LengthInChars = UnescapedFilePath.LengthInChars;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Enumerate of %y failed: %s"), &DirName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
    }
    return Result;
}


/**
 Translate a string of characters into their corresponding attribute flags and
 return the result.

 @param String Pointer to a string containing one or more attribute characters.

 @return The result in attribute flags, or zero to indicate that at least one
         character could not be resolved to a flag.
 */
DWORD
AttribStringToFlags(
    __in PYORI_STRING String
    )
{
    DWORD Flags;
    DWORD Index;
    DWORD FlagIndex;
    TCHAR Char;

    Flags = 0;
    for (Index = 0; Index < String->LengthInChars; Index++) {
        Char = YoriLibUpcaseChar(String->StartOfString[Index]);

        for (FlagIndex = 0; FlagIndex < sizeof(AttribFileAttrPairs)/sizeof(AttribFileAttrPairs[0]); FlagIndex++) {
            if (Char == AttribFileAttrPairs[FlagIndex].DisplayLetter) {
                Flags = Flags | AttribFileAttrPairs[FlagIndex].Flag;
                break;
            }
        }

        if (FlagIndex == sizeof(AttribFileAttrPairs)/sizeof(AttribFileAttrPairs[0])) {
            return 0;
        }
    }

    return Flags;
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the attrib builtin command.
 */
#define ENTRYPOINT YoriCmd_YATTRIB
#else
/**
 The main entrypoint for the attrib standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the attrib cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero on success, nonzero on failure.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    DWORD MatchFlags;
    DWORD Result;
    DWORD NewAttributes;
    BOOLEAN BasicEnumeration = FALSE;
    BOOLEAN MatchAllFiles = FALSE;
    ATTRIB_CONTEXT AttribContext;
    TCHAR PrefixChar;
    YORI_STRING Arg;
    YORI_STRING FullPath;

    ZeroMemory(&AttribContext, sizeof(AttribContext));
    YoriLibInitEmptyString(&Arg);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        Arg.StartOfString = ArgV[i].StartOfString;
        Arg.LengthInChars = ArgV[i].LengthInChars;

        PrefixChar = '\0';
        if (Arg.LengthInChars > 0) {
            if (Arg.StartOfString[0] == '/' ||
                Arg.StartOfString[0] == '-' ||
                Arg.StartOfString[0] == '+') {

                PrefixChar = Arg.StartOfString[0];
                Arg.StartOfString++;
                Arg.LengthInChars--;
            }
        }

        //
        //  Unlike everything else, options here use / exclusively because
        //  - is for removing attributes.
        //

        if (PrefixChar == '/') {
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                AttribHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                AttribContext.IncludeDirectories = TRUE;
                ArgumentUnderstood = TRUE;

            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                AttribContext.Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("v")) == 0) {
                AttribContext.Verbose = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartArg = i + 1;
                break;
            }
        } else if (PrefixChar == '-') {
            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                AttribHelp();
                return EXIT_SUCCESS;
            } else {
                NewAttributes = AttribStringToFlags(&Arg);
                if (NewAttributes != 0) {
                    AttribContext.AttributesToClear |= NewAttributes;
                    ArgumentUnderstood = TRUE;
                }
            }
        } else if (PrefixChar == '+') {
            NewAttributes = AttribStringToFlags(&Arg);
            if (NewAttributes != 0) {
                AttribContext.AttributesToSet |= NewAttributes;
                ArgumentUnderstood = TRUE;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0 || StartArg == ArgC) {
        MatchAllFiles = TRUE;
    }

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    Result = EXIT_SUCCESS;
    MatchFlags = YORILIB_FILEENUM_RETURN_FILES;
    if (AttribContext.IncludeDirectories) {
        MatchFlags |= YORILIB_FILEENUM_RETURN_DIRECTORIES;
    }
    if (AttribContext.Recursive) {
        MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN;
        if (!AttribContext.IncludeDirectories) {
            MatchFlags |= YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
        }
    }
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    if (MatchAllFiles) {
        YoriLibConstantString(&Arg, _T("*"));

        if (!YoriLibUserStringToSingleFilePath(&Arg, TRUE, &FullPath)) {
            return EXIT_FAILURE;
        }

        AttribContext.FilesFoundThisArg = 0;
        AttribContext.SavedErrorThisArg = ERROR_SUCCESS;

        YoriLibForEachFile(&FullPath,
                           MatchFlags,
                           0,
                           AttribFileFoundCallback,
                           AttribFileEnumerateErrorCallback,
                           &AttribContext);

        YoriLibFreeStringContents(&FullPath);

        if (AttribContext.SavedErrorThisArg != ERROR_SUCCESS) {
            Result = EXIT_FAILURE;
        }

    } else {
        for (i = StartArg; i < ArgC; i++) {

            AttribContext.FilesFoundThisArg = 0;
            AttribContext.SavedErrorThisArg = ERROR_SUCCESS;

            YoriLibForEachFile(&ArgV[i],
                               MatchFlags,
                               0,
                               AttribFileFoundCallback,
                               AttribFileEnumerateErrorCallback,
                               &AttribContext);

            if (AttribContext.FilesFoundThisArg == 0) {
                YoriLibInitEmptyString(&FullPath);
                if (YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
                    AttribFileFoundCallback(&FullPath, NULL, 0, &AttribContext);
                    YoriLibFreeStringContents(&FullPath);
                }
            }

            if (AttribContext.SavedErrorThisArg != ERROR_SUCCESS) {
                Result = EXIT_FAILURE;
            }
        }
    }

    YoriLibFreeStringContents(&AttribContext.UnescapedPath);

    if (AttribContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("attrib: no matching files found\n"));
        Result = EXIT_FAILURE;
    }

    return Result;
}

// vim:sw=4:ts=4:et:
