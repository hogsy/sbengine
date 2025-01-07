#include "PCH.h"

#include "WRPGRifle.h"
#include "WRPGChar.h"
#include "../WObj_Char.h"
#include "../WObj_Ai/AI_Def.h"

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Rifle, CRPG_Object_Weapon);

#include "WRPGShotGun.h"
#include "../WObj_Char.h"

void CRPG_Object_Rifle::OnCreate()
{
	CRPG_Object_Weapon::OnCreate();
	m_iSound_FlashlightOn = 0;
	m_iSound_FlashlightOff = 0;
	m_Flashlight_Color = 0xffb2d8ff;
	m_Flashlight_Range = 512;

	m_iElectroSurface = 0;
	m_iSound_AccessDenied = 0;
	m_bDefaultRicochet = true;

	m_Flags |= RPG_ITEM_FLAGS_LASERBEAM;
}

void CRPG_Object_Rifle::CycleMode(int _iObject)
{
	CWObject *pObj = m_pWServer->Object_Get(_iObject);
	if(!pObj || m_bNoFlashLight)
		return;

	if(m_Flags & RPG_ITEM_FLAGS_FLASHLIGHT)
	{
		if(m_iSound_FlashlightOff)
			m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_FlashlightOff, 0);
		m_Flags &= ~RPG_ITEM_FLAGS_FLASHLIGHT;
	}
	else
	{
		if(m_iSound_FlashlightOn)
			m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_FlashlightOn, 0);
		m_Flags |= RPG_ITEM_FLAGS_FLASHLIGHT;
	}
	SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, 0, -1,m_AnimGripType);
}

void CRPG_Object_Rifle::OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	CRPG_Object_Weapon::OnIncludeClass(_pReg, _pMapData, _pWServer);

	IncludeSoundFromKey("SOUND_FLASHLIGHTON", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_FLASHLIGHTOFF", _pReg, _pMapData);
	IncludeSurfaceFromKey("ELECTRO_SURFACE", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_ACCESSDENIED", _pReg, _pMapData);
}

bool CRPG_Object_Rifle::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH5('SOUN','D_FL','ASHL','IGHT','ON'): // "SOUND_FLASHLIGHTON"
		{
			m_iSound_FlashlightOn = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH5('SOUN','D_FL','ASHL','IGHT','OFF'): // "SOUND_FLASHLIGHTOFF"
		{
			m_iSound_FlashlightOff = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH4('FLAS','HLIG','HT_C','OLOR'): // "FLASHLIGHT_COLOR"
		{
			m_Flashlight_Color = _pKey->GetThisValuei();
			break;
		}

	case MHASH4('FLAS','HLIG','HT_R','ANGE'): // "FLASHLIGHT_RANGE"
		{
			m_Flashlight_Range = _pKey->GetThisValuei();
			break;
		}

	case MHASH4('ELEC','TRO_','SURF','ACE'): // "ELECTRO_SURFACE"
		{
			m_iElectroSurface = m_pWServer->GetMapData()->GetResourceIndex_Surface(KeyValue);
			break;
		}

	case MHASH5('SOUN','D_AC','CESS','DENI','ED'): // "SOUND_ACCESSDENIED"
		{
			m_iSound_AccessDenied = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	default:
		{
			return CRPG_Object_Weapon::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}

	return true;
}

/*int CRPG_Object_Rifle::OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon)
{
	if(_bNoSound || SendMsg(_iObject, OBJMSG_CHAR_USEDNAWEAPONS, 2) == 1) // Assume that if there is a sound to be played, it is a traditional pickup
		return CRPG_Object_Weapon::OnPickup(_iObject, _pRoot, _bNoSound, _iSender, _bNoPickupIcon);
	else
	{
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		if(!pObj)
			return false;

		// Sound
		m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_AccessDenied, 0);

		// Damage
		CWO_DamageMsg Msg(2, 0);
		Msg.Send(_iObject, 0, m_pWServer);

		// Pickup surface
		if(m_iElectroSurface > 0)
			SendMsg(_iSender, OBJMSG_RPG_SETTEMPPICKUPSURFACE, m_iElectroSurface, 2 * SERVER_TICKSPERSECOND);

		return false;
	}
}*/

bool CRPG_Object_Rifle::CanUse(int32 _iChar)
{
	return (SendMsg(_iChar, OBJMSG_CHAR_USEDNAWEAPONS, 2) == 1);
}

bool CRPG_Object_Rifle::Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant, bool _bHold)
{
	SendMsg(_iObject, OBJMSG_CHAR_SETFLASHLIGHTCOLOR, m_Flashlight_Color, m_Flashlight_Range);

	if (m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)
		EquipDualSupport(_iObject, _pRoot, _bInstant, this);

	return CRPG_Object_Weapon::Equip(_iObject, _pRoot, _bInstant, _bHold);
}

bool CRPG_Object_Rifle::Unequip(int _iObject, CRPG_Object *_pRoot, bool _bInstant, bool _bHold)
{
	if (m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)
		UnequipDualSupport(_iObject, _pRoot, _bInstant, this);

	return CRPG_Object_Weapon::Unequip(_iObject, _pRoot,_bInstant, _bHold);
}

bool CRPG_Object_Rifle::DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout)
{
	FireMuzzle(_iObject);

	return CRPG_Object_Weapon::DrawAmmo(_Num, _Mat, _pRoot, _iObject, _bApplyFireTimeout);
}

bool CRPG_Object_Rifle::OnProcess(CRPG_Object *_pRoot, int _iObject)
{
	RefreshMuzzle(_iObject);
	if(m_Flags & RPG_ITEM_FLAGS_FLASHLIGHT)
	{	// Add 200 light to the ai/char when flashlight is on
		SendMsg(_iObject,OBJMSG_CHAR_RAISEVISIBILITY,200,-1);
	}
	return CRPG_Object_Weapon::OnProcess(_pRoot, _iObject);
}

bool CRPG_Object_Rifle::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	if (_pRoot)
	{
		CRPG_Object_Char *pRPGChar = (CRPG_Object_Char *)_pRoot;
		CRPG_Object_Item *pItem = pRPGChar->GetInventory(1)->GetEquippedItem();
		if(m_iItemType == 0x10 && pItem && pItem->m_iItemType == 0x111)
		{
			// Riot-guard dual weapon hack
			pRPGChar->QuickActivateItem(pItem, _Mat, _iObject, _Input);
		}
	}
	
	return CRPG_Object_Weapon::Activate(_Mat, _pRoot, _iObject, _Input);
}

void CRPG_Object_Rifle::OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	CRPG_Object_Weapon::OnDeltaLoad(_pFile, _pCharacter);

	if (m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)
		LoadDualSupport(_pFile, _pCharacter);

	int8 bFlashLight;
	_pFile->ReadLE(bFlashLight);
	if(bFlashLight)
		m_Flags |= RPG_ITEM_FLAGS_FLASHLIGHT;
	else
		m_Flags &= ~RPG_ITEM_FLAGS_FLASHLIGHT;
}

void CRPG_Object_Rifle::OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	CRPG_Object_Weapon::OnDeltaSave(_pFile, _pCharacter);

	if (m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)
		SaveDualSupport(_pFile, _pCharacter);

	int8 bFlashLight = (m_Flags & RPG_ITEM_FLAGS_FLASHLIGHT) != 0;
	_pFile->WriteLE(bFlashLight);
}

// Ancient weapon
MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Ancient, CRPG_Object_Rifle);
void CRPG_Object_Ancient::OnCreate()
{
	CRPG_Object_Rifle::OnCreate();
	m_DarknessDrainAmount = 0;
}

bool CRPG_Object_Ancient::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
{
	switch (_KeyHash)
	{
	case MHASH5('DARK','NESS','DRAI','NAMO','UNT'): // "DARKNESSDRAINAMOUNT"
		{
			m_DarknessDrainAmount = _pKey->GetThisValue().Val_int();
			return 1;
			break;
		}
	default:
		{
			return CRPG_Object_Rifle::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}
void CRPG_Object_Ancient::OnFinishEvalKeys()
{
	CRPG_Object_Rifle::OnFinishEvalKeys();
	m_Flags2 |= RPG_ITEM_FLAGS2_DRAINSDARKNESS;
}
//virtual int OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon = false);
bool CRPG_Object_Ancient::DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout)
{
	// Drain darknessamount from _iObject...
	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DRAWDARKNESSAMOUNT,m_DarknessDrainAmount),_iObject);
	return CRPG_Object_Rifle::DrawAmmo(_Num, _Mat, _pRoot, _iObject, _bApplyFireTimeout);
}

bool CRPG_Object_Ancient::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	// Make sure we have anough darkness left to shoot the guns
	if (_Input == 1)
		return CRPG_Object_Rifle::Activate(_Mat, _pRoot, _iObject, _Input);

	aint DarknessLeft = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DRAWDARKNESSAMOUNT,0), _iObject);
	if(DarknessLeft >= m_DarknessDrainAmount)
		return CRPG_Object_Rifle::Activate(_Mat, _pRoot, _iObject, _Input);

	return false;
}

bool CRPG_Object_Ancient::IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input)
{
	if(_iObject == 0)
		return CRPG_Object_Rifle::IsActivatable(_pRoot, _iObject, _Input);


	int Wait = ((CRPG_Object_Char*)_pRoot)->Wait() + m_iExtraActivationWait - GetGameTick(_iObject);
	if (Wait > 0)
		return false;

	aint DarknessLeft = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DRAWDARKNESSAMOUNT,0), _iObject);
	if(DarknessLeft >= m_DarknessDrainAmount)
		return true;

	return false;
}
