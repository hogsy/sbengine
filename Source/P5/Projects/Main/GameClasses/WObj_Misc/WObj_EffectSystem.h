/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			EffectSystem object class.

	Author:			Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_EffectSystem

	Comments:

	History:		
		05????:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_EffectSystem_h__
#define __WObj_EffectSystem_h__

#include "../WObj_Messages.h"
#include "../../../../SHARED/MOS/Classes/GameWorld/WObjCore.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"

enum
{
	//OBJMSG_EFFECTSYSTEM_SETTIMECONTROL	= OBJMSGBASE_MISC_EFFECTSYSTEM,
	OBJMSG_EFFECTSYSTEM_IMPULSE			= OBJMSGBASE_MISC_EFFECTSYSTEM,
	OBJMSG_EFFECTSYSTEM_EXCLUDEOWNER,
	OBJMSG_EFFECTSYSTEM_GETLIGHTGUID,
	OBJMSG_EFFECTSYSTEM_SETFADE,
	OBJMSG_EFFECTSYSTEM_SELFDESTRUCT,

	FXFADE_NA							= 0,
	FXFADE_IN							= 1,
	FXFADE_OUT							= 2,

	FXIMPULSETYPE_TIMESCALE				=  0,
	FXIMPULSETYPE_ANIMTIME				=  1,
	FXIMPULSETYPE_PAUSETIME				=  2,
	FXIMPULSETYPE_WAITSPAWN				=  3,
	FXIMPULSETYPE_TIMECONTROL			=  4,
	FXIMPULSETYPE_SETTARGET_FROMNAME	=  5,
	FXIMPULSETYPE_SETTARGET_FROMGUID	=  6,
	FXIMPULSETYPE_SETTARGET_FROMID		=  7,
	FXIMPULSETYPE_SETOWNER				=  8,

	FXOBJ_DIRTYFLAGS_MODEL				= M_Bit(0),

	FXOBJ_FLAGS_SELFDESTRUCT			= M_Bit(0),
	FXOBJ_FLAGS_BONEATTACH				= M_Bit(1),
	FXOBJ_FLAGS_TIMEPAUSED				= M_Bit(2),
	FXOBJ_FLAGS_WAITSPAWN				= M_Bit(3),
	FXOBJ_FLAGS_NOCULL					= M_Bit(4),
	FXOBJ_FLAGS_FADE					= M_Bit(5),
	FXOBJ_FLAGS_FADEOUT					= M_Bit(6),
	FXOBJ_FLAGS_FADEIN					= M_Bit(7),

	NUM_FadeArray						= 3,
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CEffectSystem_AnimStateData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CEffectSystem_AnimStateData
{
public:
	CEffectSystem_AnimStateData()
	{
	}

	~CEffectSystem_AnimStateData()
	{
	}

	void Clear()
	{
		m_Direction = 0;
		m_Length	= 0;
	}

	CVec3Dfp32	m_Direction;
	fp32		m_Length;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXAttachPoint
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFXAttachPoint : public CXR_SkeletonAttachPoint
{
public:
	CFXAttachPoint()
	{
		m_iNode = 0xffff;
		m_LocalPos.Unit();
	}

	void Pack(uint8*& _pD) const
	{
		TAutoVar_Pack(m_iNode, _pD);
		TAutoVar_Pack(m_LocalPos, _pD);
	}

	void Unpack(const uint8*& _pD)
	{
		TAutoVar_Unpack(m_iNode, _pD);
		TAutoVar_Unpack(m_LocalPos, _pD);
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_EffectSystem_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_EffectSystem_ClientData : public CReferenceCount, public CAutoVarContainer
{
	AUTOVAR_SETCLASS(CWO_EffectSystem_ClientData, CAutoVarContainer);

public:
	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	void  SetTimeScale(fp32 _TimeScale);
	void  SetWaitSpawn(bool _bWaitSpawn);
	bool  PauseSystem(int _PauseTick, fp32 _PauseFrac = 0.0f);
	bool  UnpauseSystem();
	void  SetAnimTick(int _Tick, fp32 _Frac, bool _bAbsolute = false);
	int   GetAnimTick(CWorld_Client* _pWClient = NULL);
	fp32  GetAnimFrac();

	CWObject_CoreData*	m_pObj;
	CWorld_PhysState*	m_pWPhysState;
	CWorld_Server*		m_pWServer;
	CWorld_Client*		m_pWClient;
	CXR_Model*			m_pModels[CWO_NUMMODELINDICES];
	int32				m_StartTick;
	int32				m_EndTick;

	typedef TAutoVar<CFXAttachPoint>	CAutoVar_FXAttachPoint;

	CAUTOVAR_OP(CAutoVar_uint16,		m_LightGUID,		DIRTYMASK_0_0);
	CAUTOVAR_OP(CAutoVar_fp32,			m_TimeScale,		DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_int32,			m_iTarget,			DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint8,			m_Flags,			DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_int32,			m_iOwner,			DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_uint16,		m_iBoneAttach,		DIRTYMASK_1_0);
	CAUTOVAR(CAutoVar_FXAttachPoint,	m_AttachPoint,		DIRTYMASK_1_0);
	
	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_LightGUID)
	AUTOVAR_PACK_VAR(m_TimeScale)
	AUTOVAR_PACK_VAR(m_iTarget)
	AUTOVAR_PACK_VAR(m_Flags)
	AUTOVAR_PACK_VAR(m_iOwner)
	AUTOVAR_PACK_VAR(m_iBoneAttach)
	AUTOVAR_PACK_VAR(m_AttachPoint)
	AUTOVAR_PACK_END
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_EffectSystem
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define CWObject_EffectSystemParent CWObject
class CWObject_EffectSystem : public CWObject_EffectSystemParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	uint32	m_OwnerNameHash;
	int32	m_SelfDestruct;

public:
	static const CWO_EffectSystem_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_EffectSystem_ClientData& GetClientData(      CWObject_CoreData* _pObj);

	CWObject_EffectSystem();

	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void Model_Set(uint8 _iPos, const char* _pValue);
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual int  OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;
	virtual void OnSpawnWorld();

	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static int  OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);

    static void  IncludeEffectFromKey(const CStr _Key, CRegistry* _pReg, CMapData* _pMapData);
	static void  OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);
	//static void OnIncludeClass(CMapData* _pWData, CWorld_Server* _pWServer);

	static void  OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void  OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static aint  OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);

	static void  SendImpulse(CWObject_EffectSystem* _pEffectSystem, uint _Impulse, const CStr& _String);
	void Impulse(uint _Impulse, const char* _pString);

	static bool EvaluateOwnerSkeleton(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, CXR_Skeleton*& _pSkeleton, CXR_SkeletonInstance*& _pSkeletonInstance, fp32 _IPTime, CMat4Dfp32* _pBasePos);
	static bool SetupBoneAttachData(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, CXR_AnimState& _AnimState, uint8 _iBoneAttach, fp32 _IPTime, CMat4Dfp32* _pBasePos = NULL);
	static bool SetupAttachPoint(const CFXAttachPoint& _AttachPoint, CMat4Dfp32& _ResultPos, CWObject_CoreData* _pObj, CWorld_Client* _pWClient, fp32 _IPTime, CMat4Dfp32* _pBasePos = NULL);
	static void ShaderUseAnimTime1(CXR_AnimState& _AnimState, bool _bUseAnimTime1);
};


#endif
