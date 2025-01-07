#ifndef _INC_AI_PERSONALITY
#define _INC_AI_PERSONALITY

#include "AI_Action/AI_Action.h"

class CAI_AgentInfo;
class CAI_ActionEstimate;

//Base personality class.
class CAI_Personality : public CReferenceCount
{
protected:
	//Pointer to the AI
	CAI_Core* m_pAI;

	//Current melee threshold
	int m_iMeleeThreshold;

	//Current ranged threshold
	int m_iRangedThreshold;

	//Current patience
	int m_iPatience;

	//Action estimate parameter weights. This is the chief personality effect on behaviour..
	fp32 m_lEstimationWeights[CAI_ActionEstimate::NUM_PARAMS];
	
	//Any action estimate is always multiplicatively modified by a number on the interval (1 - u)..(1 + u), where u is this value.
	fp32 m_Unpredictability;

public:
	//"Personality" types
	enum {
		NORMAL = PERSONALITY_ENUM_BASE,
		COWARD,
		BERSERKER,
		SNEAKER,
		PASSIVE,

		PERSONALITY_MAX = PASSIVE,
	};
	int m_iType;

	//Constructors
	CAI_Personality();
	void SetAI(CAI_Core* _pAI);
	void SetPersonality(int _iType);
	int GetPersonality();

	//Change AI user
	void ReInit(CAI_Core* _pAI);

	//Estimate action, and weigh estimate according to personality
	int EstimateAction(CAI_Action * _pAction);

#ifndef M_RTM
	//Get some debug info about the bot
	virtual CStr GetDebugString();
#endif
};

#endif
