#ifndef _INC_WPHYSSTATE
#define _INC_WPHYSSTATE

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Base class for Server and Client objects with helpers

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CMovePhysInfo
					CWorld_PhysState

	History:		
\*____________________________________________________________________________________________*/

#include "WClass.h"
#include "WMapData.h"
#include "../Render/MWireContainer.h"
#include "WPhysState_NavGridHash.h"
#include "WListener.h"

/*
#if defined(PLATFORM_XBOX) && 0
	#define WCLIENT_FIXEDRATE
	#define CWCLIENT_FRAMERATE 30
	#define CWCLIENT_TIMEPERFRAME (1.0f / fp32(CWCLIENT_FRAMERATE))

#else
	#define CWCLIENT_FRAMERATE 20
	#define CWCLIENT_TIMEPERFRAME (0.050f)	// Should ideally be set by server.

#endif

#define PHYSSTATE_CONVERTFROM20HZ(a) (a * 20.0f / fp32(CWCLIENT_FRAMERATE))

// -------------------------------------------------------------------
#define SERVER_TIMEPERFRAME CWCLIENT_TIMEPERFRAME	// 1/0.050f = 20 game-frames per sec.
#define SERVER_TICKSPERSECOND ((int)1.0f/SERVER_TIMEPERFRAME)	// 20 game-frames per sec.
*/

#define PHYSSTATE_TICKS_TO_TIME(ticks, physstate) CMTime::CreateFromTicks(ticks, (physstate)->GetGameTickTime())

enum
{
	PHYSSTATE_MTSTATE_ASSERTUNSAFE = 1,
};

#ifdef M_Profile
	#define PHYSSTATE_ASSERTUNSAFE if (m_MTState & PHYSSTATE_MTSTATE_ASSERTUNSAFE) { M_BREAKPOINT; }
#else
	#define PHYSSTATE_ASSERTUNSAFE
#endif

#define SERVER_SELECTIONSTACK 16					// Selection stack depth

//#ifndef M_RTM
#if (!defined(PLATFORM_PS2) && !defined(PLATFORM_DOLPHIN) || (defined(PLATFORM_XBOX) && !defined(M_RTM))) && defined(M_Profile)
#define SERVER_STATS
#endif
//#endif


// -------------------------------------------------------------------
//  Object hashing
// -------------------------------------------------------------------
#define SERVER_HASH1_SIZE 64					// => 64x64 hash
#define SERVER_HASH1_BUCKET_SHIFT_SIZE	7		// => Buckets are sized (1 << 7) by (1 << 7). (128x128)

#define SERVER_HASH2_SIZE 32					// => 32x32 hash
#define SERVER_HASH2_BUCKET_SHIFT_SIZE	9		// => Buckets are sized (1 << 9) by (1 << 9). (512x512)
//#define SERVER_HASH1_MAX_HASHED_ELEMENT_SIZE	(1 << SERVER_HASH_BUCKET_SHIFT_SIZE)

#define CSEW_HASH 1
#define CSEW_LARGE 2

#define SERVER_NUM_GUID_HASH_SLOTS		0x100
#define SERVER_NUM_GUID_HASH_AND		0xff

#define SERVER_MAXPHYSRECURSE 2

// -------------------------------------------------------------------
#define OBJSYSMSG_PHYSICS_INTERSECTION		0x0001		// Param0 = iObj, called when MovePhysical is done.
#define OBJSYSMSG_PHYSICS_PREINTERSECTION	0x0002		// Param0 = iObj, called from inside of MovePhysical.
#define OBJSYSMSG_NOTIFY_INTERSECTION		0x0003		// Param0 = iObj
#define OBJSYSMSG_PHYSICS_BLOCKING			0x0004		// Param0 = iObj, called from inside of MovePhysical.

#define OBJSYSMSG_GAME_ADDCLIENT			0x0010
#define OBJSYSMSG_GAME_REMOVECLIENT			0x0011
#define OBJSYSMSG_GAME_CLIENTCONNECTED		0x0012
#define OBJSYSMSG_GAME_INITWORLD			0x0013
#define OBJSYSMSG_GAME_CLIENTVARCHANGED		0x0014
#define OBJSYSMSG_GAME_CLIENTSAY			0x0015
#define OBJSYSMSG_GAME_CLOSEWORLD			0x0016
#define OBJSYSMSG_GAME_POSTWORLDCLOSE		0x0017
#define OBJSYSMSG_GAME_POSTWORLDLOAD		0x0018
#define OBJSYSMSG_GAME_POSTWORLDSAVE		0x0019

#define OBJSYSMSG_GETCAMERA					0x0020		// Client message
#define OBJSYSMSG_GETANIMSTATE				0x0021		// Client message
#define OBJSYSMSG_GETSKELETON				0x0022		// Client message
#define OBJSYSMSG_GETSKELETONINSTANCE		0x0023		// Client message
#define OBJSYSMSG_GETVIEWPORT				0x0024		// Client message
#define OBJSYSMSG_GETPHYSANIMSTATE			0x0025		// Client message
#define OBJSYSMSG_GETFOV					0x0026		// Client message

#define OBJSYSMSG_DESTROY					0x0030
#define OBJSYSMSG_PLAYSOUND					0x0031
#define OBJSYSMSG_PRECACHEMESSAGE			0x0032
#define OBJSYSMSG_SETSOUND					0x0033
#define OBJSYSMSG_GETDEBUGSTRING			0x0034
#define OBJSYSMSG_SETMODEL					0x0035
#define OBJSYSMSG_REQUESTFORCESYNC			0x0036
#define OBJSYSMSG_TELEPORT					0x0037
#define OBJSYSMSG_ACTIVATE					0x0038
#define OBJSYSMSG_PARAM_SET					0x0039
#define OBJSYSMSG_PARAM_GET					0x003a
#define OBJSYSMSG_GETDISTANCE				0x003b
#define OBJSYSMSG_PARAM_ADD					0x003c
#define OBJSYSMSG_PARAM_FLAG_SET			0x003d
#define OBJSYSMSG_PARAM_FLAG_CLEAR			0x003e
#define OBJSYSMSG_PARAM_FLAG_ISSET			0x003f
#define OBJSYSMSG_GETRANDOM					0x0040
#define OBJSYSMSG_PARAM_RANDOMIZE			0x0041
#define OBJSYSMSG_PARAM_COPYFROM			0x0042
#define OBJSYSMSG_PLAYER_CANSAVE			0x0043
#define OBJSYSMSG_LISTENER_EVENT			0x0044		// Param0 = event,  m_iSender = the object of interest
#define OBJSYSMSG_SETPARENT					0x0045		// Param0 = Flags,  String = Target[:Attach]
#define OBJSYSMSG_SOUNDSYNC					0x0046		// Param0 = cmd, Param1 = client bit mask
#define OBJSYSMSG_SETVISIBLE				0x0047		// Param0 = 0:visible, 1:hidden


// m_PhysRenderFlags enums  (set by 'sv_physrender' & 'cl_physrender')
enum
{
	CWO_DEBUGRENDER_CAPTURE =	M_Bit(0),		// indicates if server should create m_spCapture
	CWO_DEBUGRENDER_WIRE =		M_Bit(1),		// used for normal debug rendering (default when calling Debug_GetWireContainer)
	CWO_DEBUGRENDER_NAMES =		M_Bit(2),		// server renders object names
};

// -------------------------------------------------------------------
// OBJSYSMSG_LISTENER_EVENT event masks
enum
{
	CWO_LISTENER_EVENT_DELETED =	M_Bit(0),	// Object was deleted
	CWO_LISTENER_EVENT_MOVED =		M_Bit(1),	// Object position was updated
};

// -------------------------------------------------------------------
// OnPhysicEvent event enums
enum
{
	CWO_PHYSEVENT_GETACCELERATION = 0,			// Request accelleration from object. pMat points to a matrix to be filled with the current accelleration.
	CWO_PHYSEVENT_IMPACTHANDLEDOTHER,			// Sent on impact to object beeing moved if collsion response was handled by the other object.
	CWO_PHYSEVENT_IMPACT,						// Sent on impact to object beeing moved.
	CWO_PHYSEVENT_IMPACTOTHER,					// Sent on impact to object collided to.
	CWO_PHYSEVENT_BOUNCE,						// Sent to object after a bounce operation has been performed.  
												//		pMat points to a fp32 containing the impact velocity before handling. (velocity projected on normal)
	CWO_PHYSEVENT_BLOCKING,						// Sent to object after a push of another object have failed to clear the way for the object.
	CWO_PHYSEVENT_PREPUSHED,					// Sent to before it's beeing pushed by another object.

	CWO_PHYSEVENT_DYNAMICS_COLLISION,			// m_Pos = PointOfCollision, m_Velocity.k[0] = impact magnitude
};

// -------------------------------------------------------------------
//  Return codes for OnPhysicsEvent and OBJSYSMSG_PHYSICS_PREINTERSECTION message.
enum
{
	SERVER_PHYS_DEFAULTHANDLER				= 0,		// Collision response should be handled by Object_MovePhysical.
	SERVER_PHYS_HANDLED						= 1,		// Collision response was handled, continue with next move.
	SERVER_PHYS_ABORT						= 2,		// Abort movement. Object_MovePhysical will return immediately.
};

// -------------------------------------------------------------------
enum
{
	SERVER_MOVEPHYSINFOFLAGS_NOEXTERNALEFFECTS	= 1,	// Movement may not affect any other objects. (No OnPhysicsEvent or messages performed)
	SERVER_MOVEPHYSINFOFLAGS_ACCELLERATION		= 2,	// Accelleration provided in CMovePhysInfo should be used.
};

class CMovePhysInfo
{
public:
	int m_Flags;
	fp32 m_StepUpResult;
	CMat4Dfp32 m_Accelleration;

	CMovePhysInfo()
	{
		m_Flags = 0;
		m_StepUpResult = 0;
	}

	CMovePhysInfo(int _Flags)
	{
		m_Flags = _Flags;
		m_StepUpResult = 0;
	}
};




class CSelection
{
	uint m_Capacity;
	uint m_SelectNumElems;

protected:
	// Disabled ctor  (used by TSelection<>)
	CSelection(uint _Capacity) 
		: m_Capacity(_Capacity)
		, m_SelectNumElems(0) { }


public:
	static void copy(CSelection& _Selection1, const CSelection& _Selection2)
	{
		M_ASSERT(_Selection1.GetCapacity() >= _Selection2.GetNumElements(), "CSelection::copy(), buffer is too small");
		memcpy(_Selection1.GetData(), _Selection2.GetData(), _Selection2.GetNumElements() * sizeof(int16));
		_Selection1.m_SelectNumElems = _Selection2.m_SelectNumElems;
	}

	void AddData(int16 _Data) 
	{
		if (m_SelectNumElems < m_Capacity)
		{
			GetData()[m_SelectNumElems] = _Data;
			m_SelectNumElems++;
		}
	}

public:
	enum
	{
		LARGE_BUFFER  = 2560,
		MEDIUM_BUFFER = 512,
		SMALL_BUFFER  = 64,
	};

	M_FORCEINLINE int GetCapacity() const                  { return m_Capacity; }
	M_FORCEINLINE void Reset()                             { m_SelectNumElems = 0; }
	M_FORCEINLINE int16* GetData()                         { return (int16*)((char*)this+sizeof(CSelection)); }
	M_FORCEINLINE const int16* GetData() const             { return (const int16*)((char*)this+sizeof(CSelection)); }
	M_FORCEINLINE bool IsBufferFull() const	               { return m_SelectNumElems >= m_Capacity; }
	M_FORCEINLINE void IncreaseNumElements(uint _Increase) { m_SelectNumElems += _Increase; }
	M_FORCEINLINE uint GetNumElements() const              { return m_SelectNumElems; }
	M_FORCEINLINE void SetNumElements(uint _nElem)         { m_SelectNumElems = _nElem; }

	// Enable using a CSelection object as a standad TArray / TThinArray / TArrayPtr...
	M_FORCEINLINE uint Len() const { return m_SelectNumElems; }
	M_FORCEINLINE int16 operator[](uint _i) const { return *((int16*)((char*)this + sizeof(CSelection)) + _i); }
};


template<int _TCapacity>
class TSelection : public CSelection
{
protected:
	int16 m_Buffer[_TCapacity];
public:
	M_FORCEINLINE TSelection() : CSelection(_TCapacity) { }
};


// -------------------------------------------------------------------
//  CWorld_PhysState
// -------------------------------------------------------------------
class CWorld_PhysState : public CConsoleClient
{
protected:
	CMat4Dfp32 m_Unit;

	TPtr<CWO_SpaceEnum> m_spSpaceEnum;

	CXR_PhysicsModel_Sphere m_PhysModel_Sphere;
	CXR_PhysicsModel_Box m_PhysModel_Box;

	CIndexPool16 m_ObjPoolDynamics;
	//CWorld_Dynamics m_Dynamics;

	uint16 m_iObject_Worldspawn;
	uint16 m_iObject_DisabledLinkage;
	uint16 m_RigidBodyID;  // Need this for the broad phase early out

//	int m_hSceneGraph;
	CXR_SceneGraphInterface* m_pSceneGraph;
	spCXR_SceneGraphInstance m_spSceneGraphInstance;
	CIndexPool16 m_SceneGraphDeferredLinkage;
	CWO_LinkContext m_Listeners;

	// Block-navigator, only present on server. Phys-state needs it for invalidating the grid when objects move.
	TPtr<class CXR_BlockNav> m_spBlockNav;
	TPtr<class CXR_BlockNavSearcher> m_spBlockNavSearcher;
	CXR_BlockNav_Grid_GameWorld* m_pBlockNavGrid;

	//Navgraph and corresponding pathfinder
	TPtr<class CWorld_Navgraph_Pathfinder> m_spGraphPathfinder;
	CXR_NavGraph * m_pNavGraph;
	
	int m_DirtyMask_InsertPosition;

	int m_MovePhysRecurse;
	uint m_MTState;

#ifdef SERVER_STATS
	CMTime m_TFunc_MovePhysical;
	CMTime m_TFunc_IntersectWorld;
	CMTime m_TFunc_IntersectPrim;
	CMTime m_TFunc_IntersectLine;
	CMTime m_TFunc_GetMedium;
	CMTime m_TFunc_Selection;

	CMTime m_TFuncTotal_MovePhysical;
	CMTime m_TFuncTotal_IntersectWorld;
	CMTime m_TFuncTotal_IntersectPrim;
	CMTime m_TFuncTotal_IntersectLine;
	CMTime m_TFuncTotal_GetMedium;
	CMTime m_TFuncTotal_Selection;

	int m_nFunc_MovePhysical;
	int m_nFunc_MovePhysicalQuick;
	int m_nFunc_IntersectWorld;
	int m_nFunc_IntersectPrim;
	int m_nFunc_IntersectLine;
	int m_nFunc_GetMedium;
	int m_nFunc_Selection;
#endif

	TArray<int16> m_lEnumSpace;

	// Physics debug-rendering
	TPtr<class CRenderContextCapture> m_spPhysCapture;
	TPtr<class CDebugRenderContainer> m_spWireContainer;

	// Simulation time variables
	CMTime m_SimulationTime;
	int m_SimulationTick;
	fp32 m_TickTime;
	fp32 m_RefreshRate;
	fp32 m_TickRealTime;
	fp32 m_TimeScale;

public:
	spCWorldData m_spWData;
	spCMapData m_spMapData;

	CWorld_PhysState();
	~CWorld_PhysState();

	virtual bool IsServer() const pure;
	virtual bool IsClient() const pure;

	M_FORCEINLINE CWorldData* GetWorldData() { return m_spWData; }
	M_FORCEINLINE CMapData* GetMapData() { return m_spMapData; }
	M_FORCEINLINE CXR_BlockNav_Grid_GameWorld* GetBlockNav() { return m_pBlockNavGrid; }

	dllvirtual void SetMultiThreadState(uint _MTState);

protected:
	dllvirtual CXR_SceneGraphInterface* World_GetSceneGraph();			// Do not use this.
public:
	dllvirtual void World_CommitDeferredSceneGraphLinkage();
	dllvirtual CXR_SceneGraphInstance* World_GetSceneGraphInstance();	// Only use the light stuff in this.

public:
	dllvirtual void Phys_SetDirtyMask(int _Mask);
	dllvirtual int Phys_GetDirtyMask();

	dllvirtual void Phys_GetMinMaxBox(const class CWO_PhysicsState& _PhysState, const class CWO_PhysicsPrim& _PhysPrim, const CMat4Dfp32& _Pos, CBox3Dfp32& _Box);
	dllvirtual void Phys_GetMinMaxBox(const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Pos, CBox3Dfp32& _Box);
	dllvirtual void Phys_UpdateTree_r(int _iObj, class CWObject_CoreData* _pObj);
	dllvirtual void Phys_InsertPosition(int _iObj, CWObject_CoreData* _pObj);
	dllvirtual bool Phys_IntersectPrimitives(
						const CWO_PhysicsState& _PhysState1, const CWO_PhysicsPrim& _PhysPrim1, 
						const CWO_PhysicsState& _PhysState2, const CWO_PhysicsPrim& _PhysPrim2, 
						const CMat4Dfp32& _Origin1, const CMat4Dfp32& _Dest1,
						const CMat4Dfp32& _Origin2, const CMat4Dfp32& _Dest2, CCollisionInfo* _pCollisionInfo);
public:
	dllvirtual int Phys_IntersectStates(const CWO_PhysicsState& _PhysState1, const CWO_PhysicsState& _PhysState2, 
						const CMat4Dfp32& _Origin1, const CMat4Dfp32& _Dest1,
						const CMat4Dfp32& _Origin2, const CMat4Dfp32& _Dest2, 
						CCollisionInfo* _pCollisionInfo, int _NotifyFlags1, int _NotifyFlags2);

