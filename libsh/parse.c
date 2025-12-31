/**
 * @file libsh/parse.c
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
#include <yorish.h>

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
YoriLibShIsArgumentSeperator(
    __in PYORI_STRING String,
    __out_opt PYORI_ALLOC_SIZE_T CharsToConsumeOut,
    __out_opt PBOOLEAN TerminateArgOut
    )
{
    YORI_ALLOC_SIZE_T CharsToConsume = 0;
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

/**
 Allocate the ArgV and ArgContexts arrays within a CmdContext.  Optionally the
 caller can request additional bytes to be in this allocation, and if so, this
 routine will output a pointer to the additional payload.

 @param CmdContext Pointer to the CmdContext whose arrays should be allocated.

 @param ArgCount Specifies the number of arguments to allocate.

 @param ExtraByteCount Specifies the number of extra bytes to include in the
        allocation.  If this is nonzero, the ExtraData argument is mandatory.

 @param ExtraData Pointer to a pointer that will receive the location of the
        extra allocation, if ExtraByteCount is nonzero.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriLibShAllocateArgCount(
    __out PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __in YORI_ALLOC_SIZE_T ArgCount,
    __in YORI_ALLOC_SIZE_T ExtraByteCount,
    __out_opt PVOID *ExtraData
    )
{
    PVOID MemoryToFree;
    MemoryToFree = YoriLibReferencedMalloc((ArgCount * (sizeof(YORI_STRING) + sizeof(YORI_LIBSH_ARG_CONTEXT))) +
                                                       ExtraByteCount);
    if (MemoryToFree == NULL) {
        return FALSE;
    }

    ZeroMemory(MemoryToFree, ArgCount * (sizeof(YORI_STRING) + sizeof(YORI_LIBSH_ARG_CONTEXT)));

    CmdContext->ArgC = ArgCount;
    CmdContext->ArgV = MemoryToFree;
    CmdContext->MemoryToFreeArgV = MemoryToFree;

    YoriLibReference(MemoryToFree);
    CmdContext->ArgContexts = (PYORI_LIBSH_ARG_CONTEXT)YoriLibAddToPointer(CmdContext->ArgV, ArgCount * sizeof(YORI_STRING));
    CmdContext->MemoryToFreeArgContexts = MemoryToFree;

    //
    //  MSFIX: No explicit reference on ExtraData - is it needed?
    //

    if (ExtraData != NULL) {
        if (ExtraByteCount != 0) {
            *ExtraData = YoriLibAddToPointer(CmdContext->ArgContexts, ArgCount * sizeof(YORI_LIBSH_ARG_CONTEXT));
        } else {
            *ExtraData = NULL;
        }
    }

    return TRUE;
}

/**
 Remove spaces from the beginning of a Yori string.  Note this implies
 advancing the StartOfString pointer, so a caller cannot assume this
 pointer is unchanged across the call.

 @param String Pointer to the Yori string to remove spaces from.
 */
VOID
YoriLibShTrimSpacesFromBeginning(
    __in PYORI_STRING String
    )
{
    while (String->LengthInChars > 0) {
        if (String->StartOfString[0] == ' ') {
            String->StartOfString++;
            String->LengthInChars--;
        } else {
            break;
        }
    }
}

/**
 Remove spaces or @ characters from the beginning of a Yori string.
 Note this implies advancing the StartOfString pointer, so a caller cannot
 assume this pointer is unchanged across the call.

 @param String Pointer to the Yori string to remove leading chars from.
 */
VOID
YoriLibShTrimSpacesAndAtsFromBeginning(
    __in PYORI_STRING String
    )
{
    while (String->LengthInChars > 0) {
        if (String->StartOfString[0] == ' ' ||
            String->StartOfString[0] == '@') {

            String->StartOfString++;
            String->LengthInChars--;
        } else {
            break;
        }
    }
}


/**
 A backslash is encountered while parsing into arguments.

 There are five cases to consider:
  1. The backslashes are not followed by a quote or argument break, so
     preserve them into the argument.
  2. An odd number of backslashes are followed by a quote, so preserve the
     backslashes and quote into the argument (no argument break.)
  3. An even number of backslashes is followed by a quote, and the quote is
     followed by an argument break.  Preserve the backslashes and let the
     quote be described in the ArgContext.
  4. An even number of backslashes is followed by a quote, but the argument
     does not break (eg. "C:\Program Files\\"WindowsApps).  In this case the
     quote will move, so only half of the backslashes should be carried
     forward.
  5. Backslashes are followed by an argument break, but not a quote.  If a
     quote is about to be implicitly moved to follow these backslashes, they
     need to be escaped (ie., doubled.)

 In order to express this, this routine determines the number of characters
 to read from the source string and the number to write to the target.

 If the number to write is greater than the number to read, the first char is
 applied multiple times.  If the number to read is greater than the number to
 write, the difference is swallowed from the source string.

 @param Char Pointer to the current point of the string being parsed.

 @param QuoteOpen Indicates whether a quoted region is currently active.

 @param LookingForFirstQuote Indicates whether a quote opened the argument
        and the parser is looking for the next quote to terminate it.

 @param QuoteImplicitlyAddedAtEnd If TRUE, a quote will be added at the end
        of the argument.  This occurs when a quote terminated within the
        argument, and more text follows, then the argument itself terminates.

 @param CharsToConsume On completion, indicates the number of characters to
        read from the source string.

 @param CharsToOutput On completion, indicates the numbre of characters to
        write to the destination argument.
 */
VOID
YoriLibShCountCharsAtBackslash(
    __in PYORI_STRING Char,
    __in BOOLEAN QuoteOpen,
    __in BOOLEAN LookingForFirstQuote,
    __in BOOLEAN QuoteImplicitlyAddedAtEnd,
    __out PYORI_ALLOC_SIZE_T CharsToConsume,
    __out PYORI_ALLOC_SIZE_T CharsToOutput
    )
{
    YORI_STRING Remaining;
    YORI_ALLOC_SIZE_T ConsecutiveBackslashes;
    YORI_ALLOC_SIZE_T LocalCharsToConsume;
    YORI_ALLOC_SIZE_T LocalCharsToOutput;
    YORI_ALLOC_SIZE_T CharsToIgnore;
    BOOLEAN TrailingQuote;
    BOOLEAN TerminateArg;
    BOOLEAN TerminateNextArg;

    ConsecutiveBackslashes = YoriLibCntStringWithChars(Char, _T("\\"));

    YoriLibInitEmptyString(&Remaining);
    Remaining.StartOfString = &Char->StartOfString[ConsecutiveBackslashes];
    Remaining.LengthInChars = Char->LengthInChars - ConsecutiveBackslashes;

    LocalCharsToOutput = LocalCharsToConsume = ConsecutiveBackslashes;
    TrailingQuote = FALSE;
    TerminateArg = FALSE;
    if (Remaining.LengthInChars > 0 &&
        Remaining.StartOfString[0] == '"') {
        Remaining.StartOfString = Remaining.StartOfString + 1;
        Remaining.LengthInChars = Remaining.LengthInChars - 1;
        TrailingQuote = TRUE;
    } else if (!QuoteOpen) {
        if (Remaining.LengthInChars == 0 ||
            Remaining.StartOfString[0] == ' ' ||
            YoriLibShIsArgumentSeperator(&Remaining, &CharsToIgnore, &TerminateNextArg)) {
            TerminateArg = TRUE;
        }
    }

    if (TrailingQuote) {

        if ((ConsecutiveBackslashes % 2) != 0) {

            LocalCharsToConsume++;
            LocalCharsToOutput++;
        } else if (QuoteOpen && LookingForFirstQuote) {


            if (Remaining.LengthInChars > 0 &&
                Remaining.StartOfString[0] != ' ' &&
                !YoriLibShIsArgumentSeperator(&Remaining, &CharsToIgnore, &TerminateNextArg)) {

                LocalCharsToOutput = ConsecutiveBackslashes / 2;
            }
        }
    } else if (TerminateArg && QuoteImplicitlyAddedAtEnd) {
        LocalCharsToOutput = LocalCharsToConsume * 2;
    }

    *CharsToConsume = LocalCharsToConsume;
    *CharsToOutput = LocalCharsToOutput;
}

/**
 Move to the next argument when processing the initial scan.  This routine
 performs processing on the completed argument and updates variables to
 prepare for the next argument.

 @param CmdContext Pointer to the command context.  This routine may update
        the current argument offset if the argument needs to be modified.

 @param CurrentArgFound TRUE if the CmdContext has been populated with the
        current argument.  FALSE if it has not, meaning the argument being
        completed cannot need to update the CmdContext.

 @param ArgCountPtr Points to the current number of arguments, which is
        incremented in this routine.

 @param ArgOffsetPtr Points to the offset within the current argument, which
        is reset within this routine.

 @param PreviousCharWasQuote On input, points to a boolean indicating whether
        the final character in the argument was a quote.  For this routine,
        that could not be determined from the buffer itself, but this value
        also indicates that the quote was not escaped and was parsed as a
        shell character.  On output, updated to indicate the new argument
        does not terminate with a quote.

 @param QuoteTerminated Points to a value that will be updated to indicate
        the new argument does not have a terminated quote sequence.

 @param FirstQuoteEndOffsetPtr On input, points to a value indicating the
        character offset of the quote that would terminate any opening quote
        in the argument.  If the argument ends with a quote, this quote is
        retained, and the trailing quote is removed; if the argument does not
        end in a quote, this quote is removed.  On output, updated to indicate
        no first quote terminator has been found.
 */
VOID
YoriLibParseMoveToNextArgumentInitialScan(
    __inout PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __in BOOLEAN CurrentArgFound,
    __inout PYORI_ALLOC_SIZE_T ArgCountPtr,
    __inout PYORI_ALLOC_SIZE_T ArgOffsetPtr,
    __inout PBOOLEAN PreviousCharWasQuote,
    __out PBOOLEAN QuoteTerminated,
    __inout PYORI_ALLOC_SIZE_T FirstQuoteEndOffsetPtr
    )
{
    YORI_ALLOC_SIZE_T ArgCount;

    ArgCount = *ArgCountPtr;

    //
    //  If FirstQuoteEndOffsetPtr is in range, and
    //   - The final char in the arg was a quote, truncate the arg
    //   - The final char was not a quote, move string contents
    //     over FirstQuoteEndOffsetPtr
    //
    //  In this function, the second case needs to adjust
    //  CmdContext->CurrentArgOffset .
    //

    if ((*FirstQuoteEndOffsetPtr) != YORI_MAX_ALLOC_SIZE) {
        YORI_ALLOC_SIZE_T FirstQuoteEndOffset;
        FirstQuoteEndOffset = *FirstQuoteEndOffsetPtr;
        ASSERT(FirstQuoteEndOffset < (*ArgOffsetPtr));

        if (!(*PreviousCharWasQuote) &&
            CurrentArgFound &&
            ArgCount == CmdContext->CurrentArg) {

            if (FirstQuoteEndOffset < CmdContext->CurrentArgOffset) {
                CmdContext->CurrentArgOffset--;
            }
        }
    }

    (*ArgCountPtr)++;
    *ArgOffsetPtr = 0;
    *QuoteTerminated = FALSE;
    *FirstQuoteEndOffsetPtr = YORI_MAX_ALLOC_SIZE;
    *PreviousCharWasQuote = FALSE;
}

/**
 When building the argument array, this routine terminates a completed
 argument.  A new argument may or may not follow, but this routine does not
 prepare for a new argument, it merely terminates the previous argument.

 @param OutputStringPtr On input, points to a pointer to the character buffer
        that is behind all arguments.  This is updated to contain a NULL
        terminator and advanced within this routine.

 @param Arg Pointer to the argument being completed.  This will have its
        length values applied here, as well as a NULL terminator.

 @param PreviousCharWasQuote TRUE if the final character in the argument was
        a quote.  This value also indicates that the quote was not escaped
        and was parsed as a shell character.

 @param FirstQuoteEndOffset Indicates the character offset of the quote that
        would terminate any opening quote in the argument, or -1 if one does
        not exist.  If the argument ends with a quote, this quote is retained,
        and the trailing quote is removed; if the argument does not end in a
        quote, this quote is removed.
 */
