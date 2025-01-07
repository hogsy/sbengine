#include "PCH.h"
#include "WClient_P6.h"

MRTC_IMPLEMENT_DYNAMIC(CWClient_P6, CWClient_Mod);

CRegistry* CWClient_P6::GUI_GetWindowsResource()
{
	MAUTOSTRIP(CWClient_Mod_GUI_GetWindowsResource, NULL);
	//CFStr RcName = CFStrF("GUI\\ModInv%d", m_ClientPlayerNr);
	CFStr RcName = CFStrF("GUI\\P6_CubeInv%d", m_ClientPlayerNr);
	int iRc = m_spGUIData->GetResourceIndex_Registry(RcName);
	CRegistry* pRc = m_spGUIData->GetResource_Registry(iRc);
	return pRc;
}

void CWClient_P6::Con_EquipWeapon(int32 _iInv, int32 _iObj)
{
	CStr cmd = CStrF("%i:%i", _iInv, _iObj);
	Con_StringCmd(CONTROL_EQUIP_WEAPON, cmd);
}

void CWClient_P6::Con_UnEquipWeapon(int32 _iObj)
{
	CStr cmd = CStrF("%i", _iObj);
	Con_StringCmd(CONTROL_UNEQUIP_WEAPON, cmd);
}

void CWClient_P6::Con_InvForceUpdate(int32 _iObj)
{
	CStr cmd = CStrF("%i", _iObj);
	Con_StringCmd(CONTROL_INV_FORCE_UPDATE, cmd);
}

void CWClient_P6::Con_InvUnMod(int32 _iObj, int32 _iWeapon, int32 _x, int32 _y)
{
	CStr cmd = CStrF("%i:%i:%i:%i", _iObj, _iWeapon, _x, _y);
	Con_StringCmd(CONTROL_INV_UNMOD, cmd);
}

void CWClient_P6::Con_InvMod(int32 _iObj, int32 _iWeapon, int32 _iMod, int32 _ex, int32 _ey)
{
	CStr cmd = CStrF("%i:%i:%i:%i:%i", _iObj, _iWeapon, _iMod, _ex, _ey);
	Con_StringCmd(CONTROL_INV_MOD, cmd);
}

void CWClient_P6::Con_InvSwitch(int32 _iObj, int32 _iItem, int32 _x, int32 _y)
{
	CStr cmd = CStrF("%i:%i:%i:%i", _iObj, _iItem, _x, _y);
	Con_StringCmd(CONTROL_INV_SWITCH, cmd);
}

void CWClient_P6::Con_InvSwitch2(int32 _iObj, int32 _iItem1, int32 _iItem2)
{
	CStr cmd = CStrF("%i:%i:%i", _iObj, _iItem1, _iItem2);
	Con_StringCmd(CONTROL_INV_SWITCH2, cmd);
}

void CWClient_P6::Con_LootMoney(int32 _iObj)
{
	CStr cmd = CStrF("%i:%i", _iObj);
	Con_StringCmd(CONTROL_LOOT_MONEY, cmd);
}

void CWClient_P6::Con_LootObject(int32 _iObj, int32 _iLootIndex)
{
	CStr cmd = CStrF("%i:%i", _iObj, _iLootIndex);
	Con_StringCmd(CONTROL_LOOT_OBJECT, cmd);
}

void CWClient_P6::Con_LootExit(int32 _iObj)
{
	CStr cmd = CStrF("%i", _iObj);
	Con_StringCmd(CONTROL_LOOT_EXIT, cmd);
}

void CWClient_P6::Register(CScriptRegisterContext & _RegContext)
{
	CWClient_Mod::Register(_RegContext);

	_RegContext.RegFunction("loot_exit", this, &CWClient_P6::Con_LootExit);
	_RegContext.RegFunction("loot_money", this, &CWClient_P6::Con_LootMoney);
	_RegContext.RegFunction("loot_object", this, &CWClient_P6::Con_LootObject);

	_RegContext.RegFunction("inv_forceupdate", this, &CWClient_P6::Con_InvForceUpdate);
	_RegContext.RegFunction("inv_mod", this, &CWClient_P6::Con_InvMod);
	_RegContext.RegFunction("inv_unmod", this, &CWClient_P6::Con_InvUnMod);
	_RegContext.RegFunction("inv_switch", this, &CWClient_P6::Con_InvSwitch);
	_RegContext.RegFunction("inv_switch2", this, &CWClient_P6::Con_InvSwitch2);
	_RegContext.RegFunction("equip_weapon", this, &CWClient_P6::Con_EquipWeapon);
	_RegContext.RegFunction("unequip_weapon", this, &CWClient_P6::Con_UnEquipWeapon);
}

