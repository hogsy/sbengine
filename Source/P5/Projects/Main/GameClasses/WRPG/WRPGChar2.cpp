
#include "PCH.h"

#include "WRPGChar2.h"
#include "WRPGAmmo.h"
#include "../WObj_Char.h"

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Char2, CRPG_Object);

// -------------------------------------------------------------------
//  CRPG_Object_Char2

void CRPG_Object_Char2::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_Char2_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object::OnCreate();

	Wait() = 5;
	Pain() = 0;

	SetNumChildren(RPG_CHAR2_NUMCHILDREN);

	Child(RPG_CHAR2_INVENTORY) = CreateObject("Inventory2");
}

void CRPG_Object_Char2::OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_Char2_OnIncludeClass, MAUTOSTRIP_VOID);
	//if(m_bUseDefaultItems)
	{
		for(int i = 0; i < _pReg->GetNumChildren(); i++)
		{
			if(_pReg->GetName(i).Copy(0, 7) == "WEAPON_")
				IncludeRPGClass(_pReg->GetName(i), _pMapData, _pWServer);
			else if(_pReg->GetName(i).Copy(0, 6) == "SPELL_")
				IncludeRPGClass(_pReg->GetName(i), _pMapData, _pWServer);
			else if(_pReg->GetName(i).Copy(0, 5) == "ITEM_")
				IncludeRPGClass(_pReg->GetName(i), _pMapData, _pWServer);
			else if(_pReg->GetName(i).Copy(0, 5) == "ARMOUR_")
				IncludeRPGClass(_pReg->GetName(i), _pMapData, _pWServer);
			else if(_pReg->GetName(i).Copy(0, 4) == "ITEM")
				IncludeRPGClass(_pReg->GetValue(i), _pMapData, _pWServer);
		}
	}

	IncludeRPGClass("use_generic", _pMapData, _pWServer);
}

bool CRPG_Object_Char2::PreProcess(CRPG_Object *_pRoot, int)
{
	MAUTOSTRIP(CRPG_Object_Char2_PreProcess, false);
	return false;
}

bool CRPG_Object_Char2::OnProcess(CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Char2_OnProcess, false);
	CRPG_Object::OnProcess(_pRoot, _iObject);

	GetInventory()->OnProcess(this, _iObject);

	if (Wait() > 0)
		Wait() = Wait() - 1;

	if (Wait() == 0)
		GetInventory()->PendingUpdate(this, _iObject);

	return true;
}

bool CRPG_Object_Char2::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_Char2_OnEvalKey, false);

 const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();


	if(KeyName.CompareSubStr("WEAPON_") == 0 || KeyName.Find("SPELL_") == 0 ||
		KeyName.CompareSubStr("ITEM_") == 0 || KeyName.Find("ARMOUR_") == 0)
	{
		PickupItem(KeyName, true, false, NULL, 0, true);
	}
	else if(KeyName.CompareSubStr("ITEM") == 0)
	{
		PickupItem(KeyValue, true, false, NULL, 0, true);
	}

	case MHASH4('EQUI','PPED','_ITE','M'): // "EQUIPPED_ITEM"
		{
			EquipItem(KeyValue);
			break;
		}

	default:
		{
			return CRPG_Object::OnEvalKey(_pKey);
			break;
		}
	}

	return true;
}

CRPG_Object_Inventory2 *CRPG_Object_Char2::GetInventory()
{
	MAUTOSTRIP(CRPG_Object_Char2_GetInventory, NULL);
	if (!GetNumChildren())
		return NULL;

	return (CRPG_Object_Inventory2*)GetChild(0);
}

void CRPG_Object_Char2::ApplyWait(int _Wait)
{
	MAUTOSTRIP(CRPG_Object_Char2_ApplyWait, MAUTOSTRIP_VOID);
	if (_Wait == -1)
		Wait() = 10000; // Lock
	else if (_Wait == 0)
		Wait() = 0; // Unlock
	else if( _Wait < -1)
		Wait() = _Wait; // Set
	else
		Wait() = Wait() + _Wait; // Increase
}

void CRPG_Object_Char2::OnCharDeath(int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Char2_OnCharDeath, MAUTOSTRIP_VOID);
	GetInventory()->OnCharDeath(_iObject, this);
}

bool CRPG_Object_Char2::AddItem(int _iObject, CRPG_Object *_pObj)
{
	MAUTOSTRIP(CRPG_Object_Char2_AddItem, false);
	CRPG_Object_Item2* pItem = safe_cast<CRPG_Object_Item2>(_pObj);
	if (pItem == NULL)
		return false;

	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return false;

	return pInventory->AddItem(_iObject, pItem);
}

bool CRPG_Object_Char2::RemoveItemByType(int _iObject, int _iItemType, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Char2_RemoveItemByType, false);
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return false;

	return pInventory->RemoveItemByType(_iObject, _iItemType, _bRemoveInstant);
}

bool CRPG_Object_Char2::RemoveItemByName(int _iObject, const char* _pName, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Char2_RemoveItemByName, false);
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return false;

	return pInventory->RemoveItemByName(_iObject, _pName, _bRemoveInstant);
}

CRPG_Object_Item2 *CRPG_Object_Char2::PickupItem(const char* _pName, bool _bAllowMerge, bool _bSelect, TPtr<CRPG_Object_Item2>* _pspDropping, int _iObject, bool _bNoSound, int _iSender)
{
	MAUTOSTRIP(CRPG_Object_Char2_PickupItem, NULL);
	spCRPG_Object spObj = CreateObject(_pName);
	if (spObj == NULL)
		return NULL;

#ifndef M_RTM
	if(TDynamicCast<CRPG_Object_Item2 >((CRPG_Object *)spObj) == NULL)
		return NULL;
#endif

	TPtr<CRPG_Object_Item2> spItem = (CRPG_Object_Item2 *)((CRPG_Object *)spObj);
	if (spItem == NULL)
		return NULL;

	int Res = spItem->OnPickup(_iObject, this, _bNoSound, _iSender);
	if(Res == 0)
		return NULL;
	else if(Res == 2)
		return (CRPG_Object_Item2 *)0x1;

	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return NULL;

	CRPG_Object_Item2* pOldItem = FindItemByType(spItem->m_iItemType);

	if (pOldItem == NULL)
	{
		if (spItem->m_Flags & RPG_ITEM_FLAGS_MERGEONLY)
		{
			//This item can only be merged with old items, never picked up by itself
			return NULL;
		}
		else
		{
			//New unique item
			pInventory->AddItem(_iObject, spItem);
		}
	}
	else
	{
		bool bMerged = false;

		if (_bAllowMerge && !(spItem->m_Flags & RPG_ITEM_FLAGS_NOMERGE))
		{
			//Merge!
			bMerged = pOldItem->MergeItem(_iObject, (CRPG_Object_Item2*)spItem);
			spItem = pOldItem;
		}
		else
		{
			//We can't/won't merge so therefore we might have picked up a new weapon
			if (spItem->m_Flags & RPG_ITEM_FLAGS_MERGEONLY)
			{
				//This item can only be merged with old items, never picked up by itself
				return NULL;
			}

			//Update item model next tick
			int iTick = SendMsg(_iObject, OBJMSG_CHAR_GETGAMETICK);
			if(!iTick)
				iTick = m_pWServer->GetGameTick();
			SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, spItem->m_iItemType >> 8, iTick + 1);
		}

		if ((_pspDropping != NULL) && (!_bAllowMerge || !bMerged))
		{
			*_pspDropping = pOldItem;
			pInventory->ReplaceItemByType(_iObject, spItem);
		}

	}

	if (_bSelect)
		SelectItem(spItem->m_iItemType, true, true);

	return spItem;
}

CRPG_Object_Item2* CRPG_Object_Char2::EquipItem(const char* _pName)
{
	CRPG_Object_Item2* pItem = FindItemByName(_pName);
	if (pItem)
		SelectItem(pItem->m_iItemType, true, true);
	return pItem;
}

bool CRPG_Object_Char2::IsSelected(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char2_IsSelected, false);
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return false;

	return (pInventory->GetSelectedItemType() == _iItemType);
}

bool CRPG_Object_Char2::IsEquipped(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char2_IsEquipped, false);
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return false;

	return (pInventory->GetEquippedItemType() == _iItemType);
}

bool CRPG_Object_Char2::HasItem(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char2_HasItem, false);
	CRPG_Object_Item2* pLinkedItem = FindItemByType(_iItemType);
	return (pLinkedItem != NULL);
}

bool CRPG_Object_Char2::IsSelectableItem(CRPG_Object_Item2* _pItem, bool _bForceSelected)
{
	MAUTOSTRIP(CRPG_Object_Char2_IsSelectableItem, false);
	if (_pItem == NULL)
		return false;

	if ((_pItem->m_Flags & RPG_ITEM_FLAGS_FORCESELECTED) && (!_bForceSelected))
		return false;

	if (_pItem->m_Flags & RPG_ITEM_FLAGS_UNSELECTABLE)
		return false;

	if (_pItem->m_Flags & RPG_ITEM_FLAGS_REMOVED)
		return false;

	if (!_pItem->IsEquippable(this))
		return false;

	for (int iLink = 0; iLink < _pItem->m_NumLinks; iLink++)
	{
		bool bValid = false;
		int iLinkedItemType = _pItem->m_lLinkedItemTypes[iLink];
		if (_pItem->m_Flags & RPG_ITEM_FLAGS_NEEDEQUIPPEDLINK)
		{
			// Must work with selected items, even though they may not have been equipped yet.
			//			if (IsEquipped(iLinkedItemType))
			if (IsSelected(iLinkedItemType))
				return true;
		}
		else if (_pItem->m_Flags & RPG_ITEM_FLAGS_NEEDLINK)
		{
			if (HasItem(iLinkedItemType))
				return true;
		}
		else
			return true;
	}

	if (_pItem->m_NumLinks == 0)
		return true;

	return false;
}

bool CRPG_Object_Char2::SelectItem(int _iItemType, bool _bCheckSelectability, int _Method, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Char2_SelectItem, false);
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return false;

	if (_bCheckSelectability)
	{
		CRPG_Object_Item2* pItem = FindItemByType(_iItemType);
		if (!IsSelectableItem(pItem, _Method != SELECTIONMETHOD_NORMAL))
			return false;
	}

	bool bSelected = false;
	switch (_Method)
	{
	case SELECTIONMETHOD_NORMAL: bSelected = pInventory->SelectItem(_iItemType, this, _bInstant); break;
	case SELECTIONMETHOD_FORCE: bSelected = pInventory->ForceSelectItem(_iItemType, this, _bInstant); break;
	case SELECTIONMETHOD_FORCEACTIVATE: bSelected = pInventory->ForceActivateItem(_iItemType, this, _bInstant); break;
	}

	if (!bSelected)
		return false;

	return true;
}

// Return true if new item now is selected.
bool CRPG_Object_Char2::CycleSelectedItem(bool _bReverse, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Char2_CycleSelectedItem, false);
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return false;

	int MaxNumTries = pInventory->GetNumItems();
	int NumTries = 0;

	CRPG_Object_Item2* pNewItem;
	do
	{
		pNewItem = pInventory->GetCycledItem(NumTries, _bReverse);
		if (pNewItem == NULL)
			return false;

		if (IsSelectableItem(pNewItem, false))
			return SelectItem(pNewItem->m_iItemType, false, SELECTIONMETHOD_NORMAL, _bInstant);

		NumTries++;
	}
	while (NumTries < MaxNumTries);

	return false;
}

bool CRPG_Object_Char2::QuickActivateItem(CRPG_Object_Item2 *_pItem, const CMat43fp32 &_Mat, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_Char2_QuickActivateItem, false);
	_pItem->m_iLastActivator = _iObject;
	if(!_pItem->Activate(_Mat, this, _iObject, _Input))
	{
		return false;
	}
	return true;
}

bool CRPG_Object_Char2::ActivateItem(const CMat43fp32 &_Mat, int _iObject, int _Input, bool _bOverride)
{
	MAUTOSTRIP(CRPG_Object_Char2_ActivateItem, false);
	CRPG_Object_Item2* pEquippedItem = GetEquippedItem();
	if (pEquippedItem == NULL)
		return false;

	if (Wait() > 0)
		pEquippedItem->Interrupt(_Mat, this, _iObject, _Input);
	else
	{
		// This was put here why?! Can't remember, but I had a reason a few days ago =).
		//		if (pEquippedItem->m_Flags & RPG_ITEM_FLAGS_ACTIVATED)
		if (pEquippedItem->IsActivatable(this,_iObject,_Input) || _bOverride)
		{
			// FIXME: This is identical to the code in Inventory::PendingUpdate, so it should go through some abstracted function instead.
			// Mayby that function should be Item::Activate, but the we have to go through all activate functions to follow the convention.
			pEquippedItem->m_iLastActivator = _iObject; // Should be moved to CRPG_Object_Item2::Activate, somehow.
			pEquippedItem->m_Flags |= RPG_ITEM_FLAGS_ACTIVATED; // This too...
			pEquippedItem->Activate(_Mat, this, _iObject, _Input);
		}
	}

	return true; // Mondelore
}

bool CRPG_Object_Char2::DeactivateItem(const CMat43fp32 &_Mat, int _iObject ,int _Input)
{
	MAUTOSTRIP(CRPG_Object_Char2_DeactivateItem, false);
	CRPG_Object_Item2* pEquippedItem = GetEquippedItem();
	if (pEquippedItem == NULL)
		return false;

	if (!(pEquippedItem->m_Flags & RPG_ITEM_FLAGS_ACTIVATED))
		return false;

	pEquippedItem->Deactivate(_Mat, this, _iObject, _Input);
	pEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED; // Should be moved to CRPG_Object_Item2::Activate, somehow.

	return true;
}

bool CRPG_Object_Char2::DeactivateItems(const CMat43fp32 &_Mat, int _iObject, int _Input)
{
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return false;

	pInventory->DeactivateItems(_Mat, _iObject, _Input);
	return true;
}

CRPG_Object_Item2 *CRPG_Object_Char2::FindItemByType(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char2_FindItemByType, NULL);
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindItemByType(_iItemType);
}

CRPG_Object_Item2 *CRPG_Object_Char2::FindItemByName(const char* _pName)
{
	MAUTOSTRIP(CRPG_Object_Char2_FindItemByName, NULL);
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return NULL;

	CRPG_Object_Item2* pItem = pInventory->FindItemByName(_pName);
	if (pItem != NULL)
		return pItem;

	return NULL;
}

bool CRPG_Object_Char2::CanPickup(int32 _iItemType)
{
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return false;

	// Got inventory, check if we can merge without probs
	return pInventory->CanPickup(_iItemType);
}

CRPG_Object_Item2* CRPG_Object_Char2::GetFinalSelectedItem()
{
	MAUTOSTRIP(CRPG_Object_Char2_GetFinalSelectedItem, NULL);
	MSCOPESHORT(CRPG_Object_Char2::GetFinalSelectedItem);
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindFinalSelectedItem();
}

CRPG_Object_Item2* CRPG_Object_Char2::GetEquippedItem()
{
	MAUTOSTRIP(CRPG_Object_Char2_GetEquippedItem, NULL);
	MSCOPESHORT(CRPG_Object_Char2::GetEquippedItem);
	CRPG_Object_Inventory2* pInventory = GetInventory();
	if (pInventory == NULL)
		return NULL;

	return pInventory->GetEquippedItem();
}

CRPG_Object_Item2* CRPG_Object_Char2::GetItemFromGroup(int _iItemType, int _iGroup)
{
	MAUTOSTRIP(CRPG_Object_Char2_GetItemFromGroup, NULL);
	CRPG_Object_Inventory2 *pInventory = GetInventory();
	if (pInventory == NULL)
		return NULL;

	for(int iItemIndex = 0; iItemIndex < pInventory->GetNumItems(); iItemIndex++)
	{
		CRPG_Object_Item2* pItem = pInventory->GetItemByIndex(iItemIndex);
		if (pItem->m_WeaponGroup == _iGroup)
			return pItem;
	}

	return NULL;
}

void CRPG_Object_Char2::UpdateItems()
{
	MAUTOSTRIP(CRPG_Object_Char2_UpdateItems, MAUTOSTRIP_VOID);
	GetInventory()->UpdateItems(*this);
}