VOID
YoriLibParseTerminateCurrentArgument(
    __in LPTSTR* OutputStringPtr,
    __in PYORI_STRING Arg,
    __in BOOLEAN PreviousCharWasQuote,
    __in YORI_ALLOC_SIZE_T FirstQuoteEndOffset
    )
{
    LPTSTR OutputString;
    YORI_ALLOC_SIZE_T ArgLength;

    OutputString = *OutputStringPtr;
    ArgLength = (YORI_ALLOC_SIZE_T)(OutputString - Arg->StartOfString);

    //
    //  If FirstQuoteEndOffsetPtr is in range, and
    //   - The final char in the arg was a quote, truncate the arg
    //   - The final char was not a quote, move string contents
    //     over FirstQuoteEndOffsetPtr
    //
    //  Note this processing occurs before processing the NULL terminator.
    //  Argument parsing is guaranteed to have space for the NULL, so we
    //  may currently have a quote where the NULL is supposed to be, and
    //  need to move it first (ie., one additional character may have been
    //  written that needs to be removed here.)
    //

    if (FirstQuoteEndOffset != YORI_MAX_ALLOC_SIZE) {
        ASSERT(FirstQuoteEndOffset < ArgLength);
        ASSERT(Arg->StartOfString[FirstQuoteEndOffset] == '"');
        if (PreviousCharWasQuote) {

            //
            //  The explicit variable tracks not just whether a quote is there
            //  but also that it was not subject to an escape.  Nonetheless if
            //  the explicit variable says a quote is there, it should be
            //  there.
            //

            ASSERT(ArgLength > 0 && Arg->StartOfString[ArgLength - 1] == '"');
            ASSERT(&Arg->StartOfString[ArgLength] == (*OutputStringPtr));
        } else {

            //
            //  If the string ended with a quote, we should be in the
            //  branch above.  If it doesn't, there has to be at least
            //  one char following the quote.
            //

            ASSERT(ArgLength - FirstQuoteEndOffset > 1);
            memmove(&Arg->StartOfString[FirstQuoteEndOffset], 
                    &Arg->StartOfString[FirstQuoteEndOffset + 1],
                    (ArgLength - FirstQuoteEndOffset - 1) * sizeof(TCHAR));
        }
        ArgLength--;
        (*OutputStringPtr)--;
    }

    OutputString = *OutputStringPtr;
    *OutputString = '\0';
    Arg->LengthInChars = ArgLength;
    Arg->LengthAllocated = Arg->LengthInChars + 1;
    (*OutputStringPtr)++;
}

/**
 Move to the next argument when populating arguments.

 @param CmdContext Pointer to the command context.  This routine will
        initialize the next argument in this context.

 @param ArgCountPtr Points to the current number of arguments, which is
        incremented in this routine.

 @param RemainingString Points to the remaining characters to parse into
        future arguments.

 @param OutputString Points to a buffer to populate with characters as part
        of argument processing.  The new argument is initialized to point to
        this buffer.

 @param PreviousCharWasQuote Points to a value indicating whether the current
        argument ends in a quote.  Updated within this routine to say the
        new argument does not (yet.)

 @param LookingForFirstQuote Points to a value indicating whether the argument
        starts with a quote.  Updated to be TRUE or FALSE based on the first
        character in RemainingString within this routine.

 @param FirstQuoteEndOffset Points to a value indicating the character offset
        of the quote that would terminate any opening quote in the argument.
        Updated within this routine to indicate no first quote terminator has
        been found.
 */
VOID
YoriLibParseMoveToNextArgumentPopulate(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __inout PYORI_ALLOC_SIZE_T ArgCountPtr,
    __in PYORI_STRING RemainingString,
    __in LPTSTR OutputString,
    __out PBOOLEAN PreviousCharWasQuote,
    __out PBOOLEAN LookingForFirstQuote,
    __out PYORI_ALLOC_SIZE_T FirstQuoteEndOffset
    )
{
    PYORI_STRING Arg;
    YORI_ALLOC_SIZE_T ArgCount;

    (*ArgCountPtr)++;
    ArgCount = (*ArgCountPtr);

    //
    //  This routine should only be called if there are more characters to
    //  place into an argument.
    //

    ASSERT(RemainingString->LengthInChars > 0);

    Arg = &CmdContext->ArgV[ArgCount];
    YoriLibInitEmptyString(Arg);
    Arg->StartOfString = OutputString;
    if (RemainingString->StartOfString[0] == '"') {
        CmdContext->ArgContexts[ArgCount].Quoted = TRUE;
        *LookingForFirstQuote = TRUE;
    } else {
        CmdContext->ArgContexts[ArgCount].Quoted = FALSE;
        *LookingForFirstQuote = FALSE;
    }
    CmdContext->ArgContexts[ArgCount].QuoteTerminated = FALSE;
    YoriLibReference(CmdContext->ArgV);
    Arg->MemoryToFree = CmdContext->ArgV;
    *FirstQuoteEndOffset = YORI_MAX_ALLOC_SIZE;
    *PreviousCharWasQuote = FALSE;
}


