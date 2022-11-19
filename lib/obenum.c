/**
 * @file lib/obenum.c
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
 Initialize an OBJECT_ATTRIBUTES structure.  This is patterned on the
 "official" InitializeObjectAttributes, but since it needed to be redone
 anyway this takes the liberty of simplifying a YORI_STRING to UNICODE_STRING
 conversion.

 @param ObjectAttributes On successful completion, populated with object
        attributes that can be used to call NT APIs.

 @param RootDirectory Handle to an object which Name is being opened relative
        to.  If NULL, Name is a fully specified path.

 @param Name The name of the object to open.

 @param Attributes Attributes for the open.
 */
VOID
YoriLibInitializeObjectAttributes(
    __out PYORI_OBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt HANDLE RootDirectory,
    __in_opt PCYORI_STRING Name,
    __in DWORD Attributes
    )
{
    ObjectAttributes->Length = FIELD_OFFSET(YORI_OBJECT_ATTRIBUTES, NameStorage);
    ObjectAttributes->RootDirectory = RootDirectory;
    ObjectAttributes->Attributes = Attributes;
    ObjectAttributes->SecurityDescriptor = NULL;
    ObjectAttributes->SecurityQOS = NULL;

    if (Name == NULL) {
        ObjectAttributes->Name = NULL;
    } else {
        ObjectAttributes->Name = &ObjectAttributes->NameStorage;
        ObjectAttributes->NameStorage.LengthInBytes = (WORD)(Name->LengthInChars * sizeof(TCHAR));
        ObjectAttributes->NameStorage.LengthAllocatedInBytes = (WORD)(Name->LengthAllocated * sizeof(TCHAR));
        ObjectAttributes->NameStorage.Buffer = Name->StartOfString;
    }
}

/**
 Structure definition for an object manager directory entry.
 */
typedef struct _YORI_OBJECT_DIRECTORY_INFORMATION {

    /**
     The name of the object.
     */
    YORI_UNICODE_STRING ObjectName;

    /**
     The type of the object.
     */
    YORI_UNICODE_STRING ObjectType;
} YORI_OBJECT_DIRECTORY_INFORMATION, *PYORI_OBJECT_DIRECTORY_INFORMATION;

/**
 Enumerate all entries within an object manager directory and call a callback
 function for each entry found.

 @param DirectoryName Pointer to the object manager directory to enumerate.

 @param MatchFlags Flags to apply to the enumeration.

 @param Callback Pointer to a callback function to invoke for each entry.
 
 @param ErrorCallback Pointer to a callback to invoke if any errors are
        encountered.

 @param Context An opaque context pointer that will be supplied to the
        callback functions.

 @return TRUE to indicate all objects were successfully enumerated, FALSE to
         indicate that not all entries could be enumerated.
 */
__success(return)
BOOL
YoriLibForEachObjectEnum(
    __in PCYORI_STRING DirectoryName,
    __in DWORD MatchFlags,
    __in PYORILIB_OBJECT_ENUM_FN Callback,
    __in_opt PYORILIB_OBJECT_ENUM_ERROR_FN ErrorCallback,
    __in_opt PVOID Context
    )
{
    LONG NtStatus;
    HANDLE DirHandle;
    YORI_OBJECT_ATTRIBUTES ObjectAttributes;
    PYORI_OBJECT_DIRECTORY_INFORMATION Buffer;
    PYORI_OBJECT_DIRECTORY_INFORMATION Entry;
    DWORD NameOnlyOffset;
    DWORD BufferSize;
    DWORD EnumContext;
    DWORD BytesReturned;
    BOOLEAN Restart;

    YORI_STRING FullObjectName;
    YORI_STRING ObjectName;
    YORI_STRING ObjectType;

    UNREFERENCED_PARAMETER(Callback);
    UNREFERENCED_PARAMETER(MatchFlags);

    if (DllNtDll.pNtOpenDirectoryObject == NULL ||
        DllNtDll.pNtQueryDirectoryObject == NULL) {

        return FALSE;
    }

    YoriLibInitializeObjectAttributes(&ObjectAttributes, NULL, DirectoryName, 0);

    NtStatus = DllNtDll.pNtOpenDirectoryObject(&DirHandle, DIRECTORY_QUERY, &ObjectAttributes);
    if (NtStatus != 0) {
        if (ErrorCallback != NULL) {
            ErrorCallback(DirectoryName, NtStatus, Context);
        }
        return FALSE;
    }

    BufferSize = 64 * 1024;
    Buffer = YoriLibMalloc(BufferSize);
    if (Buffer == NULL) {
        CloseHandle(DirHandle);
        if (ErrorCallback != NULL) {
            ErrorCallback(DirectoryName, STATUS_INSUFFICIENT_RESOURCES, Context);
        }
    }

    EnumContext = 0;
    Restart = TRUE;
    YoriLibInitEmptyString(&FullObjectName);
    YoriLibInitEmptyString(&ObjectName);
    YoriLibInitEmptyString(&ObjectType);
    NameOnlyOffset = 0;

    //
    //  Loop filling a buffer with entries, then processing the entries
    //

    while (TRUE) {
        NtStatus = DllNtDll.pNtQueryDirectoryObject(DirHandle, Buffer, BufferSize, FALSE, Restart, &EnumContext, &BytesReturned);

        //
        //  If there are no more entries, enumeration is complete
        //

        if (NtStatus == STATUS_NO_MORE_ENTRIES) {
            break;
        }

        //
        //  If an error is returned that's not indicating more entries (which
        //  is a success code), indicate the problem and exit.
        //

        if (NtStatus != 0 && NtStatus != STATUS_MORE_ENTRIES) {
            if (ErrorCallback != NULL) {
                ErrorCallback(DirectoryName, NtStatus, Context);
            }
            YoriLibFreeStringContents(&FullObjectName);
            YoriLibFree(Buffer);
            CloseHandle(DirHandle);
            return FALSE;
        }

        Restart = FALSE;

        //
        //  The buffer is filled with UNICODE_STRING structures from the
        //  front, followed by string buffers.  This means BytesReturned isn't
        //  a valid way to know how many string entries there are.  This API
        //  indicates the termination condition by having a UNICODE_STRING
        //  of empty strings.
        //

        Entry = Buffer;
        while (Entry->ObjectName.LengthInBytes != 0) {
            ObjectName.StartOfString = Entry->ObjectName.Buffer;
            ObjectName.LengthInChars = Entry->ObjectName.LengthInBytes / sizeof(TCHAR);
            ObjectType.StartOfString = Entry->ObjectType.Buffer;
            ObjectType.LengthInChars = Entry->ObjectType.LengthInBytes / sizeof(TCHAR);

            //
            //  Construct a buffer for a full path name if necessary.  This
            //  can be reused across entries so long as the name component
            //  fits.  Keep the directory component unchanged across new
            //  name components.
            //

            if (FullObjectName.LengthAllocated < DirectoryName->LengthInChars + 1 + ObjectName.LengthInChars + 1) {
                YoriLibFreeStringContents(&FullObjectName);
                if (!YoriLibAllocateString(&FullObjectName, DirectoryName->LengthInChars + 1 + ObjectName.LengthInChars + 1 + 100)) {
                    YoriLibFree(Buffer);
                    CloseHandle(DirHandle);
                    if (ErrorCallback != NULL) {
                        ErrorCallback(DirectoryName, STATUS_INSUFFICIENT_RESOURCES, Context);
                    }
                    return FALSE;
                }

                FullObjectName.LengthInChars = YoriLibSPrintfS(FullObjectName.StartOfString, FullObjectName.LengthAllocated, _T("%y"), DirectoryName);
                if (FullObjectName.LengthInChars > 0 &&
                    !YoriLibIsSep(FullObjectName.StartOfString[FullObjectName.LengthInChars - 1])) {
                    FullObjectName.StartOfString[FullObjectName.LengthInChars] = '\\';
                    FullObjectName.LengthInChars = FullObjectName.LengthInChars + 1;
                }

                NameOnlyOffset = FullObjectName.LengthInChars;
            }

            memcpy(&FullObjectName.StartOfString[NameOnlyOffset], ObjectName.StartOfString, ObjectName.LengthInChars * sizeof(TCHAR));
            FullObjectName.LengthInChars = NameOnlyOffset + ObjectName.LengthInChars;
            FullObjectName.StartOfString[FullObjectName.LengthInChars] = '\0';

            if (!Callback(&FullObjectName, &ObjectName, &ObjectType, Context)) {
                break;
            }
            Entry++;
        } 
    }

    YoriLibFreeStringContents(&FullObjectName);
    YoriLibFree(Buffer);
    CloseHandle(DirHandle);
    return TRUE;
}

// vim:sw=4:ts=4:et:
