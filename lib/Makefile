
compile: yorilib.lib

!INCLUDE "..\config\common.mk"

DEBUG=0

OBJS=\
	 cabinet.obj  \
	 call.obj     \
	 cancel.obj   \
	 clip.obj     \
	 cmdline.obj  \
	 color.obj    \
	 cshot.obj    \
	 cvthtml.obj  \
	 debug.obj    \
	 dyld.obj     \
	 env.obj      \
	 ep_yori.obj  \
	 filecomp.obj \
	 fileenum.obj \
	 fileinfo.obj \
	 fullpath.obj \
	 hash.obj     \
	 hexdump.obj  \
	 iconv.obj    \
	 jobobj.obj   \
	 license.obj  \
	 lineread.obj \
	 list.obj     \
	 malloc.obj   \
	 osver.obj    \
	 recycle.obj  \
	 path.obj     \
	 printf.obj   \
	 printfa.obj  \
	 scut.obj     \
	 select.obj   \
	 string.obj   \
	 update.obj   \
	 util.obj     \
	 vt.obj       \

yorilib.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yorilib.lib

