#ifndef __WOBJ_CONTROLCLASS_
#define __WOBJ_CONTROLCLASS_

#include "WAG_ClientData_Game2.h"

// should have controller class that instantiates new objects as needed (ex camera/control....)
// how to create... and get that info to client? (need to create on both client/server to be able
// to predict everything
class CControlHandler;
class CWObject_Character2;
class CWObject_ControlClass;
class CWObject_ControlClassPacked
{
public:
	enum
	{
		CONTROLCLASS_PACKED_ADDED	= 1 << 0,
		CONTROLCLASS_PACKED_REMOVED	= 1 << 1,
		CONTROLCLASS_PACKED_UPDATED	= 1 << 2,
	};
protected:
	uint8 m_Flags;
	int8 m_ControlType;
	CMTime m_ActivateTime;
	// Probably need to know controltype and entrytime to identify correct objects
	TThinArray<uint8>	m_lData;
public:
	void Pack(const CWObject_ControlClass& _Control);
	static void Pack(const CWObject_ControlClass& _Control,uint8*& _pBuffer);
	
	static void MakeDiff(const CWObject_ControlClassPacked& _Ref, const CWObject_ControlClass& _Control,uint8*& _pBuffer);
	void UpdateWithDiff(CWObject_ControlClass& _Control,const uint8*& _pBuffer);
	int8 GetControlTypeFromBuffer(const uint8* _pBuffer);
	MACRO_INLINEACCESS_R(ControlType,int8);
	MACRO_INLINEACCESS_R(ActivateTime,CMTime);
	MACRO_INLINEACCESS_RW(Flags,uint8);
};

class CWObject_ControlClass : public CReferenceCount
{
protected:
	friend class CControlReg;
	enum
	{
		CONTROLCLASS_PACKFLAGS_CONTROLTYPE		= 1 << 0,
		CONTROLCLASS_PACKFLAGS_ACTIVATETIME		= 1 << 1,
	};
	int8 m_ControlType;
	CMTime m_ActivateTime;
public:
	CWObject_ControlClass();

	// Updates every tick on server/(predicted)client
	virtual int32 Update(CWObject_CoreData* _pChar, CWorld_PhysState* _pWPhys, CWObject_ControlClass* _pPrev) pure;
	virtual void Clear();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey) { return; }

	virtual int32 DoCamera(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMat43fp32& Camera){ return 0;};
	virtual int32 DoPhysics(int _iSel, CWObject_CoreData* _pObj, CWObject_Character2* _pChar, CWorld_PhysState* _pPhysState, 
		const CVec3Dfp32& _Move, const CVec3Dfp32& _Look, uint32 _Press, int& _Released, 
		const CXR_MediumDesc& _MediumDesc, int16& _Flags, CVec3Dfp32& _VRet) { return 0;	}
	// Client refresh..?
	virtual void OnRefresh_Client(CWObject_Client* _pObj, CWorld_Client* _pWClient) { return; }


	virtual void SCopyFrom(const void* _pFrom);
	// Pack data that needs to come across
	virtual void SPack(uint8 *&_pD) const;

	// So diff, check with packed data, and diff it

	// Check which data that is not in current set and diff it (add diff data...)
	virtual void MakeDiff(const uint8 *&_pDRef, uint8 *&_pD) const;

	virtual void SUnpack(const uint8 *&_pD);
	int32 GetControlType() { return m_ControlType; }


	virtual void OnCreateClientUpdate();
	virtual void OnClientUpdate();
	MACRO_INLINEACCESS_R(ControlType,int8);
	MACRO_INLINEACCESS_R(ActivateTime,CMTime);

#ifndef M_RTM
	virtual void DebugPrint(CStr& _Str);
#endif
};
typedef TPtr<CWObject_ControlClass> spCWObject_ControlClass;

