#ifndef __WOBJ_CAMERAUTIL_H
#define __WOBJ_CAMERAUTIL_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_CameraUtil

Author:			Olle Rosenquist

Copyright:		Copyright O3 Games AB 2005

Contents:		

Comments:		Camera for actioncutscenes, dialogs, ...

History:		
  050404:		Created File
\*____________________________________________________________________________________________*/
 
enum
{
	// camera messages
	OBJMSG_CAMERA_,
};

enum
{
	CAMERAUTIL_MODE_ORIGIN_ENGINEPATH					= 1 << 0,
	// Get camera position from someone else
	CAMERAUTIL_MODE_ORIGIN_CHARACTER					= 1 << 1,

	CAMERAUTIL_MODE_MUSTBEMOUNTED						= 1 << 2,
	

	CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_LEFT				= 1 << 8,
	CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_RIGHT				= 1 << 9,
	CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_ABOVE				= 1 << 10,
	CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_BELOW				= 1 << 11,
	CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_FRONT				= 1 << 12,
	CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_BEHIND				= 1 << 13,


	CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_MASK				= (CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_LEFT | CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_RIGHT | CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_ABOVE | CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_BELOW),

	// Fixed origin position (object), usually with dir info
	CAMERAUTIL_MODE_ORIGIN_FIXEDPOSITION				= 1 << 14,


	CAMERAUTIL_MODE_TARGET_ENGINEPATH					= 1 << 15,
	CAMERAUTIL_MODE_TARGET_DIALOG_SPEAKER				= 1 << 16,

	// Camera aim at center/top/bottom of object
	CAMERAUTIL_MODE_TARGET_OBJECTCENTER					= 1 << 17,
	CAMERAUTIL_MODE_TARGET_OBJECTTOP					= 1 << 18,
	CAMERAUTIL_MODE_TARGET_OBJECTBOTTOM					= 1 << 19,

	// Fixed position (object), usually not used
	CAMERAUTIL_MODE_TARGET_FIXEDPOSITION				= 1 << 20,
	
	CAMERAUTIL_MODE_MOUNTEDCAM_FREE						= 1 << 21,	// Can look freely within the threshold
	CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOW					= 1 << 22,	// Follows animation position, can look around a little but will blend back to anim position
	CAMERAUTIL_MODE_MOUNTEDCAM_FOLLOWFREE				= 1 << 23,	// Follows animation position, can look around based on animation

	CAMERAUTIL_MODE_MOUNTEDCAM							= 1 << 27,
	CAMERAUTIL_MODE_DIALOGMATRICES_SET					= 1 << 28,
	CAMERAUTIL_MODE_USERINPUT							= 1 << 29,
	CAMERAUTIL_MODE_ACS									= 1 << 30,
	CAMERAUTIL_MODE_DIALOG								= 1 << 31,

	CAMERAUTIL_TYPEMASK									= (CAMERAUTIL_MODE_ACS | CAMERAUTIL_MODE_DIALOG | CAMERAUTIL_MODE_MOUNTEDCAM),

	CAMERAUTIL_NEWTYPE_KEEPMASK							= 0,
	CAMERAUTIL_ACTIVEMASK								= (CAMERAUTIL_MODE_ACS | CAMERAUTIL_MODE_DIALOG | CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_MASK | CAMERAUTIL_MODE_MOUNTEDCAM),


	CAMERAUTIL_DIALOGTYPE_OVERSHOULDER_LEFT = 1,
	CAMERAUTIL_DIALOGTYPE_OVERSHOULDER_RIGHT,
	CAMERAUTIL_DIALOGTYPE_NEARSHOULDER_LEFT,
	CAMERAUTIL_DIALOGTYPE_NEARSHOULDER_RIGHT,
	CAMERAUTIL_DIALOGTYPE_TWOSHOT_LEFT,
	CAMERAUTIL_DIALOGTYPE_TWOSHOT_RIGHT,
	CAMERAUTIL_DIALOGTYPE_CLOSEBB_LEFT,
	CAMERAUTIL_DIALOGTYPE_CLOSEBB_RIGHT,

