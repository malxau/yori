/**
 * @file initool/initool.c
 *
 * Yori query or set values in INI files
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

/**
 Help text to display to the user.
 */
const
CHAR strIniToolHelpText[] =
        "\n"
        "Query or set values in INI files.\n"
        "\n"
        "INITOOL [-license]\n"
        "INITOOL -d <file> <section> [<key>]\n"
        "INITOOL -l <file> <section>\n"
        "INITOOL -r <file> <section> <key>\n"
        "INITOOL -s <file>\n"
        "INITOOL -w <file> <section> <key> <value>\n"
        "\n"
        "   -d             Delete a specified key from an INI file\n"
        "   -l             List key/value pairs in a specified section from an INI file\n"
        "   -r             Read a specified key from an INI file\n"
        "   -s             List sections in an INI file\n"
        "   -w             Write a specified value to an INI file\n";

/**
 Display usage text to the user.
 */
BOOL
IniToolHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("IniTool %i.%02i\n"), INITOOL_VER_MAJOR, INITOOL_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strIniToolHelpText);
    return TRUE;
}

/**
 Delete a value from an INI file.

 @param UserFileName Pointer to the file name of the INI file.

 @param Section Pointer to the section within the INI file to update.

 @param Key Pointer to the key part of the key value pair.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
IniToolDeleteFromIniFile(
    __in PYORI_STRING UserFileName,
    __in PYORI_STRING Section,
    __in_opt PYORI_STRING Key
    )
{
    YORI_STRING RealFileName;

    if (!YoriLibUserStringToSingleFilePath(UserFileName, FALSE, &RealFileName)) {
        return FALSE;
    }

    if (!WritePrivateProfileString(Section->StartOfString, (Key != NULL)?Key->StartOfString:NULL, NULL, RealFileName.StartOfString)) {
        YoriLibFreeStringContents(&RealFileName);
        return FALSE;
    }

    YoriLibFreeStringContents(&RealFileName);
    return TRUE;
}

/**
 List a section from an INI file and write it to standard output.

 @param UserFileName Pointer to the file name of the INI file.

 @param Section Pointer to the section within the INI file to read.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
IniToolListSectionFromIniFile(
    __in PYORI_STRING UserFileName,
    __in PYORI_STRING Section
    )
{
    YORI_STRING RealFileName;
    YORI_STRING Value;
    LPTSTR ThisVar;

    if (!YoriLibUserStringToSingleFilePath(UserFileName, FALSE, &RealFileName)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&Value, 64 * 1024)) {
        YoriLibFreeStringContents(&RealFileName);
        return FALSE;
    }

    Value.LengthInChars = GetPrivateProfileSection(Section->StartOfString, Value.StartOfString, Value.LengthAllocated, RealFileName.StartOfString);
    ThisVar = Value.StartOfString;
    while (*ThisVar != '\0') {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s\n"), ThisVar);

        ThisVar += _tcslen(ThisVar);
        ThisVar++;
    }

    YoriLibFreeStringContents(&RealFileName);
    YoriLibFreeStringContents(&Value);
    return TRUE;
}

/**
 List sections from an INI file and write them to standard output.

 @param UserFileName Pointer to the file name of the INI file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
IniToolListSectionsFromIniFile(
    __in PYORI_STRING UserFileName
    )
{
    YORI_STRING RealFileName;
    YORI_STRING Value;
    LPTSTR ThisVar;

    if (DllKernel32.pGetPrivateProfileSectionNamesW == NULL) {
        return FALSE;
    }

    if (!YoriLibUserStringToSingleFilePath(UserFileName, FALSE, &RealFileName)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&Value, 64 * 1024)) {
        YoriLibFreeStringContents(&RealFileName);
        return FALSE;
    }

    Value.LengthInChars = DllKernel32.pGetPrivateProfileSectionNamesW(Value.StartOfString, Value.LengthAllocated, RealFileName.StartOfString);
    ThisVar = Value.StartOfString;
    while (*ThisVar != '\0') {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%s\n"), ThisVar);

        ThisVar += _tcslen(ThisVar);
        ThisVar++;
    }

    YoriLibFreeStringContents(&RealFileName);
    YoriLibFreeStringContents(&Value);
    return TRUE;
}

/**
 Read a value from an INI file and write it to standard output.

 @param UserFileName Pointer to the file name of the INI file.

 @param Section Pointer to the section within the INI file to read.

 @param Key Pointer to the key part of the key value pair.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
IniToolReadFromIniFile(
    __in PYORI_STRING UserFileName,
    __in PYORI_STRING Section,
    __in PYORI_STRING Key
    )
{
    YORI_STRING RealFileName;
    YORI_STRING Value;

    if (!YoriLibUserStringToSingleFilePath(UserFileName, FALSE, &RealFileName)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&Value, 16 * 1024)) {
        YoriLibFreeStringContents(&RealFileName);
        return FALSE;
    }

    Value.LengthInChars = GetPrivateProfileString(Section->StartOfString, Key->StartOfString, _T(""), Value.StartOfString, Value.LengthAllocated, RealFileName.StartOfString);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &Value);

    YoriLibFreeStringContents(&RealFileName);
    YoriLibFreeStringContents(&Value);
    return TRUE;
}

/**
 Write a value to an INI file.

 @param UserFileName Pointer to the file name of the INI file.

 @param Section Pointer to the section within the INI file to update.

 @param Key Pointer to the key part of the key value pair.

 @param Value Pointer to the new value to set into the file.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
IniToolWriteToIniFile(
    __in PYORI_STRING UserFileName,
    __in PYORI_STRING Section,
    __in PYORI_STRING Key,
    __in PYORI_STRING Value
    )
{
    YORI_STRING RealFileName;

    if (!YoriLibUserStringToSingleFilePath(UserFileName, FALSE, &RealFileName)) {
        return FALSE;
    }

    if (!WritePrivateProfileString(Section->StartOfString, Key->StartOfString, Value->StartOfString, RealFileName.StartOfString)) {
        YoriLibFreeStringContents(&RealFileName);
        return FALSE;
    }

    YoriLibFreeStringContents(&RealFileName);
    return TRUE;
}

/**
 A list of operations that the tool can perform.
 */
