/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_AnimUtils.h

	Author:			Jens Anderssion

	Copyright:		Copyright Starbreeze AB 2006

	Contents:		CWO_AnimUtils

	Comments:		Handles generic character-animation thingys

	History:		
		060428:		Created File
\*____________________________________________________________________________________________*/
#ifndef __WOBJ_ANIMUTILS_H__
#define __WOBJ_ANIMUTILS_H__

enum
{
	// Attachpoints
	PLAYER_ATTACHPOINT_RFOOT = 3,
	PLAYER_ATTACHPOINT_LFOOT = 4,
	PLAYER_ATTACHPOINT_FACE  = 5,

	// Bone rotation tracks
	PLAYER_ROTTRACK_ROOT = 1,

	PLAYER_ROTTRACK_RLEG = 2,
	PLAYER_ROTTRACK_RKNEE = 3,
	PLAYER_ROTTRACK_RFOOT = 4,
	PLAYER_ROTTRACK_LLEG = 5,
	PLAYER_ROTTRACK_LKNEE = 6,
	PLAYER_ROTTRACK_LFOOT = 7,
	PLAYER_ROTTRACK_SPINE = 8,
	PLAYER_ROTTRACK_TORSO = 9,		// aka SPINE1
	PLAYER_ROTTRACK_SPINE2 = 15,
	PLAYER_ROTTRACK_NECK = 10,
	PLAYER_ROTTRACK_HEAD = 11,

	PLAYER_ROTTRACK_RSHOULDER = 12,
	PLAYER_ROTTRACK_RARM = 13,
	PLAYER_ROTTRACK_RELBOW = 14,
	PLAYER_ROTTRACK_RHAND = 16,
	PLAYER_ROTTRACK_RHANDATTACH = 22,

	PLAYER_ROTTRACK_LSHOULDER = 17,
	PLAYER_ROTTRACK_LARM = 18,
	PLAYER_ROTTRACK_LELBOW = 19,
	PLAYER_ROTTRACK_LHAND = 21,
	PLAYER_ROTTRACK_LHANDATTACH = 23,

	PLAYER_ROTTRACK_CAMERA = 24,

	PLAYER_ROTTRACK_CAMERA_PRIMARY = 27,
	PLAYER_MOVETRACK_CAMERA_PRIMARY = 8,
	PLAYER_ROTTRACK_CAMERA_SECONDARY = 29,
	PLAYER_MOVETRACK_CAMERA_SECONDARY = 10,
	PLAYER_MOVETRACK_CAMERA_CONTROLLER = 14,

	PLAYER_ROTTRACK_JAW = 20,
	PLAYER_ROTTRACK_REYE = 89,
	PLAYER_ROTTRACK_LEYE = 90,

	PLAYER_ROTTRACK_RHAND_THUMB_1 = 38,
	PLAYER_ROTTRACK_RHAND_THUMB_2 = 39,
	PLAYER_ROTTRACK_RHAND_THUMB_3 = 40,
	PLAYER_ROTTRACK_RHAND_INDEX_1 = 41,
	PLAYER_ROTTRACK_RHAND_INDEX_2 = 42,
	PLAYER_ROTTRACK_RHAND_INDEX_3 = 43,
	PLAYER_ROTTRACK_RHAND_MIDDLE_1 = 44,
	PLAYER_ROTTRACK_RHAND_MIDDLE_2 = 45,
	PLAYER_ROTTRACK_RHAND_MIDDLE_3 = 46,
	PLAYER_ROTTRACK_RHAND_RING_1 = 47,
	PLAYER_ROTTRACK_RHAND_RING_2 = 48,
	PLAYER_ROTTRACK_RHAND_RING_3 = 49,
	PLAYER_ROTTRACK_RHAND_PINKY_1 = 50,
	PLAYER_ROTTRACK_RHAND_PINKY_2 = 51,
	PLAYER_ROTTRACK_RHAND_PINKY_3 = 52,

	PLAYER_ROTTRACK_LHAND_THUMB_1 = 53,
	PLAYER_ROTTRACK_LHAND_THUMB_2 = 54,
	PLAYER_ROTTRACK_LHAND_THUMB_3 = 55,
	PLAYER_ROTTRACK_LHAND_INDEX_1 = 56,
	PLAYER_ROTTRACK_LHAND_INDEX_2 = 57,
	PLAYER_ROTTRACK_LHAND_INDEX_3 = 58,
	PLAYER_ROTTRACK_LHAND_MIDDLE_1 = 59,
	PLAYER_ROTTRACK_LHAND_MIDDLE_2 = 60,
	PLAYER_ROTTRACK_LHAND_MIDDLE_3 = 61,
	PLAYER_ROTTRACK_LHAND_RING_1 = 62,
	PLAYER_ROTTRACK_LHAND_RING_2 = 63,
	PLAYER_ROTTRACK_LHAND_RING_3 = 64,
	PLAYER_ROTTRACK_LHAND_PINKY_1 = 65,
	PLAYER_ROTTRACK_LHAND_PINKY_2 = 66,
	PLAYER_ROTTRACK_LHAND_PINKY_3 = 67,

	FACEDATA_BASETRACK = 21,					// movetrack 21..37 are used to store muscle group anim values (0..100)
	FACEDATA_MAXCHANNELS = 51,

	PLAYER_FACEDATA_RFRONTALIS_MAJOR = 0,
	PLAYER_FACEDATA_LFRONTALIS_MAJOR = 1,
	PLAYER_FACEDATA_RCORRUGATOR_MAJOR = 2,
	PLAYER_FACEDATA_LCORRUGATOR_MAJOR = 3,
	PLAYER_FACEDATA_RCORRUGATOR_MINOR = 4,
	PLAYER_FACEDATA_LCORRUGATOR_MINOR = 5,
	PLAYER_FACEDATA_RFRONTALIS_MINOR = 6,
	PLAYER_FACEDATA_LFRONTALIS_MINOR = 7,
	PLAYER_FACEDATA_REYELID_UP_CLOSE = 8,		// closes upper eyelid
	PLAYER_FACEDATA_LEYELID_UP_CLOSE = 9,
	PLAYER_FACEDATA_REYELID_UP_OPEN = 10,		// opens upper eyelid
	PLAYER_FACEDATA_LEYELID_UP_OPEN = 11,
	PLAYER_FACEDATA_REYELID_DOWN_CLOSE = 12,	// closes lower eyelid
	PLAYER_FACEDATA_LEYELID_DOWN_CLOSE = 13,
	PLAYER_FACEDATA_REYELID_DOWN_OPEN = 14,		// opens lower eyelid
	PLAYER_FACEDATA_LEYELID_DOWN_OPEN = 15,
	PLAYER_FACEDATA_RLEVATOR_LABII = 16,
	PLAYER_FACEDATA_LLEVATOR_LABII = 17,
	PLAYER_FACEDATA_RZYGOMATIC_MAJOR = 18,
	PLAYER_FACEDATA_LZYGOMATIC_MINOR = 19,
	PLAYER_FACEDATA_RSQUINT = 20,
	PLAYER_FACEDATA_LSQUINT = 21,
	PLAYER_FACEDATA_ORBI_ORIS_KISS = 22,
	PLAYER_FACEDATA_ORBI_ORIS_WIDE = 23,
	PLAYER_FACEDATA_ORBI_UP_U = 24,
	PLAYER_FACEDATA_ORBI_DOWN_U = 25,
	PLAYER_FACEDATA_ORBI_UP_P = 26,
	PLAYER_FACEDATA_ORBI_DOWN_P = 27,
	PLAYER_FACEDATA_RORIS_LEVATOR = 28,
	PLAYER_FACEDATA_LORIS_LEVATOR = 29,
	PLAYER_FACEDATA_RDEPRESSOR_LABII = 30,
	PLAYER_FACEDATA_LDEPRESSOR_LABII = 31,
	PLAYER_FACEDATA_RTRIANGULARIS = 32,
	PLAYER_FACEDATA_LTRIANGULARIS = 33,
	PLAYER_FACEDATA_MENTALIS = 34,
	PLAYER_FACEDATA_RLIP_LOWER_PINCH = 35,
	PLAYER_FACEDATA_LLIP_LOWER_PINCH = 36,
	PLAYER_FACEDATA_CHIN_OPEN = 37,
	PLAYER_FACEDATA_CHIN_CLOSE = 38,
	PLAYER_FACEDATA_CHIN_FORWARDS = 39,
	PLAYER_FACEDATA_CHIN_BACKWARDS = 40,
	PLAYER_FACEDATA_CHIN_RIGHT = 41,
	PLAYER_FACEDATA_CHIN_LEFT = 42,
	PLAYER_FACEDATA_CHIN_ROT_R = 43,
	PLAYER_FACEDATA_CHIN_ROT_L = 44,
	PLAYER_FACEDATA_LOOK_UP = 45,				// makes eyes look up.  
	PLAYER_FACEDATA_LOOK_DOWN = 46,				// Make sure eyes are last of the facedata and the first (PLAYER_FACEDATA_LOOK_UP) is dividable
	PLAYER_FACEDATA_LOOK_RIGHT = 47,			// by 3, or the eyedata/facedata code in CNode_Anim::BuildKeyFrames will be messed up
	PLAYER_FACEDATA_LOOK_LEFT = 48,

	// Anim events  (TODO: merge with the ones in AG2?)
	ANIM_EVENT_TYPE_IK_LEFT_FOOT_DOWN = 0,
	ANIM_EVENT_TYPE_IK_LEFT_FOOT_UP,
	ANIM_EVENT_TYPE_IK_RIGHT_FOOT_DOWN,
	ANIM_EVENT_TYPE_IK_RIGHT_FOOT_UP,
	ANIM_EVENT_TYPE_IK_LEFT_FRONT_FOOT_DOWN,
	ANIM_EVENT_TYPE_IK_LEFT_FRONT_FOOT_UP,
	ANIM_EVENT_TYPE_IK_RIGHT_FRONT_FOOT_DOWN,
	ANIM_EVENT_TYPE_IK_RIGHT_FRONT_FOOT_UP,
	ANIM_EVENT_TYPE_IK_RECOIL,

	ANIM_EVENT_EFFECTTYPE_WEAPON = 0,

	DIALOGUEANIMFLAGS_LIPSYNC_50 = M_Bit(0),
	DIALOGUEANIMFLAGS_LIPSYNC_100 = M_Bit(1),
	DIALOGUEANIMFLAGS_LIPSYNC_CHIN = M_Bit(2),

	DIALOGUEANIMFLAGS_VOCAP_SPECIALSYNC = M_Bit(3),

	DIALOGUEANIMFLAGS_FACESCALE_FACE_1 = M_Bit(10),
	DIALOGUEANIMFLAGS_FACESCALE_KISS_1 = M_Bit(12),
	DIALOGUEANIMFLAGS_FACESCALE_WIDE_1 = M_Bit(14),

	AIMING_NORMAL = 0,
	AIMING_BODY = 1,
	AIMING_NONE = 2,
	AIMING_HEAD = 3,
	AIMING_FULLBODY = 4,
	AIMING_MINIGUN = 7,
	AIMING_CROUCHBODY = 9,
	AIMING_CARRYBODY = 10,	
	AIMING_UNARMED = 11,
};
	
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_AnimUtils
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_AnimUtils
{
private:
	static CXR_Anim_TrackMask ms_FaceDataTrackMask;
	static bool ms_bFaceDataTrackMaskInit;

public:
	static void ApplyRollBone(CWObject_CoreData *_pObj, CXR_Skeleton *_pSkel, CXR_SkeletonInstance *_pSkelInstance);
	static void SetSkeletonScale(CXR_SkeletonInstance *_pSkelInstance, fp32 _Scale);

	static bool SetEyeMuscles_LookAtPosition(const CVec3Dfp32 &_Pos, fp32 *_plFaceData, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel);
	static bool SetEyeMuscles_LookDirection(const CVec3Dfp32 &_Dir, fp32 *_plFaceData, CXR_SkeletonInstance *_pSkelInstance);

	static void RotateBone_ToLookAt(int _iTrack, const CVec3Dfp32 &_TargetPos, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel);
	static void RotateEyeBones_ToLookAt(const CVec3Dfp32 &_TargetPos, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel);

	static CXR_Anim_TrackMask *GetFaceDataTrackMask();

	static void ClearCameraTracks(CXR_SkeletonInstance *_pSkelInstance);			// Since we store camera data on face-bones, we need to clear them before they get to face-eval
	static bool GetCamera(CXR_Skeleton *_pSkel, CXR_SkeletonInstance *_pSkelInstance, CMat4Dfp32 &_Camera, fp32 *_pFOV = NULL);

	static void ScaleFace(fp32 *_plFaceData, int _DialogueAnimFlags);
	static uint16 TranslateDialogueAnimFlags(const char *_pSt);

	static void EvalVocapAndLipsync(class CVoCap *_pVocap, class CLipSync *_pLipsync, CMapData* _pMapData, CSoundContext *_pSound, int _iLipsyncAnim, int _hVoice, const CMTime &_ReferenceTime, int _iMovingHold, class CXR_AnimLayer* _pLayers, int &_nLayers, int _MaxLayers, bool _bSkipLipsync, class CWorld_Client* _pWClientDebugRender = NULL);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_EyeAnim
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_EyeAnim
{
public:
	enum
	{
		NUM_EYEANIM = 1,
	};

	int m_iAnimResource;

	CWO_EyeAnim();
	void Init(CMapData* _pMapData);
	bool Eval(CMapData* _pMapData, int _iAnim, const CMTime &_ReferenceTime, class CXR_AnimLayer* _pLayers, int &_nLayers, int _MaxLayers);
};

#endif // __WOBJ_ANIMUTILS_H__
