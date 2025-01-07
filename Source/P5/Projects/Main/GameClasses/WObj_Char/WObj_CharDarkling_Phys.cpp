/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Movement handling for Darklings.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		Movement handling for Darklings.

	Comments:

	History:		
		050824		Created file, moved code from WObj_CharDarkling_ClientData.cpp
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharDarkling_ClientData.h"
#include "../../GameWorld/WClientMod_Defines.h"
#include "../WObj_Misc/WObj_ScenePoint.h"
#include "../WObj_Game/WObj_GameMod.h"
#include "WObj_CharShapeshifter.h"

#define DBG_OUT /**/DO_NOT/**/ M_TRACE
#define DBG_RENDER 0

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Helpers
|__________________________________________________________________________________________________
\*************************************************************************************************/
M_INLINE bool IsValid(fp32 _x) { return (_x > -1e10f && _x < 1e10f); }
M_INLINE void Invalidate(fp32& _x) { _x = _FP32_MAX; }


// Gives angle between _Vec and "ideal" Vec, calculated using _Up-vector
// Angle is returned in the interval -0.5 .. 0.5
/*
static fp32 GetAngle(const CVec3Dfp32& _Up, const CVec3Dfp32& _Vec, const CVec3Dfp32& _Ref)
{
	CVec3Dfp32 IdealVec;
	_Up.CrossProd(_Ref, IdealVec);
	fp32 u = _Vec * IdealVec;
	fp32 v = _Vec * _Up;
	fp32 Angle = atan2f(v, u) * (1.0f / _PI2);
	return Angle;
}
*/

// This will calculate angle between _From and _To, when using _Axis for rotation
fp32 GetPlanarRotation(const CVec3Dfp32& _From, const CVec3Dfp32& _To, const CVec3Dfp32& _Axis)
{
	CVec3Dfp32 Ref;
	_Axis.CrossProd(_From, Ref);
	fp32 u = _To * _From;
	fp32 v = _To * Ref;
	fp32 Angle = atan2f(v, u) * (1.0f / _PI2);
	return Angle;
}

