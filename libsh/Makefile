
compile: yorish.lib

!INCLUDE "..\config\common.mk"

OBJS=\
	 builtin.obj  \
	 cmdbuf.obj   \
	 exec.obj     \
	 parse.obj    \

yorish.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yorish.lib

