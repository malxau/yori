/**
 * @file sdir/usage.c
 *
 * Colorful, sorted and optionally rich directory enumeration
 * for Windows.
 *
 * This module contains help text to display to the user for invalid arguments
 * or upon request.
 *
 * Copyright (c) 2014-2018 Malcolm J. Smith
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

#include "sdir.h"

/**
 The color to use when displaying a section header in usage text.
 */
#define SDIR_USAGE_HEADER_COLOR (BACKGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY)

/**
 The color to use when displaying a section header in usage text.
 */
YORILIB_COLOR_ATTRIBUTES SdirUsageHeaderColor = {0, SDIR_USAGE_HEADER_COLOR};

/**
 Display information about the current version of the program.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUsageVersionInfo(VOID)
{
    TCHAR   Line[160];

    YoriLibSPrintfS(Line,
                sizeof(Line)/sizeof(Line[0]),
                _T("Sdir version %i.%02i, compiled %hs\n"),
                YORI_VER_MAJOR,
                YORI_VER_MINOR,
                __DATE__
                );

    if (!SdirWriteString(Line)) {
        return FALSE;
    }

    return TRUE;
}

/**
 Display a section heading in usage text.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUsageHeader(
    LPCSTR strHeader
    )
{
    TCHAR   Line[80];
    DWORD   Padding;
    DWORD   MaxWidth = 80;
    BOOLEAN ExtraNewline = FALSE;

    //
    //  Write a newline with default color.
    //

    if (!SdirWriteString(_T("\n"))) {
        return FALSE;
    }

    //
    //  Now write the heading with desired color.
    //

    YoriLibSPrintfS(Line, sizeof(Line)/sizeof(Line[0]), _T("  === %hs ==="), strHeader);
    if (!SdirWriteStringWithAttribute(Line, SdirUsageHeaderColor)) {
        return FALSE;
    }

    Padding = (DWORD)_tcslen(Line) - 1;
    if (Opts->ConsoleBufferWidth > Padding) {

        //
        //  If the console will not auto wrap for us, we need an extra newline.
        //  If it will, just skip that part.
        //

        if (Opts->ConsoleBufferWidth > MaxWidth) {
            Padding = MaxWidth - Padding;
            ExtraNewline = TRUE;
        } else {
            Padding = Opts->ConsoleBufferWidth - Padding;
        }

        while (Padding > 0) {
            if (!SdirWriteStringWithAttribute(_T(" "), SdirUsageHeaderColor)) {
                return FALSE;
            }
            Padding--;
        }
    }

    //
    //  If we're outputting to a file, assume a newline is always needed
    //  since we won't have wrapping behavior.
    //

    if (!Opts->OutputHasAutoLineWrap) {
        ExtraNewline = TRUE;
    }

    if (!SdirWriteString(_T("\n"))) {
        return FALSE;
    }

    if (ExtraNewline && !SdirWriteString(_T("\n"))) {
        return FALSE;
    }

    return TRUE;
}


/**
 An ANSI string heading for the top level help options.
 */
const
CHAR strHelpHeader[] = "HELP OPTIONS";

/**
 An ANSI string for top level options.
 */
const
CHAR strHelpUsage1[] = 
                   "  Usage: [opts] [pathspec] ...\n"
                   "\n"
                   "  For additional help:\n"
                   "\n"
                   "   -opts        General command line options\n"
                   "   -display     Options to display metadata attributes\n"
                   "   -filecolor   Configuration information for color of each file\n"
                   "   -metacolor   Configuration information for metadata attributes\n"
                   "   -license     The license to use the software\n"
                   "   -sort        Options to sort files\n"
                   "\n"
                   "   -all         Display all help topics\n";

/**
 Display top level usage information.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUsageHelp(VOID)
{
    YORI_STRING str;

    YoriLibInitEmptyString(&str);

    if (!SdirUsageHeader(strHelpHeader)) {
        return FALSE;
    }

    YoriLibYPrintf(&str, _T("%hs"), strHelpUsage1);

    if (!SdirWriteString(str.StartOfString)) {
        YoriLibFreeStringContents(&str);
        return FALSE;
    }

    YoriLibFreeStringContents(&str);
    return TRUE;
}


/**
 An ANSI string heading for generic options.
 */
const
CHAR strCmdLineHeader[] = "COMMAND LINE OPTIONS";

/**
 An ANSI string for the first section of miscellaneous and generic options.
 */
const
CHAR strCmdLineOpts[] = 
                   "  Usage: [opts] [pathspec] ...\n"
                   "\n"
                   "   -?           Display help\n"
                   "\n"
                   "   -b           Use basic search criteria for files only\n"
                   "   -cw[num]     Width of console when writing to files\n"
                   "   -fc[string]  Apply custom file color string, see file color section\n"
                   "   -fe[string]  Exclude files matching criteria, see file color section\n"
                   "   -l/-ln       Traverse symbolic links and mount points when recursing\n"
                   "   -p/-pn       Pause/no pause after each screen\n"
                   "   -r           Recurse through directories when enumerating\n"
                   "   -t/-tn       Truncate/no truncate of very long file names\n"
#ifdef UNICODE
                   "   -u/-un       Unicode/no unicode output\n"
#endif
                   "   -v           Display version/build info and exit\n"
                   "";

/**
 An ANSI string for the second section of miscellaneous and generic options.
 */
const
CHAR strCmdLineUsage2[] = "\n"
                   " Options can also be in the SDIR_OPTS environment variable.\n"
                   " Processed in order, environment then arguments.\n";


/**
 Display usage information for generic options.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUsageOpts(VOID)
{
    YORI_STRING str;

    YoriLibInitEmptyString(&str);

    if (!SdirUsageHeader(strCmdLineHeader)) {
        return FALSE;
    }

    YoriLibYPrintf(&str, _T("%hs"), strCmdLineOpts);

    if (!SdirWriteString(str.StartOfString)) {
        YoriLibFreeStringContents(&str);
        return FALSE;
    }
    YoriLibFreeStringContents(&str);

    return TRUE;
}

/**
 An ANSI string heading for the license section.
 */
const
CHAR strLicenseHeader[] = "LICENSE";

/**
 Display license information.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUsageLicense(VOID)
{
    YORI_STRING License;

    YoriLibMitLicenseText(_T("2014-2021"), &License);

    if (!SdirUsageHeader(strLicenseHeader)) {
        YoriLibFreeStringContents(&License);
        return FALSE;
    }

    if (!SdirWriteString(License.StartOfString)) {
        YoriLibFreeStringContents(&License);
        return FALSE;
    }
    YoriLibFreeStringContents(&License);

    return TRUE;
}

/**
 An ANSI string heading for the metadata display section.
 */
const
CHAR strDisplayHeader[] = "DISPLAY OPTIONS";

/**
 Display usage information for information to display.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUsageDisplay(VOID)
{
    DWORD   i;
    TCHAR   Line[80];

    if (!SdirUsageHeader(strDisplayHeader)) {
        return FALSE;
    }

    for (i = 0; i < SdirGetNumSdirOptions(); i++) {
        if (SdirOptions[i].Default.Flags & SDIR_FEATURE_ALLOW_DISPLAY) {

            YoriLibSPrintfS(Line,
                        sizeof(Line)/sizeof(Line[0]),
                        _T("   -d%s/-h%s    Display/hide %hs%hs\n"),
                        SdirOptions[i].Switch,
                        SdirOptions[i].Switch,
                        SdirOptions[i].Help,
                        (SdirOptions[i].Default.Flags&SDIR_FEATURE_DISPLAY)?" (displayed by default)":"");

            if (!SdirWriteString(Line)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**
 An ANSI string heading for the file sorting section.
 */
const
CHAR strSortHeader[] = "SORT OPTIONS";

/**
 Display usage information for sorting files.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUsageSort(VOID)
{
    DWORD   i;
    TCHAR   strUsage[256];
    TCHAR   Line[80];

    if (!SdirUsageHeader(strSortHeader)) {
        return FALSE;
    }

    YoriLibSPrintfS(Line,
                sizeof(Line)/sizeof(Line[0]),
                _T(" Up to %i sort criteria can be applied.  Supported sort flags are:\n"),
                (int)(sizeof(Opts->Sort)/sizeof(Opts->Sort[0])));

    if (!SdirWriteString(Line)) {
        return FALSE;
    }

    for (i = 0; i < SdirGetNumSdirOptions(); i++) {
        if (SdirOptions[i].Default.Flags & SDIR_FEATURE_ALLOW_SORT) {

            YoriLibSPrintfS(Line,
                        sizeof(Line)/sizeof(Line[0]),
                        _T("   -s%s/-i%s    Sort/inverse sort by %hs\n"),
                        SdirOptions[i].Switch,
                        SdirOptions[i].Switch,
                        SdirOptions[i].Help);

            if (!SdirWriteString(Line)) {
                return FALSE;
            }
        }
    }

    SdirAnsiToUnicode(strUsage, sizeof(strUsage)/sizeof(strUsage[0]), strCmdLineUsage2);

    if (!SdirWriteString(strUsage)) {
        return FALSE;
    }

    return TRUE;
}

/**
 An ANSI string heading for the file color section.
 */
