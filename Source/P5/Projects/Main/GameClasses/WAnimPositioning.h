#ifndef _INC_WANIMPOSITIONING
#define _INC_WANIMPOSITIONING

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Animation positioning
					
	Author:			Olle Rosenquist
					
	Copyright:		2003 Starbreeze Studios AB
					
	Contents:		

	Comments:
					
	History:		
		030912:		Created

\*____________________________________________________________________________________________*/

typedef TPtr<class CActionCutsceneCamera> spCActionCutsceneCamera;

// Hmm, yes... We want hardcoded, or templated positions for our relative animation positions
// We also want these as offsets from a center point and direction
enum
{
	RELATIVEANIMPOS_MOVE_UNARMED_BREAKNECK					= 1,
	RELATIVEANIMPOS_MOVE_UNARMED_GETNECKBROKEN,
	RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_PUSH,
	RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GETPUSHED,

	RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABSLOW,
	RELATIVEANIMPOS_MOVE_UNARMED_SNEAK_GRABBEDSLOW,

	RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABSLOW,
	RELATIVEANIMPOS_MOVE_CLUB_SNEAK_GRABBEDSLOW,

	RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABSLOW,
	RELATIVEANIMPOS_MOVE_SHANK_SNEAK_GRABBEDSLOW,

	RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_UNARMED_MIDDLE,
	RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_CLUB_MIDDLE,
	RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_SHANK_MIDDLE,
	RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_PISTOL,
	RELATIVEANIMPOS_MOVE_UNARMED_COUNTER_ASSAULT,

	RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_RIGHT,
	RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_LEFT,
	RELATIVEANIMPOS_MOVE_CLUB_COUNTER_UNARMED_MIDDLE,
	RELATIVEANIMPOS_MOVE_CLUB_COUNTER_CLUB_MIDDLE,
	RELATIVEANIMPOS_MOVE_CLUB_COUNTER_SHANK_MIDDLE,
	RELATIVEANIMPOS_MOVE_CLUB_COUNTER_PISTOL,
	RELATIVEANIMPOS_MOVE_CLUB_COUNTER_ASSAULT,

	RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_RIGHT,
	RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_LEFT,
	RELATIVEANIMPOS_MOVE_SHANK_COUNTER_UNARMED_MIDDLE,
	RELATIVEANIMPOS_MOVE_SHANK_COUNTER_CLUB_MIDDLE,
	RELATIVEANIMPOS_MOVE_SHANK_COUNTER_SHANK_MIDDLE,
	RELATIVEANIMPOS_MOVE_SHANK_COUNTER_PISTOL,
	RELATIVEANIMPOS_MOVE_SHANK_COUNTER_ASSAULT,

	// Kinda bad names, but first weapon is the one we have, second weapon the weapon of the other
	// person and last bit is the move we last made (attack middle/right/left), or nothing in the
	// case of club/shank
	RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_UNARMED_MIDDLE,
	RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_RIGHT,
	RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_LEFT,
	RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_CLUB_MIDDLE,
	RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_RIGHT,
	RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_LEFT,
	RELATIVEANIMPOS_MOVE_UNARMED_RESPONSE_COUNTER_SHANK_MIDDLE,
	RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_UNARMED,
	RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_CLUB,
	RELATIVEANIMPOS_MOVE_CLUB_RESPONSE_COUNTER_SHANK,
	RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_UNARMED,
	RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_CLUB,
	RELATIVEANIMPOS_MOVE_SHANK_RESPONSE_COUNTER_SHANK,

	// Pistol responses
	RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_UNARMED,
	RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_CLUB,
	RELATIVEANIMPOS_MOVE_PISTOL_RESPONSE_COUNTER_SHANK,

	// Assault/shotgun responses
	RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_UNARMED,
	RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_CLUB,
	RELATIVEANIMPOS_MOVE_ASSAULT_RESPONSE_COUNTER_SHANK,

	RELATIVEANIMPOS_MOVE_CLUB_BREAKNECK,
	RELATIVEANIMPOS_MOVE_CLUB_GETNECKBROKEN,
	RELATIVEANIMPOS_MOVE_SHANK_BREAKNECK,
	RELATIVEANIMPOS_MOVE_SHANK_GETNECKBROKEN,


	RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL,
	RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED,

	RELATIVEANIMPOS_MOVE_TRANQ_STOMP,
	RELATIVEANIMPOS_MOVE_TRANQ_STOMPED,

	RELATIVEANIMPOS_MOVE_UNARMED_GRABSLOW_FAKE,
	RELATIVEANIMPOS_MOVE_CLUB_GRABSLOW_FAKE,
	RELATIVEANIMPOS_MOVE_SHANK_GRABSLOW_FAKE,


