
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character camera 
					
	Author:			fredrik larsson
					
	Copyright:		Copy_right
					
	Contents:		OnRefresh_ServerPredicted_Camera
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		020219:		Created File
\*____________________________________________________________________________________________*/





#include "PCH.h"
#include "WObj_Char.h"
#include "WRPG/WRPGSpell.h"
#include "Models/CSinRandTable.h"
#include "WObj_Misc/WObj_ActionCutsceneCamera.h"
#include "WObj_Misc/WObj_CreepingDark.h"

//#define NEW_CAMERA

//static CSinRandTable<1024, 8> g_SinRandTable;

// Definitions used in Camera_Trace
#define MAX_ROTATIONS	5
#define ROTATION_ANGLE	0.05f
#define CAMTRACE_RINGS  4
#define CAMTRACE_SIDES  4

#define PLAYER_CAMERADISTANCECHANGE_DEAD 30

//#define	M_PI		3.14159265358979323846f	/* pi */


#define SafeTrace(v) \
{ \
	bool bHit; \
	CCollisionInfo CInfo; \
\
	CVec3Dfp32 SafeVector = v; \
\
	bHit = _pPhysState->Phys_IntersectLine(CameraPos, CameraPos + SafeVector * SoftBoundRadius, 0, objects, mediums, &CInfo, _pObj->m_iObject); \
	if (bHit && CInfo.m_bIsValid) \
	{ \
		fp32 DirWeight = (CInfo.m_Plane.n * -SafeVector); \
		fp32 Distance = CInfo.m_Time * SoftBoundRadius; \
		fp32 SoftFraction = Max(0.0f, (Distance - HardBoundRadius) / (SoftBoundRadius - HardBoundRadius)); \
		fp32 DistWeight = ((CInfo.m_Time - 1.0f) * LERP(HardBoundRadius, 0.0f, SoftFraction)); \
		Adjustment += SafeVector * DistWeight * DirWeight; \
	} \
\
	if (DebugRender) _pPhysState->Debug_RenderWire(CameraPos, CameraPos + SafeVector * SoftBoundRadius, 0xF0FF0000, DebugDuration); \
}


static void InterpolateCamera(const CMat4Dfp32& _FirstPersonCamera, const CMat4Dfp32& _ThirdPersonCamera, fp32 _Time, CMat4Dfp32& _Result)
{
	CVec3Dfp32 v0 = CVec3Dfp32::GetMatrixRow(_FirstPersonCamera, 3);
	CVec3Dfp32 v1 = v0 + CVec3Dfp32::GetRow(_FirstPersonCamera, 2) * -150; 
	CVec3Dfp32 v2 = CVec3Dfp32::GetMatrixRow(_ThirdPersonCamera, 3);

	// Interpolate rotation
	CQuatfp32 q0; q0.Create(_FirstPersonCamera);
	CQuatfp32 q3; q3.Create(_ThirdPersonCamera);
	CQuatfp32 qdest;
	q0.Lerp(q3, _Time, qdest);
	qdest.CreateMatrix(_Result);

	// Interpolate position
	CVec3Dfp32& vdest = CVec3Dfp32::GetMatrixRow(_Result, 3);
	CVec3Dfp32::Spline(&v0, &v0, &v1, &v1, &v2, &v2, &vdest, _Time, 0.5f, 1.0f, 1.0f, 1.0f, 1);
}



  
// =======================================================================
//
//	Function:			Calculates the camera matrix for aiming purposes
//						
//	Comments:			Longer_description_not_mandatory
//
// =======================================================================
void CWObject_Character::GetCameraAimMatrix(CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, CMat4Dfp32* _pMat, bool _bApplyBehindOffset)
{
	if (_pObj == NULL) return;

	*_pMat = _pObj->GetPositionMatrix();

	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if (pCD == NULL) return;

	CVec3Dfp32 HeadWS = Camera_GetHeadPos(_pObj);
	CVec3Dfp32 BehindCS = pCD->m_Camera_CurBehindOffset * pCD->m_ChaseCamDistance;
	CVec3Dfp32 BehindWS(BehindCS[2], -BehindCS[0], -BehindCS[1]); // Convert from CameraSpace rotation to WorldSpace rotation.

	if (Char_GetPhysType(_pObj) == PLAYER_PHYS_NOCLIP)
		return;

	BehindWS.MultiplyMatrix3x3(*_pMat);

	if (!_bApplyBehindOffset)
	{
		CVec3Dfp32 Fwd = CVec3Dfp32::GetMatrixRow(*_pMat, 0);
		BehindWS -= Fwd * (BehindWS * Fwd);
	}

	const int PhysType = Char_GetPhysType(_pObj);

	int CameraMode = GetCameraMode(pCD, PhysType);
	bool bFirstPerson = (CameraMode == PLAYER_CAMERAMODE_FIRSTPERSON);// || (pCD->m_FPCamera_Enable));
	if (bFirstPerson)
	{
/*
		fp32 Extrapolate = 0;
		CXR_AnimState AnimState;
		CWObject_Message Msg(OBJSYSMSG_GETANIMSTATE);
		Msg.m_pData = &AnimState;
		Msg.m_DataSize = sizeof(AnimState);
		Msg.m_Param0 = *(int *)&Extrapolate;
		if (!_pPhysState->Phys_Message_SendToObject(Msg, _pObj->m_iObject))
			return;

		if (!AnimState.m_pSkeletonInst)
			return;

		CXR_Model *pChar = _pPhysState->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
		if (!pChar)
			return;

		CXR_Skeleton *pSkelChar = (CXR_Skeleton*)pChar->GetParam(MODEL_PARAM_SKELETON);
		if (!pSkelChar)
			return;

		if (pSkelChar && pChar)
			GetAttachPos(_pObj, AnimState.m_pSkeletonInst, pSkelChar, PLAYER_ROTTRACK_HEAD, PLAYER_ATTACHPOINT_FACE, HeadWS);

*/
		CVec3Dfp32::GetMatrixRow(*_pMat, 3) += HeadWS;
	}
	else
	{
		CVec3Dfp32::GetMatrixRow(*_pMat, 3) += HeadWS + BehindWS;

/*
		// EXPIRED: SpecialRotation
		CVec3Dfp32 SpecialRotation = pCD->m_Camera_CurSpecialRotation;
		SpecialRotation *= PLAYER_SPECIALROTATION_RANGE;

		SpecialRotation *= pCD->m_Camera_CurSpecialBlend;

		_pMat->RotX_x_M(SpecialRotation.k[2]);
		_pMat->RotY_x_M(-SpecialRotation.k[0]);
		_pMat->RotZ_x_M(-SpecialRotation.k[1]);
*/
	}
}


// =======================================================================
//
//	Function:			Moderates
//						
//	Comments:			Longer_description_not_mandatory
//
// =======================================================================
/*
static void ModerateCP(int& x, int newx, int& xprim, int a)
{
	int xbiss = (int)((-fp32(a)*fp32(xprim) - fp32(x-newx)*Sqr(fp32(a))*0.25f) / 256.0f);
	xprim += xbiss;
	x += (xprim >> 8);
}
*/

CVec3Dfp32 RotateOffset(CVec3Dfp32 WantedBehindOffset, fp32 angle, int _rotdim);
CVec3Dfp32 RotateOffset(CVec3Dfp32 WantedBehindOffset, fp32 angle, int _rotdim)
{
	fp32 radangle = 2 * _PI * angle;
	
	fp32 sina = M_Sin(radangle);
	fp32 cosa = M_Cos(radangle);
	
	
	switch(_rotdim)
	{
	case 0: // Runt x
		return CVec3Dfp32(WantedBehindOffset[0],
			WantedBehindOffset[1]*cosa - WantedBehindOffset[2]*sina,
			WantedBehindOffset[1]*sina + WantedBehindOffset[2]*cosa);
	case 1: // Runt y
		return CVec3Dfp32(WantedBehindOffset[2]*sina + WantedBehindOffset[0]*cosa,
			WantedBehindOffset[1],
			WantedBehindOffset[2]*cosa - WantedBehindOffset[0]*sina);
	case 2:
	default:  // Runt z
		return CVec3Dfp32(WantedBehindOffset[0]*cosa - WantedBehindOffset[1]*sina,
			WantedBehindOffset[0]*sina + WantedBehindOffset[1]*cosa,
			WantedBehindOffset[2]);
	}
}