// Test class for autovared camera....
#define CWObject_ControlCameraParent CWObject_ControlClass
class CWObject_ControlCamera : public CWObject_ControlCameraParent
{
protected:
	enum
	{
		CONTROLCAMERA_PACKFLAGS_STANDHEADOFFSET		= 1 << 0,
		CONTROLCAMERA_PACKFLAGS_CROUCHHEADOFFSET	= 1 << 1,
		CONTROLCAMERA_PACKFLAGS_BEHINDOFFSET		= 1 << 2,
		CONTROLCAMERA_PACKFLAGS_SHOULDEROFFSET		= 1 << 3,
		CONTROLCAMERA_PACKFLAGS_AMPLITUDE			= 1 << 4,
		CONTROLCAMERA_PACKFLAGS_SPEED				= 1 << 5,
		CONTROLCAMERA_PACKFLAGS_DURATIONTICKS		= 1 << 6,
		CONTROLCAMERA_PACKFLAGS_STARTTICK			= 1 << 7,
	};
	// No need to replicate
	CVec3Dfp32	m_Camera_CurHeadOffset; // Current head offset.
	CVec3Dfp32	m_Camera_CurHeadOffsetChange; // Current change in head offset.
	CVec3Dfp32	m_Camera_LastHeadOffset; // Behind offset of previous tick, used for inter frame interpolation.
	CVec3Dfp32	m_Camera_CurBehindOffset; // Current behind head offset (Behind + Special).
	CVec3Dfp32	m_Camera_CurBehindOffsetChange; // Current change in behind offset.
	CVec3Dfp32	m_Camera_LastBehindOffset; // Behind offset of previous tick, used for inter frame interpolation.
	// Chasecamera distance settings
	fp32			m_ChaseCamDistance;
	fp32			m_dChaseCamDistance;
	fp32			m_LastChaseCamDistance;

	// Camera shake settings (needed ???)
	CVec3Dfp32	m_CameraShake_LastMomentum;
	CVec3Dfp32	m_CameraShake_CurMomentum;
	fp32			m_CameraShake_CurBlend;
	fp32			m_CameraShake_LastBlend;
	int			m_CameraShake_UpdateDelayTicks;
	int			m_CameraShake_Randseed;
	uint8		m_CameraShake_Flags;


	// TraceCamera parameters
	fp32 m_fTraceHistory;			// Stores traced camera distance
	fp32 m_fOldTraceHistory;			// Stores the previously traced camera distance
	int m_iRotationIndex;			// Keeps track of the rotations in TraceCamera

	// Replicate
	CVec3Dfp32	m_Camera_StandHeadOffset;
	CVec3Dfp32	m_Camera_CrouchHeadOffset;
	CVec3Dfp32	m_Camera_BehindOffset;
	CVec3Dfp32	m_Camera_ShoulderOffset;

	fp32 m_CameraShake_Amplitude;
	fp32 m_CameraShake_Speed;
	uint32 m_CameraShake_DurationTicks;
	uint32 m_CameraShake_StartTick;
public:
	CWObject_ControlCamera();

	virtual int32 Update(CWObject_CoreData* _pChar, CWorld_PhysState* _pWPhys, CWObject_ControlClass* _pPrev);
	virtual void Clear();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	virtual int32 DoCamera(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMat43fp32& Camera);

	void Camera_Get(CWorld_PhysState* _pWPhysState, CMat43fp32& _Camera, CWObject_CoreData* _pCamObj, fp32 _IPTime);
	int OnGetCamera(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMat43fp32& _Camera);
	void Camera_Offset(CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, CMat43fp32& _Camera, fp32 _IPTime);
	void Camera_ShakeItBaby(CWorld_PhysState* _pWPhysState, CMat43fp32& _Camera, CWO_Character_ClientData2* _pCD, fp32 _IPTime);
	bool Camera_Trace(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, bool _bRangedWeapon, CMat43fp32& _CameraMatrix, CVec3Dfp32& _WantedBehindOffset, 
		fp32& _WantedDistance, fp32& _MaxDistance, bool _Rotate = true);

