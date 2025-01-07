
SolutionDir := d:/Source/P5
SolutionName := P5_PS3
SCRIPT_PATH := ../Misc/Scripts

EMPTY=
SPACE=$(EMPTY) $(EMPTY)

LEFTPAR=$(EMPTY)($(EMPTY)
RIGHTPAR=$(EMPTY))$(EMPTY)
FIXED_SCE_PS3_ROOT := $(subst $(LEFTPAR),\$(LEFTPAR),$(subst $(RIGHTPAR),\$(RIGHTPAR),$(subst $(SPACE),\$(SPACE),$(subst \,/,$(SCE_PS3_ROOT)))))
FIXED_SN_PS3_PATH := $(subst $(LEFTPAR),\$(LEFTPAR),$(subst $(RIGHTPAR),\$(RIGHTPAR),$(subst $(SPACE),\$(SPACE),$(subst \,/,$(SN_PS3_PATH)))))

BIN_LIB := $(FIXED_SCE_PS3_ROOT)/host-win32/bin
PPU_BIN_LIB := $(FIXED_SCE_PS3_ROOT)/host-win32/ppu/bin
SPU_BIN_LIB := $(FIXED_SCE_PS3_ROOT)/host-win32/spu/bin
SN_BIN := $(FIXED_SN_PS3_PATH)/bin

CCACHE :=
DISTCC := distcc
PPU_CC := $(PPU_BIN_LIB)/ppu-lv2-gcc
PPU_AR := $(PPU_BIN_LIB)/ppu-lv2-ar
PPU_RANLIB := $(PPU_BIN_LIB)/ppu-lv2-ranlib
PPU_LD := $(PPU_BIN_LIB)/ppu-lv2-ld
PPU_OBJCOPY := $(PPU_BIN_LIB)/ppu-lv2-objcopy

MAKE_SELF := $(BIN_LIB)/make_fself
COPY := cp
MAdditionalPS3Compile := -Wno-non-virtual-dtor -Wno-sign-compare -Wno-conversion -Wno-multichar -Wno-unused -Werror

SPU_CC := $(SPU_BIN_LIB)/spu-lv2-gcc
SPU_MODGEN := $(SN_BIN)/spumodgen.exe


SPU_INCLUDES := -I $(FIXED_SN_PS3_PATH)/spu/include/sn -I $(SolutionDir)/Shared/MCC
SPU_DEFS := -DSN_TARGET_PS3_SPU -D__GCC__ -DSPU
SPU_CPP_OPTIONS := -Wall -Wno-multichar -fno-exceptions -fcheck-new -fno-strict-aliasing -finline-functions -ffunction-sections -fdata-sections
SPU_OBJCOPY_OPTIONS := -I binary -O elf64-powerpc-celloslv2 -B powerpc --set-section-align .data=7 --set-section-pad .data=128
SPU_LIBS := -ldma -latomic -lsputhread

SPURS_CPP_OPTIONS := -fpic -fstack-check -Wall -Wno-multichar -fno-exceptions -fcheck-new -fno-strict-aliasing -finline-functions -ffunction-sections -fdata-sections
SPURS_LD_OPTIONS := -fpic -nostartfiles -Ttext=0 -Wl,-q -e cellSpursJobMain
SPURS_LIBS := -ldma -latomic -lsputhread -lspurs
SPURS_DEFS := -DSN_TARGET_PS3_SPU -D__GCC__ -DSPU


PPU_LD_DEPS :=

COMPILE := -c
DEPEND := -MP -MM -MQ

ERRORCONV := 2>&1| xargs -0 perl $(SCRIPT_PATH)/gcc2vc.pl $(SolutionDir)
CC_ERRORCONV := $(ERRORCONV)
LINK_ERRORCONV :=

PPU_LN_VERBOSE := @
PPU_CC_VERBOSE := @
SPU_VERBOSE := @
SPURS_VERBOSE := @




LinkDir := $(SolutionDir)/Output/Ps3/$(Config)

BuildPrefix :=  export DISTCC_HOSTS="node5 node6 node7 node8 build02 localhost " &&  $(DISTCC)

LocalLibs := -lLib_RenderPS3GCM
LocalLibs += -lLib_MSystem
LocalLibs += -lLib_GameClassesObjMisc
LocalLibs += -lLib_XR
LocalLibs += -lLib_GameClassesMain
LocalLibs += -lLib_GameClassesModelsAI
LocalLibs += -lLib_GameWorld
LocalLibs += -lLib_GameClassesOther
LocalLibs += -lLib_XRClasses
LocalLibs += -lLib_MCC
LocalLibs += -lLib_GameClassesObjChar



LocalLibDeps := $(LinkDir)/libLib_RenderPS3GCM.a
LocalLibDeps += $(LinkDir)/libLib_MSystem.a
LocalLibDeps += $(LinkDir)/libLib_GameClassesObjMisc.a
LocalLibDeps += $(LinkDir)/libLib_XR.a
LocalLibDeps += $(LinkDir)/libLib_GameClassesMain.a
LocalLibDeps += $(LinkDir)/libLib_GameClassesModelsAI.a
LocalLibDeps += $(LinkDir)/libLib_GameWorld.a
LocalLibDeps += $(LinkDir)/libLib_GameClassesOther.a
LocalLibDeps += $(LinkDir)/libLib_XRClasses.a
LocalLibDeps += $(LinkDir)/libLib_MCC.a
LocalLibDeps += $(LinkDir)/libLib_GameClassesObjChar.a


PlatformName := Ps3
ifeq ($(Config),Debug_RTM)
	ConfDef := 1
endif
ifeq ($(Config),Debug)
	ConfDef := 1
endif
ifeq ($(Config),Release)
	ConfDef := 1
endif
ifeq ($(Config),RTM)
	ConfDef := 1
endif
ifeq ($(Config),RTMD_Profile)
	ConfDef := 1
endif
ifeq ($(Config),RTMD)
	ConfDef := 1
endif

Default: testconfig CreateOutputDirs Lib_GameClassesObjMiscExtraTargets Exe_Main_PS3BuildExtraTargets Lib_MSystemExtraTargets Exe_Main_PS3 Exe_Main_PS3ExtraTargets Lib_GameWorldExtraTargets Lib_XRExtraTargets Lib_GameClassesObjCharExtraTargets Lib_GameClassesMainExtraTargets Lib_GameClassesOtherExtraTargets Lib_RenderPS3GCMExtraTargets Lib_MCCExtraTargets Lib_GameClassesModelsAIExtraTargets Lib_XRClassesExtraTargets 
build: Default

rebuild: | clean Default

run: Default
	ps3run -r1000 $(LinkDir)/Exe_Main_PS3_$(Config).self

debug: Default
	ps3debugger -e -r -x $(LinkDir)/Exe_Main_PS3_$(Config).self

CreateLinkDir:
	@mkdir -p $(LinkDir)

contentcompile:
	s:/Projects/P5/Build/System/Win32_x86_Release/XWC_Static.exe "s:/Projects/P5/Tools/ContentCompile/ContentCompile_PS3_Full.xrg"


CCacheStats:
	@ && ccache -s


ClearCCache:
	@ && ccache -C -z

testconfig:
ifneq ($(ConfDef),1)
	@echo Please set the command line parameter Config to a valid configuration.
	@echo Example: $(MAKE) Config=RTM
	@echo Valid configs are: Debug_RTM, Debug, Release, RTM, RTMD_Profile, RTMD
	@exit 1
endif


include makefile.Lib_GameClassesObjMisc
include makefile.Exe_Main_PS3Build
include makefile.Lib_MSystem
include makefile.Exe_Main_PS3
include makefile.Lib_GameWorld
include makefile.Lib_XR
include makefile.Lib_GameClassesObjChar
include makefile.Lib_GameClassesMain
include makefile.Lib_GameClassesOther
include makefile.Lib_RenderPS3GCM
include makefile.Lib_MCC
include makefile.Lib_GameClassesModelsAI
include makefile.Lib_XRClasses

makefile.Lib_GameClassesObjMisc: Projects/Main/GameClasses/Lib_GameClassesObjMisc.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_GameClassesObjMisc
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Projects/Main/GameClasses/Lib_GameClassesObjMisc.vcproj makefile.Lib_GameClassesObjMisc ../Misc/Scripts/ConvSettings.txt> log
Lib_GameClassesObjMisc: makefile.Lib_GameClassesObjMisc testconfig $(CreateOutputDirs) $(Lib_GameClassesObjMiscTargetPath) Lib_GameClassesObjMiscExtraTargets 

makefile.Exe_Main_PS3Build: Projects/Main/Exe_PS3/Exe_Main_PS3Build.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Exe_Main_PS3Build
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Projects/Main/Exe_PS3/Exe_Main_PS3Build.vcproj makefile.Exe_Main_PS3Build ../Misc/Scripts/ConvSettings.txt> log
Exe_Main_PS3Build: makefile.Exe_Main_PS3Build testconfig $(CreateOutputDirs) $(Exe_Main_PS3BuildTargetPath) Exe_Main_PS3BuildExtraTargets 

makefile.Lib_MSystem: Shared/MOS/MSystem/Lib_MSystem.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_MSystem
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Shared/MOS/MSystem/Lib_MSystem.vcproj makefile.Lib_MSystem ../Misc/Scripts/ConvSettings.txt> log
Lib_MSystem: makefile.Lib_MSystem testconfig $(CreateOutputDirs) $(Lib_MSystemTargetPath) Lib_MSystemExtraTargets 

makefile.Exe_Main_PS3: Projects/Main/Exe_PS3/Exe_Main_PS3.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Exe_Main_PS3
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Projects/Main/Exe_PS3/Exe_Main_PS3.vcproj makefile.Exe_Main_PS3 ../Misc/Scripts/ConvSettings.txt> log
Exe_Main_PS3: makefile.Exe_Main_PS3 testconfig $(CreateOutputDirs) $(Exe_Main_PS3TargetPath) Exe_Main_PS3ExtraTargets 

makefile.Lib_GameWorld: Projects/Main/GameWorld/Lib_GameWorld.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_GameWorld
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Projects/Main/GameWorld/Lib_GameWorld.vcproj makefile.Lib_GameWorld ../Misc/Scripts/ConvSettings.txt> log
Lib_GameWorld: makefile.Lib_GameWorld testconfig $(CreateOutputDirs) $(Lib_GameWorldTargetPath) Lib_GameWorldExtraTargets 

makefile.Lib_XR: Shared/MOS/XR/Lib_XR.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_XR
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Shared/MOS/XR/Lib_XR.vcproj makefile.Lib_XR ../Misc/Scripts/ConvSettings.txt> log
Lib_XR: makefile.Lib_XR testconfig $(CreateOutputDirs) $(Lib_XRTargetPath) Lib_XRExtraTargets 

makefile.Lib_GameClassesObjChar: Projects/Main/GameClasses/Lib_GameClassesObjChar.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_GameClassesObjChar
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Projects/Main/GameClasses/Lib_GameClassesObjChar.vcproj makefile.Lib_GameClassesObjChar ../Misc/Scripts/ConvSettings.txt> log
Lib_GameClassesObjChar: makefile.Lib_GameClassesObjChar testconfig $(CreateOutputDirs) $(Lib_GameClassesObjCharTargetPath) Lib_GameClassesObjCharExtraTargets 

makefile.Lib_GameClassesMain: Projects/Main/GameClasses/Lib_GameClassesMain.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_GameClassesMain
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Projects/Main/GameClasses/Lib_GameClassesMain.vcproj makefile.Lib_GameClassesMain ../Misc/Scripts/ConvSettings.txt> log
Lib_GameClassesMain: makefile.Lib_GameClassesMain testconfig $(CreateOutputDirs) $(Lib_GameClassesMainTargetPath) Lib_GameClassesMainExtraTargets 

makefile.Lib_GameClassesOther: Projects/Main/GameClasses/Lib_GameClassesOther.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_GameClassesOther
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Projects/Main/GameClasses/Lib_GameClassesOther.vcproj makefile.Lib_GameClassesOther ../Misc/Scripts/ConvSettings.txt> log
Lib_GameClassesOther: makefile.Lib_GameClassesOther testconfig $(CreateOutputDirs) $(Lib_GameClassesOtherTargetPath) Lib_GameClassesOtherExtraTargets 

makefile.Lib_RenderPS3GCM: Shared/MOS/RenderContexts/PS3GCM/Lib_RenderPS3GCM.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_RenderPS3GCM
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Shared/MOS/RenderContexts/PS3GCM/Lib_RenderPS3GCM.vcproj makefile.Lib_RenderPS3GCM ../Misc/Scripts/ConvSettings.txt> log
Lib_RenderPS3GCM: makefile.Lib_RenderPS3GCM testconfig $(CreateOutputDirs) $(Lib_RenderPS3GCMTargetPath) Lib_RenderPS3GCMExtraTargets 

makefile.Lib_MCC: Shared/MCC/Lib_MCC.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_MCC
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Shared/MCC/Lib_MCC.vcproj makefile.Lib_MCC ../Misc/Scripts/ConvSettings.txt> log
Lib_MCC: makefile.Lib_MCC testconfig $(CreateOutputDirs) $(Lib_MCCTargetPath) Lib_MCCExtraTargets 

makefile.Lib_GameClassesModelsAI: Projects/Main/GameClasses/Lib_GameClassesModelsAI.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_GameClassesModelsAI
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Projects/Main/GameClasses/Lib_GameClassesModelsAI.vcproj makefile.Lib_GameClassesModelsAI ../Misc/Scripts/ConvSettings.txt> log
Lib_GameClassesModelsAI: makefile.Lib_GameClassesModelsAI testconfig $(CreateOutputDirs) $(Lib_GameClassesModelsAITargetPath) Lib_GameClassesModelsAIExtraTargets 

makefile.Lib_XRClasses: Shared/MOS/Libs/XRClasses/Lib_XRClasses.vcproj ../Misc/Scripts/ConvSettings.txt ../Misc/Scripts/GenerateLibMakeFile.pl
	@echo Creating makefile for Lib_XRClasses
	@perl ../Misc/Scripts/GenerateLibMakeFile.pl Shared/MOS/Libs/XRClasses/Lib_XRClasses.vcproj makefile.Lib_XRClasses ../Misc/Scripts/ConvSettings.txt> log
Lib_XRClasses: makefile.Lib_XRClasses testconfig $(CreateOutputDirs) $(Lib_XRClassesTargetPath) Lib_XRClassesExtraTargets 


ProjectMakefiles: Projects/Main/GameClasses/makefile.Lib_GameClassesObjMisc Projects/Main/Exe_PS3/makefile.Exe_Main_PS3Build Shared/MOS/MSystem/makefile.Lib_MSystem Projects/Main/Exe_PS3/makefile.Exe_Main_PS3 Projects/Main/GameWorld/makefile.Lib_GameWorld Shared/MOS/XR/makefile.Lib_XR Projects/Main/GameClasses/makefile.Lib_GameClassesObjChar Projects/Main/GameClasses/makefile.Lib_GameClassesMain Projects/Main/GameClasses/makefile.Lib_GameClassesOther Shared/MOS/RenderContexts/PS3GCM/makefile.Lib_RenderPS3GCM Shared/MCC/makefile.Lib_MCC Projects/Main/GameClasses/makefile.Lib_GameClassesModelsAI Shared/MOS/Libs/XRClasses/makefile.Lib_XRClasses 

clean: testconfig Lib_GameClassesObjMiscclean Exe_Main_PS3Buildclean Lib_MSystemclean Exe_Main_PS3clean Lib_GameWorldclean Lib_XRclean Lib_GameClassesObjCharclean Lib_GameClassesMainclean Lib_GameClassesOtherclean Lib_RenderPS3GCMclean Lib_MCCclean Lib_GameClassesModelsAIclean Lib_XRClassesclean 
	@rm $(LinkDir)/* -f

CreateOutputDirs: testconfig CreateLinkDir CreateContentCompileExeDir Lib_GameClassesObjMisc_CreateOutputDir Exe_Main_PS3Build_CreateOutputDir Lib_MSystem_CreateOutputDir Exe_Main_PS3_CreateOutputDir Lib_GameWorld_CreateOutputDir Lib_XR_CreateOutputDir Lib_GameClassesObjChar_CreateOutputDir Lib_GameClassesMain_CreateOutputDir Lib_GameClassesOther_CreateOutputDir Lib_RenderPS3GCM_CreateOutputDir Lib_MCC_CreateOutputDir Lib_GameClassesModelsAI_CreateOutputDir Lib_XRClasses_CreateOutputDir 


CreateContentCompileExeDir:
	@mkdir -p cd S:/Projects/P5/ContentCompile/PS3/System/PS3Exes
