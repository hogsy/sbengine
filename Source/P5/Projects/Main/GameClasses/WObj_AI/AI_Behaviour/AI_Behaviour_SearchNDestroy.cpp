#include "PCH.h"
#include "../AICore.h"



//Constructor
CAI_Behaviour_SearchNDestroy::CAI_Behaviour_SearchNDestroy(CAI_Core * _pAI)
	: CAI_Behaviour_Engage(_pAI)
{
	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_PrevUncertainty = 0;
	m_Type = SEARCHNDESTROY;
	m_Mode = SEARCH;
	m_ChangeTick = 0;
	m_StopTrackTick = 0;
};


//Get new random movement destination, at or closer to given distance preferrably close to the given heading
//If no such destination can be found, fail with CVec3Dfp32(_FP32_MAX).
CVec3Dfp32 CAI_Behaviour_SearchNDestroy::GetNewDestination(CAI_AgentInfo * _pTarget, fp32 _Distance, fp32 _Heading, fp32 _GoToAllyChance)
{
	if (!m_pAI || !m_pAI->IsValid())
		return CVec3Dfp32(_FP32_MAX);
	
	//Always go to believed position of current target if not already there
	if (_pTarget &&
		(m_pAI->m_pGameObject->GetPosition().DistanceSqr(_pTarget->GetPosition()) > 200*200))
	{
		CVec3Dfp32 Pos = m_pAI->m_PathFinder.GetPathPosition(_pTarget->GetPosition(), 1, 4);
		if (Pos != CVec3Dfp32(_FP32_MAX))
		{
			//Path position must also be some distance away for us to use it
			if (m_pAI->m_pGameObject->GetPosition().DistanceSqr(Pos) > 200*200)
				return Pos;
		}
		else
			ConOut("No pathpos of uncertain target position");
	}

	//Check if we should go to an ally
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


//Move to believed position if possible, otherwise move to friends or roam randomly.
void CAI_Behaviour_SearchNDestroy::OnSearch(CAI_AgentInfo * _pTarget)
{
	if (m_Mode != SEARCH)
	{
		m_Mode = SEARCH;
	}
	else 
	{
		//Check if we should switch behaviour mode
		//Only enter destroy mode if jumpy (cool dudes don't shoot unless they're certain of their target)
		if (_pTarget &&
			(m_pAI->m_KB.GetAlertness() == CAI_KnowledgeBase::ALERTNESS_JUMPY))
		{
			//If we suddenly get a drop in position uncertainty, target might have been found
			if (true)
			{
				//Get some! Get some!
				OnDestroy(_pTarget);
				return;
			}
			else
			{
				/* *** Remove when behaviours are gone
				//Otherwise, if we become aware of any phenomenon we will assume that it was caused by target
				CAI_Phenomenon * pPhen = m_pAI->m_KB.GetBestPhenomenon();
				if (pPhen )
				{
					CVec3Dfp32 Pos = pPhen->GetPosition(m_pAI->m_pServer);
					if (Pos != CVec3Dfp32(_FP32_MAX))
						_pTarget->ForceSetPosition(Pos);
				}
				*/
			}
		}
	}

	//Always stop if at low activity
	if (m_pAI->m_ActivityLevel <= CAI_Core::ACTIVITY_LOW)
		m_pAI->m_DeviceMove.Use();

	//Change default heading at random intervals
	if ((m_pAI->m_Timer % 10 == 0) && 
		(Random < 0.1f))
		m_pAI->m_DefaultHeading += Random * 0.5f - 0.25f;

	//Scan around wildly
	if (m_pAI->m_DeviceLook.IsAvailable())
	{
		if (m_pAI->m_ActionFlags & CAI_Core::LOOKING)
			m_pAI->OnLook();
		else if (Random < 0.2f)
		{
			//Sweep gaze to another angle within 45 degrees of the default heading and reset pitch
			m_pAI->OnLook(m_pAI->m_DefaultHeading + (Random/4) - 0.125f, 0, 10 + Random * 30);
		};
	};

	//Do we have a current destination?
	if (m_Destination != CVec3Dfp32(_FP32_MAX))
	{
		//m_pAI->Debug_RenderWire(m_pAI->m_pGameObject->GetPosition(), m_Destination);//DEBUG
		
		//Continue moving. 
		m_pAI->OnMove(m_Destination, true);

		//Were we unable to go to this position?
		if ((m_pAI->m_SearchStatus == CAI_Pathfinder::NO_PATH) ||
			(m_pAI->m_SearchStatus == CAI_Pathfinder::INVALID) )
		{
			//Get new destination
			m_Destination = GetNewDestination(_pTarget, 200, m_pAI->m_DeviceLook.GetNextLook()[2], 0.3f);
		}

		//Reset destination if we're there or decide to change on a whim
		if ( ((m_pAI->m_pServer->GetGameTick() > m_ChangeTick) && (m_pAI->m_Timer % 100 == 0) && (Random < 0.1f)) ||
			 (m_pAI->m_pGameObject->GetPosition()).DistanceSqr(m_Destination) < 100*100)
		{
			if ((Random < 0.5f) || (m_pAI->m_pServer->GetGameTick() < m_ChangeTick))
			{
				m_Destination = CVec3Dfp32(_FP32_MAX);
			}
			else
			{
				m_Destination = GetNewDestination(_pTarget, 400, m_pAI->m_DeviceLook.GetNextLook()[2], 0.3f);
			};
			m_ChangeTick = 0;
		};
	}
	else
	{
		//m_pAI->Debug_RenderWire(m_pAI->m_pGameObject->GetPosition(), m_pAI->m_pGameObject->GetPosition() + CVec3Dfp32(0,0,100));//DEBUG

		//Should we find a new destination?
		if ( (m_pAI->m_Timer % 50 == 0) &&
			 (Random < 0.3f) )
		{
			//Find new destination, or go to an ally
			m_Destination = GetNewDestination(_pTarget, 400, m_pAI->m_DeviceLook.GetNextLook()[2], 0.3f);
		}
		
		//Pause.
		m_pAI->OnIdle();
	};
};


//We think we've got enemy! Fire repeatedly, then go investigate.
void CAI_Behaviour_SearchNDestroy::OnDestroy(CAI_AgentInfo * _pTarget)
{
	if (m_Mode != DESTROY)
	{
		m_Mode = DESTROY;
		m_ChangeTick = m_pAI->m_pServer->GetGameTick() + Random * 80 + 20;
		m_StopTrackTick = m_pAI->m_pServer->GetGameTick() + 5;
	}
	else 
	{
		//Check if we should switch to search
		//Start investigating once we're done firing
		if ((m_pAI->m_pServer->GetGameTick() > m_ChangeTick) && 
			(m_pAI->Wait() == 0))
		{
			//Set new default heading
			m_pAI->m_DefaultHeading = m_pAI->HeadingToPosition(m_pAI->m_pGameObject->GetPosition(), _pTarget->GetPosition());		

			//Search to targets position
			m_Destination = GetNewDestination(_pTarget, 400, m_pAI->m_DefaultHeading, 0);

			//Don't deviate from this search for at least 10 seconds
			m_ChangeTick = m_pAI->m_pServer->GetGameTick() + 200;

			//Search!
			OnSearch(_pTarget);
			return;
		}
	}

	//Hold still
	m_pAI->m_DeviceMove.Use();

	//Spend the first few frames turning
	if (m_pAI->m_pServer->GetGameTick() < m_StopTrackTick)
	{
		m_pAI->OnTrack(_pTarget->GetPosition(), 5, true);
	}
	//Are we idle?
	else if ((m_pAI->Wait() == 0) && (m_pAI->m_DeviceWeapon.IsAvailable()))
	{
		//Fire if we think it's useful
		if (m_pAI->m_Weapon.GetWielded()->GetEstimate(_pTarget) > 0)
		{
			m_pAI->OnAttack(_pTarget);
		}
		else
		{
			//No point in firing, cut the change timer
			m_pAI->OnAim(_pTarget);
			m_ChangeTick = 0;
		}
	}
	else
	{
		m_pAI->OnAim(_pTarget);
	}
};



//Will move around a lot and scan a lot. If jumpy, will fire erraticaly when we think enemy has been found 
void CAI_Behaviour_SearchNDestroy::OnTakeAction()
{
	return;
};


//Valid as long as there's a target without accurately known position
bool CAI_Behaviour_SearchNDestroy::IsValid()
{
	//Not valid if AI object is invalid
	if (!m_pAI || !m_pAI->IsValid())
		return false;

	//Always valid if we have valid sub-behaviour
    if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
		return true;

	//Check for known targets
	if (CAI_Behaviour_Engage::IsValid())
	{
		//Behaviour is only valid if current target's whereabouts are uncertain
		CAI_AgentInfo* pInfo = m_pAI->m_KB.GetAgentInfo(m_iTarget);
		if (pInfo)
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
	else
		return false;
};


				
