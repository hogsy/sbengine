#include "PCH.h"
#include "../AICore.h"
//#include "../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"


//The guard behaviour

//Constructor
CAI_Behaviour_Guard::CAI_Behaviour_Guard(CAI_Core * _pAI, int _iVIP)
	: CAI_Behaviour_Follow(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Guard_ctor, MAUTOSTRIP_VOID);
	return;
	// m_Type = GUARD;
};


//Will follow VIP and attack any enemy getting close to or attacking him or self. If not 
//given a VIP when initializing, bot will revert to follow behaviour.
void CAI_Behaviour_Guard::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Guard_OnTakeAction, MAUTOSTRIP_VOID);
	/*
	//Can't take action if we don't have a valid AI

	if (!m_pAI || !m_pAI->IsValid())
		return;

	if(!m_spSubBehaviour)
		m_spSubBehaviour = DNew(CAI_Behaviour_Follow) CAI_Behaviour_Follow(m_pAI);
	
	m_spSubBehaviour->OnTakeAction();
	*/
};





