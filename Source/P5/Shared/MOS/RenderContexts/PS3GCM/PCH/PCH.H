#include "MRTC.h"

#ifdef PS3_RENDERER_PSGL
#error "PSGL renderer is enabled in Target_PS3.h, compiling GCM will break."
#endif

#ifdef PLATFORM_PS3
 #include <cell/gcm.h>
 #include <sdk_version.h>
#else
 #include "PCH_PlatformEmulation.h"
#endif

#define DAssert(_Statement, _Func, _Result) if(!(_Statement _Func _Result)) {M_BREAKPOINT;}

#include "../MRenderPS3_CellWrappers.h"

extern void* gs_lpPS3Heaps[8];
extern uint32 gs_nPS3Heaps;
extern uint32 gs_PS3HeapSize[8];
extern uint32 gs_PS3PushBufferSize;

#include "../../../MSystem/Raster/MTexture.h"
#include "../MRenderPS3_Def.h"
#include "../MRenderPS3_Heap.h"
#include "../MRenderPS3_Resource.h"
#include "../MDisplayPS3.h"
#include "../MRenderPS3_Context.h"


//#undef MSCOPE
//#define MSCOPE(_a_, _b_) LogFile("###CallGraphLog: " #_a_);