/**
 Parse a single command string into a series of arguments.  This routine takes
 care of splitting things based on the presence or absence of quotes, as well
 as performing environment variable expansion.  The resulting string has no
 knowledge of redirects, pipes, or multi program execution - it is just a
 series of arguments.

 The complexity of this routine stems from handling quotes:

 Text"More Text"Text
 ...expands to a single argument, preserving quotes.

 "C:\Program Files"\\foo
 ...expands to a single argument, with quotes at beginning and end (the
    quote in the middle is conceptually moved.)  This is indicated by the
    presence of an initial quote, finding a terminating quote, and not having
    a quote at the end of the argument.

 "."=="."
 ...expands to a single argument, preserving quotes.  This is due to the quote
    found at the end of the argument, even though the initial quote is
    terminated earlier.

 @param CmdLine The string to parse into arguments.

 @param CurrentOffset The current offset within the string.  This can be set
        to zero if not needed.  The argument that corresponds to this offset
        will be marked in the CmdContext as being the active argument.

 @param CmdContext A caller allocated CmdContext to populate with arguments.
        This routine will allocate space for the argument array and contents.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOLEAN
YoriLibShParseCmdlineToCmdContext(
    __in PYORI_STRING CmdLine,
    __in YORI_ALLOC_SIZE_T CurrentOffset,
    __out PYORI_LIBSH_CMD_CONTEXT CmdContext
    )
{
    YORI_ALLOC_SIZE_T ArgCount = 0;
    YORI_ALLOC_SIZE_T ArgOffset = 0;
    YORI_ALLOC_SIZE_T RequiredCharCount = 0;
    YORI_ALLOC_SIZE_T CharsToConsume = 0;
    BOOLEAN TerminateArg;
    BOOLEAN TerminateNextArg = FALSE;
    BOOLEAN QuoteOpen = FALSE;
    YORI_STRING Char;
    LPTSTR OutputString;
    BOOLEAN CurrentArgFound = FALSE;
    BOOLEAN PreviousCharWasQuote = FALSE;
    BOOLEAN IsMultiCommandOperatorArgument = FALSE;

    //
    //  Indicates the argument started with a quote, so the next quote
    //  terminates the initial quote.  This initial quote pair gets
    //  special treatment, to support "C:\Program Files"\foo type things.
    //

    BOOLEAN LookingForFirstQuote = FALSE;

    //
    //  Indicates that the argument contained an ending quote, as opposed
    //  to an opening quote followed by end of string.
    //

    BOOLEAN QuoteTerminated = FALSE;

    //
    //  Indicates the character offset within the argument that points to
    //  the quote that terminates the initial quote.  This is because we
    //  won't know until completing argument parsing whether it should be
    //  retained or not, so it is retained and this offset may be used to
    //  remove it afterwards.
    //

    YORI_ALLOC_SIZE_T FirstQuoteEndOffset;

    CmdContext->ArgC = 0;
    CmdContext->ArgV = NULL;
    CmdContext->ArgContexts = NULL;
    CmdContext->TrailingChars = FALSE;
    CmdContext->MemoryToFreeArgV = NULL;
    CmdContext->MemoryToFreeArgContexts = NULL;

    YoriLibInitEmptyString(&Char);
    Char.StartOfString = CmdLine->StartOfString;
    Char.LengthInChars = CmdLine->LengthInChars;

    //
    //  Consume all spaces.
    //

    YoriLibShTrimSpacesAndAtsFromBeginning(&Char);

    if (Char.LengthInChars > 0 && Char.StartOfString[0] == '"') {
        LookingForFirstQuote = TRUE;
    } else {
        LookingForFirstQuote = FALSE;
    }
    FirstQuoteEndOffset = YORI_MAX_ALLOC_SIZE;

    while (Char.LengthInChars > 0) {

        //
        //  If it's an escape char, consume two characters as literal until
        //  we hit the end of the string.  If the end of the string ends the
        //  argument and the argument is quoted, drop the escape so as to not
        //  escape the end quote.
        //

        if (YoriLibIsEscapeChar(Char.StartOfString[0])) {

            Char.StartOfString++;
            Char.LengthInChars--;

            if (Char.LengthInChars > 0 || !QuoteTerminated) {

                PreviousCharWasQuote = FALSE;
                RequiredCharCount++;
                ArgOffset++;

                if (Char.LengthInChars > 0) {
                    Char.StartOfString++;
                    Char.LengthInChars--;
                    RequiredCharCount++;
                    ArgOffset++;
                }
            }

            if (!CurrentArgFound &&
                (Char.StartOfString - CmdLine->StartOfString >= (LONG)CurrentOffset)) {

                CurrentArgFound = TRUE;
                CmdContext->CurrentArg = ArgCount;
                CmdContext->CurrentArgOffset = ArgOffset;
            }

            if (Char.LengthInChars == 0) {
                YoriLibParseMoveToNextArgumentInitialScan(CmdContext, CurrentArgFound, &ArgCount, &ArgOffset, &PreviousCharWasQuote, &QuoteTerminated, &FirstQuoteEndOffset);
            }

            continue;
        }

        if (Char.StartOfString[0] == '\\') {

            YORI_ALLOC_SIZE_T CharsToOutput;

            PreviousCharWasQuote = FALSE;

            YoriLibShCountCharsAtBackslash(&Char,
                                           QuoteOpen,
                                           LookingForFirstQuote,
                                           QuoteTerminated,
                                           &CharsToConsume,
                                           &CharsToOutput);

            RequiredCharCount = RequiredCharCount + CharsToOutput;
            ArgOffset = ArgOffset + CharsToOutput;
            Char.StartOfString += CharsToConsume;
            Char.LengthInChars = Char.LengthInChars - CharsToConsume;

            if (!CurrentArgFound &&
                (Char.StartOfString - CmdLine->StartOfString >= (LONG)CurrentOffset)) {

                CurrentArgFound = TRUE;
                CmdContext->CurrentArg = ArgCount;
                CmdContext->CurrentArgOffset = ArgOffset;
            }

            if (Char.LengthInChars == 0) {
                YoriLibParseMoveToNextArgumentInitialScan(CmdContext, CurrentArgFound, &ArgCount, &ArgOffset, &PreviousCharWasQuote, &QuoteTerminated, &FirstQuoteEndOffset);
            }

            continue;

        } else if (Char.StartOfString[0] == '"') {

            //
            //  If the argument started with a quote and we found the end
            //  to that quote, copy it into the output string, and make a
            //  note of where it is to facilitate later removal.  It will
            //  be removed if the argument doesn't end in a quote, which
            //  we don't know at this point.
            //

            if (QuoteOpen && LookingForFirstQuote) {
                QuoteOpen = FALSE;
                LookingForFirstQuote = FALSE;
                ASSERT(!QuoteTerminated);
                QuoteTerminated = TRUE;
                FirstQuoteEndOffset = ArgOffset;
            } else {

                //
                //  If we see a quote, either we're opening a section that
                //  belongs in one argument or we're ending that section.
                //

                QuoteOpen = (BOOLEAN)(!QuoteOpen);
                if (LookingForFirstQuote) {
                    Char.StartOfString++;
                    Char.LengthInChars--;
                    if (Char.LengthInChars == 0) {
                        if (!CurrentArgFound) {
                            CurrentArgFound = TRUE;
                            CmdContext->CurrentArg = ArgCount;
                            CmdContext->CurrentArgOffset = ArgOffset;
                        }
                        YoriLibParseMoveToNextArgumentInitialScan(CmdContext, CurrentArgFound, &ArgCount, &ArgOffset, &PreviousCharWasQuote, &QuoteTerminated, &FirstQuoteEndOffset);
                    }
                    continue;
                }
            }
        }

        //
        //  If no quote section is open and we see a space, it's time for a
        //  new argument.  If we see other common seperators, like "|" or ">",
        //  then we also need another argument.  Depending on the form this
        //  takes, that argument is either fully complete so we move ahead
        //  by two arguments, or it still has additional input following.
        //  (This is really because Yori expects ">file" to be one argument
        //  but "| program" to be two.)
        //

        TerminateArg = FALSE;
        if (!QuoteOpen) {
            if (Char.StartOfString[0] == ' ') {
                TerminateArg = TRUE;
                TerminateNextArg = FALSE;
                CharsToConsume = 0;
            } else if ((ArgCount > 0 || RequiredCharCount > 0) &&
                       YoriLibShIsArgumentSeperator(&Char, &CharsToConsume, &TerminateNextArg)) {
                TerminateArg = TRUE;
            }
        }

        if (TerminateArg) {

            if (Char.LengthInChars > 0) {
                YoriLibShTrimSpacesFromBeginning(&Char);
                if (Char.LengthInChars == 0) {
                    CmdContext->TrailingChars = TRUE;
                }
            }

            if (!CurrentArgFound &&
                (Char.StartOfString - CmdLine->StartOfString > (LONG)CurrentOffset)) {

                CurrentArgFound = TRUE;
                CmdContext->CurrentArg = ArgCount;
                CmdContext->CurrentArgOffset = ArgOffset;
            }

            YoriLibParseMoveToNextArgumentInitialScan(CmdContext, CurrentArgFound, &ArgCount, &ArgOffset, &PreviousCharWasQuote, &QuoteTerminated, &FirstQuoteEndOffset);

            //
            //  Note this is intentionally not trying to set CurrentArg.
            //  We may have just incremented ArgCount and end up setting
            //  CurrentArg below, ie., CurrentArg is beyond ArgCount.
            //  This is how "cd <tab>" works - one argument, but the current
            //  argument is the second one.
            //

            if (Char.LengthInChars == 0) {
                break;
            }

            //
            //  If we were processing a space but the next argument is a
            //  common seperator, see if it's self contained and we should
            //  move ahead one more argument.
            //

            if (CharsToConsume == 0) {
                YoriLibShIsArgumentSeperator(&Char, &CharsToConsume, &TerminateNextArg);
            }

            RequiredCharCount = RequiredCharCount + CharsToConsume;
            ArgOffset = ArgOffset + CharsToConsume;
            Char.StartOfString += CharsToConsume;
            Char.LengthInChars = Char.LengthInChars - CharsToConsume;

            if (Char.LengthInChars == 0) {
                if (!CurrentArgFound) {
                    CurrentArgFound = TRUE;
                    CmdContext->CurrentArg = ArgCount;
                    CmdContext->CurrentArgOffset = ArgOffset;
                }
                YoriLibParseMoveToNextArgumentInitialScan(CmdContext, CurrentArgFound, &ArgCount, &ArgOffset, &PreviousCharWasQuote, &QuoteTerminated, &FirstQuoteEndOffset);
                break;
            }

            if (TerminateNextArg) {
                RequiredCharCount++;
                ArgOffset++;

                YoriLibShTrimSpacesFromBeginning(&Char);

                if (!CurrentArgFound &&
                    (Char.StartOfString - CmdLine->StartOfString > (LONG)CurrentOffset)) {

                    CurrentArgFound = TRUE;
                    CmdContext->CurrentArg = ArgCount;
                    CmdContext->CurrentArgOffset = ArgOffset;
                }

                YoriLibParseMoveToNextArgumentInitialScan(CmdContext, CurrentArgFound, &ArgCount, &ArgOffset, &PreviousCharWasQuote, &QuoteTerminated, &FirstQuoteEndOffset);
            }

            if (Char.LengthInChars > 0 && Char.StartOfString[0] == '"') {
                LookingForFirstQuote = TRUE;
            } else {
                LookingForFirstQuote = FALSE;
            }

        } else {

            if (Char.StartOfString[0] == '"') {
                PreviousCharWasQuote = TRUE;
            } else {
                PreviousCharWasQuote = FALSE;
            }

            RequiredCharCount++;
            ArgOffset++;
            Char.StartOfString++;
            Char.LengthInChars--;

            if (!CurrentArgFound &&
                (Char.StartOfString - CmdLine->StartOfString >= (LONG)CurrentOffset)) {

                CurrentArgFound = TRUE;
                CmdContext->CurrentArg = ArgCount;
                CmdContext->CurrentArgOffset = ArgOffset;
            }

            //
            //  If we hit a break char, we count the argument then.
            //  If we hit end of string, count it here; note we're
            //  only counting it if we counted a character before it
            //  (ie., trailing whitespace is not an arg.)
            //

            if (Char.LengthInChars == 0) {
                if (!CurrentArgFound) {
                    CurrentArgFound = TRUE;
                    CmdContext->CurrentArg = ArgCount;
                    CmdContext->CurrentArgOffset = ArgOffset;
                }
                YoriLibParseMoveToNextArgumentInitialScan(CmdContext, CurrentArgFound, &ArgCount, &ArgOffset, &PreviousCharWasQuote, &QuoteTerminated, &FirstQuoteEndOffset);
            }
        }
    }

    RequiredCharCount++;
    ArgOffset++;

    if (!CurrentArgFound) {
        CurrentArgFound = TRUE;
        CmdContext->CurrentArg = ArgCount;
        CmdContext->CurrentArgOffset = ArgOffset;
    }

    CmdContext->ArgC = ArgCount;

    if (ArgCount == 0) {
        CmdContext->ArgV = NULL;
        CmdContext->ArgContexts = NULL;
        CmdContext->MemoryToFreeArgV = NULL;
        CmdContext->MemoryToFreeArgContexts = NULL;
        return TRUE;
    }

    if (!YoriLibShAllocateArgCount(CmdContext, ArgCount, (RequiredCharCount + ArgCount) * sizeof(TCHAR), (PVOID *)&OutputString)) {
        return FALSE;
    }

    __analysis_assume(CmdContext->ArgV != NULL);

    ArgCount = 0;
    QuoteOpen = FALSE;
    IsMultiCommandOperatorArgument = FALSE;
    YoriLibInitEmptyString(&CmdContext->ArgV[ArgCount]);
    CmdContext->ArgV[ArgCount].StartOfString = OutputString;
    CmdContext->ArgContexts[ArgCount].Quoted = FALSE;
    CmdContext->ArgContexts[ArgCount].QuoteTerminated = FALSE;
    YoriLibReference(CmdContext->ArgV);
    CmdContext->ArgV[ArgCount].MemoryToFree = CmdContext->ArgV;

    //
    //  Consume all spaces.  After this, we're either at
    //  the end of string, or we have an arg, and it
    //  might start with a quote
    //

    YoriLibInitEmptyString(&Char);
    Char.StartOfString = CmdLine->StartOfString;
    Char.LengthInChars = CmdLine->LengthInChars;

    YoriLibShTrimSpacesAndAtsFromBeginning(&Char);

    if (Char.LengthInChars > 0 && Char.StartOfString[0] == '"') {
        LookingForFirstQuote = TRUE;
        CmdContext->ArgContexts[ArgCount].Quoted = TRUE;
    } else {
        LookingForFirstQuote = FALSE;
    }
    FirstQuoteEndOffset = YORI_MAX_ALLOC_SIZE;
    PreviousCharWasQuote = FALSE;

    while (Char.LengthInChars > 0) {

        //
        //  If it's an escape char, consume two characters as literal until
        //  we hit the end of the string.  If the end of the string ends the
        //  argument and the argument is quoted, drop the escape so as to not
        //  escape the end quote.
        //

        if (YoriLibIsEscapeChar(Char.StartOfString[0])) {
            if (Char.LengthInChars == 1 && CmdContext->ArgContexts[ArgCount].QuoteTerminated) {
                Char.StartOfString++;
                Char.LengthInChars--;
            } else {
                PreviousCharWasQuote = FALSE;
                *OutputString = Char.StartOfString[0];
                Char.StartOfString++;
                Char.LengthInChars--;
                OutputString++;
                if (Char.LengthInChars > 0) {
                    *OutputString = Char.StartOfString[0];
                    Char.StartOfString++;
                    Char.LengthInChars--;
                    OutputString++;
                }
            }
            continue;
        }

        if (Char.StartOfString[0] == '\\') {

            YORI_ALLOC_SIZE_T CharsToOutput;

            PreviousCharWasQuote = FALSE;

            YoriLibShCountCharsAtBackslash(&Char,
                                           QuoteOpen,
                                           LookingForFirstQuote,
                                           CmdContext->ArgContexts[ArgCount].QuoteTerminated,
                                           &CharsToConsume,
                                           &CharsToOutput);

            if (CharsToConsume > CharsToOutput) {
                Char.StartOfString += (CharsToConsume - CharsToOutput);
                Char.LengthInChars -= (CharsToConsume - CharsToOutput);
                CharsToConsume = 0;
            } else if (CharsToOutput > CharsToConsume) {
                for (;CharsToOutput > CharsToConsume; CharsToOutput--) {
                    OutputString[0] = Char.StartOfString[0];
                    OutputString++;
                }
            }

            for (CharsToConsume = 0; CharsToConsume < CharsToOutput; CharsToConsume++) {
                OutputString[CharsToConsume] = Char.StartOfString[CharsToConsume];
            }

            OutputString += CharsToOutput;
            Char.StartOfString += CharsToOutput;
            Char.LengthInChars = Char.LengthInChars - CharsToOutput;

            continue;

        } else if (Char.StartOfString[0] == '"') {

            //
            //  If the argument started with a quote and we found the end
            //  to that quote, copy it into the output string, and make a
            //  note of where it is to facilitate later removal.  It will
            //  be removed if the argument doesn't end in a quote, which
            //  we don't know at this point.
            //

            if (QuoteOpen && LookingForFirstQuote) {
                QuoteOpen = FALSE;
                LookingForFirstQuote = FALSE;
                ASSERT(!CmdContext->ArgContexts[ArgCount].QuoteTerminated);
                CmdContext->ArgContexts[ArgCount].QuoteTerminated = TRUE;
                FirstQuoteEndOffset = (YORI_ALLOC_SIZE_T)(OutputString - CmdContext->ArgV[ArgCount].StartOfString);
            } else {

                //
                //  If we see a quote, either we're opening a section that
                //  belongs in one argument or we're ending that section.
                //

                QuoteOpen = (BOOLEAN)(!QuoteOpen);
                if (LookingForFirstQuote) {
                    Char.StartOfString++;
                    Char.LengthInChars--;
                    continue;
                }
            }
        }

        //
        //  If no quote section is open and we see a space, it's time for a
        //  new argument.  If we see other common seperators, like "|" or ">",
        //  then we also need another argument.  Depending on the form this
        //  takes, that argument is either fully complete so we move ahead
        //  by two arguments, or it still has additional input following.
        //  (This is really because Yori expects ">file" to be one argument
        //  but "| program" to be two.)
        //

        TerminateArg = FALSE;
        if (!QuoteOpen) {
            if (Char.StartOfString[0] == ' ') {
                TerminateArg = TRUE;
                TerminateNextArg = FALSE;
                CharsToConsume = 0;
            } else if ((ArgCount > 0 || (YORI_ALLOC_SIZE_T)(OutputString - CmdContext->ArgV[ArgCount].StartOfString) > 0) &&
                       YoriLibShIsArgumentSeperator(&Char, &CharsToConsume, &TerminateNextArg)) {
                TerminateArg = TRUE;
            }
        }

        if (TerminateArg) {

            YoriLibShTrimSpacesFromBeginning(&Char);
            YoriLibParseTerminateCurrentArgument(&OutputString, &CmdContext->ArgV[ArgCount], PreviousCharWasQuote, FirstQuoteEndOffset);

            if (Char.LengthInChars > 0) {
                YoriLibParseMoveToNextArgumentPopulate(CmdContext, &ArgCount, &Char, OutputString, &PreviousCharWasQuote, &LookingForFirstQuote, &FirstQuoteEndOffset);

                //
                //  If we were processing a space but the next argument is a
                //  common seperator, see if it's self contained and we should
                //  move ahead one more argument.
                //

                if (CharsToConsume == 0) {
                    if (!YoriLibShIsArgumentSeperator(&Char, &CharsToConsume, &TerminateNextArg)) {
                        CharsToConsume = 0;
                        TerminateNextArg = FALSE;
                    }
                }

                if (CharsToConsume > 0) {
                    memcpy(OutputString, Char.StartOfString, CharsToConsume * sizeof(TCHAR));
                    OutputString += CharsToConsume;
                    Char.StartOfString += CharsToConsume;
                    Char.LengthInChars = Char.LengthInChars - CharsToConsume;

                    //
                    //  Check for '>"file name"' type syntax.  This isn't a
                    //  fully quoted argument, but we do want to keep going
                    //  until the end of the second quote.  Since it's 
                    //  going to be consumed by the shell, we don't terribly
                    //  care that we can't round trip this information
                    //  perfectly back into a command string.
                    //

                    if (!TerminateNextArg) {
                        if (Char.LengthInChars > 0 && Char.StartOfString[0] == '"') {
                            LookingForFirstQuote = TRUE;
                        }
                    }
                }

                if (TerminateNextArg) {
                    YoriLibShTrimSpacesFromBeginning(&Char);
                    YoriLibParseTerminateCurrentArgument(&OutputString, &CmdContext->ArgV[ArgCount], PreviousCharWasQuote, FirstQuoteEndOffset);
                    if (Char.LengthInChars > 0) {
                        YoriLibParseMoveToNextArgumentPopulate(CmdContext, &ArgCount, &Char, OutputString, &PreviousCharWasQuote, &LookingForFirstQuote, &FirstQuoteEndOffset);
                    }
                }
            }
        } else {

            if (Char.StartOfString[0] == '"') {
                PreviousCharWasQuote = TRUE;
            } else {
                PreviousCharWasQuote = FALSE;
            }

            *OutputString = Char.StartOfString[0];
            OutputString++;
            Char.StartOfString++;
            Char.LengthInChars--;
        }
    }

    //
    //  If the argument hasn't already been terminated, terminate it now.
    //

    if (CmdContext->ArgV[ArgCount].LengthInChars == 0) {
        YoriLibParseTerminateCurrentArgument(&OutputString, &CmdContext->ArgV[ArgCount], PreviousCharWasQuote, FirstQuoteEndOffset);
    }

    return TRUE;
}

/**
 This routine is the inverse of @ref YoriLibShParseCmdlineToCmdContext.  It takes
 a series of arguments and reassembles them back into a single string.

 @param CmdContext Pointer to the command context to use to create the string.

 @param CmdLine On input, points to an initialized Yori string.  On output,
        updated to contain the constructed command line.  Note this string
        may be reallocated in this routine.

 @param RemoveEscapes If TRUE, escape characters should be removed from the
        command line.  This is done immediately before passing the string to
        an external program.  If FALSE, escapes are retained.  This is done
        when processing the string internally.

 @param BeginCurrentArg If non-NULL, will be populated with the offset in
        the string corresponding to the beginning of the current argument.

 @param EndCurrentArg If non-NULL, will be populated with the offset in
        the string corresponding to the end of the current argument.

 @return TRUE to indicate success, FALSE to indicate allocation failure.
 */
