/**
 * @file tee/tee.c
 *
 * Yori shell output to a file and stdout
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
CHAR strTeeHelpText[] =
        "\n"
        "Output the contents of standard input to standard output and a file.\n"
        "\n"
        "TEE [-license] [-a] <file>\n"
        "\n"
        "   -a             Append to the file\n";

/**
 Display usage text to the user.
 */
BOOL
TeeHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Tee %i.%i\n"), TEE_VER_MAJOR, TEE_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strTeeHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each source stream
 processed.
 */
typedef struct _TEE_CONTEXT {

    /**
     Handle to a file which will receive all output in addition to standard
     output.
     */
    HANDLE hFile;

} TEE_CONTEXT, *PTEE_CONTEXT;

/**
 Process a single stream.

 @param hSource Handle to the source.

 @param TeeContext Pointer to the context for the operation, including a
        handle to the output stream to write data to.

 @return TRUE to indicate success or FALSE to indicate failure.
 */
BOOL
TeeProcessStream(
    __in HANDLE hSource,
    __in PTEE_CONTEXT TeeContext
    )
{
    PVOID LineContext = NULL;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    YORI_STRING LineString;

    YoriLibInitEmptyString(&LineString);

    while (TRUE) {

        if (!YoriLibReadLineToString(&LineString, &LineContext, hSource)) {
            break;
        }

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &LineString);
        if (LineString.LengthInChars == 0 || !GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo) || ScreenInfo.dwCursorPosition.X != 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n"));
        }

        YoriLibOutputToDevice(TeeContext->hFile, 0, _T("%y\n"), &LineString);
    }

    YoriLibLineReadClose(LineContext);
    YoriLibFreeStringContents(&LineString);

    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the for builtin command.
 */
#define ENTRYPOINT YoriCmd_TEE
#else
/**
 The main entrypoint for the for standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the tee cmdlet.

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
    BOOL Append = FALSE;
    TEE_CONTEXT TeeContext;
    DWORD FileType;
    YORI_STRING FileName;
    YORI_STRING Arg;

    ZeroMemory(&TeeContext, sizeof(TeeContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                TeeHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("a")) == 0) {
                Append = TRUE;
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

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tee: argument missing\n"));
        return EXIT_FAILURE;
    }

    FileType = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
    FileType = FileType & ~(FILE_TYPE_REMOTE);
    if (FileType == FILE_TYPE_CHAR) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("No file or pipe for input\n"));
        return EXIT_FAILURE;
    }

    if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &FileName)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tee: getfullpathname of %y failed: %s"), &ArgV[StartArg], ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return EXIT_FAILURE;
    }

    TeeContext.hFile = CreateFile(FileName.StartOfString,
                                  (Append?FILE_APPEND_DATA:FILE_WRITE_DATA) | SYNCHRONIZE,
                                  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

    if (TeeContext.hFile == INVALID_HANDLE_VALUE || TeeContext.hFile == NULL) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("tee: open of %y failed: %s"), &FileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FileName);
        return EXIT_FAILURE;
    }

    TeeProcessStream(GetStdHandle(STD_INPUT_HANDLE), &TeeContext);

    CloseHandle(TeeContext.hFile);
    YoriLibFreeStringContents(&FileName);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
