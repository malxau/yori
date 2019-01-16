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
        "DF [-license] [-m] [<drive>]\n"
        "\n"
        "   -m             Minimal display, raw data only\n";

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
 Context structure passed between each drive whose free space is being
 displayed.
 */
typedef struct _DF_CONTEXT {

    /**
     TRUE if the display should be minimal so that it can be easily parsed,
     with no human readability added.
     */
    BOOL MinimalDisplay;

    /**
     TRUE if a graph of space utilization should be displayed.
     */
    BOOL DisplayGraph;

    /**
     The width of the console, in characters.
     */
    DWORD ConsoleWidth;

    /**
     A buffer to generate the graph line into.  This is allocated when the
     app starts and contains ConsoleWidth chars plus space for two VT100
     escape sequences to initiate and terminate the color of the graph.
     */
    YORI_STRING LineBuffer;

    /**
     Color information to display against matching directories.
     */
    YORI_LIB_FILE_FILTER ColorRules;

} DF_CONTEXT, *PDF_CONTEXT;

/**
 Report the space usage on a single volume.

 @param VolName The volume name.  This can either be a volume GUID path
        returned from volume enumeration, or a user specified path to
        anything.

 @param DfContext Pointer to the context used to indicate options for each
        drive whose free space is being reported.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
DfReportSingleVolume(
    __in LPTSTR VolName,
    __in PDF_CONTEXT DfContext
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
    WIN32_FIND_DATA FindData;
    YORI_STRING VtAttribute;
    TCHAR VtAttributeBuffer[YORI_MAX_INTERNAL_VT_ESCAPE_CHARS];

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
    YoriLibInitEmptyString(&VtAttribute);
    VtAttribute.StartOfString = VtAttributeBuffer;
    VtAttribute.LengthAllocated = sizeof(VtAttributeBuffer)/sizeof(VtAttributeBuffer[0]);

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

        if (DfContext->MinimalDisplay) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%lli %lli %s\n"), TotalBytes, FreeBytes, NameToReport, NameToReport);
        } 

        while (TotalBytes.HighPart != 0) {
            TotalBytes.QuadPart = TotalBytes.QuadPart >> 1;
            FreeBytes.QuadPart = FreeBytes.QuadPart >> 1;
        }
        PercentageUsed = 1000 - (DWORD)(FreeBytes.QuadPart * 1000 / TotalBytes.QuadPart);

        if (!DfContext->MinimalDisplay) {
            LPTSTR FinalComponent;
            YORILIB_COLOR_ATTRIBUTES Attribute;
            YORI_STRING YsVolName;

            YoriLibUpdateFindDataFromFileInformation(&FindData, VolName, FALSE);
            FinalComponent = _tcsrchr(NameToReport, '\\');
            if (FinalComponent != NULL) {
                YoriLibSPrintfS(FindData.cFileName, MAX_PATH, _T("%s"), FinalComponent + 1);
            } else {
                YoriLibSPrintfS(FindData.cFileName, MAX_PATH, _T("%s"), NameToReport);
            }
            YoriLibConstantString(&YsVolName, VolName);
            if (!YoriLibFileFiltCheckColorMatch(&DfContext->ColorRules, &YsVolName, &FindData, &Attribute)) {
                Attribute.Ctrl = 0;
                Attribute.Win32Attr = (UCHAR)YoriLibVtGetDefaultColor();
            }
            YoriLibVtStringForTextAttribute(&VtAttribute, Attribute.Win32Attr);
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y total %y free %3i.%i%% used %y%s%c[0m\n"), &StrTotalSize, &StrFreeSize, PercentageUsed / 10, PercentageUsed % 10, &VtAttribute, NameToReport, 27);
        }

        if (DfContext->DisplayGraph) {
            YORI_STRING Subset;
            WORD Background;
            DWORD TotalBarSize;
            DWORD BarsSet;
            DWORD Index;

            DfContext->LineBuffer.StartOfString[0] = ' ';
            DfContext->LineBuffer.StartOfString[1] = '[';

            YoriLibInitEmptyString(&Subset);
            Subset.StartOfString = &DfContext->LineBuffer.StartOfString[2];
            Subset.LengthAllocated = DfContext->LineBuffer.LengthAllocated - 2;

            Background = (USHORT)(YoriLibVtGetDefaultColor() & 0xF0);

            if (PercentageUsed <= 700) {
                YoriLibVtStringForTextAttribute(&Subset, (USHORT)(Background | FOREGROUND_GREEN | FOREGROUND_INTENSITY));
            } else if (PercentageUsed <= 850) {
                YoriLibVtStringForTextAttribute(&Subset, (USHORT)(Background | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY));
            } else {
                YoriLibVtStringForTextAttribute(&Subset, (USHORT)(Background | FOREGROUND_RED | FOREGROUND_INTENSITY));
            }

            DfContext->LineBuffer.LengthInChars = 2 + Subset.LengthInChars;

            TotalBarSize = DfContext->ConsoleWidth - 4;
            BarsSet = TotalBarSize * PercentageUsed / 1000;

            for (Index = 0; Index < BarsSet; Index++) {
                DfContext->LineBuffer.StartOfString[Index + DfContext->LineBuffer.LengthInChars] = '#';
            }
            for (; Index < TotalBarSize; Index++) {
                DfContext->LineBuffer.StartOfString[Index + DfContext->LineBuffer.LengthInChars] = ' ';
            }
            DfContext->LineBuffer.LengthInChars += TotalBarSize;

            Subset.StartOfString = &DfContext->LineBuffer.StartOfString[DfContext->LineBuffer.LengthInChars];
            Subset.LengthAllocated = DfContext->LineBuffer.LengthAllocated - DfContext->LineBuffer.LengthInChars;

            Subset.LengthInChars = YoriLibSPrintf(Subset.StartOfString, _T("%c[0m]\n"), 27);
            DfContext->LineBuffer.LengthInChars += Subset.LengthInChars;

            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DfContext->LineBuffer);

        }
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
    DF_CONTEXT DfContext;
    YORI_STRING Combined;

    ZeroMemory(&DfContext, sizeof(DfContext));

    YoriLibGetWindowDimensions(GetStdHandle(STD_OUTPUT_HANDLE), &DfContext.ConsoleWidth, NULL);
    DfContext.DisplayGraph = TRUE;

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
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("m")) == 0) {
                DfContext.MinimalDisplay = TRUE;
                DfContext.DisplayGraph = FALSE;
                ArgumentUnderstood = TRUE;
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

    //
    //  Load the default color string and parse it into rules.
    //

    if (YoriLibLoadCombinedFileColorString(NULL, &Combined)) {
        YORI_STRING ErrorSubstring;
        YoriLibFileFiltParseColorString(&DfContext.ColorRules, &Combined, &ErrorSubstring);
        YoriLibFreeStringContents(&Combined);
    }

    if (DfContext.DisplayGraph) {
        if (!YoriLibAllocateString(&DfContext.LineBuffer, DfContext.ConsoleWidth + 2 * YORI_MAX_INTERNAL_VT_ESCAPE_CHARS)) {
            YoriLibFileFiltFreeFilter(&DfContext.ColorRules);
            return EXIT_FAILURE;
        }
    }

    if (StartArg != 0) {
        for (i = StartArg; i < ArgC; i++) {
            if (!DfReportSingleVolume(ArgV[i].StartOfString, &DfContext)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("df: Could not query %y\n"), &ArgV[i]);
            }
        }
    } else {
        FindHandle = YoriLibFindFirstVolume(VolName, sizeof(VolName)/sizeof(VolName[0]));
        if (FindHandle != INVALID_HANDLE_VALUE) {
            do {
                DfReportSingleVolume(VolName, &DfContext);
            } while(YoriLibFindNextVolume(FindHandle, VolName, sizeof(VolName)/sizeof(VolName[0])));
            YoriLibFindVolumeClose(FindHandle);
        }
    }

    YoriLibFileFiltFreeFilter(&DfContext.ColorRules);
    YoriLibFreeStringContents(&DfContext.LineBuffer);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
