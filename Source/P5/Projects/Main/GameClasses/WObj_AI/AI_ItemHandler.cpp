#include "PCH.h"
#include "AICore.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "../WRPG/WRPGChar.h"
#include "../WRPG/WRPGAmmo.h"
#include "../WRPG/WRPGItem.h"
#include "../WRPG/WRPGInventory.h"
#include "../WObj_CharMsg.h"
#include "../WRPG/WRPGFist.h"
#include "../WObj_Game/WObj_GameMod.h"

//WEAPONS////////////////////////////////////////////////////////////////////////////////////////////

//Constructor
CAI_Weapon::CAI_Weapon(CRPG_Object_Item * _pItem, CAI_Core * _pAI, int _iType)	
{
    MAUTOSTRIP(CAI_Weapon_ctor, MAUTOSTRIP_VOID);

	m_pItem = _pItem;
	m_pAI = _pAI;
	m_iType = _iType;
	m_iUseTimer = -1;
};


//Resets the weapon data
void CAI_Weapon::Reset()
{
	MAUTOSTRIP(CAI_Weapon_Reset, MAUTOSTRIP_VOID);

	m_iUseTimer = -1;
};


//Weapon type
int CAI_Weapon::GetType()
{
	MAUTOSTRIP(CAI_Weapon_GetType, 0);

	return m_iType;
};


//Refreshes the weapon one frame
void CAI_Weapon::OnRefresh()
{
	MAUTOSTRIP(CAI_Weapon_OnRefresh, MAUTOSTRIP_VOID);

	if (!IsValid())
		//Aargh! Invalid AI!
		return;

	if (IsWielded())
	{
		//Set use timer
		if (m_iUseTimer != -1)
			if (m_pAI->Wait() == 0)
				m_iUseTimer = -1;
			else
				m_iUseTimer++;
	}
	else
		Reset();
};



//Returns CActionEstimate values (0.100f) appropriately for taking the given action (or default 
//action if no action is specified) against the given target right now, taking such factors as 
//damage per attack, rate of attacks ammo, skill, etc into consideration. 
CAI_ActionEstimate CAI_Weapon::GetEstimate(CAI_AgentInfo* _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Weapon_GetEstimate, CAI_ActionEstimate());

	return CAI_ActionEstimate();
};

//Use the weapon if possible
void CAI_Weapon::OnUse(CAI_AgentInfo * _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Weapon_OnUse, MAUTOSTRIP_VOID);

	//We cannot use the item if this itemhandler or the target is invalid
	if (!IsValid() || !_pTarget || !_pTarget->GetObject())
		return;
	
	//Reset use timer
	m_iUseTimer = 0;

	m_pAI->m_DeviceWeapon.Use();
};

//Use the item at a position instead of a target
void CAI_Weapon::OnUse(const CVec3Dfp32& _Pos)
{
	MAUTOSTRIP(CAI_Item_OnUse2, MAUTOSTRIP_VOID);

	//We cannot use the item if this itemhandler is invalid
	if (!IsValid())
		return;
	
	//Reset use timer
	m_iUseTimer = 0;

	//..and use the item, hoping that something good happens
	m_pAI->m_DeviceWeapon.Use();
};


//Take aim at the target
void CAI_Weapon::OnAim(CAI_AgentInfo* _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_OnAim, MAUTOSTRIP_VOID);

	//We cannot aim if this itemhandler or the target is invalid
	if (!IsValid() || !_pTarget || !_pTarget->GetObject())
		return;

	//Aim at target
	m_pAI->AddAimAtPositionCmd(m_pAI->GetFocusPosition(_pTarget),2 * m_pAI->m_Reaction);
};


//Adds commands to switch to this weapon if possible
void CAI_Weapon::OnWield()
{
	MAUTOSTRIP(CAI_Weapon_OnWield, MAUTOSTRIP_VOID);

	if (!IsValid() || !m_pAI->m_DeviceWeapon.IsAvailable())
		return;

	m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, m_pItem->m_Name);
};


//Checks target is "good" i.e. can be attacked when weapon is ready for this
bool CAI_Weapon::IsGoodTarget(CAI_AgentInfo * _pTarget, int _Mode)
{
	MAUTOSTRIP(CAI_Weapon_IsGoodTarget, false);

	return false;
};

//Checks if this is a ranged weapon
bool CAI_Weapon::IsRangedWeapon()
{
	MAUTOSTRIP(CAI_Weapon_IsRangedWeapon, false);

	return (m_iType > MAX_MELEE) && (m_iType < MAX_RANGED);
};

//Checks if this is a melee weapon
bool CAI_Weapon::IsMeleeWeapon()
{
	MAUTOSTRIP(CAI_Weapon_IsMeleeWeapon, false);

	return (m_iType > UNKNOWN) && (m_iType < MAX_MELEE);
};



//Returns current number of frames since weapon was used
int CAI_Weapon::GetUseTimer()
{
	MAUTOSTRIP(CAI_Weapon_GetUseTimer, 0);

	return m_iUseTimer;
};


//Checks if the item cannot be used to inflict damage
bool CAI_Weapon::IsHarmless()
{
	MAUTOSTRIP(CAI_Weapon_IsHarmless, false);

	return true;
};

//Checks if the item is valid (i.e. has it's Item, Wielder and World pointers set)
bool CAI_Weapon::IsValid()
{
	MAUTOSTRIP(CAI_Weapon_IsValid, false);

	return	(m_pItem && m_pAI && m_pAI->IsValid());
};

//Checks if this item is wielded
bool CAI_Weapon::IsWielded()
{
	MAUTOSTRIP(CAI_Weapon_IsWielded, false);

	if (!IsValid())
		return false;

	CRPG_Object_Item * pItem = m_pAI->GetRPGItem(m_pAI->m_pGameObject->m_iObject, RPG_CHAR_INVENTORY_WEAPONS);
	if (pItem && (m_pItem == pItem))
		return true;
	else
	{
		CRPG_Object_Inventory * pInv = m_pAI->GetRPGInventory(m_pAI->m_pGameObject->m_iObject, RPG_CHAR_INVENTORY_WEAPONS);
		if (pInv && (m_pItem->m_iItemType == pInv->GetSelectedItemType()))
			return true;
		else 
			return false;
	}
};

//Returns the RPG item
CRPG_Object_Item * CAI_Weapon::GetItem()
{
	MAUTOSTRIP(CAI_Weapon_GetItem, NULL);

	return m_pItem;
};

//Check if given position is within usable arc of given AI with given item
bool CAI_Weapon::IsInUsableFOV(const CVec3Dfp32& _Position, CAI_Core * _pUserAI, CRPG_Object_Item * _pItem)
{
	// Is item usable regardless of angle?
	if (_pItem->m_AIUseFOV >= 1.0f)
		return true;
	
	// Some FOV restriction
	return (_pUserAI->AngleOff(_Position, true) <= (_pItem->m_AIUseFOV / 2.0f));
};

/* Deprecated
//Returns the truncated current aiming score
int CAI_Weapon::GetAimingScore(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_GetAimingScore, 0);

	return 0;
};

//Returns the current aiming score delta per frame when aiming at the given target
fp32 CAI_Weapon::GetAimingScoreDelta(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_GetAimingScoreDelta, 0.0f);

	return 0;
};
*/

//Returns maximum effective range against the specified target for estimation purposes. 
//If no target is given this simply returns the effective range against a singularity.
fp32 CAI_Weapon::MaxRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_MaxRange, 0.0f);

	return 0;
};

//Returns minimum effective range, agaist the specified target. If no target is given 
//this simply returns the minimum range against a singularity.
fp32 CAI_Weapon::MinRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_MinRange, 0.0f);

	return 0;
};

//Change AI user
void CAI_Weapon::ReInit(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_Weapon_ReInit, MAUTOSTRIP_VOID);

	m_pAI = _pAI;
	Reset();
};

//Calculate the activateposition matrix for this weapons wielder.
void CAI_Weapon::GetActivatePosition(CMat4Dfp32* _pMat)
{
	MAUTOSTRIP(CAI_Weapon_ReInit, MAUTOSTRIP_VOID);

	if (!IsValid() || !_pMat)
		return;

	//Just set matrix to current look matrix but...
	*_pMat = m_pAI->m_pGameObject->GetPositionMatrix();
	(*_pMat).GetRow(3) = m_pAI->GetTrueAimPosition();
};



//CAI_Weapon_Ranged : Base handler for ranged weapons /////////////////////////////////

const fp32 CAI_Weapon_Ranged::MAX_UNCERTAINTY = 24.0f;

//Constructor
CAI_Weapon_Ranged::CAI_Weapon_Ranged(CRPG_Object_Item * _pItem, CAI_Core * _pAI, int _iType)	
	: CAI_Weapon(_pItem, _pAI, _iType)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_ctor, MAUTOSTRIP_VOID);

	m_TargetPos = CVec3Dfp32(_FP32_MAX);
	m_iTarget = 0;
	m_AimingScore = 0.0f;
	m_StationaryBonus = 0.0f;
	m_bAiming = false;
};

//Reset stuff
void CAI_Weapon_Ranged::Reset()
{
	MAUTOSTRIP(CAI_Weapon_Ranged_Reset, MAUTOSTRIP_VOID);

	CAI_Weapon::Reset();
	m_TargetPos = CVec3Dfp32(_FP32_MAX);
	m_iTarget = 0;
	m_AimingScore = 0.0f;
	m_StationaryBonus = 0.0f;
	m_bAiming = false;
};


//Checks if there is a line of sight to the target
bool CAI_Weapon_Ranged::IsLineOfSight(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_IsLineOfSight, false);

	if (!_pTarget)
		return false;

	//Save the target position, in case we want to shoot. This will be (_FP32_MAX, _FP32_MAX, _FP32_MAX) 
	//if we don't have LOS.
	if (_pTarget->InLOS(&m_TargetPos))
	{
		//Have LOS!
		return true;
	}
	else
	{
		//Don't have LOS!
		return false;
	};
}; 


//Check how much ammo there's left. Check ammo in magazine only if the optional flag is true
//This returns 100 if weapon does not draw ammo.
int CAI_Weapon_Ranged::CheckAmmo(bool _bMagazineOnly)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_CheckAmmo, 0);

	if (!IsValid())
		return 0;

	int nAmmo = m_pAI->m_pGameObject->AI_GetRPGAttribute((_bMagazineOnly) ? AIQUERY_RPGATTRIBUTE_LOADEDAMMO : AIQUERY_RPGATTRIBUTE_AMMO, m_pItem->m_iItemType);
	if (nAmmo == -1) 
		nAmmo = 100;
	return nAmmo;
}


//Refreshes the weapon one frame
void CAI_Weapon_Ranged::OnRefresh()
{
	MAUTOSTRIP(CAI_Weapon_Ranged_OnRefresh, MAUTOSTRIP_VOID);

	if (!m_pAI || !m_pAI->IsValid())
		//Aargh! Invalid AI!
		return;

	if (IsWielded())
	{
		//Reset target position; this is set when calling GetEstimate, so if OnUse
		//is called the same frame as GetEstimate, the former method will not need 
		//to find a target position
		m_TargetPos = CVec3Dfp32(_FP32_MAX);

		CAI_AgentInfo* pInfo = m_pAI->m_KB.GetAgentInfo(m_iTarget);
		CWObject* pTarget;
		if (pInfo && (pTarget = pInfo->GetObject()))
		{
			if (m_bAiming)
			{
				/* Deprecated
				//Modify aiming score appropriately.
				m_AimingScore += GetAimingScoreDelta(pInfo);
				*/

				//There's an aiming score bonus as long as target stay stationary
				if (pTarget->GetPosition().AlmostEqual(pTarget->GetLastPosition(), 0.1))
				{
					if (m_AimingScore < 100)
					{
						m_AimingScore += m_pAI->m_StationaryBonus;
						m_StationaryBonus += m_pAI->m_StationaryBonus;
					}
				}
				else
				{
					//Target moved, lose stationary bonus
					m_AimingScore -= m_StationaryBonus;
					m_StationaryBonus = 0;
				}

				//Aiming score can never get higher than 100 and never lower than 0
				m_AimingScore = Min(100.0f, Max(0.0f, m_AimingScore));

				//ConOut(CStrF("SCORE: %d, BONUS: %d", (int)m_AimingScore, (int)m_StationaryBonus));
			}
			else
			{
				//Didn't aim, or target position uncertain, reset aiming score and target
				m_AimingScore = 0;
				m_StationaryBonus = 0;
				m_iTarget = 0;
			};
		}
		else
		{
			//No target, reset aiming score and target
			m_AimingScore = 0;
			m_StationaryBonus = 0;
			m_iTarget = 0;
		};

		//Reset aiming flag
		m_bAiming = false;
	}

	CAI_Weapon::OnRefresh();	
};


