#ifndef WRPGInventory_h
#define WRPGInventory_h

//-------------------------------------------------------------------

#include "WRPGCore.h"
//#include "WRPGChar.h"
#include "WRPGItem.h"

//-------------------------------------------------------------------

enum
{
	RPG_ITEMTYPE_UNDEFINED = -1,
	RPG_ITEMTYPE_EMPTY = -2,
	RPG_ITEMTYPE_POTION = 0x106,
};

enum
{
	FORCESELECTION_NONE,
	FORCESELECTION_ACTIVATE,
	FORCESELECTION_WAITACTIVATE,
	FORCESELECTION_REMOVE,
};

//-------------------------------------------------------------------

class CRPG_Object_Char;
class CRPG_Object_Item;

//-------------------------------------------------------------------

class CRPG_Object_Inventory : public CRPG_Object
{
	MRTC_DECLARE;

protected:

	int m_LastEquippedItemIdentifier;
	int m_EquippedItemIdentifier; // Currently equipped item.
	int m_iSelectedItemType; // Wanted item.
	int m_iSelectedItemIdentifier; // Wanted item.
	int m_iForceSelectedItemType; // Temp wanted item.
	int m_iForceSelectedItemAction; // Pending action for force selected item.
	int m_iForceSelectedItemIdentifier; // Temp wanted item.
	int m_ItemCounter;
	bool m_bInstantEquip;
	bool m_bShareAnimation;

public:

	int m_iForceSelectedOtherInventory;
	int16 m_GUITaggedItemID;

	virtual int GetType() const					{ return TYPE_INVENTORY; }

	int GetGameTick(int _iObject);

	void UpdateItems(class CRPG_Object_Char &_Char);
	void ActivateItems(CRPG_Object_Char &_Char, CRPG_Object_Item *_pPrimaryItem = NULL);
	void DeactivateItems(const CMat4Dfp32 &_Mat, int _iObject, int _Input);

public:

	void OnCreate();
	bool OnProcess(CRPG_Object* _pRoot, int _iObject);

	void OnCharDeath(int _iObject, CRPG_Object* _pRoot);

	int GetNumItems();
	CRPG_Object_Item* GetItemByIndex(int _iItemIndex);

	CRPG_Object_Item* FindItemByType(int _iItemType);
	CRPG_Object_Item* FindItemByAnimType(int _iAnimType);
	CRPG_Object_Item* FindNextItemByType(int _Identifier, int _iItemType, bool _bForce = false);
	CRPG_Object_Item* FindNextItemForReloadByType(int _Identifier, int _iItemType, CRPG_Object_Item* _pOldItem);
	CRPG_Object_Item* FindNextItemNotEquippedByType(int _Identifier, int _iItemType, CRPG_Object_Item* _pOldItem = NULL);
	CRPG_Object_Item* FindNextItemNotEquippedByAnimType(int _Identifier, int _AnimType, int _NotThisItemType = -1, CRPG_Object_Item* _pOldItem = NULL, bool _bWithAmmo = false);
	CRPG_Object_Item* FindNextItemNotEquippedByGroupType(CRPG_Object* _pRoot, int _Identifier, int _GroupType, int _NotThisItemType = -1, bool _bWithAmmo = false, CRPG_Object_Item* _pOldItem = NULL);
	CRPG_Object_Item* FindItemByIdentifier(int _Identifier);
	CRPG_Object_Item* FindItemByName(const char* _pName);

	void RemoveEmptyItems(int _Identifier = -1);
	void ReloadItemFromOthers(CRPG_Object_Summon* _pItemSummon);

	int GetItemType(int _iItemIndex);
	int GetItemTypeByIdentifier(int _Identifier);
	int FindItemIndex(int _iItemType);
	int FindItemIndexByAnimType(int _iAnimType);
	int FindItemIndexByIdentifier(int _Identifier);
	int FindItemIndex(const char* _pName);
	bool CanPickup(int32 _iItemType);

	// true of operation successfull (ambigeous for removing, was item there before at all, or was it really deleted).
	bool AddItem(int _iObject, CRPG_Object_Item* _pItem);
	bool RemoveItemByIndex(int _iObject, int _iItemIndex, bool _bRemoveInstant = false);   // _iObject is deadcode, plz remove /kma
	bool RemoveItemByIdentifier(int _iObject, int _Identifier, bool _bRemoveInstant = false);   // _iObject is deadcode, plz remove /kma
	bool RemoveItemByName(int _iObject, const char* _pName, bool _bRemoveInstant = false);
	bool RemoveItemByType(int _iObject, int _iItemType, bool _bRemoveInstant = false);
	bool ReplaceItemByType(int _iObject, CRPG_Object_Item* _pNewItem, bool _bInstantly = false);

	bool UpdateItemIdentifier(CRPG_Object_Item* _pItem);

	int GetEquippedItemType();
	int GetSelectedItemType();
	int GetFinalSelectedItemType();
	int GetFinalSelectedItemIdentifier();

	void SetItemCounter(int32 _ItemCounter) { m_ItemCounter = _ItemCounter; }
	int32 GetItemCounter() { return m_ItemCounter; }

	int GetFinalSelectedItemIndex();
	int GetNextValidSelectedItemIndex(int _iIndex);
	int GetPreviousValidSelectedItemIndex(int _iIndex);
	int GetNextValidSelectedItemIndexByType(CRPG_Object_Item * pCurrent, int _ExcludeType = -1);
	int GetPreviousValidSelectedItemIndexByType(CRPG_Object_Item * pCurrent, int _ExcludeType = -1);

	CRPG_Object_Item* GetEquippedItem();
	CRPG_Object_Item* FindFinalSelectedItem();

	CRPG_Object_Item* GetCycledItem(int _NumTries, bool _bReverse);
	void ForceSetEquipped(int _iObject);
	void ForceSetUnequipped(int _iObject);

	bool SelectItem(int _iItemType, CRPG_Object_Char* _pChar, bool _bInstant = false); // Return true if item was selected.
	bool SelectItemByIdentifier(int _Identifier, CRPG_Object_Char* _pChar, bool _bInstant = false); // Return true if item was selected.
	bool ForceSelectItem(int _iItemType, CRPG_Object_Char* _pChar, bool _bInstant = false); // Return true if item was selected.
	bool ForceSelectItemByIdentifier(int _Identifier, CRPG_Object_Char* _pChar, bool _bInstant = false); // Return true if item was selected.
	bool ForceActivateItem(int _iItemType, CRPG_Object_Char* _pChar, bool _bInstant = false); // Return true if item was selected.
	bool ForceActivateItemByIdentifier(int _Identifier, CRPG_Object_Char* _pChar, bool _bInstant = false); // Return true if item was selected.

	void OnUpdateModel();
	void PendingUpdate(CRPG_Object* _pRoot, int _iObject);
};

//-------------------------------------------------------------------

#endif /* WRPGInventory_h */
