#include "PCH.h"
#include "../AICore.h"
//#include "../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"


//The lead behaviour

//Constructor
CAI_Behaviour_Lead::CAI_Behaviour_Lead(CAI_Core * _pAI, int _iPath, int _iFollower)
	: CAI_Behaviour_Patrol(_pAI, _iPath)
{
	MAUTOSTRIP(CAI_Behaviour_Lead_ctor, MAUTOSTRIP_VOID);
	m_iFollower = _iFollower;
	m_Type = LEAD;
};


//Will follow path when follower is nearby, or follow follower if he moves away from path. 
//When enemy spotted, will then deal with enemy according to personality
//If path is unavaliable, bot will revert to follow behaviour with the follower as leader.
void CAI_Behaviour_Lead::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Lead_OnTakeAction, MAUTOSTRIP_VOID);
	//if (CAI_Core::ms_bRemoveFunctionality)
	//	return;

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

		//Get path object
		CWObject * pPath = m_pAI->m_pServer->Object_Get(m_iPath);

		//Get follower. Just get first player. FIX!
		CWObject * pFollower = m_pAI->IsFriend(m_iFollower);
		if (!pFollower)
		{
			for (int i = 0; i < m_pAI->m_KB.Global_GetAgents()->Length(); i++)
			{
				if ((pFollower = m_pAI->IsFriend(m_pAI->m_KB.Global_GetAgents()->Get(i))) &&
					m_pAI->IsPlayer(pFollower->m_iObject))
				{
					m_iFollower = pFollower->m_iObject;
					break;
				}
				else
				{
					pFollower = NULL;
				};
			};
		};

		//Look for enemies
		if ( (m_pAI->m_KB.OnSpotEnemies()) &&
			 (m_spSubBehaviour = m_pAI->m_spPersonality->OnDealWithEnemies()) )
		{
			//We have spotted an enemy and wants to do something about it! Stop engine path, 
			//start up a sub-behaviour and use that until it becomes invalid.
			if (pPath)
			{
				 m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
				 m_PathPosition = m_pAI->m_PathFinder.SafeGetPathPosition(pPath->GetPosition(), 10, 2); //Assume path is airborne, and can be within 2 cells from a traversable cell
				 m_bReturningToPath = true;
			};
			m_pAI->ResetPathSearch();
			m_spSubBehaviour->OnTakeAction();
		}
		//FIX!
		//Is there a threat?
		//else if (m_pAI->OnThreatAlert())
		//{
			//We're looking for a threat
		//}
		//Should we step aside for someone?
		else if (m_pAI->OnStepAside((pPath) ? pPath->GetPosition() : CVec3Dfp32(_FP32_MAX), (pPath) ? m_pAI->GetAimPosition() + CVec3Dfp32::GetMatrixRow(pPath->GetPositionMatrix(), 0) : CVec3Dfp32(_FP32_MAX)))
		{
			m_pAI->ResetPathSearch();
		}
		else
		{
			//We have not spotted an enemy, do we have a path to patrol and 
			//a good follower (i.e. one that is close enough to us or our return position)?
			if ( pPath &&
				 pFollower &&
				 ((m_pAI->m_pGameObject->GetPosition().DistanceSqr(pFollower->GetPosition()) < 300*300) ||
				  (m_bReturningToPath &&
				   (m_PathPosition.DistanceSqr(pFollower->GetPosition()) < 300*300))) )
			{
				//We have a path and good follower
				//Start it if we haven't yet
				if (!m_bStarted)
				{
	 				m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
					m_bStarted = true;					
				};
				
				if (!m_bReturningToPath &&
					(m_PathPosition == CVec3Dfp32(_FP32_MAX)))
				{
					CMat43fp32 Mat = m_pAI->GetEnginePathMatrix(m_iPath);
					m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
					m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
				}
				
				//Check distance to path position
				fp32 DistSqr = CAI_Core::XYDistanceSqr(m_pAI->m_pGameObject->GetPosition(), m_PathPosition);

				m_pAI->Debug_RenderWire(m_pAI->m_pGameObject->GetPosition(), m_PathPosition);//DEBUG
				m_pAI->Debug_RenderWire(m_PathPosition, m_PathPosition + CVec3Dfp32(0,0,100), 0xffffffff, 0);

				//If we've fallen behind, check where we need to go, or if we've caught up with the path.
				if (m_bReturningToPath)
				{
					//Are we still behind the path?
					if (DistSqr < 16*16)
					{
						//We've caught up!
						m_bReturningToPath = false;
						CMat43fp32 Mat = m_pAI->GetEnginePathMatrix(m_iPath);
						m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
						m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
						DistSqr = CAI_Core::XYDistanceSqr(m_pAI->m_pGameObject->GetPosition(), m_PathPosition);
						m_bStop = false;
						m_pAI->ResetStuckInfo();	
					}
				}
				else
				{
					//Have we fallen behind just now?
					if (DistSqr > 32*32)
					{
						//Jupp, we must catch up with path. 
						m_bReturningToPath = true;
						m_PathPosition = m_pAI->m_PathFinder.GetPathPosition(m_PathPosition, 10, 2); 
						m_pAI->ResetStuckInfo();	
					};
				};

				//Should we follow path normally or are we behind and need to navigate our way to the path
				if (m_bReturningToPath) 
				{
					//We need to navigate if possible
					if (m_PathPosition == CVec3Dfp32(_FP32_MAX))
					{
						//Return position invalid, Propel path a bit so that we hopefully can get a position next frame..
						CMat43fp32 Mat;
						m_pAI->PropelPath(m_iPath, 32, true, true, &Mat);
						m_PathPosition = m_pAI->m_PathFinder.GetPathPosition(CVec3Dfp32::GetRow(Mat, 3), 10, 3);
						m_pAI->ResetPathSearch();
					}

					if (m_PathPosition == CVec3Dfp32(_FP32_MAX))
					{
						//Return position still invalid. Stay put.
						m_pAI->m_DeviceMove.Use();
					}
					else
				{
						//Return position ok! Stop path, and allow escape sequences
					m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
						m_pAI->OnMove(m_PathPosition, m_pAI->m_IdleMaxSpeed);
					}
				}
				else 
				{
					//We're following path normally. Propel path each frame when at stop, 
					//or propel to new position whenever we get close to path
					bool bPropelled = false;
					if (m_bStop || (DistSqr < Sqr(2 * m_pAI->m_IdleMaxSpeed)))
					{
						//Since we allow stops when propelling, path won't move when at a stop until it has 
						//been propelled the number of frames it should stop, regardless of distance
						CVec3Dfp32 PrevPos = m_PathPosition;
						CMat43fp32 Mat;
						m_pAI->PropelPath(m_iPath, 2 * m_pAI->m_IdleMaxSpeed, true, true, &Mat);
						m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
						m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
						m_pAI->ResetStuckInfo();
						bPropelled = true;

						//Check if (still) at stop
						if (m_PathPosition.AlmostEqual(PrevPos, 0.1))
							m_bStop = true;
						else
							m_bStop = false;
					}
					//Check if we've just become stuck
					else if (!m_pAI->m_bNoEscapeSeqMoves && !m_pAI->m_bIsStuck && m_pAI->IsStuck(m_PathPosition))
					{
						//We're stuck. Propel path a bit and reset stop flag. 
						//We'll start an escape move below.
						m_pAI->PropelPath(m_iPath, 32, true, true, NULL);
						m_bStop = false;
					}

					//Always look in the same direction as path
                    if (m_LookVector != CVec3Dfp32(_FP32_MAX))
					{
						m_pAI->AddAimAtPositionCmd(m_pAI->GetAimPosition() + m_LookVector * 100);
					}

					//Should we stop?
					if (m_bStop)
					{
						m_pAI->m_DeviceMove.Use();
						m_pAI->ResetStuckInfo();	
					}
					//Are we stuck?
					else if (m_pAI->m_bIsStuck)
					{
						//Escape sequence move
						m_pAI->OnEscapeSequenceMove(m_pAI->m_IdleMaxSpeed);
				}
				else
				{
						//Go to path position 
						m_pAI->AddMoveTowardsPositionCmd(m_PathPosition, m_pAI->m_IdleMaxSpeed);

						//If not propelled, propel path an extra time every few frames to avoid zig-zag behaviour
						if (!bPropelled && (m_pAI->m_Timer % 10 == 0))
						{
							CMat43fp32 Mat;
							m_pAI->PropelPath(m_iPath, 2 * m_pAI->m_IdleMaxSpeed, true, true, &Mat);
							m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
							m_pAI->ResetStuckInfo();
						}
					}
				};

				//Make a noise every few seconds
				// m_pAI->MakeNoise("Behaviour lead",CAI_Device_Sound::IDLE_IDLE, m_pAI->m_MinIdleSoundsInterval, m_pAI->m_MaxIdleSoundsInterval);
			}
			else
			{
				//Stop path
 				m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);

				//We do not have a path or must wait for follower. Set default heading toward follower, so that we look back at him after a while
				if (pFollower &&
					(m_pAI->m_Timer % 20 == 0))
					m_pAI->m_DefaultHeading = m_pAI->HeadingToPosition(m_pAI->m_pGameObject->GetPosition(), pFollower->GetPosition());
				
				//Wait
				m_pAI->OnIdle();
			};
		};
	};
};


//Stops path and sets up behaviour as when initialized
void CAI_Behaviour_Lead::Reset()
{
	MAUTOSTRIP(CAI_Behaviour_Lead_Reset, MAUTOSTRIP_VOID);
	CAI_Behaviour_Patrol::Reset();
	m_iFollower = 0;
};

//Change path and/or follower
void CAI_Behaviour_Lead::ReInit(int iParam1, int iParam2, const CVec3Dfp32& VecParam1, const CVec3Dfp32& VecParam2)
{
	MAUTOSTRIP(CAI_Behaviour_Lead_ReInit, MAUTOSTRIP_VOID);
	CAI_Behaviour_Patrol::ReInit(iParam1, iParam2, VecParam1, VecParam2);
	m_iFollower = iParam2;
};


//Behaviour is valid when there's a valid follower around
bool CAI_Behaviour_Lead::IsValid()
{
	MAUTOSTRIP(CAI_Behaviour_Lead_IsValid, false);
	//FIX
	return true;
};


//Path prio is greater when leading a human player
int CAI_Behaviour_Lead::GetPathPrio(fp32 _CameraScore)
{
	MAUTOSTRIP(CAI_Behaviour_Lead_GetPathPrio, 0);
	CWObject * pChar = m_pAI->IsValidAgent(m_iFollower);
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
		return m_spSubBehaviour->GetPathPrio(_CameraScore) + ((pChar && !m_pAI->IsBot(pChar->m_iObject)) ? 127 : 0);
	else
		return (128 * _CameraScore) + ((pChar && !m_pAI->IsBot(pChar->m_iObject)) ? 127 : 0);
};

