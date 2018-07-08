/**
 * @file erase/erase.c
 *
 * Yori shell erase files
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
 * FITNESS ERASE A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ERASE ANY CLAIM, DAMAGES OR OTHER
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
CHAR strHelpText[] =
        "\n"
        "Delete one or more files.\n"
        "\n"
        "ERASE [-license] [-b] [-r] [-s] <file> [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -r             Send files to the recycle bin\n"
        "   -s             Erase all files matching the pattern in all subdirectories\n";

/**
 Display usage text to the user.
 */
BOOL
EraseHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Erase %i.%i\n"), ERASE_VER_MAJOR, ERASE_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 A structure passed to each file found.
 */
typedef struct _ERASE_CONTEXT {

    /**
     TRUE if files should be sent to the recycle bin.
     */
    BOOL RecycleBin;

} ERASE_CONTEXT, *PERASE_CONTEXT;

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param Context Specifies if erase should move objects to the recycle bin.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
EraseFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PERASE_CONTEXT Context
    )
{
    DWORD Err;
    LPTSTR ErrText;
    BOOLEAN FileDeleted;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(FilePath->StartOfString[FilePath->LengthInChars] == '\0');

    FileDeleted = FALSE;
    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

        //
        //  If the user wanted it deleted via the recycle bin, try that.
        //

        if (Context->RecycleBin) {
            if (YoriLibRecycleBinFile(FilePath)) {
                FileDeleted = TRUE;
            }
        }

        //
        //  If the user didn't ask for recycle bin or if that failed, delete
        //  directly.
        //

        if (!FileDeleted && !DeleteFile(FilePath->StartOfString)) {
            Err = GetLastError();
            if (Err == ERROR_ACCESS_DENIED) {
                DWORD OldAttributes = GetFileAttributes(FilePath->StartOfString);
                DWORD NewAttributes = OldAttributes & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        
                if (OldAttributes != NewAttributes) {
                    SetFileAttributes(FilePath->StartOfString, NewAttributes);
        
                    Err = NO_ERROR;
        
                    if (!DeleteFile(FilePath->StartOfString)) {
                        Err = GetLastError();
                    }
        
                    if (Err != NO_ERROR) {
                        SetFileAttributes(FilePath->StartOfString, OldAttributes);
                    }
                }
            }

            if (Err != NO_ERROR) {
                ErrText = YoriLibGetWinErrorText(Err);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("erase: delete of %y failed: %s"), FilePath, ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
        }
    }
    return TRUE;
}

/**
 The main entrypoint for the erase cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD MatchFlags;
    BOOL Recursive;
    BOOL BasicEnumeration;
    DWORD StartArg = 0;
    DWORD i;
    ERASE_CONTEXT Context;
    YORI_STRING Arg;

    ZeroMemory(&Context, sizeof(Context));
    Recursive = FALSE;
    BasicEnumeration = FALSE;

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));
        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                EraseHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                Context.RecycleBin = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
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

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("erase: missing argument\n"));
        return EXIT_FAILURE;
    }

    MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_DIRECTORY_CONTENTS;
    if (Recursive) {
        MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
    }
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    for (i = StartArg; i < ArgC; i++) {

        YoriLibForEachFile(&ArgV[i], MatchFlags, 0, EraseFileFoundCallback, &Context);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
