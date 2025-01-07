#include "PCH.h"
#include "../AICore.h"
#include "../../Wobj_CharMsg.h"


//The engage behaviour. As in attacking, not a prelude to marriage :)

const fp32 CAI_Behaviour_Engage::MAX_UNCERTAINTY = 200.0f;


//Constructor
CAI_OpponentInfo::CAI_OpponentInfo(int _State, int _Action, fp32 _DistSqr)
{
	m_State = _State;
	m_Action = _Action;
	m_DistSqr = _DistSqr;
};


//Checks if the given server index points to a valid target.
//Returns pointer to targets agent info if so else NULL
CAI_AgentInfo * CAI_Behaviour_Engage::IsValidTarget(int _iObj)
{
	MAUTOSTRIP(CAI_Behaviour_Engage_IsValidTarget, NULL);
    //Is the indexed character valid and a spotted enemy?
	CAI_AgentInfo * pInfo = m_pAI->m_KB.GetAgentInfo(_iObj);
	if ( pInfo &&
		 m_pAI->IsValidAgent(pInfo->GetObject()) && 
		 (m_bTrueTarget || (pInfo->GetRelation() >= CAI_AgentInfo::ENEMY)) &&
		 (pInfo->GetAwareness() >= CAI_AgentInfo::SPOTTED)
	   ) 
		return pInfo;
	else
		return NULL;
};	

//Tries to find a good target.
//Returns pointer to targets character object if successful, else NULL
CAI_AgentInfo * CAI_Behaviour_Engage::FindTarget()
{
	MAUTOSTRIP(CAI_Behaviour_Engage_FindTarget, NULL);
	//Spot enemies
	if (!m_pAI->m_KB.OnSpotEnemies())
		//We couldn't spot any enemies
		return NULL;

	CAI_AgentInfo * pTarget = NULL;
	
	//If we are in fight mode, we always choose the character we're in fight mode with
	int iFighting = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISINFIGHTMODE), m_pAI->m_pGameObject->m_iObject);
	if (iFighting &&
		(pTarget = IsValidTarget(iFighting)))
	{
		m_iTarget = iFighting;
		return pTarget;
	}

	//Check all spotted characters to find any suitable target.
	//Find the most certain and closest valid target within _Radius distance	
	fp32 MinDistSqr = _FP32_MAX;
	fp32 DistSqr;
	CAI_AgentInfo * pInfo;
	for (int i = 0; i < m_pAI->m_KB.NumAgentInfo(); i++)
	{
		pInfo = m_pAI->m_KB.IterateAgentInfo(i);
		if (pInfo)
		{
			//Check if agent is a valid target
			if (IsValidTarget(pInfo->GetObjectID()))
			{	
				DistSqr = (pInfo->GetPosition()).DistanceSqr(m_pAI->m_pGameObject->GetPosition());
				if (DistSqr < MinDistSqr)
				{
					//Closer target found, continue checking...
					pTarget = pInfo;
					MinDistSqr = DistSqr;
				}
			};
		}
	};

	//Did we find a new target?
	if (pTarget && (m_iTarget != pTarget->GetObjectID()))
	{
		//Set current target to found target and return with success
		m_iTarget = pTarget->GetObjectID();
		return pTarget;
	}
	else
		//We didn't find a target
		return NULL;
};


//Check current target and acquire a new one if this is bad
CAI_AgentInfo* CAI_Behaviour_Engage::AcquireTarget()
{
	MAUTOSTRIP(CAI_Behaviour_Engage_AcquireTarget, NULL);
	CAI_AgentInfo * pTarget = NULL;
	
	//First check if any lost true target has reappeared or something...
	if ( m_iTrueTarget &&
		 !m_bTrueTarget &&
		 (pTarget = IsValidTarget(m_iTrueTarget)) )
	{
		//True target has been found again! Attack him instead.
		m_iTarget = m_iTrueTarget;
		m_bTrueTarget = true;
		//Set new path destination
		m_pAI->m_PathDestination = m_pAI->m_PathFinder.GetPathPosition(pTarget->GetObject());
		if (m_pAI->m_PathDestination == CVec3Dfp32(_FP32_MAX))
			//Target is airborne or something
			m_pAI->m_PathDestination = m_pAI->m_PathFinder.GetPathPosition(pTarget->GetObject(), true);

		ResetMeleeState();

		//Notify...
#ifndef M_RTM
		ConOut("'We meet again, " + m_pAI->GetDebugName(pTarget->GetObjectID()) + ". This time you will surely die!'");
#endif
	}
	//Check if current target is valid, and find a new target if it isn't.
	else if (!(pTarget = IsValidTarget(m_iTarget)))
	{
		//Target no longer valid.
		ResetMeleeState();

		if ((pTarget = FindTarget()))
		{
			//This newfound target cannot be a true target
			m_bTrueTarget = false;
			//Set new path destination
			m_pAI->m_PathDestination = m_pAI->m_PathFinder.GetPathPosition(pTarget->GetObject());
			if (m_pAI->m_PathDestination == CVec3Dfp32(_FP32_MAX))
				//Target is airborne or something
				m_pAI->m_PathDestination = m_pAI->m_PathFinder.GetPathPosition(pTarget->GetObject(), true);

			//Notify...
#ifndef M_RTM
			ConOut("'Kill " + m_pAI->GetDebugName(pTarget->GetObjectID()) + "'");
#endif
		}
	}
	else
	{
		//We have a valid target, but should we look for an alternative one? 
		//Do this only once every few frames, when the current target is "bad" given the 
		//weapon we're currently using, and the target is not a true one.
		CAI_AgentInfo * pChar;
		if ( !m_bTrueTarget &&
			 ((++m_TargetFindingTimer % 10) == 0) && 
			 m_pAI->m_Weapon.GetWielded()->IsValid() &&
			 !m_pAI->m_Weapon.GetWielded()->IsGoodTarget(pTarget) )
			//Time to check! 
			if ((pChar = FindTarget()))
			{
				//New target found (m_bTrueTarget is already false...)
				pTarget = pChar;
				//Set new path destination
				m_pAI->m_PathDestination = m_pAI->m_PathFinder.GetPathPosition(pTarget->GetObject());
				if (m_pAI->m_PathDestination == CVec3Dfp32(_FP32_MAX))
					//Target is airborne or something
					m_pAI->m_PathDestination = m_pAI->m_PathFinder.GetPathPosition(pTarget->GetObject(), true);
				
				//Notify...
#ifndef M_RTM
				ConOut("'Kill " + m_pAI->GetDebugName(pTarget->GetObjectID()) + " instead!'");
#endif
			}
	};

	return pTarget;
};	


//Get current action of given agent
int CAI_Behaviour_Engage::GetAction(int _iObj)
{
	if (!m_pAI || !m_pAI->m_pServer)
		return 0;
	
	//Ask agent about current action
	return m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FIGHTGETCURRENTACTION), _iObj);
};

//Resets melee-related values
void CAI_Behaviour_Engage::ResetMeleeState()
{
	MAUTOSTRIP(CAI_Behaviour_Engage_ResetMeleeState, MAUTOSTRIP_VOID);
	m_OpponentInfo = CAI_OpponentInfo();
	if (m_pAI)
	{
		m_pAI->m_MeleePos = CVec3Dfp32(_FP32_MAX);
		m_pAI->m_MeleeMode = CAI_Core::MELEE_NONE;
		m_pAI->m_MeleeTimer = -1;
	}
};


//Melee manoeuvering priority
int CAI_Behaviour_Engage::GetMeleePrio()
{
	MAUTOSTRIP(CAI_Behaviour_Engage_GetMeleePrio, 0);
	if (!m_pAI || !m_pAI->IsValid())
		return CAI_Resource_Activity::PRIO_MIN;

	//Only calculate new melee prio if it's invalid, or every ten frames
	if ((m_MeleePrio > CAI_Resource_Activity::PRIO_MIN) && 
		(m_MeleePrio < CAI_Resource_Activity::PRIO_MAX) &&
		(m_pAI->m_Timer % 10))
		return m_MeleePrio;

	//Calculate new path prio
	m_MeleePrio = CAI_Resource_Activity::PRIO_MIN;
	if (!m_pAI->IsValid(true) || !m_pAI->IsActive() || !m_iTarget)
		return m_MeleePrio;

	//Wielders of non-melee weapons have low prio
	if (!m_pAI->m_Weapon.GetWielded()->IsMeleeWeapon())
	{
		m_MeleePrio	= CAI_Resource_Activity::PRIO_MIN + 1;
	}
	else
	{
		//Melee weapon wielders get prio inversely relative to the squared distance to closest enemy
		fp32 MinDistSqr = 256*256;
		fp32 DistSqr;
		CAI_AgentInfo * pInfo;			
		for (int i = 0; i < m_pAI->m_KB.NumAgentInfo(); i++)
		{
			if ((pInfo = m_pAI->m_KB.IterateAgentInfo(i)) &&
				m_pAI->IsValidAgent(pInfo->GetObject()) &&	
				(pInfo->GetRelation() >= CAI_AgentInfo::ENEMY) &&
				((DistSqr = m_pAI->m_pGameObject->GetPosition().DistanceSqr(pInfo->GetObject()->GetPosition())) < MinDistSqr))
			{
				MinDistSqr = DistSqr;
			}
		}

		m_MeleePrio = (1.0f - (MinDistSqr / (fp32)(256*256))) * CAI_Resource_Activity::PRIO_MAX;
	}

	return m_MeleePrio;
};


//Will we engage the given opponent if we try to initiate melee now?
bool CAI_Behaviour_Engage::CanEngage(int _iObj)
{
	CWObject_Message GetOpponent(OBJMSG_CHAR_GETFIGHTCHARACTER);
	return (_iObj == m_pAI->m_pServer->Message_SendToObject(GetOpponent, m_pAI->m_pGameObject->m_iObject));
};


//Constructor
CAI_Behaviour_Engage::CAI_Behaviour_Engage(CAI_Core * _pAI, int _iTarget, bool _bTrueTarget)
	: CAI_Behaviour(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Engage_ctor, MAUTOSTRIP_VOID);
	m_iTarget = _iTarget;
	m_EvadePos = CVec3Dfp32(_FP32_MAX);
	m_TargetFindingTimer = 0;
	m_bTrueTarget = _bTrueTarget;
	if (m_bTrueTarget)
		m_iTrueTarget = m_iTarget;
	else
		m_iTrueTarget = 0;
	ResetMeleeState();
	m_SwitchWeaponTimer = 0;
	m_MeleePrio = CAI_Resource_Activity::PRIO_MIN - 1;
	m_HoldFireTick = 0;
	m_bSniper = true;
	m_Type = ENGAGE;
};


//Will move to position where the target can be attacked, and attack target. Evasive manoeuvers
//will be used according to personality.
//If no target is specified, the bot will engage any enemy spotted. 
void CAI_Behaviour_Engage::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Engage_OnTakeAction, MAUTOSTRIP_VOID);
	//m_pAI->AIOut("ENGAGE");

	//Can't take action if we don't have a valid AI
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Get useful weapon
	if (m_pAI->m_Weapon.GetWielded()->IsHarmless())
	{
		m_pAI->m_Weapon.SwitchToBestWeapon();
	};

	//If we have a valid sub-behaviour, it should choose the action we should take
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
	{
		m_spSubBehaviour->OnTakeAction();
	}
	else
	{
		//We don't have a sub-behaviour to follow
		m_spSubBehaviour = NULL;

		//Make sure we use a shield if we've got one every few frames
		if (m_pAI->m_Timer % 40 == 0)
			m_pAI->m_Item.WieldShield();		

		//Always complete a started taunt
		if (m_pAI->m_ActionFlags & CAI_Core::TAUNTING)
		{
			//Note that target may be dead or missing by this time. If this is the case, complete 
			//taunt without changing look direction.
			CAI_AgentInfo * pTarget = m_pAI->m_KB.GetAgentInfo(m_iTarget);
			if (pTarget)
			{
				// m_pAI->OnTaunt(pTarget);
			}
			else
			{
				//No target.
				m_pAI->m_DeviceLook.Use();
				// m_pAI->OnTaunt(NULL);				
			}

			//Needn't do anything else
			return;
		};


		//Get best current target
		CAI_AgentInfo * pTarget = AcquireTarget();

		//Do we have a valid target now?
		if (pTarget)
		{
			if (m_bTrueTarget)
			{
				//A true target is always considered an enemy
				pTarget->SetRelation(CAI_AgentInfo::ENEMY);
			}

			//Check if we should switch weapon every second or so (HACK fix properly later)
			if ((m_SwitchWeaponTimer == 0) &&
				(m_pAI->m_Timer % 20 == 0))
			{
				bool bUseRanged = true;	// **** CRAP ***
				if (bUseRanged && !m_pAI->m_Weapon.GetWielded()->IsRangedWeapon())
				{
					m_pAI->m_Weapon.SwitchToBestWeapon(CAI_WeaponHandler::RANGED_ONLY);
					m_SwitchWeaponTimer = Random * 60 + 40;
				}
				else if (!bUseRanged && m_pAI->m_Weapon.GetWielded()->IsRangedWeapon())
				{
					m_pAI->m_Weapon.SwitchToBestWeapon(CAI_WeaponHandler::MELEE_ONLY);
					m_SwitchWeaponTimer = Random * 60 + 40;
				}
			}

			//Call for backup at regular intervals
			if (m_pAI->m_Timer % 200 == 0)
			{
				m_pAI->RaiseAlarm(pTarget->GetObjectID());
			}

			//If target position is too uncertain, we must initiate a search and destroy behaviour
			if (false)
			{
				ResetMeleeState();
				m_pAI->ResetPathSearch();
				m_spSubBehaviour = MNew1(CAI_Behaviour_SearchNDestroy, m_pAI);
				m_spSubBehaviour->OnTakeAction();
			}
			else
			{
				//m_pAI->OnSpeak("ENGAGE");

				//We have target, engage! If we are using a melee weapon, then we will try to get into 
				//melee, taking evasive actions if we think it's necessary, and commence melee when close 
				//enough to do so. If we're using a ranged weapon then we will move towards target until 
				//we have line of sight and then take evasive actions while attacking the target, until it 
				//or we moves out of LOS, when we will start pursuing again.

				//If target is a player, update sniper status
                if (m_bSniper && m_pAI->IsPlayer(pTarget->GetObjectID()))
				{
					//We lose sniper status once we're hit, we think player has spotted us or
					//after any sniper penalty expires
					if ((m_pAI->m_KB.WasAttacked()) ||
						(m_pAI->m_SniperPenalty == 1) ||
						(pTarget->GetAwarenessOfUs() >= CAI_AgentInfo::SPOTTED))
					{
						m_bSniper = false;
						m_pAI->m_SniperPenalty = 0;
					}

					//Count down sniper penalty if applicable
					if (m_pAI->m_SniperPenalty > 0)
					{
						m_pAI->m_SniperPenalty--;
					}
				}

				//Are we using a ranged weapon?
				if (m_pAI->m_Weapon.GetWielded()->IsRangedWeapon())
				{
					//Not fighting in melee
					ResetMeleeState(); 

					//Hack! Set holdfire tick if not previously set
					if (m_HoldFireTick == 0)
					{	// How long for initial response
						m_HoldFireTick = m_pAI->m_pServer->GetGameTick() + m_pAI->m_Reaction * 10;
					}

					//Attack if personality deems we should
					if ((m_pAI->m_pServer->GetGameTick() > m_HoldFireTick) && 
						m_pAI->m_spPersonality->ShouldAttack(m_pAI->m_Weapon.GetWielded(), pTarget))
					{
						//m_pAI->AIOut(CStrF("Estimate: %d, Precision: %d, Distance: %f", m_pAI->m_Weapon.GetWielded()->GetEstimate(pTarget), m_pAI->m_Skill + m_pAI->m_Weapon.GetAimingScore(pTarget), (m_pAI->m_pGameObject->GetPosition()).Distance(pTarget->GetPosition())));//DEBUG
						// We should not keep firing for too long (as this is too good for us and bad for player :)

						//We are were we want to be so reset path
						m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);					
						m_pAI->ResetPathSearch();
						m_EvadePos = CVec3Dfp32(_FP32_MAX);

						//Set sniper penalty if applicable
						if (m_bSniper && (m_pAI->m_SniperPenalty == 0))
							m_pAI->m_SniperPenalty = 100; 

						//Attack!
						m_pAI->OnAttack(pTarget);

						//Stand still while attacking
						m_pAI->OnIdle();

						// FireDuration: Unskilled = 0.25s, Avg = 1.5s Skill100 = 2.75s
						// EvadeDuration: 1.5 - 4.5 seconds
						fp32 FireDuration = 5 + m_pAI->m_Skill * 0.5f;
						fp32 EvadeDuration = 20 * 3.0f * (0.5f + Random);
						// Hack! We max the time a guy can keep firing
						if ((!m_pAI->m_DeviceWeapon.IsAvailable())&&
							(m_HoldFireTick + FireDuration < m_pAI->m_pServer->GetGameTick()))
						{	
							m_HoldFireTick = m_pAI->m_pServer->GetGameTick() + EvadeDuration;
						}

						//Make angry noises sometimes
						//m_pAI->m_DeviceSound.MakeNoise(CAI_Device_Sound::ANGRY, 0.1, 1);
					}
					//Is target still good from where we are?
					else if (m_pAI->m_Weapon.GetWielded()->IsGoodTarget(pTarget))
					{
						//Aim at target
						// We move aiming to only occur when no evade is possible
						// m_pAI->OnAim(pTarget);

						//Decide whether we should take evasive action or not.
						// m_pAI->m_bEvasions = m_pAI->m_spPersonality->ShouldNonMeleeEvade();
						
						// Hack! Highly skilled guys can keep firing longer times
						if (!m_pAI->m_DeviceWeapon.IsAvailable())
						{
							m_pAI->m_bEvasions = false;
						}
						else
						{	// Evade for 2 seconds
							m_pAI->m_bEvasions = true;
						}
						
						//Evade any threats or stay put if we can't or don't need to evade anything
						//Use "stupid" evasion if we fail skill test (and are in melee)
						
						// Hack! Get the evade pos not closing on the target
						#define HACK_MAX_EVADERANGE	200
						if ((m_pAI->SqrDistanceToUs(m_EvadePos) > HACK_MAX_EVADERANGE * HACK_MAX_EVADERANGE)||(m_pAI->SqrDistanceToUs(m_EvadePos) < 16 * 16))
						{	// Only calculate once a second or so
							m_EvadePos = m_pAI->FindGroundPosition(m_pAI->HeadingToPosition(
																			pTarget->GetObject()->GetPosition(),
																			m_pAI->m_pGameObject->GetPosition()),
																	HACK_MAX_EVADERANGE,100,0.125f,0.375f,5);
							if (m_EvadePos != CVec3Dfp32(_FP32_MAX))
							{
								m_pAI->Debug_RenderWire(m_pAI->m_pGameObject->GetPosition(),m_EvadePos,0xffff0000,1.0f);
								m_pAI->Debug_RenderWire(m_EvadePos,m_EvadePos + CVec3Dfp32(0,0,32),0xffff0000,1.0f);
							}
						}

						// Removed OnSimpleMeleeEvade() as it's never true when not in melee anyway
						if ((m_pAI->m_bEvasions)&&
							(m_pAI->OnEvade(m_pAI->GetFocusPosition(pTarget->GetObject()),m_EvadePos)))
						{
						}
						else
						{
							m_pAI->OnAim(pTarget);
							m_pAI->OnIdle();
						}

						//We are were we want to be so reset path
						m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);					
						m_pAI->ResetPathSearch();
					}
					else
					{
						//We should follow target
						m_HoldFireTick = m_pAI->m_pServer->GetGameTick() + m_pAI->m_Reaction * 5;
						m_pAI->OnFollowTarget(pTarget);
					};

					//We're engaged in ranged combat, at the very least
					m_pAI->SetActivityScore(CAI_ActivityCounter::RANGEDCOMBAT);
				}
				//Are we using a melee weapon?
				else if (m_pAI->m_Weapon.GetWielded()->IsMeleeWeapon())
				{
					//Check if we're allowed to make melee manoeuvres
					bool bAllowedManoeuvres;
					if (m_pAI->m_KB.IsPlayerAlly())
						bAllowedManoeuvres = m_pAI->ms_AllowPlayerAllyMeleeManoeuvres.Poll(m_pAI->m_pGameObject->m_iObject, GetMeleePrio(), m_pAI->m_pServer->GetGameTick());
					else
						bAllowedManoeuvres = m_pAI->ms_AllowMeleeManoeuvres.Poll(m_pAI->m_pGameObject->m_iObject, GetMeleePrio(), m_pAI->m_pServer->GetGameTick());

					//Are we in melee with the target

					if (pTarget->InMelee())
					{
						// We are ine melee and should show the player this
						m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);

						//Are we in fight mode?
						//if (pTarget->GetObjectID() == m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISINFIGHTMODE), m_pAI->m_pGameObject->m_iObject))
						if (true)
						{
							//Don't do anything if at low activity or ot allowed melee manoeuvres
							if (!bAllowedManoeuvres ||	
								(m_pAI->m_ActivityLevel <= CAI_Core::ACTIVITY_LOW))
							{
								m_pAI->m_DeviceMove.Use();
								m_pAI->m_DeviceLook.Use();
							}
							else
							{
								//Try to figure out what opponent is doing at random intervals. 
								//A fighter with high skill will be more adept at reading the combat stance of his opponent
								//and will therefore do this more often. 
								//When not performing a combat manoeuvre, one has an increased combat awareness and will check 
								//for opponents manoeuvre more often
								int iCombatAwareness = 50 - m_pAI->ModSkill() / 2;
								if (m_pAI->m_Timer % Max(1, iCombatAwareness) == 0)
								{
									//We must make a very simple skill test in order to appraise the situation appropriately
									if (Random * 50 < m_pAI->m_Skill)
									{
										m_OpponentInfo.m_Action = GetAction(pTarget->GetObjectID());
										m_OpponentInfo.m_State = 0;

										if (m_pAI->m_pGameObject->m_iObject == m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISINFIGHTMODE), pTarget->GetObjectID()))
										{
											//Target is fighting us
											m_OpponentInfo.m_State |= CAI_OpponentInfo::FIGHTING;
										}
										else
										{
											//Target is not fighting us, hence vulnerable
											m_OpponentInfo.m_State |= CAI_OpponentInfo::VULNERABLE;;
										}
									}
									else
									{
										//We're not skilled enough to figure out what he's doing.
										m_OpponentInfo = CAI_OpponentInfo();
									}
								}

								//Hack! Just swing and move randomly
								if (true)
								{
									if (m_pAI->m_Timer % Max(1, iCombatAwareness) == 0)
									{
										int Manoeuvre = 0;

										//Decide move fwd/bwd
										fp32 DistSqr = m_pAI->SqrDistanceToUs(pTarget->GetObject());
										fp32 Rnd = Random;
										if (DistSqr > 64*64)
										{
											// If health is low we try to break off
											if (m_pAI->IsHurt(Rnd))
											{
												Manoeuvre |= CAI_Device_Melee::MOVE_BWD;
												Manoeuvre |= CAI_Device_Melee::BREAK_OFF;
											}
											else
											{
												if (Rnd < 0.4f)
													Manoeuvre |= CAI_Device_Melee::MOVE_FWD;
											}
										}
										else if (DistSqr < 32*32)
										{
											if (Rnd < 0.2f)
												Manoeuvre |= CAI_Device_Melee::MOVE_FWD;
											else if (Rnd > 0.7f)
												Manoeuvre |= CAI_Device_Melee::MOVE_BWD;
										}
										else
										{
											if (Rnd < 0.2f)
												Manoeuvre |= CAI_Device_Melee::MOVE_FWD;
											else if (Rnd > 0.9f)
												Manoeuvre |= CAI_Device_Melee::MOVE_BWD;
										}

										//Decide move left/right
										Rnd = Random;
										if (Rnd < 0.1f)
											Manoeuvre |= CAI_Device_Melee::MOVE_LEFT;
										else if (Rnd > 0.9f)
											Manoeuvre |= CAI_Device_Melee::MOVE_RIGHT;

										//Decide attack/block
										Rnd = Random;
										if (Rnd < 0.25f)
											Manoeuvre |= CAI_Device_Melee::ATTACK_PUNCH;
										else if (Rnd < 0.5f)
											Manoeuvre |= CAI_Device_Melee::BLOCK;

										m_pAI->m_DeviceMelee.Use(Manoeuvre);	
									}
									else
									{
										m_pAI->m_DeviceMelee.Use(0);
										m_pAI->m_DeviceMove.Use();
									}
								}
								else
								{
									//Let personality choose action, based on current weapon and target,
									//as well as assumed opponent info
									m_OpponentInfo.m_DistSqr = m_pAI->m_pGameObject->GetPosition().DistanceSqr(pTarget->GetObject()->GetPosition());
									int Action = m_pAI->m_spPersonality->ChooseMeleeAction(m_pAI->m_Weapon.GetWielded(), pTarget, m_OpponentInfo);				

									//Take action!
									m_pAI->m_Weapon.GetWielded()->OnUse(pTarget, Action);
								}
							}
						}
						else 
						{
							//Enter fight mode if possible, else charge
							if (CanEngage(pTarget->GetObjectID()))
								m_pAI->m_DeviceMelee.Use(CAI_Device_Melee::INITIATE);
							m_pAI->OnCharge(pTarget);
						}
					}
					//Can we attack?
					else if (m_pAI->m_Weapon.GetWielded()->GetEstimate(pTarget))
					{
						//Yup, we can attack, but we're not in melee with anyone (yet!). Thus we can charge with 
						//impunity! (target might be using a ranged weapon or haven't spotted us yet for example)
						if (bAllowedManoeuvres)
						{
							m_OpponentInfo.m_Action = 0;
							m_OpponentInfo.m_State = CAI_OpponentInfo::VULNERABLE;
							m_OpponentInfo.m_DistSqr = m_pAI->m_pGameObject->GetPosition().DistanceSqr(pTarget->GetObject()->GetPosition());
							int Action = m_pAI->m_spPersonality->ChooseMeleeAction(m_pAI->m_Weapon.GetWielded(), pTarget, m_OpponentInfo);				
							m_pAI->m_Weapon.GetWielded()->OnUse(pTarget, Action);

							//We're engaged in melee
							m_pAI->SetActivityScore(CAI_ActivityCounter::MELEE);
						}
						else
							m_pAI->OnWait(pTarget);
					}
					else
					{
						//Not in melee, and cannot attack. Must get closer. 
						
						//Check if we should switch (back) to primary weapon
						if ((m_pAI->m_UseFlags & CAI_Core::USE_SWITCHWEAPON) &&
							(m_SwitchWeaponTimer == 0) &&
							(m_pAI->m_Timer % 5 == 0) &&
							(Random < 0.4f))
						{
							m_pAI->m_Weapon.SwitchToBestWeapon();
							m_SwitchWeaponTimer = Random * 60 + 40;
						}

						//Charge after target and try to engage in melee
						ResetMeleeState();
						m_pAI->OnFollowTarget(pTarget);

						if (CanEngage(pTarget->GetObjectID()))
							m_pAI->m_DeviceMelee.Use(CAI_Device_Melee::INITIATE);

						//If we're using weapon or moving and close to target we count as engaged in melee
						if ((m_pAI->m_Weapon.GetWielded()->GetUseTimer() >= 0) ||
							((m_pAI->m_pGameObject->GetPosition() != m_pAI->m_pGameObject->GetLastPosition()) &&
							 (m_pAI->m_pGameObject->GetPosition().DistanceSqr(pTarget->GetObject()->GetPosition()) < 200*200)))
						{
							m_pAI->SetActivityScore(CAI_ActivityCounter::MELEE);
						}
					};
				}
				else
				{
					//Not using a weapon, switch weapon and evade if necessary
					m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH_NEXT);
					if (!m_pAI->OnEvade(m_pAI->GetFocusPosition(pTarget->GetObject())))
						m_pAI->OnIdle();

					ResetMeleeState();

					//We are were we want to be so reset path
					m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);					
					m_pAI->ResetPathSearch();
				};

				//Change default heading to where we are now facing.
				m_pAI->m_DefaultHeading = m_pAI->GetHeading(m_pAI->m_pGameObject);
			}
		} 
		else
		{
			//No target, but we might be in danger from non-spotted characters or other stuff

			//Are we taking evasive actions?
			if (m_pAI->m_bEvasions)
			{
				//Should we stop taking evasive actions?
				if (m_pAI->m_spPersonality->IsSafe())
					m_pAI->m_bEvasions = false;
			}
			//Should we start taking evasive actions?
			else if (m_pAI->m_spPersonality->IsInDanger())
			{
				//Jupp. 
				m_pAI->m_bEvasions = true;
			};

			//Should we take evasive actions?
			if (m_pAI->m_bEvasions)
			{
				/*
				//There's danger to evade. Scan for unknown threats if there are any
				m_pAI->OnScanForUnknownThreats();				
				*/

				//Evade or cower in terror if we cannot evade
				if (!m_pAI->OnEvade())
					m_pAI->OnIdle();
			}
			else
			{
				//Not in danger. Pick lint from belly-button. 
				m_pAI->OnIdle();
			};

			//Reset path
			m_pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
			m_pAI->ResetPathSearch();	

			ResetMeleeState();
		};

		//Count down timers
		if (m_SwitchWeaponTimer > 0)
			m_SwitchWeaponTimer--;
	};
};


//Resets the current target to the true target if there is one, and terminates any sub-behaviours.
void CAI_Behaviour_Engage::Reset()
{
	MAUTOSTRIP(CAI_Behaviour_Engage_Reset, MAUTOSTRIP_VOID);
	m_spSubBehaviour = NULL;
	if (m_iTrueTarget)
	{
		m_iTarget = m_iTrueTarget;
		m_bTrueTarget = true;
	};
	m_TargetFindingTimer = 0;
};



//Behaviour is valid when there is a potential target, or if sub behaviour is valid.
bool CAI_Behaviour_Engage::IsValid()
{
	MAUTOSTRIP(CAI_Behaviour_Engage_IsValid, false);
	//Not valid if AI object is invalid
	if (!m_pAI || !m_pAI->IsValid())
		return false;

	//Always valid if we have valid sub-behaviour
    if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
		return true;

	//Were this behaviour initialized with a true target?
	bool bRes;
	if (m_iTrueTarget)
		//Valid if we're still trying to engage true target, and this is valid.
		bRes = (m_bTrueTarget && IsValidTarget(m_iTrueTarget));
	else
		//Valid if current target is valid or we're under attack
		bRes = ( (IsValidTarget(m_iTarget) != NULL) ||
				 m_pAI->m_KB.WasAttacked(100) );
	if (!bRes)
	{
		ResetMeleeState();
	};

	return bRes;
};


