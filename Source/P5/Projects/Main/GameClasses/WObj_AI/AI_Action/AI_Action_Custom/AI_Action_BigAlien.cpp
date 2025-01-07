#include "PCH.h"
#include "AI_Action_BigAlien.h"
#include "../../AICore.h"
#include "../../AI_Custom/AICore_BigAlien.h"

//CAI_Action_BigAlienSleep//////////////////////////////////////////////////////////////////////////
CAI_Action_BigAlienSleep::CAI_Action_BigAlienSleep(CAI_ActionHandler * _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = BIGALIEN_SLEEP;
	m_TypeFlags = 0;
	m_YawnTick = 0;
};

//Mandatory: Move
int CAI_Action_BigAlienSleep::MandatoryDevices()
{
	return (1 << CAI_Device::MOVE);
};

//Valid if in sleeping mode
bool CAI_Action_BigAlienSleep::IsValid()
{
	return CAI_Action::IsValid() && 
		(AI()->OnMessage(CWObject_Message(CAI_Core_BigAlien::OBJMSG_GETACTIVITYMODE)) == CAI_Core_BigAlien::MODE_SLEEPING);
};

// We can attack anything if we're not sleeping
bool CAI_Action_BigAlienSleep::IsValid(CAI_AgentInfo* _pTarget)
{
	if (_pTarget == NULL)
	{
		return(false);
	}
	else
	{
		return(IsValid());
	}
};

//Always very good
CAI_ActionEstimate CAI_Action_BigAlienSleep::GetEstimate()
{
	CAI_ActionEstimate Est;
	if (IsValid())
	{
		//Default score 100
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, 100);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 0);
	}
	return Est;
};