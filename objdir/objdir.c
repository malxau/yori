/**
 * @file objdir/objdir.c
 *
 * Yori shell enumerate object manager objects
 *
 * Copyright (c) 2017-2022 Malcolm J. Smith
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
CHAR strObjDirHelpText[] =
        "\n"
        "Enumerate the contents of the object manager.\n"
        "\n"
        "OBJDIR [-license] [-m] [<spec>...]\n"
        "\n"
        "   -m             Minimal display, file names only\n";

/**
 Display usage text to the user.
 */
BOOL
ObjDirHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("ObjDir %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strObjDirHelpText);
    return TRUE;
}

/**
 Context passed to the callback which is invoked for each file found.
 */
typedef struct _OBJDIR_CONTEXT {

    /**
     TRUE if the display should be minimal and only include file names.
     */
    BOOLEAN MinimalDisplay;

    /**
     Records the total number of objects processed.
     */
    YORI_MAX_SIGNED_T ObjectsFound;

    /**
     Records the total number of directories processed.
     */
    YORI_MAX_SIGNED_T DirsFound;

    /**
     An allocation that receives the target location of a symbolic link.
     This is here so the same allocation can be reused for different links
     encountered when enumerating the same directory.
     */
    LPWSTR SymbolicLinkBuffer;

    /**
     The number of bytes in SymbolicLinkBuffer allocation.
     */
    WORD SymbolicLinkBufferLength;

} OBJDIR_CONTEXT, *POBJDIR_CONTEXT;

/**
 The number of characters to use to display the size of objects in the
 directory.
 */
#define OBJDIR_TYPE_FIELD_SIZE  6

/**
 The number of characters to use to display the count of objects in the
 directory.
 */
#define OBJDIR_COUNT_FIELD_SIZE 6

/**
 Before displaying the contents of a directory, this function displays any
 directory level header information.

 @param DirectoryName The name of the directory being enumerated.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ObjDirOutputBeginningOfDirectorySummary(
    __in PCYORI_STRING DirectoryName
    )
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("\n Directory of %y\n\n"), DirectoryName);
    return TRUE;
}

/**
 After displaying the contents of a directory, this function displays any
 directory level footer information.

 @param ObjDirContext Context describing the state of the enumeration.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
ObjDirOutputEndOfDirectorySummary(
    __in POBJDIR_CONTEXT ObjDirContext
    )
{
    YORI_STRING CountString;
    TCHAR CountStringBuffer[OBJDIR_COUNT_FIELD_SIZE];

    YoriLibInitEmptyString(&CountString);
    CountString.StartOfString = CountStringBuffer;
    CountString.LengthAllocated = sizeof(CountStringBuffer)/sizeof(CountStringBuffer[0]);

    YoriLibNumberToString(&CountString, ObjDirContext->ObjectsFound, 10, 3, ',');

    YoriLibRightAlignString(&CountString, OBJDIR_COUNT_FIELD_SIZE);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y Object(s)\n"), &CountString);

    YoriLibNumberToString(&CountString, ObjDirContext->DirsFound, 10, 3, ',');

    YoriLibRightAlignString(&CountString, OBJDIR_COUNT_FIELD_SIZE);

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y Dir(s)\n"), &CountString);

    YoriLibFreeStringContents(&CountString);
    return TRUE;
}

/**
 Load the reparse buffer from an object manager symbolic link.

 @param FullPath Pointer to the full path to the object.

 @param ObjDirContext Pointer to the context for the application.  This
        contains the buffer to populate with the symbolic link target
        location.

 @param ReparsePath On successful completion, a string to be updated to point
        to the symbolic link target location.

 @return TRUE to indicate the reparse data was successfully obtained, FALSE to
         indicate it was not.
 */
__success(return)
BOOL
ObjDirLoadReparseData(
    __in PCYORI_STRING FullPath,
    __inout POBJDIR_CONTEXT ObjDirContext,
    __out PYORI_STRING ReparsePath
    )
{
    HANDLE FileHandle;
    DWORD BytesReturned;
    LONG NtStatus;
    YORI_OBJECT_ATTRIBUTES ObjectAttributes;
    YORI_UNICODE_STRING LinkTarget;

    if (DllNtDll.pNtOpenSymbolicLinkObject == NULL ||
        DllNtDll.pNtQuerySymbolicLinkObject == NULL) {

        return FALSE;
    }

    YoriLibInitializeObjectAttributes(&ObjectAttributes, NULL, FullPath, 0);

    NtStatus = DllNtDll.pNtOpenSymbolicLinkObject(&FileHandle, GENERIC_READ, &ObjectAttributes);
    if (NtStatus != 0) {
        return FALSE;
    }

    LinkTarget.LengthInBytes =
    LinkTarget.LengthAllocatedInBytes = ObjDirContext->SymbolicLinkBufferLength;
    LinkTarget.Buffer = ObjDirContext->SymbolicLinkBuffer;

    NtStatus = DllNtDll.pNtQuerySymbolicLinkObject(FileHandle, &LinkTarget, &BytesReturned);
    if (NtStatus != 0) {
        CloseHandle(FileHandle);
        return FALSE;
    }

    YoriLibInitEmptyString(ReparsePath);
    ReparsePath->StartOfString = LinkTarget.Buffer;
    ReparsePath->LengthInChars = LinkTarget.LengthInBytes / sizeof(TCHAR);

    CloseHandle(FileHandle);
    return TRUE;
}

/**
 A callback that is invoked when an object is found in an object manager
 directory.

 @param FullPath Fully qualified name to the object that was found.

 @param NameOnly The name of the object, not including the full path to
        the object.

 @param ObjectType The type of the object.

 @param Context Pointer to the ObjDirContext structure indicating the
        action to perform and populated with the number of objects found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
ObjDirFileFoundCallback(
    __in PCYORI_STRING FullPath,
    __in PCYORI_STRING NameOnly,
    __in PCYORI_STRING ObjectType,
    __in PVOID Context
    )
{
    YORI_STRING TypeString;
    YORI_STRING ReparseString;
    TCHAR TypeStringBuffer[OBJDIR_TYPE_FIELD_SIZE];
    POBJDIR_CONTEXT ObjDirContext = (POBJDIR_CONTEXT)Context;
    BOOLEAN IsDirectory;
    BOOLEAN IsLink;
    BOOLEAN IsDevice;
    BOOLEAN IsDriver;

    IsDirectory = FALSE;
    IsLink = FALSE;
    IsDevice = FALSE;
    IsDriver = FALSE;

    if (YoriLibCompareStringWithLiteral(ObjectType, _T("Directory")) == 0) {
        IsDirectory = TRUE;
    } else if (YoriLibCompareStringWithLiteral(ObjectType, _T("SymbolicLink")) == 0) {
        IsLink = TRUE;
    } else if (YoriLibCompareStringWithLiteral(ObjectType, _T("Device")) == 0) {
        IsDevice = TRUE;
    } else if (YoriLibCompareStringWithLiteral(ObjectType, _T("Driver")) == 0) {
        IsDriver = TRUE;
    }

    if (IsDirectory) {
        ObjDirContext->DirsFound++;
    } else {
        ObjDirContext->ObjectsFound++;
    }

    YoriLibInitEmptyString(&TypeString);
    YoriLibInitEmptyString(&ReparseString);

    if (ObjDirContext->MinimalDisplay) {

        YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), NameOnly);

    } else {

        BOOLEAN DisplayReparseBuffer;

        TypeString.StartOfString = TypeStringBuffer;
        TypeString.LengthAllocated = sizeof(TypeStringBuffer)/sizeof(TypeStringBuffer[0]);
        DisplayReparseBuffer = FALSE;

        if (IsLink) {

            //
            //  If the entry should display the reparse buffer contents after
            //  the file name, allocate memory for the buffer if necessary and
            //  load the buffer
            //

            if (ObjDirContext->SymbolicLinkBuffer == NULL) {

                //
                //  This is the maximum size that can fit in a UNICODE_STRING
                //

                ObjDirContext->SymbolicLinkBufferLength = 0xfffe;
                ObjDirContext->SymbolicLinkBuffer = YoriLibMalloc(ObjDirContext->SymbolicLinkBufferLength);
                if (ObjDirContext->SymbolicLinkBuffer == NULL) {
                    ObjDirContext->SymbolicLinkBufferLength = 0;
                }
            }

            if (ObjDirContext->SymbolicLinkBuffer != NULL) {
                ObjDirLoadReparseData(FullPath, ObjDirContext, &ReparseString);
            }
        }

        if (IsDirectory) {
            YoriLibYPrintf(&TypeString, _T("<DIR>"));
        } else if (IsLink) {
            YoriLibYPrintf(&TypeString, _T("<LNK>"));
        } else if (IsDevice) {
            YoriLibYPrintf(&TypeString, _T("<DEV>"));
        } else if (IsDriver) {
            YoriLibYPrintf(&TypeString, _T("<DRV>"));
        } else {
            YoriLibYPrintf(&TypeString, _T("<\?\?\?>"));
        }

        if (ReparseString.LengthInChars > 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y %y [%y]\n"), &TypeString, NameOnly, &ReparseString);
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y %y\n"), &TypeString, NameOnly);
        }

        YoriLibFreeStringContents(&TypeString);
    }

    return TRUE;
}

/**
 A callback that is invoked when a directory cannot be successfully enumerated.

 @param FullName Pointer to the full path of an object manager directory that
        could not be enumerated.

 @param NtStatus The NTSTATUS code describing the failure.

 @param Context Pointer to the ObjDirContext.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
ObjDirFileEnumerateErrorCallback(
    __in PCYORI_STRING FullName,
    __in LONG NtStatus,
    __in PVOID Context
    )
{
    BOOL Result = FALSE;
    LPTSTR ErrText;

    UNREFERENCED_PARAMETER(Context);

    ErrText = YoriLibGetNtErrorText(NtStatus);
    YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Enumerate of %y failed: %08x %s"), FullName, NtStatus, ErrText);
    YoriLibFreeWinErrorText(ErrText);

    return Result;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the dir builtin command.
 */
#define ENTRYPOINT YoriCmd_YOBJDIR
#else
/**
 The main entrypoint for the dir standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the dir cmdlet.

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
    YORI_ALLOC_SIZE_T StartArg = 0;
    OBJDIR_CONTEXT ObjDirContext;
    YORI_STRING Arg;

    ZeroMemory(&ObjDirContext, sizeof(ObjDirContext));

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ObjDirHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2022"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("m")) == 0) {
                ObjDirContext.MinimalDisplay = TRUE;
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

#if YORI_BUILTIN
    YoriLibCancelEnable(FALSE);
#endif

    if (StartArg == 0 || StartArg == ArgC) {
        YORI_STRING FilesInDirectorySpec;
        YoriLibConstantString(&FilesInDirectorySpec, _T("\\"));
        if (!ObjDirContext.MinimalDisplay) {
            ObjDirOutputBeginningOfDirectorySummary(&FilesInDirectorySpec);
        }
        YoriLibForEachObjectEnum(&FilesInDirectorySpec,
                                 0,
                                 ObjDirFileFoundCallback,
                                 ObjDirFileEnumerateErrorCallback,
                                 &ObjDirContext);
    } else {
        for (i = StartArg; i < ArgC; i++) {
            if (!ObjDirContext.MinimalDisplay) {
                ObjDirOutputBeginningOfDirectorySummary(&ArgV[i]);
            }
            YoriLibForEachObjectEnum(&ArgV[i],
                                     0,
                                     ObjDirFileFoundCallback,
                                     ObjDirFileEnumerateErrorCallback,
                                     &ObjDirContext);
        }
    }

    if ((ObjDirContext.ObjectsFound > 0 || ObjDirContext.DirsFound > 0) && !ObjDirContext.MinimalDisplay) {
        ObjDirOutputEndOfDirectorySummary(&ObjDirContext);
    }

    if (ObjDirContext.SymbolicLinkBuffer != NULL) {
        YoriLibFree(ObjDirContext.SymbolicLinkBuffer);
    }

    if (ObjDirContext.ObjectsFound == 0 && ObjDirContext.DirsFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("objdir: no objects found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
