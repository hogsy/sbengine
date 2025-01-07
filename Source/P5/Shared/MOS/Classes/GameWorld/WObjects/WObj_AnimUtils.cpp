/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_AnimUtils.h

	Author:			Jens Anderssion

	Copyright:		Copyright Starbreeze AB 2006

	Contents:		CWO_AnimUtils

	Comments:		Handles generic character-animation thingys

	History:		
		060428:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_AnimUtils.h"
#include "WObj_Vocap.h"
#include "../../../MSystem/Sound/LipSync/LipSync.h"

CXR_Anim_TrackMask CWO_AnimUtils::ms_FaceDataTrackMask;
bool CWO_AnimUtils::ms_bFaceDataTrackMaskInit = false;

void CWO_AnimUtils::ApplyRollBone(CWObject_CoreData *_pObj, CXR_Skeleton *_pSkel, CXR_SkeletonInstance *_pSkelInstance)
{
	// Disabled for now...
	return;

	CMat4Dfp32 Res;
	CMat4Dfp32 MInvRefAxisRot, MRefAxisRot;
	CQuatfp32 QShoulder;
	CAxisRotfp32 AShoulder;

	{
		// Find axis that rollbone should rotate around
		CVec3Dfp32 Ref = (_pSkel->m_lNodes[14].m_LocalCenter - _pSkel->m_lNodes[13].m_LocalCenter).Normalize();

		// Create AxisRotation from Shoulder-matrix
		M_ASSERT(_pSkelInstance->m_nBoneLocalPos > 13, "Not enough bones in skeleton instance");
		QShoulder.Create(_pSkelInstance->m_pBoneLocalPos[13]);
		AShoulder.Create(QShoulder);

		// Find how much Shoulder-matrix have rotated around RefAxis
		CAxisRotfp32 InvRefAxisRot(Ref, -AShoulder.m_Angle * (AShoulder.m_Axis * Ref) * 0.4f);

		// Remove rotation around RefAxis from Shoulder-matrix
		InvRefAxisRot.CreateMatrix(MInvRefAxisRot);
		M_ASSERT(_pSkelInstance->m_nBoneLocalPos > 13, "Not enough bones in skeleton instance");
		MInvRefAxisRot.Multiply(_pSkelInstance->m_pBoneLocalPos[13], Res);
		M_ASSERT(_pSkelInstance->m_nBoneLocalPos > 70, "Not enough bones in skeleton instance");
		_pSkelInstance->m_pBoneLocalPos[70] = Res;

		// Apply the rotation to roll-bone instead
		MInvRefAxisRot.InverseOrthogonal(MRefAxisRot);
		M_ASSERT(_pSkelInstance->m_nBoneLocalPos > 71, "Not enough bones in skeleton instance");
		for(int i = 0; i < 3; i++)
			for(int j = 0; j < 3; j++)
				_pSkelInstance->m_pBoneLocalPos[71].k[i][j] = MRefAxisRot.k[i][j];
	}

	{
		CVec3Dfp32 Ref = (_pSkel->m_lNodes[19].m_LocalCenter - _pSkel->m_lNodes[18].m_LocalCenter).Normalize();
		M_ASSERT(_pSkelInstance->m_nBoneLocalPos > 18, "Not enough bones in skeleton instance");
		QShoulder.Create(_pSkelInstance->m_pBoneLocalPos[18]);
		AShoulder.Create(QShoulder);
		//ConOut(CStrF("%f", AShoulder.m_Angle * (AShoulder.m_Axis * Ref)));
		CAxisRotfp32 InvRefAxisRot(Ref, -AShoulder.m_Angle * (AShoulder.m_Axis * Ref) * 0.4f);
		InvRefAxisRot.CreateMatrix(MInvRefAxisRot);
		MInvRefAxisRot.Multiply(_pSkelInstance->m_pBoneLocalPos[18], Res);
		M_ASSERT(_pSkelInstance->m_nBoneLocalPos > 72, "Not enough bones in skeleton instance");
		_pSkelInstance->m_pBoneLocalPos[72] = Res;
		MInvRefAxisRot.InverseOrthogonal(MRefAxisRot);
		M_ASSERT(_pSkelInstance->m_nBoneLocalPos > 73, "Not enough bones in skeleton instance");
		for(int i = 0; i < 3; i++)
			for(int j = 0; j < 3; j++)
				_pSkelInstance->m_pBoneLocalPos[73].k[i][j] = MRefAxisRot.k[i][j];
	}
}

void CWO_AnimUtils::SetSkeletonScale(CXR_SkeletonInstance *_pSkelInstance, fp32 _Scale)
{
	// Set model scale
	static const int BoneSet[] =
	{
		PLAYER_ROTTRACK_ROOT, PLAYER_ROTTRACK_RLEG, PLAYER_ROTTRACK_RKNEE, 
		PLAYER_ROTTRACK_RFOOT, PLAYER_ROTTRACK_LLEG, PLAYER_ROTTRACK_LKNEE, 
		PLAYER_ROTTRACK_LFOOT, PLAYER_ROTTRACK_SPINE, PLAYER_ROTTRACK_TORSO, 
		PLAYER_ROTTRACK_NECK, PLAYER_ROTTRACK_HEAD, PLAYER_ROTTRACK_RSHOULDER, 
		PLAYER_ROTTRACK_RARM, PLAYER_ROTTRACK_RELBOW,  
		PLAYER_ROTTRACK_RHAND, PLAYER_ROTTRACK_LSHOULDER, PLAYER_ROTTRACK_LARM,
		PLAYER_ROTTRACK_LELBOW, PLAYER_ROTTRACK_LHAND,
		PLAYER_ROTTRACK_RHANDATTACH, PLAYER_ROTTRACK_LHANDATTACH, 
		PLAYER_ROTTRACK_CAMERA
	};
	static const int BoneSetLen = sizeof(BoneSet) / sizeof(int);

	_pSkelInstance->ApplyScale(_Scale, BoneSet, BoneSetLen);
}

bool CWO_AnimUtils::SetEyeMuscles_LookAtPosition(const CVec3Dfp32 &_Pos, fp32 *_plFaceData, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel)
{
	if(_pSkel->m_lNodes.Len() <= PLAYER_ROTTRACK_HEAD)
		return false;

	const CMat4Dfp32& MatHead = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_HEAD];
	CVec3Dfp32 Pos = (_pSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter + CVec3Dfp32(5, 0, 1)) * MatHead;
	CVec3Dfp32 Dir = -(Pos - _Pos).Normalize();
	return SetEyeMuscles_LookDirection(Dir, _plFaceData, _pSkelInstance);
}

bool CWO_AnimUtils::SetEyeMuscles_LookDirection(const CVec3Dfp32 &_Dir, fp32 *_plFaceData, CXR_SkeletonInstance *_pSkelInstance)
{
	if(_pSkelInstance->m_nBoneTransform <= PLAYER_ROTTRACK_HEAD)
		return false;

	const CMat4Dfp32& MatHead = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_HEAD];
	fp32 Fwd = _Dir * MatHead.GetRow(0);
	if(Fwd > 0.1f)
	{
		fp32 Right = (M_ACos(  _Dir * MatHead.GetRow(1) ) * (1.0f / _PI) - 0.5f) * (1.0f / 0.2f);
		fp32 Up =    (M_ACos(-(_Dir * MatHead.GetRow(2))) * (1.0f / _PI) - 0.5f) * (1.0f / 0.2f);
		_plFaceData[PLAYER_FACEDATA_LOOK_UP] =    Clamp01(_plFaceData[PLAYER_FACEDATA_LOOK_UP] + Right);
		_plFaceData[PLAYER_FACEDATA_LOOK_DOWN] =  Clamp01(_plFaceData[PLAYER_FACEDATA_LOOK_DOWN] - Right);
		_plFaceData[PLAYER_FACEDATA_LOOK_RIGHT] = Clamp01(_plFaceData[PLAYER_FACEDATA_LOOK_RIGHT] + Up);
		_plFaceData[PLAYER_FACEDATA_LOOK_LEFT] =  Clamp01(_plFaceData[PLAYER_FACEDATA_LOOK_LEFT] - Up);
		return true;
	}
	else
		return false;
}

void CWO_AnimUtils::RotateBone_ToLookAt(int _iTrack, const CVec3Dfp32 &_TargetPos, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel)
{
	CMat4Dfp32& MatHead = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_HEAD];
	CMat4Dfp32& Mat = _pSkelInstance->m_pBoneTransform[_iTrack];
	CVec3Dfp32 LC = _pSkel->m_lNodes[_iTrack].m_LocalCenter;
	CVec3Dfp32 Pos = LC * Mat;
	CVec3Dfp32 Dir = (_TargetPos - Pos).Normalize();
	if(MatHead.GetRow(0) * Dir >= 0.97f)
	{	// We don't track well toward the edges of our vision (also we rely on muscle clamping)
		Mat.GetRow(0) = Dir;
		Mat.RecreateMatrix(0, 1);
		LC.MultiplyMatrix3x3(Mat);
		Mat.GetRow(3) = Pos - LC;
	}
}

void CWO_AnimUtils::RotateEyeBones_ToLookAt(const CVec3Dfp32 &_TargetPos, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel)
{
	uint nJoints = Min(int(_pSkelInstance->m_nBoneTransform), _pSkel->m_lNodes.Len());
	if (nJoints <= PLAYER_ROTTRACK_LEYE)
		return;

	RotateBone_ToLookAt(PLAYER_ROTTRACK_LEYE, _TargetPos, _pSkelInstance, _pSkel);
	RotateBone_ToLookAt(PLAYER_ROTTRACK_REYE, _TargetPos, _pSkelInstance, _pSkel);
}

CXR_Anim_TrackMask *CWO_AnimUtils::GetFaceDataTrackMask()
{
	if(!ms_bFaceDataTrackMaskInit)
	{
		ms_FaceDataTrackMask.Clear();
		for(int i = FACEDATA_BASETRACK; i < FACEDATA_BASETRACK + ((FACEDATA_MAXCHANNELS + 2) / 3); i++)
			ms_FaceDataTrackMask.m_TrackMaskMove.Enable(i);

		ms_FaceDataTrackMask.m_TrackMaskRot.Enable(PLAYER_ROTTRACK_CAMERA_PRIMARY);
		ms_FaceDataTrackMask.m_TrackMaskRot.Enable(PLAYER_ROTTRACK_CAMERA_SECONDARY);
		ms_FaceDataTrackMask.m_TrackMaskMove.Enable(PLAYER_MOVETRACK_CAMERA_PRIMARY);
		ms_FaceDataTrackMask.m_TrackMaskMove.Enable(PLAYER_MOVETRACK_CAMERA_SECONDARY);
		ms_FaceDataTrackMask.m_TrackMaskMove.Enable(PLAYER_MOVETRACK_CAMERA_CONTROLLER);

		ms_bFaceDataTrackMaskInit = true;
	}
	return &ms_FaceDataTrackMask;
}

void CWO_AnimUtils::ClearCameraTracks(CXR_SkeletonInstance *_pSkelInstance)
{
	if(_pSkelInstance->m_nTracksRot > PLAYER_ROTTRACK_CAMERA_SECONDARY)
	{
		_pSkelInstance->m_pTracksRot[PLAYER_ROTTRACK_CAMERA_PRIMARY].Unit();
		_pSkelInstance->m_pTracksRot[PLAYER_ROTTRACK_CAMERA_SECONDARY].Unit();
	}
	if(_pSkelInstance->m_nTracksMove> PLAYER_MOVETRACK_CAMERA_CONTROLLER)
	{
		_pSkelInstance->m_pTracksMove[PLAYER_MOVETRACK_CAMERA_PRIMARY] =  M_VConst(0,0,0,1.0f);
		_pSkelInstance->m_pTracksMove[PLAYER_MOVETRACK_CAMERA_SECONDARY] =  M_VConst(0,0,0,1.0f);
		_pSkelInstance->m_pTracksMove[PLAYER_MOVETRACK_CAMERA_CONTROLLER] = 0;
	}
}

bool CWO_AnimUtils::GetCamera(CXR_Skeleton *_pSkel, CXR_SkeletonInstance *_pSkelInstance, CMat4Dfp32 &_Camera, fp32 *_pFOV)
{
	if(_pSkelInstance->m_nTracksRot > PLAYER_ROTTRACK_CAMERA_SECONDARY && _pSkelInstance->m_nTracksMove > PLAYER_MOVETRACK_CAMERA_CONTROLLER &&
		_pSkelInstance->m_pTracksMove[PLAYER_MOVETRACK_CAMERA_PRIMARY].k[0] !=0 &&
		_pSkelInstance->m_pTracksMove[PLAYER_MOVETRACK_CAMERA_PRIMARY].k[1] !=0 &&
		_pSkelInstance->m_pTracksMove[PLAYER_MOVETRACK_CAMERA_PRIMARY].k[2] !=0)
	{
		CVec4Dfp32 Ctrl = _pSkelInstance->m_pTracksMove[PLAYER_MOVETRACK_CAMERA_CONTROLLER];
		if(Ctrl[2] > 0.5f)
		{
			_Camera.Create(_pSkelInstance->m_pTracksRot[PLAYER_ROTTRACK_CAMERA_SECONDARY], 
				_pSkelInstance->m_pTracksMove[PLAYER_MOVETRACK_CAMERA_SECONDARY]);
			if(_pFOV)
				*_pFOV = Ctrl[1];
		}
		else
		{
			_Camera.Create(_pSkelInstance->m_pTracksRot[PLAYER_ROTTRACK_CAMERA_PRIMARY], 
				_pSkelInstance->m_pTracksMove[PLAYER_MOVETRACK_CAMERA_PRIMARY]);
			if(_pFOV)
				*_pFOV = Ctrl[0];
		}

		CWO_AnimUtils::ClearCameraTracks(_pSkelInstance);
		return true;
	}
	else if(_pSkelInstance->m_nTracksRot> PLAYER_ROTTRACK_CAMERA && _pSkel->m_lNodes.Len() > PLAYER_ROTTRACK_CAMERA)
	{
		_Camera = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];
		_Camera.GetRow(3) = _pSkel->m_lNodes[PLAYER_ROTTRACK_CAMERA].m_LocalCenter * _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];
		if(_pFOV)
			*_pFOV = -1;
		return true;
	}

	return false;
}

void CWO_AnimUtils::ScaleFace(fp32 *_plFaceData, int _DialogueAnimFlags)
{
	if(_DialogueAnimFlags & DIALOGUEANIMFLAGS_FACESCALE_FACE_1)
	{
		/*for(int i = 0; i <= PLAYER_FACEDATA_LSQUINT; i++)
			_plFaceData[i] = Clamp01(_plFaceData[i] * 3.0f);*/
		for(int i = 0; i < FACEDATA_MAXCHANNELS; i++)
			_plFaceData[i] = Clamp01(_plFaceData[i] * 1.5f);
	}
	if(_DialogueAnimFlags & DIALOGUEANIMFLAGS_FACESCALE_KISS_1)
		_plFaceData[PLAYER_FACEDATA_ORBI_ORIS_KISS] = Clamp01(_plFaceData[PLAYER_FACEDATA_ORBI_ORIS_KISS] * 3.0f);

	if(_DialogueAnimFlags & DIALOGUEANIMFLAGS_FACESCALE_WIDE_1)
		_plFaceData[PLAYER_FACEDATA_ORBI_ORIS_WIDE] = Clamp01(_plFaceData[PLAYER_FACEDATA_ORBI_ORIS_WIDE] * 0.2f);
}

uint16 CWO_AnimUtils::TranslateDialogueAnimFlags(const char *_pSt)
{
	static char *lTranslate[] = { "Lipsync50", "Lipsync100", "LipsyncChin", "SpecialSync",
								  "", "", "", "",
								  "", "", "FaceUp", "",
								  "KissUp", "", "WideDown", "" };

	return CStr::TranslateFlags(_pSt, (const char**)lTranslate);
}

void CWO_AnimUtils::EvalVocapAndLipsync(class CVoCap *_pVocap, class CLipSync *_pLipsync, CMapData* _pMapData, CSoundContext *_pSound, int _iLipsyncAnim, int _hVoice, const CMTime &_ReferenceTime, int _iMovingHold, class CXR_AnimLayer* _pLayers, int &_nLayers, int _MaxLayers, bool _bSkipLipSync, CWorld_Client* _pWClientDebugRender)
{
	fp32 MaxBlend = _pVocap->Eval(_pSound, _hVoice, _ReferenceTime, _iMovingHold, _pLayers, _nLayers, _MaxLayers, _pWClientDebugRender);
	int DialogueAnimFlags = _pVocap->m_DialogueAnimFlags;
	if(!_bSkipLipSync && (MaxBlend < 0.99f || (DialogueAnimFlags & (DIALOGUEANIMFLAGS_LIPSYNC_50 | DIALOGUEANIMFLAGS_LIPSYNC_100 | DIALOGUEANIMFLAGS_LIPSYNC_CHIN))))
	{
		fp32 Blend = 1.0f - MaxBlend;
		if(DialogueAnimFlags & DIALOGUEANIMFLAGS_LIPSYNC_50)
			Blend = Min(Blend, 0.5f);
		else if(DialogueAnimFlags & DIALOGUEANIMFLAGS_LIPSYNC_100)
			Blend = 1.0f;

		const static int RotTracks[] = { PLAYER_ROTTRACK_HEAD };
		_pLipsync->Eval(_pSound, _iLipsyncAnim, _pMapData, _pLayers, _nLayers, RotTracks, sizeof(RotTracks) / sizeof(int), Blend);
	}

}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_EyeAnim
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_EyeAnim::CWO_EyeAnim()
{
	m_iAnimResource = 0;
}

void CWO_EyeAnim::Init(CMapData* _pMapData)
{
	m_iAnimResource = _pMapData->GetResourceIndex_Anim("Eyes/Eyes");	
	if(m_iAnimResource <= 0)
		ConOutL("§cf00ERROR: (CWO_EyeAnim::Init) Could not load animation");
}

bool CWO_EyeAnim::Eval(CMapData* _pMapData, int _iAnim, const CMTime &_ReferenceTime, class CXR_AnimLayer* _pLayers, int &_nLayers, int _MaxLayers)
{
	if(m_iAnimResource <= 0 || _nLayers >= _MaxLayers)
		return false;

	CXR_Anim_Base *pAnim = _pMapData->GetResource_Anim(m_iAnimResource);
	if(pAnim->GetNumSequences() <= _iAnim)
		ConOutL(CStrF("§cf00ERROR: (CWO_EyeAnim::Eval) Container didn't include sequence %i", _iAnim));

	CXR_Anim_SequenceData *pSeq = pAnim->GetSequence(_iAnim);
	if(pSeq)
		_pLayers[_nLayers++] = CXR_AnimLayer(pSeq, pSeq->GetLoopedTime(_ReferenceTime), 1.0f, 1.0f, 11, CXR_ANIMLAYER_IGNOREBASENODE | CXR_ANIMLAYER_ADDITIVEBLEND);
	return true;
}
