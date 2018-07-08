/**
 * @file mkdir/mkdir.c
 *
 * Yori shell mkdir
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
 * FITNESS MKDIR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE MKDIR ANY CLAIM, DAMAGES OR OTHER
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
        "Creates directories.\n"
        "\n"
        "MKDIR [-license] <dir> [<dir>...]\n"
        ;

/**
 Display usage text to the user.
 */
BOOL
MkdirHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Mkdir %i.%i\n"), MKDIR_VER_MAJOR, MKDIR_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}


/**
 Create a directory, and any parent directories that do not yet exist.

 @param DirName The directory to create.
 */
VOID
MkdirCreateDirectoryAndParents(
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
                    LPTSTR ErrText = YoriLibGetWinErrorText(Err);
                    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mkdir: create failed: %y: %s"), DirName, ErrText);
                    YoriLibFreeWinErrorText(ErrText);
                    return;
                }

                DirName->StartOfString[SepIndex] = '\0';
                DirName->LengthInChars = SepIndex;
                continue;

            } else {
                LPTSTR ErrText = YoriLibGetWinErrorText(Err);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mkdir: create failed: %y: %s"), DirName, ErrText);
                YoriLibFreeWinErrorText(ErrText);
                return;
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
                return;
            }
        }
    }
}


/**
 The main entrypoint for the for cmdlet.

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
    DWORD StartArg = 0;
    DWORD i;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                MkdirHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mkdir: missing argument\n"));
        return EXIT_FAILURE;
    }

    for (i = StartArg; i < ArgC; i++) {
        YORI_STRING FullPath;
        if (!YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mkdir: could not resolve full path: %y\n"), &ArgV[i]);
        } else {
            MkdirCreateDirectoryAndParents(&FullPath);
            YoriLibFreeStringContents(&FullPath);
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
