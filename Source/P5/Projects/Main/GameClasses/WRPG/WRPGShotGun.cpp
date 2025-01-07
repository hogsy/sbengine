#include "PCH.h"

#include "WRPGShotGun.h"
#include "../WObj_Char.h"

void CRPG_Object_ShotGun::OnCreate()
{
	CRPG_Object_Weapon::OnCreate();
	m_iSound_FlashlightOn = 0;
	m_iSound_FlashlightOff = 0;
	m_Flashlight_Color = 0xffb2d8ff;
	m_Flashlight_Range = 512;

	m_Flags |= RPG_ITEM_FLAGS_LASERBEAM;
}

void CRPG_Object_ShotGun::CycleMode(int _iObject)
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

void CRPG_Object_ShotGun::OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	CRPG_Object_Weapon::OnIncludeClass(_pReg, _pMapData, _pWServer);

	IncludeSoundFromKey("SOUND_FLASHLIGHTON", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_FLASHLIGHTOFF", _pReg, _pMapData);
}

bool CRPG_Object_ShotGun::OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
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

	default:
		{
			return CRPG_Object_Weapon::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}

	return true;
}

bool CRPG_Object_ShotGun::Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant, bool _bHold)
{
	SendMsg(_iObject, OBJMSG_CHAR_SETFLASHLIGHTCOLOR, m_Flashlight_Color, m_Flashlight_Range);

	return CRPG_Object_Weapon::Equip(_iObject, _pRoot, _bInstant, _bHold);
}

bool CRPG_Object_ShotGun::DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout)
{
	FireMuzzle(_iObject);

	return CRPG_Object_Weapon::DrawAmmo(_Num, _Mat, _pRoot, _iObject, _bApplyFireTimeout);
}

bool CRPG_Object_ShotGun::OnProcess(CRPG_Object *_pRoot, int _iObject)
{
	RefreshMuzzle(_iObject);
	if(m_Flags & RPG_ITEM_FLAGS_FLASHLIGHT)
	{	// Add 200 light to the ai/char when flashlight is on
		SendMsg(_iObject,OBJMSG_CHAR_RAISEVISIBILITY,200,-1);
	}
	return CRPG_Object_Weapon::OnProcess(_pRoot, _iObject);
}

void CRPG_Object_ShotGun::OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	CRPG_Object_Weapon::OnDeltaLoad(_pFile, _pCharacter);

	int8 bFlashLight;
	_pFile->ReadLE(bFlashLight);
	if(bFlashLight)
		m_Flags |= RPG_ITEM_FLAGS_FLASHLIGHT;
	else
		m_Flags &= ~RPG_ITEM_FLAGS_FLASHLIGHT;
}

void CRPG_Object_ShotGun::OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	CRPG_Object_Weapon::OnDeltaSave(_pFile, _pCharacter);

	int8 bFlashLight = (m_Flags & RPG_ITEM_FLAGS_FLASHLIGHT) != 0;
	_pFile->WriteLE(bFlashLight);
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_ShotGun, CRPG_Object_Weapon);




class CRPG_Object_ShotGun_Pit : public CRPG_Object_ShotGun
{
	MRTC_DECLARE;

public:
	int m_StartFadeTick;
	int m_FadeDuration;
	int m_FadeDelay;
	bool m_bSent;

	virtual void OnCreate()
	{
		CRPG_Object_ShotGun::OnCreate();
		m_Flags |= RPG_ITEM_FLAGS_FLASHLIGHT;
		m_FadeDuration = (int)(30 * m_pWServer->GetGameTicksPerSecond());
		m_FadeDelay = (int)(330 * m_pWServer->GetGameTicksPerSecond());
		m_StartFadeTick = 0;
		m_bSent = false;
	}

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
	{
		CStr Key = _pKey->GetThisName();
		if(Key == "FADE_DURATION")
		{
			m_FadeDuration = (int)(_pKey->GetThisValuef() * m_pWServer->GetGameTicksPerSecond());
			m_StartFadeTick = m_pWServer->GetGameTick();
			if(m_StartFadeTick == 0)
				m_StartFadeTick++;
		}
		else if(Key == "FADE_DELAY")
			m_FadeDelay = (int)(_pKey->GetThisValuef() * m_pWServer->GetGameTicksPerSecond());
		
		else
			return CRPG_Object_ShotGun::OnEvalKey(_KeyHash, _pKey);

		return true;
	}

	virtual bool OnActivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
	{
		if(m_StartFadeTick == 0)
			m_StartFadeTick = GetGameTick(_iObject);
	
		return CRPG_Object_ShotGun::OnActivate(_Mat, _pRoot, _iObject, _Input);
	}

	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant, bool _bHold)
	{
		if(m_StartFadeTick != -1)
			m_Flags |= RPG_ITEM_FLAGS_FLASHLIGHT;
		
		return CRPG_Object_ShotGun::Equip(_iObject, _pRoot, _bInstant, _bHold);
	}

	virtual bool MergeItem(int _iObject, CRPG_Object_Item *_pObj)
	{
		if(m_iItemType == _pObj->m_iItemType)
		{
			CRPG_Object_ShotGun *pShotGun = TDynamicCast<CRPG_Object_ShotGun>(_pObj);
			if(pShotGun && m_Flashlight_Color != pShotGun->m_Flashlight_Color) // "Repair" Pit Shotgun
			{
				m_Flashlight_Color = pShotGun->m_Flashlight_Color;
				m_StartFadeTick = -1;
			}
		}

		return CRPG_Object_ShotGun::MergeItem(_iObject, _pObj);
	}

	virtual bool OnProcess(CRPG_Object *_pRoot, int _iObject)
	{
		CPixel32 Color;
		if(m_StartFadeTick == -1)
			Color = m_Flashlight_Color;
		else if(MRTC_RAND() < 1500)
			Color = 0;
		else if(m_StartFadeTick > 0)
		{
			int DurationTicks = GetGameTick(_iObject) - m_StartFadeTick;
			fp32 Fade;
			if(DurationTicks > m_FadeDelay)
				Fade = MaxMT(1.0f - fp32(DurationTicks - m_FadeDelay) / m_FadeDuration, 0.0f);
			else
				Fade = 1.0f;

			CPixel32 C = m_Flashlight_Color;
			Color = CPixel32(RoundToInt(C.GetR() * Fade),
							 RoundToInt(C.GetG() * Fade),
							 RoundToInt(C.GetB() * Fade), 255);
		}
		else
			Color = m_Flashlight_Color;

		SendMsg(_iObject, OBJMSG_CHAR_SETFLASHLIGHTCOLOR, Color, m_Flashlight_Range);

		return CRPG_Object_ShotGun::OnProcess(_pRoot, _iObject);
	}

	void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter)
	{
		CRPG_Object_ShotGun::OnDeltaLoad(_pFile, _pCharacter);

		int32 Duration = 0;
		_pFile->ReadLE(Duration);
		if(Duration > 0)
			m_StartFadeTick = m_pWServer->GetGameTick() - Duration;
		else
			m_StartFadeTick = Duration;
		_pFile->ReadLE(m_Flashlight_Color);
	}

	void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter)
	{
		CRPG_Object_ShotGun::OnDeltaSave(_pFile, _pCharacter);

		int32 Duration = m_StartFadeTick;
		if(Duration > 0)
			Duration = m_pWServer->GetGameTick() - Duration;
		_pFile->WriteLE(Duration);
		_pFile->WriteLE(m_Flashlight_Color);
	}


	virtual void CycleMode(int _iObject)
	{
		if(m_StartFadeTick == -1)
			CRPG_Object_ShotGun::CycleMode(_iObject);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_ShotGun_Pit, CRPG_Object_ShotGun);
