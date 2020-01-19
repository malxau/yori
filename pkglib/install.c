/**
 * @file pkglib/install.c
 *
 * Yori shell install packages
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
#include "yoripkgp.h"

/**
 Return TRUE if a file can be deleted, or FALSE if it cannot.  This deletion
 check involves opening the file for delete and requesting it to be deleted.
 Note in particular this should fail if an executable is running.  This is
 implemented through APIs that were only documented in Vista+, so on earlier
 releases this function assumes all files are deleteable.

 @param FilePath Pointer to a file to check for deleteability.  This function
        requires the path to be NULL terminated.

 @return TRUE to indicate the file can be deleted, or at least, is not
         currently executing.  FALSE to indicate the file is in use or cannot
         be deleted for some other reason.
 */
BOOL
YoriPkgCheckIfFileDeleteable(
    __in PYORI_STRING FilePath
    )
{
    HANDLE FileHandle;
    FILE_DISPOSITION_INFO DispositionInfo;
    BOOL Result;

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    //
    //  If the OS doesn't have support for this, claim that the file is
    //  deleteable.  This means pre-Vista won't have helpful error messages
    //  when trying to uninstall from within a running shell.
    //

    if (DllKernel32.pSetFileInformationByHandle == NULL) {
        return TRUE;
    }

    //
    //  Open the file.  If we can't open it for delete, it's not deleteable.
    //

    FileHandle = CreateFile(FilePath->StartOfString,
                            DELETE | SYNCHRONIZE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if (FileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    Result = FALSE;
    DispositionInfo.DeleteFile = TRUE;
    if (DllKernel32.pSetFileInformationByHandle(FileHandle,
                                                FileDispositionInfo,
                                                &DispositionInfo,
                                                sizeof(DispositionInfo))) {

        Result = TRUE;

        DispositionInfo.DeleteFile = FALSE;
        DllKernel32.pSetFileInformationByHandle(FileHandle,
                                                FileDispositionInfo,
                                                &DispositionInfo,
                                                sizeof(DispositionInfo));
    }


    CloseHandle(FileHandle);
    return Result;
}

/**
 Check if all of the files in a specified package can be deleted.

 @param PkgIniFile Pointer to the full path to the packages.ini file.

 @param TargetDirectory Pointer to a string specifying the directory
        containing the package.  If NULL, the directory containing the
        application is used.

 @param PackageName Specifies the package name to check for delete.

 @param IgnoreFailureOfCurrentExecutable If TRUE, and the currently
        executing program forms part of the current package, ignore any
        inability to delete caused by the currently executing program being
        in use.  If FALSE, and the currently executing program is the first
        program in the package to delete, abort the delete with an error.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgCheckIfPackageDeleteable(
    __in PYORI_STRING PkgIniFile,
    __in_opt PYORI_STRING TargetDirectory,
    __in PYORI_STRING PackageName,
    __in BOOLEAN IgnoreFailureOfCurrentExecutable
    )
{
    YORI_STRING AppPath;
    YORI_STRING IniValue;
    YORI_STRING FileToDelete;
    PYORI_STRING FileBeingDeleted;
    DWORD FileCount;
    DWORD FileIndex;
    TCHAR FileIndexString[16];
    BOOL DeleteResult;

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        return FALSE;
    }

    if (TargetDirectory == NULL) {
        if (!YoriPkgGetApplicationDirectory(&AppPath)) {
            YoriLibFreeStringContents(&IniValue);
            return FALSE;
        }
    } else {
        if (!YoriLibAllocateString(&AppPath, TargetDirectory->LengthInChars + MAX_PATH)) {
            return FALSE;
        }
        memcpy(AppPath.StartOfString, TargetDirectory->StartOfString, TargetDirectory->LengthInChars * sizeof(TCHAR));
        AppPath.StartOfString[TargetDirectory->LengthInChars] = '\0';
        AppPath.LengthInChars = TargetDirectory->LengthInChars;
    }

    FileCount = GetPrivateProfileInt(PackageName->StartOfString, _T("FileCount"), 0, PkgIniFile->StartOfString);
    if (FileCount == 0) {
        YoriLibFreeStringContents(&AppPath);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    YoriLibInitEmptyString(&FileToDelete);
    if (!YoriLibAllocateString(&FileToDelete, AppPath.LengthInChars + YORIPKG_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&AppPath);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    for (FileIndex = 1; FileIndex <= FileCount; FileIndex++) {
        YoriLibSPrintf(FileIndexString, _T("File%i"), FileIndex);

        IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, FileIndexString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile->StartOfString);
        if (IniValue.LengthInChars > 0) {
            if (!YoriLibIsPathPrefixed(&IniValue)) {
                YoriLibYPrintf(&FileToDelete, _T("%y\\%y"), &AppPath, &IniValue);
                FileBeingDeleted = &FileToDelete;
            } else {
                FileBeingDeleted = &IniValue;
            }
            DeleteResult = YoriPkgCheckIfFileDeleteable(FileBeingDeleted);

            if (!DeleteResult && IgnoreFailureOfCurrentExecutable) {
                YORI_STRING ModuleName;

                if (!YoriPkgGetExecutableFile(&ModuleName)) {
                    return FALSE;
                }

                //
                //  Note this is the same way that the app directory is determined,
                //  so the only chance for a false miscompare is if the application
                //  is executing from a different name (eg. short file name.)
                //

                if (YoriLibCompareStringInsensitive(&ModuleName, FileBeingDeleted) == 0) {
                    DeleteResult = TRUE;
                }
                YoriLibFreeStringContents(&ModuleName);
            }

            //
            //  If any file can't be deleted, the package can't be deleted.
            //

            if (!DeleteResult) {
                YoriLibFreeStringContents(&IniValue);
                YoriLibFreeStringContents(&AppPath);
                YoriLibFreeStringContents(&FileToDelete);
                return FALSE;
            }
        }
    }

    YoriLibFreeStringContents(&IniValue);
    YoriLibFreeStringContents(&AppPath);
    YoriLibFreeStringContents(&FileToDelete);

    return TRUE;
}



/**
 Delete a file that was installed by a package.  If it works, try to delete
 the parent directory.  The file system will fail this if the directory
 still has files in it.  If it succeeds, keep moving through the parents to
 see what can be removed.

 @param FilePath Pointer to the file to remove.

 @return TRUE if one or more objects were successfully scheduled for deletion.
 */
BOOL
YoriPkgDeleteInstalledPackageFile(
    __in PYORI_STRING FilePath
    )
{
    DWORD Index;
    DWORD RetryCount = 0;
    BOOL FileDeleted = FALSE;

    if (FilePath->LengthInChars == 0) {
        return FALSE;
    }

    //
    //  Retry a few times in case the file is transiently in use.
    //

    for (RetryCount = 0; RetryCount < 3; RetryCount++) {
        if (DeleteFile(FilePath->StartOfString)) {
            FileDeleted = TRUE;
            break;
        }
        Sleep(20);
    }

    //
    //  MSFIX: If the file can't be deleted, one option is to defer until
    //  reboot, but this presumably requires privilege to modify the
    //  registry, and we risk creating problems if a future installation
    //  occurs while a previous uninstallation is pending on the next
    //  reboot.  Working with delete-delayed-until-reboot therefore implies
    //  being able to detect and/or fix it on installation.
    //
    //  But since this requires privilege anyway, another option is to see
    //  what other options exist for deleting an in use binary.
    //

    if (!FileDeleted) {
        return FALSE;
    }

    for (Index = FilePath->LengthInChars - 1; Index > 0; Index--) {

        //
        //  If we find a seperator, turn it into a NULL and try to remove the
        //  directory.  If it fails (eg. the directory has files in it), stop.
        //  If it succeeds, put back the seperator and look for the next
        //  parent.
        //

        if (YoriLibIsSep(FilePath->StartOfString[Index])) {
            FilePath->StartOfString[Index] = '\0';
            if (!RemoveDirectory(FilePath->StartOfString)) {
                FilePath->StartOfString[Index] = '\\';
                return TRUE;
            }
            FilePath->StartOfString[Index] = '\\';
        }
    }

    return TRUE;
}

/**
 Delete a specified package from the system.

 @param PkgIniFile Pointer to the full path to the packages.ini file.

 @param TargetDirectory Pointer to a string specifying the directory
        containing the package.  If NULL, the directory containing the
        application is used.

 @param PackageName Specifies the package name to delete.

 @param IgnoreFailureOfCurrentExecutable If TRUE, and the currently
        executing program forms part of the current package, continue to
        delete the package on a best effort basis.  If FALSE, and the
        currently executing program is the first program in the package to
        delete, abort the delete with an error.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgDeletePackageInternal(
    __in PYORI_STRING PkgIniFile,
    __in_opt PYORI_STRING TargetDirectory,
    __in PYORI_STRING PackageName,
    __in BOOLEAN IgnoreFailureOfCurrentExecutable
    )
{
    YORI_STRING AppPath;
    YORI_STRING IniValue;
    YORI_STRING FileToDelete;
    PYORI_STRING FileBeingDeleted;
    DWORD FileCount;
    DWORD FileIndex;
    TCHAR FileIndexString[16];
    BOOL DeleteResult;

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        return FALSE;
    }

    if (TargetDirectory == NULL) {
        if (!YoriPkgGetApplicationDirectory(&AppPath)) {
            YoriLibFreeStringContents(&IniValue);
            return FALSE;
        }
    } else {
        if (!YoriLibAllocateString(&AppPath, TargetDirectory->LengthInChars + MAX_PATH)) {
            return FALSE;
        }
        memcpy(AppPath.StartOfString, TargetDirectory->StartOfString, TargetDirectory->LengthInChars * sizeof(TCHAR));
        AppPath.StartOfString[TargetDirectory->LengthInChars] = '\0';
        AppPath.LengthInChars = TargetDirectory->LengthInChars;
    }

    FileCount = GetPrivateProfileInt(PackageName->StartOfString, _T("FileCount"), 0, PkgIniFile->StartOfString);
    if (FileCount == 0) {
        YoriLibFreeStringContents(&AppPath);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    YoriLibInitEmptyString(&FileToDelete);
    if (!YoriLibAllocateString(&FileToDelete, AppPath.LengthInChars + YORIPKG_MAX_FIELD_LENGTH)) {
        YoriLibFreeStringContents(&AppPath);
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    for (FileIndex = 1; FileIndex <= FileCount; FileIndex++) {
        YoriLibSPrintf(FileIndexString, _T("File%i"), FileIndex);

        IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, FileIndexString, _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile->StartOfString);
        if (IniValue.LengthInChars > 0) {
            if (!YoriLibIsPathPrefixed(&IniValue)) {
                YoriLibYPrintf(&FileToDelete, _T("%y\\%y"), &AppPath, &IniValue);
                FileBeingDeleted = &FileToDelete;
            } else {
                FileBeingDeleted = &IniValue;
            }
            DeleteResult = YoriPkgDeleteInstalledPackageFile(FileBeingDeleted);

            if (!DeleteResult && IgnoreFailureOfCurrentExecutable) {
                YORI_STRING ModuleName;

                if (!YoriPkgGetExecutableFile(&ModuleName)) {
                    return FALSE;
                }

                //
                //  Note this is the same way that the app directory is determined,
                //  so the only chance for a false miscompare is if the application
                //  is executing from a different name (eg. short file name.)
                //

                if (YoriLibCompareStringInsensitive(&ModuleName, FileBeingDeleted) == 0) {
                    DeleteResult = TRUE;
                }
                YoriLibFreeStringContents(&ModuleName);
            }

            //
            //  If delete fails on the first file, don't continue deleting the
            //  package.  If it fails on a later file, the package is already
            //  inconsistent.
            //

            if (!DeleteResult && FileIndex == 1) {
                YoriLibFreeStringContents(&IniValue);
                YoriLibFreeStringContents(&AppPath);
                YoriLibFreeStringContents(&FileToDelete);
                return FALSE;
            }
        }

        WritePrivateProfileString(PackageName->StartOfString, FileIndexString, NULL, PkgIniFile->StartOfString);
    }

    WritePrivateProfileString(PackageName->StartOfString, _T("FileCount"), NULL, PkgIniFile->StartOfString);
    WritePrivateProfileString(PackageName->StartOfString, _T("Architecture"), NULL, PkgIniFile->StartOfString);
    WritePrivateProfileString(PackageName->StartOfString, _T("UpgradePath"), NULL, PkgIniFile->StartOfString);
    WritePrivateProfileString(PackageName->StartOfString, _T("SourcePath"), NULL, PkgIniFile->StartOfString);
    WritePrivateProfileString(PackageName->StartOfString, _T("SymbolPath"), NULL, PkgIniFile->StartOfString);
    WritePrivateProfileString(PackageName->StartOfString, _T("Version"), NULL, PkgIniFile->StartOfString);
    WritePrivateProfileString(_T("Installed"), PackageName->StartOfString, NULL, PkgIniFile->StartOfString);

    WritePrivateProfileString(PackageName->StartOfString, NULL, NULL, PkgIniFile->StartOfString);

    YoriLibFreeStringContents(&IniValue);
    YoriLibFreeStringContents(&AppPath);
    YoriLibFreeStringContents(&FileToDelete);

    return TRUE;
}



/**
 A context structure passed for each file installed as part of a package.
 */
typedef struct _YORIPKG_INSTALL_PKG_CONTEXT {

    /**
     Pointer to the set of operations to perform and assorted state with
     the top level request.
     */
    PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages;

    /**
     Path to the INI file recording package installation.
     */
    PYORI_STRING IniFileName;

    /**
     The name of the package being installed.
     */
    PYORI_STRING PackageName;

    /**
     The number of files installed as part of this package.  THis value is
     incremented each time a file is found.
     */
    DWORD NumberFiles;

    /**
     If TRUE, files extracted from this package should be compressed.
     */
    BOOL CompressFiles;

    /**
     If TRUE, installation is aborted due to a file conflict.
     */
    BOOL ConflictingFileFound;

    /**
     Context for background compression threads.
     */
    YORILIB_COMPRESS_CONTEXT CompressContext;

} YORIPKG_INSTALL_PKG_CONTEXT, *PYORIPKG_INSTALL_PKG_CONTEXT;

/**
 A callback function invoked for each file installed as part of a package.

 @param FullPath The full path name of the file on disk.

 @param RelativePath The relative path name of the file as stored within the
        package.

 @param Context Pointer to the YORIPKG_INSTALL_PKG_CONTEXT structure.

 @return TRUE to continue to apply the file, FALSE to skip the file.
 */
BOOL
YoriPkgInstallPackageFileCallback(
    __in PYORI_STRING FullPath,
    __in PYORI_STRING RelativePath,
    __in PVOID Context
    )
{
    PYORIPKG_INSTALL_PKG_CONTEXT InstallContext = (PYORIPKG_INSTALL_PKG_CONTEXT)Context;
    TCHAR FileIndexString[16];

    if (InstallContext->ConflictingFileFound) {
        return FALSE;
    }

    if (YoriPkgIsFileToBeDeletedOnReboot(FullPath)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("File %y is scheduled to be deleted on next reboot\n"), FullPath);
        InstallContext->ConflictingFileFound = TRUE;
        return FALSE;
    }

    if (YoriPkgCheckIfFileAlreadyExists(InstallContext->PendingPackages, RelativePath)) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Install of package %y conflicts with installed file %y\n"), InstallContext->PackageName, RelativePath);
        InstallContext->ConflictingFileFound = TRUE;
        return FALSE;
    }

    InstallContext->NumberFiles++;
    YoriLibSPrintf(FileIndexString, _T("File%i"), InstallContext->NumberFiles);

    WritePrivateProfileString(InstallContext->PackageName->StartOfString, FileIndexString, RelativePath->StartOfString, InstallContext->IniFileName->StartOfString);
    return TRUE;
}

