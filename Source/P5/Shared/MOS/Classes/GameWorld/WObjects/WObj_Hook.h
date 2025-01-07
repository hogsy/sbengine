#ifndef __WOBJ_HOOK_H
#define __WOBJ_HOOK_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Engine/Hook system

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWObject_Attach
					CWObject_Hook
					CWObject_Engine_Rotate
					CWObject_Engine_Wheel
					CWObject_Hook_NoRotate
					CWObject_Hook_To_Target
					CWObject_Hook_To_Line
					CWObject_Hook_To_Circle
					CWObject_Engine_Path
					CWObject_Dynamic
\*____________________________________________________________________________________________*/

#include "WObj_AutoVar.h"
#include "WObj_SimpleMessage.h"
#include "WObj_System.h"
#include "WObj_PosHistory.h"

#define EXTRACLIENTMESSAGE(ThisClass, BaseClass, AttachDataClass)

#define HOOK_DECLARE_WOBJECT(ThisClass, BaseClass, AttachDataClass)\
protected:\
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)\
	{\
		switch( _Msg.m_Msg )\
		{\
			EXTRACLIENTMESSAGE(ThisClass, BaseClass, AttachDataClass)\
			case OBJMSG_HOOK_GETRENDERMATRIX:\
			{\
				CMTime *pTime = (CMTime *)_Msg.m_Param0;\
				CMat4Dfp32 *pMat = (CMat4Dfp32 *)_Msg.m_Param1;\
				*pMat = ((ThisClass *)_pObj)->GetRenderMatrix(_pWClient, *pTime, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());\
				return 1;\
			}\
			case OBJMSG_HOOK_GETCURRENTMATRIX:\
			{\
				CMTime Time = GetTime(_pObj, _pWClient, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());\
				CMat4Dfp32* pMat = (CMat4Dfp32*)_Msg.m_Param0;\
				*pMat = ((ThisClass *)_pObj)->GetRenderMatrix(_pWClient, Time, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());\
				return 1;\
			}\
			case OBJMSG_HOOK_GETORGMATRIX:\
			{\
				CMat4Dfp32 *pMat = (CMat4Dfp32 *)_Msg.m_Param0;\
				*pMat = ((ThisClass *)_pObj)->GetOrgPositionMatrix();\
				return 1;\
			}\
			case OBJMSG_HOOK_ALLOCATEATTACHDATA:\
			{\
			return aint(MNew(AttachDataClass));\
			}\
			default:\
				return BaseClass::OnClientMessage(_pObj, _pWClient, _Msg);\
			}\
		}\
\
	virtual void ServerMsg_GetRenderMatrix(CMTime _Time, CMat4Dfp32 &_Mat)\
	{\
		_Mat = GetRenderMatrix(m_pWServer, _Time, m_pWServer->GetGameTick(), 0);\
	}
//			ConOutL(CStrF("Object: %i, LocalTime %f, JumpTime %f", _pObj->m_iObject, Time, JumpTime));


/////////////////////////////////////////////////////////////////////////////
// CWObject_Attach
class CWObject_Attach : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	HOOK_DECLARE_WOBJECT(CWObject_Attach, CWObject_Model, CAttachClientData);

