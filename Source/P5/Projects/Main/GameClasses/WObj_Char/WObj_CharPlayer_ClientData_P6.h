#ifndef __WObj_CharPlayer_ClientData_P6_h__
#define __WObj_CharPlayer_ClientData_P6_h__

#include "WObj_CharPlayer_ClientData.h"
#include "WObj_CharNPC_ClientData_P6.h"

struct SInventoryItem
{
	int32 x;
	int32 y;
	SLootItem item;
	SLootItem inv[3][2];	//Weapons have their own inventory
};

enum
{
	INV_ARMOR,
	INV_WEAPON,
	INV_QUEST,
	INV_WEAPON_MOD,
	INV_HEALTH,
};

class CWO_CharPlayer_ClientData_P6 : public CWO_CharPlayer_ClientData
{
	typedef CWO_CharPlayer_ClientData parent;
	MRTC_DECLARE;
	AUTOVAR_SETCLASS(CWO_CharPlayer_ClientData_P6, parent);

public:
	CAUTOVAR_OP(CAutoVar_int32, m_Money, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_int32, m_iLootTarget, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_int32, m_iWeaponIndex, DIRTYMASK_0_3);	//index to m_lInventory, weapon being modded
	//CAUTOVAR(TArray<SInventoryItem>, m_lInventory, DIRTYMASK_0_4);
		
	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_Money)
	AUTOVAR_PACK_VAR(m_iLootTarget)
	AUTOVAR_PACK_VAR(m_iWeaponIndex)
	AUTOVAR_PACK_END

	TArray<SInventoryItem> m_lInventory;
	bool	m_bDoOnce;

public:
	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	virtual void Copy(const CWO_CharPlayer_ClientData_P6& _CD);

	virtual void InventorySwitch(int _x, int _y, int _x2, int _y2);
	virtual bool InventoryAddItem(CStr _name);
	virtual bool InventoryAddItem(SLootItem _item);
	virtual void InventoryEquip(int _ex, int _ey, int _ix, int _iy);
	virtual void WeaponMod(int _ex, int _ey, int _ix, int _iy);

	void UnequipMod(int _x, int _y);
	void UnequipItem(int _i, int _x, int _y);
};



#endif // __WObj_CharPlayer_ClientData_h__
