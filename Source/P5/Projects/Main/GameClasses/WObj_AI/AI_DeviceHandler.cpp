#include "PCH.h"
#include "AICore.h"
#include "MFloat.h"
#include "../WObj_CharMsg.h"
#include "../WObj_RPG.h"


//Classes for controlling devices 


//Base class
void CAI_Device::SetAI(CAI_Core * _pAI)
{
	m_pAI = _pAI;
};

CAI_Device::CAI_Device()
{
	MAUTOSTRIP(CAI_Device_ctor, MAUTOSTRIP_VOID);
	m_pAI = NULL;
	m_bAvailable = true;
	m_bLocked = false;
	m_bPause = false;
};

//Checks if device isn't in use
bool CAI_Device::IsAvailable()
{
	MAUTOSTRIP(CAI_Device_IsAvailable, false);
	if (m_bPause)
	{
		return false;
	}
	else
	{
		return m_bAvailable;
	}	
};

//Lock device for usage, without using it
void CAI_Device::Lock(bool _bUse)
{
	MAUTOSTRIP(CAI_Device_Lock, MAUTOSTRIP_VOID);
	m_bAvailable = false;

	//Is this a non-using lock?
	if (!_bUse)
		m_bLocked = true;
};

//Free device for usage, clearing any current usage values
void CAI_Device::Free()
{
	MAUTOSTRIP(CAI_Device_Free, MAUTOSTRIP_VOID);
	//Derived classes must clear their usage values as appropriate
	m_bAvailable = true;
	m_bLocked = false;
};
 
void CAI_Device::Pause(bool _bPause)
{
	m_bPause = _bPause;
};

//Advances the device one frame
void CAI_Device::OnRefresh()
{
	MAUTOSTRIP(CAI_Device_OnRefresh, MAUTOSTRIP_VOID);
	//Base functionality is to make device available. Note that this may not be done in all derived classes
	if (!m_bPause)
	{
		m_bAvailable = true;
		m_bLocked = false;
	}
};

void CAI_Device::OnPostActionsRefresh()
{
};

//Change AI user
void CAI_Device::ReInit(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_Device_ReInit, MAUTOSTRIP_VOID);
	m_pAI = _pAI;
};

//Get control data. Fails if device haven't been used this frame. Any data is written into the 
//given pointer adresses.
bool CAI_Device::GetData(int* _piData, CVec3Dfp32* _pVecData)
{
	MAUTOSTRIP(CAI_Device_GetData, false);
	//Just return whether this is in use or not as default
	return !IsAvailable() && !m_bLocked;
};

void CAI_Device::OnDeltaLoad(CCFile* _pFile)
{
	int8 Temp8;
	_pFile->ReadLE(Temp8);
	if (Temp8)
	{
		m_bPause = true;
	}
	else
	{
		m_bPause = false;
	}
};

void CAI_Device::OnDeltaSave(CCFile* _pFile)
{
	int8 Temp8;
	Temp8 = (bool)m_bPause;
	_pFile->WriteLE(Temp8);
};


//CAI_Device_Look
CAI_Device_Look::CAI_Device_Look()
{
	MAUTOSTRIP(CAI_Device_Look_ctor, MAUTOSTRIP_VOID);
	// m_Look = CVec3Dfp32(_FP32_MAX);
	// m_NextLook = CVec3Dfp32(_FP32_MAX);
	m_Look.Set(0.0f,0.0f,0.0f);
	m_NextLook.Set(0.0f,0.0f,0.0f);
	m_bSoft = false;
	m_bAlwaysSoft = false;
	m_SoftLookCosThreshold = -1.0f; //No threshold as default
	m_TargetDir = CVec3Dfp32(_FP32_MAX);
};

void CAI_Device_Look::UpdateNextLook()
{
	if (m_pAI)
	{
		m_NextLook = m_pAI->GetLookAngles();
	};
}

//Change look according to the given value or not at all as deafult
void CAI_Device_Look::Use(CVec3Dfp32 _Look, bool _bSoft, const CVec3Dfp32& _TargetDir)
{
	MAUTOSTRIP(CAI_Device_Look_Use, MAUTOSTRIP_VOID);
	if (IsAvailable())
	{
		// *** DEBUG ***
#ifndef M_RTM
		if ((m_pAI)&&(m_pAI->DebugTarget()))
		{
			CVec3Dfp32 HeadPos = m_pAI->GetLookPos();
			m_pAI->Debug_RenderWire(HeadPos,HeadPos+_TargetDir * 100,kColorRed,1.0f);
		}
#endif

		m_Look = _Look;
		m_bSoft = _bSoft || m_bAlwaysSoft;	
		m_TargetDir = _TargetDir;
		Lock(true);

		//Set next look direction...
		if (m_pAI)
		{
			m_NextLook = m_pAI->GetLookAngles() + _Look;
		};
	};
};

//Set look straight
void CAI_Device_Look::Use(bool _bSoft, const CVec3Dfp32& _TargetDir)
{
	MAUTOSTRIP(CAI_Device_Look_Use, MAUTOSTRIP_VOID);
	if (IsAvailable())
	{
		m_Look = CVec3Dfp32(0.0f);
		m_bSoft = _bSoft || m_bAlwaysSoft;	
		m_TargetDir = _TargetDir;
		Lock(true);

		//Set next look direction...
		if (m_pAI)
		{
			m_NextLook = m_pAI->GetLookAngles(m_pAI->m_pGameObject);
		};
	};
};

//The direction we'll be looking in after the current look command has been executed
CVec3Dfp32 CAI_Device_Look::GetNextLook()
{
 	MAUTOSTRIP(CAI_Device_Look_GetNextLook, CVec3Dfp32());
	return m_NextLook;
};

//Advances the device one frame
void CAI_Device_Look::OnRefresh()
{
	MAUTOSTRIP(CAI_Device_Look_OnRefresh, MAUTOSTRIP_VOID);
	CAI_Device::OnRefresh();
};

//Get control data. Fails if device haven't been used this frame. Any data is written into the 
//given pointer adresses.
bool CAI_Device_Look::GetData(int* _piData, CVec3Dfp32* _pVecData)
{
	MAUTOSTRIP(CAI_Device_Look_GetData, false);

	if (_pVecData)
	{
		if(m_Look == CVec3Dfp32(0.0f))
			return false;

		*_pVecData = m_Look;
		m_Look = CVec3Dfp32(0.0f);
	}
	return CAI_Device::GetData(_piData, _pVecData);
};

//Set soft look threshold (given angle in fractions)
void CAI_Device_Look::SetSoftLookThreshold(fp32 _Angle)
{
	m_SoftLookCosThreshold = M_Cos(_Angle * _PI2);
};


//Should we abort current look if we want to move in given direction? (Applies to soft look)
bool CAI_Device_Look::BadLookTarget(const CVec3Dfp32& _WantedMove)
{
	//Only applies to soft look with target direction
	if (!(m_bSoft || m_bAlwaysSoft) || 
		(m_TargetDir == CVec3Dfp32(_FP32_MAX)) || 
		((M_Fabs(_WantedMove[0]) < 0.01f) && (M_Fabs(_WantedMove[1]) < 0.01f) && (M_Fabs(_WantedMove[2]) < 0.01f)))
	{
		return false;
	}
	else
	{
		//Normalized vectors and use heading only if appropriate
		CVec3Dfp32 MoveDir = _WantedMove;
		if (!(m_pAI->m_UseFlags & CAI_Core::USE_SOFTLOOKPITCH))
			MoveDir[2] = 0;
		if (MoveDir.LengthSqr() != 1.0f)
			MoveDir.Normalize();
		
		if (!(m_pAI->m_UseFlags & CAI_Core::USE_SOFTLOOKPITCH))
			m_TargetDir[2] = 0;
		if (m_TargetDir.LengthSqr() != 1.0f)
			m_TargetDir.Normalize();

#ifndef M_RTM
		if (m_pAI->DebugTarget())
		{
			m_pAI->Debug_RenderWire(m_pAI->GetAimPosition(), m_pAI->GetAimPosition() + m_TargetDir * 200.0f, 0xffff0000);//DEBUG
			m_pAI->Debug_RenderWire(m_pAI->GetAimPosition(), m_pAI->GetAimPosition() + MoveDir * 200.0f, 0xff00ff00);//DEBUG
		}
#endif

		//Check cos of angle between vectors is too low then look target is bad
		return (m_TargetDir * MoveDir < m_SoftLookCosThreshold);
	}
};

bool CAI_Device_Look::BadLookDir(const CVec3Dfp32& _Dir)
{
	if (m_pAI)
	{
		if (m_pAI->GetBaseDir() * _Dir < m_SoftLookCosThreshold)
		{
			return(true);
		}
		else if ((m_pAI->GetPrevSpeed() > 0.01f)&&(m_pAI->GetPrevMoveDir() * _Dir < m_SoftLookCosThreshold))
		{
			return(true);
		}
	}
	return(false);
};


//Check if we're currently soft looking
bool CAI_Device_Look::IsSoftLook(bool _bAlwaysOnly)
{
	return ((!_bAlwaysOnly && m_bSoft) || m_bAlwaysSoft);
};

// Set/reset wether bot should softlook or not
void CAI_Device_Look::SetAlwaysLookSoft(bool _bAlways)
{
	m_bAlwaysSoft= _bAlways;
};





//CAI_Device_Move

CAI_Device_Move::CAI_Device_Move()
{
	MAUTOSTRIP(CAI_Device_Move_ctor, MAUTOSTRIP_VOID);
	m_Move = CVec3Dfp32(_FP32_MAX); 
	m_LastMove = CVec3Dfp32(0);
	m_bAnimControl = true;
};


void CAI_Device_Move::UseLastMove(bool _bAnimControl)
{
	MAUTOSTRIP(CAI_Device_Move_UselastMove, MAUTOSTRIP_VOID);
	Use(m_LastMove,_bAnimControl);
};

CVec3Dfp32 CAI_Device_Move::GetLastMove()
{
	MAUTOSTRIP(CAI_Device_Move_GetLastMove, MAUTOSTRIP_VOID);
	return(m_LastMove);
};

CVec3Dfp32 CAI_Device_Move::GetNextMove()
{
	MAUTOSTRIP(CAI_Device_Move_GetNextMove, MAUTOSTRIP_VOID);

	if (VALID_POS(m_Move))
	{
		return(m_Move);
	}
	else
	{
		return(CVec3Dfp32(0.0f));
	}
};


//Move according to the given value, or stop as default
void CAI_Device_Move::Use(CVec3Dfp32 _Move, bool _bAnimControl)
{
	MAUTOSTRIP(CAI_Device_Move_Use, MAUTOSTRIP_VOID);
	if (IsAvailable())
	{
		m_Move = _Move;
		m_bAnimControl = _bAnimControl;
		Lock(true);
		if (_Move == CVec3Dfp32(0))
		{
			m_pAI->ResetStuckInfo();
		}
		else if (m_pAI->m_DeviceStance.IsCrouching())
		{
			m_pAI->m_DeviceStance.Crouch(false);
		}
	};
};

//Advances the device one frame
void CAI_Device_Move::OnRefresh()
{
	MAUTOSTRIP(CAI_Device_Move_OnRefresh, MAUTOSTRIP_VOID);
	CAI_Device::OnRefresh();
	m_Move = CVec3Dfp32(_FP32_MAX);
	m_bAnimControl = true;
};


//Get control data. Never fails.
bool CAI_Device_Move::GetData(int* _piData, CVec3Dfp32* _pVecData)
{
	MAUTOSTRIP(CAI_Device_Move_GetData, false);

	//If device is unused, stop
	if (!CAI_Device::GetData(_piData, _pVecData))
	{
		if(m_LastMove == CVec3Dfp32(0))
		{
			m_pAI->m_pGameObject->AI_SetAnimControlMode(m_bAnimControl);
			return false;
		}

		*_pVecData = 0;
		*_piData = true;
		m_LastMove = CVec3Dfp32(0);
	}
	else
	{
		if(m_Move == CVec3Dfp32(_FP32_MAX) || m_Move == m_LastMove)
		{
			m_pAI->m_pGameObject->AI_SetAnimControlMode(m_bAnimControl);
			return false;
		}

		if(_pVecData)
		{
			*_pVecData = m_Move;
			*_piData = m_bAnimControl;
			m_LastMove = m_Move;
		}
	}

	return true;
};




//CAI_Device_Jump
//Jump once
void CAI_Device_Jump::Use()
{
	MAUTOSTRIP(CAI_Device_Jump_Use, MAUTOSTRIP_VOID);
	m_bJump = true;
	Lock(true);
};

CAI_Device_Jump::CAI_Device_Jump()
{
	MAUTOSTRIP(CAI_Device_Jump_ctor, MAUTOSTRIP_VOID);
	m_bJump = false;
};

//Advances the device one frame
void CAI_Device_Jump::OnRefresh()
{
	MAUTOSTRIP(CAI_Device_Jump_OnRefresh, MAUTOSTRIP_VOID);
	CAI_Device::OnRefresh();
	m_bJump = false;
};



