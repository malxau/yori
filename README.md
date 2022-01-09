# Yori: CMD reimagined

Yori is a CMD replacement shell that supports backquotes, job control, and improves tab completion, file matching, aliases, command history, and more.  It includes a handful of native Win32 tools that implement commonly needed tasks which can be used with any shell.

## Build

Compiling currently requires Visual C++, version 2 or newer.  To compile, run NMAKE.  Once compiled, YMAKE.EXE allows for more efficient subsequent compilation, using all cores in the machine.

## License

Yori is available under the MIT license.

## System requirements

For the core shell and components, NT 3.1 or newer for 32 bit; XP 64 or newer for 64 bit.  Individual features may require newer versions.  Note the ysetup.exe installer works best with NT 4 and IE 4 or newer.

To install on older versions:

| Release | Considerations |
|---------|----------------|
| Nano Server | Use the native [AMD64 installer](<http://www.malsmith.net/download/?obj=yori/latest-stable/amd64/ysetup.exe>). |
| 95, 98 or Me | These releases are not supported and extensive changes would be required to execute on them. |
| NT 4 for MIPS | Ensure IE 3 is installed, and use the [MIPS installer](<http://www.malsmith.net/download/?obj=yori/latest-stable/mips/ysetup.exe>) . |
| NT 4 with IE 2 | Install the [wintdist.exe](<http://www.malsmith.net/download/?obj=yori/latest-redist/wintdist.exe>) redistributable which contains the IE 3 wininet.dll before running the installer. |
| NT 3.5x | Install the [wint351.exe](<http://www.malsmith.net/download/?obj=yori/latest-redist/wint351.exe>) redistributable which contains the IE 3 wininet.dll before running the installer. |
| NT 3.1 | No installer exists.  Files must be copies to the machine manually. |
