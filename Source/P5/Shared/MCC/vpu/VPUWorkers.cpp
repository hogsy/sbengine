
#include "PCH.h"
#include "VPU_Platform.h"
// build on ps3:
// Command line:	echo Compiling... && cd $(IntDir) && %SCE_PS3_ROOT%/host-win32/spu/bin/spu-lv2-gcc -c -gdwarf-2 -I "C:\Program Files\SN Systems\PS3\spu\include\sn" -I "%SCE_PS3_ROOT%/target/spu/include" -D "SN_TARGET_PS3_SPU" -D "_DEBUG" -D "__GCC__" -D "SPU" -Wall -Wno-multichar -fno-exceptions -fcheck-new -fno-strict-aliasing -o "$(InputName).spu.o" "$(InputFileName)" && echo Compile done && echo Linking... && %SCE_PS3_ROOT%/host-win32/spu/bin/spu-lv2-gcc -g -o "$(InputName).elf" "$(InputName).spu.o" -ldma -lsputhread -Wl,-Map="$(InputName).spu.map" && echo Link done && echo Objcopying... && %SCE_PS3_ROOT%/host-win32/ppu/bin/ppu-lv2-objcopy -I binary -O elf64-powerpc-celloslv2 -B powerpc --debugging --set-section-align .data=7 --set-section-pad .data=128 --rename-section .data=.spuelf.$(InputName) "$(InputName).elf" "$(InputName).ppu.o" && echo Objcopy done && copy /Y "$(InputName).spu.elf" "S:\Projects\P5\Build" && copy /Y "$(InputName).spu.elf" "S:\Projects\P5\ContentCompile\PS3\"
// Command line:	echo Compiling... && %CELL_SDK%/host-win32/spu/bin/spu-lv2-gcc -c -gdwarf-2 -I "C:\Program Files\SN Systems\PS3\spu\include\sn" -I "%CELL_SDK%/target/spu/include" -D "SN_TARGET_PS3_SPU" -D "_DEBUG" -D "__GCC__" -D "SPU" -Wall -fno-exceptions -fcheck-new -fno-strict-aliasing -o "$(IntDir)\$(InputName).spu.o" "$(InputPath)" && echo Compile done && echo Linking... && %CELL_SDK%/host-win32/spu/bin/spu-lv2-gcc -g -o "$(IntDir)\$(InputName).spu.elf" "$(IntDir)\$(InputName).spu.o" -ldma -lsputhread -Wl,-Map="$(IntDir)\$(InputName).spu.map" && echo Link done && echo Objcopying... && %CELL_SDK%/host-win32/ppu/bin/ppu-lv2-objcopy -I binary -O elf64-powerpc-celloslv2 -B powerpc --debugging --set-section-align .data=7 --set-section-pad .data=128 --rename-section .data=.spuelf.$(InputName) "$(IntDir)\$(InputName).spu.elf" "$(IntDir)\$(InputName).ppu.o" && echo Objcopy done && copy /Y "$(IntDir)\$(InputName).spu.elf" "S:\Projects\P5\Build"
// Output:			$(IntDir)\$(InputName).ppu.o
#ifndef PLATFORM_SPU
#include "../MMath_Vec128.h"
#else
#include "../../mcc/MMath_Vec128_Spu.h"
#endif

#include "VPUWorkerTransform.h"
#include "VPUWorkerShadow.h"

MVPU_MAIN


MVPU_WORKER_BEGIN
MVPU_WORKER_DECLARE(MHASH3('TEST','_CAC','HE'),VPUWorker_TestCache);
MVPU_WORKER_DECLARE(MHASH3('TEST','_STR','EAM'),VPUWorker_TestStream);
MVPU_WORKER_DECLARE(MHASH2('SHAD','OW'),VPUWorker_Shadow);
MVPU_WORKER_END

/*
int main(void)
{	
	VPU_Main();
}



uint32 VPU_Worker(uint32 _JobHash,CVPU_JobInfo& _JobInfo)
{
	switch(_JobHash)
	{
		case MHASH3('TRAN','SFOR','MV4') : 
			VPUWorker_TransformV4(_JobInfo);
		default :
			M_TRACE("Unrecognized job. JobHash: %d", _JobHash);
	}
	return 0;
}

*/
