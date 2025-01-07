/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			Inverse kinamatics system.

Author:			Fredrik Johansson
				Jakob Ericsson

Copyright:		2005 Starbreeze Studios AB

Contents:		CIKSystem Implementation

Comments:

History:
051207:		Created file
060109:		Rewrote everything
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "CIKSystem.h"
#include "../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Hook.h"
#include "WObj_Char/WObj_CharDarkling_ClientData.h"
#include "../../Shared/MOS/Classes/GameWorld/Client/WClient.h"
#include "WObj_Char.h"

// This file should be moved to shared

CIKSystem::CIKSystem()
{
	m_iIKMode = 0;
	m_TargetPos.k[0] = 0.0f;
	m_TargetPos.k[1] = 0.0f;
	m_TargetPos.k[2] = 0.0f;
	m_fTargetAngle = 0.0f;
}


void CIKSystem::LowerSkeleton(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CXR_Model* _pModel, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{

	// Try to lower skeleton prior any IK calculations
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);

	// "Global" scale should only be set on root node
	CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(_pSkelInstance->m_pBoneTransform[0],3);
	CMat4Dfp32 *P = &_pSkelInstance->m_pBoneTransform[0];
	CMat43fp32 GlobalScale;
	CMat43fp32 Temp;
	GlobalScale.Unit();
	fp32 TestScale = pCD->m_CharGlobalScale;
	GlobalScale.k[0][0] *= TestScale;
	GlobalScale.k[1][1] *= TestScale;
	GlobalScale.k[2][2] *= TestScale;
	CVec3Dfp32 HeightDiff(0.0f, 0.0f, 0.0f);

	CCollisionInfo IntersectInfoLeft, IntersectInfoRight;
	CVec3Dfp32 TargetPos(0.0f, 0.0f, 0.0f);

	int32 ObjectFlags			= 0;
	int32 ObjectIntersectFlags	= OBJECT_FLAGS_WORLD;
	int32 MediumFlags			= XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;

	CMat4Dfp32 FootWorldMatMP;
	CVec3Dfp32 LeftFootWorld, RightFootWorld, HitRightFoot, HitLeftFoot;
	CVec3Dfp32 StartPos, EndPos;
	bool bHit = false;
	fp32 HitFootHeight = 0.0f;

	// Right toe: 68
	// Left  toe: 69

	// Left foot
	FootWorldMatMP = _pSkelInstance->m_pBoneTransform[69];
	LeftFootWorld		= _pSkel->m_lNodes[69].m_LocalCenter;
	LeftFootWorld		= LeftFootWorld * FootWorldMatMP;
	LeftFootWorld.k[2] -= 1.29f;
	EndPos			= LeftFootWorld;
	EndPos.k[2]		-= 25.0f;	
	StartPos		= LeftFootWorld;
	StartPos.k[2]	+= 5.0f;
	bHit = _pWPhysState->Phys_IntersectLine(StartPos, EndPos, ObjectFlags, ObjectIntersectFlags, MediumFlags, &IntersectInfoLeft, _pObj->m_iObject);
	if(bHit && IntersectInfoLeft.m_bIsValid)
	{
		HitLeftFoot = IntersectInfoLeft.m_Pos;
	}
	else
	{
		int a = 0;
	}

	// Right leg
	FootWorldMatMP = _pSkelInstance->m_pBoneTransform[68];
	RightFootWorld		= _pSkel->m_lNodes[68].m_LocalCenter;
	RightFootWorld		= RightFootWorld * FootWorldMatMP;
	RightFootWorld.k[2] -= 1.29f;
	EndPos			= RightFootWorld;
	EndPos.k[2]		-= 25.0f;	
	StartPos		= RightFootWorld;
	StartPos.k[2]	+= 5.0f;
	bHit = _pWPhysState->Phys_IntersectLine(StartPos, EndPos, ObjectFlags, ObjectIntersectFlags, MediumFlags, &IntersectInfoRight, _pObj->m_iObject);
	if(bHit && IntersectInfoRight.m_bIsValid)
	{
		HitRightFoot = IntersectInfoRight.m_Pos;
	}
	else
	{
		int a = 0;
	}

	// The difference between the foot position and the heel is 3.59 units.

	CMat4Dfp32 Left, Right;
	Left.Unit();
	Left.GetRow(3) = HitLeftFoot;
	Right.Unit();
	Right.GetRow(3) = HitRightFoot;

	//_pWPhysState->Debug_RenderMatrix(Left, false);
	//_pWPhysState->Debug_RenderMatrix(Right, false);

	//Diff =  (FootWorld.k[2] - 3.59f)- M_Fabs(IntersectInfo.m_Pos.k[2]);

	// Left foot
	//fp32 DiffL = IntersectInfoLeft.m_Pos.k[2] - LeftFootWorld.k[2];
	fp32 DiffL = LeftFootWorld.k[2] - (IntersectInfoLeft.m_Pos.k[2]);

	// Right foot
	//fp32 DiffR = IntersectInfoRight.m_Pos.k[2] - RightFootWorld.k[2];
	fp32 DiffR = RightFootWorld.k[2] - (IntersectInfoRight.m_Pos.k[2]);

	fp32 Diff;
	Diff = (DiffL < DiffR) ? DiffL : DiffR;
	
	CMat4Dfp32 A, B, C, D;
	A.Unit();
	B.Unit();
	C.Unit();
	D.Unit();
	A.GetRow(3) = IntersectInfoLeft.m_Pos;
	B.GetRow(3) = IntersectInfoRight.m_Pos;
	C.GetRow(3) = LeftFootWorld;
	D.GetRow(3) = RightFootWorld;
	_pWPhysState->Debug_RenderMatrix(A, false);
	//_pWPhysState->Debug_RenderMatrix(B, false);
	_pWPhysState->Debug_RenderMatrix(C, false);
	//Diff += -3.59f;

    Diff += -3.39f; // Need some kind of epsilon value here.. MUST prevent the character from colliding with the underlying geometry..

	M_TRACE("Diff %f\n", Diff);
	
	if(Diff > 0.0f)
		Pos.k[2] -= Diff;
	
	//Pos.SetMatrixRow(GlobalScale,3);
	//GlobalScale.GetRow(3) = Pos;
	//_pSkelInstance->m_pBoneTransform[0].Multiply(GlobalScale,Temp);
	if(!bHit && Diff < 1.0f)
		Diff = 0.0f;

	P->GetRow(3).k[2] -= Diff;

	_pSkel->EvalNode(PLAYER_ROTTRACK_ROOT, P, _pSkelInstance);
}


bool CIKSystem::IKSolve(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CXR_Model* _pModel, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	fp32 stepSize = _pObj->m_PhysAttrib.m_StepSize;

	static bool bStepUpActive = false;
	static CMat4Dfp32 DebugIKFootPosition;

	//if(!bStepUpActive)
	//bStepUpActive = _pObj->m_bStepUpActive;

	for(uint8 i = 0; i < 2; i++)	// 0 = Right leg, 1 = Left leg
	{

		static CVec3Dfp32 RightTargetPos, LeftTargetPos;
		//static CIKSystem IKSolver;
		static bool		 bStateLowerSkeleton;
		bool			 bIsCloseToGeometry;

		struct IKBoneNode
		{
			uint8		m_Id;
			uint8       m_ParentId;
			bool		m_bRighFootIsLocked;
			bool		m_bLeftFootIsLocked;
			CVec3Dfp32	m_LeftTargetPos, m_RightTargetPos;
			fp32			m_HeightInfo;

			IKBoneNode::IKBoneNode()
			{
				m_Id = 0;
				m_ParentId = 0;
				m_bRighFootIsLocked = false;
				m_bLeftFootIsLocked = false;
				m_HeightInfo = 0.0f;
			};
		};

		static IKBoneNode iNodes[4];
		int ToeId = 0;

		if(i == 0)	// Right leg
		{
			iNodes[0].m_Id = PLAYER_ROTTRACK_RLEG; 
			iNodes[1].m_Id = PLAYER_ROTTRACK_RKNEE; 
			iNodes[2].m_Id = PLAYER_ROTTRACK_RFOOT;
			iNodes[3].m_Id = PLAYER_ATTACHPOINT_RFOOT;

			// Parents
			iNodes[0].m_ParentId = PLAYER_ROTTRACK_ROOT;
			iNodes[1].m_ParentId = PLAYER_ROTTRACK_RLEG;
			iNodes[2].m_ParentId = PLAYER_ROTTRACK_RKNEE;

			ToeId = 68;
		}
		else  // Left leg
		{
			iNodes[0].m_Id = PLAYER_ROTTRACK_LLEG; 
			iNodes[1].m_Id = PLAYER_ROTTRACK_LKNEE; 
			iNodes[2].m_Id = PLAYER_ROTTRACK_LFOOT;
			iNodes[3].m_Id = PLAYER_ATTACHPOINT_LFOOT;

			iNodes[0].m_ParentId = PLAYER_ROTTRACK_ROOT;
			iNodes[1].m_ParentId = PLAYER_ROTTRACK_LLEG;
			iNodes[2].m_ParentId = PLAYER_ROTTRACK_LKNEE;

			ToeId = 69;
		}

		CCollisionInfo IntersectInfo;
		CVec3Dfp32 TargetPos(0.0f, 0.0f, 0.0f);

		int32 ObjectFlags			= 0;
		int32 ObjectIntersectFlags	= OBJECT_FLAGS_WORLD;
		int32 MediumFlags			= XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;

		CMat4Dfp32 FootWorldMatMP = _pSkelInstance->m_pBoneTransform[iNodes[2].m_Id];

		CVec3Dfp32 FootWorld;
		FootWorld		= _pSkel->m_lNodes[iNodes[2].m_Id].m_LocalCenter;
		//FootWorldLowered.k[2]	= 0.0f;
		FootWorld		= FootWorld * FootWorldMatMP;
		FootWorldMatMP.GetRow(3) = FootWorld;

		CVec3Dfp32 StartPos, EndPos;
		EndPos			= FootWorld;
		EndPos.k[2]		-= 25.0f;	
		StartPos		= FootWorld;
		StartPos.k[2]	+= 5.0f;

		const CMat4Dfp32& KneeWorldMat = _pSkelInstance->m_pBoneTransform[iNodes[1].m_Id];
		CVec3Dfp32 KneeWorldVec = _pSkel->m_lNodes[iNodes[1].m_Id].m_LocalCenter;
		KneeWorldVec = KneeWorldVec * KneeWorldMat;

		CVec3Dfp32 OffsetFromKnee(30.0f, 0.0f, 0.0f);
		CVec3Dfp32 KneeDir;
		CMat4Dfp32 FromKnee, TempMat;
		FromKnee.Unit();
		TempMat.Unit();
		KneeDir = FootWorld - KneeWorldVec;
		KneeDir.Normalize();
		KneeDir.SetMatrixRow(TempMat, 0);
		TempMat.RecreateMatrix(0, 2);
		TempMat.GetRow(3) = KneeWorldVec;

		FromKnee.GetRow(0) = TempMat.GetRow(0);
		FromKnee.GetRow(1) = TempMat.GetRow(1);
		FromKnee.GetRow(2) = TempMat.GetRow(2);
		FromKnee.GetRow(3) = KneeWorldVec;

		// Transform the "knee-point" 30 units in the knee´s direction
		FromKnee.GetRow(3) = OffsetFromKnee * FromKnee;

		CVec3Dfp32 DbgKnee, DbgFromKnee;
		DbgKnee = KneeWorldVec;
		DbgFromKnee = FromKnee.GetRow(3);

		// Test leg collision: 
		// Take current world point in the leg(start point) and transform it 50 units in legs forward direction(endpoint).
		// Find out where it collide.
		const CMat4Dfp32& RootWorldMat = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_ROOT];
		CVec3Dfp32 RootWorldVec = _pSkel->m_lNodes[PLAYER_ROTTRACK_ROOT].m_LocalCenter;
		RootWorldVec = RootWorldVec * RootWorldMat;

		CVec3Dfp32 OffsetFromRoot(50.0f, 0.0f, 0.0f);
		CVec3Dfp32 LegDir;
		CVec3Dfp32 NewPoint;
		CMat4Dfp32 TempMatLeg;
		TempMatLeg.Unit();
		LegDir = KneeWorldVec - RootWorldVec;
		LegDir.Normalize();
		LegDir.SetMatrixRow(TempMatLeg, 0);
		TempMatLeg.RecreateMatrix(0, 2);
		TempMatLeg.GetRow(3) = RootWorldVec;
		NewPoint = OffsetFromRoot * TempMatLeg;

		CVec3Dfp32 FromLegToGroundVec;
		CVec3Dfp32 OffsetToGround(0.0f, 0.0f, -50.0f);
		CMat4Dfp32 TempRotMat;
		TempRotMat.Unit();
		TempRotMat.GetRow(3) = RootWorldVec;
		FromLegToGroundVec = OffsetToGround * TempRotMat; 
		//_pWPhysState->Debug_RenderWire(RootWorldVec, NewPoint, 0xff7f7f7f, 10.0f, false);
		//_pWPhysState->Debug_RenderWire(RootWorldVec, FromLegToGroundVec, 0xff7f7f7f, 10.0f, false);

		CVec3Dfp32 RootDirDown;
		RootDirDown = FromLegToGroundVec - RootWorldVec;
		RootDirDown.Normalize();
		// Check angle between LegDir and Root dir(down)
		fp32 RootLegAngle = (LegDir * RootDirDown);

		if(RootLegAngle < 0.90f)
		{
			CCollisionInfo TestHit;

			bool bHit;
			// Check collision
			bHit = _pWPhysState->Phys_IntersectLine(RootWorldVec, NewPoint, ObjectFlags, ObjectIntersectFlags, MediumFlags, &TestHit, _pObj->m_iObject);

			// We have a collision
			if(bHit && TestHit.m_bIsValid)
			{
				DebugIKFootPosition.Unit();
				DebugIKFootPosition.GetRow(3) = TestHit.m_Pos;
				if(i==0)
				{
					iNodes[2].m_RightTargetPos = TestHit.m_Pos;
				}
				else
				{
					iNodes[2].m_LeftTargetPos = TestHit.m_Pos;
				}
			}
		}
		// End test leg collision

		//_pWPhysState->Debug_RenderMatrix(DebugIKFootPosition, false);

		if(i == 0)
		{
			// Debug right leg
			//_pWPhysState->Debug_RenderWire(DbgKnee, DbgFromKnee, 0xff7f7f7f, 30.0f, false);
			//_pWPhysState->Debug_RenderWire(StartPos, EndPos, 0xfffff00f, 30.0f, false);

			CVec3Dfp32 KneeToFoot, StartToEnd;
			KneeToFoot = DbgFromKnee - DbgKnee;
			KneeToFoot.Normalize();
			StartToEnd = EndPos - StartPos;
			StartToEnd.Normalize();
			fp32 DotProd = KneeToFoot * StartToEnd;
			if(DotProd > 1.0f)
				DotProd = 1.0f;
			
			_pWPhysState->Phys_IntersectLine(StartPos, EndPos, ObjectFlags, ObjectIntersectFlags, MediumFlags, &IntersectInfo);

			fp32 Distance = IntersectInfo.m_Pos.k[2];
			bIsCloseToGeometry = (Distance < 1.0f) ? true : false;

			if((M_Fabs(DotProd) > 0.95f))
			{
				CVec3Dfp32 LeftF, RightF;
				LowerSkeleton(_pSkel, _pSkelInstance, _pModel, _pObj, _pWPhysState);

				// We are in a stair
				if(!iNodes[2].m_bRighFootIsLocked && bIsCloseToGeometry)
				{
					fp32 fStepUp = _pObj->m_PhysAttrib.m_StepSize;

					CVec3Dfp32 OffsetNextStair(10.0f, 0.0f, fStepUp);
					CVec3Dfp32 NextTargetPosition;	

					// We want the point under the foot
					FootWorldMatMP.GetRow(3).k[2] -= 3.59;

					CMat4Dfp32 FootMatPlayerDir;
					FootMatPlayerDir.Unit();
					_pObj->GetPositionMatrix().GetRow(0).SetRow(FootMatPlayerDir, 0);
					FootMatPlayerDir.RecreateMatrix(0, 2);
					FootMatPlayerDir.GetRow(3) = FootWorldMatMP.GetRow(3);
                    NextTargetPosition = OffsetNextStair * FootMatPlayerDir;

					CMat4Dfp32 DebugMat;
					DebugMat.Unit();
					//DebugMat.GetRow(0) = FootWorldMatMP.GetRow(0);
					//DebugMat.GetRow(1) = FootWorldMatMP.GetRow(1);
					//DebugMat.GetRow(2) = FootWorldMatMP.GetRow(2);*
					DebugMat.GetRow(3) = NextTargetPosition;

					CVec3Dfp32 FirstPos, SecondPos;
					FirstPos = NextTargetPosition;
					SecondPos = NextTargetPosition;
					SecondPos.k[2] -= 25.0f;

					CCollisionInfo CollisionInfo;

					_pWPhysState->Phys_IntersectLine(FirstPos, SecondPos, ObjectFlags, ObjectIntersectFlags, MediumFlags, &CollisionInfo);

					iNodes[2].m_RightTargetPos = DebugMat.GetRow(3);

					// Subtract the difference in height
					iNodes[2].m_RightTargetPos.k[2] -= (CollisionInfo.m_Pos.k[2]);

					//_pWPhysState->Debug_RenderMatrix(DebugMat, 10.0f, false);
				}
				
				if(iNodes[2].m_bRighFootIsLocked)
					M_TRACE("Right foot near ground!\n");

				// Lock right foot and get target position(once!)
				if(!iNodes[2].m_bRighFootIsLocked)
				{
					//_pWPhysState->Phys_IntersectLine(StartPos, EndPos, ObjectFlags, ObjectIntersectFlags, MediumFlags, &IntersectInfo);
					//RightTargetPos = IntersectInfo.m_Pos;
					
					RightTargetPos = iNodes[2].m_RightTargetPos;
					//RightTargetPos = RightF;
					
					iNodes[2].m_bRighFootIsLocked = true;
				}

				uint8 NodeArray[4], ParentArray[3];
				NodeArray[0] = iNodes[0].m_Id;
				NodeArray[1] = iNodes[1].m_Id;
				NodeArray[2] = iNodes[2].m_Id;
				NodeArray[3] = iNodes[3].m_Id;

				ParentArray[0] = iNodes[0].m_ParentId;
				ParentArray[1] = iNodes[1].m_ParentId;
				ParentArray[2] = iNodes[2].m_ParentId;

				// Add 2 unit to target position for right leg(approximation)
				CMat4Dfp32 FootWorldMat;
				CVec3Dfp32 OffsetToUnderFoot(2.0f, 0.0f, 0.0f);
				FootWorldMat.GetRow(0) = FootWorldMatMP.GetRow(0);
				FootWorldMat.GetRow(1) = FootWorldMatMP.GetRow(1);
				FootWorldMat.GetRow(2) = FootWorldMatMP.GetRow(2);
				FootWorldMat.GetRow(3) = FootWorld;
				RightTargetPos = OffsetToUnderFoot * FootWorldMat;

				/*static CVec3Dfp32 iVNodes[4];
				iVNodes[0] = (CVec3Dfp32(-68.0f, -168.0f, 8.0f)); // First stair
				iVNodes[1] = (CVec3Dfp32(-76.0f, -172.0f, 16.0f)); // Second stair
				iVNodes[2] = (CVec3Dfp32(-84.0f, -168.0f, 24.0f)); // Third stair
				iVNodes[3] = (CVec3Dfp32(-92.0f, -172.0f, 32.0f)); // Fourth stair*/

				//RightTargetPos = CVec3Dfp32(-68.0f, -168.0f, 8.0f);

				bool bResult = false;

				//bResult = IKSolver.CalcCCD(pSkel, pSkelInstance, pModel, 2, NodeArray, ParentArray, RightTargetPos, _pWPhysState);

				
				//if(iNodes[2].m_bRighFootIsLocked)
					//bResult = CalcCCD(_pSkel, _pSkelInstance, _pModel, 2, NodeArray, ParentArray, RightTargetPos, _pWPhysState);

					//bResult = CalcCCD(_pSkel, _pSkelInstance, _pModel, 2, NodeArray, ParentArray, RightTargetPos, _pWPhysState);

				if(!bResult)
				{
					int a = 0;
				}

				CMat4Dfp32 DebugTargetPos;
				DebugTargetPos.Unit();
				DebugTargetPos.GetRow(3) = iNodes[2].m_RightTargetPos;//IntersectInfo.m_Pos;
				_pWPhysState->Debug_RenderMatrix(DebugTargetPos, false);

				bStateLowerSkeleton = true;
			}
			else
			{
				// No IK, unlock right foot
				if(iNodes[2].m_bRighFootIsLocked)
					iNodes[2].m_bRighFootIsLocked = false;

				bStateLowerSkeleton = false;
			}
		}
		else
		{
			// Debug left leg
			//_pWPhysState->Debug_RenderWire(DbgKnee, DbgFromKnee, 0xfff0000f, 30.0f, false);
			//_pWPhysState->Debug_RenderWire(StartPos, EndPos, 0xfffff00f, 30.0f, false);

			CVec3Dfp32 KneeToFoot, StartToEnd;
			KneeToFoot = DbgFromKnee - DbgKnee;
			KneeToFoot.Normalize();
			StartToEnd = EndPos - StartPos;
			StartToEnd.Normalize();
			fp32 DotProd = KneeToFoot * StartToEnd;
			if(DotProd > 1.0f)
				DotProd = 1.0f;

			_pWPhysState->Phys_IntersectLine(StartPos, EndPos, ObjectFlags, ObjectIntersectFlags, MediumFlags, &IntersectInfo);

			fp32 Distance = IntersectInfo.m_Pos.k[2];
			bIsCloseToGeometry = (Distance < 1.0f) ? true : false;

			if((M_Fabs(DotProd) > 0.95f))
			{
				CVec3Dfp32 LeftT, RightT;
				LowerSkeleton(_pSkel, _pSkelInstance, _pModel, _pObj, _pWPhysState);

				// We are in a stair
				if(!iNodes[2].m_bLeftFootIsLocked && bIsCloseToGeometry)
				{
					// This is the highest stair in the world. Probably need adjust the final world position as their 
					// can be lower stair steps..	
					fp32 fStepUp = _pObj->m_PhysAttrib.m_StepSize;

					CVec3Dfp32 OffsetNextStair(10.0f, 0.0f, fStepUp);
					CVec3Dfp32 NextTargetPosition;	

					// We want the point under the foot
					FootWorldMatMP.GetRow(3).k[2] -= 3.59f;
					CMat4Dfp32 FootMatPlayerDir;
					FootMatPlayerDir.Unit();
					_pObj->GetPositionMatrix().GetRow(0).SetRow(FootMatPlayerDir, 0);
					FootMatPlayerDir.RecreateMatrix(0, 2);
					FootMatPlayerDir.GetRow(3) = FootWorldMatMP.GetRow(3);

					NextTargetPosition = OffsetNextStair * FootMatPlayerDir;

					CMat4Dfp32 DebugMat;
					DebugMat.Unit();
					//DebugMat.GetRow(0) = FootWorldMatMP.GetRow(0);
					//DebugMat.GetRow(1) = FootWorldMatMP.GetRow(1);
					//DebugMat.GetRow(2) = FootWorldMatMP.GetRow(2);*
					DebugMat.GetRow(3) = NextTargetPosition;

					CVec3Dfp32 FirstPos, SecondPos;
					FirstPos = NextTargetPosition;
					SecondPos = NextTargetPosition;
					SecondPos.k[2] -= 25.0f;

					CCollisionInfo CollisionInfo;

					_pWPhysState->Phys_IntersectLine(FirstPos, SecondPos, ObjectFlags, ObjectIntersectFlags, MediumFlags, &CollisionInfo);

					iNodes[2].m_LeftTargetPos = DebugMat.GetRow(3);

					// Subtract the difference in height
					iNodes[2].m_LeftTargetPos.k[2] -= (CollisionInfo.m_Pos.k[2]);

					//_pWPhysState->Debug_RenderMatrix(DebugMat, 10.0f, false);
				}

				if(iNodes[2].m_bLeftFootIsLocked)
					M_TRACE("Left leg near ground!\n");

				// Lock left foot and get target position(once!)
				if(!iNodes[2].m_bLeftFootIsLocked)
				{
					//_pWPhysState->Phys_IntersectLine(StartPos, EndPos, ObjectFlags, ObjectIntersectFlags, MediumFlags, &IntersectInfo);
					//LeftTargetPos = IntersectInfo.m_Pos;
					
					LeftTargetPos = iNodes[2].m_LeftTargetPos;
				
					//LeftTargetPos = LeftT;
					
					iNodes[2].m_bLeftFootIsLocked = true;
				}

				uint8 NodeArray[4], ParentArray[3];
				// Fill arrays with nodes
				NodeArray[0] = iNodes[0].m_Id;
				NodeArray[1] = iNodes[1].m_Id;
				NodeArray[2] = iNodes[2].m_Id;
				NodeArray[3] = iNodes[3].m_Id;

				ParentArray[0] = iNodes[0].m_ParentId;
				ParentArray[1] = iNodes[1].m_ParentId;
				ParentArray[2] = iNodes[2].m_ParentId;

				// Add 2 unit to target position for right leg(approximation)
				CMat4Dfp32 FootWorldMat;
				CVec3Dfp32 OffsetToUnderFoot(2.0f, 0.0f, 0.0f);
				FootWorldMat.GetRow(0) = FootWorldMatMP.GetRow(0);
				FootWorldMat.GetRow(1) = FootWorldMatMP.GetRow(1);
				FootWorldMat.GetRow(2) = FootWorldMatMP.GetRow(2);
				FootWorldMat.GetRow(3) = FootWorld;
				LeftTargetPos = OffsetToUnderFoot * FootWorldMat;

				bool bResult = false;
				// Solve the 3-node link for left leg
				//bResult = IKSolver.CalcCCD(pSkel, pSkelInstance, pModel, 2, NodeArray, ParentArray, LeftTargetPos, _pWPhysState);
				
				//if(iNodes[2].m_bLeftFootIsLocked)
					//bResult = CalcCCD(_pSkel, _pSkelInstance, _pModel, 2, NodeArray, ParentArray, LeftTargetPos, _pWPhysState);

					//bResult = CalcCCD(_pSkel, _pSkelInstance, _pModel, 2, NodeArray, ParentArray, LeftTargetPos, _pWPhysState);
		
				CMat4Dfp32 DebugTargetPos;
				DebugTargetPos.Unit();
				DebugTargetPos.GetRow(3) = IntersectInfo.m_Pos;
				_pWPhysState->Debug_RenderMatrix(DebugTargetPos, false);

				bStateLowerSkeleton = true;
			}
			else
			{
				// No IK, unlock left foot
				if(iNodes[2].m_bLeftFootIsLocked)
					iNodes[2].m_bLeftFootIsLocked = false;

				bStateLowerSkeleton = false;
			}
		}
	}

	return true;
}

void CIKSystem::FindNextIKTarget(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CXR_Model* _pModel, CWObject_CoreData* _pObj, CWO_Character_ClientData *_pCDFirst, CWorld_PhysState* _pWPhysState)
{
	/* Right.. Prototype code here. Should try to capture good candidate point for the IK solution for feet.
	The proper way of doing it would be to capture a presumably good point for the right foot when, for example, the left foot is about
	to hit the ground and vice versa. 
	*/

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	
	static CMat4Dfp32 LegWorldMat[2];
	static CVec3Dfp32 TargetPos[2];
	CVec3Dfp32 TracedKneePoint[2];
	int LegID, KneeID, FootID, ToeID;
	CCollisionInfo InfoFoot[2];
	CMat4Dfp32 UnitMat;
	UnitMat.Unit();
	static CMat4Dfp32 S(UnitMat), E(UnitMat);
	
	struct TargetPos
	{
		bool		m_bGotTarget;
        CVec3Dfp32	m_Target;

		TargetPos::TargetPos()
		{
			m_bGotTarget = false;
		}
	};
	
	static struct TargetPos Targets[2];

	
	for(uint8 i = 0; i < 2; i++)	// 0 = right leg, 1 = left leg
	{
		CMat4Dfp32 DbgMat, DbgTarget;

		_pCDFirst->m_aLegIKIsDirty[i] = false;

		if(i == 0)	// Right leg
		{
			LegID = PLAYER_ROTTRACK_RLEG;
			KneeID = PLAYER_ROTTRACK_RKNEE;
			FootID = PLAYER_ROTTRACK_RFOOT;
			ToeID = 68;
		}
		else		// Left leg
		{
			LegID = PLAYER_ROTTRACK_LLEG;
			KneeID = PLAYER_ROTTRACK_LKNEE;
			FootID = PLAYER_ROTTRACK_LFOOT;
			ToeID = 69;
		}

		LegWorldMat[i].Unit();
		// Knee world position
		LegWorldMat[i].GetRow(3) = _pSkel->m_lNodes[KneeID].m_LocalCenter * _pSkelInstance->m_pBoneTransform[KneeID];
		CVec3Dfp32 Direction = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_ROOT].GetRow(0);
		
		
		CMat4Dfp32 TempMat;
		TempMat.Unit();
		Direction.SetMatrixRow(TempMat, 0);
		TempMat.RecreateMatrix(0, 2);
		TempMat.GetRow(3) = LegWorldMat[i].GetRow(3);
	
		CVec3Dfp32 TraceP(26.0f, 0.0f, 0.0f);
		TracedKneePoint[i] =  TraceP * TempMat;
		CMat4Dfp32 Dbg;
		Dbg.Unit();
		Dbg = TempMat;
		Dbg.GetRow(3) = TracedKneePoint[i];
	
		S = TempMat;
		E.GetRow(3) = TracedKneePoint[i];

		int32 ObjectFlags			= 0;
		int32 ObjectIntersectFlags	= OBJECT_FLAGS_WORLD;
		int32 MediumFlags			= XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;

		CVec3Dfp32 StartPos, EndPos;
		EndPos			= TracedKneePoint[i];
		EndPos.k[2]		-= 35.0f;	
		StartPos		= TracedKneePoint[i];
		StartPos.k[2]	+= 10.0f;

		bool bHit;
		bHit = _pWPhysState->Phys_IntersectLine(StartPos, EndPos, ObjectFlags, ObjectIntersectFlags, MediumFlags, &InfoFoot[i], _pObj->m_iObject);
		
		if(bHit && InfoFoot[i].m_bIsValid)
		{
			// We have a collision
			TargetPos[i] = InfoFoot[i].m_Pos;

			_pWPhysState->Debug_RenderWire(TracedKneePoint[i], TargetPos[i], 0xff7f7f7f, false);

			// Hopefully roughly aligned with the plane
			CVec3Dfp32 Direction = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_ROOT].GetRow(0);

			CVec3Dfp32 SPos, EPos;
			CMat4Dfp32 TempMat;
			TempMat.Unit();
			Direction.SetMatrixRow(TempMat, 0);
			TempMat.RecreateMatrix(0, 2);
			TempMat.GetRow(3) = TargetPos[i];
			CVec3Dfp32 Offset(8.0f, 0.0f, 0.0f);
            EPos = Offset * TempMat;
			EPos.k[2] += 1.0f;

			DbgTarget = TempMat;
			DbgTarget.GetRow(3) = TargetPos[i];

			CCollisionInfo Check;
			SPos		= TargetPos[i];
			SPos.k[2]	+= 1.0f;
			
			CVec3Dfp32 DbgSPoint, DbgEPoint;
			DbgSPoint = SPos;
			DbgSPoint.k[2] -= 1.0f;
			DbgEPoint = EPos;
			DbgEPoint.k[2] -= 1.0f;
			_pWPhysState->Debug_RenderWire(DbgSPoint, DbgEPoint, 0xff7f7f7f, false);
			
			bHit = _pWPhysState->Phys_IntersectLine(SPos, EPos, ObjectFlags, ObjectIntersectFlags, MediumFlags, &Check, _pObj->m_iObject);
			if(bHit && Check.m_bIsValid)
			{
				//fp32 Diff = Check.m_Pos.k[0] - InfoFoot[i].m_Pos.k[0]; 
				fp32 Diff = Check.m_Pos.k[0] - (InfoFoot[i].m_Pos.k[0] - 4.0f); 
				

				if(M_Fabs(Diff) < 0.1f)
				{
					// We now have a good candidate point, use it!
					Targets[i].m_bGotTarget = true;
					Targets[i].m_Target = TargetPos[i];
			
				}

				// This is the desired point on the plane
				CMat4Dfp32 Target;
				Target.Unit();
				Target.GetRow(3) = Targets[i].m_Target;
				_pWPhysState->Debug_RenderMatrix(Target, false);

			}
		}
		_pWPhysState->Debug_RenderWire(S.GetRow(3), E.GetRow(3), 0xff7f7f7f, false);
	}

	/*
	fp32 aBlendValues[2];
	CVec3Dfp32 Vec3LegWorld, Vec3KneeWorld, Vec3FootWorld, Vec3ToeWorld, Vec3FootWorldLowered, Vec3ToeWorldLowered,
		Vec3TargetOrg, Vec3TargetNew, Vec3LegToKnee, Vec3KneeToTargetOrg, Vec3KneeToTargetNew, Vec3LegToTargetOrg, 
		Vec3LegToTargetNew, Vec3LegToFoot, Vec3LegToToe, Vec3LegToTargetFoot, Vec3LegToTargetToe;

	for(uint8 i = 0; i < 2; i++)	// 0 = right leg, 1 = left leg
	{
		CMat43fp32 *pMatLeg, *pMatKnee, *pMatFoot, *pMatToe;		

		if(i == 0)	//right leg
		{
			LegID = PLAYER_ROTTRACK_RLEG;
			KneeID = PLAYER_ROTTRACK_RKNEE;
			FootID = PLAYER_ROTTRACK_RFOOT;
			ToeID = 68;
		}
		else		//left leg
		{
			LegID = PLAYER_ROTTRACK_LLEG;
			KneeID = PLAYER_ROTTRACK_LKNEE;
			FootID = PLAYER_ROTTRACK_LFOOT;
			ToeID = 69;
		}

		pMatLeg		= &_pSkelInstance->m_pBoneTransform[LegID];
		pMatKnee	= &_pSkelInstance->m_pBoneTransform[KneeID];
		pMatFoot	= &_pSkelInstance->m_pBoneTransform[FootID];
		pMatToe		= &_pSkelInstance->m_pBoneTransform[ToeID];

		Vec3LegWorld				= _pSkel->m_lNodes[LegID].m_LocalCenter * (*pMatLeg);
		Vec3KneeWorld				= _pSkel->m_lNodes[KneeID].m_LocalCenter * (*pMatKnee);
		Vec3FootWorldLowered		= _pSkel->m_lNodes[FootID].m_LocalCenter;
		Vec3FootWorldLowered.k[2]	= 0.0f;
		Vec3FootWorldLowered		= Vec3FootWorldLowered * (*pMatFoot);
		Vec3FootWorld				= _pSkel->m_lNodes[FootID].m_LocalCenter * (*pMatFoot);
		Vec3ToeWorldLowered			= _pSkel->m_lNodes[ToeID].m_LocalCenter;
		Vec3ToeWorldLowered.k[2]	= 0.0f;
		Vec3ToeWorldLowered			= Vec3ToeWorldLowered * (*pMatToe);
		Vec3ToeWorld				= _pSkel->m_lNodes[ToeID].m_LocalCenter * (*pMatToe);

		if(_pCDFirst->m_aLegIKFootLocked[i])	//Foot is locked to the floor, do IK
		{
			if(bUseFoot[i])
			{
				Vec3TargetOrg = Vec3FootWorld;
				Vec3TargetNew = _pCDFirst->m_aVec3LegIKLockedFootPos[i];// + (Vec3FootWorld - Vec3FootWorldLowered);
				Vec3TargetNew.k[2] += 3.69f;
			}
			else 
			{
				Vec3TargetOrg = Vec3ToeWorld;
				Vec3TargetNew = _pCDFirst->m_aVec3LegIKLockedToePos[i];// + (Vec3ToeWorld - Vec3ToeWorldLowered);
				Vec3TargetNew.k[2] += 1.09f;
			}

		
			Vec3LegToKnee		= Vec3LegWorld	- Vec3KneeWorld;
			Vec3KneeToTargetOrg	= Vec3KneeWorld	- Vec3TargetOrg;
			Vec3KneeToTargetNew	= Vec3KneeWorld - Vec3TargetNew;
			Vec3LegToTargetOrg	= Vec3LegWorld	- Vec3TargetOrg;
			Vec3LegToTargetNew	= Vec3LegWorld	- Vec3TargetNew;

			Vec3LegToFoot		= Vec3LegWorld - Vec3FootWorld;
			Vec3LegToToe		= Vec3LegWorld - Vec3ToeWorld;
			Vec3LegToTargetFoot	= Vec3LegWorld - (_pCDFirst->m_aVec3LegIKLockedFootPos[i] + (Vec3FootWorld - Vec3FootWorldLowered));
			Vec3LegToTargetToe	= Vec3LegWorld - (_pCDFirst->m_aVec3LegIKLockedToePos[i] + (Vec3ToeWorld - Vec3ToeWorldLowered));

			fp32 DistanceLegToTargetOrg	= Vec3LegToTargetOrg.LengthSqr();
			fp32 DistanceLegToTargetNew	= Vec3LegToTargetNew.LengthSqr();
			fp32 DistanceLegToKnee		= Vec3LegToKnee.LengthSqr();
			fp32 DistanceKneeToTargetNew	= Vec3KneeToTargetNew.LengthSqr();
			fp32 DistanceKneeToTargetOrg = Vec3KneeToTargetOrg.LengthSqr();

			//Check if the target is in reach
			if(M_Sqrt(DistanceLegToTargetNew) > M_Sqrt(DistanceLegToKnee) + M_Sqrt(DistanceKneeToTargetOrg))
			{	//The target is to far away, move it a bit closer
				DistanceLegToTargetNew = M_Sqrt(DistanceLegToKnee) + M_Sqrt(DistanceKneeToTargetOrg);
				Vec3LegToTargetNew.Normalize();
				Vec3LegToTargetNew = Vec3LegToTargetNew * DistanceLegToTargetNew;
				Vec3TargetNew = Vec3LegWorld - Vec3LegToTargetNew;
			}

			//_pWPhysState->Debug_RenderSphere(MatDebug, 0.5f, DEBUG_COLOR_RED, true);

			// 0 = right, 1 = left
			//Ve3TargetNew = 
			CVec3Dfp32 Vec3FootToFirstStair = nIKHandles[0].m_IKHandle - Vec3FootWorld;
			CVec3Dfp32 Vec3FootToSecondStair = nIKHandles[1].m_IKHandle - Vec3FootWorld;
			CVec3Dfp32 Vec3FootToThirdStair = nIKHandles[2].m_IKHandle - Vec3FootWorld;
			CVec3Dfp32 Vec3FootToFourthStair = nIKHandles[3].m_IKHandle - Vec3FootWorld;


			fp32 DistFootToFirstStair = Vec3FootToFirstStair.LengthSqr();
			fp32 DistFootToSecondStair = Vec3FootToSecondStair.LengthSqr();
			fp32 DistFootToThirdStair = Vec3FootToThirdStair.LengthSqr();
			fp32 DistFootToFourthStair = Vec3FootToFourthStair.LengthSqr();

			//M_TRACE("Distance to first stair: %f\n", M_Sqrt(DistFootToFirstStair));

			CMat43fp32 MatBoneLocalPosCopyToe, MatBoneLocalPosCopyFoot, MatBoneLocalPosCopyKnee, MatTmp, MatPlaneRotation;
			CVec3Dfp32 Vec3Pos, Vec3ErrorCorrection;
			//Calcuate the leg-foot-toe plane, both current and target
			CVec3Dfp32 Vec3CurrentPlaneNormal, Vec3TargetPlaneNormal;

			Vec3LegToFoot.CrossProd(Vec3LegToToe, Vec3CurrentPlaneNormal);
			Vec3LegToTargetFoot.CrossProd(Vec3LegToTargetToe, Vec3TargetPlaneNormal);

			Vec3CurrentPlaneNormal.Normalize();
			Vec3TargetPlaneNormal.Normalize();

			MatDebug.GetRow(0) = Vec3TargetPlaneNormal;
			MatDebug.GetRow(1) = Vec3CurrentPlaneNormal;
			MatDebug.GetRow(2) = Vec3TargetPlaneNormal;
			MatDebug.GetRow(3) = Vec3LegWorld;
			//_pWPhysState->Debug_RenderMatrix(MatDebug, 0.5f);

			//Get the angle between the planes
			fp32 Dot = Vec3CurrentPlaneNormal * Vec3TargetPlaneNormal;
			if(Dot > 1.0f) Dot = 1.0f;	//if the normals are very close to each other the dot product gets too big sometimes
			fp32 PlaneAngleDifference = M_ACos(Dot) / (2.0f * _PI);

			//Get the the rotation axis
			CVec3Dfp32 Vec3PlaneRotationAxis;
			Vec3TargetPlaneNormal.CrossProd(Vec3CurrentPlaneNormal, Vec3PlaneRotationAxis);
			Vec3PlaneRotationAxis.Normalize();

			
			//Planes are aligned
			Vec3LegWorld				= _pSkel->m_lNodes[LegID].m_LocalCenter * (*pMatLeg);
			Vec3KneeWorld				= _pSkel->m_lNodes[KneeID].m_LocalCenter * (*pMatKnee);
			Vec3FootWorld				= _pSkel->m_lNodes[FootID].m_LocalCenter * (*pMatFoot);
			Vec3ToeWorld				= _pSkel->m_lNodes[ToeID].m_LocalCenter * (*pMatToe);

			if(bUseFoot[i])
			{
				Vec3TargetOrg = Vec3FootWorld;
				//Vec3TargetOrg.k[2] -= 3.69f;
			}
			else 
			{
				Vec3TargetOrg = Vec3ToeWorld;
				//Vec3TargetOrg.k[2] -= 1.09f;
			}

			fp32 rotation_angle = 0.0f;
			if(_pCDFirst->m_aLegIKBlendStatus[i] != 2)
			{	//Not blending out, do IK
				Vec3LegToKnee				= Vec3LegWorld	- Vec3KneeWorld;
				Vec3KneeToTargetOrg			= Vec3KneeWorld	- Vec3TargetOrg;
				Vec3LegToTargetOrg			= Vec3LegWorld	- Vec3TargetOrg;
				Vec3LegToTargetNew			= Vec3LegWorld	- Vec3TargetNew;

				fp32 DistanceLegToTargetOrg	= Vec3LegToTargetOrg.LengthSqr();
				fp32 DistanceLegToTargetNew	= Vec3LegToTargetNew.LengthSqr();
				fp32 DistanceLegToKnee		= Vec3LegToKnee.LengthSqr();
	
				DistanceKneeToTargetOrg	= Vec3KneeToTargetOrg.LengthSqr();

				fp32 CurrentLegAngle = M_ACos((DistanceLegToTargetOrg + DistanceLegToKnee - DistanceKneeToTargetOrg)
					/ (2.0f * M_Sqrt(DistanceLegToTargetOrg) * M_Sqrt(DistanceLegToKnee))) / (2.0f * _PI);

				fp32 TargetDot = (DistanceLegToTargetNew + DistanceLegToKnee - DistanceKneeToTargetOrg)
					/ (2.0f * M_Sqrt(DistanceLegToTargetNew) * M_Sqrt(DistanceLegToKnee));
				if(TargetDot > 1.0f)
					TargetDot = 1.0f;
				fp32 TargetLegAngle = M_ACos(TargetDot) / (2.0f * _PI);

				rotation_angle = TargetLegAngle - CurrentLegAngle; 
				_pCDFirst->m_aLegIKLegAngle[i] = rotation_angle;
			}
			else	//blending out, use old rotation values
				rotation_angle = _pCDFirst->m_aLegIKLegAngle[i];

			//Now we know how much we will need to rotate the leg
			if(bUseFoot[i])
				CQuatfp32(Vec3TargetPlaneNormal, aBlendValues[i] * rotation_angle).CreateMatrix3x3(MatPlaneRotation);
			else
				CQuatfp32(Vec3TargetPlaneNormal, aBlendValues[i] * -rotation_angle).CreateMatrix3x3(MatPlaneRotation);

			//apply the rotation to the leg
			MatTmp = (*pMatLeg);
			//Backup the position
			Vec3Pos = pMatLeg->GetRow(3);
			MatTmp.Multiply3x3(MatPlaneRotation, (*pMatLeg));
			pMatLeg->GetRow(3) = Vec3Pos;

			//Make sure that rotating this joint doesn't alter it's position
			Vec3ErrorCorrection = Vec3LegWorld - _pSkel->m_lNodes[LegID].m_LocalCenter * (*pMatLeg);
			pMatLeg->GetRow(3) += Vec3ErrorCorrection;

			MatBoneLocalPosCopyKnee = _pSkelInstance->m_pBoneLocalPos[KneeID];
			MatBoneLocalPosCopyKnee.GetRow(3) = (_pSkel->m_lNodes[KneeID].m_LocalCenter - 
				(_pSkel->m_lNodes[KneeID].m_LocalCenter * 
				_pSkelInstance->m_pBoneLocalPos[KneeID]));

			MatBoneLocalPosCopyKnee.Multiply((*pMatLeg), (*pMatKnee));

			MatBoneLocalPosCopyFoot = _pSkelInstance->m_pBoneLocalPos[FootID];
			MatBoneLocalPosCopyFoot.GetRow(3) = (_pSkel->m_lNodes[FootID].m_LocalCenter - 
				(_pSkel->m_lNodes[FootID].m_LocalCenter * 
				_pSkelInstance->m_pBoneLocalPos[FootID]));

			MatBoneLocalPosCopyFoot.Multiply((*pMatKnee), (*pMatFoot));

			MatBoneLocalPosCopyToe = _pSkelInstance->m_pBoneLocalPos[ToeID];
			MatBoneLocalPosCopyToe.GetRow(3) = (_pSkel->m_lNodes[ToeID].m_LocalCenter - 
				(_pSkel->m_lNodes[ToeID].m_LocalCenter * 
				_pSkelInstance->m_pBoneLocalPos[ToeID]));

			MatBoneLocalPosCopyToe.Multiply((*pMatFoot), (*pMatToe));

			//Now lets do the knee
			//Get the new world positions
			Vec3LegWorld	= _pSkel->m_lNodes[LegID].m_LocalCenter * (*pMatLeg);
			Vec3KneeWorld	= _pSkel->m_lNodes[KneeID].m_LocalCenter * (*pMatKnee);
			Vec3FootWorld	= _pSkel->m_lNodes[FootID].m_LocalCenter * (*pMatFoot);
			Vec3ToeWorld	= _pSkel->m_lNodes[ToeID].m_LocalCenter * (*pMatToe);

			CVec3Dfp32 Vec3DebugTmp = Vec3TargetNew - Vec3KneeWorld;
			Vec3DebugTmp.Normalize();
			Vec3DebugTmp = Vec3DebugTmp * M_Sqrt(DistanceKneeToTargetOrg);
			//_pWPhysState->Debug_RenderVector(Vec3KneeWorld, Vec3DebugTmp, DEBUG_COLOR_BLUE, 0.5f, true);

			if(_pCDFirst->m_aLegIKBlendStatus[i] != 2)
			{	//Not blending out, do IK
				if(bUseFoot[i])
					Vec3TargetOrg = _pSkel->m_lNodes[FootID].m_LocalCenter * (*pMatFoot);
				else
					Vec3TargetOrg = _pSkel->m_lNodes[ToeID].m_LocalCenter * (*pMatToe);

				Vec3KneeToTargetOrg	= Vec3KneeWorld	- Vec3TargetOrg;
				Vec3KneeToTargetNew	= Vec3KneeWorld - Vec3TargetNew;
				Vec3LegToTargetOrg	= Vec3LegWorld	- Vec3TargetOrg;
				Vec3LegToTargetNew	= Vec3LegWorld	- Vec3TargetNew;
				Vec3LegToKnee		= Vec3LegWorld	- Vec3KneeWorld;

				//DistanceKneeToTargetOrg = Vec3KneeToTargetOrg.LengthSqr();
				DistanceKneeToTargetNew = Vec3KneeToTargetNew.LengthSqr();
				DistanceLegToTargetNew	= Vec3LegToTargetNew.LengthSqr();
				DistanceLegToTargetOrg	= Vec3LegToTargetOrg.LengthSqr();

				//_pWPhysState->Debug_RenderVector(Vec3LegWorld, -Vec3LegToTargetNew, DEBUG_COLOR_RED, 0.5f, true);
				//_pWPhysState->Debug_RenderVector(Vec3KneeWorld, -Vec3KneeToTargetNew, DEBUG_COLOR_GREEN, 0.5f, true);
				//_pWPhysState->Debug_RenderVector(Vec3LegWorld, -Vec3LegToKnee, DEBUG_COLOR_BLUE, 0.5f, true);

				fp32 CurrentKneeAngle = M_ACos((DistanceLegToKnee + DistanceKneeToTargetOrg - DistanceLegToTargetOrg)
					/ (2.0f * M_Sqrt(DistanceLegToKnee) * M_Sqrt(DistanceKneeToTargetOrg))) / (2.0f * _PI);

				fp32 TargetKneeAngle = M_ACos((DistanceLegToKnee + DistanceKneeToTargetNew - DistanceLegToTargetNew)
					/ (2.0f * M_Sqrt(DistanceLegToKnee) * M_Sqrt(DistanceKneeToTargetNew))) / (2.0f * _PI);

				rotation_angle = TargetKneeAngle - CurrentKneeAngle;
				_pCDFirst->m_aLegIKKneeAngle[i] = rotation_angle;
			}
			else	//blending out, use old rotation values
				rotation_angle = _pCDFirst->m_aLegIKKneeAngle[i];

			//Now we know how much we will need to rotate the knee
			CQuatfp32(Vec3TargetPlaneNormal, aBlendValues[i] * -rotation_angle).CreateMatrix3x3(MatPlaneRotation);

			//apply the rotation to the knee
			MatTmp = (*pMatKnee);
			//Backup the position
			Vec3Pos = pMatKnee->GetRow(3);
			MatTmp.Multiply3x3(MatPlaneRotation, (*pMatKnee));
			pMatKnee->GetRow(3) = Vec3Pos;

			//Make sure that rotating this joint doesn't alter it's position
			Vec3ErrorCorrection = Vec3KneeWorld - _pSkel->m_lNodes[KneeID].m_LocalCenter * (*pMatKnee);
			pMatKnee->GetRow(3) += Vec3ErrorCorrection;

			MatBoneLocalPosCopyFoot = _pSkelInstance->m_pBoneLocalPos[FootID];
			MatBoneLocalPosCopyFoot.GetRow(3) = (_pSkel->m_lNodes[FootID].m_LocalCenter - 
				(_pSkel->m_lNodes[FootID].m_LocalCenter * 
				_pSkelInstance->m_pBoneLocalPos[FootID]));

			MatBoneLocalPosCopyFoot.Multiply((*pMatKnee), (*pMatFoot));

			MatBoneLocalPosCopyToe = _pSkelInstance->m_pBoneLocalPos[ToeID];
			MatBoneLocalPosCopyToe.GetRow(3) = (_pSkel->m_lNodes[ToeID].m_LocalCenter - 
				(_pSkel->m_lNodes[ToeID].m_LocalCenter * 
				_pSkelInstance->m_pBoneLocalPos[ToeID]));

			MatBoneLocalPosCopyToe.Multiply((*pMatFoot), (*pMatToe));

			Vec3ToeWorldLowered			= _pSkel->m_lNodes[ToeID].m_LocalCenter;
			Vec3ToeWorldLowered.k[2]	= 0.0f;
			Vec3ToeWorldLowered			= Vec3ToeWorldLowered * (*pMatToe);
			Vec3ToeWorld				= _pSkel->m_lNodes[ToeID].m_LocalCenter * (*pMatToe);
			Vec3FootWorldLowered		= _pSkel->m_lNodes[FootID].m_LocalCenter;
			Vec3FootWorldLowered.k[2]	= 0.0f;
			Vec3FootWorldLowered		= Vec3FootWorldLowered * (*pMatFoot);
			Vec3FootWorld				= _pSkel->m_lNodes[FootID].m_LocalCenter * (*pMatFoot);

			if(bUseFoot[i])
				Vec3TargetOrg = Vec3FootWorld;
			else 
				Vec3TargetOrg = Vec3ToeWorld;

			MatDebug.GetRow(0) = pMatLeg->GetRow(0);
			MatDebug.GetRow(1) = pMatLeg->GetRow(1);
			MatDebug.GetRow(2) = pMatLeg->GetRow(2);
			MatDebug.GetRow(3) = Vec3TargetOrg;
		}
	}*/
}


