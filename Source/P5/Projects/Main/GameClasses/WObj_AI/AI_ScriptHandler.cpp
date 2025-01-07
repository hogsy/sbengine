#include "PCH.h"
#include "../WObj_Char.h"
#include "../WObj_CharMsg.h"
#include "AICore.h"
#include "../WRPG/WRPGItem.h"
#include "../WObj_Misc/WObj_ScenePoint.h"
#include "../WObj_Misc/WObj_Turret.h"

//Script controller

const int MAX_NO_WAITING_ACTIONS = 64;



//Chooses the actions the bot should take to perform the current animation if there are available devices
void CAI_ScriptHandler::OnPerformAnimation()
{
	MAUTOSTRIP(CAI_ScriptHandler_OnPerformAnimation, MAUTOSTRIP_VOID);
	//Make sure AI is valid and anim device free
	if (!m_pAI || !m_pAI->IsValid() || (!m_pAI->m_DeviceAnimation.IsAvailable()))
		return;

	//Lock anim device
	m_pAI->m_DeviceAnimation.Lock();

	//Should we start animation?
	if (!m_bAnimStarted &&
		(m_iAnim != -1))
	{
		//Nope, start animation!
		m_bAnimStarted = true;

		//Set animation sequence
		if (m_bTorsoAnim)
		{
			//Torso anim
			m_AnimTimer = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, m_iAnim, 1), m_pAI->m_pGameObject->m_iObject);
		}
		else
		{
			//Full anim
			m_AnimTimer = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, m_iAnim), m_pAI->m_pGameObject->m_iObject);
		};
	}
	//Is the animation done?
	else if (m_AnimTimer < 0)
	{
		//Animation is done. Stop animations and clear animation script stuff.
		if (m_bTorsoAnim)
		{
			//Stop torso anim
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, 0, 1), m_pAI->m_pGameObject->m_iObject);
		}
		else
		{
			//Stop full anim
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, 0), m_pAI->m_pGameObject->m_iObject);
		};

		m_iAnim = -1;
		m_bTorsoAnim = false;
		m_bAnimStarted = false;
		m_pAI->m_DeviceAnimation.Free();
	}
	else
	{
		//Count down animation timer
		m_AnimTimer--;
	}
};


//Maps the actions in the action enum to the corresponding AI devices.
int CAI_ScriptHandler::ms_iDeviceMap[MAX_ACTION] = 
{
	CAI_Device::LOOK,
	CAI_Device::MOVE,
	CAI_Device::JUMP,
	CAI_Device::WEAPON,
	CAI_Device::ITEM,
	CAI_Device::STANCE,
	CAI_Device::DARKNESS,
};


//Act aggressively
void CAI_ScriptHandler::OnAggressive()
{
	MAUTOSTRIP(CAI_ScriptHandler_OnAggressive, MAUTOSTRIP_VOID);
	CWObject * pTarget;
	CAI_AgentInfo * pInfo;
	static int LastEnemyTick = 0;

	m_pAI->m_DeviceStance.SetTargetInFOV(true);
	if (m_pAI->m_DeviceStance.GetMaxStance() < CAI_Device_Stance::IDLESTANCE_COMBAT)
	{

	}
	m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
	m_pAI->Reload(1.0f);
	//Is weapon unavailble for attack?
	if (m_pAI->Wait())
	{
		//No need to find new target.
		//Just look at last target we attacked, if valid (this is secondary if such exist, or primary otherwise)
		if ((pTarget = m_pAI->IsEnemy(m_iSecondaryTargetParam))||
			(pTarget = m_pAI->IsEnemy(m_iTargetParam)))
		{
			if ((pInfo = m_pAI->m_KB.GetAgentInfo(pTarget->m_iObject)) != NULL)
			{
				if (m_pAI->CanAttack(pInfo))
				{
					m_pAI->OnAim(pInfo);
				}
				if (pInfo->GetCurAwareness() >= CAI_AgentInfo::DETECTED)
				{
					m_pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
					
				}
				m_pAI->OnTrackAgent(pInfo,5,false,false);
				LastEnemyTick = m_pAI->GetAITick();
			}
		}
	}
	else
	{
		//Get best target and attack it!
		
		//Do we have a primary target which is good?
		if (m_iTargetParam && 
			(pTarget = m_pAI->IsEnemy(m_iTargetParam)) &&
			(pInfo = m_pAI->m_KB.GetAgentInfo(m_iTargetParam))&&
			m_pAI->m_Weapon.GetWielded()->GetEstimate(pInfo))
		{
			//Attack primary target and reset secondary target if valid
			if (m_pAI->CanAttack(pInfo))
			{
				m_pAI->OnAttack(pInfo);
				m_iSecondaryTargetParam = 0;
			}
			if (pInfo->GetCurAwareness() >= CAI_AgentInfo::DETECTED)
			{
				m_pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
			}
			m_pAI->OnTrackAgent(pInfo,5,false,false);
			LastEnemyTick = m_pAI->GetAITick();
		}
		//Do we have a secondary target which is good?
		else if (m_iSecondaryTargetParam && 
				 (pTarget = m_pAI->IsEnemy(m_iSecondaryTargetParam))&&
				 (pInfo = m_pAI->m_KB.GetAgentInfo(m_iSecondaryTargetParam)) &&
				 m_pAI->m_Weapon.GetWielded()->GetEstimate(pInfo)) 
		{
			//Attack secondary target
			if (m_pAI->CanAttack(pInfo))
			{
				m_pAI->OnAttack(pInfo);
				m_iSecondaryTargetParam = 0;
			}
			if (pInfo->GetCurAwareness() >= CAI_AgentInfo::DETECTED)
			{
				m_pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
				m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
			}
			m_pAI->OnTrackAgent(pInfo,5,false,false);
			LastEnemyTick = m_pAI->GetAITick();
		}
		//Try to find other target
		else
		{
			//Find first target which is spotted and good
			m_iSecondaryTargetParam = 0;
			CAI_AgentInfo * pInfo;
			for (int i = 0; i < m_pAI->m_KB.NumAgentInfo(); i++)
			{
				pInfo = m_pAI->m_KB.IterateAgentInfo(i);
				if (pInfo &&
					(pInfo->GetObjectID() != m_iTargetParam) &&
					(pInfo->GetObjectID() != m_iSecondaryTargetParam) &&
					(pInfo->GetRelation() >= CAI_AgentInfo::ENEMY) &&
					(pInfo->GetAwareness() >= CAI_AgentInfo::SPOTTED) &&
					m_pAI->m_Weapon.GetWielded()->GetEstimate(pInfo))
				{
					//Target found!
					//Attack secondary target
					if (m_pAI->CanAttack(pInfo))
					{
						m_pAI->OnAttack(pInfo);
						m_iSecondaryTargetParam = pInfo->GetObjectID();
					}
					if (pInfo->GetCurAwareness() >= CAI_AgentInfo::DETECTED)
					{
						m_pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
						m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
					}
					m_pAI->OnTrackAgent(pInfo,5,false,false);
					LastEnemyTick = m_pAI->GetAITick();
					break;
				};
			};
			
		};
	}

	if (m_pAI->GetAITick() < LastEnemyTick + 30)
	{	// Keep looking at the last enemy that was alive for a second and a half
		m_pAI->m_DeviceLook.Use();
	}
};

void CAI_ScriptHandler::OnUseDarkness()
{
	if (m_DarknessUsage & CAI_Device_Darkness::USAGE_ACTIVATE)
	{
		// Always force drain maximum darkness; this enables us to use darkness in lighted areas.
		if (m_DarknessUsage & CAI_Device_Darkness::USAGE_DRAIN)
			m_pAI->m_DeviceDarkness.ForceDrainDarkness(true);

		// Use look and move devices to do horrible things to the given target
		// (or just cause havoc if there is no target)
		if (m_DarknessPower == CAI_Device_Darkness::POWER_DEMON_ARM)
		{
			OnUseDemonArm(m_DarknessTarget, m_DarknessSecondaryTarget);
		}
		else
		{
			//Power not supported, just activate
			m_pAI->m_DeviceMove.Use();
			m_pAI->m_DeviceLook.Use();
			UseDarkness(m_DarknessPower);
		}
	}
	else 
	{
		bool bContinuousUse = false;

		// Always force drain darkness; this enables us to use darkness in lighted areas. Drain a normal amount when not activating.
		if (m_DarknessUsage & CAI_Device_Darkness::USAGE_DRAIN)
		{
			m_pAI->m_DeviceDarkness.ForceDrainDarkness(true);
			m_pAI->m_DeviceDarkness.Use(CAI_Device_Darkness::USAGE_DRAIN);
			bContinuousUse = true;
		}

		if (m_DarknessUsage & CAI_Device_Darkness::USAGE_SELECT)
		{
			// Try selecting for a while (once, i.e. next if power wasn't specified)
			int StopSelectTime = ((m_DarknessPower == CAI_Device_Darkness::POWER_INVALID) ? 1 : 20);
			if ((m_DarknessUseTimer < StopSelectTime) && 
				(m_DarknessPower != m_pAI->m_DeviceDarkness.GetSelectedPower()))
			{
				m_pAI->m_DeviceDarkness.Use(CAI_Device_Darkness::USAGE_SELECT);
				bContinuousUse = true;
			}
		}

		if (!bContinuousUse)
		{
			ClearUseDarkness(); // Note that we don't call the StopUsingDarkness since we weren't using a power.
		}
	}

	m_DarknessUseTimer++;
}


void CAI_ScriptHandler::OnUseDemonArm(int _GrabTarget, int _WhackTarget)
{
	// In progress... currently we never throw grabbed objects)

	CWObject * pGrabTarget = m_pAI->m_pServer->Object_Get(_GrabTarget);
	CWObject * pWhackTarget = m_pAI->m_pServer->Object_Get(_WhackTarget);

	//if (pGrabTarget) m_pAI->Debug_RenderWire(m_pAI->GetHeadPos(), pGrabTarget->GetPosition());//REMOVE

	// Is the schnabel active?
	if (m_pAI->m_DeviceDarkness.GetActivePower() == CAI_Device_Darkness::POWER_DEMON_ARM)
	{
		// Have we grabbed anything?
		int Grabbed = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETGRABBEDOBJECT), m_pAI->m_pGameObject->m_iObject);
		if (Grabbed)
		{
			// If target is close, let go straight away
			fp32 TargetDistSqr = (pWhackTarget ? m_pAI->GetBasePos().DistanceSqr(pWhackTarget->GetPosition()) : _FP32_MAX);
			if (TargetDistSqr < 128*128)
			{
				StopUsingDarkness();
			}
			// Are we throwing?
			else if (m_DarknessState == DARKNESSSTATE_DEMONARM_THROWING)
			{
				CVec3Dfp32 TentaclePos = GetDarknessPosition(); 
				if (VALID_POS(TentaclePos) && pWhackTarget)
				{
					// Retract arm, then push away and let go while tracking position above target
					CVec3Dfp32 TargetPos = pWhackTarget->GetPosition() + CVec3Dfp32(0,0,64);
					m_pAI->OnTrackPos(TargetPos, 20, false, m_pAI->m_bLookSoft);

					fp32 TentacleDistSqr = m_pAI->GetBasePos().DistanceSqr(TentaclePos);

					// Retract until held close or a short while
					if ((m_DarknessUseTimer < 40) && 
					    (TentacleDistSqr > 64*64))
					{
						UseDarkness(CAI_Device_Darkness::POWER_DEMON_ARM, CVec3Dfp32(-0.5f,0,0));
					}
					// Push towards target until too close 
					else if (TentacleDistSqr / TargetDistSqr < 0.3f)
					{
						if (m_DarknessUseTimer < 40) 
							m_DarknessUseTimer = 40; 
						UseDarkness(CAI_Device_Darkness::POWER_DEMON_ARM, CVec3Dfp32(1.0f,0,0));
					}
					// Throw!
					else
					{
						StopUsingDarkness();
					}
				}
				else
				{
					// Not enough info, just let go
					StopUsingDarkness();
				}
			}
			else
			{
				// Grabbed object but not throwing
				SetDarknessState(DARKNESSSTATE_DEMONARM_GRABBED);
				//Might want to check if we've grabbed the right target, but wtf... //if (Grabbed == _GrabTarget)

				// Swing the grabbed object towards whack target
				CVec3Dfp32 TentaclePos = GetDarknessPosition(); 
				if (VALID_POS(TentaclePos) && pWhackTarget)
				{
					// Swing grabbed object past target.
					CVec3Dfp32 TargetPos = pWhackTarget->GetPosition();
					// Change direction every once in a while
					if (m_DarknessUseTimer % 30 == 0)
					{
						// Swing towards the other side of the target
						CVec3Dfp32 SideDir = m_pAI->GetSideDir();
						fp32 TentacleToTargetDistSqr = TargetPos.DistanceSqr(TentaclePos);
						fp32 TentaclePlusSideToTargetDistSqr = TargetPos.DistanceSqr(TentaclePos + SideDir);
						fp32 SwingDirection = (TentaclePlusSideToTargetDistSqr < TentacleToTargetDistSqr) ? 1.0f : -1.0f;
						CVec3Dfp32 WhackPos = TargetPos + SideDir * SwingDirection * 200.0f;
						m_pAI->OnTrackPos(WhackPos, 10, false, m_pAI->m_bLookSoft);
					}
					else if (m_DarknessUseTimer % 30 < 6)
					{
						// Continue current swing
						CVec3Dfp32 CurrentSwingDirection = (m_pAI->GetForwardDir() - m_pAI->GetLastForwardDir()).Normalize();
						CVec3Dfp32 WhackPos = TargetPos + CurrentSwingDirection * 200.0f + CVec3Dfp32(0,0,-100.0f);
						m_pAI->OnTrackPos(WhackPos, 5, false, m_pAI->m_bLookSoft);
					}
					else if (m_DarknessUseTimer % 30 < 20)
					{
						// Continue current swing and lift
						CVec3Dfp32 CurrentSwingDirection = (m_pAI->GetForwardDir() - m_pAI->GetLastForwardDir()).Normalize();
						CVec3Dfp32 WhackPos = TargetPos + CurrentSwingDirection * 200.0f + CVec3Dfp32(0,0,200.0f);
						m_pAI->OnTrackPos(WhackPos, 10, false, m_pAI->m_bLookSoft);
					}
					else
					{
						// Hold still for a while
						m_pAI->m_DeviceLook.Use();
					}

					// Lengthen or shorten tentacle as needed. Try to keep it slightly in front of target so that 
					// it will look good and you can dodge back to avoid getting clobbered.
					fp32 TentacleDistSqr = m_pAI->GetBasePos().DistanceSqr(TentaclePos);
					fp32 fwd;
					if (TargetDistSqr > TentacleDistSqr)
						fwd = (TargetDistSqr < m_pAI->GetBasePos().DistanceSqr(TentaclePos + m_pAI->GetForwardDir() * 32.0f)) ? 0.0f : 0.1f; //Rather ad hoc...
					else
						fwd = -0.1f;

					UseDarkness(CAI_Device_Darkness::POWER_DEMON_ARM, CVec3Dfp32(fwd,0,0));

					// Should we start a throw?
					if ((m_DarknessUseTimer % 20 == 19) &&
						((Random * 100 < m_DarknessUseTimer) || (TargetDistSqr > Sqr(m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETDEMONARMREACH), m_pAI->m_pGameObject->m_iObject)))))
					{
						SetDarknessState(DARKNESSSTATE_DEMONARM_THROWING);
					}
				}
				else
				{
					// Swing a while, then throw
					m_pAI->OnTrackObj(_WhackTarget, 10, false, m_pAI->m_bLookSoft);
					UseDarkness(CAI_Device_Darkness::POWER_DEMON_ARM, CVec3Dfp32(1.0f,0,0));
					if (m_DarknessUseTimer > 40)
					{
						SetDarknessState(DARKNESSSTATE_DEMONARM_THROWING);
					}
				}
			}
		}
		else
		{
			// Grabbing, but no grab. Quit trying after a short while
			SetDarknessState(DARKNESSSTATE_DEMONARM_GRABBING);
			m_pAI->OnTrackObj(_GrabTarget, 5, false, m_pAI->m_bLookSoft);
			if (m_DarknessUseTimer > 20)
				StopUsingDarkness();
			else
				UseDarkness(CAI_Device_Darkness::POWER_DEMON_ARM);
		}
	}
	// Do we have a grab target?
	else if (pGrabTarget)
	{
		// Are we aiming towards the grab target?
 		if (m_pAI->CosHeadingDiff(_GrabTarget) > 0.99)
		{
			// Try to grab or give up if arm isn't activated shortly
			SetDarknessState(DARKNESSSTATE_DEMONARM_GRABBING);
			m_pAI->OnTrackObj(_GrabTarget, 5, false, m_pAI->m_bLookSoft);
			if (m_DarknessUseTimer > 20)
				StopUsingDarkness();
			else
				UseDarkness(CAI_Device_Darkness::POWER_DEMON_ARM);
		}
		else
		{
			// Look at grab target. Keep the schnabel in check for now.
			SetDarknessState(DARKNESSSTATE_DEMONARM_PREGRAB);
			m_pAI->OnTrackObj(_GrabTarget, 10, false, m_pAI->m_bLookSoft);

			// Give up if trying this too long
			if (m_DarknessUseTimer > 60)
				StopUsingDarkness();
		}
	}
	else
	{
		// No grab target, do nothing
		StopUsingDarkness();
	}
}

void CAI_ScriptHandler::SetDarknessState(int _State)
{
	if (m_DarknessState != _State)
	{
		m_DarknessState = _State;
		m_DarknessUseTimer = 0;
	}
}

void CAI_ScriptHandler::UseDarkness(int _Power, const CVec3Dfp32& _Move)
{
	// Always drain
	int Usage = CAI_Device_Darkness::USAGE_DRAIN;
	if (_Power != CAI_Device_Darkness::POWER_INVALID)
	{
		Usage |= CAI_Device_Darkness::USAGE_ACTIVATE;
	}
	m_pAI->m_DeviceDarkness.Use(Usage, _Power, _Move);
}

void CAI_ScriptHandler::StopUsingDarkness()
{
	// Clears darkness usage stuff and raises "behaviour finished" event to allow script detection
	// Should perhaps be a separate event, but it'll work well for DarkJackie...

	m_pAI->m_EventHandler.RaiseEvent(CAI_EventHandler::ON_END_DARKNESS,m_pAI->GetObjectID());
	ClearUseDarkness();
}

void CAI_ScriptHandler::ClearUseDarkness()
{
	m_liAction[LOOK] = 0;
	m_liAction[MOVE] = 0;
	m_liAction[DARKNESS] = 0;
	m_DarknessPower = CAI_Device_Darkness::POWER_INVALID;
	m_DarknessState = DARKNESSSTATE_INIT;
	m_DarknessUseTimer = 0;
	m_DarknessTarget = 0;
	m_DarknessSecondaryTarget = 0;
	m_DarknessUsage = CAI_Device_Darkness::USAGE_NONE;
}

CVec3Dfp32 CAI_ScriptHandler::GetDarknessPosition()
{
	CVec3Dfp32 DarknessPos = CVec3Dfp32(_FP32_MAX);
	CWObject_Message Msg(OBJMSG_CHAR_GETDARKNESSPOSITION);
	Msg.m_pData = &DarknessPos;
	Msg.m_DataSize = sizeof(DarknessPos);
	m_pAI->m_pServer->Message_SendToObject(Msg, m_pAI->m_pGameObject->m_iObject);
	return DarknessPos;
}


CWO_ScenePoint* CAI_ScriptHandler::GetMoveToScenepoint()
{
	if ((m_liAction[MOVE] == MOVE_TO_SCENEPOINT))
	{
		if (m_pDoorScenePoint)
		{
			return(m_pDoorScenePoint);
		}
		else
		{
			return(m_pScenePoint);
		}
	}
	else
	{
		return(NULL);
	}
};

//Creates commands that perform the current actions 
void CAI_ScriptHandler::OnPerformActions()
{
	MAUTOSTRIP(CAI_ScriptHandler_OnPerformActions, MAUTOSTRIP_VOID);
	//Go through all actions and perform those that should be performed
	for (int i = MIN_ACTION; i < MAX_ACTION; i++)
	{
		//Do we have an action of this kind and the corresponding device is available?
		if (m_liAction[i] && (m_pAI->m_lDevices[ms_iDeviceMap[i]]->IsAvailable()))
			switch (m_liAction[i])
			{
			case CONTINUOUS_MOVE:
				{
					if (m_MoveParam != CVec3Dfp32(_FP32_MAX))
					{
						m_pAI->m_DeviceMove.Use(m_MoveParam);
						if (m_MoveParam.LengthSqr() <= 0.001f)
						{	// We're paused and probably playing ana animation.
							// Track look object here as actions are stopped
							CVec3Dfp32 LookDir = m_pAI->GetLookDir();
							if (m_pAI->m_iLookAtObject)
							{
								m_pAI->m_pGameObject->AI_SetEyeLookDir(LookDir, m_pAI->m_iLookAtObject);
							}
							else
							{
								m_pAI->m_pGameObject->AI_SetEyeLookDir(LookDir,0);
							}
						}
					}
					else
					{
						m_pAI->m_DeviceMove.Use(0);
						m_liAction[MOVE] = 0;
					};
				};
				break;
			case SINGLE_JUMP:
				{
					m_pAI->m_DeviceJump.Use();
					m_liAction[JUMP] = 0;
				};
				break;
			case SINGLE_SWITCH_WEAPON:
				{
					if (!m_pAI->Wait())
					{
						if (m_iWeaponParam == -1)
							m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH_PREVIOUS);
						else if (m_iWeaponParam == 0)
							m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH_NEXT);
						else if (m_iWeaponParam == -2)
							m_pAI->m_Weapon.SwitchToBestWeapon(CAI_WeaponHandler::RANGED_ONLY);
						else if (m_iWeaponParam == -3)
							m_pAI->m_Weapon.SwitchToBestWeapon(CAI_WeaponHandler::MELEE_ONLY);
						else if (m_iWeaponParam == -4)
							m_pAI->m_Weapon.SwitchToBestWeapon(CAI_WeaponHandler::NONWEAPON_ONLY);
						else
							//Deprecated
							//m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, m_iWeaponParam - 1);
							m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH_NEXT);

						m_liAction[WEAPON] = 0;
					};
				};
				break;
			case SINGLE_SWITCH_ITEM:
				{
					if (!m_pAI->Wait())
					{
						if (m_iItemParam == -1)
							m_pAI->m_DeviceItem.Use(CAI_Device_Item::SWITCH_PREVIOUS);
						else if (m_iItemParam == 0)
							m_pAI->m_DeviceItem.Use(CAI_Device_Item::SWITCH_NEXT);
						else
						{
							//Try to switch to item of given type
							m_pAI->m_Item.WieldItem(m_iItemParam);
						}
						m_liAction[ITEM] = 0;
					};
				};
				break;
			case SINGLE_USE_WEAPON:
				{
					//Try to use wepon immediately
					m_pAI->m_DeviceWeapon.UseContinuous(m_iWeaponParam);
					m_liAction[WEAPON] = 0;
				};
				break;
			case SINGLE_USE_ITEM:
				{
					//Try to use item immediately
					m_pAI->m_DeviceItem.UseContinuous(m_iItemParam);
					m_liAction[ITEM] = 0;
				};
				break;
			case TURRET_MOUNT:
				{	// Handle being mounted
					bool bReleaseMount = false;
					CWObject * pObj = m_pAI->m_pServer->Object_Get(m_pAI->m_iMountParam);
					if (pObj)
					{	// Get the turret matrix
						CMat4Dfp32 Mat;
						CWObject_Message Msg(OBJSYSMSG_GETCAMERA);
						Msg.m_pData = &Mat;
						Msg.m_DataSize = sizeof(Mat);
						m_pAI->m_pServer->Message_SendToObject(Msg, m_pAI->m_iMountParam);
						CVec3Dfp32 MountPos = Mat.GetRow(3);
						// OK, now we should somehow track enemies and hostiles using this information
						m_pAI->m_DeviceMove.Use();
						bool bEnemy = false;
						int iTarget = 0;
						CVec3Dfp32 TargetPos;
						CVec3Dfp32 TargetHeadPos;
						INVALIDATE_POS(TargetPos);
						INVALIDATE_POS(TargetHeadPos);
						if (m_pScenePoint)
						{
							if (!m_pAI->m_AH.RequestScenePoint(m_pScenePoint,3))
							{
								bReleaseMount = true;
							}
						}
						if (m_iTargetParam)
						{
							iTarget = m_iTargetParam;
							bEnemy = true;
						}
						else if (m_iSecondaryTargetParam)
						{
							iTarget = m_iSecondaryTargetParam;
							bEnemy = true;
						}
						else if (m_pAI->m_AH.m_iTarget)
						{
							iTarget = m_pAI->m_AH.m_iTarget;
							CAI_AgentInfo* pTarget = m_pAI->m_KB.GetAgentInfo(iTarget);
							if ((pTarget)&&((pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)||(pTarget->GetGetLastSpottedTicks() < m_pAI->m_Patience)))
							{
								bEnemy = true;
							}
						}
						else if (m_pAI->m_AH.m_iTarget2)
						{
							iTarget = m_pAI->m_AH.m_iTarget2;
							bEnemy = true;
						}
						else if (m_pAI->m_AH.m_iHostile)
						{
							iTarget = m_pAI->m_AH.m_iHostile;
						}
						else if (m_pAI->m_AH.m_iHostile2)
						{
							iTarget = m_pAI->m_AH.m_iHostile2;
						}
						else if ((m_pScenePoint)&&(m_pScenePoint->GetType() & (CWO_ScenePoint::ROAM | CWO_ScenePoint::SEARCH)))
						{	// Use m_iInvestigateObj primarily
							if (m_pAI->m_AH.m_iInvestigateObj)
							{
								iTarget = m_pAI->m_AH.m_iInvestigateObj;
								TargetHeadPos = m_pAI->GetHeadPos(iTarget);
								TargetPos = TargetHeadPos;
							}
							else if (m_pScenePoint->GetType() & CWO_ScenePoint::SEARCH)
							{
								if (m_pAI->m_AH.m_iInvestigate)
								{
									iTarget = m_pAI->m_AH.m_iInvestigate;
								}
								else if (m_pAI->m_AH.m_iInvestigate2)
								{
									iTarget = m_pAI->m_AH.m_iInvestigate2;
								}
							}
							if (!iTarget)
							{	// Sweep area robotically using m_pAI->m_Timer and the mighty modulus
								TargetPos = MountPos + m_pScenePoint->GetSweepDir(m_pAI->m_Timer,m_pAI->GetAITicksPerSecond() * 3) * 100.0f;
								TargetHeadPos = TargetPos;
							}	
						}
						if ((INVALID_POS(TargetHeadPos))&&(iTarget))
						{
							CAI_AgentInfo* pTarget = m_pAI->m_KB.GetAgentInfo(iTarget);
							if ((pTarget)&&(pTarget->GetCurAwareness() >= CAI_AgentInfo::NOTICED))
							{	// Target believed pos
								TargetHeadPos = pTarget->GetSuspectedHeadPosition();
								TargetPos = pTarget->GetBasePos();
							}
							else
							{	// Object pos
								TargetHeadPos = m_pAI->GetHeadPos(iTarget);
								TargetPos = m_pAI->GetBasePos(iTarget);
							}
						}
						
						if ((bEnemy)&&(m_pScenePoint)&&(m_pScenePoint->GetType() & CWO_ScenePoint::ROAM))
						{	// Shouldn't use ROAM scenepoints when firing
							bReleaseMount = true;
						}
						if (VALID_POS(TargetHeadPos))
						{
							//TrackDir.Normalize(); Not needed(?)
							int TrackingTime = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(CWObject_Turret::OBJMSG_TURRET_GETTRACKINGTIME), m_pAI->m_iMountParam);
							if (bEnemy)
							{	// Fast
								m_pAI->OnTrackPos(TargetHeadPos, Max(TrackingTime, 6), false, false);
								
								// Shoot if target is in mount's usable FOV
								CRPG_Object_Item* pWeapon = NULL;
								CWObject_Message GetWeapon = CWObject_Message(CWObject_Turret::OBJMSG_TURRET_GETWEAPON);
								GetWeapon.m_pData = &pWeapon;
								if (m_pAI->m_pServer->Message_SendToObject(GetWeapon, m_pAI->m_iMountParam) && pWeapon)
								{
									if (CAI_Weapon::IsInUsableFOV(TargetHeadPos, m_pAI, pWeapon))
									{
										// m_pAI->m_DeviceWeapon.Use();
										m_pAI->m_DeviceWeapon.UsePeriodic();
									}
								}
							}
							else
							{	// Slowly
								m_pAI->OnTrackPos(TargetHeadPos,Max(TrackingTime, 10),false,false);
							}
							// Handle eyeball tracking
							CVec3Dfp32 TrackDir = TargetHeadPos - m_pAI->GetHeadPos();
							TrackDir.Normalize();
							m_pAI->m_pGameObject->AI_SetEyeLookDir(TrackDir,iTarget);
							if (m_pScenePoint)
							{	// Handle the scenepoint too
								if (m_pAI->m_AH.RequestScenePoint(m_pScenePoint,3))
								{	// We don't care about pos and look of user to continue using the scenepoint
									// Arc for COMBAT Sp is the only criterium
									if ((m_pScenePoint->GetType() & CWO_ScenePoint::TACTICAL)&&(VALID_POS(TargetPos)))
									{
										if (!m_pScenePoint->InFrontArc(TargetPos))
										{
											if ((bEnemy)||(!m_pScenePoint->GetNoTargetTacFlag()))
											{
												bReleaseMount = true;
											}
										}
									}
#ifndef M_RTM
									m_pAI->m_AH.DebugDrawScenePoint(m_pScenePoint,true);
#endif
									if (m_pScenePoint->GetBehaviourDuration() <= 0.0f)
									{
										m_ScenePointStayTimeout = m_pAI->m_Timer + m_pAI->GetAITicksPerSecond();
									}
									if (m_pAI->m_Timer > m_ScenePointStayTimeout)
									{	// Unmount as well
										bReleaseMount = true;
									}	
								}
								else
								{
									bReleaseMount = true;
								}
							}
						}
						else
						{	// No target to track
							if (iTarget)
							{
								bReleaseMount = true;
							}
						}
					}
					else
					{
						bReleaseMount = true;
					}

					if (bReleaseMount)
					{
						CWObject_Message Msg(OBJMSG_CHAR_MOUNT);
						Msg.m_pData = NULL;
						m_pAI->m_pServer->Message_SendToObject(Msg, m_pAI->GetObjectID());
						m_pAI->m_DeviceWeapon.SetPressDuration(m_pAI->m_PressDuration);
						m_pAI->m_DeviceWeapon.SetPeriod(m_pAI->m_ShootPeriod);
						m_liAction[LOOK] = 0;
						m_pAI->m_iMountParam = 0;
						if (m_pScenePoint)
						{
							m_pAI->NoPerfectPlacement();
							m_ScenePointStayTimeout = -1;
							m_ScenePointBlendTicks = 0;
							m_pScenePoint = NULL;
						}
					}
				};
				break;
			case MOVE_TO:
				{
					//Are we at destination or don't we have a position to go to or
					CVec3Dfp32 OurPos = m_pAI->m_PathFinder.SafeGetPathPosition(m_pAI->GetBasePos(),2,1);
					if ((OurPos.DistanceSqr(m_MoveParam) < Sqr(8.0f))||(m_MoveParam == CVec3Dfp32(_FP32_MAX)))
					{
						//Stop and release move action.
						m_pAI->m_DeviceMove.Use(0);
						m_pAI->OnPreScriptAction(MOVE_TO,false);
						m_liAction[MOVE] = 0;	
					} 
					else 
					{
						//We are not at position, navigate there
						m_pAI->m_bNoEscapeSeqMoves = false;
						int32 moveResult = m_pAI->OnMove(m_MoveParam,m_pAI->m_IdleMaxSpeed,false,false,NULL);
						if (!m_pAI->CheckMoveresult(moveResult,NULL))
						{	// Move action failed
							ConOutL("Script MOVE_TO failed");
							//Stop and release move action.
							m_pAI->m_DeviceMove.Use(0);
							m_pAI->OnPreScriptAction(MOVE_TO,false);
							m_liAction[MOVE] = 0;	
						};
					};
				};
				break;
			case MOVE_TO_SCENEPOINT:
				{
 					if ((m_pScenePoint)||(m_pDoorScenePoint))
					{
						// ***
						if (m_pAI->DebugTarget())
						{
							bool wtf = true;
						}
						// ***
						bool bCheckValidTeams = m_pAI->m_AH.ShouldCheckForValidTeams();
						m_pAI->m_AH.SetCheckForValidTeams(false);
						bool bRequestResult = false;
						CWO_ScenePoint* pSP = NULL;
						if (m_pDoorScenePoint)
						{
							pSP = m_pDoorScenePoint;
							m_pAI->m_bCollAvoidanceBlocked = true;						
							if (m_pScenePoint)
							{	// Keep m_pScenePoint alive
								if (!m_pAI->m_AH.RequestScenePoint(m_pScenePoint,3))
								{
									m_pScenePoint = NULL;
								}
							}
							if (m_pAI->m_AH.RequestScenePoint(m_pDoorScenePoint,3))
							{
								bRequestResult = true;
							}
						}
						else if (m_pScenePoint)
						{
							pSP = m_pScenePoint;
							if (m_pAI->m_AH.RequestScenePoint(m_pScenePoint,3))
							{
								bRequestResult = true;
							}
						}
						m_pAI->m_AH.SetCheckForValidTeams(bCheckValidTeams);
						if (bRequestResult)
						{
							if (m_pAI->m_AH.StopForPlayer())
							{
								m_pAI->m_DeviceMove.Use();
								break;
							}
							// Measure the range to the scenepoint
							// If within its radius we start looking to get inside its arc
							CVec3Dfp32 OurPos = m_pAI->GetBasePos();
							CVec3Dfp32 OurDir = m_pAI->GetLookDir();
							CVec3Dfp32 SPDir = pSP->GetDirection();
							CVec3Dfp32 SPPos = pSP->GetPathPosition(m_pAI->m_pServer);
							if ((m_ScenePointStayTimeout == -1)&&(m_pAI->SqrDistanceToUs(SPPos) <= Sqr(32.0f)))
							{
								m_pAI->OnTrackDir(SPDir,0,10,false,false);
							}
							if ((m_ScenePointStayTimeout > 0)||(pSP->IsAt(OurPos)))
							{	// We're there dude, let's look at the bright colors
								if ((m_ScenePointStayTimeout == -1)&&(OurDir * SPDir >= 0.99f)&&(pSP->GetSqrRadius() < Sqr(8.0f)))
								{
									m_pAI->SetPerfectPlacement(pSP);
									if (m_ScenePointBlendTicks)
									{
										m_ScenePointBlendTicks--;
									}
								}
								if ((m_ScenePointStayTimeout > 0)||((pSP->IsAligned(OurDir))&&(!m_ScenePointBlendTicks)))
								{
									m_pAI->NoPerfectPlacement();
									if (m_ScenePointStayTimeout == -1)
									{
										int StayTicks = (int)(pSP->GetBehaviourDuration() * m_pAI->GetAITicksPerSecond());
										m_ScenePointStayTimeout = m_pAI->m_Timer + StayTicks;
										if (pSP->GetType() & (CWO_ScenePoint::TACTICAL | CWO_ScenePoint::COVER))
										{
											m_pAI->m_AH.ActivateScenePoint(pSP,CAI_Action::PRIO_COMBAT);
										}
										else if (pSP->GetType() & CWO_ScenePoint::SEARCH)
										{
											m_pAI->m_AH.ActivateScenePoint(pSP,CAI_Action::PRIO_ALERT);
										}
										else
										{
											m_pAI->m_AH.ActivateScenePoint(pSP,CAI_Action::PRIO_IDLE);
										}

										if (pSP != m_pScenePoint)
										{	// A script message changed the value of m_pScenePoint
											return;
										}
										if ((pSP->GetBehaviour())&&(StayTicks <= 0))
										{
											m_ScenePointStayTimeout = m_pAI->m_Timer + m_pAI->GetAITicksPerSecond();
										}
									}
								#ifndef M_RTM
									m_pAI->m_AH.DebugDrawScenePoint(pSP,true);
								#endif
									if (pSP->GetBehaviour())
									{
										if (pSP->PlayOnce())
										{
											if (m_pAI->m_bBehaviourRunning)
											{
												m_ScenePointStayTimeout = m_pAI->m_Timer + 1;
											}
										}
										else if (pSP->GetBehaviourDuration() <= 0.0f)
										{
											m_ScenePointStayTimeout = m_pAI->m_Timer + m_pAI->GetAITicksPerSecond();
										}
									}
									if (m_pAI->m_Timer > m_ScenePointStayTimeout)
									{
										if (m_pDoorScenePoint)
										{
											if (!m_pScenePoint)
											{
												m_pAI->OnPreScriptAction(MOVE_TO_SCENEPOINT,false);
												m_liAction[MOVE] = 0;
											}
											m_ScenePointStayTimeout = -1;
											m_ScenePointBlendTicks = 0;
											m_pDoorScenePoint = NULL;
										}
										else
										{
											m_pAI->OnPreScriptAction(MOVE_TO_SCENEPOINT,false);
											m_liAction[MOVE] = 0;
											m_ScenePointStayTimeout = -1;
											m_ScenePointBlendTicks = 0;
											m_pScenePoint = NULL;
										}
									}
								}
								else
								{	// Look there until we have aligned up
									m_pAI->OnTrackDir(SPDir,0,10,false,false);
								}
							}
							else
							{	// Go there
							#ifndef M_RTM
								m_pAI->m_AH.DebugDrawScenePoint(pSP,false);
							#endif
								//Move to scenepoint (idle speed or walk when close to allow for breaking distance)
								// Handle walkstops
								if ((!pSP->IsWaypoint())&&(pSP->GetSqrRadius() >= Sqr(16.0f)))
								{
									m_pAI->CheckWalkStop(SPPos);
								}
								
								CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
								if ((!pCD)||(pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOLOOK))
								{	// Keep walking; we're walkstarting or walkstopping
									m_pAI->m_DeviceMove.Use();
									break;
								}
								fp32 Speed = m_pAI->m_IdleMaxSpeed;
								if (!m_pAI->OnNudge(pSP,Speed,true))
								{
									int32 moveResult = m_pAI->OnMove(SPPos,Speed,false,false,pSP);
									if ((moveResult != CAI_Core::MOVE_DEVICE_BUSY)&&(!m_pAI->CheckMoveresult(moveResult,pSP)))
									{	// Move action failed
									#ifndef M_RTM
										CStr name = m_pAI->m_pGameObject->GetName();
										ConOut(CStr("Script MOVE_TO_SCENEPOINT failed: " + name + " " + pSP->GetName()));
									#endif
										//Stop and release move action.
										if (m_pDoorScenePoint)
										{
											if (!m_pScenePoint)
											{
												m_pAI->OnPreScriptAction(MOVE_TO_SCENEPOINT,false);
												m_liAction[MOVE] = 0;
											}
											m_ScenePointStayTimeout = -1;
											m_ScenePointBlendTicks = 0;
											m_pDoorScenePoint = NULL;
										}
										else
										{
											m_pAI->OnPreScriptAction(MOVE_TO_SCENEPOINT,false);
											m_liAction[MOVE] = 0;
											m_ScenePointStayTimeout = -1;
											m_ScenePointBlendTicks = 0;
											m_pScenePoint = NULL;
										}
									}
									else if ((m_pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)&&(m_pAI->m_DeviceMove.IsAvailable()))
									{
										if ((m_ScenePointStayTimeout == -1)&&
											(m_pAI->SqrDistanceToUs(SPPos) <= Sqr(16.0f))&&
											(pSP->GetSqrRadius() < Sqr(8.0f))&&
											(m_pAI->m_PrevSpeed < 1.0f))
										{
											{	// Nudge us closer
												m_pAI->AddMoveTowardsPositionCmd(SPPos,m_pAI->m_IdleMaxSpeed);
											}
										}
									}
								}
							}
						}
						else
						{	// Request scenepoint failed
							// m_pAI->ResetPathSearch();
						#ifndef M_RTM
							CStr name = m_pAI->m_pGameObject->GetName();
							ConOut(CStr("Script MOVE_TO_SCENEPOINT request failed: " + name + " " + pSP->GetName()));
						#endif
							m_pAI->NoPerfectPlacement();
							if (m_pDoorScenePoint)
							{
								if (!m_pScenePoint)
								{
									m_pAI->OnPreScriptAction(MOVE_TO_SCENEPOINT,false);
									m_liAction[MOVE] = 0;
								}
								m_ScenePointStayTimeout = -1;
								m_ScenePointBlendTicks = 0;
								m_pDoorScenePoint = NULL;
							}
							else
							{
								m_pAI->OnPreScriptAction(MOVE_TO_SCENEPOINT,false);
								m_liAction[MOVE] = 0;
								m_ScenePointStayTimeout = -1;
								m_ScenePointBlendTicks = 0;
								m_pScenePoint = NULL;
							}
						}
					}
				};
				break;
			case LOOK_AT:
				{
					CWObject * pObj = m_pAI->m_pServer->Object_Get(m_iTargetParam);
					if (pObj)
					{
						m_pAI->OnTrackObj(m_iTargetParam, 10, false, m_pAI->m_bLookSoft);
						/* LOOK_AT is the new Continuous look, right?
						if (m_pAI->GetAITick() > m_LookTimeout)
						{
							m_liAction[LOOK] = 0;
						}
						*/
					}
					else
					{
						m_pAI->m_DeviceLook.Use();
						m_liAction[LOOK] = 0;
						m_LookTimeout = 0;
					};
				};
				break;
			case SNAP_LOOK_AT:
				{
					CWObject* pObj = m_pAI->m_pServer->Object_Get(m_iTargetParam);
					if (pObj)
					{
						m_pAI->AddAimAtPositionCmd(m_pAI->GetFocusPosition(pObj),0);
						if (m_pAI->GetAITick() > m_LookTimeout)
						{
							m_liAction[LOOK] = 0;
						}
					}
					else
					{
						m_pAI->m_DeviceLook.Use();
						m_liAction[LOOK] = 0;
						m_LookTimeout = 0;
					};
				};
				break;
			case ATTACK:
				{	//Attack uses both look and weapon device, so we only have to perform 
					//actions during the look action round.
					if ((i == WEAPON) && (m_liAction[LOOK] == ATTACK))
					{
						break;
					}

					CVec3Dfp32 TargetPos;
					INVALIDATE_POS(TargetPos);
					m_pAI->m_DeviceStance.SetTargetInFOV(true);
					m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
					m_pAI->Reload(1.0f);
					if (m_iTargetParam)
					{
						CAI_AgentInfo* pAgent = m_pAI->m_KB.GetAgentInfo(m_iTargetParam);
						if (pAgent)
						{
							if (pAgent->GetCurRelation() < CAI_AgentInfo::ENEMY)
							{
								pAgent->SetRelation(CAI_AgentInfo::ENEMY);
							}
							if (pAgent->GetCurAwareness() < CAI_AgentInfo::SPOTTED)
							{
								pAgent->SetAwareness(CAI_AgentInfo::SPOTTED,false,true);
							}
							TargetPos = m_pAI->GetAimPosition(pAgent);
						}
					}

					if (INVALID_POS(TargetPos))
					{
						CWObject* pObj = m_pAI->m_pServer->Object_Get(m_iTargetParam);
						if (pObj)
						{
							TargetPos = pObj->GetPosition();
							CVec3Dfp32 Dim = m_pAI->GetBoundBoxDimensions(pObj);
							TargetPos[2] += Dim[2] * 0.5f;
						}
					}

					if (VALID_POS(TargetPos))
					{
						CVec3Dfp32 AimDir,LookDir;
						m_liAction[LOOK] = ATTACK;
						m_liAction[WEAPON] = ATTACK;
						LookDir = m_pAI->GetLookDir();
						if (m_pAI->SqrDistanceToUs(TargetPos) >= Sqr(48.0f))
						{	// Far enough away to aim at in a regular manner
							AimDir = TargetPos - m_pAI->GetAimPosition();
							AimDir.Normalize();
						}
						else
						{	// This may result in a miss
							AimDir = TargetPos - m_pAI->GetLookPos();
							AimDir.Normalize();
						}
						m_pAI->OnTrackDir(AimDir,m_iTargetParam,3,false,false);
						if (LookDir * AimDir >= 0.707f)
						{
							if (m_pAI->m_Weapon.GetWielded()->GetType() == CAI_Weapon::RANGED_RAPIDFIRE)
							{
								m_pAI->m_DeviceWeapon.Use();
							}
							else
							{
								m_pAI->m_DeviceWeapon.UsePeriodic();
							}
						}
						// m_pAI->OnTrackPos(TargetPos,10,false,m_pAI->m_bLookSoft);
					}
					else
					{	// m_iTargetParam == 0
						m_liAction[LOOK] = 0;
						m_liAction[WEAPON] = 0;
					}
				}
				break;
			case AIM:
				{
					//Aim uses both look and weapon device, so we only have to perform 
					//actions during the look action round.
					if ((i == WEAPON) && (m_liAction[LOOK] == AIM))
					{
						break;
					}

					CVec3Dfp32 TargetPos;
					INVALIDATE_POS(TargetPos);
					m_pAI->m_DeviceStance.SetTargetInFOV(true);
					m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
					m_pAI->Reload(1.0f);
					if (m_iTargetParam)
					{
						CAI_AgentInfo* pAgent = m_pAI->m_KB.GetAgentInfo(m_iTargetParam);
						if (pAgent)
						{
							if (pAgent->GetCurRelation() < CAI_AgentInfo::ENEMY)
							{
								pAgent->SetRelation(CAI_AgentInfo::ENEMY);
							}
							if (pAgent->GetCurAwareness() < CAI_AgentInfo::SPOTTED)
							{
								pAgent->SetAwareness(CAI_AgentInfo::SPOTTED,false,true);
							}
							TargetPos = m_pAI->GetAimPosition(pAgent);
						}
					}

					if (INVALID_POS(TargetPos))
					{
						CWObject* pObj = m_pAI->m_pServer->Object_Get(m_iTargetParam);
						if (pObj)
						{
							TargetPos = pObj->GetPosition();
							CVec3Dfp32 Dim = m_pAI->GetBoundBoxDimensions(pObj);
							TargetPos[2] += Dim[2] * 0.5f;
						}
					}

					if (VALID_POS(TargetPos))
					{
						CVec3Dfp32 AimDir,LookDir;
						m_liAction[LOOK] = AIM;
						m_liAction[WEAPON] = AIM;
						LookDir = m_pAI->GetLookDir();
						if (m_pAI->SqrDistanceToUs(TargetPos) >= Sqr(64.0f))
						{	// Far enough away to aim at in a regular manner
							AimDir = TargetPos - m_pAI->GetAimPosition();
							AimDir.Normalize();
						}
						else
						{	// This may result in a miss
							AimDir = TargetPos - m_pAI->GetLookPos();
							AimDir.Normalize();
						}
						m_pAI->OnTrackDir(AimDir,m_iTargetParam,3,false,false);
			
						// m_pAI->OnTrackPos(TargetPos,10,false,m_pAI->m_bLookSoft);
					}
					else
					{	// m_iTargetParam == 0
						m_liAction[LOOK] = 0;
						m_liAction[WEAPON] = 0;
					}

				};
				break;
			case AGGRESSIVE:
				{
					//Aggressive uses both look and weapon device, so we only have to perform 
					//actions during the look action round.
					if ((i == WEAPON) && (m_liAction[LOOK] == AGGRESSIVE))
					{
						break;
					}
					
					OnAggressive();
				};
				break;
			case USE_DARKNESS:
				{
					// Using darkness may use move, look and darkness devices. Perform during darkness.
					if (i < DARKNESS)
					{
						break;
					}

					OnUseDarkness();
				};
				break;
			};
	};
};


//Move along a path on the ground using pathfinding, stopping the path when it gets too far away, and looking in the paths 
//direction if look device is available. If we should force follow, we don't stop path when
//falling behind and always just move blindly towards the path.
void CAI_ScriptHandler::OnGroundFollowPath()
{
	MAUTOSTRIP(CAI_ScriptHandler_OnGroundFollowPath, MAUTOSTRIP_VOID);

	//Check that at least move device is available
	if (!m_pAI || !m_pAI->m_DeviceMove.IsAvailable())
	{
		//Device unavailable, stop path if necessary and abort!
		if (!(m_iFollowMode & FOLLOW_FORCE))
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
		return;
	}

	/*
	m_pAI->OnGroundFollowPath(m_pFD);
	return;
	*/

	//Get path object
	CWObject * pPath;
	if (!m_iPath || 
		!(pPath = m_pAI->m_pServer->Object_Get(m_iPath)))
	{
		//Can't get path, stay put and abort!
		m_pAI->OnIdle();
		m_iPath = 0;
		//Turn on gravity and animcontrol if necessary
		if (m_bTurnOnGravity && !m_pAI->m_bWeightless)
		{
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 1), m_pAI->m_pGameObject->m_iObject);
			m_bTurnOnGravity = false;
		};
		if (m_bTurnOnAnimControl)
		{
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 1), m_pAI->m_pGameObject->m_iObject);
			m_bTurnOnAnimControl = false;
		};

		return;
	};

#ifndef M_RTM
	m_pAI->DebugDrawPath(m_iPath,100);
#endif

	//Should we follow the paths relative movement, but not it's actual position?
	if (m_iFollowMode & FOLLOW_RELATIVE)
	{
		//Should we force follow relative?
		if (m_iFollowMode & FOLLOW_FORCE)
		{
			//Look in the same direction as path
			CVec3Dfp32 Dir = CVec3Dfp32::GetMatrixRow(pPath->GetPositionMatrix(),0);
			m_pAI->OnTrackDir(Dir,0,6,false,false);
			//Move in the same fashion as path
			CVec3Dfp32 PathMove = m_pAI->GetBasePos() - m_pAI->GetLastBasePos(); 
			fp32 Speed = Min(m_pAI->m_IdleMaxSpeed, PathMove.Length());
			m_pAI->AddMoveTowardsPositionCmd(m_pAI->GetBasePos() + PathMove, Speed);
		}
		else
		{
			//Non-force following not implemented	
		};
	}
	//Should we "force follow" path?
	else if (m_iFollowMode & FOLLOW_FORCE)
	{
		//Jupp, always move directly towards the sender and look in the same direction as it
		// Lets look in the direction of the path
		CVec3Dfp32 Dir = CVec3Dfp32::GetMatrixRow(pPath->GetPositionMatrix(),0);
		m_pAI->OnTrackDir(Dir,0,6,false,false);
		//The speed at which to move might be tuned better, and perhaps be based on the XY-distance instead of full 3D-distance.
		fp32 Distance;
		if (m_pAI->m_bWallClimb)
			Distance = m_pAI->GetBasePos().Distance(pPath->GetPosition());
		else
			Distance = CAI_Core::XYDistance(m_pAI->GetBasePos(), pPath->GetPosition());
		fp32 Speed = Min(m_pAI->m_IdleMaxSpeed, Max((pPath->GetPosition()).Distance(pPath->GetLastPosition()), Distance));
		// We are considered on the path when we're within half a steplength
		// Speed of 0.7+ is treated as running and otherwise walk
		if (Speed >= 0.7)
		{
			if (Distance >= m_pAI->GetRunStepLength() * 0.5f)
			{
				m_pAI->AddMoveTowardsPositionCmd(pPath->GetPosition(),Speed);
			}
		}
		else
		{
			if (Distance >= m_pAI->GetWalkStepLength() * 0.5f)
			{
				m_pAI->AddMoveTowardsPositionCmd(pPath->GetPosition(),Speed);
			}
		}
	}
	else
	{
		if (!m_bFallenBehind &&
			(m_PathPosition == CVec3Dfp32(_FP32_MAX)))
		{
			CMat4Dfp32 Mat = m_pAI->GetEnginePathMatrix(m_iPath);
			m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
			m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
			m_bFallenBehind = true;
		}
		
		//Check distance to path position
		fp32 DistSqr;
		if (m_pAI->m_bWallClimb)
		{
			DistSqr = m_PathPosition.DistanceSqr(m_pAI->GetBasePos());
		}
		else
		{
			DistSqr = CAI_Core::XYDistanceSqr(m_pAI->GetBasePos(), m_PathPosition);
		}

		m_pAI->Debug_RenderWire(m_pAI->GetBasePos()+CVec3Dfp32(0,0,3), m_PathPosition+CVec3Dfp32(0,0,3),0xffff0000);//DEBUG

		//If we've fallen behind, check where we need to go, or if we've caught up with the path.
		if (m_bFallenBehind)
		{
			//Are we still behind the path?
 			if (DistSqr < m_pAI->m_CaughtUpRangeSqr)
			{
				//We've caught up!
				m_bFallenBehind = false;
				CMat4Dfp32 Mat = m_pAI->GetEnginePathMatrix(m_iPath);
				m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
				m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
				if (m_pAI->m_bWallClimb)
				{
					DistSqr = m_PathPosition.DistanceSqr(m_pAI->GetBasePos());
				}
				else
				{
					DistSqr = CAI_Core::XYDistanceSqr(m_pAI->GetBasePos(), m_PathPosition);
				}

				//Allow some extra time before we start force-propelling path when we've just returned to path.
				m_LastPropelTick = m_pAI->GetAITick() + 10;
				m_bStop = false;
			}
		}
		else
		{
			//Have we fallen behind just now?
			if (DistSqr > m_pAI->m_FallenBehindRangeSqr)
			{
				//Jupp, we must catch up with path. 
 				m_bFallenBehind = true;
				m_PathPosition = m_pAI->m_PathFinder.GetPathPosition(m_PathPosition,10,2); 
			};
		};
		
		//Should we follow path normally or are we behind and need to navigate our way to the path
		if (m_bFallenBehind) 
		{
			//We need to navigate if possible
 			if (m_PathPosition == CVec3Dfp32(_FP32_MAX))
			{
				// Current pathpos is invalid, we propel the path a bit
				// to (hopefully) get a position that is legit next frame
				CMat4Dfp32 Mat;
				if (m_pAI->m_bWallClimb)
				{
					m_pAI->PropelPath(m_iPath, 32, true, false, true, &Mat);
				}
				else
				{
					m_pAI->PropelPath(m_iPath, 32, true, true, true, &Mat);
				}
				m_PathPosition = m_pAI->m_PathFinder.GetPathPosition(CVec3Dfp32::GetRow(Mat, 3), 10, 3);
			}

			if (m_PathPosition == CVec3Dfp32(_FP32_MAX))
			{	//Return position still invalid. Stay put.
				m_pAI->m_DeviceMove.Use();
			}
			else
			{
				//Return position ok! Stop path, and allow escape sequences
	 			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
				m_pAI->m_bNoEscapeSeqMoves = false;
				int32 moveResult = m_pAI->OnMove(m_PathPosition,m_pAI->m_IdleMaxSpeed,false,false,NULL);
				if (!m_pAI->CheckMoveresult(moveResult,NULL))
				{	// Move action failed
					ConOutL("Script OnGroundFollowPath failed");
					m_pAI->m_DeviceMove.Use();
					m_bFallenBehind = true;
				};
			}
		}
		else 
		{
			//Check how far we should propel path each "step"
			fp32 LastMoveLen = (m_pAI->GetBasePos() - m_pAI->GetLastBasePos()).Length();
			fp32 PropelStep = 2.0f * LastMoveLen;
			if (m_pAI->GetPrevSpeed() >= m_pAI->m_ReleaseMaxSpeed * 0.75f)
			{	// Running
				PropelStep = Max3(PropelStep, 2.0f * m_pAI->m_MinPropelStep, 2.0f * m_pAI->m_IdleMaxSpeed);
			}
			else
			{	// Walking
				PropelStep = Max3(PropelStep, m_pAI->m_MinPropelStep, 2.0f * m_pAI->m_IdleMaxSpeed);
			}

			//We're following path normally. Propel path each frame when at stop, 
			//or propel to new position whenever we get close to path
 			if (m_bStop || (DistSqr < Sqr(PropelStep)))
			{
				//Since we allow stops when propelling, path won't move when at a stop until it has 
				//been propelled the number of frames it should stop, regardless of distance
				CVec3Dfp32 PrevPos = m_PathPosition;
				CMat4Dfp32 Mat;
				if (m_pAI->m_bWallClimb)
				{
					m_pAI->PropelPath(m_iPath, PropelStep, true, false, true, &Mat);
				}
				else
				{
					m_pAI->PropelPath(m_iPath, PropelStep, true, true, true, &Mat);
				}
				m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
				m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
				m_LastPropelTick = m_pAI->GetAITick();				

				//Check if (still) at stop
				if (m_PathPosition.AlmostEqual(PrevPos, 0.1f))
					m_bStop = true;
				else
					m_bStop = false;
			}
			//If we haven't propelled path for a while, we're probably stuck and must propel path 
			//past the obstacle and then use pathfinding
			else if (m_pAI->GetAITick() > m_LastPropelTick + 20)
			{
				CMat4Dfp32 Mat;
				if (m_pAI->m_bWallClimb)
				{
					m_pAI->PropelPath(m_iPath, 32, true, false, true, &Mat);
				}
				else
				{
					m_pAI->PropelPath(m_iPath, 32, true, true, true,&Mat);
				}
				m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
				m_PathPosition = CVec3Dfp32::GetRow(Mat, 3);
			}
			else
			{
				//If we don't propel path, make sure it is stopped
				m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
			}

			//Should we stop?
 			if (m_bStop && (DistSqr < Sqr(m_pAI->GetWalkStepLength() / 2.0f)))
			{
				//Look in direction of path
				if (m_LookVector == CVec3Dfp32(_FP32_MAX))
				{
					CMat4Dfp32 Mat = m_pAI->GetEnginePathMatrix(m_iPath);
					m_LookVector = CVec3Dfp32::GetRow(Mat, 0);
				}
				// *** What is this. I am not really sure what to look at here
				m_pAI->OnTrackPos(m_pAI->GetAimPosition() + m_LookVector * 100,10,false,false);

				//Use perfect placement to move to exact stop position
				m_pAI->AddMoveTowardsPositionCmd(m_PathPosition,Min(m_pAI->m_IdleMaxSpeed,m_pAI->GetMaxSpeedWalk()));
			}
			else 
			{
				//Go to path position 
				if (m_pAI->m_DeviceLook.IsAvailable())
				{
					CMat4Dfp32 AheadMat;
					m_pAI->PropelPath(m_iPath,32.0f,true,false,false,&AheadMat);
					// *** Experimental first pass at ep lookahead
					CVec3Dfp32 LookPos = AheadMat.GetRow(3);
					if (VALID_POS(LookPos))
					{
						LookPos += m_pAI->GetUp() * m_pAI->GetCurHeight() * 0.75f;
						m_pAI->OnTrackPos(LookPos,10,false,false); 
#ifndef M_RTM
						// *** Remove this when LookAhead is OK:ed
						if (m_pAI->DebugTarget())
						{
							m_pAI->Debug_RenderVertex(LookPos,kColorYellow,1.0f);
						}
#endif
					}
				}
				m_pAI->AddMoveTowardsPositionCmd(m_PathPosition,m_pAI->m_IdleMaxSpeed);
			}
		};
	};
};


//Move to the path object along the ground, but when the object has been reached, jump and then fly
//along the path. If the we should force follow, we always fly off immediately and fly towards the path object.
void CAI_ScriptHandler::OnFlyFollowPath()
{
	MAUTOSTRIP(CAI_ScriptHandler_OnFlyFollowPath, MAUTOSTRIP_VOID);
	//Check that at least move device is available
	if (!m_pAI->m_DeviceMove.IsAvailable())
		//Device unavailable, abort!
		return;

	//Get path object
	CWObject * pPath;
	if (!m_iPath || 
		!(pPath = m_pAI->m_pServer->Object_Get(m_iPath)))
	{
		//Can't get path, stay put and abort!
		m_pAI->OnIdle();
		m_iPath = 0;

		//Turn on gravity if necessary
		if (m_bTurnOnGravity && !m_pAI->m_bWeightless)
		{
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 1), m_pAI->m_pGameObject->m_iObject);
			m_bTurnOnGravity = false;
		};
		if (m_bTurnOnAnimControl)
		{
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 1), m_pAI->m_pGameObject->m_iObject);
			m_bTurnOnAnimControl = false;
		};

		return;
	};

	//True fly follow?
	if (m_iFollowMode & FOLLOW_TRUE)
	{
		// Are we walking or running?
		fp32 FwdControl;
		if (m_pAI->m_IdleMaxSpeed > 0.75f * m_pAI->GetMaxSpeedForward())
		{
			FwdControl = 1.0f;
		}
		else
		{
			FwdControl = 0.25f;
		}

		//Fly! Switch off gravity and animcontrol
		m_bFlying = true;
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 0), m_pAI->m_pGameObject->m_iObject);
		m_bTurnOnGravity = true;

/*		//Old code, changed so that anim control mode is set when using device
		CWObject_Message ControlMode(OBJMSG_CHAR_GETCONTROLMODE);
		if (m_pAI->m_pServer->Message_SendToObject(ControlMode, m_pAI->m_pGameObject->m_iObject) == PLAYER_CONTROLMODE_ANIMATION)
		{
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 0), m_pAI->m_pGameObject->m_iObject);
			m_bTurnOnAnimControl = true;
		}
*/

		//Have we just reached path?
		if (m_bFallenBehind &&
			(pPath->GetPosition().DistanceSqr(m_pAI->GetBasePos()) < Sqr(Max(16.0f, m_pAI->m_IdleMaxSpeed))))
		{
			m_bFallenBehind = false;
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
		}
		
		//Are we at path?
		if (m_bFallenBehind)
		{
			//Nope, go to path at full speed (or idle max speed, of course)
	 		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
			m_pAI->AddAimAtPositionCmd(m_pAI->GetAimPosition() + CVec3Dfp32::GetMatrixRow(pPath->GetPositionMatrix(), 0));
			CVec3Dfp32 FlightVel = pPath->GetPosition() - m_pAI->GetBasePos();
			FlightVel.Normalize();
			FlightVel *= m_pAI->m_IdleMaxSpeed;
			m_pAI->m_pServer->Object_SetVelocity(m_pAI->m_pGameObject->m_iObject, FlightVel);		
		}
		else
		{
			//Make sure path runs
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);

			//Go to position of path next frame
			CMat4Dfp32 NextMat;
			CMTime Time;
			CWObject_Message TimeMsg(OBJMSG_HOOK_GETTIME);
			TimeMsg.m_pData = &Time;
			TimeMsg.m_DataSize = sizeof(Time);
			m_pAI->m_pServer->Message_SendToObject(TimeMsg, m_iPath);
			Time += CMTime::CreateFromSeconds(0.05f);
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)(&Time), (aint)(&NextMat), m_pAI->m_pGameObject->m_iObject), m_iPath);

			// Move with the velocity necessary to reach the position
			CVec3Dfp32 FlightVel = CVec3Dfp32::GetMatrixRow(NextMat, 3) - m_pAI->GetBasePos();
			if (!(m_pAI->m_UseFlags & CAI_Core::USE_FLYATPATHSPEED) &&
				FlightVel.LengthSqr() > Sqr(m_pAI->m_IdleMaxSpeed))
			{	
				// We don't want to exceed our own speed limit - it's better to lag behind the path
				// Actually we should propel flight paths as well so that we don't lag behind either, 
				// but that might fuck up previous scripts, so we'll have to wait until next project
				FlightVel.Normalize();
				FlightVel *= m_pAI->m_IdleMaxSpeed;
			}
			m_pAI->m_pServer->Object_SetVelocity(m_pAI->m_pGameObject->m_iObject, FlightVel);		

			// Set rotation to match engine path if look device is available
			if (m_pAI->m_DeviceLook.IsAvailable())
			{
				// We use this instead of normal look to enable full body rotation. 
				// This rotation should really be moderated to avoid snapping looks...
				m_pAI->m_DeviceLook.Lock();
				m_pAI->m_pServer->Object_SetRotation(m_pAI->m_pGameObject->m_iObject, NextMat);
			}
			else 
			{
				// Was there a soft look which we cannot allow due to path direction?
				CVec3Dfp32 PathDir = CVec3Dfp32::GetMatrixRow(NextMat, 3) - pPath->GetPosition();
				if (m_pAI->m_DeviceLook.BadLookTarget(PathDir))
				{
					// Override previous look. Track in path direction instead.
					m_pAI->m_DeviceLook.Free();
					m_pAI->OnTrackDir(PathDir, 0, 10, false, false);
				}
			}
		}

		//Lock move device and disable anim control
		m_pAI->m_DeviceMove.Use(CVec3Dfp32(FwdControl,0,0), false);
	}
	else
	{
		//Normal fly follow

		//Should we make a ground move to path position? 
		if (m_bFallenBehind)
		{
			OnGroundFollowPath();
		}
		else
		{
			//Fly! Stop regular movement though, and make sure path is running.
			CVec3Dfp32 FlightVel = pPath->GetPosition() - pPath->GetLastPosition();
			m_pAI->m_DeviceMove.Use(0);
 			//m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);

			//Look in the direction of the engine path object
			m_pAI->AddAimAtPositionCmd(m_pAI->GetAimPosition() + CVec3Dfp32::GetMatrixRow(pPath->GetPositionMatrix(), 0));

			//Jump if we're not flying yet.
			if (!m_bFlying)
			{
				//Up, up and away!
				m_pAI->m_DeviceJump.Use();
				m_bFlying = true;

				//Switch off gravity and animcontrol
				m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 0), m_pAI->m_pGameObject->m_iObject);
				m_bTurnOnGravity = true;
				CWObject_Message ControlMode(OBJMSG_CHAR_GETCONTROLMODE);
				if (m_pAI->m_pServer->Message_SendToObject(ControlMode, m_pAI->m_pGameObject->m_iObject) == PLAYER_CONTROLMODE_ANIMATION)
				{
					m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 0), m_pAI->m_pGameObject->m_iObject);
					m_bTurnOnAnimControl = true;
				}
			};

			m_pAI->m_pServer->Object_SetVelocity(m_pAI->m_pGameObject->m_iObject, FlightVel);		
		}
	}
};


//Teleport to paths position every frame
void CAI_ScriptHandler::OnTeleportFollowMe()
{
	MAUTOSTRIP(CAI_ScriptHandler_OnTeleportFollowMe, MAUTOSTRIP_VOID);
	//Check that at least move device is available
	if (!m_pAI->m_DeviceMove.IsAvailable())
		//Device unavailable, abort!
		return;

	//Get path object
	CWObject * pPath;
	if (!m_iPath || 
		!(pPath = m_pAI->m_pServer->Object_Get(m_iPath)))
	{
		//Can't get path, stay put and abort!
		m_pAI->OnIdle();
		m_iPath = 0;

		//Turn on gravity and animcontrol if necessary
		if (m_bTurnOnGravity && !m_pAI->m_bWeightless)
		{
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 1), m_pAI->m_pGameObject->m_iObject);
			m_bTurnOnGravity = false;
		};
		if (m_bTurnOnAnimControl)
		{
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 1), m_pAI->m_pGameObject->m_iObject);
			m_bTurnOnAnimControl = false;
		};

		return;
	};

	//Are we flying yet?
	if (!m_bFlying)
	{
		//Up, up and away!
		m_pAI->m_DeviceJump.Use();
		m_bFlying = true;

		//Switch off gravity and animcontrol
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 0), m_pAI->m_pGameObject->m_iObject);
		m_bTurnOnGravity = true;
		CWObject_Message ControlMode(OBJMSG_CHAR_GETCONTROLMODE);
		if (m_pAI->m_pServer->Message_SendToObject(ControlMode, m_pAI->m_pGameObject->m_iObject) == PLAYER_CONTROLMODE_ANIMATION)
		{
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 0), m_pAI->m_pGameObject->m_iObject);
			m_bTurnOnAnimControl = true;
		}
	};

	//Teleport! Stop regular movement though, and make sure path is running.
	m_pAI->m_DeviceMove.Use(0);
	m_pAI->m_pServer->Object_SetVelocity(m_pAI->m_pGameObject->m_iObject, CVec3Dfp32(0));		
 	//m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);

	//Teleport to paths predicted position, with paths direction
	m_pAI->m_pServer->Object_SetPosition(m_pAI->m_pGameObject->m_iObject, pPath->GetPositionMatrix());		
};


//Chooses the actions the bot should take to follow the path, if there are available devices
void CAI_ScriptHandler::OnFollowPath()
{
	MAUTOSTRIP(CAI_ScriptHandler_OnFollowPath, MAUTOSTRIP_VOID);
	if (!m_pAI || !m_pAI->IsValid() || !m_iPath || (m_pAI->m_pGameObject->AI_GetPhysFlags() & PLAYER_PHYSFLAGS_IMMOBILE))
		return;

	//We have a path object. Should we teleport, fly or move along the ground?
	if (m_iFollowMode & FOLLOW_TELEPORT)
		OnTeleportFollowMe();
	else if (m_iFollowMode & FOLLOW_FLY)
		OnFlyFollowPath();
	else
		OnGroundFollowPath();
};

//Constructor. Script is initially invalid.
CAI_ScriptHandler::CAI_ScriptHandler()
{
	MAUTOSTRIP(CAI_ScriptHandler_ctor, MAUTOSTRIP_VOID);
	m_pAI = NULL;
	ClearParameters();	
};

// Clear all scripting parameters
void CAI_ScriptHandler::ClearParameters()
{
	m_iAnim = -1;
	m_bTorsoAnim = false;
	m_bAnimStarted = false;
	m_AnimTimer = 0;

	m_iPath = 0;
	m_iFollowMode = FOLLOW_NORMAL;
	m_bFallenBehind = false;
	m_PathPosition = CVec3Dfp32(_FP32_MAX);
	m_bFlying = false;
	m_bTurnOnGravity = false;
	m_bTurnOnAnimControl = false;
	m_MovePosRel = CVec3Dfp32(_FP32_MAX);
	m_LastPropelTick = 0;
	m_bStop = false;
	m_LookVector = CVec3Dfp32(_FP32_MAX);

	for(int i = MIN_ACTION; i < MAX_ACTION; i++)
		m_liAction[i] = 0;
	m_MoveParam = CVec3Dfp32(_FP32_MAX);
	m_LookParam = CVec3Dfp32(_FP32_MAX);
	m_iWeaponParam = -1;
	m_iItemParam = -1;
	m_iTargetParam = 0;
	m_iSecondaryTargetParam = 0;
	m_LookTimeout = 0;
	m_pScenePoint = NULL;
	m_pNextScenePoint = NULL;
	m_pDoorScenePoint = NULL;
	m_ScenePointStayTimeout = -1;
	m_ScenePointBlendTicks = 0;
	m_iMeleeManoeuvre = -1;

	m_iDelayedBehaviour = 0;
	m_iDelayedBehaviourTarget = 0;
	m_DelayedbehaviourMat.Unit();
	m_BehaviourStartTimer = 0;
	
	if (m_pAI)
	{
		m_pAI->OnPreScriptAction(INVALID_ACTION,false);
	}
	ClearUseDarkness();
};



void CAI_ScriptHandler::SetAI(CAI_Core* _pAI)
{
	m_pAI = _pAI;
};

//Set path to follow, specifying following mode
void CAI_ScriptHandler::SetPath(int _iPath, int _iFollowMode)
{
	MAUTOSTRIP(CAI_ScriptHandler_SetPath, MAUTOSTRIP_VOID);
	if (!m_pAI || !m_pAI->IsValid())
		return;

	// *** TEST
	/*
	if (!m_pFD)
		return;
	//Don't do anything if we're already following the path under the exact same circumstances
	if ((m_pFD->m_iPath == _iPath)&&
		(m_pFD->m_iFollowMode == _iFollowMode))
		return;

	m_pFD->Clear();
	m_pAI->SetPath(m_pFD,_iPath, _iFollowMode);
	
	// These lines should follow CAI_ScriptHandler call to SetPath
	// If we already have a normal move action, interrupt it.
	m_liAction[MOVE] = 0;
	m_MoveParam = CVec3Dfp32(_FP32_MAX);
	return;
	*/
	// ***



	//Don't do anything if we're already following the path under the exact same circumstances
	if ((m_iPath == _iPath) &&
		(m_iFollowMode == _iFollowMode))
		return;

	m_iPath = _iPath;
	
	m_pScenePoint = NULL;
	m_pNextScenePoint = NULL;
	m_ScenePointBlendTicks = 0;
	m_ScenePointStayTimeout = -1;

	m_iFollowMode = _iFollowMode;
	m_bFlying = false;
	m_MovePosRel = CVec3Dfp32(_FP32_MAX);

	//Turn on gravity and animcontrol if necessary
	if (m_bTurnOnGravity  && !m_pAI->m_bWeightless)
	{
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 1), m_pAI->m_pGameObject->m_iObject);
		m_bTurnOnGravity = false;
	};
	if (m_bTurnOnAnimControl)
	{
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 1), m_pAI->m_pGameObject->m_iObject);
		m_bTurnOnAnimControl = false;
	};

	//Check if we're at path or not, and notify path accordingly if we're not force following, relative following or teleport following
	if (!(m_iFollowMode & (FOLLOW_FORCE | FOLLOW_RELATIVE | FOLLOW_TELEPORT)))
	{
		//This is somewhat deprecated... FIX
		
		//Always start path to become activator and run any immediate messages and then stop path. 
		//When we script follow path control of that path belongs to the bot!
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, m_pAI->m_pGameObject->m_iObject), m_iPath);

		//Get path object
		CWObject * pPath;
		if ( m_iPath && 
			 (pPath = m_pAI->m_pServer->Object_Get(m_iPath)) &&
			 (CAI_Core::XYDistanceSqr(m_pAI->GetBasePos(), pPath->GetPosition()) < 16*16) )
		{
			//Jupp, we're at path
			m_bFallenBehind = false;
		} 
		else 
		{
			//We're not at path.
			m_bFallenBehind = true;
		};
	};
	
	//No matter if we're at path or not, don't set correct path position now.
	m_PathPosition = CVec3Dfp32(_FP32_MAX);
	m_LookVector = CVec3Dfp32(_FP32_MAX);
	m_LastPropelTick = m_pAI->GetAITick();				
	m_bStop = false;

	//If we already have a normal move action, interrupt it.
	m_liAction[MOVE] = 0;
	m_MoveParam = CVec3Dfp32(_FP32_MAX);
};


//Set animation to execute and also starts animation. Animations therefore starts asynchronously.
//If animsequence is -1 then any currnet animation is stopped.
void CAI_ScriptHandler::SetAnimation(int _iAnim, bool _bTorsoAnim)
{
	MAUTOSTRIP(CAI_ScriptHandler_SetAnimation, MAUTOSTRIP_VOID);
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Stop any current animation
	if (m_iAnim != -1)
	{
		if (m_bTorsoAnim)
		{
			//Stop torso anim
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, 0, 1), m_pAI->m_pGameObject->m_iObject);
		}
		else
		{
			//Stop full anim
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, 0), m_pAI->m_pGameObject->m_iObject);
		};
		m_pAI->m_DeviceAnimation.Free();
	};
	
	m_iAnim = _iAnim;
	m_bTorsoAnim = _bTorsoAnim;
	m_bAnimStarted = false;

	//Start animation
	OnPerformAnimation();

	//Reset behaviour if we've got a valid animation
	/*
	if ((_iAnim != -1) &&
		m_pAI->m_spBehaviour)
		m_pAI->m_spBehaviour->Reset();
	*/

};


//Set action to take
void CAI_ScriptHandler::SetAction(int _iAction, int _iParam, const CVec3Dfp32& _VecParam)
{
	MAUTOSTRIP(CAI_ScriptHandler_SetAction, MAUTOSTRIP_VOID);
	m_pAI->ClearCombatBehaviours(0);
	m_pAI->ResetStuckInfo();
	switch (_iAction)
	{
	case TURRET_MOUNT:
		{	// Handle being mounted
			if (_iParam)
			{	// *** TODO: Make the heli mode entirely scripted for my sanity ***
				if (m_pAI->m_AH.SetActionParameter(CAI_Action::FLY_COMBAT,CAI_Action_FlyCombat::PARAM_MOUNT,_iParam))
				{
					m_pAI->m_pGameObject->ClientFlags() &= ~PLAYER_CLIENTFLAGS_NOMOVE;
					m_liAction[LOOK] = 0;
				}
				else
				{	// No helicopters are we, sayeth Yoda
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
					CWAG2I_Context Context(m_pAI->m_pGameObject,m_pAI->m_pServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pAI->m_pServer->GetGameTickTime()));
					CMat4Dfp32 Mat;
					CWObject_Message Msg(OBJSYSMSG_GETCAMERA);
					Msg.m_pData = &Mat;
					Msg.m_DataSize = sizeof(Mat);
					if (m_pAI->m_pServer->Message_SendToObject(Msg,_iParam))
					{
						CVec3Dfp32 MountPos = Mat.GetRow(3);
						CVec3Dfp32 OurPos = m_pAI->GetBasePos();
						if (MountPos[2] - OurPos[2] >= 24.0f)
						{	// High turret
							pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_GAMEPLAY_TURRET_STANDING),0);

						}
						else
						{	// Low turret
							pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_GAMEPLAY_TURRET_LOW),0);
						}
						// Setup for burst firing (this may be bad because if will affect our nonmounted firing)
						// *** Experimental
						m_pAI->m_DeviceWeapon.SetPressDuration(60);
						m_pAI->m_DeviceWeapon.SetPeriod(20);
						// ***
						m_liAction[LOOK] = TURRET_MOUNT;
						m_pAI->m_iMountParam = _iParam;
					}
					else
					{	// No such object
#ifndef M_RTM
						ConOut(CStr("TurretMount failed: Couldn't find object"));
#endif
						m_pAI->m_DeviceWeapon.SetPressDuration(m_pAI->m_PressDuration);
						m_pAI->m_DeviceWeapon.SetPeriod(m_pAI->m_ShootPeriod);
						m_liAction[LOOK] = 0;
						m_pAI->m_iMountParam = 0;
					}
				}
			}
			else
			{
				/* Not needed Olle?
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
				CWAG2I_Context Context(m_pAI->m_pGameObject,m_pAI->m_pServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pAI->m_pServer->GetGameTickTime()));
				pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,0),0);
				*/
				m_liAction[LOOK] = 0;
				m_pAI->m_iMountParam = 0;
			}
			return;
		};
	case CONTINUOUS_MOVE:
		{
			m_liAction[MOVE] = CONTINUOUS_MOVE;
			m_MoveParam = _VecParam;
			return;
		};

	case SINGLE_JUMP:
		{
			m_liAction[JUMP] = SINGLE_JUMP;
			return;
		};
	case SINGLE_SWITCH_WEAPON:
		{
			m_liAction[WEAPON] = SINGLE_SWITCH_WEAPON;
			m_iWeaponParam = _iParam;
			return;
		};
	case SINGLE_SWITCH_ITEM:
		{
			m_liAction[ITEM] = SINGLE_SWITCH_ITEM;
			m_iItemParam = _iParam;
			return;
		};
	case SINGLE_USE_WEAPON:
		{
			m_liAction[WEAPON] = SINGLE_USE_WEAPON;
			m_iWeaponParam = _iParam;
			return;
		};
	case SINGLE_USE_ITEM:
		{
			m_liAction[ITEM] = SINGLE_USE_ITEM;
			m_iItemParam = _iParam;
			return;
		};
	case MOVE_TO:
		{
			m_pAI->OnPreScriptAction(MOVE_TO,true);
			m_liAction[MOVE] = MOVE_TO;
			m_MoveParam = m_pAI->m_PathFinder.GetPathPosition(_VecParam, 3, 1);
			return;
		};
	case LOOK_AT:
		{
			if (m_pAI->m_iMountParam)
			{
				m_iTargetParam = _iParam;
			}
			else
			{
				if (_iParam)
				{
					m_liAction[LOOK] = LOOK_AT;
					m_iTargetParam = _iParam;
					m_LookTimeout = (m_pAI->GetAITick() + 2L * m_pAI->GetAITicksPerSecond());
				}
				else
				{
					m_liAction[LOOK] = 0;
					m_iTargetParam = 0;
					m_LookTimeout = 0;
				};
			}
			return;
		};
	case SNAP_LOOK_AT:
		{
			if (_iParam)
			{
				m_liAction[LOOK] = SNAP_LOOK_AT;
				m_iTargetParam = _iParam;
				m_LookTimeout = m_pAI->GetAITick() + m_pAI->GetAITicksPerSecond();
			}
			else
			{
				m_liAction[LOOK] = 0;
				m_iTargetParam = 0;
			};
			return;
		};
	case ATTACK:
		{
			if (m_pAI->m_iMountParam)
			{
				m_iTargetParam = _iParam;
			}
			else
			{
				if (_iParam)
				{
					m_pAI->OnPreScriptAction(ATTACK,true);
					m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
					m_liAction[LOOK] = ATTACK;
					m_liAction[WEAPON] = ATTACK;
					m_iTargetParam = _iParam;
				}
				else
				{
					m_pAI->OnPreScriptAction(ATTACK,false);
					m_liAction[LOOK] = 0;
					m_liAction[WEAPON] = 0;
					m_iTargetParam = 0;
				};
			}
			return;
		};
	case AIM:
		{
			if (m_pAI->m_iMountParam)
			{
				m_iTargetParam = _iParam;
			}
			else
			{
				if (_iParam)
				{
					m_pAI->OnPreScriptAction(ATTACK,true);
					m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
					m_liAction[LOOK] = AIM;
					m_iTargetParam = _iParam;
				}
				else
				{
					m_pAI->OnPreScriptAction(ATTACK,false);
					m_liAction[LOOK] = 0;
					m_iTargetParam = 0;
				}
			}
			return;
		};
	case AGGRESSIVE:
		{
			m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
			m_liAction[LOOK] = AGGRESSIVE;
			m_liAction[WEAPON] = AGGRESSIVE;
			m_iTargetParam = _iParam;
			m_iSecondaryTargetParam = 0;
		};
		return;
	case USE_DARKNESS:
		{
			m_DarknessUsage = (int8)_VecParam[2];
			if (m_DarknessUsage == CAI_Device_Darkness::USAGE_NONE)
			{
				if (m_DarknessPower != CAI_Device_Darkness::POWER_INVALID)
					StopUsingDarkness();
				else
					ClearUseDarkness();
			}
			else
			{
				m_liAction[DARKNESS] = USE_DARKNESS;

				// Activating a power also requires move and look device
				if (m_DarknessUsage & CAI_Device_Darkness::USAGE_ACTIVATE)
				{
					m_liAction[MOVE] = USE_DARKNESS;
					m_liAction[LOOK] = USE_DARKNESS;
				}

				m_DarknessPower = _iParam;
				m_DarknessUseTimer = 0;
				m_DarknessState = DARKNESSSTATE_INIT;
				m_DarknessTarget = (int32)_VecParam[0];
				m_DarknessSecondaryTarget = (int32)_VecParam[1];
			}
		};
		return;
	};
};