// =======================================================================
//
//	Function:			Refreshes the camera
//						
//	Comments:			This is where all the data to determine the
//						position of the camera is collected. The data 
//						is later used in GetCamera. This is called from 
//						OnRefresh_ServerPredicted
//
// NOTE:				Run on server and on predicted players.
// =======================================================================
void CWObject_Character::Camera_OnRefresh(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	CWO_Character_ClientData *pCD= GetClientData(_pObj);
	if (!pCD) return;
	
	MSCOPE(RefreshCamera, CHARACTER);
	
	// Adjust to Widescreen
//	fp32 WidescreenOffset = GetScreenAspect(); 
	
	//pCD->m_DefaultCutsceneLaps = 5;
	
	// Get cameramode
	const int PhysType = Char_GetPhysType(_pObj);
	int CameraMode = GetCameraMode(pCD, PhysType);

	bool bRangedWeapon = (pCD->m_Item0_Flags & RPG_ITEM_FLAGS_AUTOAIM) || (pCD->m_Item0_Flags & RPG_ITEM_FLAGS_AIMING);

	// ----------------------------------------------
	// Wanted settings
	CVec3Dfp32	WantedHeadOffset;
	CVec3Dfp32	WantedBehindOffset;
//	fp32			WantedSpecialBlend;
//	CVec3Dfp32   WantedSpecialRotation;
	fp32			WantedWallTiltBlend;
	fp32			WantedWallTiltAngle;
//	fp32			WantedDeadLook;
	
	WantedWallTiltAngle		= -0.2f;
	WantedWallTiltBlend		= 0.f;
//	WantedSpecialRotation	= pCD->m_Camera_SpecialRotation;
	WantedHeadOffset		= (Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH) ? (CVec3Dfp32)pCD->m_Camera_CrouchHeadOffset : (CVec3Dfp32)pCD->m_Camera_StandHeadOffset;
	
	switch (CameraMode)
	{
	case PLAYER_CAMERAMODE_FIRSTPERSON: 
		WantedBehindOffset = 0;
		//		WantedSpecialBlend = 0.0f;
		break;
		
	case PLAYER_CAMERAMODE_NEAR: 
		WantedBehindOffset = (CVec3Dfp32)pCD->m_Camera_ShoulderOffset;
		//		WantedSpecialBlend = 0.0f;
		break;
		
	case PLAYER_CAMERAMODE_NORMAL:
		WantedBehindOffset = (CVec3Dfp32)pCD->m_Camera_BehindOffset;
		//		WantedSpecialBlend = 1.0f;
		break;
		
	case PLAYER_CAMERAMODE_FAR: 
		WantedBehindOffset = (CVec3Dfp32)pCD->m_Camera_BehindOffset * PLAYER_CAMERAMODE_FAR_BEHINDSCALE; 
		//		WantedSpecialBlend = 1.0f;
		break;
	}
	
	if(Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
	{
		pCD->m_Camera_Rotation += 0.0015f;
		WantedBehindOffset = RotateOffset(WantedBehindOffset, pCD->m_Camera_Rotation, 1);
	}

/*
	// EXPIRED: Special Rotation
	if (AssistCamera(_pObj))
	{
		fp32 Blend = 1.0f;
		WantedBehindOffset += (CVec3Dfp32)pCD->m_Camera_SpecialOffset * WantedSpecialBlend * Blend;
	}
*/
	
	// Adjust to Widescreen
//	WantedBehindOffset[2] *=  WidescreenOffset;
	
	
	// ----------------------------------------------
	// Camera base
	CMat4Dfp32 CameraMatrix = _pObj->GetPositionMatrix();

	CameraMatrix.RotX_x_M(-0.25f);
	CameraMatrix.RotY_x_M(0.25f);
	Camera_GetHeadPos(_pObj).AddMatrixRow(CameraMatrix, 3);
	pCD->m_Cam_BobMove.AddMatrixRow(CameraMatrix, 3);
	
	// ----------------------------------------------
	// Trace to see if we need to zoom in on the char
	fp32 WantedDistance, MaxDistance;
	
	if (CameraMode != PLAYER_CAMERAMODE_FIRSTPERSON)
		Camera_Trace(_pObj, _pWPhysState, bRangedWeapon, CameraMatrix, WantedBehindOffset, WantedDistance, MaxDistance);
	else
		WantedDistance = 0;
	
/*		
	// EXPIRED: Special Rotation
	// We need to zoom in!
	if( WantedDistance < 1.0f)
	{
		if (CameraMode == PLAYER_CAMERAMODE_NORMAL || CameraMode == PLAYER_CAMERAMODE_FAR)
			WantedSpecialBlend = Max(0.0f, (WantedDistance - 0.2f) / 0.8f);

		if (AssistCamera(_pObj))
		{
			fp32 Blend = 1.0f;
			WantedBehindOffset += (CVec3Dfp32)pCD->m_Camera_SpecialOffset * WantedSpecialBlend * Blend;
		}

		WantedBehindOffset	= LERP(CVec3Dfp32(0,0,0), WantedBehindOffset, WantedSpecialBlend); //face	
		WantedDistance		= LERP(1.0f, WantedDistance, WantedSpecialBlend);
		MaxDistance			= LERP(1.0f, MaxDistance, WantedSpecialBlend);
	}
*/	

	// This replaces the above code.
	if( WantedDistance < 1.0f)
	{
		fp32 ModifiedDistanceBlend = Max(0.0f, (WantedDistance - 0.2f) / 0.8f);

		WantedBehindOffset	= LERP(CVec3Dfp32(0,0,0), WantedBehindOffset, ModifiedDistanceBlend);
		WantedDistance		= LERP(1.0f, WantedDistance, ModifiedDistanceBlend);
		MaxDistance			= LERP(1.0f, MaxDistance, ModifiedDistanceBlend);
	}

	// ----------------------------------------------
	// Set the values for the head and behind offset
	pCD->m_Camera_LastHeadOffset = pCD->m_Camera_CurHeadOffset;
	pCD->m_Camera_LastBehindOffset = pCD->m_Camera_CurBehindOffset;
	fp32 ModerateSpeedAdjustment = 20.0f * _pWPhysState->GetGameTickTime();
	ModerateVec3f(pCD->m_Camera_CurHeadOffset, WantedHeadOffset, pCD->m_Camera_CurHeadOffsetChange, (int)(PLAYER_CAMERACHANGE*ModerateSpeedAdjustment));
	ModerateVec3f(pCD->m_Camera_CurBehindOffset, WantedBehindOffset, pCD->m_Camera_CurBehindOffsetChange, (int)(PLAYER_CAMERADISTANCECHANGE*ModerateSpeedAdjustment));

/*
	// EXPIRED: SpecialRotation

	// ----------------------------------------------
	// Set the special blending
	pCD->m_Camera_LastSpecialBlend = pCD->m_Camera_CurSpecialBlend;
	WantedSpecialBlend *= 100.0f;
	pCD->m_Camera_CurSpecialBlend *= 100.0f;
	pCD->m_Camera_CurSpecialBlendChange *= 100.0f;
	Moderatef(pCD->m_Camera_CurSpecialBlend, WantedSpecialBlend, pCD->m_Camera_CurSpecialBlendChange, 100);
	WantedSpecialBlend *= 1.0f / 100.0f;
	pCD->m_Camera_CurSpecialBlend *= 1.0f / 100.0f;
	pCD->m_Camera_CurSpecialBlendChange *= 1.0f / 100.0f;

	// ----------------------------------------------
	// Set the value for the special rotation
	pCD->m_Camera_LastSpecialRotation = pCD->m_Camera_CurSpecialRotation;
	WantedSpecialRotation *= 100.0f;
	pCD->m_Camera_CurSpecialRotation *= 100.0f;
	pCD->m_Camera_CurSpecialRotationChange *= 100.0f;
	ModerateVec3f(pCD->m_Camera_CurSpecialRotation, WantedSpecialRotation, pCD->m_Camera_CurSpecialRotationChange, 100);
	pCD->m_Camera_CurSpecialRotation *= 1.0f / 100.0f;
	pCD->m_Camera_CurSpecialRotationChange *= 1.0f / 100.0f;
*/	

	// ----------------------------------------------
	// Fade the character if the camera is too close
/*	if((_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE) == 0)
	{
		fp32 WantedFade	= pCD->m_Camera_CurBehindOffset.Length() - 10.f;
		WantedFade = (WantedFade/pCD->m_Camera_FadeOffset) * 255.0f;
		WantedFade *= fp32(255 - pCD->m_FadeTargetInv) / 255;

		int Fade = pCD->m_iCharFade;
		int FadePrim = pCD->m_iCharFadePrim;
#ifdef CPU_INTELP5
		__asm xor eax, eax;		// Break optimization to get rid of compiler optimization fuck-up..  /mh
#endif 
		
#if defined(PLATFORM_XBOX) && !defined(M_RTM)
		ModerateCP(Fade, WantedFade, FadePrim, 200);
#else
		Moderate(Fade, WantedFade, FadePrim, 200);
#endif
		pCD->m_iCharFade = (uint8)Max(0, Min(255, Fade));
		pCD->m_iCharFadePrim = (int16)FadePrim;
		
		// Make sure the fade values are correct
		if(Fade > 255) 
			pCD->m_iCharFade = 255; 
		if(Fade < 0)
			pCD->m_iCharFade = 0;	
		
		// Don't fade the weapon
		//pCD->m_iWeaponFade = 255 - (255 - pCD->m_iCharFade)/1.5f;
	}
	else
	{
		pCD->m_iCharFade = 255;
		pCD->m_iCharFadePrim = 0;
	}*/
	

	// ----------------------------------------------
	// Chasecam distance
	pCD->m_LastChaseCamDistance	= pCD->m_ChaseCamDistance;
	WantedDistance				*= 100.0f;
	pCD->m_ChaseCamDistance		*= 100.0f;
	pCD->m_dChaseCamDistance	*= 100.0f;
	Moderatef(pCD->m_ChaseCamDistance, WantedDistance, pCD->m_dChaseCamDistance, (int)(PLAYER_CAMERADISTANCECHANGE*ModerateSpeedAdjustment));
	pCD->m_ChaseCamDistance		*= 1.f / 100.0f;
	pCD->m_dChaseCamDistance	*= 1.f / 100.0f;
	pCD->m_ChaseCamDistance		= Min(MaxDistance, pCD->m_ChaseCamDistance);

	//ConOut(CStrF("%s: WantedDistance: %f ChaseCamDistance: %f", _pWPhysState->IsServer() ? "S":"C",WantedDistance,pCD->m_ChaseCamDistance));
}


// =======================================================================
//
//	Function:			Traces the camera to see if its valid
//						
//	Comments:			Longer_description_not_mandatory
//
// =======================================================================
bool CWObject_Character::Camera_Trace(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, bool _bRangedWeapon, CMat4Dfp32& _CameraMatrix, CVec3Dfp32& _WantedBehindOffset, 
												  fp32& _WantedDistance, fp32& _MaxDistance, bool _Rotate)
{
	// Debug stuff
	bool DebugRender	= false;
	fp32 DebugDuration	= 90.0f;
	bool bWallHit		= false;
	CMat4Dfp32 Camera	= _CameraMatrix;

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD) return false;
	
	// If noclip then return
	if (Char_GetPhysType(_pObj) == PLAYER_PHYS_NOCLIP)
	{
		_WantedDistance = 1.0f;
		_MaxDistance = 1.0f;
		return false;
	}
	

	CVec3Dfp32 Head		= CVec3Dfp32::GetMatrixRow(Camera, 3);
	CVec3Dfp32 Behind	= _WantedBehindOffset;
	fp32 BehindDistance	= Behind.Length();
	
	if (BehindDistance > 127.0f)
	{
		Behind *= 127.0f / BehindDistance;
		BehindDistance = 127.0f;
	}
	
	Behind.MultiplyMatrix3x3(Camera);
	
	if (DebugRender) _pWPhysState->Debug_RenderVertex(Head, 0xF0000F0, DebugDuration);

	// Get the lookat directions
	CVec3Dfp32 Side, Up, Fwd;
	Side = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 0);
	Up	 = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 1);
	Fwd	= -(Behind / BehindDistance);
	Fwd.CrossProd(Side, Up);

	// =====================================================================================
	// Calculate the rays we're gonna use for tracing (so we can optimize the selection box)
	CBox3Dfp32 Box(Head, Head);
	CVec3Dfp32 Rays[CAMTRACE_RINGS * CAMTRACE_SIDES];
	int iNumRays = 0;
	Rays[iNumRays] = Head + Behind;
	Box.Expand(CBox3Dfp32(Head + Behind, Head + Behind));
	
	for (int iRing = 1; (iRing - 1) < CAMTRACE_RINGS; iRing++)
	{
		fp32 segmentfraction = ((fp32)iRing / (fp32)CAMTRACE_RINGS);
		if(_Rotate) segmentfraction += (ROTATION_ANGLE * pCD->m_iRotationIndex);
		fp32 a = 0.5f * (segmentfraction * 0.5f * _PI);	
		fp32 cosa = M_Cos(a);
		fp32 sina = M_Sin(a);
		
		for (int iSide = 0; iSide < CAMTRACE_SIDES; iSide++)
		{
			fp32 ringfraction = ((fp32)iSide / (fp32)CAMTRACE_SIDES); 
			if(_Rotate) ringfraction += (ROTATION_ANGLE * pCD->m_iRotationIndex);
			fp32 b = ringfraction * 2.0f * _PI;
			fp32 cosb = M_Cos(b) * sina;
			fp32 sinb = M_Sin(b) * sina;
			fp32 DistanceScale = 0.4f + 0.6f * Sqr(Sqr(cosa));
			CVec3Dfp32 ray = (-Fwd * cosa + Side * cosb + Up * sinb) * BehindDistance * DistanceScale;
			iNumRays++;
			if(iNumRays < CAMTRACE_RINGS * CAMTRACE_SIDES)
			{
				Rays[iNumRays] = Head + ray;
				Box.Expand(CBox3Dfp32(Head + ray, Head + ray));
			}
		}
	}


	bool bHit;
	CCollisionInfo CInfo;
	int objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL; //| OBJECT_FLAGS_PHYSOBJECT; // Exclude characters
	int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_CAMERASOLID | XW_MEDIUM_DYNAMICSSOLID;//XW_MEDIUM_PLAYERSOLID;
	
	// Create selection, to optimize tracing.
	TSelection<CSelection::LARGE_BUFFER> Selection;
	_pWPhysState->Selection_AddBoundBox(Selection, objects, Box.m_Min, Box.m_Max);

	_MaxDistance = 1.0f; 
	fp32 DistanceHint = _MaxDistance;
	

	// Trace the precalculated rays
	bHit = _pWPhysState->Phys_IntersectLine(Selection, Head, Rays[0], 0, objects, mediums, &CInfo);
	if (bHit && CInfo.m_bIsValid)
		_MaxDistance = Min(CInfo.m_Time, _MaxDistance);
		
	for (int iTraces = 1; iTraces < CAMTRACE_RINGS * CAMTRACE_SIDES; iTraces++)
	{
		bHit = _pWPhysState->Phys_IntersectLine(Selection, Head, Rays[iTraces], 0, objects, mediums, &CInfo);
		if (bHit && CInfo.m_bIsValid)
			DistanceHint = Min(CInfo.m_Time, DistanceHint);
		
		if (DebugRender) 
			if(bHit && CInfo.m_bIsValid)	
				_pWPhysState->Debug_RenderWire(Head, Rays[iTraces] * CInfo.m_Time, 0x40400000, DebugDuration);
			else							
				_pWPhysState->Debug_RenderWire(Head, Rays[iTraces], 0x40004000, DebugDuration);
	}

	if(!_Rotate) 
	{
		_WantedDistance = Min(Max(0.0f, DistanceHint), _MaxDistance);
		return bWallHit;
	}
	
	pCD->m_fTraceHistory = Min(DistanceHint, pCD->m_fTraceHistory);
	pCD->m_iRotationIndex++;
	
	if(Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
	{
		_WantedDistance = pCD->m_fTraceHistory;
	}
	else if(pCD->m_iRotationIndex == MAX_ROTATIONS)
	{
		_WantedDistance = Min(Max(0.0f, pCD->m_fTraceHistory), _MaxDistance); 
		pCD->m_fTraceHistory = _MaxDistance;
		pCD->m_fOldTraceHistory = _WantedDistance;
		pCD->m_iRotationIndex = 0;
	}
	else
	{
		_WantedDistance = pCD->m_fOldTraceHistory;
	}

	return bWallHit;
}

