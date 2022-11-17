/**
 * @file for/for.c
 *
 * Yori shell enumerate and operate on strings or files
 *
 * Copyright (c) 2017-2019 Malcolm J. Smith
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
#ifdef YORI_BUILTIN
#include <yoricall.h>
#endif

/**
 Help text to display to the user.
 */
const
CHAR strForHelpText[] =
        "Enumerates through a list of strings or files.\n"
        "\n"
        "FOR [-license] [-b] [-c] [-d] [-i <criteria>] [-l] [-p n] [-r]\n"
        "    <var> in (<list>) do <cmd>\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Use cmd as a subshell rather than Yori\n"
        "   -d             Match directories rather than files\n"
        "   -i <criteria>  Only treat match files if they meet criteria, see below\n"
        "   -l             Use (start,step,end) notation for the list\n"
        "   -p <n>         Execute with <n> concurrent processes\n"
        "   -r             Look for matches in subdirectories under the current directory\n"
        "\n"
        " The -i option will match files only if they meet criteria.  This is a\n"
        " semicolon delimited list of entries matching the following form:\n"
        "\n"
        "   [file attribute][operator][criteria]\n";

/**
 Display usage text to the user.
 */
BOOL
ForHelp(VOID)
{

    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("For %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strForHelpText);

    //
    //  Display supported options and operators
    //

    YoriLibFileFiltHelp();
    return TRUE;
}

/**
 State about the currently running processes as well as information required
 to launch any new processes from this program.
 */
typedef struct _FOR_EXEC_CONTEXT {

    /**
     If TRUE, use CMD as a subshell.  If FALSE, use Yori.
     */
    BOOL InvokeCmd;

    /**
     The string that might be found in ArgV which should be changed to contain
     the value of any match.
     */
    PYORI_STRING SubstituteVariable;

    /**
     The number of elements in ArgV.
     */
    DWORD ArgC;

    /**
     The template form of an argv style argument array, before any
     substitution has taken place.
     */
    PYORI_STRING ArgV;

    /**
     The number of processes that this program would like to have concurrently
     running.
     */
    DWORD TargetConcurrentCount;

    /**
     The number of processes that are currently running as a result of this
     program.
     */
    DWORD CurrentConcurrentCount;

    /**
     An array of handles with CurrentConcurrentCount number of valid
     elements.  These correspond to processes that are currently running.
     */
    PHANDLE HandleArray;

    /**
     A list of criteria to filter matches against.
     */
    YORI_LIB_FILE_FILTER Filter;

} FOR_EXEC_CONTEXT, *PFOR_EXEC_CONTEXT;

/**
 Wait for any single process to complete.

 @param ExecContext Pointer to the for exec context containing information
        about currently running processes.
 */
VOID
ForWaitForProcessToComplete(
    __in PFOR_EXEC_CONTEXT ExecContext
    )
{
    DWORD Result;
    DWORD Index;
    DWORD Count;

    Result = WaitForMultipleObjectsEx(ExecContext->CurrentConcurrentCount, ExecContext->HandleArray, FALSE, INFINITE, FALSE);

    ASSERT(Result < (WAIT_OBJECT_0 + ExecContext->CurrentConcurrentCount));

    Index = Result - WAIT_OBJECT_0;

    ASSERT(Index < ExecContext->CurrentConcurrentCount);

    CloseHandle(ExecContext->HandleArray[Index]);

    for (Count = Index; Count < ExecContext->CurrentConcurrentCount - 1; Count++) {

#if defined(_MSC_VER) && (_MSC_VER >= 1700)
        //
        //  The "correct" annotation here would describe that HandleArray is
        //  of TargetConcurrentCount elements, CurrentConcurrentCount of which
        //  are readable, and CurrentConcurrentCount needs to be less than or
        //  equal to TargetConcurrentCount.  This is something that I don't
        //  see a way to express as of this writing.
        //
#pragma warning(suppress: 6001)
#endif
        ExecContext->HandleArray[Count] = ExecContext->HandleArray[Count + 1];
    }

    ExecContext->CurrentConcurrentCount--;
}

/**
 Add a quote to a substring within an argument.  This is used because the
 string may contain backquotes, where any quotes need to only surround the
 text, without quoting the backquote.

 @param Arg Pointer to the string that contains the entire argument.  This
        argument can be reallocated within this routine as needed.

 @param ArgContext Pointer to the argument context, indicating whether the
        argument is quoted.

 @param Section Pointer to a substring within Arg.

 @param WhiteSpaceInSection If TRUE, the substring contains white space,
        indicating the substring requires quoting.

 @param CurrentPosition This function is called when processing an argument,
        and the caller describes its position with a string whose start is
        within the argument and whose length is the remainder of characters
        to process.  Because this function can reallocate the argument, this
        position must be updated, including to reflect any modifications
        performed by this routine that would adjust the position relative to
        the newly created argument.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
ForQuoteSectionIfNeeded(
    __in PYORI_STRING Arg,
    __in PYORI_LIBSH_ARG_CONTEXT ArgContext,
    __in PYORI_STRING Section,
    __in BOOLEAN WhiteSpaceInSection,
    __inout PYORI_STRING CurrentPosition
    )
{
    DWORD Offset;

    //
    //  This routine will update the current position and depends upon the
    //  current position being relative to the current argument, and not
    //  having any extra memory to consider.
    //

    ASSERT(CurrentPosition->MemoryToFree == NULL);

    Offset = (DWORD)(CurrentPosition->StartOfString - Arg->StartOfString);

    if (!ArgContext->Quoted && WhiteSpaceInSection && Section->LengthInChars > 0) {

        //
        //  If there's whitespace in it, the section should have characters
        //

        ASSERT(Section->LengthInChars > 0);

        if (Section->StartOfString == Arg->StartOfString &&
            Section->LengthInChars == Arg->LengthInChars) {

            ArgContext->Quoted = TRUE;
            ArgContext->QuoteTerminated = TRUE;
        } else {
            YORI_STRING NewArg;
            YORI_STRING Prefix;
            YORI_STRING Suffix;
            DWORD ExtraOffsetChars;

            YoriLibInitEmptyString(&Prefix);
            YoriLibInitEmptyString(&Suffix);

            Prefix.StartOfString = Arg->StartOfString;
            Prefix.LengthInChars = (DWORD)(Section->StartOfString - Arg->StartOfString);

            Suffix.StartOfString = Section->StartOfString + Section->LengthInChars;
            Suffix.LengthInChars = Arg->LengthInChars - (DWORD)(Suffix.StartOfString - Arg->StartOfString);

            if (!YoriLibAllocateString(&NewArg, Arg->LengthInChars + 3)) {
                return FALSE;
            }

            ExtraOffsetChars = 0;
            if (Offset >= Prefix.LengthInChars + Section->LengthInChars) {
                ExtraOffsetChars += 1;
            }

            if (Offset >= Prefix.LengthInChars) {
                ExtraOffsetChars += 1;
            }

            NewArg.LengthInChars = YoriLibSPrintf(NewArg.StartOfString, _T("%y\"%y\"%y"), &Prefix, Section, &Suffix);
            YoriLibFreeStringContents(Arg);
            memcpy(Arg, &NewArg, sizeof(YORI_STRING));

            CurrentPosition->StartOfString = NewArg.StartOfString + Offset + ExtraOffsetChars;
            CurrentPosition->LengthInChars = NewArg.LengthInChars - Offset - ExtraOffsetChars;
        }
    }

    return TRUE;
}

/**
 Search through arguments for a character that would normally implicitly
 terminate an argument.  This includes shell operators like redirectors,
 pipes, or backquotes.  When these are encountered, split the argument at
 that point.  This means when quotes are added based on arguments with
 spaces, they will only span the text with spaces, not any operators.

 If there are characters within an argument, such as a backquote, the
 argument cannot be split without altering its meaning.  In this case, real
 quotes need to be inserted around a substring without splitting the
 argument.

 @param CmdContext Pointer to the command context, containing the number of
        arguments, argument values, and quote state of each argument.  These
        values may be reallocated or changed within this routine.  Note that
        this routine assumes that the contents of the strings are not part of
        these allocations, so any new string allocations are still pointing
        to the previous string locations.

 @return TRUE to indicate successful completion, FALSE to indicate failure.
 */
BOOLEAN
ForBreakArgumentsAsNeeded(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext
    )
{
    DWORD ArgCount;
    DWORD ArgIndex;
    DWORD ExistingArg;
    DWORD CharsToConsume;
    DWORD InitialArgOffset;
    DWORD BraceNestingLevel;
    PYORI_STRING Arg;
    YORI_STRING Char;
    YORI_STRING Section;
    BOOLEAN TerminateNextArg;
    BOOL QuoteOpen = FALSE;
    BOOLEAN LookingForFirstQuote = FALSE;
    BOOLEAN WhiteSpaceInSection = FALSE;

    YoriLibInitEmptyString(&Char);
    YoriLibInitEmptyString(&Section);
    ArgCount = CmdContext->ArgC;
    Arg = CmdContext->ArgV;
    InitialArgOffset = 0;
    BraceNestingLevel = 0;

    for (ArgIndex = 0; ArgIndex < ArgCount; ArgIndex++) {

        Char.StartOfString = Arg[ArgIndex].StartOfString;
        Char.LengthInChars = Arg[ArgIndex].LengthInChars;

        if (InitialArgOffset > 0) {
            Char.StartOfString = Char.StartOfString + InitialArgOffset;
            Char.LengthInChars = Char.LengthInChars - InitialArgOffset;
        }

        Section.StartOfString = Char.StartOfString;

        //
        //  Remove leading spaces
        //

        while (Char.LengthInChars > 0) {
            if (Char.StartOfString[0] == ' ') {
                WhiteSpaceInSection = TRUE;
                Char.StartOfString++;
                Char.LengthInChars--;
            } else {
                break;
            }
        }

        if (CmdContext->ArgContexts[ArgIndex].Quoted) {
            LookingForFirstQuote = TRUE;
            QuoteOpen = TRUE;
        } else if (Char.LengthInChars > 0 && Char.StartOfString[0] == '"') {
            LookingForFirstQuote = TRUE;
        } else {
            LookingForFirstQuote = FALSE;
        }

        //
        //  Go through the arg looking for operators that would indicate
        //  an argument break.  Note that unlike any normal case, spaces
        //  do not constitute an argument break.
        //

        while (Char.LengthInChars > 0) {

            if (YoriLibIsEscapeChar(Char.StartOfString[0])) {
                Char.StartOfString++;
                Char.LengthInChars--;
                if (Char.LengthInChars > 0) {
                    Char.StartOfString++;
                    Char.LengthInChars--;
                }
                continue;
            }

            if (Char.StartOfString[0] == '"' && QuoteOpen && LookingForFirstQuote) {
                QuoteOpen = FALSE;
                LookingForFirstQuote = FALSE;
                Char.StartOfString++;
                Char.LengthInChars--;
                continue;
            }

            if (Char.StartOfString[0] == '"') {
                QuoteOpen = !QuoteOpen;
                if (LookingForFirstQuote) {
                    Char.StartOfString++;
                    Char.LengthInChars--;
                    continue;
                }
            }

            if (!QuoteOpen) {

                //
                //  MSFIX: Current thinking goes something like, maintain a
                //  section string that starts at the beginning of the arg
                //  and is extended up to a terminating backquote, or the end
                //  of the string.  As we go, check if a whitespace is
                //  detected.  At the end of the section, if the arg doesn't
                //  already have quotes, insert them around that section.
                //
                //  This could be optimized in the common case (section ==
                //  arg) by setting the ArgContext flag.
                //
                //  Note that backquote accounting needs to happen across all
                //  arguments, although we "know" that a quoted arg doesn't
                //  apply backquote updates.
                //

                if (Char.StartOfString[0] == '$' &&
                    Char.LengthInChars >= 2 &&
                    Char.StartOfString[1] == '(') {

                    Section.LengthInChars = (DWORD)(Char.StartOfString - Section.StartOfString);
                    ForQuoteSectionIfNeeded(&CmdContext->ArgV[ArgIndex],
                                            &CmdContext->ArgContexts[ArgIndex],
                                            &Section,
                                            WhiteSpaceInSection,
                                            &Char);
                    Section.StartOfString = &Char.StartOfString[2];
                    WhiteSpaceInSection = FALSE;
                    BraceNestingLevel++;
                } else if (Char.StartOfString[0] == ')' &&
                           BraceNestingLevel > 0) {

                    Section.LengthInChars = (DWORD)(Char.StartOfString - Section.StartOfString);
                    ForQuoteSectionIfNeeded(&CmdContext->ArgV[ArgIndex],
                                            &CmdContext->ArgContexts[ArgIndex],
                                            &Section,
                                            WhiteSpaceInSection,
                                            &Char);
                    Section.StartOfString = &Char.StartOfString[1];
                    WhiteSpaceInSection = FALSE;
                    BraceNestingLevel--;
                } else if (Char.StartOfString[0] == '`') {
                    Section.LengthInChars = (DWORD)(Char.StartOfString - Section.StartOfString);
                    ForQuoteSectionIfNeeded(&CmdContext->ArgV[ArgIndex],
                                            &CmdContext->ArgContexts[ArgIndex],
                                            &Section,
                                            WhiteSpaceInSection,
                                            &Char);
                    Section.StartOfString = &Char.StartOfString[1];
                    WhiteSpaceInSection = FALSE;
                } else if (Char.StartOfString[0] == ' ') {
                    WhiteSpaceInSection = TRUE;
                }

                if (!CmdContext->ArgContexts[ArgIndex].Quoted &&
                    YoriLibShIsArgumentSeperator(&Char, &CharsToConsume, &TerminateNextArg)) {

                    DWORD NewArgCount;
                    YORI_LIBSH_CMD_CONTEXT NewCmd;

                    Section.LengthInChars = (DWORD)(Char.StartOfString - Section.StartOfString);
                    ForQuoteSectionIfNeeded(&CmdContext->ArgV[ArgIndex],
                                            &CmdContext->ArgContexts[ArgIndex],
                                            &Section,
                                            WhiteSpaceInSection,
                                            &Char);
                    WhiteSpaceInSection = FALSE;

                    //
                    //  If the next arg is terminated but is already
                    //  terminating, don't do it twice.
                    //

                    if (TerminateNextArg && CharsToConsume == Char.LengthInChars) {
                        TerminateNextArg = FALSE;
                    }

                    //
                    //  At this point we need to reallocate the array to add
                    //  either one or two new args.  The current arg ends at
                    //  the current offset.  The next arg starts at the
                    //  current offset.  If TerminateNextArg is TRUE, the
                    //  next arg is CharsToConsume length and the next next
                    //  arg starts after that.  We should advance the
                    //  arg index to the final arg generated here, ensure
                    //  Char.LengthInChars is zero, and continue.
                    //

                    if (TerminateNextArg) {
                        NewArgCount = ArgCount + 2;
                    } else {
                        NewArgCount = ArgCount + 1;
                    }

                    if (!YoriLibShAllocateArgCount(&NewCmd, NewArgCount, 0, NULL)) {
                        return FALSE;
                    }

                    for (ExistingArg = 0; ExistingArg <= ArgIndex; ExistingArg++) {
                        YoriLibShCopyArg(CmdContext, ExistingArg, &NewCmd, ExistingArg);
                    }

                    NewCmd.ArgV[ArgIndex].LengthInChars = (DWORD)(Char.StartOfString - Arg[ArgIndex].StartOfString);
                    YoriLibShCheckIfArgNeedsQuotes(&NewCmd, ArgIndex);
                    YoriLibInitEmptyString(&NewCmd.ArgV[ArgIndex + 1]);
                    NewCmd.ArgV[ArgIndex + 1].StartOfString = Char.StartOfString;

                    if (TerminateNextArg) {
                        NewCmd.ArgV[ArgIndex + 1].LengthInChars = CharsToConsume;
                        YoriLibShCheckIfArgNeedsQuotes(&NewCmd, ArgIndex + 1);
                        YoriLibInitEmptyString(&NewCmd.ArgV[ArgIndex + 2]);
                        NewCmd.ArgV[ArgIndex + 2].StartOfString = &Char.StartOfString[CharsToConsume];
                        NewCmd.ArgV[ArgIndex + 2].LengthInChars = Char.LengthInChars - CharsToConsume;
                        YoriLibShCheckIfArgNeedsQuotes(&NewCmd, ArgIndex + 2);
                         for (ExistingArg = ArgIndex + 1; ExistingArg < ArgCount; ExistingArg++) {
                             YoriLibShCopyArg(CmdContext, ExistingArg, &NewCmd, ExistingArg + 2);
                         }

                        ArgIndex++;

                    } else {
                        NewCmd.ArgV[ArgIndex + 1].LengthInChars = Char.LengthInChars;
                        YoriLibShCheckIfArgNeedsQuotes(&NewCmd, ArgIndex + 1);
                        for (ExistingArg = ArgIndex + 1; ExistingArg < ArgCount; ExistingArg++) {
                            YoriLibShCopyArg(CmdContext, ExistingArg, &NewCmd, ExistingArg + 1);
                        }
                        InitialArgOffset = CharsToConsume;
                    }

                    //
                    //  MSFIX: Need to update Section and Char to refer to
                    //  their respective locations in the new argument.  Note
                    //  that the argument index may have changed, and the
                    //  argument length may have changed.
                    //
                    //  The nightmare case is something like:
                    //  %i>>foo
                    //
                    //  Where we want:
                    //  "My File">>foo
                    //
                    //  Meaning that the section is being terminated by the
                    //  argument seperator but the previous section needs to
                    //  be resolved.
                    //

                    WhiteSpaceInSection = FALSE;

                    YoriLibShFreeCmdContext(CmdContext);
                    CmdContext->ArgC = NewArgCount;
                    CmdContext->ArgV = NewCmd.ArgV;
                    CmdContext->ArgContexts = NewCmd.ArgContexts;
                    CmdContext->MemoryToFree = NewCmd.MemoryToFree;

                    ArgCount = NewArgCount;
                    Arg = NewCmd.ArgV;

                    break;
                }
            }

            Char.StartOfString++;
            Char.LengthInChars--;
        }

        if (WhiteSpaceInSection) {
            Section.LengthInChars = (DWORD)(Char.StartOfString - Section.StartOfString);
            ForQuoteSectionIfNeeded(&CmdContext->ArgV[ArgIndex],
                                    &CmdContext->ArgContexts[ArgIndex],
                                    &Section,
                                    WhiteSpaceInSection,
                                    &Char);
            WhiteSpaceInSection = FALSE;
        }
    }

    return TRUE;
}

/**
 Execute a new command in response to a newly matched element.

 @param Match The match that was found from the set.

 @param ExecContext The current state of child processes and information about
        the arguments for any new child process.
 */
VOID
ForExecuteCommand(
    __in PYORI_STRING Match,
    __in PFOR_EXEC_CONTEXT ExecContext
    )
{
    DWORD ArgsNeeded = ExecContext->ArgC;
    DWORD PrefixArgCount;
    DWORD FoundOffset;
    DWORD Count;
    DWORD SubstitutesFound;
    DWORD ArgLengthNeeded;
    YORI_STRING OldArg;
    YORI_STRING NewArgWritePoint;
    YORI_STRING CmdLine;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    YORI_LIBSH_CMD_CONTEXT NewCmd;

    YoriLibInitEmptyString(&CmdLine);

#ifdef YORI_BUILTIN
    if (!ExecContext->InvokeCmd &&
        ExecContext->TargetConcurrentCount == 1) {
        PrefixArgCount = 0;
    } else {
        PrefixArgCount = 2;
    }
#else
    PrefixArgCount = 2;
#endif

    ArgsNeeded += PrefixArgCount;

    if (!YoriLibShAllocateArgCount(&NewCmd, ArgsNeeded, 0, NULL)) {
        return;
    }

    //
    //  If a full path is specified in the environment, use it.  If not,
    //  use a file name only and let PATH evaluation find an interpreter.
    //

    if (PrefixArgCount > 0) {
        if (ExecContext->InvokeCmd) {
            ArgLengthNeeded = GetEnvironmentVariable(_T("COMSPEC"), NULL, 0);
            if (ArgLengthNeeded != 0) {
                if (YoriLibAllocateString(&NewCmd.ArgV[0], ArgLengthNeeded)) {
                    NewCmd.ArgV[0].LengthInChars = GetEnvironmentVariable(_T("COMSPEC"), NewCmd.ArgV[0].StartOfString, NewCmd.ArgV[0].LengthAllocated);
                    if (NewCmd.ArgV[0].LengthInChars == 0) {
                        YoriLibFreeStringContents(&NewCmd.ArgV[0]);
                    }
                }
            }
            if (NewCmd.ArgV[0].LengthInChars == 0) {
                YoriLibConstantString(&NewCmd.ArgV[0], _T("cmd.exe"));
            }
        } else {
            ArgLengthNeeded = GetEnvironmentVariable(_T("YORISPEC"), NULL, 0);
            if (ArgLengthNeeded != 0) {
                if (YoriLibAllocateString(&NewCmd.ArgV[0], ArgLengthNeeded)) {
                    NewCmd.ArgV[0].LengthInChars = GetEnvironmentVariable(_T("YORISPEC"), NewCmd.ArgV[0].StartOfString, NewCmd.ArgV[0].LengthAllocated);
                    if (NewCmd.ArgV[0].LengthInChars == 0) {
                        YoriLibFreeStringContents(&NewCmd.ArgV[0]);
                    }
                }
            }
            if (NewCmd.ArgV[0].LengthInChars == 0) {
                YoriLibConstantString(&NewCmd.ArgV[0], _T("yori.exe"));
            }
        }
        YoriLibConstantString(&NewCmd.ArgV[1], _T("/c"));
    }

    //
    //  Go through all of the arguments, count the number of variable
    //  substitutions to apply, allocate a string to contain the subtituted
    //  value, and copy the string into the new allocation applying
    //  substitutions.
    //

    for (Count = 0; Count < ExecContext->ArgC; Count++) {
        YoriLibInitEmptyString(&OldArg);
        OldArg.StartOfString = ExecContext->ArgV[Count].StartOfString;
        OldArg.LengthInChars = ExecContext->ArgV[Count].LengthInChars;
        SubstitutesFound = 0;

        //
        //  MSFIX: Ideally this would have ArgContexts for the input arguments
        //  and copy quote state of those rather than trying to infer whether
        //  a quote is needed.  It makes complete sense for a user to specify
        //  "%i" or similar (with quotes) as an input and expect them to be
        //  applied universally.  For the external version of for, this could
        //  be done by passing GetCommandLine() to YoriSh, but the builtin
        //  version requires a little more thought.
        //

        YoriLibInitEmptyString(&NewCmd.ArgV[Count + PrefixArgCount]);
        NewCmd.ArgV[Count + PrefixArgCount].StartOfString = OldArg.StartOfString;
        NewCmd.ArgV[Count + PrefixArgCount].LengthInChars = OldArg.LengthInChars;

        YoriLibShCheckIfArgNeedsQuotes(&NewCmd, Count + PrefixArgCount);

        while (YoriLibFindFirstMatchingSubstring(&OldArg, 1, ExecContext->SubstituteVariable, &FoundOffset)) {
            SubstitutesFound++;
            OldArg.StartOfString += FoundOffset + 1;
            OldArg.LengthInChars -= FoundOffset + 1;
        }

        ArgLengthNeeded = ExecContext->ArgV[Count].LengthInChars + SubstitutesFound * Match->LengthInChars - SubstitutesFound * ExecContext->SubstituteVariable->LengthInChars + 1;
        if (!YoriLibAllocateString(&NewCmd.ArgV[Count + PrefixArgCount], ArgLengthNeeded)) {
            goto Cleanup;
        }

        YoriLibInitEmptyString(&NewArgWritePoint);
        NewArgWritePoint.StartOfString = NewCmd.ArgV[Count + PrefixArgCount].StartOfString;
        NewArgWritePoint.LengthAllocated = NewCmd.ArgV[Count + PrefixArgCount].LengthAllocated;

        YoriLibInitEmptyString(&OldArg);
        OldArg.StartOfString = ExecContext->ArgV[Count].StartOfString;
        OldArg.LengthInChars = ExecContext->ArgV[Count].LengthInChars;

        while (TRUE) {
            if (YoriLibFindFirstMatchingSubstring(&OldArg, 1, ExecContext->SubstituteVariable, &FoundOffset)) {
                memcpy(NewArgWritePoint.StartOfString, OldArg.StartOfString, FoundOffset * sizeof(TCHAR));
                NewArgWritePoint.StartOfString += FoundOffset;
                NewArgWritePoint.LengthAllocated -= FoundOffset;
                memcpy(NewArgWritePoint.StartOfString, Match->StartOfString, Match->LengthInChars * sizeof(TCHAR));
                NewArgWritePoint.StartOfString += Match->LengthInChars;
                NewArgWritePoint.LengthAllocated -= Match->LengthInChars;

                OldArg.StartOfString += FoundOffset + ExecContext->SubstituteVariable->LengthInChars;
                OldArg.LengthInChars -= FoundOffset + ExecContext->SubstituteVariable->LengthInChars;
            } else {
                memcpy(NewArgWritePoint.StartOfString, OldArg.StartOfString, OldArg.LengthInChars * sizeof(TCHAR));
                NewArgWritePoint.StartOfString += OldArg.LengthInChars;
                NewArgWritePoint.LengthAllocated -= OldArg.LengthInChars;
                NewArgWritePoint.StartOfString[0] = '\0';

                NewCmd.ArgV[Count + PrefixArgCount].LengthInChars = (DWORD)(NewArgWritePoint.StartOfString - NewCmd.ArgV[Count + PrefixArgCount].StartOfString);
                ASSERT(NewCmd.ArgV[Count + PrefixArgCount].LengthInChars < NewCmd.ArgV[Count + PrefixArgCount].LengthAllocated);
                ASSERT(YoriLibIsStringNullTerminated(&NewCmd.ArgV[Count + PrefixArgCount]));
                break;
            }
        }
    }

    //
    //  Split arguments where meaning has changed. This is because when
    //  converting the arguments into a combined string, quotes will be
    //  inserted around arguments containing spaces.  However, the location
    //  of the quotes matters:
    //
    //  My Text>foo
    //  should be
    //  "My Text">foo
    //
    //  `My Text`
    //  should be
    //  `"My Text"`
    //
    //  etc.
    //

    ForBreakArgumentsAsNeeded(&NewCmd);

    if (!YoriLibShBuildCmdlineFromCmdContext(&NewCmd, &CmdLine, FALSE, NULL, NULL)) {
        goto Cleanup;
    }

#ifdef YORI_BUILTIN
    if (PrefixArgCount == 0) {
        YoriCallExecuteExpression(&CmdLine);
        goto Cleanup;
    }
#endif

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (!CreateProcess(NULL, CmdLine.StartOfString, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &StartupInfo, &ProcessInfo)) {
        DWORD LastError = GetLastError();
        LPTSTR ErrText = YoriLibGetWinErrorText(LastError);
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: execution failed: %s"), ErrText);
        YoriLibFreeWinErrorText(ErrText);
        goto Cleanup;
    }

    CloseHandle(ProcessInfo.hThread);

    ExecContext->HandleArray[ExecContext->CurrentConcurrentCount] = ProcessInfo.hProcess;
    ExecContext->CurrentConcurrentCount++;

    if (ExecContext->CurrentConcurrentCount == ExecContext->TargetConcurrentCount) {
        ForWaitForProcessToComplete(ExecContext);
    }

Cleanup:

    YoriLibShFreeCmdContext(&NewCmd);
    YoriLibFreeStringContents(&CmdLine);
}

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Recursion depth, ignored in this application.

 @param Context The current state of the program and information needed to
        launch new child processes.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
ForFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PVOID Context
    )
{
    PFOR_EXEC_CONTEXT ExecContext;

    UNREFERENCED_PARAMETER(Depth);
    UNREFERENCED_PARAMETER(FileInfo);

    ExecContext = (PFOR_EXEC_CONTEXT)Context;

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    if (!YoriLibFileFiltCheckFilterMatch(&ExecContext->Filter, FilePath, FileInfo)) {
        return TRUE;
    }

    ForExecuteCommand(FilePath, ExecContext);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the for builtin command.
 */
#define ENTRYPOINT YoriCmd_FOR
#else
/**
 The main entrypoint for the for standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the for cmdlet.

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
    YORI_STRING Arg;
    BOOL LeftBraceOpen;
    BOOL RequiresExpansion;
    BOOL MatchDirectories;
    BOOL Recurse;
    BOOL BasicEnumeration;
    BOOL StepMode;
    DWORD CharIndex;
    DWORD MatchFlags;
    DWORD StartArg = 0;
    DWORD ListArg = 0;
    DWORD CmdArg = 0;
    DWORD ArgIndex;
    DWORD i;
    FOR_EXEC_CONTEXT ExecContext;

    ZeroMemory(&ExecContext, sizeof(ExecContext));

    ExecContext.TargetConcurrentCount = 1;
    ExecContext.CurrentConcurrentCount = 0;
    MatchDirectories = FALSE;
    Recurse = FALSE;
    StepMode = FALSE;
    BasicEnumeration = FALSE;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                ForHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2019"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c")) == 0) {
                ExecContext.InvokeCmd = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("d")) == 0) {
                MatchDirectories = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("i")) == 0) {
                if (i + 1 < ArgC) {
                    YORI_STRING ErrorSubstring;
                    YORI_LIB_FILE_FILTER Filter;
                    YoriLibInitEmptyString(&ErrorSubstring);

                    if (!YoriLibFileFiltParseFilterString(&Filter, &ArgV[i + 1], &ErrorSubstring)) {
                        if (ErrorSubstring.LengthInChars > 0) {
                            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: error parsing filter string '%y' at '%y'\n"), &ArgV[i + 1], &ErrorSubstring);
                        } else {
                            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: error parsing filter string '%y'\n"), &ArgV[i + 1]);
                        }
                        goto cleanup_and_exit;
                    }
                    memcpy(&ExecContext.Filter, &Filter, sizeof(YORI_LIB_FILE_FILTER));
                    i++;
                    ArgumentUnderstood = TRUE;
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("l")) == 0) {
                StepMode = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("p")) == 0) {
                if (i + 1 < ArgC) {
                    LONGLONG LlNumberProcesses = 0;
                    DWORD CharsConsumed = 0;
                    if (YoriLibStringToNumber(&ArgV[i + 1], TRUE, &LlNumberProcesses, &CharsConsumed) &&
                        CharsConsumed > 0) {

                        ExecContext.TargetConcurrentCount = (DWORD)LlNumberProcesses;
                        ArgumentUnderstood = TRUE;
                        if (ExecContext.TargetConcurrentCount < 1) {
                            ExecContext.TargetConcurrentCount = 1;
                        }
                        i++;
                    }
                }
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("r")) == 0) {
                Recurse = TRUE;
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
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0 || StartArg == ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: missing argument\n"));
        goto cleanup_and_exit;
    }

    ExecContext.SubstituteVariable = &ArgV[StartArg];

    //
    //  We need at least "%i in (*) do cmd"
    //

    if (ArgC < StartArg + 4) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: missing argument\n"));
        goto cleanup_and_exit;
    }

    if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[StartArg + 1], _T("in")) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: 'in' not found\n"));
        goto cleanup_and_exit;
    }

    if (YoriLibCompareStringWithLiteralInsensitiveCount(&ArgV[StartArg + 2], _T("("), 1) != 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: left bracket not found\n"));
        goto cleanup_and_exit;
    }

    LeftBraceOpen = TRUE;

    //
    //  Walk through all arguments looking for a closing brace, then looking
    //  for "do".  Once we've done finding both, we have a trailing command
    //  string
    //

    ListArg = StartArg + 2;
    for (ArgIndex = ListArg; ArgIndex < ArgC; ArgIndex++) {
        if (LeftBraceOpen) {
            if (ArgV[ArgIndex].LengthInChars > 0 && ArgV[ArgIndex].StartOfString[ArgV[ArgIndex].LengthInChars - 1] == ')') {
                LeftBraceOpen = FALSE;
            }
        } else {
            if (YoriLibCompareStringWithLiteralInsensitive(&ArgV[ArgIndex], _T("do")) == 0) {
                CmdArg = ArgIndex + 1;
                break;
            }
        }
    }

    if (CmdArg == 0) {
        if (LeftBraceOpen) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: right bracket not found\n"));
            goto cleanup_and_exit;
        } else {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: do not found\n"));
            goto cleanup_and_exit;
        }
    }

    if (CmdArg >= ArgC) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: command not found\n"));
        goto cleanup_and_exit;
    }

    ExecContext.ArgC = ArgC - CmdArg;
    ExecContext.ArgV = &ArgV[CmdArg];
    ExecContext.HandleArray = YoriLibMalloc(ExecContext.TargetConcurrentCount * sizeof(HANDLE));
    if (ExecContext.HandleArray == NULL) {
        goto cleanup_and_exit;
    }

    MatchFlags = 0;
    if (MatchDirectories) {
        MatchFlags = YORILIB_FILEENUM_RETURN_DIRECTORIES;
    } else {
        MatchFlags = YORILIB_FILEENUM_RETURN_FILES;
    }

    if (Recurse) {
        MatchFlags |= YORILIB_FILEENUM_RECURSE_AFTER_RETURN | YORILIB_FILEENUM_RECURSE_PRESERVE_WILD;
    }
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    if (StepMode) {
        LONGLONG Start;
        LONGLONG Step;
        LONGLONG End;
        LONGLONG Current;
        DWORD CharsConsumed;
        YORI_STRING FoundMatch;
        YORI_STRING Criteria;

        if (!YoriLibBuildCmdlineFromArgcArgv(CmdArg - ListArg, &ArgV[ListArg], FALSE, TRUE, &Criteria)) {
            goto cleanup_and_exit;
        }

        //
        //  Remove the brackets
        //

        Criteria.StartOfString++;
        Criteria.LengthInChars -= 2;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the start value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &Start, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        Criteria.StartOfString += CharsConsumed;
        Criteria.LengthInChars -= CharsConsumed;
        YoriLibTrimSpaces(&Criteria);

        if (Criteria.StartOfString[0] != ',') {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: seperator not found\n"));

            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        Criteria.StartOfString += 1;
        Criteria.LengthInChars -= 1;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the step value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &Step, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        Criteria.StartOfString += CharsConsumed;
        Criteria.LengthInChars -= CharsConsumed;
        YoriLibTrimSpaces(&Criteria);

        if (Criteria.StartOfString[0] != ',') {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: seperator not found\n"));

            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        Criteria.StartOfString += 1;
        Criteria.LengthInChars -= 1;
        YoriLibTrimSpaces(&Criteria);

        //
        //  Get the end value
        //

        if (!YoriLibStringToNumber(&Criteria, FALSE, &End, &CharsConsumed)) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("for: argument not numeric\n"));
            YoriLibFreeStringContents(&Criteria);
            goto cleanup_and_exit;
        }

        YoriLibFreeStringContents(&Criteria);

        if (!YoriLibAllocateString(&FoundMatch, 32)) {
            goto cleanup_and_exit;
        }

        Current = Start;

        do {
            if (Step > 0) {
                if (Current > End) {
                    break;
                }
            } else if (Step < 0) {
                if (Current < End) {
                    break;
                }
            }

            YoriLibNumberToString(&FoundMatch, Current, 10, 0, '\0');
            ForExecuteCommand(&FoundMatch, &ExecContext);
            Current += Step;
        } while(TRUE);

        YoriLibFreeStringContents(&FoundMatch);

    } else {

        for (ArgIndex = ListArg; ArgIndex < CmdArg - 1; ArgIndex++) {
            YORI_STRING ThisMatch;

            YoriLibInitEmptyString(&ThisMatch);

            ThisMatch.StartOfString = ArgV[ArgIndex].StartOfString;
            ThisMatch.LengthInChars = ArgV[ArgIndex].LengthInChars;
            ThisMatch.LengthAllocated = ArgV[ArgIndex].LengthAllocated;

            if (ArgIndex == ListArg) {
                ThisMatch.StartOfString++;
                ThisMatch.LengthInChars--;
                ThisMatch.LengthAllocated--;
            }

            if (ArgIndex == CmdArg - 2) {
                LPTSTR NullTerminatedString;

                ThisMatch.LengthInChars--;
                ThisMatch.LengthAllocated--;
                NullTerminatedString = YoriLibCStringFromYoriString(&ThisMatch);
                if (NullTerminatedString == NULL) {
                    goto cleanup_and_exit;
                }
                ThisMatch.MemoryToFree = NullTerminatedString;
                ThisMatch.StartOfString = NullTerminatedString;
            }

            RequiresExpansion = FALSE;
            if (Recurse) {
                RequiresExpansion = TRUE;
            } else {
                for (CharIndex = 0; CharIndex < ThisMatch.LengthInChars; CharIndex++) {
                    if (ThisMatch.StartOfString[CharIndex] == '*' ||
                        ThisMatch.StartOfString[CharIndex] == '?') {

                        RequiresExpansion = TRUE;
                        break;
                    }
                    if (!BasicEnumeration) {
                        if (ThisMatch.StartOfString[CharIndex] == '[' ||
                            ThisMatch.StartOfString[CharIndex] == '{') {

                            RequiresExpansion = TRUE;
                            break;
                        }
                    }
                }
            }

            if (ThisMatch.LengthInChars > 0) {

                if (RequiresExpansion) {
                    YoriLibForEachFile(&ThisMatch, MatchFlags, 0, ForFileFoundCallback, NULL, &ExecContext);
                } else {
                    ForExecuteCommand(&ThisMatch, &ExecContext);
                }
            }

            //
            //  Because MemoryToFree is not normally populated, this only
            //  really frees where the memory was double buffered
            //

            YoriLibFreeStringContents(&ThisMatch);
        }
    }

    while (ExecContext.CurrentConcurrentCount > 0) {
        ForWaitForProcessToComplete(&ExecContext);
    }

    YoriLibFileFiltFreeFilter(&ExecContext.Filter);
    YoriLibFree(ExecContext.HandleArray);

    return EXIT_SUCCESS;

cleanup_and_exit:

    YoriLibFileFiltFreeFilter(&ExecContext.Filter);

    return EXIT_FAILURE;
}

// vim:sw=4:ts=4:et:
