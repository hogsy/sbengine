#include "PCH.h"
#include "WObj_DialogueCamera.h"
#include "../Client/WClient.h"
#include "../Server/WServer.h"
#include "WObj_System.h"
#include "../../GameContext/WGameContext.h"

CDialogueToken CWO_DialogueCamera::ms_ValidToken;
/* = 
{
0,//int m_Delay;
0,//int m_StartGameTick;

0,//int m_CameraMode;
int m_CameraModeParameter;
CVec3Dfp32 m_CameraSpeakerOffset;
CVec3Dfp32 m_CameraListenerOffset;
fp32 m_FOV;
fp32 m_dFOV;
};
*/

CWO_DialogueCamera::CWO_DialogueCamera() 
{ 
	m_pCamShakeAnimBase = NULL; 
	m_bFirstFrameIn3PI = false; 
	SetMode(DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT); 
	m_bForceRightShoulder = false; 
	m_TickToNewCamera = 0;
}

void CWO_DialogueCamera::Init(CMapData* _pMapData)
{
	SetupCameraTemplates(_pMapData);
}

// -- Camera positions ----------------------------------------------------------------------------
void CWO_DialogueCamera::GetCameraPosition(CVec3Dfp32& _Camera)
{
	/*switch(m_Mode_Camera)
	{
	case DIALOGUECAMERA_CAMERA_ENGINEPATH:
	GetCameraPosition_EnginePath(_Camera);
	break;

	case DIALOGUECAMERA_CAMERA_ATTACHPOINT:
	GetCameraPosition_AttachPoint(_Camera);
	break;

	case DIALOGUECAMERA_CAMERA_OVERSHOULDER:
	GetCameraPosition_OverShoulder(_Camera);
	break;

	case DIALOGUECAMERA_CAMERA_NEAROVERSHOULDER:
	GetCameraPosition_NearOverShoulder(_Camera);
	break;

	case DIALOGUECAMERA_CAMERA_TWOSHOT:
	GetCameraPosition_TwoShot(_Camera);
	break;

	case DIALOGUECAMERA_CAMERA_SPEAKERHEAD:
	GetCameraPosition_SpeakerHead(_Camera);
	break;
	}

	_Camera += m_pDialogueToken->m_CameraListenerOffset;*/
}

void CWO_DialogueCamera::GetCameraPosition_EnginePath(CVec3Dfp32& _Camera)
{
	GetEnginePathPosition(_Camera, m_Time);
}

void CWO_DialogueCamera::GetCameraPosition_AttachPoint(CVec3Dfp32& _Camera)
{
}


void CWO_DialogueCamera::GetCameraPosition_SafeBoundingBox(CVec3Dfp32& _Camera, const CVec3Dfp32& _LookAtPos)
{
	// Get camera position that is safe inside the bounding box 
	// Find x that is farthest away

	CWObject_CoreData* pObj = NULL;

	if(m_pWClient)
		pObj = m_pWClient->Object_GetCD(m_iSafeBB);

	if (!pObj)
	{
		GetCameraPosition_TwoShot(_Camera, 0);
		return;
	}

	CVec3Dfp32 CameraVal(0.0f, 0.0f, 0.0f);
	CStr ConStr;

	if(m_pWClient)
	{
		// Get camera tweak values(xyz) as a string 
		CStr ConStr;
		ConStr = m_pWClient->Registry_GetGame()->GetValue("XYZ_Camera");

		CameraVal.ParseString(ConStr);
	}

	CVec3Dfp32 Right;
	CVec3Dfp32 DirToLook = _LookAtPos - pObj->GetPosition();
	DirToLook.k[2] = 0.0f;
	DirToLook.Normalize();
	DirToLook.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),Right);
	Right = -Right;
	//if (m_Mode_Flipped == true)
	//Right = -Right;
	CBox3Dfp32 BoundBox = *pObj->GetAbsBoundBox();

	CVec3Dfp32 P0 = pObj->GetPosition();
	// Make sure we're inside the BB
	P0.k[2] += 10.0f;
	CVec3Dfp32 P1 = P0 + Right * 15.0f;
	fp32 MaxHitSqr = 0.0f;
	for (int32 i = 0; i < 4; i++)
	{
		CVec3Dfp32 HitPos;
		if (BoundBox.IntersectLine(P1, P0, HitPos))
			MaxHitSqr = Max((HitPos - P0).LengthSqr(),MaxHitSqr);
	}

	if (!(MaxHitSqr > 0.0f))
	{
		GetCameraPosition_TwoShot(_Camera, 0);
		return;
	}

	// A little inside for safety
	MaxHitSqr = M_Sqrt(MaxHitSqr) * 0.95f;

	_Camera = CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3) + Right * MaxHitSqr + m_HeadOfsSpeaker;
	_Camera.k[2] -= 5.0f;
	int a = 0;

	CMat4Dfp32 DebugMat;
	DebugMat.Unit();
	CVec3Dfp32 Os(0.0f, -10.0f, 0.0f);
	DebugMat = m_MatSpeaker;
	DebugMat.GetRow(3) += m_HeadOfsSpeaker;
	DebugMat.GetRow(3).k[2] -= 4.0f;
	DebugMat.GetRow(3) = Os * DebugMat;

	//if(m_pWClient)
	//m_pWClient->Debug_RenderMatrix(DebugMat, false);

	//_Camera = CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3) + Right * MaxHitSqr + m_HeadOfsSpeaker + CameraVal;
}

// Not testing against bounding box at the moment..
void CWO_DialogueCamera::GetCameraPosition_TestBoundingBox(CMat4Dfp32& _Mat)
{
	CVec3Dfp32 Offset(-10.0f, -15.0f, 0.0f);

	CVec3Dfp32 CameraVal(0.0f, 0.0f, 0.0f);
	CStr ConStr;


	if(m_pWClient)
	{
		// Get camera tweak values(xyz) as a string(if we got one)
		CStr ConStr;
		ConStr = m_pWClient->Registry_GetGame()->GetValue("XYZ_Camera");
		CameraVal.ParseString(ConStr);
	}

	// Check mode(speaker/listener) flip
	if(m_Mode_Flipped)
	{
		Offset.k[1] = -Offset.k[1];
		CameraVal.k[1] = -CameraVal.k[1];
	}

	// Add tweak values(if any..)
	Offset += CameraVal;

	CVec3Dfp32 ListenerPos, LookDirection;
	ListenerPos = m_MatListener.GetRow(3) + m_HeadOfsListener;
	LookDirection = ListenerPos - (m_MatSpeaker.GetRow(3) + m_HeadOfsSpeaker);

	CMat4Dfp32 SpeakerMat = m_MatSpeaker;
	// Force direction to speaker
	SpeakerMat.Unit3x3();
	LookDirection.SetMatrixRow(SpeakerMat, 0);
	SpeakerMat.RecreateMatrix(0, 2);

	SpeakerMat.GetRow(3) += m_HeadOfsSpeaker;
	SpeakerMat.GetRow(3).k[2] -= 4.0f;
	SpeakerMat.GetRow(3) = Offset * SpeakerMat;

	// Set new translation part
	_Mat.Unit();
	_Mat.GetRow(3) = SpeakerMat.GetRow(3);

	// Set direction and recreate matrix
	CVec3Dfp32 Direction = ListenerPos - _Mat.GetRow(3);
	Direction.SetMatrixRow(_Mat, 0);
	_Mat.RecreateMatrix(0, 2);

	//if(m_pWClient)
	//m_pWClient->Debug_RenderMatrix(_Mat, false);
}

void CWO_DialogueCamera::GetCameraPosition_LookAtSpeaker(CMat4Dfp32& _Mat)
{
	CVec3Dfp32 Offset(15.0f, 0.0f, -4.0f);
	CVec3Dfp32 LookatOffset(0.0f, 0.0f, -3.0f);

	// with this camera, the offset values represent lookat offset and camera position offset
	LookatOffset += m_pDialogueToken->m_CameraListenerOffset;
	Offset += m_pDialogueToken->m_CameraSpeakerOffset;
	
	CMat4Dfp32 Temp = m_MatListener;
	Temp.GetRow(3) = 0.0f;
	CVec3Dfp32 OffsetLocal = Offset * Temp;
	CVec3Dfp32 LookOffsetLocal = LookatOffset * Temp;
	CVec3Dfp32 ListenerMatAndHeadofs = (m_MatListener.GetRow(3) + m_HeadOfsListener);
	_Mat.GetRow(3) = ListenerMatAndHeadofs + OffsetLocal;

	CVec3Dfp32 LookAtThisPoint = ListenerMatAndHeadofs + LookOffsetLocal;
	CVec3Dfp32 LookDirection = LookAtThisPoint - _Mat.GetRow(3);
	_Mat.GetRow(0) = LookDirection;
	_Mat.RecreateMatrix(0,2);
	

	/*
	CVec3Dfp32 Offset(0.0f, -10.0f, 0.0f);

	CVec3Dfp32 CameraVal(0.0f, 0.0f, 0.0f);
	CStr ConStr;

	CVec3Dfp32 ListenerPos, LookDirection, TempSpeakerPos, HeadOffsetSpeaker;
	CMat4Dfp32 SpeakerMat;

	if(m_Mode_Flipped)
	{
		TempSpeakerPos = m_MatListener.GetRow(3);
		ListenerPos = m_MatSpeaker.GetRow(3) + m_HeadOfsSpeaker;
		LookDirection = ListenerPos - (TempSpeakerPos + m_HeadOfsListener);
		SpeakerMat = m_MatListener;
		HeadOffsetSpeaker = m_HeadOfsListener;
	}
	else
	{
		TempSpeakerPos = m_MatSpeaker.GetRow(3);
		ListenerPos = m_MatListener.GetRow(3) + m_HeadOfsListener;
		LookDirection = ListenerPos - (TempSpeakerPos + m_HeadOfsSpeaker);
		SpeakerMat = m_MatSpeaker;
		HeadOffsetSpeaker = m_HeadOfsSpeaker;
	}

	// Force direction to speaker
	SpeakerMat.Unit3x3();
	LookDirection.SetMatrixRow(SpeakerMat, 0);
	SpeakerMat.RecreateMatrix(0, 2);

	SpeakerMat.GetRow(3) += HeadOffsetSpeaker;
	SpeakerMat.GetRow(3).k[2] -= 4.0f;
	SpeakerMat.GetRow(3) = Offset * SpeakerMat;

	// Set new translation part
	_Mat.Unit();
	_Mat.GetRow(3) = SpeakerMat.GetRow(3);

	// Set direction and recreate matrix
	CVec3Dfp32 Direction = ListenerPos - _Mat.GetRow(3);
	Direction.SetMatrixRow(_Mat, 0);
	_Mat.RecreateMatrix(0, 2);
	*/
}

void CWO_DialogueCamera::GetCameraPosition_LookAtListener(CMat4Dfp32& _Camera)
{

}

void CWO_DialogueCamera::GetCameraPosition_DefaultTwoShot(CMat4Dfp32& _Mat, CDialogueToken* _pDialogueToken)
{

	CVec3Dfp32 Offset(-10.0f, -10.0f, 0.0f);
	CVec3Dfp32 CameraVal(-10.0f, -8.0f, -4.0f);
	fp32 AddRightOffsetter = 10.0f;

	// Check mode(speaker/listener) flip
	if(m_Mode_Flipped || m_bForceRightShoulder)
	{
		Offset.k[1] = -Offset.k[1];
		CameraVal.k[1] = -CameraVal.k[1];
	}

	// Add tweak values(if any..)
	Offset += CameraVal;

	CVec3Dfp32 ListenerPos, LookDirection;
	ListenerPos = m_MatListener.GetRow(3) +  m_OrgListenerHeadOffset + _pDialogueToken->m_CameraListenerOffset;
	LookDirection = ListenerPos - (m_MatSpeaker.GetRow(3) + m_OrgSpeakerHeadOffset);

	CMat4Dfp32 SpeakerMat = m_MatSpeaker;
	// Force direction to speaker
	SpeakerMat.Unit3x3();
	LookDirection.SetMatrixRow(SpeakerMat, 0);
	SpeakerMat.RecreateMatrix(0, 2);

	SpeakerMat.GetRow(3) += m_OrgSpeakerHeadOffset;
	SpeakerMat.GetRow(3).k[2] -= 4.0f;
	SpeakerMat.GetRow(3) = Offset * SpeakerMat + _pDialogueToken->m_CameraSpeakerOffset;

	// Set new translation part
	_Mat.Unit();
	_Mat.GetRow(3) = SpeakerMat.GetRow(3);

	// Set direction and recreate matrix
	CVec3Dfp32 LookAtPoint;
	CVec3Dfp32 ModifyPoint(0.0f, 0.0f, 6.0f);
	ModifyPoint.k[1] = (m_Mode_Flipped || m_bForceRightShoulder) ? AddRightOffsetter : -AddRightOffsetter;
	CMat4Dfp32 TempMat(SpeakerMat);	
	LookAtPoint = ModifyPoint * TempMat;

	CVec3Dfp32 Direction = ListenerPos - LookAtPoint;
	//Direction = m_Mode_Flipped ? -Direction : Direction;
	Direction.SetMatrixRow(_Mat, 0);
	_Mat.RecreateMatrix(0, 2);	
}

