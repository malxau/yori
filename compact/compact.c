/**
 * @file compact/compact.c
 *
 * Yori shell compress or decompress files
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

#include <yoripch.h>
#include <yorilib.h>

/**
 Help text to display to the user.
 */
const
CHAR strHelpText[] =
        "\n"
        "Compress or decompress one or more files.\n"
        "\n"
        "COMPACT [-license] [-b] [-c:algorithm | -u] [-s] [<file>...]\n"
        "\n"
        "   -b             Use basic search criteria for files only\n"
        "   -c             Compress files with the specified algorithm.  Options are:\n"
        "                    lzx, ntfs, xp4k, xp8k, xp16k\n"
        "   -s             Process files from all subdirectories\n"
        "   -u             Decompress files\n"
        "   -v             Verbose output\n";

/**
 Display usage text to the user.
 */
BOOL
CompactHelp()
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Compact %i.%i\n"), COMPACT_VER_MAJOR, COMPACT_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strHelpText);
    return TRUE;
}

/**
 Context passed for each file found.
 */
typedef struct _COMPACT_CONTEXT {

    /**
     TRUE if files should be compressed.  FALSE if they should be
     decompressed.
     */
    BOOL Compress;

    /**
     TRUE if output should be generated for each file processed.  FALSE for
     silent processing.
     */
    BOOL Verbose;

    /**
     Records the total number of files processed.
     */
    LONGLONG FilesFound;

    /**
     Context for the background thread pool that performs compression tasks.
     */
    YORILIB_COMPRESS_CONTEXT CompressContext;

} COMPACT_CONTEXT, *PCOMPACT_CONTEXT;

/**
 A callback that is invoked when a file is found that matches a search criteria
 specified in the set of strings to enumerate.

 @param FilePath Pointer to the file path that was found.

 @param FileInfo Information about the file.

 @param Depth Specifies recursion depth.  Ignored in this application.

 @param CompactContext Pointer to the compact context structure indicating the
        action to perform and populated with the file found.

 @return TRUE to continute enumerating, FALSE to abort.
 */
BOOL
CompactFileFoundCallback(
    __in PYORI_STRING FilePath,
    __in PWIN32_FIND_DATA FileInfo,
    __in DWORD Depth,
    __in PCOMPACT_CONTEXT CompactContext
    )
{
    BOOL IncludeFile;

    UNREFERENCED_PARAMETER(Depth);

    ASSERT(YoriLibIsStringNullTerminated(FilePath));

    IncludeFile = TRUE;

    if ((FileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
        CompactContext->Compress &&
        CompactContext->CompressContext.CompressionAlgorithm.NtfsAlgorithm == 0) {

        IncludeFile = FALSE;
    }

    if (IncludeFile) {
        if (CompactContext->Compress) {
            if (CompactContext->Verbose) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Compressing %y...\n"), FilePath);
            }
            YoriLibCompressFileInBackground(&CompactContext->CompressContext, FilePath);
        } else {
            if (CompactContext->Verbose) {
                YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Decompressing %y...\n"), FilePath);
            }
            YoriLibDecompressFileInBackground(&CompactContext->CompressContext, FilePath);
        }
        CompactContext->FilesFound++;
    }

    return TRUE;
}


/**
 The main entrypoint for the compact cmdlet.

 @param ArgC The number of arguments.

 @param ArgV An array of arguments.

 @return Exit code of the process, zero on success, nonzero on failure.
 */
DWORD
ymain(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    DWORD MatchFlags;
    BOOL Recursive = FALSE;
    BOOL BasicEnumeration = FALSE;
    COMPACT_CONTEXT CompactContext;
    YORILIB_COMPRESS_ALGORITHM CompressionAlgorithm;
    YORI_STRING Arg;

    ZeroMemory(&CompactContext, sizeof(CompactContext));
    CompressionAlgorithm.EntireAlgorithm = 0;

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                CompactHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("b")) == 0) {
                BasicEnumeration = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:lzx")) == 0) {

                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_LZX;
                CompactContext.Compress = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:ntfs")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.NtfsAlgorithm = COMPRESSION_FORMAT_DEFAULT;
                CompactContext.Compress = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xpress")) == 0 ||
                       YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xp4k")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS4K;
                CompactContext.Compress = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xp8k")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS8K;
                CompactContext.Compress = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("c:xp16k")) == 0) {
                CompressionAlgorithm.EntireAlgorithm = 0;
                CompressionAlgorithm.WofAlgorithm = FILE_PROVIDER_COMPRESSION_XPRESS16K;
                CompactContext.Compress = TRUE;

            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("s")) == 0) {
                Recursive = TRUE;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("u")) == 0) {
                CompactContext.Compress = FALSE;
                CompressionAlgorithm.EntireAlgorithm = 0;
                ArgumentUnderstood = TRUE;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("v")) == 0) {
                CompactContext.Verbose = TRUE;
                ArgumentUnderstood = TRUE;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    //
    //  If no file name is specified, use stdin; otherwise open
    //  the file and use that
    //

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("compact: missing argument\n"));
        return EXIT_FAILURE;
    }

    if (!YoriLibInitializeCompressContext(&CompactContext.CompressContext, CompressionAlgorithm)) {
        YoriLibFreeCompressContext(&CompactContext.CompressContext);
        return EXIT_FAILURE;
    }

    MatchFlags = YORILIB_FILEENUM_RETURN_FILES | YORILIB_FILEENUM_RETURN_DIRECTORIES;
    if (Recursive) {
        MatchFlags |= YORILIB_FILEENUM_RECURSE_BEFORE_RETURN;
    }
    if (BasicEnumeration) {
        MatchFlags |= YORILIB_FILEENUM_BASIC_EXPANSION;
    }

    for (i = StartArg; i < ArgC; i++) {

        YoriLibForEachFile(&ArgV[i], MatchFlags, 0, CompactFileFoundCallback, &CompactContext);
    }

    YoriLibFreeCompressContext(&CompactContext.CompressContext);

    if (CompactContext.FilesFound == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("compact: no matching files found\n"));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
