/**
 * @file sh/yorifull.c
 *
 * Yori table of supported builtins for the monolithic build of Yori
 *
 * Copyright (c) 2017 Malcolm J. Smith
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
YORI_CMD_BUILTIN YoriCmd_CAB;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YCAL;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_CHDIR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YCLIP;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YCLS;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_COLOR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YCOMPACT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YCOPY;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_CSHOT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YCUT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_CVTVT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YDATE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YDF;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YDIR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YDU;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YECHO;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YERASE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_EXIT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YEXPR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_FALSE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_FINFO;

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
YORI_CMD_BUILTIN YoriCmd_FSCMP;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YGET;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_GRPCMP;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YHASH;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YHELP;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_HEXDUMP;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_HILITE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_HISTORY;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_ICONV;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_IF;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_INITOOL;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_INTCMP;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_JOB;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_LINES;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_LSOF;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YMEM;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YMKDIR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YMKLINK;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YMORE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YMOUNT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YMOVE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_NICE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_OSVER;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YPATH;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YPAUSE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_PUSHD;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_READLINE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_REM;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_REPL;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YRMDIR;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_SCUT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_SDIR;

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
YORI_CMD_BUILTIN YoriCmd_SETVER;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_SLEEP;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YSPLIT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YSTART;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_STRCMP;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_SYNC;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_TAIL;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_TEE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_TIMETHIS;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YTITLE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_TOUCH;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_TRUE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YTYPE;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_VER;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YVOL;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_WAIT;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_WHICH;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_WININFO;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_WINPOS;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YDBG;

/**
 Declaration for the builtin command.
 */
YORI_CMD_BUILTIN YoriCmd_YPM;

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
                    {_T("CAB"),       YoriCmd_CAB},
                    {_T("CAL"),       YoriCmd_YCAL},
                    {_T("CHDIR"),     YoriCmd_CHDIR},
                    {_T("COLOR"),     YoriCmd_COLOR},
                    {_T("CSHOT"),     YoriCmd_CSHOT},
                    {_T("CVTVT"),     YoriCmd_CVTVT},
                    {_T("EXIT"),      YoriCmd_EXIT},
                    {_T("FALSE"),     YoriCmd_FALSE},
                    {_T("FG"),        YoriCmd_FG},
                    {_T("FINFO"),     YoriCmd_FINFO},
                    {_T("FOR"),       YoriCmd_FOR},
                    {_T("FSCMP"),     YoriCmd_FSCMP},
                    {_T("GRPCMP"),    YoriCmd_GRPCMP},
                    {_T("HEXDUMP"),   YoriCmd_HEXDUMP},
                    {_T("HILITE"),    YoriCmd_HILITE},
                    {_T("HISTORY"),   YoriCmd_HISTORY},
                    {_T("ICONV"),     YoriCmd_ICONV},
                    {_T("IF"),        YoriCmd_IF},
                    {_T("INITOOL"),   YoriCmd_INITOOL},
                    {_T("INTCMP"),    YoriCmd_INTCMP},
                    {_T("JOB"),       YoriCmd_JOB},
                    {_T("LINES"),     YoriCmd_LINES},
                    {_T("LSOF"),      YoriCmd_LSOF},
                    {_T("NICE"),      YoriCmd_NICE},
                    {_T("OSVER"),     YoriCmd_OSVER},
                    {_T("PUSHD"),     YoriCmd_PUSHD},
                    {_T("REM"),       YoriCmd_REM},
                    {_T("READLINE"),  YoriCmd_READLINE},
                    {_T("REPL"),      YoriCmd_REPL},
                    {_T("SCUT"),      YoriCmd_SCUT},
                    {_T("SDIR"),      YoriCmd_SDIR},
                    {_T("SET"),       YoriCmd_SET},
                    {_T("SETLOCAL"),  YoriCmd_SETLOCAL},
                    {_T("SETVER"),    YoriCmd_SETVER},
                    {_T("SLEEP"),     YoriCmd_SLEEP},
                    {_T("STRCMP"),    YoriCmd_STRCMP},
                    {_T("SYNC"),      YoriCmd_SYNC},
                    {_T("TAIL"),      YoriCmd_TAIL},
                    {_T("TEE"),       YoriCmd_TEE},
                    {_T("TIMETHIS"),  YoriCmd_TIMETHIS},
                    {_T("TOUCH"),     YoriCmd_TOUCH},
                    {_T("TRUE"),      YoriCmd_TRUE},
                    {_T("VER"),       YoriCmd_VER},
                    {_T("WAIT"),      YoriCmd_WAIT},
                    {_T("WHICH"),     YoriCmd_WHICH},
                    {_T("WININFO"),   YoriCmd_WININFO},
                    {_T("WINPOS"),    YoriCmd_WINPOS},
                    {_T("YCAL"),      YoriCmd_YCAL},
                    {_T("YCLIP"),     YoriCmd_YCLIP},
                    {_T("YCLS"),      YoriCmd_YCLS},
                    {_T("YCOMPACT"),  YoriCmd_YCOMPACT},
                    {_T("YCOPY"),     YoriCmd_YCOPY},
                    {_T("YCUT"),      YoriCmd_YCUT},
                    {_T("YDATE"),     YoriCmd_YDATE},
                    {_T("YDBG"),      YoriCmd_YDBG},
                    {_T("YDF"),       YoriCmd_YDF},
                    {_T("YDIR"),      YoriCmd_YDIR},
                    {_T("YDU"),       YoriCmd_YDU},
                    {_T("YECHO"),     YoriCmd_YECHO},
                    {_T("YERASE"),    YoriCmd_YERASE},
                    {_T("YEXPR"),     YoriCmd_YEXPR},
                    {_T("YGET"),      YoriCmd_YGET},
                    {_T("YHASH"),     YoriCmd_YHASH},
                    {_T("YHELP"),     YoriCmd_YHELP},
                    {_T("YMEM"),      YoriCmd_YMEM},
                    {_T("YMKDIR"),    YoriCmd_YMKDIR},
                    {_T("YMKLINK"),   YoriCmd_YMKLINK},
                    {_T("YMORE"),     YoriCmd_YMORE},
                    {_T("YMOUNT"),    YoriCmd_YMOUNT},
                    {_T("YMOVE"),     YoriCmd_YMOVE},
                    {_T("YPATH"),     YoriCmd_YPATH},
                    {_T("YPAUSE"),    YoriCmd_YPAUSE},
                    {_T("YPM"),       YoriCmd_YPM},
                    {_T("YRMDIR"),    YoriCmd_YRMDIR},
                    {_T("YS"),        YoriCmd_YS},
                    {_T("YSPLIT"),    YoriCmd_YSPLIT},
                    {_T("YSTART"),    YoriCmd_YSTART},
                    {_T("YTITLE"),    YoriCmd_YTITLE},
                    {_T("YTYPE"),     YoriCmd_YTYPE},
                    {_T("YVOL"),      YoriCmd_YVOL},
                    {_T("Z"),         YoriCmd_Z},
                    {NULL,            NULL}
                   };

