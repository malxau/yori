/**
 * @file lib/cmdline.c
 *
 * Converts between strings and argc/argv arrays
 *
 * Copyright (c) 2017-2021 Malcolm J. Smith
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
 Returns TRUE if the character should be treated as indicating a command
 line option.

 @param Char The character to check.

 @return TRUE if the character indicates a command line option, FALSE if
         if it does not.
 */
BOOL
YoriLibIsCommandLineOptionChar(
    __in TCHAR Char
    )
{
    if (Char == '/' || Char == '-') {
        return TRUE;
    }
    return FALSE;
}

/**
 Returns TRUE if the string commences with a character indicating a command
 line option, and returns the remainder of the string.

 @param String The string to check.

 @param Arg On successful completion, if the string is an option, this Arg
        contains the option string.

 @return TRUE if the character indicates a command line option, FALSE if
         if it does not.
 */
__success(return)
BOOL
YoriLibIsCommandLineOption(
    __in PYORI_STRING String,
    __out PYORI_STRING Arg
    )
{
    if (String->LengthInChars < 1) {
        return FALSE;
    }
    if (YoriLibIsCommandLineOptionChar(String->StartOfString[0])) {
        YoriLibInitEmptyString(Arg);
        Arg->StartOfString = &String->StartOfString[1];
        Arg->LengthInChars = String->LengthInChars - 1;
        return TRUE;
    }
    return FALSE;
}


/**
 Check if an argument contains spaces and now requires quoting.

 @param Arg The argument to check.

 @return TRUE if quoting is required, FALSE if not.
 */
BOOLEAN
YoriLibCheckIfArgNeedsQuotes(
    __in PYORI_STRING Arg
    )
{
    BOOLEAN HasWhiteSpace;
    DWORD i;

    if (Arg->LengthInChars > 0 &&
        Arg->StartOfString[0] == '"') {

        return FALSE;
    }

    HasWhiteSpace = FALSE;
    for (i = 0; i < Arg->LengthInChars; i++) {
        if (Arg->StartOfString[i] == ' ') {
            HasWhiteSpace = TRUE;
            break;
        }
    }

    if (HasWhiteSpace) {
        return TRUE;
    }

    return FALSE;
}

/**
 This routine creates a command line string from a series of argc/argv style
 arguments described with yori strings.  The caller is expected to free the
 result with @ref YoriLibDereference.

 @param ArgC The number of arguments in the argument array.

 @param ArgV An array of YORI_STRINGs constituting the argument array.

 @param EncloseInQuotes If the argument contains a space, enclose it in quotes.
        If FALSE, return purely space delimited arguments.

 @param ApplyChildProcessEscapes If TRUE, quotes and backslashes preceeding
        quotes are escaped with an extra backslash.  If FALSE, this does not
        occur and the argument retains its original form.  Generally, this
        should be TRUE if the purpose of constructing the command line is to
        launch a child process, which is expected to process its command line
        and remove these escapes, and FALSE if the string is constructed to
        facilitate display or similar where the user specified the escapes to
        indicate how to display text and they should now be removed.

 @param CmdLine On successful completion, updated to point to a newly
        allocated string containing the entire command line.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibBuildCmdlineFromArgcArgv(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[],
    __in BOOLEAN EncloseInQuotes,
    __in BOOLEAN ApplyChildProcessEscapes,
    __out PYORI_STRING CmdLine
    )
{
    DWORD count;
    PYORI_STRING ThisArg;
    DWORD BufferLength;
    DWORD SlashesToWrite;
    DWORD SrcOffset;
    DWORD SlashCount;
    DWORD DestOffset;
    DWORD CmdLineOffset;
    DWORD WriteSlashCount;
    BOOLEAN Quoted;
    BOOLEAN AddQuote;

    YoriLibInitEmptyString(CmdLine);

    BufferLength = 0;

    for (count = 0; count < ArgC; count++) {
        BufferLength += 1;
        ThisArg = &ArgV[count];
        Quoted = FALSE;
        if (EncloseInQuotes) {
            Quoted = YoriLibCheckIfArgNeedsQuotes(ThisArg);
            if (Quoted) {
                BufferLength += 2;
            }
        }
        for (SrcOffset = 0; SrcOffset < ThisArg->LengthInChars; SrcOffset++) {
            if (ApplyChildProcessEscapes &&
                (ThisArg->StartOfString[SrcOffset] == '\\' ||
                 ThisArg->StartOfString[SrcOffset] == '"')) {

                for (SlashCount = 0; SrcOffset + SlashCount < ThisArg->LengthInChars && ThisArg->StartOfString[SrcOffset + SlashCount] == '\\'; SlashCount++);
                if (SrcOffset + SlashCount < ThisArg->LengthInChars && ThisArg->StartOfString[SrcOffset + SlashCount] == '"') {
                    SlashesToWrite = SlashCount * 2 + 1;
                    SrcOffset += SlashCount;
                    BufferLength += 1;
                } else if (SrcOffset + SlashCount == ThisArg->LengthInChars && Quoted) {
                    SlashesToWrite = SlashCount * 2;
                    SrcOffset += SlashCount - 1;
                } else {
                    SlashesToWrite = SlashCount;
                    SrcOffset += SlashCount - 1;
                }
                BufferLength += SlashesToWrite;
            } else {
                BufferLength += 1;
            }
        }
    }

    BufferLength += 1;

    if (!YoriLibAllocateString(CmdLine, BufferLength)) {
        return FALSE;
    }

    CmdLineOffset = 0;
    for (count = 0; count < ArgC; count++) {
        ThisArg = &ArgV[count];

        if (count != 0) {
            CmdLine->StartOfString[CmdLineOffset] = ' ';
            CmdLineOffset++;
        }

        if (EncloseInQuotes) {
            Quoted = YoriLibCheckIfArgNeedsQuotes(ThisArg);

            if (Quoted) {
                CmdLine->StartOfString[CmdLineOffset] = '"';
                CmdLineOffset++;
            }
        } else {
            Quoted = FALSE;
        }

        for (SrcOffset = DestOffset = 0; SrcOffset < ThisArg->LengthInChars; SrcOffset++, DestOffset++) {
            if (ApplyChildProcessEscapes &&
                (ThisArg->StartOfString[SrcOffset] == '\\' ||
                 ThisArg->StartOfString[SrcOffset] == '"')) {

                for (SlashCount = 0; SrcOffset + SlashCount < ThisArg->LengthInChars && ThisArg->StartOfString[SrcOffset + SlashCount] == '\\'; SlashCount++);
                if (SrcOffset + SlashCount < ThisArg->LengthInChars && ThisArg->StartOfString[SrcOffset + SlashCount] == '"') {
                    // Escape the escapes and the quote
                    SlashesToWrite = SlashCount * 2 + 1;
                    AddQuote = TRUE;
                } else if (SrcOffset + SlashCount == ThisArg->LengthInChars && Quoted) {
                    // Escape the escapes but not the quote
                    SlashesToWrite = SlashCount * 2;
                    AddQuote = FALSE;
                } else {
                    // No escapes, just copy verbatim
                    SlashesToWrite = SlashCount;
                    AddQuote = FALSE;
                }
                for (WriteSlashCount = 0; WriteSlashCount < SlashesToWrite; WriteSlashCount++) {
                    CmdLine->StartOfString[CmdLineOffset + DestOffset + WriteSlashCount] = '\\';
                }
                if (AddQuote) {
                    CmdLine->StartOfString[CmdLineOffset + DestOffset + WriteSlashCount] = '"';
                    SrcOffset += SlashCount;
                    DestOffset += WriteSlashCount;
                } else {
                    SrcOffset += SlashCount - 1;
                    DestOffset += WriteSlashCount - 1;
                }
            } else {
                CmdLine->StartOfString[CmdLineOffset + DestOffset] = ThisArg->StartOfString[SrcOffset];
            }
        }
        CmdLineOffset += DestOffset;

        if (EncloseInQuotes) {
            if (Quoted) {
                CmdLine->StartOfString[CmdLineOffset] = '"';
                CmdLineOffset++;
            }
        }
    }

    CmdLine->StartOfString[CmdLineOffset] = '\0';
    CmdLine->LengthInChars = CmdLineOffset;

    return TRUE;
}

/**
 Expand any $ delimited variables by processing the input string and calling
 a callback function for every variable found, allowing the callback to
 populate the output with the correct value.

 @param String The input string, which may contain variables to expand.

 @param MatchChar The character to use to delimit the variable being expanded.

 @param PreserveEscapes If TRUE, escape characters (^) are preserved in the
        output; if FALSE, they are removed from the output.

 @param Function The callback function to invoke when variables are found.

 @param Context A caller provided context to pass to the callback function.

 @param ExpandedString A string allocated by this function containing the
        expanded result.  The caller should free this when it is no longer
        needed with @ref YoriLibFree .

 @return TRUE to indicate success, FALSE to indicate failure.
 */
