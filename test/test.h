/**
 * @file test/test.h
 *
 * Yori shell test header
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

/**
 Specifies the function signature for a test variation.
 */
typedef
BOOLEAN
YORI_TEST_FN(VOID);

/**
 A pointer to a test variation.
 */
typedef YORI_TEST_FN *PYORI_TEST_FN;

/**
 A test variation to enumerate files in the root.
 */
YORI_TEST_FN TestEnumRoot;

/**
 A test variation to enumerate files in the Windows directory.
 */
YORI_TEST_FN TestEnumWindows;

/**
 A test variation to parse a command with two space delimited arguments.
 */
YORI_TEST_FN TestParseTwoArgCmd;

/**
 A test variation to parse a command with one argument containing quotes and
 a space embedded partway.
 */
YORI_TEST_FN TestParseOneArgContainingQuotesCmd;

/**
 A test variation to parse a command that starts with a quote that ends
 partway through an argument.
 */
YORI_TEST_FN TestParseOneArgWithStartingQuotesCmd;

/**
 A test variation to parse a command that starts and ends with a quote.
 */
YORI_TEST_FN TestParseOneArgEnclosedInQuotesCmd;

/**
 A test variation to parse a command that starts with a quote that ends
 partway through an argument ending with backslashes.
 */
YORI_TEST_FN TestParseOneArgWithStartingQuotesEndingSlashCmd;

/**
 A test variation to parse a command that starts with a quote that ends
 partway through an argument ending with carets.
 */
YORI_TEST_FN TestParseOneArgWithStartingQuotesEndingCaretCmd;

/**
 A test variation to parse a command that starts and ends with a quote that
 contains quotes in the middle.
 */
YORI_TEST_FN TestParseOneArgContainingAndEnclosedInQuotesCmd;

/**
 A test variation to parse a command that contains a quote and ends with a
 quote.
 */
YORI_TEST_FN TestParseRedirectWithEndingQuoteCmd;

/**
 A test variation to parse a command with two space delimited arguments.
 */
YORI_TEST_FN TestArgTwoArgCmd;

/**
 A test variation to parse a command with one argument containing quotes and
 a space embedded partway.
 */
YORI_TEST_FN TestArgOneArgContainingQuotesCmd;

/**
 A test variation to parse a command that starts with a quote that ends
 partway through an argument.
 */
YORI_TEST_FN TestArgOneArgWithStartingQuotesCmd;

/**
 A test variation to parse a command that starts and ends with a quote.
 */
YORI_TEST_FN TestArgOneArgEnclosedInQuotesCmd;

/**
 A test variation to parse a command that contains a quote and ends with a
 quote.
 */
YORI_TEST_FN TestArgRedirectWithEndingQuoteCmd;

/**
 A test variation to parse a command containing backslash escapes.
 */
YORI_TEST_FN TestArgBackslashEscapeCmd;

// vim:sw=4:ts=4:et:
