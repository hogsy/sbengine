#include "PCH.h"
#include "AICore_Darkling.h"
#include "../../WObj_Char.h"
#include "../../WObj_CharMsg.h"
#include "../../WObj_Misc/WObj_ScenePoint.h"

MRTC_IMPLEMENT(CAI_Core_Darkling, CAI_Core);

#define MACRO_WRITEVERIFY {uint32 Apa = 0x81920467; _pFile->WriteLE(Apa);}
#define MACRO_READVERIFY {uint32 Apa; _pFile->ReadLE(Apa); M_ASSERT(Apa == 0x81920467, CStrF("Load/save mismatch in file '%s' on line %i", __FILE__, __LINE__)); };

//Copy constructor
CAI_Core_Darkling::CAI_Core_Darkling(CAI_Core * _pAI_Core)
: CAI_Core(_pAI_Core)
{
	MAUTOSTRIP(CAI_Core_Darkling_ctor, MAUTOSTRIP_VOID);

	m_CharacterClass = CLASS_DARKLING;

	m_JumpMinRange = -1;	// No jumping as a default
	m_JumpMaxRange = -1;	// No jumping as a default

	m_bMeatFace = false;
	m_DK_LightResistance = DK_AVERAGE;
	m_DK_Toughness = DK_AVERAGE;
	m_DK_MaxBoredomTicks = -1;
	m_CurBoredomTicks = 0;
	m_Bravery = BRAVERY_EQUAL;
	m_DK_Special = DK_SPECIAL_NONE;
	m_iDKHeadAttack = 0;

	INVALIDATE_POS(m_DarkSpot);
	m_DarkSpotLight = 1.0f;

	m_DKLightDamageThreshold = 0.6f;
	m_AccumulatedLightDamage = 0.0f;
	m_bTookLightDamageLastTick = false;
	
	m_bPlayerFOVTargets = false;

	m_SPLightMeasureCount = 0;
};

//Initializer
void CAI_Core_Darkling::Init(CWObject * _pCharacter, CWorld_Server * _pServer)
{
	MAUTOSTRIP(CAI_Core_Darkling_Init, MAUTOSTRIP_VOID);
	CAI_Core::Init(_pCharacter, _pServer);
};

//Get type of AI
int CAI_Core_Darkling::GetType()
{
	MAUTOSTRIP(CAI_Core_Darkling_GetType, 0);
	return TYPE_DARKLING;
};

void CAI_Core_Darkling::OnPreScriptAction(int _iAction, bool _bTake)
{
	if (m_DK_Special == DK_SPECIAL_GUNNER)
	{
		if (_bTake)
		{	// Take action
			switch(_iAction)
			{
			case CAI_ScriptHandler::ATTACK:
			case CAI_ScriptHandler::AIM:
				{
					m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
				};
				break;
			case CAI_ScriptHandler::MOVE_TO_SCENEPOINT:
			case CAI_ScriptHandler::MOVE_TO:
				{
					m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE);
				};
			default:
				break;
			}
		}
		else
		{	// Clear action
			switch(_iAction)
			{
			case CAI_ScriptHandler::ATTACK:
			case CAI_ScriptHandler::AIM:
				{
					m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE);
				};
				break;
			case CAI_ScriptHandler::MOVE_TO_SCENEPOINT:
			case CAI_ScriptHandler::MOVE_TO:
				{
					m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
				};
			case CAI_ScriptHandler::INVALID_ACTION:
				{	// Clear actions
					m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE);
				};
			default:
				break;
			}
		}
	}
	else
	{
		m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE);
	}
};

//Updates the bot one frame. If the _bPassive flag is true, no actions are considered or generated, 
//but information is gathered.
void CAI_Core_Darkling::OnRefresh(bool _bPassive) 
{
	bool bScriptValid = m_Script.IsValid();
	uint32 iPlayer = GetClosestPlayer();
	CAI_Core* pPlayerAI = GetAI(iPlayer);
	if (m_bFirstRefresh)
	{	// Find what room we are in and tell that to restrict
		// This is to avoid first frame stutter
		INVALIDATE_POS(m_AH.m_RetargetPos);
		INVALIDATE_POS(m_AH.m_RetargetUp);
		m_ActivationState = STATE_ACTIVE;
		m_SPLightMeasureCount = AI_SP_LIGHTMETER_MAXCOUNT;
		m_CurBoredomTicks = 0;
		m_AccumulatedLightDamage = 0.0f;
		TempDisableWallClimb(GetAITicksPerSecond());	// No wallclimb first 1 sec
		CAI_Core::OnRefresh(_bPassive);
		m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE);
		m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE);
		// Check wether we're friend or enemy with player (to help the autoaim system)
		if (!m_bMeatFace)
		{
			CAI_AgentInfo* pPlayer = m_KB.GetAgentInfo(iPlayer);
			if (!pPlayer)
			{	
				pPlayer = m_KB.AddAgent(iPlayer);
			}
			if ((pPlayer)&&(pPlayer->GetRelation(true) < CAI_AgentInfo::ENEMY))
			{
				((CWObject_Character *)m_pGameObject)->m_Flags |= PLAYER_FLAGS_NOAUTOAIMATARGET;
				// Get enemies from player KB if any
				CAI_Core* pPlayerAI = GetAI(iPlayer);
				if (pPlayerAI)
				{
					m_KB.AddAgent(pPlayerAI->m_AH.m_iTarget,CAI_AgentInfo::IN_LOS,CAI_AgentInfo::DETECTED,CAI_AgentInfo::ENEMY);
					m_KB.AddAgent(pPlayerAI->m_AH.m_iTarget2,CAI_AgentInfo::IN_LOS,CAI_AgentInfo::DETECTED,CAI_AgentInfo::ENEMY);
				}
			}
		}
		// Say something appropriate, depending on type
		switch(m_DK_Special)
		{
		case DK_SPECIAL_INFORMER:
			break;

		case DK_SPECIAL_BERSERKER:
			{
				int rnd = TruncToInt(Random * 2.999);
				if (rnd == 0)
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::IDLE_TALK,CAI_Action::PRIO_COMBAT,9); // "I'm feeling bizarre and just a little random."
				}
				else if (rnd == 1)
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::IDLE_TALK,CAI_Action::PRIO_COMBAT,19); // "Lets get it started."
				}
				else
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::COMBAT_AFFIRMATIVE,CAI_Action::PRIO_COMBAT,7); // "Oh, yeah!"
				}
			}
			break;
		case DK_SPECIAL_LIGHTKILLER:
			{
				int rnd = TruncToInt(Random * 2.999);
				if (rnd == 0)
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::IDLE_TALK,CAI_Action::PRIO_COMBAT,6); //  "Got a light?  (cackles)"
				}
				else if (rnd == 1)
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::COMBAT_SPOTTED,CAI_Action::PRIO_COMBAT,0); // "Let me cut someone."
				}
				else
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::IDLE_BOREDOM,CAI_Action::PRIO_COMBAT,4); // "Behold!"
				}
			}
			break;
		case DK_SPECIAL_KAMIKAZE:
			{

				int rnd = TruncToInt(Random * 2.999);
				if (rnd == 0)
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::IDLE_TALK,CAI_Action::PRIO_COMBAT,6); //  "How do you spell, "Carnage?""
				}
				else if (rnd == 1)
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::IDLE_BOREDOM,CAI_Action::PRIO_COMBAT,9); // "I would like to do some mischief."
				}
				else
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::COMBAT_SPOTTED,CAI_Action::PRIO_COMBAT,6); // "I must kill something."
				}
			}
			break;
		case DK_SPECIAL_GUNNER:
			{
				int rnd = TruncToInt(Random * 2.999);
				if (rnd == 0)
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::COMBAT_SPOTTED,CAI_Action::PRIO_COMBAT,9); //  "Ready for action"
				}
				else if (rnd == 1)
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::IDLE_TALK,CAI_Action::PRIO_COMBAT,2); // "Let's kill a Commie!"
				}
				else
				{
					m_DeviceSound.UseRandom(CAI_Device_Sound::IDLE_BOREDOM,CAI_Action::PRIO_COMBAT,7); // "I want to shoot someone."
				}
			}
			break;	
		}


		// Should be dark enough I guess
		m_DarkSpot = GetBasePos();
		m_DarkSpotLight = 1.0f;
		m_bFirstRefresh = false;
	}
	else
	{
		// Check number of enemies etc
		/*
		if (m_KB.GetDetectedEnemyCount())
		{
			TempEnableWallClimb(m_Patience);
		}
		if (m_KB.GetDetectedEnemyCount() <= 0)
		{
			if ((GetUp() * CVec3Dfp32(0.0f,0.0f,1.0f) < 0.99f)||(bScriptValid))
			{	// We're climbing or scripted, keep climbing
				TempEnableWallClimb(m_Patience);
			}
			else if ((!m_ClimbDisableTicks)&&(!m_ClimbEnableTicks))
			{
				ResetPathSearch();
				UseRandom("Stop climbing",CAI_Device_Sound::IDLE_TALK,CAI_Action::PRIO_IDLE);
				// TempDisableWallClimb(m_Patience);
				TempEnableWallClimb(m_Patience);
			}
		}
		*/

		m_SPLightMeasureCount = 0;	// *** No light hurt damage ***
		if ((!m_bMeatFace)&&
			(m_Timer % AI_TICKS_PER_SECOND == 0)&&
			(GetStealthTension() < TENSION_HOSTILE)&&
			(!m_pGameObject->AI_IsJumping())&&
			(!m_pGameObject->AI_IsOnWall()))
		{
			if ((pPlayerAI)&&(pPlayerAI->m_Weapon.GetWieldedArmsClass() >= CAI_WeaponHandler::AI_ARMSCLASS_GUN))
			{
				CVec3Dfp32 OurLookDir = GetLookDir();
				CVec3Dfp32 OurLookPos = GetBasePos();	// Cannot really use GetLookPos();
				OurLookPos[2] += 8.0f;
				CVec3Dfp32 LookDir,EyePos,PosDir;
				if (GetPlayerLookAndPos(LookDir,EyePos))
				{
#ifndef M_RTM
					Debug_RenderWire(EyePos,OurLookPos,kColorRed,1.0f);
#endif	
					PosDir = (OurLookPos - EyePos).Normalize();
					fp32 LookCosAngle = PosDir * LookDir;
					if (LookCosAngle >= 0.966f)		// 15 degrees
					{
						m_DeviceLook.Free();
						OnTrackObj(iPlayer,10,false,false);
						if (LookCosAngle >= 0.867f)		// 30 degrees
						{	// Check our look angle with player
							LookCosAngle = -PosDir * OurLookDir;
							if (LookCosAngle >= 0.867f)		// 30 degrees
							{	// TODO: Find an appropriate speech thingy for this
								if (Random > 0.5f)
								{	// Half the time
									int Rnd = TruncToInt(5.999f * Random);
									switch(Rnd)
									{
										case 0:
										{
											SetWantedBehaviour2(BEHAVIOUR_DARKLING_BACKFLIP,CAI_Action::PRIO_IDLE,0,-1);
											break;
										}
											
										case 1:
										{
											SetWantedBehaviour2(BEHAVIOUR_DARKLING_GROWL,CAI_Action::PRIO_IDLE,0,-1);
											break;
										}
										
										case 2:
										{
											SetWantedBehaviour2(BEHAVIOUR_DARKLING_NONO,CAI_Action::PRIO_IDLE,0,-1);
											break;
										}
											

									default:
										SetWantedBehaviour2(BEHAVIOUR_DARKLING_HANDSUP,CAI_Action::PRIO_IDLE,0,-1);
										break;
									}
								}
								SetStealthTension(TENSION_HOSTILE);
							}
						}
					}
				}
			}
		}
		
		// Disable wallclimb during berserker attacks
		int32 iBehaviour = GetCurrentBehaviour();
		if ((iBehaviour >= BEHAVIOUR_BERZERKER_MIN)&&(iBehaviour <= BEHAVIOUR_BERZERKER_MAX))
		{
			TempDisableWallClimb(1);
		}
		CAI_Core::OnRefresh(_bPassive);
	}
	
	// Ugly bugly handling of Darkling boredom
	if (!m_bMeatFace)
	{
		// Handle damage from light
		fp32 Light = GetLightIntensity(0);
		if (Light > m_DKLightDamageThreshold)
		{
			fp32 DamageAdd = Light - m_DKLightDamageThreshold;
			m_AccumulatedLightDamage += DamageAdd;
			if (m_AccumulatedLightDamage > 10.0f)
			{
				// This was kinda buggy...
				// I know it's not a good solution, but damn it works!
				//if (!m_bTookLightDamageLastTick)
				{	// We have just reached the threshold for taking damage
					CWObject_Message Msg(OBJMSG_CHAR_DARKLING_SMOKE);
					Msg.m_Param0 = 1;	// Turn on the effect
					m_pServer->Message_SendToObject(Msg,m_pGameObject->m_iObject);
				}

				// 0.1 propability of taking 10 hits of lightdamage
				// (to get the hurt spamming down)
				m_bTookLightDamageLastTick = true;
				if ((m_pGameObject)&&(GetStealthTension() < TENSION_HOSTILE))
				{
					CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
					if (pChar)
					{
						if (Random <= 0.2f)
						{
							if (GetStealthTension() < TENSION_HOSTILE)
							{
								SetWantedBehaviour2(BEHAVIOUR_DARKLING_BLINDED,CAI_Action::PRIO_IDLE,0,-1);
							}

							CWO_DamageMsg Msg(5);
							Msg.m_DamageType = DAMAGETYPE_LIGHT;
							pChar->Physics_Damage(Msg,m_pGameObject->m_iObject);
							ConOutL(CStr("Darkling: Light hurts!"));
						}
					}
				}
				m_AccumulatedLightDamage -= 10.0f;
				
				// We go to a dark place to avoid the light
				if ((INVALID_POS(m_AH.m_RetargetPos))&&(VALID_POS(m_DarkSpot))&&(m_KB.GetDetectedEnemyCount() < 1)&&(!bScriptValid))
				{
					m_AH.m_RetargetPos = m_DarkSpot;
					m_AH.m_RetargetUp =  GetUp();
					m_AH.m_RetargetPosPF = m_PathFinder.SafeGetPathPosition(m_AH.m_RetargetPos,2,2);
					if ((m_bCanWallClimb)&&(m_bWallClimb))
					{
						m_AH.m_RetargetPos += m_AH.m_RetargetUp;	// Tweak it up a tinsy little bit
					}
					else
					{
						m_AH.m_RetargetPos += m_AH.m_RetargetUp * DARKLING_PF_OFFSET;
					}
					INVALIDATE_POS(m_DarkSpot);
				}
			}
		}
		else
		{	// We're in a dark spot
			if (!m_pGameObject->AI_IsJumping())
			{
				if ((INVALID_POS(m_DarkSpot))||(!pPlayerAI)||(SqrDistanceToUs(iPlayer) < SqrDistanceToUs(m_DarkSpot)))
				{
					m_DarkSpot = GetBasePos();
					m_DarkSpot = m_PathFinder.SafeGetPathPosition(m_DarkSpot,2,2);
#ifndef M_RTM
					if (DebugTarget())
					{
						Debug_RenderWire(m_DarkSpot,m_DarkSpot+GetUp()*100.0f,kColorBlue,3.0f);
					}
#endif
				}
			}

			// This really shouldn't be done this often!!
			// Other way around seemed to screw things up a bit and deactivation never happened sometimes.
			// Which also is unacceptable... *sigh*
			//if (m_bTookLightDamageLastTick)
			{
				CWObject_Message Msg(OBJMSG_CHAR_DARKLING_SMOKE);
				Msg.m_Param0 = 0;	// Turn off the effect
				m_pServer->Message_SendToObject(Msg,m_pGameObject->m_iObject);
			}
			m_AccumulatedLightDamage = 0.0f;
			m_bTookLightDamageLastTick = false;
		}

		if (bScriptValid)
		{	// Scripted darklings are not bored
			m_CurBoredomTicks = 0;
		}
		else
		{	// Check proximity to the player
			int iPlayer = GetClosestPlayer();
			if (SqrDistanceToUs(iPlayer) <= Sqr(320.0f))
			{
				m_CurBoredomTicks = 0;
			}
		}

		if (m_DK_MaxBoredomTicks >= 0)
		{	// Ah, sumthin' interesting, bye boredom!
			if (m_AH.AcquireTarget())
			{
				m_CurBoredomTicks = 0;
			}
		}
		
		if (m_DK_MaxBoredomTicks >= 0)
		{	// Agh, if I don't get sumthin' to do I swear I die of BOREDOM!
			m_CurBoredomTicks++;
			if ((m_CurBoredomTicks >= m_DK_MaxBoredomTicks)&&(m_MinHealth <= 0))
			{
				if (m_CurBoredomTicks == m_DK_MaxBoredomTicks)
				{	// Run boredom behaviour
					UseRandom(CStr("Darkling bored"),CAI_Device_Sound::IDLE_BOREDOM,CAI_Action::PRIO_IDLE);
					SetWantedBehaviour2(BEHAVIOUR_DARKLING_BORED,CAI_Action::PRIO_IDLE,0,-1);
					ConOutL(CStr("Darkling: Booring!"));
				}
				else if (m_CurBoredomTicks >= m_DK_MaxBoredomTicks + 5.0f * GetAITicksPerSecond())
				{	// Die!!!
					if (m_pGameObject)
					{
						CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pGameObject);
						if (pChar)
						{
							int Dam = (int)(MaxHealth() * 2.0f);	// Die muthafucka
							CWO_DamageMsg Msg(Dam);
							pChar->Physics_Damage(Msg,m_pGameObject->m_iObject);
							if (Health() <= 0)
							{
								ConOutL(CStr("Darkling: Deadly Booring!"));
							}
						}
					}
				}
			}
		}
	}
};

void CAI_Core_Darkling::OnTakeAction()
{
	CAI_Core::OnTakeAction();
};
// ***

//Handles incoming messages
aint CAI_Core_Darkling::OnMessage(const CWObject_Message& _Msg)
{
	if ((!m_bMeatFace)&&(_Msg.m_Msg == OBJMSG_AIEFFECT_RETARGET))
	{
		// Ignore if not spawned or script is running
		if ((!m_pGameObject->AI_IsSpawned())||(m_Script.IsValid()))
		{
			return 1;
		}

		int iRetargeter = _Msg.m_iSender;
		CAI_AgentInfo* pRetargeter = m_KB.GetAgentInfo(_Msg.m_iSender);
		if ((!pRetargeter)||(pRetargeter->GetCurRelation() > CAI_AgentInfo::FRIENDLY))
		{	// We ignore retarget attempts from nonfriends
			if (!pRetargeter)
			{	// Add me
				m_KB.AddAgent(iRetargeter);
			}
			return 1;
		}
		if (INVALID_POS(_Msg.m_VecParam1))
		{	// No can do signaled
			// No jump, no walk
			INVALIDATE_POS(m_AH.m_RetargetPos);
			INVALIDATE_POS(m_AH.m_RetargetUp);
			INVALIDATE_POS(m_AH.m_RetargetPosPF);
			SetWantedBehaviour2(CAI_Core::BEHAVIOUR_DARKLING_FUCKOFF,CAI_Action::PRIO_ALERT,0,-1);
			UseRandom(CStr("No retarget from char"),CAI_Device_Sound::IDLE_NOWAY,CAI_Action::PRIO_ALERT);
			return 1;
		}
		if (INVALID_POS(_Msg.m_VecParam0))
		{	// Recall sent
			int iPlayer = GetClosestPlayer();
			CAI_Core* pPlayerAI = GetAI(iPlayer);
			if (pPlayerAI)
			{
				m_AH.m_RetargetPos = pPlayerAI->GetBasePos();
				m_AH.m_RetargetUp =  pPlayerAI->GetUp();
				m_AH.m_RetargetPos += m_AH.m_RetargetUp;	// Tweak it up a tinsy little bit
			}
			else
			{
				return 1;
			}
		}
		else
		{
			m_AH.m_RetargetPos = _Msg.m_VecParam0;
			m_AH.m_RetargetUp =  _Msg.m_VecParam1;
			if ((m_bCanWallClimb)&&(m_bWallClimb))
			{
				m_AH.m_RetargetPos += m_AH.m_RetargetUp;	// Tweak it up a tinsy little bit
			}
			else
			{
				m_AH.m_RetargetPos += m_AH.m_RetargetUp * DARKLING_PF_OFFSET;
			}
		}

		if (m_AH.m_spRestriction == NULL)
		{
			m_AH.m_spRestriction = MNew1(CAI_Action_Restrict, &m_AH);
		}
		// Ignore if restricted
		if (m_AH.m_spRestriction->IsRestricted(m_AH.m_RetargetPos))
		{
			INVALIDATE_POS(m_AH.m_RetargetPos);
			INVALIDATE_POS(m_AH.m_RetargetUp);
			UseRandom(CStr("Retarget restricted"),CAI_Device_Sound::IDLE_NOWAY,CAI_Action::PRIO_ALERT);
			SetWantedBehaviour2(BEHAVIOUR_DARKLING_FUCKOFF,CAI_Action::PRIO_ALERT,0,-1);
			return 1;
		}

		// AI should consider a new target 
		// Stop any current Idle behaviours
		if (m_bBehaviourRunning)
		{
			StopBehaviour(BEHAVIOUR_STOP_FAST,CAI_Action::PRIO_DANGER);
		}
		m_AH.ExpireActions(CAI_Action::INVALID);
		if (SqrDistanceToUs(_Msg.m_VecParam0) < Sqr(Max3(m_RoamMaxRange,m_CloseMaxRange,m_CombatMaxRange)))
		{
			m_AH.m_RetargetPos = _Msg.m_VecParam0;
			m_AH.m_RetargetUp =  _Msg.m_VecParam1;
			m_AH.m_RetargetPos += m_AH.m_RetargetUp * DARKLING_PF_OFFSET;	// Tweak it up a tinsy little bit
		}
		
		CMat4Dfp32 Mat;
		GetBaseMat(Mat);
		CVec3Dfp32 Dir = m_AH.m_RetargetPos - GetBasePos();
		Dir.Normalize();
		Dir.SetRow(Mat,0);	 				// Fwd
		m_AH.m_RetargetPos.SetRow(Mat,3);	// Pos
		m_AH.m_RetargetUp.SetRow(Mat,2);	// Up
		Mat.RecreateMatrix(2,1);
		bool bCoplanar = false;
		if (GetUp() * m_AH.m_RetargetUp >= 0.99f)
		{
			if ((m_bCanWallClimb)&&(m_bWallClimb))
			{
				if (Abs(m_AH.m_RetargetUp * (m_AH.m_RetargetPos + (Mat.GetRow(2) * DARKLING_PF_OFFSET) - Mat.GetRow(3))) <= 8.0f)
				{
					bCoplanar = true;
				}
			}
			else
			{
				if (Abs(m_AH.m_RetargetUp * (m_AH.m_RetargetPos - Mat.GetRow(3))) <= 8.0f)
				{
					bCoplanar = true;
				}
			}
		}

		CVec3Dfp32 TargetPos = Mat.GetRow(3);
		CAI_AgentInfo* pTarget = m_AH.AcquireTarget();
		if (pTarget)
		{
			TargetPos = pTarget->GetSuspectedPosition();
		}

		CVec3Dfp32 LookPos,LookDir;
		if (GetPlayerLookAndPos(LookDir,LookPos))
		{
			/* Switched off retarget onto scenepoints
			int32 Types = CWO_ScenePoint::ROAM | CWO_ScenePoint::DARKLING | CWO_ScenePoint::WALK_CLIMB;
			if (m_JumpFlags & JUMPREASON_RETARGET)
			{
				Types |= CWO_ScenePoint::JUMP_CLIMB;
			}
			m_AH.m_pRetargetSP = m_AH.GetBestScenepointAlongRay(Types,15.0f,LookPos,m_AH.m_RetargetPos,16.0f,m_JumpMaxRange,TargetPos);
			if (m_AH.m_pRetargetSP)
			{
#ifndef M_RTM
				m_AH.DebugDrawScenePoint(m_AH.m_pRetargetSP,false,0,1.0f);
#endif	
				m_AH.m_RetargetPos = m_AH.m_pRetargetSP->GetPosition();
				m_AH.m_RetargetUp = m_AH.m_pRetargetSP->GetUp();
			}
			*/
		}

		m_AH.m_RetargetPosPF = m_PathFinder.GetPathPosition(m_AH.m_RetargetPos,2,2);
		if (INVALID_POS(m_AH.m_RetargetPosPF))
		{	// No jump, no walk
			INVALIDATE_POS(m_AH.m_RetargetPos);
			INVALIDATE_POS(m_AH.m_RetargetUp);
			INVALIDATE_POS(m_AH.m_RetargetPosPF);
			SetWantedBehaviour2(CAI_Core::BEHAVIOUR_DARKLING_FUCKOFF,CAI_Action::PRIO_ALERT,0,-1);
			UseRandom(CStr("Retarget failure"),CAI_Device_Sound::IDLE_NOWAY,CAI_Action::PRIO_ALERT);
			return 1;
		}
		m_AH.m_RetargetPos = m_AH.m_RetargetPosPF;
		if ((!bCoplanar)&&(_Msg.m_Param0))
		{
			Mat.GetRow(3) = _Msg.m_VecParam0;
			AddJumpTowardsPositionCmd(&Mat,CAI_Core::JUMPREASON_RETARGET,CAI_Core::JUMP_CHECK_COLL);
		}
		m_AH.m_RetargetTimeout = 0;
		UseRandom(CStr("Darkling retargeted"),CAI_Device_Sound::IDLE_AFFIRMATIVE,CAI_Action::PRIO_ALERT);
		// Not bored, no lightdamage
		m_CurBoredomTicks = 0;
		m_AccumulatedLightDamage = 0.0f;
		m_bTookLightDamageLastTick = false;
		return 1;
	}
	else if (_Msg.m_Msg == OBJMSG_CHAR_AI_DARKLINGLAND)
	{
		if (m_iDKHeadAttack)
		{	// Play spiffy behaviour if target is appropriately nearby
			if (!_Msg.m_pData || _Msg.m_DataSize != sizeof(CMat4Dfp32) || !m_pGameObject)
				return 0;
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
			CWAG2I_Context AGContext(m_pGameObject,m_pServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pServer->GetGameTickTime()));
			CMat4Dfp32 JumpDest = *(CMat4Dfp32*)_Msg.m_pData;
			// From CharDarkling_Phys code
			// Set destination speed as well (current speed)
			pCD->m_AnimGraph2.SetDestination(JumpDest,m_pGameObject->GetVelocity().m_Move.Length());
			pCD->m_AnimGraph2.SendJumpImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_JUMP, AG2_IMPULSEVALUE_JUMP_LANDDEST));
			SetWantedBehaviour2(m_iDKHeadAttack,CAI_Action::PRIO_COMBAT,BEHAVIOUR_FLAGS_JUMPOK | BEHAVIOUR_FLAGS_PP,0,NULL,&JumpDest);
			return 1;
		}
		else
		{	// Let character handle it
			return 0;
		}
	}
	else
	{
		return CAI_Core::OnMessage(_Msg);
	}
}

//Handles any registry keys of a character that are AI-related
bool CAI_Core_Darkling::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CAI_Core_Darkling_OnEvalKey, false);
	
	if (!_pKey)
		return false;	

	const CStr KeyName = _pKey->GetThisName();
	CStr Value = _pKey->GetThisValue().UpperCase();
	uint32 ValueHash = StringToHash(Value);
	const fp32 Valuef = _pKey->GetThisValuef();
	const int Valuei = _pKey->GetThisValuei();
	
	bool bHandled = true;
	switch (_KeyHash)
	{	
	case MHASH4('AI_D','K_ME','ATFA','CE'): // "AI_DK_MEATFACE"
		{
			m_bMeatFace = (Valuei != 0) ? true : false;
			break;
		}	
	case MHASH6('AI_D','K_LI','GHTR','ESIS','TANC','E'): // "AI_DK_LIGHTRESISTANCE"
		{
			if (ValueHash == MHASH2('VERY','_LOW'))
			{
				m_DK_LightResistance = DK_VERY_LOW;
				m_DKLightDamageThreshold = 0.2f;
			}
			else if (ValueHash == MHASH1('LOW'))
			{
				m_DK_LightResistance = DK_LOW;
				m_DKLightDamageThreshold = 0.4f;
			}
			else if (ValueHash == MHASH2('AVER','AGE'))
			{
				m_DK_LightResistance = DK_AVERAGE;
				m_DKLightDamageThreshold = 0.6f;
			}
			else if (ValueHash == MHASH1('HIGH'))
			{
				m_DK_LightResistance = DK_HIGH;
				m_DKLightDamageThreshold = 0.8f;
			}
			else if (ValueHash == MHASH3('VERY','_HIG','H'))
			{
				m_DK_LightResistance = DK_VERY_HIGH;
				m_DKLightDamageThreshold = 1.0f;
			}
			break;
		}
	case MHASH4('AI_D','K_TO','UGHN','ESS'): // "AI_DK_TOUGHNESS"
		{
			if (ValueHash == MHASH2('VERY','_LOW'))
			{
				m_DK_Toughness = DK_VERY_LOW;
			}
			else if (ValueHash == MHASH1('LOW'))
			{
				m_DK_Toughness = DK_LOW;
			}
			else if (ValueHash == MHASH2('AVER','AGE'))
			{
				m_DK_Toughness = DK_AVERAGE;
			}
			else if (ValueHash == MHASH1('HIGH'))
			{
				m_DK_Toughness = DK_HIGH;
			}
			else if (ValueHash == MHASH3('VERY','_HIG','H'))
			{
				m_DK_Toughness = DK_VERY_HIGH;
			}
			break;
		}
	case MHASH4('AI_D','K_BR','AVER','Y'): // "AI_DK_BRAVERY"
		{
			if (ValueHash == MHASH2('VERY','_LOW'))
			{
				m_Bravery = BRAVERY_NEVER;
			}
			else if (ValueHash == MHASH1('LOW'))
			{
				m_Bravery = BRAVERY_BETTER;
			}
			else if (ValueHash == MHASH2('AVER','AGE'))
			{
				m_Bravery = BRAVERY_EQUAL;
			}
			else if (ValueHash == MHASH1('HIGH'))
			{
				m_Bravery = BRAVERY_ONE_LESS;
			}
			else if (ValueHash == MHASH3('VERY','_HIG','H'))
			{
				m_Bravery = BRAVERY_ALWAYS;
			}
			break;
		}
	case MHASH4('AI_D','K_BO','REDO','M'): // "AI_DK_BOREDOM"
		{
			if (Valuef >= 0.0f)
			{
				m_DK_MaxBoredomTicks = (int)(Valuef * GetAITicksPerSecond());
			}
			else
			{
				m_DK_MaxBoredomTicks = -1;
			}
			break;
		}
	case MHASH4('AI_D','K_SP','ECIA','L'): // "AI_DK_SPECIAL"
		{
			if (ValueHash == MHASH1('NONE'))
			{
				m_DK_Special = DK_SPECIAL_NONE;
			}
			else if (ValueHash == MHASH2('INFO','RMER'))
			{
				m_DK_Special = DK_SPECIAL_INFORMER;
			}
			else if (ValueHash == MHASH3('BERS','ERKE','R'))
			{
				m_DK_Special = DK_SPECIAL_BERSERKER;
			}
			else if (ValueHash == MHASH3('LIGH','TKIL','LER'))
			{
				m_DK_Special = DK_SPECIAL_LIGHTKILLER;
			}
			else if (ValueHash == MHASH2('KAMI','KAZE'))
			{
				m_DK_Special = DK_SPECIAL_KAMIKAZE;
			}
			else if (ValueHash == MHASH2('GUNN','ER'))
			{
				m_DK_Special = DK_SPECIAL_GUNNER;
			}
			break;
		}
	case MHASH4('AI_D','K_JU','MPFL','AGS'): // "AI_DK_JUMPFLAGS"
		{
			/*
			JUMPREASON_IDLE			= 1,	// Bot may jump from IDLE actions
			JUMPREASON_COMBAT		= 2,	// Bot can jump from COMBAT actions
			JUMPREASON_AVOID		= 4,	// Bot can jump when avoiding player
			JUMPREASON_RETARGET		= 8,	// Bot can jump when retargeted
			JUMPREASON_STUCK		= 16,	// Bot can jump when stuck
			JUMPREASON_NOGRID		= 32,	// Bot can jump when stuck with no grid
			JUMPREASON_SCRIPT		= 64,	// Bot can jump when scripted to do so
			*/
			static const char* TranslateJumpFlags[] =
			{
				"IDLE", "COMBAT", "AVOID", "RETARGET", "STUCK", "NOGRID", "SCRIPT", NULL
			};
			m_JumpFlags = Value.TranslateFlags(TranslateJumpFlags);
			if (m_JumpFlags)
			{
				m_bCanJump = true;
			}
			else
			{
				m_bCanJump = false;
			}
			break;
		}
	case MHASH4('AI_D','K_JU','MPMI','N'): // "AI_DK_JUMPMIN"
		{
			if (Valuef >= 0.0f)
			{
				m_JumpMinRange = Valuef;
			}
			else
			{
				m_JumpMinRange = -1.0f;
			}
			break;
		}
	case MHASH4('AI_D','K_JU','MPMA','X'): // "AI_DK_JUMPMAX"
		{
			if (Valuef >= 0.0f)
			{
				m_JumpMaxRange = Valuef;
			}
			else
			{
				m_JumpMaxRange = -1.0f;
			}
			break;
		}
	case MHASH6('AI_D','K_PL','AYER','FOVT','ARGE','TS'): // "AI_DK_PLAYERFOVTARGETS"
		{
			if (Valuei != 0)
			{
				m_bPlayerFOVTargets = true;
			}
			else
			{
				m_bPlayerFOVTargets = false;
			}
			break;
		}
	case MHASH4('AI_D','K_HE','ADAT','TACK'): // "AI_DK_HEADATTACK"
		{
			if (Valuei > 0)
			{
				m_iDKHeadAttack = Valuei;
			}
			else
			{
				m_iDKHeadAttack = 0;
			}
			break;
		}
	default:
		{
			bHandled = false;
			break;
		}
	}

	if (bHandled)
	{
		return true;
	}
	else
	{
		return CAI_Core::OnEvalKey(_KeyHash, _pKey);
	}
};

