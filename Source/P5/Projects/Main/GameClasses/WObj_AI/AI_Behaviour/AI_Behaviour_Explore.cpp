#include "PCH.h"
#include "../AICore.h"
//#include "../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
//#include "../../../../../Shared/MOS/Classes/GameWorld/Server/WServer.h"


//The explore behaviour

//Constructor
CAI_Behaviour_Explore::CAI_Behaviour_Explore(CAI_Core * _pAI, fp32 _Loyalty, const CVec3Dfp32& _Pos, int _iValidTime)
	: CAI_Behaviour_Survive(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Explore_ctor, MAUTOSTRIP_VOID);

	m_Type = EXPLORE;
	if (_pAI && _pAI->IsValid())
		m_PathPos = _pAI->m_PathFinder.GetPathPosition(_Pos, 20, 3);
	else
		m_PathPos = CVec3Dfp32(_FP32_MAX);
	m_Destination = m_PathPos;

	if (m_PathPos != CVec3Dfp32(_FP32_MAX))
		m_Speed = (m_pAI && (m_pAI->m_KB.GetAlertness() < CAI_KnowledgeBase::ALERTNESS_WATCHFUL)) ? Max(m_pAI->m_IdleMaxSpeed, m_pAI->GetMaxSpeedForward()) : m_pAI->GetMaxSpeedForward();

	if (_iValidTime >= 0)
		m_Timer = _iValidTime;
	else
		m_Timer = -1;

	if (_Loyalty < 0)
		m_Loyalty = 0.2f;
	else
		m_Loyalty = _Loyalty;
};


//Will move around a lot and scan a lot until enemy spotted; will then deal with enemy 
//accordning to personality
//Rudimentary until better pathfinding...fix
void CAI_Behaviour_Explore::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Explore_OnTakeAction, MAUTOSTRIP_VOID);
	//Can't take action if we don't have a valid AI
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//if (m_pAI->m_bSearchingForThreat)
	//	m_pAI->AIOut(CStrF("SEARCHING: %d", m_Timer));

	//Reduce timer, if applicable
	if (m_Timer > 0)
		m_Timer--;

	//If we have a valid sub-behaviour, it should choose the action we should take until we decide to abort it.
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
	{
		//1% chance of aborting follow behaviour every 100 frames
		if ( (m_spSubBehaviour->m_Type == FOLLOW) &&
			 (m_pAI->m_Timer % 100 == 0) &&
			 (Random < 0.01f) )
		{
			m_spSubBehaviour = NULL;
			m_pAI->OnIdle();
		}
		else
		{
			m_spSubBehaviour->OnTakeAction();
		}
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
		//Should we step aside for someone?
		else if (m_pAI->OnStepAside(m_Destination))
		{
			m_pAI->ResetPathSearch();
		}
		//Look for potential leaders sometimes if we don't have a specific position to go to
		else if ( (m_PathPos == CVec3Dfp32(_FP32_MAX)) &&
				  (m_pAI->m_Timer % 100 == 0) &&
				  (Random < m_Loyalty) &&
				  FindLeader() )
		{
			//Found potential leader! .Start up follow behaviour, and use that 
			//until it becomes invalid or we tire of it.
			m_Destination = CVec3Dfp32(_FP32_MAX);
			m_Speed = 0;
			m_spSubBehaviour = MNew1(CAI_Behaviour_Follow, m_pAI);
			m_spSubBehaviour->OnTakeAction();
		}
		else
		{
			//No subbehaviour. Lets just run around, staying close to friends
			//and not making any sharp turns if possible.

			//Always stop if at low activity
			if (m_pAI->m_ActivityLevel <= CAI_Core::ACTIVITY_LOW)
				m_pAI->m_DeviceMove.Use();

			//Change default heading at random intervals
			if ((m_pAI->m_Timer % 10 == 0) && 
				(Random < 0.1f))
				m_pAI->m_DefaultHeading += Random * 0.5f - 0.25f;

			//Always scan around a lot
			if (m_pAI->m_DeviceLook.IsAvailable())
			{
				//Just stay put and look around lazily.
				if (m_pAI->m_ActionFlags & CAI_Core::LOOKING)
					m_pAI->OnLook();
				else if (Random < 0.2f)
				{
					//Sweep gaze to another angle within 90 degrees of the default heading and reset pitch
					m_pAI->OnLook(m_pAI->m_DefaultHeading + (Random/2) - 0.25f, 0, 25 + Random * 50);
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

				//Continue moving.if we can get "pursuit" allowance
				if (m_pAI->ms_AllowPursuit.Poll(m_pAI->m_pGameObject->m_iObject, m_pAI->GetPathPrio(), m_pAI->m_pServer->GetGameTick()))
				{
					m_pAI->OnMove(m_Destination, m_Speed, true);

					//Were we unable to go to this position?
					if ((m_pAI->m_SearchStatus == CAI_Pathfinder::NO_PATH) ||
						(m_pAI->m_SearchStatus == CAI_Pathfinder::INVALID) )
					{
						//Change destination
						m_Destination = GetNewDestination(200, m_pAI->m_DeviceLook.GetNextLook()[2], 0.2f);
						m_Speed = Random * m_pAI->GetMaxSpeedForward() * 1.5f + 2; 
					}
				}
				else
				{
					m_pAI->OnIdle();
				}

				//Change or reset destination if we're there or decide to change on a whim
				//If we're moving towards a specific given position, this will not happen
				if ( (((m_pAI->m_Timer % 50) == 0) && (m_Destination != m_PathPos) && (Random < 0.2f)) ||
					 (m_pAI->m_pGameObject->GetPosition()).DistanceSqr(m_Destination) < 100*100)
				{
					//Reset specific given destination if we've got one (since we're there)
					m_PathPos = CVec3Dfp32(_FP32_MAX);

					//Should we stop or find new destination?
					if (Random < 0.2f)
					{
						m_Destination = CVec3Dfp32(_FP32_MAX);
						m_Speed = 0;
					}
					else
					{
						if ((m_PathPos != CVec3Dfp32(_FP32_MAX)) && 
							(Random < 0.5f))
						{
							//Go to the given position, if any
							m_Destination = GetNewDestination(400, m_pAI->m_DeviceLook.GetNextLook()[2], 0.2f);
							m_Speed = Random * m_pAI->GetMaxSpeedForward() * 1.5f + 3; 
						}
						else
						{
							//Go to random new destination
							m_Destination = GetNewDestination(400, m_pAI->m_DeviceLook.GetNextLook()[2], 0.2f);
							m_Speed = Random * m_pAI->GetMaxSpeedForward() * 1.5f + 2; 
						};
					};
				};
			}
			else
			{
				//m_pAI->Debug_RenderWire(m_pAI->m_pGameObject->GetPosition(), m_pAI->m_pGameObject->GetPosition() + CVec3Dfp32(0,0,100));//DEBUG

				//Should we find a new destination?
				if ( (m_pAI->m_Timer % 20 == 0) &&
					 (Random < 0.5f) )
				{
					if ((m_PathPos != CVec3Dfp32(_FP32_MAX)) && 
						(Random < 0.5f))
					{
						//Go to the given position, if any
						m_Destination = GetNewDestination(400, m_pAI->m_DeviceLook.GetNextLook()[2], 0.2f);
						m_Speed = Random * m_pAI->GetMaxSpeedForward() * 1.5f + 3; 
					}
					else
					{
						//Find new destination, or go to an ally
						m_Destination = GetNewDestination(400, m_pAI->m_DeviceLook.GetNextLook()[2], 0.2f);
						
						//Somewhat higher chance of maxspeed than other speeds, 
						//otherwise between maxspeed and 2 units per frame
						m_Speed = Random * m_pAI->GetMaxSpeedForward() * 1.5f + 2; 
					};
				};
				
				//If there's a threat, set default heading towards that position	
				/*
				if (m_pAI->m_bThreat &&
					m_pAI->m_ThreatPos != CVec3Dfp32(_FP32_MAX))
					m_pAI->m_DefaultHeading = m_pAI->HeadingToPosition(m_pAI->m_pGameObject->GetPosition(), m_pAI->m_ThreatPos);

				//Pause.
				m_pAI->OnIdle();
				*/
			};

			//Make a noise every few seconds
			/// m_pAI->MakeNoise("Behaviour explore",CAI_Device_Sound::IDLE_IDLE, m_pAI->m_MinIdleSoundsInterval, m_pAI->m_MaxIdleSoundsInterval);
		};
	};
};

//Valid as long as timer is non-zero
bool CAI_Behaviour_Explore::IsValid()
{
	MAUTOSTRIP(CAI_Behaviour_Explore_IsValid, false);
	return m_Timer != 0;
};

//Re-init position
void CAI_Behaviour_Explore::ReInit(int iParam1, int iParam2, const CVec3Dfp32& VecParam1, const CVec3Dfp32& VecParam2)
{
	MAUTOSTRIP(CAI_Behaviour_Explore_ReInit, MAUTOSTRIP_VOID);
	Reset();	
	if (m_pAI && m_pAI->IsValid())
		m_PathPos = m_pAI->m_PathFinder.GetPathPosition(VecParam1, 20, 3);
	else
		m_PathPos = CVec3Dfp32(_FP32_MAX);
	m_Destination = m_PathPos;

	if (m_PathPos != CVec3Dfp32(_FP32_MAX))
		m_Speed = (m_pAI && (m_pAI->m_KB.GetAlertness() < CAI_KnowledgeBase::ALERTNESS_WATCHFUL)) ? m_pAI->m_IdleMaxSpeed : m_pAI->GetMaxSpeedForward();

	if (iParam1 >= 0)
		m_Timer = iParam1;
	else
		m_Timer = -1;
};



