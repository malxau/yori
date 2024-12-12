/**
 * @file lib/ylstrcat.c
 *
 * Yori string concatenation routines
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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

#include "yoripch.h"
#include "yorilib.h"

/**
 Concatenate one yori string to an existing yori string.  Note the first
 string may be reallocated within this routine and the caller is expected to
 free it with @ref YoriLibFreeStringContents .

 @param String The first string, which will be updated to have the contents
        of the second string appended to it.

 @param AppendString The string to copy to the end of the first string.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibStringConcat(
    __inout PYORI_STRING String,
    __in PCYORI_STRING AppendString
    )
{
    DWORD LengthRequired;

    LengthRequired = String->LengthInChars + AppendString->LengthInChars + 1;
    if (!YoriLibIsSizeAllocatable(LengthRequired)) {
        return FALSE;
    }
    if (LengthRequired > String->LengthAllocated) {
        if (YoriLibIsSizeAllocatable(LengthRequired + 0x100)) {
            LengthRequired = LengthRequired + 0x100;
        }
        if (!YoriLibReallocString(String, (YORI_ALLOC_SIZE_T)LengthRequired)) {
            return FALSE;
        }
    }

    memcpy(&String->StartOfString[String->LengthInChars], AppendString->StartOfString, AppendString->LengthInChars * sizeof(TCHAR));
    String->LengthInChars = String->LengthInChars + AppendString->LengthInChars;
    String->StartOfString[String->LengthInChars] = '\0';
    return TRUE;
}

/**
 Concatenate a literal string to an existing yori string.  Note the first
 string may be reallocated within this routine and the caller is expected to
 free it with @ref YoriLibFreeStringContents .

 @param String The first string, which will be updated to have the contents
        of the second string appended to it.

 @param AppendString The string to copy to the end of the first string.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriLibStringConcatWithLiteral(
    __inout PYORI_STRING String,
    __in LPCTSTR AppendString
    )
{
    YORI_STRING AppendYoriString;

    //
    //  We need to length count the literal to ensure buffer sizes anyway,
    //  so convert it into a length counted string.
    //

    YoriLibConstantString(&AppendYoriString, AppendString);
    return YoriLibStringConcat(String, &AppendYoriString);
}


// vim:sw=4:ts=4:et:
