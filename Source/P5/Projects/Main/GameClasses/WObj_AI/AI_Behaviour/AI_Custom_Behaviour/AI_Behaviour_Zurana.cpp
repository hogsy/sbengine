#include "PCH.h"
#include "../../../WObj_Char.h"
#include "../../../WRPG/WRPGSpell.h"
#include "../../AICore.h"
#include "../../AI_Custom/AICore_Bosses.h"
#include "../../../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "AI_Behaviour_Custom.h"

//Initializes behaviour
CAI_Behaviour_Zurana::CAI_Behaviour_Zurana(CAI_Core * _pAI)
:CAI_Behaviour_Engage(_pAI)
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_ctor, MAUTOSTRIP_VOID);
	m_iType = ZURANA;
	m_iMode = NONE;
	m_iGrabMode = NONE;
	m_GrabTimer = 0;
	m_iLastMode = NONE;
	m_iObj1 = 0;
	m_iObj2 = 0;
	m_iObj3 = 0;
	m_PathPos = CVec3Dfp32(_FP32_MAX);
	m_HeadPos = CVec3Dfp32(_FP32_MAX);
	m_HandPos = CVec3Dfp32(_FP32_MAX);
	m_iSteamTimer = -1;
	m_iStunTimer = -1;
	m_StartPos = CVec3Dfp32(_FP32_MAX);
	m_iGrabee = 0;
	m_DamageTimer = 0;
	m_pAI->m_pGameObject->ClientFlags() |= CWO_CLIENTFLAGS_LINKINFINITE;
};



//Crouch at first object position and try to reach player. If successful start second (grab) engine path. 
//If hit by steam start third (recoil) engine path.
void CAI_Behaviour_Zurana::OnDuckNGrab()
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_OnDuckNGrab, MAUTOSTRIP_VOID);
	if (!m_pAI || !m_pAI->IsValid())
		return;

	if(m_GrabTimer > 0)
	{

		m_GrabTimer--;
	}
	else
	{
		m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, 0));
		m_iGrabMode = NONE;
	}

	//Make sure we try to grab in the right direction (fix proper)
	m_pAI->ForceAlignBody(0.1f);

	//Move if necessary
	bool bAtPos = true;
	CWObject * pPos = m_pAI->m_pServer->Object_Get(m_iObj1);
	if (pPos)
	{
		//Look in same direction as position object
		m_pAI->AddAimAtPositionCmd(m_pAI->GetAimPosition() + CVec3Dfp32::GetMatrixRow(pPos->GetPositionMatrix(), 0));

		//Set start position
		m_StartPos = pPos->GetPosition();

		if (m_PathPos == CVec3Dfp32(_FP32_MAX))
		{
			m_PathPos = m_pAI->GetPathPosition(pPos, true);
			if (m_PathPos == CVec3Dfp32(_FP32_MAX))
				m_PathPos = m_pAI->m_pGameObject->GetPosition();
		};

		if (m_pAI->m_pGameObject->GetPosition().DistanceSqr(m_PathPos) < 32*32)
		{
			//At position, so we don't need to move, but make sure we're aligned as well
			if (m_pAI->IsBodyAligned(0.02f))
				bAtPos = true;
			else
				m_pAI->OnIdle();
		}
		else
		{
			//Go to position
			bAtPos = false;
			m_pAI->OnMove(m_PathPos);
		};
	};
	if(m_iGrabMode == GRAB)
		m_iGrabee = CanGrab(m_HandPos);
	else
		m_iGrabee = 0;
	CWObject_Character * pGrabee = CWObject_Character::IsCharacter(m_iGrabee, m_pAI->m_pServer);

//	if(pGrabee)
//		m_pAI->OnLook(m_pAI->HeadingToPosition(m_pAI->GetAimPosition(m_pAI->m_pGameObject), pGrabee->GetPosition()),
//						0,
//						10);

	if (bAtPos)
	{
		//At position. Duck and grab
		SetAnimPositions();

		//Steam blast incoming? (i.e. triggered last frame)
		if (m_iSteamTimer == 1)
		{
			//Is head near the steam?
			if (m_HeadPos.DistanceSqr(m_SteamPos) < 105*105)
			{
					//In your face! Aargh!
					//Start recoil path...
					m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1), m_iObj3);
					//...and get hurt...
					m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_IMMUNE, 0));
					CWO_DamageMsg Msg(500, DAMAGE_FIRE);
					Msg.Send(m_pAI->m_pGameObject->m_iObject, 0, m_pAI->m_pServer);
					m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_IMMUNE, 1));
					OnStunned();
			}
			else
			{
				//Crouch and wait for it to clear
				if (m_pAI->m_DeviceStance.IsAvailable())
					m_pAI->m_DeviceStance.Use(CAI_Device_Stance::CROUCHING);
				m_pAI->OnIdle();
			};
		}
		//Steam cloud in the way?
		else if ((m_iSteamTimer > 1) && (m_iSteamTimer < 50))
		{
			//Crouch and wait for it to clear
			if (m_pAI->m_DeviceStance.IsAvailable())
				m_pAI->m_DeviceStance.Use(CAI_Device_Stance::CROUCHING);
			m_pAI->OnIdle();
		}
		//Check for successful grab
		else if (m_iGrabee)
		{
 			//Start grab path. if grabee is a player.
			if (pGrabee && !pGrabee->IsBot())
			{
				CVec3Dfp32 Dir = m_pAI->GetCenterPosition(pGrabee) - m_HandPos;
				Dir.Normalize();
				if(m_DamageTimer <= 0)
				{
					CVec3Dfp32 Temp = CVec3Dfp32(Dir[0], Dir[1], 0.2f) * 4;
					CWO_DamageMsg Msg(250, DAMAGE_CRUSH, &m_HandPos, &Temp);
					Msg.Send(pGrabee->m_iObject, 0, m_pAI->m_pServer);
					m_DamageTimer = 20;
				}
			}
			
		}
		//Should we start a grab move?
		else if (ShouldGrab())
		{
			m_iGrabMode = GRAB;
			//Duck'n'grab
			if (Random < 0.5f)
				m_GrabTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_GRAB1));
			else
				m_GrabTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_GRAB2));
			if (m_pAI->m_DeviceStance.IsAvailable())
				m_pAI->m_DeviceStance.Use(CAI_Device_Stance::CROUCHING);
			m_pAI->OnIdle();
		}
		else if(m_iGrabMode == TAUNTING)
		{
			CWObject_Character * pTauntee;
			for (int i = 0; i < m_pAI->m_lCharacters.Len(); i++)
			{
				pTauntee = m_pAI->IsEnemy(m_pAI->m_lCharacters[i].m_iObject);
				if (pTauntee &&
					(m_pAI->GetCenterPosition(pTauntee).DistanceSqr(m_HandPos) < 48*48))
				{
					CVec3Dfp32 Dir = m_pAI->GetCenterPosition(pTauntee) - m_HandPos;
					Dir.Normalize();
					CVec3Dfp32 Temp = (CVec3Dfp32(Dir[0], Dir[1], 0.2f) * 2);
					CWO_DamageMsg Msg(5, DAMAGE_CRUSH, &m_HandPos, &Temp);
					Msg.Send(pTauntee->m_iObject, 0, m_pAI->m_pServer);
				}
			};
		}
		else
		{
			//Just crouch. Should do some frustrated reaching and attacks as well
			if (m_GrabTimer == 0 &&
				(m_pAI->m_iTimer % 10 == 0) &&
				(Random < 0.2f))
			{
				m_iGrabMode = TAUNTING;
				if (Random < 0.5f)
					m_GrabTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_TAUNT_GRAB1));
				else
					m_GrabTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_TAUNT_GRAB2));
			}
			if (m_pAI->m_DeviceStance.IsAvailable())
				m_pAI->m_DeviceStance.Use(CAI_Device_Stance::CROUCHING);
			m_pAI->OnIdle();
		};
	};
};


//Player has reached safety. Rail a bit and destroy stuff and try to swat player 
//if he emerges from safety
void CAI_Behaviour_Zurana::OnPlayerSafe()
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_OnPlayerSafe, MAUTOSTRIP_VOID);
	if (m_pAI->m_pServer->Player_GetNum())
		m_pAI->OnTaunt(m_pAI->m_pServer->Player_GetObject(0));
	else
		m_pAI->OnIdle();
};


//Combat copy
void CAI_Behaviour_Zurana::OnFightCopy()
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_OnFightCopy, MAUTOSTRIP_VOID);
	if(m_pAI->m_Weapon.GetWielded()->GetItem()->m_iItemType != CAI_Core_Zurana::SLOT_HIGHSWAT)
		m_pAI->m_DeviceWeapon.Use(CAI_Device_Item::SWITCH, CAI_Core_Zurana::SLOT_HIGHSWAT);
	m_pAI->OnMessage(CWObject_Message(CAI_Core_Zurana::OBJMSG_SETLARGEOPPONENT, 1));
	CWObject *pTarget = m_pAI->GetSingleTarget(0,"$player",0);
	if(pTarget)
		m_iTarget = pTarget->m_iObject;
	CVec3Dfp32 AimPos = pTarget->GetPosition();
	AimPos[2] += 100;
	m_pAI->AddAimAtPositionCmd(AimPos);//m_pAI->GetFocusPosition(m_pTarget));

	CAI_Behaviour_Engage::OnTakeAction();
};


//Hack! Escape when fallen into lava
void CAI_Behaviour_Zurana::OnEscapeLava()
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_OnEscapeLava, MAUTOSTRIP_VOID);
	//Check if we're stuck and should jump (don't check first frame of lava escape)
	if (m_iMode != LAVAESCAPE)
	{
		m_iLastMode = m_iMode;
		m_iMode = LAVAESCAPE;
	}
	else if (m_pAI->m_pGameObject->GetPosition().DistanceSqr(m_pAI->m_pGameObject->GetLastPosition()) < 2*2)
	{
		//Stuck; jump!
		m_pAI->m_DeviceJump.Use();
		m_pAI->m_pServer->Object_AddVelocity(m_pAI->m_pGameObject->m_iObject, CVec3Dfp32(0,0,50));
	};

	//Move towards starting position
	m_pAI->AddMoveTowardsPositionCmd(m_StartPos);

	//Check every ten frames if we're out of the lava
	if ((m_pAI->m_iTimer % 10 == 0) &&
		!((m_pAI->m_pServer->Phys_GetMedium(m_pAI->m_pGameObject->GetPosition()) & XW_MEDIUM_LIQUID) ||
		  (m_pAI->m_pServer->Phys_GetMedium(m_pAI->m_pGameObject->GetPosition() + CVec3Dfp32(0,0,32)) & XW_MEDIUM_LIQUID)))

	{
		//No longer in lava!
		m_iMode = m_iLastMode;
	};
};


//Stumble around backwards until stun timer is up, when we revert to last behaviour mode
void CAI_Behaviour_Zurana::OnStunned()
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_OnStunned, MAUTOSTRIP_VOID);
	if (m_iMode != STUNNED)
	{
		m_iLastMode = m_iMode;
		m_iMode = STUNNED;
	}

	if (m_iStunTimer > 0)
	{
		m_iStunTimer--;
		
		//Just move backwards
		m_pAI->m_DeviceMove.Use(CVec3Dfp32(-1,0,0));
	}
	else
	{
		//Stop and revert to last mode
		m_pAI->OnIdle();
		m_iStunTimer = -1;
		m_iMode = m_iLastMode;
	}
};


//Victim has been grabbed. Eat him!.
void CAI_Behaviour_Zurana::OnGrabbed()
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_OnGrabbed, MAUTOSTRIP_VOID);
/*	//Get grab victim
	CWObject_Character * pGrabee = CWObject_Character::IsCharacter(m_iGrabee, m_pAI->m_pServer);
	if(m_GrabTimer > 0)
		m_GrabTimer--;

	if(m_iGrabMode == EATING)
		m_EatTimer++;

	if (m_iMode != GRABBED)
	{
		m_iLastMode = m_iMode;
		m_iMode = GRABBED;

		//If we have grab victim, make him an ai controlled, weightless ghost, so that he can be lifted
		if (pGrabee)
		{
			pGrabee->OnMessage(CWObject_Message(OBJMSG_CHAR_AICONTROL, 1));
			pGrabee->OnMessage(CWObject_Message(OBJMSG_SCRIPTIMPULSE, CAI_Core::IMPULSE_PAUSE));
			pGrabee->OnMessage(CWObject_Message(OBJMSG_CHAR_GHOST, 1));
			pGrabee->m_ClientFlags |= PLAYER_CLIENTFLAGS_NOGRAVITY;
		};
	};
	if(m_iGrabMode == GRAB1)
	{
//		ConOut("Grab 1 success");
		m_GrabTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_GRAB1_HIT));
		m_iGrabMode = GRAB1_SUCCESS;
	}
	else if(m_iGrabMode == GRAB2)
	{
//		ConOut("Grab 2 success");
		m_GrabTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_GRAB2_HIT));
		m_iGrabMode = GRAB2_SUCCESS;
	}
	
	//Let any animations be played out before starting eating anim
	if (m_GrabTimer == 0 && m_iGrabMode == EATING)
	{
		m_GrabTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_ROAR));
		m_iGrabMode = NONE;
	}
	else if (m_GrabTimer == 0)
	{
//		ConOut("Eating");
		m_GrabTimer = m_pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, ANIM_EAT));
		m_EatTimer = 0;
		m_iGrabMode = EATING;
	}
	

	//Stay put while eating
	m_pAI->OnIdle();

	if (pGrabee)
	{
		//Move grab victim along with hand and kill him when in mouth
		CVec3Dfp32 LastHandPos = m_HandPos;
		SetAnimPositions();
		if (LastHandPos == CVec3Dfp32(_FP32_MAX))
			LastHandPos = m_HandPos;

		//Can we eat grabee?
		if (m_EatTimer == 64)//m_pAI->GetCenterPosition(pGrabee).DistanceSqr(m_HeadPos) < 64*64)
		{
			CMat4Dfp32 pos = pGrabee->GetPositionMatrix();				
			m_HeadPos.SetMatrixRow(pos,3);
			pGrabee->SetPosition(pos);
			//Chomp!
			pGrabee->OnMessage(CWObject_Message(OBJMSG_PHYSICS_KILL,0,0, m_pAI->m_pGameObject->m_iObject));
			pGrabee->m_iModel[0] = 0;
			m_iMode = m_iLastMode;
		}
		else if (m_HandPos == CVec3Dfp32(_FP32_MAX))
		{
			//Invalid hand position; don't move grabee
			m_pAI->m_pServer->Object_SetVelocity(m_iGrabee, 0);		
		}
		else
		{
			//Move grabee along with hand 
//			m_pAI->m_pServer->Object_SetVelocity(m_iGrabee, m_HandPos - pGrabee->GetLastPosition());			//Predict hand position based on grabee's last position
//			m_pAI->m_pServer->Object_SetVelocity(m_iGrabee, m_HandPos - LastHandPos);
			CMat4Dfp32 pos = pGrabee->GetPositionMatrix();				
			m_HandPos.SetMatrixRow(pos,3);
			pGrabee->SetPosition(pos);
		};

	}
	else
	{
		//No victim. Revert to last mode
		m_iMode = m_iLastMode;
	};*/
};


//Set the head and right hand positions by evaluating anim
void CAI_Behaviour_Zurana::SetAnimPositions(CVec3Dfp32 _TempHeadOffset)
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_SetAnimPositions, MAUTOSTRIP_VOID);
	//Reset hand and head positions
	m_HandPos = CVec3Dfp32(_FP32_MAX);
	m_HeadPos = CVec3Dfp32(_FP32_MAX);
		
	//Get model
	CXR_Model * pModel = m_pAI->m_pServer->GetMapData()->GetResource_Model(m_pAI->m_pGameObject->m_iModel[0]);

	//Evaluate animation state to get current skeleton instance
	CXR_AnimState Anim;
	int extr = 2;
	CWObject_Message Msg(OBJSYSMSG_GETANIMSTATE);
	Msg.m_pData = &Anim;
	Msg.m_DataSize = sizeof(Anim);
	Msg.m_Param0 = *(int *)&extr;
	m_pAI->m_pGameObject->OnMessage(Msg);
	CXR_SkeletonInstance* pSkelInstance = Anim.m_pSkeletonInst;

	//If all is well, rotate hand and head along their respective rotation tracks and set positions
	if (pModel && pSkelInstance)
	{
		CXR_Skeleton * pSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
		if (pSkel)
		{
			if (pSkelInstance->m_lBoneTransform.ValidPos(PLAYER_ROTTRACK_RHAND))
			{
				m_HandPos = CVec3Dfp32(0,-120,170);
				m_HandPos *= pSkelInstance->m_lBoneTransform[PLAYER_ROTTRACK_RHAND];
			}

			if (pSkelInstance->m_lBoneTransform.ValidPos(PLAYER_ROTTRACK_HEAD))
			{
				m_HeadPos = CVec3Dfp32(40,0,380) + _TempHeadOffset; 
				m_HeadPos *= pSkelInstance->m_lBoneTransform[PLAYER_ROTTRACK_HEAD];
			}
		}
	}

	m_pAI->Debug_RenderWire(m_HandPos, m_HandPos + CVec3Dfp32(0,0,50));//DEBUG
	m_pAI->Debug_RenderWire(m_HeadPos, m_HeadPos + CVec3Dfp32(0,0,50));//DEBUG
};


//Check if we can grab a player, given our hand position, and return the index of 
//the grabbed victim if so
int CAI_Behaviour_Zurana::CanGrab(CVec3Dfp32 _HandPos)
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_CanGrab, 0);
	CWObject_Character * pGrabee;
	for (int i = 0; i < m_pAI->m_lCharacters.Len(); i++)
	{
		pGrabee	= m_pAI->IsEnemy(m_pAI->m_lCharacters[i].m_iObject);
		if (pGrabee &&
			(m_pAI->GetCenterPosition(pGrabee).DistanceSqr(_HandPos) < 52*52))
			return pGrabee->m_iObject;
	};
	return 0;
};


//Should we start a grab attack?
bool CAI_Behaviour_Zurana::ShouldGrab()
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_ShouldGrab, false);
	//We can only grab we are not currently grabbing
	if (m_iGrabMode != NONE)
		return false;

	//..and should only try if there's a target within reach
	CWObject_Character * pGrabee;
	for (int i = 0; i < m_pAI->m_lCharacters.Len(); i++)
	{
		pGrabee	= m_pAI->IsEnemy(m_pAI->m_lCharacters[i].m_iObject);
		if (pGrabee &&
			(m_pAI->m_pGameObject->GetPosition().DistanceSqr(pGrabee->GetPosition()) < 400*400))
		{
			return true;
		}
	};
	return false;
};


//Act according to current mode or just kill anything around
void CAI_Behaviour_Zurana::OnTakeAction()
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_OnTakeAction, MAUTOSTRIP_VOID);
	if (m_StartPos == CVec3Dfp32(_FP32_MAX))
		m_StartPos = m_pAI->m_pGameObject->GetPosition();

	if (m_iSteamTimer >= 0)
		m_iSteamTimer++;
	
	m_DamageTimer--;

	//Are we escaping from lava?
	if ((m_iMode == LAVAESCAPE) ||
		((m_pAI->m_iTimer % 10 == 0) && 
		 ((m_pAI->m_pServer->Phys_GetMedium(m_pAI->m_pGameObject->GetPosition()) & XW_MEDIUM_LIQUID) ||
		  (m_pAI->m_pServer->Phys_GetMedium(m_pAI->m_pGameObject->GetPosition() + CVec3Dfp32(0,0,32)) & XW_MEDIUM_LIQUID))))
	{
		OnEscapeLava();
	}
	else
	{
		switch (m_iMode)
		{
		case DUCK_N_GRAB:
			{
				OnDuckNGrab();
			};
			break;
		case PLAYER_SAFE:
			{
				OnPlayerSafe();
			};
			break;
		case FIGHTCOPY:
			{
				OnFightCopy();
			};
			break;
		case STUNNED:
			{
				OnStunned();
			};
			break;
		default:
			{
				CAI_Behaviour_Engage::OnTakeAction();
			};
		};
	}
};

//Handle special messages which influence the behaviour
bool CAI_Behaviour_Zurana::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_OnMessage, false);
	switch (_Msg.m_Msg)
	{
	case OBJMSG_SPECIAL_IMPULSE:
		{

			//Check if we should switch to the given behaviour mode (note that I don't support changing variables)
			int iMode = m_iMode;
			CStr str = (char *)_Msg.m_pData;
			CStr s;
			CWObject * pObj;
			if (str.CompareSubStr("DUCK_N_GRAB") == 0)
			{
				if (m_iMode != DUCK_N_GRAB)
				{
					iMode = DUCK_N_GRAB;
					//Trim message
					s = str.GetStrSep(";"); 
					//First object is position from which to perform the maneouvre
					s = str.GetStrSep(";"); 
					pObj = m_pAI->GetSingleTarget(0, s, 0);
					m_iObj1 = (pObj) ? pObj->m_iObject : 0;
					//Second object is path to start if grab is successful
					s = str.GetStrSep(";"); 
					pObj = m_pAI->GetSingleTarget(0, s, 0);
					m_iObj2 = (pObj) ? pObj->m_iObject : 0;
					//Third object is path to start if hit full in the face by steam
					s = str.GetStrSep(";"); 
					pObj = m_pAI->GetSingleTarget(0, s, 0);
					m_iObj3 = (pObj) ? pObj->m_iObject : 0;
				};
			}
			else if (str.CompareSubStr("STEAM") == 0)
			{
				//A steam blast is fired from the given object position
				//Trim message and get object
				s = str.GetStrSep(";"); 
				s = str.GetStrSep(";"); 
				CWObject * pMuzzle = m_pAI->GetSingleTarget(0, s, 0);
				if (pMuzzle)
				{
					//Start steam timer and set steam position and direction
					m_iSteamTimer = 0;
					m_SteamPos = CVec3Dfp32::GetRow(pMuzzle->GetPositionMatrix(), 3);
					m_SteamDir = CVec3Dfp32::GetRow(pMuzzle->GetPositionMatrix(), 0);
				};
			}
			else if (str.CompareSubStr("PLAYER_SAFE") == 0)
			{
				if (m_iMode != PLAYER_SAFE)
				{
					iMode = PLAYER_SAFE;
					//Trim message
					s = str.GetStrSep(";"); 
					//Objects are paths to choose between when acting out frustration
					s = str.GetStrSep(";"); 
					pObj = m_pAI->GetSingleTarget(0, s, 0);
					m_iObj1 = (pObj) ? pObj->m_iObject : 0;
					s = str.GetStrSep(";"); 
					pObj = m_pAI->GetSingleTarget(0, s, 0);
					m_iObj2 = (pObj) ? pObj->m_iObject : 0;
					s = str.GetStrSep(";"); 
					pObj = m_pAI->GetSingleTarget(0, s, 0);
					m_iObj3 = (pObj) ? pObj->m_iObject : 0;
				};
			}
			else if (str.CompareSubStr("FIGHTCOPY") == 0)
			{
				if (m_iMode != FIGHTCOPY)
				{
					iMode = FIGHTCOPY;
					//All objects are unused
					//Trim message
					s = str.GetStrSep(";"); 
					//Objects are paths to choose between when acting out frustration
					s = str.GetStrSep(";"); 
					pObj = m_pAI->GetSingleTarget(0, s, 0);
					m_iTarget = (pObj) ? pObj->m_iObject : 0;
					m_bTrueTarget = true;
					m_iTrueTarget = m_iTarget;
					int iCL = m_pAI->GetCharacterListIndex(m_iTarget);
					if (m_pAI->m_lCharacters.ValidPos(iCL))
						m_pAI->m_lCharacters[iCL].AddInfo(CAI_CharacterInfo::IS_SPOTTED, true);
					m_iObj1 = 0;
					m_iObj2 = 0;
					m_iObj3 = 0;
				};
			}
			else if (str.CompareSubStr("FADE_IN") == 0)
			{
				CAI_Core_Zurana *pZuranaAI = CAI_Core_Zurana::IsAIZurana(m_pAI);
		//		if(pZuranaAI)
		//			pZuranaAI->m_FadeInTimer = 0;
			};

			
			//If in stunned, grabbed or lava escape modes, change last mode instead of current
			if ((m_iMode == GRABBED) || 
				(m_iMode == STUNNED) ||
				(m_iMode == LAVAESCAPE))
			{
				m_iLastMode = iMode;
			}
			else if (iMode != m_iMode)
			{
				m_iMode = iMode;
			};
;

			return true;
		};
	default:
		return CAI_Behaviour::OnMessage(_Msg);
	};
};



//Always max prio
int CAI_Behaviour_Zurana::GetPathPrio(fp32 _CameraScore)
{
	MAUTOSTRIP(CAI_Behaviour_Zurana_GetPathPrio, 0);
	return 255;
};