// This will calculate angle and rotation axis for rotating between _From and _To.
// If _From and _To are 180 degrees apart, rotation will be around _Ref
/*
static void GetFreeRotation(const CVec3Dfp32& _From, const CVec3Dfp32& _To, const CVec3Dfp32& _Ref, CAxisRotfp32& _Result)
{
	CVec3Dfp32 bisec = _From + _To;
	fp32 Len = bisec.Length();
	if (Len > 0.01f)
	{ // ok, there is one axis to rotate around
		bisec *= (1.0f / Len);
		fp32 CosHalfAngle = _From * bisec;
		if (CosHalfAngle > 0.999f)
		{ // no difference, no need to calculate stuff
			_Result.m_Angle = 0.0f;
			_Result.m_Axis = _Ref;
		}
		else
		{ // calculate axis and angle
			_From.CrossProd(bisec, _Result.m_Axis);
			_Result.m_Axis.Normalize();
			_Result.m_Angle = -M_ACos(CosHalfAngle) * (1.0f/_PI);
		}
	}
	else
	{ // 180 degrees apart, rotate around '_Ref'
		_Result.m_Axis = _Ref;
		_Result.m_Angle = GetPlanarRotation(_From, _To, _Ref);
	}
}
*/


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Movement handling
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CWO_CharDarkling_ClientData::Phys_Move(const CSelection& _Selection, fp32 _dTime, const CVec3Dfp32& _UserVel, bool _bPredicted)
{
	// If darkling is crawling out of a body or wall which is totaly unrealistic then real world physical movement laws
	// don't apply of how one can jump from one wall to another or even walk in the ceiling.
	if(Char_IsSpawning() || CWObject_Character::Char_GetPhysType(m_pObj) == PLAYER_PHYS_DEAD)
	{
		//UpdateMatrices();
		return;
	}

	if ((m_Flags & DARKLING_FLAGS_CAN_CLIMB) && !(m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_NOWALLCLIMB))
		m_pObj->m_PhysAttrib.m_StepSize = 0.0f; 
	else
		m_pObj->m_PhysAttrib.m_StepSize = PLAYER_STEPSIZE; 


/*CVec3Dfp32 SavedVelo = m_pObj->GetMoveVelocity();
CMat4Dfp32 SavedPos = m_pObj->GetPositionMatrix();
CVec3Dfp32 SavedGravity = m_Gravity;
uint16 SavedFlags = m_Flags;
CMat4Dfp32 Saved_MatLook = m_MatLook;
CMat4Dfp32 Saved_MatBody = m_MatBody;
CMat4Dfp32 Saved_LastPos = m_pObj->GetLastPositionMatrix();
int16 Saved_Phys_bInAir = m_Phys_bInAir;
int16 Saved_Phys_nInAirFrames = m_Phys_nInAirFrames;
CVec3Dfp32 Saved_Control_Move = m_Control_Move;
CVec3Dfp32 Saved_Control_Look = m_Control_Look;//*/

	parent::Phys_Move(_Selection, _dTime, _UserVel, _bPredicted);
	// Reset rot velocity...
	CAxisRotfp32 RotVel;
	RotVel.Unit();
	m_pWPhysState->Object_SetRotVelocity(m_pObj->m_iObject, RotVel);
	UpdateMatrices();

/*CVec3Dfp32 NewVelo = m_pObj->GetMoveVelocity();
CMat4Dfp32 NewPos = m_pObj->GetPositionMatrix();
CVec3Dfp32 Diff1 = NewPos.GetRow(3) - SavedPos.GetRow(3);
fp32 k1 = SavedVelo * NewVelo;
fp32 k2 = Diff1 * SavedVelo;
if (k1 < -0.5f || k2 < -0.2f || Diff1.Length() > 32.0f)
{
	DBG_OUT("wtf2\n");

	*((CVec3Dfp32*)&m_pObj->GetMoveVelocity()) = SavedVelo;
	*((CMat4Dfp32*)&m_pObj->GetLocalPositionMatrix()) = SavedPos;
	*((CMat4Dfp32*)&m_pObj->GetPositionMatrix()) = SavedPos;
	*((CMat4Dfp32*)&m_pObj->GetLastPositionMatrix()) = Saved_LastPos;

	m_Gravity = SavedGravity;
	m_Flags = SavedFlags;
	m_MatLook = Saved_MatLook;
	m_MatBody = Saved_MatBody;

	m_Phys_bInAir = Saved_Phys_bInAir;
	m_Phys_nInAirFrames = Saved_Phys_nInAirFrames;
	m_Control_Move = Saved_Control_Move;
	m_Control_Look = Saved_Control_Look;
}//*/

/*
	if (!(m_Flags & DARKLING_FLAGS_CAN_CLIMB))
	{
		// Paranoia - TODO: check if this is needed
		SetGravity(CVec3Dfp32(0,0,-1));
		m_Flags = m_Flags & ~DARKLING_FLAGS_IS_ROTATING;
		m_State = DARKLING_STATE_NORMAL;
		return;
	}
*/
	const int PhysType = CWObject_Character::Char_GetPhysType(m_pObj);
	bool bNoClip = (PhysType == PLAYER_PHYS_NOCLIP);
	if (bNoClip)
		return;


	CWAG2I_Context AGContext(m_pObj, m_pWPhysState, m_GameTime, m_pWPhysState->GetGameTickTime());
	uint32 AGStateFlags1 = m_AnimGraph2.GetStateFlagsLo();
	uint32 AGStateFlags2 = m_AnimGraph2.GetStateFlagsHi();
	bool bWallClimb = (m_Flags & DARKLING_FLAGS_CAN_CLIMB) && !(AGStateFlags2 & AG2_STATEFLAGHI_NOWALLCLIMB);

	const CMat4Dfp32& PosMat = m_pObj->GetPositionMatrix();
	const CVec3Dfp32& Up = PosMat.GetRow(2);
	const CVec3Dfp32& Gravity = m_Gravity;//m_Gravity_Estimated;

//	bool bJumpStart = (m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_JUMPSTART) != 0;
//	bool bJumpActive = (m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_JUMPACTIVE) != 0;
//	bool bControlModeAnim = (ControlMode == PLAYER_CONTROLMODE_ANIMATION);
//	bool bControlModeAnim = (m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_ANIMPHYSOVERRIDE) != 0;
	bool bOnGround = !m_Phys_bInAir;

	CMat4Dfp32 RotMat, NewMat;

	if (m_State == DARKLING_STATE_NORMAL)
	{
		if (bOnGround)
		{
			// Has jump button been pressed?
			if (((m_Control_Press & CONTROLBITS_JUMP) & m_Control_Released))
				PerformJump();
		}
		else if (bWallClimb && (m_Phys_nInAirFrames > 5) && IsValid(m_CornerRef.m_Value.k[0]))
		{
			// Make sure darkling stays on ground
			if ((m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD) && !(AGStateFlags2 & AG2_STATEFLAGHI_ANIMPHYSOVERRIDE))
				SnapToGround();

			// This is a hack to fix problems when a convex corner isn't properly detected
			CVec3Dfp32 MoveDir = m_pObj->GetMoveVelocity();
			fp32 l = MoveDir.Length();
			fp32 k = M_Fabs(Gravity * MoveDir); 
			if ((l > 0.1f) && (k < 0.5f*l)) // ignore small movements and movement in the gravity-axis
			{
				DBG_OUT("PANIC!! state normal, but in air for more than 5 ticks!\n");
				// Set steep-slope flag (this is ok, since we've kept track of last good position)
				m_Flags = m_Flags | DARKLING_FLAGS_STEEP_SLOPE_AHEAD;
				m_CornerRef.MakeDirty();
			}
		}
	}

	if (m_State == DARKLING_STATE_JUMP_INIT)
	{
		if (AGStateFlags2 & AG2_STATEFLAGHI_JUMPSTART)
		{
			m_State = DARKLING_STATE_JUMP_START;

			// Create a rotation from current pose to one that aligns with jump direction
			CMat4Dfp32 JumpMat;
			JumpMat.GetRow(0) = m_JumpDir;
			JumpMat.GetRow(2) = PosMat.GetRow(2);
			JumpMat.RecreateMatrix(0, 2);

			CQuatfp32 r0; 
			r0.Create(PosMat); 
			r0.Inverse();
			CQuatfp32 r1;
			r1.Create(JumpMat); 
			CQuatfp32 dRot = r0;
			dRot *= r1;
			CAxisRotfp32 Rot = dRot;

			CQuatfp32 a = r1;
			a *= dRot;

			//GetFreeRotation(MatBody.GetRow(0), m_JumpDir, MatBody.GetRow(2), Rot);

			{
				dRot.SetMatrix(RotMat);
				PosMat.Multiply(RotMat, NewMat);
				bool b = NewMat.AlmostEqual(JumpMat, 0.01f);
				DBG_OUT("b = %d\n", b);
			}

			fp32 Duration = m_AnimGraph2.GetAG2I()->GetCurrentLoopDuration(&AGContext);

			DBG_OUT("Jump start, duration = %.2f\n", Duration);
			DBG_OUT("rotation: %s %.3f\n", Rot.m_Axis.GetString().Str(), Rot.m_Angle);

			Rot.m_Angle *= (m_pWPhysState->GetGameTickTime() / Duration);
			m_Jump_dRot = Rot;//.Create(Rot.m_Axis, Rot.m_Angle);

			// Set destination...
		}
		else
		{
			// Paranoia check to see if we got stuck in init state
			fp32 Time = (m_GameTick - m_JumpTick) * m_pWPhysState->GetGameTickTime();
			if (Time > 1.0f)
			{
				DBG_OUT("Got stuck in jump-init state?! aborting...\n");
				m_State = DARKLING_STATE_NORMAL;
			}
		}
	}

	if (m_State == DARKLING_STATE_JUMP_START)
	{
		m_UpVector = PosMat.GetRow(2);
		/*const CAxisRotfp32& dRot = m_Jump_dRot;
		CQuatfp32(dRot.m_Axis, dRot.m_Angle * _dTime).SetMatrix(RotMat);
		PosMat.Multiply(RotMat, NewMat);

		m_UpVector = m_UpVector.m_Value * RotMat;

		m_pWPhysState->Object_SetRotation(m_pObj->m_iObject, NewMat);*/
		// Let ag handle rotation during jumpstart......
		//m_Flags = m_Flags | DARKLING_FLAGS_IS_ROTATING;

		#ifndef M_RTM
		{
//			m_pWPhysState->Debug_RenderVector(PosMat.GetRow(3), PosMat.GetRow(0) * 50.0f, 0xF0FF0000, 5.0f, true);
		//	CAxisRotfp32 Rot;
		//	GetFreeRotation(MatBody.GetRow(0), m_JumpDir, MatBody.GetRow(2), Rot);
		//	DBG_OUT("rot left: %.3f\n", Rot.m_Angle);
		}
		#endif

		// Time to start flying?
		if (AGStateFlags1 & AG2_STATEFLAG_JUMPACTIVE)
		{
			m_State = DARKLING_STATE_JUMP_FLY;
			m_JumpTick = m_GameTick;
			DBG_OUT("Jump fly\n");
		}
		else if (!(AGStateFlags2 & AG2_STATEFLAGHI_JUMPSTART))
		{	// AnimGraph is no longer in jump-mode. Reset state
			DBG_OUT("Jump end (in start-mode)\n");
			m_State = DARKLING_STATE_NORMAL;
			m_Flags = m_Flags & ~(DARKLING_FLAGS_JUMP_MATRIX | DARKLING_FLAGS_JUMP_DIRECTION);
		}
	}

	if (m_State == DARKLING_STATE_JUMP_FLY)
	{
		// Should we prepare for landing?
		//   TODO: Check if obstacle is ahead of us

		if (m_Flags & DARKLING_FLAGS_JUMP_MATRIX)
		{
			const CMat4Dfp32& JumpDest = m_JumpDestination;
			fp32 DistSqr = JumpDest.GetRow(3).DistanceSqr(PosMat.GetRow(3));
			if (DistSqr < Sqr(50)) //TWEAKME: number of units of movement in the landing animation
			{
				DBG_OUT("Land!\n");

				SetGravity(-JumpDest.GetRow(2));
				m_Flags = m_Flags | DARKLING_FLAGS_AUTOPULLUP;
				m_State = DARKLING_STATE_JUMP_LAND;
				// Let ai handle the landing
				CWObject_Message Msg(OBJMSG_CHAR_AI_DARKLINGLAND);
				Msg.m_pData = &m_JumpDestination;
				Msg.m_DataSize = sizeof(m_JumpDestination);
				if (!m_pWPhysState->Phys_Message_SendToObject(Msg,m_pObj->m_iObject))
				{
					// In case ai isn't handling it
					// Set destination speed as well (current speed)
					m_AnimGraph2.SetDestination(JumpDest,m_pObj->GetVelocity().m_Move.Length());
					m_AnimGraph2.SendJumpImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_JUMP, AG2_IMPULSEVALUE_JUMP_LANDDEST));
				}
			}
			else
			{
				m_AnimGraph2.SetDestination(m_JumpDestination);
			}
		}

		// Rotate upvector to align with (new) ground
		/*{
			// Create a rotation from current pose to one that aligns with jump direction
			CMat4Dfp32 DestMat;
			DestMat.GetRow(2) = -Gravity;
			DestMat.GetRow(0) = m_MatBody.GetRow(0);
			DestMat.RecreateMatrix(0, 2);

			CQuatfp32 r0; 
			r0.Create(m_MatBody); 
			r0.Inverse();
			CQuatfp32 QRot;
			QRot.Create(DestMat); 
			QRot *= r0;
			CAxisRotfp32 Rot = QRot;
			const fp32 Speed = 
			Rot.m_Angle = Sign(m_Angle) * Min(Speed, M_Fabs(m_Angle)) * _dTime;
			//GetFreeRotation(MatBody.GetRow(0), m_JumpDir, MatBody.GetRow(2), Rot);
		}*/
		// Rotate upvector to align with current z (disable during landing plz)

		if (!(AGStateFlags1 & AG2_STATEFLAG_JUMPACTIVE))
		{	// AnimGraph is no longer in jump-mode. Reset state
			DBG_OUT("Jump end (in fly-mode)\n");
			m_State = DARKLING_STATE_NORMAL;
			m_Flags = m_Flags & ~(DARKLING_FLAGS_JUMP_MATRIX | DARKLING_FLAGS_JUMP_DIRECTION);
		}
	}

	if (m_State == DARKLING_STATE_JUMP_LAND)
	{
		m_UpVector = PosMat.GetRow(2);
		if (!(AGStateFlags1 & AG2_STATEFLAG_JUMPACTIVE))
		{	// AnimGraph is no longer in jump-mode. Reset state
			DBG_OUT("Jump end (in land-mode)\n");
			m_State = DARKLING_STATE_NORMAL;
			m_Phys_bInAir = 0;	//We landed, not in air any longer;
			m_Flags = m_Flags & ~(DARKLING_FLAGS_JUMP_MATRIX | DARKLING_FLAGS_JUMP_DIRECTION);
		}
	}


	// Update turn correction
	if (AGStateFlags1 & AG2_STATEFLAG_APPLYTURNCORRECTION)
	{
		CQuatfp32 RotVelocity;
		if (m_AnimGraph2.GetAG2I()->GetAnimRotVelocity(&AGContext, RotVelocity,0))
		{
			CAxisRotfp32 RotVelocityAR(RotVelocity);
			m_AnimRotVel = RotVelocityAR.m_Angle * RotVelocityAR.m_Axis.k[2];
		}
	}

	// Reset jump button
	if (!(m_Control_Press & CONTROLBITS_JUMP))
		m_Control_Released |= CONTROLBITS_JUMP;


	// Have we been in air for too long?
	if ((m_Phys_nInAirFrames > m_GravityFreeMaxTicks*2) && !(AGStateFlags2 & AG2_STATEFLAGHI_ANIMPHYSOVERRIDE))
	{
		// Time's up - turn normal gravity back on!
		CVec3Dfp32 DefaultGrav(0,0,-1);
		if (!DefaultGrav.AlmostEqual(m_Gravity, 0.01f))
		{
			SetGravity(DefaultGrav);
			Invalidate(m_CornerRef.m_Value.k[0]);
			m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;
			DBG_OUT("Gravity reset! %s\n", ((CVec3Dfp32)m_Gravity).GetString().Str());
		}
	}

	if (m_State == DARKLING_STATE_NORMAL/* || m_State == DARKLING_STATE_JUMP_LAND*/)// || m_State == DARKLING_STATE_JUMP_FLY )
	{
		if (m_Flags & DARKLING_FLAGS_AUTOPULLUP)
		{	// Do both auto-roll and auto-yaw to match current gravity vector
			CVec3Dfp32 bisec = Up - Gravity;
			fp32 Len = bisec.Length();
			if (Len < 0.01f)
			{	
				bisec = PosMat.GetRow(1);
			}
			if (Len > 1.99f)
			{	// Up and gravity are in opposition; we're done rotating
				m_Flags = m_Flags & ~DARKLING_FLAGS_AUTOPULLUP;
				m_Flags = m_Flags & ~DARKLING_FLAGS_IS_ROTATING;
			}
			else 
			{
				bisec *= (1.0f / Len);
				fp32 CosHalfAngle = Up *(-Gravity);
				fp32 CosHalfAngle2 = Up * bisec;	//CosHalfAngle2 is for when we are running into a corner and looking down, this
				if (CosHalfAngle > 0.01f || CosHalfAngle2 > 0.01f)
				{
					CVec3Dfp32 Axis; Up.CrossProd(-Gravity, Axis);
					if (Axis.LengthSqr() < 0.01f)
					{
						bool wtf = true;
					}
					fp32 Angle = -M_ACos(CosHalfAngle) * 2.0f * (0.5f/_PI) * 0.1f;
					if (M_Fabs(Angle) > 0.01f)
					{
						//				DBG_OUT("Auto-pullup, %.2f\n", Angle);
						Angle = Sign(Angle) * Min(0.1f, M_Fabs(Angle)) * _dTime;
						CMat4Dfp32 RotMat, NewMat;
						CQuatfp32(Axis, Angle).SetMatrix(RotMat);
						PosMat.Multiply(RotMat, NewMat);
						m_pWPhysState->Object_SetRotation(m_pObj->m_iObject, NewMat);
						m_Flags = m_Flags | DARKLING_FLAGS_IS_ROTATING;
					}
					else
					{
						m_Flags = m_Flags & ~DARKLING_FLAGS_AUTOPULLUP;
						m_Flags = m_Flags & ~DARKLING_FLAGS_IS_ROTATING;
					}
				}
			}
		}
		else
		{
			// Auto-roll to match current gravity vector
			const CVec3Dfp32& Axis = PosMat.GetRow(0);
//			fp32 Angle = GetAngle(-Gravity, PosMat.GetRow(1), PosMat.GetRow(0));
			fp32 Angle = -GetPlanarRotation(PosMat.GetRow(2), -Gravity, Axis);
			if (M_Fabs(Angle) > 0.01f)
			{
	//			DBG_OUT("Auto-roll, %.2f\n", Angle);
				Angle = Sign(Angle) * Min(0.05f, M_Fabs(Angle)) * _dTime;
				CMat4Dfp32 RotMat, NewMat;
				CQuatfp32(Axis, Angle).SetMatrix(RotMat);
				PosMat.Multiply(RotMat, NewMat);
				m_pWPhysState->Object_SetRotation(m_pObj->m_iObject, NewMat);
				m_Flags = m_Flags | DARKLING_FLAGS_IS_ROTATING;
			}
			else
			{
				m_Flags = m_Flags & ~DARKLING_FLAGS_IS_ROTATING;
			}
		}

		{ // Pull up "up vector"
			//CVec3Dfp32 bisec = (Up + m_UpVector.m_Value).Normalize();
			//fp32 CosHalfAngle = Up * bisec;
			const CVec3Dfp32& UpVec = m_UpVector;
			CVec3Dfp32 bisec = UpVec - Gravity;
			fp32 Len = bisec.Length();
			if (Len < 0.01f)
				bisec = PosMat.GetRow(1);
			else
				bisec *= (1.0f / Len);
			fp32 CosHalfAngle = UpVec * bisec;
			//if (CosHalfAngle > 0.01f)
			{
				fp32 Speed = (CosHalfAngle < 0.4f) ? 0.1f : 0.05f;
				CVec3Dfp32 Axis; UpVec.CrossProd(bisec, Axis);
				fp32 Angle = -M_ACos(CosHalfAngle) * 2.0f * (0.5f/_PI);
				if (M_Fabs(Angle) > 0.01f)
				{
	//				DBG_OUT("Pull-up, %.2f\n", Angle);
					Angle = Sign(Angle) * Min(Speed, M_Fabs(Angle)) * _dTime;
					CMat4Dfp32 RotMat;
					CQuatfp32(Axis, Angle).SetMatrix(RotMat);
					m_UpVector = m_UpVector.m_Value * RotMat;
				}
			}
		}
	}

	UpdateMatrices();
}


