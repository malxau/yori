


!IFDEF _YMAKE_VER

DAILYPATH=http://www.malsmith.net/download/?obj=yori/latest-daily
STABLEPATH=http://www.malsmith.net/download/?obj=yori/latest-stable

#
# PPC, Alpha and AXP64 are "special" in that I can't personally build them so
# these builds are not on the same cadence as the others.
#
!IF "$(ARCH)"=="ppc"
DAILYPATH=http://www.malsmith.net/download/?obj=yori/latest-ppc-daily
STABLEPATH=http://www.malsmith.net/download/?obj=yori/latest-ppc-stable
!ELSE
!IF "$(ARCH)"=="axp"
DAILYPATH=http://www.malsmith.net/download/?obj=yori/latest-axp-daily
STABLEPATH=http://www.malsmith.net/download/?obj=yori/latest-axp-stable
!ELSE
!IF "$(ARCH)"=="axp64"
DAILYPATH=http://www.malsmith.net/download/?obj=yori/latest-axp64-daily
STABLEPATH=http://www.malsmith.net/download/?obj=yori/latest-axp64-stable
!ENDIF # AXP64
!ENDIF # AXP
!ENDIF # PPC

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
YSETUP=out\$(ARCH)\ysetup.exe
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
	@ypm -c -filelist yori-core.lst -replaces yori-core$(INVDBGSUFFIX) -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-core$(DBGSUFFIX)-pdb-$(ARCH).cab out\$(@F) yori-core$(DBGSUFFIX) $(BUILDSTRING) $(ARCH)
	@ypm -c -filelist yori-core-pdb.lst -replaces yori-core$(INVDBGSUFFIX)-pdb -upgradepath $(UPGRADEPATH)/$(COREPDBCAB) $(UPGRADEALTARG)/$(COREPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab out\$(COREPDBCAB) yori-core$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH)

out\yori-extra$(DBGSUFFIX)-$(ARCH).cab: out yori-extra.lst @yori-extra.lst
	@echo $(@F)
	@ypm -c -filelist yori-extra.lst -replaces yori-extra$(INVDBGSUFFIX) -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-extra$(DBGSUFFIX)-pdb-$(ARCH).cab out\$(@F) yori-extra$(DBGSUFFIX) $(BUILDSTRING) $(ARCH)
	@ypm -c -filelist yori-extra-pdb.lst -replaces yori-extra$(INVDBGSUFFIX)-pdb -upgradepath $(UPGRADEPATH)/$(EXTRAPDBCAB) $(UPGRADEALTARG)/$(EXTRAPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab out\$(EXTRAPDBCAB) yori-extra$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH)

out\yori-modular$(DBGSUFFIX)-$(ARCH).cab: out yori-modular.lst @yori-modular.lst
	@echo $(@F)
	@ypm -c -filelist yori-modular.lst -replaces yori-modular$(INVDBGSUFFIX) -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-modular$(DBGSUFFIX)-pdb-$(ARCH).cab out\$(@F) yori-modular$(DBGSUFFIX) $(BUILDSTRING) $(ARCH)
	@ypm -c -filelist yori-modular-pdb.lst -replaces yori-modular$(INVDBGSUFFIX)-pdb -upgradepath $(UPGRADEPATH)/$(MODULARPDBCAB) $(UPGRADEALTARG)/$(MODULARPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab out\$(MODULARPDBCAB) yori-modular$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH)

out\yori-typical$(DBGSUFFIX)-$(ARCH).cab: out yori-typical.lst @yori-typical.lst
	@echo $(@F)
	@ypm -c -filelist yori-typical.lst -replaces yori-typical$(INVDBGSUFFIX) -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-typical$(DBGSUFFIX)-pdb-$(ARCH).cab out\$(@F) yori-typical$(DBGSUFFIX) $(BUILDSTRING) $(ARCH)
	@ypm -c -filelist yori-typical-pdb.lst -replaces yori-typical$(INVDBGSUFFIX)-pdb -upgradepath $(UPGRADEPATH)/$(TYPICALPDBCAB) $(UPGRADEALTARG)/$(TYPICALPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab out\$(TYPICALPDBCAB) yori-typical$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH)

out\yori-one$(DBGSUFFIX)-$(ARCH).cab: out yori-one.lst @yori-one.lst
	@echo $(@F)
	@ypm -c -filelist yori-one.lst -replaces yori-one$(INVDBGSUFFIX) -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-one$(DBGSUFFIX)-pdb-$(ARCH).cab out\$(@F) yori-one$(DBGSUFFIX) $(BUILDSTRING) $(ARCH)
	@ypm -c -filelist yori-one-pdb.lst -replaces yori-one$(INVDBGSUFFIX)-pdb -upgradepath $(UPGRADEPATH)/$(ONEPDBCAB) $(UPGRADEALTARG)/$(ONEPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab out\$(ONEPDBCAB) yori-one$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH)

out\yori-ypm$(DBGSUFFIX)-$(ARCH).cab: out yori-ypm.lst @yori-ypm.lst
	@echo $(@F)
	@ypm -c -filelist yori-ypm.lst -replaces yori-ypm$(INVDBGSUFFIX) -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -symbolpath $(SOURCEPATH)/yori-ypm$(DBGSUFFIX)-pdb-$(ARCH).cab out\$(@F) yori-ypm$(DBGSUFFIX) $(BUILDSTRING) $(ARCH)
	@ypm -c -filelist yori-ypm-pdb.lst -replaces yori-ypm$(INVDBGSUFFIX)-pdb -upgradepath $(UPGRADEPATH)/$(YPMPDBCAB) $(UPGRADEALTARG)/$(YPMPDBCAB) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab out\$(YPMPDBCAB) yori-ypm$(DBGSUFFIX)-pdb $(BUILDSTRING) $(ARCH)

out\yori-source-noarch.cab: out
	@echo $(@F)
	@ypm -cs -filepath .. out\$(@F) yori-source $(BUILDSTRING)

out\yori-completion-noarch.cab: out yori-completion.lst @yori-completion.lst
	@echo $(@F)
	@ypm -c -upgradepath $(UPGRADEPATH)/$(@F) $(UPGRADEALTARG)/$(@F) -sourcepath $(SOURCEPATH)/yori-source-noarch.cab -filelist yori-completion.lst out\$(@F) yori-completion $(BUILDSTRING) noarch

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
