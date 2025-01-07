//The auxiliary behaviour classes

#ifndef _INC_AI_BEHAVIOUR
#define _INC_AI_BEHAVIOUR

#include "AI_Def.h"

class CAI_AgentInfo;

class CAI_Behaviour;
typedef TPtr<CAI_Behaviour> spCAI_Behaviour;

//Base class for AI behaviour
class CAI_Behaviour : public CReferenceCount
{
protected:
	//The AI that is using this behaviour
	CAI_Core * m_pAI; 

public:
	//Current sub-behaviour
	spCAI_Behaviour m_spSubBehaviour; 

public:
	//Behaviour messages
	enum {
		OBJMSG_SPECIAL_IMPULSE = OBJMSGBASE_BEHAVIOUR,
		OBJMSG_ARE_YOU_VALID,
	};
	
	//Behaviour types
	enum {
		SURVIVE = ACTION_OVERRIDE_ENUM_BASE,
		PATROL,
		BEHAVIOUR,
		FOLLOW,
		EXPLORE,
		HOLD,
		ESCAPE,
		AMBUSH,
		ENGAGE,
		AVOID,
		LEAD,
		SEARCHNDESTROY,
		//Custom AI behaviours
		TURRET,
	};
	int m_Type;

	//Constructors
	CAI_Behaviour();
	CAI_Behaviour(CAI_Core * _pAI);

	//Decides what actions to take
	virtual void OnTakeAction() = 0;

	//Resets the behaviour. This will always terminate any sub-behaviours.
	virtual void Reset();

	//Decides if the behaviour is valid.
	virtual bool IsValid();

	//Notifies behaviour that special, out-of-sequence moves have been made by the
	//super behaviour if true is given, or that these movements have stopped if false.
	virtual void OnSuperSpecialMovement(bool _bOngoing);

	//Re-initiaizes the behaviour
	virtual void ReInit(int iParam1 = 0, int iParam2 = 0, const CVec3Dfp32& VecParam1 = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& VecParam2 = CVec3Dfp32(_FP32_MAX));
	
	//Change AI user
	virtual void ReInit(CAI_Core * _pAI);

	//Message handler for behaviour specific messages
	virtual bool OnMessage(const CWObject_Message& _Msg);

	//Get current behaviour (i.e. sub-most behaviour)
	CAI_Behaviour * GetSubBehaviour();

	//Get path prio for this behaviour (0..255), given the "closeness" value to human player cameras (0..1)
	virtual int GetPathPrio(fp32 _CameraScore);
};


//The survive behaviour
class CAI_Behaviour_Survive : public CAI_Behaviour
{
protected:
	//Checks if the given server index points to a valid leader.
	//Returns pointer to leaders character object if so else NULL
	CWObject * IsValidLeader(int _iObj);

	//Tries to find a valid leader nearby. Returns pointer to leaders character object if successful, else NULL
	bool FindLeader();

	//Get new random movement destination, at or closer to given distance preferrably close to the given heading
	//If no such destination can be found, fail with CVec3Dfp32(_FP32_MAX).
	CVec3Dfp32 GetNewDestination(fp32 _Distance, fp32 _Heading, fp32 _GoToAllyChance = 0.0f);

	//Current movement destination when moving independently.
	CVec3Dfp32 m_Destination;

	//Current movement speed
	fp32 m_Speed;

public:
	//Constructor
	CAI_Behaviour_Survive(CAI_Core * _pAI);

	//Will move around slowly, stopping at times, scanning a lot, until enemy spotted; 
	//will then deal with enemy according to personality. 
	virtual void OnTakeAction();
};


//The patrol behaviour
class CAI_Behaviour_Patrol : public CAI_Behaviour
{
protected:
	//Server index to engine path object
	int m_iPath;

	//Is only true when we have just finished a subbehaviour and is returning to patrol path
	bool m_bReturningToPath;

	//Have we started the engine path?
	bool m_bStarted;

	//The position to move to when following the patrol route and the look vector
	CVec3Dfp32 m_PathPosition;
	CVec3Dfp32 m_LookVector;

	//Are we at apath stop?
	bool m_bStop;

public:
	//Constructor
	CAI_Behaviour_Patrol(CAI_Core * _pAI, int _iPath = 0);

	//Will follow path until enemy spotted; will then deal with enemy according to personality
	//If path is unavaliable, bot will revert to hold behaviour on current location.
	virtual void OnTakeAction();

	//Stops path and sets up behaviour as when initialized
	virtual void Reset();

	//Change path
	virtual void ReInit(int iParam1 = 0, int iParam2 = 0, const CVec3Dfp32& VecParam1 = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& VecParam2 = CVec3Dfp32(_FP32_MAX));
};



//The lead behaviour
class CAI_Behaviour_Lead : public CAI_Behaviour_Patrol
{
protected:
	//The one we're leading
	int m_iFollower;

public:
	//Constructor
	CAI_Behaviour_Lead(CAI_Core * _pAI, int _iPath = 0, int _iFollower = 0);

	//Will follow path when follower is nearby, or follow follower if he moves away from path. 
	//When enemy spotted, will then deal with enemy according to personality
	//If path is unavaliable, bot will revert to follow behaviour with the follower as leader.
	virtual void OnTakeAction();

	//Stops path and sets up behaviour as when initialized
	virtual void Reset();

	//Change path and/or follower
	virtual void ReInit(int iParam1 = 0, int iParam2 = 0, const CVec3Dfp32& VecParam1 = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& VecParam2 = CVec3Dfp32(_FP32_MAX));

	//Behaviour is valid when there's a valid follower around
	virtual bool IsValid();

	//Path prio is greater when leading a human player
	virtual int GetPathPrio(fp32 _CameraScore);
};


//The follow behaviour
class CAI_Behaviour_Follow : public CAI_Behaviour
{
protected:
	//Server index to the character to follow
	int m_iLeader;

	//Is true only if the leader was specified as a key or through an order. When following a true
	//leader the rank of the leader is of no consequent.
	bool m_bTrueLeader;

	//Checks if the given server index points to a valid leader.
	//Returns pointer to leaders character object if so else NULL
	CWObject * IsValidLeader(int _iObj);

	//Tries to set a leader. Returns pointer to leaders character object if successful, else NULL
	CWObject * FindLeader();

public:
	//Constructor
	CAI_Behaviour_Follow(CAI_Core * _pAI, int _iLeader = 0, bool _bTrueLeader = false);

	//Will follow leader until enemy spotted; will then deal with enemy according to personality.
	//If not given a leader when initializing, bot will follow first friend to come close.
	virtual void OnTakeAction();

	//Behaviour is valid when there's a valid leader around
	virtual bool IsValid();

	//Re-init leader
	virtual void ReInit(int iParam1 = 0, int iParam2 = 0, const CVec3Dfp32& VecParam1 = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& VecParam2 = CVec3Dfp32(_FP32_MAX));

	//Path prio is greater when following a human player
	virtual int GetPathPrio(fp32 _CameraScore);
};


//The guard behaviour
class CAI_Behaviour_Guard : public CAI_Behaviour_Follow
{
public:
	//Constructor
	CAI_Behaviour_Guard(CAI_Core * _pAI, int _iVIP = 0);

	//Will follow VIP and attack any enemy getting close to or attacking him or self. If not 
	//given a VIP when initializing, bot will revert to follow behaviour.
	virtual void OnTakeAction();
};


//The explore behaviour
class CAI_Behaviour_Explore : public CAI_Behaviour_Survive
{
public:
	//If this value isn't CVec3Dfp32(_FP32_MAX), then we should try to get to 
	//this position, before starting explore
	CVec3Dfp32 m_PathPos;

	//Timer which keeps track of when behaviour is invalidated
	int m_Timer;

	//Chance (0..1) every 5'th second that we'll decide to follow a valid leader
	fp32 m_Loyalty;

public:
	//Constructors
	CAI_Behaviour_Explore(CAI_Core * _pAI, fp32 _Loyalty = 0.2f, const CVec3Dfp32& _Pos = CVec3Dfp32(_FP32_MAX), int _iValidTime = -1);

	//Will move around a lot and scan a lot until enemy spotted; will then deal with enemy 
	//accordning to personality
	virtual void OnTakeAction();

	//Re-init position
	virtual void ReInit(int iParam1 = 0, int iParam2 = 0, const CVec3Dfp32& VecParam1 = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& VecParam2 = CVec3Dfp32(_FP32_MAX));

	//Valid as long as timer is non-zero
	virtual bool IsValid();
};


//The hold behaviour
class CAI_Behaviour_Hold : public CAI_Behaviour
{
protected:
	//The path position to hold
	CVec3Dfp32 m_PathPos;

	//The given hold-position
	CVec3Dfp32 m_Pos;

	//The given look direction
	CVec3Dfp32 m_Look;

	//The square of the maximum distance the bot can willingly move from the position. If -1, bot can move anywhere.
	fp32 m_MaxDistSqr;

	//The square of the distance above which bot would prefer to go back to position. If -1, bot have no compunctions about going anywhere.
	fp32 m_HesitateDistSqr;

	//Were we making a move to maintain distance, and still using a subbehaviour last frame?
	bool m_bMadeSpecialMove;

	//Always pause for a while when starting to make special movement. This timer is that while.
	int m_SpecialMoveTimer;

public:
	//Constructor
	CAI_Behaviour_Hold(CAI_Core * _pAI, const CVec3Dfp32& _Pos = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& _Look = CVec3Dfp32(_FP32_MAX), fp32 _HesitateDistance = -1.0f, fp32 _MaxDistance = -1.0f);

	//Will go to position and stand fast, scanning around, until enemy spotted. will then deal 
	//with enemy accordning to personality. If no position is given, bot will always hold at 
	//current position.
	virtual void OnTakeAction();

	//Re-init position
	virtual void ReInit(int iParam1 = 0, int iParam2 = 0, const CVec3Dfp32& VecParam1 = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& VecParam2 = CVec3Dfp32(_FP32_MAX));
};


//The escape behaviour
class CAI_Behaviour_Escape : public CAI_Behaviour
{
protected:
	//The escape position we're currently trying to reach
	int iCurrent;

	//The possible esacpe positions
	TArray<CVec3Dfp32> m_lPos;

	//Have we reached an escape point?
	bool HasEscaped();

public:
	//Constructor
	CAI_Behaviour_Escape(CAI_Core * _pAI, const TArray<CVec3Dfp32>& _lPos = TArray<CVec3Dfp32>());

	//Will attempt to reach randomly selected escape point. If encountering enemy, bot will 
	//either try to resch another escape point or deal with enemy, according to personality.
	//If no escape points are given, bot will revert to survive behaviour.
	virtual void OnTakeAction();
	
	//Behaviour is valid if not at an escape position
	virtual bool IsValid();

	//Re-init with max two positions
	virtual void ReInit(int iParam1 = 0, int iParam2 = 0, const CVec3Dfp32& VecParam1 = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& VecParam2 = CVec3Dfp32(_FP32_MAX));
};


//The engage behaviour. As in attacking, not a prelude to marriage :)
class CAI_Behaviour_Engage : public CAI_Behaviour
{
protected:
	//Max allowed position uncertainty until we switch to search and destroy
	static const fp32 MAX_UNCERTAINTY;

	//Server index to the character to attack
	int m_iTarget;
	CVec3Dfp32	m_EvadePos;

	//Keeps track of the number of frames since we last found a target
	int m_TargetFindingTimer;

	//Is the current target a true target
	bool m_bTrueTarget;

	//Are we attacking?
	bool m_bAttacking;

	//Opponent info (what we think he's doing etc)
	CAI_OpponentInfo m_OpponentInfo;

	//Server index to our true target, or 0 if the original target wasn't true. This is used
	//to make sure we always revert to attacking this target if we lose him and find him again.
	int m_iTrueTarget;

	//Frames left before we can switch weapon again
	int m_SwitchWeaponTimer;

	//Current melee prio
	int m_MeleePrio;

	//We won't attack with ranged weapon until this tick
	int m_HoldFireTick;

	//As long as a player hasn't spotted us we're considered a sniper. This will cause a sniper 
	//penalty when we try to attack player so that we're less likely to kill him without him knowing of us
	bool m_bSniper;

	//Tries to find a good target.
	//Returns pointer to targets character object if successful, else NULL
	CAI_AgentInfo * FindTarget();

	//Checks if the given server index points to a valid target.
	//Returns pointer to targets agent info if so else NULL
	CAI_AgentInfo * IsValidTarget(int _iObj);

	//Check current target and acquire a new one if this is bad. Return pointer to target object or NULL
	CAI_AgentInfo * AcquireTarget();	

	//Returns the current melee action the given agent is performing
	int GetAction(int _iObj);

	//Resets melee-related values
	void ResetMeleeState();

	//Melee manoeuvering priority
	int GetMeleePrio();

	//Will we engage the given opponent if we try to initiate melee now?
	bool CanEngage(int _iObj);

public:

	//Constructor
	CAI_Behaviour_Engage(CAI_Core * _pAI, int _iTarget = 0, bool _bTrueTarget = false);

	//Will move to position where the target can be attacked, and attack target. Evasive manoeuvers
	//will be used according to personality. If the target is a true target then this target will be 
	//attacked exclusively until it's dead.
	//If no target is specified, the bot will engage any enemy spotted. 
	virtual void OnTakeAction();

	//Behaviour is valid when there is a potential target, unless the target is a true one, in which 
	//case the behaviour is only valid when that specific target is valid.
	virtual bool IsValid();

	//Resets the current target to the true target if there is one, and terminates any sub-behaviours.
	void Reset();

	//Makes behaviour defensive when ongoing, and resets it when not.
	virtual void OnSuperSpecialMovement(bool _bOngoing);

	//Re-init target and truetarget flag (cast iParam2 to bool)
	virtual void ReInit(int iParam1 = 0, int iParam2 = 0, const CVec3Dfp32& VecParam1 = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& VecParam2 = CVec3Dfp32(_FP32_MAX));

	//Always get higher path prio than normal, but especially if wielding melee weapon
	virtual int GetPathPrio(fp32 _CameraScore);
};


//The ambush behaviour; this'll be quite stupid until later I'm afraid.
class CAI_Behaviour_Ambush : public CAI_Behaviour_Engage
{
public:
	//Constructor
	CAI_Behaviour_Ambush(CAI_Core * _pAI, int _iTarget = 0, bool _bTrueTarget = false);

	//Will wait for target to come within LOS before attacking, or try to get into a position to 
	//attack if targets back is turned. If target is true target, then this is the only target that
	//bot will attack. After attacking, or when being attacked the bot will consider itself discovered 
	//and deal with enemy according to personality. 
	//If no target is specified, the bot will ambush any enemy getting close. 
	virtual void OnTakeAction();

	//Behaviour is valid when the bot hasn't been discovered (i.e. until it attacks or comes under 
	//attack) and there is a potential target. If the target is a true target the bot will only attack that 
	//target until the bot is discovered
	virtual bool IsValid();
};


//The avoid behaviour.
class CAI_Behaviour_Avoid : public CAI_Behaviour
{
protected:
	//Constructor
	int m_iScaryPerson;

	//Current path position to run to
	CVec3Dfp32 m_EscapePos;

	//Current relative escape position to try when previous escape positions were no good
	int m_iCurrentEscapeIndex;

	//Behaviour is always valid until this timer has counted up to the invalidation delay
	int m_InvalidationTimer;
	int m_InvalidationDelay;

	//Checks if the given server index points to a valid scary person
	CWObject * IsValidScaryPerson(int _iObj);

	//Return position towards closest friend or ally who does not have any 
	//enemy nearby and who can be reached without running past enemies.
	//If a scary person is specified, then he is the only therat being considered.
	CVec3Dfp32 GetEscapePosition(CWObject * _pScaryPerson = NULL);

public:
	//Initializes behaviour
	CAI_Behaviour_Avoid(CAI_Core * _pAI, int _iScaryPerson = 0, int _InvalidationDelay = 100);

	//Will try to get out of scary persons LOS and get far away from him, preferrably towards friends, 
	//taking evasive manouevers when in LOS. Bot will only fight back if cornered.
	//If no scary person is specified, the bot will try to avoid any enemy.
	virtual void OnTakeAction();

	//Resets escape position stuff and terminates subbehaviour.
	virtual void Reset();

	//Behaviour is valid when threatened by scary person.
	virtual bool IsValid();

	//Re-init scary person
	virtual void ReInit(int iParam1 = 0, int iParam2 = 0, const CVec3Dfp32& VecParam1 = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& VecParam2 = CVec3Dfp32(_FP32_MAX));

	//Always extra high path prio
	virtual int GetPathPrio(fp32 _CameraScore);
};



//The search and destroy behaviour.
class CAI_Behaviour_SearchNDestroy : public CAI_Behaviour_Engage
{
protected:
	//Current movement destination when moving independently.
	CVec3Dfp32 m_Destination;

	//Previous position uncertainty 
	fp32 m_PrevUncertainty;

	//We won't change destination until we're there, or this tick
	int m_ChangeTick;
	int m_StopTrackTick;

	//Get new random movement destination, at or closer to given distance preferrably close to the given heading
	//If no such destination can be found, fail with CVec3Dfp32(_FP32_MAX).
	CVec3Dfp32 GetNewDestination(CAI_AgentInfo * _pTarget, fp32 _Distance, fp32 _Heading, fp32 _GoToAllyChance = 0.0f);

	//Behaviour modes
	enum {
		SEARCH = 0,
		DESTROY,
	};
	int m_Mode;

	//Move to believed position if possible, otherwise move to friends or roam randomly.
	void OnSearch(CAI_AgentInfo	* _pTarget);

	//We think we've got enemy! Fire repeatedly, then go investigate.
	void OnDestroy(CAI_AgentInfo * _pTarget);

public:
	//Constructor
	CAI_Behaviour_SearchNDestroy(CAI_Core * _pAI);

	//Will mostly explore, searching for enemy. If jumpy, will fire erraticaly when we think enemy has been found 
	virtual void OnTakeAction();

	//Valid as long as there's no accurately known target
	virtual bool IsValid();
};






#endif