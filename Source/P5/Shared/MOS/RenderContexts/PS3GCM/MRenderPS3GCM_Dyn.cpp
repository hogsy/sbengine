
#include "MRTC.h"

#ifdef PS3_RENDERER_GCM
class CRegisterRenderPS3GCM
{
public:
	CRegisterRenderPS3GCM()
	{
		MRTC_REFERENCE(CDCPS3GCM);
		MRTC_REFERENCE(CRCPS3GCM);
	}
};

CRegisterRenderPS3GCM g_RenderPS3GCMDyn;

#endif