void CWO_CharDarkling_ClientData::Phys_GetAcceleration(CMat4Dfp32* _pMat, fp32 _dTime)
{
	const CMat4Dfp32& PosMat = m_pObj->GetPositionMatrix();
	const CVec3Dfp32& MoveVelocity = m_pObj->GetMoveVelocity();
//	const CVec3Dfp32& Pos = PosMat.GetRow(3);
	const CVec3Dfp32& Gravity = m_Gravity;

//	const int32 ControlMode = CWObject_Character::Char_GetControlMode(m_pObj);
	const int PhysType = CWObject_Character::Char_GetPhysType(m_pObj);
	
	bool bNoClip = (PhysType == PLAYER_PHYS_NOCLIP);
//	bool bJumpStart = (m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_JUMPSTART) != 0;
//	bool bJumpActive = (m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_JUMPACTIVE) != 0;
//	bool bControlModeAnim = (ControlMode == PLAYER_CONTROLMODE_ANIMATION);
	bool bControlModeAnim = (m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_ANIMPHYSOVERRIDE) != 0;
	
	// If in perfect placement mode, move towards that position
 	if ((m_State == DARKLING_STATE_NORMAL) && m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_PERFECTPLACEMENT) && m_AnimGraph2.GetExactPositionState() == -1)
	{
		// Move towards destination
		const fp32 Factor = 0.3f;
		CMat4Dfp32 DestPos = m_AnimGraph2.GetDestination();
		CMat4Dfp32 CurrentPos = m_pObj->GetPositionMatrix();
		CQuatfp32 CurrentRot, TargetRot,DeltaRot;
		CurrentRot.Create(CurrentPos);
		TargetRot.Create(DestPos);

		CVec3Dfp32 DeltaPos = DestPos.GetRow(3) - CurrentPos.GetRow(3);
		m_pWPhysState->Object_SetVelocity(m_pObj->m_iObject, DeltaPos * Factor);
		CurrentRot.Lerp(TargetRot, Factor,DeltaRot);
		DeltaRot.CreateMatrix3x3(CurrentPos);
		m_pWPhysState->Object_SetRotation(m_pObj->m_iObject,CurrentPos);

		return;
	}

	// Get movement vector from player input
	const CVec3Dfp32& Move = m_Control_Move;
	CVec3Dfp32 Movement(0.0f);
	if (m_Flags & DARKLING_FLAGS_WORLDSPACE)
	{	// Input is given as world space direction
		Movement += m_MatLook.GetRow(0) * ((m_MatLook.GetRow(0) * Move) * (fp32)m_Speed_Forward);
		Movement += m_MatLook.GetRow(1) * ((m_MatLook.GetRow(1) * Move) * (fp32)m_Speed_SideStep);
	}
	else
	{	// Input is given as local space (joystick) direction 
		Movement += m_MatLook.GetRow(0) * (Move.k[0] * (fp32)m_Speed_Forward);
		Movement += m_MatLook.GetRow(1) * (Move.k[1] * (fp32)m_Speed_SideStep);
	}
	m_CurrMovement = Movement;

	CWAG2I_Context AG2Context(m_pObj, m_pWPhysState, m_GameTime);

	bool bOnGround = CheckOnGround();

	CVec3Dfp32 dV(0);
	if (bNoClip)
	{
		dV += Movement * 2.5f;
		if (m_Control_Press & CONTROLBITS_JUMP)
			dV += PosMat.GetRow(2) * 20.0f;

		dV.Lerp(MoveVelocity, 0.9f, dV);
		_pMat->GetRow(3) = dV - MoveVelocity;
	}
/*	else if (m_Flags & DARKLING_FLAGS_FULLSTOP)
	{
		m_Flags = m_Flags & ~DARKLING_FLAGS_FULLSTOP;
	}*/
	else if (m_State == DARKLING_STATE_JUMP_START)
	{
		// Darkling is about to jump. Let's use animation speed for movement
		// (rotation is blended towards desired jump direction in Phys_Move)
		CVec3Dfp32 MoveVelocityAG;
		CQuatfp32 RotVelocityAG;
		if (m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXACTDESTPOSITION)
		{
			m_AnimGraph2.GetAG2I()->GetAnimVelocityToDestination(&AG2Context,MoveVelocityAG,RotVelocityAG,m_AnimGraph2.GetExactPositionState());
			m_pWPhysState->Object_SetVelocity(m_pObj->m_iObject, MoveVelocityAG);
			m_pWPhysState->Object_SetRotVelocity(m_pObj->m_iObject, RotVelocityAG);
		}
		else
		{
			if (m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_JUMPACTIVE)
			{
				// let the darkling fly for a while
				dV = m_JumpDir * m_Speed_Jump;
				_pMat->GetRow(3) = dV - MoveVelocity;	
			}
			else if (m_AnimGraph2.GetAG2I()->GetAnimVelocity(&AG2Context, MoveVelocityAG, RotVelocityAG, 0))
			{
				m_pWPhysState->Object_SetVelocity(m_pObj->m_iObject, MoveVelocityAG);
				m_pWPhysState->Object_SetRotVelocity(m_pObj->m_iObject, RotVelocityAG);
			}
		}
	}
	else if (m_State == DARKLING_STATE_JUMP_FLY)
	{
		fp32 FlyTime = (m_GameTick - m_JumpTick) * m_pWPhysState->GetGameTickTime();

		// If doing a free jump, only allow 1 second of gravity-free flying
		if (!(m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY)&&!(m_Flags & DARKLING_FLAGS_JUMP_MATRIX)&&(FlyTime > 0.5f))
		{
			_pMat->GetRow(3) = Gravity;
			CVec3Dfp32 DefaultGrav(0,0,-1);
			if (!DefaultGrav.AlmostEqual(m_Gravity, 0.01f))
			{
				SetGravity(DefaultGrav);
				Invalidate(m_CornerRef.m_Value.k[0]);
				DBG_OUT("Gravity reset! %s\n", ((CVec3Dfp32)m_Gravity).GetString().Str());
			}
		}
		else
		{
			//m_pWPhysState->Debug_RenderMatrix(PosMat,15.0f,false);
			// let the darkling fly for a while
			dV = m_JumpDir * m_Speed_Jump;
			_pMat->GetRow(3) = dV - MoveVelocity;	
		}
	}
	else if (m_State == DARKLING_STATE_JUMP_LAND)
	{
		if (m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXACTDESTPOSITION)
		{
			CVec3Dfp32 MoveVelocity;
			CQuatfp32 RotVelocity;
			m_AnimGraph2.GetAG2I()->GetAnimVelocityToDestination(&AG2Context,MoveVelocity,RotVelocity,m_AnimGraph2.GetExactPositionState());
			m_pWPhysState->Object_SetVelocity(m_pObj->m_iObject, MoveVelocity);
			m_pWPhysState->Object_SetRotVelocity(m_pObj->m_iObject, RotVelocity);
		}
		else if (m_Flags & DARKLING_FLAGS_JUMPTOAIR)
		{
			m_pWPhysState->Object_SetVelocity(m_pObj->m_iObject, CVec3Dfp32(0.0f));
		}
		else if (!bOnGround)
		{
			//dV = m_JumpDir * m_Speed_Jump;
			//_pMat->GetRow(3) = dV - MoveVelocity;
			_pMat->GetRow(3) = m_Gravity.m_Value * m_Speed_Jump.m_Value;
		}
	}
	else if (bControlModeAnim)
	{
		// Let animation control movement completely
		CVec3Dfp32 MoveVelocity = 0.0f;
		CQuatfp32 RotVelocity;
		RotVelocity.Unit();
		if (m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXACTSTARTPOSITION)
		{
			m_AnimGraph2.GetAG2I()->GetAnimVelocityFromDestination(&AG2Context,MoveVelocity,RotVelocity,m_AnimGraph2.GetExactPositionState());
		}
		else if (m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXACTDESTPOSITION)
		{
			m_AnimGraph2.GetAG2I()->GetAnimVelocityToDestination(&AG2Context,MoveVelocity,RotVelocity,m_AnimGraph2.GetExactPositionState());
		}
		else
		{
			m_AnimGraph2.GetAG2I()->GetAnimVelocity(&AG2Context, MoveVelocity, RotVelocity, 0);
			// Add gravity if we're not on ground
			if (!bOnGround)
				_pMat->GetRow(3) = Gravity;
		}
		m_pWPhysState->Object_SetVelocity(m_pObj->m_iObject, MoveVelocity);
		m_pWPhysState->Object_SetRotVelocity(m_pObj->m_iObject, RotVelocity);
	}
	else if (m_State == DARKLING_STATE_NORMAL)
	{
		if (bOnGround)
		{
			{
				// Add regular movement
				dV += Movement;

				fp32 k = (MoveVelocity.LengthSqr() > Sqr(10.0f)) ? 0.01f : 0.7f;
				dV.Lerp(MoveVelocity, k, dV);

			//	if (m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD && dV.LengthSqr() > Sqr(2.0f))
			//		dV *= 0.7f; // Slow down!


				_pMat->GetRow(3) = dV - MoveVelocity;


#if DBG_RENDER
				const CVec3Dfp32& Pos = PosMat.GetRow(3);
				m_pWPhysState->Debug_RenderVector(Pos, MoveVelocity, DEBUG_COLOR_RED, 1.0f, false);
				m_pWPhysState->Debug_RenderVector(Pos, dV, DEBUG_COLOR_BLUE, 1.0f, false);
				m_pWPhysState->Debug_RenderSphere(m_MatLook, 16.0f, DEBUG_COLOR_RED, 0.0f, false);

				CMat4Dfp32 Tmp = m_MatLook;
				Tmp.GetRow(3) += dV;
				m_pWPhysState->Debug_RenderSphere(Tmp, 16.0f, DEBUG_COLOR_BLUE, 0.0f, false);
#endif
			}
		}
		else
		{	// In air

		/*	if (m_Phys_nInAirFrames >= 1 && (m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD))
			{
				_pMat->GetRow(3) = -MoveVelocity + Gravity;
				SetGravity(-(CVec3Dfp32)m_SteepSlopeNormal);
				m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;
				//M_TRACE("Changing grav due to steep slope, new grav = %s\n", ((CVec3Dfp32)m_Gravity).GetString().Str());
			}
			else*/

#if DBG_RENDER
		m_pWPhysState->Debug_RenderSphere(m_MatLook, 16.0f, DEBUG_COLOR_BLUE, 0.0f, false);
		m_pWPhysState->Debug_RenderVector(PosMat.GetRow(3), MoveVelocity, DEBUG_COLOR_RED, 3.0f, false);
#endif
			if (m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY)
			{
				_pMat->GetRow(3) = CVec3Dfp32(0.0f,0.0f,0.0f);
			}
			else
			{
				if (!(m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD))
				{
					_pMat->GetRow(3) = Gravity;
				}
				else
				{
					_pMat->GetRow(3) = m_Gravity_Estimated;
				}
			}
			
		}
	}
	/*else if (m_JumpTick && m_Phys_nInAirFrames <= 3)
	{
		// let the player fly for a while
		dV = m_JumpDir * m_Speed_Jump;
		fp32 k = (MoveVelocity.LengthSqr() > Sqr(10.0f)) ? 0.01f : 0.7f;
		dV.Lerp(MoveVelocity, k, dV);
		_pMat->GetRow(3) = dV - MoveVelocity;
	}*/

	//else if (bOnGround && !bJumpActive)// || m_Phys_nInAirFrames < 2)
	{ 
	//	if ((m_GameTick - m_JumpTick) > int(m_pWPhysState->GetGameTicksPerSecond() * 0.5f))
	//		m_JumpTick = 0; // reset jump if on ground -- paranoia!




	}
//	else
	{
		/*if (bJumpActive && !(m_Flags & DARKLING_FLAGS_JUMP_FLYING))
		{
			DBG_OUT("Jump fly!\n");
			m_Flags = m_Flags | DARKLING_FLAGS_JUMP_FLYING;
		}*/

	}

#if 0//ndef M_RTM
	m_pWPhysState->Debug_RenderVector(Pos, Movement * 10.0f, 0xFF00FF00, 1.0f, true);
	m_pWPhysState->Debug_RenderVector(Pos, PosMat.GetRow(0) * 50.0f, 0xFFFF0000, 0.0f, false);
	m_pWPhysState->Debug_RenderVector(Pos, PosMat.GetRow(1) * 50.0f, 0xFFFFFF00, 0.0f, false);
	m_pWPhysState->Debug_RenderVector(Pos, PosMat.GetRow(2) * 50.0f, 0xFFFF00FF, 0.0f, false);
#endif

}


void CWO_CharDarkling_ClientData::Phys_OnImpact(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, const CCollisionInfo& _CInfo)
{
	//	M_TRACE("[CWObject_CharDarkling, %s] CWO_PHYSEVENT_IMPACT, new gravity=[%.1f, %.1f, %.1f]\n", 
	//		_pPhysState->IsServer() ? "server" : "client", CD.m_Gravity.m_Value.k[0], CD.m_Gravity.m_Value.k[1], CD.m_Gravity.m_Value.k[2]);
///	DBG_OUT("Impact (N: %s, V: %s)!\n", _CInfo.m_Plane.n.GetString().Str(), m_pObj->GetMoveVelocity().GetString().Str());

#if DBG_RENDER
	m_pWPhysState->Debug_RenderVector(_CInfo.m_Pos, _CInfo.m_Plane.n * 8.0f, DEBUG_COLOR_RED, 1.0f, false);
#endif

	// Check if we hit character
	bool bCharacter = false;
	bool bObject = false;
	bool bNoClimb = false;
	int iObj = _CInfo.m_iObject;
	CWObject_CoreData* pObj = m_pWPhysState->Object_GetCD(iObj);
	if (pObj)
	{
		CWO_PhysicsState PhysState = pObj->GetPhysState();
		uint16 ObjFlags = PhysState.m_ObjectFlags;
		uint16 MediumFlags = PhysState.m_MediumFlags;
		const CXW_SurfaceKeyFrame *pBaseFrame =  _CInfo.m_pSurface ? _CInfo.m_pSurface->GetBaseFrame() : NULL;
		if(pBaseFrame && pBaseFrame->m_Medium.m_MediumFlags & XW_MEDIUM_SOLID && pBaseFrame->m_Friction == 0.0f) //Ugly hack, friction 0 is used to tell normal dynamicsolids and noclimb dynamic solids apart
			bNoClimb = true;
		if (ObjFlags & OBJECT_FLAGS_CHARACTER)
			bCharacter = true;
		if (ObjFlags & OBJECT_FLAGS_OBJECT)
			bObject = true;
		if ((m_pWPhysState->IsServer())&&(_pObj)&&(!bCharacter)&&(bObject)&&(m_iPlayer == -1))
		{	// *** Maybe we should do this in multiplayer too?
			CWorld_Server* pWServer = (CWorld_Server*)m_pWPhysState;
			CWObject* pOurObj = pWServer->Object_Get(_pObj->m_iObject);
			CWObject_Character* pChar = CWObject_Character::IsCharacter(pOurObj);
			if ((pChar)&&(pChar->m_spAI))
			{
				CVec3Dfp32 Pos = _CInfo.m_Pos;
				pChar->m_spAI->OnBumped(true,pObj->m_iObject,Pos);
			}
		}
	}

	// We hit something - use new gravity vector
	if (!bCharacter && !bNoClimb)
	{
		CVec3Dfp32 NewGravity;
		if ((bObject)&&(m_iPlayer == -1))
		{	// *** Maybe we should do this in multiplayer too?
			NewGravity = m_Gravity;
		}
		else
		{
			if((m_iPlayer != -1) && !(m_Flags & DARKLING_FLAGS_CAN_CLIMB))
			{
				NewGravity.k[0] = 0.0f;
				NewGravity.k[1] = 0.0f;
				NewGravity.k[2] = -1.0f;
			}
			else
			{
				NewGravity = -_CInfo.m_Plane.n;
				if (!NewGravity.AlmostEqual(m_Gravity, 0.001f))
					m_Flags = m_Flags | DARKLING_FLAGS_AUTOPULLUP;
			}
		}
		

/*		if (m_State == DARKLING_STATE_NORMAL)
		{
			// Blend towards new gravity
			const CMat4Dfp32 PosMat = m_pObj->GetPositionMatrix();

			CAxisRotfp32 rot;
			GetFreeRotation(m_Gravity, NewGravity, PosMat.GetRow(1), rot);
			rot.m_Angle = Sign(rot.m_Angle) * Min(0.1f, M_Fabs(rot.m_Angle));
			CMat4Dfp32 RotMat;
			CQuatfp32(rot.m_Axis, rot.m_Angle).SetMatrix(RotMat);

			NewGravity = (CVec3Dfp32)m_Gravity * RotMat;
		}*/

		bool bJumping = (m_State == DARKLING_STATE_JUMP_FLY) || (m_State == DARKLING_STATE_JUMP_LAND);

		CMat4Dfp32 MatMove = m_MatLook;
		MatMove.GetRow(0) = m_CurrMovement;//m_pObj->GetMoveVelocity();
		MatMove.RecreateMatrix(2, 0);

		fp32 k = MatMove.GetRow(1) * _CInfo.m_Plane.n;
		if (bJumping || M_Fabs(k) < 0.71f || (m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD))
		{
			// Set new gravity
			SetGravity(NewGravity);

			m_Phys_nInAirFrames = 0;
			m_Phys_bInAir = 0;
		}
	}
/*	else
	{
		// we hit a character
		if (m_State >= DARKLING_STATE_JUMP_INIT && m_State <= DARKLING_STATE_JUMP_LAND)
		{
			Invalidate(m_CornerRef.m_Value.k[0]);
			SetGravity(CVec3Dfp32(0,0,-1));

			if(m_pWPhysState->IsServer())
			{
				if(m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWPhysState->Game_GetObjectIndex()))
				{
					CWObject_Message Msg;
					Msg.m_Msg = OBJMSG_GAME_GETPLAYER;
					Msg.m_Param0 = pObj->m_iObject;
					CWObject_GameDM::CWO_PlayerDM* pPlayer = (CWObject_GameDM::CWO_PlayerDM *)m_pWPhysState->Phys_Message_SendToObject(Msg, m_pWPhysState->Game_GetObjectIndex());
					if(pPlayer)// && pPlayer->m_bIsHuman)
					{
						CWO_DamageMsg Dmg;
						Dmg.m_Damage = 500;
						Dmg.m_DamageType = 0;
						Dmg.m_SurfaceType = 0;
						Dmg.m_StunTicks = 0;
						Dmg.m_bPositionValid = 0;
						Dmg.m_bForceValid = 0;
						Dmg.m_bSplattDirValid = 0;
						Dmg.m_Position = 0;
						Dmg.m_Force = 0;
						Dmg.m_SplattDir = 0;
						Dmg.m_pDamageEffect = 0;
						Dmg.m_pCInfo = 0;

						Msg.m_Msg = OBJMSG_DAMAGE;
						Msg.m_pData = &Dmg;
						Msg.m_DataSize = sizeof(CWO_DamageMsg);
						Msg.m_iSender = m_pObj->m_iObject;

						m_pWPhysState->Phys_Message_SendToObject(Msg, pPlayer->m_iObject);
					}
				}
			}
		}
	}*/

	if (m_State == DARKLING_STATE_JUMP_FLY)
	{
		// Uncontrolled impact while flying
		DBG_OUT("Jump end (impact)\n");

		CWAG2I_Context AG2Context(m_pObj, m_pWPhysState, m_GameTime);
		m_AnimGraph2.SendJumpImpulse(&AG2Context, CXRAG2_Impulse(AG2_IMPULSETYPE_JUMP, AG2_IMPULSEVALUE_JUMP_LAND));
		m_State = DARKLING_STATE_JUMP_LAND;
	}

	// Stop movement if flying
	if (m_State == DARKLING_STATE_JUMP_LAND && !(m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXACTDESTPOSITION))
	{
		m_pWPhysState->Object_SetVelocity(m_pObj->m_iObject, 0.0f);
		m_JumpDir = 0;
	}

/*
	if (m_JumpTick)
	{
		m_JumpTick = 0;
		m_JumpDir = 0.0f;

		m_Flags = m_Flags & ~(DARKLING_FLAGS_JUMP_MATRIX | DARKLING_FLAGS_JUMP_DIRECTION | DARKLING_FLAGS_JUMP_FLYING | DARKLING_FLAGS_JUMP_LAND);
		m_Flags = m_Flags | DARKLING_FLAGS_FULLSTOP;
	}
*/
}


bool CWO_CharDarkling_ClientData::PerformJump()
{
	CWObject_CharShapeshifter *pDarkling = TDynamicCast<CWObject_CharShapeshifter>(m_pObj);
	if(pDarkling->Char_GetPhysType(m_pObj) == PLAYER_PHYS_DEAD)
		return false;

	if(m_JumpTick + 30 > m_pWPhysState->GetGameTick())
		return false;

	const CMat4Dfp32& PosMat = m_pObj->GetPositionMatrix();
	const CVec3Dfp32& Gravity = m_Gravity;
	const CVec3Dfp32& Pos = PosMat.GetRow(3);

	if(m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWPhysState->Game_GetObjectIndex()))	
		if(m_pWPhysState->Phys_IntersectLine(Pos, Pos + PosMat.GetRow(0) * 64.0f, 0, OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID, NULL, m_pObj->m_iObject))
			return false;

	CMat4Dfp32 MatBody = PosMat;
	MatBody.GetRow(2) = m_UpVector;
	MatBody.RecreateMatrix(2, 0);

	CVec3Dfp32 JumpDir;
	bool bHit = false;
	if (m_Flags & DARKLING_FLAGS_JUMP_MATRIX)
	{
		// Use Destination pos to get Jump direction
		const CMat4Dfp32& JumpDest = m_JumpDestination;
		JumpDir = (JumpDest.GetRow(3) - PosMat.GetRow(3)).Normalize();

		// Use Destination matrix to get new gravity vector
		m_Gravity_Estimated = -JumpDest.GetRow(2);
	}
	else
	{
		if (m_Flags & DARKLING_FLAGS_JUMP_DIRECTION)
		{
			JumpDir = m_JumpDestination.m_Value.GetRow(0);
		}
		else
		{	// Use Look direction as Jump direction
			JumpDir = PosMat.GetRow(0);
		}
/*		fp32 k = -(JumpDir * Gravity);
		if (k < -0.3f)
			JumpDir = -JumpDir;
		else if (k < 0.1f)
			JumpDir = (JumpDir + Gravity * 0.1f).Normalize();*/

		// Estimate new gravity vector at jump target location
		int objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
		int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_CAMERASOLID | XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_DYNAMICSSOLID;
		CCollisionInfo CInfo;
//		fp32 Distance = 300.0f;
		bHit = m_pWPhysState->Phys_IntersectLine(Pos, Pos + JumpDir * 300.0f, 0, objects, mediums, &CInfo, m_pObj->m_iObject);
		if (bHit && CInfo.m_bIsValid)
		{
			m_Gravity_Estimated = -CInfo.m_Plane.n;
			m_JumpDestination.m_Value.GetRow(3) = CInfo.m_Pos;
		}
	}

	if(m_pWPhysState->IsServer())
	{
		CWorld_Server* pWServer = (CWorld_Server*)m_pWPhysState;
		if(pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), pWServer->Game_GetObjectIndex()) && (m_SoundStartTick + TruncToInt(2.0f * pWServer->GetGameTicksPerSecond())) < pWServer->GetGameTick())
		{
			m_SoundStartTick = pWServer->GetGameTick();
			pWServer->Sound_On(pDarkling->m_iObject, pWServer->GetMapData()->GetResourceIndex_Sound("D_Darkling_Hrt_Charge_01"), WCLIENT_ATTENUATION_3D);
		}

/*		int objects = OBJECT_FLAGS_PLAYER;
		int mediums = 0;
		CCollisionInfo CInfo;
		if(m_pWPhysState->Phys_IntersectLine(Pos, Pos + JumpDir * 300.0f, 0, objects, mediums, &CInfo, m_pObj->m_iObject))
		{
			CWObject_Message Msg;
			Msg.m_Msg = OBJMSG_GAME_GETPLAYER;
			Msg.m_Param0 = CInfo.m_iObject;
			CWObject_GameDM::CWO_PlayerDM* pPlayer = (CWObject_GameDM::CWO_PlayerDM *)m_pWPhysState->Phys_Message_SendToObject(Msg, m_pWPhysState->Game_GetObjectIndex());
			if(pPlayer && pPlayer->m_bIsHuman)
			{
				m_iBerzerkTarget = CInfo.m_iObject;
			}
		}*/

		CWObject_ScenePointManager* pSPM = (CWObject_ScenePointManager*)pWServer->Message_SendToObject(
			CWObject_Message(OBJMSG_GAME_GETSCENEPOINTMANAGER), pWServer->Game_GetObjectIndex());

		if (pSPM && bHit)
		{
			CVec3Dfp32 DestPos = m_JumpDestination.m_Value.GetRow(3);
			pSPM->Selection_Clear();
			pSPM->Selection_AddRange(DestPos, 64.0f);
			fp32 RangeSqr = 4096.0f;
			TArray<CWO_ScenePoint*> lpScenepoints = pSPM->Selection_Get();

			int16 iBestPoint = -1;
			for (int i = 0; i < lpScenepoints.Len(); i++)
			{
				CWO_ScenePoint* pCur = lpScenepoints[i];
				
				if(pCur->GetType() & CWO_ScenePoint::DARKLING)
				{
					fp32 CurDist = (DestPos - pCur->GetPosition()).LengthSqr();
					if(CurDist <= RangeSqr)
					{
						RangeSqr = CurDist;
						iBestPoint = i;
					}
				}
			}

			if(iBestPoint != -1)
			{
				JumpDir = lpScenepoints[iBestPoint]->GetPosition() - PosMat.GetRow(3);
				JumpDir.Normalize();
				m_JumpDestination.m_Value = lpScenepoints[iBestPoint]->GetPositionMatrix();
				m_JumpDestination.MakeDirty();

				m_iScenePointTarget = pSPM->GetScenePointIndex(lpScenepoints[iBestPoint]);
			}
		}
	}

	// Turn off old pull up
	m_Flags = m_Flags & ~DARKLING_FLAGS_AUTOPULLUP;
	// Turn off steep slope detection flag
	m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;
	// Remove jumping flags
//	m_Flags = m_Flags & ~(DARKLING_FLAGS_JUMP_START | DARKLING_FLAGS_JUMP_LAND);
	Invalidate(m_CornerRef.m_Value.k[0]);

	m_JumpTick = m_GameTick;
	m_JumpDir = JumpDir;

	m_Phys_IdleTicks = 0;

	m_State = DARKLING_STATE_JUMP_INIT;

	DBG_OUT("Jump!\n");
//	bDidJump = true;

	{
		CMat4Dfp32 InvMatBody;
		MatBody.Transpose3x3(InvMatBody);
		CVec3Dfp32 LocalJumpDir = JumpDir;
		LocalJumpDir.MultiplyMatrix3x3(InvMatBody);
		DBG_OUT("LocalJumpDir = %s\n", LocalJumpDir.GetString().Str());

		CWAG2I_Context AG2Context(m_pObj, m_pWPhysState, m_GameTime);
		m_AnimGraph2.GetJumpDirection() = LocalJumpDir;
		// Set jump destination
		CMat4Dfp32 AGJumpDest = MatBody;
		// Check which order to rotate
		int32 i00,i10;
		if (JumpDir * MatBody.GetRow(0) > -0.1f)
		{
			i00 = 1;
			i10 = 2;
		}
		else
		{
			i00 = 2;
			i10 = 1;
		}
		fp32 Angle = -GetPlanarRotation(MatBody.GetRow(0), JumpDir, MatBody.GetRow(i00));
		if (M_Fabs(Angle) > 0.0001f)
		{
			CMat4Dfp32 RotMat;
			CQuatfp32(MatBody.GetRow(i00), Angle).SetMatrix(RotMat);
			MatBody.Multiply(RotMat, AGJumpDest);
		}

		Angle = -GetPlanarRotation(AGJumpDest.GetRow(0), JumpDir, AGJumpDest.GetRow(i10));
		if (M_Fabs(Angle) > 0.0001f)
		{
			CMat4Dfp32 RotMat,Temp;
			Temp = AGJumpDest;
			CQuatfp32(AGJumpDest.GetRow(i10), Angle).SetMatrix(RotMat);
			Temp.Multiply(RotMat, AGJumpDest);
		}

		//CMat4Dfp32 AGJumpDest;
		//AGJumpDest.GetRow(0) = JumpDir;
//		AGJumpDest.GetRow(2) = MatBody.GetRow(2);
		//AGJumpDest.GetRow(1) = MatBody.GetRow(1);
		AGJumpDest.GetRow(3) = MatBody.GetRow(3) + JumpDir * 50.0f;
		//m_pWPhysState->Debug_RenderMatrix(m_JumpDestination,15.0f);
//		AGJumpDest.RecreateMatrix(0,2);
		//AGJumpDest.RecreateMatrix(0,1);
		m_AnimGraph2.SetDestination(AGJumpDest);
		m_pWPhysState->Debug_RenderMatrix(AGJumpDest,15.0f,false,0xffffffff,0xff007f00,0xff00007f);
		bool bOk = m_AnimGraph2.SendJumpImpulse(&AG2Context, CXRAG2_Impulse(AG2_IMPULSETYPE_JUMP, AG2_IMPULSEVALUE_JUMP_UP));
		if (!bOk)
			return false;
	}

#ifndef M_RTM
	m_pWPhysState->Debug_RenderVector(Pos, PosMat.GetRow(0) * 50.0f, 0xF0FF0000, 5.0f, true);
	m_pWPhysState->Debug_RenderVector(Pos, JumpDir * 50.0f, 0xF0FF0000, 5.0f, true);
#endif

	return true;
}


void CWO_CharDarkling_ClientData::SnapToGround()
{
	const CMat4Dfp32& PosMat = m_pObj->GetLocalPositionMatrix();
	const CVec3Dfp32& MoveVelocity = m_pObj->GetMoveVelocity();
//	const CVec3Dfp32& Pos = PosMat.GetRow(3);
	const CVec3Dfp32& Gravity = m_Gravity;

	CVec3Dfp32 ToGround = m_CornerRef;
	ToGround -= PosMat.GetRow(3);
#if DBG_RENDER
	m_pWPhysState->Debug_RenderVector(PosMat.GetRow(3), ToGround, DEBUG_COLOR_GREEN, 3.0f, false);
#endif

	// Find ground.
	fp32 DistToGround = ToGround.Length();
	CWO_PhysicsState PhysState(m_pObj->GetPhysState());
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PLAYERPHYSMODEL;
	PhysState.m_ObjectFlags = 0;
	CMat4Dfp32 p0 = PosMat;
	CMat4Dfp32 p1 = PosMat;
	p1.GetRow(3) += ToGround * (DistToGround - 14.0f)/DistToGround;
	CCollisionInfo CollInfo;
	CVec3Dfp32 Down(0.0f, 0.0f, -1.0f);
	bool bRestoreSphere = false;
	fp32 org_size = 0.0f;
	if(m_Flags & DARKLING_FLAGS_CAN_CLIMB && !(m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_NOWALLCLIMB) && 
		Down.AlmostEqual(m_Gravity, 0.01f) && PhysState.m_nPrim && PhysState.m_Prim[0].m_PrimType == OBJECT_PRIMTYPE_SPHERE)
	{
		org_size = PhysState.m_Prim[0].GetRadius();
		CVec3Dfp32 new_size;
		new_size = org_size + 8.0f;
		PhysState.m_Prim[0].SetDim(new_size);
	}

	if (m_pWPhysState->Phys_IntersectWorld(m_PhysSelection, PhysState, p0, p1, m_pObj->m_iObject, &CollInfo) && CollInfo.m_bIsValid)
	{
		
		m_CornerRef = CollInfo.m_Pos;

		ToGround = m_CornerRef;
		ToGround -= PosMat.GetRow(3);
		ToGround.Normalize();

		CVec3Dfp32 NewPos = (CVec3Dfp32)m_CornerRef + (ToGround * -16.5f);
		m_pWPhysState->Object_SetPosition(m_pObj->m_iObject, NewPos);

#if DBG_RENDER
		CMat4Dfp32 Tmp = PosMat; Tmp.GetRow(3) = NewPos;
		m_pWPhysState->Debug_RenderSphere(Tmp, 16.0f, 0x7f007f00, 3.0f, true);
		m_pWPhysState->Debug_RenderSphere(m_pObj->GetPositionMatrix(), 16.0f, 0x7f00007f, 3.0f, true);
		m_pWPhysState->Debug_RenderVector(PosMat.GetRow(3), MoveVelocity, DEBUG_COLOR_GREEN, 3.0f, false);
#endif

		CMat4Dfp32 RotMat;
		CVec3Dfp32 Axis, Dir = MoveVelocity;
		Gravity.CrossProd(Dir, Axis);
		if (Axis.LengthSqr() > 0.01f)
		{
			Axis.Normalize();
			fp32 Angle = (-1.1f) * GetPlanarRotation(m_Gravity, ToGround, Axis);
			if (Angle < 0.0f)
				Angle += 1.0f;
			CQuatfp32(Axis, Angle).SetMatrix(RotMat);
			CVec3Dfp32 NewVel = MoveVelocity;
			NewVel.MultiplyMatrix3x3(RotMat);
			m_pWPhysState->Object_SetVelocity(m_pObj->m_iObject, NewVel);

#if DBG_RENDER
		m_pWPhysState->Debug_RenderVector(PosMat.GetRow(3), NewVel, DEBUG_COLOR_BLUE, 3.0f, false);
#endif
		}

		const CXW_SurfaceKeyFrame *pBaseFrame = CollInfo.m_pSurface->GetBaseFrame();
		if(pBaseFrame && pBaseFrame->m_Medium.m_MediumFlags & XW_MEDIUM_SOLID && pBaseFrame->m_Friction == 0.0f)
			ToGround = CVec3Dfp32(0.0f, 0.0f, -1.0f);

		m_Flags = m_Flags | DARKLING_FLAGS_AUTOPULLUP;
		SetGravity(ToGround);
		//We are close enough to hit ground, but since we enlarge the sphere in multiplayer, we could miss the ground in other places in the code
		//this can cause us to fall down anyway, so we reset this and everything is fine
		m_Phys_nInAirFrames = 0;
		m_Phys_bInAir = 0;
		UpdateMatrices();
	}

	if (bRestoreSphere)
	{
		org_size = PhysState.m_Prim[0].GetRadius();
		CVec3Dfp32 new_size;
		new_size = org_size;
		PhysState.m_Prim[0].SetDim(new_size);
	}
}

