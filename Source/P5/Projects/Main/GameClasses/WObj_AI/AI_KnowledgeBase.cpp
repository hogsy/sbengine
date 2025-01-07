#include "PCH.h"
#include "AI_KnowledgeBase.h"
#include "AICore.h"
#include "../WRPG/WRPGChar.h"
#include "../WObj_CharMsg.h"
#include "WObj_Aux/WObj_Team.h"
#include "../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Game.h"

//String translator definitions
const char * CAI_AgentInfo::ms_TranslateRelation[] =
{
	"unknown", "friendly", "neutral", "unfriendly", "hostile", "hostile_1", "hostile_2", "enemy", NULL
};

const char * CAI_AgentInfo::ms_TranslateAwareness[] =
{
	"none", "noticed", "detected", "spotted", NULL
};

const char * CAI_KnowledgeBase::ms_TranslateAlertness[] =
{
	"oblivious", "sleeping", "drowsy", "idle", "watchful", "jumpy", NULL
};



//CAI_AgentInfo///////////////////////////////////////////////////////////////////////////////////////

//Constructors
CAI_AgentInfo::CAI_AgentInfo(int _iObject, CAI_KnowledgeBase * _pKB)
{
	m_pKB = _pKB;
	Init(_iObject);
}

//Initializer (you don't have to call this on newly constructed objects, only on invalid ones you want to reinitialize)
void CAI_AgentInfo::Init(int _iObject)
{
	m_iObject = _iObject;

	m_ValidFlags = INFOFLAGS_NONE;

	m_LastSpottedTimer = 0;
	m_AwarenessTimer = -1;
	m_Awareness = AWARENESS_INVALID;
	m_PendingAwareness = AWARENESS_INVALID;
	INVALIDATE_POS(m_PendingSuspectedPos);

	m_AwarenessOfUs = AWARENESS_INVALID;

	m_RelationTimer = 0;
	m_Relation = RELATION_INVALID;
	m_PendingRelation = RELATION_INVALID;

	m_SuspectedHeight = 32.0f;	// 1.0 meter
	INVALIDATE_POS(m_SuspectedPos);
	INVALIDATE_POS(m_SuspectedPosNotGround);

	m_bPosUpdate = false;
};

//The interval in frames during which awareness of something cannot be raised, 
//based on alertness level and reaction attribute
int CAI_AgentInfo::Alertness_AwarenessChangeDelay(int _Awareness)
{
	CAI_Core* pAI = m_pKB->GetAI();

	if (!pAI)
		return 0;

	if (_Awareness > m_Awareness)
	{	// Improve awareness delay
		switch (m_pKB->GetAlertness())
		{
		case CAI_KnowledgeBase::ALERTNESS_OBLIVIOUS:
			return(100 * pAI->m_Reaction);
			break;

		case CAI_KnowledgeBase::ALERTNESS_SLEEPING:
			return(50 * pAI->m_Reaction);
			break;

		case CAI_KnowledgeBase::ALERTNESS_DROWSY:
			return(10 * pAI->m_Reaction);
			break;

		case CAI_KnowledgeBase::ALERTNESS_IDLE:
			if (_Awareness < CAI_AgentInfo::SPOTTED)
				return(3 * pAI->m_Reaction);
			else
				return(pAI->m_Reaction);
			break;

		default:
			return(pAI->m_Reaction);
		}
	}
	else if (_Awareness < m_Awareness)
	{	// Awareness reduction
		// If awareness was SPOTTED we should very fast go down to DETECTED to indicate that we no longer see the bastard
		if (m_Awareness == SPOTTED)
		{
			//By default we still have a "magical" awareness of agent for 5 * reaction time for some reason.
			//This can be set to 0 by using the unspotfast flag though 
			if (pAI->m_UseFlags & CAI_Core::USE_UNSPOTFAST)
				return 0;
			else
				return(5 * pAI->m_Reaction);
		}

		int Tension = pAI->GetStealthTension();
		switch(Tension)
		{
		case CAI_Core::TENSION_NONE:
			return(pAI->m_Patience);
			break;

		case CAI_Core::TENSION_HOSTILE:
			return(2 * pAI->m_Patience);
			break;

		case CAI_Core::TENSION_COMBAT:
			return(5 * pAI->m_Patience);
			break;

		default:
			return(pAI->m_Patience);
			break;
		}
	}

	return(0);
};

//Get new awareness of agent based on given "wanted awareness" and own alertness level
int CAI_AgentInfo::Alertness_AwarenessMod(int _Awareness)
{
	if (!m_pKB || !m_pKB->GetAI() || !m_pKB->GetAI()->m_pGameObject || !GetObject())
		return CAI_KnowledgeBase::ALERTNESS_INVALID;

	switch (m_pKB->GetAlertness())
	{
	case CAI_KnowledgeBase::ALERTNESS_OBLIVIOUS:
		{	//No awareness possible
			return NONE;
		}
	case CAI_KnowledgeBase::ALERTNESS_SLEEPING:
		{	//Can only notice stuff
			return Min(_Awareness, (int)NOTICED);
		}
	case CAI_KnowledgeBase::ALERTNESS_DROWSY:
		{
			//No restriction if within alertness range
			if (m_pKB->GetAI()->m_pGameObject->GetPosition().DistanceSqr(GetObject()->GetPosition()) < m_pKB->GetAlertnessRangeSqr())
				return _Awareness;
			//Can only raise awareness above noticed level one level at a time
			else if (_Awareness > NOTICED)
				return Min(_Awareness, m_Awareness + 1);
			else
				return _Awareness;
		}
	case CAI_KnowledgeBase::ALERTNESS_IDLE:
		{
			//No restriction if within alertness range
			if (m_pKB->GetAI()->m_pGameObject->GetPosition().DistanceSqr(GetObject()->GetPosition()) < m_pKB->GetAlertnessRangeSqr())
				return _Awareness;
			//Can only raise awareness above detected level one level at a time
			else if (_Awareness <= DETECTED)
				return Min(_Awareness, m_Awareness + 1);
			else
				return _Awareness;
		}
	default:
		{
			//For higher (or invalid) alertness, we can always attain any awareness immediately
			return _Awareness;
		}
	}
};

void CAI_AgentInfo::SetLOS(bool _InLOS)
{
	m_ValidFlags |= CAI_AgentInfo::IN_LOS;
	if (_InLOS)
	{
		m_InfoFlags |= IN_LOS;
	}
	else
	{
		m_InfoFlags &= ~IN_LOS;
	}
}

//Update and get LOS flag
bool CAI_AgentInfo::InLOS(CVec3Dfp32 * _pLOSPos)
{
	if (!m_pKB || !m_pKB->GetAI())
		return false;

	//We think someone is in los when spotted 
	//(actually the LOS flag is kinda useless with the current awareness system)
	if ((m_InfoFlags & IN_LOS) || (m_Awareness >= SPOTTED))
	{
		if (_pLOSPos)
		{
			*_pLOSPos = GetBasePos();
		}
		return(true);
	}
	else
	{
		if (_pLOSPos)
		{
			*_pLOSPos = GetBasePos();
		}
		return(false);
	}
};

bool CAI_AgentInfo::DoesMeleeBlow(int _Tick)
{
	CAI_Core* pAgentAI = m_pKB->GetAI()->GetAI(m_iObject);
	if (pAgentAI)
	{
		if (m_MeleeTick + 2 * AI_KB_REFRESH_PERIOD >= _Tick)
		{
 			m_pKB->GetAI()->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_FIGHT,m_iObject);
			return(true);
		}
		else
		{
			m_MeleeTick = pAgentAI->m_LastMeleeAttackTick;
			return(false);
		}
	}
	return(false);
};


void CAI_AgentInfo::ClearMeleeBlow()
{
	// Set to fairly high negative value to avoid tripping directly after start
	m_MeleeTick = - 10 * AI_KB_REFRESH_PERIOD;
};

//Update and get position-LOS flag
bool CAI_AgentInfo::PositionInLOS()
{
	if (!m_pKB || !m_pKB->GetAI() || !GetObject())
		return false;

	CAI_Core* pAI = m_pKB->GetAI();

	if (!(m_ValidFlags & POSITION_IN_LOS))
	{
		m_ValidFlags |= POSITION_IN_LOS;

		//Info invalid, check for LOS to agent position offset to torso height
		CVec3Dfp32 Aim = pAI->GetHeadPos();
		CVec3Dfp32 Target = pAI->GetHeadPos(GetObject());

		bool bHit = false;
		CCollisionInfo Info;

		if (INVALID_POS(Target))
			bHit = true;
		else
			bHit = m_pKB->GetAI()->IntersectLOS(Aim, Target, m_pKB->GetAI()->m_pGameObject->m_iObject, &Info);


		//LOS if no hit or hit agent
		if (!bHit || (Info.m_bIsValid && (Info.m_iObject == m_iObject)))
		{
			//In LOS
			m_InfoFlags |= POSITION_IN_LOS;
		}
		else
		{
			//Not in LOS
			m_InfoFlags &= ~POSITION_IN_LOS;		
		}
	}

	//Check flag
	return (m_InfoFlags & IN_LOS) != 0;
};



//Refreshes the state of the character info, and decides when state info becomes invalid
void CAI_AgentInfo::OnRefresh()
{
	MSCOPESHORT(CAI_AgentInfo::OnRefresh);
	if ((m_iObject == 0) || !m_pKB)
		return;
	
	CAI_Core *pAI = m_pKB->GetAI();

	if(!pAI || !pAI->IsValid())
		return;

	//If the character has just become invalid, invalidate the info
	if (!pAI->IsValidAgent(m_iObject)) 
	{	//Empty the info
		if (!pAI->IsPlayer(m_iObject))
		{
			Init();
		}
		else
		{
			m_Awareness = NONE;
			m_PendingAwareness = AWARENESS_INVALID;
		}
		return;
	}
	

	{	// Refresh awareness
		GetAwareness(true);

		// Refresh relation
		int Relation = GetRelation(true);

		//Awareness can only be changed to a higher value after a reaction time delay. 
		if (m_PendingAwareness != AWARENESS_INVALID)
		{
			bool ChangeAwareness = false;
			if ((m_PendingAwareness > m_Awareness)&&
				(pAI->m_Timer >= m_AwarenessTimer + Alertness_AwarenessChangeDelay(m_PendingAwareness)))
			{
				// Improve awareness
				ChangeAwareness = true;

				bool bNewSpotted = (m_Awareness < SPOTTED) && (m_PendingAwareness >= SPOTTED);
				bool bNewDetected = (m_Awareness < DETECTED) && (m_PendingAwareness >= DETECTED);

				//Check if events should be raised
				if (bNewSpotted)
				{
					//Remove any phenomena related to this agent
					if (Relation >= ENEMY)
					{
						//Raise spotting event (lower level awareness events are triggered when noticing their respective phenomena)
						pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_ENEMY, m_iObject);
					}
					else if (Relation >= HOSTILE)
					{
						//Raise spotting event (lower level awareness events are triggered when noticing their respective phenomena)
						pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_HOSTILE, m_iObject);
					}

					// We (re)spotted the player
					if (pAI->IsPlayer(m_iObject))
					{
						pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_PLAYER, m_iObject);
						pAI->UseRandom("(re)SPOTTED",CAI_Device_Sound::COMBAT_SPOTTED,CAI_Action::PRIO_COMBAT);
					}

					// *** DEBUG ***
#ifndef M_RTM
					// *** Something changed
					if ((pAI->DebugRender())&&
						(pAI->m_pServer))
					{
						// Draw a line of the appropriate color from our head to the target
						CVec3Dfp32 headPos = pAI->GetLookPos();
						CVec3Dfp32 suspectedPos = GetBasePos();
						int32 color = 0;
						switch(m_PendingAwareness)
						{
						case CAI_AgentInfo::NOTICED:
							color = 0xff00ff00;	// Green
							break;
						case CAI_AgentInfo::DETECTED:
							color = 0xffffff00;	// Yellow
							break;
						case CAI_AgentInfo::SPOTTED:
							color = 0xffff0000;	// Red
							break;
						}
						if (color)
						{
							pAI->Debug_RenderWire(headPos,suspectedPos,color,1.0f);
						}
					}
#endif
					// *** DEBUG ***
				}
				else if (bNewDetected)
				{
					if (Relation >= ENEMY)
					{
						//Raise spotting event (lower level awareness events are triggered when noticing their respective phenomena)
						pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_DETECT_DISTURBANCE, m_iObject);

						// *** DEBUG ***
						// *** Something changed
						if ((pAI->DebugRender())&&
							(pAI->m_pServer))
						{
							// Draw a line of the appropriate color from our head to the target
							CVec3Dfp32 headPos = pAI->GetAimPosition(pAI->m_pGameObject);
							CVec3Dfp32 suspectedPos = GetBasePos();
							int32 color = 0;
							switch(m_PendingAwareness)
							{
							case CAI_AgentInfo::NOTICED:
								color = 0xff00ff00;	// Green
								break;
							case CAI_AgentInfo::DETECTED:
								color = 0xffffff00;	// Yellow
								break;
							case CAI_AgentInfo::SPOTTED:
								color = 0xffff0000;	// Red
								break;
							}
							if (color)
							{
								pAI->Debug_RenderWire(headPos,suspectedPos,color,1.0f);
							}
						}
						// *** DEBUG ***
					}
				}
			}
			else if ((m_PendingAwareness < m_Awareness)&&
				(pAI->m_Timer >= m_AwarenessTimer + Alertness_AwarenessChangeDelay(m_PendingAwareness)))
			{	// Reduce awareness
				ChangeAwareness = true;
			}

			// Set new awareness
			if (ChangeAwareness)
			{	// Update timer and members
				m_AwarenessTimer = pAI->m_Timer;
				m_Awareness = m_PendingAwareness;
				if (m_PendingSuspectedPos != CVec3Dfp32(_FP32_MAX))
				{
					SetSuspectedPosition(m_PendingSuspectedPos);
				}
				m_PendingAwareness = AWARENESS_INVALID;
				INVALIDATE_POS(m_PendingSuspectedPos);
			}
		}

		//Reset stuff
		m_bPosUpdate = false;
	};
};


//Checks if the character info is valid. An invalid character info can be reinitialized at will.
bool CAI_AgentInfo::IsValid()
{
	return m_iObject != 0;
};


//Agent info are considered equal if they have the same world object
bool CAI_AgentInfo::operator==(const CAI_AgentInfo& _Compare)
{
	return m_iObject == _Compare.m_iObject;
}; 


//Check if agent fulfills the given criteria
bool CAI_AgentInfo::Check(const CAI_AgentCriteria& _Criteria)
{
	if (!m_pKB || !m_pKB->GetAI())
		return false;

	//For the sake of performance, it's better to make the cheapest checks first, of course

	if (_Criteria.m_CheckPos != CVec3Dfp32(_FP32_MAX))
	{
		//Get distance to pos
		fp32 DistSqr;
		if (_Criteria.m_bXYDistOnly)
			DistSqr = CAI_Core::XYDistanceSqr(_Criteria.m_CheckPos, GetBasePos());
		else
			DistSqr = GetBasePos().DistanceSqr(_Criteria.m_CheckPos);
		
		if ((DistSqr > _Criteria.m_MaxDistanceSqr) ||
			(DistSqr < _Criteria.m_MinDistanceSqr))
			return false;
	}

	if (_Criteria.m_InfoFlagsTrue)
	{
		//Check info flags that must be set 
		if (!CheckInfoFlags(_Criteria.m_InfoFlagsTrue))
			return false;
	}

	if (_Criteria.m_InfoFlagsFalse)
	{
		//Check info flags that cannot be set 
		if (CheckInfoFlags(_Criteria.m_InfoFlagsFalse))
			return false;
	}

	if (_Criteria.m_MinAwareness != AWARENESS_INVALID)
	{
		//Check minimum awareness
		if (GetAwareness() < _Criteria.m_MinAwareness)
			return false;
	}

	if (_Criteria.m_MaxAwareness != AWARENESS_INVALID)
	{
		//Check maximum awareness
		if (GetAwareness() > _Criteria.m_MaxAwareness)
			return false;
	}

	if (_Criteria.m_MinAwarenessOfUs != AWARENESS_INVALID)
	{
		//Check minimum awareness of us
		if (GetAwarenessOfUs() < _Criteria.m_MinAwarenessOfUs)
			return false;
	}

	if (_Criteria.m_MaxAwarenessOfUs != AWARENESS_INVALID)
	{
		//Check maximum awareness of us
		if (GetAwarenessOfUs() > _Criteria.m_MaxAwarenessOfUs)
			return false;
	}
	
	if (_Criteria.m_MinRelation != RELATION_INVALID)
	{
		//Check minimum relation
		if (GetCurRelation() < _Criteria.m_MinRelation)
			return false;
	}

	if (_Criteria.m_MaxRelation != RELATION_INVALID)
	{
		//Check minimum relation
		if (GetCurRelation() > _Criteria.m_MaxRelation)
			return false;
	}
	
	//All criteria are fulfilled!
	return true;
};


//Update the given info flags and return the updated info flags value
bool CAI_AgentInfo::CheckInfoFlags(int _UpdateFlags)
{
	//Check cheapest info flags first

	if (_UpdateFlags & IN_LOS)
	{
		if (!InLOS())
			return false;
	}

	if (_UpdateFlags & POSITION_IN_LOS)
	{
		if (!PositionInLOS())
			return false;
	}

	//All flags to check are set
	return true;
};

// Returns current awareness without updating
int32 CAI_AgentInfo::GetCurAwareness()
{
	return(m_Awareness);
};

int32 CAI_AgentInfo::GetCurAwarenessTimer()
{
	return(m_AwarenessTimer);
};

int32 CAI_AgentInfo::GetGetLastSpottedTicks()
{
	CAI_Core* pAI = m_pKB->GetAI();
	if ((pAI)&&(m_LastSpottedTimer))
	{
		return (pAI->m_Timer - m_LastSpottedTimer);
	}
	else
	{	// No AI? (No spot for an hour)
		return(108000);
	}
};

//Update and get awareness 
int CAI_AgentInfo::GetAwareness(bool _bUpdateNow)
{
	MSCOPESHORT(CAI_AgentInfo::GetAwareness);
	if (!m_pKB)
	{
		return AWARENESS_INVALID;
	}

	CAI_Core* pAI = m_pKB->GetAI();
	if (!pAI)
	{
		return AWARENESS_INVALID;
	}

	if (!pAI->m_pServer)
	{
		return AWARENESS_INVALID;
	}

	CAI_Core* pAgentAI = pAI->GetAI(GetObjectID());
	if ((pAgentAI)&&(pAgentAI->m_pGameObject))
	{
		if (CWObject_Character::Char_GetPhysType(pAgentAI->m_pGameObject) == PLAYER_PHYS_NOCLIP)
		{
			m_Awareness = NONE;
			return NONE;
		}
	}
	

	// Don't update awareness of friends unless they're dead or we haven't seen each other in a while
	if (m_Relation == FRIENDLY)
	{
		if (GetGetLastSpottedTicks() < pAI->GetAITicksPerSecond() * 3)
		{
			return m_Awareness;
		}
	}

	if (pAI->m_PauseActions)
	{
		return m_Awareness;
	}

	// We always update pos as long as target is SPOTTED
	if (m_Awareness >= SPOTTED)
	{
		SetCorrectSuspectedPosition();
	}

	//Hearing is only properly checked every few frames. Therefore, the highest noise score the agent has
	//had in the interval since the last check was made is used for determining if it is heard.
	fp32 Noise = m_pKB->GetNoise();

	//Should we update info?
	if (_bUpdateNow)
	{
		m_ValidFlags |= AWARENESS;

		//Check perception
		int Awareness,Vision,Hearing;

		//Check misc. awareness senses
		Awareness = pAI->Perception_Awareness(m_iObject);

		if (Awareness >= SPOTTED)
		{	//Spotted by awareness. Position is automatically known.
			SetCorrectSuspectedPosition();
		}
		else
		{	//Check sight
			Vision = pAI->Perception_Sight(m_iObject);
			Awareness = Max(Awareness,Vision);
			//Check hearing only when needed, saves some tracelines
			Hearing = pAI->Perception_Hearing(m_iObject,(int)Noise,false);
			Awareness = Max(Awareness,Hearing);
			// Update suspected pos
			if ((Awareness < SPOTTED)&&(Hearing == NOTICED))
			{
				SetCorrectSuspectedPosition();
			}
		}

		if (Awareness >= SPOTTED)
		{
			m_LastSpottedTimer = pAI->m_Timer;
		}

		//Update awareness if this awareness is higher than previous one
		if (m_Awareness < Awareness)
		{
			//There might be some restrictions on awareness increases by normal perception 
			SetAwareness(Alertness_AwarenessMod(Awareness),true,true);
		}
		else if (m_Awareness > Awareness)
		{	// We don't tell friends when awareness goes down
			SetAwareness(Awareness,false);
		}
		else	// Awareness == m_Awareness
		{	// Update timing
			Touch();
			if ((GetCurRelation() >= ENEMY)&&(Awareness >= DETECTED)&&(Random >= 0.9f))
			{
				pAI->RaiseAwareness(m_iObject,GetSuspectedPosition(),DETECTED,GetCurRelation());
			}
		}
	}

	return m_Awareness;
};


//Update and get awareness of us
int CAI_AgentInfo::GetAwarenessOfUs()
{
	if (!m_pKB || !m_pKB->GetAI() || !m_pKB->GetAI()->m_pGameObject || !m_pKB->GetAI()->m_pServer)
		return AWARENESS_INVALID;

	//If spotted, we always assume we're still spotted by this agent, 
	//otherwise update awareness of us every few frames
	if ((m_AwarenessOfUs < SPOTTED) &&
		(!(m_ValidFlags & AWARENESS_OF_US) || (m_AwarenessOfUs == AWARENESS_INVALID)))
	{
		m_ValidFlags |= AWARENESS_OF_US;

		if (m_pKB->GetAI()->IsBot(m_iObject))
		{
			//Bot. Check agents own AI
			CAI_Core * pAI = m_pKB->GetAI()->GetAI(m_iObject);
			if (pAI)
			{
				//Get agents info about us
				CAI_AgentInfo * pInfo = pAI->m_KB.GetAgentInfo(m_pKB->GetAI()->m_pGameObject->m_iObject);
				if (pInfo)
				{
					m_AwarenessOfUs = pInfo->GetAwareness();	
				}
			}
		}
		else
		{
			//Agent is a human player. Assume he has spotted us if:
			// - We're within 100 units OR
			// - We're in LOS and within the characters front 110 degrees arc
			//Otherwise assume not noticed at all
			CWObject * pAgent = m_pKB->GetAI()->m_pServer->Object_Get(m_iObject);
			if (pAgent)
			{
				if ((m_pKB->GetAI()->m_pGameObject->GetPosition()).DistanceSqr(pAgent->GetPosition()) < 100*100)
				{
					m_AwarenessOfUs = SPOTTED;
				}
				else 
				{
					//Get characters look-direction
					fp32 AngleToUs = m_pKB->GetAI()->HeadingToPosition(pAgent->GetPosition(), m_pKB->GetAI()->m_pGameObject->GetPosition(), 0.5f, m_pKB->GetAI()->GetHeading(pAgent)); 
					if ( (M_Fabs(AngleToUs) < 0.15f) && //In FOV? 
						 InLOS() )					//In LOS? (Assume character has LOS if we do)
					{
						m_AwarenessOfUs = SPOTTED;
					}
				}
			}
		}
	}

	return m_AwarenessOfUs;
};


//Update and get relation
int CAI_AgentInfo::GetRelation(bool _bUpdateNow)
{
	if ((_bUpdateNow)||(m_Relation <= UNKNOWN))
	{
		if (!m_pKB)
		{
			return(RELATION_INVALID);
		}
		CAI_Core* pAI = m_pKB->GetAI();
		if ((!pAI)||(!pAI->m_pServer))
		{
			return(RELATION_INVALID);
		}

		m_ValidFlags |= RELATION;	// *** Not needed? ***
		if ((m_PendingRelation != RELATION_INVALID)&&(pAI->m_Timer > m_RelationTimer))
		{
			if (m_PendingRelation < m_Relation)
			{
				pAI->m_StealthTensionTime = 0;
			}
			SetRelation(m_PendingRelation,0);
			m_PendingRelation = RELATION_INVALID;
		}


		CWObject* pObj = GetObject();
		if (!pObj)
		{
			m_Relation = UNKNOWN;
			m_PendingRelation = RELATION_INVALID;
		}
		//Relation do not change implicitly unless we are aware of agent 
		else if (GetCurAwareness() >= pAI->m_RelationChangeAwareness)
		{
			//Relation transition rules
			int NewRelation = m_Relation;
			if (m_Relation <= UNKNOWN)
			{
				//We can figure out relation for first time. Set to default.
				NewRelation = m_pKB->DefaultRelation(m_iObject);
			}
			else if (m_Relation == FRIENDLY)
			{
				//Currently, we're friends forever and ever!
			}
			else if (m_Relation < ENEMY)
			{	
				bool bCanLessen = true;		

				//Relation might get worsened if we're security sensitive and agent is
				//violating security level (i.e. is somewhere he doesn't have clearance 
				//to be or is fighting or carrying weapons etc.)
				if ((NewRelation < ENEMY) && (pAI->m_SecurityTolerance >= 0))
				{
					int SecurityLevel = m_pKB->GetSecurityLevel(m_iObject);
					if (SecurityLevel > 0)
					{
						//Secure area, reduce level by agent's clearance
						SecurityLevel -= pAI->GetSecurityClearance(m_iObject);
						if (SecurityLevel > pAI->m_SecurityTolerance)
						{
							//This guy is really offlimits. Take him down!
							NewRelation = ENEMY;
						}
						else if ((NewRelation < HOSTILE_1) && (SecurityLevel > pAI->m_SecurityTolerance / 2))
						{
							//This guy should not be here. Give warning 
							NewRelation = HOSTILE_1;
						}
						else if ((NewRelation < HOSTILE) && (SecurityLevel > 0))
						{
							//This guy should not be here. Keep an eye on him.
							NewRelation = HOSTILE;
						}

						//Can't relax while agent breaches security
						if (SecurityLevel > 0)
							bCanLessen = false;	
					}
				}

				// You haven't done anything bad for 3 patiences (ca 15s typically)
				if ((bCanLessen)&&(m_PendingRelation == RELATION_INVALID)&&(m_Relation > UNFRIENDLY))
				{
					if (pAI->m_CharacterClass == CAI_Core::CLASS_TOUGHGUY)
					{	// 5 secs
						SetRelation(m_Relation - 1, (int)(5.0f * pAI->GetAITicksPerSecond()));
						NewRelation = RELATION_INVALID;
					}
					else if (pAI->m_CharacterClass == CAI_Core::CLASS_CIV)
					{	// 10 secs
						SetRelation(m_Relation - 1, (int)(10.0f * pAI->GetAITicksPerSecond()));
						NewRelation = RELATION_INVALID;
					}
					else // Badguys etc
					{	// 5 secs, quick to anger, quick to forgiveness
						SetRelation(m_Relation - 1, (int)(5.0f * pAI->GetAITicksPerSecond()));
						NewRelation = RELATION_INVALID;
					}
				}
			}
			else	// m_Relation >= ENEMY
			{
				if (m_PendingRelation == RELATION_INVALID)
				{	// CLASS_TOUGHGUY lighten up fairly quick
					if (pAI->m_CharacterClass == CAI_Core::CLASS_TOUGHGUY)
					{
						SetRelation(m_Relation - 1, (int)(5.0f * pAI->GetAITicksPerSecond()));
						NewRelation = RELATION_INVALID;
					}
					else if (pAI->m_CharacterClass == CAI_Core::CLASS_CIV)
					{	// CLASS_CIV take some time to thaw out
						SetRelation(m_Relation - 1, (int)(10.0f * pAI->GetAITicksPerSecond()));
						NewRelation = RELATION_INVALID;
					}
					// Nobody else ever forgives...
				}
			}

			//Set relation
			if ((NewRelation != RELATION_INVALID)&&(NewRelation != m_Relation))
			{
				SetRelation(NewRelation);
			}
		}
	}

	return m_Relation;
};

int CAI_AgentInfo::GetCurRelation()
{
	return m_Relation;
}

int CAI_AgentInfo::GetCurOrPendingRelation()
{
	if (m_PendingRelation != RELATION_INVALID)
	{
		return m_PendingRelation;
	}
	else
	{
		return m_Relation;
	}
}

//Update and get position (or suspected position if there's uncertainty)
CVec3Dfp32 CAI_AgentInfo::GetBasePos()
{
	if (!m_pKB || !m_pKB->GetAI() || !m_pKB->GetAI()->m_pGameObject || !GetObject())
		return CVec3Dfp32(_FP32_MAX);

	//Is info invalid?
	if (!(m_ValidFlags & POSITION))
	{	
		// m_SuspectedPos is set the first time we NOTICED the target and whenever we do something that
		// would make us NOTICED.
		if ((GetCurAwareness() >= NOTICED)&&
			(GetSuspectedPosition()[0] == _FP32_MAX))
		{
			SetCorrectSuspectedPosition();
		}
		else if ((GetCurAwareness() >= SPOTTED)/*&&(InLOS())*/)
		{	// m_SuspectedPos is identical to the actual position for SPOTTED targets
			SetCorrectSuspectedPosition();
		}

		//Position info is now valid
		m_ValidFlags |= POSITION;
	}

	if ((GetCurAwareness() == SPOTTED)&&(GetObject()))
	{
		return(GetObject()->GetPosition());
	}
	else
	{
		return GetSuspectedPosition();
	}
};

CVec3Dfp32 CAI_AgentInfo::GetHeadPos()
{
	if (!m_pKB || !m_pKB->GetAI() || !m_pKB->GetAI()->m_pGameObject || !GetObject())
		return CVec3Dfp32(_FP32_MAX);

	CAI_Core* pAgentAI = m_pKB->GetAI()->GetAI(GetObjectID());
	if (pAgentAI)
	{
		CVec3Dfp32 HeadOffset = pAgentAI->GetHeadPos() - pAgentAI->m_pGameObject->GetPosition();
		CVec3Dfp32 HeadPos = HeadOffset + GetBasePos();
		return(HeadPos);
	}
	else
	{
		return CVec3Dfp32(_FP32_MAX);
	}
};

//Get path position
CVec3Dfp32 CAI_AgentInfo::GetPathPos()
{
	if (INVALID_POS(m_PathPos))
		return GetBasePos();
	else
		return m_PathPos;
};


//Set given info to given value. Note that no other info flags are affected.
void CAI_AgentInfo::SetInfo(int _Info, bool _Val)
{
	if (_Val)
		//Info is true
		m_InfoFlags |= _Info;
	else
		//Info is false
		m_InfoFlags &= ~_Info;

	//The given state info is now valid
	m_ValidFlags |= _Info;
};

void CAI_AgentInfo::Touch()
{
	if (m_PendingAwareness <= m_Awareness)
	{
		m_AwarenessTimer = m_pKB->GetAI()->m_Timer;
		m_PendingAwareness = AWARENESS_INVALID;
	}
	if (m_Awareness >= SPOTTED)
	{
		CAI_Core* pAI = m_pKB->GetAI();
		CAI_Core* pTargetAI = pAI->GetAI(GetObjectID());
		if (pTargetAI)
		{
			m_SuspectedHeight = pTargetAI->GetCurHeight();
		}
	}
}

void CAI_AgentInfo::TouchLastSpotted()
{
	CAI_Core* pAI = m_pKB->GetAI();
	if (pAI)
	{
		m_LastSpottedTimer = pAI->m_Timer;
	}
}

//Set awareness to given value. If this means awareness will be raised, the change will 
//be delayed by reaction time
void CAI_AgentInfo::SetAwareness(int _Awareness,bool _RaiseFriends,bool _RightNow)
{
	if (!m_pKB)
	{
		return;
	}
	CAI_Core* pAI = m_pKB->GetAI();
	if (!pAI || (!pAI->m_pServer) || !GetObject())
	{
		return;
	}
	if ((pAI->m_PauseActions)||(_Awareness == AWARENESS_INVALID))
	{
		return;
	}

	// Update last spotted aimpos
	if (_Awareness >= SPOTTED)
	{
		CAI_Core* pTargetAI = pAI->GetAI(GetObjectID());
		if (pTargetAI)
		{
			m_SuspectedHeight = pTargetAI->GetCurHeight();
		}
	}

	if (_Awareness == m_Awareness)
	{
		if ((_Awareness >= DETECTED)&&(_RaiseFriends))
		{	// Tell friends now and then
			pAI->RaiseAwareness(m_iObject,GetSuspectedPosition(),_Awareness);
		}
		// Postpone awareness reduction
		Touch();
		return;
	}

	if (_Awareness > m_Awareness)
	{
		if (_Awareness > m_PendingAwareness)
		{
			// Update our own relation
			int Relation = GetRelation(true);

			// We should shut up any ongoing dialogue
			if (_Awareness >= SPOTTED)
			{	
				if ((Relation >= CAI_AgentInfo::HOSTILE)&&(m_Awareness <= CAI_AgentInfo::NOTICED))
				{
					CWObject_Message Msg(OBJMSG_CHAR_DESTROYCAUSUALDIALOGUE);
					Msg.m_Param0 = CAI_Action::PRIO_DANGER;
					pAI->m_pServer->Message_SendToObject(Msg,pAI->m_pGameObject->m_iObject);
				}
			}

 			m_AwarenessTimer = pAI->m_Timer;
			//We will switch to this awareness when the delay is up
			m_PendingAwareness = _Awareness;

			// Update our position
			m_PendingSuspectedPos = GetObject()->GetPosition();
			if ((Relation >= CAI_AgentInfo::HOSTILE)&&(_RaiseFriends))
			{
				switch(_Awareness)
				{
				case NOTICED:
					// Don't send along our relation here as we honestly don't know what it is
					pAI->RaiseAwareness(m_iObject,m_PendingSuspectedPos,NOTICED);
					break;

				case DETECTED:
					pAI->RaiseAwareness(m_iObject,m_PendingSuspectedPos,DETECTED,GetCurRelation());
					break;

				case SPOTTED:
					pAI->RaiseAwareness(m_iObject,m_PendingSuspectedPos,SPOTTED,GetCurRelation());
					break;
				}
			}
		}
	}
	else
	{	// New awareness is lower than current
		// Current is higher than NONE ie can be lowered
		// Pending is not already one less than current
		// Pending is lower than current (either 2+ steps lower or invalid)
		if (((_Awareness < m_Awareness))&&
			(m_Awareness > NONE)&&
			(m_PendingAwareness != m_Awareness-1)&&
			(m_PendingAwareness <= m_Awareness))
		{
			m_AwarenessTimer = pAI->m_Timer;
			//We will switch to this awareness when the delay is up
			m_PendingAwareness = m_Awareness-1;
			if (m_Awareness == SPOTTED)
			{	// We lost LOS
				m_PendingSuspectedPos = GetObject()->GetPosition();
			}
		}
	}

	if (_RightNow)
	{
		int Relation = GetCurRelation();
		if (m_Awareness < m_PendingAwareness)
		{
			if (m_PendingAwareness == SPOTTED)
			{
				if (pAI->IsPlayer(m_iObject))
				{
					pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_PLAYER, m_iObject);
					// We (re)spotted the player
					if (Relation >= ENEMY)
					{
						pAI->UseRandom("(re)SPOTTED",CAI_Device_Sound::COMBAT_SPOTTED,CAI_Action::PRIO_COMBAT);
					}
				}
				if (Relation >= ENEMY)
				{
					pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_ENEMY, m_iObject);
				}
				else if (Relation >= HOSTILE)
				{
					pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_HOSTILE, m_iObject);
				}
			}
			else if (m_PendingAwareness == DETECTED)
			{
				if (Relation >= ENEMY)
				{
					pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_DETECT_DISTURBANCE, m_iObject);
				}
			}
			else if (m_PendingAwareness == NOTICED)
			{
				if (Relation >= ENEMY)
				{
					pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_NOTICE_ANOMALY, m_iObject);
				}
			}
		}

		m_AwarenessTimer = pAI->m_Timer;
		m_Awareness = m_PendingAwareness;
		if (m_PendingSuspectedPos != CVec3Dfp32(_FP32_MAX))
		{
			SetSuspectedPosition(m_PendingSuspectedPos);
		}
		m_PendingAwareness = AWARENESS_INVALID;
		INVALIDATE_POS(m_PendingSuspectedPos);
	}
};

void CAI_AgentInfo::TouchRelation()
{
	if (m_PendingRelation < m_Relation)
	{
		m_PendingRelation = RELATION_INVALID;

	}
};

//Immediately set relation to given value
void CAI_AgentInfo::SetRelation(int _Relation,int _Delay)
{
	if (!m_pKB)
	{
		return;
	}
	CAI_Core* pAI = m_pKB->GetAI();
	if (!pAI)
	{
		return;
	}

	// Preflight sanity check
	if (_Relation > ENEMY)
	{
		_Relation = ENEMY;
	}

	// We cannot improve relation if m_PendingRelation is about to make it worse
	if ((m_PendingRelation > m_Relation)&&(_Relation < m_PendingRelation))
	{
		return;
	}

	if ((_Delay > 0)&&(m_Relation != _Relation))
	{
		if ((m_PendingRelation == RELATION_INVALID)||
			(_Relation > m_PendingRelation)||
			(m_pKB->GetAI()->m_Timer + _Delay <= m_RelationTimer))
		{	// We don't update the relation yet
			m_PendingRelation = _Relation;
			m_RelationTimer = m_pKB->GetAI()->m_Timer + _Delay;
			return;
		}
	}

	bool bNewEnemy = (_Relation >= ENEMY) && (m_Relation <= ENEMY);
	bool bNewHostile = (m_Relation < HOSTILE) && (_Relation < ENEMY) && (_Relation >= HOSTILE);
	bool bStopHostilities = (m_Relation > UNFRIENDLY) && (_Relation < HOSTILE);
	m_Relation = _Relation;
	m_PendingRelation = RELATION_INVALID;
	if ((pAI->IsPlayer())&&((bNewEnemy)||(bNewHostile)))
	{
		CWObject_Message Msg(OBJMSG_CHAR_DESTROYCAUSUALDIALOGUE);
		Msg.m_Param0 = CAI_Action::PRIO_DANGER;
		pAI->m_pServer->Message_SendToObject(Msg,pAI->m_pGameObject->m_iObject);
	}

	//Check if we should raise events (raise events after setting relation value,t o eliminate chance of infinite recursion)
	if (bNewEnemy && (GetCurAwareness() >= SPOTTED))
	{
		pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_ENEMY, m_iObject);
	}
	else if (bNewHostile && (GetCurAwareness() >= SPOTTED))
	{
		pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_HOSTILE, m_iObject);
	}
	else if (bStopHostilities)
	{	// Relax a bit
		pAI->SetStealthTension(CAI_Core::TENSION_NONE);
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_IDLE);
	}
};

void CAI_AgentInfo::SetSuspectedPosition(CVec3Dfp32& _Pos)
{
	m_SuspectedPosNotGround = _Pos;
};

const CVec3Dfp32& CAI_AgentInfo::GetSuspectedPosition()
{
	if (VALID_POS(m_SuspectedPosNotGround))
	{
		CAI_Core* pAI = m_pKB->GetAI();
		
		if (pAI)
		{
			m_SuspectedPos = pAI->m_PathFinder.SafeGetPathPosition(m_SuspectedPosNotGround,4,1);
		}
		else
		{
			m_SuspectedPos = m_SuspectedPosNotGround;
		}
		INVALIDATE_POS(m_SuspectedPosNotGround);
	}

	if ((INVALID_POS(m_SuspectedPos))||(m_Awareness >= SPOTTED))
	{
		SetCorrectSuspectedPosition();
	}

	return m_SuspectedPos;
}

fp32 CAI_AgentInfo::GetSuspectedHeight()
{
	return(m_SuspectedHeight);
};

CVec3Dfp32 CAI_AgentInfo::GetSuspectedTorsoPosition()
{
	CVec3Dfp32 Pos = GetSuspectedPosition();
	Pos[2] += m_SuspectedHeight * 0.7f;

	return(Pos);
};

CVec3Dfp32 CAI_AgentInfo::GetSuspectedHeadPosition()
{
	CVec3Dfp32 Pos = GetSuspectedPosition();
	Pos[2] += m_SuspectedHeight;

	return(Pos);
};

CVec3Dfp32 CAI_AgentInfo::GetCorrectPosition()
{
	CVec3Dfp32 Pos;
	INVALIDATE_POS(Pos);
	CWObject *pObj = GetObject();
	if (!pObj)
	{
		return(Pos);
	}

	CAI_Core* pAI = GetAgentAI();
	if (pAI)
	{
		Pos = pAI->GetBasePos();
		if ((m_pKB->m_iDarknessPlayer == m_iObject)&&(m_pKB->m_iCreepingDark))
		{	// This agent represents the darkness using player and creeping dark is out
			Pos = m_pKB->m_DarknessPos;
		}
	}
	else
	{
		Pos = pObj->GetPosition();
	}
	return(Pos);
}

void CAI_AgentInfo::SetCorrectSuspectedPosition()
{
	CVec3Dfp32 Pos = GetCorrectPosition();
	if (VALID_POS(Pos))
	{
		SetSuspectedPosition(Pos);
		m_SuspectedPos = Pos;
		m_PendingSuspectedPos = Pos;
		m_ValidFlags |= POSITION;
	}
}

//Get corresponding object index
int CAI_AgentInfo::GetObjectID()
{
	return m_iObject;
};

CAI_Core* CAI_AgentInfo::GetAgentAI()
{
	if ((m_pKB)&&(m_pKB->GetAI()))
	{
		return(m_pKB->GetAI()->GetAI(m_iObject));
	}
	else
	{
		return(NULL);
	}
}

//Get corresponding object pointer
CWObject* CAI_AgentInfo::GetObject()
{
	if (!m_pKB || !m_pKB->GetAI() || !m_pKB->GetAI()->m_pServer)
		return NULL;

	return m_pKB->GetAI()->m_pServer->Object_Get(m_iObject);
};

void CAI_AgentInfo::OnDeltaLoad(CCFile* _pFile)
{
	int8 Temp8;
	int32 Temp32;
	_pFile->ReadLE(Temp32);m_iObject = Temp32;
	_pFile->ReadLE(m_ValidFlags);
	_pFile->ReadLE(m_InfoFlags);
	_pFile->ReadLE(m_Awareness);
	_pFile->ReadLE(m_PendingAwareness);
	_pFile->ReadLE(Temp32); m_AwarenessTimer = Temp32;
	_pFile->ReadLE(m_AwarenessOfUs);
	_pFile->ReadLE(m_Relation);
	m_SuspectedPos.Read(_pFile);
	m_SuspectedPosNotGround.Read(_pFile);
	m_PathPos.Read(_pFile);
	_pFile->ReadLE(Temp8); m_bPosUpdate = (Temp8 != 0);
	_pFile->ReadLE(m_MeleeTick);
};

void CAI_AgentInfo::OnDeltaSave(CCFile* _pFile)
{
	int8 Temp8;
	int32 Temp32;
	
	//Ugly hack to avoid changing behaviour at last minute;
	//Agentinfo for player is by default reset before save, so that he normally start out with a clean slate
 	if (!(m_pKB->GetAI()->m_UseFlags & CAI_Core::USE_SAVEPLAYERINFO) &&
		GetObject() && GetObject()->GetInterface_AI() && !GetObject()->GetInterface_AI()->AI_IsBot())
	{
		//Player, write cleared info
		CAI_AgentInfo Cleared(0, m_pKB);
		Cleared.Init();
        Cleared.OnDeltaSave(_pFile);		
	}
	else
	{
		//Bot or player and we should save
		_pFile->WriteLE((int32)m_iObject);
		_pFile->WriteLE(m_ValidFlags);
		_pFile->WriteLE(m_InfoFlags);
		_pFile->WriteLE(m_Awareness);
		_pFile->WriteLE(m_PendingAwareness);
		Temp32 = m_AwarenessTimer; _pFile->WriteLE(Temp32);
		_pFile->WriteLE(m_AwarenessOfUs);
		_pFile->WriteLE(m_Relation);
		m_SuspectedPos.Write(_pFile);
		m_SuspectedPosNotGround.Write(_pFile);
		m_PathPos.Write(_pFile);
		Temp8 = m_bPosUpdate; _pFile->WriteLE(Temp8);
		_pFile->WriteLE(m_MeleeTick);
	}
};

//CAI_AgentCriteria///////////////////////////////////////////////////////////////////////////////

CAI_AgentCriteria::CAI_AgentCriteria()
{
	m_InfoFlagsTrue = CAI_AgentInfo::INFOFLAGS_NONE;
	m_InfoFlagsFalse = CAI_AgentInfo::INFOFLAGS_NONE;

	m_MinAwareness = CAI_AgentInfo::AWARENESS_INVALID;
	m_MaxAwareness = CAI_AgentInfo::AWARENESS_INVALID;

	m_MinAwarenessOfUs = CAI_AgentInfo::AWARENESS_INVALID;
	m_MaxAwarenessOfUs = CAI_AgentInfo::AWARENESS_INVALID;

	m_MinRelation = CAI_AgentInfo::RELATION_INVALID;
	m_MaxRelation = CAI_AgentInfo::RELATION_INVALID;

	INVALIDATE_POS(m_CheckPos);
	m_MaxDistanceSqr = _FP32_MAX;
	m_MinDistanceSqr = 0;
	m_bXYDistOnly = false;
};


//Add info flags criteria that must hold if and optionally flags that cannot hold if
void CAI_AgentCriteria::InfoFlags(int _TrueFlags, int _FalseFlags)
{
	m_InfoFlagsTrue = _TrueFlags;
	m_InfoFlagsFalse = _FalseFlags;
};


//Add min and optionally max awareness criteria 
void CAI_AgentCriteria::Awareness(int _Min, int _Max)
{
	m_MinAwareness = _Min;
	m_MaxAwareness = _Max;
};


//Add min and optionally max awareness of us criteria 
void CAI_AgentCriteria::AwarenessOfUs(int _Min, int _Max)
{
	m_MinAwarenessOfUs = _Min;
	m_MaxAwarenessOfUs = _Max;
};

//Add relation criteria 
void CAI_AgentCriteria::Relation(int _Min, int _Max)
{
	m_MinRelation = _Min;
	m_MaxRelation = _Max;
};


//Add proximity and optionally remoteness criteria. If the _bStrict flag is set any 
//uncertainties will cause the agent to fail this criteris if possible, otherwise uncertainties 
//will cause criteria to hold if possible. If the _bXYOnly value is set, only distances in the
//xy-plane will be considered
void CAI_AgentCriteria::Distance(const CVec3Dfp32& _Pos,  fp32 _WithinDist, fp32 _OutsideDist, bool _bStrict, bool _bXYOnly)
{
	m_CheckPos = _Pos;
	m_MaxDistanceSqr = Sqr(_WithinDist);
	m_MinDistanceSqr = Sqr(_OutsideDist);
	m_bXYDistOnly = _bXYOnly;
};




//CAgentIteration (knowledgebase help class)//////////////////////////////////////////////////

CAI_KnowledgeBase::CAgentIteration::CAgentIteration()
{
	m_iIndex = -1;
	m_List = ALL_AGENTS;
	m_Cur = 0;
	m_Criteria = CAI_AgentCriteria();
};

bool CAI_KnowledgeBase::CAgentIteration::operator==(const CAgentIteration& _Compare)
{
	return (_Compare.m_iIndex == m_iIndex);	
}

//Get best list, based on agent criteria
int CAI_KnowledgeBase::CAgentIteration::RecommendedList(const CAI_AgentCriteria& _Criteria)
{
	//Always use all agents list for now. Other lists may be useful for optimizing stuff later on...
	return ALL_AGENTS;
};




//CAI_KnowledgeBase///////////////////////////////////////////////////////////////////////////////////////
CAI_KnowledgeBase::CAI_KnowledgeBase()
{
	m_pAI = NULL;
	m_iCollAgent = 0;
	m_iCollAgent2 = 0;
	m_iSkipCollAgent = 0;
	m_LastAttackedTimer = -108000;	// An hour ago
	m_iLastAttacker = 0;
	m_LastHurtTimer = -108000;		// An hour ago

	m_iLastMeleeNonEnemyFighter = 0;		// Last agent that was melee fighting (not neccessarily with us)
	m_LastMeleeNonEnemyFighterTimer = 0;

	m_Knowledge = 0;	
	m_ValidKnowledge = 0;	
	m_Alertness = ALERTNESS_INVALID;
	m_DefaultAlertness = ALERTNESS_INVALID;
	m_InitialAlertness = ALERTNESS_INVALID;
	m_NEnemies = 0;
	m_NHostiles = 0;
	m_NFriends = 0;
	m_NInvestigates = 0;

	m_iDarknessPlayer = 0;
	m_DarknessPowers = 0;
	m_iCreepingDark = 0;
	INVALIDATE_POS(m_DarknessPos);

	m_AlertnessRangeSqr = Sqr(64.0f);
	m_CurNoise = 0.0f;
	m_CurLight = 0.0f;

	//Set up local data structures
	m_lAgents.Create(CAI_AgentInfo());
	m_AgentObjIDMap.Create(-1, 10);
	m_lEnemies.Create(-1);
	m_lAgentIterations.Create(CAgentIteration());

	m_lDead.Clear();
	m_liBrokenLights.Clear();
	m_iDeadFinger = 0;
	m_NbrOfValidDead = -1;
	m_iBrokenLightsFinger = 0;
};

void CAI_KnowledgeBase::SetAI(CAI_Core * _pAI)
{
	m_pAI = _pAI;
};

//An agent is a potential enemy if he's attacking us, is a player of a team we're not 
//friendly with or we have a bad relationship with him
bool CAI_KnowledgeBase::IsPotentialEnemy(CAI_AgentInfo * _Info)
{
	return _Info->CheckInfoFlags(CAI_AgentInfo::IS_ATTACKING_US) ||
	       (_Info->GetRelation() >= CAI_AgentInfo::HOSTILE) ||
		   (m_pAI->IsPlayer(_Info->GetObjectID()) && (DefaultRelation(_Info->GetObjectID()) != CAI_AgentInfo::FRIENDLY));
};


//Length of given list (param is CAgentIteration list type enum)
int CAI_KnowledgeBase::ListLength(int _List)
{
	switch (_List)
	{
	case CAgentIteration::ENEMIES:
		return m_lEnemies.Length();
	default:
		return m_lAgents.Length();
	}
};

//Change AI user
void CAI_KnowledgeBase::ReInit(CAI_Core * _pAI)
{
	SetAI(_pAI);
	for (int i = 0; i < m_lAgents.Length(); i++)
	{
		if (m_lAgents.IsValid(i))
			m_lAgents.Get(i).m_pKB = this; 
	}
};


//Initialize knowledge base. Run this before first refresh.
void CAI_KnowledgeBase::Init()
{
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Set alertness to watchful if not already set
	if (m_Alertness == ALERTNESS_INVALID)
	{
		SetAlertness(ALERTNESS_WATCHFUL, true);
	}

	//Clean up existing agent info list (i.e. those we get from load)
	for (int i = 0; i < m_lAgents.Length(); i++)
	{
		CAI_AgentInfo* pInfo = &m_lAgents.Get(i);
		if ((!pInfo)||(!pInfo->GetObjectID())||(!pInfo->GetObject()))
		{
			m_lAgents.Remove(i);
			if (pInfo)
				m_AgentObjIDMap.Remove(pInfo->GetObjectID());
		}
	};

	// Add player if not already there
	int iPlayer = m_pAI->GetClosestPlayer();
	if (iPlayer)
	{
		Global_NewAgent(iPlayer);
	}

	//Check if we should clean up global agent list
	CSimpleIntList &lAgents = m_pAI->m_pAIResources->m_KBGlobals.ms_lAgents;
	if (m_pAI->m_pAIResources->m_KBGlobals.ms_LastAgentCleaningTick + 100 < m_pAI->GetAITick())
	{
		m_pAI->m_pAIResources->m_KBGlobals.ms_LastAgentCleaningTick = m_pAI->GetAITick();
		for (int i = 0; i < lAgents.Length(); i++)
		{
			if (lAgents.IsValid(i) &&
				!m_pAI->IsValidAgent(lAgents.Get(i)))
			{
				lAgents.Remove(i);
			}
		};
	}

	//Add self to static character list
	Global_NewAgent(m_pAI->m_pGameObject->m_iObject);

	//Add all interesting agents in agent list
	for (int j = 0; j < lAgents.Length(); j++)
	{
		NewAgent(lAgents.Get(j));
	}

	// Check all loaded dead for signs of life
	for (int i = m_lDead.Len()-1; i >= 0; i--)
	{
		CAI_Core* pDeadAI = m_pAI->GetAI(m_lDead[i].m_iVictim);
		if ((pDeadAI)&&(pDeadAI->m_pGameObject)&&(pDeadAI->m_pGameObject->AI_IsAlive()))
		{	// He wasn't dead after all
			m_lDead.Del(i);
		}
	}

	// Check for enemies and hostiles
	for (int iAgent=0; iAgent < m_lAgents.Length(); iAgent++)
	{
		if (m_lAgents.IsValid(iAgent))
		{
			CAI_AgentInfo* pInfo = &m_lAgents.Get(iAgent);
			if ((pInfo)&&(pInfo->GetCurAwareness() >= CAI_AgentInfo::SPOTTED))
			{
				int iObj = pInfo->GetObjectID();
				if (pInfo->GetCurRelation() >= CAI_AgentInfo::ENEMY)
				{
					GetAI()->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_ENEMY,iObj);
				}
				else if (pInfo->GetCurRelation() >= CAI_AgentInfo::HOSTILE)
				{
					GetAI()->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_HOSTILE,iObj);
				}

				// We spotted the player?
				if (GetAI()->IsPlayer(iObj))
				{
					GetAI()->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_PLAYER,iObj);
				}
			}
		}
	}
};

//Savegame
void CAI_KnowledgeBase::OnDeltaLoad(CCFile* _pFile)
{
	//Load m_lAgents
	m_lAgents.Clear();
	m_AgentObjIDMap.Clear();
	m_lEnemies.Clear();
	int32 nAgents;
	int32 iAgent;
	_pFile->ReadLE(nAgents);
	for (iAgent=0; iAgent<nAgents; iAgent++)
	{
		CAI_AgentInfo Agent(-1, this);
		int iIndex = m_lAgents.Add(Agent);
		M_ASSERT( iIndex != -1, "Failed to add agent" );
		CAI_AgentInfo* pAgent = &(m_lAgents.Get(iIndex));
		pAgent->OnDeltaLoad(_pFile);
		m_AgentObjIDMap.Add(pAgent->m_iObject, iIndex);
		if (IsPotentialEnemy(pAgent))//Doesn't work, can't detect players at this point
			m_lEnemies.Add(iIndex);  //But currently I don't use enemy list anyway
	}

	int32 Temp32;
	int16 Temp16;
	int8 Temp8;
	_pFile->ReadLE(Temp32); m_LastAttackedTimer = Temp32; m_LastAttackedTimer += m_pAI->m_Timer;
	_pFile->ReadLE(Temp16); m_iLastAttacker = Temp16;
	_pFile->ReadLE(Temp32); m_LastHurtTimer = Temp32; m_LastHurtTimer += m_pAI->m_Timer;
	_pFile->ReadLE(Temp8); m_Knowledge = Temp8;
	_pFile->ReadLE(Temp8); m_ValidKnowledge = Temp8;
	_pFile->ReadLE(Temp8); m_Alertness = Temp8;

	int32 nDead;
	_pFile->ReadLE(nDead);
	for (int i = 0; i < nDead; i++)
	{
		SDead curDead;
		_pFile->ReadLE(Temp32); curDead.m_iCause = Temp32;
		_pFile->ReadLE(Temp32); curDead.m_iVictim = Temp32;
		_pFile->ReadLE(Temp8);
		if (Temp8 != 0)
		{
			curDead.m_bFound = true;
		}
		else
		{
			curDead.m_bFound = false;
		}
		_pFile->ReadLE(Temp8); curDead.m_CauseOfDeath = Temp8;
		_pFile->ReadLE(Temp8); curDead.m_Relation = Temp8;
		_pFile->ReadLE(Temp32); curDead.m_TimeOfDeath = Temp32;
		curDead.m_bInvestigated = true;
		m_lDead.Add(curDead);
		m_NbrOfValidDead = -1;
	}
};

void CAI_KnowledgeBase::OnDeltaSave(CCFile* _pFile)
{
	//Save m_lAgents
	int32 iAgent;
 	int32 nAgents = 0;
	for (iAgent=0; iAgent<m_lAgents.Length(); iAgent++)
	{
		if (m_lAgents.IsValid(iAgent))
			nAgents++;
	}

	_pFile->WriteLE(nAgents);
	for (iAgent=0; iAgent<m_lAgents.Length(); iAgent++)
	{
		if (m_lAgents.IsValid(iAgent))
			m_lAgents.Get(iAgent).OnDeltaSave(_pFile);
	}

	int32 Temp32;
	int16 Temp16;
	int8 Temp8;
	// Save ticks since timer
	Temp32 = m_LastAttackedTimer - m_pAI->m_Timer; _pFile->WriteLE(Temp32);
	Temp16 = m_iLastAttacker; _pFile->WriteLE(Temp16);
	// Save ticks since timer
	Temp32 = m_LastHurtTimer - m_pAI->m_Timer; _pFile->WriteLE(Temp32);
	Temp8 = m_Knowledge; _pFile->WriteLE(Temp8);
	Temp8 = m_ValidKnowledge; _pFile->WriteLE(Temp8);
	Temp8 = m_Alertness; _pFile->WriteLE(Temp8);

	int32 nDead = 0;
	if (!m_pAI->IsPlayer())
	{
		nDead = m_lDead.Len();
	}

	_pFile->WriteLE(nDead);
	for (int i = 0; i < nDead; i++)
	{	// We store neither position nor relation
		// as we will recalculate them when loading
		// All loaded deads will be considered investigated
		Temp32 = m_lDead[i].m_iCause; _pFile->WriteLE(Temp32);
		Temp32 = m_lDead[i].m_iVictim; _pFile->WriteLE(Temp32);
		Temp8 = m_lDead[i].m_bFound; _pFile->WriteLE(Temp8);
		Temp8 = m_lDead[i].m_CauseOfDeath; _pFile->WriteLE(Temp8);
		Temp8 = m_lDead[i].m_Relation; _pFile->WriteLE(Temp8);
		Temp32 = m_lDead[i].m_TimeOfDeath; _pFile->WriteLE(Temp32);
	}
};

void CAI_KnowledgeBase::OnPostWorldLoad()
{
//	for (int i = 0; i < m_lAgents.Length(); i++)
//	{
//		CAI_AgentInfo* pInfo = &m_lAgents.Get(i);
//		if ((!pInfo)||(!pInfo->GetObjectID())||(!pInfo->GetObject()))
//		{
//			m_lAgents.Remove(i);
//			if (pInfo)
//				m_AgentObjIDMap.Remove(pInfo->GetObjectID());
//		}
//	};
};

bool CAI_KnowledgeBase::CheckCollisions()
{
	if ((!m_pAI->m_pGameObject)||(!m_pAI->m_pGameObject->AI_IsAlive()))
	{
		return(false);
	}

	m_bCollAgentChanged = false;
	bool rValue = false;
	static fp32 MaxRngSqr = Sqr(AI_COLL_AVOID_RANGE);
	if (!m_pAI->m_CollisionRank)
	{
		if (!m_iSkipCollAgent)
		{
			m_pAI->SetCharacterCollisions(true);
		}
		return(rValue);
	}

	// Pick one agent to check for collisions
	CSimpleIntList &lAgents = m_pAI->m_pAIResources->m_KBGlobals.ms_lAgents;
	int iList = m_pAI->m_Timer % lAgents.Length();
	int iCur = lAgents.Get(iList);
	// Check validity and relative ranges of our new pick
	if ((iCur != m_pAI->GetObjectID())&&(iCur != m_iCollAgent)&&(iCur != m_iCollAgent2)&&(m_pAI->ShouldAvoid(iCur)))
	{
		fp32 RangeSqr = m_pAI->SqrDistanceToUs(iCur);
		if (RangeSqr <= MaxRngSqr)
		{
			if ((!m_iCollAgent)||(RangeSqr < m_pAI->SqrDistanceToUs(m_iCollAgent)))
			{
				m_iCollAgent2 = m_iCollAgent;
				m_iCollAgent = iCur;
				m_bCollAgentChanged = true;
			}
			else if ((!m_iCollAgent2)||(RangeSqr < m_pAI->SqrDistanceToUs(m_iCollAgent2)))
			{
				m_iCollAgent2 = iCur;
				m_bCollAgentChanged = true;
			}
		}
	}

	if ((m_iSkipCollAgent)||(m_pAI->m_iDialoguePartner))
	{
		if ((m_iCollAgent == m_iSkipCollAgent)||(m_iCollAgent == m_pAI->m_iDialoguePartner))
		{
			m_iCollAgent = 0;
		}
		if ((m_iCollAgent2 == m_iSkipCollAgent)||(m_iCollAgent2 == m_pAI->m_iDialoguePartner))
		{
			m_iCollAgent2 = 0;
		}
		m_iSkipCollAgent = 0;	// This member must be set every tick to be valid
	}

	// Verify m_iCollAgent and m_iCollAgent2
	if ((m_iCollAgent)&&((m_pAI->SqrDistanceToUs(m_iCollAgent) > MaxRngSqr)||(!m_pAI->ShouldAvoid(m_iCollAgent))))
	{
		m_iCollAgent = 0;
	}
	if ((m_iCollAgent2)&&((m_pAI->SqrDistanceToUs(m_iCollAgent2) > MaxRngSqr)||(!m_pAI->ShouldAvoid(m_iCollAgent2))))
	{
		m_iCollAgent2 = 0;
	}
	if ((!m_iCollAgent)&&(m_iCollAgent2))
	{
		Swap(m_iCollAgent,m_iCollAgent2);
		m_bCollAgentChanged = true;
	}
	else if ((m_iCollAgent)&&(m_iCollAgent2))
	{
		if (m_pAI->SqrDistanceToUs(m_iCollAgent2) < m_pAI->SqrDistanceToUs(m_iCollAgent))
		{
			Swap(m_iCollAgent,m_iCollAgent2);
		}
		m_bCollAgentChanged = true;
	}

	// A quick look to tell the other that we are colliding

	if ((m_iCollAgent)||(m_iCollAgent2))
	{
		rValue = m_pAI->HandleCollisionNew(m_iCollAgent,m_iCollAgent2,MaxRngSqr);
	}
	else
	{
		// Avoid corpses
		if (m_pAI->m_CharacterClass == CAI_Core::CLASS_CIV)
		{
			SDead Dead;
			if (GetFirstMatchingDead(&Dead,CAI_AgentInfo::UNKNOWN,0.0f,AI_COLL_AVOID_RANGE,false,false))
			{
				rValue = m_pAI->HandleCollisionNew(Dead.m_iVictim,m_iCollAgent2,MaxRngSqr);
			}
		}
		if (!m_iSkipCollAgent)
		{
			m_pAI->SetCharacterCollisions(true);
		}
	}
	m_iSkipCollAgent = 0;

	// Do NOT set m_iCollAgent or m_iCollAgent2 to 0 here as we use m_iCollAgent in GetBestScenePoint to determine what side of the world
	// is best for us to pick from. Hopefully this will reduce collisions quite a bit.

	return(rValue);
}

int CAI_KnowledgeBase::GetCollider()
{
	return(m_iCollAgent);
};

bool CAI_KnowledgeBase::DidCollAgentChange()
{
	return(m_bCollAgentChanged);
};

void CAI_KnowledgeBase::SkipCollChecking(int32 _iSkip)
{
	m_iSkipCollAgent = _iSkip;
};

void CAI_KnowledgeBase::AddLoudness(int _Loudness)
{
	if (_Loudness * 0.01f > m_CurNoise)
	{
		m_CurNoise = _Loudness * 0.01f;
	}
}

void CAI_KnowledgeBase::AddLight(int _Light)
{
	if (_Light * 0.01f > m_CurLight)
	{
		m_CurLight = _Light * 0.01f;
	}
}

fp32 CAI_KnowledgeBase::GetNoise()
{
	return(m_CurNoise);
}

fp32 CAI_KnowledgeBase::GetLight()
{
	return(m_CurLight);
}

void CAI_KnowledgeBase::RefreshNoise()
{
	m_CurNoise *= 0.75;
	if (m_CurNoise < 0.001f)
	{
		m_CurNoise = 0.0f;
	}
}

void CAI_KnowledgeBase::RefreshLight()
{
	m_CurLight *= 0.75;
	if (m_CurLight < 0.001f)
	{
		m_CurLight = 0.0f;
	}
}

int CAI_KnowledgeBase::GetPlayerDarkness()
{
	return(m_iDarknessPlayer);
};

bool CAI_KnowledgeBase::GetDarknessPos(CVec3Dfp32* _pDarknessPos)
{
	if (VALID_POS(m_DarknessPos))
	{
		if (_pDarknessPos)
		{
			*_pDarknessPos = m_DarknessPos;
		}
		return(true);
	}
	else
	{
		return(false);
	}
};

bool CAI_KnowledgeBase::GetCreepingDark(int32* _piCreepDark)
{
	if (m_iCreepingDark)
	{
		if (_piCreepDark)
		{
			*_piCreepDark = m_iCreepingDark;
		}
		return(true);
	}
	
	return(false);
};

void CAI_KnowledgeBase::OnRefreshDarkness()
{
	if ((m_pAI)&&(m_pAI->m_pAIResources)&&(m_pAI->m_bFearDarkness))
	{
		m_iDarknessPlayer = m_pAI->m_pAIResources->m_KBGlobals.ms_iDarknessPlayer;
		m_DarknessPowers = m_pAI->m_pAIResources->m_KBGlobals.ms_DarknessPowers;
		m_iCreepingDark = m_pAI->m_pAIResources->m_KBGlobals.ms_iCreepingDark;
		m_DarknessPos = m_pAI->m_pAIResources->m_KBGlobals.ms_DarknessPos;
		
		// OK, deal with it
		CAI_AgentInfo* pDarknessPlayer = GetAgentInfo(m_iDarknessPlayer);
		if (pDarknessPlayer)
		{
			int CurAwareness = pDarknessPlayer->GetCurAwareness();
			int CurRelation = pDarknessPlayer->GetCurRelation();
			if ((m_iCreepingDark)&&(VALID_POS(m_DarknessPos)))
			{
				bool bTrackCreepingDark = false;
				CVec3Dfp32 Start = m_pAI->GetHeadPos();
				int32 FOVResult = m_pAI->InsideFOV(m_DarknessPos);
				fp32 DarknessRangeSqr = Start.DistanceSqr(m_DarknessPos);
				fp32 PlayerRangeSqr = m_pAI->SqrDistanceToUs(m_iDarknessPlayer);
				if ((CurAwareness < CAI_AgentInfo::SPOTTED)||(DarknessRangeSqr <= PlayerRangeSqr))
				{	// We may prefer to track creeping dark
					if ((FOVResult >= CAI_AgentInfo::NOTICED)&&(DarknessRangeSqr <= Sqr(m_pAI->m_SightRange * PERCEPTIONFACTOR_CREEP_NOTICE)))
					{
						if (m_pAI->FastCheckLOS(Start,m_DarknessPos,m_pAI->m_pGameObject->m_iObject,m_iCreepingDark))
						{	// Say something appropriate!
							// OK, we have LOS, what happens next can be any of three things:
							// 1: We are close enough the spot the little bugger, shoot it
							// 2: Not quite sure, no gunplay
							// 3: What was that, investigate?
							if ((FOVResult >= CAI_AgentInfo::SPOTTED)&&(DarknessRangeSqr <= Sqr(m_pAI->m_SightRange * PERCEPTIONFACTOR_CREEP_DETECT)))
							{
								if (DarknessRangeSqr <= Sqr(m_pAI->m_SightRange * PERCEPTIONFACTOR_CREEP_SPOT))
								{
									CurRelation = CAI_AgentInfo::ENEMY;
									CurAwareness = CAI_AgentInfo::SPOTTED;
								}
								else
								{
									CurRelation = CAI_AgentInfo::ENEMY;
									CurAwareness = CAI_AgentInfo::DETECTED;
								}
							}
							else
							{	// Peripheral vision
								CurRelation = CAI_AgentInfo::ENEMY;
								CurAwareness = CAI_AgentInfo::NOTICED;
								m_pAI->m_AH.SetInvestigateObject(m_iCreepingDark);
							}

							pDarknessPlayer->SetRelation(CurRelation);
							pDarknessPlayer->SetAwareness(CurAwareness,true,true);
							pDarknessPlayer->SetSuspectedPosition(m_DarknessPos);
							bTrackCreepingDark = true;
						}
					}
				}

				if (!bTrackCreepingDark)
				{
					m_iCreepingDark = 0;
				}
			}

			// OK; we're done withe the creeoing dark pos adjustment
			if (CurAwareness >= CAI_AgentInfo::SPOTTED)
			{	// We see the player, is he showing his Darkness?
				// if (!m_pAI->m_bHasSeenDarkness)
				CurRelation = CAI_AgentInfo::ENEMY;
				if (~m_pAI->m_SeenDarknessPowers & m_DarknessPowers)
				{
					// Say something appropriate!
					m_pAI->UseRandom(CStr("Spot darkness"),CAI_Device_Sound::SPOT_PLAYER_DARKNESS,CAI_Action::PRIO_COMBAT);

					// Already checked this: lif (m_pAI->m_bFearDarkness)
					switch(m_pAI->m_CharacterClass)
					{
					case CAI_Core::CLASS_CIV:
						m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
						pDarknessPlayer->SetRelation(CurRelation);
						m_pAI->m_DeviceStance.Crouch(true);
						m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_SCARED,60);
						break;
					case CAI_Core::CLASS_TOUGHGUY:
						m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
						pDarknessPlayer->SetRelation(CurRelation);
						m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_SCARED,60);
						break;
					case CAI_Core::CLASS_BADGUY:
						m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
						pDarknessPlayer->SetRelation(CurRelation);
						// m_pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_COMBAT_GESTURE_SCARED,CAI_Action::PRIO_COMBAT,0,RoundToInt(40 + 20 * Random),NULL,NULL);
						m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_SCARED,60);
						m_pAI->GiveOrder(CAI_Core::ORDER_COVER,m_pAI->GetAITicksPerSecond() * 5);
						break;
					default:
						break;
					}
					m_pAI->m_SeenDarknessPowers |= m_DarknessPowers;
				}
				else
				{	// We've seen darkness before
					// Keep relation
					if (m_pAI->m_bFearDarkness)
					{
						switch(m_pAI->m_CharacterClass)
						{
						case CAI_Core::CLASS_CIV:
							m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
							pDarknessPlayer->SetRelation(CurRelation);
							m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_SCARED,60);
							break;
						case CAI_Core::CLASS_TOUGHGUY:
							m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
							pDarknessPlayer->SetRelation(CurRelation);
							m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_SCARED,60);
							break;
						case CAI_Core::CLASS_BADGUY:
							m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
							pDarknessPlayer->SetRelation(CurRelation);
							m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_SCARED,60);
							break;
						default:
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		INVALIDATE_POS(m_DarknessPos);
		m_iDarknessPlayer = 0;
		m_iCreepingDark = 0;
	}
};

// Finds a random agent within _Rng with _MinAwareness or better and _MaxRelation or better
CAI_AgentInfo* CAI_KnowledgeBase::GetRandomAgentInRange(fp32 _Rng, int _MinAwareness, int _MaxRelation)
{
	int N = m_lAgents.Length();
	int iStart = TruncToInt(N * Random * 0.999f);
	fp32 RngSqr = Sqr(_Rng);
	for (int i = 0; i < N; i++)
	{
		int index = (iStart+i) % N;
		if (m_lAgents.IsValid(index))
		{
			CAI_AgentInfo* pInfo = &m_lAgents.Get(index);
			if (pInfo)
			{
				if ((pInfo->GetCurAwareness() >= _MinAwareness)&&(pInfo->GetCurRelation() <= _MaxRelation))
				{
					CVec3Dfp32 Pos = pInfo->GetSuspectedPosition();
					if ((m_pAI->SqrDistanceToUs(pInfo->GetObjectID()) <= RngSqr)&&(Random <= 0.5f))
					{
						return(pInfo);
					}
				}
			}
		}
	}

	return(NULL);
};

//Refresh world knowledge
void CAI_KnowledgeBase::OnRefresh()
{
	MSCOPESHORT(CAI_KnowledgeBase::OnRefresh);
	if (!m_pAI || !m_pAI->IsValid())
		return;

	int i;

	//Refresh general knowledge
	//Player ally is never implicitly invalidated, everything else is always invalidated
	m_ValidKnowledge &= (PLAYER_ALLY);

	//Update alertness every few frames
	OnRefreshAlertness();
	RefreshNoise();
	RefreshLight();

	int iOurID = GetAI()->m_pGameObject->m_iObject;
	// Only update agent info if we're conscious
	int GameTick = m_pAI->GetAITick();
	if (GetAlertness() > ALERTNESS_OBLIVIOUS)
	{
		OnRefreshDarkness();

		//Refresh agent info
		m_NEnemies = 0;
		m_NHostiles = 0;
		m_NFriends = 0;
		m_NInvestigates = 0;

		// Refresh our knowledge of the players darkness
		for (i = 0; i < m_lAgents.Length(); i++)
		{
			if (m_lAgents.IsValid(i))
			{
				CAI_AgentInfo* pInfo = &m_lAgents.Get(i);
				int iObj = pInfo->GetObjectID();
				CAI_Core* pInfoAI = m_pAI->GetAI(iObj);
				// Ignore self (duh)
				if (iObj != iOurID)
				{
#ifndef M_RTM
					if ((m_pAI->DebugTarget())&&(m_pAI->IsPlayer(iObj)))
					{
						bool wtf = true;
					}
#endif
					pInfo->OnRefresh();
					//Remove any non-valid agents
					if (!pInfo->IsValid())
					{
						m_lAgents.Remove(i);
						m_AgentObjIDMap.Remove(iObj);
					}
					else
					{
						int CurAwareness = pInfo->GetCurAwareness();
						int CurRelation = pInfo->GetCurRelation();
						// Player's relation to others is at least as bad as their relation to him/her
						// Truly badass!
						if (m_pAI->IsPlayer())
						{	// If a player has an enemy then the player becomes the enemys enemy
							if (pInfoAI)
							{
								CAI_AgentInfo* pAgentInfoOnUs = pInfoAI->m_KB.GetAgentInfo(iOurID);
								if ((pAgentInfoOnUs)&&(pAgentInfoOnUs->GetCurRelation() > CurRelation))
								{
									CurRelation = pAgentInfoOnUs->GetCurRelation();
									pInfo->SetRelation(CurRelation);
								}
							}
						}
						else
						{	// Not a player
							if (iObj == m_iDarknessPlayer)
							{	// Is this right?
								if (m_iCreepingDark)
								{
									pInfo->SetSuspectedPosition(m_DarknessPos);
								}
							}
						}
						if (CurAwareness >= CAI_AgentInfo::DETECTED)
						{	// Update knowledge of enemies
							if (CurRelation >= CAI_AgentInfo::ENEMY)
							{
								m_NEnemies++;
								m_pAI->SwitchTarget(iObj,false);
								m_ValidKnowledge |= ENEMY_DETECTED;
								m_Knowledge |= ENEMY_DETECTED;
								if (CurAwareness >= CAI_AgentInfo::SPOTTED)
								{
									m_ValidKnowledge |= ENEMY_SPOTTED;
									m_Knowledge |= ENEMY_SPOTTED;
								}
							}
							else if ((CurRelation >= CAI_AgentInfo::HOSTILE)&&(CurRelation <= CAI_AgentInfo::HOSTILE_2))
							{
								m_NHostiles++; 
								m_pAI->SwitchHostile(iObj,false);
							}
							else if (CurRelation <= CAI_AgentInfo::FRIENDLY)
							{
								m_NFriends++;
								m_pAI->SwitchFriendly(iObj,false);
							}

							// Melee fighting hostile?
							if ((CurAwareness >= CAI_AgentInfo::SPOTTED)&&
								(CurRelation >= CAI_AgentInfo::UNFRIENDLY)&&
								(CurRelation < CAI_AgentInfo::ENEMY)&&
								(pInfo->DoesMeleeBlow(GameTick)))
							{
								if (m_pAI->m_MeleeRelationIncrease)
								{
									if (m_pAI->m_bHostileFromBlowDmg)
									{	// We cannot get angrier than HOSTILE_2
										if (CurRelation+m_pAI->m_MeleeRelationIncrease >= CAI_AgentInfo::HOSTILE_2)
										{
											pInfo->SetRelation(CAI_AgentInfo::HOSTILE_2);
										}
										else
										{
											pInfo->SetRelation(CurRelation+m_pAI->m_MeleeRelationIncrease);
										}
									}
									else
									{	// NOTE: Relation is capped to ENEMY by SetRelation()
										pInfo->SetRelation(CurRelation+m_pAI->m_MeleeRelationIncrease);
									}
								}
								m_iLastMeleeNonEnemyFighter = pInfo->GetObjectID();
								m_LastMeleeNonEnemyFighterTimer = m_pAI->m_Timer;
								pInfo->ClearMeleeBlow();
							}

							if ((m_pAI->m_SecurityTolerance > 0)&&
								(CurAwareness >= CAI_AgentInfo::SPOTTED)&&
								(m_pAI->IsPlayer(pInfo->GetObjectID()))&&
								(CurRelation >= CAI_AgentInfo::UNFRIENDLY)&&
								(CurRelation < CAI_AgentInfo::ENEMY))
							{	// Dragging body?
								int iBody = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETGRABBEDBODY),pInfo->GetObjectID());
								if (iBody)
								{
									pInfo->SetRelation(CAI_AgentInfo::ENEMY);
								}
							}
						}
						else if (CurAwareness == CAI_AgentInfo::NOTICED)
						{
							m_NInvestigates++; 
							m_pAI->SwitchInvestigate(iObj,false);
						}
					}
				}
			}
		}

		if ((!m_NEnemies)&&(!m_NHostiles)&&(!m_NInvestigates)&&(m_pAI->GetStealthTension() <= CAI_Core::TENSION_NONE))
		{
			if (m_pAI->m_DeviceStance.GetIdleStance() > CAI_Device_Stance::IDLESTANCE_IDLE)
			{
				m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_IDLE);
			}
		}

		// Done refreshing all agents
		m_pAI->RefreshInteresting();
	}
	else
	{	// Set awareness to NONE when bot is OBLIVIOUS
		for (i = 0; i < m_lAgents.Length(); i++)
		{
			if (m_lAgents.IsValid(i))
			{
				int iObj = m_lAgents.Get(i).GetObjectID();
				// Ignore self (duh)
				if (iObj != iOurID)
				{
					m_lAgents.Get(i).SetAwareness(CAI_AgentInfo::NONE,false,true);
				}
			}
		}
	}
};

//Notify knowledge base that a new agent has come into play. Knowledge base will add agent to 
//set of interesting agents if appropriate
CAI_AgentInfo * CAI_KnowledgeBase::NewAgent(int _iObj, bool _bAddGlobal)
{
	if (!m_pAI || !m_pAI->m_pServer)
		return NULL;

	CWObject* pObj = m_pAI->m_pServer->Object_Get(_iObj);
	if ((!pObj)||
		(CStrBase::CompareNoCase(pObj->GetName(), "TELEPHONEREG") == 0)||
		(CStrBase::CompareNoCase(pObj->GetName(), "RADIO") == 0))
	{
		return(false);
	}

	//Add agent to global list of agents if appropriate
	if (_bAddGlobal)
		Global_NewAgent(_iObj);

	// Don't add any agents to the players KB, and don't add self
	if (/*(m_pAI->IsPlayer())||*/(m_pAI->m_pGameObject->m_iObject == _iObj))
	{	
		return(NULL);
	}

	
	//Interesting agents are potential enemies, players and friends
	int DefaultRel = DefaultRelation(_iObj);

	// Add players, friends and unfriendlies or worse
	if ((m_pAI->IsPlayer(_iObj))||
		(DefaultRel >= CAI_AgentInfo::UNFRIENDLY)||
		(DefaultRel == CAI_AgentInfo::FRIENDLY))
	{
		//Invalidate player ally info
		m_ValidKnowledge &= ~PLAYER_ALLY;
/*
		CStr myName = m_pAI->m_pGameObject->GetName();
		myName += CStrF(" %d",m_pAI->m_pGameObject->m_iObject);
		CStr agentName = m_pAI->GetAI(_iObj)->m_pGameObject->GetName();
		agentName += CStrF(" %d",m_pAI->GetAI(_iObj)->m_pGameObject->m_iObject);
		LogFile(agentName + CStr(" was added to ") + myName);
*/
		//Agent is interesting, add
		return AddAgent(_iObj);
	}
	else
		return NULL;
};


//Adds agent to the set of interesting agents that knowledge base keeps track of, with given info etc
CAI_AgentInfo* CAI_KnowledgeBase::AddAgent(int _iObj, int _Info, int _Awareness, int _AwarenessOfUs, int _Relation)
{
	if ((!m_pAI)||(!m_pAI->m_pServer))
	{
		return NULL;
	}
	
	//Make shure the agents ai can be added as an agent
	CAI_Core* pAgentAI = m_pAI->GetAI(_iObj);
	if ((pAgentAI)&&(!pAgentAI->CanAddAgent()))
	{
		return NULL;
	}

	//Make sure agent is valid
	CWObject* pAgent = m_pAI->IsValidAgent(_iObj);

	if (pAgent)
	{
		const char* pName = pAgent->GetName();
		if ((CStrBase::CompareNoCase(pName, "TELEPHONEREG") == 0)||
			(CStrBase::CompareNoCase(pName, "RADIO") == 0))
		{
			return(false);
		}

		CAI_AgentInfo Agent(_iObj, this);

		//Check that agent hasn't already been added
		int iAgent = m_lAgents.Find(Agent);
		if (iAgent == -1)
		{
			//Not added, set up agent info and add it
			Agent.m_InfoFlags = _Info;
			Agent.m_ValidFlags |= CAI_AgentInfo::INFOFLAGS_ALL;
			Agent.m_Awareness = _Awareness;
			Agent.m_AwarenessOfUs = _AwarenessOfUs;
			Agent.m_AwarenessTimer = m_pAI->m_Timer;

			if (_Relation <= CAI_AgentInfo::UNKNOWN)
			{
				Agent.SetRelation(DefaultRelation(_iObj));
				//Check if friend by default
				if (Agent.m_Relation == CAI_AgentInfo::FRIENDLY)
				{
					if (Agent.m_Awareness <= CAI_AgentInfo::NONE)
					{
						Agent.m_Awareness = CAI_AgentInfo::SPOTTED;
						m_pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_PLAYER, _iObj);
					}
				}
			}
			else
			{
				Agent.SetRelation(_Relation);
			}

			//Add to list
			int iIndex = m_lAgents.Add(Agent);

			//Add agent to hash
			m_AgentObjIDMap.Add(_iObj, iIndex);

			//Add agent list index to appropriate auxiliary lists
			if (IsPotentialEnemy(&Agent))
				m_lEnemies.Add(iIndex);

			return &(m_lAgents.Get(iIndex));
		}
		else
		{
			return &(m_lAgents.Get(iAgent));
		}
	}
	else
		return NULL;
};

// Returns the best agent
CAI_AgentInfo* CAI_KnowledgeBase::GetBestAwareness(int _MinRelation)
{
	int BestAwareness = CAI_AgentInfo::NONE;
	int BestIndex = -1;

	for (int i = 0; i < m_lAgents.Length(); i++)
	{
		if (!m_lAgents.IsValid(i)) {continue;}

		int Relation = m_lAgents.Get(i).GetRelation();
		int Awareness = m_lAgents.Get(i).GetAwareness();
		if (((Relation == CAI_AgentInfo::UNKNOWN)||(Relation >= _MinRelation))&&(Awareness >= CAI_AgentInfo::NOTICED))
		{
			if (Awareness > BestAwareness)
			{
				BestAwareness = Awareness;
				BestIndex = i;
			}
		}
	}

	if (BestIndex != -1)
	{
		return(&m_lAgents.Get(BestIndex));
	}
	else
	{
		return(NULL);
	}
}

//Check if given agent fulfil the given criteria.
bool CAI_KnowledgeBase::CheckAgent(int _iObj, const CAI_AgentCriteria& _Criteria)
{
	//Get agent index
	int iIndex = m_AgentObjIDMap.Get(_iObj);

	//Fail on invalid index
	if (!m_lAgents.IsValid(iIndex))
		return false;
	else
		//Check agent
		return m_lAgents.Get(iIndex).Check(_Criteria);
};


fp32 CAI_KnowledgeBase::CanSpeakTeammate(fp32 _MaxRange)
{
	fp32 RangeSqr;
	int iNearMate = m_pAI->GetClosestTeammember(true,0,&RangeSqr);
	if ((iNearMate == 0)||(RangeSqr > Sqr(_MaxRange)))
	{
		return(0.0f);
	}
	else
	{
		return(M_Sqrt(RangeSqr));
	}
};

//Find first agent that fulfil the given criteria. Returns NULL if no such agent can be found.
CWObject * CAI_KnowledgeBase::FindAgent(const CAI_AgentCriteria& _Criteria)
{
	if (!m_pAI || !m_pAI->m_pServer)
		return NULL;

	//Get the list to iterate over
	int List = CAgentIteration::RecommendedList(_Criteria);

	//Iterate through the chosen list until an agent that fulfils the criteria is found or 
	//we run out of agents
	int iAgent;
	for (int i = 0; i < ListLength(List); i++)
	{
		//Get index into agents list
		switch (List)
		{
		case CAgentIteration::ENEMIES:
			iAgent = m_lEnemies.Get(i);
			break;
		default:
			iAgent = i;
		}

		//Check current agent if valid
		if (m_lAgents.IsValid(iAgent) &&
			m_lAgents.Get(iAgent).Check(_Criteria))
		{
			//Agent fulfilling criteria found!
			CWObject * pAgent = m_pAI->IsValidAgent(m_lAgents.Get(iAgent).m_iObject);
			if (pAgent)
				return pAgent;
		}
	}

	//Couldn't find any agent fulfilling the criteria
	return NULL;
};


//Set up agent iteration with given criteria, for use with the FindNextAgent method
//Returns iteration id.
int CAI_KnowledgeBase::BeginAgentIteration(const CAI_AgentCriteria& _Criteria)
{
	CAgentIteration Iter;
	Iter.m_iIndex = -2; //To make Iter non-null (otherwise adding to list will fail)
	int iIndex = m_lAgentIterations.Add(Iter);
	m_lAgentIterations.Get(iIndex).m_iIndex	= iIndex;		
	m_lAgentIterations.Get(iIndex).m_Criteria = _Criteria;
	m_lAgentIterations.Get(iIndex).m_List = CAgentIteration::RecommendedList(_Criteria);
	m_lAgentIterations.Get(iIndex).m_Cur = 0;

	return iIndex;
};


//Remove given agent iteration. Should be called when iteration has been finished.
void CAI_KnowledgeBase::EndAgentIteration(int _Iteration)
{
	m_lAgentIterations.Remove(_Iteration);
};


//Find the next agent that fulfil the criteria of the given iteration, or NULL if 
//there are no more such agents
CWObject * CAI_KnowledgeBase::FindNextAgent(int _Iteration)
{
	if (!m_pAI || !m_pAI->m_pServer)
		return NULL;

	if (!m_lAgentIterations.IsValid(_Iteration))
		return NULL;

	CAgentIteration Iter = m_lAgentIterations.Get(_Iteration);

	//Iterate through the chosen list until an agent that fulfils the criteria is found or 
	//we run out of agents
	int iAgent;
	int i;
	for (i = Iter.m_Cur; i < ListLength(Iter.m_List); i++)
	{
		//Get index into agents list
		switch (Iter.m_List)
		{
		case CAgentIteration::ENEMIES:
			iAgent = m_lEnemies.Get(i);
			break;
		default:
			iAgent = i;
		}

		//Check current agent if valid
		if (m_lAgents.IsValid(iAgent) &&
			m_lAgents.Get(iAgent).Check(Iter.m_Criteria))
		{
			//Agent fulfilling criteria found!
			CWObject * pAgent = m_pAI->IsValidAgent(m_lAgents.Get(iAgent).m_iObject);
			if (pAgent)
			{
				//Update iteration so that next agent is agent after this
				Iter.m_Cur = i + 1;
				return pAgent;
			}
		}
	}

	//Couldn't find any agent fulfilling the criteria
	Iter.m_Cur = i + 1;
	return NULL;
};


//Find all agents with that fulfil the given criteria and add them to given list. Returns number of found agents.
int CAI_KnowledgeBase::FindAllAgents(const CAI_AgentCriteria& _Criteria, TArray<CWObject*> * _plRes)
{
	if (!_plRes || !m_pAI || !m_pAI->m_pServer)
		return 0;
	
	//Get the list to iterate over
	int List = CAgentIteration::RecommendedList(_Criteria);

	//Iterate through the chosen list until an agent that fulfils the criteria is found or 
	//we run out of agents
	int iAgent;
	int nAgents = 0;
	for (int i = 0; i < ListLength(List); i++)
	{
		//Get index into agents list
		switch (List)
		{
		case CAgentIteration::ENEMIES:
			iAgent = m_lEnemies.Get(i);
			break;
		default:
			iAgent = i;
		}

		//Check current agent if valid
		if (m_lAgents.IsValid(iAgent) &&
			m_lAgents.Get(iAgent).Check(_Criteria))
		{
			//Agent fulfilling criteria found!
			CWObject * pAgent = m_pAI->IsValidAgent(m_lAgents.Get(iAgent).m_iObject);
			if (pAgent)
			{
				nAgents++;
				_plRes->Add(pAgent);		
			};
		}
	}

	return nAgents;
};



//Get agent info for agent with given object id, or NULL if there is no such agent
CAI_AgentInfo* CAI_KnowledgeBase::GetAgentInfo(int _iObj)
{
	//Get agent index
	if (!_iObj) {return(NULL);}
	int iIndex = m_AgentObjIDMap.Get(_iObj);

	//Fail on invalid index
	if (!m_lAgents.IsValid(iIndex))
		return NULL;
	else
	{
		CAI_AgentInfo* pRes = &(m_lAgents.Get(iIndex));

		//This shouldn't be necessary since we check validity above, but Jesse got a crash here so...
		if (!pRes)
		{
			m_lAgents.Remove(iIndex);
			m_AgentObjIDMap.Remove(_iObj);
			return NULL;
		}
		//Fail on non-object (note that validity will not be checked, only existance of object to avoid destroy-related bugs)
		else if (!m_pAI->m_pServer->Object_Get(pRes->GetObjectID()))
		{
			//Update agent info to let it reset itself
			pRes->OnRefresh();
			m_lAgents.Remove(iIndex);
			m_AgentObjIDMap.Remove(_iObj);
			return NULL;
		}
		else
		{
			if(pRes->m_iObject)
				return pRes;
			else
				return pRes;
		}
	}
};

//Get number of valid and invalid agent infos
int CAI_KnowledgeBase::NumAgentInfo()
{
	return m_lAgents.Length();
};

//Get i'th agent info (0..GetNum-1)
CAI_AgentInfo * CAI_KnowledgeBase::IterateAgentInfo(int _i)
{
	if (!m_lAgents.IsValid(_i))
		return NULL;

	CAI_AgentInfo * pRes = &(m_lAgents.Get(_i));

	//Fail on non-object (note that validity will not be checked, only existance of object to avoid destroy-related bugs)
	if (!m_pAI->m_pServer->Object_Get(pRes->GetObjectID()))
	{
		//Update agent info to let it reset itself
		m_AgentObjIDMap.Remove(pRes->GetObjectID());
		pRes->OnRefresh();
		m_lAgents.Remove(_i);
		return NULL;
	}
	else
		return pRes;
};
	
//Get pointer to AI that this knowledge base belongs to
CAI_Core * CAI_KnowledgeBase::GetAI()
{
	return m_pAI;
};


/*
//Get the number of frames since the bot was last attacked. Return -1 if ever attacked.
int CAI_KnowledgeBase::GetAttackedCount()
{
	if (!m_pAI || !m_pAI->m_pServer)
		return -1;

	return (m_LastAttackedTick == -1) ? -1 : m_pAI->GetAITick() - m_LastAttackedTick;
};
*/

//Have we been attacked within the given number of frames?
bool CAI_KnowledgeBase::WasAttacked(int _Time)
{
	if (!m_pAI || !m_pAI->m_pServer)
		return false;

	if (GetLastHurtTicks() <= _Time)
	{
		return(true);
	}
	else
	{
		return(false);
	}
};

// rETURNS THE CURRENT NBR OF detected+ ENEMIES
int CAI_KnowledgeBase::GetDetectedEnemyCount()
{
	return(m_NEnemies);
};

// Used by actions to claer other users of m_iLastMeleeNonEnemyFighter
// We cannot clear through GetLastNonEnemeyMeleeFighter as the action may not be taken
// when GetLastNonEnemeyMeleeFighter returns and in that case we want other actions to respond 
void CAI_KnowledgeBase::ClearLastNonEnemeyMeleeFighter()
{
	m_iLastMeleeNonEnemyFighter = 0;
	m_LastMeleeNonEnemyFighterTimer = 0;
}

CAI_AgentInfo* CAI_KnowledgeBase::GetLastNonEnemeyMeleeFighter()
{
	if (m_iLastMeleeNonEnemyFighter)
	{
		CAI_AgentInfo* pAgent = GetAgentInfo(m_iLastMeleeNonEnemyFighter);
		// Agent was not known or nodody asked for him for 3 secs
		if ((!pAgent)||(m_pAI->m_Timer > m_LastMeleeNonEnemyFighterTimer + 60))
		{
			m_iLastMeleeNonEnemyFighter = 0;
			m_LastMeleeNonEnemyFighterTimer = 0;
			return(NULL);
		}
		else
		{
			return(pAgent);
		}
	}
	else
	{
		return(NULL);
	}
};

CAI_AgentInfo* CAI_KnowledgeBase::GetLastMeleeRelationIncrease()
{
	if (m_pAI->m_MeleeRelationIncrease)
	{
		CAI_AgentInfo* pAgent = GetLastNonEnemeyMeleeFighter();
		return(pAgent);
	}
	else
	{
		return(NULL);
	}
};

//Get agent info of last attacker or NULL if never attacked or last attacker is invalid (dead)
CAI_AgentInfo* CAI_KnowledgeBase::GetLastAttacker()
{
	if (m_iLastAttacker)
	{
		CAI_AgentInfo * pAgent = GetAgentInfo(m_iLastAttacker);
		if (!pAgent || !m_pAI->IsValidAgent(pAgent->GetObject()))
		{
			m_iLastAttacker = 0;
			return NULL;
		}
		return pAgent;
	}
	else
		return NULL;
};


//Handles the event of us taking damage
void CAI_KnowledgeBase::OnTakeDamage(int _iAttacker, int _Damage, uint32 _DamageType)
{
	if (!m_pAI || !m_pAI->m_pServer || !m_pAI->m_pGameObject)
		return;

	// We wake up pretty fast when hurt
	if (GetAlertness() <= ALERTNESS_SLEEPING)
	{
		SetAlertness(ALERTNESS_IDLE);
	}
	else if (GetAlertness() < ALERTNESS_WATCHFUL)
	{
		SetAlertness(ALERTNESS_WATCHFUL);
	}

	if (_Damage > 0)
	{
		UpdateLastHurtTick();
	}

	CWObject * pAgent = m_pAI->IsValidAgent(_iAttacker);
	if (pAgent)
	{
		//Get agent info about attacker
		CAI_AgentInfo* pInfo = GetAgentInfo(_iAttacker);
		if (!pInfo)
		{
			//Previously uninteresting agent becomes very interesting indeed when he attacks us!
			if (DefaultRelation(_iAttacker) != CAI_AgentInfo::FRIENDLY)
			{
				//Additional info etc will be set below.
				pInfo = AddAgent(_iAttacker);
			}
		}

		//Is the attacker a potential enemy?
		if (pInfo && (pInfo->GetCurRelation() != CAI_AgentInfo::FRIENDLY))
		{
			int CurRelation = pInfo->GetCurRelation();
			if ((m_pAI->m_bHostileFromBlowDmg)&&(_DamageType & DAMAGETYPE_BLOW))
			{
				if (CurRelation < CAI_AgentInfo::HOSTILE_2)
				{
					if (CurRelation < CAI_AgentInfo::HOSTILE)
					{
						m_pAI->RaiseAlarm(_iAttacker,CAI_AgentInfo::HOSTILE,true);
					}
					pInfo->SetRelation(CurRelation+1);
				}
			}
			else
			{
				//Agent is most certainly an enemy, and attacking us
				if (CurRelation < CAI_AgentInfo::ENEMY)
				{
					m_pAI->RaiseAlarm(_iAttacker,CAI_AgentInfo::ENEMY,true);
					pInfo->SetRelation(CAI_AgentInfo::ENEMY);
				}
				m_pAI->SwitchTarget(_iAttacker,true);
				pInfo->SetCorrectSuspectedPosition();
			}

			pInfo->SetInfo(CAI_AgentInfo::IS_ATTACKING_US, true);

			//We were attacked by a character, check which one it was, or add him to character list
			//if not already added, and set him as spotted
			if (_DamageType & DAMAGETYPE_BLAST)
			{	// Just NOTICE the attacker if unaware
				if (pInfo->GetAwareness() <= CAI_AgentInfo::DETECTED)
				{
					//A previously non-spotted enemy is automatically spotted.
					pInfo->SetAwareness(CAI_AgentInfo::DETECTED,true,true);	
				}
				pInfo->TouchLastSpotted();
			}
			else
			{	// Spot the attacker
				if (pInfo->GetAwareness() < CAI_AgentInfo::SPOTTED)
				{
					//A previously non-spotted enemy is automatically spotted.
					pInfo->SetAwareness(CAI_AgentInfo::SPOTTED,true,true);	
				}
				pInfo->TouchLastSpotted();
			}

			//Only count enemies as attackers
			m_iLastAttacker	= _iAttacker;
		}
		else
		{
			// Play some kind of friendly fire sound?
		}
	}
};

//Are we a player ally?
bool CAI_KnowledgeBase::IsPlayerAlly()
{
	if (!m_pAI || !m_pAI->m_pServer)
		return false;

	if (!(m_ValidKnowledge & PLAYER_ALLY))
	{
		//Update knowledge
		m_ValidKnowledge |= PLAYER_ALLY;
		m_Knowledge &= ~PLAYER_ALLY;	
		CWObject_Game *pGame = m_pAI->m_pServer->Game_GetObject();
		for (int i = 0; i < pGame->Player_GetNum(); i++)
		{
			if (DefaultRelation(pGame->Player_GetObjectIndex(i)) == CAI_AgentInfo::FRIENDLY)
			{
				m_Knowledge |= PLAYER_ALLY;	
				break;
			}
		}
	}

	return (m_Knowledge & PLAYER_ALLY) != 0;
};


//Updates relation for all potential enemies and awareness for all enemy. Succeeds if 
//there was any spotted enemies
bool CAI_KnowledgeBase::OnSpotEnemies()
{
	if (!(m_ValidKnowledge & ENEMY_SPOTTED))
	{
		//Update info
		m_ValidKnowledge |= ENEMY_SPOTTED;

		//Check awareness and relation of interesting agents
		bool bSpotted = false;
		for (int i = 0; i < m_lAgents.Length(); i++)
		{
			if (m_lAgents.IsValid(i))
			{
				//Update relation
				if (m_lAgents.Get(i).GetCurRelation() >= CAI_AgentInfo::ENEMY)
				{
					//Update awareness
					if (m_lAgents.Get(i).GetCurAwareness() >= CAI_AgentInfo::SPOTTED)
						bSpotted = true;
				}
			}
		}

		if (bSpotted)
			m_Knowledge |= ENEMY_SPOTTED;
		else
			m_Knowledge &= ~ENEMY_SPOTTED;
	}
	
	return (m_Knowledge & ENEMY_SPOTTED) != 0;
};

// Ditto for detected enemies, used by FindTarget() etc
bool CAI_KnowledgeBase::OnDetectEnemies()
{
	if (!(m_ValidKnowledge & ENEMY_DETECTED))
	{
		//Update info
		m_ValidKnowledge |= ENEMY_DETECTED;

		//Check awareness and relation of interesting agents
		bool bSpotted = false;
		for (int i = 0; i < m_lAgents.Length(); i++)
		{
			if (m_lAgents.IsValid(i))
			{
				//Update relation
				if (m_lAgents.Get(i).GetCurRelation() >= CAI_AgentInfo::ENEMY)
				{
					//Update awareness
					if (m_lAgents.Get(i).GetCurAwareness() >= CAI_AgentInfo::DETECTED)
						bSpotted = true;
				}
			}
		}

		if (bSpotted)
			m_Knowledge |= ENEMY_DETECTED;
		else
			m_Knowledge &= ~ENEMY_DETECTED;
	}

	return (m_Knowledge & ENEMY_DETECTED) != 0;
};


//Our default relation with this agent (Value is one of agent info relation enum)
int CAI_KnowledgeBase::DefaultRelation(int _iObj)
{
	if (!m_pAI || !m_pAI->m_pServer)
		return CAI_AgentInfo::RELATION_INVALID;

	//Default relation is worst relation any of our teams have with any of given object's
	//teams, except friendly if at least one team is friendly to ours and worst relation
	//otherwise is lower than unfriendly. Neutral if there are no such relations
	int WorstRelation = CAI_AgentInfo::RELATION_INVALID;
	bool bFriend = false;
	uint16 liTeams[32];
	uint nTeams = m_pAI->m_pGameObject->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
	for (int i = 0; i < nTeams; i++)
	{
		int iTeam = liTeams[i];			
		if (iTeam)
		{
			int Relation = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_TEAM_GETRELATION, _iObj), iTeam);
			if (Relation > WorstRelation)
			{
				WorstRelation = Relation;
			}
			if (Relation == CAI_AgentInfo::FRIENDLY)
			{
				bFriend = true;
			}
		}
	}

	if (WorstRelation == CAI_AgentInfo::RELATION_INVALID)
	{
		return CAI_AgentInfo::NEUTRAL;
	}
	else if (bFriend && (WorstRelation <= CAI_AgentInfo::UNFRIENDLY))
	{	// Love conquers all!
		return CAI_AgentInfo::FRIENDLY;
	}
	else
	{
		return WorstRelation;
	}
};


//Set/get alertness state
void CAI_KnowledgeBase::SetAlertness(int _Alertness, bool _bDefault)
{
	if ((_Alertness > ALERTNESS_INVALID) && (_Alertness <= ALERTNESS_JUMPY))
	{
		m_Alertness = _Alertness;
		if (_bDefault)
			m_DefaultAlertness = _Alertness;
	}
};
void CAI_KnowledgeBase::SetAlertness(CStr _Alertness, bool _bDefault)
{
	SetAlertness(_Alertness.TranslateInt(ms_TranslateAlertness), _bDefault);
};

//Get alertness level, or default alertness level if the flag is set
int CAI_KnowledgeBase::GetAlertness(bool _bDefaultValue)
{
	if (_bDefaultValue)
		return m_DefaultAlertness;
	else
		return m_Alertness;
};

void CAI_KnowledgeBase::SetInitialAlertness(CStr _Alertness)
{
	int Alertness = _Alertness.TranslateInt(CAI_KnowledgeBase::ms_TranslateAlertness);
	if((Alertness > ALERTNESS_INVALID)&& (Alertness <= ALERTNESS_JUMPY))
	{
		m_InitialAlertness = Alertness;
	}
};
int CAI_KnowledgeBase::GetInitialAlertness()
{
	return m_InitialAlertness;
};

//Set alertness range
void CAI_KnowledgeBase::SetAlertnessRange(fp32 _Range)
{
	m_AlertnessRangeSqr = Sqr(_Range);
};
fp32 CAI_KnowledgeBase::GetAlertnessRangeSqr()
{
	return m_AlertnessRangeSqr;
};

//Update alertness level
void CAI_KnowledgeBase::OnRefreshAlertness()
{
	MSCOPESHORT(CAI_KnowledgeBase::OnRefreshAlertness);
	if (!m_pAI || !m_pAI->m_pServer)
		return;

	//Alertness is implicitly raised to jumpy if entire level has become "lights out"
	CWObject_Message LightsOut(OBJMSG_GAME_GETLIGHTSOUT);
	if (m_pAI->m_pServer->Message_SendToObject(LightsOut, m_pAI->m_pServer->Game_GetObjectIndex()))
	{
		if (m_Alertness < ALERTNESS_JUMPY)
		{
			SetAlertness(ALERTNESS_JUMPY);
		}
	}
	else if (m_Alertness > m_DefaultAlertness)
	{
		if (m_Alertness >= ALERTNESS_JUMPY)
		{
			SetAlertness(ALERTNESS_WATCHFUL);
		}
	}
};


//Handle alarm caused by given object and raising object at given alarm level (see CWObject_Team)
void CAI_KnowledgeBase::OnAlarm(int _iCause, int _iRaiser, int _Relation)
{
	if (!m_pAI || !m_pAI->m_pServer || !m_pAI->m_pGameObject || m_pAI->m_StunTimeout || m_pAI->m_Script.IsPaused())
		return;

	if ((m_pAI->m_PauseActions)||(_iRaiser == m_pAI->m_pGameObject->m_iObject)||(_iCause == m_pAI->m_pGameObject->m_iObject))
	{
		return;
	}

	if (_Relation == CAI_AgentInfo::ENEMY)
	{
		m_pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_ALARM,_iCause);
	}
	
	//A valid agent who cause an alarm is automatically interesting
	CAI_AgentInfo* pInfo;
	if (m_pAI->IsValidAgent(_iCause))
	{
		pInfo = GetAgentInfo(_iCause);
		if (!pInfo)
		{	//Character is not in char list, try to add him
			pInfo = AddAgent(_iCause);
		}
	}
	else
	{
		pInfo = NULL;
	}

	//Raise alertness to watchful
	if (GetAlertness() < ALERTNESS_WATCHFUL)
	{
		SetAlertness(ALERTNESS_WATCHFUL);
	}

	//Set info/create phenomena based on alarm level
	if (pInfo && pInfo->GetObject())
	{
		//If we're friendly with raiser, set relation if such is given and worse than current relation
		CAI_AgentInfo* pInfoRaiser = GetAgentInfo(_iRaiser);
		if (pInfoRaiser == NULL)
		{
			pInfoRaiser = AddAgent(_iRaiser);
		}

		bool bNewRelation = true;
		int Relation = pInfoRaiser->GetCurRelation();
		if (pInfoRaiser)
		{
			if ((Relation == CAI_AgentInfo::FRIENDLY)||(DefaultRelation(_iRaiser) == CAI_AgentInfo::FRIENDLY))
			{
				if ((Relation > CAI_AgentInfo::FRIENDLY) && (Relation < _Relation))
				{
					pInfo->SetRelation(_Relation);
				}
				else
				{
					bNewRelation = false;
				}
			}
		}
			
		//Set other info based on level
		Relation = pInfoRaiser->GetCurRelation();
		int Awareness = pInfo->GetAwareness();
		int Tension = m_pAI->GetStealthTension();
		if (Awareness < CAI_AgentInfo::DETECTED)
		{	// We somehow magically know where the enemy now is!
			pInfo->SetCorrectSuspectedPosition();
			pInfo->SetAwareness(CAI_AgentInfo::DETECTED,true);
		}
		else if (Awareness == CAI_AgentInfo::DETECTED)
		{	// Reset any degrading of awareness
			pInfo->Touch();
		}

		if ((Tension < CAI_Core::TENSION_COMBAT)&&(_Relation >= CAI_AgentInfo::ENEMY))
		{
			m_pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
			m_pAI->UseRandom("OnAlarm",CAI_Device_Sound::COMBAT_AFFIRMATIVE,CAI_Action::PRIO_COMBAT);
		}

		// New relation Hostile or worse should stop any ongoing behaviours
		if ((bNewRelation)&&(Relation >= CAI_AgentInfo::HOSTILE)&&(Awareness >= CAI_AgentInfo::DETECTED))
		{
			m_pAI->ShutUp();
			m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_IRRITATED,60);
		}
	}
};

//Determine wether our bot can see iCause and if so, set relation to _Relation
void CAI_KnowledgeBase::OnFight(int _iCause, int _iRaiser, int _Relation)
{
	if (!m_pAI || !m_pAI->m_pServer || !m_pAI->m_pGameObject)
		return;

	if (m_pAI->m_PauseActions)
	{
		return;
	}

	// If we are either the perp or the victim we're too busy fighting to bother
	if ((_iRaiser == m_pAI->m_pGameObject->m_iObject)||
		(_iCause == m_pAI->m_pGameObject->m_iObject)||
		(!m_pAI->IsValidAgent(_iCause)))
	{
		return;
	}

	// Before anything else, we must determine wether we can see _iCause or not

	// Bots that beat up/murder our freind are clearly interesting to us
	CAI_AgentInfo* pPerp = GetAgentInfo(_iCause);
	if (!pPerp)
	{
		//Character is not in char list, try to add him
		pPerp = AddAgent(_iCause);
	}
	
	CAI_AgentInfo* pVictim = GetAgentInfo(_iRaiser);
	if (pVictim == NULL)
	{
		pVictim = AddAgent(_iRaiser);
	}

	// *** NOTE: We should check for detection of pVictim as well when the lightmeter
	// for guards is working ***
	if ((pPerp)&&(pPerp->GetAwareness() >= CAI_AgentInfo::DETECTED)&&(pPerp->InLOS()))
	{	// We see the bastard!
		// Raise alertness
		m_pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_SPOT_FIGHT,_iCause);
		SetAlertness(ALERTNESS_WATCHFUL);

		if (pVictim)
		{	//If we're friendly with raiser, set relation if such is given and worse than current relation
			if ((pVictim->GetRelation() == CAI_AgentInfo::FRIENDLY)||
				(DefaultRelation(_iRaiser) == CAI_AgentInfo::FRIENDLY))
			{
				if ((pPerp)&&(_Relation > CAI_AgentInfo::UNKNOWN)&&
					(_Relation > pPerp->GetCurRelation()))
				{
					pPerp->SetRelation(_Relation);
				}
			}
		}
	}
};

//Handle NOTICE,DETECT or SPOT caused by _iCause and sent by _iRaiser
void CAI_KnowledgeBase::OnAware(int _iCause, int _iRaiser, int _Awareness,CVec3Dfp32 _SuspectedPos,int _Relation)
{
	if (!m_pAI || !m_pAI->m_pServer || !m_pAI->m_pGameObject)
		return;

	if (m_pAI->m_PauseActions)
	{
		return;
	}

	//Only handle alarms from other than self
	if ((_iRaiser != m_pAI->m_pGameObject->m_iObject)||
		(_iCause != m_pAI->m_pGameObject->m_iObject))
	{	//A valid agent raise awareness is automatically interesting
		//He belongs our team with friendly internal relation
		CAI_AgentInfo * pInfo;
		if (m_pAI->IsValidAgent(_iCause))
		{
			pInfo = GetAgentInfo(_iCause);
			if (!pInfo)
			{
				//Character is not in char list, try to add him
				pInfo = AddAgent(_iCause);
			}
		}
		else
		{
			pInfo = NULL;
		}

		//Set info/create phenomena based on alarm level
		if (pInfo && pInfo->GetObject())
		{
			//Set other info based on level
			int Awareness = pInfo->GetAwareness();
			if (_Awareness > Awareness)
			{
				// We assume the pos is better as the awareness was better
				if (_Awareness >= CAI_AgentInfo::SPOTTED)
				{	// He knew exactly where the target was, we know too
					pInfo->SetCorrectSuspectedPosition();
				}
				else
				{
					pInfo->SetSuspectedPosition(_SuspectedPos);
				}
				// *** Magical insta awareness
				pInfo->SetAwareness(_Awareness,false,true);
				int CurRelation = pInfo->GetCurRelation();
				if ((CurRelation > CAI_AgentInfo::FRIENDLY)&&(CurRelation < _Relation))
				{	// *** Magical insta relation
					pInfo->SetRelation(_Relation);
				}
			}
			else
			{
				int CurRelation = pInfo->GetCurRelation();
				if ((CurRelation > CAI_AgentInfo::FRIENDLY) && (CurRelation < _Relation))
				{	// *** Magical insta relation
					pInfo->SetRelation(_Relation);
				}
			}
			// Don't raise here as it will lead to too many OnAware calls
		}
	}
};

// =============================================================================================
// Methods related to broken lights
// =============================================================================================
void CAI_KnowledgeBase::Global_SetCareAboutBrokenLights()
{
	if ((!m_pAI)||(!m_pAI->m_pGameObject)) {return;}

	TArray<int> &liInterestedInBrokenLights = m_pAI->m_pAIResources->m_KBGlobals.ms_liInterestedInBrokenLights;
	for (int i = 0; i < liInterestedInBrokenLights.Len(); i++)
	{
		if (liInterestedInBrokenLights[i] == m_pAI->m_pGameObject->m_iObject)
		{
			return;
		}
	}
	liInterestedInBrokenLights.Add(m_pAI->m_pGameObject->m_iObject);
};

void CAI_KnowledgeBase::Global_ReportBrokenLight(int _iLight)
{
	if (!m_pAI)
	{
		return;
	}

	TArray<int> &liInterestedInBrokenLights = m_pAI->m_pAIResources->m_KBGlobals.ms_liInterestedInBrokenLights;
	// Report the broken light to every eligible bot
	for (int i = 0; i < liInterestedInBrokenLights.Len(); i++)
	{
		int iObj = liInterestedInBrokenLights[i];
		CAI_Core* pAI = m_pAI->GetAI(iObj);
		if (!pAI) {continue;}
		pAI->m_KB.ReportBrokenLight(_iLight);
	}
};

void CAI_KnowledgeBase::ReportBrokenLight(int _iLight)
{
	for (int i = 0; i < m_liBrokenLights.Len(); i++)
	{
		if (m_liBrokenLights[i] == _iLight)
		{
			return;
		}
	}
	m_liBrokenLights.Add(_iLight);
};

CVec3Dfp32 CAI_KnowledgeBase::GetBrokenLightPos(bool _bRemove)
{
	// Find any broken light that we might be influenced by, return the ground position beneath it
	// and remove it from the list if _bRemove is true
	if (m_liBrokenLights.Len() == 0)
	{
		return(CVec3Dfp32(_FP32_MAX));
	}

	int rnd = (int)(m_liBrokenLights.Len() * Random * 0.999f);
	int iLight = m_liBrokenLights[rnd];
	CWObject* pObj = m_pAI->m_pServer->Object_Get(iLight);
	if ((pObj)&&(m_pAI->m_pServer))
	{
		CVec3Dfp32 LightPos = pObj->GetPosition();
		//We must be airborne or something, check up and down and hope we find a good spot.
		CVec3Dfp32 NoticePos = m_pAI->m_PathFinder.GetPathPosition(LightPos,20,0);
		if (INVALID_POS(NoticePos))
		{	//We might be in a corner or other inaccessible spot, try a little wider
			NoticePos = m_pAI->m_PathFinder.GetPathPosition(LightPos,5,5);
		}
		if ((m_pAI->DebugRender())&&(m_pAI->m_pServer))
		{
			m_pAI->m_pServer->Debug_RenderVertex(NoticePos+CVec3Dfp32(0,0,10),0xff0000ff,10,true);
		}
		m_liBrokenLights.Del(rnd);

		return(NoticePos);
	}

	return(CVec3Dfp32(_FP32_MAX));
};

// =============================================================================================
// Methods related to dead bots and how we behave vs them
// =============================================================================================
void CAI_KnowledgeBase::Global_SetCareAboutDeath(int _Relation)
{
	if ((!m_pAI)||(!m_pAI->m_pGameObject)||(!m_pAI->m_pAIResources)||
		(_Relation == CAI_AgentInfo::RELATION_INVALID))
	{
		return;
	}

	TArray<SDead> &lDead = m_pAI->m_pAIResources->m_KBGlobals.ms_lDead;
	TArray<int> &liInterestedInDeath = m_pAI->m_pAIResources->m_KBGlobals.ms_liInterestedInDeath;
	TArray<int> &lInterestedInDeathRelations = m_pAI->m_pAIResources->m_KBGlobals.ms_lInterestedInDeathRelations;
	for (int i = 0; i < liInterestedInDeath.Len(); i++)
	{
		if (liInterestedInDeath[i] == m_pAI->m_pGameObject->m_iObject)
		{	// We found a previous match, now we must harmonize the previours relation with the new
			// to report as few unwanted deaths as possible
			int prevRelation = lInterestedInDeathRelations[i];
			if ((_Relation == CAI_AgentInfo::UNKNOWN)||(prevRelation == CAI_AgentInfo::UNKNOWN))
			{	// We are interested in all relations
				lInterestedInDeathRelations[i] = CAI_AgentInfo::UNKNOWN;
			}
			else if ((_Relation < CAI_AgentInfo::NEUTRAL)&&(prevRelation > CAI_AgentInfo::NEUTRAL))
			{	// We're interested in both above and below NEUTRAL ie all but NEUTRAL
				lInterestedInDeathRelations[i] = CAI_AgentInfo::UNKNOWN;
			}
			else if ((_Relation > CAI_AgentInfo::NEUTRAL)&&(prevRelation < CAI_AgentInfo::NEUTRAL))
			{	// We're interested in both above and below NEUTRAL ie all but NEUTRAL
				lInterestedInDeathRelations[i] = CAI_AgentInfo::UNKNOWN;
			}
			else
			{	// Both _Relation and prevRelation > NEUTRAL
				lInterestedInDeathRelations[i] = Min(_Relation,prevRelation);
			}
			// Done
			return;
		}
	}
	// No match was found, add the new one
	liInterestedInDeath.Add(m_pAI->m_pGameObject->m_iObject);
	lInterestedInDeathRelations.Add(_Relation);

	//No, you give him a list of all dead people
	// Report all previous deaths
	for (int i = 0; i < lDead.Len(); i++)
	{
		Global_ReportDeath(lDead[i].m_iCause,lDead[i].m_iVictim,lDead[i].m_DeathPos,lDead[i].m_CauseOfDeath);
	}

	// We iterate through all characters and report any dead ones
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pAI->m_pServer->Selection_AddClass(Selection, "CharPlayer");
	m_pAI->m_pServer->Selection_AddClass(Selection, "CharNPC");
	const int16* pSel = NULL;
	int nSel = m_pAI->m_pServer->Selection_Get(Selection, &pSel);

	// Iterate through 'em all (this shure will generate a spike :)
	for (int k = 0; k < nSel; k++)
	{
		int iCur = pSel[k];
		CAI_Core* pAICur = m_pAI->GetAI(pSel[k]);
		if ((pAICur)&&(pAICur->m_pGameObject)&&
			(pAICur->m_pGameObject->AI_IsSpawned())&&
			(!pAICur->m_pGameObject->AI_IsAlive())&&
			(CWObject_Character::Char_GetPhysType(pAICur->m_pGameObject) != PLAYER_PHYS_NOCLIP))
		{
			CVec3Dfp32 Pos = pAICur->m_pGameObject->GetPosition();
			ReportDeath(0,iCur,Pos,SDead::DEATHCAUSE_UNKNOWN);
		}
	}
};

void CAI_KnowledgeBase::Global_RemoveCareAboutDeath(int _iObj)
{
	TArray<int> &liInterestedInDeath = m_pAI->m_pAIResources->m_KBGlobals.ms_liInterestedInDeath;
	TArray<int> &lInterestedInDeathRelations = m_pAI->m_pAIResources->m_KBGlobals.ms_lInterestedInDeathRelations;
	for (int i = 0; i < liInterestedInDeath.Len(); i++)
	{
		if (liInterestedInDeath[i] == _iObj)
		{
			liInterestedInDeath.Del(i);
			lInterestedInDeathRelations.Del(i);
			return;
		}
	}
};

void CAI_KnowledgeBase::Global_ReportDeath(int _iCause,int _iVictim,CVec3Dfp32 _Pos,int8 _CauseOfDeath)
{
	if ((!m_pAI)||(!m_pAI->m_pAIResources))
	{
		return;
	}

	if (_iVictim == 0)
	{
		return;
	}

	// Check next of kin first!
	CAI_Core* pDeadAI = m_pAI->GetAI(_iVictim);
	if ((pDeadAI)&&(!pDeadAI->m_bNextOfKin))
	{
		Global_RemoveCareAboutDeath(_iVictim);
		return;
	}

	// Remove the victim (he/she should no longer be interested in death reports
	Global_RemoveCareAboutDeath(_iVictim);
	
	// We add the corpse to the global list for late arrivers
	SDead Dead;
	Dead.m_bFound = false;
	Dead.m_bInvestigated = false;
	Dead.m_CauseOfDeath = _CauseOfDeath;
	Dead.m_iCause = _iCause;
	Dead.m_iVictim = _iVictim;
	Dead.m_DeathPos = _Pos;

	TArray<SDead> &lDead = m_pAI->m_pAIResources->m_KBGlobals.ms_lDead;
	TArray<int> &liInterestedInDeath = m_pAI->m_pAIResources->m_KBGlobals.ms_liInterestedInDeath;
	TArray<int> &lInterestedInDeathRelations = m_pAI->m_pAIResources->m_KBGlobals.ms_lInterestedInDeathRelations;

	int iDuplicate = -1;
	for (int i = 0; i < lDead.Len(); i++)
	{
		if (Dead.m_iVictim == lDead[i].m_iVictim)
		{
			iDuplicate = i;
			break;
		}
	}
	if (iDuplicate != -1)
	{	// Check move distance
		if (_CauseOfDeath == SDead::DEATHCAUSE_MOVED)
		{	// Change cause of death to SDead::DEATHCAUSE_MOVED if moved at least 48 units
			if ((_Pos != CVec3Dfp32(_FP32_MAX))&&
				(lDead[iDuplicate].m_DeathPos != CVec3Dfp32(_FP32_MAX)))

			{	// Did he drag it long enough?
				if (_Pos.DistanceSqr(lDead[iDuplicate].m_DeathPos) >= Sqr(48.0f))
				{	// Yes
					lDead[iDuplicate].m_CauseOfDeath = SDead::DEATHCAUSE_MOVED;
				}
				else
				{	// No, nothing to report
					return;
				}
			}
		}
		Dead = lDead[iDuplicate];
	}
	else
	{	
		lDead.Add(Dead);
	}

	// Report the death to every eligible bot
	for (int i = 0; i < liInterestedInDeath.Len(); i++)
	{
		int iObj = liInterestedInDeath[i];
		if (iObj == _iCause)
		{
			continue;
		}
		CAI_Core* pAI = m_pAI->GetAI(iObj);
		if (!pAI) {continue;}
		if (lInterestedInDeathRelations[i] == CAI_AgentInfo::UNKNOWN)
		{
			pAI->m_KB.ReportDeath(Dead);
			continue;
		}

		// Get the _iVictim agent
		CAI_AgentInfo* pVictimAgent = pAI->m_KB.GetAgentInfo(_iVictim);
		if (!pVictimAgent)
		{	// We had to add him and sort out relations later
			pAI->m_KB.ReportDeath(Dead);
			continue;
		}

		int Relation = pVictimAgent->GetRelation(true);
		int MatchRelation = lInterestedInDeathRelations[i];
		switch(MatchRelation)
		{
		case CAI_AgentInfo::UNKNOWN:
			pAI->m_KB.ReportDeath(Dead);
			break;

		case CAI_AgentInfo::FRIENDLY:
			if (Relation == CAI_AgentInfo::FRIENDLY)
			{
				pAI->m_KB.ReportDeath(Dead);
			}
			break;

		case CAI_AgentInfo::NEUTRAL:
			if (Relation != CAI_AgentInfo::NEUTRAL)
			{
				pAI->m_KB.ReportDeath(Dead);
			}
			break;

		case CAI_AgentInfo::UNFRIENDLY:
		case CAI_AgentInfo::HOSTILE:
		case CAI_AgentInfo::HOSTILE_1:
		case CAI_AgentInfo::HOSTILE_2:
		case CAI_AgentInfo::ENEMY:
			if (Relation >= lInterestedInDeathRelations[i])
			{
				pAI->m_KB.ReportDeath(Dead);
			}
			break;

		default:
			break;
		}
	}
};

void CAI_KnowledgeBase::Global_RemoveDead(int _iCorpse)
{
	if ((!m_pAI)||(!m_pAI->m_pAIResources))
	{
		return;
	}

//	int iDuplicate = -1;
	TArray<SDead> &lDead = m_pAI->m_pAIResources->m_KBGlobals.ms_lDead;
	for (int i = lDead.Len()-1; i >= 0; i--)
	{
		if (lDead[i].m_iVictim == _iCorpse)
		{
			lDead.Del(i);
			break;
		}
	}

	TArray<int> &liInterestedInDeath = m_pAI->m_pAIResources->m_KBGlobals.ms_liInterestedInDeath;
	// Report the death removel to every eligible bot
	for (int i = 0; i < liInterestedInDeath.Len(); i++)
	{
		int iObj = liInterestedInDeath[i];
		CAI_Core* pAI = m_pAI->GetAI(iObj);
		if (!pAI) {continue;}
		pAI->m_KB.RemoveDead(_iCorpse);
	}
};

// Searches for a dead that fulfills the supplied search criteria
// _Relation: Relation to match:
// UNKNOWN: All are eligible
// FRIENDLY: Friendly only
// NEUTRAL: All but neutral
// UNFRIENDLY: Unfriendy or worse
// HOSTILE: Hostile or worse
// HOSTILE: Hostile or worse
// HOSTILE_1: Hostile 1 or worse
// HOSTILE_2: Hostile 2 or worse
// ENEMY: Enemy or worse if that is possible
// _MinRange,_MaxRange: Range criteria from caller
// _bUninvestigated: Only uninvestigated are considered when this is true
// _iStart: Where to begin searching.
// returns true when a valid dead is found (returned in _pDead if nonnull)
bool CAI_KnowledgeBase::GetFirstMatchingDead(SDead* _pDead,int _Relation,fp32 _MinRange,fp32 _MaxRange,bool _bUninvestigated,bool _bUnFound)
{
	SDead curDead;

	if ((_bUnFound)&&(NbrOfValidDead() < 1))
	{
		return(false);
	}
	int N = m_lDead.Len();
	if (N < 1)
	{
		return(false);
	}

	for (int i = 0; i < N; i++)
	{
		bool bFound = false;

		int iCur = (i + m_iDeadFinger) % N;
		curDead = m_lDead[iCur];
		if (_bUnFound)
		{
			if (curDead.m_bFound)
			{
				continue;
			}
		}
		else
		{
			if (!curDead.m_bFound)
			{
				continue;
			}
		}

		// Must be within ranges
		if ((m_pAI->SqrDistanceToUs(curDead.m_iVictim) < Sqr(_MinRange))||
			(m_pAI->SqrDistanceToUs(curDead.m_iVictim) > Sqr(_MaxRange)))
		{
			continue;
		}

		// Must be uninvestigated if _bUninvestigated is true
		if (_bUninvestigated)
		{
			if (curDead.m_bInvestigated == true)
			{
				continue;
			}
		}

		// _Relation must match the current relation as outlined above
		if (_Relation == CAI_AgentInfo::UNKNOWN)
		{	// We've found a match
			bFound = true;
		}
		else
		{	
			switch(_Relation)
			{
			case CAI_AgentInfo::FRIENDLY:
				// Only FRIENDLY are OK
				if (curDead.m_Relation == CAI_AgentInfo::FRIENDLY)
				{
					bFound = true;
				}
				break;

			case CAI_AgentInfo::NEUTRAL:
				// Anyone but NEUTRAL are OK
				if (curDead.m_Relation != CAI_AgentInfo::NEUTRAL)
				{
					bFound = true;
				}
				break;

			case CAI_AgentInfo::UNFRIENDLY:
			case CAI_AgentInfo::HOSTILE:
			case CAI_AgentInfo::HOSTILE_1:
			case CAI_AgentInfo::HOSTILE_2:
			case CAI_AgentInfo::ENEMY:
				// Equal or worse relation
				if (curDead.m_Relation >= _Relation)
				{
					bFound = true;
				}
				break;

			default:
				// wtf!
				continue;
				break;
			}
		}

		if (bFound)
		{	// Returns curDead

			CAI_Core* pDeadAI = m_pAI->GetAI(curDead.m_iVictim);
			if ((pDeadAI)&&(pDeadAI->m_pGameObject))
			{
				if ((!pDeadAI->m_pGameObject->AI_IsSpawned())||(pDeadAI->m_pGameObject->AI_IsAlive()))
				{
					m_lDead.Del(iCur);
					ConOut("CAI_KnowledgeBase::GetFirstMatchingDead found dead with no m_iVictim");
					m_iDeadFinger = (m_iDeadFinger+1) % N;
					return(false);
				}
			}

			if (_pDead)
			{
				if (INVALID_POS(curDead.m_DeathPos))
				{
					CWObject* pObj = m_pAI->m_pServer->Object_Get(curDead.m_iVictim);
					if (pObj)
					{
						curDead.m_DeathPos = pObj->GetPosition();
					}
					else
					{	// Bad, we should remove the dead as it has no corresponding m_iVictim
						m_lDead.Del(iCur);
						ConOut("CAI_KnowledgeBase::GetFirstMatchingDead found dead with no m_iVictim");
						m_iDeadFinger = (m_iDeadFinger+1) % N;
						return(false);
					}
				}
				*_pDead = curDead;
			}
			m_iDeadFinger = (m_iDeadFinger+1) % N;
			return(true);
		}
	}

	m_iDeadFinger = (m_iDeadFinger+1) % N;
	return(false);
};

// _Relation: Relation to match:
// UNKNOWN: All are eligible
// FRIENDLY: Friendly only
// NEUTRAL: All but neutral
// UNFRIENDLY+ Given relation or worse
// HOSTILE: Hostile or worse
// HOSTILE: Hostile or worse
// HOSTILE_1: Hostile 1 or worse
// HOSTILE_2: Hostile 2 or worse
// ENEMY: Enemy or worse if that is possible
// Returns 0 if no corpse was detected
// Returns 1 if corpse was found but no murderer
// Returns 2 if corpse and murderer was found (_pDead->m_iCause is the murderer)
int CAI_KnowledgeBase::CheckDead(SDead* _pDead,int _Relation,fp32 _MinRange,fp32 _MaxRange,int _MinAwareness,bool _bRemoveSpotted)
{
	if (GetFirstMatchingDead(_pDead,_Relation,_MinRange,_MaxRange,false))
	{	// Check if we can see this corpse
		// Turn off x-ray vision if any
		int PerceptionFlags = m_pAI->m_PerceptionFlags;
		m_pAI->m_PerceptionFlags &= ~PERC_XRAYVISION_FLAG;
		int VictimAwareness = m_pAI->Perception_Sight(_pDead->m_iVictim,true);
		m_pAI->m_PerceptionFlags = PerceptionFlags;

#ifndef M_RTM
		CWObject* pObjVictim = m_pAI->m_pServer->Object_Get(_pDead->m_iVictim);
		if ((pObjVictim)&&(m_pAI->m_pGameObject))
		{
			CVec3Dfp32 viewerPos,sightPos; 
			CMat4Dfp32 LookMat;
			m_pAI->GetHeadMat(LookMat);
			viewerPos = CVec3Dfp32::GetRow(LookMat,3);
			sightPos = pObjVictim->GetPosition();
			m_pAI->DebugDrawCheckDead(viewerPos,sightPos,VictimAwareness);
			if (viewerPos.DistanceSqr(sightPos) <= 32.0f)
			{	// Autospot within 32 units
				VictimAwareness = CAI_AgentInfo::SPOTTED;
			}
		}
#endif
		if (VictimAwareness >= _MinAwareness)
		{	// Can we see the murderer?
			CAI_AgentInfo* pCulprit = m_pAI->m_KB.GetAgentInfo(_pDead->m_iCause);
			if ((pCulprit)&&(pCulprit->GetCurRelation() > CAI_AgentInfo::FRIENDLY))
			{	// Let the music play
				m_pAI->SetStealthTension(CAI_Core::TENSION_HOSTILE);
				if (pCulprit->GetAwareness(true) >= CAI_AgentInfo::DETECTED)
				{
//					CWObject* pVictim = m_pAI->m_pServer->Object_Get(_pDead->m_iVictim);
					CVec3Dfp32 VictimPos = m_pAI->GetBasePos(_pDead->m_iVictim);
					if (VALID_POS(VictimPos))
					{
						if (VictimPos.DistanceSqr(pCulprit->GetBasePos()) < Sqr(96.0f))
						{	// Guilt by association!
							// We find you guilty of MURDER because you're too damned close to a fresh corpse
							pCulprit->SetRelation(CAI_AgentInfo::ENEMY);
							// Remove the victim to avoid multiple reports
							RemoveDead(_pDead->m_iVictim);
							return(2);
						}
					}

					// We see culprit but cannot connect him with the murder
					// Set relation to at least HOSTILE
					if (pCulprit->GetCurRelation() < CAI_AgentInfo::HOSTILE)
					{	// We'll increase to Hostile
						pCulprit->SetRelation(CAI_AgentInfo::HOSTILE);
					}
				}
			}

			// Remove the victim to avoid multiple reports
			RemoveDead(_pDead->m_iVictim);
			return(1);
		}
		else
		{
			return(0);
		}
	}
	else
	{
		return(0);
	}
};

void CAI_KnowledgeBase::ReportDeath(SDead _Dead)
{
	ReportDeath(_Dead.m_iCause,_Dead.m_iVictim,_Dead.m_DeathPos,_Dead.m_CauseOfDeath);
};

// Find any investigate actions and tell them the teammember died
void CAI_KnowledgeBase::ReportDeath(int _iCause,int _iObj,CVec3Dfp32 _DeathPos,int8 _CauseOfDeath)
{
	// We ignore deaths we caused ourselves
	if ((m_pAI->m_pGameObject)&&(m_pAI->m_pGameObject->m_iObject == _iCause))
	{
		return;
	}

	// We ignore reports on our own death
	if ((m_pAI->m_pGameObject)&&(m_pAI->m_pGameObject->m_iObject == _iObj))
	{
		return;
	}

	// Do we already know about this one?
	m_NbrOfValidDead = -1;
	for (int i = 0; i < m_lDead.Len(); i++)
	{
		if (m_lDead[i].m_iVictim == _iObj)
		{
			if (_iCause)
			{
				m_lDead[i].m_bFound = false;
				m_lDead[i].m_iCause = _iCause;
				if (_CauseOfDeath != SDead::DEATHCAUSE_UNKNOWN)
				{
					m_lDead[i].m_CauseOfDeath = _CauseOfDeath;
				}
			}
			return;
		}
	}

	// If we are friends with the actual perp we clear the _iCause to avoid getting unfriendly
	// or worse with him
	CAI_AgentInfo* pPerp = GetAgentInfo(_iCause);
	if ((pPerp)&&(pPerp->GetCurRelation() == CAI_AgentInfo::FRIENDLY))
	{
		_iCause = 0;
		if (m_pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)
		{	// Ugly trick to get the darkling to celebrate a (recent) kill by Jackie
			if (m_pAI->GetAITick() > m_pAI->GetAITicksPerSecond() * 5)
			{
				if ((!m_pAI->m_AH.m_iTarget)||(m_pAI->m_AH.m_iTarget == _iObj))
				{
					if ((Random > 0.5f)||(m_NEnemies <= 1))
					{	// Final enemy!
						m_pAI->UseRandom("Happy counting down enemies",CAI_Device_Sound::COMBAT_KILLJOY,CAI_Action::PRIO_COMBAT);
						if (Random > 0.5f)
						{
							m_pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_DARKLING_THUMB_UP,CAI_Action::PRIO_COMBAT,0,-1);
						}
						else
						{
							m_pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_DARKLING_SUCCESS,CAI_Action::PRIO_COMBAT,0,-1);
						}
					}
				}
			}
		}
		else	
		{	// We also return here as we don't care about bodies killed by friends!
			return;
		}
	}
	if ((!pPerp)&&(_iCause))
	{
		pPerp = AddAgent(_iCause,CAI_AgentInfo::INFOFLAGS_NONE,CAI_AgentInfo::NONE);
	}

	// New dead guy, add him to the list
	SDead dead;
	dead.m_iCause = _iCause;
	dead.m_iVictim = _iObj;
	dead.m_CauseOfDeath = _CauseOfDeath;
	dead.m_Relation = DefaultRelation(_iObj);
	dead.m_DeathPos = _DeathPos;
	dead.m_TimeOfDeath = m_pAI->m_Timer;

	m_lDead.Add(dead);

#ifndef M_RTM
	//DEBUG
	if (m_pAI->DebugRender())
	{
		CStr Message = m_pAI->GetDebugName(m_pAI->m_pGameObject) + " 'knows' ";
		Message += m_pAI->GetDebugName(_iObj);
		Message += CStr(" is dead");
		if (pPerp)
		{
			Message += CStr(" killed by ");
			Message += m_pAI->GetDebugName(_iCause);
		}
		if (Message.Len() > 1)
		{
			ConOutL(Message);
		}
	}
	//DEBUG
#endif
}

void CAI_KnowledgeBase::SetInvestigated(int _iVictim)
{
	for (int i = 0; i < m_lDead.Len(); i++)
	{
		if (m_lDead[i].m_iVictim == _iVictim)
		{
			m_lDead[i].m_bInvestigated = true;
			m_NbrOfValidDead = -1;
			break;
		}
	}
};

int CAI_KnowledgeBase::NbrOfValidDead()
{
	if (m_NbrOfValidDead == -1)
	{
		m_NbrOfValidDead = 0;
		for (int i = 0; i < m_lDead.Len(); i++)
		{
			if (!m_lDead[i].m_bFound)
			{
				m_NbrOfValidDead++;
			}
		}
	}

	return(m_NbrOfValidDead);
};

// Returns true if _iDead exists and if _bUninvestigated is true also is uninvestigated
bool CAI_KnowledgeBase::IsValidDead(int _iDead,bool _bUninvestigated)
{
	int N = m_lDead.Len();
	for (int i = 0; i < N; i++)
	{
		if (m_lDead[i].m_iVictim == _iDead)
		{
			if (m_lDead[i].m_bFound)
			{
				return(true);
			}

			if ((_bUninvestigated)&&(m_lDead[i].m_bInvestigated))
			{
				return(false);
			}
			else
			{
				return(true);
			}
		}
	}

	return(false);
};

bool CAI_KnowledgeBase::RemoveDead(int _iCorpse)
{
	for (int i = 0; i < m_lDead.Len(); i++)
	{
		if (m_lDead[i].m_iVictim == _iCorpse)
		{
			m_lDead[i].m_bFound = true;
			m_NbrOfValidDead = -1;
			return(true);
		}
	}

	return(false);
};

// Sets the appropriate idlestance and makes _iPerp enemy within 2 seconds
// Don't supply _iPerp unless you should know _iPerp is the murderer
void CAI_KnowledgeBase::FoundCorpse(int _iPerp,int _iVictim,CVec3Dfp32 _Pos)
{
	int Alertness = GetAlertness();
	if ((Alertness < CAI_KnowledgeBase::ALERTNESS_WATCHFUL)&&(Alertness >= CAI_KnowledgeBase::ALERTNESS_DROWSY))
	{
		SetAlertness(CAI_KnowledgeBase::ALERTNESS_WATCHFUL);
	}

	if (_iPerp > 0)
	{
		m_pAI->SetMinStealthTension(CAI_Core::TENSION_HOSTILE);
		if (m_pAI->m_DeviceStance.GetIdleStance() < CAI_Device_Stance::IDLESTANCE_HOSTILE)
		{
			m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
		}
		CAI_AgentInfo* pPerp = GetAgentInfo(_iPerp);
		if ((pPerp)&&(pPerp->GetCurRelation() < CAI_AgentInfo::ENEMY))
		{
			pPerp->SetRelation(CAI_AgentInfo::ENEMY,40);
		}
	}
	else
	{
		if ((m_pAI->m_AH.HasAction(CAI_Action::INVESTIGATE))&&(m_pAI->GetStealthTension() <= CAI_Core::TENSION_HOSTILE))
		{	
			m_pAI->m_AH.m_FoundCorpseTick = m_pAI->GetAITick();
		}
		
		int iPlayer = m_pAI->GetClosestPlayer();
		CAI_AgentInfo* pInfo = GetAgentInfo(iPlayer);
		if (!pInfo)
		{
			pInfo = AddAgent(iPlayer,CAI_AgentInfo::INFOFLAGS_NONE,CAI_AgentInfo::NONE);
		}
		m_pAI->m_AH.SetInvestigateObject(iPlayer);
	}

	RemoveDead(_iVictim);
};

//Info gathering methods.
int CAI_KnowledgeBase::GetLastHurtTicks()
{
	return(m_pAI->m_Timer - m_LastHurtTimer);
};

void CAI_KnowledgeBase::UpdateLastHurtTick()
{
	if ((m_pAI)&&(m_pAI->m_pServer))
	{
		m_LastHurtTimer = m_pAI->m_Timer;
		m_pAI->m_DeviceStance.SetTargetInFOV(true);
	}
};

//Get believed security level of this agent. This is based on any restricted zones the 
//agent is in as well as whether we think he's fighting or carrying weapons etc.
int CAI_KnowledgeBase::GetSecurityLevel(int _iObj)
{
	if (!m_pAI || !m_pAI->m_pServer)
		return 0;

	int SecurityLevel = 0;
	if (m_pAI->IsValidAgent(_iObj))
	{
		//Get security level based on restricted area
		CWObject_Message Msg(OBJMSG_CHAR_GETREQUIREDCLEARANCELEVEL);
		SecurityLevel = m_pAI->m_pServer->Message_SendToObject(Msg, _iObj);

		//Any weapons the agent is wielding might cause security level to rise
		CRPG_Object_Item * pWeapon = m_pAI->GetRPGItem(_iObj, RPG_CHAR_INVENTORY_WEAPONS);
		if (pWeapon)
		{
			int WeaponSLevel = pWeapon->GetRequiredClearanceLevel();
			if (WeaponSLevel > SecurityLevel)
				SecurityLevel = WeaponSLevel;
		}
	}

	return SecurityLevel;
};






#ifndef M_RTM
//Get some debug info about the bot
CStr CAI_KnowledgeBase::GetDebugString()
{
	if (!m_pAI)
		return CStr("");
	else
	{
		CStr str = "World knowledge: ";
		return str;
	};
};
#endif

//Calculate noise modifier for the given object from our perspective
fp32 CAI_KnowledgeBase::GetLoudness(CWObject * _pObj)
{
	if (!m_pAI || !_pObj || !m_pAI->m_pGameObject || !m_pAI->m_pServer)
		return 0;

	//Get object noise
	fp32 Noise = 0;
	CWObject_Message Msg(OBJMSG_CHAR_NOISE);
	Msg.m_pData = &Noise;
	if (!m_pAI->m_pServer->Message_SendToObject(Msg, _pObj->m_iObject))
		Noise = 0;


	return Noise;
};	

//Calculate noise modifier for the given object from our perspective
fp32 CAI_KnowledgeBase::GetVisibility(CWObject * _pObj)
{
	if (!m_pAI || !_pObj || !m_pAI->m_pGameObject || !m_pAI->m_pServer)
		return 1.0f;

	fp32 Vis = 0.0f;
	fp32 Light = 0.0f;

	CAI_Core* pObjAI = m_pAI->GetAI(_pObj->m_iObject);
	if (pObjAI)
	{
		Light = pObjAI->m_KB.GetLight();
	}
	if (Light >= 1.0f)
	{
		return(Light);
	}

	if (pObjAI)
	{
		Vis = pObjAI->GetLightIntensity(0);
	}

	if (Vis > Light)
	{
		return(Vis);
	}
	else
	{
		return(Light);
	}
};

//Add agent to knowledge base
void CAI_KnowledgeBase::Global_NewAgent(int _iObj)
{
	//Check if we should create global list first
	CWObject* pObj = m_pAI->m_pServer->Object_Get(_iObj);
	if ((!pObj)||
		(CStrBase::CompareNoCase(pObj->GetName(), "TELEPHONEREG") == 0)||
		(CStrBase::CompareNoCase(pObj->GetName(), "RADIO") == 0))
	{
		return;
	}

	if ( m_pAI->m_pAIResources)
	{
		CSimpleIntList &lAgents = m_pAI->m_pAIResources->m_KBGlobals.ms_lAgents;

		//Add agent to global list if not already added
		if (lAgents.Find(_iObj) == -1)
			lAgents.Add(_iObj);
	}
};


//Remove agent from global world info
void CAI_KnowledgeBase::Global_RemoveAgent(int _iObj)
{
	if ( m_pAI->m_pAIResources)
	{
		CSimpleIntList &lAgents = m_pAI->m_pAIResources->m_KBGlobals.ms_lAgents;
		lAgents.Remove(lAgents.Find(_iObj));
	}
};

void CAI_KnowledgeBase::Global_ClearDead()
{
	if ( m_pAI->m_pAIResources)
	{
		TArray<SDead> &lDead = m_pAI->m_pAIResources->m_KBGlobals.ms_lDead;
		lDead.Clear();
	}
};

void CAI_KnowledgeBase::CleanStatic()
{
	if ( m_pAI->m_pAIResources)
	{
		m_pAI->m_pAIResources->m_KBGlobals.CleanStatic();
	}
}