void CIKSystem::ProcessNodeArrray(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, const CXR_SkeletonAttachPoint* _pAttachWeapon, 
								  const CXR_SkeletonAttachPoint* _pAttachLeftHand, const uint8 _iNumberOfNodes, const SIKBone* _iNodes, CMat4Dfp32& _TargetPos, CWorld_PhysState* _pWPhysState)
{
	CMat4Dfp32 MatWeapon = _TargetPos;
	CXR_AnimState AnimStateWeapon;
		
	CMat4Dfp32 *pMatLeftShoulder	= &_pSkelInstance->m_pBoneTransform[_iNodes[0].m_iNodeID];
	CMat4Dfp32 *pMatLeftElbow		= &_pSkelInstance->m_pBoneTransform[_iNodes[1].m_iNodeID];
	CMat4Dfp32 *pMatLeftHand		= &_pSkelInstance->m_pBoneTransform[_iNodes[2].m_iNodeID];
	const CMat4Dfp32 *pMatLeftHandAttach	= &_pSkelInstance->m_pBoneTransform[_iNodes[3].m_iNodeID];

	CVec3Dfp32 Vec3LeftShoulderWorldPos, Vec3LeftElbowWorldPos, Vec3LeftHandWorldPos, Vec3TargetPos;
	CVec3Dfp32 Vec3LeftHandPosition	= _pAttachLeftHand->m_LocalPos.GetRow(3);
	Vec3LeftShoulderWorldPos		= _pSkel->m_lNodes[_iNodes[0].m_iNodeID].m_LocalCenter * (*pMatLeftShoulder);
	Vec3LeftElbowWorldPos			= _pSkel->m_lNodes[_iNodes[1].m_iNodeID].m_LocalCenter * (*pMatLeftElbow);
	Vec3LeftHandWorldPos			= Vec3LeftHandPosition * (*pMatLeftHand);
	Vec3TargetPos					= MatWeapon.GetRow(3);
	
	CVec3Dfp32 Vec3HandToHandAttach	= _pSkel->m_lNodes[_iNodes[2].m_iNodeID].m_LocalCenter * (*pMatLeftHand)
		- _pSkel->m_lNodes[_iNodes[3].m_iNodeID].m_LocalCenter * (*pMatLeftHandAttach);
	fp32 DistanceHandToHandAttach	= Vec3HandToHandAttach.Length();

	fp32 Angle = (m_fTargetAngle);

	//Adjust the target position so we can rotate the hand later	
	//We want the hand-weapon angle to be 45 degrees, _PI / 4 radians
	fp32 LengthOfAt		= M_Cos(Angle);
	fp32 LengthOfRight	= M_Sin(Angle);
	fp32 PercentOfAt		= LengthOfAt * LengthOfAt;
	fp32 PercentOfRight	= LengthOfRight * LengthOfRight;

	CVec3Dfp32	Vec3TargetAdjust;
	Vec3TargetAdjust = (MatWeapon.GetRow(1) * PercentOfRight) - (MatWeapon.GetRow(0) * PercentOfAt);
	Vec3TargetAdjust.Normalize();
	Vec3TargetPos += Vec3TargetAdjust * DistanceHandToHandAttach;

	CVec3Dfp32 Vec3ShoulderToHandTarget	= Vec3LeftShoulderWorldPos - Vec3TargetPos;
	CVec3Dfp32 Vec3ShoulderToHand		= Vec3LeftShoulderWorldPos - Vec3LeftHandWorldPos;
	CVec3Dfp32 Vec3ShoulderToElbow		= Vec3LeftShoulderWorldPos - Vec3LeftElbowWorldPos;
	CVec3Dfp32 Vec3ElbowToHand			= Vec3LeftElbowWorldPos - Vec3LeftHandWorldPos;

	fp32 DistanceShoulderToHandTarget	= Vec3ShoulderToHandTarget.LengthSqr();
	fp32 DistanceShoulderToHandTargetSqrt= Vec3ShoulderToHandTarget.Length();
	fp32 DistanceShoulderToElbow			= Vec3ShoulderToElbow.LengthSqr();
	fp32 DistanceShoulderToElbowSqrt		= Vec3ShoulderToElbow.Length();
	fp32 DistanceElbowToHand				= Vec3ElbowToHand.LengthSqr();	


	//Check if the point is in reach, if it's not go on without changing anything
	if(DistanceShoulderToHandTargetSqrt <= (DistanceShoulderToElbowSqrt + M_Sqrt(DistanceElbowToHand)))
	{
		//Calcuate the shoulder-elbow-hand plane, both current and target
		CVec3Dfp32 Vec3CurrentPlaneNormal, Vec3TargetPlaneNormal;
		Vec3ShoulderToElbow.CrossProd(Vec3ShoulderToHand, Vec3CurrentPlaneNormal);
		Vec3CurrentPlaneNormal.Normalize();
		Vec3ShoulderToElbow.CrossProd(Vec3ShoulderToHandTarget, Vec3TargetPlaneNormal);
		Vec3TargetPlaneNormal.Normalize();

		//Get the angle between the planes
		fp32 PlaneAngleDifference = M_ACos(Vec3CurrentPlaneNormal * Vec3TargetPlaneNormal) / (_PI2);

		//Get the the rotation axis
		CVec3Dfp32 Vec3PlaneRotationAxis;
		Vec3TargetPlaneNormal.CrossProd(Vec3CurrentPlaneNormal, Vec3PlaneRotationAxis);
		Vec3PlaneRotationAxis.Normalize();

		CMat43fp32 MatPlaneRotation;
		CQuatfp32(Vec3PlaneRotationAxis, PlaneAngleDifference).CreateMatrix3x3(MatPlaneRotation);

		//Apply the rotation to the shoulder
		CMat4Dfp32 MatTmp((*pMatLeftShoulder));
		//Backup the position
		CVec3Dfp32 Vec3Pos = pMatLeftShoulder->GetRow(3);
		MatTmp.Multiply3x3(MatPlaneRotation, (*pMatLeftShoulder));
		pMatLeftShoulder->GetRow(3) = Vec3Pos;

		//Make sure that rotating this joint doesn't alter it's position
		CVec3Dfp32 Vec3ErrorCorrection = Vec3LeftShoulderWorldPos - _pSkel->m_lNodes[_iNodes[0].m_iNodeID].m_LocalCenter * (*pMatLeftShoulder);
		pMatLeftShoulder->GetRow(3) += Vec3ErrorCorrection;

		CMat4Dfp32 MatBoneLocalPosCopyElbow = _pSkelInstance->m_pBoneLocalPos[_iNodes[1].m_iNodeID];
		MatBoneLocalPosCopyElbow.GetRow(3) = (_pSkel->m_lNodes[_iNodes[1].m_iNodeID].m_LocalCenter - 
			(_pSkel->m_lNodes[_iNodes[1].m_iNodeID].m_LocalCenter * 
			_pSkelInstance->m_pBoneLocalPos[_iNodes[1].m_iNodeID]));

		MatBoneLocalPosCopyElbow.Multiply((*pMatLeftShoulder), (*pMatLeftElbow));

		CMat4Dfp32 MatBoneLocalPosCopyHand = _pSkelInstance->m_pBoneLocalPos[_iNodes[2].m_iNodeID];
		MatBoneLocalPosCopyHand.GetRow(3) = (_pSkel->m_lNodes[_iNodes[2].m_iNodeID].m_LocalCenter - 
			(_pSkel->m_lNodes[_iNodes[2].m_iNodeID].m_LocalCenter * 
			_pSkelInstance->m_pBoneLocalPos[_iNodes[2].m_iNodeID]));

		MatBoneLocalPosCopyHand.Multiply((*pMatLeftElbow), (*pMatLeftHand));

		// Planes are aligned
		Vec3LeftElbowWorldPos		= _pSkel->m_lNodes[_iNodes[1].m_iNodeID].m_LocalCenter * (*pMatLeftElbow);
		Vec3LeftHandWorldPos		= Vec3LeftHandPosition * (*pMatLeftHand);
		Vec3ShoulderToHand			= Vec3LeftShoulderWorldPos - Vec3LeftHandWorldPos;

		fp32 DistanceShoulderToHand	= Vec3ShoulderToHand.LengthSqr();

		//Do the shoulder first
		fp32 CurrentShoulderAngle = M_ACos((DistanceShoulderToHand + DistanceShoulderToElbow - DistanceElbowToHand)
			/ (2.0f * M_Sqrt(DistanceShoulderToHand) * DistanceShoulderToElbowSqrt)) / (_PI2);

		fp32 TargetShoulderAngle = M_ACos((DistanceShoulderToHandTarget + DistanceShoulderToElbow - DistanceElbowToHand)
			/ (2.0f * DistanceShoulderToHandTargetSqrt * DistanceShoulderToElbowSqrt)) / (_PI2);

		//Now we know how much we will need to rotate the shoulder
		CQuatfp32(Vec3TargetPlaneNormal, TargetShoulderAngle - CurrentShoulderAngle).CreateMatrix3x3(MatPlaneRotation);

		//Apply the rotation to the shoulder
		MatTmp = (*pMatLeftShoulder);
		//Backup the position
		Vec3Pos = pMatLeftShoulder->GetRow(3);
		MatTmp.Multiply3x3(MatPlaneRotation, (*pMatLeftShoulder));
		pMatLeftShoulder->GetRow(3) = Vec3Pos;

		//Make sure that rotating this joint doesn't alter it's position
		Vec3ErrorCorrection = Vec3LeftShoulderWorldPos - _pSkel->m_lNodes[_iNodes[0].m_iNodeID].m_LocalCenter * (*pMatLeftShoulder);
		pMatLeftShoulder->GetRow(3) += Vec3ErrorCorrection;

		MatBoneLocalPosCopyElbow = _pSkelInstance->m_pBoneLocalPos[_iNodes[1].m_iNodeID];
		MatBoneLocalPosCopyElbow.GetRow(3) = (_pSkel->m_lNodes[_iNodes[1].m_iNodeID].m_LocalCenter - 
			(_pSkel->m_lNodes[_iNodes[1].m_iNodeID].m_LocalCenter * 
			_pSkelInstance->m_pBoneLocalPos[_iNodes[1].m_iNodeID]));

		MatBoneLocalPosCopyElbow.Multiply((*pMatLeftShoulder), (*pMatLeftElbow));

		MatBoneLocalPosCopyHand = _pSkelInstance->m_pBoneLocalPos[_iNodes[2].m_iNodeID];
		MatBoneLocalPosCopyHand.GetRow(3) = (_pSkel->m_lNodes[_iNodes[2].m_iNodeID].m_LocalCenter - 
			(_pSkel->m_lNodes[_iNodes[2].m_iNodeID].m_LocalCenter * 
			_pSkelInstance->m_pBoneLocalPos[_iNodes[2].m_iNodeID]));

		MatBoneLocalPosCopyHand.Multiply((*pMatLeftElbow), (*pMatLeftHand));

		//Now lets do the elbow
		//Get the new world positions
		CVec3Dfp32 Vec3ElbowToHandTarget;
		Vec3LeftElbowWorldPos		= _pSkel->m_lNodes[_iNodes[1].m_iNodeID].m_LocalCenter * (*pMatLeftElbow);
		Vec3ElbowToHandTarget		= Vec3LeftElbowWorldPos - Vec3TargetPos;

		fp32 DistanceElbowToHandTarget = Vec3ElbowToHandTarget.LengthSqr();

		fp32 CurrentElbowAngle = M_ACos((DistanceShoulderToElbow + DistanceElbowToHand - DistanceShoulderToHand)
			/ (2.0f * M_Sqrt(DistanceElbowToHand) * DistanceShoulderToElbowSqrt)) / (_PI2);

		fp32 TargetElbowAngle = M_ACos((DistanceShoulderToElbow + DistanceElbowToHandTarget - DistanceShoulderToHandTarget)
			/ (2.0f * M_Sqrt(DistanceElbowToHandTarget) * DistanceShoulderToElbowSqrt)) / (_PI2);

		//Now we know how much we will need to rotate the elbow
		CQuatfp32(Vec3TargetPlaneNormal, TargetElbowAngle - CurrentElbowAngle).CreateMatrix3x3(MatPlaneRotation);

		//Apply the rotation to the elbow
		MatTmp = (*pMatLeftElbow);
		//Backup the position
		Vec3Pos = pMatLeftElbow->GetRow(3);
		MatTmp.Multiply3x3(MatPlaneRotation, (*pMatLeftElbow));
		pMatLeftElbow->GetRow(3) = Vec3Pos;

		//Make sure that rotating this joint doesn't alter it's position
		Vec3ErrorCorrection = Vec3LeftElbowWorldPos - _pSkel->m_lNodes[_iNodes[1].m_iNodeID].m_LocalCenter * (*pMatLeftElbow);
		pMatLeftElbow->GetRow(3) += Vec3ErrorCorrection;

		_pSkel->EvalNode(_iNodes[2].m_iNodeID, pMatLeftElbow, _pSkelInstance);

		Vec3LeftHandWorldPos = Vec3LeftHandPosition * (*pMatLeftHand);

		//Now we will adjust the hand
		CMat43fp32 MatAttachWorld;
		_pAttachLeftHand->m_LocalPos.Multiply((*pMatLeftHandAttach), MatAttachWorld);

		fp32 CurrentWeaponHandAngle = M_ACos(MatAttachWorld.GetRow(0) * MatWeapon.GetRow(0)) / (_PI2);

		//Now we know how much we will need to rotate the hand
		CQuatfp32(MatWeapon.GetRow(2), 0.125f - CurrentWeaponHandAngle).CreateMatrix3x3(MatPlaneRotation);

		//Apply the rotation to the hand
		MatTmp = (*pMatLeftHand);
		//Backup the position
		Vec3Pos = pMatLeftHand->GetRow(3);
		MatTmp.Multiply3x3(MatPlaneRotation, (*pMatLeftHand));
		pMatLeftHand->GetRow(3) = Vec3Pos;

		Vec3ErrorCorrection = Vec3LeftHandWorldPos - _pSkel->m_lNodes[_iNodes[2].m_iNodeID].m_LocalCenter * (*pMatLeftHand);
		pMatLeftHand->GetRow(3) += Vec3ErrorCorrection;

		if((m_iIKMode == IK_MODE_LEFT_HAND) || (m_iIKMode == IK_MODE_RIGHT_HAND))
            PostEvalHands(_pSkel, _pSkelInstance, *pMatLeftHand);
	}
}


void CIKSystem::PostEvalHands(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CMat4Dfp32& _BaseMat)
{
	if(m_iIKMode == IK_MODE_LEFT_HAND)
	{
		// Left hand
		_pSkel->EvalNode(PLAYER_ROTTRACK_LHAND_THUMB_1, &_BaseMat, _pSkelInstance);
		_pSkel->EvalNode(PLAYER_ROTTRACK_LHAND_INDEX_1, &_BaseMat, _pSkelInstance);
		_pSkel->EvalNode(PLAYER_ROTTRACK_LHAND_MIDDLE_1, &_BaseMat, _pSkelInstance);
		_pSkel->EvalNode(PLAYER_ROTTRACK_LHAND_RING_1, &_BaseMat, _pSkelInstance);
		_pSkel->EvalNode(PLAYER_ROTTRACK_LHAND_PINKY_1, &_BaseMat, _pSkelInstance);
	}
	else if(m_iIKMode == IK_MODE_RIGHT_HAND)
	{
		// Right hand
		_pSkel->EvalNode(PLAYER_ROTTRACK_RHAND_THUMB_1, &_BaseMat, _pSkelInstance);
		_pSkel->EvalNode(PLAYER_ROTTRACK_RHAND_INDEX_1, &_BaseMat, _pSkelInstance);
		_pSkel->EvalNode(PLAYER_ROTTRACK_RHAND_MIDDLE_1, &_BaseMat, _pSkelInstance);
		_pSkel->EvalNode(PLAYER_ROTTRACK_RHAND_RING_1, &_BaseMat, _pSkelInstance);
		_pSkel->EvalNode(PLAYER_ROTTRACK_RHAND_PINKY_1, &_BaseMat, _pSkelInstance);
	}
}


void CIKSystem::DoDualHandIK(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, const CMat4Dfp32 *_pTarget, CWorld_PhysState* _pWPhysState, CWO_Character_ClientData *_pCD, 
							 const CVec3Dfp32 *_pDesiredPointDir, const CMat4Dfp32 *_pGrabRailIfNoReach)
{
	//--------------------------------------------------------------------------------------------
	// Simple two joint IK-implementation of the arms (applicable to legs as well)
	// 
	int iParentNode;
	int HandAttachROTTRACK = 0;
	int HandROTTRACK;
	int ElbowROTTRACK;
	int ArmROTTRACK;

	
	switch(m_iIKMode) 
	{
	case IK_MODE_LEFT_HAND:
		HandAttachROTTRACK  = PLAYER_ROTTRACK_LHANDATTACH;
		HandROTTRACK		= PLAYER_ROTTRACK_LHAND;
		ElbowROTTRACK		= PLAYER_ROTTRACK_LELBOW;
		ArmROTTRACK			= PLAYER_ROTTRACK_LARM;
		break;
		
	case IK_MODE_RIGHT_HAND:
		HandAttachROTTRACK  = PLAYER_ROTTRACK_RHANDATTACH;
		HandROTTRACK		= PLAYER_ROTTRACK_RHAND;
		ElbowROTTRACK		= PLAYER_ROTTRACK_RELBOW;
		ArmROTTRACK			= PLAYER_ROTTRACK_RARM;
		break;

	case IK_MODE_LEFT_FOOT:
		
		//HandAttachROTTRACK  = PLAYER_ROTTRACK_RHANDATTACH;
		HandROTTRACK		= PLAYER_ROTTRACK_LFOOT;
		ElbowROTTRACK		= PLAYER_ROTTRACK_LKNEE;
		ArmROTTRACK			= PLAYER_ROTTRACK_LLEG;
		break;

	case IK_MODE_RIGHT_FOOT:

		//HandAttachROTTRACK  = PLAYER_ROTTRACK_RHANDATTACH;
		HandROTTRACK		= PLAYER_ROTTRACK_RFOOT;
		ElbowROTTRACK		= PLAYER_ROTTRACK_RKNEE;
		ArmROTTRACK			= PLAYER_ROTTRACK_RLEG;
		break;

	//---------------------------------------------------------------------------
	// And for the darkling
	case IK_MODE_DARKLING_RIGHT_BACK_FOOT:
		HandROTTRACK		= PLAYER_ROTTRACK_RFOOT;
		ElbowROTTRACK		= DARKLING_ROTTRACK_RIGHT_LOWLEG;
		ArmROTTRACK			= DARKLING_ROTTRACK_RIGHT_MIDLEG;
		break;

	case IK_MODE_DARKLING_LEFT_BACK_FOOT:
		HandROTTRACK		= PLAYER_ROTTRACK_LFOOT;
		ElbowROTTRACK		= DARKLING_ROTTRACK_LEFT_LOWLEG;
		ArmROTTRACK			= DARKLING_ROTTRACK_LEFT_MIDLEG;
		break;

	case IK_MODE_DARKLING_RIGHT_FRONT_FOOT:
		HandROTTRACK		= PLAYER_ROTTRACK_RHAND;
		ElbowROTTRACK		= PLAYER_ROTTRACK_RELBOW;
		ArmROTTRACK			= PLAYER_ROTTRACK_RARM;
		break;

	case IK_MODE_DARKLING_LEFT_FRONT_FOOT:
		HandROTTRACK		= PLAYER_ROTTRACK_LHAND;
		ElbowROTTRACK		= PLAYER_ROTTRACK_LELBOW;
		ArmROTTRACK			= PLAYER_ROTTRACK_LARM;
		break;

	default:
		return;

	}

	// check if already there, then skip the whole thing
	iParentNode = _pSkel->m_lNodes[HandROTTRACK].m_iNodeParent;
	CVec3Dfp32 HandPointWrld2 = _pSkel->m_lNodes[HandROTTRACK].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];
	
	// this is pretty much done in the postanim system, and this failsafe could be scrapped
	//if((_pTarget->GetRow(3) - HandPointWrld2).LengthSqr() < 0.0001f)
	//	return;

	CVec3Dfp32 TargetPointWrld = _pTarget->GetRow(3);

	// helpers
	CMat4Dfp32 TmpMat;
	CVec3Dfp32 TmpVec;
	CMat4Dfp32 InvMat;
	CMat43fp32 NewHandMatWorld;

	//--------------------------------------------------------------------------------------------
	// 1. setup NewHandMatWorld and the newHandMatLocal matrix
	// 2. from that, get the attach point, and transform it to world space
	// 3. the diff vector is that point, to the target point
	// 4. adjust the target point with the diff vector

	// the target matrix for this hand, is the  HandTarget matrix

	NewHandMatWorld.CreateFrom(*_pTarget);

	CMat4Dfp32 NewHandMatLocalTmp;	
	iParentNode = _pSkel->m_lNodes[HandROTTRACK].m_iNodeParent;
	TmpMat = _pSkelInstance->m_pBoneTransform[iParentNode];
	TmpMat.GetRow(3) = 0.0f; //ignore translation

	TmpMat.InverseOrthogonal(InvMat);
	NewHandMatWorld.Multiply(InvMat, NewHandMatLocalTmp);
	CVec3Dfp32::GetRow(NewHandMatLocalTmp, 3) = 0.0f; //ignore translation

	_pSkelInstance->m_pBoneLocalPos[HandROTTRACK] = NewHandMatLocalTmp;
	
	iParentNode = _pSkel->m_lNodes[HandROTTRACK].m_iNodeParent;
	// instead of pSkel->EvalNode(HandROTTRACK, &pSkelInstance->m_pBoneTransform[iParentNode], pSkelInstance);
	TmpMat = _pSkelInstance->m_pBoneLocalPos[HandROTTRACK];
	TmpVec = _pSkel->m_lNodes[HandROTTRACK].m_LocalCenter;
	TmpVec *= TmpMat;
	TmpVec -= _pSkel->m_lNodes[HandROTTRACK].m_LocalCenter;
	CVec3Dfp32::GetMatrixRow(TmpMat, 3) = -TmpVec;

	TmpMat.Multiply(_pSkelInstance->m_pBoneTransform[iParentNode], _pSkelInstance->m_pBoneTransform[HandROTTRACK]);

	// now the hand is rotated and even though the wrist position is fubar, 
	// the worldVector from attach-point to wrist will still be the correct

	if(m_iIKMode == IK_MODE_LEFT_HAND || m_iIKMode == IK_MODE_RIGHT_HAND)
	{
		iParentNode = _pSkel->m_lNodes[HandAttachROTTRACK].m_iNodeParent;
		CVec3Dfp32 AttachPointWorldTemp = _pSkel->m_lNodes[HandAttachROTTRACK].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

		iParentNode = _pSkel->m_lNodes[HandROTTRACK].m_iNodeParent;
		CVec3Dfp32 WristPointWorldTemp = _pSkel->m_lNodes[HandROTTRACK].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

		TargetPointWrld += (WristPointWorldTemp - AttachPointWorldTemp);	
	}

	//--------------------------------------------------------------------------------------------
	// 1. get the world positions of interesting points
	// 2. create a plane of arm, elbow and target that will be used for rotations

	iParentNode = _pSkel->m_lNodes[HandROTTRACK].m_iNodeParent;
	CVec3Dfp32 HandPointWrld = _pSkel->m_lNodes[HandROTTRACK].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

	iParentNode = _pSkel->m_lNodes[ElbowROTTRACK].m_iNodeParent;
	CVec3Dfp32 ElbowOrgPointWrld = _pSkel->m_lNodes[ElbowROTTRACK].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

	iParentNode = _pSkel->m_lNodes[ArmROTTRACK].m_iNodeParent;
	CVec3Dfp32 ArmPointWrld = _pSkel->m_lNodes[ArmROTTRACK].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

	//----------------------------------------------------------------------
	// skeleton is not scaled at this point

	if(_pCD->m_CharSkeletonScale != 0.0f)
	{
		iParentNode = _pSkel->m_lNodes[_pSkel->m_lNodes[ArmROTTRACK].m_iNodeParent].m_iNodeParent;
		CVec3Dfp32 ShoulderPointWrld = _pSkel->m_lNodes[_pSkel->m_lNodes[ArmROTTRACK].m_iNodeParent].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];
		ArmPointWrld = ShoulderPointWrld + (ArmPointWrld - ShoulderPointWrld) * (1.0f - _pCD->m_CharSkeletonScale);

		ElbowOrgPointWrld = ArmPointWrld + (ElbowOrgPointWrld - ArmPointWrld) * (1.0f - _pCD->m_CharSkeletonScale); 
		HandPointWrld = ElbowOrgPointWrld + (HandPointWrld - ElbowOrgPointWrld) * (1.0f - _pCD->m_CharSkeletonScale);
	}
	
	//----------------------------------------------------------------------

	// find the lengths of the bones
	CVec3Dfp32 ArmToelbow  = (ElbowOrgPointWrld - ArmPointWrld);
	CVec3Dfp32 ElbowToHand = (HandPointWrld - ElbowOrgPointWrld);

	fp32 Bone1Length = ArmToelbow.Length();	// find length of overarm
	fp32 Bone2Length = ElbowToHand.Length();	// find length of underarm

	// check distance from arm to target
	CVec3Dfp32 ArmToTarget = (TargetPointWrld - ArmPointWrld);
	fp32 ArmToTargetLength = ArmToTarget.Length();

	bool bCanReach = true;
	if((Bone1Length + Bone2Length) <=  ArmToTargetLength)
	{
		bCanReach = false;
		/*
		// if the targetpoint is out of reach and a grab-rail is supplied, try to grab the rail with a straight arm
		if(_pGrabRailIfNoReach)
		{
			// restart the whole damn thing with a new pTarget
			CMat4Dfp32 NewTarget;
			NewTarget.CreateFrom(*_pTarget);
			_pWPhysState->Debug_RenderMatrix(NewTarget, false);
			
			//ClosestPoint = P1 + ((P2 - P1) * (Point - P1)) * (P2 - P1) / ((P2 - P1) * (P2 - P1))
			
			//CVec3Dfp32 NearestPointonLine; 
			//CVec3Dfp32 P1 = _pGrabRailIfNoReach->GetRow(3)- (_pGrabRailIfNoReach->GetRow(0) * 10.0f);
			//CVec3Dfp32 P2 = _pGrabRailIfNoReach->GetRow(3) + (_pGrabRailIfNoReach->GetRow(0) * 20.0f);
			//CVec3Dfp32 Point = ArmPointWrld;
			//NearestPointonLine = P1 + ((P2 - P1) * (Point - P1)) * (P2 - P1) / ((P2 - P1) * (P2 - P1));
			

			// ClosestPoint = PointOnLine + ((Point - PointOnLIne) * LineDir) * LineDir;
			CVec3Dfp32 NearestPointonLine; 
			CVec3Dfp32 PointOnLine = _pGrabRailIfNoReach->GetRow(3);
			CVec3Dfp32 Point = ArmPointWrld;
			NearestPointonLine = PointOnLine + (_pGrabRailIfNoReach->GetRow(0) * ((Point - PointOnLine) * _pGrabRailIfNoReach->GetRow(0)));
			
			//phytagoras a² + b² = c²
			// b = Sqrt(c² - a²)
			fp32 ADist = (Point - NearestPointonLine).Length();
			fp32 CDist = (Bone1Length + Bone2Length);
			fp32 BDist = M_Sqrt((CDist*CDist) - (ADist*ADist));

			NewTarget.GetRow(3) = _pGrabRailIfNoReach->GetRow(3)- (_pGrabRailIfNoReach->GetRow(0) * -BDist);

			
			_pWPhysState->Debug_RenderMatrix(NewTarget, false);
			_pWPhysState->Debug_RenderWire(_pGrabRailIfNoReach->GetRow(3), _pGrabRailIfNoReach->GetRow(3) + (_pGrabRailIfNoReach->GetRow(0) * 10.0f));

			DoDualHandIK(_pSkel, _pSkelInstance, &NewTarget, _pWPhysState, _pCD, _pDesiredPointDir);
			return;
		}
		*/
	}

	// get the (arm, elbow, target)-plane. 
	CMat4Dfp32 CalcPlane;
	CalcPlane.GetRow(0) = ArmToTarget.Normalize();

	// this is where the "desired elbow point" comes into the picture
	CVec3Dfp32 DesiredPoint;
	
	
	// the direction the elbow should point. works normally, but in some cases this needs to be set
	if(_pDesiredPointDir)
	{
		if(m_iIKMode == IK_MODE_LEFT_HAND || m_iIKMode == IK_MODE_RIGHT_HAND)
			DesiredPoint = (ElbowOrgPointWrld - HandPointWrld) + TargetPointWrld + *_pDesiredPointDir;
		else if(m_iIKMode == IK_MODE_LEFT_FOOT || m_iIKMode == IK_MODE_RIGHT_FOOT)
			DesiredPoint = _pTarget->GetRow(0) + TargetPointWrld + *_pDesiredPointDir; // desired point could be gotten by foot "at"
		else
			DesiredPoint = ElbowOrgPointWrld + *_pDesiredPointDir;
	}
	else
	{
		if(m_iIKMode == IK_MODE_LEFT_HAND || m_iIKMode == IK_MODE_RIGHT_HAND)
			DesiredPoint = (ElbowOrgPointWrld - HandPointWrld) + TargetPointWrld;
		else if(m_iIKMode == IK_MODE_LEFT_FOOT || m_iIKMode == IK_MODE_RIGHT_FOOT)
			DesiredPoint = _pTarget->GetRow(0) + TargetPointWrld; // desired point could be gotten by foot "at"
		else
			DesiredPoint = ElbowOrgPointWrld;
	}

	CVec3Dfp32 uptest(0.0f, 0.0f, -10.0f);
	ArmToelbow = (DesiredPoint - ArmPointWrld);

	CalcPlane.GetRow(1) = ArmToelbow.Normalize();
	CalcPlane.GetRow(0).CrossProd(CalcPlane.GetRow(1), CalcPlane.GetRow(2));
	CalcPlane.GetRow(2) = CalcPlane.GetRow(2).Normalize();
	CalcPlane.GetRow(0).CrossProd(CalcPlane.GetRow(2), CalcPlane.GetRow(1)); 
	CalcPlane.GetRow(3) = 0.0f;

	//--------------------------------------------------------------------------------------------
	// find the elbow target point
	CVec3Dfp32 ElbowTarget;

	// is the point reachable? reach arm straight towards point
	if(!bCanReach)
	{
		// special case. reach arm straight towards point
		TmpVec = (TargetPointWrld - ArmPointWrld);
		TmpVec.Normalize();
		TmpVec = TmpVec * Bone1Length;
		ElbowTarget = (ArmPointWrld + TmpVec);
	}
	else // point is within reach
	{
		// if rotating the arm in the plane, a circle is drawn
		// with the diameter of bone1Length, and center of armPointWrld

		// the same goes if you rotate an imaginary arm 
		// with the diameter of bone2Length, and center of TargetPointWrld

		// where the circles cross is the point where the elbow will be
		// there are two sollutions, but the correct will automagically be the second

		// Get the two circles intersection points
		CVec3Dfp32 ElbowTarget_alt2;
		CVec2Dfp32 ElbowTarget2D_alt2;

		// the first cirlce center is in (0,0), and the second in (armToTargetLength, 0)
		GetCircleCircleIntersectionPoints(Bone1Length, ArmToTargetLength, Bone2Length, NULL, &ElbowTarget2D_alt2);
		//GetCircleCircleIntersectionPoints(Bone1Length, ArmToTargetLength, Bone2Length, &ElbowTarget2D_alt2, NULL);

		TmpVec = ((CalcPlane.GetRow(0) * ElbowTarget2D_alt2.k[0]) + (CalcPlane.GetRow(1) * ElbowTarget2D_alt2.k[1]));
		ElbowTarget_alt2 = ArmPointWrld + TmpVec;

		ElbowTarget = ElbowTarget_alt2; // cut down code since this is always the correct one
	}

	//--------------------------------------------------------------------------------------------
	// setup the new matrices, aligned with the bones
	
	// first fix the arm
	SetupAndAdjustMatrix(ArmROTTRACK, ElbowROTTRACK, &ArmPointWrld, &ElbowTarget, _pSkel, _pSkelInstance, _pWPhysState);
	iParentNode = _pSkel->m_lNodes[ArmROTTRACK].m_iNodeParent;
	_pSkel->EvalNode_IK_Special(ArmROTTRACK, &_pSkelInstance->m_pBoneTransform[iParentNode], _pSkelInstance, 1);

	// then the elbow
	SetupAndAdjustMatrix(ElbowROTTRACK, HandROTTRACK, &ElbowTarget, &TargetPointWrld,  _pSkel, _pSkelInstance, _pWPhysState);
	iParentNode = _pSkel->m_lNodes[ElbowROTTRACK].m_iNodeParent;
	_pSkel->EvalNode_IK_Special(ElbowROTTRACK, &_pSkelInstance->m_pBoneTransform[iParentNode], _pSkelInstance, 0);

	// and lastly the hand (we already have the hand matrix from the target matrix)
	CMat4Dfp32 NewHandMatLocal;	
	iParentNode = _pSkel->m_lNodes[HandROTTRACK].m_iNodeParent;
	TmpMat = _pSkelInstance->m_pBoneTransform[iParentNode];	

	TmpMat.GetRow(3) = 0.0f; //ignore translation

	TmpMat.InverseOrthogonal(InvMat);
	NewHandMatWorld.Multiply(InvMat, NewHandMatLocal);
	NewHandMatLocal.GetRow(3) = 0.0f; //ignore translation

	_pSkelInstance->m_pBoneLocalPos[HandROTTRACK] = NewHandMatLocal;
	iParentNode = _pSkel->m_lNodes[HandROTTRACK].m_iNodeParent;

 	_pSkel->EvalNode(HandROTTRACK, &_pSkelInstance->m_pBoneTransform[iParentNode], _pSkelInstance); // this one cant be avoidede

}

