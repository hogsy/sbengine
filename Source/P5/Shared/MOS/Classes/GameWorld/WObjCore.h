
#include "WPhysState_Hash.h" // Do not try to move this inside the header guards, it will fail -kma

#ifndef __WOBJCORE_H
#define __WOBJCORE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Game-object base class and misc related classes

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWO_PhysicsPrim
					CWO_PhysicsState
					CWO_PhysicsAttrib
					CWObject_Message
					CTargetNameElement
					CVelocityfp32
					CWS_ClientObjInfo
					CWObject_CoreData
\*____________________________________________________________________________________________*/

// -------------------------------------------------------------------
//  CWObject
// -------------------------------------------------------------------
enum
{
	OBJECT_FLAGS_WORLD =				M_Bit(0),
	OBJECT_FLAGS_PHYSMODEL =			M_Bit(1),	// Objects that have a m_iPhysModel
	OBJECT_FLAGS_PHYSOBJECT =			M_Bit(2),	// Ordinary objects, player, monster, stuff
	OBJECT_FLAGS_PICKUP =				M_Bit(3),
	OBJECT_FLAGS_PLAYER =				M_Bit(4),
	OBJECT_FLAGS_CHARACTER =			M_Bit(5),
	OBJECT_FLAGS_PROJECTILE =			M_Bit(6),
	OBJECT_FLAGS_STATIC =				M_Bit(7),	// Stuff that won't go anywere, static lights, sounds, triggers
	OBJECT_FLAGS_CREEPING_DARK =		M_Bit(8),
	OBJECT_FLAGS_SOUND =				M_Bit(9),
	OBJECT_FLAGS_TRIGGER =				M_Bit(10),
	OBJECT_FLAGS_PLAYERPHYSMODEL =		M_Bit(11),	// Low-detail model for player(character) collision. If none exists this flag is set on worldspawn.
	OBJECT_FLAGS_NAVIGATION =			M_Bit(12),	// Object is rendered into the navigation grid.
	OBJECT_FLAGS_WORLDTELEPORT =		M_Bit(13),	// Things that will be teleported to a new world with Trigger_ChangeWorld
	OBJECT_FLAGS_ANIMPHYS =				M_Bit(14),	// Phys that is automatically disabled for characters running specific behaviours
	OBJECT_FLAGS_OBJECT =				M_Bit(15),	// Stuff in the game world that can be used in game-play. Barrels, dumpsters, televisions.. (TODO: find a better name?)
	
	// -------------------------------------------------------------------
	OBJECT_PRIMTYPE_NONE =		0,
	OBJECT_PRIMTYPE_PHYSMODEL =	1,				// Usually a BSP-Model, but for ex. watertile is also a physmodel.
	OBJECT_PRIMTYPE_SPHERE =	2,
	OBJECT_PRIMTYPE_BOX =		3,				// Oriented bounding box. (OBB)
	OBJECT_PRIMTYPE_POINT =		4,
	OBJECT_PRIMTYPE_CAPSULE =	5,

	// -------------------------------------------------------------------
	OBJECT_PHYSFLAGS_ROTATION =				M_Bit(0),	// Use obj-rotation?, else world-aligned primitive.
	OBJECT_PHYSFLAGS_PUSHER =				M_Bit(1),	// Can push
	OBJECT_PHYSFLAGS_PUSHABLE =				M_Bit(2),	// Can be pushed
	OBJECT_PHYSFLAGS_SLIDEABLE =			M_Bit(3),	// Slides against collision surfaces.
	OBJECT_PHYSFLAGS_FRICTION =				M_Bit(4),	// Receives velocity through friction
	OBJECT_PHYSFLAGS_PHYSMOVEMENT =			M_Bit(5),	// Moves around with some kind of physics.
	OBJECT_PHYSFLAGS_ROTFRICTION_SLIDE = 	M_Bit(6),	// Receives rotation through slide-friction
	OBJECT_PHYSFLAGS_ROTFRICTION_ROT = 		M_Bit(7),	// Receives rotation through rotate-friction
	OBJECT_PHYSFLAGS_OFFSET =				M_Bit(8),	// Primitive is offseted by m_Offset from object's center.
	OBJECT_PHYSFLAGS_ROTREFLECT =			M_Bit(9),	// Rotation is reflected when bouncing. (Ignored if OBJECT_PHYSFLAGS_ROTATION is used.)
	OBJECT_PHYSFLAGS_ROTVELREFLECT =		M_Bit(10),	// Rotation-velocity is reflected when bouncing.
	OBJECT_PHYSFLAGS_APPLYROTVEL =			M_Bit(11),	// Rotate object even though the physics is world-aligned.
	OBJECT_PHYSFLAGS_PHYSICSCONTROLLED =	M_Bit(12),	// Object movement is handled by the physics engine (rather than by the object itself)
	OBJECT_PHYSFLAGS_MEDIUMQUERY		=	M_Bit(13),	// 
	OBJECT_PHYSFLAGS_NODYNAMICSUPDATE   =   M_Bit(14),

};

// -------------------------------------------------------------------

template<typename T, int _BitsFraction> class TFixed
{
public:
	enum { Scale = M_Bit(_BitsFraction) };

	T value;

	TFixed() {}
	TFixed(fp32 _value) : value(T(_value * Scale)) {}
	TFixed(const TFixed& _x) : value(_x.value) {}

	TFixed& operator=(fp32 _value) { value = T(_value * Scale); return *this; }
	operator fp32() const { return value * (1.0f / Scale); }
};


#define CWO_MAXPHYSPRIM	3


#define CWO_PHYSICSPRIM_MAXDIM_XY 2047
#define CWO_PHYSICSPRIM_MAXDIM_Z 1023
#define CWO_PHYSICSPRIM_MAXOFS 32767

#define CWO_PHYSICSPRIM_MAXRESOURCEINDEX (M_Bit(13)-1)

class CWO_PhysicsPrim							// 16 bytes
{
public:
	uint16 m_PrimType : 3;						// OBJECT_PRIMTYPE_xxxxx
	uint16 m_iPhysModel : 13;					// => Max resources = 8191
	uint16 m_PhysModelMask;						// Default = 0xffff
	uint16 m_ObjectFlagsMask;					// Default = 0xffff, when this primitive is tested, this mask the appearance of this object's flags when tested against other object's intersection flags. Capishe?
	TFixed<int16, 4> m_Offset[3];
	uint32 m_DimX : 11;
	uint32 m_DimY : 11;
	uint32 m_DimZ : 10;


	CWO_PhysicsPrim() { Clear(); };
	CWO_PhysicsPrim(int _PrimType, int _iPhysModel, const CVec3Dfp32& _Dim, const CVec3Dfp32& _Offset, uint _PhysModelMask = 0xffff, uint _ObjectFlagsMask = 0xffff);
	void Create(int _PrimType, int _iPhysModel, const CVec3Dfp32& _Dim, const CVec3Dfp32& _Offset, uint _PhysModelMask = 0xffff, uint _ObjectFlagsMask = 0xffff);

	void Clear()
	{
		m_PrimType = OBJECT_PRIMTYPE_NONE;
		m_iPhysModel = 0;
		m_PhysModelMask = 0xffff;
		m_ObjectFlagsMask = 0xffff;
		m_DimX = m_DimY = m_DimZ = 0;
		m_Offset[0] = m_Offset[1] = m_Offset[2] = 0;
//		m_Mass = 1.0f;
	}

	void SetDim(const CVec3Dfp32& _v)
	{
		m_DimX = uint16(_v.k[0]);
		m_DimY = uint16(_v.k[1]);
		m_DimZ = uint16(_v.k[2]);
	}

	void SetOffset(const CVec3Dfp32& _v)
	{
		m_Offset[0] = _v.k[0];
		m_Offset[1] = _v.k[1];
		m_Offset[2] = _v.k[2];
	}

	CVec3Dfp32 GetDim() const
	{
		return CVec3Dfp32(m_DimX, m_DimY, m_DimZ);
	}

	fp32 GetRadius() const
	{
		return fp32(m_DimX);
	}

	CVec3Dfp32 GetOffset() const
	{
		return CVec3Dfp32(m_Offset[0], m_Offset[1], m_Offset[2]);
	}

	void OnLoad(CCFile* _pFile, class CMapData* _pWData);
	void OnSave(CCFile* _pFile, CMapData* _pWData) const;
	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;

	int Compare(const CWO_PhysicsPrim& _Prim) const;		// NOTE: This compare function is not valid for sorting.

	uint8* Write(uint8* _pData) const;
	const uint8* Read(const uint8* _pData);

	CStr Dump(int _DumpFlags) const;
};


class CWO_PhysicsState : public CReferenceCount
{
public:
	CWO_PhysicsPrim m_Prim[CWO_MAXPHYSPRIM];	// 3*16 = 48 bytes
	uint8 m_nPrim;								// Number of primitives used in the m_Prim array.					Only 8-bit is replicated
	uint8 m_NavGridFillValue;
	uint16 m_PhysFlags;							// Misc flags as defined above.
	uint16 m_MediumFlags;						// Mediums this object can't be inside.
	uint16 m_iExclude;							// Object to exclude from collision test. (typically the owner of a projectile.)
	uint16 m_ObjectFlags;						// Object type(s).
	uint16 m_ObjectIntersectFlags;				// Object types this object can't be inside.
	
	mutable uint16 m_RigidBodyID;				// Needed for Broad Phase early out

	// Model instances
	CXR_ModelInstance* m_pModelInstance;

	fp32 m_MoveSubdivisionSize;

	void operator= (const CWO_PhysicsState& _Phys);

	void Clear();

	CWO_PhysicsState();
	CWO_PhysicsState(const CWO_PhysicsState& _Phys);
	CWO_PhysicsState(int _PrimType, int _PhysFlags, int _MediumFlags, const CVec3Dfp32& _Dimensions, int _ObjFlags, int _ObjIntersectFlags, int _iPhysModel = -1);
	CWO_PhysicsState(int _PhysFlags, int _MediumFlags, int _ObjFlags, int _ObjIntersectFlags);
	void AddPhysicsPrim(int _iPrim, const CWO_PhysicsPrim& _Prim);
	fp32 GetMoveSubdivisionSize() const;

	void OnLoad(CCFile* _pFile, class CMapData* _pWData);
	void OnSave(CCFile* _pFile, class CMapData* _pWData) const;

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;

	int CompareCoreData(const CWO_PhysicsState& _State) const;

	bool CanIntersect(const CWO_PhysicsState& _State) const;

	uint8* Write(uint8* _pData) const;
	const uint8* Read(const uint8* _pData);

	CStr Dump(int _DumpFlags) const;

	
};

typedef TPtr<CWO_PhysicsState> spCWO_PhysicsState;

// -------------------------------------------------------------------
//  CWO_PhysicsAttrib
// -------------------------------------------------------------------
class CWO_PhysicsAttrib
{
public:
	// These values can be changed without re-testing the object's physics-state, ie. none of these parameteres affect the world's collision state.

	fp32 m_Mass;						// Not in use. (Unfortunately  :( )
	fp32 m_Friction;					// Friction coefficient. Is not really compatible with real-physics friction coefficients, but values between 0 and 1 is expected.
	fp32 m_Elasticy;					// Bouncing coefficient. 1.0f == full rebounce.
	fp32 m_StepSize;					// The highest stair-step, plus some margin, the object should be able to traverse.

	void Clear();
	CWO_PhysicsAttrib();

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;

	int Compare(const CWO_PhysicsAttrib& _Attr) const;		// NOTE: This compare function is not valid for sorting.

	uint8* Write(uint8* _pData) const;
	const uint8* Read(const uint8* _pData);
};

// -------------------------------------------------------------------
class CWObject_Message
{
public:
	CWObject_Message();
	CWObject_Message(int _Msg, aint _Param0 = 0, aint _Param1 = 0, int16 _iSender = -1, int16 _Reason = 0, const CVec3Dfp32& _VecParam0 = 0, const CVec3Dfp32& _VecParam1 = 0, void* _pData = NULL, int _DataSize = 0);

	int32 m_Msg;
	aint m_Param0;
	aint m_Param1;
	int16 m_iSender;
	int16 m_Reason; // remove
	CVec3Dfp32 m_VecParam0; // remove
	CVec3Dfp32 m_VecParam1; // remove
	void* m_pData;
	int32 m_DataSize; // remove (?)
};

//#define USE_HASHED_WOBJECT_NAME

// -------------------------------------------------------------------
class CVelocityfp32
{
public:
	CAxisRotfp32 m_Rot;
	CVec3Dfp32 m_Move;

	void Unit()
	{
		m_Move = 0;
		m_Rot.Unit();
	}

	void Read(CCFile* _pFile)
	{
		m_Move.Read(_pFile);
		m_Rot.Read(_pFile);
	}

	void Write(CCFile* _pFile) const
	{
		m_Move.Write(_pFile);
		m_Rot.Write(_pFile);
	}
};

// -------------------------------------------------------------------

class CWO_OnIntersectLineContext
{
public:
	class CWObject_CoreData* m_pObj;
	class CWorld_PhysState* m_pPhysState;
	CVec3Dfp32 m_p0;
	CVec3Dfp32 m_p1;
	int m_ObjectFlags;
	int m_ObjectIntersectionFlags;
	int m_MediumFlags;
};


class MRTC_CRuntimeClass_WObject : public MRTC_CRuntimeClass
{
public:
	void (*m_pfnOnIncludeClass)(CMapData*, class CWorld_Server*);
	void (*m_pfnOnIncludeTemplate)(CRegistry*, CMapData*, class CWorld_Server*);
	void (*m_pfnOnClientCreate)(class CWObject_ClientExecute*, class CWorld_Client*);
	void (*m_pfnOnClientExecute)(CWObject_ClientExecute*, CWorld_Client*);
	void (*m_pfnOnClientRefresh)(class CWObject_Client*, CWorld_Client*);
	void (*m_pfnOnClientPredict)(CWObject_Client*, CWorld_Client*, int, int, int);
	void (*m_pfnOnClientRender)(CWObject_Client*, CWorld_Client*, CXR_Engine*, const CMat4Dfp32&);
	void (*m_pfnOnClientRenderVis)(CWObject_Client*, CWorld_Client*, CXR_Engine*, const CMat4Dfp32&);
	aint (*m_pfnOnClientMessage)(CWObject_Client*, CWorld_Client*, const CWObject_Message& _Msg);
	void (*m_pfnOnClientNetMsg)(CWObject_Client*, CWorld_Client*, const CNetMsg& _Msg);
	int (*m_pfnOnClientUpdate)(CWObject_Client*, CWorld_Client*, const uint8* _pData, int& _Flags);
	void (*m_pfnOnClientPrecache)(CWObject_Client*, CWorld_Client*, CXR_Engine*);
	void (*m_pfnOnClientPrecacheClass)(CWorld_Client*, CXR_Engine*);
	void (*m_pfnOnClientPostPrecache)(CWObject_Client*, CWorld_Client*, CXR_Engine*);
	void (*m_pfnOnClientLoad)(CWObject_Client*, CWorld_Client*, CCFile*, CMapData*, int);
	void (*m_pfnOnClientSave)(CWObject_Client*, CWorld_Client*, CCFile*, CMapData*, int);
	bool (*m_pfnOnIntersectLine)(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo);
	int (*m_pfnOnPhysicsEvent)(CWObject_CoreData*, CWObject_CoreData*, CWorld_PhysState*, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo);
	void (*m_pfnOnInitClientObjects)(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	uint (*m_pfnOnGetAttachMatrices)(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, uint* _piAttachMatrices, uint _nAttachMatrices, CMat4Dfp32* _pRet);


	MRTC_CRuntimeClass_WObject(char* _pClassName, CReferenceCount* (*_pfnCreateObject)()

		, MRTC_CRuntimeClass* _pClassBase

		, void (*_pfnOnIncludeClass)(CMapData*, CWorld_Server*)
		, void (*_pfnOnIncludeTemplate)(CRegistry*, CMapData*, CWorld_Server*)
		, void (*_pfnOnClientCreate)(CWObject_ClientExecute*, CWorld_Client*)
		, void (*_pfnOnClientExecute)(CWObject_ClientExecute*, CWorld_Client*)
		, void (*_pfnOnClientRefresh)(CWObject_Client*, CWorld_Client*)
		, void (*_pfnOnClientPredict)(CWObject_Client*, CWorld_Client*, int, int, int)
		, void (*_pfnOnClientRender)(CWObject_Client*, CWorld_Client*, CXR_Engine*, const CMat4Dfp32&)
		, void (*_pfnOnClientRenderVis)(CWObject_Client*, CWorld_Client*, CXR_Engine*, const CMat4Dfp32&)
		, aint (*_pfnOnClientMessage)(CWObject_Client*, CWorld_Client*, const CWObject_Message& _Msg)
		, void (*_pfnOnClientNetMsg)(CWObject_Client*, CWorld_Client*, const CNetMsg& _Msg)
		, int (*_pfnOnClientUpdate)(CWObject_Client*, CWorld_Client*, const uint8* _pData, int& _Flags)
		, void (*_pfnOnClientPrecache)(CWObject_Client*, CWorld_Client*, CXR_Engine*)
		, void (*_pfnOnClientPrecacheClass)(CWorld_Client*, CXR_Engine*)
		, void (*_pfnOnClientPostPrecache)(CWObject_Client*, CWorld_Client*, CXR_Engine*)
		, void (*_pfnOnClientLoad)(CWObject_Client*, CWorld_Client*, CCFile*, CMapData*, int)
		, void (*_pfnOnClientSave)(CWObject_Client*, CWorld_Client*, CCFile*, CMapData*, int)
		, bool (*_pfnOnIntersectLine)(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo)
		, int (*_pfnOnPhysicsEvent)(CWObject_CoreData*, CWObject_CoreData*, CWorld_PhysState*, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
		, void (*_pfnOnInitClientObjects)(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
		, uint (*_pfnOnGetAttachMatrices)(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, uint* _piAttachMatrices, uint _nAttachMatrices, CMat4Dfp32* _pRet)
	)
	{
		m_ClassName = _pClassName;
		m_pfnCreateObject = _pfnCreateObject;
		m_pClassBase = _pClassBase;
		m_pfnOnIncludeClass = _pfnOnIncludeClass;
		m_pfnOnIncludeTemplate = _pfnOnIncludeTemplate;
		m_pfnOnClientCreate = _pfnOnClientCreate;
		m_pfnOnClientExecute = _pfnOnClientExecute;
		m_pfnOnClientRefresh = _pfnOnClientRefresh;
		m_pfnOnClientPredict = _pfnOnClientPredict;
		m_pfnOnClientRender = _pfnOnClientRender;
		m_pfnOnClientRenderVis = _pfnOnClientRenderVis;
		m_pfnOnClientMessage = _pfnOnClientMessage;
		m_pfnOnClientNetMsg = _pfnOnClientNetMsg;
		m_pfnOnClientUpdate = _pfnOnClientUpdate;
		m_pfnOnClientPrecache = _pfnOnClientPrecache;
		m_pfnOnClientPrecacheClass = _pfnOnClientPrecacheClass;
		m_pfnOnClientPostPrecache = _pfnOnClientPostPrecache;
		m_pfnOnClientLoad = _pfnOnClientLoad;
		m_pfnOnClientSave = _pfnOnClientSave;
		m_pfnOnIntersectLine = _pfnOnIntersectLine;
		m_pfnOnPhysicsEvent = _pfnOnPhysicsEvent;
		m_pfnOnInitClientObjects = _pfnOnInitClientObjects;
		m_pfnOnGetAttachMatrices = _pfnOnGetAttachMatrices;
	}
};


#define MRTC_IMPLEMENT_SERIAL_WOBJECT_NO_IGNORE(Name, BaseClass, Version)	\
	MRTC_CRuntimeClass_WObject Name::m_RuntimeClass(#Name, CreateCObj<Name>, &BaseClass::m_RuntimeClass,	\
	Name::OnIncludeClass, Name::OnIncludeTemplate, Name::OnClientCreate, Name::OnClientExecute,	Name::OnClientRefresh, \
	Name::OnClientPredict, Name::OnClientRender, Name::OnClientRenderVis, Name::OnClientMessage, Name::OnClientNetMsg, Name::OnClientUpdate, \
	Name::OnClientPrecache, Name::OnClientPrecacheClass, Name::OnClientPostPrecache, Name::OnClientLoad, Name::OnClientSave, Name::OnIntersectLine, \
	Name::OnPhysicsEvent, Name::OnInitClientObjects, Name::OnGetAttachMatrices); \
	MRTC_CRuntimeClass* Name::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; }; \
	MRTC_CClassInit g_ClassReg##Name(&Name::m_RuntimeClass);

#define MRTC_IMPLEMENT_SERIAL_WOBJECT(Name, BaseClass, Version) MRTC_IMPLEMENT_SERIAL_WOBJECT_NO_IGNORE(Name, BaseClass, Version)

#define MRTC_DECLARE_SERIAL_WOBJECT					\
public:														\
	static MRTC_CRuntimeClass_WObject m_RuntimeClass;		\
	virtual MRTC_CRuntimeClass* MRTC_GetRuntimeClass() const;	\
private:


// -------------------------------------------------------------------
//  CWObject_CoreData
// -------------------------------------------------------------------
enum
{
	CWO_CLIENTFLAGS_INVISIBLE =			M_Bit(0),	// OnClientRender is not called on the client.
	CWO_CLIENTFLAGS_LINKINFINITE =		M_Bit(1),	// Scenegraph linking flag for objects that are allways considered visible (worldspawn, sky, sky light, etc..)
	CWO_CLIENTFLAGS_NOUPDATE =			M_Bit(2),	// Object is never replicated to clients. This flag is automatically removed if the object is part of an object-hierarchy.
	CWO_CLIENTFLAGS_NOREFRESH =			M_Bit(3),	// OnRefresh() and OnClientRefresh() are not called
	CWO_CLIENTFLAGS_ISATTACHOBJ =		M_Bit(4),	// Object is an Attach/Hook/Engine. This flag is used to make assumtions in WObj_Hook about clientdata etc.
	CWO_CLIENTFLAGS_HIGHPRECISION = 	M_Bit(5),	// Object position, orientation and velocity is transmitted with high precision.
	CWO_CLIENTFLAGS_NOHASH =			M_Bit(6),	// Object is not hashed and can therefore not collide to anything but the world.
	CWO_CLIENTFLAGS_HASHDIRTY =			M_Bit(7),	// Object's hash-table and scengraph-linkage needs to be updated. (system use only)
	CWO_CLIENTFLAGS_MOVEVELOCITY =		M_Bit(8),	// Enable replication of object's velocity.
	CWO_CLIENTFLAGS_ROTVELOCITY =		M_Bit(9),	// Enable replication of object's velocity.
	CWO_CLIENTFLAGS_RECURSED =			M_Bit(10),	// Used internally for infinite recursion protection
	CWO_CLIENTFLAGS_DESTROYED =			M_Bit(11),	// Set after Object_Destroy() has been run.
	CWO_CLIENTFLAGS_VISIBILITY =		M_Bit(12),	// Object has an occlusion culling function and it's OnClientRenderVis must be executed before the view interface is initialized in the engine.
	CWO_CLIENTFLAGS_NOROTINHERITANCE = 	M_Bit(13),	// Don't inherit rotation from parent (Still inherits position though).
	CWO_CLIENTFLAGS_AXISALIGNEDVISBOX = M_Bit(14),	// When m_VisBox is used the orientation of the object is ignored.
	CWO_CLIENTFLAGS_SHADOWCASTER =  	M_Bit(15),	// Occlusion culling must be performed with respect to shadows cast by the object.

	CWO_CLIENTFLAGS_USERBASE =		0x00010000,
	CWO_CLIENTFLAGS_USERSHIFT =		16,


	CWO_NUMMODELINDICES	 =			3,
	CWO_NUMSOUNDINDICES	 =			2,
	CWO_NUMCLIENTOBJ =				3, // Do NOT change this to below the number of models (CWO_NUMMODELINDICES)!!!
	CWO_NUMDATA	=					8,
	CWO_VISBOXSCALE	=				8,

	CWO_DIRTYMASK_GENERAL =			M_Bit(0),
	CWO_DIRTYMASK_PHYS =			M_Bit(1),
	CWO_DIRTYMASK_POS =				M_Bit(2),			// Includes velocity
	CWO_DIRTYMASK_ROT =				M_Bit(3),			// Includes velocity
	CWO_DIRTYMASK_MODEL =			M_Bit(4),
	CWO_DIRTYMASK_SOUND =			M_Bit(5),
	CWO_DIRTYMASK_ANIM =			M_Bit(6),
	CWO_DIRTYMASK_DATA =			M_Bit(7),
	CWO_DIRTYMASK_HIERARCHY =		M_Bit(8),
	CWO_DIRTYMASK_0x200 =			M_Bit(9),

	CWO_DIRTYMASK_COREMASK =		0xffff,
	CWO_DIRTYMASK_USER =			65536,
	CWO_DIRTYMASK_USERSHIFT =		16,

	CWO_CLIENTUPDATE_AUTOVAR =		1,
	CWO_CLIENTUPDATE_EXTRADATA =	2,

	CWO_PERCLIENT_NUMDATA =			2,

	CWO_ONCLIENTUPDATEFLAGS_DOLINK =1, 
};

#define CWO_VISBOXSCALERECP (1.0f / fp32(CWO_VISBOXSCALE))

class CWS_ClientObjInfo
{
public:
	uint32 m_ClientFlags;
	uint32 m_DirtyMask;
//	int16 m_Data[CWO_PERCLIENT_NUMDATA];

	void Clear()
	{
		m_DirtyMask = 0;
		m_ClientFlags = 0;
//		for(int i = 0; i < CWO_PERCLIENT_NUMDATA; m_Data[i++] = 0);
	}

	CWS_ClientObjInfo()
	{
		Clear();
	}
};

#define CWO_DECLAREACCESS_RW(type, Name, Mask)	\
	const type & Name() const { return m_##Name; };	\
	type & Name() { m_DirtyMask |= Mask; return m_##Name; };

#define CWO_DECLAREACCESS_RW_ARRAY(type, Name, Mask)	\
	const type & Name(int _e) const { return m_##Name[_e]; };	\
	type & Name(int _e) { m_DirtyMask |= Mask; return m_##Name[_e]; };

#define CWO_DECLAREACCESS_R(type, Name)	\
	const type & Name() const { return m_##Name; };


#define WOBJECT_MAX_CONSTRAINTS 12
class CWObject_CoreData : public CReferenceCount
{
public:
	// String translation tables
	static const char* ms_PrimTranslate[];
	static const char* ms_ObjectFlagsTranslate[];
	static const char* ms_ClientFlagsTranslate[];
	static const char* ms_PhysFlagsTranslate[];
	static const char* ms_PhysExtensionsTranslate[];


private:
	CMat4Dfp32 m_LocalPos;						// Position in parent-object space. World-space if no parent exists.
	CMat4Dfp32 m_Pos;							// Position in world-space.
	CMat4Dfp32 m_LastPos;						// Last position in world-space.

	CVelocityfp32 m_PhysVelocity;				// Object's velocity in vector and axis-angle-velocity notation.
	CBox3Dfp32 m_PhysAbsBoundBox;				// Current physical axis-aligned bounding box in world-coordinates.
	CWO_PhysicsState m_PhysState;				// Object's physics-state.

	CVec3Dint8 m_VisBoxMin;						// Vis bounding box. Note that the box-size is not in units.
	CVec3Dint8 m_VisBoxMax;						// There may be a 16-bit padd after this member

	int16 m_iObjectParent;						// Parent object, zero if no parent exist.
	int16 m_iObjectChild;						// First child-object, zero if object doesn't have any children.
	int16 m_iObjectChildPrev;					// Previous child in a child-chain (that this object is part of.)
	int16 m_iObjectChildNext;					// Next child in a child-chain (that this object is part of.)

	// -------------------------------------------------------------------
	/*  A NOTE ON OBJECT HIERARCHY:

	- Child objects use the same scenegraph/PVS position as their root.

	- The hierarchy is replicated fully and consistent to the clients.

	- If an object's parent move, it will not need replication of it's position, nor will it be dirty-masked.
	
	- When a parent-object is deleted, all children are automatically deleted recursively starting from the leaves
	  in the tree. That is, the parent object is deleted after all it's children.

	- The order of deletion of children on the same level in the tree is in linking-order.

	*/

public:
	MRTC_CRuntimeClass_WObject* m_pRTC;			// Run-time class pointer for the object's current class.
												// Think of this as a pointer to an additional virtual function
												// pointer table. This pointer can be assumed to be valid. Also, all function
												// pointers in MRTC_CRuntimeClass_WObject can also be assumed to be valid.

	uint32 m_ClientFlags;						// 8 lsb reserved (see def above), 8 msb for user.
	int16 m_iObject;							// READ ONLY! Object's index. Should be protected.
	int16 m_iClass;								// READ ONLY! Object's class resource-index. Should be protected.
	int32 m_CreationGameTick;					// GameTick when object was created
	fp32 m_CreationGameTickFraction;			// Fraction of gametick creation

	CWO_PhysicsAttrib m_PhysAttrib;
	spCReferenceCount m_lspClientObj[CWO_NUMCLIENTOBJ];

	int32 m_Data[CWO_NUMDATA];					// User-data
//	int32 m_PerClientData[CWO_PERCLIENT_NUMDATA];// Per-client data, only valid on clients.
	int16 m_iModel[CWO_NUMMODELINDICES];		// Space for model resource-indices.
	int16 m_iAnim0;
	int16 m_iAnim1;
	int16 m_iAnim2;
	int16 m_iSound[CWO_NUMSOUNDINDICES];		// Resource-indices for static sounds. Static sounds are looping.
	int16 m_iParentAttach : 7;					// Index to parent attachpoint  (TODO: fix align waste)
	

#ifndef M_RTM
	int m_bIsInWOHash;
#endif

//	fp32 m_Phys_Elasticy;						// Bouncing coefficient. 1.0f == full rebounce.
//	fp32 m_Phys_Friction;						// Friction coefficient. Is not really compatible with real-physics friction coefficients, but values between 0 and 1 is expected.
//	fp32 m_Phys_StepSize;						// The highest stair-step, plus some margin, the object should be able to traverse.

	class CWD_RigidBody2* m_pRigidBody2;
	class CWPhys_Cluster* m_pPhysCluster;
	TStaticArray<uint32, WOBJECT_MAX_CONSTRAINTS> m_liConnectedToConstraint;


public:
	void operator= (const CWObject_CoreData&);

	virtual void Clear();
	CWObject_CoreData();
	~CWObject_CoreData();

	MRTC_CRuntimeClass_WObject* GetRTC() { return m_pRTC; };

	M_FORCEINLINE int GetParent() const { return m_iObjectParent; };
	M_FORCEINLINE int GetFirstChild() const { return m_iObjectChild; };
	M_FORCEINLINE int GetPrevChild() const { return m_iObjectChildPrev; };
	M_FORCEINLINE int GetNextChild() const { return m_iObjectChildNext; };

	// Returns world-space position
	M_FORCEINLINE const CVec3Dfp32& GetPosition() const { return m_Pos.GetRow(3); }
	M_FORCEINLINE vec128 GetPosition_vec128() const { return m_Pos.r[3]; }

	M_FORCEINLINE const CMat4Dfp32& GetPositionMatrix() const { return m_Pos; }
	M_FORCEINLINE const CVec3Dfp32& GetLastPosition() const { return m_LastPos.GetRow(3); }
	M_FORCEINLINE const CMat4Dfp32& GetLastPositionMatrix() const { return m_LastPos; }

	// Returns parent-space position
	M_FORCEINLINE const CVec3Dfp32& GetLocalPosition() const { return m_LocalPos.GetRow(3); }
	M_FORCEINLINE const CMat4Dfp32& GetLocalPositionMatrix() const { return m_LocalPos; } 

	M_FORCEINLINE void OverrideLastPositionMatrix( const CMat4Dfp32& _Mat ) { m_LastPos = _Mat; }	// Only use this for haxx0ring
	M_FORCEINLINE void OverridePositionMatrix( const CMat4Dfp32& _Mat ) { m_Pos = _Mat; }			// Only use this for haxx0ring

	M_FORCEINLINE const CWO_PhysicsState& GetPhysState() const { return m_PhysState; };
	M_FORCEINLINE void SetPhysStateModelInstance(CXR_ModelInstance* _pModelInstance) { m_PhysState.m_pModelInstance = _pModelInstance; }

	M_FORCEINLINE const CBox3Dfp32* GetAbsBoundBox() const { return &m_PhysAbsBoundBox; };

	void SetVisBoundBox(const CBox3Dfp32& _Box);
	void GetVisBoundBox(CBox3Dfp32& _Box) const;
	void GetAbsVisBoundBox(CBox3Dfp32& _Box) const;		// Calculates world-space vis box.

	M_FORCEINLINE const CVec3Dint8& GetVisBoundBox_RawMin() const { return m_VisBoxMin; };
	M_FORCEINLINE const CVec3Dint8& GetVisBoundBox_RawMax() const { return m_VisBoxMax; };

	M_FORCEINLINE const CVec3Dfp32& GetMoveVelocity() const { return m_PhysVelocity.m_Move; };
	M_FORCEINLINE vec128 GetMoveVelocity_vec128() const { return M_VLd_V3_Slow(&m_PhysVelocity.m_Move); };
	M_FORCEINLINE const CAxisRotfp32& GetRotVelocity() const { return m_PhysVelocity.m_Rot; };
	M_FORCEINLINE const CVelocityfp32& GetVelocity() const { return m_PhysVelocity; };

	void GetVelocityMatrix(CMat4Dfp32&) const;							// Expensive, use with caution
	void GetVelocityMatrix(fp32 _Scale, CMat4Dfp32& _Dest) const;		// Expensive, use with caution

	M_FORCEINLINE bool IsDestroyed() const { return (m_ClientFlags & CWO_CLIENTFLAGS_DESTROYED) != 0; }
	M_FORCEINLINE bool IsClass(uint32 _ClassNameHash) const { return (m_pRTC->m_ClassNameHash == _ClassNameHash); }

	static int GetUpdatedClass(int _iCurrentClass, const uint8* _pData);
	static int GetObjectNr(const uint8* _pData);

	virtual CStr Dump(CMapData* _pWData, int _DumpFlags);

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);

	friend class CWorld_Server;
	friend class CWorld_ServerCore;
	friend class CWorld_Client;
	friend class CWorld_ClientCore;
	friend class CWorld_PhysState;
	friend class CWObject;			// Networking won't compile without this
	friend class CWObject_Client;
	friend class CWObject_Object;	// Load/Save won't compile without this

	void SetAnimTick(CWorld_Server* _pWServer, int32 _AnimTick, fp32 _TickFraction);
	int32 GetAnimTick(CWorld_PhysState* _pWPhysState) const;
	fp32 GetAnimTickFraction() const;

	void CleanPhysCluster();

	//
	//	Rigid body physics
	//
	M_FORCEINLINE fp32 GetMass() const                          { return m_PhysAttrib.m_Mass; }
	M_FORCEINLINE fp32 GetCoefficientOfFriction() const	        { return m_PhysAttrib.m_Friction; }
	M_FORCEINLINE void SetMass(fp32 _Mass)	                    { m_PhysAttrib.m_Mass = _Mass; }
	M_FORCEINLINE void SetCoefficientOfFriction(fp32 _Friction)	{ m_PhysAttrib.m_Friction = _Friction; }

protected:
	//
	// Internal set routines
	// NOTE: You should never ever ever ever have to use these, they're only here to make some specific code work (mostly for load/save)
	//

	M_FORCEINLINE CVec3Dfp32& InternalState_GetPosition() {return m_Pos.GetRow(3);}
	M_FORCEINLINE CMat4Dfp32& InternalState_GetPositionMatrix() {return m_Pos;}
	M_FORCEINLINE CVec3Dfp32& InternalState_GetLocalPosition() {return m_LocalPos.GetRow(3);}
	M_FORCEINLINE CMat4Dfp32& InternalState_GetLocalPositionMatrix() {return m_LocalPos;}
	M_FORCEINLINE CVec3Dfp32& InternalState_GetLastPosition() {return m_LastPos.GetRow(3);}
	M_FORCEINLINE CMat4Dfp32& InternalState_GetLastPositionMatrix() {return m_LastPos;}
	M_FORCEINLINE CVec3Dfp32& InternalState_GetMoveVelocity() {return m_PhysVelocity.m_Move;}
	M_FORCEINLINE CAxisRotfp32& InternalState_GetRotationVelocity() {return m_PhysVelocity.m_Rot;}
};


// -------------------------------------------------------------------
//  CWObject_Client
// -------------------------------------------------------------------
class CWObject_Client : public CWObject_CoreData, public TLinkSP<CWObject_Client>
{
	MRTC_DECLARE;
public:
	int m_hVoice[CWO_NUMSOUNDINDICES];				// Sound-context voice handles
	int16 m_iSoundPlaying[CWO_NUMSOUNDINDICES];		// iSound playing.
	int32 m_ClientData[1];
	uint8 m_iActiveClientCopy : 7;
	uint8 m_bAutoVarDirty : 1;

	TPtr<CXR_ModelInstance> m_lModelInstances[CWO_NUMMODELINDICES];
	
//	TPtr<CWObject_Client> m_spNextCopy;				// List of object-copies in a prediction chain.

	// Pure-virtual from TLinkSP
	virtual CWObject_Client* GetThis() { return this; };

	CWObject_Client* GetClientCopy(int _Nr)
	{
		CWObject_Client* pObj = this;
		while(_Nr && (pObj->GetNext() != NULL))
		{
			pObj = pObj->GetNext();
			_Nr--;
		}

		if (_Nr) return NULL;
		return pObj;
	}

	CWObject_Client* GetActiveClientCopy()
	{
		return GetClientCopy(m_iActiveClientCopy);
	}

	CWObject_Client* GetLastClientCopy(int _Nr)
	{
		return GetTail();
	}

	void operator= (const CWObject_Client&);

	virtual void Clear();
	CWObject_Client();

	static void UpdateModelInstance(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _iModel, TPtr<CXR_ModelInstance> &_spInstance);

	CXR_AnimState GetDefaultAnimState(CWorld_Client* _pWClient, int iClientData = 0);
	int AddDeltaUpdate(CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
};

typedef TPtr<CWObject_Client> spCWObject_Client;

// -------------------------------------------------------------------
//  CWObject_ClientExecute
// -------------------------------------------------------------------

// Client-execute objects ("client-objects") exists only on the client they were created. They don't exists
// in client-mirrors, so client-code that create client-objects must function and extrapolate properly even if
// they fail to create client-objects.
// Client-objects are only executed (getting OnClientExecute calls) when potentially visible. (or so it will be)

class CWObject_ClientExecute : public CWObject_Client
{
public:

	void operator= (const CWObject_ClientExecute&);

	virtual void Clear();
	CWObject_ClientExecute();

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);
};

// -------------------------------------------------------------------
//  CWObject
// -------------------------------------------------------------------
enum
{
	CWO_PREDICT_ADDFRAME,
	CWO_PREDICT_REMOVEFRAME,
	CWO_PREDICT_DESTROY,
	CWO_PREDICT_COMPARE
};

class CTargetNameElement;
//typedef STANDARD_NODE(CTargetNameElement, 2) CTargetNameTreeNode;

class CWObject : public CWObject_CoreData
{
	MRTC_DECLARE_SERIAL_WOBJECT;

#ifndef M_RTMCONSOLE
	CStr m_Name;
	CStr m_ParentName;
#endif
	CTargetNameElement* m_pNameNode;		// Node in the binary search-tree the server maintains.

	CStr m_TemplateName;						// The name of the template the object was spawned from, if any.

//	int32 m_iTZObject;							// Used when saving the map's state. Index of the owning transitzone.

protected:
	uint32 m_ParentNameHash;
	int32 m_NextRefresh;						// The next gametick OnRefresh will be run. Automatically incremented by CWorld_Server::Simulate()
	uint32 m_LastClientUpdate;					// The gametick for the last update to any client.
	uint32 m_GUID;								// Object's globaly unique identifier. All cross-object references should use GUID instead of object-index.

// Bounds stuff
	void InitStatic() const;
	bool CheckForOOB() const;

public:
	uint16 m_IntersectNotifyFlags;				// Object types that should receive the OBJSYSMSG_NOTIFY_INTERSECTION message.

	//CWObject_Character::OnDeltaSave uses the exact size of m_iOwner, remember that when modifying it
	uint16 m_iOwner : 14;						// Index of owner object. (FIXME: unsafe to use when objects can be world-teleported, must convert to GUID some rainy day.)
	uint16 m_bOriginallySpawned : 1;			// Flag saying if this object was spawned during the first tick. Used when delta-saving
	uint16 m_bNoSave : 1;						// Flag saying if this object is not be stored in save games
	uint16 m_Param;								// Parameter for scripting use. Can be set and compared on any class
	uint32 m_DirtyMask;

	CWO_DECLAREACCESS_RW(uint32, ClientFlags, CWO_DIRTYMASK_GENERAL);
	CWO_DECLAREACCESS_R(int16, iObject);
	CWO_DECLAREACCESS_R(int16, iClass);
	CWO_DECLAREACCESS_RW_ARRAY(int16, iModel, CWO_DIRTYMASK_MODEL);
	//	CWO_DECLAREACCESS_RW(fp32, AnimTime, CWO_DIRTYMASK_ANIM);
	CWO_DECLAREACCESS_RW(int16, iAnim0, CWO_DIRTYMASK_ANIM);
	CWO_DECLAREACCESS_RW(int16, iAnim1, CWO_DIRTYMASK_ANIM);
	CWO_DECLAREACCESS_RW(int16, iAnim2, CWO_DIRTYMASK_ANIM);
	CWO_DECLAREACCESS_RW_ARRAY(int16, iSound, CWO_DIRTYMASK_SOUND);
	CWO_DECLAREACCESS_RW_ARRAY(int32, Data, CWO_DIRTYMASK_DATA);

	CWO_DECLAREACCESS_RW(CWO_PhysicsAttrib, PhysAttrib, CWO_DIRTYMASK_PHYS);

	dllvirtual int Phys_TranslateObjectFlags(const char* _pStr); // Helper function to translate object-flags from a string
	dllvirtual int Phys_TranslatePhysFlags(const char* _pStr);	// Helper function to translate physics-flags from a string
	dllvirtual void Phys_AddPrimitive(const char* _pPrim, CWO_PhysicsState* _pTarget);	// Helper function to translate physics-primitives from string-definitions.

	CWorld_Server* m_pWServer;

	const char* GetName() const { return m_Name.Str(); };
	const char* GetTemplateName() const { return (const char*) m_TemplateName; };
	dllvirtual void SetTemplateName(CStr _TemplateName);
	dllvirtual uint32 GetNameHash() const;
	uint GetLastTickInClientPVS() const { return m_LastClientUpdate; };
		
	void SetNextRefresh(int _Tick);

	CWObject();												// m_pServer is NULL in it's constructor.
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);			// Called (after OnCreate) once for every key when spawning from an XW file for the first time.

	virtual void OnCreate();								// Called immediately after object has been created and has recieved object-index and server-pointer. It is called EVERY time an object is created, even if it's from a savegame.
	virtual void OnFinishEvalKeys();						// Called when all keys of the entity has been processed (which is after OnCreate()) with OnEvalKey.
	virtual void OnInitInstance(const aint* _pParam, int _nParam);// Called when object is spawned, after OnFinishEvalKeys (i.e, not when it is loaded from a savegame or transitzone)
	virtual void OnTransform(const CMat4Dfp32& _Mat);		// NOT IN USE YET. Can be called at any time, uses are teleports and relocation on world-switches.
	virtual void OnRegistryChange(const CRegistry* _pReg);	// NOT IN USE YET. Called before simulation if game-registry has been altered and a global update has been requested. Can also be called implicitly by other objects at any time.
	virtual void OnSpawnWorld();							// Called after all objects on a map has been spawned. It will not be called for objects created during execution of OnSpawnWorld.
	virtual void OnSpawnWorld2();							// Called after all OnSpawnWorld, when hirerachy will be valid
	virtual void OnLoad(CCFile* _pFile);					// Called when loading savegame.
	virtual void OnSave(CCFile* _pFile);					// Called when saving savegame.
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);	// Called when loading delta savegame.
	virtual void OnDeltaSave(CCFile* _pFile);				// Called when saving delta savegame.
	virtual void OnFinishDeltaLoad();						// Called after loading delta savegame (also applied when spawning level without savegame)
	virtual void OnDestroy();								// Called before destruction. Object is still intact, but will be destroyed immedialtely after returning from OnDestroy(). OnDestroy is only called when objects are destroyed in-game, not when closing an entire map.
	virtual aint OnMessage(const CWObject_Message& _Msg);	// Called at any time when object receives a message.
	virtual int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;	// Called whenever a delta-update should be compiled.
	virtual void OnRefresh();								// Called once every game-tick, unless CWO_CLIENTFLAGS_NOREFRESH is specified.

	virtual void UpdateVisibility(int *_lpExtraModels = NULL, int _nExtraModels = 0); // Recreates the VisBox from m_iModel, also modifies the ShadowCasterFlag.

	static void IncludeModelFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData);
	static void IncludeSoundFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData);
	static void IncludeClassFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData);
	static void IncludeAnimFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData);
	static void IncludeSurfaceFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData);

	static void IncludeModel(const CStr _Model, CMapData *_pMapData);
	static void IncludeSound(const CStr _Sound, CMapData *_pMapData);
	static void IncludeClass(const CStr _Class, CMapData *_pMapData);
	static void IncludeAnim(const CStr _Anim, CMapData *_pMapData);
	static void IncludeSurface(const CStr _Surface, CMapData *_pMapData);

	void ReserveTempPhysState();					// Called whenever the temporary physics state is needed.
	void DiscardTempPhysState();					// Called from CWObject::OnFinishEvalKeys()
	CWO_PhysicsState *GetTempPhysState();			// Used to acuire the initialized temporary physics state.

	// -------------------------------------------------------------------
	//  Pseudo-virtual functions:
	// -------------------------------------------------------------------
	// Server only:
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server*);							// Called when resource-manager has received notification that the class will be used, and thus gives the class opportunity to include other classes.
	static void OnIncludeTemplate(CRegistry*, CMapData* _pWData, CWorld_Server*);

	// Client only:
	static void OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);		// Called when the object is replicated to the client. This may not occur simulaneously with the server-side OnCreate()
	static void OnClientExecute(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);		// Called only for 'client-execute' objects once every game-tick.
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);				// Called once every game-tick if the object is potentially visible, unless CWO_CLIENTFLAGS_NOREFRESH is specified
	static void OnClientPredict(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _Cmd, int, int);	// 
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine*, const CMat4Dfp32&);	// Called zero, one or several times for every frame rendered. Should not modify the state of the object in ways that make assumptions of the call-rate. (i.e, it's perfectly legal for the client to call the object 20 times for one rendered frame)
	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine*, const CMat4Dfp32&);// Called in addition to OnClientRender on objects with CWO_CLIENTFLAGS_VISIBILITY. OnClientRenderVis is called before OnClientRender
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);	// Called immediately on reception of an object net-message. Don't assume that the client game-time is equal to the server game-time when it was sent, or that the object has the same state as on the server.
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnClientPrecache(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine);	// Called ...
	static void OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine);						// Called ...
	static void OnClientPostPrecache(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine);	// Called ...

	static void OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWClient, CCFile* _pFile, CMapData* _pWData, int _Flags);
	static void OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWClient, CCFile* _pFile, CMapData* _pWData, int _Flags);

	// Server & client:
	static bool OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo = NULL);
	static int OnPhysicsEvent(CWObject_CoreData*, CWObject_CoreData*, CWorld_PhysState*, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo = NULL);
	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);	// Called when the object is creating class specific extra objects, like clientdata
	static uint OnGetAttachMatrices(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, uint* _piAttachMatrices, uint _nAttachMatrices, CMat4Dfp32* _pRet);

	static int TranslateServerFlagsStr(CStr _St);

	// Get a usable CWObject-pointer for AI to work with (inherits from CWObject to avoid multiple inheritance)
	virtual class CWObject_Interface_AI* GetInterface_AI() { return NULL; }

	// -------------------------------------------------------------------
	virtual CStr Dump(CMapData* _pWData, int _DumpFlags);	// Debug routine used for dumping the server-context.

	friend class CWorld_Server;
	friend class CWorld_ServerCore;
	friend class CTargetNameElement;

	template<typename TSPECALIZATION> friend class TWO_Hash_RO;
	template<typename TSPECALIZATION> friend class TWO_Hash_RW;

	friend class CWO_SpaceEnumSpecialization_Common::CHashSpecialization::CElementMixin;
};

typedef TPtr<CWObject> spCWObject;

// -------------------------------------------------------------------


class CTargetNameElement
{
public:
	CWObject* m_pObj;
	uint32 m_NameHash;

	class CCompare
	{
	public:
		DIdsPInlineS static aint Compare(const CTargetNameElement *_pFirst, const CTargetNameElement *_pSecond, void *_pContext)
		{
			if (_pFirst->m_NameHash < _pSecond->m_NameHash)
				return -1;
			else if (_pFirst->m_NameHash > _pSecond->m_NameHash)
				return 1;
			else
				return (auint(_pFirst->m_pObj) >> 2) - (auint(_pSecond->m_pObj) >> 2);
		}

		DIdsPInlineS static aint Compare(const CTargetNameElement *_pTest, const CTargetNameElement& _Key, void *_pContext)
		{
			if (_pTest->m_NameHash < _Key.m_NameHash)
				return -1;
			else if (_pTest->m_NameHash > _Key.m_NameHash)
				return 1;
			else
				return (auint(_pTest->m_pObj) >> 2) - (auint(_Key.m_pObj) >> 2);
		}
	};

	DIdsTreeAVLAligned_Link(CTargetNameElement, m_NameLink, CTargetNameElement, CCompare);


	CTargetNameElement()
	{
		m_pObj = NULL;
		m_NameHash = NULL;
	}

/*	void operator=(const CTargetNameElement& _Elem)
	{
		m_pObj = _Elem.m_pObj;
		m_NameHash = _Elem.m_NameHash;
	}

	int operator< (const CTargetNameElement& _Node) const
	{
		return (m_NameHash < _Node.m_NameHash);
	}

	int operator== (const CTargetNameElement& _Node) const
	{
		return (m_NameHash == _Node.m_NameHash);
	}

	int operator> (const CTargetNameElement& _Node) const
	{
		return (m_NameHash > _Node.m_NameHash);
	}*/
};

// typedef STANDARD_NODE(CTargetNameElement, 2) CTargetNameTreeNode;


#endif
