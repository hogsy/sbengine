#include "MRTC.h"

#ifdef PLATFORM_PS3
 #include <gles/gl.h>
 #include <gles/glext.h>
 #include <psgl/psgl.h>
 #include <cg/cg.h>
 #include <psgl/psglu.h>
#else
 #include "PCH_PlatformEmulation.h"
#endif

#include "../../../MSystem/Raster/MTexture.h"
#include "../MDisplayPS3.h"
#include "../MRenderPS3_Context.h"
#include "../MRenderPS3_Def.h"

