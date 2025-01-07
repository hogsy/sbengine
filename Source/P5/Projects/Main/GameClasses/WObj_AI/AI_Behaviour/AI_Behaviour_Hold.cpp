#include "PCH.h"
#include "../AICore.h"
//#include "../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"


//The hold behaviour

//Constructor
CAI_Behaviour_Hold::CAI_Behaviour_Hold(CAI_Core * _pAI, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Look, fp32 _HesitateDistance, fp32 _MaxDistance)
	: CAI_Behaviour(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Hold_ctor, MAUTOSTRIP_VOID);
	m_Pos = _Pos;
	m_Look = _Look;
	if (m_pAI && m_pAI->IsValid())
		//Find path position of object. Assumes object can be within 3 cells of a traversable cell in the xy-plane
		m_PathPos = m_pAI->m_PathFinder.GetPathPosition(_Pos, -1, 3);
	else
		m_PathPos = CVec3Dfp32(_FP32_MAX);
	m_MaxDistSqr = (_MaxDistance != -1) ? Sqr(_MaxDistance) : -1;
	m_HesitateDistSqr = (_HesitateDistance != -1) ? Sqr(_HesitateDistance) : -1;;
	m_bMadeSpecialMove = false;
	m_SpecialMoveTimer = 0;
	m_Type = HOLD;
};


//Will go to position and stand fast, scanning around, until enemy spotted. will then deal 
//with enemy accordning to personality. If no position is given, bot will always hold at 
//current position.
void CAI_Behaviour_Hold::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Hold_OnTakeAction, MAUTOSTRIP_VOID);
	//if (CAI_Core::ms_bRemoveFunctionality)
	//	return;

	//Can't take action if we don't have a valid AI
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//_Never_ move if maxdistance is 0 and we're at position
	if ((m_MaxDistSqr == 0) &&
		((m_PathPos == CVec3Dfp32(_FP32_MAX)) ||
		 ((m_pAI->m_pGameObject->GetPosition()).DistanceSqr(m_PathPos) < 16*16)))
	{
		m_pAI->m_DeviceMove.Use(0);
	};

	//If we have a valid sub-behaviour, it should choose the action we should take, unless 
	//this would take us too far from our hold-position.
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
	{
		//Should we make a special move to maintain our distance to our hold position?
		//(This may cause a bot to jiggle if distances are bad and if the bot must make a 
		//detour to reach hold position, he might not be able to get there at all,)

		//If we got maxdistance 0 and are at position we never move for any reason.
		if ((m_MaxDistSqr == 0) &&
			((m_PathPos == CVec3Dfp32(_FP32_MAX)) ||
			 ((m_pAI->m_pGameObject->GetPosition()).DistanceSqr(m_PathPos) < 16*16)))
		{
			//Don't make special move
			m_bMadeSpecialMove = false;
		}
		else if ( (m_PathPos != CVec3Dfp32(_FP32_MAX)) &&
			 //Are we more than max distance away from position?
			 (( (m_MaxDistSqr != -1) &&
			    ((m_pAI->m_pGameObject->GetPosition()).DistanceSqr(m_PathPos) > m_MaxDistSqr) )
			  ||
			  //If we're more than the hesitate distance away, there is a 20% chance we will start back every frame
			  ( (m_HesitateDistSqr != -1) &&
			    ((m_pAI->m_pGameObject->GetPosition()).DistanceSqr(m_PathPos) > m_HesitateDistSqr) &&
			    (m_bMadeSpecialMove || (Random < 0.1f))) ) )
		{
			//We should make a special move, so if we've just started making special moves, reset path stuff
			if (!m_bMadeSpecialMove)
			{
				m_bMadeSpecialMove = true;
				m_SpecialMoveTimer = Random * 40 + 20;
				m_pAI->ResetPathSearch();
				m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
			};

			//Lock move device so that subbehaviour won't try to move.
			m_pAI->m_DeviceMove.Lock();
			
			//Notify subbehaviour that we're making special movement
			m_spSubBehaviour->OnSuperSpecialMovement(true);
		}
		else if (m_bMadeSpecialMove)
		{
			//We shouldn't make a special move, but we did one last frame. 
			m_bMadeSpecialMove = false;

			//Notify subbehaviour that we've stopped making special movement
			m_spSubBehaviour->OnSuperSpecialMovement(false);
		};

		//Make sure we don't switch weapon if we've got a max distance
		int SaveUseFlags = m_pAI->m_UseFlags;
		if (m_MaxDistSqr != -1)
		{
			m_pAI->m_UseFlags &= ~CAI_Core::USE_SWITCHWEAPON;
		}

		//Let subbehaviour take actions. If we're making special movement, 
		//then move device won't be available to subbehaviour
		m_spSubBehaviour->OnTakeAction();

		//Reset use flags
		m_pAI->m_UseFlags = SaveUseFlags;
		
		//Should we make a special move?
		if (m_bMadeSpecialMove)
		{
			//Free move device
			m_pAI->m_DeviceMove.Free();

			//Pause until special move timer has expired
			if (m_SpecialMoveTimer > 0)
			{
				m_SpecialMoveTimer--;
				m_pAI->m_DeviceMove.Use();
			}
			//Evade if there is need and can be done
			else if (!m_pAI->OnEvade(CVec3Dfp32(_FP32_MAX), m_PathPos))
			{
				//No evasion. If we're currently moving back to hold position, continue.
				//Otherwise there's a 10% chance of us starting back, or alternatively we stand still.
				if ( (m_pAI->m_PathDestination == m_PathPos) ||
					 (Random < 0.1f) )
				{
					m_pAI->OnMove(m_PathPos, m_pAI->m_IdleMaxSpeed);
				}
				else
				{
					m_pAI->m_DeviceMove.Use(0);
				};
			}
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
		else if (m_pAI->OnStepAside(m_PathPos))
		{
			m_pAI->ResetPathSearch();
		}
		else
		{
			//(Re)set default heading if we've got a specified look direction
			if (m_Look != CVec3Dfp32(_FP32_MAX))
				m_pAI->m_DefaultHeading = m_Look[2];

			//We have not spotted an enemy, do we have a position to hold?
			if (m_PathPos != CVec3Dfp32(_FP32_MAX))
			{
				//m_pAI->Debug_RenderWire(m_PathPos, m_pAI->m_pGameObject->GetPosition());

				//Always return to position after leaving it.
				if ( (m_pAI->m_pGameObject->GetPosition()).DistanceSqr(m_PathPos) > 32*32 )
				{
					m_pAI->OnMove(m_PathPos, m_pAI->m_IdleMaxSpeed);

					if (m_pAI->m_bReachedPathDestination)
					{
						m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
						m_pAI->ResetPathSearch();
					};
				}
				else
				{
					//We're back
					m_pAI->OnIdle();
				};
			}
			else
			{
				//If we have a position, try to find a path position within 10 cells, probably in vain...
				//Just do this once every 100 frames
				if ((m_Pos != CVec3Dfp32(_FP32_MAX)) &&
					(m_pAI->m_Timer % 100 == 0))
				{
					m_PathPos = m_pAI->m_PathFinder.GetPathPosition(m_Pos, -1, 10);
					if (m_PathPos == CVec3Dfp32(_FP32_MAX))
						//We failed to get a position
						m_Pos = CVec3Dfp32(_FP32_MAX);
				};

				//We have no position to hold. Just stay put.
				m_pAI->OnIdle();
			};

			//Make a noise every few seconds
			// m_pAI->MakeNoise("Behaviour hold",CAI_Device_Sound::IDLE_IDLE, m_pAI->m_MinIdleSoundsInterval, m_pAI->m_MaxIdleSoundsInterval);
		};
	};
};


//Re-init maxdistamce + 1, hesitatedistance + 1 position and look
void CAI_Behaviour_Hold::ReInit(int iParam1, int iParam2, const CVec3Dfp32& VecParam1, const CVec3Dfp32& VecParam2)
{
	MAUTOSTRIP(CAI_Behaviour_Hold_ReInit, MAUTOSTRIP_VOID);
	Reset();
	m_Pos = VecParam1;
	m_Look = VecParam2;
	m_MaxDistSqr = (iParam1) ? Sqr(iParam1 - 1) : -1;
	m_HesitateDistSqr = (iParam2) ? Sqr(iParam2 - 1) : -1;
	if (m_pAI && m_pAI->IsValid())
		//Find path position of object. Assumes object can be within 3 cells of a traversable cell in the xy-plane
		m_PathPos = m_pAI->m_PathFinder.GetPathPosition(VecParam1, -1, 3);
	else
		m_PathPos = CVec3Dfp32(_FP32_MAX);
};