__success(return)
BOOLEAN
YoriLibShBuildCmdlineFromCmdContext(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __inout PYORI_STRING CmdLine,
    __in BOOL RemoveEscapes,
    __out_opt PYORI_ALLOC_SIZE_T BeginCurrentArg,
    __out_opt PYORI_ALLOC_SIZE_T EndCurrentArg
    )
{
    YORI_ALLOC_SIZE_T count;
    YORI_ALLOC_SIZE_T BufferLength = 0;
    YORI_ALLOC_SIZE_T CmdLineOffset = 0;
    LPTSTR String;
    PYORI_STRING ThisArg;
    YORI_ALLOC_SIZE_T SrcOffset;
    YORI_ALLOC_SIZE_T DestOffset;

    for (count = 0; count < CmdContext->ArgC; count++) {
        BufferLength += 1;
        if (CmdContext->ArgContexts[count].Quoted) {
            BufferLength += 2;
        }
        ThisArg = &CmdContext->ArgV[count];
        BufferLength = BufferLength + ThisArg->LengthInChars;
    }

    BufferLength += 1;

    if (CmdLine->LengthAllocated < BufferLength) {
        if (!YoriLibReallocStringNoContents(CmdLine, BufferLength)) {
            return FALSE;
        }
    }

    if (BeginCurrentArg != NULL) {
        *BeginCurrentArg = 0;
    }

    if (EndCurrentArg != NULL) {
        *EndCurrentArg = 0;
    }

    String = CmdLine->StartOfString;
    String[0] = '\0';

    for (count = 0; count < CmdContext->ArgC; count++) {
        ThisArg = &CmdContext->ArgV[count];

        if (count != 0) {
            String[CmdLineOffset++] = ' ';
        }

        if (count == CmdContext->CurrentArg && BeginCurrentArg != NULL) {
            *BeginCurrentArg = CmdLineOffset;
        }

        if (CmdContext->ArgContexts[count].Quoted) {
            String[CmdLineOffset++] = '"';
        }

        for (SrcOffset = DestOffset = 0; SrcOffset < ThisArg->LengthInChars; SrcOffset++, DestOffset++) {
            if (RemoveEscapes && YoriLibIsEscapeChar(ThisArg->StartOfString[SrcOffset])) {
                SrcOffset++;
                if (SrcOffset < ThisArg->LengthInChars) {
                    String[CmdLineOffset + DestOffset] = ThisArg->StartOfString[SrcOffset];
                } else {
                    break;
                }
            } else {
                String[CmdLineOffset + DestOffset] = ThisArg->StartOfString[SrcOffset];
            }
        }
        CmdLineOffset = CmdLineOffset + DestOffset;

        if (CmdContext->ArgContexts[count].Quoted) {
            if (CmdContext->ArgContexts[count].QuoteTerminated) {
                String[CmdLineOffset++] = '"';
            } else {
                ASSERT(count == CmdContext->ArgC - 1);
            }
        }

        if (count == CmdContext->CurrentArg && EndCurrentArg != NULL) {
            *EndCurrentArg = CmdLineOffset - 1;
        }
    }

    ASSERT(CmdLineOffset < CmdLine->LengthAllocated);

    String[CmdLineOffset] = '\0';

    CmdLine->LengthInChars = CmdLineOffset;
    return TRUE;
}

/**
 Remove escapes from a ArgC/ArgV array.  This operates on the inputs with
 no allocations or copies.

 @param ArgC Specifies the number of arguments.

 @param ArgV Pointer to an array of arguments.

 @return TRUE to indicate all escapes were removed, FALSE if not all could
         be successfully processed.
 */
__success(return)
BOOLEAN
YoriLibShRemoveEscapesFromArgCArgV(
    __in YORI_ALLOC_SIZE_T ArgC,
    __inout PYORI_STRING ArgV
    )
{
    YORI_ALLOC_SIZE_T ArgIndex;
    YORI_ALLOC_SIZE_T CharIndex;
    YORI_ALLOC_SIZE_T DestIndex;
    BOOLEAN EscapeFound;
    PYORI_STRING ThisArg;

    for (ArgIndex = 0; ArgIndex < ArgC; ArgIndex++) {
        ThisArg = &ArgV[ArgIndex];

        EscapeFound = FALSE;

        //
        //  Keep looping to the end of each argument.  This does two
        //  things: we have to search looking for an escape until we
        //  find one, and if we do find one, we need to know the
        //  length of the string.
        //

        for (CharIndex = 0; CharIndex < ThisArg->LengthInChars; CharIndex++) {
            if (YoriLibIsEscapeChar(ThisArg->StartOfString[CharIndex])) {
                EscapeFound = TRUE;
                break;
            }
        }

        if (EscapeFound) {
            YORI_STRING NewArg;

            if (!YoriLibAllocateString(&NewArg, ThisArg->LengthInChars + 1)) {
                return FALSE;
            }

            for (CharIndex = 0, DestIndex = 0; CharIndex < ThisArg->LengthInChars; CharIndex++, DestIndex++) {
                if (YoriLibIsEscapeChar(ThisArg->StartOfString[CharIndex])) {
                    CharIndex++;
                    if (CharIndex >= ThisArg->LengthInChars) {
                        break;
                    }
                }

                NewArg.StartOfString[DestIndex] = ThisArg->StartOfString[CharIndex];
            }
            NewArg.StartOfString[DestIndex] = '\0';
            NewArg.LengthInChars = DestIndex;

            YoriLibFreeStringContents(&ArgV[ArgIndex]);

            memcpy(&ArgV[ArgIndex], &NewArg, sizeof(YORI_STRING));
        }
    }

    return TRUE;
}

/**
 Take a command argument from one command context and "copy" it to another.
 Because memory is reference counted, this typically means copy a pointer
 and reference it.  This function is responsible for migrating the
 ArgContext state accurately across the copy.

 @param SrcCmdContext The command context to copy an argument from.

 @param SrcArgument The index of the argument to copy.

 @param DestCmdContext The command context to copy the argument to.

 @param DestArgument The index of the argument to populate.
 */
VOID
YoriLibShCopyArg(
    __in PYORI_LIBSH_CMD_CONTEXT SrcCmdContext,
    __in YORI_ALLOC_SIZE_T SrcArgument,
    __in PYORI_LIBSH_CMD_CONTEXT DestCmdContext,
    __in YORI_ALLOC_SIZE_T DestArgument
    )
{
    DestCmdContext->ArgContexts[DestArgument].Quoted = SrcCmdContext->ArgContexts[SrcArgument].Quoted;
    DestCmdContext->ArgContexts[DestArgument].QuoteTerminated = SrcCmdContext->ArgContexts[SrcArgument].QuoteTerminated;
    if (SrcCmdContext->ArgV[SrcArgument].MemoryToFree != NULL) {
        YoriLibReference(SrcCmdContext->ArgV[SrcArgument].MemoryToFree);
    }
    memcpy(&DestCmdContext->ArgV[DestArgument], &SrcCmdContext->ArgV[SrcArgument], sizeof(YORI_STRING));
}

/**
 Perform a deep copy of a command context.  This will allocate a new argument
 array but reference any arguments from the source (so they must still be
 reallocated individually if/when modified.)

 @param DestCmdContext Pointer to the command context to populate with contents
        from the source.

 @param SrcCmdContext Pointer to the source command context.

 @return TRUE to indicate success, or FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibShCopyCmdContext(
    __out PYORI_LIBSH_CMD_CONTEXT DestCmdContext,
    __in PYORI_LIBSH_CMD_CONTEXT SrcCmdContext
    )
{
    YORI_ALLOC_SIZE_T Count;

    if (!YoriLibShAllocateArgCount(DestCmdContext, SrcCmdContext->ArgC, 0, NULL)) {
        return FALSE;
    }

    DestCmdContext->ArgC = SrcCmdContext->ArgC;
    DestCmdContext->CurrentArg = SrcCmdContext->CurrentArg;
    DestCmdContext->CurrentArgOffset = SrcCmdContext->CurrentArgOffset;

    for (Count = 0; Count < DestCmdContext->ArgC; Count++) {
        YoriLibShCopyArg(SrcCmdContext, Count, DestCmdContext, Count);
    }

    return TRUE;
}

/**
 Add extra arguments into a CmdContext.  This routine can reallocate the
 ArgV and ArgContexts arrays to the specified size.

 @param CmdContext Pointer to the command context to expand.

 @param NewArgOffset Indicates the insertion point of any new argument. The
        newly allocated argument at this offset will be empty, and any
        existing argument at this offset will be moved.

 @param NewArgCount Specifies the new number of arguments to insert.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibShExpandCmdContext(
    __inout PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __in DWORD NewArgOffset,
    __in DWORD NewArgCount
    )
{
    YORI_ALLOC_SIZE_T Count;
    YORI_LIBSH_CMD_CONTEXT OldCmdContext;
    DWORD NewArgC;

    ZeroMemory(&OldCmdContext, sizeof(OldCmdContext));

    OldCmdContext.ArgC = CmdContext->ArgC;
    OldCmdContext.ArgV = CmdContext->ArgV;
    OldCmdContext.ArgContexts = CmdContext->ArgContexts;
    OldCmdContext.MemoryToFreeArgV = CmdContext->MemoryToFreeArgV;
    OldCmdContext.MemoryToFreeArgContexts = CmdContext->MemoryToFreeArgContexts;

    NewArgC = CmdContext->ArgC + NewArgCount;

    CmdContext->ArgV = NULL;
    CmdContext->ArgContexts = NULL;
    CmdContext->MemoryToFreeArgV = NULL;
    CmdContext->MemoryToFreeArgContexts = NULL;

    if (!YoriLibShAllocateArgCount(CmdContext, NewArgC, 0, NULL)) {
        CmdContext->ArgV = OldCmdContext.ArgV;
        CmdContext->ArgContexts = OldCmdContext.ArgContexts;
        CmdContext->MemoryToFreeArgV = OldCmdContext.MemoryToFreeArgV;
        CmdContext->MemoryToFreeArgContexts = OldCmdContext.MemoryToFreeArgContexts;
        return FALSE;
    }

    if (CmdContext->CurrentArg >= NewArgOffset) {
        CmdContext->CurrentArg = CmdContext->CurrentArg + NewArgCount;
    }

    for (Count = 0; Count < OldCmdContext.ArgC; Count++) {
        if (Count >= NewArgOffset) {
            YoriLibShCopyArg(&OldCmdContext, Count, CmdContext, Count + NewArgOffset);
        } else {
            YoriLibShCopyArg(&OldCmdContext, Count, CmdContext, Count);
        }
    }

    YoriLibShFreeCmdContext(&OldCmdContext);

    return TRUE;
}

/**
 Check if an argument contains spaces and now requires quoting.  Previously
 quoted arguments retain quotes.  This function is used when the contents of
 an argument have changed such as via tab completion.  If the argument
 requires quoting, the CmdContext is updated to indicate as much.

 @param CmdContext The command context containing the argument to check.

 @param ArgIndex The index of the argument within the command context to
        check.
 */
