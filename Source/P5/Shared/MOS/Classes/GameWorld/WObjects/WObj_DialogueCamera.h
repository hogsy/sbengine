#ifndef __WOBJ_DIALOGUECAMERA_H
#define __WOBJ_DIALOGUECAMERA_H

enum
{
	// Modes of the camera as read by script. Max 16 modes in total from dialogue token (4bits)
	DIALOGUECAMERA_MODE_UNDEFINED = -1,			// Undefined mode

	DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT = 0,		// Default third person camera. Two shoot camera, positioned just behind listener.  
	DIALOGUECAMERA_MODE_THIRDPERSON_NOCLIP = 1,			// Third person camera. Does´nt clip when speaker/listener is swapped.
	DIALOGUECAMERA_MODE_BOUNDING_BOX = 2,				// Also a two shoot camera, but the camera is positioned safe inside the bounding box
	DIALOGUECAMERA_MODE_SCRIPTED = 3,					// Scripted camera, get positions from level data
	DIALOGUECAMERA_MODE_LOOKAT_SPEAKER = 4,				// Look at speaker, no clip
	DIALOGUECAMERA_MODE_LOOKAT_LISTENER = 5,			// Look at listener, no clip
	DIALOGUECAMERA_MODE_SCRIPTED_TELEPHONE = 6,			// Telephone camera from animation
	DIALOGUECAMERA_MODE_PREVIOUS_CAMERA = 7,			// Fall back on previous camera

	DIALOGUECAMERA_MODE_TEST_SELFROTATE = 8,			// Experimental camera mode, let camera rotate around fixed position while in 3rd person

	DIALOGUECAMERA_LOOKAT_SPEAKER = 0,				// Speaker with FOV coeff'ed height offset
	DIALOGUECAMERA_LOOKAT_INTERPOLATED,				// Interpolated between speaker and listener eye positions with FOV coeff'ed height offset
	//DIALOGUECAMERA_LOOKAT_LISTENER,

	DIALOGUECAMERA_UNDEFINED = -1,

	DIALOGUECAMERA_FLAG_MATSET			= 1 << 0,
};

class CWO_DialogueCamera : public CReferenceCount
{
public:
	CWO_DialogueCamera();
	void Init(CMapData* _pMapData);

	/*int GetCameraAt(CWorld_Client* _pWClient,
	CDialogueToken* _pDialogueToken,
	const int *_iObjectIndices,
	CMat4Dfp32& _Matrix,
	const fp32 _IPTime,
	const CMTime& _Time,
	bool _bGetSafeBB = false);*/

	int GetCameraAt(CWorld_Client* _pWClient,
		CDialogueToken* _pDialogueToken,
		const CMat4Dfp32* _pMatrices,
		const fp32* _pHeadOffsets,
		const int* _iObjectIndices,
		CMat4Dfp32& _Matrix,
		const CMTime& _Time,
		CMat4Dfp32& _MouthPos, 	//_MouthPos is only valid of bTelephone is true
		bool _bSwappedSpeaker = false,
		bool _bFirstFrameIn3PI = false,
		bool _bTelephone = false);


	enum
	{
		CAMERA_TEMPLATE_TWOSHOT_0,
		CAMERA_TEMPLATE_TWOSHOT_1,
		CAMERA_TEMPLATE_TWOSHOT_2,
		CAMERA_TEMPLATE_TWOSHOT_3,
		CAMERA_TEMPLATE_TWOSHOT_4,
		CAMERA_TEMPLATE_TWOSHOT_5,
		CAMERA_TEMPLATE_TWOSHOT_6,
		CAMERA_TEMPLATE_TWOSHOT_7,
		CAMERA_TEMPLATE_TWOSHOT_8,
		CAMERA_TEMPLATE_TELEPHONE_0,
		CAMERA_TEMPLATE_TELEPHONE_1,
		CAMERA_TEMPLATE_TELEPHONE_2,

		CAMERA_TEMPLATE_SIZE,
		CAMERA_TEMPLATE_TWOSHOT_SIZE = CAMERA_TEMPLATE_TWOSHOT_8 + 1,
		CAMERA_TEMPLATE_TELEPHONE_SIZE = (CAMERA_TEMPLATE_SIZE - 1) - CAMERA_TEMPLATE_TWOSHOT_8,
	};

	void SetupCameraTemplates(CMapData* _pMapData);
	CDialogueToken m_CameraSetups[CAMERA_TEMPLATE_SIZE];
	int8 m_iCurrentCameraSetup;

	float GetFOVAt(CWorld_Client* _pWClient, CDialogueToken* _pDialogueToken, const CMTime& _Time);

	int SetMode(int _Mode);
	int GetMode();
	bool GetModeFlipFlag();

	static bool IsInView(const CMat4Dfp32& _Camera, const CVec3Dfp32& _p, float _FOV, float _SafetyBorder);

	void operator =(const CWO_DialogueCamera &_Val);

	void Clear();

	// Need these matrices for dialogue light
	const CMat4Dfp32& GetMatListener() { return m_MatListener; }
	const CMat4Dfp32& GetMatSpeaker() { return m_MatSpeaker; }
	bool GetSwappedSpeakerState() { return m_Mode_Flipped; };
	void SetFirstFrameState(bool _bState) {m_bFirstFrameIn3PI = _bState; };
	bool GetFirstFrameState() { return m_bFirstFrameIn3PI; }

private:

	void GetCameraPosition_Common(CMat4Dfp32& _Mat, CDialogueToken* _pDialogueToken, CVec3Dfp32 *_pOffset, CVec3Dfp32 *_pCameraVal, fp32 _AddrightOffsetter);

	void GetCameraPosition(CVec3Dfp32& _Camera);
	void GetCameraPosition_EnginePath(CVec3Dfp32& _Camera);
	void GetCameraPosition_AttachPoint(CVec3Dfp32& _Camera);
	void GetCameraPosition_OverShoulder(CVec3Dfp32& _Camera);
	void GetCameraPosition_NearOverShoulder(CVec3Dfp32& _Camera);
	void GetCameraPosition_TwoShot(CVec3Dfp32& _Camera, int iCameraModeParameter);
	void GetCameraPosition_Lookat(CVec3Dfp32& _Camera);
	void GetCameraPosition_SpeakerHead(CVec3Dfp32& _Camera);
	void GetCameraPosition_LookAtSpeaker(CMat4Dfp32& _Camera);
	void GetCameraPosition_LookAtListener(CMat4Dfp32& _Camera);

	void GetCameraPosition_SafeBoundingBox(CVec3Dfp32& _Camera, const CVec3Dfp32& _LookAtPos);
	void GetCameraPosition_TestBoundingBox(CMat4Dfp32& _Mat);
	void GetCameraPosition_DefaultCamera(CVec3Dfp32& _Camera);
	void GetCameraPosition_DefaultTwoShot(CMat4Dfp32& _Mat, CDialogueToken* _pDialogueToken);
	void GetCameraPosition_TelephoneScripted(CMat4Dfp32& _Mat, CDialogueToken* m_pDialogueToken);
	void GetLookatPosition(CVec3Dfp32& _Lookat);
	void GetLookatPosition_EnginePath(CVec3Dfp32& _Lookat);
	void GetLookatPosition_Speaker(CVec3Dfp32& _Lookat);
	void GetLookatPosition_SpeakerOffset(CVec3Dfp32& _Lookat);
	void GetLookatPosition_Interpolated(CVec3Dfp32& _Lookat, float _i);
	void GetLookatPosition_Lookat(CVec3Dfp32& _Lookat);

	void LookAt(CMat4Dfp32& _Matrix, const CVec3Dfp32& _LookAt);
	void GetEnginePathPosition(CVec3Dfp32& _Pos, const CMTime& _Time);
	void GetEnginePathMatrix(CMat4Dfp32& _Mat, const CMTime& _Time);

	float GetFOVAt(const CMTime& _Time);

	// camerashaker
	void ApplyCameraShake(const CMTime &_Time, CMat4Dfp32 &_CameraMat, fp32 _CameraShakeVal, fp32 _CameraShakeSpeed);
	CXR_Anim_Base*	m_pCamShakeAnimBase;
	CXR_Anim_Base*	m_pCamTelephoneAnim;
	int				m_iCurCamShakeSeq;
	fp32			m_CamShakeAnimTimer;
	fp32			m_CamTelephoneTimer;
	CMTime			m_LastTime;

	uint16			m_TickToNewCamera;	//Ticks left till the telephone camera will clip and get a new random camera

	bool		m_bForceRightShoulder;
	

	// States
	int					m_Mode;
	int					m_Mode_Camera;
	int					m_Mode_Lookat;
	bool				m_Mode_Flipped;
	bool				m_Mode_DifferentSpeaker;
	uint8				m_Flags;
	int					m_PreviousCameraMode;

	// Temporary variables
	CDialogueToken*		m_pDialogueToken;
	int16				m_iSafeBB;
	fp32					m_Fov;
	fp32					m_FovCoeff;
	CWorld_Client*		m_pWClient;
	CMTime				m_Time;
	CMat4Dfp32			m_Matrix;
	CVec3Dfp32			m_PosSpeakerLookat;
	CVec3Dfp32			m_PosListenerLookat;
	CMat4Dfp32			m_MatSpeaker;
	CMat4Dfp32			m_MatListener;
	CMat4Dfp32			m_OrgMatSpeaker;
	CMat4Dfp32			m_OrgMatListener;
	fp32				m_OrgHeadOffsetSpeaker;
	fp32				m_OrgHeadOffsetListener;
	CVec3Dfp32			m_HeadOfsSpeaker;
	CVec3Dfp32			m_HeadOfsListener;
	CMat4Dfp32			m_OrgCameraMat;				// Save original camera matrix

	CVec3Dfp32			m_OrgSpeakerHeadOffset;
	CVec3Dfp32			m_OrgListenerHeadOffset;

	bool				m_bFirstFrameIn3PI;

	static CDialogueToken	ms_ValidToken;
	static bool				IsValidToken(CDialogueToken* _pDialogueToken);
};

#endif
