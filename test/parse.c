/**
 * @file test/parse.c
 *
 * Yori shell test command line parsing
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
 A test variation to parse a command with two space delimited arguments.
 */
BOOLEAN
TestParseTwoArgCmd(VOID)
{
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    YORI_STRING InputString;

    //
    //  First, a simple command with two arguments, no quotes
    //

    YoriLibConstantString(&InputString, _T("foo bar"));
    if (!YoriLibShParseCmdlineToCmdContext(&InputString, 7, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                     _T("%hs:%i YoriLibShParseCmdlineToCmdContext failed on '%y'\n"),
                     __FILE__,
                     __LINE__,
                     &InputString);
        return FALSE;
    }

    if (CmdContext.ArgC != 2) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgC '%y', have %i expected 2\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgC);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&CmdContext.ArgV[0], _T("foo")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgV in '%y', have %y expected foo\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      &CmdContext.ArgV[0]);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&CmdContext.ArgV[1], _T("bar")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgV in '%y', have %y expected bar\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      &CmdContext.ArgV[0]);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.ArgContexts[0].Quoted != FALSE || CmdContext.ArgContexts[0].QuoteTerminated != FALSE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgContext in '%y', have %i,%i expected 0,0\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgContexts[0].Quoted,
                      CmdContext.ArgContexts[0].QuoteTerminated);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.ArgContexts[1].Quoted != FALSE || CmdContext.ArgContexts[1].QuoteTerminated != FALSE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgContext in '%y', have %i,%i expected 0,0\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgContexts[1].Quoted,
                      CmdContext.ArgContexts[1].QuoteTerminated);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArg != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArg '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArg);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArgOffset != 3) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArgOffset '%y', have %i expected 3\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArgOffset);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriLibShFreeCmdContext(&CmdContext);
    return TRUE;
}

/**
 A test variation to parse a command with one argument containing quotes and
 a space embedded partway.
 */
BOOLEAN
TestParseOneArgContainingQuotesCmd(VOID)
{
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    YORI_STRING InputString;

    //
    //  A command with quotes in the middle of a single argument
    //

    YoriLibConstantString(&InputString, _T("foo\" \"bar"));
    if (!YoriLibShParseCmdlineToCmdContext(&InputString, 7, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext failed on '%y'\n"),
                      __FILE__,
                      __LINE__,
                      &InputString);
        return FALSE;
    }

    if (CmdContext.ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgC '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgC);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&CmdContext.ArgV[0], _T("foo\" \"bar")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgV in '%y', have %y expected foo\" \"bar\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      &CmdContext.ArgV[0]);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.ArgContexts[0].Quoted != FALSE || CmdContext.ArgContexts[0].QuoteTerminated != FALSE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgContext in '%y', have %i,%i expected 0,0\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgContexts[0].Quoted,
                      CmdContext.ArgContexts[0].QuoteTerminated);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArg != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArg '%y', have %i expected 0\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArg);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArgOffset != 7) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArgOffset '%y', have %i expected 7\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArgOffset);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriLibShFreeCmdContext(&CmdContext);
    return TRUE;
}

/**
 A test variation to parse a command that starts and ends with a quote.
 */
BOOLEAN
TestParseOneArgEnclosedInQuotesCmd(VOID)
{
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    YORI_STRING InputString;

    YoriLibConstantString(&InputString, _T("\"foo\""));
    if (!YoriLibShParseCmdlineToCmdContext(&InputString, InputString.LengthInChars, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext failed on '%y'\n"),
                      __FILE__,
                      __LINE__,
                      &InputString);
        return FALSE;
    }

    if (CmdContext.ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgC '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgC);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&CmdContext.ArgV[0], _T("foo")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgV in '%y', have %y expected foo\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      &CmdContext.ArgV[0]);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.ArgContexts[0].Quoted == FALSE || CmdContext.ArgContexts[0].QuoteTerminated == FALSE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgContext in '%y', have %i,%i expected 1,1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgContexts[0].Quoted,
                      CmdContext.ArgContexts[0].QuoteTerminated);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArg != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArg '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArg);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArgOffset != 4) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArgOffset '%y', have %i expected 4\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArgOffset);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriLibShFreeCmdContext(&CmdContext);

    return TRUE;
}


