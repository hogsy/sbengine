#ifndef __WOBJ_EXPLOSION_H
#define __WOBJ_EXPLOSION_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Explosion entity

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWObject_Explosion
\*____________________________________________________________________________________________*/

#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"

class CWObject_Explosion : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	enum
	{
		FLAGS_INSTANT		= 1,
		FLAGS_SHOCKWAVE		= 2,
		FLAGS_CAMERASHAKE	= 4,
		FLAGS_STATICLIGHT	= 8,
		FLAGS_DYNAMICLIGHT	= 16,
		FLAGS_WALLMARK		= 32,
		FLAGS_RUMBLE		= 64,
		FLAGS_DEBRIS		= 128,
		FLAGS_SHOWFIRSTMODEL = 256,
		FLAGS_NOSELFDAMAGE = 512,
	};
	static const char * ms_FlagsTranslate[];

	class CClientData : public CReferenceCount
	{
	public:
		TPtr<CXR_ModelInstance> m_spDebrisInstance;
		TPtr<CXR_ModelInstance> m_spInstance1;
	};

	CWO_Shockwave m_Shockwave;
	int m_Shockwave_IntersectionFlags;

	CWO_CameraShake m_CameraShake;

	uint16 m_Flags;
	CPixel32 m_Light_Color;
	uint16 m_Light_Range;
	uint16 m_Light_DurationTicks;
	CStr m_Light_Target;
	uint16 m_iExplosionSound;
	uint16 m_DurationTicks;
	CStr m_Rumble;
	uint16 m_Rumble_Range;

	uint16 m_Wallmark_iSurface;
	uint16 m_Wallmark_Radius;

	TArray<CWO_SimpleMessage> m_lMessages_Explode;

public:
	virtual void SpawnExplosion(int _iSender, const aint* _pParam = 0, int _nParam = 0);
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void OnSpawnWorld();
	virtual void OnRefresh();
	virtual void OnInitInstance(const aint* _pParam, int _nParam);

	virtual aint OnMessage(const CWObject_Message &_Msg);
	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, const CMat4Dfp32& _ParentMat);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

	static CClientData *GetClientData(CWObject_CoreData* _pObj);
};

#endif
