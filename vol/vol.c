/**
 * @file vol/vol.c
 *
 * Yori shell display volume properties
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
CHAR strVolHelpText[] =
        "\n"
        "Outputs volume information in a specified format.\n"
        "\n"
        "VOL [-license] [-f <fmt>] [<vol>]\n"
        "\n"
        "Format specifiers are:\n"
        "   $clustersize$        The size of each cluster in bytes\n"
        "   $filerecordsize$     The size of each NTFS file record\n"
        "   $free$               Free space in bytes\n"
        "   $fsname$             The file system name\n"
        "   $fullserial$         The 64 bit serial number\n"
        "   $label$              The volume label\n"
        "   $mftsize$            The amount of bytes consumed in the NTFS MFT\n"
        "   $physicalsectorsize$ The size of each physical sector in bytes\n"
        "   $reserved$           Reserved space in bytes\n"
        "   $sectorsize$         The size of each logical sector in bytes\n"
        "   $serial$             The 32 bit serial number\n"
        "   $size$               Volume size in bytes\n"
        "   $usnjournalid$       The identifier for this USN journal on the volume\n"
        "   $usnfirst$           The first USN number in the journal\n"
        "   $usnnext$            The next USN number to add to the journal\n"
        "   $usnlowestvalid$     The minimum valid USN number\n"
        "   $usnmax$             The maximum valid USN number\n"
        "   $usnmaxallocated$    The maximum size of the journal in bytes\n"
        ;

/**
 Display usage text to the user.
 */
BOOL
VolHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Vol %i.%02i\n"), VOL_VER_MAJOR, VOL_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strVolHelpText);
    return TRUE;
}

/**
 A context structure to pass to the function expanding variables so it knows
 what values to use.
 */
typedef struct _VOL_RESULT {

    /**
     Set to TRUE if a variable was specified that could not be expanded.
     This indicates either a caller error in asking for an unknown variable,
     or asking for a variable which wasn't available for the specified
     volume.  When this occurs, the process exits with a failure code.
     */
    BOOL VariableExpansionFailure;

    /**
     Flags indicating which data has been collected.
     */
    union {
        DWORD All;
        struct {
            BOOLEAN GetVolInfo:1;
            BOOLEAN FreeSpace:1;
            BOOLEAN SectorSize:1;
            BOOLEAN PhysicalSectorSize:1;
            BOOLEAN Usn:1;
            BOOLEAN NtfsData:1;
            BOOLEAN RefsData:1;
            BOOLEAN Reserved:1;
            BOOLEAN FullSerial:1;
        };
    } Have;

    /**
     The volume label for this volume.
     */
    YORI_STRING VolumeLabel;

    /**
     A yori string containing the file system name, for example "FAT" or
     "NTFS".
     */
    YORI_STRING FsName;

    /**
     A 32 bit volume serial number.  NT internally uses 64 bit serial numbers,
     this is what's returned from Win32.
     */
    DWORD ShortSerialNumber;

    /**
     The file system capability flags.
     */
    DWORD Capabilities;

    /**
     The size of each sector in bytes.
     */
    DWORD SectorSize;

    /**
     The size of each cluster in bytes.
     */
    DWORD ClusterSize;

    /**
     The size of each physical sector in bytes.
     */
    DWORD PhysicalSectorSize;

    /**
     The size of the volume, in bytes.
     */
    LARGE_INTEGER VolumeSize;

    /**
     Free space on the volume, in bytes.
     */
    LARGE_INTEGER FreeSpace;

    /**
     The USN journal identifier. This is generated each time a new journal is
     created.
     */
    DWORDLONG UsnJournalId;

    /**
     The first valid USN record within the journal.
     */
    DWORDLONG UsnFirst;

    /**
     The next USN number to allocate.  All numbers between UsnFirst and
     UsnNext exclusive are valid in the journal.
     */
    DWORDLONG UsnNext;

    /**
     The lowest valid USN number.
     */
    DWORDLONG UsnLowestValid;

    /**
     The maximum valid USN number.
     */
    DWORDLONG UsnMax;

    /**
     The maximum size of the journal in bytes.
     */
    DWORDLONG UsnMaxAllocated;

    /**
     The number of bytes in the volume that are reserved for use by the file
     system.
     */
    DWORDLONG ReservedSize;

    /**
     A 64 bit volume serial number from file systems willing to return it.
     */
    DWORDLONG FullSerialNumber;

    /**
     Extended information returned from NTFS.
     */
    NTFS_VOLUME_DATA_BUFFER NtfsData;

    /**
     Extended information returned from REFS.
     */
    REFS_VOLUME_DATA_BUFFER RefsData;

} VOL_RESULT, *PVOL_RESULT;

/**
 A callback function to expand any known variables found when parsing the
 format string.

 @param OutputString A pointer to the output string to populate with data
        if a known variable is found.  The length allocated contains the
        length that can be populated with data.

 @param VariableName The variable name to expand.

 @param Context Pointer to a @ref VOL_RESULT structure containing
        the data to populate.
 
 @return The number of characters successfully populated, or the number of
         characters required in order to successfully populate, or zero
         on error.
 */
DWORD
VolExpandVariables(
    __inout PYORI_STRING OutputString,
    __in PYORI_STRING VariableName,
    __in PVOID Context
    )
{
    DWORD CharsNeeded;
    PVOL_RESULT VolContext = (PVOL_RESULT)Context;

    if (VolContext->Have.SectorSize &&
        YoriLibCompareStringWithLiteral(VariableName, _T("clustersize")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), VolContext->ClusterSize);

    } else if (VolContext->Have.NtfsData &&
               YoriLibCompareStringWithLiteral(VariableName, _T("filerecordsize")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), VolContext->NtfsData.BytesPerFileRecordSegment);

    } else if (VolContext->Have.FreeSpace &&
               YoriLibCompareStringWithLiteral(VariableName, _T("free")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), VolContext->FreeSpace.QuadPart);

    } else if (VolContext->Have.GetVolInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("fsname")) == 0) {

        CharsNeeded = VolContext->FsName.LengthInChars;

    } else if (VolContext->Have.FullSerial &&
               YoriLibCompareStringWithLiteral(VariableName, _T("fullserial")) == 0) {

        CharsNeeded = 16;

    } else if (VolContext->Have.GetVolInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("label")) == 0) {

        CharsNeeded = VolContext->VolumeLabel.LengthInChars;

    } else if (VolContext->Have.NtfsData &&
               YoriLibCompareStringWithLiteral(VariableName, _T("mftsize")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), VolContext->NtfsData.MftValidDataLength);

    } else if (VolContext->Have.PhysicalSectorSize &&
               YoriLibCompareStringWithLiteral(VariableName, _T("physicalsectorsize")) == 0) {
        CharsNeeded = YoriLibSPrintfSize(_T("%i"), VolContext->PhysicalSectorSize);

    } else if (VolContext->Have.Reserved &&
               YoriLibCompareStringWithLiteral(VariableName, _T("reserved")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), VolContext->ReservedSize);

    } else if (VolContext->Have.SectorSize &&
               YoriLibCompareStringWithLiteral(VariableName, _T("sectorsize")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%i"), VolContext->SectorSize);

    } else if (VolContext->Have.GetVolInfo &&
               YoriLibCompareStringWithLiteral(VariableName, _T("serial")) == 0) {

        CharsNeeded = 8;

    } else if (VolContext->Have.FreeSpace &&
               YoriLibCompareStringWithLiteral(VariableName, _T("size")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), VolContext->VolumeSize.QuadPart);

    } else if (VolContext->Have.Usn &&
               YoriLibCompareStringWithLiteral(VariableName, _T("usnjournalid")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%llx"), VolContext->UsnJournalId);

    } else if (VolContext->Have.Usn &&
               YoriLibCompareStringWithLiteral(VariableName, _T("usnfirst")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), VolContext->UsnFirst);

    } else if (VolContext->Have.Usn &&
               YoriLibCompareStringWithLiteral(VariableName, _T("usnnext")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), VolContext->UsnNext);

    } else if (VolContext->Have.Usn &&
               YoriLibCompareStringWithLiteral(VariableName, _T("usnlowestvalid")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), VolContext->UsnLowestValid);

    } else if (VolContext->Have.Usn &&
               YoriLibCompareStringWithLiteral(VariableName, _T("usnmax")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), VolContext->UsnMax);

    } else if (VolContext->Have.Usn &&
               YoriLibCompareStringWithLiteral(VariableName, _T("usnmaxallocated")) == 0) {

        CharsNeeded = YoriLibSPrintfSize(_T("%lli"), VolContext->UsnMaxAllocated);

    } else {
        VolContext->VariableExpansionFailure = TRUE;
        return 0;
    }

    if (OutputString->LengthAllocated < CharsNeeded) {
        return CharsNeeded;
    }

    if (YoriLibCompareStringWithLiteral(VariableName, _T("clustersize")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%i"), VolContext->ClusterSize);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("filerecordsize")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%i"), VolContext->NtfsData.BytesPerFileRecordSegment);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("free")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%lli"), VolContext->FreeSpace.QuadPart);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("fsname")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%y"), &VolContext->FsName);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("fullserial")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%016llx"), VolContext->FullSerialNumber);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("label")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%y"), &VolContext->VolumeLabel);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("mftsize")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%lli"), VolContext->NtfsData.MftValidDataLength);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("physicalsectorsize")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%i"), VolContext->PhysicalSectorSize);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("reserved")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%lli"), VolContext->ReservedSize);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("sectorsize")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%i"), VolContext->SectorSize);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("serial")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%08x"), VolContext->ShortSerialNumber);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("size")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%lli"), VolContext->VolumeSize.QuadPart);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("usnjournalid")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%llx"), VolContext->UsnJournalId);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("usnfirst")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%lli"), VolContext->UsnFirst);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("usnnext")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%lli"), VolContext->UsnNext);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("usnlowestvalid")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%lli"), VolContext->UsnLowestValid);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("usnmax")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%lli"), VolContext->UsnMax);
    } else if (YoriLibCompareStringWithLiteral(VariableName, _T("usnmaxallocated")) == 0) {
        YoriLibSPrintf(OutputString->StartOfString, _T("%lli"), VolContext->UsnMaxAllocated);
    }

    OutputString->LengthInChars = CharsNeeded;
    return CharsNeeded;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the vol builtin command.
 */
#define ENTRYPOINT YoriCmd_YVOL
#else
/**
 The main entrypoint for the vol standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the vol cmdlet.

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
    VOL_RESULT VolResult;
    BOOL ArgumentUnderstood;
    DWORD MaxComponentLength;
    YORI_STRING DisplayString;
    YORI_STRING YsFormatString;
    DWORD StartArg = 0;
    DWORD i;
    DWORD SectorsPerCluster;
    DWORD NumberOfFreeClusters;
    DWORD TotalNumberOfClusters;
    YORI_STRING Arg;
    YORI_STRING FullPathName;
    YORI_STRING VolRootName;
    HANDLE hDir;

    YoriLibInitEmptyString(&YsFormatString);

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                VolHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("f")) == 0) {
                if (ArgC > i + 1) {
                    YsFormatString.StartOfString = ArgV[i + 1].StartOfString;
                    YsFormatString.LengthInChars = ArgV[i + 1].LengthInChars;
                    YsFormatString.LengthAllocated = ArgV[i + 1].LengthAllocated;
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
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }


    if (StartArg == 0 || StartArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vol: missing argument\n"));
        return EXIT_FAILURE;
    }

    ZeroMemory(&VolResult, sizeof(VolResult));

    if (!YoriLibAllocateString(&VolResult.VolumeLabel, 256)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&VolResult.FsName, 256)) {
        YoriLibFreeStringContents(&VolResult.VolumeLabel);
        return FALSE;
    }

    if (!YoriLibUserStringToSingleFilePath(&ArgV[StartArg], TRUE, &FullPathName)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mklink: failed to resolve %y\n"), &ArgV[StartArg]);
        YoriLibFreeStringContents(&VolResult.VolumeLabel);
        YoriLibFreeStringContents(&VolResult.FsName);
        return EXIT_FAILURE;
    }

    //
    //  We want to translate the user specified path into a volume root.
    //  Windows 2000 and above have a nice API for this, which says it's
    //  guaranteed to return less than or equal to the size of the input
    //  string, so we allocate the input string, plus space for a trailing
    //  backslash and a NULL terminator.
    //

    if (!YoriLibAllocateString(&VolRootName, FullPathName.LengthInChars + 2)) {
        YoriLibFreeStringContents(&VolResult.VolumeLabel);
        YoriLibFreeStringContents(&VolResult.FsName);
        YoriLibFreeStringContents(&FullPathName);
        return EXIT_FAILURE;
    }

    if (DllKernel32.pGetVolumePathNameW != NULL &&
        DllKernel32.pGetVolumePathNameW(FullPathName.StartOfString, VolRootName.StartOfString, VolRootName.LengthAllocated)) {

        VolRootName.LengthInChars = _tcslen(VolRootName.StartOfString);
    } else {

        YORI_STRING EffectiveRoot;
        DWORD CharsToCopy;
        YoriLibInitEmptyString(&EffectiveRoot);

        if (YoriLibFindEffectiveRoot(&FullPathName, &EffectiveRoot)) {
            CharsToCopy = EffectiveRoot.LengthInChars;
        } else {
            CharsToCopy = FullPathName.LengthInChars;
        }

        memcpy(VolRootName.StartOfString, FullPathName.StartOfString, CharsToCopy * sizeof(TCHAR));
        VolRootName.LengthInChars = CharsToCopy;
        VolRootName.StartOfString[CharsToCopy] = '\0';
    }


    //
    //  GetVolumeInformation wants a name with a trailing backslash.  Add one
    //  if needed.
    //

    if (VolRootName.LengthInChars > 0 &&
        VolRootName.LengthInChars + 1 < VolRootName.LengthAllocated &&
        VolRootName.StartOfString[VolRootName.LengthInChars - 1] != '\\') {

        VolRootName.StartOfString[VolRootName.LengthInChars] = '\\';
        VolRootName.StartOfString[VolRootName.LengthInChars + 1] = '\0';
        VolRootName.LengthInChars++;
    }

    if (GetVolumeInformation(VolRootName.StartOfString,
                             VolResult.VolumeLabel.StartOfString,
                             VolResult.VolumeLabel.LengthAllocated,
                             &VolResult.ShortSerialNumber,
                             &MaxComponentLength,
                             &VolResult.Capabilities,
                             VolResult.FsName.StartOfString,
                             VolResult.FsName.LengthAllocated)) {

        VolResult.Have.GetVolInfo = TRUE;

        VolResult.VolumeLabel.LengthInChars = _tcslen(VolResult.VolumeLabel.StartOfString);
        VolResult.FsName.LengthInChars = _tcslen(VolResult.FsName.StartOfString);
    }

    //
    //  Get the total and free space using the best available API.
    //

    if (YoriLibGetDiskFreeSpace(VolRootName.StartOfString,
                                NULL,
                                &VolResult.VolumeSize,
                                &VolResult.FreeSpace)) {

        VolResult.Have.FreeSpace = TRUE;
    }

    //
    //  Get the sector size and calculate the cluster size.
    //

    if (GetDiskFreeSpace(VolRootName.StartOfString,
                         &SectorsPerCluster,
                         &VolResult.SectorSize,
                         &NumberOfFreeClusters,
                         &TotalNumberOfClusters)) {

        VolResult.Have.SectorSize = TRUE;
        VolResult.ClusterSize = VolResult.SectorSize * SectorsPerCluster;
    }

    VolResult.PhysicalSectorSize = VolResult.SectorSize;

    //
    //  Truncate the trailing backslash so as to open the volume instead of
    //  root directory
    //

    if (VolRootName.LengthInChars > 0 &&
        VolRootName.StartOfString[VolRootName.LengthInChars - 1] == '\\') {

        VolRootName.LengthInChars--;
        VolRootName.StartOfString[VolRootName.LengthInChars] = '\0';
    }

    //
    //  This needs to be more than FILE_READ_ATTRIBUTES to get a file system
    //  handle, but not require any form of write access or read data access,
    //  or else it needs an administrative caller.
    //

    hDir = CreateFile(VolRootName.StartOfString,
                      FILE_READ_ATTRIBUTES | FILE_TRAVERSE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_OPEN_NO_RECALL,
                      NULL);
    if (hDir != INVALID_HANDLE_VALUE) {
        FILE_STORAGE_INFO StorageInfo;
        USN_JOURNAL_DATA UsnData;
        DWORD BytesReturned;

        if (DllKernel32.pGetFileInformationByHandleEx) {
            if (DllKernel32.pGetFileInformationByHandleEx(hDir, FileStorageInfo, &StorageInfo, sizeof(StorageInfo))) {
                VolResult.Have.PhysicalSectorSize = TRUE;
                VolResult.PhysicalSectorSize = StorageInfo.FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
            }
        }

        //
        //  This needs admin for no good reason
        //

        if (DeviceIoControl(hDir,
                            FSCTL_QUERY_USN_JOURNAL,
                            NULL,
                            0,
                            &UsnData,
                            sizeof(UsnData),
                            &BytesReturned,
                            NULL)) {

            VolResult.Have.Usn = TRUE;
            VolResult.UsnJournalId = UsnData.UsnJournalID;
            VolResult.UsnFirst = UsnData.FirstUsn;
            VolResult.UsnNext = UsnData.NextUsn;
            VolResult.UsnLowestValid = UsnData.LowestValidUsn;
            VolResult.UsnMax = UsnData.MaxUsn;
            VolResult.UsnMaxAllocated = UsnData.MaximumSize;
        }

        if (DeviceIoControl(hDir,
                            FSCTL_GET_NTFS_VOLUME_DATA,
                            NULL,
                            0,
                            &VolResult.NtfsData,
                            sizeof(VolResult.NtfsData),
                            &BytesReturned,
                            NULL)) {

            VolResult.Have.NtfsData = TRUE;

            VolResult.FullSerialNumber = VolResult.NtfsData.SerialNumber.QuadPart;
            VolResult.Have.FullSerial = TRUE;
            VolResult.ReservedSize = VolResult.NtfsData.TotalReserved.QuadPart * VolResult.NtfsData.BytesPerCluster;
            VolResult.Have.Reserved = TRUE;
        }

        if (DeviceIoControl(hDir,
                            FSCTL_GET_REFS_VOLUME_DATA,
                            NULL,
                            0,
                            &VolResult.RefsData,
                            sizeof(VolResult.RefsData),
                            &BytesReturned,
                            NULL)) {

            VolResult.Have.RefsData = TRUE;

            VolResult.FullSerialNumber = VolResult.RefsData.SerialNumber.QuadPart;
            VolResult.Have.FullSerial = TRUE;
            VolResult.ReservedSize = VolResult.RefsData.TotalReserved.QuadPart * VolResult.RefsData.BytesPerCluster;
            VolResult.Have.Reserved = TRUE;
            VolResult.PhysicalSectorSize = VolResult.RefsData.BytesPerPhysicalSector;
            VolResult.Have.PhysicalSectorSize = TRUE;
        }

        CloseHandle(hDir);
    }

    YoriLibInitEmptyString(&DisplayString);

    //
    //  If the user specified a string, use it.  If not, fall back to a
    //  series of defaults depending on the information we have collected.
    //

    if (YsFormatString.StartOfString != NULL) {
        YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VolExpandVariables, &VolResult, &DisplayString);
        if (DisplayString.StartOfString != NULL) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            YoriLibFreeStringContents(&DisplayString);
        }
    } else {

        if (VolResult.Have.GetVolInfo) {

            LPTSTR FormatString = 
                          _T("File system:          $fsname$\n")
                          _T("Label:                $label$\n")
                          _T("Serial number:        $serial$\n");
            YoriLibConstantString(&YsFormatString, FormatString);
            YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VolExpandVariables, &VolResult, &DisplayString);
            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            }
        }

        if (VolResult.Have.FreeSpace) {
            LPTSTR FormatString = 
                          _T("Free space (bytes):   $free$\n")
                          _T("Size (bytes):         $size$\n");
            YoriLibConstantString(&YsFormatString, FormatString);
            YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VolExpandVariables, &VolResult, &DisplayString);
            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            }
        }

        if (VolResult.Have.SectorSize) {
            LPTSTR FormatString = 
                          _T("Cluster size:         $clustersize$\n")
                          _T("Sector size:          $sectorsize$\n");
            YoriLibConstantString(&YsFormatString, FormatString);
            YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VolExpandVariables, &VolResult, &DisplayString);
            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            }
        }

        if (VolResult.Have.PhysicalSectorSize) {
            LPTSTR FormatString = 
                          _T("Physical sector size: $physicalsectorsize$\n");
            YoriLibConstantString(&YsFormatString, FormatString);
            YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VolExpandVariables, &VolResult, &DisplayString);
            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            }
        }

        if (VolResult.Have.Usn) {
            LPTSTR FormatString = 
                          _T("USN journal id:       $usnjournalid$\n")
                          _T("First journalled USN: $usnfirst$\n")
                          _T("Next journalled USN:  $usnnext$\n")
                          _T("Minimum USN value:    $usnlowestvalid$\n")
                          _T("Maximum USN value:    $usnmax$\n")
                          _T("Maximum journal size: $usnmaxallocated$\n");
            YoriLibConstantString(&YsFormatString, FormatString);
            YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VolExpandVariables, &VolResult, &DisplayString);
            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            }
        }

        if (VolResult.Have.FullSerial) {
            LPTSTR FormatString = 
                          _T("Full serial number:   $fullserial$\n");
            YoriLibConstantString(&YsFormatString, FormatString);
            YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VolExpandVariables, &VolResult, &DisplayString);
            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            }
        }

        if (VolResult.Have.Reserved) {
            LPTSTR FormatString = 
                          _T("Reserved bytes:       $reserved$\n");
            YoriLibConstantString(&YsFormatString, FormatString);
            YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VolExpandVariables, &VolResult, &DisplayString);
            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            }
        }

        if (VolResult.Have.NtfsData) {
            LPTSTR FormatString = 
                          _T("File record size:     $filerecordsize$\n")
                          _T("MFT size:             $mftsize$\n");
            YoriLibConstantString(&YsFormatString, FormatString);
            YoriLibExpandCommandVariables(&YsFormatString, '$', FALSE, VolExpandVariables, &VolResult, &DisplayString);
            if (DisplayString.StartOfString != NULL) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y"), &DisplayString);
            }
        }

        YoriLibFreeStringContents(&DisplayString);
    }
    YoriLibFreeStringContents(&VolResult.VolumeLabel);
    YoriLibFreeStringContents(&VolResult.FsName);
    YoriLibFreeStringContents(&FullPathName);
    YoriLibFreeStringContents(&VolRootName);

    if (VolResult.VariableExpansionFailure) {
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}

// vim:sw=4:ts=4:et:
