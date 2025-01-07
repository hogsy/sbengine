#include "PCH.h"
#include "WObj_CameraUtil.h"
#include "WObj_Model_Ladder.h"


void CCameraUtil::Clear()
{
	m_MatListener.Unit();
	m_MatSpeaker.Unit();
	m_HeadOffsetListener = 0.0f;
	m_HeadOffsetSpeaker = 0.0f;
	m_PosSpeakerLookat = 0.0f;
	m_PosListenerLookat = 0.0f;
	m_CameraStartTime.MakeInvalid();
	m_pDialogToken = NULL;
	m_Mode = 0;
	m_iOriginObj = 0;
	m_iTargetObj = 0;
	m_iExtraObj = 0;
	m_FOV = 90.0f;
	m_FovCoeff = 1.0f - ((/*m_FOV*/60-10.0f) / 80.0f);

	// Standard value, skip copying over net for now
	m_CameraDistance = 70.0f;
	m_MountedLookBlendStart = 0;
	m_MountedLookBlendDuration = 0;
	m_MountedLookThreshold = 0.0f;
	m_PrevMountedLookZ = 0.0f;
	m_PrevMountedLookY = 0.0f;
	m_LastLookTick = 0;
	m_MountedLookZ = 0.0f;
	m_MountedLookY = 0.0f;
	m_MinMountedMaxLookAngleZ = 0;
	m_MinMountedMaxLookAngleY = 0;
	m_MaxMountedMaxLookAngleZ = 0;
	m_MaxMountedMaxLookAngleY = 0;
	m_DecreaseRate = 10;

	m_AngleOffsetX = 0.0f;
	m_LastAngleOffsetX = 0.0f;
	m_TargetAngleOffsetX = 0.0f;
	m_AngleOffsetXChange = 0.0f;
	m_AngleOffsetY = 0.0f;
	m_LastAngleOffsetY = 0.0f;
	m_TargetAngleOffsetY = 0.0f;
	m_AngleOffsetYChange = 0.0f;
	m_Type = 0;
	m_Mode_Flipped = false;
}

void CCameraUtil::CopyFrom(const CCameraUtil& _Util)
{
	m_MatListener = _Util.m_MatListener;
	m_MatSpeaker = _Util.m_MatSpeaker;
	m_HeadOffsetListener = _Util.m_HeadOffsetListener;
	m_HeadOffsetSpeaker = _Util.m_HeadOffsetSpeaker;
	m_PosSpeakerLookat = _Util.m_PosSpeakerLookat;
	m_PosListenerLookat = _Util.m_PosListenerLookat;
	m_CameraStartTime = _Util.m_CameraStartTime;
	m_pDialogToken =_Util.m_pDialogToken;
	m_Mode = _Util.m_Mode;
	m_iOriginObj = _Util.m_iOriginObj;
	m_iTargetObj = _Util.m_iTargetObj;
	m_iExtraObj = _Util.m_iExtraObj;
	m_FOV = _Util.m_FOV;
	m_FovCoeff = _Util.m_FovCoeff;
	m_CameraDistance = _Util.m_CameraDistance;
	m_MountedLookBlendStart = _Util.m_MountedLookBlendStart;
	m_MountedLookBlendDuration = _Util.m_MountedLookBlendDuration;
	m_MountedLookThreshold = _Util.m_MountedLookThreshold;
	m_PrevMountedLookZ = _Util.m_PrevMountedLookZ;
	m_PrevMountedLookY = _Util.m_PrevMountedLookY;
	m_LastLookTick = _Util.m_LastLookTick;
	m_MountedLookZ = _Util.m_MountedLookZ;
	m_MountedLookY = _Util.m_MountedLookY;
	m_MinMountedMaxLookAngleZ = _Util.m_MinMountedMaxLookAngleZ;
	m_MinMountedMaxLookAngleY = _Util.m_MinMountedMaxLookAngleY;
	m_MaxMountedMaxLookAngleZ = _Util.m_MaxMountedMaxLookAngleZ;
	m_MaxMountedMaxLookAngleY = _Util.m_MaxMountedMaxLookAngleY;
	m_DecreaseRate = _Util.m_DecreaseRate;

	m_AngleOffsetX = _Util.m_AngleOffsetX;
	m_LastAngleOffsetX = _Util.m_LastAngleOffsetX;
	m_TargetAngleOffsetX = _Util.m_TargetAngleOffsetX;
	m_AngleOffsetXChange = _Util.m_AngleOffsetXChange;
	m_AngleOffsetY = _Util.m_AngleOffsetY;
	m_LastAngleOffsetY = _Util.m_LastAngleOffsetY;
	m_TargetAngleOffsetY = _Util.m_TargetAngleOffsetY;
	m_AngleOffsetYChange = _Util.m_AngleOffsetYChange;
	m_Type = _Util.m_Type;
	m_Mode_Flipped = _Util.m_Mode_Flipped;
}

void CCameraUtil::Pack(uint8 *&_pD, CMapData* _pMapData) const
{
	// Bah, copy all for now
	TAutoVar_Pack(m_CameraStartTime, _pD);
	TAutoVar_Pack(m_Mode, _pD);
	TAutoVar_Pack(m_iOriginObj, _pD);
	TAutoVar_Pack(m_iTargetObj, _pD);
	TAutoVar_Pack(m_iExtraObj, _pD);
	TAutoVar_Pack(m_FOV, _pD);
	TAutoVar_Pack(m_Type, _pD);
	if (m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM)
	{
		TAutoVar_Pack(m_MinMountedMaxLookAngleZ, _pD);
		TAutoVar_Pack(m_MinMountedMaxLookAngleY, _pD);
		TAutoVar_Pack(m_MaxMountedMaxLookAngleZ, _pD);
		TAutoVar_Pack(m_MaxMountedMaxLookAngleY, _pD);
		TAutoVar_Pack(m_DecreaseRate, _pD);
		TAutoVar_Pack(m_MountedLookBlendStart, _pD);
		TAutoVar_Pack(m_MountedLookBlendDuration, _pD);
		TAutoVar_Pack(m_MountedLookZ, _pD);
		TAutoVar_Pack(m_MountedLookY, _pD);
	}
}

void CCameraUtil::Unpack(const uint8 *&_pD, CMapData* _pMapData)
{
	TAutoVar_Unpack(m_CameraStartTime, _pD);
	TAutoVar_Unpack(m_Mode, _pD);
	TAutoVar_Unpack(m_iOriginObj, _pD);
	TAutoVar_Unpack(m_iTargetObj, _pD);
	TAutoVar_Unpack(m_iExtraObj, _pD);
	TAutoVar_Unpack(m_FOV, _pD);
	TAutoVar_Unpack(m_Type, _pD);
	if (m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM)
	{
		TAutoVar_Unpack(m_MinMountedMaxLookAngleZ, _pD);
		TAutoVar_Unpack(m_MinMountedMaxLookAngleY, _pD);
		TAutoVar_Unpack(m_MaxMountedMaxLookAngleZ, _pD);
		TAutoVar_Unpack(m_MaxMountedMaxLookAngleY, _pD);
		TAutoVar_Unpack(m_DecreaseRate, _pD);
		TAutoVar_Unpack(m_MountedLookBlendStart, _pD);
		TAutoVar_Unpack(m_MountedLookBlendDuration, _pD);
		TAutoVar_Unpack(m_MountedLookZ, _pD);
		TAutoVar_Unpack(m_MountedLookY, _pD);;
	}
}

void CCameraUtil::OnDeltaSave(CCFile* _pFile) const
{
	m_CameraStartTime.Write(_pFile);
	_pFile->WriteLE(m_Mode);
	_pFile->WriteLE(m_iOriginObj);
	_pFile->WriteLE(m_iTargetObj);
	_pFile->WriteLE(m_iExtraObj);
	_pFile->WriteLE(m_FOV);
	_pFile->WriteLE(m_Type);
	if (m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM)
	{
		_pFile->WriteLE(m_MinMountedMaxLookAngleZ);
		_pFile->WriteLE(m_MinMountedMaxLookAngleZ);
		_pFile->WriteLE(m_MinMountedMaxLookAngleY);
		_pFile->WriteLE(m_MaxMountedMaxLookAngleZ);
		_pFile->WriteLE(m_MaxMountedMaxLookAngleY);
		_pFile->WriteLE(m_DecreaseRate);
		_pFile->WriteLE(m_MountedLookBlendStart);
		_pFile->WriteLE(m_MountedLookBlendDuration);
		_pFile->WriteLE(m_MountedLookZ);
		_pFile->WriteLE(m_MountedLookY);
	}
}

void CCameraUtil::OnDeltaLoad(CCFile* _pFile)
{
	m_CameraStartTime.Read(_pFile);
	_pFile->ReadLE(m_Mode);
	_pFile->ReadLE(m_iOriginObj);
	_pFile->ReadLE(m_iTargetObj);
	_pFile->ReadLE(m_iExtraObj);
	_pFile->ReadLE(m_FOV);
	_pFile->ReadLE(m_Type);
	if (m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM)
	{
		_pFile->ReadLE(m_MinMountedMaxLookAngleZ);
		_pFile->ReadLE(m_MinMountedMaxLookAngleZ);
		_pFile->ReadLE(m_MinMountedMaxLookAngleY);
		_pFile->ReadLE(m_MaxMountedMaxLookAngleZ);
		_pFile->ReadLE(m_MaxMountedMaxLookAngleY);
		_pFile->ReadLE(m_DecreaseRate);
		_pFile->ReadLE(m_MountedLookBlendStart);
		_pFile->ReadLE(m_MountedLookBlendDuration);
		_pFile->ReadLE(m_MountedLookZ);
		_pFile->ReadLE(m_MountedLookY);
	}
}


void CCameraUtil::Dialog_GetInterpolatedPos(CVec3Dfp32& _Pos, fp32 _I)
{
	m_PosListenerLookat.Lerp(m_PosSpeakerLookat,_I,_Pos);
	_Pos.k[2] -= 10.0f * m_FovCoeff;			// FOV dependent
}

void CCameraUtil::GetCameraPosition_OverShoulder(CWorld_PhysState* _pWPhys, const CMTime& Time, CVec3Dfp32& _Camera)
{
	fp32 x = -6.0f;
	if (m_Mode_Flipped)
		x = -x;

	CVec3Dfp32 Offset(-10.0f, x, 0.25f);

	CVec3Dfp32 SpeakerListenerRelation = CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3) - CVec3Dfp32::GetMatrixRow(m_MatListener, 3);
	fp32 Dist = SpeakerListenerRelation.LengthSqr() / 1536.0f;
	fp32 Angle = CVec3Dfp32::AngleFromVector(SpeakerListenerRelation.k[0], SpeakerListenerRelation.k[1]);
	CMat4Dfp32 Rotation;
	Rotation.Unit();
	Rotation.RotZ_x_M(Angle);

	Offset *= (1.0f / m_FovCoeff) * 2.0f;
	Offset *= Dist;
	Offset *= Rotation;

	_Camera = CVec3Dfp32::GetMatrixRow(m_MatListener, 3) + Offset + m_HeadOffsetListener;
}

void CCameraUtil::GetCameraPosition_NearOverShoulder(CWorld_PhysState* _pWPhys, const CMTime& Time, CVec3Dfp32& _Camera)
{
	float x = -3.0f;
	if (m_Mode_Flipped)
		x = -x;

	CVec3Dfp32 Offset(-5.0f, x, 0.25f);

	CVec3Dfp32 SpeakerListenerRelation = CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3) - CVec3Dfp32::GetMatrixRow(m_MatListener, 3);
	fp32 Dist = SpeakerListenerRelation.LengthSqr() / 1536.0f;
	if (Dist > 128.0f)
		Dist = 128.0f;

	fp32 Angle = CVec3Dfp32::AngleFromVector(SpeakerListenerRelation.k[0], SpeakerListenerRelation.k[1]);
	CMat4Dfp32 Rotation;
	Rotation.Unit();
	Rotation.RotZ_x_M(Angle);

	Offset *= (1.0f / m_FovCoeff) * 2.0f;
	Offset *= Dist;
	Offset *= Rotation;

	_Camera = CVec3Dfp32::GetMatrixRow(m_MatListener, 3) + Offset + m_HeadOffsetListener;
}

void CCameraUtil::GetCameraPosition_TwoShot(CWorld_PhysState* _pWPhys, const CMTime& Time, CVec3Dfp32& _Camera)
{
	float Dist = (m_PosSpeakerLookat - m_PosListenerLookat).Length() * (1.0f / m_FovCoeff) * 2.0f;	// FOV dependant
	if (m_Mode_Flipped)
		Dist = -Dist;

	Dist *= 0.45f;
	//	CVec3Dfp32 Rot(-Dist / 2, Dist / 2, Abs(Dist) / 4);
	static fp32 sx=0.4f,sy=0.3f,sz=0.05f;
	CVec3Dfp32 Rot(M_Fabs(Dist) * sx, Dist * sy, M_Fabs(Dist) * sz);

	CVec3Dfp32 Mid;
	Dialog_GetInterpolatedPos(Mid, 0.5f);

	Rot *= m_MatSpeaker;
	Rot -= CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3);
	_Camera = Mid + Rot;

	/*if(m_pWClient)
		m_pWClient->Debug_RenderWire(Mid, _Camera, 0xff7f7f7f, 10.0f);*/
}

void CCameraUtil::GetCameraPosition_Lookat(CWorld_PhysState* _pWPhys, const CMTime& _Time, CVec3Dfp32& _CamPos)
{
	GetCameraTargetPos(_pWPhys,_Time, _CamPos);
	_CamPos += m_pDialogToken->m_CameraListenerOffset;
}

void CCameraUtil::GetCameraPosition_SpeakerHead(CWorld_PhysState* _pWPhys, const CMTime& Time, CVec3Dfp32& _Camera)
{
	CVec3Dfp32 Offset(-35.0f, 0.0f, 5.0f);

	CVec3Dfp32 SpeakerListenerRelation = CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3) - CVec3Dfp32::GetMatrixRow(m_MatListener, 3);
	fp32 Angle = CVec3Dfp32::AngleFromVector(SpeakerListenerRelation.k[0], SpeakerListenerRelation.k[1]);
	CMat4Dfp32 Rotation;
	Rotation.Unit();
	Rotation.RotZ_x_M(Angle);
	Offset *= Rotation;
	_Camera = CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3) + Offset + m_HeadOffsetSpeaker;
}

void CCameraUtil::GetCameraPosition_SafeBoundingBox(CWorld_PhysState* _pWPhys, const CMTime& _Time, CVec3Dfp32& _Camera, const CVec3Dfp32& _LookAtPos)
{
	// Get camera position that is safe inside the bounding box 
	// Find x that is farthest away
	CWObject_CoreData* pObj = _pWPhys->Object_GetCD(m_iOriginObj);
	if (!pObj)
	{
		GetCameraPosition_NearOverShoulder(_pWPhys, _Time, _Camera);
		return;
	}

	CVec3Dfp32 Right;
	CVec3Dfp32 DirToLook = _LookAtPos - pObj->GetPosition();
	DirToLook.k[2] = 0.0f;
	DirToLook.Normalize();
	DirToLook.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),Right);
	if (m_Mode_Flipped)
		Right = -Right;
	const CBox3Dfp32* pBoundBox = pObj->GetAbsBoundBox();
	CVec3Dfp32 P0 = pObj->GetPosition();
	// Make sure we're inside the BB
	P0.k[2] += 10.0f;
	CVec3Dfp32 P1 = P0 + Right * 30.0f;
	fp32 MaxHitSqr = 0.0f;
	for (int32 i = 0; i < 4; i++)
	{
		CVec3Dfp32 HitPos;
		if (pBoundBox->IntersectLine(P1, P0, HitPos))
			MaxHitSqr = Max((HitPos - P0).LengthSqr(),MaxHitSqr);
	}

	if (!(MaxHitSqr > 0.0f))
	{
		GetCameraPosition_NearOverShoulder(_pWPhys, _Time, _Camera);
		return;
	}
	// A little inside for safety
	MaxHitSqr = M_Sqrt(MaxHitSqr) * 0.95f;

	_Camera = CVec3Dfp32::GetMatrixRow(m_MatListener, 3) + Right * MaxHitSqr + m_HeadOffsetListener;
	//m_pWClient->Debug_RenderVertex(_Camera,0xff7f7f7f,30.0f);
}

