/**
 * @file test/test.c
 *
 * Yori shell test suite
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
 Help text to display to the user.
 */
const
CHAR strTestHelpText[] =
        "\n"
        "Run tests.\n"
        "\n"
        "YORITEST [-license] [-v Variation] [-x Variation]\n"
        "\n"
        "   -v             Variation to include\n"
        "   -x             Variation to exclude\n"
        "\n"
        "Supported variations:\n";

/**
 A structure to describe a test variation.
 */
typedef struct _TEST_VARIATION {

    /**
     The function to call to invoke the variation.
     */
    PYORI_TEST_FN Fn;

    /**
     The name of the variation.
     */
    LPCTSTR Name;

    /**
     If TRUE, the execution status of this variation was set explicitly via
     command line parameter.  If FALSE, default execution should apply.
     */
    BOOLEAN ExplicitlySpecified;

    /**
     If TRUE, the variation should execute.  If FALSE, it should not.  Only
     meaningful when ExplicitlySpecified is TRUE.
     */
    BOOLEAN Execute;

} TEST_VARIATION, *PTEST_VARIATION;


/**
 A list of test variations to execute.
 */
TEST_VARIATION TestVariations[] = {
    {TestEnumRoot,    _T("EnumRoot")},
    {TestEnumWindows, _T("EnumWindows")},
};


/**
 Display usage text to the user.
 */
BOOL
TestHelp(VOID)
{
    DWORD i;
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("YoriTest %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strTestHelpText);
    for (i = 0; i < sizeof(TestVariations)/sizeof(TestVariations[0]); i++) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("    %s\n"), TestVariations[i].Name);
    }
    return TRUE;
}


/**
 The main entrypoint for the test cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero indicating success or nonzero on
         failure.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    DWORD i;
    DWORD Var;
    DWORD Succeeded;
    DWORD Failed;
    YORI_STRING Arg;
    BOOLEAN ArgumentUnderstood;
    BOOLEAN RunAll;
    BOOLEAN ExecuteVariation;

    RunAll = TRUE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                TestHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2022"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("v")) == 0) {
                if (ArgC > i + 1) {
                    for (Var = 0; Var < sizeof(TestVariations)/sizeof(TestVariations[0]); Var++) {
                        if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], TestVariations[Var].Name) == 0) {
                            TestVariations[Var].ExplicitlySpecified = TRUE;
                            TestVariations[Var].Execute = TRUE;
                            RunAll = FALSE;
                        }
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("x")) == 0) {
                if (ArgC > i + 1) {
                    for (Var = 0; Var < sizeof(TestVariations)/sizeof(TestVariations[0]); Var++) {
                        if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[i + 1], TestVariations[Var].Name) == 0) {
                            TestVariations[Var].ExplicitlySpecified = TRUE;
                            TestVariations[Var].Execute = FALSE;
                        }
                    }
                }
            }
        }
    }

    Succeeded = 0;
    Failed = 0;

    for (i = 0; i < sizeof(TestVariations)/sizeof(TestVariations[0]); i++) {

        ExecuteVariation = FALSE;
        if (RunAll) {
            if (!TestVariations[i].ExplicitlySpecified ||
                TestVariations[i].Execute) {

                ExecuteVariation = TRUE;
            }
        } else {
            if (TestVariations[i].ExplicitlySpecified &&
                TestVariations[i].Execute) {

                ExecuteVariation = TRUE;
            }
        }

        if (ExecuteVariation) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s...\n"), TestVariations[i].Name);
            if (TestVariations[i].Fn()) {
                Succeeded++;
            } else {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s FAILED\n"), TestVariations[i].Name);
                Failed++;
            }
        }
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%i succeeded, %i failed\n"), Succeeded, Failed);

    if (Failed == 0) {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

// vim:sw=4:ts=4:et:
