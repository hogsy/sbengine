#ifndef _INC_WOBJ_ITEM
#define _INC_WOBJ_ITEM

#include "WObj_RPG.h"
//#include "WObjects.h"
//#include "WRPG/WRPGItem.h"

#define RPG_ITEM_DEFAULTPICKUPLATENCY	int(0.5f*SERVER_TICKSPERSECOND)

class CWObject_RPG_Item : public CWObject_RPG
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	int16 m_RespawnTime;
	int16 m_RespawnCountdown;
	int16 m_PickupCountdown;

	CWObject_RPG_Item();

	virtual CStr RPG_GetMouseOverIdentification(int _Flags);

	virtual CRPG_Object_Item* Item_Get();
	virtual bool Item_CreateFromRPGObject(CRPG_Object_Item* _pItem, const CMat43fp32* _pPos = NULL);
	virtual bool Item_CreateFromRPGObject(CRPG_Object_Item* _pItem, const CVec3Dfp32& _Pos);

	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
	virtual void OnLoad(CCFile* _pFile, CMapData* _pWData, int _Flags);
	virtual void OnSave(CCFile* _pFile, CMapData* _pWData, int _Flags);

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine);

	void RecreateObject();
};

// -------------------------------------------------------------------
class CWObject_RPG_CTFItem : public CWObject_RPG_Item
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	int m_Resting;
	int m_Team;

public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
};


#endif
