#include "PCH.h"
#include "../../../WObj_Char.h"
#include "../../AICore.h"
#include "../../AI_Custom/AICore_Wraith.h"
#include "../../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "AI_Behaviour_Custom.h"


//Check if give target is valid
bool CAI_Behaviour_Wraith::IsValidTarget(CWObject_Character * _pTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_IsValidTarget, false);
	return (_pTarget &&
			m_pAI->IsEnemy(_pTarget) &&
			m_pAI->IsSpotted(m_pAI->GetCharacterListIndex(_pTarget->m_iObject)));
};


//Return object index of best target, or 0 if there is no valid target
int CAI_Behaviour_Wraith::FindTarget()
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_FindTarget, 0);
	//Always remember any valid locked target
	if (m_iTarget &&
		IsValidTarget(m_pAI->IsValidCharacter(m_iTarget)))
		return m_iTarget;
	else
	{
		//Get closest spotted enemy, if any
		fp32 MinDistSqr = _FP32_MAX;
		fp32 DistSqr;
		int iTarget = 0;
		for (int i = 0; i < m_pAI->m_lCharacters.Len(); i++)
		{
			CWObject_Character * pChar = m_pAI->IsValidCharacter(m_pAI->m_lCharacters[i].m_iObject);
			if (IsValidTarget(pChar) &&
				((DistSqr = pChar->GetPosition().DistanceSqr(m_pAI->m_pGameObject->GetPosition())) < MinDistSqr))
			{
				MinDistSqr = DistSqr;
				iTarget = pChar->m_iObject;	
			};
		};

		return iTarget;
	};
};


//Get new position to haunt. The argument is true if we're supposed to get back into our area
CVec3Dfp32 CAI_Behaviour_Wraith::GetHauntPosition(bool _bOfflimits)
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_GetHauntPosition, CVec3Dfp32());
	//Are we offlimits and have a restriction area to be offlimits from?
	CWObject * pArea = m_pAI->m_pServer->Object_Get(m_iArea);
	if (_bOfflimits &&
		pArea &&
		pArea->GetAbsBoundBox())
	{
		//Find position within area, preferrably close to current heading and position	
		//Are we outside of area bounding box?
		if (!m_pAI->IsInBoundBox(pArea, m_pAI->m_pGameObject->GetPosition()))
		{
			//Just head towards random position in bounding box...FIX
			CVec3Dfp32 Diff = pArea->GetAbsBoundBox()->m_Max - pArea->GetAbsBoundBox()->m_Min;
			CVec3Dfp32 Pos = m_pAI->GetCenterPosition(pArea);
			for (int i = 0; i < 3; i++)
				Pos[i] += Diff[i] * (Random - 0.5f);
			return Pos;
		}
		else
		{
			//Check positions along the six coordinate-axes parallell lines 
			uint8 bOK = 0x3f; //Bin 00111111;
			static const CVec3Dfp32 Dirs[6] = {CVec3Dfp32(0,0,1), CVec3Dfp32(0,0,-1), CVec3Dfp32(0,1,0), CVec3Dfp32(0,-1,0), CVec3Dfp32(1,0,0), CVec3Dfp32(-1,0,0)};
			int Loop = 0;
			fp32 Dist = 32;
			CVec3Dfp32 Tmp;
			int i;
			while (bOK)
			{
				Loop++;
				for (i = 0; i < 6; i++)
				{
					//Should we check this direction?
					if (bOK & (1 << i))
					{
						Tmp = m_pAI->m_pGameObject->GetPosition() + Dirs[i] * (Loop * Dist);

						//m_pAI->Debug_RenderWire(m_pAI->m_pGameObject->GetPosition(), Tmp, 0xffffff00);//DEBUG

						//Should we stop checking this direction?
						if (!m_pAI->IsInBoundBox(pArea, Tmp))
						{
							bOK &= ~(1 << i);
						}
						//Is the position in the area?
						else if (m_pAI->IsInArea(m_iArea, Tmp))
						{
							return Tmp;		
						};
					};
				};
			};

			//If we haven't found any position, just choose a random one within the areas bounding box
			CVec3Dfp32 Diff = pArea->GetAbsBoundBox()->m_Max - pArea->GetAbsBoundBox()->m_Min;
			CVec3Dfp32 Pos = m_pAI->GetCenterPosition(pArea);
			for (i = 0; i < 3; i++)
				Pos[i] += Diff[i] * (Random - 0.5f);
			return Pos;
		};
	}
	else
	{
		//Get position somewhat straight ahead at random distance
		fp32 Distance = m_pAI->GetMaxSpeedForward() * 5 + m_pAI->GetMaxSpeedForward() * 10 * Random;
		CVec3Dfp32 Pos = m_pAI->FindRandomPosition(Distance, 1.0f / 16, 48);	
		if (Pos == CVec3Dfp32(_FP32_MAX))
			//Can't get position, just go straight ahead
			return m_pAI->m_pGameObject->GetPosition() + CVec3Dfp32::GetRow(m_pAI->m_pGameObject->GetPositionMatrix(), 0) * Distance;
		else
			return Pos;
	};
};


