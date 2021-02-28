/**
 * @file help/help.c
 *
 * Yori shell display help text
 *
 * Copyright (c) 2017-2020 Malcolm J. Smith
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
 Help text to display to the user.
 */
const
CHAR strHelpHelpText[] =
        "\n"
        "Displays help.\n"
        "\n"
        "HELP [-license]\n";

/**
 Display usage text to the user.
 */
BOOL
HelpHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Help %i.%02i\n"), HELP_VER_MAJOR, HELP_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpHelpText);
    return TRUE;
}

/**
 Text to display to the user about Yori and its tools.
 */
const
CHAR strHelpHelpText1[] =
        "\n"
        "For more information about a command, run <command> /?\n"
        "\n"
        "ALIAS     Displays or updates command aliases\n"
        "ASSOC     Display or edit file associations\n"
        "ATTRIB    Display or edit file attributes\n"
        "BUILTIN   Executes a command explicitly as a builtin\n"
        "CAB       Compress or decompress cabinet archives\n"
        "CAL       Display the calendar\n"
        "CALL      Call a subroutine (only valid in scripts)\n"
        "CHDIR     Changes the current directory\n"
        "CLIP      Manipulate clipboard state including copy and paste\n"
        "CLMP      Multi process compiler wrapper\n"
        "CLS       Clears the console\n"
        "COLOR     Change the active color or all characters on the console\n"
        "COMPACT   Compress or decompress individual files\n"
        "COPY      Copies one or more files\n"
        "CSHOT     Captures previous output on the console and outputs it\n"
        "CUT       Outputs a portion of an input buffer of text\n"
        "CVTVT     Converts text with VT100 color escapes into another format\n"
        "DATE      Outputs the system date and time in a specified format\n"
        "DF        Display disk free space\n"
        "DIR       Enumerate the contents of directories in a traditional way\n"
        "DIRCASE   Update case sensitivity behavior for a directory\n"
        "DIRENV    Apply environment changes from per-directory scripts\n"
        "DOS2UNIX  Convert line endings to UNIX (LF) form\n"
        "DU        Display disk space used within a directory tree\n"
        "ECHO      Outputs text\n"
        "EDIT      Open a file for editing\n"
        "ENV       Set environment variables and launch a program\n"
        "ENVDIFF   Compare the environment to another environment\n"
        ;

/**
 More text to display to the user about Yori and its tools.
 */
const
CHAR strHelpHelpText2[] =
        "ENDLOCAL  Pop a previous saved environment from the stack (only valid after\n"
        "            SETLOCAL)\n"
        "ERASE     Delete one or more files\n"
        "ERR       Display the text for a Windows error code\n"
        "EXIT      Exits the shell\n"
        "EXPR      Evaluate simple arithmetic expressions\n"
        "FALSE     Return false\n"
        "FINFO     Output information about file metadata\n"
        "FG        Display the output of a background job in the foreground\n"
        "FOR       Enumerates through a list of strings or files\n"
        "FSCMP     Test for file system conditions\n"
        "FTYPE     Display or edit file types used in file associations\n"
        "GET       Fetches objects from HTTP and stores them in local files\n"
        "GRPCMP    Returns true if the user is a member of the specified group\n"
        "GOTO      Goto a label in a script\n"
        "HASH      Calculates the hash of a file or set of files\n"
        "HELP      Displays this help text\n"
        "HEXDUMP   Display a stream of data as hex\n"
        "HILITE    Output the contents of one or more files with highlight on lines\n"
        "            matching specified criteria\n"
        "ICONV     Convert the character encoding of one or more files\n"
        "IF        Execute a command to evaluate a condition\n"
        "INCLUDE   Include a script within another script (only valid in scripts)\n"
        "INITOOL   Query or set values in INI files\n"
        "INTCMP    Compare two integer values\n"
        "JOB       Displays or updates background job status\n"
        "KILL      Terminate one or more processes\n"
        "LINES     Count the number of lines in one or more files\n"
        ;

