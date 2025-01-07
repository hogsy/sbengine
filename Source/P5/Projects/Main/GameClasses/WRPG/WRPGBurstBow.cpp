#include "PCH.h"

//#include "WRPGSpell.h"
#include "WRPGInitParams.h"
#include "WRPGChar.h"
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
#include "../WObj_Char.h"
#include "WRPGAmmo.h"
#include "WRPGBow.h"
#include "../WObj_Weapons/WObj_BurstArrow.h"

// -------------------------------------------------------------------
//  CBurstBow
// -------------------------------------------------------------------

CBurstBow::CBurstBow()
{
	MAUTOSTRIP(CBurstBow_ctor, MAUTOSTRIP_VOID);
	// Needed for ConfluctVariables::OnIncludeClass()
	m_KeyPrefix = "BURST_";
}

void CBurstBow::OnCreate()
{
	MAUTOSTRIP(CBurstBow_OnCreate, MAUTOSTRIP_VOID);
	m_KeyPrefix = "BURST_";

	m_XOffset = 0.0f;
	m_YOffset = 0.5f;
	m_XSpread = 0.1f;
	m_YSpread = 0.1f;
	m_SpreadUnity = 0.8f;
	m_ColumnSpread = 5.0f;
	m_VelocitySpread = 0.1f;

	m_iDefaultAnim = 0;
	m_iCurAnim = m_iDefaultAnim;

	m_iReloadingAmmoItemType = RPG_ITEMTYPE_UNDEFINED;

	m_ElapsedAnimTicks = 0;
	m_AnimDurationTicks = 0;

	for (int iAnimMode = 0; iAnimMode < NUM_ANIMMODES; iAnimMode++)
	{
		m_iChargeAnim[iAnimMode] = -1;
		m_iReleaseAnim[iAnimMode] = -1;
		m_iReloadAnim[iAnimMode] = -1;
	}

	m_SubChargeAnimStart = 0.0f;
	m_SubChargeAnimHold = 3.0f;
	m_SubChargeAnimEnd = 5.0f;

	m_bQuickShot = false;
	m_QuickShot_ReleaseTime = LERP(m_SubChargeAnimStart, m_SubChargeAnimHold, 0.1f);
	m_QuickShot_VelocityFraction = 0.5f;

	m_iChargeSound = 0;
	m_iReleaseSound = 0;
	m_iReloadSound = 0;
	m_ChargeSoundActive = false;
}

// -------------------------------------------------------------------

void CBurstBow::OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CBurstBow_OnIncludeClass, MAUTOSTRIP_VOID);
	ConflictVariables::OnIncludeClass(_pReg, _pMapData, _pWServer);
	m_Base->IncludeAnimFromKey("BURST_ANIM_CHARGE", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("BURST_ANIM_RELEASE", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("BURST_ANIM_RELOAD", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("BURST_ANIM_CHARGE_CROUCH", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("BURST_ANIM_RELEASE_CROUCH", _pReg, _pMapData);
	m_Base->IncludeAnimFromKey("BURST_ANIM_RELOAD_CROUCH", _pReg, _pMapData);
	m_Base->IncludeSoundFromKey("BURST_SOUND_CHARGE", _pReg, _pMapData);
	m_Base->IncludeSoundFromKey("BURST_SOUND_RELEASE", _pReg, _pMapData);
	m_Base->IncludeSoundFromKey("BURST_SOUND_RELOAD", _pReg, _pMapData);
}

// -------------------------------------------------------------------

bool CBurstBow::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CBurstBow_OnEvalKey, false);
/*
	switch (_KeyHash)
	{
	case MHASH3('MINV','ELOC','ITY'): // "MINVELOCITY"
		{
			m_MinVelocity = _pKey->GetThisValuef();
			break;
		}
	switch (_KeyHash)
	{
	case MHASH3('MAXV','ELOC','ITY'): // "MAXVELOCITY"
		{
			m_MaxVelocity = _pKey->GetThisValuef();
			break;
		}
*/

	switch (_KeyHash)
	{
	case MHASH2('XSPR','EAD'): // "XSPREAD"
		{
			m_XSpread = _pKey->GetThisValuef();
			break;
		}
	case MHASH2('YSPR','EAD'): // "YSPREAD"
		{
			m_YSpread = _pKey->GetThisValuef();
			break;
		}
	case MHASH2('XOFF','SET'): // "XOFFSET"
		{
			m_XOffset = _pKey->GetThisValuef();
			break;
		}
	case MHASH2('YOFF','SET'): // "YOFFSET"
		{
			m_YOffset = _pKey->GetThisValuef();
			break;
		}
	case MHASH3('SPRE','ADUN','ITY'): // "SPREADUNITY"
		{
			m_SpreadUnity = _pKey->GetThisValuef();
			break;
		}
	case MHASH3('COLU','MNSP','READ'): // "COLUMNSPREAD"
		{
			m_ColumnSpread = _pKey->GetThisValuef();
			break;
		}
	case MHASH4('VELO','CITY','SPRE','AD'): // "VELOCITYSPREAD"
		{
			m_VelocitySpread = _pKey->GetThisValuef();
			break;
		}
	case MHASH3('BURS','T_DA','MAGE'): // "BURST_DAMAGE"
		{
			m_Damage.Parse(_pKey->GetThisValue(), CRPG_Object::ms_DamageTypeStr);
			break;
		}
	case MHASH5('BURS','T_AN','IM_C','HARG','E'): // "BURST_ANIM_CHARGE"
		{
			m_iChargeAnim[ANIMMODE_STAND] = m_Base->ResolveAnimHandle(_pKey->GetThisValue());
			break;
		}
	case MHASH6('BURS','T_AN','IM_C','HARG','E_CR','OUCH'): // "BURST_ANIM_CHARGE_CROUCH"
		{
			m_iChargeAnim[ANIMMODE_CROUCH] = m_Base->ResolveAnimHandle(_pKey->GetThisValue());
			break;
		}
	case MHASH5('BURS','T_AN','IM_R','ELEA','SE'): // "BURST_ANIM_RELEASE"
		{
			m_iReleaseAnim[ANIMMODE_STAND] = m_Base->ResolveAnimHandle(_pKey->GetThisValue());
			break;
		}
	case MHASH7('BURS','T_AN','IM_R','ELEA','SE_C','ROUC','H'): // "BURST_ANIM_RELEASE_CROUCH"
		{
			m_iReleaseAnim[ANIMMODE_CROUCH] = m_Base->ResolveAnimHandle(_pKey->GetThisValue());
			break;
		}
	case MHASH5('BURS','T_AN','IM_R','ELOA','D'): // "BURST_ANIM_RELOAD"
		{
			m_iReloadAnim[ANIMMODE_STAND] = m_Base->ResolveAnimHandle(_pKey->GetThisValue());
			break;
		}
	case MHASH6('BURS','T_AN','IM_R','ELOA','D_CR','OUCH'): // "BURST_ANIM_RELOAD_CROUCH"
		{
			m_iReloadAnim[ANIMMODE_CROUCH] = m_Base->ResolveAnimHandle(_pKey->GetThisValue());
			break;
		}
	case MHASH5('SUBC','HARG','EANI','MSTA','RT'): // "SUBCHARGEANIMSTART"
		{
			m_SubChargeAnimStart = _pKey->GetThisValuef();
			break;
		}
	case MHASH5('SUBC','HARG','EANI','MHOL','D'): // "SUBCHARGEANIMHOLD"
		{
			m_SubChargeAnimHold = _pKey->GetThisValuef();
			break;
		}
	case MHASH4('SUBC','HARG','EANI','MEND'): // "SUBCHARGEANIMEND"
		{
			m_SubChargeAnimEnd = _pKey->GetThisValuef();
			break;
		}

	case MHASH6('QUIC','KSHO','T_RE','LEAS','ETIM','E'): // "QUICKSHOT_RELEASETIME"
		{
			m_QuickShot_ReleaseTime = _pKey->GetThisValuef();
			break;
		}
	case MHASH7('QUIC','KSHO','T_VE','LOCI','TYUF','RACT','ION'): // "QUICKSHOT_VELOCITYUFRACTION"
		{
			m_QuickShot_VelocityFraction = _pKey->GetThisValuef();
			break;
		}
	
	case MHASH5('BURS','T_SO','UND_','CHAR','GE'): // "BURST_SOUND_CHARGE"
		{
			m_iChargeSound = m_Base->m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}
	case MHASH5('BURS','T_SO','UND_','RELE','ASE'): // "BURST_SOUND_RELEASE"
		{
			m_iReleaseSound = m_Base->m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}
	case MHASH5('BURS','T_SO','UND_','RELO','AD'): // "BURST_SOUND_RELOAD"
		{
			m_iReloadSound = m_Base->m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
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

int CBurstBow::OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner)
{
	MAUTOSTRIP(CBurstBow_OnMessage, 0);
	switch (_Msg.m_Msg)
	{
		case OBJMSG_BOW_GETBURSTCURVELOCITY:
			{
				fp32 ElapsedAnimTime = m_ElapsedAnimTicks * SERVER_TIMEPERFRAME;
				fp32 ChargeFraction = Max(0.0f, Min((ElapsedAnimTime - m_SubChargeAnimStart) / (m_SubChargeAnimHold - m_SubChargeAnimStart), 1.0f));
				return *((int*)(&ChargeFraction));
			}
			break;
			
		case OBJMSG_BOW_GETMAXLOADABLE:
			{
				CRPG_Object_BurstAmmo* pAmmo = GetBurstAmmo(_pParent);
				if (pAmmo == NULL)
					return 0;

				return pAmmo->GetMagazineSize();
			}
			break;

		case OBJMSG_BOW_GETCURRENTLYLOADED:
			{
				CRPG_Object_BurstAmmo* pAmmo = GetBurstAmmo(_pParent);
				if (pAmmo == NULL)
					return 0;

				return pAmmo->GetNumLoaded();
			}
			break;
	}

	return 0;
}

// -------------------------------------------------------------------

int CBurstBow::GetAnimMode(int _iObject)
{
	MAUTOSTRIP(CBurstBow_GetAnimMode, 0);
	CWObject *pObj = m_Base->m_pWServer->Object_Get(_iObject);
	if (pObj == NULL)
		return ANIMMODE_STAND;
	
	bool IsCrouching = ((CWObject_Character::Char_GetPhysType(pObj) == PLAYER_PHYS_CROUCH) != 0);

	return (IsCrouching ? ANIMMODE_CROUCH : ANIMMODE_STAND);
}

// -------------------------------------------------------------------

void CBurstBow::PlaySound(int _iObject, int _iSound)
{
	MAUTOSTRIP(CBurstBow_PlaySound, MAUTOSTRIP_VOID);
	CWObject *pObj = m_Base->m_pWServer->Object_Get(_iObject);
	if (pObj == NULL)
		return;

	if (_iSound != 0)
	{
//		pObj->m_iSound[0] = _iSound;
		m_Base->m_pWServer->Sound_At(pObj->GetPosition(), _iSound, 0, 0, 200);

		if (false)
			ConOut(CStrF("Playing BowSound %d", _iSound));
	}
	else
	{
//		pObj->m_iSound[0] = 0;
	}

//	m_Base->m_pWServer->Sound_At(m_Base->m_pWServer->Object_GetPosition(_iObject), _iSound, 0);
}

// -------------------------------------------------------------------

void CBurstBow::PlayAnim(int _iObject, int _iAnim, int _iSound)
{
	MAUTOSTRIP(CBurstBow_PlayAnim, MAUTOSTRIP_VOID);
//	ConOut(CStrF("PlayAnim - LastAnim = %d, Ticks = %d of %d", m_iCurAnim, m_ElapsedAnimTicks, m_AnimDurationTicks));

	m_iCurAnim = _iAnim;
	m_ElapsedAnimTicks = 0;

	m_AnimDurationTicks = m_Base->PlayAnimSequence(_iObject, _iAnim, PLAYER_ANIM_ONLYTORSO);
	
	if (_iAnim == 0)
		m_AnimDurationTicks = 0;

//	ConOut(CStrF("Playing BowAnim %d (%d ticks)", m_iCurAnim, m_AnimDurationTicks));

	PlaySound(_iObject, _iSound);
}

// -------------------------------------------------------------------

bool CBurstBow::IsAnimPlaying(int* _iAnim)
{
	MAUTOSTRIP(CBurstBow_IsAnimPlaying, false);
	for (int iAnimMode = 0; iAnimMode < NUM_ANIMMODES; iAnimMode++)
		if (_iAnim[iAnimMode] == m_iCurAnim)
			return true;

	return false;
}

// -------------------------------------------------------------------

int CBurstBow::SummonWithoutDrawAmmo(const char *_pClass, int32 _Param0, int32 _Param1, const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, CCollisionInfo *_pInfo)
{
	MAUTOSTRIP(CBurstBow_SummonWithoutDrawAmmo, 0);
	const int nParams = 2;
	int32 pParams[nParams] = { _Param0, _Param1 };
	
	int iObj = m_Base->m_pWServer->Object_Create(_pClass, _Mat, _iObject, pParams, nParams);
	return iObj;
}

// -------------------------------------------------------------------

void CBurstBow::FireArrows(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CBurstBow_FireArrows, MAUTOSTRIP_VOID);
	const CVec3Dfp32 &Pos = CVec3Dfp32::GetMatrixRow(_Mat, 3);
	const CVec3Dfp32 &Fwd = CVec3Dfp32::GetMatrixRow(_Mat, 0);
	const CVec3Dfp32 &Right = CVec3Dfp32::GetMatrixRow(_Mat, 1);
	const CVec3Dfp32 &Up = CVec3Dfp32::GetMatrixRow(_Mat, 2);
	
	CRPG_Object_BurstAmmo* pAmmo = GetBurstAmmo(_pRoot);
	if (pAmmo == NULL)
		return;

	int iRand = m_Base->GetGameTick(_iObject);

	fp32 ElapsedAnimTime = m_ElapsedAnimTicks * SERVER_TIMEPERFRAME;
	fp32 ChargeFraction = Min(1.0f, (ElapsedAnimTime - m_SubChargeAnimStart) / (m_SubChargeAnimHold - m_SubChargeAnimStart));

	if (m_bQuickShot)
		ChargeFraction = m_QuickShot_VelocityFraction;

	for (int i = 0; i < pAmmo->GetNumLoaded(); i++)
	{
		CVec3Dfp32 Dir(Fwd);
		CVec3Dfp32 IndPos = Pos;

		if (pAmmo->GetNumLoaded() > 1)
		{
			fp32 LaunchPos = (((fp32)i / (fp32)(pAmmo->GetNumLoaded() - 1)) - 0.5f);

			fp32 randx = (2.0f * (MFloat_GetRand(iRand++) - 0.5f));
			fp32 randy = (2.0f * (MFloat_GetRand(iRand++) - 0.5f));

			fp32 XSpread = m_XOffset + m_XSpread * LERP(randx, LaunchPos, m_SpreadUnity);
			fp32 YSpread = m_YOffset + m_YSpread * LERP(randy, 0.0f, m_SpreadUnity);
			Dir += Right * XSpread;
			Dir += Up * YSpread;
			Dir.Normalize();

			IndPos += Right * LaunchPos * m_ColumnSpread;
		}
		else
		{
			Dir += Right * m_XOffset;
			Dir += Up * m_YOffset;
			Dir.Normalize();
		}

		CMat43fp32 Mat;
		Dir.SetMatrixRow(Mat, 0);
		CVec3Dfp32(0, 1, 0).SetMatrixRow(Mat, 1);
		Mat.RecreateMatrix(0, 1);
		IndPos.SetMatrixRow(Mat, 3);
		
		fp32 IndividualChargeFraction;
		IndividualChargeFraction = ChargeFraction +	m_VelocitySpread * MFloat_GetRand(iRand++);
		IndividualChargeFraction = Max(0.0f, Min(IndividualChargeFraction, 1.0f));

//		SummonWithoutDrawAmmo(NULL, IndividualChargeFraction, Mat, _pRoot, _iObject, NULL);
//		SummonWithoutDrawAmmo(m_Base->m_Spawn, IndividualChargeFraction, m_Damage, Mat, _pRoot, _iObject, NULL);

//		m_Base->m_pWServer->Debug_RenderMatrix(Mat, 20.0f);
		
		CRPG_InitParams InitParams(_iObject, _pRoot, m_Base);
		InitParams.m_pDamage = &m_Damage;
		InitParams.m_Velocity = IndividualChargeFraction;
		InitParams.m_VelocityType = CRPG_InitParams::VelocityFraction;

		int iProjectile = SummonWithoutDrawAmmo(pAmmo->GetSpawnClass(), (int32)&InitParams, 0, Mat, _pRoot, _iObject, NULL);
/*
		CWObject_BurstArrow* pProjectile = TDynamicCast<CWObject_BurstArrow>(m_Base->m_pWServer->Object_Get(iProjectile));
		if (pProjectile != NULL)
		{
			fp32 Velocity = LERP(pProjectile->m_MinVelocity, pProjectile->m_MaxVelocity, );
			pProjectile->SetVelocity(pProjectile->GetForward() * Velocity);
		}
*/
	}
}

// -------------------------------------------------------------------

CRPG_Object_BurstAmmo* CBurstBow::GetBurstAmmo(CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CBurstBow_GetBurstAmmo, NULL);
	CRPG_Object_BurstAmmo* pAmmo = safe_cast<CRPG_Object_BurstAmmo>(m_Base->GetAssociatedAmmo(_pRoot));
	return pAmmo;
}

// -------------------------------------------------------------------

bool CBurstBow::Equip(int _iObject, CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CBurstBow_Equip, false);
//	if (TotalAmmoLeft(_pRoot) >= m_NumArrows)
//		PlayAnim(_iObject, m_iReloadAnim[GetAnimMode(_iObject)], m_iReloadSound);
	return true;
}

