
BINARIES=ycpuinfo.exe

!INCLUDE "..\config\common.mk"

!IF $(PDB)==1
LINKPDB=/Pdb:ycpuinfo.pdb
!ENDIF

CFLAGS=$(CFLAGS) -DCPUINFO_VER_MAJOR=$(CPUINFO_VER_MAJOR) -DCPUINFO_VER_MINOR=$(CPUINFO_VER_MINOR)

BIN_OBJS=\
	 cpuinfo.obj         \

MOD_OBJS=\
	 mod_cpu.obj     \

compile: $(BIN_OBJS) builtins.lib

ycpuinfo.exe: $(BIN_OBJS) ..\lib\yorilib.lib ..\crt\yoricrt.lib
	@echo $@
	@$(LINK) $(LDFLAGS) -entry:$(YENTRY) $(BIN_OBJS) $(LIBS) $(CRTLIB) ..\lib\yorilib.lib -version:$(CPUINFO_VER_MAJOR).$(CPUINFO_VER_MINOR) $(LINKPDB) -out:$@

mod_cpu.obj: cpuinfo.c
	@echo $@
	@$(CC) -c -DYORI_BUILTIN=1 $(CFLAGS) -Fo$@ cpuinfo.c

builtins.lib: $(MOD_OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(MOD_OBJS) -out:$@
