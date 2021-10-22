/**
 * @file petool/petool.c
 *
 * Yori shell PE tool for manipulating PE files
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
CHAR strPeToolHelpText[] =
        "\n"
        "Manage PE files.\n"
        "\n"
        "PETOOL [-license]\n"
        "PETOOL -c file\n"
        "\n"
        "   -c             Calculate the PE checksum for a binary\n";

/**
 Display usage text to the user.
 */
BOOL
PeToolHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("PeTool %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strPeToolHelpText);
    return TRUE;
}

/**
 Calculate and display the checksum for a specified file.

 @param FileName Pointer to the file name to calculate a checksum for.
 */
VOID
PeToolCalculateChecksum(
    __in PYORI_STRING FileName
    )
{
    DWORD Err;
    DWORD HeaderChecksum;
    DWORD DataChecksum;

    YoriLibLoadImageHlpFunctions();

    if (DllImageHlp.pMapFileAndCheckSumW == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("petool: OS support not present\n"));
        return;
    }

    Err = DllImageHlp.pMapFileAndCheckSumW(FileName->StartOfString, &HeaderChecksum, &DataChecksum);
    if (Err != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %y\n"), FileName);
        return;
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Checksum in PE header: %08x\n")
                                          _T("Checksum of file contents: %08x\n"),
                                          HeaderChecksum,
                                          DataChecksum);
}

/**
 A set of operations supported by this application.
 */
typedef enum _PETOOL_OP {
    PeToolOpNone = 0,
    PeToolOpCalculateChecksum = 1
} PETOOL_OP;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the petool builtin command.
 */
#define ENTRYPOINT YoriCmd_PETOOL
#else
/**
 The main entrypoint for the petool standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the petool cmdlet.

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
    DWORD i;
    DWORD StartArg;
    YORI_STRING Arg;
    PYORI_STRING FileName = NULL;
    PETOOL_OP Op;

    Op = PeToolOpNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                PeToolHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    FileName = &ArgV[i + 1];
                    Op = PeToolOpCalculateChecksum;
                    ArgumentUnderstood = TRUE;
                }
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

    if (Op == PeToolOpNone) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("petool: operation not specified\n"));
        return EXIT_FAILURE;
    }

    switch(Op) {
        case PeToolOpCalculateChecksum:
            PeToolCalculateChecksum(FileName);
            break;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