/**
 A test variation to parse a command that starts with a quote that ends
 partway through an argument.
 */
BOOLEAN
TestParseOneArgWithStartingQuotesCmd(VOID)
{
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    YORI_STRING InputString;

    //
    //  A command starting with quotes that end partway through
    //

    YoriLibConstantString(&InputString, _T("\"Program Files\"\\foo"));
    if (!YoriLibShParseCmdlineToCmdContext(&InputString, 15, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext failed on '%y'\n"),
                      __FILE__,
                      __LINE__,
                      &InputString);
        return FALSE;
    }

    if (CmdContext.ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgC '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgC);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&CmdContext.ArgV[0], _T("Program Files\\foo")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgV in '%y', have %y expected Program Files\\foo\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      &CmdContext.ArgV[0]);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.ArgContexts[0].Quoted == FALSE || CmdContext.ArgContexts[0].QuoteTerminated == FALSE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgContext in '%y', have %i,%i expected 1,1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgContexts[0].Quoted,
                      CmdContext.ArgContexts[0].QuoteTerminated);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArg != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArg '%y', have %i expected 0\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArg);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArgOffset != 13) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArgOffset '%y', have %i expected 13\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArgOffset);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriLibShFreeCmdContext(&CmdContext);

    return TRUE;
}

/**
 A test variation to parse a command that starts with a quote that ends
 partway through an argument, where the argument only contains backslashes
 afterwards.
 */
BOOLEAN
TestParseOneArgWithStartingQuotesEndingSlashCmd(VOID)
{
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    YORI_STRING InputString;

    //
    //  A command starting with quotes that end partway through
    //

    YoriLibConstantString(&InputString, _T("\"Program Files\"\\"));
    if (!YoriLibShParseCmdlineToCmdContext(&InputString, 15, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext failed on '%y'\n"),
                      __FILE__,
                      __LINE__,
                      &InputString);
        return FALSE;
    }

    if (CmdContext.ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgC '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgC);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    //
    //  Because the parser is moving the quote to the end of the argument,
    //  it also needs to escape all of the backslashes (which are about to
    //  escape the quote.)
    //

    if (YoriLibCompareStringWithLiteral(&CmdContext.ArgV[0], _T("Program Files\\\\")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgV in '%y', have %y expected Program Files\\\\\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      &CmdContext.ArgV[0]);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.ArgContexts[0].Quoted == FALSE || CmdContext.ArgContexts[0].QuoteTerminated == FALSE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgContext in '%y', have %i,%i expected 1,1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgContexts[0].Quoted,
                      CmdContext.ArgContexts[0].QuoteTerminated);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArg != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArg '%y', have %i expected 0\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArg);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArgOffset != 13) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArgOffset '%y', have %i expected 13\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArgOffset);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriLibShFreeCmdContext(&CmdContext);

    return TRUE;
}

/**
 A test variation to parse a command that starts with a quote that ends
 partway through an argument, where the argument only contains carets
 afterwards.
 */
