/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_ActionCutsceneCamera
					
	Author:			Olle Rosenquist
					
	Copyright:		Copyright O3 Games AB 2002
					
	Contents:		
					
	Comments:		Thirdperson camera for action cutscenes
					
	History:		
		021015:		Created File
\*____________________________________________________________________________________________*/

#ifndef __ACTIONCUTSCENECAMERA_H
#define __ACTIONCUTSCENECAMERA_H

#ifndef __MACROINLINEACCESS
#define __MACROINLINEACCESS
#define MACRO_INLINEACCESS_RW(name, type) \
inline const type& Get##name() const { return m_##name; } \
inline type& Get##name() { return m_##name; }

#define MACRO_INLINEACCESS_R(name, type) \
inline const type& Get##name() const { return m_##name; }

#define MACRO_INLINEACCESS_RWEXT(name, variable, type) \
inline const type& Get##name() const { return variable; } \
inline type& Get##name() { return variable; }

#define MACRO_INLINEACCESS_REXT(name, variable, type) \
inline const type& Get##name() const { return variable; }
#endif

class CWO_Character_ClientData;
class CamVars
{
protected:
	friend class CActionCutsceneCamera;
	// Smooth (hopefully) movement of the camera instead of following the character directly
	CVec3Dfp32 m_LastCameraTarget;
	CVec3Dfp32 m_CameraTarget;
	CVec3Dfp32 m_CameraTargetChange;
	int32 m_CameraMode;
	fp32	m_fTraceHistory;
	fp32 m_fOldTraceHistory;
	fp32 m_CharacterHeightOffset;
	fp32 m_CharacterHeightOffsetCrouch;
	fp32 m_DistanceOffset;
	fp32 m_HeightOffset;
	fp32 m_CameraAngle;

	// Moderate vars....
	fp32 m_LastTargetDistance;
	fp32 m_TargetDistance;
	fp32 m_DistanceChange;

	fp32 m_LastTargetRotation;
	fp32 m_TargetRotation;
	fp32 m_RotationChange;

	// Whatever...
	fp32 m_AngleOffsetX;
	fp32 m_LastAngleOffsetX;
	fp32 m_TargetAngleOffsetX;
	fp32 m_AngleOffsetXChange;

	fp32 m_AngleOffsetY;
	fp32 m_LastAngleOffsetY;
	fp32 m_TargetAngleOffsetY;
	fp32 m_AngleOffsetYChange;

	uint16 m_iRotationIndex;
	uint16 m_iCharacter;
	uint16 m_iObject;
	uint16 m_iCameraObject;

public:
	CamVars();
	void Reset();
	void ResetTraceHistory();
	void CopyFrom(const CamVars& _Vars);

	void SetToPCD(CWO_Character_ClientData* _pCD);
	void ImportFromPCD(CWO_Character_ClientData* _pCD, int _iCharacter);
	void UpdateModVars(CWO_Character_ClientData* _pCD);
	void UpdateHeightOffset(CWO_Character_ClientData* _pCD);
};

class CActionCutsceneCamera;
typedef TPtr<CActionCutsceneCamera> spCActionCutsceneCamera;

// Meh
#define ACSCAMERA_MAXPITCH			(0.1f)
#define ACSCAMERA_MAXYAW			(0.1f)

class CActionCutsceneCamera : public CReferenceCount
{
public:
	enum CameraMode
	{
		CAMERAVIEW_UNDEFINED		= 0,
		CAMERAVIEW_XYVIEWSET		= 1 << 0,
		CAMERAVIEW_ZVIEWSET			= 1 << 1,
		CAMERAVIEW_XSET				= 1 << 2,
		CAMERAVIEW_YSET				= 1 << 3,
		CAMERAMODE_MODESET			= 1 << 4,
		CAMERAVIEW_BASEBIT			= 5,
		
		// "X-Y Plane" views
		CAMERAVIEW_BEHIND			= (1 << (CAMERAVIEW_BASEBIT + 0)) | CAMERAVIEW_XYVIEWSET | CAMERAVIEW_YSET,
		CAMERAVIEW_FRONT			= (1 << (CAMERAVIEW_BASEBIT + 1)) | CAMERAVIEW_XYVIEWSET | CAMERAVIEW_YSET,
		CAMERAVIEW_RIGHTSIDE		= (1 << (CAMERAVIEW_BASEBIT + 2)) | CAMERAVIEW_XYVIEWSET | CAMERAVIEW_XSET,
		CAMERAVIEW_LEFTSIDE			= (1 << (CAMERAVIEW_BASEBIT + 3)) | CAMERAVIEW_XYVIEWSET | CAMERAVIEW_XSET,
		CAMERAVIEW_FIXEDPOSLEFT		= (1 << (CAMERAVIEW_BASEBIT + 4)) | CAMERAVIEW_XYVIEWSET | CAMERAVIEW_XSET,
		CAMERAVIEW_FIXEDPOSRIGHT	= (1 << (CAMERAVIEW_BASEBIT + 5)) | CAMERAVIEW_XYVIEWSET | CAMERAVIEW_XSET,
		
		// "Z-plane" views
		CAMERAVIEW_ABOVE			= (1 << (CAMERAVIEW_BASEBIT + 6)) | CAMERAVIEW_ZVIEWSET,
		CAMERAVIEW_BELOW			= (1 << (CAMERAVIEW_BASEBIT + 7)) | CAMERAVIEW_ZVIEWSET,
		CAMERAVIEW_LEVELED			= (1 << (CAMERAVIEW_BASEBIT + 8)) | CAMERAVIEW_ZVIEWSET,
		
		CAMERAVIEW_NUMBEROFVIEWS	= 9,
		CAMERAVIEW_YMASK			= (CAMERAVIEW_BEHIND | CAMERAVIEW_FRONT | CAMERAVIEW_FIXEDPOSLEFT |CAMERAVIEW_FIXEDPOSRIGHT),
		CAMERAVIEW_XMASK			= (CAMERAVIEW_RIGHTSIDE | CAMERAVIEW_LEFTSIDE |CAMERAVIEW_FIXEDPOSLEFT|CAMERAVIEW_FIXEDPOSRIGHT),
		CAMERAVIEW_XYMASK			= (CAMERAVIEW_YMASK | CAMERAVIEW_XMASK | CAMERAVIEW_FIXEDPOSLEFT|CAMERAVIEW_FIXEDPOSRIGHT),
		CAMERAVIEW_ZMASK			= (CAMERAVIEW_ABOVE | CAMERAVIEW_BELOW | CAMERAVIEW_LEVELED),
		CAMERAVIEW_MASK				= (CAMERAVIEW_XYMASK | CAMERAVIEW_ZMASK),
		CAMERAVIEW_DEFAULTXY		= CAMERAVIEW_BEHIND,
		CAMERAVIEW_DEFAULTZ			= CAMERAVIEW_LEVELED,
		CAMERAVIEW_DEFAULT			= (CAMERAVIEW_DEFAULTXY | CAMERAVIEW_DEFAULTZ),

		// Camera mode
		ACS_CAMERAMODE_UNDEFINED		= 0,
		ACS_CAMERAMODE_BASEBIT			= 15,
		ACS_CAMERAMODE_CHARACTER		= (1 << (ACS_CAMERAMODE_BASEBIT + 0)) | CAMERAMODE_MODESET,
		ACS_CAMERAMODE_OBJECT			= (1 << (ACS_CAMERAMODE_BASEBIT + 1)) | CAMERAMODE_MODESET,
		ACS_CAMERAMODE_CONTROLLED		= (1 << (ACS_CAMERAMODE_BASEBIT + 2)) | CAMERAMODE_MODESET,
		ACS_CAMERAMODE_CONTROLLEDONCHAR	= (1 << (ACS_CAMERAMODE_BASEBIT + 3)) | CAMERAMODE_MODESET,
		ACS_CAMERAMODE_ENGINEPATH		= (1 << (ACS_CAMERAMODE_BASEBIT + 4)) | CAMERAMODE_MODESET,
		ACS_CAMERAMODE_FIXEDPOS			= (1 << (ACS_CAMERAMODE_BASEBIT + 5)) | CAMERAMODE_MODESET,
		ACS_CAMERAMODE_LEDGE			= (1 << (ACS_CAMERAMODE_BASEBIT + 6)) | CAMERAMODE_MODESET,
		ACS_CAMERAMODE_LADDER			= (1 << (ACS_CAMERAMODE_BASEBIT + 7)) | CAMERAMODE_MODESET,
		ACS_CAMERAMODE_COMBINED			= (ACS_CAMERAMODE_CHARACTER | ACS_CAMERAMODE_OBJECT),
		ACS_CAMERAMODE_DEFAULT			= ACS_CAMERAMODE_CHARACTER,
		ACS_CAMERAMODE_MASKCONTROLLED	= ((1 << (ACS_CAMERAMODE_BASEBIT + 2)) | (1 << (ACS_CAMERAMODE_BASEBIT + 3))| (1 << (ACS_CAMERAMODE_BASEBIT + 4))),
		ACS_CAMERAMODE_MASK				= (ACS_CAMERAMODE_CHARACTER | ACS_CAMERAMODE_OBJECT | ACS_CAMERAMODE_CONTROLLED | ACS_CAMERAMODE_CONTROLLEDONCHAR | ACS_CAMERAMODE_FIXEDPOS | ACS_CAMERAMODE_LEDGE | ACS_CAMERAMODE_LADDER),

		ACS_CAMERAMODE_INTERACTIVE		 = (1 << 29),	// Set while entering and being in 3PI-mode (third person interactive)
		ACS_CAMERAMODE_INTERACTIVE_LEAVE = (1 << 30),	// Set while leaving 3PI-mode
		ACS_CAMERAMODE_ACTIVE			 = (1 << 31),	// Set when the camera is active and valid...
	};

	struct ConfigBlock
	{
		int32 m_CameraMode;
		fp32 m_DistanceOffset;
		fp32 m_HeightOffset;
	};

protected:
	CWorld_Server* m_pWServer;

	CamVars	m_CamVars;

	static int ResolveCameraMode(CStr _Str);
	static int ResolveCameraView(CStr _Str);
	
	bool SetCameraMode(int _CameraMode);
	CStr GetCameraModeString(int _CameraMode);
	bool SetCameraView(int _CameraView);
	CStr GetCameraViewString(int _CameraView);

	int CombineViews(int _View);

	static fp32 GetHeightOffset(CamVars& _CamVars, CWObject_CoreData* _pObj);

	static CVec3Dfp32 GetCameraTargetPos(int _CameraMode, CamVars& _CamVars, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const fp32& _IPTime);
	static CVec3Dfp32 GetCameraViewPos(int _CameraView, CamVars& _CamVars, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, fp32& _Distance);
	static CVec3Dfp32 GetCameraViewPosClient(int _CameraView, CamVars& _CamVars, CWObject_CoreData* _pObj, 
		CWorld_PhysState* _pWPhysState, fp32& _Distance, const fp32& _IPTime);
	static CVec3Dfp32 GetCameraViewPosXY(int _CameraView, CamVars& _CamVars, CWorld_PhysState* _pWPhysState);
	static fp32 GetCameraViewPosZ(int _CameraView, CamVars& _CamVars, CWObject_CoreData* _pObj);

	CVec3Dfp32 GetCameraTargetPos(int _CameraMode) { return GetCameraTargetPos(_CameraMode, m_CamVars, m_pWServer->Object_Get(m_CamVars.m_iCharacter), m_pWServer,0.0f); }
	CVec3Dfp32 GetCameraViewPos(int _CameraView, fp32& _Distance) { return GetCameraViewPos(_CameraView, m_CamVars, m_pWServer->Object_Get(m_CamVars.m_iCharacter), m_pWServer, _Distance); }

	void DebugRenderBox(const CBox3Dfp32& _Box);
	
	static bool VerifyCameraMode(CamVars& _CamVars);
	static bool VerifyCameraMode(ConfigBlock& _ConfigBlock);
	static bool VerifyCameraMode(int32& _CameraMode, fp32& _CharacterHeightOffset, 
		fp32& _CharacterHeightOffsetCrouch, fp32& _DistanceOffset, fp32& _HeightOffset);

	// Put in number of positions in the array, number of positions written
	// will be put there when the operation is finished (for now 0-2)
	bool GetExcludeObject(int* _liExlude, int& _Number);

public:
	virtual void OnCreate();
	void CopyFrom(const CActionCutsceneCamera& _Camera);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	inline CVec3Dfp32 GetCameraTargetPos() { return GetCameraTargetPos(m_CamVars.m_CameraMode); }
	inline CVec3Dfp32 GetCameraViewPos(fp32& _Distance) { return GetCameraViewPos(m_CamVars.m_CameraMode, _Distance); }

	// Dynamic class stuff
	static spCActionCutsceneCamera CreateRuntimeClassObject(const char *_pName, CWorld_Server *_pWServer);
	static spCActionCutsceneCamera CreateObject(const char *_pName, CWorld_Server* _pWServer);
	
	// Debug draw camera
	void DebugDrawCamera();
	inline void SetServer(CWorld_Server* _pWServer) { m_pWServer = _pWServer; }
	bool ConfigureFromString(CStr _Str);
	static bool MakeConfigBlockFromString(ConfigBlock& _ConfigBlock, CStr _Str, CWorld_Server* _pWPhysState);
	static bool MakeConfigBlock(ConfigBlock& _ConfigBlock, const int32& _CameraMode, 
											const int32& _CameraTarget, 
											const fp32& _DistanceOffset = 0.0f,
											const fp32& _HeightOffset = 0.0f);

	bool ConfigureCamera(int32 _CameraMode, fp32 _DistanceOffset, fp32 _HeightOffset);
	bool ConfigureCamera(const ConfigBlock& _ConfigBlock);
	void MakeDefaultCamera();
	
	void OnRefresh();
	void OnClientRefresh(CamVars& _CamVars, CWorld_PhysState* _pWPhys, CWO_Character_ClientData* _pCD);

	void SetCharacterAndObject(int _iCharacter, int _iObject);

	bool CycleCameraMode();
	bool CycleCameraXYView();
	bool CycleCameraZView();
	
	void GetCameraPosition(CMat4Dfp32& _CamMat, CWObject_CoreData* _pObj);
	static void GetCameraPositionClient(CMat4Dfp32& _CamMat, CamVars& _CamVars, CWObject_CoreData* _pObj, 
		CWorld_PhysState* _pWPhysState, fp32 _IPTime);
	
	bool CameraTrace(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, 
					CMat4Dfp32& _CameraMatrix, CVec3Dfp32 Target,	fp32& _WantedDistance, 
					bool _Rotate, int* _liExclude, int _NumberExclude, bool bDebugRender = false);

	MACRO_INLINEACCESS_RWEXT(Character, m_CamVars.m_iCharacter, uint16)
	MACRO_INLINEACCESS_RWEXT(Object, m_CamVars.m_iObject, uint16)
	MACRO_INLINEACCESS_RWEXT(CharacterHeightOffset, m_CamVars.m_CharacterHeightOffset, fp32)
	MACRO_INLINEACCESS_RWEXT(CharacterHeightOffsetCrouch, m_CamVars.m_CharacterHeightOffsetCrouch, fp32)
	MACRO_INLINEACCESS_RWEXT(HeightOffset, m_CamVars.m_HeightOffset, fp32)
	MACRO_INLINEACCESS_RWEXT(DistanceOffset, m_CamVars.m_DistanceOffset, fp32)
	MACRO_INLINEACCESS_RW(CamVars, CamVars)

	void ToggleValid();
	void SetActive();
	void SetValid(CWO_Character_ClientData* _pCD);
	void SetInValid(CWO_Character_ClientData* _pCD);
	static void SetInvalidToPcd(CWO_Character_ClientData* _pCD);
	bool Valid() { return (m_CamVars.m_CameraMode & ACS_CAMERAMODE_ACTIVE) != 0;}

	void InteractiveMode_Start(CWO_Character_ClientData& _CD);
	void InteractiveMode_Stop(CWO_Character_ClientData& _CD);
};

#endif
