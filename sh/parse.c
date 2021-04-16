/**
 * @file /sh/parse.c
 *
 * Parses an expression into component pieces
 *
 * Copyright (c) 2014-2020 Malcolm J. Smith
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
__success(return)
BOOL
YoriShResolveCommandToExecutable(
    __in PYORI_LIBSH_CMD_CONTEXT CmdContext,
    __out PBOOL ExecutableFound
    )
{
    YORI_STRING FoundExecutable;
    YORI_STRING ExpandedCmd;

    YoriLibInitEmptyString(&FoundExecutable);

    YoriShExpandAlias(CmdContext);

    if (!YoriLibExpandHomeDirectories(&CmdContext->ArgV[0], &ExpandedCmd)) {
        YoriLibCloneString(&ExpandedCmd, &CmdContext->ArgV[0]);
    }

    if (YoriLibLocateExecutableInPath(&ExpandedCmd, NULL, NULL, &FoundExecutable)) {

        if (FoundExecutable.LengthInChars > 0) {
            YoriLibFreeStringContents(&CmdContext->ArgV[0]);
            memcpy(&CmdContext->ArgV[0], &FoundExecutable, sizeof(YORI_STRING));
            *ExecutableFound = TRUE;
        } else {
            YoriLibFreeStringContents(&FoundExecutable);
            *ExecutableFound = FALSE;
        }
    } else {
        *ExecutableFound = FALSE;
    }

    YoriLibFreeStringContents(&ExpandedCmd);

    return TRUE;
}

/**
 Expand environment variables that remain in any argument of a command
 context.  Note these can refer to variables that are provided by the
 shell, so this functionality is not in a generic library.

 @param CmdContext Pointer to the command context.  On output, any arguments
        containing defined environment variables are replaced with their
        expanded form.

 @return TRUE to indicate success, FALSE to indicate failure.
 */
BOOLEAN
YoriShExpandEnvironmentInCmdContext(
    __inout PYORI_LIBSH_CMD_CONTEXT CmdContext
    )
{
    DWORD Index;
    DWORD ArgOffset;

    for (Index = 0; Index < CmdContext->ArgC; Index++) {
        YORI_STRING EnvExpandedString;
        ASSERT(YoriLibIsStringNullTerminated(&CmdContext->ArgV[Index]));

        ArgOffset = 0;
        if (Index == CmdContext->CurrentArg) {
            ArgOffset = CmdContext->CurrentArgOffset;
        }
        if (YoriShExpandEnvironmentVariables(&CmdContext->ArgV[Index], &EnvExpandedString, &ArgOffset)) {
            if (EnvExpandedString.StartOfString != CmdContext->ArgV[Index].StartOfString) {
                if (Index == CmdContext->CurrentArg) {
                    CmdContext->CurrentArgOffset = ArgOffset;
                }

                YoriLibFreeStringContents(&CmdContext->ArgV[Index]);
                memcpy(&CmdContext->ArgV[Index], &EnvExpandedString, sizeof(YORI_STRING));
                ASSERT(YoriLibIsStringNullTerminated(&CmdContext->ArgV[Index]));
            }
        }
    }

    return TRUE;
}


// vim:sw=4:ts=4:et:
