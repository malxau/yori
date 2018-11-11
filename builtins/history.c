/**
 * @file builtins/history.c
 *
 * Yori shell history output
 *
 * Copyright (c) 2018 Malcolm J. Smith
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
CHAR strHistoryHelpText[] =
        "\n"
        "Displays or modifies recent command history.\n"
        "\n"
        "HISTORY [-license] [-l <file>|-n lines]\n"
        "\n"
        "   -l             Load history from a file\n"
        "   -n             The number of lines of history to output\n";

/**
 Display usage text to the user.
 */
BOOL
HistoryHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("History %i.%i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHistoryHelpText);
    return TRUE;
}

/**
 Load history from a file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
HistoryLoadHistoryFromFile(
    __in PYORI_STRING FilePath
    )
{
    HANDLE FileHandle;
    PVOID LineContext = NULL;
    YORI_STRING LineString;

    FileHandle = CreateFile(FilePath->StartOfString,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (FileHandle == NULL || FileHandle == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        if (LastError != ERROR_FILE_NOT_FOUND) {
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("history: open of %y failed: %s"), &FilePath, ErrText);
            YoriLibFreeWinErrorText(ErrText);
        }
        return FALSE;
    }

    YoriCallClearHistoryStrings();
    YoriLibInitEmptyString(&LineString);

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, FileHandle)) {
            break;
        }

        //
        //  If we fail to add to history, stop.
        //

        if (!YoriCallAddHistoryString(&LineString)) {
            break;
        }
    }

    YoriLibLineReadClose(LineContext);
    YoriLibFreeStringContents(&LineString);
    CloseHandle(FileHandle);
    return TRUE;
}

/**
 Display yori shell command history.

 @param ArgC The number of arguments.

 @param ArgV The argument array.

 @return ExitCode, zero for success, nonzero for failure.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_HISTORY(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    DWORD LineCount = 0;
    YORI_STRING Arg;
    YORI_STRING HistoryStrings;
    PYORI_STRING SourceFile = NULL;
    LPTSTR ThisVar;
    DWORD VarLen;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                HistoryHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                if (ArgC > i + 1) {
                    SourceFile = &ArgV[i + 1];
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                if (ArgC > i + 1) {
                    DWORD CharsConsumed;
                    LONGLONG llTemp;
                    YoriLibStringToNumber(&ArgV[i + 1], TRUE, &llTemp, &CharsConsumed);
                    LineCount = (DWORD)llTemp;
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
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (SourceFile != NULL) {
        YORI_STRING FileName;
        if (!YoriLibUserStringToSingleFilePath(SourceFile, TRUE, &FileName)) {
            DWORD LastError = GetLastError();
            LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("history: getfullpathname of %y failed: %s"), &ArgV[StartArg], ErrText);
            YoriLibFreeWinErrorText(ErrText);
            return EXIT_FAILURE;
        }
        if (!HistoryLoadHistoryFromFile(&FileName)) {
            YoriLibFreeStringContents(&FileName);
            return EXIT_FAILURE;
        }
        YoriLibFreeStringContents(&FileName);
    } else {
        if (YoriCallGetHistoryStrings(LineCount, &HistoryStrings)) {
            ThisVar = HistoryStrings.StartOfString;
            while (*ThisVar != '\0') {
                VarLen = _tcslen(ThisVar);
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s\n"), ThisVar);
                ThisVar += VarLen;
                ThisVar++;
            }
            YoriCallFreeYoriString(&HistoryStrings);
        }
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
