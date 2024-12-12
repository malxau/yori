/**
 * @file lib/ylstrfnd.c
 *
 * Yori string find routines
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
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
 Search through a string looking to see if any substrings can be located.
 Returns the first match in offet from the beginning of the string order.
 This routine looks for matches case sensitively.

 @param String The string to search through.

 @param NumberMatches The number of substrings to look for.

 @param MatchArray An array of strings corresponding to the matches to
        look for.

 @param StringOffsetOfMatch On successful completion, returns the offset
        within the string of the match.

 @return If a match is found, returns a pointer to the entry in MatchArray
         corresponding to the substring that was matched.  If no match is
         found, returns NULL.
 */
PYORI_STRING
YoriLibFindFirstMatchSubstr(
    __in PCYORI_STRING String,
    __in YORI_ALLOC_SIZE_T NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PYORI_ALLOC_SIZE_T StringOffsetOfMatch
    )
{
    YORI_STRING RemainingString;
    YORI_ALLOC_SIZE_T CheckCount;

    YoriLibInitEmptyString(&RemainingString);
    RemainingString.StartOfString = String->StartOfString;
    RemainingString.LengthInChars = String->LengthInChars;

    while (RemainingString.LengthInChars > 0) {
        for (CheckCount = 0; CheckCount < NumberMatches; CheckCount++) {
            if (YoriLibCompareStringCnt(&RemainingString, &MatchArray[CheckCount], MatchArray[CheckCount].LengthInChars) == 0) {
                if (StringOffsetOfMatch != NULL) {
                    *StringOffsetOfMatch = String->LengthInChars - RemainingString.LengthInChars;
                }
                return &MatchArray[CheckCount];
            }
        }

        RemainingString.LengthInChars--;
        RemainingString.StartOfString++;
    }

    if (StringOffsetOfMatch != NULL) {
        *StringOffsetOfMatch = 0;
    }
    return NULL;
}

/**
 Search through a string looking to see if any substrings can be located.
 Returns the first match in offet from the beginning of the string order.
 This routine looks for matches insensitively.

 @param String The string to search through.

 @param NumberMatches The number of substrings to look for.

 @param MatchArray An array of strings corresponding to the matches to
        look for.

 @param StringOffsetOfMatch On successful completion, returns the offset
        within the string of the match.

 @return If a match is found, returns a pointer to the entry in MatchArray
         corresponding to the substring that was matched.  If no match is
         found, returns NULL.
 */
PYORI_STRING
YoriLibFindFirstMatchSubstrIns(
    __in PCYORI_STRING String,
    __in YORI_ALLOC_SIZE_T NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PYORI_ALLOC_SIZE_T StringOffsetOfMatch
    )
{
    YORI_STRING RemainingString;
    YORI_ALLOC_SIZE_T CheckCount;

    YoriLibInitEmptyString(&RemainingString);
    RemainingString.StartOfString = String->StartOfString;
    RemainingString.LengthInChars = String->LengthInChars;

    while (RemainingString.LengthInChars > 0) {
        for (CheckCount = 0; CheckCount < NumberMatches; CheckCount++) {
            if (YoriLibCompareStringInsCnt(&RemainingString, &MatchArray[CheckCount], MatchArray[CheckCount].LengthInChars) == 0) {
                if (StringOffsetOfMatch != NULL) {
                    *StringOffsetOfMatch = String->LengthInChars - RemainingString.LengthInChars;
                }
                return &MatchArray[CheckCount];
            }
        }

        RemainingString.LengthInChars--;
        RemainingString.StartOfString++;
    }

    if (StringOffsetOfMatch != NULL) {
        *StringOffsetOfMatch = 0;
    }
    return NULL;
}

/**
 Search through a string looking to see if any substrings can be located.
 Returns the last match in offet from the end of the string order.
 This routine looks for matches case sensitively.

 @param String The string to search through.

 @param NumberMatches The number of substrings to look for.

 @param MatchArray An array of strings corresponding to the matches to
        look for.

 @param StringOffsetOfMatch On successful completion, returns the offset
        within the string of the match.

 @return If a match is found, returns a pointer to the entry in MatchArray
         corresponding to the substring that was matched.  If no match is
         found, returns NULL.
 */
PYORI_STRING
YoriLibFindLastMatchSubstr(
    __in PCYORI_STRING String,
    __in YORI_ALLOC_SIZE_T NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PYORI_ALLOC_SIZE_T StringOffsetOfMatch
    )
{
    YORI_STRING RemainingString;
    YORI_ALLOC_SIZE_T CheckCount;

    YoriLibInitEmptyString(&RemainingString);

    while (TRUE) {

        RemainingString.LengthInChars++;
        if (RemainingString.LengthInChars > String->LengthInChars) {
            break;
        }
        RemainingString.StartOfString = &String->StartOfString[String->LengthInChars - RemainingString.LengthInChars];

        for (CheckCount = 0; CheckCount < NumberMatches; CheckCount++) {
            if (YoriLibCompareStringCnt(&RemainingString, &MatchArray[CheckCount], MatchArray[CheckCount].LengthInChars) == 0) {
                if (StringOffsetOfMatch != NULL) {
                    *StringOffsetOfMatch = String->LengthInChars - RemainingString.LengthInChars;
                }
                return &MatchArray[CheckCount];
            }
        }
    }

    if (StringOffsetOfMatch != NULL) {
        *StringOffsetOfMatch = 0;
    }
    return NULL;
}

/**
 Search through a string looking to see if any substrings can be located.
 Returns the last match in offet from the end of the string order.
 This routine looks for matches case insensitively.

 @param String The string to search through.

 @param NumberMatches The number of substrings to look for.

 @param MatchArray An array of strings corresponding to the matches to
        look for.

 @param StringOffsetOfMatch On successful completion, returns the offset
        within the string of the match.

 @return If a match is found, returns a pointer to the entry in MatchArray
         corresponding to the substring that was matched.  If no match is
         found, returns NULL.
 */
PYORI_STRING
YoriLibFindLastMatchSubstrIns(
    __in PCYORI_STRING String,
    __in YORI_ALLOC_SIZE_T NumberMatches,
    __in PYORI_STRING MatchArray,
    __out_opt PYORI_ALLOC_SIZE_T StringOffsetOfMatch
    )
{
    YORI_STRING RemainingString;
    YORI_ALLOC_SIZE_T CheckCount;

    YoriLibInitEmptyString(&RemainingString);

    while (TRUE) {

        RemainingString.LengthInChars++;
        if (RemainingString.LengthInChars > String->LengthInChars) {
            break;
        }
        RemainingString.StartOfString = &String->StartOfString[String->LengthInChars - RemainingString.LengthInChars];

        for (CheckCount = 0; CheckCount < NumberMatches; CheckCount++) {
            if (YoriLibCompareStringInsCnt(&RemainingString, &MatchArray[CheckCount], MatchArray[CheckCount].LengthInChars) == 0) {
                if (StringOffsetOfMatch != NULL) {
                    *StringOffsetOfMatch = String->LengthInChars - RemainingString.LengthInChars;
                }
                return &MatchArray[CheckCount];
            }
        }
    }

    if (StringOffsetOfMatch != NULL) {
        *StringOffsetOfMatch = 0;
    }
    return NULL;
}

/**
 Search through a string finding the leftmost instance of a character.  If
 no match is found, return NULL.

 @param String The string to search.

 @param CharToFind The character to find.

 @return Pointer to the leftmost matching character, or NULL if no match was
         found.
 */
LPTSTR
YoriLibFindLeftMostCharacter(
    __in PCYORI_STRING String,
    __in TCHAR CharToFind
    )
{
    YORI_ALLOC_SIZE_T Index;
    for (Index = 0; Index < String->LengthInChars; Index++) {
        if (String->StartOfString[Index] == CharToFind) {
            return &String->StartOfString[Index];
        }
    }
    return NULL;
}


/**
 Search through a string finding the rightmost instance of a character.  If
 no match is found, return NULL.

 @param String The string to search.

 @param CharToFind The character to find.

 @return Pointer to the rightmost matching character, or NULL if no match was
         found.
 */
LPTSTR
YoriLibFindRightMostCharacter(
    __in PCYORI_STRING String,
    __in TCHAR CharToFind
    )
{
    YORI_ALLOC_SIZE_T Index;
    for (Index = String->LengthInChars; Index > 0; Index--) {
        if (String->StartOfString[Index - 1] == CharToFind) {
            return &String->StartOfString[Index - 1];
        }
    }
    return NULL;
}

// vim:sw=4:ts=4:et:
