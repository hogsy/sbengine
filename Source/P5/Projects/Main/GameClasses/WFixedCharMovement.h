#ifndef __FIXEDCHARMOVEMENT_H
#define __FIXEDCHARMOVEMENT_H

// TEMPORARY PLACEMENT OF MOVEMENT MOD
//#define PHOBOSDEBUG

#ifndef __MACROINLINEACCESS
#define __MACROINLINEACCESS
#define MACRO_INLINEACCESS_RW(name, type) \
	M_INLINE const type& Get##name() const { return m_##name; } \
	M_INLINE type& Get##name() { return m_##name; }

#define MACRO_INLINEACCESS_R(name, type) \
	M_INLINE const type& Get##name() const { return m_##name; }

#define MACRO_INLINEACCESS_RWEXT(name, variable, type) \
	M_INLINE const type& Get##name() const { return variable; } \
	M_INLINE type& Get##name() { return variable; }

#define MACRO_INLINEACCESS_REXT(name, variable, type) \
	M_INLINE const type& Get##name() const { return variable; }
#endif

#ifdef PHOBOSDEBUG
#define MACRO_PHOBOSDEBUG(stuff) stuff
#else
#define MACRO_PHOBOSDEBUG(stuff)
#endif

enum
{
	MOVETARGET_MODE_INVALID		= 0,
	MOVETARGET_MODE_MOVING		= 1,
	MOVETARGET_MODE_MOVEENDED	= 2,
	MOVETARGET_MODE_SETCROUCH	= 4,
	MOVETARGET_MODE_NOOTHERADJUST = 8,
	MOVETARGET_MODE_MOVERELATIVE = 16,
	MOVETARGET_MODE_LEDGEPOS = 32,


	MOVETARGET_INVALIDATE			= 0,
	
	FIGHT_MOVE_NEWTEST,
	FIGHT_MOVE_NEWTEST_NOOPP,

	FIGHT_MOVE_NONE = 0,

	
	FIGHT_MOVE_NUMBEROFMOVES				=  2,
	
	// Ledge moves
	/*LEDGE_MOVE_LEFT,
	LEDGE_MOVE_RIGHT,
	LEDGE_MOVE_UP,
	LEDGE_MOVE_DOWN,
	LEDGE_MOVE_CORNERINLEFT,
	LEDGE_MOVE_CORNERINRIGHT,
	LEDGE_MOVE_CORNEROUTLEFT,
	LEDGE_MOVE_CORNEROUTRIGHT,
	LEDGE_MOVE_NUMBEROFMOVES = 8,*/


	FIGHT_STANCE_LEFTSAFE					= 1 << 0,
	FIGHT_STANCE_RIGHTSAFE					= 1 << 1,
	FIGHT_STANCE_LEFTNEUTRAL				= 1 << 2,
	FIGHT_STANCE_RIGHTNEUTRAL				= 1 << 3,
	FIGHT_STANCE_LEFTCLOSE					= 1 << 4,
	FIGHT_STANCE_RIGHTCLOSE					= 1 << 5,
	FIGHT_STANCE_NUMBEROFSTANCES	= 6,
	FIGHT_STANCE_NONE				= 0,

	FIGHT_STANCE_LEFTMASK					= (FIGHT_STANCE_LEFTSAFE | FIGHT_STANCE_LEFTNEUTRAL |
												FIGHT_STANCE_LEFTCLOSE),
	FIGHT_STANCE_RIGHTMASK					= (FIGHT_STANCE_RIGHTSAFE | FIGHT_STANCE_RIGHTNEUTRAL |
												FIGHT_STANCE_RIGHTCLOSE),
	FIGHT_STANCE_SAFEMASK					= (FIGHT_STANCE_LEFTSAFE | FIGHT_STANCE_RIGHTSAFE),
	FIGHT_STANCE_NEUTRALMASK				= (FIGHT_STANCE_LEFTNEUTRAL | 
												FIGHT_STANCE_RIGHTNEUTRAL),
	FIGHT_STANCE_CLOSEMASK					= (FIGHT_STANCE_LEFTCLOSE | FIGHT_STANCE_RIGHTCLOSE),
};
class CWorld_PhysState;
class CWO_Character_ClientData;

/*class CMoveTarget
{
protected:
	// Start/targettime in gametime
	//fp32			m_StartTime;
	CVec3Dfp32	m_Target;
	CVec3Dfp32	m_Velocity;
	CMTime		m_TargetTime;

	uint8		m_MoveMode;

	bool CanMove(CVec3Dfp32 _CharPos, CVec3Dfp32 _EnemyPos, CVec3Dfp32 _NewPosition);
	bool GetNewFightPosition(CWObject_CoreData* _pObj, int _Move, int _Stance, 
		const CVec3Dfp32& _CharPos, const CVec3Dfp32& _OpponentPos, CVec3Dfp32& _ReturnPos, 
		int& _MoveType);
	bool GetNewLedgePosition(int _Move, const CVec3Dfp32& _CharPos, const CVec3Dfp32& _Point1, 
		const CVec3Dfp32& _Point2, const CVec3Dfp32& _Normal, CVec3Dfp32& _ReturnPos, 
		CWorld_PhysState* _pWPhys, CWO_Character_ClientData* _pCD);
	bool GetNewLedgePosition2(int _Move, const CVec3Dfp32& _CharPos, const fp32& _CurrentPos,
		const CVec3Dfp32& _Point1, const CVec3Dfp32& _Point2, const CVec3Dfp32& _Normal, 
		CVec3Dfp32& _ReturnPos, CWorld_PhysState* _pWPhys, CWO_Character_ClientData* _pCD);
public:
	CMoveTarget();
	void Copy(const CMoveTarget& _MoveTarget);
	CMoveTarget(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CVec3Dfp32 _Target, 
		fp32 _Duration, CMTime _StartTime = CMTime::CreateInvalid());
	void SetTarget(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CVec3Dfp32 _Target, 
		fp32 _Duration, CMTime _StartTime = CMTime::CreateInvalid());

	bool DoFightMove(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int _Move, int _Stance,
		fp32 _Duration, CMTime _StartTime = CMTime::CreateInvalid());

	// Get animation velocity
	void GetUserAccelleration(const CSelection& _Selection, CWObject_CoreData* _pObj, 
		CWorld_PhysState* _pPhysState, CVec3Dfp32& _VRet, CWO_Character_ClientData* _pCD, 
		int16& _Flags, uint8 _MoveFlags);
	void MakeInvalid();

//	MACRO_INLINEACCESS_R(Start, CVec3Dfp32)
	MACRO_INLINEACCESS_R(Target, CVec3Dfp32)
//	MACRO_INLINEACCESS_R(StartTime, fp32)
	MACRO_INLINEACCESS_R(Velocity, CVec3Dfp32)
	MACRO_INLINEACCESS_R(TargetTime, CMTime)
	MACRO_INLINEACCESS_RW(MoveMode, uint8)

	// Prepared for autovar (if needed)
	void Pack(uint8 *&_pD);
	void Unpack(const uint8 *&_pD);
};

#define CFIXEDCHARMOVEMENT_MAXMOVES 1
enum
{
	FIXEDCHARMOVE_NOCOPY		= 0,
	FIXEDCHARMOVE_COPYONE		= 1 << 0,

	FIXEDCHARMOVE_FLAG_APPLYVELOCITY = 1 << 0,
};

class CFixedCharMovement// : public CReferenceCount
{
protected:
	// Array of moves, only one at the moment though... (mostly for autovar)
	uint8		m_MoveMode;
	CMoveTarget	m_MoveTargets[CFIXEDCHARMOVEMENT_MAXMOVES];
public:
	CFixedCharMovement();
	void Invalidate();
	void Copy(const CFixedCharMovement& _CharMovement);
	bool AddMoveTarget(const CMoveTarget& _MoveTarget);
	// SHOULD ADJUST TO LATEST POSTION IF NO ITEMS IN QUEUE
	uint8 GetUserAccelleration(const CSelection& _Selection, CWObject_CoreData* _pObj, 
		CWorld_PhysState* _pPhysState, CVec3Dfp32& _VRet, CWO_Character_ClientData* _pCD, 
		int16& _Flags, uint8 _MoveFlags, CWObject_CoreData* _pRelObj = NULL);

	CVec3Dfp32 GetNextTarget(CWObject_CoreData* _pObj);
	
	// Prepared for autovar (if needed)
	//void Pack(uint8 *&_pD, CMapData* _pMapData);
	//void Unpack(const uint8 *&_pD, CMapData* _pMapData);
	static void SCopyFrom(void* _pThis, const void* _pFrom);
	static void SPack(const void* _pThis, uint8 *&_pD, CMapData* _pMapData);
	static void SUnpack(void* _pThis, const uint8 *&_pD, CMapData* _pMapData);
};*/

enum
{
	AG_FIGHTMODE_LTRIGGER		= 1 << 8,
	AG_FIGHTMODE_RTRIGGER		= 1 << 9,
	AG_FIGHTMODE_MOVEFWD		= 1 << 10,
	AG_FIGHTMODE_MOVEBWD		= 1 << 11,
	AG_FIGHTMODE_MOVELEFT90		= 1 << 12,
	AG_FIGHTMODE_MOVERIGHT90	= 1 << 13,
	AG_FIGHTMODE_MOVELEFT45		= 1 << 14,
	AG_FIGHTMODE_MOVERIGHT45	= 1 << 15,


	AG_FIGHTMODE_NUMINPUTTYPES  = 16,
};

#ifdef PHOBOSDEBUG
class CAGFightEntryManager;
// Ok, helper class to aid me in scripting, should not be part of engine so I'll make it as big
// and ugly as I want, OK
class CAGFightEntry
{
	friend class CAGFightEntryManager;
protected:
	// Name, ie LongPunch/punch/moveleft/whatever
	CStr	m_ActionName;
	// Which move
	int		m_Move;
	// Which stance
	int		m_Stance;
	int		m_TargetStance;
	// Options (what blocks/interrupts/and so on is active
	int		m_FightStatus;

	int		m_Damage;
	int		m_Stamina;
	int		m_OpponentStamina;

	int		m_Index;

	fp32		m_PreAttackTime;
	fp32		m_AttackTime;
	fp32		m_PostAttackTime;
	fp32		m_AttackOffsetTime;

	int		m_AttackType;

	fp32		m_TimeScale;

	// Which inputs must be active to use it
	int		m_Input;
	bool	m_bRegisterInput;

	//static int ResolveMoveFromString(CStr _Move);
	static int ResolveStanceFromString(CStr _Stance);
	static CStr StringFromMove(int _Move);
	static CStr StringFromStance(int _Stance);
	static CStr StringFromFightStatus(int _FightStatus);
	static int ResolveAttackType(CStr _AttackType);

	CStr GeneratePrePhase(CStr _TabStr);
	CStr GenerateAttackPhase(CStr _TabStr);
	CStr GeneratePostPhase(CStr _TabStr);
	

	CStr GenerateFightStatus(int _FightStatus);
	CStr GenerateFightDamage(int _Damage);
	CStr GenerateStamina(int _Stamina, int _Target);
	CStr GenerateFightMove(int _Move);
	CStr GenerateAttackType(int _AttackType);
	CStr GenerateInput(int _Input, CStr _TabStr);
	CStr GenerateRegisterInput(int _Input, CStr _TabStr);
	CStr GenerateRegisterInputString(int _Input, bool bNotUsed);
	int InputFromString(CStr _Str);
	CStr GenerateInputTestString(int _Input, bool bNotUsed);

	int ResolveMove(CStr _Str);
	int ResolveFightStatus(CStr _Str);
	int ResolveDamage(CStr _Str);
	int ResolveStamina(CStr _Str);

	CStr GetMoveStanceStr(int _Stance);
public:
	CAGFightEntry();
	CAGFightEntry(CStr _Name, int _TargetStance, int _Move, int _Stance, int _FightStatus, 
		int _Damage, int _Stamina, int _OpponentStamina, fp32 _Pre, fp32 _Attack, fp32 _Post);

	void Clear();

	bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	// Generates a state from this entry
	CStr GenerateState(CStr _TabStr, CAGFightEntryManager* _pManager);

	// To get to this action this is what you need
	CStr GenerateActionString(CStr _TabStr, CStr _PostPhaseTime);
	CStr GetConstants(CStr _TabStr);

	MACRO_INLINEACCESS_R(Stance, int);
};

class CAGFightEntryManager
{
protected:
	TArray<CAGFightEntry>		m_FightEntries;
	
	CStr GenerateConstantList(CStr _TabStr);
public:
	//CAGFightEntryManager();
	bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	CStr GenerateActionList(int _Stance, CStr _TabStr, CStr _PostPhaseTime);

	void WriteStatesToFile(CStr _FileName);
};
#endif
#endif
