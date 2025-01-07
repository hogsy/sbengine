
#include "PCH.h"

#include "WRPGChar.h"
#include "WRPGAmmo.h"
#include "../WObj_Char.h"

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Char, CRPG_Object);

// -------------------------------------------------------------------
//  CRPG_Object_Char

const char* CRPG_Object_Char::ms_AttribStr[] =
{
	"health", "maxhealth", "mana", "maxmana", "pain", "wait", "soundindex", "soundduration", "aura0", "aura1", "aura2", NULL
};

const char* CRPG_Object_Char::ms_SkillStr[] =
{
	"unarmed", "sword", "axe", "hammer", "crossbow", "bow", "dagger", "magic", "summon", "bomb", NULL
};

bool CRPG_Object_Char::m_bUseDefaultItems = true;
bool CRPG_Object_Char::m_bInsideEquipGUI = false;

void CRPG_Object_Char::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_Char_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object::OnCreate();

	SetNumAttribs(RPG_CHAR_NUMATTRIB);

	Health() = 4;
	MaxHealth() = 4;

	Mana() = 100;
	MaxMana() = 100;

	m_ManaReg_RestTicks = 0;
	m_ManaReg_CurRestTicks = 0;
	m_ManaReg_Amount = 1;
	m_ManaReg_DelayTicks = 1;
	m_ManaReg_CurDelayTicks = 0;

	Wait() = 5;

	for(int i = 0; i < NUMSKILLS; i++)
		m_lSkillLevels[i] = 256;

//	m_iPushedItem = -1;
	
	SoundDuration() = -1;

	SetNumChildren(RPG_CHAR_NUMCHILDREN);

	Child(RPG_CHAR_INVENTORY_WEAPONS) = CreateObject("Inventory");
	Child(RPG_CHAR_INVENTORY_ITEMS) = CreateObject("Inventory");
	if (Child(RPG_CHAR_INVENTORY_ITEMS))
		((CRPG_Object_Inventory*)(CRPG_Object*)Child(RPG_CHAR_INVENTORY_ITEMS))->SetItemCounter(16384);
	Child(RPG_CHAR_INVENTORY_ARMOUR) = CreateObject("Inventory");
	if (Child(RPG_CHAR_INVENTORY_ARMOUR))
		((CRPG_Object_Inventory*)(CRPG_Object*)Child(RPG_CHAR_INVENTORY_ARMOUR))->SetItemCounter(16384*2);
	Child(RPG_CHAR_INVENTORY_QUESTITEMS) = CreateObject("Inventory");
	if (Child(RPG_CHAR_INVENTORY_QUESTITEMS))
		((CRPG_Object_Inventory*)(CRPG_Object*)Child(RPG_CHAR_INVENTORY_QUESTITEMS))->SetItemCounter(16384*3);
	
//	bool OldPrecache = m_bPrecacheForPlayerUse;
//	m_bPrecacheForPlayerUse = true;
//	PickupItem("use_generic", false);
//	m_bPrecacheForPlayerUse = OldPrecache;

	m_HealthRegenerationDelay = 0;
}

void CRPG_Object_Char::OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_Char_OnIncludeClass, MAUTOSTRIP_VOID);
	if(m_bUseDefaultItems)
	{
		for(int i = 0; i < _pReg->GetNumChildren(); i++)
		{
			if(_pReg->GetName(i).Copy(0, 7) == "WEAPON_")
			{
				if(_pReg->GetName(i) == "WEAPON_ADD")
					IncludeRPGClass(_pReg->GetValue(i), _pMapData, _pWServer);
				else
					IncludeRPGClass(_pReg->GetName(i), _pMapData, _pWServer);
			}
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

bool CRPG_Object_Char::PreProcess(CRPG_Object *_pRoot, int)
{
	MAUTOSTRIP(CRPG_Object_Char_PreProcess, false);
	return false;
}

bool CRPG_Object_Char::OnProcess(CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Char_OnProcess, false);
	CRPG_Object::OnProcess(_pRoot, _iObject);

	int iInventory;

	for (iInventory = 0; iInventory < RPG_CHAR_NUMCHILDREN; iInventory++)
	{
		/*
		if ((GetInventory(iInventory)->GetSelectedItemType() == RPG_ITEMTYPE_UNDEFINED) &&
			(GetInventory(iInventory)->GetNumItems() > 0))
		{
			CycleSelectedItem(iInventory);
		}
		*/

		GetInventory(iInventory)->OnProcess(this, _iObject);
	}

	{
		// Decrease pain
		int pain = Pain();
		if (pain)
			Pain() = Max(pain - 3, 0);
	}

/*	{
		// Fade sounds
		int iDur = SoundDuration();
		if (iDur >= 0)
		{
			iDur--;
			SoundDuration() = iDur;
		}
	}

	if (Mana() < MaxMana())
	{
		if (m_ManaReg_CurRestTicks <= 0)
		{
			if (m_ManaReg_CurDelayTicks <= 0)
			{
				GiveMana(m_ManaReg_Amount);
				m_ManaReg_CurDelayTicks = m_ManaReg_DelayTicks;
			}
			else
			{
				m_ManaReg_CurDelayTicks--;
			}
		}
		else
		{
			m_ManaReg_CurRestTicks--;
		}
	}
	else
	{
		m_ManaReg_CurRestTicks = 0;
		m_ManaReg_CurDelayTicks = 0;
	}
*/

	// Clamp health
	if (Health() > MaxHealth())
		Health() = MaxHealth();
//	if (Health().m_Value > MaxHealth().m_Current)
//		Health() = MaxHealth().m_Current;

//	if (Mana().m_Value > MaxMana().m_Current)
//		Mana() = MaxMana().m_Current;

	if (Wait() > 0)
		Wait() = Wait() - 1;

	if (Wait() == 0)
	{
		for (int iInventory = 0; iInventory < RPG_CHAR_NUMCHILDREN; iInventory++)
			GetInventory(iInventory)->PendingUpdate(this, _iObject);
	}
	
	return true;
}

bool CRPG_Object_Char::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_Char_OnEvalKey, false);
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH4('ATTR','IB_H','EALT','H'): // "ATTRIB_HEALTH"
		{
			Health() = KeyValuei;
			break;
		}

	case MHASH4('ATTR','IB_M','AXHE','ALTH'): // "ATTRIB_MAXHEALTH"
		{
			MaxHealth() = KeyValuei;
			break;
		}

	case MHASH4('MANA','REG_','DELA','Y'): // "MANAREG_DELAY"
		{
			m_ManaReg_RestTicks = (int)(KeyValuef * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH3('MANA','REG_','RATE'): // "MANAREG_RATE"
		{
			m_ManaReg_DelayTicks = (int)(KeyValuef * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH4('MANA','REG_','AMOU','NT'): // "MANAREG_AMOUNT"
		{
			m_ManaReg_Amount = (int)(KeyValuef);
			break;
		}

	case MHASH7('HEAL','THRE','COVE','RYDE','LAY_','TICK','S'): // "HEALTHRECOVERYDELAY_TICKS"
		{
			if(KeyValuei == 0)
				m_HealthRegenerationDelay = 0;
			else
				m_HealthRegenerationDelay = (int)(3 * m_pWServer->GetGameTicksPerSecond());//KeyValuei;
			break;
		}

	case MHASH4('EQUI','PPED','_ITE','M'): // "EQUIPPED_ITEM"
		{
			EquipItem(KeyValue);
			break;
		}

	default:
		{
			if(KeyName.CompareSubStr("WEAPON_") == 0 || KeyName.Find("SPELL_") == 0 ||	
				KeyName.CompareSubStr("ITEM_") == 0 || KeyName.Find("ARMOUR_") == 0)
			{
				if(m_bUseDefaultItems)
				{
					if (KeyName.CompareSubStr("WEAPON_ADD") == 0)
						PickupItem(KeyValue, true, false, NULL, 0, true);
					else
						PickupItem(KeyName, true, false, NULL, 0, true);
				}
			}
			else if(KeyName.CompareSubStr("ITEM") == 0)
			{
				if(m_bUseDefaultItems)
					PickupItem(KeyValue, true, false, NULL, 0, true);
			}
			else if(KeyName.CompareSubStr("SKILL_") == 0)
			{
				CStr st = KeyName.Copy(6, 1024);

				int iSkill = st.TranslateInt(ms_SkillStr);
				if(iSkill >= 0 && iSkill < NUMSKILLS)
					m_lSkillLevels[iSkill] = KeyValuei * 256;
			}
			else
				return CRPG_Object::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}

	return true;
}

CRPG_Object_Inventory *CRPG_Object_Char::GetInventory(int _iSlot)
{
	MAUTOSTRIP(CRPG_Object_Char_GetInventory, NULL);
	if ((_iSlot < 0) || (_iSlot >= GetNumChildren()))
		return NULL;

	return (CRPG_Object_Inventory*)GetChild(_iSlot);
}

int CRPG_Object_Char::GetSkillLevel(int _SkillType)
{
	MAUTOSTRIP(CRPG_Object_Char_GetSkillLevel, 0);
	if ((_SkillType >= 0) && (_SkillType < NUMSKILLS))
		return m_lSkillLevels[_SkillType];
	else
		return 0;
}

int CRPG_Object_Char::GetDamageBoost(int _SkillType)
{
	MAUTOSTRIP(CRPG_Object_Char_GetDamageBoost, 0);
	return GetSkillLevel(_SkillType);
}

//----------------------------------------------------------------------
#if 0
int CRPG_Object_Char::GetManaLeft()
{
	MAUTOSTRIP(CRPG_Object_Char_GetManaLeft, 0);
/*
	CRPG_Object_Ammo *pAmmo = GetAssociatedAmmo(m_pRoot);
	if(!pAmmo)
		return false;
	
	return pAmmo->m_NumItems;
*/

	return Mana();
}

//----------------------------------------------------------------------

bool CRPG_Object_Char::DrawMana(int _amount)
{
	MAUTOSTRIP(CRPG_Object_Char_DrawMana, false);
/*
	CRPG_Object_Ammo *pAmmo = GetAssociatedAmmo(m_pRoot);
	if(!pAmmo)
		return false;
	return pAmmo->ConsumeFromTotal(_amount);
*/

	if (_amount == -1)
		return false;

	if (Mana() < _amount)
		return false;

	m_ManaReg_CurRestTicks = m_ManaReg_RestTicks;
	Mana() -= _amount;
	return true;
}

//----------------------------------------------------------------------

bool CRPG_Object_Char::GiveMana(int _amount)
{
	MAUTOSTRIP(CRPG_Object_Char_GiveMana, false);
/*
	CRPG_Object_Ammo *pAmmo = GetAssociatedAmmo(m_pRoot);
	if(!pAmmo)
		return false;
	return pAmmo->AddToTotal(_amount) != 0;
*/

	Mana() += _amount;

	if (Mana() > MaxMana())
		Mana() = MaxMana();

	return true;
}
#endif 
//----------------------------------------------------------------------
void CRPG_Object_Char::ApplyDamage(int _Damage)
{
	MAUTOSTRIP(CRPG_Object_Char_ApplyDamage, MAUTOSTRIP_VOID);
	m_LastDamageTick = m_pWServer->GetGameTick();
	m_HealthRegenerationDelay = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 2.0f);
	Health() = Health() - _Damage;
	if (Health() > MaxHealth())
	{
		Health() = MaxHealth();
	}
	Pain() = Pain() + 100 * _Damage / MaxHealth();
}

void CRPG_Object_Char::ApplyWait(int _Wait)
{
	MAUTOSTRIP(CRPG_Object_Char_ApplyWait, MAUTOSTRIP_VOID);
	if (_Wait == -1)
		Wait() = 10000; // Lock
	else if (_Wait == 0)
		Wait() = 0; // Unlock
	else if( _Wait < -1)
		Wait() = -_Wait; // Set
	else
		Wait() = Wait() + _Wait; // Increase
}

void CRPG_Object_Char::ApplySound(int _iIndex, int _iDuration)
{
	MAUTOSTRIP(CRPG_Object_Char_ApplySound, MAUTOSTRIP_VOID);
	if(_iDuration > SoundDuration())
	{
		SoundIndex() = _iIndex;
		SoundDuration() = _iDuration;
	}
}

int CRPG_Object_Char::ReceiveHealth(int _Health)
{
	MAUTOSTRIP(CRPG_Object_Char_ReceiveHealth, 0);
	if (_Health < 0)
		m_LastDamageTick = m_pWServer->GetGameTick();

	int Cur = Health();
	// Cannot limit Health() here as this will break min health code that work by adding enough health
	// to stay within min health.
	// Health() = MinMT(Health().m_Value + _Health, MaxHealth());
	Health() = Health() + _Health;

	return Health() - Cur;
}

void CRPG_Object_Char::IncreaseMaxHealth(int _HealthIncrease)
{
	MaxHealth() = MaxHealth() + _HealthIncrease;
	Health() = MaxHealth();
}

void CRPG_Object_Char::OnCharDeath(int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Char_OnCharDeath, MAUTOSTRIP_VOID);
	for (int iInventory = 0; iInventory < RPG_CHAR_NUMCHILDREN; iInventory++)
	{
		GetInventory(iInventory)->OnCharDeath(_iObject, this);
	}
}

bool CRPG_Object_Char::AddItem(int _iObject, int _iSlot, CRPG_Object *_pObj)
{
	MAUTOSTRIP(CRPG_Object_Char_AddItem, false);
	CRPG_Object_Item* pItem = safe_cast<CRPG_Object_Item>(_pObj);
	if (pItem == NULL)
		return false;

/*
	if (pItem->GetType() != CRPG_Object::TYPE_ITEM)
		return false;
*/

	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return false;

	return pInventory->AddItem(_iObject, pItem);
}

void CRPG_Object_Char::ClearReloadFlags(int _iSlot)
{
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (!pInventory)
		return;

	for (int32 i = 0; i < pInventory->GetNumItems(); i++)
	{
		CRPG_Object_Item* pItem = pInventory->GetItemByIndex(i);
		if (pItem)
			pItem->m_Flags2 &= ~RPG_ITEM_FLAGS2_TAGGEDFORRELOAD;
	}
}

bool CRPG_Object_Char::RemoveItemByType(int _iObject, int _iItemType, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Char_RemoveItemByType, false);
	int iSlot = _iItemType >> 8;
	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return false;

	return pInventory->RemoveItemByType(_iObject, _iItemType, _bRemoveInstant);
}

bool CRPG_Object_Char::RemoveItemByIdentifier(int _iObject, int _Identifier, int _iSlot, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Char_RemoveItemByIdentifier, false);
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return false;

	return pInventory->RemoveItemByIdentifier(_iObject, _Identifier, _bRemoveInstant);
}

bool CRPG_Object_Char::RemoveItemByName(int _iObject, int _iSlot, const char* _pName, bool _bRemoveInstant)
{
	MAUTOSTRIP(CRPG_Object_Char_RemoveItemByName, false);
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return false;

	return pInventory->RemoveItemByName(_iObject, _pName, _bRemoveInstant);
}

bool CRPG_Object_Char::UpdateItemIdentifier(CRPG_Object_Item* _pItem)
{
	CRPG_Object_Inventory* pInventory = _pItem ? GetInventory(_pItem->m_iItemType >> 8) : NULL;
	if (!pInventory)
		return false;
	
	return pInventory->UpdateItemIdentifier(_pItem);
}

void CRPG_Object_Char::UpdateItemIdentifiers()
{
	for (int32 i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
	{
		CRPG_Object_Inventory* pInventory = GetInventory(i);
		for(int iItemIndex = 0; iItemIndex < pInventory->GetNumItems(); iItemIndex++)
		{
			CRPG_Object_Item* pItem = pInventory->GetItemByIndex(iItemIndex);
			if (pItem)
				pInventory->UpdateItemIdentifier(pItem);
		}
	}
}

CRPG_Object_Item *CRPG_Object_Char::PickupItem(const char* _pName, bool _bAllowMerge, bool _bSelect, TPtr<CRPG_Object_Item>* _pspDropping, int _iObject, bool _bNoSound, int _iSender, bool _bIsPlayer, bool _bNoPickupIcon)
{
	MAUTOSTRIP(CRPG_Object_Char_PickupItem, NULL);
	spCRPG_Object spObj = CreateObject(_pName);
	if (spObj == NULL)
		return NULL;

#ifndef M_RTM
	if(TDynamicCast<CRPG_Object_Item >((CRPG_Object *)spObj) == NULL)
		return NULL;
#endif

	TPtr<CRPG_Object_Item> spItem = (CRPG_Object_Item *)((CRPG_Object *)spObj);
	if (spItem == NULL)
		return NULL;

	if (_bIsPlayer && spItem->m_bNoPlayerPickup)
		return NULL;

	int Res = spItem->OnPickup(_iObject, this, _bNoSound, _iSender, _bNoPickupIcon);
	if(Res == 0)
		return NULL;
	else if(Res == 2)
		return (CRPG_Object_Item *)0x1;

	int iSlot = spItem->m_iItemType >> 8;
	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return NULL;

 	CRPG_Object_Item* pOldItem = FindItemByType(spItem->m_iItemType);

	if (pOldItem == NULL || pOldItem->m_Flags2 & RPG_ITEM_FLAGS2_UNIQUE)
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
			bMerged = pOldItem->MergeItem(_iObject, (CRPG_Object_Item*)spItem);
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
			SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, spItem->m_iItemType >> 8, iTick + 1,spItem->m_AnimGripType);
		}
		
		if ((_pspDropping != NULL) && (!_bAllowMerge || !bMerged))
		{
			*_pspDropping = pOldItem;
			pInventory->ReplaceItemByType(_iObject, spItem);
		}
		else if (spItem->m_bReplaceOnSameType)
		{
			pInventory->ReplaceItemByType(_iObject, spItem);
		}
	
	}

	if (_bSelect)
		SelectItem(spItem->m_iItemType, true, true);
		
	return spItem;
}

