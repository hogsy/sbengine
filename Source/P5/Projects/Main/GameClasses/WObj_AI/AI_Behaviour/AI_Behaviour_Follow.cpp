#include "PCH.h"
#include "../AICore.h"


//The follow behaviour

//Checks if the given server index points to a valid leader.
//Returns pointer to leaders character object if so else NULL
CWObject * CAI_Behaviour_Follow::IsValidLeader(int _iObj)
{
	MAUTOSTRIP(CAI_Behaviour_Follow_IsValidLeader, NULL);
	return(NULL);
};	

//Tries to set a leader. Returns pointer to leaders character object if successful, else NULL
CWObject * CAI_Behaviour_Follow::FindLeader()
{
	MAUTOSTRIP(CAI_Behaviour_Follow_FindLeader, NULL);
	return(NULL);
};


//Constructor
CAI_Behaviour_Follow::CAI_Behaviour_Follow(CAI_Core * _pAI, int _iLeader, bool _bTrueLeader)
	: CAI_Behaviour(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Follow_ctor, MAUTOSTRIP_VOID);
	m_iLeader = _iLeader;
	m_bTrueLeader = _bTrueLeader;
	m_Type = FOLLOW;
};


//Will follow leader until enemy spotted; will then deal with enemy according to personality.
//If not given a leader when initializing, bot will follow first friend to come close.
void CAI_Behaviour_Follow::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Follow_OnTakeAction, MAUTOSTRIP_VOID);
	//Can't take action if we don't have a valid AI
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//If we have a valid sub-behaviour, it should choose the action we should take
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
	{
		m_spSubBehaviour->OnTakeAction();
	} 
	else
	{
		//We don't have a sub-behaviour to follow
		m_spSubBehaviour = NULL;
	
		//Get current leader
		CWObject * pLeader = IsValidLeader(m_iLeader);
		
		//Look for enemies
		if ( (m_pAI->m_KB.OnSpotEnemies()) &&
			 (m_spSubBehaviour = m_pAI->m_spPersonality->OnDealWithEnemies()) )
		{
			//We have spotted an enemy and wants to do something about it! Start up a sub-behaviour and use that until it becomes invalid.
			m_spSubBehaviour->OnTakeAction();
		}
		//Is there a threat?
		/*
		else if (m_pAI->OnThreatAlert())
		{
			//We're looking for a threat
		}
		*/
		//Should we step aside for someone?
		else if (m_pAI->OnStepAside((pLeader) ? pLeader->GetPosition() : CVec3Dfp32(_FP32_MAX)))
		{
			m_pAI->ResetPathSearch();
		}
		else
		{
			//Always stop if at low activity
			if (m_pAI->m_ActivityLevel <= CAI_Core::ACTIVITY_LOW)
				m_pAI->m_DeviceMove.Use();

			//We have not spotted an enemy, do we have a leader?
			if (!pLeader)
			{
				//We don't have a valid leader, reset leader index. Try to find new leader every 10 frames.
				m_iLeader = 0;
				if ( (m_pAI->m_Timer % 10 == 0) &&
					 (pLeader = FindLeader()) )
				{
					//Set new path destination to follow leader
					m_pAI->m_PathDestination = m_pAI->m_PathFinder.GetPathPosition(pLeader);
					if (m_pAI->m_PathDestination == CVec3Dfp32(_FP32_MAX))
						//Leader is airborne or something
						m_pAI->m_PathDestination = m_pAI->m_PathFinder.GetPathPosition(pLeader, true);
				};
			};

			//Do we have a leader now?
			if (pLeader)
			{
				//We have leader, follow him. Currently we don't get out of the leaders way or the way
				//of other friends. Will fix.

				//m_pAI->Debug_RenderWire(m_pAI->m_pGameObject->GetPosition(), pLeader->GetPosition());//DEBUG
					
				//Move if we aren't close enough to leader
				fp32 DistSqr = (pLeader->GetPosition()).DistanceSqr(m_pAI->m_pGameObject->GetPosition());
				if (DistSqr > 100*100)
				{
					//If we are more than 200 units away from leader we should move at maximum idle speed, otherwise
					//emulate leaders speed.
					fp32 Speed;
					if (DistSqr > 200*200) 
						Speed = m_pAI->m_IdleMaxSpeed;
					else if (DistSqr > 150*150)
						Speed = (m_pAI->m_IdleMaxSpeed + 1.5f) / 2;
					else
						Speed = Min(m_pAI->m_IdleMaxSpeed, Max(1.5f, pLeader->GetLastPosition().Distance(pLeader->GetPosition())));

					//If the leader hasn't moved much go to current path destination
					if ((pLeader->GetPosition()).DistanceSqr(m_pAI->m_PathDestination) < 32*32)
					{
						//No special circumstances; just move towards current destination. Don't follow partial path
						m_pAI->OnMove(CVec3Dfp32(_FP32_MAX),Speed,false);
					}
					else
					{
						//Leader has moved more than a bit, go to it's new position, but don't follow partial path.
						m_pAI->OnMove(m_pAI->m_PathFinder.GetPathPosition(pLeader), Speed); //If leader is airborne, then go to last position
					};
				}
				else
				{
					//Look the same way as leader
					m_pAI->m_DefaultHeading = m_pAI->GetHeading(pLeader);

					//Do nothing
					m_pAI->OnIdle();

					//We are were we want to be so reset path
					m_pAI->ResetPathSearch();
				};
			}
			else
			{
				//We don't have a leader. Boooring...
				m_pAI->OnIdle();

				//Reset path
				m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
				m_pAI->ResetPathSearch();	
			};

			// m_pAI->MakeNoise("Behaviour follow",CAI_Device_Sound::IDLE_IDLE, m_pAI->m_MinIdleSoundsInterval, m_pAI->m_MaxIdleSoundsInterval);
		};
	};
};


//Behaviour is valid when there's a valid leader around or sub-behaviour is valid
bool CAI_Behaviour_Follow::IsValid()
{
	MAUTOSTRIP(CAI_Behaviour_Follow_IsValid, false);
	//Not valid if AI object is invalid
	if (!m_pAI || !m_pAI->IsValid())
		return false;

	return ( IsValidLeader(m_iLeader) || 
			 (m_spSubBehaviour && m_spSubBehaviour->IsValid()) );
};

//Re-init leader
void CAI_Behaviour_Follow::ReInit(int iParam1, int iParam2, const CVec3Dfp32& VecParam1, const CVec3Dfp32& VecParam2)
{
	MAUTOSTRIP(CAI_Behaviour_Follow_ReInit, MAUTOSTRIP_VOID);
	Reset();	
	m_iLeader = iParam1;
	m_bTrueLeader = iParam2 != 0;
};

//Path prio is greater when following a human player
int CAI_Behaviour_Follow::GetPathPrio(fp32 _CameraScore)
{
	MAUTOSTRIP(CAI_Behaviour_Follow_GetPathPrio, 0);
	CWObject * pChar = m_pAI->IsValidAgent(m_iLeader);
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
		return m_spSubBehaviour->GetPathPrio(_CameraScore) + ((pChar && !m_pAI->IsBot(pChar->m_iObject)) ? 127 : 0);
	else
		return (128 * _CameraScore) + ((pChar && !m_pAI->IsBot(pChar->m_iObject)) ? 127 : 0);
};


