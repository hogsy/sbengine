########################################################
# PS3 PPU Makefile generated for GNU Make by ProDG VSI.NET #
########################################################

.SILENT:
.SUFFIXES:
.SUFFIXES: .c .cpp .cxx .cc .s .asm

OUTDIR = PS3RTM~1
INTDIR = PS3RTM~1

LIBS=\
	 "-lsn"\
	 "-lm"\
	 "-lio"\
	 "-lpthread"\
	 "-lkb"\
	 "-lpad"\
	 "-lnet"\
	 "-lfs"\
	 "-lcgc"\
	 "-lpsgl"\
	 "-lpsglu"\
	 "-lpsglcgc"\
	 "-lgcm"\
	 "-lsysutil"\
	 "d:/Source/P5/Shared/Mos/Libs/XRCLAS~1/Output/Win32/LIB_XR~1/PS3RTM~1/LIB_XR~1.lib"\
	 "d:/Source/P5/Shared/Mos/RENDER~1/Ps3/Output/Win32/LIB_RE~1/PS3RTM~1/LIB_RE~1.lib"\
	 "d:/Source/P5/Shared/Mcc/Output/Win32/Lib_MCC/PS3RTM~1/Lib_MCC.lib"\
	 "d:/Source/P5/Shared/Mos/MSystem/Output/Win32/LIB_MS~1/PS3RTM~1/LIB_MS~1.lib"\

AS=gcc2vs ppu-lv2-gcc
AS_FLAGS=\
	-DTARGET_PS3\
	-DSN_TARGET_PS3\
	-D__GCC__\
	-DM_RTM\
	-DM_Profile\
	-I"D:/Program (x86)/Programming/SN Systems/PS3/include"\
	-I"D:/usr/local/cell/0_8_3//target-ceb/ppu/include"\
	-I"D:/usr/local/cell/0_8_3//target-ceb/ppu/include/network"\
	-I"d:/Source/P5/Research/XBench//../../Shared/MCC"\
	-I"d:/Source/P5/Research/XBench//PCH"\
	-I"d:/Source/P5/Research/XBench//../../SDK/Include"\
	-I. -c\

CC=gcc2vs ppu-lv2-gcc
CC_FLAGS=\
	-DTARGET_PS3\
	-DSN_TARGET_PS3\
	-D__GCC__\
	-DM_RTM\
	-DM_Profile\
	-I"D:/Program (x86)/Programming/SN Systems/PS3/include"\
	-I"D:/usr/local/cell/0_8_3//target-ceb/ppu/include"\
	-I"D:/usr/local/cell/0_8_3//target-ceb/ppu/include/network"\
	-I"d:/Source/P5/Research/XBench//../../Shared/MCC"\
	-I"d:/Source/P5/Research/XBench//PCH"\
	-I"d:/Source/P5/Research/XBench//../../SDK/Include"\
	-I. -c\
	-MMD\
	-Wall\
	-fno-exceptions\
	-O3\
	-Wno-non-virtual-dtor\
	-Wno-sign-compare\
	-Wno-conversion\
	-Wno-multichar\
	-Wno-unused\
	-Werror\

CXX_FLAGS=$(CC_FLAGS)

AR=gcc2vs ppu-lv2-ar

LD=gcc2vs ppu-lv2-gcc
LD_FLAGS=\
	-L"D:/PROGRA~1/PROGRA~1/SNSYST~1/PS3/ppu/lib/sn"\
	-L"D:/usr/local/cell/0_8_3//target/ppu/lib"\
	-L"D:/usr/local/cell/0_8_3//target/ppu/lib/psgl/rsx/opt"\

LD_OBJS=\
	"PS3RTM~1/MMain.obj"\
	"PS3RTM~1/Mrtc.obj"\
	"PS3RTM~1/XBench.obj"\
	"PS3RTM~1/XRSurf.obj"\
	"PS3RTM~1/XRSurfaceContext.obj"\
	"PS3RTM~1/XRVBContext.obj"\
	"PS3RTM~1/XRVBManager.obj"\
	"PS3RTM~1/MRenderUtil.obj"\
	"PS3RTM~1/MWinGrph.obj"\
	"PS3RTM~1/MCC_Dyn.obj"\
	"PS3RTM~1/MRenderPS3_Dyn.obj"\
	"PS3RTM~1/MSystem_Dyn.obj"\

COMPILE: $(LD_OBJS)

"PS3RTM~1/MMain.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/MMain.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/MMain.cpp" -o $@

"PS3RTM~1/Mrtc.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mcc/Mrtc.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mcc/Mrtc.cpp" -o $@

"PS3RTM~1/XBench.obj":
	@echo ProDG Compiling "d:/Source/P5/Research/XBench/XBench.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Research/XBench/XBench.cpp" -o $@

"PS3RTM~1/XRSurf.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/XRSurf.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/XRSurf.cpp" -o $@

"PS3RTM~1/XRSurfaceContext.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/XRSurfaceContext.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/XRSurfaceContext.cpp" -o $@

"PS3RTM~1/XRVBContext.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/XRVBContext.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/XRVBContext.cpp" -o $@

"PS3RTM~1/XRVBManager.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/XRVBManager.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/XRVBManager.cpp" -o $@

"PS3RTM~1/MRenderUtil.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Classes/Render/MRenderUtil.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Classes/Render/MRenderUtil.cpp" -o $@

"PS3RTM~1/MWinGrph.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Classes/Win/MWinGrph.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Classes/Win/MWinGrph.cpp" -o $@

"PS3RTM~1/MCC_Dyn.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mcc/MCC_Dyn.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mcc/MCC_Dyn.cpp" -o $@

"PS3RTM~1/MRenderPS3_Dyn.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Dyn.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Dyn.cpp" -o $@

"PS3RTM~1/MSystem_Dyn.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/MSystem/MSystem_Dyn.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/MSystem/MSystem_Dyn.cpp" -o $@

LINK: "PS3RTM~1/PS3 RTMD.elf"

"PS3RTM~1/PS3 RTMD.elf" : $(LD_OBJS)\
	"d:/Source/P5/Shared/Mos/Libs/XRClasses/Output/Win32/Lib_XRClasses/PS3 RTMD/Lib_XRClasses.lib"\
	"d:/Source/P5/Shared/Mos/RenderContexts/Ps3/Output/Win32/Lib_RenderPS3/PS3 RTMD/Lib_RenderPS3.lib"\
	"d:/Source/P5/Shared/Mcc/Output/Win32/Lib_MCC/PS3 RTMD/Lib_MCC.lib"\
	"d:/Source/P5/Shared/Mos/MSystem/Output/Win32/Lib_MSystem/PS3 RTMD/Lib_MSystem.lib"
	@echo ProDG Linking $@
	$(LD) $(LD_FLAGS) -Wl,--start-group $(LD_OBJS) $(LIBS) -Wl,--end-group -o $@

ifeq ($(MAKECMDGOALS),COMPILE)
include "PS3RTM~1/MMain.d"
include "PS3RTM~1/Mrtc.d"
include "PS3RTM~1/XBench.d"
include "PS3RTM~1/XRSurf.d"
include "PS3RTM~1/XRSURF~1.d"
include "PS3RTM~1/XRVBCO~1.d"
include "PS3RTM~1/XRVBMA~1.d"
include "PS3RTM~1/MRENDE~2.d"
include "PS3RTM~1/MWinGrph.d"
include "PS3RTM~1/MCC_Dyn.d"
include "PS3RTM~1/MRENDE~1.d"
include "PS3RTM~1/MSYSTE~1.d"
endif

