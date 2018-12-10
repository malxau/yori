/**
 * @file vol/vol.c
 *
 * Yori shell display volume properties
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
CHAR strVolHelpText[] =
        "\n"
        "Outputs volume information in a specified format.\n"
        "\n"
        "VOL [-license] [-f <fmt>] [<vol>]\n"
        "\n"
        "Format specifiers are:\n"
        "   $label$        The volume label\n"
        "   $fsname$       The file system name\n"
        "   $serial$       The 32 bit serial number\n";

/**
 Display usage text to the user.
 */
BOOL
VolHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Vol %i.%i\n"), VOL_VER_MAJOR, VOL_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strVolHelpText);
    return TRUE;
}

/**
 A context structure to pass to the function expanding variables so it knows
 what values to use.
 */
typedef struct _VOL_RESULT {

    /**
     The volume label for this volume.
     */
    YORI_STRING VolumeLabel;

    /**
     A yori string containing the file system name, for example "FAT" or
     "NTFS".
     */
    YORI_STRING FsName;

    /**
     A 32 bit volume serial number.  NT internally uses 64 bit serial numbers,
     this is what's returned from Win32.
     */
    DWORD ShortSerialNumber;

    /**
     The file system capability flags.
     */
    DWORD Capabilities;

} VOL_RESULT, *PVOL_RESULT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputString A pointer to the output string to populate with data
        if a known variable is found.  The length allocated contains the
        length that can be populated with data.

 @param VariableName The variable name to expand.

 @param Context Pointer to a @ref VOL_RESULT structure containing
        the data to populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
DWORD
VolExpandVariables(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    DWORD CharsNeeded;
    PVOL_RESULT VolContext = (PVOL_RESULT)Context;

    if (YoriLibCompareStringWithLiteral(VariableName, _T("fsname")) == 0) {
        CharsNeeded = VolContext->FsName.LengthInChars;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("label")) == 0) {
        CharsNeeded = VolContext->VolumeLabel.LengthInChars;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("serial")) == 0) {
        CharsNeeded = 8;
    } else {
        return 0;
    }

    if (OutputString->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringWithLiteral(VariableName, _T("fsname")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%y"), &VolContext->FsName);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("label")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%y"), &VolContext->VolumeLabel);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("serial")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%08x"), VolContext->ShortSerialNumber);
    }

    OutputString->LengthInChars = CharsNeeded;
    return CharsNeeded;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the vol builtin command.
 */
#define ENTRYPOINT YoriCmd_YVOL
#else
/**
 The main entrypoint for the vol standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the vol cmdlet.

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
    VOL_RESULT VolResult;
    BOOL ArgumentUnderstood;
    DWORD MaxComponentLength;
    LPTSTR FormatString = _T("File system: $fsname$\n")
                          _T("Label: $label$\n")
                          _T("Serial number: $serial$\n");
    YORI_STRING DisplayString;
    YORI_STRING YsFormatString;
    LPTSTR VolName = _T("\\");
    DWORD StartArg = 0;
    DWORD i;
    YORI_STRING Arg;

    YoriLibInitEmptyString(&YsFormatString);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                VolHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (ArgC > i + 1) {
                    YsFormatString.StartOfString = ArgV[i + 1].StartOfString;
                    YsFormatString.LengthInChars = ArgV[i + 1].LengthInChars;
                    YsFormatString.LengthAllocated = ArgV[i + 1].LengthAllocated;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (YsFormatString.StartOfString == NULL) {
        YoriLibConstantString(&YsFormatString, FormatString);
    }

    if (StartArg > 0) {
        VolName = ArgV[StartArg].StartOfString;
    }

    if (!YoriLibAllocateString(&VolResult.VolumeLabel, 256)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&VolResult.FsName, 256)) {
        YoriLibFreeStringContents(&VolResult.VolumeLabel);
        return FALSE;
    }

    if (!GetVolumeInformation(VolName, VolResult.VolumeLabel.StartOfString, VolResult.VolumeLabel.LengthAllocated, &VolResult.ShortSerialNumber, &MaxComponentLength, &VolResult.Capabilities, VolResult.FsName.StartOfString, VolResult.FsName.LengthAllocated)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vol: GetVolumeInformation failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&VolResult.VolumeLabel);
        YoriLibFreeStringContents(&VolResult.FsName);
        return EXIT_FAILURE;
    }

    VolResult.VolumeLabel.LengthInChars = _tcslen(VolResult.VolumeLabel.StartOfString);
    VolResult.FsName.LengthInChars = _tcslen(VolResult.FsName.StartOfString);

    YoriLibInitEmptyString(&DisplayString);
    YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VolExpandVariables, &VolResult, &DisplayString);
    if (DisplayString.StartOfString != NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
        YoriLibFreeStringContents(&DisplayString);
    }
    YoriLibFreeStringContents(&VolResult.VolumeLabel);
    YoriLibFreeStringContents(&VolResult.FsName);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
