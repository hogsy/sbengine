#include "PCH.h"
#include "AICore_RiotGuard.h"
#include "../../WObj_Char.h"
#include "../../WObj_CharMsg.h"

//Copy constructor
CAI_Core_RiotGuard::CAI_Core_RiotGuard(CAI_Core * _pAI_Core)
: CAI_Core(_pAI_Core)
{
	MAUTOSTRIP(CAI_Core_RiotGuard_ctor, MAUTOSTRIP_VOID);
};

//Initializer
void CAI_Core_RiotGuard::Init(CWObject * _pCharacter, CWorld_Server * _pServer)
{
	MAUTOSTRIP(CAI_Core_RiotGuard_Init, MAUTOSTRIP_VOID);
	CAI_Core::Init(_pCharacter, _pServer);
	

	m_bLooking = false;
	if (m_pGameObject)
	{
		//Riotguards don't blink
		m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_SETFLAGS, PLAYER_FLAGS_NOBLINK));
	}
};

/*
fp32 CAI_Core_RiotGuard::GetBaseSize()
{
	return 48;
};

fp32 CAI_Core_RiotGuard::GetHeight()
{
	return 96;
};

fp32 CAI_Core_RiotGuard::GetCrouchHeight()
{
	return (GetHeight());
};

fp32 CAI_Core_RiotGuard::GetStepHeight()
{
	return 32;
};

fp32 CAI_Core_RiotGuard::GetJumpHeight()
{
	return 32;
};
*/

fp32 CAI_Core_RiotGuard::GetWalkStepLength()
{
	return 24;
};

fp32 CAI_Core_RiotGuard::GetRunStepLength()
{
	return 48;
};

void CAI_Core_RiotGuard::AddFacePositionCmd(const CVec3Dfp32& _Pos)
{
	m_DeviceLook.Use();

	fp32 Heading = HeadingToPosition(m_pGameObject->GetPosition(), _Pos, 0.5f, GetHeading(m_pGameObject));
	if (M_Fabs(Heading) > 0.05f)
	{
		// /fp32 Turn = (Heading > 0) ? -1 : 1;
		fp32 Turn = -Heading;
		m_DeviceMove.Free();
		m_DeviceMove.Use(CVec3Dfp32(0,Turn,0));
		m_bLooking = true;
	}
};

void CAI_Core_RiotGuard::AddAimAtPositionCmd(const CVec3Dfp32& _Pos)
{
	AddFacePositionCmd(_Pos);
};
void CAI_Core_RiotGuard::OnLook(fp32 _Heading, fp32 _Pitch, int _Time)
{
	fp32 Radians = _Heading * _PI2;
	AddFacePositionCmd(m_pGameObject->GetPosition() + CVec3Dfp32(M_Cos(Radians), M_Sin(-Radians), 0));
};
void CAI_Core_RiotGuard::OnTrack(const CVec3Dfp32& _Pos, int _Time, bool bHeadingOnly)
{
	AddFacePositionCmd(_Pos);
};

//Adds commands to move towards the given position.
// If _Speed is higher than walkspeed we check if we could move a RunStepLength in a straight
// line and if that fails we cap movespeed to walk.
void CAI_Core_RiotGuard::AddMoveTowardsPositionCmd(const CVec3Dfp32& _Pos, fp32 _Speed, bool _bPerfectPlacement)
{
	//Make sure movetowardsposition overrides any use of movedevice to look
 	if (m_bLooking)
	{
		m_DeviceMove.Free();
		m_bLooking = false;
	}
	else if (!m_DeviceMove.IsAvailable())
	{	// Something else is using the movedevice
		return;
	}

	fp32 Speed = _Speed;
	if (Speed > m_ReleaseMaxSpeed)
	{
		Speed = m_ReleaseMaxSpeed;
	}
	else if (Speed < m_IdleMaxSpeed)
	{
		Speed = m_IdleMaxSpeed;
	}

	if (Speed > m_IdleMaxSpeed)
	{	// They're asking us to run, check if that is possible
		CVec3Dfp32 Diff = _Pos - m_pGameObject->GetPosition();
		fp32 Range = Diff.Length();
		if (Range < GetRunStepLength())
		{	// We don't run as the distance is too short
			Speed = m_IdleMaxSpeed;
		}
		else
		{
			CVec3Dfp32 StopPos;
			Diff = Diff * GetRunStepLength() / Range;
			if (!m_PathFinder.GridPF()->IsGroundTraversable(m_pGameObject->GetPosition(),
														m_pGameObject->GetPosition() + Diff,
														GetBaseSize(),
														GetHeight(),
														GetStepHeight(),
														GetJumpHeight(),
														&StopPos))
			{	// We don't run
				Speed = m_IdleMaxSpeed;
			}
		}
	}
	
	// MoveTurn
	CVec3Dfp32 MoveTurn = 0;
	fp32 Heading = HeadingToPosition(m_pGameObject->GetPosition(), _Pos, 0.5f, GetHeading(m_pGameObject));
	
	MoveTurn[0] = Speed / m_ReleaseMaxSpeed;
	if (M_Fabs(Heading) > 0.0f)
	{	// Angle off is high enough to warrant turning
		MoveTurn[1] = -Heading;
		if (M_Fabs(Heading) > 0.05f)
		{
			ResetStuckInfo();
		}
	}

	SetActivityScore(CAI_ActivityCounter::MOVING);
	// Draw a cross yellow where we want to go
	if (DebugRender() && m_pServer)
	{
		m_pServer->Debug_RenderVertex(_Pos,0xffffff00,3.0f,true);
	}

	m_DeviceMove.Use(MoveTurn);
};

