/**
 * @file sh/yoristd.c
 *
 * Yori table of supported builtins for the regular build of Yori
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

#include "yori.h"

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_ALIAS;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_BUILTIN;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_CHDIR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_COLOR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_DIRENV;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_EXIT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_FALSE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_FG;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_FOR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_HISTORY;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_IF;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_JOB;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_NICE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_PUSHD;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_REM;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_SET;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_SETLOCAL;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_TRUE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_VER;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_WAIT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YS;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_Z;

/**
 The list of builtin commands supported by this build of Yori.
 */
CONST YORI_SH_BUILTIN_NAME_MAPPING
YoriShBuiltins[] = {
                    {_T("ALIAS"),     YoriCmd_ALIAS},
                    {_T("BUILTIN"),   YoriCmd_BUILTIN},
                    {_T("CHDIR"),     YoriCmd_CHDIR},
                    {_T("COLOR"),     YoriCmd_COLOR},
                    {_T("DIRENV"),    YoriCmd_DIRENV},
                    {_T("EXIT"),      YoriCmd_EXIT},
                    {_T("FALSE"),     YoriCmd_FALSE},
                    {_T("FG"),        YoriCmd_FG},
                    {_T("FOR"),       YoriCmd_FOR},
                    {_T("HISTORY"),   YoriCmd_HISTORY},
                    {_T("IF"),        YoriCmd_IF},
                    {_T("JOB"),       YoriCmd_JOB},
                    {_T("NICE"),      YoriCmd_NICE},
                    {_T("PUSHD"),     YoriCmd_PUSHD},
                    {_T("REM"),       YoriCmd_REM},
                    {_T("SET"),       YoriCmd_SET},
                    {_T("SETLOCAL"),  YoriCmd_SETLOCAL},
                    {_T("TRUE"),      YoriCmd_TRUE},
                    {_T("VER"),       YoriCmd_VER},
                    {_T("WAIT"),      YoriCmd_WAIT},
                    {_T("YS"),        YoriCmd_YS},
                    {_T("Z"),         YoriCmd_Z},
                    {NULL,            NULL}
                   };

/**
 A table of initial alias to value mappings to populate.
 */
CONST YORI_SH_DEFAULT_ALIAS_ENTRY
YoriShDefaultAliasEntries[] = {
    {_T("F7"),       _T("history -u")},
    {_T("cd"),       _T("chdir $*$")},
    {_T("cls"),      _T("ycls $*$")},
    {_T("copy"),     _T("ycopy $*$")},
    {_T("cut"),      _T("ycut $*$")},
    {_T("date"),     _T("ydate $*$")},
    {_T("del"),      _T("yerase $*$")},
    {_T("dir"),      _T("ydir $*$")},
    {_T("echo"),     _T("yecho $*$")},
    {_T("erase"),    _T("yerase $*$")},
    {_T("expr"),     _T("yexpr $*$")},
    {_T("head"),     _T("ytype -h $*$")},
    {_T("md"),       _T("ymkdir $*$")},
    {_T("mkdir"),    _T("ymkdir $*$")},
    {_T("mklink"),   _T("ymklink $*$")},
    {_T("move"),     _T("ymove $*$")},
    {_T("pause"),    _T("ypause $*$")},
    {_T("rd"),       _T("yrmdir $*$")},
    {_T("ren"),      _T("ymove $*$")},
    {_T("rename"),   _T("ymove $*$")},
    {_T("rmdir"),    _T("yrmdir $*$")},
    {_T("start"),    _T("ystart $*$")},
    {_T("time"),     _T("ydate -t $*$")},
    {_T("title"),    _T("ytitle $*$")},
    {_T("type"),     _T("ytype $*$")},
    {_T("vol"),      _T("yvol $*$")},
    {_T("?"),        _T("yexpr $*$")}
};

/**
 Register default aliases in a standard build.  This includes core aliases
 only, and is done to ensure a consistent baseline when scripts start
 executing to extend these capabilities.

 @return TRUE to indicate success.
 */
BOOL
YoriShRegisterDefaultAliases()
{
    DWORD Count;
    for (Count = 0; Count < sizeof(YoriShDefaultAliasEntries)/sizeof(YoriShDefaultAliasEntries[0]); Count++) {
        YoriShAddAliasLiteral(YoriShDefaultAliasEntries[Count].Alias, YoriShDefaultAliasEntries[Count].Value, TRUE);
    }
    return TRUE;
}

// vim:sw=4:ts=4:et:

