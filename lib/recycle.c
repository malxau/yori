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
 Attempt to send an object to the recycle bin.

 @param FilePath Pointer to the file path to delete.

 @return TRUE if the object was sent to the recycle bin, FALSE if not.
 */
BOOL
YoriLibRecycleBinFile(
    __in PYORI_STRING FilePath
    )
{
    YORI_SHFILEOP FileOp;
    YORI_STRING FilePathWithDoubleNull;
    INT Result;

    YoriLibLoadShell32Functions();

    //
    //  If loading shell failed or we couldn't find the function, recycling
    //  won't happen.
    //

    if (Shell32.pSHFileOperationW == NULL) {
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
    FileOp.Function = YORI_SHFILEOP_DELETE;
    FileOp.Source = FilePathWithDoubleNull.StartOfString;
    FileOp.Flags = YORI_SHFILEOP_FLAG_SILENT|YORI_SHFILEOP_FLAG_NOCONFIRMATION|YORI_SHFILEOP_FLAG_ALLOWUNDO|YORI_SHFILEOP_FLAG_NOERRORUI;

    Result = Shell32.pSHFileOperationW(&FileOp);
    YoriLibFreeStringContents(&FilePathWithDoubleNull);

    if (Result == 0) {
        return TRUE;
    }
    return FALSE;
}

// vim:sw=4:ts=4:et:
