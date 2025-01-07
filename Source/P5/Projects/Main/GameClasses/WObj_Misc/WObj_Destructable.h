#ifndef __WOBJ_DESTRUCTABLE_H
#define __WOBJ_DESTRUCTABLE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Destructable entity

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWObject_Destructable
					CWObject_Mine
\*____________________________________________________________________________________________*/

#include "WObj_Explosion.h"

#define CWObject_DestructableParent CWObject_Explosion
class CWObject_Destructable : public CWObject_Explosion
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	int m_Hitpoints;
	uint32 m_DamageImmunity;
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void UpdateVisibility(int *_lpExtraModels = NULL, int _nExtraModels = 0);
	virtual aint OnMessage(const CWObject_Message &_Msg);
	virtual void SpawnExplosion(int _iSender, const aint* _pParam = NULL, int _nParam = 0);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);
};

#endif