	virtual void SCopyFrom(const void* _pFrom);
	virtual void SPack(uint8 *&_pD) const;
	virtual void MakeDiff(const uint8 *&_pDRef, uint8 *&_pD) const;
	virtual void SUnpack(const uint8 *&_pD);

	static spCWObject_ControlClass CreateMe();

	#ifndef M_RTM
	void DebugPrint(CStr& _Str);
#endif
};

#define CWObject_ControlPhysFreeParent CWObject_ControlClass
class CWObject_ControlPhysFree : public CWObject_ControlPhysFreeParent
{
protected:
	enum
	{
		CONTROLPHYSFREE_PACKFLAGS_SPEED_FORWARD		= 1 << 0,
		CONTROLPHYSFREE_PACKFLAGS_SPEED_SIDESTEP	= 1 << 1,
		CONTROLPHYSFREE_PACKFLAGS_SPEED_UP			= 1 << 2,
		CONTROLPHYSFREE_PACKFLAGS_SPEED_JUMP		= 1 << 3,
	};

	// Replicate (these are really only set once.... (special initial pack?))
	fp32 m_Speed_Forward;
	fp32 m_Speed_SideStep;
	fp32 m_Speed_Up;
	fp32 m_Speed_Jump;
public:
	CWObject_ControlPhysFree();

	virtual int32 Update(CWObject_CoreData* _pChar, CWorld_PhysState* _pWPhys, CWObject_ControlClass* _pPrev);
	// Clear might not have to be virtual..
	virtual void Clear();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	virtual int32 DoPhysics(int _iSel, CWObject_CoreData* _pObj, CWObject_Character2* _pChar, CWorld_PhysState* _pPhysState, 
		const CVec3Dfp32& _Move, const CVec3Dfp32& _Look, uint32 _Press, int& _Released, 
		const CXR_MediumDesc& _MediumDesc, int16& _Flags, CVec3Dfp32& _VRet);

	static spCWObject_ControlClass CreateMe();

	virtual void SCopyFrom(const void* _pFrom);
	virtual void SPack(uint8 *&_pD) const;
	virtual void MakeDiff(const uint8 *&_pDRef, uint8 *&_pD) const;
	virtual void SUnpack(const uint8 *&_pD);
};

// Some sort of registry with the available types (auto reg)
// create from types (easily replicated)
#define CCONTROLREG_NAME_MAXLEN 24
class CControlReg
{
public:
	enum
	{
		CONTROLTYPE_UNKNOWN = -1,
	};
protected:
	struct RegEntry
	{
		spCWObject_ControlClass (*m_pfnCreate)();
		char m_Name[CCONTROLREG_NAME_MAXLEN+1];
		int8 m_Type;
	};
	static int8 m_sType;
	//static TArray<RegEntry> m_slControlTypes;
	static RegEntry m_slControlTypes[10];
public:
	static void AddControlType(const char* _pName, spCWObject_ControlClass (*_pfnCreate)());
	static spCWObject_ControlClass CreateControlClass(int8 _Type);
	static spCWObject_ControlClass CreateControlClass(char* _pName);

	static int8 GetControlType(char* _pName);
	static void GetControlType(int8 _Type, CStr& _Name);
};

class CControlRegInit
{
public:
	CControlRegInit(const char* _pName, spCWObject_ControlClass (*_pfnCreate)())
	{
		CControlReg::AddControlType(_pName,_pfnCreate);
	}
};

#define MACRO_IMPLEMENT_CONTROLTYPE(Name,CreateFunction) \
	CControlRegInit g_ControlRegInit##Name(#Name,CreateFunction);

class CControlHandlerPacked
{
protected:
	friend class CControlHandler;
	TThinArray<CWObject_ControlClassPacked>	m_lControlPacked;
public:
	int32 CreateDiff(const CControlHandler& _Handler, uint8*& _pBuffer) const;
	void UpdateWithDiff(CControlHandler& _Handler, const uint8*& _pBuffer);
	void CopyFrom(const CControlHandlerPacked& _Ref);
	void Pack(const CControlHandler& _Handler);
};

class CControlHandler
{
public:
	CControlHandlerPacked m_ClientMirror;
	enum
	{
		CONTROLHANDLER_TYPE_UNKNOWN = -1,
		CONTROLHANDLER_TYPE_CAMERA = 0,
		CONTROLHANDLER_TYPE_PHYSICS,
		CONTROLHANDLER_TYPE_SKELETON,
		//CONTROLHANDLER_TYPE_MESSAGES,
		CONTROLHANDLER_NUMTYPES,
	};

	TArray<spCWObject_ControlClass> m_lControls;

	void AddControl(int8 _ControlType, const CRegistry* _pKey);
	void AddControl(char* _pName, const CRegistry* _pKey);

	// Information over: type (creation/stuff)(id from controlreg) package,..

	int32 CreateDiff(CControlHandlerPacked& _Packed);

	// Pack stuff on server
	int32 OnCreateClientUpdate(uint8*& _pBuffer);
	// Unpack stuff on client
	void OnClientUpdate(const uint8*& _pBuffer);
	int OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	void OnRefresh_Client(CWObject_Client* _pObj, CWorld_Client* _pWClient);

	void Update(CWObject_CoreData* _pChar, CWorld_PhysState* _pWPhys);
	void Clear();
	void CopyFrom(const CControlHandler& _Handler);

	int32 DoCamera(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMat43fp32& Camera);
	int32 DoPhysics(int _iSel, CWObject_CoreData* _pObj, CWObject_Character2* _pChar, CWorld_PhysState* _pPhysState, 
		const CVec3Dfp32& _Move, const CVec3Dfp32& _Look, uint32 _Press, int& _Released, 
		const CXR_MediumDesc& _MediumDesc, int16& _Flags, CVec3Dfp32& _VRet);
	void DoSkeleton();

	void OnRefresh_Client();

#ifndef M_RTM
	void DebugPrint(CStr& _Str);
#endif
};


#define PACKMAX 500.0f
// Have to find better way of doing this I guess...
#define MACRO_DIFF_VEC3(Flag,Var) \
	if (PackedFlagRef & ##Flag) \
	{ \
		PTR_GETUINT32(_pDRef, Val); \
		int32 Packed = ##Var.Pack32(PACKMAX); \
		if (Val != Packed) \
		{ \
			PackedFlag |= ##Flag; \
			PTR_PUTUINT32(_pD,Packed); \
		} \
	} \
	else if (##Var != CVec3Dfp32(0.0f)) \
	{ \
		PackedFlag |= ##Flag; \
		Val = ##Var.Pack32(PACKMAX); \
		PTR_PUTUINT32(_pD,Val); \
	}

#define MACRO_DIFF_FP32(Flag,Var) \
	if (PackedFlagRef & ##Flag) \
	{ \
		fp32 Temp; \
		PTR_GETFP32(_pDRef, Temp); \
		if (Temp != ##Var) \
		{ \
			PackedFlag |= ##Flag; \
			PTR_PUTFP32(_pD,##Var); \
		} \
	} \
	else if (##Var != 0.0f) \
	{ \
		PackedFlag |= ##Flag; \
		PTR_PUTFP32(_pD,##Var); \
	}

#define MACRO_DIFF_INT8(Flag,Var) \
	if (PackedFlagRef & ##Flag) \
	{ \
		int8 Temp; \
		PTR_GETINT8(_pDRef, Temp); \
		if (Temp != ##Var) \
		{ \
			PackedFlag |= ##Flag; \
			PTR_PUTINT8(_pD,##Var); \
		} \
	} \
	else if (##Var != 0.0f) \
	{ \
		PackedFlag |= ##Flag; \
		PTR_PUTINT8(_pD,##Var); \
	}

#define MACRO_DIFF_UINT32(Flag,Var) \
	if (PackedFlagRef & ##Flag) \
	{ \
		int8 Temp; \
		PTR_GETUINT32(_pDRef, Temp); \
		if (Temp != ##Var) \
		{ \
			PackedFlag |= ##Flag; \
			PTR_PUTUINT32(_pD,##Var); \
		} \
	} \
	else if (##Var != 0.0f) \
	{ \
		PackedFlag |= ##Flag; \
		PTR_PUTUINT32(_pD,##Var); \
	}

#define MACRO_DIFF_CMTIME(Flag,Var) \
	if (PackedFlagRef & ##Flag) \
	{ \
		CMTime Temp; \
		PTR_GETCMTIME(_pDRef, Temp); \
		if (Temp.Compare(##Var) == 0) \
		{ \
			PackedFlag |= ##Flag; \
			PTR_PUTCMTIME(_pD,##Var); \
		} \
	} \
	else if (##Var.Compare(0.0f) != 0) \
	{ \
		PackedFlag |= ##Flag; \
		PTR_PUTCMTIME(_pD,##Var); \
	}

#define MACRO_PACK_VEC3(Flag,Var) \
	if (##Var != CVec3Dfp32(0.0f)) \
	{ \
		PackedFlag |= ##Flag; \
		Val = ##Var.Pack32(PACKMAX); \
		PTR_PUTUINT32(_pD,Val); \
	}

#define MACRO_UNPACK_VEC3(Flag,Var) \
	if (PackedFlag & ##Flag) \
	{ \
		PTR_GETUINT32(_pD, Val); \
		##Var.Unpack32(Val,PACKMAX); \
	}

#define MACRO_PACK_FP32(Flag,Var) \
	if (##Var != 0.0f) \
	{ \
		PackedFlag |= ##Flag; \
		PTR_PUTFP32(_pD,##Var); \
	}

#define MACRO_UNPACK_FP32(Flag,Var) \
	if (PackedFlag & ##Flag) \
	{ \
		PTR_GETFP32(_pD, ##Var); \
	}

#define MACRO_PACK_UINT8(Flag,Var) \
	if (##Var != 0.0f) \
	{ \
		PackedFlag |= ##Flag; \
		PTR_PUTUINT8(_pD,##Var); \
	}

#define MACRO_UNPACK_UINT8(Flag,Var) \
	if (PackedFlag & ##Flag) \
	{ \
		PTR_GETUINT8(_pD, ##Var); \
	}

#define MACRO_PACK_UINT32(Flag,Var) \
	if (##Var != 0.0f) \
	{ \
		PackedFlag |= ##Flag; \
		PTR_PUTUINT32(_pD,##Var); \
	}

#define MACRO_UNPACK_UINT32(Flag,Var) \
	if (PackedFlag & ##Flag) \
	{ \
		PTR_GETUINT32(_pD, ##Var); \
	}

#define MACRO_PACK_INT8(Flag,Var) \
	if (##Var != 0.0f) \
	{ \
		PackedFlag |= ##Flag; \
		PTR_PUTINT8(_pD,##Var); \
	}

#define MACRO_UNPACK_INT8(Flag,Var) \
	if (PackedFlag & ##Flag) \
	{ \
		PTR_GETINT8(_pD, ##Var); \
	}

#define MACRO_PACK_CMTIME(Flag,Var) \
	if (##Var.Compare(0.0f) != 0) \
	{ \
		PackedFlag |= ##Flag; \
		PTR_PUTCMTIME(_pD,##Var); \
	}

#define MACRO_UNPACK_CMTIME(Flag,Var) \
	if (PackedFlag & ##Flag) \
	{ \
		PTR_GETCMTIME(_pD, ##Var); \
	}
#endif