//Makes behaviour defensive when ongoing, and resets it when not.
void CAI_Behaviour_Engage::OnSuperSpecialMovement(bool _bOngoing)
{
	MAUTOSTRIP(CAI_Behaviour_Engage_OnSuperSpecialMovement, MAUTOSTRIP_VOID);
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Set melee manoeuvre accordingly
//	m_iManoeuvre = (_bOngoing) ? DODGE : WAIT;

	//Set evasions flag accordingly
	m_pAI->m_bEvasions = _bOngoing;

	//Reset pathfinding stuff if stopped
	if (!_bOngoing)
		m_pAI->ResetPathSearch();
};


//Re-init target and truetarget flag (cast iParam2 to bool)
void CAI_Behaviour_Engage::ReInit(int iParam1, int iParam2, const CVec3Dfp32& VecParam1, const CVec3Dfp32& VecParam2)
{
	MAUTOSTRIP(CAI_Behaviour_Engage_ReInit, MAUTOSTRIP_VOID);
	Reset();	
	m_iTarget = iParam1;
	m_bTrueTarget = iParam2 != 0;
	if (m_bTrueTarget)
		m_iTrueTarget = m_iTarget;
	else
		m_iTrueTarget = 0;
};



//Always get higher path prio than normal, but especially if wielding melee weapon
int CAI_Behaviour_Engage::GetPathPrio(fp32 _CameraScore)
{
	MAUTOSTRIP(CAI_Behaviour_Engage_GetPathPrio, 0);
	int Base;
	if (m_spSubBehaviour && m_spSubBehaviour->IsValid())
		Base = m_spSubBehaviour->GetPathPrio(_CameraScore);
	else
		Base = _CameraScore * 128;

	if (m_pAI->m_Weapon.GetWielded()->IsMeleeWeapon())
		return Base + 128;
	else
		return Base + 64;
};
