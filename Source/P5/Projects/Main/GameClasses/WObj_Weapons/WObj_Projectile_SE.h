#ifndef __WOBJ_PROJECTIL
#define __WOBJ_PROJECTILE_SE_H

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Damage.h"
#include "../WObj_AI/AI_Auxiliary.h"	// TSimpleDynamicList<int16> m_lHitList;
#include "../WObj_Char.h"
#include "WObj_Spells.h"				// CWObject_Damage

enum
{
	IMPACTMODEL_UNDEFINED = 0,
	IMPACTMODEL_METAL,
	IMPACTMODEL_CONCRETE,
	IMPACTMODEL_WOOD,
	IMPACTMODEL_GLASS,
	IMPACTMODEL_PLASTIC,
	IMPACTMODEL_PLASTER,
	IMPACTMODEL_CLOTH,
	IMPACTMODEL_FLESH,
	IMPACTMODEL_DEADFLESH,
	IMPACTMODEL_CARDBOARD,
	IMPACTMODEL_GROUND,
	IMPACTMODEL_ROCK,
	IMPACTMODEL_ASPHALT,
	IMPACTMODEL_RUBBER,
	IMPACTMODEL_PORCELAIN,
	IMPACTMODEL_METAL_RICOCHET,
	IMPACTMODEL_WATER,
	NUM_IMPACTMODELS,

	NETMSG_PCLU_CREATECLIENTOBJECT = 0x40,
	NETMSG_PCLU_GROWPROJECTILEARRAY,
	NETMSG_PCLU_PROJECTILEINSTANCE_INIT,
	NETMSG_PCLU_PROJECTILEINSTANCE_SETIMPACTTIME,
	NETMSG_PCLU_PROJECTILEINSTANCE_SENDMODELCLUSTER,
	NETMSG_PCLU_PROJECTILEINSTANCE_SETNEWVELOCITY,
	NETMSG_PCLU_PROJECTILEINSTANCE_SETNEWDIRECTION,

	IMPACTSTATE_IMPACTPENDING = 0,	// Argh, lack of imagination. :) When this is the state there hasn't been any impact for this projectile yet
	IMPACTSTATE_IMPACTTHISFRAME,	// In this state it will be an impact this frame. Check m_ImpactTime to know when
	IMPACTSTATE_IMPACTHASHAPPENED,	// Impact happened any earlier frame

	PCLU_LW_IMPACTMODEL			= M_Bit(0),
	PCLU_LW_IMPACTSOUND			= M_Bit(1),
	PCLU_LW_IMPACTWALLMARK		= M_Bit(2),
	PCLU_LW_RICOCHETSOUND		= M_Bit(3),
	PCLU_LW_LRPSOUND			= M_Bit(4),
	PCLU_LW_DAMAGE				= M_Bit(5),
	PCLU_LW_IMPACTLIGHT			= M_Bit(6),
	PCLU_LW_USEHITLIST			= M_Bit(7),
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWO_ProjectileInstance

	Comments:		Instance of a projectile. Contains all
					instance-specific data for a projectile.

\*____________________________________________________________________*/

class CWO_ModelCluster
{
public:
	struct CWallmarkInfo
	{
		int16		m_iSurface;
		int8		m_Size;
		CVec2Dfp32	m_Distance;
	};

	TThinArray<int16> m_lModels;
	TThinArray<CWallmarkInfo> m_lWallmarks;
	uint8 m_iExtension;

	CWO_ModelCluster();
	CWO_ModelCluster(const CWO_ModelCluster& _o);
	CWO_ModelCluster& operator=(const CWO_ModelCluster& _o);

	void AddModel(int16 _iModel);
/*	void Write(CCFile* _pFile);
	void Read(CCFile* _pFile);*/
};

enum
{
	PROJECTILE_FLAGS_ALIVE		= M_Bit(0),
	PROJECTILE_FLAGS_RICOCHET	= M_Bit(1),
	PROJECTILE_FLAGS_HEADSHOT	= M_Bit(2),
};

class CWObject_ProjectileCluster;
class CWO_ProjectileInstance;

class CPClu_PTCollision		// CProjectileCluster_PassThroughCollision
{
public:
	CPClu_PTCollision();
	CPClu_PTCollision(CWObject_ProjectileCluster* _pCluster, int _iProjectile, bool _bHasHit);
	~CPClu_PTCollision() {}

	void Clear();

	bool IsValidProjectile();
	bool IsValidGroup();
	bool IsValidMaterial();
	bool IsValidSurface();
	bool IsValidCollInfo();
	bool IsValidCluster();
	bool IsCollidingImpact();
	bool& IsValidCollision();

	const CVec3Dfp32& GetPosition();
	const CVec3Dfp32& GetDirection();

	void SetupLine(const CVec3Dfp32& _Pos1, const CVec3Dfp32& _Pos2);

	bool IntersectBoundBox(const CBox3Dfp32& _WBoundBox, bool _Store = true);
	bool IntersectOBB(const COBBfp32& _WOBoundBox, bool _Store = true);
	bool IntersectPlane(const CPlane3Dfp32& _Plane);
	bool IsInfrontOfCollision();

	void ResolveSurfaceMaterial(CWorld_Server* _pWServer);
	void ResolveSurfaceGroup();

	CWObject_ProjectileCluster*	m_pCluster;			// The projectile cluster
	CWO_ProjectileInstance*		m_pProjectile;		// The projectile itself
	CCollisionInfo*				m_pCInfo;			// Collision info
	bool						m_bHasHit;			// Does collision info contain a hit
	bool						m_bCollision;		// Do we have a valid collision
	int							m_iProjectile;		// Projectile number
	CVec3Dfp32					m_Pos1;				// First point of line
	CVec3Dfp32					m_Pos2;				// Second point of line
	CVec3Dfp32					m_Position;			// Position at collision
	CVec3Dfp32					m_Direction;		// Direction of projectile
	fp32							m_Distance;			// Distance to collision
	int32						m_iGroup;			// Group the material belongs to (major group of material type)
	int32						m_iMaterial;		// Material type (wood, metal, plastic etc.)
	CStr						m_SurfaceName;		// Name of surface
	//int							m_SurfaceID;		// Surface ID
	CXW_Surface*				m_pSurface;			// Surface type from resolv
};

class CWO_ProjectileInstance
{
public:
	CMat4Dfp32 m_Position;						// Current position
	CVec3Dfp32 m_Velocity;						// Velocity of the projectile

	int32 m_PendingDeath;						// Is this projectile pending for a bending? -1 say it's not, 0 say it will die this frame and >0 say it most certainly is!
	int32 m_RandSeed;							// Rand seed for this projectile
	int32 m_RandSeedTime;						// Rand seed based on spawn time for this projectile
	fp32 m_TravelDistance;						// How far has the projectile traveled in total

	uint32 m_SpawnGroup : 16;					// Spawn group
	uint32 m_Flags : 8;							// NOTE! Projectile is considered being alive after impact, this is because it is still being rendered, be it the actual projectile or impact model.
	uint32 m_Properties : 8;					// Flag field of properties for projectile

	int16 m_iObject;							// The object this projectile hit

	CWO_ProjectileInstance();

	void OnDeltaLoad(CCFile* _pFile, int _Flags);
	void OnDeltaSave(CCFile* _pFile);
};

class CWO_ProjectileInstance_Client
{
public:
	CMat4Dfp32 m_Position;
	CMat4Dfp32 m_SpawnPos;
	CVec3Dfp32 m_Velocity;
	CMat4Dfp32 m_ImpactPos;
	CWO_ModelCluster m_ProjectileModelCluster;
	CWO_ModelCluster m_ImpactModelCluster;

	int32 m_SpawnTick;
	CMTime m_PreviousRenderTime;
	fp32 m_ImpactTimeFrac;
	int32 m_ImpactState;
	int m_ImpactTick;
	CVec3Dfp32 m_ImpactNormal;

	int32 m_RandSeed;
	int32 m_RenderTicks;

	uint32 m_ImpactLightColor;
	uint16 m_ImpactLightRange;

	uint8 m_Properties;

	CWO_ProjectileInstance_Client();

	bool GetPositionMatrix(fp32 _IPTime, CMat4Dfp32 &_Mat, CMat4Dfp32 &_ImpactMat);
};

class CWO_ProjectileCluster_Client : public CReferenceCount
{
public:
	int32 m_nProjectiles;
//	int32 m_SpawnTick;
	int32 m_RenderTimeout;
	fp32 m_DistributionWidth;
	fp32 m_DistributionHeight;
	fp32 m_ProjectileVelocity;
	fp32 m_ProjectileVelocityVariation;
	TThinArray<CWO_ProjectileInstance_Client> m_lProjectileInstance_Client;
};

class CWO_ProjectileCluster_DamageRange
{
public:
	fp32 m_RangeBegin;
	fp32 m_RangeEnd;
	int m_Damage;
	uint8 m_nAllowedHits;
	CWO_ProjectileCluster_DamageRange* m_pNextRange;

	CWO_ProjectileCluster_DamageRange();
	bool IsInRange(fp32 _Pos);
	bool IsFallback();
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWObject_ProjectileCluster

	Comments:		A cluster of projectiles
\*____________________________________________________________________*/

class CWObject_ProjectileCluster : public CWObject_Damage
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	int8 m_nProjectiles;			// Number of projectiles in this cluster. No more than 256 or the clientmessages will break
	fp32 m_DistributionWidth;		// How many width units per 32 length units of individual distribution the projectile has
	fp32 m_DistributionHeight;		// How many height units per 32 length units of individual distribution of the projectile has

	uint16 m_ImpactSound;			// General impact sound
	uint16 m_ImpactFleshSound;		// Impact sound when hitting flesh
	uint16 m_RicochetSound;			// Ricochet 
	uint16 m_LRPSound;				// LRP sound
	CStr m_BounceSpawnObject;		// If a bullet is "bounced" (from angelus) this is what is spawned in it's place

	uint16 m_SpawnGroup;

	// Leightweight projectiles, (these are normal projectiles, except they don't necesarily have to spawn wallmarks impact effects etc.)
	struct SLeightweightProjectile
	{
		int8	m_nProjectiles;
		uint8	m_Flags;
	};
	SLeightweightProjectile m_LeightweightProjectiles;

	CWO_ModelCluster m_ImpactModelCluster[NUM_IMPACTMODELS];	// List of modelclusters to be rendered at impact depending on type of impact material (metal, concrete etc..)
	TThinArray<CWO_ModelCluster> m_lProjectileModelCluster;			// A list of differenct projectile model clusters so we can have some variations in projectile apperance

	int32 m_iFirstEmptyProjectile;
	int32 m_InitialProjectilePoolSize;
	int32 m_RenderTimeout;
	TSimpleDynamicList<int16> m_lHitList;
	int m_RandSeedBase;

	// Default light and color range of impact light
	uint32 m_ImpactLightDefaultColor;
	uint16 m_ImpactLightDefaultRange;

	// Impact/ricochet sound
	int32 m_ImpactSoundTick;
	bool m_bCanBeRemoved;
	
	CWO_ProjectileCluster_DamageRange* m_pFirstDamageRange;

protected:
	fp32 m_ProjectileVelocity;		// Velocity of each projectile
	fp32 m_ProjectileVelocityVariation;	// Variation velocity for multi
	fp32 m_ProjectileAcceleration;	// Acceleration of each projectile. This does not seem to be used...
	fp32 m_ProjectileGravity;	// Additional downwards acceleration of each projectile
	bool m_bFirstOnRefresh;
	TThinArray<CWO_ProjectileInstance> m_lProjectileInstance;	// If the projectilecluster contains more than one projectile we can access them here

	int ResolveDamage(fp32 _Range);	// Check how much damage we rally should give

public:
	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual void NetMsgSendModelCluster(int _iProjectile, const CWO_ModelCluster& _ModelCluster, int _Target);
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);

	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);

	void EvalDamageRange(const CRegistry* _pKey);
	int EvalMaterial(const char* _pStr);
	void EvalImpactModelKey(const CRegistry* _pKey);
	void EvalWallmarkKey(const CRegistry* _pKey);
	void AddProjectileModel(const CRegistry* _pKey);
	void SetInitialPoolSize(const CRegistry* _pKey);
	void SetRenderTimeout(const CRegistry* _pKey);
	void GetRandomProjectileModel(CWO_ModelCluster& _ModelCluster);

	bool TraceRay(const int& _iProjectile, CVec3Dfp32 _Pos1, CVec3Dfp32 _Pos2, CCollisionInfo* _pCInfo, int _CollisionObjects = 0, bool _bExcludeOwner = true, bool _bDebug = false);
	int ResolveDamageRange(fp32 _Range);

	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, const CMat4Dfp32& _ParentMat);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

	void InitProjectileArray(int _StartSize);
	int FindEmptyProjectileIndex();
	CWO_ProjectileInstance *SpawnProjectile(const CMat4Dfp32& _Mat, CWO_ProjectileInstance *_pRicochetFrom = NULL, const uint8 _Properties = 0xFF);
	void SpawnProjectileCluster(const CMat4Dfp32& _Mat);
	void TraceProjectile(int _iProjectile);

	bool IsValidHit(uint _iProjectile, uint& _nHeadHits);
	uint GetGroup(CCollisionInfo* _pCInfo);
	void NetMsgSendImpactTime(uint16 _iProjectile, uint8 _iGroup, const CCollisionInfo* _pCInfo, const CVec3Dfp32& _Pos, int16 _iObject, uint _Properties);
	void AddVisBoundBoxPos(const CVec3Dfp32& _Pos);
		
	virtual void OnImpact(int _iProjectile, const CVec3Dfp32& _Pos, uint16 _iObject = 0, CCollisionInfo* _pCInfo = NULL, bool _bNoDamage = false);
	virtual void OnPassThroughImpact(CPClu_PTCollision& _PTCollision);
};


class CWObject_ClientImpactModel : public CWObject_Ext_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	static void OnClientExecute(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:			CWObject_ProjectileCluster_ImpactSpawn

Comments:		A cluster of projectiles that will summon another 
				world object on impact.
\*____________________________________________________________________*/
class CWObject_ProjectileCluster_ImpactSpawn : public CWObject_ProjectileCluster
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	CStr m_ImpactSpawn;

public:
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnImpact(int _iProjectile, const CVec3Dfp32& _Pos, uint16 _iObject = 0, CCollisionInfo* _pCInfo = NULL, bool _bNoDamage = false);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
};
#endif // __WOBJ_PROJECTILE_SE_H