//Sound device
const char* CAI_Device_Sound::ms_lTranslateSpeech[] =
{
	"IDLE_TALK",
	"IDLE_CALL",
	"IDLE_CALL_RESPONSE",
	"IDLE_AFFIRMATIVE",
	"IDLE_TO_WARY",
	"COMBAT_AFFIRMATIVE",	
	"SUSPICIOUS_SOUND",
	"DANGEROUS_SOUND",
	"SPOT_PLAYER_ODD",
	"SPOT_PLAYER_GUN",
	"ENEMY_SPOT_PLAYER",
	"AFRAID_PLEAD_FOR_LIFE",
	"AFRAID_PANIC",
	"ENEMY_RETREAT",
	"SPOT_PLAYER_DARKNESS",
	"IDLE_WARNING",
	"WARY_THREATEN",
	"ESCALATE_THREAT_HOSTILE0",
	"ESCALATE_THREAT_HOSTILE1",
	"ESCALATE_THREAT_HOSTILE2",
	"ESCALATE_THREAT_STOP",
	"ESCALATE_ODD_HOSTILE0",
	"ESCALATE_ODD_HOSTILE1",
	"ESCALATE_ODD_HOSTILE2",
	"ESCALATE_ODD_STOP",
	"SEARCH_START",
	"SEARCH_CONTINUE",
	"SEARCH_STOP",
	"SEARCH_STOP_RESPONSE",
	"COMBAT_DETECTED",
	"COMBAT_SPOTTED",
	"COMBAT_ALONE",
	"CHECKDEAD_IDLE_SPOT_CORPSE",
	"CHECKDEAD_DEATHCAUSE_GENERIC",
	"LEADER_COMBAT_FWD",
	"LEADER_COMBAT_FLANK",
	"LEADER_COMBAT_REAR",
	"LEADER_COMBAT_COVER",
	"COVER_TAUNT",
	"CHECKDEAD_IDLE_PISS_CORPSE",
	"COMBAT_ATTACKJUMP",
	"COMBAT_KILLJOY",
	"IDLE_ATTENTION",
	"IDLE_SHORTJUMP",
	"IDLE_LONGJUMP",
	"IDLE_AVOIDJUMP",
	"IDLE_BOREDOM",
	"IDLE_NOWAY",
	"IDLE_TO_DEADBODY",
	"IDLE_TO_RIVAL",
	NULL,
};

void CAI_Device_Sound::ResetDialogueStartTicks()
{
	if (!ms_bUseticksReset)
	{
		for (int i = 0; i < MAX_SOUNDS; i++)
		{
			ms_liDialogueTypes[i].m_LastUseTick = 0;
			ms_liDialogueTypes[i].m_LastUsed1 = -1;
			ms_liDialogueTypes[i].m_LastUsed2 = -1;
		}
		ms_bUseticksReset = true;
	}
};

// This function takes a billion years to compile with optimizations
#ifdef PLATFORM_WIN_PC
#pragma optimize("",off)
#endif

// It is pointless to create arrays if the data never change
//
// TAP_RCD<uint16> CAI_SoundType::m_pVariants;
// static uint16 lIdleTalk[] = { IDLE_TALK_1, IDLE_TALK_2, IDLE_TALK_3, ... IDLE_TALK_20 };
// pSound->m_pVariants.m_pArray = lIdleTalk;
// pSound->m_pVariants.m_Len = sizeof(lIdleTalk) / sizeof(lIdleTalk[0]);

void CAI_Device_Sound::SetupDialogueIndices()
{
	CAI_SoundType* pSound = NULL;

	if (!ms_bDialogSetup)
	{
		fp32 TicksPerSecond = m_pAI->GetAITicksPerSecond();

		// Talk your buddy
		pSound = &ms_liDialogueTypes[IDLE_TALK];
		pSound->m_lVariants.SetLen(20);
		pSound->m_lVariants[0] = IDLE_TALK_1;
		pSound->m_lVariants[1] = IDLE_TALK_2;
		pSound->m_lVariants[2] = IDLE_TALK_3;
		pSound->m_lVariants[3] = IDLE_TALK_4;
		pSound->m_lVariants[4] = IDLE_TALK_5;
		pSound->m_lVariants[5] = IDLE_TALK_6;
		pSound->m_lVariants[6] = IDLE_TALK_7;
		pSound->m_lVariants[7] = IDLE_TALK_8;
		pSound->m_lVariants[8] = IDLE_TALK_9;
		pSound->m_lVariants[9] = IDLE_TALK_10;
		pSound->m_lVariants[10] = IDLE_TALK_11;
		pSound->m_lVariants[11] = IDLE_TALK_12;
		pSound->m_lVariants[12] = IDLE_TALK_13;
		pSound->m_lVariants[13] = IDLE_TALK_14;
		pSound->m_lVariants[14] = IDLE_TALK_15;
		pSound->m_lVariants[15] = IDLE_TALK_16;
		pSound->m_lVariants[16] = IDLE_TALK_17;
		pSound->m_lVariants[17] = IDLE_TALK_18;
		pSound->m_lVariants[18] = IDLE_TALK_19;
		pSound->m_lVariants[19] = IDLE_TALK_20;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Call your buddy
		pSound = &ms_liDialogueTypes[IDLE_CALL];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = IDLE_CALL_1;
		pSound->m_lVariants[1] = IDLE_CALL_2;
		pSound->m_lVariants[2] = IDLE_CALL_3;
		pSound->m_lVariants[3] = IDLE_CALL_4;
		pSound->m_lVariants[4] = IDLE_CALL_5;
		pSound->m_lVariants[5] = IDLE_CALL_6;
		pSound->m_lVariants[6] = IDLE_CALL_7;
		pSound->m_lVariants[7] = IDLE_CALL_8;
		pSound->m_lVariants[8] = IDLE_CALL_9;
		pSound->m_lVariants[9] = IDLE_CALL_10;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Response to IDLE_CALL
		pSound = &ms_liDialogueTypes[IDLE_CALL_RESPONSE];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = IDLE_CALL_RESPONSE_1;
		pSound->m_lVariants[1] = IDLE_CALL_RESPONSE_2;
		pSound->m_lVariants[2] = IDLE_CALL_RESPONSE_3;
		pSound->m_lVariants[3] = IDLE_CALL_RESPONSE_4;
		pSound->m_lVariants[4] = IDLE_CALL_RESPONSE_5;
		pSound->m_lVariants[5] = IDLE_CALL_RESPONSE_6;
		pSound->m_lVariants[6] = IDLE_CALL_RESPONSE_7;
		pSound->m_lVariants[7] = IDLE_CALL_RESPONSE_8;
		pSound->m_lVariants[8] = IDLE_CALL_RESPONSE_9;
		pSound->m_lVariants[9] = IDLE_CALL_RESPONSE_10;
		pSound->m_MinInterval = (int32)(0.0f * TicksPerSecond);
		
		// Okey dokey, general purpose response
		pSound = &ms_liDialogueTypes[IDLE_AFFIRMATIVE];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = IDLE_AFFIRMATIVE_1;
		pSound->m_lVariants[1] = IDLE_AFFIRMATIVE_2;
		pSound->m_lVariants[2] = IDLE_AFFIRMATIVE_3;
		pSound->m_lVariants[3] = IDLE_AFFIRMATIVE_4;
		pSound->m_lVariants[4] = IDLE_AFFIRMATIVE_5;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Bot goes to wary
		pSound = &ms_liDialogueTypes[IDLE_TO_WARY];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = IDLE_TO_WARY_1;
		pSound->m_lVariants[1] = IDLE_TO_WARY_2;
		pSound->m_lVariants[2] = IDLE_TO_WARY_3;
		pSound->m_lVariants[3] = IDLE_TO_WARY_4;
		pSound->m_lVariants[4] = IDLE_TO_WARY_5;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Combat order response
		pSound = &ms_liDialogueTypes[COMBAT_AFFIRMATIVE];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = COMBAT_AFFIRMATIVE_1;
		pSound->m_lVariants[1] = COMBAT_AFFIRMATIVE_2;
		pSound->m_lVariants[2] = COMBAT_AFFIRMATIVE_3;
		pSound->m_lVariants[3] = COMBAT_AFFIRMATIVE_4;
		pSound->m_lVariants[4] = COMBAT_AFFIRMATIVE_5;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Hostile- response to bangs, crashes etc
		pSound = &ms_liDialogueTypes[SUSPICIOUS_SOUND];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = SUSPICIOUS_SOUND_1;
		pSound->m_lVariants[1] = SUSPICIOUS_SOUND_2;
		pSound->m_lVariants[2] = SUSPICIOUS_SOUND_3;
		pSound->m_lVariants[3] = SUSPICIOUS_SOUND_4;
		pSound->m_lVariants[4] = SUSPICIOUS_SOUND_5;
		pSound->m_lVariants[5] = SUSPICIOUS_SOUND_6;
		pSound->m_lVariants[6] = SUSPICIOUS_SOUND_7;
		pSound->m_lVariants[7] = SUSPICIOUS_SOUND_8;
		pSound->m_lVariants[8] = SUSPICIOUS_SOUND_9;
		pSound->m_lVariants[9] = SUSPICIOUS_SOUND_10;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Enemy- response to gunshots, explosions, screams etc
		pSound = &ms_liDialogueTypes[DANGEROUS_SOUND];
		pSound->m_lVariants.SetLen(3);
		pSound->m_lVariants[0] = DANGEROUS_SOUND_1;
		pSound->m_lVariants[1] = DANGEROUS_SOUND_2;
		pSound->m_lVariants[2] = DANGEROUS_SOUND_3;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Sound the alarm!
		pSound = &ms_liDialogueTypes[ENEMY_SPOT_PLAYER];
		pSound->m_lVariants.SetLen(20);
		pSound->m_lVariants[0] = ENEMY_SPOT_PLAYER_1;
		pSound->m_lVariants[1] = ENEMY_SPOT_PLAYER_2;
		pSound->m_lVariants[2] = ENEMY_SPOT_PLAYER_3;
		pSound->m_lVariants[3] = ENEMY_SPOT_PLAYER_4;
		pSound->m_lVariants[4] = ENEMY_SPOT_PLAYER_5;
		pSound->m_lVariants[5] = ENEMY_SPOT_PLAYER_6;
		pSound->m_lVariants[6] = ENEMY_SPOT_PLAYER_7;
		pSound->m_lVariants[7] = ENEMY_SPOT_PLAYER_8;
		pSound->m_lVariants[8] = ENEMY_SPOT_PLAYER_9;
		pSound->m_lVariants[9] = ENEMY_SPOT_PLAYER_10;
		pSound->m_lVariants[10] = ENEMY_SPOT_PLAYER_11;
		pSound->m_lVariants[11] = ENEMY_SPOT_PLAYER_12;
		pSound->m_lVariants[12] = ENEMY_SPOT_PLAYER_13;
		pSound->m_lVariants[13] = ENEMY_SPOT_PLAYER_14;
		pSound->m_lVariants[14] = ENEMY_SPOT_PLAYER_15;
		pSound->m_lVariants[15] = ENEMY_SPOT_PLAYER_16;
		pSound->m_lVariants[16] = ENEMY_SPOT_PLAYER_17;
		pSound->m_lVariants[17] = ENEMY_SPOT_PLAYER_18;
		pSound->m_lVariants[18] = ENEMY_SPOT_PLAYER_19;
		pSound->m_lVariants[19] = ENEMY_SPOT_PLAYER_20;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);

		// Temporarily go for cover
		pSound = &ms_liDialogueTypes[ENEMY_RETREAT];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = ENEMY_RETREAT_1;
		pSound->m_lVariants[1] = ENEMY_RETREAT_2;
		pSound->m_lVariants[2] = ENEMY_RETREAT_3;
		pSound->m_lVariants[3] = ENEMY_RETREAT_4;
		pSound->m_lVariants[4] = ENEMY_RETREAT_5;
		pSound->m_lVariants[5] = ENEMY_RETREAT_6;
		pSound->m_lVariants[6] = ENEMY_RETREAT_7;
		pSound->m_lVariants[7] = ENEMY_RETREAT_8;
		pSound->m_lVariants[8] = ENEMY_RETREAT_9;
		pSound->m_lVariants[9] = ENEMY_RETREAT_10;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Stop jerking around the civilian said
		pSound = &ms_liDialogueTypes[SPOT_PLAYER_ODD];
		pSound->m_lVariants.SetLen(3);
		pSound->m_lVariants[0] = SPOT_PLAYER_ODD_1;
		pSound->m_lVariants[1] = SPOT_PLAYER_ODD_2;
		pSound->m_lVariants[2] = SPOT_PLAYER_ODD_3;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// "Player has a gun" the civilian said
		pSound = &ms_liDialogueTypes[SPOT_PLAYER_GUN];
		pSound->m_lVariants.SetLen(3);
		pSound->m_lVariants[0] = SPOT_PLAYER_GUN_1;
		pSound->m_lVariants[1] = SPOT_PLAYER_GUN_2;
		pSound->m_lVariants[2] = SPOT_PLAYER_GUN_3;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Cower and plead for life
		pSound = &ms_liDialogueTypes[AFRAID_PLEAD_FOR_LIFE];
		pSound->m_lVariants.SetLen(3);
		pSound->m_lVariants[0] = AFRAID_PLEAD_FOR_LIFE_1;
		pSound->m_lVariants[1] = AFRAID_PLEAD_FOR_LIFE_2;
		pSound->m_lVariants[2] = AFRAID_PLEAD_FOR_LIFE_3;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Scared shitless
		pSound = &ms_liDialogueTypes[AFRAID_PANIC];
		pSound->m_lVariants.SetLen(3);
		pSound->m_lVariants[0] = AFRAID_PANIC_1;
		pSound->m_lVariants[1] = AFRAID_PANIC_2;
		pSound->m_lVariants[2] = AFRAID_PANIC_3;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Spotted player showing his DARKNESS weirdness, one shot experience.
		pSound = &ms_liDialogueTypes[SPOT_PLAYER_DARKNESS];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = SPOT_PLAYER_DARKNESS_1;
		pSound->m_lVariants[1] = SPOT_PLAYER_DARKNESS_2;
		pSound->m_lVariants[2] = SPOT_PLAYER_DARKNESS_3;
		pSound->m_lVariants[3] = SPOT_PLAYER_DARKNESS_4;
		pSound->m_lVariants[4] = SPOT_PLAYER_DARKNESS_5;
		pSound->m_lVariants[5] = SPOT_PLAYER_DARKNESS_6;
		pSound->m_lVariants[6] = SPOT_PLAYER_DARKNESS_7;
		pSound->m_lVariants[7] = SPOT_PLAYER_DARKNESS_8;
		pSound->m_lVariants[8] = SPOT_PLAYER_DARKNESS_9;
		pSound->m_lVariants[9] = SPOT_PLAYER_DARKNESS_10;
		pSound->m_MinInterval = (int32)(15.0f * TicksPerSecond);


		// Warn player for getting too close
		pSound = &ms_liDialogueTypes[IDLE_WARNING];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = IDLE_WARNING_1;
		pSound->m_lVariants[1] = IDLE_WARNING_2;
		pSound->m_lVariants[2] = IDLE_WARNING_3;
		pSound->m_lVariants[3] = IDLE_WARNING_4;
		pSound->m_lVariants[4] = IDLE_WARNING_5;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);

		// Threaten and whack the player
		pSound = &ms_liDialogueTypes[WARY_THREATEN];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = WARY_THREATEN_1;
		pSound->m_lVariants[1] = WARY_THREATEN_2;
		pSound->m_lVariants[2] = WARY_THREATEN_3;
		pSound->m_lVariants[3] = WARY_THREATEN_4;
		pSound->m_lVariants[4] = WARY_THREATEN_5;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);

		// Escalate threat 0
		pSound = &ms_liDialogueTypes[ESCALATE_THREAT_HOSTILE0];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = ESCALATE_THREAT_HOSTILE0_1;
		pSound->m_lVariants[1] = ESCALATE_THREAT_HOSTILE0_2;
		pSound->m_lVariants[2] = ESCALATE_THREAT_HOSTILE0_3;
		pSound->m_lVariants[3] = ESCALATE_THREAT_HOSTILE0_4;
		pSound->m_lVariants[4] = ESCALATE_THREAT_HOSTILE0_5;
		pSound->m_lVariants[5] = ESCALATE_THREAT_HOSTILE0_6;
		pSound->m_lVariants[6] = ESCALATE_THREAT_HOSTILE0_7;
		pSound->m_lVariants[7] = ESCALATE_THREAT_HOSTILE0_8;
		pSound->m_lVariants[8] = ESCALATE_THREAT_HOSTILE0_9;
		pSound->m_lVariants[9] = ESCALATE_THREAT_HOSTILE0_10;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);
		// Escalate threat 1
		pSound = &ms_liDialogueTypes[ESCALATE_THREAT_HOSTILE1];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = ESCALATE_THREAT_HOSTILE1_1;
		pSound->m_lVariants[1] = ESCALATE_THREAT_HOSTILE1_2;
		pSound->m_lVariants[2] = ESCALATE_THREAT_HOSTILE1_3;
		pSound->m_lVariants[3] = ESCALATE_THREAT_HOSTILE1_4;
		pSound->m_lVariants[4] = ESCALATE_THREAT_HOSTILE1_5;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);
		// Escalate threat 2
		pSound = &ms_liDialogueTypes[ESCALATE_THREAT_HOSTILE2];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = ESCALATE_THREAT_HOSTILE2_1;
		pSound->m_lVariants[1] = ESCALATE_THREAT_HOSTILE2_2;
		pSound->m_lVariants[2] = ESCALATE_THREAT_HOSTILE2_3;
		pSound->m_lVariants[3] = ESCALATE_THREAT_HOSTILE2_4;
		pSound->m_lVariants[4] = ESCALATE_THREAT_HOSTILE2_5;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);
		// Escalate threat stop
		pSound = &ms_liDialogueTypes[ESCALATE_THREAT_STOP];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = ESCALATE_THREAT_STOP_1;
		pSound->m_lVariants[1] = ESCALATE_THREAT_STOP_2;
		pSound->m_lVariants[2] = ESCALATE_THREAT_STOP_3;
		pSound->m_lVariants[3] = ESCALATE_THREAT_STOP_4;
		pSound->m_lVariants[4] = ESCALATE_THREAT_STOP_5;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);

		// Escalate odd 0
		pSound = &ms_liDialogueTypes[ESCALATE_ODD_HOSTILE0];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = ESCALATE_ODD_HOSTILE0_1;
		pSound->m_lVariants[1] = ESCALATE_ODD_HOSTILE0_2;
		pSound->m_lVariants[2] = ESCALATE_ODD_HOSTILE0_3;
		pSound->m_lVariants[3] = ESCALATE_ODD_HOSTILE0_4;
		pSound->m_lVariants[4] = ESCALATE_ODD_HOSTILE0_5;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);
		// Escalate odd 1
		pSound = &ms_liDialogueTypes[ESCALATE_ODD_HOSTILE1];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = ESCALATE_ODD_HOSTILE1_1;
		pSound->m_lVariants[1] = ESCALATE_ODD_HOSTILE1_2;
		pSound->m_lVariants[2] = ESCALATE_ODD_HOSTILE1_3;
		pSound->m_lVariants[3] = ESCALATE_ODD_HOSTILE1_4;
		pSound->m_lVariants[4] = ESCALATE_ODD_HOSTILE1_5;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);
		// Escalate odd 2
		pSound = &ms_liDialogueTypes[ESCALATE_ODD_HOSTILE2];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = ESCALATE_ODD_HOSTILE2_1;
		pSound->m_lVariants[1] = ESCALATE_ODD_HOSTILE2_2;
		pSound->m_lVariants[2] = ESCALATE_ODD_HOSTILE2_3;
		pSound->m_lVariants[3] = ESCALATE_ODD_HOSTILE2_4;
		pSound->m_lVariants[4] = ESCALATE_ODD_HOSTILE2_5;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);
		// Escalate odd stop
		pSound = &ms_liDialogueTypes[ESCALATE_ODD_STOP];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = ESCALATE_ODD_STOP_1;
		pSound->m_lVariants[1] = ESCALATE_ODD_STOP_2;
		pSound->m_lVariants[2] = ESCALATE_ODD_STOP_3;
		pSound->m_lVariants[3] = ESCALATE_ODD_STOP_4;
		pSound->m_lVariants[4] = ESCALATE_ODD_STOP_5;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);

		// Start investigating something you heard
		pSound = &ms_liDialogueTypes[SEARCH_START];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = SEARCH_START_1;
		pSound->m_lVariants[1] = SEARCH_START_2;
		pSound->m_lVariants[2] = SEARCH_START_3;
		pSound->m_lVariants[3] = SEARCH_START_4;
		pSound->m_lVariants[4] = SEARCH_START_5;
		pSound->m_MinInterval = (int32)(15.0f * TicksPerSecond);

		// Nah nothing at this spot
		pSound = &ms_liDialogueTypes[SEARCH_CONTINUE];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = SEARCH_CONTINUE_1;
		pSound->m_lVariants[1] = SEARCH_CONTINUE_2;
		pSound->m_lVariants[2] = SEARCH_CONTINUE_3;
		pSound->m_lVariants[3] = SEARCH_CONTINUE_4;
		pSound->m_lVariants[4] = SEARCH_CONTINUE_5;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Stop the investigation, he got away
		pSound = &ms_liDialogueTypes[SEARCH_STOP];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = SEARCH_STOP_1;
		pSound->m_lVariants[1] = SEARCH_STOP_2;
		pSound->m_lVariants[2] = SEARCH_STOP_3;
		pSound->m_lVariants[3] = SEARCH_STOP_4;
		pSound->m_lVariants[4] = SEARCH_STOP_5;
		pSound->m_MinInterval = (int32)(15.0f * TicksPerSecond);

		// Response to SEARCH_STOP
		pSound = &ms_liDialogueTypes[SEARCH_STOP_RESPONSE];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = SEARCH_STOP_RESPONSE_1;
		pSound->m_lVariants[1] = SEARCH_STOP_RESPONSE_2;
		pSound->m_lVariants[2] = SEARCH_STOP_RESPONSE_3;
		pSound->m_lVariants[3] = SEARCH_STOP_RESPONSE_4;
		pSound->m_lVariants[4] = SEARCH_STOP_RESPONSE_5;
		pSound->m_MinInterval = (int32)(3.0f * TicksPerSecond);

		// Combat DETECTED
		pSound = &ms_liDialogueTypes[COMBAT_DETECTED];
		pSound->m_lVariants.SetLen(20);
		pSound->m_lVariants[0] = COMBAT_DETECTED_1;
		pSound->m_lVariants[1] = COMBAT_DETECTED_2;
		pSound->m_lVariants[2] = COMBAT_DETECTED_3;
		pSound->m_lVariants[3] = COMBAT_DETECTED_4;
		pSound->m_lVariants[4] = COMBAT_DETECTED_5;
		pSound->m_lVariants[5] = COMBAT_DETECTED_6;
		pSound->m_lVariants[6] = COMBAT_DETECTED_7;
		pSound->m_lVariants[7] = COMBAT_DETECTED_8;
		pSound->m_lVariants[8] = COMBAT_DETECTED_9;
		pSound->m_lVariants[9] = COMBAT_DETECTED_10;
		pSound->m_lVariants[10] = COMBAT_DETECTED_11;
		pSound->m_lVariants[11] = COMBAT_DETECTED_12;
		pSound->m_lVariants[12] = COMBAT_DETECTED_13;
		pSound->m_lVariants[13] = COMBAT_DETECTED_14;
		pSound->m_lVariants[14] = COMBAT_DETECTED_15;
		pSound->m_lVariants[15] = COMBAT_DETECTED_16;
		pSound->m_lVariants[16] = COMBAT_DETECTED_17;
		pSound->m_lVariants[17] = COMBAT_DETECTED_18;
		pSound->m_lVariants[18] = COMBAT_DETECTED_19;
		pSound->m_lVariants[19] = COMBAT_DETECTED_20;
		pSound->m_MinInterval = (int32)(8.0f * TicksPerSecond);

		// Combat SPOTTED
		pSound = &ms_liDialogueTypes[COMBAT_SPOTTED];
		pSound->m_lVariants.SetLen(20);
		pSound->m_lVariants[0] = COMBAT_SPOTTED_1;
		pSound->m_lVariants[1] = COMBAT_SPOTTED_2;
		pSound->m_lVariants[2] = COMBAT_SPOTTED_3;
		pSound->m_lVariants[3] = COMBAT_SPOTTED_4;
		pSound->m_lVariants[4] = COMBAT_SPOTTED_5;
		pSound->m_lVariants[5] = COMBAT_SPOTTED_6;
		pSound->m_lVariants[6] = COMBAT_SPOTTED_7;
		pSound->m_lVariants[7] = COMBAT_SPOTTED_8;
		pSound->m_lVariants[8] = COMBAT_SPOTTED_9;
		pSound->m_lVariants[9] = COMBAT_SPOTTED_10;
		pSound->m_lVariants[10] = COMBAT_SPOTTED_11;
		pSound->m_lVariants[11] = COMBAT_SPOTTED_12;
		pSound->m_lVariants[12] = COMBAT_SPOTTED_13;
		pSound->m_lVariants[13] = COMBAT_SPOTTED_14;
		pSound->m_lVariants[14] = COMBAT_SPOTTED_15;
		pSound->m_lVariants[15] = COMBAT_SPOTTED_16;
		pSound->m_lVariants[16] = COMBAT_SPOTTED_17;
		pSound->m_lVariants[17] = COMBAT_SPOTTED_18;
		pSound->m_lVariants[18] = COMBAT_SPOTTED_19;
		pSound->m_lVariants[19] = COMBAT_SPOTTED_20;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Combat SPOTTED
		pSound = &ms_liDialogueTypes[COMBAT_ALONE];
		pSound->m_lVariants.SetLen(20);
		pSound->m_lVariants[0] = COMBAT_ALONE_1;
		pSound->m_lVariants[1] = COMBAT_ALONE_2;
		pSound->m_lVariants[2] = COMBAT_ALONE_3;
		pSound->m_lVariants[3] = COMBAT_ALONE_4;
		pSound->m_lVariants[4] = COMBAT_ALONE_5;
		pSound->m_lVariants[5] = COMBAT_ALONE_6;
		pSound->m_lVariants[6] = COMBAT_ALONE_7;
		pSound->m_lVariants[7] = COMBAT_ALONE_8;
		pSound->m_lVariants[8] = COMBAT_ALONE_9;
		pSound->m_lVariants[9] = COMBAT_ALONE_10;
		pSound->m_lVariants[10] = COMBAT_ALONE_11;
		pSound->m_lVariants[11] = COMBAT_ALONE_12;
		pSound->m_lVariants[12] = COMBAT_ALONE_13;
		pSound->m_lVariants[13] = COMBAT_ALONE_14;
		pSound->m_lVariants[14] = COMBAT_ALONE_15;
		pSound->m_lVariants[15] = COMBAT_ALONE_16;
		pSound->m_lVariants[16] = COMBAT_ALONE_17;
		pSound->m_lVariants[17] = COMBAT_ALONE_18;
		pSound->m_lVariants[18] = COMBAT_ALONE_19;
		pSound->m_lVariants[19] = COMBAT_ALONE_20;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Idle finding a corpse
		pSound = &ms_liDialogueTypes[CHECKDEAD_IDLE_SPOT_CORPSE];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = CHECKDEAD_IDLE_SPOT_CORPSE_1;
		pSound->m_lVariants[1] = CHECKDEAD_IDLE_SPOT_CORPSE_2;
		pSound->m_lVariants[2] = CHECKDEAD_IDLE_SPOT_CORPSE_3;
		pSound->m_lVariants[3] = CHECKDEAD_IDLE_SPOT_CORPSE_4;
		pSound->m_lVariants[4] = CHECKDEAD_IDLE_SPOT_CORPSE_5;
		pSound->m_lVariants[5] = CHECKDEAD_IDLE_SPOT_CORPSE_6;
		pSound->m_lVariants[6] = CHECKDEAD_IDLE_SPOT_CORPSE_7;
		pSound->m_lVariants[7] = CHECKDEAD_IDLE_SPOT_CORPSE_8;
		pSound->m_lVariants[8] = CHECKDEAD_IDLE_SPOT_CORPSE_9;
		pSound->m_lVariants[9] = CHECKDEAD_IDLE_SPOT_CORPSE_10;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Generic deathcause
		pSound = &ms_liDialogueTypes[CHECKDEAD_DEATHCAUSE_GENERIC];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = CHECKDEAD_DEATHCAUSE_GENERIC_1;
		pSound->m_lVariants[1] = CHECKDEAD_DEATHCAUSE_GENERIC_2;
		pSound->m_lVariants[2] = CHECKDEAD_DEATHCAUSE_GENERIC_3;
		pSound->m_lVariants[3] = CHECKDEAD_DEATHCAUSE_GENERIC_4;
		pSound->m_lVariants[4] = CHECKDEAD_DEATHCAUSE_GENERIC_5;
		pSound->m_lVariants[5] = CHECKDEAD_DEATHCAUSE_GENERIC_6;
		pSound->m_lVariants[6] = CHECKDEAD_DEATHCAUSE_GENERIC_7;
		pSound->m_lVariants[7] = CHECKDEAD_DEATHCAUSE_GENERIC_8;
		pSound->m_lVariants[8] = CHECKDEAD_DEATHCAUSE_GENERIC_9;
		pSound->m_lVariants[9] = CHECKDEAD_DEATHCAUSE_GENERIC_10;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Move fwd soldier!
		pSound = &ms_liDialogueTypes[LEADER_COMBAT_FWD];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = LEADER_COMBAT_FWD_1;
		pSound->m_lVariants[1] = LEADER_COMBAT_FWD_2;
		pSound->m_lVariants[2] = LEADER_COMBAT_FWD_3;
		pSound->m_lVariants[3] = LEADER_COMBAT_FWD_4;
		pSound->m_lVariants[4] = LEADER_COMBAT_FWD_5;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Flank him soldier!
		pSound = &ms_liDialogueTypes[LEADER_COMBAT_FLANK];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = LEADER_COMBAT_FLANK_1;
		pSound->m_lVariants[1] = LEADER_COMBAT_FLANK_2;
		pSound->m_lVariants[2] = LEADER_COMBAT_FLANK_3;
		pSound->m_lVariants[3] = LEADER_COMBAT_FLANK_4;
		pSound->m_lVariants[4] = LEADER_COMBAT_FLANK_5;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Ambush him soldier!
		pSound = &ms_liDialogueTypes[LEADER_COMBAT_REAR];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = LEADER_COMBAT_REAR_1;
		pSound->m_lVariants[1] = LEADER_COMBAT_REAR_2;
		pSound->m_lVariants[2] = LEADER_COMBAT_REAR_3;
		pSound->m_lVariants[3] = LEADER_COMBAT_REAR_4;
		pSound->m_lVariants[4] = LEADER_COMBAT_REAR_5;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Take cover soldier!
		pSound = &ms_liDialogueTypes[LEADER_COMBAT_COVER];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = LEADER_COMBAT_COVER_1;
		pSound->m_lVariants[1] = LEADER_COMBAT_COVER_2;
		pSound->m_lVariants[2] = LEADER_COMBAT_COVER_3;
		pSound->m_lVariants[3] = LEADER_COMBAT_COVER_4;
		pSound->m_lVariants[4] = LEADER_COMBAT_COVER_5;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Taunt player when hiding
		pSound = &ms_liDialogueTypes[COVER_TAUNT];
		pSound->m_lVariants.SetLen(20);
		pSound->m_lVariants[0] = COVER_TAUNT_1;
		pSound->m_lVariants[1] = COVER_TAUNT_2;
		pSound->m_lVariants[2] = COVER_TAUNT_3;
		pSound->m_lVariants[3] = COVER_TAUNT_4;
		pSound->m_lVariants[4] = COVER_TAUNT_5;
		pSound->m_lVariants[5] = COVER_TAUNT_6;
		pSound->m_lVariants[6] = COVER_TAUNT_7;
		pSound->m_lVariants[7] = COVER_TAUNT_8;
		pSound->m_lVariants[8] = COVER_TAUNT_9;
		pSound->m_lVariants[9] = COVER_TAUNT_10;
		pSound->m_lVariants[10] = COVER_TAUNT_11;
		pSound->m_lVariants[11] = COVER_TAUNT_12;
		pSound->m_lVariants[12] = COVER_TAUNT_13;
		pSound->m_lVariants[13] = COVER_TAUNT_14;
		pSound->m_lVariants[14] = COVER_TAUNT_15;
		pSound->m_lVariants[15] = COVER_TAUNT_16;
		pSound->m_lVariants[16] = COVER_TAUNT_17;
		pSound->m_lVariants[17] = COVER_TAUNT_18;
		pSound->m_lVariants[18] = COVER_TAUNT_19;
		pSound->m_lVariants[19] = COVER_TAUNT_20;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Darkling pissing on corpse
		pSound = &ms_liDialogueTypes[CHECKDEAD_IDLE_PISS_CORPSE];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = CHECKDEAD_IDLE_PISS_CORPSE_1;
		pSound->m_lVariants[1] = CHECKDEAD_IDLE_PISS_CORPSE_2;
		pSound->m_lVariants[2] = CHECKDEAD_IDLE_PISS_CORPSE_3;
		pSound->m_lVariants[3] = CHECKDEAD_IDLE_PISS_CORPSE_4;
		pSound->m_lVariants[4] = CHECKDEAD_IDLE_PISS_CORPSE_5;
		pSound->m_lVariants[5] = CHECKDEAD_IDLE_PISS_CORPSE_6;
		pSound->m_lVariants[6] = CHECKDEAD_IDLE_PISS_CORPSE_7;
		pSound->m_lVariants[7] = CHECKDEAD_IDLE_PISS_CORPSE_8;
		pSound->m_lVariants[8] = CHECKDEAD_IDLE_PISS_CORPSE_9;
		pSound->m_lVariants[9] = CHECKDEAD_IDLE_PISS_CORPSE_10;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Darkling attackjump
		pSound = &ms_liDialogueTypes[COMBAT_ATTACKJUMP];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = COMBAT_ATTACKJUMP_1;
		pSound->m_lVariants[1] = COMBAT_ATTACKJUMP_2;
		pSound->m_lVariants[2] = COMBAT_ATTACKJUMP_3;
		pSound->m_lVariants[3] = COMBAT_ATTACKJUMP_4;
		pSound->m_lVariants[4] = COMBAT_ATTACKJUMP_5;
		pSound->m_lVariants[5] = COMBAT_ATTACKJUMP_6;
		pSound->m_lVariants[6] = COMBAT_ATTACKJUMP_7;
		pSound->m_lVariants[7] = COMBAT_ATTACKJUMP_8;
		pSound->m_lVariants[8] = COMBAT_ATTACKJUMP_9;
		pSound->m_lVariants[9] = COMBAT_ATTACKJUMP_10;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Darkling happy for the killjoy
		pSound = &ms_liDialogueTypes[COMBAT_KILLJOY];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = COMBAT_KILLJOY_1;
		pSound->m_lVariants[1] = COMBAT_KILLJOY_2;
		pSound->m_lVariants[2] = COMBAT_KILLJOY_3;
		pSound->m_lVariants[3] = COMBAT_KILLJOY_4;
		pSound->m_lVariants[4] = COMBAT_KILLJOY_5;
		pSound->m_lVariants[5] = COMBAT_KILLJOY_6;
		pSound->m_lVariants[6] = COMBAT_KILLJOY_7;
		pSound->m_lVariants[7] = COMBAT_KILLJOY_8;
		pSound->m_lVariants[8] = COMBAT_KILLJOY_9;
		pSound->m_lVariants[9] = COMBAT_KILLJOY_10;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Darkling wants the players attention
		pSound = &ms_liDialogueTypes[IDLE_ATTENTION];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = IDLE_ATTENTION_1;
		pSound->m_lVariants[1] = IDLE_ATTENTION_2;
		pSound->m_lVariants[2] = IDLE_ATTENTION_3;
		pSound->m_lVariants[3] = IDLE_ATTENTION_4;
		pSound->m_lVariants[4] = IDLE_ATTENTION_5;
		pSound->m_lVariants[5] = IDLE_ATTENTION_6;
		pSound->m_lVariants[6] = IDLE_ATTENTION_7;
		pSound->m_lVariants[7] = IDLE_ATTENTION_8;
		pSound->m_lVariants[8] = IDLE_ATTENTION_9;
		pSound->m_lVariants[9] = IDLE_ATTENTION_10;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);
		
		// Darkling idly jumping a short distance
		pSound = &ms_liDialogueTypes[IDLE_SHORTJUMP];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = IDLE_SHORTJUMP_1;
		pSound->m_lVariants[1] = IDLE_SHORTJUMP_2;
		pSound->m_lVariants[2] = IDLE_SHORTJUMP_3;
		pSound->m_lVariants[3] = IDLE_SHORTJUMP_4;
		pSound->m_lVariants[4] = IDLE_SHORTJUMP_5;
		pSound->m_lVariants[5] = IDLE_SHORTJUMP_6;
		pSound->m_lVariants[6] = IDLE_SHORTJUMP_7;
		pSound->m_lVariants[7] = IDLE_SHORTJUMP_8;
		pSound->m_lVariants[8] = IDLE_SHORTJUMP_9;
		pSound->m_lVariants[9] = IDLE_SHORTJUMP_10;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Darkling idly jumping a long distance
		pSound = &ms_liDialogueTypes[IDLE_LONGJUMP];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = IDLE_LONGJUMP_1;
		pSound->m_lVariants[1] = IDLE_LONGJUMP_2;
		pSound->m_lVariants[2] = IDLE_LONGJUMP_3;
		pSound->m_lVariants[3] = IDLE_LONGJUMP_4;
		pSound->m_lVariants[4] = IDLE_LONGJUMP_5;
		pSound->m_lVariants[5] = IDLE_LONGJUMP_6;
		pSound->m_lVariants[6] = IDLE_LONGJUMP_7;
		pSound->m_lVariants[7] = IDLE_LONGJUMP_8;
		pSound->m_lVariants[8] = IDLE_LONGJUMP_9;
		pSound->m_lVariants[9] = IDLE_LONGJUMP_10;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Darkling avoid jumping
		pSound = &ms_liDialogueTypes[IDLE_AVOIDJUMP];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = IDLE_AVOIDJUMP_1;
		pSound->m_lVariants[1] = IDLE_AVOIDJUMP_2;
		pSound->m_lVariants[2] = IDLE_AVOIDJUMP_3;
		pSound->m_lVariants[3] = IDLE_AVOIDJUMP_4;
		pSound->m_lVariants[4] = IDLE_AVOIDJUMP_5;
		pSound->m_lVariants[5] = IDLE_AVOIDJUMP_6;
		pSound->m_lVariants[6] = IDLE_AVOIDJUMP_7;
		pSound->m_lVariants[7] = IDLE_AVOIDJUMP_8;
		pSound->m_lVariants[8] = IDLE_AVOIDJUMP_9;
		pSound->m_lVariants[9] = IDLE_AVOIDJUMP_10;
		pSound->m_MinInterval = (int32)(10.0f * TicksPerSecond);

		// Darkling is bored, will soon die unless something interesting shows up (retarget and killing are interesting)
		pSound = &ms_liDialogueTypes[IDLE_BOREDOM];
		pSound->m_lVariants.SetLen(10);
		pSound->m_lVariants[0] = IDLE_BOREDOM_1;
		pSound->m_lVariants[1] = IDLE_BOREDOM_2;
		pSound->m_lVariants[2] = IDLE_BOREDOM_3;
		pSound->m_lVariants[3] = IDLE_BOREDOM_4;
		pSound->m_lVariants[4] = IDLE_BOREDOM_5;
		pSound->m_lVariants[5] = IDLE_BOREDOM_6;
		pSound->m_lVariants[6] = IDLE_BOREDOM_7;
		pSound->m_lVariants[7] = IDLE_BOREDOM_8;
		pSound->m_lVariants[8] = IDLE_BOREDOM_9;
		pSound->m_lVariants[9] = IDLE_BOREDOM_10;
		pSound->m_MinInterval = (int32)(5.0f * TicksPerSecond);

		// Darkling refuses to follow orders
		pSound = &ms_liDialogueTypes[IDLE_NOWAY];
		pSound->m_lVariants.SetLen(5);
		pSound->m_lVariants[0] = IDLE_NOWAY_1;
		pSound->m_lVariants[1] = IDLE_NOWAY_2;
		pSound->m_lVariants[2] = IDLE_NOWAY_3;
		pSound->m_lVariants[3] = IDLE_NOWAY_4;
		pSound->m_lVariants[4] = IDLE_NOWAY_5;


		ms_bDialogSetup = true;
		ms_bUseticksReset = true;
	}	// End of 'if (!m_bDialogSetup)'
}
#ifdef PLATFORM_WIN_PC
#pragma optimize("", on)
#endif


// Initializes the static
CAI_Device_Sound::CAI_SoundType CAI_Device_Sound::ms_liDialogueTypes[MAX_SOUNDS];
bool CAI_Device_Sound::ms_bDialogSetup = false; 
bool CAI_Device_Sound::ms_bUseticksReset = false; 
int CAI_Device_Sound::ms_NextUseTick = 0;

CAI_Device_Sound::CAI_SoundType::CAI_SoundType()
{
	Clear();
};

void CAI_Device_Sound::CAI_SoundType::Clear()
{
	m_lVariants.Clear();
	m_lVariants.SetLen(1);
	m_lVariants[0] = INVALID_SOUND;
	m_LastUseTick = 0;
	m_MinInterval = AI_SOUNDTYPE_DEFAULT_INTERVAL;
	m_LastUsed1 = -1;
	m_LastUsed2 = -1;
};

void CAI_Device_Sound::CAI_SoundType::operator= (const CAI_SoundType& _Sound)
{
	m_lVariants = _Sound.m_lVariants;
	m_LastUseTick = _Sound.m_LastUseTick;
	m_MinInterval = _Sound.m_MinInterval;
};

int16 CAI_Device_Sound::CAI_SoundType::GetRandom()
{
	int Cur = (int32)(Random * 0.999f * m_lVariants.Len());
	return(Get(Cur));
	
};

int16 CAI_Device_Sound::CAI_SoundType::Get(int _Index)
{
	if (_Index < m_lVariants.Len())
	{
		return(m_lVariants[_Index]);
	}
	else
	{
		return(INVALID_SOUND);
	}
};

//Constructor
CAI_Device_Sound::CAI_Device_Sound()
{
	MAUTOSTRIP(CAI_Device_Sound_ctor, MAUTOSTRIP_VOID);

	m_iWait = 0;
	m_iDelayTicks = -1;
	m_iDelaySound = -1;
	m_iDelayPrio = 0;
};


void CAI_Device_Sound::ReInit(CAI_Core* _pAI)
{
	CAI_Device::ReInit(_pAI);
}


int CAI_Device_Sound::GetRandomDialogue(int _iType)
{
	int iVariant = GetRandomToUse(_iType);
	if (iVariant != -1)
	{
		return(ms_liDialogueTypes[_iType].m_lVariants[iVariant]);
	}
	else
	{
		return(0);
	}
};

int CAI_Device_Sound::GetRandomToUse(int _iType)
{
	if ((m_pAI->m_UseFlags & CAI_Core::USE_SCRIPTMUTE) &&
		(m_pAI->m_Script.IsValid()))
	{
		return(-1);
	}

	if ((IsAvailable())&&
		(_iType >= 0)&&
		(_iType < MAX_SOUNDS))
	{	// Check if enough time has passed
		if (m_pAI->GetGlobalAITick() >= ms_liDialogueTypes[_iType].m_LastUseTick + ms_liDialogueTypes[_iType].m_MinInterval)
		{
			int iRandom = (int32)(ms_liDialogueTypes[_iType].m_lVariants.Len() * 0.999f * Random);
			if ((iRandom == ms_liDialogueTypes[_iType].m_LastUsed1)||
				(iRandom == ms_liDialogueTypes[_iType].m_LastUsed2))
			{
				iRandom = (int32)(ms_liDialogueTypes[_iType].m_lVariants.Len() * 0.999f * Random);
				if ((iRandom == ms_liDialogueTypes[_iType].m_LastUsed1)||
					(iRandom == ms_liDialogueTypes[_iType].m_LastUsed2))
				{
					iRandom = (int32)(ms_liDialogueTypes[_iType].m_lVariants.Len() * 0.999f * Random);
				}
			}
			return(iRandom);
		}
		else
		{	// Too short a time between utterances
			return(-1);
		}
	}

	return(-1);
};

bool CAI_Device_Sound::SoundAvailable(int _iType)
{
	if ((IsAvailable())&&((_iType >= 0)&&(_iType < MAX_SOUNDS))&&
		(m_pAI->GetGlobalAITick() >= ms_liDialogueTypes[_iType].m_LastUseTick + ms_liDialogueTypes[_iType].m_MinInterval))
	{
		return(true);
	}
	else
	{
		return(false);
	}
};

void CAI_Device_Sound::PauseType(int _iType,fp32 _Duration)
{
	if ((_iType >= 0)&&(_iType < MAX_SOUNDS))
	{
		if (_Duration >= 0)
		{
			ms_liDialogueTypes[_iType].m_LastUseTick = (int32)(m_pAI->GetGlobalAITick() + _Duration * m_pAI->GetAITicksPerSecond());
		}
		else if (_Duration < 0)
		{
			ms_liDialogueTypes[_iType].m_LastUseTick = (int32)(m_pAI->GetGlobalAITick() + ms_liDialogueTypes[_iType].m_MinInterval);
		}
		else
		{
			ms_liDialogueTypes[_iType].m_LastUseTick = (int32)m_pAI->GetGlobalAITick();
		}
	}
};

int CAI_Device_Sound::GetDuration(int _iType,int _iVariant)
{
	if ((_iType >= 0)&&(_iType < MAX_SOUNDS)&&(_iVariant >= 0))
	{
		int index = ms_liDialogueTypes[_iType].Get(_iVariant);
		if (index != -1)
		{
			CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pAI->m_pGameObject);
			if (pChar)
			{
				int Duration = pChar->GetDialogueLength(index);
				return(Duration);
			}
		}

	}
	return(-1);
};

// Play _iVariant of _iType if  _Prio permits, returns -1 or the variant actually played (_iVariant)
int CAI_Device_Sound::UseRandom(int _iType, int _Prio, int _iVariant)
{
	MAUTOSTRIP(CAI_Device_Sound_UseRandom0, MAUTOSTRIP_VOID);
	//Never make noise if scriptmute and in script mode
	if ((m_pAI->m_UseFlags & CAI_Core::USE_SCRIPTMUTE)&&
		(m_pAI->m_Script.IsValid()))
	{
		return(-1);
	}

	if (m_pAI->IsPlayer())
	{
		return(-1);
	}

	// No dialogues while hurting (Duh!)
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
	if (pCD->m_AnimGraph2.GetStateFlagsLoCombined() & AG2_STATEFLAG_HURTACTIVE)
	{
		return(false);
	}

	int globalAITick = m_pAI->GetGlobalAITick();
	if ((IsAvailable())&&
		(_iType >= 0)&&
		(_iType < MAX_SOUNDS))
	{	// Check if enough time has passed
		if ((globalAITick >= ms_NextUseTick)&&(globalAITick >= ms_liDialogueTypes[_iType].m_LastUseTick + ms_liDialogueTypes[_iType].m_MinInterval))
		{
			int32 index = ms_liDialogueTypes[_iType].Get(_iVariant);
			if (index == -1)
			{
				return(-1);
			}
			int32 Duration = -1;
			CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pAI->m_pGameObject);
			if (pChar)
			{
				Duration = pChar->GetDialogueLength(index);
			}
#ifndef M_RTM
			if (!Duration)
			{
				CStr msg = "§cf00";
				msg += m_pAI->m_pGameObject->GetName();
				msg += " failed Dialogue: ";
				if ((_iType > CAI_Device_Sound::INVALID_SOUND)&&(_iType < CAI_Device_Sound::MAX_SOUNDS))
				{
					msg += CAI_Device_Sound::ms_lTranslateSpeech[_iType];
				}
				msg += CStrF(" Dialogueindex %d",index);
				ConOut(msg);
			}
#endif
			/*
			if (ms_liDialogueTypes[_iType].m_lVariants.Len() == 1)
			{	// Hash based random sound.
				CWObject_Message Msg(OBJMSG_RPG_SPEAK,0,_Prio);
				// The mind boggles as to why one (in year 2006 no less) must send the dialogue index as a f#*!ing string!
				CFStr Number;
				if (ms_liDialogueTypes[_iType].m_lVariants.Len() == 1)
				{
					Number = CFStrF("%d",Abs(_iType)); 
				}
				else
				{
					Number = CFStrF("%d",Abs(index)); 
				}
				Msg.m_pData = (char *)Number;
				Msg.m_DataSize = sizeof(Number);
				m_pAI->m_pServer->Message_SendToObject(Msg,m_pAI->m_pGameObject->m_iObject);
			}
			else
			*/

			{	// Regular, old school sound
				m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_SPEAK_OLD,index,_Prio),m_pAI->m_pGameObject->m_iObject);
			}

			Lock(true);
			ms_liDialogueTypes[_iType].m_LastUsed2 = ms_liDialogueTypes[_iType].m_LastUsed1;
			ms_liDialogueTypes[_iType].m_LastUsed1 = _iVariant;
			if (Duration > 0)
			{
				m_iWait = Duration;
			}
			else
			{
				m_iWait = AI_SOUNDTYPE_DEFAULT_DURATION;
			}
			ms_liDialogueTypes[_iType].m_LastUseTick = m_pAI->GetGlobalAITick() + m_iWait;
			ms_NextUseTick = m_pAI->GetGlobalAITick() + TruncToInt(fp32(m_iWait) * AI_GLOBAL_VOICE_SEPARATION_FACTOR);
			return(_iVariant);
		}
	}
	return(-1);
};

// Play any variant of _iType if _Prio allows, returns the variant played or -1
int CAI_Device_Sound::UseRandom(int _iType, int _Prio)
{
	MAUTOSTRIP(CAI_Device_Sound_UseRandom1, MAUTOSTRIP_VOID);

	if ((IsAvailable())&&
		(_iType >= 0)&&
		(_iType < MAX_SOUNDS))
	{	// Check if enough time has passed
		if (m_pAI->GetGlobalAITick() >= ms_liDialogueTypes[_iType].m_LastUseTick + ms_liDialogueTypes[_iType].m_MinInterval)
		{
			int32 iVariant = (int32)(ms_liDialogueTypes[_iType].m_lVariants.Len() * 0.999f * Random);
			if ((iVariant == ms_liDialogueTypes[_iType].m_LastUsed1)||
				(iVariant == ms_liDialogueTypes[_iType].m_LastUsed2))
			{
				iVariant = (int32)(ms_liDialogueTypes[_iType].m_lVariants.Len() * 0.999f * Random);
				if ((iVariant == ms_liDialogueTypes[_iType].m_LastUsed1)||
					(iVariant == ms_liDialogueTypes[_iType].m_LastUsed2))
				{
					iVariant = (int32)(ms_liDialogueTypes[_iType].m_lVariants.Len() * 0.999f * Random);
					if ((iVariant == ms_liDialogueTypes[_iType].m_LastUsed1)||
						(iVariant == ms_liDialogueTypes[_iType].m_LastUsed2))
					{	// Nah, we say nothing here
						m_iWait = AI_SOUNDTYPE_DEFAULT_DURATION;
						ms_liDialogueTypes[_iType].m_LastUseTick = m_pAI->GetGlobalAITick() + m_iWait;
						ms_NextUseTick = m_pAI->GetGlobalAITick() + TruncToInt(fp32(m_iWait) * AI_GLOBAL_VOICE_SEPARATION_FACTOR);
						ms_liDialogueTypes[_iType].m_LastUsed2 = ms_liDialogueTypes[_iType].m_LastUsed1;
						ms_liDialogueTypes[_iType].m_LastUsed1 = -1;	// Must do this, otherwise we can never say something when there are only one variant
						return(-1);
					}
				}
			}
			iVariant = UseRandom(_iType, _Prio, iVariant);
			return(iVariant);
		}
		else
		{	// Too short a time between utterances
			return(-1);
		}
	}

	return(-1);
};

// TODO: Add priority to all calls to UseRandom to differentiate between speech priorities
void CAI_Device_Sound::UseRandomDelayed(int _iType, int _Prio, int _DelayTicks)
{
	MAUTOSTRIP(CAI_Device_Sound_UseRandomDelayed, MAUTOSTRIP_VOID);

	if (_DelayTicks > 0)
	{
		if ((_Prio > m_iDelayPrio)||(m_iDelaySound <= 0))
		{
			m_iDelaySound = _iType;
			m_iDelayTicks = _DelayTicks;
			m_iDelayPrio = _Prio;
		}
	}
};

void CAI_Device_Sound::PlayDialogue(int _iDialogue, int _Prio, int _Wait)
{
	if ((m_pAI->m_UseFlags & CAI_Core::USE_SCRIPTMUTE)&&
		(m_pAI->m_Script.IsValid()))
	{
		return;
	}

	if (m_pAI->IsPlayer())
	{
		return;
	}

	if ((m_pAI->m_pGameObject)&&(IsAvailable())&&(m_pAI->m_pGameObject->AI_IsAlive()))
	{
		m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_SPEAK_OLD,_iDialogue,_Prio),m_pAI->m_pGameObject->m_iObject);
		Lock(true);
		if (_Wait > AI_SOUNDTYPE_DEFAULT_DURATION)
		{
			m_iWait = _Wait;
		}
		else
		{
			m_iWait = AI_SOUNDTYPE_DEFAULT_DURATION;
		}
	}
};

int CAI_Device_Sound::GetDialogueLength(int _iDialogue)
{
	int Duration = -1;
	CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pAI->m_pGameObject);
	if (pChar)
	{
		Duration = pChar->GetDialogueLength(_iDialogue);
		if (Duration > 0)
		{
			return(Duration);
		}
	}

	return(-1);
};

//Free device and reset waiting time
void CAI_Device_Sound::Free()
{
	MAUTOSTRIP(CAI_Device_Sound_Free, MAUTOSTRIP_VOID);
	CAI_Device::Free();
	m_iWait = 0;
};


//The currently playing sound will lock device for a number of frames as set by the 
//duration of the sound, or until device is explicitly freed. 
//Note that the current sound functionality does not support moving a sound source, 
//so all sounds will be emitted from the position where they where originally emitted.
void CAI_Device_Sound::OnRefresh()
{
	MAUTOSTRIP(CAI_Device_Sound_OnRefresh, MAUTOSTRIP_VOID);
	if (m_iWait > 0)
	{
		Lock(true);
		m_iWait--;
	}
	else
	{
		Free();
	}

	if (m_iDelayTicks > 0)
	{
		m_iDelayTicks--;
		if (m_iDelayTicks == 0)
		{
			int iVariant = UseRandom(m_iDelaySound,m_iDelayPrio);
#ifndef M_RTM
			if ((iVariant > -1)&&(m_pAI->m_pAIResources->ms_iDebugRenderTarget == m_pAI->GetObjectID()))
			{
				m_pAI->DebugPrintSoundReason(CStr("Delayed sound"),m_iDelaySound,iVariant);
			}
#endif
			m_iDelayTicks = -1;
			m_iDelaySound = -1;
			m_iDelayPrio = 0;
		}
	}
};

//This always fails, since device doesn't use the normal control frame routine
bool CAI_Device_Sound::GetData(int* _piData, CVec3Dfp32* _pVecData)
{
	MAUTOSTRIP(CAI_Device_Sound_GetData, false);
	return false;
};


//Savegame
void CAI_Device_Sound::OnDeltaLoad(CCFile* _pFile)
{
	int32 Temp;
	CAI_Device::OnDeltaLoad(_pFile);
	_pFile->ReadLE(Temp); m_iWait = Temp;
	if (!ms_bUseticksReset)
	{
		ResetDialogueStartTicks();
	}
};

void CAI_Device_Sound::OnDeltaSave(CCFile* _pFile)
{
	CAI_Device::OnDeltaSave(_pFile);
	int32 Temp = m_iWait;
	_pFile->WriteLE(Temp);
	ms_bUseticksReset = false;
};


//CAI_Device_Item
CAI_Device_Item::CAI_Device_Item()
{
	MAUTOSTRIP(CAI_Device_Item_ctor, MAUTOSTRIP_VOID);
	m_iUse = NONE;
	m_iDelayUse = NONE;
	m_iDelay = -1;
	m_ContinuousUse = -1;
};

//Use item.
void CAI_Device_Item::Use(int _Mode, CStr _Name)
{
	MAUTOSTRIP(CAI_Device_Item_Use, MAUTOSTRIP_VOID);
	if (IsAvailable() && _Mode)
	{
		m_iUse = _Mode;
		if (_Mode == SWITCH)
		{
			if (m_pAI && m_pAI->m_pServer && m_pAI->m_pGameObject)
			{
				CWObject_Message Switch(OBJMSG_CHAR_SELECTITEMBYNAME);
				Switch.m_pData = (char *)_Name;
				m_pAI->m_pServer->Message_SendToObject(Switch, m_pAI->m_pGameObject->m_iObject);
			}
		}

		Lock(true);
	};
};


//Use item after the given delay. Only one delayeduse can be active at one time, and 
//device will be locked until use
void CAI_Device_Item::UseDelayed(int _iDelay, int _iMode)
{
	MAUTOSTRIP(CAI_Device_Item_UseDelayed, MAUTOSTRIP_VOID);
	if (_iDelay <= 0)
		//Use immediately
		Use (_iMode);
	else
	{
		//Lock device and set delay-info
		Lock(true);
		m_iDelayUse = _iMode;
		m_iDelay = _iDelay;
	}
};


//Use item (not switch) for the given number of frames before releasing it
void CAI_Device_Item::UseContinuous(int _UseTime)
{
	Use();
	m_ContinuousUse = _UseTime;
};


//Advances the device one frame
void CAI_Device_Item::OnRefresh()
{
	MAUTOSTRIP(CAI_Device_Item_OnRefresh, MAUTOSTRIP_VOID);
	CAI_Device::OnRefresh();
	m_iUse = NONE;

	//Check for continuos usage
	if (m_ContinuousUse > 0)
	{
		Use();
		m_ContinuousUse--;
	}
	//Check for delayed usage
	else if (m_iDelay > 0)
	{
		if (m_iDelay == 1)
		{
			//Time to use stuff
			Use(m_iDelayUse);
			m_iDelay = -1;
		}
		else
		{
			//Decrease delay and lock device
			Lock(true);
			m_iDelay--;
		}
	}
};

//Get control data. Fails if device haven't been used this frame. Any data is written into the 
//given pointer adresses.
bool CAI_Device_Item::GetData(int* _piData, CVec3Dfp32* _pVecData)
{
	MAUTOSTRIP(CAI_Device_Item_GetData, false);
	if (m_iUse == SWITCH)
		return false;
	if (_piData)
		*_piData = m_iUse;
	return CAI_Device::GetData(_piData, _pVecData);
};

//Savegame
void CAI_Device_Item::OnDeltaLoad(CCFile* _pFile)
{
	int32 Temp;
	CAI_Device::OnDeltaLoad(_pFile);
	_pFile->ReadLE(Temp); m_ContinuousUse = Temp;
	_pFile->ReadLE(Temp); m_iDelayUse = Temp;
	_pFile->ReadLE(Temp); m_iDelay = Temp;
};

void CAI_Device_Item::OnDeltaSave(CCFile* _pFile)
{
	int32 Temp;
	CAI_Device::OnDeltaSave(_pFile);
	Temp = m_ContinuousUse; _pFile->WriteLE(Temp);
	Temp = m_iDelayUse; _pFile->WriteLE(Temp);
	Temp = m_iDelay; _pFile->WriteLE(Temp);
};

//CAI_Device_Weapon
CAI_Device_Weapon::CAI_Device_Weapon()
{
	MAUTOSTRIP(CAI_Device_Weapon_ctor, MAUTOSTRIP_VOID);
	m_Period = AI_WEAPON_SHOT_INTERVAL;
	m_NextUseTimer = 0;
	m_PressDuration = 0;
};

void CAI_Device_Weapon::SetPeriod(int _Period)
{
	m_Period = _Period;
};

void CAI_Device_Weapon::SetPressDuration(int _Duration)
{
	m_PressDuration = _Duration;
};

void CAI_Device_Weapon::UsePeriodic()
{
	if (m_pAI->m_Timer >= m_NextUseTimer)
	{
		Use();
	}
	else
	{
		Free();
	}
};

void CAI_Device_Weapon::OnPostActionsRefresh()
{
	if (m_pAI->m_Timer > m_NextUseTimer + m_PressDuration)
	{
		m_NextUseTimer = (int)(m_pAI->m_Timer + m_Period * (0.5f + Random));
	}
	CAI_Device_Item::OnPostActionsRefresh();
};

//Savegame
void CAI_Device_Weapon::OnDeltaLoad(CCFile* _pFile)
{
	int32 Temp;
	CAI_Device_Item::OnDeltaLoad(_pFile);
	_pFile->ReadLE(Temp); m_Period = Temp;
	_pFile->ReadLE(Temp); m_PressDuration = Temp;
};

void CAI_Device_Weapon::OnDeltaSave(CCFile* _pFile)
{
	int32 Temp;
	CAI_Device_Item::OnDeltaSave(_pFile);
	Temp = m_Period; _pFile->WriteLE(Temp);
	Temp = m_PressDuration; _pFile->WriteLE(Temp);
};

//CAI_Device_Stance
CAI_Device_Stance::CAI_Device_Stance()
{
	MAUTOSTRIP(CAI_Device_Stance_ctor, MAUTOSTRIP_VOID);
	m_iStance = IDLESTANCE_IDLE;
	m_iMinStance = IDLESTANCE_IDLE;
	m_iMaxStance = IDLESTANCE_COMBAT;
	m_bCrouching = false;
	m_bCrouchAvailable = false;
	m_bStanceAvailable = false;
	m_TargetInFOVCounter = 0;
	m_CrouchCounter = 0;
};

void CAI_Device_Stance::SetTargetInFOV(bool _State)
{
	if (_State)
	{
		m_TargetInFOVCounter = TARGET_IN_FOV_TICKS;
	}
	else
	{
		m_TargetInFOVCounter = 0;
	}
}

//Set stance. If not set every frame, stance reverts to standing, unless the optional peristent
//flag is set to true, in which case stance is kept until it's explicitly changed by another use.
void CAI_Device_Stance::Use(int _iStance)
{
	MAUTOSTRIP(CAI_Device_Stance_Use, MAUTOSTRIP_VOID);

	if (IsAvailable())
	{
		m_iStance = _iStance;
		Lock(true);
	}
};


//Advances the device one frame
void CAI_Device_Stance::OnRefresh()
{
	MAUTOSTRIP(CAI_Device_Stance_OnRefresh, MAUTOSTRIP_VOID);
	CAI_Device::OnRefresh();
	m_bCrouchAvailable = true;
	m_bStanceAvailable = true;
};

