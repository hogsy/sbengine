#include "PCH.h"

#include "AICore.h"
#include "../WObj_CharMsg.h"

//Personalities

//Base personality class.

//Constructor
CAI_Personality::CAI_Personality()
{
	m_iType = NORMAL;
	m_pAI = NULL;
	m_iMeleeThreshold = 0;
	m_iRangedThreshold = 0;
	m_iPatience = 0;
	m_Unpredictability = 0.25f;
	SetPersonality(NORMAL);
};

void CAI_Personality::SetAI(CAI_Core* _pAI)
{
	m_pAI = _pAI;
};

int CAI_Personality::GetPersonality()
{
	return(m_iType);
};

void CAI_Personality::SetPersonality(int _iType)
{
	m_iType = _iType;
	switch(m_iType)
	{
	case NORMAL:
		m_lEstimationWeights[CAI_ActionEstimate::OFFENSIVE] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::DEFENSIVE] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::EXPLORATION] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::LOYALTY] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::LAZINESS] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::STEALTH] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::VARIETY] = 1.0f;
		break;
	case COWARD:
		m_lEstimationWeights[CAI_ActionEstimate::OFFENSIVE] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::DEFENSIVE] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::EXPLORATION] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::LOYALTY] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::LAZINESS] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::STEALTH] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::VARIETY] = 1.0f;
		break;
	case BERSERKER:
		m_lEstimationWeights[CAI_ActionEstimate::OFFENSIVE] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::DEFENSIVE] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::EXPLORATION] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::LOYALTY] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::LAZINESS] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::STEALTH] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::VARIETY] = 1.0f;
		break;
	case SNEAKER:
		m_lEstimationWeights[CAI_ActionEstimate::OFFENSIVE] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::DEFENSIVE] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::EXPLORATION] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::LOYALTY] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::LAZINESS] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::STEALTH] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::VARIETY] = 1.0f;
		break;
	case PASSIVE:
		m_lEstimationWeights[CAI_ActionEstimate::OFFENSIVE] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::DEFENSIVE] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::EXPLORATION] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::LOYALTY] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::LAZINESS] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::STEALTH] = 1.0f;
		m_lEstimationWeights[CAI_ActionEstimate::VARIETY] = 1.0f;
		break;
	}
};

//Change AI user
void CAI_Personality::ReInit(CAI_Core * _pAI)
{
	m_pAI = _pAI;
};

//Estimate action, and weigh estimate according to personality
int CAI_Personality::EstimateAction(CAI_Action * _pAction)
{
	if (!_pAction)
		return 0;

	int Res = 0;

	//Get estimate
	CAI_ActionEstimate Est = _pAction->GetEstimate();
	if (Est.IsValid())
	{
		//Weigh estimate
		for (int i = 0; i < CAI_ActionEstimate::NUM_PARAMS; i++)
		{
			Res += Est.Mult(i, m_lEstimationWeights[i]);
		}

		if (Res > 0)
		{
			//Introduce unpredictability
			Res = (int)(Res * (2 * m_Unpredictability * Random) + (1 - m_Unpredictability));

			//Introduce variety reduction (i.e. action that have already been taken 
			//for a while will get lower estimates)
			Res = (int)(Res * 1 - Min(1.0f, (_pAction->VarietyReduction() * m_lEstimationWeights[CAI_ActionEstimate::VARIETY]))); 

			//The above cannot reduce result to zero
			Res = Max(Res, 1);
		}
	}
 
	return Res;
};



#ifndef M_RTM
//Get some debug info about the bot
CStr CAI_Personality::GetDebugString()
{
	CStr str = "Personality: ";
	switch (m_iType)
	{
	case NORMAL:
		str += "Normal ";
		break;
	case COWARD:
		str += "Coward ";
		break;
	case BERSERKER:
		str += "Berserker ";
		break;
	case SNEAKER:
		str += "Sneaker ";
		break;
	case PASSIVE:
		str += "Passive ";
		break;
	};
	return str;		
};
#endif