const
CHAR strFileColorHeader[] = "FILE COLOR CODING";

/**
 An ANSI string for the first section of information about file color.
 */
const
CHAR strFileColorUsage1[] = 
                   " Color coding for files is defined via three environment variables,\n"
                   " plus the command line, processed in order:\n"
                   "   YORICOLORPREPEND, processed first\n"
                   "   Command line -fc options, processed next\n"
                   "   YORICOLORREPLACE, if defined; otherwise, built in defaults apply\n"
                   "   YORICOLORAPPEND, processed last\n"
                   "\n"
                   " Each variable contains a semicolon delimited list of rules.  Each rule takes\n"
                   " the form of:\n"
                   "\n"
                   "   [file attribute][operator][criteria],[color]\n"
                   "\n"
                   " The -fe command line option is a shorthand form, which is equivalent to\n"
                   " specifying a color of 'hide', and is condensed to:\n"
                   "\n"
                   "   [file attribute][operator][criteria]\n"
                   "\n"
                   " Valid operators are:\n"
                   "   =   File attribute matches criteria\n"
                   "   !=  File attribute does not match criteria\n"
                   "   >   File attribute greater than criteria\n"
                   "   >=  File attribute greater than or equal to criteria\n"
                   "   <   File attribute less than criteria\n"
                   "   <=  File attribute less than or equal to criteria\n"
                   "   &   File attribute includes criteria or wildcard string\n"
                   "   !&  File attribute does not include criteria or wildcard string\n"
                   "\n"
                   " Valid attributes and corresponding operators are:\n";

/**
 An ANSI string for the second section of information about file color.
 */
const
CHAR strFileColorUsage2[] = "\n"
                   " Multiple colors and keywords can be combined, delimited by +. Valid colors\n"
                   " are:\n";

/**
 An ANSI string for the third section of information about file color.
 */
const
CHAR strFileColorUsage3[] = "\n"
                   " In addition to regular colors, special keywords can be included.\n"
                   " For file colors, these are:\n"
                   "\n"
                   "   bright    To display the file with a bright form of the color\n"
                   "   hide      To suppress a file from display\n"
                   "   invert    To swap background and foreground color\n"
                   "   window_bg To use the background color from the window\n"
                   "   window_fg To use the foreground color from the window\n"
                   "\n"
                   "   continue  To continue matching further rules and merge with subsequent\n"
                   "             results.\n"
                   "\n"
                   " The default set of file color attributes in this build is:\n";


