/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Camera handling for Darklings.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		Camera handling for Darklings.

	Comments:

	History:		
		050824		Created file, moved code from WObj_CharDarkling_ClientData.cpp
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharDarkling_ClientData.h"

#define SafeTrace(v) \
{ \
	bool bHit; \
	CCollisionInfo CInfo; \
	CVec3Dfp32 SafeVector = v; \
	bHit = m_pWPhysState->Phys_IntersectLine(CameraPos, CameraPos + SafeVector * SoftBoundRadius, 0, objects, mediums, &CInfo, m_pObj->m_iObject); \
	if (bHit && CInfo.m_bIsValid) \
	{ \
		fp32 DirWeight = (CInfo.m_Plane.n * -SafeVector); \
		fp32 Distance = CInfo.m_Time * SoftBoundRadius; \
		fp32 SoftFraction = Max(0.0f, (Distance - HardBoundRadius) / (SoftBoundRadius - HardBoundRadius)); \
		fp32 DistWeight = ((CInfo.m_Time - 1.0f) * LERP(HardBoundRadius, 0.0f, SoftFraction)); \
		Adjustment += SafeVector * DistWeight * DirWeight; \
	} \
	if (DebugRender) m_pWPhysState->Debug_RenderWire(CameraPos, CameraPos + SafeVector * SoftBoundRadius, 0xF0FF0000, DebugDuration); \
}



void CWO_CharDarkling_ClientData::Camera_Offset(CMat4Dfp32& _Camera, fp32 _IPTime)
{
	MSCOPE(Camera_ThirdPerson_Offset, CHARACTER);

	bool DebugRender = false;
	fp32 DebugDuration = 90.0f;

	CVec3Dfp32 Head = CVec3Dfp32::GetMatrixRow(_Camera, 3);
	CVec3Dfp32 Behind = CVec3Dfp32(-100.0f,0,20.0f);//LERP(m_Camera_LastBehindOffset, m_Camera_CurBehindOffset, _IPTime);

	Behind.MultiplyMatrix3x3(_Camera);

	int objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;// | OBJECT_FLAGS_PHYSOBJECT; // Exclude characters
	int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_CAMERASOLID | XW_MEDIUM_DYNAMICSSOLID;// | XW_MEDIUM_PLAYERSOLID;

	bool bHit;
	CCollisionInfo CInfo;

	fp32 Distance = 1.0f;//LERP(m_LastChaseCamDistance, m_ChaseCamDistance, _IPTime);

	CVec3Dfp32 NormalOffset = 0.0f;

	if (DebugRender) m_pWPhysState->Debug_RenderVertex(Head + Behind * Distance, 0xF000FF00, 50.0f);

	bHit = m_pWPhysState->Phys_IntersectLine(Head, Head + Behind * Distance, 0, objects, mediums, &CInfo, m_pObj->m_iObject);
	if (bHit && CInfo.m_bIsValid)
		Distance = Min(CInfo.m_Time * Distance, Distance);


	// Shrink distance with 5% (can't remember exactly why, but it gets a bit away of any walls).
	Behind *= Distance * 0.95f;

	fp32 BehindDistance = Behind.Length();

	if (DebugRender) m_pWPhysState->Debug_RenderVertex(Head, 0xF0FF00FF, DebugDuration);
	if (DebugRender) m_pWPhysState->Debug_RenderWire(Head, Head + Behind, 0xF00000FF, DebugDuration);

	//Include characters in cmaera FOV check
	objects |= OBJECT_FLAGS_PHYSOBJECT; 

	CVec3Dfp32 Adjustment = 0.0f;

	{ // Bound Check Camera FOV
		CVec3Dfp32 CameraPos = Head + Behind;

		const CVec3Dfp32& Fwd = _Camera.GetRow(0);
		const CVec3Dfp32& Side = _Camera.GetRow(1);
		const CVec3Dfp32& Up = _Camera.GetRow(2);

		fp32 HardBoundRadius = 5.0f;
		fp32 SoftBoundRadius = 8.0f;

		CVec3Dfp32 SafeVector;
		SafeTrace(Side + Fwd);
		SafeTrace(-Side + Fwd);
		SafeTrace(Up + Fwd);
		SafeTrace(-Up + Fwd);

		Adjustment *= 1.0f; // Scale down, since there may often be several safevectors hitting the same wall.

		if (DebugRender) m_pWPhysState->Debug_RenderWire(Head + Behind, Head + Behind + Adjustment, 0xF0F0F000, DebugDuration);
		if(Adjustment.Length() > 0)
			Behind += Adjustment;
	}

	if (DebugRender) m_pWPhysState->Debug_RenderWire(Head, Head + Behind, 0xF000FF00, DebugDuration);

	// Make sure bound adjustment didn't invalidate camera.
	objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;// | OBJECT_FLAGS_PHYSOBJECT; // Exclude characters
	bHit = m_pWPhysState->Phys_IntersectLine(Head, Head + Behind, 0, objects, mediums, &CInfo, m_pObj->m_iObject);
	if (bHit && CInfo.m_bIsValid)
		Behind *= CInfo.m_Time;

	Behind.AddMatrixRow(_Camera, 3);

	CVec3Dfp32 Fwd = _Camera.GetRow(0);

	if (BehindDistance > 0)
	{
		Fwd -= Adjustment / BehindDistance;
		Fwd.Normalize();
		_Camera.GetRow(0) = Fwd;
		_Camera.RecreateMatrix(0, 2);
	}
}


void CWO_CharDarkling_ClientData::GetCamera(CMat4Dfp32& _Result, fp32 _IPTime, CMat4Dfp32* _pResultWorld)
{
	const CMat4Dfp32& PosMat = m_pObj->GetPositionMatrix();
	CMat4Dfp32 Camera;
	_IPTime = m_PredictFrameFrac;

	bool bThirdPerson = (m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
	if (bThirdPerson)
	{ 
		Camera = PosMat;
//		Camera.GetRow(3) += Camera.GetRow(2) * 1.0f;
//		Camera_Offset(Camera, _IPTime);

		if(m_pObj)
		{
			CXR_Model* pModel = m_pWPhysState->GetMapData()->GetResource_Model(m_pObj->m_iModel[0]);
			if(pModel)
			{
				CXR_Skeleton *pSkel = pModel ? pModel->GetSkeleton() : NULL;
				if(pSkel)
				{
					CXR_AnimState Anim;
					if (!CWObject_Character::OnGetAnimState(m_pObj, m_pWPhysState, pSkel, 0, Camera, _IPTime, Anim, NULL))
						return;

					CXR_SkeletonInstance* pSkelInst = Anim.m_pSkeletonInst;
					if(pSkelInst)
					{
						int iTrack = 11;
						CVec3Dfp32 x = pSkel->m_lNodes[iTrack].m_LocalCenter;

//						x.k[0] += 10.0f;

/* 			x.k[0] -= 15.0f;
			x.k[1] -= 7.0f;
			x.k[2] += 10.0f;

			CVec3Dfp32 CamRightPos, CamLeftPos;
			CamLeftPos = CamRightPos = x;
			CamLeftPos.k[1] += 14.0f;

			switch(m_CameraStatus) 
			{
			case DARKLING_CAMERA_LEFT:	
				{
					x = CamLeftPos;
					break;	
				}
			case DARKLING_CAMERA_RIGHT:
				{
					x = CamRightPos;
					break;
				}
			};

			fp32 p = 0.0f;
			switch(m_CameraStatus) 
			{
			case DARKLING_CAMERA_LEFT_TO_RIGHT:	
				{
					p = (m_pWPhysState->GetGameTick() - m_CameraStartTick) / (DARKLING_CAMERA_SWITCH_TIME * m_pWPhysState->GetGameTicksPerSecond());
					if(p > 1.0f) 
					{
						p = 1.0f;
					}
					CVec3Dfp32 left_to_right;
					left_to_right = CamRightPos - CamLeftPos;
					x = CamLeftPos + left_to_right * p;
					break;
				}
			case DARKLING_CAMERA_RIGHT:	
				{
					x = CamRightPos;
					break;
				}
			case DARKLING_CAMERA_LEFT:
				{
					x = CamLeftPos;
					break;	
				}
			case DARKLING_CAMERA_RIGHT_TO_LEFT:	
				{
					p = (m_pWPhysState->GetGameTick() - m_CameraStartTick) / (DARKLING_CAMERA_SWITCH_TIME * m_pWPhysState->GetGameTicksPerSecond());
					if(p > 1.0f) 
					{
						p = 1.0f;
					}
					CVec3Dfp32 right_to_left;
					right_to_left = CamLeftPos - CamRightPos;
					x = CamRightPos + right_to_left * p;
					break;
				}
			}
*/
						x *= pSkelInst->m_pBoneTransform[iTrack];

						Camera.GetRow(3) = x;
					}
//					m_pWPhysState->Debug_RenderMatrix(Camera, 0.002f, false);
				}
			}
		}
		// Transform world space -> camera space
		_Result.GetRow(2) =  Camera.GetRow(0);	//  X -> Z
		_Result.GetRow(0) = -Camera.GetRow(1);	// -Y -> X
		_Result.RecreateMatrix(2, 0);			// (Z -> Y)
		_Result.GetRow(3) =  Camera.GetRow(3);
	}
	else
	{
		// Create a camera
		Camera = PosMat;
		CWObject_Character::Camera_Get_FirstPerson(m_pWPhysState, &Camera, m_pObj, _IPTime);
		_Result = Camera;
	}

	if(_pResultWorld)
		*_pResultWorld = Camera;
}
