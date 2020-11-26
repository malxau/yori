/**
 * @file lib/parse.c
 *
 * Parses an expression into component pieces
 *
 * Copyright (c) 2014-2019 Malcolm J. Smith
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
 Determines if the immediately following characters constitute an argument
 seperator.  Things like "|" or ">" can be placed between arguments without
 spaces but constitute a break in the argument flow.  Some of these consist
 of multiple characters, such as "&&", "||", or ">>".  Depending on the
 argument, this may be self contained, indicating following characters are
 on a subsequent argument, or may not be terminated, indicating following
 characters belong on the same argument as the operator.  Internally, Yori
 keeps redirection paths in the same argument as the redirection operator,
 but subsequent commands belong in subsequent arguments.

 @param String Pointer to the remainder of the string to parse for
        argument breaks.

 @param BraceNestingLevel Optionally points to a value that maintains the
        nesting level of $(foo) style backquotes.  This routine will update
        this value when it encounters a start operator, or a terminating
        brace when this value is nonzero.  Note that for this to work, the
        caller cannot call this function on the same range of the string
        twice - if this function indicates an argument break, that argument
        break is assumed to be processed.

 @param CharsToConsumeOut On successful completion, indicates the number of
        characters that form part of this argument.

 @param TerminateArgOut On successful completion, indicates that the argument
        should be considered complete and subsequent characters should go
        into a subsequent argument.  If FALSE, indicates that subsequent
        characters should continue as part of the same argument as this
        operator.

 @return TRUE to indicate this point in the string is an argument seperator.
         If TRUE is returned, CharsToConsumeOut and TerminateArgOut must
         be consulted to determine the length of the operator and the
         behavior following the operator.  FALSE if this point in the string
         is not an argument seperator.
 */
__success(return)
BOOLEAN
YoriLibIsArgumentSeperator(
    __in PYORI_STRING String,
    __inout_opt PDWORD BraceNestingLevel,
    __out_opt PDWORD CharsToConsumeOut,
    __out_opt PBOOLEAN TerminateArgOut
    )
{
    DWORD CharsToConsume = 0;
    BOOLEAN Terminate = FALSE;

    if (String->LengthInChars >= 1) {
        if (String->StartOfString[0] == '|') {
            CharsToConsume++;
            if (String->LengthInChars >= 2 && String->StartOfString[1] == '|') {
                CharsToConsume++;
            }
            Terminate = TRUE;
        } else if (String->StartOfString[0] == '&') {
            CharsToConsume++;
            if (String->LengthInChars >= 2 && String->StartOfString[1] == '&') {
                CharsToConsume++;
            } else if (String->LengthInChars >= 2 && String->StartOfString[1] == '!') {
                CharsToConsume++;
                if (String->LengthInChars >= 3 && String->StartOfString[2] == '!') {
                    CharsToConsume++;
                }
            }
            Terminate = TRUE;
        } else if (String->StartOfString[0] == '\n') {
            CharsToConsume++;
            Terminate = TRUE;
        } else if (String->StartOfString[0] == '>') {
            CharsToConsume++;
            if (String->LengthInChars >= 2 && String->StartOfString[1] == '>') {
                CharsToConsume++;
            } else if (String->LengthInChars >= 3 && String->StartOfString[1] == '&' && String->StartOfString[2] == '2') {
                CharsToConsume += 2;
                Terminate = TRUE;
            }
        } else if (String->StartOfString[0] == '<') {
            CharsToConsume++;
        } else if (String->StartOfString[0] == '1') {
            if (String->LengthInChars >= 2 && String->StartOfString[1] == '>') {
                CharsToConsume += 2;
                if (String->LengthInChars >= 3 && String->StartOfString[2] == '>') {
                    CharsToConsume++;
                } else if (String->LengthInChars >= 4 && String->StartOfString[2] == '&' && String->StartOfString[3] == '2') {
                    CharsToConsume += 2;
                    Terminate = TRUE;
                }
            }
        } else if (String->StartOfString[0] == '2') {
            if (String->LengthInChars >= 2 && String->StartOfString[1] == '>') {
                CharsToConsume += 2;
                if (String->LengthInChars >= 3 && String->StartOfString[2] == '>') {
                    CharsToConsume++;
                } else if (String->LengthInChars >= 4 && String->StartOfString[2] == '&' && String->StartOfString[3] == '1') {
                    CharsToConsume += 2;
                    Terminate = TRUE;
                }
            }
        } else if (String->StartOfString[0] == '`') {
            CharsToConsume++;
            Terminate = TRUE;
        } else if (String->StartOfString[0] == '$') {
            if (String->LengthInChars >= 2 && String->StartOfString[1] == '(') {

                if (BraceNestingLevel != NULL) {
                    (*BraceNestingLevel)++;
                }

                CharsToConsume += 2;
                Terminate = TRUE;
            }

        } else if (String->StartOfString[0] == ')' &&
                   (BraceNestingLevel == NULL || (*BraceNestingLevel) > 0)) {

            if (BraceNestingLevel != NULL) {
                (*BraceNestingLevel)--;
            }

            CharsToConsume++;
            Terminate = TRUE;
        }
    }

    if (TerminateArgOut != NULL) {
        *TerminateArgOut = Terminate;
    }

    if (CharsToConsumeOut != NULL) {
        *CharsToConsumeOut = CharsToConsume;
    }

    if (CharsToConsume > 0) {
        return TRUE;
    } 

    return FALSE;
}

// vim:sw=4:ts=4:et:
