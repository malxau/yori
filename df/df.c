/**
 * @file df/df.c
 *
 * Yori shell display disk free on volumes
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, DFESS OR
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
CHAR strDfHelpText[] =
        "\n"
        "Display disk free space.\n"
        "\n"
        "DF [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
DfHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Df %i.%i\n"), DF_VER_MAJOR, DF_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strDfHelpText);
    return TRUE;
}

/**
 Report the space usage on a single volume.

 @param VolName The volume name.  This can either be a volume GUID path
        returned from volume enumeration, or a user specified path to
        anything.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
DfReportSingleVolume(
    __in LPTSTR VolName
    )
{
    LARGE_INTEGER TotalBytes;
    LARGE_INTEGER FreeBytes;
    TCHAR TotalSizeBuffer[10];
    YORI_STRING StrTotalSize;
    TCHAR FreeSizeBuffer[10];
    YORI_STRING StrFreeSize;
    DWORD PercentageUsed;
    BOOL Result = FALSE;

    TCHAR MountPointName[MAX_PATH];
    LPTSTR NameToReport;

    //
    //  Prepare string buffers to hold the human readable sizes.
    //

    YoriLibInitEmptyString(&StrTotalSize);
    StrTotalSize.StartOfString = TotalSizeBuffer;
    StrTotalSize.LengthAllocated = sizeof(TotalSizeBuffer)/sizeof(TotalSizeBuffer[0]);
    YoriLibInitEmptyString(&StrFreeSize);
    StrFreeSize.StartOfString = FreeSizeBuffer;
    StrFreeSize.LengthAllocated = sizeof(FreeSizeBuffer)/sizeof(FreeSizeBuffer[0]);

    //
    //  If the OS supports it, try to translate the GUID volume names back
    //  into drive letters.
    //

    NameToReport = VolName;
    if (DllKernel32.pGetVolumePathNamesForVolumeNameW) {
        if (DllKernel32.pGetVolumePathNamesForVolumeNameW(VolName, MountPointName, sizeof(MountPointName)/sizeof(MountPointName[0]), &PercentageUsed)) {
            NameToReport = MountPointName;
        }
    }

    //
    //  Query free space and display the result.
    //

    if (YoriLibGetDiskFreeSpace(VolName, &FreeBytes, &TotalBytes, NULL)) {

        YoriLibFileSizeToString(&StrTotalSize, &TotalBytes);
        YoriLibFileSizeToString(&StrFreeSize, &FreeBytes);
        while (TotalBytes.HighPart != 0 && FreeBytes.HighPart != 0) {
            TotalBytes.QuadPart = TotalBytes.QuadPart >> 1;
            FreeBytes.QuadPart = FreeBytes.QuadPart >> 1;
        }
        PercentageUsed = 1000 - (DWORD)(FreeBytes.QuadPart * 1000 / TotalBytes.QuadPart);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y total %y free %3i.%i%% used %s\n"), &StrTotalSize, &StrFreeSize, PercentageUsed / 10, PercentageUsed % 10, NameToReport);
        Result = TRUE;
    }

    return Result;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the df builtin command.
 */
#define ENTRYPOINT YoriCmd_YDF
#else
/**
 The main entrypoint for the df standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the df cmdlet.

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
    DWORD StartArg = 0;
    YORI_STRING Arg;
    HANDLE FindHandle;
    TCHAR VolName[512];

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));
        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                DfHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                StartArg = i;
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

    if (StartArg != 0) {
        for (i = StartArg; i < ArgC; i++) {
            if (!DfReportSingleVolume(ArgV[i].StartOfString)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("df: Could not query %y\n"), &ArgV[i]);
            }
        }
    } else {
        FindHandle = YoriLibFindFirstVolume(VolName, sizeof(VolName)/sizeof(VolName[0]));
        if (FindHandle != INVALID_HANDLE_VALUE) {
            do {
                DfReportSingleVolume(VolName);
            } while(YoriLibFindNextVolume(FindHandle, VolName, sizeof(VolName)/sizeof(VolName[0])));
            YoriLibFindVolumeClose(FindHandle);
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