	CAMERAUTIL_DIALOGTYPE_NUMTYPES = CAMERAUTIL_DIALOGTYPE_CLOSEBB_RIGHT,





	CAMERAUTIL_ACS_AUTOCONFIG1 = (CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_LEFT | CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_ABOVE|CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_BEHIND),
	CAMERAUTIL_ACS_AUTOCONFIG2 = (CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_RIGHT | CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_ABOVE|CAMERAUTIL_MODE_ORIGIN_AUTOMATIC_BEHIND),
};
class CCameraUtil
{
protected:

	CMat4Dfp32 m_MatListener;
	CMat4Dfp32 m_MatSpeaker;
	CVec3Dfp32 m_HeadOffsetListener;
	CVec3Dfp32 m_HeadOffsetSpeaker;
	CVec3Dfp32 m_PosSpeakerLookat;
	CVec3Dfp32 m_PosListenerLookat;

	CMTime m_CameraStartTime;
	CDialogueToken* m_pDialogToken;
	int32 m_Mode;
	int16 m_iOriginObj;
	int16 m_iTargetObj;
	// Extra object index, might be actioncutscene for instance
	int16 m_iExtraObj;
	fp32	m_FOV;
	fp32 m_FovCoeff;

	// Distance info (Needed?)
	fp32 m_CameraDistance;

	// Not copied for now, see how it goes...
	uint32 m_MountedLookBlendStart;
	int32 m_MountedLookBlendDuration;
	int32 m_LastLookTick;
	fp32 m_MountedLookThreshold;		// How much we can look
	fp32 m_PrevMountedLookZ;			// Look sideways
	fp32 m_PrevMountedLookY;			// Look up/down
	fp32 m_MountedLookZ;				// Look sideways
	fp32 m_MountedLookY;				// Look up/down
	// Copied
	uint8 m_MinMountedMaxLookAngleZ;	//... 0 - 180
	uint8 m_MinMountedMaxLookAngleY;	//... 0 - 90
	uint8 m_MaxMountedMaxLookAngleZ;	//... 0 - 180
	uint8 m_MaxMountedMaxLookAngleY;	//... 0 - 90
	uint8 m_DecreaseRate;				//... Rate of decrease

	// (not copied over net... (yet...))
	fp32 m_AngleOffsetX;
	fp32 m_LastAngleOffsetX;
	fp32 m_TargetAngleOffsetX;
	fp32 m_AngleOffsetXChange;

	fp32 m_AngleOffsetY;
	fp32 m_LastAngleOffsetY;
	fp32 m_TargetAngleOffsetY;
	fp32 m_AngleOffsetYChange;

	int8 m_Type;
	bool m_Mode_Flipped;

	void BindAngleZ(fp32& _Angle, fp32 _Reference, fp32 _MaxLook);
	void BindAngleY(fp32& _Angle, fp32 _Reference, fp32 _MaxLook);
	void BlendAngle(fp32& _Angle, fp32& _PrevAngle, fp32 _Reference, fp32 _MaxLook, fp32 _TimeLeft);


	bool GetCameraTargetPos(class CWorld_PhysState* _pWPhys, CMTime _Time, CVec3Dfp32& _TargetPos);
	bool GetCameraViewPos(class CWorld_PhysState* _pWPhys, CMTime _Time, CVec3Dfp32& _ViewPos);
	bool ValidCamera(const CVec3Dfp32& _ViewPos, const CVec3Dfp32& _CameraTarget);

	aint GetEnginePathMatrix(CWorld_PhysState* _pWPhys, CMTime& _Time, int16 _iEnginePath, CMat4Dfp32* _EPMat);

	bool GetCameraACS(CWorld_PhysState* _pWPhys, CMat4Dfp32& _CamMat, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD);
	bool GetMountedCamera(CWorld_PhysState* _pWPhys, CMat4Dfp32& _CamMat, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, fp32 _IPTime);

	void AddUserOffset(CMat4Dfp32& _CamMat, fp32 _IPTime);
	

	// Dialog camera options
	bool GetCameraDialog(CWorld_PhysState* _pWPhys, CMat4Dfp32& _CamMat, CMTime _Time);
	bool CheckDialogCamera(CWorld_PhysState* _pWPhys, CMat4Dfp32& _NewCamera, fp32 _FOV);
	void Dialog_GetInterpolatedPos(CVec3Dfp32& _Pos, fp32 _I);
	void GetCameraPosition_OverShoulder(CWorld_PhysState* _pWPhys, const CMTime& Time, CVec3Dfp32& _Camera);
	void GetCameraPosition_NearOverShoulder(CWorld_PhysState* _pWPhys, const CMTime& Time, CVec3Dfp32& _Camera);
	void GetCameraPosition_TwoShot(CWorld_PhysState* _pWPhys, const CMTime& Time, CVec3Dfp32& _Camera);
	void GetCameraPosition_Lookat(CWorld_PhysState* _pWPhys, const CMTime& Time, CVec3Dfp32& _Camera);
	void GetCameraPosition_SpeakerHead(CWorld_PhysState* _pWPhys, const CMTime& Time, CVec3Dfp32& _Camera);
	void GetCameraPosition_SafeBoundingBox(CWorld_PhysState* _pWPhys, const CMTime& Time, CVec3Dfp32& _Camera, const CVec3Dfp32& _LookAtPos);

	bool DialogIsFlipped();
public:
	void Clear();
	void Refresh(CWorld_PhysState* _pWPhysState, CWO_Character_ClientData* _pCD);

	CVec3Dfp32 GetMountedAngles(fp32 _IPTime);
	void SetACSCamera(int32 _Mode, int16 _iACS, int16 _iEnginePath, int16 _iTarget, CMTime _StartTime);
	//void SetDialogCamera(int32 _Mode, int16 _iListener, int16 _iSpeaker);
	bool SetDialogCamera(CWorld_PhysState* _pWPhys, CDialogueToken* _pDialogToken, int8 _PreferredType, const CMat4Dfp32 &_MatSpeaker,
		const CMat4Dfp32 &_MatListener, fp32 _SpeakerHeadOffset, fp32 _ListenerHeadOffset,
		int16 _SpeakerID, int16 _ListenerID, const CMTime& _StartTime);
	void SetAutomaticCamera(CWorld_PhysState* _pWPhys, int16 _iObject, int16 _iChar, bool _bOnChar = false);

	// Get a cutscene camera, attached to a character for instance (only one supported atm)
	bool SetMountedCamera(CWorld_PhysState* _pWPhys, CWO_Character_ClientData* _pCDChar, int16 _iTargetObj, int8 _CSCMode, int _MinLookFreedomZ, int _MinLookFreedomY,int _MaxLookFreedomZ, int _MaxLookFreedomY, fp32 _LookBlend, uint8 _DecreaseRate = 10);
	void UpdateMountedLook(CWorld_PhysState* _pWPhys, CWO_Character_ClientData* _pPlayerCD, const CVec3Dfp32& _dLook);
	bool GetCamera(CWorld_PhysState* _pWPhys, CMat4Dfp32& _CamMat, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, fp32 _IPTime);
	int16 GetMountedCameraObject() { return (IsActive(CAMERAUTIL_MODE_MOUNTEDCAM) ?  m_iOriginObj: 0); }

	fp32 GetFOV();

	int32 GetExtraObj() { return m_iExtraObj; }
	void UpdateOffsetVars();
	void SetCamOffsets(fp32 _OffsetX, fp32 _OffsetY);
	M_INLINE bool IsActive() { return (m_Mode & CAMERAUTIL_ACTIVEMASK) != 0; }
	M_INLINE bool IsActive(int32 _Mode) { return ((m_Mode & CAMERAUTIL_ACTIVEMASK) & _Mode) != 0; }

	void CopyFrom(const CCameraUtil& _From);
	void Pack(uint8 *&_pD, CMapData* _pMapData) const;
	void Unpack(const uint8 *&_pD, CMapData* _pMapData);
	void OnDeltaSave(CCFile* _pFile) const;
	void OnDeltaLoad(CCFile* _pFile);
};
#endif