bool CCameraUtil::GetCameraTargetPos(class CWorld_PhysState* _pWPhys, CMTime _Time, CVec3Dfp32& _TargetPos)
{
	// Origin object for now...
	if (m_Mode & CAMERAUTIL_MODE_TARGET_ENGINEPATH)
	{
		// Get from enginepath
		CMat4Dfp32 Mat;
		bool bRes = GetEnginePathMatrix(_pWPhys, _Time, m_iTargetObj, &Mat) != 0;
		_TargetPos = CVec3Dfp32::GetMatrixRow(Mat,3);
		return bRes;
	}
	else if (m_Mode & CAMERAUTIL_MODE_TARGET_DIALOG_SPEAKER)
	{
		// Bleh, get pos for now...
		_TargetPos = _pWPhys->Object_GetPosition(m_iTargetObj);
		return true;
	}
	else if (m_Mode & (CAMERAUTIL_MODE_TARGET_OBJECTCENTER|CAMERAUTIL_MODE_TARGET_OBJECTTOP|CAMERAUTIL_MODE_TARGET_OBJECTBOTTOM))
	{
		// Look at center of target object
		CWObject_CoreData* pObj = _pWPhys->Object_GetCD(m_iTargetObj);
		if (pObj)
		{
			const CBox3Dfp32* pBox = pObj->GetAbsBoundBox();
			pBox->GetCenter(_TargetPos);
			if (m_Mode & CAMERAUTIL_MODE_TARGET_OBJECTTOP)
				_TargetPos.k[2] += pBox->m_Max.k[2] - _TargetPos.k[2];
			else if (m_Mode & CAMERAUTIL_MODE_TARGET_OBJECTBOTTOM)
				_TargetPos.k[2] -= _TargetPos.k[2] - pBox->m_Min.k[2];

			return true;
		}
	}
	else if (m_Mode & CAMERAUTIL_MODE_TARGET_FIXEDPOSITION)
	{
		_TargetPos = _pWPhys->Object_GetPosition(m_iTargetObj);
		return true;
	}

	return false;
}

bool CCameraUtil::GetCameraViewPos(class CWorld_PhysState* _pWPhys, CMTime _Time, CVec3Dfp32& _ViewPos)
{
	if (m_Mode & CAMERAUTIL_MODE_ORIGIN_ENGINEPATH)
	{
		// Get from enginepath
		CMat4Dfp32 Mat;
		bool bRes = GetEnginePathMatrix(_pWPhys, _Time, m_iOriginObj, &Mat) != 0;
		_ViewPos = CVec3Dfp32::GetMatrixRow(Mat,3);
		return bRes;
	}
	if (m_Mode &CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_MASK)
	{
		CMat4Dfp32 MatOrigin;
		MatOrigin = _pWPhys->Object_GetPositionMatrix(m_iOriginObj);
		_ViewPos = CVec3Dfp32::GetMatrixRow(MatOrigin,3);
		CVec3Dfp32 Offset(0.0f,0.0f,0.0f);

		// These flags can be set at the same time
		if (m_Mode & CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_LEFT)
		{
			// Left of origin object (70 out...)
			Offset += CVec3Dfp32::GetMatrixRow(MatOrigin,1);
		}
		if (m_Mode & CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_RIGHT)
		{
			// Right of origin object
			Offset -= CVec3Dfp32::GetMatrixRow(MatOrigin,1);
		}
		if (m_Mode & CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_ABOVE)
		{
			// Above origin object
			Offset += CVec3Dfp32::GetMatrixRow(MatOrigin,2);
		}
		if (m_Mode & CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_BELOW)
		{
			// Below origin object
			Offset -= CVec3Dfp32::GetMatrixRow(MatOrigin,2);
		}
		if (m_Mode & CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_FRONT)
		{
			// Infront of origin object
			Offset += CVec3Dfp32::GetMatrixRow(MatOrigin,0);
		}
		if (m_Mode & CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_BEHIND)
		{
			Offset -= CVec3Dfp32::GetMatrixRow(MatOrigin,0);
		}
		fp32 Len = Offset.Length();
		if (Len > 0.0f)
			_ViewPos += (Offset / Len) * m_CameraDistance;

		return true;
	}

	return false;
}

// Do some checks to see if the camera has a clear field of view
bool CCameraUtil::ValidCamera(const CVec3Dfp32& _ViewPos, const CVec3Dfp32& _CameraTarget)
{
	return false;
}

void CCameraUtil::SetACSCamera(int32 _Mode, int16 _iACS, int16 _iEnginePath, int16 _iTarget, CMTime _StartTime)
{
	m_Mode = _Mode;
	m_iOriginObj = _iEnginePath;
	m_iTargetObj = _iTarget;
	m_iExtraObj = _iACS;
	m_CameraStartTime = _StartTime;
}
bool CCameraUtil::DialogIsFlipped()
{
	switch (m_Type)
	{
	case CAMERAUTIL_DIALOGTYPE_OVERSHOULDER_LEFT:
	case CAMERAUTIL_DIALOGTYPE_NEARSHOULDER_LEFT:
	case CAMERAUTIL_DIALOGTYPE_TWOSHOT_LEFT:
	case CAMERAUTIL_DIALOGTYPE_CLOSEBB_LEFT:
		return true;
	case CAMERAUTIL_DIALOGTYPE_OVERSHOULDER_RIGHT:
	case CAMERAUTIL_DIALOGTYPE_NEARSHOULDER_RIGHT:
	case CAMERAUTIL_DIALOGTYPE_TWOSHOT_RIGHT:
	case CAMERAUTIL_DIALOGTYPE_CLOSEBB_RIGHT:
		return false;
	}

	return false;
}

bool CCameraUtil::SetDialogCamera(CWorld_PhysState* _pWPhys, CDialogueToken* _pDialogToken, int8 _PreferredType, const CMat4Dfp32 &_MatSpeaker,
									  const CMat4Dfp32 &_MatListener, fp32 _SpeakerHeadOffset, fp32 _ListenerHeadOffset,
									  int16 _SpeakerID, int16 _ListenerID, const CMTime& _StartTime)
{

	if (!_pDialogToken)
		return false;

	// Check if the same camera is already set, if it is, just return...
	if (IsActive() && m_pDialogToken && (m_pDialogToken->m_StartGameTime.Compare(_pDialogToken->m_StartGameTime) == 0)
		&& (m_pDialogToken->m_CameraMode == _pDialogToken->m_CameraMode))
		return true;

	m_pDialogToken = _pDialogToken;
	m_FOV = m_pDialogToken->m_FOV;
	m_Mode &= CAMERAUTIL_NEWTYPE_KEEPMASK;
	// Create dialog camera
	m_pDialogToken = _pDialogToken;
	m_MatSpeaker = _MatSpeaker;
	m_MatListener = _MatListener;
	m_HeadOffsetListener = CVec3Dfp32(0.0f,0.0f,_ListenerHeadOffset);
	m_HeadOffsetSpeaker = CVec3Dfp32(0.0f,0.0f,_SpeakerHeadOffset);
	m_PosSpeakerLookat = CVec3Dfp32::GetMatrixRow(m_MatSpeaker,3) + m_HeadOffsetSpeaker;
	m_PosListenerLookat = CVec3Dfp32::GetMatrixRow(m_MatListener,3) + m_HeadOffsetListener;
	CVec3Dfp32 m_PosListenerLookat;
	m_Mode |= CAMERAUTIL_MODE_DIALOGMATRICES_SET | CAMERAUTIL_MODE_DIALOG;
	m_iTargetObj = _SpeakerID;
	m_iOriginObj = _ListenerID;
	m_CameraStartTime = _StartTime;

	// Test so that next camera works as well (from different pov)...
	// Check which type of dialog camera to use (overshoulder left/right...)
	// Set left for now SHOULD TEST HERE LATER
	
	//int8 CurrentType = 0;

	int8 CurrentType = _PreferredType;
	m_Type = _PreferredType;//CAMERAUTIL_MODE_ORIGIN_DIALOG_OVERSHOULDER_LEFT;
	m_Mode_Flipped = (m_pDialogToken->m_iLastSpeaker == _SpeakerID ? !DialogIsFlipped() : DialogIsFlipped());
	while (1)
	{
		// Test the camera and see if it works....
		CMat4Dfp32 Camera;
		if (GetCameraDialog(_pWPhys, Camera,CMTime::CreateFromSeconds(0.0f)) && CheckDialogCamera(_pWPhys,Camera,m_FOV))
		{
			// Found a working camera, return true
			return true;
		}
		else
		{
			// Check another type
			if (CurrentType == _PreferredType)
				CurrentType++;
			if (CurrentType > CAMERAUTIL_DIALOGTYPE_NUMTYPES)
				break;
			m_Type = CurrentType;
			m_Mode_Flipped = (m_pDialogToken->m_iLastSpeaker == _SpeakerID ? !DialogIsFlipped() : DialogIsFlipped());
			CurrentType++;
		}
	}

	return false;
}

void CCameraUtil::SetAutomaticCamera(CWorld_PhysState* _pWPhys, int16 _iObject, int16 _iChar, bool _bOnChar)
{
	Clear();
	m_iOriginObj = _bOnChar ? _iChar : _iObject;
	m_iTargetObj = _bOnChar ? _iChar : _iObject;
	// Make sure it works, otherwise try some other automatic config
	CMTime Time = CMTime::CreateFromSeconds(0.0f);
	static int32 Tests[4] = 
	{
		CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_LEFT,
		CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_RIGHT,
		CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_FRONT,
		CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_BEHIND,
	};
	int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
	int32 ObjectFlags = OBJECT_FLAGS_WORLD;// | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
	CVec3Dfp32 ViewPos,TargetPos;
	for (int32 i = 0; i < 4; i++)
	{
		m_Mode = Tests[i] | CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_ABOVE|CAMERAUTIL_MODE_TARGET_FIXEDPOSITION|CAMERAUTIL_MODE_USERINPUT;
		if (GetCameraViewPos(_pWPhys,Time,ViewPos) && GetCameraTargetPos(_pWPhys,Time,TargetPos))
		{
			// Check so we don't hit anything
			if (!_pWPhys->Phys_IntersectLine(TargetPos, ViewPos, OwnFlags, ObjectFlags, MediumFlags, m_iOriginObj,_iChar))
				return;
		}
	}
}

// Cutscene camera
//#define CAMERAUTIL_DEBUG
bool CCameraUtil::SetMountedCamera(CWorld_PhysState* _pWPhys, CWO_Character_ClientData* _pCDChar, int16 _iTargetObj, int8 _CSCMode, int _MinLookFreedomZ, int _MinLookFreedomY, int _MaxLookFreedomZ, int _MaxLookFreedomY, fp32 _LookBlend, uint8 _DecreaseRate)
{
	// If we were a mounted camera before and on the same object, assume update and check if thresholds have changed that need blend
#ifdef CAMERAUTIL_DEBUG
	{
		CStr Msg = CStrF("Setting camera: Target: %d Mode: %d MFZ: %d MFY: %d MaFZ: %d MaFY: %d Blend: %f",_iTargetObj, _CSCMode, _MinLookFreedomZ,_MinLookFreedomY, _MaxLookFreedomZ,_MaxLookFreedomY, _LookBlend);
		ConOut(Msg);
		Msg+="\n";
		M_TRACEALWAYS(Msg);
	}
#endif
	m_DecreaseRate = _DecreaseRate;
	bool bMustBeMounted = false;//(_CSCMode & 2) != 0;
	int32 MountedMode = CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOW;
	switch (_CSCMode)
	{
	case 1:
		MountedMode = CAMERAUTIL_MODE_MOUNTEDCAM_FREE;
		break;
	case 2:
		MountedMode = CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOW;
		bMustBeMounted = true;
		break;
	case 3:
		MountedMode = CAMERAUTIL_MODE_MOUNTEDCAM_FREE;
		bMustBeMounted = true;
		break;
	case 4:
		MountedMode = CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOWFREE;
		break;
	default:
		MountedMode = CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOW;
		break;
	};
	
	m_MountedLookBlendDuration = (int32)(_LookBlend * _pWPhys->GetGameTicksPerSecond());
	if (m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM && m_iOriginObj == _iTargetObj)
	{
		CWObject_CoreData* pOriginObj = _pWPhys->Object_GetCD(m_iOriginObj);
		CWO_Character_ClientData* pCDOrigin = pOriginObj ? CWObject_Character::GetClientData(pOriginObj) : NULL;
		if (!pCDOrigin)
		{
			m_Mode = 0;
			return false;
		}

		// If previous is follow resetlook (get from current later on)
		if (MountedMode == CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOW && m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOW)
		{
			// Previous mode was follow, find offset
			fp32 Diff = m_PrevMountedLookZ - m_MountedLookZ;
			m_MountedLookZ = MACRO_ADJUSTEDANGLE(pCDOrigin->m_ActionCutSceneCameraOffsetX + m_MountedLookZ);
			m_PrevMountedLookZ = m_MountedLookZ + Diff;
			Diff = m_PrevMountedLookY - m_MountedLookY;
			m_MountedLookY = MACRO_ADJUSTEDANGLE(pCDOrigin->m_ActionCutSceneCameraOffsetY + m_MountedLookY);
			m_PrevMountedLookY = m_MountedLookY + Diff;
		}
		if (MountedMode == CAMERAUTIL_MODE_MOUNTEDCAM_FREE && m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM_FREE)
		{
			// Previous mode was freelook, set offset to current, need to get skeleton (FIXME)
			fp32 Diff = m_PrevMountedLookZ - m_MountedLookZ;
			m_MountedLookZ = m_MountedLookZ - pCDOrigin->m_ActionCutSceneCameraOffsetX;
			m_MountedLookZ = m_MountedLookZ > 0.5f ? m_MountedLookZ - 1.0f : m_MountedLookZ < -0.5f ? m_MountedLookZ + 1.0f : m_MountedLookZ;
			m_PrevMountedLookZ = m_MountedLookZ + Diff;
			Diff = m_PrevMountedLookY - m_MountedLookY;
			m_MountedLookY = m_MountedLookY - pCDOrigin->m_ActionCutSceneCameraOffsetY;
			m_MountedLookY = m_MountedLookY > 0.5f ? m_MountedLookY - 1.0f : m_MountedLookY < -0.5f ? m_MountedLookY + 1.0f : m_MountedLookY;
			m_PrevMountedLookY = m_MountedLookY + Diff;
		}
		if (_CSCMode)
		{
			if (_MaxLookFreedomZ < m_MaxMountedMaxLookAngleZ)
			{
				// Blend back to within range..
				if (m_MountedLookBlendDuration)
				{
					m_MountedLookBlendStart = _pCDChar->m_GameTick + 1;
				}
				else
				{
					// Reset to current values
					fp32 MaxLook = ((fp32)m_MaxMountedMaxLookAngleZ) / 360.0f;
					BindAngleZ(m_MountedLookZ,pCDOrigin->m_ActionCutSceneCameraOffsetX,MaxLook);
				}
			}
			if (_MaxLookFreedomY < m_MaxMountedMaxLookAngleY)
			{
				// Blend back to within range..
				if (m_MountedLookBlendDuration)
				{
					m_MountedLookBlendStart = _pCDChar->m_GameTick + 1;
				}
				else
				{
					fp32 MaxLook = ((fp32)m_MaxMountedMaxLookAngleY) / 360.0f;
					BindAngleY(m_MountedLookY,pCDOrigin->m_ActionCutSceneCameraOffsetY,MaxLook);
				}
			}
		}
	}
	else
	{
		// Copy look from target object
		CWObject_CoreData* pOriginObj = _pWPhys->Object_GetCD(_iTargetObj);
		CWO_Character_ClientData* pCDOrigin = pOriginObj ? CWObject_Character::GetClientData(pOriginObj) : NULL;
		if (pCDOrigin && MountedMode == CAMERAUTIL_MODE_MOUNTEDCAM_FREE)
		{
			m_MountedLookZ = pCDOrigin->m_Control_Look_Wanted[2];
			m_MountedLookY = pCDOrigin->m_Control_Look_Wanted[1];
		}
		else
		{
			m_MountedLookZ = m_MountedLookY = 0;
		}
	}

	m_MinMountedMaxLookAngleZ = _MinLookFreedomZ;
	m_MinMountedMaxLookAngleY = _MinLookFreedomY;
	m_MaxMountedMaxLookAngleZ = _MaxLookFreedomZ;
	m_MaxMountedMaxLookAngleY = _MaxLookFreedomY;

	m_Mode = 0;
	m_Mode = CAMERAUTIL_MODE_ORIGIN_CHARACTER | CAMERAUTIL_MODE_MOUNTEDCAM;
	m_Mode |= MountedMode;
	if (bMustBeMounted)
		m_Mode |= CAMERAUTIL_MODE_MUSTBEMOUNTED;
	// Origin char (where we want to get the anim)
	m_iOriginObj = _iTargetObj;
	return true;
}

void CCameraUtil::AddUserOffset(CMat4Dfp32& _CamMat, fp32 _IPTime)
{
	// Ok, extract angles from direction
	// Add the offsets
	// Transform back into direction and set to camera
	CVec3Dfp32 Direction = CVec3Dfp32::GetMatrixRow(_CamMat,2);
	CVec3Dfp32 Angles = CWObject_Character::GetLook(Direction);

	// Add "x" "y" offsets

	fp32 OffsetX = m_TargetAngleOffsetX;
	fp32 OffsetY = m_TargetAngleOffsetY;
	fp32 LastOffsetX = m_LastAngleOffsetX;
	fp32 LastOffsetY = m_LastAngleOffsetY;

	Angles.k[2] += LERP(LastOffsetX, OffsetX, _IPTime);
	Angles.k[1] += LERP(LastOffsetY, OffsetY, _IPTime);

	/*Direction = VectorFromAngles(Angles.k[1],Angles.k[2]);
	Direction.k[1] = -Direction.k[1]; //hmm?*/

	CMat4Dfp32 Mat;
	Angles.CreateMatrixFromAngles(1,Mat);
	Mat.RotX_x_M(-0.25f);
	Mat.RotY_x_M(0.25f);
	CVec3Dfp32::GetMatrixRow(_CamMat,3).SetMatrixRow(Mat,3);
	_CamMat = Mat;
}

aint CCameraUtil::GetEnginePathMatrix(CWorld_PhysState* _pWPhys, CMTime& _Time, int16 _iEnginePath, CMat4Dfp32* _EPMat)
{
	CWObject_Message EPRM(OBJMSG_HOOK_GETRENDERMATRIX,(aint)&_Time,(aint)_EPMat);
	return _pWPhys->Phys_Message_SendToObject(EPRM,m_iOriginObj);
}

bool CCameraUtil::GetCameraACS(CWorld_PhysState* _pWPhys, CMat4Dfp32& _CamMat, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD)
{
	// Get actioncutscenecamera (usually an enginepath or fixed camera position)
	if (m_Mode & CAMERAUTIL_MODE_ORIGIN_ENGINEPATH)
	{
		// Get camera from enginepath, time on enginepath is given either from 
		// acs or camera start time (I guess)
		// Origin obj = enginepath, targetobj = character, extraobj = acs
		CMTime Time;
		CMat4Dfp32 PosMat;
		bool bGotLadder = CWObject_Phys_Ladder::GetRelativeLadderPosition(_pWPhys,_pCD,
			_pObj->GetPosition(), Time);

//		aint bEPOk = false;
		if (bGotLadder)
		{
			// Scale with enginepath duration
			int32 Scale = _pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ENGINEPATH_GETDURATION,0),
				m_iOriginObj);
			Time.Scale(*(fp32*)&Scale);
		}
		else
		{
			// Get position from startime
			Time = _pCD->m_GameTime - m_CameraStartTime;
		}

		if (GetEnginePathMatrix(_pWPhys, Time, m_iOriginObj, &PosMat))
		{
			// Get the cameramatrix from the enginepath
			_CamMat.r[2] = PosMat.r[0];
			_CamMat.r[1] = M_VNeg(PosMat.r[2]);		// Up
			_CamMat.r[3] = PosMat.r[3];
			// Create the cam matrix
			_CamMat.RecreateMatrix<2,1>();
			return true;
		}
		// Check if we got fixed pos...
		//return false;
	}
	if (m_Mode & CAMERAUTIL_MODE_ORIGIN_FIXEDPOSITION)
	{
		if (m_iOriginObj == 0)
			return false;

		// Just take position from origin object matrix
		// Set the matrix as the object position
		const CMat4Dfp32& PosMat  = _pWPhys->Object_GetPositionMatrix(m_iOriginObj);

		_CamMat.r[2] = PosMat.r[0];
		_CamMat.r[1] = M_VNeg(PosMat.r[2]);		// Up
		_CamMat.r[3] = PosMat.r[3];

		// Create the cam matrix
		_CamMat.RecreateMatrix<2,1>();

		return true;
	}
	if (m_Mode & CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_MASK)
	{
		// Make an automatic camera (some time)
		// Blech....
	}

	ConOut("CCameraUtil: No Proper ACS camera mode set");
	return false;
}

bool CCameraUtil::GetMountedCamera(CWorld_PhysState* _pWPhys, CMat4Dfp32& _CamMat, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, fp32 _IPTime)
{
	CWObject_CoreData* pOriginObj = _pWPhys->Object_GetCD(m_iOriginObj);
	if (!pOriginObj)
		return false;

	if (!(pOriginObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER))
		return false; //TODO: fix this. the code below assumes character...

	CWO_Character_ClientData* pCDOrigin = pOriginObj ? CWObject_Character::GetClientData(pOriginObj) : NULL;
	if (!pCDOrigin)
		return false;

	// Might not have been mounted yet on the other object..
	if ((m_Mode & CAMERAUTIL_MODE_MUSTBEMOUNTED) && (pCDOrigin->m_iMountedObject != _pObj->m_iObject))
		return false;

	fp32 IPTime = pCDOrigin->m_PredictFrameFrac;
	if (_pWPhys->IsClient())
	{
		CWorld_Client* pWClient = (CWorld_Client*)_pWPhys;
		IPTime = pWClient->GetRenderTickFrac();
		Interpolate2(pOriginObj->GetLastPositionMatrix(), pOriginObj->GetPositionMatrix(), _CamMat, IPTime);
		if (pCDOrigin->m_RenderAttached != 0)
		{
			CWObject_Client *pObj = pWClient->Object_Get(pCDOrigin->m_RenderAttached);
			if(pObj)
			{
				CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)&_CamMat, pCDOrigin->m_iAttachJoint);
				pWClient->ClientMessage_SendToObject(Msg, pCDOrigin->m_RenderAttached);
			}
		}

		if((pCDOrigin->m_DialogueInstance.m_SubtitleFlags & SUBTITLE_TYPE_MASK) == SUBTITLE_TYPE_CUTSCENE)
		{
			CWorld_ClientCore *pWClientCore = safe_cast<CWorld_ClientCore>(pWClient);
			CWObject_Client *pObjOriginClient = pWClient->Object_Get(m_iOriginObj);
			if(pWClientCore->m_spSound != NULL)
			{
				int iHold = pCDOrigin->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_VOCAPINFO) & AG2_VOCAP_FLAG_IDLE;
				int nLayers = 0;
				pCDOrigin->m_VoCap.Eval(pWClientCore->m_spSound, pObjOriginClient->m_ClientData[0], pCDOrigin->m_GameTime, iHold, NULL, nLayers, 0);
				CVoCap_AnimItem *pQueue = pCDOrigin->m_VoCap.m_spAnimQueue;
				if(pQueue && pQueue->m_spCurSequence)
				{
					if (pCDOrigin->m_RenderAttached == 0)
						_CamMat = pCDOrigin->m_VocapOrigin;
					CMat4Dfp32 RotMat,Temp;
					vec128 Move;
					CQuatfp32 Rot;
					pQueue->m_spCurSequence->EvalTrack0(CMTime::CreateFromSeconds(pQueue->m_Time), Move, Rot);
					Move = M_VSetW1(M_VMul(Move, M_VLdScalar(_pCD->m_CharGlobalScale * (1.0f - _pCD->m_CharSkeletonScale))));
					Rot.CreateMatrix(RotMat);
					_CamMat.Multiply3x3(RotMat,Temp);
					Move = M_VMulMat4x3(Move, _CamMat);
					_CamMat = Temp;
					_CamMat.r[3] = Move;
				}
			}
		}
	}
	else
	{
		if (pCDOrigin->m_RenderAttached != 0)
		{
			CWObject_CoreData* pObj = _pWPhys->Object_GetCD(pCDOrigin->m_RenderAttached);
			if(pObj)
			{
				CMTime Time = pCDOrigin->m_GameTime;
				CWObject_Message Msg(OBJMSG_HOOK_GETTIME);
				Msg.m_pData = &Time;
				Msg.m_DataSize = sizeof(CMTime);
				_pWPhys->Phys_Message_SendToObject(Msg, pCDOrigin->m_RenderAttached);
				Msg = CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)&Time, (aint)&_CamMat);
				if (!_pWPhys->Phys_Message_SendToObject(Msg, pCDOrigin->m_RenderAttached))
					_CamMat = pOriginObj->GetPositionMatrix();
			}
		}
		else
		{
			_CamMat = pOriginObj->GetPositionMatrix();
		}
	}

	// First person camera
	CXR_Model* pModel = _pWPhys->GetMapData()->GetResource_Model(pOriginObj->m_iModel[0]);
	CXR_Model* pModel1 = _pWPhys->GetMapData()->GetResource_Model(pOriginObj->m_iModel[1]);
	CXR_Skeleton *pSkel1 = pModel1 ? pModel1->GetPhysSkeleton() : NULL;
	CXR_Skeleton *pSkel0 = pModel ? pModel->GetPhysSkeleton() : NULL;

	CXR_Skeleton *pSkel = pSkel1;
	if(!pSkel || (pSkel0 && pSkel0->m_lNodes.Len() > pSkel->m_lNodes.Len()))
		pSkel = pSkel0;

	CXR_AnimState Anim;
	Anim.m_Anim0 = -2;
	if (!CWObject_Character::OnGetAnimState(pOriginObj, _pWPhys, pSkel, 1, _CamMat, IPTime, Anim, NULL))
		return false;

	CXR_SkeletonInstance* pSkelInst = Anim.m_pSkeletonInst;

	if (!pModel || !pSkel)
	{
		_CamMat.RotX_x_M(-0.25f);
		_CamMat.RotY_x_M(0.25f);
		fp32 Offset = CWObject_Character::Char_GetPhysType(pOriginObj) == PLAYER_PHYS_CROUCH ? 30.0f : 60.0f;
		CVec3Dfp32::GetMatrixRow(_CamMat,3).k[2] += Offset;
		return false;
	}

	CMat4Dfp32 Trans;
	vec128 Point = M_VZero();

	int RotTrack = PLAYER_ROTTRACK_CAMERA;
	if (pSkelInst && RotTrack < pSkelInst->m_nBoneTransform)
	{
		Point = pSkel->m_lNodes[PLAYER_ROTTRACK_CAMERA].m_LocalCenterv;
		Trans = pSkelInst->GetBoneTransform(PLAYER_ROTTRACK_CAMERA);
	}
	else
		Trans.Unit();

	const CXR_SkeletonAttachPoint* pPoint = pSkel->GetAttachPoint(PLAYER_ATTACHPOINT_FACE);
	if (pPoint)
		Point = pPoint->m_LocalPos.r[3];

	vec128 IdealPosition = M_VMulMat4x3(Point, Trans);
	// Put offsets to the angle...

	//if (m_Mode & (CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOWFREE|CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOW))
	{
		_CamMat.r[3] = IdealPosition;
		_CamMat.r[2] = Trans.r[0];
		_CamMat.r[0] = M_VNeg(Trans.r[1]);
		_CamMat.RecreateMatrix<2,0>();
	}
	/*else if (m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOW)
	{
		// Follow anim camera, only go to maximum angle though (so we can't look through our chest etc...)
		fp32 MaxLookZ = Max(((fp32)m_MaxMountedMaxLookAngleZ) / 360.0f, Max(m_MountedLookZ,-m_MountedLookZ));
		fp32 MaxLookY = Max(((fp32)m_MaxMountedMaxLookAngleY) / 360.0f, Max(m_MountedLookY,-m_MountedLookY));
		fp32 MinLookZ = Max(((fp32)m_MinMountedMaxLookAngleZ) / 360.0f, Max(m_MountedLookZ,-m_MountedLookZ));
		fp32 MinLookY = Max(((fp32)m_MinMountedMaxLookAngleY) / 360.0f, Max(m_MountedLookY,-m_MountedLookY));
		CMat4Dfp32 PosMat = pOriginObj->GetLocalPositionMatrix();
		PosMat.GetRow(0).k[2] = 0.0f;
		PosMat.GetRow(0).Normalize();
		PosMat.RecreateMatrix(0,2);
		//CVec3Dfp32 LookAnglesRef = CWObject_Character::GetLook(CVec3Dfp32::GetMatrixRow(PosMat,0));
		CVec3Dfp32 LookAngles = CWObject_Character::GetLook(CVec3Dfp32::GetMatrixRow(Trans,0));
		CVec3Dfp32 LookAnglesRef = LookAngles;
		LookAngles[2] += LERP(m_PrevMountedLookZ,m_MountedLookZ, _IPTime);
		LookAngles[1] += LERP(m_PrevMountedLookY,m_MountedLookY, _IPTime);

		LookAngles[1] = Min(MaxLookY + LookAnglesRef[1], Max(-MinLookY + LookAnglesRef[1], LookAngles[1]));
		LookAngles[2] = Min(MaxLookZ + LookAnglesRef[2], Max(-MinLookZ + LookAnglesRef[2], LookAngles[2]));
		LookAngles.CreateMatrixFromAngles(0, Trans);;

		_CamMat.GetRow(3) = IdealPosition;
		_CamMat.GetRow(2) = Trans.GetRow(0);
		_CamMat.GetRow(0) = -Trans.GetRow(1);
		_CamMat.RecreateMatrix(2,0);
	}
	else
	{
		// Assume free
		CVec3Dfp32 LookAngles = CWObject_Character::GetLook(CVec3Dfp32::GetMatrixRow(Trans,0));
		LookAngles[2] = LERP(m_PrevMountedLookZ,m_MountedLookZ, _IPTime);
		LookAngles[1] = LERP(m_PrevMountedLookY,m_MountedLookY, _IPTime);
		LookAngles.CreateMatrixFromAngles(0, Trans);;


		// Recreate cam
		_CamMat.GetRow(3) = IdealPosition;
		_CamMat.GetRow(2) = Trans.GetRow(0);
		_CamMat.GetRow(0) = -Trans.GetRow(1);
		_CamMat.RecreateMatrix(2,0);
	}*/

	return true;
}

CVec3Dfp32 CCameraUtil::GetMountedAngles(fp32 _IPTime)
{
	return CVec3Dfp32(0.0f,LERP(m_PrevMountedLookY,m_MountedLookY, _IPTime),LERP(m_PrevMountedLookZ,m_MountedLookZ, _IPTime));
}

void CCameraUtil::UpdateMountedLook(CWorld_PhysState* _pWPhys, CWO_Character_ClientData* _pPlayerCD, const CVec3Dfp32& _dLook)
{
	// Make sure we didn't exceed boundary
	CWObject_CoreData* pOriginObj = _pWPhys->Object_GetCD(m_iOriginObj);
	if (!pOriginObj)
		return;

	if (!(pOriginObj->GetPhysState().m_ObjectFlags & (OBJECT_FLAGS_CHARACTER|OBJECT_FLAGS_PLAYER)))
		return; // TODO: Fix the code below! it assumes characters, but other objects end up here...

	CWO_Character_ClientData* pCDOrigin = pOriginObj ? CWObject_Character::GetClientData(pOriginObj) : NULL;
	if (!pCDOrigin)
		return;

	CVec3Dfp32 dLook = _dLook * 0.5f;

	if (m_Mode & (CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOW|CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOWFREE))
	{
		fp32 MaxLookZ = Max(((fp32)m_MaxMountedMaxLookAngleZ) / 360.0f, Max(m_MountedLookZ,-m_MountedLookZ));
		fp32 MaxLookY = Max(((fp32)m_MaxMountedMaxLookAngleY) / 360.0f, Max(m_MountedLookY,-m_MountedLookY));
		fp32 MinLookZ = Max(((fp32)m_MinMountedMaxLookAngleZ) / 360.0f, Max(m_MountedLookZ,-m_MountedLookZ));
		fp32 MinLookY = Max(((fp32)m_MinMountedMaxLookAngleY) / 360.0f, Max(m_MountedLookY,-m_MountedLookY));
		fp32 ScaleZ = MaxLookZ / 0.25f;
		fp32 ScaleY = MaxLookY / 0.2f;
		fp32 OldMLZ = m_MountedLookZ;
		fp32 OldMLY = m_MountedLookY;
		m_MountedLookY = Min(MaxLookY, Max(-MinLookY, m_MountedLookY + dLook[1] * ScaleY));
		m_MountedLookZ = Min(MaxLookZ, Max(-MinLookZ, m_MountedLookZ + dLook[2] * ScaleZ));
		m_PrevMountedLookY = Min(MaxLookY, Max(-MinLookY, m_PrevMountedLookY + dLook[1] * ScaleY));
		m_PrevMountedLookZ = Min(MaxLookZ, Max(-MinLookZ, m_PrevMountedLookZ + dLook[2] * ScaleZ));
		m_LastLookTick = _pPlayerCD->m_GameTick;
	}
	else
	{
		fp32 NextMountedLookY = Min(0.249f, Max(-0.249f, m_MountedLookY + dLook[1]));
		fp32 NextMountedLookZ = M_FMod((fp32)m_MountedLookZ + dLook[2] + 2.0f, 1.0f);
		fp32 MaxLookZ,MaxLookY;
		if (m_MountedLookBlendStart)
		{
			// Ok then, what we need to do here is make sure we don't "increase" the angle or turn it
			// over the threshold
			fp32 MaxRealLookZ = ((fp32)m_MaxMountedMaxLookAngleZ) / 360.0f;
			fp32 MaxRealLookY = ((fp32)m_MaxMountedMaxLookAngleY) / 360.0f;
			//fp32 MinRealLookZ = ((fp32)m_MinMountedMaxLookAngleZ) / 360.0f;
			//fp32 MinRealLookY = ((fp32)m_MinMountedMaxLookAngleY) / 360.0f;
			fp32 DiffBefore = m_MountedLookZ - pCDOrigin->m_Control_Look_Wanted[2];
			DiffBefore = (DiffBefore > 0.5f ? DiffBefore - 1.0f : (DiffBefore < -0.5f ? DiffBefore + 1.0f : DiffBefore));
			MaxLookZ = Max(DiffBefore,-DiffBefore);
			fp32 DiffAfter = NextMountedLookZ - pCDOrigin->m_Control_Look_Wanted[2];
			DiffAfter = (DiffAfter > 0.5f ? DiffAfter - 1.0f : (DiffAfter < -0.5f ? DiffAfter + 1.0f : DiffAfter));
			if ((DiffBefore > 0.0f && DiffAfter < -MaxRealLookZ) ||
				(DiffBefore < 0.0f && DiffAfter > MaxRealLookZ))
				MaxLookZ = MaxRealLookZ;

			DiffBefore = m_MountedLookY - pCDOrigin->m_Control_Look_Wanted[1];
			DiffBefore = (DiffBefore > 0.5f ? DiffBefore - 1.0f : (DiffBefore < -0.5f ? DiffBefore + 1.0f : DiffBefore));
			MaxLookY = Max(DiffBefore,-DiffBefore);
			DiffAfter = NextMountedLookY - pCDOrigin->m_Control_Look_Wanted[1];
			DiffAfter = (DiffAfter > 0.5f ? DiffAfter - 1.0f : (DiffAfter < -0.5f ? DiffAfter + 1.0f : DiffAfter));
			if ((DiffBefore > 0.0f && DiffAfter < -MaxRealLookY) ||
				(DiffBefore < 0.0f && DiffAfter > MaxRealLookY))
				MaxLookY = MaxRealLookY;
		}
		else
		{
			MaxLookZ = ((fp32)m_MinMountedMaxLookAngleZ) / 360.0f;
			MaxLookY = ((fp32)m_MinMountedMaxLookAngleY) / 360.0f;
		}
		BindAngleZ(NextMountedLookZ, pCDOrigin->m_Control_Look_Wanted[2], MaxLookZ);
		m_PrevMountedLookZ += NextMountedLookZ - m_MountedLookZ;
		m_MountedLookZ = NextMountedLookZ;

		BindAngleY(NextMountedLookY, pCDOrigin->m_Control_Look_Wanted[1], MaxLookY);
		// Meh, FIX
		m_PrevMountedLookY += NextMountedLookY - m_MountedLookY;
		m_MountedLookY = NextMountedLookY;
	}
}

void CCameraUtil::Refresh(CWorld_PhysState* _pWPhys, CWO_Character_ClientData* _pCD)
{
	// Update once per tick on both client/server
	if (!IsActive())
		return;
	
	if (m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM) 
	{
		// Update mounted camera blend values
		CWObject_CoreData* pOriginObj = _pWPhys->Object_GetCD(m_iOriginObj);
		if (!pOriginObj)
			return;

		if (!(pOriginObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER))
		{
			M_TRACE("ERROR: CCameraUtil::Refresh, mounted cam on non-character!\n");
			return;
		}
		CWO_Character_ClientData* pCDOrigin = pOriginObj ? CWObject_Character::GetClientData(pOriginObj) : NULL;
		if (!pCDOrigin)
			return;

		m_PrevMountedLookZ = m_MountedLookZ;
		m_PrevMountedLookY = m_MountedLookY;

		if (m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOW)
		{
			//ConOutL(CStrF("%s Z: %f Y: %f", _pWPhys->IsServer() ? "S:" : "C:", m_MountedLookZ, m_MountedLookY));
			// Blend to 0
			if ((_pCD->m_GameTick - m_LastLookTick) > TruncToInt(_pWPhys->GetGameTicksPerSecond() * 0.5f))
			{
				fp32 ExtraModifier = Min((((fp32)(_pCD->m_GameTick - m_LastLookTick)) - _pWPhys->GetGameTicksPerSecond() * 0.5f) * _pWPhys->GetGameTickTime(), 1.0f);
				fp32 Modifier = (100.0f - ExtraModifier * ((fp32)(m_DecreaseRate)) * 20.0f * _pWPhys->GetGameTickTime()) * 0.01f;
				m_MountedLookZ *= Modifier;
				m_MountedLookY *= Modifier;
			}
		}
		else if (m_MountedLookBlendStart && (m_Mode & CAMERAUTIL_MODE_MOUNTEDCAM_FREE))
		{
			// Check how much time we have left
			fp32 TimeLeft = (fp32)(m_MountedLookBlendDuration - (int32)(_pCD->m_GameTick - m_MountedLookBlendStart));
			fp32 MaxLook = ((fp32)m_MaxMountedMaxLookAngleZ) / 360.0f;
			if (TimeLeft <= 0.0f)
			{
				m_MountedLookBlendStart = 0;
				// Reset to current values
				BindAngleZ(m_MountedLookZ, pCDOrigin->m_Control_Look_Wanted[2], MaxLook);
				BindAngleY(m_MountedLookY, pCDOrigin->m_Control_Look_Wanted[1], MaxLook);
			}
			else
			{
//				fp32 Prev = m_MountedLookZ;
				BlendAngle(m_MountedLookZ, m_PrevMountedLookZ, pCDOrigin->m_Control_Look_Wanted[2],MaxLook, TimeLeft);
				MaxLook = ((fp32)m_MaxMountedMaxLookAngleY) / 360.0f;
				BlendAngle(m_MountedLookY, m_PrevMountedLookY, pCDOrigin->m_Control_Look_Wanted[1],MaxLook, TimeLeft);
			}
		}
	}
}

// Check if given dialog camera is valid
bool CCameraUtil::CheckDialogCamera(CWorld_PhysState* _pWPhys, CMat4Dfp32& _NewCamera, fp32 _FOV)
{
	// Collision
	CVec3Dfp32 Start, Stop;
	CCollisionInfo CInfo;
	CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	CInfo.SetReturnValues(0);

	int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
	int32 ObjectFlags = OBJECT_FLAGS_WORLD;// | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;

	// Previous matrix was unit matrix so we know that this is the first GetCameraAt called since dialogue begun
	CWObject_CoreData* pSpeakerObj = _pWPhys->Object_GetCD(m_iTargetObj);
	CWObject_CoreData* pListenerObj = _pWPhys->Object_GetCD(m_iOriginObj);
	CWO_Character_ClientData* pSpeakerCD = pSpeakerObj ? CWObject_Character::GetClientData(pSpeakerObj) : NULL;
	CWO_Character_ClientData* pListenerCD = pListenerObj ? CWObject_Character::GetClientData(pListenerObj) : NULL;
	if (!(pSpeakerCD && pListenerCD)) return false;

	//
	// Make sure the camera is seen from the listener (so the camera isn't inside a volume)
	//
	Start = pListenerObj->GetPosition() + pListenerCD->m_Camera_StandHeadOffset;
	Stop = CVec3Dfp32::GetMatrixRow(_NewCamera, 3);
	if (_pWPhys->Phys_IntersectLine(Start, Stop, OwnFlags, ObjectFlags, MediumFlags, &CInfo, m_iOriginObj))
	{
		// Collision. This camera is invalid
		_pWPhys->Debug_RenderWire(Start, Stop, 0xffff0000, 10.0f);
		return false;
	}
	else
	{
		_pWPhys->Debug_RenderWire(Start, Stop, 0xff00ff00, 10.0f);
	}

/*
	//
	// Make sure the camera doesn't try to move inside a volume
	//
	Start = CVec3Dfp32::GetMatrixRow(_PrevCamera, 3);
	Stop = CVec3Dfp32::GetMatrixRow(_NewCamera, 3);
	if (_pWPhys->Phys_IntersectLine(Start, Stop, OwnFlags, ObjectFlags, MediumFlags, &CInfo))
	{
		_pWPhys->Debug_RenderWire(Start,Stop,0xffff00f0,20.0f,false);
	}
	else
	{
		// No collision
		_pWPhys->Debug_RenderWire(Start,Stop,0xff00fff0,20.0f,false);
	}

	//
	// Make sure both speaker and listener are in camera view
	//
	CVec3Dfp32 p = pListenerObj->GetPosition() + pListenerCD->m_Camera_StandHeadOffset;
	if (!CWO_DialogueCamera::IsInView(_NewCamera, p, _FOV, 1.0f))
	{
		// Listener was not in view
		//		ConOut(CStr("Listener was not in view"));
	}*/

	return true;
}

bool CCameraUtil::GetCameraDialog(CWorld_PhysState* _pWPhys, CMat4Dfp32& _CamMat, CMTime _Time)
{
	// Get dialog camera
	CVec3Dfp32 CameraPos,LookAtPos;
	switch (m_Type)
	{
	case CAMERAUTIL_DIALOGTYPE_OVERSHOULDER_LEFT:
	case CAMERAUTIL_DIALOGTYPE_OVERSHOULDER_RIGHT:
		{
			LookAtPos = m_PosSpeakerLookat;
			GetCameraPosition_OverShoulder(_pWPhys,_Time,CameraPos);
			break;
		}
	case CAMERAUTIL_DIALOGTYPE_NEARSHOULDER_LEFT:
	case CAMERAUTIL_DIALOGTYPE_NEARSHOULDER_RIGHT:
		{
			LookAtPos = m_PosSpeakerLookat;
			GetCameraPosition_NearOverShoulder(_pWPhys,_Time,CameraPos);
			break;
		}
	case CAMERAUTIL_DIALOGTYPE_TWOSHOT_LEFT:
	case CAMERAUTIL_DIALOGTYPE_TWOSHOT_RIGHT:
		{
			LookAtPos = LERP(m_PosSpeakerLookat,m_PosListenerLookat,0.5f);
			GetCameraPosition_TwoShot(_pWPhys,_Time,CameraPos);
			break;
		}
	case CAMERAUTIL_DIALOGTYPE_CLOSEBB_LEFT:
	case CAMERAUTIL_DIALOGTYPE_CLOSEBB_RIGHT:
		{
			LookAtPos = m_PosSpeakerLookat;
			GetCameraPosition_SafeBoundingBox(_pWPhys, _Time, CameraPos, LookAtPos);
			break;
		}
	default:
		return false;
	}
	
	vec128 CameraPosv=M_VLd_P3_Slow(&CameraPos);
	vec128 Dir = M_VSub(M_VLd_P3_Slow(&LookAtPos), CameraPosv);
	Dir = M_VNrm3(Dir);
	_CamMat.r[2] = Dir;
	_CamMat.r[3] = CameraPosv;
	_CamMat.r[1] = M_VConst(0,0,-1.0f,0);
	_CamMat.RecreateMatrix<2,1>();
	return true;
}

bool CCameraUtil::GetCamera(CWorld_PhysState* _pWPhys, CMat4Dfp32& _CamMat, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, fp32 _IPTime)
{
	// Camera mapping
	// 0 -> 2  (dir)
	// 1 -> 0  (right)
	// 2 -> 1  (up)
	// 3 -> 3  (pos)
	// Depending on camera mode, get camera and view position
	int32 Type = m_Mode & CAMERAUTIL_TYPEMASK;
	bool bRetVal = false;
	switch (Type)
	{
	case CAMERAUTIL_MODE_ACS:
		{
			bRetVal = GetCameraACS(_pWPhys,_CamMat, _pObj,_pCD);
			break;
		}
	case CAMERAUTIL_MODE_DIALOG:
		{
			bRetVal = GetCameraDialog(_pWPhys,_CamMat, _pCD->m_GameTime - m_CameraStartTime);
			break;
		}
	case CAMERAUTIL_MODE_MOUNTEDCAM:
		{
			bRetVal = GetMountedCamera(_pWPhys, _CamMat, _pObj,_pCD, _IPTime);
			break;
		}
	default:
		{
			// Attempt to get the automatic camera
			CMTime Time = _pCD->m_GameTime - m_CameraStartTime;
			CVec3Dfp32 ViewPos,TargetPos;
			if (GetCameraViewPos(_pWPhys,Time,ViewPos) && GetCameraTargetPos(_pWPhys,Time,TargetPos))
			{
				CVec3Dfp32 Dir = TargetPos - ViewPos;
				Dir.Normalize();
				Dir.SetMatrixRow(_CamMat,2);
				CVec3Dfp32(0.0f,0.0f,-1.0f).SetMatrixRow(_CamMat,1);
				_CamMat.RecreateMatrix(2,1);
				ViewPos.SetMatrixRow(_CamMat,3);

				/*_CamMat.Unit();
				CVec3Dfp32(0.0f,0.0f,1.0f).SetMatrixRow(_CamMat,1);
				CVec3Dfp32(1.0f,0.0f,0.0f).SetMatrixRow(_CamMat,2);
				_CamMat.RecreateMatrix(2,1);
				ViewPos.SetMatrixRow(_CamMat,3);*/

				bRetVal = true;
			}
			break;
		}
	}
	if (bRetVal && (m_Mode & CAMERAUTIL_MODE_USERINPUT))
		AddUserOffset(_CamMat,_IPTime);

	return bRetVal;
}

fp32 CCameraUtil::GetFOV()
{
	return m_FOV;
}

void CCameraUtil::UpdateOffsetVars()
{
	m_AngleOffsetX -= m_AngleOffsetX * 0.05f;
	m_AngleOffsetY -= m_AngleOffsetY * 0.05f;

	// Update angle offsets
	//ConOut(CStrF("Massa update: %f", _pWPhys->GetGameTime()));
	int32 ModSpeed = 100;
	fp32 WantedAngleOffset = m_AngleOffsetX * 100;
	m_LastAngleOffsetX = m_TargetAngleOffsetX;
	m_TargetAngleOffsetX *= 100.0f;
	m_AngleOffsetXChange *= 100.0f;
	Moderatef(m_TargetAngleOffsetX, WantedAngleOffset, m_AngleOffsetXChange, 
		ModSpeed);
	m_TargetAngleOffsetX *= (1.0f/100.0f);
	m_AngleOffsetXChange *= (1.0f/100.0f);

	WantedAngleOffset = m_AngleOffsetY * 100;
	m_LastAngleOffsetY = m_TargetAngleOffsetY;
	m_TargetAngleOffsetY *= 100.0f;
	m_AngleOffsetYChange *= 100.0f;
	Moderatef(m_TargetAngleOffsetY, WantedAngleOffset, m_AngleOffsetYChange, 
		ModSpeed);
	m_TargetAngleOffsetY *= (1.0f/100.0f);
	m_AngleOffsetYChange *= (1.0f/100.0f);

}

void CCameraUtil::SetCamOffsets(fp32 _OffsetX, fp32 _OffsetY)
{
	m_AngleOffsetX = _OffsetX;
	m_AngleOffsetY = _OffsetY;
}

void CCameraUtil::BindAngleZ(fp32& _Angle, fp32 _Reference, fp32 _MaxLook)
{
	fp32 Diff = _Angle - _Reference;
	Diff = (Diff > 0.5f ? Diff - 1.0f : (Diff < -0.5f ? Diff + 1.0f : Diff));
	if (Diff  > 0.0f)
		Diff = Min(_MaxLook,Diff);
	else
		Diff = Max(-_MaxLook,Diff);

	fp32 NewVal = _Reference + Diff;
	_Angle = (NewVal > 1.0f ? NewVal - 1.0f : (NewVal < 0.0f ? NewVal + 1.0f : NewVal));
}

void CCameraUtil::BindAngleY(fp32& _Angle, fp32 _Reference, fp32 _MaxLook)
{
	fp32 Diff = _Angle - _Reference;
	Diff = (Diff > 0.5f ? Diff - 1.0f : (Diff < -0.5f ? Diff + 1.0f : Diff));
	if (Diff  > 0.0f)
		Diff = Min(_MaxLook,Diff);
	else
		Diff = Max(-_MaxLook,Diff);

	_Angle = _Reference + Diff;
}

void CCameraUtil::BlendAngle(fp32& _Angle, fp32& _PrevAngle, fp32 _Reference, fp32 _MaxLook, fp32 _TimeLeft)
{
	fp32 Diff = _Angle - _Reference;
	Diff = (Diff > 0.5f ? Diff - 1.0f : (Diff < -0.5f ? Diff + 1.0f : Diff));
	if (Diff  > 0.0f)
	{
		_Angle += (_MaxLook - Diff) / _TimeLeft;
	}
	else
	{
		_Angle += (-_MaxLook - Diff) / _TimeLeft; //Diff = Max(-MaxLook,Diff);
	}
}
