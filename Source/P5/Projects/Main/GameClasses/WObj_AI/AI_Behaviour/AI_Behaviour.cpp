#include "PCH.h"
#include "../AICore.h"
//#include "../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"

// Speeding up compilation
/*#include "AI_Behaviour_Ambush.cpp"
#include "AI_Behaviour_Avoid.cpp"
#include "AI_Behaviour_Engage.cpp"
#include "AI_Behaviour_Escape.cpp"
#include "AI_Behaviour_Explore.cpp"
#include "AI_Behaviour_Follow.cpp"
#include "AI_Behaviour_Guard.cpp"
#include "AI_Behaviour_Hold.cpp"
#include "AI_Behaviour_Lead.cpp"
#include "AI_Behaviour_Patrol.cpp"
#include "AI_Behaviour_Survive.cpp"*/

//Behaviours


//Base class

//Constructor
CAI_Behaviour::CAI_Behaviour()
{
	MAUTOSTRIP(CAI_Behaviour_ctor, MAUTOSTRIP_VOID);
	m_pAI = NULL;
	m_spSubBehaviour = NULL;
	m_Type = NULL;
};
	

//Constructor
CAI_Behaviour::CAI_Behaviour(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_Behaviour_ctor_2, MAUTOSTRIP_VOID);
	m_pAI = _pAI;
	m_spSubBehaviour = NULL;
	m_Type = NULL;
};


//Decides if the objective of¨the behaviour is fulfilled
bool CAI_Behaviour::IsValid()
{
	MAUTOSTRIP(CAI_Behaviour_IsValid, false);
	//A behaviour is by default valid
	return true;
};


//Resets the behaviour. This will always terminate any sub-behaviours.
void CAI_Behaviour::Reset()
{
	MAUTOSTRIP(CAI_Behaviour_Reset, MAUTOSTRIP_VOID);
	m_spSubBehaviour = NULL;
};

//Notifies behaviour that special, out-of-sequence moves have been made by the
//super behaviour if true is given, or that these movements have stopped if false.
void CAI_Behaviour::OnSuperSpecialMovement(bool _bOngoing)
{
	MAUTOSTRIP(CAI_Behaviour_OnSuperSpecialMovement, MAUTOSTRIP_VOID);
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Reset path search when stopped
	if (!_bOngoing)
		m_pAI->ResetPathSearch();
};


//Re-initiaizes the behaviour
void CAI_Behaviour::ReInit(int iParam1, int iParam2, const CVec3Dfp32& VecParam1, const CVec3Dfp32& VecParam2)
{
	MAUTOSTRIP(CAI_Behaviour_ReInit, MAUTOSTRIP_VOID);
	Reset();
};

//Change AI user
void CAI_Behaviour::ReInit(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_Behaviour_ReInit_2, MAUTOSTRIP_VOID);
	m_pAI = _pAI;
};


//Message handler for behaviour specific messages
bool CAI_Behaviour::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CAI_Behaviour_OnMessage, false);
	//No messages handled as default
	return false;
};



//Get current behaviour (i.e. sub-most behaviour)
CAI_Behaviour * CAI_Behaviour::GetSubBehaviour()
{
	MAUTOSTRIP(CAI_Behaviour_GetSubBehaviour, NULL);
	CAI_Behaviour * Res = this;
	while (Res && Res->m_spSubBehaviour && Res->m_spSubBehaviour->IsValid())
		Res = Res->m_spSubBehaviour;
	return Res;
};

//Get path prio for this behaviour (0..255), given the "closeness" value to human player cameras (0..1)
int CAI_Behaviour::GetPathPrio(fp32 _CameraScore)
{
	MAUTOSTRIP(CAI_Behaviour_GetPathPrio, 0);
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
		return m_spSubBehaviour->GetPathPrio(_CameraScore);
	else
		return 128 * _CameraScore;
};


 