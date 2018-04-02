/**
 * @file builtins/ver.c
 *
 * Yori shell version display
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
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strVerHelpText[] =
        "\n"
        "Outputs the Yori version in a specified format.\n"
        "\n"
        "VER [<fmt>]\n"
        "\n"
        "Format specifiers are:\n"
        "   $LIBMAJOR$     The YoriLib major version with leading zero\n"
        "   $libmajor$     The YoriLib major version without leading zero\n"
        "   $LIBMINOR$     The YoriLib minor version with leading zero\n"
        "   $libminor$     The YoriLib minor version without leading zero\n"
        "   $SHMAJOR$      The Yori shell major version with leading zero\n"
        "   $shmajor$      The Yori shell major version without leading zero\n"
        "   $SHMINOR$      The Yori shell minor version with leading zero\n"
        "   $shminor$      The Yori shell minor version without leading zero\n"
        "   $VERDATE$      The build date of the version module\n";

/**
 Display usage text to the user.
 */
BOOL
VerHelp()
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2017"), &License);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Ver %i.%i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strVerHelpText);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &License);
    YoriLibFreeStringContents(&License);
    return TRUE;
}

/**
 A context structure to pass to the function expanding variables so it knows
 what values to use.
 */
typedef struct _VER_VERSION_RESULT {

    /**
     The shell major version number.
     */
    DWORD ShMajorVersion;

    /**
     The shell minor version number.
     */
    DWORD ShMinorVersion;

    /**
     The lib major version number.
     */
    DWORD LibMajorVersion;

    /**
     The lib minor version number.
     */
    DWORD LibMinorVersion;

} VER_VERSION_RESULT, *PVER_VERSION_RESULT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputString A pointer to the output string to populate with data
        if a known variable is found, including the allocated length of
        the string.

 @param VariableName The variable name to expand.

 @param Context Pointer to a @ref VER_VERSION_RESULT structure containing
        the data to populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
DWORD
VerExpandVariables(
    __out PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PVER_VERSION_RESULT Context
    )
{
    DWORD CharsNeeded;
    if (YoriLibCompareStringWithLiteral(VariableName, _T("LIBMAJOR")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("libmajor")) == 0) {
        CharsNeeded = 3;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("LIBMINOR")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("libminor")) == 0) {
        CharsNeeded = 3;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SHMAJOR")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("shmajor")) == 0) {
        CharsNeeded = 3;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SHMINOR")) == 0) {
        CharsNeeded = 2;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("shminor")) == 0) {
        CharsNeeded = 3;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("VERDATE")) == 0) {
        CharsNeeded = strlen(__DATE__);
    } else {
        return 0;
    }

    if (OutputString->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringWithLiteral(VariableName, _T("LIBMAJOR")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%02i"), Context->LibMajorVersion);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("LIBMINOR")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%02i"), Context->LibMinorVersion);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("libmajor")) == 0) {
        if (Context->LibMajorVersion < 1000) {
            CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), Context->LibMajorVersion);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("libminor")) == 0) {
        if (Context->LibMinorVersion < 1000) {
            CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), Context->LibMinorVersion);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SHMAJOR")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%02i"), Context->ShMajorVersion);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SHMINOR")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%02i"), Context->ShMinorVersion);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("shmajor")) == 0) {
        if (Context->ShMajorVersion < 1000) {
            CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), Context->ShMajorVersion);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("shminor")) == 0) {
        if (Context->ShMinorVersion < 1000) {
            CharsNeeded = YoriLibSPrintf(OutputString->StartOfString, _T("%i"), Context->ShMinorVersion);
        } else {
            CharsNeeded = 0;
        }
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("VERDATE")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%hs"), __DATE__);
    }

    OutputString->LengthInChars = CharsNeeded;
    return CharsNeeded;
}

/**
 Entrypoint for the ver builtin command.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_VER(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    VER_VERSION_RESULT VersionResult;
    YORI_STRING DisplayString;
    LPTSTR FormatString = _T("YoriLib version: $LIBMAJOR$.$LIBMINOR$\n")
                          _T("Yori shell version: $SHMAJOR$.$SHMINOR$\n")
                          _T("Build date: $VERDATE$\n");
    YORI_STRING YsFormatString;
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                VerHelp();
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

    if (StartArg > 0) {
        YoriLibInitEmptyString(&YsFormatString);
        YsFormatString.StartOfString = ArgV[StartArg].StartOfString;
        YsFormatString.LengthInChars = ArgV[StartArg].LengthInChars;
        YsFormatString.LengthAllocated = ArgV[StartArg].LengthAllocated;
    } else {
        YoriLibConstantString(&YsFormatString, FormatString);
    }

    YoriCallGetYoriVersion(&VersionResult.ShMajorVersion, &VersionResult.ShMinorVersion);
    VersionResult.LibMajorVersion = YORI_VER_MAJOR;
    VersionResult.LibMinorVersion = YORI_VER_MINOR;

    YoriLibInitEmptyString(&DisplayString);
    YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VerExpandVariables, &VersionResult, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        YoriLibFreeStringContents(&DisplayString);
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
