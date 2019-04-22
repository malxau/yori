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
CHAR strMkdirHelpText[] =
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
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Mkdir %i.%02i\n"), MKDIR_VER_MAJOR, MKDIR_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strMkdirHelpText);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the mkdir builtin command.
 */
#define ENTRYPOINT YoriCmd_YMKDIR
#else
/**
 The main entrypoint for the mkdir standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the mkdir cmdlet.

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
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
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
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mkdir: missing argument\n"));
        return EXIT_FAILURE;
    }

    for (i = StartArg; i < ArgC; i++) {
        YORI_STRING FullPath;
        if (!YoriLibUserStringToSingleFilePath(&ArgV[i], TRUE, &FullPath)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mkdir: could not resolve full path: %y\n"), &ArgV[i]);
        } else {

            //
            //  When this call fails, it leaves the path indicating the
            //  component it failed on.
            //

            if (!YoriLibCreateDirectoryAndParents(&FullPath)) {
                DWORD Err = GetLastError();
                LPTSTR ErrText = YoriLibGetWinErrorText(Err);
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mkdir: create failed: %y: %s"), &FullPath, ErrText);
                YoriLibFreeWinErrorText(ErrText);
            }
            YoriLibFreeStringContents(&FullPath);
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
