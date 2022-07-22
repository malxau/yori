/**
 * @file mount/mount.c
 *
 * Yori shell mount or unmount ISO disks
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
CHAR strMountHelpText[] =
        "\n"
        "Mount or unmount an ISO disk.\n"
        "\n"
        "MOUNT [-license] [-r] [-i <file>|-u <file>|-v <file>]\n"
        "\n"
        "   -i <file>      Mount an ISO disk\n"
        "   -r             Mount read only\n"
        "   -u <file>      Unmount a disk\n"
        "   -v <file>      Mount a VHD disk\n";

/**
 Display usage text to the user.
 */
BOOL
MountHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Mount %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strMountHelpText);
    return TRUE;
}

/**
 Mount an ISO file as a locally attached storage device.

 @param FileName Pointer to the file name of the ISO file.  This routine will
        resolve it into a full path.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MountMountIso(
    __in PYORI_STRING FileName
    )
{
    VIRTUAL_STORAGE_TYPE StorageType;
    OPEN_VIRTUAL_DISK_PARAMETERS OpenParams;
    ATTACH_VIRTUAL_DISK_PARAMETERS AttachParams;
    HANDLE StorHandle;
    DWORD Err;
    DWORD Length;
    LPTSTR ErrText;
    YORI_STRING FullFileName;
    YORI_STRING DiskPhysicalPath;

    YoriLibLoadVirtDiskFunctions();

    if (DllVirtDisk.pAttachVirtualDisk == NULL ||
        DllVirtDisk.pGetVirtualDiskPhysicalPath == NULL ||
        DllVirtDisk.pOpenVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: OS support not present\n"));
        return FALSE;
    }

    YoriLibInitEmptyString(&FullFileName);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullFileName)) {
        return FALSE;
    }

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_ISO;
    StorageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;

    ZeroMemory(&OpenParams, sizeof(OpenParams));
    OpenParams.Version = OPEN_VIRTUAL_DISK_VERSION_1;

    Err = DllVirtDisk.pOpenVirtualDisk(&StorageType, FullFileName.StartOfString, VIRTUAL_DISK_ACCESS_READ, OPEN_VIRTUAL_DISK_FLAG_NONE, &OpenParams, &StorHandle);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: open of %y failed: %s"), &FullFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    ZeroMemory(&AttachParams, sizeof(AttachParams));
    AttachParams.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

    Err = DllVirtDisk.pAttachVirtualDisk(StorHandle, NULL, ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY | ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME, 0, &AttachParams, NULL);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: attach of %y failed: %s"), &FullFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(StorHandle);
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullFileName);

    if (!YoriLibAllocateString(&DiskPhysicalPath, 32768)) {
        CloseHandle(StorHandle);
        return FALSE;
    }

    Length = DiskPhysicalPath.LengthAllocated;
    Err = DllVirtDisk.pGetVirtualDiskPhysicalPath(StorHandle, &Length, DiskPhysicalPath.StartOfString);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: query physical disk name failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(StorHandle);
        YoriLibFreeStringContents(&DiskPhysicalPath);
        return FALSE;
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Disk mounted as %s\n"), DiskPhysicalPath.StartOfString);
    YoriLibFreeStringContents(&DiskPhysicalPath);

    CloseHandle(StorHandle);
    return TRUE;
}

/**
 Mount a VHD file as a locally attached storage device.

 @param FileName Pointer to the file name of the VHD file.  This routine will
        resolve it into a full path.

 @param ReadOnly TRUE if the device should be read only, FALSE if it should be
        read and write.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MountMountVhd(
    __in PYORI_STRING FileName,
    __in BOOL ReadOnly
    )
{
    VIRTUAL_STORAGE_TYPE StorageType;
    OPEN_VIRTUAL_DISK_PARAMETERS OpenParams;
    ATTACH_VIRTUAL_DISK_PARAMETERS AttachParams;
    HANDLE StorHandle;
    DWORD Err;
    DWORD Length;
    DWORD AccessRequested;
    YORI_STRING FullFileName;
    YORI_STRING DiskPhysicalPath;
    LPTSTR ErrText;

    YoriLibLoadVirtDiskFunctions();
    YoriLibLoadAdvApi32Functions();

    if (DllVirtDisk.pAttachVirtualDisk == NULL ||
        DllVirtDisk.pGetVirtualDiskPhysicalPath == NULL ||
        DllVirtDisk.pOpenVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: OS support not present\n"));
        return FALSE;
    }

    if (!YoriLibEnableManageVolumePrivilege()) {
        return FALSE;
    }

    YoriLibInitEmptyString(&FullFileName);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullFileName)) {
        return FALSE;
    }

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    StorageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    ZeroMemory(&OpenParams, sizeof(OpenParams));
    OpenParams.Version = OPEN_VIRTUAL_DISK_VERSION_1;
    OpenParams.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;

    if (ReadOnly) {
        AccessRequested = VIRTUAL_DISK_ACCESS_READ;
    } else {
        AccessRequested = VIRTUAL_DISK_ACCESS_ATTACH_RW | VIRTUAL_DISK_ACCESS_GET_INFO | VIRTUAL_DISK_ACCESS_DETACH;
    }

    Err = DllVirtDisk.pOpenVirtualDisk(&StorageType, FullFileName.StartOfString, AccessRequested, OPEN_VIRTUAL_DISK_FLAG_NONE, &OpenParams, &StorHandle);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: open of %y failed: %s"), &FullFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    ZeroMemory(&AttachParams, sizeof(AttachParams));
    AttachParams.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

    AccessRequested = ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME;
    if (ReadOnly) {
        AccessRequested |= ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY;
    }

    Err = DllVirtDisk.pAttachVirtualDisk(StorHandle, NULL, AccessRequested, 0, &AttachParams, NULL);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: attach of %y failed: %s"), &FullFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(StorHandle);
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullFileName);

    if (!YoriLibAllocateString(&DiskPhysicalPath, 32768)) {
        CloseHandle(StorHandle);
        return FALSE;
    }

    Length = DiskPhysicalPath.LengthAllocated;
    Err = DllVirtDisk.pGetVirtualDiskPhysicalPath(StorHandle, &Length, DiskPhysicalPath.StartOfString);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: query physical disk name failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(StorHandle);
        YoriLibFreeStringContents(&DiskPhysicalPath);
        return FALSE;
    }

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Disk mounted as %s\n"), DiskPhysicalPath.StartOfString);
    YoriLibFreeStringContents(&DiskPhysicalPath);

    CloseHandle(StorHandle);
    return TRUE;
}

/**
 Unmount a previously mounted file being used as a locally attached storage
 device.

 @param FileName Pointer to the file name of a device to unmount.  This
        routine will resolve it into a full path.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
MountUnmount(
    __in PYORI_STRING FileName
    )
{
    VIRTUAL_STORAGE_TYPE StorageType;
    OPEN_VIRTUAL_DISK_PARAMETERS OpenParams;
    HANDLE StorHandle;
    DWORD Err;
    YORI_STRING FullFileName;

    YoriLibLoadVirtDiskFunctions();

    if (DllVirtDisk.pDetachVirtualDisk == NULL ||
        DllVirtDisk.pOpenVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: OS support not present\n"));
        return FALSE;
    }

    YoriLibInitEmptyString(&FullFileName);
    if (!YoriLibUserStringToSingleFilePath(FileName, TRUE, &FullFileName)) {
        return FALSE;
    }

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    StorageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    ZeroMemory(&OpenParams, sizeof(OpenParams));
    OpenParams.Version = OPEN_VIRTUAL_DISK_VERSION_1;

    Err = DllVirtDisk.pOpenVirtualDisk(&StorageType, FullFileName.StartOfString, VIRTUAL_DISK_ACCESS_DETACH, OPEN_VIRTUAL_DISK_FLAG_NONE, &OpenParams, &StorHandle);
    if (Err != ERROR_SUCCESS) {
        LPTSTR ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: open of %y failed: %s"), &FullFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    Err = DllVirtDisk.pDetachVirtualDisk(StorHandle, 0, 0);
    if (Err != ERROR_SUCCESS) {
        LPTSTR ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: detach of %y failed: %s"), &FullFileName, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        CloseHandle(StorHandle);
        YoriLibFreeStringContents(&FullFileName);
        return FALSE;
    }

    CloseHandle(StorHandle);
    YoriLibFreeStringContents(&FullFileName);
    return TRUE;
}

/**
 A set of operations supported by this application.
 */
typedef enum _MOUNT_OP {
    MountOpNone = 0,
    MountOpMountIso = 1,
    MountOpUnmount = 2,
    MountOpMountVhd = 3
} MOUNT_OP;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the mount builtin command.
 */
#define ENTRYPOINT YoriCmd_YMOUNT
#else
/**
 The main entrypoint for the mount standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the mount cmdlet.

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
    YORI_STRING Arg;
    PYORI_STRING FileName = NULL;
    MOUNT_OP Op;
    BOOL ReadOnly = FALSE;

    Op = MountOpNone;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                MountHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                if (ArgC > i + 1) {
                    FileName = &ArgV[i + 1];
                    Op = MountOpMountIso;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                ReadOnly = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("u")) == 0) {
                if (ArgC > i + 1) {
                    FileName = &ArgV[i + 1];
                    Op = MountOpUnmount;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("v")) == 0) {
                if (ArgC > i + 1) {
                    FileName = &ArgV[i + 1];
                    Op = MountOpMountVhd;
                    ArgumentUnderstood = TRUE;
                    i++;
                }
            }
        } else {
            if (Op == MountOpNone) {
                LPTSTR Period;
                Period = YoriLibFindRightMostCharacter(&ArgV[i], '.');
                if (Period != NULL) {
                    YORI_STRING Ext;
                    YoriLibInitEmptyString(&Ext);
                    Ext.StartOfString = Period + 1;
                    Ext.LengthInChars = ArgV[i].LengthInChars - (DWORD)(Period - ArgV[i].StartOfString) - 1;
                    if (YoriLibCompareStringWithLiteralInsensitive(&Ext, _T("iso")) == 0) {
                        FileName = &ArgV[i];
                        Op = MountOpMountIso;
                        ArgumentUnderstood = TRUE;
                    } else if (YoriLibCompareStringWithLiteralInsensitive(&Ext, _T("vhd")) == 0 ||
                               YoriLibCompareStringWithLiteralInsensitive(&Ext, _T("vhdx")) == 0) {
                        FileName = &ArgV[i];
                        Op = MountOpMountVhd;
                        ArgumentUnderstood = TRUE;
                    }
                }
            }
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (Op == MountOpNone) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("mount: operation not specified\n"));
        return EXIT_FAILURE;
    }

    //
    //  This is a hack in case the user specifies -u -? or similar.
    //

    if (FileName != NULL &&
        YoriLibIsCommandLineOption(FileName, &Arg)) {

        if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
            MountHelp();
            return EXIT_SUCCESS;
        }
    }

    switch(Op) {
        case MountOpMountIso:
            MountMountIso(FileName);
            break;
        case MountOpUnmount:
            MountUnmount(FileName);
            break;
        case MountOpMountVhd:
            MountMountVhd(FileName, ReadOnly);
            break;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
