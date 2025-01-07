#ifndef _INC_AI_ACTION_SMALLALIEN
#define _INC_AI_ACTION_SMALLALIEN

#include "../AI_Action.h"

class CAI_Action_SmallAlienDodge : public CAI_Action
{
public:
	CAI_Action_SmallAlienDodge(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_FORCED);

	//Mandatory: Move, Jump
	virtual int MandatoryDevices();

	//Valid if an enemy can attack us
	virtual bool IsValid();

	//Good defensively
	virtual CAI_ActionEstimate GetEstimate();

	//Dodge left or right randomly
	virtual void OnTakeAction();
};

#endif