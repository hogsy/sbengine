/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character class for darkling characters.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharDarkling

	Comments:

	History:		
		050311:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharDarkling_h__
#define __WObj_CharDarkling_h__

#include "WObj_CharPlayer.h"
#include "WObj_CharDarkling_ClientData.h"


class CWObject_CharDarkling : public CWObject_Character//CWObject_CharPlayer
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_Character parent;
	//typedef CWObject_CharPlayer parent;

	int32 m_iLightHurtObj;
	void FadeLightHurt(bool _bFadeIn);

public:
	// Client data - server only interface
	const CWO_CharDarkling_ClientData& GetClientData() const { return GetClientData(this); }
	      CWO_CharDarkling_ClientData& GetClientData()       { return GetClientData(this); }
	// Client data - client/server interface
	static const CWO_CharDarkling_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_CharDarkling_ClientData& GetClientData(CWObject_CoreData* _pObj);

	CWObject_CharDarkling();

	static void GetLightSparks(CWorld_Client* _pWClient, uint& _bAttach, uint& _nAttach, uint _Flags);

	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static int  OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo);

//	static int  OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
//	virtual int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags);

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);
	static void OnIncludeClass(CMapData* _pMapData, CWorld_Server* _pWServer);

	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnDestroy();

	virtual void Char_PrepareSpawn();
	virtual void Char_ReleaseSpawn(CWO_CharDarkling_ClientData* _pCD);
	virtual bool Char_IsSpawning();

	// AI-interface
	virtual bool AI_IsForceRotated();
	virtual bool AI_IsJumping();
	virtual bool AI_CanJump(const bool _bFromAir = false);
	virtual bool AI_Jump(const CMat4Dfp32* _pDestMat, int32 _Flags);
	virtual void AI_EnableWallClimbing(bool _bActivate);				// Activate/deactivate wallclimbing
	virtual bool AI_IsWallClimbingEnabled(); 							// Returns true if wall climbing is enabled
	virtual bool AI_IsOnWall();											// Returns true when not on ground
};



#endif // __WObj_CharDarkling_h__
