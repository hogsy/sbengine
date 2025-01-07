########################################################
# PS3 PPU Makefile generated for GNU Make by ProDG VSI.NET #
########################################################

.SILENT:
.SUFFIXES:
.SUFFIXES: .c .cpp .cxx .cc .s .asm

OUTDIR = ./Output/Win32/LIB_RE~1/PS3RTM~1
INTDIR = ./INTERM~1/Win32/LIB_RE~1/PS3RTM~1

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
	-I"D:/usr/local/cell/0_8_3//target/ppu/include/network"\
	-I"d:/Source/P5/Shared/Mos/RenderContexts/Ps3/PCH"\
	-I"d:/Source/P5/Shared/Mos/RenderContexts/Ps3/../../../mcc"\
	-I"d:/Source/P5/Shared/Mos/RenderContexts/Ps3/../../../../SDK/Include"\
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
	-I"D:/usr/local/cell/0_8_3//target/ppu/include/network"\
	-I"d:/Source/P5/Shared/Mos/RenderContexts/Ps3/PCH"\
	-I"d:/Source/P5/Shared/Mos/RenderContexts/Ps3/../../../mcc"\
	-I"d:/Source/P5/Shared/Mos/RenderContexts/Ps3/../../../../SDK/Include"\
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
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Attrib.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_OcclusionQuery.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MDisplayPS3.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Context.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Image.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Cmd.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Render.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_RenderVA.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Texture.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_VBO.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_FragmentProgram.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_VertexProgram.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/Pch.obj"\
	"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderVPGen.obj"\

COMPILE: $(LD_OBJS)

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Attrib.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Attrib.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Attrib.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_OcclusionQuery.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_OcclusionQuery.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_OcclusionQuery.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MDisplayPS3.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MDisplayPS3.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MDisplayPS3.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Context.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Context.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Context.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Image.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Image.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Image.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Cmd.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Cmd.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Cmd.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Render.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Render.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Render.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_RenderVA.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_RenderVA.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_RenderVA.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_Texture.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Texture.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_Texture.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_VBO.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_VBO.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_VBO.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_FragmentProgram.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_FragmentProgram.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_FragmentProgram.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderPS3_VertexProgram.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_VertexProgram.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/MRenderPS3_VertexProgram.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/Pch.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/Pch/Pch.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/RenderContexts/Ps3/Pch/Pch.cpp" -o $@

"./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRenderVPGen.obj":
	@echo ProDG Compiling "d:/Source/P5/Shared/Mos/Classes/Render/MRenderVPGen.cpp"
	$(CC) $(CC_FLAGS) "d:/Source/P5/Shared/Mos/Classes/Render/MRenderVPGen.cpp" -o $@

LIB: "./Output/Win32/LIB_RE~1/PS3RTM~1/Lib_RenderPS3.lib"

"./Output/Win32/LIB_RE~1/PS3RTM~1/Lib_RenderPS3.lib" : $(LD_OBJS)
	@echo ProDG Creating Library $@
	$(AR) rvus $@ $(LD_OBJS) $(LIBS)

ifeq ($(MAKECMDGOALS),COMPILE)
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRENDE~2.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRENDE~1.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MDISPL~1.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRENDE~3.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRENDE~4.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MR5CFE~1.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRA851~1.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MR66DD~1.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRF040~1.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRF3A1~1.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MR2611~1.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MR856A~1.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/Pch.d"
include "./INTERM~1/Win32/LIB_RE~1/PS3RTM~1/MRE160~1.d"
endif

