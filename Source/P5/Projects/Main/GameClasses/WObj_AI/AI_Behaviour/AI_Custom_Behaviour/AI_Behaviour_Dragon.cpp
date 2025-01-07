#include "PCH.h"
#include "../../../WObj_Char.h"
#include "../../AICore.h"
#include "../../AI_Custom/AICore_Bosses.h"
#include "../../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "AI_Behaviour_Custom.h"
#include "../../../WRPG/WRPGChar.h"


//Initializes behaviour
CAI_Behaviour_Dragon::CAI_Behaviour_Dragon(CAI_Core * _pAI,	int	_iStartNavNode)
	:CAI_Behaviour_Engage(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Dragon_ctor, MAUTOSTRIP_VOID);

	m_iType = DRAGON;
	m_iMode = NONE;
	m_bOnRun = false;
	m_iDiePath = _iStartNavNode;
	m_ResetLook = 0;
	m_SwitchCounter = 0;
	m_bFallenBehind = true;
	m_TrueFollowTime = 0;
	m_bHooveringSet = false;
	m_bAtDiePos = false;
	m_DieTimer = 0;
	m_bOnDieRun = false;
	m_DamageTimer = 0;
	m_bHoldFire = false;;
	m_iAttackRuns.Clear();
	m_iPath = 0;
	m_Wait = 0;
	m_pAI->m_pGameObject->ClientFlags() |= CWO_CLIENTFLAGS_LINKINFINITE;
	m_bTargetLockOn = false;
};




//Are we in position to attack target?
bool CAI_Behaviour_Dragon::InAttackPosition(int _iTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Dragon_InAttackPosition, false);
	CWObject_Character * pTarget = m_pAI->IsValidCharacter(_iTarget);
	if (!IsValidTarget(_iTarget))
		return false;

	//In attack position if there's LOS to target, we're at least half attack distance 
	//units away from target and aiming towards it.
	CAI_Core_Flyer * pAI = CAI_Core_Dragon::IsAIFlyer(m_pAI);
	if (pAI &&
		(m_pAI->m_pGameObject->GetPosition().DistanceSqr(pTarget->GetPosition()) < Sqr(700)) &&
		m_pAI->CheckLOS(_iTarget) != CVec3Dfp32(_FP32_MAX))
		return true;
	else
		return false;
};


//Move into attack position against target and attack
void CAI_Behaviour_Dragon::OnEngage(CWObject * _pTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Dragon_OnEngage, MAUTOSTRIP_VOID);
//	ConOut("-----------------------------------");
	if (!_pTarget)
		return;
	
	CAI_Weapon_Dragonbreath *pWeapon = TDynamicCast<CAI_Weapon_Dragonbreath > (m_pAI->m_Weapon.GetWielded());

	fp32 MaxRange = 0;
	fp32 MinRange = _FP32_MAX;
	
	if(pWeapon)
	{
		MaxRange = pWeapon->MaxRange();
		MinRange = pWeapon->MinRange();
	}

	fp32 DistSqr = m_pAI->m_pGameObject->GetPosition().DistanceSqr(_pTarget->GetPosition());

	if(m_SwitchCounter <=0 && (DistSqr > Sqr(MaxRange) || DistSqr < Sqr(MinRange)))
	{
		m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH);
		m_SwitchCounter = 3;
	}

	//Should we attack? (This is placed last so that look device will have been used by now.)
	CWObject * pPath = m_pAI->m_pServer->Object_Get(m_iPath);
	if (pPath )
	{
		
		CVec3Dfp32 PathDir = CVec3Dfp32::GetMatrixRow(pPath->GetPositionMatrix(), 0);	
		PathDir[2] = 0.0f;
		PathDir.Normalize();
		CVec3Dfp32 Dir = _pTarget->GetPosition() - m_pAI->m_pGameObject->GetPosition();
		Dir[2] = 0.0f;
		Dir.Normalize();
		fp32 CosA = PathDir * Dir;
		CVec3Dfp32 RelPos = _pTarget->GetPosition() - m_pAI->GetAimPosition(m_pAI->m_pGameObject);
			
		fp32 Pitch = M_ASin(RelPos[2]/RelPos.Length());
		Pitch /= 2*_PI;			
	
		if (Pitch > -0.2f && Pitch < 0.05f && CosA > 0.7f && m_pAI->m_Weapon.GetWielded()->GetEstimate(_pTarget)>0)
		{
			m_ResetLook = 20;
//			CVec3Dfp32 TargetPos = _pTarget->GetPosition() - m_pAI->m_pGameObject->GetMoveVelocity();
			CVec3Dfp32 AimPos;
			CMat43fp32 AimMat;
			if(pWeapon)
			{
				pWeapon->GetActivatePosition(&AimMat);
				AimPos = CVec3Dfp32::GetMatrixRow(AimMat,3);
			}
			else
			{
				AimPos = m_pAI->GetAimPosition(m_pAI->m_pGameObject);
			}
			CVec3Dfp32 TargetPos = _pTarget->GetPosition();//m_pAI->GetCenterPosition(_pTarget);
			TargetPos[0] += Random * 32.0f - 16.0f;
			TargetPos[1] += Random * 32.0f - 16.0f;
			TargetPos[2] += Random * 32.0f - 16.0f;
			//Get angle in radians
			RelPos = TargetPos - m_pAI->GetAimPosition(m_pAI->m_pGameObject);
			
			Pitch = M_ASin(RelPos[2]/RelPos.Length());
			//Convert to fractions
			Pitch /= 2*_PI;

//			m_pAI->AddAimAtPositionCmd(_pTarget->GetPosition()-(m_pAI->m_pGameObject->GetPosition()-m_pAI->m_pGameObject->GetLastPosition())*5.0f );
			//		if(Random * 100  < 10)
			if(!m_bHoldFire && m_bTargetLockOn)
			{
				m_pAI->OnAttack(_pTarget);
				m_bTargetLockOn = false;
			}
			m_pAI->OnLook(m_pAI->HeadingToPosition(AimPos, TargetPos),
							Pitch,
							1);
			m_bTargetLockOn = true;
		}
	}
};



void CAI_Behaviour_Dragon::OnFollowPath()
{
	MAUTOSTRIP(CAI_Behaviour_Dragon_OnFollowPath, MAUTOSTRIP_VOID);
	//Check that at least move device is available
	if (!m_pAI->m_DeviceMove.IsAvailable())
		//Device unavailable, abort!
		return;

	//Get path object
	CWObject * pPath;
	if (!m_iPath || 
		!(pPath = m_pAI->m_pServer->Object_Get(m_iPath)))
	{
		//Can't get path, stay put and abort!
		m_pAI->OnIdle();
		m_iPath = 0;

		return;
	};

	//Have we just reached path?
/*	if ((pPath->GetPosition().DistanceSqr(m_pAI->m_pGameObject->GetPosition()) < 32*32))
		m_bFallenBehind = false;
	else
		m_bFallenBehind = true;
	fp32 Pitch;	
	CVec3Dfp32 RelPos = pPath->GetPosition() - m_pAI->GetAimPosition(m_pAI->m_pGameObject);
	if (false)//m_bFallenBehind)
	{
		Pitch = M_ASin(RelPos[2]/RelPos.Length());
		//Convert to fractions
		Pitch /= 2*_PI;

		if(m_ResetLook <= 0)
		{
			CMat4Dfp32 NextMat;
			fp32 Time;
			CWObject_Message TimeMsg(OBJMSG_HOOK_GETTIME);
			TimeMsg.m_pData = &Time;
			m_pAI->m_pServer->Message_SendToObject(TimeMsg, m_iPath);
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (int)(&Time), (int)(&NextMat), m_pAI->m_pGameObject->m_iObject), m_iPath);
	
			m_pAI->OnLook(m_pAI->HeadingToPosition(m_pAI->GetAimPosition(m_pAI->m_pGameObject), m_pAI->GetAimPosition() + CVec3Dfp32::GetMatrixRow(NextMat, 0)),
							Pitch,
							4);
		}
		//Nope, go to path at full speed (or idle max speed, of course)
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
		m_pAI->m_pServer->Object_SetPosition(m_pAI->m_pGameObject->m_iObject, pPath->GetPositionMatrix());		
	}
	else
	{*/
		//Go to position of path next frame
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
		CMat43fp32 NextMat;
		fp32 Time;
		CWObject_Message TimeMsg(OBJMSG_HOOK_GETTIME);
		TimeMsg.m_pData = &Time;
		m_pAI->m_pServer->Message_SendToObject(TimeMsg, m_iPath);
		Time += 0.05f;
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (int)(&Time), (int)(&NextMat), m_pAI->m_pGameObject->m_iObject), m_iPath);
		
		//Get angle in radians
		fp32 Pitch;
		CVec3Dfp32 RelPos = m_pAI->GetAimPosition(m_pAI->m_pGameObject) - (m_pAI->GetAimPosition() + CVec3Dfp32::GetMatrixRow(NextMat, 0));
		
		Pitch = M_ASin(RelPos[2]/RelPos.Length());
		//Convert to fractions
		Pitch /= 2*_PI;

		//Look in the direction of the engine path object
		if(m_ResetLook <= 0)
			m_pAI->OnLook(m_pAI->HeadingToPosition(m_pAI->GetAimPosition(m_pAI->m_pGameObject), m_pAI->GetAimPosition() + CVec3Dfp32::GetMatrixRow(NextMat, 0)),
							Pitch,
							4);
		
		//Move with the velocity necessary to reach the position
		m_pAI->m_pServer->Object_SetPosition(m_pAI->m_pGameObject->m_iObject, CVec3Dfp32::GetMatrixRow(NextMat, 3));		
//		CVec3Dfp32 FlightVel = CVec3Dfp32::GetMatrixRow(NextMat, 3) - m_pAI->m_pGameObject->GetPosition();
//		m_pAI->m_pServer->Object_SetVelocity(m_pAI->m_pGameObject->m_iObject, FlightVel);
//	}
	//Are we at path?
	/*	if (m_bFallenBehind)
	{
	//Nope, go to path at full speed (or idle max speed, of course)
	//Get angle in radians
	fp32 Pitch;
	CVec3Dfp32 RelPos = pPath->GetPosition() - m_pAI->GetAimPosition(m_pAI->m_pGameObject);
		
		Pitch = M_ASin(RelPos[2]/RelPos.Length());
		//Convert to fractions
		Pitch /= 2*_PI;
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);

		if(m_ResetLook <= 0)
			m_pAI->OnLook(m_pAI->HeadingToPosition(m_pAI->GetAimPosition(m_pAI->m_pGameObject), pPath->GetPosition()),
							Pitch,
							4);
		CVec3Dfp32 FlightVel = pPath->GetPosition() - m_pAI->m_pGameObject->GetPosition();
		FlightVel.Normalize();
		FlightVel *= m_pAI->m_IdleMaxSpeed;
		m_pAI->m_DeviceMove.Use(0);
		m_pAI->m_pServer->Object_SetVelocity(m_pAI->m_pGameObject->m_iObject, FlightVel);		
	}
	else
	{
		//Go to position of path next frame
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
		m_TrueFollowTime++;
		CMat4Dfp32 NextMat;
		fp32 Time = m_TrueFollowTime * SERVER_TIMEPERFRAME;
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (int)(&Time), (int)(&NextMat), m_pAI->m_pGameObject->m_iObject), m_iPath);
		

		//Get angle in radians
		fp32 Pitch;
		CVec3Dfp32 RelPos = m_pAI->GetAimPosition(m_pAI->m_pGameObject) - (m_pAI->GetAimPosition() + CVec3Dfp32::GetMatrixRow(NextMat, 0));
		
		Pitch = M_ASin(RelPos[2]/RelPos.Length());
		//Convert to fractions
		Pitch /= 2*_PI;

		//Look in the direction of the engine path object
		if(m_ResetLook <= 0)
			m_pAI->OnLook(m_pAI->HeadingToPosition(m_pAI->GetAimPosition(m_pAI->m_pGameObject), m_pAI->GetAimPosition() + CVec3Dfp32::GetMatrixRow(NextMat, 0)),
							Pitch,
							4);
		
		//Move with the velocity necessary to reach the position
		m_pAI->m_DeviceMove.Use(0);
		CVec3Dfp32 FlightVel = CVec3Dfp32::GetMatrixRow(NextMat, 3) - m_pAI->m_pGameObject->GetPosition();
		m_pAI->m_pServer->Object_SetVelocity(m_pAI->m_pGameObject->m_iObject, FlightVel);		
	}*/

	/*
	//Check that at least move device is available
	if (!m_pAI->m_DeviceMove.IsAvailable())
		//Device unavailable, abort!
		return;

	//Get path object
	CWObject * pPath;
	if (!m_iPath || 
		!(pPath = m_pAI->m_pServer->Object_Get(m_iPath)))
	{
		//Can't get path, stay put and abort!
		m_pAI->OnIdle();
		m_iPath = 0;

		return;
	};

	//Teleport! Stop regular movement though, and make sure path is running.
	m_pAI->m_DeviceMove.Use(0);
	m_pAI->m_pServer->Object_SetVelocity(m_pAI->m_pGameObject->m_iObject, CVec3Dfp32(0));		
 	m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);

	//Look in the direction of the engine path object
	if(m_ResetLook <= 0)
		m_pAI->AddAimAtPositionCmd(m_pAI->GetAimPosition() + CVec3Dfp32::GetMatrixRow(pPath->GetPositionMatrix(), 0));

	//Teleport to paths predicted position
	m_pAI->m_pServer->Object_SetPosition(m_pAI->m_pGameObject->m_iObject, pPath->GetPosition());*/
}

void CAI_Behaviour_Dragon::OnDie()
{
	MAUTOSTRIP(CAI_Behaviour_Dragon_OnDie, MAUTOSTRIP_VOID);
	m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iDiePath);
	CWObject *pDiePos = m_pAI->m_pServer->Object_Get(m_iDiePath);
	if(pDiePos)
	{
		m_pAI->m_pServer->Object_SetPosition(m_pAI->m_pGameObject->m_iObject, pDiePos->GetPositionMatrix());
		if(m_iMode != DYING)
		{
			m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HOOVER));
			m_iMode = DYING;
		}
	}

	//Look in the direction of the engine path object
/*	CWObject *pDiePos = m_pAI->m_pServer->Object_Get(m_iDiePath);
	m_pAI->OnLook(m_pAI->HeadingToPosition(m_pAI->GetAimPosition(m_pAI->m_pGameObject), m_pAI->GetCenterPosition(pDiePos)),
		0,m_iMode == DYING
		40);
	if(m_bAtDiePos)
	{
		m_DieTimer--;
		if(m_DieTimer <= 0)
		{
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iDiePath);
//			m_bAtDiePos = false;
			return;
//			m_pAI->m_pRPGChar->Health() = m_pAI->m_pRPGChar->Health() + 100;
//			m_bOnDieRun = true;
//			m_bOnRun = true;
//			m_iPath = m_iDiePath;
//			m_pAI->m_iMinHealth = m_RemMinHealth;

//		}			
	}
	if(pDiePos)
	{
		CVec3Dfp32 FlightVel = pDiePos->GetPosition() - m_pAI->m_pGameObject->GetPosition();
		int Length = FlightVel.Length();
		if(Length > 12.0f)
		{
			FlightVel[0] /= Length;
			FlightVel[1] /= Length;
			FlightVel[2] /= Length;
			FlightVel *= 12.0f;
		}
		else if(!m_bAtDiePos)
		{
			m_bAtDiePos = true;
			m_iMode = DYING;
			m_DieTimer = 800;
			m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_IMMUNE, 0));
			m_RemMinHealth = m_pAI->m_iMinHealth;
			m_pAI->m_iMinHealth = 0;
		}

		m_pAI->m_DeviceMove.Use(0);
		m_pAI->m_pServer->Object_SetVelocity(m_pAI->m_pGameObject->m_iObject, FlightVel);	
		if(!m_bHooveringSet)
		{			
			m_bHooveringSet = true;
			m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HOOVER));
			m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_IMMUNE, 1));
		}
	}*/
}

//Act according to current mode or just kill anything around
void CAI_Behaviour_Dragon::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Dragon_OnTakeAction, MAUTOSTRIP_VOID);
	//Can't take action if we don't have a valid AI
	if (!m_pAI || !m_pAI->IsValid())
		return;

	m_DamageTimer--;
	m_ResetLook--;
	m_SwitchCounter--;

	//Fly away to drop zone if badly injured and in endfight
	if(!m_bOnRun)
		m_Wait--;
	if (m_pAI->m_pRPGChar->Health() <= m_pAI->m_iMinHealth || m_iMode == DYING)
	{
		OnDie();
		return;
	}

	if(m_Wait <= 0 && !m_bOnRun && m_iAttackRuns.Len() > 0)
	{
		TArray<int> PossibleRuns;
		PossibleRuns.Clear();
		for(int i= 0; i < m_iAttackRuns.Len(); i++)
		{
			if(m_iAttackRuns[i] != m_iPath)
				PossibleRuns.Add(m_iAttackRuns[i]);
		}

		int iRun;
		if(PossibleRuns.Len() > 0)
		{
			iRun = (int)(Random * 0.9999f * PossibleRuns.Len());
			m_iPath = PossibleRuns[iRun];
		}
		else
		{
			iRun = (int)(Random * 0.9999f * m_iAttackRuns.Len());
			m_iPath = m_iAttackRuns[iRun];
		}
//		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iAttackRuns[iRun]);
		m_TrueFollowTime = 0;
		m_bOnRun = true;
		m_bHoldFire = true;
	}
	else if(m_bOnRun)
	{
		CWObject_Character *pTarget;
		if ((pTarget = IsValidTarget(m_iTarget)))
			OnEngage(pTarget);
		else
			FindTarget();
		OnFollowPath();
	}
}


//Handle special messages which influence the behaviour
bool CAI_Behaviour_Dragon::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CAI_Behaviour_Dragon_OnMessage, false);
	switch (_Msg.m_Msg)
	{
	case OBJMSG_SPECIAL_IMPULSE:
		{
			//Check if we should switch to the given behaviour mode (note that I don't support changing variables)
			int iMode = m_iMode;
			CStr str = (char *)_Msg.m_pData;
			CStr s;
			CWObject * pObj;
			if (str.CompareSubStr("PATHS") == 0)
			{
				m_iAttackRuns.Clear();
				//Trim message
				s = str.GetStrSep(";"); 
				//Objects are attack paths to use.
				bool FoundAttackRun = true;
				while(FoundAttackRun)
				{
					s = str.GetStrSep(";"); 
					pObj = m_pAI->GetSingleTarget(0, s, 0);
					if(pObj)
						m_iAttackRuns.Add(pObj->m_iObject);
					else
						FoundAttackRun = false;
				}

			}
			else if (str.CompareSubStr("RUNDONE") == 0)
			{
				if(!m_bOnDieRun)
					m_bOnRun = false;
			}
			else if (str.CompareSubStr("DIERUNDONE") == 0)
			{
				m_bOnDieRun = false;
				m_bOnRun = false;
			}
			else if(str.CompareSubStr("FIREATWILL") == 0)
			{
				m_bHoldFire = false;
			}
			else if (str.CompareSubStr("RANDOMWAIT") == 0)
			{
				s = str.GetStrSep(";"); 
				int Wait = str.GetIntSep(";");
				m_Wait = (int)(Random * Wait * SERVER_TICKSPERSECOND);
			}
			else if (str.CompareSubStr("ANIM_HOOVER") == 0)
			{
				CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
				if(pCD)
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_AIRCRAFT_USELOOK;
				m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_HOOVER));
			}
			else if (str.CompareSubStr("ANIM_FLY") == 0)
			{
				CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
				if(pCD)
					pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_AIRCRAFT_USELOOK;
				m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_FLY));
			}
			return true;
		};
	default:
		return CAI_Behaviour::OnMessage(_Msg);
	};
};


//Always max prio
int CAI_Behaviour_Dragon::GetPathPrio(fp32 _CameraScore)
{
	MAUTOSTRIP(CAI_Behaviour_Dragon_GetPathPrio, 0);
	return 255;
};


