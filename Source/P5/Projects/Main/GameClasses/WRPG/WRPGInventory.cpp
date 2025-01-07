#include "PCH.h"

#include "WRPGChar.h"
#include "WRPGInventory.h"
#include "WRPGItem.h"
#include "WRPGAmmo.h"
#include "../WObj_Char.h"

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Inventory, CRPG_Object);

//-------------------------------------------------------------------

int CRPG_Object_Inventory::GetGameTick(int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetGameTick, 0);
	int iTick = SendMsg(_iObject, OBJMSG_CHAR_GETGAMETICK);
	if (iTick == 0)
		iTick = m_pWServer->GetGameTick();

	return iTick;
}

//-------------------------------------------------------------------

void CRPG_Object_Inventory::UpdateItems(CRPG_Object_Char &_Char)
{
	MAUTOSTRIP(CRPG_Object_Inventory_UpdateItems, MAUTOSTRIP_VOID);
	for(int i = 0; i < GetNumItems(); i++)
		GetItemByIndex(i)->UpdateAmmoList(&_Char);
}

//-------------------------------------------------------------------

void CRPG_Object_Inventory::ActivateItems(CRPG_Object_Char &_Char, CRPG_Object_Item *_pPrimaryItem)
{
	MAUTOSTRIP(CRPG_Object_Inventory_ActivateItems, MAUTOSTRIP_VOID);
	if(_pPrimaryItem)
	{
		for(int i = 0; i < GetNumItems(); i++)
		{
			CRPG_Object_Item *pItem = GetItemByIndex(i);
			int Flags = RPG_ITEM_FLAGS_USELESS; // Set flag so that linked items explicitly removes flag.
			for (int iLinkedItemType = 0; iLinkedItemType < RPG_ITEM_MAXLINKS; iLinkedItemType++)
			{
				int NeededItemType = pItem->m_lLinkedItemTypes[iLinkedItemType];
				if ((NeededItemType != -1) && (_pPrimaryItem->m_iItemType == NeededItemType))
					Flags &= ~RPG_ITEM_FLAGS_USELESS;
			}

			pItem->m_Flags &= ~RPG_ITEM_FLAGS_USELESS;
			pItem->m_Flags |= Flags;
		}
	}
}

void CRPG_Object_Inventory::DeactivateItems(const CMat4Dfp32 &_Mat, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_Inventory_DeactivateItems, MAUTOSTRIP_VOID);
	for(int i = 0; i < GetNumItems(); i++)
	{
		CRPG_Object_Item *pItem = GetItemByIndex(i);

		if (pItem->m_Flags & RPG_ITEM_FLAGS_ACTIVATED)
		{
			pItem->Deactivate(_Mat, this, _iObject, _Input);
			pItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED;
		}
	}
}
//-------------------------------------------------------------------

void CRPG_Object_Inventory::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_Inventory_OnCreate, MAUTOSTRIP_VOID);
	m_EquippedItemIdentifier = 0;
	m_LastEquippedItemIdentifier = 0;
	m_iSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
	m_iForceSelectedItemIdentifier = 0;
	m_iSelectedItemIdentifier = 0;
	m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
	m_iForceSelectedItemAction = FORCESELECTION_NONE;
	m_iForceSelectedOtherInventory = -1;
	m_GUITaggedItemID = -1;
	m_bInstantEquip = false;
	m_bShareAnimation = true;
	m_ItemCounter = 0;
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory::OnProcess(CRPG_Object* _pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Inventory_OnProcess, false);
	CRPG_Object_Item* pEquippedItem = GetEquippedItem();
	if (pEquippedItem == NULL)
		return false;

/*
	if (!(pEquippedItem->m_Flags & RPG_ITEM_FLAGS_USEABLE))
		return false;
*/

	return pEquippedItem->OnProcess(_pRoot, _iObject);
}

//-------------------------------------------------------------------

void CRPG_Object_Inventory::OnCharDeath(int _iObject, CRPG_Object* _pRoot)
{
	MAUTOSTRIP(CRPG_Object_Inventory_OnCharDeath, MAUTOSTRIP_VOID);
	for(int i = 0; i < GetNumItems(); i++)
		GetItemByIndex(i)->OnCharDeath(_iObject, _pRoot);
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory::AddItem(int _iObject, CRPG_Object_Item* _pItem)
{
	MAUTOSTRIP(CRPG_Object_Inventory_AddItem, false);
	// Set unique item identifier
	if (_pItem)
	{
		if (_pItem->m_Identifier == 0)
			_pItem->SetItemIdentifier(++m_ItemCounter);
		else
			m_ItemCounter = Max(m_ItemCounter, _pItem->m_Identifier);

		m_GUITaggedItemID = _pItem->m_Identifier;
	}

	return (AddChild(_pItem) > -1);
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory::RemoveItemByIndex(int _iObject, int _iItemIndex, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory_RemoveItemByIndex, false);
	CRPG_Object_Item* pItem = GetItemByIndex(_iItemIndex);
	if (pItem == NULL)
		return false;

	// FIXME: Mayby we need to get a real pChar here, instead of cheating with NULL =).
	CRPG_Object_Char* pChar = NULL;

	int iEquippedItem = FindItemIndexByIdentifier(m_EquippedItemIdentifier);

	if (iEquippedItem == _iItemIndex)
	{
		// Item to be removed is currently equipped. We then have to unequip it first.

		// Flag it as removed, so that this function is called later, when the item has been unequipped.
		pItem->m_Flags |= RPG_ITEM_FLAGS_REMOVED | (_bRemoveInstant ? RPG_ITEM_FLAGS_REMOVEINSTANT : 0);

		// Try to reselect same itemtype (removed item may have been replaced).
		// If current item should be/has been dropped we may have a suitably selected item as well. I haven't the faintest clue if this is the correct way to go but wtf...
		// Unequip the item
		pItem->Unequip(_iObject, pChar, true, false);
		if ((pItem->m_iItemType == m_iSelectedItemType) || (pItem->m_Flags & RPG_ITEM_FLAGS_DROPONUNEQUIP))
			SelectItem(m_iSelectedItemType, pChar, m_bInstantEquip); 
		else
			SelectItem(RPG_ITEMTYPE_UNDEFINED, pChar, true); // Else select nothing.


		if(_bRemoveInstant)
			m_EquippedItemIdentifier = 0;
/*		m_iEquippedItemIndex = -1;
		iEquippedItemType = RPG_ITEMTYPE_UNDEFINED;
		SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, pItem->m_iItemType >> 8, GetGameTick(_iObject));
*/
		DelChild(_iItemIndex);
		return false;
	}

	if (pItem->m_iItemType == m_iSelectedItemType)
		SelectItem(m_iSelectedItemType, pChar, m_bInstantEquip); // Try to reselect same itemtype (removed item may have been replaced).
		
	if (pItem->m_iItemType == m_iForceSelectedItemType)
	{
		m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemAction = FORCESELECTION_NONE;
		m_iForceSelectedItemIdentifier = 0;
	}

	DelChild(_iItemIndex);

	return true;
}

bool CRPG_Object_Inventory::RemoveItemByIdentifier(int _iObject, int _Identifier, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory_RemoveItemByIndex, false);
	CRPG_Object_Item* pItem = FindItemByIdentifier(_Identifier);
	if (pItem == NULL)
		return false;

	// FIXME: Mayby we need to get a real pChar here, instead of cheating with NULL =).
	CRPG_Object_Char* pChar = NULL;

	int ItemIndex = FindItemIndexByIdentifier(_Identifier);

	if (m_EquippedItemIdentifier == _Identifier)
	{
		// Item to be removed is currently equipped. We then have to unequip it first.

		// Flag it as removed, so that this function is called later, when the item has been unequipped.
		pItem->m_Flags |= RPG_ITEM_FLAGS_REMOVED | (_bRemoveInstant ? RPG_ITEM_FLAGS_REMOVEINSTANT : 0);

		// Try to reselect same itemtype (removed item may have been replaced).
		// If current item should be/has been dropped we may have a suitably selected item as well. I haven't the faintest clue if this is the correct way to go but wtf...
		// Unequip the item
		pItem->Unequip(_iObject, pChar, true, false);
		if ((pItem->m_iItemType == m_iSelectedItemType) || (pItem->m_Flags & RPG_ITEM_FLAGS_DROPONUNEQUIP))
			SelectItem(m_iSelectedItemType, pChar, m_bInstantEquip); 
		else
			SelectItem(RPG_ITEMTYPE_UNDEFINED, pChar, true); // Else select nothing.

		if(_bRemoveInstant)
			m_EquippedItemIdentifier = 0;

		DelChild(ItemIndex);
		return false;
	}

	/*if (pItem->m_iItemType == m_iSelectedItemType)
		SelectItem(m_iSelectedItemType, pChar, m_bInstantEquip); // Try to reselect same itemtype (removed item may have been replaced).*/

	if (pItem->m_iItemType == m_iForceSelectedItemType)
	{
		m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemAction = FORCESELECTION_NONE;
		m_iForceSelectedItemIdentifier = 0;
	}

	DelChild(ItemIndex);

	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory::RemoveItemByName(int _iObject, const char* _pName, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory_RemoveItemByName, false);
	return RemoveItemByIndex(_iObject, FindItemIndex(_pName), _bRemoveInstant);
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory::RemoveItemByType(int _iObject, int _iItemType, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory_RemoveItemByType, false);
	return RemoveItemByIndex(_iObject, FindItemIndex(_iItemType), _bRemoveInstant);
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory::ReplaceItemByType(int _iObject, CRPG_Object_Item* _pNewItem, bool _bInstantly)
{
	MAUTOSTRIP(CRPG_Object_Inventory_ReplaceItemByType, false);
	if (_pNewItem == NULL)
		return false;

	int iOldItemIndex = FindItemIndex(_pNewItem->m_iItemType);
	if (iOldItemIndex != -1)
	{
		CRPG_Object_Item* pOldItem = GetItemByIndex(iOldItemIndex);
		// Old item in conflict over this itemtype. Replace it nicely.
		if (pOldItem && (m_EquippedItemIdentifier == pOldItem->m_Identifier))
		{
			// Unequip the item
			pOldItem->Unequip(_iObject, NULL, true, false);
			// Old item was equipped.
//			if (_bInstantly)
			{
				// Replicating equipped flag.
				_pNewItem->m_Flags |= pOldItem->m_Flags & RPG_ITEM_FLAGS_EQUIPPED;

				// Replace instantly
				SetChild(iOldItemIndex, _pNewItem);
			}
/*				else
			{
				// Delay removal of old item (so that it can be unequipped nicely).
				pOldItem->m_Flags |= RPG_ITEM_FLAGS_REMOVED;

				// Add new item, so that it may be equipped later.
				AddItem(_iObject, _pNewItem);
			}*/
		}
		else
		{
			SetChild(iOldItemIndex, _pNewItem);
		}
	}
	else
	{
		// No old item in conflict over this itemtype. Just add the new item.
		AddItem(_iObject, _pNewItem);
	}

	return true;
}

bool CRPG_Object_Inventory::UpdateItemIdentifier(CRPG_Object_Item* _pItem)
{
	if (!_pItem)
		return false;

	_pItem->m_Identifier = ++m_ItemCounter;
	return true;
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory::GetItemType(int _iItemIndex)
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetItemType, 0);
	CRPG_Object_Item* pItem = GetItemByIndex(_iItemIndex);
	if (pItem == NULL)
		return RPG_ITEMTYPE_UNDEFINED;

	return pItem->m_iItemType;
}

int CRPG_Object_Inventory::GetItemTypeByIdentifier(int _Identifier)
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetItemTypeByIdentifier, 0);
	CRPG_Object_Item* pItem = FindItemByIdentifier(_Identifier);
	if (pItem == NULL)
		return RPG_ITEMTYPE_UNDEFINED;

	return pItem->m_iItemType;
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory::FindItemIndex(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindItemIndex, 0);
	if (_iItemType == RPG_ITEMTYPE_UNDEFINED)
		return -1;

	for (int iChild = 0; iChild < GetNumItems(); iChild++)
		if ((GetItemByIndex(iChild)->m_iItemType == _iItemType) && 
			!(GetItemByIndex(iChild)->m_Flags & RPG_ITEM_FLAGS_REMOVED))
			return iChild;

	return -1;
}

int CRPG_Object_Inventory::FindItemIndexByAnimType(int _iAnimType)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindItemIndex, 0);
	if (_iAnimType == RPG_ITEMTYPE_UNDEFINED)
		return -1;

	for (int iChild = 0; iChild < GetNumItems(); iChild++)
		if ((GetItemByIndex(iChild)->m_AnimType == _iAnimType) && 
			!(GetItemByIndex(iChild)->m_Flags & RPG_ITEM_FLAGS_REMOVED))
			return iChild;

	return -1;
}

int CRPG_Object_Inventory::FindItemIndexByIdentifier(int _Identifier)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindItemIndex, 0);
	if (_Identifier == 0)
		return -1;

	for (int iChild = 0; iChild < GetNumItems(); iChild++)
	{
		CRPG_Object_Item* pItem = GetItemByIndex(iChild);
		if ((pItem->m_Identifier == _Identifier) && 
			!(pItem->m_Flags & RPG_ITEM_FLAGS_REMOVED))
			return iChild;
	}

	return -1;
}

