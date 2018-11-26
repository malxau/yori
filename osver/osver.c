/**
 * @file osver/osver.c
 *
 * Yori shell display operating system version
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
CHAR strOsVerHelpText[] =
        "\n"
        "Outputs the operating system version in a specified format.\n"
        "\n"
        "OSVER [-license] [<fmt>]\n"
        "\n"
        "Format specifiers are:\n"
        "   $BUILD$        The build number with leading zero\n"
        "   $build$        The build number without leading zero\n"
        "   $MAJOR$        The major version with leading zero\n"
        "   $major$        The major version without leading zero\n"
        "   $MINOR$        The minor version with leading zero\n"
        "   $minor$        The minor version without leading zero\n";

/**
 Display usage text to the user.
 */
BOOL
OsVerHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("OsVer %i.%i\n"), OSVER_VER_MAJOR, OSVER_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strOsVerHelpText);
    return TRUE;
}

/**
 A context structure to pass to the function expanding variables so it knows
 what values to use.
 */
typedef struct _OSVER_VERSION_RESULT {

    /**
     The OS major version number.
     */
    DWORD MajorVersion;

    /**
     The OS minor version number.
     */
    DWORD MinorVersion;

    /**
     The OS build number.
     */
    DWORD BuildNumber;
} OSVER_VERSION_RESULT, *POSVER_VERSION_RESULT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputString A pointer to the output string to populate with data
        if a known variable is found.  The allocated length indicates the
        amount of the string that can be written to.

 @param VariableName The variable name to expand.

 @param Context Pointer to a @ref OSVER_VERSION_RESULT structure containing
        the data to populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
DWORD
OsVerExpandVariables(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in POSVER_VERSION_RESULT Context
    )
{
    DWORD CharsNeeded;
    if (YoriLibCompareStringWithLiteral(VariableName, _T("MAJOR")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("major")) == 0) {
        CharsNeeded = 3;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("MINOR")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("minor")) == 0) {
        CharsNeeded = 3;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("BUILD")) == 0) {
        CharsNeeded = 5;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("build")) == 0) {
        CharsNeeded = 5;
    } else {
        return 0;
    }

    if (OutputString->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringWithLiteral(VariableName, _T("MAJOR")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%02i"), Context->MajorVersion);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("MINOR")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%02i"), Context->MinorVersion);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("BUILD")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%05i"), Context->BuildNumber);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("major")) == 0) {
        if (Context->MajorVersion < 1000) {
            CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), Context->MajorVersion);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("minor")) == 0) {
        if (Context->MinorVersion < 1000) {
            CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), Context->MinorVersion);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("build")) == 0) {
        if (Context->MinorVersion < 100000) {
            CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), Context->BuildNumber);
        } else {
            CharsNeeded = 0;
        }
    }

    OutputString->LengthInChars = CharsNeeded;
    return CharsNeeded;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the osver builtin command.
 */
#define ENTRYPOINT YoriCmd_OSVER
#else
/**
 The main entrypoint for the osver standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the osver cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process indicating success or failure.
 */
DWORD
ENTRYPOINT(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    OSVER_VERSION_RESULT VersionResult;
    BOOL ArgumentUnderstood;
    LPTSTR FormatString = _T("\x1b[41;34;1m\x2584\x1b[42;33;1m\x2584\x1b[0m Windows version: $major$.$minor$.$build$\n");
    YORI_STRING DisplayString;
    DWORD i;
    YORI_STRING Arg;
    YORI_STRING YsFormatString;

    YoriLibInitEmptyString(&YsFormatString);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                OsVerHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            }
        } else {
            ArgumentUnderstood = TRUE;
            YsFormatString.StartOfString = ArgV[i].StartOfString;
            YsFormatString.LengthInChars = ArgV[i].LengthInChars;
            YsFormatString.LengthAllocated = ArgV[i].LengthAllocated;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (YsFormatString.StartOfString == NULL) {
        YoriLibConstantString(&YsFormatString, FormatString);
    }

    //
    //  MSFIX Have option for architecture
    //

    YoriLibInitEmptyString(&DisplayString);
    YoriLibGetOsVersion(&VersionResult.MajorVersion, &VersionResult.MinorVersion, &VersionResult.BuildNumber);
    YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, OsVerExpandVariables, &VersionResult, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        YoriLibFreeStringContents(&DisplayString);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