void CIKSystem::GetCircleCircleIntersectionPoints(fp32 _C1Radius, fp32 _C2XPos, fp32 _C2Radius, CVec2Dfp32 *_pResult1, CVec2Dfp32 *_pResult2)
{
	CVec2Dfp32 _C1Pos(0.0f, 0.0f);
	CVec2Dfp32 _C2Pos(_C2XPos, 0.0f);

	// there will be only one X result since we've 0 aligned positions
	fp32 ResX;
	fp32 ResY; 

	// (d^2 - r^2 + R^2) / 2*d
	ResX = ( ( (_C2Pos.k[0] * _C2Pos.k[0]) - (_C2Radius * _C2Radius) + (_C1Radius * _C1Radius) ) / (2.0f * _C2Pos.k[0]) );
	// y^2 = R^2 - x^2
	fp32 CheckVal = (_C1Radius * _C1Radius) - (ResX * ResX);
	if(CheckVal <= 0.0f)
	{
		ResX = ( ( (_C2Pos.k[0] * _C2Pos.k[0]) - (_C1Radius * _C1Radius) + (_C1Radius * _C1Radius) ) / (2.0f * _C2Pos.k[0]) );
		CheckVal = (_C1Radius * _C1Radius) - (ResX * ResX);
	}

	ResY = M_Sqrt(CheckVal);

	if(_pResult1)
	{
		_pResult1->k[0] = ResX;
		_pResult1->k[1] = ResY;
	}
	
	if(_pResult2)
	{
		_pResult2->k[0] = ResX;
		_pResult2->k[1] = -ResY;
	}
}

void CIKSystem::SetupAndAdjustMatrix(int8 _iJoint, int8 _iChildJoint, const CVec3Dfp32* _pJointPosWorld, const CVec3Dfp32* _pChildJointPosWorld, 
									CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState)
{	
	CVec3Dfp32 RotAxis;
	CVec3Dfp32 JointToChildTarget = (*_pChildJointPosWorld - *_pJointPosWorld).Normalize();
	CVec3Dfp32 JointToChildOrig = ((_pSkel->m_lNodes[_iChildJoint].m_LocalCenter * _pSkelInstance->m_pBoneTransform[_iJoint]) - *_pJointPosWorld).Normalize();
	
	JointToChildOrig.CrossProd(JointToChildTarget, RotAxis);
	if (RotAxis.LengthSqr() > 1e-5f)
		RotAxis.Normalize();
	else
	        RotAxis.k[0] = 1.0f;

	fp32 CosVal = JointToChildTarget * JointToChildOrig;
	if(FloatIsNAN(CosVal))
		return;

	fp32	fAngle = M_ACos(JointToChildTarget * JointToChildOrig);
	
	if(FloatIsNAN(fAngle))
		fAngle = 0.0f;

	fAngle *= (1.0f / _PI2); // convert to 0->1 angle
	
	CMat4Dfp32 MatPlaneRotation;
	CQuatfp32(RotAxis, -fAngle).CreateMatrix3x3(MatPlaneRotation);
	MatPlaneRotation.GetRow(3) = 0.0f;

	CMat4Dfp32 TmpMat = _pSkelInstance->m_pBoneTransform[_iJoint];
	TmpMat.GetRow(3) = 0.0f;

	CMat4Dfp32 ResultRot;
	TmpMat.Multiply3x3(MatPlaneRotation, ResultRot);
	ResultRot.GetRow(3) = 0.0f;

	// ResultRot to localspace
	int iParentNode = _pSkel->m_lNodes[_iJoint].m_iNodeParent;
	TmpMat = _pSkelInstance->m_pBoneTransform[iParentNode];
	TmpMat.GetRow(3) = 0.0f; //ignore translation

	CMat4Dfp32 ParentInvMat;
	TmpMat.InverseOrthogonal(ParentInvMat);
	ParentInvMat.GetRow(3) = 0.0f;

	CMat4Dfp32 ResultRotLocal;
	ResultRot.Multiply3x3(ParentInvMat, ResultRotLocal);
	ResultRotLocal.GetRow(3) = 0.0f;

	CVec3Dfp32 StorePos = _pSkelInstance->m_pBoneLocalPos[_iJoint].GetRow(3);
	_pSkelInstance->m_pBoneLocalPos[_iJoint] = ResultRotLocal;
	_pSkelInstance->m_pBoneLocalPos[_iJoint].GetRow(3) = StorePos;
	  	
	return;
}
