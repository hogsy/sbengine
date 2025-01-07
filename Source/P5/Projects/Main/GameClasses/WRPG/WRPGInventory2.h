#ifndef WRPGInventory2_h
#define WRPGInventory2_h

//-------------------------------------------------------------------

#include "WRPGCore.h"
//#include "WRPGChar.h"
#include "WRPGItem2.h"

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

class CRPG_Object_Char22;
class CRPG_Object_Item2;

//-------------------------------------------------------------------

class CRPG_Object_Inventory2 : public CRPG_Object
{
	MRTC_DECLARE;

protected:

	int m_iLastEquippedItemIndex;
	int m_iEquippedItemIndex; // Currently equipped item.
	int m_iSelectedItemType; // Wanted item.
	int m_iForceSelectedItemType; // Temp wanted item.
	int m_iForceSelectedItemAction; // Pending action for force selected item.
	bool m_bInstantEquip;

public:

	virtual int GetType() const					{ return TYPE_INVENTORY; }

	int GetGameTick(int _iObject);

	void UpdateItems(class CRPG_Object_Char2 &_Char);
	void ActivateItems(CRPG_Object_Char2 &_Char, CRPG_Object_Item2 *_pPrimaryItem = NULL);
	void DeactivateItems(const CMat43fp32 &_Mat, int _iObject, int _Input);

public:

	void OnCreate();
	bool OnProcess(CRPG_Object* _pRoot, int _iObject);

	void OnCharDeath(int _iObject, CRPG_Object* _pRoot);

	int GetNumItems();
	CRPG_Object_Item2* GetItemByIndex(int _iItemIndex);

	CRPG_Object_Item2* FindItemByType(int _iItemType);
	CRPG_Object_Item2* FindItemByName(const char* _pName);

	int GetItemType(int _iItemIndex);
	int FindItemIndex(int _iItemType);
	int FindItemIndex(const char* _pName);
	bool CanPickup(int32 _iItemType);

	// true of operation successfull (ambigeous for removing, was item there before at all, or was it really deleted).
	bool AddItem(int _iObject, CRPG_Object_Item2* _pItem);
	bool RemoveItemByIndex(int _iObject, int _iItemIndex, bool _bRemoveInstant = false);   // _iObject is deadcode, plz remove /kma
	bool RemoveItemByName(int _iObject, const char* _pName, bool _bRemoveInstant = false);
	bool RemoveItemByType(int _iObject, int _iItemType, bool _bRemoveInstant = false);
	bool ReplaceItemByType(int _iObject, CRPG_Object_Item2* _pNewItem, bool _bInstantly = false);

	int GetEquippedItemType();
	int GetSelectedItemType();
	int GetFinalSelectedItemType();

	int GetFinalSelectedItemIndex();
	int GetNextValidSelectedItemIndex(int _iIndex);

	CRPG_Object_Item2* GetEquippedItem();
	CRPG_Object_Item2* FindFinalSelectedItem();

	CRPG_Object_Item2* GetCycledItem(int _NumTries, bool _bReverse);

	bool SelectItem(int _iItemType, CRPG_Object_Char2* _pChar, bool _bInstant = false); // Return true if item was selected.
	bool ForceSelectItem(int _iItemType, CRPG_Object_Char2* _pChar, bool _bInstant = false); // Return true if item was selected.
	bool ForceActivateItem(int _iItemType, CRPG_Object_Char2* _pChar, bool _bInstant = false); // Return true if item was selected.

	void OnUpdateModel();
	void PendingUpdate(CRPG_Object* _pRoot, int _iObject);
};

//-------------------------------------------------------------------

#endif /* WRPGInventory_h */