#ifndef _INC_AI_ACTION
#define _INC_AI_ACTION

struct SDead;
class CAI_Core;
class CAI_Action;
class CAI_Action_Restrict;
class CAI_ActionDefinition;
class CAI_LockedActionList;
typedef TPtr<CAI_Action> spCAI_Action;
typedef TPtr<CAI_Action_Restrict> spCAI_Action_Restrict;
class CWO_ScenePoint;
class CWObject_Room;

#include "../AI_Auxiliary.h"
#include "../AI_KnowledgeBase.h"
#include "../../WNameHash.h"

//Action estimation parameters
class CAI_ActionEstimate
{
public:
	//Parameters
	enum{
		OFFENSIVE = 0,  
		DEFENSIVE,
		EXPLORATION,	
		LOYALTY,		
		LAZINESS,		
		STEALTH,
		VARIETY,

		NUM_PARAMS,
	};
	static const char * ms_lTranslateParam[];

protected:
	//Validity flag. This is true if any parameter is positive.
	bool m_bValid;

	//Parameter values (always -100..100)
	int8 m_lValues[NUM_PARAMS];

public:
	CAI_ActionEstimate();
	CAI_ActionEstimate(int nParam, ...);	

	//Get/set values. Value can never exceed 100 or fall below -100. 
	const int Get(int _iParam) const;
	int Set(int _iParam, int _Value);

	//Check validity
	const bool IsValid() const;

	//Max value
	operator int() const;

	//Factor value
	int Mult(int _iParam, fp32 _Factor);
};

// A struct holding the neccessary data for handling scenepoints
struct CHandleScenepoints
{
public:
	CHandleScenepoints();
	~CHandleScenepoints();
	void Clear(bool _bSaveLasts);

	int				m_StayTimeout;			// When we should expire after reaching our goal
	CVec3Dfp32		m_Destination;			// Current destination
	bool			m_bBehaviourStarted;	// True when a behaviour was started fromn the SP (when behaviour is released the SP should expire)
	CWO_ScenePoint* m_pScenePoint;			// Current scenepoint
	CWO_ScenePoint* m_pScenePoint01;		// Prev scenepoints
	CWO_ScenePoint* m_pScenePoint02;
	CWO_ScenePoint* m_pScenePoint03;
	CWO_ScenePoint* m_pScenePoint04;
	CWO_ScenePoint* m_pScenePoint05;
};


class CAI_ActionHandler;
//Handles estimation and execution of alla actions
class CAI_Action : public CReferenceCount
{
//NOTE:
//When creating a new action, the following should be done:
// - Add action type (below)
// - Declare class (in appropriate .h file)
// - Add action string ID to CAI_Action::ms_lStrID (in CAI_Action.cpp)
// - Add constructor call to CAI_Action::CreateAction (in CAI_Action.cpp)
// - Define class (in appropriate .cpp file)
//If you need some additional parameters that can be set from registry, do the following as well: 
// - Declare param enum(s). Make sure params do not conflict with any of parents' params.
// - Declare/define appropriate SetParameter overloads.
// - Declare/define a StrToParam overload
// - If you want a flags parameter, define/declare a StrToFlags overload.
//If action can be altered by script (such as the restrict action and any action that can be used 
//as action overrides), you must also implement the OnDeltaLoad and OnDeltaSave methods.

public:
	//Action types (keep the ms_lStrID string list consistent with below definitions)
	enum {
		INVALID = 0,

		//GENERAL ACTIONS
		
		//Auxiliary prio actions

		//Idle prio actions
		MAKE_IDLE_CALL,	
		IDLE,
		CHECKDEAD,
		FOLLOW,
		HOLD,
		RESTRICT,
		PATROL_PATH,
		ROAM,
		FLY_ROAM,
		LOOK,

		//Alert prio actions
		SCAN,
		ESCALATE_ODD,
		ESCALATE_THREAT,
		INVESTIGATE,

		//Danger prio actions
		KEEP_DISTANCE,
		WATCH,
		THREATEN,
		WARN,

		//Combat prio actions
		CLOSE,
		COMBAT,
		COMBATLEADER,
		COVER,
		FLEE,
		DARKLING_CLOSE,
		MEATFACE_CLOSE,
		DARKLING_JUMPCLOSE,
		FLY_COMBAT,

		//Weapon actions
		DARKLING_ATTACK,
		ANGELUS_ATTACK,
		WEAPON_AIM,
		WEAPON_ATTACK_RANGED,
		MELEE_FIGHT,

		//Forced prio actions

		//Number of actions...
		NUM_ACTIONS,
	};
	static const char* ms_lStrID[];

	//Prioriy levels
	// A prio level consists of two parts:
	// Prio class: Set through PARAM_PRIO, masked with MASK_PRIO_ONLY
	// Prio add: Set through PARAM_PRIOADD, masked with MAX_PRIO_ADD
	// A behaviour with higher prio but the same prio class will NOT abort a behviour
	enum { //Consts really...
		MASK_PRIO_ONLY		= 0xE0,
		MAX_PRIO_ADD		= 0x1F,
	};
	enum {
		PRIO_MIN			= 0,
		PRIO_AUXILIARY		= 0x20,
		PRIO_IDLE			= 0x40,	// Roam, PatrolPath etc
		PRIO_ALERT			= 0x60,	// Watch, Search etc
		PRIO_DANGER			= 0x80,	// Escalatexxx, KeepDistance, Threaten, Warn etc
		PRIO_COMBAT			= 0xA0,	// Combat, Close etc
		PRIO_FORCED			= 0xC0,	// None at the moment, die?
		PRIO_MAX			= 0xFF,
	};

	//Parameters
	enum {
		PARAM_INVALID	= -1,
		
		PARAM_PRIO = 0,			// Priority class
		PARAM_PRIOADD,			// Priority subclass
		PARAM_ID,				// Auxiliary ID used for differentiating actions of same type
		PARAM_INTERVAL,			// Min interval between ::OnStart
		PARAM_SPEECH,			// Std speech(-1), none(0) or param(1+) at ::OnStart
		PARAM_BASE,

		PARAM_MAX = 100,
	};

	//Param types
	enum {
		PARAMTYPE_INVALID = 0,
		PARAMTYPE_INT,			//int	
		PARAMTYPE_FLOAT,		//fp32
		PARAMTYPE_VEC,			//CVec3Dfp32
		PARAMTYPE_TARGET,		//Object target name; CStr (don't evaluate in action definition)
		PARAMTYPE_PRIO,			//Action priority; int (evaluate in action definition)
		PARAMTYPE_FLAGS,		//Action specific flags; int (evaluate in action definition)
		PARAMTYPE_SPECIAL,		//Any uncommon param that should be evaluated; CStr (don't evaluate in action definition)
	};

	//Expiration tick for ongoing action.
	enum {
		ET_EXPIRED = 0,	//Action is always expired
		ET_INDEFINITE = -1,	//Action will never become implicitly expired
	};

protected:
	//Action type
	friend class CAI_ActionHandler;
	uint8 m_Type;
	uint8 m_ID;				// Either 0 or a value that tells *ai_killactions what individual action to kill
	int8 m_iSpeech;			// -1=speak normally,-2=mute,0+=Use the designated sound OnStart
	enum
	{
		SPEAK_DEFAULT = -1,
		SPEAK_MUTE	  = -2,
	};
	int32 m_MinInterval;	// Min nbr of ticks between OnStarts

	enum {
		TYPE_ATTACK	=		1 << 0,		// Combat action that tries to hurt target
		TYPE_OFFENSIVE = 	1 << 1,		// Combat action that tries to behave offensively
		TYPE_DEFENSIVE =	1 << 2,		// Combat action that tries to avoid getting hurt
		TYPE_MOVE =			1 << 3,		// Action that will move the bot (not just turn it)
		TYPE_SOUND =		1 << 4,		// Action that may emit a sound
		TYPE_SEARCH	=		1 << 5,		// Action that tries to find something unknown
		TYPE_DEATH =		1 << 6,		// Action that reacts to dead bodies
	};

	// m_TypeFlags is a bitvector that determines what general kind an action is
	// Combine the flags above for m_TypeFlags to determine what general class/classes an action
	// belongs to. It is used by ActionOverrides to determine among what actions to choose from
	uint8 m_TypeFlags;

	//The action handler
	class CAI_ActionHandler* m_pAH;

	//Priority determines which actions will be taken first if valid. This can be used
	//to make decision making more deterministic and reduce estimation costs.
	uint8 m_Priority;	// Priority of the action, don't change this at runtime as it would fuck up the sort of the actions list
	uint8 m_OverridePriority;

	//This can be set in OnTakeAction to control how long an action should go on before 
	//giving other actions the chance to take over, or in constructor if an action should 
	//have some default expiration. Otherwise default expiration is expired, which means 
	//action will only be taken once before giving other actions the chance to take over.
	int m_ExpirationTick;

	//The last tick we started and stopped taking action
	int m_LastStartTick;
	int m_LastStopTick;

	bool m_bTaken;
	bool m_bPaused;
	bool m_bRemoveOnRefresh;

	// Sets the given behaviour with _Duration and _pScenePoint.
	// Adds the actions priority before calling the AI, only actions with higher prio van stop the behaviour
	bool SetWantedBehaviour(int _iBehaviour,int _Duration,CWO_ScenePoint* _pScenePoint);

	//Calls main ai ontrack/onlook with soft look as default if at suitably low prio
	void OnTrackDir(CVec3Dfp32 _Dir, int _iObj, int _Time);
	void OnTrackPos(const CVec3Dfp32& _Pos, int _Time,bool bHeadingOnly = false);
	void OnTrackObj(int _iObj,int _Time,bool bHeadingOnly = false);
	void OnTrackAgent(CAI_AgentInfo* _pAgent,int _Time, bool _bHeadingOnly = false);

	// Speak _iSpeech unless overridden by m_iSpeach etc
	// Returns -1 or the variant spoken
	int SpeakRandom(int _iSpeech,fp32 _DelaySeconds = 0.0f);

	// Speak _iSpeech using _iVariant unless overridden by m_iSpeach etc
	// Returns -1 or the variant spoken
	int SpeakVariant(int _iSpeech,int _iVariant,bool _bRandomPause = false);

public:
	CAI_Action(class CAI_ActionHandler* _pAH = NULL, int _Priority = PRIO_MIN);

	//Type accessor
	int GetType();

	// Adds any unique behaviours to _liBehaviours, returns the nbr of added behaviours
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;
	virtual int GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const;
	virtual int GetUsedAcs(TArray<int16>& _liAcs) const;
	virtual bool AnimationEvent(int _iUser, int _iEvent);
	// Aux method for GetUsedBehaviours
	static int AddUnique(TArray<int16> _liBehaviours,int16 _iBehaviour);

	//Syntactic sugar...Gets using AI.
	CAI_Core* AI();

	//Create and return action of given type
	static spCAI_Action CreateAction(int _Type, class CAI_ActionHandler * _pAH);

	//Get corresponding action type from string
	static int StrToType(CStr _Str);

	//Set action parameters. Param is enum defined in specific action.
	virtual void SetParameter(int _Param, int _Val);
	virtual void SetParameter(int _Param, fp32 _Val);
	virtual void SetParameter(int _Param, const CVec3Dfp32& _Val);
	virtual void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from sting (used when parsing registry). If optional type result pointer
	//is given, this is set to parameter type. (Overload this for action specific params)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Get action priority value for given string
	static int StrToPrio(CStr _PrioStr);

	//Get flags for given string and action type (overload this for action specific flags)
	virtual int StrToFlags(CStr _FlagsStr);

	// GetPriority(false) returns the actual priority of the action as given in AIActions.xrg
	// GetPriority(true) returns the prioclass of thaction when calling TestTakeActions
	// This may be different because GetPriority(true) is only used to determine when an action
	// potentially could disrupt a dialogue or behaviour.
	virtual int GetPriority(bool _bTest = false);

	//The devices the action need to be valid. Returns (1 << <DEVICE1> | 1 << <DEVICE2> etc)
	virtual int MandatoryDevices();

	//Check if the action can be taken or not
	virtual bool IsValid();
	//Check if the action can be taken on _pTarget or not
	virtual bool IsValid(CAI_AgentInfo* _pTarget);
	//Returns true when the action is taken (
	bool IsTaken();

	//Check if the devices needed for the action is available
	bool IsDeviceValid();
	
	//Estimate how good the action will be to take (-100..100)
	virtual CAI_ActionEstimate GetEstimate() pure;

	//Action estimate result, as calculated by personality will be modified by this value 
	//(0..1) based on how long the action have been taken, to encourage variety 
	//(0 = no reduction, 1 = full reduction of estimate)
	virtual fp32 VarietyReduction();

	// Checks held scenepoints if they are still takeable
	virtual void RequestScenepoints();
	
	//Take action!
	// Returns priority of taken action
	virtual void OnTakeAction() pure;
	virtual bool OnServiceBehaviour();

	//Set expiration delay if not previously set. Delay is _Time + _RndAdd * <Random 0..1> 
	void SetExpiration(int _Time, int _RndAdd = 0);
	M_FORCEINLINE void SetExpirationExpired()
	{
		m_ExpirationTick = ET_EXPIRED; 
	}
	M_FORCEINLINE void SetExpirationIndefinite()
	{
		m_ExpirationTick = ET_INDEFINITE;
	}

	// Delays expiration to at least another _Ticks
	void SetMinExpirationDuration(int _Tick);

	//Check if action has expired
	virtual bool IsExpired();

	// Returns true if the action will expire from time the next tick
	// Will not return true if already expired or has ET_INDEFINITE
	bool ExpiresNextTick();

	//Will be called whenever an ongoing action is started being taken
	virtual void OnStart();

	//Will be called whenever an ongoing action expires or is overridden.
	virtual void OnQuit();

	// Nbr of ticks since start, -1 if not running
	int GetRunTicks();
	//Expires the action and delays the taking of it by at least _Delay ticks
	void ExpireWithDelay(int _Delay);

	//Change action handler we belong to
	void SetActionHandler(class CAI_ActionHandler * _pAH);
	void SetPause(bool _bPause);
	bool IsPaused();

	// Called before ::OnTakeActions
	virtual bool CheckPrioClass();

	//Should thus action be paused by action override. Isn't that obvious?
	virtual bool IsOverridableByActionOverride();

	//Savegame. Implemented sparingly, since only certain special actions 
	//(action overrides and restriction) actually need be saved.
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);

#ifndef M_RTM
	//Debug info
	virtual CStr GetDebugString();
#endif
};
typedef TPtr<CAI_Action> spCAI_Action;


//Auxiliary class for sorting action pointers into prio queue according to estimate
class CAI_EstimatedAction
{
public:
	CAI_EstimatedAction(spCAI_Action _pAction = NULL, int _Estimate = 0);
	spCAI_Action m_pAction;
	int m_Estimate;
	int GetPriority();
};

//Class that manages available actions. Actions are sorted according to priority.
class CAI_ActionHandler 
{
public:

	//Action override types
	enum {
		PATROL = ACTION_OVERRIDE_ENUM_BASE+1,
		BEHAVIOUR = ACTION_OVERRIDE_ENUM_BASE+2,
		EXPLORE = ACTION_OVERRIDE_ENUM_BASE+4,
		HOLD = ACTION_OVERRIDE_ENUM_BASE+5,
		ENGAGE = ACTION_OVERRIDE_ENUM_BASE+8,
	};

	// Degrees of being fired at
	// NONE nonbody has fired at you
	// GRAZED someone fired near you but missed (wether you would have been hurt if hit is undetermined)
	// UNINJURED someone hit you but the armour absorbed the hit (scary though)
	// INJURED someone hit you and hurt you, Ouch!
	enum {
		NONE = 0,
		GRAZED,
		UNINJURED,
		INJURED,
	};

	TArray<int>		m_lKillTypes;			// Actiontypes to kill
	TArray<uint8>	m_lKillIDs;				// IDs for actions to kill

	// Returns NONE,GRAZED,UNINJURED,INJURED
	// If _piAttacker will hold the index of the attacker if any and the variables will be cleared if
	// _bClear is true. Typically one uses FiredAt() with no params to determine if one was fired at
	// and then if it returned > 0 one calls again to retrieve the attacker and clear the variables.
	// _IgnoreDamageTypes is a mask of damage types to ignore (Use CRPG_Object::ms_DamageTypeStr to translate)
	int FiredAt(int* _piAttacker,uint32 _IgnoreDamageTypes,bool _bClear);

protected:
	//Attributes

	//The AI
	CAI_Core* m_pAI;

	//The sorted action pointer list
	TSimpleSortedList<CAI_Action> m_lpActions;	
	int m_CurPrioClass;
	int m_nActiveActions;	// Nbr of active actions last frame

	//The action save list. The only function of this is to keep actions alive.
	TArray<spCAI_Action> m_lspAs;

	//The current essential devices. If at least one of these are available, then some action may be valid.
	int m_EssentialDevices;

	//The essentialdevices combo list. If none of the essential devices above were available and none of 
	//these combos are available, then no further actions can be valid.
	CSimpleIntList m_lEssentialDeviceCombos;

	//Will we automatically create weapon handling actions? If so, some actions might be excluded.
	bool m_bWeaponHandling;
	TArray<int> m_lWeaponHandlerExcludes;
	
	//Action registry keys. These are evaluated and converted into actions on first refresh
	TArray<spCRegistry> m_lspActionRegs;

protected:
	//Action data:

	//Current available scenepoints and the distance delimiter used to get them.
	//Note that all these scenepoints need not actually be within the given distance, since it's
	//cheaper to get a chunk of scenepoints that include all that actually are within this distance.
	//This list is updated every few frames.
	TArray<CWO_ScenePoint *> m_lpScenePoints;
	uint32 m_ScenePointTypes;	// Bitfield over what types of scenepoints m_lpScenePoints has
	int32 m_NextScenePointTick;	// Next tick we may asked for scenepoints
	int32 m_LastGetBestSPTimer;
#define AH_GETBESTSCENEPOINT_PERIOD		90
	
	bool m_bCheckValidTeams;	// When true we check teams as well
	//Scenepoints held last frame and taken this frame
	TSimpleDynamicList<CWO_ScenePoint*> m_lpLastHeldScenePoints;
	TSimpleDynamicList<CWO_ScenePoint*> m_lpCurrentHeldScenePoints;

	//Override scene point
	CWO_ScenePoint * m_pOverrideScenePoint;
	bool m_bClearOverrideScenePoint;

	//Reported events (1 << <EVENTID> | 1 << <EVENTID> etc) since last OnTakeActions
	int m_ReportedEvents;

	//Friendly agent whom we wish to follow (or guard)
	int m_iLeader;
	int m_LeaderFindingTick;

	//Friendly agent who we think wants to follow us
	int m_iFollower;
	int m_FollowerFindingTick;

public:
	// CAI_Core and its descendants must have access to m_iTarget for its SwitchTarget() method
	// (Eventually, I will move all FindTarget code over to CAI_Core and CKnowledgeBase
	//Enemy agent whom we consider our primary target to attack or avoid
#define AI_RETARGET_MAXRANGE	96.0f

	uint32 m_RetargetTimeout;	// When != 0 determines when to check m_Retarget again
	CVec3Dfp32 m_RetargetPos;
	CVec3Dfp32 m_RetargetUp;
	CVec3Dfp32 m_RetargetPosPF;
	CWO_ScenePoint* m_pRetargetSP;

	// Primary and secondary DETECTED+ enemy
	int32 m_iTarget;
	int32 m_iTarget2;
	bool m_bFindNewTarget;
	// Primary and secondary DETECTED+ hostile
	int32 m_iHostile;
	int32 m_iHostile2;
	// Primary and secondary friend
	int32 m_iFriend;
	int32 m_iFriend2;
	// Primary and secondary NOTICED enemy
	int32 m_iInvestigate;
	int32 m_iInvestigate2;
	int32 m_iInvestigateObj;

	CWO_ScenePoint* m_pInterestingScenepoint;
	int32 m_InterestingScenepointTimeout;

protected:
	int m_iAttacker;		// Object index of bot that attacked us

	uint32 m_DamageType;		// Type of damage that hit us
	bool m_bWasInjured;		// Set when bot was injured, reset by actions
	bool m_bWasUninjured;	// Set when bot was hit but NOT injured
	bool m_bWasGrazed;		// Set when bot was nearly hit by ranged fire

	// ActionOverride
	// ==============
	// The action system (as of this writing, 2003-04-16) works "well" when running in free mode
	// ie not affected by scripts, impulses etc. In order to move away from behaviours we must
	// somehow enable all the old Ogier stuff with the action system
	// ActionOverrides will be the solution that does this.
	
	friend class CAI_Action;
	TArray<spCAI_Action> m_lspActionOverrides;	// When m_lspActionOverrides.Len() != 0 we should skip IDLE actions	
	void ClearActionOverrides(bool _bClearEnemyOverride = false);

	// m_lScenepointHistory: A list of scenepoints recently activated by the ai, recent scenepoints are much
	// less fit for selection, especially the last activated which has less fitness than a lowprio scenepoint.
	// The list is updated through the 
	#define AH_SP_HISTORY_COUNT	5
	CWO_ScenePoint* m_lScenepointHistory[AH_SP_HISTORY_COUNT];
	CWO_ScenePoint* m_lLookpointHistory[AH_SP_HISTORY_COUNT];

public:
	// Updates the SP history, setting _pScenePoint as the last visited ie least eligible
	CWO_ScenePoint* GetPrevScenepoint(uint _iPos = 0);
	void UpdateScenepointHistory(CWO_ScenePoint* _pScenePoint);
	void UpdateLookpointHistory(CWO_ScenePoint* _pLookPoint);
	
	// Checks each actions held scenepoints if they are still requestable
	void RequestScenepoints();

	int m_iEnemyOverride;				// When m_iEnemyOverride != -1 we treat that agent as an enemy no matter what
	int m_FoundCorpseTick;	// When we last found a body, -1 means it's invalid
	int m_ExitCombatTick;	// When we last exited combat, -1 means it's invalid

	// Returns true if at least one action fit the _iBehaviour parameter
	bool SetActionOverride(int _iAction, CStr _sParamName, fp32 _DistParam, fp32 _MaxDistParam, int _iActivator);
	
	// A more generic SetActionOverride(). _iBehaviour is the kind of action and _Params holds a list
	// of parameters for the action. *** I will eventually rewrite SetActionOverride() above to be included
	// in this call to make for a cleaner design.
	bool SetActionOverride(int _iAction,TArray<CNameValue>& _Params);

	// Add _pAction to action overrides, removing any currently taken actions
	// and locking all IDLE or less actions
	void AddActionOverride(CAI_Action* _pAction,int _Prio = CAI_Action::PRIO_IDLE);
	// Sets the preferred enemy to _iEnemyOverride (or claers it if _iEnemyOverride == 0)
	void SetEnemyOverride(int _iEnemyOverride);

	// Helper methods that returns wether a position is restricted or not
	bool IsRestricted(const CVec3Dfp32& _Pos, bool _bActivate = false);
	bool IsRestricted(CAI_AgentInfo* _pTarget, bool _bActivate = false);

	//Do we have any action overrides?
	bool HasActionOverrides();
	int GetCurrentPriorityClass();

	spCAI_Action_Restrict m_spRestriction;

protected:
	//General private methods
	
	//Checks if any actions might be valid, according to the essential devices criteria
	bool CheckEssentialDevices();

	//Add action to handler. Any default params from action definition is assumed to have been already set.
	void AddAction(spCAI_Action _pAction, bool _keepFlag = true);

	// Removes the given action from all members except m_lspActionOverrides and m_lspAs
	// Returns true if it succeeded and false if not
	bool RemoveAction(spCAI_Action _pAction);

	//Create action from given definition
	spCAI_Action CreateAction(CAI_ActionDefinition * _pActionDef);

	//Override any current actions of lower prio
	void Override(int _Priority);

public:
	//General interface

	CAI_ActionHandler();
	void Init(bool _bClearRegs = true);
	void SetAI(CAI_Core* _pAI);	
	void ReInit(CAI_Core* _pAI);

	//Get AI user
	CAI_Core* GetAI();

	//Create and add any actions that have been defined by registries. This should be done on first
	//refresh to make sure world has been spawned. Note that this must be done _after_ weapon and item
	//handler has been initialized for weapon handling action set actions to be correctly created.
	void OnSpawn();

	// Add unique behaviours used from individual actions to _liBehaviours
	// Returns the number of added behaviours
	int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;
	int GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const;
	int GetUsedAcs(TArray<int16>& _liAcs) const;
	
	//Returns true if actionhandler thinks its OK to deactivate the bot
	//What must be fulfilled to be ready for deactivation?
	//m_CurPrioClass <= PRIO_IDLE
	//m_lspActionOverrides can only contain Hold
	//Current position may not be restricted
	bool CanDeactivate();
	int GetCurPrioClass() {return(m_CurPrioClass & CAI_Action::MASK_PRIO_ONLY);}

	// Kills all actions referred to by KillActions
	int HandleActionKills();

	//Evaluate all actions of highest priority and choose the one with highest positive estimate, 
	//given any remaining devices. Never choose an action with zero or less estimate.
	//Repeat this for every prio group until all devices have been used or we run out of actions.

	// Handles action execution and action estimates as well as action ovcerrides etc
	int OnTakeActions();

	// Handles those few action that need to run during behaviours
	int OnServiceBehaviourActions();

	// Returns the nearest EnginePathScenePoint (EPSP) within _RangeDelimiter
	CWO_ScenePoint* GetNearestEPSP(int _EPSPType,fp32 _RangeDelimiter);

	// Removes the given action from all members except m_lspActionOverrides and m_lspAs
	// Returns true if it succeeded and false if not
	bool RemoveActions(int _ActionType);
	bool ExpireActions(int _ActionType,int _Duration = 0);

	// Returns true if an action with higher prio clas than _Prio would be taken
	// Returns the new prio in _pPrio if supplied
	bool TestTakeActions(int32 _Prio,int32* _pPrio);

	//Create actions and action sets defined by AI resource handler and given registry key. For actions 
	//value of key itself is the name by which action is identified in resource handler. Any children 
	//are parameter values that will overwrite the defaults defined in resource handler.
	void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	//Add action to be automatically taken until expiration, or until overridden
	void RequestOverride(int _Priority);
	int m_CurOverridePrio;

	//Notify action handler of any newly acquired weapon, so that appropriate actions can be added
	void ReportNewWeapon(int _Type);

	//Report that an event has occurred and let action handler decide what to do
	//If _iWho != 0 is tells us who was responsible, meaning depends on event
	void ReportEvent(int _iEvent,int _iWho,int _Data = -1);

	//Get highest prio of current actions
	int GetCurPriority();

#ifndef M_RTM
	//Debug info
	CStr GetDebugString();
#endif

	//Savegame
	void OnDeltaLoad(CCFile* _pFile);
	void OnDeltaSave(CCFile* _pFile);

public:
	//Action data auxiliaries

	//Tries to find a good target.
	//Returns pointer to targets agentinfo if successful, else NULL
	// CAI_AgentInfo * FindTarget(bool _bFindNow = false);

	//Checks if the given server index points to a valid target.
	//Returns pointer to targets agent info if so else NULL
	CAI_AgentInfo* IsValidTarget(int _iObj);

	//Check current target and acquire a new one if this is bad. Return pointer to target object or NULL
	//If _bFindNow == true we will find a new target instead of reusing any previously found
	//If _bIgnoreBravery == true we will accept the target even if our bravery wouldn't let us
	CAI_AgentInfo* AcquireTarget();
	CAI_AgentInfo* AcquireHostile();


	//Suggest that we should look for another target. Call when target is bad for an action and we're reasonably
	//sure there are no other actions that target is better for.
	void SuggestTargetChange();

	// Returns true if _iObj is a security threat to this ai
	bool IsSecurityThreat(int _iObj);

	//Checks if the given server index points to a valid agent with a relation of _MinRelation or worse
	//Returns pointer to hostile's agent info if so else NULL
	CAI_AgentInfo * IsValidRelation(int _iObj,int _MinRelation = CAI_AgentInfo::HOSTILE,int _MaxRelation = CAI_AgentInfo::ENEMY);

	//Returns pointer to hostile agents info that we have a _MinRelation or worse with
	// If none is found it returns NULL
	CAI_AgentInfo* FindRelation(bool _bFindNow = false,
		int _MinRelation = CAI_AgentInfo::HOSTILE,int _MaxRelation = CAI_AgentInfo::ENEMY,
		int _MinAwareness = CAI_AgentInfo::DETECTED,int _MaxAwareness = CAI_AgentInfo::SPOTTED);

	//Will we engage the given opponent if we try to initiate melee now?
	bool CanEngage(CAI_AgentInfo * _pTarget);
	// Returns the player agent if the ai should stop for the player (it doesn't actually stop or anything)
	CAI_AgentInfo* StopForPlayer();
	// ================================================================================= 
	// Scenepos related methods
	// =================================================================================
	M_FORCEINLINE bool ShouldCheckForValidTeams() { return(m_bCheckValidTeams);};
	M_FORCEINLINE void SetCheckForValidTeams(bool _bFlag) { m_bCheckValidTeams = _bFlag;};

	//Set override scene point from name (clears any previous override scene point)
	void SetOverrideScenePoint(CStr _Name = "");
	// Measures the light at _pSP using traclines if _bTraceLines is true
	fp32 MeasureLightAtScenepoint(CWO_ScenePoint* _pSP,bool _bTraceLines = false);

	// Get all scenepoints that are potentially within the given distance and valid for us
	int32 GetScenePoints(fp32 _RangeDelimiter);
	//Get i'th scene point of those currently in scene point list
	CWO_ScenePoint * IterateScenePoints(int _i);
#ifdef M_Profile
	int32 DebugDrawScenepoints(bool _bUseable,fp32 _RangeDelimiter,CWObject_Room* _pRoom);
#endif
	int32 GetEnginepathSPCount(CWO_ScenePoint* _pCurSP);
	//Get i'th enginepath scenepoint of _pCurSP
	CWO_ScenePoint* GetNthEnginepathSP(CWO_ScenePoint* _pCurSP,int _i);
	int32 GetEnginepathID(CWO_ScenePoint* _pToSP);

	// Returns wether there exist scenepoints of the given _Type
	bool ScenePointTypeAvailable(int _Type,fp32 _MaxRange);

	// Checks wether the SP is OK for tracelines
	bool CheckSPTraceline(CWO_ScenePoint* _pSP,CVec3Dfp32 _TargetPos,int _iTarget);
	bool CheckLookSPTraceline(CWO_ScenePoint* _pSP);

	// Returns the best scenepoint along the ray _Start to _End within maxangle
	// CWO_ScenePoint* GetBestScenepointAlongRay(int _Type,fp32 _MaxAngle,CVec3Dfp32 _Start,CVec3Dfp32 _End,fp32 _MinRange,fp32 _MaxRange,CVec3Dfp32 _TargetPos);

	// Bitfield for _Flags
	enum
	{
		SP_PREFER_NEAR_TARGETPOS = 1,		// SPS near _TargetPos have higher fitness
		SP_REQUIRE_TARGET_CLOSER = 2,		// SP must be closer to target than we are now 
		SP_RANDOM_RANGE			= 4,		// Pick randomly from range
		SP_PREFER_PLAYERFOV		= 8,		// Bias SPs in player FOV
		SP_REQUIRE_PLAYERFOV	= 16,		// Skip SPS outside player FOV
		SP_AVOID_TARGET			= 32,		// SPs are preferred if close
		SP_PREFER_HEADING		= 64,		// Prefer SPs in the callers present heading
		SP_AVOID_LIGHT			= 128,		// Darker SPs are better
		SP_PREFER_LIGHT			= 256,		// Brighter SPs are better
		SP_SAME_UP				= 512,		// SP must have close to same up as the supplied Matrix
		SP_DIFFERENT_UP			= 1024,		// SP must have radically different up than supplied Matrix
		SP_RECENT_FORBIDDEN		= 2048,		// Recent SPs cannot be chosen again
		SP_ENGINEPATH_SP		= 4096,		// Only consider enginepath SPs from on pSkip01
		SP_COMBATBEHAVIOURS		= 8192,		// Consider CB scenepoints (GetTacFlags() != 0)
		SP_PREFER_INSIDE_ARC	= 16384,	// Outside arc means less fitness, that's all
		SP_RETREAT_ARC			= 32768,	// We allow target outside arc as long as our cur pos was inside it
		SP_PREFER_FLANK			= 65536,	// Left / right 90 degrees arcs of target are preferred
	};

	// _Type: SP type to consider
	// _Flags: Bitfield that determines how to evaluate the SPs
	// _MinRange: Minimum range to SP
	// _MaxRange: Maxmium range to SP
	// _TargetPos: Used when SP_PREFER_NEAR_TARGETPOS and SP_AVOID_TARGET are used in _Flags
	// _TargetMinRange: Min range of SP to _TargetPos
	CWO_ScenePoint* GetBestScenePoint(int _Type,uint _Flags,fp32 _MinRange,fp32 _MaxRange,
		const CMat4Dfp32& _pMat,
		const CVec3Dfp32& _pTargetPos,fp32 _TargetMinRange,int _iTarget);

	//Check if there are any spawned scenepoints of given type fulfilling the optional criteria.
	bool CheckScenePoints(int _Type, fp32 _MaxRange = _FP32_MAX, fp32 _MinRange = 0.0f);
	bool HandleSitFlag(CWO_ScenePoint* _pScenePoint);
#ifdef M_Profile
	void DebugDrawScenePoint(CWO_ScenePoint* _pSP,bool _bActivated, uint32 _Color = 0, fp32 _Duration = 0.0f);
#endif

	// NOTE When/if we start using scene point indices and call all scenepoint stuff through CAI_ActionHandler
	// we should change _pScenePoint into its index
	bool ActivateScenePoint(CWO_ScenePoint* _pScenePoint,int _Prio = CAI_Action::PRIO_IDLE,CMat4Dfp32* _pPOMat = NULL);
	bool RequestScenePoint(CWO_ScenePoint* _pScenePoint,uint8 _Prio = 1);
	bool AlignScenepoint(CWO_ScenePoint* _pScenePoint,int _Prio = CAI_Action::PRIO_IDLE);
	// Handles arcs for TACTICAL and COVER scenepoints dealing with NoTargetFlag etc
	bool CheckScenePointTarget(CWO_ScenePoint* _pScenePoint, CAI_AgentInfo* _pTarget);
	bool CheckScenePointTarget(CWO_ScenePoint* _pScenePoint, int _iTarget);

	void ReleaseSPs(int32 _Duration);

	// A team has been removed
	void OnRemoveTeam(int32 _iTeam);

	//Refreshes held scenepoint info and releases any scenepoints we don't hold anymore
	void OnRefreshScenePointStatus();

	//Check if override scene point fulfills the given conditions. If the destructive flag is true
	//and types match, override scene point status will be lost at next scenepoint refresh.
	//If successful, return override scenepoint pointer or NULL if check fails.
	CWO_ScenePoint* CheckOverrideScenePoint(uint16 _Type = -1, fp32 _MaxRange = _FP32_MAX, fp32 _MinRange = 0.0f, bool _bDestructive = false);

	// *** Temporary solution until action/objective system is fully done
	// Returns true if any CustomAttack action is valid on _pTarget
	// Used by CAI_Core::CanAttack
	bool CanAttack(CAI_AgentInfo* _pTarget);
	// Pass on an animation event to the first action that handle the event
	// Returns true if it was handled and false if not.
	// Note: If more than one action could handle the event only the highest prio one will actually do it
	bool AnimationEvent(int _iUser, int _iEvent);
	void PrecacheEffect(CStr _sEffect) const;

	// Returns true if an action of the given type is available
	bool HasAction(int _ActionType, bool _bActive = false);
	// Returns 0 or the closest teammate with the given _ActionType
	int GetClosestActionUser(int _ActionType,fp32 _MaxRange = _FP32_MAX, bool _bActive = false);
	// Sets _Param to _Val for the first _ActionType encountered
	// Returns true if we found one and false if not
	bool SetActionParameter(int _ActionType, int _Param, CStr _Val);
	bool SetActionParameter(int _ActionType, int _Param, int _Val);
	// Returns an action of _Type in _pAction
	bool GetAction(int _Type,CAI_Action** _pAction);
	// _iActionType will be paused for _PauseTicks ticks
	// _PauseTicks=0: unpause, _PauseTicks=-1 pause forever(until unpaused)
	bool PauseAction(int32 _iActionType, int32 _PauseTicks);
	// Can we respond to a call by _iCaller with _iSoundType,_iSoundVariant? If true we will!
	int ReceiveCall(int _iCaller, CMat4Dfp32& _PosMat, int _iSoundVariant);

	//Get new random movement destination, at or closer to given distance preferrably close to the given heading
	//If no such destination can be found, fail with CVec3Dfp32(_FP32_MAX).
	CVec3Dfp32 GetNewDestination(fp32 _Distance, fp32 _Heading, bool _bPathfind = true);

public:
	// Tells the bot to set a particular search object, returns true if the bot can search for the object
	bool SetInvestigateObject(int32 _iObject);

	int ReportedEvents();
	int Leader();
	int Target();
	int Follower();
	int Hostile();
};






//Move the below to other files...


//Auxiliary prio actions//////////////////////////////////////////////////////////////////////////////


//Idle priority actions///////////////////////////////////////////////////////////////////////////////

// This action handles dialogue between two bots, bith using CAI_Action_IdleCall
// ::GetEstimate() first checks for eligible Talk SPs
// ::GetEstimate() the searches for eligible users, how depends on if a Speak msg OnActivate is present or not
//		Speak present: Find the USERS and LINK of the dialogua and make sure ALL of them are eligible
//		Speak not present: Find any random CAI_Action_IdleCall user that is eligible
// Tell all users where to go in the initial 
// ::GetEstimate() then searches for eligible CAI_Action_IdleCall users at the Pos of the SP
// ::GetEstimate() stores the m_iPartner and will start moving towards the SP
// ::OnTakeAction() we call ::ReceiveCall() repeatedly until failure or we reach our goal
// Receiver will move towards his goal given through ::ReceiveCall()
// Caller will wait for Receiver at his goal, Receiver will wait for Caller at his goal
// When Caller and Receiver has arrived Caller will activate the SP and:
// If there's no Speak msg in the SP:
// 
// If there's a Speak msg in the SP:

class CAI_Action_IdleCall : public CAI_Action
{
	friend class CAI_Core;
protected:
	fp32 m_MinRange;		// Min range to scenepoint and receiver
	fp32 m_MaxRange;		// Max range to scenepoint and receiver

	// These members deal with random dialogue ie when there's no Speak message on the SP
	int m_iSoundVariant;// What variant of IDLE_CALL_RESPONSE?
	int m_iDialogue;	// Dialogue ID if the SP has a dialogue

	int m_iCaller;				// The dialogue initiator when we're receiving (m_bReceiver == true), m_liReceivers.Len() == 0
	int m_iReceiver;			// Only ONE receiver for sanity reasons
#define IDLECALL_SKIP_COUNT	3
	int m_liSkipList[IDLECALL_SKIP_COUNT];	// Previous/bad receivers
	int m_LastReceiveCallTimer;	// When was the last time we got a ReceiveCall() call?
	bool m_bReceiverReady;		// true when receiver is ready
	int m_ReceiverResponseTimer;	// Time when receiver can respond
	uint8 m_Mode;		// 0=call/receive, 1=call only, 2=reveice only
	enum {
		MODE_BOTH = 0,
		MODE_CALL,
		MODE_RECEIVE,
	};

	enum {
		FLAGS_SCENEPOINTS	= 1,	// Allow the use of scenepoints
		FLAGS_RANDOM		= 2,	// Allow random points non SP points, possibly after no SP was found
		FLAGS_CLOSEST_PT	= 4,	// Choose the closest SP instead of a random	
		FLAGS_NEARPLAYER	= 8,	// Prefer SPs near player
		FLAGS_IN_PLAYERFOV	= 16,	// Prefer SPs in player FOV (not neccesarily visible to him though)
		FLAGS_PREFER_HEADING = 32,	// Prefer SPs in forward 180 degree arc
		FLAGS_AVOID_LIGHT	= 64,	// Avoid bright SPs
		FLAGS_PREFER_LIGHT	= 128,	// Prefer bright SPs
	};
	// Various flags
	int				m_Flags;
	static const char * ms_lStrFlags[];

	// Scenepoint stuff
	CVec3Dfp32		m_Destination;		// SP pos
	CMat4Dfp32		m_ReceiverMat;		// Mat given by ReceiveCall
	int				m_StayTimeout;		// Tick when we should no longer stay at the spot but continue the search
	CWO_ScenePoint* m_pScenePoint;		// Current active scenepoint

public:
	CAI_Action_IdleCall(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_IDLE);

	//Parameters
	enum {
		PARAM_MINRANGE,
		PARAM_MAXRANGE,
		PARAM_MODE,
		PARAM_FLAGS,

		PARAM_BASE_IDLECALL,
	};

	virtual void SetParameter(int _Param, CStr _Val);
	virtual void SetParameter(int _Param, int _Val);
	virtual void SetParameter(int _Param, fp32 _Val);
	virtual int StrToParam(CStr _Str, int* _pResType);
	virtual int StrToFlags(CStr _FlagsStr);

	//Mandatory: Sound
	virtual int MandatoryDevices();

	//Valid when we're using idle or lower priority actions
	virtual bool IsValid();

	//Good for variety, but not very stealthy
	virtual CAI_ActionEstimate GetEstimate();

	//Make idle noises every once in a while
	bool FindScenePointDestination();
	// bool FindRandomDestination();
	bool FindDestination();
	bool MoveToDestination();
	bool StayAtDestination();
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
	virtual void RequestScenepoints();

	enum
	{
		MSG_AVAILABLE = 1,	// Querying wether available
		MSG_

	};

	CVec3Dfp32 FindPos(CWO_ScenePoint* _pScenePoint);
	// Result codes from ReceiveCall
	enum
	{
		RESULT_FAIL = -1,	// Failure, can't do sir
		RESULT_THERE = 0,	// We're there, do your thing
		// RESULT_NOT_YET > 0: // The estimated nbr of aiticks to get there
	};

	// RESULT_FAIL = -1		// Failure, caller should abort and note m_iLastPartner
	// RESULT_THERE = 0		// We're there dude, bring it on
	// RESULT_NOT_YET > 1	// We'll be there in a nbr of ticks
	int ReceiveCall(int _iCaller, CMat4Dfp32& _PosMat, int _iSoundVariant);
	// Helper for checking if _iCaller is OK
	bool IsValidReceiver(int _iCaller);
};
typedef TPtr<CAI_Action_IdleCall> spCAI_Action_IdleCall;


class CAI_Action_Idle : public CAI_Action
{
public:
	CAI_Action_Idle(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_IDLE);
	
	fp32 m_Angle;						// The max angle off default headings for turns
	TArray<uint16>	m_liBehaviours;		// List of behaviours to run
	TArray<uint16>	m_liGestures;		// List of behaviours to run
	TArray<uint16>	m_liSounds;			// List of dialogues to play

	//Parameters
	enum {
		PARAM_ANGLE = PARAM_BASE,		// The max angle off default headings for turns
		PARAM_BEHAVIOURS,				// List of behaviours to randomly choose between
		PARAM_GESTURES,					// List of gesture indexes to randomly choose between
		PARAM_SOUNDS,					// List of dialogue indexes to randomly choose between
	
		PARAM_BASE_IDLE,
	};
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;
	virtual int GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const;
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual void SetParameter(int _Param, CStr _Val);
	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Look
	virtual int MandatoryDevices();

	//Quite lazy but boring
	virtual CAI_ActionEstimate GetEstimate();

	//Look idly around. Stay put if possible.
	virtual void OnTakeAction();

	// Turn off animgraph
	virtual void OnStart();

	// Turn on animationgraph again
	virtual void OnQuit();
};

typedef TPtr<CAI_Action_Idle> spCAI_Action_Idle;

// This action will see if it can see any dead bodies within m_MaxRange
// and if it does, will remove the dead from its list and report to teammembers.
class CAI_Action_CheckDead : public CAI_Action
{
	friend class CAI_Core;
public:
	// These flags must be public as they are used by CAI_KnowledgeBase to filter out what

protected:
	SDead	m_Dead;				// The dead as determined by OnSTart() that we check
	fp32	m_MaxRange;			// Max range within which corpses will be considered
	fp32	m_StopRange;		// Stop for behaviour/kneel when within this range
	int8	m_Relation;			// Somewhat tricky concept, determines what relation is required for dead of interest:
								// FRIENDLY		Only friendly are valid
								// UNKNOWN		All relations are OK
								// HOSTILE+		All realtions and above are valid

	bool	m_bFoundCorpse;
	bool	m_bCrouch;					// true if we crouched at the corpse	DEPRECATED?
	int32	m_KneelDoneTick;			// What tick can we rise again?			DEPRECATED?
	int32	m_iCheckCorpseBehaviour;	// What behaviour to play when near the corpse

	CVec3Dfp32 m_PathPos;		//Pathfinder-valid position
	CVec3Dfp32 m_Destination;	//Pos we're avoiding towards
public:
	CAI_Action_CheckDead(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_DANGER);
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;
	

	//Parameters
	enum {
		PARAM_RELATION = PARAM_BASE,	// Relation required for corpse to be interesting
		PARAM_MAXRANGE,					// Only corpses within this range will be LOS checked
		PARAM_STOPRANGE,				// Stop for behaviour/kneel when within this range
		PARAM_BEHAVIOUR,				// Behaviournbr when checking corpse
		PARAM_FLAGS,					// Various flags as determined below

		PARAM_BASE_CHECKDEAD,		
	};

	//Flags
	enum {
		FLAGS_KNEEL			= 1,	// Kneel at corpse (if no behaviour that is)
		FLAGS_GLOBALREMOVE	= 2,	// When true found dead will be globally removed
		FLAGS_TEAMONLY		= 4,	// Will only get mad at murderers of teammates
		FLAGS_AVOID			= 8,	// Avoid corpse instead of investigate, move away until maxrange-16 and then stay there
	};
	int m_Flags;
	static const char* ms_lStrFlags[];
	virtual int StrToFlags(CStr _FlagsStr);

	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	// We just check for dead here
	virtual bool IsValid();
	virtual int GetPriority(bool _bTest = false);

	// Estimate check for dead AND closest range to a dead vs m_MaxRange
	virtual CAI_ActionEstimate GetEstimate();

	// Check one dead
	virtual bool AnimationEvent(int _iUser, int _iEvent);
	virtual void OnTakeAction();
	bool OnAvoid(CVec3Dfp32 _AvoidPos);

	// OnStart will try to find a dead bot within range and valid for m_Flags
	// and fill in the m_Dead member with its data.
	// If m_Dead.m_iVictim == 0 we failed
	virtual void OnStart();
	virtual void OnQuit();

	//Not overridable normally (perhaps make this action dependable)
	virtual bool IsOverridableByActionOverride();
};
typedef TPtr<CAI_Action_CheckDead> spCAI_Action_CheckDead;

class CAI_Action_Follow : public CAI_Action
{
protected:
	//Any specified leader object
	int m_iTrueLeader;

	//Leader-finding range
	fp32 m_FindingRange;

	//Loyalty estimate
	int8 m_Loyalty;

	//Squared following range
	fp32 m_FollowRangeSqr;

	//Next path position to got to
	CVec3Dfp32 m_FollowPos;

public:
	CAI_Action_Follow(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_IDLE);

	//Parameters
	enum {
		PARAM_LEADER = PARAM_BASE,	//The object we should use as leader if possible
		PARAM_FINDINGRANGE,			//The maximum range we will look for leaders	
		PARAM_LOYALTY,				//The loyalty estimate
		PARAM_FOLLOWRANGE,			//Range we try to keep with leader when following

		PARAM_BASE_FOLLOW,
	};
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Move, Turn
	virtual int MandatoryDevices();

	//Valid when there's a leader around
	virtual bool IsValid();

	//Following someone is always a bit safer than being by yourself, although it might be boring
	//and won't satisfy any exploration needs if leader stays put.
	virtual CAI_ActionEstimate GetEstimate();

	//Go to best leader and follow him around if possible.
	virtual void OnTakeAction();

	//Reset path stuff
	virtual void OnQuit();
};
typedef TPtr<CAI_Action_Follow> spCAI_Action_Follow;

class CAI_Action_Hold : public CAI_Action
{
protected:
	//Position to hold at and direction to look in (tilt, pitch, heading)
	CVec3Dfp32 m_Destination;
	CVec3Dfp32 m_LookDir;
	bool m_bWalkStop;

	//Is position path position?
	bool m_bPathPos;

	//Next tick to update pos/look stuff
	int m_UpdateTick;
	int8 m_Stance;	// What stance we should hold in (default is -1 ie ignore)
	int8 m_OrgStance;	// The stance we held before taken

	//Flags
	enum {
		FLAGS_NOPOS = 0x1, //Hold position is never set, so we always just stand where we are

		//Non-parameter flags:
		FLAGS_ATPOS	= 0x2,	//Set when we consider ourselves to be at our hold position
	};
	int8 m_Flags;
	static const char * ms_lStrFlags[];
	
public:
	CAI_Action_Hold(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_IDLE);

	//Parameters
	enum {
		PARAM_POS = PARAM_BASE,			//The position to hold at
		PARAM_POSOBJECT,				//Use position and direction of object to set the above two params
		PARAM_FLAGS,					//Flags, see above

		PARAM_BASE_HOLD,
	};

	virtual	void SetActionOverride(int _iBehaviour, CStr _sParamName, fp32 _DistParam, fp32 _MaxDistParam, int _iActivator);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, const CVec3Dfp32& _Val);
	virtual void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Get flags for given string
	virtual int StrToFlags(CStr _FlagsStr);

	//Mandatory: Move, Turn
	virtual int MandatoryDevices();

	//Holding at current location is a pretty neutral action, slightly safe and of course lazy 
	virtual CAI_ActionEstimate GetEstimate();

	//Stay put, or go to position and stay put.
	virtual void OnTakeAction();

	// Deactivate animgraph
	// Not needed as we inherit from CAI_Action_Idle
	// virtual void OnStart();

	virtual void OnStart();
	//Reset path stuff and reactivate animgraph
	virtual void OnQuit();

	//Save game stuff
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};
typedef TPtr<CAI_Action_Hold> spCAI_Action_Hold;

// CAI_Action_Restrict restricts the character a certain radius from a position
// or inside a certain area (brush). Whenever it finds itself outside it will move
// to get inside the area. Set its prio to PRIO_FORCE to override even combat or
// to its default PRIO_IDLE to restrict random roaming, patroling
class CAI_Action_Restrict : public CAI_Action
{
protected:

	//Position to hold at and direction to look in (tilt, pitch, heading)
	CVec3Dfp32		m_RestrictPos;			// Return pos when restricted
	CVec3Dfp32		m_PathPos;				// Pathfinder friendly version of m_RestrictPos
	fp32			m_MaxRange;				// Max range to m_RestrictPos
	int				m_iObject;				// Object to retrieve m_RestrictPos from
	CWObject_Room*	m_pRoom;				// Must stay inside room
	bool			m_bIsRestricted;		// Keep moving when m_bTaken is set
	bool			m_bActive;				// Restict when active

public:
	// *** Quick'N'dirty kludge to get around the fact that CAI_Action_Restrict is created
	// way before any other actions and actually not part of the action system ***
	CStr		m_sObjName;				// Temp storage of object name
	CAI_Action_Restrict(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_FORCED);

	//Parameters
	enum {
		PARAM_POS = PARAM_BASE,	//The position to hold at
		PARAM_RANGE,			//The direction to look in when holding
		PARAM_OBJECT,			//Name of object to look up its m_iObject
		PARAM_ACTIVE,			//State of activation flag
		PARAM_MULTI,			//Try to match param with an object name
								//If that fails convert to number and positive means range, 0 means off, 1 means on
		PARAM_BASE_RESTRICT,
	};

	void SetActionOverride(int _iBehaviour, CStr _sParamName, fp32 _DistParam, fp32 _MaxDistParam, int _iActivator);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, const CVec3Dfp32& _Val);
	virtual void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Move
	virtual int MandatoryDevices();

	virtual bool IsValid();
	CWObject_Room* GetRoom();
	bool IsRestricted(const CVec3Dfp32& _Pos, bool _bActivate = false);
	virtual CAI_ActionEstimate GetEstimate();

	//Move back inside restriction area if needed
	virtual void OnTakeAction();
	// Taunt if enemy in sight
	virtual void OnStart();
	//Restet the m_bTaken flag
	virtual void OnQuit();

	//Save game stuff
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};
typedef TPtr<CAI_Action_Restrict> spCAI_Action_Restrict;

class CAI_Action_PatrolPath : public CAI_Action
{
protected:
	//Path to patrol
	int m_iPath;

	//Are we away from path?
	bool m_bReturningToPath;

	//Pathfinding position of path
	CVec3Dfp32 m_PathPosition;

	//Are we at a stop?
	bool m_bStop;

	int8 m_Stance;	// What stance we should hold in (default is -1 ie ignore)
	int8 m_OrgStance;	// The stance we held before taken

public:
	CAI_Action_PatrolPath(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_IDLE);

	//Parameters
	enum {
		PARAM_PATH = PARAM_BASE,	//The path to patrol

		PARAM_BASE_PATROL,
	};
	virtual	void SetParameter(int _Param, int _Val);
	virtual void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Move, Turn
	virtual int MandatoryDevices();

	//Valid if there's a path
	virtual bool IsValid();

	//Good exploratory, otherwise pretty neutral.
	virtual CAI_ActionEstimate GetEstimate();

	//Move along a path, looking in the direction of the path if possible
	virtual void OnTakeAction();

	virtual void OnStart();

	//Reset path stuff
	virtual void OnQuit();

	//Save game stuff
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
};
typedef TPtr<CAI_Action_PatrolPath> spCAI_Action_PatrolPath;

class CAI_Action_Roam : public CAI_Action
{
public:
	CWO_ScenePoint* m_pScenePoint;		// Current scenepoint
protected:
	//Current destination
	CVec3Dfp32	m_Destination;	// Current destination
	int			m_StayTimeout;	// When we should expire after reaching our goal
	
	//Current destination scenepoint
	bool m_bWalkStop;			// When true, check walkstops when appropriate and set to false
	

	//Flags
	enum {
		FLAGS_PATHFINDING	= 1,	// Allow pathfinding
		FLAGS_RUN			= 2,	// Run instead of walk
		FLAGS_SCENEPOINTS	= 4,	// Allow the use of scenepoints
		FLAGS_RANDOM		= 8,	// Allow random points non SP points, possibly after no SP was found
		FLAGS_CLOSEST_PT	= 16,	// Choose the closest SP instead of a random	
		FLAGS_NEARPLAYER	= 32,	// Prefer SPs near player, also choose random pts near player
		FLAGS_IN_PLAYERFOV	= 64,	// Prefer SPs in player FOV (not neccesarily visible to him though)
		FLAGS_PREFER_HEADING = 128,	// Prefer SPs in forward 180 degree arc
		FLAGS_AVOID_LIGHT	= 256,	// Avoid bright SPs
		FLAGS_PREFER_LIGHT	= 512,	// Prefer bright SPs
		FLAGS_DYNAMIC		= 1024,	// Include Dynamic SPs
		FLAGS_JUMP			= 2048,	// Jump SPs
		//Non-parameter flags
		FLAGS_ATPOS			= 2048,
	};
	int m_Flags;
	static const char* ms_lStrFlags[];

	// Members for handling jumping movement
	bool m_bAirborne;			// AI is jumping and gravity etc is turned off

	CVec3Dfp32 m_JumpVelocity;	// The speed we jump with (set up at jump, modified by gravity)
	int m_JumpDurationTicks;	// Total nbr of ticks for this jump
	int m_JumpTicks;			// Number of ticks done in this jump


public:
	CAI_Action_Roam(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_IDLE);

	enum {
		PARAM_FLAGS = PARAM_BASE,	//Flags
		PARAM_MAXRANGE,				// Maxrange within which we look for suitable positions.
		PARAM_MINRANGE,				// Minrange for suitable positions
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual void SetParameter(int _Param, CStr _Val);
	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	//Get flags for given string
	int StrToFlags(CStr _FlagsStr);

	// Valid as usual AND tension isn't too high
	virtual bool IsValid();

	//Mandatory: Move, Turn
	virtual int MandatoryDevices();

	//Very good exploratory, somewhat unsafe
	virtual CAI_ActionEstimate GetEstimate();

	//Move around, preferrably to unknown areas.
	bool FindScenePointDestination();
	bool FindRandomDestination();
	bool FindPositionInPlayerFOV();
	bool FindDestination();
	bool MoveToDestination();
	bool StayAtDestination();
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
	virtual void RequestScenepoints();
};
typedef TPtr<CAI_Action_Roam> spCAI_Action_Roam;

//Action to look continuously at current look object
class CAI_Action_Look : public CAI_Action
{
protected:
	//Tracking time in ticks
	int16	m_TrackingTime;
	int		m_StayTimeout;			// Tick when we should no longer stay at the spot but continue the search
	int		m_LookHumanTimeout;		// Tick when we should no longer look at that human
	int		m_iLookHumanTarget;		// Who are we looking at?
public:
	CWO_ScenePoint* m_pScenePoint;		// Current active scenepoint, public so we can debugdraw it from outside

public:
	CAI_Action_Look(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_IDLE);

	enum {
		PARAM_TRACKINGTIME = PARAM_BASE,

		PARAM_BASE_LOOK,
	};

	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Look
	virtual int MandatoryDevices();

	//Valid if there is a look at object
	virtual bool IsValid();

	//Always good when valid
	virtual CAI_ActionEstimate GetEstimate();

	// Returns true if we have a valid look SP or if we find one
	// Actually finding one will only be called sporadically
	// Returns true if we have one or found a new one
	bool FindLookScenepoint(bool _bSearch = false);
	// Handles looking at a particular SP
	bool LookAtScenepoint();
	// Both finds and looks at another person
	bool FindLookHuman(bool _bSearch = false);
	//Track look at object continously
	virtual void OnTakeAction();
	virtual bool OnServiceBehaviour();
	virtual bool AnimationEvent(int _iUser, int _iEvent);
	virtual void RequestScenepoints();
	//Not overridable
	virtual bool IsOverridableByActionOverride();
};
typedef TPtr<CAI_Action_Look> spCAI_Action_Look;

//Alert priority actions///////////////////////////////////////////////////////////////////////////////
#define kCAI_Action_Scan_PatienceMultiplier	6
class CAI_Action_Scan : public CAI_Action
{
protected:
	int8 m_MinAwareness;
	int8 m_MaxAwareness;

	//Min alertness level for this action
	int8 m_AlertnessThreshold;
	fp32 m_Propability;				// Propability the action will be taken whe conditions are right(1.0f)
	int32 m_Interval;				// Min interval between take considerations (0)
	int m_LastIntervalTick;			// Last time we tested for propability
	bool m_bSecThreatOnly;			// Wether security threats only should be considered (true)

	//Current phenomenon position
    CVec3Dfp32 m_Pos;

	//The tick we stop focusing on the phenomenon position
	int m_AlertTick;
	fp32 m_MaxRange;
public:
	CAI_Action_Scan(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_ALERT);

	//Parameters
	enum {
		PARAM_AWARENESS = PARAM_BASE,	//The min awareness to bother with
		PARAM_ALERTNESS,				//The min alertness to be valid
		PARAM_PROPABILITY,				//Chance the action will be taken
		PARAM_INTERVAL,					//Interval between prop checks in ticks
		PARAM_SECONLY,					//Check sec threats only when true
		PARAM_RANGE,
		PARAM_BASE_SCAN,
	};
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Look
	virtual int MandatoryDevices();

	//Valid when there's a phenomenon to scan for
	virtual bool IsValid();

	//Scanning is only somewhat exploratory, but relatively safe and lazy
	virtual CAI_ActionEstimate GetEstimate();

	//Turn to look at best phenomenon, then scan randomly in that general direction
	virtual void OnTakeAction();

	//Reset phenomenon position
	virtual void OnStart();
	virtual void OnQuit();
};
typedef TPtr<CAI_Action_Scan> spCAI_Action_Scan;


class CAI_Action_EscalateOdd : public CAI_Action
{
protected:
	//Flags
	enum {
		FLAGS_CROUCH = 1,		// Player is crouching
		FLAGS_JUMP = 2,			// Player is jumping
		FLAGS_LEDGE = 4,		// Player is ledge climbing
		FLAGS_RUN = 8,			// Player is running
		FLAGS_MELEE = 16,		// Player is fisting/Shiving
		FLAGS_SHOOT = 32,		// Player is shooting
		
		NUM_FLAGS = 7,
	};

	int8 m_Flags;
	static const char* ms_lStrFlags[NUM_FLAGS];
	int32 m_Interval;					// Min interval between triggering
	int m_NextCheck;
	fp32 m_MaxRangeSqr;
	fp32 m_ApproachMinSqr;
	fp32 m_ApproachMaxSqr;

public:
	CAI_Action_EscalateOdd(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_ALERT);
	//Parameters
	enum {
		PARAM_FLAGS = PARAM_BASE,	//The min awareness to bother with
		PARAM_RANGE,
		PARAM_INTERVAL,
		PARAM_APPROACHMIN,
		PARAM_APPROACHMAX,

		PARAM_BASE_OBSERVE,
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual void SetParameter(int _Param, CStr _Val);
	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	//Get flags for given string
	int StrToFlags(CStr _FlagsStr);

	//Mandatory: Look
	virtual int MandatoryDevices();

	// Can we see the player
	virtual bool IsValid();

	// Returns an estimate if we the player is SPOTTED
	virtual CAI_ActionEstimate GetEstimate();
	// Returns true if pAgent is spotted and acts oddly
	bool IsActingOdd(CAI_AgentInfo* pAgent);

	// Query player according to all flags
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
};
typedef TPtr<CAI_Action_EscalateOdd> spCAI_Action_EscalateOdd;

class CAI_Action_EscalateThreat : public CAI_Action
{
protected:
	int m_iTarget;					// Index of our target (to generalize beyond the player)
	int32 m_Interval;					// Min interval between triggering
	int m_StartEscalate;			// Timer for last escalate
	fp32 m_MaxRangeSqr;
	fp32 m_ApproachMinSqr;
	fp32 m_ApproachMaxSqr;

public:
	CAI_Action_EscalateThreat(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_ALERT);
	//Parameters
	enum {
		PARAM_RANGE,
		PARAM_INTERVAL,
		PARAM_APPROACHMIN,
		PARAM_APPROACHMAX,

		PARAM_BASE_OBSERVE,
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual void SetParameter(int _Param, CStr _Val);
	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Look
	virtual int MandatoryDevices();

	// Can we see the player
	virtual bool IsValid();

	// Returns an estimate if we the player is SPOTTED
	virtual CAI_ActionEstimate GetEstimate();

	// Query player according to all flags
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
};
typedef TPtr<CAI_Action_EscalateThreat> spCAI_Action_EscalateThreat;

class CAI_Action_Investigate : public CAI_Action
{
protected:
	CVec3Dfp32 m_Destination;

	//Have we visited the original investigation spot and should start roaming?
	bool			m_bLeadSearch;			// True for the lead searcher only
	int				m_iSuspect;				// Who are we searching for?
	CVec3Dfp32		m_SuspectPos;			// Where do we think he/she is?
	CMat4Dfp32		m_StartStayMat;			// Grab starting look when scanning
	int32			m_iBehaviourStand;		// Behaviour to run standing
	int32			m_iBehaviourCrouch;		// Behaviour to run crouched

	// We have a loong memory of visited scenepoints to not look totally dorky
	bool			m_bWalkStop;
	int32			m_StayTimeout;
	CWO_ScenePoint* m_pScenePoint;		// Current scenepoint

	
	int32			m_LookTimeout;
	CWO_ScenePoint* m_pLookPoint;		// Current active LOOK scenepoint

public:
	CAI_Action_Investigate(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_ALERT);
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;

	//Flags
	enum {
		FLAGS_NOTICEPOINTS	= 0x1,	// Allow the use of initial noticepoint
		FLAGS_SCENEPOINTS	= 0x2,	// Allow the use of scenepoints
		FLAGS_DEATHSPOTS	= 0x4,
		FLAGS_RANGEFROMHOLD = 0x8,	// Measure range from holdpos if any
		FLAGS_RUN			= 0x10,

		NUM_FLAGS = 6,
	};

	int32			m_Flags;
	static const char* ms_lStrFlags[NUM_FLAGS];

	enum {
		PARAM_FLAGS = PARAM_BASE,	//Flags
		PARAM_BEHAVIOUR_STAND,
		PARAM_BEHAVIOUR_CROUCH,

		PARAM_BASE_INVESTIGATE,
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, CStr _Str);
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	virtual int StrToFlags(CStr _FlagsStr);

	//Mandatory: Move, Turn, Look
	virtual int MandatoryDevices();

	//Investigating is of course mainly exploratory, but also useful for aggressive agents.
	//It might be dangerous though.
	virtual CAI_ActionEstimate GetEstimate();

	//Valid when there's a phenomenon to investigate for
	virtual bool IsValid();

	void SetInvestigateObject(int32 _iObj);

	bool FindDestination();
	bool MoveToDestination();
	bool StayAtDestination();
	bool HandleLookScenepoints();

	//Go look at best phenomenon. Then roam around in that general area.
	virtual void OnTakeAction();

	//Reset search stuff
	virtual void OnStart();
	virtual void OnQuit();
	virtual void RequestScenepoints();
};
typedef TPtr<CAI_Action_Investigate> spCAI_Action_Investigate;

//Danger priority actions///////////////////////////////////////////////////////////////////////////////
class CAI_Action_KeepDistance : public CAI_Action
{
protected:
	fp32 m_MaxRange;
	fp32 m_Distance;
	int m_iBehaviour;
	fp32 m_BehaviourRange;
	bool m_bCrouch;
	int m_MoveMode;
	int m_iTarget;
	CVec3Dfp32 m_TargetPos;	// Last pos we evaded from, will only move when theis pos changes substantially
	CVec3Dfp32 m_Destination;// The position we are moving towards
	CWO_ScenePoint* m_pScenePoint;

public:
	CAI_Action_KeepDistance(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_DANGER);

	// Movemodes
	enum {
		MODE_DEFAULT = 0,	// Walk when close, run when far
		MODE_WALK = 1,		// Always walk
		MODE_RUN = 2,		// Always run
	};

	//Parameters
	enum {
		PARAM_MAXRANGE = PARAM_BASE,		//We move away when threat is within this range
		PARAM_DISTANCE,
		PARAM_BEHAVIOUR,					//Behaviour nbr to play when in gun FOV
		PARAM_BEHAVIOURRANGE,				//Maxrange for gun FOV behaviour
		PARAM_MOVEMODE,
		PARAM_FLAGS,

		PARAM_KEEPDISTANCE_BASE,
	};
	uint16 m_Flags;
	enum {
		FLAG_HOSTILE	= 1,
		FLAG_ENEMY		= 2,
		FLAG_LOOK		= 4,
		FLAG_CORNERED	= 8,
		FLAG_GUN		= 16,	// Turn Hostile when target wields his gun
		FLAG_PLAYER		= 32,
		FLAG_JUMP		= 64,
		FLAG_CROUCH		= 128,	// Crouch when cornered

		NUM_FLAGS = 9,
	};
	static const char* ms_lStrFlags[NUM_FLAGS];

	int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	int StrToFlags(CStr _FlagsStr);

	//Mandatory: Move
	virtual int MandatoryDevices();

	//Valid if we've got position and distance restrictions
	virtual bool IsValid();

	//Always good (since we should have no choice in the matter), but better the closer we get to maxdistance
	virtual CAI_ActionEstimate GetEstimate();
	//bool FindScenepointDestination(CAI_AgentInfo* _pTarget);
	bool FindDestination(CAI_AgentInfo* _pTarget);
	bool MoveToDestination(CAI_AgentInfo* _pTarget);
	bool StayAtDestination(CAI_AgentInfo* _pTarget);
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
	virtual void RequestScenepoints();
};
typedef TPtr<CAI_Action_KeepDistance> spCAI_Action_KeepDistance;

class CAI_Action_Watch : public CAI_Action
{
protected:
	fp32 m_MaxRange;

	int m_iTarget;

public:
	CAI_Action_Watch(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_DANGER);

	//Parameters
	enum {
		PARAM_RANGE = PARAM_BASE,

		PARAM_BASE_WATCH,
	};
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Look; Optional: -
	virtual int MandatoryDevices();

	//Valid when there's a hostile within sight
	virtual bool IsValid();
	virtual int GetPriority(bool _bTest = false);

	//Watching is good defensively and somewhat lazy
	virtual CAI_ActionEstimate GetEstimate();

	//Try to keep hostile within sight
	int FindTargetToWatch();
	virtual void OnTakeAction();
	virtual void OnQuit();
	virtual bool IsOverridableByActionOverride();
};
typedef TPtr<CAI_Action_Watch> spCAI_Action_Watch;

class CAI_Action_Threaten : public CAI_Action_Watch
{
protected:
	int m_OrgStance;
	int m_LastRaiseTimer;

public:
	CAI_Action_Threaten(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_DANGER);

	//Mandatory: Move, Turn, Look, Weapon
	virtual int MandatoryDevices();

	//Threaten is pretty offensive and might be dangerous
	virtual CAI_ActionEstimate GetEstimate();

	//Use appropriate forceful action and/or sound to make hostile stay away. 
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
};
typedef TPtr<CAI_Action_Threaten> spCAI_Action_Threaten;

class CAI_Action_Warn : public CAI_Action_Watch
{
protected:
	int m_OrgStance;
	bool m_bMeleeBlowTarget;

public:
	CAI_Action_Warn(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_DANGER);

	//Mandatory: Move, Turn, Look
	virtual int MandatoryDevices();

	//Warnings are a bit offensive and might be somewhat dangerous
	virtual CAI_ActionEstimate GetEstimate();

	//Use appropriate gestures and/or sound to warn hostiles to stay away. 
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
};
typedef TPtr<CAI_Action_Warn> spCAI_Action_Warn;


//Combat priority actions///////////////////////////////////////////////////////////////////////////////
//CAI_Action_Flee///////////////////////////////////////////////////////////////////////////////
//Bot either runs away in fear or starts a behaviour when the action is triggered
//The action can be triggered in several ways:
//1 When m_MinAwareness+ bot with relation within m_MinRelation,m_MaxRelation is within m_MaxRange
//1 When melee fighting bot is
class CAI_Action_Flee : public CAI_Action
{
protected:
	fp32 m_MaxRange;
	fp32 m_BehaviourRange;	// Range within which we run our behaviour

	int m_iTarget;
	int m_iBehaviour;		// Behaviour nbr to play when triggered, default 0
	bool m_bCrouch;

	int m_NextBehaviourTimer;
	bool m_bUseScenePoints;
	bool m_bDarkness;		// Avoid darkness player
	uint8 m_MoveMode;

	CVec3Dfp32		m_Destination;		// Where we should go.
	CVec3Dfp32		m_HostilePosAtDecision;	// Where hostile was when we decided to go to m_Destination
	int				m_StayTimeout;		// Tick when we should no longer stay at the spot but expire
	CWO_ScenePoint* m_pScenePoint;		// Current active scenepoint

public:
	CAI_Action_Flee(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;

	enum {
		MODE_DEFAULT = 0,
		MODE_WALK = 1,
		MODE_RUN = 2,
	};

	//Parameters
	enum {
		PARAM_RANGE = PARAM_BASE,
		PARAM_BEHAVIOUR_RANGE,
		PARAM_BEHAVIOUR,
		PARAM_SCENEPOINTS,
		PARAM_DARKNESS,
		PARAM_BASE_NEAR,
		PARAM_MOVEMODE,

		PARAM_BASE_FLEE,
	};
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual void SetParameter(int _Param, CStr _Val);

	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Move
	virtual int MandatoryDevices();

	//Valid when there's enemy about
	virtual bool IsValid();

	//Avoid is usually a good action for cowards when there's enemy around
	virtual CAI_ActionEstimate GetEstimate();

	bool FindScenePointDestination(const CVec3Dfp32& _TargetPos);

	//Flee from enemy, and towards friends if possible
	virtual void OnTakeAction();
	bool AvoidThreat(CAI_AgentInfo* pHostile,fp32 _MaxRange,fp32 _RetreatRange,bool _bDuckWhenCornered);
	bool HandleCoverpoint(CAI_AgentInfo* pHostile,bool _bDuckWhenClose);
	bool DuckForGun();

	virtual void OnStart();
	virtual void OnQuit();
	virtual void RequestScenepoints();
};
typedef TPtr<CAI_Action_Flee> spCAI_Action_Flee;


//Forced priority actions///////////////////////////////////////////////////////////////////////////////

//Combat actions///////////////////////////////////////////////////////////////////////////////////////////

class CAI_Action_WeaponAim : public CAI_Action
{
public:
	CAI_Action_WeaponAim(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);
	
	int32 m_iObject;
	int32 m_iTarget;
	fp32 m_CosAngle;
	int m_Tracktime;
	fp32 m_MinRange;
	fp32 m_MaxRange;

	//Parameters
	enum {
		PARAM_ANGLE = PARAM_BASE,		// Max angle
		PARAM_TRACKTIME,				// Time when aiming takes place
		PARAM_MINRANGE,
		PARAM_MAXRANGE,
		PARAM_OBJECT,					// Object name

		PARAM_BASE_AIM,
	};
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, CStr _Str);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Weapon, Look
	virtual int MandatoryDevices();

	//Valid if weapon is available and we've got a good target
	virtual bool IsValid();

	//Good if current aim is bad
	virtual CAI_ActionEstimate GetEstimate();

	//Try to improve aim
	virtual void OnTakeAction();
};
typedef TPtr<CAI_Action_WeaponAim> spCAI_Action_WeaponAim;


class CAI_Action_WeaponAttackRanged : public CAI_Action
{
protected:
	int				m_iTarget;			// Target index as set by GetEstimate
	fp32			m_CosAngle;
	int				m_Tracktime;
	int				m_iNearbyFriendly;	// Nearest friendly teammate
	TArray<uint16>	m_liGestures;		// List of shoot gestures to run

	uint16 m_Flags;
	enum {
		FLAGS_STOP					= M_Bit(0),	// Stop movement when firing
		FLAGS_RELOAD				= M_Bit(1),	// Check reloads
		FLAGS_FRIENDLYFIRE			= M_Bit(2),	// Don't shoot when friendlies are in LOS
		FLAGS_DARKLING				= M_Bit(3),	// Keep stance upright, fire wildly!

		NUM_FLAGS = 5,							// One more to give room for NULL
	};

	static const char* ms_lStrFlags[NUM_FLAGS];

	//Parameters
	enum {
		PARAM_ANGLE = PARAM_BASE,		// Max angle
		PARAM_TRACKTIME,				// Time to track target
		PARAM_GESTURES,
		PARAM_FLAGS,

		PARAM_BASE_RANGED,
	};
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, CStr _Str);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	int StrToFlags(CStr _FlagsStr);

public:
	CAI_Action_WeaponAttackRanged(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);

	// Is valid does NOT require the target to be visible - we want to bang away a bit
	virtual bool IsValid();
	virtual int MandatoryDevices();
	virtual int GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const;

	//Estimate improves with estimated chance of hitting, as well as power of weapon
	virtual CAI_ActionEstimate GetEstimate();
	bool CheckFriendlyFire();	// Returns true if it is OK to fire
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
};

typedef TPtr<CAI_Action_WeaponAttackRanged> spCAI_Action_WeaponAttackRanged;

class CAI_Action_MeleeFight : public CAI_Action
{
	friend class CAI_Core;
public:
	CAI_Action_MeleeFight(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);

	int m_iOpponent;				// Who are we fighting?

	CVec3Dfp32 m_StartPos;			// Pos we begun the meleefight at
	fp32 m_MeleeRestrictSqr;			// Max range sqr from m_StartPos, if 0, no moves allowed, if neg no limit imposed
	bool m_bRetreating;				// When this is true we cannot expire until we get inside m_MeleeRestrictSqr

	fp32 m_CosAngle;					//Cos of max deviation allowed for an attack, otherwise we turn/strafe
	fp32 m_AttackPropability;		//Propability of attack within sweatspot,
	int32 m_ManeuverInterval;		// Interval in serverticks between maneuvers
	int32 m_LastManeuverTick;		// Timestamp of last attack
	fp32 m_MovePropability;			//Propability of move wether we're at the sweetspot or not
	int32 m_LastManeuver;			// Manuver used previous tick
	int32 m_LastOpponentManeuver;	// Maneuver used previous tick
	int32 m_OpponentManeuverTemp;	// We must store the prev manuver so we can set m_LastOpponentManeuver when it is 0
	bool m_bFirstBlow;				// true until our first attack/block

	uint16 m_Flags;
	enum {
		FLAGS_FWD					= M_Bit(0),	// Fwd movement allowed
		FLAGS_BWD					= M_Bit(1),	// Bwd movement allowed
		FLAGS_LEFT					= M_Bit(2),	// Left movement
		FLAGS_RIGHT					= M_Bit(3),	// Right movement
		FLAGS_ATTACK				= M_Bit(4),	// Regular attacks allowed (left,right,middle allowed)
		FLAGS_ATTACK_SPECIAL		= M_Bit(5),	// Slash and a kick
		FLAGS_BLOCK					= M_Bit(6),	// Block (not used)
		FLAGS_PATHFIND				= M_Bit(7),	// Use pathfinding for moves/attacks
		FLAGS_TRACE					= M_Bit(8),	// Trace to target for legit attacks (expensive)
		FLAGS_GENTLEMAN				= M_Bit(9),	// Only one can fight the player
		FLAGS_PLAYERONLY			= M_Bit(10),	// Only fight player, still subject to HOSTILE etc rules
		FLAGS_HOSTILE				= M_Bit(11),	// Fight HOSTILE+ targets (instead of ENEMY+)
		
		NUM_FLAGS = 13,
	};
	static const char* ms_lStrFlags[NUM_FLAGS];

	//Parameters
	enum {
		PARAM_RESTRICTRANGE = PARAM_BASE,
		PARAM_ANGLE,
		PARAM_ATTACKPROP,
		PARAM_INTERVAL,
		PARAM_MOVEPROP,
		PARAM_FLAGS,

		PARAM_BASE_MELEEFIGHT,
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, CStr _Str);
	virtual	void SetParameter(int _Param, int _Val);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	int StrToFlags(CStr _FlagsStr);

	//Mandatory: Weapon, Move
	virtual int MandatoryDevices();

	// We must have a weapon and be within maxrange
	virtual bool IsValid();
	// virtual bool IsValid(CAI_AgentInfo* _pTarget);

	// Estimate is 0 beyond m_MaxRangeSqr
	// Estimate is 75 between m_MinRangeSqr and m_HitRangeSqr
	// Estimate is 50 within m_MinRangeSqr
	virtual CAI_ActionEstimate GetEstimate();
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;

	virtual void OnStart();
	virtual void OnTakeAction();
	void OnQuit();
};
typedef TPtr<CAI_Action_MeleeFight> spCAI_Action_MeleeFight;


// While this action is taken only actions with typeflags TYPE_OFFENSIVE, TYPE_DEFENSIVE
class CAI_Action_Close : public CAI_Action
{
public:
	CAI_Action_Close(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);


	int32 m_iTarget;			// Our current enemy
	CVec3Dfp32 m_Destination;	// Pos we move towards
	fp32 m_MinRangeDetect;		// The minimum range for DETECTED targets (not spotted that is)
	int32 m_StayTimeout;		// When to expire due to staying at the same spot too long
	uint8 m_MoveMode;
	CVec3Dfp32 m_ScanPos;	// A position we track when we don't know where the target is
	enum {
		MODE_DEFAULT = 0,
		MODE_WALK = 1,
		MODE_RUN = 2,
	};

	//Parameters
	enum {
		PARAM_MINRANGE_DETECT = PARAM_BASE,
		PARAM_FLAGS,
		PARAM_MOVEMODE,

		PARAM_BASE_CLOSE,
	};

	//Flags
	enum {
		FLAGS_GENTLEMAN		= M_Bit(0),	// When true only one character may use the action at a time!
		FLAGS_STOPONDETECT	= M_Bit(1),	// Close stops if target has not been SPOTTED for Patience ticks
		FLAGS_STOPONSPOT	= M_Bit(2),	// Close stops if is SPOTTED

		NUM_FLAGS = 4,				// One more than the above to make room for the NULL
	};
	static const char* ms_lStrFlags[NUM_FLAGS];
	int16 m_Flags;

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, CStr _Str);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	virtual int StrToFlags(CStr _FlagsStr);

	//Mandatory: Weapon
	virtual int MandatoryDevices();

	//Valid if at least one action is of TYPE_OFFENSIVE
	virtual bool IsValid();
	virtual bool IsValid(CAI_AgentInfo* _pTarget);

	//Good if weapon is useful in current situation, and we're not currently wielding another weapon that is useful.
	virtual CAI_ActionEstimate GetEstimate();

	//Switch weapons until we wield this weapon
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
};
typedef TPtr<CAI_Action_Close> spCAI_Action_Close;

// General combat maneuver action
// It basically moves the bot around between various locations while tracking the enemy
class CAI_Action_Combat : public CAI_Action
{
public:
	CAI_Action_Combat(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);

	int32 m_iTarget;			// Our current enemy
	int32 m_Duration;			// Time at position when not using a scenepoint
	int32 m_NextRetreatSPTimer;	// When was the last timer value we 
	int8 m_MinAwareness;		// Minimum awareness to be valid, default (DETECTED)

	enum {
		MODE_DEFAULT = 0,	// Walk when we don't see the target, run when we see
		MODE_WALK = 1,		// Always walk
		MODE_RUN = 2,		// Always run
	};
	uint8 m_MoveMode;

	//Flags
	enum {
		FLAGS_SCENEPOINTS		= 1,	// Allow the use of scenepoints
		FLAGS_RANDOM			= 2,	// Allow random points, possibly if no scenepoint was found
		FLAGS_CLOSEST_PT		= 4,	// Closest point will be used instead of a random range point
		FLAGS_AVOID_PLAYERAIM	= 8,	// Expire scenepoint when player is looking straight at us
		FLAGS_IN_PLAYERFOV		= 16,	// Prefer SPs in player FOV (not neccesarily visible to him though)
		FLAGS_PREFER_HEADING	= 32,	// Prefer SPs in forward 180 degree arc
		FLAGS_AVOID_LIGHT		= 64,	// Avoid bright SPs
		FLAGS_PREFER_LIGHT		= 128,	// Prefer bright SPs
		FLAGS_DYNAMIC			= 256,	// Consider dynamic scenepoints
		FLAGS_RETREAT			= 512,	// When no SP is available, we can use SPs were our cur pos is in the arc

		NUM_FLAGS = 11,	// One more than the above to make room for the NULL
	};
	static const char* ms_lStrFlags[NUM_FLAGS];
	int16 m_Flags;

	bool			m_bCrouch;			// Did we crouch?
	CVec3Dfp32		m_Destination;		// Where we should go.
	bool			m_bWalkStop;
	int				m_StayTimeout;		// Tick when we should no longer stay at the spot but expire
	CWO_ScenePoint* m_pScenePoint;		// Current active scenepoint

	//Parameters
	enum {
		PARAM_MINRANGE = PARAM_BASE,
		PARAM_MAXRANGE,
		PARAM_MINRANGE_TARGET,
		PARAM_MAXRANGE_TARGET,
		PARAM_MOVEMODE,
		PARAM_DURATION,
		PARAM_FLAGS,
		PARAM_MINAWARENESS,

		PARAM_BASE_COMBAT,
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, CStr _Str);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	virtual int StrToFlags(CStr _FlagsStr);

	//Mandatory: Weapon
	virtual int MandatoryDevices();

	virtual bool IsValid();

	//Good if weapon is useful in current situation, and we're not currently wielding another weapon that is useful.
	virtual CAI_ActionEstimate GetEstimate();

	bool FindScenePointDestination();
	bool FindRandomDestination();

	bool FindDestination();
	bool MoveToDestination();
	bool StayAtDestination();
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
	virtual void RequestScenepoints();
};
typedef TPtr<CAI_Action_Combat> spCAI_Action_Combat;

// General cover action
// Hopefully takes us out of sight/range with the enemy after being shot at etc
class CAI_Action_Cover : public CAI_Action
{
public:
	CAI_Action_Cover(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;

	int m_Duration;			// Time at position when not using a scenepoint
	int	m_iAttacker;		// Who fired at us?
	bool m_bTaken;			// True after first been taken
	bool m_bCrouch;
	uint8 m_MoveMode;
	enum {
		MODE_DEFAULT = 0,	// Walk when we don't see the target, run when we see
		MODE_WALK = 1,		// Always walk
		MODE_RUN = 2,		// Always run
	};
	//Flags
	uint8 m_Flags;
	enum {
		FLAGS_SCENEPOINTS	= 1,	// Allow the use of scenepoints
		FLAGS_CLOSEST_PT	= 2,	// Closest point will be used instead of random
		FLAGS_CROUCH		= 4,	// Allow crouching when no valid scenepoint was found
		FLAGS_LEFT			= 8,	// Allow left dodge when no valid scenepoint was found
		FLAGS_RIGHT			= 16,	// Allow right dodge when no valid scenepoint was found
		FLAGS_DYNAMIC		= 32,	// 
		NUM_FLAGS = 6,
	};
	static const char* ms_lStrFlags[NUM_FLAGS];

	CVec3Dfp32		m_Destination;		// Where we should go.
	int				m_StayTimeout;		// Tick when we should no longer stay at the spot but expire
	CWO_ScenePoint* m_pScenePoint;		// Current active scenepoint

	//Parameters
	enum {
		PARAM_MINRANGE = PARAM_BASE,
		PARAM_MAXRANGE,
		PARAM_MINRANGE_TARGET,
		PARAM_MAXRANGE_TARGET,
		PARAM_MOVEMODE,
		PARAM_DURATION,
		PARAM_FLAGS,

		PARAM_BASE_COMBAT,
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, CStr _Str);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	virtual int StrToFlags(CStr _FlagsStr);

	//Mandatory: Weapon
	virtual int MandatoryDevices();

	virtual bool IsValid();

	//Good if weapon is useful in current situation, and we're not currently wielding another weapon that is useful.
	virtual CAI_ActionEstimate GetEstimate();

	bool FindScenePointDestination(CVec3Dfp32& _TargetPos);
	bool FindRandomDestination(CVec3Dfp32& _TargetPos);

	//Switch weapons until we wield this weapon
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
	virtual void RequestScenepoints();
};
typedef TPtr<CAI_Action_Cover> spCAI_Action_Cover;

// Users of Combat and Close action will send messages to users of this action telling them what they do
// The action will respond by issuing orders, running behaviours to that effect.
class CAI_Action_CombatLeader : public CAI_Action
{
public:
	CAI_Action_CombatLeader(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;
	virtual	void SetParameter(int _Param, int _Val);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Parameters
	enum {
		PARAM_BEHAVIOURNBR_GENERIC = PARAM_BASE,
		PARAM_BEHAVIOURNBR_CLOSE,
		PARAM_BEHAVIOURNBR_FWD,
		PARAM_BEHAVIOURNBR_FLANK,
		PARAM_BEHAVIOURNBR_REAR,
		PARAM_BEHAVIOURNBR_COVER,

		PARAM_BASE_COMBATLEADER,
	};

	bool ReceiveDecision(int _iCaller,int _iTarget,CVec3Dfp32 _MovePos,int _Type = PARAM_BEHAVIOURNBR_GENERIC);

	int m_iBehaviourClose;
	int m_iBehaviourFwd;
	int m_iBehaviourFlank;
	int m_iBehaviourRear;
	int m_iBehaviourCover;

	// The order to issue, when 0 we expire
	int m_iOrderToIssue;
	int m_iCaller;
	int m_iTarget;
	CVec3Dfp32 m_MovePos;
	
	//Mandatory: Weapon
	virtual int MandatoryDevices();

	virtual bool IsValid();

	//Good if weapon is useful in current situation, and we're not currently wielding another weapon that is useful.
	virtual CAI_ActionEstimate GetEstimate();

	//Switch weapons until we wield this weapon
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
};

// Darkling specific action for attacking
// Note: Darklings and all other ai uses CAI_Core::m_MeleeMaxRange for max range
class CAI_Action_DarklingAttack : public CAI_Action
{
public:
	CAI_Action_DarklingAttack(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);

	int m_iTarget;			// Our current enemy
	bool m_bGentleman;		// When true only one character may use the action at a time!

public:

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, CStr _Str);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Weapon
	virtual int MandatoryDevices();

	//Valid if at least one action is of TYPE_OFFENSIVE
	virtual bool IsValid();
	virtual bool IsValid(CAI_AgentInfo* _pTarget);

	//Good if weapon is useful in current situation, and we're not currently wielding another weapon that is useful.
	virtual CAI_ActionEstimate GetEstimate();

	//Switch weapons until we wield this weapon
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
};
typedef TPtr<CAI_Action_DarklingAttack> spCAI_Action_DarklingAttack;

// While this action is taken only actions with typeflags TYPE_OFFENSIVE, TYPE_DEFENSIVE
// The action will behave differently depending on range to target and flags
class CAI_Action_DarklingClose : public CAI_Action
{
public:
	CAI_Action_DarklingClose(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);

	int				m_iTarget;			// Our current enemy
	uint8			m_MoveMode;
	bool			m_bGentleman;		// When true only one character may use the action at a time!
	int32			m_NextScareTimer;	// Next pAI->m_Timer where we can pull our famous scare stunt again

	// m_AttackBehaviour: Attack behaviour nbr
	// m_AttackRange: Range between us and victim when to initiate synchronized behaviour
	// m_AttackRetreat: Required clearance behind victim
	int32 m_AttackBehaviour;
	int32 m_AttackBehaviourLong;		// Use this when the target is the last known enemy
	fp32 m_AttackRange;
	fp32 m_AttackRangeLong;
	
	int32 m_AttackJumpBehaviour;	// Use this when jump attacking someone...
	int32 m_AttackJumpBehaviour2;	// ...or maybe this one.
	
	int32				m_iAttackTargetKilled;			// Set to target id after the kill behaviour started, OnQuit we deal this guy all health plus one
	CStr				m_AnimEventEffect;				// Name of animation effect that responds to animevent 2
	bool				m_bJumping;

	CVec3Dfp32		m_Destination;		// Where we should go.
	int				m_StayTimeout;		// Tick when we should no longer stay at the spot but expire
	CWO_ScenePoint* m_pScenePoint;		// Current active scenepoint

	//Flags
	enum {
		FLAGS_PATHFINDING	= 1,	// Allow pathfinding on walls ***TBD***
		FLAGS_RUN			= 2,	// Run instead of walk
		FLAGS_SCENEPOINTS	= 4,	// Allow the use of scenepoints
		FLAGS_RANDOM		= 8,	// Allow random points non SP points, possibly after no SP was found
		FLAGS_CLOSEST_PT	= 16,	// Choose the closest SP instead of a random	
		FLAGS_NEARPLAYER	= 32,	// Prefer SPs near player
		FLAGS_IN_PLAYERFOV	= 64,	// Prefer SPs in player FOV (not neccesarily visible to him though)
		FLAGS_PREFER_HEADING = 128,	// Prefer SPs in forward 180 degree arc
		FLAGS_AVOID_LIGHT	= 256,	// Avoid bright SPs
		FLAGS_PREFER_LIGHT	= 512,	// Prefer bright SPs
		FLAGS_SYNC			= 1024,	// get offsets from behaviours, synch target behaviour
		FLAGS_SCARE_CIVS	= 2048,	// Scare civilians if this flag is set
	};
	int m_Flags;
	static const char* ms_lStrFlags[];

public:
	// Only checked during GetEstimate, polled at all times though
	enum {
		MODE_DEFAULT = 0,
		MODE_WALK = 1,
		MODE_RUN = 2,
	};

	//Parameters
	enum {
		PARAM_MOVEMODE = PARAM_BASE,
		PARAM_GENTLEMAN,
		PARAM_ATTACK_BEHAVIOUR,
		PARAM_ATTACK_RANGE,
		PARAM_ATTACK_BEHAVIOUR_LONG,
		PARAM_ATTACK_RANGE_LONG,
		PARAM_JUMP_BEHAVIOUR,
		PARAM_JUMP_BEHAVIOUR2,
		PARAM_FLAGS,
		PARAM_EFFECT,

		PARAM_BASE_DARKLINGCLOSE,
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, CStr _Str);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	//Get flags for given string and action type (overload this for action specific flags)
	virtual int StrToFlags(CStr _FlagsStr);

	//Mandatory: Weapon
	virtual int MandatoryDevices();

	//Valid if at least one action is of TYPE_OFFENSIVE
	virtual bool IsValid();

	//Good if weapon is useful in current situation, and we're not currently wielding another weapon that is useful.
	virtual CAI_ActionEstimate GetEstimate();

	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;
	virtual int GetUsedAcs(TArray<int16>& _liAcs) const;
	virtual bool AnimationEvent(int _iUser, int _iEvent);

	// Returns true if m_iTarget is eligible for running m_AttackBehaviour
	bool DoJumpAttackBehaviour(CAI_AgentInfo* _pTarget);
	bool DoJumpLandBehaviour(CAI_AgentInfo* _pTarget);
	bool DoAttackBehaviour(CAI_AgentInfo* _pTarget);
	virtual void OnTakeAction();

	virtual void OnStart();
	virtual void OnQuit();
	virtual void RequestScenepoints();
};
typedef TPtr<CAI_Action_DarklingClose> spCAI_Action_DarklingClose;

class CAI_Action_MeatfaceClose : public CAI_Action
{
public:
	CAI_Action_MeatfaceClose(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);

	int				m_iTarget;					// Our current enemy
	uint8			m_MoveMode;
	bool			m_bGentleman;				// When true only one character may use the action at a time!
	bool			m_bWaitForBomb;				// After a jump or when within range but outside FOV
	int32			m_BombTimer;				// When not -1 we decrement it each tick until zero, then poof!
	int32			m_BombDuration;				// How long until poof?
	int32			m_AttackBehaviour;			// Matched behaviour to play when real close
	fp32			m_AttackRange;				// Close range retrived from behaviour
	CStr			m_AnimEventEffect;			// Name of animation effect that responds to animevent 2
	CVec3Dfp32		m_Destination;				// Where we should go.

	fp32			m_JumpAttackRange;			// When to jump at the player

	fp32			m_StartingHealth;			// Health when action starts, used to determine if we should blow up!

	//Flags
	enum {
		FLAGS_PATHFINDING	= 1,	// Allow pathfinding on walls ***TBD***
		FLAGS_GENTLEMAN		= 2,	// One Meatface at a time can star´t its timer.
		FLAGS_SYNC			= 3,	// Play synchro ACS on the player
		FLAGS_SHOOT			= 8,	// Shoot at player with gesture
		FLAGS_JUMP			= 16,	// Jump towards player
	};
	int m_Flags;
	static const char* ms_lStrFlags[];

public:
	// Only checked during GetEstimate, polled at all times though
	enum {
		MODE_DEFAULT = 0,
		MODE_WALK = 1,
		MODE_RUN = 2,
	};

	//Parameters
	enum {
		PARAM_MOVEMODE = PARAM_BASE,
		PARAM_ATTACK_BEHAVIOUR,
		PARAM_FLAGS,
		PARAM_EFFECT,
		PARAM_BOMBTIME,

		PARAM_BASE_MEATFACECLOSE,
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, CStr _Str);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	//Get flags for given string and action type (overload this for action specific flags)
	virtual int StrToFlags(CStr _FlagsStr);

	//Mandatory: Weapon
	virtual int MandatoryDevices();

	//Valid if at least one action is of TYPE_OFFENSIVE
	virtual bool IsValid();

	//Good if weapon is useful in current situation, and we're not currently wielding another weapon that is useful.
	virtual CAI_ActionEstimate GetEstimate();

	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;
	virtual int GetUsedAcs(TArray<int16>& _liAcs) const;
	virtual int GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const;
	virtual bool AnimationEvent(int _iUser, int _iEvent);

	bool DoAttackBehaviour(CAI_AgentInfo* _pTarget);
	bool DoJumpBehaviour(CAI_AgentInfo* _pTarget);
	bool DoShootBehaviour(CAI_AgentInfo* _pTarget);

	virtual void OnTakeAction();
	virtual bool OnServiceBehaviour();
	virtual void OnStart();
	virtual void OnQuit();
};
typedef TPtr<CAI_Action_MeatfaceClose> spCAI_Action_MeatfaceClose;

class CAI_Action_DarklingJumpClose : public CAI_Action
{
protected:
	//Current destination
	int			m_iTarget;		// Target we're moving towards
	CVec3Dfp32	m_Destination;	// Current destination
	CVec3Dfp32	m_Direction;	// Current direction
	int			m_StayTimeout;	// When we should expire after reaching our goal

	//Current destination scenepoint

	CWO_ScenePoint* m_pScenePoint;		// Current scenepoint

	//Flags
	enum {
		FLAGS_RUN			= 1,	// Run instead of walk
		FLAGS_CLOSEST_PT	= 2,	// Choose the closest SP instead of a random	
		FLAGS_IN_PLAYERFOV	= 4,	// Prefer SPs in player FOV (not neccesarily visible to him though)
		FLAGS_PREFER_HEADING = 8,	// Prefer SPs in forward 180 degree arc
		FLAGS_AVOID_LIGHT	= 16,	// Avoid bright SPs
		FLAGS_PREFER_LIGHT	= 32,	// Prefer bright SPs
		FLAGS_NO_STAY		= 64,	// Don't stay at the SP, jump immideately
		FLAGS_TO_HEAD		= 128,	// Jumps directly onto head is OK (Meatface)
	};
	int m_Flags;
	static const char* ms_lStrFlags[];

public:
	CAI_Action_DarklingJumpClose(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);

	enum {
		PARAM_FLAGS = PARAM_BASE,	//Flags
		PARAM_MAXRANGE,				// Maxrange within which we look for suitable positions.
		PARAM_MINRANGE,				// Minrange for suitable positions
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual void SetParameter(int _Param, CStr _Val);
	//Get parameter ID from sting (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	//Get flags for given string
	int StrToFlags(CStr _FlagsStr);

	//Mandatory: Move, Turn
	virtual int MandatoryDevices();
	virtual bool IsValid();

	//Very good exploratory, somewhat unsafe
	virtual CAI_ActionEstimate GetEstimate();

	//Move around, preferrably to unknown areas.
	bool FindScenePointDestination();
	bool FindDestination();
	bool MoveToDestination();
	bool StayAtDestination();
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
	virtual void RequestScenepoints();
};
typedef TPtr<CAI_Action_DarklingJumpClose> spCAI_Action_DarklingJumpClose;

// Special combat action for enginepath following bots
class CAI_Action_FlyCombat : public CAI_Action
{
public:
	CAI_Action_FlyCombat(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);

	int m_iTarget;			// Our current enemy
	CVec3Dfp32 m_TargetPos;	// Our current target pos

	uint8 m_MoveMode;
	enum {
		MODE_DEFAULT = 0,	// Walk when we don't see the target, run when we see
		MODE_WALK = 1,		// Always walk
		MODE_RUN = 2,		// Always run
	};
	//Flags
	int m_Flags;
	enum {
		FLAGS_CLOSEST_PT	= M_Bit(0),	// Closest point will be used instead of a random range point
		FLAGS_IN_PLAYERFOV	= M_Bit(1),	// Prefer SPs in player FOV (not neccesarily visible to him though)
		FLAGS_PREFER_HEADING = M_Bit(2),	// Prefer SPs in forward 180 degree arc
		FLAGS_AVOID_LIGHT	= M_Bit(3),	// Avoid bright SPs
		FLAGS_PREFER_LIGHT	= M_Bit(4),	// Prefer bright SPs
		FLAGS_ALIGN_2_PATH	= M_Bit(5),	// Align bot to path
		FLAGS_HELI_BANKING	= M_Bit(6),	// Measure current acceleration and apply helicopter style banking
		FLAGS_JUMPS			= M_Bit(7),	// Angelus style jumping/landing when true
		FLAGS_ANIMPATHS		= M_Bit(8),	// Treat enginepaths from scenepoints as animations to play (behaviours)
		FLAGS_BURST			= M_Bit(9),	// Fire bursts when target is visible
		FLAGS_WORLDTRACKING	= M_Bit(10),	// Track in world coordinates

		NUM_FLAGS = 12,	// One more than the above to make room for the NULL
	};
	static const char* ms_lStrFlags[NUM_FLAGS];

	bool			m_bCrouch;			// Dod we crouch?
	CVec3Dfp32		m_Destination;		// Where we should go.
	int				m_StayTimeout;		// Tick when we should no longer stay at the spot but expire
	CWO_ScenePoint* m_pScenePoint;		// Current active scenepoint
	
	int32			m_iPath;			// Follow this enginepath
	int32			m_iAnimation;		// Play this animation

	// These factors determine how we scale the averaged v and a before applying them to up
#define FLY_COMBAT_V_FACTOR 1.0f
#define FLY_COMBAT_A_FACTOR 5.0f
	enum
	{
		POS_COUNT = 20,
	};
	int32			m_iPosHistoryFinger;
	CVec3Dfp32		m_lPosHistory[POS_COUNT];
	enum
	{
		JUMPSTATE_GROUND = 0,
		JUMPSTATE_JUMPING = 1,
		JUMPSTATE_INAIR = 2,
		JUMPSTATE_LANDING = 3,
	};

	//Parameters
	enum {
		PARAM_MOVEMODE = PARAM_BASE,
		PARAM_MOUNT,
		PARAM_FLAGS,

		PARAM_BASE_FLYCOMBAT,
	};

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, CStr _Str);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);
	virtual int StrToFlags(CStr _FlagsStr);
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;

	//Mandatory: Weapon
	virtual int MandatoryDevices();

	virtual bool IsValid();


	//Good if weapon is useful in current situation, and we're not currently wielding another weapon that is useful.
	virtual CAI_ActionEstimate GetEstimate();

	bool FindScenePointDestination();
	// Tries to a find a suitable scenepoint with an enginepath leading from current SP to the next one
	bool FindDestination();
	// Moves the bot along the path, returns TRUE while we're not at the destination scenepoint
	// Returns FALSE when we have finished moving and are at the scenepoint destination
	// (ie we cannot call StayAtDestination until OnFlyFollowPath returns FALSE)
	bool OnFlyFollowPath();
	bool MoveToDestination();
	bool StayAtDestination();
	virtual bool OnServiceBehaviour();
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
	virtual void RequestScenepoints();
};
typedef TPtr<CAI_Action_FlyCombat> spCAI_Action_FlyCombat;

// CAI_Action_AngelusAttack
class CAI_Action_AngelusAttack : public CAI_Action
{
public:
	CAI_Action_AngelusAttack(CAI_ActionHandler * _pAH = NULL, int _Priority = PRIO_COMBAT);

	bool m_bPrimary;			// When true press weapon, when false press item
	int32 m_PrimaryDuration;	// Number of ticks to press primary fire
	int32 m_SecondaryDuration;	// Number of ticks to press secondary fire
	int32 m_PressTicks;			// Nbr of ticks remaining for the current attack
	int32 m_iTarget;			// Our current enemy

	int32 m_FlashBangState;		// ANGELUS_AURASTATE_IDLE		= 0,		// Idle, power is fully charged and ready to use
								// ANGELUS_AURASTATE_RAMPING,				// Charging powers
								// ANGELUS_AURASTATE_RESTING,				// Just used powers, behaving exhausted
								// ANGELUS_AURASTATE_USING

	int32 m_SecondaryState;		// ANGELUS_TENTACLESTATE_IDLE = 0,			// Ready, tentacle in
								// ANGELUS_TENTACLESTATE_HITOBJECT,			// Tentacle hit object, yank it away
								// ANGELUS_TENTACLESTATE_HITTARGET,			// Tentacle hit target, start reeling in
								// ANGELUS_TENTACLESTATE_REELING_IN,		// Reeling in
								// ANGELUS_TENTACLESTATE_EMBRACING,			// Embracing after reeling

public:

	//Parameters
	enum {
		PARAM_PRIMARYDURATION = PARAM_BASE,
		PARAM_SECONDARYDURATION,
	};


	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;

	virtual	void SetParameter(int _Param, fp32 _Val);
	virtual	void SetParameter(int _Param, int _Val);
	virtual	void SetParameter(int _Param, CStr _Str);

	//Get parameter ID from string (used when parsing registry)
	virtual int StrToParam(CStr _Str, int* _pResType = NULL);

	//Mandatory: Weapon
	virtual int MandatoryDevices();

	//Valid if at least one action is of TYPE_OFFENSIVE
	virtual bool IsValid();
	virtual bool IsValid(CAI_AgentInfo* _pTarget);

	virtual CAI_ActionEstimate GetEstimate();
	
	bool SetTentacleState(int32 _State);
	//Switch weapons until we wield this weapon
	virtual void OnTakeAction();
	virtual void OnStart();
	virtual void OnQuit();
};
typedef TPtr<CAI_Action_AngelusAttack> spCAI_Action_AngelusAttack;
#endif
