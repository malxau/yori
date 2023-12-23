
!IF $(KERNELBASE)==1
!IF "$(ARCH)"=="amd64" || "$(ARCH)"=="arm64"
compile: kernelbase.lib
!ELSE
# x86 uses stdcall and depends upon having an import library that contains
# symbols specifying the amount of stack to be popped from a function.
# This currently doesn't support this, and that's partly because there's
# currently no system that would benefit from a kernelbase-only x86 build.
!ERROR Architecture not supported for kernelbase builds
!ENDIF
!ELSE
compile:
!ENDIF

!INCLUDE "..\config\common.mk"

kernelbase.lib: $(OBJS) kernelbase.def
	@echo $@
	@$(LIB32) $(LIBFLAGS) -MACHINE:$(MACHINE) -DEF:kernelbase.def -out:$@
