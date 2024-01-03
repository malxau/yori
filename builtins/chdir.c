/**
 * @file builtins/chdir.c
 *
 * Yori shell change directory
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strChdirHelpText[] =
        "\n"
        "Changes the current directory.\n"
        "\n"
        "CHDIR [-e] [-license] <directory>\n"
        "\n"
        "   -e             Change to an escaped long path\n";

/**
 Display usage text to the user.
 */
BOOL
ChdirHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Chdir %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strChdirHelpText);
    return TRUE;
}

/**
 Change current directory builtin command.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_CHDIR(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL Result;
    YORI_ALLOC_SIZE_T OldCurrentDirectoryLength;
    YORI_STRING OldCurrentDirectory;
    YORI_STRING NewCurrentDirectory;
    PYORI_STRING NewDir;
    BOOLEAN ArgumentUnderstood;
    BOOLEAN SetToLongPath = FALSE;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    DWORD LastError;
    LPTSTR ErrText;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ChdirHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                SetToLongPath = TRUE;
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("chdir: missing argument\n"));
        return EXIT_FAILURE;
    }

    NewDir = &ArgV[StartArg];

    OldCurrentDirectoryLength = (YORI_ALLOC_SIZE_T)GetCurrentDirectory(0, NULL);
    if (!YoriLibAllocateString(&OldCurrentDirectory, OldCurrentDirectoryLength)) {
        return EXIT_FAILURE;
    }

    OldCurrentDirectory.LengthInChars = (YORI_ALLOC_SIZE_T)GetCurrentDirectory(OldCurrentDirectory.LengthAllocated, OldCurrentDirectory.StartOfString);
    if (OldCurrentDirectory.LengthInChars == 0 ||
        OldCurrentDirectory.LengthInChars >= OldCurrentDirectory.LengthAllocated) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("chdir: could not query current directory: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&OldCurrentDirectory);
        return EXIT_FAILURE;
    }

    if (ArgC <= 1) {
        YoriLibFreeStringContents(&OldCurrentDirectory);
        return EXIT_FAILURE;
    }

    if (NewDir->LengthInChars == 2 &&
        ((NewDir->StartOfString[0] >= 'A' && NewDir->StartOfString[0] <= 'Z') ||
         (NewDir->StartOfString[0] >= 'a' && NewDir->StartOfString[0] <= 'z')) &&
        NewDir->StartOfString[1] == ':') {

        //
        //  Cases:
        //  1. Same drive, do nothing (return success)
        //  2. Different drive, prior dir exists
        //  3. Different drive, no prior dir exists
        //

        if (OldCurrentDirectory.StartOfString[0] >= 'a' && OldCurrentDirectory.StartOfString[0] <= 'z') {
            OldCurrentDirectory.StartOfString[0] = YoriLibUpcaseChar(OldCurrentDirectory.StartOfString[0]);
        }

        if (OldCurrentDirectory.StartOfString[1] == ':' &&
            (OldCurrentDirectory.StartOfString[0] == NewDir->StartOfString[0] ||
             (NewDir->StartOfString[0] >= 'a' &&
              NewDir->StartOfString[0] <= 'z' &&
              (NewDir->StartOfString[0] - 'a' + 'A') == OldCurrentDirectory.StartOfString[0]))) {

            YoriLibFreeStringContents(&OldCurrentDirectory);
            return EXIT_SUCCESS;
        }

        if (!YoriLibGetCurrentDirectoryOnDrive(NewDir->StartOfString[0], &NewCurrentDirectory)) {
            YoriLibFreeStringContents(&OldCurrentDirectory);
            return EXIT_FAILURE;
        }

        //
        //  Check if the current directory on another drive is still valid.
        //  If it's not, truncate it.  Note because sizeof includes a NULL
        //  the below check is that it's above or equal to 4 chars, so it's
        //  safe to truncate back to 3.
        //

        if (NewCurrentDirectory.LengthInChars >= sizeof("x:\\") &&
            GetFileAttributes(NewCurrentDirectory.StartOfString) == (DWORD)-1) {

            NewCurrentDirectory.LengthInChars = 3;
            NewCurrentDirectory.StartOfString[3] = '\0';
        }
    } else {

        YORI_STRING CdPath;
        YORI_STRING Component;
        LPTSTR Sep;

        //
        //  The user specified a path name that's more than just a drive
        //  letter.  Scan through all of the components in YORICDPATH to
        //  find the first one that exists.
        //

        YoriLibInitEmptyString(&CdPath);
        if (!YoriLibAllocateAndGetEnvironmentVariable(_T("YORICDPATH"), &CdPath)) {
            LastError = GetLastError();
            ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("chdir: could not query environment: %s"), ErrText);
            YoriLibFreeWinErrorText(ErrText);
            YoriLibFreeStringContents(&OldCurrentDirectory);
            return EXIT_FAILURE;
        }

        if (CdPath.LengthInChars == 0) {
            YoriLibConstantString(&CdPath, _T("."));
        }

        YoriLibInitEmptyString(&Component);
        YoriLibInitEmptyString(&NewCurrentDirectory);
        Component.StartOfString = CdPath.StartOfString;

        while(TRUE) {
            Component.LengthInChars = CdPath.LengthInChars - (YORI_ALLOC_SIZE_T)(Component.StartOfString - CdPath.StartOfString);

            Sep = YoriLibFindLeftMostCharacter(&Component, ';');
            if (Sep != NULL) {
                Component.LengthInChars = (YORI_ALLOC_SIZE_T)(Sep - Component.StartOfString);
            }

            NewCurrentDirectory.LengthInChars = 0;

            //
            //  If the component is ".", special rules apply.  Here  we allow
            //  paths to anywhere, including paths relative to the current
            //  directory on another drive.
            //

            if (YoriLibCompareStringWithLiteral(&Component, _T(".")) == 0) {

                if (!YoriLibUserStringToSingleFilePath(NewDir, SetToLongPath, &NewCurrentDirectory)) {
                    LastError = GetLastError();
                    ErrText = YoriLibGetWinErrorText(LastError);
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("chdir: could not resolve full path: %y: %s"), NewDir, ErrText);
                    YoriLibFreeWinErrorText(ErrText);
                    YoriLibFreeStringContents(&OldCurrentDirectory);
                    YoriLibFreeStringContents(&CdPath);
                    return EXIT_FAILURE;
                }
            } else {

                if (Component.LengthInChars > 0 &&
                    !YoriLibGetFullPathNameRelativeTo(&Component, NewDir, SetToLongPath, &NewCurrentDirectory, NULL)) {
                    LastError = GetLastError();

                    //
                    //  ERROR_BAD_PATHNAME is ignored because it may be
                    //  the string the user entered is drive relative and
                    //  hence cannot match against this component.
                    //

                    if (LastError != ERROR_BAD_PATHNAME) {
                        ErrText = YoriLibGetWinErrorText(LastError);
                        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("chdir: could not resolve relative full path: %y: %y: %s"), NewDir, &Component, ErrText);
                        YoriLibFreeWinErrorText(ErrText);
                        YoriLibFreeStringContents(&OldCurrentDirectory);
                        YoriLibFreeStringContents(&CdPath);
                        return EXIT_FAILURE;
                    }
                    NewCurrentDirectory.LengthInChars = 0;
                }
            }

            //
            //  If we've found a directory that exists, try to use it
            //

            if (NewCurrentDirectory.LengthInChars > 0 &&
                GetFileAttributes(NewCurrentDirectory.StartOfString) != (DWORD)-1) {
                break;
            }

            //
            //  If there are no more components, we can't find it
            //

            if (Sep == NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("chdir: could not locate path: %y"), NewDir);
                YoriLibFreeStringContents(&OldCurrentDirectory);
                YoriLibFreeStringContents(&NewCurrentDirectory);
                YoriLibFreeStringContents(&CdPath);
                return EXIT_FAILURE;
            }

            //
            //  If we haven't found it yet, and there are more components,
            //  move to the next one
            //

            Component.StartOfString = Sep + 1;
        }
        YoriLibFreeStringContents(&CdPath);
    }

    Result = YoriCallSetCurrentDirectory(&NewCurrentDirectory);
    if (!Result) {
        LastError = GetLastError();
        ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("chdir: could not change directory: %y: %s"), &NewCurrentDirectory, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&OldCurrentDirectory);
        YoriLibFreeStringContents(&NewCurrentDirectory);
        return EXIT_FAILURE;
    }

    //
    //  Convert the first character to uppercase for comparison later.
    //

    if (OldCurrentDirectory.StartOfString[0] >= 'a' && OldCurrentDirectory.StartOfString[0] <= 'z') {
        OldCurrentDirectory.StartOfString[0] = YoriLibUpcaseChar(OldCurrentDirectory.StartOfString[0]);
    }

    if (NewCurrentDirectory.StartOfString[0] >= 'a' && NewCurrentDirectory.StartOfString[0] <= 'z') {
        NewCurrentDirectory.StartOfString[0] = YoriLibUpcaseChar(NewCurrentDirectory.StartOfString[0]);
    }

    //
    //  If the old current directory is drive letter based, preserve the old
    //  current directory in the environment.
    //

    if (OldCurrentDirectory.StartOfString[0] >= 'A' &&
        OldCurrentDirectory.StartOfString[0] <= 'Z' &&
        OldCurrentDirectory.StartOfString[1] == ':') {

        if (NewCurrentDirectory.StartOfString[0] != OldCurrentDirectory.StartOfString[0]) {
            YoriLibSetCurrentDirectoryOnDrive(OldCurrentDirectory.StartOfString[0], &OldCurrentDirectory);
        }
    }

    YoriLibFreeStringContents(&OldCurrentDirectory);
    YoriLibFreeStringContents(&NewCurrentDirectory);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