//Returns CActionEstimate values (0.100f) appropriately for taking the given action (or default 
//action if no action is specified) against the given target right now, taking such factors as 
//damage per attack, rate of attacks ammo, skill, etc into consideration. 
CAI_ActionEstimate CAI_Weapon_Ranged::GetEstimate(CAI_AgentInfo* _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_GetEstimate, Est);

	//Hugely simplified. Will fix.
	//Shurely mr Olsson you're jesting
	
	//The estimate should really be calculated by a formula based on the damage, the time 
	//until an attack can be performed, chance of hitting the target, skill etc.
	CAI_ActionEstimate Est; 

	//Estimate is always 0 if this itemhandler or the target is invalid
	if (!IsValid() || !_pTarget || !_pTarget->GetObject())
	{
		return Est;
	};

	if (m_pAI->Wait())
	{	//If we cannot use it right now the item is considered useless (gross simplification!)
		return Est;
	}
	else if (CheckAmmo() == 0)
	{	//If we're out of ammo, the weapon is considered useless
		return Est;
	}
	else
	{	// We don't give a flying fuck about LOS and dispersion etc
		fp32 MinRangeSqr = Sqr(MinRange(_pTarget));
		fp32 MaxRangeSqr = Sqr(MaxRange(_pTarget));
		fp32 RangeSqr = _pTarget->GetBasePos().DistanceSqr(m_pAI->GetBasePos());
		// Offensive estimate
		// 0 inside MinRange
		// 0 outside MaxRange
		if ((RangeSqr < MinRangeSqr)||(RangeSqr > MaxRangeSqr))
		{
			return Est;
		}
		else
		{
			Est.Set(CAI_ActionEstimate::OFFENSIVE,100);
			return Est;
		}
	}
};
 


//Use the weapon if possible
void CAI_Weapon_Ranged::OnUse(CAI_AgentInfo * _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_OnUse, MAUTOSTRIP_VOID);

	if (!_pTarget || !_pTarget->GetObject() || !m_pAI)
		return;

	//Nullify aiming score and change target if this is not the target we've been aiming at
	if (_pTarget->GetObjectID() != m_iTarget)
	{
		m_AimingScore = 0.0f;
		m_iTarget = _pTarget->GetObjectID();
	}

	//Are there shots left?
	if (CheckAmmo() > 0)
	{
		//Reset use timer
		m_iUseTimer = 0;

		//Aim at the target focus position 
		// *** m_pAI->AddAimAtPositionCmd(m_pAI->GetFocusPosition(_pTarget->GetObject()));
		//..and let loose the dogs of war! Or at least an arrow or something...
		m_pAI->m_DeviceWeapon.Use();

		//Aiming score is halved due to recoil
		m_AimingScore /= 2;
	}
	else
	{
		//Switch to next weapon instead
		// *** m_pAI->AddAimAtPositionCmd(m_pAI->GetFocusPosition(_pTarget->GetObject()));
		m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH_NEXT);
	}

	//We're using weapon!
	m_bAiming = true;
};

//Use the weapon if possible
void CAI_Weapon_Ranged::OnUse(const CVec3Dfp32& _Pos)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_OnUse2, MAUTOSTRIP_VOID);

	if (!m_pAI)
		return;


	//Nullify aiming score and change target if this is not the target we've been aiming at
	if (!_Pos.AlmostEqual(m_TargetPos,0.5f))
	{
		m_AimingScore = 0.0f;
	}
	if (_Pos != CVec3Dfp32(_FP32_MAX))
	{
		m_TargetPos = _Pos;
	}
	m_iTarget = 0;

	//Are there shots left?
	if (CheckAmmo() > 0)
	{
		//Reset use timer
		m_iUseTimer = 0;
		m_pAI->m_DeviceWeapon.Use();

		//Aiming score is halved due to recoil
		m_AimingScore /= 2;
	}
	else
	{
		//Switch to next weapon instead
		m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH_NEXT);
	}

	//We're using weapon!
	m_bAiming = true;
};


//Take aim at the target
void CAI_Weapon_Ranged::OnAim(CAI_AgentInfo* _pTarget)
{
	MAUTOSTRIP( CAI_Weapon_Ranged_OnAim, MAUTOSTRIP_VOID);

	//We cannot aim if this itemhandler or the target is invalid
	if (!IsValid() || !_pTarget || !_pTarget->GetObject())
		return;

	//Aim at target
	m_pAI->AddAimAtPositionCmd(m_pAI->GetFocusPosition(_pTarget),2 * m_pAI->m_Reaction);
	//If this is a new target, reset aiming score and set new target, 
	if (_pTarget->GetObjectID() != m_iTarget)
	{
		m_AimingScore = 0.0f;
		m_iTarget = _pTarget->GetObjectID()	;
	};

	//We're aiming!
	m_bAiming = true;
};


//Checks target is "good" i.e. is in LOS and effective range interval
bool CAI_Weapon_Ranged::IsGoodTarget(CAI_AgentInfo * _pTarget, int _Mode)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_IsGoodTarget, false);

	if (!_pTarget || !IsValid())
		return false;

	CAI_AgentCriteria GoodTarget;
 	if (_Mode == MODE_MELEE)
	{
		//Hardcoded range
		GoodTarget.Distance(m_pAI->m_pGameObject->GetPosition(), 64, 0);
	}
	else
	{
		//Default ranged mode
		GoodTarget.InfoFlags(CAI_AgentInfo::IN_LOS);
		GoodTarget.Distance(m_pAI->m_pGameObject->GetPosition(), MaxRange(_pTarget), MinRange(_pTarget));
	}

	return _pTarget->Check(GoodTarget);
};

/*
//Returns the truncated current aiming score
int CAI_Weapon_Ranged::GetAimingScore(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_GetAimingScore, 0);

	if (_pTarget && _pTarget->GetObjectID() == m_iTarget)
		return (int)m_AimingScore;
	else
		return 0;
};


//Returns the current aiming score delta per frame when aiming at the given target
fp32 CAI_Weapon_Ranged::GetAimingScoreDelta(CAI_AgentInfo* _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_GetAimingScoreDelta, 0.0f);

	//Aiming score can never exceed 50

	//Aiming score is only changed every other frame when weapon is ready to be fired
	if (m_pAI->Wait())
	{
		return 0;
	}
	else
	{
		if (m_pAI->m_Timer % 2)
			return 0;
	};

	//Aiming score is increased relative to precision (modified by own movement) 
	//and reduced relative to 1-complement of target movement
 	return ( (50 - m_AimingScore) * 0.5f * (m_pAI->m_Skill * 0.01f) * m_pAI->PrecisionModifier(_pTarget->GetObject(), CAI_Core::OWN_MOVEMENT) -
			 (36 * (1 - m_pAI->PrecisionModifier(_pTarget->GetObject(), CAI_Core::TARGET_MOVEMENT))) );
};
*/


//Returns maximum effective range agaist the specified target for estimation purposes. 
//If no target is given this simply returns the effective range against a singularity.
fp32 CAI_Weapon_Ranged::MaxRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_MaxRange, 0.0f);

	return INFINITE_RANGE;
};


//Returns minimum effective range, agaist the specified target. If no target is given 
//this simply returns the minimum range against a singularity.
fp32 CAI_Weapon_Ranged::MinRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_MinRange, 0.0f);

	return 0;
};


//Calculate the activateposition matrix for this weapons wielder. This adds some scatter due to lack of skill etc to the given matrix
void CAI_Weapon_Ranged::GetActivatePosition(CMat4Dfp32* _pMat)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_GetActivatePosition, MAUTOSTRIP_VOID);

	if (!m_pAI || !m_pAI->IsValid() || !_pMat)
		return;

	// We scatter wether we have a target or not
	CAI_AgentInfo* pTarget = NULL;					// Target object if any
	CVec3Dfp32 OwnAimPos = m_pAI->GetAimPosition();	// Pos where we fire from
	CVec3Dfp32 AimOffset = CVec3Dfp32(0.0f);
	// The weapon may have an aim offset
	if ((m_pItem) && (m_pItem->m_AimOffset != CVec3Dfp32(0.0f)))
	{
		AimOffset = m_pItem->m_AimOffset;
		AimOffset.MultiplyMatrix3x3(*_pMat);
		OwnAimPos += AimOffset;
	}
	else
	{	// *** Use offset from activate pos and real weapon pos ***
		if ((true) && (!m_pAI->IsPlayer()) && (m_pAI->m_pGameObject) && (m_pAI->IsCharacter(m_pAI->m_pGameObject->m_iObject)))
		{
			CWObject_Character* pChar = (CWObject_Character*)m_pAI->m_pGameObject;
			if (pChar->m_ActivatePositionTick + 5 >= m_pAI->GetAITick())
			{
				OwnAimPos = pChar->m_ActivatePosition;
			}
		}
		// ***
	}
	
	CVec3Dfp32 Aim;									// Direction vector from our pos to target
	fp32 Accuracy = 0.5f;							// Quality factor affecting scatter 0.0(worst) to 1.0(best, no scatter)
	
	// Determine m_TargetPos
	if (m_iTarget)
	{
		pTarget = m_pAI->m_KB.GetAgentInfo(m_iTarget);
		if (pTarget && pTarget->GetObject())
		{
			m_TargetPos = m_pAI->GetTorsoPosition(pTarget->GetObject());
			/*
			// *** Don't know if I like this RPGish jumble of code ***
			Accuracy = 0.01f * Min(100, m_pAI->m_Skill + GetAimingScore(pTarget));	//Accuracy base is skilla and aimingscore...
			Accuracy = Max(0.0f, Accuracy - (1 - (0.01f * m_pAI->m_Skill)) * (1 - m_pAI->PrecisionModifier(pTarget->GetObject())));	//..and modified by precision modifier
			Accuracy *= m_pAI->PrecisionModifier(pTarget->GetObject(), CAI_Core::VISIBILITY);							//..further modified by visibility only
			*/
			Accuracy = m_pAI->m_Skill * 0.01f;
			if (pTarget->GetCurAwareness() <= CAI_AgentInfo::DETECTED)
			{
				Accuracy *= 0.5f;
			}
		}
	}
	
	// If we don't have a 
	if (m_TargetPos == CVec3Dfp32(_FP32_MAX))
	{	// Blast away in the look direction
		// CVec3Dfp32 LookDir = CVec3Dfp32::GetMatrixRow(m_pAI->m_pGameObject->GetPositionMatrix(),0);
		CVec3Dfp32 LookDir = _pMat->GetRow(0);
		LookDir.Normalize();
		LookDir = LookDir * 160.f;	// Fire at something 5m away in the look dir
		// LookDir = ((LookDir - OwnAimPos).Normalize()) * 160.0f;
		Aim = LookDir;
		Aim[2] -= Random * 32.0f;
	}
	else
	{
		Aim = m_TargetPos - OwnAimPos;
	}
	fp32 Range = Aim.Length();						// Range to intended target pos
	CVec3Dfp32 Scatter = CVec3Dfp32(0.0f,0.0f,0.0f);	// Deviation from intended target
	
	// *** if ((m_pAI->ms_Gamestyle == CWObject_GameP4::GAMESTYLE_FIGHT)&&(!m_pAI->IsPlayer()))
	if ((!m_pAI->IsPlayer())&&(m_pAI->m_pAIResources->ms_Gamestyle != CWObject_GameP4::GAMESTYLE_KILL))
	{	// *** We check separately for gamestyle because the Accuracy scatter MAY be something
		// we may always want to have in the game (to increase the fun)
		if (Accuracy > 0.5f)
		{
			Accuracy = 0.5f;
		}
		{	// We scatter randomly by 0.1 * Range * (1-Accuracy) in each dimension
			Scatter[0] += 0.2f * Range * (1.0f-Accuracy) * (Random - 0.5f);
			Scatter[1] += 0.2f * Range * (1.0f-Accuracy) * (Random - 0.5f);
			Scatter[2] += 0.2f * Range * (1.0f-Accuracy) * (Random - 0.5f);
		};
	}

	if ((pTarget)&&
		(m_pAI->m_pAIResources->ms_Gamestyle == CWObject_GameP4::GAMESTYLE_FIGHT)&&
		(!m_pAI->IsPlayer())
		&&(m_pAI->m_pAIResources->ms_GameDifficulty < DIFFICULTYLEVEL_HARD))
	{
		if (Random * 100 > (m_pAI->m_HitRatio - m_pAI->m_SniperPenalty))
		{
			const CBox3Dfp32* pBox;
			if ((pBox = pTarget->GetObject()->GetAbsBoundBox()))
			{
				CVec3Dfp32 ScatterBox = pBox->m_Max - pBox->m_Min;

				if (ScatterBox[0] > 0.1f * Range)
				{
					ScatterBox[0] = 0.1f * Range;
				}
				if (ScatterBox[1] > 0.1f * Range)
				{
					ScatterBox[1] = 0.1f * Range;
				}
				if (ScatterBox[2] > 0.1f * Range)
				{
					ScatterBox[2] = 0.1f * Range;
				}
				if (Scatter[0] >= 0.0f)
				{
					Scatter[0] += ScatterBox[0] * (Random+0.5f);
				}
				else
				{
					Scatter[0] -= ScatterBox[0] * (Random+0.5f);
				}
				if (Scatter[1] >= 0.0f)
				{
					Scatter[1] += ScatterBox[0] * (Random+0.5f);
				}
				else
				{
					Scatter[1] -= ScatterBox[0] * (Random+0.5f);
				}
				if (Scatter[2] >= 0.0f)
				{
					Scatter[2] += ScatterBox[0] * (Random+0.5f);
				}
				else
				{
					Scatter[2] -= ScatterBox[0] * (Random+0.5f);
				}
			}
		}
	}

	// Target to scatter
#ifndef M_RTM
	if ((m_pAI->DebugTarget())&&(!m_pAI->m_DeviceWeapon.IsAvailable()))
	{
		m_pAI->Debug_RenderWire(OwnAimPos+Aim,OwnAimPos+Aim+Scatter,0xffffffff,1);
	}
#endif
	Aim += Scatter;
#ifndef M_RTM
	if ((m_pAI->DebugTarget())&&(!m_pAI->m_DeviceWeapon.IsAvailable()))
	{
		// Weapon to scattered target
		m_pAI->Debug_RenderWire(OwnAimPos,OwnAimPos+Aim, 0xffffffff,1);
	}
#endif
	Aim.Normalize();
	//Set matrix to aim according to calculated aim vector
	Aim.SetMatrixRow(*_pMat, 0);
 	if(M_Fabs(Aim * CVec3Dfp32(0, 0, 1)) < 0.99f)
		CVec3Dfp32(0, 0, 1).SetMatrixRow(*_pMat, 2);
	else
		CVec3Dfp32(0, 1, 0).SetMatrixRow(*_pMat, 2);
	(*_pMat).RecreateMatrix(0, 2);

	//Set matrix position to aim position
	// m_pAI->GetAimPosition(m_pAI->m_pGameObject).SetMatrixRow(*_pMat, 3);
	OwnAimPos.SetMatrixRow(*_pMat, 3);

	return;
};


//Checks if the item cannot be used to inflict damage. 
bool CAI_Weapon_Ranged::IsHarmless()
{
	MAUTOSTRIP(CAI_Weapon_Ranged_IsHarmless, false);

	//Assume all ranged weapons are harmfull unless they're out of ammo, except for weapons
	//that use mana (since mana will regenerate)
	return (CheckAmmo() == 0);	
};



//CAI_Weapon_Melee : Base handler for melee weapons/////////////////////

//Constructor
CAI_Weapon_Melee::CAI_Weapon_Melee(CRPG_Object_Item * _pItem, CAI_Core * _pAI, int _iType)
	: CAI_Weapon(_pItem, _pAI, _iType) 
{
	MAUTOSTRIP(CAI_Weapon_Melee_ctor, MAUTOSTRIP_VOID);
}


//Is the target within striking distance?
bool CAI_Weapon_Melee::InMeleeRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Melee_InMeleeRange, false);

	return (_pTarget &&
			m_pAI &&
			((m_pAI->GetCenterPosition(_pTarget->GetObject())).DistanceSqr(m_pAI->GetCenterPosition(m_pAI->m_pGameObject)) < Sqr(MaxRange(_pTarget))));
};


//Is the target within minimum melee range?
bool CAI_Weapon_Melee::InMinMeleeRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Melee_InMinMeleeRange, false);

	return (_pTarget &&
			m_pAI &&
			((m_pAI->GetCenterPosition(_pTarget->GetObject())).DistanceSqr(m_pAI->GetCenterPosition(m_pAI->m_pGameObject)) < Sqr(MinRange(_pTarget))));
};


//Get the corresponding melee device action for the given action
int CAI_Weapon_Melee::GetMeleeAction(int _Action)
{
	//Always attack right for now if there is an action
	if (_Action)
	{
		//return CAI_Device_Melee::ATTACK_RIGHT;	
		return CAI_Device_Melee::ATTACK_PUNCH;
	}
	else 
	{
		return 0;
	}
};


//Returns CActionEstimate values (0.100f) appropriately for taking the given action (or default 
//action if no action is specified) against the given target right now, taking such factors as 
//damage per attack, rate of attacks ammo, skill, etc into consideration. 
CAI_ActionEstimate CAI_Weapon_Melee::GetEstimate(CAI_AgentInfo* _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Weapon_Melee_GetEstimate, 0);

	//Hugely simplified. Will fix.
	
	//The estimate should really be calculated by a formula based on the damage, the time 
	//until an attack can be performed, chance of hitting the target, skill etc.
	CAI_ActionEstimate Est = CAI_ActionEstimate();

	//Estimate is always 0 if this itemhandler or the target is invalid
	if (!IsValid() || !_pTarget || !_pTarget->GetObject())
	{
		return Est;
	};

	//If we cannot use it right now the item is considered useless (gross simplification!)
	if (m_pAI->Wait())
		return Est;
	else
	{
		//Check range. We must also pass a skill test to know when to strike unless we're at min range.
		if (InMinMeleeRange(_pTarget) ||
			(InMeleeRange(_pTarget) &&
			 (100 * Random < Sqr(m_pAI->m_Skill) * 0.01f)))
		{
			//We're in melee range and passed the skill test! "If it ain't green, belt it until it's dead. Then belt it some more for good measure."
			//Estimate depends solely upon distance for now (Range distance -> 0, MinRange distance -> 100) 
			Est.Set(CAI_ActionEstimate::OFFENSIVE, (int)(100 * (1 - (((m_pAI->GetCenterPosition(_pTarget->GetObject())).Distance(m_pAI->GetCenterPosition(m_pAI->m_pGameObject)) - MinRange(_pTarget)) / (MaxRange() - MinRange())))));
		}

		return Est;
	}
};


//Checks target is "good" i.e. in melee range
bool CAI_Weapon_Melee::IsGoodTarget(CAI_AgentInfo * _pTarget, int _Mode)
{
	MAUTOSTRIP(CAI_Weapon_Melee_IsGoodTarget, false);

	if (!_pTarget)
		return false;

	return (InMeleeRange(_pTarget));
};


//Take the given action if possible
void CAI_Weapon_Melee::OnUse(CAI_AgentInfo * _pTarget, int _Action)
{
	//We cannot use the item if this itemhandler or the target is invalid
	if (!IsValid() || !_pTarget)
		return;
	
	//Take given melee action
	if (_Action != 0)
	{
		m_pAI->m_DeviceMelee.Use(GetMeleeAction(_Action));

		//Lock weapon and look device, and run base class method
		m_pAI->m_DeviceWeapon.Lock();
		m_pAI->m_DeviceLook.Lock();
	}
	CAI_Weapon::OnUse(_pTarget, _Action);
};


//Returns maximum effective range agaist the specified target for estimation purposes. 
//If no target is given this simply returns the effective range against a singularity.
fp32 CAI_Weapon_Melee::MaxRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Melee_MaxRange, 0.0f);

	if (!IsValid())
		return 0;
	
	fp32 BaseSize = 0;
	if (_pTarget && _pTarget->GetObject())
		BaseSize = 0.5f * m_pAI->GetBoundBoxDimensions(_pTarget->GetObject())[0];

	//Should be some weapon value...fix
	return 88 + BaseSize;
};


//Returns minimum effective range, agaist the specified target. If no target is given 
//this simply returns the minimum range against a singularity.
fp32 CAI_Weapon_Melee::MinRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Melee_MinRange, 0.0f);

	if (!IsValid())
		return 0;

	fp32 BaseSize = 0;
	if (_pTarget && _pTarget->GetObject())
		BaseSize = 0.5f * m_pAI->GetBoundBoxDimensions(_pTarget->GetObject())[0];

	//Should be some weapon value...fix
	return 12 + BaseSize;
};


//Checks if the item cannot be used to inflict damage. 
bool CAI_Weapon_Melee::IsHarmless()
{
	MAUTOSTRIP(CAI_Weapon_Melee_IsHarmless, false);

	//Assume all melee weapons are harmful
	return false;
};





//CAI_Weapon_Melee_Fist : Base handler for fist-fighting/////////////////////

//Constructor
CAI_Weapon_Melee_Fist::CAI_Weapon_Melee_Fist(CRPG_Object_Item * _pItem, CAI_Core * _pAI, int _iType)
	: CAI_Weapon_Melee(_pItem, _pAI, _iType) 
{
	MAUTOSTRIP(CAI_Weapon_Melee_Fist_ctor, MAUTOSTRIP_VOID);
}


//Get the corresponding melee device action for the given action
int CAI_Weapon_Melee_Fist::GetMeleeAction(int _Action)
{
	/*
	//This should perhaps be defined in fist rpg class.or something...
	switch (_Action)
	{
	case FIST_ACTION_PUNCHLEFT:
		return CAI_Device_Melee::ATTACK_LEFT;

	case FIST_ACTION_PUNCHRIGHT:
		return CAI_Device_Melee::ATTACK_RIGHT;

	case FIST_ACTION_LEFTDODGEPUNCH:
		return CAI_Device_Melee::ATTACK_LEFT | CAI_Device_Melee::DODGE_LEFT;

	case FIST_ACTION_RIGHTDODGEPUNCH:
		return CAI_Device_Melee::ATTACK_RIGHT | CAI_Device_Melee::DODGE_RIGHT;

	case FIST_MOVE_ENTERDODGELEFT:
		return CAI_Device_Melee::DODGE_LEFT;

	case FIST_MOVE_ENTERDODGERIGHT:
		return CAI_Device_Melee::DODGE_RIGHT;

	case FIST_MOVE_ENTERBLOCK:
		return CAI_Device_Melee::DEFEND;

	case FIST_MOVE_LEAVEBLOCK:
		return CAI_Device_Melee::RESUME;

	case FIST_ACTION_GRAB:
	case FIST_ACTION_BREAKGRAB:
		return CAI_Device_Melee::ATTACK_RIGHT | CAI_Device_Melee::ATTACK_LEFT;

	default:
		//Do nothing, action is automatic
		return 0;
	}*/
	return 0;
};


//Returns a value that estimates how suitable the weapon is to use against the specified 
//target right now, taking such factors as damage per attack, rate of attacks
//ammo, skill, etc into consideration. A value of 0 means that the weapon is useless 
//against the target. Opponent info is the action we think opponent is taking.
CAI_ActionEstimate CAI_Weapon_Melee_Fist::GetEstimate(CAI_AgentInfo* _pTarget, int _Action, int _OpponentInfo)
{
	MAUTOSTRIP(CAI_Weapon_Melee_Fist_GetEstimate, 0);

	//Hugely simplified. Will fix.
	
	//The estimate should really be calculated by a formula based on the damage, the time 
	//until an attack can be performed, chance of hitting the target, skill etc.

	//Estimate is always 0 if this itemhandler or the target is invalid
	if (!IsValid() || !_pTarget)
	{
		return CAI_ActionEstimate();
	};

	//We must be able to take action now for action to be of any use.
	//We must also be in fighting mode with target
	if ((m_pAI->Wait() == 0) &&
		(_pTarget->GetObjectID() == m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISINFIGHTMODE), m_pAI->m_pGameObject->m_iObject)))
	{
		//Evaluate the given action against the opponents action and set difficulty
		//This should be replaced by a call to a static method defined in weapon class
		//where the rules are read from a template

		//Each action has a difficulty. In order to get an estimate the bot must pass a skill test with this difficulty
		fp32 Difficulty = 1.0f;

		//Each action has a set of opponent actions that they're godd offensively and/or defensively. 
		//The "no action" action is represented by an unused flag 
		int GoodOffensive = 0;
		int GoodDefensive = 0;
		int NoAction = 1 << FIST_ACTION_NUMBEROFACTIONS;
		if (_OpponentInfo == 0)
			_OpponentInfo = NoAction;

		//Each action has a basic offensive and defensive score
		int OffensiveScore = 0;
		int DefensiveScore = 0;
		
		//Set the above stuff
		switch (_Action)
		{
		case FIST_ACTION_PUNCHLEFT:
			{
				Difficulty = 1;
				GoodOffensive = FIST_MOVE_ENTERDODGERIGHT | FIST_MOVE_ENTERBLOCK |
								FIST_MOVE_LEAVEDODGELEFT | FIST_MOVE_LEAVEDODGERIGHT |
								FIST_MOVE_LEAVEBLOCK | FIST_STANCE_MIDDLE | FIST_STANCE_RIGHT |
								FIST_STANCE_BLOCK | FIST_HURT_MIDDLE | FIST_HURT_RIGHT |
								FIST_ACTION_BREAKGRAB | NoAction;
				GoodDefensive = 0;
				OffensiveScore = 75;
				DefensiveScore = 50;
			};
			break;
		case FIST_ACTION_PUNCHRIGHT:
			{
				Difficulty = 1;
				GoodOffensive = FIST_MOVE_ENTERDODGELEFT | FIST_MOVE_ENTERBLOCK |
								FIST_MOVE_LEAVEDODGERIGHT | FIST_MOVE_LEAVEDODGELEFT |
								FIST_MOVE_LEAVEBLOCK | FIST_STANCE_MIDDLE | FIST_STANCE_LEFT |
								FIST_STANCE_BLOCK | FIST_HURT_MIDDLE | FIST_HURT_LEFT |
								FIST_ACTION_BREAKGRAB | NoAction;
				GoodDefensive = 0;
				OffensiveScore = 75;
				DefensiveScore = 50;
			};
			break;
		case FIST_ACTION_LEFTDODGEPUNCH:
			{
				Difficulty = 2;
				GoodOffensive = FIST_ACTION_PUNCHLEFT | FIST_ACTION_LEFTDODGEPUNCH |
								FIST_MOVE_ENTERDODGERIGHT | FIST_MOVE_ENTERBLOCK|
								FIST_MOVE_LEAVEDODGELEFT | FIST_MOVE_LEAVEDODGERIGHT |
								FIST_MOVE_LEAVEBLOCK | FIST_STANCE_MIDDLE | FIST_STANCE_RIGHT | 
								FIST_STANCE_BLOCK | FIST_HURT_MIDDLE | FIST_HURT_RIGHT |
								FIST_ACTION_BREAKGRAB | NoAction;
				GoodDefensive = FIST_ACTION_PUNCHLEFT | FIST_ACTION_LEFTDODGEPUNCH;
				OffensiveScore = 50;
				DefensiveScore = 75;
			};
			break;
		case FIST_ACTION_RIGHTDODGEPUNCH:
			{
				Difficulty = 2;
				GoodOffensive = FIST_ACTION_PUNCHRIGHT | FIST_ACTION_RIGHTDODGEPUNCH |
								FIST_MOVE_ENTERDODGELEFT | FIST_MOVE_ENTERBLOCK |
								FIST_MOVE_LEAVEDODGELEFT | FIST_MOVE_LEAVEDODGERIGHT |
								FIST_MOVE_LEAVEBLOCK  |	FIST_STANCE_MIDDLE | FIST_STANCE_LEFT | 
								FIST_STANCE_BLOCK | FIST_HURT_MIDDLE | FIST_HURT_LEFT |
								FIST_ACTION_BREAKGRAB | NoAction;
				GoodDefensive = FIST_ACTION_PUNCHRIGHT | FIST_ACTION_RIGHTDODGEPUNCH;
				OffensiveScore = 50;
				DefensiveScore = 75;
			};
			break;
		case FIST_MOVE_ENTERDODGELEFT:
			{
				Difficulty = 1.25f;
				GoodOffensive = 0;
				GoodDefensive = FIST_ACTION_PUNCHLEFT | FIST_ACTION_LEFTDODGEPUNCH;
				OffensiveScore = 0;
				DefensiveScore = 75;
			};
			break;
		case FIST_MOVE_ENTERDODGERIGHT:
			{
				Difficulty = 1.25f;
				GoodOffensive = 0;
				GoodDefensive = FIST_ACTION_PUNCHRIGHT | FIST_ACTION_RIGHTDODGEPUNCH;
				OffensiveScore = 0; 
				DefensiveScore = 75;
			};
			break;
		case FIST_MOVE_ENTERBLOCK:
			{
				Difficulty = 1.0f;
				GoodOffensive = 0;
				GoodDefensive = FIST_ACTION_PUNCHRIGHT | FIST_ACTION_RIGHTDODGEPUNCH | 
								FIST_ACTION_PUNCHLEFT | FIST_ACTION_LEFTDODGEPUNCH;
				OffensiveScore = 0;
				DefensiveScore = 40;
			};
			break;
		case FIST_MOVE_LEAVEBLOCK:
			{
				//This is always useful unless opponent is pummelling us with right blows
				Difficulty = 0.5f;
				GoodOffensive = ~(FIST_ACTION_PUNCHRIGHT | FIST_ACTION_RIGHTDODGEPUNCH);
				GoodDefensive = 0;
				OffensiveScore = 50;
			}
			break;
		case FIST_ACTION_GRAB:
			{
				Difficulty = 3.0f;
				GoodOffensive = FIST_MOVE_ENTERDODGELEFT | FIST_MOVE_ENTERDODGERIGHT |
								FIST_MOVE_ENTERBLOCK | FIST_MOVE_LEAVEDODGELEFT | 
								FIST_MOVE_LEAVEDODGERIGHT | FIST_MOVE_LEAVEBLOCK |
								FIST_STANCE_MIDDLE | FIST_STANCE_LEFT |	FIST_STANCE_RIGHT |
								FIST_STANCE_BLOCK | FIST_HURT_MIDDLE | FIST_HURT_LEFT | 
								FIST_HURT_RIGHT | FIST_ACTION_BREAKGRAB | NoAction;
				GoodDefensive = 0;
				OffensiveScore = 100;
				DefensiveScore = 0;
			};
			break;
		case FIST_ACTION_BREAKGRAB:
			{
				Difficulty = 15.0f;
				GoodOffensive = 0;
				GoodDefensive = FIST_ACTION_GRAB;
				OffensiveScore = 0;
				DefensiveScore = 200;
			};
			break;
		case 0:
			{
				//No action is useful for waiting out opponent sometimes, both from a defensive 
				//and offensive standpoint. This is of course better for skilled opponents.
				//It's no use to wait if the opponent is attacking, though
				Difficulty = 0;
				GoodOffensive = FIST_MOVE_ENTERDODGELEFT | FIST_MOVE_ENTERDODGERIGHT |
								FIST_MOVE_ENTERBLOCK | FIST_MOVE_LEAVEDODGELEFT | 
								FIST_MOVE_LEAVEDODGERIGHT | FIST_MOVE_LEAVEBLOCK |
								FIST_STANCE_MIDDLE | FIST_STANCE_LEFT |	FIST_STANCE_RIGHT |
								FIST_STANCE_BLOCK | NoAction;
				GoodDefensive = GoodOffensive;
				OffensiveScore = m_pAI->m_Skill / 4;
				DefensiveScore = OffensiveScore;
			}
			break;
		default:
			{
				//The rest are involuntary actions and are by default not useful.
				Difficulty = 0;
				GoodOffensive = 0;
				GoodDefensive = 0;
				OffensiveScore = 0;
				DefensiveScore = 0;
			}
		}

		//Make skill test
		if ((Difficulty > 0) && (100 * Random * Difficulty > Sqr(m_pAI->m_Skill) * 0.01f))
		{
			//Skill test failed
			return CAI_ActionEstimate();
		}
		else
		{
			//Set offensive and defensive score if opponents action is good
			CAI_ActionEstimate Est = CAI_ActionEstimate();
			if (GoodOffensive & _OpponentInfo)
			{
				Est.Set(CAI_ActionEstimate::OFFENSIVE, OffensiveScore);
			}
			if (GoodDefensive & _OpponentInfo)
			{
				Est.Set(CAI_ActionEstimate::DEFENSIVE, DefensiveScore);
			}

			return Est;
		}
	}
	else
		return CAI_ActionEstimate();
};


//A good target is one which we have LOS to and within the weapons min and max ranges
bool CAI_Weapon_Melee_Fist::IsGoodTarget(CAI_AgentInfo * _pTarget, int _Mode)
{
	MAUTOSTRIP(CAI_Weapon_Melee_Fist_IsGoodTarget, false);

	if (!_pTarget || !IsValid())
		return false;

	// *** Hack from hell
	return true;
	// ***

	CAI_AgentCriteria GoodTarget;
	GoodTarget.InfoFlags(CAI_AgentInfo::IN_LOS);
	GoodTarget.Distance(m_pAI->m_pGameObject->GetPosition(), MaxRange(_pTarget), MinRange(_pTarget));

	return _pTarget->Check(GoodTarget);
};


//Returns maximum effective range agaist the specified target for estimation purposes. 
//If no target is given this simply returns the effective range against a singularity.
fp32 CAI_Weapon_Melee_Fist::MaxRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Melee_Fist_MaxRange, 0.0f);

	if (!IsValid())
		return 0;

	int Res = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETFIGHTMODERADIUS), m_pAI->m_pGameObject->m_iObject);
	fp32	Radius = *(fp32*)&Res;
	return Radius;
};


//Returns minimum effective range, agaist the specified target. If no target is given 
//this simply returns the minimum range against a singularity.
fp32 CAI_Weapon_Melee_Fist::MinRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Melee_Fist_MinRange, 0.0f);

	if (!IsValid())
		return 0;

	return 0;
};



//CAI_Weapon_Ranged_AreaEffect/////////////////////////////////////////

//Constructor
CAI_Weapon_Ranged_AreaEffect::CAI_Weapon_Ranged_AreaEffect(CRPG_Object_Item * _pItem, CAI_Core * _pAI, int _iType)
	: CAI_Weapon_Ranged(_pItem, _pAI, _iType)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_AreaEffect_ctor, MAUTOSTRIP_VOID);

	if (m_pItem)
	{
		//This should calculate blast radius in some way...fix
		m_BlastRadius = 100;
	}
	else
	{
		m_BlastRadius = 0;
	}
};

//Checks if there is a line of sight to the target
bool CAI_Weapon_Ranged_AreaEffect::IsLineOfSight(CAI_AgentInfo* _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_AreaEffect_IsLineOfSight, false);

	if (!_pTarget || !m_pAI)
		return false;
	
	//Since area effect weapon use another targeting area priority order (feet, center and aim position)
	//than normal ranged weapons, targeting position is never set in this method, but always in GetActivatePosition
	m_TargetPos = CVec3Dfp32(_FP32_MAX);

	if (_pTarget->InLOS())
	{
		return true;
	}
	else
	{
		return false;
	};
}; 

//Returns a value that estimates how suitable the weapon is to use against the specified 
//target right now, taking such factors as damage per attack, rate of attacks
//ammo, skill, etc into consideration. A value of 0 means that the weapon is useless 
//against the target.
CAI_ActionEstimate CAI_Weapon_Ranged_AreaEffect::GetEstimate(CAI_AgentInfo* _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_AreaEffect_GetEstimate, 0);

	//Just use ranged weapon estimate for now... should take into account potential damage
	//to friends and othere enemies as well... FIX
	return CAI_Weapon_Ranged::GetEstimate(_pTarget);
};

//Returns minimum effective range, agaist the specified target. If no target is given 
//this simply returns the minimum range against a singularity.
fp32 CAI_Weapon_Ranged_AreaEffect::MinRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_AreaEffect_MinRange, 0.0f);

	return m_BlastRadius;
};


//Aim at feet as default, not focus position
void CAI_Weapon_Ranged_AreaEffect::GetActivatePosition(CMat4Dfp32* _pMat)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_AreaEffect_GetActivatePosition, MAUTOSTRIP_VOID);

	if (m_iTarget)
	{
		//Set default target position if this is lacking
		if (m_TargetPos == CVec3Dfp32(_FP32_MAX))
		{
			CAI_AgentInfo * pInfo = m_pAI->m_KB.GetAgentInfo(m_iTarget);
			CWObject * pTarget;
			if (pInfo && (pTarget = pInfo->GetObject()))
			{
				CVec3Dfp32 Offsets[3];
				Offsets[0] = CVec3Dfp32(0);							
				Offsets[1] = m_pAI->GetTorsoPosition(pTarget)	- pTarget->GetPosition();
				Offsets[2] = m_pAI->GetVulnerablePosition(pTarget) - pTarget->GetPosition();
			
				m_TargetPos = m_pAI->CheckLOS(m_pAI->GetAimPosition(), pTarget, Offsets, 3, m_pAI->m_pGameObject->m_iObject);

				//If we can't get valid targetposition, use base position
				if (m_TargetPos == CVec3Dfp32(_FP32_MAX))
					 m_TargetPos = pTarget->GetPosition();
			}
		}
	}
	CAI_Weapon_Ranged::GetActivatePosition(_pMat);
};



//CAI_Weapon_Grenade/////////////////////////////////////////////////

//Constructor
CAI_Weapon_Grenade::CAI_Weapon_Grenade(CRPG_Object_Item * _pItem, CAI_Core * _pAI, int _iType)
	: CAI_Weapon_Ranged_AreaEffect(_pItem, _pAI, _iType)
{
	MAUTOSTRIP(CAI_Weapon_Grenade_ctor, MAUTOSTRIP_VOID);

	//These should really be gotten from the bomb object in question, but this is 
	//currently not supported
	m_ThrowSpeed = 40.0f;
	m_Gravity = 4.0f;
	m_UseCount = -1;
}

//Refreshes the weapon one frame. Continually uses weapon until it's time to release, 
//while aiming towards target
void CAI_Weapon_Grenade::OnRefresh()
{
	MAUTOSTRIP(CAI_Weapon_Grenade_OnRefresh, MAUTOSTRIP_VOID);

	//Continue using bomb until we've gained enough force (very approximated)
	if (m_UseCount >= 0)
	{
		CWObject * pTarget = m_pAI->m_pServer->Object_Get(m_iTarget);
		if (pTarget)
		{
			CVec3Dfp32 PosDiff = pTarget->GetPosition() - m_pAI->m_pGameObject->GetPosition();
			fp32 DistFactor = PosDiff.LengthSqr() + PosDiff[2] * Sqr(PosDiff[2]);
			if ((DistFactor < 250*250) ||
				((DistFactor < 400*400) && (m_UseCount > 10)) ||
				((DistFactor < 600*600) && (m_UseCount > 20)) ||
				(m_UseCount > 30))
			{
				//Stop using
				m_UseCount = -1;
				m_pAI->m_DeviceWeapon.Lock();
				m_iUseTimer = 0;
			}
			else
			{
				//Continue using
				m_pAI->m_DeviceWeapon.Use();
				m_UseCount++;
			}
		}
		else
		{
			m_UseCount = -1;
		}
	}

	CAI_Weapon_Ranged_AreaEffect::OnRefresh();
};



//Returns a value that estimates how suitable the weapon is to use against the specified 
//target right now, taking such factors as damage per attack, rate of attacks
//ammo, skill, etc into consideration. A value of 0 means that the weapon is useless 
//against the target.
CAI_ActionEstimate CAI_Weapon_Grenade::GetEstimate(CAI_AgentInfo* _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Weapon_Grenade_GetEstimate, 0);

	CAI_ActionEstimate Est = CAI_ActionEstimate();

	//Just use ranged weapon estimate for now... should take into account potential damage
	//to friends and other enemies as well... FIX
	if (!IsValid() || !_pTarget || !_pTarget->GetObject())
		return Est;

	//Estimate is zero when using
	if (m_UseCount >= 0)
		return Est;

	//Halve estimate for ranges over 600 units
	fp32 DistSqr = m_pAI->m_pGameObject->GetPosition().DistanceSqr(_pTarget->GetObject()->GetPosition());

	Est = CAI_Weapon_Ranged::GetEstimate(_pTarget, _Action);
	if (DistSqr > 600*600)
	{
		Est.Mult(CAI_ActionEstimate::OFFENSIVE, 0.5);
	}
		
	return Est;
}


//Start using bomb
void CAI_Weapon_Grenade::OnUse(CAI_AgentInfo* _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Weapon_Grenade_OnUse, MAUTOSTRIP_VOID);

	m_UseCount = 0;
	CAI_Weapon_Ranged_AreaEffect::OnUse(_pTarget);
};


//Calculate the activateposition matrix for this weapons wielder. This adds some scatter due to lack of skill etc to the given matrix
void CAI_Weapon_Grenade::GetActivatePosition(CMat4Dfp32* _pMat)
{
	MAUTOSTRIP(CAI_Weapon_Grenade_GetActivatePosition, MAUTOSTRIP_VOID);

	if (!m_pAI || !m_pAI->IsValid() || !_pMat)
		return;

	//Aim at character in normal fashion (with scatter etc)...
	CAI_Weapon_Ranged::GetActivatePosition(_pMat);

	//...then change pitch of this matrix according to distance to target (if we have one)
	CWObject * pTarget = m_pAI->m_pServer->Object_Get(m_iTarget);
	CAI_AgentInfo * pInfo = m_pAI->m_KB.GetAgentInfo(m_iTarget);		
	if (pTarget && pInfo)
	{
		//Get aim vector
		CVec3Dfp32 Aim = CVec3Dfp32::GetRow(*_pMat, 0);

		//Just shift aim upwards a bit for long ranges and downwards for short ones
		fp32 DistSqr = m_pAI->m_pGameObject->GetPosition().DistanceSqr(pTarget->GetPosition());
		fp32 Offset;
		if (DistSqr > 600*600)
			Offset = 0.8f;
		else
			Offset = DistSqr/(600*600) - 0.2f;

		/* Deprecated
		//We might bungle the height offset some due to lack of skill
		if (m_pAI->m_Skill + GetAimingScore(pInfo) < 100)
			Offset += (4 * Random - 2) * (fp32)(100 - m_pAI->m_Skill) / 100.0f;;
		*/

		Aim += CVec3Dfp32(0,0,1) * Offset;
		Aim.Normalize();
		
		//Align matrix
		Aim.SetMatrixRow(*_pMat, 0);
		if(M_Fabs(Aim * CVec3Dfp32(0, 0, 1)) < 0.99f)
			CVec3Dfp32(0, 0, 1).SetMatrixRow(*_pMat, 1);
		else
			CVec3Dfp32(0, 1, 0).SetMatrixRow(*_pMat, 1);
		(*_pMat).RecreateMatrix(0, 1);

		//DEBUG
		//_pMat->Unit();
		//CVec3Dfp32(0,0,1).SetRow(*_pMat,0);
		//CVec3Dfp32(0, 1, 0).SetRow(*_pMat, 1);
		//(*_pMat).RecreateMatrix(0, 1);
		//DEBUG

		//Set matrix position to aim position
		m_pAI->GetAimPosition(m_pAI->m_pGameObject).SetMatrixRow(*_pMat, 3);

		//m_pAI->Debug_RenderWire(CVec3Dfp32::GetRow(*_pMat, 3), CVec3Dfp32::GetRow(*_pMat, 3) + CVec3Dfp32::GetRow(*_pMat, 0) * 400, 0xffff0000);		
	};	
}


//Returns maximum effective range agaist the specified target for estimation purposes. 
//If no target is given this simply returns the effective range against a singularity.
fp32 CAI_Weapon_Grenade::MaxRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Grenade_MaxRange, 0.0f);

	if (!IsValid())
		return 0;
	
	//Approximate maximum throwing range for now
	if (_pTarget && _pTarget->GetObject())
		return 600 + m_pAI->m_pGameObject->GetPosition()[2] - _pTarget->GetObject()->GetPosition()[2];
	else
		return 600;
}


//Returns minimum effective range, agaist the specified target. If no target is given 
//this simply returns the minimum range against a singularity.
fp32 CAI_Weapon_Grenade::MinRange(CAI_AgentInfo * _pTarget)
{
	MAUTOSTRIP(CAI_Weapon_Grenade_MinRange, 0.0f);

	if (!IsValid())
		return 0;

	//Approximate minimum throwing range for now
	return 0;
}



//CAI_Weapon_Ranged_RapidFire/////////////////////////////////////////////////////////

//Constructor
CAI_Weapon_Ranged_RapidFire::CAI_Weapon_Ranged_RapidFire(CRPG_Object_Item * _pItem, CAI_Core * _pAI, int _iType)
	: CAI_Weapon_Ranged(_pItem, _pAI, _iType)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_RapidFire_ctor, MAUTOSTRIP_VOID);

	m_nBurstLeft = 0;
	m_PrevTargetPos = CVec3Dfp32(_FP32_MAX);
};


//Calculate rate of fire per second
void CAI_Weapon_Ranged_RapidFire::Reset()
{
	MAUTOSTRIP(CAI_Weapon_Ranged_RapidFire_Reset, MAUTOSTRIP_VOID);

	CAI_Weapon_Ranged::Reset();
	m_nBurstLeft = 0;
	m_PrevTargetPos = CVec3Dfp32(_FP32_MAX);
};


//Calculate rate of fire per second
fp32 CAI_Weapon_Ranged_RapidFire::ROF()
{
	MAUTOSTRIP(CAI_Weapon_Ranged_RapidFire_ROF, 0.0f);

	if (!m_pItem)
		return 0;

	return (m_pItem->m_FireTimeout) ? (m_pAI->GetAITicksPerSecond() / m_pItem->m_FireTimeout) : m_pAI->GetAITicksPerSecond();
};


//Refreshes the weapon one frame. During a burst, the weapon will be fired repeatedly, 
//while strafing the target position across the target.
void CAI_Weapon_Ranged_RapidFire::OnRefresh()
{
	MAUTOSTRIP(CAI_Weapon_Ranged_RapidFire_OnRefresh, MAUTOSTRIP_VOID);

	if (!IsValid())
		return;

	CVec3Dfp32 TargetPos = CVec3Dfp32(_FP32_MAX);
	if (IsWielded())
	{
		//Should we continue to fire a burst?
		CWObject * pTarget = NULL;
		if ((m_nBurstLeft > 0) &&
			m_pAI->m_DeviceWeapon.IsAvailable() &&
			(pTarget = m_pAI->IsValidAgent(m_iTarget)))
		{
			//Hold down trigger!
			m_pAI->m_DeviceWeapon.Use();
			m_nBurstLeft--;

			if (!m_pAI->Wait())
			{
				//Strafing doesn't work, since wait is never 0... fix!
				//Modify target position appropriately
				/*
				if ((m_PrevTargetPos != CVec3Dfp32(_FP32_MAX)) &&
					(m_TargetPos != CVec3Dfp32(_FP32_MAX)))
				{
					// No strafing
					//Check target movement since last shot
					CVec3Dfp32 TargetMove = pTarget->GetPosition() - m_PrevTargetPos;
					//If target moves fast, we might be unable to keep up
					fp32 SpeedSqr = TargetMove.LengthSqr();
					if (SpeedSqr > Sqr(m_pAI->m_Skill * 0.1f * (m_pItem->m_FireTimeout + 1)))
					{
						//Too fast, clamp target move length
						TargetMove.Normalize();
						TargetMove *= m_pAI->m_Skill * 0.1f * (m_pItem->m_FireTimeout + 1);
					}
					
					//If we've got a valid burst direction, then add strafe movement
					if (m_BurstDir != CVec3Dfp32(_FP32_MAX))
						TargetMove += m_BurstDir;

					m_TargetPos += TargetMove;
				}
				else
				*/
					//Previous target position invalid, just aim at belly
				m_PrevTargetPos = m_TargetPos;
				m_TargetPos	= m_pAI->GetCenterPosition(pTarget);

				//m_pAI->Debug_RenderWire(m_TargetPos, m_TargetPos + CVec3Dfp32(0,0,20), 0xffffffff, 1);//DEBUG

				// m_PrevTargetPos = pTarget->GetPosition();
				
				TargetPos = m_TargetPos;
			}
		}
		else if ((m_nBurstLeft > 0) && !pTarget)
		{
			//Abort burst
			m_nBurstLeft = 0;
		}
	}

	CAI_Weapon_Ranged::OnRefresh();
	m_TargetPos = TargetPos;
};


//Same as for ordinary ranged weapons, but increased a bit. Wielders of rapid fire weapons should always be more trigger happy :)
CAI_ActionEstimate CAI_Weapon_Ranged_RapidFire::GetEstimate(CAI_AgentInfo* _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_RapidFire_GetEstimate, 0);

	//Estimate is 0 if burst already is in progress
	if (m_nBurstLeft)
		return CAI_ActionEstimate();
	else
	{
		CAI_ActionEstimate Est = CAI_Weapon_Ranged::GetEstimate(_pTarget, _Action);
		Est.Mult(CAI_ActionEstimate::OFFENSIVE, 1.2f);
		return Est;
	}
};


//Always fire a burst unless there's very little ammo left
void CAI_Weapon_Ranged_RapidFire::OnUse(CAI_AgentInfo* _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Weapon_Ranged_RapidFire_OnUse, MAUTOSTRIP_VOID);

	if (!_pTarget || !_pTarget->GetObject() || !m_pAI || !m_pAI->m_pGameObject)
		return;

	//Check ammo in magazine
	int iAmmo = CheckAmmo(true);
	if ((iAmmo > 5) &&
		(ROF() > 3))
	{
		//How many shots should we fire?
		m_nBurstLeft = 0;//Random * Min(CheckAmmo(true), (int)ROF()) + 1;

		if (m_nBurstLeft > 0)
		{
			//Fire a burst!
			//Remember targets position
			m_PrevTargetPos = m_TargetPos;
			
	
			//Convert burst from "number of shots" to number of frames to hold down trigger
			m_nBurstLeft = (int32)(((m_nBurstLeft- 1) / ROF()) * m_pAI->GetAITicksPerSecond());
		}
	}
	
	//Use weapon
	CAI_Weapon_Ranged::OnUse(_pTarget);
};



//CAI_WeaponHandler : Handles all weapons of a bot///////////////////////////////////

//Dummy weapon handler
CAI_Weapon CAI_WeaponHandler::ms_DummyWeapon(NULL, NULL, CAI_Weapon::UNKNOWN);

//Constructors
CAI_WeaponHandler::CAI_WeaponHandler(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_WeaponHandler_ctor, MAUTOSTRIP_VOID);

	m_pAI = _pAI;
	m_iWielded = 0;
	m_ArmsClass = AI_ARMSCLASS_INVALID;
	m_bUpdateWeapons = true;
};


//Defines what weapon type a given string corresponds to
int CAI_WeaponHandler::GetWeaponType(CStr _TypeStr)
{
	MAUTOSTRIP(CAI_WeaponHandler_GetWeaponType, 0);

	CStr Type = _TypeStr.UpperCase();
	CStr Prefix = Type.GetStrSep("_");
	if (Prefix == "MELEE")
	{
		Prefix = Type.GetStrSep("_");
		if (Prefix == "FIST")
		{
			return CAI_Weapon::FIST;
		}
		else if (Prefix == "DRILL")
		{
			Prefix = Type.GetStrSep("_");
			return CAI_Weapon::DRILL;
		}
		else if (Prefix == "BIGALIEN")
		{
			Prefix = Type.GetStrSep("_");
			if (Prefix == "HEAD")
			{
				return CAI_Weapon::BIGALIEN_HEAD;
			}
			else 
				return CAI_Weapon::BIGALIEN_TAIL;
		}
		else if (Prefix == "SMALLALIEN")
		{
			Prefix = Type.GetStrSep("_");
			return CAI_Weapon::SMALLALIEN_SPURS;
		}
		else
			return CAI_Weapon::MELEE;
	}
	else if (Prefix == "RANGED")
	{
		Prefix = Type.GetStrSep("_");
		if (Prefix == "RAPIDFIRE")
		{
			return CAI_Weapon::RANGED_RAPIDFIRE;
		}
		else if (Prefix == "AREA")
		{
			return CAI_Weapon::RANGED_AREAEFFECT;
		}
		else if (Prefix == "ROCKET")
		{
			return CAI_Weapon::ROCKET;
		}
		else if (Prefix == "PERSISTENT")
		{
			return CAI_Weapon::PERSISTENT;
		}
		else if (Prefix == "GRENADE")
		{
			return CAI_Weapon::GRENADE;
		}
		else
			return CAI_Weapon::RANGED;
	}
	else if (Prefix == "NONWEAPON")
	{
		return CAI_Weapon::NONWEAPON;
	}
	else
		return CAI_Weapon::UNKNOWN;
};


//Constructs a new weapon object from the corresponding world object and returns smart pointer to it
spCAI_Weapon CAI_WeaponHandler::CreateWeapon(CRPG_Object_Item * _pWeapon)
{
	MAUTOSTRIP(CAI_WeaponHandler_CreateWeapon, NULL);
	MSCOPE(CAI_WeaponHandler::CreateWeapon, CHARACTER);

	if (!m_pAI || !_pWeapon)
		return NULL;
	
	//Get weapon type
	int iType = GetWeaponType(_pWeapon->m_AIType);

	//Create weapon handler
	switch (iType)
	{
		case CAI_Weapon::MELEE:
		case CAI_Weapon::DRILL:
		case CAI_Weapon::BIGALIEN_HEAD:
		case CAI_Weapon::BIGALIEN_TAIL:
		case CAI_Weapon::SMALLALIEN_SPURS:
			return MNew3(CAI_Weapon_Melee, _pWeapon, m_pAI, iType);

		case CAI_Weapon::FIST:
			return MNew3(CAI_Weapon_Melee_Fist, _pWeapon, m_pAI, iType);

		case CAI_Weapon::RANGED:
			return MNew3(CAI_Weapon_Ranged, _pWeapon, m_pAI, iType);

		case CAI_Weapon::RANGED_RAPIDFIRE:
			return MNew3(CAI_Weapon_Ranged_RapidFire, _pWeapon, m_pAI, iType);

		case CAI_Weapon::RANGED_AREAEFFECT:
		case CAI_Weapon::ROCKET:
		case CAI_Weapon::PERSISTENT:
			return MNew3(CAI_Weapon_Ranged_AreaEffect, _pWeapon, m_pAI, iType);

		case CAI_Weapon::GRENADE:
			return MNew3(CAI_Weapon_Grenade, _pWeapon, m_pAI, iType);

		default:
			return MNew3(CAI_Weapon, _pWeapon, m_pAI, iType);
	};
}; 
	

//Synch weapon handlers with character weapons
void CAI_WeaponHandler::OnUpdateWeapons()
{
	MAUTOSTRIP(CAI_WeaponHandler_OnUpdateWeapons, MAUTOSTRIP_VOID);

	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Iterate through weapon object and make sure all weapons have corresponding weapon handlers
	int Start = 0;
	bool bFound;
	CRPG_Object_Inventory * pInventory = m_pAI->GetRPGInventory(m_pAI->m_pGameObject->m_iObject, RPG_CHAR_INVENTORY_WEAPONS);
	if (pInventory)
	{
		CRPG_Object_Item * pItem;
		for (int i = 0; i < pInventory->GetNumItems(); i++)
		{
			pItem = pInventory->GetItemByIndex(i);
			bFound = false;
			for (int j = 0; j < m_lspWeapons.Len(); j++)
			{
				if (m_lspWeapons[(Start + j) % m_lspWeapons.Len()] &&
					(pItem == m_lspWeapons[(Start + j) % m_lspWeapons.Len()]->GetItem()))
				{
					bFound = true;
					Start = (Start + j + 1) % m_lspWeapons.Len();
					break;
				}
			}

			//Did we find a corresponding handler?
			if (!bFound && pItem)
			{
				//No! Create new handler
				spCAI_Weapon Wpn = CreateWeapon(pItem);
				m_lspWeapons.Add(Wpn);
				m_pAI->m_AH.ReportNewWeapon(Wpn->GetType());
			}
		}

		//Are there any weapon handlers for non-existant weapons? 
		//(This should currently never happen, so I've made a terribly brute force solution)
		//Doesn't work well...
		if (pInventory->GetNumItems() != m_lspWeapons.Len())
		{
			m_lspWeapons.Clear();
			for (int i = 0; i < pInventory->GetNumItems(); i++)
			{
				spCAI_Weapon Wpn = CreateWeapon(pInventory->GetItemByIndex(i));
				m_lspWeapons.Add(Wpn);
				m_pAI->m_AH.ReportNewWeapon(Wpn->GetType());
			};
		};

		//Weapons have been successfully updated
		m_bUpdateWeapons = false;
	}
};


//Initialize weapon handler
void CAI_WeaponHandler::Init()
{
	MAUTOSTRIP(CAI_WeaponHandler_Init, MAUTOSTRIP_VOID);

	//Update weapons...
	OnUpdateWeapons();

	//...and find wielded weapon handler
	for (int i = 0; i < m_lspWeapons.Len(); i++)
	{
		if (m_lspWeapons[i] && 
			m_lspWeapons[i]->IsWielded())
		{
			m_iWielded = i;
			break;
		}
	}

	UpdateWieldedArmsClass();
};


//Refreshes the item handler one frame
void CAI_WeaponHandler::OnRefresh()
{
	MAUTOSTRIP(CAI_WeaponHandler_OnRefresh, MAUTOSTRIP_VOID);

	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Update weapons when prompted explicitly or if the number of weapons are chagned
	CRPG_Object_Inventory * pInventory = m_pAI->GetRPGInventory(m_pAI->m_pGameObject->m_iObject, RPG_CHAR_INVENTORY_WEAPONS);
	if ( pInventory &&
		 (m_bUpdateWeapons ||
		  (m_lspWeapons.Len() != pInventory->GetNumItems())) )
	{
		OnUpdateWeapons();
	};

	//Stop here if there are no weapons
	if (m_lspWeapons.Len() == 0)
		return;

	//Update wielded weapon if necessary
	if (!m_lspWeapons.ValidPos(m_iWielded) ||
		!m_lspWeapons[m_iWielded] ||
		!m_lspWeapons[m_iWielded]->IsWielded())
	{
		//Reset previously wielded weapon 
		if (m_lspWeapons.ValidPos(m_iWielded) &&
			m_lspWeapons[m_iWielded])
			m_lspWeapons[m_iWielded]->Reset();

		//Find wielded weapon handler
		for (int i = 0; i < m_lspWeapons.Len(); i++)
		{
			if (m_lspWeapons[i] && 
				m_lspWeapons[i]->IsWielded())
			{
				m_iWielded = i;
				break;
			}
		}
		UpdateWieldedArmsClass();
	}

	//Refresh wielded weapon handler
	if (m_lspWeapons.ValidPos(m_iWielded) &&
		m_lspWeapons[m_iWielded])
		m_lspWeapons[m_iWielded]->OnRefresh();
};


//Update weapons next refresh
void CAI_WeaponHandler::UpdateWeapons()
{
	MAUTOSTRIP(CAI_WeaponHandler_UpdateWeapons, MAUTOSTRIP_VOID);

	m_bUpdateWeapons = true;
};


//Currently wielded weapon
CAI_Weapon* CAI_WeaponHandler::GetWielded()
{
	MAUTOSTRIP(CAI_WeaponHandler_GetWielded, NULL);

	if (!m_lspWeapons.ValidPos(m_iWielded))
		return &ms_DummyWeapon;

	return m_lspWeapons[m_iWielded];
};

void CAI_WeaponHandler::UpdateWieldedArmsClass()
{
	CRPG_Object_Item* pItem = NULL;
	CAI_Weapon* pWeapon = NULL;
	if (!m_pAI->IsPlayer())
	{
		pWeapon = GetWielded();
		if (!pWeapon)
		{
			m_ArmsClass = AI_ARMSCLASS_INVALID;
			return;
		}

		pItem = pWeapon->GetItem();
	}
	else
	{	// How do we know what item the player is currently wielding?
		pItem = m_pAI->m_pGameObject->AI_GetRPGItem(RPG_CHAR_INVENTORY_WEAPONS);
	}

	if (!pItem)
	{
		m_ArmsClass = AI_ARMSCLASS_INVALID;
		return;
	}

	/*
	AI_WEAPONTYPE_UNDEFINED		= 0,
	AI_WEAPONTYPE_BAREHANDS		= 1,
	AI_WEAPONTYPE_GUN			= 2,
	AI_WEAPONTYPE_SHOTGUN		= 4,
	AI_WEAPONTYPE_ASSAULT		= 8,
	AI_WEAPONTYPE_MINGUN		= 16,
	AI_WEAPONTYPE_KNUCKLEDUSTER	= 32,
	AI_WEAPONTYPE_SHANK			= 64,
	AI_WEAPONTYPE_CLUB			= 128,
	AI_WEAPONTYPE_GRENADE		= 256,
	*/
	int16 orgWeaponType = m_WeaponType;
	m_WeaponType = pItem->m_AnimProperty;
	if (m_WeaponType & (AI_WEAPONTYPE_MINGUN | AI_WEAPONTYPE_GRENADE))
	{
		m_ArmsClass = AI_ARMSCLASS_HEAVY;
	}
	else if (m_WeaponType & (AI_WEAPONTYPE_GUN | AI_WEAPONTYPE_SHOTGUN | AI_WEAPONTYPE_ASSAULT))
	{
		m_ArmsClass = AI_ARMSCLASS_GUN;
	}
	else if (m_WeaponType & (AI_WEAPONTYPE_KNUCKLEDUSTER | AI_WEAPONTYPE_SHANK | AI_WEAPONTYPE_CLUB))
	{
		m_ArmsClass = AI_ARMSCLASS_MELEE;
	}
	else if (m_WeaponType & AI_WEAPONTYPE_BAREHANDS)
	{
		m_ArmsClass = AI_ARMSCLASS_UNARMED;
	}
	else
	{
		m_ArmsClass = AI_ARMSCLASS_INVALID;
	}

	if (orgWeaponType != m_WeaponType)
	{
		if ((!m_pAI->IsPlayer())&&(!(m_pAI->m_UseFlags & CAI_Core::USE_DRAWAMMO)&&(pWeapon)))
		{
			CRPG_Object_Item* pItem = pWeapon->GetItem();
			if (pItem)
			{
				pItem->m_Flags2 |= RPG_ITEM_FLAGS2_NOAMMODRAW;
			}
		}
	}
};

