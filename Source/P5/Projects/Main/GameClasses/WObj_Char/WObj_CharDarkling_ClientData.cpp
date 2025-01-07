/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client data for CWObject_CharDarkling.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharDarkling_ClientData implementation

	Comments:

	History:		
		050311:		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharDarkling_ClientData.h"
#include "../../GameWorld/WClientMod_Defines.h"
#include "../WObj_Game/WObj_GameMod.h"

#define DBG_OUT /**/DO_NOT/**/ M_TRACE

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_CharDarkling_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT(CWO_CharDarkling_ClientData, CWO_CharDarkling_ClientData::parent);

CWO_CharDarkling_ClientData::CWO_CharDarkling_ClientData()
{
	for (uint i = 0; i < DARKLING_NUM_EXTRAINSTANCE; i++)
		m_spExtraModelInstance[i] = NULL;
}


void CWO_CharDarkling_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	parent::Clear(_pObj, _pWPhysState);

	CVec3Dfp32 Up(0,0,1);
	m_Gravity = -Up;
	m_Gravity_Estimated = -Up;
	m_UpVector = Up;
	m_JumpTick = 0;
	m_Flags = DARKLING_FLAGS_CAN_CLIMB;
	m_GravityFreeMaxTicks = 20;

	m_MatLook.Unit();
	m_MatBody.Unit();
	m_CurrMovement = 0.0f;
	m_SoundStartTick = 0;

	for (uint i = 0; i < DARKLING_NUM_EXTRAINSTANCE; i++)
		m_spExtraModelInstance[i] = NULL;

	m_CameraStatus = DARKLING_CAMERA_LEFT;

	m_TotalLeftTurn = 0.0f;
	m_TotalRightTurn = 0.0f;
	m_CameraStatus = 0;
	m_CameraStartTick = 0;

	m_JumpDir = 0.0f;

	m_iScenePointTarget = -1;
}


void CWO_CharDarkling_ClientData::Copy(const CWO_Player_ClientData& _CD)
{
	const CWO_CharDarkling_ClientData& CD = *safe_cast<const CWO_CharDarkling_ClientData>(&_CD);

	m_TotalLeftTurn = CD.m_TotalLeftTurn;
	m_TotalRightTurn = CD.m_TotalRightTurn;
	m_CameraStatus = CD.m_CameraStatus;
	m_CameraStartTick = CD.m_CameraStartTick;
	m_JumpDir = CD.m_JumpDir;
	m_SoundStartTick = CD.m_SoundStartTick;

	parent::Copy(_CD);
}


void CWO_CharDarkling_ClientData::Char_UpdateLook(fp32 _FrameFrac)
{
//	m_Control_Look = m_Control_Look_Wanted;
}


bool CWO_CharDarkling_ClientData::Char_IsSpawning()
{
	return (m_Spawning) ? true : false;
}


