#ifndef _INC_AI_KNOWLEDGEBASE
#define _INC_AI_KNOWLEDGEBASE

#include "AI_Auxiliary.h"
#include "../WRPG/WRPGDef.h"
class CAI_Core;
class CAI_AgentCriteria;
class CAI_KnowledgeBase;

//Agent information. This info is always tied to a specific AI, representing the 
//knowledge that AI has about the agent.
class CAI_AgentInfo
{
public:
	//Constants

	//Relation regarding this agent 
	enum {
		RELATION_INVALID = -1,
		UNKNOWN = 0,	//We haven't identified the affiliation of this agent yet. Some bot will assume unknowns are hostile, others friendly etc.
		FRIENDLY,		//Agent is allied and should be helped if necessary.
		NEUTRAL,		//Agent won't care about us as long as we don't care about it.
		UNFRIENDLY,		//Agent is potentially hostile
		HOSTILE,		//Agent is not friendly and might attack us.
		HOSTILE_1,		//Higher grades of hostility
		HOSTILE_2,		//...
		ENEMY,			//Agent will attack us on sight, and should be dealt with likewise.
	};
	//String translator for relation (defined at the top of .cpp file)
	static const char* ms_TranslateRelation[];

	//Our awareness level of this agent. The internal order of this enum is important.
	enum {
		AWARENESS_INVALID = -1,
		NONE = 0,	//No awareness whatsoever
		NOTICED,	//Have noticed agent on a sub-concious level. Might investigate if very alert.
		DETECTED,	//Have conciously noticed the agent, but not identified it as an agent yet.
		SPOTTED,	//Know of the presence of the agent
	};
	//String translator for awareness
	static const char * ms_TranslateAwareness[];

	//Info/Validity flags. When adding or removing info flags, make sure the ms_lInfoflagIIs array is properly updated
	enum {
		INFOFLAGS_NONE = 0,
		INFOFLAGS_ALL = 0xffff,

		//Info flags / Validity flags for corresponding info flag 
		IN_LOS					= 0x1,	//Is in LOS
		WILL_COLLIDE			= 0x2,	//This agent is on a collision course *** DEPRECATED ***
		IS_ATTACKING_US			= 0x4,	//This agent is currently attacking us*** DEPRECATED ***
		CAN_ATTACK_US			= 0x8,  //This agent can attack us mow*** DEPRECATED ***
		CAN_ATTACK_US_RANGED	= 0x10,	//This agent can attack us with ranged weapons now*** DEPRECATED ***
		CAN_ATTACK_US_MELEE		= 0x20,	//This agent can attack us in melee now*** DEPRECATED ***
		IN_MELEE				= 0x40,	//This agent is engaged in melee with us*** DEPRECATED ***
		POSITION_IN_LOS			= 0x80, //The position we think agent is at is in LOS
		NUM_INFOFLAGS			= 8,	//This must be kept updated

		//Validity flags for non-infoflag info
		AWARENESS				= 0x100,
		AWARENESS_OF_US			= 0x200,
		RELATION				= 0x400,
		POSITION				= 0x800,
	};

protected:

	//Knowledge base that this agent info belongs to
	CAI_KnowledgeBase* m_pKB;

	//Server index
	int m_iObject;

	//What info is considered to be valid?
	int16 m_ValidFlags;

	//General boolean info about the agent
	int16 m_InfoFlags;
	// m_MeleeTick when the ai last dealt a melee blow
	int32 m_MeleeTick;
	//Our awareness level of this agent. 
	int8 m_Awareness;
	friend class CAI_Core;
	//Since awareness level changes are always delayed by reaction time, we must remember 
	//any new awareness level until such time has passed.
	int8 m_PendingAwareness;
	CVec3Dfp32 m_PendingSuspectedPos;

	//CAI_Core::m_Timer when we last started changing awareness
	int m_AwarenessTimer;
	int m_LastSpottedTimer;	//CAI_Core::m_Timer since last spot (not affected by Touch())

	//CAI_Core::m_Timer when we last changed relation
	int m_RelationTimer;

	//The agent´s presumed awareness of us.
	int8 m_AwarenessOfUs;
	//Relation. How should we treat this agent, and how do we expect it to treat us. See enum above.
	int8 m_Relation;
	int8 m_PendingRelation;

	// Height of target last seen
	fp32 m_SuspectedHeight;
	// m_SuspectedPos is identical to the actual position for DETECTED and SPOTTED targets
	// m_SuspectedPos is set the first time we NOTICED the target and whenever we do something that
	// would make us NOTICED. 
	CVec3Dfp32 m_SuspectedPos;
	CVec3Dfp32 m_SuspectedPosNotGround;

	//Last known path position of agent. 
	CVec3Dfp32 m_PathPos;

	//Have position been updated this frame?
	bool m_bPosUpdate;

	//Knowledgebase is allowed to tamper with the above attributes
	friend class CAI_KnowledgeBase;

protected:
	//Get new awareness of agent based on given "wanted awareness" and own alertness level
	int Alertness_AwarenessMod(int _Awareness);
	//The interval in frames during which awareness of something cannot be raised, 
	//based on alertness level and reaction attribute
	int Alertness_AwarenessChangeDelay(int _Awareness);

public:

	//Constructors
	CAI_AgentInfo(int _iObject = 0, CAI_KnowledgeBase * _pKB = NULL);
	
	//Initializer (you don't have to call this on newly constructed objects, only on invalid ones you want to reinitialize)
	void Init(int _iObject = 0);

	//Refreshes the state of the agent info, and decides when state info becomes invalid
	void OnRefresh();

	//Checks if the agent info is valid. An invalid agent info can be reinitialized at will.
	bool IsValid();

	//Agent info are considered equal if they have the same world object
	bool operator==(const CAI_AgentInfo& _Compare);

	//Check if agent fulfills the given criteria under the given knowledge base
	bool Check(const CAI_AgentCriteria& _Criteria);

	//Info flag specific checking methods. These will update the flag and return the value.
	//The InLOS method sets the optionally provided position to the LOS "hit" if LOS is checked this frame
	void SetLOS(bool _InLOS);
	bool InLOS(CVec3Dfp32* _pLOSPos = NULL);

	bool DoesMeleeBlow(int _Tick);		// Is swinging blows, not neccesarily against us
	void ClearMeleeBlow();
	bool PositionInLOS();


	//Update the given info flags and check if they are all set
	bool CheckInfoFlags(int _UpdateFlags);

	// Returns current awareness without updating
	int32 GetCurAwareness();
	// Returns a timestamp since cur awareness, updated by Touch(), OnALarm messages etc
	int32 GetCurAwarenessTimer();
	// Retuns nbr of ticks since we actually spotted the target, NOT affected by Touch(), OnAlarm() etc
	int32 GetGetLastSpottedTicks();
	void TouchLastSpotted();

	//Update and get awareness 
	int GetAwareness(bool _bUpdateNow = false);

	//Update and get awareness of us
	int GetAwarenessOfUs();

	//Update and get relation
	int GetRelation(bool _bUpdateNow = false);

	// Get relation but don't update
	int GetCurRelation();
	// Get pending relation or if none is coming get current. Don't update
	int GetCurOrPendingRelation();

	//Update and get position (or suspected position if there's uncertainty)
	CVec3Dfp32 GetBasePos();
	CVec3Dfp32 GetHeadPos();

	//Get path position or position if no path position have been set.
	CVec3Dfp32 GetPathPos();

	//Set given info to given value. Note that no other info flags are affected.
	void SetInfo(int _Info, bool _Val);

	//Set awareness to given value. If this means awareness will be raised, the change will 
	//be delayed by reaction time. If _RaiseFriends is true RaiseAwareness will be called ie
	//we will tell our team freinds what we know
	void SetAwareness(int _Awareness,bool _RaiseFriends,bool _RightNow = false);

	void Touch();

	// Forces a relation to not gow down due to time
	void TouchRelation();

	//Immediately set relation to given value
	void SetRelation(int _Relation,int _Delay = 0);

	//Set suspected position to _Pos
	void SetSuspectedPosition(CVec3Dfp32& _Pos);
	fp32 GetSuspectedHeight();
	const CVec3Dfp32& GetSuspectedPosition();
	CVec3Dfp32 GetSuspectedTorsoPosition();
	CVec3Dfp32 GetSuspectedHeadPosition();
	//Set suspected position to the current pos of the object
	void SetCorrectSuspectedPosition();
	CVec3Dfp32 GetCorrectPosition();

	//Get corresponding object index and object pointer, respectively
	int GetObjectID();
	CWObject* GetObject();
	CAI_Core* GetAgentAI();

	//Savegame
	void OnDeltaLoad(CCFile* _pFile);
	void OnDeltaSave(CCFile* _pFile);
};

//Class for specifying criteria when finding/checking agents
class CAI_AgentCriteria
{
protected:
	//Info flags that must/cannot be set (see CAI_AgentInfo info flag enum)
	int m_InfoFlagsTrue;
	int m_InfoFlagsFalse;

	//Awareness must be equal to or higher/lower than this (see CAI_AgentInfo awareness enum)
	int m_MinAwareness;
	int m_MaxAwareness;

	//Agents awareness of us must be equal to or higher/lower than this (see CAI_AgentInfo awareness enum)
	int m_MinAwarenessOfUs;
	int m_MaxAwarenessOfUs;

	//We must have this or worse/better relation to agent (see CAI_AgentInfo relation enum)
	int m_MinRelation;
	int m_MaxRelation;
	
	//Agent must be closer to and/or farther away from this position than the below values
	CVec3Dfp32 m_CheckPos;
	fp32 m_MaxDistanceSqr;
	fp32 m_MinDistanceSqr;

	//Only consider distance in the xy-plane for the distance criteria
	bool m_bXYDistOnly;

	//Agent info can look at the above
	friend class CAI_AgentInfo;

public:
	CAI_AgentCriteria();
	
	/////////////////////////////////////////////////////////////////////////////////
	//Info flags, awareness and relation values are enum values from CAI_AgentInfo
	/////////////////////////////////////////////////////////////////////////////////

	//Add info flags criteria that must hold if and optionally flags that cannot hold if
	void InfoFlags(int _TrueFlags, int _FalseFlags = CAI_AgentInfo::INFOFLAGS_NONE);
	
	//Add min and optionally max awareness criteria 
	void Awareness(int _Min, int _Max = CAI_AgentInfo::AWARENESS_INVALID);

	//Add min and optionally max awareness of us criteria 
	void AwarenessOfUs(int _Min, int _Max = CAI_AgentInfo::AWARENESS_INVALID);

	//Add min and optionally max relation criteria 
	void Relation(int _Min, int _Max = CAI_AgentInfo::RELATION_INVALID);

	//Add proximity and optionally remoteness criteria. If the _bStrict flag is set any 
	//uncertainties will cause the agent to fail this criteris if possible, otherwise uncertainties 
	//will cause criteria to hold if possible. If the _bXYOnly value is set, only distances in the
	//xy-plane will be considered
	void Distance(const CVec3Dfp32& _Pos,  fp32 _WithinDist, fp32 _OutsideDist = 0, bool _bStrict = false, bool _bXYOnly = false);
};

// Helper struct for storing death related info
struct SDead
{
	// The classes of deaths available
	enum
	{
		DEATHCAUSE_UNKNOWN = 0,
		DEATHCAUSE_DARKNESS,
		DEATHCAUSE_CUT,
		DEATHCAUSE_SHOT,
		DEATHCAUSE_MOVED,
	};

	SDead()
	{
		m_bFound = false;
		m_bInvestigated = false;
		m_CauseOfDeath = DEATHCAUSE_UNKNOWN;
		m_iCause = 0;
		m_iVictim = 0;
		m_Relation = CAI_AgentInfo::UNKNOWN;
		m_TimeOfDeath = 0;
		m_DeathPos = CVec3Dfp32(_FP32_MAX);
	};

	bool m_bInvestigated;	// Wether the death has been investigated or not, only relevant if
	bool m_bFound;			// Wether we've found the dead or not
	int8 m_CauseOfDeath;

