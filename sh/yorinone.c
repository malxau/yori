/**
 * @file sh/yorinone.c
 *
 * Yori table of supported builtins for the modular build of Yori (ie., none.)
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
 The list of builtin commands supported by this build of Yori.
 */
CONST YORI_SH_BUILTIN_NAME_MAPPING
YoriShBuiltins[] = {
                    {NULL,            NULL}
                   };

/**
 A table of initial alias to value mappings to populate.
 */
YORI_SH_DEFAULT_ALIAS_ENTRY
YoriShDefaultAliasEntries[] = {
    {_T("cd"),       _T("chdir $*$")},
    {_T("clip"),     _T("yclip $*$")},
    {_T("cls"),      _T("ycls $*$")},
    {_T("compact"),  _T("ycompact $*$")},
    {_T("copy"),     _T("ycopy $*$")},
    {_T("cut"),      _T("ycut $*$")},
    {_T("date"),     _T("ydate $*$")},
    {_T("del"),      _T("yerase $*$")},
    {_T("dir"),      _T("ydir $*$")},
    {_T("echo"),     _T("yecho $*$")},
    {_T("erase"),    _T("yerase $*$")},
    {_T("expr"),     _T("yexpr $*$")},
    {_T("head"),     _T("ytype -h $*$")},
    {_T("help"),     _T("yhelp $*$")},
    {_T("htmlclip"), _T("yclip -h $*$")},
    {_T("md"),       _T("ymkdir $*$")},
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
    {_T("start"),    _T("ystart $*$")},
    {_T("split"),    _T("ysplit $*$")},
    {_T("time"),     _T("ydate -t $*$")},
    {_T("title"),    _T("ytitle $*$")},
    {_T("type"),     _T("ytype $*$")},
    {_T("vol"),      _T("yvol $*$")},
    {_T("?"),        _T("yexpr $*$")}
};

/**
 Return the number of elements in the YoriShDefaultAliasEntries array.
 */
DWORD
YoriShDefaultAliasEntriesCount()
{
    return sizeof(YoriShDefaultAliasEntries)/sizeof(YoriShDefaultAliasEntries[0]);
}

// vim:sw=4:ts=4:et:
