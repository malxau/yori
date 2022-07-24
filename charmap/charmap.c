/**
 * @file charmap/charmap.c
 *
 * Yori shell display characters from the character map
 *
 * Copyright (c) 2019-2022 Malcolm J. Smith
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
CHAR strCharMapHelpText[] =
        "\n"
        "Displays a set of characters.\n"
        "\n"
        "YCHARMAP [-license] [-c <count>] [-e <encoding>] [-s <start>]\n"
        "\n"
        "   -c <count>     The number of characters to display\n"
        "   -e <encoding>  Specifies the encoding of the range\n"
        "   -s <start>     The first character number to display\n";

/**
 Display usage text to the user.
 */
BOOL
CharMapHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("CharMap %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strCharMapHelpText);
    return TRUE;
}

/**
 Displays a character map.

 @param Encoding Specifies the character encoding of the range of characters
        to display.  This can be CP_UTF16, indicating that the range should
        be a range of USHORT values.  It can also be any value supported by
        MultiByteToWideChar to indicate that these are UCHAR values that need
        to be translated into the USHORT range.

 @param StartChar The first character number to display.  This can be any
        USHORT value for CP_UTF16, or any UCHAR value for other encodings.

 @param CharCount The number of characters to display.  This will be truncated
        to USHORT_MAX for CP_UTF16 or UCHAR_MAX for anything else.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
CharMapDisplay(
    __in DWORD Encoding,
    __in DWORD StartChar,
    __in DWORD CharCount
    )
{
    CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
    LPSTR NarrowCharArray = NULL;
    DWORD ColumnIndex;
    DWORD ColumnCount;
    DWORD i;
    YORI_STRING WideChars;
    TCHAR CharToDisplay;

    //
    //  Calculate the number of columns to display.
    //

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &BufferInfo)) {
        DWORD RoundedColumnCount;
        DWORD LengthNeededPerEntry;

        if (Encoding == CP_UTF16) {
            LengthNeededPerEntry = 11;
        } else {
            LengthNeededPerEntry = 15;
        }
        ColumnCount = (BufferInfo.srWindow.Right - BufferInfo.srWindow.Left) / LengthNeededPerEntry;
        RoundedColumnCount = (ColumnCount / 4) * 4;
        if (RoundedColumnCount == 0) {
            RoundedColumnCount = (ColumnCount / 2) * 2;
        }
        if (RoundedColumnCount == 0) {
            RoundedColumnCount = 1;
        }
        ColumnCount = RoundedColumnCount;
    } else {
        ColumnCount = 1;
    }

    if (!YoriLibAllocateString(&WideChars, CharCount)) {
        return FALSE;
    }

    //
    //  Cap the range that can be displayed, and populate the array of wide
    //  chars to display.
    //

    if (Encoding == CP_UTF16) {
        if (StartChar > 0xffff) {
            YoriLibFreeStringContents(&WideChars);
            return EXIT_FAILURE;
        }

        if (StartChar + CharCount > 0xffff) {
            CharCount = 0x10000 - StartChar;
        }
        for (i = 0; i < CharCount; i++) {
            WideChars.StartOfString[i] = (TCHAR)(StartChar + i);
        }
    } else {
        if (StartChar > 0xff) {
            YoriLibFreeStringContents(&WideChars);
            return EXIT_FAILURE;
        }

        if (StartChar + CharCount > 0xff) {
            CharCount = 0x100 - StartChar;
        }
        NarrowCharArray = YoriLibMalloc(CharCount);
        if (NarrowCharArray == NULL) {
            YoriLibFreeStringContents(&WideChars);
            return EXIT_FAILURE;
        }

        for (i = 0; i < CharCount; i++) {
            NarrowCharArray[i] = (UCHAR)(StartChar + i);
        }

        MultiByteToWideChar(Encoding, 0, NarrowCharArray, CharCount, WideChars.StartOfString, WideChars.LengthAllocated);
    }

    //
    //  Display the characters.  Note there are some nonprinting chars that
    //  are really unsafe to display, and these are substituted with spaces.
    //

    ColumnIndex = 0;
    for (i = 0; i < CharCount; i++) {
        CharToDisplay = WideChars.StartOfString[i];
        if (!YoriLibIsCharPrintable(CharToDisplay)) {
            CharToDisplay = ' ';
        }

        if (Encoding == CP_UTF16) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT | YORI_LIB_OUTPUT_PASSTHROUGH_VT, _T(" %c 0x%04x"), CharToDisplay, WideChars.StartOfString[i]);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT | YORI_LIB_OUTPUT_PASSTHROUGH_VT, _T(" %c %3i 0x%04x"), CharToDisplay, StartChar + i, WideChars.StartOfString[i]);
        }
        ColumnIndex++;
        if (ColumnIndex < ColumnCount) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  "));
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
            ColumnIndex = 0;
        }
    }

    YoriLibFreeStringContents(&WideChars);
    if (NarrowCharArray != NULL) {
        YoriLibFree(NarrowCharArray);
    }

    return TRUE;
}

/**
 Parse a user specified argument into an encoding identifier.

 @param String The string to parse.

 @return The encoding identifier.  -1 is used to indicate failure.
 */
DWORD
CharMapEncodingFromString(
    __in PYORI_STRING String
    )
{
    if (YoriLibCompareStringWithLiteralInsensitive(String, _T("ascii")) == 0) {
        return CP_OEMCP;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("ansi")) == 0) {
        return CP_ACP;
    } else if (YoriLibCompareStringWithLiteralInsensitive(String, _T("utf16")) == 0) {
        return CP_UTF16;
    }
    return (DWORD)-1;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the charmap builtin command.
 */
#define ENTRYPOINT YoriCmd_YCHARMAP
#else
/**
 The main entrypoint for the charmap standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the charmap cmdlet.

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
    YORI_STRING Arg;
    DWORD Encoding;
    DWORD CharCount;
    DWORD StartChar;

    Encoding = CP_OEMCP;
    StartChar = 0;
    CharCount = 256;

    for (i = 1; i < ArgC; i++) {

        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                CharMapHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;

            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    DWORD CharsConsumed;
                    LONGLONG llTemp;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
                        CharCount = (DWORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                if (ArgC > i + 1) {
                    DWORD NewEncoding;
                    NewEncoding = CharMapEncodingFromString(&ArgV[i + 1]);
                    if (NewEncoding != (DWORD)-1) {
                        Encoding = NewEncoding;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                if (ArgC > i + 1) {
                    DWORD CharsConsumed;
                    LONGLONG llTemp;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed) && CharsConsumed > 0) {
                        StartChar = (DWORD)llTemp;
                        ArgumentUnderstood = TRUE;
                        i++;
                    }
                }
            }
        } else {
            ArgumentUnderstood = TRUE;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), ArgV[i]);
        }
    }

    if (!CharMapDisplay(Encoding, StartChar, CharCount)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