BOOLEAN
TestParseOneArgWithStartingQuotesEndingCaretCmd(VOID)
{
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    YORI_STRING InputString;

    //
    //  A command starting with quotes that end partway through
    //

    YoriLibConstantString(&InputString, _T("\"Program Files\"^"));
    if (!YoriLibShParseCmdlineToCmdContext(&InputString, 15, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext failed on '%y'\n"),
                      __FILE__,
                      __LINE__,
                      &InputString);
        return FALSE;
    }

    if (CmdContext.ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgC '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgC);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    //
    //  Because the parser is moving the quote to the end of the argument,
    //  it also needs to remove the caret (which would escape the quote.)
    //

    if (YoriLibCompareStringWithLiteral(&CmdContext.ArgV[0], _T("Program Files")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgV in '%y', have %y expected Program Files\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      &CmdContext.ArgV[0]);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.ArgContexts[0].Quoted == FALSE || CmdContext.ArgContexts[0].QuoteTerminated == FALSE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgContext in '%y', have %i,%i expected 1,1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgContexts[0].Quoted,
                      CmdContext.ArgContexts[0].QuoteTerminated);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArg != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArg '%y', have %i expected 0\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArg);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArgOffset != 14) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArgOffset '%y', have %i expected 13\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArgOffset);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriLibShFreeCmdContext(&CmdContext);

    return TRUE;
}


/**
 A test variation to parse a command that starts and ends with a quote that
 contains quotes in the middle.
 */
BOOLEAN
TestParseOneArgContainingAndEnclosedInQuotesCmd(VOID)
{
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    YORI_STRING InputString;

    YoriLibConstantString(&InputString, _T("\"foo\"==\"foo\" "));
    if (!YoriLibShParseCmdlineToCmdContext(&InputString, InputString.LengthInChars, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext failed on '%y'\n"),
                      __FILE__,
                      __LINE__,
                      &InputString);
        return FALSE;
    }

    if (CmdContext.ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgC '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgC);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&CmdContext.ArgV[0], _T("foo\"==\"foo")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgV in '%y', have %y expected foo\"==\"foo\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      &CmdContext.ArgV[0]);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.ArgContexts[0].Quoted == FALSE || CmdContext.ArgContexts[0].QuoteTerminated == FALSE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgContext in '%y', have %i,%i expected 1,1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgContexts[0].Quoted,
                      CmdContext.ArgContexts[0].QuoteTerminated);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArg != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArg '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArg);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    //
    //  MSFIX This hasn't changed as part of the latest changes but it seems
    //  like this should be zero
    //

    if (CmdContext.CurrentArgOffset != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArgOffset '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArgOffset);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriLibShFreeCmdContext(&CmdContext);

    return TRUE;
}

/**
 A test variation to parse a command that contains a quote and ends with a
 quote.
 */
BOOLEAN
TestParseRedirectWithEndingQuoteCmd(VOID)
{
    YORI_LIBSH_CMD_CONTEXT CmdContext;
    YORI_STRING InputString;

    YoriLibConstantString(&InputString, _T(">\"file name\""));
    if (!YoriLibShParseCmdlineToCmdContext(&InputString, InputString.LengthInChars, &CmdContext)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%hs:%i YoriLibShParseCmdlineToCmdContext failed on '%y'\n"), __FILE__, __LINE__, &InputString);
        return FALSE;
    }

    if (CmdContext.ArgC != 1) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgC '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgC);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteral(&CmdContext.ArgV[0], _T(">\"file name\"")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgV in '%y', have %y expected >\"file name\"\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      &CmdContext.ArgV[0]);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.ArgContexts[0].Quoted != FALSE || CmdContext.ArgContexts[0].QuoteTerminated != FALSE) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected ArgContext in '%y', have %i,%i expected 0,0\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.ArgContexts[0].Quoted,
                      CmdContext.ArgContexts[0].QuoteTerminated);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArg != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArg '%y', have %i expected 1\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArg);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    if (CmdContext.CurrentArgOffset != 12) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR,
                      _T("%hs:%i YoriLibShParseCmdlineToCmdContext returned unexpected CurrentArgOffset '%y', have %i expected 12\n"),
                      __FILE__,
                      __LINE__,
                      &InputString,
                      CmdContext.CurrentArgOffset);
        YoriLibShFreeCmdContext(&CmdContext);
        return FALSE;
    }

    YoriLibShFreeCmdContext(&CmdContext);

    return TRUE;
}

// vim:sw=4:ts=4:et:
