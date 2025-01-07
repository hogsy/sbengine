#include "PCH.h"
#include "../../../WObj_Char.h"
#include "../../AICore.h"
#include "../../AI_Custom/AICore_Bosses.h"
#include "../../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "AI_Behaviour_Custom.h"


//Initializes behaviour
CAI_Behaviour_Durzu::CAI_Behaviour_Durzu(CAI_Core * _pAI, int _iArea, int _Spawn)
:CAI_Behaviour_Engage(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Durzu_ctor, MAUTOSTRIP_VOID);

	m_iType = DURZU;
	m_iMode = NONE;
	m_StartPos = CVec3Dfp32(_FP32_MAX);
	m_iTurnTimer = 0;
	m_iSettleTimer = 0;
	m_iHighJumpSeq = 0;
	m_iHighJumpTimer = 0;
	m_iArea = _iArea;
	m_bAttack = false;	
	m_nSpawns = _Spawn;
	if (m_nSpawns < 0)
		m_nSpawns = 2;
	m_lSpawns.Clear();
};

fp32 CAI_Behaviour_Durzu::GetTimeScale()
{
	MAUTOSTRIP(CAI_Behaviour_Durzu_GetTimeScale, 0.0f);
	//Return durzu AI slowdown factor intead of character timescale
	CAI_Core_Durzu * pAIDurzu = CAI_Core_Durzu::IsAIDurzu(m_pAI);
	if (pAIDurzu)
		return Max(0.01f, pAIDurzu->GetSlowdownFactor());
	else
		return 1.0f;
	
	//Character timescale below. Not used.
/*	CWObject_Character	*pDurzu;
	if(pDurzu = CWObject_Character::IsCharacter(m_pAI->m_pGameObject->m_iObject, m_pAI->m_pServer))
	{
		CWO_Character_ClientData *pCD =  CWObject_Character::GetClientData(pDurzu);
		if(pCD)
		{
			return pCD->m_Item0_TimeScale;
		}
	}
	return 1.0f;*/
}


//Go to position from which we can attack target
void CAI_Behaviour_Durzu::OnFollowTarget(CWObject * _pTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Durzu_OnFollowTarget, MAUTOSTRIP_VOID);
	//Stop if close to target
	if (_pTarget->GetPosition().DistanceSqr(m_pAI->m_pGameObject->GetPosition()) < 200*200)
	{
		m_pAI->m_DeviceMove.Use();
	}
	else
	{
		//Check if we can move towards the target without going out of our movement area
		CVec3Dfp32 FuturePos = m_pAI->m_pGameObject->GetPosition() + (_pTarget->GetPosition() - m_pAI->m_pGameObject->GetPosition()).Normalize() * 128.0f + CVec3Dfp32(0,0,20);

		m_pAI->Debug_RenderWire(FuturePos, FuturePos + CVec3Dfp32(0,0,100));//DEBUG
		CPixel32 Clr = 0xff00ff00;
		fp32 Time = 0.1f;
		bool bRes = false;
		CStr Res = "";
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSGBASE_AREAINFO + 4, (int)(&Clr), (int)(&Time)), m_iArea);

		if(!m_iArea || (m_iArea && m_pAI->IsInArea(m_iArea, FuturePos)))
		{
			//m_pAI->OnFollowTarget(_pTarget);
			m_pAI->AddMoveTowardsPositionCmd(_pTarget->GetPosition());
		}
		else
		{
			//Can't move further (not quite true actually... will fix if necessary)
			m_pAI->m_DeviceMove.Use();
		}
	}
}


//Act according to current mode or just kill anything around
void CAI_Behaviour_Durzu::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Durzu_OnTakeAction, MAUTOSTRIP_VOID);
	//Can't take action if we don't have a valid AI
	if (!m_pAI || !m_pAI->IsValid())
		return;

	if (m_StartPos == CVec3Dfp32(_FP32_MAX))
		m_StartPos = m_pAI->m_pGameObject->GetPosition();

	fp32 TimeScale = GetTimeScale();
	fp32 SettleMod = Sqr(1 / TimeScale);

	//Count valid spawns, and remove invalid ones
	int nSpawns = 0;
	CWObject_Message Valid(OBJMSG_ARE_YOU_VALID);
	for (int i = 0; i < m_lSpawns.Len(); i++)
	{
		if (m_lSpawns[i] != 0)
		{
			if (m_pAI->m_pServer->Message_SendToObject(Valid, m_lSpawns[i]))
				nSpawns++;
			else
				m_lSpawns[i] = 0;
		}
	}

	//Are we escaping from lava?
	if ((m_iMode == LAVAESCAPE) ||
		((m_pAI->m_iTimer % 10 == 0) && 
		 ((m_pAI->m_pServer->Phys_GetMedium(m_pAI->m_pGameObject->GetPosition()) & XW_MEDIUM_LIQUID) ||
		  (m_pAI->m_pServer->Phys_GetMedium(m_pAI->m_pGameObject->GetPosition() + CVec3Dfp32(0,0,32)) & XW_MEDIUM_LIQUID))))
	{
		OnEscapeLava();
	}
	//Always complete a started taunt
	else if (m_pAI->m_iActionFlags & CAI_Core::TAUNTING)
	{
		//Note that target may be dead or missing by this time. If this is the case, complete 
		//taunt without changing look direction.
		CWObject * pTarget = m_pAI->m_pServer->Object_Get(m_iTarget);
		if (pTarget)
		{
			m_pAI->OnTaunt(pTarget);
		}
		else
		{
			//No target. Use self as dummy target
			m_pAI->m_DeviceLook.Use();
			m_pAI->OnTaunt(m_pAI->m_pGameObject);				
		}
	}
	//Are we turning?
	else if ((m_iMode == TURNING) &&
			 (m_iTurnTimer > 0))
	{
		//Continue turning without moving (make sure body turns with look)
		m_iTurnTimer--;
		m_pAI->m_DeviceMove.Use();
		m_pAI->m_DeviceLook.Use(CVec3Dfp32(0, 0, m_LookTurnSpeed));
		CWO_Character_ClientData * pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
		if (pCD)
			pCD->m_Anim_BodyAngleZ = pCD->m_Anim_BodyAngleZ + m_BodyTurnSpeed;

		//If we're done, allow some extra time for settling
		if (m_iTurnTimer == 0)
		{
			m_iMode = SETTLING;
			m_iSettleTimer = 5 * SettleMod;
		}
	}
	//Are we settling after a performed animation?
	else if ((m_iMode == SETTLING) &&
			 (m_iSettleTimer > 0))
	{
		//Jupp, do nothing
		CWObject *pObj = m_pAI->m_pServer->Object_Get(m_iTarget);
		if(pObj)
		{
			m_pAI->OnAim(pObj);
			m_pAI->m_DeviceLook.Free();
		}		
		m_pAI->m_DeviceLook.Use();
		m_pAI->m_DeviceMove.Use();
		m_iSettleTimer--;

		//We might consider switching weapons if we've got a target
		CWObject_Character * pTarget = AcquireTarget();
		if (pTarget)
		{
			//Switch to spawn lava ball if we need spawns, quake fist if target is close or 
			//ordinary lava balls otherwise
			if (nSpawns < m_nSpawns)
			{
				if (m_pAI->m_Weapon.GetWielded()->GetType() != CAI_Weapon::SPAWNBALL)
					m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, CAI_Core_Durzu::SLOT_LAVABALL_SPAWN);
			}
			else if (m_pAI->m_pGameObject->GetPosition().DistanceSqr(pTarget->GetPosition()) < 600*600)
			{
				if (!m_pAI->m_Weapon.GetWielded()->IsMeleeWeapon())
					m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, CAI_Core_Durzu::SLOT_QUAKEFIST);
			}
			else
			{
				if (m_pAI->m_Weapon.GetWielded()->GetType() != CAI_Weapon::BALL)
					m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, CAI_Core_Durzu::SLOT_LAVABALL);
			};
		}
	}
	else
	{
		//Default mode
		m_iMode = NONE;

		//Do we have a valid target?
		CWObject_Character * pTarget = AcquireTarget();

		//Do we have a valid target now?
		if (pTarget)
		{
			//Durzu will always try to look towards player. If the angle is too great he will have to turn
			CWO_Character_ClientData * pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
			fp32 TargetHeading = m_pAI->HeadingToPosition(m_pAI->GetAimPosition(m_pAI->m_pGameObject), m_pAI->GetCenterPosition(pTarget), 1.0f);
			fp32 Diff = CAI_Core::AngleDiff(TargetHeading, pCD->m_Anim_BodyAngleZ);
			if (Abs(Diff) <= 0.125f)
			{
				//Sweep gaze towards target feet
				CVec3Dfp32 Look = m_pAI->GetLook(m_pAI->m_pGameObject);
				fp32 HeadingDiff = CAI_Core::AngleDiff(TargetHeading, Look[2]);
				fp32 TargetPitch = m_pAI->PitchToPosition(m_pAI->GetAimPosition(m_pAI->m_pGameObject), pTarget->GetPosition(), 0.5f);
				if (TargetPitch > 0.25f)
					TargetPitch = 0.5f - TargetPitch;
				else if (TargetPitch < -0.25f)
					TargetPitch = -0.5f - TargetPitch;
				fp32 PitchDiff = CAI_Core::AngleDiff(TargetPitch, Look[1]);
				m_pAI->m_DeviceLook.Use(CVec3Dfp32(0, PitchDiff * 0.2f, HeadingDiff * 0.2f));
			}
			

			if ( //If wielding lavaball_spawn, we should always use it if we need spawns and it's usable
				 ((m_pAI->m_Weapon.GetWielded()->GetType() == CAI_Weapon::SPAWNBALL) &&
				  (nSpawns < m_nSpawns) &&
				  (m_bAttack || (m_pAI->m_Weapon.GetWielded()->GetEstimate(pTarget) > 50))) 
				 ||
				 //If wielding quakefist we should always use it if it's good, even though not facing the right direction
				 //Don't use it if we need to spawn more gnomes though
				 (m_pAI->m_Weapon.GetWielded()->IsMeleeWeapon() &&
				  (nSpawns >= m_nSpawns) &&
				  (m_bAttack || (m_pAI->m_Weapon.GetWielded()->GetEstimate(pTarget) > 90))) )
			{
				if (m_bAttack)
				{
					//Attack now!
					m_pAI->m_DeviceLook.Use();
					m_pAI->OnAttack(pTarget);

					//We are were we want to be so reset path and stay put
					m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);					
					m_pAI->ResetPathSearch();
					m_pAI->m_DeviceMove.Use();
					
					//Settle for a while after making attack
					m_iMode = SETTLING;
					m_iSettleTimer = (50 + Random * 20) * SettleMod;

					//Reset attack flag
					m_bAttack = false;
				}
				else
				{
					//Settle for a short while, then attack
					m_iMode = SETTLING;
					m_iSettleTimer = 5 * SettleMod;
					m_pAI->m_DeviceLook.Use();
					m_pAI->m_DeviceMove.Use();
					m_bAttack = true;
				}
			}
			//Check if we should start turning left or right
			else if (Abs(Diff) > 0.125f)
			{
				m_iMode = TURNING;
				CWObject_Message Msg(OBJMSG_CHAR_PLAYANIMSEQUENCE);
				Msg.m_Param0 = (Diff < 0) ? ANIM_TURNLEFT : ANIM_TURNRIGHT;
				m_iTurnTimer = m_pAI->m_pGameObject->OnMessage(Msg);
				m_iTurnTimer = (int)((1.0f/TimeScale) * m_iTurnTimer);
				fp32 LookDiff = CAI_Core::AngleDiff(TargetHeading, m_pAI->GetLook(m_pAI->m_pGameObject)[2]);	
				
				//Make sure we don't turn too far in one go
				if (Abs(Diff) > 0.25f)
					 Diff = 0.25f * ((Diff > 0) ? 1 : -1);
				if (Abs(LookDiff) > 0.25f)
					 LookDiff = 0.25f * ((LookDiff > 0) ? 1 : -1);

				//Set time to turn
				if (m_iTurnTimer)
				{
					m_BodyTurnSpeed = Diff / m_iTurnTimer;
					m_LookTurnSpeed = LookDiff / m_iTurnTimer;
				}
				else
				{
					m_BodyTurnSpeed = 0;
					m_LookTurnSpeed = 0;
				}

				//Don't do anything 
				m_pAI->m_DeviceLook.Use();
				m_pAI->m_DeviceMove.Use();
			}
			//Have we already decided to attack and have settled and/or turned?
			else if (m_bAttack)
			{
				//Attack now!
				m_pAI->m_DeviceLook.Use();
				m_pAI->OnAttack(pTarget);

				//We are were we want to be so reset path and stay put
				m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);					
				m_pAI->ResetPathSearch();
				m_pAI->m_DeviceMove.Use();
				
				//Settle for a while after making attack
				m_iMode = SETTLING;
				m_iSettleTimer = (50 + Random * 20) * SettleMod;

				//Reset attack flag
				m_bAttack = false;
			}
			//Should we initiate lava ball attack? 
			else if ((m_pAI->m_Weapon.GetWielded()->GetType() == CAI_Weapon::BALL) &&
					 m_pAI->m_spPersonality->ShouldAttack(m_pAI->m_Weapon.GetWielded(), pTarget))
			{
				//Settle for a short while, then attack
				m_iMode = SETTLING;
				m_iSettleTimer = 5;
				if (pCD)
					m_iSettleTimer /= pCD->m_Item0_TimeScale;

				m_pAI->m_DeviceLook.Use();
				m_pAI->m_DeviceMove.Use();
				m_bAttack = true;
			}
			else
			{
				m_bAttack = false;

				//Switch to spawn lava ball if we need spawns, quake fist if target is close or 
				//ordinary lava balls otherwise
				if (nSpawns < m_nSpawns)
				{
					if (m_pAI->m_Weapon.GetWielded()->GetType() != CAI_Weapon::SPAWNBALL)
						m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, CAI_Core_Durzu::SLOT_LAVABALL_SPAWN);
				}
				else if (m_pAI->m_pGameObject->GetPosition().DistanceSqr(pTarget->GetPosition()) < 600*600)
				{
					if (!m_pAI->m_Weapon.GetWielded()->IsMeleeWeapon())
						m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, CAI_Core_Durzu::SLOT_QUAKEFIST);
				}
				else
				{
					if (m_pAI->m_Weapon.GetWielded()->GetType() != CAI_Weapon::BALL)
						m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, CAI_Core_Durzu::SLOT_LAVABALL);
				};

				//Go towards target
				OnFollowTarget(pTarget);
			};
		}
		//Let sub-behaviour take action if there is any
	//	else if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
	//	{
	//		m_spSubBehaviour->OnTakeAction();
	//	}
		//Is there a threat?
	//	else if (m_pAI->OnThreatAlert())
	//	{
			//We're looking for a threat
	//	}
		else
		{
			//No valid target, just stay
			m_spSubBehaviour = NULL;
			m_pAI->OnIdle();
		};
	}
};


//Hack! Escape when fallen into lava
void CAI_Behaviour_Durzu::OnEscapeLava()
{
	MAUTOSTRIP(CAI_Behaviour_Durzu_OnEscapeLava, MAUTOSTRIP_VOID);
	bool bLavaCheck = false;

	//Check if we're stuck and should jump (don't check first frame of lava escape)
	if (m_iMode != LAVAESCAPE)
	{
		m_iMode = LAVAESCAPE;
		
		//Reset highjump-stuff
		m_iHighJumpSeq = 0;
		m_iHighJumpTimer = 0;
	}
	//Are we performing a highjump?
	else if (m_iHighJumpSeq == ANIM_HIGHJUMP_ANTICIPATION)
	{
		//Tensing before jump; switch to liftoff when anim is done
		m_iHighJumpTimer--;
		if (m_iHighJumpTimer <= 0)
		{
			//Time for jump!
			m_iHighJumpSeq = ANIM_HIGHJUMP_LIFTOFF;
			m_iHighJumpTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HIGHJUMP_LIFTOFF));
			m_pAI->m_pServer->Object_AddVelocity(m_pAI->m_pGameObject->m_iObject, CVec3Dfp32(0, 0, 50));
		}
		else
		{
			//Stand still
			m_pAI->m_DeviceMove.Use();
		}
	}
	else if (m_iHighJumpSeq == ANIM_HIGHJUMP_LIFTOFF)
	{
		//Push off from ground; switch to summit if passed summit of jump, or rise otherwise when anim is done
		m_iHighJumpTimer--;
		if (m_iHighJumpTimer <= 0)
		{
			if (m_pAI->m_pGameObject->GetPosition()[2] <= m_pAI->m_pGameObject->GetLastPosition()[2])
			{
				//Reached or passed summit
				m_iHighJumpSeq = ANIM_HIGHJUMP_SUMMIT;
				m_iHighJumpTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HIGHJUMP_SUMMIT));
			}
			else
			{
				//Still rising
				m_iHighJumpSeq = ANIM_HIGHJUMP_RISE;
				m_iHighJumpTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HIGHJUMP_RISE));
			}
		}
		//Allow movement
	}
	else if (m_iHighJumpSeq == ANIM_HIGHJUMP_RISE)
	{
		//Rise through air; switch to summit when we're at summit of jump
		if (m_pAI->m_pGameObject->GetPosition()[2] <= m_pAI->m_pGameObject->GetLastPosition()[2])
		{
			//Reached or passed summit
			m_iHighJumpSeq = ANIM_HIGHJUMP_SUMMIT;
			m_iHighJumpTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HIGHJUMP_SUMMIT));
		}
		//Allow movement
	}
	else if (m_iHighJumpSeq == ANIM_HIGHJUMP_SUMMIT)
	{
		//Start of fall; switch to land if we've struck ground or fall otherwise when anim is done
		m_iHighJumpTimer--;
		if (m_iHighJumpTimer <= 0)
		{
			if (m_pAI->m_pGameObject->GetPosition()[2] >= m_pAI->m_pGameObject->GetLastPosition()[2])
			{
				//Struck ground
				m_iHighJumpSeq = ANIM_HIGHJUMP_LAND;
				m_iHighJumpTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HIGHJUMP_LAND));
			}
			else
			{
				//Still falling
				m_iHighJumpSeq = ANIM_HIGHJUMP_FALL;
				m_iHighJumpTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HIGHJUMP_FALL));
			}
		}
		//Allow movement
	}
	else if (m_iHighJumpSeq == ANIM_HIGHJUMP_FALL)
	{
		//Fall down; switch to land when striking ground
		if (m_pAI->m_pGameObject->GetPosition()[2] >= m_pAI->m_pGameObject->GetLastPosition()[2])
		{
			//Struck ground
			m_iHighJumpSeq = ANIM_HIGHJUMP_LAND;
			m_iHighJumpTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HIGHJUMP_LAND));
		}
		//Allow movement
	}
	else if (m_iHighJumpSeq == ANIM_HIGHJUMP_LAND)
	{
		//Land, then settle for a while. Check if we got out of lava.
		m_iHighJumpTimer--;
		if (m_iHighJumpTimer <= -10) 
		{
			//Highjump is completed. Check if we're out of lava.
			m_iHighJumpSeq = 0;
			m_iHighJumpTimer = 0;
			bLavaCheck = true;
		}
		//Stand still 
		m_pAI->m_DeviceMove.Use();
	}
	else if (m_pAI->m_pGameObject->GetPosition().DistanceSqr(m_pAI->m_pGameObject->GetLastPosition()) < 2*2)
	{
		//Stuck; initiate highjump!
		m_iHighJumpSeq = ANIM_HIGHJUMP_ANTICIPATION;
		m_iHighJumpTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HIGHJUMP_ANTICIPATION));
		m_pAI->m_DeviceMove.Use();
	}
	else
	{
		//Check if we're out of lava every ten frames
		bLavaCheck = (m_pAI->m_iTimer % 10 == 0);
	}

	//Should we move?
	if (m_pAI->m_DeviceMove.IsAvailable())
	{
		//Move towards current target or starting position
		CWObject * pTarget = m_pAI->m_pServer->Object_Get(m_iTarget);
		if (pTarget)
			m_pAI->AddMoveTowardsPositionCmd(pTarget->GetPosition());
		else
			m_pAI->AddMoveTowardsPositionCmd(m_StartPos);
	}

	//Should we check if we're out of the lava
	if (bLavaCheck &&
		!((m_pAI->m_pServer->Phys_GetMedium(m_pAI->m_pGameObject->GetPosition()) & XW_MEDIUM_LIQUID) ||
		  (m_pAI->m_pServer->Phys_GetMedium(m_pAI->m_pGameObject->GetPosition() + CVec3Dfp32(0,0,32)) & XW_MEDIUM_LIQUID)))
	{
		//No longer in lava!
		m_iMode = NONE;
	};
};

//Handles incoming messages
bool CAI_Behaviour_Durzu::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CAI_Behaviour_Durzu_OnMessage, false);
	switch (_Msg.m_Msg)
	{
	case CAI_Core::OBJMSG_IM_ALIIIVE:
		{
			if (!m_pAI || !m_pAI->m_pServer)
				return false;

			//Check if this is a durzu spawn, and add it to list of spawns if so
			//Compare template name (ugly but wtf)
			CWObject * pObj = m_pAI->m_pServer->Object_Get(_Msg.m_iSender);
			if (pObj && (CStr(pObj->GetTemplateName()) == CStr("AI_StoneGnome_DurzuSpawn")))
			{
				for (int i = 0; i < m_lSpawns.Len(); i++)
				{
					if (m_lSpawns[i] == 0)
					{
						m_lSpawns[i] = _Msg.m_iSender;
						return true;
					}
				}
				m_lSpawns.Add(_Msg.m_iSender);
			}
		}
		return true;
	default:
		return CAI_Behaviour_Engage::OnMessage(_Msg);
	}
}


//Always max prio
int CAI_Behaviour_Durzu::GetPathPrio(fp32 _CameraScore)
{
	MAUTOSTRIP(CAI_Behaviour_Durzu_GetPathPrio, 0);
	return 255;
};
