/**
 * @file crt/string.c
 *
 * Implementations of the str* library functions.
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

#ifndef WIN32_LEAN_AND_MEAN
/**
 Standard define to include somewhat less of the Windows headers.  Command
 line software doesn't need much.
 */
#define WIN32_LEAN_AND_MEAN 1
#endif

#pragma warning(disable: 4001) /* Single line comment */
#pragma warning(disable: 4127) // conditional expression constant
#pragma warning(disable: 4201) // nameless struct/union
#pragma warning(disable: 4214) // bit field type other than int
#pragma warning(disable: 4514) // unreferenced inline function

#if defined(_MSC_VER) && (_MSC_VER >= 1910)
#pragma warning(disable: 5045) // spectre mitigation 
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(push)
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#pragma warning(disable: 4668) // preprocessor conditional with nonexistent macro, SDK bug
#pragma warning(disable: 4255) // no function prototype given.  8.1 and earlier SDKs exhibit this.
#pragma warning(disable: 4820) // implicit padding added in structure
#endif

#include <windows.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma warning(pop)
#endif

/**
 Indicate to the standard CRT header that we're compiling the CRT itself.
 */
#define MINICRT_BUILD
#include "yoricrt.h"


#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of @ref mini_ttoi.
 */
#define mini_ttoi mini_wtoi
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_ttoi.
 */
#define mini_ttoi mini_atoi
#endif

/**
 Convert a NULL terminated string into a number.
 This function is compiled seperately to generate an ANSI and Unicode form,
 and is available in the header as both of these plus a conditional form.

 @param str The string to convert to a number

 @return The number form of the string, which may be zero if no numeric
         characters were in the string.
 */
