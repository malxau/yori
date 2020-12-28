/**
 * @file envdiff/envdiff.c
 *
 * Yori shell display differences between two environments
 *
 * Copyright (c) 2021 Malcolm J. Smith
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
CHAR strEnvDiffHelpText[] =
        "\n"
        "Compares the difference between the current environment and one in a file.\n"
        "\n"
        "ENVDIFF [-license] [<file>]\n";

/**
 Display usage text to the user.
 */
BOOL
EnvDiffHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("EnvDiff %i.%02i\n"), ENVDIFF_VER_MAJOR, ENVDIFF_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strEnvDiffHelpText);
    return TRUE;
}

/**
 Given a specified offset in an environment block, return the key value pair
 as a single substring.  An environment block is a block of NULL terminated
 strings terminated with an additional NULL.

 @param EnvironmentBlock Pointer to the environment block.

 @param Offset Offset within the environment block of the key value pair to
        return.

 @param KeyValue On completion, updated to point to the key value substring
        within the environment block.
 */
VOID
EnvDiffKeyValueAtOffset(
    __in PYORI_STRING EnvironmentBlock,
    __in DWORD Offset,
    __out PYORI_STRING KeyValue
    )
{
    DWORD Index;
    KeyValue->StartOfString = &EnvironmentBlock->StartOfString[Offset];
    KeyValue->LengthInChars = EnvironmentBlock->LengthInChars - Offset;

    for (Index = 0; Index < KeyValue->LengthInChars; Index++) {
        if (KeyValue->StartOfString[Index] == '\0') {
            KeyValue->LengthInChars = Index;
            break;
        }
    }
}

/**
 Given an existing key value substring within an environment block, find the
 offset to the next key value location.  Note the previous key value subtring
 contains a length to know where it ends.

 @param EnvironmentBlock Pointer to the environment block.

 @param KeyValue Pointer to a substring contained within the environment block
        of a previous key value match.

 @param Offset Specifies the offset within the environment block of the data
        in KeyValue.

 @return Offset to the next key value offset.  Once the end of the block is
         reached, this will return the same offset at the end of the block
         repeatedly.
 */
DWORD
EnvDiffGetNextKeyValueOffset(
    __in PYORI_STRING EnvironmentBlock,
    __in PYORI_STRING KeyValue,
    __in DWORD Offset
    )
{
    DWORD NewOffset;

    if (KeyValue->LengthInChars == 0) {
        return Offset;
    }

    NewOffset = Offset + KeyValue->LengthInChars;

    if (NewOffset < EnvironmentBlock->LengthInChars &&
        EnvironmentBlock->StartOfString[NewOffset] == '\0') {

        NewOffset++;
    }

    return NewOffset;
}

/**
 Find the key substring within the key value string.

 @param KeyValue Pointer to the key value string.

 @param Key On completion, updated to point to the key string within the
        key value string.
 */
VOID
EnvDiffGetKeyFromKeyValue(
    __in PYORI_STRING KeyValue,
    __out PYORI_STRING Key
    )
{
    DWORD Index;
    YoriLibInitEmptyString(Key);
    Key->StartOfString = KeyValue->StartOfString;

    //
    //  Intentionally start counting from one.  The first character can be
    //  an equals sign, and this is used for per drive current directories
    //  and exit codes.
    //

    for (Index = 1; Index < KeyValue->LengthInChars; Index++) {
        if (Key->StartOfString[Index] == '=') {
            Key->LengthInChars = Index;
            break;
        }
    }
}

/**
 Find the value substring within the key value string.

 @param KeyValue Pointer to the key value string.

 @param Key Pointer to the key substring within the key value string.

 @param Value On completion, updated to point to the value string within the
        key value string.
 */
VOID
EnvDiffGetValueFromKeyValue(
    __in PYORI_STRING KeyValue,
    __in PYORI_STRING Key,
    __out PYORI_STRING Value
    )
{
    YoriLibInitEmptyString(Value);

    if (Key->LengthInChars < KeyValue->LengthInChars &&
        KeyValue->StartOfString[Key->LengthInChars] == '=') {

        Value->StartOfString = &KeyValue->StartOfString[Key->LengthInChars + 1];
        Value->LengthInChars = KeyValue->LengthInChars - Key->LengthInChars - 1;
    }
}

/**
 Specifies the modification type that occurred to an environment variable.
 */
typedef enum _ENVDIFF_CHANGE_TYPE {
    EnvDiffChangeAdd = 0,
    EnvDiffChangeRemove = 1,
    EnvDiffChangeModify = 2
} ENVDIFF_CHANGE_TYPE;

/**
 Specifies the format to output environment changes in.
 */
typedef enum _ENVDIFF_OUTPUT_FORMAT {
    EnvDiffOutputCmdBatch = 0
} ENVDIFF_OUTPUT_FORMAT;

/**
 Output a change to the environment in the specified format.

 @param Key The name of the key of the environment variable.

 @param BaseValue The old value of the variable.  This can be NULL if it did
        not previously exist.

 @param NewValue The new value of the variable.  This can be NULL if it is
        being deleted.

 @param Format Specifies the format to output the changes in.

 @param ChangeType Specifies the type of modification that occurred.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
EnvDiffOutputDifference(
    __in PYORI_STRING Key,
    __in_opt PYORI_STRING BaseValue,
    __in_opt PYORI_STRING NewValue,
    __in ENVDIFF_OUTPUT_FORMAT Format,
    __in ENVDIFF_CHANGE_TYPE ChangeType
    )
{
    UNREFERENCED_PARAMETER(BaseValue);
    UNREFERENCED_PARAMETER(Format);

    if (ChangeType == EnvDiffChangeAdd) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("set %y=%y\n"), Key, NewValue);
    } else if (ChangeType == EnvDiffChangeRemove) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("set %y=\n"), Key);
    } else if (ChangeType == EnvDiffChangeModify) {
        DWORD MatchOffset;

        //
        //  Check if the original value is contained within the new value, and
        //  if so, output the update referring to the original value.
        //

        if (YoriLibFindFirstMatchingSubstring(NewValue, 1, BaseValue, &MatchOffset)) {
            YORI_STRING Prefix;
            YORI_STRING Suffix;
            YoriLibInitEmptyString(&Prefix);
            YoriLibInitEmptyString(&Suffix);
            Prefix.StartOfString = NewValue->StartOfString;
            Prefix.LengthInChars = MatchOffset;
            Suffix.StartOfString = NewValue->StartOfString + Prefix.LengthInChars + BaseValue->LengthInChars;
            Suffix.LengthInChars = NewValue->LengthInChars - Prefix.LengthInChars - BaseValue->LengthInChars;
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("set %y=%y%%%y%%%y\n"), Key, &Prefix, Key, &Suffix);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("set %y=%y\n"), Key, NewValue);
        }
    }

    return TRUE;
}

/**
 Compare two environment blocks, and output the differences in the specified
 format.

 @param BaseEnvironment Pointer to the original environment block.

 @param NewEnvironment Pointer to the new environment block.

 @param OutputFormat Specifies the format to output the changes.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
EnvDiffCompareEnvironments(
    __in PYORI_STRING BaseEnvironment,
    __in PYORI_STRING NewEnvironment,
    __in ENVDIFF_OUTPUT_FORMAT OutputFormat
    )
{
    YORI_STRING BaseKeyValue;
    YORI_STRING NewKeyValue;
    YORI_STRING BaseKey;
    YORI_STRING NewKey;
    YORI_STRING BaseValue;
    YORI_STRING NewValue;
    DWORD BaseStart;
    DWORD NewStart;
    int Compare;

    YoriLibInitEmptyString(&BaseKeyValue);
    YoriLibInitEmptyString(&NewKeyValue);

    BaseStart = 0;
    NewStart = 0;

    while (TRUE) {

        EnvDiffKeyValueAtOffset(BaseEnvironment, BaseStart, &BaseKeyValue);
        EnvDiffKeyValueAtOffset(NewEnvironment, NewStart, &NewKeyValue);

        if (BaseKeyValue.LengthInChars == 0 &&
            NewKeyValue.LengthInChars == 0) {

            break;
        }

        EnvDiffGetKeyFromKeyValue(&BaseKeyValue, &BaseKey);
        EnvDiffGetKeyFromKeyValue(&NewKeyValue, &NewKey);
        EnvDiffGetValueFromKeyValue(&BaseKeyValue, &BaseKey, &BaseValue);
        EnvDiffGetValueFromKeyValue(&NewKeyValue, &NewKey, &NewValue);

        //
        //  Skip any variables whose name starts with "=".  These are used for
        //  per drive current directories and exit code, and are not 
        //  really user state.
        //

        if (BaseKey.LengthInChars > 0 &&
            BaseKey.StartOfString[0] == '=') {

            BaseStart = EnvDiffGetNextKeyValueOffset(BaseEnvironment, &BaseKeyValue, BaseStart);
            continue;
        }

        if (NewKey.LengthInChars > 0 &&
            NewKey.StartOfString[0] == '=') {

            NewStart = EnvDiffGetNextKeyValueOffset(NewEnvironment, &NewKeyValue, NewStart);
            continue;
        }

        //
        //  If there is no base value, there is a new variable added that is
        //  not in base.  If there is no new value, there is a value in base
        //  that has been removed.
        //

        if (BaseKeyValue.LengthInChars == 0) {
            EnvDiffOutputDifference(&NewKey, NULL, &NewValue, OutputFormat, EnvDiffChangeAdd);
            NewStart = EnvDiffGetNextKeyValueOffset(NewEnvironment, &NewKeyValue, NewStart);
        } else if (NewKeyValue.LengthInChars == 0) {
            EnvDiffOutputDifference(&BaseKey, &BaseValue, NULL, OutputFormat, EnvDiffChangeRemove);
            BaseStart = EnvDiffGetNextKeyValueOffset(BaseEnvironment, &BaseKeyValue, BaseStart);
        } else {

            //
            //  If both have variables, check to see if one is ahead of the
            //  other.  Because environment blocks are sorted, if there's
            //  a difference we know which one has a variable that the other
            //  does not by lexicographic order.
            //

            Compare = YoriLibCompareStringInsensitive(&BaseKey, &NewKey);
            if (Compare < 0) {
                EnvDiffOutputDifference(&BaseKey, &BaseValue, NULL, OutputFormat, EnvDiffChangeRemove);
                BaseStart = EnvDiffGetNextKeyValueOffset(BaseEnvironment, &BaseKeyValue, BaseStart);
            } else if (Compare > 0) {
                EnvDiffOutputDifference(&NewKey, NULL, &NewValue, OutputFormat, EnvDiffChangeAdd);
                NewStart = EnvDiffGetNextKeyValueOffset(NewEnvironment, &NewKeyValue, NewStart);
            } else {

                //
                //  If the value is the same, nothing has happened.
                //  Otherwise, indicate the modification.
                //

                if (YoriLibCompareString(&BaseValue, &NewValue) != 0) {
                    EnvDiffOutputDifference(&NewKey, &BaseValue, &NewValue, OutputFormat, EnvDiffChangeModify);
                }
                BaseStart = EnvDiffGetNextKeyValueOffset(BaseEnvironment, &BaseKeyValue, BaseStart);
                NewStart = EnvDiffGetNextKeyValueOffset(NewEnvironment, &NewKeyValue, NewStart);
            }
        }
    }

    return FALSE;
}

/**
 Load the environment block specified in an opened stream.

 @param FileHandle A handle to the opened stream.

 @param EnvironmentBlock On successful completion, updated to point to the
        contents of the stream in the format of an environment block.  The
        caller is expected to free this with @ref YoriLibFreeStringContents .

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
EnvDiffLoadStreamIntoEnvironmentBlock(
    __in HANDLE FileHandle,
    __out PYORI_STRING EnvironmentBlock
    )
{
    DWORD BufferSize;
    PVOID LineContext = NULL;
    YORI_STRING LineString;

    YoriLibInitEmptyString(EnvironmentBlock);
    YoriLibInitEmptyString(&LineString);

    BufferSize = 1024;
    if (!YoriLibAllocateString(EnvironmentBlock, BufferSize)) {
        return FALSE;
    }

    while (TRUE) {
        if (!YoriLibReadLineToString(&LineString, &LineContext, FileHandle)) {
            break;
        }

        if (EnvironmentBlock->LengthInChars + LineString.LengthInChars + 2 > EnvironmentBlock->LengthAllocated) {
            BufferSize = BufferSize * 4;
            if (!YoriLibReallocateString(EnvironmentBlock, BufferSize)) {
                YoriLibFreeStringContents(EnvironmentBlock);
                YoriLibFreeStringContents(&LineString);
                YoriLibLineReadClose(LineContext);
                return FALSE;
            }
        }

        memcpy(&EnvironmentBlock->StartOfString[EnvironmentBlock->LengthInChars],
               LineString.StartOfString,
               LineString.LengthInChars * sizeof(TCHAR));

        EnvironmentBlock->LengthInChars = EnvironmentBlock->LengthInChars + LineString.LengthInChars;
        EnvironmentBlock->StartOfString[EnvironmentBlock->LengthInChars] = '\0';
        EnvironmentBlock->LengthInChars++;
    }

    YoriLibFreeStringContents(&LineString);
    YoriLibLineReadClose(LineContext);

    EnvironmentBlock->StartOfString[EnvironmentBlock->LengthInChars] = '\0';
    EnvironmentBlock->LengthInChars++;

    return TRUE;
}

/**
 Load the environment block specified in a file.

 @param FileName Pointer to the name of the file.

 @param EnvironmentBlock On successful completion, updated to point to the
        contents of the stream in the format of an environment block.  The
        caller is expected to free this with @ref YoriLibFreeStringContents .

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
EnvDiffLoadFileIntoEnvironmentBlock(
    __in PYORI_STRING FileName,
    __out PYORI_STRING EnvironmentBlock
    )
{
    YORI_STRING FullPath;
    HANDLE FileHandle;

    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        return FALSE;
    }

    FileHandle = CreateFile(FullPath.StartOfString,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL);

    YoriLibFreeStringContents(&FullPath);
    if (FileHandle == INVALID_HANDLE_VALUE) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("envdiff: open file failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    if (!EnvDiffLoadStreamIntoEnvironmentBlock(FileHandle, EnvironmentBlock)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("envdiff: out of memory\n"));
        CloseHandle(FileHandle);
        return FALSE;
    }

    CloseHandle(FileHandle);

    return TRUE;
}


#ifdef YORI_BUILTIN
/**
 The main entrypoint for the envdiff builtin command.
 */
#define ENTRYPOINT YoriCmd_ENVDIFF
#else
/**
 The main entrypoint for the envdiff standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the envdiff cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process indicating success or failure.
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
    YORI_STRING CurrentEnvironment;
    YORI_STRING BaseEnvironment;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                EnvDiffHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2021"));
                return EXIT_SUCCESS;
            }
        } else {
            StartArg = i;
            ArgumentUnderstood = TRUE;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0) {
        if (YoriLibIsStdInConsole()) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("envdiff: No file or pipe for input\n"));
            return EXIT_FAILURE;
        } else {
            if (!EnvDiffLoadStreamIntoEnvironmentBlock(GetStdHandle(STD_INPUT_HANDLE), &BaseEnvironment)) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("envdiff: could not load environment\n"));
                return EXIT_FAILURE;
            }
        }
    } else {
        if (!EnvDiffLoadFileIntoEnvironmentBlock(&ArgV[StartArg], &BaseEnvironment)) {
            YoriLibFreeStringContents(&CurrentEnvironment);
            return EXIT_FAILURE;
        }
    }

    if (!YoriLibGetEnvironmentStrings(&CurrentEnvironment)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("envdiff: could not load environment\n"));
        YoriLibFreeStringContents(&BaseEnvironment);
        return EXIT_FAILURE;
    }

    EnvDiffCompareEnvironments(&BaseEnvironment, &CurrentEnvironment, EnvDiffOutputCmdBatch);

    YoriLibFreeStringContents(&BaseEnvironment);
    YoriLibFreeStringContents(&CurrentEnvironment);

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
