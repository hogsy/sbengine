#include "PCH.h"
#include "../../AICore.h"
#include "AI_Behaviour_Custom.h"
 

CAI_Behaviour_Turret::CAI_Behaviour_Turret(CAI_Core * _pAI)
	: CAI_Behaviour_Engage(_pAI)
{
	m_Type = TURRET;
}


void CAI_Behaviour_Turret::OnTakeAction()
{
//	if ()
}