	// m_iVictim is a teammember and we have InvestigateDeath action
	int	m_iCause;			// Object id of murderer if any
	int	m_iVictim;			// Object id of victim
	int m_Relation;			// Victims relation to us, important as different actions may have
							// different criteria for interesting corpses
	int	m_TimeOfDeath;		// Time of death
	CVec3Dfp32 m_DeathPos;	// Position where the victim died, dead guys can be dragged!
};

//Class that handles all world knowledge of an AI
class CAI_KnowledgeBase
{
public:
	//Public constants

	//Alertness states
	enum{
		ALERTNESS_INVALID = -1,
		ALERTNESS_OBLIVIOUS = 0,	//No perception whatsoever 
		ALERTNESS_SLEEPING,			//Low hearing, no other perception. Can only notice stuff.
		ALERTNESS_DROWSY,			//Lowered perception. Can only detect after noticing and only spot after detecting stuff.
		ALERTNESS_IDLE,				//Normal perception. Can only spot after detecting stuff. 
		ALERTNESS_WATCHFUL,			//Normal perception. 
		ALERTNESS_JUMPY,			//Normal perception. Will notice or detect imaginary stuff at times.
	};

	//String translators (defined at top of .cpp file)
	static const char * ms_TranslateAlertness[];

	void Global_ClearDead();

protected:
	// The currently closest agent and the squared range to him
	uint16						m_iCollAgent;
	uint16						m_iCollAgent2;
	uint16						m_iSkipCollAgent;
	bool						m_bCollAgentChanged;

protected:

	//Simple class for keeping track of knowledge base agent iterations
	class CAgentIteration 
	{
	public:
		//List to iterate over
		enum {
			ALL_AGENTS,
			ENEMIES,
		};
		int m_List;

		//Current iteration index
		int m_Cur;

		//Criteria
		CAI_AgentCriteria m_Criteria;

		//Own index
		int m_iIndex;

		CAgentIteration();

		//Considered equal if same index
		bool operator==(const CAgentIteration& _Compare);
	
		//Get best list, based on agent criteria
		static int RecommendedList(const CAI_AgentCriteria& _Criteria);
	};

protected:
	friend class CAI_AgentInfo;

	//Local attributes

	//Pointer to owning AI-object
	CAI_Core* m_pAI;

	//Agent info for all interesting agents
	TSimpleDynamicList<CAI_AgentInfo> m_lAgents;

	//Object ID -> Agent list index mapping hash
	TIntHashMap<int> m_AgentObjIDMap;

	//List of potential enemy agent's indices into m_lAgents for faster iteration when scouting etc.
	CSimpleIntList m_lEnemies;	

	//Current iterations
	TSimpleDynamicList<CAgentIteration> m_lAgentIterations;

	//The last server tick when bot was attacked. -1 if never attacked.
	int m_LastAttackedTimer;
	int m_LastHurtTimer;

	//The object id of last attacker
	int16 m_iLastAttacker; 
	int16 m_iLastMeleeNonEnemyFighter;		// Last non enemy agent that was melee fighting (not neccessarily with us)
	int m_LastMeleeNonEnemyFighterTimer;

	// Current noiselevel
	fp32 m_CurNoise;
	fp32 m_CurLight;

	//Some useful info flags and corresponding validity flags
	//The use of the validity flags means the knowledge need not be computed needlessly over and over again
	enum {
		PLAYER_ALLY			= 1,
		ENEMY_SPOTTED		= 2,
		ENEMY_DETECTED		= 4,
		IN_MELEE			= 8,
		UNDER_ATTACK		= 16,
	};
	int8 m_Knowledge;
	int8 m_ValidKnowledge;

	//Bot current and default alertness state
	int8 m_Alertness;
	int8 m_DefaultAlertness;
	int8 m_InitialAlertness;
	int8 m_NEnemies;				// Current nbr of enemies
	int8 m_NHostiles;				// Current nbr of hostiles
	int8 m_NFriends;				// Current nbr of friends
	int8 m_NInvestigates;			// Current nbr of investigation targets

	uint16 m_iDarknessPlayer;		// Obj ID of Darkness player, 0 when player is not using darkness (visibly)
	uint16 m_DarknessPowers;		// Darknesspowers active at the moment
	int32 m_iCreepingDark;			// m_iCreepingDark != 0 if creepingdarkness is out
	CVec3Dfp32 m_DarknessPos;		// Valid pos when m_iCreepingDark != 0

	//Squared range where most alertness awareness mods cease to apply
	fp32 m_AlertnessRangeSqr;

protected:
	//Methods

	//An agent is a potential enemy if he's attacking us, is a player of a team other than our own or 
	//we have a bad relationship with him
	bool IsPotentialEnemy(CAI_AgentInfo * _Info);

	//Length of given list (param is CAgentIteration list type enum)
	int ListLength(int _List);

public:
	//Constructor. 
	CAI_KnowledgeBase();

	void SetAI(CAI_Core * _pAI);

	//Change AI user
	void ReInit(CAI_Core * _pAI);

	//Initialize knowledge base. Run this before first refresh.
	void Init();

	//Savegame
	void OnDeltaLoad(CCFile* _pFile);
	void OnDeltaSave(CCFile* _pFile);
	void OnPostWorldLoad();

	// Handles AI vs AI collisions
	bool CheckCollisions();
	int GetCollider();
	bool DidCollAgentChange();
	void SkipCollChecking(int32 _iSkip);

	// Pick a random agent inside _Rng with _MinAwareness and _MaxRelation
	CAI_AgentInfo* GetRandomAgentInRange(fp32 _Rng, int _MinAwareness = CAI_AgentInfo::SPOTTED, int _MaxRelation = CAI_AgentInfo::UNFRIENDLY);

	//Refresh world knowledge status. Most actual knowledge isn't updated until it's asked for.
	void OnRefresh();
	void RefreshNoise();
	void RefreshLight();
	void OnRefreshDarkness();
	void AddLoudness(int _Loudness);
	void AddLight(int _Light);

	//Notify knowledge base that a new agent has come into play. Knowledge base will add agent to set of 
	//interesting agents if appropriate. Returns pointer to newly added agent info if added, otherwise NULL
	CAI_AgentInfo * NewAgent(int _iObj, bool _bAddGlobal = false);
	
	//Adds agent to the set of interesting agents that knowledge base keeps track of, with given info etc
	//Returns pointer to agent info or NULL if agent was invalid.
	CAI_AgentInfo * AddAgent(int _iObj, int _Info = CAI_AgentInfo::INFOFLAGS_NONE, int _Awareness = CAI_AgentInfo::NONE, int _AwarenessOfUs = CAI_AgentInfo::NONE, int _Relation = CAI_AgentInfo::UNKNOWN);

	// Get agent with best awareness value
	CAI_AgentInfo* GetBestAwareness(int _MinRelation = CAI_AgentInfo::NEUTRAL);

	//Check if given agent fulfil the given criteria.
	bool CheckAgent(int _iObj, const CAI_AgentCriteria& _Criteria);

	// Returns iObj of nearest and returns the squared range to him
#define MAX_TEAMMATE_SPEAK_RANGE	512.0f
#define MAX_DIALOGUE_CROWD_RANGE	160.0f
	fp32 CanSpeakTeammate(fp32 _MaxRange = MAX_TEAMMATE_SPEAK_RANGE);

	//Find first agent that fulfil the given criteria. Returns NULL if no such agent can be found.
	CWObject* FindAgent(const CAI_AgentCriteria& _Criteria);

	//Set up agent iteration with given criteria, for use with the FindNextAgent method
	//Returns iteration id.
	int BeginAgentIteration(const CAI_AgentCriteria& _Criteria);

	//Remove given agent iteration. Should be called when iteration has been finished.
	void EndAgentIteration(int _Iteration);

	//Find the next agent that fulfil the criteria of the given iteration, or NULL if 
	//there are no more such agents
	CWObject * FindNextAgent(int _Iteration);

	//Find all agents with that fulfil the given criteria and add them to given list. Returns number of found agents.
	int FindAllAgents(const CAI_AgentCriteria& _Criteria, TArray<CWObject*> * _plRes);

	//Get agent info for agent with given object id, or NULL if there is no such agent
	CAI_AgentInfo * GetAgentInfo(int _iObj);
	
	//Get number of valid agent infos
	int NumAgentInfo();

	//Get i'th agent info. Returns NULL if agent info is invalid
	CAI_AgentInfo * IterateAgentInfo(int _i);

	//Get pointer to AI that this knowledge base belongs to
	CAI_Core * GetAI();

	/* Deprecated
	//Get the number of frames since the bot was last attacked. Return -1 if ever attacked.
	int GetAttackedCount();
	*/

	//Have we been attacked within the given number of frames? If no time is specified, then 
	//this returns true if we've ever been attacked.
	bool WasAttacked(int _Time = -1);

	// Returns the nbr current nbr of detected enemies
	int GetDetectedEnemyCount();
	
	// Returns the object index of the player if using darkness, 0 otherwise
	int GetPlayerDarkness();
	// Returns true and the darkness pos if valid
	bool GetDarknessPos(CVec3Dfp32* _pDarknessPos);
	// Returns true and the index of the creeping dark if valid
	bool GetCreepingDark(int32* _piCreepDark);
	//Get agent info of last attacker or NULL if never attacked or last attacker is invalid (dead)
	CAI_AgentInfo* GetLastAttacker();
	// Returns the index to the last non enemy agent we increased(worsened) relation because of melee fighting
	// if the increase was done in the last 3 seconds
	CAI_AgentInfo* GetLastMeleeRelationIncrease();
	// Returns the index to the last non enemy agent we saw fighting melee within 3 secs
	CAI_AgentInfo* GetLastNonEnemeyMeleeFighter();
	void ClearLastNonEnemeyMeleeFighter();

	//Handles the event of us taking damage
	void OnTakeDamage(int _iAttacker, int _Damage = 0, uint32 _DamageType = DAMAGETYPE_UNDEFINED);

	//Are we a player ally?
	bool IsPlayerAlly();

	//Updates relation for all potential enemies and awareness for all enemies. Succeeds if 
	//there was any spotted enemies
	bool OnSpotEnemies();
	// Ditto for detected enemies, used by FindTarget() etc
	bool OnDetectEnemies();

	//Our default relation with this agent (Value is one of agent info relation enum)
	int DefaultRelation(int _iObj);

	//Set alertness level. If the default flag is true this alertness is the new default alertness
	void SetAlertness(int _Alertness, bool _bDefault = false);
	void SetAlertness(CStr _Alertness, bool _bDefault = false);

	//Get alertness level, or default alertness level if the flag is set
	int GetAlertness(bool _bDefaultValue = false);

	//Specify which alertness should be set when AI is spawned
	void SetInitialAlertness(CStr _Alertness);
	int GetInitialAlertness();

	//Set alertness range and get squared alertness range
	void SetAlertnessRange(fp32 _Range);
	fp32 GetAlertnessRangeSqr();

	//Update alertness level
	void OnRefreshAlertness();

	//Handle NOTICE,DETECT or SPOT caused by _iCause and sent by _iRaiser
	void OnAware(int _iCause, int _iRaiser, int _Awareness,CVec3Dfp32 _SuspectedPos,int _Relation = CAI_AgentInfo::UNKNOWN);

	//Handle alarm caused by given object and raising object at given alarm level (see CWObject_Team)
	void OnAlarm(int _iCause, int _iRaiser, int _Relation = CAI_AgentInfo::RELATION_INVALID);

	//Determine wether our bot can see iCause and if so, set relation to _Relation
	void OnFight(int _iCause, int _iRaiser, int _Relation = CAI_AgentInfo::RELATION_INVALID);	

#ifndef M_RTM
	//Get some debug info about the bot
	virtual CStr GetDebugString();
#endif

public:
	// Returns nbr of ticks since last hurt
	int GetLastHurtTicks();
	void UpdateLastHurtTick();

