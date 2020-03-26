/**
 * @file assoc/assoc.c
 *
 * Yori shell display or edit file associations
 *
 * Copyright (c) 2020 Malcolm J. Smith
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
CHAR strAssocHelpText[] =
        "\n"
        "Display or edit file associations.\n"
        "\n"
        "ASSOC [-license] [-m|-s|-u] [.ext[=[filetype]]]\n"
        "ASSOC -t [-m|-s|-u] [filetype[=[openCommandString]]]\n"
        "\n"
        "   -m             Display the contents from the merged system and user registry\n"
        "   -s             Display or update the contents from the system registry\n"
        "   -t             Display or update file types instead of extension associations\n"
        "   -u             Display or update the contents from the user registry\n"
        ;

/**
 Display usage text to the user.
 */
BOOL
AssocHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Assoc %i.%02i\n"), ASSOC_VER_MAJOR, ASSOC_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strAssocHelpText);
    return TRUE;
}

/**
 A set of potential registry locations to inspect or update.  The meaning
 of default is context dependent.
 */
typedef enum _ASSOC_SCOPE {
    AssocScopeDefault = 0,
    AssocScopeSystem = 1,
    AssocScopeMerged = 2,
    AssocScopeUser = 3
} ASSOC_SCOPE;

/**
 Modify or delete a single extension association's file type.

 @param RootKey The parent registry key of the extension key.  This function
        will open the extension key as needed.

 @param Extension The file extension, including initial period.

 @param NewType The new file type to associate with the extension.  If this
        is an empty string, the association is deleted.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocUpdateSingleAssociation(
    __in HKEY RootKey,
    __in PYORI_STRING Extension,
    __in PYORI_STRING NewType
    )
{
    DWORD Error;
    HANDLE ThisKey;
    DWORD Disposition;

    Error = DllAdvApi32.pRegCreateKeyExW(RootKey, Extension->StartOfString, 0, NULL, 0, KEY_SET_VALUE, NULL, &ThisKey, &Disposition);

    if (Error != ERROR_SUCCESS) {
        return FALSE;
    }

    if (NewType->LengthInChars == 0) {
        Error = DllAdvApi32.pRegDeleteValueW(ThisKey, _T(""));
    } else {
        Error = DllAdvApi32.pRegSetValueExW(ThisKey, _T(""), 0, REG_SZ, (LPBYTE)NewType->StartOfString, NewType->LengthInChars * sizeof(TCHAR));
    }

    DllAdvApi32.pRegCloseKey(ThisKey);

    if (Error != ERROR_SUCCESS) {
        return FALSE;
    }

    //
    //  Attempt to delete the key.  This isn't essential so failure to
    //  delete the key is not fatal.
    //

    if (NewType->LengthInChars == 0) {
        Error = DllAdvApi32.pRegDeleteKeyW(RootKey, Extension->StartOfString);
    }

    return TRUE;
}

/**
 Modify or delete a single file type's associated program.

 @param RootKey The parent registry key of the file type key.  This function
        will open the file type key as needed.

 @param FileType The file type.

 @param NewProgram The new program to associate with the file type.  If this
        is an empty string, the file type is deleted.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocUpdateSingleFileType(
    __in HKEY RootKey,
    __in PYORI_STRING FileType,
    __in PYORI_STRING NewProgram
    )
{
    DWORD Error;
    HANDLE ThisKey;
    DWORD Disposition;
    YORI_STRING SubKeyName;

    YoriLibInitEmptyString(&SubKeyName);
    YoriLibYPrintf(&SubKeyName, _T("%y\\shell\\open\\command"), FileType);
    if (SubKeyName.StartOfString == NULL) {
        return FALSE;
    }

    Error = DllAdvApi32.pRegCreateKeyExW(RootKey, SubKeyName.StartOfString, 0, NULL, 0, KEY_SET_VALUE, NULL, &ThisKey, &Disposition);

    YoriLibFreeStringContents(&SubKeyName);

    if (Error != ERROR_SUCCESS) {
        return FALSE;
    }

    if (NewProgram->LengthInChars == 0) {
        Error = DllAdvApi32.pRegDeleteValueW(ThisKey, _T(""));
    } else {
        Error = DllAdvApi32.pRegSetValueExW(ThisKey, _T(""), 0, REG_SZ, (LPBYTE)NewProgram->StartOfString, NewProgram->LengthInChars * sizeof(TCHAR));
    }

    DllAdvApi32.pRegCloseKey(ThisKey);

    if (Error != ERROR_SUCCESS) {
        return FALSE;
    }

    //
    //  Attempt to delete the key.  This isn't essential so failure to
    //  delete the key is not fatal.
    //

    if (NewProgram->LengthInChars == 0) {
        Error = DllAdvApi32.pRegDeleteKeyW(RootKey, NewProgram->StartOfString);
    }

    return TRUE;
}

/**
 Display the current file type associated with a specified extension.

 @param RootKey The parent registry key of the extension key.  This function
        will open the extension key as needed.

 @param Extension The file extension, including initial period.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocDisplaySingleAssociation(
    __in HKEY RootKey,
    __in PYORI_STRING Extension
    )
{
    DWORD Error;
    YORI_STRING KeyValue;
    DWORD KeyValueSize;
    HANDLE ThisKey;
    DWORD KeyType;

    if (!YoriLibAllocateString(&KeyValue, 1024)) {
        return FALSE;
    }

    Error = DllAdvApi32.pRegOpenKeyExW(RootKey, Extension->StartOfString, 0, KEY_QUERY_VALUE, &ThisKey);

    if (Error != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&KeyValue);
        return FALSE;
    }

    while(TRUE) {
        KeyValueSize = KeyValue.LengthAllocated * sizeof(TCHAR);
        Error = DllAdvApi32.pRegQueryValueExW(ThisKey, _T(""), NULL, &KeyType, (LPBYTE)KeyValue.StartOfString, &KeyValueSize);

        if (Error != ERROR_MORE_DATA) {
            break;
        }

        // 
        //  Note that RegQueryValueEx works in bytes, and this
        //  is reallocating chars, having the effect of allocating
        //  twice what is really needed
        //

        YoriLibFreeStringContents(&KeyValue);
        if (!YoriLibAllocateString(&KeyValue, KeyValueSize)) {
            break;
        }
    }

    DllAdvApi32.pRegCloseKey(ThisKey);

    if (Error != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&KeyValue);
        return FALSE;
    }

    KeyValue.LengthInChars = KeyValueSize / sizeof(TCHAR);
    if (KeyValue.LengthInChars > 0) {
        if (KeyValue.StartOfString[KeyValue.LengthInChars - 1] == '\0') {
            KeyValue.LengthInChars--;
        }
        if (KeyValue.LengthInChars > 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y=%y\n"), Extension, &KeyValue);
        }
    }

    YoriLibFreeStringContents(&KeyValue);
    return TRUE;
}

/**
 Display the current application associated with a file type.

 @param RootKey The parent registry key of the file type key.  This function
        will open the file type key as needed.

 @param FileType The file type.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocDisplaySingleFileType(
    __in HKEY RootKey,
    __in PYORI_STRING FileType
    )
{
    DWORD Error;
    YORI_STRING SubKeyName;
    YORI_STRING KeyValue;
    DWORD KeyValueSize;
    HANDLE ThisKey;
    DWORD KeyType;

    YoriLibInitEmptyString(&SubKeyName);
    YoriLibYPrintf(&SubKeyName, _T("%y\\shell\\open\\command"), FileType);
    if (SubKeyName.StartOfString == NULL) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&KeyValue, 1024)) {
        YoriLibFreeStringContents(&SubKeyName);
        return FALSE;
    }

    Error = DllAdvApi32.pRegOpenKeyExW(RootKey, SubKeyName.StartOfString, 0, KEY_QUERY_VALUE, &ThisKey);

    YoriLibFreeStringContents(&SubKeyName);

    if (Error != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&KeyValue);
        return FALSE;
    }

    while(TRUE) {
        KeyValueSize = KeyValue.LengthAllocated * sizeof(TCHAR);
        Error = DllAdvApi32.pRegQueryValueExW(ThisKey, _T(""), NULL, &KeyType, (LPBYTE)KeyValue.StartOfString, &KeyValueSize);

        if (Error != ERROR_MORE_DATA) {
            break;
        }

        // 
        //  Note that RegQueryValueEx works in bytes, and this
        //  is reallocating chars, having the effect of allocating
        //  twice what is really needed
        //

        YoriLibFreeStringContents(&KeyValue);
        if (!YoriLibAllocateString(&KeyValue, KeyValueSize)) {
            break;
        }
    }

    DllAdvApi32.pRegCloseKey(ThisKey);

    if (Error != ERROR_SUCCESS) {
        YoriLibFreeStringContents(&KeyValue);
        return FALSE;
    }

    KeyValue.LengthInChars = KeyValueSize / sizeof(TCHAR);
    if (KeyValue.LengthInChars > 0) {
        if (KeyValue.StartOfString[KeyValue.LengthInChars - 1] == '\0') {
            KeyValue.LengthInChars--;
        }
        if (KeyValue.LengthInChars > 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y=%y\n"), FileType, &KeyValue);
        }
    }

    YoriLibFreeStringContents(&KeyValue);
    return TRUE;
}

/**
 Display the file types of all extensions underneath the specified registry
 key.

 @param RootKey Specifies the registry key to enumerate.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocEnumerateAssociations(
    __in HKEY RootKey
    )
{
    DWORD Error;
    DWORD Index;
    YORI_STRING KeyName;
    DWORD KeyNameSize;
    FILETIME LastWritten;

    if (!YoriLibAllocateString(&KeyName, 1024)) {
        return FALSE;
    }

    Index = 0;
    while (TRUE) {

        KeyNameSize = KeyName.LengthAllocated;
        Error = DllAdvApi32.pRegEnumKeyExW(RootKey, Index, KeyName.StartOfString, &KeyNameSize, NULL, NULL, NULL, &LastWritten);

        if (Error == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (Error == ERROR_MORE_DATA) {
            KeyNameSize = KeyName.LengthAllocated * 2;
            YoriLibFreeStringContents(&KeyName);
            if (KeyNameSize > 0x40000) {
                break;
            }
            if (!YoriLibAllocateString(&KeyName, KeyNameSize)) {
                break;
            }
            continue;
        }

        if (Error != ERROR_SUCCESS) {
            break;
        }

        KeyName.LengthInChars = KeyNameSize;
        if (KeyName.LengthInChars >= 2 && KeyName.StartOfString[0] == '.') {
            AssocDisplaySingleAssociation(RootKey, &KeyName);
        }
        Index++;
    }

    YoriLibFreeStringContents(&KeyName);
    return TRUE;
}

/**
 Display the file types underneath the specified registry key.

 @param RootKey Specifies the registry key to enumerate.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocEnumerateFileTypes(
    __in HKEY RootKey
    )
{
    DWORD Error;
    DWORD Index;
    YORI_STRING KeyName;
    DWORD KeyNameSize;
    FILETIME LastWritten;

    if (!YoriLibAllocateString(&KeyName, 1024)) {
        return FALSE;
    }

    Index = 0;
    while (TRUE) {

        KeyNameSize = KeyName.LengthAllocated;
        Error = DllAdvApi32.pRegEnumKeyExW(RootKey, Index, KeyName.StartOfString, &KeyNameSize, NULL, NULL, NULL, &LastWritten);

        if (Error == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (Error == ERROR_MORE_DATA) {
            KeyNameSize = KeyName.LengthAllocated * 2;
            YoriLibFreeStringContents(&KeyName);
            if (KeyNameSize > 0x40000) {
                break;
            }
            if (!YoriLibAllocateString(&KeyName, KeyNameSize)) {
                break;
            }
            continue;
        }

        if (Error != ERROR_SUCCESS) {
            break;
        }

        KeyName.LengthInChars = KeyNameSize;
        if (KeyName.LengthInChars >= 1 && KeyName.StartOfString[0] != '.') {
            AssocDisplaySingleFileType(RootKey, &KeyName);
        }
        Index++;
    }

    YoriLibFreeStringContents(&KeyName);
    return TRUE;
}


/**
 Returns the handle to a registry key that is appropriate for the specified
 scope.

 @param Scope Specifies whether this is a machine wide, user wide, or merged
        operation.

 @param RootKey On successful completion, populated with a handle to the
        registry key.

 @param CloseKey On successful completion, set to TRUE to indicate the key
        was explicitly opened and should be closed with RegCloseKey.  If
        this value is FALSE, the registry key is a predefined key and should
        not be closed.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocOpenRegRootForScope(
    __in ASSOC_SCOPE Scope,
    __out PHKEY RootKey,
    __out PBOOLEAN CloseKey
    )
{
    DWORD Error;

    Error = ERROR_SUCCESS;
    *CloseKey = FALSE;

    switch(Scope) {
        case AssocScopeSystem:
            Error = DllAdvApi32.pRegOpenKeyExW(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Classes"), 0, KEY_ENUMERATE_SUB_KEYS, RootKey);
            *CloseKey = TRUE;
            break;
        case AssocScopeMerged:
            *RootKey = HKEY_CLASSES_ROOT;
            break;
        case AssocScopeUser:
            Error = DllAdvApi32.pRegOpenKeyExW(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes"), 0, KEY_ENUMERATE_SUB_KEYS, RootKey);
            *CloseKey = TRUE;
            break;
        default:
            return FALSE;
    }

    if (Error != ERROR_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}

/**
 Display the file types of all extensions within the specified scope.

 @param Scope Specifies the scope of the operation.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocEnumerateAssociationsForScope(
    __in ASSOC_SCOPE Scope
    )
{
    ASSOC_SCOPE EffectiveScope;
    HKEY RootKey;
    BOOLEAN CloseKey;
    BOOLEAN Result;

    EffectiveScope = Scope;
    if (EffectiveScope == AssocScopeDefault) {
        EffectiveScope = AssocScopeMerged;
    }

    if (!AssocOpenRegRootForScope(EffectiveScope, &RootKey, &CloseKey)) {
        return FALSE;
    }

    Result = AssocEnumerateAssociations(RootKey);
    if (CloseKey) {
        DllAdvApi32.pRegCloseKey(RootKey);
    }

    return Result;
}

/**
 Display the known file types within the specified scope.

 @param Scope Specifies the scope of the operation.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocEnumerateFileTypesForScope(
    __in ASSOC_SCOPE Scope
    )
{
    ASSOC_SCOPE EffectiveScope;
    HKEY RootKey;
    BOOLEAN CloseKey;
    BOOLEAN Result;

    EffectiveScope = Scope;
    if (EffectiveScope == AssocScopeDefault) {
        EffectiveScope = AssocScopeMerged;
    }

    if (!AssocOpenRegRootForScope(EffectiveScope, &RootKey, &CloseKey)) {
        return FALSE;
    }

    Result = AssocEnumerateFileTypes(RootKey);
    if (CloseKey) {
        DllAdvApi32.pRegCloseKey(RootKey);
    }

    return Result;
}

/**
 Display the file type for a specified extension within a specified scope.

 @param Scope Specifies the scope of the operation.

 @param Extension The file extension, including initial period.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocDisplayOneExtensionForScope(
    __in ASSOC_SCOPE Scope,
    __in PYORI_STRING Extension
    )
{
    ASSOC_SCOPE EffectiveScope;
    HKEY RootKey;
    BOOLEAN CloseKey;
    BOOLEAN Result;

    EffectiveScope = Scope;
    if (EffectiveScope == AssocScopeDefault) {
        EffectiveScope = AssocScopeMerged;
    }

    if (!AssocOpenRegRootForScope(EffectiveScope, &RootKey, &CloseKey)) {
        return FALSE;
    }

    Result = AssocDisplaySingleAssociation(RootKey, Extension);
    if (CloseKey) {
        DllAdvApi32.pRegCloseKey(RootKey);
    }

    if (!Result) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File association not found for extension %y\n"), Extension);
    }

    return Result;
}

/**
 Display the program associated with a specified file type within a specified
 scope.

 @param Scope Specifies the scope of the operation.

 @param FileType The file type.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocDisplayOneFileTypeForScope(
    __in ASSOC_SCOPE Scope,
    __in PYORI_STRING FileType
    )
{
    ASSOC_SCOPE EffectiveScope;
    HKEY RootKey;
    BOOLEAN CloseKey;
    BOOLEAN Result;

    EffectiveScope = Scope;
    if (EffectiveScope == AssocScopeDefault) {
        EffectiveScope = AssocScopeMerged;
    }

    if (!AssocOpenRegRootForScope(EffectiveScope, &RootKey, &CloseKey)) {
        return FALSE;
    }

    Result = AssocDisplaySingleFileType(RootKey, FileType);
    if (CloseKey) {
        DllAdvApi32.pRegCloseKey(RootKey);
    }

    if (!Result) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File type '%y' not found or no open command associated with it.\n"), FileType);
    }

    return Result;
}

/**
 Update the file type associated with a specified extension.

 @param Scope Specifies the scope of the operation.

 @param Extension The file extension, including initial period.

 @param NewType Specifies the new file type.  If this is an empty string, the
        existing file type is deleted.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocUpdateAssociation(
    __in ASSOC_SCOPE Scope,
    __in PYORI_STRING Extension,
    __in PYORI_STRING NewType
    )
{
    ASSOC_SCOPE EffectiveScope;
    HKEY RootKey;
    BOOLEAN CloseKey;
    BOOLEAN Result;

    EffectiveScope = Scope;
    if (EffectiveScope == AssocScopeDefault) {
        EffectiveScope = AssocScopeSystem;
    }

    if (!AssocOpenRegRootForScope(EffectiveScope, &RootKey, &CloseKey)) {
        return FALSE;
    }

    Result = AssocUpdateSingleAssociation(RootKey, Extension, NewType);
    if (CloseKey) {
        DllAdvApi32.pRegCloseKey(RootKey);
    }

    if (!Result) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Error occurred while processing: %y.\n"), Extension);
    }

    return Result;
}

/**
 Update the program associated with a file type.

 MSFIX This doesn't work well right now because it's very common to want to
 use strings such as "%1" (with quotes) in the program string and ArgC/ArgV
 parsing strips these out.  This state should be received by the process
 though, so it may need a tweaked ArgC/ArgV parser...

 @param Scope Specifies the scope of the operation.

 @param FileType The file type.

 @param NewProgram Specifies the new program to associate with the file type.
        If this is an empty string, the existing file type is deleted.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
AssocUpdateFileType(
    __in ASSOC_SCOPE Scope,
    __in PYORI_STRING FileType,
    __in PYORI_STRING NewProgram
    )
{
    ASSOC_SCOPE EffectiveScope;
    HKEY RootKey;
    BOOLEAN CloseKey;
    BOOLEAN Result;

    EffectiveScope = Scope;
    if (EffectiveScope == AssocScopeDefault) {
        EffectiveScope = AssocScopeSystem;
    }

    if (!AssocOpenRegRootForScope(EffectiveScope, &RootKey, &CloseKey)) {
        return FALSE;
    }

    Result = AssocUpdateSingleFileType(RootKey, FileType, NewProgram);
    if (CloseKey) {
        DllAdvApi32.pRegCloseKey(RootKey);
    }

    if (!Result) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Error occurred while processing: %y.\n"), FileType);
    }

    return Result;
}



#ifdef YORI_BUILTIN
/**
 The main entrypoint for the assoc builtin command.
 */