VOID
YoriLibShCheckIfArgNeedsQuotes(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __in YORI_ALLOC_SIZE_T ArgIndex
    )
{
    BOOL HasWhiteSpace;

    HasWhiteSpace = YoriLibCheckIfArgNeedsQuotes(&CmdContext->ArgV[ArgIndex]);
    if (HasWhiteSpace) {
        CmdContext->ArgContexts[ArgIndex].Quoted = TRUE;
        CmdContext->ArgContexts[ArgIndex].QuoteTerminated = TRUE;
    }
}

/**
 Free the contents of a @ref YORI_LIBSH_CMD_CONTEXT .  The allocation containing
 the context is not freed, since that context is often on the stack or in
 another structure, and it is better left to the caller to clean up.

 @param CmdContext Pointer to the @ref YORI_LIBSH_CMD_CONTEXT to free.
 */
VOID
YoriLibShFreeCmdContext(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext
    )
{
    YORI_ALLOC_SIZE_T Count;

    if (CmdContext->ArgV != NULL) {
        for (Count = 0; Count < CmdContext->ArgC; Count++) {
            YoriLibFreeStringContents(&CmdContext->ArgV[Count]);
        }
    }

    if (CmdContext->MemoryToFreeArgV) {
        YoriLibDereference(CmdContext->MemoryToFreeArgV);
        CmdContext->MemoryToFreeArgV = NULL;
    }

    if (CmdContext->MemoryToFreeArgContexts) {
        YoriLibDereference(CmdContext->MemoryToFreeArgContexts);
        CmdContext->MemoryToFreeArgContexts = NULL;
    }
}

/**
 Clean up any currently existing StdIn information in an ExecContext.

 @param ExecContext the context to clean up.
 */
VOID
YoriLibShExecContextCleanupStdIn(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    switch(ExecContext->StdInType) {
        case StdInTypePipe:
            if (ExecContext->StdIn.Pipe.PipeFromPriorProcess != NULL) {
                CloseHandle(ExecContext->StdIn.Pipe.PipeFromPriorProcess);
                ExecContext->StdIn.Pipe.PipeFromPriorProcess = NULL;
            }
            break;
        case StdInTypeFile:
            YoriLibFreeStringContents(&ExecContext->StdIn.File.FileName);
            break;
    }
    ExecContext->StdInType = StdInTypeDefault;
}

/**
 Clean up any currently existing StdOut information in an ExecContext.

 @param ExecContext the context to clean up.
 */
VOID
YoriLibShExecContextCleanupStdOut(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    switch(ExecContext->StdOutType) {
        case StdOutTypeOverwrite:
            YoriLibFreeStringContents(&ExecContext->StdOut.Overwrite.FileName);
            break;
        case StdOutTypeAppend:
            YoriLibFreeStringContents(&ExecContext->StdOut.Append.FileName);
            break;
        case StdOutTypeBuffer:
            if (ExecContext->StdOut.Buffer.PipeFromProcess != NULL) {
                CloseHandle(ExecContext->StdOut.Buffer.PipeFromProcess);
                ExecContext->StdOut.Buffer.PipeFromProcess = NULL;
            }
            if (ExecContext->StdOut.Buffer.ProcessBuffers != NULL) {
                YoriLibShDereferenceProcessBuffer(ExecContext->StdOut.Buffer.ProcessBuffers);
                ExecContext->StdOut.Buffer.ProcessBuffers = NULL;
            }
            break;
    }
    ExecContext->StdOutType = StdOutTypeDefault;
}

/**
 Clean up any currently existing StdErr information in an ExecContext.

 @param ExecContext the context to clean up.
 */
VOID
YoriLibShExecContextCleanupStdErr(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    switch(ExecContext->StdErrType) {
        case StdErrTypeOverwrite:
            YoriLibFreeStringContents(&ExecContext->StdErr.Overwrite.FileName);
            break;
        case StdErrTypeAppend:
            YoriLibFreeStringContents(&ExecContext->StdErr.Append.FileName);
            break;
    }
    ExecContext->StdErrType = StdErrTypeDefault;
}


/**
 Return TRUE if the argument is a seperator between different programs,
 FALSE if it is part of the same program's arguments.

 @param Arg Pointer to the argument to check.

 @param EndOfExpression TRUE if the argument is the final argument in the
        expression, FALSE if it is an intermediate argument.

 @return TRUE to indicate this argument is a seperator between programs.
 */
BOOL
YoriLibShIsArgumentProgramSeperator(
    __in PYORI_STRING Arg,
    __in BOOL EndOfExpression
    )
{
    if (YoriLibCompareStringLitIns(Arg, _T("&")) == 0 ||
        YoriLibCompareStringLitIns(Arg, _T("&&")) == 0 ||
        YoriLibCompareStringLitIns(Arg, _T("\n")) == 0 ||
        YoriLibCompareStringLitIns(Arg, _T("|")) == 0 ||
        YoriLibCompareStringLitIns(Arg, _T("||")) == 0) {

        return TRUE;
    }

    if (EndOfExpression) {
        if (YoriLibCompareStringLitIns(Arg, _T("&!")) == 0 ||
            YoriLibCompareStringLitIns(Arg, _T("&!!")) == 0) {

            return TRUE;
        }
    }
    return FALSE;
}

/**
 Check if a given name is a DOS device name.  If it is, retain the name since
 the intention is to talk to the device, and reference it for use in a new
 environment.  If it is not a DOS device name, resolve it to a fully qualified
 escaped path, and reference the new allocation.  The caller can then use the
 two interchangably since they are semantically identical.

 @param UserString Pointer to the user specified command parameter.
 
 @param UserStringOffset The offset, in characters, within the argument
        parameter to the file name.

 @param ResolvedName On successful completion, updated to point to the new,
        possibly expanded name.  The memory underlying this has been
        referenced, regardless of source.

 @return TRUE to indicate success, FALSE to indicate allocation failure.
 */
__success(return)
BOOL
YoriLibShCheckForDeviceNameAndDuplicate(
    __inout PYORI_STRING UserString,
    __in YORI_ALLOC_SIZE_T UserStringOffset,
    __inout PYORI_STRING ResolvedName
    )
{

    YORI_STRING UserName;

    ASSERT(UserString->LengthInChars >= UserStringOffset);

    YoriLibInitEmptyString(&UserName);
    UserName.StartOfString = &UserString->StartOfString[UserStringOffset];
    UserName.LengthInChars = UserString->LengthInChars - UserStringOffset;
    UserName.LengthAllocated = UserString->LengthAllocated - UserStringOffset;
    UserName.MemoryToFree = UserString->MemoryToFree;

    //
    //  If the user doesn't provide a name with this argument, then we're
    //  going to attempt to use an empty file name and fail.
    //

    if (UserName.LengthInChars == 0 ||
        YoriLibIsFileNameDeviceName(&UserName)) {

        if (UserName.MemoryToFree != NULL) {
            YoriLibReference(UserName.MemoryToFree);
        }
        memcpy(ResolvedName, &UserName, sizeof(YORI_STRING));

    } else {
        return YoriLibUserToSingleFilePath(&UserName, TRUE, ResolvedName);
    }
    return TRUE;
}

/**
 Parse a series of raw arguments into information about how to execute a
 single program, and return the number of arguments consumed.  This
 function takes care of identifying things like arguments seperating
 different programs, as well as redirection information for the
 program being parsed.

 @param CmdContext Pointer to a raw series of arguments to parse for
        and individual program's execution.

 @param InitialArgument Specifies the first argument to be used to
        generate information for this program.

 @param ExecContext Is populated with information about how to execute a
        single program.

 @param CurrentArgIsForProgram If specified, this routine populates this value
        with TRUE if the current argument in the command context has become a
        parameter for the current command (as opposed to a seperator or
        redirection.)

 @param CurrentArgIndex If specified, this routine populates this value with
        the index within the CurrentExecContext of the current argument in the
        command context.  This is only meaningful if CurrentArgIsForProgram
        is TRUE.

 @param CurrentArgOffset If specified, this routine populates this value with
        the character offset within the current argument for the cursor
        location.

 @return The number of arguments consumed while creating information about
         how to execute a single program.
 */
__success(return > 0)
YORI_ALLOC_SIZE_T
YoriLibShParseCmdContextToExecContext(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __in YORI_ALLOC_SIZE_T InitialArgument,
    __out PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __out_opt PBOOLEAN CurrentArgIsForProgram,
    __out_opt PYORI_ALLOC_SIZE_T CurrentArgIndex,
    __out_opt PYORI_ALLOC_SIZE_T CurrentArgOffset
    )
{
    YORI_ALLOC_SIZE_T Count;
    BOOLEAN RemoveThisArg;
    BOOLEAN EndOfExpression;
    YORI_ALLOC_SIZE_T ArgumentsConsumed = 0;
    YORI_ALLOC_SIZE_T CharOffset;
    PYORI_STRING ThisArg;
    PYORI_STRING ExecContextRedirectString;

    if (CurrentArgIsForProgram != NULL) {
        *CurrentArgIsForProgram = FALSE;
    }

    if (CurrentArgIndex != NULL) {
        *CurrentArgIndex = 0;
    }

    if (CurrentArgOffset != NULL) {
        *CurrentArgOffset = 0;
    }

    ZeroMemory(ExecContext, sizeof(YORI_LIBSH_SINGLE_EXEC_CONTEXT));
    ExecContext->ReferenceCount = 1;
    ExecContext->StdInType = StdInTypeDefault;
    ExecContext->StdOutType = StdOutTypeDefault;
    ExecContext->StdErrType = StdErrTypeDefault;
    ExecContext->WaitForCompletion = TRUE;
    ExecContext->RunOnSecondConsole = FALSE;
    ExecContext->TaskCompletionDisplayed = FALSE;
    ExecContext->SuppressTaskCompletion = FALSE;
    ExecContext->TerminateGracefully = FALSE;
    YoriLibInitializeListHead(&ExecContext->DebuggedChildren);

    //
    //  First, count the number of arguments that will be consumed by this
    //  program.
    //

    for (Count = InitialArgument; Count < CmdContext->ArgC; Count++) {
        EndOfExpression = FALSE;
        if (Count == CmdContext->ArgC - 1) {
            EndOfExpression = TRUE;
        }
        if (!CmdContext->ArgContexts[Count].Quoted &&
            YoriLibShIsArgumentProgramSeperator(&CmdContext->ArgV[Count], EndOfExpression)) {
            break;
        }
    }

    ArgumentsConsumed = Count - InitialArgument;

    if (!YoriLibShAllocateArgCount(&ExecContext->CmdToExec, ArgumentsConsumed, 0, NULL)) {
        return 0;
    }
    ExecContext->CmdToExec.ArgC = 0;

    for (Count = InitialArgument; Count < (InitialArgument + ArgumentsConsumed); Count++) {

        RemoveThisArg = FALSE;
        ThisArg = &CmdContext->ArgV[Count];
        ExecContextRedirectString = NULL;
        CharOffset = 0;

        //
        //  When parsing the CmdContext, any argument starting with a
        //  quote is not a candidate to be a redirect.  However, it is
        //  valid to have a redirect followed by a file name in quotes,
        //  in which case the argument isn't considered "quoted" but
        //  can still contain spaces.
        //

        if (!CmdContext->ArgContexts[Count].Quoted) {

            if (ThisArg->StartOfString[0] == '<') {
                YoriLibShExecContextCleanupStdIn(ExecContext);
                ExecContext->StdInType = StdInTypeFile;
                YoriLibInitEmptyString(&ExecContext->StdIn.File.FileName);
                CharOffset = 1;
                ExecContextRedirectString = &ExecContext->StdIn.File.FileName;
                RemoveThisArg = TRUE;
            }

            if (ThisArg->StartOfString[0] == '>') {
                YoriLibShExecContextCleanupStdOut(ExecContext);
                if (ThisArg->StartOfString[1] == '>') {
                    ExecContext->StdOutType = StdOutTypeAppend;
                    CharOffset = 2;
                    ExecContextRedirectString = &ExecContext->StdOut.Append.FileName;
                } else if (ThisArg->StartOfString[1] == '&') {
                    if (ThisArg->StartOfString[2] == '2') {
                        ExecContext->StdOutType = StdOutTypeStdErr;
                        if (ExecContext->StdErrType == StdErrTypeStdOut) {
                            ExecContext->StdErrType = StdErrTypeDefault;
                        }
                    }
                } else {
                    ExecContext->StdOutType = StdOutTypeOverwrite;
                    CharOffset = 1;
                    ExecContextRedirectString = &ExecContext->StdOut.Overwrite.FileName;
                }
                RemoveThisArg = TRUE;
            }

            if (ThisArg->StartOfString[0] == '1') {
                if (ThisArg->StartOfString[1] == '>') {
                    YoriLibShExecContextCleanupStdOut(ExecContext);
                    if (ThisArg->StartOfString[2] == '>') {
                        ExecContext->StdOutType = StdOutTypeAppend;
                        CharOffset = 3;
                        ExecContextRedirectString = &ExecContext->StdOut.Append.FileName;
                    } else if (ThisArg->StartOfString[2] == '&') {
                        if (ThisArg->StartOfString[3] == '2') {
                            ExecContext->StdOutType = StdOutTypeStdErr;
                            if (ExecContext->StdErrType == StdErrTypeStdOut) {
                                ExecContext->StdErrType = StdErrTypeDefault;
                            }
                        }
                    } else {
                        ExecContext->StdOutType = StdOutTypeOverwrite;
                        CharOffset = 2;
                        ExecContextRedirectString = &ExecContext->StdOut.Overwrite.FileName;
                    }
                    RemoveThisArg = TRUE;
                }
            }

            if (ThisArg->StartOfString[0] == '2') {
                if (ThisArg->StartOfString[1] == '>') {
                    YoriLibShExecContextCleanupStdErr(ExecContext);
                    ExecContext->StdErrType = StdErrTypeDefault;
                    if (ThisArg->StartOfString[2] == '>') {
                        ExecContext->StdErrType = StdErrTypeAppend;
                        CharOffset = 3;
                        ExecContextRedirectString = &ExecContext->StdErr.Append.FileName;
                    } else if (ThisArg->StartOfString[2] == '&') {
                        if (ThisArg->StartOfString[3] == '1') {
                            ExecContext->StdErrType = StdErrTypeStdOut;
                            if (ExecContext->StdOutType == StdOutTypeStdErr) {
                                ExecContext->StdOutType = StdOutTypeDefault;
                            }
                        }
                    } else {
                        ExecContext->StdErrType = StdErrTypeOverwrite;
                        CharOffset = 2;
                        ExecContextRedirectString = &ExecContext->StdErr.Overwrite.FileName;
                    }
                    RemoveThisArg = TRUE;
                }
            }
        }

        //
        //  If this is a redirect, populate the remainder of the argument, or
        //  the next argument if the remainder is empty, into the appropriate
        //  redirect field.  Note this will increment Count to skip arguments
        //  as needed.
        //

        if (ExecContextRedirectString) {
            while (ThisArg->LengthInChars == CharOffset &&
                   (Count + 1) < (InitialArgument + ArgumentsConsumed)) {

                Count++;
                ThisArg = &CmdContext->ArgV[Count];
                CharOffset = 0;
            }

            YoriLibShCheckForDeviceNameAndDuplicate(ThisArg,
                                                 CharOffset,
                                                 ExecContextRedirectString);
        }

        if (!RemoveThisArg) {
            PYORI_LIBSH_CMD_CONTEXT CmdToExec = &ExecContext->CmdToExec;
            YoriLibShCopyArg(CmdContext, Count, CmdToExec, CmdToExec->ArgC);
            if (CurrentArgIsForProgram != NULL) {
                if (CmdContext->CurrentArg == Count) {
                    *CurrentArgIsForProgram = TRUE;
                    if (CurrentArgIndex != NULL) {
                        *CurrentArgIndex = CmdToExec->ArgC;
                    }
                    if (CurrentArgOffset != NULL) {
                        *CurrentArgOffset = CmdContext->CurrentArgOffset;
                    }
                }
            }
            CmdToExec->ArgC++;
        } else if (CurrentArgOffset != NULL) {
            if (CmdContext->CurrentArg == Count) {
                *CurrentArgOffset = CmdContext->CurrentArgOffset;
            }
        }
    }

    return ArgumentsConsumed;
}

/**
 Frees any internal allocations in a @ref YORI_LIBSH_SINGLE_EXEC_CONTEXT .  Note
 it does not free the context itself, which is the caller's responsibility.
 This is done because it's frequently convenient to have context on the
 stack.

 @param ExecContext Pointer to the single execution context to free.
 */
VOID
YoriLibShFreeExecContext(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIBSH_DEBUGGED_CHILD_PROCESS DebuggedChild;

    ASSERT(ExecContext->ReferenceCount == 0);

    //
    //  If the process was being debugged, the debugger thread should
    //  have torn down before we tear down the context it uses.
    //

    if (ExecContext->hDebuggerThread != NULL) {
        ASSERT(WaitForSingleObject(ExecContext->hDebuggerThread, 0) == WAIT_OBJECT_0 || ExecContext->DebugPumpThreadFinished);
        CloseHandle(ExecContext->hDebuggerThread);
        ExecContext->hDebuggerThread = NULL;
    }

    //
    //  Free any ancestor processes that are being tracked by the
    //  debugger.
    //

    ListEntry = NULL;
    while (TRUE) {
        ListEntry = YoriLibGetNextListEntry(&ExecContext->DebuggedChildren, ListEntry);
        if (ListEntry == NULL) {
            break;
        }

        DebuggedChild = CONTAINING_RECORD(ListEntry, YORI_LIBSH_DEBUGGED_CHILD_PROCESS, ListEntry);
        YoriLibRemoveListItem(&DebuggedChild->ListEntry);
        if (DebuggedChild->hProcess != NULL) {
            CloseHandle(DebuggedChild->hProcess);
        }
        if (DebuggedChild->hInitialThread != NULL) {
            CloseHandle(DebuggedChild->hInitialThread);
        }
        YoriLibDereference(DebuggedChild);
        ListEntry = NULL;
    }

    YoriLibShFreeCmdContext(&ExecContext->CmdToExec);

    YoriLibShExecContextCleanupStdIn(ExecContext);
    YoriLibShExecContextCleanupStdOut(ExecContext);
    YoriLibShExecContextCleanupStdErr(ExecContext);
    if (ExecContext->hProcess != NULL) {
        CloseHandle(ExecContext->hProcess);
        ExecContext->hProcess = NULL;
    }
    if (ExecContext->hPrimaryThread != NULL) {
        CloseHandle(ExecContext->hPrimaryThread);
        ExecContext->hPrimaryThread = NULL;
    }
}

/**
 Add a reference to a single exec context.

 @param ExecContext Pointer to the exec context to reference.
 */
VOID
YoriLibShReferenceExecContext(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext
    )
{
    ASSERT(ExecContext->ReferenceCount > 0);
    InterlockedIncrement((LONG *)&ExecContext->ReferenceCount);
}

/**
 Dereference a single exec context.

 @param ExecContext Pointer to the exec context to dereference.

 @param Deallocate If TRUE, the ExecContext memory should be freed on
        dereference to zero.  If FALSE, the structure should be cleaned
        up but the memory should remain allocated.
 */
VOID
YoriLibShDereferenceExecContext(
    __in PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext,
    __in BOOLEAN Deallocate
    )
{
    ASSERT(ExecContext->ReferenceCount > 0);
    if (InterlockedDecrement((LONG *)&ExecContext->ReferenceCount) == 0) {
        YoriLibShFreeExecContext(ExecContext);
        if (Deallocate) {
            YoriLibFree(ExecContext);
        }
    }
}


/**
 Frees any internal allocations in a @ref YORI_LIBSH_EXEC_PLAN .  Note it
 does not free the plan itself, which is the caller's responsibility.
 This is done because it's frequently convenient to have plan on the
 stack.

 @param ExecPlan Pointer to the execution plan to free.
 */
VOID
YoriLibShFreeExecPlan(
    __in PYORI_LIBSH_EXEC_PLAN ExecPlan
    )
{
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ExecContext;
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT NextExecContext;

    ExecContext = ExecPlan->FirstCmd;

    while (ExecContext != NULL) {
        NextExecContext = ExecContext->NextProgram;
        ExecContext->NextProgram = NULL;
        YoriLibShDereferenceExecContext(ExecContext, TRUE);
        ExecContext = NextExecContext;
    }

    YoriLibShDereferenceExecContext(&ExecPlan->EntireCmd, FALSE);
}

/**
 Parse a series of raw arguments into information about how to execute a
 set of programs.

 @param CmdContext Pointer to a raw series of arguments to parse for
        a set of program's execution.

 @param ExecPlan Is populated with information about how to execute a
        series of programs.

 @param CurrentExecContext If specified, this routine populated this value
        with which single exec context is the one that processed the current
        argument in the command context.

 @param CurrentArgIsForProgram If specified, this routine populates this value
        with TRUE if the current argument in the command context has become a
        parameter for the current command (as opposed to a seperator or
        redirection.)

 @param CurrentArgIndex If specified, this routine populates this value with
        the index within the CurrentExecContext of the current argument in the
        command context.  This is only meaningful if CurrentArgIsForProgram
        is TRUE.

 @param CurrentArgOffset If specified, this routine populates this value with
        the character offset within the current argument for the cursor
        location.

 @return TRUE to indicate parsing success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibShParseCmdContextToExecPlan(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __out PYORI_LIBSH_EXEC_PLAN ExecPlan,
    __out_opt PYORI_LIBSH_SINGLE_EXEC_CONTEXT* CurrentExecContext,
    __out_opt PBOOLEAN CurrentArgIsForProgram,
    __out_opt PYORI_ALLOC_SIZE_T CurrentArgIndex,
    __out_opt PYORI_ALLOC_SIZE_T CurrentArgOffset
    )
{
    YORI_ALLOC_SIZE_T CurrentArg = 0;
    YORI_ALLOC_SIZE_T ArgsConsumed;
    YORI_ALLOC_SIZE_T ArgOfLastOperatorIndex = 0;
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT ThisProgram;
    PYORI_LIBSH_SINGLE_EXEC_CONTEXT PreviousProgram = NULL;
    BOOLEAN LocalCurrentArgIsForProgram;
    BOOLEAN FoundProgramMatch;
    YORI_ALLOC_SIZE_T LocalCurrentArgIndex;
    YORI_ALLOC_SIZE_T LocalCurrentArgOffset;

    if (CmdContext->ArgC == 0) {
        return FALSE;
    }

    if (CurrentExecContext != NULL) {
        *CurrentExecContext = NULL;
    }

    if (CurrentArgIsForProgram != NULL) {
        *CurrentArgIsForProgram = FALSE;
    }

    if (CurrentArgIndex != NULL) {
        *CurrentArgIndex = 0;
    }

    if (CurrentArgOffset != NULL) {
        *CurrentArgOffset = 0;
    }

    ZeroMemory(ExecPlan, sizeof(YORI_LIBSH_EXEC_PLAN));
    FoundProgramMatch = FALSE;

    //
    //  First, turn the entire CmdContext into an ExecContext.
    //

    if (!YoriLibShCopyCmdContext(&ExecPlan->EntireCmd.CmdToExec, CmdContext)) {
        YoriLibShFreeExecPlan(ExecPlan);
        return FALSE;
    }
    ExecPlan->EntireCmd.WaitForCompletion = TRUE;
    ExecPlan->EntireCmd.ReferenceCount = 1;
    ExecPlan->WaitForCompletion = TRUE;

    while (CurrentArg < CmdContext->ArgC) {

        ThisProgram = YoriLibMalloc(sizeof(YORI_LIBSH_SINGLE_EXEC_CONTEXT));
        if (ThisProgram == NULL) {
            YoriLibShFreeExecPlan(ExecPlan);
            return FALSE;
        }

        ArgsConsumed = YoriLibShParseCmdContextToExecContext(CmdContext, CurrentArg, ThisProgram, &LocalCurrentArgIsForProgram, &LocalCurrentArgIndex, &LocalCurrentArgOffset);
        if (ArgsConsumed == 0) {
            YoriLibShDereferenceExecContext(ThisProgram, TRUE);
            YoriLibShFreeExecPlan(ExecPlan);
            return FALSE;
        }

        if (CurrentArg + ArgsConsumed == (CmdContext->ArgC - 1)) {
            PYORI_STRING ThisArg;
            ThisArg = &CmdContext->ArgV[CurrentArg + ArgsConsumed];
            if (!CmdContext->ArgContexts[CurrentArg + ArgsConsumed].Quoted &&
                ThisArg->StartOfString[0] == '&') {

                if (YoriLibCompareStringLit(ThisArg, _T("&")) == 0) {
                    ExecPlan->WaitForCompletion = FALSE;
                    ExecPlan->EntireCmd.WaitForCompletion = FALSE;

                    ThisProgram->WaitForCompletion = FALSE;

                    YoriLibFreeStringContents(&ExecPlan->EntireCmd.CmdToExec.ArgV[CurrentArg + ArgsConsumed]);
                    ExecPlan->EntireCmd.CmdToExec.ArgC--;
                    ArgsConsumed++;
                } else if (YoriLibCompareStringLit(ThisArg, _T("&!")) == 0) {
                    ExecPlan->WaitForCompletion = FALSE;

                    ExecPlan->EntireCmd.WaitForCompletion = FALSE;
                    ExecPlan->EntireCmd.StdInType = StdInTypeNull;
                    ExecPlan->EntireCmd.StdOutType = StdOutTypeBuffer;
                    ExecPlan->EntireCmd.StdOut.Buffer.RetainBufferData = TRUE;
                    ExecPlan->EntireCmd.StdErrType = StdErrTypeBuffer;
                    ExecPlan->EntireCmd.StdErr.Buffer.RetainBufferData = TRUE;

                    ThisProgram->WaitForCompletion = FALSE;
                    ThisProgram->StdInType = StdInTypeNull;
                    ThisProgram->StdOutType = StdOutTypeBuffer;
                    ThisProgram->StdOut.Buffer.RetainBufferData = TRUE;
                    ThisProgram->StdErrType = StdErrTypeBuffer;
                    ThisProgram->StdErr.Buffer.RetainBufferData = TRUE;

                    YoriLibFreeStringContents(&ExecPlan->EntireCmd.CmdToExec.ArgV[CurrentArg + ArgsConsumed]);
                    ExecPlan->EntireCmd.CmdToExec.ArgC--;
                    ArgsConsumed++;
                } else if (YoriLibCompareStringLit(ThisArg, _T("&!!")) == 0) {
                    ExecPlan->WaitForCompletion = FALSE;

                    ExecPlan->EntireCmd.WaitForCompletion = FALSE;
                    ExecPlan->EntireCmd.RunOnSecondConsole = TRUE;

                    ThisProgram->WaitForCompletion = FALSE;
                    ThisProgram->RunOnSecondConsole = TRUE;

                    YoriLibFreeStringContents(&ExecPlan->EntireCmd.CmdToExec.ArgV[CurrentArg + ArgsConsumed]);
                    ExecPlan->EntireCmd.CmdToExec.ArgC--;
                    ArgsConsumed++;
                }
            }
        }

        //
        //  If the active argument within the command context falls within the
        //  scope of this single program, and the caller wants to know where it
        //  falls, return this program, whether the current arg index maps to
        //  a program argument (as opposed to redirection or a seperator),
        //  and the index of the program argument.
        //

        if (CmdContext->CurrentArg >= CurrentArg &&
            CmdContext->CurrentArg < CurrentArg + ArgsConsumed) {

            FoundProgramMatch = TRUE;

            if (CurrentExecContext != NULL) {
                *CurrentExecContext = ThisProgram;
            }

            if (CurrentArgIndex != NULL) {
                *CurrentArgIndex = LocalCurrentArgIndex;
            }

            if (CurrentArgOffset != NULL) {
                *CurrentArgOffset = LocalCurrentArgOffset;
            }

            if (CurrentArgIsForProgram != NULL) {
                *CurrentArgIsForProgram = LocalCurrentArgIsForProgram;
            }
        }

        ThisProgram->ReferenceCount = 1;

        if (PreviousProgram != NULL) {
            PYORI_STRING ArgOfLastOperator;

            ArgOfLastOperator = &CmdContext->ArgV[ArgOfLastOperatorIndex];

            PreviousProgram->NextProgram = ThisProgram;
            if (YoriLibCompareStringLitIns(ArgOfLastOperator, _T("&")) == 0 ||
                YoriLibCompareStringLitIns(ArgOfLastOperator, _T("\n")) == 0) {
                PreviousProgram->NextProgramType = NextProgramExecUnconditionally;
            } else if (YoriLibCompareStringLitIns(ArgOfLastOperator, _T("&&")) == 0) {
                PreviousProgram->NextProgramType = NextProgramExecOnSuccess;
            } else if (YoriLibCompareStringLitIns(ArgOfLastOperator, _T("||")) == 0) {
                PreviousProgram->NextProgramType = NextProgramExecOnFailure;
            } else if (YoriLibCompareStringLitIns(ArgOfLastOperator, _T("|")) == 0) {
                PreviousProgram->NextProgramType = NextProgramExecConcurrently;
                if (PreviousProgram->StdOutType == StdOutTypeDefault) {
                    PreviousProgram->StdOutType = StdOutTypePipe;
                }
                if (ThisProgram->StdInType == StdInTypeDefault) {
                    ThisProgram->StdInType = StdInTypePipe;
                }
                PreviousProgram->WaitForCompletion = FALSE;
            } else {
                ASSERT(!"YoriLibShIsArgumentProgramSeperator said this argument was a seperator but YoriLibShParseCmdContextToExecPlan doesn't know what to do with it");
                PreviousProgram->NextProgramType = NextProgramExecUnconditionally;
            }
        } else {
            ExecPlan->FirstCmd = ThisProgram;
        }

        ExecPlan->NumberCommands++;
        PreviousProgram = ThisProgram;
        CurrentArg = CurrentArg + ArgsConsumed;

        while (CurrentArg < CmdContext->ArgC &&
               YoriLibShIsArgumentProgramSeperator(&CmdContext->ArgV[CurrentArg], FALSE)) {

            ArgOfLastOperatorIndex = CurrentArg;
            CurrentArg++;
        }
    }

    if (CmdContext->CurrentArg >= CmdContext->ArgC &&
        PreviousProgram != NULL &&
        !FoundProgramMatch) {

        if (CurrentExecContext != NULL) {
            *CurrentExecContext = PreviousProgram;
        }

        if (CurrentArgIndex != NULL) {
            *CurrentArgIndex = PreviousProgram->CmdToExec.ArgC + 1;
        }

        if (CurrentArgOffset != NULL) {
            *CurrentArgOffset = 0;
        }

        if (CurrentArgIsForProgram != NULL) {
            *CurrentArgIsForProgram = TRUE;
        }
    }

    return TRUE;
}

/**
 Returns TRUE if the string specified by the user contains any path seperator.
 If it does, this implies the string should not be resolved against %PATH% and
 should be treated as absolutely specified.

 @param SearchFor Pointer to a user provided string to test for a path
        seperator.

 @return TRUE if a path seperator is found, FALSE if it is not.
 */
BOOL
YoriLibShDoesExpressionSpecifyPath(
    __in PYORI_STRING SearchFor
    )
{
    DWORD i;

    //
    //  If the path contains a backslash or anything that suggests
    //  it contains path information of its own, don't attempt to
    //  resolve it via path expansion
    //

    for (i = 0; i < SearchFor->LengthInChars; i++) {
        if (SearchFor->StartOfString[i] == '\\' ||
            SearchFor->StartOfString[i] == '/' ||
            SearchFor->StartOfString[i] == ':') {

            return TRUE;
        }
    }
    return FALSE;
}

/**
 A structure describing a single substring within a master string that is
 encompassed by backquote operators.
 */
typedef struct _YORI_LIBSH_BACKQUOTE_ENTRY {

    /**
     The links of all matches encountered while parsing the string.
     */
    YORI_LIST_ENTRY MatchList;

    /**
     The substring within the master string of this backquote pair.  This is
     not referenced and is not NULL terminated.
     */
    YORI_STRING String;

    /**
     Indicates the starting offset from the master string, in characters.
     This could also be calculated from the String value's StartOfString
     member compared to the master string, but keeping it in integer form
     is convenient.
     */
    YORI_ALLOC_SIZE_T StartingOffset;

    /**
     Indicates the level of nesting of this match.  Higher numbers indicate
     more nesting and should be executed earlier.
     */
    YORI_ALLOC_SIZE_T TreeDepth;

    /**
     Set to TRUE if this is a new style entry, aka $(foo) form.  Matches
     such as `foo` will have this be FALSE.
     */
    BOOLEAN NewStyleMatch;

    /**
     Set to TRUE to indicate this entry has found an opening and closing
     operator, and the string is updated to contain the contents in
     between.
     */
    BOOLEAN Terminated;

    /**
     Set to TRUE if this entry has been closed implicitly by encountering
     an operator that fails to complete it.  In this state it is not
     something that can be executed, but it is recorded for the benefit
     of tab completion.
     */
    BOOLEAN Abandoned;
} YORI_LIBSH_BACKQUOTE_ENTRY, *PYORI_LIBSH_BACKQUOTE_ENTRY;


/**
 Conceptually a tree structured as a list of backquoted sequences discovered
 within a flat string.  The depth of the tree corresponds to the level of
 nesting; the first item of the deepest level is evaluated first, then
 the next item at that level, working back through the levels until none
 remains.
 */
typedef struct _YORI_LIBSH_BACKQUOTE_CONTEXT {

    /**
     A list of elements within the tree.
     */
    YORI_LIST_ENTRY MatchList;

    /**
     The number of elements within the tree.
     */
    YORI_ALLOC_SIZE_T MatchCount;

    /**
     The maximum depth of any entry within the tree.
     */
    YORI_ALLOC_SIZE_T MaxDepth;

    /**
     The current depth of the tree (number of opens minus number of closes)
     */
    YORI_ALLOC_SIZE_T CurrentDepth;
} YORI_LIBSH_BACKQUOTE_CONTEXT, *PYORI_LIBSH_BACKQUOTE_CONTEXT;

/**
 Free all entries within a backquote context structure and prepare it for
 reuse.  Note this does not free the backquote context structure itself,
 which is frequently on the stack.

 @param BackquoteContext Pointer to the context to free all suballocations
        from.
 */
VOID
YoriLibShFreeBackquoteContext(
    __in PYORI_LIBSH_BACKQUOTE_CONTEXT BackquoteContext
    )
{
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIBSH_BACKQUOTE_ENTRY BackquoteEntry;

    ListEntry = YoriLibGetNextListEntry(&BackquoteContext->MatchList, NULL);
    while (ListEntry != NULL) {
        BackquoteEntry = CONTAINING_RECORD(ListEntry, YORI_LIBSH_BACKQUOTE_ENTRY, MatchList);
        ASSERT(BackquoteContext->MatchCount > 0);
        BackquoteContext->MatchCount--;
        YoriLibRemoveListItem(&BackquoteEntry->MatchList);
        YoriLibDereference(BackquoteEntry);

        ListEntry = YoriLibGetNextListEntry(&BackquoteContext->MatchList, NULL);
    }

    ASSERT(BackquoteContext->MatchCount == 0);
    BackquoteContext->MaxDepth = 0;
    BackquoteContext->CurrentDepth = 0;
}

/**
 Allocate an entry that can describe a substring within the main string
 representing the text between two backquote operators.

 @param BackquoteContext Pointer to the backquote context describing the
        set of things found in the string so far.  On success, the new
        allocation will be linked into this structure.

 @param CompleteString Pointer to the master string being parsed.

 @param Offset Specifies the beginning of this substring from the master
        string, in characters.

 @param NewStyleMatch If TRUE, specifies that the substring takes $(foo) form.
        If FALSE, the substring takes `foo` form.

 @return On success, a pointer to the newly allocated entry.  On failure,
         returns NULL.
 */
PYORI_LIBSH_BACKQUOTE_ENTRY
YoriLibShAllocateBackquoteEntry(
    __in PYORI_LIBSH_BACKQUOTE_CONTEXT BackquoteContext,
    __in PYORI_STRING CompleteString,
    __in YORI_ALLOC_SIZE_T Offset,
    __in BOOLEAN NewStyleMatch
    )
{
    PYORI_LIBSH_BACKQUOTE_ENTRY BackquoteEntry;

    BackquoteEntry = YoriLibReferencedMalloc(sizeof(YORI_LIBSH_BACKQUOTE_ENTRY));
    if (BackquoteEntry == NULL) {
        return NULL;
    }

    BackquoteContext->CurrentDepth++;
    BackquoteContext->MatchCount++;

    YoriLibInitializeListHead(&BackquoteEntry->MatchList);
    YoriLibInitEmptyString(&BackquoteEntry->String);
    BackquoteEntry->String.StartOfString = &CompleteString->StartOfString[Offset];
    BackquoteEntry->String.LengthInChars = CompleteString->LengthInChars - Offset;
    BackquoteEntry->NewStyleMatch = NewStyleMatch;
    BackquoteEntry->TreeDepth = BackquoteContext->CurrentDepth;
    BackquoteEntry->StartingOffset = Offset;
    BackquoteEntry->Terminated = FALSE;
    BackquoteEntry->Abandoned = FALSE;

    if (BackquoteEntry->TreeDepth > BackquoteContext->MaxDepth) {
        BackquoteContext->MaxDepth = BackquoteEntry->TreeDepth;
    }

    YoriLibAppendList(&BackquoteContext->MatchList, &BackquoteEntry->MatchList);

    return BackquoteEntry;
}

/**
 Indicate that a character was found which may indicate the termination of
 a previously opened substring that requires execution.  Note that in the
 case of the ` operator, it is ambiguous whether it represents the start or
 the end of a substring, so this function is always called to determine
 whether it is ending a previously opened substring, and if not, a new
 substring is opened.

 @param BackquoteContext Pointer to the context describing currently found
        backquotes to check for a matching, non-terminated substring.

 @param Offset The offset within the main string of the termination character.

 @param NewStyleMatch TRUE if the operator is of $(foo) form; FALSE if it is
        of `foo` form.  This is required to check which previously opened
        substrings should be matched against the terminator.

 @return If a matching substring was found, returns a pointer to the substring
         entry.  If no matching substring was found, returns NULL.
 */
PYORI_LIBSH_BACKQUOTE_ENTRY
YoriLibShTerminateMatchingBackquoteEntry(
    __in PYORI_LIBSH_BACKQUOTE_CONTEXT BackquoteContext,
    __in YORI_ALLOC_SIZE_T Offset,
    __in BOOLEAN NewStyleMatch
    )
{

    //
    //  Note this function wants to implicitly remove entries that were
    //  not terminated by the found character.  For example, $(`) means
    //  that the ` should be removed when ) is found, leaving ` as
    //  something not currently active so the next character will reopen
    //  it.
    //

    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIBSH_BACKQUOTE_ENTRY Entry;

    ListEntry = YoriLibGetPreviousListEntry(&BackquoteContext->MatchList, NULL);
    while(ListEntry != NULL) {

        Entry = CONTAINING_RECORD(ListEntry, YORI_LIBSH_BACKQUOTE_ENTRY, MatchList);
        if (!Entry->Terminated && !Entry->Abandoned) {
            if (NewStyleMatch == Entry->NewStyleMatch) {

                //
                //  If a termination character was found that matches a non-
                //  terminated opened substring, we have a match, so return
                //  it.
                //

                Entry->Terminated = TRUE;
                Entry->String.LengthInChars = Offset - Entry->StartingOffset;
                ASSERT(BackquoteContext->CurrentDepth > 0);
                BackquoteContext->CurrentDepth--;
                return Entry;
            } else if (!NewStyleMatch) {

                //
                //  If this character is ` but the previously non-terminated
                //  substring is $(, this implies the beginning of a new
                //  substring.
                //

                return NULL;
            } else {

                //
                //  If this character is ) and the previously non-terminated
                //  substring is `, this implies the earlier substring has
                //  not been completed correctly.  This is a syntax error,
                //  and it is handled by treating the ` as a literal character
                //  and not attempting to execute any substring.  It is
                //  retained in the structure for the benefit of tab
                //  completion, which may want to reason about substrings that
                //  are not yet complete.
                //

                Entry->Abandoned = TRUE;
                Entry->String.LengthInChars = Offset - Entry->StartingOffset;
                ASSERT(BackquoteContext->CurrentDepth > 0);
                BackquoteContext->CurrentDepth--;
            }
        }

        ListEntry = YoriLibGetPreviousListEntry(&BackquoteContext->MatchList, ListEntry);
    }

    return NULL;
}

/**
 Parse a master string into a tree structure of substrings which require
 execution.

 @param String Pointer to the master string.

 @param BackquoteContext Pointer to a structure which will be populated within
        this routine containing the set of substrings and their ranges.

 @return TRUE if the master string has been successfully resolved to a series
         of substrings; FALSE if an error occurred.
 */
__success(return)
BOOL
YoriLibShParseBackquoteSubstrings(
    __in PYORI_STRING String,
    __out PYORI_LIBSH_BACKQUOTE_CONTEXT BackquoteContext
    )
{
    PYORI_LIBSH_BACKQUOTE_ENTRY Entry;
    YORI_ALLOC_SIZE_T Index;
    BOOLEAN QuoteOpen;

    YoriLibInitializeListHead(&BackquoteContext->MatchList);
    BackquoteContext->MatchCount = 0;
    BackquoteContext->MaxDepth = 0;
    BackquoteContext->CurrentDepth = 0;
    QuoteOpen = FALSE;

    for (Index = 0; Index < String->LengthInChars; Index++) {

        //
        //  If it's an escape, advance to the next character and ignore
        //  its value, then continue processing from the next next
        //  character.
        //

        if (YoriLibIsEscapeChar(String->StartOfString[Index])) {
            Index++;
            if (Index >= String->LengthInChars) {
                break;
            } else {
                continue;
            }
        }

        if (String->StartOfString[Index] == '"') {
            if (QuoteOpen) {
                QuoteOpen = FALSE;
            } else {
                QuoteOpen = TRUE;
            }
        }

        if (QuoteOpen) {
            continue;
        }

        if (String->StartOfString[Index] == '`') {
            if (!YoriLibShTerminateMatchingBackquoteEntry(BackquoteContext, Index, FALSE)) {
                Entry = YoriLibShAllocateBackquoteEntry(BackquoteContext, String, Index + 1, FALSE);
                if (Entry == NULL) {
                    break;
                }

            }
        } else if (String->StartOfString[Index] == ')') {
            YoriLibShTerminateMatchingBackquoteEntry(BackquoteContext, Index, TRUE);
        } else if (String->StartOfString[Index] == '$' &&
                   Index + 1 < String->LengthInChars &&
                   String->StartOfString[Index + 1] == '(') {

            Entry = YoriLibShAllocateBackquoteEntry(BackquoteContext, String, Index + 2, TRUE);
            if (Entry == NULL) {
                break;
            }
        }
    }

    if (Index == String->LengthInChars) {
        return TRUE;
    } else {
        YoriLibShFreeBackquoteContext(BackquoteContext);
        return FALSE;
    }
}


/**
 Search through a string and return the next backquote substring to execute.
 If no backquote substrings requiring execution are found, this function
 returns FALSE.

 @param String Pointer to the string to process.

 @param CurrentSubset On successful completion, updated to point to the next
        substring to execute.

 @param CharsInPrefix On successful completion, updated to indicate the number
        of characters before the CurrentSubset were used to indicate its
        commencement.  This allows the caller to regenerate a new string with
        contents substituted.

 @return TRUE if there is a substring to execute, FALSE if there is not.
 */
__success(return)
BOOL
YoriLibShFindNextBackquoteSubstring(
    __in PYORI_STRING String,
    __out PYORI_STRING CurrentSubset,
    __out PYORI_ALLOC_SIZE_T CharsInPrefix
    )
{
    YORI_LIBSH_BACKQUOTE_CONTEXT BackquoteContext;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIBSH_BACKQUOTE_ENTRY BackquoteEntry;
    YORI_ALLOC_SIZE_T SeekingDepth;
    if (!YoriLibShParseBackquoteSubstrings(String, &BackquoteContext)) {
        return FALSE;
    }

    for (SeekingDepth = BackquoteContext.MaxDepth; SeekingDepth > 0; SeekingDepth--) {

        ListEntry = YoriLibGetNextListEntry(&BackquoteContext.MatchList, NULL);
        while (ListEntry != NULL) {
            BackquoteEntry = CONTAINING_RECORD(ListEntry, YORI_LIBSH_BACKQUOTE_ENTRY, MatchList);
            if (BackquoteEntry->Terminated && BackquoteEntry->TreeDepth == SeekingDepth) {
                memcpy(CurrentSubset, &BackquoteEntry->String, sizeof(YORI_STRING));
                if (BackquoteEntry->NewStyleMatch) {
                    *CharsInPrefix = 2;
                } else {
                    *CharsInPrefix = 1;
                }
                YoriLibShFreeBackquoteContext(&BackquoteContext);
                return TRUE;
            }
            ListEntry = YoriLibGetNextListEntry(&BackquoteContext.MatchList, ListEntry);
        }
    }

    YoriLibShFreeBackquoteContext(&BackquoteContext);
    return FALSE;
}

/**
 Given a string and a current selected offset within the string, find the
 "best" backquote substring for tab completion.  This means the innermost
 level of nesting that overlaps with the current selected offset.

 @param String The entire string to parse for backquotes.

 @param StringOffset The current offset within the string, in characters.

 @param CurrentSubset On successful completion, updated to contain the best
        substring to use for tab completion.  Note this shares an allocation
        with String, is not referenced, and is not NULL terminated.

 @return TRUE if a substring was found, FALSE if it was not.
 */
__success(return)
BOOL
YoriLibShFindBestBackquoteSubstringAtOffset(
    __in PYORI_STRING String,
    __in YORI_ALLOC_SIZE_T StringOffset,
    __out PYORI_STRING CurrentSubset
    )
{
    YORI_LIBSH_BACKQUOTE_CONTEXT BackquoteContext;
    PYORI_LIST_ENTRY ListEntry;
    PYORI_LIBSH_BACKQUOTE_ENTRY BackquoteEntry;
    YORI_ALLOC_SIZE_T SeekingDepth;

    if (!YoriLibShParseBackquoteSubstrings(String, &BackquoteContext)) {
        return FALSE;
    }

    for (SeekingDepth = BackquoteContext.MaxDepth; SeekingDepth > 0; SeekingDepth--) {

        ListEntry = YoriLibGetNextListEntry(&BackquoteContext.MatchList, NULL);
        while (ListEntry != NULL) {
            BackquoteEntry = CONTAINING_RECORD(ListEntry, YORI_LIBSH_BACKQUOTE_ENTRY, MatchList);

            //
            //  For tab completion, it doesn't matter if the substring is
            //  terminated, abandoned or neither.  The assumption is that
            //  substrings on the same depth can't overlap, so if we search
            //  from the deepest level to the shallowest level, the first
            //  overlapping range is the "right" one.
            //

            if (BackquoteEntry->TreeDepth == SeekingDepth &&
                StringOffset >= BackquoteEntry->StartingOffset &&
                StringOffset <= BackquoteEntry->StartingOffset + BackquoteEntry->String.LengthInChars) {
                memcpy(CurrentSubset, &BackquoteEntry->String, sizeof(YORI_STRING));
                YoriLibShFreeBackquoteContext(&BackquoteContext);
                return TRUE;
            }
            ListEntry = YoriLibGetNextListEntry(&BackquoteContext.MatchList, ListEntry);
        }
    }

    YoriLibShFreeBackquoteContext(&BackquoteContext);
    return FALSE;
}


// vim:sw=4:ts=4:et:
