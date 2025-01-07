#include "MRTC.h"

class CRegisterMCC
{
public:
	CRegisterMCC()
	{
		MRTC_REFERENCE(CCExceptionLog);
		MRTC_REFERENCE(CReferenceCount);
		#ifdef MRTC_ENABLE_REMOTEDEBUGGER
			MRTC_REFERENCE(CCException);
			MRTC_REFERENCE(CCExceptionMemory);
			MRTC_REFERENCE(CCExceptionFile);
			MRTC_REFERENCE(CCExceptionGraphicsHAL);
		#endif
		MRTC_REFERENCE(ILogFile);
		MRTC_REFERENCE(IProgress);
		MRTC_REFERENCE(CLogFile);
		MRTC_REFERENCE(CNetwork);
		MRTC_REFERENCE(CNetwork_Device);
		MRTC_REFERENCE(CNetworkCore);
		MRTC_REFERENCE(CNetwork_Device_UDP);

	}
};

CRegisterMCC g_MCCDyn;