	RELATIVEANIMPOS_MOVE_NRMOVES = RELATIVEANIMPOS_MOVE_SHANK_GRABSLOW_FAKE,


	// No need for table offsets from ledges, only one fixed offset
	RELATIVEANIMPOS_MOVE_LEDGE_IDLE = RELATIVEANIMPOS_MOVE_SHANK_GRABSLOW_FAKE+1,
	RELATIVEANIMPOS_MOVE_LEDGE_LEFT,
	RELATIVEANIMPOS_MOVE_LEDGE_RIGHT,
	RELATIVEANIMPOS_MOVE_LEDGE_UPLOW,
	RELATIVEANIMPOS_MOVE_LEDGE_UPMEDIUM,
	RELATIVEANIMPOS_MOVE_LEDGE_JUMPUPANDGRAB,
	RELATIVEANIMPOS_MOVE_LEDGE_CLIMBUPFROMHANGING,
	RELATIVEANIMPOS_MOVE_LEDGE_ALTCLIMBUPFROMHANGING,
	RELATIVEANIMPOS_MOVE_LEDGE_CLIMBDOWNTOLEDGE,
	RELATIVEANIMPOS_MOVE_LEDGE_ALTCLIMBDOWNTOLEDGE,
	RELATIVEANIMPOS_MOVE_LEDGE_DROPDOWN,
	RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNERINLEFT,
	RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNEROUTLEFT,
	RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNERINRIGHT,
	RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNEROUTRIGHT,

	RELATIVEANIMPOS_MOVE_LEDGE_MAXNR = RELATIVEANIMPOS_MOVE_LEDGE_CLIMBCORNEROUTRIGHT,
	RELATIVEANIMPOS_MOVE_LEDGE_FIRSTMOVE = RELATIVEANIMPOS_MOVE_LEDGE_IDLE,

	RELATIVEANIMPOS_MOVE_LADDER_CLIMBUP,
	RELATIVEANIMPOS_MOVE_LADDER_SLIDEDOWN,
	RELATIVEANIMPOS_MOVE_LADDER_SLIDESTART,
	RELATIVEANIMPOS_MOVE_LADDER_SLIDESTOP,
	RELATIVEANIMPOS_MOVE_LADDER_SLIDESTOPEND,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBONDOWN,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBONUP,
	RELATIVEANIMPOS_MOVE_LADDER_IDLE,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL4PLUS,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL0,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL4MINUS,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL8MINUS,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPL12MINUS,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR4PLUS,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR0,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR4MINUS,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR8MINUS,
	RELATIVEANIMPOS_MOVE_LADDER_CLIMBOFFUPR12MINUS,

	RELATIVEANIMPOS_MOVE_HANGRAIL_IDLE,
	RELATIVEANIMPOS_MOVE_HANGRAIL_STARTFORWARD,
	RELATIVEANIMPOS_MOVE_HANGRAIL_FORWARD,
	RELATIVEANIMPOS_MOVE_HANGRAIL_TURN180,
	RELATIVEANIMPOS_MOVE_HANGRAIL_STOP,
	RELATIVEANIMPOS_MOVE_HANGRAIL_JUMPUP,

	
	RELATIVEANIMPOS_MOVE_LADDER_MAXNR = RELATIVEANIMPOS_MOVE_HANGRAIL_JUMPUP,
	RELATIVEANIMPOS_MOVE_LADDER_FIRSTMOVE = RELATIVEANIMPOS_MOVE_LADDER_CLIMBUP,	



	RELATIVEANIMPOS_FLAG_HASTARGETPOS			= 1 << 0,
	RELATIVEANIMPOS_FLAG_HASTARGETANGLE			= 1 << 1,
	RELATIVEANIMPOS_FLAG_HASMOVETOKEN			= 1 << 2,
	RELATIVEANIMPOS_FLAG_HASOTHERINDEX			= 1 << 3,
	RELATIVEANIMPOS_FLAG_HASANIMINDEX			= 1 << 4,
	RELATIVEANIMPOS_FLAG_HASSTARTPOS			= 1 << 5,
	//RELATIVEANIMPOS_FLAG_HASANGLEOFFSET			= 1 << 5,
	// Look/move of character enabled
	//RELATIVEANIMPOS_FLAG_MOVEENABLED			= 1 << 6,
	//RELATIVEANIMPOS_FLAG_LOOKENABLED			= 1 << 7,
	RELATIVEANIMPOS_FLAG_LINKED					= 1 << 6,
	