//Roam around
void CAI_Behaviour_Wraith::OnHaunt()
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_OnHaunt, MAUTOSTRIP_VOID);
	//Switch to engage if we have a target
	if ((m_iTarget = FindTarget()))
	{
		OnEngage();
		return;
	};
	
	//We're haunting. Always on the move. OooooEEeeeOOOooo...
	if (m_iState != HAUNTING) 
	{
		m_iState = HAUNTING;
		m_pAI->ResetPathSearch();
		m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
	};

	//Get any area we're restrained to
	CWObject * pArea = m_pAI->m_pServer->Object_Get(m_iArea);

	//If we are restricted to an area, we must check if we get outside of it at regular intervals
	//and must sometimes initiate special moves to return to it
	if (pArea)
	{
		//Are we returning to area?
		if (m_bReturningToArea)
		{
			//Jupp, check if we've gotten back at regular intervals
			if ( (m_pAI->m_iTimer % 10 == 0) &&
				 m_pAI->IsInArea(m_iArea, m_pAI->m_pGameObject->GetPosition()) )
			{
				//Reset destination and flag
				m_bReturningToArea = false;
				m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
			};
		}
		//Check if we've left the area, and is not about to enter it again at regular intervals
		else if ( (m_pAI->m_iTimer % 5 == 0) &&
				  !m_pAI->IsInArea(m_iArea, m_pAI->m_pGameObject->GetPosition()) &&
				  !m_pAI->IsInArea(m_iArea, m_pAI->m_PathDestination) )
		{
			//We're offlimits! Find new position in area
			m_bReturningToArea = true;
			m_pAI->m_PathDestination = GetHauntPosition(true);
		};
	};
	
	//Do we have a position to haunt?
	if ( (m_pAI->m_PathDestination != CVec3Dfp32(_FP32_MAX)) &&
		 (m_pAI->m_pGameObject->GetPosition().DistanceSqr(m_pAI->m_PathDestination) > 16*16))
	{
		//Go to it
		m_pAI->OnMove(m_pAI->m_IdleMaxSpeed);
	}
	else
	{
		//Find new position, and go there
		m_pAI->OnMove(GetHauntPosition(), m_pAI->m_IdleMaxSpeed);
		
		//We can assume that the completion of this move has brought us inside any restriction area.
		m_bReturningToArea = false;
	};

	m_pAI->m_DeviceSound.MakeNoise(CAI_Device_Sound::IDLE, m_pAI->m_iMinIdleSoundsInterval, m_pAI->m_iMaxIdleSoundsInterval);
};


//Get new position to attack from. 
CVec3Dfp32 CAI_Behaviour_Wraith::GetAttackPosition(CWObject * _pTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_GetAttackPosition, CVec3Dfp32());
	//Find suitable attack position, i.e. position with LOS to target, which is at 
	//than attack distance units to target and traversable

	//Always try to attack target from the same height as it's center position
	//Create half-sphere position generator at target center position, with heading roughly towards us, but no pitch
	CPositionGenerator_HalfSphere PosGen;
	fp32 HeadingToUs = m_pAI->HeadingToPosition(_pTarget->GetPosition(), m_pAI->m_pGameObject->GetPosition(), 1.0f);
	PosGen.Init(m_pAI->GetCenterPosition(_pTarget), CVec3Dfp32(0, 0, HeadingToUs + 0.5f * (Random - 0.5f)), m_AttackDistance, 32);

	//Check position on the half sphere for LOS and traversability
	CVec3Dfp32 Pos;
	CVec3Dfp32 LOSPos = CVec3Dfp32(_FP32_MAX);
	CVec3Dfp32 Offsets[1] = {CVec3Dfp32(m_pAI->GetCenterPosition(_pTarget) - _pTarget->GetPosition())};
	while (PosGen.IsValid())
	{
		Pos = PosGen.GetNextPosition();
		//m_pAI->Debug_RenderWire(m_pAI->GetCenterPosition(_pTarget),  Pos, 0xffff0000, 5);//DEBUG

		//Check for LOS
		if (m_pAI->CheckLOS(Pos, _pTarget, Offsets, 1, m_pAI->m_pGameObject->m_iObject) != CVec3Dfp32(_FP32_MAX))
		{
			//We have LOS, check for traversablility
			if (!m_pAI->m_pNavGrid && !(m_pAI->m_pNavGrid = m_pAI->m_pServer->Path_GetBlockNav()))
			{
				//No navgrid, ignore traversability check, and accept position
				return Pos;
			}
			else if (m_pAI->IsTraversable(Pos))
			{
				//Position is traversable! 
				return Pos;
			}
			else
			{
				//Position wasn't traversable, but at least it had LOS
				if (LOSPos != CVec3Dfp32(_FP32_MAX))
					LOSPos = Pos;
			};
		};
	};

	//If we found a position with LOS, use this even though it might be intraversable
	//(since we didn't find any traversable position this round)
	if (LOSPos != CVec3Dfp32(_FP32_MAX))
		return LOSPos;
	else
	{
		//We couldn't find any good attack position. Use position directly in front of or directly behind target
		Pos = m_pAI->GetCenterPosition(_pTarget) + CVec3Dfp32::GetRow(_pTarget->GetPositionMatrix(), 0) * (m_AttackDistance * ((Abs(HeadingToUs) > 0.25f) ? -1 : 1));
		return Pos;
	};
};


//Get into position to attack target
void CAI_Behaviour_Wraith::OnEngage()
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_OnEngage, MAUTOSTRIP_VOID);
	//Should we abort engagement or switch target?
	CWObject_Character * pTarget = m_pAI->IsValidCharacter(m_iTarget);
	if (!IsValidTarget(pTarget))
	{
		if (!(m_iTarget = FindTarget()))
		{
			//Can't find target! Wtf!
			OnHaunt();
			return;
		};

		//New target found; reset attack position
		pTarget = m_pAI->IsValidCharacter(m_iTarget);
		m_pAI->ResetPathSearch();
		m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
	}
	//Should we commence attack?
	else if (InAttackPosition(m_iTarget))
	{
		OnAttack();
		return;
	};

	//Try to get into an attack position
	if (m_iState != ENGAGING) 
	{
		m_iState = ENGAGING;
		m_pAI->ResetPathSearch();
		m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
	};
	
	//Do we have an attack position?
	if (m_pAI->m_PathDestination != CVec3Dfp32(_FP32_MAX))
	{
		//Are we at attack position?
		if (m_pAI->m_PathDestination.DistanceSqr(m_pAI->m_pGameObject->GetPosition()) < 8*8)
		{
			//Aren't we facing target?
			CAI_Core_Wraith * pAI = CAI_Core_Wraith::IsAIWraith(m_pAI);
			if (!pAI || !pAI->DirectMove(m_pAI->GetCenterPosition(pTarget)))
			{
				//Move a bit towards target
				m_pAI->OnMove(m_pAI->m_pGameObject->GetPosition() + ((m_pAI->GetCenterPosition(pTarget) - m_pAI->m_pGameObject->GetPosition()) * 0.2f));
			}
			else
			{
				//Get new attack position and move there
				m_pAI->OnMove(GetAttackPosition(pTarget));
			};
		}
		else
		{
			//Move to it!
			m_pAI->OnMove();
		};
	}
	else
	{
		//Find attack position and move to it!
		m_pAI->OnMove(GetAttackPosition(pTarget));
	};
};


//Perform attack run
void CAI_Behaviour_Wraith::OnAttack()
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_OnAttack, MAUTOSTRIP_VOID);
	//Wheeeee! Consume his soul! 
	if (m_iState != ATTACKING) 
	{
		m_iState = ATTACKING;
		m_pAI->ResetPathSearch();
		m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
	};

	//Make sure we've got a target
	CWObject_Character * pTarget = m_pAI->IsValidCharacter(m_iTarget);
	if (!IsValidTarget(pTarget))
	{
		if (!(m_iTarget = FindTarget()))
		{
			//Can't find target! 
			OnHaunt();
			return;
		};

		//New target found; reset attack position
		pTarget = m_pAI->IsValidCharacter(m_iTarget);
		m_pAI->ResetPathSearch();
		m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
	};

	//Hold until we're ready for attack
	if (m_pAI->Wait())
	{
		m_pAI->AddAimAtPositionCmd(m_pAI->GetCenterPosition(pTarget));
		m_pAI->OnIdle();
		m_pAI->ResetPathSearch();
		m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
	}
	//Can we attack right now?
	else if (m_pAI->m_Weapon.GetWielded()->GetEstimate(pTarget) &&
			 m_pAI->IsInBoundBox(pTarget, m_pAI->GetCenterPosition(m_pAI->m_pGameObject)))
	{
		//Unleash attack and switch to recovering 
//		m_pAI->OnAttack(pTarget);
		OnRecover();
		return;
	}
	else
	{
		//Charge! Move directly towards target focus position if we're very close to target or if target has moved too much
		if ((m_pAI->m_pGameObject->GetPosition().DistanceSqr(m_pAI->m_PathDestination) < 8*8) ||
			(m_pAI->m_PathDestination.DistanceSqr(m_pAI->GetCenterPosition(pTarget)) > 0.2f*0.2f * m_pAI->m_pGameObject->GetPosition().DistanceSqr(pTarget->GetPosition())))
		{
			m_pAI->OnMove(m_pAI->GetCenterPosition(pTarget));
		}	
		else
		{
			m_pAI->OnMove();
		};
	};
};


//Recover after attack
void CAI_Behaviour_Wraith::OnRecover()
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_OnRecover, MAUTOSTRIP_VOID);
	//Boo-yaa! Got the sucker!
	if (m_iState != RECOVERING) 
	{
		m_iState = RECOVERING;
		m_pAI->ResetPathSearch();

		//Set recover destination to straight ahead, plus some scatter
		CVec3Dfp32 Pos = m_pAI->FindRandomPosition(m_AttackDistance, 1.0f / 16, 48);	
		if (Pos == CVec3Dfp32(_FP32_MAX))
			//Can't get position, just go straight ahead
			Pos = m_pAI->m_pGameObject->GetPosition() + CVec3Dfp32::GetRow(m_pAI->m_pGameObject->GetPositionMatrix(), 0) * m_AttackDistance;
		m_pAI->m_PathDestination = Pos;
	};

	//Are we done recovering?
	if (m_pAI->m_pGameObject->GetPosition().DistanceSqr(m_pAI->m_PathDestination) < 16*16)
	{
		//We're at destination, switch to engage
		OnEngage();
		return;
	};

	//Just move to recover position
	m_pAI->OnMove();
};


//Are we in position to attack target?
bool CAI_Behaviour_Wraith::InAttackPosition(int _iTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_InAttackPosition, false);
	CWObject_Character * pTarget = m_pAI->IsValidCharacter(_iTarget);
	if (!IsValidTarget(pTarget))
		return false;

	//In attack position if there's LOS to target, we're at least half attack distance 
	//units away from target and aiming towards it.
	CAI_Core_Wraith * pAI = CAI_Core_Wraith::IsAIWraith(m_pAI);
	if (pAI &&
		pAI->DirectMove(m_pAI->GetCenterPosition(pTarget)) &&
		(m_pAI->m_pGameObject->GetPosition().DistanceSqr(pTarget->GetPosition()) >= Sqr(0.5f * m_AttackDistance)) &&
		m_pAI->CheckLOS(_iTarget) != CVec3Dfp32(_FP32_MAX))
		return true;
	else
		return false;
};


//Initializes behaviour
CAI_Behaviour_Wraith::CAI_Behaviour_Wraith(CAI_Core * _pAI, int _iArea, fp32 _AttackDistance)
:CAI_Behaviour(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_ctor, MAUTOSTRIP_VOID);
	m_iType = WRAITH;
	m_iArea = _iArea;
	m_AttackDistance = (_AttackDistance >= 0) ? _AttackDistance : 100.0f;
	m_iState = HAUNTING;
	m_iTarget = 0;
	m_bReturningToArea = false;
	m_iAttackTimer = 0;
};


//Will zoom about in a random fashion, attacking any valid targets that are spotted.
void CAI_Behaviour_Wraith::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_OnTakeAction, MAUTOSTRIP_VOID);
	CAI_Core_Wraith * pAI = CAI_Core_Wraith::IsAIWraith(m_pAI);
	if (!pAI)
	{
		//This ain't a wraith!
		m_pAI->OnIdle();
		return;
	};

	switch (m_iState)
	{
	case HAUNTING:
		OnHaunt();
		break;
	case ENGAGING:
		OnEngage();
		break;
	case ATTACKING:
		OnAttack();
		break;
	case RECOVERING:
		OnRecover();
		break;
	default:
		m_pAI->OnIdle();
		break;
	};
};


//Resets stuff.
void CAI_Behaviour_Wraith::Reset()
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_Reset, MAUTOSTRIP_VOID);
	m_spSubBehaviour = NULL;
	m_iState = HAUNTING;
	m_iTarget = 0;
	m_bReturningToArea = false;
	m_iAttackTimer = 0;
};


//Re-init area
void CAI_Behaviour_Wraith::ReInit(int iParam1, int iParam2, CVec3Dfp32 VecParam1, CVec3Dfp32 VecParam2)
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_ReInit, MAUTOSTRIP_VOID);
	m_iArea = iParam1;
	m_AttackDistance = (iParam2 >= 0) ? iParam2 : 100.0f;
	Reset();	
};


#ifndef M_RTM
//Get some debug info about the bot
CStr CAI_Behaviour_Wraith::GetDebugString()
{
	MAUTOSTRIP(CAI_Behaviour_Wraith_GetDebugString, CStr());
	CStr str = CAI_Behaviour::GetDebugString();
	switch (m_iState)
	{
	case HAUNTING:
		{
			str = "Haunting ";
			if (m_iArea)
				str += "area " + m_pAI->GetDebugName(m_iArea) + " ";
		};
		break;
	case ENGAGING:
		{
			str = "Engaging " + m_pAI->GetDebugName(m_iTarget) + " ";
		};
		break;
	case ATTACKING:
		{
			str = "Attacking " + m_pAI->GetDebugName(m_iTarget) + " ";
		};
		break;
	case RECOVERING:
		{
			str = "Recovering from attack against " + m_pAI->GetDebugName(m_iTarget) + " ";
		};
		break;
	default:
		str = "WARNING! Invalid behaviour state! "; 		
		break;
	};
	
	return str;		
};
#endif

