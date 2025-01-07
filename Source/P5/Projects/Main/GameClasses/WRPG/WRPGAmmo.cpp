// -------------------------------------------------------------------

#include "PCH.h"

#include "WRPGAmmo.h"
#include "WRPGChar.h"
//#include "WRPGBow.h"

/*
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
#include "../WObj_Char.h"
*/

// -------------------------------------------------------------------
//  CRPG_Object_Ammo
// -------------------------------------------------------------------

CRPG_Object_Ammo::CRPG_Object_Ammo()
{
	MAUTOSTRIP(CRPG_Object_Ammo_ctor, MAUTOSTRIP_VOID);
	m_NumItems = 0;
	m_NumLoaded = 0;
	m_MagazineSize = 1;
	m_iAmmoSurface1 = 0;
	m_iAmmoSurface2 = 0;
	m_SpawnType = 0;
}

// -------------------------------------------------------------------

void CRPG_Object_Ammo::OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_Ammo_OnIncludeClass, MAUTOSTRIP_VOID);
	CRPG_Object_Item::OnIncludeClass(_pReg, _pMapData, _pWServer);
	
	IncludeSurfaceFromKey("AMMOSURFACE1", _pReg, _pMapData);
	IncludeSurfaceFromKey("AMMOSURFACE2", _pReg, _pMapData);
	IncludeClassFromKey("SPAWN", _pReg, _pMapData);
}

// -------------------------------------------------------------------

bool CRPG_Object_Ammo::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_Ammo_OnEvalKey, false);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	int KeyValuei = KeyValue.Val_int();

	switch (_KeyHash)			// Presumably maximum number of magazines that can be carried
	{
	case MHASH2('MAXI','TEMS'): // "MAXITEMS"
		{
			m_MaxItems = KeyValuei;
			break;
		}

	case MHASH3('MAGA','ZINE','SIZE'): // "MAGAZINESIZE"
		{
			m_MagazineSize = KeyValuei;
			break;
		}

	case MHASH2('SPAW','N'): // "SPAWN"
		{
			m_Spawn = KeyValue;
			m_pWServer->GetMapData()->GetResourceIndex_Class(m_Spawn);
			break;
		}

	case MHASH3('SPAW','NTYP','E'): // "SPAWNTYPE"
		{
			m_SpawnType = KeyValuei;
			break;
		}

	case MHASH2('NUMI','TEMS'): // "NUMITEMS"
		{
			m_NumLoaded = KeyValuei;
			m_NumItems	= KeyValuei;
			break;
		}
	
	default:
		{
			if(KeyName.CompareSubStr("AMMOSURFACE1") == 0)
				m_iAmmoSurface1 = m_pWServer->GetMapData()->GetResourceIndex_Surface(KeyValue);

			else if(KeyName.CompareSubStr("AMMOSURFACE2") == 0)
				m_iAmmoSurface2 = m_pWServer->GetMapData()->GetResourceIndex_Surface(KeyValue);
			else
				return CRPG_Object_Item::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}

	return true;
}

// -------------------------------------------------------------------

bool CRPG_Object_Ammo::OnActivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Ammo_OnActivate, false);
/*	CRPG_Object_Item *pPrimary = GetCurLinkedItem(_pRoot);
	if(pPrimary)
		pPrimary->Reload(&_Mat, _pRoot, _iObject);*/
	return false;
}

// -------------------------------------------------------------------

bool CRPG_Object_Ammo::MergeItem(int _iObject, CRPG_Object_Item *_pObj)
{
	MAUTOSTRIP(CRPG_Object_Ammo_MergeItem, false);
	CRPG_Object_Ammo *pAmmo = safe_cast<CRPG_Object_Ammo >(_pObj);
	if(pAmmo)
	{
		if(AddToTotal(pAmmo->m_NumItems) > 0)
			return true;
	}
//	return false;
	
	// We should return false, so that you can't pickup ammo if you have your magazine full,
	// but things break if we do.
	return true;
}

// -------------------------------------------------------------------

void CRPG_Object_Ammo::SetMagazineSize(int _Size)
{
	MAUTOSTRIP(CRPG_Object_Ammo_SetMagazineSize, MAUTOSTRIP_VOID);
	if(_Size != m_MagazineSize)
	{
		m_MagazineSize = _Size;
		RefillMagazine();
	}
}

// -------------------------------------------------------------------

int CRPG_Object_Ammo::RefillMagazine()
{
	MAUTOSTRIP(CRPG_Object_Ammo_RefillMagazine, 0);
//	return AddLoaded(m_MagazineSize);
	return 1;	// Don't refill magazine when loading game from savefile and since the code can't possibly know just .. ugh .. fuck it! - RUNE
}

// -------------------------------------------------------------------

int CRPG_Object_Ammo::AddLoaded(int _Num)
{
	MAUTOSTRIP(CRPG_Object_Ammo_AddLoaded, 0);
	int OldNumLoaded = m_NumLoaded;
	int PrevMin = Min(int(m_MagazineSize), int(m_NumItems));
	m_NumLoaded = Min(int(m_NumLoaded + _Num), PrevMin);
	int NewlyLoaded = m_NumLoaded - OldNumLoaded;
	return NewlyLoaded;
}

// -------------------------------------------------------------------

int CRPG_Object_Ammo::ConsumeLoaded(int _Num)
{
	MAUTOSTRIP(CRPG_Object_Ammo_ConsumeLoaded, 0);
	int Consumed = Min(int(m_NumLoaded), _Num);
	ConsumeFromTotal(Consumed);
	m_NumLoaded -= _Num;
	return Consumed;
}

// -------------------------------------------------------------------

int CRPG_Object_Ammo::ComsumeAllLoaded()
{
	MAUTOSTRIP(CRPG_Object_Ammo_ComsumeAllLoaded, 0);
	return ConsumeLoaded(m_NumLoaded);
}

// -------------------------------------------------------------------

void CRPG_Object_Ammo::Unload()
{
	MAUTOSTRIP(CRPG_Object_Ammo_Unload, MAUTOSTRIP_VOID);
	m_NumLoaded = 0;
}

// -------------------------------------------------------------------

int CRPG_Object_Ammo::AddToTotal(int _Num)
{
	MAUTOSTRIP(CRPG_Object_Ammo_AddToTotal, 0);
	if(m_NumItems == m_MaxItems)
		return false;

	m_NumItems += _Num;
	if(m_NumItems > m_MaxItems)
		m_NumItems = m_MaxItems;
	return true;
}

// -------------------------------------------------------------------

bool CRPG_Object_Ammo::ConsumeFromTotal(int _Num)
{
	MAUTOSTRIP(CRPG_Object_Ammo_ConsumeFromTotal, false);
	if(m_NumItems < _Num)
		return false;
	
	m_NumItems -= _Num;
	return true;
}

// -------------------------------------------------------------------

int CRPG_Object_Ammo::GetNumTotal()
{
	MAUTOSTRIP(CRPG_Object_Ammo_GetNumTotal, 0);
	return m_NumItems;
}

// -------------------------------------------------------------------

int CRPG_Object_Ammo::GetMaxTotal()
{
	MAUTOSTRIP(CRPG_Object_Ammo_GetMaxTotal, 0);
	return m_MaxItems;
}

// -------------------------------------------------------------------

int CRPG_Object_Ammo::GetNumLoaded()
{
	MAUTOSTRIP(CRPG_Object_Ammo_GetNumLoaded, 0);
	return m_NumLoaded;
}

// -------------------------------------------------------------------

int CRPG_Object_Ammo::GetMagazineSize()
{
	MAUTOSTRIP(CRPG_Object_Ammo_GetMagazineSize, 0);
	return m_MagazineSize;
}

// -------------------------------------------------------------------

const char* CRPG_Object_Ammo::GetSpawnClass()
{
	MAUTOSTRIP(CRPG_Object_Ammo_GetSpawnClass, NULL);
	return m_Spawn;
}

int CRPG_Object_Ammo::GetSpawnType()
{
	return m_SpawnType;
};

void CRPG_Object_Ammo::OnDeltaSave(CCFile* _pFile, CRPG_Object* _pChar)
{
//	CRPG_Object_Item::OnDeltaSave(_pFile, _pChar);
//	_pFile->WriteLE(m_NumLoaded);
}

void CRPG_Object_Ammo::OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pChar)
{
//	CRPG_Object_Item::OnDeltaLoad(_pFile, _pChar);
//	_pFile->ReadLE(m_NumLoaded);
}

// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Ammo, CRPG_Object_Item);

// -------------------------------------------------------------------
//  CRPG_Object_BurstAmmo
// -------------------------------------------------------------------
/*
CRPG_Object_Bow* CRPG_Object_BurstAmmo::GetPrimary(CRPG_Object* _pRoot)
{
	MAUTOSTRIP(CRPG_Object_BurstAmmo_GetPrimary, NULL);
	if ((_pRoot == NULL) || (_pRoot->GetType() != TYPE_CHAR))
		return NULL;

	CRPG_Object_Char* pChar = (CRPG_Object_Char*)_pRoot;
	CRPG_Object_Item* pItem = pChar->FindItemByType(m_lLinkedItemTypes[m_iCurLink]);
	CRPG_Object_Bow* pPrimary = safe_cast<CRPG_Object_Bow>(pItem);

	return pPrimary;
}

// -------------------------------------------------------------------

bool CRPG_Object_BurstAmmo::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_BurstAmmo_Activate, false);
	CRPG_Object_Bow* pBow = GetPrimary(_pRoot);
	if (pBow == NULL)
		return false;

	pBow->ActivateSecondary(_Mat, _pRoot, _iObject);

	return true;
}

// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_BurstAmmo, CRPG_Object_Ammo);

// -------------------------------------------------------------------
//  CRPG_Object_SniperAmmo
// -------------------------------------------------------------------

CRPG_Object_Bow* CRPG_Object_SniperAmmo::GetPrimary(CRPG_Object* _pRoot)
{
	MAUTOSTRIP(CRPG_Object_SniperAmmo_GetPrimary, NULL);
	if ((_pRoot == NULL) || (_pRoot->GetType() != TYPE_CHAR))
		return NULL;

	CRPG_Object_Char* pChar = (CRPG_Object_Char*)_pRoot;
	CRPG_Object_Item* pItem = pChar->FindItemByType(m_lLinkedItemTypes[m_iCurLink]);
	CRPG_Object_Bow* pPrimary = safe_cast<CRPG_Object_Bow>(pItem);

	return pPrimary;
}

// -------------------------------------------------------------------

void CRPG_Object_SniperAmmo::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_SniperAmmo_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object_Ammo::OnCreate();
}

// -------------------------------------------------------------------

bool CRPG_Object_SniperAmmo::Equip(int _iObject, CRPG_Object* _pRoot)
{
	MAUTOSTRIP(CRPG_Object_SniperAmmo_Equip, false);
	CRPG_Object_Bow* pBow = GetPrimary(_pRoot);
	if (pBow == NULL)
		return false;

	pBow->EquipSecondary(_iObject, _pRoot);

	return CRPG_Object_Ammo::Equip(_iObject, _pRoot);
}

// -------------------------------------------------------------------

bool CRPG_Object_SniperAmmo::Unequip(int _iObject, CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CRPG_Object_SniperAmmo_Unequip, false);
	CRPG_Object_Bow* pBow = GetPrimary(_pRoot);
	if (pBow == NULL)
		return false;

	pBow->UnequipSecondary(_iObject, _pRoot);

	return CRPG_Object_Ammo::Unequip(_iObject, _pRoot);
}

// -------------------------------------------------------------------

bool CRPG_Object_SniperAmmo::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_SniperAmmo_Activate, false);
	CRPG_Object_Bow* pBow = GetPrimary(_pRoot);
	if (pBow == NULL)
		return false;

	pBow->ActivateSecondary(_Mat, _pRoot, _iObject);

	return true;
}

// -------------------------------------------------------------------

bool CRPG_Object_SniperAmmo::Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_SniperAmmo_Deactivate, false);
	CRPG_Object_Bow* pBow = GetPrimary(_pRoot);
	if (pBow == NULL)
		return false;

	pBow->DeactivateSecondary(_Mat, _pRoot, _iObject);

	return true;
}

// -------------------------------------------------------------------

bool CRPG_Object_SniperAmmo::OnProcess(CRPG_Object *_pRoot, const CMat4Dfp32 &_Mat, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_SniperAmmo_OnProcess, false);
	return CRPG_Object_Ammo::OnProcess(_pRoot, _Mat, _iObject);
}

// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_SniperAmmo, CRPG_Object_Ammo);

// -------------------------------------------------------------------
*/
