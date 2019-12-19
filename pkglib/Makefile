
compile: yoripkg.lib 

!INCLUDE "..\config\common.mk"

CFLAGS=$(CFLAGS) -DYPM_VER_MAJOR=$(YPM_VER_MAJOR) -DYPM_VER_MINOR=$(YPM_VER_MINOR)

OBJS=\
	 api.obj         \
	 backup.obj      \
	 create.obj      \
	 install.obj     \
	 reg.obj         \
	 remote.obj      \
	 util.obj        \

yoripkg.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yoripkg.lib

