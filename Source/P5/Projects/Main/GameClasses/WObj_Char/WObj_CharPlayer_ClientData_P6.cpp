#include "PCH.h"
#include "WObj_CharPlayer_ClientData_P6.h"
#include "WObj_CharPlayer_P6.h"

MRTC_IMPLEMENT(CWO_CharPlayer_ClientData_P6, CWO_CharPlayer_ClientData);

void CWO_CharPlayer_ClientData_P6::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	parent::Clear(_pObj, _pWPhysState);

	m_Money = 0;
	m_iLootTarget = -1;
	m_bDoOnce = true;
}

void CWO_CharPlayer_ClientData_P6::Copy(const CWO_CharPlayer_ClientData_P6& _CD)
{
	parent::Copy(_CD);

	m_Money = _CD.m_Money;
	m_iLootTarget = _CD.m_iLootTarget;
	m_lInventory = _CD.m_lInventory;
	m_bDoOnce = _CD.m_bDoOnce;
}

void CWO_CharPlayer_ClientData_P6::InventorySwitch(int _x, int _y, int _x2, int _y2)
{
	SInventoryItem *pItem1 = NULL;
	SInventoryItem *pItem2 = NULL;
	int i1 = -1;
	int i2 = -1;

	for(int i = 0; i < m_lInventory.Len(); i++)
	{
		if(_x == m_lInventory[i].x && _y == m_lInventory[i].y)
		{
			pItem1 = &m_lInventory[i];
			i1 = i;
		}
		if(_x2 == m_lInventory[i].x && _y2 == m_lInventory[i].y)
		{
			pItem2 = &m_lInventory[i];
			i2 = i;
		}
	}

	if(pItem1 && pItem2)
	{	//switch
		if(pItem1->item.m_LootType == INV_WEAPON && pItem2->item.m_LootType == INV_WEAPON_MOD)
		{
			m_iWeaponIndex = i1;
			ConExecute("cg_submenu(\"p6_weapon_mod\")");
			return;
		}
		if(pItem2->item.m_LootType == INV_WEAPON && pItem1->item.m_LootType == INV_WEAPON_MOD)
		{
			m_iWeaponIndex = i2;
			ConExecute("cg_submenu(\"p6_weapon_mod\")");
			return;
		}
		int ox = pItem1->x;
		int oy = pItem1->y;
		pItem1->x = pItem2->x;
		pItem1->y = pItem2->y;
		pItem2->x = ox;
		pItem2->y = oy;

		if(m_pWPhysState->IsClient())
		{	//We need to tell the server
			ConExecute(CStrF("inv_switch2(%i, %i, %i)", m_pObj->m_iObject, i1, i2));
		}
	}
	else if(pItem1 || pItem2)
	{
		SInventoryItem *pItem = NULL;
		pItem = pItem1 ? pItem1 : pItem2;
		if(pItem->x == _x && pItem->y == _y)
		{
			pItem->x = _x2;
			pItem->y = _y2;
			if(m_pWPhysState->IsClient())
			{
				if(pItem1)
					ConExecute(CStrF("inv_switch(%i, %i, %i, %i)", m_pObj->m_iObject, i1, _x2, _y2));
				else
					ConExecute(CStrF("inv_switch(%i, %i, %i, %i)", m_pObj->m_iObject, i2, _x2, _y2));
			}
		}
		else
		{
			pItem->x = _x;
			pItem->y = _y;
			if(m_pWPhysState->IsClient())
			{
				if(pItem1)
					ConExecute(CStrF("inv_switch(%i, %i, %i, %i)", m_pObj->m_iObject, i1, _x, _y));
				else
					ConExecute(CStrF("inv_switch(%i, %i, %i, %i)", m_pObj->m_iObject, i2, _x, _y));
			}
		}
	}
}

bool CWO_CharPlayer_ClientData_P6::InventoryAddItem(CStr _name)
{
	SLootItem Item;
	Item.m_LootObject = _name.GetStrSep(":");	//Real name
	Item.m_LootTemplate = _name.GetStrSep(":");	//template name
	Item.m_Info = _name.GetStrSep(":");
	Item.m_GUI_Icon = _name.GetStrSep(":");
	Item.m_LootType = _name.GetStrSep(":").Val_int();	//obj type
	Item.m_SubType = _name.Val_int();

	return InventoryAddItem(Item);
}

bool CWO_CharPlayer_ClientData_P6::InventoryAddItem(SLootItem _item)
{
	int x = 1;
	int y = 0;
	for(int ix = 1; ix < 4; ix++)
	{
		for(int iy = 0; iy < 8; iy++)	
		{
			bool bEmpty = true;
			int i = 0;
			for(; i <m_lInventory.Len(); i++)
			{
				if(m_lInventory[i].x == ix && m_lInventory[i].y == iy && m_lInventory[i].item.m_GUI_Icon.Len())
					bEmpty = false;
			}
			if(bEmpty)
			{
				x = ix;
				y = iy;
				goto break_out;
			}
		}
	}
break_out:

	SInventoryItem Item;
	Item.item = _item;

	if(Item.item.m_LootType == INV_WEAPON)
	{
		Item.inv[1][0].m_LootType = INV_WEAPON_MOD;
		Item.inv[1][0].m_LootObject = "Weapon mod";
		Item.inv[1][0].m_LootTemplate = "Weapon mod";
		Item.inv[1][0].m_GUI_Icon = "P6_inv_stock";
		Item.inv[1][0].m_Info = "Weapon stock";
		Item.inv[1][0].m_SubType = 1;

		Item.inv[2][0].m_LootType = INV_WEAPON_MOD;
		Item.inv[2][0].m_LootObject = "Weapon mod";
		Item.inv[2][0].m_LootTemplate = "Weapon mod";
		Item.inv[2][0].m_GUI_Icon = "P6_inv_barrel02";
		Item.inv[2][0].m_Info = "Old Rusty Weapon barrel";
		Item.inv[2][0].m_SubType = 2;

		Item.inv[0][1].m_LootType = INV_WEAPON_MOD;
		Item.inv[0][1].m_LootObject = "Weapon mod";
		Item.inv[0][1].m_LootTemplate = "Weapon mod";
		Item.inv[0][1].m_GUI_Icon = "P6_inv_hand01";
		Item.inv[0][1].m_Info = "Weapon handle";
		Item.inv[0][1].m_SubType = 3;

		Item.inv[2][1].m_LootType = INV_WEAPON_MOD;
		Item.inv[2][1].m_LootObject = "Weapon mod";
		Item.inv[2][1].m_LootTemplate = "Weapon mod";
		Item.inv[2][1].m_GUI_Icon = "P6_inv_hand02";
		Item.inv[2][1].m_Info = "Weapon handle";
		Item.inv[2][1].m_SubType = 5;

		Item.inv[0][0].m_LootType = INV_WEAPON_MOD;
		Item.inv[0][0].m_LootObject = "Weapon mod";
		Item.inv[0][0].m_LootTemplate = "Weapon mod";
		Item.inv[0][0].m_GUI_Icon = "P6_inv_kolv";
		Item.inv[0][0].m_Info = "Weapon kolv";
		Item.inv[0][0].m_SubType = 0;

		Item.inv[1][1].m_LootType = INV_WEAPON_MOD;
		Item.inv[1][1].m_LootObject = "Weapon mod";
		Item.inv[1][1].m_LootTemplate = "Weapon mod";
		Item.inv[1][1].m_GUI_Icon = "P6_inv_mag";
		Item.inv[1][1].m_Info = "Weapon mag";
		Item.inv[1][1].m_SubType = 4;
	}

	Item.x = x;
	Item.y = y;
	m_lInventory.Add(Item);

	return true;
}

void CWO_CharPlayer_ClientData_P6::WeaponMod(int _ex, int _ey, int _ix, int _iy)
{
	int iModToEquip = -1;

	for(int i = 0; i < m_lInventory.Len(); i++)
	{
		if(_ix == m_lInventory[i].x && _iy == m_lInventory[i].y)
			iModToEquip = i;
	}

	if(_ey == 0)
	{
		if(_ex == 0)
		{
			if(m_lInventory[iModToEquip].item.m_SubType != 0)
			{
				return;
			}
		}
		else if(_ex == 1)
		{
			if(m_lInventory[iModToEquip].item.m_SubType != 1)
			{
				return;
			}
		}
		else if(_ex == 2)
		{
			if(m_lInventory[iModToEquip].item.m_SubType != 2)
			{
				return;
			}
		}
	}
	else
	{
		if(_ex == 0)
		{
			if(m_lInventory[iModToEquip].item.m_SubType != 3)
			{
				return;
			}
		}
		else if(_ex == 1)
		{
			if(m_lInventory[iModToEquip].item.m_SubType != 4)
			{
				return;
			}
		}
		else if(_ex == 2)
		{
			if(m_lInventory[iModToEquip].item.m_SubType != 5)
			{
				return;
			}
		}
	}

	UnequipMod(_ex, _ey);

	SInventoryItem check = m_lInventory[m_iWeaponIndex];

	if(m_lInventory[m_iWeaponIndex].item.m_LootTemplate == "weapon_ak47_mod" || m_lInventory[m_iWeaponIndex].item.m_LootTemplate == "weapon_ak47")
	{
		if(m_lInventory[iModToEquip].item.m_GUI_Icon == "P6_inv_barrel")
			m_lInventory[m_iWeaponIndex].item.m_LootTemplate = "weapon_ak47_mod";
		else
			m_lInventory[m_iWeaponIndex].item.m_LootTemplate = "weapon_ak47";
	}

	m_lInventory[m_iWeaponIndex].inv[_ex][_ey] = m_lInventory[iModToEquip].item;
	m_lInventory[iModToEquip].item.m_GUI_Icon = "";
	m_lInventory[iModToEquip].item.m_Info = "";
	m_lInventory[iModToEquip].item.m_LootObject = "";
	m_lInventory[iModToEquip].item.m_LootTemplate = "";
	m_lInventory[iModToEquip].item.m_LootType = -1;
	m_lInventory[iModToEquip].item.m_SubType = -1;
	m_lInventory[iModToEquip].x = -1;
	m_lInventory[iModToEquip].y = -1;

	if(m_pWPhysState->IsClient())
		ConExecute(CStrF("inv_mod(%i, %i, %i, %i, %i)", m_pObj->m_iObject, (int32)m_iWeaponIndex, iModToEquip, _ex, _ey));
}

void CWO_CharPlayer_ClientData_P6::InventoryEquip(int _ex, int _ey, int _ix, int _iy)
{
	int iEquippedWeapon = -1;
	int iItemToEquip = -1;

	for(int i = 0; i < m_lInventory.Len(); i++)
	{
		if(_ex == m_lInventory[i].x && _ey == m_lInventory[i].y)
			iEquippedWeapon = i;
		if(_ix == m_lInventory[i].x && _iy == m_lInventory[i].y)
			iItemToEquip = i;
	}

	UnequipItem(iEquippedWeapon, _ix, _iy); 

	if(iItemToEquip == -1)
		return;

	switch(m_lInventory[iItemToEquip].item.m_LootType) 
	{
	case INV_ARMOR:
		{
			if(_ex == 0 && (_ey == 2 || _ex == 3))
			{
				m_lInventory[iItemToEquip].x = _ex;
				m_lInventory[iItemToEquip].y = _ey;
			}
		}
		break;
	case INV_WEAPON:
		{
			if((_ex == 0) && (_ey == 4 || _ey == 5))
			{
				m_lInventory[iItemToEquip].x = _ex;
				m_lInventory[iItemToEquip].y = _ey;
				//Equip it
				ConExecute(CStrF("equip_weapon(%i, %i)", iItemToEquip, m_pObj->m_iObject));
			}
		}
		break;
	}
	if(m_pWPhysState->IsClient())
		ConExecute(CStrF("inv_switch(%i, %i, %i, %i)", m_pObj->m_iObject, iItemToEquip, _ex, _ey));
}


void CWO_CharPlayer_ClientData_P6::UnequipItem(int _i, int _x, int _y)
{
	if(_i == -1)
		return;

	//is desired position empty?
	bool bEmpty = true;
	int i = 0;
	for(; i < m_lInventory.Len(); i++)
	{
		if(m_lInventory[i].x == _x && m_lInventory[i].y == _y)
			bEmpty = false;
	}
	if(bEmpty)
	{
		m_lInventory[_i].x = _x;
		m_lInventory[_i].y = _y;
		switch(m_lInventory[_i].item.m_LootType) 
		{
		case INV_WEAPON:
			{	//remove weapon
				ConExecute(CStrF("unequip_weapon(%i)", m_pObj->m_iObject));
				if(m_pWPhysState->IsClient())
					ConExecute(CStrF("inv_switch(%i, %i, %i, %i)", m_pObj->m_iObject, _i, _x, _y));
			}
			break;
		}
		return;
	}

	//First spot wasn't empty, bruteforce it!
	for(int x = 1; x < 4; x++)
	{
		for(int y = 0; y < 8; y++)	
		{
			bool bEmpty = true;
			int i = 0;
			for(; i < m_lInventory.Len(); i++)
			{
				if(m_lInventory[i].x == x && m_lInventory[i].y == y)
					bEmpty = false;
			}
			if(bEmpty)
			{
				m_lInventory[_i].x = x;
				m_lInventory[_i].y = y;
				switch(m_lInventory[_i].item.m_LootType) 
				{
				case INV_WEAPON:
					{	//remove weapon
						ConExecute(CStrF("unequip_weapon(%i)", m_pObj->m_iObject));
						if(m_pWPhysState->IsClient())
							ConExecute(CStrF("inv_switch(%i, %i, %i, %i)", m_pObj->m_iObject, _i, x, y));
					}
					break;
				}
				return;
			}
		}
	}
}

void CWO_CharPlayer_ClientData_P6::UnequipMod(int _x, int _y)
{
	if(!m_lInventory[m_iWeaponIndex].inv[_x][_y].m_GUI_Icon.Len())
		return;	//empty

	for(int x = 1; x < 4; x++)
	{
		for(int y = 0; y < 8; y++)	
		{
			bool bEmpty = true;
			int i = 0;
			for(; i < m_lInventory.Len(); i++)
			{
				if(m_lInventory[i].x == x && m_lInventory[i].y == y)
					bEmpty = false;
			}
			if(bEmpty)
			{
				SInventoryItem item;
				item.item = m_lInventory[m_iWeaponIndex].inv[_x][_y];
				item.x = x;
				item.y = y;
				m_lInventory.Add(item);
				m_lInventory[m_iWeaponIndex].inv[_x][_y].m_LootObject = "";
				m_lInventory[m_iWeaponIndex].inv[_x][_y].m_LootTemplate = "";
				m_lInventory[m_iWeaponIndex].inv[_x][_y].m_Info = "";
				m_lInventory[m_iWeaponIndex].inv[_x][_y].m_GUI_Icon = "";
				m_lInventory[m_iWeaponIndex].inv[_x][_y].m_LootType = -1;
				m_lInventory[m_iWeaponIndex].inv[_x][_y].m_SubType = -1;

				if(m_pWPhysState->IsClient())
					ConExecute(CStrF("inv_unmod(%i, %i, %i, %i)", m_pObj->m_iObject, (int32)m_iWeaponIndex, _x, _y));

				return;
			}
		}
	}
}

