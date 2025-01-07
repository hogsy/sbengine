#include "PCH.h"
#include "AICore_SmallAlien.h"
#include "../../WObj_Char.h"
#include "../../WObj_CharMsg.h"

//Copy constructor
CAI_Core_SmallAlien::CAI_Core_SmallAlien(CAI_Core * _pAI_Core)
: CAI_Core(_pAI_Core)
{
	MAUTOSTRIP(CAI_Core_SmallAlien_ctor, MAUTOSTRIP_VOID);
};

fp32 CAI_Core_SmallAlien::GetMaxSpeedWalk(){return 6;};
fp32 CAI_Core_SmallAlien::GetMaxSpeedForward(){return 16;};
fp32 CAI_Core_SmallAlien::GetWalkStepLength(){return 32;};
fp32 CAI_Core_SmallAlien::GetRunStepLength(){return 64;};
/*
fp32 CAI_Core_SmallAlien::GetStepHeight() {return 12;};	// Higher ?
fp32 CAI_Core_SmallAlien::GetJumpHeight() {return 48;};	// Higher ?
fp32 CAI_Core_SmallAlien::GetHeight() {return 16;};
fp32 CAI_Core_SmallAlien::GetCrouchHeight() {return 16;};
*/

//Initializer
void CAI_Core_SmallAlien::Init(CWObject * _pCharacter, CWorld_Server * _pServer)
{
	MAUTOSTRIP(CAI_Core_SmallAlien_Init, MAUTOSTRIP_VOID);
	CAI_Core::Init(_pCharacter, _pServer);

	m_bLooking = false;
	if (m_pGameObject)
	{
		//Aliens don't blink
		m_pGameObject->OnMessage(CWObject_Message(OBJMSG_CHAR_SETFLAGS, PLAYER_FLAGS_NOBLINK));
	}
};


//Alien should never look only turn
void CAI_Core_SmallAlien::AddFacePositionCmd(const CVec3Dfp32& _Pos)
{
 	CAI_Core::AddFacePositionCmd(_Pos);
/*
	m_DeviceLook.Use();

	if (m_DeviceMove.IsAvailable())
	{
		fp32 Heading = HeadingToPosition(m_pGameObject->GetPosition(), _Pos, 0.5f, GetHeading(m_pGameObject));
		if (M_Fabs(Heading) > 0.01f)
		{
			//Turn in the relative direction we want to look at
			fp32 Radians = Heading * _PI2;
			fp32 TurnRadius = 1.0f; //Less than 0.1, more than 0.01
			CVec3Dfp32 TurnVec = CVec3Dfp32(M_Cos(Radians) * TurnRadius, M_Sin(-Radians) * TurnRadius, 0);
			m_DeviceMove.Use(TurnVec);
			m_bLooking = true;
		}
	}
*/
}

void CAI_Core_SmallAlien::AddAimAtPositionCmd(const CVec3Dfp32& _Pos)
{
	CAI_Core::AddAimAtPositionCmd(_Pos);
//	AddFacePositionCmd(_Pos);
};
void CAI_Core_SmallAlien::OnLook(fp32 _Heading, fp32 _Pitch, int _Time, bool _bLookSoft, bool _bMaintainLook)
{
	CAI_Core::OnLook(_Heading, _Pitch, _Time, _bLookSoft, _bMaintainLook);
//	fp32 Radians = _Heading * _PI2;
//	AddFacePositionCmd(m_pGameObject->GetPosition() + CVec3Dfp32(M_Cos(Radians), M_Sin(-Radians), 0));
};
void CAI_Core_SmallAlien::OnTrack(const CVec3Dfp32& _Pos, int _Time, bool bHeadingOnly, bool _bLookSoft)
{
 	CAI_Core::OnTrack(_Pos, _Time, bHeadingOnly, _bLookSoft);
	//AddFacePositionCmd(_Pos);
};


//Adds commands to move towards the given position.
void CAI_Core_SmallAlien::AddMoveTowardsPositionCmd(const CVec3Dfp32& _Pos, fp32 _Speed, bool _bPerfectPlacement)
{
	CAI_Core::AddMoveTowardsPositionCmd(_Pos, _Speed, _bPerfectPlacement);
/*
 	//Make sure movetowardsposition overrides any use of movedevice to look
 	if (m_bLooking)
	{
		m_DeviceMove.Free();
		m_bLooking = false;
	}
	else if (!m_DeviceMove.IsAvailable())
		return;

	fp32 MaxSpeed = GetMaxSpeedForward();
	fp32 Heading = HeadingToPosition(m_pGameObject->GetPosition(), _Pos, 0.5f, GetHeading(m_pGameObject));
	if (M_Fabs(Heading) > 0.25f)
	{
		//Turn on spot
		_Speed = 0.05f;
		ResetStuckInfo();
	}
	else if (_Speed == -1) 
		_Speed = 1.0f;
	else
		_Speed = _Speed / MaxSpeed;

	fp32 DistSqr = SqrDistanceToUs(_Pos);
	if ((_Speed > 0.5f) && (DistSqr < Sqr(GetRunStepLength())))
	{
		_Speed = 0.5f;
		//_Speed = sqrt(DistSqr) / GetMaxSpeedForward();
	}

	fp32 Radians = Heading * _PI2;
	CVec3Dfp32 MoveTurn = CVec3Dfp32(M_Cos(Radians) * _Speed, M_Sin(-Radians) * _Speed, 0);
	m_DeviceMove.Use(MoveTurn);

	//We're moving (at least)
	SetActivityScore(CAI_ActivityCounter::MOVING);
*/
};


//Checks if we've gotten stuck, assuming that we are moving along a path.
//Tightly tied to OnFollowPath and OnEscapeSequenceMove
bool CAI_Core_SmallAlien::IsStuck(const CVec3Dfp32& _Dest)
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


//Move to position. Free mode device if it's been used for looking
bool CAI_Core_SmallAlien::OnMove(const CVec3Dfp32& _NewDestination, fp32 _Speed,  bool _bFollowPartial, bool _bPerfectPlacement)
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


//Just swishing our tail
void CAI_Core_SmallAlien::OnIdle()
{
	m_DeviceMove.Use();
};


//Updates the bot one frame. If the _bPassive flag is true, no actions are considered or generated, 
//but information is gathered.
void CAI_Core_SmallAlien::OnRefresh(bool _bPassive)
{
	m_bLooking = false;
	CAI_Core::OnRefresh(_bPassive);
};


//Get type of AI
int CAI_Core_SmallAlien::GetType()
{
	MAUTOSTRIP(CAI_Core_SmallAlien_GetType, 0);
	return TYPE_SMALLALIEN;
};


//Handles any registry keys of a character that are AI-related
bool CAI_Core_SmallAlien::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CAI_Core_SmallAlien_OnEvalKey, false);
	//Try base class
	return CAI_Core::OnEvalKey(_pKey);
};


