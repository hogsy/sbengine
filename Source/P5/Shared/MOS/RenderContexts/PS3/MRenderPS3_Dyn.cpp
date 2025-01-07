
#include "MRTC.h"

class CRegisterRenderPS3
{
public:
	CRegisterRenderPS3()
	{
		MRTC_REFERENCE(CDisplayContextPS3);
		MRTC_REFERENCE(CRenderContextPS3);
	}
};

CRegisterRenderPS3 g_RenderPS3Dyn;