bool CRPG_Object_Inventory::CanPickup(int32 _iItemType)
{
	for (int iChild = 0; iChild < GetNumItems(); iChild++)
	{
		CRPG_Object_Item* pItem = GetItemByIndex(iChild);
		if ((pItem->m_iItemType == _iItemType) && 
			!(pItem->m_Flags & RPG_ITEM_FLAGS_REMOVED))
		{
			if (!pItem->CanPickup())
				return false;
		}
	}
	return true;
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory::FindItemIndex(const char* _pName)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindItemIndex_2, 0);
	for (int iChild = 0; iChild < GetNumItems(); iChild++)
		if ((GetItemByIndex(iChild)->m_Name.CompareNoCase(_pName) == 0) && 
			!(GetItemByIndex(iChild)->m_Flags & RPG_ITEM_FLAGS_REMOVED))
			return iChild;

	return -1;
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory::GetNumItems()
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetNumItems, 0);
	return GetNumChildren();
}

//-------------------------------------------------------------------

CRPG_Object_Item* CRPG_Object_Inventory::GetItemByIndex(int _iItemIndex)
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetItemByIndex, NULL);
	if ((_iItemIndex < 0) || (_iItemIndex >= GetNumItems()))
		return NULL;

	return (CRPG_Object_Item*)GetChild(_iItemIndex);
}

//-------------------------------------------------------------------

CRPG_Object_Item* CRPG_Object_Inventory::FindItemByName(const char* _pName)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindItemByName, NULL);
	return GetItemByIndex(FindItemIndex(_pName));
}

void CRPG_Object_Inventory::RemoveEmptyItems(int _Identifier)
{
	MAUTOSTRIP(CRPG_Object_Inventory_RemoveEmptyItems, 0);
	int NumItems = GetNumItems();
	int SelectedItemIdent = GetFinalSelectedItemIdentifier();
	for (int iChild = 0; iChild < NumItems; iChild++)
	{
		CRPG_Object_Item* pItem = GetItemByIndex(iChild);
		if (pItem->m_Identifier != _Identifier && pItem->m_Identifier != SelectedItemIdent && 
			(pItem->m_Flags2 & RPG_ITEM_FLAGS2_THROWAWAYONEMPTY) && !pItem->GetAmmo(NULL) && 
			!(pItem->m_Flags & RPG_ITEM_FLAGS_EQUIPPED))
		{
			// Make sure theres more of this type if we shouldn't throw away the last one
			if (pItem->m_Flags2 & RPG_ITEM_FLAGS2_NOLASTHROWAWAY)
			{
				CRPG_Object_Item* pOtherItem = FindNextItemByType(pItem->m_Identifier,pItem->m_iItemType);
				if (!pOtherItem)
					continue;
			}
			// Remove this item
			RemoveItemByIndex(0,iChild);
			iChild--;
			NumItems--;
		}
	}
}

void CRPG_Object_Inventory::ReloadItemFromOthers(CRPG_Object_Summon* _pItemSummon)
{
	int Identifier = _pItemSummon->m_Identifier;
	int ItemType = _pItemSummon->m_iItemType;
	// Steal ammo from the other items
	int32 AmmoNeeded = _pItemSummon->m_MaxAmmo - _pItemSummon->m_AmmoLoad;
	if(AmmoNeeded)
	{
		int32 NumItems = GetNumItems();
		for(int iChild = 0; iChild < NumItems; iChild++)
		{
			CRPG_Object_Item* pItemNext = GetItemByIndex(iChild);
			if(pItemNext->m_iItemType != ItemType || pItemNext->m_Identifier == Identifier)
				continue;

			CRPG_Object_Summon* pItemNextSummon = safe_cast<CRPG_Object_Summon>(pItemNext);
			if(pItemNextSummon)
				continue;

			if(!(pItemNext->m_Flags2 & RPG_ITEM_FLAGS2_NOTSELECTABLE))
			{
				int32 Ammo = pItemNextSummon->m_AmmoLoad;
				if(Ammo >= AmmoNeeded)
				{
					pItemNextSummon->m_AmmoLoad = Ammo - AmmoNeeded;
					_pItemSummon->m_AmmoLoad = _pItemSummon->m_MaxAmmo;
					break;
				}
				else if(Ammo)
				{
					_pItemSummon->m_AmmoLoad = _pItemSummon->m_AmmoLoad + Ammo;
					pItemNextSummon->m_AmmoLoad = 0;
					AmmoNeeded -= Ammo;
				}
			}
		}
		RemoveEmptyItems();
	}
}

//-------------------------------------------------------------------

CRPG_Object_Item* CRPG_Object_Inventory::FindItemByType(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindItemByType, NULL);
	return GetItemByIndex(FindItemIndex(_iItemType));
}

CRPG_Object_Item* CRPG_Object_Inventory::FindItemByAnimType(int _iAnimType)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindItemByAnimType, NULL);
	return GetItemByIndex(FindItemIndexByAnimType(_iAnimType));
}

CRPG_Object_Item* CRPG_Object_Inventory::FindNextItemByType(int _Identifier, int _iItemType, bool _bForce)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindNextItemByType, 0);
	if (_iItemType == RPG_ITEMTYPE_UNDEFINED)
		return NULL;

	int32 NumItems = GetNumItems();
	for (int iChild = 0; iChild < NumItems; iChild++)
	{
		CRPG_Object_Item* pItem = GetItemByIndex(iChild);
		if ((pItem->m_iItemType == _iItemType) && (pItem->m_Identifier != _Identifier) &&
			(_bForce || !(pItem->m_Flags2 & RPG_ITEM_FLAGS2_NOTSELECTABLE)) && !(pItem->m_Flags & RPG_ITEM_FLAGS_REMOVED))
			return pItem;
	}

	return NULL;
}

CRPG_Object_Item* CRPG_Object_Inventory::FindNextItemForReloadByType(int _Identifier, int _iItemType, CRPG_Object_Item* _pOldItem)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindNextItemByType, 0);
	if (_iItemType == RPG_ITEMTYPE_UNDEFINED)
		return NULL;

	// Can't switch from permanent to permanent
	bool bPermanent = _pOldItem ? (_pOldItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT) != 0 : false;
	int32 NumItems = GetNumItems();
	CRPG_Object_Item* pLastResort = NULL;
	for (int iChild = 0; iChild < NumItems; iChild++)
	{
		CRPG_Object_Item* pItem = GetItemByIndex(iChild);
		if ((pItem->m_iItemType == _iItemType) && (pItem->m_Identifier != _Identifier) &&
			!(pItem->m_Flags & RPG_ITEM_FLAGS_REMOVED) && !(pItem->m_Flags2 & (RPG_ITEM_FLAGS2_TAGGEDFORRELOAD|RPG_ITEM_FLAGS2_NOTSELECTABLE)) &&
			!(pItem->m_Flags & RPG_ITEM_FLAGS_EQUIPPED))
		{
			if (pItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT)
			{
				pLastResort = pItem;
				continue;
			}
			return pItem;
		}
	}

	return bPermanent ? NULL : pLastResort;
}

CRPG_Object_Item* CRPG_Object_Inventory::FindNextItemNotEquippedByType(int _Identifier, int _iItemType, CRPG_Object_Item* _pOldItem)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindNextItemNotEquippedByType, 0);
	if (_iItemType == RPG_ITEMTYPE_UNDEFINED)
		return NULL;

	bool bPermanent = _pOldItem ? (_pOldItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT) != 0 : false;
	CRPG_Object_Item* pLastResort = NULL;
	int32 NumItems = GetNumItems();
	for (int iChild = 0; iChild < NumItems; iChild++)
	{
		CRPG_Object_Item* pItem = GetItemByIndex(iChild);
		if ((pItem->m_iItemType == _iItemType) && (pItem->m_Identifier != _Identifier) && 
			!(pItem->m_Flags2 & RPG_ITEM_FLAGS2_TAGGEDFORRELOAD) &&
			!(pItem->m_Flags & (RPG_ITEM_FLAGS_REMOVED|RPG_ITEM_FLAGS_EQUIPPED)))
		{
			if (pItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT)
			{
				pLastResort = pItem;
				continue;
			}
			return pItem;
		}
	}

	return pLastResort;
}

CRPG_Object_Item* CRPG_Object_Inventory::FindNextItemNotEquippedByAnimType(int _Identifier, int _AnimType, int _NotThisItemType, CRPG_Object_Item* _pOldItem, bool _bWithAmmo)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindNextItemNotEquippedByAnimType, 0);
	bool bPermanent = _pOldItem ? (_pOldItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT) != 0 : false;
	int32 NumItems = GetNumItems();
	CRPG_Object_Item* pLastResort = NULL;
	int iBest = 0x7fffffff;
	CRPG_Object_Item* pBest = NULL;
	for (int iChild = 0; iChild < NumItems; iChild++)
	{
		CRPG_Object_Item* pItem = GetItemByIndex(iChild);
		if (pItem->m_LastEquipped < iBest && (pItem->m_AnimType == _AnimType) && (pItem->m_iItemType != _NotThisItemType) &&
			(pItem->m_Identifier != _Identifier) && !(pItem->m_Flags & (RPG_ITEM_FLAGS_REMOVED|RPG_ITEM_FLAGS_EQUIPPED)) && 
			!(pItem->m_Flags2 & (RPG_ITEM_FLAGS2_TAGGEDFORRELOAD|RPG_ITEM_FLAGS2_NOTSELECTABLE)) &&
			(!_bWithAmmo || pItem->GetAmmo(NULL) || pItem->GetMagazines()))
		{
			if (pItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT)
			{
				pLastResort = pItem;
				continue;
			}
			pBest = pItem;
			iBest = pItem->m_LastEquipped;
		}
	}

	return pBest ? pBest : (bPermanent ? NULL : pLastResort);
}

CRPG_Object_Item* CRPG_Object_Inventory::FindNextItemNotEquippedByGroupType(CRPG_Object* _pRoot, int _Identifier, int _GroupType, int _NotThisItemType, bool _bWithAmmo, CRPG_Object_Item* _pOldItem)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindNextItemNotEquippedByGroupType, 0);
	bool bPermanent = _pOldItem ? (_pOldItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT) != 0 : false;
	int32 NumItems = GetNumItems();
	CRPG_Object_Item* pLastResort = NULL;
	int iBest = 0x7fffffff;
	CRPG_Object_Item* pBest = NULL;
	for (int iChild = 0; iChild < NumItems; iChild++)
	{
		CRPG_Object_Item* pItem = GetItemByIndex(iChild);
		if (pItem->m_LastEquipped < iBest && (pItem->m_GroupType == _GroupType) && (pItem->m_iItemType != _NotThisItemType) &&
			(pItem->m_Identifier != _Identifier) && !(pItem->m_Flags & (RPG_ITEM_FLAGS_REMOVED|RPG_ITEM_FLAGS_EQUIPPED)) &&
			!(pItem->m_Flags2 & (RPG_ITEM_FLAGS2_TAGGEDFORRELOAD|RPG_ITEM_FLAGS2_NOTSELECTABLE)) &&	
			(!_bWithAmmo || pItem->GetAmmo(_pRoot) || pItem->GetMagazines()))
		{
			if (pItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT)
			{
				pLastResort = pItem;
				continue;
			}
			pBest = pItem;
			iBest = pItem->m_LastEquipped;
		}
	}

	return pBest ? pBest : (bPermanent ? NULL : pLastResort);
}


CRPG_Object_Item* CRPG_Object_Inventory::FindItemByIdentifier(int _Identifier)
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindItemByType, NULL);
	return GetItemByIndex(FindItemIndexByIdentifier(_Identifier));
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory::GetFinalSelectedItemIndex()
{
	// Get corresponding item index from item type we have equipped or is trying to equip
	return FindItemIndexByIdentifier(GetFinalSelectedItemIdentifier());
};

//-------------------------------------------------------------------

int CRPG_Object_Inventory::GetNextValidSelectedItemIndex(int _iIndex)
{
	//Check each item after the given. 
	//Currently we allow all non-removed items, but if we wanted we might for example exclude 
	//firearms without ammo etc (if we did that we should still allow one item with a flashlight though)
	// Check current type, don't select one with the same type
	CRPG_Object_Item * pCurrent = GetItemByIndex(_iIndex);
	if (!pCurrent)
		return _iIndex;

	int iNext;
	CRPG_Object_Item * pNext;
	for (int i = 1; i < GetNumItems(); i++)
	{
		iNext = (_iIndex + i) % GetNumItems();
		pNext = GetItemByIndex(iNext);
		if (pNext && pCurrent->m_iItemType != pNext->m_iItemType && 
			!(pNext->m_Flags & RPG_ITEM_FLAGS_REMOVED) && !(pNext->m_Flags2 & RPG_ITEM_FLAGS2_NOTSELECTABLE))
		{
			//Valid next item found
			return iNext;
		}
	}
	//Couldn't find any other valid items.
	return _iIndex;
};

int CRPG_Object_Inventory::GetPreviousValidSelectedItemIndex(int _iIndex)
{
	//Check item before the given. 
	CRPG_Object_Item * pCurrent = GetItemByIndex(_iIndex);
	if (!pCurrent)
		return _iIndex;

	int iPrev;
	CRPG_Object_Item * pPrev;
	const int NumItems = GetNumItems();
	for (int i = 1; i < NumItems; i++)
	{
		iPrev = (_iIndex + NumItems - i) % NumItems;
		pPrev = GetItemByIndex(iPrev);
		if (pPrev && pCurrent->m_iItemType != pPrev->m_iItemType && 
			!(pPrev->m_Flags & RPG_ITEM_FLAGS_REMOVED) && !(pPrev->m_Flags2 & RPG_ITEM_FLAGS2_NOTSELECTABLE))
		{
			//Valid next item found
			return iPrev;
		}
	}
	//Couldn't find any other valid items.
	return _iIndex;
};

int CRPG_Object_Inventory::GetNextValidSelectedItemIndexByType(CRPG_Object_Item * _pCurrent, int _ExcludeType)
{
	if (!_pCurrent)
		return -1;

	int NumItems = GetNumItems(); 
	// Check all items and determine order for them, for example inv: fist gun ak47 gun shotgun gun
	// When we switch, only go: fist gun ak47
	TArray<int> lTypeOrder;
	int iCurrentType = -1;
	for (int32 i = 0; i < NumItems; i++)
	{
		CRPG_Object_Item* pItem = GetItemByIndex(i);
		if (pItem && pItem->m_iItemType != _ExcludeType &&
			!(pItem->m_Flags & RPG_ITEM_FLAGS_REMOVED) && !(pItem->m_Flags2 & RPG_ITEM_FLAGS2_NOTSELECTABLE))
		{
			// Check if we've added this type before
			bool bOk = true;
			int NumTypes = lTypeOrder.Len();
			for (int j = 0; j < NumTypes; j++)
			{
				if (lTypeOrder[j] == pItem->m_iItemType)
					bOk = false;
			}
			if (bOk)
			{
				lTypeOrder.Add(pItem->m_iItemType);
				// Check if it's the given type, if so set it
				if (pItem->m_iItemType == _pCurrent->m_iItemType)
					iCurrentType = NumTypes;
			}
		}
	}

	if (iCurrentType < 0)
		return -1;

	NumItems = lTypeOrder.Len();
	for (int i = 1; i < NumItems; i++)
	{
		int Type = lTypeOrder[(iCurrentType + i) % NumItems];
		CRPG_Object_Item* pNext = FindItemByType(Type);
		if (pNext && _pCurrent->m_iItemType != pNext->m_iItemType && 
			!(pNext->m_Flags & RPG_ITEM_FLAGS_REMOVED) && !(pNext->m_Flags2 & RPG_ITEM_FLAGS2_NOTSELECTABLE))
		{
			return FindItemIndexByIdentifier(pNext->m_Identifier);
		}
	}

	return -1;
}

int CRPG_Object_Inventory::GetPreviousValidSelectedItemIndexByType(CRPG_Object_Item * _pCurrent, int _ExcludeType)
{
	if (!_pCurrent)
		return -1;

	int NumItems = GetNumItems(); 
	// Check all items and determine order for them, for example inv: fist gun ak47 gun shotgun gun
	// When we switch, only go: fist gun ak47
	TArray<int> lTypeOrder;
	int iCurrentType = -1;
	for (int32 i = 0; i < NumItems; i++)
	{
		CRPG_Object_Item* pItem = GetItemByIndex(i);
		if (pItem && pItem->m_iItemType != _ExcludeType &&
			!(pItem->m_Flags & RPG_ITEM_FLAGS_REMOVED) && !(pItem->m_Flags2 & RPG_ITEM_FLAGS2_NOTSELECTABLE))
		{
			// Check if we've added this type before
			bool bOk = true;
			int NumTypes = lTypeOrder.Len();
			for (int j = 0; j < NumTypes; j++)
			{
				if (lTypeOrder[j] == pItem->m_iItemType)
					bOk = false;
			}
			if (bOk)
			{
				lTypeOrder.Add(pItem->m_iItemType);
				// Check if it's the given type, if so set it
				if (pItem->m_iItemType == _pCurrent->m_iItemType)
					iCurrentType = NumTypes;
			}
		}
	}

	if (iCurrentType < 0)
		return -1;

	NumItems = lTypeOrder.Len();
	for (int i = 1; i < NumItems; i++)
	{
		int Type = lTypeOrder[(iCurrentType + NumItems - 1) % NumItems];
		CRPG_Object_Item* pNext = FindItemByType(Type);
		if (pNext && _pCurrent->m_iItemType != pNext->m_iItemType && 
			!(pNext->m_Flags & RPG_ITEM_FLAGS_REMOVED) && !(pNext->m_Flags2 & RPG_ITEM_FLAGS2_NOTSELECTABLE))
		{
			return FindItemIndexByIdentifier(pNext->m_Identifier);
		}
	}

	return -1;
}

//-------------------------------------------------------------------

CRPG_Object_Item* CRPG_Object_Inventory::GetEquippedItem()
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetEquippedItem, NULL);
	return FindItemByIdentifier(m_EquippedItemIdentifier);
}

//-------------------------------------------------------------------

CRPG_Object_Item* CRPG_Object_Inventory::FindFinalSelectedItem()
{
	MAUTOSTRIP(CRPG_Object_Inventory_FindFinalSelectedItem, NULL);
	return FindItemByIdentifier(GetFinalSelectedItemIdentifier());
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory::GetEquippedItemType()
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetEquippedItemType, 0);
	return GetItemTypeByIdentifier(m_EquippedItemIdentifier);
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory::GetSelectedItemType()
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetSelectedItemType, 0);
	return m_iSelectedItemType;
}

//-------------------------------------------------------------------

CRPG_Object_Item* CRPG_Object_Inventory::GetCycledItem(int _NumTries, bool _bReverse)
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetCycledItem, NULL);
	if (m_iForceSelectedItemType != RPG_ITEMTYPE_UNDEFINED)
		return NULL;

	if (GetNumItems() == 0)
		return NULL;

	int iSelectedItemIndex = FindItemIndexByIdentifier(m_iSelectedItemIdentifier);

	if (m_iSelectedItemType == RPG_ITEMTYPE_UNDEFINED)
	{
		if (GetNumItems() > 0)
		{
			iSelectedItemIndex = _NumTries;
			if (iSelectedItemIndex >= GetNumItems())
				iSelectedItemIndex -= GetNumItems();
		}
		else
			iSelectedItemIndex = -1;
	}
	else
	{
		if (!_bReverse)
		{
			iSelectedItemIndex += _NumTries + 1;
			if (iSelectedItemIndex >= GetNumItems())
				iSelectedItemIndex -= GetNumItems();
		}
		else
		{
			iSelectedItemIndex -= _NumTries + 1;
			if (iSelectedItemIndex < 0)
				iSelectedItemIndex += GetNumItems();
		}
	}

	CRPG_Object_Item* pNewItem = GetItemByIndex(iSelectedItemIndex);
	if (pNewItem == NULL)
		return NULL;

	return pNewItem;
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory::SelectItem(int _iItemType, CRPG_Object_Char* _pChar, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory_SelectItem, false);
//	if (m_iForceSelectedItemType != RPG_ITEMTYPE_UNDEFINED)
//		return false;

/*
	if (_iItemType == m_iSelectedItemType)
		return true;
*/

	// Find out if we have any item of this type.
	CRPG_Object_Item* pItem = GetItemByIndex(FindItemIndex(_iItemType));
	if ((_iItemType != RPG_ITEMTYPE_UNDEFINED) && (FindItemIndex(_iItemType) == -1) || !pItem)
	{
		// If we don't have such an item, but such an item is selected, select nothing.
		if (m_iSelectedItemType == _iItemType)
			m_iSelectedItemType = RPG_ITEMTYPE_UNDEFINED;

		return false;
	}

	m_iSelectedItemType = _iItemType;
	m_iSelectedItemIdentifier = pItem->m_Identifier;

	if (m_iForceSelectedItemType == RPG_ITEMTYPE_UNDEFINED)
		m_bInstantEquip = _bInstant;

	return true;
}

bool CRPG_Object_Inventory::SelectItemByIdentifier(int _Identifier, CRPG_Object_Char* _pChar, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory_SelectItemByIdentifier, false);

	// Find out if we have any item of this type.
	CRPG_Object_Item* pItem = FindItemByIdentifier(_Identifier);
	if (!pItem)
	{
		m_iSelectedItemType = RPG_ITEMTYPE_UNDEFINED;

		return false;
	}

	m_iSelectedItemType = pItem->m_iItemType;
	m_iSelectedItemIdentifier = pItem->m_Identifier;

	if (m_iForceSelectedItemType == RPG_ITEMTYPE_UNDEFINED)
		m_bInstantEquip = _bInstant;

	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory::ForceSelectItem(int _iItemType, CRPG_Object_Char* _pChar, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory_ForceSelectItem, false);
	if (_iItemType == RPG_ITEMTYPE_UNDEFINED)
	{
		m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemAction = FORCESELECTION_NONE;
		m_iForceSelectedItemIdentifier = 0;
		m_bInstantEquip = _bInstant;
		return true;
	}
	else
	{
		int iForcedItemIndex = FindItemIndex(_iItemType);
		CRPG_Object_Item* pForcedItem = GetItemByIndex(iForcedItemIndex);
		if (_iItemType != RPG_ITEMTYPE_EMPTY)
		{
			// Find item and see if it's equippable (has any ammo/items left).
			if ((pForcedItem == NULL) || (!pForcedItem->IsEquippable(_pChar)))
				return false;
		}

		m_iForceSelectedItemType = _iItemType;
		m_iForceSelectedItemAction = FORCESELECTION_WAITACTIVATE;
		m_iForceSelectedItemIdentifier = pForcedItem->m_Identifier;
		m_bInstantEquip = _bInstant;
		return true;
	}
}

bool CRPG_Object_Inventory::ForceSelectItemByIdentifier(int _Identifier, CRPG_Object_Char* _pChar, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory_ForceSelectItem, false);
	if (!_Identifier)
	{
		m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemAction = FORCESELECTION_NONE;
		m_iForceSelectedItemIdentifier = 0;
		m_bInstantEquip = _bInstant;
		return true;
	}
	else
	{
		int iItemType = RPG_ITEMTYPE_UNDEFINED;
		// Find item and see if it's equippable (has any ammo/items left).
		int iForcedItemIndex = FindItemIndexByIdentifier(_Identifier);
		CRPG_Object_Item* pForcedItem = GetItemByIndex(iForcedItemIndex);
		if (pForcedItem)
			iItemType = pForcedItem->m_iItemType;
		if (!pForcedItem || (!pForcedItem->IsEquippable(_pChar)))
			return false;

		m_iForceSelectedItemType = iItemType;
		m_iForceSelectedItemAction = FORCESELECTION_WAITACTIVATE;
		m_iForceSelectedItemIdentifier = pForcedItem->m_Identifier;
		m_bInstantEquip = _bInstant;
		return true;
	}
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory::ForceActivateItem(int _iItemType, CRPG_Object_Char* _pChar, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory_ForceActivateItem, false);
	if (_iItemType == RPG_ITEMTYPE_UNDEFINED)
	{
		m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemAction = FORCESELECTION_NONE;
		m_iForceSelectedItemIdentifier = 0;
		m_bInstantEquip = _bInstant;
		return true;
	}
	else
	{
		int iForcedItemIndex = FindItemIndex(_iItemType);
		CRPG_Object_Item* pForcedItem = GetItemByIndex(iForcedItemIndex);
		if (_iItemType != RPG_ITEMTYPE_EMPTY)
		{
			// Find item and see if it's equippable (has any ammo/items left).
			if ((pForcedItem == NULL) || (!pForcedItem->IsEquippable(_pChar)) || (!pForcedItem->IsActivatable(_pChar,0)))
				return false;
		}

		m_iForceSelectedItemType = _iItemType;
		m_iForceSelectedItemAction = FORCESELECTION_ACTIVATE;
		m_iForceSelectedItemIdentifier = pForcedItem->m_Identifier;
		m_bInstantEquip = _bInstant;
		return true;
	}
}

bool CRPG_Object_Inventory::ForceActivateItemByIdentifier(int _Identifier, CRPG_Object_Char* _pChar, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory_ForceActivateItem, false);
	if (_Identifier == 0)
	{
		m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemAction = FORCESELECTION_NONE;
		m_iForceSelectedItemIdentifier = 0;
		m_bInstantEquip = _bInstant;
		return true;
	}
	else
	{
		// Find item and see if it's equippable (has any ammo/items left).
		int iForcedItemIndex = FindItemIndexByIdentifier(_Identifier);
		CRPG_Object_Item* pForcedItem = GetItemByIndex(iForcedItemIndex);
		if ((pForcedItem == NULL) || (!pForcedItem->IsEquippable(_pChar)) || (!pForcedItem->IsActivatable(_pChar,0)))
			return false;

		m_iForceSelectedItemType = pForcedItem->m_iItemType;
		m_iForceSelectedItemAction = FORCESELECTION_ACTIVATE;
		m_iForceSelectedItemIdentifier = pForcedItem->m_Identifier;
		m_bInstantEquip = _bInstant;
		return true;
	}
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory::GetFinalSelectedItemType()
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetFinalSelectedItemType, 0);
	if (m_iForceSelectedItemType != RPG_ITEMTYPE_UNDEFINED)
	{
		if (m_iForceSelectedItemType == RPG_ITEMTYPE_EMPTY)
			return RPG_ITEMTYPE_UNDEFINED;
		else
			return m_iForceSelectedItemType;
	}
	else
		return m_iSelectedItemType;
}

int CRPG_Object_Inventory::GetFinalSelectedItemIdentifier()
{
	MAUTOSTRIP(CRPG_Object_Inventory_GetFinalSelectedItemType, 0);
	if (m_iForceSelectedItemType != RPG_ITEMTYPE_UNDEFINED)
	{
		if (m_iForceSelectedItemType == RPG_ITEMTYPE_EMPTY)
			return RPG_ITEMTYPE_UNDEFINED;
		else
			return m_iForceSelectedItemIdentifier;
	}
	else
		return m_iSelectedItemIdentifier;
}

//-------------------------------------------------------------------

void CRPG_Object_Inventory::OnUpdateModel()
{
	MAUTOSTRIP(CRPG_Object_Inventory_OnUpdateModel, MAUTOSTRIP_VOID);
	m_bShareAnimation = false; // At this point, animation sharing has already taken place, so we don't allow it anymore.
}

//-------------------------------------------------------------------

void CRPG_Object_Inventory::ForceSetEquipped(int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Inventory_ForceSetEquipped, MAUTOSTRIP_VOID);
	// Force set an item as equipped
	int iFinalWantedIdentifier = GetFinalSelectedItemIdentifier();
	int iFinalWantedItemIndex = FindItemIndexByIdentifier(iFinalWantedIdentifier);
	if (m_EquippedItemIdentifier != iFinalWantedIdentifier)
	{
		// FIXME: In case item was force selected, we have to deactivate it here. This is not nice either. Fix för fan.
		CRPG_Object_Item* pItem = GetItemByIndex(iFinalWantedItemIndex);
		if (pItem)
		{
			ConOut(CStrF("Force Equipping item - Ident: %d",pItem->m_Identifier));
			pItem->m_Flags |= RPG_ITEM_FLAGS_EQUIPPED;
			m_EquippedItemIdentifier = iFinalWantedIdentifier;
			SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, pItem->m_iItemType >> 8, m_pWServer->GetGameTick() + 1,pItem->m_AnimGripType);
			CAutoVar_WeaponStatus Status;
			Status.m_CurrentTimeOut = pItem->m_iExtraActivationWait > GetGameTick(_iObject) ? pItem->m_iExtraActivationWait : 0;
			Status.m_FireTimeOut = pItem->m_FireTimeout;
			int16 iSound = pItem->m_iSound_Cast_Player > 0 ? pItem->m_iSound_Cast_Player : pItem->m_iSound_Cast;
			if (pItem->m_Flags2 & RPG_ITEM_FLAGS2_IAMACLONE)
			{
				iSound = pItem->m_iSound_Cast_LeftItem_Player > 0 ? pItem->m_iSound_Cast_LeftItem_Player : iSound;
			}
			Status.m_iSoundActivatePlayer = iSound;
			Status.m_AmmoLoad = pItem->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS ? pItem->GetAmmoDraw() : pItem->GetAmmo(NULL,0);
			Status.m_Flags = Status.m_iSoundActivatePlayer != -1 ? CAutoVar_WeaponStatus::WEAPONSTATUS_HASSOUND : 0;
			Status.m_Flags |= pItem->m_Flags & RPG_ITEM_FLAGS_SINGLESHOT ? CAutoVar_WeaponStatus::WEAPONSTATUS_SINGLESHOT : 0;
			Status.m_Flags |= pItem->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS ? CAutoVar_WeaponStatus::WEAPONSTATUS_DARKNESSDRAIN : 0;
			Status.m_Flags |= pItem->IsMeleeWeapon() ? CAutoVar_WeaponStatus::WEAPONSTATUS_MELEEWEAPON : 0;
			SendMsg(_iObject, OBJMSG_CHAR_SETWEAPONAG2,pItem->m_iAnimGraph, pItem->m_iItemType >> 8 > 0 ? 1 : 0,-1,0,0,0,&Status,sizeof(CAutoVar_WeaponStatus));
		}
	}
}

void CRPG_Object_Inventory::ForceSetUnequipped(int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Inventory_ForceSetEquipped, MAUTOSTRIP_VOID);
	// Force set an item as equipped
	CRPG_Object_Item* pItem = FindItemByIdentifier(m_EquippedItemIdentifier);
	if (pItem)
	{
		pItem->m_Flags &= ~RPG_ITEM_FLAGS_EQUIPPED;
		SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, pItem->m_iItemType >> 8, m_pWServer->GetGameTick() + 1);
		SendMsg(_iObject, OBJMSG_CHAR_SETWEAPONAG2,0, pItem->m_iItemType >> 8 > 0 ? 1 : 0);
		m_LastEquippedItemIdentifier = pItem->m_Identifier;
		m_iSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemIdentifier = 0;
		m_iSelectedItemIdentifier = 0;
		m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemAction = FORCESELECTION_NONE;
		m_iForceSelectedOtherInventory = -1;
	}
}

//-------------------------------------------------------------------

void CRPG_Object_Inventory::PendingUpdate(CRPG_Object* _pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Inventory_PendingUpdate, MAUTOSTRIP_VOID);
	bool bLog = false;

	bool bHold = (m_iForceSelectedItemType != RPG_ITEMTYPE_UNDEFINED);

	// Find out what itemtype that should be equipped.
	int iFinalWantedItemType = GetFinalSelectedItemType();
	int iFinalWantedIdentifier = GetFinalSelectedItemIdentifier();

	// Get corresponding item index.
	int iFinalWantedItemIndex = FindItemIndexByIdentifier(iFinalWantedIdentifier);

	// See if that item is actually equipped.
	if (m_EquippedItemIdentifier != iFinalWantedIdentifier)
	{
		// Both/Either m_iEquippedItemIndex and/or iFinalWantedItemIndex may be undefined (-1). GetItem will return NULL in such case.
		CRPG_Object_Item* pEquippedItem = FindItemByIdentifier(m_EquippedItemIdentifier);
		if (pEquippedItem != NULL)
		{
			m_bShareAnimation = true; // At this point, we allow animation sharing.

//			CRPG_Object_Item* pSucceedingItem = GetItemByIndex(iFinalWantedItemIndex);
			m_LastEquippedItemIdentifier = 0;

			// Unequip unwanted item.
			if (pEquippedItem->Unequip(_iObject, _pRoot, m_bInstantEquip, bHold))
			{
				if (bLog)
					ConOut(CStrF("Unequipping '%s' (index=%d)", pEquippedItem->m_Name.Str(), m_EquippedItemIdentifier));

				//if (iFinalWantedItemIndex == -1)
					m_LastEquippedItemIdentifier = m_EquippedItemIdentifier;
				
				pEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_USEABLE;
				m_EquippedItemIdentifier = 0;
			}

			if ((m_iForceSelectedOtherInventory != -1) && (m_iForceSelectedItemAction == FORCESELECTION_NONE))
			{
				CRPG_Object_Char* pChar = safe_cast<CRPG_Object_Char>(_pRoot);
				if (pChar != NULL)
				{
					CRPG_Object_Inventory* pInventory = pChar->GetInventory(m_iForceSelectedOtherInventory);
					if (pInventory != NULL)
					{
						pInventory->ForceSelectItem(RPG_ITEMTYPE_UNDEFINED, pChar);
						m_iForceSelectedOtherInventory = -1;
					}
				}
			}
		}
		else
		{
			// Equip wanted item.
			CRPG_Object_Item* pItem = GetItemByIndex(iFinalWantedItemIndex);
			if (pItem != NULL)
			{
				CRPG_Object_Item* pLastEquippedItem = FindItemByIdentifier(m_LastEquippedItemIdentifier);
				if (pLastEquippedItem != NULL)
				{
					// FIXME: In case item was force selected, we have to deactivate it here. This is not nice either. Fix för fan.
					pLastEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED;

					// See if last equipped item has pending removal.
					if (pLastEquippedItem->m_Flags & RPG_ITEM_FLAGS_REMOVED)
					{
						if (bLog)
							ConOut(CStrF("Removing '%s' (index=%d)", pLastEquippedItem->m_Name.Str(), m_LastEquippedItemIdentifier));

						// Store wanted item type (to refind it's index later).
						iFinalWantedItemType = GetItemType(iFinalWantedItemIndex);

						RemoveItemByIdentifier(_iObject, m_LastEquippedItemIdentifier);

						m_LastEquippedItemIdentifier = 0;
						pLastEquippedItem = NULL;

						// Refind index, since it may have been shifted (this should mayby be done by RemoveItemByIndex, via a parameter).
						iFinalWantedItemIndex = FindItemIndex(iFinalWantedItemType);
					}
				}

				/*if (m_bShareAnimation && (iPreceedingEquipAnim != -1) && (pItem->m_iAnimEquip != iPreceedingEquipAnim))
				{
					m_bShareAnimation = false;
					pItem->ApplyWait(_iObject, pItem->m_SharedEquipAnimMidTicks, 0, 0);
					return;
				}*/

				if (pItem->Equip(_iObject, _pRoot, m_bInstantEquip, bHold))
				{
					if (bLog)
						ConOut(CStrF("Equipping '%s' (index=%d)", pItem->m_Name.Str(), iFinalWantedItemIndex));

					if (iFinalWantedIdentifier != m_LastEquippedItemIdentifier)
					{
						if (pItem->IsEquippable(_pRoot))
							pItem->m_Flags |= RPG_ITEM_FLAGS_EQUIPPED;

						if (pLastEquippedItem != NULL)
							pLastEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_EQUIPPED;

						m_LastEquippedItemIdentifier = iFinalWantedIdentifier;
					}
					else
					{
						// Why is this needed? Can't remember. Find out!
						SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, pItem->m_iItemType >> 8, m_pWServer->GetGameTick() + 1,pItem->m_AnimGripType);
					}
					
					m_EquippedItemIdentifier = iFinalWantedIdentifier;
				}
			}
			else
			{
				// See if last equipped item has pending removal.
				CRPG_Object_Item* pLastEquippedItem = GetItemByIndex(m_LastEquippedItemIdentifier);
				if (pLastEquippedItem != NULL)
				{
					// FIXME: In case item was force selected, we have to deactivate it here. This is not nice either. Fix för fan.
					pLastEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED;

					if (pLastEquippedItem->m_Flags & RPG_ITEM_FLAGS_REMOVED)
					{
						RemoveItemByIndex(_iObject, m_LastEquippedItemIdentifier);

						m_LastEquippedItemIdentifier = 0;
						pLastEquippedItem = NULL;

						if (bLog)
							ConOut(CStrF("Removing '%s' (index=%d) (no new item)", pLastEquippedItem->m_Name.Str(), m_LastEquippedItemIdentifier));
					}

				}
			}
		}
	}
	else
	{
/*
		CRPG_Object_Item* pLastEquippedItem = GetItemByIndex(m_iLastEquippedItemIndex);
		if (pLastEquippedItem != NULL)
		{
			// FIXME: In case item was force selected, we have to deactivate it here. This is not nice either. Fix för fan.
			pLastEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED;
		}
*/

		// Test only needed for debug!
//		if (m_bInstantEquip)
//			m_bInstantEquip = false;

		CRPG_Object_Item* pEquippedItem = GetItemByIndex(iFinalWantedItemIndex);
		if (pEquippedItem != NULL)
		{
			if (!(pEquippedItem->m_Flags & RPG_ITEM_FLAGS_USEABLE))
				pEquippedItem->m_Flags |= RPG_ITEM_FLAGS_USEABLE;

			// Wanted item is equipped, but should it also be force activated?
			if ((iFinalWantedItemType == m_iForceSelectedItemType) && (m_iForceSelectedItemAction == FORCESELECTION_ACTIVATE))
			{
				// FIXME: This is identical to the code in Char::ActivateItem, so it should go through some abstracted function instead.
				// Mayby that function should be Item::Activate, but the we have to go through all activate functions to follow the convention.
				pEquippedItem->m_iLastActivator = _iObject; // Should be moved to CRPG_Object_Item::Activate, somehow.
				pEquippedItem->m_Flags |= RPG_ITEM_FLAGS_ACTIVATED; // This too...

				CMat4Dfp32 Mat;
				CWObject_Message Msg(OBJMSG_AIQUERY_GETACTIVATEPOSITION);
				Msg.m_pData = &Mat;
				Msg.m_DataSize = sizeof(CMat4Dfp32);
				if (!m_pWServer->Message_SendToObject(Msg, _iObject))
					Mat.Unit();

//	Error("PendingUpdate", "Oh this code is used...  better fixed it then");
			
				pEquippedItem->Activate(Mat, _pRoot, _iObject, 0);

				// We've activated the force activated item, now it should be unequipped and the selected item reequipped.
				m_iForceSelectedItemAction = FORCESELECTION_REMOVE;
			}
			else if (m_iForceSelectedItemAction == FORCESELECTION_REMOVE)
			{
				// We can't deactivate item here, since it is still equipped and may be "action2" activated AGAIN.
				m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
				m_iForceSelectedItemAction = FORCESELECTION_NONE;
			}

		}
		else
		{
			CRPG_Object_Item* pLastEquippedItem = GetItemByIndex(m_LastEquippedItemIdentifier);
			if (pLastEquippedItem != NULL)
			{
				// FIXME: In case item was force selected, we have to deactivate it here. This is not nice either. Fix för fan.
				pLastEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED;
			}

			m_LastEquippedItemIdentifier = -1;
			pLastEquippedItem = NULL;
		}
	}
}

//-------------------------------------------------------------------

