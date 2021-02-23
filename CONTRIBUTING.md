# Contributing guidelines

## Pull Request Checklist

Before sending pull requests, attempt the following:
 - Compile with a recent compiler including static analysis (ymake ANALYZE=1)
 - Run doxygen and ensure it finds no issues.  As of this writing, use 1.8.x (1.9.x doesn't appear to expand predefined macros correctly.)
 - Compile with an older compiler to ensure no use of newer headers.  Ideally this is Visual C++ 2, but not everyone will have access to that

## Coding style

In order to ensure tools can run without installation on the largest variety of systems, this code does not use the C runtime library from the compiler.  Individual functions that are useful are implemented within the "crt" directory.

Windows API functions that are newer than NT 3.1 are dynamically loaded to ensure binaries can still launch on NT 3.1.  This logic is implemented in lib\yoricmpt.h and lib\dyld.c.

## New components

This project welcomes any new tools that seem generically useful for Windows command line users.
