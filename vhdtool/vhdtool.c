/**
 * @file vhdtool/vhdtool.c
 *
 * Yori shell vhdtool for managing VHD files
 *
 * Copyright (c) 2019 Malcolm J. Smith
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
CHAR strVhdToolHelpText[] =
        "\n"
        "Manage VHD files.\n"
        "\n"
        "VHDTOOL [-license]\n"
        "VHDTOOL [-sector:512|-sector:512e|-sector:4096] -clonedynamic <file> <source>\n"
        "VHDTOOL [-sector:512|-sector:512e|-sector:4096] -clonefixed <file> <source>\n"
        "VHDTOOL -compact <file>\n"
        "VHDTOOL -creatediff <file> <parent>\n"
        "VHDTOOL [-sector:512|-sector:512e|-sector:4096] -createdynamic <file> <size>\n"
        "VHDTOOL [-sector:512|-sector:512e|-sector:4096] -createfixed <file> <size>\n"
        "VHDTOOL -expand <file> <size>\n"
        "VHDTOOL -merge <file>\n"
        "VHDTOOL -shrink <file> <size>\n"
        "\n"
        "   -clonedynamic  Copy an existing disk or VHD into a dynamically expanding\n"
        "                  .vhd or .vhdx file\n"
        "   -clonefixed    Copy an existing disk or VHD into a fixed sized .iso, .vhd\n"
        "                  or .vhdx file\n"
        "   -compact       Remove unused regions from a dynamically expanding .vhd or\n"
        "                  .vhdx file\n"
        "   -creatediff    Create a differencing .vhd or .vhdx file from a read-only\n"
        "                  parent .vhd or .vhdx file\n"
        "   -createdynamic Create a new dynamically expanding .vhd or .vhdx file.  Size\n"
        "                  can be specified with a 'k', 'm', 'g', or 't' suffix.\n"
        "   -createfixed   Create a new fixed size .vhd or .vhdx file.  Size can be\n"
        "                  specified with a 'k', 'm', 'g', or 't' suffix.\n"
        "   -expand        Increase the size of a .vhd or .vhdx file.  Size can be\n"
        "                  specified with a 'k', 'm', 'g', or 't' suffix.\n"
        "   -merge         Merge a differencing .vhd or .vhdx file into its parent\n"
        "   -shrink        Decrease the size of a .vhdx file.  Size can be specified\n"
        "                  with a 'k', 'm', 'g', or 't' suffix.\n";

/**
 Display usage text to the user.
 */
BOOL
VhdToolHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("VhdTool %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strVhdToolHelpText);
    return TRUE;
}

/**
 A set of detected file types supported by this application.
 */
typedef enum _VHDTOOL_EXT_TYPE {
    VhdToolExtUnknown = 0,
    VhdToolExtVhd = 1,
    VhdToolExtVhdx = 2,
    VhdToolExtIso = 3,
} VHDTOOL_EXT_TYPE;

/**
 A set of supported sector sizes.
 */
typedef enum _VHDTOOL_SECTOR_SIZE {
    VhdToolSectorDefault = 0,
    VhdToolSector512Native = 1,
    VhdToolSector512e = 2,
    VhdToolSector4kNative = 3
} VHDTOOL_SECTOR_SIZE;

/**
 Clone a fixed ISO file.

 @param Path Pointer to the path of the file to create.

 @param SourcePath Pointer to the path of the device to populate from.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
VhdToolCloneIso(
    __in PYORI_STRING Path,
    __in PYORI_STRING SourcePath
    )
{
    HANDLE SourceHandle;
    HANDLE TargetHandle;
    YORI_STRING FullPath;
    YORI_STRING FullSourcePath;
    PVOID Buffer;
    DWORD BufferSize;
    DWORD BytesRead;
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD FreeClusters;
    DWORD TotalClusters;
    DISK_GEOMETRY DiskGeometry;
    LPTSTR ErrText;
    DWORD Err;

    YoriLibInitEmptyString(&FullPath);
    YoriLibInitEmptyString(&FullSourcePath);

    if (!YoriLibUserStringToSingleFilePath(Path, TRUE, &FullPath)) {
        return FALSE;
    }

    if (!YoriLibUserStringToSingleFilePath(SourcePath, TRUE, &FullSourcePath)) {
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    //
    //  Open the source.  Note this can be a file or a device.
    //

    SourceHandle = CreateFile(FullSourcePath.StartOfString, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (SourceHandle == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of source failed: %y: %s"), &FullSourcePath, ErrText);
        YoriLibFreeStringContents(&FullPath);
        YoriLibFreeStringContents(&FullSourcePath);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    TargetHandle = CreateFile(FullPath.StartOfString, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (SourceHandle == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Open of target failed: %y: %s"), &FullPath, ErrText);
        YoriLibFreeStringContents(&FullPath);
        YoriLibFreeStringContents(&FullSourcePath);
        CloseHandle(SourceHandle);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    //
    //  Try to query the sector size of the source, first as a device, and if
    //  that fails, as a file.
    //

    if (DeviceIoControl(SourceHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &DiskGeometry, sizeof(DiskGeometry), &BytesRead, NULL)) {
        BytesPerSector = DiskGeometry.BytesPerSector;
    } else if (!GetDiskFreeSpace(FullSourcePath.StartOfString, &SectorsPerCluster, &BytesPerSector, &FreeClusters, &TotalClusters)) {
        BytesPerSector = 4096;
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("BytesPerSector could not be detected, using default %i\n"), BytesPerSector);
    }

    BufferSize = 1024 * 1024;
    Buffer = YoriLibMalloc(BufferSize);
    if (Buffer == NULL) {
        YoriLibFreeStringContents(&FullPath);
        YoriLibFreeStringContents(&FullSourcePath);
        CloseHandle(SourceHandle);
        CloseHandle(TargetHandle);
        return FALSE;
    }

    //
    //  Copy data.  Devices appear to fail outright if the end of the device
    //  is reached, so the final portion is read one sector at a time.
    //

    while(TRUE) {
        if (!ReadFile(SourceHandle, Buffer, BufferSize, &BytesRead, NULL)) {
            Err = GetLastError();
            if (Err == ERROR_INVALID_FUNCTION) {
                if (BufferSize != BytesPerSector) {
                    BufferSize = BytesPerSector;
                    continue;
                }
            }
            break;
        }

        if (BytesRead == 0) {
            break;
        }

        WriteFile(TargetHandle, Buffer, BytesRead, &BytesRead, NULL);
    }

    YoriLibFree(Buffer);
    YoriLibFreeStringContents(&FullPath);
    YoriLibFreeStringContents(&FullSourcePath);
    CloseHandle(SourceHandle);
    CloseHandle(TargetHandle);
    return TRUE;
}

/**
 Create a fixed or dynamic VHD file.

 @param Path Pointer to the path of the file to create.

 @param SizeAsString Pointer to a string form of the size of the file to
        create.

 @param Fixed If TRUE, the VHD file is created fully allocated.  If not, the
        file is expanded as needed.

 @param ExtType The extension type for the virtual disk.

 @param SourceFile Optionally points to a source to populate from.  Note
        that if this is specified SizeAsString is meaningless.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
VhdToolCreateNewVhd(
    __in PYORI_STRING Path,
    __in_opt PYORI_STRING SizeAsString,
    __in BOOL Fixed,
    __in VHDTOOL_EXT_TYPE ExtType,
    __in_opt PYORI_STRING SourceFile
    )
{
    LARGE_INTEGER FileSize;
    CREATE_VIRTUAL_DISK_PARAMETERS CreateParams;
    VIRTUAL_STORAGE_TYPE StorageType;
    HANDLE Handle;
    DWORD Flags;
    DWORD Err;
    YORI_STRING FullPath;
    YORI_STRING FullSourcePath;

    YoriLibLoadVirtDiskFunctions();
    if (DllVirtDisk.pCreateVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: OS support not present\n"));
        return FALSE;
    }

    if (ExtType == VhdToolExtIso) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: cannot create empty ISO\n"));
        return FALSE;
    }

    ZeroMemory(&CreateParams, sizeof(CreateParams));
    ZeroMemory(&StorageType, sizeof(StorageType));

    YoriLibInitEmptyString(&FullPath);
    YoriLibInitEmptyString(&FullSourcePath);

    if (!YoriLibUserStringToSingleFilePath(Path, TRUE, &FullPath)) {
        return FALSE;
    }

    CreateParams.Version = 1;
    if (SizeAsString != NULL) {
        FileSize = YoriLibStringToFileSize(SizeAsString);
        CreateParams.Version1.MaximumSize = FileSize.QuadPart;
    }
    CreateParams.Version1.BlockSizeInBytes = 0;
    CreateParams.Version1.SectorSizeInBytes = 0x200;
    CreateParams.Version1.ParentPath = NULL;
    if (SourceFile != NULL) {
        if (!YoriLibUserStringToSingleFilePath(SourceFile, TRUE, &FullSourcePath)) {
            YoriLibFreeStringContents(&FullPath);
            return FALSE;
        }
        CreateParams.Version1.SourcePath = FullSourcePath.StartOfString;
    } else {
        CreateParams.Version1.SourcePath = NULL;
    }

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD;
    memcpy(&StorageType.VendorId, &VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT, sizeof(GUID));

    Flags = 0;
    if (Fixed) {
        Flags |= CREATE_VIRTUAL_DISK_FLAG_FULL_PHYSICAL_ALLOCATION;
    }

    Err = DllVirtDisk.pCreateVirtualDisk(&StorageType,
                                         FullPath.StartOfString,
                                         VIRTUAL_DISK_ACCESS_CREATE,
                                         NULL,
                                         Flags,
                                         0,
                                         &CreateParams,
                                         NULL,
                                         &Handle);

    if (Err != 0) {
        LPTSTR ErrText;
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Create of disk failed: %y: %s"), &FullPath, ErrText);
        YoriLibFreeStringContents(&FullPath);
        YoriLibFreeStringContents(&FullSourcePath);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    YoriLibFreeStringContents(&FullSourcePath);
    CloseHandle(Handle);
    return TRUE;
}

/**
 Create a fixed or dynamic VHDX file.

 @param Path Pointer to the path of the file to create.

 @param SizeAsString Pointer to a string form of the size of the file to
        create.

 @param Fixed If TRUE, the VHD file is created fully allocated.  If not, the
        file is expanded as needed.

 @param SectorSize The size of each sector within the virtual disk.

 @param SourceFile Optionally points to a source to populate from.  Note
        that if this is specified SizeAsString is meaningless.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
VhdToolCreateNewVhdx(
    __in PYORI_STRING Path,
    __in_opt PYORI_STRING SizeAsString,
    __in BOOL Fixed,
    __in VHDTOOL_SECTOR_SIZE SectorSize,
    __in_opt PYORI_STRING SourceFile
    )
{
    LARGE_INTEGER FileSize;
    CREATE_VIRTUAL_DISK_PARAMETERS CreateParams;
    VIRTUAL_STORAGE_TYPE StorageType;
    HANDLE Handle;
    DWORD Flags;
    DWORD Err;
    YORI_STRING FullPath;
    YORI_STRING FullSourcePath;

    YoriLibLoadVirtDiskFunctions();
    if (DllVirtDisk.pCreateVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: OS support not present\n"));
        return FALSE;
    }

    ZeroMemory(&CreateParams, sizeof(CreateParams));
    ZeroMemory(&StorageType, sizeof(StorageType));

    YoriLibInitEmptyString(&FullPath);
    YoriLibInitEmptyString(&FullSourcePath);

    if (!YoriLibUserStringToSingleFilePath(Path, TRUE, &FullPath)) {
        return FALSE;
    }

    CreateParams.Version = 2;
    if (SizeAsString != NULL) {
        FileSize = YoriLibStringToFileSize(SizeAsString);
        CreateParams.Version2.MaximumSize = FileSize.QuadPart;
    }
    CreateParams.Version2.BlockSizeInBytes = 0;
    switch (SectorSize) {
        case VhdToolSector512e:
            CreateParams.Version2.SectorSizeInBytes = 0x200;
            CreateParams.Version2.PhysicalSectorSizeInBytes = 0x1000;
            break;
        case VhdToolSector4kNative:
            CreateParams.Version2.SectorSizeInBytes = 0x1000;
            CreateParams.Version2.PhysicalSectorSizeInBytes = 0x1000;
            break;
        default:
            CreateParams.Version2.SectorSizeInBytes = 0x200;
            CreateParams.Version2.PhysicalSectorSizeInBytes = 0x200;
            break;
    }
    CreateParams.Version2.ParentPath = NULL;
    if (SourceFile != NULL) {
        if (!YoriLibUserStringToSingleFilePath(SourceFile, TRUE, &FullSourcePath)) {
            return FALSE;
        }
        CreateParams.Version2.SourcePath = FullSourcePath.StartOfString;
    } else {
        CreateParams.Version2.SourcePath = NULL;
    }
    CreateParams.Version2.ParentVirtualStorageType = 0;
    CreateParams.Version2.SourceVirtualStorageType = 0;

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHDX;
    memcpy(&StorageType.VendorId, &VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT, sizeof(GUID));

    Flags = 0;
    if (Fixed) {
        Flags |= CREATE_VIRTUAL_DISK_FLAG_FULL_PHYSICAL_ALLOCATION;
    }

    Err = DllVirtDisk.pCreateVirtualDisk(&StorageType,
                                         FullPath.StartOfString,
                                         0,
                                         NULL,
                                         Flags,
                                         0,
                                         &CreateParams,
                                         NULL,
                                         &Handle);

    if (Err != 0) {
        LPTSTR ErrText;
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Create of disk failed: %y: %s"), &FullPath, ErrText);
        YoriLibFreeStringContents(&FullPath);
        YoriLibFreeStringContents(&FullSourcePath);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    YoriLibFreeStringContents(&FullSourcePath);
    CloseHandle(Handle);
    return TRUE;
}

/**
 Create a differencing VHD file.

 @param Path Pointer to the path of the file to create.

 @param ParentPath Pointer to the path of the parent.

 @param ExtType The extension type for the virtual disk.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
VhdToolCreateDiffVhd(
    __in PYORI_STRING Path,
    __in PYORI_STRING ParentPath,
    __in VHDTOOL_EXT_TYPE ExtType
    )
{
    CREATE_VIRTUAL_DISK_PARAMETERS CreateParams;
    VIRTUAL_STORAGE_TYPE StorageType;
    HANDLE Handle;
    DWORD Flags;
    DWORD Err;
    YORI_STRING FullPath;
    YORI_STRING FullParentPath;

    YoriLibLoadVirtDiskFunctions();
    if (DllVirtDisk.pCreateVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: OS support not present\n"));
        return FALSE;
    }

    if (ExtType == VhdToolExtIso) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: cannot create differencing ISO\n"));
        return FALSE;
    }

    ZeroMemory(&CreateParams, sizeof(CreateParams));
    ZeroMemory(&StorageType, sizeof(StorageType));

    YoriLibInitEmptyString(&FullPath);
    YoriLibInitEmptyString(&FullParentPath);

    if (!YoriLibUserStringToSingleFilePath(Path, TRUE, &FullPath)) {
        return FALSE;
    }

    if (!YoriLibUserStringToSingleFilePath(ParentPath, TRUE, &FullParentPath)) {
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    CreateParams.Version = 1;
    CreateParams.Version1.MaximumSize = 0;
    CreateParams.Version1.BlockSizeInBytes = 0;
    CreateParams.Version1.SectorSizeInBytes = 0x200;
    CreateParams.Version1.ParentPath = FullParentPath.StartOfString;
    CreateParams.Version1.SourcePath = NULL;

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD;
    memcpy(&StorageType.VendorId, &VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT, sizeof(GUID));

    Flags = 0;

    Err = DllVirtDisk.pCreateVirtualDisk(&StorageType,
                                         FullPath.StartOfString,
                                         VIRTUAL_DISK_ACCESS_CREATE,
                                         NULL,
                                         Flags,
                                         0,
                                         &CreateParams,
                                         NULL,
                                         &Handle);

    if (Err != 0) {
        LPTSTR ErrText;
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Create of disk failed: %y: %s"), &FullPath, ErrText);
        YoriLibFreeStringContents(&FullPath);
        YoriLibFreeStringContents(&FullParentPath);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    YoriLibFreeStringContents(&FullParentPath);
    CloseHandle(Handle);
    return TRUE;
}

/**
 Create a differencing VHDX file.

 @param Path Pointer to the path of the file to create.

 @param ParentPath Pointer to the path of the parent.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
VhdToolCreateDiffVhdx(
    __in PYORI_STRING Path,
    __in PYORI_STRING ParentPath
    )
{
    CREATE_VIRTUAL_DISK_PARAMETERS CreateParams;
    VIRTUAL_STORAGE_TYPE StorageType;
    HANDLE Handle;
    DWORD Flags;
    DWORD Err;
    YORI_STRING FullPath;
    YORI_STRING FullParentPath;

    YoriLibLoadVirtDiskFunctions();
    if (DllVirtDisk.pCreateVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: OS support not present\n"));
        return FALSE;
    }

    ZeroMemory(&CreateParams, sizeof(CreateParams));
    ZeroMemory(&StorageType, sizeof(StorageType));

    YoriLibInitEmptyString(&FullPath);
    YoriLibInitEmptyString(&FullParentPath);

    if (!YoriLibUserStringToSingleFilePath(Path, TRUE, &FullPath)) {
        return FALSE;
    }

    if (!YoriLibUserStringToSingleFilePath(ParentPath, TRUE, &FullParentPath)) {
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    CreateParams.Version = 2;
    CreateParams.Version2.MaximumSize = 0;
    CreateParams.Version2.BlockSizeInBytes = 0;
    CreateParams.Version2.SectorSizeInBytes = 0;
    CreateParams.Version2.PhysicalSectorSizeInBytes = 0;
    CreateParams.Version2.ParentPath = FullParentPath.StartOfString;
    CreateParams.Version2.SourcePath = NULL;
    CreateParams.Version2.ParentVirtualStorageType = 0;
    CreateParams.Version2.SourceVirtualStorageType = 0;

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHDX;
    memcpy(&StorageType.VendorId, &VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT, sizeof(GUID));

    Flags = 0;

    Err = DllVirtDisk.pCreateVirtualDisk(&StorageType,
                                         FullPath.StartOfString,
                                         0,
                                         NULL,
                                         Flags,
                                         0,
                                         &CreateParams,
                                         NULL,
                                         &Handle);

    if (Err != 0) {
        LPTSTR ErrText;
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Create of disk failed: %y: %s"), &FullPath, ErrText);
        YoriLibFreeStringContents(&FullPath);
        YoriLibFreeStringContents(&FullParentPath);
        YoriLibFreeWinErrorText(ErrText);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    YoriLibFreeStringContents(&FullParentPath);
    CloseHandle(Handle);
    return TRUE;
}

/**
 Expand a VHD to a larger size.

 @param Path Pointer to the path of the file to expand.

 @param SizeAsString Pointer to a string form of the size of the file to
        expand to.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
VhdToolExpand(
    __in PYORI_STRING Path,
    __in PYORI_STRING SizeAsString
    )
{
    LARGE_INTEGER FileSize;
    VIRTUAL_STORAGE_TYPE StorageType;
    HANDLE Handle;
    YORI_STRING FullPath;
    OPEN_VIRTUAL_DISK_PARAMETERS OpenParams;
    EXPAND_VIRTUAL_DISK_PARAMETERS ExpandParams;
    DWORD Err;
    DWORD AccessRequested;
    LPTSTR ErrText;

    YoriLibLoadVirtDiskFunctions();
    if (DllVirtDisk.pExpandVirtualDisk == NULL ||
        DllVirtDisk.pOpenVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: OS support not present\n"));
        return FALSE;
    }

    ZeroMemory(&StorageType, sizeof(StorageType));

    YoriLibInitEmptyString(&FullPath);

    if (!YoriLibUserStringToSingleFilePath(Path, TRUE, &FullPath)) {
        return FALSE;
    }

    FileSize = YoriLibStringToFileSize(SizeAsString);

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    StorageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    ZeroMemory(&OpenParams, sizeof(OpenParams));
    OpenParams.Version = OPEN_VIRTUAL_DISK_VERSION_1;
    OpenParams.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;

    AccessRequested = VIRTUAL_DISK_ACCESS_METAOPS;

    Err = DllVirtDisk.pOpenVirtualDisk(&StorageType, FullPath.StartOfString, AccessRequested, OPEN_VIRTUAL_DISK_FLAG_NONE, &OpenParams, &Handle);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: open of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    ExpandParams.Version = 1;
    ExpandParams.Version1.NewSizeInBytes = FileSize.QuadPart;
    Err = DllVirtDisk.pExpandVirtualDisk(Handle, 0, &ExpandParams, NULL);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: expand of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        CloseHandle(Handle);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    CloseHandle(Handle);
    return TRUE;
}

/**
 Compact a dynamic VHD by removing unused space.

 @param Path Pointer to the path of the file to compact.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
VhdToolCompact(
    __in PYORI_STRING Path
    )
{
    VIRTUAL_STORAGE_TYPE StorageType;
    HANDLE Handle;
    YORI_STRING FullPath;
    OPEN_VIRTUAL_DISK_PARAMETERS OpenParams;
    COMPACT_VIRTUAL_DISK_PARAMETERS CompactParams;
    DWORD Err;
    DWORD AccessRequested;
    LPTSTR ErrText;

    YoriLibLoadVirtDiskFunctions();
    if (DllVirtDisk.pCompactVirtualDisk == NULL ||
        DllVirtDisk.pOpenVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: OS support not present\n"));
        return FALSE;
    }

    ZeroMemory(&StorageType, sizeof(StorageType));

    YoriLibInitEmptyString(&FullPath);

    if (!YoriLibUserStringToSingleFilePath(Path, TRUE, &FullPath)) {
        return FALSE;
    }

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    StorageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    ZeroMemory(&OpenParams, sizeof(OpenParams));
    OpenParams.Version = OPEN_VIRTUAL_DISK_VERSION_1;
    OpenParams.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;

    AccessRequested = VIRTUAL_DISK_ACCESS_METAOPS;

    Err = DllVirtDisk.pOpenVirtualDisk(&StorageType, FullPath.StartOfString, AccessRequested, OPEN_VIRTUAL_DISK_FLAG_NONE, &OpenParams, &Handle);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: open of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    CompactParams.Version = 1;
    Err = DllVirtDisk.pCompactVirtualDisk(Handle, 0, &CompactParams, NULL);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: compact of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        CloseHandle(Handle);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    CloseHandle(Handle);
    return TRUE;
}

/**
 Shrink a VHD to a smaller, specified size.

 @param Path Pointer to the path of the file to shrink.

 @param SizeAsString Pointer to a string form of the size of the file to
        shrink to.

 @param ExtType The extension type for the virtual disk.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
VhdToolShrink(
    __in PYORI_STRING Path,
    __in PYORI_STRING SizeAsString,
    __in VHDTOOL_EXT_TYPE ExtType
    )
{
    LARGE_INTEGER FileSize;
    VIRTUAL_STORAGE_TYPE StorageType;
    HANDLE Handle;
    YORI_STRING FullPath;
    OPEN_VIRTUAL_DISK_PARAMETERS OpenParams;
    RESIZE_VIRTUAL_DISK_PARAMETERS ResizeParams;
    DWORD Err;
    DWORD AccessRequested;
    LPTSTR ErrText;

    YoriLibLoadVirtDiskFunctions();
    if (DllVirtDisk.pResizeVirtualDisk == NULL ||
        DllVirtDisk.pOpenVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: OS support not present\n"));
        return FALSE;
    }

    if (ExtType != VhdToolExtVhdx &&
        ExtType != VhdToolExtUnknown) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: shrink only supported on vhdx\n"));
        return FALSE;
    }

    ZeroMemory(&StorageType, sizeof(StorageType));

    YoriLibInitEmptyString(&FullPath);

    if (!YoriLibUserStringToSingleFilePath(Path, TRUE, &FullPath)) {
        return FALSE;
    }

    FileSize = YoriLibStringToFileSize(SizeAsString);

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    StorageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    ZeroMemory(&OpenParams, sizeof(OpenParams));
    OpenParams.Version = OPEN_VIRTUAL_DISK_VERSION_1;
    OpenParams.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;

    AccessRequested = VIRTUAL_DISK_ACCESS_METAOPS | VIRTUAL_DISK_ACCESS_ATTACH_RW;

    Err = DllVirtDisk.pOpenVirtualDisk(&StorageType, FullPath.StartOfString, AccessRequested, OPEN_VIRTUAL_DISK_FLAG_NONE, &OpenParams, &Handle);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: open of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    ResizeParams.Version = 1;
    ResizeParams.Version1.NewSizeInBytes = FileSize.QuadPart;
    Err = DllVirtDisk.pResizeVirtualDisk(Handle, 1, &ResizeParams, NULL);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: shrink of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        CloseHandle(Handle);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    CloseHandle(Handle);
    return TRUE;
}

/**
 Merge a differencing VHD with its parent.

 @param Path Pointer to the path of the file to merge.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
VhdToolMerge(
    __in PYORI_STRING Path
    )
{
    VIRTUAL_STORAGE_TYPE StorageType;
    HANDLE Handle;
    YORI_STRING FullPath;
    OPEN_VIRTUAL_DISK_PARAMETERS OpenParams;
    MERGE_VIRTUAL_DISK_PARAMETERS MergeParams;
    DWORD Err;
    DWORD AccessRequested;
    LPTSTR ErrText;

    YoriLibLoadVirtDiskFunctions();
    if (DllVirtDisk.pMergeVirtualDisk == NULL ||
        DllVirtDisk.pOpenVirtualDisk == NULL) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: OS support not present\n"));
        return FALSE;
    }

    ZeroMemory(&StorageType, sizeof(StorageType));

    YoriLibInitEmptyString(&FullPath);

    if (!YoriLibUserStringToSingleFilePath(Path, TRUE, &FullPath)) {
        return FALSE;
    }

    StorageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
    StorageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

    ZeroMemory(&OpenParams, sizeof(OpenParams));
    OpenParams.Version = OPEN_VIRTUAL_DISK_VERSION_1;
    OpenParams.Version1.RWDepth = 2;

    AccessRequested = VIRTUAL_DISK_ACCESS_METAOPS;

    Err = DllVirtDisk.pOpenVirtualDisk(&StorageType, FullPath.StartOfString, AccessRequested, OPEN_VIRTUAL_DISK_FLAG_NONE, &OpenParams, &Handle);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: open of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        return FALSE;
    }

    MergeParams.Version = 1;
    MergeParams.Version1.DepthToMerge = 1;
    Err = DllVirtDisk.pMergeVirtualDisk(Handle, 0, &MergeParams, NULL);
    if (Err != ERROR_SUCCESS) {
        ErrText = YoriLibGetWinErrorText(Err);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: merge of %y failed: %s"), &FullPath, ErrText);
        YoriLibFreeWinErrorText(ErrText);
        YoriLibFreeStringContents(&FullPath);
        CloseHandle(Handle);
        return FALSE;
    }

    YoriLibFreeStringContents(&FullPath);
    CloseHandle(Handle);
    return TRUE;
}

/**
 A set of operations supported by this application.
 */
typedef enum _VHDTOOL_OP {
    VhdToolOpNone = 0,
    VhdToolOpCreateFixedVhd = 1,
    VhdToolOpCreateDynamicVhd = 2,
    VhdToolOpExpand = 3,
    VhdToolOpCompact = 4,
    VhdToolOpShrink = 5,
    VhdToolOpCreateDiffVhd = 6,
    VhdToolOpMerge = 7,
    VhdToolOpCloneFixed = 8,
    VhdToolOpCloneDynamic = 9,
} VHDTOOL_OP;

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the vhdtool builtin command.
 */
#define ENTRYPOINT YoriCmd_VHDTOOL
#else
/**
 The main entrypoint for the vhdtool standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the vhdtool cmdlet.

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
    DWORD StartArg;
    YORI_STRING Arg;
    PYORI_STRING FileName = NULL;
    PYORI_STRING FileParent = NULL;
    PYORI_STRING FileSize = NULL;
    VHDTOOL_OP Op;
    VHDTOOL_EXT_TYPE ExtType;
    VHDTOOL_SECTOR_SIZE SectorSize;

    Op = VhdToolOpNone;
    ExtType = VhdToolExtUnknown;
    SectorSize = VhdToolSectorDefault;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                VhdToolHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("clonedynamic")) == 0) {
                if (ArgC > i + 2) {
                    FileName = &ArgV[i + 1];
                    FileParent = &ArgV[i + 2];
                    Op = VhdToolOpCloneDynamic;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("clonefixed")) == 0) {
                if (ArgC > i + 2) {
                    FileName = &ArgV[i + 1];
                    FileParent = &ArgV[i + 2];
                    Op = VhdToolOpCloneFixed;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("compact")) == 0) {
                if (ArgC > i + 1) {
                    FileName = &ArgV[i + 1];
                    Op = VhdToolOpCompact;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("creatediff")) == 0) {
                if (ArgC > i + 2) {
                    FileName = &ArgV[i + 1];
                    FileParent = &ArgV[i + 2];
                    Op = VhdToolOpCreateDiffVhd;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("createdynamic")) == 0) {
                if (ArgC > i + 2) {
                    FileName = &ArgV[i + 1];
                    FileSize = &ArgV[i + 2];
                    Op = VhdToolOpCreateDynamicVhd;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("createfixed")) == 0) {
                if (ArgC > i + 2) {
                    FileName = &ArgV[i + 1];
                    FileSize = &ArgV[i + 2];
                    Op = VhdToolOpCreateFixedVhd;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("expand")) == 0) {
                if (ArgC > i + 2) {
                    FileName = &ArgV[i + 1];
                    FileSize = &ArgV[i + 2];
                    Op = VhdToolOpExpand;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("sector:512")) == 0) {
                SectorSize = VhdToolSector512Native;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("sector:512e")) == 0) {
                SectorSize = VhdToolSector512e;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("sector:4k")) == 0 ||
                       YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("sector:4096")) == 0) {
                SectorSize = VhdToolSector4kNative;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("merge")) == 0) {
                if (ArgC > i + 1) {
                    FileName = &ArgV[i + 1];
                    Op = VhdToolOpMerge;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("shrink")) == 0) {
                if (ArgC > i + 2) {
                    FileName = &ArgV[i + 1];
                    FileSize = &ArgV[i + 2];
                    Op = VhdToolOpShrink;
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

    if (Op == VhdToolOpNone) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("vhdtool: operation not specified\n"));
        return EXIT_FAILURE;
    }

    if (FileName != NULL) {
        YORI_STRING Ext;
        LPTSTR Period;

        Period = YoriLibFindRightMostCharacter(FileName, '.');
        if (Period != NULL) {
            YoriLibInitEmptyString(&Ext);
            Ext.StartOfString = Period + 1;
            Ext.LengthInChars = FileName->LengthInChars - (DWORD)(Period - FileName->StartOfString) - 1;
            if (YoriLibCompareStringWithLiteralInsensitive(&Ext, _T("vhdx")) == 0) {
                ExtType = VhdToolExtVhdx;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Ext, _T("vhd")) == 0) {
                ExtType = VhdToolExtVhd;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Ext, _T("iso")) == 0) {
                ExtType = VhdToolExtIso;
            } 
        }
    }

    switch(Op) {
        case VhdToolOpCloneDynamic:
            if (ExtType == VhdToolExtVhdx) {
                VhdToolCreateNewVhdx(FileName, NULL, FALSE, SectorSize, FileParent);
            } else {
                VhdToolCreateNewVhd(FileName, NULL, FALSE, ExtType, FileParent);
            }
            break;
        case VhdToolOpCloneFixed:
            if (ExtType == VhdToolExtIso) {
                VhdToolCloneIso(FileName, FileParent);
            } else if (ExtType == VhdToolExtVhdx) {
                VhdToolCreateNewVhdx(FileName, NULL, TRUE, SectorSize, FileParent);
            } else {
                VhdToolCreateNewVhd(FileName, NULL, TRUE, ExtType, FileParent);
            }
            break;
        case VhdToolOpCreateFixedVhd:
            if (ExtType == VhdToolExtVhdx) {
                VhdToolCreateNewVhdx(FileName, FileSize, TRUE, SectorSize, NULL);
            } else {
                VhdToolCreateNewVhd(FileName, FileSize, TRUE, ExtType, NULL);
            }
            break;
        case VhdToolOpCreateDynamicVhd:
            if (ExtType == VhdToolExtVhdx) {
                VhdToolCreateNewVhdx(FileName, FileSize, FALSE, SectorSize, NULL);
            } else {
                VhdToolCreateNewVhd(FileName, FileSize, FALSE, ExtType, NULL);
            }
            break;
        case VhdToolOpExpand:
            VhdToolExpand(FileName, FileSize);
            break;
        case VhdToolOpCompact:
            VhdToolCompact(FileName);
            break;
        case VhdToolOpShrink:
            VhdToolShrink(FileName, FileSize, ExtType);
            break;
        case VhdToolOpCreateDiffVhd:
            if (ExtType == VhdToolExtVhdx) {
                VhdToolCreateDiffVhdx(FileName, FileParent);
            } else {
                VhdToolCreateDiffVhd(FileName, FileParent, ExtType);
            }
            break;
        case VhdToolOpMerge:
            VhdToolMerge(FileName);
            break;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
