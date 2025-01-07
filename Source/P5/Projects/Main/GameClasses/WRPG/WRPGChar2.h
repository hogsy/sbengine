#ifndef __INC_WRPGCHAR2
#define __INC_WRPGCHAR2

//-------------------------------------------------------------------

#include "WRPGCore.h"
#include "WRPGInventory2.h"
#include "WRPGItem2.h"

//-------------------------------------------------------------------

class CRPG_Object_Item2;

//-------------------------------------------------------------------

enum
{
	SELECTIONMETHOD_NORMAL,
	SELECTIONMETHOD_FORCE,
	SELECTIONMETHOD_FORCEACTIVATE,
};

enum
{
	RPG_CHAR2_INVENTORY = 0,
	RPG_CHAR2_NUMCHILDREN,

	RPG_CHAR2_ITEMTYPE_WEAPON = 0,
	RPG_CHAR2_ITEMTYPE_ITEM = 1,
};
class CRPG_Object_Char2 : public CRPG_Object
{
	MRTC_DECLARE;

	int16 m_Wait;
	int16 m_Pain;
public:
	virtual void OnCreate();
	virtual void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);

	virtual bool PreProcess(CRPG_Object *_pRoot, int);
	virtual bool OnProcess(CRPG_Object *_pRoot, int);

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	virtual int GetType() const					{ return TYPE_CHAR; }

	//void ApplyDamage(int _iDamage);
	void ApplyWait(int _Wait);

	void OnCharDeath(int _iObject);

	// Items
	bool AddItem(int _iObject, CRPG_Object *_pItem);
	bool RemoveItemByType(int _iObject, int _iItemType, bool _bRemoveInstant = false);
	bool RemoveItemByName(int _iObject, const char* _pName, bool _bRemoveInstant = false);

	bool QuickActivateItem(CRPG_Object_Item2 *_pItem, const CMat43fp32 &_Mat, int _iObject, int _Input);
	bool ActivateItem(const CMat43fp32 &_Mat, int _iObject, int _Input, bool _bOverride = false);
	bool DeactivateItem(const CMat43fp32 &_Mat, int _iObject, int _Input);
	bool DeactivateItems(const CMat43fp32 &_Mat, int _iObject, int _Input);
	bool SelectItem(int _iItemType, bool _bCheckSelectability, int _Method = SELECTIONMETHOD_NORMAL, bool _bInstant = false);
	CRPG_Object_Item2 *PickupItem(const char *_pName, bool _bAllowMerge, bool _bSelect = false, TPtr<CRPG_Object_Item2> *_pspDropping = NULL, int _iObject = 0, bool _bNoSound = false, int _iSender = 0);
	CRPG_Object_Item2 *EquipItem(const char* _pName);

	CRPG_Object_Item2 *GetItemFromGroup(int _iItemType, int _iGroup);

	CRPG_Object_Item2 *GetEquippedItem();
	CRPG_Object_Item2 *GetFinalSelectedItem();
	CRPG_Object_Item2 *FindItemByType(int _iItemType);
	CRPG_Object_Item2 *FindItemByName(const char* _pName);

	virtual bool CanPickup(int32 _iItemType);

	bool IsSelected(int _iItemType);
	bool IsEquipped(int _iItemType);
	bool HasItem(int _iItemType);
	bool IsSelectableItem(CRPG_Object_Item2* _pItem, bool _bForceSelected = false);
	bool CycleSelectedItem(bool _bReverse = false, bool _bInstant = false);

	int GetNumItems() { return GetInventory()->GetNumChildren(); }

	// For inventory enumerations only!!!
	CRPG_Object_Item2 *GetItemByIndex(int _iItemIndex);

	void UpdateItems();

	class CRPG_Object_Inventory2 *GetInventory();

	const int16& Wait() const { return m_Wait; }
	int16& Wait() { return m_Wait; }
	const int16& Pain() const { return m_Pain; }
	int16& Pain() { return m_Pain; }
};

#endif