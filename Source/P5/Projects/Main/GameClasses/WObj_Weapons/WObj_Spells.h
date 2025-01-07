#ifndef __WOBJ_SPELLS_H
#define __WOBJ_SPELLS_H

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Damage.h"
#include "../WObj_Messages.h"
#include "../WRPG/WRPGDef.h"
#include "WObj_Params.h"
//#include "../WRPG/WRPGCore.h"

enum
{
	OBJMSG_SPELLS_SUMMONINIT = OBJMSGBASE_SPELLS,
	//OBJMSG_SPELLS_GETACTIVATEPOSITION,
	OBJMSG_SPELLS_SETEFFECTSTATE,
	OBJMSG_SPELLS_GETRENDERMATRIX,
	OBJMSG_SPELLS_GETANIMSTATE,
	OBJMSG_SPELLS_SETTARGET,
	OBJMSG_SPELLS_ALLOCATECLIENTDATA,
};

enum
{
	NETMSG_SPAWN_SURFACEDEBRIS = 0x40,
	NETMSG_SPAWN_SPARKS = 0x41,
	NETMSG_SPAWN_BOLTTRAIL,
	NETMSG_SPAWN_COLTSHOTTRAIL,
	NETMSG_SPAWN_CLIENTMODEL,
};

//-------------------------------------------------------------------
// Ext_Model
//-------------------------------------------------------------------
class CWObject_Ext_Model : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:	

	enum
	{
		CLIENTFLAGS_NOTMOVING			= (CWO_CLIENTFLAGS_USERBASE << 0),
		CLIENTFLAGS_ATTACHED			= (CWO_CLIENTFLAGS_USERBASE << 1),
		CLIENTFLAGS_RENDERHANSOFFSET	= (CWO_CLIENTFLAGS_USERBASE << 2), // Remove, or rename to RENDER_OFFSET
		CLIENTFLAGS_INLIMBO				= (CWO_CLIENTFLAGS_USERBASE << 3),
		CLIENTFLAGS_RANDPERMODEL		= (CWO_CLIENTFLAGS_USERBASE << 4),
		CLIENTFLAGS_RENDER_NOROT		= (CWO_CLIENTFLAGS_USERBASE << 5), // Unify render matrix rotation (i.e. unit rotation to rendered models).
		CLIENTFLAGS_RENDER_ZROT			= (CWO_CLIENTFLAGS_USERBASE << 6), // Unify render matrix XY rotation (i.e. only forward Z rotation to rendered models).
	};
	
	int m_iSoundSpawn;
	int m_iSoundLoop0;
	int m_iSoundLoop1;

	int32 m_Timeout;
	bool m_bPendingDestroy;
	int m_DestroyDelayTicks;

	fp32 m_MovingSpawnDuration;

public:	

	CWObject_Ext_Model();
	virtual void OnInitInstance(const aint* _pParam, int _nParam);

	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	void SetTimeout(int _Timeout)								{ m_Timeout = _Timeout; m_iAnim2 = m_Timeout; }
	int GetTimeout()											{ return m_Timeout; }

	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message &_Msg);

	static void GetAnimTime(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMTime& _AnimTime, fp32& _TickFrac);
	static bool GetRenderMatrix(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMat4Dfp32& _Matrix, fp32 _TickFrac);
	static bool GetAnimState(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_AnimState& _AnimState, CMat4Dfp32& _Matrix, const CMTime& _AnimTime, fp32 _TickFrac, int _iModel);

	static void GetDefaultAnimState(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void GetDefaultAnimState(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_AnimState& _AnimState, const CMTime& _AnimTime, int _iModel);
	static void RenderModelsDefault(const CMTime& _AnimTime, fp32 _TickFrac, CMat4Dfp32& _Matrix, CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine);

	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);

	//Overridables
	virtual int OnPreIntersection(int _iObject, CCollisionInfo *_pInfo) { return 0; }
	virtual int OnNotifyIntersection(int _iObject) { return 0; }
	virtual int OnSummonInit(const class CSummonInit &) { return 0; }
	virtual void OnTimeout() { Destroy(); }

	virtual int IsInLiquid(CVec3Dfp32 _Pos);

	//Phys creatiors
	bool SetSphereProjectile(fp32 _Radius, int _Intersectflags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER, int _iExlude = -1);

public:

	/*
	//Misc helpers
	void SendShockwave(CVec3Dfp32 _Pos, fp32 _Radius, int _Force, int _Damage, uint32 _DamageType = 0, int _iExclude = 0);
	int SendDamage(int _iObject, const CVec3Dfp32& _Pos, int _iSender, int _Damage, uint32 _DamageType, int _SurfaceType, int _SplashDmg, fp32 _Radius, const CVec3Dfp32& _Force, CVec3Dfp32 *_pSplatterDir);
	*/

protected:

	CMat4Dfp32 GetReflection(CCollisionInfo *_pInfo);
	//bool TraceRay(float _Range, CCollisionInfo &_Info);

	//Primitives
	void Line(const CVec3Dfp32& _Pos0, const CVec3Dfp32& _Pos1, int _Col = 0xffffffff);

	static bool InitializeDebris(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _iSlot);
};

//-------------------------------------------------------------------
//- CWObject_Damage -------------------------------------------------
//-------------------------------------------------------------------

#define CWObject_Damage_Parent CWObject_Ext_Model
class CWObject_Damage : public CWObject_Damage_Parent
{

	MRTC_DECLARE_SERIAL_WOBJECT;

protected:

	CWO_Damage		m_Damage;
	CStr			m_DamageEffect;

	CWO_Shockwave	m_Shockwave;

public:

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual aint OnMessage(const CWObject_Message &_Msg);

};

#ifndef M_DISABLE_TODELETE
//-------------------------------------------------------------------
//- CWObject_PainBox ------------------------------------------------
//-------------------------------------------------------------------

#define NUMDAMAGEDOBJECTS 5

#define CWObject_PainBox_Parent CWObject_Damage
class CWObject_PainBox : public CWObject_PainBox_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:

	int	m_DamagedObject[NUMDAMAGEDOBJECTS];
	fp32	m_DamageDelivered[NUMDAMAGEDOBJECTS];

	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual int OnIntersectingObject(int _iObj, bool _bFirstTime = false);

};
#endif

//-------------------------------------------------------------------
// Projectile
//-------------------------------------------------------------------
/*
#define CWObject_Projectile_Parent CWObject_Damage
class CWObject_Projectile : public CWObject_Projectile_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	enum
	{
		FLAGS_NODESTROY = 1,
		FLAGS_NOATTACH = 2,
		FLAGS_BOUNCE = 4,
		FLAGS_NODAMAGEONIMPACT = 8,
		FLAGS_EXPLODEONTIMEOUT = 16,
		FLAGS_KEEPLOOPING = 32,
		FLAGS_TRUEOFFSET = 64,
	};

	//int16 m_TrailFreq;
	CStr m_AttachClassWorld;
	CStr m_AttachClassChar;
	bool m_bAttachAligned;
	int m_iAttachAlignedAxis;
	int16 m_AttachOnObjectFlags;

	int16 m_CollisionObjects;

	uint8 m_RemoveAttachedModel; // Bitfield, one bit for each model index to hide/remove when object is attached.
	
	CStr m_TimeOutEffect;
	fp32 m_TimeOutEffectFrontOffset;
	fp32 m_TimeOutEffectInheritVelocity;

	CStr m_Explosion;
	CStr m_ExplosionUW;
	//CStr m_Trail;
	//CStr m_TrailUW;
	uint8 m_ExplosionType;
	uint8 m_ExplosionAlignAxis;
	uint8 m_PendingDeath;
	int m_Flags;
	int m_bHit : 1;

	CWO_Shockwave m_Shockwave;
	fp32 m_ImpactForce;				// How much push a hit object recieves
	
	fp32 m_RenderOffset_MaxBlendTime;

	fp32 m_MinVelocity, m_MaxVelocity;
	fp32 m_Velocity;
	CVec3Dfp32 m_RotVelocity;

	// Mondelore: What is this?! (har iofs inte orkat kolla själv =)
	int16 m_AttachObjType;
	int m_iAttachObject;

	int m_iSoundHit;
	int m_iSoundHitWorld;
	int m_iSoundHitChar;

//	static CVec3Dfp32 GetHandsDistance(CWObject_CoreData* _pObj, fp32 _Time);

	virtual void OnCreate();
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
//	virtual aint OnMessage(const CWObject_Message &_Msg);
//	virtual void OnSpawnTrail();
	virtual void OnRefreshAttached();
	virtual void OnRefresh();
	virtual int OnPreIntersection(int _iObject, CCollisionInfo *_pInfo);
//	virtual int OnSummonInit(const class CSummonInit &);
	virtual void OnRefreshVelocity() {}

	virtual void OnTimeout();

	virtual void PlayHitSounds(CCollisionInfo* _pCInfo, bool _bScaleByVelocity);
	virtual void StopLoopingSounds();

	virtual void Attach(CCollisionInfo* _pCInfo);
	virtual void Explode(int _iObject, CVec3Dfp32 _Pos, CCollisionInfo *_pInfo, bool _bDestroy = true);
	virtual bool TraceRay(CVec3Dfp32 _Pos1, CVec3Dfp32 _Pos2, CCollisionInfo* _pCInfo, int _CollisionObjects = 0, bool _bExcludeOwner = true, bool _bDebug = false);

	virtual const char* GetExplosion(CVec3Dfp32 _Pos);

	static CVec3Dfp32 GetRenderOffset(CWObject_Client* _pObj, fp32 _AnimTime);
	
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);

};

//-------------------------------------------------------------------
// Projectile_Tracer
//-------------------------------------------------------------------
class CWObject_Projectile_Tracer : public CWObject_Projectile
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	int m_TraceObjFlags;

	CWObject_Projectile_Tracer();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual bool OnTracerIntersection(CCollisionInfo* _pCInfo); // Return true if object should "explode".
	virtual void OnRefresh();
};

//-------------------------------------------------------------------

#define CWObject_Projectile_Bouncer_Parent CWObject_Projectile_Tracer
class CWObject_Projectile_Bouncer: public CWObject_Projectile_Bouncer_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:

	int16		m_AbsorbingObjects;
	bool		m_bExcludeOwner;
	int			m_NumBouncesLeft;
	fp32			m_BounceAlignmentLimit;
	CStr		m_BounceEffect;
	fp32			m_BounceEffectOffset;
	CWO_Damage	m_BounceDamage;
	fp32			m_BounceImpactForce;
	int			m_iBounceSound;
	fp32			m_BounceElasticy;
	fp32			m_BounceElasticyVerticalFactor;
	fp32			m_BounceFriction;
	fp32			m_Gravity;

	virtual void OnCreate();
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual void OnRefresh();
	virtual void OnRefreshVelocity();
	virtual bool OnTracerIntersection(CCollisionInfo* _pCInfo); // Return true if object should "explode".
	virtual int OnPreIntersection(int _iObject, CCollisionInfo *_pCInfo);
	static int OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo = NULL);
};

//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Projectile_Bounce
//-------------------------------------------------------------------
class CWObject_Projectile_Bounce : public CWObject_Projectile
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	int16 m_nBounce;
	CStr m_Sound_Bounce;
	CStr m_Sound_Stuck;
	fp32 m_Elasticity;
	fp32 m_BounceAngle;

	virtual void OnCreate();
	virtual void OnRefresh();
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual int OnPreIntersection(int _iObject, CCollisionInfo *_pInfo);
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
};

//-------------------------------------------------------------------
// Projectile_Instant
//-------------------------------------------------------------------

#define CWObject_Projectile_Instant_Parent CWObject_Projectile
class CWObject_Projectile_Instant : public CWObject_Projectile_Instant_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:

	int m_iSoundLRP;

	fp32 m_MoveOut;
	fp32 m_StartOffset;
	fp32 m_VelocityOffsetFactor;

	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};
*/
//-------------------------------------------------------------------
// AttachModel
//-------------------------------------------------------------------

#ifndef M_DISABLE_TODELETE

#define CWObject_AttachModel_Parent CWObject_Damage
class CWObject_AttachModel : public CWObject_AttachModel_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:

	static const char* ms_lpAttachTypeStr[];
	enum 
	{
		AttachType_Harmless	= 0,
		AttachType_Painfull	= 1,
		AttachType_Leathal	= 2,
	};

protected:

	int m_iAttachType;
	bool m_bAllowAttachedOnly;

	fp32 m_DamageDeliveredDelay;

public:

	void Attach(int _iParentObj = 0, int _iBone = 0, CMat4Dfp32* _pWorldToBone = NULL, bool _bGenerateWTB = true);
	void Attach(CCollisionInfo& _CInfo, bool _bForceAligned = false, int _iAlignAxis = 0);
	static int CreateAndAttach(const char* _Class, CWorld_Server* _pWServer, int _iOwner, CMat4Dfp32& _Matrix, int _iParentObj = 0, int _iBone = 0, CMat4Dfp32* _pWorldToBone = NULL, bool _bGenerateWTB = true);
	static int CreateAndAttach(const char* _Class, CWorld_Server* _pWServer, int _iOwner, CMat4Dfp32& _Matrix, CCollisionInfo& _CInfo, bool _bForceAligned = false, int _iAlignAxis = 0);

	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual void OnRefresh();

	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);

};
#endif
/*
// -------------------------------------------------------------------
//  CWObject_Projectile_SkelAnim
// -------------------------------------------------------------------

#define CWObject_Projectile_Homing_Parent CWObject_Projectile_Tracer
class CWObject_Projectile_Homing: public CWObject_Projectile_Homing_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:

	enum HOMINGMODE
	{
		HOMINGMODE_NOTARGET,
		HOMINGMODE_TARGETPOS,
		HOMINGMODE_TARGETOBJ,
	};
	
	fp32			m_Homing_MinVelocity;
	fp32			m_Homing_MaxVelocity;
	fp32			m_Homing_Acceleration;
	bool		m_Homing_bTurning;
	fp32			m_Homing_StartTurnDistance;
	fp32			m_Homing_EndTurnDistance;
	fp32			m_Homing_ContraTurnHelp;
	fp32			m_Homing_TurnAroundDistance;
	fp32			m_Homing_Banking;
	fp32			m_Homing_MaxBanking;
	fp32			m_Homing_MaxPitch;
	fp32			m_Homing_BankReduction;
	fp32			m_Homing_PitchReduction;

	bool		m_Homing_bPitchControl;
	bool		m_Homing_bBankControl;

	int			m_Homing_TargetObjects;
	bool		m_Homing_bAutoSelectTarget;

	HOMINGMODE	m_Homing_Mode;
	CVec3Dfp32	m_Homing_TargetPos;
	int			m_Homing_TargetObjGUID;

	int			m_Homing_StartDelayTicks;
	int			m_Homing_StartFadeTicks;
	
public:

	bool SetTargetPos(CVec3Dfp32& _TargetPos);
	bool SetTargetObj(int _iTargetObj);

	int FindTarget();

	virtual void OnCreate();
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
};
*/
// -------------------------------------------------------------------
//  CSkeletonAnimated
// -------------------------------------------------------------------

class CSkeletonAnimated
{

public:

	static void OnCreate(CWObject* _pObj);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	static void OnEvalKey(CWObject* _pObj, CWorld_Server *_pWServer, const CRegistry* _pKey);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg, bool& _bHandled);

};

// -------------------------------------------------------------------
//  CWObject_Projectile_SkelAnim
// -------------------------------------------------------------------
/*
#define CWObject_Projectile_HomingSkelAnim_Parent CWObject_Projectile_Homing
class CWObject_Projectile_HomingSkelAnim : public CWObject_Projectile_HomingSkelAnim_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:

	virtual void OnCreate();
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);

};*/

#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  CWObject_Ext_ModelEmitter
// -------------------------------------------------------------------
class CWObject_Ext_ModelEmitter : public CWObject_Ext_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	fp32 m_Freq;
	fp32 m_MinDuration;

	CWObject_Ext_ModelEmitter();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	void OnRefresh();
};
#endif

#ifndef M_DISABLE_CURRENTPROJECT
// -------------------------------------------------------------------
//  CWObject_ClientModel
// -------------------------------------------------------------------
class CWObject_ClientModel : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	static void OnClientExecute(CWObject_ClientExecute *_pObj, CWorld_Client *_pWClient)
	{
		CWObject_Model::OnClientExecute(_pObj, _pWClient);

		if(_pObj->m_iAnim2 > 0 && _pObj->m_iAnim2 <= _pObj->GetAnimTick(_pWClient))
			_pWClient->Object_Destroy(_pObj->m_iObject);
	}
};

// -------------------------------------------------------------------
//  CWObject_ClientModel_Ext
// -------------------------------------------------------------------
class CWObject_ClientModel_Ext : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	static void OnClientExecute(CWObject_ClientExecute *_pObj, CWorld_Client *_pWClient)
	{
		CWObject_Model::OnClientExecute(_pObj, _pWClient);

		if(_pObj->m_iAnim2 > 0 && _pObj->m_iAnim2 <= _pObj->GetAnimTick(_pWClient))
			_pWClient->Object_Destroy(_pObj->m_iObject);
	}
};
#endif

#endif
