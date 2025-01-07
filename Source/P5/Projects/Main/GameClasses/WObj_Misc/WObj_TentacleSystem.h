/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CWObject_TentacleSystem.

	Author:			Anton Ragnarsson, Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_TentacleSystem

	Comments:

	History:		
\*____________________________________________________________________________________________*/
#ifndef __WObj_TentacleSystem_h__
#define __WObj_TentacleSystem_h__

#include "../WObj_Char.h"
#include "../WObj_Char/WObj_CharDarkling.h"
#include "../WObj_Char/WObj_CharDarkling_ClientData.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_TentacleSystem
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_TentacleSystem : public CWObject
{
	typedef CWObject parent;
	MRTC_DECLARE_SERIAL_WOBJECT;

private:
	CWObject* m_pSelectedObject;
	CVec3Dfp32 m_LocalGrabPoint;

	int8 m_InterestingType;
	fp32 m_ArmMaxLength;
	fp32 m_ArmMinLength;

public:
	static const class CWO_TentacleSystem_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       class CWO_TentacleSystem_ClientData& GetClientData(      CWObject_CoreData* _pObj);

public:
	CWObject_TentacleSystem();

	// CWObject overrides:
	virtual void OnCreate();								// Called immediately after object has been created and has recieved object-index and server-pointer. It is called EVERY time an object is created, even if it's from a savegame.
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);			// Called (after OnCreate) once for every key when spawning from an XW file for the first time.
	virtual void OnFinishEvalKeys();						// Called when all keys of the entity has been processed (which is after OnCreate()) with OnEvalKey.
	virtual aint OnMessage(const CWObject_Message& _Msg);	// Called at any time when object receives a message.
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	virtual int  OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;	// Called whenever a delta-update should be compiled.
	virtual void OnRefresh();								// Called once every game-tick, unless CWO_CLIENTFLAGS_NOREFRESH is specified.

	aint OnPrecacheMessage(const CWObject_Message& _Msg);

	static void OnIncludeClass(CMapData* _pWData, CWorld_Server*);							// Called when resource-manager has received notification that the class will be used, and thus gives the class opportunity to include other classes.
	static void OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);

	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);				// Called once every game-tick if the object is potentially visible, unless CWO_CLIENTFLAGS_NOREFRESH is specified
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine*, const CMat4Dfp32& _ParentMat);	// Called zero, one or several times for every frame rendered. Should not modify the state of the object in ways that make assumptions of the call-rate. (i.e, it's perfectly legal for the client to call the object 20 times for one rendered frame)
	static int  OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);

	static void OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);
	static void OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);

	// Gameplay functionality:
	CWObject* SelectTarget(uint* _pSelectionMask = NULL);
	void GrabObject(CWObject& _Object, fp32 _GrabPower, uint _nDamage = 0);
	void PushObject(CWObject& _Object, fp32 _GrabPower, uint _nDamage = 0);
	void GrabCharacterObject(CWObject& _Object, fp32 _GrabPower);
	bool GrabAndDevour(CWObject& _Object, bool _bExtractDarkling);
	void GetNothing(const CVec3Dfp32& _GotoPos);
	void BreakObject(CWObject& _Object, uint _nDamage = 100);
	bool StartCreepingDark(const CVec3Dfp32& _StartPos);
	void StopCreepingDark();
	bool IsIdle() const;
	bool IsDevouring() const;
	bool IsCreepingDark() const;
	bool ReleaseObject(const CVec3Dfp32* _pControlMove = NULL);
	void UpdateArmControl(const CVec3Dfp32& _ControlMove);
	void ActivateDemonHeads(bool _bActivate);
	bool GetMainTentaclePos(CVec3Dfp32* _pPos) const;
	void UpdateInteresting();
	fp32 GetArmMaxLength();

	void SendDecalMsg(const CVec3Dfp32& _Dir, const CVec3Dfp32& _Pos, int32 _iTarget);

	void DoHurtResponse();

	static CMat4Dfp32 GetCreepCam(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CMat4Dfp32 &_WMat);
};






#endif // __WObj_TentacleSystem_h__
