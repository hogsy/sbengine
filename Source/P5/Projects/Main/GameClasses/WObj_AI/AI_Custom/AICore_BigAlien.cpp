#include "PCH.h"
#include "AICore_BigAlien.h"
#include "../../WObj_Char.h"
#include "../../WObj_CharMsg.h"

#define MACRO_WRITEVERIFY {uint32 Apa = 0x81920467; _pFile->WriteLE(Apa);}
#define MACRO_READVERIFY {uint32 Apa; _pFile->ReadLE(Apa); M_ASSERT(Apa == 0x81920467, CStrF("Load/save mismatch in file '%s' on line %i", __FILE__, __LINE__)); };

//Copy constructor
CAI_Core_BigAlien::CAI_Core_BigAlien(CAI_Core * _pAI_Core)
: CAI_Core(_pAI_Core)
{
	MAUTOSTRIP(CAI_Core_BigAlien_ctor, MAUTOSTRIP_VOID);
	m_ImmuneTick = 0;
	m_ActivityMode = MODE_DEFAULT;
};

//Initializer
void CAI_Core_BigAlien::Init(CWObject * _pCharacter, CWorld_Server * _pServer)
{
	MAUTOSTRIP(CAI_Core_BigAlien_Init, MAUTOSTRIP_VOID);
	CAI_Core::Init(_pCharacter, _pServer);
	
	m_bLooking = false;
	if (m_pGameObject)
	{
		//Aliens don't blink
		m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_SETFLAGS, PLAYER_FLAGS_NOBLINK));
	}
};

/*
fp32 CAI_Core_BigAlien::GetBaseSize()
{
	return 64;
};

fp32 CAI_Core_BigAlien::GetHeight()
{
	return 32;
};

fp32 CAI_Core_BigAlien::GetCrouchHeight()
{
	return 32;
};

fp32 CAI_Core_BigAlien::GetStepHeight()
{
	return 32;
};

fp32 CAI_Core_BigAlien::GetJumpHeight()
{
	return 32;
};
*/
fp32 CAI_Core_BigAlien::GetWalkStepLength()
{
	return 24;
};

fp32 CAI_Core_BigAlien::GetRunStepLength()
{
	return 48;
};


//Alien should never look
void CAI_Core_BigAlien::AddFacePositionCmd(const CVec3Dfp32& _Pos)
{
	m_DeviceLook.Use();

	if (m_DeviceMove.IsAvailable())
	{
 		fp32 Heading = HeadingToPosition(m_pGameObject->GetPosition(), _Pos, 0.5f, GetHeading(m_pGameObject));
  		if (M_Fabs(Heading) > 0.0625f)
		{
			//We are making a standing turn
			fp32 Radians = Heading * 2 * _PI;
			m_DeviceMove.Use(CVec3Dfp32(M_Cos(Radians) * 0.05f, M_Sin(-Radians) * 0.05f, 0));
			ResetStuckInfo();
			m_bLooking = true;
		}
	}
}
void CAI_Core_BigAlien::AddAimAtPositionCmd(const CVec3Dfp32& _Pos)
{
	AddFacePositionCmd(_Pos);
};
void CAI_Core_BigAlien::OnLook(fp32 _Heading, fp32 _Pitch, int _Time, bool _bLookSoft, bool _bMaintainLook)
{
	fp32 Radians = _Heading * 2 * _PI;
	AddFacePositionCmd(m_pGameObject->GetPosition() + CVec3Dfp32(M_Cos(Radians), M_Sin(-Radians), 0));
};
void CAI_Core_BigAlien::OnTrack(const CVec3Dfp32& _Pos, int _Time, bool bHeadingOnly, bool _bLookSoft)
{
 	AddFacePositionCmd(_Pos);
};


//Adds commands to move towards the given position.
void CAI_Core_BigAlien::AddMoveTowardsPositionCmd(const CVec3Dfp32& _Pos, fp32 _Speed, bool _bPerfectPlacement)
{
//	CAI_Core::AddMoveTowardsPositionCmd(_Pos, _Speed, _bPerfectPlacement);

	Debug_RenderWire(m_pGameObject->GetPosition()+ CVec3Dfp32(0,0,10), _Pos);//DEBUG

	//Make sure movetowardsposition overrides any use of movedevice to look
 	if (m_bLooking)
	{ 
		m_DeviceMove.Free();
		m_bLooking = false;
	}
	else if (!m_DeviceMove.IsAvailable())
		return;
	
	if (_Speed == -1) 
		_Speed = GetMaxSpeedForward();
	
	CVec3Dfp32 MoveTurn = 0;
	fp32 Heading = HeadingToPosition(m_pGameObject->GetPosition(), _Pos, 0.5f, GetHeading(m_pGameObject));

	//Try to move towards the position
//	fp32 Radians = Heading * 2 * _PI;
//	m_DeviceMove.Use(CVec3Dfp32(M_Cos(Radians), M_Sin(-Radians), 0));
//	return;

 	if (M_Fabs(Heading) > 0.0625f)
	{
		//We are making a standing turn
		fp32 Radians = Heading * 2 * _PI;
		MoveTurn[0] = M_Cos(Radians) * 0.2f;
		MoveTurn[1] = M_Sin(-Radians) * 0.2f;
		ResetStuckInfo();
	}
 	else
	{
		//Move forwards
		MoveTurn[0] = 1.0f;
	}
	
	m_DeviceMove.Use(MoveTurn);

	//We're moving (at least)
	SetActivityScore(CAI_ActivityCounter::MOVING);
};

//Checks if we've gotten stuck, assuming that we are moving along a path.
//Tightly tied to OnFollowPath and OnEscapeSequenceMove
bool CAI_Core_BigAlien::IsStuck(const CVec3Dfp32& _Dest)
{
	//We're never actually stuck if move device is unavailable
	if (!m_DeviceMove.IsAvailable())
		return false;

	//We consider ourselves to have gotten stuck if we fail to get closer to our current destination
	//during three consecutive frames time.
	fp32 DistSqr = ((_Dest == CVec3Dfp32(_FP32_MAX)) ? _FP32_MAX : m_pGameObject->GetPosition().DistanceSqr(_Dest));

	//Have we gotten closer or have we just struck out for a new destination?
	if ((m_CurDestDistSqr == -1) ||
		(DistSqr < (m_CurDestDistSqr - 0.1f)))
	{
		//Jupp, reset stuck counter and set new min distance 
		ResetStuckInfo();
		m_CurDestDistSqr = DistSqr;
	}
	else
	{
		//Nope, we might be stuck
		m_StuckCounter++;
	};

	//We're stuck if counter exceeds a certain limit
	return (m_StuckCounter > m_StuckThreshold);
};


//Make moves to escape from a "stuck" position
void CAI_Core_BigAlien::OnEscapeSequenceMove(fp32 _Speed)
{
	//StartBM("OnEscapeSequence");
	if (!m_bIsStuck)
	{
		//Init escape sequence
		m_bIsStuck = true;

		//We're never stuck with a brand new escape sequence!
		ResetStuckInfo();

		//Get initial escape position
		CVec3Dfp32 CurDest;
		if (m_SubDest != CVec3Dfp32(_FP32_MAX))
			CurDest = m_SubDest;
		else if (m_PathDestination != CVec3Dfp32(_FP32_MAX))
			CurDest = m_PathDestination;
		else
			CurDest = m_pGameObject->GetPosition() + CVec3Dfp32::GetMatrixRow(m_pGameObject->GetPositionMatrix(), 0) * 100;
		fp32 EscHeading = ((Random < 0.5f) ? 1 : -1) * (Random * 0.1f + 0.2f) + HeadingToPosition(m_pGameObject->GetPosition(), CurDest); 

		m_EscSeqPos = FindGroundPosition(EscHeading, 64, 16, 0.125f, 0.25f);

		if (m_EscSeqPos == CVec3Dfp32(_FP32_MAX))
			m_EscSeqPos = FindGroundPosition(EscHeading + 0.5f, 64, 16, 0.125f, 0.25f);
	};

	if (m_EscSeqPos != CVec3Dfp32(_FP32_MAX)) Debug_RenderWire(m_EscSeqPos, m_EscSeqPos + CVec3Dfp32(0,0,100));//DEBUG
	if (m_EscSeqPos != CVec3Dfp32(_FP32_MAX)) Debug_RenderWire(m_EscSeqPos, m_pGameObject->GetPosition(), 0xffff0000);//DEBUG
	if (m_EscSeqPos != CVec3Dfp32(_FP32_MAX)) Debug_RenderWire(m_pGameObject->GetPosition(), m_pGameObject->GetPosition() + CVec3Dfp32(0,0,100), 0xffff0000);//DEBUG

	//Check if we've gotten stuck while escaping
	if (IsStuck(m_EscSeqPos))
	{
		//We've gotten stuck again, get new escape position in random direction
		m_EscSeqPos = FindGroundPosition(Random, 64, 16, 0.25f);
		ResetStuckInfo();
	};

	//Small chance of abandoning escape move every few frames or if we reach escape position
	if ( (!(m_Timer % 20) &&
		(Random < 0.1f)) ||
		((m_EscSeqPos != CVec3Dfp32(_FP32_MAX)) &&
		(m_pGameObject->GetPosition().DistanceSqr(m_EscSeqPos) < 16*16)) )	
	{
		m_bIsStuck = false;
		ResetPathSearch();

		//If we've got a path destination, strike out anew!
		if (m_PathDestination != CVec3Dfp32(_FP32_MAX))
			OnMove(m_PathDestination, _Speed);
		else
			//...or just rest.
			OnIdle();
	}
	//Should we get frustrated?
	else if (m_EscSeqPos == CVec3Dfp32(_FP32_MAX))
	{
		OnFrustration();
	}
	else  
	{
		//Try to move towards escape position.
		AddMoveTowardsPositionCmd(m_EscSeqPos, Min(_Speed, XYDistance(m_pGameObject->GetPosition(), m_EscSeqPos)));
	}

	//StopBM();
};


//Move to position. Free mode device if it's been used for looking
bool CAI_Core_BigAlien::OnMove(const CVec3Dfp32& _NewDestination, fp32 _Speed,  bool _bFollowPartial, bool _bPerfectPlacement)
{
	if (m_bLooking)
	{
		m_DeviceMove.Free();
		m_bLooking = false;
	}
	else if (!m_DeviceMove.IsAvailable())
		return false;

	return CAI_Core::OnMove(_NewDestination, _Speed, _bFollowPartial, _bPerfectPlacement);
}


//Act frustrated
void CAI_Core_BigAlien::OnFrustration()
{
	//Lash out with primary and secondary attacks and turn randomly
	if ((m_Timer % 20 == 0) && (Random < 0.5f))
	{
		switch ((int)(Random * 2))
		{
		case 0:
			m_DeviceItem.Use();
		case 1:
			m_DeviceMove.Use(CVec3Dfp32(0,1,0));
		case 2:
			m_DeviceMove.Use(CVec3Dfp32(0,-1,0));
		default:
			m_DeviceWeapon.Use();
		}
	}
};


//Handles impulses
bool CAI_Core_BigAlien::OnImpulse(int _iImpulse, int _iSender, int _iObject, const CVec3Dfp32& _VecParam, void * _pData)
{
	//Check for server and character
	if (!m_pServer || !m_pGameObject)
		return false;

	switch (_iImpulse)
	{
	case IMPULSE_SPECIAL:
		{
			if (CStr((char *)_pData).UpperCase() == "SLEEP")
			{
				//Go to sleep
				m_ActivityMode = MODE_SLEEPING;
				m_KB.SetAlertness(CAI_KnowledgeBase::ALERTNESS_SLEEPING);
				CWObject_Message AGAction(OBJMSG_CHAR_MOVETOKEN);
				AGAction.m_pData = (void*)"SetState.Action0";
				//AGAction.m_pData = (char *)CStr("SetState.Action0");
				m_pServer->Message_SendToObject(AGAction, m_pGameObject->m_iObject);
			}
			else if (CStr((char *)_pData).UpperCase() == "YAWN")
			{
				//Yawn
				CWObject_Message AGAction(OBJMSG_CHAR_MOVETOKEN);
				AGAction.m_pData = (void*)"SetState.Action1";
				m_pServer->Message_SendToObject(AGAction, m_pGameObject->m_iObject);
			}
			else if (CStr((char *)_pData).UpperCase() == "AWAKE")
			{
				//Awake and fall down if hanging from ceiling
				m_ActivityMode = MODE_DEFAULT;
				m_KB.SetAlertness(m_KB.GetAlertness(true));
				CWObject_Message AGAction(OBJMSG_CHAR_MOVETOKEN);
				AGAction.m_pData = (void*)"SetState.Action2";
				m_pServer->Message_SendToObject(AGAction, m_pGameObject->m_iObject);

				//Hack! To avoid fall damage, make alien immune for a while
				m_ImmuneTick = m_pServer->GetGameTick() + 60;
				m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_IMMUNE, 1));
			}
			return true;
		}
	default:
		return CAI_Core::OnImpulse(_iImpulse, _iSender, _iObject, _VecParam, _pData);
	}
}

//Get type of AI
int CAI_Core_BigAlien::GetType()
{
	MAUTOSTRIP(CAI_Core_BigAlien_GetType, 0);
	return TYPE_BIGALIEN;
};

//Updates the bot one frame. If the _bPassive flag is true, no actions are considered or generated, 
//but information is gathered.
void CAI_Core_BigAlien::OnRefresh(bool _bPassive)
{
	//Aliens don't look
	if (m_bFirstRefresh && m_pGameObject)
		m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, PLAYER_CLIENTFLAGS_NOLOOK));

	if ((m_ImmuneTick != 0) && m_pServer && (m_pServer->GetGameTick() > m_ImmuneTick))
	{
		//Turn off immunity
		m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_IMMUNE, 0));
 		m_ImmuneTick = 0;
	}

	m_bLooking = false;
	CAI_Core::OnRefresh(_bPassive);

	//Never use look device
	m_DeviceLook.Free();
	m_DeviceLook.Use();
};


//Handles incoming messages
aint CAI_Core_BigAlien::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GETACTIVITYMODE :
		{
			return m_ActivityMode;
		}
	default:
		return CAI_Core::OnMessage(_Msg);
	}
}

//Handles any registry keys of a character that are AI-related
bool CAI_Core_BigAlien::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CAI_Core_BigAlien_OnEvalKey, false);
	//Try base class
	return CAI_Core::OnEvalKey(_pKey);
};

void CAI_Core_BigAlien::OnDeltaLoad(CCFile* _pFile)
{
	MACRO_WRITEVERIFY;
	_pFile->ReadLE(m_ActivityMode);
	_pFile->ReadLE(m_ImmuneTick);
};
void CAI_Core_BigAlien::OnDeltaSave(CCFile* _pFile)
{
	MACRO_WRITEVERIFY;
	_pFile->WriteLE(m_ActivityMode);
	_pFile->WriteLE(m_ImmuneTick);
};


