
compile: yoridlg.lib

!INCLUDE "..\config\common.mk"

OBJS=\
	 file.obj     \
	 find.obj     \
	 input.obj    \
	 msgbox.obj   \
	 replace.obj  \

yoridlg.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yoridlg.lib