CRPG_Object_Item* CRPG_Object_Char::LoadItem(const char *_pName, CCFile* _pFile, int _iObject, bool _bIsPlayer)
{
	MAUTOSTRIP(CRPG_Object_Char_LoadItem, NULL);
	spCRPG_Object spObj = CreateObject(_pName);
	if (spObj == NULL)
		return NULL;

#ifndef M_RTM
	if(TDynamicCast<CRPG_Object_Item >((CRPG_Object *)spObj) == NULL)
		return NULL;
#endif

	TPtr<CRPG_Object_Item> spItem = (CRPG_Object_Item *)((CRPG_Object *)spObj);
	if (spItem == NULL)
		return NULL;

	// Delta load before stuffing in an inventory
	spItem->OnDeltaLoad(_pFile,this);

	int Res = spItem->OnPickup(_iObject, this, true, 0, true);
	if(Res == 0)
		return NULL;
	else if(Res == 2)
		return (CRPG_Object_Item *)0x1;

	int iSlot = spItem->m_iItemType >> 8;
	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return NULL;
	//New unique item
	pInventory->AddItem(_iObject, spItem);
	return spItem;
}

CRPG_Object_Item* CRPG_Object_Char::EquipItem(const char* _pName)
{
	CRPG_Object_Item* pItem = FindItemByName(_pName);
	if (pItem)
		SelectItem(pItem->m_iItemType, true, true);
	return pItem;
}

bool CRPG_Object_Char::IsSelected(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char_IsSelected, false);
	int iSlot = _iItemType >> 8;
	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return false;

	return (pInventory->GetSelectedItemType() == _iItemType);
}

bool CRPG_Object_Char::IsEquipped(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char_IsEquipped, false);
	int iSlot = _iItemType >> 8;
	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return false;

	return (pInventory->GetEquippedItemType() == _iItemType);
}

bool CRPG_Object_Char::HasItem(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char_HasItem, false);
	CRPG_Object_Item* pLinkedItem = FindItemByType(_iItemType);
	return (pLinkedItem != NULL);
}

bool CRPG_Object_Char::IsSelectableItem(CRPG_Object_Item* _pItem, bool _bForceSelected)
{
	MAUTOSTRIP(CRPG_Object_Char_IsSelectableItem, false);
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
//		bool bValid = false;
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

bool CRPG_Object_Char::SelectItem(int _iItemType, bool _bCheckSelectability, bool _bSelectAssociated, int _Method, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Char_SelectItem, false);
	int iSlot = _iItemType >> 8;
	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return false;

	if (_bCheckSelectability)
	{
		CRPG_Object_Item* pItem = FindItemByType(_iItemType);
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

	// If we are selecting an item (i.e. slot 1) we need to be safe from two handed weapons.
	if (iSlot == RPG_CHAR_INVENTORY_ITEMS)
	{
		CRPG_Object_Inventory* pInventory0 = GetInventory(RPG_CHAR_INVENTORY_WEAPONS);
		if (pInventory0 != NULL)
		{
			CRPG_Object_Item* pWeapon = pInventory0->GetEquippedItem();
			if ((pWeapon != NULL) && (pWeapon->m_Flags & RPG_ITEM_FLAGS_TWOHANDED))
			{
				bool bCompatible = false;
				for (int iLink = 0; iLink < pWeapon->m_NumLinks; iLink++)
				{
//					int iLinkedItemType = pWeapon->m_lLinkedItemTypes[iLink];
					if (_iItemType == pWeapon->m_iItemType)
						bCompatible = true;
				}
				if (!bCompatible)
				{
					if (_Method == SELECTIONMETHOD_NORMAL)
					{
						pInventory0->SelectItem(RPG_ITEMTYPE_EMPTY, this, _bInstant);
					}
					else // ForceSelected
					{
						// Set this item's inventory to know that the two handed item has been force unselected in it's inventory.
						pInventory->m_iForceSelectedOtherInventory = 0;
						pInventory0->ForceSelectItem(RPG_ITEMTYPE_EMPTY, this, _bInstant);
					}
				}
			}

			if (pWeapon != NULL)
			{
				for (int iLink = 0; iLink < pWeapon->m_NumLinks; iLink++)
				{
					int iLinkedItemType = pWeapon->m_lLinkedItemTypes[iLink];
					if (iLinkedItemType == _iItemType)
					{
						pWeapon->m_iCurLink = iLink;
						break;
					}
				}
			}
		}
	}

	// Don't allow item selection to select weapons.
	// The design is for items to be secondary, ammo and shields only.
	if ((iSlot == RPG_CHAR_INVENTORY_ITEMS) && (_bSelectAssociated == true))
		_bSelectAssociated = false;

	if (_bSelectAssociated)
		SelectAssociatedItem(_iItemType, true, _Method, _bInstant);

	return true;
}

bool CRPG_Object_Char::SelectItemByIdentifier(int _Identifier, int _iSlot, bool _bCheckSelectability, bool _bSelectAssociated, int _Method, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Char_SelectItem, false);
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return false;

	// Ok then find specific item in the inventory
	CRPG_Object_Item* pItem = pInventory->FindItemByIdentifier(_Identifier);
	if (!pItem)
		return false;

	if (_bCheckSelectability)
	{	
		if (!IsSelectableItem(pItem, _Method != SELECTIONMETHOD_NORMAL))
			return false;
	}

	bool bSelected = false;
	switch (_Method)
	{
	case SELECTIONMETHOD_NORMAL: bSelected = pInventory->SelectItemByIdentifier(_Identifier, this, _bInstant); break;
	case SELECTIONMETHOD_FORCE: bSelected = pInventory->ForceSelectItemByIdentifier(_Identifier, this, _bInstant); break;
	case SELECTIONMETHOD_FORCEACTIVATE: bSelected = pInventory->ForceActivateItemByIdentifier(_Identifier, this, _bInstant); break;
	}

	if (!bSelected)
		return false;

	// If we are selecting an item (i.e. slot 1) we need to be safe from two handed weapons.
	if (_iSlot == RPG_CHAR_INVENTORY_ITEMS)
	{
		CRPG_Object_Inventory* pInventory0 = GetInventory(RPG_CHAR_INVENTORY_WEAPONS);
		if (pInventory0 != NULL)
		{
			CRPG_Object_Item* pWeapon = pInventory0->GetEquippedItem();
			if ((pWeapon != NULL) && (pWeapon->m_Flags & RPG_ITEM_FLAGS_TWOHANDED))
			{
				bool bCompatible = false;
				for (int iLink = 0; iLink < pWeapon->m_NumLinks; iLink++)
				{
//					int iLinkedItemType = pWeapon->m_lLinkedItemTypes[iLink];
					if (pItem->m_iItemType == pWeapon->m_iItemType)
						bCompatible = true;
				}
				if (!bCompatible)
				{
					if (_Method == SELECTIONMETHOD_NORMAL)
					{
						pInventory0->SelectItem(RPG_ITEMTYPE_EMPTY, this, _bInstant);
					}
					else // ForceSelected
					{
						// Set this item's inventory to know that the two handed item has been force unselected in it's inventory.
						pInventory->m_iForceSelectedOtherInventory = 0;
						pInventory0->ForceSelectItem(RPG_ITEMTYPE_EMPTY, this, _bInstant);
					}
				}
			}

			if (pWeapon != NULL)
			{
				for (int iLink = 0; iLink < pWeapon->m_NumLinks; iLink++)
				{
					int iLinkedItemType = pWeapon->m_lLinkedItemTypes[iLink];
					if (iLinkedItemType == pItem->m_iItemType)
					{
						pWeapon->m_iCurLink = iLink;
						break;
					}
				}
			}
		}
	}

	// Don't allow item selection to select weapons.
	// The design is for items to be secondary, ammo and shields only.
	if ((_iSlot == RPG_CHAR_INVENTORY_ITEMS) && (_bSelectAssociated == true))
		_bSelectAssociated = false;

	if (_bSelectAssociated)
		SelectAssociatedItem(pItem->m_iItemType, true, _Method, _bInstant);

	return true;
}

bool CRPG_Object_Char::SelectAssociatedItem(int _iItemType, bool _bCheckSelectability, int _Method, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Char_SelectAssociatedItem, false);
	int iSelectedItemType = GetAssociatedItemType(_iItemType);
	if (iSelectedItemType != RPG_ITEMTYPE_UNDEFINED)
		if (SelectItem(iSelectedItemType, _bCheckSelectability, false, _Method, _bInstant))
			return true;

	// Cycle other inventory if that item has needequippedflag set (or has become invalid in some way).
	int iSelectedSlot = _iItemType >> 8;
	for (int iSlot = 0; iSlot < GetNumChildren(); iSlot++)
	{
		if (iSlot != iSelectedSlot)
		{
			CRPG_Object_Item *pItem = GetInventory(iSlot)->FindFinalSelectedItem();
			if ((pItem != NULL) && !IsSelectableItem(pItem, _Method != SELECTIONMETHOD_NORMAL))
			{
				// We must cycle item, since it can no longer be selected.
				CycleSelectedItem(iSlot, true, false);

				// See if we successfully cycled item away.
				if (GetInventory(iSlot)->GetSelectedItemType() != pItem->m_iItemType)
					return true;

				// We failed to cycle item away, it's still selected.
				// So lets select nothing.
				if (GetInventory(iSlot)->SelectItem(RPG_ITEMTYPE_UNDEFINED, this))
					return true;

				// We failed to unselect item. It's still selected! God is agaionst us!
				return false;
			}
		}
	}

	return false;
}

// Return true if new item now is selected.
bool CRPG_Object_Char::CycleSelectedItem(int _iSlot, bool _bReverse, bool _bSelectAssociated, bool _bInstant)
{
	MAUTOSTRIP(CRPG_Object_Char_CycleSelectedItem, false);
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return false;

	int MaxNumTries = pInventory->GetNumItems();
	int NumTries = 0;

	CRPG_Object_Item* pNewItem;
	do
	{
		pNewItem = pInventory->GetCycledItem(NumTries, _bReverse);
		if (pNewItem == NULL)
			return false;

		if (IsSelectableItem(pNewItem, false))
			return SelectItem(pNewItem->m_iItemType, false, _bSelectAssociated, SELECTIONMETHOD_NORMAL, _bInstant);

		NumTries++;
	}
	while (NumTries < MaxNumTries);

	return false;
}

bool CRPG_Object_Char::QuickActivateItem(CRPG_Object_Item *_pItem, const CMat4Dfp32 &_Mat, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_Char_QuickActivateItem, false);
	_pItem->m_iLastActivator = _iObject;
	if(!_pItem->Activate(_Mat, this, _iObject, _Input))
	{
		return false;
	}
	return true;
}

bool CRPG_Object_Char::ActivateItem(int _iSlot, const CMat4Dfp32 &_Mat, int _iObject, int _Input, bool _bOverride)
{
	MAUTOSTRIP(CRPG_Object_Char_ActivateItem, false);
	CRPG_Object_Item* pEquippedItem = GetFinalSelectedItem(_iSlot);
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
			pEquippedItem->m_iLastActivator = _iObject; // Should be moved to CRPG_Object_Item::Activate, somehow.
			pEquippedItem->m_Flags |= RPG_ITEM_FLAGS_ACTIVATED; // This too...
			pEquippedItem->Activate(_Mat, this, _iObject, _Input);
		}
	}

	return true; // Mondelore
}

bool CRPG_Object_Char::DeactivateItem(int _iSlot, const CMat4Dfp32 &_Mat, int _iObject ,int _Input)
{
	MAUTOSTRIP(CRPG_Object_Char_DeactivateItem, false);
	CRPG_Object_Item* pEquippedItem = GetEquippedItem(_iSlot);
	if (pEquippedItem == NULL)
		return false;

	if (!(pEquippedItem->m_Flags & RPG_ITEM_FLAGS_ACTIVATED))
		return false;

	pEquippedItem->Deactivate(_Mat, this, _iObject, _Input);
	pEquippedItem->m_Flags &= ~RPG_ITEM_FLAGS_ACTIVATED; // Should be moved to CRPG_Object_Item::Activate, somehow.

	return true;
}

bool CRPG_Object_Char::DeactivateItems(int _iSlot, const CMat4Dfp32 &_Mat, int _iObject, int _Input)
{
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return false;

	pInventory->DeactivateItems(_Mat, _iObject, _Input);
	return true;
}

CRPG_Object_Item *CRPG_Object_Char::FindItemByType(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char_FindItemByType, NULL);
	int iSlot = _iItemType >> 8;

	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindItemByType(_iItemType);
}

CRPG_Object_Item *CRPG_Object_Char::FindItemByAnimType(int _iAnimType, int _iSlot)
{
	MAUTOSTRIP(CRPG_Object_Char_FindItemByType, NULL);
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindItemByAnimType(_iAnimType);
}

CRPG_Object_Item* CRPG_Object_Char::FindNextItemByType(int _Identifier, int _iItemType, bool _bForce)
{
	MAUTOSTRIP(CRPG_Object_Char_FindItemByType, NULL);
	int iSlot = _iItemType >> 8;

	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindNextItemByType(_Identifier,_iItemType, _bForce);
}

CRPG_Object_Item* CRPG_Object_Char::FindNextItemForReloadByType(int _Identifier, int _iItemType, CRPG_Object_Item* _pOldItem)
{
	MAUTOSTRIP(CRPG_Object_Char_FindItemByType, NULL);
	int iSlot = _iItemType >> 8;

	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindNextItemForReloadByType(_Identifier,_iItemType, _pOldItem);
}

CRPG_Object_Item* CRPG_Object_Char::FindNextItemNotEquippedByType(int _Identifier, int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char_FindNextItemNotEquippedByType, NULL);
	int iSlot = _iItemType >> 8;

	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindNextItemNotEquippedByType(_Identifier,_iItemType);
}

CRPG_Object_Item* CRPG_Object_Char::FindNextItemNotEquippedByAnimType(int _Identifier, int _AnimType, int _iSlot, int _NotThisItemType, CRPG_Object_Item* _pOldItem, bool _bWithAmmo)
{
	MAUTOSTRIP(CRPG_Object_Char_FindNextItemNotEquippedByAnimType, NULL);
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindNextItemNotEquippedByAnimType(_Identifier,_AnimType,_NotThisItemType, _pOldItem, _bWithAmmo);
}

CRPG_Object_Item* CRPG_Object_Char::FindNextItemNotEquippedByGroupType(int _Identifier, int _GroupType, int _iSlot, int _NotThisItemType, bool _WithAmmo, CRPG_Object_Item* _pOldItem)
{
	MAUTOSTRIP(CRPG_Object_Char_FindNextItemNotEquippedByAnimType, NULL);
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindNextItemNotEquippedByGroupType(this, _Identifier,_GroupType,_NotThisItemType, _WithAmmo, _pOldItem);
}

CRPG_Object_Item *CRPG_Object_Char::FindItemByIdentifier(int _Identifier, int _iSlot)
{
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindItemByIdentifier(_Identifier);
}

CRPG_Object_Item *CRPG_Object_Char::FindItemByName(const char* _pName)
{
	MAUTOSTRIP(CRPG_Object_Char_FindItemByName, NULL);
	for (int iInventory = 0; iInventory < GetNumChildren(); iInventory++)
	{
		CRPG_Object_Inventory* pInventory = GetInventory(iInventory);
		if (pInventory == NULL)
			return NULL;

		CRPG_Object_Item* pItem = pInventory->FindItemByName(_pName);
		if (pItem != NULL)
			return pItem;
	}

	return NULL;
}

bool CRPG_Object_Char::CanPickup(int32 _iItemType)
{
	int iSlot = _iItemType >> 8;

	CRPG_Object_Inventory* pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return false;

	// Got inventory, check if we can merge without probs
	return pInventory->CanPickup(_iItemType);
}

bool CRPG_Object_Char::HasTranscendingItems()
{
	for(int i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
	{
		CRPG_Object_Inventory* pInv = GetInventory(i);
		uint32 nItems = pInv->GetNumItems();

		for(int j = 0; j < nItems; j++)
		{
			CRPG_Object_Item *pItem = pInv->GetItemByIndex(j);
			if (pItem && (pItem->m_Flags2 & RPG_ITEM_FLAGS2_TRANSCENDIVENTORY))
			{
				return true;
			}
		}
	}
	return false;
}

int CRPG_Object_Char::NumTranscendingItems(int _iInventory)
{
	CRPG_Object_Inventory* pInv = GetInventory(_iInventory);
	uint32 nItems = pInv->GetNumItems();
	int Count = 0;
	for(int j = 0; j < nItems; j++)
	{
		CRPG_Object_Item *pItem = pInv->GetItemByIndex(j);
		if (pItem && (pItem->m_Flags2 & RPG_ITEM_FLAGS2_TRANSCENDIVENTORY))
		{
			Count++;
		}
	}
	return Count;
}

CRPG_Object_Item* CRPG_Object_Char::GetFinalSelectedItem(int _iSlot)
{
	MAUTOSTRIP(CRPG_Object_Char_GetFinalSelectedItem, NULL);
	MSCOPESHORT(CRPG_Object_Char::GetFinalSelectedItem);
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->FindFinalSelectedItem();
}

CRPG_Object_Item* CRPG_Object_Char::GetEquippedItem(int _iSlot)
{
	MAUTOSTRIP(CRPG_Object_Char_GetEquippedItem, NULL);
	MSCOPESHORT(CRPG_Object_Char::GetEquippedItem);
	CRPG_Object_Inventory* pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->GetEquippedItem();
}

int CRPG_Object_Char::GetAssociatedItemType(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char_GetAssociatedItemType, 0);
	if (_iItemType == RPG_ITEMTYPE_UNDEFINED)
	{
		CRPG_Object_Inventory* pPrimaryInventory = GetInventory(RPG_CHAR_INVENTORY_WEAPONS);
		if (pPrimaryInventory == NULL)
			return RPG_ITEMTYPE_UNDEFINED;

		_iItemType = pPrimaryInventory->GetSelectedItemType();
	}

	CRPG_Object_Item *pPrimary = FindItemByType(_iItemType);
	if (pPrimary == NULL)
		return RPG_ITEMTYPE_UNDEFINED;

	int iLink;
	int iLinkedItemType;
	
	{ // Prefer currently equipped item.
		CRPG_Object_Inventory* pInventory = GetInventory(RPG_CHAR_INVENTORY_ITEMS);
		if (pInventory != NULL)
		{
			int EquippedItemType = pInventory->GetEquippedItemType();

			for (iLink = 0; iLink < pPrimary->m_NumLinks; iLink++)
			{
				iLinkedItemType = pPrimary->m_lLinkedItemTypes[iLink];
				if (EquippedItemType == iLinkedItemType)
					return EquippedItemType;
			}
		}
	}

	// Prefer currently linked item.
	iLinkedItemType = pPrimary->m_lLinkedItemTypes[pPrimary->m_iCurLink];
	CRPG_Object_Item* pLinkedItem = FindItemByType(iLinkedItemType);
	if (pLinkedItem != NULL)
	{
		if (pLinkedItem->IsEquippable(this))
			return iLinkedItemType;
	}

	int iFirstEquippableItemType = RPG_ITEMTYPE_UNDEFINED;

	// We can't select the currently linked item.
	// Prefer already equipped item.
	for (iLink = 0; iLink < pPrimary->m_NumLinks; iLink++)
	{
		iLinkedItemType = pPrimary->m_lLinkedItemTypes[iLink];

		CRPG_Object_Item* pLinkedItem = FindItemByType(iLinkedItemType);
		if (pLinkedItem != NULL)
		{
			if (pLinkedItem->IsEquippable(this))
			{
				iFirstEquippableItemType = iLinkedItemType;

				if (IsEquipped(iLinkedItemType))
					return iLinkedItemType;
			}
		}
	}

	// We didn't have an equipped item.
	// See if we've found an equippable item.
	if (iFirstEquippableItemType != RPG_ITEMTYPE_UNDEFINED)
		return iFirstEquippableItemType;

	// We didn't find an equippable item.
	return RPG_ITEMTYPE_UNDEFINED;
}

CRPG_Object_Ammo* CRPG_Object_Char::GetAssociatedAmmo(int _iItemType)
{
	MAUTOSTRIP(CRPG_Object_Char_GetAssociatedAmmo, NULL);
	int iLinkedItemType = GetAssociatedItemType(_iItemType);
	CRPG_Object_Item* pLinkedItem = FindItemByType(iLinkedItemType);
	CRPG_Object_Ammo* pAmmo = TDynamicCast<CRPG_Object_Ammo>(pLinkedItem);
	return pAmmo;
}

CRPG_Object_Item* CRPG_Object_Char::GetItemFromGroup(int _iItemType, int _iGroup)
{
	MAUTOSTRIP(CRPG_Object_Char_GetItemFromGroup, NULL);
	int iSlot = _iItemType >> 8;
	CRPG_Object_Inventory *pInventory = GetInventory(iSlot);
	if (pInventory == NULL)
		return NULL;

	for(int iItemIndex = 0; iItemIndex < pInventory->GetNumItems(); iItemIndex++)
	{
		CRPG_Object_Item* pItem = pInventory->GetItemByIndex(iItemIndex);
		if (pItem->m_WeaponGroup == _iGroup)
			return pItem;
	}

	return NULL;
}

CRPG_Object_Item* CRPG_Object_Char::GetItemByIndex(int _iSlot, int _iItemIndex)
{
	CRPG_Object_Inventory *pInventory = GetInventory(_iSlot);
	if (pInventory == NULL)
		return NULL;

	return pInventory->GetItemByIndex(_iItemIndex);
}

void CRPG_Object_Char::UpdateItems()
{
	MAUTOSTRIP(CRPG_Object_Char_UpdateItems, MAUTOSTRIP_VOID);
	GetInventory(RPG_CHAR_INVENTORY_WEAPONS)->UpdateItems(*this);
	GetInventory(RPG_CHAR_INVENTORY_ITEMS)->UpdateItems(*this);
	GetInventory(RPG_CHAR_INVENTORY_ARMOUR)->UpdateItems(*this);
	GetInventory(RPG_CHAR_INVENTORY_QUESTITEMS)->UpdateItems(*this);
}