public:
	enum
	{
		CLIENTFLAGS_RUN =				M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 0),
		CLIENTFLAGS_PHYSICS = 			M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 1),
		CLIENTFLAGS_HELPAXIS = 			M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 2),
		CLIENTFLAGS_LOOP = 				M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 3),
		CLIENTFLAGS_REVERSE = 			M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 4),
		CLIENTFLAGS_SEMIPHYSICS = 		M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 5),
		CLIENTFLAGS_UNRELIABLE = 		M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 6),
		CLIENTFLAGS_INVISWHENSTOPPED =	M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 7),
		CLIENTFLAGS_PROPELLED = 		M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 8),
		CLIENTFLAGS_DEACTIVATED = 		M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 9),
		CLIENTFLAGS_LIGHTINGSHIFT =	(CWO_CLIENTFLAGS_USERSHIFT + 10),
		CLIENTFLAGS_LIGHTING_BIT0 =		M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 10),
		CLIENTFLAGS_LIGHTING_BIT1 = 	M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 11),
		CLIENTFLAGS_USEHINT = 			M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 12),
		CLIENTFLAGS_USEHINT2 = 			M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 13),
		CLIENTFLAGS_LOCKZ =				M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 14),
		CLIENTFLAGS_PHYSICS_DRIVEN =	M_Bit(CWO_CLIENTFLAGS_USERSHIFT + 15),
	};

	enum
	{
		FLAGS_GAMEPHYSICS		= M_Bit(0),
		FLAGS_AUTOSTART			= M_Bit(1),
		FLAGS_AUTORESET			= M_Bit(2),
		FLAGS_LOOP				= M_Bit(3),
		FLAGS_AUTOREVERSE		= M_Bit(4),
		FLAGS_PUSHING			= M_Bit(5),
		FLAGS_UNRELIABLE		= M_Bit(6),
		FLAGS_HANDDRIVEN		= M_Bit(7),
		FLAGS_NEVERINTERRUPT	= M_Bit(8),
		FLAGS_INVISWHENSTOPPED	= M_Bit(9),
		FLAGS_BLOCKNAVIGATION	= M_Bit(10),
		FLAGS_WAITSPAWN			= M_Bit(11),
		FLAGS_ONCE				= M_Bit(12),
		FLAGS_USEHINT			= M_Bit(13),
		FLAGS_PUSHSYNC			= M_Bit(14),
		FLAGS_USEHINT2			= M_Bit(15),
		FLAGS_LOCKZ				= M_Bit(16),
		FLAGS_PHYSICS_DRIVEN	= M_Bit(17),

		FLAGS_RESETFIX			= M_Bit(18),
		FLAGS_PENDINGSTOP		= M_Bit(19),
		FLAGS_PSEUDOSTOP		= M_Bit(20),

		FLAGS_WAITTYPE_IMPULSE		= 0 << 21,
		FLAGS_WAITTYPE_PARAM		= 1 << 21,
		FLAGS_WAITTYPE_PARAMFLAG	= 2 << 21,
		FLAGS_WAITTYPE_MASK			= 3 << 21,

		MAXATTACH = 4, 
	};

public:
	struct CInitData : public CReferenceCount
	{
		CStr m_AttachNode[MAXATTACH];
		bool m_bAttachNode[MAXATTACH];
		CVec3Dfp32 m_AttachOrigin[MAXATTACH];
		CStr m_JumpParam;
		CStr m_UserPortal;
	};

	TPtr<CInitData> m_spInitData;
	uint32 m_Flags;
	uint16 m_iActivator;
	int16 m_DestroyTime;
	int16 m_DestroyPhysicsTime;
	uint16 m_Damage;

	struct CTimedMessage : public CWO_SimpleMessage
	{
		fp32 m_Time;

		CTimedMessage()
		{
			m_Time = -1.0f;
		}

		void Parse(CStr _St, CWorld_Server *_pWServer)
		{
			m_Time = _St.GetStrSep(";").Val_fp64();
			
			CWO_SimpleMessage::Parse(_St, _pWServer);
		}
	};

	TArray<CTimedMessage> m_lTimedMessages;
	TArray<CStr> m_lTimedSounds;
	uint8 m_iCurTimedMessage;
	int8 m_Wait;

	struct CAttachClientData : public CReferenceCount, public CAutoVarContainer
	{
		CAttachClientData()
		{
			m_CachedTime.MakeInvalid();
		}

		CMTime m_CachedTime;
		CMat4Dfp32 m_CachedMat;

		AUTOVAR_SETCLASS(CAttachClientData, CAutoVarContainer);
		CAUTOVAR_OP(CAutoVar_CMat4Dfp32, m_OrgPos, DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_CMat4Dfp32, m_TransformMat, DIRTYMASK_0_4);
		CAUTOVAR_OP(CAutoVar_uint16, m_iController, DIRTYMASK_1_0); // Could be remade if we would have virtuals on Client

		AUTOVAR_PACK_BEGIN
		AUTOVAR_PACK_VAR(m_OrgPos)
		AUTOVAR_PACK_VAR(m_TransformMat)
		AUTOVAR_PACK_VAR(m_iController)
		AUTOVAR_PACK_END
	};

	virtual void OnCreate();

	virtual void Stop();
	virtual void Run(int _iType);
	virtual void Reset();
	virtual void Reverse();
	virtual void GotoEnd();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual void OnSpawnWorld2();
	virtual void OnTransform(const CMat4Dfp32 &_Mat);
	virtual void Spawn(int _iSender);

	virtual aint OnMessage(const CWObject_Message &_Msg);
	virtual void OnImpulse(int _iImpulse, int _iSender);

	virtual void InitAttachPointers();
	virtual void InitClientData();
	virtual CMTime GetUpdatedTime(CMTime *_Time = NULL);
	virtual void OnRefresh();

	virtual bool NeedRefresh();
	virtual void UpdateNoRefreshFlag();

	virtual void OnDeltaSave(CCFile *_pFile);
	virtual void OnDeltaLoad(CCFile *_pFile, int _Flags);
	
	bool Attach_SetPosition(const CMat4Dfp32 &_Mat, bool _bMoveTo);
	
	void AddTimedMessage(uint _iSlot, const CTimedMessage &_Msg);
	void InsertTimedMessage(const CTimedMessage &_Msg);
	void UpdateTimedMessages(const CMTime& _Time);

	virtual fp32 GetDuration() { return -1; }
	static CMTime GetTime(CWObject_CoreData *_pObj, CWorld_PhysState *_pWPhysState, int _GameTick, fp32 _TickFrac);

	void ParseAttach(CStr _Attach, int _iAttach);
	CInitData *GetInitData();
	
	static M_INLINE CAttachClientData *GetAttachClientData(CWObject_CoreData *_pObj);
	static M_INLINE const CAttachClientData *GetAttachClientData(const CWObject_CoreData *_pObj);
	int GetAttach(int _iSlot);

	CVec3Dfp32 GetOrgAttachPos(int _iSlot);
	CVec3Dfp32 GetOrgAttachPosRel(int _iSlot);
	CMat4Dfp32 GetOrgAttachMatrix(int _iSlot);
	CVec3Dfp32 GetAttachPos(CWorld_PhysState *_pWPhysState, int _iSlot, const CMTime& _Time, int32 _GameTick, fp32 _TickFraction);
	CVec3Dfp32 GetOrgPosition();
	const CMat4Dfp32 &GetOrgPositionMatrix();
	CMat4Dfp32 GetAttachMatrix(CWorld_PhysState *_pWPhysState, int _iSlot, const CMTime& _Time, int32 _GameTick, fp32 _TickFraction, bool _bForceTime = false);

	fp32 GetFloat(int _iSlot);
	void SetFloat(int _iSlot, fp32 _Value);

	static int OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo);

	int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const;
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);
	static void OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);

	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

	CMat4Dfp32 GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac);
};

M_INLINE CWObject_Attach::CAttachClientData* CWObject_Attach::GetAttachClientData(CWObject_CoreData *_pObj)
{
	MAUTOSTRIP(CWObject_Attach_GetAttachClientData, NULL);

	if(_pObj->m_lspClientObj[0] == NULL)
	{
		_pObj->m_lspClientObj[0] = (CAttachClientData *)_pObj->GetRTC()->m_pfnOnClientMessage(NULL, NULL, CWObject_Message(OBJMSG_HOOK_ALLOCATEATTACHDATA));
		if(!_pObj->m_lspClientObj[0])
			Error_static("CWObject_Attach::GetAttachClientData", "Could not allocate AttachData.")
		CAttachClientData *pData = (CAttachClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
		return pData;
	}
	else
		return (CAttachClientData *)(CReferenceCount *)_pObj->m_lspClientObj[0];
}

M_INLINE const CWObject_Attach::CAttachClientData* CWObject_Attach::GetAttachClientData(const CWObject_CoreData *_pObj)
{
	MAUTOSTRIP(CWObject_Attach_GetAttachClientData_const, NULL);

	return GetAttachClientData(const_cast<CWObject_CoreData *>(_pObj));
}

/////////////////////////////////////////////////////////////////////////////
// CWObject_Hook
class CWObject_Hook : public CWObject_Attach
{
	MRTC_DECLARE_SERIAL_WOBJECT;
#undef EXTRACLIENTMESSAGE
#define EXTRACLIENTMESSAGE(ThisClass, BaseClass, AttachDataClass) \
case OBJSYSMSG_GETCAMERA:\
	{\
		CMTime Time = ((ThisClass *)_pObj)->GetTime(_pObj, _pWClient, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());\
		CMat4Dfp32 *pMat = (CMat4Dfp32*)_Msg.m_pData;\
		*pMat = ((ThisClass *)_pObj)->GetRenderMatrix(_pWClient, Time, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());\
		return 1;\
	}
	HOOK_DECLARE_WOBJECT(CWObject_Hook, CWObject_Attach, CAttachClientData);
#undef EXTRACLIENTMESSAGE
#define EXTRACLIENTMESSAGE(ThisClass, BaseClass, AttachDataClass)

	struct CAttachClientData_Hook : public CWObject_Attach::CAttachClientData
	{
		AUTOVAR_SETCLASS(CAttachClientData_Hook, CAttachClientData);
		CAUTOVAR_OP(CAutoVar_CVec3Dfp32, m_HelpAxis, DIRTYMASK_0_2);

		AUTOVAR_PACK_BEGIN
		AUTOVAR_PACK_VAR(m_HelpAxis)
		AUTOVAR_PACK_END
	};

protected:
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);

	static CAttachClientData_Hook *GetAttachClientData_Hook(CWObject_CoreData *_pObj);
	CMat4Dfp32 GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac);
};

/////////////////////////////////////////////////////////////////////////////
// CWObject_Engine_Rotate
class CWObject_Engine_Rotate : public CWObject_Hook
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	HOOK_DECLARE_WOBJECT(CWObject_Engine_Rotate, CWObject_Hook, CAttachClientData_Hook);

	virtual void OnCreate();
	virtual void OnTransform(const CMat4Dfp32 &_Mat);

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);

	virtual fp32 GetDuration();
	CMat4Dfp32 GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac);
};

#ifndef M_DISABLE_CURRENTPROJECT
/////////////////////////////////////////////////////////////////////////////
// CWObject_Engine_Wheel
class CWObject_Engine_Wheel : public CWObject_Hook
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	HOOK_DECLARE_WOBJECT(CWObject_Engine_Wheel, CWObject_Hook, CAttachClientData_Hook);

	virtual void OnCreate();
	virtual void OnTransform(const CMat4Dfp32 &_Mat);

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);

	CMat4Dfp32 GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac);
};
#endif

#ifndef M_DISABLE_CURRENTPROJECT
/////////////////////////////////////////////////////////////////////////////
// CWObject_Hook_NoRotate
class CWObject_Hook_NoRotate : public CWObject_Hook
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	HOOK_DECLARE_WOBJECT(CWObject_Hook_NoRotate, CWObject_Hook, CAttachClientData);

protected:
	CMat4Dfp32 GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac);
};
#endif

/////////////////////////////////////////////////////////////////////////////
// CWObject_Hook_To_Target
class CWObject_Hook_To_Target : public CWObject_Hook
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	HOOK_DECLARE_WOBJECT(CWObject_Hook_To_Target, CWObject_Hook, CAttachClientData_Hook);

protected:
	virtual void OnCreate();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);
	virtual void OnTransform(const CMat4Dfp32 &_Mat);

	CMat4Dfp32 GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac);
};

#ifndef M_DISABLE_CURRENTPROJECT
/////////////////////////////////////////////////////////////////////////////
// CWObject_Hook_To_Line
class CWObject_Hook_To_Line : public CWObject_Hook
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	HOOK_DECLARE_WOBJECT(CWObject_Hook_To_Line, CWObject_Hook, CAttachClientData_Hook);

	virtual void OnCreate();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);
	virtual void OnTransform(const CMat4Dfp32 &_Mat);

	CMat4Dfp32 GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac);
};



/////////////////////////////////////////////////////////////////////////////
// CWObject_Hook_To_Circle
class CWObject_Hook_To_Circle : public CWObject_Hook
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	HOOK_DECLARE_WOBJECT(CWObject_Hook_To_Circle, CWObject_Hook, CAttachClientData_Hook);

	virtual void OnCreate();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);
	virtual void OnTransform(const CMat4Dfp32 &_Mat);

	CMat4Dfp32 GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFrac);
};

#endif

/////////////////////////////////////////////////////////////////////////////
// CWObject_Engine_Path
class CWObject_Engine_Path : public CWObject_Hook
{
	MRTC_DECLARE_SERIAL_WOBJECT;
#undef EXTRACLIENTMESSAGE
#define EXTRACLIENTMESSAGE(ThisClass, BaseClass, AttachDataClass) \
	case OBJSYSMSG_GETCAMERA:\
	{\
		CMTime Time = ((ThisClass *)_pObj)->GetTime(_pObj, _pWClient, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());\
		CMTime JumpTime = CMTime::CreateFromTicks(GetAttachClientData_Engine_Path(_pObj)->m_JumpTick, _pWClient->GetGameTickTime());\
		if(JumpTime.Compare(CMTime()) > 0 && Time.Compare(JumpTime) > 0)\
			return _pWClient->ClientMessage_SendToObject(_Msg, GetAttachClientData_Engine_Path(_pObj)->m_JumpIndex);\
		CMat4Dfp32 *pMat = (CMat4Dfp32*)_Msg.m_pData;\
		*pMat = ((ThisClass *)_pObj)->GetRenderMatrix(_pWClient, Time, _pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());\
		return 1;\
	}\
	case OBJMSG_ENGINEPATH_GETDURATION:\
	{\
		CWO_PosHistory* pCData = GetClientData(_pObj);\
		if(_pObj->m_iAnim0 != -1 && !pCData->IsValid())\
				CWObject_Engine_Path::LoadPath(_pWClient, _pObj, _pObj->m_iAnim0);\
		if (pCData && (pCData->m_lSequences.Len() > _Msg.m_Param0))\
		{\
			fp32 Temp = pCData->m_lSequences[_Msg.m_Param0].GetDuration();\
			return *(int32*)&Temp;\
		}\
		return 0;\
	}
	HOOK_DECLARE_WOBJECT(CWObject_Engine_Path, CWObject_Hook, CAttachClientData_Engine_Path);
#undef EXTRACLIENTMESSAGE
#define EXTRACLIENTMESSAGE(ThisClass, BaseClass, AttachDataClass)

	int16 m_iUserPortal;

	struct CAttachClientData_Engine_Path : public CWObject_Attach::CAttachClientData
	{
		AUTOVAR_SETCLASS(CAttachClientData_Engine_Path, CAttachClientData);
		CAUTOVAR_OP(CAutoVar_uint16, m_JumpIndex, DIRTYMASK_0_1);
		CAUTOVAR_OP(CAutoVar_int32, m_JumpTick, DIRTYMASK_0_1);
		CAUTOVAR_OP(CAutoVar_uint16, m_iXWData, DIRTYMASK_0_3);
		CAUTOVAR_OP(CAutoVar_CMat4Dfp32, m_PathRelMat, DIRTYMASK_0_4);

		AUTOVAR_PACK_BEGIN
		AUTOVAR_PACK_VAR(m_JumpIndex)
		AUTOVAR_PACK_VAR(m_JumpTick)
		AUTOVAR_PACK_VAR(m_iXWData)
		AUTOVAR_PACK_VAR(m_PathRelMat)
		AUTOVAR_PACK_END
	};

	CStr				m_TargetName;
	CWO_PhysicsState	m_BackupPhysState;
	CMTime				m_Time;
	CMat4Dfp32			m_RotOffset;
	CVec3Dfp32			m_PosOffset;
	CVec3Dfp32			m_LastTargetPos;
	uint32				m_iDynamicModel;
	int32				m_iObjectLinkedToEP;
	fp32				m_Mass;
	fp32				m_Falloff;
	uint16				m_nTicksFailed;
	uint8				m_iCurBlockMsg;
	uint8				m_iCurResumeMsg;
	bool				m_bWorldCollision:1;
	bool				m_bPathIsForObjects:1;

	TArray<CTimedMessage>	m_lOnBlockMsgs;
	TArray<CTimedMessage>	m_lOnResumeMsgs;

	void		AddBlockMsg(CTimedMessage _Msg);
	void		SendBlockedMsg(void);

	void		AddResumeMsg(CTimedMessage _Msg);
	void		SendResumeMsg(void);
	void		AttachObject();	//Attached m_TargetName object to this EP, only physics driven patrhs
	
public:
	CWObject_Engine_Path();
	virtual void OnCreate();

	virtual void Stop();
	virtual void Run(int _iType);
	virtual void GotoEnd();

	int CheckFalloff(fp32 _Impact);

	//Propel path forward to given distance sending any messages as normal. If _AllowStops is true the 
	//path won't be propelled past a stop in the path. (i.e. two keyframes at the same position) until 
	//the path has run normally past the stop. If the _XYOnly param is true the path will be propelled 
	//to the given distance from the position in the XY-plane, i.e. height is ignored.
	enum
	{
		EPFLAGS_ALLOW_STOPS	= 1,	// ALlow stops
		EPFLAGS_XYONLY		= 2,	// Ignore z distance
		EPFLAGS_PROPEL		= 4,	// Propel path, send messages
	};
	virtual void PropelPath(fp32 _Distance, bool _AllowStops = true, bool _XYOnly = false, bool _bPropel = true, CMat4Dfp32* _pMat = NULL);
	virtual void SetTime(fp32 _Time, bool _bSendMessages = true);
 
	//Position matrix of propelled path is normal render position matrix or position 
	//of last keyframe with the direction of render matrix if stops are allowed and 
	//path is between two engine paths with same position
	virtual CMat4Dfp32 PropelledPosition(bool _AllowStops = true);

	//Helpers to propelpath
	bool IsStop(class CWO_PosHistory * _pPH, int _iSeq, fp32 _Time, fp32 _Epsilon = 1.0f);
	bool IsStop(class CWO_PosHistory * _pPH, int _iSeq, int _iKeyFrame, fp32 _Epsilon = 1.0f);

	static class CWO_PosHistory *GetClientData(CWObject_CoreData *_pObj);
	static CAttachClientData_Engine_Path *GetAttachClientData_Engine_Path(CWObject_CoreData *_pObj);

	static void LoadPath(CWorld_PhysState *_pPhysState, CWObject_CoreData *_pObj, int _iIndex);

	static int OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo = NULL);
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);
	static void OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);
	virtual void OnSpawnWorld();
	virtual void OnRefresh();
	virtual void OnFinishEvalKeys();
	virtual fp32 GetDuration();

	void MoveObject(void);

	virtual bool NeedRefresh();
	float GetKeyframeLength(CWO_PosHistory *_pCData, int _iKeyframe);
	CMat4Dfp32 GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFraction);

	virtual aint OnMessage(const CWObject_Message &_Msg);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
};

/////////////////////////////////////////////////////////////////////////////
// CWObject_Dynamic
class CWObject_Dynamic : public CWObject_Engine_Path
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	virtual void OnCreate();
};

class CWObject_TimedMessages : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	enum
	{
		TIMEDMESSAGES_FLAG_AUTOSTART = M_Bit(0),
		TIMEDMESSAGES_FLAG_AUTORESET = M_Bit(1),
	};
	TArray<struct CWObject_Attach::CTimedMessage> m_lTimedMessages;
	CMTime m_StartTime;
	int32 m_Flags;
	int32 m_iCurrentMessage;
	void UpdateMessages();
public:
	virtual void OnCreate();
	// Invisible etc
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnSpawnWorld();
	virtual aint OnMessage(const CWObject_Message &_Msg);
	virtual void OnRefresh();
	virtual void OnFinishEvalKeys();
};

#endif
