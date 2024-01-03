/**
 * @file get/get.c
 *
 * Yori shell fetch objects from HTTP
 *
 * Copyright (c) 2017-2019 Malcolm J. Smith
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
CHAR strGetHelpText[] =
        "Fetches objects from HTTP and stores them in local files.\n"
        "\n"
        "GET [-license] [-n] <url> <file>\n"
        "\n"
        "   -n             Only download URL if newer than file\n";

/**
 Display usage text to the user.
 */
BOOL
GetHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Get %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strGetHelpText);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the get builtin command.
 */
#define ENTRYPOINT YoriCmd_YGET
#else
/**
 The main entrypoint for the get standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the get cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the child process on success, or failure if the child
         could not be launched.
 */
DWORD
ENTRYPOINT(
    __in YORI_ALLOC_SIZE_T ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOLEAN ArgumentUnderstood;
    YORI_ALLOC_SIZE_T i;
    YORI_ALLOC_SIZE_T StartArg = 1;
    YORI_STRING NewFileName;
    PYORI_STRING ExistingUrlName;
    YORI_LIB_UPDATE_ERROR Error;
    YORI_STRING Agent;
    YORI_STRING Arg;
    BOOLEAN NewerOnly = FALSE;
    SYSTEMTIME ExistingFileTime;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                GetHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("n")) == 0) {
                NewerOnly = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("-")) == 0) {
                ArgumentUnderstood = TRUE;
                StartArg = i + 1;
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

    if (ArgC - StartArg < 2) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("get: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg + 1], TRUE, &NewFileName)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("get: failed to resolve %y\n"), &ArgV[StartArg + 1]);
        return EXIT_FAILURE;
    }

    if (NewerOnly) {
        HANDLE hFile;

        hFile = CreateFile(NewFileName.StartOfString,
                           FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (hFile == INVALID_HANDLE_VALUE) {
            NewerOnly = FALSE;
        } else {
            FILETIME LastAccessTime;
            FILETIME LastWriteTime;
            FILETIME CreateTime;

            GetFileTime(hFile, &CreateTime, &LastAccessTime, &LastWriteTime);
            if (!FileTimeToSystemTime(&LastWriteTime, &ExistingFileTime)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("get: failed obtain time from file\n"));
                YoriLibFreeStringContents(&NewFileName);
                CloseHandle(hFile);
                return EXIT_FAILURE;
            }
            CloseHandle(hFile);
        }
    }

    ExistingUrlName = &ArgV[StartArg];

    YoriLibInitEmptyString(&Agent);
    YoriLibYPrintf(&Agent, _T("YGet %i.%02i\r\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
    if (Agent.StartOfString == NULL) {
        YoriLibFreeStringContents(&NewFileName);
        return EXIT_FAILURE;
    }
    Error = YoriLibUpdateBinaryFromUrl(ExistingUrlName,
                                       &NewFileName,
                                       &Agent,
                                       NewerOnly?&ExistingFileTime:NULL);
    YoriLibFreeStringContents(&NewFileName);
    YoriLibFreeStringContents(&Agent);
    if (Error != YoriLibUpdErrorSuccess) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("get: failed to download: %s\n"), YoriLibUpdateErrorString(Error));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
