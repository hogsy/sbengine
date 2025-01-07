#ifndef __WOBJ_ROCKET_H
#define __WOBJ_ROCKET_H

#if !defined(M_DISABLE_TODELETE) || 1
#include "../WObj_Misc/WObj_Explosion.h"

class CWObject_Rocket : public CWObject_Explosion
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CVec3Dfp32 m_Destiation;

	virtual void OnCreate();
	virtual void OnFinishEvalKeys();
	virtual void OnRefresh();
	static void OnIncludeClass(CMapData *_pWData, CWorld_Server *_pWServer);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static CXR_ModelInstance *GetRocketModelInstance(CWObject_Client* _pObj, CWorld_Client* _pWClient);
};
#endif

#endif
