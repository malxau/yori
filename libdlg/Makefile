
compile: yoridlg.lib

!INCLUDE "..\config\common.mk"

OBJS=\
	 input.obj    \
	 msgbox.obj   \

yoridlg.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yoridlg.lib

