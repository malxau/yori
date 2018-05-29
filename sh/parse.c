/**
 * @file parse.c
 *
 * Parses an expression into component pieces
 *
 * Copyright (c) 2014-2017 Malcolm J. Smith
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

#include "yori.h"

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
BOOL
YoriShIsArgumentSeperator(
    __in PYORI_STRING String,
    __out PDWORD CharsToConsumeOut,
    __out PBOOL TerminateArgOut
    )
{
    ULONG CharsToConsume = 0;
    BOOL Terminate = FALSE;

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
            if (String->StartOfString[1] == '>') {
                CharsToConsume++;
            }
        } else if (String->StartOfString[0] == '<') {
            CharsToConsume++;
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

    *TerminateArgOut = Terminate;
    *CharsToConsumeOut = CharsToConsume;

    if (CharsToConsume > 0) {
        return TRUE;
    } 

    return FALSE;
}

/**
 Remove spaces from the beginning of a Yori string.  Note this implies
 advancing the StartOfString pointer, so a caller cannot assume this
 pointer is unchanged across the call.

 @param String Pointer to the Yori string to remove spaces from.
 */
VOID
YoriShTrimSpacesFromBeginning(
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
 Parse a single command string into a series of arguments.  This routine takes
 care of splitting things based on the presence or absence of quotes, as well
 as performing environment variable expansion.  The resulting string has no
 knowledge of redirects, pipes, or multi program execution - it is just a
 series of arguments.

 @param CmdLine The string to parse into arguments.

 @param CurrentOffset The current offset within the string.  This can be set
        to zero if not needed.  The argument that corresponds to this offset
        will be marked in the CmdContext as being the active argument.

 @param CmdContext A caller allocated CmdContext to populate with arguments.
        This routine will allocate space for the argument array and contents.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShParseCmdlineToCmdContext(
    __in PYORI_STRING CmdLine,
    __in DWORD CurrentOffset,
    __out PYORI_CMD_CONTEXT CmdContext
    )
{
    DWORD ArgCount = 0;
    DWORD RequiredCharCount = 0;
    DWORD CharsToConsume = 0;
    BOOL TerminateArg;
    BOOL TerminateNextArg = FALSE;
    BOOL QuoteOpen = FALSE;
    YORI_STRING Char;
    LPTSTR OutputString;
    BOOLEAN CurrentArgFound = FALSE;
    BOOLEAN LookingForFirstQuote = FALSE;
    BOOLEAN IsMultiCommandOperatorArgument = FALSE;

    YoriLibInitEmptyString(&Char);
    Char.StartOfString = CmdLine->StartOfString;
    Char.LengthInChars = CmdLine->LengthInChars;

    //
    //  Consume all spaces.
    //

    YoriShTrimSpacesFromBeginning(&Char);

    if (Char.LengthInChars > 0 && Char.StartOfString[0] == '"') {
        LookingForFirstQuote = TRUE;
    } else {
        LookingForFirstQuote = FALSE;
    }

    while (Char.LengthInChars > 0) {

        //
        //  If it's an escape char, consume two characters as literal until
        //  we hit the end of the string.
        //

        if (YoriLibIsEscapeChar(Char.StartOfString[0])) {
            Char.StartOfString++;
            Char.LengthInChars--;
            RequiredCharCount++;
            if (Char.LengthInChars > 0) {
                Char.StartOfString++;
                Char.LengthInChars--;
                RequiredCharCount++;
                if (Char.LengthInChars == 0) {
                    ArgCount++;
                }
            } else {
                ArgCount++;
            }
            continue;
        }

        //
        //  If the argument started with a quote and we found the end to that
        //  quote, don't copy it into the output string.
        //

        if (Char.StartOfString[0] == '"' && QuoteOpen && LookingForFirstQuote) {
            QuoteOpen = FALSE;
            LookingForFirstQuote = FALSE;
            Char.StartOfString++;
            Char.LengthInChars--;
            if (Char.LengthInChars == 0) {
                if (!CurrentArgFound) {
                    CurrentArgFound = TRUE;
                    CmdContext->CurrentArg = ArgCount;
                }
                ArgCount++;
            }
            continue;
        }

        //
        //  If we see a quote, either we're opening a section that belongs in
        //  one argument or we're ending that section.
        //

        if (Char.StartOfString[0] == '"') {
            QuoteOpen = !QuoteOpen;
            if (LookingForFirstQuote) {
                Char.StartOfString++;
                Char.LengthInChars--;
                if (Char.LengthInChars == 0) {
                    if (!CurrentArgFound) {
                        CurrentArgFound = TRUE;
                        CmdContext->CurrentArg = ArgCount;
                    }
                    ArgCount++;
                }
                continue;
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
            } else if (YoriShIsArgumentSeperator(&Char, &CharsToConsume, &TerminateNextArg)) {
                TerminateArg = TRUE;
            }
        }

        if (TerminateArg) {

            YoriShTrimSpacesFromBeginning(&Char);

            if (!CurrentArgFound &&
                (Char.StartOfString - CmdLine->StartOfString > (LONG)CurrentOffset)) {

                CurrentArgFound = TRUE;
                CmdContext->CurrentArg = ArgCount;
            }

            ArgCount++;

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
                YoriShIsArgumentSeperator(&Char, &CharsToConsume, &TerminateNextArg);
            }

            RequiredCharCount += CharsToConsume;
            Char.StartOfString += CharsToConsume;
            Char.LengthInChars -= CharsToConsume;

            if (Char.LengthInChars == 0) {
                if (!CurrentArgFound) {
                    CurrentArgFound = TRUE;
                    CmdContext->CurrentArg = ArgCount;
                }
                ArgCount++;
                break;
            }

            if (TerminateNextArg) {
                RequiredCharCount++;

                YoriShTrimSpacesFromBeginning(&Char);

                if (!CurrentArgFound &&
                    (Char.StartOfString - CmdLine->StartOfString > (LONG)CurrentOffset)) {

                    CurrentArgFound = TRUE;
                    CmdContext->CurrentArg = ArgCount;
                }

                ArgCount++;
            }

            if (Char.LengthInChars > 0 && Char.StartOfString[0] == '"') {
                LookingForFirstQuote = TRUE;
            } else {
                LookingForFirstQuote = FALSE;
            }


        } else {

            RequiredCharCount++;
            Char.StartOfString++;
            Char.LengthInChars--;

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
                }
                ArgCount++;
            }
        }
    }

    RequiredCharCount++;

    if (!CurrentArgFound) {
        CurrentArgFound = TRUE;
        CmdContext->CurrentArg = ArgCount;
    }

    CmdContext->ArgC = ArgCount;

    if (ArgCount == 0) {
        CmdContext->MemoryToFree = NULL;
        CmdContext->ArgV = NULL;
        CmdContext->ArgContexts = NULL;
        return TRUE;
    }

    CmdContext->MemoryToFree = YoriLibReferencedMalloc((ArgCount * (sizeof(YORI_STRING) + sizeof(YORI_ARG_CONTEXT))) +
                                                       (RequiredCharCount + ArgCount) * sizeof(TCHAR));
    if (CmdContext->MemoryToFree == NULL) {
        return FALSE;
    }

    CmdContext->ArgV = CmdContext->MemoryToFree;

    CmdContext->ArgContexts = (PYORI_ARG_CONTEXT)YoriLibAddToPointer(CmdContext->ArgV, ArgCount * sizeof(YORI_STRING));
    OutputString = (LPTSTR)(CmdContext->ArgContexts + ArgCount);

    ArgCount = 0;
    QuoteOpen = FALSE;
    IsMultiCommandOperatorArgument = FALSE;
    YoriLibInitEmptyString(&CmdContext->ArgV[ArgCount]);
    CmdContext->ArgV[ArgCount].StartOfString = OutputString;
    CmdContext->ArgContexts[ArgCount].Quoted = FALSE;
    YoriLibReference(CmdContext->MemoryToFree);
    CmdContext->ArgV[ArgCount].MemoryToFree = CmdContext->MemoryToFree;

    //
    //  Consume all spaces.  After this, we're either at
    //  the end of string, or we have an arg, and it
    //  might start with a quote
    //

    YoriLibInitEmptyString(&Char);
    Char.StartOfString = CmdLine->StartOfString;
    Char.LengthInChars = CmdLine->LengthInChars;

    YoriShTrimSpacesFromBeginning(&Char);

    if (Char.LengthInChars > 0 && Char.StartOfString[0] == '"') {
        LookingForFirstQuote = TRUE;
        CmdContext->ArgContexts[ArgCount].Quoted = TRUE;
    } else {
        LookingForFirstQuote = FALSE;
    }

    while (Char.LengthInChars > 0) {

        //
        //  If it's an escape char, consume two characters as literal until
        //  we hit the end of the string.
        //

        if (YoriLibIsEscapeChar(Char.StartOfString[0])) {
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
            continue;
        }

        //
        //  If the argument started with a quote and we found the end to that
        //  quote, don't copy it into the output string.
        //

        if (Char.StartOfString[0] == '"' && QuoteOpen && LookingForFirstQuote) {
            QuoteOpen = FALSE;
            LookingForFirstQuote = FALSE;
            Char.StartOfString++;
            Char.LengthInChars--;
            continue;
        }

        //
        //  If we see a quote, either we're opening a section that belongs in
        //  one argument or we're ending that section.
        //

        if (Char.StartOfString[0] == '"') {
            QuoteOpen = !QuoteOpen;
            if (LookingForFirstQuote) {
                Char.StartOfString++;
                Char.LengthInChars--;
                continue;
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
            } else if (YoriShIsArgumentSeperator(&Char, &CharsToConsume, &TerminateNextArg)) {
                TerminateArg = TRUE;
            }
        }

        if (TerminateArg) {

            YoriShTrimSpacesFromBeginning(&Char);

            *OutputString = '\0';
            CmdContext->ArgV[ArgCount].LengthInChars = (DWORD)(OutputString - CmdContext->ArgV[ArgCount].StartOfString);
            CmdContext->ArgV[ArgCount].LengthAllocated = CmdContext->ArgV[ArgCount].LengthInChars + 1;
            OutputString++;

            if (Char.LengthInChars > 0) {
                ArgCount++;
                YoriLibInitEmptyString(&CmdContext->ArgV[ArgCount]);
                CmdContext->ArgV[ArgCount].StartOfString = OutputString;
                if (Char.StartOfString[0] == '"') {
                    CmdContext->ArgContexts[ArgCount].Quoted = TRUE;
                    LookingForFirstQuote = TRUE;
                } else {
                    CmdContext->ArgContexts[ArgCount].Quoted = FALSE;
                    LookingForFirstQuote = FALSE;
                }
                YoriLibReference(CmdContext->MemoryToFree);
                CmdContext->ArgV[ArgCount].MemoryToFree = CmdContext->MemoryToFree;

                //
                //  If we were processing a space but the next argument is a
                //  common seperator, see if it's self contained and we should
                //  move ahead one more argument.
                //

                if (CharsToConsume == 0) {
                    YoriShIsArgumentSeperator(&Char, &CharsToConsume, &TerminateNextArg);
                }

                if (CharsToConsume > 0) {
                    memcpy(OutputString, Char.StartOfString, CharsToConsume * sizeof(TCHAR));
                    OutputString += CharsToConsume;
                    Char.StartOfString += CharsToConsume;
                    Char.LengthInChars -= CharsToConsume;

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
                    YoriShTrimSpacesFromBeginning(&Char);
                    *OutputString = '\0';
                    CmdContext->ArgV[ArgCount].LengthInChars = (DWORD)(OutputString - CmdContext->ArgV[ArgCount].StartOfString);
                    CmdContext->ArgV[ArgCount].LengthAllocated = CmdContext->ArgV[ArgCount].LengthInChars + 1;
                    OutputString++;
                    if (Char.LengthInChars > 0) {
                        ArgCount++;
                        YoriLibInitEmptyString(&CmdContext->ArgV[ArgCount]);
                        CmdContext->ArgV[ArgCount].StartOfString = OutputString;
                        if (Char.StartOfString[0] == '"') {
                            CmdContext->ArgContexts[ArgCount].Quoted = TRUE;
                            LookingForFirstQuote = TRUE;
                        } else {
                            CmdContext->ArgContexts[ArgCount].Quoted = FALSE;
                            LookingForFirstQuote = FALSE;
                        }
                        YoriLibReference(CmdContext->MemoryToFree);
                        CmdContext->ArgV[ArgCount].MemoryToFree = CmdContext->MemoryToFree;
                    }
                }
            }
        } else {

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
        *OutputString = '\0';
        CmdContext->ArgV[ArgCount].LengthInChars = (DWORD)(OutputString - CmdContext->ArgV[ArgCount].StartOfString);
        CmdContext->ArgV[ArgCount].LengthAllocated = CmdContext->ArgV[ArgCount].LengthInChars + 1;
    }

    //
    //  Expand any environment variables in any of the arguments.
    //

    for (ArgCount = 0; ArgCount < CmdContext->ArgC; ArgCount++) {
        YORI_STRING EnvExpandedString;
        ASSERT(YoriLibIsStringNullTerminated(&CmdContext->ArgV[ArgCount]));
        if (YoriShExpandEnvironmentVariables(&CmdContext->ArgV[ArgCount], &EnvExpandedString)) {
            if (EnvExpandedString.StartOfString != CmdContext->ArgV[ArgCount].StartOfString) {
                YoriLibFreeStringContents(&CmdContext->ArgV[ArgCount]);
                memcpy(&CmdContext->ArgV[ArgCount], &EnvExpandedString, sizeof(YORI_STRING));
                ASSERT(YoriLibIsStringNullTerminated(&CmdContext->ArgV[ArgCount]));
            }
        }
    }

    return TRUE;
}

