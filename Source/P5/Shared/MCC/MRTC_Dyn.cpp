#include "MRTC.h"

class CRegisterMRTC
{
public:
	CRegisterMRTC()
	{
		MRTC_REFERENCE(CCExceptionLog);
		MRTC_REFERENCE(CReferenceCount);
		#ifdef MRTC_ENABLE_REMOTEDEBUGGER
			MRTC_REFERENCE(CCException);
			MRTC_REFERENCE(CCExceptionMemory);
			MRTC_REFERENCE(CCExceptionFile);
			MRTC_REFERENCE(CCExceptionGraphicsHAL);
		#endif
	}
};

CRegisterMRTC g_MRTCDyn;

