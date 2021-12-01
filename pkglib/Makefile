
compile: yoripkg.lib 

!INCLUDE "..\config\common.mk"

OBJS=\
	 api.obj         \
	 backup.obj      \
	 config.obj      \
	 create.obj      \
	 install.obj     \
	 reg.obj         \
	 remote.obj      \
	 util.obj        \

yoripkg.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yoripkg.lib

