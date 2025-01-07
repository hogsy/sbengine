/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CWObject_AngelusChain.h

	Author:			Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CAngelusChainSetup
					CAngelusChainState
					CWO_AngelusChain_ClientData
					CWObject_AngelusChain

	Comments:		Based upon the tentacle system.

	History:
		051120		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_AngelusChain_h__
#define __WObj_AngelusChain_h__

#include "../WObj_Char/WObj_CharAngelus.h"


// Class defines
class CSpline_Vec3Dfp32;
class CWO_AngelusChain_ClientData;
class CWObject_AngelusChain;
typedef CSpline_Vec3Dfp32 CSpline_Chain;


// enums
enum
{
	// Chains
	ANGELUSCHAIN_CHAIN					= 0,			// Just allow one chain for the moment,
	ANGELUSCHAIN_MAXCHAINS,

	// Chain tasks
	ANGELUSCHAIN_TASK_IDLE				= 0,
	ANGELUSCHAIN_TASK_GRABHOLD,
	ANGELUSCHAIN_TASK_GETNOTHING,
	ANGELUSCHAIN_TASK_BREAKOBJECT,
	
	// Chain states
	ANGELUSCHAIN_STATE_IDLE				= 0,
	ANGELUSCHAIN_STATE_RETURN,
	ANGELUSCHAIN_STATE_GRABOBJECT,
	ANGELUSCHAIN_STATE_REACHOBJECT,
	ANGELUSCHAIN_STATE_REACHPOSITION,
	
	// Chain Sounds
	ANGELUSCHAIN_SOUND_REACH			= 0,
	ANGELUSCHAIN_SOUND_GRAB,
	ANGELUSCHAIN_SOUND_HOLD,
	ANGELUSCHAIN_SOUND_RETURN,
	ANGELUSCHAIN_NUMSOUNDS,

	// Chain selection
	ANGELUSCHAIN_SELECTION_CHARACTER	= M_Bit(0),
	ANGELUSCHAIN_SELECTION_OBJECT		= M_Bit(1),
	ANGELUSCHAIN_SELECTION_ALL			= ~0,

	// Constants
	ANGELUSCHAIN_SPEED_SNAPTOTARGET		= 1000,

	// ClientData flags
	ANGELUSCHAIN_CD_NORENDER			= M_Bit(0),
	ANGELUSCHAIN_CD_ONCLIENTRENDER		= M_Bit(1),
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CAngelusChainSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
struct CAngelusChainSetup
{
	TFStr<20>				m_Name;
	uint16					m_iTemplateModel;

	CXR_SkeletonAttachPoint	m_AttachPoint;
	CVec3Dfp32				m_TargetAttachPoint;
	
	CAngelusChainSetup();
	void Setup(const CRegistry& _Reg, CWorld_Server& _WServer);
	void Pack(uint8*& _pD) const;
	void Unpack(const uint8*& _pD);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CAngelusChainState
|__________________________________________________________________________________________________
\*************************************************************************************************/
struct CAngelusChainState
{
	// Replicated:
	uint8 m_Task;
	uint8 m_State;
	uint16 m_iTarget;			// dynamic target
	fp32 m_Speed;
	CVec3Dfp32 m_TargetPos;		// static target
	CVec3Dfp32 m_GrabPoint;
	fp32 m_Length;
	int16 m_iRotTrack;

	// Not replicated:
	CVec3Dfp32 m_Pos;
	CMat4Dfp32 m_Attach1_Cache;
	CVec3Dfp32 m_PidIntegrateTerm, m_PidDeriveTerm, m_LastTargetPos;
	//CVec3Dfp32 m_Dir;
	CVec3Dfp32 m_CurrTargetPos; // for blending towards TargetPos

	CAngelusChainState();
	void Pack(uint8*& _pD) const;
	void Unpack(const uint8*& _pD);
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_AngelusChain_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_AngelusChain_ClientData : public CReferenceCount, public CAutoVarContainer
{
	friend class CWObject_AngelusChain;

	CWObject_CoreData* m_pObj;
	CWorld_PhysState* m_pWPhysState;

	CMTime m_LastRender;
public:
	AUTOVAR_SETCLASS(CWO_AngelusChain_ClientData, CAutoVarContainer);

	typedef TAutoVar_StaticArray<CAngelusChainSetup, ANGELUSCHAIN_MAXCHAINS>	CAutoVar_ChainSetupArray;
	typedef TAutoVar_StaticArray<CAngelusChainState, ANGELUSCHAIN_MAXCHAINS>	CAutoVar_ChainStateArray;
	typedef TAutoVar_StaticArray<uint16, ANGELUSCHAIN_NUMSOUNDS>				CAutoVar_ChainSoundArray;

	CAUTOVAR(CAutoVar_ChainStateArray,	m_lChainState,	DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_int16,			m_Flags,		DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_int16,			m_iOwner,		DIRTYMASK_1_0);
	CAUTOVAR(CAutoVar_ChainSoundArray,	m_liSounds,		DIRTYMASK_1_0);
	CAUTOVAR(CAutoVar_ChainSetupArray,	m_lChainSetup,	DIRTYMASK_1_0);

	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_lChainState)
	AUTOVAR_PACK_VAR(m_Flags)
	AUTOVAR_PACK_VAR(m_iOwner)
	AUTOVAR_PACK_VAR(m_liSounds)
	AUTOVAR_PACK_VAR(m_lChainSetup)
	AUTOVAR_PACK_END

protected:
	void GetSpline(uint8 _iChain, CSpline_Chain& _Spline);

	bool GetCharBonePos(CWObject_CoreData& _Char, int _RotTrack, CVec3Dfp32& _Result);

public:
	CWO_AngelusChain_ClientData();

	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	void OnRefresh();
	void OnRender(CWorld_Client* _pWClient, CXR_Engine* _pEngine);

	void Server_UpdateState(CAngelusChainState& _ChainState, bool _bAtTarget, const CMat4Dfp32& _EndPos);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_AngelusChain
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_AngelusChain : public CWObject
{
	typedef CWObject parent;
	MRTC_DECLARE_SERIAL_WOBJECT;

private:
	CWObject* m_pSelectedObject;
	CVec3Dfp32 m_LocalGrabPoint;

public:
	static const CWO_AngelusChain_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_AngelusChain_ClientData& GetClientData(      CWObject_CoreData* _pObj);

public:
	CWObject_AngelusChain();

	// CWObject overrides:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual int  OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;
	virtual void OnRefresh();

	static void OnIncludeClass(CMapData* _pWData, CWorld_Server*);
	static void OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);

	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine*, const CMat4Dfp32& _ParentMat);
	static int  OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	// Gameplay functionality:
	CWObject* SelectTarget(uint _SelectionMask = ANGELUSCHAIN_SELECTION_ALL);
	void ReachPosition(const CVec3Dfp32& _Position);
	
	bool ReleaseObject();
};

#endif // __WObj_AngelusChain_h__

