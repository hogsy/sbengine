#ifndef _INC_AI_RESOURCEHANDLER
#define _INC_AI_RESOURCEHANDLER

#include "AI_Auxiliary.h"
#include "AI_KnowledgeBase.h"

class CAI_ActionDefinition;

// CAI_ResourceActions holds all globals for CAI_Action and its children
class CAI_ResourceAction
{
public:
	CAI_ResourceAction()
	{
		if (!ms_IdleCallSemaphore.IsInitialized())
		{
			ms_IdleCallSemaphore.Init(1,1,1);
		}
		if (!ms_CheckDeadSemaphore.IsInitialized())
		{
			ms_CheckDeadSemaphore.Init(1,1,1);
		}
		if (!ms_InvestigateSemaphore.IsInitialized())
		{
			ms_InvestigateSemaphore.Init(1,1,1);
		}
		if (!ms_MeleeFightSemaphore.IsInitialized())
		{
			ms_MeleeFightSemaphore.Init(1,1,1);
		}
		if (!ms_BerserkerSemaphore.IsInitialized())
		{
			ms_BerserkerSemaphore.Init(1,1,1);
		}
	};

	~CAI_ResourceAction()
	{
		ms_IdleCallSemaphore.Destroy();
		ms_CheckDeadSemaphore.Destroy();
		ms_InvestigateSemaphore.Destroy();
		ms_MeleeFightSemaphore.Destroy();
		ms_BerserkerSemaphore.Destroy();
	};

	void CleanStatic()
	{
		ms_IdleCallSemaphore.ReleaseAll();
		ms_CheckDeadSemaphore.ReleaseAll();
		ms_InvestigateSemaphore.ReleaseAll();
		ms_MeleeFightSemaphore.ReleaseAll();
		ms_BerserkerSemaphore.ReleaseAll();
	};

	CAI_Resource_Activity ms_IdleCallSemaphore;
	CAI_Resource_Activity ms_CheckDeadSemaphore;
	CAI_Resource_Activity ms_InvestigateSemaphore;
	CAI_Resource_Activity ms_MeleeFightSemaphore;
	CAI_Resource_Activity ms_BerserkerSemaphore;
};

// CAI_Resource_KnowledgeBase holds all globals used by CAI_KnowledgeBase
class CAI_Resource_KnowledgeBase
{
public:
	CAI_Resource_KnowledgeBase()
	{
		ms_LastAgentCleaningTick = 0;
		ms_lAgents.Create(0);
		ms_DarknessPos = CVec3Dfp32(_FP32_MAX);
		ms_iDarknessPlayer = 0;
		ms_DarknessPowers = 0;
		ms_DarknessCount = 0;
		ms_DarknessPosCount= 0;
		ms_iCreepingDark = 0;
		ms_JumpTick = 0;
	};

	void CleanStatic()
	{
		ms_lAgents.Destroy();
		ms_lDead.Destroy();
		ms_liInterestedInDeath.Destroy();
		ms_lInterestedInDeathRelations.Destroy();
		ms_liInterestedInBrokenLights.Destroy();
		ms_iDarknessPlayer = 0;
		ms_DarknessPowers = 0;
		ms_DarknessPos = CVec3Dfp32(_FP32_MAX);
		ms_DarknessCount = 0;
		ms_DarknessPosCount= 0;
		ms_iCreepingDark = 0;
		ms_JumpTick = 0;
	};

	CSimpleIntList	ms_lAgents;
	int				ms_LastAgentCleaningTick;
	TArray<SDead>	ms_lDead;
	TArray<int>		ms_liInterestedInDeath;
	TArray<int>		ms_lInterestedInDeathRelations;
	TArray<int>		ms_liInterestedInBrokenLights;
	uint16			ms_iDarknessPlayer;	// Obj nbr of player or 0 when player is not (visibly) using darkness
	uint16			ms_DarknessPowers;	// What darknesspower(s) is in use
	uint16			ms_DarknessCount;	// Nbr of ticks remaining of showing darkness
	uint16			ms_DarknessPosCount;// Nbr of ticks remaining of ms_DarknessPos validity
	uint16			ms_iCreepingDark;	// Obj nbr of creephead when using creeping dark, otherwise 0, even when using other darkness powers
	CVec3Dfp32		ms_DarknessPos;		// Pos of darkness when using creeping dark (or tentacle)
	int				ms_JumpTick;
};

//Resource handler for all global AI stuff. This is currently a part of game object.
class CAI_ResourceHandler
{
public:
	//Global pause flags. The flags set will determine how much the AI is allowed to do.
	//Specific pause flags should be used to handle "overlapping" pause situations, such as when a dialogue ends when a cutscene is playing. 
	//If the same pause flag would be used by both cutscenes and dialogues in such a case, the pause would be removed in the middle of the cutscene!
	enum {
		PAUSE_GENERAL	= 0x1,	//Pauses all non-scripted actions. Debug pause etc. Use when pause should be allowed to be set and removed arbitrarily
		PAUSE_CUTSCENE	= 0x2,	//Pauses all non-scripted actions when in cutscene
		PAUSE_DIALOGUE	= 0x4,	//... when in dialogue mode
		PAUSE_ACS		= 0x8,	//... when in action cutscene
		PAUSE_SCRIPT	= 0x16,	//... when set by script 
	};

protected:
	//Classes

	class CNameList
	{
	public:
		TArray<CStr> m_lNames;
		CNameList(){m_lNames.Clear();};
		bool operator==(const CNameList& _Val);
	};

protected:
	//Attributes

	//Action definitions
	TArray<CAI_ActionDefinition> m_lActionDefinitions;

	//Weapon types mapped to action definition name lists
	TIntHashMap<CNameList> m_lWeaponHandlerMap;

	//Global pause flags. The flags set will determine how much the AI is allowed to do.
	int8 m_PauseFlags;

public:
	CAI_ResourceHandler();
	~CAI_ResourceHandler();
	void Init(CWorld_Server* _pServer);

	int GetAITick();

	//Handle registry keys
	void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	//Get pointer to action definition for given name (or NULL if there isn't any)
	CAI_ActionDefinition* GetActionDefinition(CStr _Str); 

	//Add all appropriate action names for the given weapon type to given list.
	//Fail if there are no action definitions for that type
	bool GetWeaponHandlerActions(int _WeaponType, TArray<CStr> * _plRes);

	//Set specific pause flags
	void Pause(int _PauseFlags = PAUSE_GENERAL);

	//Remove specific pause flags
	void Unpause(int _PauseFlags = PAUSE_GENERAL);

	//Clear all pause flags
	void UnpauseAll();

	//Check if AI is allowed to tale non-scripted actions
	bool AllowActions();

	void OnRefresh();

	//Load/save stuff
	void OnDeltaSave(CCFile *_pFile);
	void OnDeltaLoad(CCFile *_pFile);

	void CleanStatic();

	// Ptr to server
	CWorld_Server* m_pServer;

	// Knowledge base globals
	CAI_Resource_KnowledgeBase m_KBGlobals;

	// Action globals
	CAI_ResourceAction m_ActionGlobals;

	// Action handler globals
	// AI globals
	int ms_ServerTick;
	fp32 ms_ServerTickFraction;
	int ms_Gamestyle;
	int ms_GameDifficulty;
	int ms_Tension;
	CAI_Resource_Pathfinding ms_PFResource_Grid;
	CAI_Resource_Pathfinding ms_PFResource_Graph;
	CAI_Resource_Activity ms_FlashlightUsers;
	CAI_ActivityCounter ms_ActivityCounter;
	//AI debug rendering flag
	bool ms_bDebugRender;
	int ms_iDebugRenderTarget;
};



//Action definition: default parameters for actions
class CAI_ActionDefinition
{
private:
	//Parameter value lists during OnEvalKey (destroyed after OnFinishedEvalKeys, when below values are set)
	TArray<spCRegistry> m_lKeys;

	//Parameter value lists
	TArray<int> m_lInts;
	TArray<fp32> m_lFloats;
	TArray<CVec3Dfp32> m_lVecs;
	TArray<CStr> m_lStrings;

	//List type stuff
	enum {
		INDEX_SHIFT = 3,					  //List index >> 3 is actual index in list
		LIST_MASK	= (1 << INDEX_SHIFT) - 1, //First three bits identifies list (see CAI_Action PARAMTYPE enum)
	};

	//Parameter type -> value list index mapping
	TIntHashMap<int> m_lParamMap;

	//Registry name
	CStr m_Name;

	//Action type
	int m_Type;

	//This definition will use parent's values if own values are undefined
	CAI_ActionDefinition * m_pParent;

public:
	CAI_ActionDefinition();

	//Create action definition from registry key, given AI resource handler
	void Create(const CRegistry* _pKey, CAI_ResourceHandler * _pAIRes);

	//Handle registry keys
	void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey, CAI_ResourceHandler * _pAIRes);

	//Set parameter values properly and remove junk
	void OnFinishedEvalKeys();

	//Check if name matches given string
	bool MatchName(CStr _Str);	

	//Type accessor
	int GetType();

	//Parameter accessors; set given value to value of given parameter if defined 
	//or fail if not defined
	bool GetParam(int _Param, int& _Val);
	bool GetParam(int _Param, fp32& _Val);
	bool GetParam(int _Param, CVec3Dfp32& _Val);
	bool GetParam(int _Param, CStr& _Val);

	//Get all of the parameter IDs and corresponding types into given arrays. 
	//Return the number of params.
	int GetParamIDs(TArray<int>* _plParams, TArray<int>* _plTypes);
};


#endif
