#include "PCH.h"

#include "WObj_ControlClass.h"
#include "WObj_CharClientData2.h"
#include "WObj_Char2.h"

#include "Models/CSinRandTable.h"
// Definitions used in Camera_Trace
#define MAX_ROTATIONS	5
#define ROTATION_ANGLE	0.05f
#define CAMTRACE_RINGS  4
#define CAMTRACE_SIDES  4


#define PLAYER_CAMERADISTANCECHANGE_DEAD 30

#define	M_PI		3.14159265358979323846f	/* pi */


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
} \

void CWObject_ControlClassPacked::Pack(const CWObject_ControlClass& _Control, uint8*& _pBuffer)
{
	// Make controltype cache
	//uint8* pBufferCache = _pBuffer;
	PTR_PUTUINT8(_pBuffer,_Control.GetControlType());
	_Control.SPack(_pBuffer);
	/*int32 Size = _pBuffer - pBufferCache;
	m_lData.SetLen(Size);
	uint8* pArray = m_lData.GetBasePtr();
	for (int32 i = 0; i < Size; i++)
		pArray[i] = pBufferCache[i];*/
}

void CWObject_ControlClassPacked::Pack(const CWObject_ControlClass& _Control)
{
	// Make controltype cache
	uint8 Buffer[2048];
	uint8* pBuffer = Buffer;
	m_ControlType = _Control.GetControlType();
	m_ActivateTime = _Control.GetActivateTime();
	PTR_PUTINT8(pBuffer,m_ControlType);
	_Control.SPack(pBuffer);
	int32 Size = pBuffer - Buffer;
	m_lData.SetLen(Size);
	uint8* pArray = m_lData.GetBasePtr();
	for (int32 i = 0; i < Size; i++)
		pArray[i] = Buffer[i];
}

void CWObject_ControlClassPacked::MakeDiff(const CWObject_ControlClassPacked& _Ref, const CWObject_ControlClass& _Control, uint8*& _pBuffer)
{
	uint8* pBufferCache = _pBuffer;
	PTR_PUTINT8(_pBuffer,_Control.GetControlType());
	const uint8* pRef = _Ref.m_lData.GetBasePtr();
	// Remove controltype from buffer
	pRef++;
	_Control.MakeDiff(pRef,_pBuffer);
	// Skip copying when making diff
	/*int32 Size = _pBuffer - pBufferCache;
	m_lData.SetLen(Size);
	uint8* pArray = m_lData.GetBasePtr();
	for (int32 i = 0; i < Size; i++)
		pArray[i] = pBufferCache[i];*/
}

int8 CWObject_ControlClassPacked::GetControlTypeFromBuffer(const uint8* _pBuffer)
{
	PTR_GETINT8(_pBuffer,m_ControlType);
	return m_ControlType;
}

void CWObject_ControlClassPacked::UpdateWithDiff(CWObject_ControlClass& _Control, const uint8*& _pBuffer)
{
	const uint8* pBufferCache = _pBuffer;
	if (!pBufferCache)
		return;

	PTR_GETINT8(_pBuffer,m_ControlType);
	// Just unpack as usual
	_Control.SUnpack(_pBuffer);
	// Update controltype cache
	//m_ControlType = _Control.GetControlType();
	m_ActivateTime = _Control.GetActivateTime();
	int32 Size = _pBuffer - pBufferCache;
	m_lData.SetLen(Size);
	uint8* pArray = m_lData.GetBasePtr();
	for (int32 i = 0; i < Size; i++)
		pArray[i] = pBufferCache[i];
}

// Pack stuff on server (agi like..)
void CWObject_ControlClass::OnCreateClientUpdate()
{

}
// Unpack stuff on client
void CWObject_ControlClass::OnClientUpdate()
{

}

CWObject_ControlClass::CWObject_ControlClass()
{
	m_ControlType = CControlHandler::CONTROLHANDLER_TYPE_UNKNOWN;
}

void CWObject_ControlClass::SCopyFrom(const void* _pFrom)
{

}

void CWObject_ControlClass::Clear()
{

}

void CWObject_ControlClass::SPack(uint8 *&_pD) const
{
	MAUTOSTRIP(CWObject_ControlClass, MAUTOSTRIP_VOID);
	uint8 &PackedFlag = _pD[0];
	PackedFlag = 0;
	_pD++;
	uint32 Val = 0;
	MACRO_PACK_INT8(CONTROLCLASS_PACKFLAGS_CONTROLTYPE,m_ControlType);
	MACRO_PACK_CMTIME(CONTROLCLASS_PACKFLAGS_ACTIVATETIME,m_ActivateTime);
}

void CWObject_ControlClass::MakeDiff(const uint8 *&_pDRef, uint8 *&_pD) const
{
	MAUTOSTRIP(CWObject_ControlClass, MAUTOSTRIP_VOID);
	uint8 PackedFlagRef;
	uint8& PackedFlag = _pD[0];
	PackedFlag = 0;
	_pD++;
	PTR_GETUINT8(_pDRef,PackedFlagRef);
	//uint32 Val = 0;
	MACRO_DIFF_INT8(CONTROLCLASS_PACKFLAGS_CONTROLTYPE,m_ControlType);
	MACRO_DIFF_CMTIME(CONTROLCLASS_PACKFLAGS_ACTIVATETIME,m_ActivateTime);
}

void CWObject_ControlClass::SUnpack(const uint8 *&_pD)
{
	MAUTOSTRIP(CWObject_ControlClass, MAUTOSTRIP_VOID);
	uint8 PackedFlag;
	PTR_GETUINT8(_pD,PackedFlag);
	uint32 Val = 0;
	MACRO_UNPACK_INT8(CONTROLCLASS_PACKFLAGS_CONTROLTYPE,m_ControlType);
	MACRO_UNPACK_CMTIME(CONTROLCLASS_PACKFLAGS_ACTIVATETIME,m_ActivateTime);
}

CWObject_ControlCamera::CWObject_ControlCamera()
{
	Clear();
//	m_ControlType = CControlHandler::CONTROLHANDLER_TYPE_CAMERA;
}