__success(return)
BOOL
YoriLibExpandCommandVariables(
    __in PYORI_STRING String,
    __in TCHAR MatchChar,
    __in BOOLEAN PreserveEscapes,
    __in PYORILIB_VARIABLE_EXPAND_FN Function,
    __in_opt PVOID Context,
    __inout PYORI_STRING ExpandedString
    )
{
    DWORD DestIndex;
    DWORD Index;
    DWORD FinalIndex;
    BOOL Processed;
    YORI_STRING CmdString;
    YORI_STRING DestString;
    DWORD LengthNeeded;
    DWORD IgnoreUntil;

    if (ExpandedString->LengthAllocated < 256) {
        YoriLibFreeStringContents(ExpandedString);
        if (!YoriLibAllocateString(ExpandedString, 256)) {
            return FALSE;
        }
    }
    DestIndex = 0;
    IgnoreUntil = 0;

    for (Index = 0; Index < String->LengthInChars; Index++) {
        Processed = FALSE;

        if (Index >= IgnoreUntil && YoriLibIsEscapeChar(String->StartOfString[Index])) {
            IgnoreUntil = Index + 2;
            if (!PreserveEscapes) {
                continue;
            }
        }

        if (Index >= IgnoreUntil && String->StartOfString[Index] == MatchChar) {
            FinalIndex = Index + 1;
            while (FinalIndex < String->LengthInChars && String->StartOfString[FinalIndex] != MatchChar) {
                FinalIndex++;
            }

            YoriLibInitEmptyString(&CmdString);
            CmdString.StartOfString = &String->StartOfString[Index + 1];
            CmdString.LengthAllocated = 
            CmdString.LengthInChars = FinalIndex - Index - 1;

            while (TRUE) {
                YoriLibInitEmptyString(&DestString);
                DestString.StartOfString = &ExpandedString->StartOfString[DestIndex];
                DestString.LengthAllocated = ExpandedString->LengthAllocated - DestIndex - 1;

                LengthNeeded = Function(&DestString, &CmdString, Context);

                if (LengthNeeded <= (ExpandedString->LengthAllocated - DestIndex - 1)) {
                    Processed = TRUE;
                    DestIndex += LengthNeeded;
                    Index = FinalIndex;
                    break;
                } else {
                    ExpandedString->LengthInChars = DestIndex;
                    if (!YoriLibReallocateString(ExpandedString, ExpandedString->LengthAllocated * 4)) {
                        YoriLibFreeStringContents(ExpandedString);
                        return FALSE;
                    }
                }
            }
        }

        if (!Processed) {
            ExpandedString->StartOfString[DestIndex] = String->StartOfString[Index];
            DestIndex++;
        }

        if (DestIndex + 1 >= ExpandedString->LengthAllocated) {
            ExpandedString->LengthInChars = DestIndex;
            if (!YoriLibReallocateString(ExpandedString, ExpandedString->LengthAllocated * 4)) {
                YoriLibFreeStringContents(ExpandedString);
                return FALSE;
            }
        }
    }

    ExpandedString->LengthInChars = DestIndex;
    ExpandedString->StartOfString[DestIndex] = '\0';

    return TRUE;
}

/**
 Parses a NULL terminated command line string into an argument count and array
 of YORI_STRINGs corresponding to arguments.

 @param CmdLine The NULL terminated command line.

 @param MaxArgs The maximum number of arguments to return.  All trailing
        arguments are joined with the final argument.

 @param ApplyCaretAsEscape If TRUE, a caret character indicates the following
        character should be interpreted literally and should not be used to
        break arguments.  Caret characters are only meaningful within the
        shell, so external processes should generally use FALSE.  TRUE is
        used when parsing ArgC/ArgV to invoke builtin commands, where escapes
        need to be retained and removed later so the builtin can observe the
        escaped arguments.

 @param argc On successful completion, populated with the count of arguments.

 @return A pointer to an array of YORI_STRINGs containing the parsed
         arguments.
 */
PYORI_STRING
YoriLibCmdlineToArgcArgv(
    __in LPTSTR CmdLine,
    __in DWORD MaxArgs,
    __in BOOLEAN ApplyCaretAsEscape,
    __out PDWORD argc
    )
{
    DWORD ArgCount = 0;
    DWORD CharCount = 0;
    DWORD SlashCount;
    TCHAR BreakChar = ' ';
    TCHAR * c;
    PYORI_STRING ArgvArray;
    LPTSTR ReturnStrings;
    BOOLEAN EndArg;

    //
    //  Consume all spaces.  After this, we're either at
    //  the end of string, or we have an arg, and it
    //  might start with a quote
    //

    c = CmdLine;
    while (*c == ' ') c++;
    if (*c == '"') {
        BreakChar = '"';
        c++;
    }

    while (*c != '\0') {
        EndArg = FALSE;

        if (*c == '^' && c[1] != '\0' && ApplyCaretAsEscape) {
            c++;
            CharCount++;
        } else if (*c == '\\') {
            for (SlashCount = 1; c[SlashCount] == '\\'; SlashCount++);
            if (c[SlashCount] == '"') {

                //
                //  Because one char is left for regular processing, only
                //  adjust for one less pair.  Three slashes means consume
                //  two chars, output one; four means consume three, output
                //  one, etc.
                //

                if ((SlashCount % 2) == 0) {
                    SlashCount--;
                }
                CharCount += SlashCount / 2;
                c += SlashCount;
            }
        } else if (*c == BreakChar) {
            EndArg = TRUE;
        }

        if (ArgCount + 1 < MaxArgs && EndArg) {
            BreakChar = ' ';
            c++;
            while (*c == BreakChar) c++;
            if (*c == '"') {
                BreakChar = '"';
                c++;
            }
            ArgCount++;
        } else {
            CharCount++;

            //
            //  If we hit a break char, we count the argument then.
            //  If we hit end of string, count it here; note we're
            //  only counting it if we counted a character before it
            //  (ie., trailing whitespace is not an arg.)
            //

            c++;

            if (ArgCount < MaxArgs && *c == '\0') {
                ArgCount++;
            }
        }
    }

    *argc = ArgCount;
    if (ArgCount == 0) {
        return NULL;
    }

    ArgvArray = YoriLibReferencedMalloc( (ArgCount * sizeof(YORI_STRING)) + (CharCount + ArgCount) * sizeof(TCHAR));
    if (ArgvArray == NULL) {
        *argc = 0;
        return NULL;
    }

    ReturnStrings = (LPTSTR)(ArgvArray + ArgCount);

    ArgCount = 0;
    YoriLibInitEmptyString(&ArgvArray[ArgCount]);
    ArgvArray[ArgCount].StartOfString = ReturnStrings;
    ArgvArray[ArgCount].MemoryToFree = ArgvArray;
    YoriLibReference(ArgvArray);

    //
    //  Consume all spaces.  After this, we're either at
    //  the end of string, or we have an arg, and it
    //  might start with a quote
    //

    c = CmdLine;
    while (*c == ' ') c++;
    BreakChar = ' ';
    if (*c == '"') {
        BreakChar = '"';
        c++;
    }

    while (*c != '\0') {
        EndArg = FALSE;

        if (*c == '^' && c[1] != '\0' && ApplyCaretAsEscape) {
            *ReturnStrings = *c;
            ReturnStrings++;
            ArgvArray[ArgCount].LengthInChars++;
            c++;
        } else if (*c == '\\') {
            for (SlashCount = 1; c[SlashCount] == '\\'; SlashCount++);
            if (c[SlashCount] == '"') {

                //
                //  Always add one character in the regular path, below.  This
                //  code therefore needs to process each double-slash except
                //  the last one, and advance the c pointer to skip the first
                //  slash of the last pair.  After that can either be a slash
                //  or a double quote, which will be processed as a regular
                //  character below.
                //

                for (CharCount = 2; CharCount < SlashCount; CharCount += 2) {
                    *ReturnStrings = '\\';
                    ReturnStrings++;
                    ArgvArray[ArgCount].LengthInChars++;
                    c += 2;
                }
                c++;
            }
        } else if (*c == BreakChar) {
            EndArg = TRUE;
        }

        if (ArgCount + 1 < MaxArgs && EndArg) {
            *ReturnStrings = '\0';
            ReturnStrings++;
            ArgvArray[ArgCount].LengthAllocated = ArgvArray[ArgCount].LengthInChars + 1;

            BreakChar = ' ';
            c++;
            while (*c == BreakChar) c++;
            if (*c == '"') {
                BreakChar = '"';
                c++;
            }
            if (*c != '\0') {
                ArgCount++;
                YoriLibInitEmptyString(&ArgvArray[ArgCount]);
                ArgvArray[ArgCount].StartOfString = ReturnStrings;
                YoriLibReference(ArgvArray);
                ArgvArray[ArgCount].MemoryToFree = ArgvArray;
            }
        } else {
            *ReturnStrings = *c;
            ReturnStrings++;
            ArgvArray[ArgCount].LengthInChars++;

            //
            //  If we hit a break char, we count the argument then.
            //  If we hit end of string, count it here; note we're
            //  only counting it if we counted a character before it
            //  (ie., trailing whitespace is not an arg.)
            //

            c++;

            if (*c == '\0') {
                *ReturnStrings = '\0';
                ArgvArray[ArgCount].LengthAllocated = ArgvArray[ArgCount].LengthInChars + 1;
            }
        }
    }

    return ArgvArray;
}


// vim:sw=4:ts=4:et:
