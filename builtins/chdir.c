/**
 * @file builtins/chdir.c
 *
 * Yori shell change directory
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

#include <yoripch.h>
#include <yorilib.h>

/**
 Help text to display to the user.
 */
const
CHAR strChdirHelpText[] =
        "\n"
        "Changes the current directory.\n"
        "\n"
        "CHDIR [-e] <directory>\n"
        "\n"
        "   -e             Change to an escaped long path\n";

/**
 Display usage text to the user.
 */
BOOL
ChdirHelp()
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2017"), &License);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Chdir %i.%i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strChdirHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &License);
    YoriLibFreeStringContents(&License);
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
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL Result;
    DWORD OldCurrentDirectoryLength;
    YORI_STRING OldCurrentDirectory;
    YORI_STRING NewCurrentDirectory;
    PYORI_STRING NewDir;
    BOOL ArgumentUnderstood;
    BOOL SetToLongPath = FALSE;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ChdirHelp();
                return EXIT_SUCCESS;
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

    OldCurrentDirectoryLength = GetCurrentDirectory(0, NULL);
    if (!YoriLibAllocateString(&OldCurrentDirectory, OldCurrentDirectoryLength)) {
        return EXIT_FAILURE;
    }

    OldCurrentDirectory.LengthInChars = GetCurrentDirectory(OldCurrentDirectory.LengthAllocated, OldCurrentDirectory.StartOfString);
    if (OldCurrentDirectory.LengthInChars == 0 ||
        OldCurrentDirectory.LengthInChars >= OldCurrentDirectory.LengthAllocated) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not query current directory: %s"), ErrText);
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

    } else {

        if (!YoriLibUserStringToSingleFilePath(NewDir, SetToLongPath, &NewCurrentDirectory)) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could resolve full path: %y: %s"), NewDir, ErrText);
            YoriLibFreeWinErrorText(ErrText);
            YoriLibFreeStringContents(&OldCurrentDirectory);
            return EXIT_FAILURE;
        }

    }

    Result = SetCurrentDirectory(NewCurrentDirectory.StartOfString);
    if (!Result) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not change directory: %y: %s"), &NewCurrentDirectory, ErrText);
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