void CWObject_ControlCamera::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
 const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	switch (_KeyHash)
	{
	case MHASH6('CAME','RA_S','TAND','_HEA','D_OF','FSET'): // "CAMERA_STAND_HEAD_OFFSET"
		{
			CVec3Dfp32 offset;
			CStr Value = KeyValue;
			offset.k[0] = Value.Getfp64Sep(",");
			offset.k[1] = Value.Getfp64Sep(",");
			//			offset.k[2] = Min((fp32)_Value.Getfp64Sep(","), (fp32)PLAYER_MAX_STAND_HEAD_HEIGHT);
			offset.k[2] = Value.Getfp64Sep(",");
			m_Camera_StandHeadOffset = offset;
			break;
		}
	case MHASH7('CAME','RA_C','ROUC','H_HE','AD_O','FFSE','T'): // "CAMERA_CROUCH_HEAD_OFFSET"
		{
			CVec3Dfp32 offset;
			CStr Value = KeyValue;
			offset.k[0] = Value.Getfp64Sep(",");
			offset.k[1] = Value.Getfp64Sep(",");
			//			offset.k[2] = Min((fp32)_Value.Getfp64Sep(","), (fp32)PLAYER_MAX_CROUCH_HEAD_HEIGHT);
			offset.k[2] = Value.Getfp64Sep(",");
			m_Camera_CrouchHeadOffset = offset;
			break;
		}
	case MHASH7('CAME','RA_B','EHIN','D_HE','AD_O','FFSE','T'): // "CAMERA_BEHIND_HEAD_OFFSET"
		{
			CStr Value = KeyValue;
			CVec3Dfp32 offset;
			offset.k[0] = Value.Getfp64Sep(",");
			offset.k[1] = Value.Getfp64Sep(",");
			offset.k[2] = Value.Getfp64Sep(",");
			m_Camera_BehindOffset = offset;
			break;
		}
	case MHASH6('CAME','RA_S','HOUL','DER_','OFFS','ET'): // "CAMERA_SHOULDER_OFFSET"
		{
			CStr Value = KeyValue;
			CVec3Dfp32 offset;
			offset.k[0] = Value.Getfp64Sep(",");
			offset.k[1] = Value.Getfp64Sep(",");
			offset.k[2] = Value.Getfp64Sep(",");
			m_Camera_ShoulderOffset = offset;
			break;
		}
	default:
		{
			CWObject_ControlCameraParent::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_ControlCamera::Clear()
{
	CWObject_ControlCameraParent::Clear();
	m_Camera_CurHeadOffset = 0.0f;
	m_Camera_CurHeadOffsetChange = 0.0f;
	m_Camera_LastHeadOffset = 0.0f;
	m_Camera_CurBehindOffset = 0.0f;
	m_Camera_CurBehindOffsetChange = 0.0f;
	m_Camera_LastBehindOffset = 0.0f;

	m_ChaseCamDistance = 0.0f;
	m_dChaseCamDistance = 0.0f;
	m_LastChaseCamDistance = 0.0f;

	m_CameraShake_LastMomentum = 0.0f;
	m_CameraShake_CurMomentum = 0.0f;
	m_CameraShake_CurBlend = 0.0f;
	m_CameraShake_LastBlend = 0.0f;
	m_CameraShake_UpdateDelayTicks = 0;
	m_CameraShake_Randseed = 0;
	m_CameraShake_Flags = 0;

	m_fTraceHistory = 0.0f;
	m_fOldTraceHistory = 0.0f;
	m_iRotationIndex = 0.0f;

	m_Camera_StandHeadOffset = 0.0f;
	m_Camera_CrouchHeadOffset = 0.0f;
	m_Camera_BehindOffset = 0.0f;
	m_Camera_ShoulderOffset = 0.0f;
}

MACRO_IMPLEMENT_CONTROLTYPE(ControlCamera,CWObject_ControlCamera::CreateMe)

spCWObject_ControlClass CWObject_ControlCamera::CreateMe()
{
	return MNew(CWObject_ControlCamera);
}

void CWObject_ControlCamera::SCopyFrom(const void* _pFrom)
{
	CWObject_ControlCameraParent::SCopyFrom(_pFrom);
	const CWObject_ControlCamera& From = *(const CWObject_ControlCamera*)_pFrom;
	m_Camera_CurHeadOffset = From.m_Camera_CurHeadOffset;
	m_Camera_CurHeadOffsetChange = From.m_Camera_CurHeadOffsetChange;
	m_Camera_LastHeadOffset = From.m_Camera_LastHeadOffset;
	m_Camera_CurBehindOffset = From.m_Camera_CurBehindOffset;
	m_Camera_CurBehindOffsetChange = From.m_Camera_CurBehindOffsetChange;
	m_Camera_LastBehindOffset = From.m_Camera_LastBehindOffset;

	m_ChaseCamDistance = From.m_ChaseCamDistance;
	m_dChaseCamDistance = From.m_dChaseCamDistance;
	m_LastChaseCamDistance = From.m_LastChaseCamDistance;

	// Camera shake settings (needed ???)
	m_CameraShake_LastMomentum = From.m_CameraShake_LastMomentum;
	m_CameraShake_CurMomentum = From.m_CameraShake_CurMomentum;
	m_CameraShake_CurBlend = From.m_CameraShake_CurBlend;
	m_CameraShake_LastBlend = From.m_CameraShake_LastBlend;
	m_CameraShake_UpdateDelayTicks = From.m_CameraShake_UpdateDelayTicks;
	m_CameraShake_Randseed = From.m_CameraShake_Randseed;
	m_CameraShake_Flags = From.m_CameraShake_Flags;

	m_fTraceHistory = From.m_fTraceHistory;
	m_fOldTraceHistory = From.m_fOldTraceHistory;
	m_iRotationIndex = From.m_iRotationIndex;

	m_Camera_StandHeadOffset = From.m_Camera_StandHeadOffset;
	m_Camera_CrouchHeadOffset = From.m_Camera_CrouchHeadOffset;
	m_Camera_BehindOffset = From.m_Camera_BehindOffset;
	m_Camera_ShoulderOffset = From.m_Camera_ShoulderOffset;

	m_CameraShake_Amplitude = 0.0f;
	m_CameraShake_Speed = 0.0f;
	m_CameraShake_DurationTicks = 0.0f;
	m_CameraShake_StartTick = 0.0f;
}

void CWObject_ControlCamera::SPack(uint8 *&_pD) const
{
	MAUTOSTRIP(CWObject_ControlCamera_Pack, MAUTOSTRIP_VOID);
	CWObject_ControlCameraParent::SPack(_pD);

	// Flag with what's packed and not... (if != 0 -> Add and pack..)
	uint8 &PackedFlag = _pD[0];
	PackedFlag = 0;
	_pD++;
	uint32 Val = 0;
	MACRO_PACK_VEC3(CONTROLCAMERA_PACKFLAGS_STANDHEADOFFSET,m_Camera_StandHeadOffset);
	MACRO_PACK_VEC3(CONTROLCAMERA_PACKFLAGS_CROUCHHEADOFFSET,m_Camera_CrouchHeadOffset);
	MACRO_PACK_VEC3(CONTROLCAMERA_PACKFLAGS_BEHINDOFFSET,m_Camera_BehindOffset);
	MACRO_PACK_VEC3(CONTROLCAMERA_PACKFLAGS_SHOULDEROFFSET,m_Camera_ShoulderOffset);
	MACRO_PACK_FP32(CONTROLCAMERA_PACKFLAGS_AMPLITUDE,m_CameraShake_Amplitude);
	MACRO_PACK_FP32(CONTROLCAMERA_PACKFLAGS_SPEED,m_CameraShake_Speed);
	MACRO_PACK_UINT32(CONTROLCAMERA_PACKFLAGS_DURATIONTICKS,m_CameraShake_DurationTicks);
	MACRO_PACK_UINT32(CONTROLCAMERA_PACKFLAGS_STARTTICK,m_CameraShake_StartTick);
}

void CWObject_ControlCamera::MakeDiff(const uint8 *&_pDRef, uint8 *&_pD) const
{
	MAUTOSTRIP(CWObject_ControlCamera_MakeDiff, MAUTOSTRIP_VOID);
	CWObject_ControlCameraParent::MakeDiff(_pDRef,_pD);
	// Check with ref what's different with current version, add items not there (or changed)
	// to new version
	// Flag with what's packed and not... (if != 0 -> Add and pack..)
	uint8 PackedFlagRef;
	uint8& PackedFlag = _pD[0];
	PackedFlag = 0;
	_pD++;
	PTR_GETUINT8(_pDRef,PackedFlagRef);
	uint32 Val = 0;
	MACRO_DIFF_VEC3(CONTROLCAMERA_PACKFLAGS_STANDHEADOFFSET,m_Camera_StandHeadOffset);
	MACRO_DIFF_VEC3(CONTROLCAMERA_PACKFLAGS_CROUCHHEADOFFSET,m_Camera_CrouchHeadOffset);
	MACRO_DIFF_VEC3(CONTROLCAMERA_PACKFLAGS_BEHINDOFFSET,m_Camera_BehindOffset);
	MACRO_DIFF_VEC3(CONTROLCAMERA_PACKFLAGS_SHOULDEROFFSET,m_Camera_ShoulderOffset);
	MACRO_DIFF_FP32(CONTROLCAMERA_PACKFLAGS_AMPLITUDE,m_CameraShake_Amplitude);
	MACRO_DIFF_FP32(CONTROLCAMERA_PACKFLAGS_SPEED,m_CameraShake_Speed);
	MACRO_DIFF_UINT32(CONTROLCAMERA_PACKFLAGS_DURATIONTICKS,m_CameraShake_DurationTicks);
	MACRO_DIFF_UINT32(CONTROLCAMERA_PACKFLAGS_STARTTICK,m_CameraShake_StartTick);
}

void CWObject_ControlCamera::SUnpack(const uint8 *&_pD)
{
	MAUTOSTRIP(CWObject_ControlCamera_Unpack, MAUTOSTRIP_VOID);
	CWObject_ControlCameraParent::SUnpack(_pD);
	
	// Flag with what's packed and not... (if != 0 -> Add and pack..)
	uint8 PackedFlag;
	PTR_GETUINT8(_pD,PackedFlag);
	uint32 Val = 0;
	MACRO_UNPACK_VEC3(CONTROLCAMERA_PACKFLAGS_STANDHEADOFFSET,m_Camera_StandHeadOffset);
	MACRO_UNPACK_VEC3(CONTROLCAMERA_PACKFLAGS_CROUCHHEADOFFSET,m_Camera_CrouchHeadOffset);
	MACRO_UNPACK_VEC3(CONTROLCAMERA_PACKFLAGS_BEHINDOFFSET,m_Camera_BehindOffset);
	MACRO_UNPACK_VEC3(CONTROLCAMERA_PACKFLAGS_SHOULDEROFFSET,m_Camera_ShoulderOffset);
	MACRO_UNPACK_FP32(CONTROLCAMERA_PACKFLAGS_AMPLITUDE,m_CameraShake_Amplitude);
	MACRO_UNPACK_FP32(CONTROLCAMERA_PACKFLAGS_SPEED,m_CameraShake_Speed);
	MACRO_UNPACK_UINT32(CONTROLCAMERA_PACKFLAGS_DURATIONTICKS,m_CameraShake_DurationTicks);
	MACRO_UNPACK_UINT32(CONTROLCAMERA_PACKFLAGS_STARTTICK,m_CameraShake_StartTick);
}

int32 CWObject_ControlCamera::Update(CWObject_CoreData* _pChar, CWorld_PhysState* _pWPhysState, CWObject_ControlClass* _pPrev)
{
	CWO_Character_ClientData2 *pCD = CWObject_Character2::GetClientData(_pChar);
	if (!pCD) return 0;

	MSCOPE(RefreshCamera, CHARACTER);

	// Get cameramode
	const int PhysType = CWObject_Character2::Char_GetPhysType(_pChar);
	int CameraMode = GetCameraMode(pCD, PhysType);

	bool bRangedWeapon = (pCD->m_Item0_Flags & RPG_ITEM_FLAGS_AUTOAIM) || (pCD->m_Item0_Flags & RPG_ITEM_FLAGS_AIMING);

	// ----------------------------------------------
	// Wanted settings
	CVec3Dfp32	WantedHeadOffset;
	CVec3Dfp32	WantedBehindOffset;
	fp32			WantedWallTiltBlend;
	fp32			WantedWallTiltAngle;

	WantedWallTiltAngle		= -0.2f;
	WantedWallTiltBlend		= 0.f;
	WantedHeadOffset		= (CWObject_Character2::Char_GetPhysType(_pChar) == PLAYER_PHYS_CROUCH) ? (CVec3Dfp32)m_Camera_CrouchHeadOffset : (CVec3Dfp32)m_Camera_StandHeadOffset;

	if (m_Camera_BehindOffset[0] > 490)
		m_Camera_BehindOffset = 0;
	if (m_Camera_ShoulderOffset[0] > 490)
		m_Camera_ShoulderOffset = 0;

	switch (CameraMode)
	{
	case PLAYER_CAMERAMODE_FIRSTPERSON: 
		WantedBehindOffset = 0;
		break;

	case PLAYER_CAMERAMODE_NEAR: 
		WantedBehindOffset = (CVec3Dfp32)m_Camera_ShoulderOffset;
		break;

	case PLAYER_CAMERAMODE_NORMAL:
		WantedBehindOffset = (CVec3Dfp32)m_Camera_BehindOffset;
		break;

	case PLAYER_CAMERAMODE_FAR: 
		WantedBehindOffset = (CVec3Dfp32)m_Camera_BehindOffset * PLAYER_CAMERAMODE_FAR_BEHINDSCALE;
		break;
	}

	// ----------------------------------------------
	// Camera base
	CMat43fp32 CameraMatrix = _pChar->GetPositionMatrix();

	CameraMatrix.RotX_x_M(-0.25f);
	CameraMatrix.RotY_x_M(0.25f);
	Camera_GetHeadPos(_pChar).AddMatrixRow(CameraMatrix, 3);

	// ----------------------------------------------
	// Trace to see if we need to zoom in on the char
	fp32 WantedDistance, MaxDistance;

	if (CameraMode != PLAYER_CAMERAMODE_FIRSTPERSON)
		Camera_Trace(_pChar, _pWPhysState, bRangedWeapon, CameraMatrix, WantedBehindOffset, WantedDistance, MaxDistance);
	else
		WantedDistance = 0;

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
	m_Camera_LastHeadOffset = m_Camera_CurHeadOffset;
	m_Camera_LastBehindOffset = m_Camera_CurBehindOffset;
	ModerateVec3f(m_Camera_CurHeadOffset, WantedHeadOffset, m_Camera_CurHeadOffsetChange, PLAYER_CAMERACHANGE);
	ModerateVec3f(m_Camera_CurBehindOffset, WantedBehindOffset, m_Camera_CurBehindOffsetChange, PLAYER_CAMERADISTANCECHANGE);


	// ----------------------------------------------
	// Chasecam distance
	m_LastChaseCamDistance	= m_ChaseCamDistance;
	WantedDistance				*= 100.0f;
	m_ChaseCamDistance		*= 100.0f;
	m_dChaseCamDistance	*= 100.0f;
	Moderatef(m_ChaseCamDistance, WantedDistance, m_dChaseCamDistance, PLAYER_CAMERADISTANCECHANGE);
	m_ChaseCamDistance		*= 1.f / 100.0f;
	m_dChaseCamDistance	*= 1.f / 100.0f;
	m_ChaseCamDistance		= Min(MaxDistance, m_ChaseCamDistance);

	//ConOut(CStrF("%s: WantedDistance: %f ChaseCamDistance: %f", _pWPhysState->IsServer() ? "S":"C",WantedDistance,m_ChaseCamDistance));

	return 1;
}

void CWObject_ControlCamera::Camera_Get(CWorld_PhysState* _pWPhysState, CMat43fp32& _Camera, CWObject_CoreData* _pCamObj, fp32 _IPTime)
{
	MSCOPE(GetCamera, CHARACTER);

	// Offset the camera to around the head....
	CVec3Dfp32 CameraPos = CVec3Dfp32::GetMatrixRow(_Camera, 3);

	CWO_Character_ClientData2* pCD = CWObject_Character2::GetClientData(_pCamObj);

	//	CRegistry* pUserReg = _pWPhysState->Registry_GetUser();
	//	int bThirdPerson = (pUserReg ? pUserReg->GetValuei("THIRDPERSON") : 0);
	bool bThirdPerson = (pCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
	if(_pCamObj->m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR)
		_Camera.k[3][2] += _pCamObj->GetPhysState().m_Prim[0].GetDim()[2];
	else if (bThirdPerson && !(CWObject_Character2::Char_GetPhysType(_pCamObj) == PLAYER_PHYS_NOCLIP))
	{
		_Camera.RotX_x_M(-0.25f);
		_Camera.RotY_x_M(0.25f);

		CVec3Dfp32::GetMatrixRow(_Camera, 3) += Camera_GetHeadPos(_pCamObj);
		Camera_ShakeItBaby(_pWPhysState, _Camera, pCD, _IPTime);

		Camera_Offset(_pCamObj, _pWPhysState, _Camera, _IPTime);
	}
	else if (!(CWObject_Character2::Char_GetPhysType(_pCamObj) == PLAYER_PHYS_NOCLIP))
	{
		// First person camera
		CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(_pCamObj->m_iModel[0]);
		CXR_Model* pModel1 = _pWPhysState->GetMapData()->GetResource_Model(_pCamObj->m_iModel[1]);
		CXR_Skeleton *pSkel1 = pModel1 ? pModel1->GetSkeleton() : NULL;
		CXR_Skeleton *pSkel0 = pModel ? pModel->GetSkeleton() : NULL;

		CXR_Skeleton *pSkel = pSkel1;
		if(!pSkel || (pSkel0 && pSkel0->m_lNodes.Len() > pSkel->m_lNodes.Len()))
			pSkel = pSkel0;			

		CXR_AnimState Anim;
		if (!CWObject_Character2::OnGetAnimState(_pCamObj, _pWPhysState, pSkel, 0, _Camera/*_pCamObj->GetPositionMatrix()*/, _IPTime, Anim, NULL))
			return;

		CXR_SkeletonInstance* pSkelInst = Anim.m_pSkeletonInst;

		if (!pModel)
		{
			_Camera.RotX_x_M(-0.25f);
			_Camera.RotY_x_M(0.25f);
			return;
		}

		if (!pSkel)
		{
			_Camera.RotX_x_M(-0.25f);
			_Camera.RotY_x_M(0.25f);
			return;
		}


		CMat43fp32_MP Trans;
		CVec3Dfp32 Point = 0;

		bool bOk = false;
		if(pCD->m_Aim_SkeletonType == 3)
		{
			const CXR_SkeletonAttachPoint* pPoint = pSkel->GetAttachPoint(4);
			if (pPoint)
				Point = pPoint->m_LocalPos;
			//Point += CVec3Dfp32(-8,4,12);

			int RotTrack = 1;
			if(pSkelInst->m_lBoneTransform.ValidPos(RotTrack))
			{
				Trans = pSkelInst->GetBoneTransform(RotTrack);
				bOk = true;
			}

			CVec3Dfp32::GetRow(Trans, 3) = Point * Trans;
			Point = 0;

			RotTrack = 14;
			if(pSkelInst->m_lBoneTransform.ValidPos(RotTrack))
			{
				CMat43fp32_MP Mat2 = pSkelInst->GetBoneLocalPos(RotTrack);
				CMat43fp32_MP Res;
				Mat2.Multiply(Trans, Res);
				CVec3Dfp32::GetRow(Trans, 0) = CVec3Dfp32::GetRow(Res, 0);
				CVec3Dfp32::GetRow(Trans, 1) = CVec3Dfp32::GetRow(Res, 1);
				CVec3Dfp32::GetRow(Trans, 2) = CVec3Dfp32::GetRow(Res, 2);
			}
		}
		else if(pCD->m_Aim_SkeletonType == 5)
		{
			const CXR_SkeletonAttachPoint* pPoint = pSkel->GetAttachPoint(0);
			if (pPoint)
				Point = pPoint->m_LocalPos;

			int RotTrack = 3;
			if(pSkelInst->m_lBoneTransform.ValidPos(RotTrack))
			{
				Trans = pSkelInst->GetBoneTransform(RotTrack);
				bOk = true;
			}
		}
		else
		{
			int RotTrack = PLAYER_ROTTRACK_CAMERA;
			if(pSkelInst->m_lBoneTransform.ValidPos(RotTrack))
			{
				Trans = pSkelInst->GetBoneTransform(RotTrack);
				bOk = true;
			}

			CVec3Dfp32 Point(0);

			const CXR_SkeletonAttachPoint* pPoint = pSkel->GetAttachPoint(PLAYER_ATTACHPOINT_FACE);
			if (pPoint)
				Point = pPoint->m_LocalPos;

		}

		if(!bOk)
			Trans.Unit();

		CVec3Dfp32 IdealPosition = Point * Trans;

		CVec3Dfp32 ResultingPos = IdealPosition;

		CVec3Dfp32::GetMatrixRow(_Camera, 3) = ResultingPos;
		CVec3Dfp32::GetMatrixRow(Trans, 0).SetMatrixRow(_Camera, 2);
		CVec3Dfp32 Side = CVec3Dfp32(0) - CVec3Dfp32::GetMatrixRow(Trans, 1);
		Side.SetMatrixRow(_Camera, 0);
		_Camera.RecreateMatrix(2,0);
	}
	else
	{
		CVec3Dfp32::GetMatrixRow(_Camera, 3) += Camera_GetHeadPos(_pCamObj);
		_Camera.RotX_x_M(-0.25f);
		_Camera.RotY_x_M(0.25f);
	}
}

int32 CWObject_ControlCamera::DoCamera(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMat43fp32& _Camera)
{
	MSCOPE(OnGetCamera, CHARACTER);

	fp32 IPTime = _pWClient->GetRenderTickFrac();
	fp32 IPTime2 = IPTime;

	//ConOutL(CStrF("GameTick %d, IpTime %f, RealTime %f", _pWClient->GetGameTick(), IPTime, GetCPUClock() / GetCPUFrequency() ));

	// Interpolate direction and position for player-object.
	_pWClient->Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), _Camera, IPTime );

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
		bool bLag = (TCurrent - _pWClient->GetLastSnapshotTime()).GetTime() * _pWClient->GetTimeScale() > 2.0f;

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
			int a = 1;	// Just so I can break here
		}
		else
		{
#ifndef WCLIENT_NOPREDICTION
			TProfileDef(TPredict);
			{
				TMeasureProfile(TPredict);
				{
					CWC_Player* pP = _pWClient->Player_GetLocal();

					((CWorld_ClientCore*)_pWClient)->Con_Cmd(127);
					CWObject_Character2::OnClientPredict(_pObj, _pWClient, 0, 0, 1);

					//				if (bPut) pP->m_spCmdQueue->DelHead();
				}

				pCamObj = CWObject_Character2::Player_GetLastValidPrediction(_pObj);
				IPTime = CWObject_Character2::GetClientData(pCamObj)->m_PredictFrameFrac;

				//pCamObj = pCamObj->GetNext();
			}
			//			ConOutL(CStrF("Time %f", GetCPUClock()/GetCPUFrequency()) +  T_String("Predict", TPredict));
#endif

			// Removed by Mondelore (FIXME: Not 100% sure if I may change this =).
			if(CWObject_Character2::Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
			{
				// Control look is not correct if dead
				_Camera = pCamObj->GetPositionMatrix();
			}
			else
			{
				CWObject_Character2::GetClientData(pCamObj)->m_Control_Look.CreateMatrixFromAngles(0, _Camera);
				CVec3Dfp32::GetMatrixRow(pCamObj->GetPositionMatrix(), 3).AddMatrixRow(_Camera, 3);
			}
		}
	}

	{

		CWO_Character_ClientData2 *pCD = CWObject_Character2::GetClientData(pCamObj);
		if (pCD)
		{
			{
				CWObject_CoreData* pObjFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
				if (pObjFirst)
				{
					CWO_Character_ClientData2 *pCDFirst = CWObject_Character2::GetClientData(pObjFirst);
					if (pCDFirst)
					{
						int GameTick = _pWClient->GetGameTick();
						int PMT = pCDFirst->m_PredMiss_Tick;
						fp32 Frac = _pWClient->GetRenderTickFrac();
						fp32 IPPredMiss = 1.0f - Clamp01(fp32(GameTick - PMT) + Frac);
						//						if (IPPredMiss < 0.0f)
						CVec3Dfp32 PredMissAdd;
						pCDFirst->m_PredMiss_dPos.Scale(IPPredMiss, PredMissAdd);

						fp32 OldCamY = _Camera.k[3][2];
						CVec3Dfp32::GetRow(_Camera, 3) += PredMissAdd;

						//					LogFile(CStrF("Z = %f = %f + %f*%f (%f), Tick %d, PMT %d, LUNT %d, Frac %f, dPos %s", 
						//						Camera.k[3][2], OldCamY, pCDFirst->m_PredMiss_dPos[2], IPPredMiss, (pCDFirst->m_PredMiss_dPos[2] * IPPredMiss), _pWClient->GetGameTick(), PMT, pCDFirst->m_LastUpdateNumTicks, _pWClient->GetRenderTickFrac(), PredMissAdd.GetString().Str()));
					}
				}
			}


			Camera_Get(_pWClient, _Camera, pCamObj, IPTime);

			// Do some camera rotation. Object's forward vector is the matrix's x-axis, but for rendering the forward-vector needs to be z
			//			Camera.RotX_x_M(-0.25f);
			//			Camera.RotY_x_M(0.25f);
		}
		else
		{
			_Camera.RotX_x_M(-0.25f);
			_Camera.RotY_x_M(0.25f);
		}
	}

	//Camera_ShakeItBaby(_pWClient, Camera, pCD, IPTime2);
	return 1;
}

void CWObject_ControlCamera::Camera_Offset(CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, CMat43fp32& _Camera, fp32 _IPTime)
{
	MSCOPE(Camera_ThirdPerson_Offset, CHARACTER);

	/*CWO_Character_ClientData2* pCD = GetClientData(_pObj);
	if (pCD == NULL)
		return;*/

	/*bool DebugRender = false;
	fp32 DebugDuration = 90.0f;*/

	CVec3Dfp32 Head = CVec3Dfp32::GetMatrixRow(_Camera, 3);
	CVec3Dfp32 Behind = LERP(m_Camera_LastBehindOffset, m_Camera_CurBehindOffset, _IPTime);

	Behind.MultiplyMatrix3x3(_Camera);

	int objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;// | OBJECT_FLAGS_PHYSOBJECT; // Exclude characters
	int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_CAMERASOLID | XW_MEDIUM_PLAYERSOLID;

	bool bHit;
	CCollisionInfo CInfo;

	fp32 Distance = LERP(m_LastChaseCamDistance, m_ChaseCamDistance, _IPTime);

	CVec3Dfp32 NormalOffset = 0.0f;

	//if (DebugRender) _pPhysState->Debug_RenderVertex(Head + Behind * Distance, 0xF000FF00, 50.0f);

	bHit = _pPhysState->Phys_IntersectLine(Head, Head + Behind * Distance, 0, objects, mediums, &CInfo, _pObj->m_iObject);
	if (bHit && CInfo.m_bIsValid)
		Distance = Min(CInfo.m_Time * Distance, Distance);


	// Shrink distance with 5% (can't remember exactly why, but it gets a bit away of any walls).
	Behind *= Distance * 0.95f;

	fp32 BehindDistance = Behind.Length();

	/*if (DebugRender) _pPhysState->Debug_RenderVertex(Head, 0xF0FF00FF, DebugDuration);
	if (DebugRender) _pPhysState->Debug_RenderWire(Head, Head + Behind, 0xF00000FF, DebugDuration);*/

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

		//if (DebugRender) _pPhysState->Debug_RenderWire(Head + Behind, Head + Behind + Adjustment, 0xF0F0F000, DebugDuration);
		if(Adjustment.Length() > 0)
			Behind += Adjustment;
	}

	//if (DebugRender) _pPhysState->Debug_RenderWire(Head, Head + Behind, 0xF000FF00, DebugDuration);

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
}

void CWObject_ControlCamera::Camera_ShakeItBaby(CWorld_PhysState* _pWPhysState, CMat43fp32& _Camera, CWO_Character_ClientData2* _pCD, fp32 _IPTime)
{
	if (_pCD == NULL)
		return;

	// Camera Shake (Shake camera before it's safetraced).
	uint32 ElapsedTicks = _pCD->m_GameTick - m_CameraShake_StartTick;
	if ((m_CameraShake_DurationTicks > 0) && (ElapsedTicks >= 0) && (ElapsedTicks <= m_CameraShake_DurationTicks))
	{

		// Shake the camera
		fp32 iptime = _IPTime;
		CMTime GameTime = CMTime::CreateFromTicks(_pCD->m_GameTick, SERVER_TIMEPERFRAME, iptime);

		// Blend...
		fp32 Time = ((fp32)ElapsedTicks + iptime) * SERVER_TIMEPERFRAME;
		fp32 Duration = (fp32)m_CameraShake_DurationTicks * SERVER_TIMEPERFRAME;
		fp32 BlendInTime = Duration * PLAYER_CAMERASHAKE_BLENDINDURATION;
		fp32 BlendOutTime = Duration * PLAYER_CAMERASHAKE_BLENDOUTDURATION;

		fp32 Blend = GetFade(Time, Duration, BlendInTime, BlendOutTime);

		fp32 ox, oy, oz, sx, sy, sz;
		ox = MFloat_GetRand(m_CameraShake_Randseed + 0) * Duration;
		oy = MFloat_GetRand(m_CameraShake_Randseed + 1) * Duration;
		oz = MFloat_GetRand(m_CameraShake_Randseed + 2) * Duration;
		sx = 0.8f + 4.0f * MFloat_GetRand(m_CameraShake_Randseed + 3);
		sy = 0.8f + 4.0f * MFloat_GetRand(m_CameraShake_Randseed + 4);
		sz = 0.8f + 4.0f * MFloat_GetRand(m_CameraShake_Randseed + 5);

		fp32 Speed = m_CameraShake_Speed * PLAYER_CAMERASHAKE_FREQSCALE;

		CVec3Dfp32 MomentumVector;
		//		MomentumVector[0] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime + ox) * Speed * sx) - 0.5f);
		//		MomentumVector[1] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime + oy) * Speed * sy) - 0.5f);
		//		MomentumVector[2] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime + oz) * Speed * sz) - 0.5f);
		MomentumVector[0] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime - CMTime::CreateFromSeconds((0.5f - ox) * Speed * sx)).GetTimeModulusScaled(Speed * sx, 1.0f)));
		MomentumVector[1] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime - CMTime::CreateFromSeconds((0.5f - oy) * Speed * sy)).GetTimeModulusScaled(Speed * sy, 1.0f)));
		MomentumVector[2] = 0.0f + 1 * (g_SinRandTable.GetRand((GameTime - CMTime::CreateFromSeconds((0.5f - oz) * Speed * sz)).GetTimeModulusScaled(Speed * sz, 1.0f)));

		fp32 Momentum = m_CameraShake_Amplitude * Blend * PLAYER_CAMERASHAKE_SCALE;
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
	}
}

bool CWObject_ControlCamera::Camera_Trace(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, bool _bRangedWeapon, CMat43fp32& _CameraMatrix, CVec3Dfp32& _WantedBehindOffset, 
											 fp32& _WantedDistance, fp32& _MaxDistance, bool _Rotate)
{
	// Debug stuff
	bool bWallHit		= false;
	CMat43fp32 Camera	= _CameraMatrix;

	CWO_Character_ClientData2 *pCD = CWObject_Character2::GetClientData(_pObj);
	if (!pCD) return false;

	// If noclip then return
	if (CWObject_Character2::Char_GetPhysType(_pObj) == PLAYER_PHYS_NOCLIP)
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

	//if (DebugRender) _pWPhysState->Debug_RenderVertex(Head, 0xF0000F0, DebugDuration);

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
		if(_Rotate) segmentfraction += (ROTATION_ANGLE * m_iRotationIndex);
		fp32 a = 0.5f * (segmentfraction * 0.5f * _PI);	
		fp32 cosa = M_Cos(a);
		fp32 sina = M_Sin(a);

		for (int iSide = 0; iSide < CAMTRACE_SIDES; iSide++)
		{
			fp32 ringfraction = ((fp32)iSide / (fp32)CAMTRACE_SIDES); 
			if(_Rotate) ringfraction += (ROTATION_ANGLE * m_iRotationIndex);
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
	int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_CAMERASOLID | XW_MEDIUM_PLAYERSOLID;

	// Create selection, to optimize tracing.
	TSelection<CSelection::LARGE_BUFFER> Selection;
	_pWPhysState->Selection_AddBoundBox(Selection, objects, Box.m_Min, Box.m_Max);

	_MaxDistance = 1.0f; 
	fp32 DistanceHint = _MaxDistance;


	// Trace the precalculated rays
	bHit = _pWPhysState->Phys_IntersectLine(iSelection, Head, Rays[0], 0, objects, mediums, &CInfo);
	if (bHit && CInfo.m_bIsValid)
		_MaxDistance = Min(CInfo.m_Time, _MaxDistance);

	for (int iTraces = 1; iTraces < CAMTRACE_RINGS * CAMTRACE_SIDES; iTraces++)
	{
		bHit = _pWPhysState->Phys_IntersectLine(iSelection, Head, Rays[iTraces], 0, objects, mediums, &CInfo);
		if (bHit && CInfo.m_bIsValid)
			DistanceHint = Min(CInfo.m_Time, DistanceHint);

		/*if (DebugRender) 
		if(bHit && CInfo.m_bIsValid)	_pWPhysState->Debug_RenderWire(Head, Rays[iTraces] * CInfo.m_Time, 0x40400000, DebugDuration);
		else							_pWPhysState->Debug_RenderWire(Head, Rays[iTraces], 0x40004000, DebugDuration);*/
	}

	_pWPhysState->Selection_Pop();

	if(!_Rotate) 
	{
		_WantedDistance = Min(Max(0.0f, DistanceHint), _MaxDistance);
		return bWallHit;
	}

	m_fTraceHistory = Min(DistanceHint, m_fTraceHistory);
	m_iRotationIndex++;

	if(CWObject_Character2::Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
	{
		_WantedDistance = m_fTraceHistory;
	}
	else if(m_iRotationIndex == MAX_ROTATIONS)
	{
		_WantedDistance = Min(Max(0.0f, m_fTraceHistory), _MaxDistance); 
		m_fTraceHistory = _MaxDistance;
		m_fOldTraceHistory = _WantedDistance;
		m_iRotationIndex = 0;
	}
	else
	{
		_WantedDistance = m_fOldTraceHistory;
	}

	return bWallHit;
}

// Physfree
MACRO_IMPLEMENT_CONTROLTYPE(ControlPhysFree,CWObject_ControlPhysFree::CreateMe)

CWObject_ControlPhysFree::CWObject_ControlPhysFree()
{
	Clear();
//	m_ControlType = CControlHandler::CONTROLHANDLER_TYPE_CAMERA;
}

void CWObject_ControlPhysFree::Clear()
{
	CWObject_ControlPhysFreeParent::Clear();
	m_Speed_Forward = 0.0f;
	m_Speed_SideStep = 0.0f;
	m_Speed_Up = 0.0f;
	m_Speed_Jump = 0.0f;
}

spCWObject_ControlClass CWObject_ControlPhysFree::CreateMe()
{
	return MNew(CWObject_ControlPhysFree);
}

void CWObject_ControlPhysFree::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
 const CStr KeyName = _pKey->GetThisName();
	fp32 KeyValuef = (fp32)_pKey->GetThisValue().Val_fp64();
	switch (_KeyHash)
	{
	case MHASH4('SPEE','D_FO','RWAR','D'): // "SPEED_FORWARD"
		{
			m_Speed_Forward = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	case MHASH4('SPEE','D_SI','DEST','EP'): // "SPEED_SIDESTEP"
		{
			m_Speed_SideStep = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	case MHASH2('SPEE','D_UP'): // "SPEED_UP"
		{
			m_Speed_Up = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	case MHASH3('SPEE','D_JU','MP'): // "SPEED_JUMP"
		{
			m_Speed_Jump = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	default:
		{
			return CWObject_ControlPhysFreeParent::OnEvalKey(_pKey);
			break;
		}
	}
}

void CWObject_ControlPhysFree::SCopyFrom(const void* _pFrom)
{
	CWObject_ControlCameraParent::SCopyFrom(_pFrom);
	const CWObject_ControlPhysFree& From = *(const CWObject_ControlPhysFree*)_pFrom;
	m_Speed_Forward = From.m_Speed_Forward;
	m_Speed_SideStep = From.m_Speed_SideStep;
	m_Speed_Up = From.m_Speed_Up;
	m_Speed_Jump = From.m_Speed_Jump;
}

void CWObject_ControlPhysFree::SPack(uint8 *&_pD) const
{
	MAUTOSTRIP(CWObject_ControlPhysFree_Pack, MAUTOSTRIP_VOID);
	CWObject_ControlCameraParent::SPack(_pD);
	// Flag with what's packed and not... (if != 0 -> Add and pack..)
	uint8 &PackedFlag = _pD[0];
	PackedFlag = 0;
	_pD++;
	uint32 Val = 0;
	MACRO_PACK_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_FORWARD,m_Speed_Forward);
	MACRO_PACK_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_SIDESTEP,m_Speed_SideStep);
	MACRO_PACK_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_UP,m_Speed_Up);
	MACRO_PACK_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_JUMP,m_Speed_Jump);
}

void CWObject_ControlPhysFree::MakeDiff(const uint8 *&_pDRef, uint8 *&_pD) const
{
	MAUTOSTRIP(CWObject_ControlPhysFree_MakeDiff, MAUTOSTRIP_VOID);
	CWObject_ControlCameraParent::MakeDiff(_pDRef,_pD);
	// Check with ref what's different with current version, add items not there (or changed)
	// to new version
	// Flag with what's packed and not... (if != 0 -> Add and pack..)
	uint8 PackedFlagRef;
	uint8& PackedFlag = _pD[0];
	PackedFlag = 0;
	_pD++;
	PTR_GETUINT8(_pDRef,PackedFlagRef);
	uint32 Val = 0;
	MACRO_DIFF_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_FORWARD,m_Speed_Forward);
	MACRO_DIFF_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_SIDESTEP,m_Speed_SideStep);
	MACRO_DIFF_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_UP,m_Speed_Up);
	MACRO_DIFF_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_JUMP,m_Speed_Jump);
}

void CWObject_ControlPhysFree::SUnpack(const uint8 *&_pD)
{
	MAUTOSTRIP(CWObject_ControlPhysFree_Unpack, MAUTOSTRIP_VOID);
	CWObject_ControlCameraParent::SUnpack(_pD);
	// Flag with what's packed and not... (if != 0 -> Add and pack..)
	uint8 PackedFlag;
	PTR_GETUINT8(_pD,PackedFlag);
	uint32 Val = 0;
	MACRO_UNPACK_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_FORWARD,m_Speed_Forward);
	MACRO_UNPACK_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_SIDESTEP,m_Speed_SideStep);
	MACRO_UNPACK_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_UP,m_Speed_Up);
	MACRO_UNPACK_FP32(CONTROLPHYSFREE_PACKFLAGS_SPEED_JUMP,m_Speed_Jump);
}

int32 CWObject_ControlPhysFree::Update(CWObject_CoreData* _pChar, CWorld_PhysState* _pWPhys, CWObject_ControlClass* _pPrev)
{
	return 0;
}

#define PLAYER_PHYS_COS_MAX_SLOPE 0.69465837045899728665640629942269f // 46 degrees
int32 CWObject_ControlPhysFree::DoPhysics(int _iSel, CWObject_CoreData* _pObj, CWObject_Character2* _pChar, CWorld_PhysState* _pPhysState, 
										 const CVec3Dfp32& _Move, const CVec3Dfp32& _Look, uint32 _Press, int& _Released, 
										 const CXR_MediumDesc& _MediumDesc, int16& _Flags, CVec3Dfp32& _VRet)
{
	CMat43fp32 MatLook;
	CVec3Dfp32 Move;

	_VRet = 0.0f;
	CWO_Character_ClientData2* pCD = CWObject_Character2::GetClientData(_pObj);
	if(!pCD)
		return 0;

	if ((_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOMOVE) ||
		(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE))
		Move = 0.0f;
	else
		Move = _Move;

	const int32 PhysType = CWObject_Character2::Char_GetPhysType(_pObj);


	_Look.CreateMatrixFromAngles(0, MatLook);
	_pObj->GetPosition().SetMatrixRow(MatLook, 3);

	CMat43fp32 MatWalk;
	//	CVec3Dfp32 WalkDir(0, 0, Look.k[2]);
	//	WalkDir.CreateMatrixFromAngles(0, MatWalk);
	MatWalk.Unit();
	MatWalk.M_x_RotZ(_Look.k[2]);

	//Should we switch off gravity?
	if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOGRAVITY || pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY)
		_Flags |= 1;

	if (!(_MediumDesc.m_MediumFlags & XW_MEDIUM_LIQUID))
	{
		// -------------------------------------------------------------------
		//  Not in liquid
		// -------------------------------------------------------------------

		// Slowdown factor does not affect jumping speed

		CVec3Dfp32 dV(0);
		if(!(PhysType == PLAYER_PHYS_DEAD))
		{
			dV += CVec3Dfp32::GetRow(MatWalk, 0) * (Move[0] * (fp32)m_Speed_Forward);
			dV += CVec3Dfp32::GetRow(MatWalk, 1) * (Move[1] * (fp32)m_Speed_SideStep);
		}
		//				dV *= Char()->Speed();
		//				if ((_Press & CONTROLBITS_TURBO) && Char_CheatsEnabled()) dV *= 2.0f;

		// Check if we're standing on something.
		CCollisionInfo CInfo_1;
		CMat43fp32 p = _pObj->GetPositionMatrix();
		CVec3Dfp32::GetMatrixRow(p, 3) += CVec3Dfp32(0,0,-0.02f);


		bool bOnGround;
		{
			// MUPPJOKKO - IMPLEMENT ME! en hel kollisions query enbart för att se om man står på marken eller inte!!
#ifdef	USE_PCS
			CPotColSet pcs;
			{
				MSCOPESHORT(BuildPCS);
				float fBoxMinMax[6];																// bounds of movement
				CMat43fp32 DestPos;																	// destination of movement

				//							GetMovementBounds( fBoxMinMax, _iObj, pObj->GetLocalPositionMatrix(), p );	// extract [x,y,z,t] bounds

				CBox3Dfp32 Box1, Box2;
				_pPhysState->Phys_GetMinMaxBox( _pObj->GetPhysState(), _pObj->GetPositionMatrix(), Box1 );
				//							_pPhysState->Phys_GetMinMaxBox( _pObj->GetPhysState(), _pObj->GetPositionMatrix(), Box2 );

				//							Box1.Expand( Box2 );

				fBoxMinMax[0]	= Box1.m_Min[0];
				fBoxMinMax[1]	= Box1.m_Min[1];
				fBoxMinMax[2]	= Box1.m_Min[2] - 0.02f;
				fBoxMinMax[3]	= Box1.m_Max[0];
				fBoxMinMax[4]	= Box1.m_Max[1];
				fBoxMinMax[5]	= Box1.m_Min[2] + 1.0f;

				pcs.SetBox( fBoxMinMax );
				_pPhysState->Selection_GetArray( &pcs, _iSel, _pObj->GetPhysState(), _pObj->m_iObject, _pObj->GetLocalPositionMatrix(), p );
			}
			// NOTE: Assuming char origin is at the bottom of it's box  -mh
			CWO_PhysicsState PhysState(_pObj->GetPhysState());
			PhysState.m_Prim[0].m_Dimensions[2] = 1;
			PhysState.m_Prim[0].m_Offset[2] = 1;
			bOnGround = _pPhysState->Phys_IntersectWorld(&pcs, PhysState, _pObj->GetPositionMatrix(), p, _pObj->m_iObject, &CInfo_1);
#else
			// NOTE: Assuming char origin is at the bottom of it's box  -mh
			CWO_PhysicsState PhysState(_pObj->GetPhysState());
			PhysState.m_Prim[0].m_Dimensions[2] = 1;
			PhysState.m_Prim[0].m_Offset[2] = 1;
			bOnGround = _pPhysState->Phys_IntersectWorld(_iSel, PhysState, _pObj->GetPositionMatrix(), p, _pObj->m_iObject, &CInfo_1);
#endif
		}

		if (!CInfo_1.m_bIsValid)
			bOnGround = false;

		if (!bOnGround)
			_Flags |= 2;

		// Invalidate the material
		///pCD->m_GroundMaterial = 0;

		if ((bOnGround || pCD->m_Phys_nInAirFrames < 3) && !(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOGRAVITY) && !(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY))
		{
			// -------------------------------------------------------------------
			//  Walking
			// -------------------------------------------------------------------

			/*if(CInfo_1.m_pSurface)
			pCD->m_GroundMaterial = (uint8)CInfo_1.m_pSurface->GetBaseFrame()->m_MaterialType;*/

			// We hit something, lets look at the collision plane...
			CVec3Dfp32 Normal;
			Normal = (CInfo_1.m_bIsValid) ? CInfo_1.m_Plane.n : CVec3Dfp32(0,0,1);
			Normal.Normalize();
			fp32 Friction = 1.0f;

			if( CInfo_1.m_bIsValid )
			{
				pCD->m_Phys_iLastGroundObject = CInfo_1.m_iObject;

				if (Normal.k[2] > PLAYER_PHYS_COS_MAX_SLOPE)
				{
					// Slope is ok. Project velocity so that it is parallell to the plane. (this way we get ramp-jumping since the velocity increases with the slope)
					CVec3Dfp32 ProjDir(0,0,1);
					dV.Combine(ProjDir, -(dV*Normal) / (ProjDir*Normal), dV);
				}
				else if (Normal.k[2] < PLAYER_PHYS_COS_MAX_SLOPE)
				{
					// Slope is greater than the accepted 'full-speed' slope, so we scale the influence by the feet.
					if (Normal.k[2] > 0.0f)
						Friction = (2.0f*Clamp01(0.5f - Normal.k[2]));
					else
						Friction = 0;
				}
			}

			//				if (CInfo_1.m_bIsValid)
			//					_pObj->m_PhysAttrib.m_StepSize = PLAYER_STEPSIZE * Clamp01(5.0f * (Normal[2] - 0.8f));

			const fp32 MoveLenFric = Clamp01(0.35f + Length2(Move[0], Move[1]) * 0.65f);
			Friction *= MoveLenFric;

			// Feet on floor
			CVec3Dfp32 dVMoved2;
			if (CInfo_1.m_bIsValid)
			{
				const CWObject_CoreData* pCoreData = _pPhysState->Object_GetCD(CInfo_1.m_iObject);
				if (pCoreData && pCoreData->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
				{
					_pObj->GetPosition().Sub(pCoreData->GetPosition(), dVMoved2);
					dVMoved2[2] = 0;
					//								dVMoved2.Normalize();
					//								dVMoved2 *= 8.0f;
					dVMoved2.SetLength( 8.0f );

				}
				else
				{
					dVMoved2 = CInfo_1.m_Velocity;
				}
			}
			else
			{
				const CWObject_CoreData* pCoreData = _pPhysState->Object_GetCD(pCD->m_Phys_iLastGroundObject);
				if (pCoreData)
				{
					dVMoved2 = pCoreData->GetMoveVelocity();
				}
				else
					dVMoved2 = 0;
			}

			//				ConOut("dVMoved2 == " + dVMoved2.GetString());
			CVec3Dfp32 v = _pObj->GetMoveVelocity();
			const fp32 vlenSqr = dV.LengthSqr();
			dV += dVMoved2;
			CVec3Dfp32 dV2(dV - v);

			// This is the acceleration formula we've allways had.
			// VRet += dV2.Normalize() * Min(dV2.Length() / 2.0f, Max(2.6f, vlen / 2.0f));
			const fp32 dV2LenSqr = dV2.LengthSqr();
			if (dV2LenSqr > Sqr(0.001f))
				_VRet += dV2.Normalize() * Min(M_Sqrt(dV2LenSqr) * (1.0f / 2.0f), Max(2.6f, M_Sqrt(vlenSqr) * (1.0f / 2.0f)));
			_VRet *= Friction;

			if (!bOnGround)
				_VRet[2] = 0;
			/*						else if ((dV2LenSqr > Sqr(0.001f)) && (Normal.k[2] > PLAYER_PHYS_COS_MAX_SLOPE))
			{
			fp32 Scale = VRet.Length() / Length2(VRet[0], VRet[1]);
			VRet[0] *= Scale;
			VRet[1] *= Scale;
			VRet[2] = 0;
			}*/

#ifdef	ENABLE_PHYSLOG
			//		if (_pChar && _pChar->m_bPhysLog) 
			/*							ConOutL(CStrF("V %s, VRet %s, Valid %d, dV2 %s, CInfo.m_Vel %s, CInfo.Normal %s", 
			(char*) _pObj->GetMoveVelocity().GetString(), 
			(char*) VRet.GetString(), 
			CInfo_1.m_bIsValid,
			(char*) dV2.GetString(), 
			(char*) CInfo_1.m_Velocity.GetString(), 
			(char*) CInfo_1.m_Plane.n.GetString()));*/
#endif

			if (CInfo_1.m_bIsValid && (Normal.k[2] > PLAYER_PHYS_COS_MAX_SLOPE))
			{
				if (bOnGround && dVMoved2.LengthSqr() == 0)
					_Flags |= 1;	// Turn off gravity in PhysUtil_Move

				if (!(PhysType == PLAYER_PHYS_DEAD) &&
					!(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOMOVE) &&
					!(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE) &&
					(_Press & CONTROLBITS_JUMP) & _Released)
				{
					// -------------------------------------------------------------------
					//  Jump
					// -------------------------------------------------------------------

					// NOTE: Parts of this code is duplicated in OnPhysicsEvent
					_Released &= ~CONTROLBITS_JUMP;
					_VRet.k[2] = 0;

					// Make some noise
					_pPhysState->Phys_Message_SendToObject(
						CWObject_Message(OBJMSG_CHAR_RAISENOISELEVEL,99),_pObj->m_iObject);

					// Hmm, ok before we can jump, test if there's a ledge around that
					// we can use....
					/*CWAGI_Context Context(_pObj,_pPhysState,_pPhysState->GetGameTime(),0,false);
					fp32 PickupType = AG_PICKUP_TYPELEDGE;
					CXRAG_ICallbackParams Params(&PickupType,1);
					pCD->Effect_PickupStuff(&Context, &Params);
					if (Char_GetControlMode(_pObj) != PLAYER_CONTROLMODE_LEDGE)*/
					{
						const CVec3Dfp32& CurV = _pObj->GetMoveVelocity();
						fp32 vz = _VRet.k[2] - Min(0.0f, CurV[2]);
						float fSpeed_Jump = m_Speed_Jump/* * Char()->JumpSpeed()*/;
						fp32 JumpV = Max(vz + fSpeed_Jump, fSpeed_Jump);
						_pPhysState->Object_AddVelocity(_pObj->m_iObject, CVec3Dfp32(0,0,JumpV));

						_Flags |= 2;	// ??
						//									VRet[0] *= 0.2f;
						//									VRet[1] *= 0.2f;
						//									VRet[0] += CurV[0] * 0.2f;
						//									VRet[1] += CurV[1] * 0.2f;
						_VRet[0] = _VRet[0] * 0.2f + CurV[0] * 0.2f;
						_VRet[1] = _VRet[1] * 0.2f + CurV[1] * 0.2f;
#ifdef	ENABLE_PHYSLOG
						if (_pChar && _pChar->m_bPhysLog)
							ConOutL(CStrF("PostJump speed %s, %f", (char*)_VRet.GetString(), CurV[2]));
#endif

						// Ruin accuracy.
						//pCD->Target_SetAccuracy(0);

						/*								if(pCD->m_bIsServerRefresh)
						{
						// Hack
						CWObject_Character *pObj = (CWObject_Character *)_pObj;
						pObj->PlayDialogue(PLAYER_DIALOGUE_JUMP);
						}*/

						// Movetoken to jump in animgraph
						/*MoveToken(_pObj,_pPhysState, pCD,0,CStr("MTJumpSwitch.Action_InitJump"));
						_pPhysState->Phys_Message_SendToObject(
							CWObject_Message(OBJMSG_CHAR_SETGRABBEDBODY,0), _pObj->m_iObject);*/
						/*char* MoveToken = "MTJumpSwitch.Action_InitJump";
						CWObject_Message JumpAG(OBJMSG_CHAR_MOVETOKEN,0,0,0,0,0,0,MoveToken);
						_pPhysState->Phys_Message_SendToObject(JumpAG,_pObj->m_iObject);*/
						/*
						{
						int StanceBase = Char_GetAnimStance(_pObj);
						if(StanceBase == 0xfff)
						StanceBase = 0;
						//									Char_SetAnimSequence(_pObj, _pPhysState, StanceBase + PLAYER_ANIM_SEQUENCE_JUMP);
						}
						*/								}
				}
				else
				{
					if (_VRet.k[2] < 0)
						_VRet.k[2] = 0;
					//						if (!_pChar) ConOut(CStrF("No Jump: Press %.8x, Release %.8x, ", _Press, _Released) + Normal.GetString());
				}

			}
		}
		else
		{
			// -------------------------------------------------------------------
			//  Airborne
			// -------------------------------------------------------------------

			// Well, air control isn't quite as simple as one might believe:
			if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOGRAVITY || pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY)
			{
				//Truly flying
				CVec3Dfp32 dV(0);
				dV += CVec3Dfp32::GetRow(MatLook, 0) * (Move[0] * (fp32)m_Speed_Forward);
				dV += CVec3Dfp32::GetRow(MatLook, 1) * (Move[1] * (fp32)m_Speed_SideStep);
				dV += CVec3Dfp32::GetRow(MatLook, 2) * (Move[2] * (fp32)m_Speed_Up);

				dV *= 2.5f;
				dV.Lerp(_pObj->GetMoveVelocity(), 0.9f, dV);

				_VRet = dV - _pObj->GetMoveVelocity();
			}
			else
			{
				//							VRet = 0;
				//							break;

				//Just falling
				CVec3Dfp32 CurrentV = _pPhysState->Object_GetVelocity(_pObj->m_iObject);
				CurrentV[2] = 0;
				dV[2] = 0;
				const fp32 dVLen = dV.Length();
				CVec3Dfp32 VAdd;
				VAdd = 0;
				const fp32 CurrentVLen = CurrentV.Length();
				const fp32 dVProjCurrent = (dV*CurrentV) / CurrentVLen;
				fp32 CurrentProjdV = (dVLen > 0.0f) ? (dV*CurrentV) / dVLen : 0;
				if (CurrentProjdV < 0.0f) CurrentProjdV = 0;
				if (CurrentProjdV < dVLen)
				{
					VAdd = dV*((dVLen - CurrentProjdV) / dVLen); // - CurrentV*(CurrentVLen
					const fp32 VAddLen = VAdd.Length();
					VAdd = VAdd.Normalize() * Min(VAddLen * (1.0f/ 6.0f), Max(0.0f, dVLen * (1.0f / 15.0f)));
				}

				_VRet = VAdd;
			}
		}
	}
	else
	{
		// -------------------------------------------------------------------
		//  In liquid (swimming)
		// -------------------------------------------------------------------
		CVec3Dfp32 dV(0);
		dV += CVec3Dfp32::GetRow(MatLook, 0) * (Move[0] * (fp32)m_Speed_Forward);
		dV += CVec3Dfp32::GetRow(MatLook, 1) * (Move[1] * (fp32)m_Speed_SideStep);
		dV += CVec3Dfp32::GetRow(MatLook, 2) * (Move[2] * (fp32)m_Speed_Up);
		//	dV *= Char()->Speed();
		// if ((_Press & CONTROLBITS_TURBO) && Char_CheatsEnabled()) dV *= 2.0f;

		// Up?
		if (_Press & CONTROLBITS_JUMP)
			dV.k[2] = Min(dV.k[2] + 8.0f, 8.0f);


		CCollisionInfo CInfo_2;
		CMat43fp32 p = _pObj->GetPositionMatrix();
		CVec3Dfp32::GetMatrixRow(p, 3) += CVec3Dfp32(0,0,-8);
		bool bOnGround;
		{
			// MUPPJOKKO - IMPLEMENT ME! en hel kollisions query enbart för att se om man står på marken eller inte!!
			// NOTE: Assuming char origin is at the bottom of it's box  -mh
			CWO_PhysicsState PhysState(_pObj->GetPhysState());
			PhysState.m_Prim[0].m_Dimensions[2] = 1;
			PhysState.m_Prim[0].m_Offset[2] = 1;
			bOnGround = _pPhysState->Phys_IntersectWorld(_iSel, PhysState, _pObj->GetPositionMatrix(), p, _pObj->m_iObject, &CInfo_2);
		}
		if (!CInfo_2.m_bIsValid)
			bOnGround = false;

		CVec3Dfp32 v = _pPhysState->Object_GetVelocity(_pObj->m_iObject);

		/*int MedType = pCD->m_HeadMediumType; 
		// Swim only if not feet is on solid ground and the head is above surface.
		if(!(!(MedType & XW_MEDIUM_LIQUID) && bOnGround))
		{
		// Force player up
		bool IsDiving = (_Look[1] < -0.05f) && (v.LengthSqr() > Sqr(2.0f));
		if(!IsDiving)
		dV.k[2] = Min(dV.k[2] + 2.0f, 8.0f);
		//pCD->m_LiquidTickCountDown = 5;
		}*/
		dV *= 0.9f;

		CVec3Dfp32 dVMoved2 = _MediumDesc.m_Velocity;
		fp32 vlen = dV.Length() * (1.f / 9.f);
		dV += dVMoved2;
		CVec3Dfp32 dV2(dV - v);
		fp32 Length = dV2.Length();
		dV2 *= 1.f / Length;
		Length *= 0.25f;
		_VRet += dV2 * Min(Length, Max(0.0f, vlen));
	}

	if (_MediumDesc.m_MediumFlags & XW_MEDIUM_LIQUID)
	{
		pCD->m_TicksInLiquid = pCD->m_TicksInLiquid + 1;
		// The risk of recieving damage from being under water is growing linearly from 200 ticks.
		// It safe to be under water for 200 ticks. (10 seconds)
		// After that the chance of recieving damage every tick is increasing with 0.2% per tick.
		// Example: After 20 seconds (400 ticks) under water the risk of getting damage that tick is
		// 40%. After 700 ticks (35 seconds) the risk is 100% every frame.
		/*
		int Tinl = pCD->m_TicksInLiquid;
		if(Tinl > 1200 && pCD->m_bIsServerRefresh &&
		MRTC_RAND() % 800 < ((fp32)Tinl - 1200.0f))	
		{
		CWO_DamageMsg Msg(1, DAMAGE_SUFFOCATE);
		Msg.Send(_pObj->m_iObject, 0, _pPhysState);
		//						_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_PHYSICS_DAMAGE, 1, DAMAGE_SUFFOCATE, -1, 0, 0, 0), _pObj->m_iObject);
		}		
		*/
	}
	else
	{
		pCD->m_TicksInLiquid = 0;
	}

	if (!(_Press & CONTROLBITS_JUMP))
		_Released |= CONTROLBITS_JUMP;

	if ((pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE) &&
		((_VRet[0] != 0) || (_VRet[1] != 0)))
	{
		//Eliminate horizontal and upwards acceleration
		//Can occur due to fixed char move from fighting etc...
		_VRet = CVec3Dfp32(0,0,Max(0.0f, _VRet[2]));
	}

	return 1;
}




