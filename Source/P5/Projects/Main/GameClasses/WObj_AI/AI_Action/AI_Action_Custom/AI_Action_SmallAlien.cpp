#include "PCH.h"
#include "AI_Action_SmallAlien.h"
#include "../../AICore.h"

//CAI_Action_SmallAlienDodge//////////////////////////////////////////////////////////////////////////
CAI_Action_SmallAlienDodge::CAI_Action_SmallAlienDodge(CAI_ActionHandler * _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = SMALLALIEN_DODGE;
	m_TypeFlags = TYPE_MOVE | TYPE_DEFENSIVE;
};

//Mandatory: Move, Jump
int CAI_Action_SmallAlienDodge::MandatoryDevices()
{
	return (1 << CAI_Device::MOVE) | (1 << CAI_Device::JUMP);
};

//Valid if an enemy can attack us
bool CAI_Action_SmallAlienDodge::IsValid()
{
	//FIX
	return CAI_Action::IsValid() && false;
};

//Good defensively
CAI_ActionEstimate CAI_Action_SmallAlienDodge::GetEstimate()
{
	CAI_ActionEstimate Est;
	if (IsValid())
	{
		//Default score 65
		Est.Set(CAI_ActionEstimate::OFFENSIVE, -10);
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 75);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, 0);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 0);
	}
	return Est;
};

//Dodge left or right randomly
void CAI_Action_SmallAlienDodge::OnTakeAction()
{
	//FIX
};
