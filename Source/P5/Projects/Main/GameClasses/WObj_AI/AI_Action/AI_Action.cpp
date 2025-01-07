#include "PCH.h"
#include "AI_Action.h"
#include "../AICore.h"
#include "../AI_Custom/AICore_Darkling.h"
#include "../AI_ResourceHandler.h"
#include "../../WObj_Char.h"
#include "../../WObj_CharMsg.h"
#include "../../WObj_Game/WObj_GameMod.h" 
#include "../../WObj_Misc/WObj_Room.h"
#include "../../WObj_Misc/WObj_Turret.h"
#include "../../WObj_Misc/WObj_ScenePoint.h"


//Action estimation parameters/////////////////////////////////////////////////////////

const char * CAI_ActionEstimate::ms_lTranslateParam[] =
{
	"OFFENSIVE",  
	"DEFENSIVE",
	"EXPLORATION",	
	"LOYALTY",		
	"LAZINESS", 
	"STEALTH",
	"VARIETY",
};


#ifndef M_RTM
	#define DEBUG_PRINT_MELEE		1
	#define DEBUG_PRINT_ACTIONS		1
#endif

CAI_ActionEstimate::CAI_ActionEstimate()
{
	for (int i = 0; i < NUM_PARAMS; i++)
	{
		m_lValues[i] = 0;
	}
	m_bValid = false;
};

CAI_ActionEstimate::CAI_ActionEstimate(int nParam, ...)
{
	//Clear list
	int i;
	for (i = nParam; i < NUM_PARAMS; i++)
	{
		m_lValues[i] = 0;
	}
	m_bValid = false;

	//Get argument list
	va_list lArgs;
	va_start(lArgs,nParam);
	int Val;
	for (i = 0; i < Min(nParam, (int)NUM_PARAMS); i++)
	{
		#ifdef COMPILER_GNU
			Val = (int)va_arg(lArgs, int);
		#else
			Val = va_arg(lArgs, int);
		#endif
		m_lValues[i] = Val;
		m_bValid = true;
	}
	va_end(lArgs);
}


const int CAI_ActionEstimate::Get(int _iParam) const
{
	if ((_iParam >= 0) && (_iParam < NUM_PARAMS))
		return m_lValues[_iParam];
	else
		return 0;
};

int CAI_ActionEstimate::Set(int _iParam, int _Value)
{
	if ((_iParam >= 0) && (_iParam < NUM_PARAMS))
	{
		m_lValues[_iParam] = Max(-100, Min(100, _Value));
		m_bValid = true;
		return m_lValues[_iParam];
	}
	else
		return 0;
};

//Check validity
const bool CAI_ActionEstimate::IsValid() const
{
	return m_bValid;
}

//Max value
CAI_ActionEstimate::operator int() const
{
	int Res = 0;
	for (int i = 0; i < NUM_PARAMS; i++)
	{
		if (m_lValues[i] > Res)
			Res = m_lValues[i];
	}
	return Res;
};

//Factor value
int CAI_ActionEstimate::Mult(int _iParam, fp32 _Factor)
{
	if ((_iParam >= 0) && (_iParam < NUM_PARAMS))
	{
		m_lValues[_iParam] = Max(-100, Min(100, (int)(m_lValues[_iParam] * _Factor)));
		return m_lValues[_iParam];
	}
	else
		return 0;
};


//Action base class

const char* CAI_Action::ms_lStrID[] =
{
		"INVALID",

		//GENERAL ACTIONS
		//Auxiliary prio

		//Idle prio
		"IDLECALL",	
		"IDLE",
		"CHECKDEAD",
		"FOLLOW",
		"HOLD",
		"RESTRICT",
		"PATROLPATH",
		"ROAM",
		"FLYROAM",
		"LOOK",

		//Alert prio
		"SCAN",
		"ESCALATEODD",
		"ESCALATETHREAT",
		"INVESTIGATE",

		//Danger prio
		"KEEPDISTANCE",
		"WATCH",
		"THREATEN",
		"WARN",

		//Combat prio
		"CLOSE",
		"COMBAT",
		"COMBATLEADER",
		"COVER",
		"FLEE",
		"DARKLINGCLOSE",
		"MEATFACECLOSE",
		"DARKLINGJUMPCLOSE",
		"FLYCOMBAT",

		// Weapon actions (Combat prio)
		"DARKLINGATTACK",
		"ANGELUSATTACK",
		"OBJECTAIM",
		"WEAPONATTACKRANGED",
		"MELEEFIGHT",

		//Forced prio actions
		//Null terminator
		NULL,
};

//Constructor
CAI_Action::CAI_Action(CAI_ActionHandler * _pAH, int _Priority)
{
	m_pAH = _pAH;
	m_Priority = _Priority;
	if (m_Priority != PRIO_FORCED)
	{
		m_OverridePriority = (m_Priority + 0x40) & MASK_PRIO_ONLY;
	}
	else
	{
		m_OverridePriority = PRIO_MAX;
	}
	
	m_ExpirationTick = ET_EXPIRED;
	m_LastStartTick = ET_EXPIRED;
	m_LastStopTick = ET_EXPIRED;
	m_bTaken = false;
	m_bPaused = false;
	m_bRemoveOnRefresh = false;
	m_Type = INVALID;
	m_ID = 0;
	m_MinInterval = 0;
	m_iSpeech = SPEAK_DEFAULT;
	m_TypeFlags = 0;
};


//Syntactic sugar
CAI_Core* CAI_Action::AI()
{
	M_ASSERT(m_pAH,"No action handler! (Propably forgot to use base constructor in action constructor)");
	M_ASSERT(m_pAH->GetAI(),"No ai!");
	return m_pAH->GetAI();
}


//Set expiration delay if not previously set. Delay is _Time + _RndAdd * <Random 0..1> 
void CAI_Action::SetExpiration(int _Time, int _RndAdd)
{
	if (_Time == ET_INDEFINITE)
	{
		m_ExpirationTick = ET_INDEFINITE;
	}
	else if ((_Time == ET_EXPIRED) && (_RndAdd == 0))
	{
		m_ExpirationTick = ET_EXPIRED;
	}
	else if ((m_ExpirationTick == ET_INDEFINITE) || IsExpired())
	{
		m_ExpirationTick = (int)(AI()->GetAITick() + _Time + (_RndAdd * Random));
	}
};

void CAI_Action::SetMinExpirationDuration(int _Ticks)
{
	if ((_Ticks != ET_EXPIRED)&&(m_ExpirationTick != ET_INDEFINITE)&&(m_ExpirationTick < AI()->GetAITick() + _Ticks))
	{
		m_ExpirationTick = AI()->GetAITick() + _Ticks;
	}	
};

bool CAI_Action::ExpiresNextTick()
{
	if ((m_ExpirationTick != ET_INDEFINITE)&&
		(m_ExpirationTick != ET_EXPIRED)&&
		(m_ExpirationTick <= AI()->GetAITick()))
	{
		return(true);
	}
	else
	{
		return(false);
	}
}

// Returns true if the behaviour can be set
bool CAI_Action::SetWantedBehaviour(int _iBehaviour,int _Duration,CWO_ScenePoint* _pScenePoint)
{
	bool rValue = AI()->SetWantedBehaviour2(_iBehaviour,GetPriority(),0,_Duration,_pScenePoint);
	return(rValue);
};

void CAI_Action::OnTrackDir(CVec3Dfp32 _Dir, int _iObj, int _Time)
{
	AI()->OnTrackDir(_Dir, _iObj, _Time, false, (GetPriority() < PRIO_ALERT));
};
void CAI_Action::OnTrackPos(const CVec3Dfp32& _Pos, int _Time, bool _bHeadingOnly)
{
	AI()->OnTrackPos(_Pos, _Time, _bHeadingOnly, (GetPriority() < PRIO_ALERT));
};
void CAI_Action::OnTrackObj(int _iObj,int _Time, bool _bHeadingOnly)
{
	AI()->OnTrackObj(_iObj, _Time, _bHeadingOnly, (GetPriority() < PRIO_ALERT));
};
void CAI_Action::OnTrackAgent(CAI_AgentInfo* _pAgent,int _Time, bool _bHeadingOnly)
{
	AI()->OnTrackAgent(_pAgent, _Time, _bHeadingOnly, (GetPriority() < PRIO_ALERT));
};

int CAI_Action::SpeakRandom(int _iSpeech,fp32 _DelaySeconds)
{
	int iVariant = -1;
	CAI_Core* pAI = AI();

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
	if (pCD && pCD->m_DialogueInstance.IsValid())
	{
		return(-1);
	}

	if (m_iSpeech == SPEAK_DEFAULT)
	{
		CStr Reason;
#ifndef M_RTM
		Reason = GetDebugString();
#endif
		iVariant = pAI->UseRandom(Reason,_iSpeech,GetPriority());
		if ((_DelaySeconds > 0.0)&&(iVariant >= 0))
		{
			pAI->m_DeviceSound.PauseType(_iSpeech,_DelaySeconds);
		}
		return(iVariant);
	}
	else
	{
		return(-1);
	}
};

int CAI_Action::SpeakVariant(int _iSpeech,int _iVariant,bool _bRandomPause)
{
	int iVariant = -1;
	if (m_iSpeech == SPEAK_DEFAULT)
	{
		CStr Reason;
#ifndef M_RTM
		Reason = GetDebugString();
#endif
		iVariant = AI()->UseRandom(Reason,_iSpeech,GetPriority(),_iVariant);
		if (_bRandomPause)
		{
			AI()->m_DeviceSound.PauseType(_iSpeech,4.0f * Random);
		}
		return(iVariant);
	}
	else
	{
		return(-1);
	}
};

//Type accessor
int CAI_Action::GetType()
{
	return m_Type;
};

int CAI_Action::AddUnique(TArray<int16> _liBehaviours,int16 _iBehaviour)
{

//	bool bFoundDuplicate = false;
	for (int i = 0; i < _liBehaviours.Len(); i++)
	{
		if (_liBehaviours[i] == _iBehaviour)
		{
			return(0);
		}
	}
	_liBehaviours.Add(_iBehaviour);

	return(1);
};


int CAI_Action::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	return(0);
};

int CAI_Action::GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const
{
	return(0);
};

int CAI_Action::GetUsedAcs(TArray<int16>& _liAcs) const
{
	return(0);
};

bool CAI_Action::OnServiceBehaviour()
{
	return(false);
};

bool CAI_Action::AnimationEvent(int _iUser, int _iEvent)
{
	return(false);
};

void CAI_Action::RequestScenepoints()
{
	return;
};

//Create and return action of given type
spCAI_Action CAI_Action::CreateAction(int _Type, CAI_ActionHandler * _pAH)
{
	switch (_Type)
	{

	case MAKE_IDLE_CALL:
		return MNew1(CAI_Action_IdleCall, _pAH);
	case IDLE:
		return MNew1(CAI_Action_Idle, _pAH);
	case CHECKDEAD:
		return MNew1(CAI_Action_CheckDead, _pAH);
	case FOLLOW:
		return MNew1(CAI_Action_Follow, _pAH);
	case HOLD:
		return MNew1(CAI_Action_Hold, _pAH);
	case RESTRICT:
		return MNew1(CAI_Action_Restrict, _pAH);
	case PATROL_PATH:
		return MNew1(CAI_Action_PatrolPath, _pAH);
	case ROAM:
		return MNew1(CAI_Action_Roam, _pAH);
	case LOOK:
		return MNew1(CAI_Action_Look, _pAH);
		
	case SCAN:
		return MNew1(CAI_Action_Scan, _pAH);
	case ESCALATE_ODD:
		return MNew1(CAI_Action_EscalateOdd, _pAH);
	case ESCALATE_THREAT:
		return MNew1(CAI_Action_EscalateThreat, _pAH);
	case INVESTIGATE:
		return MNew1(CAI_Action_Investigate, _pAH);
	
	case KEEP_DISTANCE:
		return MNew1(CAI_Action_KeepDistance, _pAH);
	case WATCH:
		return MNew1(CAI_Action_Watch, _pAH);
	case THREATEN:
		return MNew1(CAI_Action_Threaten, _pAH);
	case WARN:
		return MNew1(CAI_Action_Warn, _pAH);

	case CLOSE:
		return MNew1(CAI_Action_Close, _pAH);
	case COMBAT:
		return MNew1(CAI_Action_Combat, _pAH);
	case COMBATLEADER:
		return MNew1(CAI_Action_CombatLeader, _pAH);
	case COVER:
		return MNew1(CAI_Action_Cover, _pAH);
	case FLEE:
		return MNew1(CAI_Action_Flee, _pAH);
	case DARKLING_CLOSE:
		return MNew1(CAI_Action_DarklingClose, _pAH);
	case MEATFACE_CLOSE:
		return MNew1(CAI_Action_MeatfaceClose, _pAH);
	case DARKLING_JUMPCLOSE:
		return MNew1(CAI_Action_DarklingJumpClose, _pAH);
	case FLY_COMBAT:
		return MNew1(CAI_Action_FlyCombat, _pAH);

	case DARKLING_ATTACK:
		return MNew1(CAI_Action_DarklingAttack, _pAH);
	case ANGELUS_ATTACK:
		return MNew1(CAI_Action_AngelusAttack, _pAH);
	case WEAPON_AIM:
		return MNew1(CAI_Action_WeaponAim, _pAH);
	case WEAPON_ATTACK_RANGED:
		return MNew1(CAI_Action_WeaponAttackRanged, _pAH);
	case MELEE_FIGHT:
		return MNew1(CAI_Action_MeleeFight, _pAH);

	default:
		return NULL;
	};
};


//Get corresponding type from string
int CAI_Action::StrToType(CStr _Str)
{
	int iAction = _Str.TranslateInt(ms_lStrID);
	if ((iAction < INVALID) || (iAction >= NUM_ACTIONS))
		return INVALID;
	else
		return iAction;
};


//Set action parameters. Param is enum defined in specific action.
//There are no default params to set of course
void CAI_Action::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_PRIO:
		m_Priority = Max((uint8)PRIO_MIN, Min((uint8)PRIO_MAX, (uint8)_Val));
		break;
	case PARAM_PRIOADD:
		{
			//Cannot increase priority beyond prio max or into next prio group
			if ((int)m_Priority + _Val > PRIO_MAX)
				m_Priority = PRIO_MAX;
			else
				m_Priority += Max(0, Min((int)MAX_PRIO_ADD, _Val));
		}
		break;
	case PARAM_ID:
		m_ID = _Val;
		break;

	case PARAM_SPEECH:
		if ((_Val >= SPEAK_MUTE)||(_Val < CAI_Device_Sound::MAX_SOUNDS))
		{
			m_iSpeech = _Val;
		}
		break;
	}
};

void CAI_Action::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_INTERVAL:
		m_MinInterval = (int32)(_Val * CAI_Core::AI_TICKS_PER_SECOND);
		m_LastStopTick = 0;	// SO the action CAN start right away
		break;
	}
};
void CAI_Action::SetParameter(int _Param, const CVec3Dfp32& _Val){};
void CAI_Action::SetParameter(int _Param, CStr _Val)
{
	switch (_Param)
	{
	case PARAM_PRIO:
		SetParameter(_Param, StrToPrio(_Val));
		break;
	case PARAM_PRIOADD:
		SetParameter(_Param, _Val.Val_int());
		break;
	case PARAM_ID:
		SetParameter(_Param, _Val.Val_int());
		break;
	case PARAM_INTERVAL:
		SetParameter(_Param, (fp32)_Val.Val_fp64());
		break;
	case PARAM_SPEECH:
		int Val = _Val.TranslateInt(CAI_Device_Sound::ms_lTranslateSpeech);
		if ((Val >= 0)&&(Val < CAI_Device_Sound::MAX_SOUNDS))
		{
			SetParameter(_Param,Val);
		}
		else
		{
			if (_Val.UpperCase() == "MUTE")
			{
				SetParameter(_Param,SPEAK_MUTE);
			}
			else if (_Val.UpperCase() == "DEFAULT")
			{
				SetParameter(_Param,SPEAK_DEFAULT);
			}
		}
		break;
	}
};


//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "PRIO")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_PRIO;
		return PARAM_PRIO;
	}
	else if (_Str == "PRIOADD")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_INT;
		return PARAM_PRIOADD;
	}
	else if (_Str == "ID")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_INT;
		return PARAM_ID;
	}
	else if (_Str == "MININTERVAL")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_INTERVAL;
	}
	else if (_Str == "SPEECH")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_SPEECH;
	}
	else
	{
		if (_pResType)
			*_pResType = PARAMTYPE_INVALID;
		return PARAM_INVALID;
	}
};


//Get action priority value for given string
int CAI_Action::StrToPrio(CStr _PrioStr)
{
	if (_PrioStr.CompareNoCase("IDLE") == 0)
	{
		return PRIO_IDLE;
	}
	else if (_PrioStr.CompareNoCase("AUXILIARY") == 0)
	{
		return PRIO_AUXILIARY;
	}
	else if (_PrioStr.CompareNoCase("ALERT") == 0)
	{
		return PRIO_ALERT;
	}
	else if (_PrioStr.CompareNoCase("DANGER") == 0)
	{
		return PRIO_DANGER;
	}
	else if (_PrioStr.CompareNoCase("COMBAT") == 0)
	{
		return PRIO_COMBAT;
	}
	else if (_PrioStr.CompareNoCase("FORCED") == 0)
	{
		return PRIO_FORCED;
	}
	else if (_PrioStr.CompareNoCase("MAX") == 0)
	{
		return PRIO_MAX;
	}
	else
	{
		return PRIO_MIN;
	}
}


//Get flags for given string and action type
int CAI_Action::StrToFlags(CStr _FlagsStr)
{
	return 0;
};



// GetPriority(false) returns the actual priority of the action as given in AIActions.xrg
// GetPriority(true) returns the prioclass of thaction when calling TestTakeActions
// This may be different because GetPriority(true) is only used to determine when an action
// potentially could disrupt a dialogue or behaviour. Note however that most actions do NOT
// override GetPriority() and thus returns the same irrespective of _bTest
// CAI_Action_CheckDead::GetPriority(true) returns PRIO_IDLE
// CAI_Action_Watch::GetPriority(true) returns PRIO_IDLE
int CAI_Action::GetPriority(bool _bTest)
{
	return m_Priority;
};



//The devices the action need to be valid and the devices the action will try to use
//(1 << <DEVICE1> | 1 << <DEVICE2> etc)
int CAI_Action::MandatoryDevices()
{
	return 0;	
};


//Check if the action can be taken or not
//It cannot be taken if:
//It is paused
//Its mandatory devices are not available
//It doesn't have an AI (BAD!!!)
//It has a nonzero m_MinInterval and its not yet time to run 
bool CAI_Action::IsValid()
{
	CAI_Core* pAI = AI();
	// Check paused and devices
	if (IsPaused() || !IsDeviceValid() || !pAI)
	{
		return(false);
	}
	else if (pAI->m_pServer && (pAI->GetAITick() < m_LastStopTick + m_MinInterval))
	{
		return(false);
	}
	else if (m_pAH->m_CurPrioClass >= m_OverridePriority)
	{
		return(false);
	}

	return(true);
};

bool CAI_Action::IsTaken()
{
	return m_bTaken;
};

//Only combat actions that really could attack _pTarget should return true here
bool CAI_Action::IsValid(CAI_AgentInfo* _pTarget)
{
	if ((_pTarget)&&(_pTarget->GetCurRelation() >= CAI_AgentInfo::ENEMY)&&(_pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED))
	{
		return(true);
	}
	else
	{
		return(false);
	}
}

//Check if the devices needed for the action is available
bool CAI_Action::IsDeviceValid()
{
	int iDevice = 0;
	int Devices = MandatoryDevices();
	if (!AI()) {return(false);}
	while ((Devices > 0) && (iDevice < CAI_Device::NUM_DEVICE))
	{
		if (Devices & 0x1)
		{
			if (AI()->m_lDevices[iDevice])
			{
				if (!(AI()->m_lDevices[iDevice]->IsAvailable()))
				{	//Unavailable mandatory device found!
					return false;
				}
			}
		}

		//Check next device
		iDevice++;
		Devices >>= 1;
	}

	//All devices available
	return true;
};

//Action estimate result, as calculated by personality will be modified by this factor 
//(0..1) based on how long the action have been taken, to encourage variety.
fp32 CAI_Action::VarietyReduction()
{
	//Check for how long we've been taking action
	if (AI() && AI()->m_pServer)
	{
		if ((m_LastStartTick > m_LastStopTick) ||
			(m_LastStopTick > AI()->GetAITick() - 2))
		{
			//Action is being taken, or has just recently stopped being taken 
			int Ticks = AI()->GetAITick() - m_LastStartTick + 1;

			//Reduction is based on the time action has been taken
			return Ticks / 400;
		}
		else 
		{
			//Action hasn't been taken for a while. Always full varietyfactor;
			return 0;
		}
	}
	return 0;
};


//Check if action has expired
bool CAI_Action::IsExpired()
{
	if (!AI() || !AI()->m_pServer)
		return true;

	if (m_ExpirationTick == ET_INDEFINITE)
		return false;
	else
		return (AI()->GetAITick() > m_ExpirationTick);
}; 


//Will be called whenever an ongoing action is started being taken
void CAI_Action::OnStart()
{
	CAI_Core* pAI = AI();
	if ((pAI)&&(pAI->m_pServer))
	{
		m_bTaken = true;
		//If we've been taking action within the previous few ticks, we don't count this as a new start
		if ((m_LastStartTick == ET_EXPIRED)||
			(pAI->GetAITick() - 1 > m_LastStopTick))
		{
			m_LastStartTick = pAI->GetAITick();
			if (m_iSpeech >= 0)
			{
				CStr Reason;
#ifndef M_RTM
				Reason = GetDebugString();
#endif
			
				pAI->UseRandom(Reason,m_iSpeech,GetPriority());
			}

		#ifdef DEBUG_PRINT_ACTIONS
			if (pAI->DebugTarget())
			{
				CStr name = CStr(pAI->m_pGameObject->GetName()) + ": ";
				CStr action = GetDebugString();
				ConOutL(name + action);
			}
		#endif
		}
	}
};

//Will be called whenever action is stopped being taken.
void CAI_Action::OnQuit()
{
	if (AI() && AI()->m_pServer)
	{
		m_bTaken = false;
		int Tick = AI()->GetAITick();
		if (m_LastStopTick <= Tick)
		{
			m_LastStopTick = AI()->GetAITick();
		}
	}
};

int CAI_Action::GetRunTicks()
{
	if (m_LastStartTick > 0)
	{
		return(AI()->GetAITick() - m_LastStartTick);
	}
	else
	{
		return(-1);
	}
};

void CAI_Action::ExpireWithDelay(int _Delay)
{
	SetExpirationExpired();
	m_LastStopTick = AI()->GetAITick() + _Delay;
};

//Change action handler we belong to
void CAI_Action::SetActionHandler(CAI_ActionHandler * _pAH)
{
	m_pAH = _pAH;
};

// Paused actions cannot be taken
void CAI_Action::SetPause(bool _bPause)
{
	m_bPaused = _bPause;
	if ((m_bTaken)&&(m_bPaused))
	{
		OnQuit();
	}
};

bool CAI_Action::IsPaused()
{
	return m_bPaused;
};

bool CAI_Action::CheckPrioClass()
{
	if (m_pAH->m_CurPrioClass >= m_OverridePriority)
	{
		SetExpiration(60);
		return(false);
	}
	else
	{
		return(true);
	}
};

//Should this action be paused by action override. Isn't that obvious?
bool CAI_Action::IsOverridableByActionOverride()
{
	//Override all idle or lower actions by default
	return (m_Priority < CAI_Action::PRIO_ALERT);
};

void CAI_Action::OnDeltaLoad(CCFile* _pFile)
{
	uint8 TempU8;
	int8 Temp8;
	int32 Temp32;
	_pFile->ReadLE(TempU8); m_Priority = TempU8;
	_pFile->ReadLE(TempU8); m_ID = TempU8;
	_pFile->ReadLE(Temp32); m_MinInterval = Temp32;
	_pFile->ReadLE(Temp8); m_iSpeech = Temp8;
};

void CAI_Action::OnDeltaSave(CCFile* _pFile)
{
	//Save all attributes that can be set via parameters
	uint8 TempU8;
	int8 Temp8;
	int32 Temp32;
	TempU8 = m_Priority; _pFile->WriteLE(TempU8);
	TempU8 = m_ID; _pFile->WriteLE(TempU8);
	Temp32 = m_MinInterval; _pFile->WriteLE(Temp32);
	Temp8 = m_iSpeech; _pFile->WriteLE(Temp8);
};


#ifndef M_RTM
//Debug info
CStr CAI_Action::GetDebugString()
{
	return CStr(CAI_Action::ms_lStrID[GetType()]);
};
#endif


	
//CAI_EstimatedAction/////////////////////////////////////////////////////////////////
CAI_EstimatedAction::CAI_EstimatedAction(spCAI_Action _pAction, int _Estimate)
{
	m_pAction = _pAction;
	m_Estimate = _Estimate;
};

int CAI_EstimatedAction::GetPriority()
{
	return m_Estimate;
};


//CHandleScenepoints
CHandleScenepoints::CHandleScenepoints()
{
	Clear(false);
};

CHandleScenepoints::~CHandleScenepoints()
{
	Clear(false);
};

void CHandleScenepoints::Clear(bool _bSaveLasts)
{
	m_StayTimeout = -1;
	INVALIDATE_POS(m_Destination);
	m_bBehaviourStarted = false;
	if (_bSaveLasts)
	{
		m_pScenePoint05 = m_pScenePoint04;
		m_pScenePoint04 = m_pScenePoint03;
		m_pScenePoint03 = m_pScenePoint02;
		m_pScenePoint02 = m_pScenePoint01;
		m_pScenePoint01 = m_pScenePoint;
	}
	m_pScenePoint = NULL;
};

//CAI_ActionHandler///////////////////////////////////////////////////////////////////

//Checks if any actions might be valid, according to the essential devices criteria
int CAI_ActionHandler::FiredAt(int* _piAttacker,uint32 _IgnoreDamageTypes,bool _bClear)
{
	int Ret = NONE;

	if (m_DamageType & _IgnoreDamageTypes)
	{
		return(Ret);
	}
	if (m_bWasInjured)
	{
		Ret = INJURED;
	}
	else if (m_bWasUninjured)
	{
		Ret = UNINJURED;
	}
	else if (m_bWasGrazed)
	{
		Ret = GRAZED;
	}

	if (Ret > NONE)
	{
		if (_piAttacker)
		{
			*_piAttacker = m_iAttacker;
		}
	}

	if (_bClear)
	{
		m_iAttacker = 0;
		m_DamageType = ~0;
		m_bWasInjured = false;
		m_bWasUninjured = false;
		m_bWasGrazed = false;

	}
	return(Ret);
}

bool CAI_ActionHandler::CheckEssentialDevices()
{
	//Check single essential devices
	int iDevice = 0;
	int Devices = m_EssentialDevices;
	while (Devices > 0)
	{
		if ((Devices & 0x1) &&
			(m_pAI->m_lDevices[iDevice]->IsAvailable()))
		{
			//Essential device available
			return true;
		}

		//Check next device
		Devices >>= 1;
		iDevice++;
	}

	//Check device combos
	for (int i = 0; i < m_lEssentialDeviceCombos.Length(); i++)
	{
		if (m_lEssentialDeviceCombos.IsValid(i))
		{
//			bool Valid = false;
			int iDevice = 0;
			int Devices = m_lEssentialDeviceCombos.Get(i);
			while (Devices > 0)
			{
				if ((Devices & 0x1) &&
					(m_pAI->m_lDevices[iDevice]->IsAvailable()))
				{
					//Essential device available
					return true;
				}

				//Check next device
				Devices >>= 1;
				iDevice++;
			}
		}
	}

	//No essential devices or devicecombos available.
	return false;
};


//Add action to handler, save a smartpointer if _keepFlag == true
void CAI_ActionHandler::AddAction(spCAI_Action _pAction, bool _keepFlag)
{
	if (_pAction)
	{
		//Add pointer to sorted list and smartpointer to container list 
		//The latter is necessary to keep action objects from getting automatically destroyed, but has no other use.
		m_lpActions.Add(_pAction);
		if (_keepFlag)
		{
			m_lspAs.Add(_pAction);
		}
		
		//If any mandatory devices already are included among the essential devices, then
		//essentialdevices and essential device combos need not be updated 
		if (!(m_EssentialDevices & _pAction->MandatoryDevices()))
		{
			bool bNew = true; 
			for (int i = 0; i < m_lEssentialDeviceCombos.Length(); i++)
			{
				if (m_lEssentialDeviceCombos.IsValid(i) &&
					(m_lEssentialDeviceCombos.Get(i) == _pAction->MandatoryDevices()))
				{
					bNew = false;
				}
			}

			if (bNew)
			{
				//Check if it is single device or not
				int nDevice = 0;
				int Devices = _pAction->MandatoryDevices();
				while ((Devices > 0) && (nDevice < 2))
				{
					if (Devices & 0x1)
						nDevice++;
					Devices >>= 1;
				}

				if (nDevice > 1)
				{
					m_lEssentialDeviceCombos.Add(_pAction->MandatoryDevices());
				}
				else
				{
					m_EssentialDevices |= _pAction->MandatoryDevices();

					//Remove any devicecombos that include this device
					for (int i = 0; i < m_lEssentialDeviceCombos.Length(); i++)
					{
						if (m_lEssentialDeviceCombos.IsValid(i) &&
							(m_lEssentialDeviceCombos.Get(i) & m_EssentialDevices))
						{
							m_lEssentialDeviceCombos.Remove(i);
						}
					}
				}
			}
		}
	}
};

// Removes the given action from all members except m_lspActionOverrides and m_lspAs
// Returns true if it succeeded and false if not
bool CAI_ActionHandler::RemoveAction(spCAI_Action _pAction)
{
	bool result = false;
	if (_pAction)
	{
		// Remove _pAction from m_lpActions
		int iRemove = m_lpActions.Find(_pAction);
		if (iRemove >= 0)
		{
			m_lpActions.Remove(iRemove);
			result = true;
		}
	}

	return(result);
}


//Create action from given definition
spCAI_Action CAI_ActionHandler::CreateAction(CAI_ActionDefinition * _pActionDef)
{
	if (!_pActionDef)
		return NULL;

	spCAI_Action spAction = CAI_Action::CreateAction(_pActionDef->GetType(), this);
	if (spAction)
	{
		//Set parameters according to definition
		TArray<int> lParams;
		TArray<int> lTypes;
 		int nParam = _pActionDef->GetParamIDs(&lParams, &lTypes);
		for (int i = 0; i < nParam; i++)
		{
			switch (lTypes[i])
			{
			case CAI_Action::PARAMTYPE_INT:
			case CAI_Action::PARAMTYPE_PRIO:
			case CAI_Action::PARAMTYPE_FLAGS:
				{
					int Val;
					if (_pActionDef->GetParam(lParams[i], Val))
						spAction->SetParameter(lParams[i], Val);
				}
				break;
			case CAI_Action::PARAMTYPE_FLOAT:
				{
					fp32 Val;
					if (_pActionDef->GetParam(lParams[i], Val))
						spAction->SetParameter(lParams[i], Val);
				}
				break;
			case CAI_Action::PARAMTYPE_VEC:
				{
					CVec3Dfp32 Val;
					if (_pActionDef->GetParam(lParams[i], Val))
						spAction->SetParameter(lParams[i], Val);
				}
				break;
			case CAI_Action::PARAMTYPE_TARGET:
			case CAI_Action::PARAMTYPE_SPECIAL:
				{
					CStr Val;
					if (_pActionDef->GetParam(lParams[i], Val))
						spAction->SetParameter(lParams[i], Val);
				}
				break;
			}
		}
	}

	return spAction;
}


CAI_ActionHandler::CAI_ActionHandler()
{
	Init();
};


void CAI_ActionHandler::Init(bool _bClearRegs)
{
	m_pAI = NULL;
	m_lpActions.Create(NULL, 64);
	m_lspAs.Clear();
	m_lspAs.SetGrow(64);
	m_CurPrioClass = CAI_Action::PRIO_MIN;
	m_nActiveActions = 0;

	m_EssentialDevices = 0;
	m_lEssentialDeviceCombos.Create(0);
	m_ReportedEvents = 0;
	m_iLeader = 0;
	m_LeaderFindingTick = 0;
	m_iFollower = 0;
	m_FollowerFindingTick = 0;
	m_iTarget = 0;
	m_iTarget2 = 0;
	m_RetargetTimeout = 0;
	INVALIDATE_POS(m_RetargetPos);
	INVALIDATE_POS(m_RetargetPosPF);
	m_RetargetUp = CVec3Dfp32(0.0f,0.0f,1.0f);
	m_pRetargetSP = NULL;

	m_iHostile = 0;
	m_iHostile2 = 0;
	m_bFindNewTarget = false;
	m_iFriend = 0;
	m_iFriend2 = 0;
	m_iInvestigate = 0;
	m_iInvestigate2 = 0;
	m_iInvestigateObj = 0;
	m_pInterestingScenepoint = NULL;
	m_InterestingScenepointTimeout = 0;

	m_lpScenePoints.Clear();
	m_ScenePointTypes = 0;
	m_NextScenePointTick = 0;
	m_LastGetBestSPTimer = -AH_GETBESTSCENEPOINT_PERIOD;
	m_lpLastHeldScenePoints.Create(NULL);
	m_lpCurrentHeldScenePoints.Create(NULL);
	m_bCheckValidTeams = false;

	m_pOverrideScenePoint = NULL;
	m_bClearOverrideScenePoint = false;

	m_iAttacker = 0;
	m_DamageType = ~0;
	m_bWasInjured = false;
	m_bWasUninjured = false;
	m_bWasGrazed = false;

	m_FoundCorpseTick = -1;
	m_ExitCombatTick = -1;

	m_bWeaponHandling = false;
	m_lWeaponHandlerExcludes.Clear();
	if (_bClearRegs)
	{
		m_lspActionRegs.Clear();
	}
	m_lspActionOverrides.Clear();
	m_spRestriction = NULL;


	for (int i = 0; i < AH_SP_HISTORY_COUNT; i++)
	{
		m_lScenepointHistory[i] = NULL;
		m_lLookpointHistory[i] = NULL;
	}
	
	m_iEnemyOverride = 0;
};

void CAI_ActionHandler::ReInit(CAI_Core* _pAI)
{
	Init(false);	// *** Don't clear the regs
	SetAI(_pAI);
};

//Switch ai user
void CAI_ActionHandler::SetAI(CAI_Core* _pAI)
{
	m_pAI = _pAI;
};


//Using AI
CAI_Core* CAI_ActionHandler::GetAI()
{
	return m_pAI;
};

//Create and add any actions that have been defined by registries. This should be done on first
//refresh to make sure world has been spawned.
void CAI_ActionHandler::OnSpawn()
{
	if (!m_pAI)
		return;

	m_CurPrioClass = CAI_Action::PRIO_MIN;
	{
		//Add actions from registry
		for (int i = 0; i < m_lspActionRegs.Len(); i++)
		{
			CRegistry *	pKey = m_lspActionRegs[i];

			//Action template name is either classname value (if any) or given name
			CStr Name;
			const CRegistry * pChild = pKey->FindChild("CLASSNAME");
			if (pChild)
			{
				//Found classname; this denotes inheritance so use this as template name instead of given name.
				Name = pChild->GetThisValue(); 
			}
			else
			{
				//Use given name
				Name = pKey->GetThisName();
			}
			
			CStr ID = pKey->GetThisValue();
//			const char* n = Name.Str();//DEBUG

			//Get AI resource handler and action definition of action (if any)
			CAI_ResourceHandler * pAIRes = m_pAI->GetAIResources();
			CAI_ActionDefinition * pActionDef = (pAIRes) ? pAIRes->GetActionDefinition(Name) : NULL;

			//Create action
			spCAI_Action spAction;
			if (pActionDef)
			{
				spAction = CreateAction(pActionDef);
			}
			else
			{
				//No action definition, try to create action from name
				spAction = CAI_Action::CreateAction(CAI_Action::StrToType(Name), this);
			}

			//Action hopefully created and any default params from definition set.
			if (spAction)
			{	// USer may have added an ID other than the default implicit 0
				if (ID.Len() > 0)
				{
					spAction->SetParameter(CAI_Action::PARAM_ID,ID);
				}
				//Parse any keys to overwrite defaults
				int Param;
				for (int i = 0; i < pKey->GetNumChildren(); i++)
				{
					Param = spAction->StrToParam(pKey->GetName(i));
					/* *** Use when action evaluation seems buggy! ***
					CStr KeyName = pKey->GetName(i);
					CStr KeyValue = pKey->GetValue(i);
					if (Param == CAI_Action_Idle::PARAM_BEHAVIOURS)
					{
						static count = 0;
						pKey->XRG_Write(CStrF("EvaluatedActionReg%d.xrg", count));
						count++;
					}
					*/
					if (Param != CAI_Action::PARAM_INVALID)
						spAction->SetParameter(Param, pKey->GetValue(i));
				}

				//Add action
				AddAction(spAction);
			}
		}
	}

	//Remove keys
	m_lspActionRegs.Destroy();
};

int CAI_ActionHandler::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int nAdded = 0;

	for (int i = 0; i < m_lspAs.Len(); i++)
	{
		nAdded += m_lspAs[i]->GetUsedBehaviours(_liBehaviours);
	}
	 
	return(nAdded);
};

int CAI_ActionHandler::GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const
{
	int nAdded = 0;

	for (int i = 0; i < m_lspAs.Len(); i++)
	{
		nAdded += m_lspAs[i]->GetUsedGesturesAndMoves(_liGestures,_liMoves);
	}
	 
	return(nAdded);
}

int CAI_ActionHandler::GetUsedAcs(TArray<int16>& _liAcs) const
{
	int nAdded = 0;

	for (int i = 0; i < m_lspAs.Len(); i++)
	{
		nAdded += m_lspAs[i]->GetUsedAcs(_liAcs);
	}

	return(nAdded);
}

//Returns true if actionhandler thinks its OK to deactivate the bot
//What must be fulfilled to be ready for deactivation?
//m_CurPrioClass <= PRIO_IDLE
//m_lspActionOverrides can only contain Hold
//Current position may not be restricted
bool CAI_ActionHandler::CanDeactivate()
{
	if (m_CurPrioClass > CAI_Action::PRIO_IDLE)
	{
		return(false);
	}
	for (int i = 0; i < m_lspActionOverrides.Len(); i++)
	{
		if (m_lspActionOverrides[i]->m_Type != CAI_Action::HOLD)
		{
			return(false);
		}
	}
	if ((GetAI())&&(m_spRestriction)&&(m_spRestriction->IsRestricted(GetAI()->GetBasePos())))
	{
		return(false);
	}

	return(true);
}

// Kills all actions referred to by KillActions
int CAI_ActionHandler::HandleActionKills()
{
	int KilledActions = 0;

	if (m_lKillTypes.Len() == 0)
	{
		return(KilledActions);
	}

	// Remove all taken actions 
	Override(CAI_Action::PRIO_MAX);

	int i,j;
	for (i = m_lKillTypes.Len()-1; i >= 0; i--)
	{
		for (j = m_lpActions.Length()-1; j >= 0; j--)
		{
			CAI_Action* pCur = m_lpActions.Get(j);
			if (pCur)
			{
				if ((pCur->m_Type == m_lKillTypes[i])&&(pCur->m_ID == m_lKillIDs[i]))
				{
					m_lpActions.Remove(j);
					KilledActions++;
				}
			}
			else
			{	// pCur == NULL
				m_lpActions.Remove(j);
			}
			
		}
	}

	m_lKillTypes.Clear();
	m_lKillIDs.Clear();

	return(KilledActions);
}

void CAI_ActionHandler::ClearActionOverrides(bool _bClearEnemyOverride)
{
	// Remove all taken actions 
	Override(CAI_Action::PRIO_MAX);

	// Remove any action(s) previously added by SetActionOverride
	for (int i = m_lspActionOverrides.Len()-1; i >= 0; i--)
	{
//		CAI_Action* pAction = m_lspActionOverrides[i];
		m_lspActionOverrides[i]->m_bRemoveOnRefresh = true;
		m_lspActionOverrides[i]->SetExpirationExpired();
	}

	// For action overrides that set m_iEnemyOverride we must also clear m_iEnemyOverridewhen clearing the action
	if ((_bClearEnemyOverride)&&(m_iEnemyOverride))
	{
		m_iEnemyOverride = 0;
	}
}

// Add _pAction to action overrides, removing any currently taken actions
// and locking all IDLE or less actions
void CAI_ActionHandler::AddActionOverride(CAI_Action* _pAction,int _Prio)
{
	// Remove all taken actions
	Override(CAI_Action::PRIO_MAX);

	if (_pAction)
	{
		m_lspActionOverrides.Add(_pAction);
		// Add the action (the false parameter tells us not to save a smart pointer to the action)
		_pAction->m_bRemoveOnRefresh = false;
		AddAction(_pAction,false);
	}
}

// SetActionOverride creates an action that will be evaluated with the other actions
bool CAI_ActionHandler::SetActionOverride(int _iAction, CStr _sParamName, fp32 _DistParam, fp32 _MaxDistParam, int _iActivator)
{
	if ((!m_pAI)||(!m_pAI->IsValid()))
	{
		return(false);
	}

	//When we set actionoverride, we always set behaviour from key param, so that this can be saved
	m_pAI->m_ActionFromKey = _iAction;

	switch(_iAction)
	{
	case EXPLORE:
		// Explore stops the current action override 
		ClearActionOverrides();
		// We return true despite there is no actual action CAI_Behaviour::EXPLORE
		return(true);
		break;

	case PATROL:
		{
			ClearActionOverrides();
			//Find random object with the given name (this should be a server index to an 
			//engine path unless someone has fucked up)
			CWObject* pObj = m_pAI->GetSingleTarget(_iActivator, _sParamName, m_pAI->m_pGameObject->m_iObject);
			if (pObj) 
			{	//Found patrol path object
				spCAI_Action_PatrolPath spAct = MNew1(CAI_Action_PatrolPath, this);
				spAct->SetParameter(CAI_Action_PatrolPath::PARAM_PATH,pObj->m_iObject);
				AddActionOverride(spAct);
				return(true);
			}
			else
			{
				return(false);
			}
		}
		break;

	case HOLD: 
		{	
			ClearActionOverrides();
			// Hold action
			spCAI_Action_Hold spAct = MNew1(CAI_Action_Hold, this);
			spAct->SetActionOverride(_iAction, _sParamName, _DistParam, _MaxDistParam, _iActivator);
			AddActionOverride(spAct);
			return(true);
		}
		break;

	case ENGAGE:
		{	// Remove all taken actions, turn off all IDLE actions
			ClearActionOverrides();
			CWObject* pObj = m_pAI->GetSingleTarget(_iActivator, _sParamName, m_pAI->m_pGameObject->m_iObject);
			if (pObj)
			{	// Supplied target was valid
				m_iEnemyOverride = pObj->iObject();
				CAI_AgentInfo* pInfo = m_pAI->m_KB.GetAgentInfo(m_iEnemyOverride);
				if (!pInfo)
				{
					pInfo = m_pAI->m_KB.AddAgent(m_iEnemyOverride,0,CAI_AgentInfo::DETECTED,CAI_AgentInfo::NONE,CAI_AgentInfo::ENEMY);
				}
				if ((pInfo)&&(m_pAI->IsValidAgent(pInfo->GetObject())))
				{
					pInfo->SetRelation(CAI_AgentInfo::ENEMY);
				}
			}
			else
			{	// Supplied target was invalid
				m_iEnemyOverride = 0;
			}
			AddActionOverride(NULL);
			return(true);
		}
		break;

	case -1:
		// When no menu was set in Ogier -1 is returned which we treat as EXPLORE
		ClearActionOverrides();
		// We return true despite there is no actual action CAI_Behaviour::EXPLORE
		return(true);
		break;

	default:
		ConOut("Unknown action override requested");
		return(false);
		break;
	}


	return(false);
}

bool CAI_ActionHandler::SetActionOverride(int _iBehaviour,TArray<CNameValue>& _Params)
{
	switch(_iBehaviour)
	{
	case CAI_Core::HOLD:
		{
			ClearActionOverrides();
			// Create CAI_Action_Run_Behaviour object
			spCAI_Action_Hold spAct = MNew1(CAI_Action_Hold, this);
			for (int i = 0; i < _Params.Len(); i++)
			{
				CStr NameStr = _Params[i].m_Name;
				CStr ValueStr = _Params[i].m_Value;
				if (NameStr == "AI_POSITION")
				{
					spAct->SetParameter(CAI_Action_Hold::PARAM_POSOBJECT,ValueStr);
				}
			}
			AddActionOverride(spAct);
			return(true);
		}
		break;

	case CAI_Core::PATROL:
		{
			ClearActionOverrides();
			// Create CAI_Action_PatrolPath object
			spCAI_Action_PatrolPath spAct = MNew1(CAI_Action_PatrolPath, this);
			for (int i = 0; i < _Params.Len(); i++)
			{
				CStr NameStr = _Params[i].m_Name;
				CStr ValueStr = _Params[i].m_Value;
				if (NameStr == "AI_PATH")
				{
					spAct->SetParameter(CAI_Action_PatrolPath::PARAM_PATH,ValueStr);
				}
			}
			AddActionOverride(spAct);
			return(true);
		}
		break;

	default:
		return(false);
	}

	return(false);
}

void CAI_ActionHandler::SetEnemyOverride(int _iEnemyOverride)
{
	CWObject* pObj = m_pAI->m_pServer->Object_Get(_iEnemyOverride);
	if (pObj)
	{
		m_iEnemyOverride = _iEnemyOverride;
		CAI_AgentInfo* pInfo = m_pAI->m_KB.GetAgentInfo(m_iEnemyOverride);
		if (!pInfo)
		{
			pInfo = m_pAI->m_KB.AddAgent(m_iEnemyOverride,0,CAI_AgentInfo::DETECTED,CAI_AgentInfo::NONE,CAI_AgentInfo::ENEMY);
		}
		if ((pInfo)&&(m_pAI->IsValidAgent(pInfo->GetObject())))
		{
			pInfo->SetRelation(CAI_AgentInfo::ENEMY);
		}
	}
};

// Returns true if an action of higher prio class than the prio class of _Prio could be taken
// The optional _pPrio returns the actions actual prio.
// Use the method to check weather higher prio actions should stop dialogue or behaviours
// Note: TestTakeActions() compares prio CLASS only; if _Prio is PRIO_COMBAT and there's an action
// with prio PRIO_COMBAT+1 TestTakeActions will still returns false.
bool CAI_ActionHandler::TestTakeActions(int32 _Prio,int32* _pPrio)
{
	MSCOPESHORT(CAI_ActionHandler::TestTakeActions);

	int PrioClassToBeat = _Prio & CAI_Action::MASK_PRIO_ONLY;
//	int rPrio = 0;
	for (int i = 0; i < m_lpActions.Length(); i++)
	{
		CAI_Action* pAction = m_lpActions.Get(i);
		if (pAction)
		{	// The look action isn't coinsidered an action per se, it should never
			// abort dialogues nor stop behaviours
			if (pAction->GetType() == CAI_Action::LOOK)
			{
				continue;
			}
			if (pAction->GetType() == CAI_Action::CHECKDEAD)
			{
				continue;
			}

			// GetPriority(false) returns the actual priority of the action as given in AIActions.xrg
			// GetPriority(true) returns the prioclass of thaction when calling TestTakeActions
			// This may be different because GetPriority(true) is only used to determine when an action
			// potentially could disrupt a dialogue or behaviour.
			int curPrioClass = 0;
			if (pAction->IsTaken())
			{	// Taken actions use their actual prio
				curPrioClass = pAction->GetPriority(false) & CAI_Action::MASK_PRIO_ONLY;
			}
			else
			{	// Actions not taken use the 'test take' prio
				curPrioClass = pAction->GetPriority(true) & CAI_Action::MASK_PRIO_ONLY;
			}
			
			if (curPrioClass > PrioClassToBeat)
			{
				if ((pAction->IsTaken()) || (m_pAI->m_Personality.EstimateAction(pAction)))
				{	// This is the best action, which should be returned
					// Update m_CurPrioClass so that methods relying on it get accurate results
					// (They don't actually as we do not check all actions down to lowest prio)
					m_CurPrioClass = Max(m_CurPrioClass,curPrioClass); 
#ifndef M_RTM
#ifdef DEBUG_PRINT_ACTIONS
					if (m_pAI->DebugTarget())
					{
						CStr Name = m_pAI->m_pGameObject->GetName();
						CStr ActionName = pAction->GetDebugString();
						ConOutL(CStrF("%s: TestTakeActions() got prio %d from action %s",Name.Str(),curPrioClass,ActionName.Str()));
					}
#endif
#endif
					if (_pPrio)
					{
						*_pPrio = curPrioClass;
					}
					return(true);
				}
			}
			else // curPrioClass <= PrioClassToBeat
			{
				// We skip here as m_lpActions are sorted in descending prio order
				// (Note: This assumes that actions cannot change prio at runtime, something I am
				// eternally grateful for but who knows, some schmuck may change this in the future.)
				return(false);
			}
		}
	}

	return(false);
}

CWO_ScenePoint* CAI_ActionHandler::GetPrevScenepoint(uint _iPos)
{
	if (_iPos < AH_SP_HISTORY_COUNT)
	{
		return(m_lScenepointHistory[_iPos]);
	}
	else
	{
		return(NULL);
	}
};

void CAI_ActionHandler::UpdateScenepointHistory(CWO_ScenePoint* _pScenePoint)
{
	for (int i = AH_SP_HISTORY_COUNT-1; i >= 1; i--)
	{
		m_lScenepointHistory[i] = m_lScenepointHistory[i-1];
	}
	if (_pScenePoint)
	{	// Don't zap index 0 with NULL
		m_lScenepointHistory[0] = _pScenePoint;
	}
};

void CAI_ActionHandler::UpdateLookpointHistory(CWO_ScenePoint* _pLookPoint)
{
	for (int i = AH_SP_HISTORY_COUNT-1; i >= 1; i--)
	{
		m_lLookpointHistory[i] = m_lLookpointHistory[i-1];
	}
	m_lLookpointHistory[0] = _pLookPoint;
};


// Returns false if it is OK to move to _Pos
bool CAI_ActionHandler::IsRestricted(const CVec3Dfp32& _Pos, bool _bActivate)
{
	if (!m_spRestriction)
	{
		return(false);
	}
	else
	{
		return(m_spRestriction->IsRestricted(_Pos,_bActivate));
	}
}

bool CAI_ActionHandler::IsRestricted(CAI_AgentInfo* _pTarget, bool _bActivate)
{
	if (_pTarget)
	{
		return(IsRestricted(_pTarget->GetBasePos(),_bActivate));
	}
	else
	{
		return(false);
	}
}

//Do we have any action overrides?
bool CAI_ActionHandler::HasActionOverrides()
{
	return (m_lspActionOverrides.Len() > 0);
};

int CAI_ActionHandler::GetClosestActionUser(int _ActionType,fp32 _MaxRange, bool _bActive)
{
	if (m_pAI)
	{
		return(m_pAI->GetClosestActionUser(_ActionType,_MaxRange,_bActive));
	}
	return(0);
}	

int CAI_ActionHandler::GetCurrentPriorityClass()
{
	return (m_CurPrioClass & CAI_Action::MASK_PRIO_ONLY);
};

int CAI_ActionHandler::OnServiceBehaviourActions()
{
	MSCOPESHORT(CAI_ActionHandler::OnServiceBehaviourActions);


#ifndef M_RTM
	if ((m_pAI->DebugRender())&&(m_pAI->m_pAIResources->ms_iDebugRenderTarget == m_pAI->GetObjectID()))
	{	// *** Put debug code here! ***
		bool wtf = true;
	}
#endif

	/*
	if ((!m_pAI->m_iDialoguePartner)&&(!m_pAI->m_iCrowdFocus))
	{	// Track player if nearby and looking at us
		int32 iPlayer = m_pAI->GetClosestPlayer();
		CAI_Core* pAIPlayer = m_pAI->GetAI(iPlayer);
		if ((pAIPlayer)&&(m_pAI->SqrDistanceToUs(iPlayer) < Sqr(64.0f))&&(m_pAI->IsLookWithinAngle(15.0f,iPlayer)))
		{	// Should we check relation?
			// CAI_AgentInfo* pPlayerInfo = m_pAI->m_KB.GetAgentInfo(iPlayer);
			m_pAI->m_pGameObject->AI_SetEyeLookDir(CVec3Dfp32(_FP32_MAX,0,0),iPlayer);
			
		}
	}
	*/

	int nActions = m_lpActions.GetNum();
	int nActiveActions = 0;
	for (int i = 0; i < nActions; i++)
	{
		CAI_Action* pAction = m_lpActions.GetNth(i);
		if (pAction)
		{
			if (pAction->IsTaken())
			{
				if ((pAction->IsValid())&&(!pAction->IsExpired()))
				{
					if (pAction->OnServiceBehaviour())
					{
						nActiveActions++;
					}
				}
			}
			else
			{	// Look should be running
				if ((m_pAI->m_Timer % 10 == 0)&&(pAction->GetType() == CAI_Action::LOOK))
				{
					int Estimate = m_pAI->m_Personality.EstimateAction(pAction);
					if (Estimate > 0)
					{	// We should store the result of Estimate to micro sort actions (or maybe not?)
						//Positive estimate, add to estimated actions and prio queue
						pAction->OnStart();
						pAction->OnTakeAction();
					}
				}	
			}
		}
	}

	return(nActiveActions);
};

int CAI_ActionHandler::OnTakeActions()
{
	MSCOPESHORT(CAI_ActionHandler::OnTakeActions);

	m_CurOverridePrio = CAI_Action::PRIO_MIN;
	int HighestPrio = CAI_Action::PRIO_MIN;
	int Prio = CAI_Action::PRIO_MIN;
	int PrioClass = CAI_Action::PRIO_MIN;
	if (!m_pAI || !m_pAI->IsValid())
	{	
		m_CurPrioClass = (HighestPrio & CAI_Action::MASK_PRIO_ONLY);
		return(HighestPrio);
	}
	m_CurPrioClass = (HighestPrio & CAI_Action::MASK_PRIO_ONLY);

	//Remove action overrides that have been marked for removal
	for (int i = m_lspActionOverrides.Len()-1; i >= 0; i--)
	{
//		CAI_Action* pAction = m_lspActionOverrides[i];
		if (m_lspActionOverrides[i]->m_bRemoveOnRefresh == true)
		{
			m_lspActionOverrides[i]->m_bRemoveOnRefresh = false;
			RemoveAction(m_lspActionOverrides[i]);
			m_lspActionOverrides[i] = NULL;
			m_lspActionOverrides.Del(i);
		}
	}
	int nActionOverrides = m_lspActionOverrides.Len();

	// Remove actions that should be killed
	if (m_lKillTypes.Len() > 0)
	{
		HandleActionKills();
	}

	// Handle m_spRestriction
	if (m_spRestriction)
	{
		if (m_spRestriction->IsTaken())
		{
			if (m_spRestriction->IsValid())
			{
				m_spRestriction->OnTakeAction();
				if (!m_spRestriction->IsExpired())
				{
					Prio = m_spRestriction->GetPriority();
					if (Prio > HighestPrio)
					{	// NOTE: We do NOT set m_CurPrioClass from restriction
						// m_CurPrioClass = (HighestPrio & CAI_Action::MASK_PRIO_ONLY);
						HighestPrio = Prio;
					}
				}
				else
				{
					m_spRestriction->OnQuit();
				}
			}
			else
			{
				m_spRestriction->OnQuit();
			}
		}
		else
		{
			int RestrictEstimate = m_pAI->m_Personality.EstimateAction(m_spRestriction);
			if (RestrictEstimate > 0)
			{
				m_spRestriction->OnStart();
			}
		}
	}

	if ((VALID_POS(m_RetargetPos))&&
		(m_pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)&&
		(!m_pAI->m_bBehaviourRunning)&&(!m_pAI->m_pGameObject->AI_IsJumping())&&
		(m_RetargetTimeout <= m_pAI->m_Timer))
	{
		// If m_RetargetPos we must consider what to do
		// ============================================
		CMat4Dfp32 Mat;
		m_pAI->GetBaseMat(Mat);
		bool bCoplanar = false;
		if (Mat.GetRow(2) * m_RetargetUp >= 0.99f)
		{
			if (Abs(m_RetargetUp * (m_RetargetPos + (Mat.GetRow(2) * DARKLING_PF_OFFSET) - Mat.GetRow(3))) <= 8.0f)
			{
				bCoplanar = true;
			}
		}
		CVec3Dfp32 Dir = m_RetargetPos - m_pAI->GetBasePos();
		if ((m_pRetargetSP)||(Dir.LengthSqr() > Sqr(AI_RETARGET_MAXRANGE * 0.5f)))
		{
			Dir.Normalize();
			Dir.SetRow(Mat,0);				// Fwd
			m_RetargetPos.SetRow(Mat,3);	// Pos
			m_RetargetUp.SetRow(Mat,2);		// Up
			Mat.RecreateMatrix(2,0);
		}
		else
		{
			INVALIDATE_POS(m_RetargetPos);
			INVALIDATE_POS(m_RetargetPosPF);
			m_pRetargetSP = NULL;
		}

#ifndef M_RTM
		if (m_pAI->DebugRender())
		{
			m_pAI->m_pServer->Debug_RenderMatrix(Mat,1.0f);
			if (m_pRetargetSP)
			{
				DebugDrawScenePoint(m_pRetargetSP,false,0,1.0f);
			}
		}
#endif
		// If theres no SP we try to jump there and failing we walk there 
		if ((!m_pRetargetSP)&&(!bCoplanar))
		{
			if ((m_pAI->SqrDistanceToUs(m_RetargetPos) >= Sqr(m_pAI->m_JumpMinRange))&&
				(m_pAI->SqrDistanceToUs(m_RetargetPos) <= Sqr(m_pAI->m_JumpMaxRange)))
			{
				if (m_pAI->OnMove(m_RetargetPosPF,m_pAI->m_ReleaseMaxSpeed,false,true,NULL) < CAI_Core::MOVE_WAIT_NO_PATHFINDER)
				{	// Sorry bud, no dice
					m_RetargetTimeout = uint32(m_pAI->m_Timer + (m_pAI->m_Patience * 2.0f));
					if (m_pRetargetSP)
					{
						m_pRetargetSP->InvalidateScenepoint(m_pAI->m_pServer,m_pAI->m_Patience);
					}
					m_pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_DARKLING_FUCKOFF,CAI_Action::PRIO_ALERT,0,-1);
					m_pAI->UseRandom(CStr("Retarget failure"),CAI_Device_Sound::IDLE_NOWAY,CAI_Action::PRIO_ALERT);
					m_pAI->ResetStuckInfo();
					m_pAI->ResetPathSearch();
				}
			}
		}
	
		// If we're at the designated SP we take it
		if ((m_pRetargetSP)&&(m_pRetargetSP->IsAt(m_pAI->GetBasePos())))
		{	// We are there but not neccessary aligned
			if (m_pRetargetSP->IsAligned(m_pAI->GetLookDir()))
			{
				if (RequestScenePoint(m_pRetargetSP))
				{
#ifndef M_RTM
					DebugDrawScenePoint(m_pRetargetSP,true,0,m_pRetargetSP->GetBehaviourDuration());
#endif
					ActivateScenePoint(m_pRetargetSP,CAI_Action::PRIO_IDLE);
					INVALIDATE_POS(m_RetargetPos);
					INVALIDATE_POS(m_RetargetPosPF);
					m_pRetargetSP = NULL;
				}
				else
				{
					m_pRetargetSP = NULL;
				}
			}
			else
			{	// Not yet aligned
				m_pAI->OnTrackDir(m_pRetargetSP->GetDirection(),0,6,false,false);
			}
		}
	
		// Can we jump to the SP?
		if ((!bCoplanar)&&(m_pRetargetSP)&&(m_pRetargetSP->GetType() & CWO_ScenePoint::JUMP_CLIMB))
		{
			if ((m_pAI->SqrDistanceToUs(m_RetargetPos) >= Sqr(m_pAI->m_JumpMinRange))&&
				(m_pAI->SqrDistanceToUs(m_RetargetPos) <= Sqr(m_pAI->m_JumpMaxRange)))
			{
				Mat = m_pRetargetSP->GetPositionMatrix();
				uint32 Flags = CAI_Core::JUMP_CHECK_COLL | CAI_Core::JUMP_TO_AIR;
				if (m_pAI->AddJumpTowardsPositionCmd(&Mat,CAI_Core::JUMPREASON_RETARGET,Flags))
				{
#ifdef M_Profile
					m_pAI->Debug_RenderWire(m_pAI->GetBasePos(),m_RetargetPos,kColorGreen,3.0f);
#endif
				}
				else
				{	// Couldn't jump there and shouldn't walk there?
					if (!(m_pRetargetSP->GetType() & CWO_ScenePoint::WALK_CLIMB))
					{
						m_pRetargetSP = NULL;
					}
				}
			}
		}

		// If there's no SP, movedevice is free and we're not jumping we walk there
		if ((m_pAI->m_DeviceMove.IsAvailable())&&(!m_pAI->m_pGameObject->AI_IsJumping()))
		{	// Either there's no scenepoint or the scenepoint is not a jumpclimb
			if ((!m_pRetargetSP)||(!(m_pRetargetSP->GetType() & CWO_ScenePoint::JUMP_CLIMB)))
			{
				if ((m_pAI->SqrDistanceToUs(m_RetargetPos) > Sqr(AI_RETARGET_MAXRANGE * 0.5f))&&
					(m_pAI->SqrDistanceToUs(m_RetargetPos) <= Sqr(Max3(m_pAI->m_RoamMaxRange,m_pAI->m_CloseMaxRange,m_pAI->m_CombatMaxRange))))
				{	// Move there
					// We're beyond AI_RETARGET_MAXRANGE, walk there dude
					if (m_pAI->OnMove(m_RetargetPosPF,m_pAI->m_ReleaseMaxSpeed,false,true,NULL) < CAI_Core::MOVE_WAIT_NO_PATHFINDER)
					{	// Sorry bud, no dice
						m_RetargetTimeout = uint32(m_pAI->m_Timer + (m_pAI->m_Patience * 2.0f));
						if (m_pRetargetSP)
						{
							m_pRetargetSP->InvalidateScenepoint(m_pAI->m_pServer,m_pAI->m_Patience);
						}
						m_pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_DARKLING_FUCKOFF,CAI_Action::PRIO_ALERT,0,-1);
						m_pAI->UseRandom(CStr("Retarget failure"),CAI_Device_Sound::IDLE_NOWAY,CAI_Action::PRIO_ALERT);
						m_pAI->ResetStuckInfo();
						m_pAI->ResetPathSearch();
					}
				}
				else
				{	// We're there dude
					INVALIDATE_POS(m_RetargetPos);
					INVALIDATE_POS(m_RetargetPosPF);
					m_pAI->ResetStuckInfo();
					m_pAI->ResetPathSearch();
					CAI_Action* pAction = NULL;
					if (GetAction(CAI_Action::ROAM,&pAction))
					{
						pAction->ExpireWithDelay(m_pAI->m_Patience);
					}
				}
			}
		}

		// Finally, if our enemy is really close to m_RetargetPos we may ignore the m_RetargetPos in favour of a fresh kill
		if ((VALID_POS(m_RetargetPos))&&(m_iTarget))
		{
			CAI_Core* pTargetAI = m_pAI->GetAI(m_iTarget);
			if (pTargetAI)
			{
				CVec3Dfp32 Pos = pTargetAI->GetBasePos();
				if (m_RetargetPos.DistanceSqr(Pos) <= Sqr(AI_RETARGET_MAXRANGE * 0.5f))
				{
					INVALIDATE_POS(m_RetargetPos);
					INVALIDATE_POS(m_RetargetPosPF);
				}
			}
		}
	}
	else
	{
		m_RetargetTimeout = 0;
		INVALIDATE_POS(m_RetargetPos);
		INVALIDATE_POS(m_RetargetPosPF);
	}

#ifndef M_RTM
	if (m_pAI->DebugTarget())
	{
		// *** Put debug code here! ***
		bool wtf = true;
	}
#endif

	int nActions = m_lpActions.GetNum();
	int nActiveActions = 0;
	for (int i = 0; i < nActions; i++)
	{
		CAI_Action* pAction = m_lpActions.GetNth(i);
		if (pAction)
		{
			if (pAction->IsTaken())
			{
				if (pAction->IsExpired())
				{
					pAction->OnQuit();
				}
				else
				{
					if (pAction->IsValid())
					{
						// If the action is more than one prio class below tha highest taken it cannot be taken
						Prio = pAction->GetPriority();
						PrioClass = Prio & CAI_Action::MASK_PRIO_ONLY;
						if (pAction->CheckPrioClass())
						{
							pAction->OnTakeAction();
							if (Prio > HighestPrio)
							{	
								m_CurPrioClass = (HighestPrio & CAI_Action::MASK_PRIO_ONLY);
								HighestPrio = Prio;
							}
							if (pAction->GetType() != CAI_Action::LOOK)
							{	// Look doesn't count as an action per se
								nActiveActions++;
							}
						}
					}
					else
					{
						pAction->OnQuit();
					}
				}
			}
			else
			{	// Action is NOT taken
				if ((m_nActiveActions == 0)||(m_ReportedEvents)||((m_pAI->m_Timer % 20 == 0)&&(CheckEssentialDevices())))
				{
					//Estimate current action
					if (nActionOverrides > 0)
					{	// Check if action should be overridden
						if ((pAction->m_Type != CAI_Action::PATROL_PATH)&&
							(pAction->m_Type != CAI_Action::HOLD)&&
							(pAction->IsOverridableByActionOverride()))
						{
							continue;
						}
					}

					int Estimate = m_pAI->m_Personality.EstimateAction(pAction);
					if (Estimate > 0)
					{	// We should store the result of Estimate to micro sort actions (or maybe not?)
						//Positive estimate, add to estimated actions and prio queue
						pAction->OnStart();
						if (pAction->CheckPrioClass())
						{
							pAction->OnTakeAction();
							Prio = pAction->GetPriority();
							if (Prio > HighestPrio)
							{
								m_CurPrioClass = (HighestPrio & CAI_Action::MASK_PRIO_ONLY);
								HighestPrio = Prio;
							}
							if (pAction->GetType() != CAI_Action::LOOK)
							{	// Look doesn't count as an action per se
								nActiveActions++;
							}
						}
					}
				}
			}
		}
	}
	m_nActiveActions = nActiveActions;

	if ((GetAI())&&(GetAI()->m_pServer)&&((HighestPrio & CAI_Action::MASK_PRIO_ONLY) > m_CurPrioClass))
	{
		if (GetAI()->GetStealthTension() >= CAI_Core::TENSION_HOSTILE)
		{
			CWObject_Message Msg(OBJMSG_CHAR_DESTROYCAUSUALDIALOGUE);
			Msg.m_Param0 = (HighestPrio & CAI_Action::MASK_PRIO_ONLY);
			GetAI()->m_pServer->Message_SendToObject(Msg,GetAI()->m_pGameObject->m_iObject);
		}
	}
	m_CurPrioClass = (HighestPrio & CAI_Action::MASK_PRIO_ONLY);

	//Reset stuff
	m_ReportedEvents = 0;
	if (m_CurOverridePrio > CAI_Action::PRIO_MIN)
	{
		Override(m_CurOverridePrio);
	}
	m_CurOverridePrio = CAI_Action::PRIO_MIN;

	return(HighestPrio);
}

//Create actions and action sets defined by AI resource handler and given registry key. For actions 
//value of key itself is the name by which action is identified in resource handler. Any children 
//are parameter values that will overwrite the defaults defined in resource handler.
void CAI_ActionHandler::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (!_pKey || !m_pAI || !m_pAI->IsValid())
		return;

	const CStr KeyName = _pKey->GetThisName();

	if (KeyName.CompareSubStr("AI_ACTIONSETS") == 0)
	{
		//Check which actionsets we've got
		for (int i = 0; i < _pKey->GetNumChildren(); i++)
		{
			//Check type of action set
			const CRegistry * pChild = _pKey->GetChild(i);
			if (pChild)
			{
				if (pChild->GetThisName().CompareNoCase("WEAPONHANDLING") == 0)
				{
					//We will automatically create weapon handling actions for every weapon we get, 
					//possibly excluding some actions as defined by subkeys
					m_bWeaponHandling = true;
					for (int j = 0; j < pChild->GetNumChildren(); j++)
					{
						if (pChild->GetName(j) == "EXCLUDE")
						{
							int Action = CAI_Action::StrToType(pChild->GetValue(j));
							if (Action != CAI_Action::INVALID)
							{
								m_lWeaponHandlerExcludes.Add(Action);
							}
						}
					}
				}
			}
		}
	}
	else if (KeyName.CompareSubStr("AI_ACTIONS") == 0)
	{	//Add all actions
		for (int i = 0; i < _pKey->GetNumChildren(); i++)
		{
			const CRegistry* pChild = _pKey->GetChild(i);
			if (pChild)
				//This should be an action definition or action name, to be handled on spawn
				m_lspActionRegs.Add(pChild->Duplicate());
		}
	}
	else if (KeyName.CompareSubStr("AI_ACTIONKILLS") == 0)
	{
		//Add all actiontypes to kill
		for (int i = 0; i < _pKey->GetNumChildren(); i++)
		{
			const CRegistry* pChild = _pKey->GetChild(i);
			if (pChild)
			{
				// Find the name of the action to kill
				CStr Temp = pChild->GetThisName();
				int iAction = Temp.TranslateInt(CAI_Action::ms_lStrID);
				Temp = pChild->GetThisValue();
				int ActionID = Temp.Val_int();
				if (iAction != -1)
				{
					m_lKillTypes.Add(iAction);
					if (iAction > 0)
					{
						m_lKillIDs.Add(ActionID);
					}
					else
					{
						m_lKillIDs.Add((uint8)0);
					}
				}
			}
		}
	}
};

//Override any current actions of lower prio
void CAI_ActionHandler::Override(int _Priority)
{
	int nActions = m_lpActions.GetNum();
//	int nActiveActions = 0;
	for (int i = 0; i < nActions; i++)
	{
		CAI_Action* pAction = m_lpActions.GetNth(i);
		if ((pAction)&&(pAction->GetPriority() < _Priority))
		{	// Quit taken actions
			if (pAction->IsTaken())
			{
				pAction->OnQuit();
			}
		}
	}
};

void CAI_ActionHandler::RequestOverride(int _Priority)
{
	if (_Priority > m_CurOverridePrio)
	{
		m_CurOverridePrio = _Priority;
	}
}

//Notify action handler of any newly acquired weapon, so that appropriate actions can be added
void CAI_ActionHandler::ReportNewWeapon(int _Type)
{
	//Add weapon handling actions, if we've got the weapon handling action set
	if (m_bWeaponHandling)
	{
		//Get action definitions for weapon type
		CAI_ResourceHandler * pAIRes = m_pAI->GetAIResources();
		if (pAIRes)
		{
			TArray<CStr> lpActionNames;
			lpActionNames.Clear();
			if (pAIRes->GetWeaponHandlerActions(_Type, &lpActionNames))
			{
				//Create and add actions from definitions
				CAI_ActionDefinition * pActDef;
				for (int i = 0; i < lpActionNames.Len(); i++)
				{
					pActDef = pAIRes->GetActionDefinition(lpActionNames[i]);
					if (pActDef)
					{
						//Create action from definition
						spCAI_Action spAction = CreateAction(pActDef);
						AddAction(spAction);
					}
					else
					{
						//Create action from name
						int ActionType = CAI_Action::StrToType(lpActionNames[i]);
						spCAI_Action spAction = CAI_Action::CreateAction(ActionType, this);
						AddAction(spAction);
					}
				}
			}
		}
	}
};


//Report that an event has occurred and let action handler decide what to do
void CAI_ActionHandler::ReportEvent(int _iEvent,int _iWho,int _Data)
{
	m_ReportedEvents |= (1 << _iEvent);
	
	//All events cause an auxiliary prio override
	Override(CAI_Action::PRIO_AUXILIARY);

	//Some events have additional side effects
	switch (_iEvent)
	{
	case CAI_EventHandler::ON_SPOT_ENEMY:
		break;
		
	case CAI_EventHandler::ON_SPOT_HOSTILE:
		break;

	case CAI_EventHandler::ON_INJURED:
		m_iAttacker = _iWho;
		m_DamageType = _Data;
		m_bWasInjured = true;
		m_bWasUninjured = false;
		m_bWasGrazed = false;
		break;

	case CAI_EventHandler::ON_HIT_UNINJURED:
		if (!m_bWasInjured)
		{	// Injured is more important than uninjured ye?
			m_iAttacker = _iWho;
			m_DamageType = _Data; 
			m_bWasInjured = false;
			m_bWasUninjured = true;
			m_bWasGrazed = false;
		}
		break;
	}
};

//Get highest prio of current actions
int CAI_ActionHandler::GetCurPriority()
{
	return m_CurPrioClass;
};


#ifndef M_RTM
//Debug info about current actions
CStr CAI_ActionHandler::GetDebugString()
{
	CStr Info = "Actions: ";
	CAI_Action * pAction;
	for (int i = 0; i < m_lpActions.GetNum(); i++)
	{
		pAction = m_lpActions.Get(i);
		if (pAction)
		{
			if (pAction->IsTaken())
			{
				Info += pAction->GetDebugString() + ", ";
			}
			else
			{
				Info += "("+pAction->GetDebugString() + "), ";
			}
		}
	}
	return Info;
}
#endif

void CAI_ActionHandler::OnDeltaLoad(CCFile* _pFile)
{
	int32 Temp32;
	int8 Temp8;

	// Load friends, enemies etc
	// Primary and secondary DETECTED+ enemy
	_pFile->ReadLE(Temp32); m_iTarget = Temp32;
	_pFile->ReadLE(Temp32); m_iTarget2 = Temp32;
	_pFile->ReadLE(Temp32); m_iHostile = Temp32;
	_pFile->ReadLE(Temp32); m_iHostile2 = Temp32;
	_pFile->ReadLE(Temp32); m_iFriend = Temp32;
	_pFile->ReadLE(Temp32); m_iFriend2 = Temp32;
	_pFile->ReadLE(Temp32); m_iInvestigate = Temp32;
	_pFile->ReadLE(Temp32); m_iInvestigate2 = Temp32;
	_pFile->ReadLE(Temp32); m_iInvestigateObj = Temp32;

	//Load restriction, if any
	int8 HasRestriction = 0;
	_pFile->ReadLE(HasRestriction);
	if (HasRestriction)
	{
		if (m_spRestriction == NULL)
		{
			m_spRestriction = MNew1(CAI_Action_Restrict, this);
		}
		m_spRestriction->OnDeltaLoad(_pFile);
	}
	else
	{
		m_spRestriction = NULL;
	}

	//Load any action overrides
	ClearActionOverrides();
	int8 nAO = 0;
	_pFile->ReadLE(nAO);
	if (nAO > 0)
	{
		uint8 Type;
		for (int i = 0; i < nAO; i++)
		{
			//Create action of saved type, then load it
			_pFile->ReadLE(Type);
			spCAI_Action spAction = CAI_Action::CreateAction(Type, this);
			spAction->OnDeltaLoad(_pFile);
			AddActionOverride(spAction);
		}
	}
	_pFile->ReadLE(Temp32); m_iEnemyOverride = Temp32;

	//Check if we've got a saved override scenepoint
	int8 HasOverrideSP = 0;
	_pFile->ReadLE(HasOverrideSP);
	if (HasOverrideSP)
	{
		CStr SPName = _pFile->Readln();
		SetOverrideScenePoint(SPName);
	}

	// Save/load the state of m_bCheckValidTeams
	_pFile->ReadLE(Temp8); m_bCheckValidTeams = (Temp8 != 0);

	//Load any scenepoints we have been holding, so that we make sure 
	//they are released next refresh after if unused
	_pFile->ReadLE(Temp8);
	int16 iSP;
	CWO_ScenePoint * pSP;
	CWO_ScenePointManager * pSPM = m_pAI->GetScenePointManager();
	for (int i = 0; i < Temp8; i++)
	{
		_pFile->ReadLE(iSP);
		if (pSPM)
		{
			pSP = pSPM->GetScenePointFromIndex(iSP);
			if (pSP)
			{
				//Found a scenepoint that was saved
				if (m_lpLastHeldScenePoints.Find(pSP) == -1)
					m_lpLastHeldScenePoints.Add(pSP);
			}
		}
	}
};

void CAI_ActionHandler::OnDeltaSave(CCFile* _pFile)
{
	int32 Temp32;
	int8 Temp8;
	int i;

	Temp32 = m_iTarget; _pFile->WriteLE(Temp32);
	Temp32 = m_iTarget2; _pFile->WriteLE(Temp32);
	Temp32 = m_iHostile; _pFile->WriteLE(Temp32);
	Temp32 = m_iHostile2; _pFile->WriteLE(Temp32);
	Temp32 = m_iFriend; _pFile->WriteLE(Temp32);
	Temp32 = m_iFriend2; _pFile->WriteLE(Temp32);
	Temp32 = m_iInvestigate; _pFile->WriteLE(Temp32);
	Temp32 = m_iInvestigate2; _pFile->WriteLE(Temp32);
	Temp32 = m_iInvestigateObj; _pFile->WriteLE(Temp32);

	//Save restriction
	if (m_spRestriction)
	{
		Temp8 = 1; _pFile->WriteLE(Temp8);
		m_spRestriction->OnDeltaSave(_pFile);
	}
	else
	{
		Temp8 = 0; _pFile->WriteLE(Temp8);
	}

	//Save action overrides
	Temp8 = m_lspActionOverrides.Len(); _pFile->WriteLE(Temp8);
	uint8 Type;
	for (i = 0; i < m_lspActionOverrides.Len(); i++)
	{
		//Save type and action
		Type = (m_lspActionOverrides[i])->GetType();  _pFile->WriteLE(Type);
		(m_lspActionOverrides[i])->OnDeltaSave(_pFile);
	}
	Temp32 = m_iEnemyOverride; _pFile->WriteLE(Temp32);

	//Save any override scenepoint we might have
	Temp8 = (m_pOverrideScenePoint) ? 1 : 0;
	_pFile->WriteLE(Temp8);
	if (m_pOverrideScenePoint)
	{
		CStr SPName = m_pOverrideScenePoint->GetName();
		_pFile->Writeln(SPName);
	}

	// Save/load the state of m_bCheckValidTeams
	Temp8 = m_bCheckValidTeams;	_pFile->WriteLE(Temp8);

	//Save any scenepoints we have held, so that we make sure 
	//they are released next refresh after load if unused
	CWO_ScenePointManager * pSPM = m_pAI->GetScenePointManager();
	Temp8 = (pSPM) ? m_lpLastHeldScenePoints.GetNum() : 0;
	_pFile->WriteLE(Temp8);
	int16 iSP;
	for (i = 0; i < Temp8; i++)
	{
		if (m_lpLastHeldScenePoints.IsValid(i))
		{
			iSP = pSPM->GetScenePointIndex(m_lpLastHeldScenePoints.Get(i));
			_pFile->WriteLE(iSP);
		}
	}
};

//Checks if the given server index points to a valid target.
//Returns pointer to targets agent info if so else NULL
CAI_AgentInfo* CAI_ActionHandler::IsValidTarget(int _iObj)
{
	CAI_AgentInfo* pInfo = IsValidRelation(_iObj,CAI_AgentInfo::ENEMY);
	if (pInfo)
	{
		if (m_pAI->IsValidTarget(_iObj))
		{
			return(pInfo);
		}
		else
		{
			return(NULL);
		}
	}
	else
	{
		return(NULL);
	}
};

//Check current target and acquire a new one if this is bad
CAI_AgentInfo* CAI_ActionHandler::AcquireTarget()
{
	CAI_AgentInfo* pTarget = NULL;
	
	//Check if current target is valid, and find a new target if it isn't.
	if ((m_iEnemyOverride >= 0)&&(pTarget = IsValidTarget(m_iEnemyOverride)))
	{
		m_iTarget = m_iEnemyOverride;
		return(pTarget);
	}
	else if (!(pTarget = IsValidTarget(m_iTarget)))
	{
		// *** Let CAI_Core
		//If we are in fight mode, we always choose the character we're in fight mode with
		int iFighting = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISINFIGHTMODE), m_pAI->m_pGameObject->m_iObject);
		if ((iFighting)&&(pTarget = IsValidTarget(iFighting)))
		{
			m_iTarget = iFighting;
			return pTarget;
		}
		// ***
	}
	return(pTarget);
};

//Check current target and acquire a new one if this is bad
CAI_AgentInfo* CAI_ActionHandler::AcquireHostile()
{
	CAI_AgentInfo* pHostile = NULL;

	pHostile = m_pAI->m_KB.GetAgentInfo(m_iHostile);
	if ((pHostile)&&(pHostile->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)&&
		(pHostile->GetCurRelation() >= CAI_AgentInfo::HOSTILE)&&
		(pHostile->GetCurRelation() >= CAI_AgentInfo::HOSTILE_2))
	{
		return(pHostile);
	}

	return(NULL);
};


//Suggest that we should look for another target. Call when target is bad for an action and we're reasonably
//sure there are no other actions that target is better for.
void CAI_ActionHandler::SuggestTargetChange()
{
	m_iTarget = 0;
};



// Returns true if _iObj is a security threat to this ai
bool CAI_ActionHandler::IsSecurityThreat(int _iObj)
{
	CAI_Core* pAI = GetAI();

	if (pAI->m_SecurityTolerance < 0)
	{	// We need a m_SecurityTolerance of 0+ to ever consider someone a security threat
		return(false);
	}

	//Check how much target is violating security
	int SecurityLevel = pAI->m_KB.GetSecurityLevel(_iObj);

	if (SecurityLevel > 0)
	{	//Secure area, reduce level by own clearance
		SecurityLevel -= pAI->GetSecurityClearance(_iObj);
	}

	if (SecurityLevel >= pAI->m_SecurityTolerance)
	{
		return(true);
	}
	else
	{
		return(false);
	}
}

//Checks if the given server index points to a valid agent with a relation of _MinRelation or worse
//Returns pointer to hostile's agent info if so else NULL
CAI_AgentInfo * CAI_ActionHandler::IsValidRelation(int _iObj,int _MinRelation,int _MaxRelation)
{
	//Is the indexed character valid and a spotted enemy?
	CAI_AgentInfo * pInfo = m_pAI->m_KB.GetAgentInfo(_iObj);
	if (pInfo &&
		m_pAI->IsValidAgent(pInfo->GetObject())&& 
		(pInfo->GetRelation() >= _MinRelation)&&
		(pInfo->GetRelation() <= _MaxRelation)&&
		(pInfo->GetAwareness() >= CAI_AgentInfo::DETECTED)
		) 
		return pInfo;
	else
		return NULL;
};

// *** TODO: This clearly sucks. Use info from KB for all normal cases ***
CAI_AgentInfo * CAI_ActionHandler::FindRelation(bool _bFindNow,int _MinRelation,int _MaxRelation,int _MinAwareness,int _MaxAwareness)
{
	if ((_MaxRelation < _MinRelation)||(_MaxRelation <= CAI_AgentInfo::UNKNOWN)||(_MaxAwareness < _MinAwareness))
	{
		return(NULL);
	}

	CAI_AgentInfo* pInfo = NULL;
//	CAI_AgentInfo* pInfo2 = NULL;
//	int32 Relation = 0;
//	int32 Relation2 = 0;
//	int32 Awareness = 0;
//	int32 Awareness2 = 0;
	if (_MinRelation >= CAI_AgentInfo::ENEMY)
	{
		pInfo = m_pAI->m_KB.GetAgentInfo(m_iTarget);
		if ((pInfo)&&(pInfo->GetCurRelation() >= _MinRelation)&&
			(pInfo->GetCurRelation() <= _MaxRelation)&&
			(pInfo->GetCurAwareness() >= _MinAwareness)&&
			(pInfo->GetCurAwareness() <= _MaxAwareness))
		{
			return(pInfo);
		}
		pInfo = m_pAI->m_KB.GetAgentInfo(m_iTarget2);
		if ((pInfo)&&(pInfo->GetCurRelation() >= _MinRelation)&&
			(pInfo->GetCurRelation() <= _MaxRelation)&&
			(pInfo->GetCurAwareness() >= _MinAwareness)&&
			(pInfo->GetCurAwareness() <= _MaxAwareness))
		{
			return(pInfo);
		}
	}
	else if (_MinRelation >= CAI_AgentInfo::HOSTILE)
	{
		pInfo = m_pAI->m_KB.GetAgentInfo(m_iHostile);
		if ((pInfo)&&(pInfo->GetCurRelation() >= _MinRelation)&&
			(pInfo->GetCurRelation() <= _MaxRelation)&&
			(pInfo->GetCurAwareness() >= _MinAwareness)&&
			(pInfo->GetCurAwareness() <= _MaxAwareness))
		{
			return(pInfo);
		}
		pInfo = m_pAI->m_KB.GetAgentInfo(m_iHostile2);
		if ((pInfo)&&(pInfo->GetCurRelation() >= _MinRelation)&&
			(pInfo->GetCurRelation() <= _MaxRelation)&&
			(pInfo->GetCurAwareness() >= _MinAwareness)&&
			(pInfo->GetCurAwareness() <= _MaxAwareness))
		{
			return(pInfo);
		}
	}
	else if (_MaxRelation <= CAI_AgentInfo::FRIENDLY)
	{
		pInfo = m_pAI->m_KB.GetAgentInfo(m_iFriend);
		if ((pInfo)&&(pInfo->GetCurRelation() >= _MinRelation)&&
			(pInfo->GetCurRelation() <= _MaxRelation)&&
			(pInfo->GetCurAwareness() >= _MinAwareness)&&
			(pInfo->GetCurAwareness() <= _MaxAwareness))
		{
			return(pInfo);
		}
		pInfo = m_pAI->m_KB.GetAgentInfo(m_iFriend2);
		if ((pInfo)&&(pInfo->GetCurRelation() >= _MinRelation)&&
			(pInfo->GetCurRelation() <= _MaxRelation)&&
			(pInfo->GetCurAwareness() >= _MinAwareness)&&
			(pInfo->GetCurAwareness() <= _MaxAwareness))
		{
			return(pInfo);
		}
	}

	//Only look for Hostile every few frames, unless the _bFindNow flag is true
	if (true)
	{	//Check all spotted characters to find any suitable Hostile.
		//Find the most certain and closest valid Hostile
		int BestAwareness = CAI_AgentInfo::NONE;
		fp32 MinDistSqr = _FP32_MAX;
		fp32 DistSqr;
		CAI_AgentInfo * pInfo;
		CAI_AgentInfo * pHostile = NULL;
		for (int i = 0; i < m_pAI->m_KB.NumAgentInfo(); i++)
		{
			pInfo = m_pAI->m_KB.IterateAgentInfo(i);
			if (pInfo)
			{
				if ((_MinAwareness > NONE)&&(pInfo->GetCurAwareness() < _MinAwareness))
				{
					continue;
				}
				if (pInfo->GetCurAwareness() > _MaxAwareness)
				{
					continue;
				}

				//Check if agent is a valid target
				int32 Relation = pInfo->GetCurRelation();
				if ((Relation >= _MinRelation)&&(Relation <= _MaxRelation))
				{
					if (pInfo->GetAwareness() > BestAwareness)
					{	// Better awareness, we choose this one regardless of range
						DistSqr = (pInfo->GetBasePos()).DistanceSqr(m_pAI->GetBasePos());
						pHostile = pInfo;
						MinDistSqr = DistSqr;
						BestAwareness = pInfo->GetAwareness();
					}
					else if (pInfo->GetAwareness() == BestAwareness)
					{
						// Better awareness, we choose this one regardless of range
						DistSqr = (pInfo->GetBasePos()).DistanceSqr(m_pAI->GetBasePos());
						if (DistSqr < MinDistSqr)
						{
							//Closer target found, continue checking...
							pHostile = pInfo;
							MinDistSqr = DistSqr;
							BestAwareness = pInfo->GetAwareness();
						};
					}
				};
			}
		};

		//Did we find a hostile?
		if (pHostile)
		{	//Set current Hostile to found Hostile and return with success
			m_iHostile = pHostile->GetObjectID();
			return pHostile;
		}
		else
		{
			return NULL;
		}
	}
	else
	{	
		//Check if old hostile is still valid
		if (m_iHostile)
		{
			CAI_AgentInfo* pHostile = m_pAI->m_KB.GetAgentInfo(m_iHostile);
			if ((pHostile)&&(IsValidRelation(pHostile->GetObjectID(),_MinRelation,_MaxRelation)))
			{
				return(pHostile);
			}
			else
			{
				m_iHostile = 0;
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}
};


//Will we engage the given opponent if we try to initiate melee now?
bool CAI_ActionHandler::CanEngage(CAI_AgentInfo * _pTarget)
{
	CWObject_Message GetOpponent(OBJMSG_CHAR_GETFIGHTCHARACTER);
	return (_pTarget->GetObjectID() == m_pAI->m_pServer->Message_SendToObject(GetOpponent, m_pAI->m_pGameObject->m_iObject));
};

CAI_AgentInfo* CAI_ActionHandler::StopForPlayer()
{
	// If we're close enough to the PLAYER and he is to the front we pause
	int32 playerID = m_pAI->GetClosestPlayer();
	CAI_AgentInfo* pPlayerAgent = m_pAI->m_KB.GetAgentInfo(playerID);
	CAI_Core* pPlayerAI = m_pAI->GetAI(playerID);
	if ((!pPlayerAgent)||(pPlayerAI))
	{
		pPlayerAgent = m_pAI->m_KB.AddAgent(playerID);
	}

	// Stop for player unless he is an enemy
	if ((pPlayerAgent)&&(pPlayerAgent->GetCurAwareness() > CAI_AgentInfo::NOTICED)&&
		(pPlayerAgent->GetCurRelation() < CAI_AgentInfo::ENEMY))
	{
		CVec3Dfp32 PlayerPos = m_pAI->GetBasePos(playerID);
		CVec3Dfp32 Offset = PlayerPos - m_pAI->GetBasePos();
		if (Offset.LengthSqr() < Sqr(48.0f))
		{
			CVec3Dfp32 OffsetDir = Offset;
			OffsetDir[2] = 0.0f;
			OffsetDir.Normalize();
			CVec3Dfp32 LookDir =  m_pAI->GetLookDir();
			LookDir[2] = 0.0f;
			LookDir.Normalize();
			
			// Stop if I face him
			if (OffsetDir * LookDir < 0.1f)
			{
				return(NULL);
			}

			CVec3Dfp32 Velocity = m_pAI->GetPrevVelocity();
			Velocity -= pPlayerAI->GetPrevVelocity();
			if (Velocity.LengthSqr() >= Sqr(0.1f))
			{
				Velocity.Normalize();
				if (Velocity * OffsetDir <= 0.1f)
				{	// We're moving apart
					return(NULL);
				}
			}
			if (pPlayerAgent->GetCurRelation() < CAI_AgentInfo::HOSTILE)
			{
				m_pAI->OnTrackAgent(pPlayerAgent,10,false,true);
				if ((pPlayerAI)&&(pPlayerAI->GetLookDir() * OffsetDir < -0.707f))
				{
					m_pAI->UseRandom("StopForPlayer",CAI_Device_Sound::IDLE_CALL,CAI_Action::PRIO_IDLE);
				}
			}
			return(pPlayerAgent);
		}
	}

	return(NULL);
};

// =================================================================================
// Scenepoint related methods
// =================================================================================

fp32 CAI_ActionHandler::MeasureLightAtScenepoint(CWO_ScenePoint* _pSP,bool _bTraceLines)
{
	fp32 rLight = 1.0f;
	if (_pSP)
	{	// *** Should we check for validity of SP? Nah, we know what we're dong ;) ***
		rLight = m_pAI->MeasureLightIntensityAt(_pSP->GetPosition(),_bTraceLines,0);
	}

	return(rLight);
};

//Set override scene point from name (clears any previous override scene point)
void CAI_ActionHandler::SetOverrideScenePoint(CStr _Name)
{
	if (_Name == CStr(""))
	{
		m_pOverrideScenePoint = NULL;
	}
	else
	{
		CWO_ScenePointManager * pSPM = m_pAI->GetScenePointManager();
		if (pSPM)
		{
			//Get override scene point
			m_pOverrideScenePoint = pSPM->Find(_Name);

			//If we got a scene point check that it's potentially usable by us
			CWObject_Interface_AI* pObj = m_pAI->m_pGameObject;
			uint32 OwnerName = m_pAI->m_pServer->Object_GetNameHash(pObj->m_iOwner);

			uint16 liTeams[32];
			uint nTeams = pObj->AI_GetTeams(liTeams, sizeof_buffer(liTeams));

			if (m_pOverrideScenePoint && !m_pOverrideScenePoint->IsValid(m_pAI->m_pServer, pObj->GetNameHash(), OwnerName, liTeams, nTeams))
			{
				//Scenepoint not valid
				m_pOverrideScenePoint = NULL;
			}
		}
	}
	m_bClearOverrideScenePoint = false;
};

// Returns wether there exist scenepoints of a given type
bool CAI_ActionHandler::ScenePointTypeAvailable(int _Type,fp32 _MaxRange)
{
	if (_Type & m_ScenePointTypes)
	{
		return(true);
	}
	else
	{	// We may sometimes return true to force updates through GetScenePoints()
		int Tick = m_pAI->GetAITick();
		if (Tick > m_NextScenePointTick)
		{
			GetScenePoints(_MaxRange);
		}
		
		if (_Type & m_ScenePointTypes)
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
};



// Get all scenepoints that are potentially within the given distance and valid for us
int32 CAI_ActionHandler::GetScenePoints(fp32 _RangeDelimiter)
{
	M_ASSERT(m_pAI && m_pAI->m_pServer,"Code error: No ai or server in action handler");

	//NOTE: This method doesn't bother with override scenepoint at all. It's up to using
	//to handle that appropriately themselves.

	//If current scenepoints are old, scrap them and get new
	int Tick = m_pAI->GetAITick();
	if (Tick > m_NextScenePointTick)
	{
		m_lpScenePoints.QuickSetLen(0);
		m_ScenePointTypes = 0;

		//Get scene point resource manager
		CWO_ScenePointManager * pSPM = m_pAI->GetScenePointManager();
		if (!pSPM)
		{
#ifndef M_RTM
			CStr Name = m_pAI->m_pGameObject->GetName();
			ConOut(Name + CStr(": GetScenePointManager() failed"));	
#endif
			return m_lpScenePoints.Len();
		}
		//Get own teams
		CWObject_Interface_AI* pObj = m_pAI->m_pGameObject;
		uint16 liTeams[32];
		uint nTeams = pObj->AI_GetTeams(liTeams, sizeof_buffer(liTeams));

		//Select scene points
		pSPM->Selection_Clear();
		if (m_spRestriction)
		{
			pSPM->Selection_AddChunk(m_pAI->GetBasePos(), 
				_RangeDelimiter, 
				pObj->GetNameHash(),
				pObj->m_pWServer->Object_GetNameHash( pObj->m_iOwner ),
				liTeams, nTeams,
				m_spRestriction->GetRoom());
		}
		else
		{
			pSPM->Selection_AddChunk(m_pAI->GetBasePos(), 
				_RangeDelimiter, 
				pObj->GetNameHash(),
				pObj->m_pWServer->Object_GetNameHash( pObj->m_iOwner ),
				liTeams, nTeams,
				NULL);
		}
		TArray<CWO_ScenePoint*> lpScenepoints = pSPM->Selection_Get();

		//Add selection to current scene points and update scene point tick
		m_lpScenePoints.Insertx(m_lpScenePoints.Len(), lpScenepoints.GetBasePtr(), lpScenepoints.Len());
		m_ScenePointTypes = 0;
		TAP<CWO_ScenePoint*> pSP = m_lpScenePoints;
		for (int i = 0; i < pSP.Len(); i++)
		{
			CWO_ScenePoint* pCur = pSP[i];
			m_ScenePointTypes |= pCur->GetType();
		}
		m_NextScenePointTick = Tick + m_pAI->GetAITicksPerSecond() * 3;
	}

	return m_lpScenePoints.Len();
};

#ifdef M_Profile
// Get all scenepoints that are potentially within the given distance and valid for us
int32 CAI_ActionHandler::DebugDrawScenepoints(bool _bUseable,fp32 _RangeDelimiter,CWObject_Room* _pRoom)
{
	M_ASSERT(m_pAI && m_pAI->m_pServer,"Code error: No ai or server in action handler");

	//NOTE: This method doesn't bother with override scenepoint at all. It's up to using
	//to handle that appropriately themselves.

	CWO_ScenePointManager * pSPM = m_pAI->GetScenePointManager();
	if (pSPM)
	{
		//Select scene points
		pSPM->Selection_Clear();
		if (_bUseable)
		{
			//Get own teams
			CWObject_Interface_AI* pObj = m_pAI->m_pGameObject;
			uint16 liTeams[32];
			uint nTeams = pObj->AI_GetTeams(liTeams, sizeof_buffer(liTeams));

			pSPM->Selection_AddChunk(m_pAI->GetBasePos(), 
				_RangeDelimiter, 
				pObj->GetNameHash(),
				pObj->m_pWServer->Object_GetNameHash(pObj->m_iOwner),
				liTeams, nTeams,
				_pRoom);
		}
		else
		{
			pSPM->Selection_AddAll();
		}
		TArray<CWO_ScenePoint*> lpScenepoints = pSPM->Selection_Get();
		int N = lpScenepoints.Len();
		for (int i = 0; i < N; i++)
		{
			if (lpScenepoints[i])
			{
				if ((lpScenepoints[i]->IsSpawned())&&(lpScenepoints[i]->IsValidDynamic()))
				{
					DebugDrawScenePoint(lpScenepoints[i],true,0,10.0f);
				}
				else
				{
					DebugDrawScenePoint(lpScenepoints[i],false,0,1.0f);
				}
			}
		}
		return(N);
	}
    
	return 0;
};
#endif


CWO_ScenePoint* CAI_ActionHandler::GetNearestEPSP(int _EPSPType,fp32 _RangeDelimiter)
{
	MSCOPESHORT(CAI_ActionHandler::GetNearestEPSP);
	M_ASSERT(m_pAI && m_pAI->m_pServer,"Code error: No ai or server in action handler");

	m_lpScenePoints.QuickSetLen(0);
	m_ScenePointTypes = 0;
	CWO_ScenePoint* pBestEPSP = NULL;
	CWObject_ScenePointManager * pSPM = m_pAI->GetScenePointManager();
	if (pSPM)
	{
		//Get own teams
		CWObject_Interface_AI* pObj = m_pAI->m_pGameObject;
		uint16 liTeams[32];
		uint nTeams = pObj->AI_GetTeams(liTeams, sizeof_buffer(liTeams));

		CVec3Dfp32 OurPos = m_pAI->GetBasePos();

		//Select scene points
		pSPM->Selection_Clear();
		if (m_spRestriction)
		{
			pSPM->Selection_AddChunk(OurPos, 
				_RangeDelimiter, 
				pObj->GetNameHash(),
				pObj->m_pWServer->Object_GetNameHash(pObj->m_iOwner),
				liTeams, nTeams,
				m_spRestriction->GetRoom());
		}
		else
		{
			pSPM->Selection_AddChunk(OurPos, 
				_RangeDelimiter, 
				pObj->GetNameHash(),
				pObj->m_pWServer->Object_GetNameHash(pObj->m_iOwner),
				liTeams, nTeams,
				NULL);
		}
		TArray<CWO_ScenePoint*> lpScenepoints = pSPM->Selection_Get();

		//Add selection to current scene points and update scene point tick
		m_lpScenePoints.Insertx(m_lpScenePoints.Len(), lpScenepoints.GetBasePtr(), lpScenepoints.Len());
		
		fp32 BestRangeSqr = 1000000000.0f;
		TAP<CWO_ScenePoint*> pSP = m_lpScenePoints;
		for (int i = 0; i < pSP.Len(); i++)
		{
			CWO_ScenePoint* pCur = pSP[i];
			if ((pCur)&&(pCur->GetType() & _EPSPType)&&(pCur->GetEPSPCount() > 0)&&(OurPos.DistanceSqr(pCur->GetPosition()) < BestRangeSqr))
			{
				pBestEPSP = pCur;
				BestRangeSqr = OurPos.DistanceSqr(pCur->GetPosition());
			}
		}
	}

	m_lpScenePoints.QuickSetLen(0);
	m_ScenePointTypes = 0;

	return(pBestEPSP);
};

// Get all scenepoints that are potentially within the given distance and valid for us
int32 CAI_ActionHandler::GetEnginepathSPCount(CWO_ScenePoint* _pCurSP)
{
	M_ASSERT(m_pAI && m_pAI->m_pServer,"Code error: No ai or server in action handler");

	m_lpScenePoints.QuickSetLen(0);
	m_ScenePointTypes = 0;
	CWO_ScenePointManager * pSPM = m_pAI->GetScenePointManager();
	if (pSPM)
	{
		return(pSPM->GetEnginepathSPCount(_pCurSP));
	}

	return 0;
};

CWO_ScenePoint* CAI_ActionHandler::GetNthEnginepathSP(CWO_ScenePoint* _pCurSP,int _i)
{
	CWO_ScenePointManager* pSPM = m_pAI->GetScenePointManager();
	if ((pSPM)&&(_pCurSP))
	{
		return(pSPM->GetNthEnginepathSP(_pCurSP,_i));
	}
	else
	{
		return(NULL);
	}
};

int32 CAI_ActionHandler::GetEnginepathID(CWO_ScenePoint* _pToSP)
{
	CWO_ScenePointManager* pSPM = m_pAI->GetScenePointManager();
	if ((pSPM)&&(_pToSP))
	{
		return(pSPM->GetEnginepathID(m_lScenepointHistory[0],_pToSP));
	}
	return(0);
};

//Get i'th scene point of those currently in scene point list
CWO_ScenePoint* CAI_ActionHandler::IterateScenePoints(int _i)
{
	if (!m_lpScenePoints.ValidPos(_i))
		return NULL;

	return m_lpScenePoints[_i];    
};

// *** Temporary solution until action/objective system is fully done
// Returns true if any CustomAttack action is valid on _pTarget
// Used by CAI_Core::CanAttack
bool CAI_ActionHandler::CanAttack(CAI_AgentInfo* _pTarget)
{
	int iOrgTarget = m_iTarget;
	m_iTarget = _pTarget->GetObjectID();
	int nActions = m_lpActions.GetNum();
	for (int iAct = 0; iAct < nActions; iAct++)
	{
		CAI_Action* pAction = m_lpActions.Get(iAct);
		if (pAction && (pAction->m_TypeFlags & CAI_Action::TYPE_ATTACK)&&(pAction->IsValid(_pTarget)))
		{
			m_iTarget = iOrgTarget;
			return(true);
		}
	}
	m_iTarget = iOrgTarget;

	return(false);
}

void CAI_ActionHandler::PrecacheEffect(CStr _sEffect) const
{
	if ((m_pAI)&&(m_pAI->m_pServer))
	{
		int iTemplate = m_pAI->m_pServer->GetMapData()->GetResourceIndex_Template(_sEffect);
	}
};

// Pass on an animation event to the first action that handle the event
// Returns true if it was handled and false if not.
// Note: If more than one action could handle the event only the highest prio one will actually do it
bool CAI_ActionHandler::AnimationEvent(int _iUser, int _iEvent)
{
	int nActions = m_lpActions.GetNum();
	for (int iAct = 0; iAct < nActions; iAct++)
	{
		CAI_Action* pAction = m_lpActions.Get(iAct);
		if ((pAction)&&(pAction->AnimationEvent(_iUser,_iEvent)))
		{
			return(true);
		}
	}
	return(false);
};

bool CAI_ActionHandler::HasAction(int _ActionType, bool _bActive)
{
	int nActions = m_lpActions.GetNum();
	for (int iAct = 0; iAct < nActions; iAct++)
	{
		CAI_Action* pAction = m_lpActions.Get(iAct);
		if (pAction && pAction->m_Type == _ActionType)
		{
			if ((_bActive)&&(!pAction->IsTaken()))
			{
				continue;
			}
			return(true);
		}
	}
	return(false);
};

bool CAI_ActionHandler::ExpireActions(int _ActionType,int _Delay)
{
	int nActions = m_lpActions.GetNum();
	bool bFoundOne = false;
	for (int iAct = 0; iAct < nActions; iAct++)
	{
		CAI_Action* pAction = m_lpActions.Get(iAct);
		if ((pAction)&&((_ActionType == CAI_Action::INVALID)||(pAction->m_Type == _ActionType)))
		{
			if (_Delay > 0)
			{
				pAction->ExpireWithDelay(_Delay);
			}
			else
			{
				pAction->SetExpirationExpired();
			}
			bFoundOne = true;
		}
	}
	return(bFoundOne);
};

bool CAI_ActionHandler::SetActionParameter(int _ActionType,int _Param,CStr _Val)
{
	int nActions = m_lpActions.GetNum();
	for (int iAct = 0; iAct < nActions; iAct++)
	{
		CAI_Action* pAction = m_lpActions.Get(iAct);
		if (pAction && pAction->m_Type == _ActionType)
		{
			pAction->SetParameter(_Param,_Val);
			return(true);
		}
	}
	return(false);
};

bool CAI_ActionHandler::SetActionParameter(int _ActionType, int _Param, int _Val)
{
	int nActions = m_lpActions.GetNum();
	for (int iAct = 0; iAct < nActions; iAct++)
	{
		CAI_Action* pAction = m_lpActions.Get(iAct);
		if (pAction && pAction->m_Type == _ActionType)
		{
			pAction->SetParameter(_Param,_Val);
			return(true);
		}
	}
	return(false);
};

bool CAI_ActionHandler::GetAction(int _Type,CAI_Action** _pAction)
{
	int nActions = m_lpActions.GetNum();
	for (int iAct = 0; iAct < nActions; iAct++)
	{
		CAI_Action* pAction = m_lpActions.Get(iAct);
		if ((pAction)&&((_Type == -1)||(_Type == pAction->m_Type)))
		{
			*_pAction = pAction;
			return(true);
		}
	}

	return(false);
};

// _iActionType will be paused for _PauseTicks ticks
// _PauseTicks=0: unpause, _PauseTicks=-1 pause forever(until unpaused)
bool CAI_ActionHandler::PauseAction(int32 _iActionType, int32 _PauseTicks)
{
	CAI_Action* pAction = NULL;
	if (_iActionType <= 0)
	{	// ALL actions
		int nActions = m_lpActions.GetNum();
		for (int iAct = 0; iAct < nActions; iAct++)
		{
			CAI_Action* pAction = m_lpActions.Get(iAct);
			if (pAction)
			{
				if (_PauseTicks > 0)
				{	// Pause for _PauseTicks
					pAction->ExpireWithDelay(_PauseTicks);
					return(true);
				}
				else if (_PauseTicks == 0)
				{	// Unpause
					pAction->m_LastStopTick = 0;
					return(true);
				}
				else
				{	// Pause 'forever' (_PauseTicks < 0)
					// 100 million ticks is about 57 days; forever for most games
					pAction->ExpireWithDelay(100000000);
					return(true);
				}
			}
		}
	}
	else
	{
		if (GetAction(_iActionType,&pAction))
		{
			if (_PauseTicks > 0)
			{	// Pause for _PauseTicks
				pAction->ExpireWithDelay(_PauseTicks);
				return(true);
			}
			else if (_PauseTicks == 0)
			{	// Unpause
				pAction->m_LastStopTick = 0;
				return(true);
			}
			else
			{	// Pause 'forever' (_PauseTicks < 0)
				// 100 million ticks is about 57 days; forever for most games
				pAction->ExpireWithDelay(100000000);
				return(true);
			}
		}
	}

	return(false);
};

void CAI_ActionHandler::RequestScenepoints()
{
	int nActions = m_lpActions.GetNum();
	for (int iAct = 0; iAct < nActions; iAct++)
	{
		CAI_Action* pAction = m_lpActions.Get(iAct);
		if ((pAction)&&(pAction->IsTaken()))
		{
			pAction->RequestScenepoints();
		}
	}
};

int CAI_ActionHandler::ReceiveCall(int _iCaller, CMat4Dfp32& _PosMat, int _iSoundVariant)
{
	int nActions = m_lpActions.GetNum();
	for (int iAct = 0; iAct < nActions; iAct++)
	{
		CAI_Action* pAction = m_lpActions.Get(iAct);
		if ((pAction)&&(pAction->m_Type == CAI_Action::MAKE_IDLE_CALL))
		{
			CAI_Action_IdleCall* pIdleCallAction = safe_cast<CAI_Action_IdleCall>(pAction);
			int iResult = pIdleCallAction->ReceiveCall(_iCaller, _PosMat, _iSoundVariant);
			return(iResult);
		}
	}

	return(CAI_Action_IdleCall::RESULT_FAIL);
};


bool CAI_ActionHandler::SetInvestigateObject(int32 _iObject)
{
	CAI_Action * pAction;
	m_iInvestigateObj = _iObject;
	if (GetAction(CAI_Action::INVESTIGATE,&pAction))
	{	// Handle the two cases of _iObject
		CAI_Action_Investigate* pInvestigateAction = safe_cast<CAI_Action_Investigate>(pAction);
		if (pInvestigateAction)
		{
			pInvestigateAction->SetInvestigateObject(_iObject);
			return(true);
		}
	}
	return(false);
};

//Get new random movement destination, at or closer to given distance preferrably close to the given heading
//If no such destination can be found, fail with CVec3Dfp32(_FP32_MAX).
CVec3Dfp32 CAI_ActionHandler::GetNewDestination(fp32 _Distance, fp32 _Heading, bool _bPathfind)
{
	if (!m_pAI || !m_pAI->IsValid())
		return CVec3Dfp32(_FP32_MAX);

	if ((_bPathfind)&&(m_pAI->m_PathFinder.GridPF()))
	{
		//Offset given heading randomly a bit
		fp32 rnd = Random;
		_Heading += (rnd < 0.3f) ? -rnd/2 : ((rnd > 0.7f) ? (1-rnd)/2 : 0); 

		fp32 Heading;
		fp32 Height;
		CVec3Dfp32 Pos;

		//The direction we primarily search is randomly determined
		int8 iDir = (Random > 0.5f) ? 1 : -1;

		//Check for positions at the given distance, but if this fails all around, check at closer distances etc...
		for (fp32 Distance = _Distance; Distance > 48; Distance /= 2)
		{		
			for (fp32 dHeading = 0; dHeading <= 0.5f; dHeading += 0.125f)
			{
				//Set heading
				Heading = _Heading + (iDir * dHeading);

				//Set height randomly.
				Height = (int)((rnd - 0.5f) * _Distance / Distance);

				CVec3Dfp32 Pos = m_pAI->GetBasePos() + (CVec3Dfp32(M_Cos(Heading*_PI2) , M_Sin(-Heading*_PI2), Height) * Distance);

				//Use closest graph node position if such can be found
				TThinArray<int16> lNodePos;
				if(m_pAI->m_PathFinder.GraphPF())
					m_pAI->m_PathFinder.GraphPF()->GetNodesAt(m_pAI->m_PathFinder.GetBlockNav()->GetGridPosition(Pos), &lNodePos);

				if (lNodePos.Len())
				{
					//Found node position, first is closest
					return m_pAI->m_PathFinder.GetBlockNav()->GetWorldPosition(m_pAI->m_PathFinder.GraphPF()->GetNodePos(lNodePos[0]));
				}
				else
				{
					//Couldn't find any nodes, try to just find traversable grid position
					Pos = m_pAI->m_PathFinder.GetPathPosition(Pos, 10, 5);

					//m_pAI->Debug_RenderWire(m_pAI->GetBasePos(), Pos, 0xffffff00, 2);//DEBUG

					//Did we find a (hopefully) suitable position?
					if (VALID_POS(Pos))
					{
						//Check that position is not on an edge or something
						int nGround = 0;
						CVec3Dint16 GridPos = m_pAI->m_PathFinder.GetBlockNav()->GetGridPosition(Pos);
						if (!m_pAI->m_PathFinder.GetBlockNav()->IsOutsideGridBoundaries(GridPos, 24, 32))
						{
							for (int x = -1; x < 2; x++)
								for (int y = -1; y < 2; y++)
								{
									if (m_pAI->m_PathFinder.GetBlockNav()->IsOnGround(GridPos + CVec3Dint16(x, y, 0), 1, false))
										nGround++;
								};
						};

						if (nGround > 5)
							//Jupp, success!
							return Pos;
					};
				}

				//Did not find any position in this pass, set up next pass
				//Toggle direction
				iDir = -iDir;
			};
		};

		//No position can be found
		return CVec3Dfp32(_FP32_MAX);
	}
	else
	{
		if (m_pAI->m_PathFinder.GridPF())
		{	
			// Apparently the grid is there but we shouldn't pathfind ie check groundtraversibility
			// We do a really simple
			// We don't have any scenepoints at the moment so we just generate random
			// movement directions (forward +- 90 degrees, 2-10m)
			CVec3Dfp32 OurPos,Dir,Goal,StopPos,Result;
			OurPos = m_pAI->GetBasePos();
			fp32 Heading;
			if (Random > 0.1f)
			{	// Random direction in the front hemicircle
				Heading = _Heading + (Random - 0.5f) * 0.5f;
			}
			else
			{	// Sometimes we do a fully random turn
				Heading = Random;
			}
			if (Heading > 1.0f) {Heading -= 1.0f;}
			if (Heading < -1.0f) {Heading += 1.0f;}
			fp32 Range = _Distance;
			Goal = OurPos + CVec3Dfp32(Range * QCos(Heading * _PI2), Range * QSin(-Heading * _PI2), 0);
			if (!m_pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,Goal,
				(int)m_pAI->GetBaseSize(),
				(int)m_pAI->GetHeight(),
				m_pAI->m_bWallClimb,
				(int)m_pAI->GetStepHeight(),
				(int)m_pAI->GetJumpHeight(),
				&StopPos))
			{
				return(StopPos);
			}
			else
			{
				return(Goal);
			}
		}
		else
		{
			//We're effectively blind without a navgrid, so just use position _Distance units away in specified _Heading
			return m_pAI->m_PathFinder.GetPathPosition(m_pAI->GetBasePos() + (CVec3Dfp32(M_Cos(_Heading*_PI2), M_Sin(-_Heading*_PI2), 0) * _Distance), 10, 0) ;
		}
	}
};

bool CAI_ActionHandler::CheckLookSPTraceline(CWO_ScenePoint* _pSP)
{
	if ((!_pSP)||(!(_pSP->GetType() & CWO_ScenePoint::LOOK))||(_pSP->GetNotraceFlag()))
	{
		return(false);
	}

	CVec3Dfp32 Start, End, Dir;
	Start = m_pAI->GetLookPos();
	End = _pSP->GetPosition();
	// Backtrack 16 units along ray
	// If that closer or closer we certainly can see the light
	Dir = End - Start;
	if (Dir.LengthSqr() <= Sqr(16.0f))
	{
		return(true);
	}
	Dir.Normalize();
	End -= Dir * 16.0f;
	int16 iLight = _pSP->GetConnectedLight();
	if (m_pAI->FastCheckLOS(Start,End,m_pAI->GetObjectID(),iLight))
	{
		return(true);
	}
	else
	{	// We cannot see the lighte despite being inside it's arc, disable for 30 secs
		_pSP->InvalidateScenepoint(m_pAI->m_pServer,m_pAI->GetAITicksPerSecond() * 30l);
#ifndef M_RTM
		// if (DebugTarget())
		{
			DebugDrawScenePoint(_pSP,true,kColorWhite,30.0f);
			m_pAI->Debug_RenderWire(Start,End,kColorWhite,30.0f);
		}
#endif
		return(false);
	}
};

// Checks wether the SP is OK for tracelines
bool CAI_ActionHandler::CheckSPTraceline(CWO_ScenePoint* _pSP,CVec3Dfp32 _TargetPos,int _iTarget)
{
	if (!_pSP)
	{
		return(false);
	}

	if ((_pSP->GetNotraceFlag())||(!(_pSP->GetType() & CWO_ScenePoint::DYNAMIC)))
	{	// SP is OK, we don't need to check
		return(true);
	}
	
	uint16 TacFlags = _pSP->GetTacFlags();
	CMat4Dfp32 SPMat = _pSP->GetPositionMatrix();
	CVec3Dfp32 Pos = SPMat.GetRow(3);
	CVec3Dfp32 Fwd = SPMat.GetRow(0);
	CVec3Dfp32 Left = SPMat.GetRow(1);
	CVec3Dfp32 Up = SPMat.GetRow(2);
	CVec3Dfp32 Start, End;

	if (_pSP->GetType() & CWO_ScenePoint::TACTICAL)
	{
		if (TacFlags)
		{
			if (TacFlags & CWO_ScenePoint::TACFLAGS_STAND)
			{
				Start = Pos + Up * CB_OFF_STAND;
				End = Start + Fwd * 64.0f;
			}
			else if (TacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)
			{
				Start = Pos + Up * CB_OFF_CROUCH;
				End = Start + Fwd * 64.0f;
			}
			else
			{	// Neither crouch nor stand: Not good
				return(false);
			}

			if (m_pAI->FastCheckLOS(Start,End,m_pAI->GetObjectID(),_iTarget))
			{	// We are not in cover: Not good
				_pSP->InvalidateScenepoint(m_pAI->m_pServer,m_pAI->GetAITicksPerSecond() * 30l);
#ifndef M_RTM
				// if (DebugTarget())
				{
					DebugDrawScenePoint(_pSP,true,kColorWhite,30.0f);
					m_pAI->Debug_RenderWire(Start,End,kColorRed,30.0f);
				}
#endif
				return(false);
			}
			else
			{
				return(true);
			}
		}
		else
		{
			if (_pSP->GetCrouchFlag())
			{
				Start = Pos + Up * CB_OFF_CROUCH;
				End = Start + Fwd * 64.0f;
			}
			else
			{
				Start = Pos + Up * CB_OFF_STAND;
				End = Start + Fwd * 64.0f;
			}
			if (!m_pAI->FastCheckLOS(Start,End,m_pAI->GetObjectID(),_iTarget))
			{	// We are in cover: Not good
				_pSP->InvalidateScenepoint(m_pAI->m_pServer,m_pAI->GetAITicksPerSecond() * 30l);
#ifndef M_RTM
				// if (DebugTarget())
				{
					DebugDrawScenePoint(_pSP,true,kColorWhite,30.0f);
					m_pAI->Debug_RenderWire(Start,End,kColorWhite,30.0f);
				}
#endif
				return(false);
			}
			else
			{
				return(true);
			}
		}
	}
	else if (_pSP->GetType() & CWO_ScenePoint::COVER)
	{
		if (_pSP->GetCrouchFlag())
		{
			Start = Pos + Up * CB_OFF_CROUCH;
			End = Start + Fwd * 64.0f;
		}
		else
		{
			Start = Pos + Up * CB_OFF_STAND;
			End = Start + Fwd * 64.0f;
		}
		if (m_pAI->FastCheckLOS(Start,End,m_pAI->GetObjectID(),_iTarget))
		{	// We are not in cover: Not good
			_pSP->InvalidateScenepoint(m_pAI->m_pServer,m_pAI->GetAITicksPerSecond() * 30l);
#ifndef M_RTM
			// if (DebugTarget())
			{
				DebugDrawScenePoint(_pSP,true,kColorWhite,30.0f);
				m_pAI->Debug_RenderWire(Start,End,kColorWhite,30.0f);
			}
#endif
			return(false);
		}
		else
		{
			return(true);
		}
	}
	else
	{
		return(true);
	}

	return(true);
};

/*
CWO_ScenePoint* CAI_ActionHandler::GetBestScenepointAlongRay(int _Type,fp32 _MaxAngle,CVec3Dfp32 _Start,CVec3Dfp32 _End,fp32 _MinRange,fp32 _MaxRange,CVec3Dfp32 _TargetPos)
{
	// Darklings can only use darkling scenepoints
	if (m_pAI->GetType() == CAI_Core::TYPE_DARKLING)
	{
		_Type |= CWO_ScenePoint::DARKLING;
		if (m_pAI->m_bWallClimb)
		{
			_Type |= CWO_ScenePoint::WALK_CLIMB;
		}
	}

	if (!m_pAI->m_bCanWallClimb)
	{
		_Type &= ~(CWO_ScenePoint::WALK_CLIMB | CWO_ScenePoint::JUMP_CLIMB);
	}
	else if (m_pAI->m_JumpMaxRange <= 0.0f)
	{
		_Type &= ~CWO_ScenePoint::JUMP_CLIMB;
	}

	CVec3Dfp32 RayDir = _End - _Start;
	CVec3Dfp32 OurPos = m_pAI->GetBasePos();
	RayDir.Normalize();
	fp32 CosAngle = M_Cos(_PI2 * _MaxAngle / 360.0f); //Value is assumed to be in degrees here
	int32 nScenePoints = GetScenePoints(_MaxRange);
	if (nScenePoints > 0)
	{
		CWO_ScenePoint* pBestScenepoint = NULL;
		fp32 BestFitness = -1000.0f;
		for (int i = 0; i < nScenePoints; i++)
		{
			fp32 CurRange,CurFitness;
			CVec3Dfp32 Pos,SPPos,SPUp;
			CMat4Dfp32 SPMat;
			CWO_ScenePoint* pCurScenePoint;
			pCurScenePoint = IterateScenePoints(i);	
			if (!pCurScenePoint) {continue;}
			if (!(pCurScenePoint->CheckType(_Type))) {continue;}
			int CurSPType = pCurScenePoint->GetType();
			if (pCurScenePoint->GetTacFlags()) {continue;}
			if (!pCurScenePoint->PeekRequest(m_pAI->GetObjectID(),m_pAI->m_pServer)) {continue;}
			if (!(CurSPType & _Type)) {continue;}
			// Ignore JUMP_CLIMB and WALK_CLIMB scenepoints unless we explicitly ask for them
			if (CurSPType & (CWO_ScenePoint::WALK_CLIMB | CWO_ScenePoint::JUMP_CLIMB))
			{
				if (!(_Type & (CWO_ScenePoint::WALK_CLIMB | CWO_ScenePoint::JUMP_CLIMB)))  {continue;}
			}
			// Ignore LOOK scenepoints unless we explicitly ask for them
			if (_Type & CWO_ScenePoint::LOOK)
			{
				if (!(CurSPType & CWO_ScenePoint::LOOK))
				{
					continue;
				}
				// We ask for ROAM we don't want SEARCH (and vice versa)
				if ((_Type & CWO_ScenePoint::ROAM)&&(!(CurSPType & CWO_ScenePoint::ROAM)))
				{
					continue;
				}
				if ((_Type & CWO_ScenePoint::SEARCH)&&(!(CurSPType & CWO_ScenePoint::SEARCH)))
				{
					continue;
				}
			}
			else if (CurSPType & CWO_ScenePoint::LOOK)
			{
				continue;
			}

			// Don't check InFrontArc if ROAM
			if ((CurSPType & (CWO_ScenePoint::TACTICAL | CWO_ScenePoint::COVER))&&(!(CurSPType & CWO_ScenePoint::ROAM)))
			{
				if ((!pCurScenePoint->GetNoTargetTacFlag())&&(!pCurScenePoint->InFrontArc(_TargetPos)))
				{
					continue;
				}
			}
			else if (CurSPType & (CWO_ScenePoint::TALK | CWO_ScenePoint::LOOK))
			{
				if (!pCurScenePoint->InFrontArc(OurPos))
				{
					continue;
				}
			}

			SPMat = pCurScenePoint->GetPositionMatrix();
			SPUp = SPMat.GetRow(2);
			SPPos = pCurScenePoint->GetPosition();
			CurRange = OurPos.Distance(SPPos);
			if (CurRange > _MaxRange) {continue;}
			if (CurRange < _MinRange) {continue;}
			// Only those wanting a jump SP should be able to use one
			if ((CurSPType & CWO_ScenePoint::JUMP_CLIMB)&&(!(CurSPType & CWO_ScenePoint::WALK_CLIMB)))
			{	// NOTE: We do NOT measure range from _Mat here as we may have called GetBestScenepoint with another pos
				// than our own. If we find a suitable scenepoint we shouldn't cull it for max range
				if (CurRange > m_pAI->m_JumpMaxRange) {continue;}
				if (CurRange < m_pAI->m_JumpMinRange) {continue;}
			}
			if (CurSPType & CWO_ScenePoint::DYNAMIC) {continue;}

			// A small random value to randomly select between two identical SPs
			// (or two checks of the same SP)
			CurFitness = Random * 0.01f;
			if (pCurScenePoint->GetPrio())
			{
				CurFitness += 10.0f;	// This one is soooo god!
			}
			if (pCurScenePoint->GetLowPrioFlag())
			{
				CurFitness -= 10.0f;	// This one is soooo bad!
			}
			
			if ((CurSPType & CWO_ScenePoint::WALK_CLIMB)&&(!(CurSPType & CWO_ScenePoint::JUMP_CLIMB)))
			{	// We prefer pure WALK_CLIMB somewhat
				CurFitness += 0.5f;
			}
			CurFitness += 1.0 - (CurRange / _MaxRange);
			if (CurRange > 16.0f)
			{	
				CVec3Dfp32 SPDir = (SPPos - _Start).Normalize();
				fp32 Dot = RayDir * SPDir;
				if (Dot < CosAngle) { continue; }
				CurFitness += Dot;
			}
			else
			{
				continue;
			}

			// Retarget position
			if ((VALID_POS(m_RetargetPos))&&(m_RetargetPos.DistanceSqr(SPPos) <= Sqr(AI_RETARGET_MAXRANGE)))
			{
				CurFitness += 10.0f * (2.0 - (m_RetargetPos.DistanceSqr(SPPos) / Sqr(AI_RETARGET_MAXRANGE)));
			}
			if (pCurScenePoint == m_pRetargetSP)
			{	// Retarget changes SP choice
				CurFitness -= 1.0f;
			}

			if (CurFitness > BestFitness)
			{
				pBestScenepoint = pCurScenePoint;
				BestFitness = CurFitness;
			}
		}

		if (pBestScenepoint)
		{	// If we've chosen a scenepoint close to m_RetargetPos we should clear it
			// so that further calls to GetBestScenepoint won't use it
			if (VALID_POS(m_RetargetPos))
			{
				CVec3Dfp32 SPPos = pBestScenepoint->GetPosition();
				// if (m_RetargetPos.DistanceSqr(SPPos) <= Sqr(AI_RETARGET_MAXRANGE))
				{
					m_RetargetPos = SPPos;
					m_pRetargetSP = pBestScenepoint;
					m_RetargetPosPF = m_pAI->m_PathFinder.SafeGetPathPosition(m_RetargetPos,4,2); 
					return(pBestScenepoint);
				}
			}
		}
	}


	return(NULL);
};
*/

//SP_PREFER_NEAR_TARGETPOS = 1,		// SPS near _TargetPos have higher fitness
//SP_REQUIRE_TARGET_CLOSER = 2,		// SP must be closer to target than we are now
//SP_RANDOM_RANGE			= 4,	// Pick randomly from range
//SP_PREFER_PLAYERFOV		= 8,	// Bias SPs in player FOV
//SP_REQUIRE_PLAYERFOV	= 16,		// Skip SPS outside player FOV
//SP_AVOID_TARGET			= 32,	// SPs are preferred if close
//SP_PREFER_HEADING		= 64,		// Prefer SPs in the callers present heading
//SP_AVOID_LIGHT			= 128,	// Darker SPs are better
//SP_PREFER_LIGHT			= 256,	// Brighter SPs are better
//SP_SAME_UP				= 512,	// SP must have close to same up as the supplied Matrix
//SP_DIFFERENT_UP			= 1024,	// SP must have radically different up than supplied Matrix
//SP_RECENT_FORBIDDEN		= 2048,	// Recent SPs cannot be chosen again
//SP_ENGINEPATH_SP		= 4096,		// Only consider enginepath SPs from on pSkip01
//SP_COMBATBEHAVIOURS		= 8192,	// Consider CB scenepoints (GetTacFlags() != 0)
// PSEUDO:
// SPs outside _MinRange.._MaxRange are skipped
// SPs within
// Fitness is 1.0 at _MinRange and 0.0 at _MaxRange
// Fitness is Random [0..1] if SP_RANDOM_RANGE
CWO_ScenePoint* CAI_ActionHandler::GetBestScenePoint(int _Type,uint _Flags,fp32 _MinRange,fp32 _MaxRange,
									const CMat4Dfp32& _Mat,
									const CVec3Dfp32& _TargetPos,fp32 _TargetMinRange,int _iTarget)
{
	//Check override scene point first
	CWO_ScenePoint* pOSP = CheckOverrideScenePoint(_Type, _MaxRange, 0, true);
	if ((pOSP)&&(pOSP->PeekRequest(m_pAI->m_pGameObject->m_iObject,m_pAI->m_pServer)))
	{	// We cannot return pOSP unless it's also within arc and arc range if any.
		if (pOSP->GetType() &
			(CWO_ScenePoint::TACTICAL | CWO_ScenePoint::COVER | CWO_ScenePoint::TALK | CWO_ScenePoint::LOOK))
		{
			if ((pOSP->GetNoTargetTacFlag())||(pOSP->InFrontArc(_TargetPos)))
			{
				return pOSP;
			}
		}
		else
		{	
			return pOSP;
		}
	}
	
	if (!(_Type & CWO_ScenePoint::LOOK))
	{
		if (m_pAI->m_Timer < m_LastGetBestSPTimer + AH_GETBESTSCENEPOINT_PERIOD)
		{
			return(NULL);
		}
		m_LastGetBestSPTimer = m_pAI->m_Timer;
	}

	// Darklings can only use darkling scenepoints
	if (m_pAI->GetType() == CAI_Core::TYPE_DARKLING)
	{
		_Type |= CWO_ScenePoint::DARKLING;
		if (m_pAI->m_bWallClimb)
		{
			_Type |= CWO_ScenePoint::WALK_CLIMB;
		}
	}

	if (!m_pAI->m_bCanWallClimb)
	{
		_Type &= ~(CWO_ScenePoint::WALK_CLIMB | CWO_ScenePoint::JUMP_CLIMB);
	}
	else if (m_pAI->m_JumpMaxRange <= 0.0f)
	{
		_Type &= ~CWO_ScenePoint::JUMP_CLIMB;
	}

	CVec3Dfp32 OurPos = _Mat.GetRow(3);
	CVec3Dfp32 TargetOff = _TargetPos - OurPos;	// Offset from us to target
	CVec3Dfp32 TargetDir = TargetOff;			// Normalized dir from us to target
	if (TargetDir.LengthSqr() > 0.001f)
	{
		TargetDir.Normalize();
	}
	else
	{	// Better safe than sorry
		TargetDir = CVec3Dfp32(1.0f,0.0f,0.0f);
	}
	CVec3Dfp32 OurLookDir = m_pAI->GetLookDir();	// Normalized look dir
	
	// Target awareness
	CAI_AgentInfo* pTarget = NULL;
	int Awareness = CAI_AgentInfo::DETECTED;
	if (_iTarget)
	{
		pTarget = m_pAI->m_KB.GetAgentInfo(_iTarget);
		Awareness = pTarget->GetCurAwareness();
	}

	//Find scene points
	int nScenePoints;
	if (_Flags & SP_ENGINEPATH_SP)
	{
		nScenePoints = GetEnginepathSPCount(m_lScenepointHistory[0]);
	}
	else
	{
		nScenePoints = GetScenePoints(_MaxRange);
	}

	if (nScenePoints > 0)
	{
		int32 nExamined = 0;
		CWO_ScenePoint* pBestScenepoint = NULL;
		fp32 BestFitness = -1000.0f;
		fp32 TargetRange = OurPos.Distance(_TargetPos);
		for (int i = 0; i < nScenePoints; i++)
		{
			fp32 CurRangeSqr,CurFitness;
			CVec3Dfp32 Pos,SPPos,SPUp;
			CMat4Dfp32 SPMat;
			CWO_ScenePoint* pCurScenePoint;
			if (_Flags & SP_ENGINEPATH_SP)
			{
				// pCurScenePoint = GetNthEnginepathSP(m_lScenepointHistory[0],(i+m_iScenePointFinger) % nScenePoints);
				pCurScenePoint = GetNthEnginepathSP(m_lScenepointHistory[0],i);
			}
			else
			{
				// pCurScenePoint = IterateScenePoints((i+m_iScenePointFinger) % nScenePoints);
				pCurScenePoint = IterateScenePoints(i);	
			}
			if (!pCurScenePoint) {continue;}
			if (!(pCurScenePoint->CheckType(_Type))) {continue;}
			int CurSPType = pCurScenePoint->GetType();
			if (!(CurSPType & _Type)) {continue;}
			// Ignore LOOK scenepoints unless we explicitly ask for them
			if (_Type & CWO_ScenePoint::LOOK)
			{
				if (!(CurSPType & CWO_ScenePoint::LOOK))
				{
					continue;
				}
				// We ask for ROAM we don't want SEARCH (and vice versa)
				if ((_Type & CWO_ScenePoint::ROAM)&&(!(CurSPType & CWO_ScenePoint::ROAM)))
				{
					continue;
				}
				if ((_Type & CWO_ScenePoint::SEARCH)&&(!(CurSPType & CWO_ScenePoint::SEARCH)))
				{
					continue;
				}
			}
			else if (CurSPType & CWO_ScenePoint::LOOK)
			{
				continue;
			}

			if ((!(_Flags & SP_COMBATBEHAVIOURS))&&(pCurScenePoint->GetTacFlags())) {continue;}
			if (!pCurScenePoint->PeekRequest(m_pAI->GetObjectID(),m_pAI->m_pServer)) {continue;}
			
			// Ignore JUMP_CLIMB and WALK_CLIMB scenepoints unless we explicitly ask for them
			if (CurSPType & (CWO_ScenePoint::WALK_CLIMB | CWO_ScenePoint::JUMP_CLIMB))
			{
				if (!(_Type & (CWO_ScenePoint::WALK_CLIMB | CWO_ScenePoint::JUMP_CLIMB)))  {continue;}
				if (!m_pAI->m_bWallClimb) {continue;}
			}

			// Best fitness wins
			/*
			nExamined++;
			if ((nExamined > AH_GETBESTSCENEPOINT_COUNT)&&(pBestScenepoint))
			{	// Break out of the loop, we're done
				break;
			}
			*/
			CurFitness = 0.0;
			SPMat = pCurScenePoint->GetPositionMatrix();	// SP matrix
			SPUp = pCurScenePoint->GetUp();					// SP upvector
			SPPos = pCurScenePoint->GetPosition();			// SP Pos
			CVec3Dfp32 SPLookDir = pCurScenePoint->GetDirection();		// SP Dir (Normalized)
			CVec3Dfp32 SPOff = SPPos - OurPos;				// Offset from us to SP
			CVec3Dfp32 SPOffDir = SPOff;						// Normalized offset from us to SP
			if (SPOffDir.LengthSqr() > 0.001f)
			{
				SPOffDir.Normalize();
			}
			else
			{	// Better safe than sorry
				SPOffDir = CVec3Dfp32(1.0f,0.0f,0.0f);
			}
			CVec3Dfp32 TargetSPOffDir = _TargetPos - SPPos;	// Normalized dir between SP and target
			if (TargetSPOffDir.LengthSqr() > 0.001f)
			{
				TargetSPOffDir.Normalize();
			}
			else
			{	// Better safe than sorry
				TargetSPOffDir = CVec3Dfp32(0.0f,0.0f,1.0f);
			}

			if ((_Type & CWO_ScenePoint::LOOK)&&(CurSPType & CWO_ScenePoint::ROAM))
			{
				// We cannot really use ROAM LOOK sps if their dir is way off ours
				if ((OurLookDir * SPOffDir <= 0.0f)&&(m_pAI->GetType() != CAI_Core::TYPE_DARKLING))
				{	// SP is more than 90 degrees off our current look
					continue;
				}
			}

			if (IsRestricted(SPPos))
			{
				continue;
			}

			// Don't check InFrontArc if ROAM
			if ((CurSPType & (CWO_ScenePoint::TACTICAL | CWO_ScenePoint::COVER))&&(!(CurSPType & CWO_ScenePoint::ROAM)))
			{
				// MUST take us closer to target
				if ((_Flags & SP_REQUIRE_TARGET_CLOSER)&&
					(Sqr(TargetRange) < SPPos.DistanceSqr(_TargetPos)))
				{
					continue;
				}


				bool bInsideArc = CheckScenePointTarget(pCurScenePoint, pTarget);
				if (!bInsideArc)
				{
					// Target is not inside arc as it should. The SP may still be valid however:
					// SP_PREFER_INSIDE_ARC: Give a really shitty fitness (-10.0 + SP dir dotted with SP -> Target dir)
					// SP_RETREAT_ARC: OurPos must be inside arc, give shitty fitness + (-10.0 + SP dir dotted with SP -> Target dir)
					//		SP must also be in the hemisphere away from target (that is why it's called SP_RETREAT_ARC)
					if (_Flags & SP_PREFER_INSIDE_ARC)
					{	// Helicopter fix
						CurFitness -= 10.0f;	// This one is soooo bad!
						CurFitness += SPLookDir * TargetSPOffDir;
					}
					else
					{
						if ((_Flags & SP_RETREAT_ARC)&&(pCurScenePoint->InFrontArc(OurPos))&&(SPOffDir * TargetDir <= 0.0f))
						{
							CurFitness -= 10.0f;
							CurFitness += SPLookDir * TargetSPOffDir;
						}
						else
						{
							continue;
						}
					}
				}

				if (pCurScenePoint->GetTacFlags())
				{
					CurFitness += 0.25f;
				}
			}
			else if (CurSPType & (CWO_ScenePoint::TALK | CWO_ScenePoint::LOOK))
			{
				if (!pCurScenePoint->InFrontArc(OurPos))
				{
					continue;
				}
			}

			if (_Flags & SP_PREFER_NEAR_TARGETPOS)
			{
				Pos = _TargetPos;
			}
			else
			{
				Pos = OurPos;
			}
			CurRangeSqr = Pos.DistanceSqr(SPPos);
			if (CurRangeSqr > Sqr(_MaxRange)) {continue;}
			// Only those wanting a jump SP should be able to use one
			if ((CurSPType & CWO_ScenePoint::JUMP_CLIMB)&&(!(CurSPType & CWO_ScenePoint::WALK_CLIMB)))
			{	// NOTE: We do NOT measure range from _Mat here as we may have called GetBestScenepoint with another pos
				// than our own. If we find a suitable scenepoint we shouldn't cull it for max range
				fp32 JumpRangeSqr = m_pAI->SqrDistanceToUs(SPPos);
				if (JumpRangeSqr > Sqr(m_pAI->m_JumpMaxRange)) {continue;}
				if (JumpRangeSqr < Sqr(m_pAI->m_JumpMinRange)) {continue;}
			}
			// MUST take us closer to target
			if ((_Flags & SP_REQUIRE_TARGET_CLOSER)&&
				(Sqr(TargetRange) < _TargetPos.DistanceSqr(SPPos)))
			{
				continue;
			}
			if ((!(_Flags & SP_COMBATBEHAVIOURS))&&(m_pRetargetSP != pCurScenePoint)&&(CurRangeSqr < Sqr(_MinRange))) {continue;}

			if (CurSPType & CWO_ScenePoint::DYNAMIC)
			{
				if (!pCurScenePoint->IsValidDynamic())
				{
					continue;
				}

				// First we check the up *** Do we need to do this Anton? ***
				if ((!(CurSPType & CWO_ScenePoint::LOOK))&&(SPUp * CVec3Dfp32(0.0f,0.0f,1.0f) <= 0.99f))
				{	// SP not properly aligned
					continue;
				}
				CVec3Dfp32 TargetHead = _TargetPos + CVec3Dfp32(0.0f,0.0f,32.0f);
				if (!pCurScenePoint->GetNotraceFlag())
				{
					if (!CheckSPTraceline(pCurScenePoint,TargetHead,_iTarget))
					{
						continue;
					}
				}
			}
			
			if (!pCurScenePoint->GetNotraceFlag())
			{
				if (CurSPType & CWO_ScenePoint::LOOK)
				{	// Traceline
					if (!CheckLookSPTraceline(pCurScenePoint))
					{
						continue;
					}
				}
			}

			// A small random value to randomly select between two identical SPs
			// (or two checks of the same SP)
			CurFitness += Random * 0.01f;
			if (pCurScenePoint->GetPrio())
			{
				CurFitness += 10.0f;	// This one is soooo god!
			}
			if (pCurScenePoint->GetLowPrioFlag())
			{
				CurFitness -= 10.0f;	// This one is soooo bad!
			}
			if ((CurSPType & CWO_ScenePoint::WALK_CLIMB)&&(!(CurSPType & CWO_ScenePoint::JUMP_CLIMB)))
			{	// We prefer pure WALK_CLIMB somewhat
				CurFitness += 0.5f;
			}
			if (_Flags & SP_RANDOM_RANGE)
			{
				CurFitness += Random;
			}
			else
			{
				if (_Flags & SP_PREFER_NEAR_TARGETPOS)
				{
					CurFitness += 3.0f * (1.0 - (CurRangeSqr / Sqr(_MaxRange)));
				}
				else
				{
					CurFitness += 1.0 - (CurRangeSqr / Sqr(_MaxRange));
				}
			}

			// Retarget position
			if (VALID_POS(m_RetargetPos))
			{
				if (m_RetargetPos.DistanceSqr(SPPos) > Sqr(32.0f))
				{
					CurFitness += Sqr(_MaxRange * 0.5f) / m_RetargetPos.DistanceSqr(SPPos);
				}
				else
				{
					CurFitness += Sqr(_MaxRange * 0.5f) / Sqr(32.0f);
				}
			}

			// We like the retarget SP as well
			if (pCurScenePoint == m_pRetargetSP)
			{
				CurFitness += 1.0f;
			}
			else
			{
				if (_Flags & SP_RECENT_FORBIDDEN)
				{
					if (_Type & CWO_ScenePoint::LOOK)
					{
						if ((pCurScenePoint == m_lLookpointHistory[0])||
							(pCurScenePoint == m_lLookpointHistory[1])||
							(pCurScenePoint == m_lLookpointHistory[2])||
							(pCurScenePoint == m_lLookpointHistory[3])||
							(pCurScenePoint == m_lLookpointHistory[4]))
						{
							continue;
						}
					}
					else
					{
						if ((pCurScenePoint == m_lScenepointHistory[0])||
							(pCurScenePoint == m_lScenepointHistory[1])||
							(pCurScenePoint == m_lScenepointHistory[2])||
							(pCurScenePoint == m_lScenepointHistory[3])||
							(pCurScenePoint == m_lScenepointHistory[4]))
						{
							continue;
						}
					}
					
				}
				else
				{
					fp32 historyFitnessAdd = 0.0f;
					if (_Type & CWO_ScenePoint::LOOK)
					{
						if (pCurScenePoint == m_lLookpointHistory[0])
						{	// Never the same LOOK SP twice in a row
							continue;
							// As choosing the same SP two times in a row is BAD we downgrade this one a lot
							// historyFitnessAdd -= 20.0f;
						}
						else if (pCurScenePoint == m_lLookpointHistory[1])
						{
							// As choosing the same SP two times in a row is BAD we downgrade this one a lot
							historyFitnessAdd -= 4.0f;
						}
						else if (pCurScenePoint == m_lLookpointHistory[2])
						{
							// As choosing the same SP two times in a row is BAD we downgrade this one a lot
							historyFitnessAdd -= 3.0f;
						}
						else if (pCurScenePoint == m_lLookpointHistory[3])
						{
							// As choosing the same SP two times in a row is BAD we downgrade this one a lot
							historyFitnessAdd -= 2.0f;
						}
						else if (pCurScenePoint == m_lLookpointHistory[4])
						{
							// As choosing the same SP two times in a row is BAD we downgrade this one a lot
							historyFitnessAdd -= 1.0f;
						}
					}
					else
					{
						if (pCurScenePoint == m_lScenepointHistory[0])
						{
							// As choosing the same SP two times in a row is BAD we downgrade this one a lot
							historyFitnessAdd -= 20.0f;
						}
						else if (pCurScenePoint == m_lScenepointHistory[1])
						{
							// As choosing the same SP two times in a row is BAD we downgrade this one a lot
							historyFitnessAdd -= 4.0f;
						}
						else if (pCurScenePoint == m_lScenepointHistory[2])
						{
							// As choosing the same SP two times in a row is BAD we downgrade this one a lot
							historyFitnessAdd -= 3.0f;
						}
						else if (pCurScenePoint == m_lScenepointHistory[3])
						{
							// As choosing the same SP two times in a row is BAD we downgrade this one a lot
							historyFitnessAdd -= 2.0f;
						}
						else if (pCurScenePoint == m_lScenepointHistory[4])
						{
							// As choosing the same SP two times in a row is BAD we downgrade this one a lot
							historyFitnessAdd -= 1.0f;
						}
						if ((CurSPType & CWO_ScenePoint::TACTICAL)&&(pCurScenePoint->GetTacFlags()))
						{	// A LOT less history reduction for Tactical scenepoints
							historyFitnessAdd *= 0.1f;
						}
					}
					CurFitness += historyFitnessAdd;
				}
			}
			if ((_Flags & (SP_PREFER_PLAYERFOV | SP_REQUIRE_PLAYERFOV))&&(!m_pAI->IsPlayerLookWithinAngle(45.0f,SPPos)))
			{
				if (_Flags & SP_REQUIRE_PLAYERFOV)
				{	// Skip if outside player FOV
					continue;
				}
				else
				{	// Less fitness if outside player FOV
					CurFitness -= 1.0f;
				}
			}
			if (_Flags & SP_AVOID_TARGET)
			{	// Determine if/how much the SP is in front of the target
				if ((SPOff.LengthSqr() >= Sqr(16.0f))&&(TargetOff.LengthSqr() >= Sqr(16.0f)))
				{
					fp32 CosAngle = SPOffDir * TargetDir;
					// Within +-30 degrees of target?
					if (CosAngle >= 0.86f)
					{
						if (_Type & CWO_ScenePoint::COVER)
						{
							continue;
							// CurFitness -= 5.0f;
						}
						else
						{	
							CurFitness -= 5.0f;
						}
					}
				}
			}
			
			if ((_iTarget)&&(_Flags & SP_PREFER_FLANK))
			{
				CAI_Core* pTargetAI = m_pAI->GetAI(_iTarget);
				if (pTargetAI)
				{
					CMat4Dfp32 TargetMat;
					m_pAI->GetBaseMat(TargetMat);
					if (Abs(TargetMat.GetRow(1) * TargetDir) >= 0.707f)
					{
						CurFitness += 1.0f;
					}
				}
			}

			// Better fitness if search SP arc does not contain the target
			if ((CurSPType & CWO_ScenePoint::SEARCH)&&(pCurScenePoint->InFrontArc(_TargetPos)))
			{
				CurFitness += 1.0f;
			}

			// SPS in callers front 180 degrees are preferred
			if (_Flags & SP_PREFER_HEADING)
			{
				if (_Type & CWO_ScenePoint::SEARCH)
				{	// We prefer SPs that face more or less as we face
					CurFitness += SPLookDir * OurLookDir;
				}
				// We also prefer SPs that are in our own front 180 degrees
				if (OurLookDir * SPOffDir > 0.0f)
				{
					CurFitness += 1.0f;
				}
				// We prefer JUMP SPs that are oriented similarly to us
				if (CurSPType & CWO_ScenePoint::JUMP_CLIMB)
				{
					CurFitness += TargetDir * SPUp;
				}
			}

			// If a collider is nearby we consider all SPs on his half of the world
			if (m_pAI->m_KB.GetCollider())
			{
				CAI_Core* pCollider = m_pAI->GetAI(m_pAI->m_KB.GetCollider());
				if (pCollider)
				{
					CVec3Dfp32 CollPos = pCollider->GetBasePos();
					if (m_pAI->SqrDistanceToUs(CollPos) <= Sqr(AI_COLL_AVOID_RANGE))
					{
						CVec3Dfp32 DirNorm = (CollPos - OurPos).Normalize();
						if ((SPPos - OurPos) * DirNorm >= 0.0f)
						{	// This one is bad but not as bad as the recent one (which is -20.0f)
							CurFitness -= 5.0f;
						}
					}
				}
			}

			if (CurFitness > BestFitness)
			{	// We do this late as it is fairly expensive (for Darklings at least)
				if (!m_pAI->CheckScenepoint(pCurScenePoint)) {continue;}
			}
			if (_Flags & SP_AVOID_LIGHT)
			{
				CurFitness -= pCurScenePoint->GetLight();
			}
			if (_Flags & SP_PREFER_LIGHT)
			{
				CurFitness += pCurScenePoint->GetLight();
			}

			if (CurFitness > BestFitness)
			{
				pBestScenepoint = pCurScenePoint;
				BestFitness = CurFitness;
			}
		}
		// m_iScenePointFinger = (m_iScenePointFinger + nExamined) % nScenePoints;

		if (pBestScenepoint)
		{
			// If we've chosen a scenepoint close to m_RetargetPos we should clear it
			// so that further calls to GetBestScenepoint won't use it
			if (VALID_POS(m_RetargetPos))
			{
				CVec3Dfp32 SPPos = pBestScenepoint->GetPosition();
				if (m_RetargetPos.DistanceSqr(SPPos) <= Sqr(AI_RETARGET_MAXRANGE))
				{
					m_RetargetPos = SPPos;
					m_pRetargetSP = pBestScenepoint;
					m_RetargetPosPF = m_pAI->m_PathFinder.SafeGetPathPosition(m_RetargetPos,4,2); 
				}
			}
			return(pBestScenepoint);
		}
		else
		{
			return NULL;
		}
	}

	//No suitable scene points found
	return NULL;
};

#ifdef M_Profile
void CAI_ActionHandler::DebugDrawScenePoint(CWO_ScenePoint* _pSP,bool _bActivated,uint32 _Color,fp32 _Duration)
{
	if ((_pSP)&&(m_pAI->m_pServer)&&(m_pAI->m_pServer->Debug_GetWireContainer()))
	{
		uint16 Type = _pSP->GetType();
		uint32 Color = kColorWhite;
		if (_Color)
		{
			Color = _Color;
		}
		else
		{
			if (Type & CWO_ScenePoint::TACTICAL)
			{
				Color = kSPColorTactical;
			}
			else if (Type & CWO_ScenePoint::COVER)
			{
				Color = kSPColorCover;
			}
			else if (Type & CWO_ScenePoint::SEARCH)
			{
				Color = kSPColorSearch;
			}
			else if (Type & CWO_ScenePoint::ROAM)
			{
				Color = kSPColorRoam;
			}
			else if (Type & CWO_ScenePoint::TALK)
			{
				Color = kSPColorTalk;
			}
			else if (Type & CWO_ScenePoint::LOOK)
			{
				Color = kSPColorLook;
			}
			if (Type & CWO_ScenePoint::DARKLING)
			{
				Color = kSPColorDarkling;
			}
		}
		

		CVec3Dfp32 Pos = m_pAI->GetBasePos();
		CVec3Dfp32 SPPos = _pSP->GetPosition(false);
		CMat4Dfp32 SPMat = _pSP->GetPositionMatrix();
		CVec3Dfp32 Dir = SPMat.GetRow(0) * 32.0f;
		CVec3Dfp32 Up = SPMat.GetRow(2) * 100.0f;
		CVec3Dfp32 Mid = SPPos+Up*0.5f;
		bool bFade = true;
		if (_Duration < 1.0f)
		{	// Line from user to SP (8 units above)
			m_pAI->m_pServer->Debug_RenderWire(Pos+CVec3Dfp32(0,0,8),SPPos+CVec3Dfp32(0,0,8),Color, _Duration, bFade);
		}
		if (_bActivated)
		{
			m_pAI->m_pServer->Debug_RenderWire(SPPos,SPPos+Up,Color, _Duration, bFade);
			m_pAI->m_pServer->Debug_RenderWire(Mid,Mid+Dir,Color, _Duration, bFade);
		}
		else
		{
			m_pAI->m_pServer->Debug_RenderWire(SPPos,Mid,Color, _Duration, bFade);
			m_pAI->m_pServer->Debug_RenderWire(Mid,Mid+Dir,Color, _Duration, bFade);
		}
		// Where's the pathPos?
		m_pAI->m_pServer->Debug_RenderVertex(_pSP->GetPathPosition(m_pAI->m_pServer),Color, _Duration, bFade);
		// Draw tacflags
		if ((_bActivated)&&(_pSP->GetTacFlags()))
		{
			Dir = SPMat.GetRow(0);
			Up = SPMat.GetRow(2);
			CVec3Dfp32 Left = SPMat.GetRow(1);	
			uint16 TacFlags = _pSP->GetTacFlags();
			CVec3Dfp32 Start,End;
			if (TacFlags & CWO_ScenePoint::TACFLAGS_CROUCH)
			{
				// Draw a cross
				Start = SPPos + Up * CB_OFF_CROUCH;
				m_pAI->m_pServer->Debug_RenderWire(Start-Dir*4.0f,Start+Dir*4.0f,Color, _Duration, bFade);
				m_pAI->m_pServer->Debug_RenderWire(Start-Left*4.0f,Start+Left*4.0f, Color, _Duration, bFade);
				m_pAI->m_pServer->Debug_RenderWire(Start-Up*4.0f,Start+Up*4.0f, Color, _Duration, bFade);
				if (TacFlags & CWO_ScenePoint::TACFLAGS_LEFT)
				{
					Start = SPPos + Up * CB_OFF_CROUCH + Left * CB_OFF_LEAN;
					End = Start + Dir * 16.0f;
					m_pAI->m_pServer->Debug_RenderWire(Start,End, Color, _Duration, bFade);
				}
				if (TacFlags & CWO_ScenePoint::TACFLAGS_RIGHT)
				{
					Start = SPPos + Up * CB_OFF_CROUCH - Left * CB_OFF_LEAN;
					End = Start + Dir * 16.0f;
					m_pAI->m_pServer->Debug_RenderWire(Start,End, Color, _Duration, bFade);
				}
				if ((!(TacFlags & CWO_ScenePoint::TACFLAGS_STAND))&&(TacFlags & CWO_ScenePoint::TACFLAGS_POPUP))
				{
					Start = SPPos + Up * CB_OFF_POPUP;
					End = Start + Dir * 16.0f;
					m_pAI->m_pServer->Debug_RenderWire(Start,End, Color, _Duration, bFade);
				}
			}
			if (TacFlags & CWO_ScenePoint::TACFLAGS_STAND)
			{
				// Draw a cross
				Start = SPPos + Up * CB_OFF_STAND;
				m_pAI->m_pServer->Debug_RenderWire(Start-Dir*4.0f,Start+Dir*4.0f, Color, _Duration, bFade);
				m_pAI->m_pServer->Debug_RenderWire(Start-Left*4.0f,Start+Left*4.0f, Color, _Duration, bFade);
				m_pAI->m_pServer->Debug_RenderWire(Start-Up*4.0f,Start+Up*4.0f, Color, _Duration, bFade);
				if (TacFlags & CWO_ScenePoint::TACFLAGS_LEFT)
				{
					Start = SPPos + Up * CB_OFF_STAND + Left * CB_OFF_LEAN;
					End = Start + Dir * 16.0f;
					m_pAI->m_pServer->Debug_RenderWire(Start,End, Color, _Duration, bFade);
				}
				if (TacFlags & CWO_ScenePoint::TACFLAGS_RIGHT)
				{
					Start = SPPos + Up * CB_OFF_STAND - Left * CB_OFF_LEAN;
					End = Start + Dir * 16.0f;
					m_pAI->m_pServer->Debug_RenderWire(Start,End, Color, _Duration, bFade);
				}
			}
		}
	}
}
#endif

//Check if there are any spawned scenepoints of given type fulfilling the optional criteria.
bool CAI_ActionHandler::CheckScenePoints(int _Type, fp32 _MaxRange, fp32 _MinRange)
{
	//Should perhaps optimize this with a cache or something...
    int nSP = GetScenePoints(_MaxRange);

	//Make non-destructive check for override scene point
	if (CheckOverrideScenePoint(_Type, _MaxRange, _MinRange))
		return true;

	//Check each scene point until we find one matching the criteria
	CWO_ScenePoint * pSP;
	fp32 MaxRangeSqr = Sqr(_MaxRange);
	fp32 MinRangeSqr = Sqr(_MinRange);
    for (int i = 0; i < nSP; i++)
	{
		//Check existance and type
		pSP = IterateScenePoints(i);
		if (pSP && (pSP->CheckType(_Type)))
		{
			//Check maxrange
			if (m_pAI->SqrDistanceToUs(pSP->GetPosition()) > MaxRangeSqr)
				continue;
			//Check minrange
			if (m_pAI->SqrDistanceToUs(pSP->GetPosition()) < MinRangeSqr)
				continue;
			
			//Found scene point matching criteria!
			return true;
		}
	}

	//Couldn't find any matching scene points
	return false;
};

bool CAI_ActionHandler::HandleSitFlag(CWO_ScenePoint* _pScenePoint)
{
	if ((!_pScenePoint)||(!_pScenePoint->GetSitFlag())||(m_pAI->m_bBehaviourRunning))
	{
		return(false);
	}
	
	CVec3Dfp32 OurPos = m_pAI->GetBasePos();
	CVec3Dfp32 Pos = _pScenePoint->GetPosition();
	CVec3Dfp32 PathPos = _pScenePoint->GetPathPosition(m_pAI->m_pServer);
	fp32 RangeSqr = Pos.DistanceSqr(OurPos);
	if ((RangeSqr > Sqr(32.0f))&&(OurPos.DistanceSqr(PathPos) > Sqr(32.0)))
	{
		m_pAI->SetObjColl(true);
		if (RangeSqr <= Sqr(64.0f))
		{	// Turn off walkstops
			m_pAI->NoStops();
		}
		return(false);
	}
	// Set object collision flag to false
	m_pAI->ResetStuckInfo();
	m_pAI->SetObjColl(false);
	if (Pos.DistanceSqr(OurPos) <= Sqr(8.0f))
	{
		m_pAI->m_DeviceMove.Use();
		m_pAI->OnTrackDir(_pScenePoint->GetDirection(),0,20,false,false);
		if (m_pAI->GetLookDir() * _pScenePoint->GetDirection() >= 0.95f)
		{
			m_pAI->SetPerfectOrigin(_pScenePoint);
			if (m_pAI->GetLookDir() * _pScenePoint->GetDirection() >= 0.99f)
			{
				return(true);
			}
		}
		return(false);
	}
	m_pAI->AddMoveTowardsPositionCmd(Pos, m_pAI->m_ReleaseIdleSpeed);

	return(false);
};

// NOTE When/if we start using scene point indices and call all scenepoint stuff through CAI_ActionHandler
// we should change _pScenePoint into its index
bool CAI_ActionHandler::ActivateScenePoint(CWO_ScenePoint* _pScenePoint,int _Prio,CMat4Dfp32* _pPOMat)
{
	if (!_pScenePoint)
	{
		return(false);
	}

	if (_pScenePoint->Activate(m_pAI->m_pGameObject->m_iObject,m_pAI->m_pServer, m_pAI->GetScenePointManager()))
	{
		int nDurationTicks = (int)(_pScenePoint->GetBehaviourDuration() * m_pAI->GetAITicksPerSecond());
		if ((m_pAI->m_pGameObject->AI_IsOnWall())||(_pScenePoint->GetType() & (CWO_ScenePoint::JUMP_CLIMB | CWO_ScenePoint::WALK_CLIMB)))
		{
	
			if (nDurationTicks <= 0)
			{
				m_pAI->TempEnableWallClimb((int)(60.0f * m_pAI->GetAITicksPerSecond()));
			}
			else
			{
				m_pAI->TempEnableWallClimb(nDurationTicks+10);
			}
		}

		// if ((_pScenePoint->GetBehaviour() != 0)&&(m_pAI->IsBehaviourSupported(_pScenePoint->GetBehaviour())))
		int32 iBehaviour = _pScenePoint->GetBehaviour();
		if (iBehaviour != 0)
		{
			int Flags = CAI_Core::BEHAVIOUR_FLAGS_LOOP;
			if (_pScenePoint->PlayOnce())
			{
				Flags &= ~CAI_Core::BEHAVIOUR_FLAGS_LOOP;
				nDurationTicks = 0;
			}
			if ((_pScenePoint->GetSqrRadius() <= 0.0f)||(_pPOMat))
			{
				Flags |= CAI_Core::BEHAVIOUR_FLAGS_PO;
			}
			if (_pPOMat)
			{
				m_pAI->SetWantedBehaviour2(iBehaviour,_Prio, Flags, nDurationTicks, _pScenePoint, _pPOMat);
			}
			else
			{
				m_pAI->SetWantedBehaviour2(iBehaviour,_Prio, Flags, nDurationTicks, _pScenePoint);
			}
		}
		else if (_pScenePoint->GetCrouchFlag())
		{	// Only crouch when crouch flag is set and there either are no tacflags or no CBs in AI
			if ((!_pScenePoint->GetTacFlags())||(m_pAI->m_lCombatBehaviour.Len() == 0))
			{
				m_pAI->m_DeviceStance.Crouch(true);
			}
		}
		if (_pScenePoint == m_pRetargetSP)
		{
			INVALIDATE_POS(m_RetargetPos);
			INVALIDATE_POS(m_RetargetPosPF);
			m_pRetargetSP = NULL;
		}
		// Finally check if we became script-mounted
		CMat4Dfp32 Mat;
		if (m_pAI->IsMounted(Mat))
		{	// Supply CAI_ScriptHandler with a scenepoint to check against
			// Check duration and unmount when time is up
			// Check arc for TACTICAL and SEARCH
			m_pAI->m_Script.SetMountScenepoint(_pScenePoint);
		}

		// Update m_lScenepointHistory
		if (_pScenePoint->GetType() & CWO_ScenePoint::LOOK)
		{	// LOOK scenepoints has their own history
			UpdateLookpointHistory(_pScenePoint);
		}
		else
		{
			UpdateScenepointHistory(_pScenePoint);
		}
		return(true);
	}
	else
	{
		return(false);
	}
}

bool CAI_ActionHandler::CheckScenePointTarget(CWO_ScenePoint* _pScenePoint, int _iTarget)
{
	CAI_AgentInfo* pTarget = m_pAI->m_KB.GetAgentInfo(_iTarget);
	return(CheckScenePointTarget(_pScenePoint, pTarget));
};

bool CAI_ActionHandler::CheckScenePointTarget(CWO_ScenePoint* _pScenePoint, CAI_AgentInfo* _pTarget)
{
	if (_pScenePoint)
	{
		int Awareness = CAI_AgentInfo::NONE;
		CVec3Dfp32 Pos = m_pAI->GetBasePos();
		if (_pTarget)
		{
			Awareness = _pTarget->GetCurAwareness();
			Pos = _pTarget->GetSuspectedPosition();
		}
		switch(Awareness)
		{
		case CAI_AgentInfo::SPOTTED:
			{	// Must enforce arc
				if (_pScenePoint->InFrontArc(Pos))
				{
					return(true);
				}
				else
				{
					return(false);
				}
			}
			break;
		case CAI_AgentInfo::DETECTED:
			{
				if (_pScenePoint->GetNoTargetTacFlag())
				{
					if (Pos.DistanceSqr(_pScenePoint->GetPosition()) < _pScenePoint->GetSqrArcMinRange())
					{
						return(false);
					}
					else
					{
						return(true);
					}
				}
				else
				{
					if (_pScenePoint->InFrontArc(Pos))
					{
						return(true);
					}
					else
					{
						return(false);
					}
				}
			}
			break;
		default:
			{	// NOTICED or NONE Awareness
				if (_pScenePoint->GetNoTargetTacFlag())
				{
					return(true);
				}
				else
				{
					return(false);
				}
			}
			break;
		}
	}
	else
	{
		return(false);
	}
};

bool CAI_ActionHandler::AlignScenepoint(CWO_ScenePoint* _pScenePoint,int _Prio)
{
	if ((!_pScenePoint)||(m_pAI->m_bBehaviourRunning))
	{
		return(true);
	}

	CVec3Dfp32 OurPos = m_pAI->GetBasePos();
	CVec3Dfp32 Pos = _pScenePoint->GetPosition();
	CVec3Dfp32 PathPos = _pScenePoint->GetPathPosition(m_pAI->m_pServer);
	fp32 RangeSqr = Pos.DistanceSqr(OurPos);
	fp32 RadiusSqr = Max(_pScenePoint->GetSqrRadius(),Sqr(8.0f));
	CVec3Dfp32 Dir = m_pAI->GetLookDir();
	bool bIsAligned = false;
	bool bIsThere = false;
	bool bSitFlag = _pScenePoint->GetSitFlag();
	if ((RangeSqr <= Sqr(32.0f))||(OurPos.DistanceSqr(PathPos) <= Sqr(32.0))||(RangeSqr <= RadiusSqr))
	{
		if (bSitFlag)
		{	// Turn off collisions with furniture etc
			m_pAI->ResetStuckInfo();
			m_pAI->SetObjColl(false);
		}

		if (_pScenePoint->IsAt(OurPos))
		{
			m_pAI->m_DeviceMove.Use();
			bIsThere = true;
		}

		if (_pScenePoint->IsAligned(Dir))
		{
			bIsAligned = true;
		}

		// Start aligning
		if ((!bIsAligned)&&((bIsThere)||(_pScenePoint->GetType() & CWO_ScenePoint::TACTICAL)||(RangeSqr <= 2 * RadiusSqr)))
		{	// Some looking to do
			if (_Prio > CAI_Action::PRIO_IDLE)
			{
				m_pAI->OnTrackDir(_pScenePoint->GetDirection(),0,10,false,false);
			}
			else
			{
				m_pAI->OnTrackDir(_pScenePoint->GetDirection(),0,20,false,false);
			}
		}

		if (RadiusSqr <= Sqr(8.0))
		{
			m_pAI->ResetStuckInfo();
			if (bIsThere)
			{
				if (m_pAI->GetLookDir() * _pScenePoint->GetDirection() >= 0.95f)
				{
					m_pAI->SetPerfectOrigin(_pScenePoint);
				}
			}
			else if (bSitFlag)
			{
				m_pAI->AddMoveTowardsPositionCmd(Pos, m_pAI->m_ReleaseIdleSpeed);
			}

			// Turn the lower body?
			if ((_pScenePoint->GetBehaviour())||(_pScenePoint->GetTacFlags()))
			{
				if (bIsAligned)
				{
					/*
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
					if (pCD->m_TurnCorrectionTargetAngle > 0.01f)
					{
						pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_DISABLETURNCORRECTION,1);
						return(false);
					}
					else
					{
						pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_DISABLETURNCORRECTION,0);
						return(bIsThere);
					}
					*/
				}
			}
		}
		if ((bIsThere)&&(bIsAligned))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
	else
	{	// Too far away for aligning
		// It's OK  to collide with furniture etc (again)
		m_pAI->SetObjColl(true);
		if (RangeSqr <= Sqr(64.0f))
		{
			if (RadiusSqr <= Sqr(32.0f))
			{	// No walkstops
				m_pAI->NoStops();
			}
		}
		return(false);
	}


	return(false);
};

bool CAI_ActionHandler::RequestScenePoint(CWO_ScenePoint* _pScenePoint,uint8 _Prio)
{
	if (!_pScenePoint)
	{
		return(false);
	}

	//Clear override scene point whenever we try to request it (thus we only have one try for it)
	if (_pScenePoint == m_pOverrideScenePoint)
	{
		m_bClearOverrideScenePoint = true;
	}
	else if ((m_pOverrideScenePoint)&&(m_pOverrideScenePoint->CheckType(_pScenePoint->GetType())))
	{
		return(false);
	}

	if (m_bCheckValidTeams)
	{
		CWObject_Interface_AI* pObj = m_pAI->m_pGameObject;
		uint32 OwnerName = m_pAI->m_pServer->Object_GetNameHash(pObj->m_iOwner);
		uint16 liTeams[16];
		uint nTeams = pObj->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
		if (!_pScenePoint->IsValid(m_pAI->m_pServer, pObj->GetNameHash(), OwnerName, liTeams, nTeams))
		{
			return(false);
		}
	}

	if (_pScenePoint->Request(m_pAI->m_pGameObject->m_iObject,m_pAI->m_pServer, m_pAI->GetScenePointManager(), 0, _Prio))
	{
		//We got scene point. Add it to currently held scene points (this can create duplicate entries, but it doesn't matter)
		m_lpCurrentHeldScenePoints.Add(_pScenePoint);

		//Remove entry from previously held scenepoints (last-list shouldn't hold any duplicates)
		m_lpLastHeldScenePoints.Remove(m_lpLastHeldScenePoints.Find(_pScenePoint));

		if (_pScenePoint->GetAllowNearFlag())
		{
			m_pAI->SetCharacterCollisions(false);
		}
		//Request successful
 		return true;
	}
	else
	{
		//Note that even if scene point was on previously held scenepoints list and failed to get it now, we still might get it later this tick
		//Request failed
		return false;
	}
}

void CAI_ActionHandler::ReleaseSPs(int32 _Duration)
{
	m_pAI->m_pBehaviourScenePoint = NULL;
	m_pAI->ClearCombatBehaviours();

	//Any scenepoints still on last held scene point list was not taken this frame and should therefore be released
	for (int i = 0; i < m_lpLastHeldScenePoints.Length(); i++)
	{
		if (m_lpLastHeldScenePoints.IsValid(i))
		{
			m_lpLastHeldScenePoints.Get(i)->InvalidateScenepoint(m_pAI->m_pServer,_Duration);
		}
	}

	//Any scenepoints still on last held scene point list was not taken this frame and should therefore be released
	for (int i = 0; i < m_lpCurrentHeldScenePoints.Length(); i++)
	{
		if (m_lpCurrentHeldScenePoints.IsValid(i))
		{
			m_lpCurrentHeldScenePoints.Get(i)->InvalidateScenepoint(m_pAI->m_pServer,_Duration);
		}
	}
};

// Called by REMOVE_TEAM impulse to force actionhandler to reconsider what scenepoints to keep holding
void CAI_ActionHandler::OnRemoveTeam(int32 _iTeam)
{
	// First we force the ai to get new scenepoints
	m_NextScenePointTick = 0;
	m_bCheckValidTeams = true;
	RequestScenepoints();
};

//Refreshes held scenepoint info and releases any scenepoints we don't hold anymore
void CAI_ActionHandler::OnRefreshScenePointStatus()
{
	//This method should be called at end of every CAI_Core::OnTakeAction	

	//Clear override scenepoint if so called for
	if (m_bClearOverrideScenePoint)
	{
		m_pOverrideScenePoint = NULL;
		m_bClearOverrideScenePoint = false;
	}

	//Any scenepoints still on last held scene point list was not taken this frame and should therefore be released
	for (int i = 0; i < m_lpLastHeldScenePoints.Length(); i++)
	{
		if (m_lpLastHeldScenePoints.IsValid(i))
		{
			CWO_ScenePoint* curSP = m_lpLastHeldScenePoints.Get(i);
			if (m_pAI->GetStealthTension() < CAI_Core::TENSION_COMBAT)
			{	// Clear activated flag
				curSP->HandleCombatReleaseFlag();
			}
			curSP->Release(m_pAI->m_pGameObject->m_iObject, m_pAI->m_pServer, m_pAI->GetScenePointManager());
			CAI_Core* releaserAI = m_pAI->GetAI(m_pAI->GetObjectID());	// Bizarre!
			if (releaserAI)
			{
				if (curSP->GetCrouchFlag())
				{
					releaserAI->m_DeviceStance.Crouch(false);
				}
				if ((curSP->GetBehaviour() != 0)&&(curSP->GetBehaviour() == releaserAI->m_iBehaviourRunning))
				{
					releaserAI->StopBehaviour(CAI_Core::BEHAVIOUR_STOP_NORMAL,releaserAI->m_BehaviourPrioClass);
				}
				if (curSP->GetSitFlag())
				{	// This can be a tad risky
					releaserAI->SetObjColl(true);
				}
			}
		}
	}

	//Transfer all scenepoints on current to cleared last list. Remove duplicates.
	m_lpLastHeldScenePoints.Clear();

	if (m_bCheckValidTeams)
	{
		m_bCheckValidTeams = false;	// We set the flag to false BEFORE releasing any SPs as the release scripts may call REMOVE_TEAM
		// which we now handle on the next tick. Were we to set it to false after looping we might miss some SPs
		// (clever bastards the scripters, eh?)
		// If scripted there can by scenepoints that are used but not in our m_lpCurrentHeldScenePoints list
		if (m_pAI->m_Script.IsValid())
		{	// Release ALL actions goddammit!
			RequestScenepoints();
		}

		// Next we check all scenepoints and release them if they cannot be held
		CWObject_Interface_AI* pObj = m_pAI->m_pGameObject;
		uint32 OwnerName = m_pAI->m_pServer->Object_GetNameHash(pObj->m_iOwner);
		uint16 liTeams[16];
		uint nTeams = pObj->AI_GetTeams(liTeams, sizeof_buffer(liTeams));
		for (int i = m_lpCurrentHeldScenePoints.Length()-1; i >= 0; i--)
		{
			if (m_lpCurrentHeldScenePoints.IsValid(i))
			{
				CWO_ScenePoint * pPoint = m_lpCurrentHeldScenePoints.Get(i);
				if (!pPoint->IsValid(m_pAI->m_pServer, pObj->GetNameHash(), OwnerName, liTeams, nTeams))
				{
					pPoint->Release(m_pAI->m_pGameObject->m_iObject, m_pAI->m_pServer, m_pAI->GetScenePointManager());
					if (m_pAI)
					{
						if (pPoint->GetCrouchFlag())
						{
							m_pAI->m_DeviceStance.Crouch(false);
						}
						if ((pPoint->GetBehaviour() != 0)&&(pPoint->GetBehaviour() == m_pAI->m_iBehaviourRunning))
						{
							m_pAI->StopBehaviour(CAI_Core::BEHAVIOUR_STOP_NORMAL,m_pAI->m_BehaviourPrioClass);
						}
						if (pPoint->GetSitFlag())
						{	// This can be a tad risky
							CWObject_Message Msg(OBJMSG_CHAR_SETANIMPHYS,0);
							m_pAI->m_pServer->Message_SendToObject(Msg,m_pAI->GetObjectID());
						}
					}
					// Invalidate scenepoint for 2 tick
					pPoint->InvalidateScenepoint(m_pAI->m_pServer,2);
					// Remove the scenepoint, even if we're dead
					m_lpCurrentHeldScenePoints.Remove(m_lpCurrentHeldScenePoints.Find(pPoint));
					
				}
			}
		}
	}


	for (int i = 0; i < m_lpCurrentHeldScenePoints.Length(); i++)
	{
		if (m_lpCurrentHeldScenePoints.IsValid(i))
		{
			CWO_ScenePoint * pPoint = m_lpCurrentHeldScenePoints.Get(i);
			if (m_lpLastHeldScenePoints.Find(pPoint) == -1)
				m_lpLastHeldScenePoints.Add(pPoint);
		}
	}
	
	m_lpCurrentHeldScenePoints.Clear();
};

//Check if override scene point fulfills the given conditions. If the destructive flag is true
//and types match, override scene point status will be lost at next scenepoint refresh.
CWO_ScenePoint * CAI_ActionHandler::CheckOverrideScenePoint(uint16 _Type, fp32 _MaxRange, fp32 _MinRange, bool _bDestructive)
{
	if (m_pAI->GetType() == CAI_Core::TYPE_DARKLING)
	{
		_Type |= CWO_ScenePoint::DARKLING;
	}

	//Check if we have a override senepoint with matching type
	if (!m_pOverrideScenePoint || !(m_pOverrideScenePoint->CheckType(_Type)))
	{
		return NULL;
	}

	//Check range
	if ((_MaxRange < _FP32_MAX) || (_MinRange > 0))
	{
		fp32 DistSqr = m_pAI->SqrDistanceToUs(m_pOverrideScenePoint->GetPosition());
		if ((DistSqr > Sqr(_MaxRange)) || (DistSqr < Sqr(_MinRange)))
			return NULL;
	}

	if (!m_pAI->CheckScenepoint(m_pOverrideScenePoint))
	{	// No can do
		return(NULL);
	}

	if (!m_pOverrideScenePoint->PeekRequest(m_pAI->m_pGameObject->m_iObject,m_pAI->m_pServer))
	{
		return NULL;
	}

	//Should we only make one try for override scene point?
	if (_bDestructive)
		m_bClearOverrideScenePoint = true;

	//All criteria fulfilled
	return m_pOverrideScenePoint;
};



//Private accessors
int CAI_ActionHandler::ReportedEvents(){return m_ReportedEvents;};
int CAI_ActionHandler::Leader(){return m_iLeader;};
int CAI_ActionHandler::Target(){return m_iTarget;};
int CAI_ActionHandler::Follower(){return m_iFollower;};
int CAI_ActionHandler::Hostile(){return m_iHostile;};



//CAI_Action_IdleCall///////////////////////////////////////////////////////////////////////////////
const char * CAI_Action_IdleCall::ms_lStrFlags[] = 
{
	"SCENEPTS",
	"RANDOMPTS",
	"CLOSEST",
	"NEARPLAYER",
	"PLAYERFOV",
	"HEADING",
	"AVOIDLIGHT",
	"PREFERLIGHT",
	NULL,
};

CAI_Action_IdleCall::CAI_Action_IdleCall(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = MAKE_IDLE_CALL;
	m_TypeFlags = TYPE_MOVE | TYPE_SOUND;

	m_MinRange = 64.0f;	// 2m
	m_MaxRange = 512.0f;// 16m

	m_iDialogue = 0;
	m_iSoundVariant = -1;

	m_iCaller = 0;
	m_iReceiver = 0;
	for (int i = 0; i < IDLECALL_SKIP_COUNT; i++)
	{
		m_liSkipList[i] = 0;
	}
	m_LastReceiveCallTimer = 0;
	m_bReceiverReady = false;
	m_ReceiverResponseTimer = -1;

	m_Mode = MODE_BOTH;
	m_Flags = FLAGS_SCENEPOINTS | FLAGS_CLOSEST_PT;

	// 10 seconds interval as default
	m_MinInterval = (int32)(10.0f * CAI_Core::AI_TICKS_PER_SECOND);
	if (_pAH)
	{
		m_LastStopTick = AI()->GetAITick();
	}

	m_ReceiverMat.Unit();
	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_ReceiverMat.GetRow(3));

	m_StayTimeout = -1;			// Tick when we should no longer stay at the spot but continue the search
	m_pScenePoint = NULL;		// Current active scenepoint
};

void CAI_Action_IdleCall::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_MODE:
		if ((_Val == MODE_BOTH)||(_Val == MODE_CALL)||(_Val == MODE_RECEIVE))
		{
			m_Mode = _Val;
		}
		break;
	case PARAM_FLAGS:
			m_Flags = _Val;
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
};

void CAI_Action_IdleCall::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_MINRANGE:
		m_MinRange = _Param;
		break;

	case PARAM_MAXRANGE:
		m_MaxRange = _Param;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
};

void CAI_Action_IdleCall::SetParameter(int _Param, CStr _Val)
{

	switch (_Param)
	{
	case PARAM_MINRANGE:
	case PARAM_MAXRANGE:
		SetParameter(_Param,(fp32)_Val.Val_fp64());
		break;

	case PARAM_MODE:
		SetParameter(_Param, _Val.Val_int());
		break;
	case PARAM_FLAGS:
		SetParameter(_Param, StrToFlags(_Val));
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}


//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_IdleCall::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "MINRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_MINRANGE;
	}
	if (_Str == "MAXRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_MAXRANGE;
	}
	else if (_Str == "MODE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_MODE;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

int CAI_Action_IdleCall::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

//Mandatory: Move and Sound (do we really need the SOUND device at this stage?
int CAI_Action_IdleCall::MandatoryDevices()
{
	return ((1 << CAI_Device::MOVE) | (1 << CAI_Device::LOOK));
};

//Valid when we're using idle or lower priority actions
bool CAI_Action_IdleCall::IsValid()
{
	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		return(false);
	}
	else
	{
		if (pAI->GetStealthTension() >= CAI_Core::TENSION_HOSTILE)
		{
			return(false);
		}
		if (m_iCaller)
		{
			if ((m_Mode == MODE_CALL)||(pAI->m_Timer > m_LastReceiveCallTimer+1))
			{
				m_iCaller = 0;
				return(false);
			}
		}
	}

	return(true);
};


//Bad for stealth, good for variety
CAI_ActionEstimate CAI_Action_IdleCall::GetEstimate()
{
	CAI_ActionEstimate Est;
	CAI_Core* pAI = AI();
	// Required devices does NOT include m_DeviceSound, we only check it in GetEstimate as the action
	// would otherwise disable itself.
	if ((IsValid())&&(pAI->m_DeviceSound.IsAvailable()))
	{	
		if (!m_iCaller)
		{	// Nbr of sending users are limited by ms_UseSemaphore
			if ((m_Mode == MODE_RECEIVE)||
				(!pAI->m_pAIResources->m_ActionGlobals.ms_IdleCallSemaphore.Peek(AI()->m_pGameObject->m_iObject,1,pAI->GetGlobalAITick())))
			{
				return(Est);
			}
		}
		
		// No idle call while dialogue talking
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
		if (pCD && pCD->m_DialogueInstance.IsValid())
		{
			return(Est);
		}

		// Unless receiver, check that we actually have a partner to talk to
		if (m_iCaller == 0)
		{
			m_iReceiver = 0;
			int iReceiver = pAI->GetClosestActionUserWithSkiplist(MAKE_IDLE_CALL,m_MaxRange,false,m_liSkipList,IDLECALL_SKIP_COUNT);
			if (iReceiver > 0)
			{
				m_iReceiver = iReceiver;
			}
			else
			{
				return (Est);
			}
		}
		else
		{
			m_iReceiver = 0;
		}

		//Default score 15
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, 0);
		Est.Set(CAI_ActionEstimate::STEALTH, -10);
		Est.Set(CAI_ActionEstimate::VARIETY, 25);
	}
	return Est;
};

bool CAI_Action_IdleCall::FindScenePointDestination()
{
	CAI_Core* pAI = AI();

	// We already have a valid scenepoint?
	if (m_pScenePoint)
	{
		if (m_pAH->RequestScenePoint(m_pScenePoint))
		{
			return(true);
		}
		else
		{	// No, it's not OUR job to set the memory scenepoints
			m_pScenePoint = NULL;
		}
	}

	CVec3Dfp32 OurPos = pAI->GetBasePos();
	int Flags = CAI_ActionHandler::SP_PREFER_HEADING;
	if (!(m_Flags & FLAGS_CLOSEST_PT))
	{
		Flags |= CAI_ActionHandler::SP_RANDOM_RANGE;
	}
	if (m_Flags & FLAGS_NEARPLAYER)
	{
		int32 playerID = pAI->GetClosestPlayer();
		if ((playerID)&&(playerID != pAI->m_pGameObject->m_iObject))
		{
			CWObject* pObj = pAI->m_pServer->Object_Get(playerID);
			if (pObj)
			{
				OurPos = pObj->GetPosition();
				Flags |= CAI_ActionHandler::SP_PREFER_NEAR_TARGETPOS;
			}
		}
	}
	if (m_Flags & FLAGS_IN_PLAYERFOV)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_PLAYERFOV;
	}
	if (m_Flags & FLAGS_PREFER_HEADING)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_HEADING;
	}
	if (m_Flags & FLAGS_AVOID_LIGHT)
	{
		Flags |= CAI_ActionHandler::SP_AVOID_LIGHT;
	}
	if (m_Flags & FLAGS_PREFER_LIGHT)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_LIGHT;
	}

	CMat4Dfp32 OurMat;
	pAI->GetBaseMat(OurMat);
	m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::TALK,Flags,m_MinRange,m_MaxRange,OurMat,OurPos,0.0f,0/*,m_pScenePoint01,m_pScenePoint02,m_pScenePoint03,m_pScenePoint04,m_pScenePoint05*/);
	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_ReceiverMat.GetRow(3));
	if (!m_pScenePoint)
	{
		return(false);
	}

	// m_pScenePoint->GetPosition() returns the offset position
	// The receiver should get the m_pScenePoint->GetPos() instead
	if (m_pScenePoint->GetType() & CWO_ScenePoint::JUMP_CLIMB)
	{
		m_Destination = m_pScenePoint->GetPosition();
	}
	else
	{
		m_Destination = pAI->m_PathFinder.GetPathPosition(m_pScenePoint->GetPosition(),4,2);
	}

	if ((!m_pAH->RequestScenePoint(m_pScenePoint))||(INVALID_POS(m_Destination))||(m_pAH->IsRestricted(m_Destination)))
	{
		INVALIDATE_POS(m_Destination);
		INVALIDATE_POS(m_ReceiverMat.GetRow(3));
		m_pAH->UpdateScenepointHistory(m_pScenePoint);
		m_pScenePoint = NULL;
		return(false);
	}

#ifndef M_RTM
	m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif

	// Now we must determine if the SP has a OBJMSG_RPG_SPEAK messages and if so, what users it has
	m_iDialogue = m_pScenePoint->GetSpeakDialog();
	if (m_iDialogue)
	{
		m_iReceiver = 0;
		// Get the dialogue resource and find its EVENTTYPE_USERS
		// If none is found, use its EVENTTYPE_LINK instead.
		// Add the users to m_iReceivers.
		CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
		if(pCD)
		{
			CWRes_Dialogue* pDialogue = CWObject_Character::GetDialogueResource(pAI->m_pGameObject,pAI->m_pServer);
			if (pDialogue)
			{	// Extract the EVENTTYPE_USERS or if that fails the EVENTTYPE_LINK
				const char *pStr = pDialogue->FindEvent(m_iDialogue,CWRes_Dialogue::EVENTTYPE_USERS);
				if (!pStr)
				{
					pStr = pDialogue->FindEvent(m_iDialogue,CWRes_Dialogue::EVENTTYPE_LINK);
					// Keep the part up until the ":"
					CStr Name = CStr(pStr).GetStrSep(":");
					int iObj = pAI->m_pServer->Selection_GetSingleTarget(Name);
					// iObj is unique because we cleared the list upon entering
					m_iReceiver = iObj;
				}
				else
				{	// Just pick the first valid name
					CStr Names = pStr;
					while(Names != "")
					{
						CStr Name = Names.GetStrSep(":");
						int iObj = pAI->m_pServer->Selection_GetSingleTarget(Name);
						if (iObj)
						{
							m_iReceiver = iObj;
							break;
						}
					}
				}
			}
		}
	}

	return(true);
};

/*
bool CAI_Action_IdleCall::FindRandomDestination()
{
	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_ReceiverMat.GetRow(3));

	if (m_liReceivers.Len() > 0)
	{
		int iReceiver = m_liReceivers[0];
		CAI_Core* pReceiverAI = AI()->GetAI(iReceiver);
		if ((pReceiverAI)&&(pReceiverAI->m_pGameObject))
		{
			CVec3Dfp32 Pos = pReceiverAI->GetBasePos();
			CVec3Dfp32 Dir = pReceiverAI->GetLookDir();
			Dir[2] = 0.0f; Dir.Normalize();
			CVec3Dfp32 OurPos = Pos + Dir * 48.0f;
			m_Destination = AI()->m_PathFinder.GetPathPosition(OurPos,2,1);
			if (VALID_POS(m_Destination))
			{
				return(true);
			}
		}
	}
	return(false);
};
*/

bool CAI_Action_IdleCall::FindDestination()
{
	if (VALID_POS(m_Destination))
	{
		return(true);
	}

	m_StayTimeout = -1;
	if (m_Flags & FLAGS_SCENEPOINTS)
	{
		if (FindScenePointDestination())
		{
			return(true);
		}
	}

	/*
	if (m_Flags & FLAGS_RANDOM)
	{
		if (FindRandomDestination())
		{
			return(true);
		}
	}
	*/

	return(false);
}

bool CAI_Action_IdleCall::MoveToDestination()
{
	CAI_Core* pAI = AI();

	// PESUDO: Move to m_Destination using the appropriate movement speed
	if (INVALID_POS(m_Destination))
	{
		m_StayTimeout = -1;
		if (m_pScenePoint)
		{
			m_pAH->UpdateScenepointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
		}
		return(false);
	}

	// Some basice useful values
	CVec3Dfp32 OurPos = pAI->m_PathFinder.SafeGetPathPosition(pAI->GetBasePos(),4,0);
	CVec3Dfp32 OurDir = pAI->GetLookDir();
	// As ::IsValid check for available devices we KNOW that if m_DeviceMove isn't available we have used it ourselves
	// Probably waiting for the receiver to get closer.
	if (!pAI->m_DeviceMove.IsAvailable())
	{
		if ((!m_iCaller)&&(OurPos.DistanceSqr(m_Destination) < Sqr(8.0f)))
		{
			return(true);
		}
		return(false);
	}

	// Stop for player unless he is an enemy
	CAI_AgentInfo* pPlayerAgent = m_pAH->StopForPlayer();
	if (pPlayerAgent)
	{
		pAI->ResetStuckInfo();
		pAI->m_DeviceMove.Use();
		// We probably shouldn't try to IdleCall for a while
		ExpireWithDelay(pAI->m_Patience);
		return(false);
	}

	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint)))
	{	// Badness, we can no longer use that scenepoint
		m_pAH->UpdateScenepointHistory(m_pScenePoint);
		m_pScenePoint = NULL;
		return(false);
	}

	// Stop roaming if restricted
	if ((VALID_POS(m_Destination))&&(m_pAH->IsRestricted(m_Destination)))
	{	
		INVALIDATE_POS(m_Destination);
		INVALIDATE_POS(m_ReceiverMat.GetRow(3));
		if (m_pScenePoint)
		{	// Badness, we can no longer use that scenepoint
			m_pAH->UpdateScenepointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
		}
		return(false);
	}

	if (m_pScenePoint)
	{
		if (m_StayTimeout == -1)
		{
			if (pAI->SqrDistanceToUs(m_Destination) < Sqr(32.0f))
			{
				pAI->OnTrackDir(-m_pScenePoint->GetDirection(),0,10,false,false);
			}

			if (m_pScenePoint->IsAt(OurPos))
			{
				CVec3Dfp32 Dir = pAI->GetLookDir();
				CMat4Dfp32 SenderMat = m_pScenePoint->GetPositionMatrix();
				SenderMat.GetRow(3) = m_pScenePoint->GetPosition();
				SenderMat.GetRow(0) = -SenderMat.GetRow(0);
				SenderMat.GetRow(1) = -SenderMat.GetRow(1);
				if ((m_bReceiverReady)&&(pAI->OnAlign(m_pScenePoint,m_iReceiver,10,true)))
				{
					int StayTicks = (int)(m_pScenePoint->GetBehaviourDuration() * pAI->GetAITicksPerSecond());
					m_StayTimeout = pAI->m_Timer + StayTicks;
					CWO_ScenePoint* orgSP = m_pScenePoint;
					m_pAH->ActivateScenePoint(m_pScenePoint,GetPriority(),&SenderMat);
					if (orgSP != m_pScenePoint) { return(false); }
					// Now, first receiver should also be forced to run the behaviour
					if ((m_iReceiver)&&(m_pScenePoint->GetSecondaryBehaviour()))
					{
						CAI_Core* pReceiverAI = pAI->GetAI(m_iReceiver);
						if (pReceiverAI)
						{
							int32 iSecondaryBehaviour = m_pScenePoint->GetSecondaryBehaviour();
							CMat4Dfp32 ReceiverMat = m_pScenePoint->GetPositionMatrix();
							if (m_pScenePoint->PlayOnce())
							{
								pReceiverAI->SetWantedBehaviour2(m_pScenePoint->GetSecondaryBehaviour(),GetPriority(),CAI_Core::BEHAVIOUR_FLAGS_PO,0,NULL,&ReceiverMat);
							}
							else
							{
								pReceiverAI->SetWantedBehaviour2(m_pScenePoint->GetSecondaryBehaviour(),GetPriority(),CAI_Core::BEHAVIOUR_FLAGS_PO |CAI_Core::BEHAVIOUR_FLAGS_LOOP,StayTicks,NULL,&ReceiverMat);
							}
						}
					}
					// Darkling is on wall or SP allows wall climbing
					/*
					if ((pAI->m_pGameObject->AI_IsOnWall())||(m_pScenePoint->GetType() & CWO_ScenePoint::WALK_CLIMB))
					{
						pAI->TempEnableWallClimb(StayTicks+pAI->m_Patience);
					}
					*/
				}
			}
		}
		if ((m_pScenePoint)&&(m_StayTimeout != -1)&&(m_pScenePoint->GetBehaviour()))
		{
			if (m_pScenePoint->PlayOnce())
			{
				m_StayTimeout = pAI->m_Timer + 1;
			}
			else if (m_pScenePoint->GetBehaviourDuration() == 0.0f)
			{
				m_StayTimeout = pAI->m_Timer + pAI->GetAITicksPerSecond();
			}
		}
		if (m_StayTimeout != -1)
		{
			return(true);
		}
	}
	else
	{
		if ((m_iCaller == 0)||(OurPos.DistanceSqr(m_Destination) < Sqr(8.0f)))
		{	// *** This and perfect origin too??? ***
			return(true);
		}
	}

	//Move there (use perfect placement in case radius is low)
	if (m_pScenePoint)
	{
		if (pAI->OnNudge(m_pScenePoint,pAI->m_ReleaseIdleSpeed,true))
		{
			return(true);
		}
	}
	else
	{
		if (pAI->OnNudge(m_Destination,pAI->m_ReleaseIdleSpeed))
		{
			return(true);
		}
	}

	int32 moveResult;
	moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,false,false,m_pScenePoint);
	if ((moveResult != CAI_Core::MOVE_DEVICE_BUSY)&&(!pAI->CheckMoveresult(moveResult,m_pScenePoint)))
	{
		if (m_pScenePoint)
		{
			m_pAH->UpdateScenepointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
		}
		INVALIDATE_POS(m_Destination);
		return(false);
	}
	return(false);
}

bool CAI_Action_IdleCall::StayAtDestination()
{
	CAI_Core* pAI = AI();

	// Track caller/receiver when far, align with matrix when close
	if (m_iCaller)
	{	// We are the receiver
		if ((pAI->SqrDistanceToUs(m_iCaller) >= Sqr(64.0))||(m_StayTimeout != -1))
		{
			pAI->OnTrackObj(m_iCaller,10,false,true);
		}
		else
		{
			pAI->OnTrackDir(m_ReceiverMat.GetRow(0),m_iCaller,10,false,false);
		}
	}
	else
	{	// We are the sender
		if ((pAI->SqrDistanceToUs(m_iReceiver) >= Sqr(64.0))||(m_StayTimeout != -1))
		{
			pAI->OnTrackObj(m_iReceiver,10,false,true);
		}
		else if (m_pScenePoint)
		{
			pAI->OnTrackDir(m_pScenePoint->GetDirection(false),m_iCaller,10,false,false);
		}
	}

	pAI->ResetStuckInfo();
	pAI->m_DeviceMove.Use();	// Hog the move device

	// Handle the talking, return true when talking is done
	if (m_iCaller == 0)
	{
		if (!m_iDialogue)
		{
			if ((m_bReceiverReady)&&(m_iSoundVariant == -1))
			{
				m_iSoundVariant = SpeakRandom(CAI_Device_Sound::IDLE_CALL);
				if (m_iSoundVariant != -1)
				{
					if (!pAI->GetCurrentBehaviour())
					{
						pAI->OnTrackObj(m_iReceiver,20,false,false);
					}
					int Duration = pAI->m_DeviceSound.GetDuration(CAI_Device_Sound::IDLE_CALL,m_iSoundVariant);
					pAI->m_DeviceSound.PauseType(CAI_Device_Sound::IDLE_CALL,0.0f);
					m_ReceiverResponseTimer = (int)(pAI->m_Timer + Duration + (pAI->GetAITicksPerSecond() * Random * 0.5f));
					SetExpirationExpired();
					SetExpiration(Duration + pAI->GetAITicksPerSecond() * 3);
					if (m_iReceiver)
					{
						CAI_Core* pReceiverAI = pAI->GetAI(m_iReceiver);
						if (pReceiverAI)
						{
							pReceiverAI->UseRandomDelayed("Idlecall response",CAI_Device_Sound::IDLE_CALL_RESPONSE,GetPriority(), Duration);
							if (!pReceiverAI->GetCurrentBehaviour())
							{
								pReceiverAI->OnTrackObj(pAI->GetObjectID(),20,false,false);
							}
						}
					}
				}
				else
				{	// Couldn't speak somehow; bail!
					SetExpirationExpired();
					return(true);
				}
			}
		}
	}

	return(false);
}

void CAI_Action_IdleCall::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		m_pScenePoint = NULL;
	}
}

//Make idle noises every once in a while
void CAI_Action_IdleCall::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_IdleCall::OnTakeAction);
	CAI_Core* pAI = AI();

	if ((m_iCaller == 0)&&(m_iReceiver == 0))
	{
		SetExpirationExpired();
		return;
	}

	if (pAI->HighTension())
	{
		SetExpirationExpired();
		return;
	}

	if ((m_iCaller == 0)&&
		(!pAI->m_pAIResources->m_ActionGlobals.ms_IdleCallSemaphore.Poll(pAI->m_pGameObject->m_iObject,1,pAI->GetGlobalAITick())))
	{
		SetExpirationExpired();
		return;
	}

	if (FindDestination())
	{
		if (m_iReceiver)
		{
			int Result = RESULT_THERE;
			CAI_Core* pReceiverAI = pAI->GetAI(m_iReceiver);
			if ((!pReceiverAI)||(!pReceiverAI->IsConscious()))
			{
				SetExpirationExpired();
				return;
			}
			
			CMat4Dfp32 ReceiverMat;
			if (m_pScenePoint)
			{
				ReceiverMat = m_pScenePoint->GetPositionMatrix();
			}
			else
			{
				pReceiverAI->GetBaseMat(ReceiverMat);
			}
			if ((m_ReceiverResponseTimer != -1)&&(pAI->m_Timer > m_ReceiverResponseTimer))
			{	// Receiver is not ready to voice a response yet	
				Result = pReceiverAI->m_AH.ReceiveCall(pAI->m_pGameObject->m_iObject,ReceiverMat,m_iSoundVariant);
				m_ReceiverResponseTimer = -1;
			}
			else
			{	// Speak your piece and then go quietly into the night
				Result = pReceiverAI->m_AH.ReceiveCall(pAI->m_pGameObject->m_iObject,ReceiverMat,-1);
			}
			if (Result == RESULT_FAIL)
			{
				SetExpirationExpired();
				return;
			}
			else if (Result == RESULT_THERE)
			{
				m_bReceiverReady = true;
			}
			else if (Result > 1)
			{
				fp32 Range = m_Destination.Distance(pAI->GetBasePos());
				if (Range >= 8.0f)
				{	// We don't start walking until the receiver halfway there relative us
					int32 walkTicks = int(Range / pAI->m_ReleaseIdleSpeed);
					if (walkTicks < Result * 2)
					{
						pAI->m_DeviceMove.Use();
					}
				}
			}

			if ((pAI->SqrDistanceToUs(m_Destination) <= Sqr(AI_COLL_AVOID_RANGE))&&
				(pAI->SqrDistanceToUs(m_iReceiver) <= Sqr(AI_COLL_AVOID_RANGE)))
			{
				pAI->m_KB.SkipCollChecking(m_iReceiver);
				pReceiverAI->m_KB.SkipCollChecking(pAI->GetObjectID());

				if ((m_pScenePoint)&&(m_pScenePoint->GetBehaviour())&&(m_pScenePoint->GetSecondaryBehaviour()))
				{
					pAI->SetCharacterCollisions(false);
					pReceiverAI->SetCharacterCollisions(false);
				}
			}
		}

		if (MoveToDestination())
		{
			if (StayAtDestination())
			{
				SetExpirationExpired();
				return;
			}
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
		}
		else
		{
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif
		}
		// Hog move device to stop ROAM from running
		pAI->m_DeviceMove.Use();
	}
	else
	{	// No destination, bail out
		SetExpirationExpired();
	}

	return;
};

bool CAI_Action_IdleCall::IsValidReceiver(int _iCaller)
{
	CAI_Core* pAI = AI();
	if ((!pAI)||(m_Mode == MODE_CALL))
	{
		return(false);
	}

	// Being scripted, running behaviours, in danger make us ineligible to receive calls
	if ((pAI->m_Script.IsValid())||(pAI->GetCurrentBehaviour())||
		(pAI->GetStealthTension() >= CAI_Core::TENSION_HOSTILE)||(pAI->m_DeviceStance.GetIdleStance() > CAI_Device_Stance::IDLESTANCE_IDLE))
	{
		return(false);
	}

	if (IsExpired())
	{
		if (m_iCaller == 0)
		{
			// Must see or recently have seen caller, otherwise the walk may be too long to do
			CAI_AgentInfo* pCaller = pAI->m_KB.GetAgentInfo(_iCaller);
			if (!pCaller)
			{
				pCaller = pAI->m_KB.AddAgent(_iCaller);
			}
			if ((!pCaller)||(pCaller->GetCurAwareness() < CAI_AgentInfo::DETECTED)||(pCaller->GetGetLastSpottedTicks() > pAI->m_Patience))
			{
				return(false);
			}
			return(true);
		}
		else
		{
			if (m_iCaller != _iCaller)
			{
				return(false);
			}
		}
	}
	else
	{
		if (!m_iCaller)
		{
			return(false);
		}
	}

	return(true);
};

// RESULT_FAIL = -1		// Failure, caller should abort and note m_iLastPartner
// RESULT_THERE = 0		// We're there dude, bring it on
// RESULT_NOT_YET > 1	// We'll be there in a nbr of ticks
int CAI_Action_IdleCall::ReceiveCall(int _iCaller, CMat4Dfp32& _PosMat, int _iSoundVariant)
{
	CAI_Core* pAI = AI();

	if (!IsValidReceiver(_iCaller))
	{
		return(RESULT_FAIL);
	}

	if (IsExpired())
	{
		if (m_iCaller == 0)
		{
			m_LastStopTick = 0;	// We don't give a hoot about m_Interval now!
			m_iCaller = _iCaller;
			m_Destination = pAI->m_PathFinder.SafeGetPathPosition(_PosMat.GetRow(3),4,2);
			m_ReceiverMat = _PosMat;
			m_iSoundVariant = _iSoundVariant;
			m_LastReceiveCallTimer = pAI->m_Timer;
			m_pScenePoint = NULL;
		}
	}
	
	if (m_iCaller)
	{	// If we lack a destination or the caller isn't talking to us we must bail
		if ((INVALID_POS(m_Destination))||(pAI->m_Timer > m_LastReceiveCallTimer+1))
		{
			SetExpirationExpired();
			return(RESULT_FAIL);
		}

		// Timestamp and estimate how long until we get there
		m_LastReceiveCallTimer = pAI->m_Timer;
		m_iSoundVariant = _iSoundVariant;	// *** This right??? ***
		int iResult = RESULT_THERE;
		CVec3Dfp32 OurPos = pAI->m_PathFinder.SafeGetPathPosition(pAI->GetBasePos(),4,0);
		fp32 Range = m_Destination.Distance(OurPos);
		pAI->m_DeviceLook.Free();
		pAI->OnTrackDir(_PosMat.GetRow(0),m_iCaller,10,false,false);
		if (Range < 8.0f)
		{	// We're there dude
			// Do some perfect origining
			if (pAI->OnAlign(&_PosMat,m_iCaller,10))
			{	// Inside 2.5 degrees
				return(iResult);
			}
			return(1);
		}
		else
		{	// Getting there dude
			iResult = int(Range / pAI->m_ReleaseIdleSpeed);
			return(iResult);
		}
	}
	else
	{
		return(RESULT_FAIL);
	}
}

void CAI_Action_IdleCall::OnStart()
{
	CAI_Core* pAI = AI();
	m_iDialogue = 0;
	m_iSoundVariant = -1;
	m_bReceiverReady = false;
	m_ReceiverResponseTimer = -1;
	CAI_Action::OnStart();
	AI()->ActivateAnimgraph();
	SetExpirationIndefinite();
};

void CAI_Action_IdleCall::OnQuit()
{
	CAI_Core* pAI = AI();

	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_ReceiverMat.GetRow(3));

	m_iDialogue = 0;
	m_iSoundVariant = -1;

	pAI->ClearPerfectOrigin();
	if (m_iReceiver)
	{
		for (int i = 0; i < IDLECALL_SKIP_COUNT-1; i++)
		{
			m_liSkipList[i+1] = m_liSkipList[i];
		}
		m_liSkipList[0] = m_iReceiver;
		// pAI->OnTrackObj(m_iReceiver,20,false,true);
		m_iReceiver = 0;
	}
	if (m_iCaller)
	{
		for (int i = 0; i < IDLECALL_SKIP_COUNT-1; i++)
		{
			m_liSkipList[i+1] = m_liSkipList[i];
		}
		m_liSkipList[0] = m_iCaller;
		pAI->OnTrackObj(m_iCaller,20,false,true);
		m_iCaller = 0;
	}
	m_bReceiverReady = false;
	m_ReceiverResponseTimer = -1;

	m_StayTimeout = -1;
	if (m_pScenePoint)
	{
		m_pScenePoint = NULL;
	}

	CAI_Action::OnQuit();
};

//CAI_Action_Idle///////////////////////////////////////////////////////////////////////////////////
CAI_Action_Idle::CAI_Action_Idle(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = IDLE;
	m_TypeFlags = 0;
	CAI_Action::SetParameter(PARAM_INTERVAL,30.0f);
	m_Angle = 2.0f * 45.0f / 360.0f;		// +-45 degrees off
};

int CAI_Action_Idle::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int nAdded = 0;
	for (int i = 0; i < m_liBehaviours.Len(); i++)
	{
		nAdded += AddUnique(_liBehaviours,m_liBehaviours[i]);
	}

	return(nAdded);
};

int CAI_Action_Idle::GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const
{
	int nAdded = 0;
	for (int i = 0; i < m_liGestures.Len(); i++)
	{
		nAdded += AddUnique(_liGestures,m_liGestures[i]);
	}

	return(nAdded);
};

void CAI_Action_Idle::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_ANGLE:
		m_Angle = 2.0f * _Val / 360.0f;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}
void CAI_Action_Idle::SetParameter(int _Param, CStr _Val)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_ANGLE:
		SetParameter(_Param, (fp32)_Val.Val_fp64());
		break;
	case PARAM_BEHAVIOURS:
		// Extract the behaviour nbrs
		{
			m_liBehaviours.Clear();
			int iBehaviour = -1;
			CStr Temp = _Val;
			do
			{
				if ((iBehaviour = Temp.GetIntMSep(",+")))
				{
					m_liBehaviours.Add(iBehaviour);
				}
			} while (iBehaviour > 0);
		}
		break;
	case PARAM_GESTURES:
		// Extract the gesture nbrs
		{
			m_liGestures.Clear();
			int iGesture= -1;
			CStr Temp = _Val;
			do
			{
				if ((iGesture = Temp.GetIntMSep(",+")))
				{
					m_liGestures.Add(iGesture);
				}
			} while (iGesture > 0);
		}
		break;
	case PARAM_SOUNDS:
		// Extract the sound nbrs
		{
			m_liSounds.Clear();
			int iSound = -1;
			CStr Temp = _Val;
			do
			{
				if ((iSound = Temp.GetIntMSep(",+")))
				{
					m_liSounds.Add(iSound);
				}
			} while (iSound > 0);
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}


//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_Idle::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "ANGLE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_ANGLE;
	}
	else if (_Str == "BEHAVIOURS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_BEHAVIOURS;
	}
	else if (_Str == "GESTURES")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_GESTURES;
	}
	else if (_Str == "SOUNDS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_SOUNDS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

//Mandatory: Look; Optional: Move, Sound, Animation
int CAI_Action_Idle::MandatoryDevices()
{
	return (1 << CAI_Device::LOOK);
};

//Quite lazy but boring
CAI_ActionEstimate CAI_Action_Idle::GetEstimate()
{
	CAI_Core* pAI = AI();
	CAI_ActionEstimate Est;

	if (pAI->GetCurrentBehaviour())
	{
		m_LastStopTick = pAI->GetAITick() + pAI->m_Patience;
	}

	if ((IsValid())&&(AI()->GetStealthTension() < CAI_Core::TENSION_HOSTILE)&&(!pAI->HighTension())&&
		(!pAI->m_bBehaviourRunning)&&(!pAI->m_iDialoguePartner)&&(pAI->m_DeviceSound.IsAvailable()))
	{
		if ((!pAI->GetCollider())&&(!pAI->GetCurrentBehaviour()))
		{	// No idlebehaviours near colliders
			//Default score 10
			Est.Set(CAI_ActionEstimate::OFFENSIVE, 0);
			Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
			Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
			Est.Set(CAI_ActionEstimate::LOYALTY, 0);
			Est.Set(CAI_ActionEstimate::LAZINESS, 20);
			Est.Set(CAI_ActionEstimate::STEALTH, 0);
			Est.Set(CAI_ActionEstimate::VARIETY, -10);

			return Est;
		}
	};

	return Est;
};



//Look idly around. Stay put if possible.
void CAI_Action_Idle::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Idle::OnTakeAction);
	CAI_Core* pAI = AI();

	if ((pAI->GetCurrentBehaviour())||
		(pAI->m_DeviceStance.GetIdleStance() > CAI_Device_Stance::IDLESTANCE_IDLE)||
		(pAI->GetStealthTension() > CAI_Core::TENSION_NONE)||
		(pAI->HighTension()))
	{
		SetExpirationExpired();
		return;
	}
	// Darklings
	if (pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)
	{
		if ((pAI->GetUp() * CVec3Dfp32(0.0f,0.0f,1.0f)) < 0.99f)
		{	// More than 8 degrees of normal up
			SetExpirationExpired();
			return;
		}

		pAI->ActivateAnimgraph();
		if ((pAI->m_bBehaviourRunning)||(pAI->m_iDialoguePartner)||(!pAI->m_DeviceSound.IsAvailable())||(pAI->GetPrevSpeed() > 0.1f))
		{
			SetExpirationExpired();
			return;
		}

		if (m_pAH->HasAction(MAKE_IDLE_CALL,true))
		{	// Don't interfere with IdleCall
			ExpireWithDelay(30 * pAI->GetAITicksPerSecond());
			return;
		}

		// Handle Idle banter here
		// Some idle banter
		// Handle rivalry between the two darklings
		CAI_Core* pAIColl = pAI->GetAI(pAI->m_KB.GetCollider());
		if (pAIColl)
		{
			if (MRTC_ISKINDOF(pAI,CAI_Core_Darkling))
			{
				CAI_Core_Darkling* pDK = safe_cast<CAI_Core_Darkling>(pAI);
				if (pDK)
				{
					if (MRTC_ISKINDOF(pAIColl,CAI_Core_Darkling))
					{
						CAI_Core_Darkling* pDKColl = safe_cast<CAI_Core_Darkling>(pAIColl);
						if (pDKColl)
						{
							CVec3Dfp32 OurEyes = pDK->GetLookPos();
							CVec3Dfp32 HisEyes = pDKColl->GetLookPos();
							CVec3Dfp32 WantedDir = (HisEyes-OurEyes).Normalize();
							pDK->OnTrackDir(WantedDir,pDKColl->GetObjectID(),20,false,false);
							pDKColl->OnTrackDir(-WantedDir,pDK->GetObjectID(),20,false,false);
							if (pDK->GetLookDir() * WantedDir < 0.96f)
							{
								SetExpiration(80);
								return;
							}
							if (((pDK->m_DK_Special == CAI_Core_Darkling::DK_SPECIAL_KAMIKAZE)&&(pDKColl->m_DK_Special == CAI_Core_Darkling::DK_SPECIAL_GUNNER))||
								((pDK->m_DK_Special == CAI_Core_Darkling::DK_SPECIAL_GUNNER)&&(pDKColl->m_DK_Special == CAI_Core_Darkling::DK_SPECIAL_KAMIKAZE)))
							{	// Cold war revisited!
								pDK->UseRandom(CStr("Darkling coldwar"),CAI_Device_Sound::IDLE_CALL,CAI_Action::PRIO_ALERT);
							}
							else
							{	// Regular chitchat
								int iVariant = pDK->UseRandom(CStr("Darkling quarrel"),CAI_Device_Sound::IDLE_CALL,CAI_Action::PRIO_IDLE);
								if (iVariant >= 0)
								{
									int duration = pDKColl->m_DeviceSound.GetDuration(CAI_Device_Sound::IDLE_CALL,iVariant);
									if (duration)
									{	// Respond after half a second
										pDKColl->UseRandomDelayed(CStr("Darkling quarrel response"),CAI_Device_Sound::IDLE_CALL_RESPONSE,CAI_Action::PRIO_IDLE,duration+20);
									}
								}
							}
							// SetExpirationExpired();
							// return;
						}
					}
				}
			}
		}
	}

	{
		// Pick something at random between behaviours, gestures or sounds
		fp32 Len = (m_liBehaviours.Len()+m_liGestures.Len()+m_liSounds.Len());
		int Rnd = (int)(Len * Random * 0.999f);
		if (Rnd < m_liBehaviours.Len())
		{	// Run behaviour, then quit
			if ((pAI->GetPrevSpeed() > 0.01f)||(!pAI->m_DeviceMove.IsAvailable()))
			{	// No Idle behaviours when moving
				return;
			}
			int Behaviour = m_liBehaviours[Rnd];
			pAI->SetWantedBehaviour2(Behaviour,GetPriority(),0,(int32)(pAI->GetAITicksPerSecond() * (0.5f+Random) * 6.0f),NULL,NULL);
			pAI->m_DeviceMove.Use();
			SetExpirationExpired();
			return;
		}
		Rnd -= m_liBehaviours.Len();
		
		if (Rnd < m_liGestures.Len())
		{	// Run gesture, then quit
			int Gesture = m_liGestures[Rnd];
			// How do we handle gestures?
			pAI->SetWantedGesture(Gesture);
			SetExpirationExpired();
			return;
		}
		Rnd -= m_liGestures.Len();

		if (Rnd < m_liSounds.Len())
		{	// Run behaviour, then quit
			int Dialogue = m_liSounds[Rnd];
			pAI->m_DeviceSound.PlayDialogue(Dialogue);
			SetExpirationExpired();
			return;
		}
	}
};

void CAI_Action_Idle::OnStart()
{
	CAI_Action::OnStart();
}

void CAI_Action_Idle::OnQuit()
{
	CAI_Action::OnQuit();
}

//CAI_Action_CheckDead////////////////////////////////////////////////////////////////////////////
const char * CAI_Action_CheckDead::ms_lStrFlags[] = 
{
	"Kneel",
	"GlobalRemove",
	"TeamOnly",
	"Avoid",

	NULL,
};

CAI_Action_CheckDead::CAI_Action_CheckDead(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = CHECKDEAD;
	m_TypeFlags = TYPE_SEARCH | TYPE_DEATH;
	m_MaxRange = 640.0f;					// 20 m default maxrange
	m_StopRange = 16.0f;
	m_Relation = CAI_AgentInfo::NEUTRAL;	// All but neutral by default

	// The -1 indicates that we haven't even told the ai we want deaths reported
	m_Dead.m_iVictim = -1;
	m_bFoundCorpse = false;
	m_KneelDoneTick = -1;
	m_bCrouch = false;
	m_iCheckCorpseBehaviour = 0;
	m_MinInterval = (int32)(0.5f * CAI_Core::AI_TICKS_PER_SECOND);	// Half a second
	INVALIDATE_POS(m_PathPos);
	INVALIDATE_POS(m_Destination);
};

int CAI_Action_CheckDead::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

void CAI_Action_CheckDead::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_RELATION:
		{
			m_Relation = _Val;
		}
		break;

	case PARAM_BEHAVIOUR:
		{
			if (_Val >= 1)
			{
				m_iCheckCorpseBehaviour = _Val;
			}
			else
			{
				m_iCheckCorpseBehaviour = 0;
			}
		}
		break;

	case PARAM_FLAGS:
		{
			m_Flags = _Val;
		}
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_CheckDead::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_MAXRANGE:
		m_MaxRange = _Val;
		break;

	case PARAM_STOPRANGE:
		m_StopRange = _Val;
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}
void CAI_Action_CheckDead::SetParameter(int _Param, CStr _Val)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_MAXRANGE:
	case PARAM_STOPRANGE:
		{
			SetParameter(_Param, (fp32)_Val.Val_fp64());
		}
		break;

	case PARAM_RELATION:
		{
			int Val = _Val.TranslateInt(CAI_AgentInfo::ms_TranslateRelation);
			SetParameter(_Param, Val);
		}
		break;

	case PARAM_BEHAVIOUR:
		{
			SetParameter(_Param,_Val.Val_int());
		}
		break;

	case PARAM_FLAGS:
		{
			SetParameter(_Param, StrToFlags(_Val));
		}
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}


//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_CheckDead::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "MAXRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_MAXRANGE;
	}
	if (_Str == "STOPRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_STOPRANGE;
	}
	else if (_Str == "RELATION")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_RELATION;
	}
	else if (_Str == "BEHAVIOUR")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_BEHAVIOUR;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_FLAGS;			
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

int CAI_Action_CheckDead::GetPriority(bool _bTest)
{
	if (_bTest)
	{
		return(PRIO_IDLE);
	}
	else
	{
		if (m_bFoundCorpse)
		{
			return(m_Priority);
		}
		else
		{
			return(PRIO_IDLE);
		}
	}
};

// Are there any dead?
bool CAI_Action_CheckDead::IsValid()
{
	CAI_Core* pAI = AI();

	// tell the world we are interested in the dead
	if (m_Dead.m_iVictim == -1)
	{
		pAI->m_KB.Global_SetCareAboutDeath(m_Relation);
		m_Dead.m_iVictim = 0;
	}

	// Cheap early out
	if (!CAI_Action::IsValid())
	{	
		return(false);
	}

	if (pAI->GetStealthTension() >= CAI_Core::TENSION_COMBAT)
	{
		return(false);
	}

	// Checking for all flags is fairly cheap
	if ((m_bFoundCorpse)||(pAI->m_KB.NbrOfValidDead() > 0)||(m_Dead.m_iVictim > 0))
	{
		return(true);
	}
	else
	{
		return(false);
	}
};

// Check every dead for range first and then for relation
CAI_ActionEstimate CAI_Action_CheckDead::GetEstimate()
{
	CAI_ActionEstimate Est;
	CAI_Core* pAI = AI();
	if ((IsValid())&&(pAI->GetStealthTension() < CAI_Core::TENSION_COMBAT)&&(!pAI->HighTension()))
	{	// Check ranges to all the dead
		if (pAI->m_KB.GetFirstMatchingDead(&m_Dead,m_Relation,0.0f,m_MaxRange,false))
		{
			//Default score 65
			Est.Set(CAI_ActionEstimate::OFFENSIVE,0); 
			Est.Set(CAI_ActionEstimate::DEFENSIVE,10);
			Est.Set(CAI_ActionEstimate::EXPLORATION,50);
			Est.Set(CAI_ActionEstimate::LOYALTY,10);
			Est.Set(CAI_ActionEstimate::LAZINESS,0);
			Est.Set(CAI_ActionEstimate::STEALTH,0);
			Est.Set(CAI_ActionEstimate::VARIETY,0);

			return(Est);
		}
	}

	return Est;
}

int CAI_Action_CheckDead::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int nAdded = 0;

	if (m_iCheckCorpseBehaviour)
	{
		nAdded += AddUnique(_liBehaviours,m_iCheckCorpseBehaviour);
	}

	return(nAdded);
};

bool CAI_Action_CheckDead::AnimationEvent(int _iUser, int _iEvent)
{	// Handle kick, lightkiller, whatever

	CAI_Core* pAI = AI();

	if ((pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)&&(MRTC_ISKINDOF(pAI,CAI_Core_Darkling)))
	{
		CAI_Core_Darkling* pDK = safe_cast<CAI_Core_Darkling>(pAI);
		if ((pDK)&&(pDK->m_DK_Special == CAI_Core_Darkling::DK_SPECIAL_LIGHTKILLER)&&(pDK->m_iCurBehaviour)&&(m_Dead.m_iVictim))
		{	// Tell Wilbo that we're ready to Zapp or Unzapp as the case may be
			CWObject* pObjVictim = pAI->m_pServer->Object_Get(m_Dead.m_iVictim);
			if ((pObjVictim)&&(_iEvent == 0))
			{
				CVec3Dfp32 Zap = pObjVictim->GetPosition();
				pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DARKLING_LIGHTNING, 1, 0, -1, 0, Zap), pDK->GetObjectID());
				CVec3Dfp32 Dir = CVec3Dfp32(Random,Random,0.5f);
				Dir.Normalize();
				CWObject_Message Msg(OBJMSG_CHAR_PUSHRAGDOLL);
				Msg.m_VecParam1 = Dir * 20.0f;
				pAI->m_pServer->Message_SendToObject(Msg,m_Dead.m_iVictim);
			}
			else if (_iEvent == 1)
			{
				pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DARKLING_LIGHTNING, 0, 0, -1, 0), pDK->GetObjectID());
			}
			return(true);
		}
	}
	else
	{	// Others kick?
		if (_iEvent == 0)
		{
		}
	}

	return(false);
};

void CAI_Action_CheckDead::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_CheckDead::OnTakeAction);
	CAI_Core* pAI = AI();

	// We expire if the tension is too high
	if (pAI->HighTension())
	{
		SetExpirationExpired();
		return;
	}

	// We expire onece for every found dead
	if ((m_bFoundCorpse)&&(m_KneelDoneTick != -1)&&(pAI->m_Timer > m_KneelDoneTick))
	{	// We've payed our last respect or whatever
		pAI->m_KB.SetInvestigated(m_Dead.m_iVictim);
		SetExpirationExpired();
		return;
	}

	// Preliminary
	int N = pAI->m_KB.NbrOfValidDead();
	if (N < 1)
	{	// Someone else removed the corpse from list
		m_bFoundCorpse = false;
		m_Dead.m_iVictim = 0;
		SetExpirationExpired();
		return;
	}

	// Just bail if semaphore is not available and we haven't yet found a corpse
	// Is the semaphore available?
	if ((!m_bFoundCorpse)&&(!(m_Flags & FLAGS_AVOID))&&
		(!pAI->m_pAIResources->m_ActionGlobals.ms_CheckDeadSemaphore.Peek(pAI->m_pGameObject->m_iObject,1,pAI->GetGlobalAITick())))
	{	// Well, maybe next time then?
		return;
	}
	else if ((m_bFoundCorpse)&&(!(m_Flags & FLAGS_AVOID)))
	{
		if 	(!pAI->m_pAIResources->m_ActionGlobals.ms_CheckDeadSemaphore.Poll(pAI->m_pGameObject->m_iObject,1,pAI->GetGlobalAITick(),1))
		{
			SetExpirationExpired();
			return;
		}
	}

	CWObject* pObjVictim = NULL;
	if ((!m_bFoundCorpse)&&(m_Dead.m_iVictim == 0))
	{	// No dead to investigate yet
		m_KneelDoneTick = -1;
		if (pAI->m_KB.GetFirstMatchingDead(&m_Dead,m_Relation,0.0f,m_MaxRange,false))
		{
			pObjVictim = pAI->m_pServer->Object_Get(m_Dead.m_iVictim);
			if (pObjVictim)
			{	// OK, increment the counter
				m_bFoundCorpse = false;
			}
			else
			{	// Can we really get here? I dunno
				SetExpirationExpired();
				return;
			}
		}
		else
		{
			SetExpirationExpired();
			return;
		}
	}
	else
	{
		pObjVictim = pAI->m_pServer->Object_Get(m_Dead.m_iVictim);
	}

	// Have we or someone else removed the dead from the list since OnStart?
	if (!pAI->m_KB.IsValidDead(m_Dead.m_iVictim))
	{
		m_Dead.m_iVictim = 0;
		return;
	}

	if (!pObjVictim)
	{
		SetExpirationExpired();
		return;
	}

	CVec3Dfp32 Pos = pObjVictim->GetPosition();
	Pos = pAI->m_PathFinder.SafeGetPathPosition(Pos,4,2);
	if (m_pAH->IsRestricted(Pos))
	{
		ExpireWithDelay(pAI->GetAITicksPerSecond() * 3);
		SetExpirationExpired();
		return;
	}
	CVec3Dfp32 OurPos = pAI->GetBasePos();
	OurPos = pAI->m_PathFinder.SafeGetPathPosition(OurPos,2,1);

	//If victim has (been) moved or we haven't set path position, then do it
	if ((INVALID_POS(m_PathPos)) || 
		(CAI_Core::XYDistanceSqr(m_PathPos, Pos) > Sqr(32.0f)))
	{
		CVec3Dfp32 NewPos = pAI->m_PathFinder.GetPathPosition(Pos, 2, 1);
		if (VALID_POS(NewPos))
		{
			m_PathPos = NewPos;
		}
		else
		{
			SetExpirationExpired();
			return;
		}
	}

	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();
	fp32 RangeSqr = OurPos.DistanceSqr(Pos);
	if ((!m_bFoundCorpse)&&((pAI->m_Timer % 5 == 0)||(RangeSqr <= Sqr(48.0f))))
	{
		int VictimAwareness = CAI_AgentInfo::NONE;
		if (RangeSqr > Sqr(48.0f))
		{	// Don't use x-ray vision when checking the dead
			int PerceptionFlags = pAI->m_PerceptionFlags;
			pAI->m_PerceptionFlags &= ~PERC_XRAYVISION_FLAG;
			VictimAwareness = pAI->Perception_Sight(m_Dead.m_iVictim,true);
			pAI->m_PerceptionFlags = PerceptionFlags;
		}
		else
		{	// Autospot corpses within 32 units
			VictimAwareness = CAI_AgentInfo::DETECTED;
		}

#ifndef M_RTM
		if ((pObjVictim)&&(pAI->m_pGameObject))
		{
			CVec3Dfp32 viewerPos,sightPos;
			CMat4Dfp32 LookMat;
			pAI->GetHeadMat(LookMat);
			viewerPos = CVec3Dfp32::GetRow(LookMat,3);
			sightPos = pObjVictim->GetPosition();
			pAI->DebugDrawCheckDead(viewerPos,sightPos,VictimAwareness);
		}
#endif
		if (VictimAwareness >= CAI_AgentInfo::DETECTED)
		{	// We found one!!
			// Raise friends, remove body from list
			m_bFoundCorpse = true;
			int Alertness = pAI->m_KB.GetAlertness();
			if ((Alertness < CAI_KnowledgeBase::ALERTNESS_WATCHFUL)&&(Alertness >= CAI_KnowledgeBase::ALERTNESS_DROWSY))
			{
				pAI->m_KB.SetAlertness(CAI_KnowledgeBase::ALERTNESS_WATCHFUL);
			}

			CAI_AgentInfo* pCulprit = pAI->m_KB.GetAgentInfo(m_Dead.m_iCause);
			int CulpritAwareness = CAI_AgentInfo::NONE;
			if ((pCulprit)&&(pCulprit->GetCurRelation() > CAI_AgentInfo::FRIENDLY))
			{
				CulpritAwareness = pCulprit->GetAwareness(true);
				if (CulpritAwareness >= CAI_AgentInfo::DETECTED)
				{	// Guilt by association!
					// We find you guilty of MURDER because you're too damned close to a fresh corpse
					if (Pos.DistanceSqr(pCulprit->GetBasePos()) < Sqr(96.0f))
					{
						if ((!(m_Flags & FLAGS_TEAMONLY))||(pAI->IsTeammember(m_Dead.m_iVictim)))
						{
							pCulprit->SetRelation(CAI_AgentInfo::ENEMY,40);
							// We know who the murderer is!
							// We know the suspect!
							pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
							pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
							if (pAI->m_CharacterClass == CAI_Core::CLASS_CIV)
							{
								SpeakRandom(CAI_Device_Sound::AFRAID_PANIC);
							}
							else
							{
								SpeakRandom(CAI_Device_Sound::ENEMY_SPOT_PLAYER);
							}
						}
						
						pAI->m_KB.SetInvestigated(m_Dead.m_iVictim);
						if (m_Flags & FLAGS_GLOBALREMOVE)
						{
							pAI->m_KB.Global_RemoveDead(m_Dead.m_iVictim);
							pAI->RaiseFoundCorpse(m_Dead.m_iCause,m_Dead.m_iVictim,m_Dead.m_DeathPos);
						}
						SetExpirationExpired();
						return;
					}
					else if ((Pos.DistanceSqr(pCulprit->GetBasePos()) < Sqr(256.0f))&&
						(pCulprit->GetCurRelation() >= CAI_AgentInfo::UNFRIENDLY)&&
						(pCulprit->GetCurRelation() < CAI_AgentInfo::HOSTILE_2))
					{	// Culprit is too far away to be the murderer but we sure don't like him
						int CulpritRelation = pCulprit->GetCurRelation();
						pCulprit->SetRelation(CulpritRelation+1,40);
						pAI->SetStealthTension(CAI_Core::TENSION_HOSTILE);
					}
				}
				else // NOTICED or less
				{
					if ((pCulprit->GetCurRelation() >= CAI_AgentInfo::UNFRIENDLY)&&(pCulprit->GetCurRelation() < CAI_AgentInfo::HOSTILE))
					{	// Fake us looking for the culprit
						pCulprit->SetRelation(CAI_AgentInfo::HOSTILE);
						pCulprit->SetAwareness(CAI_AgentInfo::NOTICED,true,true);
						pCulprit->SetCorrectSuspectedPosition();
						pCulprit->Touch();
					}

				}
			}

			// Check if the corpse is not too close and didn't die recently
			if ((pAI->SqrDistanceToUs(pObjVictim) < Sqr(96))&&(m_Dead.m_TimeOfDeath >= pAI->m_Timer + 40))
			{	// Don't globally remove the dead, others might still find the death interesting
				pAI->m_KB.RemoveDead(m_Dead.m_iVictim);
				pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
				SetExpirationExpired();
				return;
			}

			// We found the body but no murderer
			pAI->OverrideFlashThreshold(0.6f,pAI->m_Patience);
			if (pAI->GetStealthTension() < CAI_Core::TENSION_COMBAT)
			{
				SpeakRandom(CAI_Device_Sound::CHECKDEAD_IDLE_SPOT_CORPSE);
				pAI->SetMinStealthTension(CAI_Core::TENSION_HOSTILE);
				if (pAI->m_DeviceStance.GetIdleStance() < CAI_Device_Stance::IDLESTANCE_HOSTILE)
				{
					pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
				}
				if (m_Flags & FLAGS_GLOBALREMOVE)
				{
					pAI->RaiseFoundCorpse(0,m_Dead.m_iVictim,m_Dead.m_DeathPos);
				}
			}
		}
		else if (VictimAwareness < CAI_AgentInfo::NOTICED)
		{	// Didn't see the corpse at all
			m_Dead.m_iVictim = 0;
		}
	}

	if ((m_bFoundCorpse)&&(m_Flags & FLAGS_AVOID))
	{	// Avoid the corpse instead
		pAI->SetMinStealthTension( CAI_Core::TENSION_HOSTILE);
		if (pAI->m_DeviceStance.GetIdleStance() < CAI_Device_Stance::IDLESTANCE_HOSTILE)
		{
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
		}
		OnAvoid(Pos);
		return;
	}

	if (m_bFoundCorpse)
	{
		pAI->SetMinStealthTension( CAI_Core::TENSION_HOSTILE);
		if (pAI->m_DeviceStance.GetIdleStance() < CAI_Device_Stance::IDLESTANCE_HOSTILE)
		{
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
		}
		// We have already expired if we found the corpse and kneeled long enough
		// so no check is needed
		if (RangeSqr <= Sqr(m_StopRange))
		{
			if (m_KneelDoneTick == -1)
			{	// Use the find body behaviour if available
				if (m_iCheckCorpseBehaviour)
				{
					if (pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)
					{	// Piss
						pAI->UseRandom(CStr("CAI_Action_CheckDead"),CAI_Device_Sound::CHECKDEAD_IDLE_PISS_CORPSE,GetPriority());
						bool bUseBehaviour = false;
						CMat4Dfp32 Mat;
						pAI->GetBaseMat(Mat);
						CVec3Dfp32 Diff = Pos - Mat.GetRow(3);
						if (m_iCheckCorpseBehaviour != CAI_Core::BEHAVIOUR_DARKLING_PISS)
						{
							pAI->OnTrackDir(-Diff,0,10,false,false);
							Diff[2] = 0.0f;
							if (Mat.GetRow(0) * Diff < -0.86f)
							{	// More or less pointing away from corpse
								uint32 Flags = 0;
								if (Mat.GetRow(1) * Diff < 0.0f)
								{	// Right
									Flags |= CAI_Core::BEHAVIOUR_FLAGS_RIGHT;
								}
								// 1 second is enough, we expire after the behaviour has finished
								m_KneelDoneTick = (int32)(pAI->m_Timer + 1.0f * pAI->GetAITicksPerSecond());
								pAI->SetWantedBehaviour2(m_iCheckCorpseBehaviour,GetPriority(),Flags,0,NULL,NULL);
								if (m_Flags & FLAGS_GLOBALREMOVE)
								{
									pAI->m_KB.Global_RemoveDead(m_Dead.m_iVictim);
								}
							}
						}
						else
						{	// More or less pointing towards corpse
							pAI->OnTrackDir(Diff,0,10,false,false);
							// 1 second is enough, we expire after the behaviour has finished
							m_KneelDoneTick = (int32)(pAI->m_Timer + 1.0f * pAI->GetAITicksPerSecond());
							pAI->SetWantedBehaviour2(m_iCheckCorpseBehaviour,GetPriority(),0,0,NULL,NULL);
							if (m_Flags & FLAGS_GLOBALREMOVE)
							{
								pAI->m_KB.Global_RemoveDead(m_Dead.m_iVictim);
							}
						}					
					}
					else
					{	// Buhu
						// 1 second is enough, we expire after the behaviour has finished
						int StayTicks = (int32)(2.0f * pAI->GetAITicksPerSecond());
						uint32 Flags = CAI_Core::BEHAVIOUR_FLAGS_LOOP;
						m_KneelDoneTick = pAI->m_Timer + StayTicks;
						pAI->SetWantedBehaviour2(m_iCheckCorpseBehaviour,GetPriority(),0,0,NULL,NULL);
						pAI->UseRandom(CStr("CAI_Action_CheckDead"),CAI_Device_Sound::CHECKDEAD_DEATHCAUSE_GENERIC,GetPriority());
						if (m_Flags & FLAGS_GLOBALREMOVE)
						{
							pAI->m_KB.Global_RemoveDead(m_Dead.m_iVictim);
						}
						// Kick the sucker a bit
						// *** TODO: React to animevent instead to time better with animation
						CVec3Dfp32 Dir = pAI->GetLookDir();
						Dir[2] = 0.5f;
						Dir.Normalize();
						CWObject_Message Msg(OBJMSG_CHAR_PUSHRAGDOLL);
						Msg.m_VecParam1 = Dir * 5.0f;
						pAI->m_pServer->Message_SendToObject(Msg,m_Dead.m_iVictim);
						CAI_AgentInfo* pCulprit = pAI->m_KB.GetAgentInfo(m_Dead.m_iCause);
						if ((pCulprit)&&(pCulprit->GetCurAwareness() < CAI_AgentInfo::SPOTTED)&&(pCulprit->GetCurRelation() >= CAI_AgentInfo::UNFRIENDLY))
						{
							pCulprit->SetAwareness(CAI_AgentInfo::NOTICED,true,true);
							pCulprit->Touch();
						}
					}
				}
				else
				{	// Kneel down and pray!
					if (m_Flags & FLAGS_GLOBALREMOVE)
					{
						pAI->m_KB.Global_RemoveDead(m_Dead.m_iVictim);
					}
					pAI->UseRandom(CStr("CAI_Action_CheckDead"),CAI_Device_Sound::CHECKDEAD_DEATHCAUSE_GENERIC,GetPriority());
					if (m_Flags & FLAGS_KNEEL)
					{
						m_KneelDoneTick = (int32)(pAI->m_Timer + 3.0f * pAI->GetAITicksPerSecond());
						if (!pAI->m_DeviceStance.IsCrouching())
						{
							pAI->m_DeviceStance.Crouch(true);
							m_bCrouch = true;
						}
					}
				}
			}

			// Look at the corpse
			if (m_KneelDoneTick >= 0)
			{
				OnTrackObj(m_Dead.m_iVictim,10);
			}

			// Use the look and move devices
			pAI->m_DeviceMove.Use();
		}
		else
		{
			if (pAI->m_DeviceMove.IsAvailable())
			{	// Move towards corpse
				int32 moveResult = pAI->OnMove(Pos,pAI->m_ReleaseIdleSpeed,false,false,NULL);
				if (!pAI->CheckMoveresult(moveResult,NULL))
				{	// Couldn't move somehow
					pAI->m_KB.RemoveDead(m_Dead.m_iVictim);
					SetExpirationExpired();
					return;
				}
				pAI->m_DeviceMove.Use();
			}
		}
	}
}

bool CAI_Action_CheckDead::OnAvoid(CVec3Dfp32 _AvoidPos)
{
	CAI_Core* pAI = AI();

	if (INVALID_POS(m_PathPos))
	{
		INVALIDATE_POS(m_Destination);
		return(false);
	}

	// Invalidate m_Destination if target has moved much
	if ((INVALID_POS(m_Destination))||(pAI->m_Timer % 40 == 0))
	{	// This gets pretty expensive!
		INVALIDATE_POS(m_Destination);
	}

	if (INVALID_POS(m_Destination))
	{	
		// Set up a new move away from the threat
		CVec3Dfp32 OurPos = pAI->Proj2Base(pAI->GetBasePos());
		OurPos = pAI->m_PathFinder.SafeGetPathPosition(OurPos,1,1);

		CVec3Dfp32 Separation = pAI->Proj2Base(m_PathPos) - OurPos;
		CVec3Dfp32 SepNorm = Separation;
		SepNorm.Normalize();
		CVec3Dfp32 LeftNorm = SepNorm;
		LeftNorm[0] = -SepNorm[1];
		LeftNorm[1] = SepNorm[0];
		LeftNorm.Normalize();
		// SepNorm and LeftNorm make up our coordinate system
		CVec3Dfp32 BackMove = OurPos - SepNorm * m_MaxRange * 0.25f;
		CVec3Dfp32 LeftMove = OurPos + LeftNorm * m_MaxRange * 0.25f;
		CVec3Dfp32 RightMove = OurPos - LeftNorm * m_MaxRange * 0.25f;
		if (pAI->m_bWallClimb)
		{
			m_Destination = BackMove;
		}
		else
		{	// No climbey wally
			BackMove = pAI->m_PathFinder.SafeGetPathPosition(BackMove,5,4);
			LeftMove = pAI->m_PathFinder.SafeGetPathPosition(LeftMove,5,4);
			RightMove = pAI->m_PathFinder.SafeGetPathPosition(RightMove,5,4);

			CVec3Dfp32 StopPos;
			if (!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,BackMove,
				(int)pAI->GetBaseSize(),
				(int)pAI->GetHeight(),
				pAI->m_bWallClimb,
				(int)pAI->GetStepHeight(),
				(int)pAI->GetJumpHeight(),
				&StopPos))
			{	// We couldn't move straight away from the threat
				BackMove = StopPos;
				if (!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,RightMove,
					(int)pAI->GetBaseSize(),
					(int)pAI->GetHeight(),
					pAI->m_bWallClimb,
					(int)pAI->GetStepHeight(),
					(int)pAI->GetJumpHeight(),
					&StopPos))
				{	// We couldn't move right, measure left
					RightMove = StopPos;
				}

				if (!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,LeftMove,
					(int)pAI->GetBaseSize(),
					(int)pAI->GetHeight(),
					pAI->m_bWallClimb,	
					(int)pAI->GetStepHeight(),
					(int)pAI->GetJumpHeight(),
					&StopPos))
				{	// We couldn't move right, measure left
					LeftMove = StopPos;
				}
				fp32 BackMoveSqr = BackMove.DistanceSqr(m_PathPos);
				fp32 LeftMoveSqr = LeftMove.DistanceSqr(m_PathPos);
				fp32 RightMoveSqr = RightMove.DistanceSqr(m_PathPos);
				fp32 SepSqr = Separation.LengthSqr() + Sqr(16.0f);
				INVALIDATE_POS(m_Destination);
				if ((BackMoveSqr > Sqr(m_MaxRange * 0.25f))||((BackMoveSqr >= RightMoveSqr * 0.75f)&&(BackMoveSqr >= LeftMoveSqr * 0.75f)))
				{	// Long enough
					m_Destination = BackMove;
				}
				else if (RightMoveSqr >= LeftMoveSqr)
				{
					m_Destination = RightMove;
				}
				else
				{
					m_Destination = LeftMove;
				}
				m_Destination = pAI->m_PathFinder.SafeGetPathPosition(m_Destination,5,4);
			}
			else
			{	// BackMove is OK
				m_Destination = BackMove;
			}

#ifndef M_RTM
			pAI->Debug_RenderWire(OurPos,LeftMove,kColorRed,3.0f);
			pAI->Debug_RenderWire(OurPos,BackMove,kColorGreen,3.0f);
			pAI->Debug_RenderWire(OurPos,RightMove,kColorBlue,3.0f);
#endif
		}
	}

	pAI->OnTrackPos(m_PathPos,10,false,false);
	if ((VALID_POS(m_Destination))&&(pAI->SqrFlatDistance(m_Destination) >= Sqr(16.0f)))
	{
		pAI->AddMoveTowardsPositionCmd(m_Destination,pAI->m_ReleaseIdleSpeed);
		return(true);
	}
	else
	{
		pAI->m_DeviceMove.Use();
		if (m_Dead.m_iVictim)
		{
			if (m_Flags & FLAGS_GLOBALREMOVE)
			{
				pAI->m_KB.Global_RemoveDead(m_Dead.m_iVictim);
			}
			else
			{
				pAI->m_KB.RemoveDead(m_Dead.m_iVictim);
			}
		}
	}

	return(false);
};


// OnStart will try to find a dead bot within range and valid for m_Flags
// and fill in the m_Dead member with its data.
// If m_Dead.m_iVictim == 0 we failed
void CAI_Action_CheckDead::OnStart()
{
	CAI_Action::OnStart();
	INVALIDATE_POS(m_Destination);
	m_bFoundCorpse = false;
	m_KneelDoneTick = -1;
};

void CAI_Action_CheckDead::OnQuit()
{
	CAI_Core* pAI = AI();

	INVALIDATE_POS(m_Destination);
	if (m_bFoundCorpse)
	{
		m_pAH->m_FoundCorpseTick = pAI->GetAITick();
		if (pAI->m_DeviceStance.GetIdleStance() <= CAI_Device_Stance::IDLESTANCE_HOSTILE)
		{
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_IDLE);
		}
		CAI_AgentInfo* pCulprit = pAI->m_KB.GetAgentInfo(m_Dead.m_iCause);
		if (pCulprit)
		{	// Notice murderer to get a little search party going
			if (pCulprit->GetCurAwareness() < CAI_AgentInfo::DETECTED)
			{	// Mute SEARCH_START for a long time
				pAI->m_DeviceSound.PauseType(CAI_Device_Sound::SEARCH_START,5.0f);
				pCulprit->SetAwareness(CAI_AgentInfo::NOTICED,true,true);
				CWObject* pObjVictim = pAI->m_pServer->Object_Get(m_Dead.m_iVictim);
				if (pObjVictim)
				{
					CVec3Dfp32 Pos = pObjVictim->GetPosition();
					Pos = pAI->m_PathFinder.GetPathPosition(Pos,5);
					pCulprit->SetSuspectedPosition(Pos);
				}
			}
		}
		pAI->m_KB.FoundCorpse(0,m_Dead.m_iVictim,m_Dead.m_DeathPos);
		pAI->m_DeviceStance.Crouch(false);
#ifndef M_RTM
		if (AI()->DebugRender())
		{
			CStr Name = AI()->m_pGameObject->GetName();
			ConOut(Name + CStr(": CAI_Action_CheckDead::OnQuit() found corpse"));
		}
#endif
	}

	m_bFoundCorpse = false;
	m_bCrouch = false;
	m_KneelDoneTick = -1;
	m_Dead.m_iVictim = 0;
	AI()->m_DeviceStance.Crouch(false);
	CAI_Action::OnQuit();
};

//Not overridable normally (perhaps make this action dependable)
bool CAI_Action_CheckDead::IsOverridableByActionOverride()
{
	return false;
};


//CAI_Action_Follow////////////////////////////////////////////////////////////////////////////

CAI_Action_Follow::CAI_Action_Follow(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = FOLLOW;
	m_TypeFlags = TYPE_MOVE;
	m_iTrueLeader = 0;
	m_FindingRange = 1000;
	m_FollowRangeSqr = 100*100;
	m_Loyalty = 70;
	m_FollowPos = CVec3Dfp32(_FP32_MAX);
};


//Set params
void CAI_Action_Follow::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_LEADER:
		m_iTrueLeader = _Val;
		break;
	case PARAM_LOYALTY:
		m_Loyalty = Max(0, Min(100, _Val));
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}
void CAI_Action_Follow::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_FINDINGRANGE:
		m_FindingRange = _Val;
		break;
	case PARAM_FOLLOWRANGE:
		m_FollowRangeSqr = Sqr(_Val);
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}
void CAI_Action_Follow::SetParameter(int _Param, CStr _Val)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_LEADER:
		{
			CWObject * pLeader = AI()->GetSingleTarget(0, _Val, AI()->m_pGameObject->m_iObject);
			if (pLeader)
				SetParameter(_Param, pLeader->m_iObject);
		}
		break;
	case PARAM_FINDINGRANGE:
	case PARAM_FOLLOWRANGE:
		SetParameter(_Param, (fp32)_Val.Val_fp64());
		break;
	case PARAM_LOYALTY:
		SetParameter(_Param, _Val.Val_int());
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}


//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_Follow::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "LEADER")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_TARGET;
		return PARAM_LEADER;
	}
	else if (_Str == "RANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_FINDINGRANGE;
	}
	else if (_Str == "FOLLOWRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_FOLLOWRANGE;
	}
	else if (_Str == "LOYALTY")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_LOYALTY;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};



//Mandatory: Move; Optional: Look
int CAI_Action_Follow::MandatoryDevices()
{
	return (1 << CAI_Device::MOVE);
};

//Valid when there's a leader around
bool CAI_Action_Follow::IsValid()
{
	//If we don't have a leader, try to find one
	return CAI_Action::IsValid();
};

	
//Following someone is usually a bit safer than being by yourself, although it might be boring
//and won't satisfy any exploration needs if leader stays put.
CAI_ActionEstimate CAI_Action_Follow::GetEstimate()
{
	CAI_ActionEstimate Est;
	if (IsValid())
	{
		//Only support true leader fo now
		CWObject * pLeader = AI()->IsValidAgent(m_iTrueLeader);

		//Only good if away from leader or if leader is moving
		if (pLeader && 
			((AI()->SqrDistanceToUs(pLeader) > m_FollowRangeSqr) || (pLeader->GetLastPosition() != pLeader->GetPosition())))
		{
			//Default score 65
			Est.Set(CAI_ActionEstimate::OFFENSIVE, 0); 
			Est.Set(CAI_ActionEstimate::DEFENSIVE, 10);
			Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
			Est.Set(CAI_ActionEstimate::LOYALTY, m_Loyalty);
			Est.Set(CAI_ActionEstimate::LAZINESS, 0);
			Est.Set(CAI_ActionEstimate::STEALTH, -10);
			Est.Set(CAI_ActionEstimate::VARIETY, -5);

			/*
			//Check distance to leader
			CWObject * pLeader = m_pAH->IsValidLeader(m_iTrueLeader);
			if (!pLeader) 
			pLeader = m_pAH->IsValidLeader(m_pAH->Leader());
			if (pLeader->GetPosition().DistanceSqr(AI()->GetBasePos()) > 400*400)
			{
			//Leader far away (55)
			Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
			Est.Set(CAI_ActionEstimate::EXPLORATION, 10);
			Est.Set(CAI_ActionEstimate::LAZINESS, -10);
			};
			*/
		}
	}
	return Est;
}


//Go to best leader and follow him around if possible.
void CAI_Action_Follow::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Follow::OnTakeAction);
	CAI_Core* pAI = AI();

	//Always stop if at low activity
	pAI->ActivateAnimgraph();
	if (pAI->m_ActivityLevel <= CAI_Core::ACTIVITY_LOW)
		pAI->m_DeviceMove.Use();

	//Get current leader (only support true leader for now)
	CWObject* pLeader = pAI->IsValidAgent(m_iTrueLeader);

	//Do we have a leader now?
	if (pLeader)
	{
		//We have leader, follow him. Currently we don't get out of the leaders way or the way
		//of other friends. Will fix.
		if (INVALID_POS(m_FollowPos))
		{
			//Get path position of leader and reset path stuff
			pAI->ResetPathSearch();
			m_FollowPos = pAI->m_PathFinder.GetPathPosition(pLeader);
		}

		//Move if we aren't close enough to leader and have follow position
		fp32 DistSqr = (pLeader->GetPosition()).DistanceSqr(pAI->GetBasePos());
		if ((DistSqr > m_FollowRangeSqr) && (VALID_POS(m_FollowPos)))
		{
			//If we are more than 200 units away from leader we should move at maximum idle speed, otherwise
			//emulate leaders speed.
			fp32 Speed;
			if (DistSqr > Sqr(200)) 
				Speed = pAI->m_ReleaseIdleSpeed;
			else 
			{
				fp32 LeaderSpeedSqr = pLeader->GetLastPosition().DistanceSqr(pLeader->GetPosition());
				if (LeaderSpeedSqr > Sqr(pAI->m_ReleaseIdleSpeed))
					Speed = pAI->m_ReleaseMaxSpeed;
				else
					Speed = pAI->m_ReleaseIdleSpeed;
			}

			//Check if leader has moved (or if we have new leader)
			if ((pLeader->GetPosition()).DistanceSqr(m_FollowPos) > Sqr(32))
			{
				//Leader has moved more than a bit, go to it's new position, but don't follow partial path.
				CVec3Dfp32 PathPos = pAI->m_PathFinder.GetPathPosition(pLeader);
				if (VALID_POS(PathPos))
					//If leader is airborne, then go to last position (i.e. don't update follow pos)
					m_FollowPos = PathPos;
			};

			//Follow
			pAI->OnMove(m_FollowPos,Speed,false,false,NULL);
			SetExpiration(100, 200);
		}
		else
		{
			if (!(pAI->m_ActionFlags & CAI_Core::LOOKING))
			{	//Look the same way as leader
				CVec3Dfp32 HeadPos = pAI->GetLookPos();
				CVec3Dfp32 HeadDir = pAI->GetLookDir();
				OnTrackPos(HeadPos + HeadDir * 100.0f, 20, true);
			}

			//Do nothing
			pAI->OnIdle();

			//We are were we want to be so reset path
			pAI->ResetPathSearch();
	
			//We are with leader, so we don't need to follow
			SetExpirationExpired();
		};
	}
	else
	{
		//We don't have a leader. Boooring...
		pAI->OnIdle();

		//Reset path
		pAI->ResetPathSearch();	
		SetExpirationExpired();
	}
}


//Reset path stuff
void CAI_Action_Follow::OnQuit()
{
	if (AI())
	{
		//Reset path and follow pos
		AI()->m_PathDestination = CVec3Dfp32(_FP32_MAX);
		AI()->ResetPathSearch();
		m_FollowPos = CVec3Dfp32(_FP32_MAX);		
	}
	CAI_Action::OnQuit();
}



//CAI_Action_Restrict///////////////////////////////////////////////////////////////////////////////
CAI_Action_Restrict::CAI_Action_Restrict(CAI_ActionHandler* _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = RESTRICT;
	m_TypeFlags = TYPE_MOVE;
	m_RestrictPos = CVec3Dfp32(_FP32_MAX);
	m_MaxRange = 512.0f;
	m_iObject = 0;
	m_sObjName = "";
	m_pRoom = NULL;
	m_bIsRestricted = false;
	m_PathPos = CVec3Dfp32(_FP32_MAX);

	m_bActive = false;
};

// Set the parameters
void CAI_Action_Restrict::SetActionOverride(int _iBehaviour, CStr _sParamName, fp32 _DistParam, fp32 _MaxDistParam, int _iActivator)
{
	//Assume that _sParamName actually may be a string of the format <param name>;<distance param>;<max distance param>
	int iPos = _sParamName.Find(";");
	if (iPos != -1)
	{
		//";" detected. Get string after ";"
		CStr sDistParams = _sParamName.Right(_sParamName.Len() - iPos - 1); 

		//Trim param name
		_sParamName = _sParamName.Left(iPos);

		//Extract distance params
		if ((iPos = sDistParams.Find(";")) != -1)
		{
			if (iPos == 0)
				//No distance param
				_DistParam = -1.0f;
			else
				_DistParam =  (fp32)sDistParams.Left(iPos).Val_fp64();

			if (iPos == sDistParams.Len() - 1)
				//No maxdistance param
				_MaxDistParam = -1.0f;
			else
				_MaxDistParam =  (fp32)sDistParams.Right(sDistParams.Len() - iPos - 1).Val_fp64();
		}
		else
		{
			//No maxdistance param
			_MaxDistParam = -1.0f;

			if (_sParamName.Len() == 0)
				//No distance param
				_DistParam = -1.0f;
			else
				_DistParam =  (fp32)sDistParams.Val_fp64();			
		}
	};

	m_sObjName = _sParamName;
	m_MaxRange =  _MaxDistParam;

	IsRestricted(AI()->GetBasePos());
}

//Set params
void CAI_Action_Restrict::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_ACTIVE:
		{
			if (_Val != 0)
			{
				m_bActive = true;
			}
			else
			{
				m_bActive = false;
			}
		}
		break;
	case PARAM_OBJECT:
		{
			m_iObject = _Val;
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Restrict::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_RANGE:
		if (_Val < 0.00001f)
		{
			_Val = 256.0f;
		}
		m_MaxRange = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Restrict::SetParameter(int _Param, const CVec3Dfp32& _Val)
{
	switch (_Param)
	{
	case PARAM_POS:
		m_RestrictPos = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Restrict::SetParameter(int _Param, CStr _Val)
{
	CAI_Core* pAI = AI();

	switch (_Param)
	{
	case PARAM_POS:
		{
			CVec3Dfp32 Vec;
			Vec.ParseString(_Val);
			SetParameter(_Param, Vec);
		}
		break;

	case PARAM_RANGE:
		{
			SetParameter(_Param,(fp32)_Val.Val_fp64());
		}
		break;

	case PARAM_OBJECT:
		{	// Handle under restriction instead
			m_sObjName = _Val;
		}
		break;

	case PARAM_ACTIVE:
		{
			SetParameter(_Param,_Val.Val_int());
		}
		break;

	case PARAM_MULTI:
		{
			CWObject* pAreaObj = AI()->GetSingleTarget(0, _Val, AI()->m_pGameObject->m_iObject);
			if (pAreaObj)
			{
				SetParameter(_Param, pAreaObj->m_iObject);
			}
			else
			{	// Conevrt _Val to integer:
				// iVal <= 0 : Off
				// iVal == 1 : On
				// iVal > 1  : Range
				int iVal = _Val.Val_int();
				if (iVal > 1)
				{
					SetParameter(PARAM_RANGE,(fp32)iVal);
				}
				else if (iVal == 1)
				{
					SetParameter(PARAM_ACTIVE,1);
				}
				else
				{
					SetParameter(PARAM_ACTIVE,0);
				}
			}
		}
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}


//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_Restrict::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "POS")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_VEC;			
		return PARAM_POS;
	}
	else if (_Str == "RANGE")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_VEC;			
		return PARAM_RANGE;
	}
	else if (_Str == "OBJECT")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_TARGET;
		return PARAM_OBJECT;
	}
	else if (_Str == "ACTIVE")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_INT;			
		return PARAM_ACTIVE;
	}
	else if (_Str == "MULTI")
	{	// This is added for completeness, the param will only be used through mesages
		if (_pResType)
			*_pResType = PARAMTYPE_TARGET;
		return PARAM_MULTI;
	}
	return CAI_Action::StrToParam(_Str, _pResType);
};


//Mandatory: Move; Optional: Look
int CAI_Action_Restrict::MandatoryDevices()
{
	// return (1 << CAI_Device::MOVE);
	return(0);
};


bool CAI_Action_Restrict::IsValid()
{
	CAI_Core* pAI = AI();
	if ((!m_bActive)||(!CAI_Action::IsValid()))
	{
		m_bIsRestricted = false;
		return(false);
	}

	if (m_bIsRestricted)
	{
		if ((m_pRoom)&&(!m_pRoom->PointIsInsideRoom(pAI->GetBasePos())))
		{
			return(true);
		}
		if (pAI->SqrDistanceToUs(m_RestrictPos) > Sqr(m_MaxRange * 0.5f))
		{
			return(true);
		}
	}
	else
	{
		if (IsRestricted(pAI->GetBasePos()))
		{
			m_bIsRestricted = true;
			return(true);
		}
	}

	return(false);
}

// If restriction is active we return the room if any
CWObject_Room* CAI_Action_Restrict::GetRoom()
{
	if (m_bActive)
	{
		return(m_pRoom);
	}
	else
	{
		return(NULL);
	}
}

// Returns true if _Pos would be outside the restriction range
bool CAI_Action_Restrict::IsRestricted(const CVec3Dfp32& _Pos, bool _bActivate)
{
	CAI_Core* pAI = AI();

	// Resolve object/room name here
	if (m_sObjName != "")
	{
		// We must check for Room objects first
		CWO_RoomManager* pRoomManager = (CWO_RoomManager*)pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETROOMMANAGER),pAI->m_pServer->Game_GetObjectIndex());
		if (pRoomManager)
		{	// Is there a room with that name?
			CNameHash NameHash = CNameHash(m_sObjName);
			m_pRoom = pRoomManager->GetRoom(NameHash);
			if ((!m_pRoom)&&(m_sObjName.CompareSubStr("$ROOM") == 0))
			{
				m_pRoom = pRoomManager->GetRoom(pAI->GetBasePos());
			}
			if ((m_pRoom)&&((INVALID_POS(m_RestrictPos))||(!m_pRoom->PointIsInsideRoom(m_RestrictPos))))
			{
				m_RestrictPos = pAI->GetBasePos();
				if (!m_pRoom->PointIsInsideRoom(m_RestrictPos))
				{	// This may be expensive
					m_RestrictPos = m_pRoom->GetSafePos();
				}
				if (VALID_POS(m_RestrictPos))
				{
					m_RestrictPos = pAI->m_PathFinder.SafeGetPathPosition(m_RestrictPos,4,2);
					m_PathPos = m_RestrictPos;
					if (!m_pRoom->PointIsInsideRoom(pAI->m_HoldPosition))
					{
						pAI->m_HoldPosition = m_RestrictPos;
						pAI->m_HoldLookDir = pAI->GetLookDir();
						pAI->m_HoldLookDir[2] = 0.0f;
						pAI->m_HoldLookDir.Normalize();
					}
				}
				
			}
		}

		if ((!pRoomManager)||(!m_pRoom))
		{
			CWObject* pObj = AI()->GetSingleTarget(0,m_sObjName,AI()->m_pGameObject->m_iObject);
			if (pObj)
			{
				m_iObject = pObj->m_iObject;
				m_RestrictPos = pObj->GetPosition();
				m_RestrictPos = pAI->m_PathFinder.SafeGetPathPosition(m_RestrictPos,4,2);
				m_PathPos = m_RestrictPos;
				if (pAI->SqrDistanceToUs(pAI->m_HoldPosition) > Sqr(m_MaxRange))
				{
					pAI->m_HoldPosition = m_RestrictPos;
					pAI->m_HoldLookDir = pAI->GetLookDir();
					pAI->m_HoldLookDir[2] = 0.0f;
					pAI->m_HoldLookDir.Normalize();
				}
			}
		}
		m_sObjName = "";
	}

	if (!m_bActive)
	{
		m_bIsRestricted = false;
		return(false);
	}

	// IS the point inside the room?
	if (m_pRoom)
	{
		if (!m_pRoom->PointIsInsideRoom(_Pos))
		{
			if (_bActivate)
			{
				m_bIsRestricted = true;
			}
			return(true);
		}
	}
	else if (m_iObject)
	{	// Update pos
		CWObject* pObj = pAI->m_pServer->Object_Get(m_iObject);
		if (pObj)
		{
			m_iObject = pObj->m_iObject;
			m_RestrictPos = pObj->GetPosition();
			m_RestrictPos = pAI->m_PathFinder.SafeGetPathPosition(m_RestrictPos,4,2);
			m_PathPos = m_RestrictPos;
			if (pAI->SqrDistanceToUs(pAI->m_HoldPosition) > Sqr(m_MaxRange))
			{
				pAI->m_HoldPosition = m_RestrictPos;
				pAI->m_HoldLookDir = pAI->GetLookDir();
				pAI->m_HoldLookDir[2] = 0.0f;
				pAI->m_HoldLookDir.Normalize();
			}
		}
		else
		{
#ifndef M_RTM
			if (pAI->DebugRender())
			{
				CStr Name = pAI->m_pGameObject->GetName();
				ConOut(Name + CStr(": Restriction failed to find object"));
			}
#endif
		}
	}
	else
	{	// No object was given
		m_iObject = pAI->GetObjectID();
		m_RestrictPos = pAI->GetBasePos();
		m_RestrictPos = pAI->m_PathFinder.SafeGetPathPosition(m_RestrictPos,4,2);
		m_PathPos = m_RestrictPos;
		if (pAI->SqrDistanceToUs(pAI->m_HoldPosition) > Sqr(m_MaxRange))
		{
			pAI->m_HoldPosition = m_RestrictPos;
			pAI->m_HoldLookDir = pAI->GetLookDir();
			pAI->m_HoldLookDir[2] = 0.0f;
			pAI->m_HoldLookDir.Normalize();
		}
	}

	if (INVALID_POS(m_RestrictPos))
	{
		return(false);
	}

	if (m_RestrictPos.DistanceSqr(_Pos) > Sqr(m_MaxRange))
	{
		if (_bActivate)
		{
			m_bIsRestricted = true;
		}
		return(true);
	}
	
	return(false);
}

//Holding at current location is a pretty neutral action, slightly safe and of course lazy 
CAI_ActionEstimate CAI_Action_Restrict::GetEstimate()
{
	CAI_ActionEstimate Est;
	if (IsValid())
	{
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 5);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 5);
		Est.Set(CAI_ActionEstimate::LOYALTY, 20);
		Est.Set(CAI_ActionEstimate::LAZINESS, 20);
		Est.Set(CAI_ActionEstimate::VARIETY, 0); 
	}
	return Est;
};


//Stay put, or go to position and stay put.
void CAI_Action_Restrict::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Restrict::OnTakeAction);

	//Can't take action if we don't have a valid AI
	CAI_Core* pAI = AI();
	if (INVALID_POS(m_RestrictPos))
	{	// We cannot afford to do a IsValid check again here
		// As the action was taken it must have passed a previous IsValid check
		SetExpirationExpired();
		return;
	}

	if (!pAI->m_DeviceMove.IsAvailable())
	{	// Nobody, I mean NOBODY steals the movedevice from Restrict, capiche?
		pAI->m_DeviceMove.Free();
	}
	
	pAI->ActivateAnimgraph();
	CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
	if (pTarget)
	{
		OnTrackAgent(pTarget,pAI->m_Reaction * 5,false);
	}

	// First we must update m_Pos to the current location of restrict object
	if (m_iObject)
	{
		CWObject* pObj = pAI->m_pServer->Object_Get(m_iObject);
		if (pObj)
		{
			// Ignore if noclip
			CAI_Core* pRestrictAI = pAI->GetAI(m_iObject);
			if ((pRestrictAI)&&(CWObject_Character::Char_GetPhysType(pRestrictAI->m_pGameObject) == PLAYER_PHYS_NOCLIP))
			{
				m_bIsRestricted = false;
				INVALIDATE_POS(m_RestrictPos);
				return;
			}
			CVec3Dfp32 Pos = pObj->GetPosition();
			if (Pos.DistanceSqr(m_RestrictPos) > Sqr(Min(m_MaxRange, 16.0f)))
			{
				m_RestrictPos = Pos;
			}
		}
	}

	m_PathPos = pAI->m_PathFinder.SafeGetPathPosition(m_RestrictPos,4,2);

	// We should move towards the center
	// Actually, as the action will stop before hitting our target spot we will not look all that robotlike
	pAI->OnMove(m_PathPos,pAI->m_ReleaseIdleSpeed,false,false,NULL);
	// pAI->OnBasicInvestigatePos(m_PathPos, pAI->m_ReleaseIdleSpeed,0.0f);
	SetExpiration(pAI->m_Patience);

	m_bIsRestricted = true;
	pAI->m_DeviceMove.Use();
};

void CAI_Action_Restrict::OnStart()
{
	CAI_Action::OnStart();
	CAI_Core* pAI = AI();
	CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
	if (!pTarget)
	{
		pTarget = m_pAH->AcquireHostile();
	}
	if ((pTarget)&&(pTarget->GetCurRelation() >= CAI_AgentInfo::ENEMY)&&(pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED))
	{
		pAI->UseRandom("Restricted from enemy",CAI_Device_Sound::COVER_TAUNT,PRIO_COMBAT);
	}
}

void CAI_Action_Restrict::OnQuit()
{
	m_bIsRestricted = false;
	CAI_Action::OnQuit();
}

void CAI_Action_Restrict::OnDeltaLoad(CCFile* _pFile)
{
	int32 Temp32;
	int8 Temp8;
	fp32 TempFloat;
	_pFile->ReadLE(Temp8); m_bActive = (Temp8 != 0);
	_pFile->ReadLE(Temp32); m_iObject = Temp32;
	_pFile->ReadLE(TempFloat); m_MaxRange = TempFloat;
	m_RestrictPos.Read(_pFile);
	CAI_Action::OnDeltaLoad(_pFile);
};

void CAI_Action_Restrict::OnDeltaSave(CCFile* _pFile)
{
	//Save all attributes that can be set via parameters
	int32 Temp32;
	int8 Temp8;
	fp32 TempFloat;
	Temp8 = m_bActive; _pFile->WriteLE(Temp8);
	Temp32 = m_iObject; _pFile->WriteLE(Temp32);
	TempFloat = m_MaxRange; _pFile->WriteLE(TempFloat);
	m_RestrictPos.Write(_pFile);
	CAI_Action::OnDeltaSave(_pFile);
};



//CAI_Action_Hold///////////////////////////////////////////////////////////////////////////////
const char * CAI_Action_Hold::ms_lStrFlags[] = 
{
	"NoPos",
	NULL,
};


CAI_Action_Hold::CAI_Action_Hold(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = HOLD;
	m_TypeFlags = TYPE_MOVE;
	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_LookDir);
	m_bPathPos = false;
	m_UpdateTick = 0;
	m_Flags = 0;
	m_Stance = -1;		// Ignore if -1
	m_OrgStance = -1;	// Ignore if -1	
	m_bWalkStop = false;
};

// Set the parameters
void CAI_Action_Hold::SetActionOverride(int _iBehaviour, CStr _sParamName, fp32 _DistParam, fp32 _MaxDistParam, int _iActivator)
{
	//Assume that _sParamName actually may be a string of the format <param name>;<distance param>;<max distance param>
	int iPos = _sParamName.Find(";");
	if (iPos != -1)
	{
		//";" detected. Get string after ";"
		CStr sDistParams = _sParamName.Right(_sParamName.Len() - iPos - 1); 

		//Trim param name
		_sParamName = _sParamName.Left(iPos);

		//Extract distance params
		if ((iPos = sDistParams.Find(";")) != -1)
		{
			if (iPos == 0)
				//No distance param
				_DistParam = -1.0f;
			else
				_DistParam =  (fp32)sDistParams.Left(iPos).Val_fp64();

			if (iPos == sDistParams.Len() - 1)
				//No maxdistance param
				_MaxDistParam = -1.0f;
			else
				_MaxDistParam =  (fp32)sDistParams.Right(sDistParams.Len() - iPos - 1).Val_fp64();
		}
		else
		{
			//No maxdistance param
			_MaxDistParam = -1.0f;

			if (_sParamName.Len() == 0)
				//No distance param
				_DistParam = -1.0f;
			else
				_DistParam =  (fp32)sDistParams.Val_fp64();			
		}
	};

	// Now we have all the information we need
	// Find the position of random object with given name 
	CWObject* pObj = AI()->GetSingleTarget(0, _sParamName,AI()->m_pGameObject->m_iObject);
	if (pObj) 
	{	//Object found
		CMat4Dfp32 Mat = pObj->GetPositionMatrix();
		m_Destination = Mat.GetRow(3);
		m_LookDir = Mat.GetRow(0);
		AI()->m_HoldPosition = m_Destination;
		AI()->m_HoldLookDir = m_LookDir;
	}
	else
	{
		// No position found, hold on current position with current facing
		if (INVALID_POS(AI()->m_HoldPosition))
		{
			m_Destination = AI()->GetBasePos();
			AI()->m_HoldPosition = m_Destination;
		}
		if (INVALID_POS(AI()->m_HoldLookDir))
		{
			m_LookDir = AI()->GetLookDir();
			AI()->m_HoldLookDir = m_LookDir;
		}
	}
}

//Set params
void CAI_Action_Hold::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_POSOBJECT:
		{
			CWObject * pObj = AI()->m_pServer->Object_Get(_Val);
			if (pObj)
			{
				CMat4Dfp32 Mat = pObj->GetPositionMatrix();
				m_Destination = Mat.GetRow(3);
				m_LookDir = Mat.GetRow(0);
				AI()->m_HoldPosition = m_Destination;
				AI()->m_HoldLookDir = m_LookDir;
			}
		}
		break;
	case PARAM_FLAGS:
		{
			m_Flags = _Val;
		}
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}
void CAI_Action_Hold::SetParameter(int _Param, const CVec3Dfp32& _Val)
{
	switch (_Param)
	{
	case PARAM_POS:
		m_Destination = _Val;
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}
void CAI_Action_Hold::SetParameter(int _Param, CStr _Val)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_POS:
		{
			CVec3Dfp32 Vec;
			Vec.ParseString(_Val);
			SetParameter(_Param, Vec);
		}
		break;
	case PARAM_POSOBJECT:
		{
			CWObject* pObj = AI()->GetSingleTarget(0,_Val,AI()->m_pGameObject->m_iObject);
			if (pObj) 
			{	//Object found
				CMat4Dfp32 Mat = pObj->GetPositionMatrix();
				m_Destination = Mat.GetRow(3);
				m_Destination = Mat.GetRow(0);
				AI()->m_HoldPosition = m_Destination;
				AI()->m_HoldLookDir = m_LookDir;
			}
			else
			{
				// No position found, hold on current position with current facing
				if (INVALID_POS(m_Destination))
				{
					if (INVALID_POS(AI()->m_HoldPosition))
					{
						AI()->m_HoldPosition = AI()->GetBasePos();
					}
					m_Destination = AI()->m_HoldPosition;
				}
				if (INVALID_POS(m_LookDir))
				{
					if (INVALID_POS(AI()->m_HoldLookDir))
					{
						AI()->m_HoldLookDir = AI()->GetLookDir();
					}
					m_LookDir = AI()->m_HoldLookDir;
				}
			}
		}
		break;
	case PARAM_FLAGS:
		{
			SetParameter(_Param, StrToFlags(_Val));
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}


//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_Hold::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "POS")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_VEC;			
		return PARAM_POS;
	}
	else if (_Str == "POSOBJECT")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_TARGET;			
		return PARAM_POSOBJECT;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_FLAGS;			
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};


//Get flags for given string
int CAI_Action_Hold::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};


//Mandatory: Move; Optional: Look
int CAI_Action_Hold::MandatoryDevices()
{
	return (1 << CAI_Device::MOVE);
};


//Holding at current location is a pretty neutral action, slightly safe and of course lazy 
CAI_ActionEstimate CAI_Action_Hold::GetEstimate()
{
	CAI_ActionEstimate Est;

	// No position found, hold on current position with current facing
	if (INVALID_POS(m_Destination))
	{
		if (INVALID_POS(AI()->m_HoldPosition))
		{
			AI()->m_HoldPosition = AI()->GetBasePos();
		}
		m_Destination = AI()->m_HoldPosition;
	}
	if (INVALID_POS(m_LookDir))
	{
		if (INVALID_POS(AI()->m_HoldLookDir))
		{
			AI()->m_HoldLookDir = AI()->GetLookDir();
		}
		m_LookDir = AI()->m_HoldLookDir;
	}

	if (IsValid())
	{
		//Default score 65
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 0); 
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 10);
		Est.Set(CAI_ActionEstimate::EXPLORATION, -10);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, 75);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, -10);

		//Check if not at position
		if ((VALID_POS(m_Destination))&&
			(m_Destination.DistanceSqr(AI()->GetBasePos()) > Sqr(64)))
		{
			//Not at position (50)
			Est.Set(CAI_ActionEstimate::DEFENSIVE, 5);
			Est.Set(CAI_ActionEstimate::EXPLORATION, 5);
			Est.Set(CAI_ActionEstimate::LOYALTY, 20);
			Est.Set(CAI_ActionEstimate::LAZINESS, 20);
			Est.Set(CAI_ActionEstimate::VARIETY, 0);
		}
	}
	return Est;
};


//Stay put, or go to position and stay put.
void CAI_Action_Hold::OnTakeAction()
{	
	MSCOPESHORT(CAI_Action_Hold::OnTakeAction);
	CAI_Core* pAI = AI();

	if ((m_Stance > -1)&&(m_Stance > pAI->m_DeviceStance.GetIdleStance()))
	{
		pAI->m_DeviceStance.SetIdleStance(m_Stance);
	}

	if (!(m_Flags & FLAGS_NOPOS)&&
		(pAI->GetAITick() > m_UpdateTick))
	{
		//Get position if we don't have any
		if (INVALID_POS(m_Destination))
		{
			m_Destination = pAI->m_PathFinder.GetPathPosition(pAI->m_pGameObject);
			m_Flags &= ~FLAGS_ATPOS;
			if (VALID_POS(m_Destination))
				m_Destination = true;
		}
		else if (!m_bPathPos)
		{
			CVec3Dfp32 PathPos = pAI->m_PathFinder.GetPathPosition(m_Destination, -1, 10);
			if (INVALID_POS(PathPos))
			{
				//Can't get path position of given position, use own position instead
				m_Destination = pAI->m_PathFinder.GetPathPosition(pAI->m_pGameObject);
				if (VALID_POS(m_Destination))
					m_bPathPos = true;
			}
			else
			{
				m_Destination = PathPos;
				m_bPathPos = true;
			}
			m_Flags &= ~FLAGS_ATPOS;
		}

		//(Re)set default heading if we've got a specified look direction
		if (VALID_POS(m_LookDir))
		{
			OnTrackDir(m_LookDir,0,10);
		}

		m_UpdateTick = (int)(pAI->GetAITick() + 5.0f * pAI->GetAITicksPerSecond());
	}


	//Do we have a position to hold?
	if ((m_Flags & FLAGS_NOPOS)||
		(INVALID_POS(m_Destination)))
	{
		//We have no position to hold. Just stay put.
		OnTrackDir(m_LookDir,0,10);
		SetExpiration(100, 200);
	}
	else
	{
		//Always return to position after leaving it.
		fp32 MaxRange = 32.0f;

		//Clumsy characters are allowed to move away from position when turning to align with look
		if ((pAI->m_UseFlags & CAI_Core::USE_CLUMSY) && (m_Flags & FLAGS_ATPOS))
		{
			MaxRange = 48.0f;
		}

		//Have we left position?
		if ((pAI->GetBasePos()).DistanceSqr(m_Destination) > Sqr(MaxRange))
		{	//Not at position, go there
			if (m_bWalkStop)
			{
				if (pAI->CheckWalkStop(m_Destination))
				{
					m_bWalkStop = false;
				}
			}

			m_Flags &= ~FLAGS_ATPOS;
			pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,false,false,NULL);
			if (pAI->m_bReachedPathDestination)
			{
				pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
				pAI->ResetPathSearch();
				pAI->ResetStuckInfo();
			};
		}
		else
		{
			//We're back, align with look
			m_Flags |= FLAGS_ATPOS;
			OnTrackDir(m_LookDir,0,10);
			pAI->ResetStuckInfo();
		};

		SetExpiration(100, 200);
	};
};

void CAI_Action_Hold::OnStart()
{
	CAI_Action::OnStart();
	m_OrgStance = AI()->m_DeviceStance.GetIdleStance();
	m_bWalkStop = true;
};

//Reset path stuff
void CAI_Action_Hold::OnQuit()
{
	CAI_Core* pAI = AI();
	if ((m_Stance != -1)&&(m_OrgStance != -1))
	{
		pAI->m_DeviceStance.SetIdleStance(m_OrgStance);
	}
	m_OrgStance = -1;
	
	//Reset path and follow pos
	pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
	pAI->ResetPathSearch();
	pAI->ActivateAnimgraph();
	m_Flags &= ~FLAGS_ATPOS;

	CAI_Action::OnQuit();
};

void CAI_Action_Hold::OnDeltaLoad(CCFile* _pFile)
{
	m_Destination.Read(_pFile);
	m_LookDir.Read(_pFile);
};

void CAI_Action_Hold::OnDeltaSave(CCFile* _pFile)
{
	m_Destination.Write(_pFile);
	m_LookDir.Write(_pFile);
};


//CAI_Action_PatrolPath/////////////////////////////////////////////////////////////////////////
CAI_Action_PatrolPath::CAI_Action_PatrolPath(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = PATROL_PATH;
	m_TypeFlags = TYPE_MOVE;
	m_iPath = 0; 
	m_bReturningToPath = false;
	m_bStop = false;
	m_PathPosition = CVec3Dfp32(_FP32_MAX);

	m_Stance = -1;		// Ignore if -1
	m_OrgStance = -1;	// Ignore if -1	
};


//Set params
void CAI_Action_PatrolPath::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_PATH:
		{	// Set the path param but also stop the ep
			m_iPath = _Val;
			CAI_Core* pAI = AI();
			if ((pAI)&&(pAI->m_pGameObject))
			{
				pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, pAI->m_pGameObject->m_iObject), m_iPath);
			}
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}
void CAI_Action_PatrolPath::SetParameter(int _Param, CStr _Val)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_PATH:
		{
			CWObject * pPath = AI()->GetSingleTarget(0, _Val, AI()->m_pGameObject->m_iObject);
			if (pPath)
				SetParameter(_Param, pPath->m_iObject);
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}


//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_PatrolPath::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "PATH")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_TARGET;
		return PARAM_PATH;
	}
	/*
	else if (_Str == "AI_STANCE")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_INT;
		return PARAM_STANCE;
	}
	*/
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};


//Mandatory: Move; Optional: Look
int CAI_Action_PatrolPath::MandatoryDevices()
{
	return (1 << CAI_Device::MOVE);
};


//Valid if there's a path
bool CAI_Action_PatrolPath::IsValid()
{
	return CAI_Action::IsValid() && 
		   AI()->m_pServer->Object_Get(m_iPath);	
};


//Good exploratory, otherwise pretty neutral.
CAI_ActionEstimate CAI_Action_PatrolPath::GetEstimate()
{
	CAI_ActionEstimate Est;
	if (IsValid())
	{
		//Default score 65
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 50);
		Est.Set(CAI_ActionEstimate::LOYALTY, 10);
		Est.Set(CAI_ActionEstimate::LAZINESS, -5);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 10);
	}
	return Est;
};


//Move along a path, looking in the direction of the path if possible
void CAI_Action_PatrolPath::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_PatrolPath::OnTakeAction);
	CAI_Core* pAI = AI();

	//Get path object
	pAI->ActivateAnimgraph();
	CWObject* pPath = pAI->m_pServer->Object_Get(m_iPath);

	//We have not spotted an enemy, do we have a path to patrol?
	if (pPath)
	{
#ifndef M_RTM
		if (pAI->DebugTarget())
		{
			pAI->DebugDrawPath(m_iPath,100);
		}
#endif	
		//We have a path
		if (!m_bReturningToPath &&
			(INVALID_POS(m_PathPosition)))
		{
			m_PathPosition = pAI->GetEnginePathPosition(m_iPath);
		}
		
		// If we're close enough to the PLAYER and he is to the front we pause
		// Stop for player unless he is an enemy
		CAI_AgentInfo* pPlayerAgent = m_pAH->StopForPlayer();
		if (pPlayerAgent)
		{
			pAI->ResetStuckInfo();
			pAI->m_DeviceMove.Use();
			return;
		}

		// Stop patrol if restricted
		if ((VALID_POS(m_PathPosition))&&(m_pAH->IsRestricted(m_PathPosition)))
		{	
			if (pAI->DebugRender())
			{
				CStr Name = pAI->m_pGameObject->GetName();
				ConOut(Name+CStr(": PatrolPath restricted"));
			}
			SetExpirationExpired();
			return;
		}

		// Don't set stance until after restriction check
		if ((m_Stance > -1)&&(m_Stance > pAI->m_DeviceStance.GetIdleStance()))
		{
			pAI->m_DeviceStance.SetIdleStance(m_Stance);
		}

		CVec3Dfp32 LastMove = pAI->GetBasePos() - pAI->GetLastBasePos();
		fp32 LastMoveLen = LastMove.Length();
		if (LastMoveLen < pAI->m_IdleMaxSpeed)
		{
			LastMoveLen = pAI->m_IdleMaxSpeed;
		}
		else if (LastMoveLen > pAI->GetRunStepLength())
		{
			LastMoveLen = pAI->GetRunStepLength();
		}

		//Check distance to path position
		fp32 DistSqr;
		if (pAI->m_bWallClimb)
		{
			DistSqr = m_PathPosition.DistanceSqr(pAI->GetBasePos());
		}
		else
		{
			DistSqr = CAI_Core::XYDistanceSqr(pAI->GetBasePos(), m_PathPosition);
		}
		
		pAI->Debug_RenderWire(pAI->GetBasePos(), m_PathPosition);//DEBUG
		pAI->Debug_RenderWire(m_PathPosition, m_PathPosition + CVec3Dfp32(0,0,100), 0xffffffff, 0);
		
		//If we've fallen behind, check where we need to go, or if we've caught up with the path.
		if (m_bReturningToPath)
		{
			//Are we still behind the path?
			if (DistSqr < pAI->m_CaughtUpRangeSqr)
			{
				//We've caught up!
				m_bReturningToPath = false;
				m_PathPosition = pAI->GetEnginePathPosition(m_iPath);
				if (pAI->m_bWallClimb)
				{
					DistSqr = m_PathPosition.DistanceSqr(pAI->GetBasePos());
				}
				else
				{
					DistSqr = CAI_Core::XYDistanceSqr(pAI->GetBasePos(), m_PathPosition);
				}
				m_bStop = false;
				pAI->ResetStuckInfo();	
			}
		}
		else
		{
			//Have we fallen behind just now?
			if (DistSqr > pAI->m_FallenBehindRangeSqr)
			{	//Jupp, we must catch up with path. 
				m_bReturningToPath = true;
				if (pAI->m_bWallClimb)
				{
					m_PathPosition = pAI->m_PathFinder.GetPathPosition(m_PathPosition,2,2);
				}
				else
				{
					m_PathPosition = pAI->m_PathFinder.GetPathPosition(m_PathPosition,10,2);
				} 
				pAI->ResetStuckInfo();	
			};
		};
		
		//Should we follow path normally or are we behind and need to navigate our way to the path
		if (m_bReturningToPath) 
		{
			//We need to navigate if possible
			if (INVALID_POS(m_PathPosition))
			{
				//Return position invalid, Propel path a bit so that we hopefully can get a position next frame..
				CMat4Dfp32 Mat;
				if (pAI->m_bWallClimb)
				{
					pAI->PropelPath(m_iPath, 32.0f, true, false, true, &Mat);
					m_PathPosition = pAI->m_PathFinder.GetPathPosition(CVec3Dfp32::GetRow(Mat,3),2,3);
				}
				else
				{
					pAI->PropelPath(m_iPath, 32.0f, true, true, true, &Mat);
					m_PathPosition = pAI->m_PathFinder.GetPathPosition(CVec3Dfp32::GetRow(Mat,3),10,3);
				} 
				pAI->ResetPathSearch();
			}
			
			if (INVALID_POS(m_PathPosition))
			{
				//Return position still invalid. Stay put.
				pAI->m_DeviceMove.Use();
			}
			else
			{
				//Return position ok! Stop path, and allow escape sequences
				pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, pAI->m_pGameObject->m_iObject), m_iPath);
				int32 moveResult = pAI->OnMove(m_PathPosition, pAI->m_IdleMaxSpeed,false,false,NULL);
			}
		}
		else 
		{
			fp32 PropelStep = 2.0f * LastMoveLen;
			if (pAI->GetPrevSpeed() >= pAI->m_ReleaseMaxSpeed * 0.75f)
			{	// Running
				PropelStep = Max3(PropelStep, 2.0f * pAI->m_MinPropelStep, 2.0f * pAI->m_IdleMaxSpeed);
			}
			else
			{	// Walking
				PropelStep = Max3(PropelStep, pAI->m_MinPropelStep, 2.0f * pAI->m_IdleMaxSpeed);
			}
			// fp32 PropelStep = Max(pAI->m_MinPropelStep, 2.0f * LastMoveLen);
			//Should use sqr of propelstep really, unless there's some special reason. Backman?
			// fp32 PropelRangeSqr = Sqr(Max(pAI->m_MinPropelStep, pAI->m_IdleMaxSpeed));
			fp32 PropelRangeSqr = Sqr(PropelStep);

			//We're following path normally. Propel path each frame when at stop, 
			//or propel to new position whenever we get close to path
			bool bPropelled = false;
			if (m_bStop || (DistSqr < PropelRangeSqr))
			{
				//Since we allow stops when propelling, path won't move when at a stop until it has 
				//been propelled the number of frames it should stop, regardless of distance
				CVec3Dfp32 PrevPos = m_PathPosition;
				CMat4Dfp32 Mat;
				
				if (pAI->m_bWallClimb)
				{
					pAI->PropelPath(m_iPath, PropelStep, true, false, true, &Mat);
				}
				else
				{
					pAI->PropelPath(m_iPath, PropelStep, true, true, true, &Mat);
				}
				m_PathPosition = CVec3Dfp32::GetRow(Mat,3);
				pAI->ResetStuckInfo();
				bPropelled = true;
				
				//Check if (still) at stop
				if (m_PathPosition.AlmostEqual(PrevPos, 0.1f))
					m_bStop = true;
				else
					m_bStop = false;
			}
			//Check if we've just become stuck
			else if (!pAI->m_bNoEscapeSeqMoves && !pAI->m_bIsStuck && pAI->IsStuck(m_PathPosition))
			{
				//We're stuck. Propel path a bit and reset stop flag. 
				//We'll start an escape move below.
				if (pAI->m_bWallClimb)
				{
					pAI->PropelPath(m_iPath,32.0f,true,true,NULL);
				}
				else
				{
					pAI->PropelPath(m_iPath,32.0f,true,true,NULL);
				}
				m_bStop = false;
				pAI->m_bIsStuck = true;
			}
			
			//Should we stop?
			if (m_bStop)
			{
				pAI->SetDestination(m_PathPosition);
				pAI->m_DeviceMove.Use();
				pAI->ResetStuckInfo();	
			}
			//Are we stuck?
			else if (pAI->m_bIsStuck)
			{
				//Escape sequence move
				//pAI->OnEscapeSequenceMove(pAI->m_IdleMaxSpeed);
				pAI->OnEscapeSequenceMove(LastMoveLen);//??? 
			}
			else
			{
				//Go to path position
				CVec3Dfp32 Pos = pAI->GetBasePos();
				if (Pos.DistanceSqr(m_PathPosition) >= 1.0f)
				{
					if (pAI->m_DeviceLook.IsAvailable())
					{
						CMat4Dfp32 AheadMat;
						pAI->PropelPath(m_iPath,32.0f,true,false,false,&AheadMat);
						// *** Experimental first pass at ep lookahead
						CVec3Dfp32 LookPos = AheadMat.GetRow(3);
						if (VALID_POS(LookPos))
						{
							LookPos += pAI->GetUp() * pAI->GetCurHeight() * 0.75f;
							pAI->OnTrackPos(LookPos,10,false,false);
#ifndef M_RTM
							// *** Remove this when LookAhead is OK:ed
							if (pAI->DebugTarget())
							{
								pAI->Debug_RenderVertex(LookPos,kColorYellow,1.0f);
							}
#endif
						}
					}
					// Must do move AFTER look ahead
					pAI->AddMoveTowardsPositionCmd(m_PathPosition, pAI->m_IdleMaxSpeed);
				}
				else
				{
					pAI->m_DeviceMove.Use();
				}

				//If not propelled, propel path an extra time every few frames to avoid zig-zag behaviour
				if (!bPropelled && (pAI->m_Timer % 10 == 0))
				{
					CMat4Dfp32 Mat;
					if (pAI->m_bWallClimb)
					{
						pAI->PropelPath(m_iPath, Min(32.0f, PropelStep), true, false, true, &Mat);
					}
					else
					{
						pAI->PropelPath(m_iPath, Min(32.0f, PropelStep), true, true, true, &Mat);
					}
					m_PathPosition = CVec3Dfp32::GetRow(Mat,3);
					pAI->ResetStuckInfo();
				}
			}
		};

		if (!m_bStop)
		{
			pAI->NoStops();
		}

		// Why actually should we ever expire?
		SetExpirationIndefinite();
	}
	else
	{
		SetExpirationExpired();
	}
};

void CAI_Action_PatrolPath::OnStart()
{
	CAI_Action::OnStart();
	m_OrgStance = AI()->m_DeviceStance.GetIdleStance();
};


//Reset path stuff
void CAI_Action_PatrolPath::OnQuit()
{
	if (AI())
	{
		if ((m_Stance != -1)&&(m_OrgStance != -1))
		{
			AI()->m_DeviceStance.SetIdleStance(m_OrgStance);
		}
		m_OrgStance = -1;

		//Reset stuff
		AI()->m_PathDestination = CVec3Dfp32(_FP32_MAX);
		AI()->ResetPathSearch();
		m_bReturningToPath = false;
		m_bStop = false;
	}
	CAI_Action::OnQuit();
};

void CAI_Action_PatrolPath::OnDeltaLoad(CCFile* _pFile)
{
	int32 Temp32;
	_pFile->ReadLE(Temp32); m_iPath = Temp32;
	CAI_Action::OnDeltaLoad(_pFile);
};

void CAI_Action_PatrolPath::OnDeltaSave(CCFile* _pFile)
{
	//Save all attributes that can be set via parameters
	int32 Temp32;
	Temp32 = m_iPath; _pFile->WriteLE(Temp32);
	CAI_Action::OnDeltaSave(_pFile);
};




//CAI_Action_Roam////////////////////////////////////////////////////////////////////////////
const char* CAI_Action_Roam::ms_lStrFlags[] = 
{	
	"PATHFIND",
	"RUN",
	"SCENEPTS",
	"RANDOMPTS",
	"CLOSEST",
	"NEARPLAYER",
	"PLAYERFOV",
	"HEADING",
	"AVOIDLIGHT",
	"PREFERLIGHT",
	"DYNAMIC",
	"JUMP",

	NULL,
};

CAI_Action_Roam::CAI_Action_Roam(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = ROAM;
	m_TypeFlags = TYPE_MOVE;
	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_Flags = FLAGS_PATHFINDING | FLAGS_SCENEPOINTS | FLAGS_DYNAMIC;
	m_StayTimeout = -1;

	m_bWalkStop = false;
	m_pScenePoint = NULL;

	// Members for handling jumping movement
	m_bAirborne = false;

	m_JumpVelocity = CVec3Dfp32(0,0,1);
	m_JumpDurationTicks = 0;
	m_JumpTicks = 0;
};

void CAI_Action_Roam::SetParameter(int _Param, fp32 _Val)
{
	CAI_Action::SetParameter(_Param, _Val);
}

void CAI_Action_Roam::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_FLAGS:
		m_Flags = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Roam::SetParameter(int _Param, CStr _Val)
{
	switch (_Param)
	{
	case PARAM_FLAGS:
		{
			int Val = StrToFlags(_Val);
			SetParameter(_Param,Val);
		}
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}

}

//Get parameter ID from sting (used when parsing registry)
int CAI_Action_Roam::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
}

//Get flags for given string
int CAI_Action_Roam::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
}

//Mandatory: Move; Optional: Look
int CAI_Action_Roam::MandatoryDevices()
{
	return (1 << CAI_Device::MOVE);
};

bool CAI_Action_Roam::IsValid()
{
	if (!CAI_Action::IsValid())
	{
		return(false);
	}
	CAI_Core* pAI = AI();
	if (pAI->GetStealthTension() >= CAI_Core::TENSION_COMBAT)
	{	// No ROAM when fighting continues
		return(false);
	}
	if ((pAI->m_DeviceStance.GetIdleStance() > CAI_Device_Stance::IDLESTANCE_IDLE)&&(pAI->m_DeviceStance.GetMinStance() < CAI_Device_Stance::IDLESTANCE_HOSTILE))
	{	// No ROAM unless stance is IDLE (and we can hold that stance)
		return(false);
	}

	return(true);
};

//Very good exploratory, somewhat unsafe
CAI_ActionEstimate CAI_Action_Roam::GetEstimate()
{
	CAI_ActionEstimate Est;
	CAI_Core* pAI = AI();
	if (IsValid())
	{
		//Check for available scene point type
		if ((!(m_Flags & FLAGS_IN_PLAYERFOV))&&
			(m_Flags & FLAGS_SCENEPOINTS)&&
			(!m_pAH->ScenePointTypeAvailable(CWO_ScenePoint::ROAM,pAI->m_RoamMaxRange)))
		{
			return Est;
		}

		//Default score 65
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 0); 
		Est.Set(CAI_ActionEstimate::DEFENSIVE, -10);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 75);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, -20);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 20);
	}
	return Est;
};

// Tries to find a destination position from available scenepoints
bool CAI_Action_Roam::FindScenePointDestination()
{
	CAI_Core* pAI = AI();

	// We already have a valid scenepoint?
	if (m_pScenePoint)
	{
		if (m_pAH->RequestScenePoint(m_pScenePoint))
		{
			return(true);
		}
		else
		{	// No, it's not OUR job to set the memory scenepoints
			m_pScenePoint = NULL;
		}
	}

	pAI->SetObjColl(true);
	CVec3Dfp32 OurPos = pAI->GetBasePos();
	int Flags = CAI_ActionHandler::SP_PREFER_HEADING;
	if (!(m_Flags & FLAGS_CLOSEST_PT))
	{
		Flags |= CAI_ActionHandler::SP_RANDOM_RANGE;
	}
	if (m_Flags & FLAGS_NEARPLAYER)
	{
		int32 playerID = pAI->GetClosestPlayer();
		if ((playerID)&&(playerID != pAI->m_pGameObject->m_iObject))
		{
			CWObject* pObj = pAI->m_pServer->Object_Get(playerID);
			if (pObj)
			{
				OurPos = pObj->GetPosition();
				Flags |= CAI_ActionHandler::SP_PREFER_NEAR_TARGETPOS;
			}
		}
	}
	if (m_Flags & FLAGS_IN_PLAYERFOV)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_PLAYERFOV;
	}
	if (m_Flags & FLAGS_PREFER_HEADING)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_HEADING;
	}
	if (m_Flags & FLAGS_AVOID_LIGHT)
	{
		Flags |= CAI_ActionHandler::SP_AVOID_LIGHT;
	}
	if (m_Flags & FLAGS_PREFER_LIGHT)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_LIGHT;
	}
	
	int SPType = CWO_ScenePoint::ROAM;
	if (m_Flags & FLAGS_DYNAMIC)
	{
		SPType |= CWO_ScenePoint::DYNAMIC;
	}
	if (m_Flags & FLAGS_JUMP)
	{ 
		SPType |= CWO_ScenePoint::JUMP_CLIMB;
	}
	if (pAI->m_bWallClimb)
	{
		SPType |= CWO_ScenePoint::WALK_CLIMB;
	}

	CMat4Dfp32 OurMat;
	pAI->GetBaseMat(OurMat);
	m_pScenePoint = m_pAH->GetBestScenePoint(SPType,Flags,pAI->m_RoamMinRange,pAI->m_RoamMaxRange,OurMat,OurPos,0.0f,0);	
	INVALIDATE_POS(m_Destination);
	m_bWalkStop = false;
	if (!m_pScenePoint)
	{
		return(false);
	}
	if (m_pScenePoint->GetType() & (CWO_ScenePoint::JUMP_CLIMB | CWO_ScenePoint::WALK_CLIMB))
	{
		m_Destination = m_pScenePoint->GetPosition();
	}
	else
	{
		m_Destination = pAI->m_PathFinder.GetPathPosition(m_pScenePoint->GetPosition(),4,4);
	}

	if ((!m_pAH->RequestScenePoint(m_pScenePoint))||(INVALID_POS(m_Destination))||(m_pAH->IsRestricted(m_Destination)))
	{
		INVALIDATE_POS(m_Destination);
		m_pAH->UpdateScenepointHistory(m_pScenePoint);
		m_pScenePoint = NULL;
		return(false);
	}

	pAI->SetDestination(m_Destination);
	// Is destination walkstoppable
	if ((!m_pScenePoint->IsWaypoint())&&(m_pScenePoint->GetSqrRadius() > Sqr(16.0f)))
	{
		m_bWalkStop = true;
	}
#ifndef M_RTM
	m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif

	return(true);
};

bool CAI_Action_Roam::FindRandomDestination()
{
	CAI_Core* pAI = AI();
	if ((!pAI)||(!pAI->m_PathFinder.GridPF()))
	{
		SetExpirationExpired();
		return(false);
	}

	if (m_Flags & FLAGS_NEARPLAYER)
	{
		int32 iPlayer = pAI->GetClosestPlayer();
		CAI_AgentInfo* pInfoPlayer = pAI->m_KB.GetAgentInfo(iPlayer);
		if (pInfoPlayer)
		{	// We don't worry about bumping into the player
			CVec3Dfp32 Pos = pInfoPlayer->GetBasePos();
			fp32 SqrDistance = pAI->SqrDistanceToUs(Pos);
			if ((SqrDistance >= Sqr(pAI->m_RoamMinRange))&&(SqrDistance <= Sqr(pAI->m_RoamMaxRange)))
			{
				m_pScenePoint = NULL;
				m_Destination = Pos;
				return(true);
			}
		}
	}

	// We don't have any scenepoints at the moment so we just generate random
	// movement directions (forward +- 90 degrees, 2-10m)
	CVec3Dfp32 OurPos,Dir,Goal,StopPos;
	OurPos = pAI->GetBasePos();
	Dir = pAI->GetLookAngles(pAI->m_pGameObject);
	fp32 Heading;
	if (Random > 0.1f)
	{	// Random direction in the front hemicircle
		Heading = Dir[2] + (Random - 0.5f) * 0.5f;
	}
	else
	{	// Sometimes we do a fully random turn
		Heading = Random;
	}
	if (Heading > 1.0f) {Heading -= 1.0f;}
	if (Heading < -1.0f) {Heading += 1.0f;}
	fp32 Range = pAI->m_RoamMinRange + (pAI->m_RoamMaxRange - pAI->m_RoamMinRange) * Random;
	Goal = OurPos + CVec3Dfp32(Range * QCos(Heading * _PI2), Range * QSin(-Heading * _PI2), 0);
	if (!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,Goal,
		(int)pAI->GetBaseSize(),
		(int)pAI->GetHeight(),
		pAI->m_bWallClimb,
		(int)pAI->GetStepHeight(),
		(int)pAI->GetJumpHeight(),
		&StopPos))
	{
		m_Destination = StopPos;
	}
	else
	{
		m_Destination = Goal;
	}

	if (OurPos.DistanceSqr(m_Destination) < Sqr(pAI->m_RoamMinRange))
	{
		m_Destination = CVec3Dfp32(_FP32_MAX);
		return(false);
	}

	// Stop roaming if restricted
	if ((VALID_POS(m_Destination))&&(m_pAH->IsRestricted(m_Destination)))
	{	
		INVALIDATE_POS(m_Destination);
		return(false);
	}
	pAI->SetDestination(m_Destination);
	m_bWalkStop = true;

	return(true);
}

bool CAI_Action_Roam::FindPositionInPlayerFOV()
{
	CAI_Core* pAI = AI();

	INVALIDATE_POS(m_Destination);

	int playerID = pAI->GetClosestPlayer();
	CAI_AgentInfo* pPlayerInfo = pAI->m_KB.GetAgentInfo(playerID);
	CAI_Core* pPlayerAI = pAI->GetAI(playerID);
	if ((pPlayerInfo)&&(pPlayerAI)&&(pPlayerAI->m_pGameObject))
	{	// Make a straight grid trace from player in player direction and set the pos as the destination
		CVec3Dfp32 PlayerPos = pPlayerInfo->GetBasePos();
		CVec3Dfp32 Look = pPlayerAI->GetLookDir();
		Look[2] = 0.0f;
		CVec3Dfp32 Goal = PlayerPos + Look * 96.0f;
		CVec3Dfp32 StopPos;
		if ((pAI->m_PathFinder.GetBlockNav())&&
			(!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(PlayerPos,Goal,
			(int)pAI->GetBaseSize(),
			(int)pAI->GetHeight(),
			pAI->m_bWallClimb,
			(int)pAI->GetStepHeight(),
			(int)pAI->GetJumpHeight(),
			&StopPos)))
		{	// Movement blocked
			m_Destination = StopPos;
		}
		else
		{
			m_Destination = Goal;
		}
	}
	
	// Stop roaming if restricted
	if ((VALID_POS(m_Destination))&&(m_pAH->IsRestricted(m_Destination)))
	{	
		INVALIDATE_POS(m_Destination);
		return(false);
	}
	m_bWalkStop = true;

	return(true);
};

bool CAI_Action_Roam::FindDestination()
{
	MSCOPESHORT(CAI_Action_Roam::FindDestination);

	if (VALID_POS(m_Destination))
	{
		if (m_pScenePoint)
		{
			if (!m_pAH->RequestScenePoint(m_pScenePoint))
			{
				INVALIDATE_POS(m_Destination);
				return(false);
			}
		}
		return(true);
	}

	CAI_Core* pAI = AI();
	m_StayTimeout = -1;
	if (m_Flags & FLAGS_SCENEPOINTS)
	{
		if (FindScenePointDestination())
		{	// OK we got a scenepoint
			// If it is a JUMP_CLIMB or if we're on a wall we should jump to the SP,
			// otherwise we do as per usual.
			if ((m_Flags & FLAGS_JUMP)&&(pAI->m_bCanWallClimb)&&(!pAI->m_pGameObject->AI_IsJumping()))
			{	// SP has JUMP_CLIMB or ai is on wall and SP has not JUMP_CLIMB | WALK_CLIMB
				if (m_pScenePoint->GetType() & CWO_ScenePoint::JUMP_CLIMB)
				{	// JUMP!
					CMat4Dfp32 Mat = m_pScenePoint->GetPositionMatrix();
					pAI->TempEnableWallClimb(pAI->m_Patience);
					if (pAI->AddJumpTowardsPositionCmd(&Mat,CAI_Core::JUMPREASON_IDLE,CAI_Core::JUMP_CHECK_COLL | CAI_Core::JUMP_TO_AIR))
					{
#ifndef M_RTM
						m_pAH->DebugDrawScenePoint(m_pScenePoint,true,0,5.0f);
#endif
						pAI->ResetPathSearch();
						m_bAirborne = true;
						return(false);
					}
					else
					{
#ifndef M_RTM
						m_pAH->DebugDrawScenePoint(m_pScenePoint,false,0,5.0f);
#endif
						if (!pAI->m_pGameObject->AI_IsOnWall())
						{
							pAI->SetWallclimb(false);
						}
						// Tweak for continued pathfind when a jump fails
						if (!(m_pScenePoint->GetType() & CWO_ScenePoint::WALK_CLIMB))
						{
							pAI->ResetPathSearch();
							INVALIDATE_POS(m_Destination);
							m_pAH->UpdateScenepointHistory(m_pScenePoint);
							m_pScenePoint = NULL;
						}
						return(false);
					}
				}
			}
			// If we're on the ground and the target scenepoint is neither WALK_CLIMB nor JUMP_CLIMB
			// we can switch off wallclimbing for a while
/* DEBUG: Back again when it works
			if (pAI->m_bCanWallClimb)
			{
				if ((!pAI->m_pGameObject->AI_IsOnWall())&&(!(m_pScenePoint->GetType() & (CWO_ScenePoint::WALK_CLIMB | CWO_ScenePoint::JUMP_CLIMB))))
				{
					pAI->TempDisableWallClimb(pAI->m_Patience);
				}
			}
*/
			pAI->SetDestination(m_Destination);
			return(true);
		}
	}

	if (m_Flags & FLAGS_RANDOM)
	{
		if (FindRandomDestination())
		{
			pAI->SetDestination(m_Destination);
			return(true);
		}
	}

	if ((!(m_Flags & FLAGS_SCENEPOINTS))&&(m_Flags & FLAGS_IN_PLAYERFOV))
	{
		if (FindPositionInPlayerFOV())
		{
			pAI->SetDestination(m_Destination);
			return(true);
		}
	}

	return(false);
};

bool CAI_Action_Roam::MoveToDestination()
{
	CAI_Core* pAI = AI();

	if (INVALID_POS(m_Destination))
	{
		m_StayTimeout = -1;
		if (m_pScenePoint)
		{
			m_pAH->UpdateScenepointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
		}
		return(false);
	}

	// Some basice useful values
	CVec3Dfp32 OurPos = pAI->GetBasePos();
	fp32 RangeSqr = m_Destination.DistanceSqr(OurPos);
	CVec3Dfp32 OurDir = pAI->GetLookDir();
		
	// Stop for player unless ENEMY
	// If we're close enough to the PLAYER and he is to the front we pause
	CAI_AgentInfo* pPlayerAgent = m_pAH->StopForPlayer();
	if (pPlayerAgent)
	{
		pAI->ResetStuckInfo();
		pAI->m_DeviceMove.Use();
		return(true);
	}

	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint)))
	{	// Badness, we can no longer use that scenepoint
		INVALIDATE_POS(m_Destination);
		return(false);
	}

	// Stop roaming if restricted
	if ((VALID_POS(m_Destination))&&(m_pAH->IsRestricted(m_Destination)))
	{	
		INVALIDATE_POS(m_Destination);
		return(false);
	}

	if (m_StayTimeout != -1)
	{
		return(true);
	}

	if (pAI->m_pGameObject->AI_IsJumping())
	{
		return(false);
	}

	if (m_pScenePoint)
	{
		if ((!m_pScenePoint->IsWaypoint())&&(m_pScenePoint->GetSqrRadius() >= Sqr(16.0f))&&(m_pScenePoint->GetCosArcAngle() <= 0.707f))
		{
			pAI->CheckWalkStop(m_Destination);
		}

		// TBD: Backman test this!
		if (m_pAH->AlignScenepoint(m_pScenePoint,PRIO_IDLE))
		{
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
			// ***
#ifndef M_RTM
			if (m_pScenePoint->GetDirection() * pAI->GetLookDir() < m_pScenePoint->GetCosArcAngle() - 0.0001f)
			{
				CStr Name = pAI->m_pGameObject->GetName();
				ConOut(Name+CStr(": AlignScenepoint() broken ")+m_pScenePoint->GetName());
			}
			// ***
#endif
			return(true);
		}
		else
		{
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif
			bool bMoveResult = true;
			if (pAI->m_DeviceMove.IsAvailable())
			{
				fp32 moveSpeed = pAI->m_ReleaseIdleSpeed;
				if (m_Flags & FLAGS_RUN)
				{
					moveSpeed = pAI->m_ReleaseMaxSpeed;
				}
				int32 moveResult = pAI->OnMove(m_Destination,moveSpeed,false,true,m_pScenePoint);
				if (!pAI->CheckMoveresult(moveResult,m_pScenePoint))
				{	// Couldn't move somehow
					m_pAH->UpdateScenepointHistory(m_pScenePoint);
					m_pScenePoint = NULL;
					SetExpirationExpired();
					return(false);
				}
			}
			return(false);
		}
	}

	if ((VALID_POS(m_Destination))&&(RangeSqr > Sqr(32.0f)))
	{
		pAI->SetDestination(m_Destination);
		int32 moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,false,true,NULL);
		return(false);
	}

	return(false);
};

// Returns 1 if we're staying at the scenepoint
bool CAI_Action_Roam::StayAtDestination()
{
	CAI_Core* pAI = AI();

	if (INVALID_POS(m_Destination))
	{
		m_StayTimeout = -1;
		return(false);
	}

	CVec3Dfp32 OurPos = pAI->GetBasePos();
//	fp32 RangeSqr = m_Destination.DistanceSqr(OurPos);
	CVec3Dfp32 OurDir = pAI->GetLookDir();

	if (m_pScenePoint)
	{
		// First we determine if we're there at all
		if (!m_pScenePoint->IsAt(OurPos))
		{
			INVALIDATE_POS(m_Destination);
			m_StayTimeout = -1;
			if (m_pScenePoint)
			{
				m_pScenePoint = NULL;
			}
			return(false);
		}

		// We're there dude, look sharp mon
		pAI->ResetStuckInfo();
		if (m_StayTimeout == -1)
		{	// We have not yet activated
			// if ((m_pAH->HandleSitFlag(m_pScenePoint))||(m_pScenePoint->IsAligned(OurDir)))
			if (true)
			{	
				pAI->m_DeviceMove.Use();
				// Is the AG behaviour ready?
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
				if (!(pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_BEHAVIORDISABLED))
				{
					int StayTicks = (int)(m_pScenePoint->GetBehaviourDuration() * pAI->GetAITicksPerSecond());
					m_StayTimeout = pAI->m_Timer + StayTicks;
					CWO_ScenePoint* orgSP = m_pScenePoint;
					m_pAH->ActivateScenePoint(m_pScenePoint,GetPriority());
					if (orgSP != m_pScenePoint) { return(false); }
					SetMinExpirationDuration(StayTicks);
					// Here we do some fancy schmancy waypoint checkup
					if (m_pScenePoint->IsWaypoint())
					{	// OK; the ActivateScenePoint above should stored our scenepoint in the history
						INVALIDATE_POS(m_Destination);
						m_StayTimeout = -1;
						m_pScenePoint = NULL;
						pAI->m_DeviceMove.Free();
						if (FindDestination())
						{
							MoveToDestination();
						}
					}
				}
				return(true);
			}
			else
			{	// We are not yet aligned
				pAI->m_DeviceMove.Use();
				// fp32 turntimeFactor = 2.0f - pAI->GetLookDir() * m_pScenePoint->GetDirection();
				pAI->OnTrackDir(m_pScenePoint->GetDirection(),0,20,false,false);
				m_StayTimeout = -1;
			}
		}
		else if (pAI->m_Timer > m_StayTimeout)
		{	// We're done dude
			INVALIDATE_POS(m_Destination);
			m_StayTimeout = -1;
			if (m_pScenePoint)
			{
				m_pScenePoint = NULL;
			}
			return(false);
		}

		if ((m_pScenePoint)&&(m_pScenePoint->GetBehaviour())&&(m_pScenePoint->GetBehaviourDuration() <= 0.0f))
		{
			m_StayTimeout = pAI->m_Timer + pAI->GetAITicksPerSecond();
		}
		return(true);
	}
	else
	{	// No scenepoint, just stay some time
		if (m_StayTimeout == -1)
		{
			int StayTicks = (int)((0.5f + Random) * 20);
			m_StayTimeout = pAI->m_Timer + StayTicks;
			SetMinExpirationDuration(StayTicks);
		}
		else if (pAI->m_Timer > m_StayTimeout)
		{	// We're done dude
			INVALIDATE_POS(m_Destination);
			m_StayTimeout = -1;
			m_pScenePoint = NULL;
			return(false);
		}
		return(true);
	}

	m_StayTimeout = -1;
	return(false);
};

void CAI_Action_Roam::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		INVALIDATE_POS(m_Destination);
		m_StayTimeout = -1;
		m_pScenePoint = NULL;
	}
}

void CAI_Action_Roam::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Roam::OnTakeAction);
	CAI_Core* pAI = AI();

	if ((pAI->HighTension())&&(pAI->GetStealthTension() >= CAI_Core::TENSION_HOSTILE))
	{
		pAI->ShutUp(CAI_Action::PRIO_COMBAT);
		ExpireWithDelay(60);
		return;
	}

	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();

	/*
	if ((m_Flags & FLAGS_IN_PLAYERFOV)&&(VALID_POS(m_Destination)))
	{
		if (!pAI->IsPlayerLookWithinAngle(45.0f,m_Destination))
		{
			INVALIDATE_POS(m_Destination);
			m_StayTimeout = -1;
			if (m_pScenePoint)
			{
				m_pScenePoint05 = m_pScenePoint04;
				m_pScenePoint04 = m_pScenePoint03;
				m_pScenePoint03 = m_pScenePoint02;
				m_pScenePoint02 = m_pScenePoint01;
				m_pScenePoint01 = m_pScenePoint;
				m_pScenePoint = NULL;
			}
		}
	}
	*/

	if (FindDestination())
	{	// We have a valid Destination
		if (MoveToDestination())
		{
			if (StayAtDestination())
			{
#ifndef M_RTM
				m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
			}
			else
			{
#ifndef M_RTM
				m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif
			}
		}
	}
	
};

void CAI_Action_Roam::OnStart()
{
	CAI_Action::OnStart();
	CAI_Core* pAI = AI();

	if (pAI->m_DeviceStance.GetIdleStance() > CAI_Device_Stance::IDLESTANCE_HOSTILE)
	{
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE); 
	}

	// Default member values
	m_StayTimeout = -1;
	pAI->m_PathDestination = CVec3Dfp32(_FP32_MAX);
	INVALIDATE_POS(m_Destination);
	m_bWalkStop = false;
};

//Reset path stuff
void CAI_Action_Roam::OnQuit()
{
	CAI_Core* pAI = AI();

	pAI->SetObjColl(true);
	if (m_pScenePoint)
	{
		m_pScenePoint = NULL;
	}

	CAI_Action::OnQuit();

	// AO:Should we reset destination (including scenepoint) stuff?
	// AB: Yes, we got to the scenepoint and stood there longer than its duration
	// This happens when activating a scenepoint starts a behaviour that is longer than the duration.
	// Upon finishing the behaviour Roam would otherwise take the scenepoint again if permitted by min range
	if ((m_StayTimeout != -1) && (pAI->m_Timer > m_StayTimeout))
	{
		SetExpirationExpired();
		return;
	}
};


//CAI_Action_Look/////////////////////////////////////////////////////////////////////////////////////
CAI_Action_Look::CAI_Action_Look(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = LOOK;
	m_TrackingTime = (int16)(0.5f * CAI_Core::AI_TICKS_PER_SECOND);
	m_OverridePriority = PRIO_MAX;

	m_StayTimeout = -1;		// Tick when we should no longer stay at the spot but continue the search
	m_pScenePoint = NULL;	// Current active scenepoint
	m_LookHumanTimeout = 0;
	m_iLookHumanTarget = 0;
};


void CAI_Action_Look::SetParameter(int _Param, int _Val)
{
	CAI_Action::SetParameter(_Param, _Val);
}

void CAI_Action_Look::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_TRACKINGTIME:
		m_TrackingTime = (int16)(_Val * AI()->GetAITicksPerSecond()); //Val is time in seconds
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Look::SetParameter(int _Param, CStr _Val)
{
	switch (_Param)
	{
	case PARAM_TRACKINGTIME:
		SetParameter(_Param, (fp32)_Val.Val_fp64());
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get parameter ID from sting (used when parsing registry)
int CAI_Action_Look::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "TRACKINGTIME")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_TRACKINGTIME;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};


//Look
int CAI_Action_Look::MandatoryDevices()
{
	// return (1 << CAI_Device::LOOK);
	return(0);
}

//Valid if there's a look object
bool CAI_Action_Look::IsValid()
{
	//Check base
	if (!CAI_Action::IsValid())
		return false;

	return true;
}

//Always good when valid
CAI_ActionEstimate CAI_Action_Look::GetEstimate()
{
	CAI_ActionEstimate Est;
	if (IsValid())
	{
		//Default score 100
		Est.Set(CAI_ActionEstimate::EXPLORATION, 50);
		Est.Set(CAI_ActionEstimate::VARIETY, 50);
	}
	return Est;
}

bool CAI_Action_Look::FindLookScenepoint(bool _bSearch)
{
	CAI_Core* pAI = AI();

	if (m_pScenePoint)
	{
		if (!m_pAH->RequestScenePoint(m_pScenePoint))
		{
			m_pAH->UpdateLookpointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
		}
		if ((m_pScenePoint)&&(m_StayTimeout != -1)&&(!m_pScenePoint->GetBehaviour())&&(!m_pScenePoint->InFrontArc(pAI->GetLookPos())))
		{
			m_pAH->UpdateLookpointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
		}
	}

	if (!m_pScenePoint)
	{	// We don't have a scenepoint
		// If the time is ripe we should find a new SP
		if ((pAI->m_Timer+3) % AI_KB_REFRESH_PERIOD == 0)	// Add 3 to not coincide with pAI->m_KB refresh
		{
			CVec3Dfp32 OurPos = pAI->GetBasePos();
			CMat4Dfp32 OurMat;
			pAI->GetBaseMat(OurMat);
			if ((_bSearch)&&(pAI->GetType() != CAI_Core::TYPE_DARKLING))
			{
				m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::LOOK | CWO_ScenePoint::DYNAMIC | CWO_ScenePoint::SEARCH,
												CAI_ActionHandler::SP_AVOID_LIGHT,
												32,pAI->m_SightRange,OurMat,OurPos,0.0f,0);
			}
			else
			{
				m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::LOOK | CWO_ScenePoint::DYNAMIC | CWO_ScenePoint::ROAM,
												CAI_ActionHandler::SP_PREFER_LIGHT,
												32,pAI->m_SightRange,OurMat,OurPos,0.0f,0);
			}
		}
	}

	if (m_pScenePoint)
	{
		return(true);
	}
	else
	{
		return(false);
	}
}

bool CAI_Action_Look::FindLookHuman(bool _bSearch)
{
	CAI_Core* pAI = AI();

	CAI_AgentInfo* pAgent = NULL;
	if ((pAI->m_DeviceLook.IsAvailable())&&(m_LookHumanTimeout < pAI->m_Timer)&&((pAI->m_Timer+7) % (AI_KB_REFRESH_PERIOD * 8) == 0))	// Add 7 to not coincide with pAI->m_KB or FindLookScenepoint
	{
		pAgent = pAI->m_KB.GetRandomAgentInRange(256.0f,CAI_AgentInfo::SPOTTED,CAI_AgentInfo::HOSTILE_2);
		if (pAgent)
		{
			m_iLookHumanTarget = pAgent->GetObjectID();
			// OK, now we determine wether we should just steal quick glances or longer looks
			if (pAgent->GetCurRelation() >= CAI_AgentInfo::HOSTILE)
			{	// Short glance
				m_LookHumanTimeout = pAI->m_Timer + TruncToInt(5.0f + 5.0f * Random);
			}
			else
			{
				CAI_Core* pHisAI = pAI->GetAI(m_iLookHumanTarget);
				if ((pHisAI)&&(pAI->IsPlayer(m_iLookHumanTarget)))
				{
					CVec3Dfp32 Diff = pAI->GetHeadPos() - pHisAI->GetHeadPos();
					Diff.Normalize();
					if (Diff * pHisAI->GetLookDir() >= 0.87f)
					{	// He sees us, short glance
						m_LookHumanTimeout = pAI->m_Timer + TruncToInt(2.0f + 5.0f * Random);
					}
					else
					{	// Oogle, 1-2 secs
						m_LookHumanTimeout = pAI->m_Timer + TruncToInt(pAI->GetAITicksPerSecond() * (1.0f + Random));
					}
				}
				else
				{	// Oogle, 1-2 secs
					m_LookHumanTimeout = pAI->m_Timer + TruncToInt(pAI->GetAITicksPerSecond() * (1.0f + Random));
				}
			}
		}
	}

	pAgent = pAI->m_KB.GetAgentInfo(m_iLookHumanTarget);
	if ((pAgent)&&((m_LookHumanTimeout >=  pAI->m_Timer)))
	{
		pAI->OnTrackAgent(pAgent,10,false,true);
		if (!pAI->m_DeviceLook.IsAvailable())
		{	// Good look at target
#ifndef M_RTM
			if ((!pAI->DebugTargetSet())||(pAI->DebugTarget()))
			{
				pAI->Debug_RenderWire(pAI->GetHeadPos(),pAgent->GetSuspectedHeadPosition(),kColorBlue,1.0f);
			}
#endif
			return(true);
		}
		else
		{	// No good look target apparently
#ifndef M_RTM
			if ((!pAI->DebugTargetSet())||(pAI->DebugTarget()))
			{
				pAI->Debug_RenderWire(pAI->GetHeadPos(),pAgent->GetSuspectedHeadPosition(),kColorPurple,1.0f);
			}
#endif
			if (m_iLookHumanTarget)
			{	// Clear the look
				pAI->m_pGameObject->AI_SetEyeLookDir();
			}
			m_LookHumanTimeout = 0;
			m_iLookHumanTarget = 0;
			return(false); 
		}
	}
	else
	{
		if (m_iLookHumanTarget)
		{	// Clear the look
			pAI->m_pGameObject->AI_SetEyeLookDir();
		}
		m_LookHumanTimeout = 0;
		m_iLookHumanTarget = 0;
	}

	return(false);
}

bool CAI_Action_Look::LookAtScenepoint()
{
	CAI_Core* pAI = AI();

	if (!m_pScenePoint)
	{
		return(false);
	}

	if (!m_pAH->RequestScenePoint(m_pScenePoint))
	{
		m_pAH->UpdateLookpointHistory(m_pScenePoint);
		m_pScenePoint = NULL;
		return(false);
	}

	if (m_pScenePoint->InFrontArc(pAI->GetBasePos()))
	{
		pAI->OnTrackPos(m_pScenePoint->GetPosition(), 15, false, true);
		if (m_StayTimeout == -1)
		{
			CVec3Dfp32 OffsetNorm = (m_pScenePoint->GetPosition() - pAI->GetLookPos()).Normalize();
			if (OffsetNorm * pAI->GetLookDir() >= 0.87f)
			{
				int StayTicks = (int)(m_pScenePoint->GetBehaviourDuration() * pAI->GetAITicksPerSecond());
				m_StayTimeout = pAI->m_Timer + StayTicks;
				CWO_ScenePoint* orgSP = m_pScenePoint;
				m_pAH->ActivateScenePoint(m_pScenePoint,GetPriority());
				if (orgSP != m_pScenePoint) { return(false); }
			}
#ifndef M_RTM
			else
			{
				m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
			}
#endif
		}
		else
		{
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
			if (pAI->m_Timer > m_StayTimeout)
			{	// We're done dude
				m_StayTimeout = -1;
				if (m_pScenePoint)
				{
					m_pScenePoint = NULL;
				}
				return(false);
			}
			if ((m_pScenePoint)&&(m_pScenePoint->GetBehaviourDuration() <= 0.0f))
			{
				m_StayTimeout = pAI->m_Timer + pAI->GetAITicksPerSecond();
			}
		}
		return(true);
	}
	else
	{
		m_pScenePoint = NULL;
		return(false);
	}
		
	return(false);
}

void CAI_Action_Look::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		m_pScenePoint = NULL;
	}
}


bool CAI_Action_Look::OnServiceBehaviour()
{
	CAI_Core* pAI = AI();

	if ((pAI->m_bBehaviourEntering)||(pAI->m_bBehaviourExiting))
	{
		if ((m_iLookHumanTarget)||(m_pScenePoint)||(pAI->m_iLookAtObject))
		{	// Clear the look
			pAI->m_pGameObject->AI_SetEyeLookDir();
			m_LookHumanTimeout = 0;
			m_iLookHumanTarget = 0;
		}

		return(false);
	}

	if (pAI->m_iLookAtObject != 0)
	{
		if (!pAI->m_pServer->Object_Get(pAI->m_iLookAtObject))
		{	//No correct look at object, reset it
			pAI->m_iLookAtObject = 0;
			return(false);
		}

		//Track continuously. Use CAI_Core::OnTrack directly, since we want to be able to set soft look flag explicitly
		pAI->OnTrackObj(pAI->m_iLookAtObject,m_TrackingTime,false,pAI->m_bLookSoft);
		return(true);
	}
	else
	{	
		// No LOOK Roam SPs unless IDLE
		if ((m_pAH->GetCurPrioClass() >= CAI_Action::PRIO_ALERT)||(pAI->GetStealthTension() >= CAI_Core::TENSION_HOSTILE))
		{
			// Handle search look SPs
			if (FindLookScenepoint(true))
			{	// We have a valid look
				LookAtScenepoint();
				return(true);
			}
			if (FindLookHuman(true))
			{	// We have a valid look
				return(true);
			}
		}
		else
		{
			// Handle look SPs
			if (FindLookScenepoint(false))
			{	// We have a valid look
				LookAtScenepoint();
				return(true);
			}
			if (FindLookHuman(false))
			{	// We have a valid look
				return(true);
			}
		}
	}

	return(false);
};

//Track look at object continously
void CAI_Action_Look::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Look::OnTakeAction);
	CAI_Core* pAI = AI();
	if ((!pAI)||(!pAI->m_pServer))
	{
		SetExpirationExpired();
		return;
	}
	
	SetExpirationIndefinite();

	// No look when we're close to our Roam destination (32 units?)
	CAI_Action* pAction = NULL;
	if ((m_pAH->GetAction(ROAM,&pAction))&&(pAction->IsTaken())&&(!pAI->m_iCurBehaviour))
	{
		CAI_Action_Roam* pRoamAction = safe_cast<CAI_Action_Roam>(pAction);
		if ((pRoamAction)&&(pRoamAction->m_pScenePoint)&&(pRoamAction->m_pScenePoint->GetCosArcAngle() > -1.0f))
		{
			fp32 SqrDistance = pAI->SqrDistanceToUs(pRoamAction->m_pScenePoint->GetPosition());
			if ((SqrDistance <= Sqr(48.0f))||(SqrDistance <= pRoamAction->m_pScenePoint->GetSqrRadius()))
			{
				pAI->m_pGameObject->AI_SetEyeLookDir();
				ExpireWithDelay(pAI->GetAITicksPerSecond() * 3);
				return;
			}
		}
	}

	if (pAI->m_AH.StopForPlayer())
	{
		return;
	}

	if (pAI->m_iLookAtObject != 0)
	{
		if (!pAI->m_pServer->Object_Get(pAI->m_iLookAtObject))
		{	//No correct look at object, reset it
			pAI->m_iLookAtObject = 0;
			return;
		}

		//Track continuously. Use CAI_Core::OnTrack directly, since we want to be able to set soft look flag explicitly
		pAI->OnTrackObj(pAI->m_iLookAtObject,m_TrackingTime,false,pAI->m_bLookSoft);
		return;
	}
	else
	{
		if ((m_pAH->GetCurPrioClass() >= CAI_Action::PRIO_ALERT)||(pAI->GetStealthTension() >= CAI_Core::TENSION_HOSTILE))
		{
			if ((m_pAH->GetCurPrioClass() < CAI_Action::PRIO_COMBAT)&&(pAI->GetStealthTension() < CAI_Core::TENSION_COMBAT))
			{
				// Handle search look SPs
				if (FindLookScenepoint(true))
				{	// We have a valid look
					LookAtScenepoint();
					return;
				}
				if (FindLookHuman(true))
				{	// We have a valid look
					return;
				}
			}
		}
		else
		{
			// Handle look SPs
			if (FindLookScenepoint(false))
			{	// We have a valid look
				LookAtScenepoint();
				return;
			}
			if (FindLookHuman(false))
			{	// We have a valid look
				return;
			}
		}
	}

}

bool CAI_Action_Look::AnimationEvent(int _iUser, int _iEvent)
{
	CAI_Core* pAI = AI();
	
	if ((pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)&&(MRTC_ISKINDOF(pAI,CAI_Core_Darkling)))
	{
		CAI_Core_Darkling* pDK = safe_cast<CAI_Core_Darkling>(pAI);
		if ((pDK)&&(pDK->m_DK_Special == CAI_Core_Darkling::DK_SPECIAL_LIGHTKILLER)&&(pDK->m_iCurBehaviour)&&(m_pScenePoint))
		{	// Tell Wilbo that we're ready to Zapp or Unzapp as the case may be
			CVec3Dfp32 Zap = m_pScenePoint->GetPosition();
			
			// Send zap message to darkling so he can start effect on clients
			if (_iEvent == 0)
			{
				pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DARKLING_LIGHTNING, 1, 0, -1, 0, Zap), pDK->GetObjectID());
			}
			else if (_iEvent == 1)
			{
				pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DARKLING_LIGHTNING, 0, 0, -1, 0, Zap), pDK->GetObjectID());
			}
			return(true);
		}
	}

	return(false);
};

//Not overridable
bool CAI_Action_Look::IsOverridableByActionOverride()
{
	return false;
};

//CAI_Action_Scan////////////////////////////////////////////////////////////////////////////
CAI_Action_Scan::CAI_Action_Scan(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = SCAN;
	m_TypeFlags = TYPE_SEARCH;
	m_MinAwareness = CAI_AgentInfo::NOTICED;
	m_MaxAwareness = CAI_AgentInfo::DETECTED;
	m_AlertnessThreshold = CAI_KnowledgeBase::ALERTNESS_IDLE;
	m_Pos = CVec3Dfp32(_FP32_MAX);
	m_AlertTick = 0;
	m_bSecThreatOnly = true;
	m_Propability = 1.0f;
	m_Interval = 0;
	m_LastIntervalTick = 0;
	m_MaxRange = 512.0f;
};


//Set params

void CAI_Action_Scan::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_PROPABILITY:
		_Val = Max(0.0f, Min(1.0f, _Val));
		m_Propability = _Val;
		break;

	case PARAM_INTERVAL:
		m_Interval = (int32)(_Val * AI()->GetAITicksPerSecond());
		break;

	case PARAM_RANGE:
		if (_Val > 0)
		{
			m_MaxRange  = _Val;
		}
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
};


void CAI_Action_Scan::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_AWARENESS:
		if ((_Val >= CAI_AgentInfo::NOTICED)&&(_Val < CAI_AgentInfo::SPOTTED))
		{
			m_MinAwareness = _Val;
		}
		break;

	case PARAM_ALERTNESS:
		m_AlertnessThreshold = _Val;
		break;

	case PARAM_SECONLY:
		if (_Val != 0)
		{
			m_bSecThreatOnly = true;
		}
		else
		{
			m_bSecThreatOnly = false;
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Scan::SetParameter(int _Param, CStr _Val)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_RANGE:
		{
			SetParameter(_Param, (fp32)_Val.Val_fp64());
		}
		break;
	case PARAM_AWARENESS:
		{
			int Val = _Val.TranslateInt(CAI_AgentInfo::ms_TranslateAwareness);
			SetParameter(_Param, Val);
		}
		break;
	case PARAM_ALERTNESS:
		{
			int Val = _Val.TranslateInt(CAI_KnowledgeBase::ms_TranslateAlertness);
			SetParameter(_Param, Val);
		}
		break;
	case PARAM_SECONLY:
		{
			SetParameter(_Param, _Val.Val_int());
		}
		break;
	case PARAM_PROPABILITY:
		{
			SetParameter(_Param, (fp32)_Val.Val_fp64());
		}
		break;
	case PARAM_INTERVAL:
		{
			SetParameter(_Param, (fp32)_Val.Val_fp64());
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}


//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_Scan::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "MAXRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_RANGE;
	}
	else if (_Str == "THRESHOLD")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_AWARENESS;
	}
	else if (_Str == "ALERTNESS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_ALERTNESS;
	}
	else if (_Str == "SECONLY")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_SECONLY;
	}
	else if (_Str == "PROPABILITY")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_PROPABILITY;
	}
	else if (_Str == "INTERVAL")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_INTERVAL;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};


//Mandatory: Look; Optional: Move
int CAI_Action_Scan::MandatoryDevices()
{
	return (1 << CAI_Device::LOOK);
};


//Valid when there's a phenomenon to scan for
bool CAI_Action_Scan::IsValid()
{
	if (CAI_Action::IsValid())
	{
		CWObject_Game* pGame = AI()->m_pServer->Game_GetObject();
		int32 playerID = pGame->Player_GetObjectIndex(0);	// There is only one player in PB right?
		CAI_AgentInfo* pAgent = AI()->m_KB.GetAgentInfo(playerID);
		if ((pAgent)&&(pAgent->GetCurAwareness() < CAI_AgentInfo::SPOTTED))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
	return(false);
};


//Scanning is only somewhat exploratory, but relatively safe and lazy
CAI_ActionEstimate CAI_Action_Scan::GetEstimate()
{
	CAI_ActionEstimate Est;
	if (IsValid())
	{
		// We have some extra requirements when the action is taken
		if (AI()->GetAITick() < m_LastIntervalTick + m_Interval)
		{
			return(Est);
		}
		// Scan will only be taken when stationary
		if (AI()->GetPrevSpeed() < 0.1f)
		{
			return(Est);
		}
		CWObject_Game* pGame = AI()->m_pServer->Game_GetObject();
		int32 playerID = pGame->Player_GetObjectIndex(0);	// There is only one player in PB right?
		CAI_AgentInfo* pAgent = AI()->m_KB.GetAgentInfo(playerID);
		// We are alert enough, awareness is above the needed threshold, he is a security threat
		// and we don't see him well enough.
		if ((pAgent)&&
			(AI()->m_KB.GetAlertness() >= m_AlertnessThreshold)&&
			(pAgent->GetCurAwareness() >= m_MinAwareness)&&
			(pAgent->GetCurAwareness() <= m_MaxAwareness)&&
			((!m_bSecThreatOnly)||(m_pAH->IsSecurityThreat(playerID))))
		{
			if ((m_MaxRange > 0)&&(Sqr(m_MaxRange) < pAgent->GetBasePos().DistanceSqr(AI()->GetBasePos())))
			{
				return Est;
			}

			m_LastIntervalTick = AI()->GetAITick();
			if (Random <= m_Propability)
			{
				//Default score 50
				Est.Set(CAI_ActionEstimate::OFFENSIVE, 0); 
				Est.Set(CAI_ActionEstimate::DEFENSIVE, 10);
				Est.Set(CAI_ActionEstimate::EXPLORATION, 15);
				Est.Set(CAI_ActionEstimate::LOYALTY, 0);
				Est.Set(CAI_ActionEstimate::LAZINESS, 25);
				Est.Set(CAI_ActionEstimate::STEALTH, 0);
				Est.Set(CAI_ActionEstimate::VARIETY, 0);
			}
		}
	}
	return Est;
};



//Turn to look at best phenomenon, then scan randomly in that general direction
void CAI_Action_Scan::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Scan::OnTakeAction);

	//Track phenomenon for a while, then set default heading and idle
	CAI_AgentInfo* pAgent = AI()->m_KB.GetBestAwareness();
	if (pAgent)
	{
		if (m_Pos != pAgent->GetHeadPos())
		{	//New phenomenon
			m_Pos = pAgent->GetHeadPos();
			m_AlertTick = (int)(AI()->GetAITick() + 1.5f * AI()->GetAITicksPerSecond());
			// Play some kind of spotting noise?
		}
		OnTrackAgent(pAgent,AI()->m_Reaction * 5);
		SetExpiration(AI()->m_Reaction * 5);
	}
	else
	{
		SetExpirationExpired();
	}
};

//Reset phenomenon position
void CAI_Action_Scan::OnStart()
{
	CAI_Action::OnStart();
	AI();
	m_AlertTick = 0;
};

//Reset phenomenon position
void CAI_Action_Scan::OnQuit()
{
	//Reset stuff
	m_AlertTick = 0;
	CAI_Action::OnQuit();
};


//CAI_Action_EscalateOdd////////////////////////////////////////////////////////////////////////////
const char * CAI_Action_EscalateOdd::ms_lStrFlags[] = 
{
	"CROUCH",
	"JUMP",
	"LEDGE",
	"RUN",
	NULL,
};

CAI_Action_EscalateOdd::CAI_Action_EscalateOdd(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = ESCALATE_ODD;
	m_TypeFlags = TYPE_SEARCH;
	m_Flags = FLAGS_CROUCH | FLAGS_JUMP | FLAGS_LEDGE | FLAGS_MELEE | FLAGS_SHOOT;
	m_Interval = (int32)(10.0f * CAI_Core::AI_TICKS_PER_SECOND);
	
	m_NextCheck = 0;

	m_MaxRangeSqr = -1;
	m_ApproachMinSqr = -1;
	m_ApproachMaxSqr = -1;
};

void CAI_Action_EscalateOdd::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_RANGE:
		m_MaxRangeSqr = Sqr(_Val);
		break;
	case PARAM_APPROACHMIN:
		m_ApproachMinSqr = Sqr(_Val);
		break;
	case PARAM_APPROACHMAX:
		m_ApproachMaxSqr = Sqr(_Val);
		break;

	case PARAM_INTERVAL:
		m_Interval = (int32)(_Val * AI()->GetAITicksPerSecond());
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_EscalateOdd::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_FLAGS:
		m_Flags = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_EscalateOdd::SetParameter(int _Param, CStr _Val)
{
	switch (_Param)
	{
	case PARAM_FLAGS:
		{
			int Val = StrToFlags(_Val);
			SetParameter(_Param,Val);
		}
		break;

	case PARAM_RANGE:
	case PARAM_APPROACHMIN:
	case PARAM_APPROACHMAX:
		SetParameter(_Param,(fp32)_Val.Val_fp64());
		break;

	case PARAM_INTERVAL:
		SetParameter(_Param,(fp32)_Val.Val_fp64());
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}

}

//Get parameter ID from sting (used when parsing registry)
int CAI_Action_EscalateOdd::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "MAXRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_RANGE;
	}
	else if (_Str == "APPROACHMIN")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_APPROACHMIN;
	}
	else if (_Str == "APPROACHMAX")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_APPROACHMAX;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else if (_Str == "INTERVAL")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_INTERVAL;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
}

//Get flags for given string
int CAI_Action_EscalateOdd::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
}


//Mandatory: Look
int CAI_Action_EscalateOdd::MandatoryDevices()
{
	return(0);
}

// Can we see the player
bool CAI_Action_EscalateOdd::IsValid()
{
	AI();

	if (m_pAH->GetCurrentPriorityClass() >= PRIO_COMBAT)
	{
		return(false);
	}
	return(CAI_Action::IsValid());
}

CAI_ActionEstimate CAI_Action_EscalateOdd::GetEstimate()
{
	CAI_ActionEstimate Est;
	CAI_Core* pAI = AI();

	if (IsValid())
	{
		int32 playerID = pAI->GetClosestPlayer();
		CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(playerID);
		if ((pAgent)&&
			((m_MaxRangeSqr < 0)||(pAI->SqrDistanceToUs(pAgent->GetObject())< m_MaxRangeSqr))&&
			(pAgent->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)&&
			(pAgent->GetRelation() >= CAI_AgentInfo::UNFRIENDLY)&&
			(pAgent->GetRelation() < CAI_AgentInfo::ENEMY)&&
			(IsActingOdd(pAgent)))
		{
			//Default score 35
			Est.Set(CAI_ActionEstimate::OFFENSIVE, 10); 
			Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
			Est.Set(CAI_ActionEstimate::EXPLORATION, 5);
			Est.Set(CAI_ActionEstimate::LOYALTY, 0);
			Est.Set(CAI_ActionEstimate::LAZINESS, 10);
			Est.Set(CAI_ActionEstimate::STEALTH, 0);
			Est.Set(CAI_ActionEstimate::VARIETY, 10);
		}
	}

	return(Est);
};

bool CAI_Action_EscalateOdd::IsActingOdd(CAI_AgentInfo* pAgent)
{
	CAI_Core* pAI = AI();

	if (!pAgent)
	{
		return(false);
	}

	int agentID = pAgent->GetObjectID();
	CAI_Core* pAgentAI = pAI->GetAI(agentID);
	if ((!pAgentAI)||(!pAgentAI->m_pGameObject))
	{
		return(false);
	}
	int Awareness = pAgent->GetCurAwareness();
//	int Relation = pAgent->GetCurRelation();
	int MoveMode = pAgentAI->GetMoveMode();
	
	if (Awareness < CAI_AgentInfo::SPOTTED)
	{
		return(false);
	}

	if (pAI->HasOrder(CAI_Core::ORDER_ESCALATE))
	{
		return(true);
	}

	if ((m_Flags & FLAGS_CROUCH)&&(MoveMode == CAI_Core::MOVEMODE_CROUCH))
	{
		return(true);
	}

	if ((m_Flags & FLAGS_RUN)&&(MoveMode == CAI_Core::MOVEMODE_RUN))
	{
		return(true);
	}

	if (m_Flags & FLAGS_JUMP)
	{	// Jumping or falling?
		CVec3Dfp32 PrevSpeed = pAgentAI->GetBasePos() - pAgentAI->GetLastBasePos();
		if (M_Fabs(PrevSpeed[2]) >= 6)
		{	
//			bool bJumping = false;
			// We are moving vertically, determine how high above ground we are
			if (pAI->m_PathFinder.GridPF())
			{
				if (!pAI->m_PathFinder.IsOnGround(pAgentAI->GetBasePos()) && 
					!pAI->m_PathFinder.IsOnGround(pAgentAI->GetLastBasePos()))
				{
					return(true);
				}
			}
			else
			{
				return(true);
			}
		}
	}

	if (m_Flags & FLAGS_LEDGE)
	{
		CWObject_Message Msg(OBJMSG_CHAR_GETCONTROLMODE);
		int ControlMode = pAI->m_pServer->Message_SendToObject(Msg,pAgentAI->m_pGameObject->m_iObject);
		if ((ControlMode == PLAYER_CONTROLMODE_LEDGE2)||
			(ControlMode == PLAYER_CONTROLMODE_CARRYBODY)||
			(ControlMode == PLAYER_CONTROLMODE_LADDER)||
			(ControlMode == PLAYER_CONTROLMODE_HANGRAIL))
		{
			return(true);
		}
	}

	if (m_Flags & FLAGS_MELEE)
	{	// Meleeing within the last 1.5 seconds?
		if (pAgentAI->m_LastMeleeAttackTick > pAI->GetAITick()+30)
		{
			return(true);
		}
	}

	return(false);
};

// Query player according to all flags
void CAI_Action_EscalateOdd::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_EscalateOdd::OnTakeAction);
	CAI_Core* pAI = AI();

	// Safety checks
	if (pAI->HighTension())
	{
		ExpireWithDelay(m_Interval);
		return;
	}

	// OK, observe what the player is doing
	int32 playerID = pAI->GetClosestPlayer();
	CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(playerID);
	if (!pAgent)
	{
		SetExpirationExpired();
		return;
	}

	if (m_pAH->IsRestricted(pAgent,true))
	{
		pAI->OnTrackAgent(pAgent,10,false,false);
		ExpireWithDelay(60);
		return;
	}

	CAI_Core* pAgentAI = pAgent->GetAgentAI();
	if (!pAgentAI)
	{
		SetExpirationExpired();
		return;
	}
	fp32 RangeSqr = pAI->SqrDistanceToUs(playerID);
	int Relation = pAgent->GetCurRelation();
//	int PendingRelation = pAgent->GetCurOrPendingRelation();
	if ((Relation >= CAI_AgentInfo::ENEMY)||(pAgent->GetCurAwareness() <= CAI_AgentInfo::NOTICED))
	{	// We became enemies during action running
		if ((Relation >= CAI_AgentInfo::ENEMY)&&(pAgent->GetCurAwareness() >= CAI_AgentInfo::DETECTED))
		{
			pAI->RaiseAlarm(playerID,CAI_AgentInfo::ENEMY,false);
		}
		SetExpirationExpired();
		return;
	}

	bool bActingOdd = IsActingOdd(pAgent);
	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();

	// Track target
	// Fasser tracking the more aggro we are
	switch(Relation)
	{
	case CAI_AgentInfo::UNFRIENDLY:
		pAI->OnTrackAgent(pAgent,20,false,true);
		break;
	case CAI_AgentInfo::HOSTILE:
		pAI->OnTrackAgent(pAgent,10,false,true);
		break;
	case CAI_AgentInfo::HOSTILE_1:
		case CAI_AgentInfo::HOSTILE_2:
		pAI->OnTrackAgent(pAgent,5,false,false);
		break;	
	}

	if (bActingOdd)
	{
		pAgent->TouchRelation();
	}

	if ((Relation > CAI_AgentInfo::UNFRIENDLY)&&((m_ApproachMinSqr > -1)||(m_ApproachMaxSqr > -1)))
	{	// Perhaps walk toward the guy
		if ((RangeSqr >= m_ApproachMinSqr)&&(RangeSqr <= m_ApproachMaxSqr))
		{	// Move towards suspicious looking
			if (!m_pAH->IsRestricted(pAgent))
			{
				pAI->OnBasicFollowTarget(pAgent,pAI->m_ReleaseIdleSpeed);
			}
		}
	}

	pAI->m_DeviceStance.SetTargetInFOV(true);
	if (pAI->m_Timer > m_NextCheck)
	{
		if (bActingOdd)
		{
			if (Relation == CAI_AgentInfo::UNFRIENDLY)
			{
				SpeakRandom(CAI_Device_Sound::ESCALATE_ODD_HOSTILE0);
				pAgent->SetRelation(Relation+1);
			}
			else if (Relation == CAI_AgentInfo::HOSTILE)
			{
				SpeakRandom(CAI_Device_Sound::ESCALATE_ODD_HOSTILE1);
				pAgent->SetRelation(Relation+1);
				pAI->SetMinStealthTension(CAI_Core::TENSION_HOSTILE);
				if (pAI->m_DeviceStance.GetIdleStance() < CAI_Device_Stance::IDLESTANCE_HOSTILE)
				{
					pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
				}
			}
			else if (Relation < CAI_AgentInfo::HOSTILE_2)
			{
				SpeakRandom(CAI_Device_Sound::ESCALATE_ODD_HOSTILE2);
				pAgent->SetRelation(Relation+1);
				pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
				pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
			}
			else if (Relation >= CAI_AgentInfo::HOSTILE_2)
			{
				pAgent->SetRelation(CAI_AgentInfo::ENEMY);
				SetExpirationExpired();
				pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
				pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
			}
			m_NextCheck = pAI->m_Timer + m_Interval;
		}
	}

	// If we got here without raised relation we should expire
	if ((pAI->m_Timer > m_NextCheck + m_Interval)||(pAgent->GetCurRelation() <= CAI_AgentInfo::UNFRIENDLY))
	{
		SetExpirationExpired();
		return;
	}

	pAI->m_DeviceMove.Use();
}

void CAI_Action_EscalateOdd::OnStart()
{
	CAI_Action::OnStart();
	m_NextCheck = 0;
};

void CAI_Action_EscalateOdd::OnQuit()
{
	CAI_Core* pAI = AI();

	int32 playerID = pAI->GetClosestPlayer();
	CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(playerID);
	if ((pAgent)&&(m_NextCheck))
	{
		int Relation = pAgent->GetCurRelation();
		if (Relation < CAI_AgentInfo::ENEMY)
		{
			SpeakRandom(CAI_Device_Sound::ESCALATE_ODD_STOP);
			if (Relation > CAI_AgentInfo::UNFRIENDLY)
			{
				pAgent->SetRelation(CAI_AgentInfo::UNFRIENDLY, (int)(40 + Random * 20));
			}
		}
	}
	CAI_Action::OnQuit();
};

//CAI_Action_EscalateThreat////////////////////////////////////////////////////////////////////////////
CAI_Action_EscalateThreat::CAI_Action_EscalateThreat(CAI_ActionHandler * _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = ESCALATE_THREAT;
	m_TypeFlags = TYPE_SEARCH;
	m_Interval = (int32)(5.0f * CAI_Core::AI_TICKS_PER_SECOND);

	m_iTarget = 0;
	m_StartEscalate = 0;

	m_MaxRangeSqr = -1;
	m_ApproachMinSqr = -1;
	m_ApproachMaxSqr = -1;
};

void CAI_Action_EscalateThreat::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_RANGE:
		m_MaxRangeSqr = Sqr(_Val);
		break;
	case PARAM_APPROACHMIN:
		m_ApproachMinSqr = Sqr(_Val);
		break;
	case PARAM_APPROACHMAX:
		m_ApproachMaxSqr = Sqr(_Val);
		break;

	case PARAM_INTERVAL:
		m_Interval = (int32)(_Val * AI()->GetAITicksPerSecond());
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_EscalateThreat::SetParameter(int _Param, int _Val)
{
	CAI_Action::SetParameter(_Param, _Val);
}

void CAI_Action_EscalateThreat::SetParameter(int _Param, CStr _Val)
{
	switch (_Param)
	{
	case PARAM_RANGE:
	case PARAM_APPROACHMIN:
	case PARAM_APPROACHMAX:
		SetParameter(_Param,(fp32)_Val.Val_fp64());
		break;

	case PARAM_INTERVAL:
		SetParameter(_Param,(fp32)_Val.Val_fp64());
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get parameter ID from sting (used when parsing registry)
int CAI_Action_EscalateThreat::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "MAXRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_RANGE;
	}
	else if (_Str == "APPROACHMIN")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_APPROACHMIN;
	}
	else if (_Str == "APPROACHMAX")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_APPROACHMAX;
	}
	else if (_Str == "INTERVAL")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_INTERVAL;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
}


//Mandatory: Look
int CAI_Action_EscalateThreat::MandatoryDevices()
{
	return(0);
}

// Can we see the player
bool CAI_Action_EscalateThreat::IsValid()
{
	AI();

	if ((!CAI_Action::IsValid())||(m_pAH->GetCurrentPriorityClass() >= PRIO_COMBAT))
	{
		m_iTarget = 0;
		return(false);
	}

	if (m_iTarget)
	{
		CAI_Core* pAI = AI();
		CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(m_iTarget);
		if (!pAgent)
		{
			m_iTarget = 0;
			return(false);
		}
		CAI_Core* pAgentAI = pAI->GetAI(m_iTarget);
		if (!pAgentAI)
		{
			m_iTarget = 0;
			return(false);
		}
		if ((pAgent->GetCurAwareness() < CAI_AgentInfo::DETECTED)||
			(pAgent->GetRelation() < CAI_AgentInfo::UNFRIENDLY)||
			(pAgent->GetRelation() >= CAI_AgentInfo::ENEMY)||
			(pAgentAI->m_Weapon.GetWieldedArmsClass() < CAI_WeaponHandler::AI_ARMSCLASS_GUN)||
			((m_MaxRangeSqr > 0)&&(pAI->SqrDistanceToUs(m_iTarget) > m_MaxRangeSqr)))
		{
			return(false);
		}
		else
		{
			return(true);
		}
	}

	return(true);
}

CAI_ActionEstimate CAI_Action_EscalateThreat::GetEstimate()
{
	CAI_ActionEstimate Est;
	CAI_Core* pAI = AI();

	if (IsValid())
	{
		m_iTarget = pAI->GetClosestPlayer();
		CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(m_iTarget);
		if ((!pAgent)||(pAgent->GetCurAwareness() < CAI_AgentInfo::SPOTTED))
		{	// If we cannot see him we cannot know if he has his gun out
			SetExpirationExpired();
			return Est;
		}
		if (!IsValid())
		{
			m_iTarget = 0;
			return Est;
		}
		
		//Default score 35
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 10); 
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 5);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, 10);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 10);
	}

	return(Est);
}

// Query player according to all flags
void CAI_Action_EscalateThreat::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_EscalateThreat::OnTakeAction);
	CAI_Core* pAI = AI();

	// Safety checks
 	if (pAI->HighTension())
	{
		ExpireWithDelay(m_Interval);
		return;
	}

	CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (!pAgent)
	{
		SetExpirationExpired();
		return;
	}

	if (m_pAH->IsRestricted(pAgent,true))
	{
		pAI->OnTrackAgent(pAgent,10,false,false);
		ExpireWithDelay(60);
		return;
	}

	CAI_Core* pAgentAI = pAgent->GetAgentAI();
	if (!pAgentAI)
	{
		SetExpirationExpired();
		return;
	}
	fp32 RangeSqr = pAI->SqrDistanceToUs(m_iTarget);
	int Relation = pAgent->GetCurRelation();
	if ((Relation < CAI_AgentInfo::UNFRIENDLY)||
		(Relation >= CAI_AgentInfo::ENEMY)||
		(pAgent->GetCurAwareness() <= CAI_AgentInfo::NOTICED))
	{	// We became enemies during action running
		if ((Relation >= CAI_AgentInfo::ENEMY)&&(pAgent->GetCurAwareness() >= CAI_AgentInfo::DETECTED))
		{
			pAI->RaiseAlarm(m_iTarget,CAI_AgentInfo::ENEMY,false);
			pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		}
		SetExpirationExpired();
		return;
	}

	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();

	// Track target
	// Fasser tracking the more aggro we are
	switch(Relation)
	{
	case CAI_AgentInfo::UNFRIENDLY:
		// Very slow and soft
		pAI->OnTrackAgent(pAgent,10,false,true);
		break;
	case CAI_AgentInfo::HOSTILE:
		// Slow but hard
		pAI->OnTrackAgent(pAgent,5,false,false);
		break;
	case CAI_AgentInfo::HOSTILE_1:
	case CAI_AgentInfo::HOSTILE_2:
		// Fast and hard
		pAI->OnTrackAgent(pAgent,1,false,false);
		break;	
	}

	if (pAgentAI->m_Weapon.GetWieldedArmsClass() < CAI_WeaponHandler::AI_ARMSCLASS_GUN)
	{	// Jackie has removed his f%&#ng gun
		SetExpirationExpired();
		return;
	}

	if ((Relation > CAI_AgentInfo::UNFRIENDLY)&&((m_ApproachMinSqr > -1)||(m_ApproachMaxSqr > -1)))
	{	// Perhaps walk toward the guy
		if ((RangeSqr >= m_ApproachMinSqr)&&(RangeSqr <= m_ApproachMaxSqr))
		{	// Move towards suspicious looking
			if (!m_pAH->IsRestricted(pAgent))
			{
				pAI->OnBasicFollowTarget(pAgent,pAI->m_ReleaseIdleSpeed);
			}
		}
	}

	pAI->m_DeviceMove.Use();

	pAI->m_DeviceStance.SetTargetInFOV(true);
	if ((Relation < CAI_AgentInfo::HOSTILE_2)&&(pAI->IsLookWithinAngle(5.0f,m_iTarget)))
	{
		SpeakRandom(CAI_Device_Sound::ESCALATE_THREAT_HOSTILE2);
		pAgent->SetRelation(CAI_AgentInfo::HOSTILE_2);
		Relation = pAgent->GetCurRelation();
		pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		// Reduce time to ENEMY to one m_Interval
		m_StartEscalate = pAI->m_Timer - 2 * m_Interval;
		// Tell our friends about this asshole
		pAI->RaiseAwareness(m_iTarget,pAgent->GetSuspectedHeadPosition(),CAI_AgentInfo::SPOTTED,CAI_AgentInfo::HOSTILE_2);
	}

	if ((Relation == CAI_AgentInfo::UNFRIENDLY)&&(pAI->m_Timer > m_StartEscalate + 5))
	{
		SpeakRandom(CAI_Device_Sound::ESCALATE_THREAT_HOSTILE0);
		pAgent->SetRelation(Relation+1);
		pAI->SetMinStealthTension(CAI_Core::TENSION_HOSTILE);
		if (pAI->m_DeviceStance.GetIdleStance() < CAI_Device_Stance::IDLESTANCE_HOSTILE)
		{
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
		}
	}
	else if ((Relation == CAI_AgentInfo::HOSTILE)&&(pAI->m_Timer > m_StartEscalate + m_Interval))
	{
		SpeakRandom(CAI_Device_Sound::ESCALATE_THREAT_HOSTILE1);
		pAgent->SetRelation(Relation+1);
		pAI->SetMinStealthTension(CAI_Core::TENSION_COMBAT);
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
	}
	else if ((Relation < CAI_AgentInfo::HOSTILE_2)&&(pAI->m_Timer > m_StartEscalate + 2 * m_Interval))
	{
		SpeakRandom(CAI_Device_Sound::ESCALATE_THREAT_HOSTILE2);
		pAgent->SetRelation(Relation+1);
		pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
	}
	else if ((Relation >= CAI_AgentInfo::HOSTILE_2)&&(pAI->m_Timer > m_StartEscalate + 3 * m_Interval))
	{
		pAgent->SetRelation(CAI_AgentInfo::ENEMY);
		pAI->RaiseAlarm(m_iTarget,CAI_AgentInfo::ENEMY,false);
		pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		SetExpirationExpired();
	}


	if (pAI->GetStealthTension() < CAI_Core::TENSION_HOSTILE)
	{
		pAI->SetStealthTension(CAI_Core::TENSION_HOSTILE);
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
	}
}

void CAI_Action_EscalateThreat::OnStart()
{
	CAI_Core* pAI = AI();
	m_StartEscalate = pAI->m_Timer;
	pAI->m_DeviceSound.PauseType(CAI_Device_Sound::IDLE_TO_WARY,15.0f);
	CAI_Action::OnStart();
};

void CAI_Action_EscalateThreat::OnQuit()
{
	CAI_Core* pAI = AI();
	CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (pAgent)
	{
		int Relation = pAgent->GetCurRelation();
		if (Relation < CAI_AgentInfo::ENEMY)
		{
			SpeakRandom(CAI_Device_Sound::ESCALATE_THREAT_STOP);
			if (Relation > CAI_AgentInfo::UNFRIENDLY)
			{
				pAI->SetStealthTension(CAI_Core::TENSION_HOSTILE);
				pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
				pAgent->SetRelation(CAI_AgentInfo::UNFRIENDLY, (int)(40 + Random * 20));
			}
		}
	}
	m_iTarget = 0;
	m_StartEscalate = 0;
	CAI_Action::OnQuit();
};

//CAI_Action_Investigate////////////////////////////////////////////////////////////////////////////
const char * CAI_Action_Investigate::ms_lStrFlags[] = 
{
	"NOTICEPTS",
	"SCENEPTS",
	"DEATHSPOTS",
	"HOLDRANGE",
	"RUN",
		NULL,
};

CAI_Action_Investigate::CAI_Action_Investigate(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = INVESTIGATE;
	m_TypeFlags = TYPE_SEARCH | TYPE_MOVE | TYPE_DEATH;
	// We do NOT allow FLAGS_DEATHPTS as default
	m_Flags = FLAGS_NOTICEPOINTS | FLAGS_SCENEPOINTS;
	m_iBehaviourStand = 0;
	m_iBehaviourCrouch = 0;
	m_bLeadSearch = false;
	m_StartStayMat.Unit();
	m_iSuspect = 0;
	INVALIDATE_POS(m_SuspectPos);
	INVALIDATE_POS(m_Destination);
	m_pScenePoint = NULL;
	m_pLookPoint = NULL;
	m_StayTimeout = -1;

	m_bWalkStop = false;
	
};

void CAI_Action_Investigate::SetParameter(int _Param, fp32 _Val)
{
	CAI_Action::SetParameter(_Param, _Val);
}

void CAI_Action_Investigate::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_FLAGS:
		m_Flags = _Val;
		break;

	case PARAM_BEHAVIOUR_STAND:
		m_iBehaviourStand = _Val;
		break;

	case PARAM_BEHAVIOUR_CROUCH:
		m_iBehaviourCrouch = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Investigate::SetParameter(int _Param, CStr _Val)
{
	if (!AI())
		return;

	int Val;
	switch (_Param)
	{
	case PARAM_FLAGS:
		{
			Val = _Val.TranslateFlags(CAI_Action_Investigate::ms_lStrFlags);
			if (Val != -1)
			{
				SetParameter(_Param,Val);
			}
		}
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

int CAI_Action_Investigate::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_FLAGS;
	}
	else if (_Str == "BEHAVIOURSTAND")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_BEHAVIOUR_STAND;
	}
	else if (_Str == "BEHAVIOURCROUCH")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_BEHAVIOUR_CROUCH;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
}

int CAI_Action_Investigate::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

//Mandatory: Move, Look; Optional: Stance, Jump
int CAI_Action_Investigate::MandatoryDevices()
{
	// return (1 << CAI_Device::MOVE) | (1 << CAI_Device::LOOK);
	return (1 << CAI_Device::MOVE);
};


//Valid when there's a phenomenon to investigate for
bool CAI_Action_Investigate::IsValid()
{
	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		return(false);
	}

	if (pAI->GetStealthTension() >= CAI_Core::TENSION_COMBAT)
	{
		return(false);
	}

	if (m_iSuspect)
	{
		CAI_Core* pTargetAI = pAI->GetAI(m_iSuspect);
		if (pTargetAI)
		{	// We make sure we have an agent in that case
			CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iSuspect);
			if (!pTarget)
			{	// We really should add the agent no matter what the relation. m_iSuspect might have been set by script and we
				// should search for him even if he is our friend. GetEstimate will check relations when there's n0 m_iSuspect
				pTarget = pAI->m_KB.AddAgent(m_iSuspect);
			}
			if (!pTarget)
			{	// Agent couldn't be added
				m_iSuspect = 0;
				return(false);
			}
			if (pTarget->GetCurAwareness() >= CAI_AgentInfo::DETECTED)
			{	// We know where the little sucker is
				m_iSuspect = 0;
				return(false);
			}

			// pTarget != NULL
			m_SuspectPos = pTarget->GetSuspectedPosition();
			return(true);
		}
		else
		{	// No AI, m_iSuspect is an object
			// Do periodic tracelines against m_iSuspect
			CWObject* pObj = pAI->m_pServer->Object_Get(m_iSuspect);
			if (!pObj)
			{	// No such object
				m_iSuspect = 0;
				return(false);
			}
			if (INVALID_POS(m_SuspectPos))
			{
				m_SuspectPos = pObj->GetPosition();
			}
			
			// Check if we can see m_iSuspect
			if (pAI->m_Timer % AI_KB_REFRESH_PERIOD == 0)
			{
				CVec3Dfp32 headPos = pAI->GetHeadPos(m_iSuspect);
				fp32 Vis = pAI->MeasureLightIntensityAt(headPos,true,m_iSuspect);
				Vis = Max(Vis,PERCEPTIONFACTOR_MIN_LIGHT);
				// Scale Vis with object size if object is really small
				const CBox3Dfp32* pBox = pObj->GetAbsBoundBox();
				if (pBox)
				{
					CVec3Dfp32 Diff = pBox->m_Max - pBox->m_Min;
					fp32 MaxDim = (Max3(Diff[0],Diff[1],Diff[2])) / 32.0f;
					MaxDim = Max(MaxDim,0.25f);	// No matter how small, no object is considered smaller than 1/4
					if (MaxDim < 1.0f)
					{
						Vis *= MaxDim;
					}
				}
				int Awareness = pAI->Perception_Sight(m_iSuspect,true,Vis);
				if (Awareness >= CAI_AgentInfo::SPOTTED)
				{	// Found the object!
					m_bLeadSearch = false;
					SpeakRandom(CAI_Device_Sound::SEARCH_STOP);
					pAI->RaiseSearchOff(m_iSuspect);
					m_iSuspect = 0;
					return(false);
				}
			}
			
			return(true);
		}
	}

	return(true);
};

//Investigating is of course mainly exploratory, but also useful for aggressive agents.
//It might be dangerous though.
CAI_ActionEstimate CAI_Action_Investigate::GetEstimate()
{
	CAI_ActionEstimate Est;
	CAI_Core* pAI = AI();

	if (IsValid())
	{
		// We have some extra requirements when the action is taken

		if (!m_iSuspect)
		{	// Nothing to search for:
			// Check if we're enemy with the player and if we are if our knowledge is NOTICED
			m_iSuspect = pAI->GetClosestPlayer();
			CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(m_iSuspect);
			if (!pAgent)
			{
				pAgent = pAI->m_KB.AddAgent(m_iSuspect);
			}
			// If we don't know about the player or if we don't hate him or if we see him
			if ((!pAgent)||(pAgent->GetCurRelation() < CAI_AgentInfo::ENEMY)||(pAgent->GetCurAwareness() != CAI_AgentInfo::NOTICED))
			{
				m_iSuspect = 0;
			}
			else
			{
				m_SuspectPos = pAgent->GetSuspectedPosition();
				pAgent->Touch();
			}

			if ((m_iSuspect)&&(VALID_POS(m_SuspectPos)))
			{
				if ((pAI->SqrDistanceToUs(m_SuspectPos) < Sqr(pAI->m_SearchMinRange))||(pAI->SqrDistanceToUs(m_SuspectPos) > Sqr(pAI->m_SearchMaxRange)))
				{
					m_iSuspect = 0;
				}
			}

			if ((m_iSuspect)&&(VALID_POS(m_SuspectPos))&&(VALID_POS(pAI->m_HoldPosition))&&(m_Flags & FLAGS_RANGEFROMHOLD))
			{
				fp32 SqrDistanceToHold = m_SuspectPos.DistanceSqr(pAI->m_HoldPosition);
				if ((SqrDistanceToHold < Sqr(pAI->m_SearchMinRange))||(SqrDistanceToHold > Sqr(pAI->m_SearchMaxRange)))
				{
					m_iSuspect = 0;
				}
			}
		}


		if (!m_iSuspect)
		{	// Do we have an NPC noticed enemy to search for?
			if (m_pAH->m_iInvestigate)
			{
				m_iSuspect = m_pAH->m_iInvestigate;
			}
			else if (m_pAH->m_iInvestigate2)
			{
				m_iSuspect = m_pAH->m_iInvestigate;
			}
			else if (m_pAH->m_iInvestigateObj)
			{
				m_iSuspect = m_pAH->m_iInvestigateObj;
			}

			CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(m_iSuspect);
			if ((!pAgent)&&(m_iSuspect))
			{
				pAgent = pAI->m_KB.AddAgent(m_iSuspect);
			}
			// If we don't know about the player or if we don't hate him or if we see him
			if ((!pAgent)||(pAgent->GetCurRelation() < CAI_AgentInfo::ENEMY)||(pAgent->GetCurAwareness() != CAI_AgentInfo::NOTICED))
			{
				m_iSuspect = 0;
			}
			else
			{
				m_SuspectPos = pAgent->GetSuspectedPosition();
			}

			if ((m_iSuspect)&&(VALID_POS(m_SuspectPos)))
			{
				if ((pAI->SqrDistanceToUs(m_SuspectPos) < Sqr(pAI->m_SearchMinRange))||(pAI->SqrDistanceToUs(m_SuspectPos) > Sqr(pAI->m_SearchMaxRange)))
				{
					m_iSuspect = 0;
				}
			}

			if ((m_iSuspect)&&(VALID_POS(m_SuspectPos))&&(VALID_POS(pAI->m_HoldPosition))&&(m_Flags & FLAGS_RANGEFROMHOLD))
			{
				fp32 SqrDistanceToHold = m_SuspectPos.DistanceSqr(pAI->m_HoldPosition);
				if ((SqrDistanceToHold < Sqr(pAI->m_SearchMinRange))||(SqrDistanceToHold > Sqr(pAI->m_SearchMaxRange)))
				{
					m_iSuspect = 0;
				}
			}
		}

		if ((!m_iSuspect)&&(m_Flags & FLAGS_DEATHSPOTS))
		{
			SDead Dead;
			if (pAI->m_KB.GetFirstMatchingDead(&Dead,CAI_AgentInfo::FRIENDLY,100,800,true))
			{	// Dead must be at least 5 seconds cold.
				if ((pAI->IsTeammember(Dead.m_iVictim))&&(pAI->m_Timer >= Dead.m_TimeOfDeath + 100))
				{
					m_iSuspect = Dead.m_iVictim;
					m_SuspectPos = Dead.m_DeathPos;
				}
			}
		}

		if ((m_iSuspect)&&(VALID_POS(m_SuspectPos)))
		{	
			//Default score 75
			Est.Set(CAI_ActionEstimate::OFFENSIVE, 10); 
			Est.Set(CAI_ActionEstimate::DEFENSIVE, -10);
			Est.Set(CAI_ActionEstimate::EXPLORATION, 75);
			Est.Set(CAI_ActionEstimate::LOYALTY, 0);
			Est.Set(CAI_ActionEstimate::LAZINESS, -20);
			Est.Set(CAI_ActionEstimate::STEALTH, 0);
			Est.Set(CAI_ActionEstimate::VARIETY, 20);
		}
	}

	return Est;
};

int CAI_Action_Investigate::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int nAdded = 0;

	if (m_iBehaviourStand)
	{
		nAdded+= AddUnique(_liBehaviours,m_iBehaviourStand);
	}
	if (m_iBehaviourCrouch)
	{
		nAdded+= AddUnique(_liBehaviours,m_iBehaviourCrouch);
	}

	return(nAdded);
};

void CAI_Action_Investigate::SetInvestigateObject(int32 _iObj)
{
	m_iSuspect = _iObj;
};


// We try to find a scenepoint if allowed
// If that failes we go for the m_SuspectPos
// If we already are near m_SuspectPos we fail and expire
bool CAI_Action_Investigate::FindDestination()
{
	MSCOPESHORT(CAI_Action_Investigate::FindDestination);
	CAI_Core* pAI = AI();

	if (VALID_POS(m_Destination))
	{
		return(true);
	}

	if (m_Flags & FLAGS_SCENEPOINTS)
	{
		// Find scenepoint
		CMat4Dfp32 OurMat;
		pAI->GetBaseMat(OurMat);

		// *** Does SP_AVOID_LIGHT really work for non-darklings? ***
		int Flags = CAI_ActionHandler::SP_PREFER_NEAR_TARGETPOS |
			CAI_ActionHandler::SP_RECENT_FORBIDDEN | 
			CAI_ActionHandler::SP_PREFER_HEADING |
			CAI_ActionHandler::SP_AVOID_LIGHT;

		m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::SEARCH,Flags,pAI->m_SearchMinRange,pAI->m_SearchMaxRange,OurMat,m_SuspectPos,32.0f,0);
		INVALIDATE_POS(m_Destination);
		m_bWalkStop = false;

		if (!m_pScenePoint)
		{	// Use combat scenepoints as positions to search
			Flags |= CAI_ActionHandler::SP_RECENT_FORBIDDEN | CAI_ActionHandler::SP_PREFER_INSIDE_ARC;
			m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::TACTICAL,Flags,pAI->m_SearchMinRange,pAI->m_SearchMaxRange,OurMat,m_SuspectPos,32.0f,0);
			if (m_pScenePoint)
			{
				m_Destination = pAI->m_PathFinder.GetPathPosition(m_pScenePoint->GetPosition(false),4,2);
				// Forward history as these SPs are never activated
				m_pAH->UpdateScenepointHistory(m_pScenePoint);
				m_pScenePoint = NULL;
			}
		}
	}
	
	if (m_pScenePoint)
	{
		m_Destination = pAI->m_PathFinder.GetPathPosition(m_pScenePoint->GetPosition(),4,2);
		if ((!m_pAH->RequestScenePoint(m_pScenePoint))||(INVALID_POS(m_Destination))||(pAI->SqrDistanceToUs(m_Destination) < Sqr(32.0f)))
		{
			INVALIDATE_POS(m_Destination);
			m_pAH->UpdateScenepointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
			return(false);
		}
	}

	if (INVALID_POS(m_Destination))
	{
		m_pScenePoint = NULL;
		if (m_Flags & FLAGS_NOTICEPOINTS)
		{
			m_Destination = pAI->m_PathFinder.GetPathPosition(m_SuspectPos,4,4);
			// Give up, cannot go there
			if ((INVALID_POS(m_Destination))||(pAI->SqrDistanceToUs(m_Destination) < Sqr(32.0f)))
			{
				INVALIDATE_POS(m_Destination);
				INVALIDATE_POS(m_SuspectPos);
				m_pScenePoint = NULL;
				ExpireWithDelay(80);
				return(false);
			}
			return(true);
		}
	}

	// Stop roaming if restricted
	if ((VALID_POS(m_Destination))&&(m_pAH->IsRestricted(m_Destination)))
	{
		INVALIDATE_POS(m_Destination);
		m_pScenePoint = NULL;
		return(false);
	}

	if ((m_pScenePoint)&&(!m_pScenePoint->IsWaypoint()))
	{
		if (m_pScenePoint->GetSqrRadius() > Sqr(16.0f))
		{
			m_bWalkStop = true;
		}
	}

	return(true);
};

bool CAI_Action_Investigate::MoveToDestination()
{
	MSCOPESHORT(CAI_Action_Investigate::MoveToDestination);
	CAI_Core* pAI = AI();

	// PESUDO: Move to m_Destination using the appropriate movement speed
	if (INVALID_POS(m_Destination))
	{
		m_StayTimeout = -1;
		if (m_pScenePoint)
		{
			m_pAH->UpdateScenepointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
		}
		return(false);
	}

	// Some basice useful values
	CVec3Dfp32 OurPos = pAI->GetBasePos();
	fp32 RangeSqr = m_Destination.DistanceSqr(OurPos);
	CVec3Dfp32 OurDir = pAI->GetLookDir();


	// Stop for player unless ENEMY
	// If we're close enough to the PLAYER and he is to the front we pause
	CAI_AgentInfo* pPlayerAgent = m_pAH->StopForPlayer();
	if (pPlayerAgent)
	{
		pAI->ResetStuckInfo();
		pAI->m_DeviceMove.Use();
		return(false);
	}

	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint)))
	{	// Badness, we can no longer use that scenepoint
		INVALIDATE_POS(m_Destination);
		return(false);
	}

	// Stop roaming if restricted
	if ((VALID_POS(m_Destination))&&(m_pAH->IsRestricted(m_Destination)))
	{	
		INVALIDATE_POS(m_Destination);
		return(false);
	}

	if (pAI->m_pGameObject->AI_IsJumping())
	{
		return(false);
	}

	if (m_pScenePoint)
	{
		if ((!m_pScenePoint->IsWaypoint())&&(m_pScenePoint->GetSqrRadius() >= Sqr(16.0f))&&(m_pScenePoint->GetCosArcAngle() <= 0.707f))
		{
			pAI->CheckWalkStop(m_Destination);
		}
		if (m_pScenePoint->IsAt(OurPos))
		{	// Align and shit
			// We don't check look during our stay
			if ((m_StayTimeout != -1)||(m_pScenePoint->IsAligned(pAI->GetLookDir())))
			{
				return(true);
			}
			pAI->OnTrackDir(m_pScenePoint->GetDirection(),0,10,false,false);
			pAI->m_DeviceMove.Use();
			return(false);
		}
		else
		{
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif
			if (m_pAH->HandleSitFlag(m_pScenePoint))
			{	// Were there
				return(true);
			}
			else
			{
				fp32 moveSpeed = pAI->m_ReleaseIdleSpeed;
				if (m_Flags & FLAGS_RUN)
				{
					moveSpeed = pAI->m_ReleaseMaxSpeed;

				}
				int32 moveResult = pAI->OnMove(m_Destination,moveSpeed,false,true,m_pScenePoint);
				if (!pAI->CheckMoveresult(moveResult,m_pScenePoint))
				{	// Couldn't move somehow
					m_pAH->UpdateLookpointHistory(m_pScenePoint);
					m_pScenePoint = NULL;
					INVALIDATE_POS(m_Destination);
					return(false);
				}
				pAI->m_DeviceMove.Use();
				return(false);
			}
		}
		return(true);
	}

	if ((VALID_POS(m_Destination))&&(RangeSqr > Sqr(32.0f)))
	{
		fp32 moveSpeed = pAI->m_ReleaseIdleSpeed;
		pAI->SetDestination(m_Destination);
		if (m_Flags & FLAGS_RUN)
		{	
			moveSpeed = pAI->m_ReleaseMaxSpeed;
		}
		int32 moveResult = pAI->OnMove(m_Destination,moveSpeed,false,false,NULL);
		if (!pAI->CheckMoveresult(moveResult,NULL))
		{	// Couldn't move somehow
			INVALIDATE_POS(m_Destination);
			return(false);
		}
		pAI->m_DeviceMove.Use();
		return(false);
	}

	return(true);
};

bool CAI_Action_Investigate::StayAtDestination()
{
	CAI_Core* pAI = AI();

	if (INVALID_POS(m_Destination))
	{
		m_StayTimeout = -1;
		return(false);
	}

	if (m_pScenePoint)
	{
		// We're there dude, look sharp mon
		pAI->m_DeviceMove.Use();
		if (m_StayTimeout == -1)
		{
			int StayTicks = (int)(m_pScenePoint->GetBehaviourDuration() * pAI->GetAITicksPerSecond());
			m_StayTimeout = pAI->m_Timer + StayTicks;
			CWO_ScenePoint* orgSP = m_pScenePoint;
			m_pAH->ActivateScenePoint(m_pScenePoint,GetPriority());
			if (orgSP != m_pScenePoint) { return(false); }
			SetMinExpirationDuration(StayTicks);
			if ((!m_pScenePoint->GetBehaviour())&&(!pAI->m_bBehaviourRunning)&&(StayTicks))
			{
				int Flags = CAI_Core::BEHAVIOUR_FLAGS_LOOP;
				CVec3Dfp32 Dir = m_pScenePoint->GetDirection();
				if (m_pScenePoint->GetCrouchFlag())
				{
					if (m_iBehaviourCrouch)
					{
						// Crouch: General_left = search fwd, general right = search down
						if (Dir[2] <= -0.5f)	// 30 degrees or more down
						{
							Flags |= CAI_Core::BEHAVIOUR_FLAGS_RIGHT;
						}
						pAI->SetWantedBehaviour2(m_iBehaviourCrouch,GetPriority(),Flags,StayTicks,NULL,NULL);
					}
				}
				else
				{
					if (m_iBehaviourStand)
					{
						// Standing: General_Left = search fwd, general_right = search up				
						if (Dir[2] >= 0.5f)	// 30 degrees or more up
						{
							Flags |= CAI_Core::BEHAVIOUR_FLAGS_RIGHT;
						}
						pAI->SetWantedBehaviour2(m_iBehaviourStand,GetPriority(),Flags,StayTicks,NULL,NULL);
					}
				}
			}
		}
		else if (pAI->m_Timer > m_StayTimeout)
		{	// We're done dude
			SpeakRandom(CAI_Device_Sound::SEARCH_CONTINUE);
			INVALIDATE_POS(m_Destination);
			m_StayTimeout = -1;
			if (m_pScenePoint)
			{
				m_pScenePoint = NULL;
			}
			return(false);
		}
		if ((m_pScenePoint)&&(m_pScenePoint->GetBehaviour())&&(m_pScenePoint->GetBehaviourDuration() == 0.0f))
		{
			m_StayTimeout = pAI->m_Timer + pAI->GetAITicksPerSecond();
		}
		// *** TODO: Replace with appropriate search behaviour when the behaviour arrives
		pAI->OnTrackDir(m_pScenePoint->GetSweepDir(pAI->m_Timer,40),0,10,false,false);
		return(true);
	}
	else
	{	// No scenepoint, just stay some time
		pAI->m_DeviceMove.Use();
		if (m_StayTimeout == -1)
		{
			pAI->GetBaseMat(m_StartStayMat);
			int StayTicks = (int)((0.5f + Random) * 2.0f * pAI->GetAITicksPerSecond());
			m_StayTimeout = pAI->m_Timer + StayTicks;
			SetMinExpirationDuration(StayTicks);
			if ((m_iBehaviourStand)&&(StayTicks)&&(!pAI->m_bBehaviourRunning)&&(Random > 0.5f))
			{
				int Flags = CAI_Core::BEHAVIOUR_FLAGS_LOOP;
				pAI->SetWantedBehaviour2(m_iBehaviourStand,GetPriority(),Flags,StayTicks,NULL,NULL);
			}
		}
		else if (pAI->m_Timer > m_StayTimeout)
		{	// We're done dude
			SpeakRandom(CAI_Device_Sound::SEARCH_CONTINUE);
			INVALIDATE_POS(m_Destination);
			m_StayTimeout = -1;
			m_pScenePoint = NULL;
			return(false);
		}
		if (!pAI->m_bBehaviourRunning)
		{
			// Sweep round each round taking 4 secs
			CVec3Dfp32 LookDir = pAI->GetLookDir();
			int Period = (int)(3 * pAI->GetAITicksPerSecond());
			int32 timerMod = pAI->m_Timer % Period;
			int32 timerDiv = pAI->m_Timer / Period;
			fp32 Angle = fp32(timerMod) / fp32(Period);
			CMat4Dfp32 Mat;
			Mat = m_StartStayMat;
			// Fractional arc angle
			if (pAI->GetObjectID() % 1 == 0)
			{	// Even numbered CCW
				Mat.RotZ_x_M(Angle);
				pAI->OnTrackDir(Mat.GetRow(0),0,10,false,false);
			}
			else
			{	// Odd numbered CW
				Mat.RotZ_x_M(-Angle);
				pAI->OnTrackDir(Mat.GetRow(0),0,10,false,false);
			}
		}
		return(true);
	}

	m_StayTimeout = -1;
	return(false);
};

bool CAI_Action_Investigate::HandleLookScenepoints()
{
	CAI_Core* pAI = AI();

	// We don't have a look scenepoint, how do we do this?
	// We handle it through the fabulous CAI_Action_Look action, telling it to
	// If the time is ripe we should find a new SP
	CAI_Action* pAction;
	if (pAI->m_AH.GetAction(LOOK,&pAction))
	{
		CAI_Action_Look* pLookAction = safe_cast<CAI_Action_Look>(pAction);
		if (pLookAction->FindLookScenepoint(true))
		{
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(pLookAction->m_pScenePoint,true,0,5.0f);
#endif
		}
	}

	return(true);
}

void CAI_Action_Investigate::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		INVALIDATE_POS(m_Destination);
		m_StayTimeout = -1;
		m_pScenePoint = NULL;
	}
}

void CAI_Action_Investigate::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Investigate::OnTakeAction);
	CAI_Core* pAI = AI();

	// Restricted?
	if ((VALID_POS(m_Destination))&&(m_pAH->IsRestricted(m_Destination)))
	{
		ExpireWithDelay((int)(pAI->GetAITicksPerSecond() * 5.0f));
		return;
	}

	if ((!m_iSuspect)||(INVALID_POS(m_SuspectPos)))
	{
		ExpireWithDelay((int)(pAI->GetAITicksPerSecond() * 5.0f));
		return;
	}

	pAI->ActivateAnimgraph();
	SetExpiration(int(pAI->m_Patience * 10.0f));	// 50 seconds

	// Set enough tension and stance
	pAI->SetStealthTension(CAI_Core::TENSION_HOSTILE);
	pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);

	// Keep our knowledge of any suspect fresh
	CAI_AgentInfo* pSuspect = pAI->m_KB.GetAgentInfo(m_iSuspect);
	if (pSuspect)
	{
		pSuspect->Touch();
		pSuspect->TouchRelation();
		if ((ExpiresNextTick())&&(Random < 0.5f))
		{
			m_bLeadSearch = true;
			pSuspect->SetAwareness(CAI_AgentInfo::NOTICED,false,true);
			pSuspect->SetCorrectSuspectedPosition();
			m_Destination = pAI->m_PathFinder.SafeGetPathPosition(pSuspect->GetBasePos(),10,8);
			SetExpirationExpired();
			SetExpiration(int(pAI->m_Patience * 10.0f));
		}
	}

	if (FindDestination())
	{	// We have a valid Destination
		if (MoveToDestination())
		{
			if (StayAtDestination())
			{
#ifndef M_RTM
				m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
			}
			else
			{
#ifndef M_RTM
				m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif
			}
		}
		else
		{
			HandleLookScenepoints();
		}
	}
	else
	{	// No destination
		SetExpirationExpired();
	}
};

void CAI_Action_Investigate::OnStart()
{
	CAI_Action::OnStart();
	CAI_Core* pAI = AI();

	INVALIDATE_POS(m_Destination);
	m_bWalkStop = false;
	m_LookTimeout = -1;
	m_pLookPoint = NULL;
	m_pScenePoint = NULL;
	m_StayTimeout = -1;
	// If there are no other running CAI_Action_Investigate we are the lead searcher
	int iUser = m_pAH->GetClosestActionUser(INVESTIGATE,pAI->m_SearchMaxRange,true);
	if (iUser)
	{
		m_bLeadSearch = false;
	}
	else
	{	// Ahem, as the lead searcher I must tell the others to search too
		m_bLeadSearch = true;
		SpeakRandom(CAI_Device_Sound::SEARCH_START);
		CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(m_iSuspect);
		if ((pAgent)&&(pAgent->GetCurRelation() >= CAI_AgentInfo::ENEMY)&&(pAgent->GetCurAwareness() == CAI_AgentInfo::NOTICED))
		{
			pAI->RaiseAwareness(m_iSuspect,pAgent->GetSuspectedPosition(),CAI_AgentInfo::NOTICED,CAI_AgentInfo::ENEMY);
			m_Destination = pAI->m_PathFinder.GetPathPosition(pAgent->GetSuspectedPosition(),4,4);
		}
		else
		{
			m_Destination = m_SuspectPos;
		}
	}
	m_StartStayMat.Unit();
};

//Reset roam stuff
void CAI_Action_Investigate::OnQuit()
{
	CAI_Core* pAI = AI();

	INVALIDATE_POS(m_Destination);
	m_LookTimeout = -1;
	m_pLookPoint = NULL;
	m_StartStayMat.Unit();
	//Reset stuff
	if (pAI->m_DeviceStance.GetIdleStance() <= CAI_Device_Stance::IDLESTANCE_HOSTILE)
	{	// Set general property for variants on going Idle
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_IDLE,TruncToInt(Random * 6.999f));
	}
	if ((pAI->m_iCurBehaviour == m_iBehaviourStand)||(pAI->m_iCurBehaviour == m_iBehaviourCrouch))
	{
		pAI->StopBehaviour(CAI_Core::BEHAVIOUR_STOP_NORMAL,GetPriority());
	}
	if (pAI->GetStealthTension() < CAI_Core::TENSION_COMBAT)
	{
		if (m_bLeadSearch)
		{
			SpeakRandom(CAI_Device_Sound::SEARCH_STOP);
			pAI->RaiseSearchOff(m_iSuspect);
		}
		else
		{
			pAI->UseRandomDelayed("Nonlead finished searching", CAI_Device_Sound::SEARCH_STOP_RESPONSE, GetPriority(), (int)(pAI->GetAITicksPerSecond() * 1.0f));
		}

		// If our knowledge of the suspect is hearing only we should forget about him completely
		// ie if we searched for a suspicious sound and then gave up we should write the sucker off as uninteresting
		if (m_iSuspect)
		{
			CAI_AgentInfo* pAgent = pAI->m_KB.GetAgentInfo(m_iSuspect);
			if ((pAgent)&&(pAgent->GetCurAwareness() == CAI_AgentInfo::NOTICED))
			{
				pAgent->SetAwareness(CAI_AgentInfo::NONE,false,true);
			}
		}
	}

	m_iSuspect = 0;
	INVALIDATE_POS(m_SuspectPos);
	m_bLeadSearch = false;

	CAI_Action::OnQuit();
};

//CAI_Action_Watch////////////////////////////////////////////////////////////////////////////
CAI_Action_Watch::CAI_Action_Watch(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = WATCH;
	m_TypeFlags = TYPE_SEARCH;
	m_MaxRange = _FP32_MAX;
	m_iTarget = 0;
};

void CAI_Action_Watch::SetParameter(int _Param, int _Val)
{
	CAI_Action::SetParameter(_Param, _Val);
}

//Set params
void CAI_Action_Watch::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_RANGE:
		m_MaxRange = _Val;
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Watch::SetParameter(int _Param, CStr _Val)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_RANGE:
		{
			SetParameter(_Param, (fp32)_Val.Val_fp64());
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}


//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_Watch::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "RANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_RANGE;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};


//Mandatory: Look; Optional: -
int CAI_Action_Watch::MandatoryDevices()
{
	return (1 << CAI_Device::LOOK);
};

int CAI_Action_Watch::GetPriority(bool _bTest)
{
	if (_bTest)
	{
		return(PRIO_IDLE);
	}
	else
	{
		return(m_Priority);
	}
};

//Valid when there's a hostile within sight
bool CAI_Action_Watch::IsValid()
{
	if (!CAI_Action::IsValid())
	{
		return false;
	}

	if (AI()->m_bBehaviourRunning)
	{
		return false;
	}
	return true;
};


int CAI_Action_Watch::FindTargetToWatch()
{
	CAI_Core* pAI = AI();

	int iTarget = 0;

	CAI_AgentInfo* pHostile = m_pAH->FindRelation(false,CAI_AgentInfo::HOSTILE,CAI_AgentInfo::HOSTILE_2);
	if ((!pHostile)||(pHostile->GetCurAwareness() < CAI_AgentInfo::DETECTED))
	{
		pHostile = NULL;
	}

	// Watch the player if we're aware of him and he is close
	/*
	if (!pHostile)
	{
		int iPlayer = pAI->GetClosestPlayer();
		pHostile = pAI->m_KB.GetAgentInfo(iPlayer);
		if (!pHostile)
		{
			pHostile = pAI->m_KB.AddAgent(iPlayer);
		}
		if (pHostile)
		{
			if ((pHostile->GetCurRelation() >= CAI_AgentInfo::ENEMY)||
				(pHostile->GetCurRelation() < CAI_AgentInfo::UNFRIENDLY)||
				(pHostile->GetCurAwareness() < CAI_AgentInfo::DETECTED)||
				(pAI->SqrDistanceToUs(pHostile->GetBasePos()) > Sqr(100.0f)))
			{
				pHostile = NULL;
			}
		}	
	}
	*/

	if ((!pHostile)&&(pAI->m_iCrowdFocus))
	{
		pHostile = pAI->m_KB.GetAgentInfo(pAI->m_iCrowdFocus);
		if (!pHostile)
		{
			pHostile = pAI->m_KB.AddAgent(pAI->m_iCrowdFocus);
		}
		if (pHostile)
		{
			CAI_Core* pHostileAI = pAI->GetAI(pAI->m_iCrowdFocus);
			if ((pHostileAI)&&(pHostileAI->m_pGameObject)&&(pHostileAI->m_pGameObject->AI_IsAlive()))
			{
				pHostile = NULL;
			}
		}
	}

	if ((pHostile)&&(pAI->SqrDistanceToUs(pHostile->GetObject()) < Sqr(m_MaxRange)))
	{
		iTarget = pHostile->GetObjectID();
	}

	return(iTarget);
};

//Watching is good defensively and somewhat exploratory
CAI_ActionEstimate CAI_Action_Watch::GetEstimate()
{
	CAI_ActionEstimate Est;
	if (IsValid())
	{
		CAI_Core* pAI = AI();

		// Dont break when a behaviour runs
		if (pAI->m_bBehaviourRunning)
		{
			return Est;
		}

		m_iTarget = FindTargetToWatch();

		if (m_iTarget)
		{
			//Default score 35
			Est.Set(CAI_ActionEstimate::OFFENSIVE,0); 
			Est.Set(CAI_ActionEstimate::DEFENSIVE,30);
			Est.Set(CAI_ActionEstimate::EXPLORATION,5);
			Est.Set(CAI_ActionEstimate::LOYALTY,0);
			Est.Set(CAI_ActionEstimate::LAZINESS,0);
			Est.Set(CAI_ActionEstimate::STEALTH,0);
			Est.Set(CAI_ActionEstimate::VARIETY,0);
		}
	}
	return Est;
};



//Try to keep hostile within sight
void CAI_Action_Watch::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Watch::OnTakeAction);
	CAI_Core* pAI = AI();

	if (!m_iTarget)
	{
		m_iTarget = FindTargetToWatch();
		if (m_iTarget)
		{
			SetExpiration(pAI->m_Patience);
			return;
		}
		else
		{	// No target, expire now
			m_iTarget = 0;
			SetExpirationExpired();
			return;
		}
	}

	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();
	CAI_AgentInfo* pHostile = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (pHostile)
	{
		if (pHostile->GetCurAwareness() < CAI_AgentInfo::DETECTED)
		{
			SetExpirationExpired();
			m_iTarget = 0;
			return;
		}
		if ((pHostile->GetCurRelation() < CAI_AgentInfo::HOSTILE)||((pHostile->GetCurRelation() > CAI_AgentInfo::HOSTILE_2)))
		{
			if (pAI->IsPlayer(m_iTarget))
			{
				if (pAI->SqrDistanceToUs(pHostile->GetBasePos()) > Sqr(64.0f))
				{
					m_iTarget = 0;
					return;
				}
				CAI_Core* pPlayerAI = pAI->GetAI(m_iTarget);
				if ((pPlayerAI)&&(CWObject_Character::Char_GetPhysType(pPlayerAI->m_pGameObject) == PLAYER_PHYS_NOCLIP))
				{
					m_iTarget = 0;
					return;
				}
			}
			else if (m_iTarget == pAI->m_iCrowdFocus)
			{
				if (pAI->SqrDistanceToUs(pHostile->GetBasePos()) > Sqr(MAX_DIALOGUE_CROWD_RANGE))
				{
					m_iTarget = 0;
					return;
				}
			}
			else
			{
				m_iTarget = 0;
				return;
			}
		}

		if (pHostile->GetCurRelation() < CAI_AgentInfo::HOSTILE)
		{	// Soft
			pAI->OnTrackAgent(pHostile,10,false,true);
		}
		else
		{	// Hard
			pAI->OnTrackAgent(pHostile,20,false,false);
		}
	}
	else
	{
		m_iTarget = 0;
		SetExpirationExpired();
	}
};


void CAI_Action_Watch::OnQuit()
{
	m_iTarget = 0;
	CAI_Action::OnQuit();
};

//Not overridable
bool CAI_Action_Watch::IsOverridableByActionOverride()
{
	return false;
};

//CAI_Action_Threaten////////////////////////////////////////////////////////////////////////////
CAI_Action_Threaten::CAI_Action_Threaten(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action_Watch(_pAH, _Priority)
{
	m_Type = THREATEN;
	m_TypeFlags = TYPE_SOUND | TYPE_ATTACK;
	m_MaxRange = 96;	// 3 m
	m_OrgStance = CAI_Device_Stance::IDLESTANCE_IDLE;
	m_LastRaiseTimer = 0;
};


//Mandatory: Move, Look, Sound, Animation, Weapon
int CAI_Action_Threaten::MandatoryDevices()
{
	return (1 << CAI_Device::ITEM);
};


//Threaten is pretty offensive and might be dangerous
CAI_ActionEstimate CAI_Action_Threaten::GetEstimate()
{
	CAI_ActionEstimate Est;
	if (IsValid())
	{
		m_iTarget = 0;
		CAI_AgentInfo* pHostile = m_pAH->FindRelation(false,CAI_AgentInfo::UNFRIENDLY,CAI_AgentInfo::HOSTILE_2);
		if (pHostile)
		{
			if (AI()->SqrDistanceToUs(pHostile->GetObject()) < Sqr(m_MaxRange))
			{
				m_iTarget = pHostile->GetObjectID();
				//Default score 75
				Est.Set(CAI_ActionEstimate::OFFENSIVE, 60); 
				Est.Set(CAI_ActionEstimate::DEFENSIVE, -10);
				Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
				Est.Set(CAI_ActionEstimate::LOYALTY, 0);
				Est.Set(CAI_ActionEstimate::LAZINESS, 0);
				Est.Set(CAI_ActionEstimate::STEALTH, 0);
				Est.Set(CAI_ActionEstimate::VARIETY, 25);
			}
		}
	}
	return Est;
};

void CAI_Action_Threaten::OnStart()
{
	CAI_Action_Watch::OnStart();

	m_OrgStance = AI()->m_DeviceStance.GetIdleStance();
	if (m_OrgStance < CAI_Device_Stance::IDLESTANCE_HOSTILE)
	{
		AI()->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
	}
}

void CAI_Action_Threaten::OnQuit()
{
	if (AI()->GetStealthTension() <= CAI_Core::TENSION_HOSTILE)
	{
		AI()->m_DeviceStance.SetIdleStance(m_OrgStance);
	}
	CAI_Action_Watch::OnQuit();
}

//Use appropriate forceful action and/or sound to make hostile stay away.
void CAI_Action_Threaten::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Threaten::OnTakeAction);
	CAI_Core* pAI = AI();
	
	CAI_AgentInfo* pHostile = pAI->m_KB.GetAgentInfo(m_iTarget);
	if ((!pHostile)||(pAI->SqrDistanceToUs(pHostile->GetObject()) > Sqr(m_MaxRange)))
	{
		SetExpirationExpired();
		return;
	}
	int Relation = pHostile->GetCurRelation();
	if ((Relation > CAI_AgentInfo::HOSTILE_2)||(Relation < CAI_AgentInfo::UNFRIENDLY))
	{
		SetExpirationExpired();
		return;
	}

	// This may take a while
	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();
	pHostile->Touch();	// Keep relation from falling below
	OnTrackAgent(pHostile,6);
	// *** Period here is one of those things that may be affected by game difficulty ***
	// *** Scale difficulty by time instead of hitpoints ***
	if ((pAI->IsPlayerLookWithinAngle(15))&&(Relation < CAI_AgentInfo::HOSTILE_2))
	{	// Go to HOSTILE_2 right now!
		SpeakRandom(CAI_Device_Sound::WARY_THREATEN);
		pHostile->SetRelation(CAI_AgentInfo::HOSTILE_2);
		m_LastRaiseTimer = pAI->m_Timer;
	}
	else if (m_LastRaiseTimer >= pAI->m_Timer + 40)
	{
		if (Relation < CAI_AgentInfo::HOSTILE_2)
		{
			SpeakRandom(CAI_Device_Sound::IDLE_WARNING);
		}
		else
		{
			SpeakRandom(CAI_Device_Sound::WARY_THREATEN);
		}
		pHostile->SetRelation(Relation+1);
		m_LastRaiseTimer = pAI->m_Timer;
	}

	// Whack!
	if (Relation > CAI_AgentInfo::HOSTILE_1)
	{
		if (pAI->m_Timer % 2 == 0)
		{
			pAI->m_DeviceItem.Use(CAI_Device_Item::USE);
		}
	}
	pAI->m_DeviceItem.Use(CAI_Device_Item::NONE);
};




//CAI_Action_Warn////////////////////////////////////////////////////////////////////////////
CAI_Action_Warn::CAI_Action_Warn(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action_Watch(_pAH, _Priority)
{
	m_Type = WARN;
	m_TypeFlags = TYPE_SOUND;
	m_MaxRange = 256.0f;	// 8m
	m_OrgStance = CAI_Device_Stance::IDLESTANCE_IDLE;
	m_bMeleeBlowTarget = false;
};


//Mandatory: Move, Look, Sound, Animation
int CAI_Action_Warn::MandatoryDevices()
{
	return (1 << CAI_Device::MOVE) | (1 << CAI_Device::LOOK);
};


//Warnings are a bit offensive and might be somewhat dangerous
CAI_ActionEstimate CAI_Action_Warn::GetEstimate()
{
	CAI_ActionEstimate Est;
	if ((IsValid())&&(AI()->GetStealthTension(false) < CAI_Core::TENSION_COMBAT))
	{
		m_iTarget = 0;
		CAI_AgentInfo* pHostile = AI()->m_KB.GetLastMeleeRelationIncrease();
		if (pHostile)
		{
			m_bMeleeBlowTarget = true;
			if (AI()->SqrDistanceToUs(pHostile->GetObject()) >= Sqr(AI_SOUNDDIALOGUE_MAXRANGE))
			{
				pHostile = NULL;
				m_bMeleeBlowTarget = false;
			}

		}

		if (!pHostile)
		{
			m_bMeleeBlowTarget = false;
			pHostile = m_pAH->FindRelation(false,CAI_AgentInfo::HOSTILE,CAI_AgentInfo::HOSTILE_2);
			if ((!pHostile)||(AI()->SqrDistanceToUs(pHostile->GetObject()) >= Sqr(m_MaxRange)))
			{
				pHostile = NULL;
			}
		}

		if (pHostile)
		{
			m_iTarget = pHostile->GetObjectID();
			//Default score 45
			Est.Set(CAI_ActionEstimate::OFFENSIVE, 30); 
			Est.Set(CAI_ActionEstimate::DEFENSIVE, -5);
			Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
			Est.Set(CAI_ActionEstimate::LOYALTY, 0);
			Est.Set(CAI_ActionEstimate::LAZINESS, 0);
			Est.Set(CAI_ActionEstimate::STEALTH, 0);
			Est.Set(CAI_ActionEstimate::VARIETY, 20);
		}
	}
	return Est;
};

void CAI_Action_Warn::OnStart()
{
	CAI_Action_Watch::OnStart();

	m_OrgStance = AI()->m_DeviceStance.GetIdleStance();
	if (AI()->GetStealthTension() < CAI_Core::TENSION_COMBAT)
	{
		SpeakRandom(CAI_Device_Sound::IDLE_WARNING);
		AI()->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
		m_OrgStance = CAI_Device_Stance::IDLESTANCE_HOSTILE;
	}
}

void CAI_Action_Warn::OnQuit()
{
	if (AI()->GetStealthTension() <= CAI_Core::TENSION_HOSTILE)
	{
		AI()->m_DeviceStance.SetIdleStance(m_OrgStance);
	}
	CAI_Action_Watch::OnQuit();
}

//Use appropriate gestures and/or sound to warn hostiles to stay away. 
void CAI_Action_Warn::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Warn::OnTakeAction);
	AI()->ActivateAnimgraph();
	if (m_bMeleeBlowTarget)
	{
		SetExpiration((int)(AI()->m_Patience * 0.33f));
	}
	else
	{
		SetExpiration(AI()->m_Patience);
	}
	CAI_Action_Watch::OnTakeAction();
};


//CAI_Action_Flee////////////////////////////////////////////////////////////////////////////
CAI_Action_Flee::CAI_Action_Flee(CAI_ActionHandler* _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = FLEE;
	m_TypeFlags = TYPE_SOUND;

	m_MaxRange = 512.0f;		// 16m
	m_BehaviourRange = 64.0f;	// 2m

	m_iTarget = 0;
	m_iBehaviour = 0;			// Behaviour nbr to play when triggered, default 0
	m_bCrouch = false;
	m_MoveMode = MODE_DEFAULT;

	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_HostilePosAtDecision);

	m_bUseScenePoints = true;
	m_pScenePoint = NULL;
};

int CAI_Action_Flee::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	if (m_iBehaviour)
	{
		AddUnique(_liBehaviours,m_iBehaviour);
		return(1);
	}
	else
	{
		return(0);
	}
};

void CAI_Action_Flee::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_BEHAVIOUR:
		m_iBehaviour = _Val;
		break;
	case PARAM_SCENEPOINTS:
		if (_Val != 0)
		{
			m_bUseScenePoints = true;
		}
		else
		{
			m_bUseScenePoints = false;
		}
		break;
	case PARAM_DARKNESS:
		if (_Val != 0)
		{
			m_bDarkness = true;
		}
		else
		{
			m_bDarkness = false;
		}
		break;
	case PARAM_MOVEMODE:
		{
			if ((_Val >= MODE_DEFAULT)&&(_Val <= MODE_RUN))
			{
				m_MoveMode = _Val;
			}
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Flee::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_RANGE:
		m_MaxRange = _Val;
		break;

	case PARAM_BEHAVIOUR_RANGE:
		m_BehaviourRange = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Flee::SetParameter(int _Param, CStr _Val)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_BEHAVIOUR:
	case PARAM_DARKNESS:
	case PARAM_MOVEMODE:
		{
			SetParameter(_Param,_Val.Val_int());
		}
		break;
	case PARAM_RANGE:
	case PARAM_BEHAVIOUR_RANGE:
		{
			SetParameter(_Param, (fp32)_Val.Val_fp64());
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get parameter ID from sting (used when parsing registry)
int CAI_Action_Flee::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "MAXRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_RANGE;
	}
	if (_Str == "BEHAVIOURRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_BEHAVIOUR_RANGE;
	}
	else if (_Str == "BEHAVIOUR")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_INT;
		return PARAM_BEHAVIOUR;
	}
	else if (_Str == "DARKNESS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_DARKNESS;
	}
	else if (_Str == "SCENEPOINTS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_SCENEPOINTS;
	}
	else if (_Str == "MOVEMODE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_MOVEMODE;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
}

//Mandatory: Move
int CAI_Action_Flee::MandatoryDevices()
{
	return (1 << CAI_Device::MOVE);
}

//Valid when there's enemy about
bool CAI_Action_Flee::IsValid()
{
	return(CAI_Action::IsValid());	
}

//Avoid is usually a good action for cowards when there's enemy around
CAI_ActionEstimate CAI_Action_Flee::GetEstimate()
{
	CAI_ActionEstimate Est;
	if (IsValid())
	{

		CAI_Core* pAI = AI();
		m_iTarget = 0;
		if (pAI->m_DeviceStance.IsCrouching())
		{
			return Est;
		}

		// Find an appropriate agent
		CAI_AgentInfo* pHostile = NULL;
		if (m_bDarkness)
		{
			int iPlayer = pAI->m_KB.GetPlayerDarkness();
			if (iPlayer)
			{
				pHostile = pAI->m_KB.GetAgentInfo(iPlayer);
				if ((!pHostile)||(pHostile->GetCurRelation() >= CAI_AgentInfo::ENEMY))
				{
					pHostile = NULL;
				}
			}
		}
		if (!pHostile)
		{
			pHostile = m_pAH->FindRelation(true,CAI_AgentInfo::ENEMY,CAI_AgentInfo::ENEMY,CAI_AgentInfo::NOTICED);
		}
		if ((pHostile)&&(pHostile->GetCurAwareness() >= CAI_AgentInfo::NOTICED))
		{
			fp32 Range;
			if (m_bUseScenePoints)
			{
				Range = m_MaxRange;
			}
			else
			{
				Range = m_BehaviourRange;
			}
			if (pAI->SqrDistanceToUs(pHostile->GetBasePos()) <= Sqr(Range))
			{
				m_iTarget = pHostile->GetObjectID();
				//Default score 35
				Est.Set(CAI_ActionEstimate::OFFENSIVE,0); 
				Est.Set(CAI_ActionEstimate::DEFENSIVE,30);
				Est.Set(CAI_ActionEstimate::EXPLORATION,5);
				Est.Set(CAI_ActionEstimate::LOYALTY,0);
				Est.Set(CAI_ActionEstimate::LAZINESS,0);
				Est.Set(CAI_ActionEstimate::STEALTH,0);
				Est.Set(CAI_ActionEstimate::VARIETY,0);
			}
		}
	}
	return Est;
}

// Tries to find a destination position from available scenepoints
bool CAI_Action_Flee::FindScenePointDestination(const CVec3Dfp32& _TargetPos)
{
	CAI_Core* pAI = AI();

	// Try to find a valid scenepoint
	if (_TargetPos.DistanceSqr(pAI->GetBasePos()) > Sqr(m_MaxRange))
	{
		m_pScenePoint = NULL;
		m_Destination = CVec3Dfp32(_FP32_MAX);
		SetExpirationExpired();
		return(false);
	}

	CMat4Dfp32 OurMat;
	pAI->GetBaseMat(OurMat);
	m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::COVER,CAI_ActionHandler::SP_AVOID_TARGET,
		m_BehaviourRange,m_MaxRange,OurMat,_TargetPos,m_BehaviourRange * 2.0f,0);
	INVALIDATE_POS(m_Destination);
	if (!m_pScenePoint)
	{
		return(false);
	}
	if (m_pScenePoint->GetType() & CWO_ScenePoint::JUMP_CLIMB)
	{
		m_Destination = m_pScenePoint->GetPosition();
	}
	else
	{
		m_Destination = pAI->m_PathFinder.GetPathPosition(m_pScenePoint->GetPosition(),4,2);
	}
	if ((!m_pAH->RequestScenePoint(m_pScenePoint))||(INVALID_POS(m_Destination))||(m_pAH->IsRestricted(m_Destination)))
	{
		INVALIDATE_POS(m_Destination);
		m_pAH->UpdateScenepointHistory(m_pScenePoint);
		m_pScenePoint = NULL;
		return(false);
	}

	return(true);
};

// Walk away from the direct vicinity of pHostile and track him 
bool CAI_Action_Flee::AvoidThreat(CAI_AgentInfo* pHostile,fp32 _MaxRange,fp32 _RetreatRange,bool _bDuckWhenCornered)
{	
	CAI_Core* pAI = AI();
	if ((!pHostile)||(!pAI->m_pGameObject))
	{
		return(false);
	}

	// OnTrackAgent(pHostile,10,false);

	fp32 RangeSqr = pAI->SqrDistanceToUs(pHostile->GetObjectID());
	CVec3Dfp32 HostilePos = pHostile->GetBasePos();	// Get correct pos to avoid embarrasing behaviours
	CVec3Dfp32 OurPos = pAI->GetBasePos();

	CVec3Dfp32 RetreatVector = OurPos - HostilePos;
	RetreatVector.Normalize();
	RetreatVector = RetreatVector * _RetreatRange;
	CVec3Dfp32 Goal = OurPos + RetreatVector;
	if (VALID_POS(m_Destination))
	{	// Target has moved a meter since we last decided where to go
		if ((VALID_POS(m_HostilePosAtDecision))&&(m_HostilePosAtDecision.DistanceSqr(HostilePos) > Sqr(32.0f)))
		{
			INVALIDATE_POS(m_HostilePosAtDecision);
			INVALIDATE_POS(m_Destination);
		}
		else if (OurPos.DistanceSqr(m_Destination) < 8.0f)
		{	// We are pretty close to our target
			pAI->ResetStuckInfo();
			pAI->m_DeviceMove.Use();
			return(false);
		}
	}

	if (INVALID_POS(m_Destination))
	{	// Find new destination
		if (RangeSqr > Sqr(_MaxRange))
		{
			return(false);
		}

		CVec3Dfp32 StopPos;
		bool foundGoal = true;
		for (int k = 0; k < 6; k++)
		{
			if (!foundGoal)
			{
				fp32 Angle = Random * _PI2;
				CVec3Dfp32 Offset;
				Offset[0] += M_Cos(Angle);
				Offset[1] += M_Sin(Angle);
				Offset[2] = 0.0f;
				Offset.Normalize();
				Goal = OurPos + (RetreatVector * 0.5f) + (Offset * _RetreatRange * 0.25f);
			}

			if ((pAI->m_PathFinder.GetBlockNav())&&
				(!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,Goal,
				(int)pAI->GetBaseSize(),
				(int)pAI->GetHeight(),
				pAI->m_bWallClimb,
				(int)pAI->GetStepHeight(),
				(int)pAI->GetJumpHeight(),
				&StopPos)))
			{	// Movement blocked
				if ((k >= 5)&&(RangeSqr < Sqr(m_BehaviourRange))&&(_bDuckWhenCornered)&&(m_iBehaviour)&&(pAI->m_Timer >= pAI->m_LastHurtTimer + 100))
				{
					SetWantedBehaviour(m_iBehaviour,20,NULL);
					SetExpirationExpired();
				}
				foundGoal = false;
				m_Destination = StopPos;
				m_HostilePosAtDecision = HostilePos;
			}
			else
			{
				foundGoal = true;
				m_Destination = Goal;
				m_HostilePosAtDecision = HostilePos;
				break;	// Break the loop
			}
		}

		if (INVALID_POS(m_Destination)&&(RangeSqr < Sqr(m_BehaviourRange))&&(_bDuckWhenCornered)&&(m_iBehaviour)&&(pAI->m_Timer >= pAI->m_LastHurtTimer + 100))
		{
			SetWantedBehaviour(m_iBehaviour,20,NULL);
			INVALIDATE_POS(m_Destination);
			INVALIDATE_POS(m_HostilePosAtDecision);
			SetExpirationExpired();
			return(true);
		}
	}

	if (INVALID_POS(m_Destination))
	{
		return(false);
	}

	// Just retreat a bit looking at the target
	int32 moveResult;
	if (pHostile->GetCurRelation() >= CAI_AgentInfo::ENEMY)
	{
		moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseMaxSpeed,false,false,NULL);
	}
	else
	{
		moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,false,false,NULL);
	}
	if (!pAI->CheckMoveresult(moveResult,NULL))
	{
		INVALIDATE_POS(m_Destination);
		return(false);
	}

	OnTrackAgent(pHostile,10,false);

	return(true);
};

// Go to the nearest scenepoint, cwoard duck if getting too close to pHostile
bool CAI_Action_Flee::HandleCoverpoint(CAI_AgentInfo* pHostile,bool _bDuckWhenClose)
{
	CAI_Core* pAI = AI();

	if (!pHostile)
	{
		return(false);
	}

	fp32 RangeSqr = pAI->SqrDistanceToUs(pHostile->GetObjectID());
	if (RangeSqr > Sqr(m_MaxRange))
	{
		return(false);
	}

	if ((_bDuckWhenClose)&&(m_iBehaviour)&&(RangeSqr <= Sqr(m_BehaviourRange)))
	{
		if (m_iBehaviour)
		{
			SetWantedBehaviour(m_iBehaviour,0,NULL);
			SetExpirationExpired();
			INVALIDATE_POS(m_Destination);
			m_iTarget = 0;
			return(true);
		}
		else
		{
			// No more walking neccessary
			OnTrackAgent(pHostile,10,false);
			pAI->m_DeviceMove.Use();
			pAI->m_DeviceStance.Crouch(true);
			m_bCrouch = true;
			return(true);
		}
		
	}

	if ((!m_pScenePoint)||(INVALID_POS(m_Destination)))
	{
		if (!FindScenePointDestination(pHostile->GetSuspectedPosition()))
		{
			return(false);
		}
	}

	// We have a scenepoint, go there etc
	if ((!m_pScenePoint)||(INVALID_POS(m_Destination)))
	{
		return(false);
	}

	// Is the scenepoint (if any) valid?
	bool IsAt = false;

	if (m_pAH->RequestScenePoint(m_pScenePoint))
	{
		// Ignore arc after SP is taken
		if ((m_StayTimeout > 0)||(m_pScenePoint->InFrontArc(pHostile->GetBasePos())))
		{
			if (m_pScenePoint->IsAt(pAI->GetBasePos()))
			{
				IsAt = true;
			}
		}
	}
	else
	{	// Scenepoint was bad or no longer relevant visavi target
		// We return true here as we don't want some other behaviour kicking in, if we fail to find an SP next tick...
		m_pScenePoint = NULL;
		m_StayTimeout = -1;
		INVALIDATE_POS(m_Destination);
		return(true);
	}

	// Redundant redundancy, I know, I know.
	if (IsAt)
	{
#ifndef M_RTM
		m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
		if (m_StayTimeout == -1)
		{
//			int StayTicks = (int)(m_pScenePoint->GetBehaviourDuration() * pAI->GetAITicksPerSecond());
			// m_StayTimeout = pAI->m_Timer + StayTicks;
			m_StayTimeout = pAI->m_Timer + 60 * 60 * 20;	// Stay for 1 hour
			CWO_ScenePoint* orgSP = m_pScenePoint;
			m_pAH->ActivateScenePoint(m_pScenePoint,GetPriority());
			if (orgSP != m_pScenePoint) { return(false); }
			// Reset expiration to guarantee(?) that we will stay 
			SetExpirationExpired();
			SetExpirationIndefinite(); // Note: we stay here forever
		}
		if ((m_pScenePoint)&&(m_pScenePoint->GetBehaviour())&&(m_pScenePoint->GetBehaviourDuration() == 0.0f))
		{
			m_StayTimeout = pAI->m_Timer + pAI->GetAITicksPerSecond();
		}
		// Forever I tell you
		m_StayTimeout = pAI->m_Timer + pAI->GetAITicksPerSecond();

		// No more walking neccessary
		OnTrackAgent(pHostile,10,false);
		pAI->m_DeviceMove.Use();
		pAI->m_DeviceStance.Crouch(true);
		m_bCrouch = true;
		return(true);
	}

	// Not there yet, keep moving
#ifndef M_RTM
	m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif

	// Bot there, move towards the spot
	fp32 FleeSpeed;
	switch(m_MoveMode)
	{
	case MODE_WALK:
		{
			FleeSpeed = pAI->m_ReleaseIdleSpeed;
		}
		break;

	case MODE_RUN:
		{
			FleeSpeed = pAI->m_ReleaseMaxSpeed;
		}
		break;

	default:	// MODE_DEFAULT:
		{
			FleeSpeed = pAI->m_ReleaseMaxSpeed;
		}
		break;
	}

	//Move there (use perfect placement in case radius is low)
	int32 moveResult = pAI->OnMove(m_Destination,FleeSpeed,false,false,NULL);
	if (!pAI->CheckMoveresult(moveResult,NULL))
	{
		m_pScenePoint = NULL;
		m_StayTimeout = -1;
		INVALIDATE_POS(m_Destination);
		return(false);
	}
	pAI->m_DeviceMove.Use();
	return(true);
};

// Duck when player is aiming at us with something we couldn't handle
bool CAI_Action_Flee::DuckForGun()
{
	CAI_Core* pAI = AI();

	int iPlayer = pAI->GetClosestPlayer();
	CAI_AgentInfo* pHostile = pAI->m_KB.GetAgentInfo(iPlayer);
	if (!pHostile)
	{
		return(false);
	}
	CAI_Core* TargetAI = pAI->GetAI(pHostile->GetObjectID());
	if (!TargetAI)
	{
		return(false);
	}
	int TargetArmsClass = TargetAI->m_Weapon.GetWieldedArmsClass();
	if (TargetArmsClass < CAI_WeaponHandler::AI_ARMSCLASS_GUN)
	{
		return(false);
	}
	int Relation = pHostile->GetCurRelation();
	if (pHostile->GetCurAwareness() < CAI_AgentInfo::SPOTTED)
	{	
		return(false);
	}

	if (Relation < CAI_AgentInfo::HOSTILE)
	{
		pHostile->SetRelation(CAI_AgentInfo::HOSTILE);
	}
	
	if (pAI->IsPlayerLookWithinAngle(15))
	{
		SpeakRandom(CAI_Device_Sound::AFRAID_PLEAD_FOR_LIFE);
		if (m_iBehaviour)
		{
			OnTrackAgent(pHostile,10,false);
			SetWantedBehaviour(m_iBehaviour,-1,NULL);
			SetExpirationExpired();
			INVALIDATE_POS(m_Destination);
			m_iTarget = 0;
			return(true);
		}
		else
		{
			pAI->m_DeviceStance.Crouch(true);
			m_bCrouch = true;
			pAI->m_DeviceMove.Free();
			pAI->m_DeviceMove.Use();
			return(true);
		}
	}
	else
	{
		if ((!m_bCrouch)||(!pAI->IsPlayerLookWithinAngle(45)))
		{
			pAI->m_DeviceStance.Crouch(false);
			pAI->m_DeviceMove.Use();
		}
	}

	return(false);
};

void CAI_Action_Flee::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		INVALIDATE_POS(m_Destination);
		m_StayTimeout = -1;
		m_pScenePoint = NULL;
	}
}

void CAI_Action_Flee::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Flee::OnTakeAction);
	CAI_Core* pAI = AI();

	// Handle the preliminaries
	CAI_AgentInfo* pHostile = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (!pHostile)
	{
		SetExpirationExpired();
		return;
	}

	int Relation = pHostile->GetCurRelation();
	int Awareness = pHostile->GetCurAwareness();

	CAI_Core* pHostileAI = pHostile->GetAgentAI();
	if (!pHostileAI)
	{
		SetExpirationExpired();
		return;
	}

	if ((!pHostile)||(Relation < CAI_AgentInfo::ENEMY))
	{
		SetExpirationExpired();
		return;
	}

	// Are we restricted?
	if ((VALID_POS(m_Destination))&&(m_pAH->IsRestricted(m_Destination)))
	{	
		if (pAI->DebugRender())
		{
			CStr Name = pAI->m_pGameObject->GetName();
			ConOut(Name+CStr(": CAI_Action_Flee restricted"));
		}
		pAI->OnTrackAgent(pHostile,10,false,false);
		ExpireWithDelay(60);
		return;
	}

	if (pAI->m_DeviceStance.IsCrouching())
	{
		SetExpirationExpired();
		return;
	}

	// OK we have or go
	pAI->ActivateAnimgraph();
	if (Relation >= CAI_AgentInfo::ENEMY)
	{
		SetExpirationIndefinite();
	}
	
	if (Relation > CAI_AgentInfo::NEUTRAL)
	{
		if (Relation >= CAI_AgentInfo::ENEMY)
		{	// Combat/panic
			if (Awareness >= CAI_AgentInfo::SPOTTED)
			{
				pHostile->TouchRelation();
			}
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		}
		else
		{	// Hostile/wary
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
		}
	}

	bool bCanDuck = false;
	if (m_iBehaviour)
	{
		bCanDuck = true;
	}

	// Decide what kind of situation we are in
	// Check scenepoints first
	if (HandleCoverpoint(pHostile,true))
	{
		pAI->m_DeviceMove.Use();
		return;
	}

	/*
	// Just move away from the threat
	if (AvoidThreat(pHostile,m_MaxRange,96.0f,bCanDuck))
	{
		pAI->m_DeviceMove.Use();
		return;
	}
	*/

	if (DuckForGun())
	{
		return;
	}
};

void CAI_Action_Flee::OnStart()
{
	CAI_Core* pAI = AI();
	
	CAI_Action::OnStart();
	m_bCrouch = false;
	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_HostilePosAtDecision);
	m_pScenePoint = NULL;
	m_StayTimeout = -1;
	if (m_iTarget)
	{
		CAI_AgentInfo* pHostile = pAI->m_KB.GetAgentInfo(m_iTarget);
		if ((pHostile)&&(pHostile->GetAwareness() >= CAI_AgentInfo::SPOTTED))
		{
			if (pAI->m_CharacterClass == CAI_Core::CLASS_CIV)
			{
				SpeakRandom(CAI_Device_Sound::AFRAID_PANIC);
			}
		}
	}
};

void CAI_Action_Flee::OnQuit()
{
	CAI_Core* pAI = AI();

	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_HostilePosAtDecision);
	m_iTarget = 0;
	m_pScenePoint = NULL;

	CAI_Action::OnQuit();
};

//CAI_Action_KeepDistance////////////////////////////////////////////////////////////////////////////
const char* CAI_Action_KeepDistance::ms_lStrFlags[] = 
{
	"HOSTILE",
	"ENEMY",
	"LOOK",
	"CORNERED",
	"GUN",
	"PLAYER",
	"JUMP",
	"CROUCH",
	NULL,
};

CAI_Action_KeepDistance::CAI_Action_KeepDistance(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = KEEP_DISTANCE;
	m_TypeFlags = TYPE_MOVE | TYPE_DEFENSIVE;

	m_Flags = 0;
	m_MaxRange = 256.0f;
	m_Distance = 64.0f;
	m_iBehaviour = 0;
	m_BehaviourRange = 512.0f;
	m_bCrouch = false;
	m_MoveMode = MODE_DEFAULT;
	m_iTarget = 0;
	INVALIDATE_POS(m_TargetPos);
	INVALIDATE_POS(m_Destination);
	m_pScenePoint = NULL;
};

int CAI_Action_KeepDistance::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	if (m_iBehaviour)
	{
		AddUnique(_liBehaviours,m_iBehaviour);
		return(1);
	}
	else
	{
		return(0);
	}
};

//Set params
void CAI_Action_KeepDistance::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_BEHAVIOUR:
		m_iBehaviour = _Val;
		break;

	case PARAM_MOVEMODE:
		if ((_Val >= MODE_DEFAULT)&&(_Val <= MODE_RUN))
		{
			m_MoveMode = _Val;
		}
		break;

	case PARAM_FLAGS:
		m_Flags = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}
void CAI_Action_KeepDistance::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_MAXRANGE:
		m_MaxRange = _Val;
		break;

	case PARAM_DISTANCE:
		m_Distance = _Val;
		break;

	case PARAM_BEHAVIOURRANGE:
		m_BehaviourRange = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}
void CAI_Action_KeepDistance::SetParameter(int _Param, CStr _Val)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_MAXRANGE:
	case PARAM_DISTANCE:
	case PARAM_BEHAVIOURRANGE:
			SetParameter(_Param, (fp32)_Val.Val_fp64());
		break;

	case PARAM_BEHAVIOUR:
	case PARAM_MOVEMODE:
		SetParameter(_Param, _Val.Val_int());
		break;

	case PARAM_FLAGS:
		SetParameter(_Param, StrToFlags(_Val));
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

int CAI_Action_KeepDistance::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

//Get parameter ID from sting (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_KeepDistance::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "MAXRANGE")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_MAXRANGE;
	}
	else if (_Str == "DISTANCE")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_DISTANCE;
	}
	else if (_Str == "BEHAVIOUR")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_INT;
		return PARAM_BEHAVIOUR;
	}
	else if (_Str == "BEHAVIOURRANGE")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_BEHAVIOURRANGE;
	}
	else if (_Str == "MOVEMODE")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_INT;
		return PARAM_MOVEMODE;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType)
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};


int CAI_Action_KeepDistance::MandatoryDevices()
{
	return(CAI_Action::MandatoryDevices());
};


//Valid if we've got position and distance restrictions
bool CAI_Action_KeepDistance::IsValid()
{
	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		return(false);
	}
	if (!m_iTarget)
	{
		return(true);
	}

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	CVec3Dfp32 TargetPos;
	if (pTarget)
	{
		if ((m_Flags & FLAG_PLAYER)&&(pAI->IsPlayer(pTarget->GetObjectID())))
		{
			TargetPos = pTarget->GetBasePos();
			if (pAI->SqrDistanceToUs(TargetPos) <= Sqr(m_MaxRange * 1.5f))
			{
				return(true);
			}
			else
			{
				m_iTarget = 0;
				return(false);
			}
		}
		else
		{
			TargetPos = pTarget->GetSuspectedPosition();
			if ((pAI->SqrDistanceToUs(TargetPos) > Sqr(m_MaxRange * 1.5f))&&(!pAI->m_DeviceStance.IsCrouching()))
			{
				m_iTarget = 0;
				return(false);
			}
			int Awareness = pTarget->GetCurAwareness();
			int Relation = pTarget->GetCurRelation();
			if ((m_Flags & FLAG_HOSTILE)&&(Relation >= CAI_AgentInfo::HOSTILE)&&(Relation <= CAI_AgentInfo::HOSTILE_2))
			{
				return(true);
			}
			if ((m_Flags & FLAG_ENEMY)&&(Relation == CAI_AgentInfo::ENEMY))
			{
				if (Awareness >= CAI_AgentInfo::SPOTTED)
				{
					pTarget->TouchRelation();
				}
				return(true);
			}
		}
	}

	m_iTarget = 0;

	return(false);
};


//Always good (since we should have no choice in the matter), but better the closer we get to maxdistance
CAI_ActionEstimate CAI_Action_KeepDistance::GetEstimate()
{
	CAI_ActionEstimate Est;
	CAI_Core* pAI = AI();

	if (IsValid())
	{	
		CAI_AgentInfo* pTarget = NULL;
		int Relation = 0;
		int Awareness = 0;
//		int iTarget = 0;

		// Check for player within range first

		// We must make sure this doesn't get too expensive
		if (!m_iTarget)
		{	// We prefer the player as a target
			int iTarget = pAI->GetClosestPlayer();
			if (iTarget)
			{
				pTarget = pAI->m_KB.GetAgentInfo(iTarget);
				if (pTarget)
				{
					if (m_Flags & FLAG_PLAYER)
					{	// Just avoid the player, nothing else is more important
						m_iTarget = iTarget;
					}
					else
					{
						Relation = pTarget->GetCurRelation();
						Awareness = pTarget->GetCurAwareness();
						if (Awareness >= CAI_AgentInfo::DETECTED)
						{
							if (Relation >= CAI_AgentInfo::HOSTILE)
							{
								if ((m_Flags & FLAG_HOSTILE)&&
									(Relation >= CAI_AgentInfo::HOSTILE)&&(Relation <= CAI_AgentInfo::HOSTILE_2))
								{
									m_iTarget = iTarget;
								}
								else if ((m_Flags & FLAG_ENEMY)&&(Relation == CAI_AgentInfo::ENEMY))
								{
									m_iTarget = iTarget;
								}
							}
							else if ((m_Flags & FLAG_GUN)&&(Awareness >= CAI_AgentInfo::SPOTTED)&&(Relation == CAI_AgentInfo::UNFRIENDLY))
							{	// Check for wielded gun
								CAI_Core* pTargetAI = pAI->GetAI(iTarget);
								if ((pTargetAI)&&(pTargetAI->m_Weapon.GetWieldedArmsClass() >= CAI_WeaponHandler::AI_ARMSCLASS_GUN))
								{
									pTarget->SetRelation(CAI_AgentInfo::HOSTILE, (int)(10+20*Random));	// 0.5s to 1.5s delay
									if (pAI->m_CharacterClass == CAI_Core::CLASS_CIV)
									{
										SpeakRandom(CAI_Device_Sound::SPOT_PLAYER_GUN);
									}
								}
							}
						}
					}
				}
			}
		}

		if (!m_iTarget)
		{
			if (m_Flags & (FLAG_HOSTILE | FLAG_ENEMY))
			{
				int MinRelation = CAI_AgentInfo::HOSTILE;
				int MaxRelation = CAI_AgentInfo::ENEMY;
				if (!(m_Flags & FLAG_HOSTILE))
				{
					MinRelation = CAI_AgentInfo::ENEMY;
				}
				if (!(m_Flags & FLAG_ENEMY))
				{
					MaxRelation = CAI_AgentInfo::HOSTILE;
				}
				pTarget = m_pAH->FindRelation(true,MinRelation,MaxRelation,CAI_AgentInfo::DETECTED);
				if (pTarget)
				{
					m_iTarget = pTarget->GetObjectID();
				}
			}
			
		}

		if (m_iTarget)
		{
			pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
			if (!pTarget)
			{
				return Est;
			}

			CVec3Dfp32 OurPos,TargetPos;
			OurPos = pAI->GetBasePos();
			if (m_Flags & FLAG_PLAYER)
			{
				TargetPos = pTarget->GetBasePos();
			}
			else
			{
				TargetPos = pTarget->GetSuspectedPosition();
			}
			if ((OurPos.DistanceSqr(TargetPos) > Sqr(m_BehaviourRange))&&
				(OurPos.DistanceSqr(TargetPos) > Sqr(m_MaxRange)))
			{
				return Est;
			}

			//Default score 100
			Est.Set(CAI_ActionEstimate::OFFENSIVE, 0); 
			Est.Set(CAI_ActionEstimate::DEFENSIVE, 10);
			Est.Set(CAI_ActionEstimate::EXPLORATION, -10);
			Est.Set(CAI_ActionEstimate::LOYALTY, 100);
			Est.Set(CAI_ActionEstimate::LAZINESS, 20);
			Est.Set(CAI_ActionEstimate::STEALTH, 0);
			Est.Set(CAI_ActionEstimate::VARIETY, -20);
		}
	}
	return Est;
};

void CAI_Action_KeepDistance::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		INVALIDATE_POS(m_Destination);
		m_pScenePoint = NULL;
	}
};

void CAI_Action_KeepDistance::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_KeepDistance::OnTakeAction);
	CAI_Core* pAI = AI();

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (!pTarget)
	{
		SetExpirationExpired();
		return;
	}
	CAI_Core* pTargetAI = pTarget->GetAgentAI();
	if (!pTargetAI)
	{
		SetExpirationExpired();
		return;
	}
	CVec3Dfp32 OurPos,TargetPos;
	if (m_Flags & FLAG_PLAYER)
	{
		TargetPos = pTargetAI->GetBasePos();
	}
	else
	{
		TargetPos = pTarget->GetSuspectedPosition();
	}

	TargetPos = pAI->Proj2Base(TargetPos);

	// Proj2Base will change the value of OurPos only if our up is not "up"
	OurPos = pAI->GetBasePos();
	OurPos = pAI->Proj2Base(OurPos);

	if (!pAI->m_bWallClimb)
	{
		OurPos = pAI->m_PathFinder.SafeGetPathPosition(OurPos,4,4);
	}

	if (m_Flags & FLAG_PLAYER)
	{	// We're within range but we should expire if not in the front cone
		CVec3Dfp32 LookDir = pTargetAI->GetLookDir();
		LookDir[2] = 0.0f;	// OK for wallclimbers?
		CVec3Dfp32 Diff = OurPos - TargetPos;
		LookDir[2] = 0.0f;
		LookDir.Normalize();
		Diff.Normalize();
		if (LookDir * Diff < 0.0f)
		{
			SetExpirationExpired();
			return;
		}
	}

	// Only jump when really close
	if ((m_Flags & FLAG_JUMP)&&(OurPos.DistanceSqr(TargetPos) <= Sqr(m_MaxRange * 0.5f)))
	{	// Check for jump scenepoints, if none found just jump away from the guy
		// Generate a direction that point's away from TargetPos, and upwards
		CVec3Dfp32 Dir = OurPos - TargetPos;
		Dir.Normalize();
		Dir += pAI->GetUp();
		Dir += CVec3Dfp32(Random-0.5f,Random-0.5f,Random-0.5f);
		pAI->TempEnableWallClimb((int)(3.0f * CAI_Core::AI_TICKS_PER_SECOND));
		if (pAI->m_bWallClimb)
		{
			CMat4Dfp32 Mat;
			pAI->GetBaseMat(Mat);
			CVec3Dfp32 Dst = OurPos + Dir * m_MaxRange;
			Dst.SetRow(Mat,3);
			if (pAI->AddJumpTowardsPositionCmd(&Mat,CAI_Core::JUMPREASON_AVOID,CAI_Core::JUMP_CHECK_COLL))
			{
#ifdef M_Profile
				pAI->Debug_RenderWire(OurPos,Dst,kColorGreen,3.0f);
#endif
				SetExpirationExpired();
				return;
			}
		}
	}

	if ((!pAI->m_PathFinder.IsValid())||(!pAI->m_PathFinder.GridPF()))
	{
		SetExpirationExpired();
		return;
	}

	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();

	int Awareness = pTarget->GetCurAwareness();
	int Relation = pTarget->GetCurRelation();
	if (Relation > CAI_AgentInfo::NEUTRAL)
	{
		if (Relation >= CAI_AgentInfo::ENEMY)
		{	// Combat/panic
			if (Awareness >= CAI_AgentInfo::SPOTTED)
			{
				pTarget->TouchRelation();
			}
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		}
		else
		{	// Hostile/wary
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_HOSTILE);
		}
	}
	if (pAI->m_DeviceStance.IsCrouching())
	{	// No rotation when crouched
		pAI->m_DeviceMove.Use();
		pAI->m_DeviceLook.Use();
	}

	if ((Relation >= CAI_AgentInfo::HOSTILE)&&(Awareness >= CAI_AgentInfo::SPOTTED))
	{
		if (pTargetAI->m_Weapon.GetWieldedArmsClass() >= CAI_WeaponHandler::AI_ARMSCLASS_GUN)
		{
			if ((m_iBehaviour)&&(OurPos.DistanceSqr(TargetPos) <= Sqr(m_BehaviourRange)))
			{
				SpeakRandom(CAI_Device_Sound::AFRAID_PLEAD_FOR_LIFE);
				if (SetWantedBehaviour(m_iBehaviour,-1,NULL))
				{	// We don't expire as this may give the damned Roam a few ticks
					// before we kick in after behaviour has finished.
					pAI->m_DeviceMove.Use();
					return;
				}
			}

			if (m_Flags & FLAG_CROUCH)
			{
				if (!pAI->m_DeviceStance.IsCrouching())
				{	// Not crouching
					if (pAI->IsLookWithinAngle(15.0f,m_iTarget))
					{
						SpeakRandom(CAI_Device_Sound::AFRAID_PLEAD_FOR_LIFE);
						pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
						pAI->m_DeviceStance.Crouch(true);
						pAI->m_DeviceMove.Use();
						pAI->m_DeviceLook.Use();
					}
				}
				else // Crouching
				{
					if ((!pAI->IsLookWithinAngle(90.0f,m_iTarget))&&(!pTarget->InLOS()))
					{
						pAI->m_DeviceStance.Crouch(false);
						pAI->m_DeviceMove.Use();
						pAI->m_DeviceLook.Use();
					}
				}
			}
		}
	}
	else	// Relation is less than hostile or awareness is less than Spotted
	{
		if ((Relation < CAI_AgentInfo::ENEMY)&&(!pAI->IsLookWithinAngle(90.0f,m_iTarget))&&(!pTarget->InLOS()))
		{
			pAI->m_DeviceStance.Crouch(false);
			pAI->m_DeviceMove.Use();
			pAI->m_DeviceLook.Use();
		}
	}

	// We're valid up until m_MaxRange x 1.5
	if (OurPos.DistanceSqr(TargetPos) > Sqr(m_MaxRange * 1.5f))
	{
		SetExpirationExpired();
		return;
	}

	if ((m_Flags & FLAG_LOOK)&&(Relation < CAI_AgentInfo::ENEMY)&&(Awareness >= CAI_AgentInfo::SPOTTED))
	{
		pAI->OnTrackObj(m_iTarget,10,false,false);
	}

	if (FindDestination(pTarget))
	{
		if (MoveToDestination(pTarget))
		{
			StayAtDestination(pTarget);
		}
	}
	// Hog the move device
	pAI->m_DeviceMove.Use();
};

/*
bool CAI_Action_KeepDistance::FindScenepointDestination(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();
	if (_pTarget)
	{
		int Relation = _pTarget->GetCurRelation();
		int Awareness = _pTarget->GetCurAwareness();
	}
};
*/
bool CAI_Action_KeepDistance::FindDestination(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();
	if (_pTarget)
	{
		int Relation = _pTarget->GetCurRelation();
		int Awareness = _pTarget->GetCurAwareness();
		// Invalidate m_Destination if target has moved much
		CVec3Dfp32 TargetPos = _pTarget->GetSuspectedPosition();
		if ((INVALID_POS(m_TargetPos))||(TargetPos.DistanceSqr(m_TargetPos) > Sqr(32.0f))||(pAI->m_Timer % 40 == 0))
		{	// This gets pretty expensive!
			INVALIDATE_POS(m_Destination);
		}

		if (VALID_POS(m_Destination))
		{	
			return(true);
		}
		else // m_Destination invalid
		{	// Set up a new move away from the threat
			m_TargetPos = TargetPos;
			CVec3Dfp32 OurPos = pAI->GetBasePos();
			OurPos = pAI->Proj2Base(OurPos);
			OurPos = pAI->m_PathFinder.SafeGetPathPosition(OurPos,1,1);
			TargetPos = pAI->Proj2Base(TargetPos);
			TargetPos = pAI->m_PathFinder.SafeGetPathPosition(TargetPos,1,1);

			// Is there a cover scenepoint somewhere we can use?
			if (Relation >= CAI_AgentInfo::ENEMY)
			{
				CMat4Dfp32 OurMat;
				pAI->GetBaseMat(OurMat);
				m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::COVER,CAI_ActionHandler::SP_AVOID_TARGET,
					64.0f,m_MaxRange,OurMat,TargetPos,m_BehaviourRange,0);
				if (m_pScenePoint)
				{
					m_Destination = pAI->m_PathFinder.SafeGetPathPosition(m_pScenePoint->GetPosition(),4,2);
					if (VALID_POS(m_Destination))
					{	
						return(true);
					}
				}
			}
			CVec3Dfp32 Separation = pAI->Proj2Base(TargetPos) - pAI->Proj2Base(OurPos);
			CVec3Dfp32 SepNorm = Separation;
			SepNorm.Normalize();
			CVec3Dfp32 LeftNorm = SepNorm;
			LeftNorm[0] = -SepNorm[1];
			LeftNorm[1] = SepNorm[0];
			LeftNorm.Normalize();
			// SepNorm and LeftNorm make up our coordinate system
			CVec3Dfp32 BackMove = OurPos - SepNorm * m_MaxRange;
			CVec3Dfp32 LeftMove = OurPos + LeftNorm * m_Distance;
			CVec3Dfp32 RightMove = OurPos - LeftNorm * m_Distance;
			if (pAI->m_bWallClimb)
			{
				m_Destination = BackMove;
				if (VALID_POS(m_Destination))
				{
					return(true);
				}
				{
					return(false);
				}
			}

			BackMove = pAI->m_PathFinder.SafeGetPathPosition(BackMove,5,4);
			LeftMove = pAI->m_PathFinder.SafeGetPathPosition(LeftMove,5,4);
			RightMove = pAI->m_PathFinder.SafeGetPathPosition(RightMove,5,4);

			CVec3Dfp32 StopPos;
			if (!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,BackMove,
				(int)pAI->GetBaseSize(),
				(int)pAI->GetHeight(),
				pAI->m_bWallClimb,
				(int)pAI->GetStepHeight(),
				(int)pAI->GetJumpHeight(),
				&StopPos))
			{	// We couldn't move straight away from the threat
				if ((Relation >= CAI_AgentInfo::ENEMY)&&(m_iBehaviour)&&(m_Flags & FLAG_CORNERED))
				{
					if (SetWantedBehaviour(m_iBehaviour,-1,NULL))
					{
						SpeakRandom(CAI_Device_Sound::AFRAID_PLEAD_FOR_LIFE);
						SetExpirationExpired();
						return(false);
					}
				}
				BackMove = StopPos;

				if (!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,RightMove,
					(int)pAI->GetBaseSize(),
					(int)pAI->GetHeight(),
					pAI->m_bWallClimb,
					(int)pAI->GetStepHeight(),
					(int)pAI->GetJumpHeight(),
					&StopPos))
				{	// We couldn't move right, measure left
					RightMove = StopPos;
				}

				if (!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,LeftMove,
					(int)pAI->GetBaseSize(),
					(int)pAI->GetHeight(),
					pAI->m_bWallClimb,	
					(int)pAI->GetStepHeight(),
					(int)pAI->GetJumpHeight(),
					&StopPos))
				{	// We couldn't move right, measure left
					LeftMove = StopPos;
				}
				fp32 BackMoveSqr = BackMove.DistanceSqr(TargetPos);
				fp32 LeftMoveSqr = LeftMove.DistanceSqr(TargetPos);
				fp32 RightMoveSqr = RightMove.DistanceSqr(TargetPos);
				fp32 SepSqr = Separation.LengthSqr() + Sqr(16.0f);
				INVALIDATE_POS(m_Destination);
				if ((BackMoveSqr > Sqr(m_Distance))||((BackMoveSqr >= RightMoveSqr * 0.75f)&&(BackMoveSqr >= LeftMoveSqr * 0.75f)))
				{	// Long enough
					m_Destination = BackMove;
				}
				else if (RightMoveSqr >= LeftMoveSqr)
				{
					m_Destination = RightMove;
				}
				else
				{
					m_Destination = LeftMove;
				}
				m_Destination = pAI->m_PathFinder.SafeGetPathPosition(m_Destination,5,4);
			}
			else
			{	// BackMove is OK
				m_Destination = BackMove;
			}

			if (pAI->SqrFlatDistance(m_Destination) < Sqr(16.0f))
			{
				if ((_pTarget->GetCurRelation() >= CAI_AgentInfo::ENEMY)&&(pAI->SqrDistanceToUs(_pTarget->GetSuspectedPosition()) < Sqr(m_MaxRange)))
				{
					pAI->m_DeviceStance.Crouch(true);
					pAI->m_DeviceMove.Use();
					pAI->m_DeviceLook.Use();
				}
			}

#ifndef M_RTM
			pAI->Debug_RenderWire(OurPos,LeftMove,kColorRed,3.0f);
			pAI->Debug_RenderWire(OurPos,BackMove,kColorGreen,3.0f);
			pAI->Debug_RenderWire(OurPos,RightMove,kColorBlue,3.0f);
#endif
		}
	}

	if (VALID_POS(m_Destination))
	{
		return(true);
	}
	else
	{
		return(false);
	}
};

bool CAI_Action_KeepDistance::MoveToDestination(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();
	if ((_pTarget)&&(VALID_POS(m_Destination)))
	{
		if (m_pAH->IsRestricted(m_Destination))
		{
			INVALIDATE_POS(m_Destination);
			pAI->ClearPerfectOrigin();
			return(false);
		}

		// Proj2Base will change the value of OurPos only if our up is not "up"
		CVec3Dfp32 OurPos = pAI->GetBasePos();
		OurPos = pAI->Proj2Base(OurPos);
		if (pAI->SqrFlatDistance(m_Destination) < Sqr(16.0f))
		{
			m_Destination = OurPos;
			pAI->m_DeviceMove.Use();
			if (m_pScenePoint)
			{
				pAI->m_DeviceStance.Crouch(true);
				pAI->m_DeviceMove.Use();
				pAI->m_DeviceLook.Use();
				m_pScenePoint = NULL;
			}
			return(true);
		}
		int Relation = _pTarget->GetCurRelation();
		int Awareness = _pTarget->GetCurAwareness();
		int Mode = MODE_WALK;
		if (m_MoveMode == MODE_DEFAULT)
		{
			if (Relation >= CAI_AgentInfo::ENEMY)
			{	// Run
				Mode = MODE_RUN;
			}
			else
			{	// Walk away warily
				Mode = MODE_WALK;
			}
		}
		else if (m_MoveMode == MODE_WALK)
		{
			Mode = MODE_WALK;
		}
		else
		{
			Mode = MODE_RUN;
		}

		bool bResult = true;
		if (Mode == MODE_WALK)
		{	// Walk away
			pAI->SetDestination(m_Destination);
			if (m_pScenePoint)
			{
				int moveResult = pAI->OnMove(m_Destination, pAI->m_ReleaseIdleSpeed,false,false,m_pScenePoint);
				if (!pAI->CheckMoveresult(moveResult,NULL))
				{
					INVALIDATE_POS(m_Destination);
					m_pScenePoint->InvalidateScenepoint(pAI->m_pServer,60);
					m_pScenePoint = NULL;
					return(false);
				}
			}
			else
			{
				bool bResult = pAI->AddMoveTowardsPositionCmd(m_Destination, pAI->m_ReleaseIdleSpeed);
				if (!bResult)
				{
					INVALIDATE_POS(m_Destination);
					pAI->ClearPerfectOrigin();
				}
			}
		}
		else	// MODE_RUN
		{
			pAI->SetDestination(m_Destination);
			if (m_pScenePoint)
			{
				int moveResult = pAI->OnMove(m_Destination, pAI->m_ReleaseMaxSpeed,false,false,m_pScenePoint);
				if (!pAI->CheckMoveresult(moveResult,NULL))
				{
					INVALIDATE_POS(m_Destination);
					m_pScenePoint->InvalidateScenepoint(pAI->m_pServer,60);
					m_pScenePoint = NULL;
					return(false);
				}
			}
			else
			{
				bool bResult = pAI->AddMoveTowardsPositionCmd(m_Destination, pAI->m_ReleaseMaxSpeed);
				if (!bResult)
				{
					INVALIDATE_POS(m_Destination);
					pAI->ClearPerfectOrigin();
				}
			}
		}
	}
	return(false);
};

bool CAI_Action_KeepDistance::StayAtDestination(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();
	if ((_pTarget)&&(!pAI->m_DeviceStance.IsCrouching()))
	{
		OnTrackAgent(_pTarget,10,false);
	}
	return(true);
};

void CAI_Action_KeepDistance::OnStart()
{
	CAI_Action::OnStart();
	// Invalidate the target pos to force the first round of OnTakeAction to set up a new move
	INVALIDATE_POS(m_TargetPos);
	INVALIDATE_POS(m_Destination);
	m_pScenePoint = NULL;
};

void CAI_Action_KeepDistance::OnQuit()
{
	CAI_Core* pAI = AI();

	INVALIDATE_POS(m_TargetPos);
	INVALIDATE_POS(m_Destination);
	m_pScenePoint = NULL;
	if (pAI->m_DeviceStance.IsCrouching())
	{
		pAI->m_DeviceStance.Crouch(false);
	}
	CAI_Action::OnQuit();
};


//CAI_Action_WeaponAim///////////////////////////////////////////////////////////////////////////
CAI_Action_WeaponAim::CAI_Action_WeaponAim(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = WEAPON_AIM;
	m_TypeFlags = TYPE_OFFENSIVE;

	m_iObject = 0;		// Object we track with
	m_iTarget = 0;		// Target we track
	m_Tracktime = 1;	// Time we track
	m_CosAngle = M_Cos(_PI2 * 45 / 360.0f); //Value is assumed to be in degrees here
	m_MinRange = 0.0f;
	m_MaxRange = 512.0f;
};

void CAI_Action_WeaponAim::SetParameter(int _Param, CStr _Str)
{
	MAUTOSTRIP(CAI_Action_WeaponAim_SetParameter, MAUTOSTRIP_VOID);
	if (!AI())
		return;
	
	switch (_Param)
	{
	case PARAM_TRACKTIME:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;
	case PARAM_ANGLE:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;
	case PARAM_MINRANGE:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;
	case PARAM_MAXRANGE:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;

	case PARAM_OBJECT:
		{
			CWObject* pObj = AI()->GetSingleTarget(0,_Str,AI()->m_pGameObject->m_iObject);
			if (pObj) 
			{
				m_iObject = pObj->m_iObject;
			}
		}
		break;

	default:
		CAI_Action::SetParameter(_Param, _Str);
	}
}

//Set params
void CAI_Action_WeaponAim::SetParameter(int _Param, int _Val)
{
	MAUTOSTRIP(CAI_Action_WeaponAim_SetParameter_2, MAUTOSTRIP_VOID);

	if (_Param == PARAM_OBJECT)
	{
		m_iObject = _Val;
	}
	else
	{
		CAI_Action::SetParameter(_Param, _Val);
	}
};

void CAI_Action_WeaponAim::SetParameter(int _Param, fp32 _Val)
{
	MAUTOSTRIP(CAI_Action_WeaponAim_SetParameter_3, MAUTOSTRIP_VOID);
	switch (_Param)
	{
	case PARAM_ANGLE:
		_Val = Max(0.0f, Min(180.0f, _Val));
		m_CosAngle = M_Cos(_Val * (_PI2 / 360.0f)); //Value is assumed to be in degrees here
		break;
	case PARAM_TRACKTIME:
		if (_Val <= 0.0f)
		{
			m_Tracktime = 1;
		}
		else
		{
			m_Tracktime = (int)(_Val * AI()->GetAITicksPerSecond());
		}
		break;
	case PARAM_MINRANGE:
		if (_Val >= 0.0f)
		{
			m_MinRange = _Val;
		}
		else
		{
			m_MinRange = 0.0f;
		}
		break;
	case PARAM_MAXRANGE:
		if (_Val > m_MinRange)
		{
			m_MaxRange = _Val;
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get parameter ID from string (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_WeaponAim::StrToParam(CStr _Str, int* _pResType)
{
	MAUTOSTRIP(CAI_Action_WeaponAim_StrToParam, 0);

	if (_Str == "TRACK")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_TRACKTIME;
	}
	else if (_Str == "ANGLE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_ANGLE;
	}
	else if (_Str == "MINRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_MINRANGE;
	}
	else if (_Str == "MAXRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_MAXRANGE;
	}
	else if (_Str == "OBJECT")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_TARGET;
		return PARAM_OBJECT;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

//Mandatory: Weapon, Look
int CAI_Action_WeaponAim::MandatoryDevices()
{
	return 0;
};

//Valid if weapon is available and we've got a good target
bool CAI_Action_WeaponAim::IsValid()
{
	MAUTOSTRIP(CAI_Action_WeaponAim_IsValid, false);
	if (!CAI_Action::IsValid())
		return false;

	CAI_Core* pAI = AI();

	if (!m_iObject)
	{
		return(false);
	}
	CWObject* pObj = pAI->m_pServer->Object_Get(m_iObject);
	if (!pObj)
	{
		m_iObject = 0;
		return(false);
	}

	if (m_iTarget)
	{
		CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
		if (!pTarget)
		{
			m_iTarget = 0;
			return(false);
		}
		CVec3Dfp32 TargetPos = pTarget->GetSuspectedPosition();
		CVec3Dfp32 BasePos = pObj->GetPosition();
		if ((BasePos.DistanceSqr(TargetPos) > m_MaxRange)||(BasePos.DistanceSqr(TargetPos) < m_MinRange))
		{
			return(false);
		}

		// Measure angle
		if ((m_CosAngle > -1.0f))
		{	
			if (pAI->CosHeadingDiff(pAI->m_pGameObject, pTarget->GetObject()) < m_CosAngle)
			{
				return(false);
			}
		}
		return(true);
	}
	else
	{
		return(true);
	}
}

// Offensive estimate goes to 0 when aim is equal or better than PARAM_MAXAIM
// Offensive goes to 50 when aim is 0
// When aim >= PARAM_MAXAIM: Offensive estimate is 0
// When PARAM_MAXAIM > aim >= PARAM_MAXAIM/2: Offensive estimate is 50
// When aim < PARAM_MAXAIM/2: Offensive estimate is 75
CAI_ActionEstimate CAI_Action_WeaponAim::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP(CAI_Action_WeaponAim_GetEstimate, Est);
	if (!CAI_Action::IsValid())
		return Est;

	// We must have a valid hostile target
	CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
	if (pTarget == NULL)
	{
		return Est;
	}
	
	if (pTarget->GetCurAwareness() < CAI_AgentInfo::SPOTTED)
	{
		return Est;
	}

	Est.Set(CAI_ActionEstimate::OFFENSIVE,75);
	Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
	Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
	Est.Set(CAI_ActionEstimate::LOYALTY, 0);
	Est.Set(CAI_ActionEstimate::LAZINESS, 0);
	Est.Set(CAI_ActionEstimate::STEALTH, 0);
	Est.Set(CAI_ActionEstimate::VARIETY, 0);

	return Est;
}


//Try to improve aim
void CAI_Action_WeaponAim::OnTakeAction()
{
	MAUTOSTRIP(CAI_Action_WeaponAim_OnTakeAction, MAUTOSTRIP_VOID);
	MSCOPESHORT(CAI_Action_WeaponAim::OnTakeAction);

	CAI_Core* pAI = AI();

	// We must have a valid hostile target
	CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
	if (pTarget == NULL)
	{
		return;
	}
	m_iTarget = pTarget->GetObjectID();
	CAI_Weapon* pWeapon = pAI->m_Weapon.GetWielded();
	if (pWeapon == NULL)
	{
		return;
	}

	pAI->ActivateAnimgraph();
	// Aim and track
	if (m_Tracktime <= 0)
	{
		pAI->AddAimAtPositionCmd(pAI->GetFocusPosition(pTarget),pAI->m_Reaction * 2);
		SetExpiration(AI()->m_Reaction * 2);
	}
	else
	{
		pAI->AddAimAtPositionCmd(pAI->GetFocusPosition(pTarget),m_Tracktime);
		SetExpiration(m_Tracktime);
	}
}

//CAI_Action_WeaponAttackRanged//////////////////////////////////////////////////////////////////
const char* CAI_Action_WeaponAttackRanged::ms_lStrFlags[] = 
{
	"STOP",
	"RELOAD",
	"FRIENDLY",
	"DARKLING",

	NULL,
};

CAI_Action_WeaponAttackRanged::CAI_Action_WeaponAttackRanged(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = WEAPON_ATTACK_RANGED;
	m_TypeFlags = TYPE_SOUND | TYPE_OFFENSIVE | TYPE_ATTACK;
	m_Flags = 0;
	m_CosAngle = -1.0f;	// -1 signifies 360 degrees fire
	m_iTarget = 0;
	m_Tracktime = 6;
	m_iNearbyFriendly = 0;
	m_liGestures.Clear();
};

void CAI_Action_WeaponAttackRanged::SetParameter(int _Param, CStr _Str)
{
	MAUTOSTRIP(CAI_Action_WeaponAttackRanged_SetParameter, MAUTOSTRIP_VOID);
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_TRACKTIME:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;
	case PARAM_ANGLE:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
	case PARAM_GESTURES:
		{	// Extract the gesture nbrs
			m_liGestures.Clear();
			int iGesture= -1;
			CStr Temp = _Str;
			do
			{
				if ((iGesture = Temp.GetIntMSep(",+")))
				{
					m_liGestures.Add(iGesture);
				}
			} while (iGesture > 0);
		}
		break;
	case PARAM_FLAGS:
		SetParameter(_Param, StrToFlags(_Str));
		break;

	default:
		CAI_Action::SetParameter(_Param, _Str);
	}
}

//Set params
void CAI_Action_WeaponAttackRanged::SetParameter(int _Param, int _Val)
{
	MAUTOSTRIP(CAI_Action_WeaponAttackRanged_SetParameter_2, MAUTOSTRIP_VOID);
	
	if (_Param == PARAM_FLAGS)
	{
		m_Flags = _Val;
	}
	else
	{
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_WeaponAttackRanged::SetParameter(int _Param, fp32 _Val)
{
	MAUTOSTRIP(CAI_Action_WeaponAttackRanged_SetParameter_3, MAUTOSTRIP_VOID);
	switch (_Param)
	{
	case PARAM_ANGLE:
		_Val = Max(0.0f, Min(180.0f, _Val));
		m_CosAngle = M_Cos(_Val * (_PI2 / 360.0f)); //Value is assumed to be in degrees here
		break;
	case PARAM_TRACKTIME:
		if (_Val <= 0.0f)
		{
			m_Tracktime = -1;
		}
		else
		{
			m_Tracktime = (int)(_Val * AI()->GetAITicksPerSecond());
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get parameter ID from string (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_WeaponAttackRanged::StrToParam(CStr _Str, int* _pResType)
{
	MAUTOSTRIP(CAI_Action_WeaponAttackRanged_StrToParam, 0);

	if (_Str == "TRACK")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_TRACKTIME;
	}
	else if (_Str == "ANGLE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_ANGLE;
	}
	else if (_Str == "GESTURES")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_GESTURES;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

int CAI_Action_WeaponAttackRanged::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

int CAI_Action_WeaponAttackRanged::GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const
{
	int nAdded = 0;
	for (int i = 0; i < m_liGestures.Len(); i++)
	{
		nAdded += AddUnique(_liMoves,m_liGestures[i]);
	}

	return(nAdded);
};

bool CAI_Action_WeaponAttackRanged::IsValid()
{
	MAUTOSTRIP(CAI_Action_WeaponAttackRanged_IsValid, false);
	
	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		m_iTarget = 0;
		return(false);
	}

	if (pAI->m_iMountParam)
	{	// Cannot use firearms while mounted
		return(false);
	}

	if (pAI->m_Weapon.GetWieldedArmsClass() < CAI_WeaponHandler::AI_ARMSCLASS_GUN)
	{	// Save on CPU
		ExpireWithDelay(200);
		return false;
	}

	if (m_iTarget)
	{
		CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
		if (!pTarget)
		{
			m_iTarget = 0;
			return(false);
		}

		int32 Awareness = pTarget->GetCurAwareness();
		if (Awareness <= CAI_AgentInfo::NOTICED)
		{
			m_iTarget = 0;
			return(false);
		}
		if ((Awareness == CAI_AgentInfo::DETECTED)&&(pAI->m_Timer >= pTarget->GetCurAwarenessTimer() + pAI->GetAITicksPerSecond()))
		{	// Target Good, visibility bad, stop firing after 1 second
			return(false);
		}
	}

	// Cannot shoot while climbing walls
	if (m_Flags & FLAGS_DARKLING)
	{
		if (pAI->GetUp() * CVec3Dfp32(0.0f,0.0f,1.0f) < 0.99f)
		{
			return(false);
		}
	}

	// We cannot fire weapons when combat behaviours are active
	if ((pAI->m_iBehaviourRunning)&&(pAI->m_CurCB.m_iBehaviour != 0))
	{
		return(false);
	}

	return(true);
};

//Mandatory: Weapon, Look
int CAI_Action_WeaponAttackRanged::MandatoryDevices()
{	// Yes, we stop to fire but we do not REQUIRE a movedvice to fire
	return (1 << CAI_Device::WEAPON);
};

//Estimate improves with estimated chance of hitting, as well as power of weapon
CAI_ActionEstimate CAI_Action_WeaponAttackRanged::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP(CAI_Action_WeaponAttackRanged_GetEstimate, Est);
	CAI_Core* pAI = AI();

	// NOTE: We should NOT call CAI_Action_WeaponAim::IsValid()
	if (!CAI_Action::IsValid())
		return Est;

	CAI_Weapon* pWeapon = pAI->m_Weapon.GetWielded();
	if (pWeapon == NULL)
	{
		return Est;
	}

	int ArmsClass = pAI->m_Weapon.GetWieldedArmsClass();
	if (ArmsClass < CAI_WeaponHandler::AI_ARMSCLASS_GUN)
	{	// No ranged weapon
		return Est;
	}


	// We must have a valid hostile target
	CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
	if (m_Flags & FLAGS_RELOAD)
	{
		if ((!pTarget)||(pTarget->GetCurAwareness() < CAI_AgentInfo::SPOTTED))
		{
			pAI->Reload(0.5f);
		}
		else
		{
			pAI->Reload(1.0f);
		}
	}

	if ((!pTarget)||(pTarget->GetCurAwareness() < CAI_AgentInfo::SPOTTED))
	{
		return Est;
	}
	m_iTarget = pTarget->GetObjectID();
	fp32 RangeSqr = pAI->SqrDistanceToUs(pTarget->GetObjectID());
	if (((RangeSqr > Sqr(pAI->m_RangedMaxRange)))||(RangeSqr < Sqr(pAI->m_RangedMinRange)))
	{
		if (pAI->m_KB.GetDetectedEnemyCount() > 1)
		{
			m_pAH->SuggestTargetChange();
		}
		m_iTarget = 0;
		return Est;
	}

	//Default score 50
	if (m_Tracktime <= 1)
	{
		m_Tracktime = 1;
	}

	Est.Set(CAI_ActionEstimate::OFFENSIVE, 75); 
	Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
	Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
	Est.Set(CAI_ActionEstimate::LOYALTY, 0);
	Est.Set(CAI_ActionEstimate::LAZINESS, 0);
	Est.Set(CAI_ActionEstimate::STEALTH, 0);
	Est.Set(CAI_ActionEstimate::VARIETY, 0);

	return Est;
}


//Fire once
void CAI_Action_WeaponAttackRanged::OnTakeAction()
{
	MAUTOSTRIP(CAI_Action_WeaponAttackRanged_OnTakeAction, MAUTOSTRIP_VOID);
	MSCOPESHORT(CAI_Action_WeaponAttackRanged::OnTakeAction);

	CAI_Core* pAI = AI();

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (!pTarget)
	{
		SetExpirationExpired();
		return;
	}

	CAI_Core* pTargetAI = pAI->GetAI(m_iTarget);
	if ((!pTargetAI)||(!pTargetAI->IsConscious()))
	{	// Target unconscious or lacks AI
		m_pAH->SuggestTargetChange();
		ExpireWithDelay((int)(3.0f * pAI->GetAITicksPerSecond()));
	}

	// If we are running, we expire
	// If we are walking we may fire but should not stop to do so.
	if (pAI->GetPrevSpeed() >= pAI->m_ReleaseMaxSpeed * 0.75f)
	{
		SetExpiration((int)(pAI->m_Patience * 0.25f));
		return;
	}
	// If FLAGS_STOP is set we should stop to fire, but only when running
	if ((m_Flags & FLAGS_STOP)&&(pAI->m_DeviceMove.IsAvailable()))
	{
		pAI->m_DeviceMove.Use();
		pAI->ResetStuckInfo();
	}

	pAI->ActivateAnimgraph();
	if (m_Flags & FLAGS_DARKLING)
	{
		if (pAI->m_DeviceStance.GetIdleStance(true) < CAI_Device_Stance::IDLESTANCE_COMBAT)
		{
			pAI->m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		}
	}
	else
	{
		if (pAI->m_DeviceStance.GetIdleStance() < CAI_Device_Stance::IDLESTANCE_COMBAT)
		{
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		}
	}

	if (m_Flags & FLAGS_RELOAD)
	{
		bool bReload = false;
		if (pTarget->GetCurAwareness() < CAI_AgentInfo::SPOTTED)
		{
			bReload = pAI->Reload(0.5f);
		}
		else
		{
			bReload = pAI->Reload(1.0f);
		}

		if (bReload)	// Questionable, why expire?
		{
			SetExpirationIndefinite();
			return;
		}
	}

	// ***
	if (pAI->DebugTarget())
	{
		bool wtf = true;
	}
	// ***
	CAI_Weapon* pWeapon = pAI->m_Weapon.GetWielded();
	if (!pWeapon)
	{
		SetExpirationExpired();
		return;
	}

	bool bRapidFire = false;
	if (pWeapon->GetType() == CAI_Weapon::RANGED_RAPIDFIRE)
	{
		bRapidFire = true;
	}
	int32 iTrackObj = m_iTarget;
	CVec3Dfp32 OurLookDir = pAI->GetLookDir();
	CVec3Dfp32 WantedLookDir;
	fp32 lookDot = 1.0f;
	if ((m_Flags & FLAGS_DARKLING)&&(bRapidFire))
	{	// Flail around wildly
		CVec3Dfp32 TargetPos = pTarget->GetCorrectPosition();
		if (pAI->SqrDistanceToUs(TargetPos) < Sqr(48.0f))
		{
			SetExpirationExpired();
			return;
		}
		
		int32 timerMod = pAI->m_Timer % m_Tracktime;
		int32 timerDiv = pAI->m_Timer  / m_Tracktime;
		fp32 t = fp32(timerMod) / fp32(m_Tracktime);
		fp32 Angle;
		if (m_CosAngle > -1.0f)
		{	// Sweep
			fp32 ArcAngle = M_ACos(m_CosAngle) / _PI2;
			if (timerDiv % 2 == 0)
			{	// Even
				Angle = -ArcAngle + 2.0f * ArcAngle * t;
			}
			else
			{	// Odd
				Angle = ArcAngle - 2.0f * ArcAngle * t;
			}
		}
		else
		{	// Rotate
			Angle = t;
		}
		fp32 suspectedHeight = pTarget->GetSuspectedHeight();
		TargetPos[2] += suspectedHeight * (0.5f+Random);
		CVec3Dfp32 AimPos = pAI->GetAimPosition();
		WantedLookDir = TargetPos-AimPos;
		if (pAI->DebugTarget())
		{
			pAI->Debug_RenderWire(AimPos,TargetPos,kColorYellow,1.0f);
		}

		// Expensive?
		CMat4Dfp32 Mat;
		pAI->GetBaseMat(Mat);
		Mat.GetRow(0) = TargetPos-AimPos;
		Mat.GetRow(2) = pAI->GetUp();
		Mat.RecreateMatrix(0,2);
		Mat.RotZ_x_M(Angle);
		WantedLookDir = Mat.GetRow(0);
		lookDot = OurLookDir * WantedLookDir;
		pAI->OnTrackDir(WantedLookDir,iTrackObj,10,false,false);
	}
	else
	{
		WantedLookDir = pAI->OnWeaponTrackHead(pTarget,m_Tracktime,48.0f,false);
		if (m_iTarget == pAI->m_KB.GetPlayerDarkness())
		{
			pAI->m_KB.GetCreepingDark(&iTrackObj);
		}
		lookDot = OurLookDir * WantedLookDir;
		if (lookDot >= m_CosAngle)
		{
			// pAI->OnTrackDir(WantedLookDir,iTrackObj,m_Tracktime,false,false);
			pAI->OnTrackDir(WantedLookDir,iTrackObj,-1,false,false);
		}
		else
		{	// Outside firing cone
			// We'll track if COMBAT isn't running
			if (!m_pAH->HasAction(COMBAT,true))
			{
				// pAI->OnTrackDir(WantedLookDir,iTrackObj,10,false,false);
				pAI->OnTrackDir(WantedLookDir,iTrackObj,-1,false,false);
			}
		}
	}

	bool bClearLOS = CheckFriendlyFire();
	if ((bClearLOS)&&((m_CosAngle <= -1.0f)||(lookDot >= m_CosAngle)))
	{
		if ((pAI->m_CharacterClass != CAI_Core::CLASS_DARKLING)||(pAI->m_DeviceStance.GetIdleStance() >= CAI_Device_Stance::IDLESTANCE_COMBAT))
		{
			pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_AIMGUN,60);
			// pWeapon->OnUse(pTarget);
			if (bRapidFire)
			{
				pAI->m_DeviceWeapon.Use();
			}
			else
			{
				pAI->m_DeviceWeapon.UsePeriodic();
			}
			pAI->m_DeviceStance.SetTargetInFOV(true);
		}
	}
	else
	{
		// Speak only when NOT firing
		SpeakRandom(CAI_Device_Sound::COMBAT_SPOTTED,3.0f);
	}

	// Keep firing for quarter patience
	if (pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)
	{
		SetExpirationIndefinite();
	}
	else
	{
		SetExpiration((int)(pAI->m_Patience * 0.25f));
	}
}

// Returns true if it is OK to fire
bool CAI_Action_WeaponAttackRanged::CheckFriendlyFire()
{
	CAI_Core* pAI = AI();

	if (m_Flags & FLAGS_FRIENDLYFIRE)
	{
		if ((!m_iNearbyFriendly)||(pAI->m_Timer % 10 == 0))
		{	// Try to find a new nearby teammate
			fp32 RangeSquare = 0.0f;
			m_iNearbyFriendly = pAI->GetClosestTeammember(false,m_iTarget,&RangeSquare);
		}
		if ((m_iNearbyFriendly)&&(pAI->SqrDistanceToUs(m_iNearbyFriendly) < Sqr(256.0f)))
		{	
			CVec3Dfp32 HisHead = pAI->GetHeadPos(m_iNearbyFriendly);
			if (VALID_POS(HisHead))
			{
				CVec3Dfp32 Dir = (HisHead - pAI->GetHeadPos()).Normalize();
				CVec3Dfp32 Look = pAI->GetLookDir();
				if ((Look * Dir >= Max(m_CosAngle,0.87f)))
				{
					return(false);
				}
			}
		}
	}


	return(true);
};

void CAI_Action_WeaponAttackRanged::OnStart()
{
	CAI_Core* pAI = AI();

	CAI_Action::OnStart();
	if ((pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)&&(pAI->GetUp() * CVec3Dfp32(0.0f,0.0f,1.0f) >= 0.98f))
	{
		pAI->m_DeviceStance.SetTargetInFOV(true);
		pAI->m_DeviceStance.Free();
		pAI->m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		pAI->m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
	}
};

void CAI_Action_WeaponAttackRanged::OnQuit()
{
	CAI_Core* pAI = AI();

	if (pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)
	{	// Switch class so we can set Stance
		pAI->m_DeviceStance.Free();
		pAI->m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE);
		pAI->m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE);
	}
	CAI_Action::OnQuit();
};


//CAI_Action_MeleeFight//////////////////////////////////////////////////////////////////
// Combination action with both moving around the target and punches and kicks
const char* CAI_Action_MeleeFight::ms_lStrFlags[] = 
{
		"FWD",
		"BWD",
		"LEFT",
		"RIGHT",
		"ATTACK",
		"SPECIAL",
		"BLOCK",
		"PATHFIND",
		"TRACE",
		"GENTLEMAN",
		"PLAYERONLY",
		"HOSTILE",

		NULL,
};

CAI_Action_MeleeFight::CAI_Action_MeleeFight(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = MELEE_FIGHT;
	m_TypeFlags = TYPE_SOUND | TYPE_OFFENSIVE | TYPE_ATTACK | TYPE_DEFENSIVE | TYPE_MOVE;

	m_iOpponent = 0;

	m_StartPos = CVec3Dfp32(_FP32_MAX);
	m_MeleeRestrictSqr = -1.0f;

	m_CosAngle = M_Cos(_PI2 * (30.0f / 360.0f)); // +-30 degrees and we may punch/kick
	m_AttackPropability = 0.5f;
	m_ManeuverInterval = RoundToInt((0.5f * CAI_Core::AI_TICKS_PER_SECOND));		// By default once a second
	m_LastManeuverTick = 0;
	m_MovePropability = 1.0f;		// Propability of movement
	m_LastManeuver = 0;
	m_LastOpponentManeuver = 0;
	m_bFirstBlow = true;

	m_Flags = FLAGS_ATTACK | FLAGS_BLOCK | FLAGS_FWD | FLAGS_BWD | FLAGS_LEFT | FLAGS_RIGHT;
};

int CAI_Action_MeleeFight::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int nAdded = 0;
	
	/*
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_NBR_TAUNT_0);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_NBR_TAUNT_1);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_NBR_TAUNT_2);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_NBR_SCARE);
	*/

	return(nAdded);
};

void CAI_Action_MeleeFight::SetParameter(int _Param, CStr _Str)
{
	MAUTOSTRIP(CAI_Action_MeleeFight_SetParameter, MAUTOSTRIP_VOID);
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_RESTRICTRANGE:
	case PARAM_ANGLE:
	case PARAM_ATTACKPROP:
	case PARAM_MOVEPROP:
	case PARAM_INTERVAL:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;
	case PARAM_FLAGS:
		SetParameter(_Param, StrToFlags(_Str));
		break;

	default:
		CAI_Action::SetParameter(_Param, _Str);
	}
}


void CAI_Action_MeleeFight::SetParameter(int _Param, fp32 _Val)
{
	MAUTOSTRIP(CAI_Action_MeleeFight_SetParameter_2, MAUTOSTRIP_VOID);
	switch (_Param)
	{
	case PARAM_RESTRICTRANGE:
		m_MeleeRestrictSqr = Sqr(_Val);
		break;

	case PARAM_ANGLE:
		//Cos of max deviation allowed for an attack, otherwise we turn/strafe
		_Val = Max(0.0f, Min(180.0f, _Val));
		m_CosAngle = M_Cos(_Val * (_PI2 / 360.0f)); //Value is assumed to be in degrees here			
		break;

	case PARAM_ATTACKPROP:
		_Val = Max(0.0f, Min(1.0f, _Val));
		m_AttackPropability = _Val;					//Propability of attack within sweatspot,
		break;

	case PARAM_MOVEPROP:
		_Val = Max(0.0f, Min(1.0f, _Val));
		m_MovePropability = _Val;					//Propability of move
		break;

	case PARAM_INTERVAL:
		m_ManeuverInterval = (int32)(_Val * AI()->GetAITicksPerSecond());
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
};

void CAI_Action_MeleeFight::SetParameter(int _Param, int _Val)
{
	MAUTOSTRIP(CAI_Action_MeleeFight_SetParameter_3, MAUTOSTRIP_VOID);
	switch (_Param)
	{
	case PARAM_FLAGS:
		m_Flags = _Val;
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	};
};

//Get flags for given string
int CAI_Action_MeleeFight::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

int CAI_Action_MeleeFight::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "ATTACKANGLE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_ANGLE;
	}
	else if (_Str == "ATTACKPROP")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_ATTACKPROP;
	}
	else if (_Str == "MOVEPROP")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_MOVEPROP;
	}
	else if (_Str == "INTERVAL")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_INTERVAL;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

//Mandatory: Weapon, Move
int CAI_Action_MeleeFight::MandatoryDevices()
{
	return (1 << CAI_Device::MELEE)/* | (1 << CAI_Device::MOVE)*/ | 
		   (1 << CAI_Device::WEAPON) | (1 << CAI_Device::ITEM);
}

bool CAI_Action_MeleeFight::IsValid()
{
	MAUTOSTRIP(CAI_Action_MeleeFight_IsValid, false);
	if (!CAI_Action::IsValid())
	{
		return(false);
	}
	CAI_Core* pAI = AI();

	// Before anything else we check if we're beyond m_MeleeRestrictSqr
	if ((m_MeleeRestrictSqr >= 0.0f)&&(VALID_POS(m_StartPos))&&(pAI->SqrDistanceToUs(m_StartPos) > m_MeleeRestrictSqr))
	{	// We don't set the m_bRetreating here - it will be set by OnTakeAction if relation etc is OK
		return(true);
	}

	if (m_iOpponent)
	{
		if ((m_Flags & FLAGS_PLAYERONLY)&&(!pAI->IsPlayer(m_iOpponent)))
		{
			return false;
		}
		CAI_AgentInfo* pTarget = AI()->m_KB.GetAgentInfo(m_iOpponent);

		if (!pTarget)
		{
			return(false);
		}

		if (pTarget->GetCurAwareness() < CAI_AgentInfo::DETECTED)
		{
			return(false);
		}

		if (pAI->SqrDistanceToUs(pTarget->GetObjectID()) > Sqr(pAI->m_MeleeMaxRange))
		{
			return false;
		}

		// Has relation changed?
		if ((m_Flags & FLAGS_HOSTILE)||(pAI->m_bHostileFromBlowDmg))
		{
			if (pTarget->GetCurRelation() < CAI_AgentInfo::HOSTILE)
			{
				return(false);
			}
		}
		else
		{
			if (pTarget->GetCurRelation() < CAI_AgentInfo::ENEMY)
			{
				return(false);
			}
		}
	}

	return(true);
}

//Estimate improves with estimated chance of hitting, as well as power of weapon
CAI_ActionEstimate CAI_Action_MeleeFight::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP(CAI_Action_MeleeFight_GetEstimate, Est);
	if (!IsValid())
		return Est;
	CAI_Core* pAI = AI();

	// We must have a valid hostile target
	m_iOpponent = 0;
	CAI_AgentInfo* pTarget = NULL;

	if (m_Flags & FLAGS_PLAYERONLY)
	{
		int iPlayer = pAI->GetClosestPlayer();
		pTarget = pAI->m_KB.GetAgentInfo(iPlayer);
		if (pTarget)
		{
			if (m_Flags & FLAGS_HOSTILE)
			{
				if (pTarget->GetCurRelation() < CAI_AgentInfo::HOSTILE)
				{
					return Est;
				}
			}
			else
			{
				if (pTarget->GetCurRelation() < CAI_AgentInfo::ENEMY)
				{
					return Est;
				}
			}
		}
	}
	else	// NOT player only
	{
		if ((m_Flags & FLAGS_HOSTILE)||(pAI->m_bHostileFromBlowDmg))
		{
			pTarget = m_pAH->FindRelation(false,CAI_AgentInfo::HOSTILE,CAI_AgentInfo::ENEMY);
		}
		if (!pTarget)
		{
			int orgBravery = pAI->m_Bravery;
			pAI->m_Bravery = CAI_Core::BRAVERY_ALWAYS;
			pTarget = m_pAH->AcquireTarget();
			pAI->m_Bravery = orgBravery;
		}
	}

	CAI_Weapon* pWeapon = pAI->m_Weapon.GetWielded();
	if (pWeapon == NULL)
	{
		return Est;
	}

	if ((pTarget)&&(pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)&&(pAI->SqrDistanceToUs(pTarget->GetObjectID()) < Sqr(pAI->m_MeleeMaxRange)))
	{
		m_iOpponent = pTarget->GetObjectID();
	}
	else
	{
		return Est;
	}

	//Default score 50
	Est.Set(CAI_ActionEstimate::OFFENSIVE, 75); 
	Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
	Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
	Est.Set(CAI_ActionEstimate::LOYALTY, 0);
	Est.Set(CAI_ActionEstimate::LAZINESS, 0);
	Est.Set(CAI_ActionEstimate::STEALTH, 0);
	Est.Set(CAI_ActionEstimate::VARIETY, 0);

	return Est;
}

void CAI_Action_MeleeFight::OnTakeAction()
{
	MAUTOSTRIP(CAI_Action_MeleeFight__OnTakeAction, MAUTOSTRIP_VOID);
	MSCOPESHORT(CAI_Action_MeleeFight::OnTakeAction);
	CAI_Core* pAI = AI();
	if ((!pAI)||(!pAI->m_pGameObject))
	{
		SetExpirationExpired();
		return;
	}

	// Reset stuck
	pAI->ResetStuckInfo();
	CVec3Dfp32 OurPos = pAI->GetBasePos();
	if (m_Flags & FLAGS_PATHFIND)
	{
		if ((pAI->m_PathFinder.IsValid())&&(pAI->m_PathFinder.GridPF()))
		{
			OurPos = pAI->m_PathFinder.GetPathPosition(OurPos,5,0);
		}
		else
		{
			SetExpirationExpired();
			return;
		}
	}

	// Get the target if any and its range/relation if any and its ai if any 
	// (if these fails the values should have reasonable defaults)
	CAI_AgentInfo* pTarget = NULL;
	int TargetRel = CAI_AgentInfo::NEUTRAL;
	fp32 RestrictDistSqr = -1.0f;
	CAI_Core* pTargetAI = NULL;
	
	pTarget = pAI->m_KB.GetAgentInfo(m_iOpponent);
	pTargetAI = pAI->GetAI(m_iOpponent);
	if ((!pTarget)||(!pTargetAI)||(pTarget->GetCurAwareness() < CAI_AgentInfo::DETECTED))
	{
		SetExpirationExpired();
		return;
	}

	TargetRel = pTarget->GetCurRelation();
	fp32 TargetDistSqr = pAI->SqrDistanceToUs(m_iOpponent);

	if ((TargetRel < CAI_AgentInfo::ENEMY)&&(m_MeleeRestrictSqr >= 0.0f)&&(VALID_POS(m_StartPos)))
	{
		RestrictDistSqr = pAI->SqrDistanceToUs(m_StartPos);
		if ((RestrictDistSqr >= m_MeleeRestrictSqr)||(pTarget->GetCurAwareness() < CAI_AgentInfo::SPOTTED))
		{
			m_bRetreating = true;
		}
	}

	// Should we expire or retreat
	if (ExpiresNextTick())
	{
		if (TargetDistSqr <= Sqr(pAI->m_MeleeMaxRange))
		{	// Prolong fight
			SetExpirationExpired();
			SetExpiration(pAI->m_Patience);
		}
		else if (TargetRel < CAI_AgentInfo::ENEMY)
		{
			int HitsToRestore = (pAI->MaxHealth() - pAI->Health()) / 2;
			pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH,HitsToRestore),pAI->m_pGameObject->m_iObject);
		}
	}

	// We gotta track the target here, so the move commands don't fuck up our look
	CVec3Dfp32 TargetPos = pTargetAI->GetBasePos();
	TargetPos += pTargetAI->GetUp() * 0.9f * pTargetAI->GetCurHeight();
	pAI->OnTrackDir(TargetPos - pAI->GetLookPos(),m_iOpponent,6,false,false);
	// NPC fighting NPC will not keep the relation up
	if (pAI->IsPlayer(m_iOpponent))
	{
		if (TargetRel < CAI_AgentInfo::HOSTILE_1)
		{	// Keep relation to at least HOSTILE_1
			pTarget->SetRelation(CAI_AgentInfo::HOSTILE_1);
		}
	}

	// Go back to m_StartPos, with perfect placement goddamit!
	if ((m_bRetreating)&&(VALID_POS(m_StartPos)))
	{
		if (RestrictDistSqr >= Sqr(16))
		{
			SetExpirationExpired();
			return;
		}

		//Move there
		pAI->OnMove(m_StartPos,pAI->m_ReleaseIdleSpeed,false,false,NULL);

#ifndef M_RTM
		pAI->Debug_RenderWire(OurPos,m_StartPos,kColorYellow,0.05f);
#endif
	}

	// Get the client data
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
	if (!pCD)
	{
		return;
	}

	if (!pTarget)
	{
		return;
	}

	// All systems are GO!
	pAI->ActivateAnimgraph();
	SetExpiration((int)(pAI->m_Patience * (1.0f + Random)));
	pAI->m_DeviceStance.SetTargetInFOV(true);
	pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
	pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);

	// Collect some extra data useful when determining what blows to try, what to say, what tapdancing to do etc
	bool bHostile = true;	// When !bHostile we cannot strike but we can still block and dance (but not fwd)
	bool bIsHurt = pAI->IsHurt(0.5f);
	bool bGun = false;		// When bGun we are not using a proper melee weapon
	bool bFacingOK = true;	// When !FacingOK we should not strike
	bool bTargetHurtRecently = false;	// When true we feel like winning
	bool bSelfHurtRecently = false;		// When true we feel like losing
//	bool bOpponentBlocking = false;		// When true we shouldn't block
	if ((m_Flags & FLAGS_GENTLEMAN)&&(pAI->m_pAIResources->ms_GameDifficulty < DIFFICULTYLEVEL_HARD))
	{
		int Prio = 3;
		if ((bIsHurt)&&(pAI->m_pAIResources->ms_GameDifficulty >= DIFFICULTYLEVEL_NORMAL))
		{
			Prio += -2;
		}
		if (TargetDistSqr <= Sqr(55))
		{
			Prio += 1;
		}
		if (m_Flags & FLAGS_ATTACK_SPECIAL)
		{
			Prio += 1;
		}
		if (!pAI->m_pAIResources->m_ActionGlobals.ms_MeleeFightSemaphore.Poll(pAI->m_pGameObject->m_iObject,Prio,pAI->GetGlobalAITick(),1))
		{
			bHostile = false;
		}
	}
	
	if (pAI->m_Weapon.GetWieldedArmsClass() >= CAI_WeaponHandler::AI_ARMSCLASS_GUN)
	{
		bGun = true;
		pAI->m_DeviceWeapon.Lock();
	}

	if (pAI->CosHeadingDiff(AI()->m_pGameObject,pTarget->GetObject()) < m_CosAngle)
	{	// We must turn
		bFacingOK = false;
	}
	int GameTick = pAI->GetAITick();
	if (pTargetAI->m_KB.GetLastHurtTicks() <= pAI->GetAITicksPerSecond() * 2)
	{
		bTargetHurtRecently = true;
	}
	if (pAI->m_KB.GetLastHurtTicks() <= pAI->GetAITicksPerSecond() * 2)
	{
		bSelfHurtRecently = true;
	}

	pAI->ResetStuckInfo();
	// OK we now have enough info to decide what to do
	if (GameTick >= m_LastManeuverTick + m_ManeuverInterval)
	{
		int32 CurManeuver = 0;
		if (bFacingOK)
		{
			fp32 Range = sqrt(TargetDistSqr);
			fp32 IdealRange = pAI->m_MeleeMaxRange * 0.5f;
			if ((m_Flags & FLAGS_ATTACK)&&((Range <= IdealRange)||(Random <= m_AttackPropability)))
			{
				// Decide which attack to use
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
				int attackType = 0;
				// Kick?
				CVec3Dfp32 OurHead = pAI->GetHeadPos();
				CVec3Dfp32 HisHead = pTargetAI->GetHeadPos();
				if (OurHead[2] - HisHead[2] >= 10.0f)
				{	// A kick in da head!
					attackType = 2;
				}
				else
				{
					if (bGun)
					{
						attackType = TruncToInt(Random * 1.999f);
					}
					else
					{	// Knife?
						if (pAI->m_Skill >= Random * 100)
						{
							attackType = TruncToInt(Random * 7.999f);
						}
						else
						{
							attackType = TruncToInt(Random * 3.999f);
						}
					}
				}
				
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_GENERAL,attackType);
				pAI->m_DeviceItem.Use();
				pAI->ResetStuckInfo();
				CurManeuver |= FLAGS_ATTACK;
			}

			// Handle tapdancing
			// if (!(CurManeuver & FLAGS_ATTACK))
			{
				if (bGun)
				{
					if ((Range < IdealRange)||(Range <= pAI->m_RangedMinRange))
					{
						CurManeuver |= FLAGS_BWD;
					}
					if (Random <= m_MovePropability)
					{
						if ((m_Flags & FLAGS_RIGHT)&&(Random > 0.5f))
						{
							CurManeuver |= FLAGS_RIGHT;
						}
						else if (m_Flags & FLAGS_LEFT)
						{
							CurManeuver |= FLAGS_LEFT;
						}
					}
				}
				else
				{
					if ((m_Flags & FLAGS_FWD)&&(Range > IdealRange))
					{
						CurManeuver |= FLAGS_FWD;
					}
					if (Random <= m_MovePropability)
					{
						if ((m_Flags & FLAGS_RIGHT)&&(Random > 0.5f))
						{
							CurManeuver |= FLAGS_RIGHT;
						}
						else if (m_Flags & FLAGS_LEFT)
						{
							CurManeuver |= FLAGS_LEFT;
						}
					}
				}
			}
		}
		else
		{	// Retreat a bit as well
			CurManeuver |= FLAGS_BWD;
		}
		m_LastManeuver = CurManeuver;
		m_LastManeuverTick = GameTick;

		// Appropriate facials
		if (m_LastManeuver & FLAGS_ATTACK)
		{
			pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_IRRITATED,20);
		}
		else if (m_LastManeuver & (FLAGS_FWD | FLAGS_BWD | FLAGS_RIGHT | FLAGS_LEFT))
		{
			pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_SECRET,20);
		}
	}
	
	if (m_LastManeuver & FLAGS_ATTACK)
	{
		// pAI->m_DeviceWeapon.Lock(true);
		pAI->m_DeviceItem.Lock(true);
	}

	CVec3Dfp32 Move = CVec3Dfp32(0.0f);
	if (m_LastManeuver & FLAGS_FWD)
	{
		Move[0] = 0.3f;
	}
	else if (m_LastManeuver & FLAGS_BWD)
	{
		Move[0] = -0.3f;
	}
	
	if (m_LastManeuver & FLAGS_RIGHT)
	{
		Move[1] = -0.3f;
	}
	else if (m_LastManeuver & FLAGS_LEFT)
	{
		Move[1] = 0.3f;
	}
	pAI->m_DeviceMove.Use(Move);
}

void CAI_Action_MeleeFight::OnStart()
{
	CAI_Core* pAI = AI();
	if ((!pAI)||(!pAI->m_pGameObject))
	{
		ExpireWithDelay(60);
		return;
	}
	CAI_Action::OnStart();

	m_bRetreating = false;
	m_StartPos = pAI->m_PathFinder.GetPathPosition(pAI->m_pGameObject);
	if ((m_MeleeRestrictSqr < Sqr(256))&&(pAI->m_bHostileFromBlowDmg))
	{
		m_MeleeRestrictSqr = Sqr(256);
	}
	pAI->SetMinStealthTension(CAI_Core::TENSION_HOSTILE);

	m_bFirstBlow = true;
	m_LastManeuverTick = 0;
	m_LastManeuver = 0;
};

void CAI_Action_MeleeFight::OnQuit()
{
	CAI_Core* pAI = AI();

	m_StartPos = CVec3Dfp32(_FP32_MAX);

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iOpponent);
	pAI->GetStealthTension();
	m_bRetreating = false;

	pAI->SetMinStealthTension(CAI_Core::TENSION_HOSTILE);

	// Mebbe touch up the relation?
	if ((pTarget)&&(pAI->m_bPlayerOnlyMeleeDmg)&&(!pAI->IsPlayer(m_iOpponent)))
	{
		if (pTarget->GetCurRelation() < CAI_AgentInfo::ENEMY)
		{	// This is the last time for us
			pTarget->SetRelation(pAI->m_KB.DefaultRelation(m_iOpponent));
		}
	}

	pAI->ResetStuckInfo();
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
	pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_GENERAL,0);

	m_bFirstBlow = true;
	m_iOpponent = 0;
	m_LastManeuverTick = 0;
	m_LastManeuver = 0;

	CAI_Action::OnQuit();
};

//CAI_Action_Close////////////////////////////////////////////////////////////////////////////
const char * CAI_Action_Close::ms_lStrFlags[] = 
{
	"GENTLEMAN",
	"STOPONDETECT",
	"STOPONSPOT",
	NULL,
};

CAI_Action_Close::CAI_Action_Close(CAI_ActionHandler* _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = CLOSE;
	m_TypeFlags = TYPE_MOVE | TYPE_OFFENSIVE;

	m_iTarget = 0;
	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_ScanPos);
	m_MinRangeDetect = 64.0f;	// Inside this range for DETECTED targets give estimate 0
	m_StayTimeout = -1;
	m_MoveMode = MODE_DEFAULT;
	m_Flags = 0;
};

void CAI_Action_Close::SetParameter(int _Param, CStr _Str)
{
	MAUTOSTRIP(CAI_Action_Close_SetParameter, 0);
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_FLAGS:
		SetParameter(_Param, StrToFlags(_Str));
		break;
	case PARAM_MINRANGE_DETECT:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;
	case PARAM_MOVEMODE:
		SetParameter(_Param, _Str.Val_int());
		break;
	default:
		CAI_Action::SetParameter(_Param, _Str);
	}
}


void CAI_Action_Close::SetParameter(int _Param, fp32 _Val)
{
	MAUTOSTRIP(CAI_Action_Close_SetParameter_2, 0);
	switch (_Param)
	{
	case PARAM_MINRANGE_DETECT:
		m_MinRangeDetect = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Close::SetParameter(int _Param, int _Val)
{
	MAUTOSTRIP(CAI_Action_Close_SetParameter_3, 0);
	switch (_Param)
	{
	case PARAM_FLAGS:
		m_Flags = _Val;
		break;
	case PARAM_MOVEMODE:
		if ((_Val == MODE_WALK)||(_Val == MODE_RUN))
		{
			m_MoveMode = _Val;
		}
		else
		{
			m_MoveMode = MODE_DEFAULT;
		}
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get flags for given string
int CAI_Action_Close::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

//Get parameter ID from string (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_Close::StrToParam(CStr _Str, int* _pResType)
{
	MAUTOSTRIP(CAI_Action_Close_StrToParam, 0);
	if (_Str == "MINRANGEDETECT")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_MINRANGE_DETECT;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else if (_Str == "MOVEMODE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_MOVEMODE;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

//Mandatory: Move; Optional: Look
int CAI_Action_Close::MandatoryDevices()
{
	MAUTOSTRIP(CAI_Action_Close_MandatoryDevices, 0);
	return (1 << CAI_Device::MOVE);
};

bool CAI_Action_Close::IsValid()
{
	MAUTOSTRIP(CAI_Action_Close_IsValid, false);

	CAI_Core* pAI = AI();

	if (!CAI_Action::IsValid())
	{
		return(false);
	}

	if (m_iTarget != 0)
	{
		if (!pAI->IsValidTarget(m_iTarget))
		{
			m_iTarget = 0;
			return(false);
		}

		CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
		if (!IsValid(pTarget))
		{
			m_iTarget = 0;
			return(false);
		}
	}
	return(true);
}


bool CAI_Action_Close::IsValid(CAI_AgentInfo* _pTarget)
{
	MAUTOSTRIP(CAI_Action_Close_IsValid_2, false);
	if (_pTarget == NULL)
	{
		return(false);
	}

	int Awareness = _pTarget->GetAwareness();
	if (Awareness < CAI_AgentInfo::DETECTED)
	{
		return(false);
	}

	if (!AI()->IsValidTarget(_pTarget->GetObjectID()))
	{
		return false;
	}

	fp32 RangeSqr = AI()->SqrDistanceToUs(_pTarget->GetBasePos());
	if (RangeSqr >  Sqr(AI()->m_CloseMaxRange))
	{
		return(false);
	}

	return true;
}

//Good offensively. Bad defensively.
CAI_ActionEstimate CAI_Action_Close::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP(CAI_Action_Close__GetEstimate, Est);

	CAI_Core* pAI = AI();
	if (IsValid())
	{
		CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
		if (!pTarget)
		{
			return Est;
		}

		if (!IsValid(pTarget))
		{
			return Est;
		}

		if ((m_Flags & FLAGS_GENTLEMAN)&&
			(pAI->SqrDistanceToUs(pTarget->GetObjectID()) <= Sqr(64))&&
			(pAI->m_pGameObject)&&(pAI->m_pServer)&&
			(pAI->m_pAIResources->ms_GameDifficulty < DIFFICULTYLEVEL_HARD))
		{
			if (!pAI->m_pAIResources->m_ActionGlobals.ms_MeleeFightSemaphore.Peek(pAI->m_pGameObject->m_iObject,1,pAI->GetGlobalAITick()))
			{
				return Est;
			}
		}
		//Default score 95
		m_iTarget = pTarget->GetObjectID();
		m_Destination = pTarget->GetSuspectedPosition();
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 60); 
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 10);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, -20);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 5);
	}
	return Est;
};

//Fetch boy, fetch!
void CAI_Action_Close::OnTakeAction()
{
	MAUTOSTRIP(CAI_Action_Close__OnTakeAction, MAUTOSTRIP_VOID);
	MSCOPESHORT(CAI_Action_Close::OnTakeAction);
	CAI_Core* pAI = AI();

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (!pTarget)
	{
		m_iTarget = 0;
		SetExpirationExpired();
		return;
	}

	if (m_pAH->IsRestricted(pTarget,true))
	{	
		if (pAI->DebugRender())
		{
			CStr Name = pAI->m_pGameObject->GetName();
			ConOut(Name+CStr(": Close restricted, delay 3s"));
		}
		pAI->OnTrackAgent(pTarget,10,false,false);
		ExpireWithDelay(60);
		return;
	}

	CAI_Core* pTargetAI = pAI->GetAI(m_iTarget);
	if ((!pTargetAI)||(!pTargetAI->IsConscious()))
	{	// Target unconscious or lacks AI
		ExpireWithDelay((int)(3.0f * pAI->GetAITicksPerSecond()));
	}

	int Awareness = pTarget->GetAwareness();
	CVec3Dfp32 OurPos = pAI->GetBasePos();
	CVec3Dfp32 TargetPos = pTarget->GetSuspectedPosition();
	fp32 Range = OurPos.Distance(TargetPos);
	if (INVALID_POS(m_Destination)||(m_Destination.DistanceSqr(TargetPos) > Sqr(64.0f)))
	{
		m_Destination = TargetPos;
	}


	pAI->ActivateAnimgraph();
	// SetExpiration(pAI->m_Patience);
	SetExpirationIndefinite();
	if ((pAI->m_bCanWallClimb)&&(pAI->m_bWallClimb))
	{	// CAI_Action_Close uses ground movement only!
		pAI->TempDisableWallClimb(pAI->m_Patience);
		return;
	}

	if ((Awareness == CAI_AgentInfo::SPOTTED)||(pTarget->GetGetLastSpottedTicks() < pAI->m_Patience))
	{
		pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		if ((m_Flags & FLAGS_STOPONSPOT)&&(Awareness == CAI_AgentInfo::SPOTTED)&&(Range <= pAI->m_SightRange * 0.75f)&&(!pAI->HasOrder(CAI_Core::ORDER_CHARGE)))
		{	// Just stand there, tracking our prey
			pAI->m_DeviceMove.Use();	// Hog device
			OnTrackAgent(pTarget,3);	// Track
			return;						// Nothing else
		}
	}
	else
	{
		pAI->SetStealthTension(CAI_Core::TENSION_HOSTILE,true);
		pAI->m_DeviceStance.SetIdleStance(CAI_Core::TENSION_HOSTILE);
		if (m_Flags & FLAGS_STOPONDETECT)
		{	// Just stand there, tracking our prey
			pAI->m_DeviceMove.Use();	// Hog device
			OnTrackAgent(pTarget,10);	// Track
			return;						// Nothing else
		}
	}
	if ((m_Flags & FLAGS_GENTLEMAN)&&(pAI->SqrDistanceToUs(pTarget->GetObjectID()) <= Sqr(64))&&(pAI->m_pGameObject)&&(pAI->m_pServer))
	{
		if (!pAI->m_pAIResources->m_ActionGlobals.ms_MeleeFightSemaphore.Peek(pAI->m_pGameObject->m_iObject,1,pAI->GetGlobalAITick()))
		{
			OnTrackAgent(pTarget,3);
			return;
		}
	}

	if ((Awareness == CAI_AgentInfo::SPOTTED)||(pTarget->GetGetLastSpottedTicks() < pAI->m_Patience))
	{
		m_StayTimeout = -1;
		OnTrackAgent(pTarget,3);
		if (Range >= pAI->m_CloseMinRange)
		{
			int32 moveResult;
			if (m_MoveMode == MODE_WALK)
			{
				moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,false,false,NULL);
			}
			else
			{
				moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseMaxSpeed,false,false,NULL);
			}
			if (!pAI->CheckMoveresult(moveResult,NULL))
			{
				INVALIDATE_POS(m_Destination);
				ExpireWithDelay(pAI->m_Patience);
				return;
			}
		}

		pAI->m_DeviceMove.Use();
	}
	else if (Awareness == CAI_AgentInfo::DETECTED)
	{
		if (Range > m_MinRangeDetect)
		{
			OnTrackAgent(pTarget,6);
			if (Range >= pAI->m_CloseMinRange)
			{
				int32 moveResult;
				if (m_MoveMode == MODE_RUN)
				{
					moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseMaxSpeed,false,false,NULL);
				}
				else
				{
					moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,false,false,NULL);
				}
				if (!pAI->CheckMoveresult(moveResult,NULL))
				{
					INVALIDATE_POS(m_Destination);
					ExpireWithDelay(pAI->m_Patience);
					return;
				}
			}
			pAI->m_DeviceMove.Use();
		}
		else
		{	// DETECTED target inside m_MinRangeDetect
			// Stay and scan for a while and then expire with delay
			if (m_StayTimeout == -1)
			{
				int32 StayDuration = (int32)(pAI->m_Patience);
				pAI->OverrideFlashThreshold(0.6f,StayDuration);
				m_ScanPos = pTarget->GetSuspectedHeadPosition();
				m_ScanPos[0] += (Random - Random) * Range * 0.5f;
				m_ScanPos[1] += (Random - Random) * Range * 0.5f;
				m_StayTimeout = pAI->m_Timer + StayDuration;
				pAI->m_DeviceMove.Use();
			}
			else if (pAI->m_Timer > m_StayTimeout)
			{	// Finished
				if (Random > 0.5f)
				{	// We cheat by actually giving away the real actual position of the target
					pTarget->SetCorrectSuspectedPosition();
					ExpireWithDelay((int)(pAI->m_Patience * (0.5f + Random)));
					return;
				}
				ExpireWithDelay(pAI->m_Patience);
				return;
			}
			else
			{
				OnTrackPos(m_ScanPos,CAI_Core::AI_TICKS_PER_SECOND);
				pAI->m_DeviceMove.Use();
			}
		}
	}
	else	// Awareness is NOTICED or less
	{
		ExpireWithDelay(pAI->m_Patience);
		return;
	}
};

void CAI_Action_Close::OnStart()
{
	CAI_Core* pAI = AI();

	CAI_Action::OnStart();
	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (pTarget)
	{
		if (pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)
		{
			SpeakRandom(CAI_Device_Sound::COMBAT_SPOTTED,5.0f);
		}
		else
		{
			SpeakRandom(CAI_Device_Sound::COMBAT_DETECTED,5.0f);
		}
	}
	else
	{
		m_iTarget = 0;
	}
};

void CAI_Action_Close::OnQuit()
{
	m_pAH->m_ExitCombatTick = AI()->GetAITick();
	m_iTarget = 0;
	INVALIDATE_POS(m_Destination);
	m_StayTimeout = -1;
	CAI_Action::OnQuit();
};

//CAI_Action_Combat////////////////////////////////////////////////////////////////////////////
const char* CAI_Action_Combat::ms_lStrFlags[] = 
{
	"SCENEPTS",
	"RANDOMPTS",
	"CLOSEST",
	"AVOIDAIM",
	"PLAYERFOV",
	"HEADING",
	"AVOIDLIGHT",
	"PREFERLIGHT",
	"DYNAMIC",
	"RETREAT",

		NULL,
};

CAI_Action_Combat::CAI_Action_Combat(CAI_ActionHandler * _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = COMBAT;
	m_TypeFlags = TYPE_MOVE | TYPE_OFFENSIVE;

	m_iTarget = 0;
	m_Duration = (int32)(5.0f * CAI_Core::AI_TICKS_PER_SECOND);
	m_NextRetreatSPTimer = 0;
	m_MoveMode = MODE_DEFAULT;
	m_MinAwareness = CAI_AgentInfo::DETECTED;

	INVALIDATE_POS(m_Destination);
	m_bWalkStop = false;
	m_StayTimeout = 0;
	m_pScenePoint = NULL;
	m_bCrouch = false;
	m_Flags = FLAGS_SCENEPOINTS | FLAGS_CLOSEST_PT;
};

void CAI_Action_Combat::SetParameter(int _Param, CStr _Str)
{
	MAUTOSTRIP(CAI_Action_Combat_SetParameter, 0);
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_DURATION:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;
	case PARAM_MOVEMODE:
		SetParameter(_Param, _Str.Val_int());
		break;
	case PARAM_FLAGS:
		SetParameter(_Param, StrToFlags(_Str));
		break;
	case PARAM_MINAWARENESS:
		{
			int Val = _Str.TranslateInt(CAI_AgentInfo::ms_TranslateAwareness);
			SetParameter(_Param, Val);
		}
		break;

	default:
		CAI_Action::SetParameter(_Param, _Str);
	}
}


void CAI_Action_Combat::SetParameter(int _Param, fp32 _Val)
{
	MAUTOSTRIP(CAI_Action_Combat_SetParameter_2, 0);
	// You can complain all you want - I'll keep it this way until I add a fp32 param again!
	CAI_Action::SetParameter(_Param, _Val);
}

void CAI_Action_Combat::SetParameter(int _Param, int _Val)
{
	MAUTOSTRIP(CAI_Action_Combat_SetParameter_3, 0);
	switch (_Param)
	{
	case PARAM_MOVEMODE:
		if ((_Val == MODE_WALK)||(_Val == MODE_RUN))
		{
			m_MoveMode = _Val;
		}
		else
		{
			m_MoveMode = MODE_DEFAULT;
		}
		break;

	case PARAM_FLAGS:
			m_Flags = _Val;
		break;

	case PARAM_MINAWARENESS:
		m_MinAwareness = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get flags for given string
int CAI_Action_Combat::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

//Get parameter ID from string (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_Combat::StrToParam(CStr _Str, int* _pResType)
{
	MAUTOSTRIP(CAI_Action_Combat_StrToParam, 0);
	
	if (_Str == "DURATION")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_DURATION;
	}
	else if (_Str == "MOVEMODE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_MOVEMODE;
	}
	else if (_Str == "MINAWARENESS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_MINAWARENESS;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

//Mandatory: Move; Optional: Look
int CAI_Action_Combat::MandatoryDevices()
{
	MAUTOSTRIP(CAI_Action_Combat_MandatoryDevices, 0);
	return (1 << CAI_Device::MOVE);
};

bool CAI_Action_Combat::IsValid()
{
	MAUTOSTRIP(CAI_Action_Combat_IsValid, false);
	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		return(false);
	}

	if (m_iTarget != 0)
	{
		CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
		if ((pTarget == NULL)||(pTarget->GetCurAwareness() < m_MinAwareness))
		{
			m_pAH->SetInvestigateObject(m_iTarget);
			m_iTarget = 0;
			return(false);
		}
	}

	// If creeping dark is out, Combat is invalid (for simplicity)
	int32 iCreepDark = 0;
	if (pAI->m_KB.GetCreepingDark(&iCreepDark))
	{
		m_iTarget = 0;
		return(false);
	}

	return(true);
}

CAI_ActionEstimate CAI_Action_Combat::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP(CAI_Action_Combat__GetEstimate, Est);
	CAI_Core* pAI = AI();

	if (!IsValid())
	{
		return Est;
	}

	CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
	if ((pTarget)&&(pTarget->GetCurAwareness() >= m_MinAwareness))
	{
		if ((m_Flags & FLAGS_SCENEPOINTS)&&(!m_pAH->ScenePointTypeAvailable(CWO_ScenePoint::TACTICAL,pAI->m_CombatMaxRange)))
		{
			m_iTarget = 0;
			return Est;
		}

		m_iTarget = pTarget->GetObjectID();

		Est.Set(CAI_ActionEstimate::OFFENSIVE,75);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 25);
		Est.Set(CAI_ActionEstimate::LOYALTY,0);
		Est.Set(CAI_ActionEstimate::LAZINESS,-20);
		Est.Set(CAI_ActionEstimate::STEALTH,0);
		Est.Set(CAI_ActionEstimate::VARIETY,5);
	}
	else
	{
		m_iTarget = 0;
	}

	return Est;
};

// Tries to find a destination position from available scenepoints
bool CAI_Action_Combat::FindScenePointDestination()
{
	CAI_Core* pAI = AI();

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	CAI_Core* pTargetAI = pAI->GetAI(m_iTarget);
	if ((!pTarget)||(!pTargetAI))
	{
		m_iTarget = 0;
		return(false);
	}
	CVec3Dfp32 TargetPos = pTarget->GetSuspectedPosition();

	// Try to find a valid scenepoint
	if (TargetPos.DistanceSqr(pAI->GetBasePos()) > Sqr(pAI->m_CombatTargetMaxRange))
	{
		m_pScenePoint = NULL;
		m_Destination = CVec3Dfp32(_FP32_MAX);
		SetExpirationExpired();
		return(false);
	}

	int Flags = 0;
	if (!(m_Flags & FLAGS_CLOSEST_PT))
	{
		Flags |= CAI_ActionHandler::SP_RANDOM_RANGE;
	}
	if (m_Flags & FLAGS_IN_PLAYERFOV)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_PLAYERFOV;
	}
	if (m_Flags & FLAGS_PREFER_HEADING)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_HEADING;
	}
	if (m_Flags & FLAGS_AVOID_LIGHT)
	{
		Flags |= CAI_ActionHandler::SP_AVOID_LIGHT;
	}
	else if (m_Flags & FLAGS_PREFER_LIGHT)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_LIGHT;
	}
	if (pAI->m_lCombatBehaviour.Len() > 0)
	{
		Flags |= CAI_ActionHandler::SP_COMBATBEHAVIOURS;
	}
	if ((m_Flags & FLAGS_RETREAT)&&
		(pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)&&
		/*(TargetPos.DistanceSqr(pAI->GetBasePos()) < Sqr(pAI->m_CombatMaxRange * 0.5f))&&*/
		(pTargetAI->IsLookWithinAngle(15.0f,pAI->GetObjectID())))
	{
		Flags |= CAI_ActionHandler::SP_RETREAT_ARC;
	}
	if (pAI->HasOrder(CAI_Core::ORDER_COVER))
	{
		Flags |= CAI_ActionHandler::SP_RETREAT_ARC;
	}
	if (pAI->HasOrder(CAI_Core::ORDER_FLANK))
	{
		Flags |= CAI_ActionHandler::SP_PREFER_FLANK;
	}

	CMat4Dfp32 OurMat;
	pAI->GetBaseMat(OurMat);
	if (m_Flags & FLAGS_DYNAMIC)
	{
		m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::TACTICAL | CWO_ScenePoint::DYNAMIC,Flags,pAI->m_CombatMinRange,pAI->m_CombatMaxRange,OurMat,TargetPos,0.0f,m_iTarget);
	}
	else
	{
		m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::TACTICAL,Flags,pAI->m_CombatMinRange,pAI->m_CombatMaxRange,OurMat,TargetPos,0.0f,m_iTarget);
	}
	INVALIDATE_POS(m_Destination);
	m_bWalkStop = false;
	if (!m_pScenePoint)
	{
		return(false);
	}
	if (m_pScenePoint->GetType() & CWO_ScenePoint::JUMP_CLIMB)
	{
		m_Destination = m_pScenePoint->GetPosition();
	}
	else
	{
		m_Destination = pAI->m_PathFinder.GetPathPosition(m_pScenePoint->GetPosition(),4,2);
	}
	if ((!m_pAH->RequestScenePoint(m_pScenePoint))||(INVALID_POS(m_Destination))||(m_pAH->IsRestricted(m_Destination)))
	{
		INVALIDATE_POS(m_Destination);
		m_pAH->UpdateScenepointHistory(m_pScenePoint);
		m_pScenePoint = NULL;
		return(false);
	}

	if ((Flags & CAI_ActionHandler::SP_RETREAT_ARC)&&(!m_pScenePoint->InFrontArc(TargetPos)))
	{	// We are retreating a bit
		SpeakRandom(CAI_Device_Sound::ENEMY_RETREAT);
		/*
		Har lagt in det på riflecombat

		(0 = left, 1 = right)

		pCD->AnimGraph2.GetAG2I()->SendImpulse(Context,CXRAG2_Impulse(AG2_IMPULSETYPE_GLANCE,0));

		*/
#ifndef M_RTM
		if (pAI->DebugRender())
		{
			CStr SPName = m_pScenePoint->GetName();
			CStr DudeName = pAI->m_pGameObject->GetName();
			ConOutL(CStrF("Retreat SP: ")+DudeName+":"+SPName);
		}
#endif
	}

	if ((m_pScenePoint)&&(!m_pScenePoint->IsWaypoint()))
	{
		if (m_pScenePoint->GetSqrRadius() > Sqr(16.0f))
		{
			m_bWalkStop = true;
		}
	}

	return(true);
}

bool CAI_Action_Combat::FindRandomDestination()
{
	CAI_Core* pAI = AI();
	if ((!pAI)||(!(m_Flags & FLAGS_RANDOM)))
	{
		m_pScenePoint = NULL;
		m_Destination = CVec3Dfp32(_FP32_MAX);
		SetExpirationExpired();
		return(false);
	}

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (!pTarget)
	{
		m_iTarget = 0;
		return(false);
	}
	CVec3Dfp32 TargetPos = pTarget->GetSuspectedPosition();

	if (TargetPos.DistanceSqr(pAI->GetBasePos()) > Sqr(pAI->m_CombatTargetMaxRange))
	{
		m_pScenePoint = NULL;
		m_Destination = CVec3Dfp32(_FP32_MAX);
		SetExpirationExpired();
		return(false);
	}

	// We should generate a random 
	// We don't have any scenepoints at the moment so we just generate random
	// movement directions (forward +- 90 degrees, 2-10m)
	CVec3Dfp32 OurPos,Goal,StopPos;
	OurPos = pAI->GetBasePos();

	fp32 Heading,Range,CurRangeSqr,BestRangeSqr;
	CVec3Dfp32 CurPos;
	BestRangeSqr =_FP32_MAX;
	Goal = CVec3Dfp32(_FP32_MAX);
	for (int i = 0; i < 10; i++)
	{	// Iterate 10 times to find a valid point
		Range = pAI->m_CombatMinRange + (pAI->m_CombatMaxRange - pAI->m_CombatMinRange) * Random;
		Heading = Random;
		CurPos = OurPos + CVec3Dfp32(Range * QCos(Heading * _PI2), Range * QSin(-Heading * _PI2), 0);
		CurRangeSqr = CurPos.DistanceSqr(TargetPos);
		if (CurRangeSqr < Sqr(pAI->m_CombatTargetMinRange)) {continue;}
		if ((m_Flags & FLAGS_CLOSEST_PT)&&(CurRangeSqr < BestRangeSqr))
		{
			BestRangeSqr = CurRangeSqr;
			Goal = CurPos;
		}
		else
		{
			Goal = CurPos;
			break;
		}
	}

	if (INVALID_POS(Goal))
	{
		m_pScenePoint = NULL;
		m_Destination = CVec3Dfp32(_FP32_MAX);
		SetExpirationExpired();
		return(false);
	}

	if ((pAI->m_PathFinder.GetBlockNav())&&
		(!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,Goal,
			(int)pAI->GetBaseSize(),
			(int)pAI->GetHeight(),
			pAI->m_bWallClimb,
			(int)pAI->GetStepHeight(),
			(int)pAI->GetJumpHeight(),
			&StopPos)))
	{
		m_Destination = StopPos;
	}
	else
	{
		m_Destination = Goal;
	}

	if (OurPos.DistanceSqr(m_Destination) < Sqr(pAI->m_CombatMinRange))
	{
		m_Destination = CVec3Dfp32(_FP32_MAX);
		return(false);
	}

	// Stop roaming if restricted
	if ((VALID_POS(m_Destination))&&(m_pAH->IsRestricted(m_Destination)))
	{	
#ifndef M_RTM
		if (pAI->DebugRender())
		{
			CStr Name = pAI->m_pGameObject->GetName();
			ConOut(Name+CStr(": Combat restricted"));
		}
#endif
		m_Destination = CVec3Dfp32(_FP32_MAX);
		return(false);
	}

	m_bWalkStop = true;

	return(true);
}

bool CAI_Action_Combat::FindDestination()
{
	MSCOPESHORT(CAI_Action_Combat::FindDestination);
	CAI_Core* pAI = AI();

	if (VALID_POS(m_Destination))
	{
		return(true);
	}

	m_StayTimeout = -1;
	if (m_Flags & FLAGS_SCENEPOINTS)
	{
		FindScenePointDestination();
	}
	if ((INVALID_POS(m_Destination))&&(m_Flags & FLAGS_RANDOM))
	{
		FindRandomDestination();
	}
	
	if (VALID_POS(m_Destination))
	{	// Handle speaking
		CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
		int Awareness = CAI_AgentInfo::NONE;
		if (pTarget)
		{
			if (pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)
			{
				SpeakRandom(CAI_Device_Sound::COMBAT_SPOTTED,Random);
			}
			else
			{
				SpeakRandom(CAI_Device_Sound::COMBAT_DETECTED,Random);
			}
		}
		return(true);
	}
	else
	{
		return(false);
	}
};

bool CAI_Action_Combat::MoveToDestination()
{
	MSCOPESHORT(CAI_Action_Combat::MoveToDestination);
	CAI_Core* pAI = AI();

	if (INVALID_POS(m_Destination))
	{
		m_StayTimeout = -1;
		if (m_pScenePoint)
		{
			m_pAH->UpdateScenepointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
		}
		return(false);
	}

	if (pAI->m_pGameObject->AI_IsJumping())
	{
		return(false);
	}

	// Some basice useful values
	CVec3Dfp32 OurPos = pAI->GetBasePos();
	fp32 RangeSqr = m_Destination.DistanceSqr(OurPos);
	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (!pTarget)
	{
		m_iTarget = 0;
		INVALIDATE_POS(m_Destination);
		return(false);
	}
	CVec3Dfp32 TargetPos = pTarget->GetSuspectedPosition();
	int32 Awareness = pTarget->GetCurAwareness();

	// Handle speaking
	if (Awareness < CAI_AgentInfo::DETECTED)
	{	// NOTICED or worse
		SetExpirationExpired();
	}

	// Determine movemode as follows
	// If we're within 32 units of m_Destination we walk, otherwise we decide on how to move based on first m_MoveMode
	int32 bRun = true;
	if (m_MoveMode == MODE_RUN)
	{
		bRun = true;
	}
	else if (m_MoveMode == MODE_WALK)
	{
		bRun = false;
	}
	else
	{	// Based upon awareness
		if ((Awareness == CAI_AgentInfo::SPOTTED)||(pTarget->GetGetLastSpottedTicks() < pAI->m_Patience))
		{
			bRun = true;
		}
		else
		{
			bRun = false;
		}
	}

	// Is the scenepoint (if any) valid?
	if (m_pScenePoint)
	{
		if ((!bRun)&&(!m_pScenePoint->IsWaypoint())&&(m_pScenePoint->GetSqrRadius() >= Sqr(16.0f))&&(m_pScenePoint->GetCosArcAngle() <= 0.707f))
		{
			pAI->CheckWalkStop(m_Destination);
		}
		bool bTargetInArc = m_pAH->CheckScenePointTarget(m_pScenePoint,pTarget);
		// If player is aiming at us, we should change SP
		if ((m_Flags & FLAGS_AVOID_PLAYERAIM)&&(pAI->IsPlayerLookWithinAngle(5)))
		{
			INVALIDATE_POS(m_Destination);
			m_pAH->UpdateScenepointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
			return(false);
		}

		if (!m_pAH->RequestScenePoint(m_pScenePoint))
		{
			SetExpirationExpired();
			return(false);
		}

		if (!bTargetInArc)
		{
			if ((m_Flags & FLAGS_RETREAT)||(pAI->HasOrder(CAI_Core::ORDER_COVER)))
			{
				// We stay put but periodically check for other scenepoints
				if ((pAI->m_Timer % 10 == 0)&&(pAI->m_Timer >= m_NextRetreatSPTimer))
				{
					int Flags = 0;
					if (!(m_Flags & FLAGS_CLOSEST_PT))
					{
						Flags |= CAI_ActionHandler::SP_RANDOM_RANGE;
					}
					if (m_Flags & FLAGS_IN_PLAYERFOV)
					{
						Flags |= CAI_ActionHandler::SP_PREFER_PLAYERFOV;
					}
					if (m_Flags & FLAGS_PREFER_HEADING)
					{
						Flags |= CAI_ActionHandler::SP_PREFER_HEADING;
					}
					if (m_Flags & FLAGS_AVOID_LIGHT)
					{
						Flags |= CAI_ActionHandler::SP_AVOID_LIGHT;
					}
					else if (m_Flags & FLAGS_PREFER_LIGHT)
					{
						Flags |= CAI_ActionHandler::SP_PREFER_LIGHT;
					}
					if (pAI->m_lCombatBehaviour.Len() > 0)
					{
						Flags |= CAI_ActionHandler::SP_COMBATBEHAVIOURS;
					}
					// No CAI_ActionHandler::SP_RETREAT_ARC when (m_Flags & FLAGS_RETREAT)
					CMat4Dfp32 OurMat;
					pAI->GetBaseMat(OurMat);
					CWO_ScenePoint* pNewSP = NULL;
					int32 Types = CWO_ScenePoint::TACTICAL;
					if (m_Flags & FLAGS_DYNAMIC)
					{
						Types |= CWO_ScenePoint::DYNAMIC;
					}
					pNewSP = m_pAH->GetBestScenePoint(Types,Flags,pAI->m_CombatMinRange,pAI->m_CombatMaxRange,OurMat,TargetPos,0.0f,m_iTarget);
					if ((pNewSP)&&(pNewSP != m_pScenePoint))
					{
						m_pScenePoint = pNewSP;
						m_Destination = pAI->m_PathFinder.GetPathPosition(m_pScenePoint->GetPosition(),4,2);
						m_NextRetreatSPTimer = pAI->m_Timer + int(5.0f * fp32(pAI->GetAITicksPerSecond()));
						return(false);
					}
				}
			}
			else
			{	// Ah, shuck it, this SP is no good
				INVALIDATE_POS(m_Destination);
				m_pAH->UpdateScenepointHistory(m_pScenePoint);
				m_pScenePoint = NULL;
				return(false);
			}
		}

#ifndef M_RTM
		m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif

		// Handle running with HOSTILE stance while between scenepoints
		if (pAI->SqrDistanceToUs(m_Destination) > Sqr(64.0f))
		{
			bRun = true;
		}

		// If m_pScenePoint has TacFlags we are not actually using IsAt to determine closeness to destination
		if ((m_pScenePoint->GetTacFlags())&&(bTargetInArc))
		{
			if (pAI->SqrDistanceToUs(m_Destination) <= Sqr(64.0f))
			{
				pAI->OnTrackDir(m_pScenePoint->GetDirection(),m_iTarget,10,false,false);
				if (pAI->SqrDistanceToUs(m_Destination) <= Sqr(32.0f))
				{	// Silence gun within 32 units
					m_pAH->ExpireActions(WEAPON_ATTACK_RANGED,pAI->GetAITicksPerSecond());
					pAI->m_DeviceStance.SetTargetInFOV(true);	// Keep stance
					pAI->m_DeviceWeapon.Lock(false);
					// Align with sp
					pAI->m_DeviceLook.Free();
					pAI->OnTrackDir(m_pScenePoint->GetDirection(),m_iTarget,3,false,false);
				}
				if (pAI->GridSqrDistanceToUs(m_Destination) <= Sqr(8.0f))
				{
					pAI->ResetStuckInfo();
					if (pAI->InitCombatBehaviours(m_pScenePoint,GetPriority(),m_iTarget))
					{	// Don't choose m_pScenePoint next time, don't NULL it yet
						m_pAH->UpdateScenepointHistory(m_pScenePoint);
						if (Awareness >= CAI_AgentInfo::SPOTTED)
						{
							SpeakRandom(CAI_Device_Sound::COMBAT_SPOTTED,true);
						}
						else
						{
							SpeakRandom(CAI_Device_Sound::COMBAT_DETECTED,true);
						}
						pAI->m_DeviceMove.Use();
						return(true);
					}
					else
					{	// The scenepoint's flag are incompatible with any of the bots behaviours
						// Invalidate the SP, and save it in history to lessen the risk that we may actually happen upon it again
						m_pAH->UpdateScenepointHistory(m_pScenePoint);
						m_pScenePoint = NULL;
						INVALIDATE_POS(m_Destination);
						return(false);
					}
				}

				int32 moveResult;
				if (bRun)
				{
					moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseMaxSpeed,false,false,m_pScenePoint);
				}
				else
				{
					moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,false,false,m_pScenePoint);
				}
				if (!pAI->CheckMoveresult(moveResult,m_pScenePoint))
				{
					m_pAH->UpdateScenepointHistory(m_pScenePoint);
					m_pScenePoint = NULL;
					m_StayTimeout = -1;
					INVALIDATE_POS(m_Destination);
					return(false);
				}
				return(false);
			}
		}
		else
		{
			if ((pAI->SqrDistanceToUs(m_Destination) <= Sqr(32.0f))&&(!m_pScenePoint->DirInFrontArc()))
			{	// Align with sp
				pAI->m_DeviceLook.Free();
				pAI->OnTrackDir(m_pScenePoint->GetDirection(),m_iTarget,6,false,false);
			}

			if ((m_pScenePoint->GetSqrRadius() <= Sqr(0.0f))&&(RangeSqr <= Sqr(16.0f)))
			{
				pAI->SetPerfectPlacement(m_pScenePoint);
			}
			if (m_pScenePoint->IsAt(pAI->GetBasePos()))
			{
				return(true);
			}
		}
	}
	else
	{	// Random point, we're at when within an arbitrary 32 units
		if (pAI->SqrDistanceToUs(m_Destination) < Sqr(32.0f))
		{
			return(true);
		}
	}

	int32 moveResult;
	if (bRun)
	{
		moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseMaxSpeed,false,false,m_pScenePoint);
	}
	else
	{
		moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,false,false,m_pScenePoint);
	}
	if (moveResult== CAI_Core::MOVE_DONE)
	{
		return(true);
	}
	if (!pAI->CheckMoveresult(moveResult,m_pScenePoint))
	{
		if (m_pScenePoint)
		{
			m_pAH->UpdateScenepointHistory(m_pScenePoint);
			m_pScenePoint = NULL;
			m_StayTimeout = -1;
		}
		INVALIDATE_POS(m_Destination);
		return(false);
	}
	// Hog device
	pAI->m_DeviceMove.Use();

	return(false);
};

bool CAI_Action_Combat::StayAtDestination()
{
	CAI_Core* pAI = AI();

	// Handle Combat behaviours
	if ((m_pScenePoint)&&(m_pScenePoint->GetTacFlags())&&!(pAI->HandleCombatBehaviours()))
	{
		m_pAH->UpdateScenepointHistory(m_pScenePoint);
		m_pScenePoint = NULL;
		INVALIDATE_POS(m_Destination);
		return(false);
	}

	if (m_StayTimeout == -1)
	{
		int StayTicks = 0;
		if (m_pScenePoint)
		{
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
			StayTicks = (int)(m_pScenePoint->GetBehaviourDuration() * pAI->GetAITicksPerSecond());
		}
		else
		{
			StayTicks = m_Duration;
		}
		if (!pAI->m_CurCB.m_iBehaviour)
		{	// No crouching if we do combat behaviours
			if ((!m_bCrouch)&&(m_pScenePoint->GetCrouchFlag()))
			{
				pAI->m_DeviceStance.Crouch(true);
				m_bCrouch = true;
			}
		}
		m_StayTimeout = pAI->m_Timer + StayTicks;
		CWO_ScenePoint* orgSP = m_pScenePoint;
		m_pAH->ActivateScenePoint(m_pScenePoint,GetPriority());
		if (orgSP != m_pScenePoint) { return(false); }
		pAI->OverrideFlashThreshold(0.6f,60);
		// Reset expiration to guarantee(?) that we will stay StayTicks+20
		SetMinExpirationDuration(StayTicks+20);
		if ((pAI->m_bWallClimb)&&(pAI->m_pGameObject->AI_IsOnWall()))
		{
			pAI->TempEnableWallClimb(StayTicks+pAI->m_Patience);
		}
		return(true);
	}
	else
	{
		if (pAI->m_Timer > m_StayTimeout)
		{	// We're done staying
			m_StayTimeout = -1;
			if (m_pScenePoint)
			{
				m_pScenePoint = NULL;
			}
			INVALIDATE_POS(m_Destination);
			return(false);
		}
		else
		{	// We're not yet done staying
			if ((m_pScenePoint)&&(m_pScenePoint->GetBehaviour())&&(m_pScenePoint->GetBehaviourDuration() == 0.0f))
			{
				m_StayTimeout = pAI->m_Timer + pAI->GetAITicksPerSecond();
			}
			pAI->m_DeviceMove.Use();
			return(true);
		}
	}
};

void CAI_Action_Combat::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		INVALIDATE_POS(m_Destination);
		m_StayTimeout = -1;
		m_pScenePoint = NULL;
	}
};

void CAI_Action_Combat::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_Combat::OnTakeAction);
	CAI_Core* pAI = AI();

	CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
	if (!pTarget)
	{
		SetExpirationExpired();
		return;
	}
	m_iTarget = pTarget->GetObjectID();
	int32 Awareness = pTarget->GetCurAwareness();
	if ((pTarget->GetCurRelation() < CAI_AgentInfo::ENEMY)||(Awareness < m_MinAwareness))
	{
		m_pAH->SetInvestigateObject(m_iTarget);
		SetExpirationExpired();
		return;
	}
	
	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();
	
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
	if (pCD->m_AnimGraph2.GetStateFlagsLoCombined() & AG2_STATEFLAG_HURTACTIVE)
	{
		pAI->ResetStuckInfo();
		return;
	}

	// STANCE
	// IDLESTANCE_COMBAT: We are close to the scenepoint or the target is pretty much straight ahead
	// IDLESTANCE_HOSTILE: All other cases
	if ((VALID_POS(m_Destination))&&(pAI->SqrDistanceToUs(m_Destination) < Sqr(64.0f)))
	{	// IDLESTANCE_COMBAT: We are close to the scenepoint
		pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
	}
	else
	{	// IDLESTANCE_COMBAT: The target is pretty much straight ahead
		if (pAI->CosHeadingDiff(m_iTarget,true) >= 0.86f)
		{
			if (Awareness >= CAI_AgentInfo::SPOTTED)
			{
				OnTrackAgent(pTarget,10);
			}
			pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		}
		else
		{
			pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		}
	}

	if (FindDestination())
	{
		if (MoveToDestination())
		{
			StayAtDestination();
		}
	}

	// We put the tracking code last, to allow FindDestination(), MoveToDestination() and StayAtDestination()
	// first use of look device.
	if (Awareness == CAI_AgentInfo::SPOTTED)
	{
		OnTrackAgent(pTarget,10);
	}
	else
	{
		OnTrackAgent(pTarget,6);
	}
};

void CAI_Action_Combat::OnStart()
{
	CAI_Core* pAI = AI();
	CAI_Action::OnStart();

	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_pScenePoint = NULL;
	m_StayTimeout = -1;
	m_NextRetreatSPTimer = 0;
	m_bCrouch = false;

	CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
	if (pTarget)
	{
		m_iTarget = pTarget->GetObjectID();
		CVec3Dfp32 TargetPos = pTarget->GetBasePos();
		// Find a destination (or terminate)
		if (m_Flags & FLAGS_SCENEPOINTS)
		{
			FindScenePointDestination();
		}
		if ((INVALID_POS(m_Destination))&&(m_Flags & FLAGS_RANDOM))
		{
			FindRandomDestination();
		}
		if (VALID_POS(m_Destination))
		{
			if (!m_pAH->HasAction(COMBATLEADER))
			{
				int iLeader = m_pAH->GetClosestActionUser(COMBATLEADER);
				CAI_Core* pAI_Leader = pAI->GetAI(iLeader);
				if (pAI_Leader)
				{
					CAI_Action* pAction;
					if (pAI_Leader->m_AH.GetAction(COMBATLEADER,&pAction))
					{
						CAI_Action_CombatLeader* pLeaderAction = safe_cast<CAI_Action_CombatLeader>(pAction);
						pLeaderAction->ReceiveDecision(pAI->m_pGameObject->m_iObject,pTarget->GetObjectID(),m_Destination);
					}
				}
			}
		}
	}
};

void CAI_Action_Combat::OnQuit()
{
	CAI_Core* pAI = AI();

	// Initiate search if quitting after substantial time
	m_pAH->m_ExitCombatTick = pAI->GetAITick();
	m_iTarget = 0;
	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_pScenePoint = NULL;
	if (m_bCrouch)
	{
		pAI->m_DeviceStance.Crouch(false);
		m_bCrouch = false;
	}
	CAI_Action::OnQuit();
};

//CAI_Action_Cover////////////////////////////////////////////////////////////////////////////
const char* CAI_Action_Cover::ms_lStrFlags[] = 
{
	"SCENEPTS",
	"CLOSEST",
	"CROUCH",
	"LEFT",
	"RIGHT",
	NULL,
};

CAI_Action_Cover::CAI_Action_Cover(CAI_ActionHandler * _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = COVER;
	m_TypeFlags = TYPE_MOVE | TYPE_DEFENSIVE;

	m_Duration = (int)(5.0f * CAI_Core::AI_TICKS_PER_SECOND);
	m_MoveMode = MODE_DEFAULT;

	m_iAttacker = 0;
	m_bTaken = false;
	m_bCrouch = false;

	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_StayTimeout = 0;
	m_pScenePoint = NULL;
	m_Flags = FLAGS_SCENEPOINTS | FLAGS_CLOSEST_PT | FLAGS_CROUCH | FLAGS_LEFT | FLAGS_RIGHT;

	// Don't dodge more than once every 3 seconds
	m_MinInterval = (int)(3.0f * CAI_Core::AI_TICKS_PER_SECOND);
};

void CAI_Action_Cover::SetParameter(int _Param, CStr _Str)
{
	MAUTOSTRIP(CAI_Action_Cover_SetParameter, MAUTOSTRIP_VOID);
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_MINRANGE:
	case PARAM_MAXRANGE:
	case PARAM_MINRANGE_TARGET:
	case PARAM_MAXRANGE_TARGET:
	case PARAM_DURATION:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;
	case PARAM_MOVEMODE:
		SetParameter(_Param, _Str.Val_int());
		break;
	case PARAM_FLAGS:
		SetParameter(_Param, StrToFlags(_Str));
		break;

	default:
		CAI_Action::SetParameter(_Param, _Str);
	}
}


void CAI_Action_Cover::SetParameter(int _Param, fp32 _Val)
{
	MAUTOSTRIP(CAI_Action_Cover_SetParameter_2, MAUTOSTRIP_VOID);
	switch (_Param)
	{
	case PARAM_DURATION:
		m_Duration = (int)(_Val * AI()->GetAITick());
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_Cover::SetParameter(int _Param, int _Val)
{
	MAUTOSTRIP(CAI_Action_Cover_SetParameter_3, 0);
	switch (_Param)
	{
	case PARAM_MOVEMODE:
		if ((_Val == MODE_WALK)||(_Val == MODE_RUN))
		{
			m_MoveMode = _Val;
		}
		else
		{
			m_MoveMode = MODE_DEFAULT;
		}
		break;

	case PARAM_FLAGS:
		m_Flags = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get flags for given string
int CAI_Action_Cover::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

//Get parameter ID from string (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_Cover::StrToParam(CStr _Str, int* _pResType)
{
	MAUTOSTRIP(CAI_Action_Cover_StrToParam, 0);
	if (_Str == "DURATION")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_DURATION;
	}
	else if (_Str == "MOVEMODE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_MOVEMODE;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

//Mandatory: Move; Optional: Look
int CAI_Action_Cover::MandatoryDevices()
{
	MAUTOSTRIP(CAI_Action_Cover_MandatoryDevices, 0);
	return (1 << CAI_Device::MOVE);
};

bool CAI_Action_Cover::IsValid()
{
	MAUTOSTRIP(CAI_Action_Cover_IsValid, false);
	if (!CAI_Action::IsValid())
	{
		return(false);
	}
	
	if ((m_bTaken)||(m_pAH->FiredAt(NULL,DAMAGETYPE_TRANQUILLIZER | DAMAGETYPE_BLOW,false) > CAI_ActionHandler::NONE))
	{
		return(true);
	}

	return(false);
}

CAI_ActionEstimate CAI_Action_Cover::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP(CAI_Action_Cover_GetEstimate, Est);
	CAI_Core* pAI = AI();

	if (IsValid())
	{	// Find the attacker
		if (m_pAH->FiredAt(&m_iAttacker,DAMAGETYPE_TRANQUILLIZER | DAMAGETYPE_BLOW,false) > CAI_ActionHandler::NONE)
		{
			CAI_AgentInfo* pAttackerInfo = pAI->m_KB.GetAgentInfo(m_iAttacker);
			if (!pAttackerInfo)
			{
				return Est;
			}
			else
			{
				fp32 DistSqr = pAI->SqrDistanceToUs(m_iAttacker);
				if ((DistSqr < Sqr(pAI->m_CoverTargetMinRange))||(DistSqr > Sqr(pAI->m_CoverTargetMaxRange)))
				{
					return Est;
				}
			}
		}
		else
		{
			return Est;
		}

		Est.Set(CAI_ActionEstimate::DEFENSIVE,75);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 25);
		Est.Set(CAI_ActionEstimate::LOYALTY,0);
		Est.Set(CAI_ActionEstimate::LAZINESS,-20);
		Est.Set(CAI_ActionEstimate::STEALTH,0);
		Est.Set(CAI_ActionEstimate::VARIETY,5);
	}
	return Est;
};

// Tries to find a destination position from available scenepoints
bool CAI_Action_Cover::FindScenePointDestination(CVec3Dfp32& _TargetPos)
{
	CAI_Core* pAI = AI();

	// Try to find a valid scenepoint
	if (_TargetPos.DistanceSqr(pAI->GetBasePos()) > Sqr(pAI->m_CoverTargetMaxRange))
	{
		m_pScenePoint = NULL;
		m_Destination = CVec3Dfp32(_FP32_MAX);
		SetExpirationExpired();
		return(false);
	}

	int Flags = 0;
	CVec3Dfp32 OurPos = pAI->GetBasePos();
	CMat4Dfp32 OurMat;
	pAI->GetBaseMat(OurMat);
	if (!(m_Flags & FLAGS_CLOSEST_PT))
	{
		Flags |= CAI_ActionHandler::SP_RANDOM_RANGE;
	}
	if (m_Flags & FLAGS_DYNAMIC)
	{
		m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::COVER | CWO_ScenePoint::DYNAMIC,Flags,pAI->m_CoverMinRange,pAI->m_CoverMaxRange,OurMat,_TargetPos,0.0f,0);
	}
	else
	{
		m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::COVER,Flags,pAI->m_CoverMinRange,pAI->m_CoverMaxRange,OurMat,_TargetPos,0.0f,0);
	}
	INVALIDATE_POS(m_Destination);
	if (!m_pScenePoint)
	{	// Combat SPs?
		if (!(m_Flags & FLAGS_CLOSEST_PT))
		{
			Flags |= CAI_ActionHandler::SP_RANDOM_RANGE;
		}
		if (m_Flags & FLAGS_DYNAMIC)
		{
			m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::TACTICAL | CWO_ScenePoint::DYNAMIC,Flags,pAI->m_CoverMinRange,pAI->m_CoverMaxRange,OurMat,_TargetPos,0.0f,0);
		}
		else
		{
			m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::TACTICAL,Flags,pAI->m_CoverMinRange,pAI->m_CoverMaxRange,OurMat,_TargetPos,0.0f,0);
		}
		if ((m_pScenePoint)&&(m_pScenePoint->GetTacFlags()))
		{
			m_Destination = pAI->m_PathFinder.GetPathPosition(m_pScenePoint->GetPosition(),4,2);
			m_pScenePoint = NULL;
			return(true);
		}
		else
		{
			return(false);
		}
	}
	m_Destination = pAI->m_PathFinder.GetPathPosition(m_pScenePoint->GetPosition(),4,2);

	if ((!m_pAH->RequestScenePoint(m_pScenePoint))||(INVALID_POS(m_Destination))||(m_pAH->IsRestricted(m_Destination)))
	{
		INVALIDATE_POS(m_Destination);
		m_pAH->UpdateScenepointHistory(m_pScenePoint);
		m_pScenePoint = NULL;
		return(false);
	}

	return(true);
}

bool CAI_Action_Cover::FindRandomDestination(CVec3Dfp32& _TargetPos)
{
	CAI_Core* pAI = AI();

	if ((INVALID_POS(_TargetPos))||
		(pAI->SqrDistanceToUs(_TargetPos) > Sqr(pAI->m_CoverTargetMaxRange))||
		(pAI->SqrDistanceToUs(_TargetPos) < Sqr(pAI->m_CoverTargetMinRange))||
		(!(m_Flags & (FLAGS_LEFT |FLAGS_RIGHT))))
	{
		m_pScenePoint = NULL;
		m_Destination = CVec3Dfp32(_FP32_MAX);
		return(false);
	}

	// We move left/right or crouch at random or based on flags
	// We should generate a random 
	CVec3Dfp32 NoseDir,Move;
	NoseDir = pAI->GetBasePos() - _TargetPos;
	NoseDir[2] = 0.0f;
	Move = CVec3Dfp32(0.0f);

	if ((m_Flags & FLAGS_LEFT)&&((Random < 0.5)||!(m_Flags & FLAGS_RIGHT)))
	{	// Left
		Move[0] = NoseDir[1];
		Move[1] = -NoseDir[0];
		Move[2] = NoseDir[2];
	}
	else
	{	// Right
		Move[0] = -NoseDir[1];
		Move[1] = NoseDir[0];
		Move[2] = NoseDir[2];
	}
	CVec3Dfp32 OurPos = pAI->GetBasePos();
	Move.Normalize();
	Move = OurPos + Move * 96.0f;
	// We must now check to see if we really can go to m_GoalPos in a straight line
	if (pAI->m_PathFinder.IsValid())
	{
		CVec3Dfp32 StopPos;
		if ((pAI->m_PathFinder.GetBlockNav())&&
			(!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,Move,
				(int)pAI->GetBaseSize(),
				(int)pAI->GetHeight(),
				pAI->m_bWallClimb,
				(int)pAI->GetStepHeight(),
				(int)pAI->GetJumpHeight(),
				&StopPos)))
		{
			Move = StopPos;
		}
	}
	m_Destination = Move;

	return(true);
}

int CAI_Action_Cover::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int nAdded = 0;

	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_NBR_DODGE_LEFT);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_NBR_DODGE_RIGHT);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_NBR_ROLL_LEFT);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_NBR_ROLL_RIGHT);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_NBR_CROUCH_ROLL_LEFT);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_NBR_CROUCH_ROLL_RIGHT);

	return(nAdded);
};

void CAI_Action_Cover::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		INVALIDATE_POS(m_Destination);
		m_StayTimeout = -1;
		m_pScenePoint = NULL;
	}
};

void CAI_Action_Cover::OnTakeAction()
{
	MAUTOSTRIP(CAI_Action_Cover_OnTakeAction, MAUTOSTRIP_VOID);
	MSCOPESHORT(CAI_Action_Cover::OnTakeAction);
	CAI_Core* pAI = AI();
	if ((!pAI)||(!m_bTaken))
	{
		SetExpirationExpired();
		return;
	}

	if ((pAI->SqrDistanceToUs(m_iAttacker) > Sqr(pAI->m_CoverTargetMaxRange))||
		(pAI->SqrDistanceToUs(m_iAttacker) < Sqr(pAI->m_CoverTargetMinRange)))
	{
		SetExpirationExpired();
		return;
	}

	// No guns
	pAI->m_DeviceWeapon.Free();
	pAI->m_DeviceWeapon.Lock(false);

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iAttacker);
	bool bFriendlyFire = (pTarget)&&(pTarget->GetCurRelation() < CAI_AgentInfo::HOSTILE);
	if ((pTarget)&&(!bFriendlyFire)&&(VALID_POS(m_Destination)))
	{
		pAI->ActivateAnimgraph();

		// Are we restricted?
		if (m_pAH->IsRestricted(m_Destination))
		{	
			m_Destination = CVec3Dfp32(_FP32_MAX);
			return;
		}

		SetExpiration(RoundToInt(pAI->m_Patience * 2.0f));

		// Is the scenepoint (if any) valid?
		bool IsAt = false;
		if (m_pScenePoint)
		{
			if ((!m_pScenePoint->InFrontArc(pTarget->GetBasePos()))||(!m_pAH->RequestScenePoint(m_pScenePoint)))
			{
				SetExpirationExpired();
				return;
			}

			if (m_pScenePoint->IsAt(pAI->GetBasePos()))
			{
				CVec3Dfp32 Dir = m_pScenePoint->GetDirection();
				if (m_pScenePoint->IsAligned(pAI->GetLookDir()))
				{
					IsAt = true;
					pAI->m_DeviceMove.Use();
				}
				else
				{
					// We free look to force WeaponAttackRanged to not aim at us anymore
					pAI->m_DeviceLook.Free();
					OnTrackDir(Dir,m_iAttacker,6);
				}
			}
		}
		else
		{	// Random point, we're at when within an arbitrary 32 units
			if (pAI->SqrDistanceToUs(m_Destination) < Sqr(32.0f))
			{
				pAI->m_DeviceMove.Use();
				IsAt = true;
			}
		}

		// Track the target wether we see him or not
		if (m_MoveMode == MODE_WALK)
		{
			OnTrackAgent(pTarget,m_iAttacker,6);
		}

		if (IsAt)
		{

#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
			SetExpirationExpired();
			SetExpirationIndefinite();
			if (m_StayTimeout == -1)
			{
				int StayTicks = 0;
				if (m_pScenePoint)
				{
					StayTicks = (int)(m_pScenePoint->GetBehaviourDuration() * pAI->GetAITicksPerSecond());
				}
				else
				{
					StayTicks = m_Duration;
				}
				m_StayTimeout = pAI->m_Timer + StayTicks;
				CWO_ScenePoint* orgSP = m_pScenePoint;
				m_pAH->ActivateScenePoint(m_pScenePoint,GetPriority());
				if (orgSP != m_pScenePoint) { return; }
				if (pTarget->GetCurAwareness() < CAI_AgentInfo::SPOTTED)
				{
					SpeakRandom(CAI_Device_Sound::COVER_TAUNT);
				}
			}
			if ((m_pScenePoint)&&(m_pScenePoint->GetBehaviour())&&(m_pScenePoint->GetBehaviourDuration() == 0.0f))
			{
				m_StayTimeout = pAI->m_Timer + pAI->GetAITicksPerSecond();
			}
			if (m_StayTimeout > pAI->m_Timer)
			{
				SetExpirationExpired();
				return;
			}
			// No more walking neccessary
			pAI->m_DeviceMove.Use();
			return;
		}
		else
		{	// Not at
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif
		}

		int Awareness = pTarget->GetAwareness();
		if (Awareness == CAI_AgentInfo::SPOTTED)
		{
			SpeakRandom(CAI_Device_Sound::COMBAT_SPOTTED,true);
		}
		if ((m_pScenePoint)&&(pAI->SqrDistanceToUs(m_Destination) < Sqr(32.0f)))
		{
			CVec3Dfp32 Dir = m_pScenePoint->GetDirection();
			// We free look to force WeaponAttackRanged to not aim at us anymore
			pAI->m_DeviceLook.Free();
			OnTrackDir(Dir,m_iAttacker,6);
		}
		int32 moveResult;
		if ((m_MoveMode == MODE_WALK)||(pAI->SqrDistanceToUs(m_Destination) < Sqr(32.0f)))
		{
			moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,true,false,NULL);
		}
		else
		{
			moveResult = pAI->OnMove(m_Destination,pAI->m_ReleaseMaxSpeed,true,false,NULL);
		}
		if (!pAI->CheckMoveresult(moveResult,m_pScenePoint))
		{
			m_pScenePoint = NULL;
			INVALIDATE_POS(m_Destination);
			return;
		}
	}
	else
	{	// No target or no destination, only thing to do is crouch/dodge if allowed
		// If crouch allowed there is a 25% chance we crouch (or 100% if friendly fire or neither left nor right allowed)
		if ((m_Flags & FLAGS_CROUCH)&&
			((bFriendlyFire)||(m_bCrouch)||(Random < 0.25)||!(m_Flags & (FLAGS_LEFT |FLAGS_RIGHT))))
		{
			if (!m_bCrouch)
			{
				pAI->m_DeviceStance.Crouch(true);
				m_bCrouch = true;
			}
			SetExpiration(20,40);
		}
		else if ((!m_bCrouch)&&(m_Flags & (FLAGS_LEFT | FLAGS_RIGHT)))
		{
			CVec3Dfp32 NoseDir,Move;
			NoseDir = CVec3Dfp32::GetRow(pAI->m_pGameObject->GetPositionMatrix(),0);
			NoseDir[2] = 0.0f;
			Move = CVec3Dfp32(0.0f);
			CVec3Dfp32 OurPos = pAI->GetBasePos();
			int Direction = 0;	// 0=Crouch,-1=left,1=right
			if ((m_Flags & FLAGS_LEFT)&&((Random < 0.5)||!(m_Flags & FLAGS_RIGHT)))
			{	// Left?
				Move[0] = -NoseDir[1];
				Move[1] = NoseDir[0];
				Move[2] = NoseDir[2];
				Direction = -1;
			}
			else
			{	// Right?
				Move[0] = NoseDir[1];
				Move[1] = -NoseDir[0];
				Move[2] = NoseDir[2];
				Direction = 1;
			}
			Move.Normalize();
			Move = OurPos + Move * 48.0f;
			// Is the coast clear for 1.5m? If not, we duck
			if (pAI->m_PathFinder.IsValid())
			{
				CVec3Dfp32 StopPos;
				if ((pAI->m_PathFinder.GetBlockNav())&&
					(!pAI->m_PathFinder.GetBlockNav()->IsGroundTraversable(OurPos,Move,
					(int)pAI->GetBaseSize(),
					(int)pAI->GetHeight(),
					pAI->m_bWallClimb,
					(int)pAI->GetStepHeight(),
					(int)pAI->GetJumpHeight(),
					&StopPos)))
				{
					Move = StopPos;
					Direction = 0;
				}
			}

			// Roll or move?
			bool bRoll = (pAI->m_Skill >= Random * 100);
			if (Direction == 0)
			{	// Duck if allowed
				if ((m_Flags & FLAGS_CROUCH)&&(!m_bCrouch))
				{
					if (!m_bCrouch)
					{
						pAI->m_DeviceStance.Crouch(true);
						m_bCrouch = true;
					}
					SetExpiration(20,40);
					return;
				}
			}
			else if (Direction == 1)	// Right
			{
				if (bRoll)
				{
					if (pAI->m_DeviceStance.IsCrouching())
					{
						SetWantedBehaviour(CAI_Core::BEHAVIOUR_NBR_CROUCH_ROLL_RIGHT,-1,NULL);
					}
					else
					{
						SetWantedBehaviour(CAI_Core::BEHAVIOUR_NBR_ROLL_RIGHT,-1,NULL);
					}
				}
				else
				{
					SetWantedBehaviour(CAI_Core::BEHAVIOUR_NBR_DODGE_RIGHT,-1,NULL);
				}
			}
			else	// Direction == -1 ie left
			{
				if (bRoll)
				{
					if (pAI->m_DeviceStance.IsCrouching())
					{
						SetWantedBehaviour(CAI_Core::BEHAVIOUR_NBR_CROUCH_ROLL_LEFT,-1,NULL);
					}
					else
					{
						SetWantedBehaviour(CAI_Core::BEHAVIOUR_NBR_ROLL_LEFT,-1,NULL);
					}
				}
				else
				{
					SetWantedBehaviour(CAI_Core::BEHAVIOUR_NBR_DODGE_LEFT,-1,NULL);
				}
			}
			SetExpirationExpired();
		}
		else
		{	// No crouch/dodge, expire!
			SetExpirationExpired();
		}
	}
};

void CAI_Action_Cover::OnStart()
{
	CAI_Action::OnStart();
	CAI_Core* pAI = AI();

	m_bTaken = false;
	m_bCrouch = false;
	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_pScenePoint = NULL;
	m_StayTimeout = -1;
	m_iAttacker = 0;

	// Find the attacker
	if (m_pAH->FiredAt(&m_iAttacker,DAMAGETYPE_TRANQUILLIZER,true) == CAI_ActionHandler::NONE)
	{
		SetExpirationExpired();
		return;
	}

	CAI_Core* pAttackerAI = pAI->GetAI(m_iAttacker);
	if ((pAttackerAI)&&(pAttackerAI->m_pGameObject))
	{	// Look at the bastard!
		OnTrackObj(m_iAttacker,10);
		CVec3Dfp32 TargetPos = pAttackerAI->GetBasePos();
		// Find a destination (or terminate)
		if ((m_Flags & FLAGS_SCENEPOINTS)&&(Random < 0.75f))
		{
			FindScenePointDestination(TargetPos);
			CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iAttacker);
			if ((VALID_POS(m_Destination))&&
				(pTarget)&&(pTarget->GetCurRelation() >= CAI_AgentInfo::ENEMY))
			{
				SpeakRandom(CAI_Device_Sound::ENEMY_RETREAT);
			}
		}
		if ((VALID_POS(m_Destination))||(m_Flags & (FLAGS_CROUCH | FLAGS_LEFT | FLAGS_RIGHT)))
		{
			m_bTaken = true;
		}
		if (VALID_POS(m_Destination))
		{
			// Inform our beloved leader
			if (!m_pAH->HasAction(COMBATLEADER))
			{
				int iLeader = m_pAH->GetClosestActionUser(COMBATLEADER);
				CAI_Core* pAI_Leader = pAI->GetAI(iLeader);
				if (pAI_Leader)
				{
					CAI_Action* pAction;
					if (pAI_Leader->m_AH.GetAction(COMBATLEADER,&pAction))
					{
						CAI_Action_CombatLeader* pLeaderAction = safe_cast<CAI_Action_CombatLeader>(pAction);
						pLeaderAction->ReceiveDecision(pAI->m_pGameObject->m_iObject,m_iAttacker,m_Destination,CAI_Action_CombatLeader::PARAM_BEHAVIOURNBR_COVER);
					}
				}
			}
			// Stop firing the gun
			CAI_Action* pAction;
			if (m_pAH->GetAction(WEAPON_ATTACK_RANGED,&pAction))
			{
				pAction->ExpireWithDelay(pAI->m_Patience);
			}
		}
	}
};

void CAI_Action_Cover::OnQuit()
{
	m_iAttacker = 0;
	m_bTaken = false;
	if (m_bCrouch)
	{
		AI()->m_DeviceStance.Crouch(false);
		m_bCrouch = false;
	}
	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_pScenePoint = NULL;
	CAI_Action::OnQuit();
};

CAI_Action_CombatLeader::CAI_Action_CombatLeader(CAI_ActionHandler * _pAH, int _Priority)
	: CAI_Action(_pAH, _Priority)
{
	m_Type = COMBATLEADER;
	m_TypeFlags = TYPE_MOVE | TYPE_SOUND | TYPE_OFFENSIVE;

	m_iBehaviourClose = 0;
	m_iBehaviourFwd = 0;
	m_iBehaviourFlank = 0;
	m_iBehaviourRear = 0;
	m_iBehaviourCover = 0;

	m_iOrderToIssue = 0;
	m_iCaller = 0;
	m_iTarget = 0;
	INVALIDATE_POS(m_MovePos);
};

int CAI_Action_CombatLeader::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int nAdded = 0;
	if (m_iBehaviourClose)
	{
		nAdded += AddUnique(_liBehaviours,m_iBehaviourClose);
	}
	if (m_iBehaviourFwd)
	{
		nAdded += AddUnique(_liBehaviours,m_iBehaviourFwd);
	}
	if (m_iBehaviourFlank)
	{
		nAdded += AddUnique(_liBehaviours,m_iBehaviourFlank);
	}
	if (m_iBehaviourRear)
	{
		nAdded += AddUnique(_liBehaviours,m_iBehaviourRear);
	}
	if (m_iBehaviourCover)
	{
		nAdded += AddUnique(_liBehaviours,m_iBehaviourCover);
	}
	return(nAdded);
};

int CAI_Action_CombatLeader::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "CLOSE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_BEHAVIOURNBR_CLOSE;
	}
	else if (_Str == "FORWARD")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_BEHAVIOURNBR_FWD;
	}
	else if (_Str == "FLANK")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_BEHAVIOURNBR_FLANK;
	}
	else if (_Str == "REAR")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_BEHAVIOURNBR_REAR;
	}
	else if (_Str == "COVER")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_BEHAVIOURNBR_COVER;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

void CAI_Action_CombatLeader::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_BEHAVIOURNBR_CLOSE:
		m_iBehaviourClose = _Val;
		break;
	case PARAM_BEHAVIOURNBR_FWD:
		m_iBehaviourFwd = _Val;
		break;
	case PARAM_BEHAVIOURNBR_FLANK:
		m_iBehaviourFlank = _Val;
		break;
	case PARAM_BEHAVIOURNBR_REAR:
		m_iBehaviourRear = _Val;
		break;
	case PARAM_BEHAVIOURNBR_COVER:
		m_iBehaviourCover = _Val;
		break;
	default:
		CAI_Action::SetParameter(_Param,_Val);
		break;
	}
};

// _iCaller made a decision that we might issue the order for
// Returns true if we will issue the "order"
bool CAI_Action_CombatLeader::ReceiveDecision(int _iCaller,int _iTarget,CVec3Dfp32 _Pos,int _Type)
{
	CAI_Core* pAI = AI();

	if (!m_iOrderToIssue)
	{
		if (_Type == PARAM_BEHAVIOURNBR_GENERIC)
		{	// We only issue orders when the move would get _iCaller closer to _iTarget
			CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(_iTarget);
			CAI_AgentInfo* pCaller = pAI->m_KB.GetAgentInfo(_iCaller);
			if ((pTarget)&&(pCaller))
			{
				CVec3Dfp32 TargetPos = pTarget->GetBasePos();
				CVec3Dfp32 CallerPos = pCaller->GetBasePos();
				fp32 OldRangeSqr = CallerPos.DistanceSqr(TargetPos);
				fp32 CurRangeSqr = _Pos.DistanceSqr(TargetPos);
				if (OldRangeSqr <= CurRangeSqr)
				{
					return(false);
				}
			}
			else
			{
				return(false);
			}
		}

		m_iOrderToIssue = _Type;
		m_iCaller = _iCaller;
		m_iTarget = _iTarget;
		m_MovePos = _Pos;
		
		return(true);
	}

	return(false);
};


int CAI_Action_CombatLeader::MandatoryDevices()
{
	if (m_iBehaviourClose || m_iBehaviourFwd || m_iBehaviourFlank || m_iBehaviourRear || m_iBehaviourCover)
	{
		return (1 << CAI_Device::LOOK) | (1 << CAI_Device::MOVE) | (1 << CAI_Device::SOUND);
	}
	else
	{
		return (1 << CAI_Device::LOOK) | (1 << CAI_Device::SOUND);
	}
};


bool CAI_Action_CombatLeader::IsValid()
{
	if (!CAI_Action::IsValid())
	{
		return(false);
	}
	else
	{
		if ((m_iOrderToIssue == 0)||(m_iCaller == 0))
		{
			return(false);
		}
	}

	return(true);
};

CAI_ActionEstimate CAI_Action_CombatLeader::GetEstimate()
{
	CAI_ActionEstimate Est;
	AI();

	if (IsValid())
	{	//Default score 70
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 30);
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 30);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 0);
		Est.Set(CAI_ActionEstimate::LOYALTY, 10);
		Est.Set(CAI_ActionEstimate::LAZINESS, 0);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 0);
	}

	return Est;
};

void CAI_Action_CombatLeader::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_CombatLeader::OnTakeAction);
	CAI_Core* pAI = AI();

	CAI_AgentInfo* pTarget = AI()->m_KB.GetAgentInfo(m_iTarget);
	if (!pTarget)
	{
		SetExpirationExpired();
		return;
	}

	pAI->ActivateAnimgraph();
	SetExpiration(pAI->m_Patience);

	if (m_iOrderToIssue == PARAM_BEHAVIOURNBR_GENERIC)
	{	// We must determine what order to issue ourselves
		CAI_Core* pTargetAI = pAI->GetAI(m_iTarget);
		if (!pTargetAI)
		{
			m_iTarget = 0;
			m_iCaller = 0;
			m_iOrderToIssue = 0;
			INVALIDATE_POS(m_MovePos);
			return;
		}
		fp32 Angle = M_Fabs(pTargetAI->AngleOff(m_MovePos,true));
		if (Angle <= 0.125f)
		{	// Forward or close
			if (pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)
			{	// WE see hin, charge ahead!!
				m_iOrderToIssue = PARAM_BEHAVIOURNBR_CLOSE;
			}
			else
			{	// He's there sneak up on him
				m_iOrderToIssue = PARAM_BEHAVIOURNBR_FWD;
			}
		}
		else if (Angle >= 0.375f)
		{	// Ambush
			m_iOrderToIssue = PARAM_BEHAVIOURNBR_REAR;
		}
		else
		{	// Flank
			m_iOrderToIssue = PARAM_BEHAVIOURNBR_FLANK;
		}
	}

	// We don't issue the order until we look at the target pos (the target)
	CVec3Dfp32 Pos = pTarget->GetSuspectedPosition();
	if (M_Fabs(pAI->AngleOff(Pos,true)) > 0.05f)
	{
		OnTrackAgent(pTarget,6);
		return;
	}

	int iVariant = -1;
	int SoundDuration = 0;
	switch(m_iOrderToIssue)
	{
	case PARAM_BEHAVIOURNBR_CLOSE:
		iVariant = SpeakRandom(CAI_Device_Sound::LEADER_COMBAT_FWD);
		SoundDuration = pAI->m_DeviceSound.GetDuration(CAI_Device_Sound::LEADER_COMBAT_FWD,iVariant);
		if (m_iBehaviourClose)
		{
			SetWantedBehaviour(m_iBehaviourClose,-1,NULL);
		}
		break;
	case PARAM_BEHAVIOURNBR_FWD:
		iVariant = SpeakRandom(CAI_Device_Sound::LEADER_COMBAT_FWD);
		SoundDuration = pAI->m_DeviceSound.GetDuration(CAI_Device_Sound::LEADER_COMBAT_FWD,iVariant);
		if (m_iBehaviourFwd)
		{
			SetWantedBehaviour(m_iBehaviourFwd,-1,NULL);
		}
		break;
	case PARAM_BEHAVIOURNBR_FLANK:
		iVariant = SpeakRandom(CAI_Device_Sound::LEADER_COMBAT_FLANK);
		SoundDuration = pAI->m_DeviceSound.GetDuration(CAI_Device_Sound::LEADER_COMBAT_FLANK,iVariant);
		if (m_iBehaviourFlank)
		{
			SetWantedBehaviour(m_iBehaviourFlank,-1,NULL);
		}
		break;
	case PARAM_BEHAVIOURNBR_REAR:
		iVariant = SpeakRandom(CAI_Device_Sound::LEADER_COMBAT_REAR);
		SoundDuration = pAI->m_DeviceSound.GetDuration(CAI_Device_Sound::LEADER_COMBAT_REAR,iVariant);
		if (m_iBehaviourRear)
		{
			SetWantedBehaviour(m_iBehaviourRear,-1,NULL);
		}
		break;
	case PARAM_BEHAVIOURNBR_COVER:
		iVariant = SpeakRandom(CAI_Device_Sound::LEADER_COMBAT_COVER);
		SoundDuration = pAI->m_DeviceSound.GetDuration(CAI_Device_Sound::LEADER_COMBAT_COVER,iVariant);
		if (m_iBehaviourCover)
		{
			SetWantedBehaviour(m_iBehaviourCover,-1,NULL);
		}
		break;
	default:
		SetExpirationExpired();
		break;
	}
	if ((m_iCaller)&&(iVariant >= 0)&&(SoundDuration))
	{
		CAI_Core* pReceiver = pAI->GetAI(m_iCaller);
		if (pReceiver)
		{
			pReceiver->UseRandomDelayed(CStr("Leader response"),CAI_Device_Sound::COMBAT_AFFIRMATIVE,GetPriority(),SoundDuration);
		}
	}

	SetExpirationExpired();
};

void CAI_Action_CombatLeader::OnStart()
{
	CAI_Action::OnStart();
};

void CAI_Action_CombatLeader::OnQuit()
{
	m_iOrderToIssue = 0;
	m_iCaller = 0;
	m_iTarget = 0;
	INVALIDATE_POS(m_MovePos);
	CAI_Action::OnQuit();
};

//CAI_Action_DarklingAttack////////////////////////////////////////////////////////////////////////////
CAI_Action_DarklingAttack::CAI_Action_DarklingAttack(CAI_ActionHandler* _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = DARKLING_ATTACK;
	m_TypeFlags = TYPE_MOVE | TYPE_OFFENSIVE;

	m_iTarget = 0;
	m_bGentleman = false;
};

void CAI_Action_DarklingAttack::SetParameter(int _Param, fp32 _Val)
{
	if (!AI())
		return;

	// TODO: Add switch statement when we got params
	CAI_Action::SetParameter(_Param, _Val);
};

void CAI_Action_DarklingAttack::SetParameter(int _Param, int _Val)
{
	if (!AI())
		return;

	// TODO: Add switch statement when we got actions
	CAI_Action::SetParameter(_Param, _Val);

};

void CAI_Action_DarklingAttack::SetParameter(int _Param, CStr _Str)
{
	if (!AI())
		return;

	// TODO: Add switch statement when we got actions
	CAI_Action::SetParameter(_Param, _Str);

};

//Get parameter ID from string (used when parsing registry)
int CAI_Action_DarklingAttack::StrToParam(CStr _Str, int* _pResType)
{
	// TODO: Add switch statement when we got actions
	int rResult = CAI_Action::StrToParam(_Str, _pResType);
	return(rResult);
};

//Mandatory: Weapon
int CAI_Action_DarklingAttack::MandatoryDevices()
{
	return (1 << CAI_Device::MELEE) | (1 << CAI_Device::MOVE) | 
		(1 << CAI_Device::WEAPON) | (1 << CAI_Device::ITEM);
};

//Valid if at least one action is of TYPE_OFFENSIVE
bool CAI_Action_DarklingAttack::IsValid()
{
	MAUTOSTRIP(CAI_Action_DarklingAttack_IsValid, false);

	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		return(false);
	}

	if (m_iTarget)
	{
		CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
		if (!pTarget)
		{
			pTarget = pAI->m_KB.AddAgent(m_iTarget);
		}
		if (IsValid(pTarget))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
	else
	{
		return(true);
	}
};

bool CAI_Action_DarklingAttack::IsValid(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();
	if (_pTarget)
	{
		if (_pTarget->GetCurAwareness() < CAI_AgentInfo::DETECTED)
		{
			return(false);
		}
		if (_pTarget->GetCurRelation() < CAI_AgentInfo::ENEMY)
		{
			return(false);
		}
		if (pAI->SqrDistanceToUs(_pTarget->GetObjectID()) > Sqr(pAI->m_MeleeMaxRange))
		{
			return false;
		}
		if (!pAI->FastCheckLOS(_pTarget->GetObjectID()))
		{
			return false;
		}
		return(true);
	}
	else
	{
		return(false);
	}
};

CAI_ActionEstimate CAI_Action_DarklingAttack::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP(CAI_Action_DarklingAttack_GetEstimate, Est);

	CAI_Core* pAI = AI();
	if (IsValid())
	{
		CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
		if (!pTarget)
		{
			return Est;
		}
		if (pAI->SqrDistanceToUs(pTarget->GetObjectID()) > Sqr(pAI->m_MeleeMaxRange))
		{
			return Est;
		}
		if ((m_bGentleman)&&
			(pAI->m_pGameObject)&&(pAI->m_pServer)&&
			(pAI->m_pAIResources->ms_GameDifficulty < DIFFICULTYLEVEL_HARD))
		{
			if (!pAI->m_pAIResources->m_ActionGlobals.ms_MeleeFightSemaphore.Peek(pAI->m_pGameObject->m_iObject,1,pAI->GetGlobalAITick()))
			{
				return Est;
			}
		}
		if (!pAI->IsValidTarget(pTarget->GetObjectID()))
		{
			return Est;
		}

		//Default score 95
		m_iTarget = pTarget->GetObjectID();
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 60); 
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 10);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, -20);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 5);
	}
	return Est;
};

// Punch that guy dammit!
void CAI_Action_DarklingAttack::OnTakeAction()
{
	MAUTOSTRIP(CAI_Action_DarklingAttack__OnTakeAction, 0);
	MSCOPESHORT(CAI_Action_DarklingAttack::OnTakeAction);

	CAI_Core* pAI = AI();
	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if ((!pTarget)||(!IsValid(pTarget)))
	{
		SetExpirationExpired();
		return;
	}

	int Relation = pTarget->GetCurRelation();
	if (Relation < CAI_AgentInfo::ENEMY)
	{
		SetExpirationExpired();
		return;
	}

	CAI_Core* pTargetAI = pAI->GetAI(m_iTarget);
	if ((!pTargetAI)||(!pTargetAI->IsConscious()))
	{	// Target unconscious or lacks AI
		ExpireWithDelay((int)(3.0f * pAI->GetAITicksPerSecond()));
	}

	// This may take a while
	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();
	OnTrackAgent(pTarget,6,true);	// Heading only
	pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
	pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);

	CVec3Dfp32 LookDir = pAI->GetLookDir();
	CVec3Dfp32 Diff = pTargetAI->GetBasePos() - pAI->GetBasePos();
	CVec3Dfp32 Up = pAI->GetUp();
	Diff -= Up * (Diff * Up);
	LookDir -= Up * (LookDir * Up);
	Diff.Normalize();
	LookDir.Normalize();
	if (LookDir * Diff >= 0.0f)
	{	// Whack!
		// CAI_Weapon* pWeapon = pAI->m_Weapon.GetWielded();
		if (pAI->m_Timer % 2 == 0)
		{
			pAI->m_DeviceItem.Use();
		}
		else
		{
			pAI->m_DeviceItem.Lock();
		}
		
		/*
		if (pWeapon->GetType() == CAI_Weapon::RANGED_RAPIDFIRE)
		{
			pAI->m_DeviceItem.Use();
		}
		else
		{
			pAI->m_DeviceWeapon.UsePeriodic();
		}
		*/
	}
	pAI->m_DeviceMove.Use();
};

void CAI_Action_DarklingAttack::OnStart()
{
	CAI_Core* pAI = AI();
	CAI_Action::OnStart();

	if ((pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)&&(MRTC_ISKINDOF(pAI,CAI_Core_Darkling)))
	{
		CAI_Core_Darkling* pDK = safe_cast<CAI_Core_Darkling>(pAI);
		if ((pDK)&&(pDK->m_DK_Special == CAI_Core_Darkling::DK_SPECIAL_GUNNER)&&(pAI->GetUp() * CVec3Dfp32(0.0f,0.0f,1.0f) >= 0.98f))
		{
			pAI->m_DeviceStance.SetTargetInFOV(true);
			pAI->m_DeviceStance.Free();
			pAI->m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
			pAI->m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
			pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		}
	}
};

void CAI_Action_DarklingAttack::OnQuit()
{
	CAI_Core* pAI = AI();

	if ((pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)&&(MRTC_ISKINDOF(pAI,CAI_Core_Darkling)))
	{
		CAI_Core_Darkling* pDK = safe_cast<CAI_Core_Darkling>(pAI);
		if ((pDK)&&(pDK->m_DK_Special == CAI_Core_Darkling::DK_SPECIAL_GUNNER)&&(pAI->GetUp() * CVec3Dfp32(0.0f,0.0f,1.0f) >= 0.98f))
		{
			pAI->m_DeviceStance.Free();
			pAI->m_DeviceStance.SetMinStance(CAI_Device_Stance::IDLESTANCE_IDLE);
			pAI->m_DeviceStance.SetMaxStance(CAI_Device_Stance::IDLESTANCE_IDLE);
		}
	}

	m_iTarget = 0;
	CAI_Action::OnQuit();
};

//CAI_Action_DarklingClose////////////////////////////////////////////////////////////////////////////
const char* CAI_Action_DarklingClose::ms_lStrFlags[] = 
{	
	"PATHFIND",
	"RUN",
	"SCENEPTS",
	"RANDOMPTS",
	"CLOSEST",
	"NEARPLAYER",
	"PLAYERFOV",
	"HEADING",
	"AVOIDLIGHT",
	"PREFERLIGHT",
	"SYNC",
	"SCARECIVS",
	
	NULL,
};

CAI_Action_DarklingClose::CAI_Action_DarklingClose(CAI_ActionHandler* _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = DARKLING_CLOSE;
	m_TypeFlags = TYPE_MOVE | TYPE_OFFENSIVE;

	m_iTarget = 0;
	m_MoveMode = MODE_DEFAULT;
	m_bGentleman = false;
	m_NextScareTimer = 0;
	m_Flags = 0;

	// TODO: SetParams for these...
	m_AttackBehaviour = 0;
	m_AttackBehaviourLong = 0;
	m_AttackRange = 0.0f;
	m_AttackRangeLong = 0.0f;

	m_AttackJumpBehaviour = 0;
	m_AttackJumpBehaviour2 = 0;

	m_iAttackTargetKilled = 0;
	m_AnimEventEffect = "";
	m_bJumping = false;

	INVALIDATE_POS(m_Destination);
	m_StayTimeout = 0;
	m_pScenePoint = NULL;
};

void CAI_Action_DarklingClose::SetParameter(int _Param, CStr _Str)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_MOVEMODE:
		SetParameter(_Param, _Str.Val_int());
		break;
	case PARAM_GENTLEMAN:
		SetParameter(_Param, _Str.Val_int());
		break;
	case PARAM_FLAGS:
		SetParameter(_Param, StrToFlags(_Str));
		break;
	case PARAM_ATTACK_BEHAVIOUR:
	case PARAM_ATTACK_BEHAVIOUR_LONG:
	case PARAM_JUMP_BEHAVIOUR:
	case PARAM_JUMP_BEHAVIOUR2:
		SetParameter(_Param, _Str.Val_int());
		break;
	case PARAM_EFFECT:
		m_AnimEventEffect = _Str;
		break;
	default:
		CAI_Action::SetParameter(_Param, _Str);
	}
}


void CAI_Action_DarklingClose::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_ATTACK_RANGE:
		{
			m_AttackRange = _Val;
		}
		break;
	case PARAM_ATTACK_RANGE_LONG:
		{
			m_AttackRangeLong = _Val;
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
		break;
	}
};

void CAI_Action_DarklingClose::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_MOVEMODE:
		if ((_Val == MODE_WALK)||(_Val == MODE_RUN))
		{
			m_MoveMode = _Val;
		}
		else
		{
			m_MoveMode = MODE_DEFAULT;
		}
		break;
	case PARAM_GENTLEMAN:
		if (_Val)
		{
			m_bGentleman = true;
		}
		else
		{
			m_bGentleman = false;
		}
		break;
	case PARAM_FLAGS:
		m_Flags = _Val;
		break;
	case PARAM_ATTACK_BEHAVIOUR:
		m_AttackBehaviour = _Val;
		break;
	case PARAM_ATTACK_BEHAVIOUR_LONG:
		m_AttackBehaviourLong = _Val;
		break;
	case PARAM_JUMP_BEHAVIOUR:
		m_AttackJumpBehaviour = _Val;
		break;
	case PARAM_JUMP_BEHAVIOUR2:
		m_AttackJumpBehaviour2 = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get parameter ID from string (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_DarklingClose::StrToParam(CStr _Str, int* _pResType)
{
	MAUTOSTRIP(CAI_Action_Close_StrToParam, 0);
	if (_Str == "MOVEMODE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_MOVEMODE;
	}
	else if (_Str == "GENTLEMAN")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_GENTLEMAN;
	}
	else if (_Str == "ATTACKBEHAVIOUR")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_ATTACK_BEHAVIOUR;
	}
	else if (_Str == "ATTACKRANGE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_ATTACK_RANGE;
	}
	else if (_Str == "ATTACKBEHAVIOURLONG")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_ATTACK_BEHAVIOUR_LONG;
	}
	else if (_Str == "ATTACKRANGELONG")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_ATTACK_RANGE_LONG;
	}
	else if (_Str == "JUMPBEHAVIOUR")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_JUMP_BEHAVIOUR;
	}
	else if (_Str == "JUMPBEHAVIOUR2")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_JUMP_BEHAVIOUR2;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else if (_Str == "EFFECT")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_EFFECT;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

int CAI_Action_DarklingClose::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

//Mandatory: Move; Optional: Look
int CAI_Action_DarklingClose::MandatoryDevices()
{
	MAUTOSTRIP(CAI_Action_DarklingClose_MandatoryDevices, 0);
	return (1 << CAI_Device::MOVE);
};

bool CAI_Action_DarklingClose::IsValid()
{
	MAUTOSTRIP(CAI_Action_DarklingClose_IsValid, false);

	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		return(false);
	}

	// We're valid until we've moved away from corpse
	if (m_iAttackTargetKilled)
	{
		return(true);
	}

	if (m_iTarget != 0)
	{
		CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
		if (pTarget)
		{
			int Awareness = pTarget->GetCurAwareness();
			if (Awareness < CAI_AgentInfo::DETECTED)
			{
				m_iTarget = 0;
				return(false);
			}

			fp32 RangeSqr = AI()->SqrDistanceToUs(pTarget->GetBasePos());
			if (RangeSqr >  Sqr(AI()->m_CloseMaxRange))
			{
				m_iTarget = 0;
				return(false);
			}
			if (RangeSqr <  Sqr(AI()->m_CloseMinRange))
			{
				if (pAI->m_KB.GetDetectedEnemyCount() > 1)
				{	// Many enemies, use short attacks
					if (!m_AttackBehaviour)
					{
						m_iTarget = 0;
						return(false);
					}
				}
				else
				{	// One enemy, use long attack if available
					if ((!m_AttackBehaviour)&&(m_AttackBehaviourLong))
					{
						m_iTarget = 0;
						return(false);
					}
				}
			}
		}
		else
		{
			m_iTarget = 0;
			return(false);
		}
	}

	return(true);
};

//Good offensively. Bad defensively.
CAI_ActionEstimate CAI_Action_DarklingClose::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP(CAI_Action_DarklingClose__GetEstimate, Est);

	CAI_Core* pAI = AI();
	if (IsValid())
	{
		CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
		if (!pTarget)
		{
			return Est;
		}

		if ((m_bGentleman)&&
			(pAI->SqrDistanceToUs(pTarget->GetObjectID()) <= Sqr(64))&&
			(pAI->m_pGameObject)&&(pAI->m_pServer)&&
			(pAI->m_pAIResources->ms_GameDifficulty < DIFFICULTYLEVEL_HARD))
		{
			if (!pAI->m_pAIResources->m_ActionGlobals.ms_MeleeFightSemaphore.Peek(pAI->m_pGameObject->m_iObject,1,pAI->GetGlobalAITick()))
			{
				return Est;
			}
		}
		if (!pAI->IsValidTarget(pTarget->GetObjectID()))
		{
			return Est;
		}

		//Default score 95
		m_iTarget = pTarget->GetObjectID();
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 60); 
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 10);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, -20);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 5);
	}
	return Est;
};

int CAI_Action_DarklingClose::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int nAdded = 0;
	if (m_AttackBehaviour)
	{
		nAdded += AddUnique(_liBehaviours,m_AttackBehaviour);
	}
	if (m_AttackBehaviourLong)
	{
		nAdded += AddUnique(_liBehaviours,m_AttackBehaviourLong);
	}
	if (m_AttackJumpBehaviour)
	{
		nAdded += AddUnique(_liBehaviours,m_AttackJumpBehaviour);
	}
	if (m_AttackJumpBehaviour2)
	{
		nAdded += AddUnique(_liBehaviours,m_AttackJumpBehaviour2);
	}
	if (m_Flags & FLAGS_SCARE_CIVS)
	{
		nAdded += CAI_Action::AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_DARKLING_SCARE);
		nAdded += CAI_Action::AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_DARKLING_SCARE2);
	}

	if (m_AnimEventEffect.Len() > 0)
	{	// Precache effect
		m_pAH->PrecacheEffect(m_AnimEventEffect);
	}
	return(nAdded);
};

int CAI_Action_DarklingClose::GetUsedAcs(TArray<int16>& _liAcs) const
{
	int nAdded = 0;
	return(nAdded);
};

bool CAI_Action_DarklingClose::AnimationEvent(int _iUser, int _iEvent)
{
	CAI_Core* pAI = AI();
	if ((m_AnimEventEffect.Len() > 0)&&(_iEvent == 1))
	{	// Kablooey!
		CMat4Dfp32 Mat;
		pAI->GetBaseMat(Mat);
		int iObj = pAI->m_pServer->Object_Create(m_AnimEventEffect,Mat);
		// And then die!
		int OrgMinHealth = pAI->m_MinHealth;
		pAI->m_MinHealth = 0;
		pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_ADDHEALTH,-2 * pAI->MaxHealth()));				
		pAI->OnTakeDamage(pAI->GetObjectID(),pAI->MaxHealth(),DAMAGETYPE_UNDEFINED);
		pAI->m_MinHealth = OrgMinHealth;
		return(true);
	}
	return(false);
};

bool CAI_Action_DarklingClose::DoJumpAttackBehaviour(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();

	if (m_bJumping == true)
	{
		return(true);
	}

	if ((!m_AttackBehaviour)&&(!m_AttackBehaviourLong))
	{
		return(false);
	}

	if ((!_pTarget)||(!m_pAH->IsValidTarget(_pTarget->GetObjectID())))
	{
		return(false);
	}

	if (pAI->IsPlayer(_pTarget->GetObjectID()))
	{
		return(false);
	}

	CAI_Core* pTargetAI = pAI->GetAI(_pTarget->GetObjectID());
	if (pTargetAI)
	{
		// Get target headmat and switcharound
		CVec3Dfp32 TargetPos = pTargetAI->GetHeadPos();
		CMat4Dfp32 TargetHeadMat;
		pTargetAI->GetBaseMat(TargetHeadMat);
		TargetHeadMat.GetRow(0) = -TargetHeadMat.GetRow(0);
		TargetHeadMat.GetRow(1) = -TargetHeadMat.GetRow(1);
		TargetHeadMat.GetRow(3) = TargetPos;
		CVec3Dfp32 OurPos = pAI->GetBasePos();
		CVec3Dfp32 Diff = TargetPos - OurPos;
		fp32 RangeSqr = Diff.LengthSqr();
		if ((RangeSqr >= Sqr(pAI->m_JumpMinRange))&&(RangeSqr <= Sqr(pAI->m_JumpMaxRange)))
		{
			if (pAI->AddJumpTowardsPositionCmd(&TargetHeadMat,CAI_Core::JUMPREASON_COMBAT,CAI_Core::JUMP_CHECK_COLL | CAI_Core::JUMP_TO_AIR))
			{
				m_bJumping = true;
				return(true);
			}
		}
	}
	return(false);
};

bool CAI_Action_DarklingClose::DoJumpLandBehaviour(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();

	if (m_bJumping)
	{
		pAI->m_DeviceWeapon.Use();
		return(true);
	}
	else
	{
		return(false);
	}
};

bool CAI_Action_DarklingClose::DoAttackBehaviour(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();
	
	if ((!m_AttackBehaviour)&&(!m_AttackBehaviourLong))
	{
		return(false);
	}

	if ((!_pTarget)||(!m_pAH->IsValidTarget(_pTarget->GetObjectID())))
	{
		return(false);
	}

	CVec3Dfp32 TargetPos = _pTarget->GetCorrectPosition();
	CAI_Core* pTargetAI = pAI->GetAI(_pTarget->GetObjectID());
	if (!pTargetAI)
	{
		return(false);
	}

	if ((pTargetAI->m_CharacterClass != CAI_Core::CLASS_BADGUY)&&(pTargetAI->m_CharacterClass != CAI_Core::CLASS_UNDEAD))
	{	// Only badguys can be properly berserked
		return(false);
	}

	CMat4Dfp32 OurMat;
	pAI->GetBaseMat(OurMat);
	// Reverse fix for darkling
	if (pAI->m_CharacterClass == CAI_Core::CLASS_DARKLING)
	{	// Drop base mat
		OurMat.GetRow(3) -= pAI->GetUp() * DARKLING_PF_OFFSET;
	}
	CVec3Dfp32 OurPos = OurMat.GetRow(3);
	CVec3Dfp32 Diff = TargetPos - OurPos;
	if ((m_Flags & FLAGS_SYNC)&&(Abs(Diff[2]) >= 4.0f))
	{
		return(false);
	}
	fp32 RangeSqr = Diff.LengthSqr();
	int32 iBehaviour = m_AttackBehaviour;
	fp32 AttackRange = m_AttackRange;
	if ((RangeSqr > Sqr(m_AttackRangeLong-8.0f))&&(m_AttackBehaviourLong)&&(pAI->m_KB.GetDetectedEnemyCount() <= 1))
	{
		iBehaviour = m_AttackBehaviourLong;
		AttackRange = m_AttackRangeLong;
	}
	if ((!iBehaviour)||(AttackRange <= 0.0f))
	{
		return(false);
	}

	if ((m_Flags & FLAGS_SYNC)&&(RangeSqr <= Sqr(AttackRange+32.0f))&&(RangeSqr >= Sqr(AttackRange+8.0f)))
	{
		if (pTargetAI->m_bBehaviourRunning)
		{
			pTargetAI->StopBehaviour(CAI_Core::BEHAVIOUR_STOP_FAST,GetPriority());
			return(false);
		}
		pTargetAI->OnTrackObj(pAI->GetObjectID(),10,false,false);
		// We cannot Berserk scripted characters
		if (pTargetAI->m_Script.IsValid())
		{
			return(false);
		}
	}


	if ((RangeSqr <= Sqr(AttackRange+8.0f))&&(RangeSqr >= Sqr(AttackRange-8.0f)))
	{	
		if (!pAI->m_pGameObject->AI_IsOnWall())
		{
			if ((m_Flags & FLAGS_SYNC)&&(pTargetAI->m_bBehaviourRunning))
			{
				return(false);
			}

			// Check LOS to target
			// We do an actual traceline here (from base of Darkling to head of target to worstcaseify it)
			if (!pAI->FastCheckLOS(OurPos,pTargetAI->GetHeadPos(),pAI->GetObjectID(),_pTarget->GetObjectID()))
			{
				return(false);
			}

			if ((m_Flags & FLAGS_SYNC)&&
				((!pAI->m_pAIResources->m_ActionGlobals.ms_BerserkerSemaphore.Peek(pAI->GetObjectID(),1,pAI->GetGlobalAITick()))))
			{
				return(false);
			}

			// Expensive box coll det
			if (m_Flags & FLAGS_SYNC)
			{
				CMat4Dfp32 DestMat;
				pTargetAI->GetHeadMat(DestMat);
				DestMat.GetRow(3) += DestMat.GetRow(2) * 12.0f;
				const CVec3Dfp32& Pos = pAI->GetBasePos();
				CVec3Dfp32 JumpOffset = DestMat.GetRow(3) - Pos;
				fp32 Len = JumpOffset.Length();
				JumpOffset *= (1.0f / Len);
				if (Len > 512.0f)
					return(false);

				fp32 BoxWidth =  0.707f * (16.0f - 2.0f);
				fp32 BoxLenHalf = Len*0.5f - (16.0f + 2.0f);
				CWO_PhysicsState PhysState(pAI->m_pGameObject->GetPhysState());
				PhysState.m_PhysFlags = OBJECT_PHYSFLAGS_ROTATION;
				PhysState.m_nPrim = 1;
				PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(BoxLenHalf, BoxWidth, BoxWidth), CVec3Dfp32(0,0,0));

				CMat4Dfp32 Mat;
				Mat.GetRow(0) = JumpOffset;
				Mat.GetRow(2) = (JumpOffset.k[2] > 0.9f) ? CVec3Dfp32(1,0,0) : CVec3Dfp32(0,0,1);
				Mat.RecreateMatrix(0, 2);
				Mat.GetRow(3) = Pos + JumpOffset * (Len * 0.5f);
				if (pAI->m_pServer->Phys_IntersectWorld((CSelection *)NULL, PhysState, Mat, Mat, pAI->GetObjectID(), NULL))
				{
#ifndef M_RTM
					pAI->m_pServer->Debug_RenderOBB(Mat, PhysState.m_Prim[0].GetDim(), kColorRed, 5.0f, true);
#endif
					return(false);
				}
			}

			CMat4Dfp32 TargetMat;
			pTargetAI->GetBaseMat(TargetMat);
			CVec3Dfp32 Diff = (OurPos - TargetPos).Normalize();
			if (m_Flags & FLAGS_SYNC)
			{
				pTargetAI->OnTrackDir(Diff,pAI->GetObjectID(),6,true,false);
			}
			pAI->OnTrackDir(-Diff,pTargetAI->GetObjectID(),6,true,false);
			// TODO: Split the pos-discrepancy equally between us and target?
			Diff.SetRow(TargetMat,0);
			TargetMat.RecreateMatrix(2,0);
			(-Diff).SetRow(OurMat,0);
			OurPos = TargetMat.GetRow(3) + Diff * AttackRange;
			OurPos.SetRow(OurMat,3);
			OurMat.RecreateMatrix(2,0);
			pTargetAI->m_CurCB.Clear();
			pTargetAI->m_PrevCB.Clear();
			if (m_Flags & FLAGS_SYNC)
			{
				pTargetAI->m_Script.SetDelayedBehaviour(iBehaviour,pAI->GetObjectID(),0.9f,TargetMat);
			}
			// The other dude has his behaviour taken care of, let's do our
			pAI->m_CurCB.Clear();
			pAI->m_PrevCB.Clear();
			if (pAI->SetWantedBehaviour2(iBehaviour,
				CAI_Action::PRIO_FORCED,
				CAI_Core::BEHAVIOUR_FLAGS_PP,-1,
				NULL,&OurMat))
			{
				pTargetAI->ShutUp(CAI_Action::PRIO_FORCED);
				m_iAttackTargetKilled = _pTarget->GetObjectID();
				if (m_Flags & FLAGS_SYNC)
				{
					CWObject_Message Msg(OBJMSG_CHAR_IMMUNE);
					Msg.m_Param0 = 1;	// On
					pAI->m_pServer->Message_SendToObject(Msg,pAI->GetObjectID());
					pAI->m_pServer->Message_SendToObject(Msg,_pTarget->GetObjectID());
					pAI->m_pAIResources->m_ActionGlobals.ms_BerserkerSemaphore.Poll(pAI->GetObjectID(),1,pAI->GetGlobalAITick(),pAI->GetAITicksPerSecond() * 5);
				}
				SetExpirationExpired();
				return(true);
			}
			else
			{
				return(false);
			}
		}
	}

	return(false);
};

void CAI_Action_DarklingClose::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		INVALIDATE_POS(m_Destination);
		m_StayTimeout = -1;
		m_pScenePoint = NULL;
	}
};

//Fetch boy, fetch!
void CAI_Action_DarklingClose::OnTakeAction()
{
	MAUTOSTRIP(CAI_Action_DarklingClose__OnTakeAction, MAUTOSTRIP_VOID);
	MSCOPESHORT(CAI_Action_DarklingClose::OnTakeAction);
	CAI_Core* pAI = AI();

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (!pTarget)
	{
		SetExpirationExpired();
		return;
	}

	// Check restriction
	// Yes we might be allowed to close to target without violating the restriction but what use would that be?
	if (m_pAH->IsRestricted(pTarget,true))
	{	
		if (pAI->DebugRender())
		{
			CStr Name = pAI->m_pGameObject->GetName();
			ConOut(Name+CStr(": Close restricted, delay 3s"));
		}
		pAI->OnTrackAgent(pTarget,10,false,false);
		ExpireWithDelay(60);
		return;
	}
	
	fp32 TargetRangeSqr = pAI->SqrDistanceToUs(pTarget->GetObjectID());
	// Range check target vs pAI->m_CloseMinRange and pAI->m_CloseMaxRange
	if ((TargetRangeSqr < Sqr(pAI->m_CloseMinRange))||(TargetRangeSqr > Sqr(pAI->m_CloseMaxRange)))
	{
		SetExpirationExpired();
		return;
	}

	pAI->ActivateAnimgraph();
	SetExpiration((int)(pAI->m_Patience * 3.0f));

	// Move towards target according to flags and awareness
	pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
	pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
	if ((m_bGentleman)&&(TargetRangeSqr <= Sqr(64))&&(pAI->m_pGameObject)&&(pAI->m_pServer))
	{
		if (!pAI->m_pAIResources->m_ActionGlobals.ms_MeleeFightSemaphore.Peek(pAI->m_pGameObject->m_iObject,1,pAI->GetGlobalAITick()))
		{
			OnTrackAgent(pTarget,6);
			return;
		}
	}

	CAI_Core* pTargetAI = pAI->GetAI(pTarget->GetObjectID());
	if ((!pTargetAI)||(!pTargetAI->IsConscious()))
	{	// Target unconscious or lacks AI
		ExpireWithDelay((int)(3.0f * pAI->GetAITicksPerSecond()));
	}

	if (pAI->m_pGameObject->AI_IsJumping())
	{	// Weee!
		pAI->m_DeviceMove.Use();
		return;
	}
	else if (m_bJumping)
	{	// We have landed, strike!
		DoJumpLandBehaviour(pTarget);
		m_bJumping = false;
		return;
	}

	int Awareness = pTarget->GetAwareness();
	// Check for eligible Darkling attack first
	if (Awareness == CAI_AgentInfo::SPOTTED)
	{
		// Nah, we just scare them civs
		if ((m_Flags & FLAGS_SCARE_CIVS)&&(pTargetAI->m_CharacterClass == CAI_Core::CLASS_CIV)&&(pAI->m_Timer >= m_NextScareTimer))
		{	// Check if we face the dude
			if (TargetRangeSqr <= Sqr((pAI->m_CloseMinRange+pAI->m_CloseMaxRange)*0.5f))
			{
				if (pAI->IsLookWithinAngle(34.0f,m_iTarget))
				{
					if (Random > 0.5f)
					{
						pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_DARKLING_SCARE,GetPriority(),0,60);
					}
					else
					{
						pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_DARKLING_SCARE2,GetPriority(),0,60);
					}
					pAI->UseRandom("Darkling scaring civ",CAI_Device_Sound::COMBAT_KILLJOY,GetPriority());
				}
			}
			m_NextScareTimer = pAI->m_Timer + pAI->GetAITicksPerSecond() * 10;
		}


		if ((m_AttackBehaviour)||(m_AttackBehaviourLong))
		{
			if (DoAttackBehaviour(pTarget))
			{
				pAI->m_DeviceMove.Use();
				return;
			}
		}

		if ((m_AttackJumpBehaviour)||(m_AttackJumpBehaviour2))
		{
			if (DoJumpAttackBehaviour(pTarget))
			{
				pAI->m_DeviceMove.Use();
				return;
			}
		}
	}

	CVec3Dfp32 TargetPos = pTargetAI->GetBasePos();
	if ((INVALID_POS(m_Destination))||(m_Destination.DistanceSqr(TargetPos) >= Sqr(48.0f)))
	{
		pAI->ResetPathSearch();
		m_Destination = pTarget->GetSuspectedPosition();
	}

	if (Awareness == CAI_AgentInfo::SPOTTED)
	{
		OnTrackAgent(pTarget,6);
		if (m_MoveMode == MODE_WALK)
		{
			pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,true,false,NULL);
		}
		else
		{
			pAI->OnMove(m_Destination,pAI->m_ReleaseMaxSpeed,true,false,NULL);
		}
		pAI->m_DeviceMove.Use();
		SpeakRandom(CAI_Device_Sound::COMBAT_SPOTTED,3.0f);
	}
	else if (Awareness == CAI_AgentInfo::DETECTED)
	{
		OnTrackAgent(pTarget,10);
		if (m_MoveMode == MODE_RUN)
		{
			pAI->OnMove(m_Destination,pAI->m_ReleaseMaxSpeed,true,false,NULL);
		}
		else
		{
			pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,true,false,NULL);
		}
		pAI->m_DeviceMove.Use();
		// Treat as SPOTTED until we haven't seen the bastard for some time now
		if (pAI->m_Timer >= pTarget->GetCurAwarenessTimer() + pAI->m_Patience)
		{
			SpeakRandom(CAI_Device_Sound::COMBAT_DETECTED,5.0f);
		}
		else
		{
			SpeakRandom(CAI_Device_Sound::COMBAT_SPOTTED,3.0f);
		}
	}
	else	// Awareness is NOTICED or less
	{
		SetExpirationExpired();
		return;
	}
};



void CAI_Action_DarklingClose::OnStart()
{
	CAI_Action::OnStart();
	m_StayTimeout = -1;
	INVALIDATE_POS(m_Destination);
	m_bJumping = false;

	// OK, I figure we can get the actual jumpranges here
	CAI_Core* pAI = AI();
	if (m_Flags & FLAGS_SYNC)
	{
		TArray<CXRAG2_StateConstant> lConstants; 
		lConstants.Clear();
		lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_OFFSETX,0.0f)); 
		lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_OFFSETY,0.0f)); 
		lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_OFFSETZ,0.0f));
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
		if (m_AttackBehaviour)
		{
			pCD->m_AnimGraph2.GetAG2I()->GetGraphBlockConstants(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, m_AttackBehaviour),lConstants);
			CVec3Dfp32 Offset;
			Offset[0] = lConstants[0].m_Value;
			Offset[1] = lConstants[1].m_Value;
			Offset[2] = lConstants[2].m_Value;
			if (Offset.LengthSqr() > Sqr(8.1f))
			{
				m_AttackRange = Offset.Length();
			}
		}
		if (m_AttackBehaviourLong)
		{
			pCD->m_AnimGraph2.GetAG2I()->GetGraphBlockConstants(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, m_AttackBehaviourLong),lConstants);
			CVec3Dfp32 Offset;
			Offset[0] = lConstants[0].m_Value;
			Offset[1] = lConstants[1].m_Value;
			Offset[2] = lConstants[2].m_Value;
			if (Offset.LengthSqr() > Sqr(8.1f))
			{
				m_AttackRangeLong = Offset.Length();
			}
		}
	}
};

void CAI_Action_DarklingClose::OnQuit()
{
	CAI_Core* pAI = AI();

	pAI->ResetPathSearch();
	if (m_iAttackTargetKilled)
	{
		if (m_Flags & FLAGS_SYNC)
		{	// Deimmunize
			CWObject_Message Msg(OBJMSG_CHAR_IMMUNE);
			Msg.m_Param0 = 0;	// Off
			pAI->m_pServer->Message_SendToObject(Msg,pAI->GetObjectID());
			pAI->m_pServer->Message_SendToObject(Msg,m_iAttackTargetKilled);
		}

		// Kill the target now
		if ((pAI->GetCurrentBehaviour() == m_AttackBehaviour)||(pAI->GetCurrentBehaviour() == m_AttackBehaviourLong))
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
			pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_PERFECTPLACEMENT,false);
		}

		CAI_Core* pTargetAI = pAI->GetAI(m_iAttackTargetKilled);
		if (pTargetAI)
		{
			CWObject_Character* pChar = CWObject_Character::IsCharacter(pTargetAI->m_pGameObject);
			if (pChar)
			{
				pChar->m_Flags |= PLAYER_FLAGS_NODEATHSOUND;
			}
			int32 Damage = pTargetAI->MaxHealth(m_iAttackTargetKilled)+1;
			CWO_DamageMsg Msg(Damage, DAMAGETYPE_DARKNESS);
			Msg.Send(m_iAttackTargetKilled, pAI->GetObjectID(), pAI->m_pServer);
			pTargetAI->ShutUp(PRIO_FORCED);
			pTargetAI->OnDie();
		}
		m_iAttackTargetKilled = 0;
	}
	m_pAH->m_ExitCombatTick = pAI->GetAITick();
	m_iTarget = 0;
	m_StayTimeout = -1;
	m_bJumping = false;
	INVALIDATE_POS(m_Destination);
	CAI_Action::OnQuit();
};

//CAI_Action_MeatfaceClose////////////////////////////////////////////////////////////////////////////
const char* CAI_Action_MeatfaceClose::ms_lStrFlags[] = 
{	
	"PATHFIND",
	"GENTLEMAN",
	"SYNC",
	"SHOOT",
	"JUMP",

	NULL,
};

CAI_Action_MeatfaceClose::CAI_Action_MeatfaceClose(CAI_ActionHandler* _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = MEATFACE_CLOSE;
	m_TypeFlags = TYPE_MOVE | TYPE_OFFENSIVE;

	m_bWaitForBomb = false;
	m_BombDuration = TruncToInt(3.0f * CAI_Core::AI_TICKS_PER_SECOND);
	m_BombTimer = -1;
	m_iTarget = 0;
	m_MoveMode = MODE_DEFAULT;
	m_bGentleman = false;
	m_Flags = 0;
	
	m_AnimEventEffect = "";

	m_AttackBehaviour = 0;
	m_AttackRange = 48.0f;

	INVALIDATE_POS(m_Destination);

	m_StartingHealth = 8.0f;
};

void CAI_Action_MeatfaceClose::SetParameter(int _Param, CStr _Str)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_MOVEMODE:
		SetParameter(_Param, _Str.Val_int());
		break;
	case PARAM_FLAGS:
		SetParameter(_Param, StrToFlags(_Str));
		break;
	case PARAM_ATTACK_BEHAVIOUR:
		SetParameter(_Param, _Str.Val_int());
		break;
	case PARAM_BOMBTIME:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;
	case PARAM_EFFECT:
		m_AnimEventEffect = _Str;
		break;
	default:
		CAI_Action::SetParameter(_Param, _Str);
	}
}


void CAI_Action_MeatfaceClose::SetParameter(int _Param, fp32 _Val)
{
	switch (_Param)
	{
	case PARAM_BOMBTIME:
		m_BombDuration = TruncToInt(_Val * CAI_Core::AI_TICKS_PER_SECOND);
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
		break;
	};
};

void CAI_Action_MeatfaceClose::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_MOVEMODE:
		if ((_Val == MODE_WALK)||(_Val == MODE_RUN))
		{
			m_MoveMode = _Val;
		}
		else
		{
			m_MoveMode = MODE_DEFAULT;
		}
		break;
	case PARAM_FLAGS:
		m_Flags = _Val;
		break;
	case PARAM_ATTACK_BEHAVIOUR:
		m_AttackBehaviour = _Val;
		break;
	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get parameter ID from string (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_MeatfaceClose::StrToParam(CStr _Str, int* _pResType)
{
	MAUTOSTRIP(CAI_Action_Close_StrToParam, 0);
	if (_Str == "MOVEMODE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_MOVEMODE;
	}
	else if (_Str == "ATTACKBEHAVIOUR")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_ATTACK_BEHAVIOUR;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else if (_Str == "EFFECT")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_EFFECT;
	}
	else if (_Str == "BOMBTIME")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_BOMBTIME;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

int CAI_Action_MeatfaceClose::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

bool CAI_Action_MeatfaceClose::AnimationEvent(int _iUser, int _iEvent)
{
	CAI_Core* pAI = AI();
	if ((m_AnimEventEffect.Len() > 0)&&(_iEvent == 1))
	{	// Kablooey!
		CMat4Dfp32 Mat;
		pAI->GetBaseMat(Mat);
		int iObj = pAI->m_pServer->Object_Create(m_AnimEventEffect,Mat);
		// And then die!
		int OrgMinHealth = pAI->m_MinHealth;
		pAI->m_MinHealth = 0;
		pAI->m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_ADDHEALTH,-2 * pAI->MaxHealth()));				
		pAI->OnTakeDamage(pAI->GetObjectID(),pAI->MaxHealth(),DAMAGETYPE_UNDEFINED);
		pAI->m_MinHealth = OrgMinHealth;
		return(true);
	}
	return(false);
};

//Mandatory: Move; Optional: Look
int CAI_Action_MeatfaceClose::MandatoryDevices()
{
	MAUTOSTRIP(CAI_Action_DarklingClose_MandatoryDevices, 0);
	return (1 << CAI_Device::MOVE);
};

bool CAI_Action_MeatfaceClose::IsValid()
{
	MAUTOSTRIP(CAI_Action_DarklingClose_IsValid, false);

	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		return(false);
	}

	if (m_iTarget != 0)
	{
		CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
		if (pTarget)
		{
			int Awareness = pTarget->GetCurAwareness();
			if (Awareness < CAI_AgentInfo::DETECTED)
			{
				m_iTarget = 0;
				return(false);
			}

			fp32 RangeSqr = AI()->SqrDistanceToUs(pTarget->GetBasePos());
			if (RangeSqr >  Sqr(AI()->m_CloseMaxRange))
			{
				m_iTarget = 0;
				return(false);
			}
		}
		else
		{
			m_iTarget = 0;
			return(false);
		}
	}

	return(true);
};

//Good offensively. Bad defensively.
CAI_ActionEstimate CAI_Action_MeatfaceClose::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP(CAI_Action_DarklingClose__GetEstimate, Est);

	CAI_Core* pAI = AI();
	if (IsValid())
	{
		CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
		if (!pTarget)
		{
			return Est;
		}

		if (!pAI->IsValidTarget(pTarget->GetObjectID()))
		{
			return Est;
		}

		//Default score 95
		m_iTarget = pTarget->GetObjectID();
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 60); 
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 10);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, -20);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 5);
	}
	return Est;
};

int CAI_Action_MeatfaceClose::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int nAdded = 0;
	if (m_AttackBehaviour)
	{
		nAdded += AddUnique(_liBehaviours,m_AttackBehaviour);
		nAdded += AddUnique(_liBehaviours,m_AttackBehaviour+1);
	}
	if (m_AnimEventEffect.Len() > 0)
	{	// Precache effect
		m_pAH->PrecacheEffect(m_AnimEventEffect);
	}
	return(nAdded);
};

int CAI_Action_MeatfaceClose::GetUsedAcs(TArray<int16>& _liAcs) const
{
	int nAdded = 0;
	if ((m_Flags & FLAGS_SYNC)&&(m_AttackBehaviour))
	{
		nAdded += AddUnique(_liAcs,m_AttackBehaviour);
		nAdded += AddUnique(_liAcs,m_AttackBehaviour+1);
	}
	return(nAdded);
};

int CAI_Action_MeatfaceClose::GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const
{
	int nAdded = 0;

	if (m_Flags & FLAGS_SHOOT)
	{
		nAdded += AddUnique(_liMoves,CAI_Core::GESTURE_MEATFACE_CHECK_THROW);
	}

	return(nAdded);
};

bool CAI_Action_MeatfaceClose::DoJumpBehaviour(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();

	if ((!_pTarget)||(!(m_Flags & FLAGS_JUMP))||(_pTarget->GetCurAwareness() < CAI_AgentInfo::SPOTTED))
	{
		return(false);
	}

	CVec3Dfp32 TargetPos = _pTarget->GetCorrectPosition();
	CAI_Core* pTargetAI = pAI->GetAI(_pTarget->GetObjectID());
	if (!pTargetAI)
	{
		return(false);
	}

	// Range
	if ((pAI->SqrDistanceToUs(TargetPos) > Sqr(m_AttackRange+64))||(pAI->SqrDistanceToUs(TargetPos) <= Sqr(m_AttackRange+32)))
	{
		return(false);
	}

	// FOV
	pAI->OnTrackAgent(_pTarget,10,false,false);
	if (pAI->CosHeadingDiff(AI()->m_pGameObject,_pTarget->GetObject()) < 0.92f)
	{	// We must turn
		return(false);
	}

	CMat4Dfp32 OurMat;
	pAI->GetBaseMat(OurMat);
	CVec3Dfp32 Pos = _pTarget->GetSuspectedHeadPosition();
	OurMat.GetRow(3) = Pos;
	if (pAI->AddJumpTowardsPositionCmd(&OurMat,CAI_Core::JUMPREASON_COMBAT,CAI_Core::JUMP_CHECK_COLL | CAI_Core::JUMP_TO_AIR))
	{
		return(true);
	}

	return(false);
}
bool CAI_Action_MeatfaceClose::DoShootBehaviour(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();

	if ((!_pTarget)||(!(m_Flags & FLAGS_SHOOT)))
	{
		return(false);
	}

	CVec3Dfp32 TargetPos = _pTarget->GetCorrectPosition();
	CAI_Core* pTargetAI = pAI->GetAI(_pTarget->GetObjectID());
	if ((!pTargetAI)||(pAI->m_pGameObject->AI_IsOnWall()))
	{
		return(false);
	}

	// Range
	if ((pAI->SqrDistanceToUs(TargetPos) > Sqr(m_AttackRange+32))||(pAI->SqrDistanceToUs(TargetPos) <= Sqr(m_AttackRange+8)))
	{
		return(false);
	}

	// FOV
	pAI->OnTrackAgent(_pTarget,10,false,false);
	if (pAI->CosHeadingDiff(AI()->m_pGameObject,_pTarget->GetObject()) < 0.92f)
	{	// We must turn
		return(false);
	}

	pAI->SetWantedMove(CAI_Core::GESTURE_MEATFACE_CHECK_THROW);	

	return(true);
};

bool CAI_Action_MeatfaceClose::DoAttackBehaviour(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();

	if ((!_pTarget)||(!pAI->IsPlayer(_pTarget->GetObjectID())))
	{
		return(false);
	}

	CVec3Dfp32 TargetPos = _pTarget->GetCorrectPosition();
	CAI_Core* pTargetAI = pAI->GetAI(_pTarget->GetObjectID());
	if ((!pTargetAI)||(pAI->m_pGameObject->AI_IsOnWall()))
	{
		return(false);
	}

	CMat4Dfp32 OurMat;
	pAI->GetBaseMat(OurMat);
	// Reverse fix for darkling
	if ((pAI->m_bCanWallClimb)&&(pAI->m_bWallClimb))
	{	// Drop base mat
		OurMat.GetRow(3) -= pAI->GetUp() * DARKLING_PF_OFFSET;
	}
	CVec3Dfp32 OurPos = OurMat.GetRow(3);
	CVec3Dfp32 Diff = TargetPos - OurPos;
	fp32 RangeSqr = Diff.LengthSqr();
	// Start the bombtimer even if elevation forbid facehugs
	if ((m_BombTimer == -1)&&(RangeSqr <= Sqr(m_AttackRange+8.0f)))
	{	
		pAI->UseRandom("MeatFace bomb activated",CAI_Device_Sound::COMBAT_KILLJOY,GetPriority());
		m_BombTimer = m_BombDuration;
		m_StartingHealth = pAI->Health();
		// Start smoking
		CWObject_Message Msg(OBJMSG_CHAR_DARKLING_SMOKE);
		Msg.m_Param0 = 1;	// Turn on the effect, 0 means turning it off again
		pAI->m_pServer->Message_SendToObject(Msg,pAI->m_pGameObject->m_iObject);
		if (!m_AttackBehaviour)
		{
			m_bWaitForBomb = true;
		}
	}

	if (m_bWaitForBomb)
	{
		pAI->m_DeviceMove.Use();
		return(true);
	}

	// Needed
	if (Abs(Diff[2]) >= 4.0f)
	{	
		return(false);
	}
	
	int32 iBehaviour = m_AttackBehaviour;
	fp32 AttackRange = m_AttackRange;
	if ((!iBehaviour)||(AttackRange <= 0.0f))
	{
		return(false);
	}

	if ((m_Flags & FLAGS_SYNC)&&(RangeSqr <= Sqr(AttackRange+32.0f)))
	{	// *** Start checking for LOOK
		if (!pAI->IsPlayerLookWithinCosAngle(0.87f))
		{
			if (RangeSqr <= Sqr(AttackRange+8.0f))
			{
				pAI->m_DeviceMove.Use();
				return(true);
			}
			return(false);
		}
	}

	if (RangeSqr < Sqr(AttackRange-8.0f))
	{	// Too close, bring out the dirty tricks
		return(false);
	}
	if ((!iBehaviour)||(AttackRange <= 0.0f))
	{
		return(false);
	}

	if ((RangeSqr <= Sqr(AttackRange+8.0f))&&(RangeSqr >= Sqr(AttackRange-8.0f)))
	{	// TODO: Should this be 16.0 units, 8.0 each?
		CWO_Character_ClientData* pOurCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
		CWO_Character_ClientData* pTargetCD = CWObject_Character::GetClientData(pTargetAI->m_pGameObject);
		if (pTargetCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BLOCKACTIVE)
		{	// Stop if Jackie is already fighting one of us meatfaces
			// (meatfaces, why do they call us this? I think I am pretty pretty, and with a sense of humor too!)
			return(false);
		}
		CMat4Dfp32 TargetMat;
		pTargetAI->GetBaseMat(TargetMat);
		CVec3Dfp32 Diff = (OurPos - TargetPos).Normalize();
		pAI->OnTrackDir(-Diff,pTargetAI->GetObjectID(),6,true,false);
		// TODO: Split the pos-discrepancy equally between us and target?
		Diff.SetRow(TargetMat,0);
		TargetMat.RecreateMatrix(0,2);
		(-Diff).SetRow(OurMat,0);
		OurPos = TargetMat.GetRow(3) + Diff * AttackRange;
		OurPos.SetRow(OurMat,3);
		OurMat.RecreateMatrix(2,0);
		if (m_Flags & FLAGS_SYNC)
		{
			CAI_Weapon* pWeapon = pTargetAI->m_Weapon.GetWielded();
			if (pWeapon)
			{
				CRPG_Object_Item* pItem = pWeapon->GetItem();
				if ((pItem)&&(pItem->m_AnimType == AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE))
				{
					iBehaviour = iBehaviour+1;
				}
			}
			// We use our time here
			CWAG2I_Context Context(pTargetAI->m_pGameObject,pAI->m_pServer,pOurCD->m_GameTime);
			pTargetCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,iBehaviour),0);
			pTargetCD->m_AnimGraph2.SetDestination(TargetMat,0.0f);
			// pTargetCD->m_AnimGraph2.SetUpVector();
			// Set fighting character
			pTargetCD->m_iFightingCharacter = pAI->m_pGameObject->m_iObject;
		}

		// The player dude has his animation taken care of, let's do our
		pAI->m_CurCB.Clear();
		pAI->m_PrevCB.Clear();
		if (pAI->SetWantedBehaviour2(iBehaviour,
			CAI_Action::PRIO_FORCED,
			CAI_Core::BEHAVIOUR_FLAGS_PO,-1,
			NULL,&OurMat))
		{	// Even  nicer, we rest the bombtimer when attacking the players face
			m_BombTimer = m_BombDuration;
			m_StartingHealth = pAI->Health();
			m_bWaitForBomb = true;
			return(true);
		}
	}
	else
	{
		return(false);
	}

	return(false);
};

//Fetch boy, fetch!
void CAI_Action_MeatfaceClose::OnTakeAction()
{
	MAUTOSTRIP(CAI_Action_DarklingClose__OnTakeAction, MAUTOSTRIP_VOID);
	MSCOPESHORT(CAI_Action_DarklingClose::OnTakeAction);
	CAI_Core* pAI = AI();

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (!pTarget)
	{
		SetExpirationExpired();
	}

	// Check restriction
	// Yes we might be allowed to close to target without violating the restriction but what use would that be?
	if (m_pAH->IsRestricted(pTarget,true))
	{	
		if (pAI->DebugRender())
		{
			CStr Name = pAI->m_pGameObject->GetName();
			ConOut(Name+CStr(": Close restricted, delay 3s"));
		}
		pAI->OnTrackAgent(pTarget,10,false,false);
		ExpireWithDelay(60);
		return;
	}

	fp32 TargetRangeSqr = pAI->SqrDistanceToUs(pTarget->GetObjectID());
	// Range check target vs pAI->m_CloseMinRange and pAI->m_CloseMaxRange
	if ((TargetRangeSqr < Sqr(pAI->m_CloseMinRange))||(TargetRangeSqr > Sqr(pAI->m_CloseMaxRange)))
	{
		SetExpirationExpired();
		return;
	}

	pAI->ActivateAnimgraph();
	SetExpiration((int)(pAI->m_Patience * 3.0f));

	// Handle the bomb, before anything else
	OnServiceBehaviour();

	// Move towards target according to flags and awareness
	pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
	pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
	if ((m_Flags & FLAGS_GENTLEMAN)&&(TargetRangeSqr <= Sqr(64))&&(pAI->m_pGameObject)&&(pAI->m_pServer))
	{
		if (!pAI->m_pAIResources->m_ActionGlobals.ms_MeleeFightSemaphore.Peek(pAI->m_pGameObject->m_iObject,1,pAI->GetGlobalAITick()))
		{
			OnTrackAgent(pTarget,6);
			return;
		}
	}

	CAI_Core* pTargetAI = pAI->GetAI(pTarget->GetObjectID());
	if ((!pTargetAI)||(!pTargetAI->IsConscious()))
	{	// Target unconscious or lacks AI
		ExpireWithDelay((int)(3.0f * pAI->GetAITicksPerSecond()));
	}

	if (pAI->m_pGameObject->AI_IsJumping())
	{	// Weee!
		pAI->m_DeviceMove.Use();
		return;
	}

	int Awareness = pTarget->GetAwareness();
	// Check for eligible Darkling attack first
	if (Awareness == CAI_AgentInfo::SPOTTED)
	{
		if (DoJumpBehaviour(pTarget))
		{
			pAI->m_DeviceMove.Use();
			return;
		}

		if (DoShootBehaviour(pTarget))
		{
			pAI->m_DeviceMove.Use();
			return;
		}

		if (DoAttackBehaviour(pTarget))
		{
			pAI->m_DeviceMove.Use();
			return;
		}
	}

	CVec3Dfp32 TargetPos = pTargetAI->GetBasePos();
	if ((INVALID_POS(m_Destination))||(m_Destination.DistanceSqr(TargetPos) >= Sqr(48.0f)))
	{
		pAI->ResetPathSearch();
		m_Destination = pTarget->GetSuspectedPosition();
	}

	if (Awareness == CAI_AgentInfo::SPOTTED)
	{
		OnTrackAgent(pTarget,6);
		if (m_MoveMode == MODE_WALK)
		{
			pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,true,false,NULL);
		}
		else
		{
			pAI->OnMove(m_Destination,pAI->m_ReleaseMaxSpeed,true,false,NULL);
		}
		pAI->m_DeviceMove.Use();
		SpeakRandom(CAI_Device_Sound::COMBAT_SPOTTED,3.0f);
	}
	else if (Awareness == CAI_AgentInfo::DETECTED)
	{
		OnTrackAgent(pTarget,10);
		if (m_MoveMode == MODE_RUN)
		{
			pAI->OnMove(m_Destination,pAI->m_ReleaseMaxSpeed,true,false,NULL);
		}
		else
		{
			pAI->OnMove(m_Destination,pAI->m_ReleaseIdleSpeed,true,false,NULL);
		}
		pAI->m_DeviceMove.Use();
		// Treat as SPOTTED until we haven't seen the bastard for some time now
		if (pAI->m_Timer >= pTarget->GetCurAwarenessTimer() + pAI->m_Patience)
		{
			SpeakRandom(CAI_Device_Sound::COMBAT_DETECTED,5.0f);
		}
		else
		{
			SpeakRandom(CAI_Device_Sound::COMBAT_SPOTTED,3.0f);
		}
	}
	else	// Awareness is NOTICED or less
	{
		SetExpirationExpired();
		return;
	}
};

bool CAI_Action_MeatfaceClose::OnServiceBehaviour()
{
	CAI_Core* pAI = AI();

	if ((m_BombTimer > -1)||(pAI->Health() < m_StartingHealth))
	{
		if ((pAI->Health() < m_StartingHealth)||(m_BombTimer == 0))
		{	// Oh no! Said the Lemming...
			CMat4Dfp32 Mat;
			pAI->GetBaseMat(Mat);
			if (m_AnimEventEffect.Len() > 0)
			{
				int iObj = pAI->m_pServer->Object_Create(m_AnimEventEffect,Mat);
			}
			int32 Damage = pAI->MaxHealth()+1;
			CWO_DamageMsg Msg(Damage, DAMAGETYPE_UNDEFINED);
			Msg.Send(pAI->GetObjectID(),0,pAI->m_pServer);
			pAI->OnDie();
			SetExpirationExpired();
			m_BombTimer = -1; 
			return(true);
		}
		m_BombTimer--;
		return(true);
	}
	else
	{
		return(false);
	}
};

void CAI_Action_MeatfaceClose::OnStart()
{
	CAI_Action::OnStart();
	INVALIDATE_POS(m_Destination);

	// OK, I figure we can get the actual ranges here
	CAI_Core* pAI = AI();
	TArray<CXRAG2_StateConstant> lConstants; 
	lConstants.Clear();
	lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_OFFSETX,0.0f)); 
	lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_OFFSETY,0.0f)); 
	lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_OFFSETZ,0.0f));
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pAI->m_pGameObject);
	if (m_AttackBehaviour)
	{
		pCD->m_AnimGraph2.GetAG2I()->GetGraphBlockConstants(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, m_AttackBehaviour),lConstants);
		CVec3Dfp32 Offset;
		Offset[0] = lConstants[0].m_Value;
		Offset[1] = lConstants[1].m_Value;
		Offset[2] = lConstants[2].m_Value;
		if (Offset.LengthSqr() > Sqr(8.1f))
		{
			m_AttackRange = Offset.Length();
		}
	}

	m_StartingHealth = pAI->Health();
};

void CAI_Action_MeatfaceClose::OnQuit()
{
	CAI_Core* pAI = AI();

	pAI->ResetPathSearch();
	m_pAH->m_ExitCombatTick = pAI->GetAITick();
	m_iTarget = 0;
	INVALIDATE_POS(m_Destination);
	CAI_Action::OnQuit();
};

//CAI_Action_DarklingJumpClose////////////////////////////////////////////////////////////////////////////
const char* CAI_Action_DarklingJumpClose::ms_lStrFlags[] = 
{	
		"RUN",
		"CLOSEST",
		"PLAYERFOV",
		"HEADING",
		"AVOIDLIGHT",
		"PREFERLIGHT",
		"NOSTAY",
		"TOHEAD",
		NULL,
};

CAI_Action_DarklingJumpClose::CAI_Action_DarklingJumpClose(CAI_ActionHandler * _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = DARKLING_JUMPCLOSE;
	m_TypeFlags = TYPE_MOVE;

	m_iTarget = 0;
	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_Direction = CVec3Dfp32(_FP32_MAX);
	m_Flags = FLAGS_NO_STAY;
	m_StayTimeout = -1;
	m_pScenePoint = NULL;
};

void CAI_Action_DarklingJumpClose::SetParameter(int _Param, fp32 _Val)
{
	CAI_Action::SetParameter(_Param, _Val);
}

void CAI_Action_DarklingJumpClose::SetParameter(int _Param, int _Val)
{
	switch (_Param)
	{
	case PARAM_FLAGS:
		m_Flags = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

void CAI_Action_DarklingJumpClose::SetParameter(int _Param, CStr _Val)
{
	switch (_Param)
	{
	case PARAM_FLAGS:
		{
			int Val = StrToFlags(_Val);
			SetParameter(_Param,Val);
		}
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}

}

//Get parameter ID from sting (used when parsing registry)
int CAI_Action_DarklingJumpClose::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
}

//Get flags for given string
int CAI_Action_DarklingJumpClose::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
}

//Mandatory: Move; Optional: Look
int CAI_Action_DarklingJumpClose::MandatoryDevices()
{
	return (1 << CAI_Device::MOVE);
};

bool CAI_Action_DarklingJumpClose::IsValid()
{
	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		m_iTarget = 0;
		return(false);
	}

	if (!(pAI->m_JumpFlags & CAI_Core::JUMPREASON_COMBAT))
	{
		m_iTarget = 0;
		return(false);
	}

	if (m_iTarget)
	{
		if (!pAI->m_bWallClimb)
		{
			m_iTarget = 0;
			return(false);
		}
		CAI_Core* pTargetAI = pAI->GetAI(m_iTarget);
		if ((!pTargetAI)||(!pTargetAI->IsConscious()))
		{	// Target unconscious or lacks AI
			return(false);
		}
	}
	return(true);
}

//Very good exploratory, somewhat unsafe
CAI_ActionEstimate CAI_Action_DarklingJumpClose::GetEstimate()
{
	CAI_ActionEstimate Est;
	CAI_Core* pAI = AI();
	if (IsValid())
	{
		//Check for available scene point type
		m_pAH->GetScenePoints(pAI->m_JumpMaxRange);
		if (!m_pAH->ScenePointTypeAvailable(CWO_ScenePoint::ROAM | CWO_ScenePoint::TACTICAL,pAI->m_JumpMaxRange))
		{
			return Est;
		}
		
		if (!m_iTarget)
		{
			CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
			if (pTarget)
			{
				m_iTarget = pTarget->GetObjectID();
				if (pAI->SqrDistanceToUs(m_iTarget) < Sqr(pAI->m_JumpMinRange))
				{
					return Est;
				}
			}
		}
		
		if (!m_iTarget)
		{
			return Est;
		}

		if (!pAI->m_bWallClimb)
		{
			return Est;
		}

		//Default score 65
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 0); 
		Est.Set(CAI_ActionEstimate::DEFENSIVE, -10);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 75);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, -20);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 20);
	}
	return Est;
};

// Tries to find a destination position from available scenepoints
bool CAI_Action_DarklingJumpClose::FindScenePointDestination()
{
	CAI_Core* pAI = AI();

	// We already have a valid scenepoint?
	if (m_pScenePoint)
	{
		if (m_pAH->RequestScenePoint(m_pScenePoint))
		{
			return(true);
		}
		else
		{	// No, it's not OUR job to set the memory scenepoints
			m_pScenePoint = NULL;
			return(false);
		}
	}

	CVec3Dfp32 TargetPos;
	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (pTarget)
	{
		TargetPos = pTarget->GetSuspectedPosition();
	}
	else
	{
		m_pScenePoint = NULL;
		return(false);
	}

	int Flags = CAI_ActionHandler::SP_PREFER_HEADING | CAI_ActionHandler::SP_REQUIRE_TARGET_CLOSER | CAI_ActionHandler::SP_PREFER_NEAR_TARGETPOS;
	if (!(m_Flags & FLAGS_CLOSEST_PT))
	{
		Flags |= CAI_ActionHandler::SP_RANDOM_RANGE;
	}
	if (m_Flags & FLAGS_IN_PLAYERFOV)
	{
		Flags |= CAI_ActionHandler::SP_REQUIRE_PLAYERFOV;
	}
	if (m_Flags & FLAGS_PREFER_HEADING)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_HEADING;
	}
	if (m_Flags & FLAGS_AVOID_LIGHT)
	{
		Flags |= CAI_ActionHandler::SP_AVOID_LIGHT;
	}
	if (m_Flags & FLAGS_PREFER_LIGHT)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_LIGHT;
	}

	CMat4Dfp32 OurMat;
	pAI->GetBaseMat(OurMat);
	m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::JUMP_CLIMB | CWO_ScenePoint::ROAM | CWO_ScenePoint::TACTICAL,Flags,pAI->m_JumpMinRange,pAI->m_JumpMaxRange,OurMat,TargetPos,0.0f,0);
	INVALIDATE_POS(m_Destination);
	if (!m_pScenePoint)
	{
		return(false);
	}
	if (m_pScenePoint->GetType() & CWO_ScenePoint::JUMP_CLIMB)
	{
		m_Destination = m_pScenePoint->GetPosition();
	}
	else
	{
		m_Destination = pAI->m_PathFinder.GetPathPosition(m_pScenePoint->GetPosition(),4,2);
	}
	if ((!m_pAH->RequestScenePoint(m_pScenePoint))||(INVALID_POS(m_Destination))||(m_pAH->IsRestricted(m_Destination)))
	{
		INVALIDATE_POS(m_Destination);
		m_pAH->UpdateScenepointHistory(m_pScenePoint);
		m_pScenePoint = NULL;
		return(false);
	}

	m_Direction = m_pScenePoint->GetDirection();

#ifndef M_RTM
	m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif

	return(true);
};

bool CAI_Action_DarklingJumpClose::FindDestination()
{
	MSCOPESHORT(CAI_Action_DarklingJumpClose::FindDestination);
	CAI_Core* pAI = AI();

	if ((m_iTarget)&&(m_Flags & FLAGS_TO_HEAD)&&
		(pAI->SqrDistanceToUs(m_iTarget) > Sqr(pAI->m_JumpMinRange))&&
		(pAI->SqrDistanceToUs(m_iTarget) < Sqr(pAI->m_JumpMaxRange)))
	{	// CAI_Action_DarklingJumpClose::ISValid checks for target validity each tick so we don't have to
		if (pAI->m_pGameObject->AI_IsJumping())
		{
			return(true);
		}
		// We are NOT jumping, set up a jump towards the head (of Yuri Gagarin)
		CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
		if (pTarget)
		{
			CAI_Core* pTargetAI = pAI->GetAI(m_iTarget);
			if (pTargetAI)
			{
				CMat4Dfp32 Mat;
				pTargetAI->GetHeadMat(Mat);
				CVec3Dfp32 Pos = Mat.GetRow(3);
				CVec3Dfp32 Fwd = Pos - pAI->GetBasePos();
				Fwd.Normalize();
				Fwd.SetRow(Mat,0);
				Mat.RecreateMatrix(0,2);
				if (pAI->AddJumpTowardsPositionCmd(&Mat,CAI_Core::JUMPREASON_COMBAT,CAI_Core::JUMP_CHECK_COLL | CAI_Core::JUMP_TO_AIR))
				{
					return(true);
				}
			}
		}
	}

	// CAI_Action_DarklingJumpClose don't use m_Destination to denote a vladit destination
	// Instead it uses m_pScenePoint as only scenepoints are in fact valid destinations
	if (m_pScenePoint)
	{
		if (m_pAH->RequestScenePoint(m_pScenePoint))
		{
			return(true);
		}
		else
		{
			m_pScenePoint = false;
			return(false);
		}
	}

	if (pAI->m_pGameObject->AI_IsJumping())
	{
		return(false);
	}

	m_StayTimeout = -1;
	if (FindScenePointDestination())
	{
		if (m_pScenePoint->GetType() & CWO_ScenePoint::JUMP_CLIMB)
		{	// JUMP!
			CMat4Dfp32 Mat = m_pScenePoint->GetPositionMatrix();
			CVec3Dfp32 Pos = Mat.GetRow(3);
			CVec3Dfp32 Up = Mat.GetRow(2);
			Pos.SetRow(Mat,3);
			if (pAI->AddJumpTowardsPositionCmd(&Mat,CAI_Core::JUMPREASON_COMBAT,CAI_Core::JUMP_CHECK_COLL | CAI_Core::JUMP_TO_AIR))
			{
#ifndef M_RTM
				m_pAH->DebugDrawScenePoint(m_pScenePoint,true,0,5.0f);
#endif
				return(true);
			}
			else
			{	// Cannot jump
				INVALIDATE_POS(m_Destination);
				m_pAH->UpdateScenepointHistory(m_pScenePoint);
				m_pScenePoint = NULL;
				return(false);
			}
		}
	}

	// No scenepoint destination, perhaps we can just jump towards the target and hope for the best
	if ((pAI->SqrDistanceToUs(m_iTarget) > Sqr(pAI->m_JumpMinRange))&&
		(pAI->SqrDistanceToUs(m_iTarget) < Sqr(pAI->m_JumpMaxRange)))
	{
		CAI_Core* pTargetAI = pAI->GetAI(m_iTarget);
		if (pTargetAI)
		{	// JUMP!
			CMat4Dfp32 Mat;
			pTargetAI->GetBaseMat(Mat);
			CVec3Dfp32 OurPos = pAI->GetBasePos();
			CVec3Dfp32 TargetPos = pTargetAI->GetBasePos(); 
			CVec3Dfp32 Dir = OurPos - TargetPos;
			Dir[2] = 0.0f;
			Dir.Normalize();
			CVec3Dfp32 Pos = TargetPos + Dir * pAI->m_JumpMinRange * 0.75f;
			CAI_Action* pAction;
			if (pAI->m_AH.GetAction(DARKLING_CLOSE,&pAction))
			{
				CAI_Action_DarklingClose* pDarklingClose = safe_cast<CAI_Action_DarklingClose>(pAction);
				if ((pDarklingClose)&&(pDarklingClose->m_AttackRange))
				{
					Pos = TargetPos + Dir * pDarklingClose->m_AttackRange;
				}
				
			}
			else
			{
				CVec3Dfp32 PathPos = pAI->m_PathFinder.GetPathPosition(Pos,2,8);
				if (VALID_POS(PathPos))
				{
					Pos = PathPos;
				}
			}
			Pos.SetRow(Mat,3);
			(-Dir).SetRow(Mat,0);
			Mat.RecreateMatrix(2,0);
#ifndef	M_RTM
			pAI->m_pServer->Debug_RenderMatrix(Mat,5.0f,true,kColorRed,kColorGreen,kColorBlue);
#endif
			if (pAI->AddJumpTowardsPositionCmd(&Mat,CAI_Core::JUMPREASON_COMBAT,CAI_Core::JUMP_CHECK_COLL))
			{
				ExpireWithDelay(pAI->m_Patience);
				return(true);
			}
		}
	}

	// Bad
	// We neither expire nor fall down. We just hope that DarklingClose or even Roam will move us to a better position/alignment
	// for jumping and then the almighty jumping darkling will reemerge.
	return(false);
};

bool CAI_Action_DarklingJumpClose::MoveToDestination()
{
	MSCOPESHORT(CAI_Action_DarklingJumpClose::MoveToDestination);
	CAI_Core* pAI = AI();

	if (!m_pScenePoint)
	{
		m_pScenePoint = false;
		m_StayTimeout = -1;
		return(false);
	}
	
	if (pAI->m_pGameObject->AI_IsJumping())
	{	// Hog the move and look devices
		pAI->m_DeviceMove.Use();
		pAI->m_DeviceLook.Use();
		return(false);
	}

	// We should be there dude
	if (m_Flags & FLAGS_NO_STAY)
	{
		CVec3Dfp32 SPDir = m_pScenePoint->GetDirection();
		pAI->OnTrackDir(SPDir,0,10,false,false);
		m_pScenePoint = NULL;
		m_StayTimeout = -1;
		INVALIDATE_POS(m_Destination);
		return(false);
	}

	return(true);
};

// Returns 1 if we're staying at the scenepoint
bool CAI_Action_DarklingJumpClose::StayAtDestination()
{
	CAI_Core* pAI = AI();

	if (!m_pScenePoint)
	{
		m_StayTimeout = -1;
		return(false);
	}

	CVec3Dfp32 OurPos = pAI->GetBasePos();
	CVec3Dfp32 OurDir = pAI->GetLookDir();
	if (m_pScenePoint->IsAligned(OurDir))
	{	
		if (m_StayTimeout == -1)
		{
			int StayTicks = (int)(m_pScenePoint->GetBehaviourDuration() * pAI->GetAITicksPerSecond());
			m_StayTimeout = pAI->m_Timer + StayTicks;
			CWO_ScenePoint* orgSP = m_pScenePoint;
			m_pAH->ActivateScenePoint(m_pScenePoint,GetPriority());
			if (orgSP != m_pScenePoint) { return(false); }
			SetMinExpirationDuration(StayTicks);
			// Darkling is on wall or SP allows wall climbing
			if ((pAI->m_pGameObject->AI_IsOnWall())||(m_pScenePoint->GetType() & CWO_ScenePoint::WALK_CLIMB))
			{
				pAI->TempEnableWallClimb(StayTicks+pAI->m_Patience);
			}
		}
		else if (pAI->m_Timer > m_StayTimeout)
		{	// We're done dude
			m_StayTimeout = -1;
			if (m_pScenePoint)
			{
				m_pScenePoint = NULL;
			}
			return(false);
		}
		if ((m_pScenePoint)&&(m_pScenePoint->GetBehaviour())&&(m_pScenePoint->GetBehaviourDuration() == 0.0f))
		{
			m_StayTimeout = pAI->m_Timer + pAI->GetAITicksPerSecond();
		}
		return(true);
	}
	else
	{	// We are not yet aligned
		CVec3Dfp32 SPDir = m_pScenePoint->GetDirection();
		pAI->OnTrackDir(SPDir,0,10,false,false);
		m_StayTimeout = -1;
		return(true);
	}
	
	return(true);
};

void CAI_Action_DarklingJumpClose::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		INVALIDATE_POS(m_Destination);
		m_StayTimeout = -1;
		m_pScenePoint = NULL;
	}
};

void CAI_Action_DarklingJumpClose::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_DarklingJumpClose::OnTakeAction);
	CAI_Core* pAI = AI();

	pAI->ActivateAnimgraph();
	SetExpiration((int)(5.0f * pAI->m_Patience));

	if (FindDestination())
	{	// We have a valid Destination
		if (MoveToDestination())
		{
			if (StayAtDestination())
			{
#ifndef M_RTM
				m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
			}
			else
			{
#ifndef M_RTM
				m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif
			}
		}
	}
};

void CAI_Action_DarklingJumpClose::OnStart()
{
	CAI_Action::OnStart();
	CAI_Core* pAI = AI();

	// Default member values
	m_StayTimeout = -1;
	INVALIDATE_POS(pAI->m_PathDestination);
	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_Direction);

	return;
};

//Reset path stuff
void CAI_Action_DarklingJumpClose::OnQuit()
{
	CAI_Core* pAI = AI();

	m_pScenePoint = NULL;
	m_StayTimeout = -1;
	pAI->ResetPathSearch();
	INVALIDATE_POS(m_Destination);
	INVALIDATE_POS(m_Direction);
	CAI_Action::OnQuit();
};

//CAI_Action_FlyCombat////////////////////////////////////////////////////////////////////////////
const char* CAI_Action_FlyCombat::ms_lStrFlags[] = 
{
		"CLOSEST",
		"PLAYERFOV",
		"HEADING",
		"AVOIDLIGHT",
		"PREFERLIGHT",
		"ALIGN2PATH",
		"HELIBANKING",
		"JUMPS",
		"ANIMPATHS",
		"BURST",
		"WORLDTRACKING",

		NULL,
};

CAI_Action_FlyCombat::CAI_Action_FlyCombat(CAI_ActionHandler * _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = FLY_COMBAT;
	m_TypeFlags = TYPE_MOVE | TYPE_OFFENSIVE;

	m_iTarget = 0;
	INVALIDATE_POS(m_TargetPos);
	m_MoveMode = MODE_DEFAULT;

	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_StayTimeout = 0;
	m_pScenePoint = NULL;
	m_iPath = 0;
	m_iAnimation = 0;
	m_iPosHistoryFinger = 0;
	for (int32 i = 0; i < POS_COUNT; i++)
	{
		m_lPosHistory[i] = CVec3Dfp32(0.0f);
	}
	m_Flags = FLAGS_CLOSEST_PT;
};

void CAI_Action_FlyCombat::SetParameter(int _Param, CStr _Str)
{
	MAUTOSTRIP(CAI_Action_FlyCombat_SetParameter, 0);

	CAI_Core* pAI = AI();

	switch (_Param)
	{
	case PARAM_MOVEMODE:
		SetParameter(_Param, _Str.Val_int());
		break;
	case PARAM_FLAGS:
		SetParameter(_Param, StrToFlags(_Str));
		break;
	case PARAM_MOUNT:
		{	// Mounted object
			int iObj = pAI->m_pServer->Selection_GetSingleTarget(_Str);
			SetParameter(_Param, iObj);
		}
		break;
	default:
		CAI_Action::SetParameter(_Param, _Str);
	}
}


void CAI_Action_FlyCombat::SetParameter(int _Param, fp32 _Val)
{
	MAUTOSTRIP(CAI_Action_FlyCombat_SetParameter_2, 0);
	// You can complain all you want - I'll keep it this way until I add a fp32 param again!
	CAI_Action::SetParameter(_Param, _Val);
}

void CAI_Action_FlyCombat::SetParameter(int _Param, int _Val)
{
	MAUTOSTRIP(CAI_Action_FlyCombat_SetParameter_3, 0);
	switch (_Param)
	{
	case PARAM_MOVEMODE:
		if ((_Val == MODE_WALK)||(_Val == MODE_RUN))
		{
			m_MoveMode = _Val;
		}
		else
		{
			m_MoveMode = MODE_DEFAULT;
		}
		break;
	case PARAM_MOUNT:
		if (_Val > 0)
		{
			CAI_Core* pAI = AI();
			pAI->m_iMountParam = _Val;
		}
		break;
	case PARAM_FLAGS:
		m_Flags = _Val;
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
	}
}

//Get flags for given string
int CAI_Action_FlyCombat::StrToFlags(CStr _FlagsStr)
{
	return _FlagsStr.TranslateFlags(ms_lStrFlags);
};

//Get parameter ID from string (used when parsing registry). If optional type result pointer
//is given, this is set to parameter type.
int CAI_Action_FlyCombat::StrToParam(CStr _Str, int* _pResType)
{
	MAUTOSTRIP(CAI_Action_FlyCombat_StrToParam, 0);

	if (_Str == "MOVEMODE")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_INT;
		return PARAM_MOVEMODE;
	}
	else if (_Str == "MOUNT")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_SPECIAL;
		return PARAM_MOUNT;
	}
	else if (_Str == "FLAGS")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLAGS;
		return PARAM_FLAGS;
	}
	else
		return CAI_Action::StrToParam(_Str, _pResType);
};

int CAI_Action_FlyCombat::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int32 nAdded = 0;
/*
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_ANGELUS_THROWBALL);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_ANGELUS_BREAKBLEECHER);
*/
	return(nAdded);
};

//Mandatory: Move; Optional: Look
int CAI_Action_FlyCombat::MandatoryDevices()
{
	MAUTOSTRIP(CAI_Action_FlyCombat_MandatoryDevices, 0);
	return (1 << CAI_Device::MOVE);
};

bool CAI_Action_FlyCombat::IsValid()
{
	MAUTOSTRIP(CAI_Action_FlyCombat_IsValid, false);
	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		return(false);
	}

	if (m_iPath)
	{	// We must always finish our path
		return(true);
	}

	if (m_iTarget != 0)
	{
		CAI_AgentInfo* pTarget = AI()->m_KB.GetAgentInfo(m_iTarget);
		if ((pTarget == NULL)||(pTarget->GetCurAwareness() < CAI_AgentInfo::DETECTED))
		{
			m_iTarget = 0;
			if (pAI->m_Script.m_iTargetParam)
			{
				return(true);
			}
			else
			{
				return(false);
			}
		}
	}

	return(true);
}

CAI_ActionEstimate CAI_Action_FlyCombat::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP(CAI_Action_FlyCombat__GetEstimate, Est);
	CAI_Core* pAI = AI();

	if (!IsValid())
	{
		return Est;
	}

	CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
	if ((pTarget)&&(pTarget->GetCurAwareness() >= CAI_AgentInfo::DETECTED))
	{
		if (!m_pAH->ScenePointTypeAvailable(CWO_ScenePoint::TACTICAL,pAI->m_CombatMaxRange))
		{
			m_iTarget = 0;
			return Est;
		}

		m_iTarget = pTarget->GetObjectID();
		m_TargetPos = pTarget->GetSuspectedTorsoPosition();

		Est.Set(CAI_ActionEstimate::OFFENSIVE,75);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 25);
		Est.Set(CAI_ActionEstimate::LOYALTY,0);
		Est.Set(CAI_ActionEstimate::LAZINESS,-20);
		Est.Set(CAI_ActionEstimate::STEALTH,0);
		Est.Set(CAI_ActionEstimate::VARIETY,5);
	}
	else
	{
		if (pAI->m_Script.m_iTargetParam)
		{
			m_iTarget = 0;
			m_TargetPos = pAI->GetBasePos(pAI->m_Script.m_iTargetParam);
			Est.Set(CAI_ActionEstimate::OFFENSIVE,75);
			Est.Set(CAI_ActionEstimate::EXPLORATION, 25);
			Est.Set(CAI_ActionEstimate::LOYALTY,0);
			Est.Set(CAI_ActionEstimate::LAZINESS,-20);
			Est.Set(CAI_ActionEstimate::STEALTH,0);
			Est.Set(CAI_ActionEstimate::VARIETY,5);
		}
	}

	return Est;
};

bool CAI_Action_FlyCombat::OnFlyFollowPath()
{
	MSCOPESHORT(CAI_Action_FlyCombat::OnFlyFollowPath);

	CAI_Core* pAI = AI();
	if (!m_iPath)
	{
		return(false);
	}

	// Path exists, has it finished?
	CMTime Time;
	CWObject_Message TimeMsg(OBJMSG_HOOK_GETTIME);
	TimeMsg.m_pData = &Time;
	TimeMsg.m_DataSize = sizeof(Time);
	pAI->m_pServer->Message_SendToObject(TimeMsg, m_iPath);
	// TimeMsg.m_pData now holds the current time
	fp32 curTick = Time.GetTime() * CAI_Core::AI_TICKS_PER_SECOND;
	TimeMsg.m_Msg = OBJMSG_HOOK_GETDURATION;
	fp32 fTime;
	TimeMsg.m_Param0 = 0;
	TimeMsg.m_Param1 = (aint)&fTime;
	TimeMsg.m_DataSize = sizeof(fp32);
	pAI->m_pServer->Message_SendToObject(TimeMsg, m_iPath);
	fp32 endTick = fTime * CAI_Core::AI_TICKS_PER_SECOND;
#ifndef M_RTM
	pAI->DebugDrawPath(m_iPath,100,kColorRed);
	m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif
	if (curTick >= endTick)
	{	// EP has finished
		if (m_pScenePoint)
		{
			if (m_pScenePoint->IsAt(pAI->GetBasePos()))
			{	// We're there dude
				fp32 Duration;
				CWObject_Message Msg(OBJMSG_HOOK_GETDURATION);
				Msg.m_Param0 = 0;
				Msg.m_Param1 = (aint)&Duration;
				Msg.m_DataSize = sizeof(Duration);
				pAI->m_pServer->Message_SendToObject(Msg,m_iPath);
				pAI->SetTimePath(m_iPath,Duration,true);
				return(true);
			}
			else
			{	// Force move us onto the SP (*** Perhaps blend this move over several ticks ***)
#ifndef M_RTM
				CStr EPName = pAI->m_pServer->Object_GetName(m_iPath);
				CStr SPName = m_pScenePoint->GetName();
				ConOutL(CStrF("CAI_Action_FlyCombat: EP missed SP: ")+EPName+":"+SPName);
				pAI->DebugDrawPath(m_iPath,100,kColorGreen);
				m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
				// We must determine some kind of speed to use to get to our destination: flightvel
				// We then calculate the time it would take us to get to dest with that speed tottime
				// If tottime > ticktime
				//		Move flightvel towards goal
				//		Lerp orientation ticktime / tottime
				// else
				//		Set end matrix and object velocity 0.0

				CMat4Dfp32 CurMat,NextMat,FinalMat;
				TimeMsg.m_Msg = OBJMSG_HOOK_GETTIME;
				TimeMsg.m_pData = &Time;
				TimeMsg.m_DataSize = sizeof(Time);
				pAI->m_pServer->Message_SendToObject(TimeMsg, m_iPath);
				pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)(&Time), (aint)(&CurMat), pAI->GetObjectID()), m_iPath);
				pAI->GetBaseMat(CurMat);
				FinalMat = m_pScenePoint->GetPositionMatrix();
				fp32 Speed = pAI->GetPrevSpeed();
				Speed = Max(Speed,1.0f);	// Move at least 1 unit per tick ie 0.625 m/s ie 2.25 km/h
				CVec3Dfp32 Velocity = FinalMat.GetRow(3) - CurMat.GetRow(3);
				fp32 Distance = Velocity.Length();
				if ((Speed >= Distance)||(Distance < 4.0f))
				{
					Velocity = CVec3Dfp32(0.0f,0.0f,0.0f);
					NextMat = FinalMat;
					pAI->m_DeviceMove.Use(CVec3Dfp32(0.0f,0.0f,0.0f), false);
				}
				else
				{
					fp32 tfrac = Speed / Distance;
					CQuatfp32 Q1,Q2,Q3;
					Q1.Create(CurMat);
					Q2.Create(FinalMat);
					Q1.Lerp(Q2,tfrac,Q3);
					Q3.CreateMatrix(NextMat);
					CVec3Dfp32 Pos = CurMat.GetRow(3) + Velocity * tfrac;
					Pos.SetRow(NextMat,3);
					pAI->m_DeviceMove.Use(CVec3Dfp32(1.0f,0.0f,0.0f), false);
				}
				pAI->m_pServer->Object_SetPosition(pAI->GetObjectID(),NextMat);
				pAI->m_pServer->Object_SetVelocity(pAI->GetObjectID(),Velocity);
				return(false);
			}
		}
		else
		{
			ConOut("CAI_Action_FlyCombat: End of enginepath without scenepoint");
			return(false);
		}
	}
	else
	{
		// There is still time left on the EnginePath
		//Fly! Switch off gravity and animcontrol
		pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRAVITY, 0), pAI->GetObjectID());
		pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ANIMCONTROL, 0), pAI->GetObjectID());

		CMat4Dfp32 CurMat,NextMat;
		TimeMsg.m_Msg = OBJMSG_HOOK_GETTIME;
		TimeMsg.m_pData = &Time;
		TimeMsg.m_DataSize = sizeof(Time);
		pAI->m_pServer->Message_SendToObject(TimeMsg, m_iPath);
		pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)(&Time), (aint)(&CurMat), pAI->GetObjectID()), m_iPath);
		Time += CMTime::CreateFromSeconds(AI_TICK_DURATION);
		pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)(&Time), (aint)(&NextMat), pAI->GetObjectID()), m_iPath);
		CVec3Dfp32 FlightVel = NextMat.GetRow(3) - CurMat.GetRow(3);


		// Find distance between current and next tick of EP and use that as
		pAI->m_pServer->Object_SetPosition(pAI->GetObjectID(), CurMat);
		pAI->m_pServer->Object_SetVelocity(pAI->GetObjectID(),FlightVel);
		pAI->m_DeviceMove.Use(CVec3Dfp32(1.0f,0,0), false);
		FlightVel.Normalize();

		// We set rotation to CurMat but we modify it to point along EP direction if FLAGS_ALIGN_2_PATH is not set,
		// and we modify the Up vector according to speed and acceleration if FLAGS_HELI_BANKING is set
		if (m_Flags & FLAGS_ALIGN_2_PATH)
		{	// Use EP direction
			FlightVel.SetRow(CurMat,0);
			CurMat.RecreateMatrix(2,0);
		}

		if (m_Flags & FLAGS_HELI_BANKING)
		{	 
			// *** Fix HELIBANKING ***
			// *** Lateral banking is wrong signed ***

			// We take a moving average of speed and acceleration over POS_COUNT measurements
			// These will perturb the up vector to a certain extent. Then we recreate the matrix
			// prioritising the forward and then the up.
			CVec3Dfp32 AvgV = CVec3Dfp32(0.0f);
			CVec3Dfp32 AvgA = CVec3Dfp32(0.0f);
			CVec3Dfp32 V = CVec3Dfp32(0.0f);
			CVec3Dfp32 prevV = CVec3Dfp32(0.0f);
			for (int i = 1; i < POS_COUNT; i++)
			{
				prevV = V;
				V = m_lPosHistory[(i+m_iPosHistoryFinger) % POS_COUNT] - m_lPosHistory[(i+m_iPosHistoryFinger-1) % POS_COUNT];
				AvgV += V;
				AvgA += V - prevV;
			}
			AvgV *= FLY_COMBAT_V_FACTOR / (POS_COUNT-1);
			AvgA *= FLY_COMBAT_A_FACTOR / (POS_COUNT-1);
			CVec3Dfp32 Up = pAI->GetUp();

			// Use only AvgA perpendicular to FlightVel, and reverse sign?
			CVec3Dfp32 Left;
			Up.CrossProd(FlightVel,Left);
			Left.Normalize();
			fp32 Dot = AvgA * Left;
			AvgA = -Left * Dot;

			Dot = AvgV * Left;
			AvgV = -Left * Dot;

			// Limit AvgV contribution to 1.0f
			if (AvgV.LengthSqr() > Sqr(0.5f))
			{
				AvgV.Normalize();
			}
			// Limit AvgA contribution to 1.0f
			if (AvgA.LengthSqr() > Sqr(1.0f))
			{
				AvgA.Normalize();
			}
			Up += AvgV;
			Up += AvgA;
			Up.Normalize();
			Up.SetRow(CurMat,2);
			CurMat.RecreateMatrix(0,2);
			CVec3Dfp32 Base = CVec3Dfp32(0,0,64);
			pAI->Debug_RenderWire(Base,Base+(AvgV * 32),kColorGreen,0.0f);
			pAI->Debug_RenderWire(Base,Base+(AvgA * 32),kColorRed,1.0f);
			pAI->Debug_RenderWire(Base,Base+(Up * 32),kColorWhite,0.0f);
			// ***
		}

		pAI->m_pServer->Object_SetRotation(pAI->GetObjectID(), CurMat);

		return(false);
	}
};

// Tries to find a destination position from available scenepoints
bool CAI_Action_FlyCombat::FindScenePointDestination()
{
	CAI_Core* pAI = AI();

	CVec3Dfp32 TargetPos;
	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if (pTarget)
	{
		TargetPos = pTarget->GetSuspectedPosition();
	}
	else
	{
		m_iTarget = 0;
		if (pAI->m_Script.m_iTargetParam)
		{
			TargetPos = pAI->GetBasePos(pAI->m_Script.m_iTargetParam);
		}
		else
		{
			return(false);
		}
	}

	// Try to find a valid scenepoint
	if (TargetPos.DistanceSqr(pAI->GetBasePos()) > Sqr(pAI->m_CombatTargetMaxRange))
	{
		m_pScenePoint = NULL;
		m_Destination = CVec3Dfp32(_FP32_MAX);
		SetExpirationExpired();
		return(false);
	}

	int Flags = CAI_ActionHandler::SP_ENGINEPATH_SP;
	Flags |= CAI_ActionHandler::SP_PREFER_INSIDE_ARC;	// *** Selectable by flag perhaps? ***
	Flags |= CAI_ActionHandler::SP_PREFER_NEAR_TARGETPOS;
	if (!(m_Flags & FLAGS_CLOSEST_PT))
	{
		Flags |= CAI_ActionHandler::SP_RANDOM_RANGE;
	}
	if (m_Flags & FLAGS_IN_PLAYERFOV)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_PLAYERFOV;
	}
	if (m_Flags & FLAGS_PREFER_HEADING)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_HEADING;
	}
	if (m_Flags & FLAGS_AVOID_LIGHT)
	{
		Flags |= CAI_ActionHandler::SP_AVOID_LIGHT;
	}
	else if (m_Flags & FLAGS_PREFER_LIGHT)
	{
		Flags |= CAI_ActionHandler::SP_PREFER_LIGHT;
	}
	CMat4Dfp32 OurMat;
	pAI->GetBaseMat(OurMat);
	m_pScenePoint = m_pAH->GetBestScenePoint(CWO_ScenePoint::TACTICAL,Flags,pAI->m_CombatMinRange,pAI->m_CombatMaxRange,OurMat,TargetPos,0.0f,m_iTarget);
	INVALIDATE_POS(m_Destination);
	if ((m_pScenePoint)&&(m_pAH->RequestScenePoint(m_pScenePoint)))
	{	// Found scenepoint we could request with good path position
		m_Destination = m_pScenePoint->GetPosition();
		// Stop roaming if restricted
		if ((VALID_POS(m_Destination))&&(m_pAH->IsRestricted(m_Destination)))
		{
			INVALIDATE_POS(m_Destination);
			m_pScenePoint = NULL;
			return(false);
		}

		// TODO: Make action capable of handling mixed path/anim
		if (m_Flags & FLAGS_ANIMPATHS)
		{
			m_iAnimation = m_pAH->GetEnginepathID(m_pScenePoint);
		}
		else
		{
			m_iPath = m_pAH->GetEnginepathID(m_pScenePoint);
		}
		if (m_iPath)
		{
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif
			// Rewind and start playing enginepath
			pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, -1, 0, pAI->GetObjectID()), m_iPath);
			// Only start path if we're going to use the pathspeed
			pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 1, 0, pAI->GetObjectID()), m_iPath);
			return(true);
		}
		else if (m_iAnimation)
		{
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,false);
#endif
			pAI->SetWantedBehaviour2(m_iAnimation,CAI_Action::PRIO_FORCED,CAI_Core::BEHAVIOUR_FLAGS_PP,0,m_pScenePoint,NULL);
			return(true);
		}
	}

	// If wer got here we failed
	// No path:
	INVALIDATE_POS(m_Destination);
	m_pScenePoint = NULL;
	return(false);
};

bool CAI_Action_FlyCombat::FindDestination()
{
	AI();

	if (VALID_POS(m_Destination))
	{
		return(true);
	}

	m_StayTimeout = -1;
	FindScenePointDestination();

	if (VALID_POS(m_Destination))
	{
		return(true);
	}
	else
	{
		return(false);
	}
};

bool CAI_Action_FlyCombat::MoveToDestination()
{
	MSCOPESHORT(CAI_Action_FlyCombat::MoveToDestination);
	CAI_Core* pAI = AI();

	if ((INVALID_POS(m_Destination))||(!m_pScenePoint))
	{
		m_StayTimeout = -1;
		if ((m_pScenePoint)&&(m_pScenePoint->IsAt(pAI->GetBasePos())))
		{
			m_pScenePoint = NULL;
		}
		return(false);
	}

	pAI->ResetStuckInfo();
	if (m_iPath)
	{
		if (OnFlyFollowPath())
		{
#ifndef M_RTM
			m_pAH->DebugDrawScenePoint(m_pScenePoint,true,0,1.0f);
#endif
			return(true);
		}
		return(false);
	}
	else if (m_iAnimation)
	{
		if (pAI->m_bBehaviourRunning)
		{
			return(false);
		}
		else
		{	// We should be there, for safetys sake we check nearest SP
			m_pScenePoint = m_pAH->GetNearestEPSP(CWO_ScenePoint::TACTICAL,pAI->m_CombatMaxRange);
			return(true);
		}
	}

	// If we got here something is terribly wrong
	INVALIDATE_POS(m_Destination);
	return(false);
};

bool CAI_Action_FlyCombat::StayAtDestination()
{
	CAI_Core* pAI = AI();

	if (!m_pScenePoint)
	{
		return(false);
	}

#ifndef M_RTM
	m_pAH->DebugDrawScenePoint(m_pScenePoint,true);
#endif
	if (m_StayTimeout == -1)
	{
		int StayTicks = 0;
		pAI->StopNow();
		StayTicks = (int)(m_pScenePoint->GetBehaviourDuration() * pAI->GetAITicksPerSecond());
		m_StayTimeout = pAI->m_Timer + StayTicks;
		CWO_ScenePoint* orgSP = m_pScenePoint;
		m_pAH->ActivateScenePoint(m_pScenePoint,GetPriority());
		if (orgSP != m_pScenePoint) { return(false); }
		return(true);
	}
	else
	{
		if (VALID_POS(m_TargetPos))
		{
			if ((!m_pScenePoint->InFrontArc(m_TargetPos))||(!m_pAH->RequestScenePoint(m_pScenePoint)))
			{
				m_StayTimeout = -1;
				m_pScenePoint = NULL;
				INVALIDATE_POS(m_Destination);
				return(false);
			}
		}

		if (pAI->m_Timer > m_StayTimeout)
		{	// We're done staying
			m_StayTimeout = -1;
			m_pScenePoint = NULL;
			INVALIDATE_POS(m_Destination);
			return(false);
		}
		else
		{	// We're not yet done staying
			// pAI->m_DeviceMove.Use();
			if ((m_pScenePoint)&&(m_pScenePoint->GetBehaviour())&&(m_pScenePoint->GetBehaviourDuration() == 0.0f))
			{
				m_StayTimeout = pAI->m_Timer + pAI->GetAITicksPerSecond();
			}
			return(true);
		}
	}
};

bool CAI_Action_FlyCombat::OnServiceBehaviour()
{
	CAI_Core* pAI = AI();

	int32 Flags = 0;
	if (m_Flags & FLAGS_WORLDTRACKING) { Flags |= CAI_Core::MOUNTFLAGS_WORLDTRACKING;}
	if (m_Flags & FLAGS_BURST) { Flags |= CAI_Core::MOUNTFLAGS_BURST;}
	if (m_iTarget)
	{
		if (pAI->HandleMount(Flags,m_iTarget,&m_TargetPos))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
	else
	{
		if (pAI->HandleMount(Flags,pAI->m_Script.m_iTargetParam,&m_TargetPos))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
};

void CAI_Action_FlyCombat::RequestScenepoints()
{
	if ((m_pScenePoint)&&(!m_pAH->RequestScenePoint(m_pScenePoint,1)))
	{
		INVALIDATE_POS(m_Destination);
		m_StayTimeout = -1;
		m_pScenePoint = NULL;
	}
};

void CAI_Action_FlyCombat::OnTakeAction()
{
	MSCOPESHORT(CAI_Action_FlyCombat::OnTakeAction);
	CAI_Core* pAI = AI();

	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();
	if ((!pTarget)||(pTarget->GetCurRelation() < CAI_AgentInfo::ENEMY))
	{
		m_iTarget = 0;
		m_TargetPos = pAI->GetBasePos(pAI->m_Script.m_iTargetParam);
		if ((!m_iPath)&&(INVALID_POS(m_TargetPos)))
		{
			SetExpirationExpired();
			return;
		}
		// Safety check
		if (!m_pAH->GetPrevScenepoint())
		{
			CWO_ScenePoint* pNearestSP = m_pAH->GetNearestEPSP(CWO_ScenePoint::TACTICAL,pAI->m_CombatMaxRange);
			if (pNearestSP)
			{
				m_pAH->UpdateScenepointHistory(pNearestSP);
				if (pAI->DebugRender())
				{
					CStr Name = CStr(pAI->m_pGameObject->GetName());
					ConOut(CStr("FlyCombat had no scenepoint ") + Name);
				}
			}
		}
		if (pAI->m_iMountParam)
		{
			OnServiceBehaviour();
		}
		if (m_StayTimeout)
		{
			pAI->OnTrackObj(m_iTarget,6,false,false);
		}
	}
	else
	{
		m_TargetPos = pTarget->GetSuspectedTorsoPosition();
		int32 Awareness = pTarget->GetCurAwareness();
		if (Awareness == CAI_AgentInfo::SPOTTED)
		{	// 104,105,106,107
			if (m_iSpeech == SPEAK_DEFAULT)
			{
				int iDialogue = 104 + (int)(4 * Random * 0.999f);
				pAI->m_DeviceSound.PlayDialogue(iDialogue,PRIO_COMBAT,20 * int(Random * 20.0f));
			}
		}
		else
		{	// 100, 101, 102, 103
			if (m_iSpeech == SPEAK_DEFAULT)
			{
				int iDialogue = 100 + (int)(4 * Random * 0.999f);
				pAI->m_DeviceSound.PlayDialogue(iDialogue,PRIO_COMBAT,20 * int(Random * 20.0f));
			}
		}
		pAI->SetStealthTension(CAI_Core::TENSION_COMBAT);
		pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
		if (pAI->m_iMountParam)
		{
			OnServiceBehaviour();
		}
		if (m_StayTimeout)
		{
			pAI->OnTrackObj(m_iTarget,6,false,false);
		}
	}
	
	// Store the current unaltered position, this is used in the moving average
	// speed and acceleration computations used for banking
	if (m_Flags & FLAGS_HELI_BANKING)
	{
		m_lPosHistory[m_iPosHistoryFinger] = pAI->GetBasePos();
		m_iPosHistoryFinger = (m_iPosHistoryFinger+1) % POS_COUNT;
	}

	if (FindDestination())
	{
		if (MoveToDestination())
		{
			StayAtDestination();
		}
		else
		{
			pAI->m_DeviceMove.Use();
		}
	}
};

void CAI_Action_FlyCombat::OnStart()
{
	CAI_Core* pAI = AI();
	CAI_Action::OnStart();

	m_Destination = CVec3Dfp32(_FP32_MAX);
	CWO_ScenePoint* pNearestSP = m_pAH->GetNearestEPSP(CWO_ScenePoint::TACTICAL,pAI->m_CombatMaxRange);
	if (pNearestSP)
	{
		m_pAH->UpdateScenepointHistory(pNearestSP);
	}
	m_pScenePoint = NULL;
	m_StayTimeout = -1;

	m_iPath = 0;
	m_iPosHistoryFinger = 0;
	for (int32 i = 0; i < POS_COUNT; i++)
	{
		m_lPosHistory[i] = pAI->GetBasePos();
	}

	// Prepare the gunfire
	if (m_Flags & FLAGS_BURST)
	{
		pAI->m_DeviceWeapon.SetPeriod(100);
		pAI->m_DeviceWeapon.SetPressDuration(100);
	}
};

void CAI_Action_FlyCombat::OnQuit()
{
	CAI_Core* pAI = AI();
	m_pAH->m_ExitCombatTick = AI()->GetAITick();

	m_iTarget = 0;
	INVALIDATE_POS(m_TargetPos);
	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_pScenePoint = NULL;
	if (m_iPath)
	{
		// Stop and rewind enginepath
		pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, 0, 0, pAI->GetObjectID()), m_iPath);
		pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_IMPULSE, -1, 0, pAI->GetObjectID()), m_iPath);
		m_iPath = 0;
	}
	CAI_Action::OnQuit();
};

//CAI_Action_AngelusAttack////////////////////////////////////////////////////////////////////////////
CAI_Action_AngelusAttack::CAI_Action_AngelusAttack(CAI_ActionHandler* _pAH, int _Priority)
: CAI_Action(_pAH, _Priority)
{
	m_Type = ANGELUS_ATTACK;
	m_TypeFlags = TYPE_MOVE | TYPE_OFFENSIVE;

	m_bPrimary = true;		// When true use primary fire, when false use secondary
	m_PrimaryDuration = RoundToInt(CAI_Core::AI_TICKS_PER_SECOND * 0.5f);			// Number of ticks to press primary fire
	m_SecondaryDuration = RoundToInt(CAI_Core::AI_TICKS_PER_SECOND * 3.0f);			// Number of ticks to press secondary fire
	m_PressTicks = 0;

	m_FlashBangState = ANGELUS_AURASTATE_IDLE;
	m_SecondaryState = ANGELUS_TENTACLESTATE_IDLE;

	m_iTarget = 0;
};

int CAI_Action_AngelusAttack::GetUsedBehaviours(TArray<int16>& _liBehaviours) const
{
	int nAdded = 0;

	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_ANGELUS_FLASHBANG);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_ANGELUS_TENTACLE_HITOBJECT);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_ANGELUS_TENTACLE_HITTARGET);
	nAdded += AddUnique(_liBehaviours,CAI_Core::BEHAVIOUR_ANGELUS_EMBRACE);

	return(nAdded);
};

void CAI_Action_AngelusAttack::SetParameter(int _Param, fp32 _Val)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_PRIMARYDURATION:
		m_PrimaryDuration = RoundToInt(_Val * AI()->GetAITicksPerSecond());
		break;
	case PARAM_SECONDARYDURATION:
		m_SecondaryDuration = RoundToInt(_Val * AI()->GetAITicksPerSecond());
		break;

	default:
		CAI_Action::SetParameter(_Param, _Val);
		break;
	}
};

void CAI_Action_AngelusAttack::SetParameter(int _Param, int _Val)
{
	if (!AI())
		return;

	// TODO: Add switch statement when we got actions
	CAI_Action::SetParameter(_Param, _Val);

};

void CAI_Action_AngelusAttack::SetParameter(int _Param, CStr _Str)
{
	if (!AI())
		return;

	switch (_Param)
	{
	case PARAM_PRIMARYDURATION:
	case PARAM_SECONDARYDURATION:
		SetParameter(_Param, (fp32)_Str.Val_fp64());
		break;
	
	default:
		CAI_Action::SetParameter(_Param, _Str);
		break;
	}
};

//Get parameter ID from string (used when parsing registry)
int CAI_Action_AngelusAttack::StrToParam(CStr _Str, int* _pResType)
{
	if (_Str == "PRIMARYDURATION")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_PRIMARYDURATION;
	}
	else if (_Str == "SECONDARYDURATION")
	{
		if (_pResType) 
			*_pResType = PARAMTYPE_FLOAT;
		return PARAM_SECONDARYDURATION;
	}
	else
	{
		return(CAI_Action::StrToParam(_Str, _pResType));
	}
};

//Mandatory: Weapon
int CAI_Action_AngelusAttack::MandatoryDevices()
{
	return (1 << CAI_Device::MELEE) | (1 << CAI_Device::MOVE) | 
		(1 << CAI_Device::WEAPON) | (1 << CAI_Device::ITEM);
};

//Valid if at least one action is of TYPE_OFFENSIVE
bool CAI_Action_AngelusAttack::IsValid()
{
	MAUTOSTRIP(CAI_Action_AngelusAttack_IsValid, false);

	CAI_Core* pAI = AI();
	if (!CAI_Action::IsValid())
	{
		return(false);
	}

	if (m_iTarget)
	{
		CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
		if (IsValid(pTarget))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
	else
	{
		return(true);
	}
};

bool CAI_Action_AngelusAttack::IsValid(CAI_AgentInfo* _pTarget)
{
	CAI_Core* pAI = AI();

	if (_pTarget)
	{
		if (_pTarget->GetCurAwareness() < CAI_AgentInfo::DETECTED)
		{
			return(false);
		}
		if (_pTarget->GetCurRelation() < CAI_AgentInfo::ENEMY)
		{
			return(false);
		}
		if ((pAI->SqrDistanceToUs(_pTarget->GetObjectID()) > Sqr(pAI->m_RangedMaxRange))||
			(pAI->SqrDistanceToUs(_pTarget->GetObjectID()) < Sqr(pAI->m_RangedMinRange)))
		{
			return(false);
		}
		return(true);
	}
	else
	{
		return(false);
	}
};

CAI_ActionEstimate CAI_Action_AngelusAttack::GetEstimate()
{
	CAI_ActionEstimate Est;
	MAUTOSTRIP( CAI_Action_AngelusAttack_GetEstimate, Est);

	CAI_Core* pAI = AI();
	if (IsValid())
	{
		CAI_AgentInfo* pTarget = m_pAH->AcquireTarget();
		if (!pTarget)
		{
			return Est;
		}
		if ((pAI->SqrDistanceToUs(pTarget->GetObjectID()) > Sqr(pAI->m_RangedMaxRange))||
			(pAI->SqrDistanceToUs(pTarget->GetObjectID()) < Sqr(pAI->m_RangedMinRange)))
		{
			return Est;
		}
		if (!pAI->IsValidTarget(pTarget->GetObjectID()))
		{
			return Est;
		}

		//Default score 95
		m_iTarget = pTarget->GetObjectID();
		Est.Set(CAI_ActionEstimate::OFFENSIVE, 60); 
		Est.Set(CAI_ActionEstimate::DEFENSIVE, 0);
		Est.Set(CAI_ActionEstimate::EXPLORATION, 10);
		Est.Set(CAI_ActionEstimate::LOYALTY, 0);
		Est.Set(CAI_ActionEstimate::LAZINESS, -20);
		Est.Set(CAI_ActionEstimate::STEALTH, 0);
		Est.Set(CAI_ActionEstimate::VARIETY, 5);
	}
	return Est;
};

bool CAI_Action_AngelusAttack::SetTentacleState(int32 _State)
{
	CAI_Core* pAI = AI();
	if (m_SecondaryState != _State)
	{
		switch(_State)
		{
		case ANGELUS_TENTACLESTATE_IDLE:
			{	// Force behaviour to finish
				pAI->StopBehaviour(CAI_Core::BEHAVIOUR_STOP_NORMAL,PRIO_FORCED);
			}
			break;

		case ANGELUS_TENTACLESTATE_HITOBJECT:
			{	// Play oneshot behaviour
				pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_ANGELUS_TENTACLE_HITOBJECT,
					GetPriority(),
					0,
					0,
					NULL,NULL);
			};
			break;

		case ANGELUS_TENTACLESTATE_HITTARGET:
			{
				pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_ANGELUS_TENTACLE_HITTARGET,
					GetPriority(),
					CAI_Core::BEHAVIOUR_FLAGS_LOOP,
					0,
					NULL,NULL);
				SetExpirationExpired();
			};
			break;

		case ANGELUS_TENTACLESTATE_REELING_IN:
			{	// Force behaviour to finish
				pAI->StopBehaviour(CAI_Core::BEHAVIOUR_STOP_NORMAL,PRIO_FORCED);				
			}
			break;

		case ANGELUS_TENTACLESTATE_EMBRACING:
			{
				pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_ANGELUS_EMBRACE,
					GetPriority(),
					CAI_Core::BEHAVIOUR_FLAGS_LOOP,
					0,
					NULL,NULL);
			}
			break;

		default:
			return(false);
			break;
		}

		m_SecondaryState = _State;
		return(true);
	}
	else
	{
		return(false);
	}
}

// Punch that guy dammit!
void CAI_Action_AngelusAttack::OnTakeAction()
{
	MAUTOSTRIP(CAI_Action_AngelusAttack_OnTakeAction, 0);
	MSCOPESHORT(CAI_Action_AngelusAttack::OnTakeAction);

	CAI_Core* pAI = AI();
	CAI_AgentInfo* pTarget = pAI->m_KB.GetAgentInfo(m_iTarget);
	if ((!pTarget)||(!IsValid(pTarget)))
	{
		SetExpirationExpired();
		return;
	}

	int Relation = pTarget->GetCurRelation();
	if (Relation < CAI_AgentInfo::ENEMY)
	{
		SetExpirationExpired();
		return;
	}

	// This may take a while
	pAI->ActivateAnimgraph();
	SetExpirationIndefinite();
	CVec3Dfp32 StartPos = pAI->m_pGameObject->AI_GetWeaponPos();
	CVec3Dfp32 EndPos = pAI->GetTorsoPosition(pTarget);
	pAI->OnTrackDir(EndPos-StartPos,m_iTarget,6,false,false);
	pAI->m_DeviceStance.SetIdleStance(CAI_Device_Stance::IDLESTANCE_COMBAT);
	pAI->SetMinStealthTension(CAI_Core::TENSION_COMBAT,true);

	int RecoverDurationTicks = int(pAI->GetAITicksPerSecond() * 5.0f);	// *** DUMMY, read from character ***
	// Handle weaponry
	if (m_PressTicks)
	{
		m_PressTicks--;
	}

	// Handle flashbang
	if ((!m_PressTicks)&&(m_FlashBangState == ANGELUS_AURASTATE_IDLE)&&(pAI->m_DeviceSound.IsAvailable()))
	{
		pAI->SetWantedBehaviour2(CAI_Core::BEHAVIOUR_ANGELUS_FLASHBANG,
			GetPriority(),CAI_Core::BEHAVIOUR_FLAGS_LOOP,
			RecoverDurationTicks,
			NULL,NULL);
		// Tell char we have started the mighty flashbang
		CWObject_Message Msg(OBJMSG_CHAR_ANGELUS_AURA_SETSTATE,ANGELUS_AURASTATE_USING);
		m_FlashBangState = pAI->m_pServer->Message_SendToObject(Msg,pAI->m_pGameObject->m_iObject);

		SetExpirationExpired();
		return;
	}

	// New attack?
	if (!m_PressTicks)
	{	// Should we attack
		if (pAI->CosHeadingDiff(pAI->m_pGameObject,pTarget->GetObject()) >= 0.87f)
		{	
			// if (pTarget->GetCurAwareness() >= CAI_AgentInfo::SPOTTED)

			// *** DEBUG ***
			// *** We check if the target is crouching as a debug test ***
			CAI_Core* pTargetAI = pAI->GetAI(m_iTarget);
			if ((pTargetAI)&&(CWObject_Character::Char_GetPhysType(pTargetAI->m_pGameObject) != PLAYER_PHYS_CROUCH))
			{	// Shoot
				m_bPrimary = true;
				m_PressTicks = m_PrimaryDuration;
				pAI->m_DeviceWeapon.Use();
			}
			else
			{	// Grab cover
				m_bPrimary = false;
				m_PressTicks = m_SecondaryDuration;
				pAI->m_DeviceItem.Use();
			}
		}
	}
	
};

void CAI_Action_AngelusAttack::OnStart()
{
	CAI_Action::OnStart();
	CAI_Core* pAI = AI();

	m_PressTicks = 0;
	m_bPrimary = true;

	CWObject_Message Msg(OBJMSG_CHAR_ANGELUS_AURA_GETSTATE);
	m_FlashBangState = pAI->m_pServer->Message_SendToObject(Msg,pAI->m_pGameObject->m_iObject);
	m_SecondaryState = ANGELUS_TENTACLESTATE_IDLE;
};

void CAI_Action_AngelusAttack::OnQuit()
{
	m_iTarget = 0;
	CAI_Action::OnQuit();
};