// Handler


int32 CControlHandler::OnCreateClientUpdate(uint8*& _pBuffer)
{
	// Create diff state
	int32 Size = m_ClientMirror.CreateDiff(*this, _pBuffer);
	m_ClientMirror.Pack(*this);

	return Size;
}

// Unpack stuff on client
void CControlHandler::OnClientUpdate(const uint8*& _pBuffer)
{
	m_ClientMirror.UpdateWithDiff(*this,_pBuffer);
}

int32 CControlHandler::DoCamera(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMat43fp32& _Camera)
{
	// Bleh... (register appropriate function when adding control..? and just call function)
	int32 Len = m_lControls.Len();
	for (int32 i = 0; i < Len; i++)
	{
		int32 Val = m_lControls[i]->DoCamera(_pObj,_pWClient,_Camera);
		if (Val)
			return Val;
	}

	return 0;
}

int32 CControlHandler::DoPhysics(int _iSel, CWObject_CoreData* _pObj, CWObject_Character2* _pChar, CWorld_PhysState* _pPhysState, 
						const CVec3Dfp32& _Move, const CVec3Dfp32& _Look, uint32 _Press, int& _Released, 
						const CXR_MediumDesc& _MediumDesc, int16& _Flags, CVec3Dfp32& _VRet)
{
	// Bleh... (register appropriate function when adding control..? and just call function)
	int32 Len = m_lControls.Len();
	for (int32 i = 0; i < Len; i++)
	{
		int32 Val = m_lControls[i]->DoPhysics(_iSel, _pObj, _pChar, _pPhysState, 
			_Move, _Look, _Press, _Released, _MediumDesc, _Flags, _VRet);
		if (Val)
			return Val;
	}

	return 0;
}

void CControlHandler::OnRefresh_Client(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	// Bleh...
	int32 Len = m_lControls.Len();
	for (int32 i = 0; i < Len; i++)
		m_lControls[i]->OnRefresh_Client(_pObj,_pWClient);
}

void CControlHandler::AddControl(int8 _ControlType, const CRegistry* _pKey)
{
	spCWObject_ControlClass spControl = CControlReg::CreateControlClass(_ControlType);
	if (spControl)
	{
		int32 Len = _pKey->GetNumChildren();
		for (int32 i = 0; i < Len; i++)
		{
			const CRegistry* pChild = _pKey->GetChild(i)
			spControl->OnEvalKey(pChild->GetThisNameHash(), pChild);
		}

		m_lControls.Add(spControl);
	}
}

void CControlHandler::AddControl(char* _pName, const CRegistry* _pKey)
{
	spCWObject_ControlClass spControl = CControlReg::CreateControlClass(_pName);
	if (spControl)
	{
		int32 Len = _pKey->GetNumChildren();
		for (int32 i = 0; i < Len; i++)
		{
			const CRegistry* pChild = _pKey->GetChild(i)
			spControl->OnEvalKey(pChild->GetThisNameHash(), pChild);
		}

		m_lControls.Add(spControl);
	}
}

void CControlHandler::Update(CWObject_CoreData* _pChar, CWorld_PhysState* _pWPhys)
{
    int32 Len = m_lControls.Len();
	for (int32 i = 0; i < Len; i++)
		m_lControls[i]->Update(_pChar,_pWPhys,NULL);
}

void CControlHandler::Clear()
{
	m_lControls.Clear();
}

void CControlHandler::CopyFrom(const CControlHandler& _Handler)
{
	// Hmm, make whole copy of list later ....FIXME
	m_lControls = _Handler.m_lControls;
	m_ClientMirror.CopyFrom(_Handler.m_ClientMirror);
}

#ifndef M_RTM

void CWObject_ControlClass::DebugPrint(CStr& _Str)
{
	CStr Temp;
	CControlReg::GetControlType(m_ControlType,Temp);

	_Str += CStrF(", Type: %s", Temp.GetStr());
}

void CWObject_ControlCamera::DebugPrint(CStr& _Str)
{
	m_Camera_StandHeadOffset;
	m_Camera_CrouchHeadOffset;
	m_Camera_BehindOffset;
	m_Camera_ShoulderOffset;

	_Str += CStrF(", StandHead: %s, CrouchHead: %s, Behind: %s, ShoulderOffset: %s", m_Camera_StandHeadOffset.GetString().GetStr(),m_Camera_CrouchHeadOffset.GetString().GetStr(),m_Camera_BehindOffset.GetString().GetStr(),m_Camera_ShoulderOffset.GetString().GetStr());
}


void CControlHandler::DebugPrint(CStr& _Str)
{
	int32 Len = m_lControls.Len();
	_Str += CStrF("Len: %d",Len);
	for (int32 i = 0; i < Len; i++)
	{
		_Str += "{";
		m_lControls[i]->DebugPrint(_Str);
		_Str += "}";
	}
}
#endif

void CControlHandlerPacked::CopyFrom(const CControlHandlerPacked& _Ref)
{
	m_lControlPacked = _Ref.m_lControlPacked;
}

void CControlHandlerPacked::Pack(const CControlHandler& _Handler)
{
	// Just pack the stuff
	int32 Len = _Handler.m_lControls.Len();
	m_lControlPacked.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
	{
		// Control didn't exist, make a new copy
		m_lControlPacked[i].GetFlags() |= CWObject_ControlClassPacked::CONTROLCLASS_PACKED_ADDED;
		m_lControlPacked[i].Pack(*_Handler.m_lControls[i]);
	}
}

int32 CControlHandlerPacked::CreateDiff(const CControlHandler& _Handler, uint8*& _pBuffer) const
{
	// Oki, we are the client mirror, create a diff with current state
	uint8* pBufferCache = _pBuffer;
	int32 Len = _Handler.m_lControls.Len();
	// Write len
	PTR_PUTINT32(_pBuffer,Len);
	int32 LenMirror = m_lControlPacked.Len();
	//_Dest.m_lControlPacked.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
	{
		bool bFoundCopy = false;
		
		//... (Test)
		for (int32 j = 0; j < LenMirror; j++)
		{
			// Check if control existed, if so make a diff (hmm, should the activatetime be checked..?)
			int32 ControlType = _Handler.m_lControls[i]->GetControlType();
			int32 ControlType2 = m_lControlPacked[j].GetControlType();
			if ((_Handler.m_lControls[i]->GetControlType() == m_lControlPacked[j].GetControlType()))/* &&
				_Handler.m_lControls[i]->GetActivateTime().Compare(m_lControlPacked[j].GetActivateTime()) == 0)*/
			{
				//_Dest.m_lControlPacked[i].GetFlags() |= CWObject_ControlClassPacked::CONTROLCLASS_PACKED_UPDATED;
				CWObject_ControlClassPacked::MakeDiff(m_lControlPacked[j],*_Handler.m_lControls[i],_pBuffer);
				bFoundCopy = true;
			}
		}
		
		// (add/remove flags perhaps...)
		if (!bFoundCopy)
		{
			// Control didn't exist, make a new copy
			//_Dest.m_lControlPacked[i].GetFlags() |= CWObject_ControlClassPacked::CONTROLCLASS_PACKED_ADDED;
			CWObject_ControlClassPacked::Pack(*_Handler.m_lControls[i],_pBuffer);
		}
	}
	return (_pBuffer - pBufferCache);
}

void CControlHandlerPacked::UpdateWithDiff(CControlHandler& _Handler, const uint8*& _pBuffer)
{
	// Write len
	int32 ServerLen;
	PTR_GETINT32(_pBuffer,ServerLen);

	// These might be different because of prediction
	int32 LenClient = _Handler.m_lControls.Len();

	TArray<spCWObject_ControlClass> lControls;

	lControls.SetLen(ServerLen);
	m_lControlPacked.SetLen(ServerLen);

	for (int32 i = 0; i < ServerLen; i++)
	{
		// hmm, should the activatetime be checked..?, if client is out of sync (timewise) this
		// won't work (argh, check only controltype for now)
		bool bFound = false;
		for (int32 j = 0; j < LenClient; j++)
		{
			if (m_lControlPacked[i].GetControlType() == _Handler.m_lControls[j]->GetControlType())
			{
				// Alrighty then compatible type found, update with diff
				bFound = true;
				lControls[i] = _Handler.m_lControls[j];
				m_lControlPacked[i].UpdateWithDiff(*lControls[i],_pBuffer);
			}
		}
		if (!bFound)
		{
			// Create new instance of controlclass
			m_lControlPacked[i].GetControlTypeFromBuffer(_pBuffer);
			lControls[i] = CControlReg::CreateControlClass(m_lControlPacked[i].GetControlType());
			if (lControls[i])
				m_lControlPacked[i].UpdateWithDiff(*lControls[i],_pBuffer);
		}
	}

	// Hmm, does this work? (old controls will get kicked out with this)
	_Handler.m_lControls = lControls;
}

/*void CControlHandler::CreateDiff(const CControlHandlerPacked& _Ref, CControlHandlerPacked& _Dest) const
{

}*/

/*TArray<CControlReg::RegEntry>*/CControlReg::RegEntry CControlReg::m_slControlTypes[10];
int8 CControlReg::m_sType = 0;
void CControlReg::AddControlType(const char* _pName, spCWObject_ControlClass (*_pfnCreate)())
{
	int32 Len = strlen(_pName);
	if (Len > CCONTROLREG_NAME_MAXLEN)
		Error_static("CControlReg::AddControlType","Name too long");

	int32 Index = m_sType;
	RegEntry Reg;
	Reg.m_Type = m_sType++;
	strcpy(Reg.m_Name,_pName);
	Reg.m_pfnCreate = _pfnCreate;
	m_slControlTypes[Index] = Reg;
	//m_slControlTypes.Add(Reg);
}

spCWObject_ControlClass CControlReg::CreateControlClass(int8 _Type)
{
	int32 Len = m_sType;//m_slControlTypes.Len();
	if (_Type > Len || _Type < 0)
		return NULL;

	spCWObject_ControlClass spControl = m_slControlTypes[_Type].m_pfnCreate();
	if (spControl)
		spControl->m_ControlType = _Type;

	return spControl;
}

int8 CControlReg::GetControlType(char* _pName)
{
	CStr Str(_pName);
	int32 Len = m_sType;//m_slControlTypes.Len();
	for (int32 i = 0; i < Len; i++)
		if (Str.CompareNoCase(m_slControlTypes[i].m_Name) == 0) //strcmp(m_slControlTypes[i].m_Name,_pName) == 0)
			return i;

	return CONTROLTYPE_UNKNOWN;
}

void CControlReg::GetControlType(int8 _Type, CStr& _Name)
{
	int32 Len = m_sType;//m_slControlTypes.Len();
	for (int32 i = 0; i < Len; i++)
		if (m_slControlTypes[i].m_Type == _Type)
		{
			_Name = m_slControlTypes[i].m_Name;
			return;
		}
}

spCWObject_ControlClass CControlReg::CreateControlClass(char* _pName)
{
	return CreateControlClass(GetControlType(_pName));
}