int CAI_WeaponHandler::GetWieldedWeaponType()
{
	return(m_WeaponType);
};

int CAI_WeaponHandler::GetWieldedArmsClass()
{
	return(m_ArmsClass);
};

//Switch to best weapon against the given target under the given constraints 
//Succeeds if there's a weapon to switch to
bool CAI_WeaponHandler::SwitchToBestWeapon(CAI_AgentInfo * _pTarget, int _iConstraint)
{
	MAUTOSTRIP(CAI_WeaponHandler_SwitchToBestWeapon_CWObject_int, false);

	int iBestEstimate  = 0;
	int iBestWeapon = -1;
	int iEstimate;
	for (int i = 0; i < m_lspWeapons.Len(); i++)
	{
		if (m_lspWeapons[i] &&
			((_iConstraint == ALL_WEAPONS) ||
			 ((_iConstraint == RANGED_ONLY) && m_lspWeapons[i]->IsRangedWeapon()) || 
			 ((_iConstraint == MELEE_ONLY) && m_lspWeapons[i]->IsMeleeWeapon()) ||
			 ((_iConstraint == NONWEAPON_ONLY) && (m_lspWeapons[i]->GetType() == CAI_Weapon::NONWEAPON))))
		{
			iEstimate = m_lspWeapons[i]->GetEstimate(_pTarget);
			if (iEstimate > iBestEstimate)
			{
				iBestEstimate = iEstimate;
				iBestWeapon = i;
			};
		}
	}

	if (iBestWeapon != -1)
	{
		m_lspWeapons[iBestWeapon]->OnWield();
		return true;
	}
	else
	{
		return false;
	}
};


//Switch to gererally "best" weapon. Succeeds if there's a weapon to switch to
bool CAI_WeaponHandler::SwitchToBestWeapon(int _iConstraint)
{
	MAUTOSTRIP(CAI_WeaponHandler_SwitchToBestWeapon_int, false);

	//Switch to non-harmless weapon of lowest item type
	int iBestWeapon = -1;
	int iLowestItemType = 1000;
	for (int i = 0; i < m_lspWeapons.Len(); i++)
	{
		if (m_lspWeapons[i] &&
			m_lspWeapons[i]->GetItem() &&
			((((_iConstraint == ALL_WEAPONS) ||
			   ((_iConstraint == RANGED_ONLY) && m_lspWeapons[i]->IsRangedWeapon()) || 
			   ((_iConstraint == MELEE_ONLY) && m_lspWeapons[i]->IsMeleeWeapon())) &&
			  !m_lspWeapons[i]->IsHarmless()) ||
			 ((_iConstraint == NONWEAPON_ONLY) && (m_lspWeapons[i]->GetType() == CAI_Weapon::NONWEAPON))))
		{
			if (m_lspWeapons[i]->GetItem()->m_iItemType	< iLowestItemType)
			{
				iLowestItemType = m_lspWeapons[i]->GetItem()->m_iItemType;
				iBestWeapon = i;
			};
		}
	}

	if (iBestWeapon != -1)
	{
		m_lspWeapons[iBestWeapon]->OnWield();
		return true;
	}
	else
	{
		return false;
	}
};



//Change AI user, and renew weapon handlers
void CAI_WeaponHandler::ReInit(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_WeaponHandler_ReInit, MAUTOSTRIP_VOID);

	m_pAI = _pAI;
	m_lspWeapons.Clear();
	m_bUpdateWeapons = true;
};


//Return recommended priority for given weapon type
int CAI_WeaponHandler::Recommendation(CAI_Weapon * _pWeapon)
{
	MAUTOSTRIP(CAI_WeaponHandler_Recommendation, 0);

	switch (_pWeapon->GetType())
	{
	case CAI_Weapon::GRENADE:
		return 0;
	case CAI_Weapon::NONWEAPON:
		//Non-weapons are preferred
		return 20;
	default:
		{
			if (_pWeapon->IsRangedWeapon())
				return 10;
			else if (_pWeapon->GetType() == CAI_Weapon::DRILL)
				return 6;
			else if (_pWeapon->IsMeleeWeapon())
				return 5;
			else
				return 0;
		}
	}
};

//Get itemtype of recommended weapon to start with.
int CAI_WeaponHandler::GetRecommendedWeapon()
{
	MAUTOSTRIP(CAI_WeaponHandler_GetRecommendedWeapon, 0);

	//Update weapons...
	OnUpdateWeapons();

	//...and find itemtype of "best" one
	int BestItemType = -1;
	int BestRecommendation = -1;
	int Rec;
	for (int i = 0; i < m_lspWeapons.Len(); i++)
	{
		CRPG_Object_Item * pWeapon = m_lspWeapons[i]->GetItem();
		if (pWeapon)
		{
			Rec = Recommendation(m_lspWeapons[i]);
			if (Rec > BestRecommendation)
			{
				BestItemType = pWeapon->m_iItemType;
				BestRecommendation = Rec;
			}
		}
	}
	return BestItemType;
};




//ITEMS///////////////////////////////////////////////////////////////////////////////////////////////


//CAI_Item : Base class for handling a specific item///////////////////////

//Constructor
CAI_Item::CAI_Item(CRPG_Object_Item * _pItem, CAI_Core * _pAI, int _iType)
{
	MAUTOSTRIP(CAI_Item_ctor, MAUTOSTRIP_VOID);

	m_pItem = _pItem;
	m_pAI = _pAI;
	m_iType = _iType;
	m_iUseTimer = -1;
};

//Resets the item data
void CAI_Item::Reset()
{
	MAUTOSTRIP(CAI_Item_Reset, MAUTOSTRIP_VOID);

	m_iUseTimer = -1;
};


//Get item type
int CAI_Item::GetType()
{
	MAUTOSTRIP(CAI_Item_GetType, 0);

	return m_iType;
};


//Refreshes the weapon one frame
void CAI_Item::OnRefresh()
{
	MAUTOSTRIP(CAI_Item_OnRefresh, MAUTOSTRIP_VOID);

	if (!IsValid())
		//Aargh! Invalid AI!
		return;

	if (IsWielded())
	{
		//Set use timer
		if (m_iUseTimer != -1)
			if (m_pAI->Wait() == 0)
				m_iUseTimer = -1;
			else
				m_iUseTimer++;
	}
	else
		Reset();
};


//Returns a value that estimates how suitable the item is to use against the specified 
//target right now. Most items cannot be used against targets, but will instead return estimate
//for using item on self.
CAI_ActionEstimate CAI_Item::GetEstimate(CAI_AgentInfo* _pTarget)
{
	MAUTOSTRIP(CAI_Item_GetEstimate, 0);

	return CAI_ActionEstimate();
};


//Use the item if possible
void CAI_Item::OnUse(CAI_AgentInfo * _pTarget, int _Action)
{
	MAUTOSTRIP(CAI_Item_OnUse, MAUTOSTRIP_VOID);

	//We cannot use the item if this itemhandler is invalid
	if (!IsValid())
		return;
	
	//Reset use timer
	m_iUseTimer = 0;

	//..and use the item, hoping that something good happens
	m_pAI->m_DeviceItem.Use();
};

//Adds commands to switch to this weapon if possible
void CAI_Item::OnWield()
{
	MAUTOSTRIP(CAI_Item_OnWield, MAUTOSTRIP_VOID);

	if (!IsValid() || !m_pAI->m_DeviceItem.IsAvailable())
		return;

	//Tell itemhandler to switch forward to this item type
	m_pAI->m_Item.WieldItem(GetType());
};


//Checks if this is a shield
bool CAI_Item::IsShield()
{
	MAUTOSTRIP(CAI_Item_IsShield, false);

	return (m_iType > UNKNOWN) && (m_iType < MAX_SHIELDS);
};


//Checks if this is ammo
bool CAI_Item::IsAmmo()
{
	MAUTOSTRIP(CAI_Item_IsAmmo, false);

	return (m_iType >= AMMO) && (m_iType < MAX_AMMO);
};

//Returns current number of frames since weapon was used
int CAI_Item::GetUseTimer()
{
	MAUTOSTRIP(CAI_Item_GetUseTimer, 0);

	return m_iUseTimer;
};


//Checks if the item is valid (i.e. has it's Item, Wielder and World pointers set)
bool CAI_Item::IsValid()
{
	MAUTOSTRIP(CAI_Item_IsValid, false);

	return (m_pItem && m_pAI && m_pAI->IsValid());
};


//Checks if this item is wielded
bool CAI_Item::IsWielded()
{
	MAUTOSTRIP(CAI_Item_IsWielded, false);

	if (!IsValid())
		return false;

	CRPG_Object_Item * pItem = m_pAI->GetRPGItem(m_pAI->m_pGameObject->m_iObject, RPG_CHAR_INVENTORY_ITEMS);
	if (pItem && (m_pItem == pItem))
		return true;
	else
	{
		CRPG_Object_Inventory * pInv = m_pAI->GetRPGInventory(m_pAI->m_pGameObject->m_iObject, RPG_CHAR_INVENTORY_ITEMS);
		if (pInv && (m_pItem->m_iItemType == pInv->GetSelectedItemType()))
			return true;
		else 
			return false;
	}
};


//Returns the RPG item
CRPG_Object_Item * CAI_Item::GetItem()
{
	MAUTOSTRIP(CAI_Item_GetItem, NULL);

	return m_pItem;
};


//Change AI user
void CAI_Item::ReInit(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_Item_ReInit, MAUTOSTRIP_VOID);

	m_pAI = _pAI;
	Reset();
};

//Calculate the activateposition matrix for this items wielder
void CAI_Item::GetActivatePosition(CMat4Dfp32* _pMat)
{
	MAUTOSTRIP(CAI_Item_GetActivatePosition, MAUTOSTRIP_VOID);

	if (!IsValid() || !_pMat)
		return;

	//Just set matrix to current look matrix but...
	*_pMat = m_pAI->m_pGameObject->GetPositionMatrix();

	//...set matrix position to aim position
	m_pAI->GetAimPosition(m_pAI->m_pGameObject).SetMatrixRow(*_pMat, 3);
};


//Get number of items in this item object
int CAI_Item::GetNumItems()
{
	MAUTOSTRIP(CAI_Item_GetNumItems, 0);

	if (m_pItem)
		return m_pItem->m_NumItems;
	else
		return 0;
};



//CAI_Item_Shield : Base class for shields/////////////////////////////////

//Constructor
CAI_Item_Shield::CAI_Item_Shield(CRPG_Object_Item * _Item, CAI_Core * _pAI, int _iType)
	: CAI_Item(_Item, _pAI, _iType)
{
	MAUTOSTRIP(CAI_Item_Shield_ctor, MAUTOSTRIP_VOID);

	m_iLastBlockedAttacker = 0;
	m_iBlockTimer = -1;
};


//Refreshes the shield one frame
void CAI_Item_Shield::OnRefresh()
{
	MAUTOSTRIP(CAI_Item_Shield_OnRefresh, MAUTOSTRIP_VOID);

	if (IsWielded())
	{
		//Set block timer
		if (m_iBlockTimer != -1)
			if (++m_iBlockTimer > 100)
			{
				m_iBlockTimer = -1;
				m_iLastBlockedAttacker = 0;
			};
	};

	CAI_Item::OnRefresh();
};


//Returns a value that estimates how suitable the shield is to use against the specified 
//target right now, or how suitable it is to use right now in general.
CAI_ActionEstimate CAI_Item_Shield::GetEstimate(CAI_AgentInfo* _pTarget)
{
	MAUTOSTRIP(CAI_Item_Shield_GetEstimate, 0);

	//Grossly incomplete. FIX!
	CAI_ActionEstimate Est = CAI_ActionEstimate();

	if (!IsValid())
		return Est;

	//Just return max if under attack by blockable weapon and able to block
	if (m_pAI->Wait())
		return Est;
	else if (_pTarget)
	{
		CWObject * pChar = m_pAI->IsValidAgent(_pTarget->GetObjectID());
		CRPG_Object_Item* pWeapon;
		if (pChar &&
			(pWeapon = m_pAI->GetRPGItem(m_pAI->m_pGameObject->m_iObject, RPG_CHAR_INVENTORY_WEAPONS)) &&
			pWeapon->GetDamage() &&
			(pWeapon->GetDamage()->m_Damage & DAMAGETYPE_BLOCKABLE))
		{
			Est.Set(CAI_ActionEstimate::DEFENSIVE, 100);
		}

		return Est;
	}
	else 
	{
		//This part needs to be FIXed
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 100);
		return Est;
	}
};


//We just blocked this attackers attack with the item
void CAI_Item_Shield::Block(int _iAttacker)
{
	MAUTOSTRIP(CAI_Item_Shield_Block, MAUTOSTRIP_VOID);

	m_iLastBlockedAttacker = _iAttacker;
	m_iBlockTimer = 0;
};


//Did we block this attacker less than the specified number of frames ago?
bool CAI_Item_Shield::BlockedAttack(int _iAttacker, int _iTime)
{
	MAUTOSTRIP(CAI_Item_Shield_BlockedAttack, false);

	return (_iAttacker = m_iLastBlockedAttacker) &&
		   (m_iBlockTimer != -1) &&
		   (m_iBlockTimer <= _iTime);	
};


//Checks if the given item is a shield and if so returns pointer to shield object
CAI_Item_Shield * CAI_Item_Shield::GetShield(CAI_Item * _pItem)
{
	MAUTOSTRIP(CAI_Item_Shield_GetShield, NULL);

	if (_pItem &&
		_pItem->IsShield())
		return (CAI_Item_Shield *)_pItem;
	else
		return NULL;
};



//CAI_ItemHandler : Handles all item of a bot/////////////////////////////

//Dummy weapon handler
CAI_Item CAI_ItemHandler::ms_DummyItem(NULL, NULL, CAI_Item::UNKNOWN);

//Constructors
CAI_ItemHandler::CAI_ItemHandler(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_ItemHandler_ctor, MAUTOSTRIP_VOID);

	m_pAI = _pAI;
	m_iWielded = 0;
	m_bUpdateItems = true;
	m_iWantedItem = CAI_Item::UNKNOWN;
};

//Returns corresponding item type of the given string
int CAI_ItemHandler::GetItemType(CStr _ItemType)
{
	MAUTOSTRIP(CAI_ItemHandler_GetItemType, 0);

	CStr Type = _ItemType.UpperCase();
	CStr Prefix = Type.GetStrSep("_");
	if (Prefix == "SHIELD")
	{
		return CAI_Item::SHIELD;
	}
	else if (Prefix == "AMMO")
	{
		return CAI_Item::AMMO;
	}
	else if (Prefix == "POTION")
	{
		Prefix = Type.GetStrSep("_");
		if (Prefix == "HEALING")
		{
			return CAI_Item::POTION_HEALING;
		}
		else
			return CAI_Item::POTION;
	}
	else if (Prefix == "ARMOUR")
	{
		return CAI_Item::ARMOUR;
	}
	else
	{
		return CAI_Item::UNKNOWN;
	}
};

//Helper to createitem. Defines what item type a specific RPG_Object_Item is.
int CAI_ItemHandler::GetItemType(CRPG_Object_Item * _pItem)
{
	MAUTOSTRIP(CAI_ItemHandler_GetItemType, 0);

	if (!_pItem)
		return CAI_Item::UNKNOWN;

	return GetItemType(_pItem->m_AIType);
};

 
//Constructs a new item object from the corresponding world object and returns smart pointer to it
spCAI_Item CAI_ItemHandler::CreateItem(CRPG_Object_Item * _pItem)
{
	MAUTOSTRIP(CAI_ItemHandler_CreateItem, NULL);
	MSCOPE(CAI_ItemHandler::CreateItem, CHARACTER);

	if (!m_pAI || !_pItem)
		return NULL;
	
	//Hack! To find any weapons in item slot, check if weapon handler can recognize item
	if ((CAI_WeaponHandler::GetWeaponType(_pItem->m_AIType) != CAI_Weapon::UNKNOWN) && m_pAI)
	{
		//Weapon! Add it to AI's weapon handler instead. Note that this weapon won't be used correctly
		//currently, unless it's used by the custom attack action
		spCAI_Weapon Wpn = m_pAI->m_Weapon.CreateWeapon(_pItem);
		m_pAI->m_Weapon.m_lspWeapons.Add(Wpn);
		m_pAI->m_AH.ReportNewWeapon(Wpn->GetType());
		return NULL;
	}
	else
	{
		//Get item type
		int iType = GetItemType(_pItem);

		//Create item handler
		switch (iType)
		{
			case CAI_Item::SHIELD:
				return MNew3(CAI_Item_Shield, _pItem, m_pAI, iType);

			case CAI_Item::AMMO:
			case CAI_Item::POTION:
			case CAI_Item::POTION_HEALING:
			case CAI_Item::ARMOUR:
			default:
				return MNew3(CAI_Item, _pItem, m_pAI, iType);
		};
	}
}; 
	
//Synch item handlers with character items
void CAI_ItemHandler::OnUpdateItems()
{
	MAUTOSTRIP(CAI_ItemHandler_OnUpdateItems, MAUTOSTRIP_VOID);

	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Iterate through item objects and make sure all items have corresponding item handlers
	int Start = 0;
	bool bFound;
	CRPG_Object_Inventory * pInventory = m_pAI->GetRPGInventory(m_pAI->m_pGameObject->m_iObject, RPG_CHAR_INVENTORY_ITEMS);
	if (pInventory)
	{
		CRPG_Object_Item * pItem;
		for (int i = 0; i < pInventory->GetNumItems(); i++)
		{
			pItem = pInventory->GetItemByIndex(i);
			bFound = false;
			for (int j = 0; j < m_lspItems.Len(); j++)
			{
				if (m_lspItems[(Start + j) % m_lspItems.Len()] &&
					(pItem == m_lspItems[(Start + j) % m_lspItems.Len()]->GetItem()))
				{
					bFound = true;
					Start = (Start + j + 1) % m_lspItems.Len();
					break;
				}
			}

			//Did we find a corresponding handler?
			if (!bFound)
			{
				//No! Create new handler
				m_lspItems.Add(CreateItem(pItem));
			}
		}

		//Are there any item handlers for non-existent weapons? 
		//(This should currently never happen, so I've made a terribly brute force solution)
/*		if (pInventory->GetNumItems() != m_lspItems.Len())
		{
			m_lspItems.Clear();
			for (int i = 0; i < pInventory->GetNumItems(); i++)
			{
				m_lspItems.Add(CreateItem(pInventory->GetItemByIndex(i)));
			};
		};*/

		m_bUpdateItems = false;
	}
};


//Initialize item handler
void CAI_ItemHandler::Init()
{
	MAUTOSTRIP(CAI_ItemHandler_Init, MAUTOSTRIP_VOID);

	//Update items...
	OnUpdateItems();
	
	//...and find wielded Item handler
	for (int i = 0; i < m_lspItems.Len(); i++)
	{
		if (m_lspItems[i] && 
			m_lspItems[i]->IsWielded())
		{
			m_iWielded = i;
			break;
		}
	}
};


//Refreshes the item handler one frame
void CAI_ItemHandler::OnRefresh()
{
	MAUTOSTRIP(CAI_ItemHandler_OnRefresh, MAUTOSTRIP_VOID);

	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Update Items when prompted explicitly or every second if we've got 
	//reason to believe that Items may have been added.
	CRPG_Object_Inventory * pInventory = m_pAI->GetRPGInventory(m_pAI->m_pGameObject->m_iObject, RPG_CHAR_INVENTORY_ITEMS);
	if (pInventory)
	{
		if (m_bUpdateItems ||
			((m_pAI->m_Timer % 20 == 0) && (m_lspItems.Len() != pInventory->GetNumItems())))
		{
			OnUpdateItems();
		};
	}

	//Stop here if there are no items
	if (m_lspItems.Len() == 0)
		return;

	//Update wielded Item if necessary
	if (!m_lspItems.ValidPos(m_iWielded) ||
		!m_lspItems[m_iWielded] ||
		!m_lspItems[m_iWielded]->IsWielded())
	{
		//Reset previously wielded Item 
		if (m_lspItems.ValidPos(m_iWielded) &&
			m_lspItems[m_iWielded])
			m_lspItems[m_iWielded]->Reset();

		//Find wielded Item handler
		for (int i = 0; i < m_lspItems.Len(); i++)
		{
			if (m_lspItems[i] && 
				m_lspItems[i]->IsWielded())
			{
				m_iWielded = i;
				break;
			}
		}
	}

	//Refresh wielded Item handler
	if (m_lspItems.ValidPos(m_iWielded) &&
		m_lspItems[m_iWielded])
		m_lspItems[m_iWielded]->OnRefresh();

	//Check if we want to wield another item
	if (m_iWantedItem != CAI_Item::UNKNOWN)
	{
		if ((GetWielded()->GetType() == m_iWantedItem) ||
			!GetItem(m_iWantedItem))
		{
			//We've switched to wanted item or can't switch to wanted item
			m_iWantedItem = CAI_Item::UNKNOWN;
		}
		else
		{
			//We haven't switched to wanted item, keep switching
			m_pAI->m_DeviceItem.Use(CAI_Device_Item::SWITCH_NEXT);
		}
	}
};

//Update items next refresh
void CAI_ItemHandler::UpdateItems()
{
	MAUTOSTRIP(CAI_ItemHandler_UpdateItems, MAUTOSTRIP_VOID);

	m_bUpdateItems = true;
};

//Currently wielded item
CAI_Item * CAI_ItemHandler::GetWielded()
{
	MAUTOSTRIP(CAI_ItemHandler_GetWielded, NULL);

	if (!m_lspItems.ValidPos(m_iWielded))
		return &ms_DummyItem;

	return m_lspItems[m_iWielded];
};

//Change AI user
void CAI_ItemHandler::ReInit(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_ItemHandler_ReInit, MAUTOSTRIP_VOID);

	m_pAI = _pAI;
	m_lspItems.Clear();
	m_bUpdateItems = true;
};


//Switch item to shield if possible
bool CAI_ItemHandler::WieldShield()
{
	MAUTOSTRIP(CAI_ItemHandler_WieldShield, false);

	if (!GetWielded()->IsShield())
	{
		for (int i = 0; i < m_lspItems.Len(); i++)
		{
			if (m_lspItems[i]->IsShield() && m_lspItems[i]->IsValid())
			{
				m_lspItems[i]->OnWield();
				return true;
			}
		}
		return false;
	}
	return true;
};


//Switch item to given itemtype if possible
bool CAI_ItemHandler::WieldItem(int _iType)
{
	MAUTOSTRIP(CAI_ItemHandler_WieldItem, false);

	if (!m_pAI)
		return false;

	if (GetWielded()->GetType() == _iType)
	{
		//Already wielding this itemtype
		return true;
	}
	else if (GetItem(_iType))
	{
		m_iWantedItem = _iType;
		m_pAI->m_DeviceItem.Use(CAI_Device_Item::SWITCH_NEXT);
		return true;
	}
	else
	{
		return false;
	}
};


//Get first item of given type (check wielded item first of all)
CAI_Item * CAI_ItemHandler::GetItem(int _iType)
{
	MAUTOSTRIP(CAI_ItemHandler_GetItem, NULL);

	if (GetWielded()->GetType() == _iType)
		return GetWielded();
	else
	{
		for (int i = 0; i < m_lspItems.Len(); i++)
		{
			if (m_lspItems[i]->GetType() == _iType)
			{
				return m_lspItems[i];
			}
		}
		return NULL;
	}
};