void CWO_DialogueCamera::GetCameraPosition_Common(CMat4Dfp32& _Mat, CDialogueToken* _pDialogueToken, CVec3Dfp32 *_pOffset, CVec3Dfp32 *_pCameraVal, fp32 _AddrightOffsetter)
{
	// Check mode(speaker/listener) flip
	if(m_Mode_Flipped || m_bForceRightShoulder)
	{
		_pOffset->k[1] = -_pOffset->k[1];
		_pCameraVal->k[1] = -_pCameraVal->k[1];
	}

	CMat4Dfp32 TempMat1;
	TempMat1 = m_MatListener;
	TempMat1.GetRow(3) = 0;
	CMat4Dfp32 TempMat2;
	TempMat2 = m_MatSpeaker;
	TempMat2.GetRow(3) = 0;

	// Add tweak values(if any..)
	*_pOffset += *_pCameraVal;

	CVec3Dfp32 ListenerPos, LookDirection;
	ListenerPos = m_MatListener.GetRow(3) +  m_OrgListenerHeadOffset + (_pDialogueToken->m_CameraListenerOffset * TempMat1);
	LookDirection = ListenerPos - (m_MatSpeaker.GetRow(3) + m_OrgSpeakerHeadOffset);

	CMat4Dfp32 SpeakerMat = m_MatSpeaker;
	// Force direction to speaker
	SpeakerMat.Unit3x3();
	SpeakerMat.GetRow(0) = LookDirection;
	SpeakerMat.RecreateMatrix(0, 2);

	SpeakerMat.GetRow(3) += m_OrgSpeakerHeadOffset;
	SpeakerMat.GetRow(3).k[2] -= 4.0f;
	SpeakerMat.GetRow(3) += *_pOffset * TempMat2 + (_pDialogueToken->m_CameraSpeakerOffset * TempMat1); // hepp?!

	// Set new translation part
	_Mat.Unit();
	_Mat.GetRow(3) = SpeakerMat.GetRow(3);

	// Set direction and recreate matrix
	CVec3Dfp32 LookAtPoint;
	CVec3Dfp32 ModifyPoint(0.0f, 0.0f, 6.0f);
	ModifyPoint.k[1] = (m_Mode_Flipped || m_bForceRightShoulder) ? _AddrightOffsetter : -_AddrightOffsetter;
	CMat4Dfp32 TempMat(SpeakerMat);	
	LookAtPoint = ModifyPoint * TempMat;

	CVec3Dfp32 Direction = ListenerPos - LookAtPoint;
	//Direction = m_Mode_Flipped ? -Direction : Direction;
	Direction.SetMatrixRow(_Mat, 0);
	_Mat.RecreateMatrix(0, 2);
}
	

void CWO_DialogueCamera::GetCameraPosition_OverShoulder(CVec3Dfp32& _Camera)
{
	fp32 x = -6.0f;
	if (m_Mode_Flipped == true)
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

	_Camera = CVec3Dfp32::GetMatrixRow(m_MatListener, 3) + Offset + m_HeadOfsListener;	
}

void CWO_DialogueCamera::GetCameraPosition_NearOverShoulder(CVec3Dfp32& _Camera)
{
	float x = -3.0f;
	if (m_Mode_Flipped == true)
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
	_Camera = CVec3Dfp32::GetMatrixRow(m_MatListener, 3) + Offset + m_HeadOfsListener;
}


void CWO_DialogueCamera::GetCameraPosition_DefaultCamera(CVec3Dfp32& _Camera)
{
	fp32 Dist = (m_PosSpeakerLookat - m_PosListenerLookat).Length() * (1.0f / m_FovCoeff) * 2.0f;	// FOV Dependant

	Dist *= 0.35f;

	static fp32 sx=0.4f,sy=0.25f,sz=0.0f;
	CVec3Dfp32 Rot(M_Fabs(Dist) * sx, Dist * sy, M_Fabs(Dist) * sz);

	CVec3Dfp32 Mid;
	GetLookatPosition_Interpolated(Mid, 0.5f);

	CMat4Dfp32 Mat;
	Mat = m_Mode_Flipped ? m_MatSpeaker : m_MatListener;

	Rot *= Mat;
	Rot -= CVec3Dfp32::GetMatrixRow(Mat, 3);
	_Camera = Mid + Rot;
}


void CWO_DialogueCamera::GetCameraPosition_TwoShot(CVec3Dfp32& _Camera, int iCameraModeParameter)
{
	fp32 Dist = (m_PosSpeakerLookat - m_PosListenerLookat).Length() * (1.0f / m_FovCoeff) * 2.0f;	// FOV Dependant

	// Automatic/Non-automatic camera stuff
	CMat4Dfp32 Mat = m_MatListener;
	if(iCameraModeParameter == 1)
	{
		// Look at speaker
		Mat = m_Mode_Flipped ? m_MatSpeaker : m_MatListener;
	}
	else if(iCameraModeParameter == 2)
	{
		// Look at listener
		Mat = m_Mode_Flipped ? m_MatListener : m_MatSpeaker;
		Dist = -Dist;
	}
	else if(iCameraModeParameter == 0)// We have an automatic camera, handle flip
	{	
		if (m_Mode_Flipped == true)
			Dist = -Dist;
	}

	Dist *= 0.35f;

	static fp32 sx=0.4f,sy=0.25f,sz=0.0f;
	CVec3Dfp32 Rot(M_Fabs(Dist) * sx, Dist * sy, M_Fabs(Dist) * sz);

	CVec3Dfp32 Mid;
	GetLookatPosition_Interpolated(Mid, 0.5f);

	Rot *= Mat;
	Rot -= CVec3Dfp32::GetMatrixRow(Mat, 3);
	_Camera = Mid + Rot;
}


void CWO_DialogueCamera::GetCameraPosition_SpeakerHead(CVec3Dfp32& _Camera)
{
	CVec3Dfp32 Offset(-35.0f, 0.0f, 5.0f);

	CVec3Dfp32 SpeakerListenerRelation = CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3) - CVec3Dfp32::GetMatrixRow(m_MatListener, 3);
	fp32 Angle = CVec3Dfp32::AngleFromVector(SpeakerListenerRelation.k[0], SpeakerListenerRelation.k[1]);
	CMat4Dfp32 Rotation;
	Rotation.Unit();
	Rotation.RotZ_x_M(Angle);
	Offset *= Rotation;
	_Camera = CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3) + Offset + m_HeadOfsSpeaker;
}

void CWO_DialogueCamera::GetCameraPosition_Lookat(CVec3Dfp32& _Camera)
{
	GetLookatPosition_Lookat(_Camera);
	_Camera += m_pDialogueToken->m_CameraListenerOffset;
}

// -- Lookat positions --------------------------------------------------------------------------
void CWO_DialogueCamera::GetLookatPosition(CVec3Dfp32& _Lookat)
{
	switch(m_Mode_Lookat)
	{

		// Just supporting look at speaker and interpolated for now..
	case DIALOGUECAMERA_LOOKAT_SPEAKER:
		GetLookatPosition_Speaker(_Lookat);
		break;

	case DIALOGUECAMERA_LOOKAT_INTERPOLATED:
		GetLookatPosition_Interpolated(_Lookat, 0.5f);
		break;
		// Add camera speaker offset(if any..)
		_Lookat += m_pDialogueToken->m_CameraSpeakerOffset;
	}


	/*switch(m_Mode_Lookat)
	{
	case DIALOGUECAMERA_LOOKAT_ENGINEPATH:
	GetLookatPosition_EnginePath(_Lookat);
	break;

	case DIALOGUECAMERA_LOOKAT_SPEAKEROFFSET:
	GetLookatPosition_SpeakerOffset(_Lookat);
	break;

	case DIALOGUECAMERA_LOOKAT_SPEAKER:
	GetLookatPosition_Speaker(_Lookat);
	break;

	case DIALOGUECAMERA_LOOKAT_INTERPOLATED:
	GetLookatPosition_Interpolated(_Lookat, m_pDialogueToken->m_CameraModeParameter);
	break;

	case DIALOGUECAMERA_LOOKAT_LOOKAT:
	GetLookatPosition_Lookat(_Lookat);
	break;

	case DIALOGUECAMERA_LOOKAT_TWOSHOT:
	GetLookatPosition_Interpolated(_Lookat, 0.5f);
	break;

	default:
	_Lookat.k[0] = _FP32_MAX;
	_Lookat.k[1] = _FP32_MAX;
	_Lookat.k[2] = _FP32_MAX;
	break;
	}

	_Lookat += m_pDialogueToken->m_CameraSpeakerOffset;*/
}

void CWO_DialogueCamera::GetLookatPosition_EnginePath(CVec3Dfp32& _Lookat)
{
	GetEnginePathPosition(_Lookat, m_Time);
}

void CWO_DialogueCamera::GetLookatPosition_SpeakerOffset(CVec3Dfp32& _Lookat)
{
	GetLookatPosition_Speaker(_Lookat);
	_Lookat += m_pDialogueToken->m_CameraSpeakerOffset;
}

void CWO_DialogueCamera::GetLookatPosition_Speaker(CVec3Dfp32& _Lookat)
{
	GetLookatPosition_Interpolated(_Lookat, 0.0f);
}

void CWO_DialogueCamera::GetLookatPosition_Interpolated(CVec3Dfp32& _Lookat, float _i)
{
	_Lookat = (m_PosListenerLookat * _i) + (m_PosSpeakerLookat * (1.0f - _i));
	_Lookat.k[2] -= 10.0f * m_FovCoeff;			// FOV dependent
}

void CWO_DialogueCamera::GetLookatPosition_Lookat(CVec3Dfp32& _Lookat)
{
	_Lookat = m_PosSpeakerLookat + m_pDialogueToken->m_CameraSpeakerOffset;
}


// -- FOV -----------------------------------------------------------------------------------------
fp32 CWO_DialogueCamera::GetFOVAt(CWorld_Client* _pWClient, CDialogueToken* _pDialogueToken, const CMTime& _Time)
{
	if (!_pDialogueToken->IsValid())
	{
		ConOutL(CStr("ERROR: Invalid dialoguetoken"));
		return 35.0f;
	};

	m_pWClient = _pWClient;
	m_Time = _Time;
	m_pDialogueToken = _pDialogueToken;

	return GetFOVAt(m_Time);
}

fp32 CWO_DialogueCamera::GetFOVAt(const CMTime& _Time)
{
	return m_pDialogueToken->m_FOV;
        //return 60;
	//return m_pDialogueToken->m_FOV + (m_pDialogueToken->m_dFOV * _Time.GetTime()); /*CMTIMEFIX*/
}


// -- Public -------------------------------------------------------------------------------------

int CWO_DialogueCamera::GetCameraAt(CWorld_Client* _pWClient,
									CDialogueToken* _pDialogueToken,
									const CMat4Dfp32* _pMatrices,
									const fp32* _pHeadOffsets,
									const int* _iObjectIndices,
									CMat4Dfp32& _Matrix,
									const CMTime& _Time,
									CMat4Dfp32& _MouthPos,
									bool _bSwappedSpeaker,
									bool _bFirstFrameIn3PI,
									bool _bTelephone)

{
	if (!_pDialogueToken)
	{
		ConOutL(CStr("ERROR - CWO_DialogueCamera::GetCameraAt: No dialoguetoken provided"));
		return 0;
	}

	bool bGetRandomCam = ((_pDialogueToken->m_CameraMode == 0 && 
		_pDialogueToken->m_CameraListenerOffset.LengthSqr() == 0.0f && 
		_pDialogueToken->m_CameraSpeakerOffset.LengthSqr() == 0.0f) || 
		(_pDialogueToken->m_CameraMode == DIALOGUECAMERA_MODE_SCRIPTED && _pDialogueToken->m_Camera_Scripted == "" )) ? 
		true : false;	

	if(bGetRandomCam)
	{
		int8 LastCameraSetup = m_iCurrentCameraSetup;
		int TemplateSize = CAMERA_TEMPLATE_TWOSHOT_SIZE;
		if(_bTelephone)
			TemplateSize = CAMERA_TEMPLATE_TELEPHONE_SIZE;
		m_iCurrentCameraSetup = int(Random * TemplateSize);
		if(_bTelephone)
		{
			m_iCurrentCameraSetup += CAMERA_TEMPLATE_TWOSHOT_SIZE;
			if(m_iCurrentCameraSetup == LastCameraSetup)
			{
				m_iCurrentCameraSetup++;
				if(m_iCurrentCameraSetup == CAMERA_TEMPLATE_SIZE)
					m_iCurrentCameraSetup = CAMERA_TEMPLATE_TWOSHOT_SIZE;
			}
		}

		// check if it makes sense to use the new camera
		int8 SafetyCounter = 0;
		while(1 && !_bTelephone)
		{
			CVec3Dfp32 OldToNewList = (m_CameraSetups[m_iCurrentCameraSetup].m_CameraListenerOffset - m_CameraSetups[LastCameraSetup].m_CameraListenerOffset);
			if(OldToNewList.LengthSqr() > 48.0f) // 
				break;
			else
			{
				//M_TRACEALWAYS("New Camera Template %d and the previous, %d, are too close to eachother\n", m_iCurrentCameraSetup, LastCameraSetup);
				m_iCurrentCameraSetup++;
				if(m_iCurrentCameraSetup == TemplateSize)
					m_iCurrentCameraSetup = 0;

				SafetyCounter++;
				if(SafetyCounter == TemplateSize)
				{
					M_TRACEALWAYS("Could not find a camera template that was far enough away from previous template!\n");
					break;
				}
			}
		}
		
		M_TRACEALWAYS("Setting random template - %d\n", m_iCurrentCameraSetup);

		int8 StoreParam = _pDialogueToken->m_CameraModeParameter;
		fp32 Shakeval = _pDialogueToken->m_CameraShakeVal;
		fp32 ShakeSpeed = _pDialogueToken->m_CameraShakeSpeed;
		*_pDialogueToken = m_CameraSetups[m_iCurrentCameraSetup];
		if(StoreParam)
			_pDialogueToken->m_CameraModeParameter = StoreParam;
		
		_pDialogueToken->m_CameraShakeSpeed = ShakeSpeed;
		_pDialogueToken->m_CameraShakeVal = Shakeval;

		//m_bForceRightShoulder = false;
		//m_bForceRightShoulder = (Random > 0.85f) ? true : false;
	}
	else if(_pDialogueToken->m_CameraMode == 2)
	{
		//SetMode(_pDialogueToken->m_CameraMode);
		_pDialogueToken->m_CameraListenerOffset = 0.0f; 
		_pDialogueToken->m_CameraSpeakerOffset = 0.0f;
		_pDialogueToken->m_FOV = 35.0f;
	}

	SetMode(_pDialogueToken->m_CameraMode);

	CMat4Dfp32 CurrentOrgMat;
	static bool bFirstFrameOfSwitch = false;

	// Save original camera matrix when we entered third person dialogue(if first frame)
	if(_bFirstFrameIn3PI)
		m_OrgCameraMat = _Matrix;

	CurrentOrgMat = _Matrix;
	_Matrix.Unit();

	static uint8 iLastCameraModeParam = 0;

	// Setup temporary variables
	m_Time = _Time;
	m_pWClient = _pWClient;

	// Configure camera
	m_pDialogueToken = _pDialogueToken;

	// Get listener/speaker IDs and matrices
	int iListenerId =_iObjectIndices[0];
	int iSpeakerId =_iObjectIndices[1];

	CMat4Dfp32 MatListener, MatSpeaker;
	MatListener = _pMatrices[0];
	MatSpeaker = _pMatrices[1];

	// Get head offsets for dialogue objects
	fp32 ListenerHeadOffset, SpeakerHeadOffset;
	ListenerHeadOffset = _pHeadOffsets[0];
	SpeakerHeadOffset = _pHeadOffsets[1];

	// Save original positions so they are not affected by animation
	static int curspeak = -1;
	if (_iObjectIndices[1] != curspeak || _bFirstFrameIn3PI)
	{
		
		//M_TRACEALWAYS("--- * Changing speaker to %d\n", iSpeakerId);
		m_OrgMatSpeaker = MatSpeaker;
		m_OrgMatListener = MatListener;
		m_OrgHeadOffsetSpeaker = SpeakerHeadOffset;
		m_OrgHeadOffsetListener = ListenerHeadOffset;
		//SetMode(m_pDialogueToken->m_CameraMode);
	}
	
	if(m_pDialogueToken->m_iLastSpeaker == iSpeakerId)
	{
		m_OrgMatSpeaker = MatListener;
		m_OrgMatListener = MatSpeaker;
		m_OrgHeadOffsetSpeaker = ListenerHeadOffset;
		m_OrgHeadOffsetListener = SpeakerHeadOffset;
	}
	curspeak = _iObjectIndices[1];

	// Set m_Mode_Flipped based on camera params
	if(m_pDialogueToken->m_CameraModeParameter == 0)
	{
		// We have an automatic camera
		bool bSwitchState = (m_pDialogueToken->m_iLastSpeaker == iSpeakerId);
		bFirstFrameOfSwitch = ((bSwitchState && !m_Mode_Flipped) || (!bSwitchState && m_Mode_Flipped));
		m_Mode_Flipped = bSwitchState;
	}
	else
	{
		bFirstFrameOfSwitch = (iLastCameraModeParam != _pDialogueToken->m_CameraModeParameter);
		m_Mode_Flipped = (m_pDialogueToken->m_CameraModeParameter == 2);
	}

	// Save last param
	iLastCameraModeParam = _pDialogueToken->m_CameraModeParameter;

	if(m_Mode_Flipped)
	{
		m_iSafeBB = iListenerId;
		m_MatSpeaker = m_OrgMatSpeaker;
		m_MatListener = m_OrgMatListener;
		m_HeadOfsSpeaker = CVec3Dfp32(0, 0, m_OrgHeadOffsetSpeaker);
		m_HeadOfsListener = CVec3Dfp32(0, 0, m_OrgHeadOffsetListener);
		m_PosSpeakerLookat = CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3) + m_HeadOfsSpeaker;
		m_PosListenerLookat = CVec3Dfp32::GetMatrixRow(m_MatListener, 3) + m_HeadOfsListener;
	}
	else
	{
		m_iSafeBB = iSpeakerId;
		if(!_bTelephone)
		{
			m_MatSpeaker = m_OrgMatListener;
			m_MatListener = m_OrgMatSpeaker;
		}
		else
		{
			m_MatSpeaker = m_OrgMatSpeaker;
			m_MatListener = m_OrgMatListener;
		}
		m_HeadOfsSpeaker = CVec3Dfp32(0, 0, m_OrgHeadOffsetListener);
		m_HeadOfsListener = CVec3Dfp32(0, 0, m_OrgHeadOffsetSpeaker);
		m_PosSpeakerLookat = CVec3Dfp32::GetMatrixRow(m_MatSpeaker, 3) + m_HeadOfsSpeaker;
		m_PosListenerLookat = CVec3Dfp32::GetMatrixRow(m_MatListener, 3) + m_HeadOfsListener;
	}

	if(_bFirstFrameIn3PI || bFirstFrameOfSwitch)
	{
		if(_bTelephone && m_pDialogueToken->m_iLastSpeaker == iSpeakerId)
		{	//switcharo! get new camera
			m_TickToNewCamera = _pWClient->GetGameTick() + TruncToInt((1.0f + Random * 2.0f) * _pWClient->GetGameTicksPerSecond());		
		}
		m_iCurCamShakeSeq = 3 + int32(Random*3);
		m_iCurCamShakeSeq = Min(m_iCurCamShakeSeq, 5);
		
		m_OrgSpeakerHeadOffset = m_HeadOfsSpeaker;
		m_OrgListenerHeadOffset = m_HeadOfsListener;
		m_CamTelephoneTimer = 0.0f;
		bFirstFrameOfSwitch = false;
	}

	if(m_TickToNewCamera && _pWClient->GetGameTick() >= m_TickToNewCamera)
	{
		m_TickToNewCamera = 0;
		int8 LastCameraSetup = m_iCurrentCameraSetup;
		m_iCurrentCameraSetup = int(Random * CAMERA_TEMPLATE_TELEPHONE_SIZE);
		m_iCurrentCameraSetup += CAMERA_TEMPLATE_TWOSHOT_SIZE;
		if(m_iCurrentCameraSetup == LastCameraSetup)
		{
			m_iCurrentCameraSetup++;
			if(m_iCurrentCameraSetup == CAMERA_TEMPLATE_SIZE)
				m_iCurrentCameraSetup = CAMERA_TEMPLATE_TWOSHOT_SIZE;
		}
		int8 StoreParam = _pDialogueToken->m_CameraModeParameter;
		fp32 Shakeval = _pDialogueToken->m_CameraShakeVal;
		fp32 ShakeSpeed = _pDialogueToken->m_CameraShakeSpeed;
		*_pDialogueToken = m_CameraSetups[m_iCurrentCameraSetup];
		if(StoreParam)
			_pDialogueToken->m_CameraModeParameter = StoreParam;

		_pDialogueToken->m_CameraShakeSpeed = ShakeSpeed;
		_pDialogueToken->m_CameraShakeVal = Shakeval;
	}

	m_Fov = GetFOVAt(m_Time);
	if (m_Fov < 10.0f) m_Fov = 10.0f;
	if (m_Fov > 90.0f) m_Fov = 90.0f;
	m_FovCoeff = 1.0f - ((m_Fov-10.0f) / 80.0f);

	CVec3Dfp32 CameraPos;
	CVec3Dfp32 LookatPos;

	GetLookatPosition(LookatPos);

	CVec3Dfp32 Offset;
	CVec3Dfp32 CameraVal;
	fp32 AddRightOffsetter;

	// Call appropriate function
	switch(m_Mode_Camera)
	{

	case DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT:
		if(!_bTelephone)
		{
			Offset = CVec3Dfp32(-10.0f, -10.0f, 0.0f);
			CameraVal = CVec3Dfp32(-10.0f, -8.0f, -4.0f);
			AddRightOffsetter = 10.0f;

			GetCameraPosition_Common(_Matrix, _pDialogueToken, &Offset, &CameraVal, AddRightOffsetter);
			//GetCameraPosition_DefaultTwoShot(_Matrix, _pDialogueToken);
		}
		else
		{	//Telephone
			_Matrix.GetRow(3) = _MouthPos.GetRow(3) + _MouthPos.GetRow(0) * _pDialogueToken->m_CameraSpeakerOffset.k[0] 
					+ _MouthPos.GetRow(1) * _pDialogueToken->m_CameraSpeakerOffset.k[1]
					+ _MouthPos.GetRow(2) * _pDialogueToken->m_CameraSpeakerOffset.k[2];
			LookAt(_Matrix, _MouthPos.GetRow(3));
		}
		break;

	case DIALOGUECAMERA_MODE_THIRDPERSON_NOCLIP:
		GetCameraPosition_DefaultCamera(CameraPos);
		_Matrix.GetRow(3) = CameraPos;
		LookAt(_Matrix, LookatPos);
		break;

	case DIALOGUECAMERA_MODE_BOUNDING_BOX:
		Offset = CVec3Dfp32(-7.0f, 7.0f, 0.0f);
		CameraVal = CVec3Dfp32(0.0f, 0.0f, 0.0f);
		AddRightOffsetter = 0.0f;

		GetCameraPosition_Common(_Matrix, _pDialogueToken, &Offset, &CameraVal, AddRightOffsetter);
		//GetCameraPosition_SafeBoundingBox(CameraPos, LookatPos);
		//_Matrix.GetRow(3) = CameraPos;
		LookAt(_Matrix, LookatPos);
		break;

	case DIALOGUECAMERA_MODE_SCRIPTED:
		GetEnginePathMatrix(_Matrix, m_Time);
		break;

	case DIALOGUECAMERA_MODE_LOOKAT_SPEAKER:
		//Offset = CVec3Dfp32(5.0f, -15.0f, 0.0f);
		//CameraVal = CVec3Dfp32(0.0f, 0.0f, 0.0f);
		//AddRightOffsetter = 5.0f;

		GetCameraPosition_LookAtSpeaker(_Matrix);
		//GetCameraPosition_Common(_Matrix, _pDialogueToken, &Offset, &CameraVal, AddRightOffsetter);
		//LookAt(_Matrix, LookatPos);
		break;

	case DIALOGUECAMERA_MODE_LOOKAT_LISTENER:
		//GetCameraPosition_DefaultCamera(CameraPos);
		//_Matrix.GetRow(3) = CameraPos;
		//LookAt(_Matrix, LookatPos);
        
		GetCameraPosition_LookAtSpeaker(_Matrix);
		break;

	case DIALOGUECAMERA_MODE_SCRIPTED_TELEPHONE:
		_Matrix = _MouthPos;
		GetCameraPosition_TelephoneScripted(_Matrix, _pDialogueToken);
		break;

	case DIALOGUECAMERA_MODE_PREVIOUS_CAMERA:
		// Should we have this one?!?
		break;
	case DIALOGUECAMERA_MODE_TEST_SELFROTATE:

		break;
	}

	if(!m_Mode_Flipped)
	{
		CMat4Dfp32 ConvertOrgMat, ConvertMat, Result, RotationDiff;

		ConvertOrgMat.Unit();
		ConvertOrgMat.GetRow(0) = m_OrgCameraMat.GetRow(2);
		ConvertOrgMat.GetRow(1) = -m_OrgCameraMat.GetRow(0);
		ConvertOrgMat.GetRow(2) = -m_OrgCameraMat.GetRow(1);
		ConvertOrgMat.GetRow(3) = m_OrgCameraMat.GetRow(3);

		ConvertMat.Unit();
		ConvertMat.GetRow(0) = CurrentOrgMat.GetRow(2);
		ConvertMat.GetRow(1) = -CurrentOrgMat.GetRow(0);
		ConvertMat.GetRow(2) = -CurrentOrgMat.GetRow(1);
		ConvertMat.GetRow(3) = CurrentOrgMat.GetRow(3);

		CMat4Dfp32 InvMatrix, DeltaRotation, InvConvMat;
		InvMatrix.Unit();
		Result.Unit();
		DeltaRotation.Unit();
		InvConvMat.Unit();

		ConvertOrgMat.InverseOrthogonal(InvConvMat);
		ConvertMat.Multiply3x3(InvConvMat, DeltaRotation);

		DeltaRotation.Multiply3x3(_Matrix, Result);
		CVec3Dfp32(0,0,1).SetRow(Result, 2);
		Result.RecreateMatrix(0, 2);

		_Matrix.GetRow(0) = Result.GetRow(0);
		_Matrix.GetRow(1) = Result.GetRow(1);
		_Matrix.GetRow(2) = Result.GetRow(2);
	}

	_Matrix.RotX_x_M(-0.25f);
	_Matrix.RotY_x_M(0.25f);

	// Check collision between camera and "look at point"
	// If we are talking to a phone we want to fall back on a closeup on jackies face
	if(_pWClient && (m_Mode_Camera != DIALOGUECAMERA_MODE_SCRIPTED || _bTelephone))
	{
		CCollisionInfo CInfo;
		CInfo.m_CollisionType = 0;
		int32 ObjectFlags = 0;
		int32 ObjectIntersectionFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
		int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;		
		//if(_pWClient->Phys_IntersectLine(LookatPos,CameraPos, ObjectFlags, ObjectIntersectionFlags, MediumFlags, &CInfo))
		CVec3Dfp32 FocusCharHeadpos;
		if(_pDialogueToken->m_CameraModeParameter > 1)
			FocusCharHeadpos = (m_MatListener.GetRow(3) + m_OrgListenerHeadOffset);
		else
			FocusCharHeadpos = (m_MatSpeaker.GetRow(3) + m_OrgSpeakerHeadOffset);

		if(_pWClient->Phys_IntersectLine(FocusCharHeadpos, _Matrix.GetRow(3), ObjectFlags, ObjectIntersectionFlags, MediumFlags, &CInfo))
		{
			//m_bForceRightShoulder = m_bForceRightShoulder ? false : true;
			return 2;
		}
	}

	if(_pDialogueToken->m_CameraShakeVal != 0.0f)
		ApplyCameraShake(_Time, _Matrix, _pDialogueToken->m_CameraShakeVal, _pDialogueToken->m_CameraShakeSpeed);

	return 1;
}