#define ENTRYPOINT YoriCmd_YASSOC
#else
/**
 The main entrypoint for the assoc standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the assoc cmdlet.

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
    ASSOC_SCOPE Scope;
    BOOLEAN FileTypeMode;

    Scope = AssocScopeDefault;
    FileTypeMode = FALSE;
    YoriLibLoadAdvApi32Functions();

    if (DllAdvApi32.pRegCloseKey == NULL ||
        DllAdvApi32.pRegDeleteKeyW == NULL ||
        DllAdvApi32.pRegDeleteValueW == NULL ||
        DllAdvApi32.pRegEnumKeyExW == NULL ||
        DllAdvApi32.pRegQueryValueExW == NULL ||
        DllAdvApi32.pRegOpenKeyExW == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("assoc: OS support not present\n"));
        return EXIT_FAILURE;
    }

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                AssocHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2020"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("m")) == 0) {
                Scope = AssocScopeMerged;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Scope = AssocScopeSystem;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("t")) == 0) {
                FileTypeMode = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("u")) == 0) {
                Scope = AssocScopeUser;
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

    if (StartArg == 0 || StartArg >= ArgC) {
        if (FileTypeMode) {
            AssocEnumerateFileTypesForScope(Scope);
        } else {
            AssocEnumerateAssociationsForScope(Scope);
        }
    } else {
        YORI_STRING Value;
        YORI_STRING CmdLine;
        YORI_STRING Variable;

        //
        //  Attempt to capture the remainder of the command line.  When
        //  compiled as an external program, this reparses argc/argv to
        //  keep the remainder in a single string, complete with quotes.
        //

#ifdef YORI_BUILTIN
        if (!YoriLibBuildCmdlineFromArgcArgv(ArgC - StartArg, &ArgV[StartArg], FALSE, &CmdLine)) {
            return EXIT_FAILURE;
        }
#else
        {
            PYORI_STRING SubArgV;
            DWORD SubArgC;
            SubArgV = YoriLibCmdlineToArgcArgv(GetCommandLine(), StartArg + 1, &SubArgC);
            if (SubArgV == NULL) {
                return EXIT_FAILURE;
            }

            if (SubArgC > StartArg) {
                memcpy(&CmdLine, &SubArgV[StartArg], sizeof(YORI_STRING));
                YoriLibReference(CmdLine.MemoryToFree);
            } else {
                YoriLibInitEmptyString(&CmdLine);
            }

            for (;SubArgC > 0; SubArgC--) {
                YoriLibFreeStringContents(&SubArgV[SubArgC - 1]);
            }
            YoriLibDereference(SubArgV);

            if (CmdLine.StartOfString == NULL) {
                return EXIT_FAILURE;
            }
        }
#endif

        memcpy(&Variable, &CmdLine, sizeof(YORI_STRING));

        //
        //  At this point escapes are still present but it's never valid to
        //  have an '=' in a variable name, escape or not.
        //

        YoriLibInitEmptyString(&Value);
        Value.StartOfString = YoriLibFindLeftMostCharacter(&Variable, '=');
        if (Value.StartOfString) {
            Value.StartOfString[0] = '\0';
            Value.StartOfString++;
            Variable.LengthAllocated = (DWORD)(Value.StartOfString - CmdLine.StartOfString);
            Variable.LengthInChars = Variable.LengthAllocated - 1;
            Value.LengthInChars = CmdLine.LengthInChars - Variable.LengthAllocated;
            Value.LengthAllocated = CmdLine.LengthAllocated - Variable.LengthAllocated;
        }

        if (FileTypeMode) {
            if (Value.StartOfString == NULL) {
                AssocDisplayOneFileTypeForScope(Scope, &Variable);
            } else {
                AssocUpdateFileType(Scope, &Variable, &Value);
            }
        } else {
            if (Value.StartOfString == NULL) {
                AssocDisplayOneExtensionForScope(Scope, &Variable);
            } else {
                AssocUpdateAssociation(Scope, &Variable, &Value);
            }
        }

        YoriLibFreeStringContents(&CmdLine);
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
