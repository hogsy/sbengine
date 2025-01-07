#ifndef _INC_AI_BEHAVIOUR_CUSTOM
#define _INC_AI_BEHAVIOUR_CUSTOM

#include "../../WObj_NavNode/WObj_NavNode.h"


//Custom behaviours


//Custom behaviours for wraiths
class CAI_Behaviour_Wraith : public CAI_Behaviour
{
protected:
	//The area to haunt
	int m_iArea;

	//The attack distance
	fp32 m_AttackDistance;

	//Action state
	enum {
		HAUNTING,	//Zooming about
		ENGAGING,	//Getting into position to attack
		ATTACKING,	//Making attack run
		RECOVERING, //Recovering after performed attack
	};
	int m_iState;

	//Current target, if any.
	int16 m_iTarget;

	//Are we offlimits and returning to area?
	uint8 m_bReturningToArea:1;


	//Counts the number of frames remaining during a post-attack run
	int	m_iAttackTimer;

	//Check if give target is valid
	bool IsValidTarget(CWObject_Character * _pTarget);

	//Return object index of best target, or 0 if there is no valid target
	int FindTarget();

	//Get new position to haunt. The argument is true if we're supposed to get back into our area
	CVec3Dfp32 GetHauntPosition(bool _bOfflimits = false);

	//Roam around
	void OnHaunt();

	//Get new position to attack from. 
	CVec3Dfp32 GetAttackPosition(CWObject * _pTarget);

	//Get into position to attack target
	void OnEngage();

	//Perform attack run
	void OnAttack();

	//Recover after attack
	void OnRecover();

	//Are we in position to attack target?
	bool InAttackPosition(int _iTarget);

public:
	//Initializes behaviour
	CAI_Behaviour_Wraith(CAI_Core * _pAI, int _iArea = 0, fp32 _AttackDistance = -1.0f);

	//Will zoom about in a random fashion, attacking any valid targets that are spotted.
	virtual void OnTakeAction();

	//Resets stuff.
	virtual void Reset();

	//Re-init area and attack distance
	virtual void ReInit(int iParam1 = 0, int iParam2 = 0, CVec3Dfp32 VecParam1 = CVec3Dfp32(_FP32_MAX), CVec3Dfp32 VecParam2 = CVec3Dfp32(_FP32_MAX));

#ifndef M_RTM
	//Get some debug info about the bot
	virtual CStr GetDebugString();
#endif
};


//Custom behaviours for Zurana
class CAI_Behaviour_Zurana : public CAI_Behaviour_Engage
{
protected:
	//Special animations
	enum{
		ANIM_GRAB1 = 35,
		ANIM_GRAB1_MISS = 15,
		ANIM_GRAB1_HIT = 16,
		ANIM_GRAB2 = 36,
		ANIM_GRAB2_MISS = 18,
		ANIM_GRAB2_HIT = 19,
		ANIM_EAT = 20,
		ANIM_TAUNT_GRAB1 = 21,
		ANIM_TAUNT_GRAB2 = 22,
		ANIM_START_STUMBLING = 23,
		ANIM_MH_STUMBLE = 24,
		ANIM_STUMBLE_BWD = 25,
		ANIM_LASHOUT = 26,
		ANIM_RISE_UP = 27,
		ANIM_FRUSTRATED1 = 28,
		ANIM_FRUSTRATED2 = 29,
		ANIM_GIVE_BIRTH = 30,
		ANIM_CLONE_APPEAR = 31,
		ANIM_CLONE_DISAPPEAR = 32,
		ANIM_ROAR = 33,
	};

	//Behaviour modes
	enum {
		NONE = 0,
		DUCK_N_GRAB,
		PLAYER_SAFE,
		FIGHTCOPY,
		LAVAESCAPE,
		STUNNED,
		GRABBED,
	};

	enum {
		GRAB = 1,
		EATING,
		TAUNTING,
	};
	int m_iMode;
	int m_GrabTimer;
	int m_iLastMode;
	int m_EatTimer;
	int m_DamageTimer;

	//Behaviour mode parameters
	int m_iObj1;
	int m_iObj2;
	int m_iObj3;
	
	//Current path position in some situations
	CVec3Dfp32 m_PathPos;	

	//Current head and hand positions 
	CVec3Dfp32 m_HeadPos;	
	CVec3Dfp32 m_HandPos;
	
	//Position and direction of steam blast
	CVec3Dfp32 m_SteamPos;
	CVec3Dfp32 m_SteamDir;

	//Frames since steam blast
	int m_iSteamTimer;

	//Time left to remain stunned
	int m_iStunTimer;

	//Starting position
	CVec3Dfp32 m_StartPos;

	//Grabbed victim
	int16 m_iGrabee;

	uint8 m_iGrabMode;

	//Crouch at first object position and try to reach player. If successful start second (grab) 
	//engine path. If hit by steam start third (recoil) engine path.
	void OnDuckNGrab();

	//Player has reached safety. Rail a bit and destroy stuff.
	void OnPlayerSafe();

	//Combat copy
	void OnFightCopy();

	//Hack! Escape when fallen into lava and then revert to last behaviour mode
	void OnEscapeLava();

	//Stumble around backwards until stun timer is up, when we revert to last behaviour mode
	void OnStunned();

	//Victim has been grabbed. Kill him in some gruesome fashion.
	void OnGrabbed();
	
	//Set the head and right hand positions by evaluating anim
	void SetAnimPositions(CVec3Dfp32 _TempHeadOffset = 0);

	//Check if we can grab a player, given our hand position, and return the index of 
	//the grabbed victim if so
	int CanGrab(CVec3Dfp32 _HandPos);

	//Should we start a grab attack?
	bool ShouldGrab();

public:
	//Initializes behaviour
	CAI_Behaviour_Zurana(CAI_Core * _pAI);

	//Will enagage any opponent or take special action due to recieved messages
	virtual void OnTakeAction();

	//Handle special messages which influence the behaviour
	virtual bool OnMessage(const CWObject_Message& _Msg);

	//Always max prio
	virtual int GetPathPrio(fp32 _CameraScore);
};


//Custom behaviours for vatar
class CAI_Behaviour_Vatar : public CAI_Behaviour_Engage
{
protected:
	enum {
		NONE = 0,
		START_TELEPORT,
		TELEPORTING,
		TELEPORTED,
		SETTLING,
	};
	//The different telportation points.
	TArray<int> m_iTeleportationPoints;

	TArray<int> m_iPlattformPoints;

	int m_iCurPoint;

	CVec3Dfp32 m_StartPos; 
	
	int m_WaitTimer;

	int m_FramesTargetOutRange;

	int m_LookTimer;

	int m_TeleportTimer;
	
	int m_AttackTimer;

	int m_PlattformTimer;

	int m_MinTelTime;

	int m_RandTelTime;
	
	int m_SettleTimer;
	
	CWObject_Character * m_pTarget;
	
	int m_iModel;

	int m_iMode;

	CWObject *m_pTelEffStart;
	CWObject *m_pTelEffEnd;
	
	CWObject *m_pTelTrail;
	
	int m_StartTelTime;
	int m_TransitTelTime;
	int m_EndTelTime;

	uint8 m_bOnPlattform:1;
	
	uint8 m_bSwitchWeapon:1;

	uint8 m_bForceTeleport:1;
	
	uint8 m_bTargetOnPlatform:1;

	uint8 m_bCharging:1;


	void TeleportEffect(bool _On);

	bool IsPointAvailable(int _Index);

	void ResetWaitTimer();

	int ChoosePoint(CWObject * _pTarget);
	
	void OnEngage(CWObject * _pTarget);

	bool ShouldAttack(CWObject * _pTarget);

	bool ShouldTeleport(CWObject * _pTarget);

	void TeleportTo(int _iObj, int _Ticks);

	void OnGoHunting();

	void LookAround();

	void SwitchWeapon(CWObject * _pTarget);
	
	void StartTeleporting();
	void EndTeleport();
	void TeleportVatar();
	
	void Settle(int _Time);

public:
	//Initializes behaviour
	CAI_Behaviour_Vatar(CAI_Core * _pAI, int _iArea = 0);

	virtual void OnTakeAction();
	virtual bool OnMessage(const CWObject_Message& _Msg);
	//Always max prio
	virtual int GetPathPrio(fp32 _CameraScore);
};


//Custom behaviours for Durzu
class CAI_Behaviour_Durzu : public CAI_Behaviour_Engage
{
protected:
	//Special anims
	enum {
		ANIM_TURNLEFT = 237,				//*Swim_Fwd
		ANIM_TURNRIGHT = 238,				//*Swim_Bwd
		ANIM_HIGHJUMP_ANTICIPATION = 125,	//*Hammer_FwdAttack1_Combo1
		ANIM_HIGHJUMP_LIFTOFF =	126,		//*Hammer_FwdAttack1_Combo2
		ANIM_HIGHJUMP_RISE = 128,			//*Hammer_Stand_Block
		ANIM_HIGHJUMP_SUMMIT = 129,			//*Hammer_Crouch_Block
		ANIM_HIGHJUMP_FALL = 130,			//*Hammer_Fwd_Block
		ANIM_HIGHJUMP_LAND = 131,			//*Hammer_Bwd_Block
	};
		
	//Behaviour modes
	enum {
		NONE = 0,
		LAVAESCAPE,
		TURNING,
		SETTLING,
	};
	int m_iMode;

	//The area to stay in
	int m_iArea;

	//Current highjump anim sequence and timer
	int m_iHighJumpSeq;
	int m_iHighJumpTimer;

	//Starting position
	CVec3Dfp32 m_StartPos;

	//Turn speed for body and look in fractions per frame when in turning mode
	fp32 m_BodyTurnSpeed;
	fp32 m_LookTurnSpeed;

	//Turn time left
	int m_iTurnTimer;

	//Time left to settle
	int m_iSettleTimer;

	//The number of stonegnome spawns we will try to have in action
	int m_nSpawns;

	//The indices of the currently active stonegnome spawn.
	TArray<int> m_lSpawns;

	//Should we attack when done settling?
	uint8 m_bAttack:1;

	//Hack! Escape when fallen into lava
	void OnEscapeLava();

	//Go to position from which we can attack target
	void OnFollowTarget(CWObject * _pTarget);

	fp32 GetTimeScale();
	

public:
	//Initializes behaviour
	CAI_Behaviour_Durzu(CAI_Core * _pAI, int _iArea = 0, int _Spawn = -1);

	//Will move towards opponents, unleashing lava balls at a distance and using 
	//the quake fist at close quarters
	virtual void OnTakeAction();

	//Handles incoming messages
	virtual bool OnMessage(const CWObject_Message& _Msg);

	//Always max prio
	virtual int GetPathPrio(fp32 _CameraScore);
};



//Custom behaviours for dragon
class CAI_Behaviour_Dragon : public CAI_Behaviour_Engage
{
protected:
	//Behaviour modes
	enum {
		NONE = 0,
		DYING,	
	};
	enum {
		ANIM_HOOVER = 2,	
		ANIM_FLY = 0,	
	};

	int m_iMode;
	int m_iPath;
	int m_iDiePath;
	int m_ResetLook;
	int m_SwitchCounter;
	int m_TrueFollowTime;
	int m_Wait;
	int m_DieTimer;
	int m_RemMinHealth;

	int m_DamageTimer;

	//The different attack runs
	TArray<int> m_iAttackRuns;
	
	// Is the dragon currently on a run?
	uint8 m_bOnRun:1;
	uint8 m_bOnDieRun:1;
	uint8 m_bHoldFire:1;
	uint8 m_bTargetLockOn:1; 
	uint8 m_bHooveringSet:1;
	uint8 m_bAtDiePos:1;
	uint8 m_bFallenBehind:1;


	
	//Check if is in to position to attack the target.
	bool InAttackPosition(int _iTarget);

	void OnFollowPath();

	//Move into attack position against target and attack
	void OnEngage(CWObject * _pTarget);

	//"Limp" (flap) away to fall out of sight
	void OnDie();

public:
	//Initializes behaviour
	CAI_Behaviour_Dragon(CAI_Core * _pAI, int _iStartNavNode = 0);

	//Will move around using pathfinder and breathe fire on any unfortunate enemies found
	virtual void OnTakeAction();

	//Handle special messages which influence the behaviour
	virtual bool OnMessage(const CWObject_Message& _Msg);

	//Always max prio
	virtual int GetPathPrio(fp32 _CameraScore);
};

#endif