//Set action to take, string param only
void CAI_ScriptHandler::SetAction(int _iAction,CStr _Str)
{
	MAUTOSTRIP(CAI_ScriptHandler_SetAction_2, MAUTOSTRIP_VOID);
	switch (_iAction)
	{
	case MOVE_TO_SCENEPOINT:
		{
			//Find the scenepoint given by _pData
			CWO_ScenePointManager* pSPM = m_pAI->GetScenePointManager();
			if (pSPM)
			{
				CWO_ScenePoint* pSP = pSPM->Find(_Str);
				if (pSP)
				{
					// ***
					if (m_pAI->DebugTarget())
					{
#ifndef M_RTM
						CStr name = m_pAI->m_pGameObject->GetName();
						ConOut(CStr("MoveToScenepoint: " + name + " " + _Str));
#endif
					// ***
					}
					SetAction(_iAction,pSP);
				}
				else
				{
#ifndef M_RTM
					ConOut(CStr("MoveToScenepoint failed: Couldn't find "+_Str));
#endif	
				}
			}
			return;
		};
	}
};

//Set action to take, string param only
void CAI_ScriptHandler::SetAction(int _iAction, CWO_ScenePoint* _pSP)
{
	MAUTOSTRIP(CAI_ScriptHandler_SetAction_2, MAUTOSTRIP_VOID);
	switch (_iAction)
	{
	case MOVE_TO_SCENEPOINT:
		{
			if (_pSP)
			{
				if (_pSP->GetType() & CWO_ScenePoint::DOOR)
				{
					if ((m_liAction[MOVE] == MOVE_TO_SCENEPOINT)&&(m_pDoorScenePoint == _pSP))
					{
						return;
					}
					m_pDoorScenePoint = _pSP;
				}
				else
				{
					if (m_liAction[MOVE] == MOVE_TO_SCENEPOINT)
					{
						if (m_pScenePoint == _pSP)
						{
							return;
						}
						if (m_pScenePoint)
						{	// Do it after actions finish
							m_pNextScenePoint = _pSP;
							return;
						}
					}
					m_pScenePoint = _pSP;
				}
				m_iPath = 0;
				// MOVE_TO_SCENEPOINT should work with any scenepoint regardless of teams or users
				// therefore we have to temporarily stop this checking.
				bool bCheckValidTeams = m_pAI->m_AH.ShouldCheckForValidTeams();
				m_pAI->m_AH.SetCheckForValidTeams(false);
				if ((_pSP->IsSpawned())&&(m_pAI->m_AH.RequestScenePoint(_pSP,3)))
				{
					m_pAI->OnPreScriptAction(MOVE_TO_SCENEPOINT,true);
					m_pAI->StopBehaviour(CAI_Core::BEHAVIOUR_STOP_NORMAL,CAI_Action::PRIO_FORCED);
					m_pAI->ClearCombatBehaviours();
					m_liAction[MOVE] = MOVE_TO_SCENEPOINT;
					if (_pSP->GetSqrRadius() <= Sqr(8.0f))
					{
						m_ScenePointBlendTicks = MOVETO_SP_PP_BLENDTICKS;
					}
					else
					{
						m_ScenePointBlendTicks = 0; 
					}
				}
				else
				{
					m_pAI->OnPreScriptAction(MOVE_TO_SCENEPOINT,false);
					m_liAction[MOVE] = 0;
					m_ScenePointBlendTicks = 0;
				}
				m_pAI->m_AH.SetCheckForValidTeams(bCheckValidTeams);
				m_ScenePointStayTimeout = -1;
			}
			else
			{
				m_pAI->OnPreScriptAction(MOVE_TO_SCENEPOINT,false);
				m_liAction[MOVE] = 0;
				m_ScenePointBlendTicks = 0;
				m_ScenePointStayTimeout = -1;
				if (m_pDoorScenePoint)
				{
					m_pDoorScenePoint = NULL;
					if (m_pScenePoint)
					{
						m_liAction[MOVE] = MOVE_TO_SCENEPOINT;
					}
				}
			}
		}
		break;

	default:
		{	// More here?
		}
		break;
	}

};

//Set action to take, vector parameter only
void CAI_ScriptHandler::SetAction(int _iAction, const CVec3Dfp32& _VecParam)
{
	MAUTOSTRIP(CAI_ScriptHandler_SetAction_2, MAUTOSTRIP_VOID);
	SetAction(_iAction, -1, _VecParam);
};

//Hold/release melee attacks
void CAI_ScriptHandler::SetHoldAttacks(bool _bHold)
{
	MAUTOSTRIP(CAI_ScriptHandler_SetHoldAttacks, MAUTOSTRIP_VOID);
	m_pAI->m_DeviceWeapon.Pause(_bHold);
};

//Removes any current script parameters, effectively releasing script control
void CAI_ScriptHandler::Clear()
{
	MAUTOSTRIP(CAI_ScriptHandler_Clear, MAUTOSTRIP_VOID);
	if (!m_pAI || !m_pAI->IsValid())
		return;

	//Reset animation if necessary (Enclave stuff, don't think this is still in use)
	if (m_iAnim != -1)
	{
		if (m_bTorsoAnim)
		{
			//Stop torso anim
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, 0, 1), m_pAI->m_pGameObject->m_iObject);
		}
		else
		{
			//Stop full anim
			m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, 0), m_pAI->m_pGameObject->m_iObject);
		};
		m_pAI->m_DeviceAnimation.Free();
	};

	//Turn on gravity and animcontrol if necessary
	if (m_bTurnOnGravity  && !m_pAI->m_bWeightless)
	{
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 1), m_pAI->m_pGameObject->m_iObject);
	};
	if (m_bTurnOnAnimControl)
	{
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 1), m_pAI->m_pGameObject->m_iObject);
	};

	if ((m_pAI->m_iMountParam)&&(m_liAction[LOOK] == TURRET_MOUNT))
	{	// *** Should we send unmount to m_iMountParam? ***
		m_pAI->m_DeviceWeapon.SetPressDuration(m_pAI->m_PressDuration);
		m_pAI->m_DeviceWeapon.SetPeriod(m_pAI->m_ShootPeriod);
		m_pAI->m_iMountParam = 0;
	}

	m_pAI->m_DeviceWeapon.Pause(false);
	// Turn off PP and PO
	m_pAI->NoPerfectPlacement();
	//To avoid having the bot turn unexpectedly when released, set default heading to current heading
	m_pAI->m_HoldLookDir = m_pAI->GetLookDir();

	ClearParameters();
};


//Chooses the actions the bot should take at the current time.
void CAI_ScriptHandler::OnTakeAction()
{
	MAUTOSTRIP(CAI_ScriptHandler_OnTakeAction, MAUTOSTRIP_VOID);
	//Causes the AI to act according to the script
	//Any animations are performed first, with any normal actions after that if there

	//Running behaviour uses some devices
  	if (m_pAI->m_bBehaviourRunning)
	{
		//When behaviour is running, don't try to move but request any scene point we might have 
		//(since move to scenepoint action won't get called due to move device being busy)
		m_pAI->m_DeviceMove.Use();
		CWO_ScenePoint* pSP = m_pScenePoint;
		if (m_pDoorScenePoint)
		{
			pSP = m_pDoorScenePoint;
			m_pAI->m_bCollAvoidanceBlocked = true;
		}

		if ((pSP)&&(m_pAI->m_AH.RequestScenePoint(pSP, 3))&&(m_pAI->m_Timer <= m_ScenePointStayTimeout))
		{
#ifndef M_RTM
			m_pAI->m_AH.DebugDrawScenePoint(pSP,true);
#endif
			if ((pSP->GetBehaviour())&&(pSP->GetBehaviourDuration() <= 0.0f))
			{
				m_ScenePointStayTimeout = m_pAI->m_Timer + m_pAI->GetAITicksPerSecond();
			}
			else if (pSP->PlayOnce())
			{
				m_ScenePointStayTimeout = m_pAI->m_Timer + 1;
			}
			if ((pSP->GetTacFlags())&&(pSP == m_pAI->m_pBehaviourScenePoint))
			{
				m_liAction[MOVE] = 0;
				m_ScenePointStayTimeout = -1;
				m_ScenePointBlendTicks = 0;
				m_pDoorScenePoint = NULL;
				m_pScenePoint = NULL;
			}
		}
		else
		{
			if ((pSP)&&(pSP->GetBehaviour() == m_pAI->GetCurrentBehaviour()))
			{
				m_liAction[MOVE] = 0;
				m_ScenePointStayTimeout = -1;
				m_ScenePointBlendTicks = 0;
				m_pDoorScenePoint = NULL;
				if (m_pDoorScenePoint)
				{	
					m_pDoorScenePoint = NULL;
				}
				else
				{
					m_pScenePoint = NULL;
				}
			}
		}
	}

	//Animations doesn't use any devices, but one must be careful so that unappropriate 
	//actions aren't taken while performing an animation
	if (m_iAnim != -1)
		OnPerformAnimation();

	//Continuous look is higher prio than general script use
	if (m_pAI->m_iLookAtObject)
		m_pAI->OnTrackObj(m_pAI->m_iLookAtObject, 10, false, m_pAI->m_bLookSoft);

	// Delayed behaviour is higher prio than anything else
	if (m_iDelayedBehaviour)
	{
		OnDelayedBehaviour();
	}
	else
	{
		//The normal actions can use any device except the speak device.
		int i = MIN_ACTION;
		while (i < MAX_ACTION)
		{
			if (m_liAction[i])
			{
				OnPerformActions();
				i = MAX_ACTION;
			};
			i++;
		};

		//Check if we should be aggressive
		if ((m_pAI->m_UseFlags & CAI_Core::USE_ALWAYSAGGRESSIVE) &&
			(m_pAI->m_DeviceWeapon.IsAvailable()))
		{
			OnAggressive();		
		}

		//Path following must use the move device and uses the look device if it's still available 
		if (m_iPath)
		{
			if (m_iFollowMode & (FOLLOW_FORCE | FOLLOW_TELEPORT))
			{
				m_pAI->m_DeviceMove.Free();
			}
			OnFollowPath();
		}

		//Check if we've got an object to drag
		// *** Questionable here? ***
		if (m_pAI->m_iDragObject)
			m_pAI->DragObject(m_pAI->m_iDragObject);
	}

	// Did we get a new movetoscenepoint?
	if (m_pNextScenePoint)
	{
		m_pScenePoint = NULL;
		SetAction(MOVE_TO_SCENEPOINT,m_pNextScenePoint);
		m_pNextScenePoint = NULL;
	}
};

//Script controller is valid when it wants to control the AI
bool CAI_ScriptHandler::IsValid()
{
	MAUTOSTRIP(CAI_ScriptHandler_IsValid, false);
	if (!m_pAI || !m_pAI->IsValid())
		return false;

	if ((m_iAnim != -1) || m_iPath || (m_iMeleeManoeuvre != -1))
		return true;
	
	for(int i = MIN_ACTION; i < MAX_ACTION; i++)
		if (m_liAction[i])
			return true;

	if (m_iDelayedBehaviour)
	{	
		return true;
	}

	return false;		
};

void CAI_ScriptHandler::SetDelayedBehaviour(int _iBehaviour, int _iTarget, fp32 _Delay, CMat4Dfp32& _Mat)
{
	if (!m_iDelayedBehaviour)
	{
		m_iDelayedBehaviour = _iBehaviour;
		m_iDelayedBehaviourTarget = _iTarget;
		m_DelayedbehaviourMat = _Mat;
		m_BehaviourStartTimer = m_pAI->m_Timer + int32(_Delay * m_pAI->GetAITicksPerSecond());
	}
};

void CAI_ScriptHandler::OnDelayedBehaviour()
{
	if (m_iDelayedBehaviour)
	{
		if (m_iDelayedBehaviourTarget)
		{
			m_pAI->m_DeviceStance.SetTargetInFOV(true);
			m_pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
			m_pAI->OnTrackObj(m_iDelayedBehaviourTarget,10,false,false);
			m_pAI->m_DeviceWeapon.UsePeriodic();
		}
		if (m_pAI->m_Timer >= m_BehaviourStartTimer)
		{
			m_pAI->SetWantedBehaviour2(m_iDelayedBehaviour,CAI_Action::PRIO_FORCED,CAI_Core::BEHAVIOUR_FLAGS_PP,-1,NULL,&m_DelayedbehaviourMat);
			m_iDelayedBehaviour = 0;
			m_iDelayedBehaviourTarget = 0;
			m_DelayedbehaviourMat.Unit();
			m_BehaviourStartTimer = 0;
		}
	}
};

void CAI_ScriptHandler::SetPause(bool _bPause)
{
	if (_bPause)
	{
		m_liAction[MOVE] = CONTINUOUS_MOVE;
		m_MoveParam = CVec3Dfp32(0.0f,0.0f,0.0f);
	}
	else
	{
		m_liAction[MOVE] = 0;
	}
};

bool CAI_ScriptHandler::IsPaused()
{
	if ((m_liAction[MOVE] == CONTINUOUS_MOVE)&&(m_MoveParam != CVec3Dfp32(_FP32_MAX))&&(m_MoveParam.LengthSqr() <= 0.001f))
	{
		return(true);
	}
	return(false);
};

bool CAI_ScriptHandler::SetMountScenepoint(CWO_ScenePoint* _pScenePoint)
{
	if (m_liAction[MOVE] == MOVE_TO_SCENEPOINT)
	{	// Release MOVE_TO_SCENEPOINT m_pScenePoint (it may very well be the same scenepoint)
		m_liAction[MOVE] = 0;
		m_pScenePoint = NULL;
		m_ScenePointStayTimeout = -1;
		m_ScenePointBlendTicks = 0;
	}
	if ((_pScenePoint)&&(m_pAI->m_AH.RequestScenePoint(_pScenePoint,3)))
	{	// Should not need to check IsAt and IsAligned here as these must already be checked by caller
		m_pScenePoint = _pScenePoint;
		int StayTicks = RoundToInt(m_pScenePoint->GetBehaviourDuration() * m_pAI->GetAITicksPerSecond());
		m_ScenePointStayTimeout = m_pAI->m_Timer + StayTicks;
		if (m_pScenePoint->GetBehaviourDuration() <= 0.0f)
		{
			m_ScenePointStayTimeout = m_pAI->m_Timer + m_pAI->GetAITicksPerSecond();
		}
		m_ScenePointBlendTicks = 0;
		return(true);
	}

	return(false);
};


//The given action is performed immediately (and asynchronously), regardless of any normal restrictions
void CAI_ScriptHandler::ForcedAction(int _iAction, int _iParam)
{
	MAUTOSTRIP(CAI_ScriptHandler_ForcedAction, MAUTOSTRIP_VOID);
	//Deprecated, might not work

	if (!m_pAI || !m_pAI->IsValid())
		return;

	switch (_iAction)
	{
	case FORCE_USE_WEAPON:
		{
			CRPG_Object_Item * pWeapon = m_pAI->m_Weapon.GetWielded()->GetItem();
			if (pWeapon)
			{
				CMat4Dfp32 m;
				//CWObject_Message Msg(OBJMSG_AIQUERY_GETACTIVATEPOSITION);
				//Msg.m_pData = &m;
				//if (m_pAI->m_pServer->Message_SendToObject(Msg , m_pAI->m_pGameObject->m_iObject))
				m_pAI->m_pGameObject->AI_GetActivateMat(m);
				{
					pWeapon->Activate(m, NULL,m_pAI->m_pGameObject->m_iObject, 0);	
					pWeapon->m_iLastActivator = m_pAI->m_pGameObject->m_iObject;
					if (_iParam > 0)
					{
						m_pAI->m_DeviceWeapon.Free();
						m_pAI->m_DeviceWeapon.UseContinuous(_iParam);
					}
					else
						pWeapon->Deactivate(m, NULL, m_pAI->m_pGameObject->m_iObject, 0);
				}
			};
		};
		break;
	case FORCE_USE_ITEM:
		{
			CRPG_Object_Item * pItem = m_pAI->m_Item.GetWielded()->GetItem();
			if (pItem)
			{
				CMat4Dfp32 m;
				//CWObject_Message Msg(OBJMSG_AIQUERY_GETACTIVATEPOSITION);
				//Msg.m_pData = &m;
				//if (m_pAI->m_pServer->Message_SendToObject(Msg , m_pAI->m_pGameObject->m_iObject))
				m_pAI->m_pGameObject->AI_GetActivateMat(m);
				{
					pItem->Activate(m, NULL,m_pAI->m_pGameObject->m_iObject, 0);
					pItem->m_iLastActivator = m_pAI->m_pGameObject->m_iObject;
					if (_iParam > 0)
					{
						m_pAI->m_DeviceItem.Free();
						m_pAI->m_DeviceItem.UseContinuous(_iParam);
					}
					else
						pItem->Deactivate(m, NULL,m_pAI->m_pGameObject->m_iObject, 0);
				}
			};
		};
		break;
	case FORCE_SHOOT:
		{
			CWObject * pTarget = m_pAI->m_pServer->Object_Get(_iParam);
			CRPG_Object_Item * pWeapon = m_pAI->m_Weapon.GetWielded()->GetItem();
			if (pTarget && 
				pWeapon)
			{
				CMat4Dfp32 m;
				m_pAI->m_pGameObject->AI_GetActivateMat(m);
				{
					//Aim matrix at target 
					(pTarget->GetPosition() - m_pAI->m_pGameObject->GetPosition()).SetMatrixRow(m,0);
					
					pWeapon->Activate(m, NULL,m_pAI->m_pGameObject->m_iObject, 0);
					pWeapon->m_iLastActivator = m_pAI->m_pGameObject->m_iObject;
					m_pAI->m_DeviceWeapon.Free();
					m_pAI->m_DeviceWeapon.UseContinuous(10);
				}
			};
		};
		break;
	};
};


//Change AI user
void CAI_ScriptHandler::ReInit(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_ScriptHandler_ReInit, MAUTOSTRIP_VOID);
	m_pAI = _pAI;
};

void CAI_ScriptHandler::OnDeltaLoad(CCFile* _pFile)
{
	int8 Temp8; 
	int32 Temp32;
	_pFile->ReadLE(Temp8);
	if (Temp8 == 1)
	{
		_pFile->ReadLE(Temp32); m_iAnim = Temp32;
		_pFile->ReadLE(Temp8);  m_bTorsoAnim = (Temp8 != 0);
		_pFile->ReadLE(Temp8);  m_bAnimStarted = (Temp8 != 0);

		_pFile->ReadLE(Temp32); m_iPath = Temp32;
		_pFile->ReadLE(Temp32); m_iFollowMode = Temp32;
		_pFile->ReadLE(Temp8);  m_bFallenBehind = (Temp8 != 0);
		m_PathPosition.Read(_pFile);
		_pFile->ReadLE(Temp8); m_bFlying = (Temp8 != 0);
		_pFile->ReadLE(Temp8); m_bTurnOnGravity = (Temp8 != 0);
		_pFile->ReadLE(Temp8); m_bTurnOnAnimControl = (Temp8 != 0);
		m_MovePosRel.Read(_pFile);

		for(int i = MIN_ACTION; i < MAX_ACTION; i++)
		{
			_pFile->ReadLE(Temp32); m_liAction[i] = Temp32;
		}
		m_MoveParam.Read(_pFile);
		m_LookParam.Read(_pFile);
		_pFile->ReadLE(Temp32); m_iWeaponParam = Temp32;
		_pFile->ReadLE(Temp32); m_iItemParam = Temp32;
		_pFile->ReadLE(Temp32); m_iTargetParam = Temp32;
		_pFile->ReadLE(Temp32); m_iSecondaryTargetParam = Temp32;

		_pFile->ReadLE(Temp32); m_iMeleeManoeuvre = Temp32;
		_pFile->ReadLE(Temp32); m_iMeleeTarget = Temp32;
	};
};

void CAI_ScriptHandler::OnDeltaSave(CCFile* _pFile)
{
	int8 Temp8 = 1;
	_pFile->WriteLE(Temp8);

	int32 Temp32;
	Temp32 = m_iAnim; _pFile->WriteLE(Temp32);// = -1;
	Temp8 = m_bTorsoAnim; _pFile->WriteLE(Temp8);// = false;
	Temp8 = m_bAnimStarted; _pFile->WriteLE(Temp8);// = false;

	Temp32 = m_iPath; _pFile->WriteLE(Temp32);// = 0;
	Temp32 = m_iFollowMode; _pFile->WriteLE(Temp32);// = FOLLOW_NORMAL;
	Temp8 = m_bFallenBehind; _pFile->WriteLE(Temp8);// = false;
	m_PathPosition.Write(_pFile);// = CVec3Dfp32(_FP32_MAX);
	Temp8 = m_bFlying; _pFile->WriteLE(Temp8);// = false;
	Temp8 = m_bTurnOnGravity; _pFile->WriteLE(Temp8);// = false;
	Temp8 = m_bTurnOnAnimControl; _pFile->WriteLE(Temp8);// = false;
	m_MovePosRel.Write(_pFile);// = CVec3Dfp32(_FP32_MAX);

	for(int i = MIN_ACTION; i < MAX_ACTION; i++)
	{
		Temp32 = m_liAction[i];; _pFile->WriteLE(Temp32);// = 0;
	}
	m_MoveParam.Write(_pFile);// = CVec3Dfp32(_FP32_MAX);
	m_LookParam.Write(_pFile);// = CVec3Dfp32(_FP32_MAX);
	Temp32 = m_iWeaponParam; _pFile->WriteLE(Temp32);// = -1;
	Temp32 = m_iItemParam; _pFile->WriteLE(Temp32);// = -1;
	Temp32 = m_iTargetParam; _pFile->WriteLE(Temp32);// = 0;
	Temp32 = m_iSecondaryTargetParam; _pFile->WriteLE(Temp32);// = 0;

	Temp32 = m_iMeleeManoeuvre; _pFile->WriteLE(Temp32);// = -1;
	Temp32 = m_iMeleeTarget; _pFile->WriteLE(Temp32);// = 0;
};


#ifndef M_RTM
//Get some debug info about the bot
CStr CAI_ScriptHandler::GetDebugString()
{
	MAUTOSTRIP(CAI_ScriptHandler_GetDebugString, CStr());
	if (!IsValid())
		return CStr("");
	else
	{
		CStr str = " Scripting:";
		if ((m_iAnim)&&(m_iAnim != -1))
		{
			if (m_bTorsoAnim)
				str += " Torso";
			str += CStrF(" Animation index %d", m_iAnim);
		}
		if (m_iPath)
		{
			CStr Follow = "";
			if (m_iFollowMode &	FOLLOW_FORCE)
				Follow += " Force";
			if	(m_iFollowMode & FOLLOW_FLY)
				Follow += " Fly";
			if	(m_iFollowMode & FOLLOW_RELATIVE)
				Follow += " Relative";
			str += Follow + " Following path " + m_pAI->GetDebugName(m_iPath);
		};
		for (int i = 0; i < MAX_ACTION; i++)
		{
			switch (m_liAction[i])
			{
			case CONTINUOUS_MOVE:
				{
					if (VALID_POS(m_MoveParam))
					{
						if (m_MoveParam.LengthSqr() <= 0.001f)
						{
							str += " Paused";
						}
						else
						{
							str +=" Cont.Move " + m_MoveParam.GetString();
						}
					}
					else
					{
						str +=" Cont.Move bad param";
					}
				};
				break;
			case SINGLE_JUMP:
				{
					str += " Jump ";
				};
				break;
			case SINGLE_SWITCH_WEAPON:
				{
					CStr arg;
					if (m_iWeaponParam == -1)
						arg = " previous";
					else if (m_iWeaponParam == 0)
						arg = " next";
					else
						arg = CStrF("%d ", m_iWeaponParam);

					str += " Switch weapon" + arg;
				};
				break;
			case SINGLE_SWITCH_ITEM:
				{
					CStr arg;
					if (m_iItemParam == -1)
						arg = " previous";
					else if (m_iItemParam == 0)
						arg = " next";
					else
						arg = CStrF("%d ", m_iItemParam);

					str +=" Switch item" + arg;
				};
				break;
			case SINGLE_USE_WEAPON:
				{
					str +=" Use weapon";
				};
				break;
			case SINGLE_USE_ITEM:
				{
					str +=" Use item";
				};
				break;
			case MOVE_TO:
				{
					str +=" Move to " + m_MoveParam.GetString();
				};
				break;
			case MOVE_TO_SCENEPOINT:
				{
					str += " Move to scenepoint ";
					if (m_pDoorScenePoint)
					{
						str += "DoorSP";
					}
					else if (m_pScenePoint)
					{
						str += m_pScenePoint->GetName();
					}
				};
				break;
			case LOOK_AT:
				{
					if (m_pAI->m_bLookSoft)
					{
						str +=" SoftLook at" + m_pAI->GetDebugName(m_iTargetParam);
					}
					else
					{
						str +=" Look at" + m_pAI->GetDebugName(m_iTargetParam);
					}
				};
				break;
			case SNAP_LOOK_AT:
				{
					if (m_pAI->m_bLookSoft)
					{
						str +=" SoftSnaplook at" + m_pAI->GetDebugName(m_iTargetParam);
					}
					else
					{
						str +=" Snaplook at" + m_pAI->GetDebugName(m_iTargetParam);
					}
				};
				break;
			case ATTACK:
				{
					CWObject * pObj = m_pAI->m_pServer->Object_Get(m_iTargetParam);
					str +=" Attack ";
					if (pObj)
						str += m_pAI->GetDebugName(pObj);
				};
				break;
			case AIM:
				{
					str +=" Aim at" + m_pAI->GetDebugName(m_iTargetParam) + " ";
				};
				break;
			case AGGRESSIVE:
				{
					str +=" Aggressive";
				};
				break;
			};	
		};

		return str;
	};
};
#endif