//Checks if we've gotten stuck, assuming that we are moving along a path.
//Tightly tied to OnFollowPath and OnEscapeSequenceMove
bool CAI_Core_RiotGuard::IsStuck(const CVec3Dfp32& _Dest)
{
	//We're never actually stuck if move device is unavailable
	if (!m_DeviceMove.IsAvailable())
	{
		ResetStuckInfo();
		return false;
	}

	if (_Dest == CVec3Dfp32(_FP32_MAX))
		return false;

	if (m_bIsStuck)
	{
		return(true);
	}

	fp32 DistSqr = m_pGameObject->GetPosition().DistanceSqr(_Dest);

	//Have we gotten closer or have we just struck out for a new destination?
	if ((m_CurDestDistSqr == -1)||
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
	if (m_StuckCounter > m_StuckThreshold)
	{
		return(true);
	}
	else
	{
		return(false);
	}
};


//Make moves to escape from a "stuck" position
void CAI_Core_RiotGuard::OnEscapeSequenceMove(fp32 _Speed)
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
		{
			CurDest = m_SubDest;
		}
		else if (m_PathDestination != CVec3Dfp32(_FP32_MAX))
		{
			CurDest = m_PathDestination;
		}
		else
		{	// Oooh what ugly mess, we just assume the target dest in somewhere in front of us
			CurDest = m_pGameObject->GetPosition();
			CurDest += CVec3Dfp32::GetMatrixRow(m_pGameObject->GetPositionMatrix(),0) * 100.0f;
		}
		
		// 
		fp32 EscHeading = ((Random < 0.5f) ? 1 : -1) * (Random * 0.1f + 0.2f) + HeadingToPosition(m_pGameObject->GetPosition(),CurDest); 
		if (EscHeading < 0.0f)
		{
			EscHeading += 1.0f;
		}
		else if (EscHeading > 1.0f)
		{
			EscHeading -= 1.0f;
		}

		fp32 EscapeRange = GetBaseSize()+16.0f;
		m_EscSeqPos = FindGroundPosition(EscHeading,EscapeRange,EscapeRange,0.125f,0.25f);

		if (m_EscSeqPos == CVec3Dfp32(_FP32_MAX))
		{	// Turn the other direction
			EscHeading = -EscHeading;
			if (EscHeading < 0.0f)
			{
				EscHeading += 1.0f;
			}
			else if (EscHeading > 1.0f)
			{
				EscHeading -= 1.0f;
			}
			m_EscSeqPos = FindGroundPosition(EscHeading + 0.5f,EscapeRange,EscapeRange,0.125f,0.25f);
		}
	};

	if (m_EscSeqPos != CVec3Dfp32(_FP32_MAX))
	{
		// Here we should also propel the path a bit, neh?
		Debug_RenderWire(m_EscSeqPos, m_EscSeqPos + CVec3Dfp32(0,0,100),0xffff0000);//DEBUG
	}
	else
	{
		m_bIsStuck = false;
	}

	// We move towards escape pos for a second or until we get close enough
	if ((m_Timer % 60 == 0)||
		((m_EscSeqPos != CVec3Dfp32(_FP32_MAX)) && (m_pGameObject->GetPosition().DistanceSqr(m_EscSeqPos) < Sqr(GetBaseSize() * 0.5f))))	
	{
		m_bIsStuck = false;
		ResetPathSearch();	// Includes ResetStuckInfo();
		//If we've got a path destination, strike out anew!
		if (m_PathDestination != CVec3Dfp32(_FP32_MAX))
			OnMove(m_PathDestination, _Speed);
		else
			//...or just rest.
			OnIdle();
	}
	else if (m_EscSeqPos != CVec3Dfp32(_FP32_MAX))
	{	// Try to move towards escape position.
		AddMoveTowardsPositionCmd(m_EscSeqPos, Min(_Speed, XYDistance(m_pGameObject->GetPosition(), m_EscSeqPos)));
	}
};

//Move to position. Free move device if it's been used for looking
bool CAI_Core_RiotGuard::OnMove(const CVec3Dfp32& _NewDestination, fp32 _Speed,  bool _bFollowPartial, bool _bPerfectPlacement)
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

//Get type of AI
int CAI_Core_RiotGuard::GetType()
{
	MAUTOSTRIP(CAI_Core_RiotGuard_GetType, 0);
	return TYPE_RIOTGUARD;
};

//Updates the bot one frame. If the _bPassive flag is true, no actions are considered or generated, 
//but information is gathered.
void CAI_Core_RiotGuard::OnRefresh(bool _bPassive)
{
	//Aliens don't look
	if (m_bFirstRefresh && m_pGameObject)
		m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, PLAYER_CLIENTFLAGS_NOLOOK));

	m_bLooking = false;
	CAI_Core::OnRefresh(_bPassive);

	//Never use look device
	m_DeviceLook.Free();
	m_DeviceLook.Use();
};


//Handles incoming messages
aint CAI_Core_RiotGuard::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case 0:
		// Faux case constant to avoid warnings
	default:
		return CAI_Core::OnMessage(_Msg);
	}
}

//Handles any registry keys of a character that are AI-related
bool CAI_Core_RiotGuard::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CAI_Core_RiotGuard_OnEvalKey, false);
	//Try base class
	return CAI_Core::OnEvalKey(_pKey);
};


//Handles the event of us taking damage for some reason
void CAI_Core_RiotGuard::OnTakeDamage(int _iAttacker, int _Damage, int _iDamageType)
{
	CAI_Core::OnTakeDamage(_iAttacker,_Damage,_iDamageType);
	return;
};