// -- Misc internal methods -----------------------------------------------------------------------

void CWO_DialogueCamera::LookAt(CMat4Dfp32& _Matrix, const CVec3Dfp32& _LookAt)
{
	CVec3Dfp32 MatrixPos = CVec3Dfp32::GetMatrixRow(_Matrix, 3);
	_Matrix.Unit();
	MatrixPos.SetMatrixRow(_Matrix, 3);

	CVec3Dfp32 Dir = _LookAt - MatrixPos;
	Dir.SetMatrixRow(_Matrix, 0);
	_Matrix.RecreateMatrix(0, 2);
}

void CWO_DialogueCamera::GetEnginePathMatrix(CMat4Dfp32& _Mat, const CMTime& _Time)
{
	if(!m_pWClient || m_pDialogueToken->m_Camera_Scripted == "")
		return;

	// Now this is not very nice...
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(!pGame)
		return;

	int iObj = pGame->m_spWServer->Selection_GetSingleTarget(m_pDialogueToken->m_Camera_Scripted);


	if(iObj > 0)
	{
		CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, (aint)(&_Time), (aint)(&_Mat));
		if(m_pWClient->ClientMessage_SendToObject(Msg, iObj))
			return;
		_Mat = m_pWClient->Object_GetPositionMatrix(iObj);
		return;
	}

	ConOut(CStr("Failed to get rendermatrix from scripted camera: " + m_pDialogueToken->m_Camera_Scripted));
}

void CWO_DialogueCamera::GetEnginePathPosition(CVec3Dfp32& _Pos, const CMTime& _Time)
{
	CMat4Dfp32 TempMatrix;
	GetEnginePathMatrix(TempMatrix, _Time);
	_Pos = CVec3Dfp32::GetMatrixRow(TempMatrix, 3);
}




int CWO_DialogueCamera::SetMode(int _Mode)
{
	switch(_Mode)
	{	

	case DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT:
		m_Mode_Camera = DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
		m_Mode_Lookat = DIALOGUECAMERA_LOOKAT_INTERPOLATED;
		break;

	case DIALOGUECAMERA_MODE_THIRDPERSON_NOCLIP:
		m_Mode_Camera = DIALOGUECAMERA_MODE_THIRDPERSON_NOCLIP;
		m_Mode_Lookat = DIALOGUECAMERA_LOOKAT_SPEAKER;
		break;

	case DIALOGUECAMERA_MODE_BOUNDING_BOX:
		m_Mode_Camera = DIALOGUECAMERA_MODE_BOUNDING_BOX;
		m_Mode_Lookat = DIALOGUECAMERA_LOOKAT_INTERPOLATED;
		break;

	case DIALOGUECAMERA_MODE_SCRIPTED:
		m_Mode_Camera = DIALOGUECAMERA_MODE_SCRIPTED;
		m_Mode_Lookat = DIALOGUECAMERA_LOOKAT_SPEAKER;
		break;

	case DIALOGUECAMERA_MODE_LOOKAT_SPEAKER:
		m_Mode_Camera = DIALOGUECAMERA_MODE_LOOKAT_SPEAKER;
		m_Mode_Lookat = DIALOGUECAMERA_LOOKAT_SPEAKER;
		break;

	case DIALOGUECAMERA_MODE_LOOKAT_LISTENER:
		m_Mode_Camera = DIALOGUECAMERA_MODE_LOOKAT_LISTENER;
		//m_Mode_Lookat = DIALOGUECAMERA_LOOKAT_LISTENER;
		break;
	default:
		m_Mode_Camera = _Mode;
	}

	return m_Mode = _Mode;
}

int CWO_DialogueCamera::GetMode()
{
	return m_Mode;
}

bool CWO_DialogueCamera::GetModeFlipFlag()
{
	/*switch(m_Mode)
	{
	case DIALOGUECAMERA_MODE_OVERSHOULDER_LEFT:
	case DIALOGUECAMERA_MODE_TWOSHOT_LEFT:
	case DIALOGUECAMERA_MODE_LOOKAT_LEFT:
	case DIALOGUECAMERA_MODE_TEST_LEFT:
	return true;

	case DIALOGUECAMERA_MODE_OVERSHOULDER_RIGHT:
	case DIALOGUECAMERA_MODE_TWOSHOT_RIGHT:
	case DIALOGUECAMERA_MODE_LOOKAT_RIGHT:
	case DIALOGUECAMERA_MODE_TEST_RIGHT:
	case DIALOGUECAMERA_MODE_CLOSEUP:
	return false;

	case DIALOGUECAMERA_UNDEFINED:
	default:
	return false;
	}*/

	return false;
}

bool CWO_DialogueCamera::IsInView(const CMat4Dfp32& _Camera, const CVec3Dfp32& _p, float _FOV, float _SafetyBorder)
{
	CVec3Dfp32 CameraRot = CVec3Dfp32::GetMatrixRow(_Camera, 0);
	fp32 CamAngle = 0.75f - CVec3Dfp32::AngleFromVector(CameraRot.k[0], CameraRot.k[1]);

	CVec3Dfp32 CameraPos = CVec3Dfp32::GetMatrixRow(_Camera, 3);
	CVec3Dfp32 CharCamPos = _p - CameraPos;

	fp32 CharCamAngle = 1.0f - CharCamPos.AngleFromVector(CharCamPos.k[0], CharCamPos.k[1]);
	fp32 FovAngle = CamAngle - CharCamAngle;

	if (Abs(FovAngle) < ((_FOV*0.5*_SafetyBorder) / 360.0f))
		return true;		// Point is in view

	return false;
}

void CWO_DialogueCamera::Clear()
{
	m_Mode = 0;
	m_Mode_Camera = 0;
	m_Mode_Lookat = 0;
	m_Mode_Flipped = 0;
	m_Mode_DifferentSpeaker = 0;
	m_Flags = 0;

	m_OrgListenerHeadOffset.k[0] = 0.0f;
	m_OrgListenerHeadOffset.k[1] = 0.0f;
	m_OrgListenerHeadOffset.k[2] = 0.0f;
	m_OrgSpeakerHeadOffset.k[0] = 0.0f;
	m_OrgSpeakerHeadOffset.k[1] = 0.0f;
	m_OrgSpeakerHeadOffset.k[2] = 0.0f;
	m_MatListener.Unit();
	m_MatSpeaker.Unit();
	m_OrgCameraMat.Unit();
}

void CWO_DialogueCamera::operator =(const CWO_DialogueCamera &_Val)
{
	m_Mode = _Val.m_Mode;
	m_Mode_Camera = _Val.m_Mode_Camera;
	m_Mode_Lookat = _Val.m_Mode_Lookat;
	m_Mode_Flipped = _Val.m_Mode_Flipped;
	m_Flags = _Val.m_Flags;

	// Temporary variables
	m_pDialogueToken = _Val.m_pDialogueToken;
	m_Fov = _Val.m_Fov;
	m_FovCoeff = _Val.m_FovCoeff;
	m_pWClient = _Val.m_pWClient;
	m_Time = _Val.m_Time;
	m_Matrix = _Val.m_Matrix;
	m_PosSpeakerLookat = _Val.m_PosSpeakerLookat;
	m_PosListenerLookat = _Val.m_PosListenerLookat;
	m_MatSpeaker = _Val.m_MatSpeaker;
	m_MatListener = _Val.m_MatListener;
	m_HeadOfsSpeaker = _Val.m_HeadOfsSpeaker;
	m_HeadOfsListener = _Val.m_HeadOfsListener;
	m_bFirstFrameIn3PI = _Val.m_bFirstFrameIn3PI;
}

void CWO_DialogueCamera::GetCameraPosition_TelephoneScripted(CMat4Dfp32& _Mat, CDialogueToken* m_pDialogueToken)
{
	CMTime Time = CMTime::GetCPU();
	CMTime Delta = Time - m_LastTime;
	m_LastTime = Time;

	CXR_Anim_SequenceData* pSeq = m_pCamTelephoneAnim ? m_pCamTelephoneAnim->GetSequence(m_pDialogueToken->m_CameraTelephoneScript) : NULL;
	if(!pSeq)
		return;

	fp32 AnimDuration =  pSeq->GetDuration();
	m_CamTelephoneTimer += Delta.GetTime();
	if(m_CamTelephoneTimer > AnimDuration)
		m_CamTelephoneTimer = (m_CamTelephoneTimer - AnimDuration);

	vec128 Pos;
	CQuatfp32 Rot;
	pSeq->EvalTrack0(CMTime::CreateFromSeconds(m_CamTelephoneTimer), Pos, Rot);

	Rot.Inverse();
	CMat4Dfp32 CamTelephoneMatrix;
	Rot.CreateMatrix3x3(CamTelephoneMatrix);

	CMat4Dfp32 CharMat = _Mat;
	CamTelephoneMatrix.Multiply3x3(CharMat, _Mat);
	CamTelephoneMatrix.UnitNot3x3();
	_Mat.r[3] = M_VSetW1(M_VAdd(CharMat.r[3], M_VMulMat(Pos, CamTelephoneMatrix))); //_Mat.GetRow(3) = CharMat.GetRow(3) + Pos * CamTelephoneMatrix;
}

void CWO_DialogueCamera::ApplyCameraShake(const CMTime &_Time, CMat4Dfp32 &_CameraMat, fp32 _CameraShakeVal, fp32 _CameraShakeSpeed)
{
	CMTime Time = CMTime::GetCPU();
	CMTime Delta = Time-m_LastTime;
	if(Delta.Compare(0.1f) > 0)
		Delta = CMTime::CreateFromSeconds(0.1f);
	m_LastTime= Time;
	fp32 FrameTime = Delta.GetTime() * _CameraShakeSpeed;

	CXR_Anim_SequenceData* pSeq = m_pCamShakeAnimBase ? m_pCamShakeAnimBase->GetSequence(m_iCurCamShakeSeq) : NULL;

	if(!pSeq)
		return;

	fp32 AnimDuration =  pSeq->GetDuration();

	m_CamShakeAnimTimer += FrameTime;
	if(m_CamShakeAnimTimer > AnimDuration)
		m_CamShakeAnimTimer = (m_CamShakeAnimTimer - AnimDuration);
	
	vec128 Pos;
	CQuatfp32 Rot;
	pSeq->EvalTrack0(CMTime::CreateFromSeconds(m_CamShakeAnimTimer), Pos, Rot);
	
	Rot.Inverse();
	CMat4Dfp32 CamShakeMatrix;
	Rot.CreateMatrix3x3(CamShakeMatrix);

	// Now k[0][0] is 1 (aprox)
	CamShakeMatrix.RotY_x_M(-0.25f);
	CamShakeMatrix.RotX_x_M(-0.25f);

	CMat4Dfp32 ResultRotLocal;
	CamShakeMatrix.Multiply3x3(_CameraMat, ResultRotLocal);
	
	fp32 Rest = 1.0f - _CameraShakeVal;
	_CameraMat.GetRow(0) = ((_CameraMat.GetRow(0) * Rest) + (ResultRotLocal.GetRow(0) * _CameraShakeVal)).Normalize();
	//_CameraMat.GetRow(1) = ((_CameraMat.GetRow(1) * Rest) + (ResultRotLocal.GetRow(1) * _CameraShakeVal)).Normalize();
	_CameraMat.GetRow(2) = ((_CameraMat.GetRow(1) * Rest) + (ResultRotLocal.GetRow(1) * _CameraShakeVal)).Normalize();

	_CameraMat.RecreateMatrix(1,0);
	_CameraMat.r[3] = M_VSetW1(M_VAdd(_CameraMat.r[3], M_VMul(Pos, M_VLdScalar(_CameraShakeVal))));
}

void  CWO_DialogueCamera::SetupCameraTemplates(CMapData* _pMapData)
{
	m_CamShakeAnimTimer = 0.0f;
	m_CamTelephoneTimer = 0.0f;
	m_iCurCamShakeSeq = 3;

	int iResAnim  = _pMapData->GetResourceIndex_Anim("ITEMS\\Dialog_Camera");
	m_pCamShakeAnimBase = iResAnim ? _pMapData->GetResource_Anim(iResAnim) : NULL;

	//TODO
	//Change to real animation
	iResAnim = _pMapData->GetResourceIndex_Anim("ITEMS\\Dialog_Camera");
	m_pCamTelephoneAnim = iResAnim ? _pMapData->GetResource_Anim(iResAnim) : NULL;
	
	m_iCurrentCameraSetup = 0;

	// Hardcoded templates for now
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_0].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_0].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_0].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_0].m_FOV		= 35.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_0].m_CameraSpeakerOffset  = CVec3Dfp32(0.0f, 0.0f, 1.0f);
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_0].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, 1.0f);

	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_1].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_1].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_1].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_1].m_FOV		= 35.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_1].m_CameraSpeakerOffset  = CVec3Dfp32(0.0f, 0.0f,  6.0f);
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_1].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, -6.0f);

	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_2].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_2].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_2].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_2].m_FOV		= 35.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_2].m_CameraSpeakerOffset  = CVec3Dfp32(0.0f, 0.0f,  3.0f);
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_2].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, -3.0f);

	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_3].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_3].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_3].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_3].m_FOV		= 50.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_3].m_CameraSpeakerOffset  = CVec3Dfp32(0.0f, 0.0f, 1.0f);
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_3].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, 1.0f);

	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_4].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_4].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_4].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_4].m_FOV		= 45.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_4].m_CameraSpeakerOffset  = CVec3Dfp32(0.0f,   0.0f, 10.0f);
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_4].m_CameraListenerOffset = CVec3Dfp32(0.0f,   0.0f, -10.0f);

	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_5].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_5].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_5].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_5].m_FOV		= 35.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_5].m_CameraSpeakerOffset  = CVec3Dfp32(0.0f, 0.0f, -3.0f);
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_5].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f,  3.0f);

	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_6].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_6].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_6].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_6].m_FOV		= 35.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_6].m_CameraSpeakerOffset  = CVec3Dfp32(0.0f, 0.0f, 0.1f);
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_6].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, 0.1f);

	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_7].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_7].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_7].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_7].m_FOV		= 35.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_7].m_CameraSpeakerOffset  = CVec3Dfp32(0.0f, 0.0f, 0.1f);
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_7].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, 0.1f);

	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_8].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_8].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_8].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_8].m_FOV		= 35.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_8].m_CameraSpeakerOffset  = CVec3Dfp32(0.0f, 0.0f, 0.1f);
	m_CameraSetups[CAMERA_TEMPLATE_TWOSHOT_8].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, 0.1f);

	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_0].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_0].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_0].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_0].m_FOV		= 30.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_0].m_CameraSpeakerOffset  = CVec3Dfp32(16.0f, 1.0f, 0.0f);
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_0].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, 0.0f);

	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_1].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_1].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_1].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_1].m_FOV		= 30.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_1].m_CameraSpeakerOffset  = CVec3Dfp32(15.0f, 9.0f, 0.0f);
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_1].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, 0.0f);

	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_2].Clear();
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_2].m_CameraMode	= DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT;
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_2].m_CameraModeParameter	= 0;
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_2].m_FOV		= 30.0f;
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_2].m_CameraSpeakerOffset  = CVec3Dfp32(24.0f, 4.0f, 12.0f);
	m_CameraSetups[CAMERA_TEMPLATE_TELEPHONE_2].m_CameraListenerOffset = CVec3Dfp32(0.0f, 0.0f, 0.0f);
}