/**
 More text to display to the user about Yori and its tools.
 */
const
CHAR strHelpHelpText3[] =
        "LSOF      Display which processes have a file in use\n"
        "MEM       Display memory usage information\n"
        "MKDIR     Creates directories\n"
        "MKLINK    Creates hardlinks, symbolic links, or junctions\n"
        "MORE      Displays text one page at a time with scrolling\n"
        "MOUNT     Mounts or unmounts an ISO image\n"
        "MOVE      Moves or renames one or more files\n"
        "NICE      Runs a child program at low priority\n"
        "OSVER     Outputs the operating system version in a specified format\n"
        "PATH      Converts relative paths into decomposable full paths\n"
        "PAUSE     Prompt the user to press any key before continuing\n"
        "POPD      Pop a previous current directory from the stack (only valid after\n"
        "            PUSHD)\n"
        "PS        Display a list of processes in the system\n"
        "PUSHD     Push the current directory onto a stack and change to a new directory\n"
        "READLINE  Inputs a line and sends it to output\n"
        "REM       Ignore command\n"
        "RETURN    Return from a subroutine (only valid after CALL)\n"
        "RMDIR     Removes directories\n"
        "SCUT      Create, modify, display or execute Windows shortcuts\n"
        "SDIR      Enumerate directories with color in a compact form\n"
        "SET       Displays or updates environment variables\n"
        "SETLOCAL  Push the current directory and environment onto a saved stack\n"
        "SETVER    Lie about Windows version and launch a program\n"
        "SHIFT     Shift command arguments left by one (only valid in scripts)\n"
        ;

/**
 More text to display to the user about Yori and its tools.
 */
const
CHAR strHelpHelpText4[] =
        "SHUTDN    Shutdown, reboot or logoff the system\n"
        "SLEEP     Waits for a specified number of seconds\n"
        "SLMENU    Display a menu of items from input and output the user selection\n"
        "SPLIT     Split a file into pieces\n"
        "SPONGE    Read input into memory then output, allowing rewrite of input\n"
        "START     Ask the shell to open a file\n"
        "STRCMP    Compare two strings\n"
        "STRIDE    Output lines periodically from input stream\n"
        "SYNC      Flush a file, directory or volume\n"
        "TAIL      Output the final lines of one or more files\n"
        "TEE       Output the contents of standard input to standard output and a file\n"
        "TIMETHIS  Time how long a program takes to execute\n"
        "TITLE     Get or set the console window title\n"
        "TOUCH     Create a file or update its timestamps\n"
        "TRUE      Returns true\n"
        "TYPE      Output the contents of one or more files\n"
        "UNIX2DOS  Convert line endings to Windows (CRLF) form\n"
        "VER       Outputs the Yori version in a specified format\n"
        "VOL       Outputs volume information in a specified format\n"
        "WAIT      Wait for one background job or all jobs to finish executing\n"
        "WHICH     Searches a semicolon delimited environment variable for a file\n"
        "WININFO   Return window information including size or position\n"
        "WINPOS    Set window size or position\n"
        "YCHARMAP  Display the character map\n"
        "YDBG      Debug processes\n"
        "YORI      Launch the Yori shell\n"
        "YPM       Install, uninstall, or update software packages\n"
        "YS        Executes a Yori script\n"
        ;

/**
 Display usage text to the user.
 */
BOOL
HelpText(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpHelpText1);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpHelpText2);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpHelpText3);
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs\n"), strHelpHelpText4);
    return TRUE;
}

#ifdef YORI_BUILTIN
/**
 The main entrypoint for the help builtin command.
 */
#define ENTRYPOINT YoriCmd_YHELP
#else
/**
 The main entrypoint for the help standalone application.
 */
#define ENTRYPOINT ymain
#endif

/**
 The main entrypoint for the help cmdlet.

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
    DWORD i;
    YORI_STRING Arg;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                HelpHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            }
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    HelpText();
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
