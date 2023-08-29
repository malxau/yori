/**
 * @file test/fileenum.c
 *
 * Yori shell test file enumeration
 *
 * Copyright (c) 2022 Malcolm J. Smith
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
#include "test.h"

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _TEST_ENUM_CONTEXT {

    /**
     Set to TRUE once the variation has failed and no further validation
     should occur.
     */
    BOOLEAN Failed;

    /**
     Indicates the number of files enumerated.
     */
    DWORDLONG FilesFound;

    /**
     Indicates the search criteria.
     */
    YORI_STRING FileSpec;

} TEST_ENUM_CONTEXT, *PTEST_ENUM_CONTEXT;

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.  This can be NULL if the file
        was not found by enumeration.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param Context Pointer to the tail context structure indicating the
        action to perform and populated with the file and line count found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
TestEnumFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in_opt PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PTEST_ENUM_CONTEXT TestContext = (PTEST_ENUM_CONTEXT)Context;
    DWORD Index;

    UNREFERENCED_PARAMETER(FileInfo);
    UNREFERENCED_PARAMETER(Depth);

    TestContext->FilesFound++;

    if (!TestContext->Failed) {
        if (!YoriLibIsStringNullTerminated(FilePath)) {
            TestContext->Failed = TRUE;
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i File path is not NULL terminated\n"), __FILE__, __LINE__);
            return FALSE;
        }

        if (!YoriLibIsPrefixedDriveLetterWithColonAndSlash(FilePath)) {
            TestContext->Failed = TRUE;
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i File path is not prefixed with drive letter and slash\n"), __FILE__, __LINE__);
            return FALSE;
        }

        if (FilePath->LengthInChars < sizeof("\\\\?\\C:\\x") - 1) {
            TestContext->Failed = TRUE;
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i File path is too short\n"), __FILE__, __LINE__);
            return FALSE;
        }

        for (Index = 6; Index < FilePath->LengthInChars - 1; Index++) {
            if (YoriLibIsSep(FilePath->StartOfString[Index]) &&
                YoriLibIsSep(FilePath->StartOfString[Index + 1])) {

                TestContext->Failed = TRUE;
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i File path contains double slash: %y\n"), __FILE__, __LINE__, FilePath);
                return FALSE;
            }
        }

        if (YoriLibIsSep(FilePath->StartOfString[FilePath->LengthInChars - 1])) {
            TestContext->Failed = TRUE;
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i File path contains a trailing slash: %y\n"), __FILE__, __LINE__, FilePath);
            return FALSE;
        }
    }

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
TestEnumFileEnumerateErrorCallback(
    __in PYORI_STRING FilePath,
    __in DWORD ErrorCode,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PTEST_ENUM_CONTEXT TestContext = (PTEST_ENUM_CONTEXT)Context;

    UNREFERENCED_PARAMETER(FilePath);
    UNREFERENCED_PARAMETER(ErrorCode);
    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(TestContext);

    return TRUE;
}

/**
 A test variation to enumerate files in the root and check for well formed
 paths.
 */
BOOLEAN
TestEnumRoot(VOID)
{
    TEST_ENUM_CONTEXT TestContext;
    DWORDLONG OldFilesFound;
    YORI_STRING OldCurrentDirectory;

    TestContext.Failed = FALSE;
    TestContext.FilesFound = 0;
    OldFilesFound = 0;

    YoriLibConstantString(&TestContext.FileSpec, _T("C:\\*"));
    if (!YoriLibForEachFile(&TestContext.FileSpec,
                            YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES,
                            0,
                            TestEnumFileFoundCallback,
                            TestEnumFileEnumerateErrorCallback,
                            &TestContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile failed searching %y, error %i\n"), __FILE__, __LINE__, &TestContext.FileSpec, GetLastError());
        return FALSE;
    }

    if (TestContext.FilesFound == OldFilesFound) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile found no files looking for %y\n"), __FILE__, __LINE__, &TestContext.FileSpec);
        return FALSE;
    }

    if (TestContext.Failed) {
        return FALSE;
    }

    OldFilesFound = TestContext.FilesFound;

    YoriLibConstantString(&TestContext.FileSpec, _T("\\"));
    if (!YoriLibForEachFile(&TestContext.FileSpec,
                            YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES | YORILIB_FILEENUM_DIRECTORY_CONTENTS,
                            0,
                            TestEnumFileFoundCallback,
                            TestEnumFileEnumerateErrorCallback,
                            &TestContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile failed searching %y, error %i\n"), __FILE__, __LINE__, &TestContext.FileSpec, GetLastError());
        return FALSE;
    }

    if (TestContext.FilesFound == OldFilesFound) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile found no files looking for %y\n"), __FILE__, __LINE__, &TestContext.FileSpec);
        return FALSE;
    }

    if (TestContext.Failed) {
        return FALSE;
    }

    OldFilesFound = TestContext.FilesFound;

    //
    //  Searching without enumerating directory contents should return
    //  one match, with no trailing slash
    //

    YoriLibConstantString(&TestContext.FileSpec, _T("C:\\Windows\\"));
    if (!YoriLibForEachFile(&TestContext.FileSpec,
                            YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES,
                            0,
                            TestEnumFileFoundCallback,
                            TestEnumFileEnumerateErrorCallback,
                            &TestContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile failed searching %y, error %i\n"), __FILE__, __LINE__, &TestContext.FileSpec, GetLastError());
        return FALSE;
    }

    if (TestContext.FilesFound != OldFilesFound + 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile did not find exactly one file when looking for %y\n"), __FILE__, __LINE__, &TestContext.FileSpec);
        return FALSE;
    }

    if (TestContext.Failed) {
        return FALSE;
    }

    OldFilesFound = TestContext.FilesFound;

    //
    //  Look for a single object depending on the current directory
    //

    if (!YoriLibGetCurrentDirectory(&OldCurrentDirectory)) {
        return FALSE;
    }

    if (!SetCurrentDirectory(_T("C:\\"))) {
        return FALSE;
    }

    YoriLibConstantString(&TestContext.FileSpec, _T("Windows"));
    if (!YoriLibForEachFile(&TestContext.FileSpec,
                            YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES,
                            0,
                            TestEnumFileFoundCallback,
                            TestEnumFileEnumerateErrorCallback,
                            &TestContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile failed searching %y, error %i\n"), __FILE__, __LINE__, &TestContext.FileSpec, GetLastError());
        return FALSE;
    }

    if (TestContext.FilesFound == OldFilesFound) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile found no files looking for %y\n"), __FILE__, __LINE__, &TestContext.FileSpec);
        return FALSE;
    }

    if (TestContext.Failed) {
        return FALSE;
    }

    OldFilesFound = TestContext.FilesFound;

    if (!SetCurrentDirectory(OldCurrentDirectory.StartOfString)) {
        return FALSE;
    }

    YoriLibFreeStringContents(&OldCurrentDirectory);

    YoriLibConstantString(&TestContext.FileSpec, _T("C:\\Windows\\"));
    if (!YoriLibForEachFile(&TestContext.FileSpec,
                            YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES | YORILIB_FILEENUM_DIRECTORY_CONTENTS,
                            0,
                            TestEnumFileFoundCallback,
                            TestEnumFileEnumerateErrorCallback,
                            &TestContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile failed searching %y, error %i\n"), __FILE__, __LINE__, &TestContext.FileSpec, GetLastError());
        return FALSE;
    }

    if (TestContext.FilesFound == OldFilesFound) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile found no files looking for %y\n"), __FILE__, __LINE__, &TestContext.FileSpec);
        return FALSE;
    }

    if (TestContext.Failed) {
        return FALSE;
    }

    OldFilesFound = TestContext.FilesFound;

    YoriLibConstantString(&TestContext.FileSpec, _T("C:*"));
    if (!YoriLibForEachFile(&TestContext.FileSpec,
                            YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES | YORILIB_FILEENUM_DIRECTORY_CONTENTS,
                            0,
                            TestEnumFileFoundCallback,
                            TestEnumFileEnumerateErrorCallback,
                            &TestContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile failed searching %y, error %i\n"), __FILE__, __LINE__, &TestContext.FileSpec, GetLastError());
        return FALSE;
    }

    if (TestContext.FilesFound == OldFilesFound) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile found no files looking for %y\n"), __FILE__, __LINE__, &TestContext.FileSpec);
        return FALSE;
    }

    if (TestContext.Failed) {
        return FALSE;
    }

    return TRUE;
}

/**
 A test variation to enumerate files in the windows directory and check for
 well formed paths.
 */
BOOLEAN
TestEnumWindows(VOID)
{
    TEST_ENUM_CONTEXT TestContext;
    DWORDLONG OldFilesFound;

    TestContext.Failed = FALSE;
    TestContext.FilesFound = 0;
    OldFilesFound = 0;

    //
    //  Searching without enumerating directory contents should return
    //  one match, with no trailing slash
    //

    YoriLibConstantString(&TestContext.FileSpec, _T("C:\\Windows\\"));
    if (!YoriLibForEachFile(&TestContext.FileSpec,
                            YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES,
                            0,
                            TestEnumFileFoundCallback,
                            TestEnumFileEnumerateErrorCallback,
                            &TestContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile failed searching %y, error %i\n"), __FILE__, __LINE__, &TestContext.FileSpec, GetLastError());
        return FALSE;
    }

    if (TestContext.FilesFound != OldFilesFound + 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile did not find exactly one file when looking for %y\n"), __FILE__, __LINE__, &TestContext.FileSpec);
        return FALSE;
    }

    if (TestContext.Failed) {
        return FALSE;
    }

    OldFilesFound = TestContext.FilesFound;

    YoriLibConstantString(&TestContext.FileSpec, _T("C:\\Windows\\"));
    if (!YoriLibForEachFile(&TestContext.FileSpec,
                            YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES | YORILIB_FILEENUM_DIRECTORY_CONTENTS,
                            0,
                            TestEnumFileFoundCallback,
                            TestEnumFileEnumerateErrorCallback,
                            &TestContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile failed searching %y, error %i\n"), __FILE__, __LINE__, &TestContext.FileSpec, GetLastError());
        return FALSE;
    }

    if (TestContext.FilesFound == OldFilesFound) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibForEachFile found no files looking for %y\n"), __FILE__, __LINE__, &TestContext.FileSpec);
        return FALSE;
    }

    if (TestContext.Failed) {
        return FALSE;
    }

    return TRUE;
}

// vim:sw=4:ts=4:et:
