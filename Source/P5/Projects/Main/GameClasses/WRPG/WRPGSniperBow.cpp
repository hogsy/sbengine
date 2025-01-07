#include "PCH.h"

//#include "WRPGSpell.h"
#include "WRPGInitParams.h"
#include "WRPGChar.h"
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
#include "../WObj_Char.h"
#include "WRPGBow.h"

// -------------------------------------------------------------------
//  CSniperBow
// -------------------------------------------------------------------

enum
{
	PHASE_ZOOM0,
	PHASE_ZOOM1,
	PHASE_ZOOM2,
};

// -------------------------------------------------------------------

CSniperBow::CSniperBow()
{
	MAUTOSTRIP(CSniperBow_ctor, MAUTOSTRIP_VOID);
	// Needed for ConfluctVariables::OnIncludeClass()
	m_KeyPrefix = "SNIPER_";
}

// -------------------------------------------------------------------

void CSniperBow::OnCreate()
{
	MAUTOSTRIP(CSniperBow_OnCreate, MAUTOSTRIP_VOID);
	m_KeyPrefix = "SNIPER_";

	m_Range = 1000.0f;
	m_MoveBack = 1.0f;
	
	m_bFastShot = false;

	m_Zoom1 = 1;
	m_Zoom2 = 1;
	m_iPhase = PHASE_ZOOM0;
	m_bSecondaryPressed = false;

	m_AnimTime = 0;
	m_AnimDuration = 0;

	for (int iAnim = 0; iAnim < NUM_SNIPERANIMS; iAnim++)
		for (int iAnimMode = 0; iAnimMode < NUM_ANIMMODES; iAnimMode++)
			m_iAnim[iAnim][iAnimMode] = -1;

	for (int iSound = 0; iSound < NUM_SNIPERSOUNDS; iSound++)
		m_iSound[iSound] = SNIPERSOUND_NULL;

	m_CurAnim = SNIPERANIM_NULL;
	m_CurSound = SNIPERSOUND_NULL;
};

// -------------------------------------------------------------------

void CSniperBow::OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CSniperBow_OnIncludeClass, MAUTOSTRIP_VOID);
	ConflictVariables::OnIncludeClass(_pReg, _pMapData, _pWServer);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_RELOAD", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_RAISECHARGE", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_CHARGELOOP", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_RELEASELOWERRELOAD", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_RELEASELOWER", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_RELOAD_CROUCH", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_RAISECHARGE_CROUCH", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_CHARGELOOP_CROUCH", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_RELEASELOWERRELOAD_CROUCH", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_FASTSHOTRELOAD", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("SNIPER_ANIM_FASTSHOT", _pReg, _pMapData);
	m_Base->IncludeSoundFromKey("SNIPER_SOUND_CHARGE", _pReg, _pMapData);
	m_Base->IncludeSoundFromKey("SNIPER_SOUND_RELEASE", _pReg, _pMapData);
	m_Base->IncludeSoundFromKey("SNIPER_SOUND_RELOAD", _pReg, _pMapData);
}

// -------------------------------------------------------------------

bool CSniperBow::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CSniperBow_OnEvalKey, false);
	const char* ANIM[] = { "RELOAD", "RAISECHARGE", "CHARGELOOP", "RELEASELOWERRELOAD", "RELEASELOWER", "FASTSHOTRELOAD", "FASTSHOT" };
	const char* ANIMMODE[] = { "STAND", "CROUCH" };
	
	for (int iAnim = 0; iAnim < NUM_SNIPERANIMS; iAnim++)
	{
		for (int iAnimMode = 0; iAnimMode < NUM_ANIMMODES; iAnimMode++)
		{
			if (_pKey->GetThisName() == (CFStr("SNIPER_ANIM_") + ANIM[iAnim] + "_" + ANIMMODE[iAnimMode]))
			{
				m_iAnim[iAnim][iAnimMode] = m_Base->ResolveAnimHandle(_pKey->GetThisValue());
				return true;
			}
		}
	}
	
	switch (_KeyHash)
	{
	case MHASH2('ZOOM','1'): // "ZOOM1"
		{
			m_Zoom1 = _pKey->GetThisValuef();
			break;
		}
	case MHASH2('ZOOM','2'): // "ZOOM2"
		{
			m_Zoom2 = _pKey->GetThisValuef();
			break;
		}
	
	case MHASH2('RANG','E'): // "RANGE"
		{
			m_Range = _pKey->GetThisValuei();
			break;
		}
	case MHASH2('MOVE','BACK'): // "MOVEBACK"
		{
			m_MoveBack = _pKey->GetThisValuef();
			break;
		}
	case MHASH4('SNIP','ER_D','AMAG','E'): // "SNIPER_DAMAGE"
		{
			m_Damage.Parse(_pKey->GetThisValue(), CRPG_Object::ms_DamageTypeStr);
			break;
		}
	case MHASH5('SNIP','ER_S','OUND','_REL','OAD'): // "SNIPER_SOUND_RELOAD"
		{
			m_iSound[SNIPERSOUND_RELOAD] = m_Base->m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}
	case MHASH5('SNIP','ER_S','OUND','_CHA','RGE'): // "SNIPER_SOUND_CHARGE"
		{
			m_iSound[SNIPERSOUND_CHARGE] = m_Base->m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}
	case MHASH5('SNIP','ER_S','OUND','_REL','EASE'): // "SNIPER_SOUND_RELEASE"
		{
			m_iSound[SNIPERSOUND_RELEASE] = m_Base->m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}
	default:
		{
			ConflictVariables::OnEvalKey(_pKey);
			break;
		}
	}

	return true;
}

// -------------------------------------------------------------------

int CSniperBow::OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner)
{
	MAUTOSTRIP(CSniperBow_OnMessage, 0);
	switch (_Msg.m_Msg)
	{
		case OBJMSG_BOW_GETCURRENTLYLOADED:
			{
				CRPG_Object_SniperAmmo* pAmmo = GetSniperAmmo(_pParent);
				if (pAmmo == NULL)
					return 0;

				return pAmmo->GetNumLoaded();
			}
			break;
	}

	return 0;
}

// -------------------------------------------------------------------

int CSniperBow::GetAnimMode(int _iObject)
{
	MAUTOSTRIP(CSniperBow_GetAnimMode, 0);
	CWObject *pObj = m_Base->m_pWServer->Object_Get(_iObject);
	if (pObj == NULL)
		return false;
	
	bool IsCrouching = ((CWObject_Character::Char_GetPhysType(pObj) == PLAYER_PHYS_CROUCH) != 0);

	return (IsCrouching ? ANIMMODE_CROUCH : ANIMMODE_STAND);
}

// -------------------------------------------------------------------

void CSniperBow::PlaySound(int _iObject, SNIPERSOUND _Sound)
{
	MAUTOSTRIP(CSniperBow_PlaySound, MAUTOSTRIP_VOID);
	CWObject *pObj = m_Base->m_pWServer->Object_Get(_iObject);
	if (pObj == NULL)
		return;

	m_CurSound = _Sound;

	if (m_CurSound != SNIPERSOUND_NULL)
	{
		//pObj->m_iSound[0] = m_iSound[m_CurSound];
		m_Base->m_pWServer->Sound_At(pObj->GetPosition(), m_iSound[m_CurSound], 0);
	}
}

// -------------------------------------------------------------------

void CSniperBow::PlayAnim(int _iObject, SNIPERANIM _Anim)
{
	MAUTOSTRIP(CSniperBow_PlayAnim, MAUTOSTRIP_VOID);
	m_CurAnim = _Anim;

	int iCurAnim;

	if (m_CurAnim != SNIPERANIM_NULL)
		iCurAnim = m_iAnim[m_CurAnim][GetAnimMode(_iObject)];
	else
		iCurAnim = 0;

	int AnimDurationTicks = m_Base->PlayAnimSequence(_iObject, iCurAnim, PLAYER_ANIM_ONLYTORSO);

	if (iCurAnim == 0)
		AnimDurationTicks = 0;

	m_AnimTime = 0.0f;
	m_AnimDuration = AnimDurationTicks * SERVER_TIMEPERFRAME;
}

// -------------------------------------------------------------------

int CSniperBow::SummonWithoutDrawAmmo(const char *_pClass, int32 _Param0, int32 _Param1, const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, CCollisionInfo *_pInfo)
{
	MAUTOSTRIP(CSniperBow_SummonWithoutDrawAmmo, 0);
	const int nParams = 2;
	int32 pParams[nParams] = { _Param0, _Param1 };

	int iObj = m_Base->m_pWServer->Object_Create(_pClass, _Mat, _iObject, pParams, nParams);

	return iObj;
}

// -------------------------------------------------------------------

