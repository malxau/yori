/**
 * @file err/err.c
 *
 * Yori shell display windows error codes as strings
 *
 * Copyright (c) 2019 Malcolm J. Smith
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
CHAR strErrHelpText[] =
        "\n"
        "Display error codes as strings.\n"
        "\n"
        "ERR [-license] [-n|-s|-w] <error>\n"
        "\n"
        "   -n             The error refers to an NTSTATUS code\n"
        "   -s             The error refers to an NTSTATUS code\n"
        "   -w             The error refers to a Win32 error code\n";

/**
 Display usage text to the user.
 */
BOOL
ErrHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Err %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strErrHelpText);
    return TRUE;
}

/**
 Heuristically guess if a number is hexadecimal.  Errors often take this form
 and demanding an 0x prefix for each of them seems inconvenient and counter-
 intuitive.  Note that a user can explicitly override this function in either
 direction with 0x or 0n prefixes.

 @param NumberString Pointer to the number in string form to check.

 @return TRUE to indicate the number should be treated as a hex number, FALSE
         to indicate that regular number parsing should be applied, which
         defaults to decimal.
 */
BOOL
ErrIsNumberProbablyHex(
    __in PYORI_STRING NumberString
    )
{
    DWORD Index;

    //
    //  An 8 character string is probably hex, because that corresponds to
    //  a 32 bit number, and both NTSTATUS and HRESULT codes indicate an
    //  error by setting the high bit so the entire 32 bits needs to be
    //  specified.  Win32 doesn't require the high bit set, but is also
    //  not conventionally specified as hex.
    //

    if (NumberString->LengthInChars != sizeof(DWORD) * 2) {
        return FALSE;
    }

    //
    //  This is checking if the number is a legal hex number, but what it's
    //  really looking for is an explicit prefix indicating it's some other
    //  type of number.
    //

    for (Index = 0; Index < NumberString->LengthInChars; Index++) {
        if ((NumberString->StartOfString[Index] < '0' || NumberString->StartOfString[Index] > '9') &&
            (NumberString->StartOfString[Index] < 'a' || NumberString->StartOfString[Index] > 'f') &&
            (NumberString->StartOfString[Index] < 'A' || NumberString->StartOfString[Index] > 'F')) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 The type of the error whose string should be located.
 */
typedef enum {
    ErrTypeUnknown = 0,
    ErrTypeWindows = 1,
    ErrTypeNtstatus = 2
} ERR_TYPE;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the err builtin command.
 */
#define ENTRYPOINT YoriCmd_YERR
#else
/**
 The main entrypoint for the err standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the err cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero indicating success or nonzero on
         failure.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 0;
    YORI_STRING Arg;
    YORI_MAX_SIGNED_T llTemp;
    YORI_ALLOC_SIZE_T CharsConsumed;
    DWORD Error;
    ERR_TYPE ErrType;
    LPTSTR ErrText;

    ErrType = ErrTypeUnknown;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringLitIns(&Arg, _T("?")) == 0) {
                ErrHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("n")) == 0) {
                ErrType = ErrTypeNtstatus;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("s")) == 0) {
                ErrType = ErrTypeNtstatus;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitIns(&Arg, _T("w")) == 0) {
                ErrType = ErrTypeWindows;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringLitInsCnt(&Arg, _T("0"), 1) >= 0 &&
                       YoriLibCompareStringLitInsCnt(&Arg, _T("9"), 1) <= 0) {
                StartArg = i;
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

    if (StartArg == 0 || StartArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("err: Missing argument\n"));
        return EXIT_FAILURE;
    }

    if (ErrIsNumberProbablyHex(&ArgV[StartArg])) {
        if (!YoriLibStringToNumberBase(&ArgV[StartArg], 16, TRUE, &llTemp, &CharsConsumed) || CharsConsumed == 0) {
            if (!YoriLibStringToNumber(&ArgV[StartArg], TRUE, &llTemp, &CharsConsumed) || CharsConsumed == 0) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("err: Argument not understood: %y\n"), &ArgV[StartArg]);
                return EXIT_FAILURE;
            }
        }
    } else if (!YoriLibStringToNumber(&ArgV[StartArg], TRUE, &llTemp, &CharsConsumed) || CharsConsumed == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("err: Argument not understood: %y\n"), &ArgV[StartArg]);
        return EXIT_FAILURE;
    }

    Error = (DWORD)llTemp;
    ErrText = NULL;

    if (ErrType == ErrTypeUnknown) {
        if (Error >= 0x80000000) {
            ErrType = ErrTypeNtstatus;
        } else {
            ErrType = ErrTypeWindows;
        }
    }

    switch(ErrType) {
        case ErrTypeWindows:
            ErrText = YoriLibGetWinErrorText(Error);
            break;
        case ErrTypeNtstatus:
            ErrText = YoriLibGetNtErrorText(Error);
            break;
    }

    if (ErrText != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
    } else {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not fetch error text.\n"));
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