/**
 This routine is the inverse of @ref YoriShParseCmdlineToCmdContext.  It takes
 a series of arguments and reassembles them back into a single string.

 @param CmdContext Pointer to the command context to use to create the string.

 @param RemoveEscapes If TRUE, escape characters should be removed from the
        command line.  This is done immediately before passing the string to
        an external program.  If FALSE, escapes are retained.  This is done
        when processing the string internally.

 @param BeginCurrentArg If non-NULL, will be populated with the offset in
        the string corresponding to the beginning of the current argument.

 @param EndCurrentArg If non-NULL, will be populated with the offset in
        the string corresponding to the end of the current argument.

 @return Pointer to the resulting string, or NULL to indicate allocation
         failure.  This should be freed with @ref YoriLibDereference.
 */
LPTSTR
YoriShBuildCmdlineFromCmdContext(
    __in PYORI_CMD_CONTEXT CmdContext,
    __in BOOL RemoveEscapes,
    __out_opt PDWORD BeginCurrentArg,
    __out_opt PDWORD EndCurrentArg
    )
{
    DWORD count;
    DWORD BufferLength = 0;
    DWORD CmdLineOffset = 0;
    LPTSTR CmdLine;
    PYORI_STRING ThisArg;
    DWORD SrcOffset;
    DWORD DestOffset;

    for (count = 0; count < CmdContext->ArgC; count++) {
        BufferLength += 3 + CmdContext->ArgV[count].LengthInChars;
    }

    BufferLength += 2;

    CmdLine = YoriLibReferencedMalloc(BufferLength * sizeof(TCHAR));
    if (CmdLine == NULL) {
        return NULL;
    }

    YoriLibSPrintfS(CmdLine, BufferLength, _T(""));

    for (count = 0; count < CmdContext->ArgC; count++) {
        ThisArg = &CmdContext->ArgV[count];

        if (count != 0) {
            CmdLine[CmdLineOffset++] = ' ';
        }

        if (count == CmdContext->CurrentArg && BeginCurrentArg != NULL) {
            *BeginCurrentArg = CmdLineOffset;
        }

        if (CmdContext->ArgContexts[count].Quoted) {
            CmdLine[CmdLineOffset++] = '"';
        }

        for (SrcOffset = DestOffset = 0; SrcOffset < ThisArg->LengthInChars; SrcOffset++, DestOffset++) {
            if (RemoveEscapes && YoriLibIsEscapeChar(ThisArg->StartOfString[SrcOffset])) {
                SrcOffset++;
                if (SrcOffset < ThisArg->LengthInChars) {
                    CmdLine[CmdLineOffset + DestOffset] = ThisArg->StartOfString[SrcOffset];
                } else {
                    break;
                }
            } else {
                CmdLine[CmdLineOffset + DestOffset] = ThisArg->StartOfString[SrcOffset];
            }
        }
        CmdLineOffset += DestOffset;

        if (CmdContext->ArgContexts[count].Quoted) {
            CmdLine[CmdLineOffset++] = '"';
        }

        if (count == CmdContext->CurrentArg && EndCurrentArg != NULL) {
            *EndCurrentArg = CmdLineOffset - 1;
        }
    }

    CmdLine[CmdLineOffset++] = '\0';

    return CmdLine;
}

