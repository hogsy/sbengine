/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character class for Angelus characters.

	Author:			Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharAngelus

	Comments:

	History:		
		051114:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharAngelus_h__
#define __WObj_CharAngelus_h__

#include "WObj_CharNPC.h"
#include "WObj_CharAngelus_ClientData.h"

// Blink time defines for angelus
#define CHARANGELUS_BLINKTIME		0.2f
#define CHARANGELUS_MINNONBLINKGAP	4.0f
#define CHARANGELUS_MAXNONBLINKGAP	10.0f
#define CHARANGELUS_BLINKROTANGLE	0.083f
#define CHARANGELUS_BLENDTIME		5.0f

// Misc. macros used in OnGetAnimState
#define CHARANGELUS_SINC(_x)		(M_Sin((_x - 0.5f)*_PI)*0.5f + 0.5f)

#define CHARANGLELUS_MULTMAT( _M, _M2 )	\
{ \
	CMat4Dfp32 Tmp; \
	_M2.Multiply(_M, Tmp); \
	_M = Tmp; \
}

#define CHARANGELUS_MULTMATMP( _M, _M2 ) \
{ \
	CMat43fp32 Tmp; \
	_M2.Multiply(_M, Tmp); \
	_M = Tmp; \
}

#define CHARANGELUS_APPLYQUAT(_Q1, _Q2, _B, _Blend) \
{\
	if(_Blend > 0)\
	{\
		if(!_B)\
			_Q2.Unit();\
		_Q1.Lerp(_Q2, _Blend, _Q2);\
	}\
	else\
		_Q2 = _Q1;\
	_B = true;\
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_CharAngelus
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_CharAngelus : public CWObject_CharNPC
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_CharNPC parent;

private:
	spCRPG_Object	m_spRPGObject_AuraWeapon;
	int				m_iChain;

public:

	virtual void UnspawnCharacter();
	virtual void SpawnCharacter(int _PhysMode = PLAYER_PHYS_STAND, int _SpawnBehavior = 0);

	// Client data - server only interface
	const CWO_CharAngelus_ClientData& GetClientData() const { return GetClientData(this); }
	      CWO_CharAngelus_ClientData& GetClientData()       { return GetClientData(this); }
	// Client data - client/server interface
	static const CWO_CharAngelus_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_CharAngelus_ClientData& GetClientData(CWObject_CoreData* _pObj);

    static void  OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void  OnIncludeClass(CMapData* _pWData, CWorld_Server* _pWServer);
	static void  OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);
	static aint  OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
    static void  OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void  OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static bool  OnGetAnimState(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CXR_Skeleton* _pSkel, int _iCacheSlot, const CMat4Dfp32& _Pos, fp32 _IPTime, CXR_AnimState& _Anim, CVec3Dfp32* _pRetPos = 0, bool _bIgnoreCache = false, fp32 _ClothScaleOverride = -2.0f);
	static bool  OnGetAnimStateCloth(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CXR_Skeleton* _pSkel, CXR_Skeleton* _pOrgSkel, CXR_SkeletonInstance* _pSkelInstance, const int& _iCacheSlot, const fp32& _IPTime, const fp32& _ClothScaleOverride);
	
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
};

#endif // __WObj_CharAngelus_h__