void CWO_CharDarkling_ClientData::Char_ProcessControl_Look(const CVec3Dfp32& _dLook)
{
	MSCOPE(CWO_CharDarkling_ClientData::Char_ProcessControl_Look, CHAR_DARKLING);

	const CMat4Dfp32& PosMat = m_pObj->GetPositionMatrix();

	CMat4Dfp32 BaseMat = PosMat;
	BaseMat.GetRow(2) = -m_Gravity.m_Value;
	BaseMat.RecreateMatrix(2, 0);

	CMat4Dfp32 dRot;
	CMat4Dfp32 Temp, NewPosMat;

	// Yaw
	fp32 CosAngle = PosMat.GetRow(0) * BaseMat.GetRow(0);
	CosAngle = Sign(CosAngle) * Clamp01(CosAngle); // Safety precaution
	fp32 Angle = M_ACos(CosAngle) * (1.0f / _PI2) * Sign(PosMat.GetRow(0) * BaseMat.GetRow(2));
	fp32 NewAngle = Min(PLAYER_CAMERAMAXTILT, Max(-PLAYER_CAMERAMAXTILT, Angle + _dLook.k[1]));
	fp32 dAngle = NewAngle - Angle;
	dRot.Unit();
	dRot.M_x_RotY(dAngle);
	dRot.Multiply(PosMat, Temp);		// Rotate around local Y-axis

	// Tilt
	CQuatfp32(m_Gravity, -_dLook.k[2]).SetMatrix(dRot);
	Temp.Multiply(dRot, NewPosMat);			// Rotate around gravity Z-axis
	NewAngle = m_TurnCorrectionTargetAngle - _dLook[2];
	//m_TurnCorrectionTargetAngle = m_TurnCorrectionTargetAngle - _dLook.k[2];
	fp32 MaxOffset = ((fp32)(m_AnimGraph2.GetMaxBodyOffset())) / 360.0f;
//	fp32 Diff = m_TurnCorrectionTargetAngle;
	if (NewAngle < -MaxOffset)
		m_TurnCorrectionTargetAngle = -MaxOffset;
	else if (NewAngle > MaxOffset)
		m_TurnCorrectionTargetAngle = MaxOffset;
	else
		m_TurnCorrectionTargetAngle = NewAngle;

	//ConOutL(CStrF("dLook: %f TargetAngle: %f",_dLook[2],m_TurnCorrectionTargetAngle.m_Value));

	#define IsValidFloat(v) (v > -_FP32_MAX && v < _FP32_MAX)
	#define IsValidVec(v) (IsValidFloat(v.k[0]) && IsValidFloat(v.k[1]) && IsValidFloat(v.k[2]))
	#define IsValidMat(m) (IsValidVec(m.GetRow(0)) && IsValidVec(m.GetRow(1)) && IsValidVec(m.GetRow(2)))
	M_ASSERT(IsValidMat(NewPosMat), "!");

	m_pWPhysState->Object_SetRotation(m_pObj->m_iObject, NewPosMat);

	const CMat4Dfp32& LastCamera = m_pObj->GetLastPositionMatrix();
	CVec3Dfp32 LookDir, LastLookDir;
	CVec3Dfp32 LastLookRight;
	LookDir = PosMat.GetRow(0);
	LastLookDir = LastCamera.GetRow(0);
	LastLookRight = LastCamera.GetRow(1);

	Angle = LookDir * LastLookRight;
	if(Angle < -0.001f)	//höger
	{
		m_TotalLeftTurn = 0.0f;

		m_TotalRightTurn = (fp32)m_TotalRightTurn - Angle;
	}
	else if(Angle > 0.001f)				//vänster
	{
		m_TotalRightTurn = 0.0f;

		m_TotalLeftTurn = (fp32)m_TotalLeftTurn + Angle;
	}

	if (m_TotalRightTurn > 1.25f) 
	{
		//We are turning to the right
		//Make sure the camera is to the right of the darkling
		fp32 p = 0.0f;
		switch(m_CameraStatus) 
		{
		case DARKLING_CAMERA_LEFT:	//Wrong side, start moving it
			{
				m_CameraStatus = DARKLING_CAMERA_LEFT_TO_RIGHT;
				m_CameraStartTick = m_pWPhysState->GetGameTick();
				break;	
			}
		case DARKLING_CAMERA_LEFT_TO_RIGHT:	//We are moving to the correct side
			{
				p = (m_pWPhysState->GetGameTick() - m_CameraStartTick) / (DARKLING_CAMERA_SWITCH_TIME * m_pWPhysState->GetGameTicksPerSecond());
				if(p > 1.0f) 
				{
					p = 1.0f;
					m_CameraStatus = DARKLING_CAMERA_RIGHT;
				}
				break;
			}
		case DARKLING_CAMERA_RIGHT_TO_LEFT:
			{	
				//Force it to finish for now, this will snap
				m_CameraStatus = DARKLING_CAMERA_LEFT;

				break;
			}
		}
	}

	if(m_TotalLeftTurn > 1.25f)
	{	//And to the left
		fp32 p = 0.0f;
		switch(m_CameraStatus) 
		{
		case DARKLING_CAMERA_RIGHT:	//Wrong side, start moving it
			{
				m_CameraStatus = DARKLING_CAMERA_RIGHT_TO_LEFT;
				m_CameraStartTick = m_pWPhysState->GetGameTick();
				break;
			}
		case DARKLING_CAMERA_RIGHT_TO_LEFT:	//We are moving to the correct side
			{
				p = (m_pWPhysState->GetGameTick() - m_CameraStartTick) / (DARKLING_CAMERA_SWITCH_TIME * m_pWPhysState->GetGameTicksPerSecond());
				if(p > 1.0f) 
				{
					p = 1.0f;
					m_CameraStatus = DARKLING_CAMERA_LEFT;
				}
				break;
			}
		case DARKLING_CAMERA_LEFT_TO_RIGHT:	
			{
				//Force it to finish for now, this will snap
				m_CameraStatus = DARKLING_CAMERA_RIGHT;

				break;
			}
		}
	}
}


void CWO_CharDarkling_ClientData::Char_ProcessControl_Move(const CVec3Dfp32& _Move)
{
	MSCOPE(CWO_CharDarkling_ClientData::Char_ProcessControl_Move, CHAR_DARKLING);

	fp32 k = m_Control_Move * _Move;
	if (k < 0.0f)
		DBG_OUT("Changing Control_Move to %s\n", _Move.GetString().Str());

	m_Control_Move = _Move;
}


int CWO_CharDarkling_ClientData::OnClientPredictControlFrame(CWorld_Client* _pWClient, CWObject_Client* pPredict, CControlFrame& _Ctrl, bool _bFullFrame, bool _bLog)
{
	int Res = parent::OnClientPredictControlFrame(_pWClient,  pPredict, _Ctrl, _bFullFrame, _bLog);
	int bOld = m_bIsPredicting;
	m_bIsPredicting = true;
	OnRefresh();
	m_bIsPredicting = bOld;
	return Res;
}


void CWO_CharDarkling_ClientData::OnRefresh()
{
	if (m_iPlayer != -1)
	{
		if (m_State >= DARKLING_STATE_JUMP_INIT && m_State <= DARKLING_STATE_JUMP_LAND)
		{
			if(m_pWPhysState->IsServer()/* && m_iBerzerkTarget > 1*/)
			{
				const CMat4Dfp32& PosMat = m_pObj->GetPositionMatrix();
				CVec3Dfp32 Pos = PosMat.GetRow(3);

				TSelection<CSelection::LARGE_BUFFER> Selection;
				Pos += PosMat.GetRow(0) * 16.0f;
				m_pWPhysState->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_CHARACTER, Pos, 10.0f);
				CMat4Dfp32 Tmp = PosMat;
				Tmp.GetRow(3) = Pos;
				m_pWPhysState->Debug_RenderSphere(Tmp, 10.0f, 0xffffff00, 0.1f);

				const int16* pTargetList;
				int nTargets = m_pWPhysState->Selection_Get(Selection, &pTargetList);
				for (int iTarget = 0; iTarget < nTargets; iTarget++)
				{
					if (pTargetList[iTarget] != m_pObj->m_iObject)
					{
						CWObject_Message Msg;
						Msg.m_Msg = OBJMSG_GAME_GETPLAYER;
						Msg.m_Param0 = pTargetList[iTarget];
						CWObject_GameDM::CWO_PlayerDM* pPlayer = (CWObject_GameDM::CWO_PlayerDM *)m_pWPhysState->Phys_Message_SendToObject(Msg, m_pWPhysState->Game_GetObjectIndex());
						if (pPlayer && pPlayer->m_bIsHuman)
						{
							CWO_DamageMsg Dmg;
							Dmg.m_bForceValid = 0;
							Dmg.m_Force = 0;
							if(m_Phys_nInAirFrames < 6)
							{
								Dmg.m_Damage = 30;

								SetGravity(CVec3Dfp32(0.0f, 0.0f, -1.0f));
								m_Phys_nInAirFrames = 0;
								m_Phys_bInAir = 0;

								CWAG2I_Context AG2Context(m_pObj, m_pWPhysState, m_GameTime);
								m_AnimGraph2.SendJumpImpulse(&AG2Context, CXRAG2_Impulse(AG2_IMPULSETYPE_JUMP, AG2_IMPULSEVALUE_JUMP_LAND));

								Dmg.m_bForceValid = 1;
								Dmg.m_Force = PosMat.GetRow(0) * 10.0f;
							}
							else
							{
								Dmg.m_bForceValid = 1;
								Dmg.m_Force = PosMat.GetRow(0) * 20.0f;
								Dmg.m_Damage = 500;
							}
							Dmg.m_DamageType = 0;
							Dmg.m_SurfaceType = 0;
							Dmg.m_StunTicks = 0;
							Dmg.m_bPositionValid = 0;							
							Dmg.m_bSplattDirValid = 0;
							Dmg.m_Position = 0;
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
		}
	}
}


void CWO_CharDarkling_ClientData::SetGravity(const CVec3Dfp32& _NewGravity)
{
	// Don't set gravity while behavior is active
	if (m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BEHAVIORACTIVE)
		return;

	fp32 Len2 = _NewGravity.LengthSqr();
	bool bOk = Len2 > 0.99f && Len2 < 1.01f;
	M_ASSERT(bOk, "Invalid Gravity vector!");
	if (!bOk) 
		return;

	if (m_Flags & DARKLING_FLAGS_CAN_CLIMB)
	{
		m_Gravity = _NewGravity;
		m_Gravity_Estimated = _NewGravity;
	}
	else
	{
		m_Gravity = CVec3Dfp32(0,0,-1);
		m_Gravity_Estimated = m_Gravity;
	}

	DBG_OUT("Gravity set to %s\n", ((CVec3Dfp32)m_Gravity).GetString().Str());

	m_Phys_IdleTicks = 0;

//	m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;

	m_pWPhysState->Debug_RenderVector(m_pObj->GetPosition(), (CVec3Dfp32)m_Gravity * 32.0f, DEBUG_COLOR_BLUE, 4.0f);

	// TODO: This must be set whenever m_UpVector changes - not just when gravity changes!
	m_AnimGraph2.SetUpVector(-(CVec3Dfp32)m_Gravity);
}


void CWO_CharDarkling_ClientData::UpdateMatrices()
{
	const CMat4Dfp32& PosMat = m_pObj->GetPositionMatrix();

	m_MatLook = PosMat;
	m_MatBody = PosMat;

	const int PhysType = CWObject_Character::Char_GetPhysType(m_pObj);
	if (PhysType != PLAYER_PHYS_NOCLIP)
	{
		const CVec3Dfp32& Gravity = m_Gravity;
		m_MatLook.GetRow(2) = -Gravity;
		m_MatLook.RecreateMatrix(2, 0);

		m_MatBody.GetRow(2) = m_UpVector;
		m_MatBody.RecreateMatrix(2, 0);
	}
}


bool CWO_CharDarkling_ClientData::CheckOnGround()
{
	bool bOnGround = false;
	bool bSteepSlopeAhead = false;
	CVec3Dfp32 GroundNormal;

	const int PhysType = CWObject_Character::Char_GetPhysType(m_pObj);
	if (PhysType != PLAYER_PHYS_NOCLIP)
	{
		if ((m_State == DARKLING_STATE_NORMAL)&&(m_Flags & DARKLING_FLAGS_CAN_CLIMB)&&(m_Phys_bInAir)&&(m_Phys_nInAirFrames > 6))
		{	// Fall DOWN!
			SetGravity(CVec3Dfp32(0.0f,0.0f,-1.0f));
		}
		CVec3Dfp32 Gravity = m_Gravity;

		// Check if we're standing on ground
		CWO_PhysicsState PhysState(m_pObj->GetPhysState());
		PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PLAYERPHYSMODEL;
		PhysState.m_ObjectFlags = 0;
		CMat4Dfp32 p0 = m_MatBody;
		CMat4Dfp32 p1 = m_MatBody;
		PhysState.m_Prim[0].SetDim( PhysState.m_Prim[0].GetDim() - CVec3Dfp32(1.0f) );	// Make 1 unit smaller (prevent side collisions)
		p1.GetRow(3) += Gravity * 2.0f;										// Move 2 units down   (help detect ground collisions)

		//This is needed so it's harder to fall off walls when landing on them in multiplayer
		if(m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWPhysState->Game_GetObjectIndex()))
			p1.GetRow(3) += Gravity * 2.0f;

		CCollisionInfo CInfo_1;
		bOnGround = m_pWPhysState->Phys_IntersectWorld(m_PhysSelection, PhysState, p0, p1, m_pObj->m_iObject, &CInfo_1);
		if (!CInfo_1.m_bIsValid)
			bOnGround = false;

		if (bOnGround)
		{
			bool NoClimbArea = false;
			if(CInfo_1.m_bIsValid)
			{
				const CXW_SurfaceKeyFrame *pBaseFrame = CInfo_1.m_pSurface->GetBaseFrame();
				if(pBaseFrame && pBaseFrame->m_Medium.m_MediumFlags & XW_MEDIUM_SOLID && pBaseFrame->m_Friction == 0.0f)
				{
					SetGravity(CVec3Dfp32(0.0f, 0.0f, -1.0f));
					return false;
				}
			}
			GroundNormal = CInfo_1.m_Plane.n;
#ifndef M_RTM
			m_pWPhysState->Debug_RenderVector(CInfo_1.m_Pos, CInfo_1.m_Plane.n * 16.0f, 0xF000FF00,1.0f,true);
#endif
			// Save last good position (but don't send it over the network) - this is used by panic recovery
			if (!(m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD))
				m_CornerRef.m_Value = m_MatBody.GetRow(3) + m_MatBody.GetRow(2) * (-16.0f);
		}

		if (!m_Phys_bInAir && !bOnGround)
		{	// Blend between gravity and estimated gravity
			CVec3Dfp32 NewGravity = m_Gravity.m_Value + m_Gravity_Estimated.m_Value;
			if (NewGravity.LengthSqr() > 0.001f)
			{	// The two normals may very well be opposite so this may be quite common
				NewGravity.Normalize();
				SetGravity(NewGravity);
			}
		}

#ifndef M_RTM
		if (!m_Phys_bInAir && !bOnGround)
		{
			DBG_OUT("Went from ground to air\n");
			if (!(m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD) && (m_State == DARKLING_STATE_NORMAL))
			{
				DBG_OUT("WARNING: In air while walking, but no steep-slope was detected!\n");
				m_pWPhysState->Debug_RenderSphere(m_MatLook, 16.0f, DEBUG_COLOR_RED, 4.0f);
			}
		}
		else if (m_Phys_bInAir && bOnGround)
		{
			fp32 k = -(GroundNormal * Gravity);
			if (k < 0.2f)
				DBG_OUT("in air, think we're on ground, but didn't hit ground?\n");
		}
#endif


		bool bIsWallClimber = (m_Flags & DARKLING_FLAGS_CAN_CLIMB) != 0;

		// Check if there is ground ahead of us
		CVec3Dfp32 MoveDir = m_CurrMovement;
		if (MoveDir.LengthSqr() < 0.1f)
			MoveDir = m_pObj->GetMoveVelocity();

		bool bNeedCheck = bOnGround || !(m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD);

		if (bNeedCheck && bIsWallClimber && (MoveDir.LengthSqr() > 0.1f) && (m_State == DARKLING_STATE_NORMAL))
		{
			MoveDir.SetLength(32);
			p0.GetRow(3) = m_MatBody.GetRow(3) + MoveDir;
			p1.GetRow(3) = p0.GetRow(3) + (Gravity * 8.0f);

			CInfo_1.Clear();
			bool bHit = m_pWPhysState->Phys_IntersectWorld(m_PhysSelection, PhysState, p0, p1, m_pObj->m_iObject, &CInfo_1);
#ifndef M_RTM
			if (bHit)
			{
				m_pWPhysState->Debug_RenderVector(p0.GetRow(3), p1.GetRow(3)-p0.GetRow(3), 0xF0FF0000,0.0f, false);
				m_pWPhysState->Debug_RenderSphere(p1, 16.0f, 0xF0FF0000, 0.0f, false);
			}
			else
			{
				m_pWPhysState->Debug_RenderVector(p0.GetRow(3), p1.GetRow(3)-p0.GetRow(3), 0xF00000FF,0.0f, false);
				m_pWPhysState->Debug_RenderSphere(p1, 16.0f, 0xF00000FF, 0.0f, false);
			}
#endif
			if ((bHit)&&(CInfo_1.m_bIsValid))
			{
				m_Gravity_Estimated = -CInfo_1.m_Plane.n;
				if (!bOnGround)
				{
					SetGravity(-CInfo_1.m_Plane.n);
				}
			}

			if (!bHit)
			{
				bSteepSlopeAhead = true;

				if (!bOnGround)
					DBG_OUT("NOTE: Detected steep slope. This would've been missed...\n");

				if (!(m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD))
				{
					DBG_OUT("Convex Corner detected!\n");
					DBG_OUT(" - speed is %s\n", m_pObj->GetMoveVelocity().GetString().Str());
					m_Flags = m_Flags | DARKLING_FLAGS_STEEP_SLOPE_AHEAD;

					// Find new ground plane
					m_Gravity_Estimated = Gravity;
					m_CornerRef = m_MatBody.GetRow(3) + m_MatBody.GetRow(2) * (-16.0f);
					p0.GetRow(3) = m_MatBody.GetRow(3) + MoveDir;
					p1.GetRow(3) = p0.GetRow(3) + (Gravity * 32.0f);
					CInfo_1.Clear();
					bool bNewGround = m_pWPhysState->Phys_IntersectWorld(m_PhysSelection, PhysState, p0, p1, m_pObj->m_iObject, &CInfo_1);
#ifndef M_RTM
					if (bNewGround)
					{
						m_pWPhysState->Debug_RenderVector(p0.GetRow(3), p1.GetRow(3)-p0.GetRow(3), 0xF0FF0000,0.0f, false);
						m_pWPhysState->Debug_RenderSphere(p1, 16.0f, 0xF0FF0000, 0.0f, false);
					}
					else
					{
						m_pWPhysState->Debug_RenderVector(p0.GetRow(3),p1.GetRow(3)-p0.GetRow(3), 0xF00000FF,0.0f, false);
						m_pWPhysState->Debug_RenderSphere(p1, 16.0f, 0xF00000FF, 0.0f, false);
					}
#endif
					if (!bNewGround)
					{
						p0.GetRow(3) = p1.GetRow(3);
						p1.GetRow(3) -= MoveDir/* * 0.5f*/;	// Go the whole 32 units
						bNewGround = m_pWPhysState->Phys_IntersectWorld(m_PhysSelection, PhysState, p0, p1, m_pObj->m_iObject, &CInfo_1);
#ifndef M_RTM
						if (bNewGround)
						{
							m_pWPhysState->Debug_RenderVector(p0.GetRow(3), p1.GetRow(3)-p0.GetRow(3), 0xF0FF0000,0.0f, false);
							m_pWPhysState->Debug_RenderSphere(p1, 16.0f, 0xF0FF0000, 0.0f, false);
						}
						else
						{
							m_pWPhysState->Debug_RenderVector(p0.GetRow(3), p1.GetRow(3)-p0.GetRow(3), 0xF00000FF,0.0f, false);
							m_pWPhysState->Debug_RenderSphere(p1, 16.0f, 0xF00000FF, 0.0f, false);
						}
#endif
					}
					if ((bNewGround)&&(CInfo_1.m_bIsValid))
					{
						m_Gravity_Estimated = -CInfo_1.m_Plane.n;
						if (!bOnGround)
						{
							SetGravity(-CInfo_1.m_Plane.n);
						}
						m_CornerRef = ((CVec3Dfp32)m_CornerRef + CInfo_1.m_Pos) * 0.5f;
						DBG_OUT(" - next gravity is %s\n", CVec3Dfp32(m_Gravity_Estimated).GetString().Str());
						DBG_OUT(" - corner pos is %s\n", CVec3Dfp32(m_CornerRef).GetString().Str());
						m_pWPhysState->Debug_RenderVertex(m_CornerRef, DEBUG_COLOR_RED, 3.0f);
						m_pWPhysState->Debug_RenderVector(m_CornerRef, CVec3Dfp32(m_Gravity_Estimated)*-16.0f, DEBUG_COLOR_RED, 3.0f);
					}
					else
					{
						DBG_OUT("Could not find new gravity plane... gaaaah!\n");
					}
				}
			}
		}
		else if (bOnGround && (m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD))
		{
			DBG_OUT("Warning: Back on ground in steep-slope mode, but didn't check steepslope!\n");
		}

		m_Phys_bInAir = !bOnGround;
		if (bOnGround)
		{
			if (m_Phys_nInAirFrames)
			{
				DBG_OUT("Back on ground\n");

				/*if (m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD)
				{
					SetGravity(m_Gravity_Estimated);
					m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;
				}*/

			//	if (!bSteepSlopeAhead)
			//		m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;	// was in air, but no more..
			}
			m_Phys_nInAirFrames = 0;
		}

		if (bOnGround && !bSteepSlopeAhead)
		{
			if (m_Flags & DARKLING_FLAGS_STEEP_SLOPE_AHEAD)
			{
				DBG_OUT("Coast is clear!\n");

				fp32 k = -(Gravity * GroundNormal);
				if (k < 0.9f)
					DBG_OUT(" - but pretty bad gravity :(\n");
			//	SetGravity(m_Gravity_Estimated);
				m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;
			}
		}
	}

	return bOnGround;
}

CXR_ModelInstance* CWO_CharDarkling_ClientData::GetModelInstance(CXR_Model* _pModel, const uint8& _iSlot)
{
	if(!m_spExtraModelInstance[_iSlot])
	{
		m_spExtraModelInstance[_iSlot] = _pModel->CreateModelInstance();
		if(m_spExtraModelInstance[_iSlot])
			m_spExtraModelInstance[_iSlot]->Create(_pModel, CXR_ModelInstanceContext(m_pWPhysState->GetGameTick(), m_pWPhysState->GetGameTickTime()));
	}

	return m_spExtraModelInstance[_iSlot];
}