void CAI_Device_Stance::OnPostActionsRefresh()
{
	if (m_CrouchCounter > 0)
	{
		m_CrouchCounter--;
		if ((!m_bCrouching)&&(m_CrouchCounter == 0))
		{	// Rise my friend
			CWObject_Character::Char_SetPhysics(m_pAI->m_pGameObject,m_pAI->m_pServer,NULL,PLAYER_PHYS_STAND);
		}
	}

	// Stance clamping
	if (m_iMinStance > m_iStance)
	{
		SetIdleStance(m_iMinStance);
	}
	if (m_iMaxStance < m_iStance)
	{
		SetIdleStance(m_iMaxStance);
	}

	if ((m_iStance == IDLESTANCE_COMBAT)&&(m_iMinStance < IDLESTANCE_COMBAT))
	{
		if ((m_pAI->m_CharacterClass == CAI_Core::CLASS_BADGUY)||(m_pAI->m_CharacterClass == CAI_Core::CLASS_UNDEAD))
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
			if (pCD)
			{
				int stanceProperty = pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_STANCE);
				if ((m_TargetInFOVCounter)||(m_pAI->m_Weapon.GetWieldedArmsClass() < CAI_WeaponHandler::AI_ARMSCLASS_GUN))
				{
					if (stanceProperty != AG2_STANCETYPE_COMBAT)
					{
						pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STANCE,AG2_STANCETYPE_COMBAT);
					}
				}
				else
				{
					if (stanceProperty != AG2_STANCETYPE_HOSTILE)
					{
						pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STANCE,AG2_STANCETYPE_HOSTILE);
					}
				}
			}
		}
	}

	if (m_TargetInFOVCounter > 0)
	{
		m_TargetInFOVCounter--;
	}
	CAI_Device::OnPostActionsRefresh();
};

//Get control data. Fails if device haven't been used this frame. Any data is written into the 
//given pointer adresses.
bool CAI_Device_Stance::GetData(int* _piData, CVec3Dfp32* _pVecData)
{
	MAUTOSTRIP(CAI_Device_Stance_GetData, false);
	if (_piData)
	{
		*_piData = m_iStance;
	}
	return CAI_Device::GetData(_piData, _pVecData);
};

int CAI_Device_Stance::GetMinStance()
{
	return(m_iMinStance);
};

int CAI_Device_Stance::GetMaxStance()
{
	return(m_iMaxStance);
};

void CAI_Device_Stance::SetMinStance(int _Stance,bool _bRefresh)
{
	if (_Stance != m_iMinStance)
	{
		m_iMinStance = _Stance;
		if (_bRefresh)
		{
			m_bStanceAvailable = true;
			SetIdleStance(_Stance);
		}
	}
};

void CAI_Device_Stance::SetMaxStance(int _Stance,bool _bRefresh)
{
	if (_Stance != m_iMaxStance)
	{
		m_iMaxStance = _Stance;
		if (_bRefresh)
		{
			m_bStanceAvailable = true;
			SetIdleStance(_Stance);
		}
	}
};

//Change idle stance of character. Can be performed even when device is in use.
bool CAI_Device_Stance::SetIdleStance(int _Stance, int _iVariant)
{
	if (!m_pAI || !m_pAI->m_pServer || !m_pAI->m_pGameObject)
	{
		return(false);
	}

	_Stance = Max(m_iMinStance,_Stance);
	_Stance = Min(m_iMaxStance,_Stance);

	bool bChanged = false;
	if (m_bStanceAvailable)
	{
		if (_Stance != m_iStance)
		{	//Set idle stance		
			// First we test AG for sidesteps and bumps
			CWObject_Character* pChar = CWObject_Character::IsCharacter(m_pAI->m_pGameObject);
			if (pChar)
			{
				m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETIDLESTANCE, _Stance), m_pAI->m_pGameObject->m_iObject);
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
				if (pCD)
				{
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_GENERAL,_iVariant);
					int stanceProperty = pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_STANCE);
					switch(_Stance)
					{
					case IDLESTANCE_IDLE:
						{
							if (stanceProperty != AG2_STANCETYPE_IDLE)
							{
								pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STANCE,AG2_STANCETYPE_IDLE);
							}
							m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_IDLE);
						}
						break;

					case IDLESTANCE_HOSTILE:
						{
							if (m_iStance == IDLESTANCE_IDLE)
							{
								m_pAI->UseRandom(CStr("Idle to wary"),CAI_Device_Sound::IDLE_TO_WARY,CAI_Action::PRIO_ALERT);
							}
							switch(m_pAI->m_CharacterClass)
							{
							case CAI_Core::CLASS_CIV:
								{
									if (stanceProperty != AG2_STANCETYPE_WARY)
									{
										pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STANCE,AG2_STANCETYPE_WARY);
									}
									m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_WARY);
								}
								break;
							default:
								{
									if (stanceProperty != AG2_STANCETYPE_HOSTILE)
									{
										pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STANCE,AG2_STANCETYPE_HOSTILE);
									}
									m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_HOSTILE);
								}
								break;
							}
						}
						break;

					case IDLESTANCE_COMBAT:
						{
							switch(m_pAI->m_CharacterClass)
							{
							case CAI_Core::CLASS_CIV:
								{
									if (stanceProperty != AG2_STANCETYPE_PANIC)
									{
										pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STANCE,AG2_STANCETYPE_PANIC);
									}
									m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_PANIC);
								};
								break;
							case CAI_Core::CLASS_BADGUY:
							case CAI_Core::CLASS_UNDEAD:
								{
									// Handle stance when target NOT in FOV
									if ((m_TargetInFOVCounter)||(m_iMinStance >= IDLESTANCE_COMBAT)||(m_pAI->m_Weapon.GetWieldedArmsClass() < CAI_WeaponHandler::AI_ARMSCLASS_GUN))
									{
										if (stanceProperty != AG2_STANCETYPE_COMBAT)
										{
											pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STANCE,AG2_STANCETYPE_COMBAT);
										}
									}
									else
									{
										if (stanceProperty != AG2_STANCETYPE_HOSTILE)
										{
											pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STANCE,AG2_STANCETYPE_HOSTILE);
										}
									}
									m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_COMBAT);
								};
								break;
							default:
								{
									if (stanceProperty != AG2_STANCETYPE_COMBAT)
									{
										pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STANCE,AG2_STANCETYPE_COMBAT);
									}
									m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_COMBAT);
								};
								break;
							}
							break;
						}
						break;

					default:
						{
							if (stanceProperty != AG2_STANCETYPE_IDLE)
							{
								pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STANCE,AG2_STANCETYPE_IDLE);
							}
							m_pAI->m_DeviceFacial.Use(CAI_Device_Facial::FACIAL_IDLE);
						}
						break;
					}
				}
				m_iStance = _Stance;
				bChanged = true;
			}
		}
		m_bStanceAvailable = false;
	}

	return(bChanged);
};

void CAI_Device_Stance::Crouch(bool _State, int _MinCrouchDuration)
{
	if (m_bCrouchAvailable)
	{
		if (_State)
		{	// Crouch
			if (!m_bCrouching)
			{
				m_bCrouching = true;
				CWObject_Character::Char_SetPhysics(m_pAI->m_pGameObject,m_pAI->m_pServer,NULL,PLAYER_PHYS_CROUCH);
			}
			m_CrouchCounter = Max(m_CrouchCounter,_MinCrouchDuration);
		}
		else
		{	// Rise
			m_bCrouching = false;
			if ((!m_bCrouching)&&(m_CrouchCounter == 0))
			{	// Rise my friend
				CWObject_Character::Char_SetPhysics(m_pAI->m_pGameObject,m_pAI->m_pServer,NULL,PLAYER_PHYS_STAND);
			}
		}
		m_bCrouchAvailable = false;
	}
}

bool CAI_Device_Stance::IsCrouching()
{
	if ((m_bCrouching)||(m_CrouchCounter))
	{
		return(true);
	}
	else
	{
		return(false);
	}
}

int CAI_Device_Stance::GetIdleStance(bool _bFromAG)
{
	if (_bFromAG)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
		if (pCD)
		{
			int stanceProperty = pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_STANCE);
			switch(stanceProperty)
			{
			case AG2_STANCETYPE_IDLE:
				{
					m_iStance = IDLESTANCE_IDLE;
					return(IDLESTANCE_IDLE);
				}
				break;
			case AG2_STANCETYPE_HOSTILE:
			case AG2_STANCETYPE_WARY:
				{
					m_iStance = IDLESTANCE_HOSTILE;
					return(IDLESTANCE_HOSTILE);
				}
				break;
			case AG2_STANCETYPE_COMBAT:
			case AG2_STANCETYPE_PANIC:
				{
					m_iStance = IDLESTANCE_COMBAT;
					return(IDLESTANCE_COMBAT);
				}
				break;
			}
		}
	}
	return(m_iStance);
}

//Savegame
void CAI_Device_Stance::OnDeltaLoad(CCFile* _pFile)
{
	// int32 Temp32;
	int8 Temp8;
	CAI_Device::OnDeltaLoad(_pFile);
	_pFile->ReadLE(Temp8); m_bCrouching = (Temp8 != 0);
	_pFile->ReadLE(Temp8); m_bCrouchAvailable = (Temp8 != 0);
	_pFile->ReadLE(Temp8); m_iStance = Temp8;
	_pFile->ReadLE(Temp8); m_iMinStance = Temp8;
	_pFile->ReadLE(Temp8); m_iMaxStance = Temp8;
};

void CAI_Device_Stance::OnDeltaSave(CCFile* _pFile)
{
	// int32 Temp32;
	int8 Temp8;
	CAI_Device::OnDeltaSave(_pFile);
	Temp8 = m_bCrouching; _pFile->WriteLE(Temp8);
	Temp8 = m_bCrouchAvailable; _pFile->WriteLE(Temp8);
	Temp8 = m_iStance; _pFile->WriteLE(Temp8);
	Temp8 = m_iMinStance; _pFile->WriteLE(Temp8);
	Temp8 = m_iMaxStance; _pFile->WriteLE(Temp8);
};


//CAI_Device_Animation
//Animation handler; used for explicitly trigger character animations. 
//Note that animations are used asynchronously, and not through use of the normal control frame.

//Stop current animation if it's still playing
void CAI_Device_Animation::Stop()
{
	MAUTOSTRIP(CAI_Device_Animation_Stop, MAUTOSTRIP_VOID);
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

	//Reset stuff
	m_iWait = -1;
	m_bTorsoAnim = false;
	m_iAnim = 0;
	Free();
};

//Constructor
CAI_Device_Animation::CAI_Device_Animation()
{
	MAUTOSTRIP(CAI_Device_Animation_ctor, MAUTOSTRIP_VOID);
	m_iWait = -1;
	m_bTorsoAnim = false;
	m_iAnim = 0;
};


//Reduce wait and stop animation if it times out
void CAI_Device_Animation::OnRefresh()
{
	MAUTOSTRIP(CAI_Device_Animation_OnRefresh, MAUTOSTRIP_VOID);
	if (m_iWait > 0)
	{
		Lock(true);
		m_iWait--;
	}
	else if (m_iWait != -1)
	{
		Stop();
	}
};


//This always fails, since device doesn't use the normal control frame routine
bool CAI_Device_Animation::GetData(int* _piData, CVec3Dfp32* _pVecData)
{
	MAUTOSTRIP(CAI_Device_Animation_GetData, false);
	return false;
};

//Savegame
void CAI_Device_Animation::OnDeltaLoad(CCFile* _pFile)
{
	int32 Temp32;
	int8 Temp8;
	CAI_Device::OnDeltaLoad(_pFile);
	_pFile->ReadLE(Temp32); m_iWait = Temp32;
	_pFile->ReadLE(Temp8); m_bTorsoAnim = (Temp8 != 0);
	_pFile->ReadLE(Temp32); m_iAnim = Temp32;
};

void CAI_Device_Animation::OnDeltaSave(CCFile* _pFile)
{
	int32 Temp32;
	int8 Temp8;
	CAI_Device::OnDeltaSave(_pFile);
	Temp32 = m_iWait; _pFile->WriteLE(Temp32);
	Temp8 = m_bTorsoAnim; _pFile->WriteLE(Temp8);
	Temp32 = m_iAnim; _pFile->WriteLE(Temp32);
};


//Set animation, or stop current animation if no anim was given. The device will be locked for
//the playing time of the animation, plus the optionally specified settle time in frames.
//Animation will be torso anim if so specified. If the bForce argument i true anim is 
//always played, even if device is unavailable.
void CAI_Device_Animation::Use(int _iAnim, int _SettleTime, bool _bTorsoAnim, bool _bForce)
{
	MAUTOSTRIP(CAI_Device_Animation_Use, MAUTOSTRIP_VOID);
	//Should we stop current anim?
	if (_iAnim <= 0)
	{
		Stop();
	}
	//Should we start playing anim?
	else if (_bForce ||
			 IsAvailable())
	{
		m_bTorsoAnim = _bTorsoAnim;
		m_iAnim	= _iAnim;
		
		//Set animation sequence
		int Duration;
		if (_bTorsoAnim)
		{
			//Torso anim
			Duration = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, _iAnim, 1), m_pAI->m_pGameObject->m_iObject);
		}
		else
		{
			//Full anim
			Duration = m_pAI->m_pServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_PLAYANIMSEQUENCE, _iAnim), m_pAI->m_pGameObject->m_iObject);
		};

		m_iWait = Duration + _SettleTime;
		Lock(true);
	};
};


