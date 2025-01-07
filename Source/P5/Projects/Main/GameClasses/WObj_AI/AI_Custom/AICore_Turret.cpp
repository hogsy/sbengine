#include "PCH.h"
#include "AICore_Turret.h"
#include "../../../../shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "../../WObj_Game/WObj_GameMessages.h"
#include "../../WObj_CharMsg.h"
#include "../../WObj_Char.h"

CAI_Core_Turret::CAI_Core_Turret()
: CAI_Core()
{
}

//Copy constructor
CAI_Core_Turret::CAI_Core_Turret(CAI_Core* _pAI_Core)
: CAI_Core(_pAI_Core)
{
};


//Sets behaviour from given behaviour type and parameter target
bool CAI_Core_Turret::SetAction(int _iAction, CStr _sParamName, fp32 _DistParam, fp32 _MaxDistParam)
{
	return(false);
};


//Sets behaviour from previously given key values
void CAI_Core_Turret::SetActionFromKeys()
{
};		


//Turrets can only move along rails
int32 CAI_Core_Turret::OnMove(const CVec3Dfp32& _NewDestination, fp32 _Speed,  bool _bFollowPartial, bool _bPerfectPlacement)
{	// We behave as if we are pathfinding
	return MOVE_DEVICE_BUSY;
}


//Get type of AI
int CAI_Core_Turret::GetType()
{
	return TYPE_TURRET;
};

CVec3Dfp32 CAI_Core_Turret::GetHeadPosition()
{	
	return m_pGameObject->AI_GetWeaponPos();;
}


//Handles any registry keys of a character that are AI-related
bool CAI_Core_Turret::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	//Try base class
	return CAI_Core::OnEvalKey(_KeyHash, _pKey);
};

void CAI_Core_Turret::OnTakeAction()
{
#ifndef M_RTM
	DebugDrawKnowledge();
#endif
}

//
//
//
int CAI_Core_Turret::Perception_Sight(int _iObj,bool _bObjectOnly)
{
	if (!m_pGameObject || !m_pServer)
		return CAI_AgentInfo::AWARENESS_INVALID;

	CAI_AgentInfo *pInfo = m_KB.GetAgentInfo(_iObj);
	if ((pInfo) && (!pInfo->IsValid()))
	{
		return CAI_AgentInfo::AWARENESS_INVALID;
	}

	// Get positions
	CMat4Dfp32 LookMatrix;
	GetHeadMat(LookMatrix);

	CVec3Dfp32 ViewerPos,SightPos,ViewerDirection;
	ViewerPos = CVec3Dfp32::GetRow(LookMatrix, 3);
	ViewerDirection = CVec3Dfp32::GetRow(LookMatrix, 0);
	CWObject* pObj = m_pServer->Object_Get(_iObj);
	if (!pObj)
	{
		return CAI_AgentInfo::AWARENESS_INVALID;
	}
	SightPos = pObj->GetPosition();

#ifndef M_RTM
	Debug_RenderWire(ViewerPos,ViewerPos + ViewerDirection * 100,0xff4f4fff,0.5f);
#endif
	// 180* view
	CVec3Dfp32 SightDirection = (SightPos-ViewerPos);
	SightDirection.Normalize();
	
	if (m_FOV < 1.0f)
	{
		if (ViewerDirection*SightDirection < M_Cos(m_FOV * _PI))
		{
			return CAI_AgentInfo::NONE;
		}
	}

	// range check
	fp32 DistSqr = (ViewerPos - SightPos).LengthSqr();
	if (DistSqr > Sqr(m_SightRange))
	{
		if (pInfo)
		{
			pInfo->SetLOS(false);
		}
		return CAI_AgentInfo::NONE;
	}

	// check line of sight
	if ((m_PerceptionFlags & PERC_XRAYVISION_FLAG)||(CheckLOS(_iObj) != CVec3Dfp32(_FP32_MAX)))
	{
		if (pInfo)
		{	// correct position
			pInfo->SetLOS(true);
			pInfo->SetCorrectSuspectedPosition();
		}
	}
	else
	{
		if (pInfo)
		{
			pInfo->SetLOS(false);
		}
		return CAI_AgentInfo::NONE;
	}

	// how sure are we on our target?
	fp32 Factor = GetVisionFactor(_iObj);
	if ((Factor < 1.0f) && (m_PerceptionFlags & PERC_NIGHTVISION_FLAG))
	{
		Factor = 1.0f;
	}

	// abit fuzzy
	if(DistSqr > Sqr(m_SightRange * Factor))
	{
		return CAI_AgentInfo::NOTICED;
	}

	// real close
	if(DistSqr < Sqr(m_SightRange * PERCEPTIONFACTOR_THRESHOLD_SPOT))
	{
		if (pInfo)
		{
			pInfo->SetCorrectSuspectedPosition();
		}
		return CAI_AgentInfo::SPOTTED;
	}

	if (pInfo)
	{
		pInfo->SetCorrectSuspectedPosition();
	}
	return CAI_AgentInfo::NOTICED;
}

void CAI_Core_Turret::OnRefresh(bool _bPassive)
{
	MSCOPE(CAI_Core_Turret::OnRefresh, CHARACTER);

	if(!m_pGameObject || !m_pServer)
		//Lacking either game object or server, no point refreshing
		return;

	if (!m_pAIResources)
	{
		m_pAIResources = GetAIResources();
		if (!m_pAIResources)
		{	// Lacking global resources we bail
			return;
		}
	}

	const int PhysType = CWObject_Character::Char_GetPhysType(m_pGameObject);

	//Set some "character" data 
	m_bWeightless = (m_pGameObject->m_ClientFlags & PLAYER_CLIENTFLAGS_WEIGHTLESS) != 0;
	//Reset activity
	m_ActivityScore = CAI_ActivityCounter::INACTIVE;

	//Don't do anything if the AI is invalid
	// Store old matrices and get new ones
	OnRefreshPosMats();
	RefreshTicks();

	if (!IsValid(true))
	{	//Did we just die?
		if  ((m_bWasAlive)&&(m_pGameObject))
		{
			OnDie();
		};

		//Did we just become invalid?
		if (m_bWasValid)
		{//Destroy and initialize the character 
			Destroy();
			Init(m_pGameObject, m_pServer); //Init sets m_bWasValid to false...
		};

#ifndef M_RTM
		DebugDrawLight();
#endif
		return;
	};

		//StopBM();

		//Is this the first refresh, then do some "enter game" stuff
		if (m_bFirstRefresh)
		{
			//Initialize knowledge base
			m_KB.Init();

			//Raise OnSpawn event
			m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPAWN,0);
			m_bWasAlive = true;
			m_bWasValid = true;

			//Send a "I'm aliiive!" message to all characters in char list
			SendMessage(MSGMODE_ALL, CWObject_Message(OBJMSG_IM_ALIIIVE, 0, 0, m_pGameObject->m_iObject));
			m_pAIResources->ms_Gamestyle = m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETGAMESTYLE), m_pServer->Game_GetObjectIndex());
			m_pAIResources->ms_GameDifficulty = m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETDIFFICULTYLEVEL), m_pServer->Game_GetObjectIndex());
			if (m_pAIResources->ms_GameDifficulty == DIFFICULTYLEVEL_HARD)
			{
				m_DeviceWeapon.SetPeriod(5);
			}
			else if (m_pAIResources->ms_GameDifficulty == DIFFICULTYLEVEL_NORMAL)
			{
				m_DeviceWeapon.SetPeriod(10);
			}
			else	// Easy and Commentary
			{
				m_DeviceWeapon.SetPeriod(20);
			}
			m_DeviceSound.SetupDialogueIndices();
		};

		bool bIsBraindead = false;
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
		if ((pCD)&&(pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_ISSLEEPING))||(!m_pGameObject->AI_IsAlive()))
		{
			bIsBraindead = true;
		}

		if (m_StunTimeout > 0)
		{
			m_StunTimeout--;
		}
		/*
		if ((bIsBraindead)&&(m_StunTimeout >= 0))
		{
			m_StunTimeout = 1;
		}
		*/

		//The AI is ready for (or in the middle of) action!
		//Refresh knowledge base (only every 10th frame for passive refresh)
		if ((IsActive())&&(IsAlive(PhysType)))
		{
			m_CurPriorityClass = CAI_Action::PRIO_MIN;
			if ((m_Timer % AI_KB_REFRESH_PERIOD == 0)||(m_bFirstRefresh))
			{
				if (!_bPassive)// Not the player
				{
					if ((!bIsBraindead)&&(!m_StunTimeout))
					{
						m_KB.OnRefresh();
					}
					else
					{
						// Set awareness of player to no more than DETECTED (ie not updating with player moves)
						CAI_AgentInfo* pPlayerInfo = m_KB.GetAgentInfo(GetClosestPlayer());
						if ((pPlayerInfo)&&(pPlayerInfo->GetCurAwareness() >= CAI_AgentInfo::NOTICED))
						{
							pPlayerInfo->SetAwareness(CAI_AgentInfo::NOTICED,false,true);	
						}
					}
				}
			}
			else if ((m_UseLightmeter > -1)&&((m_Timer+2) % 5 == 0))
			{
				if (m_UseLightmeter == 2)
				{	// Full light measurers every 2 seconds
					if ((m_Timer+2) % 40 == 0)
					{
						MeasureLightIntensity(true);
						if (m_bFlashlight)
						{
							HandleFlashlight(40);
						}
					}
				}
				else if (m_UseLightmeter == 1)
				{	// Basic light measurers every second
					if ((m_Timer+2) % 20 == 0)
					{
						MeasureLightIntensity(false);
						if (m_bFlashlight)
						{
							HandleFlashlight(20);
						}
					}
				}

				if (m_UseLightmeter >= 0)
				{	// Detect broken lights every 2 seconds or so
					if ((m_Timer+2) % 40 == 0)
					{
						HandleBrokenLights(40);
					}
				}
			}

			// Draw lightmeter
#ifndef M_RTM
			DebugDrawLight();
#endif
			// Don't do this at the same time as m_KB.OnRefresh() above
			if ((m_Timer + 7) % 20 == 0)
			{
				GetStealthTension(true);
			}
		}

		//The below stuff is for non-passive bots only
		if (!_bPassive)
		{
			if (!IsPlayer())
			{
				OnActivityCheck();
				//Should we activate inactive bot? Only check for activation every ten frames.
				if (OnActivationCheck())
					//Check if we perhaps should deactivate active bot
					OnDeactivationCheck();
			}

			//Refresh devices
			for (int i = 0; i < CAI_Device::NUM_DEVICE; i++)
				m_lDevices[i]->OnRefresh();
			m_DeviceMove.Lock();

			//Lock devices that should be unavailable 
			if (m_pGameObject->AI_GetPhysFlags() & PLAYER_PHYSFLAGS_IMMOBILE)
				m_DeviceMove.Lock();

			//Refresh item handlers
			if (!IsPlayer())
			{
				m_EventHandler.OnRefresh();
				OnTakeAction();

				// Maybe we should raise the alarm now (if we're still alive)
				if ((m_PendingAlarmTime > -1)&&
					(IsAlive(PhysType))&&
					(m_KB.GetAlertness() >= CAI_KnowledgeBase::ALERTNESS_DROWSY)&&
					(GetAITick() >= m_PendingAlarmTime))
				{
					m_KB.AddLoudness(100);	// We're screamin some kind of alarm - right!
					SendMessage(CAI_Core::MSGMODE_ALARM,m_PendingAlarmMsg);
					if (m_PendingAlarmMsg.m_Msg == OBJMSG_TEAM_ALARM)
					{
						m_EventHandler.RaiseEvent(CAI_EventHandler::ON_ALARM,m_PendingAlarmMsg.m_Param0);
					}

					m_PendingAlarmMsg.m_Msg = 0; 
					m_PendingAlarmTime = -1;
				}
			}

			//Activity is at least idle
			SetActivityScore(CAI_ActivityCounter::IDLE);
		}

		m_bFirstRefresh = false;

		//Destroy timer stuff
		if (m_DestroyTimer == 0)
		{
			m_EventHandler.RaiseEvent(CAI_EventHandler::ON_DIE,0);
			Destroy();
			m_pServer->Object_Destroy(m_pGameObject->m_iObject);
		}
		else if (m_DestroyTimer > 0)
			m_DestroyTimer--;

		//Life timer stuff
		if ((m_LifeTimer >= 0)&&(m_LifeDrainAmount != 0))
		{
			if (m_LifeTimer == 0)
			{
				//Lose (or gain) some health at specified intervals
				if (m_Timer % Max(1, m_LifeDrainInterval) == 0)
				{
					if (m_LifeDrainAmount > 0)
					{
						//Apply damage and die if necessary
						m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_APPLYDAMAGE, m_LifeDrainAmount), m_pGameObject->m_iObject);
						if (Health(m_pGameObject->m_iObject) <= 0)
							m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_PHYSICS_KILL, DAMAGETYPE_UNDEFINED), m_pGameObject->m_iObject);
					}
					else
						m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH, -m_LifeDrainAmount), m_pGameObject->m_iObject);
				};
			}
			else
				m_LifeTimer--; 
		}


		//Report activity score if not player
		if (!IsPlayer(m_pGameObject->m_iObject))
			m_pAIResources->ms_ActivityCounter.Report(m_ActivityScore, m_pAIResources->GetAITick());
}

