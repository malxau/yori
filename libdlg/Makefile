
compile: yoridlg.lib

!INCLUDE "..\config\common.mk"

OBJS=\
	 about.obj    \
	 device.obj   \
	 dir.obj      \
	 file.obj     \
	 find.obj     \
	 findhex.obj  \
	 input.obj    \
	 msgbox.obj   \
	 replace.obj  \
	 replhex.obj  \

yoridlg.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yoridlg.lib

