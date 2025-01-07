#include "PCH.h"
#include "../AICore.h"


//The survive behaviour

//Checks if the given server index points to a valid leader.
//Returns pointer to leaders character object if so else NULL
CWObject * CAI_Behaviour_Survive::IsValidLeader(int _iObj)
{
	MAUTOSTRIP(CAI_Behaviour_Survive_IsValidLeader, NULL);
	return(NULL);
	/*
    //Is leader a valid character, an ally and has an AI-object with higher rank than 
	//us or a human player character?
	CWObject * pLeader;
	CAI_Core * pLeaderAI;
	if ( (pLeader = m_pAI->IsValidAgent(_iObj)) &&
		 m_pAI->IsFriend(pLeader) &&
		 (pLeaderAI = m_pAI->GetAI(pLeader->m_iObject)) &&
	     ((pLeaderAI->m_Rank > m_pAI->m_Rank) ||
		  !m_pAI->IsBot(pLeader->m_iObject)) )
		return pLeader;
	else
		return NULL;
	*/	 
};	


//Tries to find a leader. Returns pointer to leaders character object if successful, else NULL
bool CAI_Behaviour_Survive::FindLeader()
{
	MAUTOSTRIP(CAI_Behaviour_Survive_FindLeader, false);
	//Check all characters to find any suitable leader.
	//Currently we look for the closest valid leader within 200 units.
		
	//Check if there's a leader within 200 units
	CWObject * pLeader = NULL;
	CAI_AgentInfo * pInfo;
	for (int i = 0; i < m_pAI->m_KB.NumAgentInfo(); i++)
	{
		//Check if character is a valid leader
		pInfo = m_pAI->m_KB.IterateAgentInfo(i);
		if (pInfo && (pLeader = IsValidLeader(pInfo->GetObjectID())))
		{	
			if ((pLeader->GetPosition()).DistanceSqr(m_pAI->m_pGameObject->GetPosition()) < 200*200)
			{
				//A good leader found
				return true;
			};
		};
	};

	//No leader found
	return false;
};


//Get new random movement destination, at or closer to given distance preferrably close to the given heading
//If no such destination can be found, fail with CVec3Dfp32(_FP32_MAX).
CVec3Dfp32 CAI_Behaviour_Survive::GetNewDestination(fp32 _Distance, fp32 _Heading, fp32 _GoToAllyChance)
{
	MAUTOSTRIP(CAI_Behaviour_Survive_GetNewDestination, CVec3Dfp32());
	//Just a hack really... will fix when the rough pathfinding is in place

	if (!m_pAI || !m_pAI->IsValid())
		return CVec3Dfp32(_FP32_MAX);
	
	if ( (_GoToAllyChance > 0.0f) &&
		 (Random < _GoToAllyChance) )
	{
		//Go to ally
		int Offset = (int)(Random * m_pAI->m_KB.NumAgentInfo());
		CWObject * pChar;
		CAI_AgentInfo * pInfo;
		CVec3Dfp32 Pos;
		for (int i = 0; i < m_pAI->m_KB.NumAgentInfo(); i++)
		{
			pInfo = m_pAI->m_KB.IterateAgentInfo((i + Offset) % m_pAI->m_KB.NumAgentInfo());
			if (pInfo)
			{
				pChar = m_pAI->IsValidAgent(pInfo->GetObjectID());
				if (pChar &&
					m_pAI->IsFriend(pChar))
				{
					if ((Pos = m_pAI->m_PathFinder.GetPathPosition(pChar)) != CVec3Dfp32(_FP32_MAX))
						return Pos;
				};
			}
		};
	};

	if (m_pAI->m_PathFinder.GridPF())
	{
		//Offset given heading randomly a bit
		fp32 rnd = Random;
		_Heading += (rnd < 0.3f) ? -rnd/2 : ((rnd > 0.7f) ? (1-rnd)/2 : 0); 
		
		fp32 Heading;
		fp32 Height;
		CVec3Dfp32 Pos;
		//The direction we primarily search is randomly determined
		int8 iDir = (Random > 0.5f) ? 1 : -1;

		//Check for positions at the given distance, but if this fails all around, check at closer distances etc...
		for (fp32 Distance = _Distance; Distance > 48; Distance /= 2)
		{		
			for (fp32 dHeading = 0; dHeading <= 0.5f; dHeading += 0.125f)
			{
				//Set heading
				Heading = _Heading + (iDir * dHeading);

				//Set height randomly.
				Height = (int)((rnd - 0.5f) * _Distance / Distance);

				CVec3Dfp32 Pos = m_pAI->m_pGameObject->GetPosition() + (CVec3Dfp32(M_Cos(Heading*2*_PI) , M_Sin(-Heading*2*_PI), Height) * Distance);

				//Use closest graph node position if such can be found
				TThinArray<int16> lNodePos;
				if(m_pAI->m_PathFinder.GraphPF())
					m_pAI->m_PathFinder.GraphPF()->GetNodesAt(m_pAI->m_PathFinder.GridPF()->GetGridPosition(Pos), &lNodePos);

				if (lNodePos.Len())
				{
					//Found node position, first is closest
					return m_pAI->m_PathFinder.GridPF()->GetWorldPosition(m_pAI->m_PathFinder.GraphPF()->GetNodePos(lNodePos[0]));
				}
				else
				{
					//Couldn't find any nodes, try to just find traversable grid position
					Pos = m_pAI->m_PathFinder.GetPathPosition(Pos, 10, 5);

					//m_pAI->Debug_RenderWire(m_pAI->m_pGameObject->GetPosition(), Pos, 0xffffff00, 2);//DEBUG
					
					//Did we find a (hopefully) suitable position?
					if (Pos != CVec3Dfp32(_FP32_MAX))
					{
						//Check that position is not on an edge or something
						int nGround = 0;
						CVec3Dint16 GridPos = m_pAI->m_PathFinder.GridPF()->GetGridPosition(Pos);
						if (!m_pAI->m_PathFinder.GridPF()->IsOutsideGridBoundaries(GridPos, 24, 32))
						{
							for (int x = -1; x < 2; x++)
								for (int y = -1; y < 2; y++)
								{
									if (m_pAI->m_PathFinder.GridPF()->IsOnGround(GridPos + CVec3Dint16(x, y, 0), 1))
										nGround++;
								};
						};

						if (nGround > 5)
							//Jupp, success!
							return Pos;
					};
				}

				//Did not find any position in this pass, set up next pass
				//Toggle direction
				iDir = -iDir;
			};
		};

		//No position can be found
		return CVec3Dfp32(_FP32_MAX);
	}
	else
		//We're effectively blind without a navgrid, so just use position _Distance units away in specified _Heading
		return m_pAI->m_PathFinder.GetPathPosition(m_pAI->m_pGameObject->GetPosition() + (CVec3Dfp32(M_Cos(_Heading*2*_PI), M_Sin(-_Heading*2*_PI), 0) * _Distance), 10, 0) ;
};


//Constructor
CAI_Behaviour_Survive::CAI_Behaviour_Survive(CAI_Core * _pAI)
	: CAI_Behaviour(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Survive_ctor, MAUTOSTRIP_VOID);
	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_Speed = 0;
	m_Type = SURVIVE;
};

//Will move around slowly, stopping at times, scanning a lot, until enemy spotted; 
//will then deal with enemy according to personality. 
void CAI_Behaviour_Survive::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Survive_OnTakeAction, MAUTOSTRIP_VOID);
	//Can't take action if we don't have a valid AI
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//If we have a valid sub-behaviour, it should choose the action we should take until we decide to abort it.
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
	{
		m_spSubBehaviour->OnTakeAction();
	} 
	else
	{
		//We don't have a sub-behaviour to follow
		m_spSubBehaviour = NULL;

		//Look for enemies
		if ( (m_pAI->m_KB.OnSpotEnemies()) &&
			 (m_spSubBehaviour = m_pAI->m_spPersonality->OnDealWithEnemies()) )
		{
			//We have spotted an enemy and wants to do something about it! Start up a sub-behaviour and use that until it becomes invalid.
			m_Destination = CVec3Dfp32(_FP32_MAX);
			m_Speed = 0;
			m_spSubBehaviour->OnTakeAction();
		}
		//Is there a threat?
		/*
		else if (m_pAI->OnThreatAlert())
		{
			//We're looking for a threat
		}
		*/
		//Look for potential leaders every 100 frames
		else if ( (m_pAI->m_Timer % 100 == 0) &&
				  FindLeader() )
		{
			//Found potential leader! .Start up follow behaviour, and use that 
			//until it becomes invalid or we tire of it.
			m_Destination = CVec3Dfp32(_FP32_MAX);
			m_Speed = 0;
			m_spSubBehaviour = MNew1(CAI_Behaviour_Follow, m_pAI);
			m_spSubBehaviour->OnTakeAction();
		}
		//Should we step aside for someone?
		else if (m_pAI->OnStepAside(m_Destination))
		{
			m_pAI->ResetPathSearch();
		}
		else
		{
			//No subbehaviour. Lets just run around, staying close to friends if possible, 
			//and not making any sharp turns if possible.

			//Always stop if at low activity
			if (m_pAI->m_ActivityLevel <= CAI_Core::ACTIVITY_LOW)
				m_pAI->m_DeviceMove.Use();

			//Change default heading at random intervals
			if ((m_pAI->m_Timer % 10 == 0) && 
				(Random < 0.1f))
				m_pAI->m_DefaultHeading += Random * 0.5f - 0.25f;

			//Scan around a bit
			if (m_pAI->m_DeviceLook.IsAvailable())
			{
				if (m_pAI->m_ActionFlags & CAI_Core::LOOKING)
					m_pAI->OnLook();
				else if (Random < 0.2f)
				{
					//Sweep gaze to another angle within 45 degrees of the default heading and reset pitch
					m_pAI->OnLook(m_pAI->m_DefaultHeading + (Random/4) - 0.125f, 0, 25 + Random * 50);
				};
			};

			//Do we have a current destination?
			if ( (m_Destination != CVec3Dfp32(_FP32_MAX)) &&
				 (m_Speed > 0) )
			{
				//m_pAI->Debug_RenderWire(m_pAI->m_pGameObject->GetPosition(), m_Destination);//DEBUG
				
				//Slight chance of movement speed change every hundred frames
				if (m_pAI->m_Timer % 100 == 0)
				{
					fp32 rnd = Random;
					if (rnd < 0.2f)
					{
						//Reduce speed
						m_Speed *= rnd * 5;
						
						//Never go slower than 2 units per frame
						if (m_Speed < 2)
							m_Speed = 2;
					}
					else if (rnd > 0.8f)
					{
						//Increase speed
						m_Speed *= (1 + (1 - rnd) * 5);
					};
				}

				//Continue moving. 
				m_pAI->OnMove(m_Destination, Min(m_pAI->m_IdleMaxSpeed, m_Speed), true);

				//Were we unable to go to this position?
				if ((m_pAI->m_SearchStatus == CAI_Pathfinder::NO_PATH) ||
					(m_pAI->m_SearchStatus == CAI_Pathfinder::INVALID) )
				{
					//Get new destination
					m_Destination = GetNewDestination(200, m_pAI->m_DeviceLook.GetNextLook()[2], 0.3f);
					m_Speed = Random * m_pAI->GetMaxSpeedForward() * 1.5f + 2; 
				}

				//Reset destination if we're there or decides to change on a whim
				if ( ((m_pAI->m_Timer % 100 == 0) && (Random < 0.1f)) ||
					 (m_pAI->m_pGameObject->GetPosition()).DistanceSqr(m_Destination) < 100*100)
				{
					if (Random < 0.5f)
					{
						m_Destination = CVec3Dfp32(_FP32_MAX);
						m_Speed = 0;
					}
					else
					{
						m_Destination = GetNewDestination(400, m_pAI->m_DeviceLook.GetNextLook()[2], 0.3f);
						m_Speed = Random * m_pAI->GetMaxSpeedForward() * 1.5f + 2; 
					};
				};
			}
			else
			{
				//m_pAI->Debug_RenderWire(m_pAI->m_pGameObject->GetPosition(), m_pAI->m_pGameObject->GetPosition() + CVec3Dfp32(0,0,100));//DEBUG

				//Should we find a new destination?
				if ( (m_pAI->m_Timer % 50 == 0) &&
					 (Random < 0.2f) )
				{
					//Find new destination, or go to an ally
					m_Destination = GetNewDestination(400, m_pAI->m_DeviceLook.GetNextLook()[2], 0.3f);
					
					//Somewhat higher chance of maxspeed than other speeds, 
					//otherwise between maxspeed and 2 units per frame
					m_Speed = Random * m_pAI->GetMaxSpeedForward() + 2; 
				};
				
				//Pause.
				m_pAI->OnIdle();
			};

			//Make a noise every few seconds
			// m_pAI->MakeNoise("Behaviour Survive",CAI_Device_Sound::IDLE_IDLE, m_pAI->m_MinIdleSoundsInterval, m_pAI->m_MaxIdleSoundsInterval);
		};
	};
};