int CAI_Core_Darkling::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int32 nbrAdded = 0;

	if (!m_bMeatFace)
	{
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BORED);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_PISS);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BLINDED);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_BACKFLIP);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_HANDSUP);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_GROWL);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_FUCKOFF);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_THUMB_DOWN);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_SUCCESS);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_THUMB_UP);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_NONO);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_JACKIESHOOT);
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,BEHAVIOUR_DARKLING_JACKIESHOOT_SHORT);
	}
	
	if (m_iDKHeadAttack)
	{
		nbrAdded += CAI_Action::AddUnique(_liBehaviours,m_iDKHeadAttack);
	}

	return(nbrAdded);
};

int CAI_Core_Darkling::GetUsedGestures(TArray<int16>& _liGestures) const
{
	int32 nbrAdded = 0;

	nbrAdded += CAI_Action::AddUnique(_liGestures,GESTURE_DARKLING_BUMP);

	return(nbrAdded);
};

void CAI_Core_Darkling::OnBumped(bool _bWeAreBumping,int _iObj,CVec3Dfp32& CollPos)
{
	if (!_bWeAreBumping)
	{
		CAI_Core::OnBumpedAG2(_iObj);
	}
	else
	{	// Inspect
		CWObject* pObj = m_pServer->Object_Get(_iObj);
		if ((pObj)&&(pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_PHYSMOVEMENT))
		{
			CVec3Dfp32 ForcePos = pObj->GetPosition();
			CVec3Dfp32 ForceDir = pObj->GetPosition() - GetBasePos();
			if (VALID_POS(CollPos))
			{
				ForcePos = CollPos;
			}
			ForceDir += GetUp();
			ForceDir.Normalize();
			fp32 Force = 5.0f;
			fp32 mass = pObj->GetMass();
			if (mass < 1000.0f)
			{
				m_pServer->Phys_AddMassInvariantImpulse(_iObj, ForcePos, ForceDir * Force);
				pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				// How do we handle gestures?
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pGameObject);
				CWAG2I_Context Context(m_pGameObject,m_pServer,pCD->m_GameTime);
				pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_SHORTGESTURE,GESTURE_DARKLING_BUMP),0);
			}
		}
	}
};

bool CAI_Core_Darkling::IsValidTarget(int _iTarget)
{
	if ((_iTarget)&&(m_AH.m_iEnemyOverride == _iTarget))
	{
		return(true);
	}

	CAI_Core* pTargetAI = GetAI(_iTarget);
	if ((pTargetAI)&&(m_pGameObject)&&(m_AH.m_iTarget != _iTarget))
	{
		if ((m_bPlayerFOVTargets)&&(!pTargetAI->IsPlayerLookWithinAngle(30)))
		{
			return(false);
		}

		/* Ignore light here
		fp32 Light = pTargetAI->GetLightIntensity(m_pGameObject->m_iObject);
		if (Light > m_DKLightDamageThreshold)
		{
			return(false);
		}
		*/
	}

	
	return(CAI_Core::IsValidTarget(_iTarget));
};

// Verifies _pSP for light etc 
bool CAI_Core_Darkling::CheckScenepoint(CWO_ScenePoint* _pSP)
{
	fp32 Light = 1.0f;

	if (_pSP)
	{
		// We check if scenepoint is above the horizon
		if (m_pGameObject->AI_IsOnWall())
		{
			CMat4Dfp32 OurMat;
			GetBaseMat(OurMat);
			CVec3Dfp32 Up = GetUp();
			CVec3Dfp32 Pos = GetBasePos();
			CMat4Dfp32 SpMat = _pSP->GetPositionMatrix();
			CVec3Dfp32 SPUp = _pSP->GetUp();
			CVec3Dfp32 SPPos = _pSP->GetPosition();
			if (Up * (SPPos - Pos) <= 0.0f)
			{	// Below horizon
				return(false);
			}
		}
		
		if (!m_bMeatFace)
		{	// We check if it's dark enough fpr our sensitive skin
			int Tick = GetAITick();
			if ((m_SPLightMeasureCount < AI_SP_LIGHTMETER_MAXCOUNT)&&(Tick > _pSP->GetLightTick()))
			{	// (re)measure light
				Light = MeasureLightIntensityAt(_pSP->GetPosition(),true,0);
				m_SPLightMeasureCount++;
				_pSP->SetLight(Light,Tick);
			}
			else
			{
				Light = _pSP->GetLight();
			}

			if (Light > m_DKLightDamageThreshold)
			{
				return(false);
			}
		}
	}

	return(CAI_Core::CheckScenepoint(_pSP));
};

void CAI_Core_Darkling::OnDeltaLoad(CCFile* _pFile)
{
	MACRO_WRITEVERIFY;

	CAI_Core::OnDeltaLoad(_pFile);
};
void CAI_Core_Darkling::OnDeltaSave(CCFile* _pFile)
{
	MACRO_WRITEVERIFY;

	CAI_Core::OnDeltaSave(_pFile);
};