/**
 A table of initial alias to value mappings to populate.
 */
CONST YORI_SH_DEFAULT_ALIAS_ENTRY
YoriShDefaultAliasEntries[] = {
    {_T("cal"),      _T("ycal $*$")},
    {_T("cd"),       _T("chdir $*$")},
    {_T("clip"),     _T("yclip $*$")},
    {_T("cls"),      _T("ycls $*$")},
    {_T("compact"),  _T("ycompact $*$")},
    {_T("copy"),     _T("ycopy $*$")},
    {_T("cut"),      _T("ycut $*$")},
    {_T("date"),     _T("ydate $*$")},
    {_T("del"),      _T("yerase $*$")},
    {_T("df"),       _T("ydf $*$")},
    {_T("dir"),      _T("ydir $*$")},
    {_T("du"),       _T("ydu $*$")},
    {_T("echo"),     _T("yecho $*$")},
    {_T("erase"),    _T("yerase $*$")},
    {_T("expr"),     _T("yexpr $*$")},
    {_T("hash"),     _T("yhash $*$")},
    {_T("head"),     _T("ytype -h $*$")},
    {_T("help"),     _T("yhelp $*$")},
    {_T("htmlclip"), _T("yclip -h $*$")},
    {_T("md"),       _T("ymkdir $*$")},
    {_T("md5sum"),   _T("yhash -a md5 $*$")},
    {_T("mem"),      _T("ymem $*$")},
    {_T("mkdir"),    _T("ymkdir $*$")},
    {_T("mklink"),   _T("ymklink $*$")},
    {_T("more"),     _T("ymore $*$")},
    {_T("mount"),    _T("ymount $*$")},
    {_T("move"),     _T("ymove $*$")},
    {_T("paste"),    _T("yclip -p $*$")},
    {_T("path"),     _T("ypath $*$")},
    {_T("pause"),    _T("ypause $*$")},
    {_T("pwd"),      _T("ypath . $*$")},
    {_T("rd"),       _T("yrmdir $*$")},
    {_T("ren"),      _T("ymove $*$")},
    {_T("rename"),   _T("ymove $*$")},
    {_T("rmdir"),    _T("yrmdir $*$")},
    {_T("sha1sum"),  _T("yhash -a sha1 $*$")},
    {_T("sha256sum"),_T("yhash -a sha256 $*$")},
    {_T("sha384sum"),_T("yhash -a sha384 $*$")},
    {_T("sha512sum"),_T("yhash -a sha512 $*$")},
    {_T("start"),    _T("ystart $*$")},
    {_T("split"),    _T("ysplit $*$")},
    {_T("time"),     _T("ydate -t $*$")},
    {_T("title"),    _T("ytitle $*$")},
    {_T("type"),     _T("ytype $*$")},
    {_T("vol"),      _T("yvol $*$")},
    {_T("?"),        _T("yexpr $*$")}
};

/**
 Register default aliases in a full static build.  This is done here to ensure
 a static binary doesn't need other files to be useful.

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
