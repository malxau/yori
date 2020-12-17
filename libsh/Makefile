
compile: yorish.lib

!INCLUDE "..\config\common.mk"

OBJS=\
	 cmdbuf.obj   \
	 parse.obj    \

yorish.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yorish.lib

