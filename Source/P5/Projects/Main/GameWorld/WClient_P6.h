#ifndef __WCLIENT_P6_H__
#define __WCLIENT_P6_H__

#include "WClientMod.h"

enum
{
	CONTROL_EQUIP_WEAPON = CONTROL_SELECTITEM + 2,
	CONTROL_UNEQUIP_WEAPON,
	CONTROL_INV_SWITCH,
	CONTROL_INV_SWITCH2,
	CONTROL_INV_MOD,
	CONTROL_INV_UNMOD,
	CONTROL_INV_FORCE_UPDATE,
	CONTROL_LOOT_MONEY,
	CONTROL_LOOT_OBJECT,
	CONTROL_LOOT_EXIT,
};

class CWClient_P6 : public CWClient_Mod
{
	MRTC_DECLARE;
private:
public:
	virtual CRegistry* GUI_GetWindowsResource();
	void Con_EquipWeapon(int32 _iInv, int32 _iObj);
	void Con_UnEquipWeapon(int32 _iObj);

	void Con_InvMod(int32 _iObj, int32 _iWeapon, int32 _iMod, int32 _ex, int32 _ey);
	void Con_InvUnMod(int32 _iObj, int32 _iWeapon, int32 _x, int32 _y);
	void Con_InvSwitch(int32 _iObj, int32 _iItem, int32 _x, int32 _y);
	void Con_InvSwitch2(int32 _iObj, int32 _iItem1, int32 _iItem2);
	void Con_InvForceUpdate(int32 _iObj);

	void Con_LootMoney(int32 _iObj);
	void Con_LootObject(int32 _iObj, int32 _iLootIndex);
	void Con_LootExit(int32 _iObj);

	void Register(CScriptRegisterContext &_RegContext);
};

#endif

