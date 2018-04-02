These are prebuilt versions of the .ASM files in the above tree to support
building MiniCRT where MASM is not present.  Modern versions of Visual C++
include MASM and these files are not needed for those.  Accordingly, these
are created with a very old MASM (6.11d) which should be compatible with a
range of older versions of Visual C++.

Note that these are not required on 64 bit systems which can do 64 bit
math natively, so only 32 bit versions are included.