	RELATIVEANIMPOS_MASK_CANMOVE				= (RELATIVEANIMPOS_FLAG_HASTARGETPOS /*| RELATIVEANIMPOS_FLAG_MOVEENABLED*/),
	RELATIVEANIMPOS_MASK_CANLOOK				= (RELATIVEANIMPOS_FLAG_HASTARGETANGLE /*| RELATIVEANIMPOS_FLAG_LOOKENABLED*/),

	RELATIVEANIMPOS_MASK_COPYMASK				= (RELATIVEANIMPOS_FLAG_HASTARGETPOS | RELATIVEANIMPOS_FLAG_HASTARGETANGLE | RELATIVEANIMPOS_FLAG_HASMOVETOKEN | RELATIVEANIMPOS_FLAG_HASOTHERINDEX | RELATIVEANIMPOS_FLAG_HASANIMINDEX | RELATIVEANIMPOS_FLAG_HASSTARTPOS | RELATIVEANIMPOS_FLAG_LINKED/*| RELATIVEANIMPOS_FLAG_MOVEENABLED | RELATIVEANIMPOS_FLAG_LOOKENABLED*/),

	RELATIVEANIMPOS_SIGNAL_START				= 1,
	RELATIVEANIMPOS_SIGNAL_STOP					= 2,
	
	// DAMNIT, at somepoint we will need to create a new linked move...
	// Will have to reset startpos to endpos of old anim, and somehow find new animation
	// Find new animation through "action_Linkedmove"...
	// Will create a new move....
	RELATIVEANIMPOS_SIGNAL_LINKANIMATION		= 3,
	RELATIVEANIMPOS_SIGNAL_SETGRABDIFFICULTY	= 4,
	RELATIVEANIMPOS_SIGNAL_STOPBREAKNECK		= 5,
	RELATIVEANIMPOS_SIGNAL_KILLOTHER			= 6,
	RELATIVEANIMPOS_SIGNAL_WEAPONKILL			= 7,
};

// The things that are in this class is the things the lie in the client data (midpoint, angle 
// and movetype, angle is direction from perp to victim)
class CRelativeAnimPos
{
	class CAnimPosEntry
	{
	public:
		fp32 m_OffsetX;
		fp32 m_OffsetY;
		fp32 m_OffsetZ;
		fp32 m_AngleOffset;
		CAnimPosEntry(fp32 _OffsetX, fp32 _OffsetY, fp32 _OffsetZ, fp32 _AngleOffset)
		{
			m_OffsetX = _OffsetX;
			m_OffsetY = _OffsetY;
			m_OffsetZ = _OffsetZ;
			m_AngleOffset = _AngleOffset;
		}
	};
protected:

	// Not replicated over network
	CVec3Dfp32 m_StartPos;
	CVec3Dfp32 m_LastEndPos;
	CQuatfp32 m_StartRot;
	CQuatfp32 m_LastEndRot;
	// Data for character placement (midpoint is the ledge point for ledge moves)
	CVec3Dfp32 m_Midpoint;
	spCActionCutsceneCamera m_spActiveCutsceneCamera;
	fp32	m_Angle;
	fp32 m_TimeOffset;
	fp32 m_ControlLookX;
	uint32	m_MoveTokenStart;
	uint16 m_iOther;
	int16 m_iAnim;
	uint16 m_TickStartOffset;
	uint8 m_MoveType;
	uint8 m_Flags;

	static CAnimPosEntry m_slPosEntryTable[RELATIVEANIMPOS_MOVE_NRMOVES];
	void Clear();
public:
	CRelativeAnimPos();
	void CreateAnimMove(int32 _Move, const CWObject_CoreData* _pSelf, const CWObject_CoreData* _pOther, int16 _iAnim);

	// Ok, this will create a movetoken entry for you that will syncronize nicely with the
	// Other character (I hope..)
	/*void MakeSynchedAnim(CWorld_PhysState* _pWPhys, int32 _Move, CWO_Character_ClientData* _pCDSelf, CWObject_CoreData* _pSelf,
		CWO_Character_ClientData* _pCDOther, CWObject_CoreData* _pOther, fp32 _TimeOffset = 0.0f, fp32 _StartOffset = 0.0f);*/
	//void MakeSynchedAnimExtras(CWorld_PhysState* _pWPhys, int32 _Move, CWO_Character_ClientData* _pCDSelf, CWObject_CoreData* _pSelf, CWO_Character_ClientData* _pCDOther, CWObject_CoreData* _pOther);
	//bool MakeLedgeMove(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, int32 _Move, const CVec3Dfp32& _LeftPoint, const CVec3Dfp32& _RightPoint, const CVec3Dfp32& _Normal, fp32 _CurrentLedgePos, fp32 _RightLength);
	bool MakeLadderMove(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, int32 _Move, const CVec3Dfp32& _Bottom, const CVec3Dfp32& _Top, const CVec3Dfp32& _Normal, fp32 _CurrentLadderPos);
	bool GetLedgePoint(int32 _Move, int16 m_iAnim, const CVec3Dfp32& _LeftPoint, const CVec3Dfp32& _RightPoint, const CVec3Dfp32& _Normal, fp32 _CurrentLedgePos, fp32 _RightLength);
	bool GetLadderPoint(CWObject_CoreData* _pObj, int32 _Move, int16 m_iAnim, const CVec3Dfp32& _Bottom, const CVec3Dfp32& _Top, const CVec3Dfp32& _Normal, fp32 _CurrentLadderPos);
	bool GetStartPosition(CVec3Dfp32& _Position, CQuatfp32& _Rot) const;
	void GetTargetDirection(CVec3Dfp32& _CharLook) const;
	void ModerateLook(CWO_Character_ClientData* _pCD);
	void InvalidateTargetPos();
	void InvalidateTargetAngle();

	void GetMidPoint(const CWObject_CoreData* _pSelf, const CWObject_CoreData* _pOther);
	
	// Effects of movetype
	//void DoStart(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhys);
	//void DoStop(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhys);
	// Refresh camera (if any)
	void OnRefresh();
	
	// For now this is only done on the server..
	//void CheckForMoveTokens(CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, CWorld_PhysState* _pWPhys);

	MACRO_INLINEACCESS_R(TimeOffset,fp32)

	// Signal start/stop of animamove (and other things later maybe)....
	//bool Signal(int32 _Signal, const CWAG2I_Context* _pContext, const CWAG2I* _pAGI, bool _bInitiator = false);
	int16 LinkAnimations(const CWAG2I_Context* _pContext, const CWAG2I* _pAGI);

	M_INLINE bool IsMoving() { return (m_Flags & RELATIVEANIMPOS_MASK_CANMOVE) == RELATIVEANIMPOS_MASK_CANMOVE; }
	M_INLINE bool HasDirection() { return (m_Flags & RELATIVEANIMPOS_MASK_CANLOOK) == RELATIVEANIMPOS_MASK_CANLOOK; }
	M_INLINE bool HasQueued(int32 _CurrentTick) { return m_Flags != 0 && _CurrentTick < (m_MoveTokenStart + 2); }

	void CopyFrom(const CRelativeAnimPos& _From);
	void Pack(uint8 *&_pD, CMapData* _pMapData) const;
	void Unpack(const uint8 *&_pD, CMapData* _pMapData);

	//static bool GetCounterMove(int32 _Weapon, int32 _OpponentMove, int32& _Move, int32& _Response, fp32& _MaxCounterTime, fp32 _RelativeHealth);
	/*static void GetActionString(int32 _Move, CStr& _Str);
	static void GetActionStringLedge(int32 _Move, CStr& _Str);
	static void GetActionStringLadder(int32 _Move, CStr& _Str);*/
	static void GetLadderImpulse(int32 _Move, CXRAG2_Impulse& _Impulse);

	// Do a "get current pos function" (find animlayer at given time and extract position)
	void GetPosition(const CWAG2I_Context* _pContext, const CWAG2I* _pAGI, CMTime _Offset, CVec3Dfp32& _Pos, CQuatfp32& _Rot) const;
	void GetVelocity(const CWAG2I_Context* _pContext, const CWAG2I* _pAGI, CVec3Dfp32& _Velocity, CQuatfp32& _RotVel);
	void GetVelocityLedge(const CWAG2I_Context* _pContext, const CWAG2I* _pAGI, const CVec3Dfp32& _RightPoint, const CVec3Dfp32& _Normal, CVec3Dfp32& _Velocity, CQuatfp32& _RotVel);
	void GetVelocityLadder(const CWAG2I_Context* _pContext, const CWAG2I* _pAG2I, CVec3Dfp32& _Velocity, CQuatfp32& _RotVel);

	MACRO_INLINEACCESS_R(MoveType,uint8)
	MACRO_INLINEACCESS_REXT(Other,m_iOther,uint16)
	MACRO_INLINEACCESS_R(ControlLookX,fp32)
};

// Play moves on some other char...
#define CWObject_AnimSyncerParent CWObject
class CWObject_AnimSyncer : public CWObject_AnimSyncerParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	CStr m_Primary;
	CStr m_Secondary;
	int32 m_PrimaryMove;
	int32 m_SecondaryMove;
public:
	
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	//virtual aint OnMessage(const CWObject_Message& _Msg);

	void InitiateMove();
	int32 GetMoveFromString(const CStr& _Move);
	int32 GetCounterMove(int32 _Move);
};
#endif
