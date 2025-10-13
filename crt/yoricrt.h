/**
 * @file crt/yoricrt.h
 *
 * Header file for minicrt based applications.  Function prototypes and
 * appropriate defines to default to their use and define Unicode-
 * conditional functions.
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

#pragma warning(disable: 4001) /* Single line comment */
#pragma warning(disable: 4127) // conditional expression constant
#pragma warning(disable: 4201) // nameless struct/union
#pragma warning(disable: 4214) // bit field type other than int
#pragma warning(disable: 4514) // unreferenced inline function

#ifdef _M_IX86

/**
 On x86 systems, use __stdcall wherever possible.
 */
#define MCRT_FN __stdcall

/**
 On x86 systems, varargs functions require __cdecl.
 */
#define MCRT_VARARGFN __cdecl

#else

/**
 On non-x86 architectures, use the default calling convention.
 */
#define MCRT_FN

/**
 On non-x86 architectures, use the default calling convention.
 */
#define MCRT_VARARGFN
#endif

#if !defined(_PREFAST_)
void *     MCRT_VARARGFN memcpy(void * dest, void const * src, size_t len);
int        MCRT_VARARGFN memcmp(void const * buf1, void const * buf2, size_t len);
void *     MCRT_VARARGFN memmove(void * dest, void const * src, size_t len);
void *     MCRT_VARARGFN memset(void * dest, int c, size_t len);
#endif

void *     MCRT_FN mini_memcpy(void * dest, const void * src, unsigned int len);
int        MCRT_FN mini_memcmp(const void * buf1, const void * buf2, unsigned int len);
void *     MCRT_FN mini_memmove(void * dest, const void * src, unsigned int len);
void *     MCRT_FN mini_memset(void * dest, char c, unsigned int len);

void       MCRT_FN mini_srand(unsigned int seed);
int        MCRT_VARARGFN mini_rand(void);

/**
 The ansi form of @ref mini_ttoi .
 */
int        MCRT_FN mini_atoi(char * str);

/**
 The unicode form of @ref mini_ttoi .
 */
int        MCRT_FN mini_wtoi(wchar_t * str);

/**
 The ansi form of @ref mini_tcsncat .
 */
char *     MCRT_FN mini_strncat(char * dest, const char * src, unsigned int len);

/**
 The unicode form of @ref mini_tcsncat .
 */
wchar_t *  MCRT_FN mini_wcsncat(wchar_t * dest, const wchar_t * src, unsigned int len);

/**
 The ansi form of @ref mini_tcscat_s .
 */
char *     MCRT_FN mini_strcat_s(char * dest, unsigned int len, const char * src);

/**
 The unicode form of @ref mini_tcscat_s .
 */
wchar_t *  MCRT_FN mini_wcscat_s(wchar_t * dest, unsigned int len, const wchar_t * src);

/**
 The ansi form of @ref mini_tcschr .
 */
char *     MCRT_FN mini_strchr(const char * str, char ch);

/**
 The unicode form of @ref mini_tcschr .
 */
wchar_t *  MCRT_FN mini_wcschr(const wchar_t * str, wchar_t ch);

/**
 The ansi form of @ref mini_tcsstr .
 */
char *     MCRT_FN mini_strstr(const char * str, char * search);

/**
 The unicode form of @ref mini_tcsstr .
 */
wchar_t *  MCRT_FN mini_wcsstr(const wchar_t * str, wchar_t * search);

/**
 The ansi form of @ref mini_tcsrchr .
 */
char *     MCRT_FN mini_strrchr(const char * str, char ch);

/**
 The unicode form of @ref mini_tcsrchr .
 */
wchar_t *  MCRT_FN mini_wcsrchr(const wchar_t * str, wchar_t ch);

/**
 The ansi form of @ref mini_tcsupr .
 */
char *     MCRT_FN mini_strupr(char * str);

/**
 The unicode form of @ref mini_tcsupr .
 */
wchar_t *  MCRT_FN mini_wcsupr(wchar_t * str);

/**
 The ansi form of @ref mini_tcslwr .
 */
char *     MCRT_FN mini_strlwr(char * str);

/**
 The unicode form of @ref mini_tcslwr .
 */
wchar_t *  MCRT_FN mini_wcslwr(wchar_t * str);

/**
 The ansi form of @ref mini_tcscmp .
 */
int        MCRT_FN mini_strcmp(const char * str1, const char * str2);

/**
 The unicode form of @ref mini_tcscmp .
 */
int        MCRT_FN mini_wcscmp(const wchar_t * str1, const wchar_t * str2);

/**
 The ansi form of @ref mini_tcsicmp .
 */
int        MCRT_FN mini_stricmp(const char * str1, const char * str2);

/**
 The unicode form of @ref mini_tcsicmp .
 */
int        MCRT_FN mini_wcsicmp(const wchar_t * str1, const wchar_t * str2);

/**
 The ansi form of @ref mini_tcsncmp .
 */
int        MCRT_FN mini_strncmp(const char * str1, const char * str2, unsigned int count);

/**
 The unicode form of @ref mini_tcsncmp .
 */
int        MCRT_FN mini_wcsncmp(const wchar_t * str1, const wchar_t * str2, unsigned int count);

/**
 The ansi form of @ref mini_tcsnicmp .
 */
int        MCRT_FN mini_strnicmp(const char * str1, const char * str2, unsigned int count);

/**
 The unicode form of @ref mini_tcsnicmp .
 */
int        MCRT_FN mini_wcsnicmp(const wchar_t * str1, const wchar_t * str2, unsigned int count);

/**
 The ansi form of @ref mini_tcsspn .
 */
int        MCRT_FN mini_strspn(char * str, char * chars);

/**
 The unicode form of @ref mini_tcsspn .
 */
int        MCRT_FN mini_wcsspn(wchar_t * str, wchar_t * chars);

/**
 The ansi form of @ref mini_tcscspn .
 */
int        MCRT_FN mini_strcspn(char * str, char * chars);

/**
 The unicode form of @ref mini_tcscspn .
 */
int        MCRT_FN mini_wcscspn(wchar_t * str, wchar_t * chars);

/**
 The ansi form of @ref mini_tcstok_s .
 */
char *     MCRT_FN mini_strtok_s(char * str, char * match, char ** context);

/**
 The unicode form of @ref mini_tcstok_s .
 */
wchar_t *  MCRT_FN mini_wcstok_s(wchar_t * str, wchar_t * match, wchar_t ** context);

/**
 The ansi form of @ref mini_tcstok .
 */
char *     MCRT_FN mini_strtok(char * str, char * match);

/**
 The unicode form of @ref mini_tcstok .
 */
wchar_t *  MCRT_FN mini_wcstok(wchar_t * str, wchar_t * match);

/**
 The ansi form of @ref mini_tcslen .
 */
int        MCRT_FN mini_strlen(const char * str);

/**
 The unicode form of @ref mini_tcslen .
 */
int        MCRT_FN mini_wcslen(const wchar_t * str);

//
//  Map tchar versions to the correct place
//

#ifdef UNICODE
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_ttoi            mini_wtoi
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcsncat         mini_wcsncat
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcscat_s        mini_wcscat_s
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcschr          mini_wcschr
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcsrchr         mini_wcsrchr
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcsstr          mini_wcsstr
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcsupr          mini_wcsupr
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcslwr          mini_wcslwr
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcscmp          mini_wcscmp
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcsncmp         mini_wcsncmp
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcsicmp         mini_wcsicmp
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcsnicmp        mini_wcsnicmp
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcsspn          mini_wcsspn
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcscspn         mini_wcscspn
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcstok_s        mini_wcstok_s
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcstok          mini_wcstok
/**
 Redirect the conditional form of this function to Unicode
 */
#define    mini_tcslen          mini_wcslen

#else

/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_ttoi            mini_atoi
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcsncat         mini_strncat
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcscat_s        mini_strcat_s
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcschr          mini_strchr
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcsrchr         mini_strrchr
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcsstr          mini_strstr
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcsupr          mini_strupr
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcslwr          mini_strlwr
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcscmp          mini_strcmp
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcsncmp         mini_strncmp
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcsicmp         mini_stricmp
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcsnicmp        mini_strnicmp
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcsspn          mini_strspn
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcscspn         mini_strcspn
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcstok_s        mini_strtok_s
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcstok          mini_strtok
/**
 Redirect the conditional form of this function to ANSI
 */
#define    mini_tcslen          mini_strlen
#endif

#ifndef MINICRT_BUILD

//
//  Have apps use them by default
//

#undef  _ttoi
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _ttoi    mini_ttoi

#undef  atoi
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define atoi     mini_atoi

#undef  wtoi
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wtoi     mini_wtoi

#undef  _tcschr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcschr  mini_tcschr

#undef  strchr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strchr   mini_strchr

#undef  wcschr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcschr   mini_wcschr

#undef  _tcsrchr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcsrchr mini_tcsrchr

#undef  strrchr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strrchr  mini_strrchr

#undef  wcsrchr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcsrchr  mini_wcsrchr

#undef  _tcsstr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcsstr  mini_tcsstr

#undef  strstr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strstr   mini_strstr

#undef  wcsstr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcsstr   mini_wcsstr

#undef  _tcsncat
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcsncat mini_tcsncat

#undef  strncat
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strncat  mini_strncat

#undef  wcsncat
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcsncat  mini_wcsncat

#undef  _tcscat_s
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcscat_s mini_tcscat_s

#undef  strcat_s
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strcat_s  mini_strcat_s

#undef  wcscat_s
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcscat_s  mini_wcscat_s

#undef  _tcscat
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcscat(a,b) mini_tcscat_s(a, (unsigned int)-1, b)

#undef  strcat
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strcat(a,b)  mini_strcat_s(a, (unsigned int)-1, b)

#undef  wcscat
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcscat(a,b)  mini_wcscat_s(a, (unsigned int)-1, b)

#undef  _tcsupr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcsupr   mini_tcsupr

#undef  _strupr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _strupr   mini_strupr

#undef  _wcsupr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _wcsupr   mini_wcsupr

#undef  _tcslwr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcslwr   mini_tcslwr

#undef  _strlwr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _strlwr   mini_strlwr

#undef  _wcslwr
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _wcslwr   mini_wcslwr

#undef  strcmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strcmp    mini_strcmp

#undef  wcscmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcscmp    mini_wcscmp

#undef  _tcscmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcscmp   mini_tcscmp

#undef  stricmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define stricmp   mini_stricmp

#undef  wcsicmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcsicmp   mini_wcsicmp

#undef  tcsicmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define tcsicmp   mini_tcsicmp

#undef  _stricmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _stricmp  mini_stricmp

#undef  _wcsicmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _wcsicmp  mini_wcsicmp

#undef  _tcsicmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcsicmp  mini_tcsicmp

#undef  strncmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strncmp   mini_strncmp

#undef  wcsncmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcsncmp   mini_wcsncmp

#undef  _tcsncmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcsncmp  mini_tcsncmp

#undef  strnicmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strnicmp  mini_strnicmp

#undef  wcsnicmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcsnicmp  mini_wcsnicmp

#undef  _tcsnicmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcsnicmp mini_tcsnicmp

#undef  _strnicmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _strnicmp mini_strnicmp

#undef  _wcsnicmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _wcsnicmp mini_wcsnicmp

#undef  strspn
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strspn    mini_strspn

#undef  wcsspn
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcsspn    mini_wcsspn

#undef  _tcsspn
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcsspn   mini_tcsspn

#undef  strcspn
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strcspn   mini_strcspn

#undef  wcscspn
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcscspn   mini_wcscspn

#undef  _tcscspn
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcscspn  mini_tcscspn

#undef  _tcstok_s
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcstok_s mini_tcstok_s

#undef  strtok_s
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strtok_s  mini_strtok_s

#undef  wcstok_s
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcstok_s  mini_wcstok_s

#undef  _tcstok
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcstok   mini_tcstok

#undef  strtok
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strtok    mini_strtok

#undef  wcstok
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcstok    mini_wcstok

#undef  _tcslen
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _tcslen   mini_tcslen

#undef  strlen
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define strlen    mini_strlen

#undef  wcslen
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define wcslen    mini_wcslen

#undef  _memcpy
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _memcpy   mini_memcpy

#undef  memcpy
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define memcpy    mini_memcpy

#undef  _memmove
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define _memmove  mini_memmove

#undef  memmove
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define memmove   mini_memmove

#undef  memset
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define memset    mini_memset

#undef  memcmp
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define memcmp    mini_memcmp

#undef  rand
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define rand      mini_rand

#undef  srand
/**
 Redirect calls to this C function to the implementation from this library.
 This ensures we can include random headers but actual linkage will be here.
 */
#define srand     mini_srand

#endif // MINICRT_BUILD

#undef RAND_MAX
/**
 The maximum value returned by @ref mini_rand .
 */
#define RAND_MAX 32768

#undef INT_MAX
/**
 The maximum value of a signed 32 bit integer.
 */
#define INT_MAX (0x7fffffff)

#undef UINT_MAX
/**
 The maximum value of an unsigned 32 bit integer.
 */
#define UINT_MAX (0xffffffff)

/**
 The exit code of a process that indicates success.
 */
#define EXIT_SUCCESS 0

/**
 The exit code of a process that indicates failure.
 */
#define EXIT_FAILURE 1


// vim:sw=4:ts=4:et:
