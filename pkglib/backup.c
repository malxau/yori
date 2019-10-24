/**
 * @file pkglib/backup.c
 *
 * Yori package manager move existing files to backups and restore from them
 *
 * Copyright (c) 2018-2019 Malcolm J. Smith
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
#include "yoripkg.h"
#include "yoripkgp.h"

/**
 Deallocate a backup package.  Note this routine assumes the package contains
 no files, since those need to be either renamed back to their original names
 in @ref YoriPkgRollbackRenamedFiles or deleted in
 @ref YoriPkgDeleteRenamedFiles.

 @param PackageBackup The backup package to deallocate.
 */
VOID
YoriPkgFreeBackupPackage(
    __in PYORIPKG_BACKUP_PACKAGE PackageBackup
    )
{
    ASSERT(YoriLibIsListEmpty(&PackageBackup->PackageList));
    ASSERT(YoriLibIsListEmpty(&PackageBackup->FileList));

    YoriLibFreeStringContents(&PackageBackup->PackageName);
    YoriLibFreeStringContents(&PackageBackup->Version);
    YoriLibFreeStringContents(&PackageBackup->Architecture);
    YoriLibFreeStringContents(&PackageBackup->UpgradePath);
    YoriLibFreeStringContents(&PackageBackup->SourcePath);
    YoriLibFreeStringContents(&PackageBackup->SymbolPath);

    YoriLibFree(PackageBackup);
}

/**
 Delete all backed up files since the backup package is no longer required.
 Note this routine is best effort and continues on error.

 @param PackageBackup Pointer to the backed up package.
 */
VOID
YoriPkgDeleteRenamedFiles(
    __in PYORIPKG_BACKUP_PACKAGE PackageBackup
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORIPKG_BACKUP_FILE BackupFile;

    ListEntry = YoriLibGetNextListEntry(&PackageBackup->FileList, ListEntry);
    while (ListEntry != NULL) {
        BackupFile = CONTAINING_RECORD(ListEntry, YORIPKG_BACKUP_FILE, ListEntry);
        ASSERT(YoriLibIsStringNullTerminated(&BackupFile->OriginalName));
        ASSERT(YoriLibIsStringNullTerminated(&BackupFile->OriginalRelativeName));

        if (BackupFile->BackupName.LengthInChars > 0) {
            ASSERT(YoriLibIsStringNullTerminated(&BackupFile->BackupName));
            DeleteFile(BackupFile->BackupName.StartOfString);
        }

        ListEntry = YoriLibGetNextListEntry(&PackageBackup->FileList, ListEntry);
        YoriLibRemoveListItem(&BackupFile->ListEntry);

        YoriLibFreeStringContents(&BackupFile->BackupName);
        YoriLibFreeStringContents(&BackupFile->OriginalName);
        YoriLibDereference(BackupFile);
    }
}

/**
 Rename all backed up files back into their original location.  Optionally
 this also restores each file entry back into the INI file.  Note this routine
 is best effort and continues on error.

 @param IniPath Pointer to the system global INI file.

 @param PackageBackup Pointer to the backed up package.

 @param RestoreIni If TRUE, File entries in the INI file are restored to
        match the names of backed up files.  If FALSE, the INI file is not
        touched.
 */
VOID
YoriPkgRollbackRenamedFiles(
    __in PYORI_STRING IniPath,
    __in PYORIPKG_BACKUP_PACKAGE PackageBackup,
    __in BOOL RestoreIni
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORIPKG_BACKUP_FILE BackupFile;
    TCHAR FileIndexString[16];
    DWORD Index;
    BOOL Result;

    ListEntry = YoriLibGetNextListEntry(&PackageBackup->FileList, ListEntry);
    Index = 1;
    while (ListEntry != NULL) {
        BackupFile = CONTAINING_RECORD(ListEntry, YORIPKG_BACKUP_FILE, ListEntry);
        ASSERT(YoriLibIsStringNullTerminated(&BackupFile->OriginalName));
        ASSERT(YoriLibIsStringNullTerminated(&BackupFile->OriginalRelativeName));

        if (RestoreIni) {
            YoriLibSPrintf(FileIndexString, _T("File%i"), Index);
            WritePrivateProfileString(PackageBackup->PackageName.StartOfString, FileIndexString, BackupFile->OriginalRelativeName.StartOfString, IniPath->StartOfString);

        }

        //
        //  There's nothing we can do when this fails other than keep going.
        //  Since we're not done installing, the original names either
        //  shouldn't exist or shouldn't be in use though, so we don't
        //  expect this to generally fail.
        //

        if (BackupFile->BackupName.LengthInChars > 0) {
            ASSERT(YoriLibIsStringNullTerminated(&BackupFile->BackupName));
            Result = MoveFileEx(BackupFile->BackupName.StartOfString, BackupFile->OriginalName.StartOfString, MOVEFILE_REPLACE_EXISTING);
            ASSERT(Result);
        }

        ListEntry = YoriLibGetNextListEntry(&PackageBackup->FileList, ListEntry);
        Index++;
        YoriLibRemoveListItem(&BackupFile->ListEntry);

        YoriLibFreeStringContents(&BackupFile->BackupName);
        YoriLibFreeStringContents(&BackupFile->OriginalName);
        YoriLibDereference(BackupFile);
    }
}

/**
 Rename all backed up files back into their original location, and restore
 package INI entries to indicate the backed up package is once again
 installed.  Note this routine is best effort and continues on error.

 @param IniPath Pointer to the system global INI file.

 @param PackageBackup Pointer to the backed up package.
 */
VOID
YoriPkgRollbackPackage(
    __in PYORI_STRING IniPath,
    __in PYORIPKG_BACKUP_PACKAGE PackageBackup
    )
{
    TCHAR FileCountString[16];

    ASSERT(YoriLibIsStringNullTerminated(IniPath));

    ASSERT(YoriLibIsStringNullTerminated(&PackageBackup->PackageName));
    ASSERT(YoriLibIsStringNullTerminated(&PackageBackup->Version));
    ASSERT(YoriLibIsStringNullTerminated(&PackageBackup->Architecture));

    ASSERT(PackageBackup->PackageName.LengthInChars > 0);
    ASSERT(PackageBackup->Version.LengthInChars > 0);
    ASSERT(PackageBackup->Architecture.LengthInChars > 0);

    //
    //  Delete the entire existing section.  This will clear out any files
    //  added there that aren't part of the backed up package.
    //

    WritePrivateProfileString(PackageBackup->PackageName.StartOfString, NULL, NULL, IniPath->StartOfString);

    //
    //  Put back the files and recreate their INI entries.
    //

    YoriPkgRollbackRenamedFiles(IniPath, PackageBackup, TRUE);
    YoriLibSPrintf(FileCountString, _T("%i"), PackageBackup->FileCount);

    //
    //  Restore all of the fixed headers for the package.
    //

    WritePrivateProfileString(PackageBackup->PackageName.StartOfString, _T("FileCount"), FileCountString, IniPath->StartOfString);
    WritePrivateProfileString(PackageBackup->PackageName.StartOfString, _T("Version"), PackageBackup->Version.StartOfString, IniPath->StartOfString);
    WritePrivateProfileString(PackageBackup->PackageName.StartOfString, _T("Architecture"), PackageBackup->Architecture.StartOfString, IniPath->StartOfString);

    //
    //  Restore any optional headers for the package.
    //

    if (PackageBackup->UpgradePath.LengthInChars > 0) {
        WritePrivateProfileString(PackageBackup->PackageName.StartOfString, _T("UpgradePath"), PackageBackup->UpgradePath.StartOfString, IniPath->StartOfString);
    } else {
        WritePrivateProfileString(PackageBackup->PackageName.StartOfString, _T("UpgradePath"), NULL, IniPath->StartOfString);
    }

    if (PackageBackup->SourcePath.LengthInChars > 0) {
        WritePrivateProfileString(PackageBackup->PackageName.StartOfString, _T("SourcePath"), PackageBackup->SourcePath.StartOfString, IniPath->StartOfString);
    } else {
        WritePrivateProfileString(PackageBackup->PackageName.StartOfString, _T("SourcePath"), NULL, IniPath->StartOfString);
    }

    if (PackageBackup->SymbolPath.LengthInChars > 0) {
        WritePrivateProfileString(PackageBackup->PackageName.StartOfString, _T("SymbolPath"), PackageBackup->SymbolPath.StartOfString, IniPath->StartOfString);
    } else {
        WritePrivateProfileString(PackageBackup->PackageName.StartOfString, _T("SymbolPath"), NULL, IniPath->StartOfString);
    }

    //
    //  Indicate the package is installed.
    //

    WritePrivateProfileString(_T("Installed"), PackageBackup->PackageName.StartOfString, PackageBackup->Version.StartOfString, IniPath->StartOfString);
}

/**
 Backup a currently installed package.  This implies the files currently on
 disk from this package are renamed to backup locations, the INI entries for
 the package are loaded into RAM, and the file names are therefore available
 for reuse by a subsequent package installation.

 @param IniPath Pointer to the system global INI file to back up a package
        from.

 @param PackageName Pointer to the canonical package name to back up.

 @param TargetDirectory Optionally points to a string containing the install
        directory to back up data from.  If not specified, the application
        directory is used.

 @param PackageBackup On successful completion, populated with a backup
        structure indicating the files that have been backed up and all of the
        associated INI entries needed to restore it.

 @return ERROR_SUCCESS to indicate the package has been successfully backed
         up, or a Win32 error code indicating the reason for any failure.
 */
__success(return == ERROR_SUCCESS)
DWORD
YoriPkgBackupPackage(
    __in PYORI_STRING IniPath,
    __in PYORI_STRING PackageName,
    __in_opt PYORI_STRING TargetDirectory,
    __out PYORIPKG_BACKUP_PACKAGE * PackageBackup
    )
{
    PYORIPKG_BACKUP_PACKAGE Context;
    PYORIPKG_BACKUP_FILE BackupFile;
    YORI_STRING FullTargetDirectory;
    YORI_STRING IniValue;
    DWORD FileIndex;
    DWORD Err;
    TCHAR FileIndexString[16];

    Context = YoriLibMalloc(sizeof(YORIPKG_BACKUP_PACKAGE));
    if (Context == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ZeroMemory(Context, sizeof(YORIPKG_BACKUP_PACKAGE));

    YoriLibInitializeListHead(&Context->PackageList);
    YoriLibInitializeListHead(&Context->FileList);

    if (TargetDirectory != NULL) {
        if (!YoriLibUserStringToSingleFilePath(TargetDirectory, FALSE, &FullTargetDirectory)) {
            YoriLibFree(Context);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        if (!YoriPkgGetApplicationDirectory(&FullTargetDirectory)) {
            YoriLibFree(Context);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (!YoriLibAllocateString(&Context->PackageName, PackageName->LengthInChars + 1)) {
        YoriLibFreeStringContents(&FullTargetDirectory);
        YoriLibFree(Context);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memcpy(Context->PackageName.StartOfString, PackageName->StartOfString, PackageName->LengthInChars * sizeof(TCHAR));
    Context->PackageName.LengthInChars = PackageName->LengthInChars;
    Context->PackageName.StartOfString[PackageName->LengthInChars] = '\0';

    if (!YoriPkgGetInstalledPackageInfo(IniPath, &Context->PackageName, &Context->Version, &Context->Architecture, &Context->UpgradePath, &Context->SourcePath, &Context->SymbolPath)) {
        YoriLibFreeStringContents(&FullTargetDirectory);
        YoriPkgFreeBackupPackage(Context);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Context->FileCount = GetPrivateProfileInt(Context->PackageName.StartOfString, _T("FileCount"), 0, IniPath->StartOfString);
    if (Context->FileCount == 0) {
        Err = GetLastError();
        YoriLibFreeStringContents(&FullTargetDirectory);
        YoriPkgFreeBackupPackage(Context);
        return Err;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&FullTargetDirectory);
        YoriPkgFreeBackupPackage(Context);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for (FileIndex = 1; FileIndex <= Context->FileCount; FileIndex++) {
        YoriLibSPrintf(FileIndexString, _T("File%i"), FileIndex);

        BackupFile = YoriLibReferencedMalloc(sizeof(YORIPKG_BACKUP_FILE));
        if (BackupFile == NULL) {
            YoriPkgRollbackRenamedFiles(IniPath, Context, FALSE);
            YoriLibFreeStringContents(&FullTargetDirectory);
            YoriLibFreeStringContents(&IniValue);
            YoriPkgFreeBackupPackage(Context);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        ZeroMemory(BackupFile, sizeof(YORIPKG_BACKUP_FILE));

        IniValue.LengthInChars = GetPrivateProfileString(Context->PackageName.StartOfString, FileIndexString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, IniPath->StartOfString);

        YoriLibYPrintf(&BackupFile->OriginalName, _T("%y\\%y"), &FullTargetDirectory, &IniValue);
        if (BackupFile->OriginalName.LengthInChars == 0) {
            YoriPkgRollbackRenamedFiles(IniPath, Context, FALSE);
            YoriLibFreeStringContents(&FullTargetDirectory);
            YoriLibFreeStringContents(&IniValue);
            YoriLibDereference(BackupFile);
            YoriPkgFreeBackupPackage(Context);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        YoriLibInitEmptyString(&BackupFile->OriginalRelativeName);
        BackupFile->OriginalRelativeName.StartOfString = &BackupFile->OriginalName.StartOfString[FullTargetDirectory.LengthInChars + 1];
        BackupFile->OriginalRelativeName.LengthInChars = BackupFile->OriginalName.LengthInChars - FullTargetDirectory.LengthInChars - 1;
        BackupFile->OriginalRelativeName.LengthAllocated = BackupFile->OriginalName.LengthAllocated - FullTargetDirectory.LengthInChars - 1;

        if (!YoriLibRenameFileToBackupName(&BackupFile->OriginalName, &BackupFile->BackupName)) {
            Err = GetLastError();
            if (Err != ERROR_FILE_NOT_FOUND) {
                YoriPkgRollbackRenamedFiles(IniPath, Context, FALSE);
                YoriLibFreeStringContents(&BackupFile->OriginalName);
                YoriLibFreeStringContents(&FullTargetDirectory);
                YoriLibFreeStringContents(&IniValue);
                YoriLibDereference(BackupFile);
                YoriPkgFreeBackupPackage(Context);
                return Err;
            }
        }

        YoriLibAppendList(&Context->FileList, &BackupFile->ListEntry);

    }
    YoriLibFreeStringContents(&FullTargetDirectory);
    YoriLibFreeStringContents(&IniValue);

    *PackageBackup = Context;
    return ERROR_SUCCESS;
}

/**
 Remove any references to a package from the INI file.  This is used once a
 backup has been generated, so all of these values can be restored.  It ensures
 the INI file is clean in preparation for a subsequent package installation.

 @param IniPath Pointer to the INI file name to remove all entries from.

 @param PackageBackup Pointer to a package backup structure.  All that's
        needed here is the package name, but the structure is used to 
        ensure this function isn't called unless a backup actually exists.
 */
VOID
YoriPkgRemoveSystemReferencesToPackage(
    __in PYORI_STRING IniPath,
    __in PYORIPKG_BACKUP_PACKAGE PackageBackup
    )
{
    WritePrivateProfileString(PackageBackup->PackageName.StartOfString, NULL, NULL, IniPath->StartOfString);
    WritePrivateProfileString(_T("Installed"), PackageBackup->PackageName.StartOfString, NULL, IniPath->StartOfString);
}

/**
 Rollback a set of backed up packages.  This implies that the in memory
 structures can be deallocated, and any backup files on disk should be renamed
 back to their original name, and any INI entries be restored.  There's not
 much we can do if anything goes wrong in this process, so this function
 swallows all errors.

 @param IniPath Pointer to the INI file name to restore entries into.

 @param NewDirectory Optionally points to a string containing the install
        directory to restore data back to.  If not specified, the application
        directory is used.

 @param ListHead Pointer to the list of packages to roll back.
 */
VOID
YoriPkgRollbackAndFreeBackupPackageList(
    __in PYORI_STRING IniPath,
    __in_opt PYORI_STRING NewDirectory,
    __in PYORI_LIST_ENTRY ListHead
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORIPKG_BACKUP_PACKAGE BackupPackage;

    ListEntry = YoriLibGetNextListEntry(ListHead, ListEntry);
    while (ListEntry != NULL) {
        BackupPackage = CONTAINING_RECORD(ListEntry, YORIPKG_BACKUP_PACKAGE, PackageList);
        ListEntry = YoriLibGetNextListEntry(ListHead, ListEntry);
        YoriPkgDeletePackage(NewDirectory, &BackupPackage->PackageName, FALSE);
        YoriPkgRollbackPackage(IniPath, BackupPackage);
        YoriLibRemoveListItem(&BackupPackage->PackageList);
        YoriLibInitializeListHead(&BackupPackage->PackageList);
        YoriPkgFreeBackupPackage(BackupPackage);
    }
}

/**
 Commit a set of backed up packages.  This implies that the in memory
 structures can be deallocated, and any backup files on disk can be deleted.
 Deletion is a best effort process; if anything goes wrong here, files can
 remain.  With any luck they'll be overwritten on a subsequent upgrade.

 @param ListHead Pointer to the list of packages to commit.
 */
VOID
YoriPkgCommitAndFreeBackupPackageList(
    __in PYORI_LIST_ENTRY ListHead
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORIPKG_BACKUP_PACKAGE BackupPackage;

    ListEntry = YoriLibGetNextListEntry(ListHead, ListEntry);
    while (ListEntry != NULL) {
        BackupPackage = CONTAINING_RECORD(ListEntry, YORIPKG_BACKUP_PACKAGE, PackageList);
        ListEntry = YoriLibGetNextListEntry(ListHead, ListEntry);

        YoriPkgDeleteRenamedFiles(BackupPackage);
        YoriLibRemoveListItem(&BackupPackage->PackageList);
        YoriLibInitializeListHead(&BackupPackage->PackageList);
        YoriPkgFreeBackupPackage(BackupPackage);
    }
}

/**
 Add a currently installed file into the installed files hash table.

 @param PendingPackages Pointer to the set of packages to install that is
        being populated with information about currently installed files.

 @param RelativeFileName Pointer to the file to insert into the hash table.
        Note this function will allocate space and copy the file name into it,
        so the buffer is available for the caller to reuse after this function
        returns.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgAddExistingFileToPendingPackages(
    __in PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages,
    __in PYORI_STRING RelativeFileName
    )
{
    PYORIPKG_EXISTING_FILE ExistingFile;

    ExistingFile = YoriLibReferencedMalloc(sizeof(YORIPKG_EXISTING_FILE) + (RelativeFileName->LengthInChars + 1) * sizeof(TCHAR));
    if (ExistingFile == NULL) {
        return FALSE;
    }

    YoriLibInitEmptyString(&ExistingFile->RelativeFileName);
    ExistingFile->RelativeFileName.MemoryToFree = ExistingFile;
    ExistingFile->RelativeFileName.StartOfString = (LPTSTR)(ExistingFile + 1);
    ExistingFile->RelativeFileName.LengthInChars = RelativeFileName->LengthInChars;
    memcpy(ExistingFile->RelativeFileName.StartOfString, RelativeFileName->StartOfString, RelativeFileName->LengthInChars * sizeof(TCHAR));
    ExistingFile->RelativeFileName.StartOfString[ExistingFile->RelativeFileName.LengthInChars] = '\0';
    ExistingFile->RelativeFileName.LengthAllocated = RelativeFileName->LengthInChars + 1;

    YoriLibHashInsertByKey(PendingPackages->ExistingFilesTable, &ExistingFile->RelativeFileName, ExistingFile, &ExistingFile->HashEntry);
    return TRUE;
}

/**
 Free the contents of the currently installed files hash table.

 @param PendingPackages Pointer to the set of packages to install that has
        been populated with currently installed file information.
 */
VOID
YoriPkgFreeExistingFiles(
    __in PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages
    )
{
    DWORD BucketIndex;
    PYORI_HASH_BUCKET Bucket;
    PYORIPKG_EXISTING_FILE ExistingFile;

    for (BucketIndex = 0; BucketIndex < PendingPackages->ExistingFilesTable->NumberBuckets; BucketIndex++) {
        Bucket = &PendingPackages->ExistingFilesTable->Buckets[BucketIndex];
        while (!YoriLibIsListEmpty(&Bucket->ListHead)) {
            ExistingFile = CONTAINING_RECORD(Bucket->ListHead.Next, YORIPKG_EXISTING_FILE, HashEntry.ListEntry);
            YoriLibHashRemoveByEntry(&ExistingFile->HashEntry);
            YoriLibDereference(ExistingFile);
        }
    }
}

/**
 Check if a file that is being installed already exists on the system.

 @param PendingPackages Pointer to a set of packages to install that has been
        populated with currently installed file information.

 @param FileName Pointer to a relative file name to check for.

 @return TRUE if the file is already installed by some package, FALSE if it is
         not.
 */
BOOL
YoriPkgCheckIfFileAlreadyExists(
    __in PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages,
    __in PYORI_STRING FileName
    )
{
    if (YoriLibHashLookupByKey(PendingPackages->ExistingFilesTable, FileName) != NULL) {
        return TRUE;
    }
    return FALSE;
}


/**
 Scan through all installed packages, and within each, all installed files,
 and populate the hash table with files that are installed by any package.
 Note that in the normal installation flow, any package being upgraded or
 replaced has already been moved aside by this point, so the only files that
 appear to be installed are those from packages which will remain after this
 installation operation is complete.

 @param PkgIniFile Pointer to the system global INI file.

 @param PendingPackages Pointer to the packages to install which will be
        populated with currently installed file information.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgAddExistingFilesToPendingPackages(
    __in PYORI_STRING PkgIniFile,
    __in PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages
    )
{
    YORI_STRING InstalledSection;
    LPTSTR ThisLine;
    LPTSTR Equals;
    YORI_STRING PkgNameOnly;
    YORI_STRING IniValue;
    DWORD LineLength;
    DWORD FileCount;
    DWORD FileIndex;
    TCHAR FileIndexString[16];

    if (!YoriLibAllocateString(&InstalledSection, YORIPKG_MAX_SECTION_LENGTH)) {
        return FALSE;
    }

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&InstalledSection);
        return FALSE;
    }

    InstalledSection.LengthInChars = GetPrivateProfileSection(_T("Installed"), InstalledSection.StartOfString, InstalledSection.LengthAllocated, PkgIniFile->StartOfString);

    YoriLibInitEmptyString(&PkgNameOnly);
    ThisLine = InstalledSection.StartOfString;

    while (*ThisLine != '\0') {
        PkgNameOnly.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        LineLength = _tcslen(ThisLine);
        if (Equals != NULL) {
            PkgNameOnly.LengthInChars = (DWORD)(Equals - ThisLine);
        } else {
            PkgNameOnly.LengthInChars = LineLength;
        }
        ThisLine += LineLength;
        ThisLine++;
        PkgNameOnly.StartOfString[PkgNameOnly.LengthInChars] = '\0';

        FileCount = GetPrivateProfileInt(PkgNameOnly.StartOfString, _T("FileCount"), 0, PkgIniFile->StartOfString);

        for (FileIndex = 1; FileIndex <= FileCount; FileIndex++) {
            YoriLibSPrintf(FileIndexString, _T("File%i"), FileIndex);

            IniValue.LengthInChars = GetPrivateProfileString(PkgNameOnly.StartOfString, FileIndexString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile->StartOfString);
            if (!YoriPkgAddExistingFileToPendingPackages(PendingPackages, &IniValue)) {
                YoriLibFreeStringContents(&InstalledSection);
                YoriLibFreeStringContents(&IniValue);
                return FALSE;
            }
        }
    }

    YoriLibFreeStringContents(&InstalledSection);
    YoriLibFreeStringContents(&IniValue);

    return TRUE;
}


/**
 Initialize a list of pending packages, including the list of packages to
 install and the list of packages that have been packed up.

 @param PendingPackages Pointer to the pending package list to initialize.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgInitializePendingPackages(
    __out PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages
    )
{
    YoriLibInitializeListHead(&PendingPackages->PackageList);
    YoriLibInitializeListHead(&PendingPackages->BackupPackages);
    YoriLibInitializeListHead(&PendingPackages->KnownPackages);
    PendingPackages->ExistingFilesTable = YoriLibAllocateHashTable(253);
    if (PendingPackages->ExistingFilesTable == NULL) {
        return FALSE;
    }
    return TRUE;
}

/**
 Delete a single packages pending install, in the sense of deallocating
 memory.  The structure itself is also deallocated.  This function assumes
 the caller has already committed or rolled back any backed up packages
 created as part of this installation set.

 @param PendingPackage The package to deallocate.
 */
VOID
YoriPkgDeletePendingPackage(
    __in PYORIPKG_PACKAGE_PENDING_INSTALL PendingPackage
    )
{
    if (PendingPackage->DeleteLocalPackagePath) {
        DeleteFile(PendingPackage->LocalPackagePath.StartOfString);
    }

    YoriLibFreeStringContents(&PendingPackage->LocalPackagePath);
    YoriLibFreeStringContents(&PendingPackage->PackageName);
    YoriLibFreeStringContents(&PendingPackage->Version);
    YoriLibFreeStringContents(&PendingPackage->Architecture);
    YoriLibFreeStringContents(&PendingPackage->MinimumOSBuild);
    YoriLibFreeStringContents(&PendingPackage->PackagePathForOlderBuilds);
    YoriLibFreeStringContents(&PendingPackage->UpgradePath);
    YoriLibFreeStringContents(&PendingPackage->SourcePath);
    YoriLibFreeStringContents(&PendingPackage->SymbolPath);
    YoriLibFree(PendingPackage);
}

/**
 Delete a list of packages pending install, in the sense of deallocating
 memory.  The structure itself is typically a stack allocation and is not
 deallocated.  This function assumes the caller has already committed or
 rolled back any backed up packages created as part of this installation
 set.

 @param PendingPackages The list of pending packages to deallocate.
 */
VOID
YoriPkgDeletePendingPackages(
    __in PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages
    )
{
    PYORI_LIST_ENTRY ListEntry = NULL;
    PYORIPKG_PACKAGE_PENDING_INSTALL PendingPackage;

    //
    //  Higher level logic needs to decide whether to commit or roll back
    //  backup packages first
    //

    ASSERT(YoriLibIsListEmpty(&PendingPackages->BackupPackages));

    YoriPkgFreeAllSourcesAndPackages(NULL, &PendingPackages->KnownPackages);

    ListEntry = YoriLibGetNextListEntry(&PendingPackages->PackageList, ListEntry);
    while (ListEntry != NULL) {
        PendingPackage = CONTAINING_RECORD(ListEntry, YORIPKG_PACKAGE_PENDING_INSTALL, PackageList);
        ListEntry = YoriLibGetNextListEntry(&PendingPackages->PackageList, ListEntry);

        YoriLibRemoveListItem(&PendingPackage->PackageList);
        YoriPkgDeletePendingPackage(PendingPackage);
    }

    if (PendingPackages->ExistingFilesTable != NULL) {
        YoriPkgFreeExistingFiles(PendingPackages);
        YoriLibFreeEmptyHashTable(PendingPackages->ExistingFilesTable);
        PendingPackages->ExistingFilesTable = NULL;
    }
}

/**
 Given a package URL, download if necessary, extract metadata, check if an
 existing package needs to be upgraded or replaced, back up any packages that
 should be upgraded or replaced, and add the package metadata to a list of
 packages awaiting installation.  After this function has been called, the
 caller is responsible for calling either
 @ref YoriPkgCommitAndFreeBackupPackageList or 
 @ref YoriPkgRollbackAndFreeBackupPackageList followed by
 @ref YoriPkgDeletePendingPackages to ensure any backed up files are either
 deleted or restored, and the memory allocated by this routine is freed.

 @param PkgIniFile Pointer to the system global INI file.

 @param TargetDirectory Optionally points to a string containing the install
        directory to back up data from.  If not specified, the application
        directory is used.

 @param PackageList Pointer to an initialized list of packages to add this
        package to.  This list should be initialized with
        @ref YoriPkgInitializePendingPackages.

 @param PackageUrl Pointer to a source for the package.  This can be a remote
        URL or a local file.

 @param RedirectToPackageUrl On completion may be updated to contain a
        referenced string to a version of the package which should be
        attempted on this system because the PackageUrl requires a newer
        host operating system.  This is only meaningful if
        ERROR_OLD_WIN_VERSION is returned.

 @return A Win32 error code, including ERROR_SUCCESS to indicate success.
 */
__success(return == ERROR_SUCCESS)
DWORD
YoriPkgPreparePackageForInstall(
    __in PYORI_STRING PkgIniFile,
    __in_opt PYORI_STRING TargetDirectory,
    __inout PYORIPKG_PACKAGES_PENDING_INSTALL PackageList,
    __in PYORI_STRING PackageUrl,
    __out_opt PYORI_STRING RedirectToPackageUrl
    )
{
    PYORIPKG_PACKAGE_PENDING_INSTALL PendingPackage;
    YORI_STRING PkgInfoFile;
    YORI_STRING TempPath;
    YORI_STRING ErrorString;
    YORI_STRING ReplacesList;
    YORI_STRING PkgInstalled;
    YORI_STRING PkgToReplace;
    DWORD LineLength;
    LONGLONG RequiredBuildNumber;
    DWORD CharsConsumed;
    LPTSTR ThisLine;
    LPTSTR Equals;
    PYORIPKG_BACKUP_PACKAGE BackupPackage;
    DWORD Result = ERROR_SUCCESS;

    YoriLibConstantString(&PkgInfoFile, _T("pkginfo.ini"));
    if (RedirectToPackageUrl != NULL) {
        YoriLibInitEmptyString(RedirectToPackageUrl);
    }

    PendingPackage = YoriLibMalloc(sizeof(YORIPKG_PACKAGE_PENDING_INSTALL));
    if (PendingPackage == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory(PendingPackage, sizeof(YORIPKG_PACKAGE_PENDING_INSTALL));
    Result = YoriPkgPackagePathToLocalPath(PackageUrl, PkgIniFile, &PendingPackage->LocalPackagePath, &PendingPackage->DeleteLocalPackagePath);
    if (Result != ERROR_SUCCESS) {
        YoriLibFree(PendingPackage);
        return Result;
    }

    YoriLibInitEmptyString(&TempPath);

    //
    //  Query for a temporary directory
    //

    TempPath.LengthAllocated = GetTempPath(0, NULL);
    if (!YoriLibAllocateString(&TempPath, TempPath.LengthAllocated + PkgInfoFile.LengthInChars + 1)) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }
    TempPath.LengthInChars = GetTempPath(TempPath.LengthAllocated, TempPath.StartOfString);

    //
    //  Extract pkginfo.ini to the temporary directory
    //

    YoriLibInitEmptyString(&ErrorString);
    if (!YoriLibExtractCab(&PendingPackage->LocalPackagePath, &TempPath, FALSE, 0, NULL, 1, &PkgInfoFile, NULL, NULL, NULL, &ErrorString)) {
        YoriLibFreeStringContents(&ErrorString);
        Result = ERROR_WRITE_FAULT;
        goto Exit;
    }

    //
    //  Query fields of interest from pkginfo.ini
    //

    YoriLibSPrintf(&TempPath.StartOfString[TempPath.LengthInChars], _T("%y"), &PkgInfoFile);
    TempPath.LengthInChars += PkgInfoFile.LengthInChars;

    if (!YoriPkgGetPackageInfo(&TempPath,
                               &PendingPackage->PackageName,
                               &PendingPackage->Version,
                               &PendingPackage->Architecture,
                               &PendingPackage->MinimumOSBuild,
                               &PendingPackage->PackagePathForOlderBuilds,
                               &PendingPackage->UpgradePath,
                               &PendingPackage->SourcePath,
                               &PendingPackage->SymbolPath)) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    //
    //  Check if a different version of the package being installed
    //  is already present.  If it is, we need to delete it.
    //

    if (!YoriLibAllocateString(&PkgInstalled, YORIPKG_MAX_FIELD_LENGTH)) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    PkgInstalled.LengthInChars = GetPrivateProfileString(_T("Installed"), PendingPackage->PackageName.StartOfString, _T(""), PkgInstalled.StartOfString, PkgInstalled.LengthAllocated, PkgIniFile->StartOfString);

    //
    //  If the version being installed is already there, we're done.
    //

    if (YoriLibCompareString(&PkgInstalled, &PendingPackage->Version) == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y version %y is already installed\n"), &PendingPackage->PackageName, &PendingPackage->Version);
        YoriLibFreeStringContents(&PkgInstalled);
        Result = ERROR_SUCCESS;
        goto Exit;
    }

    //
    //  Check if the new version can run on this host by build number.  This
    //  field may be empty, so ignore failures.
    //

    RequiredBuildNumber = 0;
    YoriLibStringToNumber(&PendingPackage->MinimumOSBuild, FALSE, &RequiredBuildNumber, &CharsConsumed);
    if (RequiredBuildNumber != 0) {
        DWORD OsMajor;
        DWORD OsMinor;
        DWORD OsBuild;

        YoriLibGetOsVersion(&OsMajor, &OsMinor, &OsBuild);
        if (RequiredBuildNumber > OsBuild) {
            if (RedirectToPackageUrl != NULL &&
                PendingPackage->PackagePathForOlderBuilds.LengthInChars > 0) {

                YoriLibCloneString(RedirectToPackageUrl, &PendingPackage->PackagePathForOlderBuilds);
            }
            Result = ERROR_OLD_WIN_VERSION;
            YoriLibFreeStringContents(&PkgInstalled);
            goto Exit;
        }
    }

    //
    //  Backup the current version
    //

    if (PkgInstalled.LengthInChars > 0) {
        Result = YoriPkgBackupPackage(PkgIniFile, &PendingPackage->PackageName, TargetDirectory, &BackupPackage);
        if (Result != ERROR_SUCCESS) {
            YoriLibFreeStringContents(&PkgInstalled);
            goto Exit;
        }
        YoriPkgRemoveSystemReferencesToPackage(PkgIniFile, BackupPackage);
        YoriLibAppendList(&PackageList->BackupPackages, &BackupPackage->PackageList);
    }

    //
    //  Walk through any packages that this package replaces and backup
    //  them
    //

    if (!YoriLibAllocateString(&ReplacesList, YORIPKG_MAX_SECTION_LENGTH)) {
        YoriLibFreeStringContents(&PkgInstalled);
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    YoriLibInitEmptyString(&PkgToReplace);
    ReplacesList.LengthInChars = GetPrivateProfileSection(_T("Replaces"), ReplacesList.StartOfString, ReplacesList.LengthAllocated, TempPath.StartOfString);
    ThisLine = ReplacesList.StartOfString;

    while (*ThisLine != '\0') {
        LineLength = _tcslen(ThisLine);
        PkgToReplace.StartOfString = ThisLine;
        Equals = _tcschr(ThisLine, '=');
        if (Equals != NULL) {
            PkgToReplace.LengthInChars = (DWORD)(Equals - ThisLine);
        } else {
            PkgToReplace.LengthInChars = LineLength;
        }
        PkgToReplace.StartOfString[PkgToReplace.LengthInChars] = '\0';

        //
        //  Check if the package that the new package wants to replace
        //  is installed, and if so, back it up too
        //

        PkgInstalled.LengthInChars = GetPrivateProfileString(_T("Installed"), PkgToReplace.StartOfString, _T(""), PkgInstalled.StartOfString, PkgInstalled.LengthAllocated, PkgIniFile->StartOfString);
        if (PkgInstalled.LengthInChars > 0) {
            Result = YoriPkgBackupPackage(PkgIniFile, &PkgToReplace, TargetDirectory, &BackupPackage);
            if (Result != ERROR_SUCCESS) {
                YoriLibFreeStringContents(&ReplacesList);
                goto Exit;
            }
            YoriPkgRemoveSystemReferencesToPackage(PkgIniFile, BackupPackage);
            YoriLibAppendList(&PackageList->BackupPackages, &BackupPackage->PackageList);
        }
        ThisLine += LineLength + 1;
    }
    YoriLibFreeStringContents(&ReplacesList);
    YoriLibFreeStringContents(&PkgInstalled);

    DeleteFile(TempPath.StartOfString);
    YoriLibFreeStringContents(&TempPath);

    YoriLibAppendList(&PackageList->PackageList, &PendingPackage->PackageList);
    return ERROR_SUCCESS;

Exit:

    if (TempPath.LengthInChars > 0) {
        DeleteFile(TempPath.StartOfString);
    }
    YoriLibFreeStringContents(&TempPath);
    YoriPkgDeletePendingPackage(PendingPackage);
    return Result;
}

/**
 Given a package URL, download if necessary, extract metadata, check if an
 alternate version of the package should be used instead, check if an existing
 package needs to be upgraded or replaced, back up any packages that should be
 upgraded or replaced, and add the package metadata to a list of packages
 awaiting installation.  After this function has been called, the caller is
 responsible for calling either @ref YoriPkgCommitAndFreeBackupPackageList or 
 @ref YoriPkgRollbackAndFreeBackupPackageList followed by
 @ref YoriPkgDeletePendingPackages to ensure any backed up files are either
 deleted or restored, and the memory allocated by this routine is freed.

 @param PkgIniFile Pointer to the system global INI file.

 @param TargetDirectory Optionally points to a string containing the install
        directory to back up data from.  If not specified, the application
        directory is used.

 @param PackageList Pointer to an initialized list of packages to add this
        package to.  This list should be initialized with
        @ref YoriPkgInitializePendingPackages.

 @param PackageUrl Pointer to a source for the package.  This can be a remote
        URL or a local file.  This may not be the URL that is actually used to
        install if the URL is not applicable for this version of the OS.

 @return A Win32 error code, including ERROR_SUCCESS to indicate success.
 */
DWORD
YoriPkgPreparePackageForInstallRedirectBuild(
    __in PYORI_STRING PkgIniFile,
    __in_opt PYORI_STRING TargetDirectory,
    __inout PYORIPKG_PACKAGES_PENDING_INSTALL PackageList,
    __in PYORI_STRING PackageUrl
    )
{
    DWORD Error;
    YORI_STRING RedirectedUrl;
    YORI_STRING PreviousRedirectedUrl;
    PYORI_STRING UrlToInstall;

    Error = ERROR_SUCCESS;
    YoriLibInitEmptyString(&RedirectedUrl);
    YoriLibInitEmptyString(&PreviousRedirectedUrl);
    UrlToInstall = PackageUrl;
    do {
        if (YoriLibIsPathUrl(UrlToInstall)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Downloading %y...\n"), UrlToInstall);
        }
        YoriLibInitEmptyString(&RedirectedUrl);
        Error = YoriPkgPreparePackageForInstall(PkgIniFile, TargetDirectory, PackageList, UrlToInstall, &RedirectedUrl);
        YoriLibFreeStringContents(&PreviousRedirectedUrl);
        YoriLibInitEmptyString(&PreviousRedirectedUrl);

        if (Error == ERROR_OLD_WIN_VERSION) {
            memcpy(&PreviousRedirectedUrl, &RedirectedUrl, sizeof(YORI_STRING));
            UrlToInstall = &PreviousRedirectedUrl;
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Version not supported on this version of Windows, attempting %y...\n"), UrlToInstall);
            continue;
        }
        if (Error != ERROR_SUCCESS && Error != ERROR_OLD_WIN_VERSION) {
            ASSERT(RedirectedUrl.MemoryToFree == NULL);
            break;
        }
    } while (Error == ERROR_OLD_WIN_VERSION);

    return Error;
}


// vim:sw=4:ts=4:et:
