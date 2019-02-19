/**
 * @file path/path.c
 *
 * Yori shell display file name components
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
CHAR strPathHelpText[] =
        "\n"
        "Converts relative paths into decomposable full paths.\n"
        "\n"
        "PATH [-license] [-e] [-f <fmtstring>] <path>\n"
        "\n"
        "   -e             Use an escaped long path\n"
        "\n"
        "Format specifiers are:\n"
        "   $BASE$         The file name without any path or extension\n"
        "   $DIR$          The directory hosting the file\n"
        "   $DRIVE$        The drive letter hosting the file\n"
        "   $EXT$          The file extension\n"
        "   $FILE$         The file name including extension\n"
        "   $PARENT$       The path to the parent of the file\n"
        "   $PATH$         The complete path to the file\n"
        "   $SHARE$        The UNC share hosting the file\n";

/**
 Display usage text to the user.
 */
BOOL
PathHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Path %i.%02i\n"), PATH_VER_MAJOR, PATH_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strPathHelpText);
    return TRUE;
}

/**
 A decomposed form of a path.
 */
typedef struct _YORI_PATH_COMPONENTS {

    /**
     The entire path, including everything.
     */
    YORI_STRING EntirePath;

    /**
     A file extension, if present.  May contain NULL to indicate no extension
     was found, or could have a length of zero indicating a trailing period.
     */
    YORI_STRING Extension;

    /**
     The file name, without any extension.
     */
    YORI_STRING BaseName;

    /**
     The file name, including extension.
     */
    YORI_STRING FullFileName;

    /**
     The path from the root of the volume, excluding volume name.
     */
    YORI_STRING PathFromRoot;

    /**
     The drive letter of the volume.  Mutually exclusive with ShareName.
     */
    YORI_STRING DriveLetter;

    /**
     The share root of the volume.  Mutually exclusive with DriveLetter.
     */
    YORI_STRING ShareName;

    /**
     The path to the parent of the object.
     */
    YORI_STRING ParentName;
} YORI_PATH_COMPONENTS, *PYORI_PATH_COMPONENTS;

/**
 A callback function to expand any known variables found when parsing the
 path.

 @param OutputString A pointer to the output string to populate with data
        if a known variable is found.  The allocated length indicates the
        amount of the buffer that can be populated with data.

 @param VariableName The variable name to expand.

 @param Context Pointer to a YORI_PATH_COMPONENTS structure containing the
        data to populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
DWORD
PathExpandVariables(
   __inout PYORI_STRING OutputString,
   __in PYORI_STRING VariableName,
   __in PVOID Context
   )
{
    DWORD CharsNeeded = 0;
    PYORI_PATH_COMPONENTS PathComponents = (PYORI_PATH_COMPONENTS)Context;

    if (YoriLibCompareStringWithLiteral(VariableName, _T("PATH")) == 0) {
        CharsNeeded = PathComponents->EntirePath.LengthInChars;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("EXT")) == 0) {
        CharsNeeded = PathComponents->Extension.LengthInChars;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("FILE")) == 0) {
        CharsNeeded = PathComponents->FullFileName.LengthInChars;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("BASE")) == 0) {
        CharsNeeded = PathComponents->BaseName.LengthInChars;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("DRIVE")) == 0) {
        CharsNeeded = PathComponents->DriveLetter.LengthInChars;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("DIR")) == 0) {
        CharsNeeded = PathComponents->PathFromRoot.LengthInChars;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SHARE")) == 0) {
        CharsNeeded = PathComponents->ShareName.LengthInChars;
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("PARENT")) == 0) {
        CharsNeeded = PathComponents->ParentName.LengthInChars;
    } else {
        return 0;
    }

    if (OutputString->LengthAllocated < CharsNeeded || CharsNeeded == 0) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringWithLiteral(VariableName, _T("PATH")) == 0) {
        memcpy(OutputString->StartOfString, PathComponents->EntirePath.StartOfString, CharsNeeded * sizeof(TCHAR));
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("EXT")) == 0) {
        memcpy(OutputString->StartOfString, PathComponents->Extension.StartOfString, CharsNeeded * sizeof(TCHAR));
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("FILE")) == 0) {
        memcpy(OutputString->StartOfString, PathComponents->FullFileName.StartOfString, CharsNeeded * sizeof(TCHAR));
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("BASE")) == 0) {
        memcpy(OutputString->StartOfString, PathComponents->BaseName.StartOfString, CharsNeeded * sizeof(TCHAR));
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("DRIVE")) == 0) {
        memcpy(OutputString->StartOfString, PathComponents->DriveLetter.StartOfString, CharsNeeded * sizeof(TCHAR));
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("DIR")) == 0) {
        memcpy(OutputString->StartOfString, PathComponents->PathFromRoot.StartOfString, CharsNeeded * sizeof(TCHAR));
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("SHARE")) == 0) {
        memcpy(OutputString->StartOfString, PathComponents->ShareName.StartOfString, CharsNeeded * sizeof(TCHAR));
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("PARENT")) == 0) {
        memcpy(OutputString->StartOfString, PathComponents->ParentName.StartOfString, CharsNeeded * sizeof(TCHAR));
    }

    OutputString->LengthInChars = CharsNeeded;
    return CharsNeeded;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the path builtin command.
 */
#define ENTRYPOINT YoriCmd_YPATH
#else
/**
 The main entrypoint for the path standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the path cmdlet.

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
    YORI_PATH_COMPONENTS PathComponents;
    LPTSTR FormatString = _T("$PATH$");
    YORI_STRING YsFormatString;
    YORI_STRING DisplayString;
    BOOL UseLongPath = FALSE;
    DWORD StartArg = 0;
    DWORD i;
    YORI_STRING Arg;

    YoriLibInitEmptyString(&YsFormatString);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                PathHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("e")) == 0) {
                UseLongPath = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (ArgC > i + 1) {
                    YsFormatString.StartOfString = ArgV[i + 1].StartOfString;
                    YsFormatString.LengthInChars = ArgV[i + 1].LengthInChars;
                    YsFormatString.LengthAllocated = ArgV[i + 1].LengthAllocated;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
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
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (YsFormatString.StartOfString == NULL) {
        YoriLibConstantString(&YsFormatString, FormatString);
    }

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("path: missing argument\n"));
        return EXIT_FAILURE;
    }

    ZeroMemory(&PathComponents, sizeof(PathComponents));

    if (YoriLibUserStringToSingleFilePath(&ArgV[StartArg], UseLongPath, &PathComponents.EntirePath)) {
        DWORD CharIndex;
        BOOL ExtensionFound = FALSE;
        BOOL FileComponentFound = FALSE;
        DWORD KeepTrailingSlashesBefore;

        KeepTrailingSlashesBefore = 0;
        if (UseLongPath) {
            if (YoriLibIsPrefixedDriveLetterWithColonAndSlash(&PathComponents.EntirePath)) {
                KeepTrailingSlashesBefore = sizeof("\\\\?\\C:\\") - 1;
            }
        } else {
            if (YoriLibIsDriveLetterWithColonAndSlash(&PathComponents.EntirePath)) {
                KeepTrailingSlashesBefore = sizeof("C:\\") - 1;
            }
        }

        //
        //  Remove any trailing slashes
        //

        for (CharIndex = PathComponents.EntirePath.LengthInChars - 1; CharIndex > KeepTrailingSlashesBefore; CharIndex--) {
            if (PathComponents.EntirePath.StartOfString[CharIndex] == '\\') {
                PathComponents.EntirePath.LengthInChars--;
            } else {
                break;
            }

            if (CharIndex == 0) {
                break;
            }
        }

        //
        //  Count backwards to find the file name and extension
        //

        for (CharIndex = PathComponents.EntirePath.LengthInChars - 1; CharIndex >= 0; CharIndex--) {

            if (PathComponents.EntirePath.StartOfString[CharIndex] == '.' && !FileComponentFound && !ExtensionFound) {
                ExtensionFound = TRUE;
                PathComponents.Extension.StartOfString = &PathComponents.EntirePath.StartOfString[CharIndex + 1];
                PathComponents.Extension.LengthInChars = PathComponents.EntirePath.LengthInChars - CharIndex - 1;
            }

            if (PathComponents.EntirePath.StartOfString[CharIndex] == '\\' && !FileComponentFound) {
                FileComponentFound = TRUE;
                PathComponents.FullFileName.StartOfString = &PathComponents.EntirePath.StartOfString[CharIndex + 1];
                PathComponents.FullFileName.LengthInChars = PathComponents.EntirePath.LengthInChars - CharIndex - 1;

                PathComponents.BaseName.StartOfString = PathComponents.FullFileName.StartOfString;
                PathComponents.BaseName.LengthInChars = PathComponents.FullFileName.LengthInChars;
                if (PathComponents.Extension.StartOfString != NULL) {
                    PathComponents.BaseName.LengthInChars -= PathComponents.Extension.LengthInChars + 1;
                }

                PathComponents.ParentName.StartOfString = PathComponents.EntirePath.StartOfString;
                PathComponents.ParentName.LengthInChars = CharIndex;

                break;
            }

            if (CharIndex == 0) {
                break;
            }
        }

        //
        //  Count forwards to find the drive letter or share
        //

        if (UseLongPath) {

            YORI_STRING PathAfterPrefix;

            //
            //  We kind of expect a long prefix if nothing else
            //

            if (PathComponents.EntirePath.LengthInChars < 4) {
                return EXIT_FAILURE;
            }

            YoriLibInitEmptyString(&PathAfterPrefix);
            PathAfterPrefix.StartOfString = PathComponents.EntirePath.StartOfString + 4;
            PathAfterPrefix.LengthInChars = PathComponents.EntirePath.LengthInChars - 4;

            if (YoriLibIsFullPathUnc(&PathComponents.EntirePath)) {
                BOOL EndOfServerNameFound = FALSE;
    
                //
                //  We have a \\?\UNC\ UNC prefix in an escaped path
                //
    
                for (CharIndex = 8; PathComponents.EntirePath.StartOfString[CharIndex] != '\0'; CharIndex++) {
                    if (PathComponents.EntirePath.StartOfString[CharIndex] == '\\') {
                        if (!EndOfServerNameFound) {
                            EndOfServerNameFound = TRUE;
                        } else {
                            break;
                        }
                    }
                }
    
                if (PathComponents.EntirePath.StartOfString[CharIndex] != 0 ||
                    EndOfServerNameFound) {
    
                    PathComponents.ShareName.StartOfString = PathComponents.EntirePath.StartOfString;
                    PathComponents.ShareName.LengthInChars = CharIndex;
    
                    //
                    //  If we have enough chars for a share name plus file name,
                    //  check for an intermediate directory.  If we don't, that
                    //  implies the file name is the last part of the share name,
                    //  so remove any reference to file name.
                    //
    
                    if (PathComponents.ShareName.LengthInChars + PathComponents.FullFileName.LengthInChars < PathComponents.EntirePath.LengthInChars) {
    
                        PathComponents.PathFromRoot.StartOfString = &PathComponents.EntirePath.StartOfString[CharIndex];
                        PathComponents.PathFromRoot.LengthInChars = PathComponents.EntirePath.LengthInChars - PathComponents.ShareName.LengthInChars - PathComponents.FullFileName.LengthInChars - 1;
                    } else if (PathComponents.ShareName.LengthInChars + PathComponents.FullFileName.LengthInChars > PathComponents.EntirePath.LengthInChars) {
                        PathComponents.BaseName.LengthInChars = 0;
                        PathComponents.FullFileName.LengthInChars = 0;
                        PathComponents.Extension.LengthInChars = 0;
                    }
                }
            } else if (YoriLibIsDriveLetterWithColonAndSlash(&PathAfterPrefix)) {

                //
                //  We have a drive letter, colon and slash in an escaped path
                //
    
                PathComponents.DriveLetter.StartOfString = &PathComponents.EntirePath.StartOfString[4];
                PathComponents.DriveLetter.LengthInChars = 1;
    
                PathComponents.PathFromRoot.StartOfString = &PathComponents.EntirePath.StartOfString[6];
                PathComponents.PathFromRoot.LengthInChars = PathComponents.EntirePath.LengthInChars - 6;
    
                if (PathComponents.FullFileName.StartOfString != NULL) {
                    PathComponents.PathFromRoot.LengthInChars -= PathComponents.FullFileName.LengthInChars + 1;
                }
            }
        } else {
            if (YoriLibIsDriveLetterWithColonAndSlash(&PathComponents.EntirePath)) {
    
                //
                //  We have a drive letter, colon and slash in a non escaped path
                //
    
                PathComponents.DriveLetter.StartOfString = &PathComponents.EntirePath.StartOfString[0];
                PathComponents.DriveLetter.LengthInChars = 1;
    
                PathComponents.PathFromRoot.StartOfString = &PathComponents.EntirePath.StartOfString[2];
                PathComponents.PathFromRoot.LengthInChars = PathComponents.EntirePath.LengthInChars - 2;
    
                if (PathComponents.FullFileName.StartOfString != NULL) {
                    PathComponents.PathFromRoot.LengthInChars -= PathComponents.FullFileName.LengthInChars + 1;
                }
            } else if ((PathComponents.EntirePath.StartOfString[0] == '\\') ||
                       (PathComponents.EntirePath.StartOfString[1] == '\\')) {
    
                BOOL EndOfServerNameFound = FALSE;
    
                //
                //  We have a \\ UNC prefix in a non escaped path
                //
    
                for (CharIndex = 2; PathComponents.EntirePath.StartOfString[CharIndex] != '\0'; CharIndex++) {
                    if (PathComponents.EntirePath.StartOfString[CharIndex] == '\\') {
                        if (!EndOfServerNameFound) {
                            EndOfServerNameFound = TRUE;
                        } else {
                            break;
                        }
                    }
                }
    
                if (PathComponents.EntirePath.StartOfString[CharIndex] != 0 ||
                    EndOfServerNameFound) {
    
                    PathComponents.ShareName.StartOfString = PathComponents.EntirePath.StartOfString;
                    PathComponents.ShareName.LengthInChars = CharIndex;
    
                    //
                    //  If we have enough chars for a share name plus file name,
                    //  check for an intermediate directory.  If we don't, that
                    //  implies the file name is the last part of the share name,
                    //  so remove any reference to file name.
                    //
    
                    if (PathComponents.ShareName.LengthInChars + PathComponents.FullFileName.LengthInChars < PathComponents.EntirePath.LengthInChars) {
    
                        PathComponents.PathFromRoot.StartOfString = &PathComponents.EntirePath.StartOfString[CharIndex];
                        PathComponents.PathFromRoot.LengthInChars = PathComponents.EntirePath.LengthInChars - PathComponents.ShareName.LengthInChars - PathComponents.FullFileName.LengthInChars - 1;
                    } else if (PathComponents.ShareName.LengthInChars + PathComponents.FullFileName.LengthInChars > PathComponents.EntirePath.LengthInChars) {
                        PathComponents.BaseName.LengthInChars = 0;
                        PathComponents.FullFileName.LengthInChars = 0;
                        PathComponents.Extension.LengthInChars = 0;
                    }
                }
            }
        }

        YoriLibInitEmptyString(&DisplayString);
        YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, PathExpandVariables, &PathComponents, &DisplayString);
        YoriLibFreeStringContents(&PathComponents.EntirePath);
        if (DisplayString.StartOfString != NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &DisplayString);
            YoriLibFreeStringContents(&DisplayString);
        }
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