/**
 Display usage information for file colors.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUsageFileColor(VOID)
{
    TCHAR   Line[80];
    YORI_STRING str;
    DWORD   i;
    LPCSTR  This;

    if (!SdirUsageHeader(strFileColorHeader)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&str, 2048)) {
        return FALSE;
    }

    YoriLibYPrintf(&str, _T("%hs"), strFileColorUsage1);

    if (!SdirWriteString(str.StartOfString)) {
        YoriLibFreeStringContents(&str);
        return FALSE;
    }

    //
    //  Display supported options and operators
    //

    for (i = 0; i < SdirGetNumSdirOptions(); i++) {
        if (SdirOptions[i].GenerateFromStringFn != NULL) {

            YoriLibSPrintfS(Line,
                        sizeof(Line)/sizeof(Line[0]),
                        _T("   %s (%hs), %hs%hs%hs\n"),
                        SdirOptions[i].Switch,
                        SdirOptions[i].Help,
                        SdirOptions[i].CompareFn?"=, !=, >, >=, <, <=":"",
                        (SdirOptions[i].CompareFn && SdirOptions[i].BitwiseCompareFn)?", ":"",
                        SdirOptions[i].BitwiseCompareFn?"&, !&":"");

            if (!SdirWriteString(Line)) {
                YoriLibFreeStringContents(&str);
                return FALSE;
            }
        }
    }

    YoriLibYPrintf(&str, _T("%hs"), strFileColorUsage2);
    if (!SdirWriteString(str.StartOfString)) {
        YoriLibFreeStringContents(&str);
        return FALSE;
    }

    //
    //  Display supported colors
    //

    for (i = 0; i < 8; i++) {

        YORILIB_COLOR_ATTRIBUTES Attr;

        YoriLibSPrintfS(Line,
                    sizeof(Line)/sizeof(Line[0]),
                    _T("   %-16s"),
                    YoriLibColorStringTable[i].String);

        if (i==0) {
            YoriLibSetColorToWin32(&Attr, SDIR_DEFAULT_COLOR);
        } else {
            YoriLibSetColorToWin32(&Attr, (UCHAR)i);
        }

        SdirWriteStringWithAttribute(Line, Attr);

        YoriLibSPrintfS(Line,
                    sizeof(Line)/sizeof(Line[0]),
                    _T(" %-16s"),
                    YoriLibColorStringTable[i+8].String);

        YoriLibSetColorToWin32(&Attr, (UCHAR)(i + 8));

        SdirWriteStringWithAttribute(Line, Attr);

        YoriLibSPrintfS(Line,
                    sizeof(Line)/sizeof(Line[0]),
                    _T(" bg_%-16s"),
                    YoriLibColorStringTable[i].String);

        YoriLibSetColorToWin32(&Attr, (UCHAR)((i<<4) + 0xF));

        SdirWriteStringWithAttribute(Line, Attr);

        YoriLibSPrintfS(Line,
                    sizeof(Line)/sizeof(Line[0]),
                    _T(" bg_%-16s"),
                    YoriLibColorStringTable[i+8].String);

        YoriLibSetColorToWin32(&Attr, (UCHAR)((i+8)<<4));

        SdirWriteStringWithAttribute(Line, Attr);
        if (!SdirWriteString(_T("\n"))) {
            YoriLibFreeStringContents(&str);
            return FALSE;
        }
    }


    YoriLibYPrintf(&str, _T("%hs"), strFileColorUsage3);
    if (!SdirWriteString(str.StartOfString)) {
        YoriLibFreeStringContents(&str);
        return FALSE;
    }
    YoriLibFreeStringContents(&str);

    //
    //  We want to display built in rules, but for formatting's sake display
    //  one per line.  This is the kludge to make that happen.
    //

    This = YoriLibGetDefaultFileColorString();

    YoriLibSPrintfS(Line,
                sizeof(Line)/sizeof(Line[0]),
                _T("   "));

    while (This != NULL) {

        //
        //  Copy characters including the semicolon, and add a newline and
        //  NULL.  We have three spaces at the beginning, so we need 6
        //  extra characters plus the size of the element itself.  It'd be
        //  nice if msvcrt didn't puke when the source string is larger
        //  than the specified size so we could just use a single
        //  stprintf_s for both cases.
        //

        for (i = 0; This[i] != '\0' && This[i] != ';'; i++) {
            Line[i + 3] = This[i];
        }
        Line[i + 3] = ';';
        Line[i + 4] = '\n';
        Line[i + 5] = '\0';

        if (!SdirWriteString(Line)) {
            return FALSE;
        }

        This += i;
        if (*This) {
            This++;
        } else {
            break;
        }
    }


    return TRUE;
}

/**
 An ANSI string heading for the metadata color section.
 */
const
CHAR strMetaColorHeader[] = "METADATA COLOR CODING";

/**
 An ANSI string for the first section of information about metadata color.
 */
const
CHAR strMetaColorUsage1[] =
                   " Color coding for metadata attributes is defined via YORICOLORMETADATA.\n"
                   " This variable also contains a semicolon delimited list of rules.  Each rule\n"
                   " takes the form of:\n"
                   "\n"
                   "   [file attribute],[color]\n"
                   "\n"
                   " Valid metadata attributes and their current defaults are:\n";

/**
 An ANSI string for the second section of information about metadata color.
 */
const
CHAR strMetaColorUsage2[] = "\n"
                   " For metadata colors, the keyword 'file' indicates to use the file color,\n"
                   " and not apply any specific metadata color.\n";


/**
 Display usage information for file metadata colors.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUsageMetaColor(VOID)
{
    TCHAR   Line[80];
    TCHAR   strUsage[80];
    YORI_STRING str;
    DWORD   i;

    if (!SdirUsageHeader(strMetaColorHeader)) {
        return FALSE;
    }

    YoriLibInitEmptyString(&str);
    YoriLibYPrintf(&str, _T("%hs"), strMetaColorUsage1);
    if (!SdirWriteString(str.StartOfString)) {
        YoriLibFreeStringContents(&str);
        return FALSE;
    }

    for (i = 0; i < SdirGetNumSdirOptions(); i++) {
        if ((SdirOptions[i].Default.Flags & SDIR_FEATURE_FIXED_COLOR) == 0) {

            PSDIR_FEATURE Feature;

            Feature = SdirFeatureByOptionNumber(i);

            SdirColorStringFromFeature(&SdirOptions[i].Default, strUsage, sizeof(strUsage)/sizeof(strUsage[0]));

            YoriLibSPrintfS(Line,
                        sizeof(Line)/sizeof(Line[0]),
                        _T("   %-25hs %s,%s;\n"),
                        SdirOptions[i].Help,
                        SdirOptions[i].Switch,
                        strUsage);

            if (!SdirWriteString(Line)) {
                YoriLibFreeStringContents(&str);
                return FALSE;
            }
        }
    }

    YoriLibYPrintf(&str, _T("%hs"), strMetaColorUsage2);
    if (!SdirWriteString(str.StartOfString)) {
        YoriLibFreeStringContents(&str);
        return FALSE;
    }
    YoriLibFreeStringContents(&str);

    return TRUE;
}

/**
 Display usage information.

 @param ArgC Count of arguments.

 @param ArgV Array of arguments.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
SdirUsage(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    YORI_STRING Arg;
    YORI_ALLOC_SIZE_T CurrentArg;
    BOOLEAN DisplayOptionsHelp = FALSE;
    BOOLEAN DisplayDisplayHelp = FALSE;
    BOOLEAN DisplaySortHelp = FALSE;
    BOOLEAN DisplayFileColorHelp = FALSE;
    BOOLEAN DisplayLicenseHelp = FALSE;
    BOOLEAN DisplayMetaColorHelp = FALSE;
    BOOLEAN DisplayOnlineHelp = FALSE;
    BOOLEAN DisplaySomething = FALSE;

    //
    //  Display current version.
    //

    if (!SdirUsageVersionInfo()) {
        return FALSE;
    }

    //
    //  Check if the user wants detailed information on something.
    //

    for (CurrentArg = 1; CurrentArg < ArgC; CurrentArg++) {

        if (YoriLibIsCommandLineOption(&ArgV[CurrentArg], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("opts")) == 0) {
                DisplayOptionsHelp = TRUE;
                DisplaySomething = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("display")) == 0) {
                DisplayDisplayHelp = TRUE;
                DisplaySomething = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("sort")) == 0) {
                DisplaySortHelp = TRUE;
                DisplaySomething = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("filecolor")) == 0) {
                DisplayFileColorHelp = TRUE;
                DisplaySomething = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("metacolor")) == 0) {
                DisplayMetaColorHelp = TRUE;
                DisplaySomething = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                DisplayLicenseHelp = TRUE;
                DisplaySomething = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("v")) == 0) {
                DisplaySomething = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("all")) == 0) {
                DisplayOptionsHelp = TRUE;
                DisplayLicenseHelp = TRUE;
                DisplayDisplayHelp = TRUE;
                DisplaySortHelp = TRUE;
                DisplayFileColorHelp = TRUE;
                DisplayMetaColorHelp = TRUE;
                DisplayOnlineHelp = TRUE;
                DisplaySomething = TRUE;
            }
        }
    }

    //
    //  If not, display help about what help topics they can select
    //  and return.
    //

    if (!DisplaySomething) {
        return SdirUsageHelp();
    }

    //
    //  Otherwise, display the detailed topics.
    //

    if (DisplayLicenseHelp && !SdirUsageLicense()) {
        return FALSE;
    }

    if (DisplayOptionsHelp && !SdirUsageOpts()) {
        return FALSE;
    }

    if (DisplayDisplayHelp && !SdirUsageDisplay()) {
        return FALSE;
    }

    if (DisplaySortHelp && !SdirUsageSort()) {
        return FALSE;
    }

    if (DisplayFileColorHelp && !SdirUsageFileColor()) {
        return FALSE;
    }

    if (DisplayMetaColorHelp && !SdirUsageMetaColor()) {
        return FALSE;
    }

    return TRUE;
}

// vim:sw=4:ts=4:et:
