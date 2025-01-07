#include "PCH.h"
#include "../AICore.h"
//#include "../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
//#include "../../../../../Shared/MOS/Classes/GameWorld/WBlockNav.h"
//#include "../../../../../Shared/MOS/Classes/GameWorld/Server/WServer.h"
//#include "MMath.h"

//The escape behaviour

//Have we reached an escape point?
bool CAI_Behaviour_Escape::HasEscaped()
{
	MAUTOSTRIP(CAI_Behaviour_Escape_HasEscaped, false);
	//Are we within 32 units of an escape point?
	for (int i = 0; i < m_lPos.Len(); i++)
	{
		if ( (m_pAI->m_pGameObject->GetPosition()).DistanceSqr(m_lPos[i]) < 32*32 )
			//We'vereached an escape point!
			return true;
	};
	//We're not at any sacpe point
	return false;
};


//Constructor
CAI_Behaviour_Escape::CAI_Behaviour_Escape(CAI_Core * _pAI, const TArray<CVec3Dfp32>& _lPos)
	: CAI_Behaviour(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Escape_ctor, MAUTOSTRIP_VOID);
	//Set up escape positions array
	if ( m_pAI && 
		 m_pAI->IsValid() &&
		 m_pAI->m_PathFinder.GridPF() )
	{
		//Add the path positions of all positions in given array to ecape position array.
		CVec3Dfp32 PathPos;
		for (int i = 0; i < _lPos.Len(); i++)
		{
			if ( (PathPos = m_pAI->m_PathFinder.GetPathPosition(_lPos[i], -1, 3)) != CVec3Dfp32(_FP32_MAX) )
				m_lPos.Add(PathPos);
		};
	}
	else
	{
		//We better be able to get navgrid later, and the given positions better be close to the
		//path positions, or we're in deep shit...Just add the possibly valid positions and hope 
		//for the best.
		for (int i = 0; i < _lPos.Len(); i++)
			if (_lPos[i] != CVec3Dfp32(_FP32_MAX))
				m_lPos.Add(_lPos[i]);
	};
	
	iCurrent = -1;

	m_Type = ESCAPE;
};


//Will attempt to reach randomly selected escape point. If encountering enemy, bot will 
//either try to resch another escape point or deal with enemy, according to personality.
//If no escape points are given, bot will revert to survive behaviour.
void CAI_Behaviour_Escape::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Escape_OnTakeAction, MAUTOSTRIP_VOID);
	//DEPRECATED

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

		//Check for escape positions
		if (m_lPos.Len() == 0)
		{
			//We don't have any escape positions. Revert to follow behaviour
			m_spSubBehaviour = MNew1(CAI_Behaviour_Follow, m_pAI);
			m_spSubBehaviour->OnTakeAction();
		}

		//Look for enemies
		if ( (m_pAI->m_KB.OnSpotEnemies()) &&
			 (m_spSubBehaviour = m_pAI->m_spPersonality->OnDealWithEnemies()) )
		{
			//We have spotted an enemy and wants to do something about it! Perhaps we should try to circumvent enemy instead if possible...
			//Start up a sub-behaviour and use that until it becomes invalid.
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
		else if (m_pAI->OnStepAside(m_lPos[iCurrent]))
		{
			m_pAI->ResetPathSearch();
		}
		else
		{
			//Always stop if at low activity
			if (m_pAI->m_ActivityLevel <= CAI_Core::ACTIVITY_LOW)
				m_pAI->m_DeviceMove.Use();

			//We have not spotted an enemy, and we have escape positions. Should we try to escape?
			if (!HasEscaped())
			{
				//Get a random new current escape position to go for, if we don't have one, 
				//or if we fail to find a path to one.
				if ( (iCurrent == -1) ||
					 (m_pAI->m_SearchStatus == CWorld_BlockNav::SEARCH_NO_PATH) ||
					 (m_pAI->m_SearchStatus == CWorld_BlockNav::SEARCH_DESTINATION_INTRAVERSABLE) ||
					 (m_pAI->m_SearchStatus == CWorld_BlockNav::SEARCH_DESTINATION_OUTOFBOUNDS) )
				{
					iCurrent = Random * m_lPos.Len();
					m_pAI->ResetPathSearch();
				};
				
				//Try to reach current escape position 
				m_pAI->OnMove(m_lPos[iCurrent], m_pAI->m_IdleMaxSpeed);
			}
			else
			{
				//We have already escaped. Catch our breath.
				m_pAI->OnIdle();
			};

			//Make a noise every few seconds
			// m_pAI->MakeNoise("Behaviour escape",CAI_Device_Sound::IDLE_IDLE, m_pAI->m_MinIdleSoundsInterval, m_pAI->m_MaxIdleSoundsInterval);
		};
	};
};


//Behaviour is valid if not at an escape position
bool CAI_Behaviour_Escape::IsValid()
{
	MAUTOSTRIP(CAI_Behaviour_Escape_IsValid, false);
	//Not valid if AI object is invalid
	if (!m_pAI || !m_pAI->IsValid())
		return false;

	return ( HasEscaped() || 
		     (m_spSubBehaviour && m_spSubBehaviour->IsValid()) );
};

//Re-init with max two positions
void CAI_Behaviour_Escape::ReInit(int iParam1, int iParam2, const CVec3Dfp32& VecParam1, const CVec3Dfp32& VecParam2)
{
	MAUTOSTRIP(CAI_Behaviour_Escape_ReInit, MAUTOSTRIP_VOID);
	Reset();	

	//Set up escape positions array
	m_lPos.Clear();
	iCurrent = 0;
	if ( m_pAI && 
		 m_pAI->IsValid() )
	{
		//Add the path positions of any valid position
		CVec3Dfp32 PathPos;
		if ( (PathPos = m_pAI->m_PathFinder.GetPathPosition(VecParam1, -1, 3)) != CVec3Dfp32(_FP32_MAX) )
			m_lPos.Add(PathPos);
		if ( (PathPos = m_pAI->m_PathFinder.GetPathPosition(VecParam2, -1, 3)) != CVec3Dfp32(_FP32_MAX) )
			m_lPos.Add(PathPos);
	}
};



