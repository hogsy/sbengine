#ifndef _INC_AI_ACTION_BIGALIEN
#define _INC_AI_ACTION_BIGALIEN

#include "../AI_Action.h"

class CAI_Action_BigAlienSleep : public CAI_Action
{
protected:
	//Don't yawn until this tick
	int m_YawnTick;

public:
	CAI_Action_BigAlienSleep(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_FORCED);

	//Mandatory: Move
	virtual int MandatoryDevices();

	//Valid if in sleeping mode
	virtual bool IsValid();
	virtual bool IsValid(CAI_AgentInfo* _pTarget);

	//Always very good
	virtual CAI_ActionEstimate GetEstimate();

	//Hang sleeping, yawn once in a while. Drop down if attacked
	virtual void OnTakeAction();
};


class CAI_Action_BigAlienPursue : public CAI_Action_Pursue
{
protected:
	//Act frustrated for a while when we can't get to target
	int m_FrustrationTick;

public:
	CAI_Action_BigAlienPursue(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);

	//Pursue if we can, otherwise act frustrated for a while
	virtual void OnTakeAction();
};

#endif