//Use random animation of the given type, forcing it to be played if so specified.
void CAI_Device_Animation::UseRandom(int _iType, int _SettleTime, bool _bForce)
{
	MAUTOSTRIP(CAI_Device_Animation_UseRandom, MAUTOSTRIP_VOID);
	if ((_bForce || IsAvailable()) &&
		(_iType >= 0) && (_iType < MAX_ANIMS))
	{
		int iAnim = 0;
		bool bTorsoAnim = false;
		fp32 Rnd = Random;
		switch (_iType)
		{
		case TAUNT_SHORT:
			{
				iAnim = (Rnd < (1.0f/3.0f)) ? ANIM_TAUNT_SHORT1 : ((Rnd < (2.0f/3.0f)) ? ANIM_TAUNT_SHORT2 : ANIM_TAUNT_SHORT3); 
			};
			break;
		case TAUNT_LONG:
			{
				iAnim = (Rnd < (1.0f/2.0f)) ? ANIM_TAUNT_LONG1 : ANIM_TAUNT_LONG2; 
			};
			break;
		case THREATEN:
			{
				iAnim = ANIM_THREATEN; 
			};
			break;
		};

		if (iAnim)
			Use(iAnim, _SettleTime, bTorsoAnim, _bForce);
	};
};



//CAI_Device_Melee
//Special device for melee actions.
CAI_Device_Melee::CAI_Device_Melee()
{
	m_Actions = NONE; 
};

//Take some actions
void CAI_Device_Melee::Use(int _Actions)
{
	if (IsAvailable())
	{
		m_Actions = _Actions;
		Lock(true);

		//Using melee device other than for initiating or breaking off locks look and move devices
		if (m_pAI && (_Actions != INITIATE) && (_Actions != BREAK_OFF))
		{
			m_pAI->m_DeviceLook.Lock();
			m_pAI->m_DeviceMove.Lock();
		}
	};
};

//Advances the device one frame
void CAI_Device_Melee::OnRefresh()
{
	CAI_Device::OnRefresh();
	m_Actions = NONE;
};

//Get control data. Fails if device haven't been used this frame. Any data is written into the 
//given pointer adresses.
bool CAI_Device_Melee::GetData(int* _piData, CVec3Dfp32* _pVecData)
{
	if (_piData)
		*_piData = m_Actions;
	return CAI_Device::GetData(_piData, _pVecData);
};

CAI_Device_Facial::CAI_Device_Facial()
{
	MAUTOSTRIP(CAI_Device_Facial_ctor, MAUTOSTRIP_VOID);
	m_iFacial = FACIAL_IDLE;
	m_iTempFacial = FACIAL_IDLE;
	m_TempFacialCounter = 0;
	m_bChanged = true;
	m_FacialGroup = FACIALGROUP_INVALID;
};

void CAI_Device_Facial::SetFacialGroup(int16 _FacialGroup)
{
	m_FacialGroup = _FacialGroup;
};

bool CAI_Device_Facial::Ag2SetFacial(int16 _FacialGroup, int16 _iFacial) const
{
	if ((m_pAI)&&(_FacialGroup != FACIALGROUP_INVALID)&&(_iFacial != FACIAL_INVALID))
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
		if (pCD)
		{
			/*
			if (m_pAI->DebugTarget())
			{
				CStr msg;
				msg += m_pAI->m_pGameObject->GetName();
				msg += CStrF(" facial %d timer %d",_iFacial,m_pAI->m_Timer);
				ConOut(msg);
			}
			*/
			CWAG2I_Context Context(m_pAI->m_pGameObject,m_pAI->m_pServer,pCD->m_GameTime);
			pCD->m_AnimGraph2.SendFacialImpulse(&Context,_FacialGroup,_iFacial);
			return(true);
		}
	}
	return(false);
};

int16 CAI_Device_Facial::GetFacial()
{
	if (m_TempFacialCounter > 0)
	{
		return(m_iTempFacial);
	}
	else
	{
		return(m_iFacial);
	}
};

// _Duration > 0: Sets a temp facial for _Duration ticks
// _Duration == 0: Sets a permanent facial
void CAI_Device_Facial::Use(int _iFacial, int32 _Duration)
{
	if (IsAvailable())
	{
		if (_Duration > 0)
		{
			if ((m_TempFacialCounter == 0)&&(m_iTempFacial != m_iFacial)&&(m_iTempFacial != _iFacial))
			{
				m_iTempFacial = _iFacial;
				m_TempFacialCounter = _Duration;
				m_bChanged = true;
			}
		}
		else
		{
			if (m_iFacial != _iFacial)
			{
				m_iFacial = _iFacial;
				if (!m_TempFacialCounter) {m_bChanged = true;}
			}
		}
		Lock(true);
	}
};

void CAI_Device_Facial::OnRefresh()
{
	MAUTOSTRIP(CAI_Device_Facial_OnRefresh, MAUTOSTRIP_VOID);

	if (m_bChanged)
	{
		if (m_TempFacialCounter > 0)
		{
			Ag2SetFacial(m_FacialGroup,m_iTempFacial);
		}
		else
		{
			Ag2SetFacial(m_FacialGroup,m_iFacial);
		}
		m_bChanged = false;
	}
	if (m_TempFacialCounter > 0)
	{
		m_TempFacialCounter--;
		if ((m_TempFacialCounter == 0)&&(m_iTempFacial != m_iFacial))
		{
			Ag2SetFacial(m_FacialGroup,m_iFacial);
			m_iTempFacial = 0;
			m_bChanged = false;
		}
	}
	CAI_Device::OnRefresh();
};

void CAI_Device_Facial::OnDeltaLoad(CCFile* _pFile)
{
	int32 Temp;
	CAI_Device::OnDeltaLoad(_pFile);
	_pFile->ReadLE(Temp); m_iFacial = Temp;
	_pFile->ReadLE(Temp); m_iTempFacial = Temp;
	_pFile->ReadLE(Temp); m_TempFacialCounter = Temp;
	m_bChanged = true;	// Force update to animgraph
};

void CAI_Device_Facial::OnDeltaSave(CCFile* _pFile)
{
	int32 Temp;
	CAI_Device::OnDeltaSave(_pFile);
	Temp = m_iFacial; _pFile->WriteLE(Temp);
	Temp = m_iTempFacial; _pFile->WriteLE(Temp);
	Temp = m_TempFacialCounter; _pFile->WriteLE(Temp);
};


//CAI_Device_Darkness
//Special device for using darkness powers.
CAI_Device_Darkness::CAI_Device_Darkness()
{
	m_Usage = USAGE_NONE;
	m_Power = POWER_INVALID;
	INVALIDATE_POS(m_Move);
};

//Take some actions.
void CAI_Device_Darkness::Use(int8 _Usage, int8 _Power, const CVec3Dfp32& _Move)
{
	if (IsAvailable())
	{
		m_Usage = _Usage;
		m_Power = (IsPowerAvailable(_Power) ? _Power : POWER_INVALID);

		// Always use force drain (with arbitrary amount) to avoid the new "tap-to-hide demon heads" system :P
		if (m_Usage & USAGE_DRAIN)
		{
			GatherPower(5);
			m_Usage &= ~USAGE_DRAIN;
		}

		// If we specify a power, we should always automatically select it
		if (m_Power != POWER_INVALID)
			m_Usage |= USAGE_SELECT;

		// This'll make a non-immobile bot run around out of control so we currently only 
		// allow it for immobile bots. Fix workaround of darkness power control if necessary.
		if (VALID_POS(_Move) && 
			(m_pAI->m_pGameObject->AI_GetPhysFlags() & PLAYER_PHYSFLAGS_IMMOBILE))
		{
			m_Move = _Move;
			m_pAI->m_DeviceMove.Lock();
		}
		Lock(true);
	};
};

// Force drain darkness, bypassing any normal lighting restrictions etc. If _bInstant is true we fill up to maximum darkness in one go.
void CAI_Device_Darkness::ForceDrainDarkness(bool _bInstant)
{
	if (_bInstant)
	{
		// Add a shitload of darkness juice. 
		int MaxDarkness = 1024;
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
		if (pCD)
			MaxDarkness = pCD->m_MaxDarkness;
		GatherPower(MaxDarkness);
	}
	else
	{
		// This is seems to be hardcoded in character code as well so wtf... There is a recharge rate key, but it isn't used.
		GatherPower(5);
	}
}

// Force remove all darkness juice.
void CAI_Device_Darkness::RemoveAllDarknessJuice()
{
	int MaxDarkness = 1024;
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
	if (pCD)
		MaxDarkness = pCD->m_MaxDarkness;
	GatherPower(-MaxDarkness);
}



int CAI_Device_Darkness::GetActivePower()
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
	if (pCD)
	{
		if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
			return POWER_CREEPING_DARK;
		if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DEMONARM)
			return POWER_DEMON_ARM;
		if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)
			return POWER_BLACK_HOLE;
	}
	return POWER_INVALID;
}

int CAI_Device_Darkness::GetSelectedPower()
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
	if (pCD)
	{
		int SelectedPlayerDarknessModePower = CWObject_Character::ResolveDarknessSelection(pCD->m_DarknessSelectedPower);
		if (SelectedPlayerDarknessModePower & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
			return POWER_CREEPING_DARK;
		if (SelectedPlayerDarknessModePower & PLAYER_DARKNESSMODE_POWER_DEMONARM)
			return POWER_DEMON_ARM;
		if (SelectedPlayerDarknessModePower & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)
			return POWER_BLACK_HOLE;
	}
	return POWER_INVALID;
}

bool CAI_Device_Darkness::IsPowerAvailable(int _Power)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_pAI->m_pGameObject);
	if (pCD)
	{
		int PlayerDarknessModePower = 0;
		if (_Power == POWER_CREEPING_DARK)
			PlayerDarknessModePower = PLAYER_DARKNESSMODE_POWER_CREEPINGDARK;	
		else if (_Power == POWER_DEMON_ARM)
			PlayerDarknessModePower = PLAYER_DARKNESSMODE_POWER_DEMONARM;	
		else if (_Power == POWER_BLACK_HOLE)
			PlayerDarknessModePower = PLAYER_DARKNESSMODE_POWER_BLACKHOLE;	
		return (pCD->m_DarknessPowersAvailable & PlayerDarknessModePower) != 0;
	}
	return false;
}

void CAI_Device_Darkness::GatherPower(int _Amount)
{
	CWObject_Message DrainMsg = CWObject_Message(OBJMSG_CHAR_ADDDARKNESSJUICE, _Amount);
	DrainMsg.m_pData = const_cast<char*>("1");
	DrainMsg.m_DataSize = sizeof("1");
	m_pAI->m_pServer->Message_SendToObject(DrainMsg, m_pAI->m_pGameObject->m_iObject);
}




//Advances the device one frame
void CAI_Device_Darkness::OnRefresh()
{
	m_Usage = USAGE_NONE;
	m_Power = POWER_INVALID;
	INVALIDATE_POS(m_Move);
	CAI_Device::OnRefresh();
};

//Get control data. Fails if device haven't been used this frame. Any data is written into the 
//given pointer addresses.
bool CAI_Device_Darkness::GetData(int* _piData, CVec3Dfp32* _pVecData)
{
	// Always drain if so used
	int Data = m_Usage & USAGE_DRAIN;

	// Should we select next power if necessary? (Note that this is set automatically in Use if a power is given)
	int SelectedPower = GetSelectedPower();
	if (m_Usage & USAGE_SELECT)
	{
		if (SelectedPower != m_Power)
		{
			// We have to change currently selected power. If power was POWER_INVALID we always change.
			Data |= USAGE_SELECT;
		}
	}

	// Should we activate/deactivate or do nothing?
	bool bActivate = (m_Usage & USAGE_ACTIVATE) != 0;
	int ActivePower = GetActivePower();
	if (ActivePower != POWER_INVALID)
	{
		if (!bActivate ||
			(ActivePower != m_Power))
		{
			// Stop using current power. We can start using the wanted power next tick. Some powers must be deactivated, 
			// others will deactivate if we do nothing.
			if (ActivePower == POWER_CREEPING_DARK)
				Data |= USAGE_ACTIVATE;
		}
		else if (SelectedPower == m_Power) 
		{
			// Continue using power. Some powers must be constantly activated, other will remain active if we do nothing
			if (m_Power == POWER_DEMON_ARM)
				Data |= USAGE_ACTIVATE;
		}
	}
	else if (bActivate &&
			 (m_Power != POWER_INVALID) && 
			 (SelectedPower == m_Power))
	{
		// No power active and wanted power selected, start using it!
		Data |= USAGE_ACTIVATE;
	}

	if (_pVecData && VALID_POS(m_Move))
	{
		*_pVecData = m_Move;
		Data |= USEMOVE;
	}

	if (_piData)
		*_piData = Data;

	return CAI_Device::GetData(_piData, _pVecData);
};

