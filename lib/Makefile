
compile: yorilib.lib

!INCLUDE "..\config\common.mk"

OBJS=\
	 builtin.obj  \
	 cabinet.obj  \
	 call.obj     \
	 cancel.obj   \
	 clip.obj     \
	 cmdline.obj  \
	 color.obj    \
	 cshot.obj    \
	 curdir.obj   \
	 cvthtml.obj  \
	 cvtrtf.obj   \
	 debug.obj    \
	 dyld.obj     \
	 dyld_cab.obj \
	 env.obj      \
	 ep_yori.obj  \
	 filecomp.obj \
	 fileenum.obj \
	 filefilt.obj \
	 fileinfo.obj \
	 fullpath.obj \
	 group.obj    \
	 hash.obj     \
	 hexdump.obj  \
	 iconv.obj    \
	 jobobj.obj   \
	 license.obj  \
	 lineread.obj \
	 list.obj     \
	 malloc.obj   \
	 movefile.obj \
	 numkey.obj   \
	 osver.obj    \
	 path.obj     \
	 printf.obj   \
	 printfa.obj  \
	 priv.obj     \
	 process.obj  \
	 recycle.obj  \
	 scut.obj     \
	 select.obj   \
	 string.obj   \
	 strmenum.obj \
	 temp.obj     \
	 update.obj   \
	 util.obj     \
	 vt.obj       \

yorilib.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yorilib.lib

