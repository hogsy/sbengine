#include "PCH.h"

#include "WRPGGun.h"
#include "../WObj_Char.h"
#include "WRPGChar.h"

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Gun, CRPG_Object_Weapon);

void CRPG_Object_Gun::OnCreate()
{
	CRPG_Object_Weapon::OnCreate();

	m_bDefaultRicochet = true;
	m_Flags |= RPG_ITEM_FLAGS_LASERBEAM;
}

bool CRPG_Object_Gun::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	return CRPG_Object_Weapon::OnEvalKey(_KeyHash, _pKey);
}

bool CRPG_Object_Gun::DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout)
{
	FireMuzzle(_iObject);

	return CRPG_Object_Weapon::DrawAmmo(_Num, _Mat, _pRoot, _iObject, _bApplyFireTimeout);
}

bool CRPG_Object_Gun::OnProcess(CRPG_Object *_pRoot, int _iObject)
{
	RefreshMuzzle(_iObject);
	return CRPG_Object_Weapon::OnProcess(_pRoot, _iObject);
}

int CRPG_Object_Gun::ShowNumMagazines()
{
	// Show ammo left
	return m_AmmoLoad;
}

bool CRPG_Object_Gun::CanPickup()
{
	if (m_AmmoLoad <= 0)
		return false;
	else
		return CRPG_Object_Weapon::CanPickup();
}

CRPG_Object_DualSupport::CRPG_Object_DualSupport()
{
	m_PrevCloneIdentifier = -1;
}

bool CRPG_Object_Gun::Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant, bool _bHold)
{
	if (m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)
		EquipDualSupport(_iObject, _pRoot, _bInstant, this);

	return CRPG_Object_Weapon::Equip(_iObject, _pRoot,_bInstant, _bHold);
}

bool CRPG_Object_Gun::Unequip(int _iObject, CRPG_Object *_pRoot, bool _bInstant, bool _bHold)
{
	if (m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)
		UnequipDualSupport(_iObject, _pRoot, _bInstant, this);

	return CRPG_Object_Weapon::Unequip(_iObject, _pRoot,_bInstant, _bHold);
}

void CRPG_Object_Gun::OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	if (m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)
		LoadDualSupport(_pFile, _pCharacter);
	CRPG_Object_Weapon::OnDeltaLoad(_pFile, _pCharacter);
}

void CRPG_Object_Gun::OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	// Save cloneid, clone isn't actually saved but instead a copy of this gun will be made
	// when equipping (basically the same thing)
	if (m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)
		SaveDualSupport(_pFile, _pCharacter);
	CRPG_Object_Weapon::OnDeltaSave(_pFile, _pCharacter);
}
