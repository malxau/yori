/**
 * @file fscmp/fscmp.c
 *
 * Yori shell test file system state
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
CHAR strFsCmpHelpText[] =
        "\n"
        "Test for file system conditions.\n"
        "\n"
        "FSCMP [-license] [-b] [-d | -e | -f | -l] <file>\n"
        "\n"
        "   -b             Use basic search criteria\n"
        "   -d             Test if directory exists\n"
        "   -e             Test if object exists\n"
        "   -f             Test if file exists\n"
        "   -l             Test if symbolic link exists\n";

/**
 Display usage text to the user.
 */
BOOL
FsCmpHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("FsCmp %i.%i\n"), FSCMP_VER_MAJOR, FSCMP_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strFsCmpHelpText);
    return TRUE;
}

/**
 An enum of tests that can be applied to files.
 */
typedef enum _FSCMP_TEST_TYPE {
    FsCmpTestTypeUnknown = 0,
    FsCmpTestTypeExists = 1,
    FsCmpTestTypeDirectoryExists = 2,
    FsCmpTestTypeFileExists = 3,
    FsCmpTestTypeLinkExists = 4,
} FSCMP_TEST_TYPE;

/**
 Context to pass when enumerating files.
 */
typedef struct _FSCMP_CONTEXT {

    /**
     The test to apply.
     */
    FSCMP_TEST_TYPE TestType;

    /**
     Set to TRUE if the test is met.
     */
    BOOL ConditionMet;
} FSCMP_CONTEXT, *PFSCMP_CONTEXT;

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param Context Pointer to an FSCMP_CONTEXT.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
FsCmpFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PFSCMP_CONTEXT Context
    )
{
    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FilePath);

    if (Context->TestType == FsCmpTestTypeExists) {
        Context->ConditionMet = TRUE;
        return FALSE;
    } else if (Context->TestType == FsCmpTestTypeDirectoryExists) {
        if (FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            Context->ConditionMet = TRUE;
            return FALSE;
        }
    } else if (Context->TestType == FsCmpTestTypeLinkExists) {
        if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 &&
            (FileInfo->dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT ||
             FileInfo->dwReserved0 == IO_REPARSE_TAG_SYMLINK)) {

            Context->ConditionMet = TRUE;
            return FALSE;
        }
    } else if (Context->TestType == FsCmpTestTypeFileExists) {
        if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
            ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0 ||
             (FileInfo->dwReserved0 != IO_REPARSE_TAG_MOUNT_POINT &&
              FileInfo->dwReserved0 != IO_REPARSE_TAG_SYMLINK))) {

            Context->ConditionMet = TRUE;
            return FALSE;
        }
    }

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the fscmp builtin command.
 */
#define ENTRYPOINT YoriCmd_FSCMP
#else
/**
 The main entrypoint for the fscmp standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the fscmp cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD StartArg = 0;
    DWORD i;
    DWORD MatchFlags;
    FSCMP_CONTEXT FsCmpContext;
    BOOL BasicExpansion;
    YORI_STRING Arg;

    ZeroMemory(&FsCmpContext, sizeof(FsCmpContext));
    BasicExpansion = FALSE;

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));
        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                FsCmpHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicExpansion = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                FsCmpContext.TestType = FsCmpTestTypeDirectoryExists;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                FsCmpContext.TestType = FsCmpTestTypeExists;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                FsCmpContext.TestType = FsCmpTestTypeFileExists;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                FsCmpContext.TestType = FsCmpTestTypeLinkExists;
                ArgumentUnderstood = TRUE;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("fscmp: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (FsCmpContext.TestType == FsCmpTestTypeUnknown) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("fscmp: missing test condition\n"));
        return EXIT_FAILURE;
    }

    MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES;
    if (BasicExpansion) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    for (i = StartArg; i < ArgC && !FsCmpContext.ConditionMet; i++) {
        YoriLibForEachFile(&ArgV[i], MatchFlags, 0, FsCmpFileFoundCallback, &FsCmpContext);
    }

    if (FsCmpContext.ConditionMet) {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

// vim:sw=4:ts=4:et:
