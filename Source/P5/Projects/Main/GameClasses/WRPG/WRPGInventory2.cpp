#include "PCH.h"

#include "WRPGChar2.h"
#include "WRPGInventory2.h"
#include "WRPGItem.h"
#include "WRPGAmmo.h"
#include "../WObj_Char.h"

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Inventory2, CRPG_Object);

//-------------------------------------------------------------------

int CRPG_Object_Inventory2::GetGameTick(int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_GetGameTick, 0);
	int iTick = SendMsg(_iObject, OBJMSG_CHAR_GETGAMETICK);
	if (iTick == 0)
		iTick = m_pWServer->GetGameTick();

	return iTick;
}

//-------------------------------------------------------------------

void CRPG_Object_Inventory2::UpdateItems(CRPG_Object_Char2 &_Char)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_UpdateItems, MAUTOSTRIP_VOID);
	for(int i = 0; i < GetNumItems(); i++)
		GetItemByIndex(i)->UpdateAmmoList(&_Char);
}

//-------------------------------------------------------------------

void CRPG_Object_Inventory2::ActivateItems(CRPG_Object_Char2 &_Char, CRPG_Object_Item2 *_pPrimaryItem)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_ActivateItems, MAUTOSTRIP_VOID);
	if(_pPrimaryItem)
	{
		for(int i = 0; i < GetNumItems(); i++)
		{
			CRPG_Object_Item2 *pItem = GetItemByIndex(i);
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

void CRPG_Object_Inventory2::DeactivateItems(const CMat43fp32 &_Mat, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_DeactivateItems, MAUTOSTRIP_VOID);
	for(int i = 0; i < GetNumItems(); i++)
	{
		CRPG_Object_Item2 *pItem = GetItemByIndex(i);

		if (pItem->m_Flags & RPG_ITEM_FLAGS_ACTIVATED)
		{
			pItem->Deactivate(_Mat, this, _iObject, _Input);
			pItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED;
		}
	}
}
//-------------------------------------------------------------------

void CRPG_Object_Inventory2::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_Inventory2_OnCreate, MAUTOSTRIP_VOID);
	m_iEquippedItemIndex = -1;
	m_iLastEquippedItemIndex = -1;
	m_iSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
	m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
	m_iForceSelectedItemAction = FORCESELECTION_NONE;
	m_bInstantEquip = false;
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory2::OnProcess(CRPG_Object* _pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_OnProcess, false);
	CRPG_Object_Item2* pEquippedItem = GetEquippedItem();
	if (pEquippedItem == NULL)
		return false;

	return pEquippedItem->OnProcess(_pRoot, _iObject);
}

//-------------------------------------------------------------------

void CRPG_Object_Inventory2::OnCharDeath(int _iObject, CRPG_Object* _pRoot)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_OnCharDeath, MAUTOSTRIP_VOID);
	for(int i = 0; i < GetNumItems(); i++)
		GetItemByIndex(i)->OnCharDeath(_iObject, _pRoot);
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory2::AddItem(int _iObject, CRPG_Object_Item2* _pItem)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_AddItem, false);
	return (AddChild(_pItem) > -1);
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory2::RemoveItemByIndex(int _iObject, int _iItemIndex, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_RemoveItemByIndex, false);
	CRPG_Object_Item2* pItem = GetItemByIndex(_iItemIndex);
	if (pItem == NULL)
		return false;

	// FIXME: Mayby we need to get a real pChar here, instead of cheating with NULL =).
	CRPG_Object_Char2* pChar = NULL;

	int iEquippedItemType = GetItemType(m_iEquippedItemIndex);

	if (m_iEquippedItemIndex == _iItemIndex)
	{
		// Item to be removed is currently equipped. We then have to unequip it first.

		// Flag it as removed, so that this function is called later, when the item has been unequipped.
		pItem->m_Flags |= RPG_ITEM_FLAGS_REMOVED | (_bRemoveInstant ? RPG_ITEM_FLAGS_REMOVEINSTANT : 0);

		// Try to reselect same itemtype (removed item may have been replaced).
		// If current item should be/has been dropped we may have a suitably selected item as well. I haven't the faintest clue if this is the correct way to go but wtf...
		if ((pItem->m_iItemType == m_iSelectedItemType) || (pItem->m_Flags & RPG_ITEM_FLAGS_DROPONUNEQUIP))
			SelectItem(m_iSelectedItemType, pChar, m_bInstantEquip); 
		else
			SelectItem(RPG_ITEMTYPE_UNDEFINED, pChar, true); // Else select nothing.


		if(_bRemoveInstant)
			m_iEquippedItemIndex = -1;

		DelChild(_iItemIndex);
		return false;
	}

	if (pItem->m_iItemType == m_iSelectedItemType)
		SelectItem(m_iSelectedItemType, pChar, m_bInstantEquip); // Try to reselect same itemtype (removed item may have been replaced).
		
	if (pItem->m_iItemType == m_iForceSelectedItemType)
	{
		m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemAction = FORCESELECTION_NONE;
	}

	DelChild(_iItemIndex);

	// Find equipped item index, in case index has been shifted.
	if (iEquippedItemType != RPG_ITEMTYPE_UNDEFINED)
		m_iEquippedItemIndex = FindItemIndex(iEquippedItemType);

	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory2::RemoveItemByName(int _iObject, const char* _pName, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_RemoveItemByName, false);
	return RemoveItemByIndex(_iObject, FindItemIndex(_pName), _bRemoveInstant);
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory2::RemoveItemByType(int _iObject, int _iItemType, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_RemoveItemByType, false);
	return RemoveItemByIndex(_iObject, FindItemIndex(_iItemType), _bRemoveInstant);
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory2::ReplaceItemByType(int _iObject, CRPG_Object_Item2* _pNewItem, bool _bInstantly)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_ReplaceItemByType, false);
	if (_pNewItem == NULL)
		return false;

	int iOldItemIndex = FindItemIndex(_pNewItem->m_iItemType);

	if (iOldItemIndex != -1)
	{
		// Old item in conflict over this itemtype. Replace it nicely.
		if (m_iEquippedItemIndex == iOldItemIndex)
		{

			// Old item was equipped.
			CRPG_Object_Item2* pOldItem = GetItemByIndex(iOldItemIndex);
			if (pOldItem != NULL)
			{
//				if (_bInstantly)
				{
					// Replicating equipped flag.
					_pNewItem->m_Flags |= pOldItem->m_Flags & RPG_ITEM_FLAGS_EQUIPPED;

					// Replace instantly
					SetChild(iOldItemIndex, _pNewItem);
				}
			}
			else
			{
				SetChild(iOldItemIndex, _pNewItem);
			}
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

//-------------------------------------------------------------------

int CRPG_Object_Inventory2::GetItemType(int _iItemIndex)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_GetItemType, 0);
	CRPG_Object_Item2* pItem = GetItemByIndex(_iItemIndex);
	if (pItem == NULL)
		return RPG_ITEMTYPE_UNDEFINED;

	return pItem->m_iItemType;
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory2::FindItemIndex(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_FindItemIndex, 0);
	if (_iItemType == RPG_ITEMTYPE_UNDEFINED)
		return -1;

	for (int iChild = 0; iChild < GetNumItems(); iChild++)
		if ((GetItemByIndex(iChild)->m_iItemType == _iItemType) && 
			!(GetItemByIndex(iChild)->m_Flags & RPG_ITEM_FLAGS_REMOVED))
			return iChild;

	return -1;
}

bool CRPG_Object_Inventory2::CanPickup(int32 _iItemType)
{
	for (int iChild = 0; iChild < GetNumItems(); iChild++)
	{
		CRPG_Object_Item2* pItem = GetItemByIndex(iChild);
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

int CRPG_Object_Inventory2::FindItemIndex(const char* _pName)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_FindItemIndex_2, 0);
	for (int iChild = 0; iChild < GetNumItems(); iChild++)
		if ((GetItemByIndex(iChild)->m_Name.CompareNoCase(_pName) == 0) && 
			!(GetItemByIndex(iChild)->m_Flags & RPG_ITEM_FLAGS_REMOVED))
			return iChild;

	return -1;
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory2::GetNumItems()
{
	MAUTOSTRIP(CRPG_Object_Inventory2_GetNumItems, 0);
	return GetNumChildren();
}

//-------------------------------------------------------------------

CRPG_Object_Item2* CRPG_Object_Inventory2::GetItemByIndex(int _iItemIndex)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_GetItemByIndex, NULL);
	if ((_iItemIndex < 0) || (_iItemIndex >= GetNumItems()))
		return NULL;

	return (CRPG_Object_Item2*)GetChild(_iItemIndex);
}

//-------------------------------------------------------------------

CRPG_Object_Item2* CRPG_Object_Inventory2::FindItemByName(const char* _pName)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_FindItemByName, NULL);
	return GetItemByIndex(FindItemIndex(_pName));
}

//-------------------------------------------------------------------

CRPG_Object_Item2* CRPG_Object_Inventory2::FindItemByType(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_FindItemByType, NULL);
	return GetItemByIndex(FindItemIndex(_iItemType));
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory2::GetFinalSelectedItemIndex()
{
	// Get corresponding item index from item type we have equipped or is trying to equip
	return FindItemIndex(GetFinalSelectedItemType());
};

//-------------------------------------------------------------------

int CRPG_Object_Inventory2::GetNextValidSelectedItemIndex(int _iIndex)
{
	//Check each item after the given. 
	//Currently we allow all non-removed items, but if we wanted we might for example exclude 
	//firearms without ammo etc (if we did that we should still allow one item with a flashlight though)
	int iNext;
	CRPG_Object_Item2 * pNext;
	for (int i = 1; i < GetNumItems(); i++)
	{
		iNext = (_iIndex + i) % GetNumItems();
		pNext = GetItemByIndex(iNext);
		if (pNext && !(pNext->m_Flags & RPG_ITEM_FLAGS_REMOVED))
		{
			//Valid next item found
			return iNext;
		}
	}
	//Couldn't find any other valid items.
	return _iIndex;
};

//-------------------------------------------------------------------

CRPG_Object_Item2* CRPG_Object_Inventory2::GetEquippedItem()
{
	MAUTOSTRIP(CRPG_Object_Inventory2_GetEquippedItem, NULL);
	return GetItemByIndex(m_iEquippedItemIndex);
}

//-------------------------------------------------------------------

CRPG_Object_Item2* CRPG_Object_Inventory2::FindFinalSelectedItem()
{
	MAUTOSTRIP(CRPG_Object_Inventory2_FindFinalSelectedItem, NULL);
	return FindItemByType(GetFinalSelectedItemType());
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory2::GetEquippedItemType()
{
	MAUTOSTRIP(CRPG_Object_Inventory2_GetEquippedItemType, 0);
	return GetItemType(m_iEquippedItemIndex);
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory2::GetSelectedItemType()
{
	MAUTOSTRIP(CRPG_Object_Inventory2_GetSelectedItemType, 0);
	return m_iSelectedItemType;
}

//-------------------------------------------------------------------

CRPG_Object_Item2* CRPG_Object_Inventory2::GetCycledItem(int _NumTries, bool _bReverse)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_GetCycledItem, NULL);
	if (m_iForceSelectedItemType != RPG_ITEMTYPE_UNDEFINED)
		return NULL;

	if (GetNumItems() == 0)
		return NULL;

	int iSelectedItemIndex = FindItemIndex(m_iSelectedItemType);

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

	CRPG_Object_Item2* pNewItem = GetItemByIndex(iSelectedItemIndex);
	if (pNewItem == NULL)
		return NULL;

	return pNewItem;
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory2::SelectItem(int _iItemType, CRPG_Object_Char2* _pChar, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_SelectItem, false);

	// Find out if we have any item of this type.
	if ((_iItemType != RPG_ITEMTYPE_UNDEFINED) && (FindItemIndex(_iItemType) == -1))
	{
		// If we don't have such an item, but such an item is selected, select nothing.
		if (m_iSelectedItemType == _iItemType)
			m_iSelectedItemType = RPG_ITEMTYPE_UNDEFINED;

		return false;
	}

	m_iSelectedItemType = _iItemType;

	if (m_iForceSelectedItemType == RPG_ITEMTYPE_UNDEFINED)
		m_bInstantEquip = _bInstant;

	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory2::ForceSelectItem(int _iItemType, CRPG_Object_Char2* _pChar, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_ForceSelectItem, false);
	if (_iItemType == RPG_ITEMTYPE_UNDEFINED)
	{
		m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemAction = FORCESELECTION_NONE;
		m_bInstantEquip = _bInstant;
		return true;
	}
	else
	{
		if (_iItemType != RPG_ITEMTYPE_EMPTY)
		{
			// Find item and see if it's equippable (has any ammo/items left).
			int iForcedItemIndex = FindItemIndex(_iItemType);
			CRPG_Object_Item2* pForcedItem = GetItemByIndex(iForcedItemIndex);
			if ((pForcedItem == NULL) || (!pForcedItem->IsEquippable(_pChar)))
				return false;
		}

		m_iForceSelectedItemType = _iItemType;
		m_iForceSelectedItemAction = FORCESELECTION_WAITACTIVATE;
		m_bInstantEquip = _bInstant;
		return true;
	}
}

//-------------------------------------------------------------------

bool CRPG_Object_Inventory2::ForceActivateItem(int _iItemType, CRPG_Object_Char2* _pChar, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_ForceActivateItem, false);
	if (_iItemType == RPG_ITEMTYPE_UNDEFINED)
	{
		m_iForceSelectedItemType = RPG_ITEMTYPE_UNDEFINED;
		m_iForceSelectedItemAction = FORCESELECTION_NONE;
		m_bInstantEquip = _bInstant;
		return true;
	}
	else
	{
		if (_iItemType != RPG_ITEMTYPE_EMPTY)
		{
			// Find item and see if it's equippable (has any ammo/items left).
			int iForcedItemIndex = FindItemIndex(_iItemType);
			CRPG_Object_Item2* pForcedItem = GetItemByIndex(iForcedItemIndex);
			if ((pForcedItem == NULL) || (!pForcedItem->IsEquippable(_pChar)) || (!pForcedItem->IsActivatable(_pChar,0)))
				return false;
		}

		m_iForceSelectedItemType = _iItemType;
		m_iForceSelectedItemAction = FORCESELECTION_ACTIVATE;
		m_bInstantEquip = _bInstant;
		return true;
	}
}

//-------------------------------------------------------------------

int CRPG_Object_Inventory2::GetFinalSelectedItemType()
{
	MAUTOSTRIP(CRPG_Object_Inventory2_GetFinalSelectedItemType, 0);
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

//-------------------------------------------------------------------

void CRPG_Object_Inventory2::OnUpdateModel()
{
	MAUTOSTRIP(CRPG_Object_Inventory2_OnUpdateModel, MAUTOSTRIP_VOID);
}

//-------------------------------------------------------------------

void CRPG_Object_Inventory2::PendingUpdate(CRPG_Object* _pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Inventory2_PendingUpdate, MAUTOSTRIP_VOID);
	bool bLog = false;

	bool bHold = (m_iForceSelectedItemType != RPG_ITEMTYPE_UNDEFINED);

	// Find out what itemtype that should be equipped.
	int iFinalWantedItemType = GetFinalSelectedItemType();

	// Get corresponding item index.
	int iFinalWantedItemIndex = FindItemIndex(iFinalWantedItemType);

	// See if that item is actually equipped.
	if (m_iEquippedItemIndex != iFinalWantedItemIndex)
	{
		// Both/Either m_iEquippedItemIndex and/or iFinalWantedItemIndex may be undefined (-1). GetItem will return NULL in such case.
		CRPG_Object_Item2* pEquippedItem = GetItemByIndex(m_iEquippedItemIndex);
		if (pEquippedItem != NULL)
		{
			int iSucceedingEquipAnim = -1;
			CRPG_Object_Item2* pSucceedingItem = GetItemByIndex(iFinalWantedItemIndex);
			if (pSucceedingItem != NULL)
				iSucceedingEquipAnim = pSucceedingItem->m_iAnimEquip;
			else
				m_iLastEquippedItemIndex = -1;

			// Unequip unwanted item.
			if (pEquippedItem->Unequip(_iObject, _pRoot, iSucceedingEquipAnim, m_bInstantEquip, bHold))
			{
				if (bLog)
					ConOut(CStrF("Unequipping '%s' (index=%d)", pEquippedItem->m_Name.Str(), m_iEquippedItemIndex));

				if (iFinalWantedItemIndex == -1)
					m_iLastEquippedItemIndex = m_iEquippedItemIndex;
				
				pEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_USEABLE;
				m_iEquippedItemIndex = -1;
			}
		}
		else
		{
			// Equip wanted item.
			CRPG_Object_Item2* pItem = GetItemByIndex(iFinalWantedItemIndex);
			if (pItem != NULL)
			{
				int iPreceedingEquipAnim = -1;
				CRPG_Object_Item2* pLastEquippedItem = GetItemByIndex(m_iLastEquippedItemIndex);
				if (pLastEquippedItem != NULL)
				{
					// FIXME: In case item was force selected, we have to deactivate it here. This is not nice either. Fix för fan.
					pLastEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED;

					iPreceedingEquipAnim = pLastEquippedItem->m_iAnimEquip;

					// See if last equipped item has pending removal.
					if (pLastEquippedItem->m_Flags & RPG_ITEM_FLAGS_REMOVED)
					{
						if (bLog)
							ConOut(CStrF("Removing '%s' (index=%d)", pLastEquippedItem->m_Name.Str(), m_iLastEquippedItemIndex));

						// Store wanted item type (to refind it's index later).
						iFinalWantedItemType = GetItemType(iFinalWantedItemIndex);

						RemoveItemByIndex(_iObject, m_iLastEquippedItemIndex);

						m_iLastEquippedItemIndex = -1;
						pLastEquippedItem = NULL;

						// Refind index, since it may have been shifted (this should mayby be done by RemoveItemByIndex, via a parameter).
						iFinalWantedItemIndex = FindItemIndex(iFinalWantedItemType);
					}
				}

				if (pItem->Equip(_iObject, _pRoot, iPreceedingEquipAnim, m_bInstantEquip, bHold))
				{
					if (bLog)
						ConOut(CStrF("Equipping '%s' (index=%d)", pItem->m_Name.Str(), iFinalWantedItemIndex));

					if (iFinalWantedItemIndex != m_iLastEquippedItemIndex)
					{
						if (pItem->IsEquippable(_pRoot))
							pItem->m_Flags |= RPG_ITEM_FLAGS_EQUIPPED;

						if (pLastEquippedItem != NULL)
							pLastEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_EQUIPPED;

						m_iLastEquippedItemIndex = iFinalWantedItemIndex;
					}
					else
					{
						// Why is this needed? Can't remember. Find out!
						SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, pItem->m_iItemType >> 8, m_pWServer->GetGameTick() + 1);
					}
					
					m_iEquippedItemIndex = iFinalWantedItemIndex;
				}
			}
			else
			{
				// See if last equipped item has pending removal.
				CRPG_Object_Item2* pLastEquippedItem = GetItemByIndex(m_iLastEquippedItemIndex);
				if (pLastEquippedItem != NULL)
				{
					// FIXME: In case item was force selected, we have to deactivate it here. This is not nice either. Fix för fan.
					pLastEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED;

					if (pLastEquippedItem->m_Flags & RPG_ITEM_FLAGS_REMOVED)
					{
						RemoveItemByIndex(_iObject, m_iLastEquippedItemIndex);

						m_iLastEquippedItemIndex = -1;
						pLastEquippedItem = NULL;

						if (bLog)
							ConOut(CStrF("Removing '%s' (index=%d) (no new item)", pLastEquippedItem->m_Name.Str(), m_iLastEquippedItemIndex));
					}

				}
			}
		}
	}
	else
	{
		CRPG_Object_Item2* pEquippedItem = GetItemByIndex(iFinalWantedItemIndex);
		if (pEquippedItem != NULL)
		{
			if (!(pEquippedItem->m_Flags & RPG_ITEM_FLAGS_USEABLE))
				pEquippedItem->m_Flags |= RPG_ITEM_FLAGS_USEABLE;

			// Wanted item is equipped, but should it also be force activated?
			if ((iFinalWantedItemType == m_iForceSelectedItemType) && (m_iForceSelectedItemAction == FORCESELECTION_ACTIVATE))
			{
				// FIXME: This is identical to the code in Char::ActivateItem, so it should go through some abstracted function instead.
				// Mayby that function should be Item::Activate, but the we have to go through all activate functions to follow the convention.
				pEquippedItem->m_iLastActivator = _iObject; // Should be moved to CRPG_Object_Item2::Activate, somehow.
				pEquippedItem->m_Flags |= RPG_ITEM_FLAGS_ACTIVATED; // This too...

				CMat43fp32 Mat;
				CWObject_Message Msg(OBJMSG_AIQUERY_GETACTIVATEPOSITION);
				Msg.m_pData = &Mat;
				Msg.m_DataSize = sizeof(CMat43fp32);
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
			CRPG_Object_Item2* pLastEquippedItem = GetItemByIndex(m_iLastEquippedItemIndex);
			if (pLastEquippedItem != NULL)
			{
				// FIXME: In case item was force selected, we have to deactivate it here. This is not nice either. Fix för fan.
				pLastEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED;
			}

			m_iLastEquippedItemIndex = -1;
			pLastEquippedItem = NULL;
		}
	}
}

//-------------------------------------------------------------------

