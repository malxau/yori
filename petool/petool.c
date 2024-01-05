/**
 * @file petool/petool.c
 *
 * Yori shell PE tool for manipulating PE files
 *
 * Copyright (c) 2021-2022 Malcolm J. Smith
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
CHAR strPeToolHelpText[] =
        "\n"
        "Manage PE files.\n"
        "\n"
        "PETOOL [-license]\n"
        "PETOOL -c file\n"
        "PETOOL -cu file\n"
        "PETOOL -os file version\n"
        "\n"
        "   -c             Calculate the PE checksum for a binary\n"
        "   -cu            Update the checksum in the PE header from contents\n"
        "   -os            Set the minimum OS version and update checksum\n";

/**
 Display usage text to the user.
 */
BOOL
PeToolHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("PeTool %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strPeToolHelpText);
    return TRUE;
}

/**
 A structure describing a major/minor version pair.
 */
typedef struct _PETOOL_KNOWN_VERSION {

    /**
     The major version component.
     */
    WORD Major;

    /**
     The minor version component.
     */
    WORD Minor;
} PETOOL_KNOWN_VERSION;

/**
 A structure describing a minimum and maximum major/minor version pair for
 a specific machine type.
 */
typedef struct _PETOOL_VERSION_RANGE {
    /**
     The machine type.
     */
    WORD Machine;

    /**
     The minimum major version for the machine type.
     */
    WORD MinimumMajor;

    /**
     The minimum minor version for the machine type.
     */
    WORD MinimumMinor;

    /**
     The maximum major version for the machine type.
     */
    WORD MaximumMajor;

    /**
     The maximum minor version for the machine type.
     */
    WORD MaximumMinor;

} PETOOL_VERSION_RANGE, *PPETOOL_VERSION_RANGE;

/**
 A pointer to a constant version range.
 */
typedef PETOOL_VERSION_RANGE CONST * PCPETOOL_VERSION_RANGE;

/**
 An array of known Windows versions that this program will allow to be
 stamped into executable files.
 */
CONST PETOOL_KNOWN_VERSION
PeToolKnownVersions[] = {
    {3,  10},
    {3,  50},
    {3,  51},
    {4,  0},
    {5,  0},
    {5,  1},
    {5,  2},
    {6,  0},
    {6,  1},
    {6,  2},
    {6,  3},
    {10, 0}
};

/**
 An array of Windows versions for specific executable architectures.
 */
CONST PETOOL_VERSION_RANGE
PeToolVersionRange[] = {
    {IMAGE_FILE_MACHINE_I386,      3, 10,  10,  0},
    {IMAGE_FILE_MACHINE_R3000,     3, 10,   4,  0},
    {IMAGE_FILE_MACHINE_R4000,     3, 10,   4,  0},
    {IMAGE_FILE_MACHINE_R10000,    3, 10,   4,  0},
    {IMAGE_FILE_MACHINE_ALPHA,     3, 10,   4,  0},
    {IMAGE_FILE_MACHINE_POWERPC,   3, 51,   4,  0},
    {IMAGE_FILE_MACHINE_IA64,      5,  1,   6,  1},
    {IMAGE_FILE_MACHINE_AMD64,     5,  2,  10,  0},
    {IMAGE_FILE_MACHINE_ARMNT,     6,  2,  10,  0},
    {IMAGE_FILE_MACHINE_ARM64,    10,  0,  10,  0}
};

/**
 Write a new checksum to a PE header in a file.

 @param FullPath Pointer to the executable file name.

 @param Checksum The new checksum to apply to the PE header.

 @return Win32 error code, including ERROR_SUCCESS to indicate success.
 */
DWORD
PeToolWriteChecksumToFile(
    __in PYORI_STRING FullPath,
    __in DWORD Checksum
    )
{
    DWORD PeHeaderOffset;
    HANDLE hFileRead;
    union {
        IMAGE_DOS_HEADER DosHeader;
        YORILIB_PE_HEADERS PeHeaders;
    } u;
    DWORD BytesReturned;
    DWORD Err;

    //
    //  We want the earlier handle to be attribute only so we can
    //  operate on directories, but we need data for this, so we
    //  end up with two handles.
    //

    hFileRead = CreateFile(FullPath->StartOfString,
                           FILE_READ_ATTRIBUTES|FILE_READ_DATA|FILE_WRITE_DATA,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);

    if (hFileRead == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        return Err;
    }

    if (ReadFile(hFileRead, &u.DosHeader, sizeof(u.DosHeader), &BytesReturned, NULL) &&
        BytesReturned == sizeof(u.DosHeader) &&
        u.DosHeader.e_magic == IMAGE_DOS_SIGNATURE &&
        u.DosHeader.e_lfanew != 0) {

        PeHeaderOffset = u.DosHeader.e_lfanew;
        SetFilePointer(hFileRead, PeHeaderOffset, NULL, FILE_BEGIN);

        if (ReadFile(hFileRead, &u.PeHeaders, sizeof(YORILIB_PE_HEADERS), &BytesReturned, NULL) &&
            BytesReturned == sizeof(YORILIB_PE_HEADERS) &&
            u.PeHeaders.Signature == IMAGE_NT_SIGNATURE &&
            u.PeHeaders.ImageHeader.SizeOfOptionalHeader >= FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(u.PeHeaders.OptionalHeader.CheckSum)) {

            u.PeHeaders.OptionalHeader.CheckSum = Checksum;
            SetFilePointer(hFileRead, PeHeaderOffset, NULL, FILE_BEGIN);

            if (!WriteFile(hFileRead, &u.PeHeaders, sizeof(YORILIB_PE_HEADERS), &BytesReturned, NULL)) {
                Err = GetLastError();
                CloseHandle(hFileRead);
                return Err;
            }

            CloseHandle(hFileRead);
            return ERROR_SUCCESS;
        }
    }

    CloseHandle(hFileRead);
    return ERROR_BAD_EXE_FORMAT;
}

/**
 Check if the specified version is valid for a specified architecture.
 This includes checking if the specified version is a valid version, and if
 the executable architecture was supported by the version.

 @param Machine The architecture of the executable file.

 @param MajorVersion The new minimum OS major version.

 @param MinorVersion The new minimum OS minor version.

 @return TRUE if the version is supported on this architecture, FALSE if not.
 */
BOOLEAN
PeToolIsVersionValidForArchitecture(
    __in WORD Machine,
    __in WORD MajorVersion,
    __in WORD MinorVersion
    )
{
    DWORD Index;
    PCPETOOL_VERSION_RANGE Range;

    //
    //  Check if the version specified is a known Windows version.
    //  Windows rejects binaries with unknown versions.
    //

    for (Index = 0; Index < sizeof(PeToolKnownVersions) / sizeof(PeToolKnownVersions[0]); Index++) {
        if (PeToolKnownVersions[Index].Major == MajorVersion &&
            PeToolKnownVersions[Index].Minor == MinorVersion) {

            break;
        }
    }

    if (Index >= sizeof(PeToolKnownVersions)/sizeof(PeToolKnownVersions[0])) {
        return FALSE;
    }

    //
    //  Check if the version is before the first OS version for
    //  the architecture of the binary.
    //

    for (Index = 0; Index < sizeof(PeToolVersionRange) / sizeof(PeToolVersionRange[0]); Index++) {
        Range = &PeToolVersionRange[Index];
        if (Range->Machine == Machine) {
            if (MajorVersion < Range->MinimumMajor ||
                (MajorVersion == Range->MinimumMajor &&
                 MinorVersion < Range->MinimumMinor)) {

                return FALSE;
            } else if (MajorVersion > Range->MaximumMajor ||
                       (MajorVersion == Range->MaximumMajor &&
                        MinorVersion > Range->MaximumMinor)) {

                return FALSE;
            } else {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/**
 Check if a specified version is applicable to an executable file, and if so,
 update the minimum OS version in the header with the specified values.

 @param FullPath Pointer to the executable file name.

 @param MajorVersion Specifies the new minimum major version.

 @param MinorVersion Specifies the new minimum minor version.

 @return Win32 error code, including ERROR_SUCCESS to indicate success.
 */
DWORD
PeToolWriteSubsystemVersionToFile(
    __in PYORI_STRING FullPath,
    __in WORD MajorVersion,
    __in WORD MinorVersion
    )
{
    DWORD PeHeaderOffset;
    HANDLE hFileRead;
    union {
        IMAGE_DOS_HEADER DosHeader;
        YORILIB_PE_HEADERS PeHeaders;
    } u;
    DWORD BytesReturned;
    DWORD Err;

    //
    //  We want the earlier handle to be attribute only so we can
    //  operate on directories, but we need data for this, so we
    //  end up with two handles.
    //

    hFileRead = CreateFile(FullPath->StartOfString,
                           FILE_READ_ATTRIBUTES|FILE_READ_DATA|FILE_WRITE_DATA,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);

    if (hFileRead == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        return Err;
    }

    if (ReadFile(hFileRead, &u.DosHeader, sizeof(u.DosHeader), &BytesReturned, NULL) &&
        BytesReturned == sizeof(u.DosHeader) &&
        u.DosHeader.e_magic == IMAGE_DOS_SIGNATURE &&
        u.DosHeader.e_lfanew != 0) {

        PeHeaderOffset = u.DosHeader.e_lfanew;
        SetFilePointer(hFileRead, PeHeaderOffset, NULL, FILE_BEGIN);

        if (ReadFile(hFileRead, &u.PeHeaders, sizeof(YORILIB_PE_HEADERS), &BytesReturned, NULL) &&
            BytesReturned == sizeof(YORILIB_PE_HEADERS) &&
            u.PeHeaders.Signature == IMAGE_NT_SIGNATURE &&
            u.PeHeaders.ImageHeader.SizeOfOptionalHeader >= FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(u.PeHeaders.OptionalHeader.CheckSum)) {

            if (!PeToolIsVersionValidForArchitecture(u.PeHeaders.ImageHeader.Machine, MajorVersion, MinorVersion)) {
                CloseHandle(hFileRead);
                return ERROR_OLD_WIN_VERSION;
            }

            u.PeHeaders.OptionalHeader.MajorSubsystemVersion = MajorVersion;
            u.PeHeaders.OptionalHeader.MinorSubsystemVersion = MinorVersion;
            SetFilePointer(hFileRead, PeHeaderOffset, NULL, FILE_BEGIN);

            if (!WriteFile(hFileRead, &u.PeHeaders, sizeof(YORILIB_PE_HEADERS), &BytesReturned, NULL)) {
                Err = GetLastError();
                CloseHandle(hFileRead);
                return Err;
            }

            CloseHandle(hFileRead);
            return ERROR_SUCCESS;
        }
    }

    CloseHandle(hFileRead);
    return ERROR_BAD_EXE_FORMAT;
}


/**
 Calculate and display the checksum for a specified file.

 @param FileName Pointer to the file name to calculate a checksum for.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
PeToolCalculateChecksum(
    __in PYORI_STRING FileName
    )
{
    DWORD Err;
    DWORD HeaderChecksum;
    DWORD DataChecksum;
    YORI_STRING FullPath;

    YoriLibLoadImageHlpFunctions();

    if (DllImageHlp.pMapFileAndCheckSumW == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("petool: OS support not present\n"));
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        return FALSE;
    }

    Err = DllImageHlp.pMapFileAndCheckSumW(FullPath.StartOfString, &HeaderChecksum, &DataChecksum);
    if (Err != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %y\n"), FileName);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Checksum in PE header: %08x\n")
                                          _T("Checksum of file contents: %08x\n"),
                                          HeaderChecksum,
                                          DataChecksum);

    YoriLibFreeStringContents(&FullPath);
    return TRUE;
}

/**
 Generate the checksum for a binary and update the header.

 @param FileName Pointer to the file name to generate a checksum for.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
PeToolUpdateChecksum(
    __in PYORI_STRING FileName
    )
{
    DWORD Err;
    DWORD HeaderChecksum;
    DWORD DataChecksum;
    YORI_STRING FullPath;

    YoriLibLoadImageHlpFunctions();

    if (DllImageHlp.pMapFileAndCheckSumW == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("petool: OS support not present\n"));
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        return FALSE;
    }

    Err = DllImageHlp.pMapFileAndCheckSumW(FullPath.StartOfString, &HeaderChecksum, &DataChecksum);
    if (Err != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %y\n"), FileName);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    ASSERT(YoriLibIsStringNullTerminated(&FullPath));

    Err = PeToolWriteChecksumToFile(&FullPath, DataChecksum);
    if (Err != ERROR_SUCCESS) {
        LPTSTR ErrText;
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of file failed: %y: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
    }

    YoriLibFreeStringContents(&FullPath);

    return TRUE;
}

/**
 Update the subsystem version for a PE file and regenerate the checksum.

 @param FileName Pointer to the file name to generate a checksum for.

 @param NewSubsystemVersion Pointer to the new subsystem version.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
PeToolUpdateSubsystemVersion(
    __in PYORI_STRING FileName,
    __in PYORI_STRING NewSubsystemVersion
    )
{
    DWORD Err;
    DWORD HeaderChecksum;
    DWORD DataChecksum;
    YORI_STRING FullPath;
    YORI_STRING WinVer;
    YORI_MAX_SIGNED_T llTemp;
    YORI_ALLOC_SIZE_T CharsConsumed;
    WORD MajorVersion;
    WORD MinorVersion;

    YoriLibLoadImageHlpFunctions();

    if (DllImageHlp.pMapFileAndCheckSumW == NULL) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("petool: OS support not present\n"));
        return FALSE;
    }

    YoriLibInitEmptyString(&FullPath);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullPath)) {
        return FALSE;
    }

    MajorVersion = 0;
    MinorVersion = 0;
    YoriLibInitEmptyString(&WinVer);
    WinVer.StartOfString = NewSubsystemVersion->StartOfString;
    WinVer.LengthInChars = NewSubsystemVersion->LengthInChars;
    if (YoriLibStringToNumber(&WinVer, FALSE, &llTemp, &CharsConsumed)) {
        MajorVersion = (WORD)llTemp;
        WinVer.LengthInChars = WinVer.LengthInChars - CharsConsumed;
        WinVer.StartOfString += CharsConsumed;
        if (WinVer.LengthInChars > 0) {
            WinVer.LengthInChars -= 1;
            WinVer.StartOfString += 1;
            if (YoriLibStringToNumber(&WinVer, FALSE, &llTemp, &CharsConsumed)) {
                MinorVersion = (WORD)llTemp;
            }
        }
    }

    Err = PeToolWriteSubsystemVersionToFile(&FullPath, MajorVersion, MinorVersion);
    if (Err == ERROR_OLD_WIN_VERSION) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("The specified version is not valid for the processor architecture of this program: %y\n"), &FullPath);
    } else if (Err == ERROR_BAD_EXE_FORMAT) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("This file is not a valid Windows executable: %y\n"), &FullPath);
    } else if (Err != ERROR_SUCCESS) {
        LPTSTR ErrText;
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of file failed: %y: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    Err = DllImageHlp.pMapFileAndCheckSumW(FullPath.StartOfString, &HeaderChecksum, &DataChecksum);
    if (Err != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %y\n"), FileName);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    ASSERT(YoriLibIsStringNullTerminated(&FullPath));

    Err = PeToolWriteChecksumToFile(&FullPath, DataChecksum);
    if (Err != ERROR_SUCCESS) {
        LPTSTR ErrText;
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of file failed: %y: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);

    return TRUE;
}

/**
 A set of operations supported by this application.
 */
typedef enum _PETOOL_OP {
    PeToolOpNone = 0,
    PeToolOpCalculateChecksum = 1,
    PeToolOpUpdateChecksum = 2,
    PeToolOpUpdateSubsystemVersion = 3,
} PETOOL_OP;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the petool builtin command.
 */
#define ENTRYPOINT YoriCmd_PETOOL
#else
/**
 The main entrypoint for the petool standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the petool cmdlet.

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
    YORI_ALLOC_SIZE_T StartArg;
    YORI_STRING Arg;
    PYORI_STRING FileName = NULL;
    PYORI_STRING NewSubsystemVersion = NULL;
    PETOOL_OP Op;
    DWORD Result;

    Op = PeToolOpNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                PeToolHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2021"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                if (ArgC > i + 1) {
                    FileName = &ArgV[i + 1];
                    Op = PeToolOpCalculateChecksum;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("cu")) == 0) {
                if (ArgC > i + 1) {
                    FileName = &ArgV[i + 1];
                    Op = PeToolOpUpdateChecksum;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("os")) == 0) {
                if (ArgC > i + 2) {
                    FileName = &ArgV[i + 1];
                    NewSubsystemVersion = &ArgV[i + 2];
                    Op = PeToolOpUpdateSubsystemVersion;
                    ArgumentUnderstood = TRUE;
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

    if (Op == PeToolOpNone) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("petool: operation not specified\n"));
        return EXIT_FAILURE;
    }

    Result = EXIT_SUCCESS;

    switch(Op) {
        case PeToolOpCalculateChecksum:
            if (!PeToolCalculateChecksum(FileName)) {
                Result = EXIT_FAILURE;
            }
            break;
        case PeToolOpUpdateChecksum:
            if (!PeToolUpdateChecksum(FileName)) {
                Result = EXIT_FAILURE;
            }
            break;
        case PeToolOpUpdateSubsystemVersion:
            if (!PeToolUpdateSubsystemVersion(FileName, NewSubsystemVersion)) {
                Result = EXIT_FAILURE;
            }
            break;
    }

    return Result;
}

// vim:sw=4:ts=4:et:
