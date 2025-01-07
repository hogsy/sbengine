#include "PCH.h"
#include "../AICore.h"


//The ambush behaviour; this'll be quite stupid until later I'm afraid.
//Constructor
CAI_Behaviour_Ambush::CAI_Behaviour_Ambush(CAI_Core * _pAI, int _iTarget, bool _bTrueTarget)
	: CAI_Behaviour_Engage(_pAI, _iTarget, _bTrueTarget)
{
	MAUTOSTRIP(CAI_Behaviour_Ambush_ctor, MAUTOSTRIP_VOID);
	m_Type = AMBUSH;
};



//Will wait for target to come within LOS before reverting to an engage behaviour, or try
//to get into a position to attack if targets back is turned. After attacking, or when being 
//attacked the bot will deal with enemy according to personality. 
//If no target is specified, the bot will ambush any enemy getting close. 
void CAI_Behaviour_Ambush::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Ambush_OnTakeAction, MAUTOSTRIP_VOID);
	//Can't take action if we don't have a valid AI
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//If we have a valid sub-behaviour, it should choose the action we should take
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
	{
		m_spSubBehaviour->OnTakeAction();
	}
	else
	{
		//We don't have a sub-behaviour to follow
		m_spSubBehaviour = NULL;

		//Are we under attack? If so we consider ourselves discovered, and must deal with our 
		//enemies in another way.
		if (m_pAI->m_KB.WasAttacked())
		{
			//Start up a sub-behaviour and use that until it becomes invalid.
			m_spSubBehaviour = m_pAI->m_spPersonality->OnDealWithEnemies();
			m_spSubBehaviour->OnTakeAction();
		}
		else
		{
			//Do we have a valid target?
			CAI_AgentInfo * pTarget = AcquireTarget();

			//Do we have a valid target that we can attack now?
			if (pTarget)
			{
				//We have target, can we attack?
				if (m_pAI->m_spPersonality->ShouldAttack(m_pAI->m_Weapon.GetWielded(), pTarget))
				{
					//Should add evasive manouevers as well...
						
					//Attack!
					m_pAI->OnAttack(pTarget);
				} 
				else
				{
					//Cannot attack target right now. Aim at him though.
					m_pAI->OnAim(pTarget);
				};
			}
			else
			{
				//No target. Camp.
				m_pAI->OnIdle();
			};

			//Try to get a ranged weapon
			if (!m_pAI->m_Weapon.GetWielded()->IsRangedWeapon())
			{
				m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH_NEXT);
			};
		};
	};
};



//Behaviour is valid when the bot hasn't been discovered (i.e. until it attacks or comes under 
//attack) and there is a potential target.
bool CAI_Behaviour_Ambush::IsValid()
{
	MAUTOSTRIP(CAI_Behaviour_Ambush_IsValid, false);
	//Not valid if AI object is invalid
	if (!m_pAI || !m_pAI->IsValid())
		return false;

	//Always valid if we have valid sub-behaviour
    if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
		return true;

	//Were this behaviour initialized with a true target?
	if (m_iTrueTarget)
		//Valid if we're still trying to engage true target, and this is valid.
		return (m_bTrueTarget && IsValidTarget(m_iTrueTarget));
	else
		//Valid if current target is valid and we're not under attack
		return ( (IsValidTarget(m_iTarget) != NULL) &&
				 !m_pAI->m_KB.WasAttacked() );
};



