/**
 * @file rmdir/rmdir.c
 *
 * Yori shell rmdir
 *
 * Copyright (c) 2017-2021 Malcolm J. Smith
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
 * FITNESS RMDIR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE RMDIR ANY CLAIM, DAMAGES OR OTHER
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
CHAR strRmdirHelpText[] =
        "\n"
        "Removes directories.\n"
        "\n"
        "RMDIR [-license] [-b] [-r] [-s] <dir> [<dir>...]\n"
        "\n"
        "   -b             Use basic search criteria for directories only\n"
        "   -f             Delete files as well as directories\n"
        "   -l             Delete links without contents\n"
        "   -p             Delete with POSIX semantics\n"
        "   -r             Send directories to the recycle bin\n"
        "   -s             Remove all contents of each directory\n";

/**
 Display usage text to the user.
 */
BOOL
RmdirHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Rmdir %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strRmdirHelpText);
    return TRUE;
}

/**
 Context information when files are found.
 */
typedef struct _RMDIR_CONTEXT {

    /**
     If TRUE, objects should be sent to the recycle bin rather than directly
     deleted.
     */
    BOOLEAN RecycleBin;

    /**
     If TRUE, delete files as well as directories.
     */
    BOOLEAN DeleteFiles;

    /**
     If TRUE, delete with POSIX semantics.
     */
    BOOLEAN PosixSemantics;

    /**
     The number of directories successfully removed.
     */
    DWORD DirectoriesRemoved;

} RMDIR_CONTEXT, *PRMDIR_CONTEXT;

BOOL
RmdirFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in SYSERR ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    );

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies the recursion depth.  Ignored in this application.

 @param Context Pointer to a RMDIR_CONTEXT.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
RmdirFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    DWORD Err = NO_ERROR;
    LPTSTR ErrText;
    DWORD OldAttributes;
    DWORD NewAttributes;
    BOOL FileDeleted;
    PRMDIR_CONTEXT RmdirContext = (PRMDIR_CONTEXT)Context;

    //
    //  Don't delete any files that are specified on the command line
    //  directly.  These can be deleted if they're enumerated underneath
    //  a parent object.
    //

    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
        Depth == 0 &&
        !RmdirContext->DeleteFiles) {

        RmdirFileEnumerateErrorCallback(FilePath, ERROR_DIRECTORY, Depth, Context);
        return TRUE;
    }

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    FileDeleted = FALSE;

    //
    //  Try to delete it.
    //

    if (RmdirContext->RecycleBin) {
        if (YoriLibRecycleBinFile(FilePath)) {
            if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                RmdirContext->DirectoriesRemoved++;
            }
            FileDeleted = TRUE;
        }
    }

    if (!FileDeleted) {
        if (RmdirContext->PosixSemantics) {
            if (!YoriLibPosixDeleteFile(FilePath)) {
                Err = GetLastError();
            } else if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                RmdirContext->DirectoriesRemoved++;
            }
        } else if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            if (!DeleteFile(FilePath->StartOfString)) {
                Err = GetLastError();
            }
        } else {
            if (!RemoveDirectory(FilePath->StartOfString)) {
                Err = GetLastError();
            } else {
                RmdirContext->DirectoriesRemoved++;
            }
        }
    }


    //
    //  If it fails with access denied, try to remove any readonly, hidden or
    //  system attributes which might be getting in the way, then try the
    //  delete again.
    //

    if (Err == ERROR_ACCESS_DENIED) {

        OldAttributes = GetFileAttributes(FilePath->StartOfString);
        NewAttributes = OldAttributes & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

        if (OldAttributes != NewAttributes) {
            SetFileAttributes(FilePath->StartOfString, NewAttributes);

            Err = NO_ERROR;

            if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                if (!DeleteFile(FilePath->StartOfString)) {
                    Err = GetLastError();
                }
            } else {
                if (!RemoveDirectory(FilePath->StartOfString)) {
                    Err = GetLastError();
                } else {
                    RmdirContext->DirectoriesRemoved++;
                }
            }

            if (Err != NO_ERROR) {
                SetFileAttributes(FilePath->StartOfString, OldAttributes);
            }
        }
    }

    //
    //  If we still can't delete it, return an error.
    //

    if (Err != NO_ERROR) {
        ErrText = YoriLibGetWinErrorText(Err);
        if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("rmdir: delete failed: %y: %s"), FilePath, ErrText);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("rmdir: rmdir failed: %y: %s"), FilePath, ErrText);
        }
        YoriLibFreeWinErrorText(ErrText);
    }
    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FilePath Pointer to the file path that could not be enumerated.

 @param ErrorCode The Win32 error code describing the failure.

 @param Depth Recursion depth, ignored in this application.

 @param Context Context, ignored in this function.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
RmdirFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in SYSERR ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    YORI_STRING UnescapedFilePath;
    BOOL Result = FALSE;

    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(Context);

    YoriLibInitEmptyString(&UnescapedFilePath);
    if (!YoriLibUnescapePath(FilePath, &UnescapedFilePath)) {
        UnescapedFilePath.StartOfString = FilePath->StartOfString;
        UnescapedFilePath.LengthInChars = FilePath->LengthInChars;
    }

    if (ErrorCode == ERROR_FILE_NOT_FOUND || ErrorCode == ERROR_PATH_NOT_FOUND) {
        if (Depth == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File or directory not found: %y\n"), &UnescapedFilePath);
        }
        Result = TRUE;
    } else {
        LPTSTR ErrText = YoriLibGetWinErrorText(ErrorCode);
        YORI_STRING DirName;
        LPTSTR FilePart;
        YoriLibInitEmptyString(&DirName);
        DirName.StartOfString = UnescapedFilePath.StartOfString;
        FilePart = YoriLibFindRightMostCharacter(&UnescapedFilePath, '\\');
        if (FilePart != NULL && ErrorCode != ERROR_DIRECTORY) {
            DirName.LengthInChars = (YORI_ALLOC_SIZE_T)(FilePart - DirName.StartOfString);
        } else {
            DirName.LengthInChars = UnescapedFilePath.LengthInChars;
        }
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Enumerate of %y failed: %s"), &DirName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
    }
    YoriLibFreeStringContents(&UnescapedFilePath);
    return Result;
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the rmdir builtin command.
 */
#define ENTRYPOINT YoriCmd_YRMDIR
#else
/**
 The main entrypoint for the rmdir standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the rmdir cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process indicating success or failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    BOOLEAN Recursive;
    BOOLEAN BasicEnumeration;
    BOOLEAN DeleteLinks;
    WORD MatchFlags;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_ALLOC_SIZE_T i;
    RMDIR_CONTEXT RmdirContext;
    YORI_STRING Arg;

    ZeroMemory(&RmdirContext, sizeof(RmdirContext));

    Recursive = FALSE;
    BasicEnumeration = FALSE;
    DeleteLinks = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                RmdirHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("f")) == 0) {
                ArgumentUnderstood = TRUE;
                RmdirContext.DeleteFiles = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("l")) == 0) {
                DeleteLinks = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("p")) == 0) {
                ArgumentUnderstood = TRUE;
                RmdirContext.PosixSemantics = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("q")) == 0) {
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("r")) == 0) {
                ArgumentUnderstood = TRUE;
                RmdirContext.RecycleBin = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("s/q")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("-")) == 0) {
                StartArg = i + 1;
                ArgumentUnderstood = TRUE;
                break;
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("rmdir: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (RmdirContext.PosixSemantics &&
        DllKernel32.pSetFileInformationByHandle == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("rmdir: OS support not present\n"));
        return EXIT_FAILURE;
    }

    MatchFlags = YORILIB_ENUM_RETURN_DIRECTORIES;
    if (RmdirContext.DeleteFiles) {
        MatchFlags |= YORILIB_ENUM_RETURN_FILES;
    }
    if (Recursive) {
        MatchFlags |= YORILIB_ENUM_REC_BEFORE_RETURN | YORILIB_ENUM_RETURN_FILES;
    }
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_ENUM_BASIC_EXPANSION;
    }
    if (DeleteLinks) {
        MatchFlags |= YORILIB_ENUM_NO_LINK_TRAVERSE;
    }

    for (i = StartArg; i < ArgC; i++) {
        YoriLibForEachFile(&ArgV[i],
                           MatchFlags,
                           0,
                           RmdirFileFoundCallback,
                           RmdirFileEnumerateErrorCallback,
                           &RmdirContext);
    }

    if (RmdirContext.DirectoriesRemoved == 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
