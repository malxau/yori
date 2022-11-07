
compile: yoriwin.lib

!INCLUDE "..\config\common.mk"

OBJS=\
	 border.obj   \
	 button.obj   \
	 checkbox.obj \
	 color.obj    \
	 combo.obj    \
	 ctrl.obj     \
	 edit.obj     \
	 hexedit.obj  \
	 itemaray.obj \
	 label.obj    \
	 list.obj     \
	 menubar.obj  \
	 mledit.obj   \
	 radio.obj    \
	 scrolbar.obj \
	 window.obj   \
	 winmgr.obj   \

yoriwin.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yoriwin.lib

