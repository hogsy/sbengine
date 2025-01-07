#ifndef __WOBJ_IMPULSECONTAINER_H
#define __WOBJ_IMPULSECONTAINER_H

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"

class CWO_Impulse
{
public:
	CWO_Impulse(int _iImpulse = 0) { m_iImpulse = _iImpulse; }
	
	TArray<CWO_SimpleMessage> m_lMessages;
	int m_iImpulse;
};

class CWO_ImpulseContainer
{
public:
	TArray<CWO_Impulse> m_lImpulses;

	bool OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey, CWorld_Server *_pWServer);
	int OnImpulse(int _iImpulse, int _iObject, int _iActivator, CWorld_Server *_pWServer);
	void SendPrecache(int _iObject, CWorld_Server *_pWServer);
};

#endif
