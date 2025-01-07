#ifndef __WOBJ_SCENEPOINT
#define __WOBJ_SCENEPOINT

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_ScenePoint

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWO_ScenePoint
					CWObject_ScenePoint
					CWObject_ScenePointManager
\*____________________________________________________________________________________________*/

#include "../../Shared/MOS/Classes/GameWorld/WObjCore.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "../WObj_AI/AI_Auxiliary.h"
#include "../WObj_AI/AI_Auxiliary.h"

class CAI_Resource_Activity;
class CWObject_ScenePointManager;



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWO_ScenePoint
					
	Comments:		AI Scene Points
\*____________________________________________________________________*/
class CWO_ScenePoint
{
public:
	enum
	{
		MAXUSERS = 8,
		MAXTEAMS = 8,
	};

	// Sceneposition type flags
	enum {
		INVALID			= 0,
		// Types:
		ROAM			= M_Bit(0),	// Point to roam towards
		SEARCH			= M_Bit(1),	// Point to search, sweep look through front arc
		TACTICAL		= M_Bit(2),	// Point to fight with ranged weapons from, given that target is within front arc
		COVER			= M_Bit(3),	// Point to hide when target is within front arc
		TALK			= M_Bit(4),	// Common point to talk at
		LOOK			= M_Bit(5),	// Point to look at, given that we are within front arcwell 
		DOOR			= M_Bit(6),	// Door scenepoint, used to connect rooms
		// Flags:
		WALK_CLIMB		= M_Bit(7),	// Wall point to pathfind to
		JUMP_CLIMB		= M_Bit(8),	// Wall point to jump to
		
		DARKLING		= M_Bit(9),	// Darkling SP, all types from above apply as well
		DYNAMIC			= M_Bit(10),	// Potentially moving SP, can fit any of the above types as well

		TYPE_MASK		= ROAM | SEARCH | TACTICAL | COVER | TALK | LOOK | DOOR,
		ALL = ~0,
	};
	static const char * ms_lTranslateType[]; 

	//TacFlags
	// Some notes about m_TacFlags:
	// If TACFLAGS_CROUCH and TACFLAGS_STAND user can transition between the two stances but cannot do popups
	// If TACFLAGS_COWARD, forward coward attacks can only be performed when TACFLAGS_STAND is NOT set
	// TACFLAGS_STAND but no TACFLAGS_CROUCH is legit AND reasonable.
	// TACFLAGS_STRAFE must stand first, this IS ok even when there's no TACFLAGS_STAND (stupid but OK)
	enum {
		TACFLAGS_LEFT			= 0x1,		// Left corner allowed
		TACFLAGS_RIGHT			= 0x2,		// Right corner allowed
		TACFLAGS_STAND			= 0x4,		// Standing, back to wall
		TACFLAGS_CROUCH			= 0x8,		// Crouching, facing wall	// *** Use regular crouch flag? ***
		TACFLAGS_POPUP			= 0x10,		// Checking sighting without fire
		TACFLAGS_COWARD			= 0x20,		// From crouch unaimed fire (requires FLAGS_CROUCH above)
		TACFLAGS_EXIT			= 0x40,		// Strafe/charge left or right as given by TACFLAGS_LEFT and TACFLAGS_RIGHT
		TACFLAGS_CHARGE			= 0x80,		// Rush out to charge the enemy
		TACFLAGS_FIRE			= 0x100,	// Shooting allowed, currently defaulted on
		TACFLAGS_CHANGEPOSTURE	= 0x200,	// Change from crouch to stand or vice versa
		TACFLAGS_NOTARGET		= 0x400,	// No target required
		TACFLAGS_LEFTRIGHT		= TACFLAGS_LEFT | TACFLAGS_RIGHT,
	};

	//Resource settings
	enum {
		RESOURCE_MINRESERVATIONTIME = 10,
		RESOURCE_POLLINTERVAL = 1,
	};

public:

	//Secondary scenepoint struct
	class CSecondaryScenePoint 
	{
	public:
		//Scenepoint index
		int16 m_iSP;
		int16 m_Flags;
		//Event action
		enum {
			ACTION_NONE					= 0,	//Do nothing
			ACTION_ONTAKE_SPAWN			= 1,	//Spawn secondary when SP is taken
			ACTION_ONRELEASE_UNSPAWN	= 2,	//UnSpawn secondary when SP is released
			ACTION_ONTAKE_RAISE			= 4,	//Raise secondary prio when SP is taken
			ACTION_ONRELEASE_LOWER		= 8,	//Lower secondary prio when SP is released
			ACTION_ONMOVE_UNSPAWN		= 16,	//Unspawn secondary when SP is moved
		};
		static const char * ms_lTranslateActions[]; 

		enum {
			EVENT_NONE = 0,		//No trigger
			EVENT_ONTAKE,		//Take action when scenepoint is taken
			EVENT_ONACTIVATE,	//Take action when scenepoint is activated
			EVENT_ONRELEASE,	//Take action when scenepoint is released
		};

		CSecondaryScenePoint(int16 _iSP=-1, int16 _Flags = ACTION_ONTAKE_RAISE | ACTION_ONRELEASE_LOWER)
		{
			m_iSP = _iSP;
			m_Flags = _Flags;
		};
		friend class CWObject_ScenePointManager;
	};

protected:
	// Struct for enginepath-scenepoint index pairs
	class CEnginepathScenePoint
	{
		public:
			int32 m_iSP;
			int32 m_iEP;

		CEnginepathScenePoint(int32 _iSP = 0, int32 _iEP = 0)
		{
			m_iSP = _iSP;
			m_iEP = _iEP;
		};
	};

protected:
	// 16 byte aligned things first
	//World position/rotation
	CMat4Dfp32 m_Position;
	//Dynamic scenepoint's position relative to parent
	CMat4Dfp32 m_LocalPos;
	//Offset from origin where behaviours should start
	CVec3Dfp32 m_Offset;
	bool m_bReverseDir;
	//Allows manager to identify scenepoint for script activation purposes
	CStr m_TargetName;

	//The messages to send when activated/taken/released
	TArray<CWO_SimpleMessage> m_lMessages_Activate;
	TArray<CWO_SimpleMessage> m_lMessages_Take;
	TArray<CWO_SimpleMessage> m_lMessages_Release;
	TArray<CWO_SimpleMessage> m_lMessages_Animation;

	//Normalized forward heading vector. Used for performance purposes for scene points with the FLAGS_ARCHEADINGONLY flag
	CVec3Dfp32 m_FwdHeading;
	fp32 m_ArcOffset;			// Arc offset in radians (CCW)
	CWObject_Room* m_pRoom;		// Use instead of arc when non NULL

	//Position suitable for pathfinding use.
    CVec3Dfp32 m_PathPos;

	//Squared radius, i.e. range within which we consider a position "at" scene point
	fp32 m_SqrRadius;

	//Cosine of allowed angle diff with scene point direction when we consider a direction aligned
	//with scene point. Also used to determine width of arc, see below.
	fp32 m_CosAngle;

	//Squared range of front arc. The area defined by this and angle can be used to determine whether scene point 
	//is useful (for example a cover scene point is considered to provide cover from enemy within the arc etc)
	fp32 m_SqrArcMinRange;
	fp32 m_SqrArcMaxRange;

	//After activation scenepoint can't get activated again until the reenabletick which is activation time plus delay
	uint16 m_DelayTicks;
	//Behaviour to be played upon activation
	uint16 m_iBehaviour;
	//Behaviour to be played by opponent upon activation
	uint16 m_iSecondaryBehaviour;	
	uint32 m_ReenableTick;
	fp32 m_BehaviourDuration;

	// Light intensity 0.0 to 1.0
	fp32 m_Light;
	// Last time we was measured, 0 indicates we have never been measured
#define SCENEPOINT_LIGHT_PERIOD	1200
	int m_LightTick;

	//Priority of scenepoint. Scene points with higher prio should be used when possible.
    uint8 m_Prio;
	bool	m_bUnspawnOnRelease;	// Unspawn the scenepoint on release if this flag is set
	int16	m_iConnectedLight;		// Light we're connected to (if any)
	uint16	m_ActivateFlags;		// Various flags dealing with activation/release
	enum {
		FLAGS_ACTIVATE_RELEASE	= 0x1,	// When true, OnRelease messages are only sent if SP was actually activated
		FLAGS_ACTIVATED			= 0x2,	// Set to true when activated, false when released but only if FLAGS_ACTIVATE_RELEASE is set
	};

	//Flags
	enum {
		FLAGS_WAITSPAWN			= 0x1, //Scenepoint can't be used until it's "spawned"
		FLAGS_HEADINGONLY		= 0x2, //When checking direction diff, for example if within scene point front arc or alignment, we only care about heading
		FLAGS_CROUCH			= 0x4, //Crouch on activate suggested
		FLAGS_DYNAMICS			= 0x8, // Temp flags to simplify Ogier look
		FLAGS_MOVING			= 0x10, // Dynamic scenepoint is currently moving
		FLAGS_SIT				= 0x20, // No animphys, no pathfind last 16 units toward SP
		FLAGS_LOWPRIO			= 0x40, // Scenepoint has permanent low prio
		FLAGS_PLAYONCE			= 0x80,	// Do not loop the behaviour
		FLAGS_LIGHTCONNECTED	= 0x0100, // Connected to a light
		FLAGS_COMBATRELEASE		= 0x0200, // OnRelease only played when in combat
		FLAGS_ALLOWMOVING		= 0x0400, // Valid even if moving
		FLAGS_NO_TRACECHECKS	= 0x0800, // Don't check traceline validity
		FLAGS_ALLOW_NEAR		= 0x1000, // Don't check nearby scenepoints
	};
	uint16 m_Flags;

	uint16 m_TacFlags;

	// Objects (namehash) allowed to use the scene point. If no users are specified, all objects can use the scene point
	CNameHash m_lUsers[MAXUSERS];

	// Teams (obj index) allowed to use the scene point. If no teams are specified, you can use the scenepoint regardles of which team you belong to
	uint16 m_liTeams[MAXTEAMS];

	//Type (bitfield)
	uint16 m_Type;

	//Parent CWObject for dynamic scenepoints
	int16 m_iParent;

	//Internal resource handler. Should perhaps use a simpler resource handler though.
	CAI_Resource_Activity m_Resource;

	//Secondary scenepoint list
	TThinArray<CSecondaryScenePoint> m_lSecondarySPs;
	//Enginepath scenepoint list
	TThinArray<CEnginepathScenePoint> m_lEnginepathSPs;

	//Unique ID
	uint32 m_ID;

	//Perform secondary scene point event
	void SecondaryScenePointEvent(int _Event, CWObject_ScenePointManager* _pSPM);

	friend class CWObject_ScenePoint;
	friend class CWObject_ScenePointManager;
	friend class CWO_ScenePointHash;
	friend class CWObject_SwingDoor;

public:
	CWO_ScenePoint();

	// Give the SP a new light measurement and a new timestamp
	void UpdateLight(fp32 _Light,int _Tick);

	//Try to activate scene point and return success/failure. Scene point can't be activated for 
	//delay ticks after last activation.
	bool Activate(int _iActivator, CWorld_Server* _pWServer, CWObject_ScenePointManager* _pSPM);

	//Spawn/unspawn scenepoint
	void Spawn(CWObject_ScenePointManager* _pSPM);
	void UnSpawn(CWObject_ScenePointManager* _pSPM);

	// Returns a direction within the arc of the SP with _Period
	// If arc is less than 360 degrees the sweep will oscillate
	CVec3Dfp32 GetSweepDir(int32 _Period, int32 _Timer);

	//Check if given normalized vector is sufficiently aligned with scene points direction
	bool IsAligned(CVec3Dfp32 _Dir, bool _bReverseDir = false);

	// Makes the scenepoint invalid (ie ::Request fails) for _Duration serverticks
	void InvalidateScenepoint(CWorld_Server *_pWServer, int _Duration);

	//Request use of scene point for given time (frames, default is current frame only) with given priority 
	//(or very low prio as default). Succeed if we're granted request, fail if not.
    bool Request(int _iUser, CWorld_Server* _pWServer, CWObject_ScenePointManager* _pSPM, int _Duration = 0, uint8 _Prio = 1);	

	//Check if we can sucessfully request use of this scene point without reserving use
	M_FORCEINLINE bool PeekRequest(int _iUser, CWorld_Server* _pWServer, uint8 _Prio = 1)
	{	
		//Not available if unspawned
		if (m_Flags & FLAGS_WAITSPAWN)
			return false;

		// Check reenable
		if (_pWServer->GetGameTick() < m_ReenableTick)
			return false;

		//Check resource manager
		if (m_Resource.IsInitialized())
		{	//Peek resource manager
			return (_pWServer && m_Resource.Peek(_iUser, _Prio, _pWServer->GetGameTick()));
		}
		else
		{
			//No resource, unlimited users.
			return true;
		}
	};

	//Notify scenepoint that we don't use it anymore
	void Release(int _iUser, CWorld_Server* _pWServer, CWObject_ScenePointManager* _pSPM);
						
	// Trigger animation events that fit _iEvent, if any
	void AnimationEvent(int _iUser, int _iEvent, CWorld_Server* _pWServer, CWObject_ScenePointManager* _pSPM);

	//Check if the given user (who belongs to the given teams) can use this scene point. Scene point also has to be spawned of course.
	//bool IsValid(int _iObject, int _iOwner = 0, const int * _lTeams = NULL, int _nTeams = 0, CWObject_Room* _pRoom = NULL);
	bool IsValid(CWorld_Server* _pWServer, uint32 _ObjectName, uint32 _OwnerName = 0, const uint16* _piTeams = NULL, uint _nTeams = 0, CWObject_Room* _pRoom = NULL) const;

	// Checks if _Type is compatible with the SP
	M_FORCEINLINE bool CheckType(const int _Type) const
	{
		// We don't ask for DYNAMIC SPs so we shouldn't get any
		if (!(_Type & CWO_ScenePoint::DYNAMIC) && (m_Type & CWO_ScenePoint::DYNAMIC))
		{
			return(false);
		}

		if ((m_Type & CWO_ScenePoint::DYNAMIC)&&(!(m_Flags & FLAGS_ALLOWMOVING)))
		{	// All DYNAMIC aside from those explicitly allowed must be semistationary before they can be taken
			if (m_Flags & FLAGS_MOVING)
			{
				return(false);
			}
		}

		// If we ask for LOOK we require LOOK
		if ((_Type & CWO_ScenePoint::LOOK)&&!(m_Type & CWO_ScenePoint::LOOK))
		{
			return(false);
		}

		if (!((m_Type & ~CWO_ScenePoint::DYNAMIC)&(_Type)))
		{	// Only DYNAMIC are shared between m_Type and _Type
			return(false);
		}

		if (_Type & CWO_ScenePoint::DARKLING)
		{
			if (m_Type & CWO_ScenePoint::DARKLING)
			{	// TYPE_MASK: ROAM | SEARCH | TACTICAL | COVER | TALK | LOOK | DOOR
				if ((m_Type & _Type) & CWO_ScenePoint::TYPE_MASK)
				{
					return(true);
				}
				else
				{
					return(false);
				}
			}
			else
			{
				return(false);
			}
		}
		else
		{
			if (m_Type & CWO_ScenePoint::DARKLING)
			{
				return(false);
			}
			else
			{
				if (m_Type & _Type & ~CWO_ScenePoint::DARKLING)
				{
					return(true);
				}
				else
				{
					return(false);
				}
			}
		}
	};

	// Searches for the first Speak message in OnActivate and returns its number
	int GetSpeakDialog();

	//Check if given position is in front arc of scene point
	bool InFrontArcImpl(const CVec3Dfp32& _Pos, const CVec3Dfp32& _ThisPos);

	M_FORCEINLINE bool InFrontArc(const CVec3Dfp32& _Pos, bool _bUseOffset = true)
	{
		return InFrontArcImpl(_Pos, GetPosition(_bUseOffset));
	}

	// true when dir lies within the front arc, false if not
	bool DirInFrontArc();

	//Check if given position is sufficiently near scene point
	bool IsAt(const CVec3Dfp32& _Pos,bool _bUseOffset = true);

	//Get the Nth secondary scene point or NULL if there are none at or after given index
	CVec3Dfp32& GetPathPosition(CWorld_Server* _pWServer);

	M_FORCEINLINE CVec3Dfp32 GetPosition(bool _bUseOffset = true) const
	{
		if ((_bUseOffset)&&(VALID_POS(m_Offset)))
		{
			CVec3Dfp32 Result = m_Position.GetRow(3);
			Result += m_Position.GetRow(0) * m_Offset[0];	// Fwd
			Result += m_Position.GetRow(1) * m_Offset[1];	// Left
			Result += m_Position.GetRow(2) * m_Offset[2];	// Up (Should really be zero but...better be safe than sorry)
			return(Result);
		}
		else
		{
			return (m_Position.GetRow(3));
		}
	}

	M_FORCEINLINE const CVec3Dfp32& GetPos() const { return(m_Position.GetRow(3)); };

	M_FORCEINLINE CVec3Dfp32 GetDirection(bool _bUseOffset = true)
	{
		CVec3Dfp32 Dir;
		if ((_bUseOffset)&&(m_bReverseDir))
		{
			return(-CVec3Dfp32::GetMatrixRow(m_Position, 0));
		}
		else
		{
			return(CVec3Dfp32::GetMatrixRow(m_Position, 0));
		}
	};

	// ROAM; no behaviour, 180+ arc halfangle, radius 16+, duration 0
	bool IsWaypoint();

	void SetOffsetAndSecondaryBehaviour(uint16 _iSecondaryBehaviour, CVec3Dfp32& _Offset, bool _bReverseDir);
	inline int16 GetConnectedLight()
	{
		if ((m_Type & LOOK)&&(m_Flags & FLAGS_LIGHTCONNECTED))
		{
			return(m_iConnectedLight);
		}
		else
		{
			return(0);
		}
	};

	inline const CVec3Dfp32& GetUp() const { return CVec3Dfp32::GetMatrixRow(m_Position, 2); }
	inline const CMat4Dfp32& GetPositionMatrix() const { return m_Position; }
	inline const CMat4Dfp32& GetLocalPositionMatrix() const { return m_LocalPos; }
	inline       int GetType() const { return m_Type; }
	inline       fp32 GetSqrArcMaxRange() const { return m_SqrArcMaxRange; };
	inline       fp32 GetSqrArcMinRange() const { return m_SqrArcMinRange; };
	inline       fp32 GetCosArcAngle() const { return m_CosAngle; };
	inline       fp32 GetSqrRadius() const { return m_SqrRadius; };
	inline       uint16 GetBehaviour() const { return m_iBehaviour; };
	inline       int16 GetSecondaryBehaviour() const { return m_iSecondaryBehaviour; };

	inline		fp32 GetBehaviourDuration() const { return m_BehaviourDuration; };
	inline		bool GetAllowNearFlag() const { return((m_Flags & FLAGS_ALLOW_NEAR) ? true : false); };
	inline		bool GetNotraceFlag() const { return((m_Flags & FLAGS_NO_TRACECHECKS) ? true : false); };
	inline		bool GetCrouchFlag() const { return((m_Flags & FLAGS_CROUCH) ? true : false); };
	inline		bool GetSitFlag() const { return((m_Flags & FLAGS_SIT) ? true : false); };
	inline		bool PlayOnce() const { return((m_Flags & FLAGS_PLAYONCE) ? true : false); };
	inline		bool IsSpawned() const { return((m_Flags & FLAGS_WAITSPAWN) ? false : true); };
	inline		uint16 GetNthTeam(uint _iTeam) const { M_ASSERT(_iTeam < MAXUSERS, "Team index out of range"); return m_liTeams[_iTeam]; }
	inline		int32 GetNthUserName(uint _iUser) const { M_ASSERT(_iUser < MAXUSERS, "User index out of range"); return m_lUsers[_iUser]; }
	inline const CStr& GetName() const { return m_TargetName; };
	inline		uint8 GetPrio() const { return m_Prio; };
	inline		bool GetLowPrioFlag() const {return ((m_Flags & FLAGS_LOWPRIO) ? 1 : 0); };
	inline		void SetLowPrioFlag(bool _State)
				{
					if (_State)
					{
						m_Flags |= FLAGS_LOWPRIO;
					}
					else
					{
						m_Flags &= ~FLAGS_LOWPRIO;
					}
				};
	             void SetLight(fp32 _Light,int _Tick);
	inline		fp32 GetLight() const { return(m_Light); };
	inline		int32 GetLightTick() const { return(m_LightTick); };
	inline		void RaisePrio() {m_Prio = 1;}
	inline		void RestorePrio() {m_Prio = 0;}
	inline		void SetUnspawnOnReleaseFlag(bool _bFlag) { m_bUnspawnOnRelease = _bFlag; }
	inline		int16 GetParent() const { return m_iParent; }
	inline		int32 GetEPSPCount() const { return m_lEnginepathSPs.Len(); }
	inline		uint16 GetTacFlags()  const { return(m_TacFlags & ~TACFLAGS_NOTARGET); }
	inline		bool GetNoTargetTacFlag()  const { return((m_TacFlags & TACFLAGS_NOTARGET) ? true : false); };
	             bool IsValidDynamic() const;	// true if not DYNAMIC, true if DYNAMIC and !FLAGS_MOVING and Up within threshold
	inline		void HandleCombatReleaseFlag()
				{
					if ((m_Flags & FLAGS_COMBATRELEASE)&&(m_ActivateFlags & FLAGS_ACTIVATED))
					{
						m_ActivateFlags &= ~FLAGS_ACTIVATED;
					}
				};
};


// Scenepoint initblock
class CWO_ScenePointInitBlock
{
public:
	TArray<CStr> m_lSecondarySPNames;
	TArray<CStr> m_lEnginepathSPNames;
	TArray<CStr> m_lEnginepathEPNames;
	TArray<CNameHash> m_lUsers;
	TArray<CNameHash> m_lTeams;
	TArray<int16> m_lSecondarySPActions;
	CNameHash		m_RoomNameHash;
	CNameHash		m_ConnectedLightHash;
	uint32 m_ParentNameHash;
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWObject_ScenePoint
					
	Comments:		Game object that is removed directly after spawned.

	See also:		CWO_ScenePoint and CWObject_ScenePointManager
\*____________________________________________________________________*/
class CWObject_ScenePoint : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	//"Constant" helper array
	static int ms_2DDirs[8][2];	

public:

	CWObject_ScenePoint();
	CWO_ScenePoint m_Point;
	CWO_ScenePointInitBlock m_InitBlock;
	// Priority activate messages, to be inserted before all other activate messages
	TArray<CWO_SimpleMessage> m_lMessages_Activate_Prio;
	int8 m_NumUsers;
	int16 m_iScenePoint;

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	//Code copied from CAI_Pathfinder.
	//Returns the closest position above or below the given position which is a traversable ground cell, 
	//or fails with CVec3Dfp32(_FP32_MAX). If the optional _iMaxDiff argument is greater than -1, then this is 
	//the maximum height-difference in cells tolerated. If the ordinary algorithm fails and the optional 
	//_iRadius argument is greater than 0, it will continue to check _iRadius cell-columns in eight directions 
	//outward from the given position.
	static CVec3Dfp32 GetPathPosition(CWorld_Server* _pWServer, const CVec3Dfp32& _Pos, int _iMaxDiff = -1, int _iRadius = 0);

	//virtual void OnSpawnWorld();
	//virtual void OnSpawnWorld2();
};




/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWO_ScenePointHash
					
	Comments:		Used for quick scenepoint position lookup.
					Used by CWObject_ScenePointManager
\*____________________________________________________________________*/
class CWO_ScenePointHash
{
public:
	enum { 
		SET_REGULAR = 0,
		SET_DARKLING,
		SET_DYNAMIC,
		NUM_SETS,

		NUM_TYPES = 7,						// This must match the CWO_ScenePoint type enums
		NUM_GRIDS = NUM_TYPES * NUM_SETS,
	};

protected:
	struct HashGrid
	{
		uint m_nBoxNum;					// Number of cells in each direction
		uint m_nBoxNumShift;
		uint m_nBoxNumMask;
		uint m_nBoxSize;
		uint m_nBoxSizeShift;
		uint m_nBuckets;				// Number of elements of 2D grid (plus one)
		fp32  m_Range;
		TThinArray<uint16> m_lHash;		// lookup, Hash -> Link index
		uint16*            m_pHash;

		M_INLINE uint GetHashIndex(uint _x, uint _y) const  { return _x + (_y << m_nBoxNumShift) + 1; }
	};

	struct HashLink
	{
		uint16 m_iNext;			// link to next element in bucket
		uint16 m_iPrev;			// link to prev element in bucket
		uint16 m_iHash;			// lookup, link -> hash bucket index
		uint16 m_iGrid;			// tells which grid the hash-index applies to
		uint16 m_iPoint;		// Scenepoint index
	};

	HashGrid             m_Grids[NUM_GRIDS];
	TThinArray<HashLink> m_lLinks;
	HashLink*            m_pLinks;

protected:
	void Insert(uint8 _iGrid, uint16 _ID, uint16 _iPoint, const CVec3Dfp32& _Pos);
	void Remove(uint16 _ID);

public:
	CWO_ScenePointHash() : m_pLinks(NULL) {}

	void Create(int _MaxIDs);
	void InitGrid(uint8 _iGrid, int _nBoxes, const CBox3Dfp32& _Bound);
	void DumpHashUsage() const;

	void Insert(const CWO_ScenePoint& _Point, uint16 _iPoint);
	void Remove(const CWO_ScenePoint& _Point);
	void Update(const CWO_ScenePoint& _Point, uint16 _iPoint);

	int EnumerateBox(uint8 _iGrid, const CBox3Dfp32& _Box, uint16* _pEnumRetIDs, uint _MaxEnumIDs) const;
	int EnumerateAll(uint8 _iGrid, uint16* _pEnumRetIDs, uint _MaxEnumIDs) const;
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWObject_ScenePointManager
					
	Comments:		Used to manage game scenepoints.
					Owned by CWObject_GameCore.
\*____________________________________________________________________*/
class CWObject_ScenePointManager : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	//All scenepoints
	TArray<CWO_ScenePoint> m_lScenePoints;
	TArray<CWO_ScenePointInitBlock> m_lScenePointInitBlocks;
	TArray<CWO_ScenePoint*> m_lActivePoints;   // dynamic scenepoints that are currently moving

	//Proximity hash for optimized scenepoint selection...
	CWO_ScenePointHash m_Hash;

	//Currently selected scenepoints
	TArray<CWO_ScenePoint *> m_lSelection;

	//List of all objects that can spawn other objects. Set when the GetSpawners method is first run. 
	//List is not saved.
	TArray<int> m_lSpawners;
	bool m_bHasSearchedForSpawners;
	uint32 m_nNextID;		// used to assign unique IDs to all scenepoints

protected:
	// Tells the scenepoint manager that an object is moving, thus allowing the manager to activate dynamic scenepoints
	void OnObjectMoved(const CWObject& _Obj);

	// Tells the scenepoint manager that an object is breaking, allowing the manager to set new owner for dynamic scenepoints
	void OnObjectBreaking(const CWObject& _Obj, fp32 _OldObjVolume, TAP<const uint16> _piNewObjects);

	//Init the space hash
	void InitHash(); 
	uint GetDynamicScenepoints(uint _iParentObj, CWO_ScenePoint** _ppRet, uint _nMaxRet);

public:
	//Init
	virtual void OnCreate();
	virtual void OnSpawnWorld2();
	virtual void OnRefresh(); //Refresh dynamic scenepoints
	virtual aint OnMessage(const CWObject_Message& _Msg);

	void InitManager();

	//Add scene point to manager. Return index.
	int AddScenePoint(const CWO_ScenePoint& _Point, const CWO_ScenePointInitBlock& _InitBlock);

	//Empty scenepoint selection
	void Selection_Clear();

	//Add all scenepoints that are within the given range of the given position, as well as a bunch that aren't :) 
	//Optionally we exclude those unavailable to the given teams and object. Also, if a lower range is provided, 
	//we exclude all those that would have been selected given that range.
	void Selection_AddChunk(const CVec3Dfp32& _Pos, fp32 _RangeDelimiter, uint32 _ObjectName = 0, uint32 _OwnerName = 0, const uint16* _piTeams = NULL, uint _nTeams = 0, CWObject_Room* _pRoom = NULL);
	void Selection_AddChunk_Hashed(const CVec3Dfp32& _Pos, fp32 _MaxRange, uint16 _Types, uint32 _ObjectName, uint32 _OwnerName, const uint16* _piTeams, uint _nTeams);
	void Selection_AddBox_Approx(const CBox3Dfp32& _Box, uint16 _Types);

	const TArray<CWO_ScenePoint*>& GetScenePoints(int _iObject, fp32 _MaxRadius, int _Types, CWObject_Room* _pRoom);

	//These methods are not optimized
	void Selection_AddAll();
	void Selection_AddRange(const CVec3Dfp32 &_Pos, fp32 _Range);
	void Selection_AddAffectingObject(uint32 _ObjectName, uint32 _OwnerName);
	void Selection_AddAffectingTeam(int _iTeam);

	void Selection_AddByName(const char* _pName);
	void Selection_AddInRoom(const CWObject_Room& _Room);
	void Selection_RemoveNotInRoom(const CWObject_Room& _Room);

	const TArray<CWO_ScenePoint*>& Selection_Get() const { return m_lSelection; }
	
	// Send a message to scenepoints.
	aint Message_SendToScenePoint(const CWObject_Message& _Msg, CWO_ScenePoint* _pSP);
	aint Message_SendToSelection(const CWObject_Message& _Msg);

	//Find first scenepoint given target name. Not optimized yet.
	CWO_ScenePoint* Find(const char *_pTargetName);

	//Find random scenepoint given target name. Not optimized yet.
	CWO_ScenePoint* FindRandom(int _iObject,const char *_pTargetName);

	// Find a random scenepoint of the given type(s) in _pRoom
	CWO_ScenePoint* FindRandomInRoom(uint32 _Type, const CWObject_Room* _pRoom);

	// Spawn(_bSpawn == true), Unspawn(_bSpawn == false) all scenepoints with _TargetName
	// Returns nbr of SPs affected
	int32 SpawnNamed(CStr _TargetName,bool _bSpawn);

	//Get object indices of all spawners. Optionally only spawners whose spawn have the given target name are considered.
	TArray<int> GetSpawners(CStr _TargetName = "");

	//Get scenepoint list index of given scenepoint. Currently only used for save/load purposes
	//Return -1 if manager can't find scenepoint
	int GetScenePointIndex(const CWO_ScenePoint * _pScenePoint);

	//Get scenepoint from index. This is currently used to preserve scenepoints over saves.
	CWO_ScenePoint* GetScenePointFromIndex(int _i);

	// Returns the nbr of enginepath SPs for the given SP
	int32 GetEnginepathSPCount(CWO_ScenePoint* _pScenePoint);
	// Returns the Nth enginepath SP or NULL if outside range
	CWO_ScenePoint* GetNthEnginepathSP(CWO_ScenePoint* _pScenePoint, int32 _iESP);
	// Returns the enginepath ID for going from _pFromSP to _pToSP
	int32 GetEnginepathID(CWO_ScenePoint* _pFromSP,CWO_ScenePoint* _pToSP);

	// Measure light at scenepoint
	void SetScenePointLight(CWO_ScenePoint* _pScenePoint,fp32 _Light,int _Tick);
	void SetScenePointLight(int _i,fp32 _Light,int _Tick);
};




#endif
