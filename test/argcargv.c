/**
 * @file test/argcargv.c
 *
 * Yori shell test entrypoint parsing
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
#include <yorish.h>
#include "test.h"

/**
 Deallocate an ArgV array.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.
 */
VOID
TestArgCleanupArg(
    __in DWORD ArgC,
    __in PYORI_STRING ArgV
    )
{
    DWORD Index;

    for (Index = 0; Index < ArgC; Index++) {
        YoriLibFreeStringContents(&ArgV[Index]);
    }
    YoriLibDereference(ArgV);
}

/**
 A test variation to parse a command with two space delimited arguments.
 */
BOOLEAN
TestArgTwoArgCmd(VOID)
{
    LPCTSTR InputString = _T("foo bar");
    PYORI_STRING ArgV;
    DWORD ArgC;

    ArgV = YoriLibCmdlineToArgcArgv(InputString, (DWORD)-1, FALSE, &ArgC);
    if (ArgV == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibCmdlineToArgcArgv failed on '%s'\n"), __FILE__, __LINE__, InputString);
        return FALSE;
    }

    if (ArgC != 2) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgC '%s', have %i expected 2\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      ArgC);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&ArgV[0], _T("foo")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgV in '%s', have %y expected foo\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      &ArgV[0]);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&ArgV[1], _T("bar")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgV in '%s', have %y expected bar\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      &ArgV[0]);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    TestArgCleanupArg(ArgC, ArgV);
    return TRUE;
}

/**
 A test variation to parse a command with one argument containing quotes and
 a space embedded partway.
 */
BOOLEAN
TestArgOneArgContainingQuotesCmd(VOID)
{
    LPCTSTR InputString = _T("foo\" \"bar");
    PYORI_STRING ArgV;
    DWORD ArgC;

    ArgV = YoriLibCmdlineToArgcArgv(InputString, (DWORD)-1, FALSE, &ArgC);
    if (ArgV == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibCmdlineToArgcArgv failed on '%s'\n"), __FILE__, __LINE__, InputString);
        return FALSE;
    }

    if (ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgC '%s', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      ArgC);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&ArgV[0], _T("foo bar")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgV in '%s', have %y expected foo bar\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      &ArgV[0]);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    TestArgCleanupArg(ArgC, ArgV);
    return TRUE;
}

/**
 A test variation to parse a command that starts with a quote that ends
 partway through an argument.
 */
BOOLEAN
TestArgOneArgWithStartingQuotesCmd(VOID)
{
    LPCTSTR InputString = _T("\"Program Files\"\\foo");
    PYORI_STRING ArgV;
    DWORD ArgC;

    ArgV = YoriLibCmdlineToArgcArgv(InputString, (DWORD)-1, FALSE, &ArgC);
    if (ArgV == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibCmdlineToArgcArgv failed on '%s'\n"), __FILE__, __LINE__, InputString);
        return FALSE;
    }

    if (ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgC '%s', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      ArgC);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&ArgV[0], _T("Program Files\\foo")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgV in '%s', have %y expected Program Files\\foo\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      &ArgV[0]);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    TestArgCleanupArg(ArgC, ArgV);

    return TRUE;
}

/**
 A test variation to parse a command that starts and ends with a quote.
 */
BOOLEAN
TestArgOneArgEnclosedInQuotesCmd(VOID)
{
    LPCTSTR InputString = _T("\"foo\"==\"foo\" ");
    PYORI_STRING ArgV;
    DWORD ArgC;

    ArgV = YoriLibCmdlineToArgcArgv(InputString, (DWORD)-1, FALSE, &ArgC);
    if (ArgV == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibCmdlineToArgcArgv failed on '%s'\n"), __FILE__, __LINE__, InputString);
        return FALSE;
    }

    if (ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgC '%s', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      ArgC);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&ArgV[0], _T("foo==foo")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgV in '%s', have %y expected foo==foo\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      &ArgV[0]);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    TestArgCleanupArg(ArgC, ArgV);

    return TRUE;
}

/**
 A test variation to parse a command that contains a quote and ends with a
 quote.
 */
BOOLEAN
TestArgRedirectWithEndingQuoteCmd(VOID)
{
    LPCTSTR InputString = _T(">\"file name\"");
    PYORI_STRING ArgV;
    DWORD ArgC;

    ArgV = YoriLibCmdlineToArgcArgv(InputString, (DWORD)-1, FALSE, &ArgC);
    if (ArgV == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibCmdlineToArgcArgv failed on '%s'\n"), __FILE__, __LINE__, InputString);
        return FALSE;
    }

    if (ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgC '%s', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      ArgC);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&ArgV[0], _T(">file name")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgV in '%s', have %y expected >file name\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      &ArgV[0]);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    TestArgCleanupArg(ArgC, ArgV);

    return TRUE;
}

/**
 A test variation to parse a command containing backslash escapes.
 */
BOOLEAN
TestArgBackslashEscapeCmd(VOID)
{
    LPCTSTR InputString = _T("\\\\ \\\" \\\\\" \\\\\\\"");
    PYORI_STRING ArgV;
    DWORD ArgC;

    ArgV = YoriLibCmdlineToArgcArgv(InputString, (DWORD)-1, FALSE, &ArgC);
    if (ArgV == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibCmdlineToArgcArgv failed on '%s'\n"), __FILE__, __LINE__, InputString);
        return FALSE;
    }

    if (ArgC != 3) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgC '%s', have %i expected 3\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      ArgC);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&ArgV[0], _T("\\\\")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgV in '%s', have %y expected \\\\\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      &ArgV[0]);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&ArgV[1], _T("\"")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgV in '%s', have %y expected \"\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      &ArgV[1]);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&ArgV[2], _T("\\ \\\"")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibCmdlineToArgcArgv returned unexpected ArgV in '%s', have %y expected \\ \\\"\n"),
                      __FILE__,
                      __LINE__,
                      InputString,
                      &ArgV[2]);
        TestArgCleanupArg(ArgC, ArgV);
        return FALSE;
    }

    TestArgCleanupArg(ArgC, ArgV);

    return TRUE;
}


// vim:sw=4:ts=4:et:
