


!IFDEF _YMAKE_VER

DAILYPATH=http://www.malsmith.net/download/?obj=yori/latest-daily
STABLEPATH=http://www.malsmith.net/download/?obj=yori/latest-stable

!IF $(YORI_BUILD_ID)==0
BUILDSTRING=$(YORI_VER_MAJOR).$(YORI_VER_MINOR)
UPGRADEPATH=$(STABLEPATH)
UPGRADEALTARG=-upgradedaily $(DAILYPATH)
!ELSE
BUILDSTRING=$(YORI_VER_MAJOR).$(YORI_VER_MINOR).$(YORI_BUILD_ID)
UPGRADEPATH=$(DAILYPATH)
UPGRADEALTARG=-upgradestable $(STABLEPATH)
!ENDIF
SOURCEPATH=http://www.malsmith.net/download/?obj=yori/$(BUILDSTRING)

#
# Determine the debug suffix based on the build type.  Also record the one
# we're not building, which is the one that any packages from this build
# would replace
#

DBGSUFFIX=
INVDBGSUFFIX=-dbg
!IF $(DEBUG)==1
DBGSUFFIX=-dbg
INVDBGSUFFIX=
!ENDIF

ARCHPKGS=out\yori-core$(DBGSUFFIX)-$(ARCH).cab    \
         out\yori-extra$(DBGSUFFIX)-$(ARCH).cab   \
         out\yori-modular$(DBGSUFFIX)-$(ARCH).cab \
         out\yori-one$(DBGSUFFIX)-$(ARCH).cab     \
         out\yori-typical$(DBGSUFFIX)-$(ARCH).cab \
         out\yori-ypm$(DBGSUFFIX)-$(ARCH).cab 

NOARCHPKGS=out\yori-source-noarch.cab             \
           out\yori-completion-noarch.cab         \

YSETUP=
!IF "$(DBGSUFFIX)"==""
!IF "$(ARCH)"=="win32"
YSETUP=out\$(ARCH)\ysetup.exe
!ENDIF
!IF "$(ARCH)"=="mips"
YSETUP=out\$(ARCH)\ysetup.exe
!ENDIF
!IF "$(ARCH)"=="amd64"
YSETUP=out\$(ARCH)\ysetup.exe
!ENDIF
!ENDIF

all: $(ARCHPKGS) $(NOARCHPKGS) $(YSETUP)

arch: $(ARCHPKGS)

noarch: $(NOARCHPKGS)

out:
	mkdir out

COREPDBCAB=yori-core$(DBGSUFFIX)-pdb-$(ARCH).cab
EXTRAPDBCAB=yori-extra$(DBGSUFFIX)-pdb-$(ARCH).cab
MODULARPDBCAB=yori-modular$(DBGSUFFIX)-pdb-$(ARCH).cab
TYPICALPDBCAB=yori-typical$(DBGSUFFIX)-pdb-$(ARCH).cab
ONEPDBCAB=yori-one$(DBGSUFFIX)-pdb-$(ARCH).cab
YPMPDBCAB=yori-ypm$(DBGSUFFIX)-pdb-$(ARCH).cab

out\yori-core$(DBGSUFFIX)-$(ARCH).cab: out yori-core.lst @yori-core.lst
	@echo $(@F)
	@ypm -c out\$(@F) yori-core$(DBGSUFFIX) $(BUILDSTRING) $(ARCH) -filelist yori-core.lst -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-core$(DBGSUFFIX)-pdb-$(ARCH).cab -replaces yori-core$(INVDBGSUFFIX)
	@ypm -c out\$(COREPDBCAB) yori-core$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH) -filelist yori-core-pdb.lst -upgradepath $(UPGRADEPATH)/$(COREPDBCAB) $(UPGRADEALTARG)/$(COREPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -replaces yori-core$(INVDBGSUFFIX)-pdb

out\yori-extra$(DBGSUFFIX)-$(ARCH).cab: out yori-extra.lst @yori-extra.lst
	@echo $(@F)
	@ypm -c out\$(@F) yori-extra$(DBGSUFFIX) $(BUILDSTRING) $(ARCH) -filelist yori-extra.lst -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-extra$(DBGSUFFIX)-pdb-$(ARCH).cab -replaces yori-extra$(INVDBGSUFFIX)
	@ypm -c out\$(EXTRAPDBCAB) yori-extra$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH) -filelist yori-extra-pdb.lst -upgradepath $(UPGRADEPATH)/$(EXTRAPDBCAB) $(UPGRADEALTARG)/$(EXTRAPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -replaces yori-extra$(INVDBGSUFFIX)-pdb

out\yori-modular$(DBGSUFFIX)-$(ARCH).cab: out yori-modular.lst @yori-modular.lst
	@echo $(@F)
	@ypm -c out\$(@F) yori-modular$(DBGSUFFIX) $(BUILDSTRING) $(ARCH) -filelist yori-modular.lst -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-modular$(DBGSUFFIX)-pdb-$(ARCH).cab -replaces yori-modular$(INVDBGSUFFIX)
	@ypm -c out\$(MODULARPDBCAB) yori-modular$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH) -filelist yori-modular-pdb.lst -upgradepath $(UPGRADEPATH)/$(MODULARPDBCAB) $(UPGRADEALTARG)/$(MODULARPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -replaces yori-modular$(INVDBGSUFFIX)-pdb

out\yori-typical$(DBGSUFFIX)-$(ARCH).cab: out yori-typical.lst @yori-typical.lst
	@echo $(@F)
	@ypm -c out\$(@F) yori-typical$(DBGSUFFIX) $(BUILDSTRING) $(ARCH) -filelist yori-typical.lst -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-typical$(DBGSUFFIX)-pdb-$(ARCH).cab -replaces yori-typical$(INVDBGSUFFIX)
	@ypm -c out\$(TYPICALPDBCAB) yori-typical$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH) -filelist yori-typical-pdb.lst -upgradepath $(UPGRADEPATH)/$(TYPICALPDBCAB) $(UPGRADEALTARG)/$(TYPICALPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -replaces yori-typical$(INVDBGSUFFIX)-pdb

out\yori-one$(DBGSUFFIX)-$(ARCH).cab: out yori-one.lst @yori-one.lst
	@echo $(@F)
	@ypm -c out\$(@F) yori-one$(DBGSUFFIX) $(BUILDSTRING) $(ARCH) -filelist yori-one.lst -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-one$(DBGSUFFIX)-pdb-$(ARCH).cab -replaces yori-one$(INVDBGSUFFIX)
	@ypm -c out\$(ONEPDBCAB) yori-one$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH) -filelist yori-one-pdb.lst -upgradepath $(UPGRADEPATH)/$(ONEPDBCAB) $(UPGRADEALTARG)/$(ONEPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -replaces yori-one$(INVDBGSUFFIX)-pdb

out\yori-ypm$(DBGSUFFIX)-$(ARCH).cab: out yori-ypm.lst @yori-ypm.lst
	@echo $(@F)
	@ypm -c out\$(@F) yori-ypm$(DBGSUFFIX) $(BUILDSTRING) $(ARCH) -filelist yori-ypm.lst -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-ypm$(DBGSUFFIX)-pdb-$(ARCH).cab -replaces yori-ypm$(INVDBGSUFFIX)
	@ypm -c out\$(YPMPDBCAB) yori-ypm$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH) -filelist yori-ypm-pdb.lst -upgradepath $(UPGRADEPATH)/$(YPMPDBCAB) $(UPGRADEALTARG)/$(YPMPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -replaces yori-ypm$(INVDBGSUFFIX)-pdb

out\yori-source-noarch.cab: out
	@echo $(@F)
	@ypm -cs out\$(@F) yori-source $(BUILDSTRING) -filepath ..

out\yori-completion-noarch.cab: out yori-completion.lst @yori-completion.lst
	@echo $(@F)
	@ypm -c out\$(@F) yori-completion $(BUILDSTRING) noarch -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -filelist yori-completion.lst

out\$(ARCH)\ysetup.exe: out ..\ysetup\ysetup.exe
	@echo $(@F)
	@mkdir out\$(ARCH)
	@copy ..\ysetup\ysetup.exe out\$(ARCH)\ysetup.exe

!ENDIF

#
# Don't do anything for regular compile and link.  Note that regular clean
# doesn't do anything here either, because the cabs are retained across
# builds for different architectures or debug configurations.
#

compile:

link:

install:

clean:
