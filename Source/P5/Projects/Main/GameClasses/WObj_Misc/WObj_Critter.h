/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_Critter.h

Author:			Roger Mattsson

Copyright:		Copyright O3 Games AB 2005

Contents:		

Comments:		

History:		
060209:		Created File
\*____________________________________________________________________________________________*/

#ifndef _INC_WOBJ_CRITTER
#define _INC_WOBJ_CRITTER

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"

enum 
{
	CROW_STATE_FLYING,
	CROW_STATE_LANDING,
	CROW_STATE_LANDED,
	CROW_STATE_DEPARTING,
	CROW_STATE_ESCAPING,
	CROW_STATE_WALK,

	CROW_AG2_TAKEOFF = 0,
	CROW_AG2_LAND,
	CROW_AG2_SOAR,
	CROW_AG2_FLY,

	CROW_AG2_GESTURE_SHRUG = 1,
	CROW_AG2_GESTURE_LOOK,
	CROW_AG2_GESTURE_PICKFEATHERS,

	RAT_STATE_EXPLORING = 0,
	RAT_STATE_IDLE,
	RAT_STATE_ESCAPE,
	RAT_STATE_HIDING,

	CROW_FLAGS_FLYING			= 0x00000001,
	CROW_FLAGS_IDLE_BEHAVIOR	= 0x00000002,
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCritterAnimGraph
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CCritterAnimGraph : public CWO_ClientData_AnimGraph2Interface
{
public:
	spCWAG2I        m_spAGI;
	spCWAG2I_Mirror m_spMirror;
	int32			m_FlagsLo;

	CCritterAnimGraph();
	virtual const CWAG2I* GetAG2I() const { return m_spAGI; }
	virtual       CWAG2I* GetAG2I()       { return m_spAGI; }

	virtual void SetInitialProperties(const CWAG2I_Context* _pContext);
	virtual void AG2_OnEnterState(const CWAG2I_Context* _pContext, CAG2TokenID _TokenID, CAG2StateIndex _iState, CAG2AnimGraphID _iAnimGraph, CAG2ActionIndex _iEnterAction);
	virtual void UpdateImpulseState(const CWAG2I_Context* _pContext);

	void Copy(const CCritterAnimGraph& _CD);
	void Clear();
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Critter_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_Critter_ClientData : public CReferenceCount, public CAutoVarContainer
{
public:
	friend class CWObject_Critter;

	CWObject_CoreData* m_pObj;
	CWorld_PhysState* m_pWPhysState;

	CCritterAnimGraph m_AnimGraph;
	spCXR_SkeletonInstance m_spSkelInstance;
	fp32 m_LastSkelInstanceFrac;
	int32 m_LastSkelInstanceTick;

	AUTOVAR_SETCLASS(CWO_Critter_ClientData, CAutoVarContainer);

	CAUTOVAR_OP(CAutoVar_int8,		m_AIState,		DIRTYMASK_0_0);

	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_AIState)
	AUTOVAR_PACK_END

	CWO_Critter_ClientData();

	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	void OnRefresh();
	void OnRender(CWorld_Client* _pWClient, CXR_Engine* _pEngine);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Critter
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Critter : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT

protected:
	class CWObject_ScenePointManager* GetSPM();

public:
	enum
	{
		CRITTER_AG2_DIRTYMASK = CAutoVarContainer::DIRTYMASK_2_1,
	};

	CWObject_Critter();
	static const CWO_Critter_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_Critter_ClientData& GetClientData(      CWObject_CoreData* _pObj);

	virtual void OnCreate();	
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine*, const CMat4Dfp32& _ParentMat);
	static int  OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnGetAnimState(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CXR_Skeleton* _pSkel, int _iCacheSlot, const CMat4Dfp32 &_Pos, fp32 _IPTime, CXR_AnimState& _Anim, CVec3Dfp32 *_pRetPos = NULL, bool _bIgnoreCache = false);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);
	virtual aint OnMessage(const CWObject_Message& _Msg);

	virtual int  OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;

	virtual bool GetNextTarget(void);

	virtual fp32 CheckForScaryStuff(CVec3Dfp32 pos, CVec3Dfp32 *escape_vec = NULL);
	virtual void UpdateSP(void);

protected:
	CXR_BlockNavSearcher	*m_pBlockNavSearcher;
	CMat4Dfp32			m_LocalDebug;
	CMat4Dfp32			m_PosDebug;

	CNameHash			m_NameHash;

	bool				m_bWaitOnImpulse:1;

	fp32				m_SpeedForward;
	fp32				m_EscapeRange;

	int					m_iSound;	
	int					m_SoundMinTime;
	int					m_SoundRandTime;
	int					m_SoundNextTick;

	int					m_BoredomMinTime;
	int					m_BoredomRandTime;
	int					m_BoredomNextTick;

	CStr				m_EffectDie;

	CWO_ScenePoint*		m_pTargetSP;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Crow
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Crow : public CWObject_Critter
{
#define parent CWObject_Critter
	MRTC_DECLARE_SERIAL_WOBJECT

	enum
	{
		FLY_FORWARD = 0,
		FLY_LEFT,
		FLY_RIGHT
	};

public:
	CWObject_Crow();
	~CWObject_Crow();

	virtual void OnCreate();	
	virtual void OnFinishEvalKeys(void);						
	virtual void OnRefresh(void);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnSpawnWorld(void);

	static int OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo = NULL);
	
	bool GetNextTarget(void);

	void FlyNormal(void);
	void FlyNormalNoSP(void);
	void FlyDeparting(void);
	void FlyLanding(void);
	void FlyEscape(void);

	void IdleLanded(void);
	void Walk(void);

private:
	void DoRandomIdleGesture(void);
	void CheckWallCollision(bool _bNoTargetSPTrace = false);
	void AvoidWalls(CMat4Dfp32 *_pTargetPos);
	fp32 UpdateBobbing(void);
	
	CWO_ScenePoint*		m_pLastTargetSP;
	CVec3Dfp32			m_OldTargetPos;
	CVec3Dfp32			m_EscapeDir;
	CVec3Dfp32			m_WalkDir;
	fp32				m_EscapeHeight;
	fp32				m_BobbingLastZMod;
	fp32				m_MaxTurnSpeed;
	int32				m_LastTraceTick;
	int32				m_LastTraceTick2;
	    
	int32				m_FlyTick;
	int32				m_WalkTick;
	int32				m_WalkIdleTick;

	int32				m_IdleTick;
	int32				m_NextIdleBehavior;
	
	int32				m_StartFlyingGameTick;
	int32				m_BobbingLastTick;
	uint8				m_FlyDir;	

	bool				m_bForceCircling:1;	//We are flying to a higher/lower target but need to circle climb/dive
	bool				m_bAvoidWall:1;
	bool				m_bWallAhead:1;
	bool				m_bLand:1;
	bool				m_bPanic:1;
	bool				m_bNoTraces:1;
	bool				m_bWalking:1;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Rat
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Rat : public CWObject_Critter
{
#define parent CWObject_Critter
	MRTC_DECLARE_SERIAL_WOBJECT

public:
	CWObject_Rat();
	~CWObject_Rat();

	virtual void OnCreate();	
	virtual void OnFinishEvalKeys(void);						
	virtual void OnRefresh(void);	
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnSpawnWorld(void);

	static int OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo = NULL);

	void Explore(void);
	void Idle(void);
	void Escape(void);
	void Hide(void);

	bool GetNextTarget(void);

private:
	void DoRandomIdleGesture(void);
	void CheckWallCollision(void);
	void ChangeDirRandom(void);

	CVec3Dfp32			m_TargetDir;
	CVec3Dfp32			m_EscapeDir;
	int					m_LastTraceTick;
	int					m_LastScaryTick;
	int					m_HideTick;

	int					m_NextChangeDir;
	int					m_EscapeTick;
};

#endif
