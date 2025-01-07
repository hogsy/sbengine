/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Object.h

	Author:			Olle Rosenquist, Anton Ragnarsson

	Copyright:		2004 Starbreeze Studios AB

	Contents:		CWObject_Object, CWObject_Effect

	Comments:		

	History:
		041110:		Created File
		050112:		Added simple script support (OnDamage / OnBreak).
		050608:		Added basic support for destructables + temp physics.
		050614:		Added support for rigid body dynamics-engine.
		060110:		Added support for playing animations.
\*____________________________________________________________________________________________*/

#ifndef _INC_WOBJ_OBJECT
#define _INC_WOBJ_OBJECT

#include "../WObj_Messages.h"
#include "../WNameHash.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMessageContainer
|
| Handles multiple CWO_SimpleMessages
|   (TODO: use/extend the CWO_SimpleMessageContainer?)
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CMessageContainer
{
public:
	class Elem : public CWO_SimpleMessage
	{
	public:
		int16 m_nHitpoints;
	};
	TArray<Elem> m_lMessages;

public:
	void Add(uint _iSlot, CStr _Msg, CWObject& _Obj);

	void Precache(CWObject& _Obj);
	void Send(uint _iSender, CWObject& _Obj, uint _nHitpoints = 0);

	CMessageContainer& operator=(const CMessageContainer& _x);
	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_SoundGroupManager
|
| TODO: Move somewhere else
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_SoundGroupManager
{
public:
	enum
	{
		SOUND_IMPACT_SOFT =			0,
		SOUND_IMPACT_MEDIUM =		1,
		SOUND_IMPACT_HARD =			2,
		SOUND_IMPACT_PROJECTILE =	3,
		SOUND_SLIDE =				4,
		SOUND_BREAK =				5,
		SOUND_DESTROY =				6,
		NUM_SOUNDS,

		PARAM_THRESHOLD_SOFT =		0,
		PARAM_THRESHOLD_MEDIUM =	1,
		PARAM_THRESHOLD_HARD =		2,
		PARAM_VOLUME_OFFSET =		3,
		PARAM_VOLUME_SCALE =		4,
		NUM_PARAMS,
	};

	class CSoundGroup
	{
	public:
		CNameHash m_Name;
		fp32      m_aParams[NUM_PARAMS];
		int16     m_aiSounds[NUM_SOUNDS];
	};

private:
	fp32 m_aDefaultParams[NUM_PARAMS];
	TArray<CSoundGroup> m_lSoundsGroups; //TODO: do a hash->elem map

public:
	void InitWorld(CWorld_Server& _Server);
	void CloseWorld();
	int GetSoundGroup(const char* _pName, CWorld_Server& _Server);

	int16 GetSound(uint _iSoundGroup, uint _iSlot) const;
	bool GetImpactSound(uint _iSoundGroup, fp32 _Impact, uint* _iSlot, int16* _iSound, fp32* _pVolume) const;
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object messages   (code only. for script messages, see WObj_System.h)
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	OBJMSG_OBJECT_APPLYFORCE = OBJMSGBASE_MISC_OBJECT,
	OBJMSG_OBJECT_APPLYVELOCITY,
	OBJMSG_OBJECT_FIND_DEMONARMATTACHPOINT, 
	OBJMSG_OBJECT_DEMONARM_GRABBED,
	OBJMSG_OBJECT_DEMONARM_RELEASED,
	OBJMSG_OBJECT_GET_GRABATTACHPOINTS,
	OBJMSG_OBJECT_ISBREAKING,
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Object_ClientData
|
| NOTE: This is to be kept as lean as possible.
|       There can be lots os 'Object' entities in the world, so be careful.
|       Also, this ClientData is only allocated for objects that actually need it,
|       so one cannot assume it always exists.
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_Object_ClientData : public CReferenceCount, public CAutoVarContainer
{
public:
	AUTOVAR_SETCLASS(CWO_Object_ClientData, CAutoVarContainer);

	// m_liJointObjects is used to build up a skeleton instance from a group of physics
	// objects. The owning object will have links to all objects that's part of the system,
	// and it is the one rendering the skeleton.
	// the uint32 contains [ ObjectIndex | JointIndex ]
	CAUTOVAR(TAutoVar_ThinArray_(uint32),	m_liJointObjects,	DIRTYMASK_1_0);
	CAUTOVAR(CAutoVar_Burnable,				m_Burnable,			DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32,		m_AimOffset,		DIRTYMASK_1_2);
	CAUTOVAR_OP(CAutoVar_CMTime,			m_Anim_StartTime,	DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_CMat4Dfp32,		m_Anim_BaseMat,		DIRTYMASK_1_4);

	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_liJointObjects)
	AUTOVAR_PACK_VAR(m_Burnable)
	AUTOVAR_PACK_VAR(m_AimOffset)
	AUTOVAR_PACK_VAR(m_Anim_StartTime)
	AUTOVAR_PACK_VAR(m_Anim_BaseMat)
	AUTOVAR_PACK_END

	// Client only
	TThinArray<CBox3Dfp32>	m_lPhysBoundBox;
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object
|
| Data[0] - Visibility mask
| Data[3] - Model flags
| Data[5] - Index of connected light
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Object : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_Model parent;

	friend class CWObject_Object_Constraint;

public:
	enum
	{ // init parameters (from Ogier, cannot be changed)
		FLAGS_NOPHYSICS =			M_Bit(0),	// Things will not collide with this object
		FLAGS_ANIMPHYS =			M_Bit(1),	// Used to turn off collision during behaviour animations
		FLAGS_WAITSPAWN =			M_Bit(2),	// Object is not spawned until spawn message arrives
		FLAGS_GAMEPHYSICS =			M_Bit(3),	// Object is controlled by physics engine
		FLAGS_BREAKCONSTRAINTS =	M_Bit(4),	// Auto-break connected constraints when object breaks
		FLAGS_SPAWNACTIVE =			M_Bit(5),	// Object will not start as 'stationary'
		FLAGS_DEBRIS =				M_Bit(6),	// Object is not important (visual extras...)
		FLAGS_NAVIGATION =			M_Bit(7),	// Object should update navgrid
		FLAGS_TRIMESHCOLLISION =	M_Bit(8),    // Use trimesh for projectile collisions
		FLAGS_LAMP =				M_Bit(9),	// Sub-part will become a lamp and inherit lampdata
		FLAGS_MODELSHADOW =			M_Bit(10),	// (lamp) Should cast shadow from the attached light?
		FLAGS_AUTOAIM		 =		M_Bit(11),	// Object will have autoaim
	// runtime flags (can be changed)
		FLAGS_FAILEDSETPHYSICS =	M_Bit(12),
		FLAGS_BROKEN =				M_Bit(13),
		FLAGS_PARENTNAMEHASH =		M_Bit(14),	// (legacy) Indicates that m_iParentObject contains a name-hash instead of an object ID
		FLAGS_SAVEPARTS =			M_Bit(15),	// Indicates whether m_Root should be saved or not (OnDeltaSave)
		FLAGS_FRAGILE =				M_Bit(16),	// Indicates that the object is made up of several parts connected with non-rigid constraints
		FLAGS_ISBREAKING =			M_Bit(17),	// Used to delay separation until all breaking of internal objects is done
		FLAGS_SAVELIFETIME =		M_Bit(18),	// Indicates whether m_LifeTimeEnd should be saved or not (OnDeltaSave)
		FLAGS_SAVEPOSITION =		M_Bit(19),	// Indicates whether position should be saved or not (OnDeltaSave)
		FLAGS_FROMTEMPLATE =		M_Bit(20),	// Indicates if object is spawned from template, or from another object
		FLAGS_DESTROYED =			M_Bit(21),	// Object is destroyed and will be removed in a few ticks..
		FLAGS_ISMOVING =			M_Bit(22),	// The object has non-zero velocity...
		FLAGS_UPDATECONSTRAINTS =	M_Bit(23),	// Indicates that the object has constraint(s) connected to other object
		FLAGS_ANIM_DELTAMOVE =		M_Bit(24),	// Used when playing animations, to control movement
		FLAGS_BURNABLE =			M_Bit(25),	// Indicates that the object is burnable
		FLAGS_HAS_CLIENTDATA =		M_Bit(26),	// Object has client data (used by I/O)
		FLAGS_IGNORE_DAMAGE =		M_Bit(27),	// Object will ignore damage messages
		FLAGS_HASPARENT	=			M_Bit(28),	// Parent related stuff will be saved
		
		CWO_CLIENTFLAGS_NORENDER	= M_Bit(0) << CWO_CLIENTFLAGS_USERSHIFT,		// Object should be invisible (but burnable is still rendered)
		CWO_CLIENTFLAGS_RENDEREXACT	= M_Bit(1) << CWO_CLIENTFLAGS_USERSHIFT,		// Render object at animated position (ignore real object position)

		CONSTRAINT_TYPE_RIGID =		0,			// Stiff constraint (actually not a real physics engine constraint at all)
		CONSTRAINT_TYPE_BALL =		1,			// Object fixed to world point
		CONSTRAINT_TYPE_AXIS =		2,			// Object fixed to world axis

		CONSTRAINT_TARGET_NONE =	-1,
		CONSTRAINT_TARGET_WORLD =   -2,

		CONSTRAINT_FLAGS_ISVERSION2 = M_Bit(0),

		SOUND_IMPACT_SOFT =			0,
		SOUND_IMPACT_MEDIUM =		1,
		SOUND_IMPACT_HARD =			2,
		SOUND_IMPACT_PROJECTILE =	3,
		SOUND_SLIDE =				4,
		SOUND_BREAK =				5,
		SOUND_DESTROY =				6,
		NUM_IMPACT_SOUNDS,

		DATA_OBJ_VISIBILITYMASK =	0,			// Data[0] - (Object) Visibility mask
		DATA_OBJ_MODELFLAGS =		3,			// Data[3] - (Object) Model flags
		DATA_LAMP_FLAGS =			1,			// Data[1] - (Lamp) lamp flags
		DATA_LAMP_LIGHTINDEX =		5,			// Data[5] - (Lamp) Index of connected light
		DATA_TV_SURFACEID =			2,			// Data[2] - (TV) Surface ID | FlushTextureID
		DATA_TV_SOUNDID =			4,			// Data[4] - (TV) Sound ID
		DATA_TV_ATTENUATION =		6,			// Data[6] - (TV) AttnMin | AttnMax
		DATA_TV_RANGE_VOLUME =		7,			// Data[7] - (TV) ViewMax | Volume

		MODELFLAGS_IMPACT_REFLECT = M_Bit(0),	// 0: XDir will be surface normal     1: XDir will be impact dir reflected in surface normal
		MODELFLAGS_IMPACT_WORLDUP =	M_Bit(1),	// 1: ZDir will be forced to (0,0,1)
		MODELFLAGS_TEMPLATE =		M_Bit(2),	// Indicates that m_iModel actually points to a template resource, not a model
		MODELFLAGS_ATTACHED =		M_Bit(3),	// The model will be attached to the spawning object
	};

	struct SPhysPrim
	{
		CBox3Dfp32 m_BoundBox;
		CVec3Dfp32 m_Origin;
		uint32 m_VisibilityMask;				// Mask to use for rendering and phys solids
		int16 m_iPhysModel;
		int16 m_iPhysModel_Dynamics;	// Low detail phys model, for dynamics engine

		SPhysPrim();
	};

	struct SDamage
	{
		int16 m_nHitPoints;			// hit points left (health)
		int16 m_nMinDamage;			// minimum amount of damage that can be drawn from hit points
		int16 m_nMaxDamage;			// maximum             -         "         -                  in one blow
		int16 m_nMinDamage_Last;	// minimum amount needed for final blow

		SDamage();
		void Parse(CStr _Str);
		bool GiveDamage(int16 _nDamage);
		bool IsDestroyed() const;
		void Write(CCFile* _pF) const;
		void Read(CCFile* _pF);
	};

	struct SBreakableBase
	{
		SDamage				m_DamageInfo;
		CMessageContainer	m_Messages_OnDamage;	// Messages that are sent when the object receives damage:
		CMessageContainer	m_Messages_OnBreak;		// Messages that are sent when the object breaks:

		SBreakableBase& operator=(const SBreakableBase& _x);
		bool OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject& _Obj);
		void OnSpawnWorld(CWObject& _Obj);
		void Write(CCFile* _pF) const;
		void Read(CCFile* _pF);
	};

	struct SPhysBase : SBreakableBase
	{
		CNameHash m_Name;
		SPhysPrim m_PhysPrim;

		SPhysBase& operator=(const SPhysBase& _x);
		bool OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject& _Obj);
		void OnFinishEvalKeys(CWObject& _Obj);
		void Write(CCFile* _pF) const;
		void Read(CCFile* _pF);

		void Merge(const SPhysBase& _Other);
	};

	struct SObjectModel
	{
		CMat4Dfp32 m_LocalPos;
		CNameHash  m_Name;
		int16      m_iModel[3];
		uint16     m_LifeTime : 12; // life time in ticks
		uint16     m_Flags : 4;

		SObjectModel();
		bool OnEvalKey(uint32 _KeyHash, const CRegistry& _Reg, CWObject& _Obj);
		void Relocate(const CMat4Dfp32& _ParentInv);
		void Write(CCFile* _pF) const;
		void Read(CCFile* _pF);
	};

	struct SPart;
	struct SConstraint : SObjectModel, SBreakableBase
	{
		int32 m_iObj0;				// local part index. temporary used as NameHash during load
		int32 m_iObj1;
		uint8 m_Type;
		uint8 m_Flags;
		int16 m_iPhysConstraint;	// index of created physics constraint, or -1
		fp16  m_Length;
		fp16  m_MinAngle;
		fp16  m_MaxAngle;
		fp16  m_Friction;
		CVec3Dfp16 m_StartPoint;

		SConstraint& operator=(const SConstraint& _x);
		void SetDefault();
		void SetDefault2();

		bool OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject& _Obj);
		void Write(CCFile* _pF) const;
		void Read(CCFile* _pF);

		static int32  HashToIndex(uint32 _NameHash, const SPart& _Owner, CWObject& _Object);
		static uint32 IndexToHash(int32 _Index, const SPart& _Owner, CWObject& _Object);
	};

	struct STrigger : SPhysBase
	{
		uint32 m_DamageTypes;

		STrigger();
		bool OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject& _Obj);
	};

	struct SSoundInfo
	{
		enum { NUM_SOUNDS = CWO_SoundGroupManager::NUM_SOUNDS };
		int16 m_iSoundGroup;
		uint16 m_anLastPlayed[NUM_SOUNDS];

		bool IsEmpty() const;
		void Write(CCFile* _pF) const;
		void Read(CCFile* _pF);
	};

	struct SPart : SPhysBase
	{
		CMat4Dfp32 m_LocalPos;
		uint32    m_Flags;				// FLAGS_xxx
		uint32    m_RenderMask;			// Mask to use for rendering  (default == PhysPrim.m_VisibilityMask)
		int32     m_Lifetime;			// Len of lifetime (in ticks)
		int16     m_iModel[3];			// Model to use for rendering
		fp16      m_Mass;				// Custom mass [kg]

		TArray<SConstraint>  m_lConstraints;
		TArray<STrigger>     m_lTriggers;
		TArray<SObjectModel> m_lObjectModels;
		TArray<CNameHash>    m_lImpactModels;
		TArray<CNameHash>    m_lDestroyModels;
		TArray<CVec3Dfp32>   m_lAttach_DemonArm;	// attach-points for demon arm
		TArray<CVec3Dfp32>   m_lAttach_Grab;		// attach-points for carrying
		TArray<int16>        m_liJoints;			// Linked to bones in the trimesh skeleton
		SSoundInfo           m_Sounds;
		CMessageContainer    m_Messages_OnMove;		// Messages that are sent when the object starts to move
		CMessageContainer    m_Messages_OnDemonArmGrab;	// Messages that are sent when the object is grabbed by a demon arm
		CMessageContainer    m_Messages_OnDemonArmRelease;	// Messages that are sent when the object is released by a demon arm
		TArray<SPart>        m_lSubParts;

		SPart();
		SPart& operator=(const SPart& _Part);
		void Clear();
		bool OnEvalKey(uint32 _KeyHash, const CRegistry& _Key, CWObject_Object& _Obj);
		void OnFinishEvalKeys(CWObject& _Obj);
		void OnSpawnWorld(CWObject& _Obj);
		void Write(CCFile* _pF) const;
		void Read(CCFile* _pF, CWObject_Object& _Obj);
		void Merge(const SPart& _Other);

		int FindConstraint(uint32 _NameHash) const;
		int FindTrigger(uint32 _NameHash) const;
		int FindPart(uint32 _NameHash) const;
		const SObjectModel* FindModel(uint32 _NameHash, const char* _pVariationName) const;

		void DeleteConstraint(int _iConstraint);
		void DeleteTrigger(int _iTrigger);
		void DeletePart(int _iPart, CWObject_Object& _Obj);
	};

	struct SDelayedBreak
	{
		CNameHash m_Name;
		uint16 m_iSender;
	};

protected:
	CVec3Dfp32 m_ImpactPos;
	CVec3Dfp32 m_ImpactForce;
	int32      m_ImpactSoundTick;
	CStr       m_VariationName;
	uint32     m_SpawnMask;

	SPart m_Root;

	TArray<SDelayedBreak> m_lDelayedBreak; // Queue of things (triggers, constraints, subparts) to break

	uint32 m_iParentObject;		// If this ID is set, the object will be added as a part in the 'parent' object, and then destroyed
	int32  m_LifetimeEnd;		// Absolute point
	uint16 m_PhysFailCount;		// This is used during spawning, if physics can't be set (because of collision)
	int8   m_iImpactPart;
	int32  m_iPhysEP;	//Index to physics driven engine path that is driving this object, if this is != -1 we need to report all collisions to the EP

protected:
	virtual void ObjectSpawn(bool _bSpawn);
	bool CreatePhysState(const SPart& _Data, CWO_PhysicsState& _Result);

	void OnDamage(const CWO_DamageMsg& _Damage, int _iSender, bool _bNoForce = false); //if _bNoForce is true OBJMSG_OBJECT_APPLYFORCE will not be sent
	bool OnDamage(SBreakableBase& _PhysObj, const CWO_DamageMsg& _Damage, int _iSender);
	bool OnDamage(SPart& _Part, const CWO_DamageMsg& _Damage, int _iSender);
	bool OnDamage(STrigger& _Trigger, const CWO_DamageMsg& _Damage, int _iSender);
	bool OnDamage(SConstraint& _Constraint, const CWO_DamageMsg& _Damage, int _iSender);

	void OnDestroy(SBreakableBase& _PhysObj, int _iSender);
	void OnDestroy(STrigger& _Trigger, int _iSender);
	void OnDestroy(SPart& _Part, int _iSender);
	void OnBreak(SConstraint& _Constraint, int _iSender);

	bool Intersect(const SPhysBase& _Phys, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CCollisionInfo& _CollInfo) const;
	bool CheckIfMobile(CWO_PhysicsState& _Phys);
	void CheckSeparation();
	void SpawnModel(const SObjectModel& _Model, const CMat4Dfp32* _pPosMat = NULL);
	void GetImpactMatrix(const SObjectModel& _Model, const CWO_DamageMsg& _Damage, CMat4Dfp32& _Dest) const;
	void UpdateRenderMask();
	void AddDelayedBreak(const CNameHash& _Name, uint _iSender);
	void FlushDelayedBreak();
	void DeleteAllPhysConstraints();
	void DeletePhysConstraint(int _iPhysConstraint);

	virtual void OnCollision(CWObject* _pOther, const CVec3Dfp32& _Pos, fp32 _Impact);

	static void DoRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, CXR_AnimState& _Anim, uint32 _RenderFlags = 0);
	static bool GetAnimLayer(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CMTime& _Time, CXR_AnimLayer& _Dest);
	static CXR_SkeletonInstance* EvalAnim(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Skeleton*& _pSkel);
	static void InitBurnable(CWObject_Client* _pObj, CWorld_Client* _pWClient);

	bool OnRefresh_GamePhysics();
	bool OnRefresh_Animation();
	void OnAnimEvent(const CXR_Anim_DataKey& _Key, fp32 _EventTimeOffset);

	static CWO_Object_ClientData* AllocClientData(CWObject_CoreData* _pObj);
	static CWO_Object_ClientData* GetClientData(CWObject_CoreData* _pObj);
	static const CWO_Object_ClientData* GetClientData(const CWObject_CoreData* _pObj);
	void RefreshDirty();

public:
	void AddPart(const SPart& _Data);
	void AddConstraint(const SConstraint& _Data);
	bool ConnectConstraint(int _iOtherObj, SConstraint& _Constraint);
	void RemoveConstraint(int _iPhysConstraint);
	uint32 GetFlags() const { return m_Root.m_Flags; }

public:
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void OnIncludeClass(CMapData *_pMapData, CWorld_Server*);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server*);

	virtual void OnCreate();
	virtual void OnRefresh();
	virtual void OnDestroy();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual void OnSpawnWorld2();
	virtual aint OnMessage(const CWObject_Message &_Msg);
	virtual int  OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;	// Called whenever a delta-update should be compiled.

	virtual void OnDeltaSave(CCFile* _pFile);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);

	static void  OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static bool  OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo);
	static int   OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo);
	static aint  OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static int   OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);

	static void OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWClient, CCFile* _pFile, CMapData* _pWData, int _Flags);
	static void OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWClient, CCFile* _pFile, CMapData* _pWData, int _Flags);
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object_Constraint
|
| Used to create constraints between "external" Objects
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Object_Constraint : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_Object::SConstraint SConstraint;

protected:
	SConstraint m_Data;
	CNameHash m_SubPart0;
	CNameHash m_SubPart1;

public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnSpawnWorld();
	virtual void OnRefresh();

	static int Spawn(CWorld_Server& _Server, const SConstraint& _Constraint);
};


// dummy class for new constraints
class CWObject_Object_Constraint2 : public CWObject_Object_Constraint 
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	virtual void OnCreate();
};


#define DO_IF(x) (!(x)) ? (void)0 :


#endif