// -------------------------------------------------------------------

bool CBurstBow::Unequip(int _iObject, CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CBurstBow_Unequip, false);
	PlayAnim(_iObject, 0, 0);

	CRPG_Object_BurstAmmo* pAmmo = GetBurstAmmo(_pRoot);
	if (pAmmo != NULL)
		pAmmo->Unload();

	return true;
}

// -------------------------------------------------------------------

bool CBurstBow::EquipSecondary(int _iObject, CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CBurstBow_EquipSecondary, false);
	return true;
}

// -------------------------------------------------------------------

bool CBurstBow::UnequipSecondary(int _iObject, CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CBurstBow_UnequipSecondary, false);
	return true;
}

// -------------------------------------------------------------------

bool CBurstBow::Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CBurstBow_Activate, false);
	CRPG_Object_BurstAmmo* pAmmo = GetBurstAmmo(_pRoot);
	if (pAmmo == NULL)
		return false;
	
	if ((m_iCurAnim == m_iDefaultAnim) && (pAmmo->GetNumLoaded() > 0))
		PlayAnim(_iObject, m_iChargeAnim[GetAnimMode(_iObject)], 0); // No sound now, delayed until true charge subseq.

	return true;
}

// -------------------------------------------------------------------

bool CBurstBow::Deactivate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CBurstBow_Deactivate, false);
	CRPG_Object_BurstAmmo* pAmmo = GetBurstAmmo(_pRoot);
	if (pAmmo == NULL)
		return false;
	
	fp32 ElapsedAnimTime = m_ElapsedAnimTicks * SERVER_TIMEPERFRAME;
	
	// Is really charging and may release?
	if (IsAnimPlaying(m_iChargeAnim))
	{
		if ((ElapsedAnimTime >= m_SubChargeAnimStart) &&
			(ElapsedAnimTime <= m_SubChargeAnimEnd))
		{
			// Unleash hell...
			FireArrows(_Mat, _pRoot, _iObject);

			pAmmo->ComsumeAllLoaded();

			// Apply reloadtime.
			m_Base->ApplyWait(_iObject, m_Base->m_FireTimeout, m_Base->m_NoiseLevel, m_Base->m_Visibility);

			PlayAnim(_iObject, m_iReleaseAnim[GetAnimMode(_iObject)], m_iReleaseSound);
			m_ChargeSoundActive = false;

		}
		else
		{ // Released at invalid time in animation, so return to default anim to allow restart.
			if (ElapsedAnimTime < m_SubChargeAnimStart)
			{
				m_bQuickShot = true;
			}
			else
			{
				// Don't remove playing anim, since it includes the lower part after timeout. (Eh, WHAT?!)
				PlayAnim(_iObject, m_iDefaultAnim, 0);
			}
		}
	}

	return true;
}

// -------------------------------------------------------------------

bool CBurstBow::ActivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CBurstBow_ActivateSecondary, false);
	CRPG_Object_BurstAmmo* pAmmo = GetBurstAmmo(_pRoot);
	if (pAmmo == NULL)
		return false;
	
	if ((m_iCurAnim == 0) &&
		(pAmmo->GetNumLoaded() < pAmmo->GetMagazineSize()) && 
		(pAmmo->GetNumLoaded() < pAmmo->GetNumTotal()))
	{
		m_iReloadingAmmoItemType = m_Base->GetAssociatedAmmo(_pRoot)->m_iItemType;
		PlayAnim(_iObject, m_iReloadAnim[GetAnimMode(_iObject)], m_iReloadSound);
	}
	return true;
}

// -------------------------------------------------------------------

bool CBurstBow::DeactivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CBurstBow_DeactivateSecondary, false);
	return true;
}

// -------------------------------------------------------------------

bool CBurstBow::OnProcess(CRPG_Object *_pRoot, const CMat43fp32 &_Mat, int _iObject)
{
	MAUTOSTRIP(CBurstBow_OnProcess, false);
	CRPG_Object_BurstAmmo* pAmmo = GetBurstAmmo(_pRoot);
	if (pAmmo == NULL)
		return false;

	m_ElapsedAnimTicks++;

//		ConOut(CStrF("iCurAnim=%d, Ticks=%d/%d", m_iCurAnim, m_ElapsedAnimTicks, m_AnimDurationTicks));

	if (m_ElapsedAnimTicks > m_AnimDurationTicks)
	{
		if (m_iCurAnim != 0)
		{
			if (IsAnimPlaying(m_iChargeAnim))
			{
				PlayAnim(_iObject, m_iDefaultAnim, 0);
				m_Base->ApplyWait(_iObject, m_Base->m_FireTimeout, m_Base->m_NoiseLevel, m_Base->m_Visibility);
			}
			else if (IsAnimPlaying(m_iReleaseAnim))
			{
/*
				if (TotalAmmoLeft(_pRoot) > m_NumLoadedArrows)
					PlayAnim(_iObject, m_iReloadAnim[GetAnimMode(_iObject)], m_iReloadSound);
				else
*/
				PlayAnim(_iObject, m_iDefaultAnim, 0);
			}
			else if (IsAnimPlaying(m_iReloadAnim))
			{
				PlayAnim(_iObject, m_iDefaultAnim, 0);

				if (m_iReloadingAmmoItemType != RPG_ITEMTYPE_UNDEFINED)
				{
					CRPG_Object_Char *pChar = m_Base->GetChar(_pRoot);
					if (pChar != NULL)
					{
						CRPG_Object_Item* pItem = pChar->FindItemByType(m_iReloadingAmmoItemType);
						CRPG_Object_BurstAmmo* pAmmo = safe_cast<CRPG_Object_BurstAmmo>(pItem);
						if (pAmmo != NULL)
							pAmmo->AddLoaded(1);
					}
					
					m_iReloadingAmmoItemType = RPG_ITEMTYPE_UNDEFINED;
				}
			}
		}
		else
		{
			if (pAmmo->GetNumLoaded() == 0)
				if (pAmmo->GetNumTotal() > 0)
				{
					m_iReloadingAmmoItemType = m_Base->GetAssociatedAmmo(_pRoot)->m_iItemType;
					PlayAnim(_iObject, m_iReloadAnim[GetAnimMode(_iObject)], m_iReloadSound);
				}
		}
	}
	else
	{
		fp32 ElapsedAnimTime = m_ElapsedAnimTicks * SERVER_TIMEPERFRAME;
		
		if (IsAnimPlaying(m_iChargeAnim))
		{
			// Play the delayed charge sound.
			if (!m_ChargeSoundActive &&
				(ElapsedAnimTime >= m_SubChargeAnimStart) &&
				(ElapsedAnimTime <= m_SubChargeAnimHold))
			{
				PlaySound(_iObject, m_iChargeSound);
				m_ChargeSoundActive = true;
			}

			if (m_bQuickShot && (ElapsedAnimTime > m_QuickShot_ReleaseTime))
			{
				// Unleash hell...
				FireArrows(_Mat, _pRoot, _iObject);

				pAmmo->ComsumeAllLoaded();

				// Apply reloadtime.
				m_Base->ApplyWait(_iObject, m_Base->m_FireTimeout, m_Base->m_NoiseLevel, m_Base->m_Visibility);

				PlayAnim(_iObject, m_iReleaseAnim[GetAnimMode(_iObject)], m_iReleaseSound);
				m_ChargeSoundActive = false;

				m_bQuickShot = false;
			}

			// See if the charge subseq is expired.
			if (ElapsedAnimTime > m_SubChargeAnimHold)
			{
				m_ChargeSoundActive = false;
			}
		}
	}

	return true;	
}

// -------------------------------------------------------------------