/**
 Remove escapes from an existing CmdContext.  This is used before invoking a
 builtin which expects ArgC/ArgV formed arguments, but does not want escapes
 preserved.

 @param CmdContext Pointer to the command context.  Individual arguments
        within this string may be modified.

 @return TRUE to indicate all escapes were removed, FALSE if not all could
         be successfully processed.
 */
BOOL
YoriShRemoveEscapesFromCmdContext(
    __in PYORI_CMD_CONTEXT CmdContext
    )
{
    DWORD ArgIndex;
    DWORD CharIndex;
    DWORD DestIndex;
    BOOLEAN EscapeFound;
    PYORI_STRING ThisArg;

    for (ArgIndex = 0; ArgIndex < CmdContext->ArgC; ArgIndex++) {
        ThisArg = &CmdContext->ArgV[ArgIndex];

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

            YoriLibFreeStringContents(&CmdContext->ArgV[ArgIndex]);

            memcpy(&CmdContext->ArgV[ArgIndex], &NewArg, sizeof(YORI_STRING));
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
YoriShCopyArg(
    __in PYORI_CMD_CONTEXT SrcCmdContext,
    __in DWORD SrcArgument,
    __in PYORI_CMD_CONTEXT DestCmdContext,
    __in DWORD DestArgument
    )
{
    DestCmdContext->ArgContexts[DestArgument].Quoted = SrcCmdContext->ArgContexts[SrcArgument].Quoted;
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
BOOL
YoriShCopyCmdContext(
    __out PYORI_CMD_CONTEXT DestCmdContext,
    __in PYORI_CMD_CONTEXT SrcCmdContext
    )
{
    DWORD Count;

    DestCmdContext->MemoryToFree = YoriLibReferencedMalloc(SrcCmdContext->ArgC * (sizeof(YORI_STRING) + sizeof(YORI_ARG_CONTEXT)));
    if (DestCmdContext->MemoryToFree == NULL) {
        return FALSE;
    }

    DestCmdContext->ArgV = DestCmdContext->MemoryToFree;
    DestCmdContext->ArgContexts = (PYORI_ARG_CONTEXT)YoriLibAddToPointer(DestCmdContext->ArgV, SrcCmdContext->ArgC * sizeof(YORI_STRING));

    DestCmdContext->ArgC = SrcCmdContext->ArgC;
    DestCmdContext->CurrentArg = SrcCmdContext->CurrentArg;

    for (Count = 0; Count < DestCmdContext->ArgC; Count++) {
        YoriShCopyArg(SrcCmdContext, Count, DestCmdContext, Count);
    }

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
YoriShCheckIfArgNeedsQuotes(
    __in PYORI_CMD_CONTEXT CmdContext,
    __in DWORD ArgIndex
    )
{
    BOOL HasWhiteSpace;

    HasWhiteSpace = YoriLibCheckIfArgNeedsQuotes(&CmdContext->ArgV[ArgIndex]);
    if (HasWhiteSpace) {
        CmdContext->ArgContexts[ArgIndex].Quoted = TRUE;
    }
}

/**
 Free the contents of a @ref YORI_CMD_CONTEXT .  The allocation containing
 the context is not freed, since that context is often on the stack or
 in another structure, and it is better left to the caller to clean up.

 @param CmdContext Pointer to the @ref YORI_CMD_CONTEXT to free.
 */
VOID
YoriShFreeCmdContext(
    __in PYORI_CMD_CONTEXT CmdContext
    )
{
    DWORD Count;

    if (CmdContext->ArgV != NULL) {
        for (Count = 0; Count < CmdContext->ArgC; Count++) {
            YoriLibFreeStringContents(&CmdContext->ArgV[Count]);
        }
    }
    if (CmdContext->MemoryToFree) {
        YoriLibDereference(CmdContext->MemoryToFree);
    }
}

/**
 Clean up any currently existing StdIn information in an ExecContext.

 @param ExecContext the context to clean up.
 */
VOID
YoriShExecContextCleanupStdIn(
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext
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
YoriShExecContextCleanupStdOut(
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext
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
                YoriShDereferenceProcessBuffer(ExecContext->StdOut.Buffer.ProcessBuffers);
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
YoriShExecContextCleanupStdErr(
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext
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

 @return TRUE to indicate this argument is a seperator between programs.
 */
BOOL
YoriShIsArgumentProgramSeperator(
    __in PYORI_STRING Arg
    )
{
    if (YoriLibCompareStringWithLiteralInsensitive(Arg, _T("&")) == 0 ||
        YoriLibCompareStringWithLiteralInsensitive(Arg, _T("&&")) == 0 ||
        YoriLibCompareStringWithLiteralInsensitive(Arg, _T("\n")) == 0 ||
        YoriLibCompareStringWithLiteralInsensitive(Arg, _T("|")) == 0 ||
        YoriLibCompareStringWithLiteralInsensitive(Arg, _T("||")) == 0) {

        return TRUE;
    }
    return FALSE;
}

/**
 Return TRUE if the argument is a special DOS device name, FALSE if it is
 a regular file.  DOS device names include things like AUX, CON, PRN etc.
 In Yori, a DOS device name is only a DOS device name if it does not
 contain any path information.

 @param File The file name to check.

 @return TRUE to indicate this argument is a DOS device name, FALSE to
         indicate that it is a regular file.
 */
BOOL
YoriShIsFileNameDeviceName(
    __in PYORI_STRING File
    )
{
    if (YoriLibCompareStringWithLiteralInsensitive(File, _T("CON")) == 0 ||
        YoriLibCompareStringWithLiteralInsensitive(File, _T("AUX")) == 0 ||
        YoriLibCompareStringWithLiteralInsensitive(File, _T("PRN")) == 0 ||
        YoriLibCompareStringWithLiteralInsensitive(File, _T("NUL")) == 0) {

        return TRUE;
    }

    if (File->LengthInChars < 4) {
        return FALSE;
    }

    if (YoriLibCompareStringWithLiteralInsensitiveCount(File, _T("LPT"), 3) == 0 &&
        (File->StartOfString[3] >= '1' && File->StartOfString[3] <= '9')) {

        return TRUE;
    }

    if (YoriLibCompareStringWithLiteralInsensitiveCount(File, _T("COM"), 3) == 0 &&
        (File->StartOfString[3] >= '1' && File->StartOfString[3] <= '9')) {

        return TRUE;
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
BOOL
YoriShCheckForDeviceNameAndDuplicate(
    __inout PYORI_STRING UserString,
    __in DWORD UserStringOffset,
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
        YoriShIsFileNameDeviceName(&UserName)) {

        if (UserName.MemoryToFree != NULL) {
            YoriLibReference(UserName.MemoryToFree);
        }
        memcpy(ResolvedName, &UserName, sizeof(YORI_STRING));

    } else {
        return YoriLibUserStringToSingleFilePath(&UserName, TRUE, ResolvedName);
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

 @return The number of arguments consumed while creating information about
         how to execute a single program.
 */
DWORD
YoriShParseCmdContextToExecContext(
    __in PYORI_CMD_CONTEXT CmdContext,
    __in DWORD InitialArgument,
    __out PYORI_SINGLE_EXEC_CONTEXT ExecContext,
    __out_opt PBOOL CurrentArgIsForProgram,
    __out_opt PDWORD CurrentArgIndex
    )
{
    DWORD Count;
    BOOL RemoveThisArg;
    DWORD ArgumentsConsumed = 0;
    PYORI_STRING ThisArg;

    ZeroMemory(ExecContext, sizeof(YORI_SINGLE_EXEC_CONTEXT));
    ExecContext->StdInType = StdInTypeDefault;
    ExecContext->StdOutType = StdOutTypeDefault;
    ExecContext->StdErrType = StdErrTypeDefault;
    ExecContext->WaitForCompletion = TRUE;
    ExecContext->RunOnSecondConsole = FALSE;

    //
    //  First, count the number of arguments that will be consumed by this
    //  program.
    //

    for (Count = InitialArgument; Count < CmdContext->ArgC; Count++) {
        if (Count != (CmdContext->ArgC - 1)) {
            if (YoriShIsArgumentProgramSeperator(&CmdContext->ArgV[Count])) {
                break;
            }
        }
    }

    ArgumentsConsumed = Count - InitialArgument;

    ExecContext->CmdToExec.MemoryToFree = YoriLibReferencedMalloc(ArgumentsConsumed * (sizeof(YORI_STRING) + sizeof(YORI_ARG_CONTEXT)));
    if (ExecContext->CmdToExec.MemoryToFree == NULL) {
        return 0;
    }

    ExecContext->CmdToExec.ArgV = ExecContext->CmdToExec.MemoryToFree;
    ExecContext->CmdToExec.ArgContexts = (PYORI_ARG_CONTEXT)YoriLibAddToPointer(ExecContext->CmdToExec.ArgV, ArgumentsConsumed * sizeof(YORI_STRING));

    //
    //  MSFIX This parsing logic is really lame.
    //  Should support quoted file names
    //  Should support file names in different args to operators
    //  Should resolve to full path names with escapes, which implies double
    //    buffering
    //

    for (Count = InitialArgument; Count < (InitialArgument + ArgumentsConsumed); Count++) {

        RemoveThisArg = FALSE;

        //
        //  The first argument is the thing to execute and cannot contain
        //  any redirection information, so skip it.
        //

        if (Count > InitialArgument) {

            ThisArg = &CmdContext->ArgV[Count];

            if (!CmdContext->ArgContexts[Count].Quoted) {

                if (ThisArg->StartOfString[0] == '<') {
                    YoriShExecContextCleanupStdIn(ExecContext);
                    ExecContext->StdInType = StdInTypeFile;
                    YoriLibInitEmptyString(&ExecContext->StdIn.File.FileName);
                    YoriShCheckForDeviceNameAndDuplicate(ThisArg,
                                                         1,
                                                         &ExecContext->StdIn.File.FileName);
                    RemoveThisArg = TRUE;
                }

                if (ThisArg->StartOfString[0] == '>') {
                    YoriShExecContextCleanupStdOut(ExecContext);
                    if (ThisArg->StartOfString[1] == '>') {
                        ExecContext->StdOutType = StdOutTypeAppend;
                        YoriShCheckForDeviceNameAndDuplicate(ThisArg,
                                                             2,
                                                             &ExecContext->StdOut.Append.FileName);
                    } else {
                        ExecContext->StdOutType = StdOutTypeOverwrite;
                        YoriShCheckForDeviceNameAndDuplicate(ThisArg,
                                                             1,
                                                             &ExecContext->StdOut.Overwrite.FileName);
                    }
                    RemoveThisArg = TRUE;
                }

                if (ThisArg->StartOfString[0] == '2') {
                    if (ThisArg->StartOfString[1] == '>') {
                        YoriShExecContextCleanupStdErr(ExecContext);
                        ExecContext->StdErrType = StdErrTypeDefault;
                        if (ThisArg->StartOfString[2] == '>') {
                            ExecContext->StdErrType = StdErrTypeAppend;
                            YoriShCheckForDeviceNameAndDuplicate(ThisArg,
                                                                 3,
                                                                 &ExecContext->StdErr.Append.FileName);
                        } else if (ThisArg->StartOfString[2] == '&') {
                            if (ThisArg->StartOfString[3] == '1') {
                                ExecContext->StdErrType = StdErrTypeStdOut;
                            }
                        } else {
                            ExecContext->StdErrType = StdErrTypeOverwrite;
                            YoriShCheckForDeviceNameAndDuplicate(ThisArg,
                                                                 2,
                                                                 &ExecContext->StdErr.Overwrite.FileName);
                        }
                        RemoveThisArg = TRUE;
                    }
                }
            }

            if (Count == (CmdContext->ArgC - 1)) {
                if (ThisArg->StartOfString[0] == '&') {
                    if (YoriLibCompareStringWithLiteral(ThisArg, _T("&")) == 0) {
                        ExecContext->WaitForCompletion = FALSE;
                        RemoveThisArg = TRUE;
                    }
                    if (YoriLibCompareStringWithLiteral(ThisArg, _T("&!")) == 0) {
                        ExecContext->WaitForCompletion = FALSE;
                        ExecContext->StdInType = StdInTypeNull;
                        ExecContext->StdOutType = StdOutTypeBuffer;
                        ExecContext->StdOut.Buffer.RetainBufferData = TRUE;
                        ExecContext->StdErrType = StdOutTypeBuffer;
                        ExecContext->StdErr.Buffer.RetainBufferData = TRUE;
                        RemoveThisArg = TRUE;
                    }
                    if (YoriLibCompareStringWithLiteral(ThisArg, _T("&!!")) == 0) {
                        ExecContext->WaitForCompletion = FALSE;
                        ExecContext->RunOnSecondConsole = TRUE;
                        RemoveThisArg = TRUE;
                    }
                }
            }
        }

        if (!RemoveThisArg) {
            PYORI_CMD_CONTEXT CmdToExec = &ExecContext->CmdToExec;
            YoriShCopyArg(CmdContext, Count, CmdToExec, CmdToExec->ArgC);
            if (CurrentArgIsForProgram != NULL) {
                if (CmdContext->CurrentArg == Count) {
                    *CurrentArgIsForProgram = TRUE;
                    *CurrentArgIndex = CmdToExec->ArgC;
                }
            }
            CmdToExec->ArgC++;
        }
    }

    return ArgumentsConsumed;
}

/**
 Frees any internal allocations in a @ref YORI_SINGLE_EXEC_CONTEXT .  Note it
 does not free the context itself, which is the caller's responsibility.
 This is done because it's frequently convenient to have context on the
 stack.

 @param ExecContext Pointer to the single execution context to free.
 */
VOID
YoriShFreeExecContext(
    __in PYORI_SINGLE_EXEC_CONTEXT ExecContext
    )
{

    //
    //  If the process was being debugged, the debugger thread should
    //  have torn down before we tear down the context it uses.
    //

    if (ExecContext->hDebuggerThread != NULL) {
        ASSERT(WaitForSingleObject(ExecContext->hDebuggerThread, 0) == WAIT_OBJECT_0);
        CloseHandle(ExecContext->hDebuggerThread);
        ExecContext->hDebuggerThread = NULL;
    }

    YoriShFreeCmdContext(&ExecContext->CmdToExec);

    YoriShExecContextCleanupStdIn(ExecContext);
    YoriShExecContextCleanupStdOut(ExecContext);
    YoriShExecContextCleanupStdErr(ExecContext);
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
 Frees any internal allocations in a @ref YORI_EXEC_PLAN .  Note it
 does not free the plan itself, which is the caller's responsibility.
 This is done because it's frequently convenient to have plan on the
 stack.

 @param ExecPlan Pointer to the execution plan to free.
 */
VOID
YoriShFreeExecPlan(
    __in PYORI_EXEC_PLAN ExecPlan
    )
{
    PYORI_SINGLE_EXEC_CONTEXT ExecContext;
    PYORI_SINGLE_EXEC_CONTEXT NextExecContext;

    ExecContext = ExecPlan->FirstCmd;

    while (ExecContext != NULL) {
        NextExecContext = ExecContext->NextProgram;
        YoriShFreeExecContext(ExecContext);
        YoriLibFree(ExecContext);
        ExecContext = NextExecContext;
    }
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

 @return TRUE to indicate parsing success, FALSE to indicate failure.
 */
BOOL
YoriShParseCmdContextToExecPlan(
    __in PYORI_CMD_CONTEXT CmdContext,
    __out PYORI_EXEC_PLAN ExecPlan,
    __out_opt PYORI_SINGLE_EXEC_CONTEXT* CurrentExecContext,
    __out_opt PBOOL CurrentArgIsForProgram,
    __out_opt PDWORD CurrentArgIndex
    )
{
    DWORD CurrentArg = 0;
    DWORD ArgsConsumed;
    DWORD ArgOfLastOperatorIndex = 0;
    PYORI_SINGLE_EXEC_CONTEXT ThisProgram;
    PYORI_SINGLE_EXEC_CONTEXT PreviousProgram = NULL;
    BOOL LocalCurrentArgIsForProgram;
    BOOL FoundProgramMatch;
    DWORD LocalCurrentArgIndex;

    ZeroMemory(ExecPlan, sizeof(YORI_EXEC_PLAN));
    FoundProgramMatch = FALSE;

    while (CurrentArg < CmdContext->ArgC) {

        ThisProgram = YoriLibMalloc(sizeof(YORI_SINGLE_EXEC_CONTEXT));
        if (ThisProgram == NULL) {
            YoriShFreeExecPlan(ExecPlan);
            return FALSE;
        }

        LocalCurrentArgIsForProgram = FALSE;
        LocalCurrentArgIndex = 0;

        ArgsConsumed = YoriShParseCmdContextToExecContext(CmdContext, CurrentArg, ThisProgram, &LocalCurrentArgIsForProgram, &LocalCurrentArgIndex);
        if (ArgsConsumed == 0) {
            YoriShFreeExecPlan(ExecPlan);
            return FALSE;
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

            if (CurrentArgIsForProgram != NULL) {
                *CurrentArgIsForProgram = LocalCurrentArgIsForProgram;
            }
        }

        if (PreviousProgram != NULL) {
            PYORI_STRING ArgOfLastOperator;

            ArgOfLastOperator = &CmdContext->ArgV[ArgOfLastOperatorIndex];

            PreviousProgram->NextProgram = ThisProgram;
            if (YoriLibCompareStringWithLiteralInsensitive(ArgOfLastOperator, _T("&")) == 0 ||
                YoriLibCompareStringWithLiteralInsensitive(ArgOfLastOperator, _T("\n")) == 0) {
                PreviousProgram->NextProgramType = NextProgramExecUnconditionally;
            } else if (YoriLibCompareStringWithLiteralInsensitive(ArgOfLastOperator, _T("&&")) == 0) {
                PreviousProgram->NextProgramType = NextProgramExecOnSuccess;
            } else if (YoriLibCompareStringWithLiteralInsensitive(ArgOfLastOperator, _T("||")) == 0) {
                PreviousProgram->NextProgramType = NextProgramExecOnFailure;
            } else if (YoriLibCompareStringWithLiteralInsensitive(ArgOfLastOperator, _T("|")) == 0) {
                PreviousProgram->NextProgramType = NextProgramExecConcurrently;
                if (PreviousProgram->StdOutType == StdOutTypeDefault) {
                    PreviousProgram->StdOutType = StdOutTypePipe;
                }
                if (ThisProgram->StdInType == StdInTypeDefault) {
                    ThisProgram->StdInType = StdInTypePipe;
                }
                PreviousProgram->WaitForCompletion = FALSE;
            } else {
                ASSERT(!"YoriShIsArgumentProgramSeperator said this argument was a seperator but YoriShParseCmdContextToExecPlan doesn't know what to do with it");
                PreviousProgram->NextProgramType = NextProgramExecUnconditionally;
            }
        } else {
            ExecPlan->FirstCmd = ThisProgram;
        }

        ExecPlan->NumberCommands++;
        PreviousProgram = ThisProgram;
        CurrentArg += ArgsConsumed;

        while (CurrentArg < CmdContext->ArgC &&
               YoriShIsArgumentProgramSeperator(&CmdContext->ArgV[CurrentArg])) {

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
YoriShDoesExpressionSpecifyPath(
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
 Expand any aliases in a command context, resolve any executable via path
 lookups, and return with an exec context indicating which program to run.
 If a program is found, ExecutableFound is set to TRUE on return.  If a 
 program is not found, the command should be considered a builtin.  This
 function does not validate if any such builtin exists.

 @param CmdContext The command context to resolve aliases and paths in.

 @param ExecutableFound On successful completion, indicates if an external
        executable was found (by being set to TRUE) or if the command should
        be considered a builtin (by being set to FALSE.)

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOL
YoriShResolveCommandToExecutable(
    __in PYORI_CMD_CONTEXT CmdContext,
    __out PBOOL ExecutableFound
    )
{
    YORI_STRING FoundExecutable;

    YoriLibInitEmptyString(&FoundExecutable);

    YoriShExpandAlias(CmdContext);

    if (YoriLibLocateExecutableInPath(&CmdContext->ArgV[0], NULL, NULL, &FoundExecutable) && FoundExecutable.LengthInChars > 0) {
        YoriLibFreeStringContents(&CmdContext->ArgV[0]);
        memcpy(&CmdContext->ArgV[0], &FoundExecutable, sizeof(YORI_STRING));
        *ExecutableFound = TRUE;
    } else {
        YoriLibFreeStringContents(&FoundExecutable);
        *ExecutableFound = FALSE;
    }

    return TRUE;
}

/**
 Find a yori string corresponding to the substring in another string that
 is delimited by backquotes.  The substring is not referenced.

 @param String The larger string to search through.

 @param StartingOffset The offset within the larger string to commence
        searching.

 @param RequireTerminator If TRUE, only return a substring if both the
        beginning and ending operators are found.  If FALSE, assume the absence
        of an ending operator implies the substring operates to the end of the
        parent string.

 @param Substring On successful completion, populated with the start and length
        of the backquote delimited substring.

 @return TRUE if a substring was found, FALSE if it was not.
 */
BOOL
YoriShFindBackquoteSubstring(
    __in PYORI_STRING String,
    __in DWORD StartingOffset,
    __in BOOL RequireTerminator,
    __out PYORI_STRING Substring
    )
{

    LPTSTR FirstBackQuote;
    LPTSTR SecondBackQuote;
    DWORD Index;

    //
    //  Look to see if backquotes are present and note their locations
    //  if they are.
    //

    FirstBackQuote = NULL;
    SecondBackQuote = NULL;
    for (Index = StartingOffset; Index < String->LengthInChars; Index++) {

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
        if (String->StartOfString[Index] == '`') {
            if (FirstBackQuote == NULL) {
                FirstBackQuote = &String->StartOfString[Index];
            } else {
                SecondBackQuote = &String->StartOfString[Index];
                break;
            }
        }
    }

    if (RequireTerminator && SecondBackQuote == NULL) {
        FirstBackQuote = NULL;
    }

    //
    //  If there are no more backquotes, expansion is complete, so return
    //  the result.
    //

    if (FirstBackQuote == NULL) {
        return FALSE;
    }

    YoriLibInitEmptyString(Substring);
    Substring->StartOfString = FirstBackQuote + 1;
    if (SecondBackQuote != NULL) {
        Substring->LengthInChars = (DWORD)(SecondBackQuote - FirstBackQuote - 1);
    } else {
        Substring->LengthInChars = String->LengthInChars - (DWORD)(FirstBackQuote - String->StartOfString) - 1;
    }

    return TRUE;
}


// vim:sw=4:ts=4:et:
