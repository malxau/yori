
compile: yorilib.lib yoriver.obj

!INCLUDE "..\config\common.mk"

OBJS=\
	 bargraph.obj \
	 builtin.obj  \
	 bytebuf.obj  \
	 cabinet.obj  \
	 call.obj     \
	 cancel.obj   \
	 clip.obj     \
	 cmdline.obj  \
	 color.obj    \
	 condrv.obj   \
	 cpuinfo.obj  \
	 cshot.obj    \
	 curdir.obj   \
	 cvthtml.obj  \
	 cvtrtf.obj   \
	 debug.obj    \
	 dyld.obj     \
	 dyld_adv.obj \
	 dyld_cab.obj \
	 dyld_net.obj \
	 dyld_usr.obj \
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
	 http.obj     \
	 iconv.obj    \
	 jobobj.obj   \
	 license.obj  \
	 lineread.obj \
	 list.obj     \
	 malloc.obj   \
	 movefile.obj \
	 numkey.obj   \
	 obenum.obj   \
	 osver.obj    \
	 path.obj     \
	 printf.obj   \
	 printfa.obj  \
	 priv.obj     \
	 process.obj  \
	 progman.obj  \
	 recycle.obj  \
	 scut.obj     \
	 scheme.obj   \
	 select.obj   \
	 string.obj   \
	 strmenum.obj \
	 temp.obj     \
	 update.obj   \
	 util.obj     \
	 vt.obj       \

all: yorilib.lib yoriver.obj

yorilib.lib: $(OBJS)
	@echo $@
	@$(LIB32) $(LIBFLAGS) $(OBJS) /out:yorilib.lib

yoriver.obj: yoriver.inc
	@echo $(**F)
	@if exist $@ erase $@
	@$(RC) /fo$(@B).res $(RCFLAGS) /d YORI_VER_MAJOR=$(YORI_VER_MAJOR) /d YORI_VER_MINOR=$(YORI_VER_MINOR) /d YORI_BIN_VER_MAJOR=$(YORI_VER_MAJOR) /d YORI_BIN_VER_MINOR=$(YORI_VER_MINOR) $** >NUL
	@if not exist $@ ren $(@B).res $@

