#ifndef __INC_WRPGCHAR
#define __INC_WRPGCHAR

//-------------------------------------------------------------------

#include "WRPGCore.h"
#include "WRPGInventory.h"
#include "WRPGItem.h"

//-------------------------------------------------------------------

enum
{
	SKILL_UNARMED = 0,
	SKILL_SWORD,
	SKILL_AXE,
	SKILL_HAMMER,
	SKILL_CROSSBOW,
	SKILL_BOW,
	SKILL_DAGGER,
	SKILL_MAGIC,
	SKILL_SUMMON,
	SKILL_BOMB,
	NUMSKILLS,
};

enum
{
	SELECTIONMETHOD_NORMAL,
	SELECTIONMETHOD_FORCE,
	SELECTIONMETHOD_FORCEACTIVATE,
};

enum
{
	// !!!!!!###### REMEMBER TO CHANGE THE ATTRIBUTE STRINGS WHEN MAKING CHANGES HERE ######!!!!!!
	RPG_CHAR_HEALTH = 0,
	RPG_CHAR_MAXHEALTH,
	RPG_CHAR_MANA,
	RPG_CHAR_MAXMANA,
	RPG_CHAR_PAIN,
	RPG_CHAR_WAIT,
	RPG_CHAR_SOUNDINDEX,
	RPG_CHAR_SOUNDDURATION,
	RPG_CHAR_AURA0,
	RPG_CHAR_AURA1,
	RPG_CHAR_AURA2,
	
	RPG_CHAR_NUMATTRIB,

	//----------------------------------------

	RPG_CHAR_NUMAURAS = 3,

	//----------------------------------------

	RPG_CHAR_INVENTORY_WEAPONS = 0,
	RPG_CHAR_INVENTORY_ITEMS,
	RPG_CHAR_INVENTORY_ARMOUR,
	RPG_CHAR_INVENTORY_QUESTITEMS,

	RPG_CHAR_NUMCHILDREN,

};

//-------------------------------------------------------------------

class CRPG_Object_Item;

//-------------------------------------------------------------------

class CRPG_Object_Char : public CRPG_Object
{
	MRTC_DECLARE;

public:
	static const char* ms_AttribStr[];
	static const char* ms_SkillStr[];

	virtual void OnCreate();
	virtual void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);

	virtual bool PreProcess(CRPG_Object *_pRoot, int);
	virtual bool OnProcess(CRPG_Object *_pRoot, int);

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	virtual int GetType() const					{ return TYPE_CHAR; }

//	int GetManaLeft();
//	bool DrawMana(int _amount);
//	bool GiveMana(int _amount);
	
	void ApplyDamage(int _iDamage);
	void ApplyWait(int _Wait);
	void ApplySound(int _iIndex, int _iDuration);

	int ReceiveHealth(int _Health);
	void IncreaseMaxHealth(int _HealthIncrease);

	void OnCharDeath(int _iObject);

	// Items
	bool AddItem(int _iObject, int _iSlot, CRPG_Object *_pItem);
	bool RemoveItemByType(int _iObject, int _iItemType, bool _bRemoveInstant = false);
	bool RemoveItemByIdentifier(int _iObject, int _Identifier, int _iSlot, bool _bRemoveInstant = false);
	bool RemoveItemByName(int _iObject, int _iSlot, const char* _pName, bool _bRemoveInstant = false);

	bool UpdateItemIdentifier(CRPG_Object_Item* _pItem);
	void UpdateItemIdentifiers();

	bool QuickActivateItem(CRPG_Object_Item *_pItem, const CMat4Dfp32 &_Mat, int _iObject, int _Input);
	bool ActivateItem(int _iSlot, const CMat4Dfp32 &_Mat, int _iObject, int _Input, bool _bOverride = false);
	bool DeactivateItem(int _iSlot, const CMat4Dfp32 &_Mat, int _iObject, int _Input);
	bool DeactivateItems(int _iSlot, const CMat4Dfp32 &_Mat, int _iObject, int _Input);
	bool SelectItem(int _iItemType, bool _bCheckSelectability, bool _bSelectAssociated, int _Method = SELECTIONMETHOD_NORMAL, bool _bInstant = false);
	bool SelectAssociatedItem(int _iItemType, bool _bCheckSelectability, int _Method = SELECTIONMETHOD_NORMAL, bool _bInstant = false);
	bool SelectItemByIdentifier(int _iItem, int _iSlot, bool _bCheckSelectability, bool _bSelectAssociated, int _Method = SELECTIONMETHOD_NORMAL, bool _bInstant = false);
	CRPG_Object_Item *PickupItem(const char *_pName, bool _bAllowMerge, bool _bSelect = false, TPtr<CRPG_Object_Item> *_pspDropping = NULL, int _iObject = 0, bool _bNoSound = false, int _iSender = 0, bool _bIsPlayer = false, bool _bNoPickupIcon = false);
	CRPG_Object_Item *EquipItem(const char* _pName);
	CRPG_Object_Item *LoadItem(const char *_pName, CCFile* _pFile, int _iObject = 0, bool _bIsPlayer = false);

	void ClearReloadFlags(int _iSlot);
	CRPG_Object_Item *GetItemFromGroup(int _iItemType, int _iGroup);

	CRPG_Object_Item *GetEquippedItem(int _iSlot);
	CRPG_Object_Item *GetFinalSelectedItem(int _iSlot);
	CRPG_Object_Item *FindItemByType(int _iItemType);
	CRPG_Object_Item *FindItemByAnimType(int _iAnimType, int _iSlot);
	CRPG_Object_Item *FindItemByName(const char* _pName);
	CRPG_Object_Item *FindItemByIdentifier(int _Identifier, int _iSlot);
	CRPG_Object_Item *FindNextItemByType(int _Identifier, int _iItemType, bool _bForce = false);
	CRPG_Object_Item* FindNextItemForReloadByType(int _Identifier, int _iItemType, CRPG_Object_Item* _pOldItem = NULL);
	CRPG_Object_Item *FindNextItemNotEquippedByType(int _Identifier, int _iItemType);
	CRPG_Object_Item *FindNextItemNotEquippedByAnimType(int _Identifier, int _iAnimType, int _iSlot, int _NotThisItemType = -1, CRPG_Object_Item* _pOldItem = NULL, bool _bWithAmmo = false);
	CRPG_Object_Item *FindNextItemNotEquippedByGroupType(int _Identifier, int _GroupType, int _iSlot, int _NotThisItemType = -1, bool _bWithAmmo = false, CRPG_Object_Item* _pOldItem = NULL);
	
	virtual bool CanPickup(int32 _iItemType);
	bool HasTranscendingItems();
	int NumTranscendingItems(int _iInventory);

	int GetAssociatedItemType(int _iItemType = RPG_ITEMTYPE_UNDEFINED);
	class CRPG_Object_Ammo* GetAssociatedAmmo(int _iItemType = RPG_ITEMTYPE_UNDEFINED);

	bool IsSelected(int _iItemType);
	bool IsEquipped(int _iItemType);
	bool HasItem(int _iItemType);
	bool IsSelectableItem(CRPG_Object_Item* _pItem, bool _bForceSelected = false);
	bool CycleSelectedItem(int _iSlot, bool _bReverse = false, bool _bSelectAssociated = true, bool _bInstant = false);

/*
	bool FindItem(int _iType, int &_iSlot, int &_iIndex); // Expired
	CRPG_Object_Item *GetCurItem(int _iSlot); // Expired
	CRPG_Object_Ammo *GetCurAmmo(); // Expired
	CRPG_Object_Item *FindItem(int _iItemType); // Expired
	int GetCurItemIndex(int _iSlot); // Expired
	int GetNextItemIndex(int _iSlot, int _iCurIndex, bool _bReverse); // Expired
	int GetSuggestedItem(); // Expired!
*/

	int GetNumItems(int _iSlot)					{ return GetInventory(_iSlot)->GetNumChildren(); }

	// For inventory enumerations only!!!
	CRPG_Object_Item *GetItemByIndex(int _iSlot, int _iItemIndex);

	void UpdateItems();

	// Attribs
	CRPG_Attrib &Health()						{ return Attrib(RPG_CHAR_HEALTH); }
	CRPG_Attrib &MaxHealth()					{ return Attrib(RPG_CHAR_MAXHEALTH); }

	CRPG_Attrib &Mana()							{ return Attrib(RPG_CHAR_MANA); }
	CRPG_Attrib &MaxMana()						{ return Attrib(RPG_CHAR_MAXMANA); }

	CRPG_Attrib &Pain()							{ return Attrib(RPG_CHAR_PAIN); }
	CRPG_Attrib &Wait()							{ return Attrib(RPG_CHAR_WAIT); }

	CRPG_Attrib &SoundIndex()					{ return Attrib(RPG_CHAR_SOUNDINDEX); }
	CRPG_Attrib &SoundDuration()				{ return Attrib(RPG_CHAR_SOUNDDURATION); }

	class CRPG_Object_Inventory *GetInventory(int _iSlot);
	int GetSkillLevel(int _SkillType);
	int GetDamageBoost(int _SkillType);

//	int m_iPushedItem;
	int m_lSkillLevels[NUMSKILLS];

	int m_ManaReg_RestTicks, m_ManaReg_CurRestTicks;
	int m_ManaReg_Amount;
	int m_ManaReg_DelayTicks, m_ManaReg_CurDelayTicks;

	// Health regeneration
	int32 m_LastDamageTick;
	int32 m_HealthRegenerationDelay;

	// Hack to let the campaign use the default templates. If this is set, all items defined in char-templates are ignored.
	static bool m_bUseDefaultItems;
	static bool m_bInsideEquipGUI;
};

#endif // __INC_WRPGCHAR