	virtual bool Phys_IntersectWorld(const CSelection* _pSelection, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj = -1, CCollisionInfo* _pCollisionInfo = NULL, int _NotifyFlags = 0, CSelection* _pNotifySelection1 = NULL, CSelection* _pNotifySelection2 = NULL) pure;
	virtual bool Phys_IntersectWorld(class CPotColSet *_pcs, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj = -1, CCollisionInfo* _pCollisionInfo = NULL, int _NotifyFlags = 0, CSelection* _pNotifySelection1 = NULL, CSelection* _pNotifySelection2 = NULL) pure;

protected:
	virtual bool Phys_SetPosition(const CSelection& _Selection, int _iObj, const CMat4Dfp32& _Pos, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_MovePosition(const CSelection* _pSelection, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_MovePosition(class CPotColSet *_pcs, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo = NULL);

public:
	virtual void GetMovementBounds( float *_BoxMinMax, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest );
	virtual void GetMovementBounds( float *_BoxMinMax, CWObject_CoreData* _pObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest );
	virtual void Selection_GetArray(class CPotColSet *pcs, const CSelection* _pSelection, const CWO_PhysicsState &_PhysState, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest);

private:
	void CalcDestPos( CMat4Dfp32 *_pDestPos, CWObject_CoreData *pObj, fp32 _dTime, CVec3Dfp32 *_pAccel );

public:
	virtual aint Phys_Message_SendToObject(const class CWObject_Message& _Msg, int _iObj) pure;
	virtual void Phys_Message_SendToSelection(const CWObject_Message& _Msg, const CSelection& _Sel) pure;
	virtual bool Phys_MessageQueue_SendToObject(const CWObject_Message& _Msg, int _iObj) pure;		// false if msg-buffer is full
	virtual void Phys_MessageQueue_Flush();

protected:
	virtual int Object_HeapSize() pure;
public:
	virtual CWObject_CoreData* Object_GetCD(int _iObj) pure;
	dllvirtual int Object_GetWorldspawnIndex();						// DON'T USE THESE UNLESS YOU HAVE AN EXTREMELY GOOD REASON!
	dllvirtual void Object_SetWorldspawnIndex(int _iObj);				// YOU SHOULD NOT MAKE ANY UNNECESSARY ASSUMPTIONS ABOUT THE ENVIRONMENT!
	virtual int Object_SetDirty(int _iObj, int _Mask) pure;

	virtual int Game_GetObjectIndex() pure;

	virtual fp32 Load_GetProgress() pure;
	virtual void Load_ProgressReset() pure;
	
	dllvirtual const CVec3Dfp32& Object_GetPosition(int _iObj);
	dllvirtual vec128 Object_GetPosition_vec128(int _iObj);
	dllvirtual const CMat4Dfp32& Object_GetPositionMatrix(int _iObj);
	dllvirtual const CVec3Dfp32& Object_GetVelocity(int _iObj);
	dllvirtual const CMat4Dfp32& Object_GetVelocityMatrix(int _iObj);
	dllvirtual const CAxisRotfp32& Object_GetRotVelocity(int _iObj);

	dllvirtual bool Object_SetPhysics(int _iObj, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Pos);	// Returns true if successful
	dllvirtual bool Object_SetPhysics(int _iObj, const CWO_PhysicsState& _PhysState, const CVec3Dfp32& _Pos);	// Returns true if successful, preserves orientation
	dllvirtual bool Object_SetPhysics(int _iObj, const CWO_PhysicsState& _PhysState);							// Returns true if successful
	dllvirtual bool Object_SetPhysics_ObjectFlags(int _iObj, uint _ObjectFlags);
	virtual bool Object_SetPhysics_DoNotify(int _iObj, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Pos);	// Returns true if successful

	// Parent space
	dllvirtual bool Object_SetPosition(int _iObj, const CVec3Dfp32& _Pos);										// Returns true if successful
	dllvirtual bool Object_SetPosition(int _iObj, const CMat4Dfp32& _Pos);										// Returns true if successful
	dllvirtual bool Object_SetPosition(int _iObj, const CVec3Dfp32& _Pos, const CVec3Dfp32 _Angles, int _AnglePriority = 0);		// Returns true if successful
	dllvirtual bool Object_SetRotation(int _iObj, const CMat4Dfp32& _Pos);										// Returns true if successful
	dllvirtual void Object_SetPositionNoIntersection(int _iObj, const CVec3Dfp32& _Pos);
	dllvirtual void Object_SetPositionNoIntersection(int _iObj, const CMat4Dfp32& _Pos);

	// World space
	dllvirtual bool Object_SetPosition_World(int _iObj, const CMat4Dfp32& _Pos);								// Returns true if successful

	dllvirtual void Object_SetVisBox(int _iObj, const CVec3Dfp32& _Min, const CVec3Dfp32& _Max);

	dllvirtual void Object_SetVelocity(int _iObj, const class CVelocityfp32& _Velocity);
	dllvirtual void Object_SetVelocity(int _iObj, const CVec3Dfp32& _Velocity);
	dllvirtual void Object_AddVelocity(int _iObj, const CVec3Dfp32& _dVelocity);
	dllvirtual void Object_SetRotVelocity(int _iObj, const CAxisRotfp32& _Rot);
	dllvirtual void Object_AddRotVelocity(int _iObj, const CAxisRotfp32& _Rot);
	dllvirtual void Object_AccelerateTo(int _iObj, const CVec3Dfp32& _Velocity, const CVec3Dfp32& _Acceleration);

	dllvirtual void Object_DisableLinkage(int _iObj);
	dllvirtual void Object_EnableLinkage(int _iObj);

	dllvirtual fp32 Object_ShortDeltaMove(const CSelection& _Selection, int _iObj, const CVec3Dfp32& _dPos, int _nSlide = 0);
	dllvirtual fp32 Object_ShortDeltaMove(class CPotColSet *_pcs, int _iObj, const CVec3Dfp32& _dPos, int _nSlide = 0);
	dllvirtual fp32 Object_TraceDeltaMove(const CSelection& _Selection, int _iObj, const CVec3Dfp32& _dPos);
	dllvirtual fp32 Object_TraceDeltaMove(class CPotColSet *_pcs, int _iObj, const CVec3Dfp32& _dPos);

	dllvirtual bool Object_MovePhysical(const CSelection* _pSelection, int _iObj, fp32 _dTime = 1.0f, CMovePhysInfo* _pMPInfo = NULL);
	dllvirtual bool Object_MovePhysical(int _iObj, fp32 _dTime = 1.0f, CMovePhysInfo* _pMPInfo = NULL);			// => Object_MovePhysical(-1, _iObj, _dTime, _pMPInfo);
	dllvirtual bool Object_MoveTo(int _iObj, const CMat4Dfp32& _NewPos, CMovePhysInfo* _pMPInfo = NULL);
	dllvirtual bool Object_MoveTo_World(int _iObj, const CMat4Dfp32& _NewPos, CMovePhysInfo* _pMPInfo = NULL);
	
	dllvirtual bool Object_IntersectLine(int _iObj, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _ObjectIntersectFlags, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);

	dllvirtual int Phys_GetMedium(const CVec3Dfp32& _Pos, CXR_MediumDesc* _pRetMedium = NULL);
	dllvirtual void Phys_GetMediums(const CSelection& _Selection, const CVec3Dfp32* _pV, int _nV, CXR_MediumDesc* _pRetMediums);
	dllvirtual void Phys_GetMediums(const CVec3Dfp32* _pV, int _nV, CXR_MediumDesc* _pRetMediums);
	dllvirtual bool Phys_IntersectLine(const CSelection& _Selection, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _ObjectIntersectFlags, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	dllvirtual bool Phys_IntersectLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _ObjectIntersectFlags, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL, int _iExclude = 0, bool _bUseVisBox = false);
	dllvirtual bool Phys_IntersectLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _ObjectIntersectFlags, int _MediumFlags, int _iExclude0,int _iExclude1, bool _bUseVisBox = false);

	// Object hierarchy
	virtual bool Object_AddChild(int _iParent, int _iChild);			// Makes iChild a child if iParent, if _iParent is zero, _iChild is unconnected from any parent it might have.
	dllvirtual bool Object_RemoveChild(int _iParent, int _iChild);			// Is not same as AddChild(0, _iChild), if _iChild is not a child of _iParent, nothing happens.
	dllvirtual int Object_GetParent(int _iObj);
	dllvirtual int Object_GetFirstChild(int _iObj);
	dllvirtual int Object_GetPrevChild(int _iObj);
	dllvirtual int Object_GetNextChild(int _iObj);
	dllvirtual int Object_GetNumChildren(int _iObj, bool _bRecursive = false);// Counts the number of children to iObj
	dllvirtual int Object_IsParentOf(int _iParent, int _iObj);
	dllvirtual int Object_IsAncectorOf(int _iAnc, int _iObj);

	// Selection management
	dllvirtual void Selection_AddAll(CSelection& _Selection);
	dllvirtual void Selection_AddObject(CSelection& _selection, int _iObject);
	dllvirtual void Selection_AddObjectChildren(CSelection& _selection, int _iObject, bool _bRecursive = false);
	dllvirtual void Selection_AddObjects(CSelection& _selection, int16* _piObjects, int _nObjects);
	dllvirtual void Selection_AddClass(CSelection& _selection, int _iClass);
	dllvirtual void Selection_AddClass(CSelection& _selection, const char* _pClassName);
	dllvirtual void Selection_AddOnFlagsSet(CSelection& _selection, int _Flags);
	dllvirtual void Selection_AddBoundSphere(CSelection& _selection, int _ObjectFlags, const CVec3Dfp32& _Center, fp32 _Radius);
	dllvirtual void Selection_AddBoundBox(CSelection& _selection, int _ObjectFlags, const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax);
	dllvirtual void Selection_AddOriginInside(CSelection& _selection, const CMat4Dfp32& _Pos, const CWO_PhysicsState& _PhysState);
	dllvirtual void Selection_AddIntersection(CSelection& _selection, const CMat4Dfp32& _Pos, const CWO_PhysicsState& _PhysState);
	dllvirtual void Selection_AddIntersection(CSelection& _selection, const CVec3Dfp32& _Pos, const CWO_PhysicsState& _PhysState);

	// NOTE: The Selection_RemoveOnXxxx family of functions are not fully implemented.
	dllvirtual void Selection_RemoveOnIndex(CSelection& _selection, int _iObject);
	dllvirtual void Selection_RemoveOnFlagsSet(CSelection& _selection, int _Flags);						// Impl.
	dllvirtual void Selection_RemoveOnNotFlagsSet(CSelection& _selection, int _Flags);					// Impl.
	dllvirtual void Selection_RemoveOnClass(CSelection& _selection, int _iClass);						// Impl.
	dllvirtual void Selection_RemoveOnClass(CSelection& _selection, const char* _pClassName);			// Impl.
	dllvirtual void Selection_RemoveOnNotClass(CSelection& _selection, int _iClass);					// Impl.
	dllvirtual void Selection_RemoveOnNotClass(CSelection& _selection, const char* _Class);				// Impl.
	dllvirtual void Selection_RemoveObjects(CSelection& _selection, const int16* _piObjects, int _nObjects);	// Impl.
	dllvirtual bool Selection_ContainsObject(const CSelection& _selection, int _iObject);
	dllvirtual bool Selection_IntersectLine(const CSelection& _selection, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL, int _iExclude = 0);
	dllvirtual int  Selection_Get(const CSelection& _selection, const int16** _ppRetList = NULL );
	dllvirtual int  Selection_GetSingleClass(const char* _pClass);	// Find a single object. (Randomized if target matches several objects)
	dllvirtual void Selection_Dump(const CSelection& _selection, CStr _DumpName);

	// Object event listener management
	dllvirtual void Object_AddListener(uint _iObject, uint _iListener, uint16 _EventMask = ~0);
	dllvirtual void Object_RemoveListener(uint _iObject, uint _iListener, uint16 _EventMaskToRemove = ~0);
	dllvirtual bool Object_HasListeners(uint _iObject) const; // (This is pretty stupid. I would rather use a ClientFlag for this)
	dllvirtual void Selection_AddListeners(CSelection& _Selection, uint _iObject, uint16 _EventMask = ~0);
	dllvirtual void Object_NotifyListeners(uint _iObject, uint16 _EventMask);

	// Time management
	M_INLINE CMTime GetGameTime() const			{ return m_SimulationTime; }	// Scaled game time
	M_INLINE int GetGameTick() const			{ return m_SimulationTick; }	// Game tick no.
	M_INLINE fp32 GetGameTickTime() const		{ return m_TickTime; }			// Time per Tick (default 0.050s <-> 20hz)
	M_INLINE fp32 GetGameTicksPerSecond() const	{ return m_RefreshRate; }
	M_INLINE fp32 GetGameTickRealTime() const	{ return m_TickRealTime; }		// Time per Tick in unscaled system time
	M_INLINE fp32 GetTimeScale() const			{ return m_TimeScale; }	

	// Debug rendering utilities
	virtual CRenderContext* Debug_GetRender() pure;
	virtual CDebugRenderContainer* Debug_GetWireContainer(uint _Flags = 3) pure;

	// For convenience the CWireContainer interface is mirrored below, for example,
	// Debug_RenderWire(...) is equivalent to "if (Debug_GetWireContainer()) Debug_GetWireContainer()->RenderWire(...)"
	dllvirtual void Debug_RenderWire(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderVertex(const CVec3Dfp32& _p, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderVector(const CVec3Dfp32& _p, const CVec3Dfp32& _v, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderMatrix(const CMat4Dfp32& _Mat, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderMatrix(const CMat4Dfp32& _Mat, fp32 _Duration = 1.0f, bool _bFade = true, CPixel32 _ColorX = 0xff7f0000, CPixel32 _ColorY = 0xff007f00, CPixel32 _ColorZ = 0xff00007f);
	dllvirtual void Debug_RenderQuaternion(const CVec3Dfp32& _p, const CQuatfp32& _Quat, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderAxisRot(const CVec3Dfp32& _p, const CAxisRotfp32& _AxisRot, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderAABB(const CVec3Dfp32& _Min, const CVec3Dfp32& _Max, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderAABB(const CBox3Dfp32& _Box, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderOBB(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Extents, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderOBB(const CMat4Dfp32& _Pos, const CBox3Dfp32& _Box, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderSphere(const CMat4Dfp32& _PosMat, fp32 _Radius, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderSphere(const CVec3Dfp32& _Pos, fp32 _Radius, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);
	dllvirtual void Debug_RenderSkeleton(class CXR_Skeleton* _pSkel, class CXR_SkeletonInstance* _pSkelInst, CPixel32 _Color = 0xff007f3f, fp32 _Duration = 30.0f, bool _bFade = true);
	dllvirtual void Debug_RenderText(const CVec3Dfp32& _Pos, const char* _pText, CPixel32 _Color = 0xff7f7f7f, fp32 _Duration = 1.0f, bool _bFade = true);

	friend class CWObject;
	// TODO: Murray. Could be avoided.
	friend class CWO_DynamicsCollider;
	friend class CWD_WorldModelCollider;
};

#endif // _INC_WPHYSSTATE