	//Get required security level of given agent. This is based on any restricted zones the 
	//agent is in as well as whether we he's fighting or carrying weapons etc.
	int GetSecurityLevel(int _iObj);

	//Calculate visibility modifiers for the given object from our perspective
	fp32 GetVisibility(CWObject * _pObj);	

	//Returns the noise based on items that has been used etc
	fp32 GetNoise();

	//Returns the light based on items that has been used etc
	fp32 GetLight();

	// Internal method to get loudness data from char
	fp32 GetLoudness(CWObject* _pObj);
	
public:
	//Global world info handling

	//Get pointer to NonZeroList of all spawned agents
	//Don't modify agent list outside of knowledgebase! (i.e. use the CAI_KnowledgeBase::NewAgent method only when adding agents)

	//Add agent to global world info
	void Global_NewAgent(int _iObj);

	//Remove agent from global world info
	void Global_RemoveAgent(int _iObj);

	void Global_ReportBrokenLight(int _iLight);
	void Global_SetCareAboutBrokenLights();
	void ReportBrokenLight(int _iLight);
	CVec3Dfp32 GetBrokenLightPos(bool _bRemove = true);

	// MEMENTO MORI
	// Tell the global lists (later the CAI_Manager object) that we're interested in receiving
	// news on ai deaths
	// If _bRemove == true we are no longer interested in deaths
	void Global_SetCareAboutDeath(int _Relation);
	void Global_RemoveCareAboutDeath(int _iObj);
	// Report to all interested bots that _iVictim just died at _Pos, killed by _iCause
	void Global_ReportDeath(int _iCause,int _iVictim,CVec3Dfp32 _Pos,int8 _CauseOfDeath);
	// Called when reporting to this bot about a death
	void ReportDeath(int _iCause,int _iObj,CVec3Dfp32 _DeathPos,int8 _CauseOfDeath);
	void ReportDeath(SDead _Dead);

	// Searches for a dead that fulfills the supplied search criteria
	// _Relation: Relation to match:
	// UNKNOWN: All are eligible
	// FRIENDLY: Friendly only
	// NEUTRAL: All but neutral
	// UNFRIENDLY: Unfriendy or worse
	// HOSTILE: Hostile or worse
	// HOSTILE: Hostile or worse
	// HOSTILE_1: Hostile 1 or worse
	// HOSTILE_2: Hostile 2 or worse
	// ENEMY: Enemy or worse if that is possible
	// _MinRange,_MaxRange: Range criteria from caller
	// _bUninvestigated: Only uninvestigated are considered when this is true
	// returns true when a valid dead is found (returned in _pDead if nonnull)
	bool GetFirstMatchingDead(SDead* _pDead,int _Relation,fp32 _MinRange,fp32 _MaxRange,bool _bUninvestigated,bool _bUnFound = true);
	// Returns 0 if no corpse was detected
	// Returns 1 if corpse was found but no murderer
	// Returns 2 if corpse and murderer was found (_pDead->m_iCause is the murderer)
	int CheckDead(SDead* _pDead,int _Relation,fp32 _MinRange,fp32 _MaxRange,int _MinAwareness,bool _bRemoveSpotted);
	void SetInvestigated(int _iVictim);
	int NbrOfValidDead();
	bool IsValidDead(int _iDead,bool _bUninvestigated = false);
	// Sets the appropriate idlestance and makes _iPerp enemy within 2 seconds
	// Don't supply _iPerp unless you should know _iPerp is the murderer
	void FoundCorpse(int _iPerp,int _iVictim,CVec3Dfp32 _Pos);

	void Global_RemoveDead(int _iCorpse);
	bool RemoveDead(int _iCorpse);
	void CleanStatic();

protected:
	// m_lDead: List of dead characters
	TArray<SDead>		m_lDead;
	TArray<int>			m_liBrokenLights;
	int8				m_iDeadFinger;				// Last checked m_lDead index
	int8				m_iBrokenLightsFinger;		// Last checked m_liBrokenLights index
	int16				m_NbrOfValidDead;			// Recalc when -1
};

#endif