void CSniperBow::FireArrow(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CSniperBow_FireArrow, MAUTOSTRIP_VOID);
/*	
	const CVec3Dfp32 &Pos = CVec3Dfp32::GetMatrixRow(_Mat, 3);
	const CVec3Dfp32 &Fwd = CVec3Dfp32::GetMatrixRow(_Mat, 0);
	const CVec3Dfp32 &Right = CVec3Dfp32::GetMatrixRow(_Mat, 1);
	const CVec3Dfp32 &Up = CVec3Dfp32::GetMatrixRow(_Mat, 2);

	CMat4Dfp32 Mat;
	
	CCollisionInfo Info;
	Info.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	if (m_Base->TraceRay(_Mat, m_Range, &Info, 0, 0, _iObject))
	{
		Mat = _Mat;
		(Info.m_Pos - Fwd * m_MoveBack).SetMatrixRow(Mat, 3);
	}
	else
	{
		Mat = _Mat;
		(Pos + Fwd * m_Range).SetMatrixRow(Mat, 3);
	}
*/
	
//	SummonWithoutDrawAmmo(m_Base->m_Spawn, 0, m_Damage, Mat, _pRoot, _iObject, &Info);

	CRPG_Object_SniperAmmo* pAmmo = GetSniperAmmo(_pRoot);
	if (pAmmo != NULL)
	{
		CRPG_InitParams InitParams(_iObject, _pRoot, m_Base);
		InitParams.m_pDamage = &m_Damage;

		int iProjectile = SummonWithoutDrawAmmo(pAmmo->GetSpawnClass(), (int32)&InitParams, 0, _Mat, _pRoot, _iObject, NULL);
	}
}

// -------------------------------------------------------------------

CRPG_Object_SniperAmmo* CSniperBow::GetSniperAmmo(CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CSniperBow_GetSniperAmmo, NULL);
	return safe_cast<CRPG_Object_SniperAmmo>(m_Base->GetAssociatedAmmo(_pRoot));
}

// -------------------------------------------------------------------

bool CSniperBow::IsZoomed()
{
	MAUTOSTRIP(CSniperBow_IsZoomed, false);
	return (m_iPhase != PHASE_ZOOM0);
}

// -------------------------------------------------------------------

void CSniperBow::SetZoom(int _iObject, int _iPhase, fp32 _ZoomSpeed, fp32 _MaxZoom)
{
	MAUTOSTRIP(CSniperBow_SetZoom, MAUTOSTRIP_VOID);
	m_iPhase = _iPhase;
	int32 ZoomSpeed = int32(_ZoomSpeed * 65536.0f);
	int32 MaxZoom = int32(_MaxZoom * 65536.0f);
	m_Base->m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETZOOM, ZoomSpeed, MaxZoom), _iObject);

	if (_MaxZoom > 1.0f)
		m_Base->m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADJUSTMOVEMENT, -0.5f * 65536.0f, 1.0f * 65536.0f), _iObject);
	else
		m_Base->m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADJUSTMOVEMENT, 0.0f * 65536.0f, 1.0f * 65536.0f), _iObject);
}

// -------------------------------------------------------------------

bool CSniperBow::Equip(int _iObject, CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CSniperBow_Equip, false);
	SetZoom(_iObject, PHASE_ZOOM0, 0.0f, 1.0f);
	m_bSecondaryPressed = false;

	return true;
}

// -------------------------------------------------------------------

bool CSniperBow::Unequip(int _iObject, CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CSniperBow_Unequip, false);
	SetZoom(_iObject, PHASE_ZOOM0, 0.0f, 1.0f);
	m_bSecondaryPressed = false;
	
	PlayAnim(_iObject, SNIPERANIM_NULL);

	CRPG_Object_SniperAmmo* pAmmo = GetSniperAmmo(_pRoot);
	if (pAmmo != NULL)
		pAmmo->Unload();

	return true;
}

// -------------------------------------------------------------------

bool CSniperBow::EquipSecondary(int _iObject, CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CSniperBow_EquipSecondary, false);
	SetZoom(_iObject, PHASE_ZOOM0, 0.0f, 1.0f);
	m_bSecondaryPressed = false;

	return true;
}

// -------------------------------------------------------------------

bool CSniperBow::UnequipSecondary(int _iObject, CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CSniperBow_UnequipSecondary, false);
	SetZoom(_iObject, PHASE_ZOOM0, 0.0f, 1.0f);
	m_bSecondaryPressed = false;
	
	return true;
}

// -------------------------------------------------------------------

bool CSniperBow::Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CSniperBow_Activate, false);
	CRPG_Object_SniperAmmo* pAmmo = GetSniperAmmo(_pRoot);
	if (pAmmo == NULL)
		return false;
	
	if (IsZoomed())
	{
		if ((m_CurAnim == SNIPERANIM_CHARGELOOP) && (pAmmo->GetNumLoaded() > 0))
		{
			FireArrow(_Mat, _pRoot, _iObject);

			pAmmo->ConsumeLoaded(1);

//			m_Base->ApplyWait(_iObject, m_Base->m_FireTimeout);

			PlaySound(_iObject, SNIPERSOUND_RELEASE);

			if (pAmmo->GetNumTotal() > 0)
			{
				PlayAnim(_iObject, SNIPERANIM_RELEASELOWERRELOAD);
				PlaySound(_iObject, SNIPERSOUND_RELOAD);
			}
			else
				PlayAnim(_iObject, SNIPERANIM_RELEASELOWER);
		}
	}
	else
	{
		if ((m_CurAnim == SNIPERANIM_NULL) && (pAmmo->GetNumLoaded() > 0))
		{
			m_bFastShot = true;
			PlayAnim(_iObject, SNIPERANIM_RAISECHARGE);
		}
	}

	return true;
}

// -------------------------------------------------------------------

bool CSniperBow::ActivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CSniperBow_ActivateSecondary, false);
	if (!m_bSecondaryPressed)
	{
		if (m_iPhase == PHASE_ZOOM0)
			SetZoom(_iObject, PHASE_ZOOM1, 0, m_Zoom1);
		else if (m_iPhase == PHASE_ZOOM1)
			SetZoom(_iObject, PHASE_ZOOM2, 0, m_Zoom2);
		else if (m_iPhase == PHASE_ZOOM2)
			SetZoom(_iObject, PHASE_ZOOM0, 0, 1);

		m_bSecondaryPressed = true;
	}

	return true;
}

// -------------------------------------------------------------------

bool CSniperBow::DeactivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CSniperBow_DeactivateSecondary, false);
	m_bSecondaryPressed = false;
	return true;
}

// -------------------------------------------------------------------

bool CSniperBow::OnProcess(CRPG_Object *_pRoot, const CMat43fp32 &_Mat, int _iObject)
{
	MAUTOSTRIP(CSniperBow_OnProcess, false);
	CRPG_Object_SniperAmmo* pAmmo = GetSniperAmmo(_pRoot);
	if (pAmmo == NULL)
		return false;
	
	m_AnimTime += SERVER_TIMEPERFRAME;

	if (m_AnimTime >= m_AnimDuration)
	{
		if (m_CurAnim != SNIPERANIM_NULL)
		{
			switch (m_CurAnim)
			{
				case SNIPERANIM_RELOAD:
				case SNIPERANIM_RELEASELOWERRELOAD:
				{
					PlayAnim(_iObject, SNIPERANIM_NULL);

					CRPG_Object_SniperAmmo* pAmmo = GetSniperAmmo(_pRoot);
					if (pAmmo != NULL)
						pAmmo->AddLoaded(1);
					break;
				}

				case SNIPERANIM_RAISECHARGE:
					if (!m_bFastShot)
					{
						PlayAnim(_iObject, SNIPERANIM_CHARGELOOP);
					}
					else
					{
						FireArrow(_Mat, _pRoot, _iObject);

						pAmmo->ConsumeLoaded(1);

//						m_Base->ApplyWait(_iObject, m_Base->m_FireTimeout);

						PlaySound(_iObject, SNIPERSOUND_RELEASE);

						if (pAmmo->GetNumTotal() > 0)
						{
							PlayAnim(_iObject, SNIPERANIM_RELEASELOWERRELOAD);
							PlaySound(_iObject, SNIPERSOUND_RELOAD);
						}
						else
							PlayAnim(_iObject, SNIPERANIM_RELEASELOWER);

						m_bFastShot = false;
					}
					break;

				case SNIPERANIM_RELEASELOWER:
					PlayAnim(_iObject, SNIPERANIM_NULL);
					break;
					
			}
		}
		else
		{
			if (pAmmo->GetNumLoaded() == 0)
			{
				if (pAmmo->GetNumTotal() > pAmmo->GetNumLoaded())
				{
					PlayAnim(_iObject, SNIPERANIM_RELOAD);
					PlaySound(_iObject, SNIPERSOUND_RELOAD);
				}
			}
			else 
			{
				if (IsZoomed())
				{
					PlayAnim(_iObject, SNIPERANIM_RAISECHARGE);
					PlaySound(_iObject, SNIPERSOUND_CHARGE);
				}
			}
		}
	}

	if ((m_CurAnim == SNIPERANIM_CHARGELOOP) && (!IsZoomed()))
		PlayAnim(_iObject, SNIPERANIM_NULL);

	CWObject_CoreData* pObj = m_Base->m_pWServer->Object_GetCD(_iObject);
	if (!IsAlive(pObj))
		SetZoom(_iObject, PHASE_ZOOM0, 0.0f, 1.0f);

	return true;	
}

// -------------------------------------------------------------------
