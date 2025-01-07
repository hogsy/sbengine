########################################################
# PS3 PPU Makefile generated for GNU Make by ProDG VSI.NET #
########################################################

.SILENT:
.SUFFIXES:
.SUFFIXES: .c .cpp .cxx .cc .s .asm

OUTDIR = ./Output/Win32/LIB_XR~1/PS3RTM~1
INTDIR = ./INTERM~1/Win32/LIB_XR~1/PS3RTM~1

LIBS=\

AS=gcc2vs ppu-lv2-gcc
AS_FLAGS=\
	-DSN_TARGET_PS3\
	-DNDEBUG\
	-D__GCC__\
	-DM_RTM\
	-DM_Profile\
	-I"D:/PROGRA~1/PROGRA~1/SNSYST~1/PS3/ppu/include/sn"\
	-I"D:/usr/local/cell/0_8_3//target/ppu/include"\
	-I"d:/Source/P5/Shared/Mos/Libs/XRClasses/../../XR/PCH"\
	-I"d:/Source/P5/Shared/Mos/Libs/XRClasses/../../../mcc"\
	-I. -c\
	-g\

CC=gcc2vs ppu-lv2-gcc
CC_FLAGS=\
	-DSN_TARGET_PS3\
	-DNDEBUG\
	-D__GCC__\
	-DM_RTM\
	-DM_Profile\
	-I"D:/PROGRA~1/PROGRA~1/SNSYST~1/PS3/ppu/include/sn"\
	-I"D:/usr/local/cell/0_8_3//target/ppu/include"\
	-I"d:/Source/P5/Shared/Mos/Libs/XRClasses/../../XR/PCH"\
	-I"d:/Source/P5/Shared/Mos/Libs/XRClasses/../../../mcc"\
	-I. -c\
	-MMD\
	-g\
	-Wall\
	-fno-exceptions\
	-O3\
	-Wno-non-virtual-dtor\
	-Wno-sign-compare\
	-Wno-conversion\
	-Wno-multichar\
	-Wno-unused\
	-Werror\
	-fno-strict-aliasing\
	-maltivec\

CXX_FLAGS=$(CC_FLAGS)

AR=gcc2vs ppu-lv2-ar

LD=gcc2vs ppu-lv2-gcc
LD_FLAGS=\

LD_OBJS=\
	"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRClass.obj"\
	"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRLight.obj"\
	"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRMediumDesc.obj"\
	"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRVBContainer.obj"\
	"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRVBMHelper.obj"\
	"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRVertexBuffer.obj"\
	"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/PCH.obj"\

COMPILE: $(LD_OBJS)

"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRClass.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/XRClass.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/XRClass.cpp" -o $@

"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRLight.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/XRLight.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/XRLight.cpp" -o $@

"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRMediumDesc.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/XRMediumDesc.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/XRMediumDesc.cpp" -o $@

"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRVBContainer.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/XRVBContainer.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/XRVBContainer.cpp" -o $@

"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRVBMHelper.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/XRVBMHelper.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/XRVBMHelper.cpp" -o $@

"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRVertexBuffer.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/XRVertexBuffer.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/XRVertexBuffer.cpp" -o $@

"./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/PCH.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Xr/Pch/PCH.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Xr/Pch/PCH.cpp" -o $@

LIB: "./Output/Win32/LIB_XR~1/PS3RTM~1/Lib_XRClasses.lib"

"./Output/Win32/LIB_XR~1/PS3RTM~1/Lib_XRClasses.lib" : $(LD_OBJS)
	@echo ProDG Creating Library $@
	$(AR) rvus $@ $(LD_OBJS) $(LIBS)

ifeq ($(MAKECMDGOALS),COMPILE)
include "./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRClass.d"
include "./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRLight.d"
include "./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRMEDI~1.d"
include "./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRVBCO~1.d"
include "./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRVBMH~1.d"
include "./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/XRVERT~1.d"
include "./INTERM~1/Win32/LIB_XR~1/PS3RTM~1/PCH.d"
endif