/**
 After a file has been extracted, this function is invoked to initiate
 compress if that feature is available.

 @param FullPath The full path name of the file on disk.

 @param RelativePath The relative path name of the file as stored within the
        package.

 @param Context Pointer to the YORIPKG_INSTALL_PKG_CONTEXT structure.

 @return TRUE, but this value is ignored since the file is already extracted.
 */
BOOL
YoriPkgCompressPackageFileCallback(
    __in PYORI_STRING FullPath,
    __in PYORI_STRING RelativePath,
    __in PVOID Context
    )
{
    PYORIPKG_INSTALL_PKG_CONTEXT InstallContext = (PYORIPKG_INSTALL_PKG_CONTEXT)Context;

    UNREFERENCED_PARAMETER(RelativePath);

    if (!InstallContext->CompressFiles) {
        return TRUE;
    }

    YoriLibCompressFileInBackground(&InstallContext->CompressContext, FullPath);
    return TRUE;
}


/**
 Install a package into the system.

 @param PendingPackages Pointer to a list of packages to install, and packages
        backed up in preparation for these installations.

 @param Package Information about a package to install.

 @param TargetDirectory Pointer to a string specifying the directory to
        install the package.  If NULL, the directory containing the
        application is used.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriPkgInstallPackage(
    __in PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages,
    __in PYORIPKG_PACKAGE_PENDING_INSTALL Package,
    __in_opt PYORI_STRING TargetDirectory
    )
{
    YORI_STRING PkgInfoFile;
    YORI_STRING PkgIniFile;
    YORI_STRING FullTargetDirectory;

    YORI_STRING ErrorString;
    YORIPKG_INSTALL_PKG_CONTEXT InstallContext;

    BOOL Result = FALSE;
    TCHAR FileIndexString[16];

    ZeroMemory(&InstallContext, sizeof(InstallContext));

    YoriLibConstantString(&PkgInfoFile, _T("pkginfo.ini"));

    YoriLibInitEmptyString(&FullTargetDirectory);
    YoriLibInitEmptyString(&PkgIniFile);

    //
    //  Create path to system packages.ini
    //

    if (!YoriPkgGetPackageIniFile(TargetDirectory, &PkgIniFile)) {
        goto Exit;
    }

    if (TargetDirectory != NULL) {
        if (!YoriLibUserStringToSingleFilePath(TargetDirectory, FALSE, &FullTargetDirectory)) {
            goto Exit;
        }
    } else {
        if (!YoriPkgGetApplicationDirectory(&FullTargetDirectory)) {
            goto Exit;
        }
    }

    {
        YORI_STRING PkgToDelete;

        //
        //  Check if a different version of the package being installed
        //  is already present.  If it is, installation at this point fails.
        //  Typically a higher level process will backup anything that an
        //  installation intends to supersede, so by this point it would
        //  not appear installed.
        //

        if (!YoriLibAllocateString(&PkgToDelete, YORIPKG_MAX_FIELD_LENGTH)) {
            goto Exit;
        }

        PkgToDelete.LengthInChars = GetPrivateProfileString(_T("Installed"), Package->PackageName.StartOfString, _T(""), PkgToDelete.StartOfString, PkgToDelete.LengthAllocated, PkgIniFile.StartOfString);

        //
        //  If the version being installed is already there, we're done.
        //

        if (YoriLibCompareString(&PkgToDelete, &Package->Version) == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y version %y is already installed\n"), &Package->PackageName, &Package->Version);
            YoriLibFreeStringContents(&PkgToDelete);
            Result = TRUE;
            goto Exit;
        } else if (PkgToDelete.LengthInChars > 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y version %y is currently installed, blocking install of %y\n"), &Package->PackageName, &PkgToDelete, &Package->Version);
            YoriLibFreeStringContents(&PkgToDelete);
            Result = FALSE;
            goto Exit;
        }

        YoriLibFreeStringContents(&PkgToDelete);
    }

    //
    //  Before starting, indicate that the package is installed with a
    //  version of zero.  This ensures that if anything goes wrong, an
    //  upgrade will detect a new version and will retry.
    //

    WritePrivateProfileString(_T("Installed"), Package->PackageName.StartOfString, _T("0"), PkgIniFile.StartOfString);
    if (Package->UpgradePath.LengthInChars > 0) {
        WritePrivateProfileString(Package->PackageName.StartOfString, _T("UpgradePath"), Package->UpgradePath.StartOfString, PkgIniFile.StartOfString);
    }

    if (YoriLibGetWofVersionAvailable(&FullTargetDirectory)) {
        YORILIB_COMPRESS_ALGORITHM CompressAlgorithm;
        CompressAlgorithm.EntireAlgorithm = 0;
        CompressAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS8K;
        if (YoriLibInitializeCompressContext(&InstallContext.CompressContext, CompressAlgorithm)) {
            InstallContext.CompressFiles = TRUE;
        } else {
            YoriLibFreeCompressContext(&InstallContext.CompressContext);
        }
    }

    //
    //  Extract the package contents, without pkginfo.ini, to the desired
    //  location
    //

    InstallContext.PendingPackages = PendingPackages;
    InstallContext.IniFileName = &PkgIniFile;
    InstallContext.PackageName = &Package->PackageName;
    InstallContext.NumberFiles = 0;
    InstallContext.ConflictingFileFound = FALSE;
    YoriLibInitEmptyString(&ErrorString);
    if (!YoriLibExtractCab(&Package->LocalPackagePath, &FullTargetDirectory, TRUE, 1, &PkgInfoFile, 0, NULL, YoriPkgInstallPackageFileCallback, YoriPkgCompressPackageFileCallback, &InstallContext, &ErrorString)) {
        WritePrivateProfileString(_T("Installed"), Package->PackageName.StartOfString, NULL, PkgIniFile.StartOfString);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Could not create or write to file %y: %y\n"), &Package->LocalPackagePath, &ErrorString);
        YoriLibFreeStringContents(&ErrorString);
        goto Exit;
    }

    if (InstallContext.ConflictingFileFound) {
        WritePrivateProfileString(_T("Installed"), Package->PackageName.StartOfString, NULL, PkgIniFile.StartOfString);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Install aborted due to file conflict\n"));
        goto Exit;
    }

    WritePrivateProfileString(Package->PackageName.StartOfString, _T("Version"), Package->Version.StartOfString, PkgIniFile.StartOfString);
    WritePrivateProfileString(Package->PackageName.StartOfString, _T("Architecture"), Package->Architecture.StartOfString, PkgIniFile.StartOfString);
    if (Package->UpgradePath.LengthInChars > 0) {
        WritePrivateProfileString(Package->PackageName.StartOfString, _T("UpgradePath"), Package->UpgradePath.StartOfString, PkgIniFile.StartOfString);
    }
    if (Package->SourcePath.LengthInChars > 0) {
        WritePrivateProfileString(Package->PackageName.StartOfString, _T("SourcePath"), Package->SourcePath.StartOfString, PkgIniFile.StartOfString);
    }
    if (Package->SymbolPath.LengthInChars > 0) {
        WritePrivateProfileString(Package->PackageName.StartOfString, _T("SymbolPath"), Package->SymbolPath.StartOfString, PkgIniFile.StartOfString);
    }

    YoriLibSPrintf(FileIndexString, _T("%i"), InstallContext.NumberFiles);

    WritePrivateProfileString(Package->PackageName.StartOfString, _T("FileCount"), FileIndexString, PkgIniFile.StartOfString);
    WritePrivateProfileString(_T("Installed"), Package->PackageName.StartOfString, Package->Version.StartOfString, PkgIniFile.StartOfString);

    Result = TRUE;

Exit:
    YoriLibFreeStringContents(&PkgIniFile);
    YoriLibFreeStringContents(&FullTargetDirectory);
    if (InstallContext.CompressFiles) {
        YoriLibFreeCompressContext(&InstallContext.CompressContext);
    }

    return Result;
}

/**
 Given a package name of an installed package and an existing upgrade path for
 the current architecture, try to munge a path for a new architecture.  This
 routine intentionally leaves "noarch" packages alone, because there's never
 a need to get a different type of noarch package.

 @param PackageName The name of an installed package.

 @param NewArchitecture The new architecture to apply.

 @param PkgIniFile Path to the system global store of installed packages.

 @param UpgradePath On input, refers to a fully qualified upgrade path for
        the package.  On successful completion, this is updated to contain
        a path for the new architecture.  Note this modification is performed
        on the existing allocation; the assumption is the caller needs to
        have a pessimistically sized buffer to read from the INI file, and
        any modifications made need to fit in the same limits anyway.

 @return TRUE to indicate the path was successfully updated to the new
         architecture; FALSE if it was not updated.
 */
BOOL
YoriPkgBuildUpgradeLocationForNewArchitecture(
    __in PYORI_STRING PackageName,
    __in PYORI_STRING NewArchitecture,
    __in PYORI_STRING PkgIniFile,
    __inout PYORI_STRING UpgradePath
    )
{
    YORI_STRING IniValue;
    YORI_STRING ExistingArchAndExtension;

    if (!YoriLibAllocateString(&IniValue, YORIPKG_MAX_FIELD_LENGTH)) {
        return FALSE;
    }

    IniValue.LengthInChars = GetPrivateProfileString(PackageName->StartOfString, _T("Architecture"), _T(""), IniValue.StartOfString, IniValue.LengthAllocated, PkgIniFile->StartOfString);
    if (IniValue.LengthInChars == 0) {
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    if (UpgradePath->LengthInChars < IniValue.LengthInChars + sizeof(".cab") - 1) {
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    YoriLibInitEmptyString(&ExistingArchAndExtension);
    ExistingArchAndExtension.LengthInChars = sizeof(".cab") - 1;
    ExistingArchAndExtension.StartOfString = &UpgradePath->StartOfString[UpgradePath->LengthInChars - ExistingArchAndExtension.LengthInChars];

    if (YoriLibCompareStringWithLiteralInsensitive(&ExistingArchAndExtension, _T(".cab")) != 0) {
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    ExistingArchAndExtension.StartOfString -= IniValue.LengthInChars;
    ExistingArchAndExtension.LengthInChars = IniValue.LengthInChars;

    if (YoriLibCompareStringInsensitive(&ExistingArchAndExtension, &IniValue) != 0) {
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteralInsensitive(&ExistingArchAndExtension, _T("noarch")) == 0) {
        YoriLibFreeStringContents(&IniValue);
        return FALSE;
    }

    if (UpgradePath->LengthInChars - IniValue.LengthInChars + NewArchitecture->LengthInChars < UpgradePath->LengthAllocated) {
        YoriLibSPrintf(ExistingArchAndExtension.StartOfString, _T("%y.cab"), NewArchitecture);
        UpgradePath->LengthInChars = UpgradePath->LengthInChars - IniValue.LengthInChars + NewArchitecture->LengthInChars;

        YoriLibFreeStringContents(&IniValue);
        return TRUE;
    }

    YoriLibFreeStringContents(&IniValue);
    return FALSE;
}


/**
 Install a set of packages.  If all installations succeed, commit the set
 (removing backups) and return TRUE.  If anything fails, roll back all backed
 up packages and return FALSE.  Note this function generates output for the
 user.

 @param PkgIniFile Pointer to the system global package INI file.

 @param TargetDirectory Pointer to a string specifying the directory
        containing the package.  If NULL, the directory containing the
        application is used.

 @param PendingPackages Pointer to a list of packages to install, and packages
        backed up in preparation for these installations.

 @return TRUE to indicate all packages were installed successfully, FALSE
         otherwise.
 */
BOOL
YoriPkgInstallPendingPackages(
    __in PYORI_STRING PkgIniFile,
    __in_opt PYORI_STRING TargetDirectory,
    __in PYORIPKG_PACKAGES_PENDING_INSTALL PendingPackages
    )
{
    PYORIPKG_PACKAGE_PENDING_INSTALL PendingPackage;
    PYORI_LIST_ENTRY ListEntry;
    DWORD TotalCount;
    DWORD CurrentIndex;
    BOOL Result;

    //
    //  Load into memory all files owned by packages that are installed
    //  prior to this installation and are not replaced by it
    //

    YoriPkgAddExistingFilesToPendingPackages(PkgIniFile, PendingPackages);

    //
    //  Count the number of packages to install
    //

    TotalCount = 0;
    ListEntry = NULL;
    ListEntry = YoriLibGetNextListEntry(&PendingPackages->PackageList, ListEntry);
    while (ListEntry != NULL) {
        TotalCount++;
        ListEntry = YoriLibGetNextListEntry(&PendingPackages->PackageList, ListEntry);
    }

    //
    //  Install the list of packages
    //

    ListEntry = NULL;
    Result = TRUE;
    CurrentIndex = 0;
    ListEntry = YoriLibGetNextListEntry(&PendingPackages->PackageList, ListEntry);
    while (ListEntry != NULL) {
        CurrentIndex++;
        PendingPackage = CONTAINING_RECORD(ListEntry, YORIPKG_PACKAGE_PENDING_INSTALL, PackageList);
        ListEntry = YoriLibGetNextListEntry(&PendingPackages->PackageList, ListEntry);
        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Installing %y version %y (%i/%i)...\n"), &PendingPackage->PackageName, &PendingPackage->Version, CurrentIndex, TotalCount);
        if (!YoriPkgInstallPackage(PendingPackages, PendingPackage, TargetDirectory)) {
            Result = FALSE;
            break;
        }
    }

    if (Result) {
        YoriPkgCommitAndFreeBackupPackageList(&PendingPackages->BackupPackages);
    }

    if (!YoriLibIsListEmpty(&PendingPackages->BackupPackages)) {
        YoriPkgRollbackAndFreeBackupPackageList(PkgIniFile, TargetDirectory, &PendingPackages->BackupPackages);
    }

    return Result;
}

// vim:sw=4:ts=4:et:
