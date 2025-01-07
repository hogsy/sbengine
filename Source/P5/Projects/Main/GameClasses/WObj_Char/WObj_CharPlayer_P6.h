#ifndef __WObj_CharPlayer_P6_h__
#define __WObj_CharPlayer_P6_h__

#include "WObj_CharPlayer.h"
#include "WObj_CharPlayer_ClientData_P6.h"

enum
{
	P6_OBJMSG_CHAR_START_TRADING = 0x1066,
	P6_OBJMSG_CHAR_REWARD = 0x1067,

	P6_OBJMSG_CHAR_EQUIP_WEAPON = OBJMSG_CHAR_GRABBED_BY_DEMONARM + 1,
	P6_OBJMSG_CHAR_UNEQUIP_WEAPON,
};

class CWObject_CharPlayer_P6 : public CWObject_CharPlayer
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_CharPlayer parent;
public:
	// Client data - server only interface
	const CWO_CharPlayer_ClientData_P6& GetClientData() const { return GetClientData(this); }
	CWO_CharPlayer_ClientData_P6& GetClientData()       { return GetClientData(this); }
	// Client data - client/server interface
	static const CWO_CharPlayer_ClientData_P6& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_CharPlayer_ClientData_P6& GetClientData(CWObject_CoreData* _pObj);

	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);

	static void OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
//	static void OnClientRenderHealthHud(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientRenderChoices(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);

	virtual void Char_CheckDarknessActivation(int _ControlPress, int _ControlLastPress, const CVec3Dfp32& _ControlMove);
	virtual void OnSpawnWorld();
	virtual void OnCreate();
	virtual void OnPress();
	virtual void OnRefresh();

	virtual aint OnMessage(const CWObject_Message& _Msg);

	virtual void Char_RefreshDarkness();

	virtual bool InventoryIsArmorEquipped(void);
};

#endif