// =======================================================================
//
//	Function:			Shakes the camera
//						
//	Comments:			Longer_description_not_mandatory
//
// =======================================================================
void CWObject_Character::Camera_ShakeItBaby(CWorld_PhysState* _pWPhysState, CMat4Dfp32& _Camera, CWO_Character_ClientData* _pCD, fp32 _IPTime)
{
	if (_pCD == NULL)
		return;
	
	_pCD->m_PostAnimSystem.RecoilModCamera(&_Camera,_pCD);

	// Camera Shake (Shake camera before it's safetraced).
	uint32 ElapsedTicks = _pCD->m_GameTick - _pCD->m_CameraShake_StartTick;
	if ((_pCD->m_CameraShake_DurationTicks > 0) && (ElapsedTicks >= 0) && (ElapsedTicks <= _pCD->m_CameraShake_DurationTicks))
	{
		
		// Shake the camera
		fp32 iptime = _IPTime;
		CMTime GameTime = CMTime::CreateFromTicks(_pCD->m_GameTick, _pWPhysState->GetGameTickTime(), iptime);
		
		// Blend...
		fp32 Time = ((fp32)ElapsedTicks + iptime) * _pWPhysState->GetGameTickTime();
		fp32 Duration = (fp32)_pCD->m_CameraShake_DurationTicks * _pWPhysState->GetGameTickTime();
		fp32 BlendInTime = Duration * PLAYER_CAMERASHAKE_BLENDINDURATION;
		fp32 BlendOutTime = Duration * PLAYER_CAMERASHAKE_BLENDOUTDURATION;
		
		fp32 Blend = GetFade(Time, Duration, BlendInTime, BlendOutTime);
		
		fp32 ox, oy, oz, sx, sy, sz;
		ox = MFloat_GetRand(_pCD->m_CameraShake_Randseed + 0) * Duration;
		oy = MFloat_GetRand(_pCD->m_CameraShake_Randseed + 1) * Duration;
		oz = MFloat_GetRand(_pCD->m_CameraShake_Randseed + 2) * Duration;
		sx = 0.8f + 4.0f * MFloat_GetRand(_pCD->m_CameraShake_Randseed + 3);
		sy = 0.8f + 4.0f * MFloat_GetRand(_pCD->m_CameraShake_Randseed + 4);
		sz = 0.8f + 4.0f * MFloat_GetRand(_pCD->m_CameraShake_Randseed + 5);
		
		fp32 Speed = _pCD->m_CameraShake_Speed * PLAYER_CAMERASHAKE_FREQSCALE;
		
		CVec3Dfp32 MomentumVector;
//		MomentumVector[0] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime + ox) * Speed * sx) - 0.5f);
//		MomentumVector[1] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime + oy) * Speed * sy) - 0.5f);
//		MomentumVector[2] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime + oz) * Speed * sz) - 0.5f);
		MomentumVector[0] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime - CMTime::CreateFromSeconds((0.5f - ox) * Speed * sx)).GetTimeModulusScaled(Speed * sx, 1.0f)));
		MomentumVector[1] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime - CMTime::CreateFromSeconds((0.5f - oy) * Speed * sy)).GetTimeModulusScaled(Speed * sy, 1.0f)));
		MomentumVector[2] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime - CMTime::CreateFromSeconds((0.5f - oz) * Speed * sz)).GetTimeModulusScaled(Speed * sz, 1.0f)));

		fp32 Momentum = _pCD->m_CameraShake_Amplitude * Blend * PLAYER_CAMERASHAKE_SCALE;
		MomentumVector *= Momentum;
		
		CVec3Dfp32 MomentumTurn = MomentumVector * PLAYER_CAMERASHAKE_TURNSCALE;
		CVec3Dfp32 MomentumPitch = MomentumVector * PLAYER_CAMERASHAKE_PITCHSCALE;
		CVec3Dfp32 MomentumBank = MomentumVector * PLAYER_CAMERASHAKE_BANKSCALE;
		CVec3Dfp32 MomentumMove = MomentumVector * PLAYER_CAMERASHAKE_MOVESCALE;
		
		MomentumTurn.AddMatrixRow(_Camera, 0);
		MomentumPitch.AddMatrixRow(_Camera, 2);
		MomentumBank.AddMatrixRow(_Camera, 1);
		MomentumMove.AddMatrixRow(_Camera, 3);
		
		_Camera.RecreateMatrix(2, 1);
/*
			// -------------------------------------------------
#ifdef PLATFORM_XBOX
		
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CSystem* pSystem = pSys;
		
		// Rumble the control
		if(pSystem->GetOptions()->GetValuei("GAME_VIBRATION"))
		{
			if (Momentum > 0.03f)
			{
				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
				if (!pSys || !pSys->m_spInput) return;
				pSys->m_spInput->AppendEnvelope(0, CStr("RUMBLE"));  
			}
		}
#endif
		// -------------------------------------------------
*/
	}
}



// =======================================================================
//
//	Function:			Retrieves the position of the players head
//						
//	Comments:			Longer_description_not_mandatory
//
// =======================================================================
CVec3Dfp32 CWObject_Character::Camera_GetHeadPos(const CWObject_CoreData* _pObj)
{
	if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR)
	{
		return CVec3Dfp32(0.0f, 0.0f, 8.0f);
	}
	else
	{
		if(Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH)
			return CVec3Dfp32(0, 0, 35);
		else
			return CVec3Dfp32(0, 0, 56);

/*		const CWO_Character_ClientData* pCD = GetClientData(_pObj);
		if (pCD)
			return LERP(pCD->m_Camera_LastHeadOffset, pCD->m_Camera_CurHeadOffset, _IPTime);
		else
			return CVec3Dfp32(0.0f);*/
	}
}




