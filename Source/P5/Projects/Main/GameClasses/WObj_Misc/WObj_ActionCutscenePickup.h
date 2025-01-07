/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			
					
	Author:			Olle Rosenquist
					
	Copyright:		Copyright O3 Games AB 2002
					
	Contents:		
					
	Comments:		RPG object pickups with extra functionality
					
	History:		
		021018:		Created File
\*____________________________________________________________________________________________*/

#ifndef _INC_CWOBJ_PICKUP
#define _INC_CWOBJ_PICKUP

#include "WObj_ActionCutscene.h"
//#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#define ITEM_REPICKCOUNTDOWN 20
#define CWObject_ActionCutscenePickupParent CWObject_ActionCutscene
class CWObject_ActionCutscenePickup : public CWObject_ActionCutscenePickupParent
{
	enum
	{
		CLIENTFLAGS_PICKUPNOFLASH = (CWO_CLIENTFLAGS_USERBASE << 2),
	};
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	CFStr m_ItemTemplate;
	TPtr<class CRPG_Object_Item> m_spDummyObject;
	int m_iItemRepickCountdown;
	int m_nPhysIdleTicks : 8;
	//CWO_SimpleMessage m_Msg_OnPickup;

	virtual bool DoActionSuccess(int _iCharacter);
	virtual int32 DetermineACSType(const CVec3Dfp32& _Charpos, bool _bCrouch);
	virtual bool OnEndACS(int _iCharacter);
	virtual bool CanActivate(int32 _iChar);
public:
	virtual void OnCreate();
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnSpawnWorld();

	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	//virtual void TagAnimationsForPrecache(CWAGI_Context* _pContext, CWAGI* _pAGI, TArray<int32>& _liACS);
	virtual void TagAnimationsForPrecache(CWAG2I_Context* _pContext, CWAG2I* _pAGI, TArray<int32>& _liACS);
		
	bool SetItem(const char *_pName);
	bool SetItem(CRPG_Object_Item *_pObj);

	void OnDeltaSave(CCFile* _pFile);
	void OnDeltaLoad(CCFile* _pFile, int _Flags);

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};



#define CWObject_DroppableItemParent CWObject_ActionCutscenePickup
class CWObject_DroppableItem : public CWObject_DroppableItemParent
{
	TPtr<class CConstraintRigidObject>	m_spRagdoll;
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	virtual void OnCreate();
	virtual void OnRefresh();
	virtual void OnFinishEvalKeys();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	//virtual int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const;
};

#endif