typedef enum _INITOOL_OPERATION {
    IniToolOpNone = 0,
    IniToolOpWriteValue = 1,
    IniToolOpReadValue = 2,
    IniToolOpDeleteValue = 3,
    IniToolOpListSection = 4,
    IniToolOpListSections = 5
} INITOOL_OPERATION;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the initool builtin command.
 */
#define ENTRYPOINT YoriCmd_INITOOL
#else
/**
 The main entrypoint for the initool standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the ini tool cmdlet.

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
    INITOOL_OPERATION Op;

    Op = IniToolOpNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                IniToolHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                Op = IniToolOpDeleteValue;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                Op = IniToolOpListSection;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                Op = IniToolOpReadValue;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Op = IniToolOpListSections;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("w")) == 0) {
                Op = IniToolOpWriteValue;
                ArgumentUnderstood = TRUE;
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

    if (Op == IniToolOpNone) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("initool: missing operation\n"));
        return EXIT_FAILURE;
    }

    if (Op == IniToolOpWriteValue) {
        if (StartArg == 0 || StartArg + 4 > ArgC) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("initool: missing argument\n"));
            return EXIT_FAILURE;
        }

        if (!IniToolWriteToIniFile(&ArgV[StartArg], &ArgV[StartArg + 1], &ArgV[StartArg + 2], &ArgV[StartArg + 3])) {
            return EXIT_FAILURE;
        }
    } else if (Op == IniToolOpReadValue) {
        if (StartArg == 0 || StartArg + 3 > ArgC) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("initool: missing argument\n"));
            return EXIT_FAILURE;
        }

        if (!IniToolReadFromIniFile(&ArgV[StartArg], &ArgV[StartArg + 1], &ArgV[StartArg + 2])) {
            return EXIT_FAILURE;
        }
    } else if (Op == IniToolOpDeleteValue) {
        if (StartArg == 0 || StartArg + 2 > ArgC) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("initool: missing argument\n"));
            return EXIT_FAILURE;
        }

        if (StartArg + 2 < ArgC) {

            if (!IniToolDeleteFromIniFile(&ArgV[StartArg], &ArgV[StartArg + 1], &ArgV[StartArg + 2])) {
                return EXIT_FAILURE;
            }
        } else {
            if (!IniToolDeleteFromIniFile(&ArgV[StartArg], &ArgV[StartArg + 1], NULL)) {
                return EXIT_FAILURE;
            }
        }
    } else if (Op == IniToolOpListSection) {
        if (StartArg == 0 || StartArg + 2 > ArgC) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("initool: missing argument\n"));
            return EXIT_FAILURE;
        }

        if (!IniToolListSectionFromIniFile(&ArgV[StartArg], &ArgV[StartArg + 1])) {
            return EXIT_FAILURE;
        }
    } else if (Op == IniToolOpListSections) {
        if (StartArg == 0 || StartArg + 1 > ArgC) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("initool: missing argument\n"));
            return EXIT_FAILURE;
        }

        if (!IniToolListSectionsFromIniFile(&ArgV[StartArg])) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