// =======================================================================
//
//	Function:			Calculates the thirdperson offset
//						
//	Comments:			Longer_description_not_mandatory
//
// =======================================================================
void CWObject_Character::Camera_Offset(CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, CMat4Dfp32& _Camera, fp32 _IPTime)
{
	MSCOPE(Camera_ThirdPerson_Offset, CHARACTER);
	
	CWO_Character_ClientData* pCD = GetClientData(_pObj);
	if (pCD == NULL)
		return;

	bool DebugRender = false;
	fp32 DebugDuration = 90.0f;

	CVec3Dfp32 Head = CVec3Dfp32::GetMatrixRow(_Camera, 3);
	CVec3Dfp32 Behind = LERP(pCD->m_Camera_LastBehindOffset, pCD->m_Camera_CurBehindOffset, _IPTime);

	Behind.MultiplyMatrix3x3(_Camera);

	int objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;// | OBJECT_FLAGS_PHYSOBJECT; // Exclude characters
	int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_CAMERASOLID | XW_MEDIUM_DYNAMICSSOLID; //XW_MEDIUM_PLAYERSOLID;
	
	bool bHit;
	CCollisionInfo CInfo;
	
	fp32 Distance = LERP(pCD->m_LastChaseCamDistance, pCD->m_ChaseCamDistance, _IPTime);
	
	CVec3Dfp32 NormalOffset = 0.0f;
	
	if (DebugRender) _pPhysState->Debug_RenderVertex(Head + Behind * Distance, 0xF000FF00, 50.0f);
  
	bHit = _pPhysState->Phys_IntersectLine(Head, Head + Behind * Distance, 0, objects, mediums, &CInfo, _pObj->m_iObject);
	if (bHit && CInfo.m_bIsValid)
		Distance = Min(CInfo.m_Time * Distance, Distance);
	

	// Shrink distance with 5% (can't remember exactly why, but it gets a bit away of any walls).
	Behind *= Distance * 0.95f;

	fp32 BehindDistance = Behind.Length();

	if (DebugRender) _pPhysState->Debug_RenderVertex(Head, 0xF0FF00FF, DebugDuration);
	if (DebugRender) _pPhysState->Debug_RenderWire(Head, Head + Behind, 0xF00000FF, DebugDuration);

	//Include characters in cmaera FOV check
	objects |= OBJECT_FLAGS_PHYSOBJECT; 

	CVec3Dfp32 Adjustment = 0.0f;

	{ // Bound Check Camera FOV
		CVec3Dfp32 CameraPos = Head + Behind;

		CVec3Dfp32 Fwd = CVec3Dfp32::GetMatrixRow(_Camera, 2);
		CVec3Dfp32 Side = CVec3Dfp32::GetMatrixRow(_Camera, 0);
		CVec3Dfp32 Up = CVec3Dfp32::GetMatrixRow(_Camera, 1);

		fp32 HardBoundRadius = 5.0f;
		fp32 SoftBoundRadius = 8.0f;

		CVec3Dfp32 SafeVector;


		SafeTrace(Side + Fwd);
		SafeTrace(-Side + Fwd);
		SafeTrace(Up + Fwd);
		SafeTrace(-Up + Fwd);

		Adjustment *= 1.0f; // Scale down, since there may often be several safevectors hitting the same wall.

		if (DebugRender) _pPhysState->Debug_RenderWire(Head + Behind, Head + Behind + Adjustment, 0xF0F0F000, DebugDuration);
		if(Adjustment.Length() > 0)
			Behind += Adjustment;
	}

	if (DebugRender) _pPhysState->Debug_RenderWire(Head, Head + Behind, 0xF000FF00, DebugDuration);

	// Make sure bound adjustment didn't invalidate camera.
	objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;// | OBJECT_FLAGS_PHYSOBJECT; // Exclude characters
	bHit = _pPhysState->Phys_IntersectLine(Head, Head + Behind, 0, objects, mediums, &CInfo, _pObj->m_iObject);
	if (bHit && CInfo.m_bIsValid)
		Behind *= CInfo.m_Time;

	Behind.AddMatrixRow(_Camera, 3);

	CVec3Dfp32 Fwd = CVec3Dfp32::GetMatrixRow(_Camera, 2);

	if (BehindDistance > 0)
	{
		Fwd -= Adjustment / BehindDistance;
		Fwd.Normalize();
		Fwd.SetMatrixRow(_Camera, 2);
		_Camera.RecreateMatrix(2, 1);
	}

/*
	// EXPIRED: SpecialRotation
	if (AssistCamera(_pObj) && (!(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOCLIP)))
	{
		fp32 iptime = _IPTime;
		fp32 WantedTilt = pCD->m_Control_Look_Wanted[1];

		CVec3Dfp32 SpecialRotation = LERP(pCD->m_Camera_LastSpecialRotation, pCD->m_Camera_CurSpecialRotation, iptime);
		SpecialRotation *= PLAYER_SPECIALROTATION_RANGE;
		SpecialRotation *= LERP(pCD->m_Camera_LastSpecialBlend, pCD->m_Camera_CurSpecialBlend, iptime);

		_Camera.RotX_x_M(SpecialRotation.k[0]);
		_Camera.RotY_x_M(SpecialRotation.k[1]);
		_Camera.RotZ_x_M(SpecialRotation.k[2]);
	}
*/

	if(Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
	{
		_Camera.RotY_x_M(LERP(pCD->m_Camera_Rotation - 0.0015f, pCD->m_Camera_Rotation,_IPTime)); 
		_Camera.RotX_x_M(-0.15f); 
 	}

/*
	pCD->m_ChaseCameraPos = CVec3Dfp32::GetMatrixRow(_Camera, 3);
	pCD->m_ChaseCameraDir = CVec3Dfp32::GetMatrixRow(_Camera, 2);
*/
}



#define  CAMERA_MAXDISTANCEHEAD		4.0f
#define  CAMERA_MAXDISTANCEPITCH	5.0f
#define  CAMERA_MINPITCH			-0.17f
#define  CAMERA_FULLPITCH			-0.08f
#define  CAMERA_FILTERSPEED		0.2f
#define  CAMERA_SPEEDLERPER		0.05f


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Retrieves the camera matrix.

	Parameters:	
		_pWPhysState:		The client / server object.
		_Camera:			Matrix to store the result in.
		_pCamObj:			The character object
		_IPTime:			The interpolation time.

	Comments:		Before calling this function, the _Camera matrix 
					needs to be initialized with the character position
					and character look (rotation).
\*____________________________________________________________________*/
void CWObject_Character::Camera_Get(CWorld_PhysState* _pWPhysState, CMat4Dfp32* _Camera, CWObject_CoreData* _pCamObj, fp32 _IPTime)
{
	MSCOPE(GetCamera, CHARACTER);
	
	CWO_Character_ClientData* pCD = GetClientData(_pCamObj);
	if (pCD == NULL)
		return;
	CWO_Character_ClientData *pCDFirst =  NULL;
	if(_pWPhysState->IsClient())
	{
		CWObject_CoreData* pObjFirst = ((CWorld_Client *)_pWPhysState)->Object_GetFirstCopy(_pCamObj->m_iObject);
		if (pObjFirst)
		{
			pCDFirst = GetClientData(pObjFirst);
			pCDFirst->m_Camera_CharacterOffset = 0;
		}
	}

	CMat4Dfp32& Camera = *_Camera;

	// Offset the camera to around the head....
	CVec3Dfp32 CameraPos = CVec3Dfp32::GetMatrixRow(Camera, 3);

//	CRegistry* pUserReg = _pWPhysState->Registry_GetUser();
//	int bThirdPerson = (pUserReg ? pUserReg->GetValuei("THIRDPERSON") : 0);
	bool bThirdPerson = (pCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
	bool bNoClip = (Char_GetPhysType(_pCamObj) == PLAYER_PHYS_NOCLIP);

	//AR-NOTE: P5 prototype hack:
	/*bool b3PI = ((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) != THIRDPERSONINTERACTIVE_MODE_NONE);
	if (b3PI)
		bThirdPerson = false;*/

	if(_pCamObj->m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR)
	{
		Camera.RotX_x_M(-0.25f);
		Camera.RotY_x_M(0.25f);
		Camera.k[3][2] += _pCamObj->GetPhysState().m_Prim[0].GetDim()[2];
		Camera_ShakeItBaby(_pWPhysState, Camera, pCD, _IPTime);
	}
	else if (bThirdPerson && !bNoClip)
	{
		Camera.RotX_x_M(-0.25f);
		Camera.RotY_x_M(0.25f);

		CVec3Dfp32::GetMatrixRow(Camera, 3) += Camera_GetHeadPos(_pCamObj);
		Camera_ShakeItBaby(_pWPhysState, Camera, pCD, _IPTime);

		Camera_Offset(_pCamObj, _pWPhysState, Camera, _IPTime);
	}
	else if (!bNoClip)
	{
		Camera_Get_FirstPerson(_pWPhysState, _Camera, _pCamObj, _IPTime);
	}
	else
	{
		CVec3Dfp32::GetMatrixRow(Camera, 3) += Camera_GetHeadPos(_pCamObj);
		Camera.RotX_x_M(-0.25f);
		Camera.RotY_x_M(0.25f);
	}

	// If an actioncutscenecamera is active, use that instead...
	int32 CameraMode = pCD->m_ActionCutSceneCameraMode;
	bool bACS = (CameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE) != 0;
	bool bACS_3PI = ((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) == THIRDPERSONINTERACTIVE_MODE_ACS);
	bool bCamUtilMounted = pCD->m_CameraUtil.IsActive(CAMERAUTIL_MODE_MOUNTEDCAM);

	if (bNoClip || _pWPhysState->IsServer())
		bACS_3PI = false; 

	fp32 SlideTime = pCD->m_3PI_CameraSlide.Get(pCD->m_GameTick, _IPTime) * (1.0f / 255.0f);
	if ((/*bThirdPerson && */!bNoClip) && (bACS || bCamUtilMounted || (bACS_3PI && SlideTime > 0.0f)))
	{
		// Use the Action cutscene camera instead
		CMat4Dfp32 ThirdPersonCamera = Camera;

		if (bACS)
		{
			CActionCutsceneCamera ActionCam;
			ActionCam.GetCamVars().ImportFromPCD(pCD, _pCamObj->m_iObject);
			ActionCam.GetCameraPositionClient(ThirdPersonCamera, ActionCam.GetCamVars(), _pCamObj, _pWPhysState, _IPTime);
		}
		else
		{
			// Temp test
			pCD->m_CameraUtil.GetCamera(_pWPhysState,ThirdPersonCamera,_pCamObj,pCD,_IPTime);
			if (pCD->m_iMountedObject != 0 && (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_BLENDANIM))
			{
				fp32 BlendTime = 0.5f; // Fixed blendtime atm..
				CMTime OtherTime = CMTime::CreateFromTicks(pCD->m_GameTick - pCD->m_MountedStartTick,_pWPhysState->GetGameTickTime(),_IPTime);
				fp32 SkelBlend = Max(0.0f,Min(1.0f,(fp32)(OtherTime.GetTime()) / BlendTime));
				if (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_BLENDOUTANIM)
				{
					// We're about to blend out, reverse order
					SkelBlend = 1.0f - SkelBlend;
				}
				CMat4Dfp32 Temp;
				Interpolate2(Camera,ThirdPersonCamera,Temp,SkelBlend);
				ThirdPersonCamera = Temp;
			}
		}

		if (bACS_3PI)
			Camera = ThirdPersonCamera;
			//InterpolateCamera(Camera, ThirdPersonCamera, SlideTime, Camera);
		else
			Camera = ThirdPersonCamera;

		return;
	}
}


int CWObject_Character::OnGetCamera(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	MSCOPE(OnGetCamera, CHARACTER);

	fp32 IPTime = _pWClient->GetRenderTickFrac();
	fp32 RenderIPTime = IPTime;

	if (_Msg.m_DataSize != sizeof(CMat4Dfp32)) return 0;
	CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;

	CWO_Character_ClientData* pCD = GetClientData(_pObj);
	if(!pCD)
		return 0;

#ifdef WADDITIONALCAMERACONTROL
	if (pCD->m_bAdditionalCameraControl)
	{
		CWC_Player* pP = _pWClient->Player_GetLocal();

		((CWorld_ClientCore*)_pWClient)->Con_Cmd(127);
		OnClientPredict(_pObj, _pWClient, 0, 0, 1);

		CWObject_Client* pCamObj = Player_GetLastValidPrediction(_pObj);
		CWO_Character_ClientData* pCDPred = GetClientData(pCamObj);

		CVec3Dfp32 LookAdd = 0.0f;
		LookAdd[1] = pCDPred->m_AdditionalCameraControlY;
		LookAdd[2] = pCDPred->m_AdditionalCameraControlZ;

		LookAdd.CreateMatrixFromAngles(0, Camera);
		pCDPred->m_AdditionalCameraPos.m_Value.SetMatrixRow(Camera,3);
		Camera.RotX_x_M(-0.25f);
		Camera.RotY_x_M(0.25f);
		return 1;
	}
#endif

	//ConOutL(CStrF("GameTick %d, IpTime %f, RealTime %f", _pWClient->GetGameTick(), IPTime, GetCPUClock() / GetCPUFrequency() ));

	if (pCD->m_iSurveillanceCamera != 0)
	{
		int16 iCameraObj = pCD->m_iSurveillanceCamera;
		CMTime Time = _pWClient->GetTime();
		CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, (aint)(&Time), (aint)_Msg.m_pData);
		if(_pWClient->ClientMessage_SendToObject(Msg, iCameraObj))
		{
			Camera.RotX_x_M(-0.25f);
			Camera.RotY_x_M(0.25f);

			/*
			CVec3Dfp32 Pos(32.0f, 0.0f, 0.0f);
			Pos = Pos * Camera;
			Pos.SetMatrixRow(Camera, 3);
			*/
			return 1;
		};
	};

	// Interpolate direction and position for player-object.
	bool bTeleport = (pCD->m_LastTeleportTick == pCD->m_GameTick);
	if (bTeleport)
		Camera = _pObj->GetPositionMatrix();
	else
		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), Camera, IPTime );

	// This is not interpolated as the one above, ehm...but Control_Look has no Last stored =).
	//			GetClientData(_pObj)->m_Control_Look.CreateMatrixFromAngles(0, Camera);
	//			CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(), 3).AddMatrixRow(Camera, 3);

	CWObject_Client* pCamObj = _pObj;

	// Are we not in recording-playback mode? (2)
	if (_pWClient->GetRecMode() != 2)
	{
		// 2 seconds is considered connection failure.
		//				bool bLag = ((fp64)GetCPUClock()/GetCPUFrequency() - _pWClient->GetLastSnapshotTime()) * _pWClient->GetTimeScale() > 2.0f;
//		bool bLag = (CMTime::GetCPU() - _pWClient->GetLastSnapshotTime()).GetTime() * _pWClient->GetTimeScale() > 2.0f;
		CMTime TCurrent;
		TCurrent.Snapshot();
		//JK-NOTE: Statement below gets raped by GCC linker and is thus replace by the if statement instead.
//		bool bLag = ((TCurrent - _pWClient->GetLastSnapshotTime()).GetTime() * _pWClient->GetTimeScale()) > 2.0f;
		bool bLag = false;
		if(((TCurrent - _pWClient->GetLastSnapshotTime()).GetTime() * _pWClient->GetTimeScale()) > 2.0f)
			bLag = true;

		if (_pWClient->GetClientMode() & WCLIENT_MODE_PAUSE)
			bLag = false;

#ifdef WCLIENT_FIXEDRATE
		int bNoPredict = true;
#else
		CRegistry* pUserReg = _pWClient->Registry_GetUser();
		int bNoPredict = (pUserReg) ? pUserReg->GetValuei("NOPREDICT") : 0;
#endif

		if (bNoPredict)
		{
			_pObj->Unlink();
			//					_pObj->m_spNextCopy = NULL;
		}

		// No, check if we're dead or lagging, in which case the camera is entirely controlled by the server-side.
		if (bNoPredict /*|| 
					   (Char_GetPhysType(this) == PLAYER_PHYS_DEAD)*/ || 
					  /* (_pWClient->GetClientMode() & WCLIENT_MODE_PAUSE) || */
					   bLag)
		{
			// Yes
			//					ConOut("No prediction");
//			int a = 1;	// Just so I can break here
		}
		else
		{
#ifndef WCLIENT_NOPREDICTION
			TProfileDef(TPredict);
			{
				TMeasureProfile(TPredict);
				{
					M_NAMEDEVENT("OnClientPredict", 0xffff00ff);
//					CWC_Player* pP = _pWClient->Player_GetLocal();

					((CWorld_ClientCore*)_pWClient)->Con_Cmd(127);
					OnClientPredict(_pObj, _pWClient, 0, 0, 1);

	//				if (bPut) pP->m_spCmdQueue->DelHead();
				}

				pCamObj = Player_GetLastValidPrediction(_pObj);
				IPTime = GetClientData(pCamObj)->m_PredictFrameFrac;

				//pCamObj = pCamObj->GetNext();
			}
			//			ConOutL(CStrF("Time %f", GetCPUClock()/GetCPUFrequency()) +  T_String("Predict", TPredict));
#endif

			// Removed by Mondelore (FIXME: Not 100% sure if I may change this =).
			if(Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
			{
				// Control look is not correct if dead
				Camera = pCamObj->GetPositionMatrix();
			}
			else
			{
				GetClientData(pCamObj)->m_Control_Look.CreateMatrixFromAngles(0, Camera);
				CVec3Dfp32::GetMatrixRow(pCamObj->GetPositionMatrix(), 3).AddMatrixRow(Camera, 3);
			}
		}
	}

	{

		CWO_Character_ClientData *pCD = GetClientData(pCamObj);
		if (pCD)
		{
			if (pCD->m_Cam_Valid && !bTeleport)
			{
				CQuatfp32 Q;
				CVec3Dfp32 P;
				pCD->m_Cam_LastRot.Lerp(pCD->m_Cam_Rot, IPTime, Q);
				Q.CreateMatrix(Camera);

				//GetClientData(pCamObj)->m_Control_Look.CreateMatrixFromAngles(0, Camera);

				pCD->m_Cam_LastPos.Lerp(pCD->m_Cam_Pos, IPTime, CVec3Dfp32::GetMatrixRow(Camera, 3));
			}


/*			if (pCD->m_Cam_BobValid)
			{
				CQuatfp32 Q, Q2, QT;
				CMat4Dfp32 Mat, NewCam;
				CVec3Dfp32 BobMove;
				pCD->m_Cam_LastBob.Interpolate(pCD->m_Cam_Bob, Q, IPTime);
				pCD->m_Cam_LastBobMove.Lerp(pCD->m_Cam_BobMove, IPTime, BobMove);

#ifdef LOG_PREDICT2
				ConOutL(CStrF("(CD %.8x) Using bob-z %f to %f, t = %f, IPZ %f", 
					pCD, pCD->m_Cam_LastBobMove[2], pCD->m_Cam_BobMove.k[2], IPTime, BobMove[2]));
#endif

				pCD->m_Cam_Tilt.CreateQuaternion(QT);
				Q.Multiply(QT, Q2);

				Q.Normalize();
				Q2.Normalize();

				Q2.CreateMatrix(Mat);

				Q.CreateMatrix(Mat);	// <-- ignore tilt.

				Mat.Multiply(Camera, NewCam);
				Camera = NewCam;
				CVec3Dfp32::GetMatrixRow(Camera, 3) += BobMove;

			}


			// Elapse tilt
			{
				fp32 RTime = _pWClient->GetRenderTime();
				pCD->ModerateTilt(RTime - pCD->m_Cam_TiltTime);
				pCD->m_Cam_TiltTime = RTime;
			}
*/
			{
				CWObject_CoreData* pObjFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
				if (pObjFirst)
				{
					CWO_Character_ClientData *pCDFirst = GetClientData(pObjFirst);
					if (pCDFirst)
					{
						int GameTick = _pWClient->GetGameTick();
						int PMT = pCDFirst->m_PredMiss_Tick;
						fp32 Frac = _pWClient->GetRenderTickFrac();
						fp32 IPPredMiss = 1.0f - Clamp01(fp32(GameTick - PMT) + Frac);
//						if (IPPredMiss < 0.0f)
						CVec3Dfp32 PredMissAdd;
						pCDFirst->m_PredMiss_dPos.Scale(IPPredMiss, PredMissAdd);

//						fp32 OldCamY = Camera.k[3][2];

						if (!bTeleport)
							Camera.GetRow(3) += PredMissAdd;

//					LogFile(CStrF("Z = %f = %f + %f*%f (%f), Tick %d, PMT %d, LUNT %d, Frac %f, dPos %s", 
//						Camera.k[3][2], OldCamY, pCDFirst->m_PredMiss_dPos[2], IPPredMiss, (pCDFirst->m_PredMiss_dPos[2] * IPPredMiss), _pWClient->GetGameTick(), PMT, pCDFirst->m_LastUpdateNumTicks, _pWClient->GetRenderTickFrac(), PredMissAdd.GetString().Str()));
					}
				}
			}


			bool bCreepOk = false;
			if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_CREEPINGCAM)
			{
				CWObject_CoreData* pCreep = _pWClient->Object_GetCD(pCD->m_iCreepingDark);
				if (pCreep)
				{
					CMat4Dfp32 CreepCam;
					// Give characters control_look to creepingdark and let it formulate a camera
					if (CWObject_CreepingDark::GetCamera(pCreep, _pWClient, CreepCam, pCD->m_Creep_Control_Look_Wanted.m_Value, _pWClient->GetGameTick(), IPTime/*_pWClient->GetRenderTickFrac()*/))
					{
						//_pWPhysState->Debug_RenderMatrix(TempMat,1.0f);
						bCreepOk = true;

						bool bIsEnd = CWObject_CreepingDark::BackTrackReachedEnd(pCreep);
						int SpecialTick =  bIsEnd ? CWObject_CreepingDark::GetEndTick(pCreep) : CWObject_CreepingDark::GetStartTick(pCreep);
//						if (bIsEnd)
//							int i =0;
						fp32 CurrentTick = _pWClient->GetGameTick() - SpecialTick;
						if (CurrentTick < 5.0f || bIsEnd)
						{
							CMat4Dfp32 CharCam;
							CQuatfp32 Creep,Char,Dest;
							CVec3Dfp32 Pos;
							
							fp32 Blend = (CurrentTick + _pWClient->GetRenderTickFrac()) / 5.0f;
							if (bIsEnd)
								Blend = 1.0f - Blend;
							Blend = Clamp01(Blend);
							Camera_Get(_pWClient, &Camera, pCamObj, IPTime);
							CharCam = Camera;

							Creep.Create(CreepCam);
							Char.Create(CharCam);
							Char.Lerp(Creep,Blend, Dest);
							CVec3Dfp32::GetMatrixRow(CharCam,3).Lerp(CVec3Dfp32::GetMatrixRow(CreepCam,3),Blend,Pos);
							Dest.CreateMatrix(Camera);
							Pos.SetMatrixRow(Camera,3);
						}
						else
						{
							Camera = CreepCam;
						}
					}
				}
			}

			if (!bCreepOk)
				Camera_Get(_pWClient, &Camera, pCamObj, IPTime);

			// Do some camera rotation. Object's forward vector is the matrix's x-axis, but for rendering the forward-vector needs to be z
//			Camera.RotX_x_M(-0.25f);
//			Camera.RotY_x_M(0.25f);
		}
		else
		{
			Camera.RotX_x_M(-0.25f);
			Camera.RotY_x_M(0.25f);
		}
	}

	bool bSwappedSpeaker = false;

	// Cutscene camera
	bool bDialogue3PI = ((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) == THIRDPERSONINTERACTIVE_MODE_DIALOGUE);
	bool bDialogue = (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE) != 0;
	fp32 SlideTime = pCD->m_3PI_CameraSlide.Get(_pWClient->GetGameTick(), RenderIPTime) * (1.0f / 255.0f);

	if(pCD->m_3PI_NoCamera)
		bDialogue3PI = false;

	int iCameraObj = 0;
	if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE)
	{
		iCameraObj = (uint16)pCD->m_Cutscene_Camera;
	}
	else if (bDialogue || bDialogue3PI)
	{
		int iSpeaker, iListener;
		if (!bDialogue)
		{
			iListener = _pObj->m_iObject;
			iSpeaker = pCD->m_3PI_FocusObject;

			if (pCD->m_DialogueInstance.IsValid())
			{
				if(!bSwappedSpeaker)
					bSwappedSpeaker = true;

				//pCD->m_PlayerDialogueToken.m_iLastSpeaker = iListener;
				Swap(iSpeaker, iListener);
				pCD->m_PlayerDialogueToken.m_iLastSpeaker = iSpeaker;
			}
			else
			{
				if(bSwappedSpeaker)
					bSwappedSpeaker = false;
			}
		}
		else if ((pCD->m_PlayerDialogueToken.m_iLastSpeaker != 0) && 
		         (pCD->m_PlayerDialogueToken.m_iLastSpeaker != pCD->m_PlayerDialogueToken.m_iOwner))
		{
			iListener = pCD->m_PlayerDialogueToken.m_iOwner;
			iSpeaker = pCD->m_PlayerDialogueToken.m_iLastSpeaker;
		} 
		else
		{
			iListener = _pObj->m_iObject;
			iSpeaker = pCD->m_iDialogueTargetObject;
		}

		CMat4Dfp32 ThirdPersonCamera = Camera;
		//GetDialogueCamera_Client(pCD, _pWClient, iSpeaker, iListener, ThirdPersonCamera, RenderIPTime);

		CDialogueInstance *pDialogueInst = &pCD->m_DialogueInstance;

		int iObjectIndices[2];
		iObjectIndices[0] = iListener;
		iObjectIndices[1] = iSpeaker;
		GetDialogueCamera_Client(pCD, _pWClient, pDialogueInst, iObjectIndices, ThirdPersonCamera, RenderIPTime, bSwappedSpeaker);

		// getclinetcam_obj
		CWO_Character_ClientData* pCDPred = GetClientData(pCamObj);
	
		if((!pCD->m_bWasSwappedSpeaker && bSwappedSpeaker) || (pCD->m_bWasSwappedSpeaker && !bSwappedSpeaker))	
		{
			pCD->m_bWasSwappedSpeaker = bSwappedSpeaker;
			pCD->m_DialogCamLook = 0;
		}
		else
		{
			ThirdPersonCamera.RotX_x_M(pCDPred->m_DialogCamLook.k[1]);
			ThirdPersonCamera.RotY_x_M(pCDPred->m_DialogCamLook.k[2]);

			// fix tilt
			ThirdPersonCamera.GetRow(0).k[2] = 0;
			ThirdPersonCamera.RecreateMatrix(0,1);

			ZeroInLookForDialogueCameras(pCD);
		}		
		
		//if(SlideTime > 0.0f)
			Camera = ThirdPersonCamera;

		//InterpolateCamera(Camera, ThirdPersonCamera, SlideTime, Camera);

		return 1;
	}
	else
	{
		pCD->m_bFirstFrameOfDialogue = false;
		pCD->m_DialogueCamera.Clear();
	}

	if(pCD->m_iMountedObject)
		iCameraObj = (uint16)pCD->m_iMountedObject;

	if(iCameraObj != 0 && !pCD->m_CameraUtil.IsActive(CAMERAUTIL_MODE_MOUNTEDCAM))
	{
		CWObject_Client *pCam = _pWClient->Object_Get(iCameraObj);
		if (pCam)
		{
			if (pCam->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
			{
				CWObject_Message Msg(OBJMSG_CHAR_GETMOUNTCAMERA,pCD->m_MountedMode);
				Msg.m_pData = &Camera;
				if(_pWClient->ClientMessage_SendToObject(Msg, iCameraObj))
				{
					Camera_ShakeItBaby(_pWClient, Camera, pCD, RenderIPTime);
					return 1;
				}
			}
			else if(_pWClient->ClientMessage_SendToObject(_Msg, iCameraObj))
			{
				Camera.RotX_x_M(-0.25f);
				Camera.RotY_x_M(0.25f);
			}
		}
	}

	Camera_ShakeItBaby(_pWClient, Camera, pCD, RenderIPTime);

	return 1;
}

void CWObject_Character::ZeroInLookForDialogueCameras(CWO_Character_ClientData *_pCD)
{
	fp32 TimeSinceLastLookChange = Max((_pCD->m_GameTime - _pCD->m_AutoAim_LastLookMoveTime).GetTime(),0.0f);
	if(TimeSinceLastLookChange > 3.0f)
	{
		fp32 BlendTime = (TimeSinceLastLookChange - 3.0f) / 2.0f; //  two sec blend back to zero
		_pCD->m_DialogCamLook *= 1.0f - Min(BlendTime, 1.0f);
	}	
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Gets the first person view camera.

	Parameters:	
		_pWPhysState:		The client / server object.
		_pCamObj:			The character object
		_IPTime:			The interpolation time.

	Comments:		The code was moved from Camera_Get, in order to
					be able to get the first person camera even though
					the character is using another camera mode.
\*____________________________________________________________________*/
void CWObject_Character::Camera_Get_FirstPerson(CWorld_PhysState* _pWPhysState, 
												CMat4Dfp32* _Camera, CWObject_CoreData* _pCamObj, 
												fp32 _IPTime)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pCamObj);
	if (pCD == NULL)
		return;

	CWO_Character_ClientData *pCDFirst =  NULL;
	if (_pWPhysState->IsClient())
	{
		CWObject_CoreData* pObjFirst = ((CWorld_Client *)_pWPhysState)->Object_GetFirstCopy(_pCamObj->m_iObject);
		if (pObjFirst)
		{
			pCDFirst = CWObject_Character::GetClientData(pObjFirst);
			pCDFirst->m_Camera_CharacterOffset = 0;
		}
	}

	CMat4Dfp32& Camera = *_Camera;

	{
		// First person camera
		CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(_pCamObj->m_iModel[0]);
		CXR_Skeleton *pSkel = pModel ? pModel->GetPhysSkeleton() : NULL;

		CXR_SkeletonInstance SkelInst;
		CXR_AnimState Anim;
		/*if (!CWObject_Character::OnGetAnimState(_pCamObj, _pWPhysState, pSkel, 0, *_Camera, _IPTime, Anim, NULL))
		return;*/
		if (!(pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_ORIGINAL) && pCD->m_iMountedObject != 0 && (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_BLENDANIM))
		{
			CXR_SkeletonInstance* pSkelInst = NULL;
			CWObject_CoreData* pOther = _pWPhysState->Object_GetCD(pCD->m_iMountedObject);
			fp32 BlendTime = 0.5f; // Fixed blendtime atm..
			bool bOk = false;
			if (pOther)
			{
				CWO_Character_ClientData *pCDOther = CWObject_Character::GetClientData(pOther);
				fp32 IPTimeOther = pCDOther->m_PredictFrameFrac;
				if (_pWPhysState->IsClient())//pOther->GetNext() != NULL)
				{
					CWObject_Client* pOtherClient = ((CWorld_Client *)_pWPhysState)->Object_Get(pOther->m_iObject);
					pOther = Player_GetLastValidPrediction(pOtherClient);
					pCDOther = CWObject_Character::GetClientData(pOther);
					IPTimeOther = pCDOther->m_PredictFrameFrac;
				}

				CMTime OtherTime = CMTime::CreateFromTicks(_pWPhysState->GetGameTick() - pCD->m_MountedStartTick,_pWPhysState->GetGameTickTime(),_IPTime);
				fp32 SkelBlend = Max(0.0f,Min(1.0f,(fp32)(OtherTime.GetTime()) / BlendTime));
				if (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_BLENDOUTANIM)
				{
					// We're about to blend out, reverse order
					SkelBlend = 1.0f - SkelBlend;
				}
				CXR_Model *pModel = _pWPhysState->GetMapData()->GetResource_Model(pOther->m_iModel[0]);
				CXR_Skeleton* pSkelOther = pModel ? pModel->GetPhysSkeleton() : NULL;
				CMat4Dfp32 MatIPOther;
				Interpolate2(pOther->GetLastPositionMatrix(), pOther->GetPositionMatrix(), MatIPOther, IPTimeOther);
				if (SkelBlend > 0.0f)
				{
					if (SkelBlend < 1.0f)
					{
						// Save camera from the evil blend things (...)
						// Get animstate for this object, the target object and blend them
						CXR_AnimState AnimThis,AnimOther;
						CVec3Dfp32 AnimPosOther,AnimPosThis;
						OnGetAnimState(pOther, _pWPhysState, pSkelOther, 1, MatIPOther, IPTimeOther, AnimOther, &AnimPosOther);
						OnGetAnimState(_pCamObj, _pWPhysState, pSkel, 1, *_Camera, _IPTime, AnimThis, &AnimPosThis);
						CMat4Dfp32 CameraTrans;
						CameraTrans.Unit();
						if ((PLAYER_ROTTRACK_CAMERA < AnimOther.m_pSkeletonInst->m_nBoneTransform) && PLAYER_ROTTRACK_CAMERA < AnimThis.m_pSkeletonInst->m_nBoneTransform)
						{
							CQuatfp32 Q1,Q2,Q3;
							const CMat4Dfp32& Mat = AnimThis.m_pSkeletonInst->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];
							const CMat4Dfp32& MatOther = AnimOther.m_pSkeletonInst->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];

							Q1.Create(MatOther);
							Q2.Create(Mat);
							Q1.Lerp(Q2,SkelBlend,Q3);
							Q3.CreateMatrix(CameraTrans);
							CVec3Dfp32::GetMatrixRow(MatOther,3).Lerp(CVec3Dfp32::GetMatrixRow(Mat,3),SkelBlend,CVec3Dfp32::GetMatrixRow(CameraTrans,3));
						}

						// Create animinstance
						SkelInst.Create(pSkel->m_lNodes.Len());
						Anim.m_pSkeletonInst = &SkelInst;
						// Only need the camera really, no need to blend the skeletons...
						if (PLAYER_ROTTRACK_CAMERA < Anim.m_pSkeletonInst->m_nBoneTransform)
						{
							Anim.m_pSkeletonInst->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA]=CameraTrans;
						}
						bOk = true;
					}
				}
				else
				{
					// Do Other char
					OnGetAnimState(pOther, _pWPhysState, pSkelOther, 1, MatIPOther, IPTimeOther, Anim, NULL);
					bOk = true;
				}
			}
			if (!bOk)
			{
				// Do this char
				OnGetAnimState(_pCamObj, _pWPhysState, pSkel, 1, *_Camera, _IPTime, Anim, NULL);
			}
		}
		else
		{
			OnGetAnimState(_pCamObj, _pWPhysState, pSkel, 1, *_Camera, _IPTime, Anim, NULL);
		}

		CXR_SkeletonInstance* pSkelInst = Anim.m_pSkeletonInst;

		if (!pModel || !pSkel)
		{
			Camera.RotX_x_M(-0.25f);
			Camera.RotY_x_M(0.25f);
			fp32 Offset = CWObject_Character::Char_GetPhysType(_pCamObj) == PLAYER_PHYS_CROUCH ? 30.0f : 60.0f;
			CVec3Dfp32::GetMatrixRow(Camera,3).k[2] += Offset;
			return;
		}

		CMat4Dfp32 Trans;
		CVec3Dfp32 Point = 0;

		bool bOk = false;
		{
			int RotTrack = PLAYER_ROTTRACK_CAMERA;
			if (pSkelInst && RotTrack < pSkelInst->m_nBoneTransform)
			{
				Point = pSkel->m_lNodes[PLAYER_ROTTRACK_CAMERA].m_LocalCenter;
				Trans = pSkelInst->GetBoneTransform(PLAYER_ROTTRACK_CAMERA);
				bOk = true;
			}

			if(!pCD->IsDarkling())
			{
				const CXR_SkeletonAttachPoint* pPoint = pSkel->GetAttachPoint(PLAYER_ATTACHPOINT_FACE);
				if (pPoint)
					Point = pPoint->m_LocalPos.GetRow(3);
			}
		}

		if(!bOk)
			Trans.Unit();

		CVec3Dfp32 IdealPosition = Point * Trans;

		// paranoia: if the Trans-matrix is fucked, let's at least have some kind of valid camera
		if (IdealPosition.k[0] != IdealPosition.k[0])
		{
			M_TRACEALWAYS("WARNING: Fucked up camera rottrack in skeleton instance\n");
			IdealPosition = _pCamObj->GetPosition() + Point;
		}

/*		Test for Resident Evil 4 style of camera
		CMat4Dfp32 Mat = _pCamObj->GetPositionMatrix();
		Mat.k[3][2] += 56;
		IdealPosition = CVec3Dfp32(-16, -12, 0) * Mat;*/

		CVec3Dfp32 ResultingPos = IdealPosition;
		if(pCDFirst)
		{
			CVec3Dfp32 LastPos = pCDFirst->m_Camera_LastPosition;
			fp32 SpeedPercent = 0;
			CMTime Time = CMTime::CreateFromTicks(pCD->m_GameTick, _pWPhysState->GetGameTickTime(), _IPTime);
			{
				SpeedPercent = Min(_pCamObj->GetMoveVelocity().Length() / pCDFirst->m_Speed_Forward,1.0f);

				if(pCD->m_Control_Look[1] > CAMERA_FULLPITCH)
					SpeedPercent = Max(SpeedPercent, 0.2f);				
				else if(pCD->m_Control_Look[1] < CAMERA_MINPITCH)
					SpeedPercent = 0.0f;
				fp32 dT = (Time - pCDFirst->m_Camera_LastCalcTime).GetTime();
				SpeedPercent = LERP(pCDFirst->m_Camera_LastSpeedPercent,SpeedPercent,Min(CAMERA_SPEEDLERPER * dT * _pWPhysState->GetGameTicksPerSecond(),1.0f));
				fp32 CamDistanceH = CAMERA_MAXDISTANCEHEAD*SpeedPercent;
				fp32 CamDistanceP = CAMERA_MAXDISTANCEPITCH*SpeedPercent;
				if (FloatIsInvalid(pCDFirst->m_Camera_LastPosition[0]) || FloatIsInvalid(pCDFirst->m_Camera_LastPosition[1]) || FloatIsInvalid(pCDFirst->m_Camera_LastPosition[2]))
					ResultingPos = IdealPosition;
				else if (pCDFirst->m_LastTeleportTick == pCDFirst->m_GameTick || (pCD->m_MountedMode & (PLAYER_MOUNTEDMODE_FLAG_TURRET|PLAYER_MOUNTEDMODE_FLAG_BLENDANIM|PLAYER_MOUNTEDMODE_FLAG_BLENDOUTANIM)))
					ResultingPos = IdealPosition;
				else
					ResultingPos =  LERP(pCDFirst->m_Camera_LastPosition, IdealPosition, Min(CAMERA_FILTERSPEED * dT * _pWPhysState->GetGameTicksPerSecond(),1.0f));

				CVec3Dfp32 ResHead = ResultingPos;
				ResHead[2] = 0;
				CVec3Dfp32 IdealHead = IdealPosition;
				IdealHead[2] = 0;
				fp32 Length = ResHead.Distance(IdealHead);
				if(Length > CamDistanceH)
				{
					CVec3Dfp32 Dir = (ResHead - IdealHead) / Length;
					ResHead = IdealHead + Dir*CamDistanceH;
				}
				if(M_Fabs(ResultingPos[2] - IdealPosition[2]) > CamDistanceP)
					ResHead[2] = IdealPosition[2] + Sign(ResultingPos[2] - IdealPosition[2]) * CamDistanceP;
				else
					ResHead[2] = ResultingPos[2];
				ResultingPos = ResHead;
			}

			if (!(pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_BLENDANIM))
			{
				CCollisionInfo Info;
				CVec3Dfp32 Source = CVec3Dfp32::GetRow(_pCamObj->GetPositionMatrix(), 3);
				if(CWObject_Character::Char_GetPhysType(_pCamObj) == PLAYER_PHYS_CROUCH)
					Source[2] = Min(Source[2] + 10.0f, ResultingPos[2]);
				else
					Source[2] = ResultingPos[2];

				Info.m_ReturnValues = CXR_COLLISIONRETURNVALUE_TIME;
				bool bHit = _pWPhysState->Phys_IntersectLine(Source, ResultingPos + (ResultingPos - Source).Normalize() * 12, 0, OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PLAYERPHYSMODEL,
					XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_CAMERASOLID | /*XW_MEDIUM_PLAYERSOLID*/XW_MEDIUM_DYNAMICSSOLID | XW_MEDIUM_GLASS,
					&Info);
				if(bHit && Info.m_bIsValid)
					ResultingPos = Source + (ResultingPos - Source) * Info.m_Time;
			}

			pCDFirst->m_Camera_LastSpeedPercent = SpeedPercent;
			pCDFirst->m_Camera_LastCalcTime = Time;
			pCDFirst->m_Camera_LastPosition = ResultingPos;
			pCDFirst->m_Camera_CharacterOffset = ResultingPos - IdealPosition;
		}

		Camera.GetRow(3) = ResultingPos;
		Camera.GetRow(2) = Trans.GetRow(0);
		Camera.GetRow(0) = -Trans.GetRow(1);
		Camera.RecreateMatrix(2,0);
	}
}