int
MCRT_FN
mini_ttoi (TCHAR * str)
{
    int ret = 0;
    while (*str >= '0' && *str <= '9') {
        ret *= 10;
        ret += *str - '0';
        str++;
    }
    return ret;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcscat_s.
 */
#define mini_tcscat_s mini_wcscat_s
#else
/**
 Indicate this compilation should generate the ansi form of
 @ref mini_tcscat_s.
 */
#define mini_tcscat_s mini_strcat_s
#endif

/**
 Append one NULL terminated string to another NULL terminated string, with
 a maximum length specified for the writable string.

 @param dest The NULL terminated string that should have new characters
        written to the end.

 @param len The number of bytes available in the dest allocation.  No writes
        should occur beyond this value.

 @param src A NULL terminated string to append to dest.

 @return Pointer to dest for some unknown reason.
 */
TCHAR *
MCRT_FN
mini_tcscat_s(TCHAR * dest, unsigned int len, const TCHAR * src)
{
    unsigned int i,j;
    for (i = 0; dest[i] != '\0' && i < len; i++);
    for (j = 0; src[j] != '\0' && i < len - 1; ) {
        dest[i++] = src[j++];
    }
    dest[i++] = '\0';
    return dest;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of 
 @ref mini_tcsncat.
 */
#define mini_tcsncat mini_wcsncat
#else
/**
 Indicate this compilation should generate the ansi form of 
 @ref mini_tcsncat.
 */
#define mini_tcsncat mini_strncat
#endif

/**
 Append one NULL terminated string to another NULL terminated string, with
 a maximum length specified for the characters to read from the source
 string.

 @param dest The NULL terminated string that should have new characters
        written to the end.

 @param src A NULL terminated string to append to dest.

 @param len The maximum number of bytes to copy from the source string.

 @return Pointer to dest for some unknown reason.
 */
TCHAR *
MCRT_FN
mini_tcsncat(TCHAR * dest, const TCHAR * src, unsigned int len)
{
    unsigned int i,j;
    for (i = 0; dest[i] != '\0'; i++);
    for (j = 0; src[j] != '\0' && j < len; ) {
        dest[i++] = src[j++];
    }
    dest[i++] = '\0';
    return dest;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of @ref mini_tcschr.
 */
#define mini_tcschr mini_wcschr
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcschr.
 */
#define mini_tcschr mini_strchr
#endif

/**
 Find the leftmost occurrence of a character within a NULL terminated string.

 @param str Pointer to the string to search through.

 @param ch The character to look for.

 @return Pointer to the first occurrence of the character if found, or NULL
         if not found.
 */
TCHAR *
MCRT_FN
mini_tcschr(const TCHAR * str, TCHAR ch)
{
    const TCHAR * ptr = str;
    while (*ptr != '\0' && *ptr != ch) ptr++;
    if (*ptr == ch) return (TCHAR *)ptr;
    return NULL;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of @ref mini_tcsrchr.
 */
#define mini_tcsrchr mini_wcsrchr
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcsrchr.
 */
#define mini_tcsrchr mini_strrchr
#endif

/**
 Find the rightmost occurrence of a character within a NULL terminated string.

 @param str Pointer to the string to search through.

 @param ch The character to look for.

 @return Pointer to the last occurrence of the character if found, or NULL
         if not found.
 */
TCHAR *
MCRT_FN
mini_tcsrchr(const TCHAR * str, TCHAR ch)
{
    const TCHAR * ptr = str;
    while (*ptr != '\0') ptr++;
    while (*ptr != ch && ptr > str) ptr--;
    if (*ptr == ch) return (TCHAR *)ptr;
    return NULL;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of @ref mini_tcslen.
 */
#define mini_tcslen mini_wcslen
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcslen.
 */
#define mini_tcslen mini_strlen
#endif

/**
 Return the number of characters in the specified NULL terminated string,
 excluding the terminating NULL character.

 @param str The string to count characters in.

 @return The number of characters in the string.
 */
int
MCRT_FN
mini_tcslen(const TCHAR * str)
{
    int i = 0;
    while (str[i] != '\0') {
        i++;
    }
    return i;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of @ref mini_tcsstr.
 */
#define mini_tcsstr mini_wcsstr
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcsstr.
 */
#define mini_tcsstr mini_strstr
#endif

/**
 Find the leftmost occurrence of a string within a NULL terminated string.

 @param str Pointer to the string to search through.

 @param search The string to search for.

 @return Pointer to the first occurrence of the search string if found, or
         NULL if not found.
 */
TCHAR *
MCRT_FN
mini_tcsstr(const TCHAR * str, TCHAR * search)
{
    const TCHAR * ptr = str;
    int i;
    while (*ptr != '\0') {
        for (i=0;ptr[i]==search[i]&&search[i]!='\0'&&ptr[i]!='\0';i++);
        if (search[i]=='\0') return (TCHAR*)ptr;
        ptr++;
    }
    return NULL;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_ttoupper.
 */
#define mini_ttoupper mini_towupper
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_ttoupper.
 */
#define mini_ttoupper mini_toupper
#endif

/**
 Convert a single character to its uppercase form.  This function only handles
 the base 26 english characters.

 @param c The character to uppercase.

 @return The resulting character, which may either be converted to uppercase
         or may be the same as the input character.
 */
int
MCRT_FN
mini_ttoupper(int c)
{
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    return c;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_ttolower.
 */
#define mini_ttolower mini_towlower
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_ttolower.
 */
#define mini_ttolower mini_tolower
#endif

/**
 Convert a single character to its lowercase form.  This function only handles
 the base 26 english characters.

 @param c The character to lowercase.

 @return The resulting character, which may either be converted to lowercase
         or may be the same as the input character.
 */
int
MCRT_FN
mini_ttolower(int c)
{
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcsupr.
 */
#define mini_tcsupr mini_wcsupr
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcsupr.
 */
#define mini_tcsupr mini_strupr
#endif

/**
 Convert a string to its uppercase form.  This function only handles
 the base 26 english characters.

 @param str The string to uppercase.

 @return Pointer to the input string for some unknown reason.
 */
TCHAR *
MCRT_FN
mini_tcsupr(TCHAR * str)
{
    TCHAR * ptr = str;

    while (*ptr != '\0') {
        *ptr = (TCHAR)mini_ttoupper(*ptr);
        ptr++;
    }
    return str;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcslwr.
 */
#define mini_tcslwr mini_wcslwr
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcslwr.
 */
#define mini_tcslwr mini_strlwr
#endif

/**
 Convert a string to its lowercase form.  This function only handles
 the base 26 english characters.

 @param str The string to lowercase.

 @return Pointer to the input string for some unknown reason.
 */
TCHAR *
MCRT_FN
mini_tcslwr(TCHAR * str)
{
    TCHAR * ptr = str;

    while (*ptr != '\0') {
        *ptr = (TCHAR)mini_ttolower(*ptr);
        ptr++;
    }
    return str;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcsncmp.
 */
#define mini_tcsncmp mini_wcsncmp
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcsncmp.
 */
#define mini_tcsncmp mini_strncmp
#endif

/**
 Compare two strings up to a maximum specified number of characters,
 and indicate which string is lexicographically less than the other or
 if the two strings are equal.

 @param str1 Pointer to the first NULL terminated string to compare.

 @param str2 Pointer to the second NULL terminated string to compare.

 @param count Specifies the maximum number of characters to compare.

 @return -1 if the first string is less than the second; 1 if the first
         string is greater than the second; or 0 if the two are identical
         up to the specified number of characters.
 */
int
MCRT_FN
mini_tcsncmp(const TCHAR * str1, const TCHAR * str2, unsigned int count)
{
    const TCHAR * ptr1 = str1;
    const TCHAR * ptr2 = str2;
    unsigned int remaining = count;

    while(remaining > 0) {
        if (*ptr1 < *ptr2) {
            return -1;
        } else if (*ptr1 > *ptr2) {
            return 1;
        } else if (*ptr1 == '\0') {
            return 0;
        }

        ptr1++;
        ptr2++;
        remaining--;
    }
    return 0;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcscmp.
 */
#define mini_tcscmp mini_wcscmp
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcscmp.
 */
#define mini_tcscmp mini_strcmp
#endif

/**
 Compare two strings and indicate which string is lexicographically
 less than the other or if the two strings are equal.

 @param str1 Pointer to the first NULL terminated string to compare.

 @param str2 Pointer to the second NULL terminated string to compare.

 @return -1 if the first string is less than the second; 1 if the first
         string is greater than the second; or 0 if the two are identical.
 */
int
MCRT_FN
mini_tcscmp(const TCHAR * str1, const TCHAR * str2)
{
    return mini_tcsncmp(str1, str2, (unsigned int)-1);
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcsnicmp.
 */
#define mini_tcsnicmp mini_wcsnicmp
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcsnicmp.
 */
#define mini_tcsnicmp mini_strnicmp
#endif

/**
 Compare two strings up to a maximum specified number of characters without
 regard to case and indicate which string is lexicographically less than the
 other or if the two strings are equal.  This function only handles case
 insensitivity of the 26 base english characters.

 @param str1 Pointer to the first NULL terminated string to compare.

 @param str2 Pointer to the second NULL terminated string to compare.

 @param count Specifies the maximum number of characters to compare.

 @return -1 if the first string is less than the second; 1 if the first
         string is greater than the second; or 0 if the two are identical
         up to the specified number of characters.
 */
int
MCRT_FN
mini_tcsnicmp(const TCHAR * str1, const TCHAR * str2, unsigned int count)
{
    const TCHAR * ptr1 = str1;
    const TCHAR * ptr2 = str2;
    unsigned int remaining = count;

    while(remaining > 0) {
        if (mini_ttoupper(*ptr1) < mini_ttoupper(*ptr2)) {
            return -1;
        } else if (mini_ttoupper(*ptr1) > mini_ttoupper(*ptr2)) {
            return 1;
        } else if (*ptr1 == '\0') {
            return 0;
        }
        ptr1++;
        ptr2++;
        remaining--;
    }
    return 0;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcsicmp.
 */
#define mini_tcsicmp mini_wcsicmp
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcsicmp.
 */
#define mini_tcsicmp mini_stricmp
#endif

/**
 Compare two strings without regard to case and indicate which string is
 lexicographically less than the other or if the two strings are equal.
 This function only handles case insensitivity of the 26 base english
 characters.

 @param str1 Pointer to the first NULL terminated string to compare.

 @param str2 Pointer to the second NULL terminated string to compare.

 @return -1 if the first string is less than the second; 1 if the first
         string is greater than the second; or 0 if the two are identical.
 */
int
MCRT_FN
mini_tcsicmp(const TCHAR * str1, const TCHAR * str2)
{
    return mini_tcsnicmp(str1, str2, (unsigned int)-1);
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcstok_s.
 */
#define mini_tcstok_s mini_wcstok_s
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcstok_s.
 */
#define mini_tcstok_s mini_strtok_s
#endif

/**
 Tokenize an incoming string by searching for a token, substituting the token
 with a NULL character, and returning the resulting substring.  This can be
 called multiple times on the same base string to continue returning matches
 until the entire base string has been returned as tokens.

 @param str Pointer to the base string to tokenize.  If NULL, continue
        tokenizing the previously specified base string.

 @param match A string that points to a single character used to indicate
        a seperator value between string parts.

 @param context Pointer to a block of memory which is read and written by this
        function to track its progress through a base string.  When the caller
        no longer wishes to continue enumerating, this memory can be forgotten
        (ie., there is no explicit free action for it.)

 @return Pointer to the next substring delimited by the match seperator, or
         NULL if no more substrings are available.
 */
TCHAR *
MCRT_FN
mini_tcstok_s(TCHAR * str, TCHAR * match, TCHAR ** context)
{
    TCHAR * next;
    if (str != NULL) {
        *context = str;
    }

    next = *context;
    if (next == NULL) {
        return NULL;
    }

    while (*next != match[0] && *next != '\0') next++;

    if (*next == match[0]) {
        TCHAR * ret = *context;
        *next = '\0';
        *context = ++next;
        return ret;
    } else {
        TCHAR * ret = *context;
        *context = NULL;
        return ret;
    }
}

#ifdef UNICODE

/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcstok.
 */
#define mini_tcstok mini_wcstok

/**
 A private context for the unicode form of @ref mini_tcstok which is
 equivalent to the context explicitly specified in @ref mini_tcstok_s .
 */
#define STRTOK_CTX  wcstok_context

#else

/**
 Indicate this compilation should generate the ansi form of @ref mini_tcstok.
 */
#define mini_tcstok mini_strtok

/**
 A private context for the unicode form of @ref mini_tcstok which is
 equivalent to the context explicitly specified in @ref mini_tcstok_s .
 */
#define STRTOK_CTX  strtok_context

#endif

/**
 Storage for the global context used by @ref mini_tcstok .
 */
TCHAR * STRTOK_CTX;

/**
 Tokenize an incoming string by searching for a token, substituting the token
 with a NULL character, and returning the resulting substring.  This can be
 called multiple times on the same base string to continue returning matches
 until the entire base string has been returned as tokens.

 @param str Pointer to the base string to tokenize.  If NULL, continue
        tokenizing the previously specified base string.

 @param match A string that points to a single character used to indicate
        a seperator value between string parts.

 @return Pointer to the next substring delimited by the match seperator, or
         NULL if no more substrings are available.
 */
TCHAR *
MCRT_FN
mini_tcstok(TCHAR * str, TCHAR * match)
{
    return mini_tcstok_s(str, match, &STRTOK_CTX);
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcsspn.
 */
#define mini_tcsspn mini_wcsspn
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcsspn.
 */
#define mini_tcsspn mini_strspn
#endif

/**
 Return the number of characters in a string that consist of characters
 specified in a second string.

 @param str The base string to count matching characters in.

 @param chars A string containing a set of characters which should be
        considered matching characters.

 @return The number of consecutive characters in str that are contained within
         the chars array.
 */
int
MCRT_FN
mini_tcsspn(TCHAR * str, TCHAR * chars)
{
    DWORD len = 0;
    DWORD i;

    for (len = 0; str[len] != '\0'; len++) {
        for (i = 0; chars[i] != '\0'; i++) {
            if (str[len] == chars[i]) {
                break;
            }
        }
        if (chars[i] == '\0') {
            return len;
        }
    }

    return len;
}

#ifdef UNICODE
/**
 Indicate this compilation should generate the unicode form of
 @ref mini_tcscspn.
 */
#define mini_tcscspn mini_wcscspn
#else
/**
 Indicate this compilation should generate the ansi form of @ref mini_tcscspn.
 */
#define mini_tcscspn mini_strcspn
#endif

/**
 Return the number of characters in a string that consist of characters
 not specified in a second string.

 @param str The base string to count matching characters in.

 @param match A string containing a set of characters which should be
        considered mismatching characters.

 @return The number of consecutive characters in str that are not contained
         within the chars array.
 */
int
MCRT_FN
mini_tcscspn(TCHAR * str, TCHAR * match)
{
    DWORD len = 0;
    DWORD i;

    for (len = 0; str[len] != '\0'; len++) {
        for (i = 0; match[i] != '\0'; i++) {
            if (str[len] == match[i]) {
                return len;
            }
        }
    }

    return len;
}

// vim:sw=4:ts=4:et:
