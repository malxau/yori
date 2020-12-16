
compile: yorish.lib

!INCLUDE "..\config\common.mk"

OBJS=\
	 parse.obj    \

yorish.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yorish.lib

