/**
 * @file lib/recycle.c
 *
 * Yori shell send files to the recycle bin.
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
 * FITNESS ERASE A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ERASE ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>

/**
 Command to the shell to delete an object.
 */
#define YORILIB_SHFILEOP_DELETE              0x003

/**
 Flag to the shell to avoid UI.
 */
#define YORILIB_SHFILEOP_FLAG_SILENT         0x004

/**
 Flag to the shell to suppress confirmation.
 */
#define YORILIB_SHFILEOP_FLAG_NOCONFIRMATION 0x010

/**
 Flag to the shell to place objects in the recycle bin.
 */
#define YORILIB_SHFILEOP_FLAG_ALLOWUNDO      0x040

/**
 Flag to the shell to suppress errors.
 */
#define YORILIB_SHFILEOP_FLAG_NOERRORUI      0x400

/**
 Shell defined structure describing a file operation.
 */
typedef struct _YORILIB_SHFILEOP {

    /**
     hWnd to use for UI, which we don't have an don't want.
     */
    HWND hWndIgnored;

    /**
     The function requested from the shell.
     */
    UINT Function;

    /**
     A NULL terminated list of NULL terminated strings of files to operate
     on.
     */
    LPCTSTR Source;

    /**
     Another NULL terminated list of NULL terminated strings, which is not
     used in this program.
     */
    LPCTSTR Dest;

    /**
     Flags for the operation.
     */
    DWORD Flags;

    /**
     Set to TRUE if the operation was cancelled.
     */
    BOOL Aborted;

    /**
     Shell voodoo.  No seriously, who comes up with this stuff?
     */
    PVOID NameMappings;

    /**
     A title that would be used by some types of UI, but not other types of
     UI, which we don't have and don't want.
     */
    LPCTSTR ProgressTitle;
} YORILIB_SHFILEOP, *PYORILIB_SHFILEOP;

/**
 Function definition for ShFileOperationW.
 */
typedef int (WINAPI * SH_FILE_OPERATION_FN)(PYORILIB_SHFILEOP);

/**
 TRUE once this module has attempted to locate ShFileOperationW.  FALSE if it
 has not yet been searched for.
 */
BOOLEAN YoriLibProbedForShFileOperation = FALSE;

/**
 Pointer to ShFileOperationW.
 */
SH_FILE_OPERATION_FN YoriLibShFileOperationW = NULL;


/**
 Attempt to send an object to the recycle bin.

 @param FilePath Pointer to the file path to delete.

 @return TRUE if the object was sent to the recycle bin, FALSE if not.
 */
BOOL
YoriLibRecycleBinFile(
    __in PYORI_STRING FilePath
    )
{
    YORILIB_SHFILEOP FileOp;
    YORI_STRING FilePathWithDoubleNull;
    INT Result;

    //
    //  If we haven't tried to load Shell yet, do it now.
    //

    if (!YoriLibProbedForShFileOperation) {
        HMODULE hShell;

        YoriLibProbedForShFileOperation = TRUE;
        hShell = LoadLibrary(_T("SHELL32.DLL"));
        if (hShell != NULL) {
            YoriLibShFileOperationW = (SH_FILE_OPERATION_FN)GetProcAddress(hShell, "SHFileOperationW");
        }
    }

    //
    //  If loading shell failed or we couldn't find the function, recycling
    //  won't happen.
    //

    if (YoriLibShFileOperationW == NULL) {
        return FALSE;
    }

    //
    //  Create a double NULL terminated file name.
    //

    YoriLibInitEmptyString(&FilePathWithDoubleNull);
    if (!YoriLibAllocateString(&FilePathWithDoubleNull, FilePath->LengthInChars + 2)) {
        return FALSE;
    }

    //
    //  Shell will explode if it sees \\?\, so try to reconvert back to
    //  Win32 limited paths.
    //

    YoriLibUnescapePath(FilePath, &FilePathWithDoubleNull);

    ASSERT(FilePathWithDoubleNull.LengthAllocated >= FilePathWithDoubleNull.LengthInChars + 2);
    FilePathWithDoubleNull.StartOfString[FilePathWithDoubleNull.LengthInChars] = '\0';
    FilePathWithDoubleNull.StartOfString[FilePathWithDoubleNull.LengthInChars + 1] = '\0';

    //
    //  Ask shell to send the object to the recycle bin.
    //

    ZeroMemory(&FileOp, sizeof(FileOp));
    FileOp.Function = YORILIB_SHFILEOP_DELETE;
    FileOp.Source = FilePathWithDoubleNull.StartOfString;
    FileOp.Flags = YORILIB_SHFILEOP_FLAG_SILENT|YORILIB_SHFILEOP_FLAG_NOCONFIRMATION|YORILIB_SHFILEOP_FLAG_ALLOWUNDO|YORILIB_SHFILEOP_FLAG_NOERRORUI;

    Result = YoriLibShFileOperationW(&FileOp);
    YoriLibFreeStringContents(&FilePathWithDoubleNull);

    if (Result == 0) {
        return TRUE;
    }
    return FALSE;
}

// vim:sw=4:ts=4:et:
