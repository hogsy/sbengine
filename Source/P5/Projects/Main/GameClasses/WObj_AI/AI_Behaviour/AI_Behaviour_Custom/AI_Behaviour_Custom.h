#ifndef _INC_AI_BEHAVIOUR_CUSTOM
#define _INC_AI_BEHAVIOUR_CUSTOM

//Custom behaviours
class CAI_Behaviour_Turret : public CAI_Behaviour_Engage
{
public:
	//Initializes behaviour
	CAI_Behaviour_Turret(CAI_Core * _pAI);

	//Will fire at any enemies in LOS
	virtual void OnTakeAction();
};

#endif