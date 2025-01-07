#include "PCH.h"
#include "WObj_CharNPC_P6.h"
#include "../wrpg/WRPGItem.h"
#include "../wrpg/WRPGChar.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_CharNPC_P6, CWObject_CharNPC, 0x0100);


const CWO_CharNPC_ClientData_P6& CWObject_CharNPC_P6::GetClientData(const CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWObject_CharNPC] Bad this-pointer!");
	const CWO_CharNPC_ClientData_P6* pCD = safe_cast<const CWO_CharNPC_ClientData_P6>((const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharNPC] No clientdata?!");
	return *pCD;
};

CWO_CharNPC_ClientData_P6& CWObject_CharNPC_P6::GetClientData(CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWObject_CharNPC] Bad this-pointer!");
	CWO_CharNPC_ClientData_P6* pCD = safe_cast<CWO_CharNPC_ClientData_P6>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharNPC] No clientdata?!");
	return *pCD;
};

void CWObject_CharNPC_P6::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	if (!TDynamicCast<CWO_CharNPC_ClientData_P6>(pData))
	{
		CWO_CharNPC_ClientData_P6* pCD = MNew(CWO_CharNPC_ClientData_P6);
		if (!pCD)
			Error_static("CWObject_CharNPC_P6", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}

	if (!InitClientObjects(_pObj, _pWPhysState))
		Error_static("CWObject_CharNPC_P6", "InitClientObjects failed");
}

void CWObject_CharNPC_P6::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	CWO_CharNPC_ClientData_P6 &CD = GetClientData(this);

	switch (_KeyHash)
	{
	case MHASH2('MONE','Y'):
		{
			CStr MaxMoney = KeyValue;
			CStr MinMoney = MaxMoney.GetStrSep(":");
			int minm = MinMoney.Val_int();
			int maxm = MaxMoney.Val_int();
			CD.m_Money = minm + (MRTC_RAND() % (maxm - minm + 1));
			break;
		}
	case MHASH3('LOOT','_OBJ','ECT'):
	case MHASH3('LOOT','_OBJ','ECT0'):
	case MHASH3('LOOT','_OBJ','ECT1'):
	case MHASH3('LOOT','_OBJ','ECT2'):
	case MHASH3('LOOT','_OBJ','ECT3'):
	case MHASH3('LOOT','_OBJ','ECT4'):
	case MHASH3('LOOT','_OBJ','ECT5'):
	case MHASH3('LOOT','_OBJ','ECT6'):
	case MHASH3('LOOT','_OBJ','ECT7'):
	case MHASH3('LOOT','_OBJ','ECT8'):
	case MHASH3('LOOT','_OBJ','ECT9'):
		{
			CStr LootObject = KeyValue;
			SLootItem Item;
			Item.m_LootObject = LootObject.GetStrSep(":");
			Item.m_LootTemplate = LootObject.GetStrSep(":");
			Item.m_Info = LootObject.GetStrSep(":");
			Item.m_GUI_Icon = LootObject.GetStrSep(":");
			Item.m_LootType = LootObject.GetStrSep(":").Val_int();
			Item.m_SubType = LootObject.Val_int();
			CD.m_lLoot.Add(Item);
			break;
		}
	default:
		{
			CWObject_CharNPC::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_CharNPC_P6::OnCreate()
{
	CWObject_CharNPC::OnCreate();
	m_bDoOnce = true;
}

void CWObject_CharNPC_P6::OnRefresh()
{
	CWObject_CharNPC::OnRefresh();

	if(m_bDoOnce)
	{
		CRPG_Object_Item *pItem = Char()->GetEquippedItem(0);
		if(pItem)
			pItem->m_Flags |= RPG_ITEM_FLAGS_NOPICKUP;
		m_bDoOnce = false;
	}

	CWO_CharNPC_ClientData_P6 &CD = GetClientData(this);
	if(Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
	{
		bool bLootLeft = false;
		for(int i = 0; i < CD.m_lLoot.Len(); i++)
		{
			if(CD.m_lLoot[i].m_LootType != -1)	//-1 means it's been looted
			{
				bLootLeft = true;
			}
		}
		if(!bLootLeft)
		{
			m_pWServer->Object_Destroy(m_iObject);
		}
	}
}







