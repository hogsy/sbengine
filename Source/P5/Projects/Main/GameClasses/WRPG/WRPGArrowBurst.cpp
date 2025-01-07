/*
*Crossbow_Stand			Main\Assassin:118
*Crossbow_StandLegs		Main\Assassin:34
*Crossbow_Crouch		Main\Assassin:119		
*Crossbow_Run_Fwd		Main\Assassin:120
*Crossbow_Run_Bwd		Main\Assassin:120
*Crossbow_Walk_Fwd		Main\Assassin:122		
*Crossbow_Walk_Bwd		Main\Assassin:123		
*Crossbow_Crouch_Fwd		Main\Assassin:124		
*Crossbow_Crouch_Bwd		Main\Assassin:125		
*Crossbow_Jump			Main\Assassin:126		
*Crossbow_Fall			Main\Assassin:127
*Crossbow_Fire			Main\Assassin:129
*Crossbow_Fire_Crouch		Main\Assassin:131
*Crossbow_Fire_WalkFwd		Main\Assassin:206
*Crossbow_Fire_WalkBwd		Main\Assassin:207
*Crossbow_Fire_RunFwd		Main\Assassin:203
*Crossbow_Fire_RunBwd		Main\Assassin:203
*Crossbow_Fire_CrouchFwd	Main\Assassin:134
*Crossbow_Fire_CrouchBwd	Main\Assassin:204
*Crossbow_Reload		Main\Assassin:130
*Crossbow_Reload_Crouch		Main\Assassin:132
*Crossbow_Reload_CrouchFwd	Main\Assassin:205
*/

#include "PCH.h"

#include "WRPGSpell.h"
#include "WRPGChar.h"
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
#include "../WObj_Char.h"
#include "WRPGAmmo.h"

// -------------------------------------------------------------------
//  CRPG_Object_ArrowBurst
// -------------------------------------------------------------------

enum
{
	ANIMMODE_NORMAL = 0,
	ANIMMODE_CROUCH = 1,
	NUM_ANIMMODES = 2,
};

// -------------------------------------------------------------------

class CRPG_Object_ArrowBurst : public CRPG_Object_Summon
{
	MRTC_DECLARE;

private:

	int m_MaxNumLoadedArrows;
	int m_NumLoadedArrows;

	fp32 m_XSpread, m_YSpread;
	fp32 m_SpreadUnity;
	fp32 m_ColumnSpread;
	fp32 m_VelocitySpread;

	int m_iCurAnim;
	int m_ElapsedAnimTicks;
	int m_AnimDurationTicks;

	int m_iDefaultAnim;
	int m_iChargeAnim[NUM_ANIMMODES];
	int m_iReleaseAnim[NUM_ANIMMODES];
	int m_iReloadAnim[NUM_ANIMMODES];

	fp32 m_SubChargeAnimStart;
	fp32 m_SubChargeAnimEnd;

	int m_iChargeSound;
	int m_iReleaseSound;
	int m_iReloadSound;
	bool m_ChargeSoundActive;

public:

	void OnCreate()
	{
		CRPG_Object_Summon::OnCreate();

		m_MaxNumLoadedArrows = 1;
		m_NumLoadedArrows = 0;

		m_XSpread = 0.1f;
		m_YSpread = 0.1f;
		m_SpreadUnity = 0.8f;
		m_ColumnSpread = 5.0f;
		m_VelocitySpread = 0.1f;

		m_iDefaultAnim = 0;
		m_iCurAnim = m_iDefaultAnim;

		m_ElapsedAnimTicks = 0;
		m_AnimDurationTicks = 0;

		for (int iAnimMode = 0; iAnimMode < NUM_ANIMMODES; iAnimMode++)
		{
			m_iChargeAnim[iAnimMode] = 0;
			m_iReleaseAnim[iAnimMode] = 0;
			m_iReloadAnim[iAnimMode] = 0;
		}

		m_SubChargeAnimStart = 0.0f;
		m_SubChargeAnimEnd = 10.0f;

		m_iChargeSound = 0;
		m_iReleaseSound = 0;
		m_iReloadSound = 0;
		m_ChargeSoundActive = false;
	}

	void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData)
	{
		CRPG_Object_Summon::OnIncludeClass(_pReg, _pMapData);

		IncludeAnimFromKey("ANIM_CHARGE", _pReg, _pMapData);
		IncludeAnimFromKey("ANIM_CHARGE_CROUCH", _pReg, _pMapData);
		IncludeAnimFromKey("ANIM_RELEASE", _pReg, _pMapData);
		IncludeAnimFromKey("ANIM_RELEASE_CROUCH", _pReg, _pMapData);
		IncludeAnimFromKey("ANIM_RELOAD", _pReg, _pMapData);
		IncludeAnimFromKey("ANIM_RELOAD_CROUCH", _pReg, _pMapData);
	}

	bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		switch (_KeyHash)
		{
		case MHASH4('MAXL','OADE','DARR','OWS'): // "MAXLOADEDARROWS"
			{
				m_MaxNumLoadedArrows = _pKey->GetThisValuei();
				break;
			}
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
		case MHASH3('ANIM','_CHA','RGE'): // "ANIM_CHARGE"
			{
				m_iChargeAnim[ANIMMODE_NORMAL] = ResolveAnimHandle(_pKey->GetThisValue());
				break;
			}
		case MHASH5('ANIM','_CHA','RGE_','CROU','CH'): // "ANIM_CHARGE_CROUCH"
			{
				m_iChargeAnim[ANIMMODE_CROUCH] = ResolveAnimHandle(_pKey->GetThisValue());
				break;
			}
		case MHASH3('ANIM','_REL','EASE'): // "ANIM_RELEASE"
			{
				m_iReleaseAnim[ANIMMODE_NORMAL] = ResolveAnimHandle(_pKey->GetThisValue());
				break;
			}
		case MHASH5('ANIM','_REL','EASE','_CRO','UCH'): // "ANIM_RELEASE_CROUCH"
			{
				m_iReleaseAnim[ANIMMODE_CROUCH] = ResolveAnimHandle(_pKey->GetThisValue());
				break;
			}
		case MHASH3('ANIM','_REL','OAD'): // "ANIM_RELOAD"
			{
				m_iReloadAnim[ANIMMODE_NORMAL] = ResolveAnimHandle(_pKey->GetThisValue());
				break;
			}
		case MHASH5('ANIM','_REL','OAD_','CROU','CH'): // "ANIM_RELOAD_CROUCH"
			{
				m_iReloadAnim[ANIMMODE_CROUCH] = ResolveAnimHandle(_pKey->GetThisValue());
				break;
			}
		case MHASH5('SUBC','HARG','EANI','MSTA','RT'): // "SUBCHARGEANIMSTART"
			{
				m_SubChargeAnimStart = _pKey->GetThisValuef();
				break;
			}
		case MHASH4('SUBC','HARG','EANI','MEND'): // "SUBCHARGEANIMEND"
			{
				m_SubChargeAnimEnd = _pKey->GetThisValuef();
				break;
			}
		case MHASH3('SOUN','D_CH','ARGE'): // "SOUND_CHARGE"
			{
				m_iChargeSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
				break;
			}
		case MHASH4('SOUN','D_RE','LEAS','E'): // "SOUND_RELEASE"
			{
				m_iReleaseSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
				break;
			}
		case MHASH3('SOUN','D_RE','LOAD'): // "SOUND_RELOAD"
			{
				m_iReloadSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
				break;
			}
		default:
			{
				return CRPG_Object_Summon::OnEvalKey(_pKey);
				break;
			}
		}

		return true;
	}

	int GetAnimMode(int _iObject)
	{
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		if (pObj == NULL)
			return false;
		
		bool IsCrouching = ((pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CROUCH) != 0);

		return (IsCrouching ? ANIMMODE_CROUCH : ANIMMODE_NORMAL);
	}

	void PlaySound(int _iObject, int _iSound)
	{
		if (_iSound != 0)
			m_pWServer->Sound_At(m_pWServer->Object_GetPosition(_iObject), _iSound, 0);
	}

	void PlayAnim(int _iObject, int _iAnim, int _iSound)
	{
		m_iCurAnim = _iAnim;
		m_ElapsedAnimTicks = 0;

		m_AnimDurationTicks = PlayAnimSequence(_iObject, _iAnim, true);

		if (_iAnim == 0)
			m_AnimDurationTicks = 0;

		PlaySound(_iObject, _iSound);
	}

	bool IsAnimPlaying(int* _iAnim)
	{
		for (int iAnimMode = 0; iAnimMode < NUM_ANIMMODES; iAnimMode++)
			if (_iAnim[iAnimMode] == m_iCurAnim)
				return true;

		return false;
	}

	void SummonWithoutDrawAmmo(const char *_pClass, fp32 _Param0, const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, CCollisionInfo *_pInfo)
	{
		int iEffect = -1;
		if(m_Effect.Len())
		{
			CMat43fp32 MEffect;
			if(GetCurEffectMatrix(MEffect))
			{
				CMat43fp32 Mat;
				Mat.UnitNot3x3();
				CVec3Dfp32 Ray = CVec3Dfp32::GetMatrixRow(_Mat, 3) - CVec3Dfp32::GetMatrixRow(MEffect, 3);
				
				Ray.SetMatrixRow(Mat, 0);
				CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 2);
				CVec3Dfp32::GetMatrixRow(MEffect, 3).SetMatrixRow(Mat, 3);
				Mat.RecreateMatrix(0, 2);
				
				iEffect = m_pWServer->Object_Create(m_Effect, Mat, _iObject);
				if(iEffect)
				{
					CWObject *pObj = m_pWServer->Object_Get(iEffect);
					if(pObj)
						pObj->m_iAnim0 = Ray.Length();
				}
			}
		}
		
		int iObj = m_pWServer->Object_Create(_pClass, _Mat, _iObject);
		if(iObj > 0)
		{
			CSummonInit Init;
			Init.m_iCaster = _iObject;
			Init.m_iEffect = iEffect;
			Init.m_pCaster = _pRoot;
			Init.m_pCollisionInfo = _pInfo;
			Init.m_pSpell = this;
			Init.m_Param0 = _Param0;
			SendMsg(iObj, OBJMSG_SPELLS_SUMMONINIT, (int)&Init);
		}
	}

	void FireArrows(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
	{
		const CVec3Dfp32 &Pos = CVec3Dfp32::GetMatrixRow(_Mat, 3);
		const CVec3Dfp32 &Fwd = CVec3Dfp32::GetMatrixRow(_Mat, 0);
		const CVec3Dfp32 &Right = CVec3Dfp32::GetMatrixRow(_Mat, 1);
		const CVec3Dfp32 &Up = CVec3Dfp32::GetMatrixRow(_Mat, 2);
		
		int iRand = GetGameTick(_iObject);

		fp32 ElapsedAnimTime = m_ElapsedAnimTicks * SERVER_TIMEPERFRAME;
		fp32 ChargeFraction = (ElapsedAnimTime - m_SubChargeAnimStart) / (m_SubChargeAnimEnd - m_SubChargeAnimStart);

		for (int i = 0; i < m_NumLoadedArrows; i++)
		{
			fp32 LaunchPos = (((fp32)i / (fp32)(m_NumLoadedArrows - 1)) - 0.5f);
			fp32 randx = (2.0f * (MFloat_GetRand(iRand++) - 0.5f));
			fp32 randy = (2.0f * (MFloat_GetRand(iRand++) - 0.5f));

			CVec3Dfp32 Dir(Fwd);
			fp32 XSpread = m_XSpread * LERP(randx, LaunchPos, m_SpreadUnity);
			fp32 YSpread = m_YSpread * LERP(randy, 0.0f, m_SpreadUnity);
			Dir += Right * XSpread;
			Dir += Up * YSpread;
			Dir.Normalize();

			CVec3Dfp32 IndPos = Pos;
			IndPos += Right * LaunchPos * m_ColumnSpread;

			CMat43fp32 Mat;
			Dir.SetMatrixRow(Mat, 0);
			CVec3Dfp32(0, 1, 0).SetMatrixRow(Mat, 1);
			Mat.RecreateMatrix(0, 1);
			IndPos.SetMatrixRow(Mat, 3);
			
			fp32 IndividualChargeFraction;
			IndividualChargeFraction = ChargeFraction +	m_VelocitySpread * MFloat_GetRand(iRand++);
			IndividualChargeFraction = Max(0.0f, Min(IndividualChargeFraction, 1.0f));

			SummonWithoutDrawAmmo(m_Spawn, IndividualChargeFraction, Mat, _pRoot, _iObject, NULL);
		}
	}

	int TotalAmmoLeft(CRPG_Object *_pRoot)
	{
		return 100;

		if((_pRoot == NULL) || (_pRoot->GetType() != TYPE_CHAR))
			return 0;

		CRPG_Object_Char *pChar = (CRPG_Object_Char *)_pRoot;

		CRPG_Object_Item *pAmmo = pChar->FindItem(m_lLinkedItemTypes[m_iCurLink]);
		if (pAmmo == NULL)
			return 0;

		return pAmmo->m_NumItems;
	}

	void ConsumeLoadedAmmo(CRPG_Object *_pRoot)
	{
/*
		if((_pRoot != NULL) || (_pRoot->GetType() != TYPE_CHAR))
			return;

		CRPG_Object_Char *pChar = (CRPG_Object_Char *)_pRoot;

		CRPG_Object_Item *pAmmo = pChar->FindItem(m_lLinkedItemTypes[m_iCurLink]);
		if (pAmmo == NULL)
			return;

		pAmmo->m_NumItems -= m_NumLoadedArrows;
*/
		m_NumLoadedArrows = 0;
	}

	CRPG_Object_BurstAmmo* GetBurstAmmo(CRPG_Object *_pRoot)
	{
		if((_pRoot == NULL) || (_pRoot->GetType() != TYPE_CHAR))
			return NULL;

		CRPG_Object_Char *pChar = (CRPG_Object_Char *)_pRoot;

		return TDynamicCast<CRPG_Object_BurstAmmo>(pChar->FindItem(m_lLinkedItemTypes[m_iCurLink]));
	}

	bool Equip(int _iObject, CRPG_Object *_pRoot)
	{
//		if (TotalAmmoLeft(_pRoot) >= m_NumArrows)
//			PlayAnim(_iObject, m_iReloadAnim[GetAnimMode(_iObject)], m_iReloadSound);
		
		return CRPG_Object_Summon::Equip(_iObject, _pRoot);
	}

	bool Unequip(int _iObject, CRPG_Object *_pRoot)
	{
		m_NumLoadedArrows = 0;
	
		return CRPG_Object_Summon::Unequip(_iObject, _pRoot);
	}

	bool Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
	{
		if ((m_iCurAnim == m_iDefaultAnim) && (m_NumLoadedArrows > 0))
			PlayAnim(_iObject, m_iChargeAnim[GetAnimMode(_iObject)], 0); // No sound now, delayed until true charge subseq.

		return true;
	}

	bool Deactivate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
	{
		fp32 ElapsedAnimTime = m_ElapsedAnimTicks * SERVER_TIMEPERFRAME;
		
		// Is really charging and may release?
		if (IsAnimPlaying(m_iChargeAnim))
		{
			if ((ElapsedAnimTime >= m_SubChargeAnimStart) &&
				(ElapsedAnimTime <= m_SubChargeAnimEnd))
			{
				// Unleash hell...
				FireArrows(_Mat, _pRoot, _iObject);

				// Consume loaded ammo.
				ConsumeLoadedAmmo(_pRoot);

				// Apply reloadtime.
				ApplyWait(_iObject, m_FireTimeout);

				PlayAnim(_iObject, m_iReleaseAnim[GetAnimMode(_iObject)], m_iReleaseSound);
				m_ChargeSoundActive = false;

			}
			else
			{ // Released at invalid time in animation, so return to default anim to allow restart.
				PlayAnim(_iObject, m_iDefaultAnim, 0);
			}
		}

		return true;
	}

	bool OnProcess(CRPG_Object *_pRoot, const CMat43fp32 &_Mat, int _iObject)
	{
		m_ElapsedAnimTicks++;

//		ConOut(CStrF("iCurAnim=%d, Ticks=%d/%d", m_iCurAnim, m_ElapsedAnimTicks, m_AnimDurationTicks));

		if (m_ElapsedAnimTicks > m_AnimDurationTicks)
		{
			if (m_iCurAnim != 0)
			{
				if (IsAnimPlaying(m_iChargeAnim))
				{
					PlayAnim(_iObject, m_iDefaultAnim, 0);
					ApplyWait(_iObject, m_FireTimeout);
				}
				else if (IsAnimPlaying(m_iReleaseAnim))
				{
					if (TotalAmmoLeft(_pRoot) > m_NumLoadedArrows)
						PlayAnim(_iObject, m_iReloadAnim[GetAnimMode(_iObject)], m_iReloadSound);
					else
						PlayAnim(_iObject, m_iDefaultAnim, 0);
				}
				else if (IsAnimPlaying(m_iReloadAnim))
				{
					m_NumLoadedArrows++;
					PlayAnim(_iObject, m_iDefaultAnim, 0);
				}
			}
			else
			{
/*
				if (m_AmmoLoaded == 0)
					if (TotalAmmoLeft(_pRoot) > 0)
						PlayAnim(_iObject, m_iReloadAnim[GetAnimMode(_iObject)], m_iReloadSound);
*/
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
					(ElapsedAnimTime <= m_SubChargeAnimEnd))
				{
					PlaySound(_iObject, m_iChargeSound);
					m_ChargeSoundActive = true;
				}

				// See if the charge subseq is expired. If so, stop the charge anim.
				if (ElapsedAnimTime > m_SubChargeAnimEnd)
				{
					PlayAnim(_iObject, m_iDefaultAnim, 0);
					ApplyWait(_iObject, m_FireTimeout);
					m_ChargeSoundActive = false;
				}
			}
		}

		return CRPG_Object_Item::OnProcess(_pRoot, _Mat, _iObject);
	}

};

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_ArrowBurst, CRPG_Object_Item);

// -------------------------------------------------------------------
